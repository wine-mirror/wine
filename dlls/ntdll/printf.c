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

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <ctype.h>
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

/* FIXME: convert sizes to SIZE_T etc. */

typedef struct pf_output_t
{
    SIZE_T used;
    SIZE_T len;
    BOOL unicode;
    union {
        LPWSTR W;
        LPSTR  A;
    } buf;
} pf_output;

typedef struct pf_flags_t
{
    char Sign, LeftAlign, Alternate, PadZero;
    int FieldLength, Precision;
    char IntegerLength, IntegerDouble, IntegerNative;
    char WideString;
    char Format;
} pf_flags;

/*
 * writes a string of characters to the output
 * returns -1 if the string doesn't fit in the output buffer
 * return the length of the string if all characters were written
 */
static inline int pf_output_stringW( pf_output *out, LPCWSTR str, int len )
{
    SIZE_T space = out->len - out->used;

    if( len < 0 )
        len = wcslen( str );
    if( out->unicode )
    {
        LPWSTR p = out->buf.W + out->used;

        if (!out->buf.W)
        {
            out->used += len;
            return len;
        }
        if( space >= len )
        {
            memcpy( p, str, len*sizeof(WCHAR) );
            out->used += len;
            return len;
        }
        if( space > 0 )
        {
            memcpy( p, str, space*sizeof(WCHAR) );
            out->used += space;
        }
    }
    else
    {
        LPSTR p = out->buf.A + out->used;
        ULONG n;

        RtlUnicodeToMultiByteSize( &n, str, len * sizeof(WCHAR) );

        if (!out->buf.A)
        {
            out->used += n;
            return len;
        }
        if( space >= n )
        {
            RtlUnicodeToMultiByteN( p, n, NULL, str, len * sizeof(WCHAR) );
            out->used += n;
            return len;
        }
        if (space > 0)
        {
            RtlUnicodeToMultiByteN( p, space, NULL, str, len * sizeof(WCHAR) );
            out->used += space;
        }
    }
    return -1;
}

static inline int pf_output_stringA( pf_output *out, LPCSTR str, int len )
{
    SIZE_T space = out->len - out->used;

    if( len < 0 )
        len = strlen( str );
    if( !out->unicode )
    {
        LPSTR p = out->buf.A + out->used;

        if (!out->buf.A)
        {
            out->used += len;
            return len;
        }
        if( space >= len )
        {
            memcpy( p, str, len );
            out->used += len;
            return len;
        }
        if( space > 0 )
        {
            memcpy( p, str, space );
            out->used += space;
        }
    }
    else
    {
        LPWSTR p = out->buf.W + out->used;
        ULONG n;

        RtlMultiByteToUnicodeSize( &n, str, len );

        if (!out->buf.W)
        {
            out->used += n / sizeof(WCHAR);
            return len;
        }
        if (space >= n / sizeof(WCHAR))
        {
            RtlMultiByteToUnicodeN( p, n, NULL, str, len );
            out->used += n / sizeof(WCHAR);
            return len;
        }
        if (space > 0)
        {
            RtlMultiByteToUnicodeN( p, space * sizeof(WCHAR), NULL, str, len );
            out->used += space;
        }
    }
    return -1;
}

/* pf_fill: takes care of signs, alignment, zero and field padding */
static inline int pf_fill( pf_output *out, int len, pf_flags *flags, char left )
{
    int i, r = 0;

    if( flags->Sign && !( flags->Format == 'd' || flags->Format == 'i' ) )
        flags->Sign = 0;

    if( left && flags->Sign )
    {
        flags->FieldLength--;
        if( flags->PadZero )
            r = pf_output_stringA( out, &flags->Sign, 1 );
    }

    if( ( !left &&  flags->LeftAlign ) ||
        (  left && !flags->LeftAlign ))
    {
        for( i=0; (i<(flags->FieldLength-len)) && (r>=0); i++ )
        {
            if( left && flags->PadZero )
                r = pf_output_stringA( out, "0", 1 );
            else
                r = pf_output_stringA( out, " ", 1 );
        }
    }

    if (left && flags->Sign && !flags->PadZero && r >= 0)
        r = pf_output_stringA( out, &flags->Sign, 1 );

    return r;
}

