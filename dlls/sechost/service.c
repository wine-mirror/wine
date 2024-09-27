/*
 * Service control API
 *
 * Copyright 1995 Sven Verdoolaege
 * Copyright 2005 Mike McCormack
 * Copyright 2007 Rolf Kalbermatter
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
#define WINADVAPI
#include "windef.h"
#include "winbase.h"
#include "winsvc.h"
#include "winternl.h"
#include "winuser.h"
#include "dbt.h"

#include "wine/debug.h"
#include "wine/exception.h"
#include "wine/list.h"

#include "svcctl.h"
#include "plugplay.h"

WINE_DEFAULT_DEBUG_CHANNEL(service);

struct notify_data
{
    SC_HANDLE service;
    SC_RPC_NOTIFY_PARAMS params;
    SERVICE_NOTIFY_STATUS_CHANGE_PARAMS_2 cparams;
    SC_NOTIFY_RPC_HANDLE notify_handle;
    SERVICE_NOTIFYW *notify_buffer;
    HANDLE calling_thread, ready_evt;
    struct list entry;
};

static struct list notify_list = LIST_INIT(notify_list);

static CRITICAL_SECTION service_cs;
static CRITICAL_SECTION_DEBUG service_cs_debug =
{
    0, 0, &service_cs,
    { &service_cs_debug.ProcessLocksList,
      &service_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": service_cs") }
};
static CRITICAL_SECTION service_cs = { &service_cs_debug, -1, 0, 0, 0, 0 };

struct service_data
{
    LPHANDLER_FUNCTION_EX handler;
    void *context;
    HANDLE thread;
    SC_HANDLE handle;
    SC_HANDLE full_access_handle;
    unsigned int unicode : 1;
    union
    {
        LPSERVICE_MAIN_FUNCTIONA a;
        LPSERVICE_MAIN_FUNCTIONW w;
    } proc;
    WCHAR *args;
    WCHAR name[1];
};

struct dispatcher_data
{
    SC_HANDLE manager;
    HANDLE pipe;
};

static struct service_data **services;
static unsigned int nb_services;
static HANDLE service_event;
static BOOL stop_service;

static WCHAR *strdupAtoW( const char *src )
{
    WCHAR *dst = NULL;
    if (src)
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, src, -1, NULL, 0 );
        if ((dst = malloc( len * sizeof(WCHAR) ))) MultiByteToWideChar( CP_ACP, 0, src, -1, dst, len );
    }
    return dst;
}

static WCHAR *strdup_multi_AtoW( const char *src )
{
    WCHAR *dst = NULL;
    const char *p = src;
    DWORD len;

    if (!src) return NULL;

    while (*p) p += strlen(p) + 1;
    for (p = src; *p; p += strlen(p) + 1);
    p++; /* final null */
    len = MultiByteToWideChar( CP_ACP, 0, src, p - src, NULL, 0 );
    if ((dst = malloc( len * sizeof(WCHAR) ))) MultiByteToWideChar( CP_ACP, 0, src, p - src, dst, len );
    return dst;
}

static inline DWORD multisz_size( const WCHAR *str )
{
    const WCHAR *p = str;

    if (!str) return 0;

    while (*p) p += wcslen(p) + 1;
    return (p - str + 1) * sizeof(WCHAR);
}

void  __RPC_FAR * __RPC_USER MIDL_user_allocate( SIZE_T len )
{
    return malloc(len);
}

void __RPC_USER MIDL_user_free( void __RPC_FAR *ptr )
{
    free(ptr);
}

static LONG WINAPI rpc_filter( EXCEPTION_POINTERS *eptr )
{
    return I_RpcExceptionFilter( eptr->ExceptionRecord->ExceptionCode );
}

static DWORD map_exception_code( DWORD exception_code )
{
    switch (exception_code)
    {
    case RPC_X_NULL_REF_POINTER:
        return ERROR_INVALID_ADDRESS;
    case RPC_X_ENUM_VALUE_OUT_OF_RANGE:
    case RPC_X_BYTE_COUNT_TOO_SMALL:
        return ERROR_INVALID_PARAMETER;
    case RPC_S_INVALID_BINDING:
    case RPC_X_SS_IN_NULL_CONTEXT:
        return ERROR_INVALID_HANDLE;
    default:
        return exception_code;
    }
}

static handle_t rpc_wstr_bind( RPC_WSTR str )
{
    WCHAR transport[] = SVCCTL_TRANSPORT;
    WCHAR endpoint[] = SVCCTL_ENDPOINT;
    RPC_WSTR binding_str;
    RPC_STATUS status;
    handle_t rpc_handle;

    status = RpcStringBindingComposeW( NULL, transport, str, endpoint, NULL, &binding_str );
    if (status != RPC_S_OK)
    {
        ERR("RpcStringBindingComposeW failed, error %ld\n", status);
        return NULL;
    }

    status = RpcBindingFromStringBindingW( binding_str, &rpc_handle );
    RpcStringFreeW( &binding_str );

    if (status != RPC_S_OK)
    {
        ERR("Couldn't connect to services.exe, error %ld\n", status);
        return NULL;
    }

    return rpc_handle;
}

static handle_t rpc_cstr_bind(RPC_CSTR str)
{
    RPC_CSTR transport = (RPC_CSTR)SVCCTL_TRANSPORTA;
    RPC_CSTR endpoint = (RPC_CSTR)SVCCTL_ENDPOINTA;
    RPC_CSTR binding_str;
    RPC_STATUS status;
    handle_t rpc_handle;

    status = RpcStringBindingComposeA( NULL, transport, str, endpoint, NULL, &binding_str );
    if (status != RPC_S_OK)
    {
        ERR("RpcStringBindingComposeA failed, error %ld\n", status);
        return NULL;
    }

    status = RpcBindingFromStringBindingA( binding_str, &rpc_handle );
    RpcStringFreeA( &binding_str );

    if (status != RPC_S_OK)
    {
        ERR("Couldn't connect to services.exe, error %ld\n", status);
        return NULL;
    }

    return rpc_handle;
}

handle_t __RPC_USER MACHINE_HANDLEA_bind( MACHINE_HANDLEA name )
{
    return rpc_cstr_bind( (RPC_CSTR)name );
}

void __RPC_USER MACHINE_HANDLEA_unbind( MACHINE_HANDLEA name, handle_t h )
{
    RpcBindingFree( &h );
}

handle_t __RPC_USER MACHINE_HANDLEW_bind( MACHINE_HANDLEW name )
{
    return rpc_wstr_bind( (RPC_WSTR)name );
}

void __RPC_USER MACHINE_HANDLEW_unbind( MACHINE_HANDLEW name, handle_t h )
{
    RpcBindingFree( &h );
}

handle_t __RPC_USER SVCCTL_HANDLEW_bind( SVCCTL_HANDLEW name )
{
    return rpc_wstr_bind( (RPC_WSTR)name );
}

void __RPC_USER SVCCTL_HANDLEW_unbind( SVCCTL_HANDLEW name, handle_t h )
{
    RpcBindingFree( &h );
}

static BOOL set_error( DWORD err )
{
    if (err) SetLastError( err );
    return !err;
}

/******************************************************************************
 *     OpenSCManagerA   (sechost.@)
 */
SC_HANDLE WINAPI DECLSPEC_HOTPATCH OpenSCManagerA( const char *machine, const char *database, DWORD access )
{
    WCHAR *machineW, *databaseW;
    SC_HANDLE ret;

    machineW = strdupAtoW( machine );
    databaseW = strdupAtoW( database );
    ret = OpenSCManagerW( machineW, databaseW, access );
    free( databaseW );
    free( machineW );
    return ret;
}

/******************************************************************************
 *     OpenSCManagerW   (sechost.@)
 */
SC_HANDLE WINAPI DECLSPEC_HOTPATCH OpenSCManagerW( const WCHAR *machine, const WCHAR *database, DWORD access )
{
    SC_RPC_HANDLE handle = NULL;
    DWORD err;

    TRACE( "%s %s %#lx\n", debugstr_w(machine), debugstr_w(database), access );

    __TRY
    {
        err = svcctl_OpenSCManagerW( machine, database, access, &handle );
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code( GetExceptionCode() );
    }
    __ENDTRY

    SetLastError( err );
    return handle;
}

/******************************************************************************
 *     OpenServiceA   (sechost.@)
 */
SC_HANDLE WINAPI DECLSPEC_HOTPATCH OpenServiceA( SC_HANDLE manager, const char *name, DWORD access )
{
    WCHAR *nameW;
    SC_HANDLE ret;

    TRACE( "%p %s %#lx\n", manager, debugstr_a(name), access );

    nameW = strdupAtoW( name );
    ret = OpenServiceW( manager, nameW, access );
    free( nameW );
    return ret;
}

