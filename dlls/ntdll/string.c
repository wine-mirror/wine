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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <ctype.h>
#include <string.h>

#include "windef.h"
#include "winternl.h"


/*********************************************************************
 *                  _memicmp   (NTDLL.@)
 */
INT __cdecl NTDLL__memicmp( LPCSTR s1, LPCSTR s2, DWORD len )
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
 *                  _strupr   (NTDLL.@)
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
 * convert a string in place to lowercase
 */
LPSTR __cdecl _strlwr( LPSTR str )
{
    LPSTR ret = str;
    for ( ; *str; str++) *str = tolower(*str);
    return ret;
}


/*********************************************************************
 *      _ultoa   (NTDLL.@)
 *
 * Converts an unsigned long integer to a string.
 *
 * RETURNS
 *  Always returns str.
 *
 * NOTES
 *  Converts value to a '\0' terminated string which is copied to str.
 *  The maximum length of the copied str is 33 bytes.
 *  Does not check if radix is in the range of 2 to 36.
 *  If str is NULL it crashes, as the native function does.
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
 * Converts a long integer to a string.
 *
 * RETURNS
 *  Always returns str.
 *
 * NOTES
 *  Converts value to a '\0' terminated string which is copied to str.
 *  The maximum length of the copied str is 33 bytes. If radix
 *  is 10 and value is negative, the value is converted with sign.
 *  Does not check if radix is in the range of 2 to 36.
 *  If str is NULL it crashes, as the native function does.
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
 *  Always returns str.
 *
 * NOTES
 *  Converts value to a '\0' terminated string which is copied to str.
 *  The maximum length of the copied str is 33 bytes. If radix
 *  is 10 and value is negative, the value is converted with sign.
 *  Does not check if radix is in the range of 2 to 36.
 *  If str is NULL it crashes, as the native function does.
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
 *  Always returns str.
 *
 * NOTES
 *  Converts value to a '\0' terminated string which is copied to str.
 *  The maximum length of the copied str is 65 bytes.
 *  Does not check if radix is in the range of 2 to 36.
 *  If str is NULL it crashes, as the native function does.
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
 *  Always returns str.
 *
 * NOTES
 *  Converts value to a '\0' terminated string which is copied to str.
 *  The maximum length of the copied str is 65 bytes. If radix
 *  is 10 and value is negative, the value is converted with sign.
 *  Does not check if radix is in the range of 2 to 36.
 *  If str is NULL it crashes, as the native function does.
 *
 * DIFFERENCES
 * - The native DLL converts negative values (for base 10) wrong:
 *                     -1 is converted to -18446744073709551615
 *                     -2 is converted to -18446744073709551614
 *   -9223372036854775807 is converted to  -9223372036854775809
 *   -9223372036854775808 is converted to  -9223372036854775808
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
 * Converts a string to a large integer.
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
 *		_splitpath (NTDLL.@)
 */
void __cdecl _splitpath(const char* inpath, char * drv, char * dir,
                        char* fname, char * ext )
{
  /* Modified PD code from 'snippets' collection. */
  char ch, *ptr, *p;
  char pathbuff[MAX_PATH], *path=pathbuff;

  strcpy(pathbuff, inpath);

  /* convert slashes to backslashes for searching */
  for (ptr = (char*)path; *ptr; ++ptr)
    if ('/' == *ptr)
      *ptr = '\\';

  /* look for drive spec */
  if ('\0' != (ptr = strchr(path, ':')))
  {
    ++ptr;
    if (drv)
    {
      strncpy(drv, path, ptr - path);
      drv[ptr - path] = '\0';
    }
    path = ptr;
  }
  else if (drv)
    *drv = '\0';

  /* find rightmost backslash or leftmost colon */
  if (NULL == (ptr = strrchr(path, '\\')))
    ptr = (strchr(path, ':'));

  if (!ptr)
  {
    ptr = (char *)path; /* no path */
    if (dir)
      *dir = '\0';
  }
  else
  {
    ++ptr; /* skip the delimiter */
    if (dir)
    {
      ch = *ptr;
      *ptr = '\0';
      strcpy(dir, path);
      *ptr = ch;
    }
  }

  if (NULL == (p = strrchr(ptr, '.')))
  {
    if (fname)
      strcpy(fname, ptr);
    if (ext)
      *ext = '\0';
  }
  else
  {
    *p = '\0';
    if (fname)
      strcpy(fname, ptr);
    *p = '.';
    if (ext)
      strcpy(ext, p);
  }

  /* Fix pathological case - Win returns ':' as part of the
   * directory when no drive letter is given.
   */
  if (drv && drv[0] == ':')
  {
    *drv = '\0';
    if (dir)
    {
      pathbuff[0] = ':';
      pathbuff[1] = '\0';
      strcat(pathbuff,dir);
      strcpy(dir,pathbuff);
    }
  }
}