static inline int pf_output_format_W( pf_output *out, LPCWSTR str,
                                      int len, pf_flags *flags )
{
    int r = 0;

    if (len < 0)
    {
        /* Do not search past the length specified by the precision. */
        if (flags->Precision >= 0)
        {
            for (len = 0; len < flags->Precision; len++) if (!str[len]) break;
        }
        else len = wcslen( str );
    }

    if (flags->Precision >= 0 && flags->Precision < len)
        len = flags->Precision;

    r = pf_fill( out, len, flags, 1 );

    if( r>=0 )
        r = pf_output_stringW( out, str, len );

    if( r>=0 )
        r = pf_fill( out, len, flags, 0 );

    return r;
}

static inline int pf_output_format_A( pf_output *out, LPCSTR str,
                                      int len, pf_flags *flags )
{
    int r = 0;

    if (len < 0)
    {
        /* Do not search past the length specified by the precision. */
        if (flags->Precision >= 0)
        {
            for (len = 0; len < flags->Precision; len++) if (!str[len]) break;
        }
        else len = strlen( str );
    }

    if (flags->Precision >= 0 && flags->Precision < len)
        len = flags->Precision;

    r = pf_fill( out, len, flags, 1 );

    if( r>=0 )
        r = pf_output_stringA( out, str, len );

    if( r>=0 )
        r = pf_fill( out, len, flags, 0 );

    return r;
}

static int pf_handle_string_format( pf_output *out, const void* str, int len,
                             pf_flags *flags, BOOL capital_letter)
{
     if(str == NULL)  /* catch NULL pointer */
        return pf_output_format_A( out, "(null)", -1, flags);

     /* prefixes take priority over %c,%s vs. %C,%S, so we handle them first */
    if(flags->WideString || flags->IntegerLength == 'l')
        return pf_output_format_W( out, str, len, flags);
    if(flags->IntegerLength == 'h')
        return pf_output_format_A( out, str, len, flags);

    /* %s,%c ->  chars in ansi functions & wchars in unicode
     * %S,%C -> wchars in ansi functions &  chars in unicode */
    if( capital_letter == out->unicode) /* either both TRUE or both FALSE */
        return pf_output_format_A( out, str, len, flags);
    else
        return pf_output_format_W( out, str, len, flags);
}

/* pf_integer_conv:  prints x to buf, including alternate formats and
   additional precision digits, but not field characters or the sign */
static void pf_integer_conv( char *buf, pf_flags *flags, LONGLONG x )
{
    unsigned int base;
    const char *digits;
    int i, j, k;

    if( flags->Format == 'o' )
        base = 8;
    else if( flags->Format == 'x' || flags->Format == 'X' )
        base = 16;
    else
        base = 10;

    if( flags->Format == 'X' )
        digits = "0123456789ABCDEFX";
    else
        digits = "0123456789abcdefx";

    if( x < 0 && ( flags->Format == 'd' || flags->Format == 'i' ) )
    {
        x = -x;
        flags->Sign = '-';
    }

    i = 0;
    if( x == 0 )
    {
        flags->Alternate = 0;
        if( flags->Precision )
            buf[i++] = '0';
    }
    else
        while( x != 0 )
        {
            j = (ULONGLONG) x % base;
            x = (ULONGLONG) x / base;
            buf[i++] = digits[j];
        }
    k = flags->Precision - i;
    while( k-- > 0 )
        buf[i++] = '0';
    if( flags->Alternate )
    {
        if( base == 16 )
        {
            buf[i++] = digits[16];
            buf[i++] = '0';
        }
        else if( base == 8 && buf[i-1] != '0' )
            buf[i++] = '0';
    }

    /* Adjust precision so pf_fill won't truncate the number later */
    flags->Precision = i;

    buf[i] = '\0';
    j = 0;
    while(--i > j) {
        char tmp = buf[j];
        buf[j] = buf[i];
        buf[i] = tmp;
        j++;
    }
}

static inline BOOL isDigit(WCHAR c)
{
    return c >= '0' && c <= '9';
}

/*********************************************************************
 *  pf_vsnprintf  (INTERNAL)
 *
 *  implements both A and W vsnprintf functions
 */
