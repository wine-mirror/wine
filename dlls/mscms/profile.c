/*
 * MSCMS - Color Management System for Wine
 *
 * Copyright 2004 Hans Leidekker
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
#include "wine/debug.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "icm.h"

#define LCMS_API_FUNCTION(f) extern typeof(f) * p##f;
#include "lcms_api.h"
#undef LCMS_API_FUNCTION

#define IS_SEPARATOR(ch)  ((ch) == '\\' || (ch) == '/')

static void MSCMS_basename( LPCWSTR path, LPWSTR name )
{
    INT i = lstrlenW( path );

    while (i > 0 && !IS_SEPARATOR(path[i - 1])) i--;
    lstrcpyW( name, &path[i] );
}

WINE_DEFAULT_DEBUG_CHANNEL(mscms);

BOOL WINAPI GetColorDirectoryA( PCSTR machine, PSTR buffer, PDWORD size )
{
    INT len;
    LPWSTR bufferW;
    BOOL ret = FALSE;
    DWORD sizeW = *size * sizeof(WCHAR);

    TRACE( "( %p, %ld )\n", buffer, *size );

    if (machine || !buffer) return FALSE;

    bufferW = HeapAlloc( GetProcessHeap(), 0, sizeW );

    if (bufferW)
    {
        ret = GetColorDirectoryW( NULL, bufferW, &sizeW );
        *size = sizeW / sizeof(WCHAR);

        if (ret)
        {
            len = WideCharToMultiByte( CP_ACP, 0, bufferW, *size, buffer, *size, NULL, NULL );
            if (!len) ret = FALSE;
        }

        HeapFree( GetProcessHeap(), 0, bufferW );
    }
    return ret;
}

BOOL WINAPI GetColorDirectoryW( PCWSTR machine, PWSTR buffer, PDWORD size )
{
    /* FIXME: Get this directory from the registry? */
    static const WCHAR colordir[] =
    { 'c',':','\\','w','i','n','d','o','w','s','\\', 's','y','s','t','e','m','3','2',
      '\\','s','p','o','o','l','\\','d','r','i','v','e','r','s','\\','c','o','l','o','r',0 };

    DWORD len;

    TRACE( "( %p, %ld )\n", buffer, *size );

    if (machine || !buffer) return FALSE;

    len = lstrlenW( colordir ) * sizeof(WCHAR);

    if (len <= *size)
    {
        lstrcpyW( buffer, colordir );
        return TRUE;
    }

    *size = len;
    return FALSE;
}

BOOL WINAPI InstallColorProfileA( PCSTR machine, PCSTR profile )
{
    UINT len;
    LPWSTR profileW;
    BOOL ret = FALSE;

    TRACE( "( %s )\n", debugstr_a(profile) );

    if (machine || !profile) return FALSE;

    len = MultiByteToWideChar( CP_ACP, 0, profile, -1, NULL, 0 );
    profileW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );

    if (profileW)
    {
        MultiByteToWideChar( CP_ACP, 0, profile, -1, profileW, len );

        ret = InstallColorProfileW( NULL, profileW );
        HeapFree( GetProcessHeap(), 0, profileW );
    }
    return ret;
}

BOOL WINAPI InstallColorProfileW( PCWSTR machine, PCWSTR profile )
{
    WCHAR dest[MAX_PATH], base[MAX_PATH];
    DWORD size = sizeof(dest);
    static const WCHAR slash[] = { '\\', 0 };

    TRACE( "( %s )\n", debugstr_w(profile) );

    if (machine || !profile) return FALSE;

    if (!GetColorDirectoryW( machine, dest, &size )) return FALSE;

    MSCMS_basename( profile, base );

    lstrcatW( dest, slash );
    lstrcatW( dest, base );

    /* Is source equal to destination? */
    if (!lstrcmpW( profile, dest )) return TRUE;

    return CopyFileW( profile, dest, TRUE );
}

BOOL WINAPI UninstallColorProfileA( PCSTR machine, PCSTR profile, BOOL delete )
{
    UINT len;
    LPWSTR profileW;
    BOOL ret = FALSE;

    TRACE( "( %s )\n", debugstr_a(profile) );

    if (machine || !profile) return FALSE;

    len = MultiByteToWideChar( CP_ACP, 0, profile, -1, NULL, 0 );
    profileW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );

    if (profileW)
    {
        MultiByteToWideChar( CP_ACP, 0, profile, -1, profileW, len );

        ret = UninstallColorProfileW( NULL, profileW , delete );

        HeapFree( GetProcessHeap(), 0, profileW );
    }
    return ret;
}

