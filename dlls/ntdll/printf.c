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
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);

static const SIZE_T size_max = ~(SIZE_T)0 >> 1;

/* FIXME: convert sizes to SIZE_T etc. */

typedef struct pf_output_t
{
    int used;
    int len;
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
    char IntegerLength, IntegerDouble;
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
    int space = out->len - out->used;

    if( len < 0 )
        len = strlenW( str );
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
    int space = out->len - out->used;

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

    if( len < 0 )
        len = strlenW( str );

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

    if( len < 0 )
        len = strlen( str );

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

static inline BOOL pf_is_integer_format( char fmt )
{
    static const char float_fmts[] = "diouxX";
    if (!fmt)
        return FALSE;
    return strchr( float_fmts, fmt ) != 0;
}

static inline BOOL pf_is_double_format( char fmt )
{
    static const char float_fmts[] = "aeEfgG";
    if (!fmt)
        return FALSE;
    return strchr( float_fmts, fmt ) != 0;
}

static inline BOOL pf_is_valid_format( char fmt )
{
    static const char float_fmts[] = "acCdeEfgGinouxX";
    if (!fmt)
        return FALSE;
    return strchr( float_fmts, fmt ) != 0;
}

static void pf_rebuild_format_string( char *p, pf_flags *flags )
{
    *p++ = '%';
    if( flags->Sign )
        *p++ = flags->Sign;
    if( flags->LeftAlign )
        *p++ = flags->LeftAlign;
    if( flags->Alternate )
        *p++ = flags->Alternate;
    if( flags->PadZero )
        *p++ = flags->PadZero;
    if( flags->FieldLength )
    {
        sprintf(p, "%d", flags->FieldLength);
        p += strlen(p);
    }
    if( flags->Precision >= 0 )
    {
        sprintf(p, ".%d", flags->Precision);
        p += strlen(p);
    }
    *p++ = flags->Format;
    *p++ = 0;
}

/* pf_integer_conv:  prints x to buf, including alternate formats and
   additional precision digits, but not field characters or the sign */
static void pf_integer_conv( char *buf, int buf_len, pf_flags *flags,
                             LONGLONG x )
{
    unsigned int base;
    const char *digits;

    int i, j, k;
    char number[40], *tmp = number;

    if( buf_len > sizeof number )
    {
        if (!(tmp = RtlAllocateHeap( GetProcessHeap(), 0, buf_len )))
        {
            buf[0] = '\0';
            return;
        }
    }

    base = 10;
    if( flags->Format == 'o' )
        base = 8;
    else if( flags->Format == 'x' || flags->Format == 'X' )
        base = 16;

    if( flags->Format == 'X' )
        digits = "0123456789ABCDEFX";
    else
        digits = "0123456789abcdefx";

    if( x < 0 && ( flags->Format == 'd' || flags->Format == 'i' ) )
    {
        x = -x;
        flags->Sign = '-';
    }

    /* Do conversion (backwards) */
    i = 0;
    if( x == 0 && flags->Precision )
        tmp[i++] = '0';
    else
        while( x != 0 )
        {
            j = (ULONGLONG) x % base;
            x = (ULONGLONG) x / base;
            tmp[i++] = digits[j];
        }
    k = flags->Precision - i;
    while( k-- > 0 )
        tmp[i++] = '0';
    if( flags->Alternate )
    {
        if( base == 16 )
        {
            tmp[i++] = digits[16];
            tmp[i++] = '0';
        }
        else if( base == 8 && tmp[i-1] != '0' )
            tmp[i++] = '0';
    }

    /* Reverse for buf */
    j = 0;
    while( i-- > 0 )
        buf[j++] = tmp[i];
    buf[j] = '\0';

    /* Adjust precision so pf_fill won't truncate the number later */
    flags->Precision = strlen( buf );

    if( tmp != number )
        RtlFreeHeap( GetProcessHeap(), 0, tmp );

    return;
}

/* pf_fixup_exponent: convert a string containing a 2 digit exponent
   to 3 digits, accounting for padding, in place. Needed to match
   the native printf's which always use 3 digits. */
static void pf_fixup_exponent( char *buf )
{
    char* tmp = buf;

    while (tmp[0] && NTDLL_tolower(tmp[0]) != 'e')
        tmp++;

    if (tmp[0] && (tmp[1] == '+' || tmp[1] == '-') &&
        isdigit(tmp[2]) && isdigit(tmp[3]))
    {
        char final;

        if (isdigit(tmp[4]))
            return; /* Exponent already 3 digits */

        /* We have a 2 digit exponent. Prepend '0' to make it 3 */
        tmp += 2;
        final = tmp[2];
        tmp[2] = tmp[1];
        tmp[1] = tmp[0];
        tmp[0] = '0';
        if (final == '\0')
        {
            /* We didn't expand into trailing space, so this string isn't left
             * justified. Terminate the string and strip a ' ' at the start of
             * the string if there is one (as there may be if the string is
             * right justified).
             */
            tmp[3] = '\0';
            if (buf[0] == ' ')
                memmove(buf, buf + 1, (tmp - buf) + 3);
        }
        /* Otherwise, we expanded into trailing space -> nothing to do */
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

    TRACE("format is %s\n",debugstr_w(format));
    while (*p)
    {
        q = strchrW( p, '%' );

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
            if( *p == 'h' || *p == 'l' || *p == 'L' )
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
                else if( isDigit(*(p+1)) || *(p+1) == 0 )
                    break;
                else
                    p++;
            }
            else if( *p == 'w' )
                flags.WideString = *p++;
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

            flags.PadZero = 0;
            if( flags.Alternate )
                sprintf(pointer, "0X%0*lX", 2 * (int)sizeof(ptr), (ULONG_PTR)ptr);
            else
                sprintf(pointer, "%0*lX", 2 * (int)sizeof(ptr), (ULONG_PTR)ptr);
            r = pf_output_format_A( out, pointer, -1, &flags );
        }

        /* deal with %n */
        else if( flags.Format == 'n' )
        {
            int *x = va_arg(valist, int *);
            *x = out->used;
        }

        /* deal with 64-bit integers */
        else if( pf_is_integer_format( flags.Format ) && flags.IntegerDouble )
        {
            char number[40], *x = number;

            /* Estimate largest possible required buffer size:
               * Chooses the larger of the field or precision
               * Includes extra bytes: 1 byte for null, 1 byte for sign,
                 4 bytes for exponent, 2 bytes for alternate formats, 1 byte
                 for a decimal, and 1 byte for an additional float digit. */
            int x_len = ((flags.FieldLength > flags.Precision) ?
                        flags.FieldLength : flags.Precision) + 10;

            if( x_len >= sizeof number)
                if (!(x = RtlAllocateHeap( GetProcessHeap(), 0, x_len )))
                    return -1;

            pf_integer_conv( x, x_len, &flags, va_arg(valist, LONGLONG) );

            r = pf_output_format_A( out, x, -1, &flags );
            if( x != number )
                RtlFreeHeap( GetProcessHeap(), 0, x );
        }

        /* deal with integers and floats using libc's printf */
        else if( pf_is_valid_format( flags.Format ) )
        {
            char fmt[20], number[40], *x = number;

            /* Estimate largest possible required buffer size:
               * Chooses the larger of the field or precision
               * Includes extra bytes: 1 byte for null, 1 byte for sign,
                 4 bytes for exponent, 2 bytes for alternate formats, 1 byte
                 for a decimal, and 1 byte for an additional float digit. */
            int x_len = ((flags.FieldLength > flags.Precision) ?
                        flags.FieldLength : flags.Precision) + 10;

            if( x_len >= sizeof number)
                if (!(x = RtlAllocateHeap( GetProcessHeap(), 0, x_len )))
                    return -1;

            pf_rebuild_format_string( fmt, &flags );

            if( pf_is_double_format( flags.Format ) )
            {
                sprintf( x, fmt, va_arg(valist, double) );
                if (NTDLL_tolower(flags.Format) == 'e' || NTDLL_tolower(flags.Format) == 'g')
                    pf_fixup_exponent( x );
            }
            else
                sprintf( x, fmt, va_arg(valist, int) );

            r = pf_output_stringA( out, x, -1 );
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
 *                  vsprintf   (NTDLL.@)
 */
int CDECL NTDLL_vsprintf( char *str, const char *format, __ms_va_list args )
{
    return NTDLL__vsnprintf( str, size_max, format, args );
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
