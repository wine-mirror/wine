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
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winedevice);
WINE_DECLARE_DEBUG_CHANNEL(relay);

extern NTSTATUS wine_ntoskrnl_main_loop( HANDLE stop_event );

static WCHAR *driver_name;
static SERVICE_STATUS_HANDLE service_handle;
static HKEY driver_hkey;
static HANDLE stop_event;
static DRIVER_OBJECT driver_obj;
static DRIVER_EXTENSION driver_extension;

/* find the LDR_MODULE corresponding to the driver module */
static LDR_MODULE *find_ldr_module( HMODULE module )
{
    LIST_ENTRY *entry, *list = &NtCurrentTeb()->Peb->LdrData->InMemoryOrderModuleList;

    for (entry = list->Flink; entry != list; entry = entry->Flink)
    {
        LDR_MODULE *ldr = CONTAINING_RECORD(entry, LDR_MODULE, InMemoryOrderModuleList);
        if (ldr->BaseAddress == module) return ldr;
        if (ldr->BaseAddress > (void *)module) break;
    }
    return NULL;
}

/* call the driver init entry point */
static NTSTATUS init_driver( HMODULE module, UNICODE_STRING *keyname )
{
    unsigned int i;
    NTSTATUS status;
    const IMAGE_NT_HEADERS *nt = RtlImageNtHeader( module );

    if (!nt->OptionalHeader.AddressOfEntryPoint) return STATUS_SUCCESS;

    driver_obj.Size            = sizeof(driver_obj);
    driver_obj.DriverSection   = find_ldr_module( module );
    driver_obj.DriverInit      = (PDRIVER_INITIALIZE)((char *)module + nt->OptionalHeader.AddressOfEntryPoint);
    driver_obj.DriverExtension = &driver_extension;

    driver_extension.DriverObject   = &driver_obj;
    driver_extension.ServiceKeyName = *keyname;

    if (WINE_TRACE_ON(relay))
        WINE_DPRINTF( "%04x:Call driver init %p (obj=%p,str=%s)\n", GetCurrentThreadId(),
                      driver_obj.DriverInit, &driver_obj, wine_dbgstr_w(keyname->Buffer) );

    status = driver_obj.DriverInit( &driver_obj, keyname );

    if (WINE_TRACE_ON(relay))
        WINE_DPRINTF( "%04x:Ret  driver init %p (obj=%p,str=%s) retval=%08x\n", GetCurrentThreadId(),
                      driver_obj.DriverInit, &driver_obj, wine_dbgstr_w(keyname->Buffer), status );

    WINE_TRACE( "init done for %s obj %p\n", wine_dbgstr_w(driver_name), &driver_obj );
    WINE_TRACE( "- DriverInit = %p\n", driver_obj.DriverInit );
    WINE_TRACE( "- DriverStartIo = %p\n", driver_obj.DriverStartIo );
    WINE_TRACE( "- DriverUnload = %p\n", driver_obj.DriverUnload );
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
        WINE_TRACE( "- MajorFunction[%d] = %p\n", i, driver_obj.MajorFunction[i] );

    return status;
}

