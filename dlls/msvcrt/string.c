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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <errno.h>
#include "msvcrt.h"
#include "bnum.h"
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

static struct fpnum fpnum(int sign, int exp, ULONGLONG m, enum fpmod mod)
{
    struct fpnum ret;

    ret.sign = sign;
    ret.exp = exp;
    ret.m = m;
    ret.mod = mod;
    return ret;
}

int fpnum_double(struct fpnum *fp, double *d)
{
    ULONGLONG bits = 0;

    if (fp->mod == FP_VAL_INFINITY)
    {
        *d = fp->sign * INFINITY;
        return 0;
    }

    if (fp->mod == FP_VAL_NAN)
    {
        bits = ~0;
        if (fp->sign == 1)
            bits &= ~((ULONGLONG)1 << (MANT_BITS + EXP_BITS - 1));
        *d = *(double*)&bits;
        return 0;
    }

    TRACE("%c %s *2^%d (round %d)\n", fp->sign == -1 ? '-' : '+',
            wine_dbgstr_longlong(fp->m), fp->exp, fp->mod);
    if (!fp->m)
    {
        *d = fp->sign * 0.0;
        return 0;
    }

    /* make sure that we don't overflow modifying exponent */
    if (fp->exp > 1<<EXP_BITS)
    {
        *d = fp->sign * INFINITY;
        return MSVCRT_ERANGE;
    }
    if (fp->exp < -(1<<EXP_BITS))
    {
        *d = fp->sign * 0.0;
        return MSVCRT_ERANGE;
    }
    fp->exp += MANT_BITS - 1;

    /* normalize mantissa */
    while(fp->m < (ULONGLONG)1 << (MANT_BITS-1))
    {
        fp->m <<= 1;
        fp->exp--;
    }
    while(fp->m >= (ULONGLONG)1 << MANT_BITS)
    {
        if (fp->m & 1 || fp->mod != FP_ROUND_ZERO)
        {
            if (!(fp->m & 1)) fp->mod = FP_ROUND_DOWN;
            else if(fp->mod == FP_ROUND_ZERO) fp->mod = FP_ROUND_EVEN;
            else fp->mod = FP_ROUND_UP;
        }
        fp->m >>= 1;
        fp->exp++;
    }
    fp->exp += (1 << (EXP_BITS-1)) - 1;

    /* handle subnormals */
    if (fp->exp <= 0)
    {
        if (fp->m & 1 && fp->mod == FP_ROUND_ZERO) fp->mod = FP_ROUND_EVEN;
        else if (fp->m & 1) fp->mod = FP_ROUND_UP;
        else if (fp->mod != FP_ROUND_ZERO) fp->mod = FP_ROUND_DOWN;
        fp->m >>= 1;
    }
    while(fp->m && fp->exp<0)
    {
        if (fp->m & 1 && fp->mod == FP_ROUND_ZERO) fp->mod = FP_ROUND_EVEN;
        else if (fp->m & 1) fp->mod = FP_ROUND_UP;
        else if (fp->mod != FP_ROUND_ZERO) fp->mod = FP_ROUND_DOWN;
        fp->m >>= 1;
        fp->exp++;
    }

    /* round mantissa */
    if (fp->mod == FP_ROUND_UP || (fp->mod == FP_ROUND_EVEN && fp->m & 1))
    {
        fp->m++;

        /* handle subnormal that falls into regular range due to rounding */
        if (fp->m == (ULONGLONG)1 << (MANT_BITS - 1))
        {
            fp->exp++;
        }
        else if (fp->m >= (ULONGLONG)1 << MANT_BITS)
        {
            fp->exp++;
            fp->m >>= 1;
        }
    }

    if (fp->exp >= (1<<EXP_BITS)-1)
    {
        *d = fp->sign * INFINITY;
        return MSVCRT_ERANGE;
    }
    if (!fp->m || fp->exp < 0)
    {
        *d = fp->sign * 0.0;
        return MSVCRT_ERANGE;
    }

    if (fp->sign == -1)
        bits |= (ULONGLONG)1 << (MANT_BITS + EXP_BITS - 1);
    bits |= (ULONGLONG)fp->exp << (MANT_BITS - 1);
    bits |= fp->m & (((ULONGLONG)1 << (MANT_BITS - 1)) - 1);

    TRACE("returning %s\n", wine_dbgstr_longlong(bits));
    *d = *(double*)&bits;
    return 0;
}