/******************************************************************************
 *     OpenServiceW   (sechost.@)
 */
SC_HANDLE WINAPI DECLSPEC_HOTPATCH OpenServiceW( SC_HANDLE manager, const WCHAR *name, DWORD access )
{
    SC_RPC_HANDLE handle = NULL;
    DWORD err;

    TRACE( "%p %s %#lx\n", manager, debugstr_w(name), access );

    if (!manager)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return NULL;
    }

    __TRY
    {
        err = svcctl_OpenServiceW( manager, name, access, &handle );
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code( GetExceptionCode() );
    }
    __ENDTRY

    if (!err) return handle;
    SetLastError( err );
    return 0;
}

/******************************************************************************
 *     CreateServiceA   (sechost.@)
 */
SC_HANDLE WINAPI DECLSPEC_HOTPATCH CreateServiceA( SC_HANDLE manager, const char *name, const char *display_name,
                                                   DWORD access, DWORD service_type, DWORD start_type,
                                                   DWORD error_control, const char *path, const char *group,
                                                   DWORD *tag, const char *dependencies, const char *username,
                                                   const char *password )
{
    WCHAR *nameW, *display_nameW, *pathW, *groupW, *dependenciesW, *usernameW, *passwordW;
    SC_HANDLE handle;

    TRACE( "%p %s %s\n", manager, debugstr_a(name), debugstr_a(display_name) );

    nameW = strdupAtoW( name );
    display_nameW = strdupAtoW( display_name );
    pathW = strdupAtoW( path );
    groupW = strdupAtoW( group );
    dependenciesW = strdup_multi_AtoW( dependencies );
    usernameW = strdupAtoW( username );
    passwordW = strdupAtoW( password );

    handle = CreateServiceW( manager, nameW, display_nameW, access, service_type, start_type, error_control,
                             pathW, groupW, tag, dependenciesW, usernameW, passwordW );

    free( nameW );
    free( display_nameW );
    free( pathW );
    free( groupW );
    free( dependenciesW );
    free( usernameW );
    free( passwordW );

    return handle;
}

/******************************************************************************
 *     CreateServiceW   (sechost.@)
 */
SC_HANDLE WINAPI DECLSPEC_HOTPATCH CreateServiceW( SC_HANDLE manager, const WCHAR *name, const WCHAR *display_name,
                                                   DWORD access, DWORD service_type, DWORD start_type,
                                                   DWORD error_control, const WCHAR *path, const WCHAR *group,
                                                   DWORD *tag, const WCHAR *dependencies, const WCHAR *username,
                                                   const WCHAR *password )
{
    SC_RPC_HANDLE handle = NULL;
    DWORD err;
    SIZE_T password_size = 0;

    TRACE( "%p %s %s\n", manager, debugstr_w(name), debugstr_w(display_name) );

    if (!manager)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return 0;
    }

    if (password) password_size = (wcslen(password) + 1) * sizeof(WCHAR);

    __TRY
    {
        BOOL is_wow64;

        if (IsWow64Process(GetCurrentProcess(), &is_wow64) && is_wow64)
            err = svcctl_CreateServiceWOW64W( manager, name, display_name, access, service_type, start_type,
                                              error_control, path, group, tag, (const BYTE *)dependencies,
                                              multisz_size( dependencies ), username, (const BYTE *)password,
                                              password_size, &handle );
        else
            err = svcctl_CreateServiceW( manager, name, display_name, access, service_type, start_type,
                                         error_control, path, group, tag, (const BYTE *)dependencies,
                                         multisz_size( dependencies ), username, (const BYTE *)password,
                                         password_size, &handle );
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code( GetExceptionCode() );
    }
    __ENDTRY

    if (!err) return handle;
    SetLastError( err );
    return NULL;
}