static int pf_vsnprintf( pf_output *out, const WCHAR *format, __ms_va_list valist )
{
    int r;
    LPCWSTR q, p = format;
    pf_flags flags;

    while (*p)
    {
        q = wcschr( p, '%' );

        /* there are no % characters left: output the rest of the string */
        if( !q )
        {
            r = pf_output_stringW(out, p, -1);
            if( r<0 )
                return r;
            p += r;
            continue;
        }

        /* there are characters before the %: output them */
        if( q != p )
        {
            r = pf_output_stringW(out, p, q - p);
            if( r<0 )
                return r;
            p = q;
        }

        /* we must be at a % now, skip over it */
        assert( *p == '%' );
        p++;

        /* output a single % character */
        if( *p == '%' )
        {
            r = pf_output_stringW(out, p++, 1);
            if( r<0 )
                return r;
            continue;
        }

        /* parse the flags */
        memset( &flags, 0, sizeof flags );
        while (*p)
        {
            if( *p == '+' || *p == ' ' )
            {
                if ( flags.Sign != '+' )
                    flags.Sign = *p;
            }
            else if( *p == '-' )
                flags.LeftAlign = *p;
            else if( *p == '0' )
                flags.PadZero = *p;
            else if( *p == '#' )
                flags.Alternate = *p;
            else
                break;
            p++;
        }

        /* deal with the field width specifier */
        flags.FieldLength = 0;
        if( *p == '*' )
        {
            flags.FieldLength = va_arg( valist, int );
            if (flags.FieldLength < 0)
            {
                flags.LeftAlign = '-';
                flags.FieldLength = -flags.FieldLength;
            }
            p++;
        }
        else while( isDigit(*p) )
        {
            flags.FieldLength *= 10;
            flags.FieldLength += *p++ - '0';
        }

        /* deal with precision */
        flags.Precision = -1;
        if( *p == '.' )
        {
            flags.Precision = 0;
            p++;
            if( *p == '*' )
            {
                flags.Precision = va_arg( valist, int );
                p++;
            }
            else while( isDigit(*p) )
            {
                flags.Precision *= 10;
                flags.Precision += *p++ - '0';
            }
        }

        /* deal with integer width modifier */
        while( *p )
        {
            if (*p == 'l' && *(p+1) == 'l')
            {
                flags.IntegerDouble++;
                p += 2;
            }
            else if( *p == 'h' || *p == 'l' || *p == 'L' )
            {
                flags.IntegerLength = *p;
                p++;
            }
            else if( *p == 'I' )
            {
                if( *(p+1) == '6' && *(p+2) == '4' )
                {
                    flags.IntegerDouble++;
                    p += 3;
                }
                else if( *(p+1) == '3' && *(p+2) == '2' )
                    p += 3;
                else if( p[1] && strchr( "diouxX", p[1] ) )
                {
                    if( sizeof(void *) == 8 )
                        flags.IntegerDouble = *p;
                    p++;
                }
                else if( isDigit(*(p+1)) || *(p+1) == 0 )
                    break;
                else
                    p++;
            }
            else if( *p == 'w' )
                flags.WideString = *p++;
            else if ((*p == 'z' || *p == 't') && p[1] && strchr("diouxX", p[1]))
                flags.IntegerNative = *p++;
            else if (*p == 'j')
            {
                flags.IntegerDouble++;
                p++;
            }
            else if( *p == 'F' )
                p++; /* ignore */
            else
                break;
        }

        flags.Format = *p;
        r = 0;

        if (flags.Format == '$')
        {
            FIXME("Positional parameters are not supported (%s)\n", wine_dbgstr_w(format));
            return -1;
        }
        /* output a string */
        if(  flags.Format == 's' || flags.Format == 'S' )
            r = pf_handle_string_format( out, va_arg(valist, const void*), -1,
                                         &flags, (flags.Format == 'S') );

        /* output a single character */
        else if( flags.Format == 'c' || flags.Format == 'C' )
        {
            INT ch = va_arg( valist, int );

            r = pf_handle_string_format( out, &ch, 1, &flags, (flags.Format == 'C') );
        }

        /* output a pointer */
        else if( flags.Format == 'p' )
        {
            char pointer[32];
            void *ptr = va_arg( valist, void * );
            int prec = flags.Precision;
            flags.Format = 'X';
            flags.PadZero = '0';
            flags.Precision = 2*sizeof(void*);
            pf_integer_conv( pointer, &flags, (ULONG_PTR)ptr );
            flags.PadZero = 0;
            flags.Precision = prec;
            r = pf_output_format_A( out, pointer, -1, &flags );
        }

        /* deal with %n */
        else if( flags.Format == 'n' )
        {
            int *x = va_arg(valist, int *);
            *x = out->used;
        }
        else if( flags.Format && strchr("diouxX", flags.Format ))
        {
            char number[40], *x = number;
            int max_len;

            /* 0 padding is added after '0x' if Alternate flag is in use */
            if((flags.Format=='x' || flags.Format=='X') && flags.PadZero && flags.Alternate
                    && !flags.LeftAlign && flags.Precision<flags.FieldLength-2)
                flags.Precision = flags.FieldLength - 2;

            max_len = (flags.FieldLength>flags.Precision ? flags.FieldLength : flags.Precision) + 10;
            if(max_len > ARRAY_SIZE(number))
                if (!(x = RtlAllocateHeap( GetProcessHeap(), 0, max_len ))) return -1;

            if(flags.IntegerDouble || (flags.IntegerNative && sizeof(void*) == 8))
                pf_integer_conv( x, &flags, va_arg(valist, LONGLONG) );
            else if(flags.Format=='d' || flags.Format=='i')
                pf_integer_conv( x, &flags, flags.IntegerLength!='h' ?
                                 va_arg(valist, int) : (short)va_arg(valist, int) );
            else
                pf_integer_conv( x, &flags, flags.IntegerLength!='h' ?
                                 (unsigned int)va_arg(valist, int) : (unsigned short)va_arg(valist, int) );

            r = pf_output_format_A( out, x, -1, &flags );
            if( x != number )
                RtlFreeHeap( GetProcessHeap(), 0, x );
        }
        else
            continue;

        if( r<0 )
            return r;
        p++;
    }

    /* check we reached the end, and null terminate the string */
    assert( *p == 0 );
    return out->used;
}


