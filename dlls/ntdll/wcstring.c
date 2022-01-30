/*
 * NTDLL wide-char functions
 *
 * Copyright 2000 Alexandre Julliard
 * Copyright 2000 Jon Griffiths
 * Copyright 2003 Thomas Mertes
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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winternl.h"
#include "ntdll_misc.h"

static const unsigned short wctypes[256] =
{
    /* 00 */
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    0x0020, 0x0068, 0x0028, 0x0028, 0x0028, 0x0028, 0x0020, 0x0020,
    /* 10 */
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    /* 20 */
    0x0048, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    /* 30 */
    0x0084, 0x0084, 0x0084, 0x0084, 0x0084, 0x0084, 0x0084, 0x0084,
    0x0084, 0x0084, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    /* 40 */
    0x0010, 0x0181, 0x0181, 0x0181, 0x0181, 0x0181, 0x0181, 0x0101,
    0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101,
    /* 50 */
    0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101,
    0x0101, 0x0101, 0x0101, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    /* 60 */
    0x0010, 0x0182, 0x0182, 0x0182, 0x0182, 0x0182, 0x0182, 0x0102,
    0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102,
    /* 70 */
    0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102,
    0x0102, 0x0102, 0x0102, 0x0010, 0x0010, 0x0010, 0x0010, 0x0020,
    /* 80 */
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    /* 90 */
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    /* a0 */
    0x0048, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    /* b0 */
    0x0010, 0x0010, 0x0014, 0x0014, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0010, 0x0014, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    /* c0 */
    0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101,
    0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101,
    /* d0 */
    0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0010,
    0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0102,
    /* e0 */
    0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102,
    0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102,
    /* f0 */
    0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0010,
    0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102
};


/*********************************************************************
 *           _wcsicmp    (NTDLL.@)
 */
int __cdecl _wcsicmp( LPCWSTR str1, LPCWSTR str2 )
{
    for (;;)
    {
        WCHAR ch1 = (*str1 >= 'A' && *str1 <= 'Z') ? *str1 + 32 : *str1;
        WCHAR ch2 = (*str2 >= 'A' && *str2 <= 'Z') ? *str2 + 32 : *str2;
        if (ch1 != ch2 || !*str1) return ch1 - ch2;
        str1++;
        str2++;
    }
}


/*********************************************************************
 *           _wcslwr    (NTDLL.@)
 */
LPWSTR __cdecl _wcslwr( LPWSTR str )
{
    WCHAR *ret = str;

    while (*str)
    {
        WCHAR ch = *str;
        if (ch >= 'A' && ch <= 'Z') ch += 32;
        *str++ = ch;
    }
    return ret;
}


/*********************************************************************
 *           _wcsnicmp    (NTDLL.@)
 */
int __cdecl _wcsnicmp( LPCWSTR str1, LPCWSTR str2, size_t n )
{
    int ret = 0;
    for ( ; n > 0; n--, str1++, str2++)
    {
        WCHAR ch1 = (*str1 >= 'A' && *str1 <= 'Z') ? *str1 + 32 : *str1;
        WCHAR ch2 = (*str2 >= 'A' && *str2 <= 'Z') ? *str2 + 32 : *str2;
        if ((ret = ch1 - ch2) ||  !*str1) break;
    }
    return ret;
}


/*********************************************************************
 *           _wcsupr    (NTDLL.@)
 */
LPWSTR __cdecl _wcsupr( LPWSTR str )
{
    WCHAR *ret = str;

    while (*str)
    {
        WCHAR ch = *str;
        if (ch >= 'a' && ch <= 'z') ch -= 32;
        *str++ = ch;
    }
    return ret;
}


/***********************************************************************
 *           wcscpy    (NTDLL.@)
 */
LPWSTR __cdecl wcscpy( LPWSTR dst, LPCWSTR src )
{
    WCHAR *p = dst;
    while ((*p++ = *src++));
    return dst;
}


/***********************************************************************
 *           wcslen    (NTDLL.@)
 */
