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

#define _ISOC99_SOURCE
#include "config.h"

#include <stdlib.h>
#include <errno.h>
#include "msvcrt.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/*********************************************************************
 *		_mbsdup (MSVCRT.@)
 *		_strdup (MSVCRT.@)
 */
char* CDECL _strdup(const char* str)
{
    if(str)
    {
      char * ret = MSVCRT_malloc(strlen(str)+1);
      if (ret) strcpy( ret, str );
      return ret;
    }
    else return 0;
}

/*********************************************************************
 *		_strnset (MSVCRT.@)
 */
char* CDECL MSVCRT__strnset(char* str, int value, MSVCRT_size_t len)
{
  if (len > 0 && str)
    while (*str && len--)
      *str++ = value;
  return str;
}

/*********************************************************************
 *		_strrev (MSVCRT.@)
 */
char* CDECL _strrev(char* str)
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
char* CDECL _strset(char* str, int value)
{
  char *ptr = str;
  while (*ptr)
    *ptr++ = value;

  return str;
}

/*********************************************************************
 *		strtok  (MSVCRT.@)
 */
char * CDECL MSVCRT_strtok( char *str, const char *delim )
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
void CDECL MSVCRT__swab(char* src, char* dst, int len)
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
double CDECL MSVCRT_atof( const char *str )
{
    return atof( str );
}

/*********************************************************************
 *		strtod_l  (MSVCRT.@)
 */
double CDECL MSVCRT_strtod_l( const char *str, char **end, MSVCRT__locale_t locale)
{
    const char *p, *dec_point=NULL, *exp=NULL;
    char *copy;
    double ret;
    int err = errno;

    if(!locale)
        locale = get_locale();

    if(!str) {
        MSVCRT__invalid_parameter(NULL, NULL, NULL, 0, 0);
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return 0;
    }

    /* FIXME: use *_l functions */
    p = str;
    while(isspace(*p))
        p++;
    if(*p=='+' || *p=='-')
        p++;
    while(isdigit(*p))
        p++;
    if(*p == *locale->locinfo->lconv->decimal_point) {
        if(*p!='.')
            dec_point = p;
        p++;
    }
    while(isdigit(*p))
        p++;
    if(*p=='d' || *p=='D')
        exp = p;

    /* FIXME: don't copy input string */
    if((dec_point || exp) && (copy=_strdup(str))) {
        if(dec_point)
            copy[dec_point-str] = '.';

        if(exp)
            copy[exp-str] = 'e';

        ret = strtod(copy, end);
        if(end)
            *end = (char*)str+(*end-copy);

        MSVCRT_free(copy);
    } else
        ret = strtod(str, end);

    if(err != errno)
        *MSVCRT__errno() = errno;

    return ret;

}

/*********************************************************************
 *		strtod  (MSVCRT.@)
 */
double CDECL MSVCRT_strtod( const char *str, char **end )
{
    return MSVCRT_strtod_l( str, end, NULL );
}

/*********************************************************************
 *		strcoll (MSVCRT.@)
 */
int CDECL MSVCRT_strcoll( const char* str1, const char* str2 )
{
    /* FIXME: handle Windows locale */
    return strcoll( str1, str2 );
}

/*********************************************************************
 *      strcpy_s (MSVCRT.@)
 */
int CDECL MSVCRT_strcpy_s( char* dst, MSVCRT_size_t elem, const char* src )
{
    MSVCRT_size_t i;
    if(!elem) return MSVCRT_EINVAL;
    if(!dst) return MSVCRT_EINVAL;
    if(!src)
    {
        dst[0] = '\0';
        return MSVCRT_EINVAL;
    }

    for(i = 0; i < elem; i++)
    {
        if((dst[i] = src[i]) == '\0') return 0;
    }
    dst[0] = '\0';
    return MSVCRT_ERANGE;
}

/*********************************************************************
 *      strcat_s (MSVCRT.@)
 */