/*********************************************************************
 *                  _vsnprintf   (NTDLL.@)
 */
int CDECL NTDLL__vsnprintf( char *str, SIZE_T len, const char *format, __ms_va_list args )
{
    DWORD sz;
    LPWSTR formatW = NULL;
    pf_output out;
    int r;

    out.unicode = FALSE;
    out.buf.A = str;
    out.used = 0;
    out.len = len;

    if (format)
    {
        RtlMultiByteToUnicodeSize( &sz, format, strlen(format) + 1 );
        if (!(formatW = RtlAllocateHeap( GetProcessHeap(), 0, sz ))) return -1;
        RtlMultiByteToUnicodeN( formatW, sz, NULL, format, strlen(format) + 1 );
    }
    r = pf_vsnprintf( &out, formatW, args );
    RtlFreeHeap( GetProcessHeap(), 0, formatW );
    if (out.used < len) str[out.used] = 0;
    return r;
}


/***********************************************************************
 *                  _vsnwprintf   (NTDLL.@)
 */
int CDECL NTDLL__vsnwprintf( WCHAR *str, SIZE_T len, const WCHAR *format, __ms_va_list args )
{
    pf_output out;
    int r;

    out.unicode = TRUE;
    out.buf.W = str;
    out.used = 0;
    out.len = len;

    r = pf_vsnprintf( &out, format, args );
    if (out.used < len) str[out.used] = 0;
    return r;
}


/*********************************************************************
 *                  _snprintf   (NTDLL.@)
 */
int WINAPIV NTDLL__snprintf( char *str, SIZE_T len, const char *format, ... )
{
    int ret;
    __ms_va_list valist;

    __ms_va_start( valist, format );
    ret = NTDLL__vsnprintf( str, len, format, valist );
    __ms_va_end( valist );
    return ret;
}


/***********************************************************************
 *                  _snwprintf   (NTDLL.@)
 */
int WINAPIV NTDLL__snwprintf( WCHAR *str, SIZE_T len, const WCHAR *format, ... )
{
    int ret;
    __ms_va_list valist;

    __ms_va_start(valist, format);
    ret = NTDLL__vsnwprintf( str, len, format, valist );
    __ms_va_end(valist);
    return ret;
}


/*********************************************************************
 *                  _vsnprintf_s   (NTDLL.@)
 */
int CDECL _vsnprintf_s( char *str, SIZE_T size, SIZE_T len, const char *format, __ms_va_list args )
{
    DWORD sz;
    LPWSTR formatW = NULL;
    pf_output out;
    int r;

    out.unicode = FALSE;
    out.buf.A = str;
    out.used = 0;
    out.len = min( size, len );

    if (format)
    {
        RtlMultiByteToUnicodeSize( &sz, format, strlen(format) + 1 );
        if (!(formatW = RtlAllocateHeap( GetProcessHeap(), 0, sz ))) return -1;
        RtlMultiByteToUnicodeN( formatW, sz, NULL, format, strlen(format) + 1 );
    }
    r = pf_vsnprintf( &out, formatW, args );
    RtlFreeHeap( GetProcessHeap(), 0, formatW );
    if (out.used < size) str[out.used] = 0;
    else str[0] = 0;
    if (r == size) r = -1;
    return r;
}