/******************************************************************************
 *     DeleteService   (sechost.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH DeleteService( SC_HANDLE service )
{
    DWORD err;

    TRACE( "%p\n", service );

    __TRY
    {
        err = svcctl_DeleteService( service );
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code( GetExceptionCode() );
    }
    __ENDTRY

    return set_error( err );
}

/******************************************************************************
 *     CloseServiceHandle   (sechost.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CloseServiceHandle( SC_HANDLE handle )
{
    DWORD err;

    TRACE( "%p\n", handle );

    __TRY
    {
        err = svcctl_CloseServiceHandle( (SC_RPC_HANDLE *)&handle );
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code( GetExceptionCode() );
    }
    __ENDTRY

    return set_error( err );
}

/******************************************************************************
 *     ChangeServiceConfig2A   (sechost.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ChangeServiceConfig2A( SC_HANDLE service, DWORD level, void *info)
{
    BOOL r = FALSE;

    TRACE( "%p %ld %p\n", service, level, info );

    if (level == SERVICE_CONFIG_DESCRIPTION)
    {
        SERVICE_DESCRIPTIONA *sd = info;
        SERVICE_DESCRIPTIONW sdw;

        sdw.lpDescription = strdupAtoW( sd->lpDescription );

        r = ChangeServiceConfig2W( service, level, &sdw );

        free( sdw.lpDescription );
    }
    else if (level == SERVICE_CONFIG_FAILURE_ACTIONS)
    {
        SERVICE_FAILURE_ACTIONSA *fa = info;
        SERVICE_FAILURE_ACTIONSW faw;

        faw.dwResetPeriod = fa->dwResetPeriod;
        faw.lpRebootMsg = strdupAtoW( fa->lpRebootMsg );
        faw.lpCommand = strdupAtoW( fa->lpCommand );
        faw.cActions = fa->cActions;
        faw.lpsaActions = fa->lpsaActions;

        r = ChangeServiceConfig2W( service, level, &faw );

        free( faw.lpRebootMsg );
        free( faw.lpCommand );
    }
    else if (level == SERVICE_CONFIG_PRESHUTDOWN_INFO || level == SERVICE_CONFIG_DELAYED_AUTO_START_INFO)
    {
        r = ChangeServiceConfig2W( service, level, info );
    }
    else
        SetLastError( ERROR_INVALID_PARAMETER );

    return r;
}

/******************************************************************************
 *     ChangeServiceConfig2W   (sechost.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ChangeServiceConfig2W( SC_HANDLE service, DWORD level, void *info )
{
    SERVICE_RPC_REQUIRED_PRIVILEGES_INFO rpc_privinfo;
    DWORD err;

    __TRY
    {
        SC_RPC_CONFIG_INFOW rpc_info;

        rpc_info.dwInfoLevel = level;
        if (level == SERVICE_CONFIG_REQUIRED_PRIVILEGES_INFO)
        {
            SERVICE_REQUIRED_PRIVILEGES_INFOW *privinfo = info;

            rpc_privinfo.cbRequiredPrivileges = multisz_size( privinfo->pmszRequiredPrivileges );
            rpc_privinfo.pRequiredPrivileges = (BYTE *)privinfo->pmszRequiredPrivileges;
            rpc_info.privinfo = &rpc_privinfo;
        }
        else
            rpc_info.descr = info;
        err = svcctl_ChangeServiceConfig2W( service, rpc_info );
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code( GetExceptionCode() );
    }
    __ENDTRY

    return set_error( err );
}

/******************************************************************************
 *     ChangeServiceConfigA   (sechost.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ChangeServiceConfigA( SC_HANDLE service, DWORD service_type, DWORD start_type,
                                                    DWORD error_control, const char *path, const char *group,
                                                    DWORD *tag, const char *dependencies, const char *username,
                                                    const char *password, const char *display_name )
{
    WCHAR *pathW, *groupW, *dependenciesW, *usernameW, *passwordW, *display_nameW;
    BOOL r;

    TRACE( "%p %ld %ld %ld %s %s %p %p %s %s %s\n", service, service_type, start_type,
           error_control, debugstr_a(path), debugstr_a(group), tag, dependencies,
           debugstr_a(username), debugstr_a(password), debugstr_a(display_name) );

    pathW = strdupAtoW( path );
    groupW = strdupAtoW( group );
    dependenciesW = strdup_multi_AtoW( dependencies );
    usernameW = strdupAtoW( username );
    passwordW = strdupAtoW( password );
    display_nameW = strdupAtoW( display_name );

    r = ChangeServiceConfigW( service, service_type, start_type, error_control, pathW,
                              groupW, tag, dependenciesW, usernameW, passwordW, display_nameW );

    free( pathW );
    free( groupW );
    free( dependenciesW );
    free( usernameW );
    free( passwordW );
    free( display_nameW );

    return r;
}

/******************************************************************************
 *     ChangeServiceConfigW   (sechost.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ChangeServiceConfigW( SC_HANDLE service, DWORD service_type, DWORD start_type,
                                                    DWORD error_control, const WCHAR *path, const WCHAR *group,
                                                    DWORD *tag, const WCHAR *dependencies, const WCHAR *username,
                                                    const WCHAR *password, const WCHAR *display_name )
{
    DWORD password_size;
    DWORD err;

    TRACE( "%p %ld %ld %ld %s %s %p %p %s %s %s\n", service, service_type, start_type,
           error_control, debugstr_w(path), debugstr_w(group), tag, dependencies,
           debugstr_w(username), debugstr_w(password), debugstr_w(display_name) );

    password_size = password ? (wcslen(password) + 1) * sizeof(WCHAR) : 0;

    __TRY
    {
        err = svcctl_ChangeServiceConfigW( service, service_type, start_type, error_control, path, group, tag,
                                           (const BYTE *)dependencies, multisz_size(dependencies), username,
                                           (const BYTE *)password, password_size, display_name );
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code( GetExceptionCode() );
    }
    __ENDTRY

    return set_error( err );
}

/******************************************************************************
 *     QueryServiceConfigA   (sechost.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH QueryServiceConfigA( SC_HANDLE service, QUERY_SERVICE_CONFIGA *config,
                                                   DWORD size, DWORD *ret_size )
{
    DWORD n;
    char *p, *buffer;
    BOOL ret;
    QUERY_SERVICE_CONFIGW *configW;

    TRACE( "%p %p %ld %p\n", service, config, size, ret_size );

    if (!(buffer = malloc( 2 * size ))) return set_error( ERROR_NOT_ENOUGH_MEMORY );
    configW = (QUERY_SERVICE_CONFIGW *)buffer;
    ret = QueryServiceConfigW( service, configW, 2 * size, ret_size );
    if (!ret) goto done;

    config->dwServiceType      = configW->dwServiceType;
    config->dwStartType        = configW->dwStartType;
    config->dwErrorControl     = configW->dwErrorControl;
    config->lpBinaryPathName   = NULL;
    config->lpLoadOrderGroup   = NULL;
    config->dwTagId            = configW->dwTagId;
    config->lpDependencies     = NULL;
    config->lpServiceStartName = NULL;
    config->lpDisplayName      = NULL;

    p = (char *)(config + 1);
    n = size - sizeof(*config);
    ret = FALSE;

#define MAP_STR(str) \
    do { \
        if (configW->str) \
        { \
            DWORD sz = WideCharToMultiByte( CP_ACP, 0, configW->str, -1, p, n, NULL, NULL ); \
            if (!sz) goto done; \
            config->str = p; \
            p += sz; \
            n -= sz; \
        } \
    } while (0)

    MAP_STR( lpBinaryPathName );
    MAP_STR( lpLoadOrderGroup );
    MAP_STR( lpDependencies );
    MAP_STR( lpServiceStartName );
    MAP_STR( lpDisplayName );
#undef MAP_STR

    *ret_size = p - (char *)config;
    ret = TRUE;

done:
    free( buffer );
    return ret;
}

static DWORD move_string_to_buffer(BYTE **buf, WCHAR **string_ptr)
{
    DWORD cb;

    if (!*string_ptr)
    {
        cb = sizeof(WCHAR);
        memset(*buf, 0, cb);
    }
    else
    {
        cb = (wcslen( *string_ptr ) + 1) * sizeof(WCHAR);
        memcpy(*buf, *string_ptr, cb);
        MIDL_user_free( *string_ptr );
    }

    *string_ptr = (WCHAR *)*buf;
    *buf += cb;

    return cb;
}

static DWORD size_string( const WCHAR *string )
{
    return (string ? (wcslen( string ) + 1) * sizeof(WCHAR) : sizeof(WCHAR));
}

/******************************************************************************
 *     QueryServiceConfigW   (sechost.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH QueryServiceConfigW( SC_HANDLE service, QUERY_SERVICE_CONFIGW *ret_config,
                                                   DWORD size, DWORD *ret_size )
{
    QUERY_SERVICE_CONFIGW config;
    DWORD total;
    DWORD err;
    BYTE *bufpos;

    TRACE( "%p %p %ld %p\n", service, ret_config, size, ret_size );

    memset(&config, 0, sizeof(config));

    __TRY
    {
        err = svcctl_QueryServiceConfigW( service, &config, size, ret_size );
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code( GetExceptionCode() );
    }
    __ENDTRY

    if (err) return set_error( err );

    /* calculate the size required first */
    total = sizeof(QUERY_SERVICE_CONFIGW);
    total += size_string( config.lpBinaryPathName );
    total += size_string( config.lpLoadOrderGroup );
    total += size_string( config.lpDependencies );
    total += size_string( config.lpServiceStartName );
    total += size_string( config.lpDisplayName );

    *ret_size = total;

    /* if there's not enough memory, return an error */
    if (size < total)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        MIDL_user_free( config.lpBinaryPathName );
        MIDL_user_free( config.lpLoadOrderGroup );
        MIDL_user_free( config.lpDependencies );
        MIDL_user_free( config.lpServiceStartName );
        MIDL_user_free( config.lpDisplayName );
        return FALSE;
    }

    *ret_config = config;
    bufpos = ((BYTE *)ret_config) + sizeof(QUERY_SERVICE_CONFIGW);
    move_string_to_buffer( &bufpos, &ret_config->lpBinaryPathName );
    move_string_to_buffer( &bufpos, &ret_config->lpLoadOrderGroup );
    move_string_to_buffer( &bufpos, &ret_config->lpDependencies );
    move_string_to_buffer( &bufpos, &ret_config->lpServiceStartName );
    move_string_to_buffer( &bufpos, &ret_config->lpDisplayName );

    TRACE( "Image path           = %s\n", debugstr_w( ret_config->lpBinaryPathName ) );
    TRACE( "Group                = %s\n", debugstr_w( ret_config->lpLoadOrderGroup ) );
    TRACE( "Dependencies         = %s\n", debugstr_w( ret_config->lpDependencies ) );
    TRACE( "Service account name = %s\n", debugstr_w( ret_config->lpServiceStartName ) );
    TRACE( "Display name         = %s\n", debugstr_w( ret_config->lpDisplayName ) );

    return TRUE;
}

