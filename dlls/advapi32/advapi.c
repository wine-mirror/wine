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
#include "wct.h"

#include "wine/debug.h"

#include "advapi32_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(advapi);

/******************************************************************************
 * GetUserNameA [ADVAPI32.@]
 */
BOOL WINAPI GetUserNameA( LPSTR name, LPDWORD size )
{
    DWORD len = GetEnvironmentVariableA( "WINEUSERNAME", name, *size );
    BOOL ret;

    if (!len) return FALSE;
    if ((ret = (len < *size))) len++;
    else SetLastError( ERROR_INSUFFICIENT_BUFFER );
    *size = len;
    return ret;
}

/******************************************************************************
 * GetUserNameW [ADVAPI32.@]
 */
BOOL WINAPI GetUserNameW( LPWSTR name, LPDWORD size )
{
    DWORD len = GetEnvironmentVariableW( L"WINEUSERNAME", name, *size );
    BOOL ret;

    if (!len) return FALSE;
    if ((ret = (len < *size))) len++;
    else SetLastError( ERROR_INSUFFICIENT_BUFFER );
    *size = len;
    return ret;
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
    TRACE("stub %s (harmless)\n", debugstr_a(lpMachineName));
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
     FIXME("%s %s %ld %d %d %#lx\n", debugstr_a(lpMachineName),
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
     FIXME("%s %s %ld %d %d %#lx\n", debugstr_w(lpMachineName),
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

/***********************************************************************
 *     InitiateShutdownA [ADVAPI32.@]
 */
DWORD WINAPI InitiateShutdownA(char *name, char *message, DWORD seconds, DWORD flags, DWORD reason)
{
    FIXME("%s, %s, %ld, %ld, %ld stub\n", debugstr_a(name), debugstr_a(message), seconds, flags, reason);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *     InitiateShutdownW [ADVAPI32.@]
 */
DWORD WINAPI InitiateShutdownW(WCHAR *name, WCHAR *message, DWORD seconds, DWORD flags, DWORD reason)
{
    FIXME("%s, %s, %ld, %ld, %ld stub\n", debugstr_w(name), debugstr_w(message), seconds, flags, reason);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

BOOL WINAPI LogonUserA( LPCSTR lpszUsername, LPCSTR lpszDomain, LPCSTR lpszPassword,
                        DWORD dwLogonType, DWORD dwLogonProvider, PHANDLE phToken )
{
    WCHAR *usernameW = NULL, *domainW = NULL, *passwordW = NULL;
    BOOL ret = FALSE;

    TRACE("%s %s %p 0x%08lx 0x%08lx %p\n", debugstr_a(lpszUsername),
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
    FIXME("%s %s %p 0x%08lx 0x%08lx %p - stub\n", debugstr_w(lpszUsername),
          debugstr_w(lpszDomain), lpszPassword, dwLogonType, dwLogonProvider, phToken);

    *phToken = (HANDLE *)0xdeadbeef;
    return TRUE;
}

typedef UINT (WINAPI *fnMsiProvideComponentFromDescriptor)(LPCWSTR,LPWSTR,DWORD*,DWORD*);

DWORD WINAPI CommandLineFromMsiDescriptor( WCHAR *szDescriptor,
                    WCHAR *szCommandLine, DWORD *pcchCommandLine )
{
    fnMsiProvideComponentFromDescriptor mpcfd;
    HMODULE hmsi;
    UINT r = ERROR_CALL_NOT_IMPLEMENTED;

    TRACE("%s %p %p\n", debugstr_w(szDescriptor), szCommandLine, pcchCommandLine);

    hmsi = LoadLibraryW( L"msi" );
    if (!hmsi)
        return r;
    mpcfd = (fnMsiProvideComponentFromDescriptor)GetProcAddress( hmsi,
                                                                 "MsiProvideComponentFromDescriptorW" );
    if (mpcfd)
        r = mpcfd( szDescriptor, szCommandLine, pcchCommandLine, NULL );
    FreeLibrary( hmsi );
    return r;
}

/***********************************************************************
 *      RegisterWaitChainCOMCallback (ole32.@)
 */
void WINAPI RegisterWaitChainCOMCallback(PCOGETCALLSTATE call_state_cb,
                                         PCOGETACTIVATIONSTATE activation_state_cb)
{
    FIXME("%p, %p\n", call_state_cb, activation_state_cb);
}
