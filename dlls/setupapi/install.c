/*
 * Setupapi install routines
 *
 * Copyright 2002 Alexandre Julliard for CodeWeavers
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "winerror.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winsvc.h"
#include "shlobj.h"
#include "objidl.h"
#include "objbase.h"
#include "setupapi.h"
#include "setupapi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

/* info passed to callback functions dealing with files */
struct files_callback_info
{
    HSPFILEQ queue;
    PCWSTR   src_root;
    UINT     copy_flags;
    HINF     layout;
};

/* info passed to callback functions dealing with the registry */
struct registry_callback_info
{
    HKEY default_root;
    BOOL delete;
};

/* info passed to callback functions dealing with registering dlls */
struct register_dll_info
{
    PSP_FILE_CALLBACK_W callback;
    PVOID               callback_context;
    BOOL                unregister;
    int                 modules_size;
    int                 modules_count;
    HMODULE            *modules;
};

typedef BOOL (*iterate_fields_func)( HINF hinf, PCWSTR field, void *arg );

/* Unicode constants */
static const WCHAR CopyFiles[]  = {'C','o','p','y','F','i','l','e','s',0};
static const WCHAR DelFiles[]   = {'D','e','l','F','i','l','e','s',0};
static const WCHAR RenFiles[]   = {'R','e','n','F','i','l','e','s',0};
static const WCHAR Ini2Reg[]    = {'I','n','i','2','R','e','g',0};
static const WCHAR LogConf[]    = {'L','o','g','C','o','n','f',0};
static const WCHAR AddReg[]     = {'A','d','d','R','e','g',0};
static const WCHAR DelReg[]     = {'D','e','l','R','e','g',0};
static const WCHAR BitReg[]     = {'B','i','t','R','e','g',0};
static const WCHAR UpdateInis[] = {'U','p','d','a','t','e','I','n','i','s',0};
static const WCHAR CopyINF[]    = {'C','o','p','y','I','N','F',0};
static const WCHAR AddService[] = {'A','d','d','S','e','r','v','i','c','e',0};
static const WCHAR DelService[] = {'D','e','l','S','e','r','v','i','c','e',0};
static const WCHAR UpdateIniFields[] = {'U','p','d','a','t','e','I','n','i','F','i','e','l','d','s',0};
static const WCHAR RegisterDlls[]    = {'R','e','g','i','s','t','e','r','D','l','l','s',0};
static const WCHAR UnregisterDlls[]  = {'U','n','r','e','g','i','s','t','e','r','D','l','l','s',0};
static const WCHAR ProfileItems[]    = {'P','r','o','f','i','l','e','I','t','e','m','s',0};
static const WCHAR Name[]            = {'N','a','m','e',0};
static const WCHAR CmdLine[]         = {'C','m','d','L','i','n','e',0};
static const WCHAR SubDir[]          = {'S','u','b','D','i','r',0};
static const WCHAR WineFakeDlls[]    = {'W','i','n','e','F','a','k','e','D','l','l','s',0};
static const WCHAR WinePreInstall[]  = {'W','i','n','e','P','r','e','I','n','s','t','a','l','l',0};
static const WCHAR DisplayName[]     = {'D','i','s','p','l','a','y','N','a','m','e',0};
static const WCHAR Description[]     = {'D','e','s','c','r','i','p','t','i','o','n',0};
static const WCHAR ServiceBinary[]   = {'S','e','r','v','i','c','e','B','i','n','a','r','y',0};
static const WCHAR StartName[]       = {'S','t','a','r','t','N','a','m','e',0};
static const WCHAR LoadOrderGroup[]  = {'L','o','a','d','O','r','d','e','r','G','r','o','u','p',0};
static const WCHAR ServiceType[]     = {'S','e','r','v','i','c','e','T','y','p','e',0};
static const WCHAR StartType[]       = {'S','t','a','r','t','T','y','p','e',0};
static const WCHAR ErrorControl[]    = {'E','r','r','o','r','C','o','n','t','r','o','l',0};

static const WCHAR ServicesKey[] = {'S','y','s','t','e','m','\\',
                        'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                        'S','e','r','v','i','c','e','s',0};

/***********************************************************************
 *            get_field_string
 *
 * Retrieve the contents of a field, dynamically growing the buffer if necessary.
 */
static WCHAR *get_field_string( INFCONTEXT *context, DWORD index, WCHAR *buffer,
                                WCHAR *static_buffer, DWORD *size )
{
    DWORD required;

    if (SetupGetStringFieldW( context, index, buffer, *size, &required )) return buffer;
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        /* now grow the buffer */
        if (buffer != static_buffer) HeapFree( GetProcessHeap(), 0, buffer );
        if (!(buffer = HeapAlloc( GetProcessHeap(), 0, required*sizeof(WCHAR) ))) return NULL;
        *size = required;
        if (SetupGetStringFieldW( context, index, buffer, *size, &required )) return buffer;
    }
    if (buffer != static_buffer) HeapFree( GetProcessHeap(), 0, buffer );
    return NULL;
}


/***********************************************************************
 *            dup_section_line_field
 *
 * Retrieve the contents of a field in a newly-allocated buffer.
 */
static WCHAR *dup_section_line_field( HINF hinf, const WCHAR *section, const WCHAR *line, DWORD index )
{
    INFCONTEXT context;
    DWORD size;
    WCHAR *buffer;

    if (!SetupFindFirstLineW( hinf, section, line, &context )) return NULL;
    if (!SetupGetStringFieldW( &context, index, NULL, 0, &size )) return NULL;
    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) ))) return NULL;
    if (!SetupGetStringFieldW( &context, index, buffer, size, NULL )) buffer[0] = 0;
    return buffer;
}

/***********************************************************************
 *            copy_files_callback
 *
 * Called once for each CopyFiles entry in a given section.
 */
static BOOL copy_files_callback( HINF hinf, PCWSTR field, void *arg )
{
    struct files_callback_info *info = arg;
    WCHAR src_root[MAX_PATH], *p;

    if (!info->src_root)
    {
        lstrcpyW( src_root, PARSER_get_inf_filename( hinf ) );
        if ((p = wcsrchr( src_root, '\\' ))) *p = 0;
    }

    if (field[0] == '@')  /* special case: copy single file */
        SetupQueueDefaultCopyW( info->queue, info->layout ? info->layout : hinf,
                info->src_root ? info->src_root : src_root, field+1, field+1, info->copy_flags );
    else
        SetupQueueCopySectionW( info->queue, info->src_root ? info->src_root : src_root,
                info->layout ? info->layout : hinf, hinf, field, info->copy_flags );
    return TRUE;
}


/***********************************************************************
 *            delete_files_callback
 *
 * Called once for each DelFiles entry in a given section.
 */
static BOOL delete_files_callback( HINF hinf, PCWSTR field, void *arg )
{
    struct files_callback_info *info = arg;
    SetupQueueDeleteSectionW( info->queue, hinf, 0, field );
    return TRUE;
}


/***********************************************************************
 *            rename_files_callback
 *
 * Called once for each RenFiles entry in a given section.
 */
static BOOL rename_files_callback( HINF hinf, PCWSTR field, void *arg )
{
    struct files_callback_info *info = arg;
    SetupQueueRenameSectionW( info->queue, hinf, 0, field );
    return TRUE;
}


/***********************************************************************
 *            get_root_key
 *
 * Retrieve the registry root key from its name.
 */
static HKEY get_root_key( const WCHAR *name, HKEY def_root )
{
    static const WCHAR HKCR[] = {'H','K','C','R',0};
    static const WCHAR HKCU[] = {'H','K','C','U',0};
    static const WCHAR HKLM[] = {'H','K','L','M',0};
    static const WCHAR HKU[]  = {'H','K','U',0};
    static const WCHAR HKR[]  = {'H','K','R',0};

    if (!wcsicmp( name, HKCR )) return HKEY_CLASSES_ROOT;
    if (!wcsicmp( name, HKCU )) return HKEY_CURRENT_USER;
    if (!wcsicmp( name, HKLM )) return HKEY_LOCAL_MACHINE;
    if (!wcsicmp( name, HKU )) return HKEY_USERS;
    if (!wcsicmp( name, HKR )) return def_root;
    return 0;
}


