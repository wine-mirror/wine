/*
 * Service process to load a kernel driver
 *
 * Copyright 2007 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winreg.h"
#include "winnls.h"
#include "winsvc.h"
#include "ddk/wdm.h"
#include "wine/rbtree.h"
#include "wine/svcctl.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winedevice);
WINE_DECLARE_DEBUG_CHANNEL(relay);

extern NTSTATUS CDECL wine_ntoskrnl_main_loop( HANDLE stop_event );

static SERVICE_STATUS_HANDLE service_handle;
static SC_HANDLE manager_handle;
static HANDLE stop_event;

struct wine_driver
{
    struct wine_rb_entry entry;

    SERVICE_STATUS_HANDLE handle;
    DRIVER_OBJECT *driver_obj;
    WCHAR name[1];
};

static struct wine_rb_tree wine_drivers;

static void *wine_drivers_rb_alloc( size_t size )
{
    return HeapAlloc( GetProcessHeap(), 0, size );
}

static void *wine_drivers_rb_realloc( void *ptr, size_t size )
{
    return HeapReAlloc( GetProcessHeap(), 0, ptr, size );
}

static void wine_drivers_rb_free( void *ptr )
{
    HeapFree( GetProcessHeap(), 0, ptr );
}

static int wine_drivers_rb_compare( const void *key, const struct wine_rb_entry *entry )
{
    const struct wine_driver *driver = WINE_RB_ENTRY_VALUE( entry, const struct wine_driver, entry );
    return strcmpW( (const WCHAR *)key, driver->name );
}

static const struct wine_rb_functions wine_drivers_rb_functions =
{
    wine_drivers_rb_alloc,
    wine_drivers_rb_realloc,
    wine_drivers_rb_free,
    wine_drivers_rb_compare,
};

/* find the LDR_MODULE corresponding to the driver module */
static LDR_MODULE *find_ldr_module( HMODULE module )
{
    LDR_MODULE *ldr;
    ULONG_PTR magic;

    LdrLockLoaderLock( 0, NULL, &magic );
    if (LdrFindEntryForAddress( module, &ldr ))
    {
        WARN( "module not found for %p\n", module );
        ldr = NULL;
    }
    LdrUnlockLoaderLock( 0, magic );

    return ldr;
}

/* load the driver module file */
static HMODULE load_driver_module( const WCHAR *name )
{
    IMAGE_NT_HEADERS *nt;
    const IMAGE_IMPORT_DESCRIPTOR *imports;
    SYSTEM_BASIC_INFORMATION info;
    int i;
    INT_PTR delta;
    ULONG size;
    HMODULE module = LoadLibraryW( name );

    if (!module) return NULL;
    nt = RtlImageNtHeader( module );

    if (!(delta = (char *)module - (char *)nt->OptionalHeader.ImageBase)) return module;

    /* the loader does not apply relocations to non page-aligned binaries or executables,
     * we have to do it ourselves */

    NtQuerySystemInformation( SystemBasicInformation, &info, sizeof(info), NULL );
    if (nt->OptionalHeader.SectionAlignment < info.PageSize ||
        !(nt->FileHeader.Characteristics & IMAGE_FILE_DLL))
    {
        DWORD old;
        IMAGE_BASE_RELOCATION *rel, *end;

        if ((rel = RtlImageDirectoryEntryToData( module, TRUE, IMAGE_DIRECTORY_ENTRY_BASERELOC, &size )))
        {
            WINE_TRACE( "%s: relocating from %p to %p\n",
                        wine_dbgstr_w(name), (char *)module - delta, module );
            end = (IMAGE_BASE_RELOCATION *)((char *)rel + size);
            while (rel < end && rel->SizeOfBlock)
            {
                void *page = (char *)module + rel->VirtualAddress;
                VirtualProtect( page, info.PageSize, PAGE_EXECUTE_READWRITE, &old );
                rel = LdrProcessRelocationBlock( page, (rel->SizeOfBlock - sizeof(*rel)) / sizeof(USHORT),
                                                 (USHORT *)(rel + 1), delta );
                if (old != PAGE_EXECUTE_READWRITE) VirtualProtect( page, info.PageSize, old, &old );
                if (!rel) goto error;
            }
            /* make sure we don't try again */
            size = FIELD_OFFSET( IMAGE_NT_HEADERS, OptionalHeader ) + nt->FileHeader.SizeOfOptionalHeader;
            VirtualProtect( nt, size, PAGE_READWRITE, &old );
            nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = 0;
            VirtualProtect( nt, size, old, &old );
        }
    }

    /* make sure imports are relocated too */

    if ((imports = RtlImageDirectoryEntryToData( module, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &size )))
    {
        for (i = 0; imports[i].Name && imports[i].FirstThunk; i++)
        {
            char *name = (char *)module + imports[i].Name;
            WCHAR buffer[32], *p = buffer;

            while (p < buffer + 32) if (!(*p++ = *name++)) break;
            if (p <= buffer + 32) FreeLibrary( load_driver_module( buffer ) );
        }
    }

    return module;

error:
    FreeLibrary( module );
    return NULL;
}