/******************************************************************************
 *     QueryServiceConfig2A   (sechost.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH QueryServiceConfig2A( SC_HANDLE service, DWORD level, BYTE *buffer,
                                                    DWORD size, DWORD *ret_size )
{
    BYTE *bufferW = NULL;

    TRACE( "%p %lu %p %lu %p\n", service, level, buffer, size, ret_size );

    if (buffer && size)
        bufferW = malloc( size );

    if (!QueryServiceConfig2W( service, level, bufferW, size, ret_size ))
    {
        free( bufferW );
        return FALSE;
    }

    switch (level)
    {
        case SERVICE_CONFIG_DESCRIPTION:
            if (buffer && bufferW) {
                SERVICE_DESCRIPTIONA *configA = (SERVICE_DESCRIPTIONA *)buffer;
                SERVICE_DESCRIPTIONW *configW = (SERVICE_DESCRIPTIONW *)bufferW;
                if (configW->lpDescription && size > sizeof(SERVICE_DESCRIPTIONA))
                {
                    configA->lpDescription = (char *)(configA + 1);
                    WideCharToMultiByte( CP_ACP, 0, configW->lpDescription, -1, configA->lpDescription,
                                         size - sizeof(SERVICE_DESCRIPTIONA), NULL, NULL );
                }
                else configA->lpDescription = NULL;
            }
            break;
        case SERVICE_CONFIG_PRESHUTDOWN_INFO:
        case SERVICE_CONFIG_DELAYED_AUTO_START_INFO:
            if (buffer && bufferW && *ret_size <= size)
                memcpy(buffer, bufferW, *ret_size);
            break;
        default:
            FIXME("conversion W->A not implemented for level %ld\n", level);
            free( bufferW );
            return FALSE;
    }

    free( bufferW );
    return TRUE;
}

/******************************************************************************
 *     QueryServiceConfig2W   (sechost.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH QueryServiceConfig2W( SC_HANDLE service, DWORD level, BYTE *buffer,
                                                    DWORD size, DWORD *ret_size )
{
    BYTE *bufptr;
    DWORD err;

    TRACE( "%p %lu %p %lu %p\n", service, level, buffer, size, ret_size );

    if (!buffer && size)
    {
        SetLastError(ERROR_INVALID_ADDRESS);
        return FALSE;
    }

    switch (level)
    {
    case SERVICE_CONFIG_DESCRIPTION:
        if (!(bufptr = malloc( size )))
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return FALSE;
        }
        break;

    case SERVICE_CONFIG_PRESHUTDOWN_INFO:
    case SERVICE_CONFIG_DELAYED_AUTO_START_INFO:
        bufptr = buffer;
        break;

    default:
        FIXME("Level %ld not implemented\n", level);
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (!ret_size)
    {
        if (level == SERVICE_CONFIG_DESCRIPTION) free( bufptr );
        SetLastError(ERROR_INVALID_ADDRESS);
        return FALSE;
    }

    __TRY
    {
        err = svcctl_QueryServiceConfig2W( service, level, bufptr, size, ret_size );
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code( GetExceptionCode() );
    }
    __ENDTRY

    switch (level)
    {
    case SERVICE_CONFIG_DESCRIPTION:
    {
        SERVICE_DESCRIPTIONW *desc = (SERVICE_DESCRIPTIONW *)buffer;
        struct service_description *s = (struct service_description *)bufptr;

        if (err != ERROR_SUCCESS && err != ERROR_INSUFFICIENT_BUFFER)
        {
            free( bufptr );
            SetLastError( err );
            return FALSE;
        }

        /* adjust for potentially larger SERVICE_DESCRIPTIONW structure */
        if (*ret_size == sizeof(*s))
            *ret_size = sizeof(*desc);
        else
            *ret_size = *ret_size - FIELD_OFFSET(struct service_description, description) + sizeof(*desc);

        if (size < *ret_size)
        {
            free( bufptr );
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }
        if (desc)
        {
            if (!s->size) desc->lpDescription = NULL;
            else
            {
                desc->lpDescription = (WCHAR *)(desc + 1);
                memcpy( desc->lpDescription, s->description, s->size );
            }
        }
        free( bufptr );
        break;
    }
    case SERVICE_CONFIG_PRESHUTDOWN_INFO:
    case SERVICE_CONFIG_DELAYED_AUTO_START_INFO:
        return set_error( err );

    default:
        break;
    }

    return TRUE;
}

/******************************************************************************
 *     GetServiceDisplayNameW   (sechost.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetServiceDisplayNameW( SC_HANDLE manager, const WCHAR *service,
                                                      WCHAR *display_name, DWORD *len )
{
    DWORD err;
    DWORD size;
    WCHAR buffer[2];

    TRACE( "%p %s %p %p\n", manager, debugstr_w(service), display_name, len );

    if (!manager)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    /* provide a buffer if the caller didn't */
    if (!display_name || *len < sizeof(WCHAR))
    {
        display_name = buffer;
        /* A size of 1 would be enough, but tests show that Windows returns 2,
         * probably because of a WCHAR/bytes mismatch in their code. */
        *len = 2;
    }

    /* RPC call takes size excluding nul-terminator, whereas *len
     * includes the nul-terminator on input. */
    size = *len - 1;

    __TRY
    {
        err = svcctl_GetServiceDisplayNameW( manager, service, display_name, &size );
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code( GetExceptionCode() );
    }
    __ENDTRY

    /* The value of *len excludes nul-terminator on output. */
    if (err == ERROR_SUCCESS || err == ERROR_INSUFFICIENT_BUFFER)
        *len = size;
    return set_error( err );
}

/******************************************************************************
 *     GetServiceKeyNameW   (sechost.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetServiceKeyNameW( SC_HANDLE manager, const WCHAR *display_name,
                                                  WCHAR *key_name, DWORD *len )
{
    DWORD err;
    WCHAR buffer[2];
    DWORD size;

    TRACE( "%p %s %p %p\n", manager, debugstr_w(display_name), key_name, len );

    if (!manager)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    /* provide a buffer if the caller didn't */
    if (!key_name || *len < 2)
    {
        key_name = buffer;
        /* A size of 1 would be enough, but tests show that Windows returns 2,
         * probably because of a WCHAR/bytes mismatch in their code.
         */
        *len = 2;
    }

    /* RPC call takes size excluding nul-terminator, whereas *len
     * includes the nul-terminator on input. */
    size = *len - 1;

    __TRY
    {
        err = svcctl_GetServiceKeyNameW( manager, display_name, key_name, &size );
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code( GetExceptionCode() );
    }
    __ENDTRY

    /* The value of *lpcchBuffer excludes nul-terminator on output. */
    if (err == ERROR_SUCCESS || err == ERROR_INSUFFICIENT_BUFFER)
        *len = size;
    return set_error( err );
}

/******************************************************************************
 *     StartServiceA   (sechost.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH StartServiceA( SC_HANDLE service, DWORD argc, const char **argv )
{
    WCHAR **argvW = NULL;
    DWORD i;
    BOOL r;

    if (argc)
        argvW = malloc( argc * sizeof(*argvW) );

    for (i = 0; i < argc; i++)
        argvW[i] = strdupAtoW( argv[i] );

    r = StartServiceW( service, argc, (const WCHAR **)argvW );

    for (i = 0; i < argc; i++)
        free( argvW[i] );
    free( argvW );
    return r;
}


/******************************************************************************
 *     StartServiceW   (sechost.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH StartServiceW( SC_HANDLE service, DWORD argc, const WCHAR **argv )
{
    DWORD err;

    TRACE( "%p %lu %p\n", service, argc, argv );

    __TRY
    {
        err = svcctl_StartServiceW( service, argc, argv );
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code( GetExceptionCode() );
    }
    __ENDTRY

    return set_error( err );
}

/******************************************************************************
 *     ControlService   (sechost.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ControlService( SC_HANDLE service, DWORD control, SERVICE_STATUS *status )
{
    DWORD err;

    TRACE( "%p %ld %p\n", service, control, status );

    __TRY
    {
        err = svcctl_ControlService( service, control, status );
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code( GetExceptionCode() );
    }
    __ENDTRY

    return set_error( err );
}

/******************************************************************************
 *     QueryServiceStatus   (sechost.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH QueryServiceStatus( SC_HANDLE service, SERVICE_STATUS *status )
{
    SERVICE_STATUS_PROCESS process_status;
    BOOL ret;
    DWORD size;

    TRACE( "%p %p\n", service, status );

    if (!service) return set_error( ERROR_INVALID_HANDLE );
    if (!status) return set_error( ERROR_INVALID_ADDRESS );

    ret = QueryServiceStatusEx( service, SC_STATUS_PROCESS_INFO, (BYTE *)&process_status,
                                sizeof(SERVICE_STATUS_PROCESS), &size );
    if (ret) memcpy(status, &process_status, sizeof(SERVICE_STATUS) );
    return ret;
}

/******************************************************************************
 *     QueryServiceStatusEx   (sechost.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH QueryServiceStatusEx( SC_HANDLE service, SC_STATUS_TYPE level,
                                                    BYTE *buffer, DWORD size, DWORD *ret_size )
{
    DWORD err;

    TRACE( "%p %d %p %ld %p\n", service, level, buffer, size, ret_size );

    if (level != SC_STATUS_PROCESS_INFO) return set_error( ERROR_INVALID_LEVEL );

    if (size < sizeof(SERVICE_STATUS_PROCESS))
    {
        *ret_size = sizeof(SERVICE_STATUS_PROCESS);
        return set_error( ERROR_INSUFFICIENT_BUFFER );
    }

    __TRY
    {
        err = svcctl_QueryServiceStatusEx( service, level, buffer, size, ret_size );
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code( GetExceptionCode() );
    }
    __ENDTRY

    return set_error( err );
}

/******************************************************************************
 *     EnumServicesStatusExW   (sechost.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumServicesStatusExW( SC_HANDLE manager, SC_ENUM_TYPE level, DWORD type, DWORD state,
                                                     BYTE *buffer, DWORD size, DWORD *needed, DWORD *returned,
                                                     DWORD *resume_handle, const WCHAR *group )
{
    DWORD err, i, offset, buflen, count, total_size = 0;
    ENUM_SERVICE_STATUS_PROCESSW *services = (ENUM_SERVICE_STATUS_PROCESSW *)buffer;
    struct enum_service_status_process *entry;
    const WCHAR *str;
    BYTE *buf;

    TRACE( "%p %u 0x%lx 0x%lx %p %lu %p %p %p %s\n", manager, level, type, state, buffer,
           size, needed, returned, resume_handle, debugstr_w(group) );

    if (level != SC_ENUM_PROCESS_INFO) return set_error( ERROR_INVALID_LEVEL );
    if (!manager) return set_error( ERROR_INVALID_HANDLE );
    if (!needed || !returned) return set_error( ERROR_INVALID_ADDRESS );

    /* make sure we pass a valid pointer */
    buflen = max( size, sizeof(*services) );
    if (!(buf = malloc( buflen ))) return set_error( ERROR_NOT_ENOUGH_MEMORY );

    __TRY
    {
        err = svcctl_EnumServicesStatusExW( manager, SC_ENUM_PROCESS_INFO, type, state, buf, buflen, needed,
                                            &count, resume_handle, group );
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code( GetExceptionCode() );
    }
    __ENDTRY

    *returned = 0;
    if (err != ERROR_SUCCESS)
    {
        /* double the needed size to fit the potentially larger ENUM_SERVICE_STATUS_PROCESSW */
        if (err == ERROR_MORE_DATA) *needed *= 2;
        free( buf );
        SetLastError( err );
        return FALSE;
    }

    entry = (struct enum_service_status_process *)buf;
    for (i = 0; i < count; i++)
    {
        total_size += sizeof(*services);
        if (entry->service_name)
        {
            str = (const WCHAR *)(buf + entry->service_name);
            total_size += (wcslen( str ) + 1) * sizeof(WCHAR);
        }
        if (entry->display_name)
        {
            str = (const WCHAR *)(buf + entry->display_name);
            total_size += (wcslen( str ) + 1) * sizeof(WCHAR);
        }
        entry++;
    }

    if (total_size > size)
    {
        free( buf );
        *needed = total_size;
        SetLastError( ERROR_MORE_DATA );
        return FALSE;
    }

    offset = count * sizeof(*services);
    entry = (struct enum_service_status_process *)buf;
    for (i = 0; i < count; i++)
    {
        DWORD str_size;
        str = (const WCHAR *)(buf + entry->service_name);
        str_size = (wcslen( str ) + 1) * sizeof(WCHAR);
        services[i].lpServiceName = (WCHAR *)((char *)services + offset);
        memcpy( services[i].lpServiceName, str, str_size );
        offset += str_size;

        if (!entry->display_name) services[i].lpDisplayName = NULL;
        else
        {
            str = (const WCHAR *)(buf + entry->display_name);
            str_size = (wcslen( str ) + 1) * sizeof(WCHAR);
            services[i].lpDisplayName = (WCHAR *)((char *)services + offset);
            memcpy( services[i].lpDisplayName, str, str_size );
            offset += str_size;
        }
        services[i].ServiceStatusProcess = entry->service_status_process;
        entry++;
    }

    free( buf );
    *needed = 0;
    *returned = count;
    return TRUE;
}