BOOL WINAPI UninstallColorProfileW( PCWSTR machine, PCWSTR profile, BOOL delete )
{
    if (machine || !profile) return FALSE;

    if (delete)
        return DeleteFileW( profile );

    return TRUE;
}

HPROFILE WINAPI OpenColorProfileA( PPROFILE profile, DWORD access, DWORD sharing, DWORD creation )
{
    HPROFILE handle = NULL;

    TRACE( "( %p, %lx, %lx, %lx )\n", profile, access, sharing, creation );

    if (!profile || !profile->pProfileData) return NULL;

    /* No AW conversion needed for memory based profiles */
    if (profile->dwType & PROFILE_MEMBUFFER)
        return OpenColorProfileW( profile, access, sharing, creation );

    if (profile->dwType & PROFILE_FILENAME)
    {
        UINT len;
        PROFILE profileW;

        profileW.dwType = profile->dwType;
 
        len = MultiByteToWideChar( CP_ACP, 0, profile->pProfileData, -1, NULL, 0 );
        profileW.pProfileData = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );

        if (profileW.pProfileData)
        {
            profileW.cbDataSize = len * sizeof(WCHAR);
            MultiByteToWideChar( CP_ACP, 0, profile->pProfileData, -1, profileW.pProfileData, len );

            handle = OpenColorProfileW( &profileW, access, sharing, creation );
            HeapFree( GetProcessHeap(), 0, profileW.pProfileData );
        }
    }
    return handle;
}

/******************************************************************************
 * OpenColorProfileW               [MSCMS.@]
 *
 * Open a color profile.
 *
 * PARAMS
 *  profile   [I] Pointer to a color profile structure
 *  access    [I] Desired access
 *  sharing   [I] Sharing mode
 *  creation  [I] Creation mode
 *
 * RETURNS
 *  Success: Handle to the opened profile
 *  Failure: NULL
 *
 * NOTES
 *  Values for access:   PROFILE_READ or PROFILE_READWRITE
 *  Values for sharing:  0 (no sharing), FILE_SHARE_READ and/or FILE_SHARE_WRITE 
 *  Values for creation: one of CREATE_NEW, CREATE_ALWAYS, OPEN_EXISTING,
 *                       OPEN_ALWAYS, TRUNCATE_EXISTING.
 */

HPROFILE WINAPI OpenColorProfileW( PPROFILE profile, DWORD access, DWORD sharing, DWORD creation )
{
#ifdef HAVE_LCMS_H
    cmsHPROFILE cmsprofile = NULL;
    HANDLE handle = NULL;

    TRACE( "( %p, %lx, %lx, %lx )\n", profile, access, sharing, creation );

    if (!profile || !profile->pProfileData) return NULL;

    if (profile->dwType & PROFILE_MEMBUFFER)
    {
        FIXME( "Memory based profile not yet supported.\n" ); return NULL;
    }

    if (profile->dwType & PROFILE_FILENAME)
    {
        char *unixname;
        DWORD flags = 0;

        TRACE("profile file: %s\n", debugstr_w( (WCHAR *)profile->pProfileData ));

        if (access & PROFILE_READ) flags = GENERIC_READ;
        if (access & PROFILE_READWRITE) flags = GENERIC_READ|GENERIC_WRITE;

        if (!flags) return NULL;

        handle = CreateFileW( profile->pProfileData, flags, sharing, NULL, creation, 0, NULL );
        if (handle == INVALID_HANDLE_VALUE) return NULL;

        unixname = wine_get_unix_file_name( (WCHAR *)profile->pProfileData );

        if (unixname)
        {
            cmsprofile = cmsOpenProfileFromFile( unixname, flags & GENERIC_READ ? "r" : "w" );
            HeapFree( GetProcessHeap(), 0, unixname );
        }
    }

    if (cmsprofile) return MSCMS_create_hprofile_handle( handle, cmsprofile );

#endif /* HAVE_LCMS_H */
    return NULL;
}

/******************************************************************************
 * CloseColorProfile               [MSCMS.@]
 *
 * Close a color profile.
 *
 * PARAMS
 *  profile [I] Handle to the profile
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI CloseColorProfile( HPROFILE profile )
{
    BOOL ret1, ret2 = FALSE;

    TRACE( "( %p )\n", profile );

#ifdef HAVE_LCMS_H
    ret1 = cmsCloseProfile( MSCMS_hprofile2cmsprofile( profile ) );
    ret2 = CloseHandle( MSCMS_hprofile2handle( profile ) );

    MSCMS_destroy_hprofile_handle( profile );

#endif /* HAVE_LCMS_H */
    return ret1 && ret2;
}