int CDECL MSVCRT_strcat_s( char* dst, MSVCRT_size_t elem, const char* src )
{
    MSVCRT_size_t i, j;
    if(!dst) return MSVCRT_EINVAL;
    if(elem == 0) return MSVCRT_EINVAL;
    if(!src)
    {
        dst[0] = '\0';
        return MSVCRT_EINVAL;
    }

    for(i = 0; i < elem; i++)
    {
        if(dst[i] == '\0')
        {
            for(j = 0; (j + i) < elem; j++)
            {
                if((dst[j + i] = src[j]) == '\0') return 0;
            }
        }
    }
    /* Set the first element to 0, not the first element after the skipped part */
    dst[0] = '\0';
    return MSVCRT_ERANGE;
}

/*********************************************************************
 *		strxfrm (MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT_strxfrm( char *dest, const char *src, MSVCRT_size_t len )
{
    /* FIXME: handle Windows locale */
    return strxfrm( dest, src, len );
}

/*********************************************************************
 *		_stricoll (MSVCRT.@)
 */
int CDECL MSVCRT__stricoll( const char* str1, const char* str2 )
{
  /* FIXME: handle collates */
  TRACE("str1 %s str2 %s\n", debugstr_a(str1), debugstr_a(str2));
  return lstrcmpiA( str1, str2 );
}

/********************************************************************
 *		_atoldbl (MSVCRT.@)
 */
int CDECL MSVCRT__atoldbl(MSVCRT__LDOUBLE *value, const char *str)
{
  /* FIXME needs error checking for huge/small values */
#ifdef HAVE_STRTOLD
  TRACE("str %s value %p\n",str,value);
  value->x = strtold(str,0);
#else
  FIXME("stub, str %s value %p\n",str,value);
#endif
  return 0;
}

/********************************************************************
 *		__STRINGTOLD (MSVCRT.@)
 */
int CDECL __STRINGTOLD( MSVCRT__LDOUBLE *value, char **endptr, const char *str, int flags )
{
#ifdef HAVE_STRTOLD
    FIXME("%p %p %s %x partial stub\n", value, endptr, str, flags );
    value->x = strtold(str,endptr);
#else
    FIXME("%p %p %s %x stub\n", value, endptr, str, flags );
#endif
    return 0;
}

/******************************************************************
 *		strtol (MSVCRT.@)
 */
MSVCRT_long CDECL MSVCRT_strtol(const char* nptr, char** end, int base)
{
    /* wrapper to forward libc error code to msvcrt's error codes */
    long ret;

    errno = 0;
    ret = strtol(nptr, end, base);
    switch (errno)
    {
    case ERANGE:        *MSVCRT__errno() = MSVCRT_ERANGE;       break;
    case EINVAL:        *MSVCRT__errno() = MSVCRT_EINVAL;       break;
    default:
        /* cope with the fact that we may use 64bit long integers on libc
         * while msvcrt always uses 32bit long integers
         */
        if (ret > MSVCRT_LONG_MAX)
        {
            ret = MSVCRT_LONG_MAX;
            *MSVCRT__errno() = MSVCRT_ERANGE;
        }
        else if (ret < -MSVCRT_LONG_MAX - 1)
        {
            ret = -MSVCRT_LONG_MAX - 1;
            *MSVCRT__errno() = MSVCRT_ERANGE;
        }
        break;
    }

    return ret;
}

/******************************************************************
 *		strtoul (MSVCRT.@)
 */
MSVCRT_ulong CDECL MSVCRT_strtoul(const char* nptr, char** end, int base)
{
    /* wrapper to forward libc error code to msvcrt's error codes */
    unsigned long ret;

    errno = 0;
    ret = strtoul(nptr, end, base);
    switch (errno)
    {
    case ERANGE:        *MSVCRT__errno() = MSVCRT_ERANGE;       break;
    case EINVAL:        *MSVCRT__errno() = MSVCRT_EINVAL;       break;
    default:
        /* cope with the fact that we may use 64bit long integers on libc
         * while msvcrt always uses 32bit long integers
         */
        if (ret > MSVCRT_ULONG_MAX)
        {
            ret = MSVCRT_ULONG_MAX;
            *MSVCRT__errno() = MSVCRT_ERANGE;
        }
        break;
    }

    return ret;
}

/******************************************************************
 *              strnlen (MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT_strnlen(const char *s, MSVCRT_size_t maxlen)
{
    MSVCRT_size_t i;

    for(i=0; i<maxlen; i++)
        if(!s[i]) break;

    return i;
}

/*********************************************************************
 *  _strtoi64_l (MSVCR90.@)
 *
 * FIXME: locale parameter is ignored
 */
