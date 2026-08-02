/*
 * Copyright 2010 Louis Lenders
 * Copyright 2012 Hans Leidekker for CodeWeavers
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

#define COBJMACROS

#include <fcntl.h>
#include <io.h>
#include <locale.h>
#include <stdio.h>
#include "windows.h"
#include "ocidl.h"
#include "initguid.h"
#include "objidl.h"
#include "wbemcli.h"
#include "shellapi.h"
#include "wmic.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wmic);

#define MAX_STRING    4096
static const struct
{
    const WCHAR *alias;
    const WCHAR *class;
}
alias_map[] =
{
    { L"baseboard", L"Win32_BaseBoard" },
    { L"bios", L"Win32_BIOS" },
    { L"computersystem", L"Win32_ComputerSystem" },
    { L"cpu", L"Win32_Processor" },
    { L"csproduct", L"Win32_ComputerSystemProduct" },
    { L"diskdrive", L"Win32_DiskDrive" },
    { L"logicaldisk", L"Win32_LogicalDisk" },
    { L"memorychip", L"Win32_PhysicalMemory" },
    { L"nic", L"Win32_NetworkAdapter" },
    { L"nicconfig", L"Win32_NetworkAdapterConfiguration" },
    { L"os", L"Win32_OperatingSystem" },
    { L"process", L"Win32_Process" },
    { L"systemenclosure", L"Win32_SystemEnclosure" },
};

static const WCHAR *find_class( const WCHAR *alias )
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(alias_map); i++)
    {
        if (!wcsicmp( alias, alias_map[i].alias )) return alias_map[i].class;
    }
    return NULL;
}

static int WINAPIV output_string( const WCHAR *msg, ... )
{
    int count, bom_count = 0;
    static BOOL bom;
    va_list va_args;

    if (!bom)
    {
        if (GetFileType((HANDLE)_get_osfhandle( STDOUT_FILENO )) == FILE_TYPE_DISK)
        {
            _setmode( STDOUT_FILENO, _O_U16TEXT );
            bom_count = wprintf( L"\xfeff" );
        }
        bom = TRUE;
    }

    va_start( va_args, msg );
    count = vwprintf( msg, va_args );
    va_end( va_args );
    return count + bom_count;
}

static int output_error( int msg )
{
    WCHAR buffer[8192];

    LoadStringW( GetModuleHandleW(NULL), msg, buffer, ARRAY_SIZE(buffer));
    return fwprintf( stderr, L"%s", buffer );
}

static int output_text( const WCHAR *str, ULONG column_width )
{
    return output_string( L"%-*s", column_width, str );
}

static int output_newline( void )
{
    return output_string( L"\n" );
}

static WCHAR *strip_spaces( WCHAR *start )
{
    WCHAR *str = start, *end = start + wcslen( start ) - 1;
    while (*str == ' ') str++;
    while (end >= start && *end == ' ') *end-- = 0;
    return str;
}

static HRESULT append_property( IWbemClassObject *obj, const WCHAR *prop, WCHAR *proplist )
{
    HRESULT hr;

    if (FAILED(hr = IWbemClassObject_Get( obj, prop, 0, NULL, NULL, NULL ))) return hr;
    if (*proplist) wcscat( proplist, L"," );
    wcscat( proplist, prop );
    return S_OK;
}

static HRESULT process_property_list( IWbemClassObject *obj, int argc, WCHAR *argv[], WCHAR **ret )
{
    WCHAR *str = NULL, *ctx, *ptr, *stripped;
    UINT i, len = 0;
    HRESULT hr;

    for (i = 0; i < argc; i++) len += wcslen( argv[i] );
    if (!(stripped = malloc( (len + 1) * sizeof(*stripped) ))) return E_OUTOFMEMORY;
    *stripped = 0;

    for (i = 0; i < argc; i++)
    {
        if (!(str = wcsdup( argv[i] )))
        {
            free( stripped );
            return E_OUTOFMEMORY;
        }

        /* Validate that every requested property is supported. */
        ptr = wcstok_s( str, L",", &ctx );
        if (!ptr)
        {
            ptr = strip_spaces( str );
            if (FAILED(hr = append_property( obj, ptr, stripped ))) goto error;
        }
        else
        {
            while (ptr)
            {
                ptr = strip_spaces( ptr );
                if (FAILED(hr = append_property( obj, ptr, stripped ))) goto error;
                ptr = wcstok_s( NULL, L",", &ctx );
            }
        }
        free( str );
    }

    *ret = stripped;
    return S_OK;

