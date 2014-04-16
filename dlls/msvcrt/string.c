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
#include "wine/port.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <errno.h>
#include "msvcrt.h"
#include "winnls.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/*********************************************************************
 *		_mbsdup (MSVCRT.@)
 *		_strdup (MSVCRT.@)
 */
char* CDECL MSVCRT__strdup(const char* str)
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
 *		_strlwr_s_l (MSVCRT.@)
 */
int CDECL MSVCRT__strlwr_s_l(char *str, MSVCRT_size_t len, MSVCRT__locale_t locale)
{
    char *ptr = str;

    if (!str || !len)
    {
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return MSVCRT_EINVAL;
    }

    while (len && *ptr)
    {
        len--;
        ptr++;
    }

    if (!len)
    {
        str[0] = '\0';
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return MSVCRT_EINVAL;
    }

    while (*str)
    {
        *str = MSVCRT__tolower_l((unsigned char)*str, locale);
        str++;
    }

    return 0;
}

/*********************************************************************
 *		_strlwr_s (MSVCRT.@)
 */
int CDECL MSVCRT__strlwr_s(char *str, MSVCRT_size_t len)
{
    return MSVCRT__strlwr_s_l(str, len, NULL);
}

/*********************************************************************
 *		_strlwr_l (MSVCRT.@)
 */
char* CDECL _strlwr_l(char *str, MSVCRT__locale_t locale)
{
    MSVCRT__strlwr_s_l(str, -1, locale);
    return str;
}

/*********************************************************************
 *		_strlwr (MSVCRT.@)
 */
char* CDECL MSVCRT__strlwr(char *str)
{
    MSVCRT__strlwr_s_l(str, -1, NULL);
    return str;
}

/*********************************************************************
 *              _strupr_s_l (MSVCRT.@)
 */
int CDECL MSVCRT__strupr_s_l(char *str, MSVCRT_size_t len, MSVCRT__locale_t locale)
{
    char *ptr = str;

    if (!str || !len)
    {
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return MSVCRT_EINVAL;
    }

    while (len && *ptr)
    {
        len--;
        ptr++;
    }

    if (!len)
    {
        str[0] = '\0';
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return MSVCRT_EINVAL;
    }

    while (*str)
    {
        *str = MSVCRT__toupper_l((unsigned char)*str, locale);
        str++;
    }

    return 0;
}

/*********************************************************************
 *              _strupr_s (MSVCRT.@)
 */
int CDECL MSVCRT__strupr_s(char *str, MSVCRT_size_t len)
{
    return MSVCRT__strupr_s_l(str, len, NULL);
}

/*********************************************************************
 *              _strupr_l (MSVCRT.@)
 */
char* CDECL MSVCRT__strupr_l(char *str, MSVCRT__locale_t locale)
{
    MSVCRT__strupr_s_l(str, -1, locale);
    return str;
}

/*********************************************************************
 *              _strupr (MSVCRT.@)
 */
