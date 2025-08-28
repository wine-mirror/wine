/*
 * Image Color Management
 *
 * Copyright 2004 Marcus Meissner
 * Copyright 2008 Hans Leidekker
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

#include "gdi_private.h"
#include "winnls.h"
#include "winreg.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(icm);

static const WCHAR color_path[]      = L"c:\\windows\\system32\\spool\\drivers\\color\\";
static const WCHAR default_profile[] = L"c:\\windows\\system32\\spool\\drivers\\color\\sRGB Color Space Profile.icm";

struct enum_profiles
{
    ICMENUMPROCA funcA;
    LPARAM data;
};

static INT CALLBACK enum_profiles_callbackA( LPWSTR filename, LPARAM lparam )
{
    int len, ret = -1;
    struct enum_profiles *ep = (struct enum_profiles *)lparam;
    char *filenameA;

    len = WideCharToMultiByte( CP_ACP, 0, filename, -1, NULL, 0, NULL, NULL );
    filenameA = HeapAlloc( GetProcessHeap(), 0, len );
    if (filenameA)
    {
        WideCharToMultiByte( CP_ACP, 0, filename, -1, filenameA, len, NULL, NULL );
        ret = ep->funcA( filenameA, ep->data );
        HeapFree( GetProcessHeap(), 0, filenameA );
    }
    return ret;
}

/***********************************************************************
 *           EnumICMProfilesA    (GDI32.@)
 */
INT WINAPI EnumICMProfilesA(HDC hdc, ICMENUMPROCA func, LPARAM lparam)
{
    struct enum_profiles ep;

    if (!func) return -1;
    ep.funcA = func;
    ep.data  = lparam;
    return EnumICMProfilesW( hdc, enum_profiles_callbackA, (LPARAM)&ep );
}

/***********************************************************************
 *           EnumICMProfilesW    (GDI32.@)
 */
INT WINAPI EnumICMProfilesW(HDC hdc, ICMENUMPROCW func, LPARAM lparam)
{
    WCHAR profile[MAX_PATH];
    DWORD size = ARRAYSIZE(profile);

    TRACE( "%p, %p, 0x%08Ix\n", hdc, func, lparam );

    if (!func) return -1;
    if (!GetICMProfileW( hdc, &size, profile )) return -1;
    if (!wcsicmp( profile, default_profile )) return -1;
    /* FIXME: support multiple profiles */
    return func( profile, lparam );
}

/**********************************************************************
 *           GetICMProfileA   (GDI32.@)
 *
 * Returns the filename of the specified device context's color
 * management profile, even if color management is not enabled
 * for that DC.
 *
 * RETURNS
 *    TRUE if filename is copied successfully.
 *    FALSE if the buffer length pointed to by size is too small.
 *
 * FIXME
 *    How does Windows assign these? Some registry key?
 */
BOOL WINAPI GetICMProfileA(HDC hdc, LPDWORD size, LPSTR filename)
{
    WCHAR filenameW[MAX_PATH];
    DWORD buflen = MAX_PATH;
    BOOL ret = FALSE;

    TRACE("%p, %p, %p\n", hdc, size, filename);

    if (!hdc || !size) return FALSE;

    if (GetICMProfileW(hdc, &buflen, filenameW))
    {
        int len = WideCharToMultiByte(CP_ACP, 0, filenameW, -1, NULL, 0, NULL, NULL);

        if (!filename)
        {
            *size = len;
            return FALSE;
        }

        if (*size >= len)
        {
            WideCharToMultiByte(CP_ACP, 0, filenameW, -1, filename, *size, NULL, NULL);
            ret = TRUE;
        }
        else SetLastError(ERROR_INSUFFICIENT_BUFFER);
        *size = len;
    }
    return ret;
}

/**********************************************************************
 *           GetICMProfileW     (GDI32.@)
 */
BOOL WINAPI GetICMProfileW(HDC hdc, LPDWORD size, LPWSTR filename)
{
    DC_ATTR *dc_attr;
    WCHAR fullname[MAX_PATH + ARRAY_SIZE(color_path)];
    DWORD len = MAX_PATH;
    HKEY hkey = 0;
    BOOL ret = FALSE;

    TRACE("%p, %p, %p\n", hdc, size, filename);

    if (!size) return FALSE;
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;

    if (!RegOpenKeyW( HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ICM\\mntr", &hkey ))
    {
        wcscpy( fullname, color_path );
        /* FIXME handle multiple values */
        if (!RegEnumValueW( hkey, 0, fullname + wcslen(fullname), &len, NULL, NULL, NULL, NULL ))
            goto found;
    }

    if (!__wine_get_icm_profile( hdc, &len, fullname )) wcscpy( fullname, default_profile );

 found:
    len = wcslen(fullname) + 1;
    if (*size >= len)
    {
        if (filename) wcscpy( filename, fullname );
        ret = TRUE;
    }
    else SetLastError( ERROR_INSUFFICIENT_BUFFER );

    *size = len;
    RegCloseKey( hkey );
    return ret;
}

/**********************************************************************
 *           GetLogColorSpaceA     (GDI32.@)
 */
BOOL WINAPI GetLogColorSpaceA(HCOLORSPACE colorspace, LPLOGCOLORSPACEA buffer, DWORD size)
{
    FIXME("%p %p 0x%08lx stub\n", colorspace, buffer, size);
    return FALSE;
}

/**********************************************************************
 *           GetLogColorSpaceW      (GDI32.@)
 */
BOOL WINAPI GetLogColorSpaceW(HCOLORSPACE colorspace, LPLOGCOLORSPACEW buffer, DWORD size)
{
    FIXME("%p %p 0x%08lx stub\n", colorspace, buffer, size);
    return FALSE;
}

/**********************************************************************
 *           SetICMProfileA         (GDI32.@)
 */
BOOL WINAPI SetICMProfileA(HDC hdc, LPSTR filename)
{
    FIXME("%p %s stub\n", hdc, debugstr_a(filename));

    if (!filename)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!hdc)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    return TRUE;
}

/**********************************************************************
 *           SetICMProfileW         (GDI32.@)
 */
BOOL WINAPI SetICMProfileW(HDC hdc, LPWSTR filename)
{
    FIXME("%p %s stub\n", hdc, debugstr_w(filename));

    if (!filename)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!hdc)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    return TRUE;
}

/**********************************************************************
 *           UpdateICMRegKeyA       (GDI32.@)
 */
BOOL WINAPI UpdateICMRegKeyA(DWORD reserved, LPSTR cmid, LPSTR filename, UINT command)
{
    FIXME("0x%08lx, %s, %s, 0x%08x stub\n", reserved, debugstr_a(cmid), debugstr_a(filename), command);
    return TRUE;
}

/**********************************************************************
 *           UpdateICMRegKeyW       (GDI32.@)
 */
BOOL WINAPI UpdateICMRegKeyW(DWORD reserved, LPWSTR cmid, LPWSTR filename, UINT command)
{
    FIXME("0x%08lx, %s, %s, 0x%08x stub\n", reserved, debugstr_w(cmid), debugstr_w(filename), command);
    return TRUE;
}