/***********************************************************************
 *            append_multi_sz_value
 *
 * Append a multisz string to a multisz registry value.
 */
static void append_multi_sz_value( HKEY hkey, const WCHAR *value, const WCHAR *strings,
                                   DWORD str_size )
{
    DWORD size, type, total;
    WCHAR *buffer, *p;

    if (RegQueryValueExW( hkey, value, NULL, &type, NULL, &size )) return;
    if (type != REG_MULTI_SZ) return;

    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, (size + str_size) * sizeof(WCHAR) ))) return;
    if (RegQueryValueExW( hkey, value, NULL, NULL, (BYTE *)buffer, &size )) goto done;

    /* compare each string against all the existing ones */
    total = size;
    while (*strings)
    {
        int len = lstrlenW(strings) + 1;

        for (p = buffer; *p; p += lstrlenW(p) + 1)
            if (!wcsicmp( p, strings )) break;

        if (!*p)  /* not found, need to append it */
        {
            memcpy( p, strings, len * sizeof(WCHAR) );
            p[len] = 0;
            total += len * sizeof(WCHAR);
        }
        strings += len;
    }
    if (total != size)
    {
        TRACE( "setting value %s to %s\n", debugstr_w(value), debugstr_w(buffer) );
        RegSetValueExW( hkey, value, 0, REG_MULTI_SZ, (BYTE *)buffer, total );
    }
 done:
    HeapFree( GetProcessHeap(), 0, buffer );
}


/***********************************************************************
 *            delete_multi_sz_value
 *
 * Remove a string from a multisz registry value.
 */
static void delete_multi_sz_value( HKEY hkey, const WCHAR *value, const WCHAR *string )
{
    DWORD size, type;
    WCHAR *buffer, *src, *dst;

    if (RegQueryValueExW( hkey, value, NULL, &type, NULL, &size )) return;
    if (type != REG_MULTI_SZ) return;
    /* allocate double the size, one for value before and one for after */
    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, size * 2 * sizeof(WCHAR) ))) return;
    if (RegQueryValueExW( hkey, value, NULL, NULL, (BYTE *)buffer, &size )) goto done;
    src = buffer;
    dst = buffer + size;
    while (*src)
    {
        int len = lstrlenW(src) + 1;
        if (wcsicmp( src, string ))
        {
            memcpy( dst, src, len * sizeof(WCHAR) );
            dst += len;
        }
        src += len;
    }
    *dst++ = 0;
    if (dst != buffer + 2*size)  /* did we remove something? */
    {
        TRACE( "setting value %s to %s\n", debugstr_w(value), debugstr_w(buffer + size) );
        RegSetValueExW( hkey, value, 0, REG_MULTI_SZ,
                        (BYTE *)(buffer + size), dst - (buffer + size) );
    }
 done:
    HeapFree( GetProcessHeap(), 0, buffer );
}


/***********************************************************************
 *            do_reg_operation
 *
 * Perform an add/delete registry operation depending on the flags.
 */
static BOOL do_reg_operation( HKEY hkey, const WCHAR *value, INFCONTEXT *context, INT flags )
{
    DWORD type, size;

    if (flags & (FLG_ADDREG_DELREG_BIT | FLG_ADDREG_DELVAL))  /* deletion */
    {
        if (*value && !(flags & FLG_DELREG_KEYONLY_COMMON))
        {
            if ((flags & FLG_DELREG_MULTI_SZ_DELSTRING) == FLG_DELREG_MULTI_SZ_DELSTRING)
            {
                WCHAR *str;

                if (!SetupGetStringFieldW( context, 5, NULL, 0, &size ) || !size) return TRUE;
                if (!(str = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) ))) return FALSE;
                SetupGetStringFieldW( context, 5, str, size, NULL );
                delete_multi_sz_value( hkey, value, str );
                HeapFree( GetProcessHeap(), 0, str );
            }
            else RegDeleteValueW( hkey, value );
        }
        else
        {
            RegDeleteTreeW( hkey, NULL );
            NtDeleteKey( hkey );
        }
        return TRUE;
    }

    if (flags & (FLG_ADDREG_KEYONLY|FLG_ADDREG_KEYONLY_COMMON)) return TRUE;

    if (flags & (FLG_ADDREG_NOCLOBBER|FLG_ADDREG_OVERWRITEONLY))
    {
        BOOL exists = !RegQueryValueExW( hkey, value, NULL, NULL, NULL, NULL );
        if (exists && (flags & FLG_ADDREG_NOCLOBBER)) return TRUE;
        if (!exists && (flags & FLG_ADDREG_OVERWRITEONLY)) return TRUE;
    }

    switch(flags & FLG_ADDREG_TYPE_MASK)
    {
    case FLG_ADDREG_TYPE_SZ:        type = REG_SZ; break;
    case FLG_ADDREG_TYPE_MULTI_SZ:  type = REG_MULTI_SZ; break;
    case FLG_ADDREG_TYPE_EXPAND_SZ: type = REG_EXPAND_SZ; break;
    case FLG_ADDREG_TYPE_BINARY:    type = REG_BINARY; break;
    case FLG_ADDREG_TYPE_DWORD:     type = REG_DWORD; break;
    case FLG_ADDREG_TYPE_NONE:      type = REG_NONE; break;
    default:                        type = flags >> 16; break;
    }

    if (!(flags & FLG_ADDREG_BINVALUETYPE) ||
        (type == REG_DWORD && SetupGetFieldCount(context) == 5))
    {
        static const WCHAR empty;
        WCHAR *str = NULL;

        if (type == REG_MULTI_SZ)
        {
            if (!SetupGetMultiSzFieldW( context, 5, NULL, 0, &size )) size = 0;
            if (size)
            {
                if (!(str = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) ))) return FALSE;
                SetupGetMultiSzFieldW( context, 5, str, size, NULL );
            }
            if (flags & FLG_ADDREG_APPEND)
            {
                if (!str) return TRUE;
                append_multi_sz_value( hkey, value, str, size );
                HeapFree( GetProcessHeap(), 0, str );
                return TRUE;
            }
            /* else fall through to normal string handling */
        }
        else
        {
            if (!SetupGetStringFieldW( context, 5, NULL, 0, &size )) size = 0;
            if (size)
            {
                if (!(str = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) ))) return FALSE;
                SetupGetStringFieldW( context, 5, str, size, NULL );
                if (type == REG_LINK) size--;  /* no terminating null for symlinks */
            }
        }

        if (type == REG_DWORD)
        {
            DWORD dw = str ? wcstoul( str, NULL, 0 ) : 0;
            TRACE( "setting dword %s to %x\n", debugstr_w(value), dw );
            RegSetValueExW( hkey, value, 0, type, (BYTE *)&dw, sizeof(dw) );
        }
        else
        {
            TRACE( "setting value %s to %s\n", debugstr_w(value), debugstr_w(str) );
            if (str) RegSetValueExW( hkey, value, 0, type, (BYTE *)str, size * sizeof(WCHAR) );
            else RegSetValueExW( hkey, value, 0, type, (const BYTE *)&empty, sizeof(WCHAR) );
        }
        HeapFree( GetProcessHeap(), 0, str );
        return TRUE;
    }
    else  /* get the binary data */
    {
        BYTE *data = NULL;

        if (!SetupGetBinaryField( context, 5, NULL, 0, &size )) size = 0;
        if (size)
        {
            if (!(data = HeapAlloc( GetProcessHeap(), 0, size ))) return FALSE;
            TRACE( "setting binary data %s len %d\n", debugstr_w(value), size );
            SetupGetBinaryField( context, 5, data, size, NULL );
        }
        RegSetValueExW( hkey, value, 0, type, data, size );
        HeapFree( GetProcessHeap(), 0, data );
        return TRUE;
    }
}


