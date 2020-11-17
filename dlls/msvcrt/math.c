/*
 * msvcrt.dll math functions
 *
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
 *
 *
 * For functions copied from musl libc (http://musl.libc.org/):
 * ====================================================
 * Copyright 2005-2020 Rich Felker, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * ====================================================
 */

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#define __USE_ISOC9X 1
#define __USE_ISOC99 1
#include <math.h>
#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif

#include "msvcrt.h"

#include "wine/asm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

#define _DOMAIN         1       /* domain error in argument */
#define _SING           2       /* singularity */
#define _OVERFLOW       3       /* range overflow */
#define _UNDERFLOW      4       /* range underflow */

typedef int (CDECL *MSVCRT_matherr_func)(struct MSVCRT__exception *);
typedef double LDOUBLE;  /* long double is just a double */

static MSVCRT_matherr_func MSVCRT_default_matherr_func = NULL;

static BOOL sse2_supported;
static BOOL sse2_enabled;

void msvcrt_init_math(void)
{
    sse2_supported = sse2_enabled = IsProcessorFeaturePresent( PF_XMMI64_INSTRUCTIONS_AVAILABLE );
}

/* Copied from musl: src/internal/libm.h */
static inline float fp_barrierf(float x)
{
    volatile float y = x;
    return y;
}

/*********************************************************************
 *      _matherr (CRTDLL.@)
 */
int CDECL MSVCRT__matherr(struct MSVCRT__exception *e)
{
    return 0;
}


static double math_error(int type, const char *name, double arg1, double arg2, double retval)
{
    struct MSVCRT__exception exception = {type, (char *)name, arg1, arg2, retval};

    TRACE("(%d, %s, %g, %g, %g)\n", type, debugstr_a(name), arg1, arg2, retval);

    if (MSVCRT_default_matherr_func && MSVCRT_default_matherr_func(&exception))
        return exception.retval;

    switch (type)
    {
    case _DOMAIN:
        *MSVCRT__errno() = MSVCRT_EDOM;
        break;
    case _SING:
    case _OVERFLOW:
        *MSVCRT__errno() = MSVCRT_ERANGE;
        break;
    case _UNDERFLOW:
        /* don't set errno */
        break;
    default:
        ERR("Unhandled math error!\n");
    }

    return exception.retval;
}

/*********************************************************************
 *      __setusermatherr (MSVCRT.@)
 */
void CDECL MSVCRT___setusermatherr(MSVCRT_matherr_func func)
{
    MSVCRT_default_matherr_func = func;
    TRACE("new matherr handler %p\n", func);
}

/*********************************************************************
 *      _set_SSE2_enable (MSVCRT.@)
 */
int CDECL MSVCRT__set_SSE2_enable(int flag)
{
    sse2_enabled = flag && sse2_supported;
    return sse2_enabled;
}

#if defined(_WIN64)
# if _MSVCR_VER>=140
/*********************************************************************
 *      _get_FMA3_enable (UCRTBASE.@)
 */
int CDECL MSVCRT__get_FMA3_enable(void)
{
    FIXME("() stub\n");
    return 0;
}
# endif

# if _MSVCR_VER>=120
/*********************************************************************
 *      _set_FMA3_enable (MSVCR120.@)
 */
int CDECL MSVCRT__set_FMA3_enable(int flag)
{
    FIXME("(%x) stub\n", flag);
    return 0;
}
# endif
#endif

#if !defined(__i386__) || _MSVCR_VER>=120

/*********************************************************************
 *      _chgsignf (MSVCRT.@)
 */
float CDECL MSVCRT__chgsignf( float num )
{
    union { float f; UINT32 i; } u = { num };
    u.i ^= 0x80000000;
    return u.f;
}

/*********************************************************************
 *      _copysignf (MSVCRT.@)
 *
 * Copied from musl: src/math/copysignf.c
 */
float CDECL MSVCRT__copysignf( float x, float y )
{
    union { float f; UINT32 i; } ux = { x }, uy = { y };
    ux.i &= 0x7fffffff;
    ux.i |= uy.i & 0x80000000;
    return ux.f;
}

/*********************************************************************
 *      _nextafterf (MSVCRT.@)
 */
float CDECL MSVCRT__nextafterf( float num, float next )
{
    if (!isfinite(num) || !isfinite(next)) *MSVCRT__errno() = MSVCRT_EDOM;
    return nextafterf( num, next );
}

/*********************************************************************
 *      _logbf (MSVCRT.@)
 */
float CDECL MSVCRT__logbf( float num )
{
    float ret = logbf(num);
    if (isnan(num)) return math_error(_DOMAIN, "_logbf", num, 0, ret);
    if (!num) return math_error(_SING, "_logbf", num, 0, ret);
    return ret;
}

#endif

#ifndef __i386__

/*********************************************************************
 *      _fpclassf (MSVCRT.@)
 */
int CDECL MSVCRT__fpclassf( float num )
{
    union { float f; UINT32 i; } u = { num };
    int e = u.i >> 23 & 0xff;
    int s = u.i >> 31;

    switch (e)
    {
    case 0:
        if (u.i << 1) return s ? MSVCRT__FPCLASS_ND : MSVCRT__FPCLASS_PD;
        return s ? MSVCRT__FPCLASS_NZ : MSVCRT__FPCLASS_PZ;
    case 0xff:
        if (u.i << 9) return ((u.i >> 22) & 1) ? MSVCRT__FPCLASS_QNAN : MSVCRT__FPCLASS_SNAN;
        return s ? MSVCRT__FPCLASS_NINF : MSVCRT__FPCLASS_PINF;
    default:
        return s ? MSVCRT__FPCLASS_NN : MSVCRT__FPCLASS_PN;
    }
}

/*********************************************************************
 *      _finitef (MSVCRT.@)
 */
int CDECL MSVCRT__finitef( float num )
{
    union { float f; UINT32 i; } u = { num };
    return (u.i & 0x7fffffff) < 0x7f800000;
}

/*********************************************************************
 *      _isnanf (MSVCRT.@)
 */
int CDECL MSVCRT__isnanf( float num )
{
    union { float f; UINT32 i; } u = { num };
    return (u.i & 0x7fffffff) > 0x7f800000;
}

/*********************************************************************
 *      MSVCRT_acosf (MSVCRT.@)
 *
 * Copied from musl: src/math/acosf.c
 */
static float acosf_R(float z)
{
    static const float pS0 = 1.6666586697e-01,
                 pS1 = -4.2743422091e-02,
                 pS2 = -8.6563630030e-03,
                 qS1 = -7.0662963390e-01;

    float p, q;
    p = z * (pS0 + z * (pS1 + z * pS2));
    q = 1.0f + z * qS1;
    return p / q;
}

float CDECL MSVCRT_acosf( float x )
{
    static const float pio2_hi = 1.5707962513e+00,
                 pio2_lo = 7.5497894159e-08;

    float z, w, s, c, df;
    unsigned int hx, ix;

    hx = *(unsigned int*)&x;
    ix = hx & 0x7fffffff;
    /* |x| >= 1 or nan */
    if (ix >= 0x3f800000) {
        if (ix == 0x3f800000) {
            if (hx >> 31)
                return 2 * pio2_lo + 2 * pio2_hi + 7.5231638453e-37;
            return 0;
        }
        if (isnan(x)) return x;
        return math_error(_DOMAIN, "acosf", x, 0, 0 / (x - x));
    }
    /* |x| < 0.5 */
    if (ix < 0x3f000000) {
        if (ix <= 0x32800000) /* |x| < 2**-26 */
            return pio2_lo + pio2_hi + 7.5231638453e-37;
        return pio2_hi - (x - (pio2_lo - x * acosf_R(x * x)));
    }
    /* x < -0.5 */
    if (hx >> 31) {
        z = (1 + x) * 0.5f;
        s = sqrtf(z);
        w = acosf_R(z) * s - pio2_lo;
        return 2 * (pio2_hi - (s + w));
    }
    /* x > 0.5 */
    z = (1 - x) * 0.5f;
    s = sqrtf(z);
    hx = *(unsigned int*)&s & 0xfffff000;
    df = *(float*)&hx;
    c = (z - df * df) / (s + df);
    w = acosf_R(z) * s + c;
    return 2 * (df + w);
}

/*********************************************************************
 *      MSVCRT_asinf (MSVCRT.@)
 *
 * Copied from musl: src/math/asinf.c
 */
static float asinf_R(float z)
{
    /* coefficients for R(x^2) */
    static const float pS0 =  1.6666586697e-01,
                 pS1 = -4.2743422091e-02,
                 pS2 = -8.6563630030e-03,
                 qS1 = -7.0662963390e-01;

    float_t p, q;
    p = z * (pS0 + z * (pS1 + z * pS2));
    q = 1.0f + z * qS1;
    return p / q;
}

float CDECL MSVCRT_asinf( float x )
{
    static const double pio2 = 1.570796326794896558e+00;

    double s;
    float z;
    unsigned int hx, ix;

    hx = *(unsigned int*)&x;
    ix = hx & 0x7fffffff;
    if (ix >= 0x3f800000) {  /* |x| >= 1 */
        if (ix == 0x3f800000)  /* |x| == 1 */
            return x * pio2 + 7.5231638453e-37;  /* asin(+-1) = +-pi/2 with inexact */
        if (isnan(x)) return x;
        return math_error(_DOMAIN, "asinf", x, 0, 0 / (x - x));
    }
    if (ix < 0x3f000000) {  /* |x| < 0.5 */
        /* if 0x1p-126 <= |x| < 0x1p-12, avoid raising underflow */
        if (ix < 0x39800000 && ix >= 0x00800000)
            return x;
        return x + x * asinf_R(x * x);
    }
    /* 1 > |x| >= 0.5 */
    z = (1 - fabsf(x)) * 0.5f;
    s = sqrt(z);
    x = pio2 - 2 * (s + s * asinf_R(z));
    if (hx >> 31)
        return -x;
    return x;
}

/*********************************************************************
 *      MSVCRT_atanf (MSVCRT.@)
 *
 * Copied from musl: src/math/atanf.c
 */
float CDECL MSVCRT_atanf( float x )
{
    static const float atanhi[] = {
        4.6364760399e-01,
        7.8539812565e-01,
        9.8279368877e-01,
        1.5707962513e+00,
    };
    static const float atanlo[] = {
        5.0121582440e-09,
        3.7748947079e-08,
        3.4473217170e-08,
        7.5497894159e-08,
    };
    static const float aT[] = {
        3.3333328366e-01,
        -1.9999158382e-01,
        1.4253635705e-01,
        -1.0648017377e-01,
        6.1687607318e-02,
    };

    float w, s1, s2, z;
    unsigned int ix, sign;
    int id;

#if _MSVCR_VER == 0
    if (isnan(x)) return math_error(_DOMAIN, "atanf", x, 0, x);
#endif

    ix = *(unsigned int*)&x;
    sign = ix >> 31;
    ix &= 0x7fffffff;
    if (ix >= 0x4c800000) {  /* if |x| >= 2**26 */
        if (isnan(x))
            return x;
        z = atanhi[3] + 7.5231638453e-37;
        return sign ? -z : z;
    }
    if (ix < 0x3ee00000) {   /* |x| < 0.4375 */
        if (ix < 0x39800000) {  /* |x| < 2**-12 */
            if (ix < 0x00800000)
                /* raise underflow for subnormal x */
                fp_barrierf(x*x);
            return x;
        }
        id = -1;
    } else {
        x = fabsf(x);
        if (ix < 0x3f980000) {  /* |x| < 1.1875 */
            if (ix < 0x3f300000) {  /*  7/16 <= |x| < 11/16 */
                id = 0;
                x = (2.0f * x - 1.0f) / (2.0f + x);
            } else {                /* 11/16 <= |x| < 19/16 */
                id = 1;
                x = (x - 1.0f) / (x + 1.0f);
            }
        } else {
            if (ix < 0x401c0000) {  /* |x| < 2.4375 */
                id = 2;
                x = (x - 1.5f) / (1.0f + 1.5f * x);
            } else {                /* 2.4375 <= |x| < 2**26 */
                id = 3;
                x = -1.0f / x;
            }
        }
    }
    /* end of argument reduction */
    z = x * x;
    w = z * z;
    /* break sum from i=0 to 10 aT[i]z**(i+1) into odd and even poly */
    s1 = z * (aT[0] + w * (aT[2] + w * aT[4]));
    s2 = w * (aT[1] + w * aT[3]);
    if (id < 0)
        return x - x * (s1 + s2);
    z = atanhi[id] - ((x * (s1 + s2) - atanlo[id]) - x);
    return sign ? -z : z;
}

/*********************************************************************
 *              MSVCRT_atan2f (MSVCRT.@)
 *
 * Copied from musl: src/math/atan2f.c
 */
float CDECL MSVCRT_atan2f( float y, float x )
{
    static const float pi     = 3.1415927410e+00,
                 pi_lo  = -8.7422776573e-08;

    float z;
    unsigned int m, ix, iy;

    if (isnan(x) || isnan(y))
        return x + y;
    ix = *(unsigned int*)&x;
    iy = *(unsigned int*)&y;
    if (ix == 0x3f800000)  /* x=1.0 */
        return atanf(y);
    m = ((iy >> 31) & 1) | ((ix >> 30) & 2);  /* 2*sign(x)+sign(y) */
    ix &= 0x7fffffff;
    iy &= 0x7fffffff;

    /* when y = 0 */
    if (iy == 0) {
        switch (m) {
        case 0:
        case 1: return y;   /* atan(+-0,+anything)=+-0 */
        case 2: return pi;  /* atan(+0,-anything) = pi */
        case 3: return -pi; /* atan(-0,-anything) =-pi */
        }
    }
    /* when x = 0 */
    if (ix == 0)
        return m & 1 ? -pi / 2 : pi / 2;
    /* when x is INF */
    if (ix == 0x7f800000) {
        if (iy == 0x7f800000) {
            switch (m) {
            case 0: return pi / 4;      /* atan(+INF,+INF) */
            case 1: return -pi / 4;     /* atan(-INF,+INF) */
            case 2: return 3 * pi / 4;  /*atan(+INF,-INF)*/
            case 3: return -3 * pi / 4; /*atan(-INF,-INF)*/
            }
        } else {
            switch (m) {
            case 0: return 0.0f;    /* atan(+...,+INF) */
            case 1: return -0.0f;   /* atan(-...,+INF) */
            case 2: return pi;      /* atan(+...,-INF) */
            case 3: return -pi;     /* atan(-...,-INF) */
            }
        }
    }
    /* |y/x| > 0x1p26 */
    if (ix + (26 << 23) < iy || iy == 0x7f800000)
        return m & 1 ? -pi / 2 : pi / 2;

    /* z = atan(|y/x|) with correct underflow */
    if ((m & 2) && iy + (26 << 23) < ix)  /*|y/x| < 0x1p-26, x < 0 */
        z = 0.0;
    else
        z = atanf(fabsf(y / x));
    switch (m) {
    case 0: return z;                /* atan(+,+) */
    case 1: return -z;               /* atan(-,+) */
    case 2: return pi - (z - pi_lo); /* atan(+,-) */
    default: /* case 3 */
        return (z - pi_lo) - pi;     /* atan(-,-) */
    }
}

/*********************************************************************
 *      MSVCRT_cosf (MSVCRT.@)
 */
float CDECL MSVCRT_cosf( float x )
{
  float ret = cosf(x);
  if (!isfinite(x)) return math_error(_DOMAIN, "cosf", x, 0, ret);
  return ret;
}

/*********************************************************************
 *      MSVCRT_coshf (MSVCRT.@)
 */
float CDECL MSVCRT_coshf( float x )
{
  float ret = coshf(x);
  if (isnan(x)) return math_error(_DOMAIN, "coshf", x, 0, ret);
  return ret;
}

/*********************************************************************
 *      MSVCRT_expf (MSVCRT.@)
 */
float CDECL MSVCRT_expf( float x )
{
  float ret = expf(x);
  if (isnan(x)) return math_error(_DOMAIN, "expf", x, 0, ret);
  if (isfinite(x) && !ret) return math_error(_UNDERFLOW, "expf", x, 0, ret);
  if (isfinite(x) && !isfinite(ret)) return math_error(_OVERFLOW, "expf", x, 0, ret);
  return ret;
}

/*********************************************************************
 *      MSVCRT_fmodf (MSVCRT.@)
 */
float CDECL MSVCRT_fmodf( float x, float y )
{
  float ret = fmodf(x, y);
  if (!isfinite(x) || !isfinite(y)) return math_error(_DOMAIN, "fmodf", x, 0, ret);
  return ret;
}

/*********************************************************************
 *      MSVCRT_logf (MSVCRT.@)
 */
float CDECL MSVCRT_logf( float x )
{
  float ret = logf(x);
  if (x < 0.0) return math_error(_DOMAIN, "logf", x, 0, ret);
  if (x == 0.0) return math_error(_SING, "logf", x, 0, ret);
  return ret;
}

/*********************************************************************
 *      MSVCRT_log10f (MSVCRT.@)
 */
float CDECL MSVCRT_log10f( float x )
{
  float ret = log10f(x);
  if (x < 0.0) return math_error(_DOMAIN, "log10f", x, 0, ret);
  if (x == 0.0) return math_error(_SING, "log10f", x, 0, ret);
  return ret;
}

/*********************************************************************
 *      MSVCRT_powf (MSVCRT.@)
 */
float CDECL MSVCRT_powf( float x, float y )
{
  float z = powf(x,y);
  if (x < 0 && y != floorf(y)) return math_error(_DOMAIN, "powf", x, y, z);
  if (!x && isfinite(y) && y < 0) return math_error(_SING, "powf", x, y, z);
  if (isfinite(x) && isfinite(y) && !isfinite(z)) return math_error(_OVERFLOW, "powf", x, y, z);
  if (x && isfinite(x) && isfinite(y) && !z) return math_error(_UNDERFLOW, "powf", x, y, z);
  return z;
}

