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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "winbase.h"
#include "winerror.h"
#include "winsock2.h"
#include "wtypes.h"
#include "nspapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winsock);

/******************************************************************************
 *          GetTypeByNameA     [WSOCK32.1113]
 *
 * Retrieves a service type GUID for a network service specified by name
 *
 * PARAMETERS
 *      lpServiceName -- Pointer to a zero-terminated ASCII string that uniquely represents the name of the service
 *      lpServiceType -- Pointer to a variable to receive a GUID that specifies the type of network service
 *
 * RETURNS
 *      Zero on success, SOCKET_ERROR on failure.
 *      GetLastError can return ERROR_SERVICE_DOES_NOT_EXIST
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
   FIXME("wsock32: GetTypeByNameA(%p, %p): stub/n", lpServiceName, lpServiceType);
   
   /* some programs may be able to compensate if they know what happened */
   SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
   return SOCKET_ERROR; /* error value */
}


/******************************************************************************
 *          GetTypeByNameW     [WSOCK32.1114]
 *
 * Retrieves a service type GUID for a network service specified by name
 *
 * PARAMETERS
 *      lpServiceName -- Pointer to a zero-terminated Unicode string that uniquely represents the name of the service
 *      lpServiceType -- Pointer to a variable to receive a GUID that specifies the type of network service
 *
 * RETURNS
 *      Zero on success, SOCKET_ERROR on failure.
 *      GetLastError can return ERROR_SERVICE_DOES_NOT_EXIST
 *
 * NOTES
 *      Obsolete Microsoft-specific extension to Winsock 1.1.
 *      Protocol-independent name resolution provides equivalent functionality in Winsock 2.
 *
 * BUGS
 *      Unimplemented
 */
INT WINAPI GetTypeByNameW(LPWSTR lpServiceName, LPGUID lpServiceType)
{
    /* tell the user they've got a substandard implementation */
    FIXME("wsock32: GetTypeByNameW(%p, %p): stub/n", lpServiceName, lpServiceType);
    
    /* some programs may be able to compensate if they know what happened */
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return SOCKET_ERROR; /* error value */
}

/******************************************************************************
 *          SetServiceA     [WSOCK32.1117]
 *
 * Registers or unregisters a network service with one or more name spaces
 *
 * PARAMETERS
 *      dwNameSpace         -- Name space or set of name spaces within which the function will operate
 *      dwOperation         -- Specifies the operation that the function will perform
 *      dwFlags             -- Set of bit flags that modify the function's operation
 *      lpServiceInfo       -- Pointer to a ASCII SERVICE_INFO structure
 *      lpServiceAsyncInfo  -- Reserved for future use.  Must be NULL.
 *      lpdwStatusFlags     -- Set of bit flags that receive function status information
 *
 * RETURNS
 *      SOCKET_ERROR on failure.
 *      GetLastError can return ERROR_ALREADY_REGISTERED
 *
 * NOTES
 *      Obsolete Microsoft-specific extension to Winsock 1.1
 *      Protocol-independent name resolution provides equivalent functionality in Winsock 2.
 *
 * BUGS
 *      Unimplemented.
 */
INT WINAPI SetServiceA(DWORD dwNameSpace, DWORD dwOperation, DWORD dwFlags, LPSERVICE_INFOA lpServiceInfo,
                       LPSERVICE_ASYNC_INFO lpServiceAsyncInfo, LPDWORD lpdwStatusFlags)
{
   /* tell the user they've got a substandard implementation */
   FIXME("wsock32: SetServiceA(%lu, %lu, %lu, %p, %p, %p): stub/n", dwNameSpace, dwOperation, dwFlags,
           lpServiceInfo, lpServiceAsyncInfo, lpdwStatusFlags);
    
   /* some programs may be able to compensate if they know what happened */
   SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
   return SOCKET_ERROR; /* error value */ 
}

/******************************************************************************
 *          SetServiceW     [WSOCK32.1118]
 *
 * Registers or unregisters a network service with one or more name spaces
 *
 * PARAMETERS
 *      dwNameSpace         -- Name space or set of name spaces within which the function will operate
 *      dwOperation         -- Specifies the operation that the function will perform
 *      dwFlags             -- Set of bit flags that modify the function's operation
 *      lpServiceInfo       -- Pointer to a Unicode SERVICE_INFO structure
 *      lpServiceAsyncInfo  -- Reserved for future use.  Must be NULL.
 *      lpdwStatusFlags     -- Set of bit flags that receive function status information
 *
 * RETURNS
 *      SOCKET_ERROR on failure.
 *      GetLastError can return ERROR_ALREADY_REGISTERED
 *
 * NOTES
 *      Obsolete Microsoft-specific extension to Winsock 1.1
 *      Protocol-independent name resolution provides equivalent functionality in Winsock 2.
 *
 * BUGS
 *      Unimplemented.
 */
INT WINAPI SetServiceW(DWORD dwNameSpace, DWORD dwOperation, DWORD dwFlags, LPSERVICE_INFOW lpServiceInfo,
                       LPSERVICE_ASYNC_INFO lpServiceAsyncInfo, LPDWORD lpdwStatusFlags)
{
   /* tell the user they've got a substandard implementation */
   FIXME("wsock32: SetServiceW(%lu, %lu, %lu, %p, %p, %p): stub/n", dwNameSpace, dwOperation, dwFlags,
           lpServiceInfo, lpServiceAsyncInfo, lpdwStatusFlags);
    
   /* some programs may be able to compensate if they know what happened */
   SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
   return SOCKET_ERROR; /* error value */ 
}
