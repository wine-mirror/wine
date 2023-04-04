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

#include <assert.h>
#include <complex.h>
#include <stdio.h>
#include <fenv.h>
#include <fpieee.h>
#include <limits.h>
#include <locale.h>
#include <math.h>

#include "msvcrt.h"
#include "winternl.h"

#include "wine/asm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

#undef div
#undef ldiv

#define _DOMAIN         1       /* domain error in argument */
#define _SING           2       /* singularity */
#define _OVERFLOW       3       /* range overflow */
#define _UNDERFLOW      4       /* range underflow */

typedef int (CDECL *MSVCRT_matherr_func)(struct _exception *);

static MSVCRT_matherr_func MSVCRT_default_matherr_func = NULL;

BOOL sse2_supported;
static BOOL sse2_enabled;

void msvcrt_init_math( void *module )
{
    sse2_supported = IsProcessorFeaturePresent( PF_XMMI64_INSTRUCTIONS_AVAILABLE );
#if _MSVCR_VER <=71
    sse2_enabled = FALSE;
#else
    sse2_enabled = sse2_supported;
#endif
}

/* Copied from musl: src/internal/libm.h */
static inline float fp_barrierf(float x)
{
    volatile float y = x;
    return y;
}

static inline double fp_barrier(double x)
{
    volatile double y = x;
    return y;
}

static inline double ret_nan( BOOL update_sw )
{
    double x = 1.0;
    if (!update_sw) return -NAN;
    return (x - x) / (x - x);
}

#define SET_X87_CW(MASK) \
    "subl $4, %esp\n\t" \
    __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t") \
    "fnstcw (%esp)\n\t" \
    "movw (%esp), %ax\n\t" \
    "movw %ax, 2(%esp)\n\t" \
    "testw $" #MASK ", %ax\n\t" \
    "jz 1f\n\t" \
    "andw $~" #MASK ", %ax\n\t" \
    "movw %ax, 2(%esp)\n\t" \
    "fldcw 2(%esp)\n\t" \
    "1:\n\t"

#define RESET_X87_CW \
    "movw (%esp), %ax\n\t" \
    "cmpw %ax, 2(%esp)\n\t" \
    "je 1f\n\t" \
    "fstpl 8(%esp)\n\t" \
    "fldcw (%esp)\n\t" \
    "fldl 8(%esp)\n\t" \
    "fwait\n\t" \
    "1:\n\t" \
    "addl $4, %esp\n\t" \
    __ASM_CFI(".cfi_adjust_cfa_offset -4\n\t")

/*********************************************************************
 *      _matherr (CRTDLL.@)
 */
int CDECL _matherr(struct _exception *e)
{
    return 0;
}