/***********************************************************************
 *            registry_callback
 *
 * Called once for each AddReg and DelReg entry in a given section.
 */
static BOOL registry_callback( HINF hinf, PCWSTR field, void *arg )
{
    struct registry_callback_info *info = arg;
    INFCONTEXT context;
    HKEY root_key, hkey;

    BOOL ok = SetupFindFirstLineW( hinf, field, NULL, &context );

    for (; ok; ok = SetupFindNextLine( &context, &context ))
    {
        DWORD options = 0;
        WCHAR buffer[MAX_INF_STRING_LENGTH];
        INT flags;

        /* get root */
        if (!SetupGetStringFieldW( &context, 1, buffer, ARRAY_SIZE( buffer ), NULL ))
            continue;
        if (!(root_key = get_root_key( buffer, info->default_root )))
            continue;

        /* get key */
        if (!SetupGetStringFieldW( &context, 2, buffer, ARRAY_SIZE( buffer ), NULL ))
            *buffer = 0;

        /* get flags */
        if (!SetupGetIntField( &context, 4, &flags )) flags = 0;

        if (!info->delete)
        {
            if (flags & FLG_ADDREG_DELREG_BIT) continue;  /* ignore this entry */
        }
        else
        {
            if (!flags) flags = FLG_ADDREG_DELREG_BIT;
            else if (!(flags & FLG_ADDREG_DELREG_BIT)) continue;  /* ignore this entry */
        }
        /* Wine extension: magic support for symlinks */
        if (flags >> 16 == REG_LINK) options = REG_OPTION_OPEN_LINK | REG_OPTION_CREATE_LINK;

        if (info->delete || (flags & FLG_ADDREG_OVERWRITEONLY))
        {
            if (RegOpenKeyExW( root_key, buffer, options, MAXIMUM_ALLOWED, &hkey ))
                continue;  /* ignore if it doesn't exist */
        }
        else
        {
            DWORD res = RegCreateKeyExW( root_key, buffer, 0, NULL, options,
                                         MAXIMUM_ALLOWED, NULL, &hkey, NULL );
            if (res == ERROR_ALREADY_EXISTS && (options & REG_OPTION_CREATE_LINK))
                res = RegCreateKeyExW( root_key, buffer, 0, NULL, REG_OPTION_OPEN_LINK,
                                       MAXIMUM_ALLOWED, NULL, &hkey, NULL );
            if (res)
            {
                ERR( "could not create key %p %s\n", root_key, debugstr_w(buffer) );
                continue;
            }
        }
        TRACE( "key %p %s\n", root_key, debugstr_w(buffer) );

        /* get value name */
        if (!SetupGetStringFieldW( &context, 3, buffer, ARRAY_SIZE( buffer ), NULL ))
            *buffer = 0;

        /* and now do it */
        if (!do_reg_operation( hkey, buffer, &context, flags ))
        {
            RegCloseKey( hkey );
            return FALSE;
        }
        RegCloseKey( hkey );
    }
    return TRUE;
}


/***********************************************************************
 *            do_register_dll
 *
 * Register or unregister a dll.
 */
static BOOL do_register_dll( struct register_dll_info *info, const WCHAR *path,
                             INT flags, INT timeout, const WCHAR *args )
{
    HMODULE module;
    HRESULT res;
    SP_REGISTER_CONTROL_STATUSW status;
    IMAGE_NT_HEADERS *nt;

    status.cbSize = sizeof(status);
    status.FileName = path;
    status.FailureCode = SPREG_SUCCESS;
    status.Win32Error = ERROR_SUCCESS;

    if (info->callback)
    {
        switch(info->callback( info->callback_context, SPFILENOTIFY_STARTREGISTRATION,
                               (UINT_PTR)&status, !info->unregister ))
        {
        case FILEOP_ABORT:
            SetLastError( ERROR_OPERATION_ABORTED );
            return FALSE;
        case FILEOP_SKIP:
            return TRUE;
        case FILEOP_DOIT:
            break;
        }
    }

    if (!(module = LoadLibraryExW( path, 0, LOAD_WITH_ALTERED_SEARCH_PATH )))
    {
        WARN( "could not load %s\n", debugstr_w(path) );
        status.FailureCode = SPREG_LOADLIBRARY;
        status.Win32Error = GetLastError();
        goto done;
    }

    if ((nt = RtlImageNtHeader( module )) && !(nt->FileHeader.Characteristics & IMAGE_FILE_DLL))
    {
        /* file is an executable, not a dll */
        STARTUPINFOW startup;
        PROCESS_INFORMATION process_info;
        WCHAR *cmd_line;
        BOOL res;
        DWORD len;
        static const WCHAR format[] = {'"','%','s','"',' ','%','s',0};
        static const WCHAR default_args[] = {'/','R','e','g','S','e','r','v','e','r',0};

        FreeLibrary( module );
        module = NULL;
        if (!args) args = default_args;
        len = lstrlenW(path) + lstrlenW(args) + 4;
        cmd_line = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        swprintf( cmd_line, len, format, path, args );
        memset( &startup, 0, sizeof(startup) );
        startup.cb = sizeof(startup);
        TRACE( "executing %s\n", debugstr_w(cmd_line) );
        res = CreateProcessW( path, cmd_line, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &process_info );
        HeapFree( GetProcessHeap(), 0, cmd_line );
        if (!res)
        {
            status.FailureCode = SPREG_LOADLIBRARY;
            status.Win32Error = GetLastError();
            goto done;
        }
        CloseHandle( process_info.hThread );

        if (WaitForSingleObject( process_info.hProcess, timeout*1000 ) == WAIT_TIMEOUT)
        {
            /* timed out, kill the process */
            TerminateProcess( process_info.hProcess, 1 );
            status.FailureCode = SPREG_TIMEOUT;
            status.Win32Error = ERROR_TIMEOUT;
        }
        CloseHandle( process_info.hProcess );
        goto done;
    }

    if (flags & FLG_REGSVR_DLLREGISTER)
    {
        const char *entry_point = info->unregister ? "DllUnregisterServer" : "DllRegisterServer";
        HRESULT (WINAPI *func)(void) = (void *)GetProcAddress( module, entry_point );

        if (!func)
        {
            status.FailureCode = SPREG_GETPROCADDR;
            status.Win32Error = GetLastError();
            goto done;
        }

        TRACE( "calling %s in %s\n", entry_point, debugstr_w(path) );
        res = func();

        if (FAILED(res))
        {
            WARN( "calling %s in %s returned error %x\n", entry_point, debugstr_w(path), res );
            status.FailureCode = SPREG_REGSVR;
            status.Win32Error = res;
            goto done;
        }
    }

    if (flags & FLG_REGSVR_DLLINSTALL)
    {
        HRESULT (WINAPI *func)(BOOL,LPCWSTR) = (void *)GetProcAddress( module, "DllInstall" );

        if (!func)
        {
            status.FailureCode = SPREG_GETPROCADDR;
            status.Win32Error = GetLastError();
            goto done;
        }

        TRACE( "calling DllInstall(%d,%s) in %s\n",
               !info->unregister, debugstr_w(args), debugstr_w(path) );
        res = func( !info->unregister, args );

        if (FAILED(res))
        {
            WARN( "calling DllInstall in %s returned error %x\n", debugstr_w(path), res );
            status.FailureCode = SPREG_REGSVR;
            status.Win32Error = res;
            goto done;
        }
    }

done:
    if (module)
    {
        if (info->modules_count >= info->modules_size)
        {
            int new_size = max( 32, info->modules_size * 2 );
            HMODULE *new = info->modules ?
                HeapReAlloc( GetProcessHeap(), 0, info->modules, new_size * sizeof(*new) ) :
                HeapAlloc( GetProcessHeap(), 0, new_size * sizeof(*new) );
            if (new)
            {
                info->modules_size = new_size;
                info->modules = new;
            }
        }
        if (info->modules_count < info->modules_size) info->modules[info->modules_count++] = module;
        else FreeLibrary( module );
    }
    if (info->callback) info->callback( info->callback_context, SPFILENOTIFY_ENDREGISTRATION,
                                        (UINT_PTR)&status, !info->unregister );
    return TRUE;
}


