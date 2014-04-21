/*
 * msvcrt.dll wide-char functions
 *
 * Copyright 1999 Alexandre Julliard
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

#include <limits.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "msvcrt.h"
#include "winnls.h"
#include "wtypes.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

static BOOL n_format_enabled = TRUE;

#include "printf.h"
#define PRINTF_WIDE
#include "printf.h"
#undef PRINTF_WIDE

/*********************************************************************
 *		_get_printf_count_output (MSVCR80.@)
 */
int CDECL MSVCRT__get_printf_count_output( void )
{
    return n_format_enabled ? 1 : 0;
}

/*********************************************************************
 *		_set_printf_count_output (MSVCR80.@)
 */
int CDECL MSVCRT__set_printf_count_output( int enable )
{
    BOOL old = n_format_enabled;
    n_format_enabled = enable != 0;
    return old ? 1 : 0;
}

/*********************************************************************
 *		_wcsdup (MSVCRT.@)
 */
MSVCRT_wchar_t* CDECL MSVCRT__wcsdup( const MSVCRT_wchar_t* str )
{
  MSVCRT_wchar_t* ret = NULL;
  if (str)
  {
    int size = (strlenW(str) + 1) * sizeof(MSVCRT_wchar_t);
    ret = MSVCRT_malloc( size );
    if (ret) memcpy( ret, str, size );
  }
  return ret;
}

INT CDECL MSVCRT__wcsicmp_l(const MSVCRT_wchar_t *str1, const MSVCRT_wchar_t *str2, MSVCRT__locale_t locale)
{
    if(!MSVCRT_CHECK_PMT(str1 != NULL) || !MSVCRT_CHECK_PMT(str2 != NULL))
        return MSVCRT__NLSCMPERROR;

    return strcmpiW(str1, str2);
}

/*********************************************************************
 *		_wcsicmp (MSVCRT.@)
 */
INT CDECL MSVCRT__wcsicmp( const MSVCRT_wchar_t* str1, const MSVCRT_wchar_t* str2 )
{
    return strcmpiW( str1, str2 );
}

/*********************************************************************
 *              _wcsnicmp (MSVCRT.@)
 */
INT CDECL MSVCRT__wcsnicmp(const MSVCRT_wchar_t *str1, const MSVCRT_wchar_t *str2, INT n)
{
    return strncmpiW(str1, str2, n);
}

/*********************************************************************
 *              _wcsicoll_l (MSVCRT.@)
 */
int CDECL MSVCRT__wcsicoll_l(const MSVCRT_wchar_t* str1, const MSVCRT_wchar_t* str2, MSVCRT__locale_t locale)
{
    MSVCRT_pthreadlocinfo locinfo;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!locinfo->lc_handle[MSVCRT_LC_COLLATE])
        return strcmpiW(str1, str2);
    return CompareStringW(locinfo->lc_handle[MSVCRT_LC_COLLATE], NORM_IGNORECASE,
			  str1, -1, str2, -1)-CSTR_EQUAL;
}

/*********************************************************************
 *		_wcsicoll (MSVCRT.@)
 */
INT CDECL MSVCRT__wcsicoll( const MSVCRT_wchar_t* str1, const MSVCRT_wchar_t* str2 )
{
    return MSVCRT__wcsicoll_l(str1, str2, NULL);
}

/*********************************************************************
 *              _wcsnicoll_l (MSVCRT.@)
 */
int CDECL MSVCRT__wcsnicoll_l(const MSVCRT_wchar_t* str1, const MSVCRT_wchar_t* str2,
			      MSVCRT_size_t count, MSVCRT__locale_t locale)
{
    MSVCRT_pthreadlocinfo locinfo;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!locinfo->lc_handle[MSVCRT_LC_COLLATE])
        return strncmpiW(str1, str2, count);
    return CompareStringW(locinfo->lc_handle[MSVCRT_LC_COLLATE], NORM_IGNORECASE,
			  str1, count, str2, count)-CSTR_EQUAL;
}

/*********************************************************************
 *		_wcsnicoll (MSVCRT.@)
 */
INT CDECL MSVCRT__wcsnicoll( const MSVCRT_wchar_t* str1, const MSVCRT_wchar_t* str2, MSVCRT_size_t count )
{
    return MSVCRT__wcsnicoll_l(str1, str2, count, NULL);
}

/*********************************************************************
 *		_wcsnset (MSVCRT.@)
 */
MSVCRT_wchar_t* CDECL MSVCRT__wcsnset( MSVCRT_wchar_t* str, MSVCRT_wchar_t c, MSVCRT_size_t n )
{
  MSVCRT_wchar_t* ret = str;
  while ((n-- > 0) && *str) *str++ = c;
  return ret;
}

/*********************************************************************
 *		_wcsrev (MSVCRT.@)
 */
MSVCRT_wchar_t* CDECL MSVCRT__wcsrev( MSVCRT_wchar_t* str )
{
  MSVCRT_wchar_t* ret = str;
  MSVCRT_wchar_t* end = str + strlenW(str) - 1;
  while (end > str)
  {
    MSVCRT_wchar_t t = *end;
    *end--  = *str;
    *str++  = t;
  }
  return ret;
}

/*********************************************************************
 *		_wcsset (MSVCRT.@)
 */
MSVCRT_wchar_t* CDECL MSVCRT__wcsset( MSVCRT_wchar_t* str, MSVCRT_wchar_t c )
{
  MSVCRT_wchar_t* ret = str;
  while (*str) *str++ = c;
  return ret;
}

/******************************************************************
 *		_wcsupr_s_l (MSVCRT.@)
 */
int CDECL MSVCRT__wcsupr_s_l( MSVCRT_wchar_t* str, MSVCRT_size_t n,
   MSVCRT__locale_t locale )
{
  MSVCRT_wchar_t* ptr = str;

  if (!str || !n)
  {
    if (str) *str = '\0';
    *MSVCRT__errno() = MSVCRT_EINVAL;
    return MSVCRT_EINVAL;
  }

  while (n--)
  {
    if (!*ptr) return 0;
    /* FIXME: add locale support */
    *ptr = toupperW(*ptr);
    ptr++;
  }

  /* MSDN claims that the function should return and set errno to
   * ERANGE, which doesn't seem to be true based on the tests. */
  *str = '\0';
  *MSVCRT__errno() = MSVCRT_EINVAL;
  return MSVCRT_EINVAL;
}

/******************************************************************
 *		_wcsupr_s (MSVCRT.@)
 *
 */
INT CDECL MSVCRT__wcsupr_s( MSVCRT_wchar_t* str, MSVCRT_size_t n )
{
  return MSVCRT__wcsupr_s_l( str, n, NULL );
}

/******************************************************************
 *		_wcslwr_s (MSVCRT.@)
 */
int CDECL MSVCRT__wcslwr_s( MSVCRT_wchar_t* str, MSVCRT_size_t n )
{
  MSVCRT_wchar_t* ptr = str;

  if (!str || !n)
  {
    if (str) *str = '\0';
    *MSVCRT__errno() = MSVCRT_EINVAL;
    return MSVCRT_EINVAL;
  }

  while (n--)
  {
    if (!*ptr) return 0;
    *ptr = tolowerW(*ptr);
    ptr++;
  }

  /* MSDN claims that the function should return and set errno to
   * ERANGE, which doesn't seem to be true based on the tests. */
  *str = '\0';
  *MSVCRT__errno() = MSVCRT_EINVAL;
  return MSVCRT_EINVAL;
}

/*********************************************************************
 *              _wcsncoll_l (MSVCRT.@)
 */
