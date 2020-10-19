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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

typedef struct
{
    enum { LEN_DEFAULT, LEN_SHORT, LEN_LONG } IntegerLength;
    BOOLEAN IntegerDouble, IntegerNative, LeftAlign, Alternate, PadZero;
    BOOLEAN WideString, NaturalString;
    int FieldLength, Precision;
    char Sign, Format;
} pf_flags;

static BOOL n_format_enabled = TRUE;

#include "printf.h"
#define PRINTF_WIDE
#include "printf.h"
#undef PRINTF_WIDE

#if _MSVCR_VER>=80

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

#endif /* _MSVCR_VER>=80 */

/*********************************************************************
 *		_wcsdup (MSVCRT.@)
 */
MSVCRT_wchar_t* CDECL MSVCRT__wcsdup( const MSVCRT_wchar_t* str )
{
  MSVCRT_wchar_t* ret = NULL;
  if (str)
  {
    int size = (MSVCRT_wcslen(str) + 1) * sizeof(MSVCRT_wchar_t);
    ret = MSVCRT_malloc( size );
    if (ret) memcpy( ret, str, size );
  }
  return ret;
}

/*********************************************************************
 *              _towlower_l (MSVCRT.@)
 */
int CDECL MSVCRT__towlower_l(MSVCRT_wint_t c, MSVCRT__locale_t locale)
{
    MSVCRT_pthreadlocinfo locinfo;
    MSVCRT_wchar_t ret;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!locinfo->lc_handle[MSVCRT_LC_CTYPE]) {
        if(c >= 'A' && c <= 'Z')
            return c + 'a' - 'A';
        return c;
    }

    if(!LCMapStringW(locinfo->lc_handle[MSVCRT_LC_CTYPE], LCMAP_LOWERCASE, &c, 1, &ret, 1))
        return c;
    return ret;
}

/*********************************************************************
 *              towlower (MSVCRT.@)
 */
int CDECL MSVCRT_towlower(MSVCRT_wint_t c)
{
    return MSVCRT__towlower_l(c, NULL);
}

INT CDECL MSVCRT__wcsicmp_l(const MSVCRT_wchar_t *str1, const MSVCRT_wchar_t *str2, MSVCRT__locale_t locale)
{
    MSVCRT_wchar_t c1, c2;

    if(!MSVCRT_CHECK_PMT(str1 != NULL) || !MSVCRT_CHECK_PMT(str2 != NULL))
        return MSVCRT__NLSCMPERROR;

    do
    {
        c1 = MSVCRT__towlower_l(*str1++, locale);
        c2 = MSVCRT__towlower_l(*str2++, locale);
    } while(c1 && (c1 == c2));
    return c1 - c2;
}

/*********************************************************************
 *		_wcsicmp (MSVCRT.@)
 */
INT CDECL MSVCRT__wcsicmp( const MSVCRT_wchar_t* str1, const MSVCRT_wchar_t* str2 )
{
    return MSVCRT__wcsicmp_l(str1, str2, NULL);
}

/*********************************************************************
 *              _wcsnicmp_l (MSVCRT.@)
 */
INT CDECL MSVCRT__wcsnicmp_l(const MSVCRT_wchar_t *str1, const MSVCRT_wchar_t *str2,
        MSVCRT_size_t n, MSVCRT__locale_t locale)
{
    MSVCRT_wchar_t c1, c2;

    if (!n)
            return 0;

    if(!MSVCRT_CHECK_PMT(str1 != NULL) || !MSVCRT_CHECK_PMT(str2 != NULL))
        return MSVCRT__NLSCMPERROR;

    do
    {
        c1 = MSVCRT__towlower_l(*str1++, locale);
        c2 = MSVCRT__towlower_l(*str2++, locale);
    } while(--n && c1 && (c1 == c2));
    return c1 - c2;
}

/*********************************************************************
 *              _wcsnicmp (MSVCRT.@)
 */
INT CDECL MSVCRT__wcsnicmp(const MSVCRT_wchar_t *str1, const MSVCRT_wchar_t *str2, MSVCRT_size_t n)
{
    return MSVCRT__wcsnicmp_l(str1, str2, n, NULL);
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
    {
        MSVCRT_wchar_t c1, c2;

        do
        {
            c1 = *str1++;
            if (c1 >= 'A' && c1 <= 'Z')
                c1 += 'a' - 'A';

            c2 = *str2++;
            if (c2 >= 'A' && c2 <= 'Z')
                c2 += 'a' - 'A';
        } while(c1 && (c1 == c2));
        return c1 - c2;
    }

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
    {
        MSVCRT_wchar_t c1, c2;

        if (!count)
            return 0;

        do
        {
            c1 = *str1++;
            if (c1 >= 'A' && c1 <= 'Z')
                c1 += 'a' - 'A';

            c2 = *str2++;
            if (c2 >= 'A' && c2 <= 'Z')
                c2 += 'a' - 'A';
        } while(--count && c1 && (c1 == c2));
        return c1 - c2;
    }

    return CompareStringW(locinfo->lc_handle[MSVCRT_LC_COLLATE], NORM_IGNORECASE,
			  str1, MSVCRT_wcsnlen(str1, count),
                          str2, MSVCRT_wcsnlen(str2, count))-CSTR_EQUAL;
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
 *              _wcsnset_s (MSVCRT.@)
 */
int CDECL MSVCRT__wcsnset_s( MSVCRT_wchar_t *str, MSVCRT_size_t size, MSVCRT_wchar_t c, MSVCRT_size_t count )
{
    MSVCRT_size_t i;

    if(!str && !size && !count) return 0;
    if(!MSVCRT_CHECK_PMT(str != NULL)) return MSVCRT_EINVAL;
    if(!MSVCRT_CHECK_PMT(size > 0)) return MSVCRT_EINVAL;

    for(i=0; i<size-1 && i<count; i++) {
        if(!str[i]) return 0;
        str[i] = c;
    }
    for(; i<size; i++)
        if(!str[i]) return 0;

    str[0] = 0;
    MSVCRT__invalid_parameter(NULL, NULL, NULL, 0, 0);
    *MSVCRT__errno() = MSVCRT_EINVAL;
    return MSVCRT_EINVAL;
}

/*********************************************************************
 *		_wcsrev (MSVCRT.@)
 */
MSVCRT_wchar_t* CDECL MSVCRT__wcsrev( MSVCRT_wchar_t* str )
{
  MSVCRT_wchar_t* ret = str;
  MSVCRT_wchar_t* end = str + MSVCRT_wcslen(str) - 1;
  while (end > str)
  {
    MSVCRT_wchar_t t = *end;
    *end--  = *str;
    *str++  = t;
  }
  return ret;
}

/*********************************************************************
 *              _wcsset_s (MSVCRT.@)
 */
int CDECL MSVCRT__wcsset_s( MSVCRT_wchar_t *str, MSVCRT_size_t n, MSVCRT_wchar_t c )
{
    MSVCRT_wchar_t *p = str;

    if(!MSVCRT_CHECK_PMT(str != NULL)) return MSVCRT_EINVAL;
    if(!MSVCRT_CHECK_PMT(n)) return MSVCRT_EINVAL;

    while(*p && --n) *p++ = c;
    if(!n) {
        str[0] = 0;
        MSVCRT__invalid_parameter(NULL, NULL, NULL, 0, 0);
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return MSVCRT_EINVAL;
    }
    return 0;
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
    *ptr = MSVCRT__towupper_l(*ptr, locale);
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
 *              _wcsupr_l (MSVCRT.@)
 */
MSVCRT_wchar_t* CDECL MSVCRT__wcsupr_l( MSVCRT_wchar_t *str, MSVCRT__locale_t locale )
{
    MSVCRT__wcsupr_s_l( str, -1, locale);
    return str;
}

/******************************************************************
 *              _wcsupr (MSVCRT.@)
 */
MSVCRT_wchar_t* CDECL MSVCRT__wcsupr( MSVCRT_wchar_t *str )
{
    return MSVCRT__wcsupr_l(str, NULL);
}

/******************************************************************
 *		_wcslwr_s_l (MSVCRT.@)
 */
int CDECL MSVCRT__wcslwr_s_l( MSVCRT_wchar_t* str, MSVCRT_size_t n, MSVCRT__locale_t locale )
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
    *ptr = MSVCRT__towlower_l(*ptr, locale);
    ptr++;
  }

  /* MSDN claims that the function should return and set errno to
   * ERANGE, which doesn't seem to be true based on the tests. */
  *str = '\0';
  *MSVCRT__errno() = MSVCRT_EINVAL;
  return MSVCRT_EINVAL;
}

