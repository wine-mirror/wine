/*
 * String functions
 *
 * Copyright 1993 Yngvi Sigurjonsson
 * Copyright 1996 Alexandre Julliard
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

#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "wine/exception.h"
#include "wine/unicode.h"
#include "winerror.h"
#include "winnls.h"
#include "excpt.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(string);

/* filter for page-fault exceptions */
static WINE_EXCEPTION_FILTER(page_fault)
{
    if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
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
 *           lstrcat    (KERNEL32.@)
 *           lstrcatA   (KERNEL32.@)
 */
LPSTR WINAPI lstrcatA( LPSTR dst, LPCSTR src )
{
    __TRY
    {
        strcat( dst, src );
    }
    __EXCEPT(page_fault)
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
    __EXCEPT(page_fault)
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
 *           lstrcpy    (KERNEL32.@)
 *           lstrcpyA   (KERNEL32.@)
 */
LPSTR WINAPI lstrcpyA( LPSTR dst, LPCSTR src )
{
    __TRY
    {
        /* this is how Windows does it */
        memmove( dst, src, strlen(src)+1 );
    }
    __EXCEPT(page_fault)
    {
        ERR("(%p, %p): page fault occurred ! Caused by bug ?\n", dst, src);
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
    __EXCEPT(page_fault)
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
 *           lstrcpyn    (KERNEL32.@)
 *           lstrcpynA   (KERNEL32.@)
 *
 * Note: this function differs from the UNIX strncpy, it _always_ writes
 * a terminating \0.
 *
 * Note: n is an INT but Windows treats it as unsigned, and will happily
 * copy a gazillion chars if n is negative.
 */
LPSTR WINAPI lstrcpynA( LPSTR dst, LPCSTR src, INT n )
{
    LPSTR p = dst;
    UINT count = n;

    TRACE("(%p, %s, %i)\n", dst, debugstr_a(src), n);

    /* In real windows the whole function is protected by an exception handler
     * that returns ERROR_INVALID_PARAMETER on faulty parameters
     * We currently just check for NULL.
     */
    if (!dst || !src) {
    	SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }
    while ((count > 1) && *src)
    {
        count--;
        *p++ = *src++;
    }
    if (count) *p = 0;
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
    LPWSTR p = dst;
    UINT count = n;

    TRACE("(%p, %s, %i)\n", dst,  debugstr_w(src), n);

    /* In real windows the whole function is protected by an exception handler
     * that returns ERROR_INVALID_PARAMETER on faulty parameters
     * We currently just check for NULL.
     */
    if (!dst || !src) {
    	SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }
    while ((count > 1) && *src)
    {
        count--;
        *p++ = *src++;
    }
    if (count) *p = 0;
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
 *           lstrlen    (KERNEL32.@)
 *           lstrlenA   (KERNEL32.@)
 */
INT WINAPI lstrlenA( LPCSTR str )
{
    INT ret;
    __TRY
    {
        ret = strlen(str);
    }
    __EXCEPT(page_fault)
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
    __EXCEPT(page_fault)
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
    if ( codepage == -1 )
	codepage = CP_ACP;

    return WideCharToMultiByte( codepage, 0, src, -1, dst, 0x7fffffff, NULL, NULL );
}