size_t __cdecl wcslen( LPCWSTR str )
{
    const WCHAR *s = str;
    while (*s) s++;
    return s - str;
}


/***********************************************************************
 *           wcscat    (NTDLL.@)
 */
LPWSTR __cdecl wcscat( LPWSTR dst, LPCWSTR src )
{
    wcscpy( dst + wcslen(dst), src );
    return dst;
}


/*********************************************************************
 *           wcschr    (NTDLL.@)
 */
LPWSTR __cdecl wcschr( LPCWSTR str, WCHAR ch )
{
    do { if (*str == ch) return (WCHAR *)(ULONG_PTR)str; } while (*str++);
    return NULL;
}


/*********************************************************************
 *           wcscmp    (NTDLL.@)
 */
int __cdecl wcscmp( LPCWSTR str1, LPCWSTR str2 )
{
    while (*str1 && (*str1 == *str2)) { str1++; str2++; }
    return *str1 - *str2;
}


/*********************************************************************
 *           wcscspn    (NTDLL.@)
 */
size_t __cdecl wcscspn( LPCWSTR str, LPCWSTR reject )
{
    const WCHAR *ptr;
    for (ptr = str; *ptr; ptr++) if (wcschr( reject, *ptr )) break;
    return ptr - str;
}


/*********************************************************************
 *           wcsncat    (NTDLL.@)
 */
LPWSTR __cdecl wcsncat( LPWSTR s1, LPCWSTR s2, size_t n )
{
    LPWSTR ret = s1;
    while (*s1) s1++;
    while (n-- > 0) if (!(*s1++ = *s2++)) return ret;
    *s1 = 0;
    return ret;
}


/*********************************************************************
 *           wcsncmp    (NTDLL.@)
 */
int __cdecl wcsncmp( LPCWSTR str1, LPCWSTR str2, size_t n )
{
    if (n <= 0) return 0;
    while ((--n > 0) && *str1 && (*str1 == *str2)) { str1++; str2++; }
    return *str1 - *str2;
}


/*********************************************************************
 *           wcsncpy    (NTDLL.@)
 */
#undef wcsncpy
LPWSTR __cdecl wcsncpy( LPWSTR s1, LPCWSTR s2, size_t n )
{
    WCHAR *ret = s1;
    for ( ; n; n--) if (!(*s1++ = *s2++)) break;
    for ( ; n; n--) *s1++ = 0;
    return ret;
}


/*********************************************************************
 *           wcsnlen    (NTDLL.@)
 */
size_t __cdecl wcsnlen( const WCHAR *str, size_t len )
{
    const WCHAR *s = str;
    for (s = str; len && *s; s++, len--) ;
    return s - str;
}


/*********************************************************************
 *           wcspbrk    (NTDLL.@)
 */
LPWSTR __cdecl wcspbrk( LPCWSTR str, LPCWSTR accept )
{
    for ( ; *str; str++) if (wcschr( accept, *str )) return (WCHAR *)(ULONG_PTR)str;
    return NULL;
}


/*********************************************************************
 *           wcsrchr    (NTDLL.@)
 */
LPWSTR __cdecl wcsrchr( LPCWSTR str, WCHAR ch )
{
    WCHAR *ret = NULL;
    do { if (*str == ch) ret = (WCHAR *)(ULONG_PTR)str; } while (*str++);
    return ret;
}


/*********************************************************************
 *           wcsspn    (NTDLL.@)
 */
size_t __cdecl wcsspn( LPCWSTR str, LPCWSTR accept )
{
    const WCHAR *ptr;
    for (ptr = str; *ptr; ptr++) if (!wcschr( accept, *ptr )) break;
    return ptr - str;
}


/*********************************************************************
 *           wcsstr    (NTDLL.@)
 */
LPWSTR __cdecl wcsstr( LPCWSTR str, LPCWSTR sub )
{
    while (*str)
    {
        const WCHAR *p1 = str, *p2 = sub;
        while (*p1 && *p2 && *p1 == *p2) { p1++; p2++; }
        if (!*p2) return (WCHAR *)str;
        str++;
    }
    return NULL;
}