#define LDBL_EXP_BITS 15
#define LDBL_MANT_BITS 64
int fpnum_ldouble(struct fpnum *fp, MSVCRT__LDOUBLE *d)
{
    if (fp->mod == FP_VAL_INFINITY)
    {
        d->x80[0] = 0;
        d->x80[1] = 0x80000000;
        d->x80[2] = (1 << LDBL_EXP_BITS) - 1;
        if (fp->sign == -1)
            d->x80[2] |= 1 << LDBL_EXP_BITS;
        return 0;
    }

    if (fp->mod == FP_VAL_NAN)
    {
        d->x80[0] = ~0;
        d->x80[1] = ~0;
        d->x80[2] = (1 << LDBL_EXP_BITS) - 1;
        if (fp->sign == -1)
            d->x80[2] |= 1 << LDBL_EXP_BITS;
        return 0;
    }

    TRACE("%c %s *2^%d (round %d)\n", fp->sign == -1 ? '-' : '+',
            wine_dbgstr_longlong(fp->m), fp->exp, fp->mod);
    if (!fp->m)
    {
        d->x80[0] = 0;
        d->x80[1] = 0;
        d->x80[2] = 0;
        if (fp->sign == -1)
            d->x80[2] |= 1 << LDBL_EXP_BITS;
        return 0;
    }

    /* make sure that we don't overflow modifying exponent */
    if (fp->exp > 1<<LDBL_EXP_BITS)
    {
        d->x80[0] = 0;
        d->x80[1] = 0x80000000;
        d->x80[2] = (1 << LDBL_EXP_BITS) - 1;
        if (fp->sign == -1)
            d->x80[2] |= 1 << LDBL_EXP_BITS;
        return MSVCRT_ERANGE;
    }
    if (fp->exp < -(1<<LDBL_EXP_BITS))
    {
        d->x80[0] = 0;
        d->x80[1] = 0;
        d->x80[2] = 0;
        if (fp->sign == -1)
            d->x80[2] |= 1 << LDBL_EXP_BITS;
        return MSVCRT_ERANGE;
    }
    fp->exp += LDBL_MANT_BITS - 1;

    /* normalize mantissa */
    while(fp->m < (ULONGLONG)1 << (LDBL_MANT_BITS-1))
    {
        fp->m <<= 1;
        fp->exp--;
    }
    fp->exp += (1 << (LDBL_EXP_BITS-1)) - 1;

    /* handle subnormals */
    if (fp->exp <= 0)
    {
        if (fp->m & 1 && fp->mod == FP_ROUND_ZERO) fp->mod = FP_ROUND_EVEN;
        else if (fp->m & 1) fp->mod = FP_ROUND_UP;
        else if (fp->mod != FP_ROUND_ZERO) fp->mod = FP_ROUND_DOWN;
        fp->m >>= 1;
    }
    while(fp->m && fp->exp<0)
    {
        if (fp->m & 1 && fp->mod == FP_ROUND_ZERO) fp->mod = FP_ROUND_EVEN;
        else if (fp->m & 1) fp->mod = FP_ROUND_UP;
        else if (fp->mod != FP_ROUND_ZERO) fp->mod = FP_ROUND_DOWN;
        fp->m >>= 1;
        fp->exp++;
    }

    /* round mantissa */
    if (fp->mod == FP_ROUND_UP || (fp->mod == FP_ROUND_EVEN && fp->m & 1))
    {
        if (fp->m == MSVCRT_UI64_MAX)
        {
            fp->m = (ULONGLONG)1 << (LDBL_MANT_BITS - 1);
            fp->exp++;
        }
        else
        {
            fp->m++;

            /* handle subnormal that falls into regular range due to rounding */
            if ((fp->m ^ (fp->m - 1)) & ((ULONGLONG)1 << (LDBL_MANT_BITS - 1))) fp->exp++;
        }
    }

    if (fp->exp >= (1<<LDBL_EXP_BITS)-1)
    {
        d->x80[0] = 0;
        d->x80[1] = 0x80000000;
        d->x80[2] = (1 << LDBL_EXP_BITS) - 1;
        if (fp->sign == -1)
            d->x80[2] |= 1 << LDBL_EXP_BITS;
        return MSVCRT_ERANGE;
    }
    if (!fp->m || fp->exp < 0)
    {
        d->x80[0] = 0;
        d->x80[1] = 0;
        d->x80[2] = 0;
        if (fp->sign == -1)
            d->x80[2] |= 1 << LDBL_EXP_BITS;
        return MSVCRT_ERANGE;
    }

    d->x80[0] = fp->m;
    d->x80[1] = fp->m >> 32;
    d->x80[2] = fp->exp;
    if (fp->sign == -1)
        d->x80[2] |= 1 << LDBL_EXP_BITS;
    return 0;
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

static struct fpnum fpnum_parse16(MSVCRT_wchar_t get(void *ctx), void unget(void *ctx),
        void *ctx, int sign, MSVCRT_pthreadlocinfo locinfo)
{
    BOOL found_digit = FALSE, found_dp = FALSE;
    enum fpmod round = FP_ROUND_ZERO;
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

        if (val || round != FP_ROUND_ZERO)
        {
            if (val < 8) round = FP_ROUND_DOWN;
            else if (val == 8 && round == FP_ROUND_ZERO) round = FP_ROUND_EVEN;
            else round = FP_ROUND_UP;
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
        return fpnum(0, 0, 0, 0);
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

        if (val || round != FP_ROUND_ZERO)
        {
            if (val < 8) round = FP_ROUND_DOWN;
            else if (val == 8 && round == FP_ROUND_ZERO) round = FP_ROUND_EVEN;
            else round = FP_ROUND_UP;
        }
    }

    if (!found_digit)
    {
        if (nch != MSVCRT_WEOF) unget(ctx);
        if (found_dp) unget(ctx);
        unget(ctx);
        return fpnum(0, 0, 0, 0);
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
                if(e>INT_MAX/10 || e*10>INT_MAX-nch+'0')
                    e = INT_MAX;
                else
                    e = e*10+nch-'0';
                nch = get(ctx);
            }
            if((nch!=MSVCRT_WEOF) && (nch < '0' || nch > '9')) unget(ctx);
            e *= s;

            if(e<0 && exp<INT_MIN-e) exp = INT_MIN;
            else if(e>0 && exp>INT_MAX-e) exp = INT_MAX;
            else exp += e;
        } else {
            if(nch != MSVCRT_WEOF) unget(ctx);
            if(found_sign) unget(ctx);
            unget(ctx);
        }
    }

    return fpnum(sign, exp, m, round);
}
#endif