/******************************************************************************
 *     EnumDependentServicesW   (sechost.@)
 */
BOOL WINAPI EnumDependentServicesW( SC_HANDLE hService, DWORD dwServiceState,
                                    LPENUM_SERVICE_STATUSW lpServices, DWORD cbBufSize,
                                    LPDWORD pcbBytesNeeded, LPDWORD lpServicesReturned )
{
    FIXME("%p 0x%08lx %p 0x%08lx %p %p - stub\n", hService, dwServiceState,
          lpServices, cbBufSize, pcbBytesNeeded, lpServicesReturned);

    *lpServicesReturned = 0;
    return TRUE;
}

/******************************************************************************
 *     QueryServiceObjectSecurity   (sechost.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH QueryServiceObjectSecurity( SC_HANDLE service, SECURITY_INFORMATION type,
                                                          PSECURITY_DESCRIPTOR ret_descriptor, DWORD size, DWORD *ret_size )
{
    SECURITY_DESCRIPTOR descriptor;
    NTSTATUS status;
    ACL acl;

    FIXME( "%p %ld %p %lu %p - semi-stub\n", service, type, ret_descriptor, size, ret_size );

    if (type != DACL_SECURITY_INFORMATION)
        FIXME("information %ld not supported\n", type);

    InitializeSecurityDescriptor( &descriptor, SECURITY_DESCRIPTOR_REVISION );

    InitializeAcl( &acl, sizeof(ACL), ACL_REVISION );
    SetSecurityDescriptorDacl( &descriptor, TRUE, &acl, TRUE );

    status = RtlMakeSelfRelativeSD( &descriptor, ret_descriptor, &size );
    *ret_size = size;

    return set_error( RtlNtStatusToDosError( status ) );
}

/******************************************************************************
 *     SetServiceObjectSecurity   (sechost.@)
 */
BOOL WINAPI SetServiceObjectSecurity(SC_HANDLE hService,
       SECURITY_INFORMATION dwSecurityInformation,
       PSECURITY_DESCRIPTOR lpSecurityDescriptor)
{
    FIXME("%p %ld %p\n", hService, dwSecurityInformation, lpSecurityDescriptor);
    return TRUE;
}

static DWORD WINAPI notify_thread(void *user)
{
    DWORD err;
    struct notify_data *data = user;
    SC_RPC_NOTIFY_PARAMS_LIST *list = NULL;
    SERVICE_NOTIFY_STATUS_CHANGE_PARAMS_2 *cparams;
    BOOL dummy;

    SetThreadDescription(GetCurrentThread(), L"wine_sechost_notify_service_status");

    __TRY
    {
        /* GetNotifyResults blocks until there is an event */
        err = svcctl_GetNotifyResults(data->notify_handle, &list);
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code(GetExceptionCode());
    }
    __ENDTRY

    EnterCriticalSection( &service_cs );

    list_remove(&data->entry);

    LeaveCriticalSection( &service_cs );

    if (err == ERROR_SUCCESS && list)
    {
        cparams = list->NotifyParamsArray[0].params;

        data->notify_buffer->dwNotificationStatus = cparams->dwNotificationStatus;
        memcpy(&data->notify_buffer->ServiceStatus, &cparams->ServiceStatus,
                sizeof(SERVICE_STATUS_PROCESS));
        data->notify_buffer->dwNotificationTriggered = cparams->dwNotificationTriggered;
        data->notify_buffer->pszServiceNames = NULL;

        QueueUserAPC((PAPCFUNC)data->notify_buffer->pfnNotifyCallback,
                data->calling_thread, (ULONG_PTR)data->notify_buffer);

        HeapFree(GetProcessHeap(), 0, list);
    }
    else
        WARN("GetNotifyResults server call failed: %lu\n", err);


    __TRY
    {
        err = svcctl_CloseNotifyHandle(&data->notify_handle, &dummy);
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code(GetExceptionCode());
    }
    __ENDTRY

    if (err != ERROR_SUCCESS)
        WARN("CloseNotifyHandle server call failed: %lu\n", err);

    CloseHandle(data->calling_thread);
    HeapFree(GetProcessHeap(), 0, data);

    return 0;
}

/******************************************************************************
 *     NotifyServiceStatusChangeW   (sechost.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH NotifyServiceStatusChangeW( SC_HANDLE service, DWORD mask,
                                                           SERVICE_NOTIFYW *notify_buffer )
{
    DWORD err;
    BOOL b_dummy = FALSE;
    GUID g_dummy = {0};
    struct notify_data *data;

    TRACE( "%p 0x%lx %p\n", service, mask, notify_buffer );

    if (!(data = calloc( 1, sizeof(*data) )))
        return ERROR_NOT_ENOUGH_MEMORY;

    data->service = service;
    data->notify_buffer = notify_buffer;
    if (!DuplicateHandle( GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(),
                          &data->calling_thread, 0, FALSE, DUPLICATE_SAME_ACCESS ))
    {
        ERR("DuplicateHandle failed: %lu\n", GetLastError());
        free( data );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    data->params.dwInfoLevel = 2;
    data->params.params = &data->cparams;

    data->cparams.dwNotifyMask = mask;

    EnterCriticalSection( &service_cs );

    __TRY
    {
        err = svcctl_NotifyServiceStatusChange( service, data->params, &g_dummy,
                                                &g_dummy, &b_dummy, &data->notify_handle );
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code( GetExceptionCode() );
    }
    __ENDTRY

    if (err != ERROR_SUCCESS)
    {
        WARN("NotifyServiceStatusChange server call failed: %lu\n", err);
        LeaveCriticalSection( &service_cs );
        CloseHandle( data->calling_thread );
        CloseHandle( data->ready_evt );
        free( data );
        return err;
    }

    CloseHandle( CreateThread( NULL, 0, &notify_thread, data, 0, NULL ) );

    list_add_tail( &notify_list, &data->entry );

    LeaveCriticalSection( &service_cs );

    return ERROR_SUCCESS;
}

/* thunk for calling the RegisterServiceCtrlHandler handler function */
static DWORD WINAPI ctrl_handler_thunk( DWORD control, DWORD type, void *data, void *context )
{
    LPHANDLER_FUNCTION func = context;

    func( control );
    return ERROR_SUCCESS;
}