/******************************************************************
 *		_wcslwr_s (MSVCRT.@)
 */
int CDECL MSVCRT__wcslwr_s( MSVCRT_wchar_t* str, MSVCRT_size_t n )
{
    return MSVCRT__wcslwr_s_l(str, n, NULL);
}

/******************************************************************
 *		_wcslwr_l (MSVCRT.@)
 */
MSVCRT_wchar_t* CDECL MSVCRT__wcslwr_l( MSVCRT_wchar_t* str, MSVCRT__locale_t locale )
{
    MSVCRT__wcslwr_s_l(str, -1, locale);
    return str;
}

/******************************************************************
 *		_wcslwr (MSVCRT.@)
 */
MSVCRT_wchar_t* CDECL MSVCRT__wcslwr( MSVCRT_wchar_t* str )
{
    MSVCRT__wcslwr_s_l(str, -1, NULL);
    return str;
}

/*********************************************************************
 *           wcsncmp    (MSVCRT.@)
 */
int CDECL MSVCRT_wcsncmp(const MSVCRT_wchar_t *str1, const MSVCRT_wchar_t *str2, MSVCRT_size_t n)
{
    if (!n)
        return 0;
    while(--n && *str1 && (*str1 == *str2))
    {
        str1++;
        str2++;
    }
    return *str1 - *str2;
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
        return MSVCRT_wcsncmp(str1, str2, count);
    return CompareStringW(locinfo->lc_handle[MSVCRT_LC_COLLATE], 0,
              str1, MSVCRT_wcsnlen(str1, count),
              str2, MSVCRT_wcsnlen(str2, count))-CSTR_EQUAL;
}

/*********************************************************************
 *              _wcsncoll (MSVCRT.@)
 */
int CDECL MSVCRT__wcsncoll(const MSVCRT_wchar_t* str1, const MSVCRT_wchar_t* str2, MSVCRT_size_t count)
{
    return MSVCRT__wcsncoll_l(str1, str2, count, NULL);
}

static MSVCRT_wchar_t strtod_wstr_get(void *ctx)
{
    const MSVCRT_wchar_t **p = ctx;
    if (!**p) return MSVCRT_WEOF;
    return *(*p)++;
}

static void strtod_wstr_unget(void *ctx)
{
    const MSVCRT_wchar_t **p = ctx;
    (*p)--;
}

/*********************************************************************
 *		_wcstod_l (MSVCRT.@)
 */