char* CDECL MSVCRT__strupr(char *str)
{
    MSVCRT__strupr_s_l(str, -1, NULL);
    return str;
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
char* CDECL MSVCRT__strrev(char* str)
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
 *		strtok_s  (MSVCRT.@)
 */
char * CDECL MSVCRT_strtok_s(char *str, const char *delim, char **ctx)
{
    if (!MSVCRT_CHECK_PMT(delim != NULL)) return NULL;
    if (!MSVCRT_CHECK_PMT(ctx != NULL)) return NULL;
    if (!MSVCRT_CHECK_PMT(str != NULL || *ctx != NULL)) return NULL;

    if(!str)
        str = *ctx;

    while(*str && strchr(delim, *str))
        str++;
    if(!*str)
        return NULL;

    *ctx = str+1;
    while(**ctx && !strchr(delim, **ctx))
        (*ctx)++;
    if(**ctx)
        *(*ctx)++ = 0;

    return str;
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

static double strtod_helper(const char *str, char **end, MSVCRT__locale_t locale, int *err)
{
    MSVCRT_pthreadlocinfo locinfo;
    unsigned __int64 d=0, hlp;
    unsigned fpcontrol;
    int exp=0, sign=1;
    const char *p;
    double ret;
    long double lret=1, expcnt = 10;
    BOOL found_digit = FALSE, negexp;

    if(err)
        *err = 0;
    else
        if(!MSVCRT_CHECK_PMT(str != NULL)) return 0;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    /* FIXME: use *_l functions */
    p = str;
    while(isspace(*p))
        p++;

    if(*p == '-') {
        sign = -1;
        p++;
    } else  if(*p == '+')
        p++;

    while(isdigit(*p)) {
        found_digit = TRUE;
        hlp = d*10+*(p++)-'0';
        if(d>MSVCRT_UI64_MAX/10 || hlp<d) {
            exp++;
            break;
        } else
            d = hlp;
    }
    while(isdigit(*p)) {
        exp++;
        p++;
    }

    if(*p == *locinfo->lconv->decimal_point)
        p++;

    while(isdigit(*p)) {
        found_digit = TRUE;
        hlp = d*10+*(p++)-'0';
        if(d>MSVCRT_UI64_MAX/10 || hlp<d)
            break;

        d = hlp;
        exp--;
    }
    while(isdigit(*p))
        p++;

    if(!found_digit) {
        if(end)
            *end = (char*)str;
        return 0.0;
    }

    if(*p=='e' || *p=='E' || *p=='d' || *p=='D') {
        int e=0, s=1;

        p++;
        if(*p == '-') {
            s = -1;
            p++;
        } else if(*p == '+')
            p++;

        if(isdigit(*p)) {
            while(isdigit(*p)) {
                if(e>INT_MAX/10 || (e=e*10+*p-'0')<0)
                    e = INT_MAX;
                p++;
            }
            e *= s;

            if(exp<0 && e<0 && exp+e>=0) exp = INT_MIN;
            else if(exp>0 && e>0 && exp+e<0) exp = INT_MAX;
            else exp += e;
        } else {
            if(*p=='-' || *p=='+')
                p--;
            p--;
        }
    }

    fpcontrol = _control87(0, 0);
    _control87(MSVCRT__EM_DENORMAL|MSVCRT__EM_INVALID|MSVCRT__EM_ZERODIVIDE
            |MSVCRT__EM_OVERFLOW|MSVCRT__EM_UNDERFLOW|MSVCRT__EM_INEXACT, 0xffffffff);

    negexp = (exp < 0);
    if(negexp)
        exp = -exp;
    while(exp) {
        if(exp & 1)
            lret *= expcnt;
        exp /= 2;
        expcnt = expcnt*expcnt;
    }
    ret = (long double)sign * (negexp ? d/lret : d*lret);

    _control87(fpcontrol, 0xffffffff);

    if((d && ret==0.0) || isinf(ret)) {
        if(err)
            *err = MSVCRT_ERANGE;
        else
            *MSVCRT__errno() = MSVCRT_ERANGE;
    }

    if(end)
        *end = (char*)p;

    return ret;
}

/*********************************************************************
 *		strtod_l  (MSVCRT.@)
 */
double CDECL MSVCRT_strtod_l(const char *str, char **end, MSVCRT__locale_t locale)
{
    return strtod_helper(str, end, locale, NULL);
}

/*********************************************************************
 *		strtod  (MSVCRT.@)
 */
double CDECL MSVCRT_strtod( const char *str, char **end )
{
    return MSVCRT_strtod_l( str, end, NULL );
}

/*********************************************************************
 *		atof  (MSVCRT.@)
 */
double CDECL MSVCRT_atof( const char *str )
{
    return MSVCRT_strtod_l(str, NULL, NULL);
}

/*********************************************************************
 *		_atof_l  (MSVCRT.@)
 */
double CDECL MSVCRT__atof_l( const char *str, MSVCRT__locale_t locale)
{
    return MSVCRT_strtod_l(str, NULL, locale);
}

/*********************************************************************
 *		_atoflt_l  (MSVCRT.@)
 */
int CDECL MSVCRT__atoflt_l( MSVCRT__CRT_FLOAT *value, char *str, MSVCRT__locale_t locale)
{
    double d;
    int err;

    d = strtod_helper(str, NULL, locale, &err);
    value->f = d;
    if(isinf(value->f))
        return MSVCRT__OVERFLOW;
    if((d!=0 || err) && value->f>-MSVCRT_FLT_MIN && value->f<MSVCRT_FLT_MIN)
        return MSVCRT__UNDERFLOW;
    return 0;
}

/*********************************************************************
 *              _atodbl_l  (MSVCRT.@)
 */
int CDECL MSVCRT__atodbl_l(MSVCRT__CRT_DOUBLE *value, char *str, MSVCRT__locale_t locale)
{
    int err;

    value->x = strtod_helper(str, NULL, locale, &err);
    if(isinf(value->x))
        return MSVCRT__OVERFLOW;
    if((value->x!=0 || err) && value->x>-MSVCRT_DBL_MIN && value->x<MSVCRT_DBL_MIN)
        return MSVCRT__UNDERFLOW;
    return 0;
}

/*********************************************************************
 *              _atodbl  (MSVCRT.@)
 */
int CDECL MSVCRT__atodbl(MSVCRT__CRT_DOUBLE *value, char *str)
{
    return MSVCRT__atodbl_l(value, str, NULL);
}

/*********************************************************************
 *		_strcoll_l (MSVCRT.@)
 */
int CDECL MSVCRT_strcoll_l( const char* str1, const char* str2, MSVCRT__locale_t locale )
{
    MSVCRT_pthreadlocinfo locinfo;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!locinfo->lc_handle[MSVCRT_LC_COLLATE])
        return strcmp(str1, str2);
    return CompareStringA(locinfo->lc_handle[MSVCRT_LC_COLLATE], 0, str1, -1, str2, -1)-CSTR_EQUAL;
}

/*********************************************************************
 *		strcoll (MSVCRT.@)
 */
int CDECL MSVCRT_strcoll( const char* str1, const char* str2 )
{
    return MSVCRT_strcoll_l(str1, str2, NULL);
}

/*********************************************************************
 *		_stricoll_l (MSVCRT.@)
 */
int CDECL MSVCRT__stricoll_l( const char* str1, const char* str2, MSVCRT__locale_t locale )
{
    MSVCRT_pthreadlocinfo locinfo;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!locinfo->lc_handle[MSVCRT_LC_COLLATE])
        return strcasecmp(str1, str2);
    return CompareStringA(locinfo->lc_handle[MSVCRT_LC_COLLATE], NORM_IGNORECASE,
            str1, -1, str2, -1)-CSTR_EQUAL;
}