__int64 CDECL MSVCRT_strtoi64_l(const char *nptr, char **endptr, int base, MSVCRT__locale_t locale)
{
    BOOL negative = FALSE;
    __int64 ret = 0;

    TRACE("(%s %p %d %p)\n", nptr, endptr, base, locale);

    if(!nptr || base<0 || base>36 || base==1) {
        MSVCRT__invalid_parameter(NULL, NULL, NULL, 0, 0);
        return 0;
    }

    while(isspace(*nptr)) nptr++;

    if(*nptr == '-') {
        negative = TRUE;
        nptr++;
    } else if(*nptr == '+')
        nptr++;

    if((base==0 || base==16) && *nptr=='0' && tolower(*(nptr+1))=='x') {
        base = 16;
        nptr += 2;
    }

    if(base == 0) {
        if(*nptr=='0')
            base = 8;
        else
            base = 10;
    }

    while(*nptr) {
        char cur = tolower(*nptr);
        int v;

        if(isdigit(cur)) {
            if(cur >= '0'+base)
                break;
            v = cur-'0';
        } else {
            if(cur<'a' || cur>='a'+base-10)
                break;
            v = cur-'a'+10;
        }

        if(negative)
            v = -v;

        nptr++;

        if(!negative && (ret>MSVCRT_I64_MAX/base || ret*base>MSVCRT_I64_MAX-v)) {
            ret = MSVCRT_I64_MAX;
            *MSVCRT__errno() = ERANGE;
        } else if(negative && (ret<MSVCRT_I64_MIN/base || ret*base<MSVCRT_I64_MIN-v)) {
            ret = MSVCRT_I64_MIN;
            *MSVCRT__errno() = ERANGE;
        } else
            ret = ret*base + v;
    }

    if(endptr)
        *endptr = (char*)nptr;

    return ret;
}

/*********************************************************************
 *  _strtoi64 (MSVCR90.@)
 */
__int64 CDECL MSVCRT_strtoi64(const char *nptr, char **endptr, int base)
{
    return MSVCRT_strtoi64_l(nptr, endptr, base, NULL);
}

/*********************************************************************
 *  _strtoui64_l (MSVCR90.@)
 *
 * FIXME: locale parameter is ignored
 */
unsigned __int64 CDECL MSVCRT_strtoui64_l(const char *nptr, char **endptr, int base, MSVCRT__locale_t locale)
{
    BOOL negative = FALSE;
    unsigned __int64 ret = 0;

    TRACE("(%s %p %d %p)\n", nptr, endptr, base, locale);

    if(!nptr || base<0 || base>36 || base==1) {
        MSVCRT__invalid_parameter(NULL, NULL, NULL, 0, 0);
        return 0;
    }

    while(isspace(*nptr)) nptr++;

    if(*nptr == '-') {
        negative = TRUE;
        nptr++;
    } else if(*nptr == '+')
        nptr++;

    if((base==0 || base==16) && *nptr=='0' && tolower(*(nptr+1))=='x') {
        base = 16;
        nptr += 2;
    }

    if(base == 0) {
        if(*nptr=='0')
            base = 8;
        else
            base = 10;
    }

    while(*nptr) {
        char cur = tolower(*nptr);
        int v;

        if(isdigit(cur)) {
            if(cur >= '0'+base)
                break;
            v = *nptr-'0';
        } else {
            if(cur<'a' || cur>='a'+base-10)
                break;
            v = cur-'a'+10;
        }

        nptr++;

        if(ret>MSVCRT_UI64_MAX/base || ret*base>MSVCRT_UI64_MAX-v) {
            ret = MSVCRT_UI64_MAX;
            *MSVCRT__errno() = ERANGE;
        } else
            ret = ret*base + v;
    }

    if(endptr)
        *endptr = (char*)nptr;

    return negative ? -ret : ret;
}

/*********************************************************************
 *  _strtoui64 (MSVCR90.@)
 */
unsigned __int64 CDECL MSVCRT_strtoui64(const char *nptr, char **endptr, int base)
{
    return MSVCRT_strtoui64_l(nptr, endptr, base, NULL);
}
