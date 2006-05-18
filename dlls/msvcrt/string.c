/*
 * MSVCRT string functions
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
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

#include <stdlib.h>
#include "msvcrt.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/* INTERNAL: MSVCRT_malloc() based strndup */
char* msvcrt_strndup(const char* buf, unsigned int size)
{
  char* ret;
  unsigned int len = strlen(buf), max_len;

  max_len = size <= len? size : len + 1;

  ret = MSVCRT_malloc(max_len);
  if (ret)
  {
    memcpy(ret,buf,max_len);
    ret[max_len] = 0;
  }
  return ret;
}

/*********************************************************************
 *		_mbsdup (MSVCRT.@)
 *		_strdup (MSVCRT.@)
 */
char* _strdup(const char* str)
{
    char * ret = MSVCRT_malloc(strlen(str)+1);
    if (ret) strcpy( ret, str );
    return ret;
}

/*********************************************************************
 *		_strnset (MSVCRT.@)
 */
char* _strnset(char* str, int value, MSVCRT_size_t len)
{
  if (len > 0 && str)
    while (*str && len--)
      *str++ = value;
  return str;
}

/*********************************************************************
 *		_strrev (MSVCRT.@)
 */
char* _strrev(char* str)
{
  char * p1;
  char * p2;

  if (str && *str)
    for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2)
    {
      *p1 ^= *p2;
      *p2 ^= *p1;
      *p1 ^= *p2;
    }

  return str;
}

/*********************************************************************
 *		_strset (MSVCRT.@)
 */
char* _strset(char* str, int value)
{
  char *ptr = str;
  while (*ptr)
    *ptr++ = value;

  return str;
}

/*********************************************************************
 *		strtok  (MSVCRT.@)
 */
char *MSVCRT_strtok( char *str, const char *delim )
{
    thread_data_t *data = msvcrt_get_thread_data();
    char *ret;

    if (!str)
        if (!(str = data->strtok_next)) return NULL;

    while (*str && strchr( delim, *str )) str++;
    if (!*str) return NULL;
    ret = str++;
    while (*str && !strchr( delim, *str )) str++;
    if (*str) *str++ = 0;
    data->strtok_next = str;
    return ret;
}


/*********************************************************************
 *		_swab (MSVCRT.@)
 */
void MSVCRT__swab(char* src, char* dst, int len)
{
  if (len > 1)
  {
    len = (unsigned)len >> 1;

    while (len--) {
      char s0 = src[0];
      char s1 = src[1];
      *dst++ = s1;
      *dst++ = s0;
      src = src + 2;
    }
  }
}

/*********************************************************************
 *		atof  (MSVCRT.@)
 */
double MSVCRT_atof( const char *str )
{
    return atof( str );
}

/*********************************************************************
 *		strtod  (MSVCRT.@)
 */
double MSVCRT_strtod( const char *str, char **end )
{
    return strtod( str, end );
}

/*********************************************************************
 *		strcoll (MSVCRT.@)
 */
int MSVCRT_strcoll( const char* str1, const char* str2 )
{
    /* FIXME: handle Windows locale */
    return strcoll( str1, str2 );
}

/*********************************************************************
 *		strxfrm (MSVCRT.@)
 */
MSVCRT_size_t MSVCRT_strxfrm( char *dest, const char *src, MSVCRT_size_t len )
{
    /* FIXME: handle Windows locale */
    return strxfrm( dest, src, len );
}

/*********************************************************************
 *		_stricoll (MSVCRT.@)
 */
INT MSVCRT__stricoll( const char* str1, const char* str2 )
{
  /* FIXME: handle collates */
  TRACE("str1 %s str2 %s\n", debugstr_a(str1), debugstr_a(str2));
  return lstrcmpiA( str1, str2 );
}
