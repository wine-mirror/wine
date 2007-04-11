/*
 * NTDLL string functions
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

#include "config.h"
#include "wine/port.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <search.h>

#include "windef.h"
#include "winternl.h"


/*********************************************************************
 *                  memchr   (NTDLL.@)
 */
void * __cdecl NTDLL_memchr( const void *ptr, int c, size_t n )
{
    return memchr( ptr, c, n );
}


/*********************************************************************
 *                  memcmp   (NTDLL.@)
 */
int __cdecl NTDLL_memcmp( const void *ptr1, const void *ptr2, size_t n )
{
    return memcmp( ptr1, ptr2, n );
}


/*********************************************************************
 *                  memcpy   (NTDLL.@)
 *
 * NOTES
 *  Behaves like memmove.
 */
void * __cdecl NTDLL_memcpy( void *dst, const void *src, size_t n )
{
    return memmove( dst, src, n );
}


/*********************************************************************
 *                  memmove   (NTDLL.@)
 */
void * __cdecl NTDLL_memmove( void *dst, const void *src, size_t n )
{
    return memmove( dst, src, n );
}


/*********************************************************************
 *                  memset   (NTDLL.@)
 */
void * __cdecl NTDLL_memset( void *dst, int c, size_t n )
{
    return memset( dst, c, n );
}


/*********************************************************************
 *                  bsearch   (NTDLL.@)
 */
void * __cdecl NTDLL_bsearch( const void *key, const void *base, size_t nmemb,
                              size_t size, int (*compar)(const void *, const void *) )
{
    return bsearch( key, base, nmemb, size, compar );
}


/*********************************************************************
 *                  qsort   (NTDLL.@)
 */
void __cdecl NTDLL_qsort( void *base, size_t nmemb, size_t size,
                          int(*compar)(const void *, const void *) )
{
    qsort( base, nmemb, size, compar );
}


/*********************************************************************
 *                  _lfind   (NTDLL.@)
 */
void * __cdecl _lfind( const void *key, const void *base, unsigned int *nmemb,
                       size_t size, int(*compar)(const void *, const void *) )
{
    size_t n = *nmemb;
    return lfind( key, base, &n, size, compar );
}


/*********************************************************************
 *                  strcat   (NTDLL.@)
 */
char * __cdecl NTDLL_strcat( char *dst, const char *src )
{
    return strcat( dst, src );
}


/*********************************************************************
 *                  strchr   (NTDLL.@)
 */
char * __cdecl NTDLL_strchr( const char *str, int c )
{
    return strchr( str, c );
}


/*********************************************************************
 *                  strcmp   (NTDLL.@)
 */
int __cdecl NTDLL_strcmp( const char *str1, const char *str2 )
{
    return strcmp( str1, str2 );
}


/*********************************************************************
 *                  strcpy   (NTDLL.@)
 */
char * __cdecl NTDLL_strcpy( char *dst, const char *src )
{
    return strcpy( dst, src );
}


/*********************************************************************
 *                  strcspn   (NTDLL.@)
 */
size_t __cdecl NTDLL_strcspn( const char *str, const char *reject )
{
    return strcspn( str, reject );
}


/*********************************************************************
 *                  strlen   (NTDLL.@)
 */
size_t __cdecl NTDLL_strlen( const char *str )
{
    return strlen( str );
}


/*********************************************************************
 *                  strncat   (NTDLL.@)
 */
char * __cdecl NTDLL_strncat( char *dst, const char *src, size_t len )
{
    return strncat( dst, src, len );
}


/*********************************************************************
 *                  strncmp   (NTDLL.@)
 */
int __cdecl NTDLL_strncmp( const char *str1, const char *str2, size_t len )
{
    return strncmp( str1, str2, len );
}


/*********************************************************************
 *                  strncpy   (NTDLL.@)
 */
char * __cdecl NTDLL_strncpy( char *dst, const char *src, size_t len )
{
    return strncpy( dst, src, len );
}


/*********************************************************************
 *                  strpbrk   (NTDLL.@)
 */
char * __cdecl NTDLL_strpbrk( const char *str, const char *accept )
{
    return strpbrk( str, accept );
}


/*********************************************************************
 *                  strrchr   (NTDLL.@)
 */
char * __cdecl NTDLL_strrchr( const char *str, int c )
{
    return strrchr( str, c );
}


/*********************************************************************
 *                  strspn   (NTDLL.@)
 */
size_t __cdecl NTDLL_strspn( const char *str, const char *accept )
{
    return strspn( str, accept );
}


/*********************************************************************
 *                  strstr   (NTDLL.@)
 */
char * __cdecl NTDLL_strstr( const char *haystack, const char *needle )
{
    return strstr( haystack, needle );
}