/*********************************************************************
 *           wcstok    (NTDLL.@)
 */
LPWSTR __cdecl wcstok( LPWSTR str, LPCWSTR delim )
{
    static LPWSTR next = NULL;
    LPWSTR ret;

    if (!str)
        if (!(str = next)) return NULL;

    while (*str && wcschr( delim, *str )) str++;
    if (!*str) return NULL;
    ret = str++;
    while (*str && !wcschr( delim, *str )) str++;
    if (*str) *str++ = 0;
    next = str;
    return ret;
}


/*********************************************************************
 *           wcstombs    (NTDLL.@)
 */
size_t __cdecl wcstombs( char *dst, const WCHAR *src, size_t n )
{
    DWORD len;

    if (!dst)
    {
        RtlUnicodeToMultiByteSize( &len, src, wcslen(src) * sizeof(WCHAR) );
        return len;
    }
    else
    {
        if (n <= 0) return 0;
        RtlUnicodeToMultiByteN( dst, n, &len, src, wcslen(src) * sizeof(WCHAR) );
        if (len < n) dst[len] = 0;
    }
    return len;
}


/*********************************************************************
 *           mbstowcs    (NTDLL.@)
 */
size_t __cdecl mbstowcs( WCHAR *dst, const char *src, size_t n )
{
    DWORD len;

    if (!dst)
    {
        RtlMultiByteToUnicodeSize( &len, src, strlen(src) );
    }
    else
    {
        if (n <= 0) return 0;
        RtlMultiByteToUnicodeN( dst, n*sizeof(WCHAR), &len, src, strlen(src) );
        if (len / sizeof(WCHAR) < n) dst[len / sizeof(WCHAR)] = 0;
    }
    return len / sizeof(WCHAR);
}


/*********************************************************************
 *           iswctype    (NTDLL.@)
 */
INT __cdecl iswctype( WCHAR wc, unsigned short type )
{
    if (wc >= 256) return 0;
    return wctypes[wc] & type;
}


/*********************************************************************
 *           iswalpha    (NTDLL.@)
 */
INT __cdecl iswalpha( WCHAR wc )
{
    if (wc >= 256) return 0;
    return wctypes[wc] & (C1_ALPHA | C1_UPPER | C1_LOWER);
}


/*********************************************************************
 *		iswdigit (NTDLL.@)
 *
 * Checks if a unicode char wc is a digit
 *
 * RETURNS
 *  TRUE: The unicode char wc is a digit.
 *  FALSE: Otherwise
 */
INT __cdecl iswdigit( WCHAR wc )
{
    if (wc >= 256) return 0;
    return wctypes[wc] & C1_DIGIT;
}


/*********************************************************************
 *		iswlower (NTDLL.@)
 *
 * Checks if a unicode char wc is a lower case letter
 *
 * RETURNS
 *  TRUE: The unicode char wc is a lower case letter.
 *  FALSE: Otherwise
 */
INT __cdecl iswlower( WCHAR wc )
{
    if (wc >= 256) return 0;
    return wctypes[wc] & C1_LOWER;
}


/*********************************************************************
 *		iswspace (NTDLL.@)
 *
 * Checks if a unicode char wc is a white space character
 *
 * RETURNS
 *  TRUE: The unicode char wc is a white space character.
 *  FALSE: Otherwise
 */
INT __cdecl iswspace( WCHAR wc )
{
    if (wc >= 256) return 0;
    return wctypes[wc] & C1_SPACE;
}


/*********************************************************************
 *		iswxdigit (NTDLL.@)
 *
 * Checks if a unicode char wc is an extended digit
 *
 * RETURNS
 *  TRUE: The unicode char wc is an extended digit.
 *  FALSE: Otherwise
 */
INT __cdecl iswxdigit( WCHAR wc )
{
    if (wc >= 256) return 0;
    return wctypes[wc] & C1_XDIGIT;
}


