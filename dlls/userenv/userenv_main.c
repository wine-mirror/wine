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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "userenv.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL( userenv );

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p %d %p\n", hinstDLL, fdwReason, lpvReserved);

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
    NTSTATUS r;

    TRACE("%p %p %d\n", lpEnvironment, hToken, bInherit );

    if (!lpEnvironment)
        return FALSE;

    r = RtlCreateEnvironment(bInherit, (WCHAR **)lpEnvironment);
    if (r == STATUS_SUCCESS)
        return TRUE;
    return FALSE;
}

BOOL WINAPI DestroyEnvironmentBlock(LPVOID lpEnvironment)
{
    NTSTATUS r;

    TRACE("%p\n", lpEnvironment);
    r = RtlDestroyEnvironment(lpEnvironment);
    if (r == STATUS_SUCCESS)
        return TRUE;
    return FALSE;
}

BOOL WINAPI ExpandEnvironmentStringsForUserA( HANDLE hToken, LPCSTR lpSrc,
                     LPSTR lpDest, DWORD dwSize )
{
    BOOL ret;

    TRACE("%p %s %p %d\n", hToken, debugstr_a(lpSrc), lpDest, dwSize);

    ret = ExpandEnvironmentStringsA( lpSrc, lpDest, dwSize );
    TRACE("<- %s\n", debugstr_a(lpDest));
    return ret;
}

BOOL WINAPI ExpandEnvironmentStringsForUserW( HANDLE hToken, LPCWSTR lpSrc,
                     LPWSTR lpDest, DWORD dwSize )
{
    BOOL ret;

    TRACE("%p %s %p %d\n", hToken, debugstr_w(lpSrc), lpDest, dwSize);

    ret = ExpandEnvironmentStringsW( lpSrc, lpDest, dwSize );
    TRACE("<- %s\n", debugstr_w(lpDest));
    return ret;
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

static const char ProfileListA[] = "Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList";

BOOL WINAPI GetProfilesDirectoryA( LPSTR lpProfilesDir, LPDWORD lpcchSize )
{
    static const char ProfilesDirectory[] = "ProfilesDirectory";
    LONG l;
    HKEY key;
    BOOL ret = FALSE;
    DWORD len = 0, expanded_len;
    LPSTR unexpanded_profiles_dir = NULL;

    TRACE("%p %p\n", lpProfilesDir, lpcchSize );

    if (!lpProfilesDir || !lpcchSize)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    l = RegOpenKeyExA(HKEY_LOCAL_MACHINE, ProfileListA, 0, KEY_READ, &key);
    if (l)
    {
        SetLastError(l);
        return FALSE;
    }
    l = RegQueryValueExA(key, ProfilesDirectory, NULL, NULL, NULL, &len);
    if (l)
    {
        SetLastError(l);
        goto end;
    }
    unexpanded_profiles_dir = HeapAlloc(GetProcessHeap(), 0, len);
    if (!unexpanded_profiles_dir)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto end;
    }
    l = RegQueryValueExA(key, ProfilesDirectory, NULL, NULL,
                             (BYTE *)unexpanded_profiles_dir, &len);
    if (l)
    {
        SetLastError(l);
        goto end;
    }
    expanded_len = ExpandEnvironmentStringsA(unexpanded_profiles_dir, NULL, 0);
    /* The returned length doesn't include the NULL terminator. */
    if (*lpcchSize < expanded_len - 1)
    {
        *lpcchSize = expanded_len - 1;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto end;
    }
    *lpcchSize = expanded_len - 1;
    /* The return value is also the expected length. */
    ret = ExpandEnvironmentStringsA(unexpanded_profiles_dir, lpProfilesDir,
                                    expanded_len) - 1;
end:
    HeapFree(GetProcessHeap(), 0, unexpanded_profiles_dir);
    RegCloseKey(key);
    return ret;
}

static const WCHAR ProfileListW[] = {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s',' ','N','T','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\','P','r','o','f','i','l','e','L','i','s','t',0};

BOOL WINAPI GetProfilesDirectoryW( LPWSTR lpProfilesDir, LPDWORD lpcchSize )
{
    static const WCHAR ProfilesDirectory[] = {'P','r','o','f','i','l','e','s','D','i','r','e','c','t','o','r','y',0};
    LONG l;
    HKEY key;
    BOOL ret = FALSE;
    DWORD len = 0, expanded_len;
    LPWSTR unexpanded_profiles_dir = NULL;

    TRACE("%p %p\n", lpProfilesDir, lpcchSize );

    if (!lpProfilesDir || !lpcchSize)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    l = RegOpenKeyExW(HKEY_LOCAL_MACHINE, ProfileListW, 0, KEY_READ, &key);
    if (l)
    {
        SetLastError(l);
        return FALSE;
    }
    l = RegQueryValueExW(key, ProfilesDirectory, NULL, NULL, NULL, &len);
    if (l)
    {
        SetLastError(l);
        goto end;
    }
    unexpanded_profiles_dir = HeapAlloc(GetProcessHeap(), 0, len);
    if (!unexpanded_profiles_dir)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto end;
    }
    l = RegQueryValueExW(key, ProfilesDirectory, NULL, NULL,
                             (BYTE *)unexpanded_profiles_dir, &len);
    if (l)
    {
        SetLastError(l);
        goto end;
    }
    expanded_len = ExpandEnvironmentStringsW(unexpanded_profiles_dir, NULL, 0);
    /* The returned length doesn't include the NULL terminator. */
    if (*lpcchSize < expanded_len - 1)
    {
        *lpcchSize = expanded_len - 1;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto end;
    }
    *lpcchSize = expanded_len - 1;
    /* The return value is also the expected length. */
    ret = ExpandEnvironmentStringsW(unexpanded_profiles_dir, lpProfilesDir,
                                    expanded_len) - 1;
end:
    HeapFree(GetProcessHeap(), 0, unexpanded_profiles_dir);
    RegCloseKey(key);
    return ret;
}