/***********************************************************************
 *            register_dlls_callback
 *
 * Called once for each RegisterDlls entry in a given section.
 */
static BOOL register_dlls_callback( HINF hinf, PCWSTR field, void *arg )
{
    struct register_dll_info *info = arg;
    INFCONTEXT context;
    BOOL ret = TRUE;
    BOOL ok = SetupFindFirstLineW( hinf, field, NULL, &context );

    for (; ok; ok = SetupFindNextLine( &context, &context ))
    {
        WCHAR *path, *args, *p;
        WCHAR buffer[MAX_INF_STRING_LENGTH];
        INT flags, timeout;

        /* get directory */
        if (!(path = PARSER_get_dest_dir( &context ))) continue;

        /* get dll name */
        if (!SetupGetStringFieldW( &context, 3, buffer, ARRAY_SIZE( buffer ), NULL ))
            goto done;
        if (!(p = HeapReAlloc( GetProcessHeap(), 0, path,
                               (lstrlenW(path) + lstrlenW(buffer) + 2) * sizeof(WCHAR) ))) goto done;
        path = p;
        p += lstrlenW(p);
        if (p == path || p[-1] != '\\') *p++ = '\\';
        lstrcpyW( p, buffer );

        /* get flags */
        if (!SetupGetIntField( &context, 4, &flags )) flags = 0;

        /* get timeout */
        if (!SetupGetIntField( &context, 5, &timeout )) timeout = 60;

        /* get command line */
        args = NULL;
        if (SetupGetStringFieldW( &context, 6, buffer, ARRAY_SIZE( buffer ), NULL ))
            args = buffer;

        ret = do_register_dll( info, path, flags, timeout, args );

    done:
        HeapFree( GetProcessHeap(), 0, path );
        if (!ret) break;
    }
    return ret;
}

/***********************************************************************
 *            fake_dlls_callback
 *
 * Called once for each WineFakeDlls entry in a given section.
 */
static BOOL fake_dlls_callback( HINF hinf, PCWSTR field, void *arg )
{
    INFCONTEXT context;
    BOOL ok = SetupFindFirstLineW( hinf, field, NULL, &context );

    for (; ok; ok = SetupFindNextLine( &context, &context ))
    {
        WCHAR *path, *p;
        WCHAR buffer[MAX_INF_STRING_LENGTH];

        /* get directory */
        if (!(path = PARSER_get_dest_dir( &context ))) continue;

        /* get dll name */
        if (!SetupGetStringFieldW( &context, 3, buffer, ARRAY_SIZE( buffer ), NULL ))
            goto done;
        if (!(p = HeapReAlloc( GetProcessHeap(), 0, path,
                               (lstrlenW(path) + lstrlenW(buffer) + 2) * sizeof(WCHAR) ))) goto done;
        path = p;
        p += lstrlenW(p);
        if (p == path || p[-1] != '\\') *p++ = '\\';
        lstrcpyW( p, buffer );

        /* get source dll */
        if (SetupGetStringFieldW( &context, 4, buffer, ARRAY_SIZE( buffer ), NULL ))
            p = buffer;  /* otherwise use target base name as default source */

        create_fake_dll( path, p );  /* ignore errors */

    done:
        HeapFree( GetProcessHeap(), 0, path );
    }
    return TRUE;
}

/***********************************************************************
 *            update_ini_callback
 *
 * Called once for each UpdateInis entry in a given section.
 */
static BOOL update_ini_callback( HINF hinf, PCWSTR field, void *arg )
{
    INFCONTEXT context;

    BOOL ok = SetupFindFirstLineW( hinf, field, NULL, &context );

    for (; ok; ok = SetupFindNextLine( &context, &context ))
    {
        WCHAR buffer[MAX_INF_STRING_LENGTH];
        WCHAR  filename[MAX_INF_STRING_LENGTH];
        WCHAR  section[MAX_INF_STRING_LENGTH];
        WCHAR  entry[MAX_INF_STRING_LENGTH];
        WCHAR  string[MAX_INF_STRING_LENGTH];
        LPWSTR divider;

        if (!SetupGetStringFieldW( &context, 1, filename, ARRAY_SIZE( filename ), NULL ))
            continue;

        if (!SetupGetStringFieldW( &context, 2, section, ARRAY_SIZE( section ), NULL ))
            continue;

        if (!SetupGetStringFieldW( &context, 4, buffer, ARRAY_SIZE( buffer ), NULL ))
            continue;

        divider = wcschr(buffer,'=');
        if (divider)
        {
            *divider = 0;
            lstrcpyW(entry,buffer);
            divider++;
            lstrcpyW(string,divider);
        }
        else
        {
            lstrcpyW(entry,buffer);
            string[0]=0;
        }

        TRACE("Writing %s = %s in %s of file %s\n",debugstr_w(entry),
               debugstr_w(string),debugstr_w(section),debugstr_w(filename));
        WritePrivateProfileStringW(section,entry,string,filename);

    }
    return TRUE;
}

static BOOL update_ini_fields_callback( HINF hinf, PCWSTR field, void *arg )
{
    FIXME( "should update ini fields %s\n", debugstr_w(field) );
    return TRUE;
}

static BOOL ini2reg_callback( HINF hinf, PCWSTR field, void *arg )
{
    FIXME( "should do ini2reg %s\n", debugstr_w(field) );
    return TRUE;
}

static BOOL logconf_callback( HINF hinf, PCWSTR field, void *arg )
{
    FIXME( "should do logconf %s\n", debugstr_w(field) );
    return TRUE;
}

static BOOL bitreg_callback( HINF hinf, PCWSTR field, void *arg )
{
    FIXME( "should do bitreg %s\n", debugstr_w(field) );
    return TRUE;
}