/******************************************************************************
 *     RegisterServiceCtrlHandlerA   (sechost.@)
 */
SERVICE_STATUS_HANDLE WINAPI DECLSPEC_HOTPATCH RegisterServiceCtrlHandlerA(
        const char *name, LPHANDLER_FUNCTION handler )
{
    return RegisterServiceCtrlHandlerExA( name, ctrl_handler_thunk, handler );
}

/******************************************************************************
 *     RegisterServiceCtrlHandlerW   (sechost.@)
 */
SERVICE_STATUS_HANDLE WINAPI DECLSPEC_HOTPATCH RegisterServiceCtrlHandlerW(
        const WCHAR *name, LPHANDLER_FUNCTION handler )
{
    return RegisterServiceCtrlHandlerExW( name, ctrl_handler_thunk, handler );
}

/******************************************************************************
 *     RegisterServiceCtrlHandlerExA   (sechost.@)
 */
SERVICE_STATUS_HANDLE WINAPI DECLSPEC_HOTPATCH RegisterServiceCtrlHandlerExA(
        const char *name, LPHANDLER_FUNCTION_EX handler, void *context )
{
    WCHAR *nameW;
    SERVICE_STATUS_HANDLE ret;

    nameW = strdupAtoW( name );
    ret = RegisterServiceCtrlHandlerExW( nameW, handler, context );
    free( nameW );
    return ret;
}

static struct service_data *find_service_by_name( const WCHAR *name )
{
    unsigned int i;

    if (nb_services == 1)  /* only one service (FIXME: should depend on OWN_PROCESS etc.) */
        return services[0];
    for (i = 0; i < nb_services; i++)
        if (!wcsicmp( name, services[i]->name )) return services[i];
    return NULL;
}

/******************************************************************************
 *     RegisterServiceCtrlHandlerExW   (sechost.@)
 */
SERVICE_STATUS_HANDLE WINAPI DECLSPEC_HOTPATCH RegisterServiceCtrlHandlerExW(
        const WCHAR *name, LPHANDLER_FUNCTION_EX handler, void *context )
{
    struct service_data *service;
    SC_HANDLE handle = 0;

    TRACE( "%s %p %p\n", debugstr_w(name), handler, context );

    EnterCriticalSection( &service_cs );
    if ((service = find_service_by_name( name )))
    {
        service->handler = handler;
        service->context = context;
        handle = service->handle;
    }
    LeaveCriticalSection( &service_cs );

    if (!handle) SetLastError( ERROR_SERVICE_DOES_NOT_EXIST );
    return (SERVICE_STATUS_HANDLE)handle;
}

/******************************************************************************
 *     SetServiceStatus   (sechost.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetServiceStatus( SERVICE_STATUS_HANDLE service, SERVICE_STATUS *status )
{
    DWORD err;

    TRACE( "%p %#lx %#lx %#lx %#lx %#lx %#lx %#lx\n", service, status->dwServiceType,
           status->dwCurrentState, status->dwControlsAccepted, status->dwWin32ExitCode,
           status->dwServiceSpecificExitCode, status->dwCheckPoint, status->dwWaitHint );

    __TRY
    {
        err = svcctl_SetServiceStatus( service, status );
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code( GetExceptionCode() );
    }
    __ENDTRY

    if (!set_error( err ))
        return FALSE;

    if (status->dwCurrentState == SERVICE_STOPPED)
    {
        unsigned int i, count = 0;
        EnterCriticalSection( &service_cs );
        for (i = 0; i < nb_services; i++)
        {
            if (services[i]->handle == (SC_HANDLE)service) continue;
            if (services[i]->thread) count++;
        }
        if (!count)
        {
            stop_service = TRUE;
            SetEvent( service_event );  /* notify the main loop */
        }
        LeaveCriticalSection( &service_cs );
    }

    return TRUE;
}

static WCHAR *service_get_pipe_name(void)
{
    static const WCHAR format[] = L"\\\\.\\pipe\\net\\NtControlPipe%u";
    WCHAR *name;
    DWORD len;
    HKEY service_current_key;
    DWORD service_current;
    LONG ret;
    DWORD type;

    ret = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                         L"SYSTEM\\CurrentControlSet\\Control\\ServiceCurrent",
                         0, KEY_QUERY_VALUE, &service_current_key );
    if (ret != ERROR_SUCCESS)
        return NULL;

    len = sizeof(service_current);
    ret = RegQueryValueExW( service_current_key, NULL, NULL, &type,
                            (BYTE *)&service_current, &len );
    RegCloseKey(service_current_key);
    if (ret != ERROR_SUCCESS || type != REG_DWORD)
        return NULL;

    len = ARRAY_SIZE(format) + 10 /* strlenW("4294967295") */;
    name = malloc(len * sizeof(WCHAR));
    if (!name)
        return NULL;

    swprintf( name, len, format, service_current );
    return name;
}

static HANDLE service_open_pipe(void)
{
    WCHAR *pipe_name = service_get_pipe_name();
    HANDLE handle = INVALID_HANDLE_VALUE;

    do
    {
        handle = CreateFileW( pipe_name, GENERIC_READ|GENERIC_WRITE,
                              0, NULL, OPEN_ALWAYS, 0, NULL );
        if (handle != INVALID_HANDLE_VALUE)
            break;
        if (GetLastError() != ERROR_PIPE_BUSY)
            break;
    } while (WaitNamedPipeW( pipe_name, NMPWAIT_USE_DEFAULT_WAIT ));
    free(pipe_name);

    return handle;
}

static DWORD WINAPI service_thread( void *arg )
{
    struct service_data *info = arg;
    WCHAR *str = info->args;
    DWORD argc = 0, len = 0;

    TRACE("%p\n", arg);
    SetThreadDescription(GetCurrentThread(), L"wine_sechost_service");

    while (str[len])
    {
        len += wcslen( &str[len] ) + 1;
        argc++;
    }
    len++;

    if (info->unicode)
    {
        WCHAR **argv, *p;

        argv = malloc( (argc + 1) * sizeof(*argv) );
        for (argc = 0, p = str; *p; p += wcslen( p ) + 1)
            argv[argc++] = p;
        argv[argc] = NULL;

        info->proc.w( argc, argv );
        free( argv );
    }
    else
    {
        char *strA, **argv, *p;
        DWORD lenA;

        lenA = WideCharToMultiByte( CP_ACP,0, str, len, NULL, 0, NULL, NULL );
        strA = malloc(lenA);
        WideCharToMultiByte(CP_ACP,0, str, len, strA, lenA, NULL, NULL);

        argv = malloc( (argc + 1) * sizeof(*argv) );
        for (argc = 0, p = strA; *p; p += strlen( p ) + 1)
            argv[argc++] = p;
        argv[argc] = NULL;

        info->proc.a( argc, argv );
        free( argv );
        free( strA );
    }
    return 0;
}

static DWORD service_handle_start( struct service_data *service, const void *data, DWORD data_size )
{
    DWORD count = data_size / sizeof(WCHAR);

    if (service->thread)
    {
        WARN("service is not stopped\n");
        return ERROR_SERVICE_ALREADY_RUNNING;
    }

    free( service->args );
    service->args = malloc( (count + 2) * sizeof(WCHAR) );
    if (count) memcpy( service->args, data, count * sizeof(WCHAR) );
    service->args[count++] = 0;
    service->args[count++] = 0;

    service->thread = CreateThread( NULL, 0, service_thread,
                                    service, 0, NULL );
    SetEvent( service_event );  /* notify the main loop */
    return 0;
}

static DWORD service_handle_control( struct service_data *service, DWORD control, const void *data, DWORD data_size )
{
    DWORD ret = ERROR_INVALID_SERVICE_CONTROL;

    TRACE( "%s control %lu data %p data_size %lu\n", debugstr_w(service->name), control, data, data_size );

    if (control == SERVICE_CONTROL_START)
        ret = service_handle_start( service, data, data_size );
    else if (service->handler)
        ret = service->handler( control, 0, (void *)data, service->context );
    return ret;
}