double CDECL MSVCRT__wcstod_l(const MSVCRT_wchar_t* str, MSVCRT_wchar_t** end,
        MSVCRT__locale_t locale)
{
    MSVCRT_pthreadlocinfo locinfo;
    const MSVCRT_wchar_t *beg, *p;
    struct fpnum fp;
    double ret;
    int err;

    if (!MSVCRT_CHECK_PMT(str != NULL)) {
        if (end) *end = NULL;
        return 0;
    }

    if (!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    p = str;
    while(MSVCRT__iswspace_l(*p, locale))
        p++;
    beg = p;

    fp = fpnum_parse(strtod_wstr_get, strtod_wstr_unget, &p, locinfo, FALSE);
    if (end) *end = (p == beg ? (MSVCRT_wchar_t*)str : (MSVCRT_wchar_t*)p);

    err = fpnum_double(&fp, &ret);
    if(err) *MSVCRT__errno() = err;
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
    BOOL used_default = FALSE;
    BOOL *pused_default;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!locinfo->lc_codepage) {
        MSVCRT_size_t i;

        if(!mbstr)
            return MSVCRT_wcslen(*wcstr);

        for(i=0; i<count; i++) {
            if((*wcstr)[i] > 255) {
                *MSVCRT__errno() = MSVCRT_EILSEQ;
                return -1;
            }

            mbstr[i] = (*wcstr)[i];
            if(!(*wcstr)[i]) break;
        }
        return i;
    }

    pused_default = (locinfo->lc_codepage != CP_UTF8 ? &used_default : NULL);

    if(!mbstr) {
        tmp = WideCharToMultiByte(locinfo->lc_codepage, WC_NO_BEST_FIT_CHARS,
                *wcstr, -1, NULL, 0, NULL, pused_default);
        if(!tmp || used_default) {
            *MSVCRT__errno() = MSVCRT_EILSEQ;
            return -1;
        }
        return tmp-1;
    }

    while(**wcstr) {
        char buf[3];
        MSVCRT_size_t i, size;

        size = WideCharToMultiByte(locinfo->lc_codepage, WC_NO_BEST_FIT_CHARS,
                *wcstr, 1, buf, 3, NULL, pused_default);
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
    int err;

    if(!mbstr && !size && wcstr) {
        conv = MSVCRT_wcsrtombs_l(NULL, wcstr, 0, locale);
        if(ret)
            *ret = conv+1;
        if(conv == -1)
            return *MSVCRT__errno();
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

    err = 0;
    conv = MSVCRT_wcsrtombs_l(mbstr, wcstr, conv, locale);
    if(conv == -1) {
        conv = 0;
        if(size)
            mbstr[0] = '\0';
        err = *MSVCRT__errno();
    }else if(conv < size)
        mbstr[conv++] = '\0';
    else if(conv==size && (count==MSVCRT__TRUNCATE || mbstr[conv-1]=='\0')) {
        mbstr[conv-1] = '\0';
        if(count==MSVCRT__TRUNCATE)
            err = MSVCRT_STRUNCATE;
    }else {
        MSVCRT_INVALID_PMT("mbstr[size] is too small", MSVCRT_ERANGE);
        conv = 0;
        if(size)
            mbstr[0] = '\0';
        err = MSVCRT_ERANGE;
    }

    if(ret)
        *ret = conv;
    return err;
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

#if _MSVCR_VER>=120

/*********************************************************************
 *              _wcstof_l  (MSVCR120.@)
 */
float CDECL MSVCRT__wcstof_l( const MSVCRT_wchar_t *str, MSVCRT_wchar_t **end, MSVCRT__locale_t locale )
{
    return MSVCRT__wcstod_l(str, end, locale);
}

/*********************************************************************
 *              wcstof  (MSVCR120.@)
 */
float CDECL MSVCRT_wcstof( const MSVCRT_wchar_t *str, MSVCRT_wchar_t **end )
{
    return MSVCRT__wcstof_l(str, end, NULL);
}

#endif /* _MSVCR_VER>=120 */

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
printf_arg arg_clbk_positional(void *ctx, int pos, int type, __ms_va_list *valist)
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

    ret = pf_printf_a(puts_clbk_str_a, &ctx, format, NULL, 0,
            arg_clbk_valist, NULL, &valist);
    puts_clbk_str_a(&ctx, 1, &nullbyte);
    return ret;
}

#if _MSVCR_VER>=140

static int puts_clbk_str_c99_a(void *ctx, int len, const char *str)
{
    struct _str_ctx_a *out = ctx;

    if(!out->buf)
        return len;

    if(out->len < len) {
        memmove(out->buf, str, out->len);
        out->buf += out->len;
        out->len = 0;
        return len;
    }

    memmove(out->buf, str, len);
    out->buf += len;
    out->len -= len;
    return len;
}

/*********************************************************************
 *              __stdio_common_vsprintf (UCRTBASE.@)
 */
int CDECL MSVCRT__stdio_common_vsprintf( unsigned __int64 options, char *str, MSVCRT_size_t len, const char *format,
                                         MSVCRT__locale_t locale, __ms_va_list valist )
{
    static const char nullbyte = '\0';
    struct _str_ctx_a ctx = {len, str};
    int ret;

    if (options & ~UCRTBASE_PRINTF_MASK)
        FIXME("options %s not handled\n", wine_dbgstr_longlong(options));
    ret = pf_printf_a(puts_clbk_str_c99_a,
            &ctx, format, locale, options & UCRTBASE_PRINTF_MASK, arg_clbk_valist, NULL, &valist);
    puts_clbk_str_a(&ctx, 1, &nullbyte);

    if(!str)
        return ret;
    if(options & UCRTBASE_PRINTF_LEGACY_VSPRINTF_NULL_TERMINATION)
        return ret>len ? -1 : ret;
    if(ret>=len) {
        if(len) str[len-1] = 0;
        if(options & UCRTBASE_PRINTF_STANDARD_SNPRINTF_BEHAVIOUR)
            return ret;
        return len > 0 ? -2 : -1;
    }
    return ret;
}

#endif /* _MSVCR_VER>=140 */

/*********************************************************************
 *		_vsnprintf_l (MSVCRT.@)
 */
int CDECL MSVCRT_vsnprintf_l( char *str, MSVCRT_size_t len, const char *format,
                            MSVCRT__locale_t locale, __ms_va_list valist )
{
    static const char nullbyte = '\0';
    struct _str_ctx_a ctx = {len, str};
    int ret;

    ret = pf_printf_a(puts_clbk_str_a, &ctx, format, locale, 0,
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
int WINAPIV MSVCRT_sprintf_l(char *str, const char *format,
                           MSVCRT__locale_t locale, ...)
{
    int retval;
    __ms_va_list valist;
    __ms_va_start(valist, locale);
    retval = MSVCRT_vsnprintf_l(str, INT_MAX, format, locale, valist);
    __ms_va_end(valist);
    return retval;
}

static int CDECL MSVCRT_vsnprintf_s_l_opt( char *str, MSVCRT_size_t sizeOfBuffer,
        MSVCRT_size_t count, const char *format, DWORD options,
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
    ret = pf_printf_a(puts_clbk_str_a, &ctx, format, locale, MSVCRT_PRINTF_INVOKE_INVALID_PARAM_HANDLER | options,
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

static int MSVCRT_vsnwprintf_s_l_opt( MSVCRT_wchar_t *str, MSVCRT_size_t sizeOfBuffer,
        MSVCRT_size_t count, const MSVCRT_wchar_t *format, DWORD options,
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
    ret = pf_printf_w(puts_clbk_str_w, &ctx, format, locale, MSVCRT_PRINTF_INVOKE_INVALID_PARAM_HANDLER | options,
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
 *		_vsnprintf_s_l (MSVCRT.@)
 */
int CDECL MSVCRT_vsnprintf_s_l( char *str, MSVCRT_size_t sizeOfBuffer,
        MSVCRT_size_t count, const char *format,
        MSVCRT__locale_t locale, __ms_va_list valist )
{
    return MSVCRT_vsnprintf_s_l_opt(str, sizeOfBuffer, count, format, 0, locale, valist);
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
int WINAPIV MSVCRT_sprintf_s_l( char *str, MSVCRT_size_t count, const char *format,
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
 *              _vsnprintf_c_l (MSVCRT.@)
 */
int CDECL MSVCRT_vsnprintf_c_l(char *str, MSVCRT_size_t len, const char *format,
        MSVCRT__locale_t locale, __ms_va_list valist)
{
    return MSVCRT_vsnprintf_s_l_opt(str, len, len, format, 0, locale, valist);
}

/*********************************************************************
 *              _vsnprintf_c (MSVCRT.@)
 */
int CDECL MSVCRT_vsnprintf_c(char *str, MSVCRT_size_t len,
        const char *format, __ms_va_list valist)
{
    return MSVCRT_vsnprintf_c_l(str, len, format, NULL, valist);
}

#if _MSVCR_VER>=140

/*********************************************************************
 *              __stdio_common_vsnprintf_s (UCRTBASE.@)
 */
int CDECL MSVCRT__stdio_common_vsnprintf_s( unsigned __int64 options,
        char *str, MSVCRT_size_t sizeOfBuffer, MSVCRT_size_t count,
        const char *format, MSVCRT__locale_t locale, __ms_va_list valist )
{
    if (options & ~UCRTBASE_PRINTF_MASK)
        FIXME("options %s not handled\n", wine_dbgstr_longlong(options));
    return MSVCRT_vsnprintf_s_l_opt(str, sizeOfBuffer, count, format, options & UCRTBASE_PRINTF_MASK, locale, valist);
}

/*********************************************************************
 *              __stdio_common_vsnwprintf_s (UCRTBASE.@)
 */
int CDECL MSVCRT__stdio_common_vsnwprintf_s( unsigned __int64 options,
        MSVCRT_wchar_t *str, MSVCRT_size_t sizeOfBuffer, MSVCRT_size_t count,
        const MSVCRT_wchar_t *format, MSVCRT__locale_t locale, __ms_va_list valist )
{
    if (options & ~UCRTBASE_PRINTF_MASK)
        FIXME("options %s not handled\n", wine_dbgstr_longlong(options));
    return MSVCRT_vsnwprintf_s_l_opt(str, sizeOfBuffer, count, format, options & UCRTBASE_PRINTF_MASK, locale, valist);
}

/*********************************************************************
 *              __stdio_common_vswprintf_s (UCRTBASE.@)
 */
int CDECL MSVCRT__stdio_common_vswprintf_s( unsigned __int64 options,
        MSVCRT_wchar_t *str, MSVCRT_size_t count, const MSVCRT_wchar_t *format,
        MSVCRT__locale_t locale, __ms_va_list valist )
{
    return MSVCRT__stdio_common_vsnwprintf_s(options, str, INT_MAX, count, format, locale, valist);
}

/*********************************************************************
 *              __stdio_common_vsprintf_s (UCRTBASE.@)
 */
int CDECL MSVCRT__stdio_common_vsprintf_s( unsigned __int64 options,
        char *str, MSVCRT_size_t count, const char *format,
        MSVCRT__locale_t locale, __ms_va_list valist )
{
    if (options & ~UCRTBASE_PRINTF_MASK)
        FIXME("options %s not handled\n", wine_dbgstr_longlong(options));
    return MSVCRT_vsnprintf_s_l_opt(str, INT_MAX, count, format, options & UCRTBASE_PRINTF_MASK, locale, valist);
}

#endif /* _MSVCR_VER>=140 */

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
 *              _vscprintf_l (MSVCRT.@)
 */
int CDECL MSVCRT__vscprintf_l(const char *format,
        MSVCRT__locale_t locale, __ms_va_list valist)
{
    return MSVCRT_vsnprintf_l(NULL, INT_MAX, format, locale, valist);
}

/*********************************************************************
 *		_vscprintf_p_l (MSVCRT.@)
 */
int CDECL MSVCRT__vscprintf_p_l(const char *format,
        MSVCRT__locale_t locale, __ms_va_list args)
{
    printf_arg args_ctx[MSVCRT__ARGMAX+1];
    struct _str_ctx_a puts_ctx = {INT_MAX, NULL};
    int ret;

    memset(args_ctx, 0, sizeof(args_ctx));

    ret = create_positional_ctx_a(args_ctx, format, args);
    if(ret < 0)  {
        MSVCRT__invalid_parameter(NULL, NULL, NULL, 0, 0);
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return ret;
    } else if(ret == 0) {
        ret = pf_printf_a(puts_clbk_str_a, &puts_ctx, format, locale,
                MSVCRT_PRINTF_INVOKE_INVALID_PARAM_HANDLER,
                arg_clbk_valist, NULL, &args);
    } else {
        ret = pf_printf_a(puts_clbk_str_a, &puts_ctx, format, locale,
                MSVCRT_PRINTF_POSITIONAL_PARAMS | MSVCRT_PRINTF_INVOKE_INVALID_PARAM_HANDLER,
                arg_clbk_positional, args_ctx, NULL);
    }

    return ret;
}

/*********************************************************************
 *		_vscprintf_p (MSVCR80.@)
 */
int CDECL MSVCRT__vscprintf_p(const char *format, __ms_va_list argptr)
{
    return MSVCRT__vscprintf_p_l(format, NULL, argptr);
}

/*********************************************************************
 *		_snprintf (MSVCRT.@)
 */
int WINAPIV MSVCRT__snprintf(char *str, MSVCRT_size_t len, const char *format, ...)
{
    int retval;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    retval = MSVCRT_vsnprintf(str, len, format, valist);
    __ms_va_end(valist);
    return retval;
}

/*********************************************************************
 *		_snprintf_l (MSVCRT.@)
 */
int WINAPIV MSVCRT__snprintf_l(char *str, MSVCRT_size_t count, const char *format,
        MSVCRT__locale_t locale, ...)
{
    int retval;
    __ms_va_list valist;
    __ms_va_start(valist, locale);
    retval = MSVCRT_vsnprintf_l(str, count, format, locale, valist);
    __ms_va_end(valist);
    return retval;
}

/*********************************************************************
 *              _snprintf_c_l (MSVCRT.@)
 */
int WINAPIV MSVCRT_snprintf_c_l(char *str, MSVCRT_size_t count, const char *format,
        MSVCRT__locale_t locale, ...)
{
    int retval;
    __ms_va_list valist;
    __ms_va_start(valist, locale);
    retval = MSVCRT_vsnprintf_c_l(str, count, format, locale, valist);
    __ms_va_end(valist);
    return retval;
}

/*********************************************************************
 *              _snprintf_c (MSVCRT.@)
 */
int WINAPIV MSVCRT_snprintf_c(char *str, MSVCRT_size_t count, const char *format, ...)
{
    int retval;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    retval = MSVCRT_vsnprintf_c(str, count, format, valist);
    __ms_va_end(valist);
    return retval;
}

/*********************************************************************
 *              _snprintf_s_l (MSVCRT.@)
 */
int WINAPIV MSVCRT_snprintf_s_l(char *str, MSVCRT_size_t len, MSVCRT_size_t count,
        const char *format, MSVCRT__locale_t locale, ...)
{
    int retval;
    __ms_va_list valist;
    __ms_va_start(valist, locale);
    retval = MSVCRT_vsnprintf_s_l(str, len, count, format, locale, valist);
    __ms_va_end(valist);
    return retval;
}

/*********************************************************************
 *		_snprintf_s (MSVCRT.@)
 */
int WINAPIV MSVCRT__snprintf_s(char *str, MSVCRT_size_t len, MSVCRT_size_t count,
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
int WINAPIV MSVCRT__scprintf(const char *format, ...)
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

    ret = pf_printf_w(puts_clbk_str_w, &ctx, format, NULL, 0,
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

    ret = pf_printf_w(puts_clbk_str_w, &ctx, format, locale, 0,
            arg_clbk_valist, NULL, &valist);
    puts_clbk_str_w(&ctx, 1, &nullbyte);
    return ret;
}

/*********************************************************************
 *              _vswprintf_c_l (MSVCRT.@)
 */
int CDECL MSVCRT_vswprintf_c_l(MSVCRT_wchar_t *str, MSVCRT_size_t len,
        const MSVCRT_wchar_t *format, MSVCRT__locale_t locale,
        __ms_va_list valist)
{
    return MSVCRT_vsnwprintf_s_l_opt(str, len, len, format, 0, locale, valist);
}

/*********************************************************************
 *              _vswprintf_c (MSVCRT.@)
 */
int CDECL MSVCRT_vswprintf_c(MSVCRT_wchar_t *str, MSVCRT_size_t len,
        const MSVCRT_wchar_t *format, __ms_va_list valist)
{
    return MSVCRT_vswprintf_c_l(str, len, format, NULL, valist);
}

static int MSVCRT_vswprintf_p_l_opt(MSVCRT_wchar_t *buffer, MSVCRT_size_t length,
        const MSVCRT_wchar_t *format, DWORD options, MSVCRT__locale_t locale, __ms_va_list args)
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
        ret = pf_printf_w(puts_clbk_str_w, &puts_ctx, format, locale, MSVCRT_PRINTF_INVOKE_INVALID_PARAM_HANDLER | options,
                arg_clbk_valist, NULL, &args);
    else
        ret = pf_printf_w(puts_clbk_str_w, &puts_ctx, format, locale,
                MSVCRT_PRINTF_POSITIONAL_PARAMS | MSVCRT_PRINTF_INVOKE_INVALID_PARAM_HANDLER | options,
                arg_clbk_positional, args_ctx, NULL);

    puts_clbk_str_w(&puts_ctx, 1, &nullbyte);
    return ret;
}

/*********************************************************************
 *		_vswprintf_p_l (MSVCRT.@)
 */
int CDECL MSVCRT_vswprintf_p_l(MSVCRT_wchar_t *buffer, MSVCRT_size_t length,
        const MSVCRT_wchar_t *format, MSVCRT__locale_t locale, __ms_va_list args)
{
    return MSVCRT_vswprintf_p_l_opt(buffer, length, format, 0, locale, args);
}

#if _MSVCR_VER>=80
/*********************************************************************
 * _vswprintf_p (MSVCR80.@)
 */
int CDECL MSVCRT__vswprintf_p(MSVCRT_wchar_t *buffer, MSVCRT_size_t length,
        const MSVCRT_wchar_t *format, __ms_va_list args)
{
    return MSVCRT_vswprintf_p_l_opt(buffer, length, format, 0, NULL, args);
}
#endif

#if _MSVCR_VER>=140
/*********************************************************************
 *              __stdio_common_vswprintf_p (UCRTBASE.@)
 */
int CDECL MSVCRT__stdio_common_vswprintf_p( unsigned __int64 options,
        MSVCRT_wchar_t *str, MSVCRT_size_t count, const MSVCRT_wchar_t *format,
        MSVCRT__locale_t locale, __ms_va_list valist )
{
    if (options & ~UCRTBASE_PRINTF_MASK)
        FIXME("options %s not handled\n", wine_dbgstr_longlong(options));
    return MSVCRT_vswprintf_p_l_opt(str, count, format, options & UCRTBASE_PRINTF_MASK, locale, valist);
}
#endif

/*********************************************************************
 *              _vsnwprintf_s_l (MSVCRT.@)
 */
int CDECL MSVCRT_vsnwprintf_s_l( MSVCRT_wchar_t *str, MSVCRT_size_t sizeOfBuffer,
        MSVCRT_size_t count, const MSVCRT_wchar_t *format,
        MSVCRT__locale_t locale, __ms_va_list valist)
{
    return MSVCRT_vsnwprintf_s_l_opt(str, sizeOfBuffer, count, format, 0, locale, valist);
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
int WINAPIV MSVCRT__snwprintf( MSVCRT_wchar_t *str, MSVCRT_size_t len, const MSVCRT_wchar_t *format, ...)
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
int WINAPIV MSVCRT__snwprintf_l( MSVCRT_wchar_t *str, MSVCRT_size_t len, const MSVCRT_wchar_t *format,
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
int WINAPIV MSVCRT__snwprintf_s( MSVCRT_wchar_t *str, MSVCRT_size_t len, MSVCRT_size_t count,
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
int WINAPIV MSVCRT__snwprintf_s_l( MSVCRT_wchar_t *str, MSVCRT_size_t len, MSVCRT_size_t count,
        const MSVCRT_wchar_t *format, MSVCRT__locale_t locale, ... )
{
    int retval;
    __ms_va_list valist;
    __ms_va_start(valist, locale);
    retval = MSVCRT_vsnwprintf_s_l(str, len, count, format, locale, valist);
    __ms_va_end(valist);
    return retval;
}

#if _MSVCR_VER>=140

static int puts_clbk_str_c99_w(void *ctx, int len, const MSVCRT_wchar_t *str)
{
    struct _str_ctx_w *out = ctx;

    if(!out->buf)
        return len;

    if(out->len < len) {
        memcpy(out->buf, str, out->len*sizeof(MSVCRT_wchar_t));
        out->buf += out->len;
        out->len = 0;
        return len;
    }

    memcpy(out->buf, str, len*sizeof(MSVCRT_wchar_t));
    out->buf += len;
    out->len -= len;
    return len;
}

/*********************************************************************
 *              __stdio_common_vswprintf (UCRTBASE.@)
 */
int CDECL MSVCRT__stdio_common_vswprintf( unsigned __int64 options,
        MSVCRT_wchar_t *str, MSVCRT_size_t len, const MSVCRT_wchar_t *format,
        MSVCRT__locale_t locale, __ms_va_list valist )
{
    static const MSVCRT_wchar_t nullbyte = '\0';
    struct _str_ctx_w ctx = {len, str};
    int ret;

    if (options & ~UCRTBASE_PRINTF_MASK)
        FIXME("options %s not handled\n", wine_dbgstr_longlong(options));
    ret = pf_printf_w(puts_clbk_str_c99_w,
            &ctx, format, locale, options & UCRTBASE_PRINTF_MASK, arg_clbk_valist, NULL, &valist);
    puts_clbk_str_w(&ctx, 1, &nullbyte);

    if(!str)
        return ret;
    if(options & UCRTBASE_PRINTF_LEGACY_VSPRINTF_NULL_TERMINATION)
        return ret>len ? -1 : ret;
    if(ret>=len) {
        if(len) str[len-1] = 0;
        if(options & UCRTBASE_PRINTF_STANDARD_SNPRINTF_BEHAVIOUR)
            return ret;
        return len > 0 ? -2 : -1;
    }
    return ret;
}

#endif /* _MSVCR_VER>=140 */

/*********************************************************************
 *		sprintf (MSVCRT.@)
 */
int WINAPIV MSVCRT_sprintf( char *str, const char *format, ... )
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
int WINAPIV MSVCRT_sprintf_s( char *str, MSVCRT_size_t num, const char *format, ... )
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
int WINAPIV MSVCRT__scwprintf( const MSVCRT_wchar_t *format, ... )
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
int WINAPIV MSVCRT_swprintf( MSVCRT_wchar_t *str, const MSVCRT_wchar_t *format, ... )
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
int WINAPIV MSVCRT_swprintf_s(MSVCRT_wchar_t *str, MSVCRT_size_t numberOfElements,
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
int WINAPIV MSVCRT__swprintf_s_l(MSVCRT_wchar_t *str, MSVCRT_size_t numberOfElements,
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
 *              _swprintf_c_l (MSVCRT.@)
 */
int WINAPIV MSVCRT_swprintf_c_l(MSVCRT_wchar_t *str, MSVCRT_size_t len,
        const MSVCRT_wchar_t *format, MSVCRT__locale_t locale, ... )
{
    __ms_va_list ap;
    int r;

    __ms_va_start(ap, locale);
    r = MSVCRT_vswprintf_c_l(str, len, format, locale, ap);
    __ms_va_end(ap);

    return r;
}

/*********************************************************************
 *              _swprintf_c (MSVCRT.@)
 */
int WINAPIV MSVCRT_swprintf_c(MSVCRT_wchar_t *str, MSVCRT_size_t len,
        const MSVCRT_wchar_t *format, ... )
{
    __ms_va_list ap;
    int r;

    __ms_va_start(ap, format);
    r = MSVCRT_vswprintf_c(str, len, format, ap);
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
    return MSVCRT_vswprintf_p_l_opt( NULL, INT_MAX, format, 0, locale, args );
}

#if _MSVCR_VER>=80
/*********************************************************************
 * _vscwprintf_p (MSVCR80.@)
 */
int CDECL MSVCRT__vscwprintf_p(const MSVCRT_wchar_t *format, __ms_va_list args)
{
    return MSVCRT_vswprintf_p_l_opt(NULL, INT_MAX, format, 0, NULL, args);
}
#endif

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

static int MSVCRT_vsprintf_p_l_opt(char *buffer, MSVCRT_size_t length, const char *format,
        DWORD options, MSVCRT__locale_t locale, __ms_va_list args)
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
        ret = pf_printf_a(puts_clbk_str_a, &puts_ctx, format, locale,
                MSVCRT_PRINTF_INVOKE_INVALID_PARAM_HANDLER | options, arg_clbk_valist, NULL, &args);
    else
        ret = pf_printf_a(puts_clbk_str_a, &puts_ctx, format, locale,
                MSVCRT_PRINTF_POSITIONAL_PARAMS | MSVCRT_PRINTF_INVOKE_INVALID_PARAM_HANDLER | options,
                arg_clbk_positional, args_ctx, NULL);

    puts_clbk_str_a(&puts_ctx, 1, &nullbyte);
    return ret;
}

/*********************************************************************
 *              _vsprintf_p_l (MSVCRT.@)
 */
int CDECL MSVCRT_vsprintf_p_l(char *buffer, MSVCRT_size_t length, const char *format,
        MSVCRT__locale_t locale, __ms_va_list args)
{
    return MSVCRT_vsprintf_p_l_opt(buffer, length, format, 0, locale, args);
}

/*********************************************************************
 *		_vsprintf_p (MSVCRT.@)
 */
int CDECL MSVCRT_vsprintf_p(char *buffer, MSVCRT_size_t length,
        const char *format, __ms_va_list args)
{
    return MSVCRT_vsprintf_p_l(buffer, length, format, NULL, args);
}

#if _MSVCR_VER>=140
/*********************************************************************
 *              __stdio_common_vsprintf_p (UCRTBASE.@)
 */
int CDECL MSVCRT__stdio_common_vsprintf_p(unsigned __int64 options, char *buffer, MSVCRT_size_t length,
        const char *format, MSVCRT__locale_t locale, __ms_va_list args)
{
    if (options & ~UCRTBASE_PRINTF_MASK)
        FIXME("options %s not handled\n", wine_dbgstr_longlong(options));
    return MSVCRT_vsprintf_p_l_opt(buffer, length, format, options & UCRTBASE_PRINTF_MASK, locale, args);
}
#endif

/*********************************************************************
 *		_sprintf_p_l (MSVCRT.@)
 */
int WINAPIV MSVCRT_sprintf_p_l(char *buffer, MSVCRT_size_t length,
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
 *		__swprintf_l (MSVCRT.@)
 */
int WINAPIV MSVCRT___swprintf_l( MSVCRT_wchar_t *str, const MSVCRT_wchar_t *format,
        MSVCRT__locale_t locale, ...)
{
    int retval;
    __ms_va_list valist;
    __ms_va_start(valist, locale);
    retval = MSVCRT_vswprintf_l(str, format, locale, valist);
    __ms_va_end(valist);
    return retval;
}

#if _MSVCR_VER>=80
/*********************************************************************
 * _sprintf_p (MSVCR80.@)
 */
int WINAPIV MSVCRT__sprintf_p(char *buffer, MSVCRT_size_t length, const char *format, ...)
{
    __ms_va_list valist;
    int r;

    __ms_va_start(valist, format);
    r = MSVCRT_vsprintf_p_l(buffer, length, format, NULL, valist);
    __ms_va_end(valist);

    return r;
}
#endif

/*********************************************************************
 *		_swprintf_p_l (MSVCRT.@)
 */
int WINAPIV MSVCRT_swprintf_p_l(MSVCRT_wchar_t *buffer, MSVCRT_size_t length,
        const MSVCRT_wchar_t *format, MSVCRT__locale_t locale, ...)
{
    __ms_va_list valist;
    int r;

    __ms_va_start(valist, locale);
    r = MSVCRT_vswprintf_p_l_opt(buffer, length, format, 0, locale, valist);
    __ms_va_end(valist);

    return r;
}

/*********************************************************************
 *              wcscmp (MSVCRT.@)
 */
int CDECL MSVCRT_wcscmp(const MSVCRT_wchar_t *str1, const MSVCRT_wchar_t *str2)
{
    while (*str1 && (*str1 == *str2))
    {
        str1++;
        str2++;
    }

    if (*str1 < *str2)
        return -1;
    if (*str1 > *str2)
        return 1;
    return 0;
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
        return MSVCRT_wcscmp(str1, str2);
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

    while (*str && MSVCRT_wcschr( delim, *str )) str++;
    if (!*str) return NULL;
    ret = str++;
    while (*str && !MSVCRT_wcschr( delim, *str )) str++;
    if (*str) *str++ = 0;
    *next_token = str;
    return ret;
}

/*********************************************************************
 *		wcstok  (MSVCRT.@)
 */
#if _MSVCR_VER>=140
MSVCRT_wchar_t * CDECL MSVCRT_wcstok( MSVCRT_wchar_t *str, const MSVCRT_wchar_t *delim, MSVCRT_wchar_t **ctx )
{
    if (!ctx)
        ctx = &msvcrt_get_thread_data()->wcstok_next;
    return MSVCRT_wcstok_s(str, delim, ctx);
}
#else
MSVCRT_wchar_t * CDECL MSVCRT_wcstok( MSVCRT_wchar_t *str, const MSVCRT_wchar_t *delim )
{
    return MSVCRT_wcstok_s(str, delim, &msvcrt_get_thread_data()->wcstok_next);
}
#endif

/*********************************************************************
 *		_wctomb_s_l (MSVCRT.@)
 */
int CDECL MSVCRT__wctomb_s_l(int *len, char *mbchar, MSVCRT_size_t size,
        MSVCRT_wchar_t wch, MSVCRT__locale_t locale)
{
    MSVCRT_pthreadlocinfo locinfo;
    BOOL error = FALSE;
    BOOL *perror;
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

    perror = (locinfo->lc_codepage != CP_UTF8 ? &error : NULL);
    mblen = WideCharToMultiByte(locinfo->lc_codepage, 0, &wch, 1, mbchar, size, NULL, perror);
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

    MSVCRT__wctomb_s_l(&len, dst, dst ? MSVCRT_MB_LEN_MAX : 0, ch, locale);
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
    BOOL error = FALSE;
    BOOL *perror;
    UINT codepage = get_locinfo()->lc_codepage;

    perror = (codepage != CP_UTF8 ? &error : NULL);

    if(!codepage) {
        if (wchar < 0xff)
            return (signed char)wchar;
        else
            return MSVCRT_EOF;
    } else if(WideCharToMultiByte( codepage, 0, &wchar, 1, &out, 1, NULL, perror ) && !error)
        return (INT)out;
    return MSVCRT_EOF;
}

/*********************************************************************
 *              wcrtomb_s (MSVCRT.@)
 */
INT CDECL MSVCRT_wcrtomb_s(MSVCRT_size_t *len, char *mbchar,
        MSVCRT_size_t size, MSVCRT_wchar_t wch, MSVCRT_mbstate_t *s)
{
    int ilen, ret;

    if (s) *s = 0;
    ret = MSVCRT_wctomb_s(&ilen, mbchar, size, wch);
    if (len) *len = ilen;
    return ret;
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
 *		_iswctype_l (MSVCRT.@)
 */
INT CDECL MSVCRT__iswctype_l( MSVCRT_wchar_t wc, MSVCRT_wctype_t type, MSVCRT__locale_t locale )
{
    WORD ct;

    if (wc == MSVCRT_WEOF) return 0;
    if (wc < 256) return MSVCRT__pwctype[wc] & type;

    if (!GetStringTypeW(CT_CTYPE1, &wc, 1, &ct))
    {
        ERR("GetStringTypeW failed for %x\n", wc);
        return 0;
    }
    return ct & type;
}

/*********************************************************************
 *		iswctype    (MSVCRT.@)
 */
INT CDECL MSVCRT_iswctype( MSVCRT_wchar_t wc, MSVCRT_wctype_t type )
{
    return MSVCRT__iswctype_l( wc, type, NULL );
}

/*********************************************************************
 *		_iswalnum_l (MSVCRT.@)
 */
int CDECL MSVCRT__iswalnum_l( MSVCRT_wchar_t wc, MSVCRT__locale_t locale )
{
    return MSVCRT__iswctype_l( wc, MSVCRT__ALPHA | MSVCRT__DIGIT, locale );
}

/*********************************************************************
 *		iswalnum (MSVCRT.@)
 */
INT CDECL MSVCRT_iswalnum( MSVCRT_wchar_t wc )
{
    return MSVCRT__iswalnum_l( wc, NULL );
}

/*********************************************************************
 *              iswalpha_l (MSVCRT.@)
 */
INT CDECL MSVCRT__iswalpha_l( MSVCRT_wchar_t wc, MSVCRT__locale_t locale )
{
    return MSVCRT__iswctype_l( wc, MSVCRT__ALPHA, locale );
}

/*********************************************************************
 *		iswalpha (MSVCRT.@)
 */
INT CDECL MSVCRT_iswalpha( MSVCRT_wchar_t wc )
{
    return MSVCRT__iswalpha_l( wc, NULL );
}

/*********************************************************************
 *		_iswcntrl_l (MSVCRT.@)
 */
int CDECL MSVCRT__iswcntrl_l( MSVCRT_wchar_t wc, MSVCRT__locale_t locale )
{
    return MSVCRT__iswctype_l( wc, MSVCRT__CONTROL, locale );
}

/*********************************************************************
 *		iswcntrl (MSVCRT.@)
 */
INT CDECL MSVCRT_iswcntrl( MSVCRT_wchar_t wc )
{
    return MSVCRT__iswcntrl_l( wc, NULL );
}

/*********************************************************************
 *		_iswdigit_l (MSVCRT.@)
 */
INT CDECL MSVCRT__iswdigit_l( MSVCRT_wchar_t wc, MSVCRT__locale_t locale )
{
    return MSVCRT__iswctype_l( wc, MSVCRT__DIGIT, locale );
}

/*********************************************************************
 *		iswdigit (MSVCRT.@)
 */
INT CDECL MSVCRT_iswdigit( MSVCRT_wchar_t wc )
{
    return MSVCRT__iswdigit_l( wc, NULL );
}

/*********************************************************************
 *		_iswgraph_l (MSVCRT.@)
 */
int CDECL MSVCRT__iswgraph_l( MSVCRT_wchar_t wc, MSVCRT__locale_t locale )
{
    return MSVCRT__iswctype_l( wc, MSVCRT__ALPHA | MSVCRT__DIGIT | MSVCRT__PUNCT, locale );
}

/*********************************************************************
 *		iswgraph (MSVCRT.@)
 */
INT CDECL MSVCRT_iswgraph( MSVCRT_wchar_t wc )
{
    return MSVCRT__iswgraph_l( wc, NULL );
}

/*********************************************************************
 *		_iswlower_l (MSVCRT.@)
 */
int CDECL MSVCRT__iswlower_l( MSVCRT_wchar_t wc, MSVCRT__locale_t locale )
{
    return MSVCRT__iswctype_l( wc, MSVCRT__LOWER, locale );
}

/*********************************************************************
 *		iswlower (MSVCRT.@)
 */
INT CDECL MSVCRT_iswlower( MSVCRT_wchar_t wc )
{
    return MSVCRT__iswlower_l( wc, NULL );
}

/*********************************************************************
 *		_iswprint_l (MSVCRT.@)
 */
int CDECL MSVCRT__iswprint_l( MSVCRT_wchar_t wc, MSVCRT__locale_t locale )
{
    return MSVCRT__iswctype_l( wc, MSVCRT__ALPHA | MSVCRT__BLANK |
            MSVCRT__DIGIT | MSVCRT__PUNCT, locale );
}

/*********************************************************************
 *		iswprint (MSVCRT.@)
 */
INT CDECL MSVCRT_iswprint( MSVCRT_wchar_t wc )
{
    return MSVCRT__iswprint_l( wc, NULL );
}

/*********************************************************************
 *		_iswpunct_l (MSVCRT.@)
 */
INT CDECL MSVCRT__iswpunct_l( MSVCRT_wchar_t wc, MSVCRT__locale_t locale )
{
    return MSVCRT__iswctype_l( wc, MSVCRT__PUNCT, locale );
}

/*********************************************************************
 *		iswpunct (MSVCRT.@)
 */
INT CDECL MSVCRT_iswpunct( MSVCRT_wchar_t wc )
{
    return MSVCRT__iswpunct_l( wc, NULL );
}

/*********************************************************************
 *		_iswspace_l (MSVCRT.@)
 */
INT CDECL MSVCRT__iswspace_l( MSVCRT_wchar_t wc, MSVCRT__locale_t locale )
{
    return MSVCRT__iswctype_l( wc, MSVCRT__SPACE, locale );
}

/*********************************************************************
 *		iswspace (MSVCRT.@)
 */
INT CDECL MSVCRT_iswspace( MSVCRT_wchar_t wc )
{
    return MSVCRT__iswspace_l( wc, NULL );
}

/*********************************************************************
 *		_iswupper_l (MSVCRT.@)
 */
int CDECL MSVCRT__iswupper_l( MSVCRT_wchar_t wc, MSVCRT__locale_t locale )
{
    return MSVCRT__iswctype_l( wc, MSVCRT__UPPER, locale );
}

/*********************************************************************
 *		iswupper (MSVCRT.@)
 */
INT CDECL MSVCRT_iswupper( MSVCRT_wchar_t wc )
{
    return MSVCRT__iswupper_l( wc, NULL );
}

/*********************************************************************
 *		_iswxdigit_l (MSVCRT.@)
 */
int CDECL MSVCRT__iswxdigit_l( MSVCRT_wchar_t wc, MSVCRT__locale_t locale )
{
    return MSVCRT__iswctype_l( wc, MSVCRT__HEX, locale );
}

/*********************************************************************
 *		iswxdigit (MSVCRT.@)
 */
INT CDECL MSVCRT_iswxdigit( MSVCRT_wchar_t wc )
{
    return MSVCRT__iswxdigit_l( wc, NULL );
}

/*********************************************************************
 *		_iswblank_l (MSVCRT.@)
 */
INT CDECL MSVCRT__iswblank_l( MSVCRT_wchar_t wc, MSVCRT__locale_t locale )
{
    return wc == '\t' || MSVCRT__iswctype_l( wc, MSVCRT__BLANK, locale );
}

/*********************************************************************
 *		iswblank (MSVCRT.@)
 */
INT CDECL MSVCRT_iswblank( MSVCRT_wchar_t wc )
{
    return wc == '\t' || MSVCRT__iswctype_l( wc, MSVCRT__BLANK, NULL );
}

/*********************************************************************
 *		wcscpy_s (MSVCRT.@)
 */
INT CDECL MSVCRT_wcscpy_s( MSVCRT_wchar_t* wcDest, MSVCRT_size_t numElement, const  MSVCRT_wchar_t *wcSrc)
{
    MSVCRT_size_t size = 0;

    if(!MSVCRT_CHECK_PMT(wcDest)) return MSVCRT_EINVAL;
    if(!MSVCRT_CHECK_PMT(numElement)) return MSVCRT_EINVAL;

    if(!MSVCRT_CHECK_PMT(wcSrc))
    {
        wcDest[0] = 0;
        return MSVCRT_EINVAL;
    }

    size = MSVCRT_wcslen(wcSrc) + 1;

    if(!MSVCRT_CHECK_PMT_ERR(size <= numElement, MSVCRT_ERANGE))
    {
        wcDest[0] = 0;
        return MSVCRT_ERANGE;
    }

    memmove( wcDest, wcSrc, size*sizeof(WCHAR) );

    return 0;
}

/***********************************************************************
 *             wcscpy (MSVCRT.@)
 */
MSVCRT_wchar_t* __cdecl MSVCRT_wcscpy( MSVCRT_wchar_t *dst, const MSVCRT_wchar_t *src )
{
    WCHAR *p = dst;
    while ((*p++ = *src++));
    return dst;
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

/***********************************************************************
 *           wcscat (MSVCRT.@)
 */
MSVCRT_wchar_t* __cdecl MSVCRT_wcscat( MSVCRT_wchar_t *dst, const MSVCRT_wchar_t *src )
{
    MSVCRT_wcscpy( dst + MSVCRT_wcslen(dst), src );
    return dst;
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
        srclen = MSVCRT_wcslen(src);
        if (srclen >= (elem - dststart))
        {
            srclen = elem - dststart - 1;
            ret = MSVCRT_STRUNCATE;
        }
    }
    else
        srclen = min(MSVCRT_wcslen(src), count);
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
 * wctoint (INTERNAL)
 */
static int wctoint(WCHAR c, int base)
{
    int v = -1;
    if ('0' <= c && c <= '9')
        v = c - '0';
    else if ('A' <= c && c <= 'Z')
        v = c - 'A' + 10;
    else if ('a' <= c && c <= 'z')
        v = c - 'a' + 10;
    else {
        /* NOTE: wine_fold_string(MAP_FOLDDIGITS) supports too many things. */
        /* Unicode points that contain digits 0-9; keep this sorted! */
        static const WCHAR zeros[] = {
            0x660, 0x6f0, 0x966, 0x9e6, 0xa66, 0xae6, 0xb66, 0xc66, 0xce6,
            0xd66, 0xe50, 0xed0, 0xf20, 0x1040, 0x17e0, 0x1810, 0xff10
        };
        int i;
        for (i = 0; i < ARRAY_SIZE(zeros) && c >= zeros[i]; ++i) {
            if (zeros[i] <= c && c <= zeros[i] + 9) {
                v = c - zeros[i];
                break;
            }
        }
    }
    return v < base ? v : -1;
}

/*********************************************************************
 *  _wcstoi64_l (MSVCRT.@)
 *
 * FIXME: locale parameter is ignored
 */
__int64 CDECL MSVCRT__wcstoi64_l(const MSVCRT_wchar_t *nptr,
        MSVCRT_wchar_t **endptr, int base, MSVCRT__locale_t locale)
{
    BOOL negative = FALSE, empty = TRUE;
    __int64 ret = 0;

    TRACE("(%s %p %d %p)\n", debugstr_w(nptr), endptr, base, locale);

    if (!MSVCRT_CHECK_PMT(nptr != NULL)) return 0;
    if (!MSVCRT_CHECK_PMT(base == 0 || base >= 2)) return 0;
    if (!MSVCRT_CHECK_PMT(base <= 36)) return 0;

    if(endptr)
        *endptr = (MSVCRT_wchar_t*)nptr;

    while(MSVCRT__iswspace_l(*nptr, locale)) nptr++;

    if(*nptr == '-') {
        negative = TRUE;
        nptr++;
    } else if(*nptr == '+')
        nptr++;

    if((base==0 || base==16) && wctoint(*nptr, 1)==0 && (nptr[1]=='x' || nptr[1]=='X')) {
        base = 16;
        nptr += 2;
    }

    if(base == 0) {
        if(wctoint(*nptr, 1)==0)
            base = 8;
        else
            base = 10;
    }

    while(*nptr) {
        int v = wctoint(*nptr, base);
        if(v<0)
            break;

        if(negative)
            v = -v;

        nptr++;
        empty = FALSE;

        if(!negative && (ret>MSVCRT_I64_MAX/base || ret*base>MSVCRT_I64_MAX-v)) {
            ret = MSVCRT_I64_MAX;
            *MSVCRT__errno() = MSVCRT_ERANGE;
        } else if(negative && (ret<MSVCRT_I64_MIN/base || ret*base<MSVCRT_I64_MIN-v)) {
            ret = MSVCRT_I64_MIN;
            *MSVCRT__errno() = MSVCRT_ERANGE;
        } else
            ret = ret*base + v;
    }

    if(endptr && !empty)
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
 *  wcstol (MSVCRT.@)
 */
MSVCRT_long CDECL MSVCRT_wcstol(const MSVCRT_wchar_t *s,
        MSVCRT_wchar_t **end, int base)
{
    return MSVCRT__wcstol_l(s, end, base, NULL);
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

#if _MSVCR_VER>=120

/*********************************************************************
 *  _wtoll_l (MSVCR120.@)
 */
MSVCRT_longlong __cdecl MSVCRT__wtoll_l(const MSVCRT_wchar_t *str, MSVCRT__locale_t locale)
{
    return MSVCRT__wcstoi64_l(str, NULL, 10, locale);
}

/*********************************************************************
 *  _wtoll (MSVCR120.@)
 */
MSVCRT_longlong __cdecl MSVCRT__wtoll(const MSVCRT_wchar_t *str)
{
    return MSVCRT__wtoll_l(str, NULL);
}

#endif /* _MSVCR_VER>=120 */

/*********************************************************************
 *  _wcstoui64_l (MSVCRT.@)
 *
 * FIXME: locale parameter is ignored
 */
unsigned __int64 CDECL MSVCRT__wcstoui64_l(const MSVCRT_wchar_t *nptr,
        MSVCRT_wchar_t **endptr, int base, MSVCRT__locale_t locale)
{
    BOOL negative = FALSE, empty = TRUE;
    unsigned __int64 ret = 0;

    TRACE("(%s %p %d %p)\n", debugstr_w(nptr), endptr, base, locale);

    if (!MSVCRT_CHECK_PMT(nptr != NULL)) return 0;
    if (!MSVCRT_CHECK_PMT(base == 0 || base >= 2)) return 0;
    if (!MSVCRT_CHECK_PMT(base <= 36)) return 0;

    if(endptr)
        *endptr = (MSVCRT_wchar_t*)nptr;

    while(MSVCRT__iswspace_l(*nptr, locale)) nptr++;

    if(*nptr == '-') {
        negative = TRUE;
        nptr++;
    } else if(*nptr == '+')
        nptr++;

    if((base==0 || base==16) && wctoint(*nptr, 1)==0 && (nptr[1]=='x' || nptr[1]=='X')) {
        base = 16;
        nptr += 2;
    }

    if(base == 0) {
        if(wctoint(*nptr, 1)==0)
            base = 8;
        else
            base = 10;
    }

    while(*nptr) {
        int v = wctoint(*nptr, base);
        if(v<0)
            break;

        nptr++;
        empty = FALSE;

        if(ret>MSVCRT_UI64_MAX/base || ret*base>MSVCRT_UI64_MAX-v) {
            ret = MSVCRT_UI64_MAX;
            *MSVCRT__errno() = MSVCRT_ERANGE;
        } else
            ret = ret*base + v;
    }

    if(endptr && !empty)
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
    __int64 ret = MSVCRT__wcstoi64_l(s, end, base, locale);

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
    MSVCRT_pthreadlocinfo locinfo;
    MSVCRT_wchar_t ret;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!locinfo->lc_handle[MSVCRT_LC_CTYPE]) {
        if(c >= 'a' && c <= 'z')
            return c + 'A' - 'a';
        return c;
    }

    if(!LCMapStringW(locinfo->lc_handle[MSVCRT_LC_CTYPE], LCMAP_UPPERCASE, &c, 1, &ret, 1))
        return c;
    return ret;
}

/*********************************************************************
 *              towupper (MSVCRT.@)
 */
int CDECL MSVCRT_towupper(MSVCRT_wint_t c)
{
    return MSVCRT__towupper_l(c, NULL);
}

/*********************************************************************
 *              wcschr (MSVCRT.@)
 */
MSVCRT_wchar_t* CDECL MSVCRT_wcschr(const MSVCRT_wchar_t *str, MSVCRT_wchar_t ch)
{
    do { if (*str == ch) return (WCHAR *)(ULONG_PTR)str; } while (*str++);
    return NULL;
}

/*********************************************************************
 *              wcsrchr (MSVCRT.@)
 */
MSVCRT_wchar_t* CDECL MSVCRT_wcsrchr(const MSVCRT_wchar_t *str, MSVCRT_wchar_t ch)
{
    WCHAR *ret = NULL;
    do { if (*str == ch) ret = (WCHAR *)(ULONG_PTR)str; } while (*str++);
    return ret;
}

/***********************************************************************
 *              wcslen (MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT_wcslen(const MSVCRT_wchar_t *str)
{
    const MSVCRT_wchar_t *s = str;
    while (*s) s++;
    return s - str;
}

/*********************************************************************
 *              wcsstr (MSVCRT.@)
 */
MSVCRT_wchar_t* CDECL MSVCRT_wcsstr(const MSVCRT_wchar_t *str, const MSVCRT_wchar_t *sub)
{
    while(*str)
    {
        const MSVCRT_wchar_t *p1 = str, *p2 = sub;
        while(*p1 && *p2 && *p1 == *p2)
        {
            p1++;
            p2++;
        }
        if(!*p2)
            return (MSVCRT_wchar_t*)str;
        str++;
    }
    return NULL;
}

/*********************************************************************
 *              _wtoi64_l (MSVCRT.@)
 */
__int64 CDECL MSVCRT__wtoi64_l(const MSVCRT_wchar_t *str, MSVCRT__locale_t locale)
{
    ULONGLONG RunningTotal = 0;
    BOOL bMinus = FALSE;

    while (MSVCRT__iswspace_l(*str, locale)) {
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
 *              _wcsxfrm_l (MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT__wcsxfrm_l(MSVCRT_wchar_t *dest, const MSVCRT_wchar_t *src,
        MSVCRT_size_t len, MSVCRT__locale_t locale)
{
    MSVCRT_pthreadlocinfo locinfo;
    int i, ret;

    if(!MSVCRT_CHECK_PMT(src)) return INT_MAX;
    if(!MSVCRT_CHECK_PMT(dest || !len)) return INT_MAX;

    if(len > INT_MAX) {
        FIXME("len > INT_MAX not supported\n");
        len = INT_MAX;
    }

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!locinfo->lc_handle[MSVCRT_LC_COLLATE]) {
        MSVCRT_wcsncpy(dest, src, len);
        return MSVCRT_wcslen(src);
    }

    ret = LCMapStringW(locinfo->lc_handle[MSVCRT_LC_COLLATE],
            LCMAP_SORTKEY, src, -1, NULL, 0);
    if(!ret) {
        if(len) dest[0] = 0;
        *MSVCRT__errno() = MSVCRT_EILSEQ;
        return INT_MAX;
    }
    if(!len) return ret-1;

    if(ret > len) {
        dest[0] = 0;
        *MSVCRT__errno() = MSVCRT_ERANGE;
        return ret-1;
    }

    ret = LCMapStringW(locinfo->lc_handle[MSVCRT_LC_COLLATE],
            LCMAP_SORTKEY, src, -1, dest, len) - 1;
    for(i=ret; i>=0; i--)
        dest[i] = ((unsigned char*)dest)[i];
    return ret;
}

/*********************************************************************
 *              wcsxfrm (MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT_wcsxfrm(MSVCRT_wchar_t *dest,
        const MSVCRT_wchar_t *src, MSVCRT_size_t len)
{
    return MSVCRT__wcsxfrm_l(dest, src, len, NULL);
}