/* load the .sys module for a device driver */
static HMODULE load_driver( const WCHAR *driver_name, const UNICODE_STRING *keyname )
{
    static const WCHAR driversW[] = {'\\','d','r','i','v','e','r','s','\\',0};
    static const WCHAR systemrootW[] = {'\\','S','y','s','t','e','m','R','o','o','t','\\',0};
    static const WCHAR postfixW[] = {'.','s','y','s',0};
    static const WCHAR ntprefixW[] = {'\\','?','?','\\',0};
    static const WCHAR ImagePathW[] = {'I','m','a','g','e','P','a','t','h',0};
    HKEY driver_hkey;
    HMODULE module;
    LPWSTR path = NULL, str;
    DWORD type, size;

    if (RegOpenKeyW( HKEY_LOCAL_MACHINE, keyname->Buffer + 18 /* skip \registry\machine */, &driver_hkey ))
    {
        WINE_ERR( "cannot open key %s, err=%u\n", wine_dbgstr_w(keyname->Buffer), GetLastError() );
        return NULL;
    }

    /* read the executable path from memory */
    size = 0;
    if (!RegQueryValueExW( driver_hkey, ImagePathW, NULL, &type, NULL, &size ))
    {
        str = HeapAlloc( GetProcessHeap(), 0, size );
        if (!RegQueryValueExW( driver_hkey, ImagePathW, NULL, &type, (LPBYTE)str, &size ))
        {
            size = ExpandEnvironmentStringsW(str,NULL,0);
            path = HeapAlloc(GetProcessHeap(),0,size*sizeof(WCHAR));
            ExpandEnvironmentStringsW(str,path,size);
        }
        HeapFree( GetProcessHeap(), 0, str );
        if (!path)
        {
            RegCloseKey( driver_hkey );
            return NULL;
        }

        if (!strncmpiW( path, systemrootW, 12 ))
        {
            WCHAR buffer[MAX_PATH];

            GetWindowsDirectoryW(buffer, MAX_PATH);

            str = HeapAlloc(GetProcessHeap(), 0, (size -11 + strlenW(buffer))
                                                        * sizeof(WCHAR));
            lstrcpyW(str, buffer);
            lstrcatW(str, path + 11);
            HeapFree( GetProcessHeap(), 0, path );
            path = str;
        }
        else if (!strncmpW( path, ntprefixW, 4 ))
            str = path + 4;
        else
            str = path;
    }
    else
    {
        /* default is to use the driver name + ".sys" */
        WCHAR buffer[MAX_PATH];
        GetSystemDirectoryW(buffer, MAX_PATH);
        path = HeapAlloc(GetProcessHeap(),0,
          (strlenW(buffer) + strlenW(driversW) + strlenW(driver_name) + strlenW(postfixW) + 1)
          *sizeof(WCHAR));
        lstrcpyW(path, buffer);
        lstrcatW(path, driversW);
        lstrcatW(path, driver_name);
        lstrcatW(path, postfixW);
        str = path;
    }
    RegCloseKey( driver_hkey );

    WINE_TRACE( "loading driver %s\n", wine_dbgstr_w(str) );

    module = load_driver_module( str );
    HeapFree( GetProcessHeap(), 0, path );
    return module;
}