/*********************************************************************
 *		_stricoll (MSVCRT.@)
 */
int CDECL MSVCRT__stricoll( const char* str1, const char* str2 )
{
    return MSVCRT__stricoll_l(str1, str2, NULL);
}

/*********************************************************************
 *              _strncoll_l (MSVCRT.@)
 */
int CDECL MSVCRT__strncoll_l( const char* str1, const char* str2, MSVCRT_size_t count, MSVCRT__locale_t locale )
{
    MSVCRT_pthreadlocinfo locinfo;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!locinfo->lc_handle[MSVCRT_LC_COLLATE])
        return strncmp(str1, str2, count);
    return CompareStringA(locinfo->lc_handle[MSVCRT_LC_COLLATE], 0, str1, count, str2, count)-CSTR_EQUAL;
}

/*********************************************************************
 *              _strncoll (MSVCRT.@)
 */
int CDECL MSVCRT__strncoll( const char* str1, const char* str2, MSVCRT_size_t count )
{
    return MSVCRT__strncoll_l(str1, str2, count, NULL);
}

/*********************************************************************
 *              _strnicoll_l (MSVCRT.@)
 */
int CDECL MSVCRT__strnicoll_l( const char* str1, const char* str2, MSVCRT_size_t count, MSVCRT__locale_t locale )
{
    MSVCRT_pthreadlocinfo locinfo;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!locinfo->lc_handle[MSVCRT_LC_COLLATE])
        return strncasecmp(str1, str2, count);
    return CompareStringA(locinfo->lc_handle[MSVCRT_LC_COLLATE], NORM_IGNORECASE,
            str1, count, str2, count)-CSTR_EQUAL;
}

/*********************************************************************
 *              _strnicoll (MSVCRT.@)
 */
int CDECL MSVCRT__strnicoll( const char* str1, const char* str2, MSVCRT_size_t count )
{
    return MSVCRT__strnicoll_l(str1, str2, count, NULL);
}

/*********************************************************************
 *                  strncpy (MSVCRT.@)
 */
char* __cdecl MSVCRT_strncpy(char *dst, const char *src, MSVCRT_size_t len)
{
    MSVCRT_size_t i;

    for(i=0; i<len; i++)
        if((dst[i] = src[i]) == '\0') break;

    while (i < len) dst[i++] = 0;

    return dst;
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
 *      strncat_s (MSVCRT.@)
 */
int CDECL MSVCRT_strncat_s( char* dst, MSVCRT_size_t elem, const char* src, MSVCRT_size_t count )
{
    MSVCRT_size_t i, j;

    if (!MSVCRT_CHECK_PMT(dst != 0)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(elem != 0)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(src != 0))
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
                if(count == MSVCRT__TRUNCATE && j + i == elem - 1)
                {
                    dst[j + i] = '\0';
                    return MSVCRT_STRUNCATE;
                }
                if(j == count || (dst[j + i] = src[j]) == '\0')
                {
                    dst[j + i] = '\0';
                    return 0;
                }
            }
        }
    }
    /* Set the first element to 0, not the first element after the skipped part */
    dst[0] = '\0';
    return MSVCRT_ERANGE;
}

/*********************************************************************
 *      strncat (MSVCRT.@)
 */
char* __cdecl MSVCRT_strncat(char *dst, const char *src, MSVCRT_size_t len)
{
    return strncat(dst, src, len);
}

/*********************************************************************
 *		strxfrm (MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT_strxfrm( char *dest, const char *src, MSVCRT_size_t len )
{
    /* FIXME: handle Windows locale */
    return strxfrm( dest, src, len );
}

/********************************************************************
 *		_atoldbl (MSVCRT.@)
 */
int CDECL MSVCRT__atoldbl(MSVCRT__LDOUBLE *value, const char *str)
{
  /* FIXME needs error checking for huge/small values */
#ifdef HAVE_STRTOLD
  long double ld;
  TRACE("str %s value %p\n",str,value);
  ld = strtold(str,0);
  memcpy(value, &ld, 10);
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
    long double ld;
    FIXME("%p %p %s %x partial stub\n", value, endptr, str, flags );
    ld = strtold(str,0);
    memcpy(value, &ld, 10);
#else
    FIXME("%p %p %s %x stub\n", value, endptr, str, flags );
#endif
    return 0;
}