/*********************************************************************
 *      MSVCRT_sinf (MSVCRT.@)
 */
float CDECL MSVCRT_sinf( float x )
{
  float ret = sinf(x);
  if (!isfinite(x)) return math_error(_DOMAIN, "sinf", x, 0, ret);
  return ret;
}

/*********************************************************************
 *      MSVCRT_sinhf (MSVCRT.@)
 */
float CDECL MSVCRT_sinhf( float x )
{
  float ret = sinhf(x);
  if (isnan(x)) return math_error(_DOMAIN, "sinhf", x, 0, ret);
  return ret;
}

/*********************************************************************
 *      MSVCRT_sqrtf (MSVCRT.@)
 *
 * Copied from musl: src/math/sqrtf.c
 */
float CDECL MSVCRT_sqrtf( float x )
{
    static const float tiny = 1.0e-30;

    float z;
    int sign = 0x80000000;
    int ix,s,q,m,t,i;
    unsigned int r;

    ix = *(int*)&x;

    /* take care of Inf and NaN */
    if ((ix & 0x7f800000) == 0x7f800000 && (ix == 0x7f800000 || ix & 0x7fffff))
        return x;

    /* take care of zero */
    if (ix <= 0) {
        if ((ix & ~sign) == 0)
            return x;  /* sqrt(+-0) = +-0 */
        return math_error(_DOMAIN, "sqrtf", x, 0, (x - x) / (x - x)); /* sqrt(-ve) = sNaN */
    }
    /* normalize x */
    m = ix >> 23;
    if (m == 0) {  /* subnormal x */
        for (i = 0; (ix & 0x00800000) == 0; i++)
            ix <<= 1;
        m -= i - 1;
    }
    m -= 127;  /* unbias exponent */
    ix = (ix & 0x007fffff) | 0x00800000;
    if (m & 1)  /* odd m, double x to make it even */
        ix += ix;
    m >>= 1;  /* m = [m/2] */

    /* generate sqrt(x) bit by bit */
    ix += ix;
    q = s = 0;       /* q = sqrt(x) */
    r = 0x01000000;  /* r = moving bit from right to left */

    while (r != 0) {
        t = s + r;
        if (t <= ix) {
            s = t + r;
            ix -= t;
            q += r;
        }
        ix += ix;
        r >>= 1;
    }

    /* use floating add to find out rounding direction */
    if (ix != 0) {
        z = 1.0f - tiny; /* raise inexact flag */
        if (z >= 1.0f) {
            z = 1.0f + tiny;
            if (z > 1.0f)
                q += 2;
            else
                q += q & 1;
        }
    }
    ix = (q >> 1) + 0x3f000000;
    r = ix + ((unsigned int)m << 23);
    z = *(float*)&r;
    return z;
}

/*********************************************************************
 *      MSVCRT_tanf (MSVCRT.@)
 */
float CDECL MSVCRT_tanf( float x )
{
  float ret = tanf(x);
  if (!isfinite(x)) return math_error(_DOMAIN, "tanf", x, 0, ret);
  return ret;
}

/*********************************************************************
 *      MSVCRT_tanhf (MSVCRT.@)
 */
float CDECL MSVCRT_tanhf( float x )
{
  float ret = tanhf(x);
  if (!isfinite(x)) return math_error(_DOMAIN, "tanhf", x, 0, ret);
  return ret;
}

/*********************************************************************
 *      ceilf (MSVCRT.@)
 */
float CDECL MSVCRT_ceilf( float x )
{
  return ceilf(x);
}

/*********************************************************************
 *      fabsf (MSVCRT.@)
 *
 * Copied from musl: src/math/fabsf.c
 */
float CDECL MSVCRT_fabsf( float x )
{
    union { float f; UINT32 i; } u = { x };
    u.i &= 0x7fffffff;
    return u.f;
}

/*********************************************************************
 *      floorf (MSVCRT.@)
 */
float CDECL MSVCRT_floorf( float x )
{
  return floorf(x);
}

/*********************************************************************
 *      frexpf (MSVCRT.@)
 */
float CDECL MSVCRT_frexpf( float x, int *exp )
{
  return frexpf( x, exp );
}

/*********************************************************************
 *      modff (MSVCRT.@)
 */
float CDECL MSVCRT_modff( float x, float *iptr )
{
  return modff( x, iptr );
}

#endif

/*********************************************************************
 *		MSVCRT_acos (MSVCRT.@)
 *
 * Copied from musl: src/math/acos.c
 */
static double acos_R(double z)
{
    static const double pS0 =  1.66666666666666657415e-01,
                 pS1 = -3.25565818622400915405e-01,
                 pS2 =  2.01212532134862925881e-01,
                 pS3 = -4.00555345006794114027e-02,
                 pS4 =  7.91534994289814532176e-04,
                 pS5 =  3.47933107596021167570e-05,
                 qS1 = -2.40339491173441421878e+00,
                 qS2 =  2.02094576023350569471e+00,
                 qS3 = -6.88283971605453293030e-01,
                 qS4 =  7.70381505559019352791e-02;

    double p, q;
    p = z * (pS0 + z * (pS1 + z * (pS2 + z * (pS3 + z * (pS4 + z * pS5)))));
    q = 1.0 + z * (qS1 + z * (qS2 + z * (qS3 + z * qS4)));
    return p/q;
}

double CDECL MSVCRT_acos( double x )
{
    static const double pio2_hi = 1.57079632679489655800e+00,
                 pio2_lo = 6.12323399573676603587e-17;

    double z, w, s, c, df;
    unsigned int hx, ix;
    ULONGLONG llx;

    hx = *(ULONGLONG*)&x >> 32;
    ix = hx & 0x7fffffff;
    /* |x| >= 1 or nan */
    if (ix >= 0x3ff00000) {
        unsigned int lx;

        lx = *(ULONGLONG*)&x;
        if (((ix - 0x3ff00000) | lx) == 0) {
            /* acos(1)=0, acos(-1)=pi */
            if (hx >> 31)
                return 2 * pio2_hi + 7.5231638452626401e-37;
            return 0;
        }
        if (isnan(x)) return x;
        return math_error(_DOMAIN, "acos", x, 0, 0 / (x - x));
    }
    /* |x| < 0.5 */
    if (ix < 0x3fe00000) {
        if (ix <= 0x3c600000)  /* |x| < 2**-57 */
            return pio2_hi + 7.5231638452626401e-37;
        return pio2_hi - (x - (pio2_lo - x * acos_R(x * x)));
    }
    /* x < -0.5 */
    if (hx >> 31) {
        z = (1.0 + x) * 0.5;
        s = sqrt(z);
        w = acos_R(z) * s - pio2_lo;
        return 2 * (pio2_hi - (s + w));
    }
    /* x > 0.5 */
    z = (1.0 - x) * 0.5;
    s = sqrt(z);
    df = s;
    llx = (*(ULONGLONG*)&df >> 32) << 32;
    df = *(double*)&llx;
    c = (z - df * df) / (s + df);
    w = acos_R(z) * s + c;
    return 2 * (df + w);
}

/*********************************************************************
 *		MSVCRT_asin (MSVCRT.@)
 *
 * Copied from musl: src/math/asin.c
 */
static double asin_R(double z)
{
    /* coefficients for R(x^2) */
    static const double pS0 =  1.66666666666666657415e-01,
                 pS1 = -3.25565818622400915405e-01,
                 pS2 =  2.01212532134862925881e-01,
                 pS3 = -4.00555345006794114027e-02,
                 pS4 =  7.91534994289814532176e-04,
                 pS5 =  3.47933107596021167570e-05,
                 qS1 = -2.40339491173441421878e+00,
                 qS2 =  2.02094576023350569471e+00,
                 qS3 = -6.88283971605453293030e-01,
                 qS4 =  7.70381505559019352791e-02;

    double p, q;
    p = z * (pS0 + z * (pS1 + z * (pS2 + z * (pS3 + z * (pS4 + z * pS5)))));
    q = 1.0 + z * (qS1 + z * (qS2 + z * (qS3 + z * qS4)));
    return p / q;
}

double CDECL MSVCRT_asin( double x )
{
    static const double pio2_hi = 1.57079632679489655800e+00,
                 pio2_lo = 6.12323399573676603587e-17;

    double z, r, s;
    unsigned int hx, ix;
    ULONGLONG llx;

    hx = *(ULONGLONG*)&x >> 32;
    ix = hx & 0x7fffffff;
    /* |x| >= 1 or nan */
    if (ix >= 0x3ff00000) {
        unsigned int lx;
        lx = *(ULONGLONG*)&x;
        if (((ix - 0x3ff00000) | lx) == 0)
            /* asin(1) = +-pi/2 with inexact */
            return x * pio2_hi + 7.5231638452626401e-37;
        if (isnan(x)) return x;
        return math_error(_DOMAIN, "asin", x, 0, 0 / (x - x));
    }
    /* |x| < 0.5 */
    if (ix < 0x3fe00000) {
        /* if 0x1p-1022 <= |x| < 0x1p-26, avoid raising underflow */
        if (ix < 0x3e500000 && ix >= 0x00100000)
            return x;
        return x + x * asin_R(x * x);
    }
    /* 1 > |x| >= 0.5 */
    z = (1 - fabs(x)) * 0.5;
    s = sqrt(z);
    r = asin_R(z);
    if (ix >= 0x3fef3333) {  /* if |x| > 0.975 */
        x = pio2_hi - (2 * (s + s * r) - pio2_lo);
    } else {
        double f, c;
        /* f+c = sqrt(z) */
        f = s;
        llx = (*(ULONGLONG*)&f >> 32) << 32;
        f = *(double*)&llx;
        c = (z - f * f) / (s + f);
        x = 0.5 * pio2_hi - (2 * s * r - (pio2_lo - 2 * c) - (0.5 * pio2_hi - 2 * f));
    }
    if (hx >> 31)
        return -x;
    return x;
}

/*********************************************************************
 *		MSVCRT_atan (MSVCRT.@)
 *
 * Copied from musl: src/math/atan.c
 */
double CDECL MSVCRT_atan( double x )
{
    static const double atanhi[] = {
        4.63647609000806093515e-01,
        7.85398163397448278999e-01,
        9.82793723247329054082e-01,
        1.57079632679489655800e+00,
    };
    static const double atanlo[] = {
        2.26987774529616870924e-17,
        3.06161699786838301793e-17,
        1.39033110312309984516e-17,
        6.12323399573676603587e-17,
    };
    static const double aT[] = {
        3.33333333333329318027e-01,
        -1.99999999998764832476e-01,
        1.42857142725034663711e-01,
        -1.11111104054623557880e-01,
        9.09088713343650656196e-02,
        -7.69187620504482999495e-02,
        6.66107313738753120669e-02,
        -5.83357013379057348645e-02,
        4.97687799461593236017e-02,
        -3.65315727442169155270e-02,
        1.62858201153657823623e-02,
    };

    double w, s1, s2, z;
    unsigned int ix, sign;
    int id;

#if _MSVCR_VER == 0
    if (isnan(x)) return math_error(_DOMAIN, "atan", x, 0, x);
#endif

    ix = *(ULONGLONG*)&x >> 32;
    sign = ix >> 31;
    ix &= 0x7fffffff;
    if (ix >= 0x44100000) {   /* if |x| >= 2^66 */
        if (isnan(x))
            return x;
        z = atanhi[3] + 7.5231638452626401e-37;
        return sign ? -z : z;
    }
    if (ix < 0x3fdc0000) {    /* |x| < 0.4375 */
        if (ix < 0x3e400000) {  /* |x| < 2^-27 */
            if (ix < 0x00100000)
                /* raise underflow for subnormal x */
                fp_barrierf((float)x);
            return x;
        }
        id = -1;
    } else {
        x = fabs(x);
        if (ix < 0x3ff30000) {  /* |x| < 1.1875 */
            if (ix < 0x3fe60000) {  /*  7/16 <= |x| < 11/16 */
                id = 0;
                x = (2.0 * x - 1.0) / (2.0 + x);
            } else {                /* 11/16 <= |x| < 19/16 */
                id = 1;
                x = (x - 1.0) / (x + 1.0);
            }
        } else {
            if (ix < 0x40038000) {  /* |x| < 2.4375 */
                id = 2;
                x = (x - 1.5) / (1.0 + 1.5 * x);
            } else {                /* 2.4375 <= |x| < 2^66 */
                id = 3;
                x = -1.0 / x;
            }
        }
    }
    /* end of argument reduction */
    z = x * x;
    w = z * z;
    /* break sum from i=0 to 10 aT[i]z**(i+1) into odd and even poly */
    s1 = z * (aT[0] + w * (aT[2] + w * (aT[4] + w * (aT[6] + w * (aT[8] + w * aT[10])))));
    s2 = w * (aT[1] + w * (aT[3] + w * (aT[5] + w * (aT[7] + w * aT[9]))));
    if (id < 0)
        return x - x * (s1 + s2);
    z = atanhi[id] - (x * (s1 + s2) - atanlo[id] - x);
    return sign ? -z : z;
}

/*********************************************************************
 *		MSVCRT_atan2 (MSVCRT.@)
 *
 * Copied from musl: src/math/atan2.c
 */
double CDECL MSVCRT_atan2( double y, double x )
{
    static const double pi     = 3.1415926535897931160E+00,
                 pi_lo  = 1.2246467991473531772E-16;

    double z;
    unsigned int m, lx, ly, ix, iy;

    if (isnan(x) || isnan(y))
        return x+y;
    ix = *(ULONGLONG*)&x >> 32;
    lx = *(ULONGLONG*)&x;
    iy = *(ULONGLONG*)&y >> 32;
    ly = *(ULONGLONG*)&y;
    if (((ix - 0x3ff00000) | lx) == 0)  /* x = 1.0 */
        return atan(y);
    m = ((iy >> 31) & 1) | ((ix >> 30) & 2);  /* 2*sign(x)+sign(y) */
    ix = ix & 0x7fffffff;
    iy = iy & 0x7fffffff;

    /* when y = 0 */
    if ((iy | ly) == 0) {
        switch(m) {
        case 0:
        case 1: return y;   /* atan(+-0,+anything)=+-0 */
        case 2: return pi;  /* atan(+0,-anything) = pi */
        case 3: return -pi; /* atan(-0,-anything) =-pi */
        }
    }
    /* when x = 0 */
    if ((ix | lx) == 0)
        return m & 1 ? -pi / 2 : pi / 2;
    /* when x is INF */
    if (ix == 0x7ff00000) {
        if (iy == 0x7ff00000) {
            switch(m) {
            case 0: return pi / 4;      /* atan(+INF,+INF) */
            case 1: return -pi / 4;     /* atan(-INF,+INF) */
            case 2: return 3 * pi / 4;  /* atan(+INF,-INF) */
            case 3: return -3 * pi / 4; /* atan(-INF,-INF) */
            }
        } else {
            switch(m) {
            case 0: return 0.0;  /* atan(+...,+INF) */
            case 1: return -0.0; /* atan(-...,+INF) */
            case 2: return pi;   /* atan(+...,-INF) */
            case 3: return -pi;  /* atan(-...,-INF) */
            }
        }
    }
    /* |y/x| > 0x1p64 */
    if (ix + (64 << 20) < iy || iy == 0x7ff00000)
        return m & 1 ? -pi / 2 : pi / 2;

    /* z = atan(|y/x|) without spurious underflow */
    if ((m & 2) && iy + (64 << 20) < ix)  /* |y/x| < 0x1p-64, x<0 */
        z = 0;
    else
        z = atan(fabs(y / x));
    switch (m) {
    case 0: return z;                /* atan(+,+) */
    case 1: return -z;               /* atan(-,+) */
    case 2: return pi - (z - pi_lo); /* atan(+,-) */
    default: /* case 3 */
        return (z - pi_lo) - pi;     /* atan(-,-) */
    }
}

/*********************************************************************
 *		MSVCRT_cos (MSVCRT.@)
 */
double CDECL MSVCRT_cos( double x )
{
  double ret = cos(x);
  if (!isfinite(x)) return math_error(_DOMAIN, "cos", x, 0, ret);
  return ret;
}

/*********************************************************************
 *		MSVCRT_cosh (MSVCRT.@)
 */
double CDECL MSVCRT_cosh( double x )
{
  double ret = cosh(x);
  if (isnan(x)) return math_error(_DOMAIN, "cosh", x, 0, ret);
  return ret;
}

/*********************************************************************
 *		MSVCRT_exp (MSVCRT.@)
 */
double CDECL MSVCRT_exp( double x )
{
  double ret = exp(x);
  if (isnan(x)) return math_error(_DOMAIN, "exp", x, 0, ret);
  if (isfinite(x) && !ret) return math_error(_UNDERFLOW, "exp", x, 0, ret);
  if (isfinite(x) && !isfinite(ret)) return math_error(_OVERFLOW, "exp", x, 0, ret);
  return ret;
}

/*********************************************************************
 *		MSVCRT_fmod (MSVCRT.@)
 */
double CDECL MSVCRT_fmod( double x, double y )
{
  double ret = fmod(x, y);
  if (!isfinite(x) || !isfinite(y)) return math_error(_DOMAIN, "fmod", x, y, ret);
  return ret;
}

/*********************************************************************
 *		MSVCRT_log (MSVCRT.@)
 */
double CDECL MSVCRT_log( double x )
{
  double ret = log(x);
  if (x < 0.0) return math_error(_DOMAIN, "log", x, 0, ret);
  if (x == 0.0) return math_error(_SING, "log", x, 0, ret);
  return ret;
}