/*********************************************************************
 *                  _memccpy   (NTDLL.@)
 */
void * __cdecl _memccpy( void *dst, const void *src, int c, size_t n )
{
    return memccpy( dst, src, c, n );
}


/*********************************************************************
 *                  _memicmp   (NTDLL.@)
 *
 * Compare two blocks of memory as strings, ignoring case.
 *
 * PARAMS
 *  s1  [I] First string to compare to s2
 *  s2  [I] Second string to compare to s1
 *  len [I] Number of bytes to compare
 *
 * RETURNS
 *  An integer less than, equal to, or greater than zero indicating that
 *  s1 is less than, equal to or greater than s2 respectively.
 *
 * NOTES
 *  Any Nul characters in s1 or s2 are ignored. This function always
 *  compares up to len bytes or the first place where s1 and s2 differ.
 */
INT __cdecl _memicmp( LPCSTR s1, LPCSTR s2, DWORD len )
{
    int ret = 0;
    while (len--)
    {
        if ((ret = tolower(*s1) - tolower(*s2))) break;
        s1++;
        s2++;
    }
    return ret;
}


/*********************************************************************
 *                  _stricmp   (NTDLL.@)
 *                  _strcmpi   (NTDLL.@)
 */
int __cdecl _stricmp( LPCSTR str1, LPCSTR str2 )
{
    return strcasecmp( str1, str2 );
}


/*********************************************************************
 *                  _strnicmp   (NTDLL.@)
 */
int __cdecl _strnicmp( LPCSTR str1, LPCSTR str2, size_t n )
{
    return strncasecmp( str1, str2, n );
}


/*********************************************************************
 *                  _strupr   (NTDLL.@)
 *
 * Convert a string to upper case.
 *
 * PARAMS
 *  str [I/O] String to convert
 *
 * RETURNS
 *  str. There is no error return, if str is NULL or invalid, this
 *  function will crash.
 */
LPSTR __cdecl _strupr( LPSTR str )
{
    LPSTR ret = str;
    for ( ; *str; str++) *str = toupper(*str);
    return ret;
}


/*********************************************************************
 *                  _strlwr   (NTDLL.@)
 *
 * Convert a string to lowercase
 *
 * PARAMS
 *  str [I/O] String to convert
 *
 * RETURNS
 *  str. There is no error return, if str is NULL or invalid, this
 *  function will crash.
 */
LPSTR __cdecl _strlwr( LPSTR str )
{
    LPSTR ret = str;
    for ( ; *str; str++) *str = tolower(*str);
    return ret;
}


/*********************************************************************
 *                  tolower   (NTDLL.@)
 */
int __cdecl NTDLL_tolower( int c )
{
    return tolower( c );
}


/*********************************************************************
 *                  toupper   (NTDLL.@)
 */
int __cdecl NTDLL_toupper( int c )
{
    return toupper( c );
}


/*********************************************************************
 *                  isalnum   (NTDLL.@)
 */
int __cdecl NTDLL_isalnum( int c )
{
    return isalnum( c );
}


/*********************************************************************
 *                  isalpha   (NTDLL.@)
 */
int __cdecl NTDLL_isalpha( int c )
{
    return isalpha( c );
}


/*********************************************************************
 *                  iscntrl   (NTDLL.@)
 */
int __cdecl NTDLL_iscntrl( int c )
{
    return iscntrl( c );
}


/*********************************************************************
 *                  isdigit   (NTDLL.@)
 */
int __cdecl NTDLL_isdigit( int c )
{
    return isdigit( c );
}


/*********************************************************************
 *                  isgraph   (NTDLL.@)
 */
int __cdecl NTDLL_isgraph( int c )
{
    return isgraph( c );
}


/*********************************************************************
 *                  islower   (NTDLL.@)
 */
int __cdecl NTDLL_islower( int c )
{
    return islower( c );
}


/*********************************************************************
 *                  isprint   (NTDLL.@)
 */
int __cdecl NTDLL_isprint( int c )
{
    return isprint( c );
}


/*********************************************************************
 *                  ispunct   (NTDLL.@)
 */
int __cdecl NTDLL_ispunct( int c )
{
    return ispunct( c );
}


/*********************************************************************
 *                  isspace   (NTDLL.@)
 */
int __cdecl NTDLL_isspace( int c )
{
    return isspace( c );
}


/*********************************************************************
 *                  isupper   (NTDLL.@)
 */
int __cdecl NTDLL_isupper( int c )
{
    return isupper( c );
}


/*********************************************************************
 *                  isxdigit   (NTDLL.@)
 */
int __cdecl NTDLL_isxdigit( int c )
{
    return isxdigit( c );
}


/*********************************************************************
 *                  strtol   (NTDLL.@)
 */