static BOOL profile_items_callback( HINF hinf, PCWSTR field, void *arg )
{
    WCHAR lnkpath[MAX_PATH];
    LPWSTR cmdline=NULL, lnkpath_end;
    unsigned int name_size;
    INFCONTEXT name_context, context;
    int attrs=0;

    static const WCHAR dotlnk[] = {'.','l','n','k',0};

    TRACE( "(%s)\n", debugstr_w(field) );

    if (SetupFindFirstLineW( hinf, field, Name, &name_context ))
    {
        SetupGetIntField( &name_context, 2, &attrs );
        if (attrs & ~FLG_PROFITEM_GROUP) FIXME( "unhandled attributes: %x\n", attrs );
    }
    else return TRUE;

    /* calculate filename */
    SHGetFolderPathW( NULL, CSIDL_COMMON_PROGRAMS, NULL, SHGFP_TYPE_CURRENT, lnkpath );
    lnkpath_end = lnkpath + lstrlenW(lnkpath);
    if (lnkpath_end[-1] != '\\') *lnkpath_end++ = '\\';

    if (!(attrs & FLG_PROFITEM_GROUP) && SetupFindFirstLineW( hinf, field, SubDir, &context ))
    {
        unsigned int subdir_size;

        if (!SetupGetStringFieldW( &context, 1, lnkpath_end, (lnkpath+MAX_PATH)-lnkpath_end, &subdir_size ))
            return TRUE;

        lnkpath_end += subdir_size - 1;
        if (lnkpath_end[-1] != '\\') *lnkpath_end++ = '\\';
    }

    if (!SetupGetStringFieldW( &name_context, 1, lnkpath_end, (lnkpath+MAX_PATH)-lnkpath_end, &name_size ))
        return TRUE;

    lnkpath_end += name_size - 1;

    if (attrs & FLG_PROFITEM_GROUP)
    {
        SHPathPrepareForWriteW( NULL, NULL, lnkpath, SHPPFW_DIRCREATE );
    }
    else
    {
        IShellLinkW* shelllink=NULL;
        IPersistFile* persistfile=NULL;
        HRESULT initresult=E_FAIL;

        if (lnkpath+MAX_PATH < lnkpath_end + 5) return TRUE;
        lstrcpyW( lnkpath_end, dotlnk );

        TRACE( "link path: %s\n", debugstr_w(lnkpath) );

        /* calculate command line */
        if (SetupFindFirstLineW( hinf, field, CmdLine, &context ))
        {
            unsigned int dir_len=0, subdir_size=0, filename_size=0;
            int dirid=0;
            LPCWSTR dir;
            LPWSTR cmdline_end;

            SetupGetIntField( &context, 1, &dirid );
            dir = DIRID_get_string( dirid );

            if (dir) dir_len = lstrlenW(dir);

            SetupGetStringFieldW( &context, 2, NULL, 0, &subdir_size );
            SetupGetStringFieldW( &context, 3, NULL, 0, &filename_size );

            if (dir_len && filename_size)
            {
                cmdline = cmdline_end = HeapAlloc( GetProcessHeap(), 0, sizeof(WCHAR) * (dir_len+subdir_size+filename_size+1) );

                lstrcpyW( cmdline_end, dir );
                cmdline_end += dir_len;
                if (cmdline_end[-1] != '\\') *cmdline_end++ = '\\';

                if (subdir_size)
                {
                    SetupGetStringFieldW( &context, 2, cmdline_end, subdir_size, NULL );
                    cmdline_end += subdir_size-1;
                    if (cmdline_end[-1] != '\\') *cmdline_end++ = '\\';
                }
                SetupGetStringFieldW( &context, 3, cmdline_end, filename_size, NULL );
                TRACE( "cmdline: %s\n", debugstr_w(cmdline));
            }
        }

        if (!cmdline) return TRUE;

        initresult = CoInitialize(NULL);

        if (FAILED(CoCreateInstance( &CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                                     &IID_IShellLinkW, (LPVOID*)&shelllink )))
            goto done;

        IShellLinkW_SetPath( shelllink, cmdline );
        SHPathPrepareForWriteW( NULL, NULL, lnkpath, SHPPFW_DIRCREATE|SHPPFW_IGNOREFILENAME );
        if (SUCCEEDED(IShellLinkW_QueryInterface( shelllink, &IID_IPersistFile, (LPVOID*)&persistfile)))
        {
            TRACE( "writing link: %s\n", debugstr_w(lnkpath) );
            IPersistFile_Save( persistfile, lnkpath, FALSE );
            IPersistFile_Release( persistfile );
        }
        IShellLinkW_Release( shelllink );

    done:
        if (SUCCEEDED(initresult)) CoUninitialize();
        HeapFree( GetProcessHeap(), 0, cmdline );
    }

    return TRUE;
}

static BOOL copy_inf_callback( HINF hinf, PCWSTR field, void *arg )
{
    FIXME( "should do copy inf %s\n", debugstr_w(field) );
    return TRUE;
}


/***********************************************************************
 *            iterate_section_fields
 *
 * Iterate over all fields of a certain key of a certain section
 */
static BOOL iterate_section_fields( HINF hinf, PCWSTR section, PCWSTR key,
                                    iterate_fields_func callback, void *arg )
{
    WCHAR static_buffer[200];
    WCHAR *buffer = static_buffer;
    DWORD size = ARRAY_SIZE( static_buffer );
    INFCONTEXT context;
    BOOL ret = FALSE;

    BOOL ok = SetupFindFirstLineW( hinf, section, key, &context );
    while (ok)
    {
        UINT i, count = SetupGetFieldCount( &context );
        for (i = 1; i <= count; i++)
        {
            if (!(buffer = get_field_string( &context, i, buffer, static_buffer, &size )))
                goto done;
            if (!callback( hinf, buffer, arg ))
            {
                WARN("callback failed for %s %s err %d\n",
                     debugstr_w(section), debugstr_w(buffer), GetLastError() );
                goto done;
            }
        }
        ok = SetupFindNextMatchLineW( &context, key, &context );
    }
    ret = TRUE;
 done:
    if (buffer != static_buffer) HeapFree( GetProcessHeap(), 0, buffer );
    return ret;
}


/***********************************************************************
 *            SetupInstallFilesFromInfSectionA   (SETUPAPI.@)
 */
BOOL WINAPI SetupInstallFilesFromInfSectionA( HINF hinf, HINF hlayout, HSPFILEQ queue,
                                              PCSTR section, PCSTR src_root, UINT flags )
{
    UNICODE_STRING sectionW;
    BOOL ret = FALSE;

    if (!RtlCreateUnicodeStringFromAsciiz( &sectionW, section ))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }
    if (!src_root)
        ret = SetupInstallFilesFromInfSectionW( hinf, hlayout, queue, sectionW.Buffer,
                                                NULL, flags );
    else
    {
        UNICODE_STRING srcW;
        if (RtlCreateUnicodeStringFromAsciiz( &srcW, src_root ))
        {
            ret = SetupInstallFilesFromInfSectionW( hinf, hlayout, queue, sectionW.Buffer,
                                                    srcW.Buffer, flags );
            RtlFreeUnicodeString( &srcW );
        }
        else SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    }
    RtlFreeUnicodeString( &sectionW );
    return ret;
}


/***********************************************************************
 *            SetupInstallFilesFromInfSectionW   (SETUPAPI.@)
 */
BOOL WINAPI SetupInstallFilesFromInfSectionW( HINF hinf, HINF hlayout, HSPFILEQ queue,
                                              PCWSTR section, PCWSTR src_root, UINT flags )
{
    struct files_callback_info info;

    info.queue      = queue;
    info.src_root   = src_root;
    info.copy_flags = flags;
    info.layout     = hlayout;
    return iterate_section_fields( hinf, section, CopyFiles, copy_files_callback, &info );
}


/***********************************************************************
 *            SetupInstallFromInfSectionA   (SETUPAPI.@)
 */
BOOL WINAPI SetupInstallFromInfSectionA( HWND owner, HINF hinf, PCSTR section, UINT flags,
                                         HKEY key_root, PCSTR src_root, UINT copy_flags,
                                         PSP_FILE_CALLBACK_A callback, PVOID context,
                                         HDEVINFO devinfo, PSP_DEVINFO_DATA devinfo_data )
{
    UNICODE_STRING sectionW, src_rootW;
    struct callback_WtoA_context ctx;
    BOOL ret = FALSE;

    src_rootW.Buffer = NULL;
    if (src_root && !RtlCreateUnicodeStringFromAsciiz( &src_rootW, src_root ))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    if (RtlCreateUnicodeStringFromAsciiz( &sectionW, section ))
    {
        ctx.orig_context = context;
        ctx.orig_handler = callback;
        ret = SetupInstallFromInfSectionW( owner, hinf, sectionW.Buffer, flags, key_root,
                                           src_rootW.Buffer, copy_flags, QUEUE_callback_WtoA,
                                           &ctx, devinfo, devinfo_data );
        RtlFreeUnicodeString( &sectionW );
    }
    else SetLastError( ERROR_NOT_ENOUGH_MEMORY );

    RtlFreeUnicodeString( &src_rootW );
    return ret;
}


/***********************************************************************
 *            SetupInstallFromInfSectionW   (SETUPAPI.@)
 */