/*********************************************************************
 *              strlen (MSVCRT.@)
 */
MSVCRT_size_t __cdecl MSVCRT_strlen(const char *str)
{
    return strlen(str);
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
 *  _strtoi64_l (MSVCRT.@)
 *
 * FIXME: locale parameter is ignored
 */
__int64 CDECL MSVCRT_strtoi64_l(const char *nptr, char **endptr, int base, MSVCRT__locale_t locale)
{
    BOOL negative = FALSE;
    __int64 ret = 0;

    TRACE("(%s %p %d %p)\n", debugstr_a(nptr), endptr, base, locale);

    if (!MSVCRT_CHECK_PMT(nptr != NULL)) return 0;
    if (!MSVCRT_CHECK_PMT(base == 0 || base >= 2)) return 0;
    if (!MSVCRT_CHECK_PMT(base <= 36)) return 0;

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
            *MSVCRT__errno() = MSVCRT_ERANGE;
        } else if(negative && (ret<MSVCRT_I64_MIN/base || ret*base<MSVCRT_I64_MIN-v)) {
            ret = MSVCRT_I64_MIN;
            *MSVCRT__errno() = MSVCRT_ERANGE;
        } else
            ret = ret*base + v;
    }

    if(endptr)
        *endptr = (char*)nptr;

    return ret;
}

/*********************************************************************
 *  _strtoi64 (MSVCRT.@)
 */
__int64 CDECL MSVCRT_strtoi64(const char *nptr, char **endptr, int base)
{
    return MSVCRT_strtoi64_l(nptr, endptr, base, NULL);
}

/*********************************************************************
 *  _atoi_l (MSVCRT.@)
 */
int __cdecl MSVCRT__atoi_l(const char *str, MSVCRT__locale_t locale)
{
    __int64 ret = MSVCRT_strtoi64_l(str, NULL, 10, locale);

    if(ret > INT_MAX) {
        ret = INT_MAX;
        *MSVCRT__errno() = MSVCRT_ERANGE;
    } else if(ret < INT_MIN) {
        ret = INT_MIN;
        *MSVCRT__errno() = MSVCRT_ERANGE;
    }
    return ret;
}

/*********************************************************************
 *  atoi (MSVCRT.@)
 */
int __cdecl MSVCRT_atoi(const char *str)
{
    BOOL minus = FALSE;
    int ret = 0;

    if(!str)
        return 0;

    while(isspace(*str)) str++;

    if(*str == '+') {
        str++;
    }else if(*str == '-') {
        minus = TRUE;
        str++;
    }

    while(*str>='0' && *str<='9') {
        ret = ret*10+*str-'0';
        str++;
    }

    return minus ? -ret : ret;
}

/******************************************************************
 *		strtol (MSVCRT.@)
 */
MSVCRT_long CDECL MSVCRT_strtol(const char* nptr, char** end, int base)
{
    __int64 ret = MSVCRT_strtoi64_l(nptr, end, base, NULL);

    if(ret > MSVCRT_LONG_MAX) {
        ret = MSVCRT_LONG_MAX;
        *MSVCRT__errno() = MSVCRT_ERANGE;
    } else if(ret < MSVCRT_LONG_MIN) {
        ret = MSVCRT_LONG_MIN;
        *MSVCRT__errno() = MSVCRT_ERANGE;
    }

    return ret;
}

/******************************************************************
 *		strtoul (MSVCRT.@)
 */
MSVCRT_ulong CDECL MSVCRT_strtoul(const char* nptr, char** end, int base)
{
    __int64 ret = MSVCRT_strtoi64_l(nptr, end, base, NULL);

    if(ret > MSVCRT_ULONG_MAX) {
        ret = MSVCRT_ULONG_MAX;
        *MSVCRT__errno() = MSVCRT_ERANGE;
    }else if(ret < -(__int64)MSVCRT_ULONG_MAX) {
        ret = 1;
        *MSVCRT__errno() = MSVCRT_ERANGE;
    }

    return ret;
}

/*********************************************************************
 *  _strtoui64_l (MSVCRT.@)
 *
 * FIXME: locale parameter is ignored
 */
unsigned __int64 CDECL MSVCRT_strtoui64_l(const char *nptr, char **endptr, int base, MSVCRT__locale_t locale)
{
    BOOL negative = FALSE;
    unsigned __int64 ret = 0;

    TRACE("(%s %p %d %p)\n", debugstr_a(nptr), endptr, base, locale);

    if (!MSVCRT_CHECK_PMT(nptr != NULL)) return 0;
    if (!MSVCRT_CHECK_PMT(base == 0 || base >= 2)) return 0;
    if (!MSVCRT_CHECK_PMT(base <= 36)) return 0;

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
            *MSVCRT__errno() = MSVCRT_ERANGE;
        } else
            ret = ret*base + v;
    }

    if(endptr)
        *endptr = (char*)nptr;

    return negative ? -ret : ret;
}

