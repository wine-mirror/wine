/*
 * Win32 advapi functions
 *
 * Copyright 1995 Sven Verdoolaege
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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "winternl.h"
#include "winerror.h"
#include "wincred.h"

#include "wine/library.h"
#include "wine/unicode.h"
#include "wine/debug.h"

#include "advapi32_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(advapi);

/******************************************************************************
 * GetUserNameA [ADVAPI32.@]
 *
 * Get the current user name.
 *
 * PARAMS
 *  lpszName [O]   Destination for the user name.
 *  lpSize   [I/O] Size of lpszName.
 *
 * RETURNS
 *  Success: The length of the user name, including terminating NUL.
 *  Failure: ERROR_MORE_DATA if *lpSize is too small.
 */
BOOL WINAPI
GetUserNameA( LPSTR lpszName, LPDWORD lpSize )
{
    WCHAR *buffer;
    BOOL ret;
    DWORD sizeW = *lpSize;

    if (!(buffer = heap_alloc( sizeW * sizeof(WCHAR) )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    ret = GetUserNameW( buffer, &sizeW );
    if (ret)
        *lpSize = WideCharToMultiByte( CP_ACP, 0, buffer, -1, lpszName, *lpSize, NULL, NULL );
    else
        *lpSize = sizeW;

    heap_free( buffer );
    return ret;
}

/******************************************************************************
 * GetUserNameW [ADVAPI32.@]
 *
 * See GetUserNameA.
 */
BOOL WINAPI
GetUserNameW( LPWSTR lpszName, LPDWORD lpSize )
{
    const char *name = wine_get_user_name();
    DWORD i, len = MultiByteToWideChar( CP_UNIXCP, 0, name, -1, NULL, 0 );
    LPWSTR backslash;

    if (len > *lpSize)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        *lpSize = len;
        return FALSE;
    }

    *lpSize = len;
    MultiByteToWideChar( CP_UNIXCP, 0, name, -1, lpszName, len );

    /* Word uses the user name to create named mutexes and file mappings,
     * and backslashes in the name cause the creation to fail.
     * Also, Windows doesn't return the domain name in the user name even when
     * joined to a domain. A Unix box joined to a domain using winbindd will
     * contain the domain name in the username. So we need to cut this off.
     * FIXME: Only replaces forward and backslashes for now, should get the
     * winbind separator char from winbindd and replace that.
     */
    for (i = 0; lpszName[i]; i++)
        if (lpszName[i] == '/') lpszName[i] = '\\';

    backslash = strrchrW(lpszName, '\\');
    if (backslash == NULL)
        return TRUE;

    len = lstrlenW(backslash);
    memmove(lpszName, backslash + 1, len * sizeof(WCHAR));
    *lpSize = len;
    return TRUE;
}

/******************************************************************************
 * GetCurrentHwProfileA [ADVAPI32.@]
 *
 * Get the current hardware profile.
 *
 * PARAMS
 *  pInfo [O] Destination for hardware profile information.
 *
 * RETURNS
 *  Success: TRUE. pInfo is updated with the hardware profile details.
 *  Failure: FALSE.
 */
BOOL WINAPI GetCurrentHwProfileA(LPHW_PROFILE_INFOA pInfo)
{
	FIXME("(%p) semi-stub\n", pInfo);
	pInfo->dwDockInfo = DOCKINFO_DOCKED;
	strcpy(pInfo->szHwProfileGuid,"{12340001-1234-1234-1234-123456789012}");
	strcpy(pInfo->szHwProfileName,"Wine Profile");
	return TRUE;
}

/******************************************************************************
 * GetCurrentHwProfileW [ADVAPI32.@]
 *
 * See GetCurrentHwProfileA.
 */
BOOL WINAPI GetCurrentHwProfileW(LPHW_PROFILE_INFOW pInfo)
{
       FIXME("(%p)\n", pInfo);
       return FALSE;
}


/**************************************************************************
 *	IsTextUnicode (ADVAPI32.@)
 *
 * Attempt to guess whether a text buffer is Unicode.
 *
 * PARAMS
 *  buf   [I] Text buffer to test
 *  len   [I] Length of buf
 *  flags [O] Destination for test results
 *
 * RETURNS
 *  TRUE if the buffer is likely Unicode, FALSE otherwise.
 */
BOOL WINAPI IsTextUnicode( LPCVOID buf, INT len, LPINT flags )
{
    return RtlIsTextUnicode( buf, len, flags );
}


/******************************************************************************
 * AbortSystemShutdownA [ADVAPI32.@]
 *
 * Stop a system shutdown if one is in progress.
 *
 * PARAMS
 *  lpMachineName [I] Name of machine to not shutdown.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 *
 * NOTES
 *  The Wine implementation of this function is a harmless stub.
 */
BOOL WINAPI AbortSystemShutdownA( LPSTR lpMachineName )
{
    TRACE("stub %s (harmless)\n", lpMachineName);
    return TRUE;
}

/******************************************************************************
 * AbortSystemShutdownW [ADVAPI32.@]
 *
 * See AbortSystemShutdownA.
 */
BOOL WINAPI AbortSystemShutdownW( LPWSTR lpMachineName )
{
    TRACE("stub %s (harmless)\n", debugstr_w(lpMachineName));
    return TRUE;
}

/******************************************************************************
 * InitiateSystemShutdownExA [ADVAPI32.@]
 *
 * Initiate a shutdown or optionally restart the computer.
 *
 * PARAMS
 *  lpMachineName    [I] Network name of machine to shutdown.
 *  lpMessage        [I] Message displayed in shutdown dialog box.
 *  dwTimeout        [I] Number of seconds dialog is displayed before shutdown.
 *  bForceAppsClosed [I] If TRUE, apps close without saving, else dialog is
 *                       displayed requesting user to close apps.
 *  bRebootAfterShutdown [I] If TRUE, system reboots after restart, else the
 *                           system flushes all caches to disk and clears
 *                           the screen
 *  dwReason [I] Reason for shutting down.  Must be a system shutdown reason
 *               code.
 *
 *  RETURNS
 *   Success: TRUE
 *   Failure: FALSE
 *
 *  NOTES
 *   if lpMachineName is NULL, the local computer is shutdown.
 */
BOOL WINAPI InitiateSystemShutdownExA( LPSTR lpMachineName, LPSTR lpMessage,
         DWORD dwTimeout, BOOL bForceAppsClosed, BOOL bRebootAfterShutdown,
         DWORD dwReason)
{
     FIXME("%s %s %d %d %d %d\n", debugstr_a(lpMachineName),
            debugstr_a(lpMessage), dwTimeout, bForceAppsClosed,
            bRebootAfterShutdown, dwReason);
     return TRUE;
} 

/******************************************************************************
 * InitiateSystemShutdownExW [ADVAPI32.@]
 *
 * See InitiateSystemShutdownExA.
 */
BOOL WINAPI InitiateSystemShutdownExW( LPWSTR lpMachineName, LPWSTR lpMessage,
         DWORD dwTimeout, BOOL bForceAppsClosed, BOOL bRebootAfterShutdown,
         DWORD dwReason)
{
     FIXME("%s %s %d %d %d %d\n", debugstr_w(lpMachineName),
            debugstr_w(lpMessage), dwTimeout, bForceAppsClosed,
            bRebootAfterShutdown, dwReason);
     return TRUE;
} 

BOOL WINAPI InitiateSystemShutdownA( LPSTR lpMachineName, LPSTR lpMessage, DWORD dwTimeout,
                                     BOOL bForceAppsClosed, BOOL bRebootAfterShutdown )
{
    return InitiateSystemShutdownExA( lpMachineName, lpMessage, dwTimeout,
                                      bForceAppsClosed, bRebootAfterShutdown,
                                      SHTDN_REASON_MAJOR_LEGACY_API );
}

BOOL WINAPI InitiateSystemShutdownW( LPWSTR lpMachineName, LPWSTR lpMessage, DWORD dwTimeout,
                                     BOOL bForceAppsClosed, BOOL bRebootAfterShutdown )
{
    return InitiateSystemShutdownExW( lpMachineName, lpMessage, dwTimeout,
                                      bForceAppsClosed, bRebootAfterShutdown,
                                      SHTDN_REASON_MAJOR_LEGACY_API );
}

BOOL WINAPI LogonUserA( LPCSTR lpszUsername, LPCSTR lpszDomain, LPCSTR lpszPassword,
                        DWORD dwLogonType, DWORD dwLogonProvider, PHANDLE phToken )
{
    WCHAR *usernameW = NULL, *domainW = NULL, *passwordW = NULL;
    BOOL ret = FALSE;

    TRACE("%s %s %p 0x%08x 0x%08x %p\n", debugstr_a(lpszUsername),
          debugstr_a(lpszDomain), lpszPassword, dwLogonType, dwLogonProvider, phToken);

    if (lpszUsername && !(usernameW = strdupAW( lpszUsername ))) return FALSE;
    if (lpszDomain && !(domainW = strdupAW( lpszUsername ))) goto done;
    if (lpszPassword && !(passwordW = strdupAW( lpszPassword ))) goto done;

    ret = LogonUserW( usernameW, domainW, passwordW, dwLogonType, dwLogonProvider, phToken );

done:
    heap_free( usernameW );
    heap_free( domainW );
    heap_free( passwordW );
    return ret;
}

BOOL WINAPI LogonUserW( LPCWSTR lpszUsername, LPCWSTR lpszDomain, LPCWSTR lpszPassword,
                        DWORD dwLogonType, DWORD dwLogonProvider, PHANDLE phToken )
{
    FIXME("%s %s %p 0x%08x 0x%08x %p - stub\n", debugstr_w(lpszUsername),
          debugstr_w(lpszDomain), lpszPassword, dwLogonType, dwLogonProvider, phToken);

    *phToken = (HANDLE *)0xdeadbeef;
    return TRUE;
}

typedef UINT (WINAPI *fnMsiProvideComponentFromDescriptor)(LPCWSTR,LPWSTR,DWORD*,DWORD*);

DWORD WINAPI CommandLineFromMsiDescriptor( WCHAR *szDescriptor,
                    WCHAR *szCommandLine, DWORD *pcchCommandLine )
{
    static const WCHAR szMsi[] = { 'm','s','i',0 };
    fnMsiProvideComponentFromDescriptor mpcfd;
    HMODULE hmsi;
    UINT r = ERROR_CALL_NOT_IMPLEMENTED;

    TRACE("%s %p %p\n", debugstr_w(szDescriptor), szCommandLine, pcchCommandLine);

    hmsi = LoadLibraryW( szMsi );
    if (!hmsi)
        return r;
    mpcfd = (fnMsiProvideComponentFromDescriptor)GetProcAddress( hmsi,
                                                                 "MsiProvideComponentFromDescriptorW" );
    if (mpcfd)
        r = mpcfd( szDescriptor, szCommandLine, pcchCommandLine, NULL );
    FreeLibrary( hmsi );
    return r;
}