int CDECL MSVCRT__wcsncoll_l(const MSVCRT_wchar_t* str1, const MSVCRT_wchar_t* str2,
			      MSVCRT_size_t count, MSVCRT__locale_t locale)
{
    MSVCRT_pthreadlocinfo locinfo;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!locinfo->lc_handle[MSVCRT_LC_COLLATE])
        return strncmpW(str1, str2, count);
    return CompareStringW(locinfo->lc_handle[MSVCRT_LC_COLLATE], 0, str1, count, str2, count)-CSTR_EQUAL;
}

/*********************************************************************
 *              _wcsncoll (MSVCRT.@)
 */
int CDECL MSVCRT__wcsncoll(const MSVCRT_wchar_t* str1, const MSVCRT_wchar_t* str2, MSVCRT_size_t count)
{
    return MSVCRT__wcsncoll_l(str1, str2, count, NULL);
}

/*********************************************************************
 *		_wcstod_l (MSVCRT.@)
 */
double CDECL MSVCRT__wcstod_l(const MSVCRT_wchar_t* str, MSVCRT_wchar_t** end,
        MSVCRT__locale_t locale)
{
    MSVCRT_pthreadlocinfo locinfo;
    unsigned __int64 d=0, hlp;
    unsigned fpcontrol;
    int exp=0, sign=1;
    const MSVCRT_wchar_t *p;
    double ret;
    long double lret=1, expcnt = 10;
    BOOL found_digit = FALSE, negexp;

    if (!MSVCRT_CHECK_PMT(str != NULL)) return 0;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    p = str;
    while(isspaceW(*p))
        p++;

    if(*p == '-') {
        sign = -1;
        p++;
    } else  if(*p == '+')
        p++;

    while(isdigitW(*p)) {
        found_digit = TRUE;
        hlp = d*10+*(p++)-'0';
        if(d>MSVCRT_UI64_MAX/10 || hlp<d) {
            exp++;
            break;
        } else
            d = hlp;
    }
    while(isdigitW(*p)) {
        exp++;
        p++;
    }
    if(*p == *locinfo->lconv->decimal_point)
        p++;

    while(isdigitW(*p)) {
        found_digit = TRUE;
        hlp = d*10+*(p++)-'0';
        if(d>MSVCRT_UI64_MAX/10 || hlp<d)
            break;

        d = hlp;
        exp--;
    }
    while(isdigitW(*p))
        p++;

    if(!found_digit) {
        if(end)
            *end = (MSVCRT_wchar_t*)str;
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

        if(isdigitW(*p)) {
            while(isdigitW(*p)) {
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

    if((d && ret==0.0) || isinf(ret))
        *MSVCRT__errno() = MSVCRT_ERANGE;

    if(end)
        *end = (MSVCRT_wchar_t*)p;

    return ret;
}

/*********************************************************************
 * wcsrtombs_l (INTERNAL)
 */
static MSVCRT_size_t MSVCRT_wcsrtombs_l(char *mbstr, const MSVCRT_wchar_t **wcstr,
        MSVCRT_size_t count, MSVCRT__locale_t locale)
{
    MSVCRT_pthreadlocinfo locinfo;
    MSVCRT_size_t tmp = 0;
    BOOL used_default;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!mbstr) {
        tmp = WideCharToMultiByte(locinfo->lc_codepage, WC_NO_BEST_FIT_CHARS,
                *wcstr, -1, NULL, 0, NULL, &used_default)-1;
        if(!tmp || used_default) {
            *MSVCRT__errno() = MSVCRT_EILSEQ;
            return -1;
        }
        return tmp;
    }

    while(**wcstr) {
        char buf[3];
        MSVCRT_size_t i, size;

        size = WideCharToMultiByte(locinfo->lc_codepage, WC_NO_BEST_FIT_CHARS,
                *wcstr, 1, buf, 3, NULL, &used_default);
        if(!size || used_default) {
            *MSVCRT__errno() = MSVCRT_EILSEQ;
            return -1;
        }
        if(tmp+size > count)
            return tmp;

        for(i=0; i<size; i++)
            mbstr[tmp++] = buf[i];
        (*wcstr)++;
    }

    if(tmp < count) {
        mbstr[tmp] = '\0';
        *wcstr = NULL;
    }
    return tmp;
}

/*********************************************************************
 *		_wcstombs_l (MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT__wcstombs_l(char *mbstr, const MSVCRT_wchar_t *wcstr,
        MSVCRT_size_t count, MSVCRT__locale_t locale)
{
    return MSVCRT_wcsrtombs_l(mbstr, &wcstr, count, locale);
}

/*********************************************************************
 *		wcstombs (MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT_wcstombs(char *mbstr, const MSVCRT_wchar_t *wcstr,
        MSVCRT_size_t count)
{
    return MSVCRT_wcsrtombs_l(mbstr, &wcstr, count, NULL);
}

/*********************************************************************
 *		wcsrtombs (MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT_wcsrtombs(char *mbstr, const MSVCRT_wchar_t **wcstr,
        MSVCRT_size_t count, MSVCRT_mbstate_t *mbstate)
{
    if(mbstate)
        *mbstate = 0;

    return MSVCRT_wcsrtombs_l(mbstr, wcstr, count, NULL);
}

/*********************************************************************
 * MSVCRT_wcsrtombs_s_l (INTERNAL)
 */
static int MSVCRT_wcsrtombs_s_l(MSVCRT_size_t *ret, char *mbstr,
        MSVCRT_size_t size, const MSVCRT_wchar_t **wcstr,
        MSVCRT_size_t count, MSVCRT__locale_t locale)
{
    MSVCRT_size_t conv;

    if(!mbstr && !size && wcstr) {
        conv = MSVCRT_wcsrtombs_l(NULL, wcstr, 0, locale);
        if(conv == -1)
            return *MSVCRT__errno();
        if(ret)
            *ret = conv+1;
        return 0;
    }

    if (!MSVCRT_CHECK_PMT(mbstr != NULL)) return MSVCRT_EINVAL;
    if (size) mbstr[0] = '\0';
    if (!MSVCRT_CHECK_PMT(wcstr != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(*wcstr != NULL)) return MSVCRT_EINVAL;

    if(count==MSVCRT__TRUNCATE || size<count)
        conv = size;
    else
        conv = count;

    conv = MSVCRT_wcsrtombs_l(mbstr, wcstr, conv, locale);
    if(conv == -1) {
        if(size)
            mbstr[0] = '\0';
        return *MSVCRT__errno();
    }else if(conv < size)
        mbstr[conv++] = '\0';
    else if(conv==size && (count==MSVCRT__TRUNCATE || mbstr[conv-1]=='\0'))
        mbstr[conv-1] = '\0';
    else {
        MSVCRT_INVALID_PMT("mbstr[size] is too small", MSVCRT_ERANGE);
        if(size)
            mbstr[0] = '\0';
        return MSVCRT_ERANGE;
    }

    if(ret)
        *ret = conv;
    return 0;
}

/*********************************************************************
 *		_wcstombs_s_l (MSVCRT.@)
 */
int CDECL MSVCRT__wcstombs_s_l(MSVCRT_size_t *ret, char *mbstr,
        MSVCRT_size_t size, const MSVCRT_wchar_t *wcstr,
        MSVCRT_size_t count, MSVCRT__locale_t locale)
{
    return MSVCRT_wcsrtombs_s_l(ret, mbstr, size, &wcstr,count, locale);
}

/*********************************************************************
 *		wcstombs_s (MSVCRT.@)
 */
int CDECL MSVCRT_wcstombs_s(MSVCRT_size_t *ret, char *mbstr,
        MSVCRT_size_t size, const MSVCRT_wchar_t *wcstr, MSVCRT_size_t count)
{
    return MSVCRT_wcsrtombs_s_l(ret, mbstr, size, &wcstr, count, NULL);
}

/*********************************************************************
 *		wcsrtombs_s (MSVCRT.@)
 */
int CDECL MSVCRT_wcsrtombs_s(MSVCRT_size_t *ret, char *mbstr, MSVCRT_size_t size,
        const MSVCRT_wchar_t **wcstr, MSVCRT_size_t count, MSVCRT_mbstate_t *mbstate)
{
    if(mbstate)
        *mbstate = 0;

    return MSVCRT_wcsrtombs_s_l(ret, mbstr, size, wcstr, count, NULL);
}

/*********************************************************************
 *		wcstod (MSVCRT.@)
 */
double CDECL MSVCRT_wcstod(const MSVCRT_wchar_t* lpszStr, MSVCRT_wchar_t** end)
{
    return MSVCRT__wcstod_l(lpszStr, end, NULL);
}

/*********************************************************************
 *		_wtof (MSVCRT.@)
 */
double CDECL MSVCRT__wtof(const MSVCRT_wchar_t *str)
{
    return MSVCRT__wcstod_l(str, NULL, NULL);
}

/*********************************************************************
 *		_wtof_l (MSVCRT.@)
 */
double CDECL MSVCRT__wtof_l(const MSVCRT_wchar_t *str, MSVCRT__locale_t locale)
{
    return MSVCRT__wcstod_l(str, NULL, locale);
}

/*********************************************************************
 * arg_clbk_valist (INTERNAL)
 */
printf_arg arg_clbk_valist(void *ctx, int arg_pos, int type, __ms_va_list *valist)
{
    printf_arg ret;

    if(type == VT_I8)
        ret.get_longlong = va_arg(*valist, LONGLONG);
    else if(type == VT_INT)
        ret.get_int = va_arg(*valist, int);
    else if(type == VT_R8)
        ret.get_double = va_arg(*valist, double);
    else if(type == VT_PTR)
        ret.get_ptr = va_arg(*valist, void*);
    else {
        ERR("Incorrect type\n");
        ret.get_int = 0;
    }

    return ret;
}

/*********************************************************************
 * arg_clbk_positional (INTERNAL)
 */
static printf_arg arg_clbk_positional(void *ctx, int pos, int type, __ms_va_list *valist)
{
    printf_arg *args = ctx;
    return args[pos];
}

/*********************************************************************
 *              _vsnprintf (MSVCRT.@)
 */
int CDECL MSVCRT_vsnprintf( char *str, MSVCRT_size_t len,
                            const char *format, __ms_va_list valist )
{
    static const char nullbyte = '\0';
    struct _str_ctx_a ctx = {len, str};
    int ret;

    ret = pf_printf_a(puts_clbk_str_a, &ctx, format, NULL, FALSE, FALSE,
            arg_clbk_valist, NULL, &valist);
    puts_clbk_str_a(&ctx, 1, &nullbyte);
    return ret;
}

/*********************************************************************
 *		_vsnprintf_l (MSVCRT.@)
 */
int CDECL MSVCRT_vsnprintf_l( char *str, MSVCRT_size_t len, const char *format,
                            MSVCRT__locale_t locale, __ms_va_list valist )
{
    static const char nullbyte = '\0';
    struct _str_ctx_a ctx = {len, str};
    int ret;

    ret = pf_printf_a(puts_clbk_str_a, &ctx, format, locale, FALSE, FALSE,
            arg_clbk_valist, NULL, &valist);
    puts_clbk_str_a(&ctx, 1, &nullbyte);
    return ret;
}

/*********************************************************************
 *		_vsprintf_l (MSVCRT.@)
 */
int CDECL MSVCRT_vsprintf_l( char *str, const char *format,
                            MSVCRT__locale_t locale, __ms_va_list valist )
{
    return MSVCRT_vsnprintf_l(str, INT_MAX, format, locale, valist);
}

/*********************************************************************
 *		_sprintf_l (MSVCRT.@)
 */
int CDECL MSVCRT_sprintf_l(char *str, const char *format,
                           MSVCRT__locale_t locale, ...)
{
    int retval;
    __ms_va_list valist;
    __ms_va_start(valist, locale);
    retval = MSVCRT_vsnprintf_l(str, INT_MAX, format, locale, valist);
    __ms_va_end(valist);
    return retval;
}

/*********************************************************************
 *		_vsnprintf_s_l (MSVCRT.@)
 */
int CDECL MSVCRT_vsnprintf_s_l( char *str, MSVCRT_size_t sizeOfBuffer,
        MSVCRT_size_t count, const char *format,
        MSVCRT__locale_t locale, __ms_va_list valist )
{
    static const char nullbyte = '\0';
    struct _str_ctx_a ctx;
    int len, ret;

    if(sizeOfBuffer<count+1 || count==-1)
        len = sizeOfBuffer;
    else
        len = count+1;

    ctx.len = len;
    ctx.buf = str;
    ret = pf_printf_a(puts_clbk_str_a, &ctx, format, locale, FALSE, TRUE,
            arg_clbk_valist, NULL, &valist);
    puts_clbk_str_a(&ctx, 1, &nullbyte);

    if(ret<0 || ret==len) {
        if(count!=MSVCRT__TRUNCATE && count>sizeOfBuffer) {
            MSVCRT_INVALID_PMT("str[sizeOfBuffer] is too small", MSVCRT_ERANGE);
            memset(str, 0, sizeOfBuffer);
        } else
            str[len-1] = '\0';

        return -1;
    }

    return ret;
}

/*********************************************************************
 *		_vsprintf_s_l (MSVCRT.@)
 */
int CDECL MSVCRT_vsprintf_s_l( char *str, MSVCRT_size_t count, const char *format,
                               MSVCRT__locale_t locale, __ms_va_list valist )
{
    return MSVCRT_vsnprintf_s_l(str, INT_MAX, count, format, locale, valist);
}

/*********************************************************************
 *		_sprintf_s_l (MSVCRT.@)
 */
int CDECL MSVCRT_sprintf_s_l( char *str, MSVCRT_size_t count, const char *format,
                              MSVCRT__locale_t locale, ...)
{
    int retval;
    __ms_va_list valist;
    __ms_va_start(valist, locale);
    retval = MSVCRT_vsnprintf_s_l(str, INT_MAX, count, format, locale, valist);
    __ms_va_end(valist);
    return retval;
}

/*********************************************************************
 *              _vsnprintf_s (MSVCRT.@)
 */
int CDECL MSVCRT_vsnprintf_s( char *str, MSVCRT_size_t sizeOfBuffer,
        MSVCRT_size_t count, const char *format, __ms_va_list valist )
{
    return MSVCRT_vsnprintf_s_l(str,sizeOfBuffer, count, format, NULL, valist);
}

/*********************************************************************
 *		vsprintf (MSVCRT.@)
 */
int CDECL MSVCRT_vsprintf( char *str, const char *format, __ms_va_list valist)
{
    return MSVCRT_vsnprintf(str, INT_MAX, format, valist);
}

/*********************************************************************
 *		vsprintf_s (MSVCRT.@)
 */
int CDECL MSVCRT_vsprintf_s( char *str, MSVCRT_size_t num, const char *format, __ms_va_list valist)
{
    return MSVCRT_vsnprintf(str, num, format, valist);
}

/*********************************************************************
 *		_vscprintf (MSVCRT.@)
 */
int CDECL MSVCRT__vscprintf( const char *format, __ms_va_list valist )
{
    return MSVCRT_vsnprintf( NULL, INT_MAX, format, valist );
}

/*********************************************************************
 *		_snprintf (MSVCRT.@)
 */
int CDECL MSVCRT__snprintf(char *str, unsigned int len, const char *format, ...)
{
    int retval;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    retval = MSVCRT_vsnprintf(str, len, format, valist);
    __ms_va_end(valist);
    return retval;
}

/*********************************************************************
 *		_snprintf_s (MSVCRT.@)
 */
int CDECL MSVCRT__snprintf_s(char *str, unsigned int len, unsigned int count,
    const char *format, ...)
{
    int retval;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    retval = MSVCRT_vsnprintf_s_l(str, len, count, format, NULL, valist);
    __ms_va_end(valist);
    return retval;
}

/*********************************************************************
 *              _scprintf (MSVCRT.@)
 */
int CDECL MSVCRT__scprintf(const char *format, ...)
{
    int retval;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    retval = MSVCRT__vscprintf(format, valist);
    __ms_va_end(valist);
    return retval;
}

/*********************************************************************
 *              _vsnwprintf (MSVCRT.@)
 */
int CDECL MSVCRT_vsnwprintf(MSVCRT_wchar_t *str, MSVCRT_size_t len,
        const MSVCRT_wchar_t *format, __ms_va_list valist)
{
    static const MSVCRT_wchar_t nullbyte = '\0';
    struct _str_ctx_w ctx = {len, str};
    int ret;

    ret = pf_printf_w(puts_clbk_str_w, &ctx, format, NULL, FALSE, FALSE,
            arg_clbk_valist, NULL, &valist);
    puts_clbk_str_w(&ctx, 1, &nullbyte);
    return ret;
}

/*********************************************************************
 *              _vsnwprintf_l (MSVCRT.@)
 */
int CDECL MSVCRT_vsnwprintf_l(MSVCRT_wchar_t *str, MSVCRT_size_t len,
        const MSVCRT_wchar_t *format, MSVCRT__locale_t locale,
        __ms_va_list valist)
{
    static const MSVCRT_wchar_t nullbyte = '\0';
    struct _str_ctx_w ctx = {len, str};
    int ret;

    ret = pf_printf_w(puts_clbk_str_w, &ctx, format, locale, FALSE, FALSE,
            arg_clbk_valist, NULL, &valist);
    puts_clbk_str_w(&ctx, 1, &nullbyte);
    return ret;
}

/*********************************************************************
 *		_vswprintf_p_l (MSVCRT.@)
 */
int CDECL MSVCRT_vswprintf_p_l(MSVCRT_wchar_t *buffer, MSVCRT_size_t length,
        const MSVCRT_wchar_t *format, MSVCRT__locale_t locale, __ms_va_list args)
{
    static const MSVCRT_wchar_t nullbyte = '\0';
    printf_arg args_ctx[MSVCRT__ARGMAX+1];
    struct _str_ctx_w puts_ctx = {length, buffer};
    int ret;

    memset(args_ctx, 0, sizeof(args_ctx));

    ret = create_positional_ctx_w(args_ctx, format, args);
    if(ret < 0)  {
        MSVCRT__invalid_parameter(NULL, NULL, NULL, 0, 0);
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return ret;
    } else if(ret == 0)
        ret = pf_printf_w(puts_clbk_str_w, &puts_ctx, format, locale, FALSE, TRUE,
                arg_clbk_valist, NULL, &args);
    else
        ret = pf_printf_w(puts_clbk_str_w, &puts_ctx, format, locale, TRUE, TRUE,
                arg_clbk_positional, args_ctx, NULL);

    puts_clbk_str_w(&puts_ctx, 1, &nullbyte);
    return ret;
}

/*********************************************************************
 * _vswprintf_p (MSVCR100.@)
 */
int CDECL MSVCRT__vswprintf_p(MSVCRT_wchar_t *buffer, MSVCRT_size_t length,
        const MSVCRT_wchar_t *format, __ms_va_list args)
{
    return MSVCRT_vswprintf_p_l(buffer, length, format, NULL, args);
}

/*********************************************************************
 *              _vsnwprintf_s_l (MSVCRT.@)
 */
int CDECL MSVCRT_vsnwprintf_s_l( MSVCRT_wchar_t *str, MSVCRT_size_t sizeOfBuffer,
        MSVCRT_size_t count, const MSVCRT_wchar_t *format,
        MSVCRT__locale_t locale, __ms_va_list valist)
{
    static const MSVCRT_wchar_t nullbyte = '\0';
    struct _str_ctx_w ctx;
    int len, ret;

    len = sizeOfBuffer;
    if(count!=-1 && len>count+1)
        len = count+1;

    ctx.len = len;
    ctx.buf = str;
    ret = pf_printf_w(puts_clbk_str_w, &ctx, format, locale, FALSE, TRUE,
            arg_clbk_valist, NULL, &valist);
    puts_clbk_str_w(&ctx, 1, &nullbyte);

    if(ret<0 || ret==len) {
        if(count!=MSVCRT__TRUNCATE && count>sizeOfBuffer) {
            MSVCRT_INVALID_PMT("str[sizeOfBuffer] is too small", MSVCRT_ERANGE);
            memset(str, 0, sizeOfBuffer*sizeof(MSVCRT_wchar_t));
        } else
            str[len-1] = '\0';

        return -1;
    }

    return ret;
}

/*********************************************************************
 *              _vsnwprintf_s (MSVCRT.@)
 */
int CDECL MSVCRT_vsnwprintf_s(MSVCRT_wchar_t *str, MSVCRT_size_t sizeOfBuffer,
        MSVCRT_size_t count, const MSVCRT_wchar_t *format, __ms_va_list valist)
{
    return MSVCRT_vsnwprintf_s_l(str, sizeOfBuffer, count,
            format, NULL, valist);
}

/*********************************************************************
 *		_snwprintf (MSVCRT.@)
 */
int CDECL MSVCRT__snwprintf( MSVCRT_wchar_t *str, unsigned int len, const MSVCRT_wchar_t *format, ...)
{
    int retval;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    retval = MSVCRT_vsnwprintf(str, len, format, valist);
    __ms_va_end(valist);
    return retval;
}

/*********************************************************************
 *		_snwprintf_l (MSVCRT.@)
 */
int CDECL MSVCRT__snwprintf_l( MSVCRT_wchar_t *str, unsigned int len, const MSVCRT_wchar_t *format,
        MSVCRT__locale_t locale, ...)
{
    int retval;
    __ms_va_list valist;
    __ms_va_start(valist, locale);
    retval = MSVCRT_vsnwprintf_l(str, len, format, locale, valist);
    __ms_va_end(valist);
    return retval;
}

/*********************************************************************
 *		_snwprintf_s (MSVCRT.@)
 */
int CDECL MSVCRT__snwprintf_s( MSVCRT_wchar_t *str, unsigned int len, unsigned int count,
    const MSVCRT_wchar_t *format, ...)
{
    int retval;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    retval = MSVCRT_vsnwprintf_s_l(str, len, count, format, NULL, valist);
    __ms_va_end(valist);
    return retval;
}

/*********************************************************************
 *              _snwprintf_s_l (MSVCRT.@)
 */
int CDECL MSVCRT__snwprintf_s_l( MSVCRT_wchar_t *str, unsigned int len, unsigned int count,
        const MSVCRT_wchar_t *format, MSVCRT__locale_t locale, ... )
{
    int retval;
    __ms_va_list valist;
    __ms_va_start(valist, locale);
    retval = MSVCRT_vsnwprintf_s_l(str, len, count, format, locale, valist);
    __ms_va_end(valist);
    return retval;
}

/*********************************************************************
 *		sprintf (MSVCRT.@)
 */
int CDECL MSVCRT_sprintf( char *str, const char *format, ... )
{
    __ms_va_list ap;
    int r;

    __ms_va_start( ap, format );
    r = MSVCRT_vsnprintf( str, INT_MAX, format, ap );
    __ms_va_end( ap );
    return r;
}

/*********************************************************************
 *		sprintf_s (MSVCRT.@)
 */
int CDECL MSVCRT_sprintf_s( char *str, MSVCRT_size_t num, const char *format, ... )
{
    __ms_va_list ap;
    int r;

    __ms_va_start( ap, format );
    r = MSVCRT_vsnprintf( str, num, format, ap );
    __ms_va_end( ap );
    return r;
}

/*********************************************************************
 *		_scwprintf (MSVCRT.@)
 */
int CDECL MSVCRT__scwprintf( const MSVCRT_wchar_t *format, ... )
{
    __ms_va_list ap;
    int r;

    __ms_va_start( ap, format );
    r = MSVCRT_vsnwprintf( NULL, INT_MAX, format, ap );
    __ms_va_end( ap );
    return r;
}

/*********************************************************************
 *		swprintf (MSVCRT.@)
 */
int CDECL MSVCRT_swprintf( MSVCRT_wchar_t *str, const MSVCRT_wchar_t *format, ... )
{
    __ms_va_list ap;
    int r;

    __ms_va_start( ap, format );
    r = MSVCRT_vsnwprintf( str, INT_MAX, format, ap );
    __ms_va_end( ap );
    return r;
}

/*********************************************************************
 *		swprintf_s (MSVCRT.@)
 */
int CDECL MSVCRT_swprintf_s(MSVCRT_wchar_t *str, MSVCRT_size_t numberOfElements,
        const MSVCRT_wchar_t *format, ... )
{
    __ms_va_list ap;
    int r;

    __ms_va_start(ap, format);
    r = MSVCRT_vsnwprintf_s(str, numberOfElements, INT_MAX, format, ap);
    __ms_va_end(ap);

    return r;
}

/*********************************************************************
 *              _swprintf_s_l (MSVCRT.@)
 */
int CDECL MSVCRT__swprintf_s_l(MSVCRT_wchar_t *str, MSVCRT_size_t numberOfElements,
        const MSVCRT_wchar_t *format, MSVCRT__locale_t locale, ... )
{
    __ms_va_list ap;
    int r;

    __ms_va_start(ap, locale);
    r = MSVCRT_vsnwprintf_s_l(str, numberOfElements, INT_MAX, format, locale, ap);
    __ms_va_end(ap);

    return r;
}

/*********************************************************************
 *		_vswprintf (MSVCRT.@)
 */
int CDECL MSVCRT_vswprintf( MSVCRT_wchar_t* str, const MSVCRT_wchar_t* format, __ms_va_list args )
{
    return MSVCRT_vsnwprintf( str, INT_MAX, format, args );
}

/*********************************************************************
 *		_vswprintf (MSVCRT.@)
 */
int CDECL MSVCRT_vswprintf_l( MSVCRT_wchar_t* str, const MSVCRT_wchar_t* format,
        MSVCRT__locale_t locale, __ms_va_list args )
{
    return MSVCRT_vsnwprintf_l( str, INT_MAX, format, locale, args );
}

/*********************************************************************
 *		_vscwprintf (MSVCRT.@)
 */
int CDECL MSVCRT__vscwprintf( const MSVCRT_wchar_t *format, __ms_va_list args )
{
    return MSVCRT_vsnwprintf( NULL, INT_MAX, format, args );
}

/*********************************************************************
 *		_vscwprintf_l (MSVCRT.@)
 */
int CDECL MSVCRT__vscwprintf_l( const MSVCRT_wchar_t *format, MSVCRT__locale_t locale, __ms_va_list args )
{
    return MSVCRT_vsnwprintf_l( NULL, INT_MAX, format, locale, args );
}

/*********************************************************************
 *		_vscwprintf_p_l (MSVCRT.@)
 */
int CDECL MSVCRT__vscwprintf_p_l( const MSVCRT_wchar_t *format, MSVCRT__locale_t locale, __ms_va_list args )
{
    return MSVCRT_vswprintf_p_l( NULL, INT_MAX, format, locale, args );
}

/*********************************************************************
 * _vscwprintf_p (MSVCR100.@)
 */
int CDECL MSVCRT__vscwprintf_p(const MSVCRT_wchar_t *format, __ms_va_list args)
{
    return MSVCRT_vswprintf_p_l(NULL, INT_MAX, format, NULL, args);
}

/*********************************************************************
 *		vswprintf_s (MSVCRT.@)
 */
int CDECL MSVCRT_vswprintf_s(MSVCRT_wchar_t* str, MSVCRT_size_t numberOfElements,
        const MSVCRT_wchar_t* format, __ms_va_list args)
{
    return MSVCRT_vsnwprintf_s(str, numberOfElements, INT_MAX, format, args );
}

/*********************************************************************
 *              _vswprintf_s_l (MSVCRT.@)
 */
int CDECL MSVCRT_vswprintf_s_l(MSVCRT_wchar_t* str, MSVCRT_size_t numberOfElements,
        const MSVCRT_wchar_t* format, MSVCRT__locale_t locale, __ms_va_list args)
{
    return MSVCRT_vsnwprintf_s_l(str, numberOfElements, INT_MAX,
            format, locale, args );
}

/*********************************************************************
 *		_vsprintf_p_l (MSVCRT.@)
 */
int CDECL MSVCRT_vsprintf_p_l(char *buffer, MSVCRT_size_t length, const char *format,
        MSVCRT__locale_t locale, __ms_va_list args)
{
    static const char nullbyte = '\0';
    printf_arg args_ctx[MSVCRT__ARGMAX+1];
    struct _str_ctx_a puts_ctx = {length, buffer};
    int ret;

    memset(args_ctx, 0, sizeof(args_ctx));

    ret = create_positional_ctx_a(args_ctx, format, args);
    if(ret < 0) {
        MSVCRT__invalid_parameter(NULL, NULL, NULL, 0, 0);
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return ret;
    } else if(ret == 0)
        ret = pf_printf_a(puts_clbk_str_a, &puts_ctx, format, locale, FALSE, TRUE,
                arg_clbk_valist, NULL, &args);
    else
        ret = pf_printf_a(puts_clbk_str_a, &puts_ctx, format, locale, TRUE, TRUE,
                arg_clbk_positional, args_ctx, NULL);

    puts_clbk_str_a(&puts_ctx, 1, &nullbyte);
    return ret;
}

/*********************************************************************
 *		_vsprintf_p (MSVCRT.@)
 */
int CDECL MSVCRT_vsprintf_p(char *buffer, MSVCRT_size_t length,
        const char *format, __ms_va_list args)
{
    return MSVCRT_vsprintf_p_l(buffer, length, format, NULL, args);
}

/*********************************************************************
 *		_sprintf_p_l (MSVCRT.@)
 */
int CDECL MSVCRT_sprintf_p_l(char *buffer, MSVCRT_size_t length,
        const char *format, MSVCRT__locale_t locale, ...)
{
    __ms_va_list valist;
    int r;

    __ms_va_start(valist, locale);
    r = MSVCRT_vsprintf_p_l(buffer, length, format, locale, valist);
    __ms_va_end(valist);

    return r;
}

/*********************************************************************
 * _sprintf_p (MSVCR100.@)
 */
int CDECL MSVCRT__sprintf_p(char *buffer, MSVCRT_size_t length, const char *format, ...)
{
    __ms_va_list valist;
    int r;

    __ms_va_start(valist, format);
    r = MSVCRT_vsprintf_p_l(buffer, length, format, NULL, valist);
    __ms_va_end(valist);

    return r;
}

/*********************************************************************
 *		_swprintf_p_l (MSVCRT.@)
 */
int CDECL MSVCRT_swprintf_p_l(MSVCRT_wchar_t *buffer, MSVCRT_size_t length,
        const MSVCRT_wchar_t *format, MSVCRT__locale_t locale, ...)
{
    __ms_va_list valist;
    int r;

    __ms_va_start(valist, locale);
    r = MSVCRT_vswprintf_p_l(buffer, length, format, locale, valist);
    __ms_va_end(valist);

    return r;
}

/*********************************************************************
 *              _wcscoll_l (MSVCRT.@)
 */
int CDECL MSVCRT__wcscoll_l(const MSVCRT_wchar_t* str1, const MSVCRT_wchar_t* str2, MSVCRT__locale_t locale)
{
    MSVCRT_pthreadlocinfo locinfo;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!locinfo->lc_handle[MSVCRT_LC_COLLATE])
        return strcmpW(str1, str2);
    return CompareStringW(locinfo->lc_handle[MSVCRT_LC_COLLATE], 0, str1, -1, str2, -1)-CSTR_EQUAL;
}

/*********************************************************************
 *		wcscoll (MSVCRT.@)
 */
int CDECL MSVCRT_wcscoll( const MSVCRT_wchar_t* str1, const MSVCRT_wchar_t* str2 )
{
    return MSVCRT__wcscoll_l(str1, str2, NULL);
}

/*********************************************************************
 *		wcspbrk (MSVCRT.@)
 */
MSVCRT_wchar_t* CDECL MSVCRT_wcspbrk( const MSVCRT_wchar_t* str, const MSVCRT_wchar_t* accept )
{
    const MSVCRT_wchar_t* p;

    while (*str)
    {
        for (p = accept; *p; p++) if (*p == *str) return (MSVCRT_wchar_t*)str;
        str++;
    }
    return NULL;
}

/*********************************************************************
 *		wcstok_s  (MSVCRT.@)
 */
MSVCRT_wchar_t * CDECL MSVCRT_wcstok_s( MSVCRT_wchar_t *str, const MSVCRT_wchar_t *delim,
                                 MSVCRT_wchar_t **next_token )
{
    MSVCRT_wchar_t *ret;

    if (!MSVCRT_CHECK_PMT(delim != NULL)) return NULL;
    if (!MSVCRT_CHECK_PMT(next_token != NULL)) return NULL;
    if (!MSVCRT_CHECK_PMT(str != NULL || *next_token != NULL)) return NULL;

    if (!str) str = *next_token;

    while (*str && strchrW( delim, *str )) str++;
    if (!*str) return NULL;
    ret = str++;
    while (*str && !strchrW( delim, *str )) str++;
    if (*str) *str++ = 0;
    *next_token = str;
    return ret;
}

/*********************************************************************
 *		wcstok  (MSVCRT.@)
 */
MSVCRT_wchar_t * CDECL MSVCRT_wcstok( MSVCRT_wchar_t *str, const MSVCRT_wchar_t *delim )
{
    return MSVCRT_wcstok_s(str, delim, &msvcrt_get_thread_data()->wcstok_next);
}

/*********************************************************************
 *		_wctomb_s_l (MSVCRT.@)
 */
int CDECL MSVCRT__wctomb_s_l(int *len, char *mbchar, MSVCRT_size_t size,
        MSVCRT_wchar_t wch, MSVCRT__locale_t locale)
{
    MSVCRT_pthreadlocinfo locinfo;
    BOOL error;
    int mblen;

    if(!mbchar && size>0) {
        if(len)
            *len = 0;
        return 0;
    }

    if(len)
        *len = -1;

    if(!MSVCRT_CHECK_PMT(size <= INT_MAX))
        return MSVCRT_EINVAL;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!locinfo->lc_codepage) {
        if(wch > 0xff) {
            if(mbchar && size>0)
                memset(mbchar, 0, size);
            *MSVCRT__errno() = MSVCRT_EILSEQ;
            return MSVCRT_EILSEQ;
        }

        if(!MSVCRT_CHECK_PMT_ERR(size >= 1, MSVCRT_ERANGE))
            return MSVCRT_ERANGE;

        *mbchar = wch;
        if(len)
            *len = 1;
        return 0;
    }

    mblen = WideCharToMultiByte(locinfo->lc_codepage, 0, &wch, 1, mbchar, size, NULL, &error);
    if(!mblen || error) {
        if(!mblen && GetLastError()==ERROR_INSUFFICIENT_BUFFER) {
            if(mbchar && size>0)
                memset(mbchar, 0, size);

            MSVCRT_INVALID_PMT("insufficient buffer size", MSVCRT_ERANGE);
            return MSVCRT_ERANGE;
        }

        *MSVCRT__errno() = MSVCRT_EILSEQ;
        return MSVCRT_EILSEQ;
    }

    if(len)
        *len = mblen;
    return 0;
}

/*********************************************************************
 *              wctomb_s (MSVCRT.@)
 */
int CDECL MSVCRT_wctomb_s(int *len, char *mbchar, MSVCRT_size_t size, MSVCRT_wchar_t wch)
{
    return MSVCRT__wctomb_s_l(len, mbchar, size, wch, NULL);
}

/*********************************************************************
 *              _wctomb_l (MSVCRT.@)
 */
int CDECL MSVCRT__wctomb_l(char *dst, MSVCRT_wchar_t ch, MSVCRT__locale_t locale)
{
    int len;

    MSVCRT__wctomb_s_l(&len, dst, dst ? 6 : 0, ch, locale);
    return len;
}

/*********************************************************************
 *		wctomb (MSVCRT.@)
 */
INT CDECL MSVCRT_wctomb( char *dst, MSVCRT_wchar_t ch )
{
    return MSVCRT__wctomb_l(dst, ch, NULL);
}

/*********************************************************************
 *		wctob (MSVCRT.@)
 */
INT CDECL MSVCRT_wctob( MSVCRT_wint_t wchar )
{
    char out;
    BOOL error;
    UINT codepage = get_locinfo()->lc_codepage;

    if(!codepage) {
        if (wchar < 0xff)
            return (signed char)wchar;
        else
            return MSVCRT_EOF;
    } else if(WideCharToMultiByte( codepage, 0, &wchar, 1, &out, 1, NULL, &error ) && !error)
        return (INT)out;
    return MSVCRT_EOF;
}

/*********************************************************************
 *              wcrtomb (MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT_wcrtomb( char *dst, MSVCRT_wchar_t ch, MSVCRT_mbstate_t *s)
{
    if(s)
        *s = 0;
    return MSVCRT_wctomb(dst, ch);
}

/*********************************************************************
 *		iswalnum (MSVCRT.@)
 */
INT CDECL MSVCRT_iswalnum( MSVCRT_wchar_t wc )
{
    return isalnumW( wc );
}

/*********************************************************************
 *		iswalpha (MSVCRT.@)
 */
INT CDECL MSVCRT_iswalpha( MSVCRT_wchar_t wc )
{
    return isalphaW( wc );
}

/*********************************************************************
 *              iswalpha_l (MSVCRT.@)
 */
INT CDECL MSVCRT__iswalpha_l( MSVCRT_wchar_t wc, MSVCRT__locale_t locale )
{
    return isalphaW( wc );
}

/*********************************************************************
 *		iswcntrl (MSVCRT.@)
 */
INT CDECL MSVCRT_iswcntrl( MSVCRT_wchar_t wc )
{
    return iscntrlW( wc );
}

/*********************************************************************
 *		iswdigit (MSVCRT.@)
 */
INT CDECL MSVCRT_iswdigit( MSVCRT_wchar_t wc )
{
    return isdigitW( wc );
}

/*********************************************************************
 *		_iswdigit_l (MSVCRT.@)
 */
INT CDECL MSVCRT__iswdigit_l( MSVCRT_wchar_t wc, MSVCRT__locale_t locale )
{
    return isdigitW( wc );
}

/*********************************************************************
 *		iswgraph (MSVCRT.@)
 */
INT CDECL MSVCRT_iswgraph( MSVCRT_wchar_t wc )
{
    return isgraphW( wc );
}

/*********************************************************************
 *		iswlower (MSVCRT.@)
 */
INT CDECL MSVCRT_iswlower( MSVCRT_wchar_t wc )
{
    return islowerW( wc );
}

/*********************************************************************
 *		iswprint (MSVCRT.@)
 */
INT CDECL MSVCRT_iswprint( MSVCRT_wchar_t wc )
{
    return isprintW( wc );
}

/*********************************************************************
 *		iswpunct (MSVCRT.@)
 */
INT CDECL MSVCRT_iswpunct( MSVCRT_wchar_t wc )
{
    return ispunctW( wc );
}

/*********************************************************************
 *		iswspace (MSVCRT.@)
 */
INT CDECL MSVCRT_iswspace( MSVCRT_wchar_t wc )
{
    return isspaceW( wc );
}

/*********************************************************************
 *		iswupper (MSVCRT.@)
 */
INT CDECL MSVCRT_iswupper( MSVCRT_wchar_t wc )
{
    return isupperW( wc );
}

/*********************************************************************
 *		iswxdigit (MSVCRT.@)
 */
INT CDECL MSVCRT_iswxdigit( MSVCRT_wchar_t wc )
{
    return isxdigitW( wc );
}

/*********************************************************************
 *		wcscpy_s (MSVCRT.@)
 */
INT CDECL MSVCRT_wcscpy_s( MSVCRT_wchar_t* wcDest, MSVCRT_size_t numElement, const  MSVCRT_wchar_t *wcSrc)
{
    MSVCRT_size_t size = 0;

    if(!MSVCRT_CHECK_PMT(wcDest)) return MSVCRT_EINVAL;
    if(!MSVCRT_CHECK_PMT(numElement)) return MSVCRT_EINVAL;

    wcDest[0] = 0;

    if(!MSVCRT_CHECK_PMT(wcSrc)) return MSVCRT_EINVAL;

    size = strlenW(wcSrc) + 1;

    if(!MSVCRT_CHECK_PMT_ERR(size <= numElement, MSVCRT_ERANGE))
        return MSVCRT_ERANGE;

    memcpy( wcDest, wcSrc, size*sizeof(WCHAR) );

    return 0;
}

/******************************************************************
 *		wcsncpy (MSVCRT.@)
 */
MSVCRT_wchar_t* __cdecl MSVCRT_wcsncpy( MSVCRT_wchar_t* s1,
        const MSVCRT_wchar_t *s2, MSVCRT_size_t n )
{
    MSVCRT_size_t i;

    for(i=0; i<n; i++)
        if(!(s1[i] = s2[i])) break;
    for(; i<n; i++)
        s1[i] = 0;
    return s1;
}

/******************************************************************
 *		wcsncpy_s (MSVCRT.@)
 */
INT CDECL MSVCRT_wcsncpy_s( MSVCRT_wchar_t* wcDest, MSVCRT_size_t numElement, const MSVCRT_wchar_t *wcSrc,
                            MSVCRT_size_t count )
{
    WCHAR *p = wcDest;
    BOOL truncate = (count == MSVCRT__TRUNCATE);

    if(!wcDest && !numElement && !count)
        return 0;

    if (!wcDest || !numElement)
        return MSVCRT_EINVAL;

    if (!wcSrc)
    {
        *wcDest = 0;
        return count ? MSVCRT_EINVAL : 0;
    }

    while (numElement && count && *wcSrc)
    {
        *p++ = *wcSrc++;
        numElement--;
        count--;
    }
    if (!numElement && truncate)
    {
        *(p-1) = 0;
        return MSVCRT_STRUNCATE;
    }
    else if (!numElement)
    {
        *wcDest = 0;
        return MSVCRT_ERANGE;
    }

    *p = 0;
    return 0;
}

/******************************************************************
 *		wcscat_s (MSVCRT.@)
 *
 */
INT CDECL MSVCRT_wcscat_s(MSVCRT_wchar_t* dst, MSVCRT_size_t elem, const MSVCRT_wchar_t* src)
{
    MSVCRT_wchar_t* ptr = dst;

    if (!dst || elem == 0) return MSVCRT_EINVAL;
    if (!src)
    {
        dst[0] = '\0';
        return MSVCRT_EINVAL;
    }

    /* seek to end of dst string (or elem if no end of string is found */
    while (ptr < dst + elem && *ptr != '\0') ptr++;
    while (ptr < dst + elem)
    {
        if ((*ptr++ = *src++) == '\0') return 0;
    }
    /* not enough space */
    dst[0] = '\0';
    return MSVCRT_ERANGE;
}

/*********************************************************************
 *  wcsncat_s (MSVCRT.@)
 *
 */
INT CDECL MSVCRT_wcsncat_s(MSVCRT_wchar_t *dst, MSVCRT_size_t elem,
        const MSVCRT_wchar_t *src, MSVCRT_size_t count)
{
    MSVCRT_size_t srclen;
    MSVCRT_wchar_t dststart;
    INT ret = 0;

    if (!MSVCRT_CHECK_PMT(dst != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(elem > 0)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(src != NULL || count == 0)) return MSVCRT_EINVAL;

    if (count == 0)
        return 0;

    for (dststart = 0; dststart < elem; dststart++)
    {
        if (dst[dststart] == '\0')
            break;
    }
    if (dststart == elem)
    {
        MSVCRT_INVALID_PMT("dst[elem] is not NULL terminated\n", MSVCRT_EINVAL);
        return MSVCRT_EINVAL;
    }

    if (count == MSVCRT__TRUNCATE)
    {
        srclen = strlenW(src);
        if (srclen >= (elem - dststart))
        {
            srclen = elem - dststart - 1;
            ret = MSVCRT_STRUNCATE;
        }
    }
    else
        srclen = min(strlenW(src), count);
    if (srclen < (elem - dststart))
    {
        memcpy(&dst[dststart], src, srclen*sizeof(MSVCRT_wchar_t));
        dst[dststart+srclen] = '\0';
        return ret;
    }
    MSVCRT_INVALID_PMT("dst[elem] is too small", MSVCRT_ERANGE);
    dst[0] = '\0';
    return MSVCRT_ERANGE;
}

/*********************************************************************
 *  _wcstoi64_l (MSVCRT.@)
 *
 * FIXME: locale parameter is ignored
 */
__int64 CDECL MSVCRT__wcstoi64_l(const MSVCRT_wchar_t *nptr,
        MSVCRT_wchar_t **endptr, int base, MSVCRT__locale_t locale)
{
    BOOL negative = FALSE;
    __int64 ret = 0;

    TRACE("(%s %p %d %p)\n", debugstr_w(nptr), endptr, base, locale);

    if (!MSVCRT_CHECK_PMT(nptr != NULL)) return 0;
    if (!MSVCRT_CHECK_PMT(base == 0 || base >= 2)) return 0;
    if (!MSVCRT_CHECK_PMT(base <= 36)) return 0;

    while(isspaceW(*nptr)) nptr++;

    if(*nptr == '-') {
        negative = TRUE;
        nptr++;
    } else if(*nptr == '+')
        nptr++;

    if((base==0 || base==16) && *nptr=='0' && tolowerW(*(nptr+1))=='x') {
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
        MSVCRT_wchar_t cur = tolowerW(*nptr);
        int v;

        if(isdigitW(cur)) {
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
        *endptr = (MSVCRT_wchar_t*)nptr;

    return ret;
}

/*********************************************************************
 *  _wcstoi64 (MSVCRT.@)
 */
__int64 CDECL MSVCRT__wcstoi64(const MSVCRT_wchar_t *nptr,
        MSVCRT_wchar_t **endptr, int base)
{
    return MSVCRT__wcstoi64_l(nptr, endptr, base, NULL);
}

/*********************************************************************
 *  _wcstol_l (MSVCRT.@)
 */
MSVCRT_long CDECL MSVCRT__wcstol_l(const MSVCRT_wchar_t *s,
        MSVCRT_wchar_t **end, int base, MSVCRT__locale_t locale)
{
    __int64 ret = MSVCRT__wcstoi64_l(s, end, base, locale);

    if(ret > MSVCRT_LONG_MAX) {
        ret = MSVCRT_LONG_MAX;
        *MSVCRT__errno() = MSVCRT_ERANGE;
    }else if(ret < MSVCRT_LONG_MIN) {
        ret = MSVCRT_LONG_MIN;
        *MSVCRT__errno() = MSVCRT_ERANGE;
    }
    return ret;
}

/*********************************************************************
 *  _wtoi_l (MSVCRT.@)
 */
int __cdecl MSVCRT__wtoi_l(const MSVCRT_wchar_t *str, MSVCRT__locale_t locale)
{
    __int64 ret = MSVCRT__wcstoi64_l(str, NULL, 10, locale);

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
 *  _wtoi (MSVCRT.@)
 */
int __cdecl MSVCRT__wtoi(const MSVCRT_wchar_t *str)
{
    return MSVCRT__wtoi_l(str, NULL);
}

/*********************************************************************
 *  _wtol_l (MSVCRT.@)
 */
MSVCRT_long __cdecl MSVCRT__wtol_l(const MSVCRT_wchar_t *str, MSVCRT__locale_t locale)
{
    __int64 ret = MSVCRT__wcstoi64_l(str, NULL, 10, locale);

    if(ret > MSVCRT_LONG_MAX) {
        ret = MSVCRT_LONG_MAX;
        *MSVCRT__errno() = MSVCRT_ERANGE;
    } else if(ret < MSVCRT_LONG_MIN) {
        ret = MSVCRT_LONG_MIN;
        *MSVCRT__errno() = MSVCRT_ERANGE;
    }
    return ret;
}

/*********************************************************************
 *  _wtol (MSVCRT.@)
 */
MSVCRT_long __cdecl MSVCRT__wtol(const MSVCRT_wchar_t *str)
{
    return MSVCRT__wtol_l(str, NULL);
}

/*********************************************************************
 *  _wcstoui64_l (MSVCRT.@)
 *
 * FIXME: locale parameter is ignored
 */
unsigned __int64 CDECL MSVCRT__wcstoui64_l(const MSVCRT_wchar_t *nptr,
        MSVCRT_wchar_t **endptr, int base, MSVCRT__locale_t locale)
{
    BOOL negative = FALSE;
    unsigned __int64 ret = 0;

    TRACE("(%s %p %d %p)\n", debugstr_w(nptr), endptr, base, locale);

    if (!MSVCRT_CHECK_PMT(nptr != NULL)) return 0;
    if (!MSVCRT_CHECK_PMT(base == 0 || base >= 2)) return 0;
    if (!MSVCRT_CHECK_PMT(base <= 36)) return 0;

    while(isspaceW(*nptr)) nptr++;

    if(*nptr == '-') {
        negative = TRUE;
        nptr++;
    } else if(*nptr == '+')
        nptr++;

    if((base==0 || base==16) && *nptr=='0' && tolowerW(*(nptr+1))=='x') {
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
        MSVCRT_wchar_t cur = tolowerW(*nptr);
        int v;

        if(isdigitW(cur)) {
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
        *endptr = (MSVCRT_wchar_t*)nptr;

    return negative ? -ret : ret;
}

/*********************************************************************
 *  _wcstoui64 (MSVCRT.@)
 */
unsigned __int64 CDECL MSVCRT__wcstoui64(const MSVCRT_wchar_t *nptr,
        MSVCRT_wchar_t **endptr, int base)
{
    return MSVCRT__wcstoui64_l(nptr, endptr, base, NULL);
}

/*********************************************************************
 *  _wcstoul_l (MSVCRT.@)
 */
MSVCRT_ulong __cdecl MSVCRT__wcstoul_l(const MSVCRT_wchar_t *s,
        MSVCRT_wchar_t **end, int base, MSVCRT__locale_t locale)
{
    __int64 ret = MSVCRT__wcstoui64_l(s, end, base, locale);

    if(ret > MSVCRT_ULONG_MAX) {
        ret = MSVCRT_ULONG_MAX;
        *MSVCRT__errno() = MSVCRT_ERANGE;
    }
    return ret;
}

/*********************************************************************
   *  wcstoul (MSVCRT.@)
    */
MSVCRT_ulong __cdecl MSVCRT_wcstoul(const MSVCRT_wchar_t *s, MSVCRT_wchar_t **end, int base)
{
    return MSVCRT__wcstoul_l(s, end, base, NULL);
}

/******************************************************************
 *  wcsnlen (MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT_wcsnlen(const MSVCRT_wchar_t *s, MSVCRT_size_t maxlen)
{
    MSVCRT_size_t i;

    for (i = 0; i < maxlen; i++)
        if (!s[i]) break;
    return i;
}

/*********************************************************************
 *              _towupper_l (MSVCRT.@)
 */
int CDECL MSVCRT__towupper_l(MSVCRT_wint_t c, MSVCRT__locale_t locale)
{
    return toupperW(c);
}

/*********************************************************************
 *              towupper (MSVCRT.@)
 */
int CDECL MSVCRT_towupper(MSVCRT_wint_t c)
{
    return MSVCRT__towupper_l(c, NULL);
}

/*********************************************************************
 *              _towlower_l (MSVCRT.@)
 */
int CDECL MSVCRT__towlower_l(MSVCRT_wint_t c, MSVCRT__locale_t locale)
{
    return tolowerW(c);
}

/*********************************************************************
 *              towlower (MSVCRT.@)
 */
int CDECL MSVCRT_towlower(MSVCRT_wint_t c)
{
    return MSVCRT__towlower_l(c, NULL);
}

/*********************************************************************
 *              wcschr (MSVCRT.@)
 */
MSVCRT_wchar_t* CDECL MSVCRT_wcschr(const MSVCRT_wchar_t *str, MSVCRT_wchar_t ch)
{
    return strchrW(str, ch);
}

/***********************************************************************
 *              wcslen (MSVCRT.@)
 */
int CDECL MSVCRT_wcslen(const MSVCRT_wchar_t *str)
{
    return strlenW(str);
}

/*********************************************************************
 *              wcsstr (MSVCRT.@)
 */
MSVCRT_wchar_t* CDECL MSVCRT_wcsstr(const MSVCRT_wchar_t *str, const MSVCRT_wchar_t *sub)
{
    return strstrW(str, sub);
}

/*********************************************************************
 *              _wtoi64_l (MSVCRT.@)
 */
__int64 CDECL MSVCRT__wtoi64_l(const MSVCRT_wchar_t *str, MSVCRT__locale_t locale)
{
    ULONGLONG RunningTotal = 0;
    BOOL bMinus = FALSE;

    while (isspaceW(*str)) {
        str++;
    } /* while */

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
 *              _wtoi64 (MSVCRT.@)
 */
__int64 CDECL MSVCRT__wtoi64(const MSVCRT_wchar_t *str)
{
    return MSVCRT__wtoi64_l(str, NULL);
}

/*********************************************************************
 *           wcsncmp    (MSVCRT.@)
 */
int CDECL MSVCRT_wcsncmp(const MSVCRT_wchar_t *str1, const MSVCRT_wchar_t *str2, int n)
{
    return strncmpW(str1, str2, n);
}