/*********************************************************************
 *		MSVCRT_log10 (MSVCRT.@)
 */
double CDECL MSVCRT_log10( double x )
{
  double ret = log10(x);
  if (x < 0.0) return math_error(_DOMAIN, "log10", x, 0, ret);
  if (x == 0.0) return math_error(_SING, "log10", x, 0, ret);
  return ret;
}

/*********************************************************************
 *		MSVCRT_pow (MSVCRT.@)
 */
double CDECL MSVCRT_pow( double x, double y )
{
  double z = pow(x,y);
  if (x < 0 && y != floor(y))
    return math_error(_DOMAIN, "pow", x, y, z);
  if (!x && isfinite(y) && y < 0)
    return math_error(_SING, "pow", x, y, z);
  if (isfinite(x) && isfinite(y) && !isfinite(z))
    return math_error(_OVERFLOW, "pow", x, y, z);
  if (x && isfinite(x) && isfinite(y) && !z)
    return math_error(_UNDERFLOW, "pow", x, y, z);
  return z;
}

/*********************************************************************
 *		MSVCRT_sin (MSVCRT.@)
 */
double CDECL MSVCRT_sin( double x )
{
  double ret = sin(x);
  if (!isfinite(x)) return math_error(_DOMAIN, "sin", x, 0, ret);
  return ret;
}

/*********************************************************************
 *		MSVCRT_sinh (MSVCRT.@)
 */
double CDECL MSVCRT_sinh( double x )
{
  double ret = sinh(x);
  if (isnan(x)) return math_error(_DOMAIN, "sinh", x, 0, ret);
  return ret;
}

/*********************************************************************
 *		MSVCRT_sqrt (MSVCRT.@)
 *
 * Copied from musl: src/math/sqrt.c
 */
double CDECL MSVCRT_sqrt( double x )
{
    static const double tiny = 1.0e-300;

    double z;
    int sign = 0x80000000;
    int ix0,s0,q,m,t,i;
    unsigned int r,t1,s1,ix1,q1;
    ULONGLONG ix;

    ix = *(ULONGLONG*)&x;
    ix0 = ix >> 32;
    ix1 = ix;

    /* take care of Inf and NaN */
    if (isnan(x) || (isinf(x) && x > 0))
        return x;

    /* take care of zero */
    if (ix0 <= 0) {
        if (((ix0 & ~sign) | ix1) == 0)
            return x;  /* sqrt(+-0) = +-0 */
        if (ix0 < 0)
            return math_error(_DOMAIN, "sqrt", x, 0, (x - x) / (x - x));
    }
    /* normalize x */
    m = ix0 >> 20;
    if (m == 0) {  /* subnormal x */
        while (ix0 == 0) {
            m -= 21;
            ix0 |= (ix1 >> 11);
            ix1 <<= 21;
        }
        for (i=0; (ix0 & 0x00100000) == 0; i++)
            ix0 <<= 1;
        m -= i - 1;
        ix0 |= ix1 >> (32 - i);
        ix1 <<= i;
    }
    m -= 1023;    /* unbias exponent */
    ix0 = (ix0 & 0x000fffff) | 0x00100000;
    if (m & 1) {  /* odd m, double x to make it even */
        ix0 += ix0 + ((ix1 & sign) >> 31);
        ix1 += ix1;
    }
    m >>= 1;      /* m = [m/2] */

    /* generate sqrt(x) bit by bit */
    ix0 += ix0 + ((ix1 & sign) >> 31);
    ix1 += ix1;
    q = q1 = s0 = s1 = 0;  /* [q,q1] = sqrt(x) */
    r = 0x00200000;        /* r = moving bit from right to left */

    while (r != 0) {
        t = s0 + r;
        if (t <= ix0) {
            s0   = t + r;
            ix0 -= t;
            q   += r;
        }
        ix0 += ix0 + ((ix1 & sign) >> 31);
        ix1 += ix1;
        r >>= 1;
    }

    r = sign;
    while (r != 0) {
        t1 = s1 + r;
        t  = s0;
        if (t < ix0 || (t == ix0 && t1 <= ix1)) {
            s1 = t1 + r;
            if ((t1&sign) == sign && (s1 & sign) == 0)
                s0++;
            ix0 -= t;
            if (ix1 < t1)
                ix0--;
            ix1 -= t1;
            q1 += r;
        }
        ix0 += ix0 + ((ix1 & sign) >> 31);
        ix1 += ix1;
        r >>= 1;
    }

    /* use floating add to find out rounding direction */
    if ((ix0 | ix1) != 0) {
        z = 1.0 - tiny; /* raise inexact flag */
        if (z >= 1.0) {
            z = 1.0 + tiny;
            if (q1 == (unsigned int)0xffffffff) {
                q1 = 0;
                q++;
            } else if (z > 1.0) {
                if (q1 == (unsigned int)0xfffffffe)
                    q++;
                q1 += 2;
            } else
                q1 += q1 & 1;
        }
    }
    ix0 = (q >> 1) + 0x3fe00000;
    ix1 = q1 >> 1;
    if (q & 1)
        ix1 |= sign;
    ix = ix0 + ((unsigned int)m << 20);
    ix <<= 32;
    ix |= ix1;
    return *(double*)&ix;
}

/*********************************************************************
 *		MSVCRT_tan (MSVCRT.@)
 */
double CDECL MSVCRT_tan( double x )
{
  double ret = tan(x);
  if (!isfinite(x)) return math_error(_DOMAIN, "tan", x, 0, ret);
  return ret;
}

/*********************************************************************
 *		MSVCRT_tanh (MSVCRT.@)
 */
double CDECL MSVCRT_tanh( double x )
{
  double ret = tanh(x);
  if (isnan(x)) return math_error(_DOMAIN, "tanh", x, 0, ret);
  return ret;
}


#if defined(__GNUC__) && defined(__i386__)

