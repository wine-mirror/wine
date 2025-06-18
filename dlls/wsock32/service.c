/*
 * WSOCK32 specific functions
 *
 * Copyright (C) 2002 Andrew Hughes
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
#include "winerror.h"
#include "winsock2.h"
#include "wtypes.h"
#include "nspapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winsock);

INT WINAPI GetAddressByNameA(DWORD dwNameSpace, LPGUID lpServiceType, LPSTR lpServiceName,
    LPINT lpiProtocols, DWORD dwResolution, LPSERVICE_ASYNC_INFO lpServiceAsyncInfo,
    LPVOID lpCsaddrBuffer, LPDWORD lpdwBufferLength, LPSTR lpAliasBuffer,
    LPDWORD lpdwAliasBufferLength)
{
    FIXME("(0x%08lx, %s, %s, %p, 0x%08lx, %p, %p, %p, %p, %p) stub\n", dwNameSpace,
          debugstr_guid(lpServiceType), debugstr_a(lpServiceName), lpiProtocols,
          dwResolution, lpServiceAsyncInfo, lpCsaddrBuffer, lpdwBufferLength,
          lpAliasBuffer, lpdwAliasBufferLength);
 
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return SOCKET_ERROR;
}

INT WINAPI GetAddressByNameW(DWORD dwNameSpace, LPGUID lpServiceType, LPWSTR lpServiceName,
    LPINT lpiProtocols, DWORD dwResolution, LPSERVICE_ASYNC_INFO lpServiceAsyncInfo,
    LPVOID lpCsaddrBuffer, LPDWORD lpdwBufferLength, LPWSTR lpAliasBuffer,
    LPDWORD lpdwAliasBufferLength)
{
    FIXME("(0x%08lx, %s, %s, %p, 0x%08lx, %p, %p, %p, %p, %p) stub\n", dwNameSpace,
          debugstr_guid(lpServiceType), debugstr_w(lpServiceName), lpiProtocols,
          dwResolution, lpServiceAsyncInfo, lpCsaddrBuffer, lpdwBufferLength,
          lpAliasBuffer, lpdwAliasBufferLength);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return SOCKET_ERROR;
}

/******************************************************************************
 *          GetTypeByNameA     [WSOCK32.1113]
 *
 * Retrieve a service type GUID for a network service specified by name.
 *
 * PARAMETERS
 *      lpServiceName [I] NUL-terminated ANSI string that uniquely represents the name of the service
 *      lpServiceType [O] Destination for the service type GUID
 *
 * RETURNS
 *      Success: 0. lpServiceType contains the requested GUID
 *      Failure: SOCKET_ERROR. GetLastError() can return ERROR_SERVICE_DOES_NOT_EXIST
 *
 * NOTES
 *      Obsolete Microsoft-specific extension to Winsock 1.1.
 *      Protocol-independent name resolution provides equivalent functionality in Winsock 2.
 *
 * BUGS
 *      Unimplemented
 */
INT WINAPI GetTypeByNameA(LPSTR lpServiceName, LPGUID lpServiceType)
{
   /* tell the user they've got a substandard implementation */
   FIXME("wsock32: GetTypeByNameA(%p, %p): stub\n", lpServiceName, lpServiceType);
   
   /* some programs may be able to compensate if they know what happened */
   SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
   return SOCKET_ERROR; /* error value */
}


/******************************************************************************
 *          GetTypeByNameW     [WSOCK32.1114]
 *
 * See GetTypeByNameA.
 */
INT WINAPI GetTypeByNameW(LPWSTR lpServiceName, LPGUID lpServiceType)
{
    /* tell the user they've got a substandard implementation */
    FIXME("wsock32: GetTypeByNameW(%p, %p): stub\n", lpServiceName, lpServiceType);
    
    /* some programs may be able to compensate if they know what happened */
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return SOCKET_ERROR; /* error value */
}

/******************************************************************************
 *          SetServiceA     [WSOCK32.1117]
 *
 * Register or unregister a network service with one or more namespaces.
 *
 * PARAMETERS
 *      dwNameSpace        [I] Name space or set of name spaces within which the function will operate
 *      dwOperation        [I] Operation to perform
 *      dwFlags            [I] Flags to modify the function's operation
 *      lpServiceInfo      [I] Pointer to an ANSI SERVICE_INFO structure
 *      lpServiceAsyncInfo [I] Reserved for future use.  Must be NULL.
 *      lpdwStatusFlags    [O] Destination for function status information
 *
 * RETURNS
 *      Success: 0.
 *      Failure: SOCKET_ERROR. GetLastError() can return ERROR_ALREADY_REGISTERED
 *
 * NOTES
 *      Obsolete Microsoft-specific extension to Winsock 1.1,
 *      Protocol-independent name resolution provides equivalent functionality in Winsock 2.
 *
 * BUGS
 *      Unimplemented.
 */