/* load the .sys module for a device driver */
static BOOL load_driver(void)
{
    static const WCHAR ntprefixW[] = {'\\','?','?','\\',0};
    static const WCHAR ImagePathW[] = {'I','m','a','g','e','P','a','t','h',0};
    static const WCHAR servicesW[] = {'\\','R','e','g','i','s','t','r','y',
                                      '\\','M','a','c','h','i','n','e',
                                      '\\','S','y','s','t','e','m',
                                      '\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t',
                                      '\\','S','e','r','v','i','c','e','s','\\',0};

    UNICODE_STRING keypath;
    HMODULE module;
    LPWSTR path = NULL, str;
    DWORD type, size;

    str = HeapAlloc( GetProcessHeap(), 0, sizeof(servicesW) + strlenW(driver_name)*sizeof(WCHAR) );
    lstrcpyW( str, servicesW );
    lstrcatW( str, driver_name );

    if (RegOpenKeyW( HKEY_LOCAL_MACHINE, str + 18 /* skip \registry\machine */, &driver_hkey ))
    {
        WINE_ERR( "cannot open key %s, err=%u\n", wine_dbgstr_w(str), GetLastError() );
        return FALSE;
    }
    RtlInitUnicodeString( &keypath, str );

    /* read the executable path from memory */
    size = 0;
    if (RegQueryValueExW( driver_hkey, ImagePathW, NULL, &type, NULL, &size )) return FALSE;

    str = HeapAlloc( GetProcessHeap(), 0, size );
    if (!RegQueryValueExW( driver_hkey, ImagePathW, NULL, &type, (LPBYTE)str, &size ))
    {
        size = ExpandEnvironmentStringsW(str,NULL,0);
        path = HeapAlloc(GetProcessHeap(),0,size*sizeof(WCHAR));
        ExpandEnvironmentStringsW(str,path,size);
    }
    HeapFree( GetProcessHeap(), 0, str );
    if (!path) return FALSE;

    /* make sure msvcrt is loaded to resolve the ntoskrnl.exe forwards */
    LoadLibraryA( "msvcrt.dll" );

    /* GameGuard uses an NT-style path name */
    str = path;
    if (!strncmpW( path, ntprefixW, 4 )) str += 4;

    WINE_TRACE( "loading driver %s\n", wine_dbgstr_w(str) );

    module = LoadLibraryW( str );
    HeapFree( GetProcessHeap(), 0, path );
    if (!module) return FALSE;

    init_driver( module, &keypath );
    return TRUE;
}

static DWORD WINAPI service_handler( DWORD ctrl, DWORD event_type, LPVOID event_data, LPVOID context )
{
    SERVICE_STATUS status;

    status.dwServiceType             = SERVICE_WIN32;
    status.dwControlsAccepted        = SERVICE_ACCEPT_STOP;
    status.dwWin32ExitCode           = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 0;
    status.dwWaitHint                = 0;

    switch(ctrl)
    {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        WINE_TRACE( "shutting down %s\n", wine_dbgstr_w(driver_name) );
        status.dwCurrentState     = SERVICE_STOP_PENDING;
        status.dwControlsAccepted = 0;
        SetServiceStatus( service_handle, &status );
        SetEvent( stop_event );
        return NO_ERROR;
    default:
        WINE_FIXME( "got service ctrl %x for %s\n", ctrl, wine_dbgstr_w(driver_name) );
        status.dwCurrentState = SERVICE_RUNNING;
        SetServiceStatus( service_handle, &status );
        return NO_ERROR;
    }
}

static void WINAPI ServiceMain( DWORD argc, LPWSTR *argv )
{
    SERVICE_STATUS status;

    WINE_TRACE( "starting service %s\n", wine_dbgstr_w(driver_name) );

    stop_event = CreateEventW( NULL, TRUE, FALSE, NULL );

    service_handle = RegisterServiceCtrlHandlerExW( driver_name, service_handler, NULL );

    status.dwServiceType             = SERVICE_WIN32;
    status.dwCurrentState            = SERVICE_START_PENDING;
    status.dwControlsAccepted        = 0;
    status.dwWin32ExitCode           = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 0;
    status.dwWaitHint                = 10000;
    SetServiceStatus( service_handle, &status );

    if (load_driver())
    {
        status.dwCurrentState     = SERVICE_RUNNING;
        status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
        SetServiceStatus( service_handle, &status );

        wine_ntoskrnl_main_loop( stop_event );
    }
    else WINE_ERR( "driver %s failed to load\n", wine_dbgstr_w(driver_name) );

    status.dwCurrentState     = SERVICE_STOPPED;
    status.dwControlsAccepted = 0;
    SetServiceStatus( service_handle, &status );
    WINE_TRACE( "service %s stopped\n", wine_dbgstr_w(driver_name) );
}

int wmain( int argc, WCHAR *argv[] )
{
    SERVICE_TABLE_ENTRYW service_table[2];

    if (!(driver_name = argv[1]))
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
