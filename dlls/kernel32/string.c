/*
 * Kernel string functions
 *
 * Copyright 1993 Yngvi Sigurjonsson
 * Copyright 1996 Alexandre Julliard
 * Copyright 2001 Dmitry Timoshkov for CodeWeavers
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

#include "config.h"
#include "wine/port.h"

#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#define WINE_NO_INLINE_STRING
#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "wine/unicode.h"
#include "wine/exception.h"


static INT (WINAPI *pLoadStringA)(HINSTANCE, UINT, LPSTR, INT);
static INT (WINAPI *pwvsprintfA)(LPSTR, LPCSTR, va_list);


/***********************************************************************
 * Helper for k32 family functions
 */
static void *user32_proc_address(const char *proc_name)
{
    static HMODULE hUser32;

    if(!hUser32) hUser32 = LoadLibraryA("user32.dll");
    return GetProcAddress(hUser32, proc_name);
}


/***********************************************************************
 *		k32CharToOemBuffA   (KERNEL32.11)
 */
BOOL WINAPI k32CharToOemBuffA(LPCSTR s, LPSTR d, DWORD len)
{
    WCHAR *bufW;

    if ((bufW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
    {
        MultiByteToWideChar( CP_ACP, 0, s, len, bufW, len );
        WideCharToMultiByte( CP_OEMCP, 0, bufW, len, d, len, NULL, NULL );
        HeapFree( GetProcessHeap(), 0, bufW );
    }
    return TRUE;
}


/***********************************************************************
 *		k32CharToOemA   (KERNEL32.10)
 */
BOOL WINAPI k32CharToOemA(LPCSTR s, LPSTR d)
{
    if (!s || !d) return TRUE;
    return k32CharToOemBuffA( s, d, strlen(s) + 1 );
}


/***********************************************************************
 *		k32OemToCharBuffA   (KERNEL32.13)
 */
BOOL WINAPI k32OemToCharBuffA(LPCSTR s, LPSTR d, DWORD len)
{
    WCHAR *bufW;

    if ((bufW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
    {
        MultiByteToWideChar( CP_OEMCP, 0, s, len, bufW, len );
        WideCharToMultiByte( CP_ACP, 0, bufW, len, d, len, NULL, NULL );
        HeapFree( GetProcessHeap(), 0, bufW );
    }
    return TRUE;
}


/***********************************************************************
 *		k32OemToCharA   (KERNEL32.12)
 */
BOOL WINAPI k32OemToCharA(LPCSTR s, LPSTR d)
{
    return k32OemToCharBuffA( s, d, strlen(s) + 1 );
}


/**********************************************************************
 *		k32LoadStringA   (KERNEL32.14)
 */
INT WINAPI k32LoadStringA(HINSTANCE instance, UINT resource_id,
                          LPSTR buffer, INT buflen)
{
    if(!pLoadStringA) pLoadStringA = user32_proc_address("LoadStringA");
    return pLoadStringA(instance, resource_id, buffer, buflen);
}


/***********************************************************************
 *		k32wvsprintfA   (KERNEL32.16)
 */
INT WINAPI k32wvsprintfA(LPSTR buffer, LPCSTR spec, va_list args)
{
    if(!pwvsprintfA) pwvsprintfA = user32_proc_address("wvsprintfA");
    return (*pwvsprintfA)(buffer, spec, args);
}


/***********************************************************************
 *		k32wsprintfA   (KERNEL32.15)
 */
INT WINAPIV k32wsprintfA(LPSTR buffer, LPCSTR spec, ...)
{
    va_list args;
    INT res;

    va_start(args, spec);
    res = k32wvsprintfA(buffer, spec, args);
    va_end(args);
    return res;
}


/***********************************************************************
 *           hmemcpy   (KERNEL.348)
 */
void WINAPI hmemcpy16( LPVOID dst, LPCVOID src, LONG count )
{
    memcpy( dst, src, count );
}


/***********************************************************************
 *           lstrcat   (KERNEL.89)
 */
SEGPTR WINAPI lstrcat16( SEGPTR dst, LPCSTR src )
{
    /* Windows does not check for NULL pointers here, so we don't either */
    strcat( MapSL(dst), src );
    return dst;
}


/***********************************************************************
 *           lstrcatA   (KERNEL32.@)
 *           lstrcat    (KERNEL32.@)
 */
LPSTR WINAPI lstrcatA( LPSTR dst, LPCSTR src )
{
    __TRY
    {
        strcat( dst, src );
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    __ENDTRY
    return dst;
}


/***********************************************************************
 *           lstrcatW   (KERNEL32.@)
 */
LPWSTR WINAPI lstrcatW( LPWSTR dst, LPCWSTR src )
{
    __TRY
    {
        strcatW( dst, src );
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    __ENDTRY
    return dst;
}


/***********************************************************************
 *           lstrcatn   (KERNEL.352)
 */
SEGPTR WINAPI lstrcatn16( SEGPTR dst, LPCSTR src, INT16 n )
{
    LPSTR p = MapSL(dst);
    LPSTR start = p;

    while (*p) p++;
    if ((n -= (p - start)) <= 0) return dst;
    lstrcpynA( p, src, n );
    return dst;
}


/***********************************************************************
 *           lstrcpy   (KERNEL.88)
 */
SEGPTR WINAPI lstrcpy16( SEGPTR dst, LPCSTR src )
{
    if (!lstrcpyA( MapSL(dst), src )) dst = 0;
    return dst;
}


/***********************************************************************
 *           lstrcpyA   (KERNEL32.@)
 *           lstrcpy    (KERNEL32.@)
 */
LPSTR WINAPI lstrcpyA( LPSTR dst, LPCSTR src )
{
    __TRY
    {
        /* this is how Windows does it */
        memmove( dst, src, strlen(src)+1 );
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    __ENDTRY
    return dst;
}


/***********************************************************************
 *           lstrcpyW   (KERNEL32.@)
 */
LPWSTR WINAPI lstrcpyW( LPWSTR dst, LPCWSTR src )
{
    __TRY
    {
        strcpyW( dst, src );
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    __ENDTRY
    return dst;
}


/***********************************************************************
 *           lstrcpyn   (KERNEL.353)
 */
SEGPTR WINAPI lstrcpyn16( SEGPTR dst, LPCSTR src, INT16 n )
{
    lstrcpynA( MapSL(dst), src, n );
    return dst;
}


/***********************************************************************
 *           lstrcpynA   (KERNEL32.@)
 *           lstrcpyn    (KERNEL32.@)
 *
 * Note: this function differs from the UNIX strncpy, it _always_ writes
 * a terminating \0.
 *
 * Note: n is an INT but Windows treats it as unsigned, and will happily
 * copy a gazillion chars if n is negative.
 */
LPSTR WINAPI lstrcpynA( LPSTR dst, LPCSTR src, INT n )
{
    __TRY
    {
        LPSTR d = dst;
        LPCSTR s = src;
        UINT count = n;

        while ((count > 1) && *s)
        {
            count--;
            *d++ = *s++;
        }
        if (count) *d = 0;
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    __ENDTRY
    return dst;
}


/***********************************************************************
 *           lstrcpynW   (KERNEL32.@)
 *
 * Note: this function differs from the UNIX strncpy, it _always_ writes
 * a terminating \0
 *
 * Note: n is an INT but Windows treats it as unsigned, and will happily
 * copy a gazillion chars if n is negative.
 */
LPWSTR WINAPI lstrcpynW( LPWSTR dst, LPCWSTR src, INT n )
{
    __TRY
    {
        LPWSTR d = dst;
        LPCWSTR s = src;
        UINT count = n;

        while ((count > 1) && *s)
        {
            count--;
            *d++ = *s++;
        }
        if (count) *d = 0;
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    __ENDTRY
    return dst;
}


/***********************************************************************
 *           lstrlen   (KERNEL.90)
 */
INT16 WINAPI lstrlen16( LPCSTR str )
{
    return (INT16)lstrlenA( str );
}


/***********************************************************************
 *           lstrlenA   (KERNEL32.@)
 *           lstrlen    (KERNEL32.@)
 */
INT WINAPI lstrlenA( LPCSTR str )
{
    INT ret;
    __TRY
    {
        ret = strlen(str);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    __ENDTRY
    return ret;
}


/***********************************************************************
 *           lstrlenW   (KERNEL32.@)
 */
INT WINAPI lstrlenW( LPCWSTR str )
{
    INT ret;
    __TRY
    {
        ret = strlenW(str);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    __ENDTRY
    return ret;
}


/***********************************************************************
 *           UnicodeToAnsi   (KERNEL.434)
 */
INT16 WINAPI UnicodeToAnsi16( LPCWSTR src, LPSTR dst, INT16 codepage )
{
    if ( codepage == -1 ) codepage = CP_ACP;
    return WideCharToMultiByte( codepage, 0, src, -1, dst, 0x7fffffff, NULL, NULL );
}


/***************************************************************************
 *
 * Win 2.x string functions now moved to USER
 *
 * We rather want to implement them here instead of doing Callouts
 */

/***********************************************************************
 *		Reserved1 (KERNEL.77)
 */
SEGPTR WINAPI KERNEL_AnsiNext16(SEGPTR current)
{
    return (*(char *)MapSL(current)) ? current + 1 : current;
}

/***********************************************************************
 *		Reserved2(KERNEL.78)
 */
SEGPTR WINAPI KERNEL_AnsiPrev16( SEGPTR start, SEGPTR current )
{
    return (current==start)?start:current-1;
}

/***********************************************************************
 *		Reserved3 (KERNEL.79)
 */
SEGPTR WINAPI KERNEL_AnsiUpper16( SEGPTR strOrChar )
{
    /* uppercase only one char if strOrChar < 0x10000 */
    if (HIWORD(strOrChar))
    {
        char *s = MapSL(strOrChar);
        while (*s)
        {
            *s = toupper(*s);
            s++;
        }
        return strOrChar;
    }
    else return toupper((char)strOrChar);
}

/***********************************************************************
 *		Reserved4 (KERNEL.80)
 */
SEGPTR WINAPI KERNEL_AnsiLower16( SEGPTR strOrChar )
{
    /* lowercase only one char if strOrChar < 0x10000 */
    if (HIWORD(strOrChar))
    {
        char *s = MapSL(strOrChar);
        while (*s)
        {
            *s = tolower(*s);
            s++;
        }
        return strOrChar;
    }
    else return tolower((char)strOrChar);
}


/***********************************************************************
 *		Reserved5 (KERNEL.87)
 */
INT16 WINAPI KERNEL_lstrcmp16( LPCSTR str1, LPCSTR str2 )
{
    return (INT16)strcmp( str1, str2 );
}