BOOL WINAPI SetupInstallFromInfSectionW( HWND owner, HINF hinf, PCWSTR section, UINT flags,
                                         HKEY key_root, PCWSTR src_root, UINT copy_flags,
                                         PSP_FILE_CALLBACK_W callback, PVOID context,
                                         HDEVINFO devinfo, PSP_DEVINFO_DATA devinfo_data )
{
    BOOL ret;
    int i;

    if (flags & SPINST_REGISTRY)
    {
        struct registry_callback_info info;

        info.default_root = key_root;
        info.delete = FALSE;
        if (!iterate_section_fields( hinf, section, WinePreInstall, registry_callback, &info ))
            return FALSE;
    }
    if (flags & SPINST_FILES)
    {
        struct files_callback_info info;
        HSPFILEQ queue;

        if (!(queue = SetupOpenFileQueue())) return FALSE;
        info.queue      = queue;
        info.src_root   = src_root;
        info.copy_flags = copy_flags;
        info.layout     = hinf;
        ret = (iterate_section_fields( hinf, section, CopyFiles, copy_files_callback, &info ) &&
               iterate_section_fields( hinf, section, DelFiles, delete_files_callback, &info ) &&
               iterate_section_fields( hinf, section, RenFiles, rename_files_callback, &info ) &&
               SetupCommitFileQueueW( owner, queue, callback, context ));
        SetupCloseFileQueue( queue );
        if (!ret) return FALSE;
    }
    if (flags & SPINST_INIFILES)
    {
        if (!iterate_section_fields( hinf, section, UpdateInis, update_ini_callback, NULL ) ||
            !iterate_section_fields( hinf, section, UpdateIniFields,
                                     update_ini_fields_callback, NULL ))
            return FALSE;
    }
    if (flags & SPINST_INI2REG)
    {
        if (!iterate_section_fields( hinf, section, Ini2Reg, ini2reg_callback, NULL ))
            return FALSE;
    }
    if (flags & SPINST_LOGCONFIG)
    {
        if (!iterate_section_fields( hinf, section, LogConf, logconf_callback, NULL ))
            return FALSE;
    }
    if (flags & SPINST_REGSVR)
    {
        struct register_dll_info info;

        info.unregister    = FALSE;
        info.modules_size  = 0;
        info.modules_count = 0;
        info.modules       = NULL;
        if (flags & SPINST_REGISTERCALLBACKAWARE)
        {
            info.callback         = callback;
            info.callback_context = context;
        }
        else info.callback = NULL;

        if (iterate_section_fields( hinf, section, WineFakeDlls, fake_dlls_callback, NULL ))
            cleanup_fake_dlls();
        else
            return FALSE;

        ret = iterate_section_fields( hinf, section, RegisterDlls, register_dlls_callback, &info );
        for (i = 0; i < info.modules_count; i++) FreeLibrary( info.modules[i] );
        HeapFree( GetProcessHeap(), 0, info.modules );
        if (!ret) return FALSE;
    }
    if (flags & SPINST_UNREGSVR)
    {
        struct register_dll_info info;

        info.unregister    = TRUE;
        info.modules_size  = 0;
        info.modules_count = 0;
        info.modules       = NULL;
        if (flags & SPINST_REGISTERCALLBACKAWARE)
        {
            info.callback         = callback;
            info.callback_context = context;
        }
        else info.callback = NULL;

        ret = iterate_section_fields( hinf, section, UnregisterDlls, register_dlls_callback, &info );
        for (i = 0; i < info.modules_count; i++) FreeLibrary( info.modules[i] );
        HeapFree( GetProcessHeap(), 0, info.modules );
        if (!ret) return FALSE;
    }
    if (flags & SPINST_REGISTRY)
    {
        struct registry_callback_info info;

        info.default_root = key_root;
        info.delete = TRUE;
        if (!iterate_section_fields( hinf, section, DelReg, registry_callback, &info ))
            return FALSE;
        info.delete = FALSE;
        if (!iterate_section_fields( hinf, section, AddReg, registry_callback, &info ))
            return FALSE;
    }
    if (flags & SPINST_BITREG)
    {
        if (!iterate_section_fields( hinf, section, BitReg, bitreg_callback, NULL ))
            return FALSE;
    }
    if (flags & SPINST_PROFILEITEMS)
    {
        if (!iterate_section_fields( hinf, section, ProfileItems, profile_items_callback, NULL ))
            return FALSE;
    }
    if (flags & SPINST_COPYINF)
    {
        if (!iterate_section_fields( hinf, section, CopyINF, copy_inf_callback, NULL ))
            return FALSE;
    }

    SetLastError(ERROR_SUCCESS);
    return TRUE;
}


/***********************************************************************
 *		InstallHinfSectionW  (SETUPAPI.@)
 *
 * NOTE: 'cmdline' is <section> <mode> <path> from
 *   RUNDLL32.EXE SETUPAPI.DLL,InstallHinfSection <section> <mode> <path>
 */
void WINAPI InstallHinfSectionW( HWND hwnd, HINSTANCE handle, LPCWSTR cmdline, INT show )
{
#ifdef __i386__
    static const WCHAR nt_platformW[] = {'.','n','t','x','8','6',0};
#elif defined(__x86_64__)
    static const WCHAR nt_platformW[] = {'.','n','t','a','m','d','6','4',0};
#else  /* FIXME: other platforms */
    static const WCHAR nt_platformW[] = {'.','n','t',0};
#endif
    static const WCHAR nt_genericW[] = {'.','n','t',0};
    static const WCHAR servicesW[] = {'.','S','e','r','v','i','c','e','s',0};

    WCHAR *s, *path, section[MAX_PATH + ARRAY_SIZE( nt_platformW ) + ARRAY_SIZE( servicesW )];
    void *callback_context;
    UINT mode;
    HINF hinf;

    TRACE("hwnd %p, handle %p, cmdline %s\n", hwnd, handle, debugstr_w(cmdline));

    lstrcpynW( section, cmdline, MAX_PATH );

    if (!(s = wcschr( section, ' ' ))) return;
    *s++ = 0;
    while (*s == ' ') s++;
    mode = wcstol( s, NULL, 10 );

    /* quoted paths are not allowed on native, the rest of the command line is taken as the path */
    if (!(s = wcschr( s, ' ' ))) return;
    while (*s == ' ') s++;
    path = s;

    hinf = SetupOpenInfFileW( path, NULL, INF_STYLE_WIN4, NULL );
    if (hinf == INVALID_HANDLE_VALUE) return;

    if (!(GetVersion() & 0x80000000))
    {
        INFCONTEXT context;

        /* check for <section>.ntx86 (or corresponding name for the current platform)
         * and then <section>.nt */
        s = section + lstrlenW(section);
        memcpy( s, nt_platformW, sizeof(nt_platformW) );
        if (!(SetupFindFirstLineW( hinf, section, NULL, &context )))
        {
            memcpy( s, nt_genericW, sizeof(nt_genericW) );
            if (!(SetupFindFirstLineW( hinf, section, NULL, &context ))) *s = 0;
        }
        if (*s) TRACE( "using section %s instead\n", debugstr_w(section) );
    }

    callback_context = SetupInitDefaultQueueCallback( hwnd );
    SetupInstallFromInfSectionW( hwnd, hinf, section, SPINST_ALL, NULL, NULL, SP_COPY_NEWER,
                                 SetupDefaultQueueCallbackW, callback_context,
                                 NULL, NULL );
    SetupTermDefaultQueueCallback( callback_context );
    lstrcatW( section, servicesW );
    SetupInstallServicesFromInfSectionW( hinf, section, 0 );
    SetupCloseInfFile( hinf );

    /* FIXME: should check the mode and maybe reboot */
    /* there isn't much point in doing that since we */
    /* don't yet handle deferred file copies anyway. */
    if (mode & 7) TRACE( "should consider reboot, mode %u\n", mode );
}


/***********************************************************************
 *		InstallHinfSectionA  (SETUPAPI.@)
 */
void WINAPI InstallHinfSectionA( HWND hwnd, HINSTANCE handle, LPCSTR cmdline, INT show )
{
    UNICODE_STRING cmdlineW;

    if (RtlCreateUnicodeStringFromAsciiz( &cmdlineW, cmdline ))
    {
        InstallHinfSectionW( hwnd, handle, cmdlineW.Buffer, show );
        RtlFreeUnicodeString( &cmdlineW );
    }
}


/***********************************************************************
 *            add_service
 *
 * Create a new service. Helper for SetupInstallServicesFromInfSectionW.
 */