static DWORD WINAPI service_control_dispatcher( void *arg )
{
    struct dispatcher_data *disp = arg;

    /* dispatcher loop */
    while (1)
    {
        struct service_data *service;
        service_start_info info;
        BYTE *data = NULL;
        WCHAR *name;
        BOOL r;
        DWORD data_size = 0, count, result;

        r = ReadFile( disp->pipe, &info, FIELD_OFFSET(service_start_info,data), &count, NULL );
        if (!r)
        {
            if (GetLastError() != ERROR_BROKEN_PIPE)
                ERR( "pipe read failed error %lu\n", GetLastError() );
            break;
        }
        if (count != FIELD_OFFSET(service_start_info,data))
        {
            ERR( "partial pipe read %lu\n", count );
            break;
        }
        if (count < info.total_size)
        {
            data_size = info.total_size - FIELD_OFFSET(service_start_info,data);
            data = malloc( data_size );
            r = ReadFile( disp->pipe, data, data_size, &count, NULL );
            if (!r)
            {
                if (GetLastError() != ERROR_BROKEN_PIPE)
                    ERR( "pipe read failed error %lu\n", GetLastError() );
                free( data );
                break;
            }
            if (count != data_size)
            {
                ERR( "partial pipe read %lu/%lu\n", count, data_size );
                free( data );
                break;
            }
        }

        EnterCriticalSection( &service_cs );

        /* validate service name */
        name = (WCHAR *)data;
        if (!info.name_size || data_size < info.name_size * sizeof(WCHAR) || name[info.name_size - 1])
        {
            ERR( "got request without valid service name\n" );
            result = ERROR_INVALID_PARAMETER;
            goto done;
        }

        if (info.magic != SERVICE_PROTOCOL_MAGIC)
        {
            ERR( "received invalid request for service %s\n", debugstr_w(name) );
            result = ERROR_INVALID_PARAMETER;
            goto done;
        }

        /* find the service */
        if (!(service = find_service_by_name( name )))
        {
            FIXME( "got request for unknown service %s\n", debugstr_w(name) );
            result = ERROR_INVALID_PARAMETER;
            goto done;
        }

        if (!service->handle)
        {
            if (!(service->handle = OpenServiceW( disp->manager, name, SERVICE_SET_STATUS )) ||
                !(service->full_access_handle = OpenServiceW( disp->manager, name,
                        GENERIC_READ|GENERIC_WRITE )))
                FIXME( "failed to open service %s\n", debugstr_w(name) );
        }

        data_size -= info.name_size * sizeof(WCHAR);
        result = service_handle_control(service, info.control, data_size ?
                                        &data[info.name_size * sizeof(WCHAR)] : NULL, data_size);

    done:
        LeaveCriticalSection( &service_cs );
        WriteFile( disp->pipe, &result, sizeof(result), &count, NULL );
        free( data );
    }

    CloseHandle( disp->pipe );
    CloseServiceHandle( disp->manager );
    free( disp );
    return 1;
}

/* wait for services which accept this type of message to become STOPPED */
static void handle_shutdown_msg(DWORD msg, DWORD accept)
{
    SERVICE_STATUS st;
    SERVICE_PRESHUTDOWN_INFO spi;
    DWORD i, n = 0, sz, timeout = 2000;
    ULONGLONG stop_time;
    BOOL res, done = TRUE;
    SC_HANDLE *wait_handles = calloc( nb_services, sizeof(SC_HANDLE) );

    EnterCriticalSection( &service_cs );
    for (i = 0; i < nb_services; i++)
    {
        res = QueryServiceStatus( services[i]->full_access_handle, &st );
        if (!res || st.dwCurrentState == SERVICE_STOPPED || !(st.dwControlsAccepted & accept))
            continue;

        done = FALSE;

        if (accept == SERVICE_ACCEPT_PRESHUTDOWN)
        {
            res = QueryServiceConfig2W( services[i]->full_access_handle, SERVICE_CONFIG_PRESHUTDOWN_INFO,
                                        (BYTE *)&spi, sizeof(spi), &sz );
            if (res)
            {
                FIXME( "service should be able to delay shutdown\n" );
                timeout = max( spi.dwPreshutdownTimeout, timeout );
            }
        }

        service_handle_control( services[i], msg, NULL, 0 );
        wait_handles[n++] = services[i]->full_access_handle;
    }
    LeaveCriticalSection( &service_cs );

    /* FIXME: these timeouts should be more generous, but we can't currently delay prefix shutdown */
    timeout = min( timeout, 3000 );
    stop_time = GetTickCount64() + timeout;

    while (!done && GetTickCount64() < stop_time)
    {
        done = TRUE;
        for (i = 0; i < n; i++)
        {
            res = QueryServiceStatus( wait_handles[i], &st );
            if (!res || st.dwCurrentState == SERVICE_STOPPED)
                continue;

            done = FALSE;
            Sleep( 100 );
            break;
        }
    }

    free( wait_handles );
}

static BOOL service_run_main_thread(void)
{
    DWORD i, n, ret;
    HANDLE wait_handles[MAXIMUM_WAIT_OBJECTS];
    UINT wait_services[MAXIMUM_WAIT_OBJECTS];
    struct dispatcher_data *disp = malloc( sizeof(*disp) );

    disp->manager = OpenSCManagerW( NULL, NULL, SC_MANAGER_CONNECT );
    if (!disp->manager)
    {
        ERR("failed to open service manager error %lu\n", GetLastError());
        free( disp );
        return FALSE;
    }

    disp->pipe = service_open_pipe();
    if (disp->pipe == INVALID_HANDLE_VALUE)
    {
        WARN("failed to create control pipe error %lu\n", GetLastError());
        CloseServiceHandle( disp->manager );
        free( disp );
        SetLastError( ERROR_FAILED_SERVICE_CONTROLLER_CONNECT );
        return FALSE;
    }

    service_event = CreateEventW( NULL, FALSE, FALSE, NULL );
    stop_service  = FALSE;

    /* FIXME: service_control_dispatcher should be merged into the main thread */
    NtSetInformationProcess( GetCurrentProcess(), ProcessWineMakeProcessSystem,
                             &wait_handles[0], sizeof(HANDLE *) );
    wait_handles[1] = CreateThread( NULL, 0, service_control_dispatcher, disp, 0, NULL );
    wait_handles[2] = service_event;

    TRACE("Starting %d services running as process %ld\n",
          nb_services, GetCurrentProcessId());

    /* wait for all the threads to pack up and exit */
    while (!stop_service)
    {
        EnterCriticalSection( &service_cs );
        for (i = 0, n = 3; i < nb_services && n < MAXIMUM_WAIT_OBJECTS; i++)
        {
            if (!services[i]->thread) continue;
            wait_services[n] = i;
            wait_handles[n++] = services[i]->thread;
        }
        LeaveCriticalSection( &service_cs );

        ret = WaitForMultipleObjects( n, wait_handles, FALSE, INFINITE );
        if (!ret)  /* system process event */
        {
            handle_shutdown_msg(SERVICE_CONTROL_PRESHUTDOWN, SERVICE_ACCEPT_PRESHUTDOWN);
            handle_shutdown_msg(SERVICE_CONTROL_SHUTDOWN, SERVICE_ACCEPT_SHUTDOWN);
            ExitProcess(0);
        }
        else if (ret == 1)
        {
            TRACE( "control dispatcher exited, shutting down\n" );
            /* FIXME: we should maybe send a shutdown control to running services */
            ExitProcess(0);
        }
        else if (ret == 2)
        {
            continue;  /* rebuild the list */
        }
        else if (ret < n)
        {
            i = wait_services[ret];
            EnterCriticalSection( &service_cs );
            CloseHandle( services[i]->thread );
            services[i]->thread = NULL;
            LeaveCriticalSection( &service_cs );
        }
        else return FALSE;
    }

    return TRUE;
}