long __cdecl NTDLL_strtol( const char *nptr, char **endptr, int base )
{
    return strtol( nptr, endptr, base );
}


/*********************************************************************
 *                  strtoul   (NTDLL.@)
 */
unsigned long __cdecl NTDLL_strtoul( const char *nptr, char **endptr, int base )
{
    return strtoul( nptr, endptr, base );
}


/*********************************************************************
 *                  atoi   (NTDLL.@)
 */
int __cdecl NTDLL_atoi( const char *nptr )
{
    return atoi( nptr );
}


/*********************************************************************
 *                  atol   (NTDLL.@)
 */
long __cdecl NTDLL_atol( const char *nptr )
{
    return atol( nptr );
}


/*********************************************************************
 *      _ultoa   (NTDLL.@)
 *
 * Convert an unsigned long integer to a string.
 *
 * RETURNS
 *  str.
 *
 * NOTES
 *  - Converts value to a Nul terminated string which is copied to str.
 *  - The maximum length of the copied str is 33 bytes.
 *  - Does not check if radix is in the range of 2 to 36.
 *  - If str is NULL it crashes, as the native function does.
 */
char * __cdecl _ultoa(
    unsigned long value, /* [I] Value to be converted */
    char *str,           /* [O] Destination for the converted value */
    int radix)           /* [I] Number base for conversion */
{
    char buffer[33];
    char *pos;
    int digit;

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

    memcpy(str, pos, &buffer[32] - pos + 1);
    return str;
}


/*********************************************************************
 *      _ltoa   (NTDLL.@)
 *
 * Convert a long integer to a string.
 *
 * RETURNS
 *  str.
 *
 * NOTES
 *  - Converts value to a Nul terminated string which is copied to str.
 *  - The maximum length of the copied str is 33 bytes. If radix
 *  is 10 and value is negative, the value is converted with sign.
 *  - Does not check if radix is in the range of 2 to 36.
 *  - If str is NULL it crashes, as the native function does.
 */
char * __cdecl _ltoa(
    long value, /* [I] Value to be converted */
    char *str,  /* [O] Destination for the converted value */
    int radix)  /* [I] Number base for conversion */
{
    unsigned long val;
    int negative;
    char buffer[33];
    char *pos;
    int digit;

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

    memcpy(str, pos, &buffer[32] - pos + 1);
    return str;
}


/*********************************************************************
 *      _itoa    (NTDLL.@)
 *
 * Converts an integer to a string.
 *
 * RETURNS
 *  str.
 *
 * NOTES
 *  - Converts value to a '\0' terminated string which is copied to str.
 *  - The maximum length of the copied str is 33 bytes. If radix
 *  is 10 and value is negative, the value is converted with sign.
 *  - Does not check if radix is in the range of 2 to 36.
 *  - If str is NULL it crashes, as the native function does.
 */
char * __cdecl _itoa(
    int value, /* [I] Value to be converted */
    char *str, /* [O] Destination for the converted value */
    int radix) /* [I] Number base for conversion */
{
    return _ltoa(value, str, radix);
}


/*********************************************************************
 *      _ui64toa   (NTDLL.@)
 *
 * Converts a large unsigned integer to a string.
 *
 * RETURNS
 *  str.
 *
 * NOTES
 *  - Converts value to a '\0' terminated string which is copied to str.
 *  - The maximum length of the copied str is 65 bytes.
 *  - Does not check if radix is in the range of 2 to 36.
 *  - If str is NULL it crashes, as the native function does.
 */
char * __cdecl _ui64toa(
    ULONGLONG value, /* [I] Value to be converted */
    char *str,       /* [O] Destination for the converted value */
    int radix)       /* [I] Number base for conversion */
{
    char buffer[65];
    char *pos;
    int digit;

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

    memcpy(str, pos, &buffer[64] - pos + 1);
    return str;
}


/*********************************************************************
 *      _i64toa   (NTDLL.@)
 *
 * Converts a large integer to a string.
 *
 * RETURNS
 *  str.
 *
 * NOTES
 *  - Converts value to a Nul terminated string which is copied to str.
 *  - The maximum length of the copied str is 65 bytes. If radix
 *  is 10 and value is negative, the value is converted with sign.
 *  - Does not check if radix is in the range of 2 to 36.
 *  - If str is NULL it crashes, as the native function does.
 *
 * DIFFERENCES
 * - The native DLL converts negative values (for base 10) wrong:
 *|                     -1 is converted to -18446744073709551615
 *|                     -2 is converted to -18446744073709551614
 *|   -9223372036854775807 is converted to  -9223372036854775809
 *|   -9223372036854775808 is converted to  -9223372036854775808
 *   The native msvcrt _i64toa function and our ntdll _i64toa function
 *   do not have this bug.
 */
