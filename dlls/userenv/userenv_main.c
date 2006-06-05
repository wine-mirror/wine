/*
 * Implementation of userenv.dll
 *
 * Copyright 2006 Mike McCormack for CodeWeavers
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
#include "winreg.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL( userenv );

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p %ld %p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

BOOL WINAPI CreateEnvironmentBlock( LPVOID* lpEnvironment,
                     HANDLE hToken, BOOL bInherit )
{
    FIXME("%p %p %d\n", lpEnvironment, hToken, bInherit );
    return FALSE;
}

BOOL WINAPI GetUserProfileDirectoryA( HANDLE hToken, LPSTR lpProfileDir,
                     LPDWORD lpcchSize )
{
    FIXME("%p %p %p\n", hToken, lpProfileDir, lpcchSize );
    return FALSE;
}

BOOL WINAPI GetUserProfileDirectoryW( HANDLE hToken, LPWSTR lpProfileDir,
                     LPDWORD lpcchSize )
{
    FIXME("%p %p %p\n", hToken, lpProfileDir, lpcchSize );
    return FALSE;
}

BOOL WINAPI GetProfilesDirectoryA( LPSTR lpProfilesDir, LPDWORD lpcchSize )
{
    FIXME("%p %p\n", lpProfilesDir, lpcchSize );
    return FALSE;
}

BOOL WINAPI GetProfilesDirectoryW( LPWSTR lpProfilesDir, LPDWORD lpcchSize )
{
    FIXME("%p %p\n", lpProfilesDir, lpcchSize );
    return FALSE;
}

typedef struct _PROFILEINFOA {
    DWORD dwSize;
    DWORD dwFlags;
    LPSTR lpUserName;
    LPSTR lpProfilePath;
    LPSTR lpDefaultPath;
    LPSTR lpServerName;
    LPSTR lpPolicyPath;
    HANDLE hProfile;
} PROFILEINFOA, *LPPROFILEINFOA;

BOOL WINAPI LoadUserProfileA( HANDLE hToken, LPPROFILEINFOA lpProfileInfo )
{
    FIXME("%p %p\n", hToken, lpProfileInfo );
    lpProfileInfo->hProfile = HKEY_CURRENT_USER;
    return TRUE;
}