INT WINAPI SetServiceA(DWORD dwNameSpace, DWORD dwOperation, DWORD dwFlags, LPSERVICE_INFOA lpServiceInfo,
                       LPSERVICE_ASYNC_INFO lpServiceAsyncInfo, LPDWORD lpdwStatusFlags)
{
   /* tell the user they've got a substandard implementation */
   FIXME("wsock32: SetServiceA(%lu, %lu, %lu, %p, %p, %p): stub\n", dwNameSpace, dwOperation, dwFlags,
           lpServiceInfo, lpServiceAsyncInfo, lpdwStatusFlags);
    
   /* some programs may be able to compensate if they know what happened */
   SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
   return SOCKET_ERROR; /* error value */ 
}

/******************************************************************************
 *          SetServiceW     [WSOCK32.1118]
 *
 * See SetServiceA.
 */
INT WINAPI SetServiceW(DWORD dwNameSpace, DWORD dwOperation, DWORD dwFlags, LPSERVICE_INFOW lpServiceInfo,
                       LPSERVICE_ASYNC_INFO lpServiceAsyncInfo, LPDWORD lpdwStatusFlags)
{
   /* tell the user they've got a substandard implementation */
   FIXME("wsock32: SetServiceW(%lu, %lu, %lu, %p, %p, %p): stub\n", dwNameSpace, dwOperation, dwFlags,
           lpServiceInfo, lpServiceAsyncInfo, lpdwStatusFlags);
    
   /* some programs may be able to compensate if they know what happened */
   SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
   return SOCKET_ERROR; /* error value */ 
}

/******************************************************************************
 *          GetServiceA     [WSOCK32.1119]
 *
 * Get information about a network service.
 *
 * PARAMETERS
 *      dwNameSpace        [I] Name space or set of name spaces within which the function 
 *                             will operate.
 *      lpGuid             [I] Pointer to GUID of network service type.
 *      lpServiceName      [I] NUL-terminated ANSI string that uniquely represents the name
 *                             of the service.
 *      dwProperties       [I] Flags specifying which information to return in lpBuffer.
 *      lpBuffer           [O] Pointer to buffer where the function returns an array
 *                             of NS_SERVICE_INFO.
 *      lpdwBufferSize     [I/O] Size of lpBuffer.  A greater number on output
 *                               indicates an error.
 *      lpServiceAsyncInfo [O] Reserved.  Set to NULL.
 *
 * RETURNS
 *      Success: 0.
 *      Failure: SOCKET_ERROR. GetLastError() returns ERROR_INSUFFICIENT_BUFFER
 *               or ERROR_SERVICE_NOT_FOUND.
 *
 * NOTES
 *      Obsolete Microsoft-specific extension to Winsock 1.1,
 *      Protocol-independent name resolution provides equivalent functionality in Winsock 2.
 *
 * BUGS
 *      Unimplemented.
 */
INT WINAPI GetServiceA(DWORD dwNameSpace, LPGUID lpGuid, LPSTR lpServiceName,
                       DWORD dwProperties, LPVOID lpBuffer, LPDWORD lpdwBufferSize,
                       LPSERVICE_ASYNC_INFO lpServiceAsyncInfo)
{
   FIXME("(%lu, %p, %s, %lu, %p, %p, %p): stub\n", dwNameSpace,
         lpGuid, lpServiceName, dwProperties, lpBuffer, lpdwBufferSize, lpServiceAsyncInfo);

   /* some programs may be able to compensate if they know what happened */
   SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
   return SOCKET_ERROR;
}

/******************************************************************************
 *          GetServiceW     [WSOCK32.1120]
 *
 * See GetServiceA.
 */
INT WINAPI GetServiceW(DWORD dwNameSpace, LPGUID lpGuid, LPSTR lpServiceName,
                       DWORD dwProperties, LPVOID lpBuffer, LPDWORD lpdwBufferSize,
                       LPSERVICE_ASYNC_INFO lpServiceAsyncInfo)
{
   FIXME("(%lu, %p, %s, %lu, %p, %p, %p): stub\n", dwNameSpace,
         lpGuid, lpServiceName, dwProperties, lpBuffer, lpdwBufferSize, lpServiceAsyncInfo);

   /* some programs may be able to compensate if they know what happened */
   SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
   return SOCKET_ERROR;
}