static int wctoint( WCHAR c )
{
    /* NOTE: MAP_FOLDDIGITS supports too many things. */
    /* Unicode points that contain digits 0-9; keep this sorted! */
    static const WCHAR zeros[] =
    {
        0x0660, 0x06f0, 0x0966, 0x09e6, 0x0a66, 0x0ae6, 0x0b66, 0x0c66, 0x0ce6,
        0x0d66, 0x0e50, 0x0ed0, 0x0f20, 0x1040, 0x17e0, 0x1810, 0xff10
    };
    int i;

    if ('0' <= c && c <= '9') return c - '0';
    if ('A' <= c && c <= 'Z') return c - 'A' + 10;
    if ('a' <= c && c <= 'z') return c - 'a' + 10;
    for (i = 0; i < ARRAY_SIZE(zeros) && c >= zeros[i]; i++)
        if (zeros[i] <= c && c <= zeros[i] + 9) return c - zeros[i];

    return -1;
}

/*********************************************************************
 *                  wcstol  (NTDLL.@)
 */
__msvcrt_long __cdecl wcstol(LPCWSTR s, LPWSTR *end, INT base)
{
    BOOL negative = FALSE, empty = TRUE;
    LONG ret = 0;

    if (base < 0 || base == 1 || base > 36) return 0;
    if (end) *end = (WCHAR *)s;
    while (iswspace(*s)) s++;

    if (*s == '-')
    {
        negative = TRUE;
        s++;
    }
    else if (*s == '+') s++;

    if ((base == 0 || base == 16) && !wctoint( *s ) && (s[1] == 'x' || s[1] == 'X'))
    {
        base = 16;
        s += 2;
    }
    if (base == 0) base = wctoint( *s ) ? 10 : 8;

    while (*s)
    {
        int v = wctoint( *s );
        if (v < 0 || v >= base) break;
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


/*********************************************************************
 *                  wcstoul  (NTDLL.@)
 */
__msvcrt_ulong __cdecl wcstoul(LPCWSTR s, LPWSTR *end, INT base)
{
    BOOL negative = FALSE, empty = TRUE;
    ULONG ret = 0;

    if (base < 0 || base == 1 || base > 36) return 0;
    if (end) *end = (WCHAR *)s;
    while (iswspace(*s)) s++;

    if (*s == '-')
    {
        negative = TRUE;
        s++;
    }
    else if (*s == '+') s++;

    if ((base == 0 || base == 16) && !wctoint( *s ) && (s[1] == 'x' || s[1] == 'X'))
    {
        base = 16;
        s += 2;
    }
    if (base == 0) base = wctoint( *s ) ? 10 : 8;

    while (*s)
    {
        int v = wctoint( *s );
        if (v < 0 || v >= base) break;
        s++;
        empty = FALSE;

        if (ret > MAXDWORD / base || ret * base > MAXDWORD - v)
            ret = MAXDWORD;
        else
            ret = ret * base + v;
    }

    if (end && !empty) *end = (WCHAR *)s;
    return negative ? -ret : ret;
}


/*********************************************************************
 *      _ultow   (NTDLL.@)
 *
 * Converts an unsigned long integer to a unicode string.
 *
 * RETURNS
 *  Always returns str.
 *
 * NOTES
 *  Converts value to a '\0' terminated wstring which is copied to str.
 *  The maximum length of the copied str is 33 bytes.
 *  Does not check if radix is in the range of 2 to 36.
 *  If str is NULL it just returns NULL.
 */
LPWSTR __cdecl _ultow( __msvcrt_ulong value, LPWSTR str, INT radix )
{
    WCHAR buffer[33];
    PWCHAR pos;
    WCHAR digit;

    pos = &buffer[32];
    *pos = '\0';

    do {
	digit = value % radix;
	value = value / radix;
	if (digit < 10) {
	    *--pos = '0' + digit;
	} else {
	    *--pos = 'a' + digit - 10;
	} /* if */
    } while (value != 0L);

    if (str != NULL) {
	memcpy(str, pos, (&buffer[32] - pos + 1) * sizeof(WCHAR));
    } /* if */
    return str;
}


/*********************************************************************
 *      _ltow   (NTDLL.@)
 *
 * Converts a long integer to a unicode string.
 *
 * RETURNS
 *  Always returns str.
 *
 * NOTES
 *  Converts value to a '\0' terminated wstring which is copied to str.
 *  The maximum length of the copied str is 33 bytes. If radix
 *  is 10 and value is negative, the value is converted with sign.
 *  Does not check if radix is in the range of 2 to 36.
 *  If str is NULL it just returns NULL.
 */
LPWSTR __cdecl _ltow( __msvcrt_long value, LPWSTR str, INT radix )
{
    ULONG val;
    int negative;
    WCHAR buffer[33];
    PWCHAR pos;
    WCHAR digit;

    if (value < 0 && radix == 10) {
	negative = 1;
        val = -value;
    } else {
	negative = 0;
        val = value;
    } /* if */

    pos = &buffer[32];
    *pos = '\0';

    do {
	digit = val % radix;
	val = val / radix;
	if (digit < 10) {
	    *--pos = '0' + digit;
	} else {
	    *--pos = 'a' + digit - 10;
	} /* if */
    } while (val != 0L);

    if (negative) {
	*--pos = '-';
    } /* if */

    if (str != NULL) {
	memcpy(str, pos, (&buffer[32] - pos + 1) * sizeof(WCHAR));
    } /* if */
    return str;
}


/*********************************************************************
 *      _itow    (NTDLL.@)
 *
 * Converts an integer to a unicode string.
 *
 * RETURNS
 *  Always returns str.
 *
 * NOTES
 *  Converts value to a '\0' terminated wstring which is copied to str.
 *  The maximum length of the copied str is 33 bytes. If radix
 *  is 10 and value is negative, the value is converted with sign.
 *  Does not check if radix is in the range of 2 to 36.
 *  If str is NULL it just returns NULL.
 *
 * DIFFERENCES
 * - The native function crashes when the string is longer than 19 chars.
 *   This function does not have this bug.
 */
LPWSTR __cdecl _itow(
    int value,  /* [I] Value to be converted */
    LPWSTR str, /* [O] Destination for the converted value */
    INT radix)  /* [I] Number base for conversion */
{
    return _ltow(value, str, radix);
}


/*********************************************************************
 *      _ui64tow   (NTDLL.@)
 *
 * Converts a large unsigned integer to a unicode string.
 *
 * RETURNS
 *  Always returns str.
 *
 * NOTES
 *  Converts value to a '\0' terminated wstring which is copied to str.
 *  The maximum length of the copied str is 33 bytes.
 *  Does not check if radix is in the range of 2 to 36.
 *  If str is NULL it just returns NULL.
 *
 * DIFFERENCES
 * - This function does not exist in the native DLL (but in msvcrt).
 *   But since the maintenance of all these functions is better done
 *   in one place we implement it here.
 */
LPWSTR __cdecl _ui64tow(
    ULONGLONG value, /* [I] Value to be converted */
    LPWSTR str,      /* [O] Destination for the converted value */
    INT radix)       /* [I] Number base for conversion */
{
    WCHAR buffer[65];
    PWCHAR pos;
    WCHAR digit;

    pos = &buffer[64];
    *pos = '\0';

    do {
	digit = value % radix;
	value = value / radix;
	if (digit < 10) {
	    *--pos = '0' + digit;
	} else {
	    *--pos = 'a' + digit - 10;
	} /* if */
    } while (value != 0L);

    if (str != NULL) {
	memcpy(str, pos, (&buffer[64] - pos + 1) * sizeof(WCHAR));
    } /* if */
    return str;
}


/*********************************************************************
 *      _i64tow   (NTDLL.@)
 *
 * Converts a large integer to a unicode string.
 *
 * RETURNS
 *  Always returns str.
 *
 * NOTES
 *  Converts value to a '\0' terminated wstring which is copied to str.
 *  The maximum length of the copied str is 33 bytes. If radix
 *  is 10 and value is negative, the value is converted with sign.
 *  Does not check if radix is in the range of 2 to 36.
 *  If str is NULL it just returns NULL.
 *
 * DIFFERENCES
 * - The native DLL converts negative values (for base 10) wrong:
 *                     -1 is converted to -18446744073709551615
 *                     -2 is converted to -18446744073709551614
 *   -9223372036854775807 is converted to  -9223372036854775809
 *   -9223372036854775808 is converted to  -9223372036854775808
 *   The native msvcrt _i64tow function and our ntdll function do
 *   not have this bug.
 */
LPWSTR __cdecl _i64tow(
    LONGLONG value, /* [I] Value to be converted */
    LPWSTR str,     /* [O] Destination for the converted value */
    INT radix)      /* [I] Number base for conversion */
{
    ULONGLONG val;
    int negative;
    WCHAR buffer[65];
    PWCHAR pos;
    WCHAR digit;

    if (value < 0 && radix == 10) {
	negative = 1;
        val = -value;
    } else {
	negative = 0;
        val = value;
    } /* if */

    pos = &buffer[64];
    *pos = '\0';

    do {
	digit = val % radix;
	val = val / radix;
	if (digit < 10) {
	    *--pos = '0' + digit;
	} else {
	    *--pos = 'a' + digit - 10;
	} /* if */
    } while (val != 0L);

    if (negative) {
	*--pos = '-';
    } /* if */

    if (str != NULL) {
	memcpy(str, pos, (&buffer[64] - pos + 1) * sizeof(WCHAR));
    } /* if */
    return str;
}


/*********************************************************************
 *      _wtol    (NTDLL.@)
 *
 * Converts a unicode string to a long integer.
 *
 * PARAMS
 *  str [I] Wstring to be converted
 *
 * RETURNS
 *  On success it returns the integer value otherwise it returns 0.
 *
 * NOTES
 *  Accepts: {whitespace} [+|-] {digits}
 *  No check is made for value overflow, only the lower 32 bits are assigned.
 *  If str is NULL it crashes, as the native function does.
 */
__msvcrt_long __cdecl _wtol( LPCWSTR str )
{
    ULONG RunningTotal = 0;
    BOOL bMinus = FALSE;

    while (iswspace(*str)) str++;

    if (*str == '+') {
	str++;
    } else if (*str == '-') {
        bMinus = TRUE;
	str++;
    } /* if */

    while (*str >= '0' && *str <= '9') {
	RunningTotal = RunningTotal * 10 + *str - '0';
	str++;
    } /* while */

    return bMinus ? -RunningTotal : RunningTotal;
}


/*********************************************************************
 *      _wtoi    (NTDLL.@)
 *
 * Converts a unicode string to an integer.
 *
 * PARAMS
 *  str [I] Wstring to be converted
 *
 * RETURNS
 *  On success it returns the integer value otherwise it returns 0.
 *
 * NOTES
 *  Accepts: {whitespace} [+|-] {digits}
 *  No check is made for value overflow, only the lower 32 bits are assigned.
 *  If str is NULL it crashes, as the native function does.
 */
int __cdecl _wtoi( LPCWSTR str )
{
    return _wtol(str);
}


/*********************************************************************
 *      _wtoi64   (NTDLL.@)
 *
 * Converts a unicode string to a large integer.
 *
 * PARAMS
 *  str [I] Wstring to be converted
 *
 * RETURNS
 *  On success it returns the integer value otherwise it returns 0.
 *
 * NOTES
 *  Accepts: {whitespace} [+|-] {digits}
 *  No check is made for value overflow, only the lower 64 bits are assigned.
 *  If str is NULL it crashes, as the native function does.
 */
LONGLONG  __cdecl _wtoi64( LPCWSTR str )
{
    ULONGLONG RunningTotal = 0;
    BOOL bMinus = FALSE;

    while (iswspace(*str)) str++;

    if (*str == '+') {
	str++;
    } else if (*str == '-') {
        bMinus = TRUE;
	str++;
    } /* if */

    while (*str >= '0' && *str <= '9') {
	RunningTotal = RunningTotal * 10 + *str - '0';
	str++;
    } /* while */

    return bMinus ? -RunningTotal : RunningTotal;
}