/* Converts first 3 limbs to ULONGLONG */
/* Return FALSE on overflow */
static inline BOOL bnum_to_mant(struct bnum *b, ULONGLONG *m)
{
    if(MSVCRT_UI64_MAX / LIMB_MAX / LIMB_MAX < b->data[bnum_idx(b, b->e-1)]) return FALSE;
    *m = (ULONGLONG)b->data[bnum_idx(b, b->e-1)] * LIMB_MAX * LIMB_MAX;
    if(b->b == b->e-1) return TRUE;
    if(MSVCRT_UI64_MAX - *m < (ULONGLONG)b->data[bnum_idx(b, b->e-2)] * LIMB_MAX) return FALSE;
    *m += (ULONGLONG)b->data[bnum_idx(b, b->e-2)] * LIMB_MAX;
    if(b->b == b->e-2) return TRUE;
    if(MSVCRT_UI64_MAX - *m < b->data[bnum_idx(b, b->e-3)]) return FALSE;
    *m += b->data[bnum_idx(b, b->e-3)];
    return TRUE;
}

static struct fpnum fpnum_parse_bnum(MSVCRT_wchar_t (*get)(void *ctx), void (*unget)(void *ctx),
        void *ctx, MSVCRT_pthreadlocinfo locinfo, BOOL ldouble, struct bnum *b)
{
#if _MSVCR_VER >= 140
    MSVCRT_wchar_t _infinity[] = { 'i', 'n', 'f', 'i', 'n', 'i', 't', 'y', 0 };
    MSVCRT_wchar_t _nan[] = { 'n', 'a', 'n', 0 };
    MSVCRT_wchar_t *str_match = NULL;
    int matched=0;
#endif
    BOOL found_digit = FALSE, found_dp = FALSE, found_sign = FALSE;
    int e2 = 0, dp=0, sign=1, off, limb_digits = 0, i;
    enum fpmod round = FP_ROUND_ZERO;
    MSVCRT_wchar_t nch;
    ULONGLONG m;

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
            if (str_match == _infinity)
                return fpnum(sign, 0, 0, FP_VAL_INFINITY);
            if (str_match == _nan)
                return fpnum(sign, 0, 0, FP_VAL_NAN);
        } else if(found_sign) {
            unget(ctx);
        }

        return fpnum(0, 0, 0, 0);
    }

    if(nch == '0') {
        found_digit = TRUE;
        nch = get(ctx);
        if(nch == 'x' || nch == 'X')
            return fpnum_parse16(get, unget, ctx, sign, locinfo);
    }