double math_error(int type, const char *name, double arg1, double arg2, double retval)
{
    struct _exception exception = {type, (char *)name, arg1, arg2, retval};

    TRACE("(%d, %s, %g, %g, %g)\n", type, debugstr_a(name), arg1, arg2, retval);

    if (MSVCRT_default_matherr_func && MSVCRT_default_matherr_func(&exception))
        return exception.retval;

    switch (type)
    {
    case 0:
        /* don't set errno */
        break;
    case _DOMAIN:
        *_errno() = EDOM;
        break;
    case _SING:
    case _OVERFLOW:
        *_errno() = ERANGE;
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
void CDECL __setusermatherr(MSVCRT_matherr_func func)
{
    MSVCRT_default_matherr_func = func;
    TRACE("new matherr handler %p\n", func);
}

/*********************************************************************
 *      _set_SSE2_enable (MSVCRT.@)
 */
int CDECL _set_SSE2_enable(int flag)
{
    sse2_enabled = flag && sse2_supported;
    return sse2_enabled;
}

#if defined(_WIN64)
# if _MSVCR_VER>=140
/*********************************************************************
 *      _get_FMA3_enable (UCRTBASE.@)
 */
int CDECL _get_FMA3_enable(void)
{
    FIXME("() stub\n");
    return 0;
}
# endif

# if _MSVCR_VER>=120
/*********************************************************************
 *      _set_FMA3_enable (MSVCR120.@)
 */
int CDECL _set_FMA3_enable(int flag)
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
float CDECL _chgsignf( float num )
{
    union { float f; UINT32 i; } u = { num };
    u.i ^= 0x80000000;
    return u.f;
}

#endif

#ifndef __i386__
static const UINT64 exp2f_T[] = {
    0x3ff0000000000000ULL, 0x3fefd9b0d3158574ULL, 0x3fefb5586cf9890fULL, 0x3fef9301d0125b51ULL,
    0x3fef72b83c7d517bULL, 0x3fef54873168b9aaULL, 0x3fef387a6e756238ULL, 0x3fef1e9df51fdee1ULL,
    0x3fef06fe0a31b715ULL, 0x3feef1a7373aa9cbULL, 0x3feedea64c123422ULL, 0x3feece086061892dULL,
    0x3feebfdad5362a27ULL, 0x3feeb42b569d4f82ULL, 0x3feeab07dd485429ULL, 0x3feea47eb03a5585ULL,
    0x3feea09e667f3bcdULL, 0x3fee9f75e8ec5f74ULL, 0x3feea11473eb0187ULL, 0x3feea589994cce13ULL,
    0x3feeace5422aa0dbULL, 0x3feeb737b0cdc5e5ULL, 0x3feec49182a3f090ULL, 0x3feed503b23e255dULL,
    0x3feee89f995ad3adULL, 0x3feeff76f2fb5e47ULL, 0x3fef199bdd85529cULL, 0x3fef3720dcef9069ULL,
    0x3fef5818dcfba487ULL, 0x3fef7c97337b9b5fULL, 0x3fefa4afa2a490daULL, 0x3fefd0765b6e4540ULL
};
#endif

/*********************************************************************
 *      _fdclass (MSVCR120.@)
 *
 * Copied from musl: src/math/__fpclassifyf.c
 */
short CDECL _fdclass(float x)
{
    union { float f; UINT32 i; } u = { x };
    int e = u.i >> 23 & 0xff;

    if (!e) return u.i << 1 ? FP_SUBNORMAL : FP_ZERO;
    if (e == 0xff) return u.i << 9 ? FP_NAN : FP_INFINITE;
    return FP_NORMAL;
}

/*********************************************************************
 *      _dclass (MSVCR120.@)
 *
 * Copied from musl: src/math/__fpclassify.c
 */
short CDECL _dclass(double x)
{
    union { double f; UINT64 i; } u = { x };
    int e = u.i >> 52 & 0x7ff;

    if (!e) return u.i << 1 ? FP_SUBNORMAL : FP_ZERO;
    if (e == 0x7ff) return (u.i << 12) ? FP_NAN : FP_INFINITE;
    return FP_NORMAL;
}

#ifndef __i386__

/*********************************************************************
 *      _fpclassf (MSVCRT.@)
 */
int CDECL _fpclassf( float num )
{
    union { float f; UINT32 i; } u = { num };
    int e = u.i >> 23 & 0xff;
    int s = u.i >> 31;

    switch (e)
    {
    case 0:
        if (u.i << 1) return s ? _FPCLASS_ND : _FPCLASS_PD;
        return s ? _FPCLASS_NZ : _FPCLASS_PZ;
    case 0xff:
        if (u.i << 9) return ((u.i >> 22) & 1) ? _FPCLASS_QNAN : _FPCLASS_SNAN;
        return s ? _FPCLASS_NINF : _FPCLASS_PINF;
    default:
        return s ? _FPCLASS_NN : _FPCLASS_PN;
    }
}

/*********************************************************************
 *      _finitef (MSVCRT.@)
 */
int CDECL _finitef( float num )
{
    union { float f; UINT32 i; } u = { num };
    return (u.i & 0x7fffffff) < 0x7f800000;
}

/*********************************************************************
 *      _isnanf (MSVCRT.@)
 */
int CDECL _isnanf( float num )
{
    union { float f; UINT32 i; } u = { num };
    return (u.i & 0x7fffffff) > 0x7f800000;
}

static float asinf_R(float z)
{
    /* coefficients for R(x^2) */
    static const float p1 = 1.66666672e-01,
                 p2 = -5.11644611e-02,
                 p3 = -1.21124933e-02,
                 p4 = -3.58742251e-03,
                 q1 = -7.56982703e-01;

    float p, q;
    p = z * (p1 + z * (p2 + z * (p3 + z * p4)));
    q = 1.0f + z * q1;
    return p / q;
}

/*********************************************************************
 *      acosf (MSVCRT.@)
 *
 * Copied from musl: src/math/acosf.c
 */
float CDECL acosf( float x )
{
    static const double pio2_lo = 6.12323399573676603587e-17;
    static const double pio2_hi = 1.57079632679489655800e+00;

    float z, w, s, c, df;
    unsigned int hx, ix;

    hx = *(unsigned int*)&x;
    ix = hx & 0x7fffffff;
    /* |x| >= 1 or nan */
    if (ix >= 0x3f800000) {
        if (ix == 0x3f800000) {
            if (hx >> 31)
                return M_PI;
            return 0;
        }
        if (isnan(x)) return x;
        return math_error(_DOMAIN, "acosf", x, 0, 0 / (x - x));
    }
    /* |x| < 0.5 */
    if (ix < 0x3f000000) {
        if (ix <= 0x32800000) /* |x| < 2**-26 */
            return M_PI_2;
        return pio2_hi - (x - (pio2_lo - x * asinf_R(x * x)));
    }
    /* x < -0.5 */
    if (hx >> 31) {
        z = (1 + x) * 0.5f;
        s = sqrtf(z);
        return 2*(pio2_hi - (s + (asinf_R(z) * s - pio2_lo)));
    }
    /* x > 0.5 */
    z = (1 - x) * 0.5f;
    s = sqrtf(z);
    hx = *(unsigned int*)&s & 0xffff0000;
    df = *(float*)&hx;
    c = (z - df * df) / (s + df);
    w = asinf_R(z) * s + c;
    return 2 * (df + w);
}

/*********************************************************************
 *      asinf (MSVCRT.@)
 *
 * Copied from musl: src/math/asinf.c
 */
float CDECL asinf( float x )
{
    static const double pio2 = 1.570796326794896558e+00;
    static const float pio4_hi = 0.785398125648;
    static const float pio2_lo = 7.54978941586e-08;

    float s, z, f, c;
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
    s = sqrtf(z);
    /* f+c = sqrt(z) */
    *(unsigned int*)&f = *(unsigned int*)&s & 0xffff0000;
    c = (z - f * f) / (s + f);
    x = pio4_hi - (2 * s * asinf_R(z) - (pio2_lo - 2 * c) - (pio4_hi - 2 * f));
    if (hx >> 31)
        return -x;
    return x;
}

/*********************************************************************
 *      atanf (MSVCRT.@)
 *
 * Copied from musl: src/math/atanf.c
 */
float CDECL atanf( float x )
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
 *      expf (MSVCRT.@)
 */
float CDECL expf( float x )
{
    static const double C[] = {
        0x1.c6af84b912394p-5 / (1 << 5) / (1 << 5) / (1 << 5),
        0x1.ebfce50fac4f3p-3 / (1 << 5) / (1 << 5),
        0x1.62e42ff0c52d6p-1 / (1 << 5)
    };
    static const double invln2n = 0x1.71547652b82fep+0 * (1 << 5);

    double kd, z, r, r2, y, s;
    UINT32 abstop;
    UINT64 ki, t;

    abstop = (*(UINT32*)&x >> 20) & 0x7ff;
    if (abstop >= 0x42b) {
        /* |x| >= 88 or x is nan.  */
        if (*(UINT32*)&x == 0xff800000)
            return 0.0f;
        if (abstop >= 0x7f8)
            return x + x;
        if (x > 0x1.62e42ep6f) /* x > log(0x1p128) ~= 88.72 */
            return math_error(_OVERFLOW, "expf", x, 0, x * FLT_MAX);
        if (x < -0x1.9fe368p6f) /* x < log(0x1p-150) ~= -103.97 */
            return math_error(_UNDERFLOW, "expf", x, 0, fp_barrierf(FLT_MIN) * FLT_MIN);
    }

    /* x*N/Ln2 = k + r with r in [-1/2, 1/2] and int k.  */
    z = invln2n * x;

    /* Round and convert z to int, the result is in [-150*N, 128*N] and
       ideally ties-to-even rule is used, otherwise the magnitude of r
       can be bigger which gives larger approximation error.  */
    kd = round(z);
    ki = (INT64)kd;
    r = z - kd;

    /* exp(x) = 2^(k/N) * 2^(r/N) ~= s * (C0*r^3 + C1*r^2 + C2*r + 1) */
    t = exp2f_T[ki % (1 << 5)];
    t += ki << (52 - 5);
    s = *(double*)&t;
    z = C[0] * r + C[1];
    r2 = r * r;
    y = C[2] * r + 1;
    y = z * r2 + y;
    y = y * s;
    return y;
}

/* Subnormal input is normalized so ix has negative biased exponent.
   Output is multiplied by POWF_SCALE (where 1 << 5). */
static double powf_log2(UINT32 ix)
{
    static const struct {
        double invc, logc;
    } T[] = {
        { 0x1.661ec79f8f3bep+0, -0x1.efec65b963019p-2 * (1 << 5) },
        { 0x1.571ed4aaf883dp+0, -0x1.b0b6832d4fca4p-2 * (1 << 5) },
        { 0x1.49539f0f010bp+0, -0x1.7418b0a1fb77bp-2 * (1 << 5) },
        { 0x1.3c995b0b80385p+0, -0x1.39de91a6dcf7bp-2 * (1 << 5) },
        { 0x1.30d190c8864a5p+0, -0x1.01d9bf3f2b631p-2 * (1 << 5) },
        { 0x1.25e227b0b8eap+0, -0x1.97c1d1b3b7afp-3 * (1 << 5) },
        { 0x1.1bb4a4a1a343fp+0, -0x1.2f9e393af3c9fp-3 * (1 << 5) },
        { 0x1.12358f08ae5bap+0, -0x1.960cbbf788d5cp-4 * (1 << 5) },
        { 0x1.0953f419900a7p+0, -0x1.a6f9db6475fcep-5 * (1 << 5) },
        { 0x1p+0, 0x0p+0 * (1 << 4) },
        { 0x1.e608cfd9a47acp-1, 0x1.338ca9f24f53dp-4 * (1 << 5) },
        { 0x1.ca4b31f026aap-1, 0x1.476a9543891bap-3 * (1 << 5) },
        { 0x1.b2036576afce6p-1, 0x1.e840b4ac4e4d2p-3 * (1 << 5) },
        { 0x1.9c2d163a1aa2dp-1, 0x1.40645f0c6651cp-2 * (1 << 5) },
        { 0x1.886e6037841edp-1, 0x1.88e9c2c1b9ff8p-2 * (1 << 5) },
        { 0x1.767dcf5534862p-1, 0x1.ce0a44eb17bccp-2 * (1 << 5) }
    };
    static const double A[] = {
        0x1.27616c9496e0bp-2 * (1 << 5), -0x1.71969a075c67ap-2 * (1 << 5),
        0x1.ec70a6ca7baddp-2 * (1 << 5), -0x1.7154748bef6c8p-1 * (1 << 5),
        0x1.71547652ab82bp0 * (1 << 5)
    };

    double z, r, r2, r4, p, q, y, y0, invc, logc;
    UINT32 iz, top, tmp;
    int k, i;

    /* x = 2^k z; where z is in range [OFF,2*OFF] and exact.
       The range is split into N subintervals.
       The ith subinterval contains z and c is near its center. */
    tmp = ix - 0x3f330000;
    i = (tmp >> (23 - 4)) % (1 << 4);
    top = tmp & 0xff800000;
    iz = ix - top;
    k = (INT32)top >> (23 - 5); /* arithmetic shift */
    invc = T[i].invc;
    logc = T[i].logc;
    z = *(float*)&iz;

    /* log2(x) = log1p(z/c-1)/ln2 + log2(c) + k */
    r = z * invc - 1;
    y0 = logc + (double)k;

    /* Pipelined polynomial evaluation to approximate log1p(r)/ln2. */
    r2 = r * r;
    y = A[0] * r + A[1];
    p = A[2] * r + A[3];
    r4 = r2 * r2;
    q = A[4] * r + y0;
    q = p * r2 + q;
    y = y * r4 + q;
    return y;
}

/* The output of log2 and thus the input of exp2 is either scaled by N
   (in case of fast toint intrinsics) or not. The unscaled xd must be
   in [-1021,1023], sign_bias sets the sign of the result. */
static float powf_exp2(double xd, UINT32 sign_bias)
{
    static const double C[] = {
        0x1.c6af84b912394p-5 / (1 << 5) / (1 << 5) / (1 << 5),
        0x1.ebfce50fac4f3p-3 / (1 << 5) / (1 << 5),
        0x1.62e42ff0c52d6p-1 / (1 << 5)
    };

    UINT64 ki, ski, t;
    double kd, z, r, r2, y, s;

    /* N*x = k + r with r in [-1/2, 1/2] */
    kd = round(xd); /* k */
    ki = (INT64)kd;
    r = xd - kd;

    /* exp2(x) = 2^(k/N) * 2^r ~= s * (C0*r^3 + C1*r^2 + C2*r + 1) */
    t = exp2f_T[ki % (1 << 5)];
    ski = ki + sign_bias;
    t += ski << (52 - 5);
    s = *(double*)&t;
    z = C[0] * r + C[1];
    r2 = r * r;
    y = C[2] * r + 1;
    y = z * r2 + y;
    y = y * s;
    return y;
}

/* Returns 0 if not int, 1 if odd int, 2 if even int. The argument is
   the bit representation of a non-zero finite floating-point value. */
static int powf_checkint(UINT32 iy)
{
    int e = iy >> 23 & 0xff;
    if (e < 0x7f)
        return 0;
    if (e > 0x7f + 23)
        return 2;
    if (iy & ((1 << (0x7f + 23 - e)) - 1))
        return 0;
    if (iy & (1 << (0x7f + 23 - e)))
        return 1;
    return 2;
}

/*********************************************************************
 *      powf (MSVCRT.@)
 *
 * Copied from musl: src/math/powf.c src/math/powf_data.c
 */
float CDECL powf( float x, float y )
{
    UINT32 sign_bias = 0;
    UINT32 ix, iy;
    double logx, ylogx;

    ix = *(UINT32*)&x;
    iy = *(UINT32*)&y;
    if (ix - 0x00800000 >= 0x7f800000 - 0x00800000 ||
            2 * iy - 1 >= 2u * 0x7f800000 - 1) {
        /* Either (x < 0x1p-126 or inf or nan) or (y is 0 or inf or nan). */
        if (2 * iy - 1 >= 2u * 0x7f800000 - 1) {
            if (2 * iy == 0)
                return 1.0f;
            if (ix == 0x3f800000)
                return 1.0f;
            if (2 * ix > 2u * 0x7f800000 || 2 * iy > 2u * 0x7f800000)
                return x + y;
            if (2 * ix == 2 * 0x3f800000)
                return 1.0f;
            if ((2 * ix < 2 * 0x3f800000) == !(iy & 0x80000000))
                return 0.0f; /* |x|<1 && y==inf or |x|>1 && y==-inf. */
            return y * y;
        }
        if (2 * ix - 1 >= 2u * 0x7f800000 - 1) {
            float x2 = x * x;
            if (ix & 0x80000000 && powf_checkint(iy) == 1)
                x2 = -x2;
            if (iy & 0x80000000 && x2 == 0.0)
                return math_error(_SING, "powf", x, y, 1 / x2);
            /* Without the barrier some versions of clang hoist the 1/x2 and
               thus division by zero exception can be signaled spuriously. */
            return iy & 0x80000000 ? fp_barrierf(1 / x2) : x2;
        }
        /* x and y are non-zero finite. */
        if (ix & 0x80000000) {
            /* Finite x < 0. */
            int yint = powf_checkint(iy);
            if (yint == 0)
                return math_error(_DOMAIN, "powf", x, y, 0 / (x - x));
            if (yint == 1)
                sign_bias = 1 << (5 + 11);
            ix &= 0x7fffffff;
        }
        if (ix < 0x00800000) {
            /* Normalize subnormal x so exponent becomes negative. */
            x *= 0x1p23f;
            ix = *(UINT32*)&x;
            ix &= 0x7fffffff;
            ix -= 23 << 23;
        }
    }
    logx = powf_log2(ix);
    ylogx = y * logx; /* cannot overflow, y is single prec. */
    if ((*(UINT64*)&ylogx >> 47 & 0xffff) >= 0x40af800000000000llu >> 47) {
        /* |y*log(x)| >= 126. */
        if (ylogx > 0x1.fffffffd1d571p+6 * (1 << 5))
            return math_error(_OVERFLOW, "powf", x, y, (sign_bias ? -1.0 : 1.0) * 0x1p1023);
        if (ylogx <= -150.0 * (1 << 5))
            return math_error(_UNDERFLOW, "powf", x, y, (sign_bias ? -1.0 : 1.0) / 0x1p1023);
    }
    return powf_exp2(ylogx, sign_bias);
}

static BOOL sqrtf_validate( float *x )
{
    short c = _fdclass(*x);

    if (c == FP_ZERO) return FALSE;
    if (c == FP_NAN) return FALSE;
    if (signbit(*x))
    {
        *x = math_error(_DOMAIN, "sqrtf", *x, 0, ret_nan(TRUE));
        return FALSE;
    }
    if (c == FP_INFINITE) return FALSE;
    return TRUE;
}

#if defined(__x86_64__) || defined(__i386__)
float CDECL sse2_sqrtf(float);
__ASM_GLOBAL_FUNC( sse2_sqrtf,
        "sqrtss %xmm0, %xmm0\n\t"
        "ret" )
#endif

/*********************************************************************
 *      sqrtf (MSVCRT.@)
 *
 * Copied from musl: src/math/sqrtf.c
 */
float CDECL sqrtf( float x )
{
#ifdef __x86_64__
    if (!sqrtf_validate(&x))
        return x;

    return sse2_sqrtf(x);
#else
    static const float tiny = 1.0e-30;

    float z;
    int ix,s,q,m,t,i;
    unsigned int r;

    ix = *(int*)&x;

    if (!sqrtf_validate(&x))
        return x;

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
#endif
}

/*********************************************************************
 *      tanhf (MSVCRT.@)
 */
float CDECL tanhf( float x )
{
    UINT32 ui = *(UINT32*)&x;
    UINT32 sign = ui & 0x80000000;
    float t;

    /* x = |x| */
    ui &= 0x7fffffff;
    x = *(float*)&ui;

    if (ui > 0x3f0c9f54) {
        /* |x| > log(3)/2 ~= 0.5493 or nan */
        if (ui > 0x41200000) {
            if (ui > 0x7f800000) {
                *(UINT32*)&x = ui | sign | 0x400000;
#if _MSVCR_VER < 140
                return math_error(_DOMAIN, "tanhf", x, 0, x);
#else
                return x;
#endif
            }
            /* |x| > 10 */
            fp_barrierf(x + 0x1p120f);
            t = 1 + 0 / x;
        } else {
            t = expm1f(2 * x);
            t = 1 - 2 / (t + 2);
        }
    } else if (ui > 0x3e82c578) {
        /* |x| > log(5/3)/2 ~= 0.2554 */
        t = expm1f(2 * x);
        t = t / (t + 2);
    } else if (ui >= 0x00800000) {
        /* |x| >= 0x1p-126 */
        t = expm1f(-2 * x);
        t = -t / (t + 2);
    } else {
        /* |x| is subnormal */
        fp_barrierf(x * x);
        t = x;
    }
    return sign ? -t : t;
}

#endif

/*********************************************************************
 *		acos (MSVCRT.@)
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

double CDECL acos( double x )
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
 *		asin (MSVCRT.@)
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

#ifdef __i386__
double CDECL x87_asin(double);
__ASM_GLOBAL_FUNC( x87_asin,
        "fldl 4(%esp)\n\t"
        SET_X87_CW(~0x37f)
        "fld %st\n\t"
        "fld1\n\t"
        "fsubp\n\t"
        "fld1\n\t"
        "fadd %st(2)\n\t"
        "fmulp\n\t"
        "fsqrt\n\t"
        "fpatan\n\t"
        RESET_X87_CW
        "ret" )
#endif

double CDECL asin( double x )
{
    static const double pio2_hi = 1.57079632679489655800e+00,
                 pio2_lo = 6.12323399573676603587e-17;

    double z, r, s;
    unsigned int hx, ix;
    ULONGLONG llx;
#ifdef __i386__
    unsigned int x87_cw, sse2_cw;
#endif

    hx = *(ULONGLONG*)&x >> 32;
    ix = hx & 0x7fffffff;
    /* |x| >= 1 or nan */
    if (ix >= 0x3ff00000) {
        unsigned int lx;
        lx = *(ULONGLONG*)&x;
        if (((ix - 0x3ff00000) | lx) == 0)
            /* asin(1) = +-pi/2 with inexact */
            return x * pio2_hi + 7.5231638452626401e-37;
        if (isnan(x))
        {
#ifdef __i386__
            return math_error(_DOMAIN, "asin", x, 0, x);
#else
            return x;
#endif
        }
        return math_error(_DOMAIN, "asin", x, 0, 0 / (x - x));
    }

#ifdef __i386__
    __control87_2(0, 0, &x87_cw, &sse2_cw);
    if (!sse2_enabled || (x87_cw & _MCW_EM) != _MCW_EM
            || (sse2_cw & (_MCW_EM | _MCW_RC)) != _MCW_EM)
        return x87_asin(x);
#endif

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
 *		atan (MSVCRT.@)
 *
 * Copied from musl: src/math/atan.c
 */
double CDECL atan( double x )
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
 *      rint (MSVCR120.@)
 *
 * Copied from musl: src/math/rint.c
 */
double CDECL rint(double x)
{
    static const double toint = 1 / DBL_EPSILON;

    ULONGLONG llx = *(ULONGLONG*)&x;
    int e = llx >> 52 & 0x7ff;
    int s = llx >> 63;
    unsigned cw;
    double y;

    if (e >= 0x3ff+52)
        return x;
    cw = _controlfp(0, 0);
    if ((cw & _MCW_PC) != _PC_53)
        _controlfp(_PC_53, _MCW_PC);
    if (s)
        y = fp_barrier(x - toint) + toint;
    else
        y = fp_barrier(x + toint) - toint;
    if ((cw & _MCW_PC) != _PC_53)
        _controlfp(cw, _MCW_PC);
    if (y == 0)
        return s ? -0.0 : 0;
    return y;
}

/* Copied from musl: src/math/exp_data.c */
static const UINT64 exp_T[] = {
    0x0ULL, 0x3ff0000000000000ULL,
    0x3c9b3b4f1a88bf6eULL, 0x3feff63da9fb3335ULL,
    0xbc7160139cd8dc5dULL, 0x3fefec9a3e778061ULL,
    0xbc905e7a108766d1ULL, 0x3fefe315e86e7f85ULL,
    0x3c8cd2523567f613ULL, 0x3fefd9b0d3158574ULL,
    0xbc8bce8023f98efaULL, 0x3fefd06b29ddf6deULL,
    0x3c60f74e61e6c861ULL, 0x3fefc74518759bc8ULL,
    0x3c90a3e45b33d399ULL, 0x3fefbe3ecac6f383ULL,
    0x3c979aa65d837b6dULL, 0x3fefb5586cf9890fULL,
    0x3c8eb51a92fdeffcULL, 0x3fefac922b7247f7ULL,
    0x3c3ebe3d702f9cd1ULL, 0x3fefa3ec32d3d1a2ULL,
    0xbc6a033489906e0bULL, 0x3fef9b66affed31bULL,
    0xbc9556522a2fbd0eULL, 0x3fef9301d0125b51ULL,
    0xbc5080ef8c4eea55ULL, 0x3fef8abdc06c31ccULL,
    0xbc91c923b9d5f416ULL, 0x3fef829aaea92de0ULL,
    0x3c80d3e3e95c55afULL, 0x3fef7a98c8a58e51ULL,
    0xbc801b15eaa59348ULL, 0x3fef72b83c7d517bULL,
    0xbc8f1ff055de323dULL, 0x3fef6af9388c8deaULL,
    0x3c8b898c3f1353bfULL, 0x3fef635beb6fcb75ULL,
    0xbc96d99c7611eb26ULL, 0x3fef5be084045cd4ULL,
    0x3c9aecf73e3a2f60ULL, 0x3fef54873168b9aaULL,
    0xbc8fe782cb86389dULL, 0x3fef4d5022fcd91dULL,
    0x3c8a6f4144a6c38dULL, 0x3fef463b88628cd6ULL,
    0x3c807a05b0e4047dULL, 0x3fef3f49917ddc96ULL,
    0x3c968efde3a8a894ULL, 0x3fef387a6e756238ULL,
    0x3c875e18f274487dULL, 0x3fef31ce4fb2a63fULL,
    0x3c80472b981fe7f2ULL, 0x3fef2b4565e27cddULL,
    0xbc96b87b3f71085eULL, 0x3fef24dfe1f56381ULL,
    0x3c82f7e16d09ab31ULL, 0x3fef1e9df51fdee1ULL,
    0xbc3d219b1a6fbffaULL, 0x3fef187fd0dad990ULL,
    0x3c8b3782720c0ab4ULL, 0x3fef1285a6e4030bULL,
    0x3c6e149289cecb8fULL, 0x3fef0cafa93e2f56ULL,
    0x3c834d754db0abb6ULL, 0x3fef06fe0a31b715ULL,
    0x3c864201e2ac744cULL, 0x3fef0170fc4cd831ULL,
    0x3c8fdd395dd3f84aULL, 0x3feefc08b26416ffULL,
    0xbc86a3803b8e5b04ULL, 0x3feef6c55f929ff1ULL,
    0xbc924aedcc4b5068ULL, 0x3feef1a7373aa9cbULL,
    0xbc9907f81b512d8eULL, 0x3feeecae6d05d866ULL,
    0xbc71d1e83e9436d2ULL, 0x3feee7db34e59ff7ULL,
    0xbc991919b3ce1b15ULL, 0x3feee32dc313a8e5ULL,
    0x3c859f48a72a4c6dULL, 0x3feedea64c123422ULL,
    0xbc9312607a28698aULL, 0x3feeda4504ac801cULL,
    0xbc58a78f4817895bULL, 0x3feed60a21f72e2aULL,
    0xbc7c2c9b67499a1bULL, 0x3feed1f5d950a897ULL,
    0x3c4363ed60c2ac11ULL, 0x3feece086061892dULL,
    0x3c9666093b0664efULL, 0x3feeca41ed1d0057ULL,
    0x3c6ecce1daa10379ULL, 0x3feec6a2b5c13cd0ULL,
    0x3c93ff8e3f0f1230ULL, 0x3feec32af0d7d3deULL,
    0x3c7690cebb7aafb0ULL, 0x3feebfdad5362a27ULL,
    0x3c931dbdeb54e077ULL, 0x3feebcb299fddd0dULL,
    0xbc8f94340071a38eULL, 0x3feeb9b2769d2ca7ULL,
    0xbc87deccdc93a349ULL, 0x3feeb6daa2cf6642ULL,
    0xbc78dec6bd0f385fULL, 0x3feeb42b569d4f82ULL,
    0xbc861246ec7b5cf6ULL, 0x3feeb1a4ca5d920fULL,
    0x3c93350518fdd78eULL, 0x3feeaf4736b527daULL,
    0x3c7b98b72f8a9b05ULL, 0x3feead12d497c7fdULL,
    0x3c9063e1e21c5409ULL, 0x3feeab07dd485429ULL,
    0x3c34c7855019c6eaULL, 0x3feea9268a5946b7ULL,
    0x3c9432e62b64c035ULL, 0x3feea76f15ad2148ULL,
    0xbc8ce44a6199769fULL, 0x3feea5e1b976dc09ULL,
    0xbc8c33c53bef4da8ULL, 0x3feea47eb03a5585ULL,
    0xbc845378892be9aeULL, 0x3feea34634ccc320ULL,
    0xbc93cedd78565858ULL, 0x3feea23882552225ULL,
    0x3c5710aa807e1964ULL, 0x3feea155d44ca973ULL,
    0xbc93b3efbf5e2228ULL, 0x3feea09e667f3bcdULL,
    0xbc6a12ad8734b982ULL, 0x3feea012750bdabfULL,
    0xbc6367efb86da9eeULL, 0x3fee9fb23c651a2fULL,
    0xbc80dc3d54e08851ULL, 0x3fee9f7df9519484ULL,
    0xbc781f647e5a3ecfULL, 0x3fee9f75e8ec5f74ULL,
    0xbc86ee4ac08b7db0ULL, 0x3fee9f9a48a58174ULL,
    0xbc8619321e55e68aULL, 0x3fee9feb564267c9ULL,
    0x3c909ccb5e09d4d3ULL, 0x3feea0694fde5d3fULL,
    0xbc7b32dcb94da51dULL, 0x3feea11473eb0187ULL,
    0x3c94ecfd5467c06bULL, 0x3feea1ed0130c132ULL,
    0x3c65ebe1abd66c55ULL, 0x3feea2f336cf4e62ULL,
    0xbc88a1c52fb3cf42ULL, 0x3feea427543e1a12ULL,
    0xbc9369b6f13b3734ULL, 0x3feea589994cce13ULL,
    0xbc805e843a19ff1eULL, 0x3feea71a4623c7adULL,
    0xbc94d450d872576eULL, 0x3feea8d99b4492edULL,
    0x3c90ad675b0e8a00ULL, 0x3feeaac7d98a6699ULL,
    0x3c8db72fc1f0eab4ULL, 0x3feeace5422aa0dbULL,
    0xbc65b6609cc5e7ffULL, 0x3feeaf3216b5448cULL,
    0x3c7bf68359f35f44ULL, 0x3feeb1ae99157736ULL,
    0xbc93091fa71e3d83ULL, 0x3feeb45b0b91ffc6ULL,
    0xbc5da9b88b6c1e29ULL, 0x3feeb737b0cdc5e5ULL,
    0xbc6c23f97c90b959ULL, 0x3feeba44cbc8520fULL,
    0xbc92434322f4f9aaULL, 0x3feebd829fde4e50ULL,
    0xbc85ca6cd7668e4bULL, 0x3feec0f170ca07baULL,
    0x3c71affc2b91ce27ULL, 0x3feec49182a3f090ULL,
    0x3c6dd235e10a73bbULL, 0x3feec86319e32323ULL,
    0xbc87c50422622263ULL, 0x3feecc667b5de565ULL,
    0x3c8b1c86e3e231d5ULL, 0x3feed09bec4a2d33ULL,
    0xbc91bbd1d3bcbb15ULL, 0x3feed503b23e255dULL,
    0x3c90cc319cee31d2ULL, 0x3feed99e1330b358ULL,
    0x3c8469846e735ab3ULL, 0x3feede6b5579fdbfULL,
    0xbc82dfcd978e9db4ULL, 0x3feee36bbfd3f37aULL,
    0x3c8c1a7792cb3387ULL, 0x3feee89f995ad3adULL,
    0xbc907b8f4ad1d9faULL, 0x3feeee07298db666ULL,
    0xbc55c3d956dcaebaULL, 0x3feef3a2b84f15fbULL,
    0xbc90a40e3da6f640ULL, 0x3feef9728de5593aULL,
    0xbc68d6f438ad9334ULL, 0x3feeff76f2fb5e47ULL,
    0xbc91eee26b588a35ULL, 0x3fef05b030a1064aULL,
    0x3c74ffd70a5fddcdULL, 0x3fef0c1e904bc1d2ULL,
    0xbc91bdfbfa9298acULL, 0x3fef12c25bd71e09ULL,
    0x3c736eae30af0cb3ULL, 0x3fef199bdd85529cULL,
    0x3c8ee3325c9ffd94ULL, 0x3fef20ab5fffd07aULL,
    0x3c84e08fd10959acULL, 0x3fef27f12e57d14bULL,
    0x3c63cdaf384e1a67ULL, 0x3fef2f6d9406e7b5ULL,
    0x3c676b2c6c921968ULL, 0x3fef3720dcef9069ULL,
    0xbc808a1883ccb5d2ULL, 0x3fef3f0b555dc3faULL,
    0xbc8fad5d3ffffa6fULL, 0x3fef472d4a07897cULL,
    0xbc900dae3875a949ULL, 0x3fef4f87080d89f2ULL,
    0x3c74a385a63d07a7ULL, 0x3fef5818dcfba487ULL,
    0xbc82919e2040220fULL, 0x3fef60e316c98398ULL,
    0x3c8e5a50d5c192acULL, 0x3fef69e603db3285ULL,
    0x3c843a59ac016b4bULL, 0x3fef7321f301b460ULL,
    0xbc82d52107b43e1fULL, 0x3fef7c97337b9b5fULL,
    0xbc892ab93b470dc9ULL, 0x3fef864614f5a129ULL,
    0x3c74b604603a88d3ULL, 0x3fef902ee78b3ff6ULL,
    0x3c83c5ec519d7271ULL, 0x3fef9a51fbc74c83ULL,
    0xbc8ff7128fd391f0ULL, 0x3fefa4afa2a490daULL,
    0xbc8dae98e223747dULL, 0x3fefaf482d8e67f1ULL,
    0x3c8ec3bc41aa2008ULL, 0x3fefba1bee615a27ULL,
    0x3c842b94c3a9eb32ULL, 0x3fefc52b376bba97ULL,
    0x3c8a64a931d185eeULL, 0x3fefd0765b6e4540ULL,
    0xbc8e37bae43be3edULL, 0x3fefdbfdad9cbe14ULL,
    0x3c77893b4d91cd9dULL, 0x3fefe7c1819e90d8ULL,
    0x3c5305c14160cc89ULL, 0x3feff3c22b8f71f1ULL
};

/*********************************************************************
 *		exp (MSVCRT.@)
 *
 * Copied from musl: src/math/exp.c
 */
double CDECL exp( double x )
{
    static const double C[] = {
        0x1.ffffffffffdbdp-2,
        0x1.555555555543cp-3,
        0x1.55555cf172b91p-5,
        0x1.1111167a4d017p-7
    };
    static const double invln2N = 0x1.71547652b82fep0 * (1 << 7),
        negln2hiN = -0x1.62e42fefa0000p-8,
        negln2loN = -0x1.cf79abc9e3b3ap-47;

    UINT32 abstop;
    UINT64 ki, idx, top, sbits;
    double kd, z, r, r2, scale, tail, tmp;

    abstop = (*(UINT64*)&x >> 52) & 0x7ff;
    if (abstop -  0x3c9 >= 0x408 - 0x3c9) {
        if (abstop - 0x3c9 >= 0x80000000)
            /* Avoid spurious underflow for tiny x. */
            /* Note: 0 is common input. */
            return 1.0 + x;
        if (abstop >= 0x409) {
            if (*(UINT64*)&x == 0xfff0000000000000ULL)
                return 0.0;
#if _MSVCR_VER == 0
            if (*(UINT64*)&x > 0x7ff0000000000000ULL)
                return math_error(_DOMAIN, "exp", x, 0, 1.0 + x);
#endif
            if (abstop >= 0x7ff)
                return 1.0 + x;
            if (*(UINT64*)&x >> 63)
                return math_error(_UNDERFLOW, "exp", x, 0, fp_barrier(DBL_MIN) * DBL_MIN);
            else
                return math_error(_OVERFLOW, "exp", x, 0, fp_barrier(DBL_MAX) * DBL_MAX);
        }
        /* Large x is special cased below. */
        abstop = 0;
    }

    /* exp(x) = 2^(k/N) * exp(r), with exp(r) in [2^(-1/2N),2^(1/2N)]. */
    /* x = ln2/N*k + r, with int k and r in [-ln2/2N, ln2/2N]. */
    z = invln2N * x;
    kd = round(z);
    ki = (INT64)kd;

    r = x + kd * negln2hiN + kd * negln2loN;
    /* 2^(k/N) ~= scale * (1 + tail). */
    idx = 2 * (ki % (1 << 7));
    top = ki << (52 - 7);
    tail = *(double*)&exp_T[idx];
    /* This is only a valid scale when -1023*N < k < 1024*N. */
    sbits = exp_T[idx + 1] + top;
    /* exp(x) = 2^(k/N) * exp(r) ~= scale + scale * (tail + exp(r) - 1). */
    /* Evaluation is optimized assuming superscalar pipelined execution. */
    r2 = r * r;
    /* Without fma the worst case error is 0.25/N ulp larger. */
    /* Worst case error is less than 0.5+1.11/N+(abs poly error * 2^53) ulp. */
    tmp = tail + r + r2 * (C[0] + r * C[1]) + r2 * r2 * (C[2] + r * C[3]);
    if (abstop == 0) {
        /* Handle cases that may overflow or underflow when computing the result that
           is scale*(1+TMP) without intermediate rounding. The bit representation of
           scale is in SBITS, however it has a computed exponent that may have
           overflown into the sign bit so that needs to be adjusted before using it as
           a double. (int32_t)KI is the k used in the argument reduction and exponent
           adjustment of scale, positive k here means the result may overflow and
           negative k means the result may underflow. */
        double scale, y;

        if ((ki & 0x80000000) == 0) {
            /* k > 0, the exponent of scale might have overflowed by <= 460. */
            sbits -= 1009ull << 52;
            scale = *(double*)&sbits;
            y = 0x1p1009 * (scale + scale * tmp);
            if (isinf(y))
                return math_error(_OVERFLOW, "exp", x, 0, y);
            return y;
        }
        /* k < 0, need special care in the subnormal range. */
        sbits += 1022ull << 52;
        scale = *(double*)&sbits;
        y = scale + scale * tmp;
        if (y < 1.0) {
            /* Round y to the right precision before scaling it into the subnormal
               range to avoid double rounding that can cause 0.5+E/2 ulp error where
               E is the worst-case ulp error outside the subnormal range. So this
               is only useful if the goal is better than 1 ulp worst-case error. */
            double hi, lo;
            lo = scale - y + scale * tmp;
            hi = 1.0 + y;
            lo = 1.0 - hi + y + lo;
            y = hi + lo - 1.0;
            /* Avoid -0.0 with downward rounding. */
            if (y == 0.0)
                y = 0.0;
            /* The underflow exception needs to be signaled explicitly. */
            fp_barrier(fp_barrier(0x1p-1022) * 0x1p-1022);
            y = 0x1p-1022 * y;
            return math_error(_UNDERFLOW, "exp", x, 0, y);
        }
        y = 0x1p-1022 * y;
        return y;
    }
    scale = *(double*)&sbits;
    /* Note: tmp == 0 or |tmp| > 2^-200 and scale > 2^-739, so there
       is no spurious underflow here even without fma. */
    return scale + scale * tmp;
}

/* Compute y+TAIL = log(x) where the rounded result is y and TAIL has about
   additional 15 bits precision. IX is the bit representation of x, but
   normalized in the subnormal range using the sign bit for the exponent. */
static double pow_log(UINT64 ix, double *tail)
{
    static const struct {
        double invc, logc, logctail;
    } T[] = {
        {0x1.6a00000000000p+0, -0x1.62c82f2b9c800p-2, 0x1.ab42428375680p-48},
        {0x1.6800000000000p+0, -0x1.5d1bdbf580800p-2, -0x1.ca508d8e0f720p-46},
        {0x1.6600000000000p+0, -0x1.5767717455800p-2, -0x1.362a4d5b6506dp-45},
        {0x1.6400000000000p+0, -0x1.51aad872df800p-2, -0x1.684e49eb067d5p-49},
        {0x1.6200000000000p+0, -0x1.4be5f95777800p-2, -0x1.41b6993293ee0p-47},
        {0x1.6000000000000p+0, -0x1.4618bc21c6000p-2, 0x1.3d82f484c84ccp-46},
        {0x1.5e00000000000p+0, -0x1.404308686a800p-2, 0x1.c42f3ed820b3ap-50},
        {0x1.5c00000000000p+0, -0x1.3a64c55694800p-2, 0x1.0b1c686519460p-45},
        {0x1.5a00000000000p+0, -0x1.347dd9a988000p-2, 0x1.5594dd4c58092p-45},
        {0x1.5800000000000p+0, -0x1.2e8e2bae12000p-2, 0x1.67b1e99b72bd8p-45},
        {0x1.5600000000000p+0, -0x1.2895a13de8800p-2, 0x1.5ca14b6cfb03fp-46},
        {0x1.5600000000000p+0, -0x1.2895a13de8800p-2, 0x1.5ca14b6cfb03fp-46},
        {0x1.5400000000000p+0, -0x1.22941fbcf7800p-2, -0x1.65a242853da76p-46},
        {0x1.5200000000000p+0, -0x1.1c898c1699800p-2, -0x1.fafbc68e75404p-46},
        {0x1.5000000000000p+0, -0x1.1675cababa800p-2, 0x1.f1fc63382a8f0p-46},
        {0x1.4e00000000000p+0, -0x1.1058bf9ae4800p-2, -0x1.6a8c4fd055a66p-45},
        {0x1.4c00000000000p+0, -0x1.0a324e2739000p-2, -0x1.c6bee7ef4030ep-47},
        {0x1.4a00000000000p+0, -0x1.0402594b4d000p-2, -0x1.036b89ef42d7fp-48},
        {0x1.4a00000000000p+0, -0x1.0402594b4d000p-2, -0x1.036b89ef42d7fp-48},
        {0x1.4800000000000p+0, -0x1.fb9186d5e4000p-3, 0x1.d572aab993c87p-47},
        {0x1.4600000000000p+0, -0x1.ef0adcbdc6000p-3, 0x1.b26b79c86af24p-45},
        {0x1.4400000000000p+0, -0x1.e27076e2af000p-3, -0x1.72f4f543fff10p-46},
        {0x1.4200000000000p+0, -0x1.d5c216b4fc000p-3, 0x1.1ba91bbca681bp-45},
        {0x1.4000000000000p+0, -0x1.c8ff7c79aa000p-3, 0x1.7794f689f8434p-45},
        {0x1.4000000000000p+0, -0x1.c8ff7c79aa000p-3, 0x1.7794f689f8434p-45},
        {0x1.3e00000000000p+0, -0x1.bc286742d9000p-3, 0x1.94eb0318bb78fp-46},
        {0x1.3c00000000000p+0, -0x1.af3c94e80c000p-3, 0x1.a4e633fcd9066p-52},
        {0x1.3a00000000000p+0, -0x1.a23bc1fe2b000p-3, -0x1.58c64dc46c1eap-45},
        {0x1.3a00000000000p+0, -0x1.a23bc1fe2b000p-3, -0x1.58c64dc46c1eap-45},
        {0x1.3800000000000p+0, -0x1.9525a9cf45000p-3, -0x1.ad1d904c1d4e3p-45},
        {0x1.3600000000000p+0, -0x1.87fa06520d000p-3, 0x1.bbdbf7fdbfa09p-45},
        {0x1.3400000000000p+0, -0x1.7ab890210e000p-3, 0x1.bdb9072534a58p-45},
        {0x1.3400000000000p+0, -0x1.7ab890210e000p-3, 0x1.bdb9072534a58p-45},
        {0x1.3200000000000p+0, -0x1.6d60fe719d000p-3, -0x1.0e46aa3b2e266p-46},
        {0x1.3000000000000p+0, -0x1.5ff3070a79000p-3, -0x1.e9e439f105039p-46},
        {0x1.3000000000000p+0, -0x1.5ff3070a79000p-3, -0x1.e9e439f105039p-46},
        {0x1.2e00000000000p+0, -0x1.526e5e3a1b000p-3, -0x1.0de8b90075b8fp-45},
        {0x1.2c00000000000p+0, -0x1.44d2b6ccb8000p-3, 0x1.70cc16135783cp-46},
        {0x1.2c00000000000p+0, -0x1.44d2b6ccb8000p-3, 0x1.70cc16135783cp-46},
        {0x1.2a00000000000p+0, -0x1.371fc201e9000p-3, 0x1.178864d27543ap-48},
        {0x1.2800000000000p+0, -0x1.29552f81ff000p-3, -0x1.48d301771c408p-45},
        {0x1.2600000000000p+0, -0x1.1b72ad52f6000p-3, -0x1.e80a41811a396p-45},
        {0x1.2600000000000p+0, -0x1.1b72ad52f6000p-3, -0x1.e80a41811a396p-45},
        {0x1.2400000000000p+0, -0x1.0d77e7cd09000p-3, 0x1.a699688e85bf4p-47},
        {0x1.2400000000000p+0, -0x1.0d77e7cd09000p-3, 0x1.a699688e85bf4p-47},
        {0x1.2200000000000p+0, -0x1.fec9131dbe000p-4, -0x1.575545ca333f2p-45},
        {0x1.2000000000000p+0, -0x1.e27076e2b0000p-4, 0x1.a342c2af0003cp-45},
        {0x1.2000000000000p+0, -0x1.e27076e2b0000p-4, 0x1.a342c2af0003cp-45},
        {0x1.1e00000000000p+0, -0x1.c5e548f5bc000p-4, -0x1.d0c57585fbe06p-46},
        {0x1.1c00000000000p+0, -0x1.a926d3a4ae000p-4, 0x1.53935e85baac8p-45},
        {0x1.1c00000000000p+0, -0x1.a926d3a4ae000p-4, 0x1.53935e85baac8p-45},
        {0x1.1a00000000000p+0, -0x1.8c345d631a000p-4, 0x1.37c294d2f5668p-46},
        {0x1.1a00000000000p+0, -0x1.8c345d631a000p-4, 0x1.37c294d2f5668p-46},
        {0x1.1800000000000p+0, -0x1.6f0d28ae56000p-4, -0x1.69737c93373dap-45},
        {0x1.1600000000000p+0, -0x1.51b073f062000p-4, 0x1.f025b61c65e57p-46},
        {0x1.1600000000000p+0, -0x1.51b073f062000p-4, 0x1.f025b61c65e57p-46},
        {0x1.1400000000000p+0, -0x1.341d7961be000p-4, 0x1.c5edaccf913dfp-45},
        {0x1.1400000000000p+0, -0x1.341d7961be000p-4, 0x1.c5edaccf913dfp-45},
        {0x1.1200000000000p+0, -0x1.16536eea38000p-4, 0x1.47c5e768fa309p-46},
        {0x1.1000000000000p+0, -0x1.f0a30c0118000p-5, 0x1.d599e83368e91p-45},
        {0x1.1000000000000p+0, -0x1.f0a30c0118000p-5, 0x1.d599e83368e91p-45},
        {0x1.0e00000000000p+0, -0x1.b42dd71198000p-5, 0x1.c827ae5d6704cp-46},
        {0x1.0e00000000000p+0, -0x1.b42dd71198000p-5, 0x1.c827ae5d6704cp-46},
        {0x1.0c00000000000p+0, -0x1.77458f632c000p-5, -0x1.cfc4634f2a1eep-45},
        {0x1.0c00000000000p+0, -0x1.77458f632c000p-5, -0x1.cfc4634f2a1eep-45},
        {0x1.0a00000000000p+0, -0x1.39e87b9fec000p-5, 0x1.502b7f526feaap-48},
        {0x1.0a00000000000p+0, -0x1.39e87b9fec000p-5, 0x1.502b7f526feaap-48},
        {0x1.0800000000000p+0, -0x1.f829b0e780000p-6, -0x1.980267c7e09e4p-45},
        {0x1.0800000000000p+0, -0x1.f829b0e780000p-6, -0x1.980267c7e09e4p-45},
        {0x1.0600000000000p+0, -0x1.7b91b07d58000p-6, -0x1.88d5493faa639p-45},
        {0x1.0400000000000p+0, -0x1.fc0a8b0fc0000p-7, -0x1.f1e7cf6d3a69cp-50},
        {0x1.0400000000000p+0, -0x1.fc0a8b0fc0000p-7, -0x1.f1e7cf6d3a69cp-50},
        {0x1.0200000000000p+0, -0x1.fe02a6b100000p-8, -0x1.9e23f0dda40e4p-46},
        {0x1.0200000000000p+0, -0x1.fe02a6b100000p-8, -0x1.9e23f0dda40e4p-46},
        {0x1.0000000000000p+0, 0x0.0000000000000p+0, 0x0.0000000000000p+0},
        {0x1.0000000000000p+0, 0x0.0000000000000p+0, 0x0.0000000000000p+0},
        {0x1.fc00000000000p-1, 0x1.0101575890000p-7, -0x1.0c76b999d2be8p-46},
        {0x1.f800000000000p-1, 0x1.0205658938000p-6, -0x1.3dc5b06e2f7d2p-45},
        {0x1.f400000000000p-1, 0x1.8492528c90000p-6, -0x1.aa0ba325a0c34p-45},
        {0x1.f000000000000p-1, 0x1.0415d89e74000p-5, 0x1.111c05cf1d753p-47},
        {0x1.ec00000000000p-1, 0x1.466aed42e0000p-5, -0x1.c167375bdfd28p-45},
        {0x1.e800000000000p-1, 0x1.894aa149fc000p-5, -0x1.97995d05a267dp-46},
        {0x1.e400000000000p-1, 0x1.ccb73cdddc000p-5, -0x1.a68f247d82807p-46},
        {0x1.e200000000000p-1, 0x1.eea31c006c000p-5, -0x1.e113e4fc93b7bp-47},
        {0x1.de00000000000p-1, 0x1.1973bd1466000p-4, -0x1.5325d560d9e9bp-45},
        {0x1.da00000000000p-1, 0x1.3bdf5a7d1e000p-4, 0x1.cc85ea5db4ed7p-45},
        {0x1.d600000000000p-1, 0x1.5e95a4d97a000p-4, -0x1.c69063c5d1d1ep-45},
        {0x1.d400000000000p-1, 0x1.700d30aeac000p-4, 0x1.c1e8da99ded32p-49},
        {0x1.d000000000000p-1, 0x1.9335e5d594000p-4, 0x1.3115c3abd47dap-45},
        {0x1.cc00000000000p-1, 0x1.b6ac88dad6000p-4, -0x1.390802bf768e5p-46},
        {0x1.ca00000000000p-1, 0x1.c885801bc4000p-4, 0x1.646d1c65aacd3p-45},
        {0x1.c600000000000p-1, 0x1.ec739830a2000p-4, -0x1.dc068afe645e0p-45},
        {0x1.c400000000000p-1, 0x1.fe89139dbe000p-4, -0x1.534d64fa10afdp-45},
        {0x1.c000000000000p-1, 0x1.1178e8227e000p-3, 0x1.1ef78ce2d07f2p-45},
        {0x1.be00000000000p-1, 0x1.1aa2b7e23f000p-3, 0x1.ca78e44389934p-45},
        {0x1.ba00000000000p-1, 0x1.2d1610c868000p-3, 0x1.39d6ccb81b4a1p-47},
        {0x1.b800000000000p-1, 0x1.365fcb0159000p-3, 0x1.62fa8234b7289p-51},
        {0x1.b400000000000p-1, 0x1.4913d8333b000p-3, 0x1.5837954fdb678p-45},
        {0x1.b200000000000p-1, 0x1.527e5e4a1b000p-3, 0x1.633e8e5697dc7p-45},
        {0x1.ae00000000000p-1, 0x1.6574ebe8c1000p-3, 0x1.9cf8b2c3c2e78p-46},
        {0x1.ac00000000000p-1, 0x1.6f0128b757000p-3, -0x1.5118de59c21e1p-45},
        {0x1.aa00000000000p-1, 0x1.7898d85445000p-3, -0x1.c661070914305p-46},
        {0x1.a600000000000p-1, 0x1.8beafeb390000p-3, -0x1.73d54aae92cd1p-47},
        {0x1.a400000000000p-1, 0x1.95a5adcf70000p-3, 0x1.7f22858a0ff6fp-47},
        {0x1.a000000000000p-1, 0x1.a93ed3c8ae000p-3, -0x1.8724350562169p-45},
        {0x1.9e00000000000p-1, 0x1.b31d8575bd000p-3, -0x1.c358d4eace1aap-47},
        {0x1.9c00000000000p-1, 0x1.bd087383be000p-3, -0x1.d4bc4595412b6p-45},
        {0x1.9a00000000000p-1, 0x1.c6ffbc6f01000p-3, -0x1.1ec72c5962bd2p-48},
        {0x1.9600000000000p-1, 0x1.db13db0d49000p-3, -0x1.aff2af715b035p-45},
        {0x1.9400000000000p-1, 0x1.e530effe71000p-3, 0x1.212276041f430p-51},
        {0x1.9200000000000p-1, 0x1.ef5ade4dd0000p-3, -0x1.a211565bb8e11p-51},
        {0x1.9000000000000p-1, 0x1.f991c6cb3b000p-3, 0x1.bcbecca0cdf30p-46},
        {0x1.8c00000000000p-1, 0x1.07138604d5800p-2, 0x1.89cdb16ed4e91p-48},
        {0x1.8a00000000000p-1, 0x1.0c42d67616000p-2, 0x1.7188b163ceae9p-45},
        {0x1.8800000000000p-1, 0x1.1178e8227e800p-2, -0x1.c210e63a5f01cp-45},
        {0x1.8600000000000p-1, 0x1.16b5ccbacf800p-2, 0x1.b9acdf7a51681p-45},
        {0x1.8400000000000p-1, 0x1.1bf99635a6800p-2, 0x1.ca6ed5147bdb7p-45},
        {0x1.8200000000000p-1, 0x1.214456d0eb800p-2, 0x1.a87deba46baeap-47},
        {0x1.7e00000000000p-1, 0x1.2bef07cdc9000p-2, 0x1.a9cfa4a5004f4p-45},
        {0x1.7c00000000000p-1, 0x1.314f1e1d36000p-2, -0x1.8e27ad3213cb8p-45},
        {0x1.7a00000000000p-1, 0x1.36b6776be1000p-2, 0x1.16ecdb0f177c8p-46},
        {0x1.7800000000000p-1, 0x1.3c25277333000p-2, 0x1.83b54b606bd5cp-46},
        {0x1.7600000000000p-1, 0x1.419b423d5e800p-2, 0x1.8e436ec90e09dp-47},
        {0x1.7400000000000p-1, 0x1.4718dc271c800p-2, -0x1.f27ce0967d675p-45},
        {0x1.7200000000000p-1, 0x1.4c9e09e173000p-2, -0x1.e20891b0ad8a4p-45},
        {0x1.7000000000000p-1, 0x1.522ae0738a000p-2, 0x1.ebe708164c759p-45},
        {0x1.6e00000000000p-1, 0x1.57bf753c8d000p-2, 0x1.fadedee5d40efp-46},
        {0x1.6c00000000000p-1, 0x1.5d5bddf596000p-2, -0x1.a0b2a08a465dcp-47},
    };
    static const double A[] = {
        -0x1p-1,
        0x1.555555555556p-2 * -2,
        -0x1.0000000000006p-2 * -2,
        0x1.999999959554ep-3 * 4,
        -0x1.555555529a47ap-3 * 4,
        0x1.2495b9b4845e9p-3 * -8,
        -0x1.0002b8b263fc3p-3 * -8
    };
    static const double ln2hi = 0x1.62e42fefa3800p-1,
        ln2lo = 0x1.ef35793c76730p-45;

    double z, r, y, invc, logc, logctail, kd, hi, t1, t2, lo, lo1, lo2, p;
    double zhi, zlo, rhi, rlo, ar, ar2, ar3, lo3, lo4, arhi, arhi2;
    UINT64 iz, tmp;
    int k, i;

    /* x = 2^k z; where z is in range [OFF,2*OFF) and exact.
       The range is split into N subintervals.
       The ith subinterval contains z and c is near its center. */
    tmp = ix - 0x3fe6955500000000ULL;
    i = (tmp >> (52 - 7)) % (1 << 7);
    k = (INT64)tmp >> 52; /* arithmetic shift */
    iz = ix - (tmp & 0xfffULL << 52);
    z = *(double*)&iz;
    kd = k;

    /* log(x) = k*Ln2 + log(c) + log1p(z/c-1). */
    invc = T[i].invc;
    logc = T[i].logc;
    logctail = T[i].logctail;

    /* Note: 1/c is j/N or j/N/2 where j is an integer in [N,2N) and
     |z/c - 1| < 1/N, so r = z/c - 1 is exactly representible. */
    /* Split z such that rhi, rlo and rhi*rhi are exact and |rlo| <= |r|. */
    iz = (iz + (1ULL << 31)) & (-1ULL << 32);
    zhi = *(double*)&iz;
    zlo = z - zhi;
    rhi = zhi * invc - 1.0;
    rlo = zlo * invc;
    r = rhi + rlo;

    /* k*Ln2 + log(c) + r. */
    t1 = kd * ln2hi + logc;
    t2 = t1 + r;
    lo1 = kd * ln2lo + logctail;
    lo2 = t1 - t2 + r;

    /* Evaluation is optimized assuming superscalar pipelined execution. */
    ar = A[0] * r; /* A[0] = -0.5. */
    ar2 = r * ar;
    ar3 = r * ar2;
    /* k*Ln2 + log(c) + r + A[0]*r*r. */
    arhi = A[0] * rhi;
    arhi2 = rhi * arhi;
    hi = t2 + arhi2;
    lo3 = rlo * (ar + arhi);
    lo4 = t2 - hi + arhi2;
    /* p = log1p(r) - r - A[0]*r*r. */
    p = (ar3 * (A[1] + r * A[2] + ar2 * (A[3] + r * A[4] + ar2 * (A[5] + r * A[6]))));
    lo = lo1 + lo2 + lo3 + lo4 + p;
    y = hi + lo;
    *tail = hi - y + lo;
    return y;
}

/* Computes sign*exp(x+xtail) where |xtail| < 2^-8/N and |xtail| <= |x|.
   The sign_bias argument is SIGN_BIAS or 0 and sets the sign to -1 or 1. */
static double pow_exp(double argx, double argy, double x, double xtail, UINT32 sign_bias)
{
    static const double C[] = {
        0x1.ffffffffffdbdp-2,
        0x1.555555555543cp-3,
        0x1.55555cf172b91p-5,
        0x1.1111167a4d017p-7
    };
    static const double invln2N = 0x1.71547652b82fep0 * (1 << 7),
        negln2hiN = -0x1.62e42fefa0000p-8,
        negln2loN = -0x1.cf79abc9e3b3ap-47;

    UINT32 abstop;
    UINT64 ki, idx, top, sbits;
    double kd, z, r, r2, scale, tail, tmp;

    abstop = (*(UINT64*)&x >> 52) & 0x7ff;
    if (abstop - 0x3c9 >= 0x408 - 0x3c9) {
        if (abstop - 0x3c9 >= 0x80000000) {
            /* Avoid spurious underflow for tiny x. */
            /* Note: 0 is common input. */
            double one = 1.0 + x;
            return sign_bias ? -one : one;
        }
        if (abstop >= 0x409) {
            /* Note: inf and nan are already handled. */
            if (*(UINT64*)&x >> 63)
                return math_error(_UNDERFLOW, "pow", argx, argy, (sign_bias ? -DBL_MIN : DBL_MIN) * DBL_MIN);
            return math_error(_OVERFLOW, "pow", argx, argy, (sign_bias ? -DBL_MAX : DBL_MAX) * DBL_MAX);
        }
        /* Large x is special cased below. */
        abstop = 0;
    }

    /* exp(x) = 2^(k/N) * exp(r), with exp(r) in [2^(-1/2N),2^(1/2N)]. */
    /* x = ln2/N*k + r, with int k and r in [-ln2/2N, ln2/2N]. */
    z = invln2N * x;
    kd = round(z);
    ki = (INT64)kd;
    r = x + kd * negln2hiN + kd * negln2loN;
    /* The code assumes 2^-200 < |xtail| < 2^-8/N. */
    r += xtail;
    /* 2^(k/N) ~= scale * (1 + tail). */
    idx = 2 * (ki % (1 << 7));
    top = (ki + sign_bias) << (52 - 7);
    tail = *(double*)&exp_T[idx];
    /* This is only a valid scale when -1023*N < k < 1024*N. */
    sbits = exp_T[idx + 1] + top;
    /* exp(x) = 2^(k/N) * exp(r) ~= scale + scale * (tail + exp(r) - 1). */
    /* Evaluation is optimized assuming superscalar pipelined execution. */
    r2 = r * r;
    /* Without fma the worst case error is 0.25/N ulp larger. */
    /* Worst case error is less than 0.5+1.11/N+(abs poly error * 2^53) ulp. */
    tmp = tail + r + r2 * (C[0] + r * C[1]) + r2 * r2 * (C[2] + r * C[3]);
    if (abstop == 0) {
        /* Handle cases that may overflow or underflow when computing the result that
           is scale*(1+TMP) without intermediate rounding. The bit representation of
           scale is in SBITS, however it has a computed exponent that may have
           overflown into the sign bit so that needs to be adjusted before using it as
           a double. (int32_t)KI is the k used in the argument reduction and exponent
           adjustment of scale, positive k here means the result may overflow and
           negative k means the result may underflow. */
        double scale, y;

        if ((ki & 0x80000000) == 0) {
            /* k > 0, the exponent of scale might have overflowed by <= 460. */
            sbits -= 1009ull << 52;
            scale = *(double*)&sbits;
            y = 0x1p1009 * (scale + scale * tmp);
            if (isinf(y))
                return math_error(_OVERFLOW, "pow", argx, argy, y);
            return y;
        }
        /* k < 0, need special care in the subnormal range. */
        sbits += 1022ull << 52;
        /* Note: sbits is signed scale. */
        scale = *(double*)&sbits;
        y = scale + scale * tmp;
        if (fabs(y) < 1.0) {
            /* Round y to the right precision before scaling it into the subnormal
               range to avoid double rounding that can cause 0.5+E/2 ulp error where
               E is the worst-case ulp error outside the subnormal range. So this
               is only useful if the goal is better than 1 ulp worst-case error. */
            double hi, lo, one = 1.0;
            if (y < 0.0)
                one = -1.0;
            lo = scale - y + scale * tmp;
            hi = one + y;
            lo = one - hi + y + lo;
            y = hi + lo - one;
            /* Fix the sign of 0. */
            if (y == 0.0) {
                sbits &= 0x8000000000000000ULL;
                y = *(double*)&sbits;
            }
            /* The underflow exception needs to be signaled explicitly. */
            fp_barrier(fp_barrier(0x1p-1022) * 0x1p-1022);
            y = 0x1p-1022 * y;
            return math_error(_UNDERFLOW, "pow", argx, argy, y);
        }
        y = 0x1p-1022 * y;
        return y;
    }
    scale = *(double*)&sbits;
    /* Note: tmp == 0 or |tmp| > 2^-200 and scale > 2^-739, so there
       is no spurious underflow here even without fma. */
    return scale + scale * tmp;
}

/* Returns 0 if not int, 1 if odd int, 2 if even int. The argument is
   the bit representation of a non-zero finite floating-point value. */
static inline int pow_checkint(UINT64 iy)
{
    int e = iy >> 52 & 0x7ff;
    if (e < 0x3ff)
        return 0;
    if (e > 0x3ff + 52)
        return 2;
    if (iy & ((1ULL << (0x3ff + 52 - e)) - 1))
        return 0;
    if (iy & (1ULL << (0x3ff + 52 - e)))
        return 1;
    return 2;
}

/*********************************************************************
 *		pow (MSVCRT.@)
 *
 * Copied from musl: src/math/pow.c
 */
double CDECL pow( double x, double y )
{
    UINT32 sign_bias = 0;
    UINT64 ix, iy;
    UINT32 topx, topy;
    double lo, hi, ehi, elo, yhi, ylo, lhi, llo;

    ix = *(UINT64*)&x;
    iy = *(UINT64*)&y;
    topx = ix >> 52;
    topy = iy >> 52;
    if (topx - 0x001 >= 0x7ff - 0x001 ||
            (topy & 0x7ff) - 0x3be >= 0x43e - 0x3be) {
        /* Note: if |y| > 1075 * ln2 * 2^53 ~= 0x1.749p62 then pow(x,y) = inf/0
           and if |y| < 2^-54 / 1075 ~= 0x1.e7b6p-65 then pow(x,y) = +-1. */
        /* Special cases: (x < 0x1p-126 or inf or nan) or
           (|y| < 0x1p-65 or |y| >= 0x1p63 or nan). */
        if (2 * iy - 1 >= 2 * 0x7ff0000000000000ULL - 1) {
            if (2 * iy == 0)
                return 1.0;
            if (ix == 0x3ff0000000000000ULL)
                return 1.0;
            if (2 * ix > 2 * 0x7ff0000000000000ULL ||
                    2 * iy > 2 * 0x7ff0000000000000ULL)
                return x + y;
            if (2 * ix == 2 * 0x3ff0000000000000ULL)
                return 1.0;
            if ((2 * ix < 2 * 0x3ff0000000000000ULL) == !(iy >> 63))
                return 0.0; /* |x|<1 && y==inf or |x|>1 && y==-inf. */
            return y * y;
        }
        if (2 * ix - 1 >= 2 * 0x7ff0000000000000ULL - 1) {
            double x2 = x * x;
            if (ix >> 63 && pow_checkint(iy) == 1)
                x2 = -x2;
            if (iy & 0x8000000000000000ULL && x2 == 0.0)
                return math_error(_SING, "pow", x, y, 1 / x2);
            /* Without the barrier some versions of clang hoist the 1/x2 and
               thus division by zero exception can be signaled spuriously. */
            return iy >> 63 ? fp_barrier(1 / x2) : x2;
        }
        /* Here x and y are non-zero finite. */
        if (ix >> 63) {
            /* Finite x < 0. */
            int yint = pow_checkint(iy);
            if (yint == 0)
                return math_error(_DOMAIN, "pow", x, y, 0 / (x - x));
            if (yint == 1)
                sign_bias = 0x800 << 7;
            ix &= 0x7fffffffffffffff;
            topx &= 0x7ff;
        }
        if ((topy & 0x7ff) - 0x3be >= 0x43e - 0x3be) {
            /* Note: sign_bias == 0 here because y is not odd. */
            if (ix == 0x3ff0000000000000ULL)
                return 1.0;
            if ((topy & 0x7ff) < 0x3be) {
                /* |y| < 2^-65, x^y ~= 1 + y*log(x). */
                return ix > 0x3ff0000000000000ULL ? 1.0 + y : 1.0 - y;
            }
            if ((ix > 0x3ff0000000000000ULL) == (topy < 0x800))
                return math_error(_OVERFLOW, "pow", x, y, fp_barrier(DBL_MAX) * DBL_MAX);
            return math_error(_UNDERFLOW, "pow", x, y, fp_barrier(DBL_MIN) * DBL_MIN);
        }
        if (topx == 0) {
            /* Normalize subnormal x so exponent becomes negative. */
            x *= 0x1p52;
            ix = *(UINT64*)&x;
            ix &= 0x7fffffffffffffff;
            ix -= 52ULL << 52;
        }
    }

    hi = pow_log(ix, &lo);
    iy &= -1ULL << 27;
    yhi = *(double*)&iy;
    ylo = y - yhi;
    *(UINT64*)&lhi = *(UINT64*)&hi & -1ULL << 27;
    llo = fp_barrier(hi - lhi + lo);
    ehi = yhi * lhi;
    elo = ylo * lhi + y * llo; /* |elo| < |ehi| * 2^-25. */
    return pow_exp(x, y, ehi, elo, sign_bias);
}

static BOOL sqrt_validate( double *x, BOOL update_sw )
{
    short c = _dclass(*x);

    if (c == FP_ZERO) return FALSE;
    if (c == FP_NAN)
    {
#ifdef __i386__
        if (update_sw)
            *x = math_error(_DOMAIN, "sqrt", *x, 0, *x);
#else
        /* set signaling bit */
        *(ULONGLONG*)x |= 0x8000000000000ULL;
#endif
        return FALSE;
    }
    if (signbit(*x))
    {
        *x = math_error(_DOMAIN, "sqrt", *x, 0, ret_nan(update_sw));
        return FALSE;
    }
    if (c == FP_INFINITE) return FALSE;
    return TRUE;
}

#if defined(__x86_64__) || defined(__i386__)
double CDECL sse2_sqrt(double);
__ASM_GLOBAL_FUNC( sse2_sqrt,
        "sqrtsd %xmm0, %xmm0\n\t"
        "ret" )
#endif

#ifdef __i386__
double CDECL x87_sqrt(double);
__ASM_GLOBAL_FUNC( x87_sqrt,
        "fldl 4(%esp)\n\t"
        SET_X87_CW(0xc00)
        "fsqrt\n\t"
        RESET_X87_CW
        "ret" )
#endif

/*********************************************************************
 *		sqrt (MSVCRT.@)
 *
 * Copied from musl: src/math/sqrt.c
 */
double CDECL sqrt( double x )
{
#ifdef __x86_64__
    if (!sqrt_validate(&x, TRUE))
        return x;

    return sse2_sqrt(x);
#elif defined( __i386__ )
    if (!sqrt_validate(&x, TRUE))
        return x;

    return x87_sqrt(x);
#else
    static const double tiny = 1.0e-300;

    double z;
    int sign = 0x80000000;
    int ix0,s0,q,m,t,i;
    unsigned int r,t1,s1,ix1,q1;
    ULONGLONG ix;

    if (!sqrt_validate(&x, TRUE))
        return x;

    ix = *(ULONGLONG*)&x;
    ix0 = ix >> 32;
    ix1 = ix;

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
#endif
}

/*********************************************************************
 *		tanh (MSVCRT.@)
 */
double CDECL tanh( double x )
{
    UINT64 ui = *(UINT64*)&x;
    UINT64 sign = ui & 0x8000000000000000ULL;
    UINT32 w;
    double t;

    /* x = |x| */
    ui &= (UINT64)-1 / 2;
    x = *(double*)&ui;
    w = ui >> 32;

    if (w > 0x3fe193ea) {
        /* |x| > log(3)/2 ~= 0.5493 or nan */
        if (w > 0x40340000) {
            if (ui > 0x7ff0000000000000ULL) {
                *(UINT64*)&x = ui | sign | 0x0008000000000000ULL;
#if _MSVCR_VER < 140
                return math_error(_DOMAIN, "tanh", x, 0, x);
#else
                return x;
#endif
            }
            /* |x| > 20 */
            /* note: this branch avoids raising overflow */
            fp_barrier(x + 0x1p120f);
            t = 1 - 0 / x;
        } else {
            t = expm1(2 * x);
            t = 1 - 2 / (t + 2);
        }
    } else if (w > 0x3fd058ae) {
        /* |x| > log(5/3)/2 ~= 0.2554 */
        t = expm1(2 * x);
        t = t / (t + 2);
    } else if (w >= 0x00100000) {
        /* |x| >= 0x1p-1022, up to 2ulp error in [0.1,0.2554] */
        t = expm1(-2 * x);
        t = -t / (t + 2);
    } else {
        /* |x| is subnormal */
        /* note: the branch above would not raise underflow in [0x1p-1023,0x1p-1022) */
        fp_barrier((float)x);
        t = x;
    }
    return sign ? -t : t;
}


#if (defined(__GNUC__) || defined(__clang__)) && defined(__i386__)

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

CREATE_FPU_FUNC1(_CIacos, acos)
CREATE_FPU_FUNC1(_CIasin, asin)
CREATE_FPU_FUNC1(_CIatan, atan)
CREATE_FPU_FUNC2(_CIatan2, atan2)
CREATE_FPU_FUNC1(_CIcos, cos)
CREATE_FPU_FUNC1(_CIcosh, cosh)
CREATE_FPU_FUNC1(_CIexp, exp)
CREATE_FPU_FUNC2(_CIfmod, fmod)
CREATE_FPU_FUNC1(_CIlog, log)
CREATE_FPU_FUNC1(_CIlog10, log10)
CREATE_FPU_FUNC2(_CIpow, pow)
CREATE_FPU_FUNC1(_CIsin, sin)
CREATE_FPU_FUNC1(_CIsinh, sinh)
CREATE_FPU_FUNC1(_CIsqrt, sqrt)
CREATE_FPU_FUNC1(_CItan, tan)
CREATE_FPU_FUNC1(_CItanh, tanh)

__ASM_GLOBAL_FUNC(_ftol,
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

#endif /* (defined(__GNUC__) || defined(__clang__)) && defined(__i386__) */

/*********************************************************************
 *		_fpclass (MSVCRT.@)
 */
int CDECL _fpclass(double num)
{
    union { double f; UINT64 i; } u = { num };
    int e = u.i >> 52 & 0x7ff;
    int s = u.i >> 63;

    switch (e)
    {
    case 0:
        if (u.i << 1) return s ? _FPCLASS_ND : _FPCLASS_PD;
        return s ? _FPCLASS_NZ : _FPCLASS_PZ;
    case 0x7ff:
        if (u.i << 12) return ((u.i >> 51) & 1) ? _FPCLASS_QNAN : _FPCLASS_SNAN;
        return s ? _FPCLASS_NINF : _FPCLASS_PINF;
    default:
        return s ? _FPCLASS_NN : _FPCLASS_PN;
    }
}

/*********************************************************************
 *		_rotl (MSVCRT.@)
 */
unsigned int CDECL MSVCRT__rotl(unsigned int num, int shift)
{
  shift &= 31;
  return (num << shift) | (num >> (32-shift));
}

/*********************************************************************
 *		_lrotl (MSVCRT.@)
 */
__msvcrt_ulong CDECL MSVCRT__lrotl(__msvcrt_ulong num, int shift)
{
  shift &= 0x1f;
  return (num << shift) | (num >> (32-shift));
}

/*********************************************************************
 *		_lrotr (MSVCRT.@)
 */
__msvcrt_ulong CDECL MSVCRT__lrotr(__msvcrt_ulong num, int shift)
{
  shift &= 0x1f;
  return (num >> shift) | (num << (32-shift));
}

/*********************************************************************
 *		_rotr (MSVCRT.@)
 */
unsigned int CDECL MSVCRT__rotr(unsigned int num, int shift)
{
    shift &= 0x1f;
    return (num >> shift) | (num << (32-shift));
}

/*********************************************************************
 *		_rotl64 (MSVCRT.@)
 */
unsigned __int64 CDECL MSVCRT__rotl64(unsigned __int64 num, int shift)
{
  shift &= 63;
  return (num << shift) | (num >> (64-shift));
}

/*********************************************************************
 *		_rotr64 (MSVCRT.@)
 */
unsigned __int64 CDECL MSVCRT__rotr64(unsigned __int64 num, int shift)
{
    shift &= 63;
    return (num >> shift) | (num << (64-shift));
}

/*********************************************************************
 *		abs (MSVCRT.@)
 */
int CDECL abs( int n )
{
    return n >= 0 ? n : -n;
}

/*********************************************************************
 *		labs (MSVCRT.@)
 */
__msvcrt_long CDECL labs( __msvcrt_long n )
{
    return n >= 0 ? n : -n;
}

#if _MSVCR_VER>=100
/*********************************************************************
 *		llabs (MSVCR100.@)
 */
__int64 CDECL llabs( __int64 n )
{
    return n >= 0 ? n : -n;
}
#endif

#if _MSVCR_VER>=120
/*********************************************************************
 *		imaxabs (MSVCR120.@)
 */
intmax_t CDECL imaxabs( intmax_t n )
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

#if defined(__i386__) || defined(__x86_64__)
static void _setfp_sse( unsigned int *cw, unsigned int cw_mask,
        unsigned int *sw, unsigned int sw_mask )
{
#if defined(__GNUC__) || defined(__clang__)
    unsigned long old_fpword, fpword;
    unsigned int flags;

    __asm__ __volatile__( "stmxcsr %0" : "=m" (fpword) );
    old_fpword = fpword;

    cw_mask &= _MCW_EM | _MCW_RC | _MCW_DN;
    sw_mask &= _MCW_EM;

    if (sw)
    {
        flags = 0;
        if (fpword & 0x1) flags |= _SW_INVALID;
        if (fpword & 0x2) flags |= _SW_DENORMAL;
        if (fpword & 0x4) flags |= _SW_ZERODIVIDE;
        if (fpword & 0x8) flags |= _SW_OVERFLOW;
        if (fpword & 0x10) flags |= _SW_UNDERFLOW;
        if (fpword & 0x20) flags |= _SW_INEXACT;

        *sw = (flags & ~sw_mask) | (*sw & sw_mask);
        TRACE("sse2 update sw %08x to %08x\n", flags, *sw);
        fpword &= ~0x3f;
        if (*sw & _SW_INVALID) fpword |= 0x1;
        if (*sw & _SW_DENORMAL) fpword |= 0x2;
        if (*sw & _SW_ZERODIVIDE) fpword |= 0x4;
        if (*sw & _SW_OVERFLOW) fpword |= 0x8;
        if (*sw & _SW_UNDERFLOW) fpword |= 0x10;
        if (*sw & _SW_INEXACT) fpword |= 0x20;
        *sw = flags;
    }

    if (cw)
    {
        flags = 0;
        if (fpword & 0x80) flags |= _EM_INVALID;
        if (fpword & 0x100) flags |= _EM_DENORMAL;
        if (fpword & 0x200) flags |= _EM_ZERODIVIDE;
        if (fpword & 0x400) flags |= _EM_OVERFLOW;
        if (fpword & 0x800) flags |= _EM_UNDERFLOW;
        if (fpword & 0x1000) flags |= _EM_INEXACT;
        switch (fpword & 0x6000)
        {
        case 0x6000: flags |= _RC_UP|_RC_DOWN; break;
        case 0x4000: flags |= _RC_UP; break;
        case 0x2000: flags |= _RC_DOWN; break;
        }
        switch (fpword & 0x8040)
        {
        case 0x0040: flags |= _DN_FLUSH_OPERANDS_SAVE_RESULTS; break;
        case 0x8000: flags |= _DN_SAVE_OPERANDS_FLUSH_RESULTS; break;
        case 0x8040: flags |= _DN_FLUSH; break;
        }

        *cw = (flags & ~cw_mask) | (*cw & cw_mask);
        TRACE("sse2 update cw %08x to %08x\n", flags, *cw);
        fpword &= ~0xffc0;
        if (*cw & _EM_INVALID) fpword |= 0x80;
        if (*cw & _EM_DENORMAL) fpword |= 0x100;
        if (*cw & _EM_ZERODIVIDE) fpword |= 0x200;
        if (*cw & _EM_OVERFLOW) fpword |= 0x400;
        if (*cw & _EM_UNDERFLOW) fpword |= 0x800;
        if (*cw & _EM_INEXACT) fpword |= 0x1000;
        switch (*cw & _MCW_RC)
        {
        case _RC_UP|_RC_DOWN: fpword |= 0x6000; break;
        case _RC_UP: fpword |= 0x4000; break;
        case _RC_DOWN: fpword |= 0x2000; break;
        }
        switch (*cw & _MCW_DN)
        {
        case _DN_FLUSH_OPERANDS_SAVE_RESULTS: fpword |= 0x0040; break;
        case _DN_SAVE_OPERANDS_FLUSH_RESULTS: fpword |= 0x8000; break;
        case _DN_FLUSH: fpword |= 0x8040; break;
        }

        /* clear status word if anything changes */
        if (fpword != old_fpword && !sw)
        {
            TRACE("sse2 clear status word\n");
            fpword &= ~0x3f;
        }
    }

    if (fpword != old_fpword)
        __asm__ __volatile__( "ldmxcsr %0" : : "m" (fpword) );
#else
    FIXME("not implemented\n");
    if (cw) *cw = 0;
    if (sw) *sw = 0;
#endif
}
#endif

static void _setfp( unsigned int *cw, unsigned int cw_mask,
        unsigned int *sw, unsigned int sw_mask )
{
#if (defined(__GNUC__) || defined(__clang__)) && defined(__i386__)
    unsigned long oldcw = 0, newcw = 0;
    unsigned long oldsw = 0, newsw = 0;
    unsigned int flags;

    cw_mask &= _MCW_EM | _MCW_IC | _MCW_RC | _MCW_PC;
    sw_mask &= _MCW_EM;

    if (sw)
    {
        __asm__ __volatile__( "fstsw %0" : "=m" (newsw) );
        oldsw = newsw;

        flags = 0;
        if (newsw & 0x1) flags |= _SW_INVALID;
        if (newsw & 0x2) flags |= _SW_DENORMAL;
        if (newsw & 0x4) flags |= _SW_ZERODIVIDE;
        if (newsw & 0x8) flags |= _SW_OVERFLOW;
        if (newsw & 0x10) flags |= _SW_UNDERFLOW;
        if (newsw & 0x20) flags |= _SW_INEXACT;

        *sw = (flags & ~sw_mask) | (*sw & sw_mask);
        TRACE("x86 update sw %08x to %08x\n", flags, *sw);
        newsw &= ~0x3f;
        if (*sw & _SW_INVALID) newsw |= 0x1;
        if (*sw & _SW_DENORMAL) newsw |= 0x2;
        if (*sw & _SW_ZERODIVIDE) newsw |= 0x4;
        if (*sw & _SW_OVERFLOW) newsw |= 0x8;
        if (*sw & _SW_UNDERFLOW) newsw |= 0x10;
        if (*sw & _SW_INEXACT) newsw |= 0x20;
        *sw = flags;
    }

    if (cw)
    {
        __asm__ __volatile__( "fstcw %0" : "=m" (newcw) );
        oldcw = newcw;

        flags = 0;
        if (newcw & 0x1) flags |= _EM_INVALID;
        if (newcw & 0x2) flags |= _EM_DENORMAL;
        if (newcw & 0x4) flags |= _EM_ZERODIVIDE;
        if (newcw & 0x8) flags |= _EM_OVERFLOW;
        if (newcw & 0x10) flags |= _EM_UNDERFLOW;
        if (newcw & 0x20) flags |= _EM_INEXACT;
        switch (newcw & 0xc00)
        {
        case 0xc00: flags |= _RC_UP|_RC_DOWN; break;
        case 0x800: flags |= _RC_UP; break;
        case 0x400: flags |= _RC_DOWN; break;
        }
        switch (newcw & 0x300)
        {
        case 0x0: flags |= _PC_24; break;
        case 0x200: flags |= _PC_53; break;
        case 0x300: flags |= _PC_64; break;
        }
        if (newcw & 0x1000) flags |= _IC_AFFINE;

        *cw = (flags & ~cw_mask) | (*cw & cw_mask);
        TRACE("x86 update cw %08x to %08x\n", flags, *cw);
        newcw &= ~0x1f3f;
        if (*cw & _EM_INVALID) newcw |= 0x1;
        if (*cw & _EM_DENORMAL) newcw |= 0x2;
        if (*cw & _EM_ZERODIVIDE) newcw |= 0x4;
        if (*cw & _EM_OVERFLOW) newcw |= 0x8;
        if (*cw & _EM_UNDERFLOW) newcw |= 0x10;
        if (*cw & _EM_INEXACT) newcw |= 0x20;
        switch (*cw & _MCW_RC)
        {
        case _RC_UP|_RC_DOWN: newcw |= 0xc00; break;
        case _RC_UP: newcw |= 0x800; break;
        case _RC_DOWN: newcw |= 0x400; break;
        }
        switch (*cw & _MCW_PC)
        {
        case _PC_64: newcw |= 0x300; break;
        case _PC_53: newcw |= 0x200; break;
        case _PC_24: newcw |= 0x0; break;
        }
        if (*cw & _IC_AFFINE) newcw |= 0x1000;
    }

    if (oldsw != newsw && (newsw & 0x3f))
    {
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

        assert(cw);

        __asm__ __volatile__( "fnstenv %0" : "=m" (fenv) );
        fenv.control_word = newcw;
        fenv.status_word = newsw;
        __asm__ __volatile__( "fldenv %0" : : "m" (fenv) : "st", "st(1)",
                "st(2)", "st(3)", "st(4)", "st(5)", "st(6)", "st(7)" );
        return;
    }

    if (oldsw != newsw)
        __asm__ __volatile__( "fnclex" );
    if (oldcw != newcw)
        __asm__ __volatile__( "fldcw %0" : : "m" (newcw) );
#elif defined(__x86_64__)
    _setfp_sse(cw, cw_mask, sw, sw_mask);
#elif defined(__aarch64__)
    ULONG_PTR old_fpsr = 0, fpsr = 0, old_fpcr = 0, fpcr = 0;
    unsigned int flags;

    cw_mask &= _MCW_EM | _MCW_RC;
    sw_mask &= _MCW_EM;

    if (sw)
    {
        __asm__ __volatile__( "mrs %0, fpsr" : "=r" (fpsr) );
        old_fpsr = fpsr;

        flags = 0;
        if (fpsr & 0x1) flags |= _SW_INVALID;
        if (fpsr & 0x2) flags |= _SW_ZERODIVIDE;
        if (fpsr & 0x4) flags |= _SW_OVERFLOW;
        if (fpsr & 0x8) flags |= _SW_UNDERFLOW;
        if (fpsr & 0x10) flags |= _SW_INEXACT;
        if (fpsr & 0x80) flags |= _SW_DENORMAL;

        *sw = (flags & ~sw_mask) | (*sw & sw_mask);
        TRACE("aarch64 update sw %08x to %08x\n", flags, *sw);
        fpsr &= ~0x9f;
        if (*sw & _SW_INVALID) fpsr |= 0x1;
        if (*sw & _SW_ZERODIVIDE) fpsr |= 0x2;
        if (*sw & _SW_OVERFLOW) fpsr |= 0x4;
        if (*sw & _SW_UNDERFLOW) fpsr |= 0x8;
        if (*sw & _SW_INEXACT) fpsr |= 0x10;
        if (*sw & _SW_DENORMAL) fpsr |= 0x80;
        *sw = flags;
    }

    if (cw)
    {
        __asm__ __volatile__( "mrs %0, fpcr" : "=r" (fpcr) );
        old_fpcr = fpcr;

        flags = 0;
        if (!(fpcr & 0x100)) flags |= _EM_INVALID;
        if (!(fpcr & 0x200)) flags |= _EM_ZERODIVIDE;
        if (!(fpcr & 0x400)) flags |= _EM_OVERFLOW;
        if (!(fpcr & 0x800)) flags |= _EM_UNDERFLOW;
        if (!(fpcr & 0x1000)) flags |= _EM_INEXACT;
        if (!(fpcr & 0x8000)) flags |= _EM_DENORMAL;
        switch (fpcr & 0xc00000)
        {
        case 0x400000: flags |= _RC_UP; break;
        case 0x800000: flags |= _RC_DOWN; break;
        case 0xc00000: flags |= _RC_CHOP; break;
        }

        *cw = (flags & ~cw_mask) | (*cw & cw_mask);
        TRACE("aarch64 update cw %08x to %08x\n", flags, *cw);
        fpcr &= ~0xc09f00ul;
        if (!(*cw & _EM_INVALID)) fpcr |= 0x100;
        if (!(*cw & _EM_ZERODIVIDE)) fpcr |= 0x200;
        if (!(*cw & _EM_OVERFLOW)) fpcr |= 0x400;
        if (!(*cw & _EM_UNDERFLOW)) fpcr |= 0x800;
        if (!(*cw & _EM_INEXACT)) fpcr |= 0x1000;
        if (!(*cw & _EM_DENORMAL)) fpcr |= 0x8000;
        switch (*cw & _MCW_RC)
        {
        case _RC_CHOP: fpcr |= 0xc00000; break;
        case _RC_UP: fpcr |= 0x400000; break;
        case _RC_DOWN: fpcr |= 0x800000; break;
        }
    }

    /* mask exceptions if needed */
    if (old_fpcr != fpcr && ~(old_fpcr >> 8) & fpsr & 0x9f != fpsr & 0x9f)
    {
        ULONG_PTR mask = fpcr & ~0x9f00;
        __asm__ __volatile__( "msr fpcr, %0" :: "r" (mask) );
    }

    if (old_fpsr != fpsr)
        __asm__ __volatile__( "msr fpsr, %0" :: "r" (fpsr) );
    if (old_fpcr != fpcr)
        __asm__ __volatile__( "msr fpcr, %0" :: "r" (fpcr) );
#elif defined(__arm__) && !defined(__SOFTFP__)
    DWORD old_fpscr, fpscr;
    unsigned int flags;

    __asm__ __volatile__( "vmrs %0, fpscr" : "=r" (fpscr) );
    old_fpscr = fpscr;

    cw_mask &= _MCW_EM | _MCW_RC;
    sw_mask &= _MCW_EM;

    if (sw)
    {
        flags = 0;
        if (fpscr & 0x1) flags |= _SW_INVALID;
        if (fpscr & 0x2) flags |= _SW_ZERODIVIDE;
        if (fpscr & 0x4) flags |= _SW_OVERFLOW;
        if (fpscr & 0x8) flags |= _SW_UNDERFLOW;
        if (fpscr & 0x10) flags |= _SW_INEXACT;
        if (fpscr & 0x80) flags |= _SW_DENORMAL;

        *sw = (flags & ~sw_mask) | (*sw & sw_mask);
        TRACE("arm update sw %08x to %08x\n", flags, *sw);
        fpscr &= ~0x9f;
        if (*sw & _SW_INVALID) fpscr |= 0x1;
        if (*sw & _SW_ZERODIVIDE) fpscr |= 0x2;
        if (*sw & _SW_OVERFLOW) fpscr |= 0x4;
        if (*sw & _SW_UNDERFLOW) fpscr |= 0x8;
        if (*sw & _SW_INEXACT) fpscr |= 0x10;
        if (*sw & _SW_DENORMAL) fpscr |= 0x80;
        *sw = flags;
    }

    if (cw)
    {
        flags = 0;
        if (!(fpscr & 0x100)) flags |= _EM_INVALID;
        if (!(fpscr & 0x200)) flags |= _EM_ZERODIVIDE;
        if (!(fpscr & 0x400)) flags |= _EM_OVERFLOW;
        if (!(fpscr & 0x800)) flags |= _EM_UNDERFLOW;
        if (!(fpscr & 0x1000)) flags |= _EM_INEXACT;
        if (!(fpscr & 0x8000)) flags |= _EM_DENORMAL;
        switch (fpscr & 0xc00000)
        {
        case 0x400000: flags |= _RC_UP; break;
        case 0x800000: flags |= _RC_DOWN; break;
        case 0xc00000: flags |= _RC_CHOP; break;
        }

        *cw = (flags & ~cw_mask) | (*cw & cw_mask);
        TRACE("arm update cw %08x to %08x\n", flags, *cw);
        fpscr &= ~0xc09f00ul;
        if (!(*cw & _EM_INVALID)) fpscr |= 0x100;
        if (!(*cw & _EM_ZERODIVIDE)) fpscr |= 0x200;
        if (!(*cw & _EM_OVERFLOW)) fpscr |= 0x400;
        if (!(*cw & _EM_UNDERFLOW)) fpscr |= 0x800;
        if (!(*cw & _EM_INEXACT)) fpscr |= 0x1000;
        if (!(*cw & _EM_DENORMAL)) fpscr |= 0x8000;
        switch (*cw & _MCW_RC)
        {
        case _RC_CHOP: fpscr |= 0xc00000; break;
        case _RC_UP: fpscr |= 0x400000; break;
        case _RC_DOWN: fpscr |= 0x800000; break;
        }
    }

    if (old_fpscr != fpscr)
        __asm__ __volatile__( "vmsr fpscr, %0" :: "r" (fpscr) );
#else
    FIXME("not implemented\n");
    if (cw) *cw = 0;
    if (sw) *sw = 0;
#endif
}

/**********************************************************************
 *		_statusfp2 (MSVCR80.@)
 */
#if defined(__i386__)
void CDECL _statusfp2( unsigned int *x86_sw, unsigned int *sse2_sw )
{
    if (x86_sw)
        _setfp(NULL, 0, x86_sw, 0);
    if (!sse2_sw) return;
    if (sse2_supported)
        _setfp_sse(NULL, 0, sse2_sw, 0);
    else *sse2_sw = 0;
}
#endif

/**********************************************************************
 *		_statusfp (MSVCRT.@)
 */
unsigned int CDECL _statusfp(void)
{
    unsigned int flags = 0;
#if defined(__i386__)
    unsigned int x86_sw, sse2_sw;

    _statusfp2( &x86_sw, &sse2_sw );
    /* FIXME: there's no definition for ambiguous status, just return all status bits for now */
    flags = x86_sw | sse2_sw;
#else
    _setfp(NULL, 0, &flags, 0);
#endif
    return flags;
}

/*********************************************************************
 *		_clearfp (MSVCRT.@)
 */
unsigned int CDECL _clearfp(void)
{
    unsigned int flags = 0;
#ifdef __i386__
    _setfp(NULL, 0, &flags, _MCW_EM);
    if (sse2_supported)
    {
        unsigned int sse_sw = 0;

        _setfp_sse(NULL, 0, &sse_sw, _MCW_EM);
        flags |= sse_sw;
    }
#else
    _setfp(NULL, 0, &flags, _MCW_EM);
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
double CDECL ldexp(double num, int exp)
{
  double z = scalbn(num, exp);

  if (isfinite(num) && !isfinite(z))
    return math_error(_OVERFLOW, "ldexp", num, exp, z);
  if (num && isfinite(num) && !z)
    return math_error(_UNDERFLOW, "ldexp", num, exp, z);
  return z;
}

/*********************************************************************
 *		_cabs (MSVCRT.@)
 */
double CDECL _cabs(struct _complex num)
{
  return sqrt(num.x * num.x + num.y * num.y);
}

/*********************************************************************
 *		_chgsign (MSVCRT.@)
 */
double CDECL _chgsign(double num)
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
    if (x86_cw)
    {
        *x86_cw = newval;
        _setfp(x86_cw, mask, NULL, 0);
    }

    if (!sse2_cw) return 1;

    if (sse2_supported)
    {
        *sse2_cw = newval;
        _setfp_sse(sse2_cw, mask, NULL, 0);
    }
    else *sse2_cw = 0;

    return 1;
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

    if (sse2_supported)
    {
        if ((flags ^ sse2_cw) & (_MCW_EM | _MCW_RC)) flags |= _EM_AMBIGUOUS;
        flags |= sse2_cw;
    }
#else
    flags = newval;
    _setfp(&flags, mask, NULL, 0);
#endif
    return flags;
}

/*********************************************************************
 *		_controlfp (MSVCRT.@)
 */
unsigned int CDECL _controlfp(unsigned int newval, unsigned int mask)
{
  return _control87( newval, mask & ~_EM_DENORMAL );
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
    static const unsigned int all_flags = (_MCW_EM | _MCW_IC | _MCW_RC |
                                           _MCW_PC | _MCW_DN);
    unsigned int val;

    if (!MSVCRT_CHECK_PMT( !(newval & mask & ~all_flags) ))
    {
        if (cur) *cur = _controlfp( 0, 0 );  /* retrieve it anyway */
        return EINVAL;
    }
    val = _controlfp( newval, mask );
    if (cur) *cur = val;
    return 0;
}

#if _MSVCR_VER >= 140 && (defined(__i386__) || defined(__x86_64__))
enum fenv_masks
{
    FENV_X_INVALID = 0x00100010,
    FENV_X_DENORMAL = 0x00200020,
    FENV_X_ZERODIVIDE = 0x00080008,
    FENV_X_OVERFLOW = 0x00040004,
    FENV_X_UNDERFLOW = 0x00020002,
    FENV_X_INEXACT = 0x00010001,
    FENV_X_AFFINE = 0x00004000,
    FENV_X_UP = 0x00800200,
    FENV_X_DOWN = 0x00400100,
    FENV_X_24 = 0x00002000,
    FENV_X_53 = 0x00001000,
    FENV_Y_INVALID = 0x10000010,
    FENV_Y_DENORMAL = 0x20000020,
    FENV_Y_ZERODIVIDE = 0x08000008,
    FENV_Y_OVERFLOW = 0x04000004,
    FENV_Y_UNDERFLOW = 0x02000002,
    FENV_Y_INEXACT = 0x01000001,
    FENV_Y_UP = 0x80000200,
    FENV_Y_DOWN = 0x40000100,
    FENV_Y_FLUSH = 0x00000400,
    FENV_Y_FLUSH_SAVE = 0x00000800
};

/* encodes x87/sse control/status word in ulong */
static __msvcrt_ulong fenv_encode(unsigned int x, unsigned int y)
{
    __msvcrt_ulong ret = 0;

#ifdef __i386__
    if (x & _EM_INVALID) ret |= FENV_X_INVALID;
    if (x & _EM_DENORMAL) ret |= FENV_X_DENORMAL;
    if (x & _EM_ZERODIVIDE) ret |= FENV_X_ZERODIVIDE;
    if (x & _EM_OVERFLOW) ret |= FENV_X_OVERFLOW;
    if (x & _EM_UNDERFLOW) ret |= FENV_X_UNDERFLOW;
    if (x & _EM_INEXACT) ret |= FENV_X_INEXACT;
    if (x & _IC_AFFINE) ret |= FENV_X_AFFINE;
    if (x & _RC_UP) ret |= FENV_X_UP;
    if (x & _RC_DOWN) ret |= FENV_X_DOWN;
    if (x & _PC_24) ret |= FENV_X_24;
    if (x & _PC_53) ret |= FENV_X_53;
#endif
    x &= ~(_MCW_EM | _MCW_IC | _MCW_RC | _MCW_PC);

    if (y & _EM_INVALID) ret |= FENV_Y_INVALID;
    if (y & _EM_DENORMAL) ret |= FENV_Y_DENORMAL;
    if (y & _EM_ZERODIVIDE) ret |= FENV_Y_ZERODIVIDE;
    if (y & _EM_OVERFLOW) ret |= FENV_Y_OVERFLOW;
    if (y & _EM_UNDERFLOW) ret |= FENV_Y_UNDERFLOW;
    if (y & _EM_INEXACT) ret |= FENV_Y_INEXACT;
    if (y & _RC_UP) ret |= FENV_Y_UP;
    if (y & _RC_DOWN) ret |= FENV_Y_DOWN;
    if (y & _DN_FLUSH) ret |= FENV_Y_FLUSH;
    if (y & _DN_FLUSH_OPERANDS_SAVE_RESULTS) ret |= FENV_Y_FLUSH_SAVE;
    y &= ~(_MCW_EM | _MCW_IC | _MCW_RC | _MCW_DN);

    if(x || y) FIXME("unsupported flags: %x, %x\n", x, y);
    return ret;
}

/* decodes x87/sse control/status word, returns FALSE on error */
static BOOL fenv_decode(__msvcrt_ulong enc, unsigned int *x, unsigned int *y)
{
    *x = *y = 0;
    if ((enc & FENV_X_INVALID) == FENV_X_INVALID) *x |= _EM_INVALID;
    if ((enc & FENV_X_DENORMAL) == FENV_X_DENORMAL) *x |= _EM_DENORMAL;
    if ((enc & FENV_X_ZERODIVIDE) == FENV_X_ZERODIVIDE) *x |= _EM_ZERODIVIDE;
    if ((enc & FENV_X_OVERFLOW) == FENV_X_OVERFLOW) *x |= _EM_OVERFLOW;
    if ((enc & FENV_X_UNDERFLOW) == FENV_X_UNDERFLOW) *x |= _EM_UNDERFLOW;
    if ((enc & FENV_X_INEXACT) == FENV_X_INEXACT) *x |= _EM_INEXACT;
    if ((enc & FENV_X_AFFINE) == FENV_X_AFFINE) *x |= _IC_AFFINE;
    if ((enc & FENV_X_UP) == FENV_X_UP) *x |= _RC_UP;
    if ((enc & FENV_X_DOWN) == FENV_X_DOWN) *x |= _RC_DOWN;
    if ((enc & FENV_X_24) == FENV_X_24) *x |= _PC_24;
    if ((enc & FENV_X_53) == FENV_X_53) *x |= _PC_53;

    if ((enc & FENV_Y_INVALID) == FENV_Y_INVALID) *y |= _EM_INVALID;
    if ((enc & FENV_Y_DENORMAL) == FENV_Y_DENORMAL) *y |= _EM_DENORMAL;
    if ((enc & FENV_Y_ZERODIVIDE) == FENV_Y_ZERODIVIDE) *y |= _EM_ZERODIVIDE;
    if ((enc & FENV_Y_OVERFLOW) == FENV_Y_OVERFLOW) *y |= _EM_OVERFLOW;
    if ((enc & FENV_Y_UNDERFLOW) == FENV_Y_UNDERFLOW) *y |= _EM_UNDERFLOW;
    if ((enc & FENV_Y_INEXACT) == FENV_Y_INEXACT) *y |= _EM_INEXACT;
    if ((enc & FENV_Y_UP) == FENV_Y_UP) *y |= _RC_UP;
    if ((enc & FENV_Y_DOWN) == FENV_Y_DOWN) *y |= _RC_DOWN;
    if ((enc & FENV_Y_FLUSH) == FENV_Y_FLUSH) *y |= _DN_FLUSH;
    if ((enc & FENV_Y_FLUSH_SAVE) == FENV_Y_FLUSH_SAVE) *y |= _DN_FLUSH_OPERANDS_SAVE_RESULTS;

    if (fenv_encode(*x, *y) != enc)
    {
        WARN("can't decode: %lx\n", enc);
        return FALSE;
    }
    return TRUE;
}
#elif _MSVCR_VER >= 120
static __msvcrt_ulong fenv_encode(unsigned int x, unsigned int y)
{
    if (y & _EM_DENORMAL)
        y = (y & ~_EM_DENORMAL) | 0x20;

    return x | y;
}

static BOOL fenv_decode(__msvcrt_ulong enc, unsigned int *x, unsigned int *y)
{
    if (enc & 0x20)
        enc = (enc & ~0x20) | _EM_DENORMAL;

    *x = *y = enc;
    return TRUE;
}
#endif

#if _MSVCR_VER>=120
/*********************************************************************
 *		fegetenv (MSVCR120.@)
 */
int CDECL fegetenv(fenv_t *env)
{
#if _MSVCR_VER>=140 && defined(__i386__)
    unsigned int x87, sse;
    __control87_2(0, 0, &x87, &sse);
    env->_Fe_ctl = fenv_encode(x87, sse);
    _statusfp2(&x87, &sse);
    env->_Fe_stat = fenv_encode(x87, sse);
#elif _MSVCR_VER>=140
    env->_Fe_ctl = fenv_encode(0, _control87(0, 0));
    env->_Fe_stat = fenv_encode(0, _statusfp());
#else
    env->_Fe_ctl = _controlfp(0, 0) & (_EM_INEXACT | _EM_UNDERFLOW |
            _EM_OVERFLOW | _EM_ZERODIVIDE | _EM_INVALID | _MCW_RC);
    env->_Fe_stat = _statusfp();
#endif
    return 0;
}

/*********************************************************************
 *		feupdateenv (MSVCR120.@)
 */
int CDECL feupdateenv(const fenv_t *env)
{
    fenv_t set;
    fegetenv(&set);
    set._Fe_ctl = env->_Fe_ctl;
    set._Fe_stat |= env->_Fe_stat;
    return fesetenv(&set);
}

/*********************************************************************
 *      fetestexcept (MSVCR120.@)
 */
int CDECL fetestexcept(int flags)
{
    return _statusfp() & flags;
}

/*********************************************************************
 *      fesetexceptflag (MSVCR120.@)
 */
int CDECL fesetexceptflag(const fexcept_t *status, int excepts)
{
    fenv_t env;

    excepts &= FE_ALL_EXCEPT;
    if(!excepts)
        return 0;

    fegetenv(&env);
    env._Fe_stat &= ~fenv_encode(excepts, excepts);
    env._Fe_stat |= *status & fenv_encode(excepts, excepts);
    return fesetenv(&env);
}

/*********************************************************************
 *      feraiseexcept (MSVCR120.@)
 */
int CDECL feraiseexcept(int flags)
{
    fenv_t env;

    flags &= FE_ALL_EXCEPT;
    fegetenv(&env);
    env._Fe_stat |= fenv_encode(flags, flags);
    return fesetenv(&env);
}

/*********************************************************************
 *      feclearexcept (MSVCR120.@)
 */
int CDECL feclearexcept(int flags)
{
    fenv_t env;

    fegetenv(&env);
    flags &= FE_ALL_EXCEPT;
    env._Fe_stat &= ~fenv_encode(flags, flags);
    return fesetenv(&env);
}

/*********************************************************************
 *      fegetexceptflag (MSVCR120.@)
 */
int CDECL fegetexceptflag(fexcept_t *status, int excepts)
{
#if _MSVCR_VER>=140 && defined(__i386__)
    unsigned int x87, sse;
    _statusfp2(&x87, &sse);
    *status = fenv_encode(x87 & excepts, sse & excepts);
#else
    *status = fenv_encode(0, _statusfp() & excepts);
#endif
    return 0;
}
#endif

#if _MSVCR_VER>=140
/*********************************************************************
 *		__fpe_flt_rounds (UCRTBASE.@)
 */
int CDECL __fpe_flt_rounds(void)
{
    unsigned int fpc = _controlfp(0, 0) & _RC_CHOP;

    TRACE("()\n");

    switch(fpc) {
        case _RC_CHOP: return 0;
        case _RC_NEAR: return 1;
        case _RC_UP: return 2;
        default: return 3;
    }
}
#endif

#if _MSVCR_VER>=120

/*********************************************************************
 *		fegetround (MSVCR120.@)
 */
int CDECL fegetround(void)
{
    return _controlfp(0, 0) & _MCW_RC;
}

/*********************************************************************
 *		fesetround (MSVCR120.@)
 */
int CDECL fesetround(int round_mode)
{
    if (round_mode & (~_MCW_RC))
        return 1;
    _controlfp(round_mode, _MCW_RC);
    return 0;
}

#endif /* _MSVCR_VER>=120 */

/*********************************************************************
 *		_finite (MSVCRT.@)
 */
int CDECL _finite(double num)
{
    union { double f; UINT64 i; } u = { num };
    return (u.i & ~0ull >> 1) < 0x7ffull << 52;
}

/*********************************************************************
 *		_fpreset (MSVCRT.@)
 */
void CDECL _fpreset(void)
{
#if (defined(__GNUC__) || defined(__clang__)) && defined(__i386__)
    const unsigned int x86_cw = 0x27f;
    __asm__ __volatile__( "fninit; fldcw %0" : : "m" (x86_cw) );
    if (sse2_supported)
    {
        unsigned int cw = _MCW_EM, sw = 0;
        _setfp_sse(&cw, ~0, &sw, ~0);
    }
#else
    unsigned int cw = _MCW_EM, sw = 0;
    _setfp(&cw, ~0, &sw, ~0);
#endif
}

#if _MSVCR_VER>=120
/*********************************************************************
 *              fesetenv (MSVCR120.@)
 */
int CDECL fesetenv(const fenv_t *env)
{
    unsigned int x87_cw, cw, x87_stat, stat;
    unsigned int mask;

    TRACE( "(%p)\n", env );

    if (!env->_Fe_ctl && !env->_Fe_stat) {
        _fpreset();
        return 0;
    }

    if (!fenv_decode(env->_Fe_ctl, &x87_cw, &cw))
        return 1;
    if (!fenv_decode(env->_Fe_stat, &x87_stat, &stat))
        return 1;

#if _MSVCR_VER >= 140
    mask = ~0;
#else
    mask = _EM_INEXACT | _EM_UNDERFLOW | _EM_OVERFLOW
        | _EM_ZERODIVIDE | _EM_INVALID | _MCW_RC;
#endif

#ifdef __i386__
    _setfp(&x87_cw, mask, &x87_stat, ~0);
    if (sse2_supported)
        _setfp_sse(&cw, mask, &stat, ~0);
    return 0;
#else
    _setfp(&cw, mask, &stat, ~0);
    return 0;
#endif
}
#endif

/*********************************************************************
 *		_isnan (MSVCRT.@)
 */
int CDECL _isnan(double num)
{
    union { double f; UINT64 i; } u = { num };
    return (u.i & ~0ull >> 1) > 0x7ffull << 52;
}

#if _MSVCR_VER>=120

/*********************************************************************
 *		_nearbyint (MSVCR120.@)
 *
 * Based on musl: src/math/nearbyteint.c
 */
double CDECL nearbyint(double x)
{
    BOOL update_cw, update_sw;
    unsigned int cw, sw;

    _setfp(&cw, 0, &sw, 0);
    update_cw = !(cw & _EM_INEXACT);
    update_sw = !(sw & _SW_INEXACT);
    if (update_cw)
    {
        cw |= _EM_INEXACT;
        _setfp(&cw, _EM_INEXACT, NULL, 0);
    }
    x = rint(x);
    if (update_cw || update_sw)
    {
        sw = 0;
        cw &= ~_EM_INEXACT;
        _setfp(update_cw ? &cw : NULL, _EM_INEXACT,
                update_sw ? &sw : NULL, _SW_INEXACT);
    }
    return x;
}

/*********************************************************************
 *		_nearbyintf (MSVCR120.@)
 *
 * Based on musl: src/math/nearbyteintf.c
 */
float CDECL nearbyintf(float x)
{
    BOOL update_cw, update_sw;
    unsigned int cw, sw;

    _setfp(&cw, 0, &sw, 0);
    update_cw = !(cw & _EM_INEXACT);
    update_sw = !(sw & _SW_INEXACT);
    if (update_cw)
    {
        cw |= _EM_INEXACT;
        _setfp(&cw, _EM_INEXACT, NULL, 0);
    }
    x = rintf(x);
    if (update_cw || update_sw)
    {
        sw = 0;
        cw &= ~_EM_INEXACT;
        _setfp(update_cw ? &cw : NULL, _EM_INEXACT,
                update_sw ? &sw : NULL, _SW_INEXACT);
    }
    return x;
}

#endif /* _MSVCR_VER>=120 */

/*********************************************************************
 *		_ecvt (MSVCRT.@)
 */
char * CDECL _ecvt( double number, int ndigits, int *decpt, int *sign )
{
    int prec, len;
    thread_data_t *data = msvcrt_get_thread_data();
    /* FIXME: check better for overflow (native supports over 300 chars) */
    ndigits = min( ndigits, 80 - 8); /* 8 : space for sign, dec point, "e",
                                      * 4 for exponent and one for
                                      * terminating '\0' */
    if (!data->efcvt_buffer)
        data->efcvt_buffer = malloc( 80 ); /* ought to be enough */

    /* handle cases with zero ndigits or less */
    prec = ndigits;
    if( prec < 1) prec = 2;
    len = _snprintf(data->efcvt_buffer, 80, "%.*le", prec - 1, number);

    if (data->efcvt_buffer[0] == '-') {
        memmove( data->efcvt_buffer, data->efcvt_buffer + 1, len-- );
        *sign = 1;
    } else *sign = 0;

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
int CDECL _ecvt_s( char *buffer, size_t length, double number, int ndigits, int *decpt, int *sign )
{
    int prec, len;
    char *result;

    if (!MSVCRT_CHECK_PMT(buffer != NULL)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(decpt != NULL)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(sign != NULL)) return EINVAL;
    if (!MSVCRT_CHECK_PMT_ERR( length > 2, ERANGE )) return ERANGE;
    if (!MSVCRT_CHECK_PMT_ERR(ndigits < (int)length - 1, ERANGE )) return ERANGE;

    /* handle cases with zero ndigits or less */
    prec = ndigits;
    if( prec < 1) prec = 2;
    result = malloc(prec + 8);

    len = _snprintf(result, prec + 8, "%.*le", prec - 1, number);
    if (result[0] == '-') {
        memmove( result, result + 1, len-- );
        *sign = 1;
    } else *sign = 0;

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
    free( result );
    return 0;
}

/***********************************************************************
 *		_fcvt  (MSVCRT.@)
 */
char * CDECL _fcvt( double number, int ndigits, int *decpt, int *sign )
{
    thread_data_t *data = msvcrt_get_thread_data();
    int stop, dec1, dec2;
    char *ptr1, *ptr2, *first;
    char buf[80]; /* ought to be enough */
    char decimal_separator = get_locinfo()->lconv->decimal_point[0];

    if (!data->efcvt_buffer)
        data->efcvt_buffer = malloc( 80 ); /* ought to be enough */

    stop = _snprintf(buf, 80, "%.*f", ndigits < 0 ? 0 : ndigits, number);
    ptr1 = buf;
    ptr2 = data->efcvt_buffer;
    first = NULL;
    dec1 = 0;
    dec2 = 0;

    if (*ptr1 == '-') {
        *sign = 1;
        ptr1++;
    } else *sign = 0;

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
int CDECL _fcvt_s(char* outbuffer, size_t size, double number, int ndigits, int *decpt, int *sign)
{
    int stop, dec1, dec2;
    char *ptr1, *ptr2, *first;
    char buf[80]; /* ought to be enough */
    char decimal_separator = get_locinfo()->lconv->decimal_point[0];

    if (!outbuffer || !decpt || !sign || size == 0)
    {
        *_errno() = EINVAL;
        return EINVAL;
    }

    stop = _snprintf(buf, 80, "%.*f", ndigits < 0 ? 0 : ndigits, number);
    ptr1 = buf;
    ptr2 = outbuffer;
    first = NULL;
    dec1 = 0;
    dec2 = 0;

    if (*ptr1 == '-') {
        *sign = 1;
        ptr1++;
    } else *sign = 0;

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
char * CDECL _gcvt( double number, int ndigit, char *buff )
{
    if(!buff) {
        *_errno() = EINVAL;
        return NULL;
    }

    if(ndigit < 0) {
        *_errno() = ERANGE;
        return NULL;
    }

    sprintf(buff, "%.*g", ndigit, number);
    return buff;
}

/***********************************************************************
 *              _gcvt_s  (MSVCRT.@)
 */
int CDECL _gcvt_s(char *buff, size_t size, double number, int digits)
{
    int len;

    if(!buff) {
        *_errno() = EINVAL;
        return EINVAL;
    }

    if( digits<0 || digits>=size) {
        if(size)
            buff[0] = '\0';

        *_errno() = ERANGE;
        return ERANGE;
    }

    len = _scprintf("%.*g", digits, number);
    if(len > size) {
        buff[0] = '\0';
        *_errno() = ERANGE;
        return ERANGE;
    }

    sprintf(buff, "%.*g", digits, number);
    return 0;
}

#include <stdlib.h> /* div_t, ldiv_t */

/*********************************************************************
 *		div (MSVCRT.@)
 * VERSION
 *	[i386] Windows binary compatible - returns the struct in eax/edx.
 */
#ifdef __i386__
unsigned __int64 CDECL div(int num, int denom)
{
    union {
        div_t div;
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
div_t CDECL div(int num, int denom)
{
    div_t ret;

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
unsigned __int64 CDECL ldiv(__msvcrt_long num, __msvcrt_long denom)
{
    union {
        ldiv_t ldiv;
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
ldiv_t CDECL ldiv(__msvcrt_long num, __msvcrt_long denom)
{
    ldiv_t ret;

    ret.quot = num / denom;
    ret.rem = num % denom;
    return ret;
}
#endif /* ifdef __i386__ */

#if _MSVCR_VER>=100
/*********************************************************************
 *		lldiv (MSVCR100.@)
 */
lldiv_t CDECL lldiv(__int64 num, __int64 denom)
{
  lldiv_t ret;

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
void __cdecl __libm_sse2_acos(void)
{
    double d;
    __asm__ __volatile__( "movq %%xmm0,%0" : "=m" (d) );
    d = acos( d );
    __asm__ __volatile__( "movq %0,%%xmm0" : : "m" (d) );
}

/***********************************************************************
 *		__libm_sse2_acosf   (MSVCRT.@)
 */
void __cdecl __libm_sse2_acosf(void)
{
    float f;
    __asm__ __volatile__( "movd %%xmm0,%0" : "=g" (f) );
    f = acosf( f );
    __asm__ __volatile__( "movd %0,%%xmm0" : : "g" (f) );
}

/***********************************************************************
 *		__libm_sse2_asin   (MSVCRT.@)
 */
void __cdecl __libm_sse2_asin(void)
{
    double d;
    __asm__ __volatile__( "movq %%xmm0,%0" : "=m" (d) );
    d = asin( d );
    __asm__ __volatile__( "movq %0,%%xmm0" : : "m" (d) );
}

/***********************************************************************
 *		__libm_sse2_asinf   (MSVCRT.@)
 */
void __cdecl __libm_sse2_asinf(void)
{
    float f;
    __asm__ __volatile__( "movd %%xmm0,%0" : "=g" (f) );
    f = asinf( f );
    __asm__ __volatile__( "movd %0,%%xmm0" : : "g" (f) );
}

/***********************************************************************
 *		__libm_sse2_atan   (MSVCRT.@)
 */
void __cdecl __libm_sse2_atan(void)
{
    double d;
    __asm__ __volatile__( "movq %%xmm0,%0" : "=m" (d) );
    d = atan( d );
    __asm__ __volatile__( "movq %0,%%xmm0" : : "m" (d) );
}

/***********************************************************************
 *		__libm_sse2_atan2   (MSVCRT.@)
 */
void __cdecl __libm_sse2_atan2(void)
{
    double d1, d2;
    __asm__ __volatile__( "movq %%xmm0,%0; movq %%xmm1,%1 " : "=m" (d1), "=m" (d2) );
    d1 = atan2( d1, d2 );
    __asm__ __volatile__( "movq %0,%%xmm0" : : "m" (d1) );
}

/***********************************************************************
 *		__libm_sse2_atanf   (MSVCRT.@)
 */
void __cdecl __libm_sse2_atanf(void)
{
    float f;
    __asm__ __volatile__( "movd %%xmm0,%0" : "=g" (f) );
    f = atanf( f );
    __asm__ __volatile__( "movd %0,%%xmm0" : : "g" (f) );
}

/***********************************************************************
 *		__libm_sse2_cos   (MSVCRT.@)
 */
void __cdecl __libm_sse2_cos(void)
{
    double d;
    __asm__ __volatile__( "movq %%xmm0,%0" : "=m" (d) );
    d = cos( d );
    __asm__ __volatile__( "movq %0,%%xmm0" : : "m" (d) );
}

/***********************************************************************
 *		__libm_sse2_cosf   (MSVCRT.@)
 */
void __cdecl __libm_sse2_cosf(void)
{
    float f;
    __asm__ __volatile__( "movd %%xmm0,%0" : "=g" (f) );
    f = cosf( f );
    __asm__ __volatile__( "movd %0,%%xmm0" : : "g" (f) );
}

/***********************************************************************
 *		__libm_sse2_exp   (MSVCRT.@)
 */
void __cdecl __libm_sse2_exp(void)
{
    double d;
    __asm__ __volatile__( "movq %%xmm0,%0" : "=m" (d) );
    d = exp( d );
    __asm__ __volatile__( "movq %0,%%xmm0" : : "m" (d) );
}

/***********************************************************************
 *		__libm_sse2_expf   (MSVCRT.@)
 */
void __cdecl __libm_sse2_expf(void)
{
    float f;
    __asm__ __volatile__( "movd %%xmm0,%0" : "=g" (f) );
    f = expf( f );
    __asm__ __volatile__( "movd %0,%%xmm0" : : "g" (f) );
}

/***********************************************************************
 *		__libm_sse2_log   (MSVCRT.@)
 */
void __cdecl __libm_sse2_log(void)
{
    double d;
    __asm__ __volatile__( "movq %%xmm0,%0" : "=m" (d) );
    d = log( d );
    __asm__ __volatile__( "movq %0,%%xmm0" : : "m" (d) );
}

/***********************************************************************
 *		__libm_sse2_log10   (MSVCRT.@)
 */
void __cdecl __libm_sse2_log10(void)
{
    double d;
    __asm__ __volatile__( "movq %%xmm0,%0" : "=m" (d) );
    d = log10( d );
    __asm__ __volatile__( "movq %0,%%xmm0" : : "m" (d) );
}

/***********************************************************************
 *		__libm_sse2_log10f   (MSVCRT.@)
 */
void __cdecl __libm_sse2_log10f(void)
{
    float f;
    __asm__ __volatile__( "movd %%xmm0,%0" : "=g" (f) );
    f = log10f( f );
    __asm__ __volatile__( "movd %0,%%xmm0" : : "g" (f) );
}

/***********************************************************************
 *		__libm_sse2_logf   (MSVCRT.@)
 */
void __cdecl __libm_sse2_logf(void)
{
    float f;
    __asm__ __volatile__( "movd %%xmm0,%0" : "=g" (f) );
    f = logf( f );
    __asm__ __volatile__( "movd %0,%%xmm0" : : "g" (f) );
}

/***********************************************************************
 *		__libm_sse2_pow   (MSVCRT.@)
 */
void __cdecl __libm_sse2_pow(void)
{
    double d1, d2;
    __asm__ __volatile__( "movq %%xmm0,%0; movq %%xmm1,%1 " : "=m" (d1), "=m" (d2) );
    d1 = pow( d1, d2 );
    __asm__ __volatile__( "movq %0,%%xmm0" : : "m" (d1) );
}

/***********************************************************************
 *		__libm_sse2_powf   (MSVCRT.@)
 */
void __cdecl __libm_sse2_powf(void)
{
    float f1, f2;
    __asm__ __volatile__( "movd %%xmm0,%0; movd %%xmm1,%1" : "=g" (f1), "=g" (f2) );
    f1 = powf( f1, f2 );
    __asm__ __volatile__( "movd %0,%%xmm0" : : "g" (f1) );
}

/***********************************************************************
 *		__libm_sse2_sin   (MSVCRT.@)
 */
void __cdecl __libm_sse2_sin(void)
{
    double d;
    __asm__ __volatile__( "movq %%xmm0,%0" : "=m" (d) );
    d = sin( d );
    __asm__ __volatile__( "movq %0,%%xmm0" : : "m" (d) );
}

/***********************************************************************
 *		__libm_sse2_sinf   (MSVCRT.@)
 */
void __cdecl __libm_sse2_sinf(void)
{
    float f;
    __asm__ __volatile__( "movd %%xmm0,%0" : "=g" (f) );
    f = sinf( f );
    __asm__ __volatile__( "movd %0,%%xmm0" : : "g" (f) );
}

/***********************************************************************
 *		__libm_sse2_tan   (MSVCRT.@)
 */
void __cdecl __libm_sse2_tan(void)
{
    double d;
    __asm__ __volatile__( "movq %%xmm0,%0" : "=m" (d) );
    d = tan( d );
    __asm__ __volatile__( "movq %0,%%xmm0" : : "m" (d) );
}

/***********************************************************************
 *		__libm_sse2_tanf   (MSVCRT.@)
 */
void __cdecl __libm_sse2_tanf(void)
{
    float f;
    __asm__ __volatile__( "movd %%xmm0,%0" : "=g" (f) );
    f = tanf( f );
    __asm__ __volatile__( "movd %0,%%xmm0" : : "g" (f) );
}

/***********************************************************************
 *		__libm_sse2_sqrt_precise   (MSVCR110.@)
 */
void __cdecl __libm_sse2_sqrt_precise(void)
{
    unsigned int cw;
    double d;

    __asm__ __volatile__( "movq %%xmm0,%0" : "=m" (d) );
    __control87_2(0, 0, NULL, &cw);
    if (cw & _MCW_RC)
    {
        d = sqrt(d);
        __asm__ __volatile__( "movq %0,%%xmm0" : : "m" (d) );
        return;
    }

    if (!sqrt_validate(&d, FALSE))
    {
        __asm__ __volatile__( "movq %0,%%xmm0" : : "m" (d) );
        return;
    }
    __asm__ __volatile__( "call " __ASM_NAME( "sse2_sqrt" ) );
}
#endif  /* __i386__ */

#if _MSVCR_VER>=120

/*********************************************************************
 *      rintf (MSVCR120.@)
 *
 * Copied from musl: src/math/rintf.c
 */
float CDECL rintf(float x)
{
    static const float toint = 1 / FLT_EPSILON;

    unsigned int ix = *(unsigned int*)&x;
    int e = ix >> 23 & 0xff;
    int s = ix >> 31;
    float y;

    if (e >= 0x7f + 23)
        return x;
    if (s)
        y = fp_barrierf(x - toint) + toint;
    else
        y = fp_barrierf(x + toint) - toint;
    if (y == 0)
        return s ? -0.0f : 0.0f;
    return y;
}

/*********************************************************************
 *      lrint (MSVCR120.@)
 */
__msvcrt_long CDECL lrint(double x)
{
    double d;

    d = rint(x);
    if ((d < 0 && d != (double)(__msvcrt_long)d)
            || (d >= 0 && d != (double)(__msvcrt_ulong)d)) {
        *_errno() = EDOM;
        return 0;
    }
    return d;
}

/*********************************************************************
 *      lrintf (MSVCR120.@)
 */
__msvcrt_long CDECL lrintf(float x)
{
    float f;

    f = rintf(x);
    if ((f < 0 && f != (float)(__msvcrt_long)f)
            || (f >= 0 && f != (float)(__msvcrt_ulong)f)) {
        *_errno() = EDOM;
        return 0;
    }
    return f;
}

/*********************************************************************
 *      llrint (MSVCR120.@)
 */
__int64 CDECL llrint(double x)
{
    double d;

    d = rint(x);
    if ((d < 0 && d != (double)(__int64)d)
            || (d >= 0 && d != (double)(unsigned __int64)d)) {
        *_errno() = EDOM;
        return 0;
    }
    return d;
}

/*********************************************************************
 *      llrintf (MSVCR120.@)
 */
__int64 CDECL llrintf(float x)
{
    float f;

    f = rintf(x);
    if ((f < 0 && f != (float)(__int64)f)
            || (f >= 0 && f != (float)(unsigned __int64)f)) {
        *_errno() = EDOM;
        return 0;
    }
    return f;
}

/*********************************************************************
 *      lround (MSVCR120.@)
 *
 * Copied from musl: src/math/lround.c
 */
__msvcrt_long CDECL lround(double x)
{
    double d = round(x);
    if (d != (double)(__msvcrt_long)d) {
        *_errno() = EDOM;
        return 0;
    }
    return d;
}

/*********************************************************************
 *      lroundf (MSVCR120.@)
 *
 * Copied from musl: src/math/lroundf.c
 */
__msvcrt_long CDECL lroundf(float x)
{
    float f = roundf(x);
    if (f != (float)(__msvcrt_long)f) {
        *_errno() = EDOM;
        return 0;
    }
    return f;
}

/*********************************************************************
 *      llround (MSVCR120.@)
 *
 * Copied from musl: src/math/llround.c
 */
__int64 CDECL llround(double x)
{
    double d = round(x);
    if (d != (double)(__int64)d) {
        *_errno() = EDOM;
        return 0;
    }
    return d;
}

/*********************************************************************
 *      llroundf (MSVCR120.@)
 *
 * Copied from musl: src/math/llroundf.c
 */
__int64 CDECL llroundf(float x)
{
    float f = roundf(x);
    if (f != (float)(__int64)f) {
        *_errno() = EDOM;
        return 0;
    }
    return f;
}

/*********************************************************************
 *      _dtest (MSVCR120.@)
 */
short CDECL _dtest(double *x)
{
    return _dclass(*x);
}

/*********************************************************************
 *      _fdtest (MSVCR120.@)
 */
short CDECL _fdtest(float *x)
{
    return _fdclass(*x);
}

static double erfc1(double x)
{
    static const double erx  = 8.45062911510467529297e-01,
                 pa0  = -2.36211856075265944077e-03,
                 pa1  =  4.14856118683748331666e-01,
                 pa2  = -3.72207876035701323847e-01,
                 pa3  =  3.18346619901161753674e-01,
                 pa4  = -1.10894694282396677476e-01,
                 pa5  =  3.54783043256182359371e-02,
                 pa6  = -2.16637559486879084300e-03,
                 qa1  =  1.06420880400844228286e-01,
                 qa2  =  5.40397917702171048937e-01,
                 qa3  =  7.18286544141962662868e-02,
                 qa4  =  1.26171219808761642112e-01,
                 qa5  =  1.36370839120290507362e-02,
                 qa6  =  1.19844998467991074170e-02;

    double s, P, Q;

    s = fabs(x) - 1;
    P = pa0 + s * (pa1 + s * (pa2 + s * (pa3 + s * (pa4 + s * (pa5 + s * pa6)))));
    Q = 1 + s * (qa1 + s * (qa2 + s * (qa3 + s * (qa4 + s * (qa5 + s * qa6)))));
    return 1 - erx - P / Q;
}

static double erfc2(UINT32 ix, double x)
{
    static const double ra0  = -9.86494403484714822705e-03,
                 ra1  = -6.93858572707181764372e-01,
                 ra2  = -1.05586262253232909814e+01,
                 ra3  = -6.23753324503260060396e+01,
                 ra4  = -1.62396669462573470355e+02,
                 ra5  = -1.84605092906711035994e+02,
                 ra6  = -8.12874355063065934246e+01,
                 ra7  = -9.81432934416914548592e+00,
                 sa1  =  1.96512716674392571292e+01,
                 sa2  =  1.37657754143519042600e+02,
                 sa3  =  4.34565877475229228821e+02,
                 sa4  =  6.45387271733267880336e+02,
                 sa5  =  4.29008140027567833386e+02,
                 sa6  =  1.08635005541779435134e+02,
                 sa7  =  6.57024977031928170135e+00,
                 sa8  = -6.04244152148580987438e-02,
                 rb0  = -9.86494292470009928597e-03,
                 rb1  = -7.99283237680523006574e-01,
                 rb2  = -1.77579549177547519889e+01,
                 rb3  = -1.60636384855821916062e+02,
                 rb4  = -6.37566443368389627722e+02,
                 rb5  = -1.02509513161107724954e+03,
                 rb6  = -4.83519191608651397019e+02,
                 sb1  =  3.03380607434824582924e+01,
                 sb2  =  3.25792512996573918826e+02,
                 sb3  =  1.53672958608443695994e+03,
                 sb4  =  3.19985821950859553908e+03,
                 sb5  =  2.55305040643316442583e+03,
                 sb6  =  4.74528541206955367215e+02,
                 sb7  = -2.24409524465858183362e+01;

    double s, R, S, z;
    UINT64 iz;

    if (ix < 0x3ff40000) /* |x| < 1.25 */
        return erfc1(x);

    x = fabs(x);
    s = 1 / (x * x);
    if (ix < 0x4006db6d) { /* |x| < 1/.35 ~ 2.85714 */
        R = ra0 + s * (ra1 + s * (ra2 + s * (ra3 + s * (ra4 + s *
                            (ra5 + s * (ra6 + s * ra7))))));
        S = 1.0 + s * (sa1 + s * (sa2 + s * (sa3 + s * (sa4 + s *
                            (sa5 + s * (sa6 + s * (sa7 + s * sa8)))))));
    } else { /* |x| > 1/.35 */
        R = rb0 + s * (rb1 + s * (rb2 + s * (rb3 + s * (rb4 + s *
                            (rb5 + s * rb6)))));
        S = 1.0 + s * (sb1 + s * (sb2 + s * (sb3 + s * (sb4 + s *
                            (sb5 + s * (sb6 + s * sb7))))));
    }
    z = x;
    iz = *(ULONGLONG*)&z;
    iz &= 0xffffffff00000000ULL;
    z = *(double*)&iz;
    return exp(-z * z - 0.5625) * exp((z - x) * (z + x) + R / S) / x;
}

/*********************************************************************
 *      erf (MSVCR120.@)
 */
double CDECL erf(double x)
{
    static const double efx8 =  1.02703333676410069053e+00,
                 pp0  =  1.28379167095512558561e-01,
                 pp1  = -3.25042107247001499370e-01,
                 pp2  = -2.84817495755985104766e-02,
                 pp3  = -5.77027029648944159157e-03,
                 pp4  = -2.37630166566501626084e-05,
                 qq1  =  3.97917223959155352819e-01,
                 qq2  =  6.50222499887672944485e-02,
                 qq3  =  5.08130628187576562776e-03,
                 qq4  =  1.32494738004321644526e-04,
                 qq5  = -3.96022827877536812320e-06;

    double r, s, z, y;
    UINT32 ix;
    int sign;

    ix = *(UINT64*)&x >> 32;
    sign = ix >> 31;
    ix &= 0x7fffffff;
    if (ix >= 0x7ff00000) {
        /* erf(nan)=nan, erf(+-inf)=+-1 */
        return 1 - 2 * sign + 1 / x;
    }
    if (ix < 0x3feb0000) { /* |x| < 0.84375 */
        if (ix < 0x3e300000) { /* |x| < 2**-28 */
            /* avoid underflow */
            return 0.125 * (8 * x + efx8 * x);
        }
        z = x * x;
        r = pp0 + z * (pp1 + z * (pp2 + z * (pp3 + z * pp4)));
        s = 1.0 + z * (qq1 + z * (qq2 + z * (qq3 + z * (qq4 + z * qq5))));
        y = r / s;
        return x + x * y;
    }
    if (ix < 0x40180000) /* 0.84375 <= |x| < 6 */
        y = 1 - erfc2(ix, x);
    else
        y = 1 - DBL_MIN;
    return sign ? -y : y;
}

static float erfc1f(float x)
{
    static const float erx  =  8.4506291151e-01,
                 pa0  = -2.3621185683e-03,
                 pa1  =  4.1485610604e-01,
                 pa2  = -3.7220788002e-01,
                 pa3  =  3.1834661961e-01,
                 pa4  = -1.1089469492e-01,
                 pa5  =  3.5478305072e-02,
                 pa6  = -2.1663755178e-03,
                 qa1  =  1.0642088205e-01,
                 qa2  =  5.4039794207e-01,
                 qa3  =  7.1828655899e-02,
                 qa4  =  1.2617121637e-01,
                 qa5  =  1.3637083583e-02,
                 qa6  =  1.1984500103e-02;

    float s, P, Q;

    s = fabsf(x) - 1;
    P = pa0 + s * (pa1 + s * (pa2 + s * (pa3 + s * (pa4 + s * (pa5 + s * pa6)))));
    Q = 1 + s * (qa1 + s * (qa2 + s * (qa3 + s * (qa4 + s * (qa5 + s * qa6)))));
    return 1 - erx - P / Q;
}

static float erfc2f(UINT32 ix, float x)
{
    static const float ra0  = -9.8649440333e-03,
                 ra1  = -6.9385856390e-01,
                 ra2  = -1.0558626175e+01,
                 ra3  = -6.2375331879e+01,
                 ra4  = -1.6239666748e+02,
                 ra5  = -1.8460508728e+02,
                 ra6  = -8.1287437439e+01,
                 ra7  = -9.8143291473e+00,
                 sa1  =  1.9651271820e+01,
                 sa2  =  1.3765776062e+02,
                 sa3  =  4.3456588745e+02,
                 sa4  =  6.4538726807e+02,
                 sa5  =  4.2900814819e+02,
                 sa6  =  1.0863500214e+02,
                 sa7  =  6.5702495575e+00,
                 sa8  = -6.0424413532e-02,
                 rb0  = -9.8649431020e-03,
                 rb1  = -7.9928326607e-01,
                 rb2  = -1.7757955551e+01,
                 rb3  = -1.6063638306e+02,
                 rb4  = -6.3756646729e+02,
                 rb5  = -1.0250950928e+03,
                 rb6  = -4.8351919556e+02,
                 sb1  =  3.0338060379e+01,
                 sb2  =  3.2579251099e+02,
                 sb3  =  1.5367296143e+03,
                 sb4  =  3.1998581543e+03,
                 sb5  =  2.5530502930e+03,
                 sb6  =  4.7452853394e+02,
                 sb7  = -2.2440952301e+01;

    float s, R, S, z;

    if (ix < 0x3fa00000) /* |x| < 1.25 */
        return erfc1f(x);

    x = fabsf(x);
    s = 1 / (x * x);
    if (ix < 0x4036db6d) { /* |x| < 1/0.35 */
        R = ra0 + s * (ra1 + s * (ra2 + s * (ra3 + s * (ra4 + s *
                            (ra5 + s * (ra6 + s * ra7))))));
        S = 1.0f + s * (sa1 + s * (sa2 + s * (sa3 + s * (sa4 + s *
                            (sa5 + s * (sa6 + s * (sa7 + s * sa8)))))));
    } else { /* |x| >= 1/0.35 */
        R = rb0 + s * (rb1 + s * (rb2 + s * (rb3 + s * (rb4 + s * (rb5 + s * rb6)))));
        S = 1.0f + s * (sb1 + s * (sb2 + s * (sb3 + s * (sb4 + s *
                            (sb5 + s * (sb6 + s * sb7))))));
    }

    ix = *(UINT32*)&x & 0xffffe000;
    z = *(float*)&ix;
    return expf(-z * z - 0.5625f) * expf((z - x) * (z + x) + R / S) / x;
}

/*********************************************************************
 *      erff (MSVCR120.@)
 *
 * Copied from musl: src/math/erff.c
 */
float CDECL erff(float x)
{
    static const float efx8 =  1.0270333290e+00,
                 pp0  =  1.2837916613e-01,
                 pp1  = -3.2504209876e-01,
                 pp2  = -2.8481749818e-02,
                 pp3  = -5.7702702470e-03,
                 pp4  = -2.3763017452e-05,
                 qq1  =  3.9791721106e-01,
                 qq2  =  6.5022252500e-02,
                 qq3  =  5.0813062117e-03,
                 qq4  =  1.3249473704e-04,
                 qq5  = -3.9602282413e-06;

    float r, s, z, y;
    UINT32 ix;
    int sign;

    ix = *(UINT32*)&x;
    sign = ix >> 31;
    ix &= 0x7fffffff;
    if (ix >= 0x7f800000) {
        /* erf(nan)=nan, erf(+-inf)=+-1 */
        return 1 - 2 * sign + 1 / x;
    }
    if (ix < 0x3f580000) { /* |x| < 0.84375 */
        if (ix < 0x31800000) { /* |x| < 2**-28 */
            /*avoid underflow */
            return 0.125f * (8 * x + efx8 * x);
        }
        z = x * x;
        r = pp0 + z * (pp1 + z * (pp2 + z * (pp3 + z * pp4)));
        s = 1 + z * (qq1 + z * (qq2 + z * (qq3 + z * (qq4 + z * qq5))));
        y = r / s;
        return x + x * y;
    }
    if (ix < 0x40c00000) /* |x| < 6 */
        y = 1 - erfc2f(ix, x);
    else
        y = 1 - FLT_MIN;
    return sign ? -y : y;
}

/*********************************************************************
 *      erfc (MSVCR120.@)
 *
 * Copied from musl: src/math/erf.c
 */
double CDECL erfc(double x)
{
    static const double pp0  =  1.28379167095512558561e-01,
                 pp1  = -3.25042107247001499370e-01,
                 pp2  = -2.84817495755985104766e-02,
                 pp3  = -5.77027029648944159157e-03,
                 pp4  = -2.37630166566501626084e-05,
                 qq1  =  3.97917223959155352819e-01,
                 qq2  =  6.50222499887672944485e-02,
                 qq3  =  5.08130628187576562776e-03,
                 qq4  =  1.32494738004321644526e-04,
                 qq5  = -3.96022827877536812320e-06;

    double r, s, z, y;
    UINT32 ix;
    int sign;

    ix = *(ULONGLONG*)&x >> 32;
    sign = ix >> 31;
    ix &= 0x7fffffff;
    if (ix >= 0x7ff00000) {
        /* erfc(nan)=nan, erfc(+-inf)=0,2 */
        return 2 * sign + 1 / x;
    }
    if (ix < 0x3feb0000) { /* |x| < 0.84375 */
        if (ix < 0x3c700000) /* |x| < 2**-56 */
            return 1.0 - x;
        z = x * x;
        r = pp0 + z * (pp1 + z * (pp2 + z * (pp3 + z * pp4)));
        s = 1.0 + z * (qq1 + z * (qq2 + z * (qq3 + z * (qq4 + z * qq5))));
        y = r / s;
        if (sign || ix < 0x3fd00000) { /* x < 1/4 */
            return 1.0 - (x + x * y);
        }
        return 0.5 - (x - 0.5 + x * y);
    }
    if (ix < 0x403c0000) { /* 0.84375 <= |x| < 28 */
        return sign ? 2 - erfc2(ix, x) : erfc2(ix, x);
    }
    if (sign)
        return 2 - DBL_MIN;
    *_errno() = ERANGE;
    return fp_barrier(DBL_MIN) * DBL_MIN;
}

/*********************************************************************
 *      erfcf (MSVCR120.@)
 *
 * Copied from musl: src/math/erff.c
 */
float CDECL erfcf(float x)
{
    static const float pp0  =  1.2837916613e-01,
                 pp1  = -3.2504209876e-01,
                 pp2  = -2.8481749818e-02,
                 pp3  = -5.7702702470e-03,
                 pp4  = -2.3763017452e-05,
                 qq1  =  3.9791721106e-01,
                 qq2  =  6.5022252500e-02,
                 qq3  =  5.0813062117e-03,
                 qq4  =  1.3249473704e-04,
                 qq5  = -3.9602282413e-06;

    float r, s, z, y;
    UINT32 ix;
    int sign;

    ix = *(UINT32*)&x;
    sign = ix >> 31;
    ix &= 0x7fffffff;
    if (ix >= 0x7f800000) {
        /* erfc(nan)=nan, erfc(+-inf)=0,2 */
        return 2 * sign + 1 / x;
    }

    if (ix < 0x3f580000) { /* |x| < 0.84375 */
        if (ix < 0x23800000) /* |x| < 2**-56 */
            return 1.0f - x;
        z = x * x;
        r = pp0 + z * (pp1 + z * (pp2 + z * (pp3 + z * pp4)));
        s = 1.0f + z * (qq1 + z * (qq2 + z * (qq3 + z * (qq4 + z * qq5))));
        y = r / s;
        if (sign || ix < 0x3e800000) /* x < 1/4 */
            return 1.0f - (x + x * y);
        return 0.5f - (x - 0.5f + x * y);
    }
    if (ix < 0x41e00000) { /* |x| < 28 */
        return sign ? 2 - erfc2f(ix, x) : erfc2f(ix, x);
    }
    if (sign)
        return 2 - FLT_MIN;
    *_errno() = ERANGE;
    return FLT_MIN * FLT_MIN;
}

/*********************************************************************
 *      _fdsign (MSVCR120.@)
 */
int CDECL _fdsign(float x)
{
    union { float f; UINT32 i; } u = { x };
    return (u.i >> 16) & 0x8000;
}

/*********************************************************************
 *      _dsign (MSVCR120.@)
 */
int CDECL _dsign(double x)
{
    union { double f; UINT64 i; } u = { x };
    return (u.i >> 48) & 0x8000;
}


/*********************************************************************
 *      _dpcomp (MSVCR120.@)
 */
int CDECL _dpcomp(double x, double y)
{
    if(isnan(x) || isnan(y))
        return 0;

    if(x == y) return 2;
    return x < y ? 1 : 4;
}

/*********************************************************************
 *      _fdpcomp (MSVCR120.@)
 */
int CDECL _fdpcomp(float x, float y)
{
    return _dpcomp(x, y);
}

/*********************************************************************
 *      asinh (MSVCR120.@)
 *
 * Copied from musl: src/math/asinh.c
 */
double CDECL asinh(double x)
{
    UINT64 ux = *(UINT64*)&x;
    int e = ux >> 52 & 0x7ff;
    int s = ux >> 63;

    /* |x| */
    ux &= (UINT64)-1 / 2;
    x = *(double*)&ux;

    if (e >= 0x3ff + 26) /* |x| >= 0x1p26 or inf or nan */
        x = log(x) + 0.693147180559945309417232121458176568;
    else if (e >= 0x3ff + 1) /* |x| >= 2 */
        x = log(2 * x + 1 / (sqrt(x * x + 1) + x));
    else if (e >= 0x3ff - 26) /* |x| >= 0x1p-26 */
        x = log1p(x + x * x / (sqrt(x * x + 1) + 1));
    else /* |x| < 0x1p-26, raise inexact if x != 0 */
        fp_barrier(x + 0x1p120f);
    return s ? -x : x;
}

/*********************************************************************
 *      asinhf (MSVCR120.@)
 *
 * Copied from musl: src/math/asinhf.c
 */
float CDECL asinhf(float x)
{
    UINT32 ux = *(UINT32*)&x;
    UINT32 i = ux & 0x7fffffff;
    int s = ux >> 31;

    /* |x| */
    x = *(float*)&i;

    if (i >= 0x3f800000 + (12 << 23))/* |x| >= 0x1p12 or inf or nan */
        x = logf(x) + 0.693147180559945309417232121458176568f;
    else if (i >= 0x3f800000 + (1 << 23)) /* |x| >= 2 */
        x = logf(2 * x + 1 / (sqrtf(x * x + 1) + x));
    else if (i >= 0x3f800000 - (12 << 23)) /* |x| >= 0x1p-12 */
        x = log1pf(x + x * x / (sqrtf(x * x + 1) + 1));
    else /* |x| < 0x1p-12, raise inexact if x!=0 */
        fp_barrierf(x + 0x1p120f);
    return s ? -x : x;
}

/*********************************************************************
 *      acosh (MSVCR120.@)
 *
 * Copied from musl: src/math/acosh.c
 */
double CDECL acosh(double x)
{
    int e = *(UINT64*)&x >> 52 & 0x7ff;

    if (x < 1)
    {
        *_errno() = EDOM;
        feraiseexcept(FE_INVALID);
        return NAN;
    }

    if (e < 0x3ff + 1) /* |x| < 2, up to 2ulp error in [1,1.125] */
        return log1p(x - 1 + sqrt((x - 1) * (x - 1) + 2 * (x - 1)));
    if (e < 0x3ff + 26) /* |x| < 0x1p26 */
        return log(2 * x - 1 / (x + sqrt(x * x - 1)));
    /* |x| >= 0x1p26 or nan */
    return log(x) + 0.693147180559945309417232121458176568;
}

/*********************************************************************
 *      acoshf (MSVCR120.@)
 *
 * Copied from musl: src/math/acoshf.c
 */
float CDECL acoshf(float x)
{
    UINT32 a = *(UINT32*)&x & 0x7fffffff;

    if (x < 1)
    {
        *_errno() = EDOM;
        feraiseexcept(FE_INVALID);
        return NAN;
    }

    if (a < 0x3f800000 + (1 << 23)) /* |x| < 2, up to 2ulp error in [1,1.125] */
        return log1pf(x - 1 + sqrtf((x - 1) * (x - 1) + 2 * (x - 1)));
    if (*(UINT32*)&x < 0x3f800000 + (12 << 23)) /* 2 <= x < 0x1p12 */
        return logf(2 * x - 1 / (x + sqrtf(x * x - 1)));
    /* x >= 0x1p12 or x <= -2 or nan */
    return logf(x) + 0.693147180559945309417232121458176568f;
}

/*********************************************************************
 *      atanh (MSVCR120.@)
 *
 * Copied from musl: src/math/atanh.c
 */
double CDECL atanh(double x)
{
    UINT64 ux = *(UINT64*)&x;
    int e = ux >> 52 & 0x7ff;
    int s = ux >> 63;

    /* |x| */
    ux &= (UINT64)-1 / 2;
    x = *(double*)&ux;

    if (x > 1) {
        *_errno() = EDOM;
        feraiseexcept(FE_INVALID);
        return NAN;
    }

    if (e < 0x3ff - 1) {
        if (e < 0x3ff - 32) {
            fp_barrier(x + 0x1p120f);
            if (e == 0) /* handle underflow */
                fp_barrier(x * x);
        } else { /* |x| < 0.5, up to 1.7ulp error */
            x = 0.5 * log1p(2 * x + 2 * x * x / (1 - x));
        }
    } else { /* avoid overflow */
        x = 0.5 * log1p(2 * (x / (1 - x)));
        if (isinf(x)) *_errno() = ERANGE;
    }
    return s ? -x : x;
}

/*********************************************************************
 *      atanhf (MSVCR120.@)
 *
 * Copied from musl: src/math/atanhf.c
 */
float CDECL atanhf(float x)
{
    UINT32 ux = *(UINT32*)&x;
    int s = ux >> 31;

    /* |x| */
    ux &= 0x7fffffff;
    x = *(float*)&ux;

    if (x > 1) {
        *_errno() = EDOM;
        feraiseexcept(FE_INVALID);
        return NAN;
    }

    if (ux < 0x3f800000 - (1 << 23)) {
        if (ux < 0x3f800000 - (32 << 23)) {
            fp_barrierf(x + 0x1p120f);
            if (ux < (1 << 23)) /* handle underflow */
                fp_barrierf(x * x);
        } else { /* |x| < 0.5, up to 1.7ulp error */
            x = 0.5f * log1pf(2 * x + 2 * x * x / (1 - x));
        }
    } else { /* avoid overflow */
        x = 0.5f * log1pf(2 * (x / (1 - x)));
        if (isinf(x)) *_errno() = ERANGE;
    }
    return s ? -x : x;
}

#endif /* _MSVCR_VER>=120 */

/*********************************************************************
 *      _scalb  (MSVCRT.@)
 *      scalbn  (MSVCR120.@)
 *      scalbln (MSVCR120.@)
 */
double CDECL _scalb(double num, __msvcrt_long power)
{
  return ldexp(num, power);
}

/*********************************************************************
 *      _scalbf  (MSVCRT.@)
 *      scalbnf  (MSVCR120.@)
 *      scalblnf (MSVCR120.@)
 */
float CDECL _scalbf(float num, __msvcrt_long power)
{
  return ldexp(num, power);
}

#if _MSVCR_VER>=120

/*********************************************************************
 *      remainder (MSVCR120.@)
 *
 * Copied from musl: src/math/remainder.c
 */
double CDECL remainder(double x, double y)
{
    int q;
#if _MSVCR_VER == 120 && defined(__x86_64__)
    if (isnan(x) || isnan(y)) *_errno() = EDOM;
#endif
    return remquo(x, y, &q);
}

/*********************************************************************
 *      remainderf (MSVCR120.@)
 *
 * Copied from musl: src/math/remainderf.c
 */
float CDECL remainderf(float x, float y)
{
    int q;
#if _MSVCR_VER == 120 && defined(__x86_64__)
    if (isnan(x) || isnan(y)) *_errno() = EDOM;
#endif
    return remquof(x, y, &q);
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
    unsigned int fpword = 0;
    WORD operation;
    int raise = 0;

    TRACE("(%lx %x %lf %lf %lx %p)\n", fpe, op, arg, res, cw, unk);

#ifdef _WIN64
    cw = ((cw >> 7) & 0x3f) | ((cw >> 3) & 0xc00);
#endif
    operation = op << 5;
    exception_arg = (ULONG_PTR)&operation;

    if (fpe & 0x1) { /* overflow */
        if ((fpe == 0x1 && (cw & 0x8)) || (fpe==0x11 && (cw & 0x28))) {
            /* 32-bit version also sets SW_INEXACT here */
            raise |= FE_OVERFLOW;
            if (fpe & 0x10) raise |= FE_INEXACT;
            res = signbit(res) ? -INFINITY : INFINITY;
        } else {
            exception = EXCEPTION_FLT_OVERFLOW;
        }
    } else if (fpe & 0x2) { /* underflow */
        if ((fpe == 0x2 && (cw & 0x10)) || (fpe==0x12 && (cw & 0x30))) {
            raise |= FE_UNDERFLOW;
            if (fpe & 0x10) raise |= FE_INEXACT;
            res = signbit(res) ? -0.0 : 0.0;
        } else {
            exception = EXCEPTION_FLT_UNDERFLOW;
        }
    } else if (fpe & 0x4) { /* zerodivide */
        if ((fpe == 0x4 && (cw & 0x4)) || (fpe==0x14 && (cw & 0x24))) {
            raise |= FE_DIVBYZERO;
            if (fpe & 0x10) raise |= FE_INEXACT;
        } else {
            exception = EXCEPTION_FLT_DIVIDE_BY_ZERO;
        }
    } else if (fpe & 0x8) { /* invalid */
        if (fpe == 0x8 && (cw & 0x1)) {
            raise |= FE_INVALID;
        } else {
            exception = EXCEPTION_FLT_INVALID_OPERATION;
        }
    } else if (fpe & 0x10) { /* inexact */
        if (fpe == 0x10 && (cw & 0x20)) {
            raise |= FE_INEXACT;
        } else {
            exception = EXCEPTION_FLT_INEXACT_RESULT;
        }
    }

    if (exception)
        raise = 0;
    feraiseexcept(raise);
    if (exception)
        RaiseException(exception, 0, 1, &exception_arg);

    if (cw & 0x1) fpword |= _EM_INVALID;
    if (cw & 0x2) fpword |= _EM_DENORMAL;
    if (cw & 0x4) fpword |= _EM_ZERODIVIDE;
    if (cw & 0x8) fpword |= _EM_OVERFLOW;
    if (cw & 0x10) fpword |= _EM_UNDERFLOW;
    if (cw & 0x20) fpword |= _EM_INEXACT;
    switch (cw & 0xc00)
    {
        case 0xc00: fpword |= _RC_UP|_RC_DOWN; break;
        case 0x800: fpword |= _RC_UP; break;
        case 0x400: fpword |= _RC_DOWN; break;
    }
    switch (cw & 0x300)
    {
        case 0x0:   fpword |= _PC_24; break;
        case 0x200: fpword |= _PC_53; break;
        case 0x300: fpword |= _PC_64; break;
    }
    if (cw & 0x1000) fpword |= _IC_AFFINE;
    _setfp(&fpword, _MCW_EM | _MCW_RC | _MCW_PC | _MCW_IC, NULL, 0);

    return res;
}

_Dcomplex* CDECL _Cbuild(_Dcomplex *ret, double r, double i)
{
    ret->_Val[0] = r;
    ret->_Val[1] = i;
    return ret;
}

double CDECL MSVCR120_creal(_Dcomplex z)
{
    return z._Val[0];
}

#endif /* _MSVCR_VER>=120 */