static BOOL add_service( SC_HANDLE scm, HINF hinf, const WCHAR *name, const WCHAR *section, DWORD flags )
{
    struct registry_callback_info info;
    SC_HANDLE service;
    INFCONTEXT context;
    SERVICE_DESCRIPTIONW descr;
    WCHAR *display_name, *start_name, *load_order, *binary_path;
    INT service_type = 0, start_type = 0, error_control = 0;
    DWORD size;
    HKEY hkey;

    /* first the mandatory fields */

    if (!SetupFindFirstLineW( hinf, section, ServiceType, &context ) ||
        !SetupGetIntField( &context, 1, &service_type ))
    {
        SetLastError( ERROR_BAD_SERVICE_INSTALLSECT );
        return FALSE;
    }
    if (!SetupFindFirstLineW( hinf, section, StartType, &context ) ||
        !SetupGetIntField( &context, 1, &start_type ))
    {
        SetLastError( ERROR_BAD_SERVICE_INSTALLSECT );
        return FALSE;
    }
    if (!SetupFindFirstLineW( hinf, section, ErrorControl, &context ) ||
        !SetupGetIntField( &context, 1, &error_control ))
    {
        SetLastError( ERROR_BAD_SERVICE_INSTALLSECT );
        return FALSE;
    }
    if (!(binary_path = dup_section_line_field( hinf, section, ServiceBinary, 1 )))
    {
        SetLastError( ERROR_BAD_SERVICE_INSTALLSECT );
        return FALSE;
    }

    /* now the optional fields */

    display_name = dup_section_line_field( hinf, section, DisplayName, 1 );
    start_name = dup_section_line_field( hinf, section, StartName, 1 );
    load_order = dup_section_line_field( hinf, section, LoadOrderGroup, 1 );
    descr.lpDescription = dup_section_line_field( hinf, section, Description, 1 );

    /* FIXME: Dependencies field */
    /* FIXME: Security field */

    TRACE( "service %s display %s type %x start %x error %x binary %s order %s startname %s flags %x\n",
           debugstr_w(name), debugstr_w(display_name), service_type, start_type, error_control,
           debugstr_w(binary_path), debugstr_w(load_order), debugstr_w(start_name), flags );

    service = CreateServiceW( scm, name, display_name, SERVICE_ALL_ACCESS,
                              service_type, start_type, error_control, binary_path,
                              load_order, NULL, NULL, start_name, NULL );
    if (service)
    {
        if (descr.lpDescription) ChangeServiceConfig2W( service, SERVICE_CONFIG_DESCRIPTION, &descr );
    }
    else
    {
        if (GetLastError() != ERROR_SERVICE_EXISTS) goto done;
        service = OpenServiceW( scm, name, SERVICE_QUERY_CONFIG|SERVICE_CHANGE_CONFIG|SERVICE_START );
        if (!service) goto done;

        if (flags & (SPSVCINST_NOCLOBBER_DISPLAYNAME | SPSVCINST_NOCLOBBER_STARTTYPE |
                     SPSVCINST_NOCLOBBER_ERRORCONTROL | SPSVCINST_NOCLOBBER_LOADORDERGROUP))
        {
            QUERY_SERVICE_CONFIGW *config = NULL;

            if (!QueryServiceConfigW( service, NULL, 0, &size ) &&
                GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                config = HeapAlloc( GetProcessHeap(), 0, size );
            if (config && QueryServiceConfigW( service, config, size, &size ))
            {
                if (flags & SPSVCINST_NOCLOBBER_STARTTYPE) start_type = config->dwStartType;
                if (flags & SPSVCINST_NOCLOBBER_ERRORCONTROL) error_control = config->dwErrorControl;
                if (flags & SPSVCINST_NOCLOBBER_DISPLAYNAME)
                {
                    HeapFree( GetProcessHeap(), 0, display_name );
                    display_name = strdupW( config->lpDisplayName );
                }
                if (flags & SPSVCINST_NOCLOBBER_LOADORDERGROUP)
                {
                    HeapFree( GetProcessHeap(), 0, load_order );
                    load_order = strdupW( config->lpLoadOrderGroup );
                }
            }
            HeapFree( GetProcessHeap(), 0, config );
        }
        TRACE( "changing %s display %s type %x start %x error %x binary %s loadorder %s startname %s\n",
               debugstr_w(name), debugstr_w(display_name), service_type, start_type, error_control,
               debugstr_w(binary_path), debugstr_w(load_order), debugstr_w(start_name) );

        ChangeServiceConfigW( service, service_type, start_type, error_control, binary_path,
                              load_order, NULL, NULL, start_name, NULL, display_name );

        if (!(flags & SPSVCINST_NOCLOBBER_DESCRIPTION))
            ChangeServiceConfig2W( service, SERVICE_CONFIG_DESCRIPTION, &descr );
    }

    /* execute the AddReg, DelReg and BitReg entries */

    info.default_root = 0;
    if (!RegOpenKeyW( HKEY_LOCAL_MACHINE, ServicesKey, &hkey ))
    {
        RegOpenKeyW( hkey, name, &info.default_root );
        RegCloseKey( hkey );
    }
    if (info.default_root)
    {
        info.delete = TRUE;
        iterate_section_fields( hinf, section, DelReg, registry_callback, &info );
        info.delete = FALSE;
        iterate_section_fields( hinf, section, AddReg, registry_callback, &info );
        RegCloseKey( info.default_root );
    }
    iterate_section_fields( hinf, section, BitReg, bitreg_callback, NULL );

    if (flags & SPSVCINST_STARTSERVICE) StartServiceW( service, 0, NULL );
    CloseServiceHandle( service );

done:
    if (!service) WARN( "failed err %u\n", GetLastError() );
    HeapFree( GetProcessHeap(), 0, binary_path );
    HeapFree( GetProcessHeap(), 0, display_name );
    HeapFree( GetProcessHeap(), 0, start_name );
    HeapFree( GetProcessHeap(), 0, load_order );
    HeapFree( GetProcessHeap(), 0, descr.lpDescription );
    return service != 0;
}


/***********************************************************************
 *            del_service
 *
 * Delete service. Helper for SetupInstallServicesFromInfSectionW.
 */
static BOOL del_service( SC_HANDLE scm, HINF hinf, const WCHAR *name, DWORD flags )
{
    BOOL ret;
    SC_HANDLE service;
    SERVICE_STATUS status;

    if (!(service = OpenServiceW( scm, name, SERVICE_STOP | DELETE )))
    {
        if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST) return TRUE;
        WARN( "cannot open %s err %u\n", debugstr_w(name), GetLastError() );
        return FALSE;
    }
    if (flags & SPSVCINST_STOPSERVICE) ControlService( service, SERVICE_CONTROL_STOP, &status );
    TRACE( "deleting %s\n", debugstr_w(name) );
    ret = DeleteService( service );
    CloseServiceHandle( service );
    return ret;
}


/***********************************************************************
 *              SetupInstallServicesFromInfSectionW  (SETUPAPI.@)
 */
BOOL WINAPI SetupInstallServicesFromInfSectionW( HINF hinf, PCWSTR section, DWORD flags )
{
    WCHAR service_name[MAX_INF_STRING_LENGTH];
    WCHAR service_section[MAX_INF_STRING_LENGTH];
    SC_HANDLE scm;
    INFCONTEXT context;
    INT section_flags;
    BOOL ret = TRUE;

    if (!SetupFindFirstLineW( hinf, section, NULL, &context ))
    {
        SetLastError( ERROR_SECTION_NOT_FOUND );
        return FALSE;
    }
    if (!(scm = OpenSCManagerW( NULL, NULL, SC_MANAGER_ALL_ACCESS ))) return FALSE;

    if (SetupFindFirstLineW( hinf, section, AddService, &context ))
    {
        do
        {
            if (!SetupGetStringFieldW( &context, 1, service_name, MAX_INF_STRING_LENGTH, NULL ))
                continue;
            if (!SetupGetIntField( &context, 2, &section_flags )) section_flags = 0;
            if (!SetupGetStringFieldW( &context, 3, service_section, MAX_INF_STRING_LENGTH, NULL ))
                continue;
            if (!(ret = add_service( scm, hinf, service_name, service_section, section_flags | flags )))
                goto done;
        } while (SetupFindNextMatchLineW( &context, AddService, &context ));
    }

    if (SetupFindFirstLineW( hinf, section, DelService, &context ))
    {
        do
        {
            if (!SetupGetStringFieldW( &context, 1, service_name, MAX_INF_STRING_LENGTH, NULL ))
                continue;
            if (!SetupGetIntField( &context, 2, &section_flags )) section_flags = 0;
            if (!(ret = del_service( scm, hinf, service_name, section_flags | flags ))) goto done;
        } while (SetupFindNextMatchLineW( &context, AddService, &context ));
    }
    if (ret) SetLastError( ERROR_SUCCESS );
 done:
    CloseServiceHandle( scm );
    return ret;
}