char * __cdecl _i64toa(
    LONGLONG value, /* [I] Value to be converted */
    char *str,      /* [O] Destination for the converted value */
    int radix)      /* [I] Number base for conversion */
{
    ULONGLONG val;
    int negative;
    char buffer[65];
    char *pos;
    int digit;

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

    memcpy(str, pos, &buffer[64] - pos + 1);
    return str;
}


/*********************************************************************
 *      _atoi64   (NTDLL.@)
 *
 * Convert a string to a large integer.
 *
 * PARAMS
 *  str [I] String to be converted
 *
 * RETURNS
 *  Success: The integer value represented by str.
 *  Failure: 0. Note that this cannot be distinguished from a successful
 *           return, if the string contains "0".
 *
 * NOTES
 *  - Accepts: {whitespace} [+|-] {digits}
 *  - No check is made for value overflow, only the lower 64 bits are assigned.
 *  - If str is NULL it crashes, as the native function does.
 */
LONGLONG __cdecl _atoi64( char *str )
{
    ULONGLONG RunningTotal = 0;
    char bMinus = 0;

    while (*str == ' ' || (*str >= '\011' && *str <= '\015')) {
	str++;
    } /* while */

    if (*str == '+') {
	str++;
    } else if (*str == '-') {
	bMinus = 1;
	str++;
    } /* if */

    while (*str >= '0' && *str <= '9') {
	RunningTotal = RunningTotal * 10 + *str - '0';
	str++;
    } /* while */

    return bMinus ? -RunningTotal : RunningTotal;
}


/*********************************************************************
 *                  sprintf   (NTDLL.@)
 */
int __cdecl NTDLL_sprintf( char *str, const char *format, ... )
{
    int ret;
    va_list valist;
    va_start( valist, format );
    ret = vsprintf( str, format, valist );
    va_end( valist );
    return ret;
}


/*********************************************************************
 *                  vsprintf   (NTDLL.@)
 */
int __cdecl NTDLL_vsprintf( char *str, const char *format, va_list args )
{
    return vsprintf( str, format, args );
}


/*********************************************************************
 *                  _snprintf   (NTDLL.@)
 */
int __cdecl _snprintf( char *str, size_t len, const char *format, ... )
{
    int ret;
    va_list valist;
    va_start( valist, format );
    ret = vsnprintf( str, len, format, valist );
    va_end( valist );
    return ret;
}


/*********************************************************************
 *                  _vsnprintf   (NTDLL.@)
 */
int __cdecl _vsnprintf( char *str, size_t len, const char *format, va_list args )
{
    return vsnprintf( str, len, format, args );
}


/*********************************************************************
 *                  sscanf   (NTDLL.@)
 */
int __cdecl NTDLL_sscanf( const char *str, const char *format, ... )
{
    int ret;
    va_list valist;
    va_start( valist, format );
    ret = vsscanf( str, format, valist );
    va_end( valist );
    return ret;
}


/*********************************************************************
 *		_splitpath (NTDLL.@)
 *
 * Split a path into its component pieces.
 *
 * PARAMS
 *  inpath [I] Path to split
 *  drv    [O] Destination for drive component (e.g. "A:"). Must be at least 3 characters.
 *  dir    [O] Destination for directory component. Should be at least MAX_PATH characters.
 *  fname  [O] Destination for File name component. Should be at least MAX_PATH characters.
 *  ext    [O] Destination for file extension component. Should be at least MAX_PATH characters.
 *
 * RETURNS
 *  Nothing.
 */
void __cdecl _splitpath(const char* inpath, char * drv, char * dir,
                        char* fname, char * ext )
{
    const char *p, *end;

    if (inpath[0] && inpath[1] == ':')
    {
        if (drv)
        {
            drv[0] = inpath[0];
            drv[1] = inpath[1];
            drv[2] = 0;
        }
        inpath += 2;
    }
    else if (drv) drv[0] = 0;

    /* look for end of directory part */
    end = NULL;
    for (p = inpath; *p; p++) if (*p == '/' || *p == '\\') end = p + 1;

    if (end)  /* got a directory */
    {
        if (dir)
        {
            memcpy( dir, inpath, end - inpath );
            dir[end - inpath] = 0;
        }
        inpath = end;
    }
    else if (dir) dir[0] = 0;

    /* look for extension: what's after the last dot */
    end = NULL;
    for (p = inpath; *p; p++) if (*p == '.') end = p;

    if (!end) end = p; /* there's no extension */

    if (fname)
    {
        memcpy( fname, inpath, end - inpath );
        fname[end - inpath] = 0;
    }
    if (ext) strcpy( ext, end );
}