#endif

    while(nch == '0') {
        found_digit = TRUE;
        nch = get(ctx);
    }

    b->b = 0;
    b->e = 1;
    b->data[0] = 0;
    while(nch>='0' && nch<='9') {
        found_digit = TRUE;
        if(limb_digits == LIMB_DIGITS) {
            if(bnum_idx(b, b->b-1) == bnum_idx(b, b->e)) break;
            else {
                b->b--;
                b->data[bnum_idx(b, b->b)] = 0;
                limb_digits = 0;
            }
        }

        b->data[bnum_idx(b, b->b)] = b->data[bnum_idx(b, b->b)] * 10 + nch - '0';
        limb_digits++;
        nch = get(ctx);
        dp++;
    }
    while(nch>='0' && nch<='9') {
        if(nch != '0') b->data[bnum_idx(b, b->b)] |= 1;
        nch = get(ctx);
        dp++;
    }

    if(nch == *locinfo->lconv->decimal_point) {
        found_dp = TRUE;
        nch = get(ctx);
    }

    /* skip leading '0' */
    if(nch=='0' && !limb_digits && !b->b) {
        found_digit = TRUE;
        while(nch == '0') {
            nch = get(ctx);
            dp--;
        }
    }

    while(nch>='0' && nch<='9') {
        found_digit = TRUE;
        if(limb_digits == LIMB_DIGITS) {
            if(bnum_idx(b, b->b-1) == bnum_idx(b, b->e)) break;
            else {
                b->b--;
                b->data[bnum_idx(b, b->b)] = 0;
                limb_digits = 0;
            }
        }

        b->data[bnum_idx(b, b->b)] = b->data[bnum_idx(b, b->b)] * 10 + nch - '0';
        limb_digits++;
        nch = get(ctx);
    }
    while(nch>='0' && nch<='9') {
        if(nch != '0') b->data[bnum_idx(b, b->b)] |= 1;
        nch = get(ctx);
    }

    if(!found_digit) {
        if(nch != MSVCRT_WEOF) unget(ctx);
        if(found_dp) unget(ctx);
        if(found_sign) unget(ctx);
        return fpnum(0, 0, 0, 0);
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
                if(e>INT_MAX/10 || e*10>INT_MAX-nch+'0')
                    e = INT_MAX;
                else
                    e = e*10+nch-'0';
                nch = get(ctx);
            }
            if(nch != MSVCRT_WEOF) unget(ctx);
            e *= s;

            if(e<0 && dp<INT_MIN-e) dp = INT_MIN;
            else if(e>0 && dp>INT_MAX-e) dp = INT_MAX;
            else dp += e;
        } else {
            if(nch != MSVCRT_WEOF) unget(ctx);
            if(found_sign) unget(ctx);
            unget(ctx);
        }
    } else if(nch != MSVCRT_WEOF) {
        unget(ctx);
    }

    if(!b->data[bnum_idx(b, b->e-1)])
        return fpnum(sign, 0, 0, 0);

    /* Fill last limb with 0 if needed */
    if(b->b+1 != b->e) {
        for(; limb_digits != LIMB_DIGITS; limb_digits++)
            b->data[bnum_idx(b, b->b)] *= 10;
    }
    for(; bnum_idx(b, b->b) < bnum_idx(b, b->e); b->b++) {
        if(b->data[bnum_idx(b, b->b)]) break;
    }

    /* move decimal point to limb boundary */
    if(limb_digits==dp && b->b==b->e-1)
        return fpnum(sign, 0, b->data[bnum_idx(b, b->e-1)], FP_ROUND_ZERO);
    off = (dp - limb_digits) % LIMB_DIGITS;
    if(off < 0) off += LIMB_DIGITS;
    if(off) bnum_mult(b, p10s[off]);

    if(dp-1 > (ldouble ? DBL80_MAX_10_EXP : MSVCRT_DBL_MAX_10_EXP))
        return fpnum(sign, INT_MAX, 1, FP_ROUND_ZERO);
    /* Count part of exponent stored in denormalized mantissa. */
    /* Increase exponent range to handle subnormals. */
    if(dp-1 < (ldouble ? DBL80_MIN_10_EXP : MSVCRT_DBL_MIN_10_EXP-MSVCRT_DBL_DIG-18))
        return fpnum(sign, INT_MIN, 1, FP_ROUND_ZERO);

    while(dp > 3*LIMB_DIGITS) {
        if(bnum_rshift(b, 9)) dp -= LIMB_DIGITS;
        e2 += 9;
    }
    while(dp <= 2*LIMB_DIGITS) {
        if(bnum_lshift(b, 29)) dp += LIMB_DIGITS;
        e2 -= 29;
    }
    /* Make sure most significant mantissa bit will be set */
    while(b->data[bnum_idx(b, b->e-1)] <= 9) {
        bnum_lshift(b, 1);
        e2--;
    }
    while(!bnum_to_mant(b, &m)) {
        bnum_rshift(b, 1);
        e2++;
    }

    if(b->e-4 >= b->b && b->data[bnum_idx(b, b->e-4)]) {
        if(b->data[bnum_idx(b, b->e-4)] > LIMB_MAX/2) round = FP_ROUND_UP;
        else if(b->data[bnum_idx(b, b->e-4)] == LIMB_MAX/2) round = FP_ROUND_EVEN;
        else round = FP_ROUND_DOWN;
    }
    if(round == FP_ROUND_ZERO || round == FP_ROUND_EVEN) {
        for(i=b->e-5; i>=b->b; i--) {
            if(!b->data[bnum_idx(b, b->b)]) continue;
            if(round == FP_ROUND_EVEN) round = FP_ROUND_UP;
            else round = FP_ROUND_DOWN;
        }
    }

    return fpnum(sign, e2, m, round);
}