/* call the driver init entry point */
static NTSTATUS WINAPI init_driver( DRIVER_OBJECT *driver_object, UNICODE_STRING *keyname )
{
    unsigned int i;
    NTSTATUS status;
    const IMAGE_NT_HEADERS *nt;
    const WCHAR *driver_name;
    HMODULE module;

    /* Retrieve driver name from the keyname */
    driver_name = strrchrW( keyname->Buffer, '\\' );
    driver_name++;

    module = load_driver( driver_name, keyname );
    if (!module)
        return STATUS_DLL_INIT_FAILED;

    driver_object->DriverSection = find_ldr_module( module );

    nt = RtlImageNtHeader( module );
    if (!nt->OptionalHeader.AddressOfEntryPoint) return STATUS_SUCCESS;
    driver_object->DriverInit = (PDRIVER_INITIALIZE)((char *)module + nt->OptionalHeader.AddressOfEntryPoint);

    if (WINE_TRACE_ON(relay))
        WINE_DPRINTF( "%04x:Call driver init %p (obj=%p,str=%s)\n", GetCurrentThreadId(),
                      driver_object->DriverInit, driver_object, wine_dbgstr_w(keyname->Buffer) );

    status = driver_object->DriverInit( driver_object, keyname );

    if (WINE_TRACE_ON(relay))
        WINE_DPRINTF( "%04x:Ret  driver init %p (obj=%p,str=%s) retval=%08x\n", GetCurrentThreadId(),
                      driver_object->DriverInit, driver_object, wine_dbgstr_w(keyname->Buffer), status );

    WINE_TRACE( "init done for %s obj %p\n", wine_dbgstr_w(driver_name), driver_object );
    WINE_TRACE( "- DriverInit = %p\n", driver_object->DriverInit );
    WINE_TRACE( "- DriverStartIo = %p\n", driver_object->DriverStartIo );
    WINE_TRACE( "- DriverUnload = %p\n", driver_object->DriverUnload );
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
        WINE_TRACE( "- MajorFunction[%d] = %p\n", i, driver_object->MajorFunction[i] );

    return status;
}

/* helper function to update service status */
static void set_service_status( SERVICE_STATUS_HANDLE handle, DWORD state, DWORD accepted )
{
    SERVICE_STATUS status;
    status.dwServiceType             = SERVICE_WIN32;
    status.dwCurrentState            = state;
    status.dwControlsAccepted        = accepted;
    status.dwWin32ExitCode           = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 0;
    status.dwWaitHint                = (state == SERVICE_START_PENDING) ? 10000 : 0;
    SetServiceStatus( handle, &status );
}

/* call the driver unload function */
static void unload_driver( struct wine_rb_entry *entry, void *context )
{
    struct wine_driver *driver = WINE_RB_ENTRY_VALUE( entry, struct wine_driver, entry );
    DRIVER_OBJECT *driver_obj = driver->driver_obj;
    LDR_MODULE *ldr = driver_obj->DriverSection;

    TRACE( "stopping driver %s\n", wine_dbgstr_w(driver->name) );
    set_service_status( driver->handle, SERVICE_STOP_PENDING, 0 );

    if (driver_obj->DriverUnload)
    {
        if (WINE_TRACE_ON(relay))
            WINE_DPRINTF( "%04x:Call driver unload %p (obj=%p)\n", GetCurrentThreadId(),
                          driver_obj->DriverUnload, driver_obj );

        driver_obj->DriverUnload( driver_obj );

        if (WINE_TRACE_ON(relay))
            WINE_DPRINTF( "%04x:Ret  driver unload %p (obj=%p)\n", GetCurrentThreadId(),
                          driver_obj->DriverUnload, driver_obj );
    }

    FreeLibrary( ldr->BaseAddress );
    IoDeleteDriver( driver_obj );
    ObDereferenceObject( driver_obj );

    set_service_status( driver->handle, SERVICE_STOPPED, 0 );
    CloseServiceHandle( (void *)driver->handle );
    HeapFree( GetProcessHeap(), 0, driver );
}