/*********************************************************************
 *  _strtoui64 (MSVCRT.@)
 */
unsigned __int64 CDECL MSVCRT_strtoui64(const char *nptr, char **endptr, int base)
{
    return MSVCRT_strtoui64_l(nptr, endptr, base, NULL);
}

static int ltoa_helper(MSVCRT_long value, char *str, MSVCRT_size_t size, int radix)
{
    MSVCRT_ulong val;
    unsigned int digit;
    BOOL is_negative;
    char buffer[33], *pos;
    size_t len;

    if (value < 0 && radix == 10)
    {
        is_negative = TRUE;
        val = -value;
    }
    else
    {
        is_negative = FALSE;
        val = value;
    }

    pos = buffer + 32;
    *pos = '\0';

    do
    {
        digit = val % radix;
        val /= radix;

        if (digit < 10)
            *--pos = '0' + digit;
        else
            *--pos = 'a' + digit - 10;
    }
    while (val != 0);

    if (is_negative)
        *--pos = '-';

    len = buffer + 33 - pos;
    if (len > size)
    {
        size_t i;
        char *p = str;

        /* Copy the temporary buffer backwards up to the available number of
         * characters. Don't copy the negative sign if present. */

        if (is_negative)
        {
            p++;
            size--;
        }

        for (pos = buffer + 31, i = 0; i < size; i++)
            *p++ = *pos--;

        str[0] = '\0';
        MSVCRT_INVALID_PMT("str[size] is too small", MSVCRT_ERANGE);
        return MSVCRT_ERANGE;
    }

    memcpy(str, pos, len);
    return 0;
}

/*********************************************************************
 *  _ltoa_s (MSVCRT.@)
 */