BOOL WINAPI GetAllUsersProfileDirectoryA( LPSTR lpProfileDir, LPDWORD lpcchSize )
{
    FIXME("%p %p\n", lpProfileDir, lpcchSize);
    return FALSE;
}

BOOL WINAPI GetAllUsersProfileDirectoryW( LPWSTR lpProfileDir, LPDWORD lpcchSize )
{
    FIXME("%p %p\n", lpProfileDir, lpcchSize);
    return FALSE;
}

BOOL WINAPI GetProfileType( DWORD *pdwFlags )
{
    FIXME("%p\n", pdwFlags );
    *pdwFlags = 0;
    return TRUE;
}

BOOL WINAPI LoadUserProfileA( HANDLE hToken, LPPROFILEINFOA lpProfileInfo )
{
    FIXME("%p %p\n", hToken, lpProfileInfo );
    lpProfileInfo->hProfile = HKEY_CURRENT_USER;
    return TRUE;
}

BOOL WINAPI LoadUserProfileW( HANDLE hToken, LPPROFILEINFOW lpProfileInfo )
{
    FIXME("%p %p\n", hToken, lpProfileInfo );
    lpProfileInfo->hProfile = HKEY_CURRENT_USER;
    return TRUE;
}

BOOL WINAPI RegisterGPNotification( HANDLE event, BOOL machine )
{
    FIXME("%p %d\n", event, machine );
    return TRUE;
}

BOOL WINAPI UnregisterGPNotification( HANDLE event )
{
    FIXME("%p\n", event );
    return TRUE;
}

BOOL WINAPI UnloadUserProfile( HANDLE hToken, HANDLE hProfile )
{
    FIXME("(%p, %p): stub\n", hToken, hProfile);
    return FALSE;
}

/******************************************************************************
 *              USERENV.138
 *
 * Create .lnk file
 *
 * PARAMETERS
 *   int     csidl               [in] well-known directory location to create link in
 *   LPCSTR lnk_dir              [in] directory (relative to directory specified by csidl) to create link in
 *   LPCSTR lnk_filename         [in] filename of the link file without .lnk extension
 *   LPCSTR lnk_target           [in] file/directory pointed to by link
 *   LPCSTR lnk_iconfile         [in] link icon resource filename
 *   DWORD   lnk_iconid          [in] link icon resource id in file referred by lnk_iconfile
 *   LPCSTR work_directory       [in] link target's work directory
 *   WORD    hotkey              [in] link hotkey (virtual key id)
 *   DWORD   win_state           [in] initial window size (SW_SHOWMAXIMIZED to start maximized,
 *                                     SW_SHOWMINNOACTIVE to start minimized, everything else is default state)
 *   LPCSTR comment              [in] comment - link's comment
 *   LPCSTR loc_filename_resfile [in] resource file which holds localized filename for this link file
 *   DWORD   loc_filename_resid  [in] resource id for this link file's localized filename
 *
 * RETURNS
 *    TRUE:  Link file was successfully created
 *    FALSE: Link file was not created
 */
BOOL WINAPI USERENV_138( int csidl, LPCSTR lnk_dir, LPCSTR lnk_filename,
            LPCSTR lnk_target, LPCSTR lnk_iconfile, DWORD lnk_iconid,
            LPCSTR work_directory, WORD hotkey, DWORD win_state, LPCSTR comment,
            LPCSTR loc_filename_resfile, DWORD loc_filename_resid)
{
    FIXME("(%d,%s,%s,%s,%s,%d,%s,0x%x,%d,%s,%s,%d) - stub\n", csidl, debugstr_a(lnk_dir),
            debugstr_a(lnk_filename), debugstr_a(lnk_target), debugstr_a(lnk_iconfile),
            lnk_iconid, debugstr_a(work_directory), hotkey, win_state,
            debugstr_a(comment), debugstr_a(loc_filename_resfile), loc_filename_resid );

    return FALSE;
}