error:
    free( str );
    free( stripped );
    return hr;
}

static void convert_to_bstr( VARIANT *v )
{
    BSTR out = NULL;
    VARTYPE vt;

    /* treat VT_I4 values as unsigned */
    if (V_VT(v) == VT_I4) V_VT(v) = VT_UI4;

    if (SUCCEEDED(VariantChangeType( v, v, 0, VT_BSTR ))) return;
    vt = V_VT(v);
    if (vt == (VT_ARRAY | VT_BSTR))
    {
        unsigned int i, count, len;
        BSTR *strings;
        WCHAR *ptr;

        if (FAILED(SafeArrayAccessData( V_ARRAY(v), (void **)&strings )))
        {
            WINE_ERR( "Could not access array.\n" );
            goto done;
        }
        count = V_ARRAY(v)->rgsabound->cElements;
        len = 0;
        for (i = 0; i < count; ++i)
            len += wcslen( strings[i] );
        len += count * 2 + 2;
        if (count) len += 2 * (count - 1);
        out = SysAllocStringLen( NULL, len );
        ptr = out;
        *ptr++ = '{';
        for (i = 0; i < count; ++i)
        {
            if (i)
            {
                memcpy( ptr, L", ", 2 * sizeof(*ptr) );
                ptr += 2;
            }
            *ptr++ = '\"';
            len = wcslen( strings[i] );
            memcpy( ptr, strings[i], len * sizeof(*ptr) );
            ptr += len;
            *ptr++ = '\"';
        }
        *ptr++ = '}';
        *ptr = 0;
        SafeArrayUnaccessData( V_ARRAY(v) );
    }
done:
    VariantClear( v );
    V_VT(v) = VT_BSTR;
    V_BSTR(v) = out ? out : SysAllocString( L"" );
    if (vt != VT_NULL && vt != VT_EMPTY && !out)
        WINE_FIXME( "Could not convert variant, vt %u.\n", vt );
}

static int query_prop( const WCHAR *class, int argc, WCHAR *argv[] )
{
    HRESULT hr;
    IWbemLocator *locator = NULL;
    IWbemServices *services = NULL;
    IEnumWbemClassObject *result = NULL;
    LONG flags = WBEM_FLAG_RETURN_IMMEDIATELY;
    BSTR path = NULL, wql = NULL, query = NULL, name, str = NULL;
    WCHAR *proplist = NULL;
    int len, ret = -1;
    IWbemClassObject *obj;
    ULONG count, width = 0;
    VARIANT v;
    int i;

    WINE_TRACE( "%s", debugstr_w(class) );
    for (i = 0; i < argc; i++) WINE_TRACE( " %s", debugstr_w(argv[i]) );
    WINE_TRACE( "\n" );

    CoInitialize( NULL );
    CoInitializeSecurity( NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
                          RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL );

    hr = CoCreateInstance( &CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, &IID_IWbemLocator,
                           (void **)&locator );
    if (hr != S_OK) goto done;

    if (!(path = SysAllocString(L"ROOT\\CIMV2" ))) goto done;
    hr = IWbemLocator_ConnectServer( locator, path, NULL, NULL, NULL, 0, NULL, NULL, &services );
    if (hr != S_OK) goto done;

    if (!(str = SysAllocString( class ))) goto done;
    hr = IWbemServices_GetObject( services, str, 0, NULL, &obj, NULL );
    SysFreeString( str );
    if (hr != S_OK)
    {
        WARN("Unrecognized class %s.\n", debugstr_w(class));
        goto done;
    }

    /* Check that this class supports all requested properties. */
    hr = process_property_list( obj, argc, argv, &proplist );
    IWbemClassObject_Release( obj );
    if (FAILED(hr))
    {
        output_error( STRING_INVALID_QUERY );
        goto done;
    }

    len = lstrlenW( class ) + lstrlenW( proplist ) + ARRAY_SIZE(L"SELECT * FROM ");
    if (!(query = SysAllocStringLen( NULL, len ))) goto done;
    swprintf( query, len, L"SELECT %s FROM %s", proplist, class );

    if (!(wql = SysAllocString(L"WQL" ))) goto done;
    hr = IWbemServices_ExecQuery( services, wql, query, flags, NULL, &result );
    if (hr != S_OK) goto done;

    for (;;) /* get column width */
    {
        IEnumWbemClassObject_Next( result, WBEM_INFINITE, 1, &obj, &count );
        if (!count) break;

        IWbemClassObject_BeginEnumeration( obj, 0 );
        while (IWbemClassObject_Next( obj, 0, &name, &v, NULL, NULL ) == S_OK)
        {
            convert_to_bstr( &v );
            width = max( lstrlenW( V_BSTR( &v ) ), width );
            VariantClear( &v );
            SysFreeString( name );
        }

        IWbemClassObject_Release( obj );
    }
    width += 2;

    /* Header */
    IEnumWbemClassObject_Reset( result );
    IEnumWbemClassObject_Next( result, WBEM_INFINITE, 1, &obj, &count );
    if (count)
    {
        IWbemClassObject_BeginEnumeration( obj, 0 );
        while (IWbemClassObject_Next( obj, 0, &name, NULL, NULL, NULL ) == S_OK)
        {
            output_text( name, width );
            SysFreeString( name );
        }
        output_newline();
        IWbemClassObject_Release( obj );
    }

    /* Values */
    IEnumWbemClassObject_Reset( result );
    for (;;)
    {
        IEnumWbemClassObject_Next( result, WBEM_INFINITE, 1, &obj, &count );
        if (!count) break;
        IWbemClassObject_BeginEnumeration( obj, 0 );
        while (IWbemClassObject_Next( obj, 0, NULL, &v, NULL, NULL ) == S_OK)
        {
            convert_to_bstr( &v );
            output_text( V_BSTR( &v ), width );
            VariantClear( &v );
        }
        output_newline();
        IWbemClassObject_Release( obj );
    }
    ret = 0;

done:
    if (result) IEnumWbemClassObject_Release( result );
    if (services) IWbemServices_Release( services );
    if (locator) IWbemLocator_Release( locator );
    SysFreeString( path );
    SysFreeString( query );
    SysFreeString( wql );
    free( proplist );
    CoUninitialize();
    return ret;
}