/***********************************************************************
 *                  _vsnwprintf_s   (NTDLL.@)
 */
int CDECL _vsnwprintf_s( WCHAR *str, SIZE_T size, SIZE_T len, const WCHAR *format, __ms_va_list args )
{
    pf_output out;
    int r;

    out.unicode = TRUE;
    out.buf.W = str;
    out.used = 0;
    out.len = min( size, len );

    r = pf_vsnprintf( &out, format, args );
    if (out.used < size) str[out.used] = 0;
    else str[0] = 0;
    if (r == size) r = -1;
    return r;
}


/*********************************************************************
 *                  _snprintf_s   (NTDLL.@)
 */
int WINAPIV _snprintf_s( char *str, SIZE_T size, SIZE_T len, const char *format, ... )
{
    int ret;
    __ms_va_list valist;

    __ms_va_start( valist, format );
    ret = _vsnprintf_s( str, size, len, format, valist );
    __ms_va_end( valist );
    return ret;
}


/*********************************************************************
 *                  _snwprintf_s   (NTDLL.@)
 */
int WINAPIV _snwprintf_s( WCHAR *str, SIZE_T size, SIZE_T len, const WCHAR *format, ... )
{
    int ret;
    __ms_va_list valist;

    __ms_va_start( valist, format );
    ret = _vsnwprintf_s( str, size, len, format, valist );
    __ms_va_end( valist );
    return ret;
}


/*********************************************************************
 *                  vsprintf   (NTDLL.@)
 */
int CDECL NTDLL_vsprintf( char *str, const char *format, __ms_va_list args )
{
    return NTDLL__vsnprintf( str, size_max, format, args );
}


/*********************************************************************
 *                  vsprintf_s   (NTDLL.@)
 */
int CDECL vsprintf_s( char *str, SIZE_T size, const char *format, __ms_va_list args )
{
    return _vsnprintf_s( str, size, size, format, args );
}


/*********************************************************************
 *                  _vswprintf   (NTDLL.@)
 */
int CDECL NTDLL__vswprintf( WCHAR *str, const WCHAR *format, __ms_va_list args )
{
    return NTDLL__vsnwprintf( str, size_max, format, args );
}


/*********************************************************************
 *                  vswprintf_s   (NTDLL.@)
 */
int CDECL vswprintf_s( WCHAR *str, SIZE_T size, const WCHAR *format, __ms_va_list args )
{
    return _vsnwprintf_s( str, size, size, format, args );
}


/*********************************************************************
 *                  sprintf   (NTDLL.@)
 */
int WINAPIV NTDLL_sprintf( char *str, const char *format, ... )
{
    int ret;
    __ms_va_list valist;

    __ms_va_start( valist, format );
    ret = NTDLL__vsnprintf( str, size_max, format, valist );
    __ms_va_end( valist );
    return ret;
}


/*********************************************************************
 *                  sprintf_s   (NTDLL.@)
 */
int WINAPIV sprintf_s( char *str, SIZE_T size, const char *format, ... )
{
    int ret;
    __ms_va_list valist;

    __ms_va_start( valist, format );
    ret = vsprintf_s( str, size, format, valist );
    __ms_va_end( valist );
    return ret;
}


/***********************************************************************
 *                  swprintf   (NTDLL.@)
 */
int WINAPIV NTDLL_swprintf( WCHAR *str, const WCHAR *format, ... )
{
    int ret;
    __ms_va_list valist;

    __ms_va_start(valist, format);
    ret = NTDLL__vsnwprintf( str, size_max, format, valist );
    __ms_va_end(valist);
    return ret;
}


/***********************************************************************
 *                  swprintf_s   (NTDLL.@)
 */
int WINAPIV swprintf_s( WCHAR *str, SIZE_T size, const WCHAR *format, ... )
{
    int ret;
    __ms_va_list valist;

    __ms_va_start(valist, format);
    ret = vswprintf_s( str, size, format, valist );
    __ms_va_end(valist);
    return ret;
}
