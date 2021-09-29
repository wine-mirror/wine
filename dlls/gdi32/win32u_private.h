/*
 * Unix call wrappers
 *
 * Copyright 2021 Jacek Caban for CodeWeavers
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

#ifndef __WINE_WIN32U_PRIVATE
#define __WINE_WIN32U_PRIVATE

#include "wine/gdi_driver.h"
#include "winuser.h"

struct user_callbacks
{
    HWND (WINAPI *pGetDesktopWindow)(void);
    UINT (WINAPI *pGetDpiForSystem)(void);
    BOOL (WINAPI *pGetMonitorInfoW)( HMONITOR, LPMONITORINFO );
    INT (WINAPI *pGetSystemMetrics)(INT);
    BOOL (WINAPI *pGetWindowRect)( HWND hwnd, LPRECT rect );
    BOOL (WINAPI *pEnumDisplayMonitors)( HDC, LPRECT, MONITORENUMPROC, LPARAM );
    BOOL (WINAPI *pEnumDisplaySettingsW)(LPCWSTR, DWORD, LPDEVMODEW );
    BOOL (WINAPI *pRedrawWindow)( HWND, const RECT*, HRGN, UINT );
    DPI_AWARENESS_CONTEXT (WINAPI *pSetThreadDpiAwarenessContext)( DPI_AWARENESS_CONTEXT );
    HWND (WINAPI *pWindowFromDC)( HDC );
};

static inline WCHAR *win32u_wcsrchr( const WCHAR *str, WCHAR ch )
{
    WCHAR *ret = NULL;
    do { if (*str == ch) ret = (WCHAR *)(ULONG_PTR)str; } while (*str++);
    return ret;
}

static inline WCHAR win32u_towupper( WCHAR ch )
{
    return RtlUpcaseUnicodeChar( ch );
}

static inline WCHAR *win32u_wcschr( const WCHAR *str, WCHAR ch )
{
    do { if (*str == ch) return (WCHAR *)(ULONG_PTR)str; } while (*str++);
    return NULL;
}

static inline int win32u_wcsicmp( const WCHAR *str1, const WCHAR *str2 )
{
    int ret;
    for (;;)
    {
        if ((ret = win32u_towupper( *str1 ) - win32u_towupper( *str2 )) || !*str1) return ret;
        str1++;
        str2++;
    }
}

static inline int win32u_wcscmp( const WCHAR *str1, const WCHAR *str2 )
{
    while (*str1 && (*str1 == *str2)) { str1++; str2++; }
    return *str1 - *str2;
}

static inline LONG win32u_wcstol( LPCWSTR s, LPWSTR *end, INT base )
{
    BOOL negative = FALSE, empty = TRUE;
    LONG ret = 0;

    if (base < 0 || base == 1 || base > 36) return 0;
    if (end) *end = (WCHAR *)s;
    while (*s == ' ' || *s == '\t') s++;

    if (*s == '-')
    {
        negative = TRUE;
        s++;
    }
    else if (*s == '+') s++;

    if ((base == 0 || base == 16) && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
    {
        base = 16;
        s += 2;
    }
    if (base == 0) base = s[0] != '0' ? 10 : 8;

    while (*s)
    {
        int v;

        if ('0' <= *s && *s <= '9') v = *s - '0';
        else if ('A' <= *s && *s <= 'Z') v = *s - 'A' + 10;
        else if ('a' <= *s && *s <= 'z') v = *s - 'a' + 10;
        else break;
        if (v >= base) break;
        if (negative) v = -v;
        s++;
        empty = FALSE;

        if (!negative && (ret > MAXLONG / base || ret * base > MAXLONG - v))
            ret = MAXLONG;
        else if (negative && (ret < (LONG)MINLONG / base || ret * base < (LONG)(MINLONG - v)))
            ret = MINLONG;
        else
            ret = ret * base + v;
    }

    if (end && !empty) *end = (WCHAR *)s;
    return ret;
}

#define towupper(c)     win32u_towupper(c)
#define wcschr(s,c)     win32u_wcschr(s,c)
#define wcscmp(s1,s2)   win32u_wcscmp(s1,s2)
#define wcsicmp(s1,s2)  win32u_wcsicmp(s1,s2)
#define wcsrchr(s,c)    win32u_wcsrchr(s,c)
#define wcstol(s,e,b)   win32u_wcstol(s,e,b)

#endif /* __WINE_WIN32U_PRIVATE */