#define CREATE_FPU_FUNC1(name, call) \
    __ASM_GLOBAL_FUNC(name, \
            "pushl   %ebp\n\t" \
            __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t") \
            __ASM_CFI(".cfi_rel_offset %ebp,0\n\t") \
            "movl    %esp, %ebp\n\t" \
            __ASM_CFI(".cfi_def_cfa_register %ebp\n\t") \
            "subl    $68, %esp\n\t" /* sizeof(double)*8 + sizeof(int) */ \
            "fstpl   (%esp)\n\t"    /* store function argument */ \
            "fwait\n\t" \
            "movl    $1, %ecx\n\t"  /* empty FPU stack */ \
            "1:\n\t" \
            "fxam\n\t" \
            "fstsw   %ax\n\t" \
            "and     $0x4500, %ax\n\t" \
            "cmp     $0x4100, %ax\n\t" \
            "je      2f\n\t" \
            "fstpl    (%esp,%ecx,8)\n\t" \
            "fwait\n\t" \
            "incl    %ecx\n\t" \
            "jmp     1b\n\t" \
            "2:\n\t" \
            "movl    %ecx, -4(%ebp)\n\t" \
            "call    " __ASM_NAME( #call ) "\n\t" \
            "movl    -4(%ebp), %ecx\n\t" \
            "fstpl   (%esp)\n\t"    /* save result */ \
            "3:\n\t"                /* restore FPU stack */ \
            "decl    %ecx\n\t" \
            "fldl    (%esp,%ecx,8)\n\t" \
            "cmpl    $0, %ecx\n\t" \
            "jne     3b\n\t" \
            "leave\n\t" \
            __ASM_CFI(".cfi_def_cfa %esp,4\n\t") \
            __ASM_CFI(".cfi_same_value %ebp\n\t") \
            "ret")

#define CREATE_FPU_FUNC2(name, call) \
    __ASM_GLOBAL_FUNC(name, \
            "pushl   %ebp\n\t" \
            __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t") \
            __ASM_CFI(".cfi_rel_offset %ebp,0\n\t") \
            "movl    %esp, %ebp\n\t" \
            __ASM_CFI(".cfi_def_cfa_register %ebp\n\t") \
            "subl    $68, %esp\n\t" /* sizeof(double)*8 + sizeof(int) */ \
            "fstpl   8(%esp)\n\t"   /* store function argument */ \
            "fwait\n\t" \
            "fstpl   (%esp)\n\t" \
            "fwait\n\t" \
            "movl    $2, %ecx\n\t"  /* empty FPU stack */ \
            "1:\n\t" \
            "fxam\n\t" \
            "fstsw   %ax\n\t" \
            "and     $0x4500, %ax\n\t" \
            "cmp     $0x4100, %ax\n\t" \
            "je      2f\n\t" \
            "fstpl    (%esp,%ecx,8)\n\t" \
            "fwait\n\t" \
            "incl    %ecx\n\t" \
            "jmp     1b\n\t" \
            "2:\n\t" \
            "movl    %ecx, -4(%ebp)\n\t" \
            "call    " __ASM_NAME( #call ) "\n\t" \
            "movl    -4(%ebp), %ecx\n\t" \
            "fstpl   8(%esp)\n\t"   /* save result */ \
            "3:\n\t"                /* restore FPU stack */ \
            "decl    %ecx\n\t" \
            "fldl    (%esp,%ecx,8)\n\t" \
            "cmpl    $1, %ecx\n\t" \
            "jne     3b\n\t" \
            "leave\n\t" \
            __ASM_CFI(".cfi_def_cfa %esp,4\n\t") \
            __ASM_CFI(".cfi_same_value %ebp\n\t") \
            "ret")

CREATE_FPU_FUNC1(_CIacos, MSVCRT_acos)
CREATE_FPU_FUNC1(_CIasin, MSVCRT_asin)
CREATE_FPU_FUNC1(_CIatan, MSVCRT_atan)
CREATE_FPU_FUNC2(_CIatan2, MSVCRT_atan2)
CREATE_FPU_FUNC1(_CIcos, MSVCRT_cos)
CREATE_FPU_FUNC1(_CIcosh, MSVCRT_cosh)
CREATE_FPU_FUNC1(_CIexp, MSVCRT_exp)
CREATE_FPU_FUNC2(_CIfmod, MSVCRT_fmod)
CREATE_FPU_FUNC1(_CIlog, MSVCRT_log)
CREATE_FPU_FUNC1(_CIlog10, MSVCRT_log10)
CREATE_FPU_FUNC2(_CIpow, MSVCRT_pow)
CREATE_FPU_FUNC1(_CIsin, MSVCRT_sin)
CREATE_FPU_FUNC1(_CIsinh, MSVCRT_sinh)
CREATE_FPU_FUNC1(_CIsqrt, MSVCRT_sqrt)
CREATE_FPU_FUNC1(_CItan, MSVCRT_tan)
CREATE_FPU_FUNC1(_CItanh, MSVCRT_tanh)

__ASM_GLOBAL_FUNC(MSVCRT__ftol,
        "pushl   %ebp\n\t"
        __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
        __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
        "movl    %esp, %ebp\n\t"
        __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
        "subl    $12, %esp\n\t"     /* sizeof(LONGLONG) + 2*sizeof(WORD) */
        "fnstcw  (%esp)\n\t"
        "mov     (%esp), %ax\n\t"
        "or      $0xc00, %ax\n\t"
        "mov     %ax, 2(%esp)\n\t"
        "fldcw   2(%esp)\n\t"
        "fistpq  4(%esp)\n\t"
        "fldcw   (%esp)\n\t"
        "movl    4(%esp), %eax\n\t"
        "movl    8(%esp), %edx\n\t"
        "leave\n\t"
        __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
        __ASM_CFI(".cfi_same_value %ebp\n\t")
        "ret")

#endif /* defined(__GNUC__) && defined(__i386__) */

/*********************************************************************
 *		_fpclass (MSVCRT.@)
 */
int CDECL MSVCRT__fpclass(double num)
{
    union { double f; UINT64 i; } u = { num };
    int e = u.i >> 52 & 0x7ff;
    int s = u.i >> 63;

    switch (e)
    {
    case 0:
        if (u.i << 1) return s ? MSVCRT__FPCLASS_ND : MSVCRT__FPCLASS_PD;
        return s ? MSVCRT__FPCLASS_NZ : MSVCRT__FPCLASS_PZ;
    case 0x7ff:
        if (u.i << 12) return ((u.i >> 51) & 1) ? MSVCRT__FPCLASS_QNAN : MSVCRT__FPCLASS_SNAN;
        return s ? MSVCRT__FPCLASS_NINF : MSVCRT__FPCLASS_PINF;
    default:
        return s ? MSVCRT__FPCLASS_NN : MSVCRT__FPCLASS_PN;
    }
}

/*********************************************************************
 *		_rotl (MSVCRT.@)
 */
unsigned int CDECL _rotl(unsigned int num, int shift)
{
  shift &= 31;
  return (num << shift) | (num >> (32-shift));
}

/*********************************************************************
 *		_lrotl (MSVCRT.@)
 */
MSVCRT_ulong CDECL MSVCRT__lrotl(MSVCRT_ulong num, int shift)
{
  shift &= 0x1f;
  return (num << shift) | (num >> (32-shift));
}

/*********************************************************************
 *		_lrotr (MSVCRT.@)
 */
MSVCRT_ulong CDECL MSVCRT__lrotr(MSVCRT_ulong num, int shift)
{
  shift &= 0x1f;
  return (num >> shift) | (num << (32-shift));
}

/*********************************************************************
 *		_rotr (MSVCRT.@)
 */
unsigned int CDECL _rotr(unsigned int num, int shift)
{
    shift &= 0x1f;
    return (num >> shift) | (num << (32-shift));
}

/*********************************************************************
 *		_rotl64 (MSVCRT.@)
 */
unsigned __int64 CDECL _rotl64(unsigned __int64 num, int shift)
{
  shift &= 63;
  return (num << shift) | (num >> (64-shift));
}

/*********************************************************************
 *		_rotr64 (MSVCRT.@)
 */
unsigned __int64 CDECL _rotr64(unsigned __int64 num, int shift)
{
    shift &= 63;
    return (num >> shift) | (num << (64-shift));
}

/*********************************************************************
 *		abs (MSVCRT.@)
 */
int CDECL MSVCRT_abs( int n )
{
    return n >= 0 ? n : -n;
}

/*********************************************************************
 *		labs (MSVCRT.@)
 */
MSVCRT_long CDECL MSVCRT_labs( MSVCRT_long n )
{
    return n >= 0 ? n : -n;
}

#if _MSVCR_VER>=100
/*********************************************************************
 *		llabs (MSVCR100.@)
 */
MSVCRT_longlong CDECL MSVCRT_llabs( MSVCRT_longlong n )
{
    return n >= 0 ? n : -n;
}
#endif

#if _MSVCR_VER>=120
/*********************************************************************
 *		imaxabs (MSVCR120.@)
 */
MSVCRT_intmax_t CDECL MSVCRT_imaxabs( MSVCRT_intmax_t n )
{
    return n >= 0 ? n : -n;
}
#endif

/*********************************************************************
 *		_abs64 (MSVCRT.@)
 */
__int64 CDECL _abs64( __int64 n )
{
    return n >= 0 ? n : -n;
}

/*********************************************************************
 *		_logb (MSVCRT.@)
 */
double CDECL MSVCRT__logb(double num)
{
  double ret = logb(num);
  if (isnan(num)) return math_error(_DOMAIN, "_logb", num, 0, ret);
  if (!num) return math_error(_SING, "_logb", num, 0, ret);
  return ret;
}

/*********************************************************************
 *		_hypot (MSVCRT.@)
 */
double CDECL _hypot(double x, double y)
{
  /* FIXME: errno handling */
  return hypot( x, y );
}

/*********************************************************************
 *      _hypotf (MSVCRT.@)
 */
float CDECL MSVCRT__hypotf(float x, float y)
{
  /* FIXME: errno handling */
  return hypotf( x, y );
}

/*********************************************************************
 *		ceil (MSVCRT.@)
 */
double CDECL MSVCRT_ceil( double x )
{
  return ceil(x);
}

/*********************************************************************
 *		floor (MSVCRT.@)
 */
double CDECL MSVCRT_floor( double x )
{
  return floor(x);
}

/*********************************************************************
 *      fma (MSVCRT.@)
 */
double CDECL MSVCRT_fma( double x, double y, double z )
{
#ifdef HAVE_FMA
  double w = fma(x, y, z);
#else
  double w = x * y + z;
#endif
  if ((isinf(x) && y == 0) || (x == 0 && isinf(y))) *MSVCRT__errno() = MSVCRT_EDOM;
  else if (isinf(x) && isinf(z) && x != z) *MSVCRT__errno() = MSVCRT_EDOM;
  else if (isinf(y) && isinf(z) && y != z) *MSVCRT__errno() = MSVCRT_EDOM;
  return w;
}

/*********************************************************************
 *      fmaf (MSVCRT.@)
 */
float CDECL MSVCRT_fmaf( float x, float y, float z )
{
#ifdef HAVE_FMAF
  float w = fmaf(x, y, z);
#else
  float w = x * y + z;
#endif
  if ((isinf(x) && y == 0) || (x == 0 && isinf(y))) *MSVCRT__errno() = MSVCRT_EDOM;
  else if (isinf(x) && isinf(z) && x != z) *MSVCRT__errno() = MSVCRT_EDOM;
  else if (isinf(y) && isinf(z) && y != z) *MSVCRT__errno() = MSVCRT_EDOM;
  return w;
}

/*********************************************************************
 *		fabs (MSVCRT.@)
 *
 * Copied from musl: src/math/fabsf.c
 */
double CDECL MSVCRT_fabs( double x )
{
    union { double f; UINT64 i; } u = { x };
    u.i &= ~0ull >> 1;
    return u.f;
}

/*********************************************************************
 *		frexp (MSVCRT.@)
 */
double CDECL MSVCRT_frexp( double x, int *exp )
{
  return frexp( x, exp );
}

/*********************************************************************
 *		modf (MSVCRT.@)
 */
double CDECL MSVCRT_modf( double x, double *iptr )
{
  return modf( x, iptr );
}

/**********************************************************************
 *		_statusfp2 (MSVCRT.@)
 *
 * Not exported by native msvcrt, added in msvcr80.
 */
#if defined(__i386__) || defined(__x86_64__)
void CDECL _statusfp2( unsigned int *x86_sw, unsigned int *sse2_sw )
{
#ifdef __GNUC__
    unsigned int flags;
    unsigned long fpword;

    if (x86_sw)
    {
        __asm__ __volatile__( "fstsw %0" : "=m" (fpword) );
        flags = 0;
        if (fpword & 0x1)  flags |= MSVCRT__SW_INVALID;
        if (fpword & 0x2)  flags |= MSVCRT__SW_DENORMAL;
        if (fpword & 0x4)  flags |= MSVCRT__SW_ZERODIVIDE;
        if (fpword & 0x8)  flags |= MSVCRT__SW_OVERFLOW;
        if (fpword & 0x10) flags |= MSVCRT__SW_UNDERFLOW;
        if (fpword & 0x20) flags |= MSVCRT__SW_INEXACT;
        *x86_sw = flags;
    }

    if (!sse2_sw) return;

    if (sse2_supported)
    {
        __asm__ __volatile__( "stmxcsr %0" : "=m" (fpword) );
        flags = 0;
        if (fpword & 0x1)  flags |= MSVCRT__SW_INVALID;
        if (fpword & 0x2)  flags |= MSVCRT__SW_DENORMAL;
        if (fpword & 0x4)  flags |= MSVCRT__SW_ZERODIVIDE;
        if (fpword & 0x8)  flags |= MSVCRT__SW_OVERFLOW;
        if (fpword & 0x10) flags |= MSVCRT__SW_UNDERFLOW;
        if (fpword & 0x20) flags |= MSVCRT__SW_INEXACT;
        *sse2_sw = flags;
    }
    else *sse2_sw = 0;
#else
    FIXME( "not implemented\n" );
#endif
}
#endif

/**********************************************************************
 *		_statusfp (MSVCRT.@)
 */
unsigned int CDECL _statusfp(void)
{
    unsigned int flags = 0;
#if defined(__i386__) || defined(__x86_64__)
    unsigned int x86_sw, sse2_sw;

    _statusfp2( &x86_sw, &sse2_sw );
    /* FIXME: there's no definition for ambiguous status, just return all status bits for now */
    flags = x86_sw | sse2_sw;
#elif defined(__aarch64__)
    unsigned long fpsr;

    __asm__ __volatile__( "mrs %0, fpsr" : "=r" (fpsr) );
    if (fpsr & 0x1)  flags |= MSVCRT__SW_INVALID;
    if (fpsr & 0x2)  flags |= MSVCRT__SW_ZERODIVIDE;
    if (fpsr & 0x4)  flags |= MSVCRT__SW_OVERFLOW;
    if (fpsr & 0x8)  flags |= MSVCRT__SW_UNDERFLOW;
    if (fpsr & 0x10) flags |= MSVCRT__SW_INEXACT;
    if (fpsr & 0x80) flags |= MSVCRT__SW_DENORMAL;
#else
    FIXME( "not implemented\n" );
#endif
    return flags;
}

/*********************************************************************
 *		_clearfp (MSVCRT.@)
 */
unsigned int CDECL _clearfp(void)
{
    unsigned int flags = 0;
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    unsigned long fpword;

    __asm__ __volatile__( "fnstsw %0; fnclex" : "=m" (fpword) );
    if (fpword & 0x1)  flags |= MSVCRT__SW_INVALID;
    if (fpword & 0x2)  flags |= MSVCRT__SW_DENORMAL;
    if (fpword & 0x4)  flags |= MSVCRT__SW_ZERODIVIDE;
    if (fpword & 0x8)  flags |= MSVCRT__SW_OVERFLOW;
    if (fpword & 0x10) flags |= MSVCRT__SW_UNDERFLOW;
    if (fpword & 0x20) flags |= MSVCRT__SW_INEXACT;

    if (sse2_supported)
    {
        __asm__ __volatile__( "stmxcsr %0" : "=m" (fpword) );
        if (fpword & 0x1)  flags |= MSVCRT__SW_INVALID;
        if (fpword & 0x2)  flags |= MSVCRT__SW_DENORMAL;
        if (fpword & 0x4)  flags |= MSVCRT__SW_ZERODIVIDE;
        if (fpword & 0x8)  flags |= MSVCRT__SW_OVERFLOW;
        if (fpword & 0x10) flags |= MSVCRT__SW_UNDERFLOW;
        if (fpword & 0x20) flags |= MSVCRT__SW_INEXACT;
        fpword &= ~0x3f;
        __asm__ __volatile__( "ldmxcsr %0" : : "m" (fpword) );
    }
#elif defined(__aarch64__)
    unsigned long fpsr;

    __asm__ __volatile__( "mrs %0, fpsr" : "=r" (fpsr) );
    if (fpsr & 0x1)  flags |= MSVCRT__SW_INVALID;
    if (fpsr & 0x2)  flags |= MSVCRT__SW_ZERODIVIDE;
    if (fpsr & 0x4)  flags |= MSVCRT__SW_OVERFLOW;
    if (fpsr & 0x8)  flags |= MSVCRT__SW_UNDERFLOW;
    if (fpsr & 0x10) flags |= MSVCRT__SW_INEXACT;
    if (fpsr & 0x80) flags |= MSVCRT__SW_DENORMAL;
    fpsr &= ~0x9f;
    __asm__ __volatile__( "msr fpsr, %0" :: "r" (fpsr) );
#else
    FIXME( "not implemented\n" );
#endif
    return flags;
}

/*********************************************************************
 *		__fpecode (MSVCRT.@)
 */
int * CDECL __fpecode(void)
{
    return &msvcrt_get_thread_data()->fpecode;
}

/*********************************************************************
 *		ldexp (MSVCRT.@)
 */
double CDECL MSVCRT_ldexp(double num, MSVCRT_long exp)
{
  double z = ldexp(num,exp);

  if (isfinite(num) && !isfinite(z))
    return math_error(_OVERFLOW, "ldexp", num, exp, z);
  if (num && isfinite(num) && !z)
    return math_error(_UNDERFLOW, "ldexp", num, exp, z);
  if (z == 0 && signbit(z))
    z = 0.0; /* Convert -0 -> +0 */
  return z;
}

/*********************************************************************
 *		_cabs (MSVCRT.@)
 */
double CDECL MSVCRT__cabs(struct MSVCRT__complex num)
{
  return sqrt(num.x * num.x + num.y * num.y);
}

/*********************************************************************
 *		_chgsign (MSVCRT.@)
 */
double CDECL MSVCRT__chgsign(double num)
{
    union { double f; UINT64 i; } u = { num };
    u.i ^= 1ull << 63;
    return u.f;
}

/*********************************************************************
 *		__control87_2 (MSVCR80.@)
 *
 * Not exported by native msvcrt, added in msvcr80.
 */
#ifdef __i386__
int CDECL __control87_2( unsigned int newval, unsigned int mask,
                         unsigned int *x86_cw, unsigned int *sse2_cw )
{
#ifdef __GNUC__
    unsigned long fpword;
    unsigned int flags;
    unsigned int old_flags;

    if (x86_cw)
    {
        __asm__ __volatile__( "fstcw %0" : "=m" (fpword) );

        /* Convert into mask constants */
        flags = 0;
        if (fpword & 0x1)  flags |= MSVCRT__EM_INVALID;
        if (fpword & 0x2)  flags |= MSVCRT__EM_DENORMAL;
        if (fpword & 0x4)  flags |= MSVCRT__EM_ZERODIVIDE;
        if (fpword & 0x8)  flags |= MSVCRT__EM_OVERFLOW;
        if (fpword & 0x10) flags |= MSVCRT__EM_UNDERFLOW;
        if (fpword & 0x20) flags |= MSVCRT__EM_INEXACT;
        switch (fpword & 0xc00)
        {
        case 0xc00: flags |= MSVCRT__RC_UP|MSVCRT__RC_DOWN; break;
        case 0x800: flags |= MSVCRT__RC_UP; break;
        case 0x400: flags |= MSVCRT__RC_DOWN; break;
        }
        switch (fpword & 0x300)
        {
        case 0x0:   flags |= MSVCRT__PC_24; break;
        case 0x200: flags |= MSVCRT__PC_53; break;
        case 0x300: flags |= MSVCRT__PC_64; break;
        }
        if (fpword & 0x1000) flags |= MSVCRT__IC_AFFINE;

        TRACE( "x86 flags=%08x newval=%08x mask=%08x\n", flags, newval, mask );
        if (mask)
        {
            flags = (flags & ~mask) | (newval & mask);

            /* Convert (masked) value back to fp word */
            fpword = 0;
            if (flags & MSVCRT__EM_INVALID)    fpword |= 0x1;
            if (flags & MSVCRT__EM_DENORMAL)   fpword |= 0x2;
            if (flags & MSVCRT__EM_ZERODIVIDE) fpword |= 0x4;
            if (flags & MSVCRT__EM_OVERFLOW)   fpword |= 0x8;
            if (flags & MSVCRT__EM_UNDERFLOW)  fpword |= 0x10;
            if (flags & MSVCRT__EM_INEXACT)    fpword |= 0x20;
            switch (flags & MSVCRT__MCW_RC)
            {
            case MSVCRT__RC_UP|MSVCRT__RC_DOWN: fpword |= 0xc00; break;
            case MSVCRT__RC_UP:                 fpword |= 0x800; break;
            case MSVCRT__RC_DOWN:               fpword |= 0x400; break;
            }
            switch (flags & MSVCRT__MCW_PC)
            {
            case MSVCRT__PC_64: fpword |= 0x300; break;
            case MSVCRT__PC_53: fpword |= 0x200; break;
            case MSVCRT__PC_24: fpword |= 0x0; break;
            }
            if (flags & MSVCRT__IC_AFFINE) fpword |= 0x1000;

            __asm__ __volatile__( "fldcw %0" : : "m" (fpword) );
        }
        *x86_cw = flags;
    }

    if (!sse2_cw) return 1;

    if (sse2_supported)
    {
        __asm__ __volatile__( "stmxcsr %0" : "=m" (fpword) );

        /* Convert into mask constants */
        flags = 0;
        if (fpword & 0x80)   flags |= MSVCRT__EM_INVALID;
        if (fpword & 0x100)  flags |= MSVCRT__EM_DENORMAL;
        if (fpword & 0x200)  flags |= MSVCRT__EM_ZERODIVIDE;
        if (fpword & 0x400)  flags |= MSVCRT__EM_OVERFLOW;
        if (fpword & 0x800)  flags |= MSVCRT__EM_UNDERFLOW;
        if (fpword & 0x1000) flags |= MSVCRT__EM_INEXACT;
        switch (fpword & 0x6000)
        {
        case 0x6000: flags |= MSVCRT__RC_UP|MSVCRT__RC_DOWN; break;
        case 0x4000: flags |= MSVCRT__RC_UP; break;
        case 0x2000: flags |= MSVCRT__RC_DOWN; break;
        }
        switch (fpword & 0x8040)
        {
        case 0x0040: flags |= MSVCRT__DN_FLUSH_OPERANDS_SAVE_RESULTS; break;
        case 0x8000: flags |= MSVCRT__DN_SAVE_OPERANDS_FLUSH_RESULTS; break;
        case 0x8040: flags |= MSVCRT__DN_FLUSH; break;
        }

        TRACE( "sse2 flags=%08x newval=%08x mask=%08x\n", flags, newval, mask );
        if (mask)
        {
            old_flags = flags;
            mask &= MSVCRT__MCW_EM | MSVCRT__MCW_RC | MSVCRT__MCW_DN;
            flags = (flags & ~mask) | (newval & mask);

            if (flags != old_flags)
            {
                /* Convert (masked) value back to fp word */
                fpword = 0;
                if (flags & MSVCRT__EM_INVALID)    fpword |= 0x80;
                if (flags & MSVCRT__EM_DENORMAL)   fpword |= 0x100;
                if (flags & MSVCRT__EM_ZERODIVIDE) fpword |= 0x200;
                if (flags & MSVCRT__EM_OVERFLOW)   fpword |= 0x400;
                if (flags & MSVCRT__EM_UNDERFLOW)  fpword |= 0x800;
                if (flags & MSVCRT__EM_INEXACT)    fpword |= 0x1000;
                switch (flags & MSVCRT__MCW_RC)
                {
                case MSVCRT__RC_UP|MSVCRT__RC_DOWN: fpword |= 0x6000; break;
                case MSVCRT__RC_UP:                 fpword |= 0x4000; break;
                case MSVCRT__RC_DOWN:               fpword |= 0x2000; break;
                }
                switch (flags & MSVCRT__MCW_DN)
                {
                case MSVCRT__DN_FLUSH_OPERANDS_SAVE_RESULTS: fpword |= 0x0040; break;
                case MSVCRT__DN_SAVE_OPERANDS_FLUSH_RESULTS: fpword |= 0x8000; break;
                case MSVCRT__DN_FLUSH:                       fpword |= 0x8040; break;
                }
                __asm__ __volatile__( "ldmxcsr %0" : : "m" (fpword) );
            }
        }
        *sse2_cw = flags;
    }
    else *sse2_cw = 0;

    return 1;
#else
    FIXME( "not implemented\n" );
    return 0;
#endif
}
#endif

/*********************************************************************
 *		_control87 (MSVCRT.@)
 */
unsigned int CDECL _control87(unsigned int newval, unsigned int mask)
{
    unsigned int flags = 0;
#ifdef __i386__
    unsigned int sse2_cw;

    __control87_2( newval, mask, &flags, &sse2_cw );

    if ((flags ^ sse2_cw) & (MSVCRT__MCW_EM | MSVCRT__MCW_RC)) flags |= MSVCRT__EM_AMBIGUOUS;
    flags |= sse2_cw;
#elif defined(__x86_64__)
    unsigned long fpword;
    unsigned int old_flags;

    __asm__ __volatile__( "stmxcsr %0" : "=m" (fpword) );
    if (fpword & 0x80)   flags |= MSVCRT__EM_INVALID;
    if (fpword & 0x100)  flags |= MSVCRT__EM_DENORMAL;
    if (fpword & 0x200)  flags |= MSVCRT__EM_ZERODIVIDE;
    if (fpword & 0x400)  flags |= MSVCRT__EM_OVERFLOW;
    if (fpword & 0x800)  flags |= MSVCRT__EM_UNDERFLOW;
    if (fpword & 0x1000) flags |= MSVCRT__EM_INEXACT;
    switch (fpword & 0x6000)
    {
    case 0x6000: flags |= MSVCRT__RC_CHOP; break;
    case 0x4000: flags |= MSVCRT__RC_UP; break;
    case 0x2000: flags |= MSVCRT__RC_DOWN; break;
    }
    switch (fpword & 0x8040)
    {
    case 0x0040: flags |= MSVCRT__DN_FLUSH_OPERANDS_SAVE_RESULTS; break;
    case 0x8000: flags |= MSVCRT__DN_SAVE_OPERANDS_FLUSH_RESULTS; break;
    case 0x8040: flags |= MSVCRT__DN_FLUSH; break;
    }
    old_flags = flags;
    mask &= MSVCRT__MCW_EM | MSVCRT__MCW_RC | MSVCRT__MCW_DN;
    flags = (flags & ~mask) | (newval & mask);
    if (flags != old_flags)
    {
        fpword = 0;
        if (flags & MSVCRT__EM_INVALID)    fpword |= 0x80;
        if (flags & MSVCRT__EM_DENORMAL)   fpword |= 0x100;
        if (flags & MSVCRT__EM_ZERODIVIDE) fpword |= 0x200;
        if (flags & MSVCRT__EM_OVERFLOW)   fpword |= 0x400;
        if (flags & MSVCRT__EM_UNDERFLOW)  fpword |= 0x800;
        if (flags & MSVCRT__EM_INEXACT)    fpword |= 0x1000;
        switch (flags & MSVCRT__MCW_RC)
        {
        case MSVCRT__RC_CHOP: fpword |= 0x6000; break;
        case MSVCRT__RC_UP:   fpword |= 0x4000; break;
        case MSVCRT__RC_DOWN: fpword |= 0x2000; break;
        }
        switch (flags & MSVCRT__MCW_DN)
        {
        case MSVCRT__DN_FLUSH_OPERANDS_SAVE_RESULTS: fpword |= 0x0040; break;
        case MSVCRT__DN_SAVE_OPERANDS_FLUSH_RESULTS: fpword |= 0x8000; break;
        case MSVCRT__DN_FLUSH:                       fpword |= 0x8040; break;
        }
        __asm__ __volatile__( "ldmxcsr %0" :: "m" (fpword) );
    }
#elif defined(__aarch64__)
    unsigned long fpcr;

    __asm__ __volatile__( "mrs %0, fpcr" : "=r" (fpcr) );
    if (!(fpcr & 0x100))  flags |= MSVCRT__EM_INVALID;
    if (!(fpcr & 0x200))  flags |= MSVCRT__EM_ZERODIVIDE;
    if (!(fpcr & 0x400))  flags |= MSVCRT__EM_OVERFLOW;
    if (!(fpcr & 0x800))  flags |= MSVCRT__EM_UNDERFLOW;
    if (!(fpcr & 0x1000)) flags |= MSVCRT__EM_INEXACT;
    if (!(fpcr & 0x8000)) flags |= MSVCRT__EM_DENORMAL;
    switch (fpcr & 0xc00000)
    {
    case 0x400000: flags |= MSVCRT__RC_UP; break;
    case 0x800000: flags |= MSVCRT__RC_DOWN; break;
    case 0xc00000: flags |= MSVCRT__RC_CHOP; break;
    }
    flags = (flags & ~mask) | (newval & mask);
    fpcr &= ~0xc09f00ul;
    if (!(flags & MSVCRT__EM_INVALID)) fpcr |= 0x100;
    if (!(flags & MSVCRT__EM_ZERODIVIDE)) fpcr |= 0x200;
    if (!(flags & MSVCRT__EM_OVERFLOW)) fpcr |= 0x400;
    if (!(flags & MSVCRT__EM_UNDERFLOW)) fpcr |= 0x800;
    if (!(flags & MSVCRT__EM_INEXACT)) fpcr |= 0x1000;
    if (!(flags & MSVCRT__EM_DENORMAL)) fpcr |= 0x8000;
    switch (flags & MSVCRT__MCW_RC)
    {
    case MSVCRT__RC_CHOP: fpcr |= 0xc00000; break;
    case MSVCRT__RC_UP:   fpcr |= 0x400000; break;
    case MSVCRT__RC_DOWN: fpcr |= 0x800000; break;
    }
    __asm__ __volatile__( "msr fpcr, %0" :: "r" (fpcr) );
#else
    FIXME( "not implemented\n" );
#endif
    return flags;
}

/*********************************************************************
 *		_controlfp (MSVCRT.@)
 */
unsigned int CDECL _controlfp(unsigned int newval, unsigned int mask)
{
  return _control87( newval, mask & ~MSVCRT__EM_DENORMAL );
}

/*********************************************************************
 *		_set_controlfp (MSVCRT.@)
 */
void CDECL _set_controlfp( unsigned int newval, unsigned int mask )
{
    _controlfp( newval, mask );
}

/*********************************************************************
 *              _controlfp_s (MSVCRT.@)
 */
int CDECL _controlfp_s(unsigned int *cur, unsigned int newval, unsigned int mask)
{
    static const unsigned int all_flags = (MSVCRT__MCW_EM | MSVCRT__MCW_IC | MSVCRT__MCW_RC |
                                           MSVCRT__MCW_PC | MSVCRT__MCW_DN);
    unsigned int val;

    if (!MSVCRT_CHECK_PMT( !(newval & mask & ~all_flags) ))
    {
        if (cur) *cur = _controlfp( 0, 0 );  /* retrieve it anyway */
        return MSVCRT_EINVAL;
    }
    val = _controlfp( newval, mask );
    if (cur) *cur = val;
    return 0;
}

#if _MSVCR_VER>=120
/*********************************************************************
 *		fegetenv (MSVCR120.@)
 */
int CDECL MSVCRT_fegetenv(MSVCRT_fenv_t *env)
{
    env->control = _controlfp(0, 0) & (MSVCRT__EM_INEXACT | MSVCRT__EM_UNDERFLOW |
            MSVCRT__EM_OVERFLOW | MSVCRT__EM_ZERODIVIDE | MSVCRT__EM_INVALID);
    env->status = _statusfp();
    return 0;
}
#endif

#if _MSVCR_VER>=140
/*********************************************************************
 *		__fpe_flt_rounds (UCRTBASE.@)
 */
int CDECL __fpe_flt_rounds(void)
{
    unsigned int fpc = _controlfp(0, 0) & MSVCRT__RC_CHOP;

    TRACE("()\n");

    switch(fpc) {
        case MSVCRT__RC_CHOP: return 0;
        case MSVCRT__RC_NEAR: return 1;
        case MSVCRT__RC_UP: return 2;
        default: return 3;
    }
}
#endif

#if _MSVCR_VER>=120

/*********************************************************************
 *		fegetround (MSVCR120.@)
 */
int CDECL MSVCRT_fegetround(void)
{
    return _controlfp(0, 0) & MSVCRT__RC_CHOP;
}

/*********************************************************************
 *		fesetround (MSVCR120.@)
 */
int CDECL MSVCRT_fesetround(int round_mode)
{
    if (round_mode & (~MSVCRT__RC_CHOP))
        return 1;
    _controlfp(round_mode, MSVCRT__RC_CHOP);
    return 0;
}

#endif /* _MSVCR_VER>=120 */

/*********************************************************************
 *		_copysign (MSVCRT.@)
 *
 * Copied from musl: src/math/copysign.c
 */
double CDECL MSVCRT__copysign( double x, double y )
{
    union { double f; UINT64 i; } ux = { x }, uy = { y };
    ux.i &= ~0ull >> 1;
    ux.i |= uy.i & 1ull << 63;
    return ux.f;
}

/*********************************************************************
 *		_finite (MSVCRT.@)
 */
int CDECL MSVCRT__finite(double num)
{
    union { double f; UINT64 i; } u = { num };
    return (u.i & ~0ull >> 1) < 0x7ffull << 52;
}

/*********************************************************************
 *		_fpreset (MSVCRT.@)
 */
void CDECL _fpreset(void)
{
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    const unsigned int x86_cw = 0x27f;
    __asm__ __volatile__( "fninit; fldcw %0" : : "m" (x86_cw) );
    if (sse2_supported)
    {
        const unsigned long sse2_cw = 0x1f80;
        __asm__ __volatile__( "ldmxcsr %0" : : "m" (sse2_cw) );
    }
#else
    FIXME( "not implemented\n" );
#endif
}

#if _MSVCR_VER>=120
/*********************************************************************
 *              fesetenv (MSVCR120.@)
 */
int CDECL MSVCRT_fesetenv(const MSVCRT_fenv_t *env)
{
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    struct {
        WORD control_word;
        WORD unused1;
        WORD status_word;
        WORD unused2;
        WORD tag_word;
        WORD unused3;
        DWORD instruction_pointer;
        WORD code_segment;
        WORD unused4;
        DWORD operand_addr;
        WORD data_segment;
        WORD unused5;
    } fenv;

    TRACE( "(%p)\n", env );

    if (!env->control && !env->status) {
        _fpreset();
        return 0;
    }

    __asm__ __volatile__( "fnstenv %0" : "=m" (fenv) );

    fenv.control_word &= ~0x3d;
    if (env->control & MSVCRT__EM_INVALID) fenv.control_word |= 0x1;
    if (env->control & MSVCRT__EM_ZERODIVIDE) fenv.control_word |= 0x4;
    if (env->control & MSVCRT__EM_OVERFLOW) fenv.control_word |= 0x8;
    if (env->control & MSVCRT__EM_UNDERFLOW) fenv.control_word |= 0x10;
    if (env->control & MSVCRT__EM_INEXACT) fenv.control_word |= 0x20;

    fenv.status_word &= ~0x3d;
    if (env->status & MSVCRT__SW_INVALID) fenv.status_word |= 0x1;
    if (env->status & MSVCRT__SW_ZERODIVIDE) fenv.status_word |= 0x4;
    if (env->status & MSVCRT__SW_OVERFLOW) fenv.status_word |= 0x8;
    if (env->status & MSVCRT__SW_UNDERFLOW) fenv.status_word |= 0x10;
    if (env->status & MSVCRT__SW_INEXACT) fenv.status_word |= 0x20;

    __asm__ __volatile__( "fldenv %0" : : "m" (fenv) : "st", "st(1)",
            "st(2)", "st(3)", "st(4)", "st(5)", "st(6)", "st(7)" );

    if (sse2_supported)
    {
        DWORD fpword;

        __asm__ __volatile__( "stmxcsr %0" : "=m" (fpword) );
        fpword &= ~0x1e80;
        if (env->control & MSVCRT__EM_INVALID) fpword |= 0x80;
        if (env->control & MSVCRT__EM_ZERODIVIDE) fpword |= 0x200;
        if (env->control & MSVCRT__EM_OVERFLOW) fpword |= 0x400;
        if (env->control & MSVCRT__EM_UNDERFLOW) fpword |= 0x800;
        if (env->control & MSVCRT__EM_INEXACT) fpword |= 0x1000;
        __asm__ __volatile__( "ldmxcsr %0" : : "m" (fpword) );
    }

    return 0;
#else
    FIXME( "not implemented\n" );
#endif
    return 1;
}
#endif

/*********************************************************************
 *		_isnan (MSVCRT.@)
 */
int CDECL MSVCRT__isnan(double num)
{
    union { double f; UINT64 i; } u = { num };
    return (u.i & ~0ull >> 1) > 0x7ffull << 52;
}

/*********************************************************************
 *		_j0 (MSVCRT.@)
 */
double CDECL MSVCRT__j0(double num)
{
  /* FIXME: errno handling */
#ifdef HAVE_J0
  return j0(num);
#else
  FIXME("not implemented\n");
  return 0;
#endif
}

/*********************************************************************
 *		_j1 (MSVCRT.@)
 */
double CDECL MSVCRT__j1(double num)
{
  /* FIXME: errno handling */
#ifdef HAVE_J1
  return j1(num);
#else
  FIXME("not implemented\n");
  return 0;
#endif
}

/*********************************************************************
 *		_jn (MSVCRT.@)
 */
double CDECL MSVCRT__jn(int n, double num)
{
  /* FIXME: errno handling */
#ifdef HAVE_JN
  return jn(n, num);
#else
  FIXME("not implemented\n");
  return 0;
#endif
}

/*********************************************************************
 *		_y0 (MSVCRT.@)
 */
double CDECL MSVCRT__y0(double num)
{
  double retval;
  if (!isfinite(num)) *MSVCRT__errno() = MSVCRT_EDOM;
#ifdef HAVE_Y0
  retval  = y0(num);
  if (MSVCRT__fpclass(retval) == MSVCRT__FPCLASS_NINF)
  {
    *MSVCRT__errno() = MSVCRT_EDOM;
    retval = NAN;
  }
#else
  FIXME("not implemented\n");
  retval = 0;
#endif
  return retval;
}

/*********************************************************************
 *		_y1 (MSVCRT.@)
 */
double CDECL MSVCRT__y1(double num)
{
  double retval;
  if (!isfinite(num)) *MSVCRT__errno() = MSVCRT_EDOM;
#ifdef HAVE_Y1
  retval  = y1(num);
  if (MSVCRT__fpclass(retval) == MSVCRT__FPCLASS_NINF)
  {
    *MSVCRT__errno() = MSVCRT_EDOM;
    retval = NAN;
  }
#else
  FIXME("not implemented\n");
  retval = 0;
#endif
  return retval;
}

/*********************************************************************
 *		_yn (MSVCRT.@)
 */
double CDECL MSVCRT__yn(int order, double num)
{
  double retval;
  if (!isfinite(num)) *MSVCRT__errno() = MSVCRT_EDOM;
#ifdef HAVE_YN
  retval  = yn(order,num);
  if (MSVCRT__fpclass(retval) == MSVCRT__FPCLASS_NINF)
  {
    *MSVCRT__errno() = MSVCRT_EDOM;
    retval = NAN;
  }
#else
  FIXME("not implemented\n");
  retval = 0;
#endif
  return retval;
}

#if _MSVCR_VER>=120

/*********************************************************************
 *		_nearbyint (MSVCR120.@)
 */
double CDECL MSVCRT_nearbyint(double num)
{
#ifdef HAVE_NEARBYINT
    return nearbyint(num);
#else
    return num >= 0 ? floor(num + 0.5) : ceil(num - 0.5);
#endif
}

/*********************************************************************
 *		_nearbyintf (MSVCR120.@)
 */
float CDECL MSVCRT_nearbyintf(float num)
{
#ifdef HAVE_NEARBYINTF
    return nearbyintf(num);
#else
    return MSVCRT_nearbyint(num);
#endif
}

/*********************************************************************
 *              nexttoward (MSVCR120.@)
 */
double CDECL MSVCRT_nexttoward(double num, double next)
{
#ifdef HAVE_NEXTTOWARD
    double ret = nexttoward(num, next);
    if (!(MSVCRT__fpclass(ret) & (MSVCRT__FPCLASS_PN | MSVCRT__FPCLASS_NN
            | MSVCRT__FPCLASS_SNAN | MSVCRT__FPCLASS_QNAN)) && !isinf(num))
    {
        *MSVCRT__errno() = MSVCRT_ERANGE;
    }
    return ret;
#else
    FIXME("not implemented\n");
    return 0;
#endif
}

/*********************************************************************
 *              nexttowardf (MSVCR120.@)
 */
float CDECL MSVCRT_nexttowardf(float num, double next)
{
#ifdef HAVE_NEXTTOWARDF
    float ret = nexttowardf(num, next);
    if (!(MSVCRT__fpclass(ret) & (MSVCRT__FPCLASS_PN | MSVCRT__FPCLASS_NN
            | MSVCRT__FPCLASS_SNAN | MSVCRT__FPCLASS_QNAN)) && !isinf(num))
    {
        *MSVCRT__errno() = MSVCRT_ERANGE;
    }
    return ret;
#else
    FIXME("not implemented\n");
    return 0;
#endif
}

#endif /* _MSVCR_VER>=120 */

/*********************************************************************
 *		_nextafter (MSVCRT.@)
 */
double CDECL MSVCRT__nextafter(double num, double next)
{
  double retval;
  if (!isfinite(num) || !isfinite(next)) *MSVCRT__errno() = MSVCRT_EDOM;
  retval = nextafter(num,next);
  return retval;
}

/*********************************************************************
 *		_ecvt (MSVCRT.@)
 */
char * CDECL MSVCRT__ecvt( double number, int ndigits, int *decpt, int *sign )
{
    int prec, len;
    thread_data_t *data = msvcrt_get_thread_data();
    /* FIXME: check better for overflow (native supports over 300 chars) */
    ndigits = min( ndigits, 80 - 7); /* 7 : space for dec point, 1 for "e",
                                      * 4 for exponent and one for
                                      * terminating '\0' */
    if (!data->efcvt_buffer)
        data->efcvt_buffer = MSVCRT_malloc( 80 ); /* ought to be enough */

    if( number < 0) {
        *sign = TRUE;
        number = -number;
    } else
        *sign = FALSE;
    /* handle cases with zero ndigits or less */
    prec = ndigits;
    if( prec < 1) prec = 2;
    len = MSVCRT__snprintf(data->efcvt_buffer, 80, "%.*le", prec - 1, number);
    /* take the decimal "point away */
    if( prec != 1)
        memmove( data->efcvt_buffer + 1, data->efcvt_buffer + 2, len - 1 );
    /* take the exponential "e" out */
    data->efcvt_buffer[ prec] = '\0';
    /* read the exponent */
    sscanf( data->efcvt_buffer + prec + 1, "%d", decpt);
    (*decpt)++;
    /* adjust for some border cases */
    if( data->efcvt_buffer[0] == '0')/* value is zero */
        *decpt = 0;
    /* handle cases with zero ndigits or less */
    if( ndigits < 1){
        if( data->efcvt_buffer[ 0] >= '5')
            (*decpt)++;
        data->efcvt_buffer[ 0] = '\0';
    }
    TRACE("out=\"%s\"\n",data->efcvt_buffer);
    return data->efcvt_buffer;
}

/*********************************************************************
 *		_ecvt_s (MSVCRT.@)
 */
int CDECL MSVCRT__ecvt_s( char *buffer, MSVCRT_size_t length, double number, int ndigits, int *decpt, int *sign )
{
    int prec, len;
    char *result;
    const char infret[] = "1#INF";

    if (!MSVCRT_CHECK_PMT(buffer != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(decpt != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(sign != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT_ERR( length > 2, MSVCRT_ERANGE )) return MSVCRT_ERANGE;
    if (!MSVCRT_CHECK_PMT_ERR(ndigits < (int)length - 1, MSVCRT_ERANGE )) return MSVCRT_ERANGE;

    /* special case - inf */
    if(number == HUGE_VAL || number == -HUGE_VAL)
    {
        memset(buffer, '0', ndigits);
        memcpy(buffer, infret, min(ndigits, sizeof(infret) - 1 ) );
        buffer[ndigits] = '\0';
        (*decpt) = 1;
        if(number == -HUGE_VAL)
            (*sign) = 1;
        else
            (*sign) = 0;
        return 0;
    }
    /* handle cases with zero ndigits or less */
    prec = ndigits;
    if( prec < 1) prec = 2;
    result = MSVCRT_malloc(prec + 7);

    if( number < 0) {
        *sign = TRUE;
        number = -number;
    } else
        *sign = FALSE;
    len = MSVCRT__snprintf(result, prec + 7, "%.*le", prec - 1, number);
    /* take the decimal "point away */
    if( prec != 1)
        memmove( result + 1, result + 2, len - 1 );
    /* take the exponential "e" out */
    result[ prec] = '\0';
    /* read the exponent */
    sscanf( result + prec + 1, "%d", decpt);
    (*decpt)++;
    /* adjust for some border cases */
    if( result[0] == '0')/* value is zero */
        *decpt = 0;
    /* handle cases with zero ndigits or less */
    if( ndigits < 1){
        if( result[ 0] >= '5')
            (*decpt)++;
        result[ 0] = '\0';
    }
    memcpy( buffer, result, max(ndigits + 1, 1) );
    MSVCRT_free( result );
    return 0;
}

/***********************************************************************
 *		_fcvt  (MSVCRT.@)
 */
char * CDECL MSVCRT__fcvt( double number, int ndigits, int *decpt, int *sign )
{
    thread_data_t *data = msvcrt_get_thread_data();
    int stop, dec1, dec2;
    char *ptr1, *ptr2, *first;
    char buf[80]; /* ought to be enough */
    char decimal_separator = get_locinfo()->lconv->decimal_point[0];

    if (!data->efcvt_buffer)
        data->efcvt_buffer = MSVCRT_malloc( 80 ); /* ought to be enough */

    if (number < 0)
    {
	*sign = 1;
	number = -number;
    } else *sign = 0;

    stop = MSVCRT__snprintf(buf, 80, "%.*f", ndigits < 0 ? 0 : ndigits, number);
    ptr1 = buf;
    ptr2 = data->efcvt_buffer;
    first = NULL;
    dec1 = 0;
    dec2 = 0;

    /* For numbers below the requested resolution, work out where
       the decimal point will be rather than finding it in the string */
    if (number < 1.0 && number > 0.0) {
	dec2 = log10(number + 1e-10);
	if (-dec2 <= ndigits) dec2 = 0;
    }

    /* If requested digits is zero or less, we will need to truncate
     * the returned string */
    if (ndigits < 1) {
	stop += ndigits;
    }

    while (*ptr1 == '0') ptr1++; /* Skip leading zeroes */
    while (*ptr1 != '\0' && *ptr1 != decimal_separator) {
	if (!first) first = ptr2;
	if ((ptr1 - buf) < stop) {
	    *ptr2++ = *ptr1++;
	} else {
	    ptr1++;
	}
	dec1++;
    }

    if (ndigits > 0) {
	ptr1++;
	if (!first) {
	    while (*ptr1 == '0') { /* Process leading zeroes */
		*ptr2++ = *ptr1++;
		dec1--;
	    }
	}
	while (*ptr1 != '\0') {
	    if (!first) first = ptr2;
	    *ptr2++ = *ptr1++;
	}
    }

    *ptr2 = '\0';

    /* We never found a non-zero digit, then our number is either
     * smaller than the requested precision, or 0.0 */
    if (!first) {
	if (number > 0.0) {
	    first = ptr2;
	} else {
	    first = data->efcvt_buffer;
	    dec1 = 0;
	}
    }

    *decpt = dec2 ? dec2 : dec1;
    return first;
}

/***********************************************************************
 *		_fcvt_s  (MSVCRT.@)
 */
int CDECL MSVCRT__fcvt_s(char* outbuffer, MSVCRT_size_t size, double number, int ndigits, int *decpt, int *sign)
{
    int stop, dec1, dec2;
    char *ptr1, *ptr2, *first;
    char buf[80]; /* ought to be enough */
    char decimal_separator = get_locinfo()->lconv->decimal_point[0];

    if (!outbuffer || !decpt || !sign || size == 0)
    {
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return MSVCRT_EINVAL;
    }

    if (number < 0)
    {
	*sign = 1;
	number = -number;
    } else *sign = 0;

    stop = MSVCRT__snprintf(buf, 80, "%.*f", ndigits < 0 ? 0 : ndigits, number);
    ptr1 = buf;
    ptr2 = outbuffer;
    first = NULL;
    dec1 = 0;
    dec2 = 0;

    /* For numbers below the requested resolution, work out where
       the decimal point will be rather than finding it in the string */
    if (number < 1.0 && number > 0.0) {
	dec2 = log10(number + 1e-10);
	if (-dec2 <= ndigits) dec2 = 0;
    }

    /* If requested digits is zero or less, we will need to truncate
     * the returned string */
    if (ndigits < 1) {
	stop += ndigits;
    }

    while (*ptr1 == '0') ptr1++; /* Skip leading zeroes */
    while (*ptr1 != '\0' && *ptr1 != decimal_separator) {
	if (!first) first = ptr2;
	if ((ptr1 - buf) < stop) {
	    if (size > 1) {
                *ptr2++ = *ptr1++;
                size--;
            }
	} else {
	    ptr1++;
	}
	dec1++;
    }

    if (ndigits > 0) {
	ptr1++;
	if (!first) {
	    while (*ptr1 == '0') { /* Process leading zeroes */
                if (number == 0.0 && size > 1) {
                    *ptr2++ = '0';
                    size--;
                }
                ptr1++;
		dec1--;
	    }
	}
	while (*ptr1 != '\0') {
	    if (!first) first = ptr2;
	    if (size > 1) {
                *ptr2++ = *ptr1++;
                size--;
            }
	}
    }

    *ptr2 = '\0';

    /* We never found a non-zero digit, then our number is either
     * smaller than the requested precision, or 0.0 */
    if (!first && (number <= 0.0))
        dec1 = 0;

    *decpt = dec2 ? dec2 : dec1;
    return 0;
}

/***********************************************************************
 *		_gcvt  (MSVCRT.@)
 */
char * CDECL MSVCRT__gcvt( double number, int ndigit, char *buff )
{
    if(!buff) {
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return NULL;
    }

    if(ndigit < 0) {
        *MSVCRT__errno() = MSVCRT_ERANGE;
        return NULL;
    }

    MSVCRT_sprintf(buff, "%.*g", ndigit, number);
    return buff;
}

/***********************************************************************
 *              _gcvt_s  (MSVCRT.@)
 */
int CDECL MSVCRT__gcvt_s(char *buff, MSVCRT_size_t size, double number, int digits)
{
    int len;

    if(!buff) {
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return MSVCRT_EINVAL;
    }

    if( digits<0 || digits>=size) {
        if(size)
            buff[0] = '\0';

        *MSVCRT__errno() = MSVCRT_ERANGE;
        return MSVCRT_ERANGE;
    }

    len = MSVCRT__scprintf("%.*g", digits, number);
    if(len > size) {
        buff[0] = '\0';
        *MSVCRT__errno() = MSVCRT_ERANGE;
        return MSVCRT_ERANGE;
    }

    MSVCRT_sprintf(buff, "%.*g", digits, number);
    return 0;
}

#include <stdlib.h> /* div_t, ldiv_t */

/*********************************************************************
 *		div (MSVCRT.@)
 * VERSION
 *	[i386] Windows binary compatible - returns the struct in eax/edx.
 */
#ifdef __i386__
unsigned __int64 CDECL MSVCRT_div(int num, int denom)
{
    union {
        MSVCRT_div_t div;
        unsigned __int64 uint64;
    } ret;

    ret.div.quot = num / denom;
    ret.div.rem = num % denom;
    return ret.uint64;
}
#else
/*********************************************************************
 *		div (MSVCRT.@)
 * VERSION
 *	[!i386] Non-x86 can't run win32 apps so we don't need binary compatibility
 */
MSVCRT_div_t CDECL MSVCRT_div(int num, int denom)
{
    MSVCRT_div_t ret;

    ret.quot = num / denom;
    ret.rem = num % denom;
    return ret;
}
#endif /* ifdef __i386__ */


/*********************************************************************
 *		ldiv (MSVCRT.@)
 * VERSION
 * 	[i386] Windows binary compatible - returns the struct in eax/edx.
 */
#ifdef __i386__
unsigned __int64 CDECL MSVCRT_ldiv(MSVCRT_long num, MSVCRT_long denom)
{
    union {
        MSVCRT_ldiv_t ldiv;
        unsigned __int64 uint64;
    } ret;

    ret.ldiv.quot = num / denom;
    ret.ldiv.rem = num % denom;
    return ret.uint64;
}
#else
/*********************************************************************
 *		ldiv (MSVCRT.@)
 * VERSION
 *	[!i386] Non-x86 can't run win32 apps so we don't need binary compatibility
 */
MSVCRT_ldiv_t CDECL MSVCRT_ldiv(MSVCRT_long num, MSVCRT_long denom)
{
    MSVCRT_ldiv_t ret;

    ret.quot = num / denom;
    ret.rem = num % denom;
    return ret;
}
#endif /* ifdef __i386__ */

#if _MSVCR_VER>=100
/*********************************************************************
 *		lldiv (MSVCR100.@)
 */
MSVCRT_lldiv_t CDECL MSVCRT_lldiv(MSVCRT_longlong num, MSVCRT_longlong denom)
{
  MSVCRT_lldiv_t ret;

  ret.quot = num / denom;
  ret.rem = num % denom;

  return ret;
}
#endif

#ifdef __i386__

/*********************************************************************
 *		_adjust_fdiv (MSVCRT.@)
 * Used by the MSVC compiler to work around the Pentium FDIV bug.
 */
int MSVCRT__adjust_fdiv = 0;

/***********************************************************************
 *		_adj_fdiv_m16i (MSVCRT.@)
 *
 * NOTE
 *    I _think_ this function is intended to work around the Pentium
 *    fdiv bug.
 */
void __stdcall _adj_fdiv_m16i( short arg )
{
  TRACE("(): stub\n");
}

/***********************************************************************
 *		_adj_fdiv_m32 (MSVCRT.@)
 *
 * NOTE
 *    I _think_ this function is intended to work around the Pentium
 *    fdiv bug.
 */
void __stdcall _adj_fdiv_m32( unsigned int arg )
{
  TRACE("(): stub\n");
}

/***********************************************************************
 *		_adj_fdiv_m32i (MSVCRT.@)
 *
 * NOTE
 *    I _think_ this function is intended to work around the Pentium
 *    fdiv bug.
 */
void __stdcall _adj_fdiv_m32i( int arg )
{
  TRACE("(): stub\n");
}

/***********************************************************************
 *		_adj_fdiv_m64 (MSVCRT.@)
 *
 * NOTE
 *    I _think_ this function is intended to work around the Pentium
 *    fdiv bug.
 */
void __stdcall _adj_fdiv_m64( unsigned __int64 arg )
{
  TRACE("(): stub\n");
}

/***********************************************************************
 *		_adj_fdiv_r (MSVCRT.@)
 * FIXME
 *    This function is likely to have the wrong number of arguments.
 *
 * NOTE
 *    I _think_ this function is intended to work around the Pentium
 *    fdiv bug.
 */
void _adj_fdiv_r(void)
{
  TRACE("(): stub\n");
}

/***********************************************************************
 *		_adj_fdivr_m16i (MSVCRT.@)
 *
 * NOTE
 *    I _think_ this function is intended to work around the Pentium
 *    fdiv bug.
 */
void __stdcall _adj_fdivr_m16i( short arg )
{
  TRACE("(): stub\n");
}

/***********************************************************************
 *		_adj_fdivr_m32 (MSVCRT.@)
 *
 * NOTE
 *    I _think_ this function is intended to work around the Pentium
 *    fdiv bug.
 */
void __stdcall _adj_fdivr_m32( unsigned int arg )
{
  TRACE("(): stub\n");
}

/***********************************************************************
 *		_adj_fdivr_m32i (MSVCRT.@)
 *
 * NOTE
 *    I _think_ this function is intended to work around the Pentium
 *    fdiv bug.
 */
void __stdcall _adj_fdivr_m32i( int arg )
{
  TRACE("(): stub\n");
}

/***********************************************************************
 *		_adj_fdivr_m64 (MSVCRT.@)
 *
 * NOTE
 *    I _think_ this function is intended to work around the Pentium
 *    fdiv bug.
 */
void __stdcall _adj_fdivr_m64( unsigned __int64 arg )
{
  TRACE("(): stub\n");
}

/***********************************************************************
 *		_adj_fpatan (MSVCRT.@)
 * FIXME
 *    This function is likely to have the wrong number of arguments.
 *
 * NOTE
 *    I _think_ this function is intended to work around the Pentium
 *    fdiv bug.
 */
void _adj_fpatan(void)
{
  TRACE("(): stub\n");
}

/***********************************************************************
 *		_adj_fprem (MSVCRT.@)
 * FIXME
 *    This function is likely to have the wrong number of arguments.
 *
 * NOTE
 *    I _think_ this function is intended to work around the Pentium
 *    fdiv bug.
 */
void _adj_fprem(void)
{
  TRACE("(): stub\n");
}

/***********************************************************************
 *		_adj_fprem1 (MSVCRT.@)
 * FIXME
 *    This function is likely to have the wrong number of arguments.
 *
 * NOTE
 *    I _think_ this function is intended to work around the Pentium
 *    fdiv bug.
 */
void _adj_fprem1(void)
{
  TRACE("(): stub\n");
}

/***********************************************************************
 *		_adj_fptan (MSVCRT.@)
 * FIXME
 *    This function is likely to have the wrong number of arguments.
 *
 * NOTE
 *    I _think_ this function is intended to work around the Pentium
 *    fdiv bug.
 */
void _adj_fptan(void)
{
  TRACE("(): stub\n");
}

/***********************************************************************
 *		_safe_fdiv (MSVCRT.@)
 * FIXME
 *    This function is likely to have the wrong number of arguments.
 *
 * NOTE
 *    I _think_ this function is intended to work around the Pentium
 *    fdiv bug.
 */
void _safe_fdiv(void)
{
  TRACE("(): stub\n");
}

/***********************************************************************
 *		_safe_fdivr (MSVCRT.@)
 * FIXME
 *    This function is likely to have the wrong number of arguments.
 *
 * NOTE
 *    I _think_ this function is intended to work around the Pentium
 *    fdiv bug.
 */
void _safe_fdivr(void)
{
  TRACE("(): stub\n");
}

/***********************************************************************
 *		_safe_fprem (MSVCRT.@)
 * FIXME
 *    This function is likely to have the wrong number of arguments.
 *
 * NOTE
 *    I _think_ this function is intended to work around the Pentium
 *    fdiv bug.
 */
void _safe_fprem(void)
{
  TRACE("(): stub\n");
}

/***********************************************************************
 *		_safe_fprem1 (MSVCRT.@)
 *
 * FIXME
 *    This function is likely to have the wrong number of arguments.
 *
 * NOTE
 *    I _think_ this function is intended to work around the Pentium
 *    fdiv bug.
 */
void _safe_fprem1(void)
{
  TRACE("(): stub\n");
}

/***********************************************************************
 *		__libm_sse2_acos   (MSVCRT.@)
 */
void __cdecl MSVCRT___libm_sse2_acos(void)
{
    double d;
    __asm__ __volatile__( "movq %%xmm0,%0" : "=m" (d) );
    d = acos( d );
    __asm__ __volatile__( "movq %0,%%xmm0" : : "m" (d) );
}

/***********************************************************************
 *		__libm_sse2_acosf   (MSVCRT.@)
 */
void __cdecl MSVCRT___libm_sse2_acosf(void)
{
    float f;
    __asm__ __volatile__( "movd %%xmm0,%0" : "=g" (f) );
    f = acosf( f );
    __asm__ __volatile__( "movd %0,%%xmm0" : : "g" (f) );
}

/***********************************************************************
 *		__libm_sse2_asin   (MSVCRT.@)
 */
void __cdecl MSVCRT___libm_sse2_asin(void)
{
    double d;
    __asm__ __volatile__( "movq %%xmm0,%0" : "=m" (d) );
    d = asin( d );
    __asm__ __volatile__( "movq %0,%%xmm0" : : "m" (d) );
}

/***********************************************************************
 *		__libm_sse2_asinf   (MSVCRT.@)
 */
void __cdecl MSVCRT___libm_sse2_asinf(void)
{
    float f;
    __asm__ __volatile__( "movd %%xmm0,%0" : "=g" (f) );
    f = asinf( f );
    __asm__ __volatile__( "movd %0,%%xmm0" : : "g" (f) );
}

/***********************************************************************
 *		__libm_sse2_atan   (MSVCRT.@)
 */
void __cdecl MSVCRT___libm_sse2_atan(void)
{
    double d;
    __asm__ __volatile__( "movq %%xmm0,%0" : "=m" (d) );
    d = atan( d );
    __asm__ __volatile__( "movq %0,%%xmm0" : : "m" (d) );
}

/***********************************************************************
 *		__libm_sse2_atan2   (MSVCRT.@)
 */
void __cdecl MSVCRT___libm_sse2_atan2(void)
{
    double d1, d2;
    __asm__ __volatile__( "movq %%xmm0,%0; movq %%xmm1,%1 " : "=m" (d1), "=m" (d2) );
    d1 = atan2( d1, d2 );
    __asm__ __volatile__( "movq %0,%%xmm0" : : "m" (d1) );
}

/***********************************************************************
 *		__libm_sse2_atanf   (MSVCRT.@)
 */
void __cdecl MSVCRT___libm_sse2_atanf(void)
{
    float f;
    __asm__ __volatile__( "movd %%xmm0,%0" : "=g" (f) );
    f = atanf( f );
    __asm__ __volatile__( "movd %0,%%xmm0" : : "g" (f) );
}

/***********************************************************************
 *		__libm_sse2_cos   (MSVCRT.@)
 */
void __cdecl MSVCRT___libm_sse2_cos(void)
{
    double d;
    __asm__ __volatile__( "movq %%xmm0,%0" : "=m" (d) );
    d = cos( d );
    __asm__ __volatile__( "movq %0,%%xmm0" : : "m" (d) );
}

/***********************************************************************
 *		__libm_sse2_cosf   (MSVCRT.@)
 */
void __cdecl MSVCRT___libm_sse2_cosf(void)
{
    float f;
    __asm__ __volatile__( "movd %%xmm0,%0" : "=g" (f) );
    f = cosf( f );
    __asm__ __volatile__( "movd %0,%%xmm0" : : "g" (f) );
}

/***********************************************************************
 *		__libm_sse2_exp   (MSVCRT.@)
 */
void __cdecl MSVCRT___libm_sse2_exp(void)
{
    double d;
    __asm__ __volatile__( "movq %%xmm0,%0" : "=m" (d) );
    d = exp( d );
    __asm__ __volatile__( "movq %0,%%xmm0" : : "m" (d) );
}

/***********************************************************************
 *		__libm_sse2_expf   (MSVCRT.@)
 */
void __cdecl MSVCRT___libm_sse2_expf(void)
{
    float f;
    __asm__ __volatile__( "movd %%xmm0,%0" : "=g" (f) );
    f = expf( f );
    __asm__ __volatile__( "movd %0,%%xmm0" : : "g" (f) );
}

/***********************************************************************
 *		__libm_sse2_log   (MSVCRT.@)
 */
void __cdecl MSVCRT___libm_sse2_log(void)
{
    double d;
    __asm__ __volatile__( "movq %%xmm0,%0" : "=m" (d) );
    d = log( d );
    __asm__ __volatile__( "movq %0,%%xmm0" : : "m" (d) );
}

/***********************************************************************
 *		__libm_sse2_log10   (MSVCRT.@)
 */
void __cdecl MSVCRT___libm_sse2_log10(void)
{
    double d;
    __asm__ __volatile__( "movq %%xmm0,%0" : "=m" (d) );
    d = log10( d );
    __asm__ __volatile__( "movq %0,%%xmm0" : : "m" (d) );
}

/***********************************************************************
 *		__libm_sse2_log10f   (MSVCRT.@)
 */
void __cdecl MSVCRT___libm_sse2_log10f(void)
{
    float f;
    __asm__ __volatile__( "movd %%xmm0,%0" : "=g" (f) );
    f = log10f( f );
    __asm__ __volatile__( "movd %0,%%xmm0" : : "g" (f) );
}

/***********************************************************************
 *		__libm_sse2_logf   (MSVCRT.@)
 */
void __cdecl MSVCRT___libm_sse2_logf(void)
{
    float f;
    __asm__ __volatile__( "movd %%xmm0,%0" : "=g" (f) );
    f = logf( f );
    __asm__ __volatile__( "movd %0,%%xmm0" : : "g" (f) );
}

/***********************************************************************
 *		__libm_sse2_pow   (MSVCRT.@)
 */
void __cdecl MSVCRT___libm_sse2_pow(void)
{
    double d1, d2;
    __asm__ __volatile__( "movq %%xmm0,%0; movq %%xmm1,%1 " : "=m" (d1), "=m" (d2) );
    d1 = pow( d1, d2 );
    __asm__ __volatile__( "movq %0,%%xmm0" : : "m" (d1) );
}

/***********************************************************************
 *		__libm_sse2_powf   (MSVCRT.@)
 */
void __cdecl MSVCRT___libm_sse2_powf(void)
{
    float f1, f2;
    __asm__ __volatile__( "movd %%xmm0,%0; movd %%xmm1,%1" : "=g" (f1), "=g" (f2) );
    f1 = powf( f1, f2 );
    __asm__ __volatile__( "movd %0,%%xmm0" : : "g" (f1) );
}

/***********************************************************************
 *		__libm_sse2_sin   (MSVCRT.@)
 */
void __cdecl MSVCRT___libm_sse2_sin(void)
{
    double d;
    __asm__ __volatile__( "movq %%xmm0,%0" : "=m" (d) );
    d = sin( d );
    __asm__ __volatile__( "movq %0,%%xmm0" : : "m" (d) );
}

/***********************************************************************
 *		__libm_sse2_sinf   (MSVCRT.@)
 */
void __cdecl MSVCRT___libm_sse2_sinf(void)
{
    float f;
    __asm__ __volatile__( "movd %%xmm0,%0" : "=g" (f) );
    f = sinf( f );
    __asm__ __volatile__( "movd %0,%%xmm0" : : "g" (f) );
}

/***********************************************************************
 *		__libm_sse2_tan   (MSVCRT.@)
 */
void __cdecl MSVCRT___libm_sse2_tan(void)
{
    double d;
    __asm__ __volatile__( "movq %%xmm0,%0" : "=m" (d) );
    d = tan( d );
    __asm__ __volatile__( "movq %0,%%xmm0" : : "m" (d) );
}

/***********************************************************************
 *		__libm_sse2_tanf   (MSVCRT.@)
 */
void __cdecl MSVCRT___libm_sse2_tanf(void)
{
    float f;
    __asm__ __volatile__( "movd %%xmm0,%0" : "=g" (f) );
    f = tanf( f );
    __asm__ __volatile__( "movd %0,%%xmm0" : : "g" (f) );
}

/***********************************************************************
 *		__libm_sse2_sqrt_precise   (MSVCR110.@)
 */
void __cdecl MSVCRT___libm_sse2_sqrt_precise(void)
{
    double d;
    __asm__ __volatile__( "movq %%xmm0,%0" : "=m" (d) );
    d = sqrt( d );
    __asm__ __volatile__( "movq %0,%%xmm0" : : "m" (d) );
}

#endif  /* __i386__ */

/*********************************************************************
 *      cbrt (MSVCR120.@)
 */
double CDECL MSVCR120_cbrt(double x)
{
#ifdef HAVE_CBRT
    return cbrt(x);
#else
    return x < 0 ? -pow(-x, 1.0 / 3.0) : pow(x, 1.0 / 3.0);
#endif
}

/*********************************************************************
 *      cbrtf (MSVCR120.@)
 */
float CDECL MSVCR120_cbrtf(float x)
{
#ifdef HAVE_CBRTF
    return cbrtf(x);
#else
    return MSVCR120_cbrt(x);
#endif
}

/*********************************************************************
 *      cbrtl (MSVCR120.@)
 */
LDOUBLE CDECL MSVCR120_cbrtl(LDOUBLE x)
{
    return MSVCR120_cbrt(x);
}

/*********************************************************************
 *      exp2 (MSVCR120.@)
 */
double CDECL MSVCR120_exp2(double x)
{
#ifdef HAVE_EXP2
    double ret = exp2(x);
#else
    double ret = pow(2, x);
#endif
    if (isfinite(x) && !isfinite(ret)) *MSVCRT__errno() = MSVCRT_ERANGE;
    return ret;
}

/*********************************************************************
 *      exp2f (MSVCR120.@)
 */
float CDECL MSVCR120_exp2f(float x)
{
#ifdef HAVE_EXP2F
    float ret = exp2f(x);
    if (isfinite(x) && !isfinite(ret)) *MSVCRT__errno() = MSVCRT_ERANGE;
    return ret;
#else
    return MSVCR120_exp2(x);
#endif
}

/*********************************************************************
 *      exp2l (MSVCR120.@)
 */
LDOUBLE CDECL MSVCR120_exp2l(LDOUBLE x)
{
    return MSVCR120_exp2(x);
}

/*********************************************************************
 *      expm1 (MSVCR120.@)
 */
double CDECL MSVCR120_expm1(double x)
{
#ifdef HAVE_EXPM1
    double ret = expm1(x);
#else
    double ret = exp(x) - 1;
#endif
    if (isfinite(x) && !isfinite(ret)) *MSVCRT__errno() = MSVCRT_ERANGE;
    return ret;
}

/*********************************************************************
 *      expm1f (MSVCR120.@)
 */
float CDECL MSVCR120_expm1f(float x)
{
#ifdef HAVE_EXPM1F
    float ret = expm1f(x);
#else
    float ret = exp(x) - 1;
#endif
    if (isfinite(x) && !isfinite(ret)) *MSVCRT__errno() = MSVCRT_ERANGE;
    return ret;
}

/*********************************************************************
 *      expm1l (MSVCR120.@)
 */
LDOUBLE CDECL MSVCR120_expm1l(LDOUBLE x)
{
    return MSVCR120_expm1(x);
}

/*********************************************************************
 *      log1p (MSVCR120.@)
 */
double CDECL MSVCR120_log1p(double x)
{
    if (x < -1) *MSVCRT__errno() = MSVCRT_EDOM;
    else if (x == -1) *MSVCRT__errno() = MSVCRT_ERANGE;
#ifdef HAVE_LOG1P
    return log1p(x);
#else
    return log(1 + x);
#endif
}

/*********************************************************************
 *      log1pf (MSVCR120.@)
 */
float CDECL MSVCR120_log1pf(float x)
{
    if (x < -1) *MSVCRT__errno() = MSVCRT_EDOM;
    else if (x == -1) *MSVCRT__errno() = MSVCRT_ERANGE;
#ifdef HAVE_LOG1PF
    return log1pf(x);
#else
    return log(1 + x);
#endif
}

/*********************************************************************
 *      log1pl (MSVCR120.@)
 */
LDOUBLE CDECL MSVCR120_log1pl(LDOUBLE x)
{
    return MSVCR120_log1p(x);
}

/*********************************************************************
 *      log2 (MSVCR120.@)
 */
double CDECL MSVCR120_log2(double x)
{
    if (x < 0) *MSVCRT__errno() = MSVCRT_EDOM;
    else if (x == 0) *MSVCRT__errno() = MSVCRT_ERANGE;
#ifdef HAVE_LOG2
    return log2(x);
#else
    return log(x) / log(2);
#endif
}

/*********************************************************************
 *      log2f (MSVCR120.@)
 */
float CDECL MSVCR120_log2f(float x)
{
#ifdef HAVE_LOG2F
    if (x < 0) *MSVCRT__errno() = MSVCRT_EDOM;
    else if (x == 0) *MSVCRT__errno() = MSVCRT_ERANGE;
    return log2f(x);
#else
    return MSVCR120_log2(x);
#endif
}

/*********************************************************************
 *      log2l (MSVCR120.@)
 */
LDOUBLE CDECL MSVCR120_log2l(LDOUBLE x)
{
    return MSVCR120_log2(x);
}

/*********************************************************************
 *      rint (MSVCR120.@)
 */
double CDECL MSVCR120_rint(double x)
{
    return rint(x);
}

/*********************************************************************
 *      rintf (MSVCR120.@)
 */
float CDECL MSVCR120_rintf(float x)
{
    return rintf(x);
}

/*********************************************************************
 *      rintl (MSVCR120.@)
 */
LDOUBLE CDECL MSVCR120_rintl(LDOUBLE x)
{
    return MSVCR120_rint(x);
}

/*********************************************************************
 *      lrint (MSVCR120.@)
 */
MSVCRT_long CDECL MSVCR120_lrint(double x)
{
    return lrint(x);
}

/*********************************************************************
 *      lrintf (MSVCR120.@)
 */
MSVCRT_long CDECL MSVCR120_lrintf(float x)
{
    return lrintf(x);
}

/*********************************************************************
 *      lrintl (MSVCR120.@)
 */
MSVCRT_long CDECL MSVCR120_lrintl(LDOUBLE x)
{
    return MSVCR120_lrint(x);
}

/*********************************************************************
 *      llrint (MSVCR120.@)
 */
MSVCRT_longlong CDECL MSVCR120_llrint(double x)
{
    return llrint(x);
}

/*********************************************************************
 *      llrintf (MSVCR120.@)
 */
MSVCRT_longlong CDECL MSVCR120_llrintf(float x)
{
    return llrintf(x);
}

/*********************************************************************
 *      rintl (MSVCR120.@)
 */
MSVCRT_longlong CDECL MSVCR120_llrintl(LDOUBLE x)
{
    return MSVCR120_llrint(x);
}

#if _MSVCR_VER>=120

/*********************************************************************
 *      round (MSVCR120.@)
 */
double CDECL MSVCR120_round(double x)
{
#ifdef HAVE_ROUND
    return round(x);
#else
    return MSVCR120_rint(x);
#endif
}

/*********************************************************************
 *      roundf (MSVCR120.@)
 */
float CDECL MSVCR120_roundf(float x)
{
#ifdef HAVE_ROUNDF
    return roundf(x);
#else
    return MSVCR120_round(x);
#endif
}

/*********************************************************************
 *      roundl (MSVCR120.@)
 */
LDOUBLE CDECL MSVCR120_roundl(LDOUBLE x)
{
    return MSVCR120_round(x);
}

/*********************************************************************
 *      lround (MSVCR120.@)
 */
MSVCRT_long CDECL MSVCR120_lround(double x)
{
#ifdef HAVE_LROUND
    return lround(x);
#else
    return MSVCR120_round(x);
#endif
}

/*********************************************************************
 *      lroundf (MSVCR120.@)
 */
MSVCRT_long CDECL MSVCR120_lroundf(float x)
{
#ifdef HAVE_LROUNDF
    return lroundf(x);
#else
    return MSVCR120_lround(x);
#endif
}

/*********************************************************************
 *      lroundl (MSVCR120.@)
 */
MSVCRT_long CDECL MSVCR120_lroundl(LDOUBLE x)
{
    return MSVCR120_lround(x);
}

/*********************************************************************
 *      llround (MSVCR120.@)
 */
MSVCRT_longlong CDECL MSVCR120_llround(double x)
{
#ifdef HAVE_LLROUND
    return llround(x);
#else
    return MSVCR120_round(x);
#endif
}

/*********************************************************************
 *      llroundf (MSVCR120.@)
 */
MSVCRT_longlong CDECL MSVCR120_llroundf(float x)
{
#ifdef HAVE_LLROUNDF
    return llroundf(x);
#else
    return MSVCR120_llround(x);
#endif
}

/*********************************************************************
 *      roundl (MSVCR120.@)
 */
MSVCRT_longlong CDECL MSVCR120_llroundl(LDOUBLE x)
{
    return MSVCR120_llround(x);
}

/*********************************************************************
 *      trunc (MSVCR120.@)
 */
double CDECL MSVCR120_trunc(double x)
{
#ifdef HAVE_TRUNC
    return trunc(x);
#else
    return (x > 0) ? floor(x) : ceil(x);
#endif
}

/*********************************************************************
 *      truncf (MSVCR120.@)
 */
float CDECL MSVCR120_truncf(float x)
{
#ifdef HAVE_TRUNCF
    return truncf(x);
#else
    return MSVCR120_trunc(x);
#endif
}

/*********************************************************************
 *      truncl (MSVCR120.@)
 */
LDOUBLE CDECL MSVCR120_truncl(LDOUBLE x)
{
    return MSVCR120_trunc(x);
}

/*********************************************************************
 *      _dclass (MSVCR120.@)
 *
 * Copied from musl: src/math/__fpclassify.c
 */
short CDECL MSVCR120__dclass(double x)
{
    union { double f; UINT64 i; } u = { x };
    int e = u.i >> 52 & 0x7ff;

    if (!e) return u.i << 1 ? MSVCRT_FP_SUBNORMAL : MSVCRT_FP_ZERO;
    if (e == 0x7ff) return (u.i << 12) ? MSVCRT_FP_NAN : MSVCRT_FP_INFINITE;
    return MSVCRT_FP_NORMAL;
}

/*********************************************************************
 *      _fdclass (MSVCR120.@)
 *
 * Copied from musl: src/math/__fpclassifyf.c
 */
short CDECL MSVCR120__fdclass(float x)
{
    union { float f; UINT32 i; } u = { x };
    int e = u.i >> 23 & 0xff;

    if (!e) return u.i << 1 ? MSVCRT_FP_SUBNORMAL : MSVCRT_FP_ZERO;
    if (e == 0xff) return u.i << 9 ? MSVCRT_FP_NAN : MSVCRT_FP_INFINITE;
    return MSVCRT_FP_NORMAL;
}

/*********************************************************************
 *      _ldclass (MSVCR120.@)
 */
short CDECL MSVCR120__ldclass(LDOUBLE x)
{
    return MSVCR120__dclass(x);
}

/*********************************************************************
 *      _dtest (MSVCR120.@)
 */
short CDECL MSVCR120__dtest(double *x)
{
    return MSVCR120__dclass(*x);
}

/*********************************************************************
 *      _fdtest (MSVCR120.@)
 */
short CDECL MSVCR120__fdtest(float *x)
{
    return MSVCR120__fdclass(*x);
}

/*********************************************************************
 *      _ldtest (MSVCR120.@)
 */
short CDECL MSVCR120__ldtest(LDOUBLE *x)
{
    return MSVCR120__dclass(*x);
}

/*********************************************************************
 *      erf (MSVCR120.@)
 */
double CDECL MSVCR120_erf(double x)
{
#ifdef HAVE_ERF
    return erf(x);
#else
    /* Abramowitz and Stegun approximation, maximum error: 1.5*10^-7 */
    double t, y;
    int sign = signbit(x);

    if (sign) x = -x;
    t = 1 / (1 + 0.3275911 * x);
    y = ((((1.061405429*t - 1.453152027)*t + 1.421413741)*t - 0.284496736)*t + 0.254829592)*t;
    y = 1.0 - y*exp(-x*x);
    return sign ? -y : y;
#endif
}

/*********************************************************************
 *      erff (MSVCR120.@)
 */
float CDECL MSVCR120_erff(float x)
{
#ifdef HAVE_ERFF
    return erff(x);
#else
    return MSVCR120_erf(x);
#endif
}

/*********************************************************************
 *      erfl (MSVCR120.@)
 */
LDOUBLE CDECL MSVCR120_erfl(LDOUBLE x)
{
    return MSVCR120_erf(x);
}

/*********************************************************************
 *      erfc (MSVCR120.@)
 */
double CDECL MSVCR120_erfc(double x)
{
#ifdef HAVE_ERFC
    return erfc(x);
#else
    return 1 - MSVCR120_erf(x);
#endif
}

/*********************************************************************
 *      erfcf (MSVCR120.@)
 */
float CDECL MSVCR120_erfcf(float x)
{
#ifdef HAVE_ERFCF
    return erfcf(x);
#else
    return MSVCR120_erfc(x);
#endif
}

/*********************************************************************
 *      erfcl (MSVCR120.@)
 */
LDOUBLE CDECL MSVCR120_erfcl(LDOUBLE x)
{
    return MSVCR120_erfc(x);
}

/*********************************************************************
 *      fmaxf (MSVCR120.@)
 */
float CDECL MSVCR120_fmaxf(float x, float y)
{
    if(isnan(x))
        return y;
    if(isnan(y))
        return x;
    if(x==0 && y==0)
        return signbit(x) ? y : x;
    return x<y ? y : x;
}

/*********************************************************************
 *      fmax (MSVCR120.@)
 */
double CDECL MSVCR120_fmax(double x, double y)
{
    if(isnan(x))
        return y;
    if(isnan(y))
        return x;
    if(x==0 && y==0)
        return signbit(x) ? y : x;
    return x<y ? y : x;
}

/*********************************************************************
 *      fdimf (MSVCR120.@)
 */
float CDECL MSVCR120_fdimf(float x, float y)
{
    if(isnan(x))
        return x;
    if(isnan(y))
        return y;
    return x>y ? x-y : 0;
}

/*********************************************************************
 *      fdim (MSVCR120.@)
 */
double CDECL MSVCR120_fdim(double x, double y)
{
    if(isnan(x))
        return x;
    if(isnan(y))
        return y;
    return x>y ? x-y : 0;
}

/*********************************************************************
 *      _fdsign (MSVCR120.@)
 */
int CDECL MSVCR120__fdsign(float x)
{
    union { float f; UINT32 i; } u = { x };
    return (u.i >> 16) & 0x8000;
}

/*********************************************************************
 *      _dsign (MSVCR120.@)
 */
int CDECL MSVCR120__dsign(double x)
{
    union { double f; UINT64 i; } u = { x };
    return (u.i >> 48) & 0x8000;
}


/*********************************************************************
 *      _dpcomp (MSVCR120.@)
 */
int CDECL MSVCR120__dpcomp(double x, double y)
{
    if(isnan(x) || isnan(y))
        return 0;

    if(x == y) return 2;
    return x < y ? 1 : 4;
}

/*********************************************************************
 *      _fdpcomp (MSVCR120.@)
 */
int CDECL MSVCR120__fdpcomp(float x, float y)
{
    return MSVCR120__dpcomp(x, y);
}

/*********************************************************************
 *      fminf (MSVCR120.@)
 */
float CDECL MSVCR120_fminf(float x, float y)
{
    if(isnan(x))
        return y;
    if(isnan(y))
        return x;
    if(x==0 && y==0)
        return signbit(x) ? x : y;
    return x<y ? x : y;
}

/*********************************************************************
 *      fmin (MSVCR120.@)
 */
double CDECL MSVCR120_fmin(double x, double y)
{
    if(isnan(x))
        return y;
    if(isnan(y))
        return x;
    if(x==0 && y==0)
        return signbit(x) ? x : y;
    return x<y ? x : y;
}

/*********************************************************************
 *      asinh (MSVCR120.@)
 */
double CDECL MSVCR120_asinh(double x)
{
#ifdef HAVE_ASINH
    return asinh(x);
#else
    if (!isfinite(x*x+1)) {
      if (x > 0) return log(2) + log(x);
      else return -log(2) - log(-x);
    }
    return log(x + sqrt(x*x+1));
#endif
}

/*********************************************************************
 *      asinhf (MSVCR120.@)
 */
float CDECL MSVCR120_asinhf(float x)
{
#ifdef HAVE_ASINHF
    return asinhf(x);
#else
    return MSVCR120_asinh(x);
#endif
}

/*********************************************************************
 *      asinhl (MSVCR120.@)
 */
LDOUBLE CDECL MSVCR120_asinhl(LDOUBLE x)
{
    return MSVCR120_asinh(x);
}

/*********************************************************************
 *      acosh (MSVCR120.@)
 */
double CDECL MSVCR120_acosh(double x)
{
    if (x < 1) *MSVCRT__errno() = MSVCRT_EDOM;

#ifdef HAVE_ACOSH
    return acosh(x);
#else
    if (x < 1) {
        MSVCRT_fenv_t env;

        MSVCRT_fegetenv(&env);
        env.status |= MSVCRT__SW_INVALID;
        MSVCRT_fesetenv(&env);
        return NAN;
    }
    if (!isfinite(x*x)) return log(2) + log(x);
    return log(x + sqrt(x*x-1));
#endif
}

/*********************************************************************
 *      acoshf (MSVCR120.@)
 */
float CDECL MSVCR120_acoshf(float x)
{
#ifdef HAVE_ACOSHF
    if (x < 1) *MSVCRT__errno() = MSVCRT_EDOM;

    return acoshf(x);
#else
    return MSVCR120_acosh(x);
#endif
}

/*********************************************************************
 *      acoshl (MSVCR120.@)
 */
LDOUBLE CDECL MSVCR120_acoshl(LDOUBLE x)
{
    return MSVCR120_acosh(x);
}

/*********************************************************************
 *      atanh (MSVCR120.@)
 */
double CDECL MSVCR120_atanh(double x)
{
    double ret;

    if (x > 1 || x < -1) {
        MSVCRT_fenv_t env;

        *MSVCRT__errno() = MSVCRT_EDOM;

        /* on Linux atanh returns -NAN in this case */
        MSVCRT_fegetenv(&env);
        env.status |= MSVCRT__SW_INVALID;
        MSVCRT_fesetenv(&env);
        return NAN;
    }

#ifdef HAVE_ATANH
    ret = atanh(x);
#else
    if (-1e-6 < x && x < 1e-6) ret = x + x*x*x/3;
    else ret = (log(1+x) - log(1-x)) / 2;
#endif

    if (!isfinite(ret)) *MSVCRT__errno() = MSVCRT_ERANGE;
    return ret;
}

/*********************************************************************
 *      atanhf (MSVCR120.@)
 */
float CDECL MSVCR120_atanhf(float x)
{
#ifdef HAVE_ATANHF
    float ret;

    if (x > 1 || x < -1) {
        MSVCRT_fenv_t env;

        *MSVCRT__errno() = MSVCRT_EDOM;

        MSVCRT_fegetenv(&env);
        env.status |= MSVCRT__SW_INVALID;
        MSVCRT_fesetenv(&env);
        return NAN;
    }

    ret = atanhf(x);

    if (!isfinite(ret)) *MSVCRT__errno() = MSVCRT_ERANGE;
    return ret;
#else
    return MSVCR120_atanh(x);
#endif
}

/*********************************************************************
 *      atanhl (MSVCR120.@)
 */
LDOUBLE CDECL MSVCR120_atanhl(LDOUBLE x)
{
    return MSVCR120_atanh(x);
}

#endif /* _MSVCR_VER>=120 */

/*********************************************************************
 *      _scalb  (MSVCRT.@)
 *      scalbn  (MSVCR120.@)
 *      scalbln (MSVCR120.@)
 */
double CDECL MSVCRT__scalb(double num, MSVCRT_long power)
{
  return MSVCRT_ldexp(num, power);
}

/*********************************************************************
 *      _scalbf  (MSVCRT.@)
 *      scalbnf  (MSVCR120.@)
 *      scalblnf (MSVCR120.@)
 */
float CDECL MSVCRT__scalbf(float num, MSVCRT_long power)
{
  return MSVCRT_ldexp(num, power);
}

#if _MSVCR_VER>=120

/*********************************************************************
 *      scalbnl  (MSVCR120.@)
 *      scalblnl (MSVCR120.@)
 */
LDOUBLE CDECL MSVCR120_scalbnl(LDOUBLE num, MSVCRT_long power)
{
    return MSVCRT__scalb(num, power);
}

/*********************************************************************
 *      remainder (MSVCR120.@)
 */
double CDECL MSVCR120_remainder(double x, double y)
{
#ifdef HAVE_REMAINDER
    /* this matches 64-bit Windows.  32-bit Windows is slightly different */
    if(!isfinite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
    if(isnan(y) || y==0.0) *MSVCRT__errno() = MSVCRT_EDOM;
    return remainder(x, y);
#else
    FIXME( "not implemented\n" );
    return 0.0;
#endif
}

/*********************************************************************
 *      remainderf (MSVCR120.@)
 */
float CDECL MSVCR120_remainderf(float x, float y)
{
#ifdef HAVE_REMAINDERF
    /* this matches 64-bit Windows.  32-bit Windows is slightly different */
    if(!isfinite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
    if(isnan(y) || y==0.0f) *MSVCRT__errno() = MSVCRT_EDOM;
    return remainderf(x, y);
#else
    FIXME( "not implemented\n" );
    return 0.0f;
#endif
}

/*********************************************************************
 *      remainderl (MSVCR120.@)
 */
LDOUBLE CDECL MSVCR120_remainderl(LDOUBLE x, LDOUBLE y)
{
    return MSVCR120_remainder(x, y);
}

/*********************************************************************
 *      remquo (MSVCR120.@)
 */
double CDECL MSVCR120_remquo(double x, double y, int *quo)
{
#ifdef HAVE_REMQUO
    if(!isfinite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
    if(isnan(y) || y==0.0) *MSVCRT__errno() = MSVCRT_EDOM;
    return remquo(x, y, quo);
#else
    FIXME( "not implemented\n" );
    return 0.0;
#endif
}

/*********************************************************************
 *      remquof (MSVCR120.@)
 */
float CDECL MSVCR120_remquof(float x, float y, int *quo)
{
#ifdef HAVE_REMQUOF
    if(!isfinite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
    if(isnan(y) || y==0.0f) *MSVCRT__errno() = MSVCRT_EDOM;
    return remquof(x, y, quo);
#else
    FIXME( "not implemented\n" );
    return 0.0f;
#endif
}

/*********************************************************************
 *      remquol (MSVCR120.@)
 */
LDOUBLE CDECL MSVCR120_remquol(LDOUBLE x, LDOUBLE y, int *quo)
{
    return MSVCR120_remquo(x, y, quo);
}

/*********************************************************************
 *      lgamma (MSVCR120.@)
 */
double CDECL MSVCR120_lgamma(double x)
{
#ifdef HAVE_LGAMMA
    return lgamma(x);
#else
    FIXME( "not implemented\n" );
    return 0.0;
#endif
}

/*********************************************************************
 *      lgammaf (MSVCR120.@)
 */
float CDECL MSVCR120_lgammaf(float x)
{
#ifdef HAVE_LGAMMAF
    return lgammaf(x);
#else
    FIXME( "not implemented\n" );
    return 0.0f;
#endif
}

/*********************************************************************
 *      lgammal (MSVCR120.@)
 */
LDOUBLE CDECL MSVCR120_lgammal(LDOUBLE x)
{
    return MSVCR120_lgamma(x);
}

/*********************************************************************
 *      tgamma (MSVCR120.@)
 */
double CDECL MSVCR120_tgamma(double x)
{
#ifdef HAVE_TGAMMA
    if(x==0.0) *MSVCRT__errno() = MSVCRT_ERANGE;
    if(x<0.0f) {
      double integral;
      if (modf(x, &integral) == 0)
        *MSVCRT__errno() = MSVCRT_EDOM;
    }
    return tgamma(x);
#else
    FIXME( "not implemented\n" );
    return 0.0;
#endif
}

/*********************************************************************
 *      tgammaf (MSVCR120.@)
 */
float CDECL MSVCR120_tgammaf(float x)
{
#ifdef HAVE_TGAMMAF
    if(x==0.0f) *MSVCRT__errno() = MSVCRT_ERANGE;
    if(x<0.0f) {
      float integral;
      if (modff(x, &integral) == 0)
        *MSVCRT__errno() = MSVCRT_EDOM;
    }
    return tgammaf(x);
#else
    FIXME( "not implemented\n" );
    return 0.0f;
#endif
}

/*********************************************************************
 *      nan (MSVCR120.@)
 */
double CDECL MSVCR120_nan(const char *tagp)
{
    /* Windows ignores input (MSDN) */
    return NAN;
}

/*********************************************************************
 *      nanf (MSVCR120.@)
 */
float CDECL MSVCR120_nanf(const char *tagp)
{
    return NAN;
}

/*********************************************************************
 *      _except1 (MSVCR120.@)
 *  TODO:
 *   - find meaning of ignored cw and operation bits
 *   - unk parameter
 */
double CDECL _except1(DWORD fpe, _FP_OPERATION_CODE op, double arg, double res, DWORD cw, void *unk)
{
    ULONG_PTR exception_arg;
    DWORD exception = 0;
    MSVCRT_fenv_t env;
    DWORD fpword = 0;
    WORD operation;

    TRACE("(%x %x %lf %lf %x %p)\n", fpe, op, arg, res, cw, unk);

#ifdef _WIN64
    cw = ((cw >> 7) & 0x3f) | ((cw >> 3) & 0xc00);
#endif
    operation = op << 5;
    exception_arg = (ULONG_PTR)&operation;

    MSVCRT_fegetenv(&env);

    if (fpe & 0x1) { /* overflow */
        if ((fpe == 0x1 && (cw & 0x8)) || (fpe==0x11 && (cw & 0x28))) {
            /* 32-bit version also sets SW_INEXACT here */
            env.status |= MSVCRT__SW_OVERFLOW;
            if (fpe & 0x10) env.status |= MSVCRT__SW_INEXACT;
            res = signbit(res) ? -INFINITY : INFINITY;
        } else {
            exception = EXCEPTION_FLT_OVERFLOW;
        }
    } else if (fpe & 0x2) { /* underflow */
        if ((fpe == 0x2 && (cw & 0x10)) || (fpe==0x12 && (cw & 0x30))) {
            env.status |= MSVCRT__SW_UNDERFLOW;
            if (fpe & 0x10) env.status |= MSVCRT__SW_INEXACT;
            res = signbit(res) ? -0.0 : 0.0;
        } else {
            exception = EXCEPTION_FLT_UNDERFLOW;
        }
    } else if (fpe & 0x4) { /* zerodivide */
        if ((fpe == 0x4 && (cw & 0x4)) || (fpe==0x14 && (cw & 0x24))) {
            env.status |= MSVCRT__SW_ZERODIVIDE;
            if (fpe & 0x10) env.status |= MSVCRT__SW_INEXACT;
        } else {
            exception = EXCEPTION_FLT_DIVIDE_BY_ZERO;
        }
    } else if (fpe & 0x8) { /* invalid */
        if (fpe == 0x8 && (cw & 0x1)) {
            env.status |= MSVCRT__SW_INVALID;
        } else {
            exception = EXCEPTION_FLT_INVALID_OPERATION;
        }
    } else if (fpe & 0x10) { /* inexact */
        if (fpe == 0x10 && (cw & 0x20)) {
            env.status |= MSVCRT__SW_INEXACT;
        } else {
            exception = EXCEPTION_FLT_INEXACT_RESULT;
        }
    }

    if (exception)
        env.status = 0;
    MSVCRT_fesetenv(&env);
    if (exception)
        RaiseException(exception, 0, 1, &exception_arg);

    if (cw & 0x1) fpword |= MSVCRT__EM_INVALID;
    if (cw & 0x2) fpword |= MSVCRT__EM_DENORMAL;
    if (cw & 0x4) fpword |= MSVCRT__EM_ZERODIVIDE;
    if (cw & 0x8) fpword |= MSVCRT__EM_OVERFLOW;
    if (cw & 0x10) fpword |= MSVCRT__EM_UNDERFLOW;
    if (cw & 0x20) fpword |= MSVCRT__EM_INEXACT;
    switch (cw & 0xc00)
    {
        case 0xc00: fpword |= MSVCRT__RC_UP|MSVCRT__RC_DOWN; break;
        case 0x800: fpword |= MSVCRT__RC_UP; break;
        case 0x400: fpword |= MSVCRT__RC_DOWN; break;
    }
    switch (cw & 0x300)
    {
        case 0x0:   fpword |= MSVCRT__PC_24; break;
        case 0x200: fpword |= MSVCRT__PC_53; break;
        case 0x300: fpword |= MSVCRT__PC_64; break;
    }
    if (cw & 0x1000) fpword |= MSVCRT__IC_AFFINE;
    _control87(fpword, 0xffffffff);

    return res;
}

_Dcomplex* CDECL MSVCR120__Cbuild(_Dcomplex *ret, double r, double i)
{
    ret->x = r;
    ret->y = i;
    return ret;
}

double CDECL MSVCR120_creal(_Dcomplex z)
{
    return z.x;
}

/*********************************************************************
 *      ilogb (MSVCR120.@)
 *
 * Copied from musl: src/math/ilogb.c
 */
int CDECL MSVCR120_ilogb(double x)
{
    union { double f; UINT64 i; } u = { x };
    int e = u.i >> 52 & 0x7ff;

    if (!e)
    {
        u.i <<= 12;
        if (u.i == 0) return MSVCRT_FP_ILOGB0;
        /* subnormal x */
        for (e = -0x3ff; u.i >> 63 == 0; e--, u.i <<= 1);
        return e;
    }
    if (e == 0x7ff) return u.i << 12 ? MSVCRT_FP_ILOGBNAN : MSVCRT_INT_MAX;
    return e - 0x3ff;
}

/*********************************************************************
 *      ilogbf (MSVCR120.@)
 *
 * Copied from musl: src/math/ilogbf.c
 */
int CDECL MSVCR120_ilogbf(float x)
{
    union { float f; UINT32 i; } u = { x };
    int e = u.i >> 23 & 0xff;

    if (!e)
    {
        u.i <<= 9;
        if (u.i == 0) return MSVCRT_FP_ILOGB0;
        /* subnormal x */
        for (e = -0x7f; u.i >> 31 == 0; e--, u.i <<= 1);
        return e;
    }
    if (e == 0xff) return u.i << 9 ? MSVCRT_FP_ILOGBNAN : MSVCRT_INT_MAX;
    return e - 0x7f;
}

/*********************************************************************
 *      ilogbl (MSVCR120.@)
 */
int CDECL MSVCR120_ilogbl(LDOUBLE x)
{
    return MSVCR120_ilogb(x);
}

#endif /* _MSVCR_VER>=120 */