int CDECL MSVCRT__ltoa_s(MSVCRT_long value, char *str, MSVCRT_size_t size, int radix)
{
    if (!MSVCRT_CHECK_PMT(str != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(size > 0)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(radix >= 2 && radix <= 36))
    {
        str[0] = '\0';
        return MSVCRT_EINVAL;
    }

    return ltoa_helper(value, str, size, radix);
}

/*********************************************************************
 *  _ltow_s (MSVCRT.@)
 */
int CDECL MSVCRT__ltow_s(MSVCRT_long value, MSVCRT_wchar_t *str, MSVCRT_size_t size, int radix)
{
    MSVCRT_ulong val;
    unsigned int digit;
    BOOL is_negative;
    MSVCRT_wchar_t buffer[33], *pos;
    size_t len;

    if (!MSVCRT_CHECK_PMT(str != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(size > 0)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(radix >= 2 && radix <= 36))
    {
        str[0] = '\0';
        return MSVCRT_EINVAL;
    }

    if (value < 0 && radix == 10)
    {
        is_negative = TRUE;
        val = -value;
    }
    else
    {
        is_negative = FALSE;
        val = value;
    }

    pos = buffer + 32;
    *pos = '\0';

    do
    {
        digit = val % radix;
        val /= radix;

        if (digit < 10)
            *--pos = '0' + digit;
        else
            *--pos = 'a' + digit - 10;
    }
    while (val != 0);

    if (is_negative)
        *--pos = '-';

    len = buffer + 33 - pos;
    if (len > size)
    {
        size_t i;
        MSVCRT_wchar_t *p = str;

        /* Copy the temporary buffer backwards up to the available number of
         * characters. Don't copy the negative sign if present. */

        if (is_negative)
        {
            p++;
            size--;
        }

        for (pos = buffer + 31, i = 0; i < size; i++)
            *p++ = *pos--;

        str[0] = '\0';
        MSVCRT_INVALID_PMT("str[size] is too small", MSVCRT_ERANGE);
        return MSVCRT_ERANGE;
    }

    memcpy(str, pos, len * sizeof(MSVCRT_wchar_t));
    return 0;
}

/*********************************************************************
 *  _itoa_s (MSVCRT.@)
 */
int CDECL MSVCRT__itoa_s(int value, char *str, MSVCRT_size_t size, int radix)
{
    return MSVCRT__ltoa_s(value, str, size, radix);
}

/*********************************************************************
 *  _itoa (MSVCRT.@)
 */
char* CDECL MSVCRT__itoa(int value, char *str, int radix)
{
    return ltoa_helper(value, str, MSVCRT_SIZE_MAX, radix) ? NULL : str;
}

/*********************************************************************
 *  _itow_s (MSVCRT.@)
 */
int CDECL MSVCRT__itow_s(int value, MSVCRT_wchar_t *str, MSVCRT_size_t size, int radix)
{
    return MSVCRT__ltow_s(value, str, size, radix);
}

/*********************************************************************
 *  _ui64toa_s (MSVCRT.@)
 */
int CDECL MSVCRT__ui64toa_s(unsigned __int64 value, char *str,
        MSVCRT_size_t size, int radix)
{
    char buffer[65], *pos;
    int digit;

    if (!MSVCRT_CHECK_PMT(str != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(size > 0)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(radix >= 2 && radix <= 36))
    {
        str[0] = '\0';
        return MSVCRT_EINVAL;
    }

    pos = buffer+64;
    *pos = '\0';

    do {
        digit = value%radix;
        value /= radix;

        if(digit < 10)
            *--pos = '0'+digit;
        else
            *--pos = 'a'+digit-10;
    }while(value != 0);

    if(buffer-pos+65 > size) {
        MSVCRT_INVALID_PMT("str[size] is too small", MSVCRT_EINVAL);
        return MSVCRT_EINVAL;
    }

    memcpy(str, pos, buffer-pos+65);
    return 0;
}

/*********************************************************************
 *      _ui64tow_s  (MSVCRT.@)
 */
int CDECL MSVCRT__ui64tow_s( unsigned __int64 value, MSVCRT_wchar_t *str,
                             MSVCRT_size_t size, int radix )
{
    MSVCRT_wchar_t buffer[65], *pos;
    int digit;

    if (!MSVCRT_CHECK_PMT(str != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(size > 0)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(radix >= 2 && radix <= 36))
    {
        str[0] = '\0';
        return MSVCRT_EINVAL;
    }

    pos = &buffer[64];
    *pos = '\0';

    do {
	digit = value % radix;
	value = value / radix;
	if (digit < 10)
	    *--pos = '0' + digit;
	else
	    *--pos = 'a' + digit - 10;
    } while (value != 0);

    if(buffer-pos+65 > size) {
        MSVCRT_INVALID_PMT("str[size] is too small", MSVCRT_EINVAL);
        return MSVCRT_EINVAL;
    }

    memcpy(str, pos, (buffer-pos+65)*sizeof(MSVCRT_wchar_t));
    return 0;
}

/*********************************************************************
 *  _ultoa_s (MSVCRT.@)
 */
int CDECL MSVCRT__ultoa_s(MSVCRT_ulong value, char *str, MSVCRT_size_t size, int radix)
{
    MSVCRT_ulong digit;
    char buffer[33], *pos;
    size_t len;

    if (!str || !size || radix < 2 || radix > 36)
    {
        if (str && size)
            str[0] = '\0';

        *MSVCRT__errno() = MSVCRT_EINVAL;
        return MSVCRT_EINVAL;
    }

    pos = buffer + 32;
    *pos = '\0';

    do
    {
        digit = value % radix;
        value /= radix;

        if (digit < 10)
            *--pos = '0' + digit;
        else
            *--pos = 'a' + digit - 10;
    }
    while (value != 0);

    len = buffer + 33 - pos;
    if (len > size)
    {
        size_t i;
        char *p = str;

        /* Copy the temporary buffer backwards up to the available number of
         * characters. */

        for (pos = buffer + 31, i = 0; i < size; i++)
            *p++ = *pos--;

        str[0] = '\0';
        *MSVCRT__errno() = MSVCRT_ERANGE;
        return MSVCRT_ERANGE;
    }

    memcpy(str, pos, len);
    return 0;
}

/*********************************************************************
 *  _ultow_s (MSVCRT.@)
 */
int CDECL MSVCRT__ultow_s(MSVCRT_ulong value, MSVCRT_wchar_t *str, MSVCRT_size_t size, int radix)
{
    MSVCRT_ulong digit;
    WCHAR buffer[33], *pos;
    size_t len;

    if (!str || !size || radix < 2 || radix > 36)
    {
        if (str && size)
            str[0] = '\0';

        *MSVCRT__errno() = MSVCRT_EINVAL;
        return MSVCRT_EINVAL;
    }

    pos = buffer + 32;
    *pos = '\0';

    do
    {
        digit = value % radix;
        value /= radix;

        if (digit < 10)
            *--pos = '0' + digit;
        else
            *--pos = 'a' + digit - 10;
    }
    while (value != 0);

    len = buffer + 33 - pos;
    if (len > size)
    {
        size_t i;
        WCHAR *p = str;

        /* Copy the temporary buffer backwards up to the available number of
         * characters. */

        for (pos = buffer + 31, i = 0; i < size; i++)
            *p++ = *pos--;

        str[0] = '\0';
        *MSVCRT__errno() = MSVCRT_ERANGE;
        return MSVCRT_ERANGE;
    }

    memcpy(str, pos, len * sizeof(MSVCRT_wchar_t));
    return 0;
}

/*********************************************************************
 *  _i64toa_s (MSVCRT.@)
 */
int CDECL MSVCRT__i64toa_s(__int64 value, char *str, MSVCRT_size_t size, int radix)
{
    unsigned __int64 val;
    unsigned int digit;
    BOOL is_negative;
    char buffer[65], *pos;
    size_t len;

    if (!MSVCRT_CHECK_PMT(str != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(size > 0)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(radix >= 2 && radix <= 36))
    {
        str[0] = '\0';
        return MSVCRT_EINVAL;
    }

    if (value < 0 && radix == 10)
    {
        is_negative = TRUE;
        val = -value;
    }
    else
    {
        is_negative = FALSE;
        val = value;
    }

    pos = buffer + 64;
    *pos = '\0';

    do
    {
        digit = val % radix;
        val /= radix;

        if (digit < 10)
            *--pos = '0' + digit;
        else
            *--pos = 'a' + digit - 10;
    }
    while (val != 0);

    if (is_negative)
        *--pos = '-';

    len = buffer + 65 - pos;
    if (len > size)
    {
        size_t i;
        char *p = str;

        /* Copy the temporary buffer backwards up to the available number of
         * characters. Don't copy the negative sign if present. */

        if (is_negative)
        {
            p++;
            size--;
        }

        for (pos = buffer + 63, i = 0; i < size; i++)
            *p++ = *pos--;

        str[0] = '\0';
        MSVCRT_INVALID_PMT("str[size] is too small", MSVCRT_ERANGE);
        return MSVCRT_ERANGE;
    }

    memcpy(str, pos, len);
    return 0;
}

/*********************************************************************
 *  _i64tow_s (MSVCRT.@)
 */
int CDECL MSVCRT__i64tow_s(__int64 value, MSVCRT_wchar_t *str, MSVCRT_size_t size, int radix)
{
    unsigned __int64 val;
    unsigned int digit;
    BOOL is_negative;
    MSVCRT_wchar_t buffer[65], *pos;
    size_t len;

    if (!MSVCRT_CHECK_PMT(str != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(size > 0)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(radix >= 2 && radix <= 36))
    {
        str[0] = '\0';
        return MSVCRT_EINVAL;
    }

    if (value < 0 && radix == 10)
    {
        is_negative = TRUE;
        val = -value;
    }
    else
    {
        is_negative = FALSE;
        val = value;
    }

    pos = buffer + 64;
    *pos = '\0';

    do
    {
        digit = val % radix;
        val /= radix;

        if (digit < 10)
            *--pos = '0' + digit;
        else
            *--pos = 'a' + digit - 10;
    }
    while (val != 0);

    if (is_negative)
        *--pos = '-';

    len = buffer + 65 - pos;
    if (len > size)
    {
        size_t i;
        MSVCRT_wchar_t *p = str;

        /* Copy the temporary buffer backwards up to the available number of
         * characters. Don't copy the negative sign if present. */

        if (is_negative)
        {
            p++;
            size--;
        }

        for (pos = buffer + 63, i = 0; i < size; i++)
            *p++ = *pos--;

        str[0] = '\0';
        MSVCRT_INVALID_PMT("str[size] is too small", MSVCRT_ERANGE);
        return MSVCRT_ERANGE;
    }

    memcpy(str, pos, len * sizeof(MSVCRT_wchar_t));
    return 0;
}

#define I10_OUTPUT_MAX_PREC 21
/* Internal structure used by $I10_OUTPUT */
struct _I10_OUTPUT_DATA {
    short pos;
    char sign;
    BYTE len;
    char str[I10_OUTPUT_MAX_PREC+1]; /* add space for '\0' */
};

/*********************************************************************
 *              $I10_OUTPUT (MSVCRT.@)
 * ld80 - long double (Intel 80 bit FP in 12 bytes) to be printed to data
 * prec - precision of part, we're interested in
 * flag - 0 for first prec digits, 1 for fractional part
 * data - data to be populated
 *
 * return value
 *      0 if given double is NaN or INF
 *      1 otherwise
 *
 * FIXME
 *      Native sets last byte of data->str to '0' or '9', I don't know what
 *      it means. Current implementation sets it always to '0'.
 */
int CDECL MSVCRT_I10_OUTPUT(MSVCRT__LDOUBLE ld80, int prec, int flag, struct _I10_OUTPUT_DATA *data)
{
    static const char inf_str[] = "1#INF";
    static const char nan_str[] = "1#QNAN";

    /* MS' long double type wants 12 bytes for Intel's 80 bit FP format.
     * Some UNIX have sizeof(long double) == 16, yet only 80 bit are used.
     * Assume long double uses 80 bit FP, never seen 128 bit FP. */
    long double ld = 0;
    double d;
    char format[8];
    char buf[I10_OUTPUT_MAX_PREC+9]; /* 9 = strlen("0.e+0000") + '\0' */
    char *p;

    memcpy(&ld, &ld80, 10);
    d = ld;
    TRACE("(%lf %d %x %p)\n", d, prec, flag, data);

    if(d<0) {
        data->sign = '-';
        d = -d;
    } else
        data->sign = ' ';

    if(isinf(d)) {
        data->pos = 1;
        data->len = 5;
        memcpy(data->str, inf_str, sizeof(inf_str));

        return 0;
    }

    if(isnan(d)) {
        data->pos = 1;
        data->len = 6;
        memcpy(data->str, nan_str, sizeof(nan_str));

        return 0;
    }

    if(flag&1) {
        int exp = 1+floor(log10(d));

        prec += exp;
        if(exp < 0)
            prec--;
    }
    prec--;

    if(prec+1 > I10_OUTPUT_MAX_PREC)
        prec = I10_OUTPUT_MAX_PREC-1;
    else if(prec < 0) {
        d = 0.0;
        prec = 0;
    }

    sprintf(format, "%%.%dle", prec);
    sprintf(buf, format, d);

    buf[1] = buf[0];
    data->pos = atoi(buf+prec+3);
    if(buf[1] != '0')
        data->pos++;

    for(p = buf+prec+1; p>buf+1 && *p=='0'; p--);
    data->len = p-buf;

    memcpy(data->str, buf+1, data->len);
    data->str[data->len] = '\0';

    if(buf[1]!='0' && prec-data->len+1>0)
        memcpy(data->str+data->len+1, buf+data->len+1, prec-data->len+1);

    return 1;
}
#undef I10_OUTPUT_MAX_PREC

/*********************************************************************
 *                  memcmp (MSVCRT.@)
 */
int __cdecl MSVCRT_memcmp(const void *ptr1, const void *ptr2, MSVCRT_size_t n)
{
    return memcmp(ptr1, ptr2, n);
}

/*********************************************************************
 *                  memcpy   (MSVCRT.@)
 */
void * __cdecl MSVCRT_memcpy(void *dst, const void *src, MSVCRT_size_t n)
{
    return memmove(dst, src, n);
}

/*********************************************************************
 *                  memmove (MSVCRT.@)
 */
void * __cdecl MSVCRT_memmove(void *dst, const void *src, MSVCRT_size_t n)
{
    return memmove(dst, src, n);
}

/*********************************************************************
 *		    memset (MSVCRT.@)
 */
void* __cdecl MSVCRT_memset(void *dst, int c, MSVCRT_size_t n)
{
    return memset(dst, c, n);
}

/*********************************************************************
 *		    strchr (MSVCRT.@)
 */
char* __cdecl MSVCRT_strchr(const char *str, int c)
{
    return strchr(str, c);
}

/*********************************************************************
 *                  strrchr (MSVCRT.@)
 */
char* __cdecl MSVCRT_strrchr(const char *str, int c)
{
    return strrchr(str, c);
}

/*********************************************************************
 *                  memchr   (MSVCRT.@)
 */
void* __cdecl MSVCRT_memchr(const void *ptr, int c, MSVCRT_size_t n)
{
    return memchr(ptr, c, n);
}

/*********************************************************************
 *                  strcmp (MSVCRT.@)
 */
int __cdecl MSVCRT_strcmp(const char *str1, const char *str2)
{
    return strcmp(str1, str2);
}

/*********************************************************************
 *                  strncmp   (MSVCRT.@)
 */
int __cdecl MSVCRT_strncmp(const char *str1, const char *str2, MSVCRT_size_t len)
{
    return strncmp(str1, str2, len);
}

/*********************************************************************
 *                  _strnicmp_l   (MSVCRT.@)
 */
int __cdecl MSVCRT__strnicmp_l(const char *s1, const char *s2,
        MSVCRT_size_t count, MSVCRT__locale_t locale)
{
    MSVCRT_pthreadlocinfo locinfo;
    char c1, c2;

    if(s1==NULL || s2==NULL)
        return MSVCRT__NLSCMPERROR;

    if(!count)
        return 0;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!locinfo->lc_handle[MSVCRT_LC_CTYPE])
        return strncasecmp(s1, s2, count);

    do {
        c1 = MSVCRT__tolower_l(*s1++, locale);
        c2 = MSVCRT__tolower_l(*s2++, locale);
    }while(--count && c1 && c1==c2);

    return c1-c2;
}

/*********************************************************************
 *                  _stricmp_l   (MSVCRT.@)
 */
int __cdecl MSVCRT__stricmp_l(const char *s1, const char *s2, MSVCRT__locale_t locale)
{
    return MSVCRT__strnicmp_l(s1, s2, -1, locale);
}

/*********************************************************************
 *                  _strnicmp   (MSVCRT.@)
 */
int __cdecl MSVCRT__strnicmp(const char *s1, const char *s2, MSVCRT_size_t count)
{
    return MSVCRT__strnicmp_l(s1, s2, count, NULL);
}

/*********************************************************************
 *                  _stricmp   (MSVCRT.@)
 */
int __cdecl MSVCRT__stricmp(const char *s1, const char *s2)
{
    return MSVCRT__strnicmp_l(s1, s2, -1, NULL);
}

/*********************************************************************
 *                  strstr   (MSVCRT.@)
 */
char* __cdecl MSVCRT_strstr(const char *haystack, const char *needle)
{
    return strstr(haystack, needle);
}