struct fpnum fpnum_parse(MSVCRT_wchar_t (*get)(void *ctx), void (*unget)(void *ctx),
       void *ctx, MSVCRT_pthreadlocinfo locinfo, BOOL ldouble)
{
    if(!ldouble) {
        BYTE bnum_data[FIELD_OFFSET(struct bnum, data[BNUM_PREC64])];
        struct bnum *b = (struct bnum*)bnum_data;

        b->size = BNUM_PREC64;
        return fpnum_parse_bnum(get, unget, ctx, locinfo, ldouble, b);
    } else {
        BYTE bnum_data[FIELD_OFFSET(struct bnum, data[BNUM_PREC80])];
        struct bnum *b = (struct bnum*)bnum_data;

        b->size = BNUM_PREC80;
        return fpnum_parse_bnum(get, unget, ctx, locinfo, ldouble, b);
    }
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

static inline double strtod_helper(const char *str, char **end, MSVCRT__locale_t locale, int *perr)
{
    MSVCRT_pthreadlocinfo locinfo;
    const char *beg, *p;
    struct fpnum fp;
    double ret;
    int err;

    if (perr) *perr = 0;
#if _MSVCR_VER == 0
    else *MSVCRT__errno() = 0;
#endif

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

    fp = fpnum_parse(strtod_str_get, strtod_str_unget, &p, locinfo, FALSE);
    if (end) *end = (p == beg ? (char*)str : (char*)p);

    err = fpnum_double(&fp, &ret);
    if (perr) *perr = err;
    else if(err) *MSVCRT__errno() = err;
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
 *      strcat (MSVCRT.@)
 */
char* __cdecl MSVCRT_strcat( char *dst, const char *src )
{
    char *d = dst;
    while (*d) d++;
    while ((*d++ = *src++));
    return dst;
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
    char *d = dst;
    while (*d) d++;
    for ( ; len && *src; d++, src++, len--) *d = *src;
    *d = 0;
    return dst;
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
 *		__STRINGTOLD_L (MSVCR80.@)
 */
int CDECL __STRINGTOLD_L( MSVCRT__LDOUBLE *value, char **endptr,
        const char *str, int flags, MSVCRT__locale_t locale )
{
    MSVCRT_pthreadlocinfo locinfo;
    const char *beg, *p;
    int err, ret = 0;
    struct fpnum fp;

    if (flags) FIXME("flags not supported: %x\n", flags);

    if (!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    p = str;
    while (MSVCRT__isspace_l((unsigned char)*p, locale))
        p++;
    beg = p;

    fp = fpnum_parse(strtod_str_get, strtod_str_unget, &p, locinfo, TRUE);
    if (endptr) *endptr = (p == beg ? (char*)str : (char*)p);
    if (p == beg) ret = 4;

    err = fpnum_ldouble(&fp, value);
    if (err) ret = (value->x80[2] & 0x7fff ? 2 : 1);
    return ret;
}

/********************************************************************
 *              __STRINGTOLD (MSVCRT.@)
 */
int CDECL __STRINGTOLD( MSVCRT__LDOUBLE *value, char **endptr, const char *str, int flags )
{
    return __STRINGTOLD_L( value, endptr, str, flags, NULL );
}

/********************************************************************
 *              _atoldbl_l (MSVCRT.@)
 */
int CDECL MSVCRT__atoldbl_l( MSVCRT__LDOUBLE *value, const char *str, MSVCRT__locale_t locale )
{
    char *endptr;
    switch(__STRINGTOLD_L( value, &endptr, str, 0, locale ))
    {
    case 1: return MSVCRT__UNDERFLOW;
    case 2: return MSVCRT__OVERFLOW;
    default: return 0;
    }
}

/********************************************************************
 *		_atoldbl (MSVCRT.@)
 */
int CDECL MSVCRT__atoldbl(MSVCRT__LDOUBLE *value, const char *str)
{
    return MSVCRT__atoldbl_l( value, str, NULL );
}

/*********************************************************************
 *              strlen (MSVCRT.@)
 */
MSVCRT_size_t __cdecl MSVCRT_strlen(const char *str)
{
    const char *s = str;
    while (*s) s++;
    return s - str;
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

    MSVCRT_sprintf(format, "%%.%dle", prec);
    MSVCRT_sprintf(buf, format, d);

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
    const unsigned char *p1, *p2;

    for (p1 = ptr1, p2 = ptr2; n; n--, p1++, p2++)
    {
        if (*p1 < *p2) return -1;
        if (*p1 > *p2) return 1;
    }
    return 0;
}

/*********************************************************************
 *                  memmove (MSVCRT.@)
 */
#ifdef WORDS_BIGENDIAN
# define MERGE(w1, sh1, w2, sh2) ((w1 << sh1) | (w2 >> sh2))
#else
# define MERGE(w1, sh1, w2, sh2) ((w1 >> sh1) | (w2 << sh2))
#endif
void * __cdecl MSVCRT_memmove(void *dst, const void *src, MSVCRT_size_t n)
{
    unsigned char *d = dst;
    const unsigned char *s = src;
    int sh1;

    if (!n) return dst;

    if ((MSVCRT_size_t)dst - (MSVCRT_size_t)src >= n)
    {
        for (; (MSVCRT_size_t)d % sizeof(MSVCRT_size_t) && n; n--) *d++ = *s++;

        sh1 = 8 * ((MSVCRT_size_t)s % sizeof(MSVCRT_size_t));
        if (!sh1)
        {
            while (n >= sizeof(MSVCRT_size_t))
            {
                *(MSVCRT_size_t*)d = *(MSVCRT_size_t*)s;
                s += sizeof(MSVCRT_size_t);
                d += sizeof(MSVCRT_size_t);
                n -= sizeof(MSVCRT_size_t);
            }
        }
        else if (n >= 2 * sizeof(MSVCRT_size_t))
        {
            int sh2 = 8 * sizeof(MSVCRT_size_t) - sh1;
            MSVCRT_size_t x, y;

            s -= sh1 / 8;
            x = *(MSVCRT_size_t*)s;
            do
            {
                s += sizeof(MSVCRT_size_t);
                y = *(MSVCRT_size_t*)s;
                *(MSVCRT_size_t*)d = MERGE(x, sh1, y, sh2);
                d += sizeof(MSVCRT_size_t);

                s += sizeof(MSVCRT_size_t);
                x = *(MSVCRT_size_t*)s;
                *(MSVCRT_size_t*)d = MERGE(y, sh1, x, sh2);
                d += sizeof(MSVCRT_size_t);

                n -= 2 * sizeof(MSVCRT_size_t);
            } while (n >= 2 * sizeof(MSVCRT_size_t));
            s += sh1 / 8;
        }
        while (n--) *d++ = *s++;
        return dst;
    }
    else
    {
        d += n;
        s += n;

        for (; (MSVCRT_size_t)d % sizeof(MSVCRT_size_t) && n; n--) *--d = *--s;

        sh1 = 8 * ((MSVCRT_size_t)s % sizeof(MSVCRT_size_t));
        if (!sh1)
        {
            while (n >= sizeof(MSVCRT_size_t))
            {
                s -= sizeof(MSVCRT_size_t);
                d -= sizeof(MSVCRT_size_t);
                *(MSVCRT_size_t*)d = *(MSVCRT_size_t*)s;
                n -= sizeof(MSVCRT_size_t);
            }
        }
        else if (n >= 2 * sizeof(MSVCRT_size_t))
        {
            int sh2 = 8 * sizeof(MSVCRT_size_t) - sh1;
            MSVCRT_size_t x, y;

            s -= sh1 / 8;
            x = *(MSVCRT_size_t*)s;
            do
            {
                s -= sizeof(MSVCRT_size_t);
                y = *(MSVCRT_size_t*)s;
                d -= sizeof(MSVCRT_size_t);
                *(MSVCRT_size_t*)d = MERGE(y, sh1, x, sh2);

                s -= sizeof(MSVCRT_size_t);
                x = *(MSVCRT_size_t*)s;
                d -= sizeof(MSVCRT_size_t);
                *(MSVCRT_size_t*)d = MERGE(x, sh1, y, sh2);

                n -= 2 * sizeof(MSVCRT_size_t);
            } while (n >= 2 * sizeof(MSVCRT_size_t));
            s += sh1 / 8;
        }
        while (n--) *--d = *--s;
    }
    return dst;
}
#undef MERGE

/*********************************************************************
 *                  memcpy   (MSVCRT.@)
 */
void * __cdecl MSVCRT_memcpy(void *dst, const void *src, MSVCRT_size_t n)
{
    return MSVCRT_memmove(dst, src, n);
}

/*********************************************************************
 *		    memset (MSVCRT.@)
 */
void* __cdecl MSVCRT_memset(void *dst, int c, MSVCRT_size_t n)
{
    volatile unsigned char *d = dst;  /* avoid gcc optimizations */
    while (n--) *d++ = c;
    return dst;
}

/*********************************************************************
 *		    strchr (MSVCRT.@)
 */
char* __cdecl MSVCRT_strchr(const char *str, int c)
{
    do
    {
        if (*str == (char)c) return (char*)str;
    } while (*str++);
    return NULL;
}

/*********************************************************************
 *                  strrchr (MSVCRT.@)
 */
char* __cdecl MSVCRT_strrchr(const char *str, int c)
{
    char *ret = NULL;
    do { if (*str == (char)c) ret = (char*)str; } while (*str++);
    return ret;
}

/*********************************************************************
 *                  memchr   (MSVCRT.@)
 */
void* __cdecl MSVCRT_memchr(const void *ptr, int c, MSVCRT_size_t n)
{
    const unsigned char *p = ptr;

    for (p = ptr; n; n--, p++) if (*p == c) return (void *)(ULONG_PTR)p;
    return NULL;
}

/*********************************************************************
 *                  strcmp (MSVCRT.@)
 */
int __cdecl MSVCRT_strcmp(const char *str1, const char *str2)
{
    while (*str1 && *str1 == *str2) { str1++; str2++; }
    if ((unsigned char)*str1 > (unsigned char)*str2) return 1;
    if ((unsigned char)*str1 < (unsigned char)*str2) return -1;
    return 0;
}

/*********************************************************************
 *                  strncmp   (MSVCRT.@)
 */
int __cdecl MSVCRT_strncmp(const char *str1, const char *str2, MSVCRT_size_t len)
{
    if (!len) return 0;
    while (--len && *str1 && *str1 == *str2) { str1++; str2++; }
    return (unsigned char)*str1 - (unsigned char)*str2;
}

/*********************************************************************
 *                  _strnicmp_l   (MSVCRT.@)
 */
int __cdecl MSVCRT__strnicmp_l(const char *s1, const char *s2,
        MSVCRT_size_t count, MSVCRT__locale_t locale)
{
    MSVCRT_pthreadlocinfo locinfo;
    int c1, c2;

    if(s1==NULL || s2==NULL)
        return MSVCRT__NLSCMPERROR;

    if(!count)
        return 0;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!locinfo->lc_handle[MSVCRT_LC_CTYPE])
    {
        do {
            if ((c1 = *s1++) >= 'A' && c1 <= 'Z')
                c1 -= 'A' - 'a';
            if ((c2 = *s2++) >= 'A' && c2 <= 'Z')
                c2 -= 'A' - 'a';
        }while(--count && c1 && c1==c2);

        return c1-c2;
    }

    do {
        c1 = MSVCRT__tolower_l((unsigned char)*s1++, locale);
        c2 = MSVCRT__tolower_l((unsigned char)*s2++, locale);
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
    for (; *str; str++) if (strchr( accept, *str )) return (char*)str;
    return NULL;
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