/* load a driver and notify services.exe about the status change */
static NTSTATUS create_driver( const WCHAR *driver_name )
{
    static const WCHAR driverW[] = {'\\','D','r','i','v','e','r','\\',0};
    struct wine_driver *driver;
    UNICODE_STRING drv_name;
    NTSTATUS status;
    DWORD length;
    WCHAR *str;

    length = FIELD_OFFSET( struct wine_driver, name[strlenW(driver_name) + 1] );
    if (!(driver = HeapAlloc( GetProcessHeap(), 0, length )))
        return STATUS_NO_MEMORY;

    strcpyW( driver->name, driver_name );
    driver->driver_obj = NULL;

    if (!(driver->handle = (void *)OpenServiceW( manager_handle, driver_name, SERVICE_SET_STATUS )))
    {
        HeapFree( GetProcessHeap(), 0, driver );
        return STATUS_UNSUCCESSFUL;
    }

    if (wine_rb_put( &wine_drivers, driver_name, &driver->entry ))
    {
        CloseServiceHandle( (void *)driver->handle );
        HeapFree( GetProcessHeap(), 0, driver );
        return STATUS_UNSUCCESSFUL;
    }

    if (!(str = HeapAlloc( GetProcessHeap(), 0, sizeof(driverW) + strlenW(driver_name)*sizeof(WCHAR) )))
    {
        status = STATUS_NO_MEMORY;
        goto error;
    }

    TRACE( "starting driver %s\n", wine_dbgstr_w(driver_name) );
    set_service_status( driver->handle, SERVICE_START_PENDING, 0 );

    lstrcpyW( str, driverW);
    lstrcatW( str, driver_name );
    RtlInitUnicodeString( &drv_name, str );

    status = IoCreateDriver( &drv_name, init_driver );
    if (status == STATUS_SUCCESS)
    {
        status = ObReferenceObjectByName( &drv_name, OBJ_CASE_INSENSITIVE, NULL,
                                          0, NULL, KernelMode, NULL, (void **)&driver->driver_obj );
        if (status != STATUS_SUCCESS)
        {
            ERR("Failed to locate loaded driver (%s)\n", wine_dbgstr_w(driver_name));
        }
    }

    RtlFreeUnicodeString( &drv_name );

    if (status == STATUS_SUCCESS)
    {
        set_service_status( driver->handle, SERVICE_RUNNING,
                            SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN );
        return STATUS_SUCCESS;
    }

error:
    set_service_status( driver->handle, SERVICE_STOPPED, 0 );
    CloseServiceHandle( (void *)driver->handle );
    wine_rb_remove( &wine_drivers, driver_name );
    HeapFree( GetProcessHeap(), 0, driver );
    return status;
}

static DWORD WINAPI service_handler( DWORD ctrl, DWORD event_type, LPVOID event_data, LPVOID context )
{
    const WCHAR *driver_name = context;

    switch (ctrl)
    {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        WINE_TRACE( "shutting down %s\n", wine_dbgstr_w(driver_name) );
        set_service_status( service_handle, SERVICE_STOP_PENDING, 0 );
        SetEvent( stop_event );
        return NO_ERROR;
    default:
        WINE_FIXME( "got service ctrl %x for %s\n", ctrl, wine_dbgstr_w(driver_name) );
        set_service_status( service_handle, SERVICE_RUNNING,
                            SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN );
        return NO_ERROR;
    }
}

static void WINAPI ServiceMain( DWORD argc, LPWSTR *argv )
{
    const WCHAR *driver_name = argv[0];

    if (wine_rb_init( &wine_drivers, &wine_drivers_rb_functions ))
        return;
    if (!(stop_event = CreateEventW( NULL, TRUE, FALSE, NULL )))
        return;
    if (!(manager_handle = OpenSCManagerW( NULL, NULL, SC_MANAGER_CONNECT )))
        return;
    if (!(service_handle = RegisterServiceCtrlHandlerExW( driver_name, service_handler, (void *)driver_name )))
        return;

    if (create_driver( driver_name ) == STATUS_SUCCESS)
        wine_ntoskrnl_main_loop( stop_event );

    wine_rb_destroy( &wine_drivers, unload_driver, NULL );
    set_service_status( service_handle, SERVICE_STOPPED, 0 );
    CloseServiceHandle( manager_handle );
    CloseHandle( stop_event );
}

int wmain( int argc, WCHAR *argv[] )
{
    SERVICE_TABLE_ENTRYW service_table[2];

    if (!argv[1])
    {
        WINE_ERR( "missing device name, winedevice isn't supposed to be run manually\n" );
        return 1;
    }

    service_table[0].lpServiceName = argv[1];
    service_table[0].lpServiceProc = ServiceMain;
    service_table[1].lpServiceName = NULL;
    service_table[1].lpServiceProc = NULL;

    StartServiceCtrlDispatcherW( service_table );
    return 0;
}
