/*
 * Win32 advapi functions
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
#include "windef.h"
#include "winbase.h"
#include "winsvc.h"
#include "svcctl.h"
#include "wine/debug.h"
#include "wine/exception.h"

#include "advapi32_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(service);

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

/******************************************************************************
 * LockServiceDatabase  [ADVAPI32.@]
 */
SC_LOCK WINAPI LockServiceDatabase( SC_HANDLE manager )
{
    SC_LOCK handle = NULL;
    DWORD err;

    TRACE("%p\n", manager);

    if (!manager)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return NULL;
    }

    __TRY
    {
        err = svcctl_LockServiceDatabase( manager, &handle );
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
 * UnlockServiceDatabase  [ADVAPI32.@]
 */
BOOL WINAPI UnlockServiceDatabase( SC_LOCK lock )
{
    DWORD err;

    TRACE("%p\n", lock);

    if (!lock)
    {
        SetLastError( ERROR_INVALID_SERVICE_LOCK );
        return FALSE;
    }

    __TRY
    {
        err = svcctl_UnlockServiceDatabase( &lock );
    }
    __EXCEPT(rpc_filter)
    {
        err = GetExceptionCode();

        if (err == ERROR_INVALID_HANDLE)
            err = ERROR_INVALID_SERVICE_LOCK;
        else
            err = map_exception_code( err );
    }
    __ENDTRY

    if (!err) return TRUE;
    SetLastError( err );
    return FALSE;

}

/******************************************************************************
 * EnumServicesStatusA [ADVAPI32.@]
 */
BOOL WINAPI
EnumServicesStatusA( SC_HANDLE hmngr, DWORD type, DWORD state, LPENUM_SERVICE_STATUSA
                     services, DWORD size, LPDWORD needed, LPDWORD returned,
                     LPDWORD resume_handle )
{
    BOOL ret;
    unsigned int i;
    ENUM_SERVICE_STATUSW *servicesW = NULL;
    DWORD sz, n;
    char *p;

    TRACE("%p 0x%lx 0x%lx %p %lu %p %p %p\n", hmngr, type, state, services, size, needed,
          returned, resume_handle);

    if (!hmngr)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (!needed || !returned)
    {
        SetLastError( ERROR_INVALID_ADDRESS );
        return FALSE;
    }

    sz = max( 2 * size, sizeof(*servicesW) );
    if (!(servicesW = malloc( sz )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    ret = EnumServicesStatusW( hmngr, type, state, servicesW, sz, needed, returned, resume_handle );
    if (!ret) goto done;

    p = (char *)services + *returned * sizeof(ENUM_SERVICE_STATUSA);
    n = size - (p - (char *)services);
    ret = FALSE;
    for (i = 0; i < *returned; i++)
    {
        sz = WideCharToMultiByte( CP_ACP, 0, servicesW[i].lpServiceName, -1, p, n, NULL, NULL );
        if (!sz) goto done;
        services[i].lpServiceName = p;
        p += sz;
        n -= sz;
        if (servicesW[i].lpDisplayName)
        {
            sz = WideCharToMultiByte( CP_ACP, 0, servicesW[i].lpDisplayName, -1, p, n, NULL, NULL );
            if (!sz) goto done;
            services[i].lpDisplayName = p;
            p += sz;
            n -= sz;
        }
        else services[i].lpDisplayName = NULL;
        services[i].ServiceStatus = servicesW[i].ServiceStatus;
    }

    ret = TRUE;

done:
    free( servicesW );
    return ret;
}

/******************************************************************************
 * EnumServicesStatusW [ADVAPI32.@]
 */
BOOL WINAPI
EnumServicesStatusW( SC_HANDLE manager, DWORD type, DWORD state, ENUM_SERVICE_STATUSW *status,
                     DWORD size, DWORD *ret_size, DWORD *ret_count, DWORD *resume_handle )
{
    ENUM_SERVICE_STATUS_PROCESSW *status_ex;
    DWORD alloc_size, count, i;
    WCHAR *p;

    TRACE("%p 0x%lx 0x%lx %p %lu %p %p %p\n", manager, type, state, status, size,
          ret_size, ret_count, resume_handle);

    if (!manager)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    if (!ret_size || !ret_count)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    *ret_size = 0;
    *ret_count = 0;
    if (!EnumServicesStatusExW( manager, SC_ENUM_PROCESS_INFO, type, state,
                                NULL, 0, &alloc_size, &count, resume_handle, NULL )
            && GetLastError() != ERROR_MORE_DATA)
        return FALSE;

    if (!(status_ex = malloc( alloc_size )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    if (!EnumServicesStatusExW( manager, SC_ENUM_PROCESS_INFO, type, state, (BYTE *)status_ex,
                                alloc_size, &alloc_size, &count, resume_handle, NULL )
            && GetLastError() != ERROR_MORE_DATA)
    {
        free( status_ex );
        return FALSE;
    }

    for (i = 0; i < count; i++)
    {
        *ret_size += sizeof(ENUM_SERVICE_STATUSW);
        *ret_size += (lstrlenW( status_ex[i].lpServiceName ) + 1) * sizeof(WCHAR);
        if (status_ex[i].lpDisplayName)
            *ret_size += (lstrlenW( status_ex[i].lpDisplayName ) + 1) * sizeof(WCHAR);

        if (*ret_size <= size)
            ++*ret_count;
    }

    p = (WCHAR *)(status + *ret_count);
    for (i = 0; i < *ret_count; i++)
    {
        lstrcpyW( p, status_ex[i].lpServiceName );
        status[i].lpServiceName = p;
        p += lstrlenW( p ) + 1;
        if (status_ex[i].lpDisplayName)
        {
            lstrcpyW( p, status_ex[i].lpDisplayName );
            status[i].lpDisplayName = p;
            p += lstrlenW( p ) + 1;
        }
        else status[i].lpDisplayName = NULL;

        status[i].ServiceStatus.dwServiceType             = status_ex[i].ServiceStatusProcess.dwServiceType;
        status[i].ServiceStatus.dwCurrentState            = status_ex[i].ServiceStatusProcess.dwCurrentState;
        status[i].ServiceStatus.dwControlsAccepted        = status_ex[i].ServiceStatusProcess.dwControlsAccepted;
        status[i].ServiceStatus.dwWin32ExitCode           = status_ex[i].ServiceStatusProcess.dwWin32ExitCode;
        status[i].ServiceStatus.dwServiceSpecificExitCode = status_ex[i].ServiceStatusProcess.dwServiceSpecificExitCode;
        status[i].ServiceStatus.dwCheckPoint              = status_ex[i].ServiceStatusProcess.dwCheckPoint;
        status[i].ServiceStatus.dwWaitHint                = status_ex[i].ServiceStatusProcess.dwWaitHint;
    }

    free( status_ex );
    if (*ret_size > size)
    {
        SetLastError( ERROR_MORE_DATA );
        return FALSE;
    }

    *ret_size = 0;
    return TRUE;
}

/******************************************************************************
 * EnumServicesStatusExA [ADVAPI32.@]
 */
BOOL WINAPI
EnumServicesStatusExA( SC_HANDLE hmngr, SC_ENUM_TYPE level, DWORD type, DWORD state,
                       LPBYTE buffer, DWORD size, LPDWORD needed, LPDWORD returned,
                       LPDWORD resume_handle, LPCSTR group )
{
    BOOL ret;
    unsigned int i;
    ENUM_SERVICE_STATUS_PROCESSA *services = (ENUM_SERVICE_STATUS_PROCESSA *)buffer;
    ENUM_SERVICE_STATUS_PROCESSW *servicesW = NULL;
    WCHAR *groupW = NULL;
    DWORD sz, n;
    char *p;

    TRACE("%p %u 0x%lx 0x%lx %p %lu %p %p %p %s\n", hmngr, level, type, state, buffer,
          size, needed, returned, resume_handle, debugstr_a(group));

    sz = max( 2 * size, sizeof(*servicesW) );
    if (!(servicesW = malloc( sz )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }
    if (group)
    {
        int len = MultiByteToWideChar( CP_ACP, 0, group, -1, NULL, 0 );
        if (!(groupW = malloc( len * sizeof(WCHAR) )))
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            free( servicesW );
            return FALSE;
        }
        MultiByteToWideChar( CP_ACP, 0, group, -1, groupW, len * sizeof(WCHAR) );
    }

    ret = EnumServicesStatusExW( hmngr, level, type, state, (BYTE *)servicesW, sz,
                                 needed, returned, resume_handle, groupW );
    if (!ret) goto done;

    p = (char *)services + *returned * sizeof(ENUM_SERVICE_STATUS_PROCESSA);
    n = size - (p - (char *)services);
    ret = FALSE;
    for (i = 0; i < *returned; i++)
    {
        sz = WideCharToMultiByte( CP_ACP, 0, servicesW[i].lpServiceName, -1, p, n, NULL, NULL );
        if (!sz) goto done;
        services[i].lpServiceName = p;
        p += sz;
        n -= sz;
        if (servicesW[i].lpDisplayName)
        {
            sz = WideCharToMultiByte( CP_ACP, 0, servicesW[i].lpDisplayName, -1, p, n, NULL, NULL );
            if (!sz) goto done;
            services[i].lpDisplayName = p;
            p += sz;
            n -= sz;
        }
        else services[i].lpDisplayName = NULL;
        services[i].ServiceStatusProcess = servicesW[i].ServiceStatusProcess;
    }

    ret = TRUE;

done:
    free( servicesW );
    free( groupW );
    return ret;
}

/******************************************************************************
 * GetServiceKeyNameA [ADVAPI32.@]
 */
BOOL WINAPI GetServiceKeyNameA( SC_HANDLE hSCManager, LPCSTR lpDisplayName,
                                LPSTR lpServiceName, LPDWORD lpcchBuffer )
{
    LPWSTR lpDisplayNameW, lpServiceNameW;
    DWORD sizeW;
    BOOL ret = FALSE;

    TRACE("%p %s %p %p\n", hSCManager,
          debugstr_a(lpDisplayName), lpServiceName, lpcchBuffer);

    lpDisplayNameW = strdupAW(lpDisplayName);
    if (lpServiceName)
        lpServiceNameW = malloc(*lpcchBuffer * sizeof(WCHAR));
    else
        lpServiceNameW = NULL;

    sizeW = *lpcchBuffer;
    if (!GetServiceKeyNameW(hSCManager, lpDisplayNameW, lpServiceNameW, &sizeW))
    {
        if (lpServiceName && *lpcchBuffer)
            lpServiceName[0] = 0;
        *lpcchBuffer = sizeW*2;  /* we can only provide an upper estimation of string length */
        goto cleanup;
    }

    if (!WideCharToMultiByte(CP_ACP, 0, lpServiceNameW, (sizeW + 1), lpServiceName,
                        *lpcchBuffer, NULL, NULL ))
    {
        if (*lpcchBuffer && lpServiceName)
            lpServiceName[0] = 0;
        *lpcchBuffer = WideCharToMultiByte(CP_ACP, 0, lpServiceNameW, -1, NULL, 0, NULL, NULL);
        goto cleanup;
    }

    /* lpcchBuffer not updated - same as in GetServiceDisplayNameA */
    ret = TRUE;

cleanup:
    free(lpServiceNameW);
    free(lpDisplayNameW);
    return ret;
}

/******************************************************************************
 * QueryServiceLockStatusA [ADVAPI32.@]
 */
BOOL WINAPI QueryServiceLockStatusA( SC_HANDLE hSCManager,
                                     LPQUERY_SERVICE_LOCK_STATUSA lpLockStatus,
                                     DWORD cbBufSize, LPDWORD pcbBytesNeeded)
{
    FIXME("%p %p %08lx %p\n", hSCManager, lpLockStatus, cbBufSize, pcbBytesNeeded);

    return FALSE;
}

/******************************************************************************
 * QueryServiceLockStatusW [ADVAPI32.@]
 */
BOOL WINAPI QueryServiceLockStatusW( SC_HANDLE hSCManager,
                                     LPQUERY_SERVICE_LOCK_STATUSW lpLockStatus,
                                     DWORD cbBufSize, LPDWORD pcbBytesNeeded)
{
    FIXME("%p %p %08lx %p\n", hSCManager, lpLockStatus, cbBufSize, pcbBytesNeeded);

    return FALSE;
}

/******************************************************************************
 * GetServiceDisplayNameA  [ADVAPI32.@]
 */
BOOL WINAPI GetServiceDisplayNameA( SC_HANDLE hSCManager, LPCSTR lpServiceName,
  LPSTR lpDisplayName, LPDWORD lpcchBuffer)
{
    LPWSTR lpServiceNameW, lpDisplayNameW;
    DWORD sizeW;
    BOOL ret = FALSE;

    TRACE("%p %s %p %p\n", hSCManager,
          debugstr_a(lpServiceName), lpDisplayName, lpcchBuffer);

    lpServiceNameW = strdupAW(lpServiceName);
    if (lpDisplayName)
        lpDisplayNameW = malloc(*lpcchBuffer * sizeof(WCHAR));
    else
        lpDisplayNameW = NULL;

    sizeW = *lpcchBuffer;
    if (!GetServiceDisplayNameW(hSCManager, lpServiceNameW, lpDisplayNameW, &sizeW))
    {
        if (lpDisplayName && *lpcchBuffer)
            lpDisplayName[0] = 0;
        *lpcchBuffer = sizeW*2;  /* we can only provide an upper estimation of string length */
        goto cleanup;
    }

    if (!WideCharToMultiByte(CP_ACP, 0, lpDisplayNameW, (sizeW + 1), lpDisplayName,
                        *lpcchBuffer, NULL, NULL ))
    {
        if (*lpcchBuffer && lpDisplayName)
            lpDisplayName[0] = 0;
        *lpcchBuffer = WideCharToMultiByte(CP_ACP, 0, lpDisplayNameW, -1, NULL, 0, NULL, NULL);
        goto cleanup;
    }

    /* probably due to a bug GetServiceDisplayNameA doesn't modify lpcchBuffer on success.
     * (but if the function succeeded it means that is a good upper estimation of the size) */
    ret = TRUE;

cleanup:
    free(lpDisplayNameW);
    free(lpServiceNameW);
    return ret;
}

/******************************************************************************
 * SetServiceBits [ADVAPI32.@]
 */
BOOL WINAPI SetServiceBits( SERVICE_STATUS_HANDLE hServiceStatus,
        DWORD dwServiceBits,
        BOOL bSetBitsOn,
        BOOL bUpdateImmediately)
{
    FIXME("%p %08lx %x %x\n", hServiceStatus, dwServiceBits,
          bSetBitsOn, bUpdateImmediately);
    return TRUE;
}

/******************************************************************************
 * EnumDependentServicesA [ADVAPI32.@]
 */
BOOL WINAPI EnumDependentServicesA( SC_HANDLE hService, DWORD dwServiceState,
                                    LPENUM_SERVICE_STATUSA lpServices, DWORD cbBufSize,
        LPDWORD pcbBytesNeeded, LPDWORD lpServicesReturned )
{
    FIXME("%p 0x%08lx %p 0x%08lx %p %p - stub\n", hService, dwServiceState,
          lpServices, cbBufSize, pcbBytesNeeded, lpServicesReturned);

    *lpServicesReturned = 0;
    return TRUE;
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

void  __RPC_FAR * __RPC_USER MIDL_user_allocate( SIZE_T len )
{
    return malloc(len);
}

void __RPC_USER MIDL_user_free( void __RPC_FAR *ptr )
{
    free(ptr);
}
