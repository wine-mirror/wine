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
    MSVCRT_pthreadlocinfo locinfo;
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

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!locinfo->lc_handle[MSVCRT_LC_CTYPE])
    {
        while (*str)
        {
            if (*str >= 'A' && *str <= 'Z')
                *str -= 'A' - 'a';
            str++;
        }
    }
    else
    {
        while (*str)
        {
            *str = MSVCRT__tolower_l((unsigned char)*str, locale);
            str++;
        }
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
    MSVCRT_pthreadlocinfo locinfo;
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

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!locinfo->lc_handle[MSVCRT_LC_CTYPE])
    {
        while (*str)
        {
            if (*str >= 'a' && *str <= 'z')
                *str -= 'a' - 'A';
            str++;
        }
    }
    else
    {
        while (*str)
        {
            *str = MSVCRT__toupper_l((unsigned char)*str, locale);
            str++;
        }
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
 *              _strnset_s (MSVCRT.@)
 */
int CDECL MSVCRT__strnset_s(char *str, MSVCRT_size_t size, int c, MSVCRT_size_t count)
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
    {
        *ctx = str;
        return NULL;
    }

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

enum round {
    ROUND_ZERO, /* only used when dropped part contains only zeros */
    ROUND_DOWN,
    ROUND_EVEN,
    ROUND_UP
};

static double make_double(int sign, int exp, ULONGLONG m, enum round round, int *err)
{
#define EXP_BITS 11
#define MANT_BITS 53
    ULONGLONG bits = 0;

    TRACE("%c %s *2^%d (round %d)\n", sign == -1 ? '-' : '+', wine_dbgstr_longlong(m), exp, round);
    if (!m) return sign * 0.0;

    /* make sure that we don't overflow modifying exponent */
    if (exp > 1<<EXP_BITS)
    {
        if (err) *err = MSVCRT_ERANGE;
        return sign * INFINITY;
    }
    if (exp < -(1<<EXP_BITS))
    {
        if (err) *err = MSVCRT_ERANGE;
        return sign * 0.0;
    }
    exp += MANT_BITS - 1;

    /* normalize mantissa */
    while(m < (ULONGLONG)1 << (MANT_BITS-1))
    {
        m <<= 1;
        exp--;
    }
    while(m >= (ULONGLONG)1 << MANT_BITS)
    {
        if (m & 1 || round != ROUND_ZERO)
        {
            if (!(m & 1)) round = ROUND_DOWN;
            else if(round == ROUND_ZERO) round = ROUND_EVEN;
            else round = ROUND_UP;
        }
        m >>= 1;
        exp++;
    }

    /* handle subnormal that falls into regular range due to rounding */
    exp += (1 << (EXP_BITS-1)) - 1;
    if (!exp && (round == ROUND_UP || (round == ROUND_EVEN && m & 1)))
    {
        if (m + 1 >= (ULONGLONG)1 << MANT_BITS)
        {
            m++;
            m >>= 1;
            exp++;
            round = ROUND_DOWN;
        }
    }

    /* handle subnormals */
    if (exp <= 0)
        m >>= 1;
    while(m && exp<0)
    {
        m >>= 1;
        exp++;
    }

    /* round mantissa */
    if (round == ROUND_UP || (round == ROUND_EVEN && m & 1))
    {
        m++;
        if (m >= (ULONGLONG)1 << MANT_BITS)
        {
            exp++;
            m >>= 1;
        }
    }

    if (exp >= 1<<EXP_BITS)
    {
        if (err) *err = MSVCRT_ERANGE;
        return sign * INFINITY;
    }
    if (!m || exp < 0)
    {
        if (err) *err = MSVCRT_ERANGE;
        return sign * 0.0;
    }

    if (sign == -1) bits |= (ULONGLONG)1 << (MANT_BITS + EXP_BITS - 1);
    bits |= (ULONGLONG)exp << (MANT_BITS - 1);
    bits |= m & (((ULONGLONG)1 << (MANT_BITS - 1)) - 1);

    TRACE("returning %s\n", wine_dbgstr_longlong(bits));
    return *((double*)&bits);
}

#if _MSVCR_VER >= 140

static inline int hex2int(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    else if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

static double strtod16(MSVCRT_wchar_t get(void *ctx), void unget(void *ctx),
        void *ctx, int sign, MSVCRT_pthreadlocinfo locinfo, int *err)
{
    BOOL found_digit = FALSE, found_dp = FALSE;
    enum round round = ROUND_ZERO;
    MSVCRT_wchar_t nch;
    ULONGLONG m = 0;
    int val, exp = 0;

    nch = get(ctx);
    while(m < MSVCRT_UI64_MAX/16)
    {
        val = hex2int(nch);
        if (val == -1) break;
        found_digit = TRUE;
        nch = get(ctx);

        m = m*16 + val;
    }
    while(1)
    {
        val = hex2int(nch);
        if (val == -1) break;
        nch = get(ctx);
        exp += 4;

        if (val || round != ROUND_ZERO)
        {
            if (val < 8) round = ROUND_DOWN;
            else if (val == 8 && round == ROUND_ZERO) round = ROUND_EVEN;
            else round = ROUND_UP;
        }
    }

    if(nch == *locinfo->lconv->decimal_point)
    {
        found_dp = TRUE;
        nch = get(ctx);
    }
    else if (!found_digit)
    {
        if(nch!=MSVCRT_WEOF) unget(ctx);
        unget(ctx);
        return 0.0;
    }

    while(m <= MSVCRT_UI64_MAX/16)
    {
        val = hex2int(nch);
        if (val == -1) break;
        found_digit = TRUE;
        nch = get(ctx);

        m = m*16 + val;
        exp -= 4;
    }
    while(1)
    {
        val = hex2int(nch);
        if (val == -1) break;
        nch = get(ctx);

        if (val || round != ROUND_ZERO)
        {
            if (val < 8) round = ROUND_DOWN;
            else if (val == 8 && round == ROUND_ZERO) round = ROUND_EVEN;
            else round = ROUND_UP;
        }
    }

    if (!found_digit)
    {
        if (nch != MSVCRT_WEOF) unget(ctx);
        if (found_dp) unget(ctx);
        unget(ctx);
        return 0.0;
    }

    if(nch=='p' || nch=='P') {
        BOOL found_sign = FALSE;
        int e=0, s=1;

        nch = get(ctx);
        if(nch == '-') {
            found_sign = TRUE;
            s = -1;
            nch = get(ctx);
        } else if(nch == '+') {
            found_sign = TRUE;
            nch = get(ctx);
        }
        if(nch>='0' && nch<='9') {
            while(nch>='0' && nch<='9') {
                if(e>INT_MAX/10 || (e=e*10+nch-'0')<0)
                    e = INT_MAX;
                nch = get(ctx);
            }
            if((nch!=MSVCRT_WEOF) && (nch < '0' || nch > '9')) unget(ctx);
            e *= s;

            if(exp<0 && e<0 && exp+e>=0) exp = INT_MIN;
            else if(exp>0 && e>0 && exp+e<0) exp = INT_MAX;
            else exp += e;
        } else {
            if(nch != MSVCRT_WEOF) unget(ctx);
            if(found_sign) unget(ctx);
            unget(ctx);
        }
    }

    return make_double(sign, exp, m, round, err);
}
#endif

struct u128 {
   ULONGLONG u[2];
};

static inline struct u128 u128_lshift(struct u128 *u)
{
    struct u128 r;

    r.u[0] = u->u[0] << 1;
    r.u[1] = (u->u[1] << 1) + (u->u[0] >> (sizeof(u->u[0])*8-1));
    return r;
}

static inline struct u128 u128_rshift(struct u128 *u)
{
    struct u128 r;

    r.u[0] = (u->u[0] >> 1) + ((u->u[1] & 1) << (sizeof(u->u[1])*8-1));
    r.u[1] = u->u[1] >> 1;
    return r;
}

static inline void u128_mul10(struct u128 *u)
{
    struct u128 tmp;

    tmp = u128_lshift(u);
    *u = u128_lshift(&tmp);
    *u = u128_lshift(u);

    u->u[0] += tmp.u[0];
    u->u[1] += tmp.u[1];
    if (u->u[0] < tmp.u[0]) u->u[1]++;
}

static inline void u128_div10(struct u128 *u)
{
    ULONGLONG h, l, r;

    r = u->u[1] % 10;
    u->u[1] /= 10;

    h = (r << 32) + (u->u[0] >> 32);
    r = h % 10;
    h /= 10;

    l = (r << 32) + (u->u[0] & 0xffffffff);
    l /= 10;
    u->u[0] = (h << 32) + l;
}

static double convert_e10_to_e2(int sign, int e10, ULONGLONG m, int *err)
{
    int e2 = 0;
    struct u128 u128;

    u128.u[0] = m;
    u128.u[1] = 0;

    while(e10 > 0)
    {
        u128_mul10(&u128);
        e10--;

        while(u128.u[1] > MSVCRT_UI64_MAX/16)
        {
            u128 = u128_rshift(&u128);
            e2++;
        }
    }

    while(e10 < 0)
    {
        while(!(u128.u[1] & (1 << (sizeof(u128.u[1])-1))))
        {
            u128 = u128_lshift(&u128);
            e2--;
        }

        u128_div10(&u128);
        e10++;
    }

    while(u128.u[1])
    {
        u128 = u128_rshift(&u128);
        e2++;
    }

    return make_double(sign, e2, u128.u[0], ROUND_DOWN, err);
}

double parse_double(MSVCRT_wchar_t (*get)(void *ctx), void (*unget)(void *ctx),
        void *ctx, MSVCRT_pthreadlocinfo locinfo, int *err)
{
#if _MSVCR_VER >= 140
    MSVCRT_wchar_t _infinity[] = { 'i', 'n', 'f', 'i', 'n', 'i', 't', 'y', 0 };
    MSVCRT_wchar_t _nan[] = { 'n', 'a', 'n', 0 };
    MSVCRT_wchar_t *str_match = NULL;
    int matched=0;
#endif
    BOOL found_digit = FALSE, found_dp = FALSE, found_sign = FALSE;
    unsigned __int64 d=0, hlp;
    MSVCRT_wchar_t nch;
    int exp=0, sign=1;

    nch = get(ctx);
    if(nch == '-') {
        found_sign = TRUE;
        sign = -1;
        nch = get(ctx);
    } else if(nch == '+') {
        found_sign = TRUE;
        nch = get(ctx);
    }

#if _MSVCR_VER >= 140
    if(nch == _infinity[0] || nch == MSVCRT__toupper(_infinity[0]))
        str_match = _infinity;
    if(nch == _nan[0] || nch == MSVCRT__toupper(_nan[0]))
        str_match = _nan;
    while(str_match && nch != MSVCRT_WEOF &&
            (nch == str_match[matched] || nch == MSVCRT__toupper(str_match[matched]))) {
        nch = get(ctx);
        matched++;
    }
    if(str_match) {
        int keep = 0;
        if(matched >= 8) keep = 8;
        else if(matched >= 3) keep = 3;
        if(nch != MSVCRT_WEOF) unget(ctx);
        for (; matched > keep; matched--) {
            unget(ctx);
        }
        if(keep) {
            if (str_match == _infinity) return sign*INFINITY;
            if (str_match == _nan) return sign*NAN;
        } else if(found_sign) {
            unget(ctx);
        }

        return make_double(sign, exp, d, ROUND_ZERO, err);
    }

    if(nch == '0') {
        found_digit = TRUE;
        nch = get(ctx);
        if(nch == 'x' || nch == 'X')
            return strtod16(get, unget, ctx, sign, locinfo, err);
    }
#endif

    while(nch>='0' && nch<='9') {
        found_digit = TRUE;
        hlp = d * 10 + nch - '0';
        nch = get(ctx);
        if(d>MSVCRT_UI64_MAX/10 || hlp<d) {
            exp++;
            break;
        } else
            d = hlp;
    }
    while(nch>='0' && nch<='9') {
        exp++;
        nch = get(ctx);
    }

    if(nch == *locinfo->lconv->decimal_point) {
        found_dp = TRUE;
        nch = get(ctx);
    }

    while(nch>='0' && nch<='9') {
        found_digit = TRUE;
        hlp = d * 10 + nch - '0';
        nch = get(ctx);
        if(d>MSVCRT_UI64_MAX/10 || hlp<d)
            break;
        d = hlp;
        exp--;
    }
    while(nch>='0' && nch<='9')
        nch = get(ctx);

    if(!found_digit) {
        if(nch != MSVCRT_WEOF) unget(ctx);
        if(found_dp) unget(ctx);
        if(found_sign) unget(ctx);
        return 0.0;
    }

    if(nch=='e' || nch=='E' || nch=='d' || nch=='D') {
        int e=0, s=1;

        nch = get(ctx);
        if(nch == '-') {
            found_sign = TRUE;
            s = -1;
            nch = get(ctx);
        } else if(nch == '+') {
            found_sign = TRUE;
            nch = get(ctx);
        } else {
            found_sign = FALSE;
        }

        if(nch>='0' && nch<='9') {
            while(nch>='0' && nch<='9') {
                if(e>INT_MAX/10 || (e=e*10+nch-'0')<0)
                    e = INT_MAX;
                nch = get(ctx);
            }
            if(nch != MSVCRT_WEOF) unget(ctx);
            e *= s;

            if(exp<0 && e<0 && exp+e>=0) exp = INT_MIN;
            else if(exp>0 && e>0 && exp+e<0) exp = INT_MAX;
            else exp += e;
        } else {
            if(nch != MSVCRT_WEOF) unget(ctx);
            if(found_sign) unget(ctx);
            unget(ctx);
        }
    } else if(nch != MSVCRT_WEOF) {
        unget(ctx);
    }

    if(!err) err = MSVCRT__errno();
    if(!d) return make_double(sign, exp, d, ROUND_ZERO, err);
    if(exp > MSVCRT_DBL_MAX_10_EXP)
        return make_double(sign, INT_MAX, d, ROUND_ZERO, err);
    /* Count part of exponent stored in denormalized mantissa. */
    /* Increase exponent range to handle subnormals. */
    if(exp < MSVCRT_DBL_MIN_10_EXP-MSVCRT_DBL_DIG-18)
        return make_double(sign, INT_MIN, d, ROUND_ZERO, err);

    return convert_e10_to_e2(sign, exp, d, err);
}

static MSVCRT_wchar_t strtod_str_get(void *ctx)
{
    const char **p = ctx;
    if (!**p) return MSVCRT_WEOF;
    return *(*p)++;
}

static void strtod_str_unget(void *ctx)
{
    const char **p = ctx;
    (*p)--;
}

static inline double strtod_helper(const char *str, char **end, MSVCRT__locale_t locale, int *err)
{
    MSVCRT_pthreadlocinfo locinfo;
    const char *beg, *p;
    double ret;

    if (err) *err = 0;
    if (!MSVCRT_CHECK_PMT(str != NULL)) {
        if (end) *end = NULL;
        return 0;
    }

    if (!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    p = str;
    while(MSVCRT__isspace_l((unsigned char)*p, locale))
        p++;
    beg = p;

    ret = parse_double(strtod_str_get, strtod_str_unget, &p, locinfo, err);
    if (end) *end = (p == beg ? (char*)str : (char*)p);
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

#if _MSVCR_VER>=120

/*********************************************************************
 *		strtof_l  (MSVCR120.@)
 */
float CDECL MSVCRT__strtof_l( const char *str, char **end, MSVCRT__locale_t locale )
{
    return MSVCRT_strtod_l(str, end, locale);
}

/*********************************************************************
 *		strtof  (MSVCR120.@)
 */
float CDECL MSVCRT_strtof( const char *str, char **end )
{
    return MSVCRT__strtof_l(str, end, NULL);
}

#endif /* _MSVCR_VER>=120 */

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
 * _atoflt  (MSVCR100.@)
 */
int CDECL MSVCRT__atoflt(MSVCRT__CRT_FLOAT *value, char *str)
{
    return MSVCRT__atoflt_l(value, str, NULL);
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
        return MSVCRT_strcmp(str1, str2);
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
        return MSVCRT__stricmp(str1, str2);
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
        return MSVCRT_strncmp(str1, str2, count);
    return CompareStringA(locinfo->lc_handle[MSVCRT_LC_COLLATE], 0,
              str1, MSVCRT_strnlen(str1, count),
              str2, MSVCRT_strnlen(str2, count))-CSTR_EQUAL;
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
        return MSVCRT__strnicmp(str1, str2, count);
    return CompareStringA(locinfo->lc_handle[MSVCRT_LC_COLLATE], NORM_IGNORECASE,
            str1, MSVCRT_strnlen(str1, count),
            str2, MSVCRT_strnlen(str2, count))-CSTR_EQUAL;
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
 *      strcpy (MSVCRT.@)
 */
char* CDECL MSVCRT_strcpy(char *dst, const char *src)
{
    char *ret = dst;
    while ((*dst++ = *src++));
    return ret;
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
 *		_strxfrm_l (MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT__strxfrm_l( char *dest, const char *src,
        MSVCRT_size_t len, MSVCRT__locale_t locale )
{
    MSVCRT_pthreadlocinfo locinfo;
    int ret;

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
        MSVCRT_strncpy(dest, src, len);
        return strlen(src);
    }

    ret = LCMapStringA(locinfo->lc_handle[MSVCRT_LC_COLLATE],
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

    return LCMapStringA(locinfo->lc_handle[MSVCRT_LC_COLLATE],
            LCMAP_SORTKEY, src, -1, dest, len) - 1;
}

/*********************************************************************
 *		strxfrm (MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT_strxfrm( char *dest, const char *src, MSVCRT_size_t len )
{
    return MSVCRT__strxfrm_l(dest, src, len, NULL);
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
    const char *p = nptr;
    BOOL negative = FALSE;
    BOOL got_digit = FALSE;
    __int64 ret = 0;

    TRACE("(%s %p %d %p)\n", debugstr_a(nptr), endptr, base, locale);

    if (!MSVCRT_CHECK_PMT(nptr != NULL)) return 0;
    if (!MSVCRT_CHECK_PMT(base == 0 || base >= 2)) return 0;
    if (!MSVCRT_CHECK_PMT(base <= 36)) return 0;

    while(MSVCRT__isspace_l((unsigned char)*nptr, locale)) nptr++;

    if(*nptr == '-') {
        negative = TRUE;
        nptr++;
    } else if(*nptr == '+')
        nptr++;

    if((base==0 || base==16) && *nptr=='0' && MSVCRT__tolower_l(*(nptr+1), locale)=='x') {
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
        char cur = MSVCRT__tolower_l(*nptr, locale);
        int v;

        if(cur>='0' && cur<='9') {
            if(cur >= '0'+base)
                break;
            v = cur-'0';
        } else {
            if(cur<'a' || cur>='a'+base-10)
                break;
            v = cur-'a'+10;
        }
        got_digit = TRUE;

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
        *endptr = (char*)(got_digit ? nptr : p);

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
#if _MSVCR_VER == 0
int __cdecl MSVCRT_atoi(const char *str)
{
    BOOL minus = FALSE;
    int ret = 0;

    if(!str)
        return 0;

    while(MSVCRT__isspace_l((unsigned char)*str, NULL)) str++;

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
#else
int CDECL MSVCRT_atoi(const char *str)
{
    return MSVCRT__atoi_l(str, NULL);
}
#endif

/******************************************************************
 *      _atoi64_l (MSVCRT.@)
 */
__int64 CDECL MSVCRT__atoi64_l(const char *str, MSVCRT__locale_t locale)
{
    return MSVCRT_strtoi64_l(str, NULL, 10, locale);
}

/******************************************************************
 *      _atoi64 (MSVCRT.@)
 */
__int64 CDECL MSVCRT__atoi64(const char *str)
{
    return MSVCRT_strtoi64_l(str, NULL, 10, NULL);
}

/******************************************************************
 *      _atol_l (MSVCRT.@)
 */
MSVCRT_long CDECL MSVCRT__atol_l(const char *str, MSVCRT__locale_t locale)
{
    __int64 ret = MSVCRT_strtoi64_l(str, NULL, 10, locale);

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
 *      atol (MSVCRT.@)
 */
MSVCRT_long CDECL MSVCRT_atol(const char *str)
{
#if _MSVCR_VER == 0
    return MSVCRT_atoi(str);
#else
    return MSVCRT__atol_l(str, NULL);
#endif
}

#if _MSVCR_VER>=120

/******************************************************************
 *      _atoll_l (MSVCR120.@)
 */
MSVCRT_longlong CDECL MSVCRT__atoll_l(const char* str, MSVCRT__locale_t locale)
{
    return MSVCRT_strtoi64_l(str, NULL, 10, locale);
}

/******************************************************************
 *      atoll (MSVCR120.@)
 */
MSVCRT_longlong CDECL MSVCRT_atoll(const char* str)
{
    return MSVCRT__atoll_l(str, NULL);
}

#endif /* if _MSVCR_VER>=120 */

/******************************************************************
 *		_strtol_l (MSVCRT.@)
 */
MSVCRT_long CDECL MSVCRT__strtol_l(const char* nptr,
        char** end, int base, MSVCRT__locale_t locale)
{
    __int64 ret = MSVCRT_strtoi64_l(nptr, end, base, locale);

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
 *		strtol (MSVCRT.@)
 */
MSVCRT_long CDECL MSVCRT_strtol(const char* nptr, char** end, int base)
{
    return MSVCRT__strtol_l(nptr, end, base, NULL);
}

/******************************************************************
 *		_strtoul_l (MSVCRT.@)
 */
MSVCRT_ulong CDECL MSVCRT_strtoul_l(const char* nptr, char** end, int base, MSVCRT__locale_t locale)
{
    __int64 ret = MSVCRT_strtoi64_l(nptr, end, base, locale);

    if(ret > MSVCRT_ULONG_MAX) {
        ret = MSVCRT_ULONG_MAX;
        *MSVCRT__errno() = MSVCRT_ERANGE;
    }else if(ret < -(__int64)MSVCRT_ULONG_MAX) {
        ret = 1;
        *MSVCRT__errno() = MSVCRT_ERANGE;
    }

    return ret;
}

/******************************************************************
 *		strtoul (MSVCRT.@)
 */
MSVCRT_ulong CDECL MSVCRT_strtoul(const char* nptr, char** end, int base)
{
    return MSVCRT_strtoul_l(nptr, end, base, NULL);
}

/*********************************************************************
 *  _strtoui64_l (MSVCRT.@)
 *
 * FIXME: locale parameter is ignored
 */
unsigned __int64 CDECL MSVCRT_strtoui64_l(const char *nptr, char **endptr, int base, MSVCRT__locale_t locale)
{
    const char *p = nptr;
    BOOL negative = FALSE;
    BOOL got_digit = FALSE;
    unsigned __int64 ret = 0;

    TRACE("(%s %p %d %p)\n", debugstr_a(nptr), endptr, base, locale);

    if (!MSVCRT_CHECK_PMT(nptr != NULL)) return 0;
    if (!MSVCRT_CHECK_PMT(base == 0 || base >= 2)) return 0;
    if (!MSVCRT_CHECK_PMT(base <= 36)) return 0;

    while(MSVCRT__isspace_l((unsigned char)*nptr, locale)) nptr++;

    if(*nptr == '-') {
        negative = TRUE;
        nptr++;
    } else if(*nptr == '+')
        nptr++;

    if((base==0 || base==16) && *nptr=='0' && MSVCRT__tolower_l(*(nptr+1), locale)=='x') {
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
        char cur = MSVCRT__tolower_l(*nptr, locale);
        int v;

        if(cur>='0' && cur<='9') {
            if(cur >= '0'+base)
                break;
            v = *nptr-'0';
        } else {
            if(cur<'a' || cur>='a'+base-10)
                break;
            v = cur-'a'+10;
        }
        got_digit = TRUE;

        nptr++;

        if(ret>MSVCRT_UI64_MAX/base || ret*base>MSVCRT_UI64_MAX-v) {
            ret = MSVCRT_UI64_MAX;
            *MSVCRT__errno() = MSVCRT_ERANGE;
        } else
            ret = ret*base + v;
    }

    if(endptr)
        *endptr = (char*)(got_digit ? nptr : p);

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
    while (*str1 && *str1 == *str2) { str1++; str2++; }
    if (*str1 > *str2) return 1;
    if (*str1 < *str2) return -1;
    return 0;
}

/*********************************************************************
 *                  strncmp   (MSVCRT.@)
 */
int __cdecl MSVCRT_strncmp(const char *str1, const char *str2, MSVCRT_size_t len)
{
    if (!len) return 0;
    while (--len && *str1 && *str1 == *str2) { str1++; str2++; }
    if (*str1 > *str2) return 1;
    if (*str1 < *str2) return -1;
    return 0;
}

/*********************************************************************
 *                  _strnicmp_l   (MSVCRT.@)
 */
int __cdecl MSVCRT__strnicmp_l(const char *s1, const char *s2,
        MSVCRT_size_t count, MSVCRT__locale_t locale)
{
    int c1, c2;

    if(s1==NULL || s2==NULL)
        return MSVCRT__NLSCMPERROR;

    if(!count)
        return 0;

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
    MSVCRT_size_t i, j, len, needle_len, lps_len;
    BYTE lps[256];

    needle_len = MSVCRT_strlen(needle);
    if (!needle_len) return (char*)haystack;
    lps_len = needle_len > ARRAY_SIZE(lps) ? ARRAY_SIZE(lps) : needle_len;

    lps[0] = 0;
    len = 0;
    i = 1;
    while (i < lps_len)
    {
        if (needle[i] == needle[len]) lps[i++] = ++len;
        else if (len) len = lps[len-1];
        else lps[i++] = 0;
    }

    i = j = 0;
    while (haystack[i])
    {
        while (j < lps_len && haystack[i] && haystack[i] == needle[j])
        {
            i++;
            j++;
        }

        if (j == needle_len) return (char*)haystack + i - j;
        else if (j)
        {
            if (j == ARRAY_SIZE(lps) && !MSVCRT_strncmp(haystack + i, needle + j, needle_len - j))
                return (char*)haystack + i - j;
            j = lps[j-1];
        }
        else if (haystack[i]) i++;
    }
    return NULL;
}

/*********************************************************************
 *                  _memicmp_l   (MSVCRT.@)
 */
int __cdecl MSVCRT__memicmp_l(const char *s1, const char *s2, MSVCRT_size_t len, MSVCRT__locale_t locale)
{
    int ret = 0;

#if _MSVCR_VER == 0 || _MSVCR_VER >= 80
    if (!s1 || !s2)
    {
        if (len)
            MSVCRT_INVALID_PMT(NULL, EINVAL);
        return len ? MSVCRT__NLSCMPERROR : 0;
    }
#endif

    while (len--)
    {
        if ((ret = MSVCRT__tolower_l(*s1, locale) - MSVCRT__tolower_l(*s2, locale)))
            break;
        s1++;
        s2++;
    }
    return ret;
}

/*********************************************************************
 *                  _memicmp   (MSVCRT.@)
 */
int __cdecl MSVCRT__memicmp(const char *s1, const char *s2, MSVCRT_size_t len)
{
    return MSVCRT__memicmp_l(s1, s2, len, NULL);
}

/*********************************************************************
 *                  strcspn   (MSVCRT.@)
 */
MSVCRT_size_t __cdecl MSVCRT_strcspn(const char *str, const char *reject)
{
    BOOL rejects[256];
    const char *p;

    memset(rejects, 0, sizeof(rejects));

    p = reject;
    while(*p)
    {
        rejects[(unsigned char)*p] = TRUE;
        p++;
    }

    p = str;
    while(*p && !rejects[(unsigned char)*p]) p++;
    return p - str;
}

/*********************************************************************
 *                  strpbrk   (MSVCRT.@)
 */
char* __cdecl MSVCRT_strpbrk(const char *str, const char *accept)
{
    return strpbrk(str, accept);
}

/*********************************************************************
 *                  __strncnt   (MSVCRT.@)
 */
MSVCRT_size_t __cdecl MSVCRT___strncnt(const char *str, MSVCRT_size_t size)
{
    MSVCRT_size_t ret = 0;

#if _MSVCR_VER >= 140
    while (*str++ && size--)
#else
    while (size-- && *str++)
#endif
    {
        ret++;
    }

    return ret;
}


#ifdef _CRTDLL
/*********************************************************************
 *		_strdec (CRTDLL.@)
 */
char * CDECL _strdec(const char *str1, const char *str2)
{
    return (char *)(str2 - 1);
}

/*********************************************************************
 *		_strinc (CRTDLL.@)
 */
char * CDECL _strinc(const char *str)
{
    return (char *)(str + 1);
}

/*********************************************************************
 *		_strnextc (CRTDLL.@)
 */
unsigned int CDECL _strnextc(const char *str)
{
    return (unsigned char)str[0];
}

/*********************************************************************
 *		_strninc (CRTDLL.@)
 */
char * CDECL _strninc(const char *str, size_t len)
{
    return (char *)(str + len);
}

/*********************************************************************
 *		_strspnp (CRTDLL.@)
 */
char * CDECL _strspnp( const char *str1, const char *str2)
{
    str1 += strspn( str1, str2 );
    return *str1 ? (char*)str1 : NULL;
}
#endif