/***********************************************************************
 *              SetupInstallServicesFromInfSectionA  (SETUPAPI.@)
 */
BOOL WINAPI SetupInstallServicesFromInfSectionA( HINF Inf, PCSTR SectionName, DWORD Flags)
{
    UNICODE_STRING SectionNameW;
    BOOL ret = FALSE;

    if (RtlCreateUnicodeStringFromAsciiz( &SectionNameW, SectionName ))
    {
        ret = SetupInstallServicesFromInfSectionW( Inf, SectionNameW.Buffer, Flags );
        RtlFreeUnicodeString( &SectionNameW );
    }
    else
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );

    return ret;
}


/***********************************************************************
 *              SetupGetInfFileListA  (SETUPAPI.@)
 */
BOOL WINAPI SetupGetInfFileListA(PCSTR dir, DWORD style, PSTR buffer,
                                 DWORD insize, PDWORD outsize)
{
    UNICODE_STRING dirW;
    PWSTR bufferW = NULL;
    BOOL ret = FALSE;
    DWORD outsizeA, outsizeW;

    if ( dir )
        RtlCreateUnicodeStringFromAsciiz( &dirW, dir );
    else
        dirW.Buffer = NULL;

    if ( buffer )
        bufferW = HeapAlloc( GetProcessHeap(), 0, insize * sizeof( WCHAR ));

    ret = SetupGetInfFileListW( dirW.Buffer, style, bufferW, insize, &outsizeW);

    if ( ret )
    {
        outsizeA = WideCharToMultiByte( CP_ACP, 0, bufferW, outsizeW,
                                        buffer, insize, NULL, NULL);
        if ( outsize ) *outsize = outsizeA;
    }

    HeapFree( GetProcessHeap(), 0, bufferW );
    RtlFreeUnicodeString( &dirW );
    return ret;
}


/***********************************************************************
 *              SetupGetInfFileListW  (SETUPAPI.@)
 */
BOOL WINAPI SetupGetInfFileListW(PCWSTR dir, DWORD style, PWSTR buffer,
                                 DWORD insize, PDWORD outsize)
{
    static const WCHAR inf[] = {'\\','*','.','i','n','f',0 };
    WCHAR *filter, *fullname = NULL, *ptr = buffer;
    DWORD dir_len, name_len = 20, size ;
    WIN32_FIND_DATAW finddata;
    HANDLE hdl;
    if (style & ~( INF_STYLE_OLDNT | INF_STYLE_WIN4 |
                   INF_STYLE_CACHE_ENABLE | INF_STYLE_CACHE_DISABLE ))
    {
        FIXME( "unknown inf_style(s) 0x%x\n",
               style & ~( INF_STYLE_OLDNT | INF_STYLE_WIN4 |
                         INF_STYLE_CACHE_ENABLE | INF_STYLE_CACHE_DISABLE ));
        if( outsize ) *outsize = 1;
        return TRUE;
    }
    if ((style & ( INF_STYLE_OLDNT | INF_STYLE_WIN4 )) == INF_STYLE_NONE)
    {
        FIXME( "inf_style INF_STYLE_NONE not handled\n" );
        if( outsize ) *outsize = 1;
        return TRUE;
    }
    if (style & ( INF_STYLE_CACHE_ENABLE | INF_STYLE_CACHE_DISABLE ))
        FIXME("ignored inf_style(s) %s %s\n",
              ( style & INF_STYLE_CACHE_ENABLE  ) ? "INF_STYLE_CACHE_ENABLE"  : "",
              ( style & INF_STYLE_CACHE_DISABLE ) ? "INF_STYLE_CACHE_DISABLE" : "");
    if( dir )
    {
        DWORD att;
        DWORD msize;
        dir_len = lstrlenW( dir );
        if ( !dir_len ) return FALSE;
        msize = ( 7 + dir_len )  * sizeof( WCHAR ); /* \\*.inf\0 */
        filter = HeapAlloc( GetProcessHeap(), 0, msize );
        if( !filter )
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return FALSE;
        }
        lstrcpyW( filter, dir );
        if ( '\\' == filter[dir_len - 1] )
            filter[--dir_len] = 0;

        att = GetFileAttributesW( filter );
        if (att != INVALID_FILE_ATTRIBUTES && !(att & FILE_ATTRIBUTE_DIRECTORY))
        {
            HeapFree( GetProcessHeap(), 0, filter );
            SetLastError( ERROR_DIRECTORY );
            return FALSE;
        }
    }
    else
    {
        static const WCHAR infdir[] = {'\\','i','n','f',0};
        DWORD msize;
        dir_len = GetWindowsDirectoryW( NULL, 0 );
        msize = ( 7 + 4 + dir_len ) * sizeof( WCHAR );
        filter = HeapAlloc( GetProcessHeap(), 0, msize );
        if( !filter )
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return FALSE;
        }
        GetWindowsDirectoryW( filter, msize );
        lstrcatW( filter, infdir );
    }
    lstrcatW( filter, inf );

    hdl = FindFirstFileW( filter , &finddata );
    if ( hdl == INVALID_HANDLE_VALUE )
    {
        if( outsize ) *outsize = 1;
        HeapFree( GetProcessHeap(), 0, filter );
        return TRUE;
    }
    size = 1;
    do
    {
        static const WCHAR key[] =
               {'S','i','g','n','a','t','u','r','e',0 };
        static const WCHAR section[] =
               {'V','e','r','s','i','o','n',0 };
        static const WCHAR sig_win4_1[] =
               {'$','C','h','i','c','a','g','o','$',0 };
        static const WCHAR sig_win4_2[] =
               {'$','W','I','N','D','O','W','S',' ','N','T','$',0 };
        WCHAR signature[ MAX_PATH ];
        BOOL valid = FALSE;
        DWORD len = lstrlenW( finddata.cFileName );
        if (!fullname || ( name_len < len ))
        {
            name_len = ( name_len < len ) ? len : name_len;
            HeapFree( GetProcessHeap(), 0, fullname );
            fullname = HeapAlloc( GetProcessHeap(), 0,
                                  ( 2 + dir_len + name_len) * sizeof( WCHAR ));
            if( !fullname )
            {
                FindClose( hdl );
                HeapFree( GetProcessHeap(), 0, filter );
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                return FALSE;
            }
            lstrcpyW( fullname, filter );
        }
        fullname[ dir_len + 1] = 0; /* keep '\\' */
        lstrcatW( fullname, finddata.cFileName );
        if (!GetPrivateProfileStringW( section, key, NULL, signature, MAX_PATH, fullname ))
            signature[0] = 0;
        if( INF_STYLE_OLDNT & style )
            valid = wcsicmp( sig_win4_1, signature ) &&
                    wcsicmp( sig_win4_2, signature );
        if( INF_STYLE_WIN4 & style )
            valid = valid || !wcsicmp( sig_win4_1, signature ) ||
                    !wcsicmp( sig_win4_2, signature );
        if( valid )
        {
            size += 1 + lstrlenW( finddata.cFileName );
            if( ptr && insize >= size )
            {
                lstrcpyW( ptr, finddata.cFileName );
                ptr += 1 + lstrlenW( finddata.cFileName );
                *ptr = 0;
            }
        }
    }
    while( FindNextFileW( hdl, &finddata ));
    FindClose( hdl );

    HeapFree( GetProcessHeap(), 0, fullname );
    HeapFree( GetProcessHeap(), 0, filter );
    if( outsize ) *outsize = size;
    return TRUE;
}