static int process_args( int argc, WCHAR *argv[] )
{
    const WCHAR *class;
    int i;

    for (i = 0; i < argc && argv[i][0] == '/'; i++)
        WINE_FIXME( "switch %s not supported\n", debugstr_w(argv[i]) );

    if (i >= argc)
        goto not_supported;

    if (!wcsicmp( argv[i], L"quit" ) || !wcsicmp( argv[i], L"exit" ))
    {
        return 0;
    }

    if (!wcsicmp( argv[i], L"class" ) || !wcsicmp( argv[i], L"context" ))
    {
        WINE_FIXME( "command %s not supported\n", debugstr_w(argv[i]) );
        goto not_supported;
    }

    if (!wcsicmp( argv[i], L"path" ))
    {
        if (++i >= argc)
        {
            output_error( STRING_INVALID_PATH );
            return 1;
        }
        class = argv[i];
    }
    else
    {
        class = find_class( argv[i] );
        if (!class)
        {
            output_error( STRING_ALIAS_NOT_FOUND );
            return 1;
        }
    }

    if (++i >= argc)
        goto not_supported;

    if (!wcsicmp( argv[i], L"get" ))
    {
        if (++i >= argc)
            goto not_supported;
        return query_prop( class, argc - i, argv + i );
    }

not_supported:
    output_error( STRING_COMMAND_NOT_SUPPORTED );
    return 1;
}

int __cdecl wmain(int argc, WCHAR *argv[])
{
    WCHAR cmd[MAX_STRING];
    int ret = 0;

    setlocale( LC_ALL, "" );

    if (argc == 1)
    {
        fputws( L"wmic:root\\cli>", stdout );

        while (fgetws( cmd, sizeof(cmd), stdin ) != NULL)
        {
            const WCHAR *stripped;

            cmd[wcslen(cmd) - 1] = 0; /* remove trailing '\n' */
            stripped = strip_spaces( cmd );

            WINE_TRACE( "command: %s\n", debugstr_w(stripped) );
            if (!wcsicmp( stripped, L"exit" ) || !wcsicmp( stripped, L"quit" ))
                return 0;

            if (!stripped[0])
                output_error( STRING_USAGE );
            else
            {
                int new_argc;
                WCHAR **new_argv;

                if (!(new_argv = CommandLineToArgvW( stripped, &new_argc ))) return 1;
                ret = process_args( new_argc, new_argv );
                LocalFree( new_argv );

                output_newline();
            }
            fputws( L"wmic:root\\cli>", stdout );
        }
        return ret;
    }

    return process_args( argc - 1, argv + 1 );
}