/******************************************************************************
 *     StartServiceCtrlDispatcherA   (sechost.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH StartServiceCtrlDispatcherA( const SERVICE_TABLE_ENTRYA *servent )
{
    struct service_data *info;
    unsigned int i;

    TRACE("%p\n", servent);

    if (nb_services)
    {
        SetLastError( ERROR_SERVICE_ALREADY_RUNNING );
        return FALSE;
    }
    while (servent[nb_services].lpServiceName && servent[nb_services].lpServiceProc)
        nb_services++;
    if (!nb_services)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    services = malloc( nb_services * sizeof(*services) );

    for (i = 0; i < nb_services; i++)
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, servent[i].lpServiceName, -1, NULL, 0 );
        DWORD sz = FIELD_OFFSET( struct service_data, name[len] );
        info = calloc( 1, sz );
        MultiByteToWideChar( CP_ACP, 0, servent[i].lpServiceName, -1, info->name, len );
        info->proc.a = servent[i].lpServiceProc;
        info->unicode = FALSE;
        services[i] = info;
    }

    return service_run_main_thread();
}

/******************************************************************************
 *     StartServiceCtrlDispatcherW   (sechost.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH StartServiceCtrlDispatcherW( const SERVICE_TABLE_ENTRYW *servent )
{
    struct service_data *info;
    unsigned int i;

    TRACE("%p\n", servent);

    if (nb_services)
    {
        SetLastError( ERROR_SERVICE_ALREADY_RUNNING );
        return FALSE;
    }
    while (servent[nb_services].lpServiceName && servent[nb_services].lpServiceProc)
        nb_services++;
    if (!nb_services)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    services = malloc( nb_services * sizeof(*services) );

    for (i = 0; i < nb_services; i++)
    {
        DWORD len = wcslen( servent[i].lpServiceName ) + 1;
        DWORD sz = FIELD_OFFSET( struct service_data, name[len] );
        info = calloc( 1, sz );
        wcscpy( info->name, servent[i].lpServiceName );
        info->proc.w = servent[i].lpServiceProc;
        info->unicode = TRUE;
        services[i] = info;
    }

    return service_run_main_thread();
}

static HANDLE device_notify_thread;
static struct list device_notify_list = LIST_INIT(device_notify_list);

struct device_notify
{
    struct list entry;
    WCHAR *path;
    HANDLE handle;
    device_notify_callback callback;
    DEV_BROADCAST_HDR header[]; /* variable size */
};

C_ASSERT( sizeof(struct device_notify) == offsetof(struct device_notify, header[0]) );

static struct device_notify *device_notify_copy( struct device_notify *notify, DEV_BROADCAST_HDR *header )
{
    struct device_notify *event;

    if (!(event = calloc( 1, sizeof(*event) + header->dbch_size ))) return NULL;
    event->handle = notify->handle;
    event->callback = notify->callback;
    memcpy( event->header, header, header->dbch_size );

    if (header->dbch_devicetype == DBT_DEVTYP_HANDLE)
    {
        DEV_BROADCAST_HANDLE *notify_handle = (DEV_BROADCAST_HANDLE *)notify->header;
        DEV_BROADCAST_HANDLE *event_handle = (DEV_BROADCAST_HANDLE *)event->header;
        event_handle->dbch_handle = notify_handle->dbch_handle;
        event_handle->dbch_hdevnotify = notify;
    }

    return event;
}

static BOOL notification_filter_matches( DEV_BROADCAST_HDR *filter, const WCHAR *path, DEV_BROADCAST_HDR *event, const WCHAR *event_path )
{
    if (!filter->dbch_devicetype) return TRUE;
    if (filter->dbch_devicetype != event->dbch_devicetype) return FALSE;

    if (filter->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
    {
        DEV_BROADCAST_DEVICEINTERFACE_W *filter_iface = (DEV_BROADCAST_DEVICEINTERFACE_W *)filter;
        DEV_BROADCAST_DEVICEINTERFACE_W *event_iface = (DEV_BROADCAST_DEVICEINTERFACE_W *)event;
        if (filter_iface->dbcc_size == offsetof(DEV_BROADCAST_DEVICEINTERFACE_W, dbcc_classguid)) return TRUE;
        return IsEqualGUID( &filter_iface->dbcc_classguid, &event_iface->dbcc_classguid );
    }

    if (filter->dbch_devicetype == DBT_DEVTYP_HANDLE) return !wcscmp(path, event_path);

    FIXME( "Filter dbch_devicetype %lu not implemented\n", filter->dbch_devicetype );
    return TRUE;
}

static DWORD WINAPI device_notify_proc( void *arg )
{
    WCHAR endpoint[] = L"\\pipe\\wine_plugplay";
    WCHAR protseq[] = L"ncacn_np";
    RPC_WSTR binding_str;
    DWORD err = ERROR_SUCCESS;
    struct device_notify *notify, *event, *next;
    struct list events = LIST_INIT(events);
    plugplay_rpc_handle handle = NULL;
    DWORD code = 0;
    unsigned int size;
    WCHAR *path;
    BYTE *buf;

    SetThreadDescription( GetCurrentThread(), L"wine_sechost_device_notify" );

    if ((err = RpcStringBindingComposeW( NULL, protseq, NULL, endpoint, NULL, &binding_str )))
    {
        ERR("RpcStringBindingCompose() failed, error %#lx\n", err);
        return err;
    }
    err = RpcBindingFromStringBindingW( binding_str, &plugplay_binding_handle );
    RpcStringFreeW( &binding_str );
    if (err)
    {
        ERR("RpcBindingFromStringBinding() failed, error %#lx\n", err);
        return err;
    }

    __TRY
    {
        handle = plugplay_register_listener();
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code( GetExceptionCode() );
    }
    __ENDTRY

    if (!handle)
    {
        ERR("failed to open RPC handle, error %lu\n", err);
        return 1;
    }

    for (;;)
    {
        path = NULL;
        buf = NULL;
        __TRY
        {
            code = plugplay_get_event( handle, &path, &buf, &size );
            err = ERROR_SUCCESS;
        }
        __EXCEPT(rpc_filter)
        {
            err = map_exception_code( GetExceptionCode() );
        }
        __ENDTRY

        if (err)
        {
            ERR("failed to get event, error %lu\n", err);
            break;
        }

        /* Make a copy to avoid a hang if a callback tries to register or unregister for notifications. */
        EnterCriticalSection( &service_cs );
        LIST_FOR_EACH_ENTRY( notify, &device_notify_list, struct device_notify, entry )
        {
            if (!notification_filter_matches( notify->header, notify->path, (DEV_BROADCAST_HDR *)buf, path )) continue;
            if (!(event = device_notify_copy( notify, (DEV_BROADCAST_HDR *)buf ))) break;
            list_add_tail( &events, &event->entry );
        }
        LeaveCriticalSection(&service_cs);

        LIST_FOR_EACH_ENTRY_SAFE( event, next, &events, struct device_notify, entry )
        {
            event->callback( event->handle, code, event->header );
            list_remove( &event->entry );
            free( event );
        }

        MIDL_user_free(buf);
        MIDL_user_free(path);
    }

    __TRY
    {
        plugplay_unregister_listener( handle );
    }
    __EXCEPT(rpc_filter)
    {
    }
    __ENDTRY

    RpcBindingFree( &plugplay_binding_handle );
    return 0;
}

/******************************************************************************
 *     I_ScRegisterDeviceNotification   (sechost.@)
 */
HDEVNOTIFY WINAPI I_ScRegisterDeviceNotification( HANDLE handle, DEV_BROADCAST_HDR *filter, device_notify_callback callback )
{
    struct device_notify *notify;

    TRACE( "handle %p, filter %p, callback %p\n", handle, filter, callback );

    if (!(notify = calloc( 1, sizeof(*notify) + filter->dbch_size )))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    notify->handle = handle;
    notify->callback = callback;
    memcpy( notify->header, filter, filter->dbch_size );

    if (filter->dbch_devicetype == DBT_DEVTYP_HANDLE)
    {
        WCHAR buffer[sizeof(OBJECT_NAME_INFORMATION) + MAX_PATH + 1];
        OBJECT_NAME_INFORMATION *info = (OBJECT_NAME_INFORMATION*)&buffer;
        DEV_BROADCAST_HANDLE *handle = (DEV_BROADCAST_HANDLE *)filter;
        NTSTATUS status;
        ULONG dummy;

        status = NtQueryObject( handle->dbch_handle, ObjectNameInformation, &buffer, sizeof(buffer) - sizeof(WCHAR), &dummy );
        if (status || !(notify->path = calloc( 1, info->Name.Length + sizeof(WCHAR) )))
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            free( notify );
            return NULL;
        }

        memcpy( notify->path, info->Name.Buffer, info->Name.Length );
    }

    EnterCriticalSection( &service_cs );
    list_add_tail( &device_notify_list, &notify->entry );

    if (!device_notify_thread)
        device_notify_thread = CreateThread( NULL, 0, device_notify_proc, NULL, 0, NULL );

    LeaveCriticalSection( &service_cs );

    return notify;
}

/******************************************************************************
 *     I_ScUnregisterDeviceNotification   (sechost.@)
 */
BOOL WINAPI I_ScUnregisterDeviceNotification( HDEVNOTIFY handle )
{
    struct device_notify *notify = handle;

    TRACE("%p\n", handle);

    if (!handle)
        return FALSE;

    EnterCriticalSection( &service_cs );
    list_remove( &notify->entry );
    LeaveCriticalSection(&service_cs);
    free( notify->path );
    free( notify );
    return TRUE;
}
