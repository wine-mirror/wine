/*
 * ntdll printf functions
 *
 * Copyright 1999, 2009 Alexandre Julliard
 * Copyright 2000 Jon Griffiths
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

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winternl.h"
#include "ntdll_misc.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);

static const SIZE_T size_max = ~(SIZE_T)0 >> 1;

typedef struct pf_flags_t
{
    enum { LEN_DEFAULT, LEN_SHORT, LEN_LONG } IntegerLength;
    BOOLEAN IntegerDouble, IntegerNative, LeftAlign, Alternate, PadZero, WideString;
    int FieldLength, Precision;
    char Sign, Format;
} pf_flags;


#include "printf.h"
#define PRINTF_WIDE
#include "printf.h"
#undef PRINTF_WIDE

/*********************************************************************
 *                  _vsnprintf   (NTDLL.@)
 */
int CDECL _vsnprintf( char *str, size_t len, const char *format, va_list args )
{
    pf_output_a out = { str, len };
    int r = pf_vsnprintf_a( &out, format, args );

    if (out.used < len) str[out.used] = 0;
    return r;
}


/***********************************************************************
 *                  _vsnwprintf   (NTDLL.@)
 */
int CDECL _vsnwprintf( WCHAR *str, size_t len, const WCHAR *format, va_list args )
{
    pf_output_w out = { str, len };
    int r = pf_vsnprintf_w( &out, format, args );

    if (out.used < len) str[out.used] = 0;
    return r;
}


/*********************************************************************
 *                  _vscprintf   (NTDLL.@)
 */
int CDECL _vscprintf( const char *format, va_list valist )
{
    return _vsnprintf( NULL, INT_MAX, format, valist );
}


/*********************************************************************
 *                  _vscwprintf   (NTDLL.@)
 */
int CDECL _vscwprintf( const wchar_t *format, va_list args )
{
    return _vsnwprintf( NULL, INT_MAX, format, args );
}


/*********************************************************************
 *                  _snprintf   (NTDLL.@)
 */
int WINAPIV NTDLL__snprintf( char *str, size_t len, const char *format, ... )
{
    int ret;
    va_list valist;

    va_start( valist, format );
    ret = _vsnprintf( str, len, format, valist );
    va_end( valist );
    return ret;
}


/***********************************************************************
 *                  _snwprintf   (NTDLL.@)
 */
int WINAPIV _snwprintf( WCHAR *str, size_t len, const WCHAR *format, ... )
{
    int ret;
    va_list valist;

    va_start(valist, format);
    ret = _vsnwprintf( str, len, format, valist );
    va_end(valist);
    return ret;
}


/*********************************************************************
 *                  _vsnprintf_s   (NTDLL.@)
 */
int CDECL _vsnprintf_s( char *str, size_t size, size_t len, const char *format, va_list args )
{
    pf_output_a out = { str, min( size, len ) };
    int r = pf_vsnprintf_a( &out, format, args );

    if (out.used < size) str[out.used] = 0;
    else str[0] = 0;
    if (r == size) r = -1;
    return r;
}


/***********************************************************************
 *                  _vsnwprintf_s   (NTDLL.@)
 */
int CDECL _vsnwprintf_s( WCHAR *str, size_t size, size_t len, const WCHAR *format, va_list args )
{
    pf_output_w out = { str, min( size, len ) };
    int r = pf_vsnprintf_w( &out, format, args );

    if (out.used < size) str[out.used] = 0;
    else str[0] = 0;
    if (r == size) r = -1;
    return r;
}


/*********************************************************************
 *                  _snprintf_s   (NTDLL.@)
 */
int WINAPIV _snprintf_s( char *str, size_t size, size_t len, const char *format, ... )
{
    int ret;
    va_list valist;

    va_start( valist, format );
    ret = _vsnprintf_s( str, size, len, format, valist );
    va_end( valist );
    return ret;
}


/*********************************************************************
 *                  _snwprintf_s   (NTDLL.@)
 */
int WINAPIV _snwprintf_s( WCHAR *str, size_t size, size_t len, const WCHAR *format, ... )
{
    int ret;
    va_list valist;

    va_start( valist, format );
    ret = _vsnwprintf_s( str, size, len, format, valist );
    va_end( valist );
    return ret;
}


/*********************************************************************
 *                  vsprintf   (NTDLL.@)
 */
int CDECL vsprintf( char *str, const char *format, va_list args )
{
    return _vsnprintf( str, size_max, format, args );
}


/*********************************************************************
 *                  vsprintf_s   (NTDLL.@)
 */
int CDECL vsprintf_s( char *str, size_t size, const char *format, va_list args )
{
    return _vsnprintf_s( str, size, size, format, args );
}


/*********************************************************************
 *                  _vswprintf   (NTDLL.@)
 */
int CDECL _vswprintf( WCHAR *str, const WCHAR *format, va_list args )
{
    return _vsnwprintf( str, size_max, format, args );
}


/*********************************************************************
 *                  vswprintf_s   (NTDLL.@)
 */
int CDECL vswprintf_s( WCHAR *str, size_t size, const WCHAR *format, va_list args )
{
    return _vsnwprintf_s( str, size, size, format, args );
}


/*********************************************************************
 *                  sprintf   (NTDLL.@)
 */
int WINAPIV NTDLL_sprintf( char *str, const char *format, ... )
{
    int ret;
    va_list valist;

    va_start( valist, format );
    ret = _vsnprintf( str, size_max, format, valist );
    va_end( valist );
    return ret;
}


/*********************************************************************
 *                  sprintf_s   (NTDLL.@)
 */
int WINAPIV sprintf_s( char *str, size_t size, const char *format, ... )
{
    int ret;
    va_list valist;

    va_start( valist, format );
    ret = vsprintf_s( str, size, format, valist );
    va_end( valist );
    return ret;
}


/***********************************************************************
 *                  swprintf   (NTDLL.@)
 */
int WINAPIV NTDLL_swprintf( WCHAR *str, const WCHAR *format, ... )
{
    int ret;
    va_list valist;

    va_start(valist, format);
    ret = _vsnwprintf( str, size_max, format, valist );
    va_end(valist);
    return ret;
}


/***********************************************************************
 *                  swprintf_s   (NTDLL.@)
 */
int WINAPIV swprintf_s( WCHAR *str, size_t size, const WCHAR *format, ... )
{
    int ret;
    va_list valist;

    va_start(valist, format);
    ret = vswprintf_s( str, size, format, valist );
    va_end(valist);
    return ret;
}
