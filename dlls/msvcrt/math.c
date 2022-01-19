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


static double math_error(int type, const char *name, double arg1, double arg2, double retval)
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

/*********************************************************************
 *      _copysignf (MSVCRT.@)
 *
 * Copied from musl: src/math/copysignf.c
 */
float CDECL _copysignf( float x, float y )
{
    union { float f; UINT32 i; } ux = { x }, uy = { y };
    ux.i &= 0x7fffffff;
    ux.i |= uy.i & 0x80000000;
    return ux.f;
}

/*********************************************************************
 *      _nextafterf (MSVCRT.@)
 *
 * Copied from musl: src/math/nextafterf.c
 */
float CDECL _nextafterf( float x, float y )
{
    unsigned int ix = *(unsigned int*)&x;
    unsigned int iy = *(unsigned int*)&y;
    unsigned int ax, ay, e;

    if (isnan(x) || isnan(y))
        return x + y;
    if (x == y) {
        if (_fpclassf(y) & (_FPCLASS_ND | _FPCLASS_PD | _FPCLASS_NZ | _FPCLASS_PZ ))
            *_errno() = ERANGE;
        return y;
    }
    ax = ix & 0x7fffffff;
    ay = iy & 0x7fffffff;
    if (ax == 0) {
        if (ay == 0)
            return y;
        ix = (iy & 0x80000000) | 1;
    } else if (ax > ay || ((ix ^ iy) & 0x80000000))
        ix--;
    else
        ix++;
    e = ix & 0x7f800000;
    /* raise overflow if ix is infinite and x is finite */
    if (e == 0x7f800000) {
        fp_barrierf(x + x);
        *_errno() = ERANGE;
    }
    /* raise underflow if ix is subnormal or zero */
    y = *(float*)&ix;
    if (e == 0) {
        fp_barrierf(x * x + y * y);
        *_errno() = ERANGE;
    }
    return y;
}

/* Copied from musl: src/math/ilogbf.c */
static int __ilogbf(float x)
{
    union { float f; UINT32 i; } u = { x };
    int e = u.i >> 23 & 0xff;

    if (!e)
    {
        u.i <<= 9;
        if (u.i == 0) return FP_ILOGB0;
        /* subnormal x */
        for (e = -0x7f; u.i >> 31 == 0; e--, u.i <<= 1);
        return e;
    }
    if (e == 0xff) return u.i << 9 ? FP_ILOGBNAN : INT_MAX;
    return e - 0x7f;
}

/*********************************************************************
 *      _logbf (MSVCRT.@)
 *
 * Copied from musl: src/math/logbf.c
 */
float CDECL _logbf(float x)
{
    if (!isfinite(x))
        return x * x;
    if (x == 0) {
        *_errno() = ERANGE;
        return -1 / (x * x);
    }
    return __ilogbf(x);
}

#endif

/* Copied from musl: src/math/scalbn.c */
static double __scalbn(double x, int n)
{
    union {double f; UINT64 i;} u;
    double y = x;

    if (n > 1023) {
        y *= 0x1p1023;
        n -= 1023;
        if (n > 1023) {
            y *= 0x1p1023;
            n -= 1023;
            if (n > 1023)
                n = 1023;
        }
    } else if (n < -1022) {
        /* make sure final n < -53 to avoid double
           rounding in the subnormal range */
        y *= 0x1p-1022 * 0x1p53;
        n += 1022 - 53;
        if (n < -1022) {
            y *= 0x1p-1022 * 0x1p53;
            n += 1022 - 53;
            if (n < -1022)
                n = -1022;
        }
    }
    u.i = (UINT64)(0x3ff + n) << 52;
    x = y * u.f;
    return x;
}

/* Copied from musl: src/math/__rem_pio2_large.c */
static int __rem_pio2_large(double *x, double *y, int e0, int nx, int prec)
{
    static const int init_jk[] = {3, 4};
    static const INT32 ipio2[] = {
        0xA2F983, 0x6E4E44, 0x1529FC, 0x2757D1, 0xF534DD, 0xC0DB62,
        0x95993C, 0x439041, 0xFE5163, 0xABDEBB, 0xC561B7, 0x246E3A,
        0x424DD2, 0xE00649, 0x2EEA09, 0xD1921C, 0xFE1DEB, 0x1CB129,
        0xA73EE8, 0x8235F5, 0x2EBB44, 0x84E99C, 0x7026B4, 0x5F7E41,
        0x3991D6, 0x398353, 0x39F49C, 0x845F8B, 0xBDF928, 0x3B1FF8,
        0x97FFDE, 0x05980F, 0xEF2F11, 0x8B5A0A, 0x6D1F6D, 0x367ECF,
        0x27CB09, 0xB74F46, 0x3F669E, 0x5FEA2D, 0x7527BA, 0xC7EBE5,
        0xF17B3D, 0x0739F7, 0x8A5292, 0xEA6BFB, 0x5FB11F, 0x8D5D08,
        0x560330, 0x46FC7B, 0x6BABF0, 0xCFBC20, 0x9AF436, 0x1DA9E3,
        0x91615E, 0xE61B08, 0x659985, 0x5F14A0, 0x68408D, 0xFFD880,
        0x4D7327, 0x310606, 0x1556CA, 0x73A8C9, 0x60E27B, 0xC08C6B,
    };
    static const double PIo2[] = {
        1.57079625129699707031e+00,
        7.54978941586159635335e-08,
        5.39030252995776476554e-15,
        3.28200341580791294123e-22,
        1.27065575308067607349e-29,
        1.22933308981111328932e-36,
        2.73370053816464559624e-44,
        2.16741683877804819444e-51,
    };

    INT32 jz, jx, jv, jp, jk, carry, n, iq[20], i, j, k, m, q0, ih;
    double z, fw, f[20], fq[20] = {0}, q[20];

    /* initialize jk*/
    jk = init_jk[prec];
    jp = jk;

    /* determine jx,jv,q0, note that 3>q0 */
    jx = nx - 1;
    jv = (e0 - 3) / 24;
    if(jv < 0) jv = 0;
    q0 = e0 - 24 * (jv + 1);

    /* set up f[0] to f[jx+jk] where f[jx+jk] = ipio2[jv+jk] */
    j = jv - jx;
    m = jx + jk;
    for (i = 0; i <= m; i++, j++)
        f[i] = j < 0 ? 0.0 : (double)ipio2[j];

    /* compute q[0],q[1],...q[jk] */
    for (i = 0; i <= jk; i++) {
        for (j = 0, fw = 0.0; j <= jx; j++)
            fw += x[j] * f[jx + i - j];
        q[i] = fw;
    }

    jz = jk;
recompute:
    /* distill q[] into iq[] reversingly */
    for (i = 0, j = jz, z = q[jz]; j > 0; i++, j--) {
        fw = (double)(INT32)(0x1p-24 * z);
        iq[i] = (INT32)(z - 0x1p24 * fw);
        z = q[j - 1] + fw;
    }

    /* compute n */
    z = __scalbn(z, q0); /* actual value of z */
    z -= 8.0 * floor(z * 0.125); /* trim off integer >= 8 */
    n = (INT32)z;
    z -= (double)n;
    ih = 0;
    if (q0 > 0) {  /* need iq[jz-1] to determine n */
        i = iq[jz - 1] >> (24 - q0);
        n += i;
        iq[jz - 1] -= i << (24 - q0);
        ih = iq[jz - 1] >> (23 - q0);
    }
    else if (q0 == 0) ih = iq[jz - 1] >> 23;
    else if (z >= 0.5) ih = 2;

    if (ih > 0) {  /* q > 0.5 */
        n += 1;
        carry = 0;
        for (i = 0; i < jz; i++) {  /* compute 1-q */
            j = iq[i];
            if (carry == 0) {
                if (j != 0) {
                    carry = 1;
                    iq[i] = 0x1000000 - j;
                }
            } else
                iq[i] = 0xffffff - j;
        }
        if (q0 > 0) {  /* rare case: chance is 1 in 12 */
            switch(q0) {
            case 1:
                iq[jz - 1] &= 0x7fffff;
                break;
            case 2:
                iq[jz - 1] &= 0x3fffff;
                break;
            }
        }
        if (ih == 2) {
            z = 1.0 - z;
            if (carry != 0)
                z -= __scalbn(1.0, q0);
        }
    }

    /* check if recomputation is needed */
    if (z == 0.0) {
        j = 0;
        for (i = jz - 1; i >= jk; i--) j |= iq[i];
        if (j == 0) {  /* need recomputation */
            for (k = 1; iq[jk - k] == 0; k++);  /* k = no. of terms needed */

            for (i = jz + 1; i <= jz + k; i++) {  /* add q[jz+1] to q[jz+k] */
                f[jx + i] = (double)ipio2[jv + i];
                for (j = 0, fw = 0.0; j <= jx; j++)
                    fw += x[j] * f[jx + i - j];
                q[i] = fw;
            }
            jz += k;
            goto recompute;
        }
    }

    /* chop off zero terms */
    if (z == 0.0) {
        jz -= 1;
        q0 -= 24;
        while (iq[jz] == 0) {
            jz--;
            q0 -= 24;
        }
    } else { /* break z into 24-bit if necessary */
        z = __scalbn(z, -q0);
        if (z >= 0x1p24) {
            fw = (double)(INT32)(0x1p-24 * z);
            iq[jz] = (INT32)(z - 0x1p24 * fw);
            jz += 1;
            q0 += 24;
            iq[jz] = (INT32)fw;
        } else
            iq[jz] = (INT32)z;
    }

    /* convert integer "bit" chunk to floating-point value */
    fw = __scalbn(1.0, q0);
    for (i = jz; i >= 0; i--) {
        q[i] = fw * (double)iq[i];
        fw *= 0x1p-24;
    }

    /* compute PIo2[0,...,jp]*q[jz,...,0] */
    for(i = jz; i >= 0; i--) {
        for (fw = 0.0, k = 0; k <= jp && k <= jz - i; k++)
            fw += PIo2[k] * q[i + k];
        fq[jz - i] = fw;
    }

    /* compress fq[] into y[] */
    switch(prec) {
    case 0:
        fw = 0.0;
        for (i = jz; i >= 0; i--)
            fw += fq[i];
        y[0] = ih == 0 ? fw : -fw;
        break;
    case 1:
    case 2:
        fw = 0.0;
        for (i = jz; i >= 0; i--)
            fw += fq[i];
        fw = (double)fw;
        y[0] = ih==0 ? fw : -fw;
        fw = fq[0] - fw;
        for (i = 1; i <= jz; i++)
            fw += fq[i];
        y[1] = ih == 0 ? fw : -fw;
        break;
    case 3:  /* painful */
        for (i = jz; i > 0; i--) {
            fw = fq[i - 1] + fq[i];
            fq[i] += fq[i - 1] - fw;
            fq[i - 1] = fw;
        }
        for (i = jz; i > 1; i--) {
            fw = fq[i - 1] + fq[i];
            fq[i] += fq[i - 1] - fw;
            fq[i - 1] = fw;
        }
        for (fw = 0.0, i = jz; i >= 2; i--)
            fw += fq[i];
        if (ih == 0) {
            y[0] = fq[0];
            y[1] = fq[1];
            y[2] = fw;
        } else {
            y[0] = -fq[0];
            y[1] = -fq[1];
            y[2] = -fw;
        }
    }
    return n & 7;
}

/* Based on musl implementation: src/math/round.c */
static double __round(double x)
{
    ULONGLONG llx = *(ULONGLONG*)&x, tmp;
    int e = (llx >> 52 & 0x7ff) - 0x3ff;

    if (e >= 52)
        return x;
    if (e < -1)
        return 0 * x;
    else if (e == -1)
        return signbit(x) ? -1 : 1;

    tmp = 0x000fffffffffffffULL >> e;
    if (!(llx & tmp))
        return x;
    llx += 0x0008000000000000ULL >> e;
    llx &= ~tmp;
    return *(double*)&llx;
}

#if !defined(__i386__) || _MSVCR_VER >= 120
/* Copied from musl: src/math/expm1f.c */
static float __expm1f(float x)
{
    static const float ln2_hi = 6.9313812256e-01,
        ln2_lo = 9.0580006145e-06,
        invln2 = 1.4426950216e+00,
        Q1 = -3.3333212137e-2,
        Q2 = 1.5807170421e-3;

    float y, hi, lo, c, t, e, hxs, hfx, r1, twopk;
    union {float f; UINT32 i;} u = {x};
    UINT32 hx = u.i & 0x7fffffff;
    int k, sign = u.i >> 31;

    /* filter out huge and non-finite argument */
    if (hx >= 0x4195b844) { /* if |x|>=27*ln2 */
        if (hx >= 0x7f800000) /* NaN */
            return u.i == 0xff800000 ? -1 : x;
        if (sign)
            return math_error(_UNDERFLOW, "exp", x, 0, -1);
        if (hx > 0x42b17217) /* x > log(FLT_MAX) */
            return math_error(_OVERFLOW, "exp", x, 0, fp_barrierf(x * FLT_MAX));
    }

    /* argument reduction */
    if (hx > 0x3eb17218) { /* if |x| > 0.5 ln2 */
        if (hx < 0x3F851592) { /* and |x| < 1.5 ln2 */
            if (!sign) {
                hi = x - ln2_hi;
                lo = ln2_lo;
                k = 1;
            } else {
                hi = x + ln2_hi;
                lo = -ln2_lo;
                k = -1;
            }
        } else {
            k = invln2 * x + (sign ? -0.5f : 0.5f);
            t = k;
            hi = x - t * ln2_hi; /* t*ln2_hi is exact here */
            lo = t * ln2_lo;
        }
        x = hi - lo;
        c = (hi - x) - lo;
    } else if (hx < 0x33000000) { /* when |x|<2**-25, return x */
        if (hx < 0x00800000)
            fp_barrierf(x * x);
        return x;
    } else
        k = 0;

    /* x is now in primary range */
    hfx = 0.5f * x;
    hxs = x * hfx;
    r1 = 1.0f + hxs * (Q1 + hxs * Q2);
    t = 3.0f - r1 * hfx;
    e = hxs * ((r1 - t) / (6.0f - x * t));
    if (k == 0) /* c is 0 */
        return x - (x * e - hxs);
    e = x * (e - c) - c;
    e -= hxs;
    /* exp(x) ~ 2^k (x_reduced - e + 1) */
    if (k == -1)
        return 0.5f * (x - e) - 0.5f;
    if (k == 1) {
        if (x < -0.25f)
            return -2.0f * (e - (x + 0.5f));
        return 1.0f + 2.0f * (x - e);
    }
    u.i = (0x7f + k) << 23; /* 2^k */
    twopk = u.f;
    if (k < 0 || k > 56) { /* suffice to return exp(x)-1 */
        y = x - e + 1.0f;
        if (k == 128)
            y = y * 2.0f * 0x1p127f;
        else
            y = y * twopk;
        return y - 1.0f;
    }
    u.i = (0x7f-k) << 23; /* 2^-k */
    if (k < 23)
        y = (x - e + (1 - u.f)) * twopk;
    else
        y = (x - (e + u.f) + 1) * twopk;
    return y;
}

/* Copied from musl: src/math/__sindf.c */
static float __sindf(double x)
{
    static const double S1 = -0x1.5555555555555p-3,
        S2 = 0x1.1111111111111p-7,
        S3 = -0x1.a01a01a01a01ap-13,
        S4 = 0x1.71de3a556c734p-19;

    double r, s, w, z;

    z = x * x;
    if (x > -7.8175831586122513e-03 && x < 7.8175831586122513e-03)
        return x * (1 + S1 * z);

    w = z * z;
    r = S3 + z * S4;
    s = z * x;
    return (x + s * (S1 + z * S2)) + s * w * r;
}

/* Copied from musl: src/math/__cosdf.c */
static float __cosdf(double x)
{
    static const double C0 = -0x1.0000000000000p-1,
        C1 = 0x1.5555555555555p-5,
        C2 = -0x1.6c16c16c16c17p-10,
        C3 = 0x1.a01a01a01a01ap-16,
        C4 = -0x1.27e4fb7789f5cp-22;
    double z;

    z = x * x;
    if (x > -7.8163146972656250e-03 && x < 7.8163146972656250e-03)
        return 1 + C0 * z;
    return 1.0 + z * (C0 + z * (C1 + z * (C2 + z * (C3 + z * C4))));
}

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
 *              atan2f (MSVCRT.@)
 *
 * Copied from musl: src/math/atan2f.c
 */
float CDECL atan2f( float y, float x )
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

/* Copied from musl: src/math/__rem_pio2f.c */
static int __rem_pio2f(float x, double *y)
{
    static const double toint = 1.5 / DBL_EPSILON,
        pio4 = 0x1.921fb6p-1,
        invpio2 = 6.36619772367581382433e-01,
        pio2_1 = 1.57079631090164184570e+00,
        pio2_1t = 1.58932547735281966916e-08;

    union {float f; uint32_t i;} u = {x};
    double tx[1], ty[1], fn;
    UINT32 ix;
    int n, sign, e0;

    ix = u.i & 0x7fffffff;
    /* 25+53 bit pi is good enough for medium size */
    if (ix < 0x4dc90fdb) { /* |x| ~< 2^28*(pi/2), medium size */
        /* Use a specialized rint() to get fn. */
        fn = fp_barrier(x * invpio2 + toint) - toint;
        n  = (int)fn;
        *y = x - fn * pio2_1 - fn * pio2_1t;
        /* Matters with directed rounding. */
        if (*y < -pio4) {
            n--;
            fn--;
            *y = x - fn * pio2_1 - fn * pio2_1t;
        } else if (*y > pio4) {
            n++;
            fn++;
            *y = x - fn * pio2_1 - fn * pio2_1t;
        }
        return n;
    }
    if(ix >= 0x7f800000) { /* x is inf or NaN */
        *y = x - x;
        return 0;
    }
    /* scale x into [2^23, 2^24-1] */
    sign = u.i >> 31;
    e0 = (ix >> 23) - (0x7f + 23); /* e0 = ilogb(|x|)-23, positive */
    u.i = ix - (e0 << 23);
    tx[0] = u.f;
    n = __rem_pio2_large(tx, ty, e0, 1, 0);
    if (sign) {
        *y = -ty[0];
        return -n;
    }
    *y = ty[0];
    return n;
}

/*********************************************************************
 *      cosf (MSVCRT.@)
 *
 * Copied from musl: src/math/cosf.c
 */
float CDECL cosf( float x )
{
    static const double c1pio2 = 1*M_PI_2,
        c2pio2 = 2*M_PI_2,
        c3pio2 = 3*M_PI_2,
        c4pio2 = 4*M_PI_2;

    double y;
    UINT32 ix;
    unsigned n, sign;

    ix = *(UINT32*)&x;
    sign = ix >> 31;
    ix &= 0x7fffffff;

    if (ix <= 0x3f490fda) { /* |x| ~<= pi/4 */
        if (ix < 0x39800000) { /* |x| < 2**-12 */
            /* raise inexact if x != 0 */
            fp_barrierf(x + 0x1p120f);
            return 1.0f;
        }
        return __cosdf(x);
    }
    if (ix <= 0x407b53d1) { /* |x| ~<= 5*pi/4 */
        if (ix > 0x4016cbe3) /* |x| ~> 3*pi/4 */
            return -__cosdf(sign ? x + c2pio2 : x - c2pio2);
        else {
            if (sign)
                return __sindf(x + c1pio2);
            else
                return __sindf(c1pio2 - x);
        }
    }
    if (ix <= 0x40e231d5) { /* |x| ~<= 9*pi/4 */
        if (ix > 0x40afeddf) /* |x| ~> 7*pi/4 */
            return __cosdf(sign ? x + c4pio2 : x - c4pio2);
        else {
            if (sign)
                return __sindf(-x - c3pio2);
            else
                return __sindf(x - c3pio2);
        }
    }

    /* cos(Inf or NaN) is NaN */
    if (isinf(x)) return math_error(_DOMAIN, "cosf", x, 0, x - x);
    if (ix >= 0x7f800000)
        return x - x;

    /* general argument reduction needed */
    n = __rem_pio2f(x, &y);
    switch (n & 3) {
    case 0: return __cosdf(y);
    case 1: return __sindf(-y);
    case 2: return -__cosdf(y);
    default: return __sindf(y);
    }
}

/* Copied from musl: src/math/__expo2f.c */
static float __expo2f(float x, float sign)
{
    static const int k = 235;
    static const float kln2 = 0x1.45c778p+7f;
    float scale;

    *(UINT32*)&scale = (UINT32)(0x7f + k/2) << 23;
    return expf(x - kln2) * (sign * scale) * scale;
}

/*********************************************************************
 *      coshf (MSVCRT.@)
 *
 * Copied from musl: src/math/coshf.c
 */
float CDECL coshf( float x )
{
    UINT32 ui = *(UINT32*)&x;
    UINT32 sign = ui & 0x80000000;
    float t;

    /* |x| */
    ui &= 0x7fffffff;
    x = *(float*)&ui;

    /* |x| < log(2) */
    if (ui < 0x3f317217) {
        if (ui < 0x3f800000 - (12 << 23)) {
            fp_barrierf(x + 0x1p120f);
            return 1;
        }
        t = __expm1f(x);
        return 1 + t * t / (2 * (1 + t));
    }

    /* |x| < log(FLT_MAX) */
    if (ui < 0x42b17217) {
        t = expf(x);
        return 0.5f * (t + 1 / t);
    }

    /* |x| > log(FLT_MAX) or nan */
    if (ui > 0x7f800000)
        *(UINT32*)&t = ui | sign | 0x400000;
    else
        t = __expo2f(x, 1.0f);
    return t;
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
    kd = __round(z);
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

/*********************************************************************
 *      fmodf (MSVCRT.@)
 *
 * Copied from musl: src/math/fmodf.c
 */
float CDECL fmodf( float x, float y )
{
    UINT32 xi = *(UINT32*)&x;
    UINT32 yi = *(UINT32*)&y;
    int ex = xi>>23 & 0xff;
    int ey = yi>>23 & 0xff;
    UINT32 sx = xi & 0x80000000;
    UINT32 i;

    if (isinf(x)) return math_error(_DOMAIN, "fmodf", x, y, (x * y) / (x * y));
    if (yi << 1 == 0 || isnan(y) || ex == 0xff)
        return (x * y) / (x * y);
    if (xi << 1 <= yi << 1) {
        if (xi << 1 == yi << 1)
            return 0 * x;
        return x;
    }

    /* normalize x and y */
    if (!ex) {
        for (i = xi << 9; i >> 31 == 0; ex--, i <<= 1);
        xi <<= -ex + 1;
    } else {
        xi &= -1U >> 9;
        xi |= 1U << 23;
    }
    if (!ey) {
        for (i = yi << 9; i >> 31 == 0; ey--, i <<= 1);
        yi <<= -ey + 1;
    } else {
        yi &= -1U >> 9;
        yi |= 1U << 23;
    }

    /* x mod y */
    for (; ex > ey; ex--) {
        i = xi - yi;
        if (i >> 31 == 0) {
            if (i == 0)
                return 0 * x;
            xi = i;
        }
        xi <<= 1;
    }
    i = xi - yi;
    if (i >> 31 == 0) {
        if (i == 0)
            return 0 * x;
        xi = i;
    }
    for (; xi>>23 == 0; xi <<= 1, ex--);

    /* scale result up */
    if (ex > 0) {
        xi -= 1U << 23;
        xi |= (UINT32)ex << 23;
    } else {
        xi >>= -ex + 1;
    }
    xi |= sx;
    return *(float*)&xi;
}

/*********************************************************************
 *      logf (MSVCRT.@)
 *
 * Copied from musl: src/math/logf.c src/math/logf_data.c
 */
float CDECL logf( float x )
{
    static const double Ln2 = 0x1.62e42fefa39efp-1;
    static const double A[] = {
        -0x1.00ea348b88334p-2,
        0x1.5575b0be00b6ap-2,
        -0x1.ffffef20a4123p-2
    };
    static const struct {
        double invc, logc;
    } T[] = {
        { 0x1.661ec79f8f3bep+0, -0x1.57bf7808caadep-2 },
        { 0x1.571ed4aaf883dp+0, -0x1.2bef0a7c06ddbp-2 },
        { 0x1.49539f0f010bp+0, -0x1.01eae7f513a67p-2 },
        { 0x1.3c995b0b80385p+0, -0x1.b31d8a68224e9p-3 },
        { 0x1.30d190c8864a5p+0, -0x1.6574f0ac07758p-3 },
        { 0x1.25e227b0b8eap+0, -0x1.1aa2bc79c81p-3 },
        { 0x1.1bb4a4a1a343fp+0, -0x1.a4e76ce8c0e5ep-4 },
        { 0x1.12358f08ae5bap+0, -0x1.1973c5a611cccp-4 },
        { 0x1.0953f419900a7p+0, -0x1.252f438e10c1ep-5 },
        { 0x1p+0, 0x0p+0 },
        { 0x1.e608cfd9a47acp-1, 0x1.aa5aa5df25984p-5 },
        { 0x1.ca4b31f026aap-1, 0x1.c5e53aa362eb4p-4 },
        { 0x1.b2036576afce6p-1, 0x1.526e57720db08p-3 },
        { 0x1.9c2d163a1aa2dp-1, 0x1.bc2860d22477p-3 },
        { 0x1.886e6037841edp-1, 0x1.1058bc8a07ee1p-2 },
        { 0x1.767dcf5534862p-1, 0x1.4043057b6ee09p-2 }
    };

    double z, r, r2, y, y0, invc, logc;
    UINT32 ix, iz, tmp;
    int k, i;

    ix = *(UINT32*)&x;
    /* Fix sign of zero with downward rounding when x==1. */
    if (ix == 0x3f800000)
        return 0;
    if (ix - 0x00800000 >= 0x7f800000 - 0x00800000) {
        /* x < 0x1p-126 or inf or nan. */
        if (ix * 2 == 0)
            return math_error(_SING, "logf", x, 0, (ix & 0x80000000 ? 1.0 : -1.0) / x);
        if (ix == 0x7f800000) /* log(inf) == inf. */
            return x;
        if (ix * 2 > 0xff000000)
            return x;
        if (ix & 0x80000000)
            return math_error(_DOMAIN, "logf", x, 0, (x - x) / (x - x));
        /* x is subnormal, normalize it. */
        x *= 0x1p23f;
        ix = *(UINT32*)&x;
        ix -= 23 << 23;
    }

    /* x = 2^k z; where z is in range [OFF,2*OFF] and exact.
       The range is split into N subintervals.
       The ith subinterval contains z and c is near its center. */
    tmp = ix - 0x3f330000;
    i = (tmp >> (23 - 4)) % (1 << 4);
    k = (INT32)tmp >> 23; /* arithmetic shift */
    iz = ix - (tmp & (0x1ffu << 23));
    invc = T[i].invc;
    logc = T[i].logc;
    z = *(float*)&iz;

    /* log(x) = log1p(z/c-1) + log(c) + k*Ln2 */
    r = z * invc - 1;
    y0 = logc + (double)k * Ln2;

    /* Pipelined polynomial evaluation to approximate log1p(r). */
    r2 = r * r;
    y = A[1] * r + A[2];
    y = A[0] * r2 + y;
    y = y * r2 + (y0 + r);
    return y;
}

/*********************************************************************
 *      log10f (MSVCRT.@)
 */
float CDECL log10f( float x )
{
    static const float ivln10hi = 4.3432617188e-01,
        ivln10lo = -3.1689971365e-05,
        log10_2hi = 3.0102920532e-01,
        log10_2lo = 7.9034151668e-07,
        Lg1 = 0xaaaaaa.0p-24,
        Lg2 = 0xccce13.0p-25,
        Lg3 = 0x91e9ee.0p-25,
        Lg4 = 0xf89e26.0p-26;

    union {float f; UINT32 i;} u = {x};
    float hfsq, f, s, z, R, w, t1, t2, dk, hi, lo;
    UINT32 ix;
    int k;

    ix = u.i;
    k = 0;
    if (ix < 0x00800000 || ix >> 31) { /* x < 2**-126 */
        if (ix << 1 == 0)
            return math_error(_SING, "log10f", x, 0, -1 / (x * x));
        if ((ix & ~(1u << 31)) > 0x7f800000)
            return x;
        if (ix >> 31)
            return math_error(_DOMAIN, "log10f", x, 0, (x - x) / (x - x));
        /* subnormal number, scale up x */
        k -= 25;
        x *= 0x1p25f;
        u.f = x;
        ix = u.i;
    } else if (ix >= 0x7f800000) {
        return x;
    } else if (ix == 0x3f800000)
        return 0;

    /* reduce x into [sqrt(2)/2, sqrt(2)] */
    ix += 0x3f800000 - 0x3f3504f3;
    k += (int)(ix >> 23) - 0x7f;
    ix = (ix & 0x007fffff) + 0x3f3504f3;
    u.i = ix;
    x = u.f;

    f = x - 1.0f;
    s = f / (2.0f + f);
    z = s * s;
    w = z * z;
    t1= w * (Lg2 + w * Lg4);
    t2= z * (Lg1 + w * Lg3);
    R = t2 + t1;
    hfsq = 0.5f * f * f;

    hi = f - hfsq;
    u.f = hi;
    u.i &= 0xfffff000;
    hi = u.f;
    lo = f - hi - hfsq + s * (hfsq + R);
    dk = k;
    return dk * log10_2lo + (lo + hi) * ivln10lo + lo * ivln10hi + hi * ivln10hi + dk * log10_2hi;
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
    kd = __round(xd); /* k */
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

/*********************************************************************
 *      sinf (MSVCRT.@)
 *
 * Copied from musl: src/math/sinf.c
 */
float CDECL sinf( float x )
{
    static const double s1pio2 = 1*M_PI_2,
        s2pio2 = 2*M_PI_2,
        s3pio2 = 3*M_PI_2,
        s4pio2 = 4*M_PI_2;

    double y;
    UINT32 ix;
    int n, sign;

    ix = *(UINT32*)&x;
    sign = ix >> 31;
    ix &= 0x7fffffff;

    if (ix <= 0x3f490fda) { /* |x| ~<= pi/4 */
        if (ix < 0x39800000) { /* |x| < 2**-12 */
            /* raise inexact if x!=0 and underflow if subnormal */
            fp_barrierf(ix < 0x00800000 ? x / 0x1p120f : x + 0x1p120f);
            return x;
        }
        return __sindf(x);
    }
    if (ix <= 0x407b53d1) { /* |x| ~<= 5*pi/4 */
        if (ix <= 0x4016cbe3) { /* |x| ~<= 3pi/4 */
            if (sign)
                return -__cosdf(x + s1pio2);
            else
                return __cosdf(x - s1pio2);
        }
        return __sindf(sign ? -(x + s2pio2) : -(x - s2pio2));
    }
    if (ix <= 0x40e231d5) { /* |x| ~<= 9*pi/4 */
        if (ix <= 0x40afeddf) { /* |x| ~<= 7*pi/4 */
            if (sign)
                return __cosdf(x + s3pio2);
            else
                return -__cosdf(x - s3pio2);
        }
        return __sindf(sign ? x + s4pio2 : x - s4pio2);
    }

    /* sin(Inf or NaN) is NaN */
    if (isinf(x))
        return math_error(_DOMAIN, "sinf", x, 0, x - x);
    if (ix >= 0x7f800000)
        return x - x;

    /* general argument reduction needed */
    n = __rem_pio2f(x, &y);
    switch (n&3) {
    case 0: return __sindf(y);
    case 1: return __cosdf(y);
    case 2: return __sindf(-y);
    default: return -__cosdf(y);
    }
}

/*********************************************************************
 *      sinhf (MSVCRT.@)
 */
float CDECL sinhf( float x )
{
    UINT32 ui = *(UINT32*)&x;
    float t, h, absx;

    h = 0.5;
    if (ui >> 31)
        h = -h;
    /* |x| */
    ui &= 0x7fffffff;
    absx = *(float*)&ui;

    /* |x| < log(FLT_MAX) */
    if (ui < 0x42b17217) {
        t = __expm1f(absx);
        if (ui < 0x3f800000) {
            if (ui < 0x3f800000 - (12 << 23))
                return x;
            return h * (2 * t - t * t / (t + 1));
        }
        return h * (t + t / (t + 1));
    }

    /* |x| > logf(FLT_MAX) or nan */
    if (ui > 0x7f800000)
        *(DWORD*)&t = *(DWORD*)&x | 0x400000;
    else
        t = __expo2f(absx, 2 * h);
    return t;
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

/* Copied from musl: src/math/__tandf.c */
static float __tandf(double x, int odd)
{
    static const double T[] = {
        0x15554d3418c99f.0p-54,
        0x1112fd38999f72.0p-55,
        0x1b54c91d865afe.0p-57,
        0x191df3908c33ce.0p-58,
        0x185dadfcecf44e.0p-61,
        0x1362b9bf971bcd.0p-59,
    };

    double z, r, w, s, t, u;

    z = x * x;
    r = T[4] + z * T[5];
    t = T[2] + z * T[3];
    w = z * z;
    s = z * x;
    u = T[0] + z * T[1];
    r = (x + s * u) + (s * w) * (t + w * r);
    return odd ? -1.0 / r : r;
}

/*********************************************************************
 *      tanf (MSVCRT.@)
 *
 * Copied from musl: src/math/tanf.c
 */
float CDECL tanf( float x )
{
    static const double t1pio2 = 1*M_PI_2,
        t2pio2 = 2*M_PI_2,
        t3pio2 = 3*M_PI_2,
        t4pio2 = 4*M_PI_2;

    double y;
    UINT32 ix;
    unsigned n, sign;

    ix = *(UINT32*)&x;
    sign = ix >> 31;
    ix &= 0x7fffffff;

    if (ix <= 0x3f490fda) { /* |x| ~<= pi/4 */
        if (ix < 0x39800000) { /* |x| < 2**-12 */
            /* raise inexact if x!=0 and underflow if subnormal */
            fp_barrierf(ix < 0x00800000 ? x / 0x1p120f : x + 0x1p120f);
            return x;
        }
        return __tandf(x, 0);
    }
    if (ix <= 0x407b53d1) { /* |x| ~<= 5*pi/4 */
        if (ix <= 0x4016cbe3) /* |x| ~<= 3pi/4 */
            return __tandf((sign ? x + t1pio2 : x - t1pio2), 1);
        else
            return __tandf((sign ? x + t2pio2 : x - t2pio2), 0);
    }
    if (ix <= 0x40e231d5) { /* |x| ~<= 9*pi/4 */
        if (ix <= 0x40afeddf) /* |x| ~<= 7*pi/4 */
            return __tandf((sign ? x + t3pio2 : x - t3pio2), 1);
        else
            return __tandf((sign ? x + t4pio2 : x - t4pio2), 0);
    }

    /* tan(Inf or NaN) is NaN */
    if (isinf(x))
        return math_error(_DOMAIN, "tanf", x, 0, x - x);
    if (ix >= 0x7f800000)
        return x - x;

    /* argument reduction */
    n = __rem_pio2f(x, &y);
    return __tandf(y, n & 1);
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
            t = __expm1f(2 * x);
            t = 1 - 2 / (t + 2);
        }
    } else if (ui > 0x3e82c578) {
        /* |x| > log(5/3)/2 ~= 0.2554 */
        t = __expm1f(2 * x);
        t = t / (t + 2);
    } else if (ui >= 0x00800000) {
        /* |x| >= 0x1p-126 */
        t = __expm1f(-2 * x);
        t = -t / (t + 2);
    } else {
        /* |x| is subnormal */
        fp_barrierf(x * x);
        t = x;
    }
    return sign ? -t : t;
}

/*********************************************************************
 *      ceilf (MSVCRT.@)
 *
 * Copied from musl: src/math/ceilf.c
 */
float CDECL ceilf( float x )
{
    union {float f; UINT32 i;} u = {x};
    int e = (int)(u.i >> 23 & 0xff) - 0x7f;
    UINT32 m;

    if (e >= 23)
        return x;
    if (e >= 0) {
        m = 0x007fffff >> e;
        if ((u.i & m) == 0)
            return x;
        if (u.i >> 31 == 0)
            u.i += m;
        u.i &= ~m;
    } else {
        if (u.i >> 31)
            return -0.0;
        else if (u.i << 1)
            return 1.0;
    }
    return u.f;
}

/*********************************************************************
 *      floorf (MSVCRT.@)
 *
 * Copied from musl: src/math/floorf.c
 */
float CDECL floorf( float x )
{
    union {float f; UINT32 i;} u = {x};
    int e = (int)(u.i >> 23 & 0xff) - 0x7f;
    UINT32 m;

    if (e >= 23)
        return x;
    if (e >= 0) {
        m = 0x007fffff >> e;
        if ((u.i & m) == 0)
            return x;
        if (u.i >> 31)
            u.i += m;
        u.i &= ~m;
    } else {
        if (u.i >> 31 == 0)
            return 0;
        else if (u.i << 1)
            return -1;
    }
    return u.f;
}

/*********************************************************************
 *      frexpf (MSVCRT.@)
 *
 * Copied from musl: src/math/frexpf.c
 */
float CDECL frexpf( float x, int *e )
{
    UINT32 ux = *(UINT32*)&x;
    int ee = ux >> 23 & 0xff;

    if (!ee) {
        if (x) {
            x = frexpf(x * 0x1p64, e);
            *e -= 64;
        } else *e = 0;
        return x;
    } else if (ee == 0xff) {
        return x;
    }

    *e = ee - 0x7e;
    ux &= 0x807ffffful;
    ux |= 0x3f000000ul;
    return *(float*)&ux;
}

/*********************************************************************
 *      modff (MSVCRT.@)
 *
 * Copied from musl: src/math/modff.c
 */
float CDECL modff( float x, float *iptr )
{
    union {float f; UINT32 i;} u = {x};
    UINT32 mask;
    int e = (u.i >> 23 & 0xff) - 0x7f;

    /* no fractional part */
    if (e >= 23) {
        *iptr = x;
        if (e == 0x80 && u.i << 9 != 0) { /* nan */
            return x;
        }
        u.i &= 0x80000000;
        return u.f;
    }
    /* no integral part */
    if (e < 0) {
        u.i &= 0x80000000;
        *iptr = u.f;
        return x;
    }

    mask = 0x007fffff >> e;
    if ((u.i & mask) == 0) {
        *iptr = x;
        u.i &= 0x80000000;
        return u.f;
    }
    u.i &= ~mask;
    *iptr = u.f;
    return x - u.f;
}

#endif

#if !defined(__i386__) && !defined(__x86_64__) && (_MSVCR_VER == 0 || _MSVCR_VER >= 110)

/*********************************************************************
 *      fabsf (MSVCRT.@)
 *
 * Copied from musl: src/math/fabsf.c
 */
float CDECL fabsf( float x )
{
    union { float f; UINT32 i; } u = { x };
    u.i &= 0x7fffffff;
    return u.f;
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
 *		atan2 (MSVCRT.@)
 *
 * Copied from musl: src/math/atan2.c
 */
double CDECL atan2( double y, double x )
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

/* Copied from musl: src/math/rint.c */
static double __rint(double x)
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

/* Copied from musl: src/math/__rem_pio2.c */
static int __rem_pio2(double x, double *y)
{
    static const double pio4    = 0x1.921fb54442d18p-1,
                 invpio2 = 6.36619772367581382433e-01,
                 pio2_1  = 1.57079632673412561417e+00,
                 pio2_1t = 6.07710050650619224932e-11,
                 pio2_2  = 6.07710050630396597660e-11,
                 pio2_2t = 2.02226624879595063154e-21,
                 pio2_3  = 2.02226624871116645580e-21,
                 pio2_3t = 8.47842766036889956997e-32;

    union {double f; UINT64 i;} u = {x};
    double z, w, t, r, fn, tx[3], ty[2];
    UINT32 ix;
    int sign, n, ex, ey, i;

    sign = u.i >> 63;
    ix = u.i >> 32 & 0x7fffffff;
    if (ix <= 0x400f6a7a) { /* |x| ~<= 5pi/4 */
        if ((ix & 0xfffff) == 0x921fb) /* |x| ~= pi/2 or 2pi/2 */
            goto medium; /* cancellation -- use medium case */
        if (ix <= 0x4002d97c) { /* |x| ~<= 3pi/4 */
            if (!sign) {
                z = x - pio2_1; /* one round good to 85 bits */
                y[0] = z - pio2_1t;
                y[1] = (z - y[0]) - pio2_1t;
                return 1;
            } else {
                z = x + pio2_1;
                y[0] = z + pio2_1t;
                y[1] = (z - y[0]) + pio2_1t;
                return -1;
            }
        } else {
            if (!sign) {
                z = x - 2 * pio2_1;
                y[0] = z - 2 * pio2_1t;
                y[1] = (z - y[0]) - 2 * pio2_1t;
                return 2;
            } else {
                z = x + 2 * pio2_1;
                y[0] = z + 2 * pio2_1t;
                y[1] = (z - y[0]) + 2 * pio2_1t;
                return -2;
            }
        }
    }
    if (ix <= 0x401c463b) { /* |x| ~<= 9pi/4 */
        if (ix <= 0x4015fdbc) { /* |x| ~<= 7pi/4 */
            if (ix == 0x4012d97c) /* |x| ~= 3pi/2 */
                goto medium;
            if (!sign) {
                z = x - 3 * pio2_1;
                y[0] = z - 3 * pio2_1t;
                y[1] = (z - y[0]) - 3 * pio2_1t;
                return 3;
            } else {
                z = x + 3 * pio2_1;
                y[0] = z + 3 * pio2_1t;
                y[1] = (z - y[0]) + 3 * pio2_1t;
                return -3;
            }
        } else {
            if (ix == 0x401921fb) /* |x| ~= 4pi/2 */
                goto medium;
            if (!sign) {
                z = x - 4 * pio2_1;
                y[0] = z - 4 * pio2_1t;
                y[1] = (z - y[0]) - 4 * pio2_1t;
                return 4;
            } else {
                z = x + 4 * pio2_1;
                y[0] = z + 4 * pio2_1t;
                y[1] = (z - y[0]) + 4 * pio2_1t;
                return -4;
            }
        }
    }
    if (ix < 0x413921fb) { /* |x| ~< 2^20*(pi/2), medium size */
medium:
        fn = __rint(x * invpio2);
        n = (INT32)fn;
        r = x - fn * pio2_1;
        w = fn * pio2_1t; /* 1st round, good to 85 bits */
        /* Matters with directed rounding. */
        if (r - w < -pio4) {
            n--;
            fn--;
            r = x - fn * pio2_1;
            w = fn * pio2_1t;
        } else if (r - w > pio4) {
            n++;
            fn++;
            r = x - fn * pio2_1;
            w = fn * pio2_1t;
        }
        y[0] = r - w;
        u.f = y[0];
        ey = u.i >> 52 & 0x7ff;
        ex = ix >> 20;
        if (ex - ey > 16) { /* 2nd round, good to 118 bits */
            t = r;
            w = fn * pio2_2;
            r = t - w;
            w = fn * pio2_2t - ((t - r) - w);
            y[0] = r - w;
            u.f = y[0];
            ey = u.i >> 52 & 0x7ff;
            if (ex - ey > 49) { /* 3rd round, good to 151 bits, covers all cases */
                t = r;
                w = fn * pio2_3;
                r = t - w;
                w = fn * pio2_3t - ((t - r) - w);
                y[0] = r - w;
            }
        }
        y[1] = (r - y[0]) - w;
        return n;
    }
    /*
     * all other (large) arguments
     */
    if (ix >= 0x7ff00000) {  /* x is inf or NaN */
        y[0] = y[1] = x - x;
        return 0;
    }
    /* set z = scalbn(|x|,-ilogb(x)+23) */
    u.f = x;
    u.i &= (UINT64)-1 >> 12;
    u.i |= (UINT64)(0x3ff + 23) << 52;
    z = u.f;
    for (i = 0; i < 2; i++) {
        tx[i] = (double)(INT32)z;
        z = (z - tx[i]) * 0x1p24;
    }
    tx[i] = z;
    /* skip zero terms, first term is non-zero */
    while (tx[i] == 0.0)
        i--;
    n = __rem_pio2_large(tx, ty, (int)(ix >> 20) - (0x3ff + 23), i + 1, 1);
    if (sign) {
        y[0] = -ty[0];
        y[1] = -ty[1];
        return -n;
    }
    y[0] = ty[0];
    y[1] = ty[1];
    return n;
}

/* Copied from musl: src/math/__sin.c */
static double __sin(double x, double y, int iy)
{
    static const double S1  = -1.66666666666666324348e-01,
                 S2  =  8.33333333332248946124e-03,
                 S3  = -1.98412698298579493134e-04,
                 S4  =  2.75573137070700676789e-06,
                 S5  = -2.50507602534068634195e-08,
                 S6  =  1.58969099521155010221e-10;

    double z, r, v, w;

    z = x * x;
    w = z * z;
    r = S2 + z * (S3 + z * S4) + z * w * (S5 + z * S6);
    v = z * x;
    if (iy == 0)
        return x + v * (S1 + z * r);
    else
        return x - ((z * (0.5 * y - v * r) - y) - v * S1);
}

/* Copied from musl: src/math/__cos.c */
static double __cos(double x, double y)
{
    static const double C1  =  4.16666666666666019037e-02,
                 C2  = -1.38888888888741095749e-03,
                 C3  =  2.48015872894767294178e-05,
                 C4  = -2.75573143513906633035e-07,
                 C5  =  2.08757232129817482790e-09,
                 C6  = -1.13596475577881948265e-11;
    double hz, z, r, w;

    z = x * x;
    w = z * z;
    r = z * (C1 + z * (C2 + z * C3)) + w * w * (C4 + z * (C5 + z * C6));
    hz = 0.5 * z;
    w = 1.0 - hz;
    return w + (((1.0 - w) - hz) + (z * r - x * y));
}

/*********************************************************************
 *		cos (MSVCRT.@)
 *
 * Copied from musl: src/math/cos.c
 */
double CDECL cos( double x )
{
    double y[2];
    UINT32 ix;
    unsigned n;

    ix = *(ULONGLONG*)&x >> 32;
    ix &= 0x7fffffff;

    /* |x| ~< pi/4 */
    if (ix <= 0x3fe921fb) {
        if (ix < 0x3e46a09e) { /* |x| < 2**-27 * sqrt(2) */
            /* raise inexact if x!=0 */
            fp_barrier(x + 0x1p120f);
            return 1.0;
        }
        return __cos(x, 0);
    }

    /* cos(Inf or NaN) is NaN */
    if (isinf(x)) return math_error(_DOMAIN, "cos", x, 0, x - x);
    if (ix >= 0x7ff00000)
        return x - x;

    /* argument reduction */
    n = __rem_pio2(x, y);
    switch (n & 3) {
    case 0: return __cos(y[0], y[1]);
    case 1: return -__sin(y[0], y[1], 1);
    case 2: return -__cos(y[0], y[1]);
    default: return __sin(y[0], y[1], 1);
    }
}

/* Copied from musl: src/math/expm1.c */
static double __expm1(double x)
{
    static const double o_threshold = 7.09782712893383973096e+02,
        ln2_hi = 6.93147180369123816490e-01,
        ln2_lo = 1.90821492927058770002e-10,
        invln2 = 1.44269504088896338700e+00,
        Q1 = -3.33333333333331316428e-02,
        Q2 = 1.58730158725481460165e-03,
        Q3 = -7.93650757867487942473e-05,
        Q4 = 4.00821782732936239552e-06,
        Q5 = -2.01099218183624371326e-07;

    double y, hi, lo, c, t, e, hxs, hfx, r1, twopk;
    union {double f; UINT64 i;} u = {x};
    UINT32 hx = u.i >> 32 & 0x7fffffff;
    int k, sign = u.i >> 63;

    /* filter out huge and non-finite argument */
    if (hx >= 0x4043687A) { /* if |x|>=56*ln2 */
        if (isnan(x))
            return x;
        if (isinf(x))
            return sign ? -1 : x;
        if (sign)
            return math_error(_UNDERFLOW, "exp", x, 0, -1);
        if (x > o_threshold)
            return math_error(_OVERFLOW, "exp", x, 0, x * 0x1p1023);
    }

    /* argument reduction */
    if (hx > 0x3fd62e42) { /* if |x| > 0.5 ln2 */
        if (hx < 0x3FF0A2B2) { /* and |x| < 1.5 ln2 */
            if (!sign) {
                hi = x - ln2_hi;
                lo = ln2_lo;
                k = 1;
            } else {
                hi = x + ln2_hi;
                lo = -ln2_lo;
                k = -1;
            }
        } else {
            k = invln2 * x + (sign ? -0.5 : 0.5);
            t = k;
            hi = x - t * ln2_hi; /* t*ln2_hi is exact here */
            lo = t * ln2_lo;
        }
        x = hi - lo;
        c = (hi - x) - lo;
    } else if (hx < 0x3c900000) { /* |x| < 2**-54, return x */
        fp_barrier(x + 0x1p120f);
        if (hx < 0x00100000)
            fp_barrier((float)x);
        return x;
    } else
        k = 0;

    /* x is now in primary range */
    hfx = 0.5 * x;
    hxs = x * hfx;
    r1 = 1.0 + hxs * (Q1 + hxs * (Q2 + hxs * (Q3 + hxs * (Q4 + hxs * Q5))));
    t = 3.0 - r1 * hfx;
    e = hxs * ((r1 - t) / (6.0 - x * t));
    if (k == 0) /* c is 0 */
        return x - (x * e - hxs);
    e = x * (e - c) - c;
    e -= hxs;
    /* exp(x) ~ 2^k (x_reduced - e + 1) */
    if (k == -1)
        return 0.5 * (x - e) - 0.5;
    if (k == 1) {
        if (x < -0.25)
            return -2.0 * (e - (x + 0.5));
        return 1.0 + 2.0 * (x - e);
    }
    u.i = (UINT64)(0x3ff + k) << 52; /* 2^k */
    twopk = u.f;
    if (k < 0 || k > 56) { /* suffice to return exp(x)-1 */
        y = x - e + 1.0;
        if (k == 1024)
            y = y * 2.0 * 0x1p1023;
        else
            y = y * twopk;
        return y - 1.0;
    }
    u.i = (UINT64)(0x3ff - k) << 52; /* 2^-k */
    if (k < 20)
        y = (x - e + (1 - u.f)) * twopk;
    else
        y = (x - (e + u.f) + 1) * twopk;
    return y;
}

static double __expo2(double x, double sign)
{
    static const int k = 2043;
    static const double kln2 = 0x1.62066151add8bp+10;
    double scale;

    *(UINT64*)&scale = (UINT64)(0x3ff + k / 2) << 52;
    return exp(x - kln2) * (sign * scale) * scale;
}

/*********************************************************************
 *		cosh (MSVCRT.@)
 *
 * Copied from musl: src/math/cosh.c
 */
double CDECL cosh( double x )
{
    UINT64 ux = *(UINT64*)&x;
    UINT64 sign = ux & 0x8000000000000000ULL;
    UINT32 w;
    double t;

    /* |x| */
    ux &= (uint64_t)-1 / 2;
    x = *(double*)&ux;
    w = ux >> 32;

    /* |x| < log(2) */
    if (w < 0x3fe62e42) {
        if (w < 0x3ff00000 - (26 << 20)) {
            fp_barrier(x + 0x1p120f);
            return 1;
        }
        t = __expm1(x);
        return 1 + t * t / (2 * (1 + t));
    }

    /* |x| < log(DBL_MAX) */
    if (w < 0x40862e42) {
        t = exp(x);
        /* note: if x>log(0x1p26) then the 1/t is not needed */
        return 0.5 * (t + 1 / t);
    }

    /* |x| > log(DBL_MAX) or nan */
    /* note: the result is stored to handle overflow */
    if (ux > 0x7ff0000000000000ULL)
        *(UINT64*)&t = ux | sign | 0x0008000000000000ULL;
    else
        t = __expo2(x, 1.0);
    return t;
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
    kd = __round(z);
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

/*********************************************************************
 *		fmod (MSVCRT.@)
 *
 * Copied from musl: src/math/fmod.c
 */
double CDECL fmod( double x, double y )
{
    UINT64 xi = *(UINT64*)&x;
    UINT64 yi = *(UINT64*)&y;
    int ex = xi >> 52 & 0x7ff;
    int ey = yi >> 52 & 0x7ff;
    int sx = xi >> 63;
    UINT64 i;

    if (isinf(x)) return math_error(_DOMAIN, "fmod", x, y, (x * y) / (x * y));
    if (yi << 1 == 0 || isnan(y) || ex == 0x7ff)
        return (x * y) / (x * y);
    if (xi << 1 <= yi << 1) {
        if (xi << 1 == yi << 1)
            return 0 * x;
        return x;
    }

    /* normalize x and y */
    if (!ex) {
        for (i = xi << 12; i >> 63 == 0; ex--, i <<= 1);
        xi <<= -ex + 1;
    } else {
        xi &= -1ULL >> 12;
        xi |= 1ULL << 52;
    }
    if (!ey) {
        for (i = yi << 12; i >> 63 == 0; ey--, i <<= 1);
        yi <<= -ey + 1;
    } else {
        yi &= -1ULL >> 12;
        yi |= 1ULL << 52;
    }

    /* x mod y */
    for (; ex > ey; ex--) {
        i = xi - yi;
        if (i >> 63 == 0) {
            if (i == 0)
                return 0 * x;
            xi = i;
        }
        xi <<= 1;
    }
    i = xi - yi;
    if (i >> 63 == 0) {
        if (i == 0)
            return 0 * x;
        xi = i;
    }
    for (; xi >> 52 == 0; xi <<= 1, ex--);

    /* scale result */
    if (ex > 0) {
        xi -= 1ULL << 52;
        xi |= (UINT64)ex << 52;
    } else {
        xi >>= -ex + 1;
    }
    xi |= (UINT64)sx << 63;
    return *(double*)&xi;
}

/*********************************************************************
 *		log (MSVCRT.@)
 *
 * Copied from musl: src/math/log.c src/math/log_data.c
 */
double CDECL log( double x )
{
    static const double Ln2hi = 0x1.62e42fefa3800p-1,
        Ln2lo = 0x1.ef35793c76730p-45;
    static const double A[] = {
        -0x1.0000000000001p-1,
        0x1.555555551305bp-2,
        -0x1.fffffffeb459p-3,
        0x1.999b324f10111p-3,
        -0x1.55575e506c89fp-3
    };
    static const double B[] = {
        -0x1p-1,
        0x1.5555555555577p-2,
        -0x1.ffffffffffdcbp-3,
        0x1.999999995dd0cp-3,
        -0x1.55555556745a7p-3,
        0x1.24924a344de3p-3,
        -0x1.fffffa4423d65p-4,
        0x1.c7184282ad6cap-4,
        -0x1.999eb43b068ffp-4,
        0x1.78182f7afd085p-4,
        -0x1.5521375d145cdp-4
    };
    static const struct {
        double invc, logc;
    } T[] = {
        {0x1.734f0c3e0de9fp+0, -0x1.7cc7f79e69000p-2},
        {0x1.713786a2ce91fp+0, -0x1.76feec20d0000p-2},
        {0x1.6f26008fab5a0p+0, -0x1.713e31351e000p-2},
        {0x1.6d1a61f138c7dp+0, -0x1.6b85b38287800p-2},
        {0x1.6b1490bc5b4d1p+0, -0x1.65d5590807800p-2},
        {0x1.69147332f0cbap+0, -0x1.602d076180000p-2},
        {0x1.6719f18224223p+0, -0x1.5a8ca86909000p-2},
        {0x1.6524f99a51ed9p+0, -0x1.54f4356035000p-2},
        {0x1.63356aa8f24c4p+0, -0x1.4f637c36b4000p-2},
        {0x1.614b36b9ddc14p+0, -0x1.49da7fda85000p-2},
        {0x1.5f66452c65c4cp+0, -0x1.445923989a800p-2},
        {0x1.5d867b5912c4fp+0, -0x1.3edf439b0b800p-2},
        {0x1.5babccb5b90dep+0, -0x1.396ce448f7000p-2},
        {0x1.59d61f2d91a78p+0, -0x1.3401e17bda000p-2},
        {0x1.5805612465687p+0, -0x1.2e9e2ef468000p-2},
        {0x1.56397cee76bd3p+0, -0x1.2941b3830e000p-2},
        {0x1.54725e2a77f93p+0, -0x1.23ec58cda8800p-2},
        {0x1.52aff42064583p+0, -0x1.1e9e129279000p-2},
        {0x1.50f22dbb2bddfp+0, -0x1.1956d2b48f800p-2},
        {0x1.4f38f4734ded7p+0, -0x1.141679ab9f800p-2},
        {0x1.4d843cfde2840p+0, -0x1.0edd094ef9800p-2},
        {0x1.4bd3ec078a3c8p+0, -0x1.09aa518db1000p-2},
        {0x1.4a27fc3e0258ap+0, -0x1.047e65263b800p-2},
        {0x1.4880524d48434p+0, -0x1.feb224586f000p-3},
        {0x1.46dce1b192d0bp+0, -0x1.f474a7517b000p-3},
        {0x1.453d9d3391854p+0, -0x1.ea4443d103000p-3},
        {0x1.43a2744b4845ap+0, -0x1.e020d44e9b000p-3},
        {0x1.420b54115f8fbp+0, -0x1.d60a22977f000p-3},
        {0x1.40782da3ef4b1p+0, -0x1.cc00104959000p-3},
        {0x1.3ee8f5d57fe8fp+0, -0x1.c202956891000p-3},
        {0x1.3d5d9a00b4ce9p+0, -0x1.b81178d811000p-3},
        {0x1.3bd60c010c12bp+0, -0x1.ae2c9ccd3d000p-3},
        {0x1.3a5242b75dab8p+0, -0x1.a45402e129000p-3},
        {0x1.38d22cd9fd002p+0, -0x1.9a877681df000p-3},
        {0x1.3755bc5847a1cp+0, -0x1.90c6d69483000p-3},
        {0x1.35dce49ad36e2p+0, -0x1.87120a645c000p-3},
        {0x1.34679984dd440p+0, -0x1.7d68fb4143000p-3},
        {0x1.32f5cceffcb24p+0, -0x1.73cb83c627000p-3},
        {0x1.3187775a10d49p+0, -0x1.6a39a9b376000p-3},
        {0x1.301c8373e3990p+0, -0x1.60b3154b7a000p-3},
        {0x1.2eb4ebb95f841p+0, -0x1.5737d76243000p-3},
        {0x1.2d50a0219a9d1p+0, -0x1.4dc7b8fc23000p-3},
        {0x1.2bef9a8b7fd2ap+0, -0x1.4462c51d20000p-3},
        {0x1.2a91c7a0c1babp+0, -0x1.3b08abc830000p-3},
        {0x1.293726014b530p+0, -0x1.31b996b490000p-3},
        {0x1.27dfa5757a1f5p+0, -0x1.2875490a44000p-3},
        {0x1.268b39b1d3bbfp+0, -0x1.1f3b9f879a000p-3},
        {0x1.2539d838ff5bdp+0, -0x1.160c8252ca000p-3},
        {0x1.23eb7aac9083bp+0, -0x1.0ce7f57f72000p-3},
        {0x1.22a012ba940b6p+0, -0x1.03cdc49fea000p-3},
        {0x1.2157996cc4132p+0, -0x1.f57bdbc4b8000p-4},
        {0x1.201201dd2fc9bp+0, -0x1.e370896404000p-4},
        {0x1.1ecf4494d480bp+0, -0x1.d17983ef94000p-4},
        {0x1.1d8f5528f6569p+0, -0x1.bf9674ed8a000p-4},
        {0x1.1c52311577e7cp+0, -0x1.adc79202f6000p-4},
        {0x1.1b17c74cb26e9p+0, -0x1.9c0c3e7288000p-4},
        {0x1.19e010c2c1ab6p+0, -0x1.8a646b372c000p-4},
        {0x1.18ab07bb670bdp+0, -0x1.78d01b3ac0000p-4},
        {0x1.1778a25efbcb6p+0, -0x1.674f145380000p-4},
        {0x1.1648d354c31dap+0, -0x1.55e0e6d878000p-4},
        {0x1.151b990275fddp+0, -0x1.4485cdea1e000p-4},
        {0x1.13f0ea432d24cp+0, -0x1.333d94d6aa000p-4},
        {0x1.12c8b7210f9dap+0, -0x1.22079f8c56000p-4},
        {0x1.11a3028ecb531p+0, -0x1.10e4698622000p-4},
        {0x1.107fbda8434afp+0, -0x1.ffa6c6ad20000p-5},
        {0x1.0f5ee0f4e6bb3p+0, -0x1.dda8d4a774000p-5},
        {0x1.0e4065d2a9fcep+0, -0x1.bbcece4850000p-5},
        {0x1.0d244632ca521p+0, -0x1.9a1894012c000p-5},
        {0x1.0c0a77ce2981ap+0, -0x1.788583302c000p-5},
        {0x1.0af2f83c636d1p+0, -0x1.5715e67d68000p-5},
        {0x1.09ddb98a01339p+0, -0x1.35c8a49658000p-5},
        {0x1.08cabaf52e7dfp+0, -0x1.149e364154000p-5},
        {0x1.07b9f2f4e28fbp+0, -0x1.e72c082eb8000p-6},
        {0x1.06ab58c358f19p+0, -0x1.a55f152528000p-6},
        {0x1.059eea5ecf92cp+0, -0x1.63d62cf818000p-6},
        {0x1.04949cdd12c90p+0, -0x1.228fb8caa0000p-6},
        {0x1.038c6c6f0ada9p+0, -0x1.c317b20f90000p-7},
        {0x1.02865137932a9p+0, -0x1.419355daa0000p-7},
        {0x1.0182427ea7348p+0, -0x1.81203c2ec0000p-8},
        {0x1.008040614b195p+0, -0x1.0040979240000p-9},
        {0x1.fe01ff726fa1ap-1, 0x1.feff384900000p-9},
        {0x1.fa11cc261ea74p-1, 0x1.7dc41353d0000p-7},
        {0x1.f6310b081992ep-1, 0x1.3cea3c4c28000p-6},
        {0x1.f25f63ceeadcdp-1, 0x1.b9fc114890000p-6},
        {0x1.ee9c8039113e7p-1, 0x1.1b0d8ce110000p-5},
        {0x1.eae8078cbb1abp-1, 0x1.58a5bd001c000p-5},
        {0x1.e741aa29d0c9bp-1, 0x1.95c8340d88000p-5},
        {0x1.e3a91830a99b5p-1, 0x1.d276aef578000p-5},
        {0x1.e01e009609a56p-1, 0x1.07598e598c000p-4},
        {0x1.dca01e577bb98p-1, 0x1.253f5e30d2000p-4},
        {0x1.d92f20b7c9103p-1, 0x1.42edd8b380000p-4},
        {0x1.d5cac66fb5ccep-1, 0x1.606598757c000p-4},
        {0x1.d272caa5ede9dp-1, 0x1.7da76356a0000p-4},
        {0x1.cf26e3e6b2ccdp-1, 0x1.9ab434e1c6000p-4},
        {0x1.cbe6da2a77902p-1, 0x1.b78c7bb0d6000p-4},
        {0x1.c8b266d37086dp-1, 0x1.d431332e72000p-4},
        {0x1.c5894bd5d5804p-1, 0x1.f0a3171de6000p-4},
        {0x1.c26b533bb9f8cp-1, 0x1.067152b914000p-3},
        {0x1.bf583eeece73fp-1, 0x1.147858292b000p-3},
        {0x1.bc4fd75db96c1p-1, 0x1.2266ecdca3000p-3},
        {0x1.b951e0c864a28p-1, 0x1.303d7a6c55000p-3},
        {0x1.b65e2c5ef3e2cp-1, 0x1.3dfc33c331000p-3},
        {0x1.b374867c9888bp-1, 0x1.4ba366b7a8000p-3},
        {0x1.b094b211d304ap-1, 0x1.5933928d1f000p-3},
        {0x1.adbe885f2ef7ep-1, 0x1.66acd2418f000p-3},
        {0x1.aaf1d31603da2p-1, 0x1.740f8ec669000p-3},
        {0x1.a82e63fd358a7p-1, 0x1.815c0f51af000p-3},
        {0x1.a5740ef09738bp-1, 0x1.8e92954f68000p-3},
        {0x1.a2c2a90ab4b27p-1, 0x1.9bb3602f84000p-3},
        {0x1.a01a01393f2d1p-1, 0x1.a8bed1c2c0000p-3},
        {0x1.9d79f24db3c1bp-1, 0x1.b5b515c01d000p-3},
        {0x1.9ae2505c7b190p-1, 0x1.c2967ccbcc000p-3},
        {0x1.9852ef297ce2fp-1, 0x1.cf635d5486000p-3},
        {0x1.95cbaeea44b75p-1, 0x1.dc1bd3446c000p-3},
        {0x1.934c69de74838p-1, 0x1.e8c01b8cfe000p-3},
        {0x1.90d4f2f6752e6p-1, 0x1.f5509c0179000p-3},
        {0x1.8e6528effd79dp-1, 0x1.00e6c121fb800p-2},
        {0x1.8bfce9fcc007cp-1, 0x1.071b80e93d000p-2},
        {0x1.899c0dabec30ep-1, 0x1.0d46b9e867000p-2},
        {0x1.87427aa2317fbp-1, 0x1.13687334bd000p-2},
        {0x1.84f00acb39a08p-1, 0x1.1980d67234800p-2},
        {0x1.82a49e8653e55p-1, 0x1.1f8ffe0cc8000p-2},
        {0x1.8060195f40260p-1, 0x1.2595fd7636800p-2},
        {0x1.7e22563e0a329p-1, 0x1.2b9300914a800p-2},
        {0x1.7beb377dcb5adp-1, 0x1.3187210436000p-2},
        {0x1.79baa679725c2p-1, 0x1.377266dec1800p-2},
        {0x1.77907f2170657p-1, 0x1.3d54ffbaf3000p-2},
        {0x1.756cadbd6130cp-1, 0x1.432eee32fe000p-2}
    };
    static const struct {
        double chi, clo;
    } T2[] = {
        {0x1.61000014fb66bp-1, 0x1.e026c91425b3cp-56},
        {0x1.63000034db495p-1, 0x1.dbfea48005d41p-55},
        {0x1.650000d94d478p-1, 0x1.e7fa786d6a5b7p-55},
        {0x1.67000074e6fadp-1, 0x1.1fcea6b54254cp-57},
        {0x1.68ffffedf0faep-1, -0x1.c7e274c590efdp-56},
        {0x1.6b0000763c5bcp-1, -0x1.ac16848dcda01p-55},
        {0x1.6d0001e5cc1f6p-1, 0x1.33f1c9d499311p-55},
        {0x1.6efffeb05f63ep-1, -0x1.e80041ae22d53p-56},
        {0x1.710000e86978p-1, 0x1.bff6671097952p-56},
        {0x1.72ffffc67e912p-1, 0x1.c00e226bd8724p-55},
        {0x1.74fffdf81116ap-1, -0x1.e02916ef101d2p-57},
        {0x1.770000f679c9p-1, -0x1.7fc71cd549c74p-57},
        {0x1.78ffffa7ec835p-1, 0x1.1bec19ef50483p-55},
        {0x1.7affffe20c2e6p-1, -0x1.07e1729cc6465p-56},
        {0x1.7cfffed3fc9p-1, -0x1.08072087b8b1cp-55},
        {0x1.7efffe9261a76p-1, 0x1.dc0286d9df9aep-55},
        {0x1.81000049ca3e8p-1, 0x1.97fd251e54c33p-55},
        {0x1.8300017932c8fp-1, -0x1.afee9b630f381p-55},
        {0x1.850000633739cp-1, 0x1.9bfbf6b6535bcp-55},
        {0x1.87000204289c6p-1, -0x1.bbf65f3117b75p-55},
        {0x1.88fffebf57904p-1, -0x1.9006ea23dcb57p-55},
        {0x1.8b00022bc04dfp-1, -0x1.d00df38e04b0ap-56},
        {0x1.8cfffe50c1b8ap-1, -0x1.8007146ff9f05p-55},
        {0x1.8effffc918e43p-1, 0x1.3817bd07a7038p-55},
        {0x1.910001efa5fc7p-1, 0x1.93e9176dfb403p-55},
        {0x1.9300013467bb9p-1, 0x1.f804e4b980276p-56},
        {0x1.94fffe6ee076fp-1, -0x1.f7ef0d9ff622ep-55},
        {0x1.96fffde3c12d1p-1, -0x1.082aa962638bap-56},
        {0x1.98ffff4458a0dp-1, -0x1.7801b9164a8efp-55},
        {0x1.9afffdd982e3ep-1, -0x1.740e08a5a9337p-55},
        {0x1.9cfffed49fb66p-1, 0x1.fce08c19bep-60},
        {0x1.9f00020f19c51p-1, -0x1.a3faa27885b0ap-55},
        {0x1.a10001145b006p-1, 0x1.4ff489958da56p-56},
        {0x1.a300007bbf6fap-1, 0x1.cbeab8a2b6d18p-55},
        {0x1.a500010971d79p-1, 0x1.8fecadd78793p-55},
        {0x1.a70001df52e48p-1, -0x1.f41763dd8abdbp-55},
        {0x1.a90001c593352p-1, -0x1.ebf0284c27612p-55},
        {0x1.ab0002a4f3e4bp-1, -0x1.9fd043cff3f5fp-57},
        {0x1.acfffd7ae1ed1p-1, -0x1.23ee7129070b4p-55},
        {0x1.aefffee510478p-1, 0x1.a063ee00edea3p-57},
        {0x1.b0fffdb650d5bp-1, 0x1.a06c8381f0ab9p-58},
        {0x1.b2ffffeaaca57p-1, -0x1.9011e74233c1dp-56},
        {0x1.b4fffd995badcp-1, -0x1.9ff1068862a9fp-56},
        {0x1.b7000249e659cp-1, 0x1.aff45d0864f3ep-55},
        {0x1.b8ffff987164p-1, 0x1.cfe7796c2c3f9p-56},
        {0x1.bafffd204cb4fp-1, -0x1.3ff27eef22bc4p-57},
        {0x1.bcfffd2415c45p-1, -0x1.cffb7ee3bea21p-57},
        {0x1.beffff86309dfp-1, -0x1.14103972e0b5cp-55},
        {0x1.c0fffe1b57653p-1, 0x1.bc16494b76a19p-55},
        {0x1.c2ffff1fa57e3p-1, -0x1.4feef8d30c6edp-57},
        {0x1.c4fffdcbfe424p-1, -0x1.43f68bcec4775p-55},
        {0x1.c6fffed54b9f7p-1, 0x1.47ea3f053e0ecp-55},
        {0x1.c8fffeb998fd5p-1, 0x1.383068df992f1p-56},
        {0x1.cb0002125219ap-1, -0x1.8fd8e64180e04p-57},
        {0x1.ccfffdd94469cp-1, 0x1.e7ebe1cc7ea72p-55},
        {0x1.cefffeafdc476p-1, 0x1.ebe39ad9f88fep-55},
        {0x1.d1000169af82bp-1, 0x1.57d91a8b95a71p-56},
        {0x1.d30000d0ff71dp-1, 0x1.9c1906970c7dap-55},
        {0x1.d4fffea790fc4p-1, -0x1.80e37c558fe0cp-58},
        {0x1.d70002edc87e5p-1, -0x1.f80d64dc10f44p-56},
        {0x1.d900021dc82aap-1, -0x1.47c8f94fd5c5cp-56},
        {0x1.dafffd86b0283p-1, 0x1.c7f1dc521617ep-55},
        {0x1.dd000296c4739p-1, 0x1.8019eb2ffb153p-55},
        {0x1.defffe54490f5p-1, 0x1.e00d2c652cc89p-57},
        {0x1.e0fffcdabf694p-1, -0x1.f8340202d69d2p-56},
        {0x1.e2fffdb52c8ddp-1, 0x1.b00c1ca1b0864p-56},
        {0x1.e4ffff24216efp-1, 0x1.2ffa8b094ab51p-56},
        {0x1.e6fffe88a5e11p-1, -0x1.7f673b1efbe59p-58},
        {0x1.e9000119eff0dp-1, -0x1.4808d5e0bc801p-55},
        {0x1.eafffdfa51744p-1, 0x1.80006d54320b5p-56},
        {0x1.ed0001a127fa1p-1, -0x1.002f860565c92p-58},
        {0x1.ef00007babcc4p-1, -0x1.540445d35e611p-55},
        {0x1.f0ffff57a8d02p-1, -0x1.ffb3139ef9105p-59},
        {0x1.f30001ee58ac7p-1, 0x1.a81acf2731155p-55},
        {0x1.f4ffff5823494p-1, 0x1.a3f41d4d7c743p-55},
        {0x1.f6ffffca94c6bp-1, -0x1.202f41c987875p-57},
        {0x1.f8fffe1f9c441p-1, 0x1.77dd1f477e74bp-56},
        {0x1.fafffd2e0e37ep-1, -0x1.f01199a7ca331p-57},
        {0x1.fd0001c77e49ep-1, 0x1.181ee4bceacb1p-56},
        {0x1.feffff7e0c331p-1, -0x1.e05370170875ap-57},
        {0x1.00ffff465606ep+0, -0x1.a7ead491c0adap-55},
        {0x1.02ffff3867a58p+0, -0x1.77f69c3fcb2ep-54},
        {0x1.04ffffdfc0d17p+0, 0x1.7bffe34cb945bp-54},
        {0x1.0700003cd4d82p+0, 0x1.20083c0e456cbp-55},
        {0x1.08ffff9f2cbe8p+0, -0x1.dffdfbe37751ap-57},
        {0x1.0b000010cda65p+0, -0x1.13f7faee626ebp-54},
        {0x1.0d00001a4d338p+0, 0x1.07dfa79489ff7p-55},
        {0x1.0effffadafdfdp+0, -0x1.7040570d66bcp-56},
        {0x1.110000bbafd96p+0, 0x1.e80d4846d0b62p-55},
        {0x1.12ffffae5f45dp+0, 0x1.dbffa64fd36efp-54},
        {0x1.150000dd59ad9p+0, 0x1.a0077701250aep-54},
        {0x1.170000f21559ap+0, 0x1.dfdf9e2e3deeep-55},
        {0x1.18ffffc275426p+0, 0x1.10030dc3b7273p-54},
        {0x1.1b000123d3c59p+0, 0x1.97f7980030188p-54},
        {0x1.1cffff8299eb7p+0, -0x1.5f932ab9f8c67p-57},
        {0x1.1effff48ad4p+0, 0x1.37fbf9da75bebp-54},
        {0x1.210000c8b86a4p+0, 0x1.f806b91fd5b22p-54},
        {0x1.2300003854303p+0, 0x1.3ffc2eb9fbf33p-54},
        {0x1.24fffffbcf684p+0, 0x1.601e77e2e2e72p-56},
        {0x1.26ffff52921d9p+0, 0x1.ffcbb767f0c61p-56},
        {0x1.2900014933a3cp+0, -0x1.202ca3c02412bp-56},
        {0x1.2b00014556313p+0, -0x1.2808233f21f02p-54},
        {0x1.2cfffebfe523bp+0, -0x1.8ff7e384fdcf2p-55},
        {0x1.2f0000bb8ad96p+0, -0x1.5ff51503041c5p-55},
        {0x1.30ffffb7ae2afp+0, -0x1.10071885e289dp-55},
        {0x1.32ffffeac5f7fp+0, -0x1.1ff5d3fb7b715p-54},
        {0x1.350000ca66756p+0, 0x1.57f82228b82bdp-54},
        {0x1.3700011fbf721p+0, 0x1.000bac40dd5ccp-55},
        {0x1.38ffff9592fb9p+0, -0x1.43f9d2db2a751p-54},
        {0x1.3b00004ddd242p+0, 0x1.57f6b707638e1p-55},
        {0x1.3cffff5b2c957p+0, 0x1.a023a10bf1231p-56},
        {0x1.3efffeab0b418p+0, 0x1.87f6d66b152bp-54},
        {0x1.410001532aff4p+0, 0x1.7f8375f198524p-57},
        {0x1.4300017478b29p+0, 0x1.301e672dc5143p-55},
        {0x1.44fffe795b463p+0, 0x1.9ff69b8b2895ap-55},
        {0x1.46fffe80475ep+0, -0x1.5c0b19bc2f254p-54},
        {0x1.48fffef6fc1e7p+0, 0x1.b4009f23a2a72p-54},
        {0x1.4afffe5bea704p+0, -0x1.4ffb7bf0d7d45p-54},
        {0x1.4d000171027dep+0, -0x1.9c06471dc6a3dp-54},
        {0x1.4f0000ff03ee2p+0, 0x1.77f890b85531cp-54},
        {0x1.5100012dc4bd1p+0, 0x1.004657166a436p-57},
        {0x1.530001605277ap+0, -0x1.6bfcece233209p-54},
        {0x1.54fffecdb704cp+0, -0x1.902720505a1d7p-55},
        {0x1.56fffef5f54a9p+0, 0x1.bbfe60ec96412p-54},
        {0x1.5900017e61012p+0, 0x1.87ec581afef9p-55},
        {0x1.5b00003c93e92p+0, -0x1.f41080abf0ccp-54},
        {0x1.5d0001d4919bcp+0, -0x1.8812afb254729p-54},
        {0x1.5efffe7b87a89p+0, -0x1.47eb780ed6904p-54}
    };

    double w, z, r, r2, r3, y, invc, logc, kd, hi, lo;
    UINT64 ix, iz, tmp;
    UINT32 top;
    int k, i;

    ix = *(UINT64*)&x;
    top = ix >> 48;
    if (ix - 0x3fee000000000000ULL < 0x3090000000000ULL) {
        double rhi, rlo;

        /* Handle close to 1.0 inputs separately. */
        /* Fix sign of zero with downward rounding when x==1. */
        if (ix == 0x3ff0000000000000ULL)
            return 0;
        r = x - 1.0;
        r2 = r * r;
        r3 = r * r2;
        y = r3 * (B[1] + r * B[2] + r2 * B[3] + r3 * (B[4] + r * B[5] + r2 * B[6] +
                    r3 * (B[7] + r * B[8] + r2 * B[9] + r3 * B[10])));
        /* Worst-case error is around 0.507 ULP. */
        w = r * 0x1p27;
        rhi = r + w - w;
        rlo = r - rhi;
        w = rhi * rhi * B[0]; /* B[0] == -0.5. */
        hi = r + w;
        lo = r - hi + w;
        lo += B[0] * rlo * (rhi + r);
        y += lo;
        y += hi;
        return y;
    }
    if (top - 0x0010 >= 0x7ff0 - 0x0010) {
        /* x < 0x1p-1022 or inf or nan. */
        if (ix * 2 == 0)
            return math_error(_SING, "log", x, 0, (top & 0x8000 ? 1.0 : -1.0) / x);
        if (ix == 0x7ff0000000000000ULL) /* log(inf) == inf. */
            return x;
        if ((top & 0x7ff0) == 0x7ff0 && (ix & 0xfffffffffffffULL))
            return x;
        if (top & 0x8000)
            return math_error(_DOMAIN, "log", x, 0, (x - x) / (x - x));
        /* x is subnormal, normalize it. */
        x *= 0x1p52;
        ix = *(UINT64*)&x;
        ix -= 52ULL << 52;
    }

    /* x = 2^k z; where z is in range [OFF,2*OFF) and exact.
       The range is split into N subintervals.
       The ith subinterval contains z and c is near its center. */
    tmp = ix - 0x3fe6000000000000ULL;
    i = (tmp >> (52 - 7)) % (1 << 7);
    k = (INT64)tmp >> 52; /* arithmetic shift */
    iz = ix - (tmp & 0xfffULL << 52);
    invc = T[i].invc;
    logc = T[i].logc;
    z = *(double*)&iz;

    /* log(x) = log1p(z/c-1) + log(c) + k*Ln2. */
    /* r ~= z/c - 1, |r| < 1/(2*N). */
    r = (z - T2[i].chi - T2[i].clo) * invc;
    kd = (double)k;

    /* hi + lo = r + log(c) + k*Ln2. */
    w = kd * Ln2hi + logc;
    hi = w + r;
    lo = w - hi + r + kd * Ln2lo;

    /* log(x) = lo + (log1p(r) - r) + hi. */
    r2 = r * r; /* rounding error: 0x1p-54/N^2. */
    /* Worst case error if |y| > 0x1p-5:
       0.5 + 4.13/N + abs-poly-error*2^57 ULP (+ 0.002 ULP without fma)
       Worst case error if |y| > 0x1p-4:
       0.5 + 2.06/N + abs-poly-error*2^56 ULP (+ 0.001 ULP without fma). */
    y = lo + r2 * A[0] +
        r * r2 * (A[1] + r * A[2] + r2 * (A[3] + r * A[4])) + hi;
    return y;
}

/*********************************************************************
 *		log10 (MSVCRT.@)
 */
double CDECL log10( double x )
{
    static const double ivln10hi = 4.34294481878168880939e-01,
        ivln10lo = 2.50829467116452752298e-11,
        log10_2hi = 3.01029995663611771306e-01,
        log10_2lo = 3.69423907715893078616e-13,
        Lg1 = 6.666666666666735130e-01,
        Lg2 = 3.999999999940941908e-01,
        Lg3 = 2.857142874366239149e-01,
        Lg4 = 2.222219843214978396e-01,
        Lg5 = 1.818357216161805012e-01,
        Lg6 = 1.531383769920937332e-01,
        Lg7 = 1.479819860511658591e-01;

    union {double f; UINT64 i;} u = {x};
    double hfsq, f, s, z, R, w, t1, t2, dk, y, hi, lo, val_hi, val_lo;
    UINT32 hx;
    int k;

    hx = u.i >> 32;
    k = 0;
    if (hx < 0x00100000 || hx >> 31) {
        if (u.i << 1 == 0)
            return math_error(_SING, "log10", x, 0, -1 / (x * x));
        if ((u.i & ~(1ULL << 63)) > 0x7ff0000000000000ULL)
            return x;
        if (hx >> 31)
            return math_error(_DOMAIN, "log10", x, 0, (x - x) / (x - x));
        /* subnormal number, scale x up */
        k -= 54;
        x *= 0x1p54;
        u.f = x;
        hx = u.i >> 32;
    } else if (hx >= 0x7ff00000) {
        return x;
    } else if (hx == 0x3ff00000 && u.i<<32 == 0)
        return 0;

    /* reduce x into [sqrt(2)/2, sqrt(2)] */
    hx += 0x3ff00000 - 0x3fe6a09e;
    k += (int)(hx >> 20) - 0x3ff;
    hx = (hx & 0x000fffff) + 0x3fe6a09e;
    u.i = (UINT64)hx << 32 | (u.i & 0xffffffff);
    x = u.f;

    f = x - 1.0;
    hfsq = 0.5 * f * f;
    s = f / (2.0 + f);
    z = s * s;
    w = z * z;
    t1 = w * (Lg2 + w * (Lg4 + w * Lg6));
    t2 = z * (Lg1 + w * (Lg3 + w * (Lg5 + w * Lg7)));
    R = t2 + t1;

    /* hi+lo = f - hfsq + s*(hfsq+R) ~ log(1+f) */
    hi = f - hfsq;
    u.f = hi;
    u.i &= (UINT64)-1 << 32;
    hi = u.f;
    lo = f - hi - hfsq + s * (hfsq + R);

    /* val_hi+val_lo ~ log10(1+f) + k*log10(2) */
    val_hi = hi * ivln10hi;
    dk = k;
    y = dk * log10_2hi;
    val_lo = dk * log10_2lo + (lo + hi) * ivln10lo + lo * ivln10hi;

    /*
     * Extra precision in for adding y is not strictly needed
     * since there is no very large cancellation near x = sqrt(2) or
     * x = 1/sqrt(2), but we do it anyway since it costs little on CPUs
     * with some parallelism and it reduces the error for many args.
     */
    w = y + val_hi;
    val_lo += (y - w) + val_hi;
    val_hi = w;

    return val_lo + val_hi;
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
    kd = __round(z);
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

/*********************************************************************
 *		sin (MSVCRT.@)
 *
 * Copied from musl: src/math/sin.c
 */
double CDECL sin( double x )
{
    double y[2];
    UINT32 ix;
    unsigned n;

    ix = *(ULONGLONG*)&x >> 32;
    ix &= 0x7fffffff;

    /* |x| ~< pi/4 */
    if (ix <= 0x3fe921fb) {
        if (ix < 0x3e500000) { /* |x| < 2**-26 */
            /* raise inexact if x != 0 and underflow if subnormal*/
            fp_barrier(ix < 0x00100000 ? x/0x1p120f : x+0x1p120f);
            return x;
        }
        return __sin(x, 0.0, 0);
    }

    /* sin(Inf or NaN) is NaN */
    if (isinf(x))
        return math_error(_DOMAIN, "sin", x, 0, x - x);
    if (ix >= 0x7ff00000)
        return x - x;

    /* argument reduction needed */
    n = __rem_pio2(x, y);
    switch (n&3) {
    case 0: return  __sin(y[0], y[1], 1);
    case 1: return  __cos(y[0], y[1]);
    case 2: return -__sin(y[0], y[1], 1);
    default: return -__cos(y[0], y[1]);
    }
}

/*********************************************************************
 *		sinh (MSVCRT.@)
 */
double CDECL sinh( double x )
{
    UINT64 ux = *(UINT64*)&x;
    UINT64 sign = ux & 0x8000000000000000ULL;
    UINT32 w;
    double t, h, absx;

    h = 0.5;
    if (ux >> 63)
        h = -h;
    /* |x| */
    ux &= (UINT64)-1 / 2;
    absx = *(double*)&ux;
    w = ux >> 32;

    /* |x| < log(DBL_MAX) */
    if (w < 0x40862e42) {
        t = __expm1(absx);
        if (w < 0x3ff00000) {
            if (w < 0x3ff00000 - (26 << 20))
                return x;
            return h * (2 * t - t * t / (t + 1));
        }
        return h * (t + t / (t + 1));
    }

    /* |x| > log(DBL_MAX) or nan */
    /* note: the result is stored to handle overflow */
    if (ux > 0x7ff0000000000000ULL)
        *(UINT64*)&t = ux | sign | 0x0008000000000000ULL;
    else
        t = __expo2(absx, 2 * h);
    return t;
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

/* Copied from musl: src/math/__tan.c */
static double __tan(double x, double y, int odd)
{
    static const double T[] = {
        3.33333333333334091986e-01,
        1.33333333333201242699e-01,
        5.39682539762260521377e-02,
        2.18694882948595424599e-02,
        8.86323982359930005737e-03,
        3.59207910759131235356e-03,
        1.45620945432529025516e-03,
        5.88041240820264096874e-04,
        2.46463134818469906812e-04,
        7.81794442939557092300e-05,
        7.14072491382608190305e-05,
        -1.85586374855275456654e-05,
        2.59073051863633712884e-05,
    };
    static const double pio4 = 7.85398163397448278999e-01;
    static const double pio4lo = 3.06161699786838301793e-17;

    double z, r, v, w, s, a, w0, a0;
    UINT32 hx;
    int big, sign;

    hx = *(ULONGLONG*)&x >> 32;
    big = (hx & 0x7fffffff) >= 0x3FE59428; /* |x| >= 0.6744 */
    if (big) {
        sign = hx >> 31;
        if (sign) {
            x = -x;
            y = -y;
        }
        x = (pio4 - x) + (pio4lo - y);
        y = 0.0;
    }
    z = x * x;
    w = z * z;
    r = T[1] + w * (T[3] + w * (T[5] + w * (T[7] + w * (T[9] + w * T[11]))));
    v = z * (T[2] + w * (T[4] + w * (T[6] + w * (T[8] + w * (T[10] + w * T[12])))));
    s = z * x;
    r = y + z * (s * (r + v) + y) + s * T[0];
    w = x + r;
    if (big) {
        s = 1 - 2 * odd;
        v = s - 2.0 * (x + (r - w * w / (w + s)));
        return sign ? -v : v;
    }
    if (!odd)
        return w;
    /* -1.0/(x+r) has up to 2ulp error, so compute it accurately */
    w0 = w;
    *(LONGLONG*)&w0 = *(LONGLONG*)&w0 & 0xffffffff00000000ULL;
    v = r - (w0 - x);       /* w0+v = r+x */
    a0 = a = -1.0 / w;
    *(LONGLONG*)&a0 = *(LONGLONG*)&a0 & 0xffffffff00000000ULL;
    return a0 + a * (1.0 + a0 * w0 + a0 * v);
}

/*********************************************************************
 *		tan (MSVCRT.@)
 *
 * Copied from musl: src/math/tan.c
 */
double CDECL tan( double x )
{
    double y[2];
    UINT32 ix;
    unsigned n;

    ix = *(ULONGLONG*)&x >> 32;
    ix &= 0x7fffffff;

    if (ix <= 0x3fe921fb) { /* |x| ~< pi/4 */
        if (ix < 0x3e400000) { /* |x| < 2**-27 */
            /* raise inexact if x!=0 and underflow if subnormal */
            fp_barrier(ix < 0x00100000 ? x / 0x1p120f : x + 0x1p120f);
            return x;
        }
        return __tan(x, 0.0, 0);
    }

    if (isinf(x))
        return math_error(_DOMAIN, "tan", x, 0, x - x);
    if (ix >= 0x7ff00000)
        return x - x;

    n = __rem_pio2(x, y);
    return __tan(y[0], y[1], n & 1);
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
            t = __expm1(2 * x);
            t = 1 - 2 / (t + 2);
        }
    } else if (w > 0x3fd058ae) {
        /* |x| > log(5/3)/2 ~= 0.2554 */
        t = __expm1(2 * x);
        t = t / (t + 2);
    } else if (w >= 0x00100000) {
        /* |x| >= 0x1p-1022, up to 2ulp error in [0.1,0.2554] */
        t = __expm1(-2 * x);
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

/* Copied from musl: src/math/ilogb.c */
static int __ilogb(double x)
{
    union { double f; UINT64 i; } u = { x };
    int e = u.i >> 52 & 0x7ff;

    if (!e)
    {
        u.i <<= 12;
        if (u.i == 0) return FP_ILOGB0;
        /* subnormal x */
        for (e = -0x3ff; u.i >> 63 == 0; e--, u.i <<= 1);
        return e;
    }
    if (e == 0x7ff) return u.i << 12 ? FP_ILOGBNAN : INT_MAX;
    return e - 0x3ff;
}

/*********************************************************************
 *		_logb (MSVCRT.@)
 *
 * Copied from musl: src/math/logb.c
 */
double CDECL _logb(double x)
{
    if (!isfinite(x))
        return x * x;
    if (x == 0)
        return math_error(_SING, "_logb", x, 0, -1 / (x * x));
    return __ilogb(x);
}

static void sq(double *hi, double *lo, double x)
{
    double xh, xl, xc;

    xc = x * (0x1p27 + 1);
    xh = x - xc + xc;
    xl = x - xh;
    *hi = x * x;
    *lo = xh * xh - *hi + 2 * xh * xl + xl * xl;
}

/*********************************************************************
 *		_hypot (MSVCRT.@)
 *
 * Copied from musl: src/math/hypot.c
 */
double CDECL _hypot(double x, double y)
{
    UINT64 ux = *(UINT64*)&x, uy = *(UINT64*)&y, ut;
    double hx, lx, hy, ly, z;
    int ex, ey;

    /* arrange |x| >= |y| */
    ux &= -1ULL >> 1;
    uy &= -1ULL >> 1;
    if (ux < uy) {
        ut = ux;
        ux = uy;
        uy = ut;
    }

    /* special cases */
    ex = ux >> 52;
    ey = uy >> 52;
    x = *(double*)&ux;
    y = *(double*)&uy;
    /* note: hypot(inf,nan) == inf */
    if (ey == 0x7ff)
        return y;
    if (ex == 0x7ff || uy == 0)
        return x;
    /* note: hypot(x,y) ~= x + y*y/x/2 with inexact for small y/x */
    /* 64 difference is enough for ld80 double_t */
    if (ex - ey > 64)
        return x + y;

    /* precise sqrt argument in nearest rounding mode without overflow */
    /* xh*xh must not overflow and xl*xl must not underflow in sq */
    z = 1;
    if (ex > 0x3ff + 510) {
        z = 0x1p700;
        x *= 0x1p-700;
        y *= 0x1p-700;
    } else if (ey < 0x3ff - 450) {
        z = 0x1p-700;
        x *= 0x1p700;
        y *= 0x1p700;
    }
    sq(&hx, &lx, x);
    sq(&hy, &ly, y);
    return z * sqrt(ly + lx + hy + hx);
}

/*********************************************************************
 *      _hypotf (MSVCRT.@)
 *
 * Copied from musl: src/math/hypotf.c
 */
float CDECL _hypotf(float x, float y)
{
    UINT32 ux = *(UINT32*)&x, uy = *(UINT32*)&y, ut;
    float z;

    ux &= -1U >> 1;
    uy &= -1U >> 1;
    if (ux < uy) {
        ut = ux;
        ux = uy;
        uy = ut;
    }

    x = *(float*)&ux;
    y = *(float*)&uy;
    if (uy == 0xff << 23)
        return y;
    if (ux >= 0xff << 23 || uy == 0 || ux - uy >= 25 << 23)
        return x + y;

    z = 1;
    if (ux >= (0x7f + 60) << 23) {
        z = 0x1p90f;
        x *= 0x1p-90f;
        y *= 0x1p-90f;
    } else if (uy < (0x7f - 60) << 23) {
        z = 0x1p-90f;
        x *= 0x1p90f;
        y *= 0x1p90f;
    }
    return z * sqrtf((double)x * x + (double)y * y);
}

/*********************************************************************
 *		ceil (MSVCRT.@)
 *
 * Based on musl: src/math/ceilf.c
 */
double CDECL ceil( double x )
{
    union {double f; UINT64 i;} u = {x};
    int e = (u.i >> 52 & 0x7ff) - 0x3ff;
    UINT64 m;

    if (e >= 52)
        return x;
    if (e >= 0) {
        m = 0x000fffffffffffffULL >> e;
        if ((u.i & m) == 0)
            return x;
        if (u.i >> 63 == 0)
            u.i += m;
        u.i &= ~m;
    } else {
        if (u.i >> 63)
            return -0.0;
        else if (u.i << 1)
            return 1.0;
    }
    return u.f;
}

/*********************************************************************
 *		floor (MSVCRT.@)
 *
 * Based on musl: src/math/floorf.c
 */
double CDECL floor( double x )
{
    union {double f; UINT64 i;} u = {x};
    int e = (int)(u.i >> 52 & 0x7ff) - 0x3ff;
    UINT64 m;

    if (e >= 52)
        return x;
    if (e >= 0) {
        m = 0x000fffffffffffffULL >> e;
        if ((u.i & m) == 0)
            return x;
        if (u.i >> 63)
            u.i += m;
        u.i &= ~m;
    } else {
        if (u.i >> 63 == 0)
            return 0;
        else if (u.i << 1)
            return -1;
    }
    return u.f;
}

/*********************************************************************
 *      fma (MSVCRT.@)
 *
 * Copied from musl: src/math/fma.c
 */
struct fma_num
{
    UINT64 m;
    int e;
    int sign;
};

static struct fma_num normalize(double x)
{
    UINT64 ix = *(UINT64*)&x;
    int e = ix >> 52;
    int sign = e & 0x800;
    struct fma_num ret;

    e &= 0x7ff;
    if (!e) {
        x *= 0x1p63;
        ix = *(UINT64*)&x;
        e = ix >> 52 & 0x7ff;
        e = e ? e - 63 : 0x800;
    }
    ix &= (1ull << 52) - 1;
    ix |= 1ull << 52;
    ix <<= 1;
    e -= 0x3ff + 52 + 1;

    ret.m = ix;
    ret.e = e;
    ret.sign = sign;
    return ret;
}

static void mul(UINT64 *hi, UINT64 *lo, UINT64 x, UINT64 y)
{
    UINT64 t1, t2, t3;
    UINT64 xlo = (UINT32)x, xhi = x >> 32;
    UINT64 ylo = (UINT32)y, yhi = y >> 32;

    t1 = xlo * ylo;
    t2 = xlo * yhi + xhi * ylo;
    t3 = xhi * yhi;
    *lo = t1 + (t2 << 32);
    *hi = t3 + (t2 >> 32) + (t1 > *lo);
}

double CDECL fma( double x, double y, double z )
{
    int e, d, sign, samesign, nonzero;
    UINT64 rhi, rlo, zhi, zlo;
    struct fma_num nx, ny, nz;
    double r;
    INT64 i;

    /* normalize so top 10bits and last bit are 0 */
    nx = normalize(x);
    ny = normalize(y);
    nz = normalize(z);

    if (nx.e >= 0x7ff - 0x3ff - 52 - 1 || ny.e >= 0x7ff - 0x3ff - 52 - 1) {
        r = x * y + z;
        if (!isnan(x) && !isnan(y) && !isnan(z) && isnan(r)) *_errno() = EDOM;
        return r;
    }
    if (nz.e >= 0x7ff - 0x3ff - 52 - 1) {
        if (nz.e > 0x7ff - 0x3ff - 52 - 1) {/* z==0 */
            r = x * y + z;
            if (!isnan(x) && !isnan(y) && isnan(r)) *_errno() = EDOM;
            return r;
        }
        return z;
    }

    /* mul: r = x*y */
    mul(&rhi, &rlo, nx.m, ny.m);
    /* either top 20 or 21 bits of rhi and last 2 bits of rlo are 0 */

    /* align exponents */
    e = nx.e + ny.e;
    d = nz.e - e;
    /* shift bits z<<=kz, r>>=kr, so kz+kr == d, set e = e+kr (== ez-kz) */
    if (d > 0) {
        if (d < 64) {
            zlo = nz.m << d;
            zhi = nz.m >> (64 - d);
        } else {
            zlo = 0;
            zhi = nz.m;
            e = nz.e - 64;
            d -= 64;
            if (d < 64 && d) {
                rlo = rhi << (64 - d) | rlo >> d | !!(rlo << (64 - d));
                rhi = rhi >> d;
            } else if (d) {
                rlo = 1;
                rhi = 0;
            }
        }
    } else {
        zhi = 0;
        d = -d;
        if (d == 0) {
            zlo = nz.m;
        } else if (d < 64) {
            zlo = nz.m >> d | !!(nz.m << (64 - d));
        } else {
            zlo = 1;
        }
    }

    /* add */
    sign = nx.sign ^ ny.sign;
    samesign = !(sign ^ nz.sign);
    nonzero = 1;
    if (samesign) {
        /* r += z */
        rlo += zlo;
        rhi += zhi + (rlo < zlo);
    } else {
        /* r -= z */
        UINT64 t = rlo;
        rlo -= zlo;
        rhi = rhi - zhi - (t < rlo);
        if (rhi >> 63) {
            rlo = -rlo;
            rhi = -rhi - !!rlo;
            sign = !sign;
        }
        nonzero = !!rhi;
    }

    /* set rhi to top 63bit of the result (last bit is sticky) */
    if (nonzero) {
        e += 64;
        if (rhi >> 32) {
            BitScanReverse((DWORD*)&d, rhi >> 32);
            d = 31 - d - 1;
        } else {
            BitScanReverse((DWORD*)&d, rhi);
            d = 63 - d - 1;
        }
        /* note: d > 0 */
        rhi = rhi << d | rlo >> (64 - d) | !!(rlo << d);
    } else if (rlo) {
        if (rlo >> 32) {
            BitScanReverse((DWORD*)&d, rlo >> 32);
            d = 31 - d - 1;
        } else {
            BitScanReverse((DWORD*)&d, rlo);
            d = 63 - d - 1;
        }
        if (d < 0)
            rhi = rlo >> 1 | (rlo & 1);
        else
            rhi = rlo << d;
    } else {
        /* exact +-0 */
        return x * y + z;
    }
    e -= d;

    /* convert to double */
    i = rhi; /* i is in [1<<62,(1<<63)-1] */
    if (sign)
        i = -i;
    r = i; /* |r| is in [0x1p62,0x1p63] */

    if (e < -1022 - 62) {
        /* result is subnormal before rounding */
        if (e == -1022 - 63) {
            double c = 0x1p63;
            if (sign)
                c = -c;
            if (r == c) {
                /* min normal after rounding, underflow depends
                   on arch behaviour which can be imitated by
                   a double to float conversion */
                float fltmin = 0x0.ffffff8p-63 * FLT_MIN * r;
                return DBL_MIN / FLT_MIN * fltmin;
            }
            /* one bit is lost when scaled, add another top bit to
               only round once at conversion if it is inexact */
            if (rhi << 53) {
                double tiny;

                i = rhi >> 1 | (rhi & 1) | 1ull << 62;
                if (sign)
                    i = -i;
                r = i;
                r = 2 * r - c; /* remove top bit */

                /* raise underflow portably, such that it
                   cannot be optimized away */
                tiny = DBL_MIN / FLT_MIN * r;
                r += (double)(tiny * tiny) * (r - r);
            }
        } else {
            /* only round once when scaled */
            d = 10;
            i = (rhi >> d | !!(rhi << (64 - d))) << d;
            if (sign)
                i = -i;
            r = i;
        }
    }
    return __scalbn(r, e);
}

/*********************************************************************
 *      fmaf (MSVCRT.@)
 *
 * Copied from musl: src/math/fmaf.c
 */
float CDECL fmaf( float x, float y, float z )
{
    union { double f; UINT64 i; } u;
    double xy, adjust;
    int e;

    xy = (double)x * y;
    u.f = xy + z;
    e = u.i>>52 & 0x7ff;
    /* Common case: The double precision result is fine. */
    if ((u.i & 0x1fffffff) != 0x10000000 || /* not a halfway case */
            e == 0x7ff || /* NaN */
            (u.f - xy == z && u.f - z == xy) || /* exact */
            (_controlfp(0, 0) & _MCW_RC) != _RC_NEAR) /* not round-to-nearest */
    {
        if (!isnan(x) && !isnan(y) && !isnan(z) && isnan(u.f)) *_errno() = EDOM;

        /* underflow may not be raised correctly, example:
           fmaf(0x1p-120f, 0x1p-120f, 0x1p-149f) */
        if (e < 0x3ff-126 && e >= 0x3ff-149 && _statusfp() & _SW_INEXACT)
            fp_barrierf((float)u.f * (float)u.f);
        return u.f;
    }

    /*
     * If result is inexact, and exactly halfway between two float values,
     * we need to adjust the low-order bit in the direction of the error.
     */
    _controlfp(_RC_CHOP, _MCW_RC);
    adjust = fp_barrier(xy + z);
    _controlfp(_RC_NEAR, _MCW_RC);
    if (u.f == adjust)
        u.i++;
    return u.f;
}

/*********************************************************************
 *		fabs (MSVCRT.@)
 *
 * Copied from musl: src/math/fabsf.c
 */
double CDECL fabs( double x )
{
    union { double f; UINT64 i; } u = { x };
    u.i &= ~0ull >> 1;
    return u.f;
}

/*********************************************************************
 *		frexp (MSVCRT.@)
 *
 * Copied from musl: src/math/frexp.c
 */
double CDECL frexp( double x, int *e )
{
    UINT64 ux = *(UINT64*)&x;
    int ee = ux >> 52 & 0x7ff;

    if (!ee) {
        if (x) {
            x = frexp(x * 0x1p64, e);
            *e -= 64;
        } else *e = 0;
        return x;
    } else if (ee == 0x7ff) {
        return x;
    }

    *e = ee - 0x3fe;
    ux &= 0x800fffffffffffffull;
    ux |= 0x3fe0000000000000ull;
    return *(double*)&ux;
}

/*********************************************************************
 *		modf (MSVCRT.@)
 *
 * Copied from musl: src/math/modf.c
 */
double CDECL modf( double x, double *iptr )
{
    union {double f; UINT64 i;} u = {x};
    UINT64 mask;
    int e = (u.i >> 52 & 0x7ff) - 0x3ff;

    /* no fractional part */
    if (e >= 52) {
        *iptr = x;
        if (e == 0x400 && u.i << 12 != 0) /* nan */
            return x;
        u.i &= 1ULL << 63;
        return u.f;
    }

    /* no integral part*/
    if (e < 0) {
        u.i &= 1ULL << 63;
        *iptr = u.f;
        return x;
    }

    mask = -1ULL >> 12 >> e;
    if ((u.i & mask) == 0) {
        *iptr = x;
        u.i &= 1ULL << 63;
        return u.f;
    }
    u.i &= ~mask;
    *iptr = u.f;
    return x - u.f;
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
  double z = __scalbn(num, exp);

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
 *		_copysign (MSVCRT.@)
 *
 * Copied from musl: src/math/copysign.c
 */
double CDECL _copysign( double x, double y )
{
    union { double f; UINT64 i; } ux = { x }, uy = { y };
    ux.i &= ~0ull >> 1;
    ux.i |= uy.i & 1ull << 63;
    return ux.f;
}

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

static double pzero(double x)
{
    static const double pR8[6] = { /* for x in [inf, 8]=1/[0,0.125] */
        0.00000000000000000000e+00,
        -7.03124999999900357484e-02,
        -8.08167041275349795626e+00,
        -2.57063105679704847262e+02,
        -2.48521641009428822144e+03,
        -5.25304380490729545272e+03,
    }, pS8[5] = {
        1.16534364619668181717e+02,
        3.83374475364121826715e+03,
        4.05978572648472545552e+04,
        1.16752972564375915681e+05,
        4.76277284146730962675e+04,
    }, pR5[6] = { /* for x in [8,4.5454]=1/[0.125,0.22001] */
        -1.14125464691894502584e-11,
        -7.03124940873599280078e-02,
        -4.15961064470587782438e+00,
        -6.76747652265167261021e+01,
        -3.31231299649172967747e+02,
        -3.46433388365604912451e+02,
    }, pS5[5] = {
        6.07539382692300335975e+01,
        1.05125230595704579173e+03,
        5.97897094333855784498e+03,
        9.62544514357774460223e+03,
        2.40605815922939109441e+03,
    }, pR3[6] = {/* for x in [4.547,2.8571]=1/[0.2199,0.35001] */
        -2.54704601771951915620e-09,
        -7.03119616381481654654e-02,
        -2.40903221549529611423e+00,
        -2.19659774734883086467e+01,
        -5.80791704701737572236e+01,
        -3.14479470594888503854e+01,
    }, pS3[5] = {
        3.58560338055209726349e+01,
        3.61513983050303863820e+02,
        1.19360783792111533330e+03,
        1.12799679856907414432e+03,
        1.73580930813335754692e+02,
    }, pR2[6] = {/* for x in [2.8570,2]=1/[0.3499,0.5] */
        -8.87534333032526411254e-08,
        -7.03030995483624743247e-02,
        -1.45073846780952986357e+00,
        -7.63569613823527770791e+00,
        -1.11931668860356747786e+01,
        -3.23364579351335335033e+00,
    }, pS2[5] = {
        2.22202997532088808441e+01,
        1.36206794218215208048e+02,
        2.70470278658083486789e+02,
        1.53875394208320329881e+02,
        1.46576176948256193810e+01,
    };

    const double *p, *q;
    double z, r, s;
    UINT32 ix;

    ix = *(ULONGLONG*)&x >> 32;
    ix &= 0x7fffffff;
    if (ix >= 0x40200000) {
        p = pR8;
        q = pS8;
    } else if (ix >= 0x40122E8B) {
        p = pR5;
        q = pS5;
    } else if (ix >= 0x4006DB6D) {
        p = pR3;
        q = pS3;
    } else /*ix >= 0x40000000*/ {
        p = pR2;
        q = pS2;
    }

    z = 1.0 / (x * x);
    r = p[0] + z * (p[1] + z * (p[2] + z * (p[3] + z * (p[4] + z * p[5]))));
    s = 1.0 + z * (q[0] + z * (q[1] + z * (q[2] + z * (q[3] + z * q[4]))));
    return 1.0 + r / s;
}

static double qzero(double x)
{
    static const double qR8[6] = { /* for x in [inf, 8]=1/[0,0.125] */
        0.00000000000000000000e+00,
        7.32421874999935051953e-02,
        1.17682064682252693899e+01,
        5.57673380256401856059e+02,
        8.85919720756468632317e+03,
        3.70146267776887834771e+04,
    }, qS8[6] = {
        1.63776026895689824414e+02,
        8.09834494656449805916e+03,
        1.42538291419120476348e+05,
        8.03309257119514397345e+05,
        8.40501579819060512818e+05,
        -3.43899293537866615225e+05,
    }, qR5[6] = { /* for x in [8,4.5454]=1/[0.125,0.22001] */
        1.84085963594515531381e-11,
        7.32421766612684765896e-02,
        5.83563508962056953777e+00,
        1.35111577286449829671e+02,
        1.02724376596164097464e+03,
        1.98997785864605384631e+03,
    }, qS5[6] = {
        8.27766102236537761883e+01,
        2.07781416421392987104e+03,
        1.88472887785718085070e+04,
        5.67511122894947329769e+04,
        3.59767538425114471465e+04,
        -5.35434275601944773371e+03,
    }, qR3[6] = {/* for x in [4.547,2.8571]=1/[0.2199,0.35001] */
        4.37741014089738620906e-09,
        7.32411180042911447163e-02,
        3.34423137516170720929e+00,
        4.26218440745412650017e+01,
        1.70808091340565596283e+02,
        1.66733948696651168575e+02,
    }, qS3[6] = {
        4.87588729724587182091e+01,
        7.09689221056606015736e+02,
        3.70414822620111362994e+03,
        6.46042516752568917582e+03,
        2.51633368920368957333e+03,
        -1.49247451836156386662e+02,
    }, qR2[6] = {/* for x in [2.8570,2]=1/[0.3499,0.5] */
        1.50444444886983272379e-07,
        7.32234265963079278272e-02,
        1.99819174093815998816e+00,
        1.44956029347885735348e+01,
        3.16662317504781540833e+01,
        1.62527075710929267416e+01,
    }, qS2[6] = {
        3.03655848355219184498e+01,
        2.69348118608049844624e+02,
        8.44783757595320139444e+02,
        8.82935845112488550512e+02,
        2.12666388511798828631e+02,
        -5.31095493882666946917e+00,
    };

    const double *p, *q;
    double s, r, z;
    unsigned int ix;

    ix = *(ULONGLONG*)&x >> 32;
    ix &= 0x7fffffff;
    if (ix >= 0x40200000) {
        p = qR8;
        q = qS8;
    } else if (ix >= 0x40122E8B) {
        p = qR5;
        q = qS5;
    } else if (ix >= 0x4006DB6D) {
        p = qR3;
        q = qS3;
    } else /*ix >= 0x40000000*/ {
        p = qR2;
        q = qS2;
    }

    z = 1.0 / (x * x);
    r = p[0] + z * (p[1] + z * (p[2] + z * (p[3] + z * (p[4] + z * p[5]))));
    s = 1.0 + z * (q[0] + z * (q[1] + z * (q[2] + z * (q[3] + z * (q[4] + z * q[5])))));
    return (-0.125 + r / s) / x;
}

/* j0 and y0 approximation for |x|>=2 */
static double j0_y0_approx(unsigned int ix, double x, BOOL y0)
{
    static const double invsqrtpi = 5.64189583547756279280e-01;

    double s, c, ss, cc, z;

    s = sin(x);
    c = cos(x);
    if (y0) c = -c;
    cc = s + c;
    /* avoid overflow in 2*x, big ulp error when x>=0x1p1023 */
    if (ix < 0x7fe00000) {
        ss = s - c;
        z = -cos(2 * x);
        if (s * c < 0) cc = z / ss;
        else ss = z / cc;
        if (ix < 0x48000000) {
            if (y0) ss = -ss;
            cc = pzero(x) * cc - qzero(x) * ss;
        }
    }
    return invsqrtpi * cc / sqrt(x);
}

/*********************************************************************
 *		_j0 (MSVCRT.@)
 *
 * Copied from musl: src/math/j0.c
 */
double CDECL _j0(double x)
{
    static const double R02 =  1.56249999999999947958e-02,
            R03 = -1.89979294238854721751e-04,
            R04 =  1.82954049532700665670e-06,
            R05 = -4.61832688532103189199e-09,
            S01 =  1.56191029464890010492e-02,
            S02 =  1.16926784663337450260e-04,
            S03 =  5.13546550207318111446e-07,
            S04 =  1.16614003333790000205e-09;

    double z, r, s;
    unsigned int ix;

    ix = *(ULONGLONG*)&x >> 32;
    ix &= 0x7fffffff;

    /* j0(+-inf)=0, j0(nan)=nan */
    if (ix >= 0x7ff00000)
        return math_error(_DOMAIN, "_j0", x, 0, 1 / (x * x));
    x = fabs(x);

    if (ix >= 0x40000000) {  /* |x| >= 2 */
        /* large ulp error near zeros: 2.4, 5.52, 8.6537,.. */
        return j0_y0_approx(ix, x, FALSE);
    }

    if (ix >= 0x3f200000) {  /* |x| >= 2**-13 */
        /* up to 4ulp error close to 2 */
        z = x * x;
        r = z * (R02 + z * (R03 + z * (R04 + z * R05)));
        s = 1 + z * (S01 + z * (S02 + z * (S03 + z * S04)));
        return (1 + x / 2) * (1 - x / 2) + z * (r / s);
    }

    /* 1 - x*x/4 */
    /* prevent underflow */
    /* inexact should be raised when x!=0, this is not done correctly */
    if (ix >= 0x38000000)  /* |x| >= 2**-127 */
        x = 0.25 * x * x;
    return 1 - x;
}

static double pone(double x)
{
    static const double pr8[6] = { /* for x in [inf, 8]=1/[0,0.125] */
        0.00000000000000000000e+00,
        1.17187499999988647970e-01,
        1.32394806593073575129e+01,
        4.12051854307378562225e+02,
        3.87474538913960532227e+03,
        7.91447954031891731574e+03,
    }, ps8[5] = {
        1.14207370375678408436e+02,
        3.65093083420853463394e+03,
        3.69562060269033463555e+04,
        9.76027935934950801311e+04,
        3.08042720627888811578e+04,
    }, pr5[6] = { /* for x in [8,4.5454]=1/[0.125,0.22001] */
        1.31990519556243522749e-11,
        1.17187493190614097638e-01,
        6.80275127868432871736e+00,
        1.08308182990189109773e+02,
        5.17636139533199752805e+02,
        5.28715201363337541807e+02,
    }, ps5[5] = {
        5.92805987221131331921e+01,
        9.91401418733614377743e+02,
        5.35326695291487976647e+03,
        7.84469031749551231769e+03,
        1.50404688810361062679e+03,
    }, pr3[6] = {
        3.02503916137373618024e-09,
        1.17186865567253592491e-01,
        3.93297750033315640650e+00,
        3.51194035591636932736e+01,
        9.10550110750781271918e+01,
        4.85590685197364919645e+01,
    }, ps3[5] = {
        3.47913095001251519989e+01,
        3.36762458747825746741e+02,
        1.04687139975775130551e+03,
        8.90811346398256432622e+02,
        1.03787932439639277504e+02,
    }, pr2[6] = { /* for x in [2.8570,2]=1/[0.3499,0.5] */
        1.07710830106873743082e-07,
        1.17176219462683348094e-01,
        2.36851496667608785174e+00,
        1.22426109148261232917e+01,
        1.76939711271687727390e+01,
        5.07352312588818499250e+00,
    }, ps2[5] = {
        2.14364859363821409488e+01,
        1.25290227168402751090e+02,
        2.32276469057162813669e+02,
        1.17679373287147100768e+02,
        8.36463893371618283368e+00,
    };

    const double *p, *q;
    double z, r, s;
    unsigned int ix;

    ix = *(ULONGLONG*)&x >> 32;
    ix &= 0x7fffffff;
    if (ix >= 0x40200000) {
        p = pr8;
        q = ps8;
    } else if (ix >= 0x40122E8B) {
        p = pr5;
        q = ps5;
    } else if (ix >= 0x4006DB6D) {
        p = pr3;
        q = ps3;
    } else /*ix >= 0x40000000*/ {
        p = pr2;
        q = ps2;
    }
    z = 1.0 / (x * x);
    r = p[0] + z * (p[1] + z * (p[2] + z * (p[3] + z * (p[4] + z * p[5]))));
    s = 1.0 + z * (q[0] + z * (q[1] + z * (q[2] + z * (q[3] + z * q[4]))));
    return 1.0 + r / s;
}

static double qone(double x)
{
    static const double qr8[6] = { /* for x in [inf, 8]=1/[0,0.125] */
        0.00000000000000000000e+00,
        -1.02539062499992714161e-01,
        -1.62717534544589987888e+01,
        -7.59601722513950107896e+02,
        -1.18498066702429587167e+04,
        -4.84385124285750353010e+04,
    }, qs8[6] = {
        1.61395369700722909556e+02,
        7.82538599923348465381e+03,
        1.33875336287249578163e+05,
        7.19657723683240939863e+05,
        6.66601232617776375264e+05,
        -2.94490264303834643215e+05,
    }, qr5[6] = { /* for x in [8,4.5454]=1/[0.125,0.22001] */
        -2.08979931141764104297e-11,
        -1.02539050241375426231e-01,
        -8.05644828123936029840e+00,
        -1.83669607474888380239e+02,
        -1.37319376065508163265e+03,
        -2.61244440453215656817e+03,
    }, qs5[6] = {
        8.12765501384335777857e+01,
        1.99179873460485964642e+03,
        1.74684851924908907677e+04,
        4.98514270910352279316e+04,
        2.79480751638918118260e+04,
        -4.71918354795128470869e+03,
    }, qr3[6] = {
        -5.07831226461766561369e-09,
        -1.02537829820837089745e-01,
        -4.61011581139473403113e+00,
        -5.78472216562783643212e+01,
        -2.28244540737631695038e+02,
        -2.19210128478909325622e+02,
    }, qs3[6] = {
        4.76651550323729509273e+01,
        6.73865112676699709482e+02,
        3.38015286679526343505e+03,
        5.54772909720722782367e+03,
        1.90311919338810798763e+03,
        -1.35201191444307340817e+02,
    }, qr2[6] = { /* for x in [2.8570,2]=1/[0.3499,0.5] */
        -1.78381727510958865572e-07,
        -1.02517042607985553460e-01,
        -2.75220568278187460720e+00,
        -1.96636162643703720221e+01,
        -4.23253133372830490089e+01,
        -2.13719211703704061733e+01,
    }, qs2[6] = {
        2.95333629060523854548e+01,
        2.52981549982190529136e+02,
        7.57502834868645436472e+02,
        7.39393205320467245656e+02,
        1.55949003336666123687e+02,
        -4.95949898822628210127e+00,
    };

    const double *p, *q;
    double s, r, z;
    unsigned int ix;

    ix = *(ULONGLONG*)&x >> 32;
    ix &= 0x7fffffff;
    if (ix >= 0x40200000) {
        p = qr8;
        q = qs8;
    } else if (ix >= 0x40122E8B) {
        p = qr5;
        q = qs5;
    } else if (ix >= 0x4006DB6D) {
        p = qr3;
        q = qs3;
    } else /*ix >= 0x40000000*/ {
        p = qr2;
        q = qs2;
    }
    z = 1.0 / (x * x);
    r = p[0] + z * (p[1] + z * (p[2] + z * (p[3] + z * (p[4] + z * p[5]))));
    s = 1.0 + z * (q[0] + z * (q[1] + z * (q[2] + z * (q[3] + z * (q[4] + z * q[5])))));
    return (0.375 + r / s) / x;
}

static double j1_y1_approx(unsigned int ix, double x, BOOL y1, int sign)
{
    static const double invsqrtpi = 5.64189583547756279280e-01;

    double z, s, c, ss, cc;

    s = sin(x);
    if (y1) s = -s;
    c = cos(x);
    cc = s - c;
    if (ix < 0x7fe00000) {
        ss = -s - c;
        z = cos(2 * x);
        if (s * c > 0) cc = z / ss;
        else ss = z / cc;
        if (ix < 0x48000000) {
            if (y1)
                ss = -ss;
            cc = pone(x) * cc - qone(x) * ss;
        }
    }
    if (sign)
        cc = -cc;
    return invsqrtpi * cc / sqrt(x);
}

/*********************************************************************
 *		_j1 (MSVCRT.@)
 *
 * Copied from musl: src/math/j1.c
 */
double CDECL _j1(double x)
{
    static const double r00 = -6.25000000000000000000e-02,
        r01 =  1.40705666955189706048e-03,
        r02 = -1.59955631084035597520e-05,
        r03 =  4.96727999609584448412e-08,
        s01 =  1.91537599538363460805e-02,
        s02 =  1.85946785588630915560e-04,
        s03 =  1.17718464042623683263e-06,
        s04 =  5.04636257076217042715e-09,
        s05 =  1.23542274426137913908e-11;

    double z, r, s;
    unsigned int ix;
    int sign;

    ix = *(ULONGLONG*)&x >> 32;
    sign = ix >> 31;
    ix &= 0x7fffffff;
    if (ix >= 0x7ff00000)
        return math_error(isnan(x) ? 0 : _DOMAIN, "_j1", x, 0, 1 / (x * x));
    if (ix >= 0x40000000)  /* |x| >= 2 */
        return j1_y1_approx(ix, fabs(x), FALSE, sign);
    if (ix >= 0x38000000) {  /* |x| >= 2**-127 */
        z = x * x;
        r = z * (r00 + z * (r01 + z * (r02 + z * r03)));
        s = 1 + z * (s01 + z * (s02 + z * (s03 + z * (s04 + z * s05))));
        z = r / s;
    } else {
        /* avoid underflow, raise inexact if x!=0 */
        z = x;
    }
    return (0.5 + z) * x;
}

/*********************************************************************
 *		_jn (MSVCRT.@)
 *
 * Copied from musl: src/math/jn.c
 */
double CDECL _jn(int n, double x)
{
    static const double invsqrtpi = 5.64189583547756279280e-01;

    unsigned int ix, lx;
    int nm1, i, sign;
    double a, b, temp;

    ix = *(ULONGLONG*)&x >> 32;
    lx = *(ULONGLONG*)&x;
    sign = ix >> 31;
    ix &= 0x7fffffff;

    if ((ix | (lx | -lx) >> 31) > 0x7ff00000) /* nan */
        return x;

    if (n == 0)
        return _j0(x);
    if (n < 0) {
        nm1 = -(n + 1);
        x = -x;
        sign ^= 1;
    } else {
        nm1 = n-1;
    }
    if (nm1 == 0)
        return j1(x);

    sign &= n;  /* even n: 0, odd n: signbit(x) */
    x = fabs(x);
    if ((ix | lx) == 0 || ix == 0x7ff00000)  /* if x is 0 or inf */
        b = 0.0;
    else if (nm1 < x) {
        if (ix >= 0x52d00000) { /* x > 2**302 */
            switch(nm1 & 3) {
            case 0:
                temp = -cos(x) + sin(x);
                break;
            case 1:
                temp = -cos(x) - sin(x);
                break;
            case 2:
                temp =  cos(x) - sin(x);
                break;
            default:
                temp =  cos(x) + sin(x);
                break;
            }
            b = invsqrtpi * temp / sqrt(x);
        } else {
            a = _j0(x);
            b = _j1(x);
            for (i = 0; i < nm1; ) {
                i++;
                temp = b;
                b = b * (2.0 * i / x) - a; /* avoid underflow */
                a = temp;
            }
        }
    } else {
        if (ix < 0x3e100000) { /* x < 2**-29 */
            if (nm1 > 32)  /* underflow */
                b = 0.0;
            else {
                temp = x * 0.5;
                b = temp;
                a = 1.0;
                for (i = 2; i <= nm1 + 1; i++) {
                    a *= (double)i; /* a = n! */
                    b *= temp;      /* b = (x/2)^n */
                }
                b = b / a;
            }
        } else {
            double t, q0, q1, w, h, z, tmp, nf;
            int k;

            nf = nm1 + 1.0;
            w = 2 * nf / x;
            h = 2 / x;
            z = w + h;
            q0 = w;
            q1 = w * z - 1.0;
            k = 1;
            while (q1 < 1.0e9) {
                k += 1;
                z += h;
                tmp = z * q1 - q0;
                q0 = q1;
                q1 = tmp;
            }
            for (t = 0.0, i = k; i >= 0; i--)
                t = 1 / (2 * (i + nf) / x - t);
            a = t;
            b = 1.0;
            tmp = nf * log(fabs(w));
            if (tmp < 7.09782712893383973096e+02) {
                for (i = nm1; i > 0; i--) {
                    temp = b;
                    b = b * (2.0 * i) / x - a;
                    a = temp;
                }
            } else {
                for (i = nm1; i > 0; i--) {
                    temp = b;
                    b = b * (2.0 * i) / x - a;
                    a = temp;
                    /* scale b to avoid spurious overflow */
                    if (b > 0x1p500) {
                        a /= b;
                        t /= b;
                        b  = 1.0;
                    }
                }
            }
            z = j0(x);
            w = j1(x);
            if (fabs(z) >= fabs(w))
                b = t * z / b;
            else
                b = t * w / a;
        }
    }
    return sign ? -b : b;
}

/*********************************************************************
 *		_y0 (MSVCRT.@)
 */
double CDECL _y0(double x)
{
    static const double tpi = 6.36619772367581382433e-01,
        u00  = -7.38042951086872317523e-02,
        u01  =  1.76666452509181115538e-01,
        u02  = -1.38185671945596898896e-02,
        u03  =  3.47453432093683650238e-04,
        u04  = -3.81407053724364161125e-06,
        u05  =  1.95590137035022920206e-08,
        u06  = -3.98205194132103398453e-11,
        v01  =  1.27304834834123699328e-02,
        v02  =  7.60068627350353253702e-05,
        v03  =  2.59150851840457805467e-07,
        v04  =  4.41110311332675467403e-10;

    double z, u, v;
    unsigned int ix, lx;

    ix = *(ULONGLONG*)&x >> 32;
    lx = *(ULONGLONG*)&x;

    /* y0(nan)=nan, y0(<0)=nan, y0(0)=-inf, y0(inf)=0 */
    if ((ix << 1 | lx) == 0)
        return math_error(_OVERFLOW, "_y0", x, 0, -INFINITY);
    if (isnan(x))
        return x;
    if (ix >> 31)
        return math_error(_DOMAIN, "_y0", x, 0, 0 / (x - x));
    if (ix >= 0x7ff00000)
        return 1 / x;

    if (ix >= 0x40000000) {  /* x >= 2 */
        /* large ulp errors near zeros: 3.958, 7.086,.. */
        return j0_y0_approx(ix, x, TRUE);
    }

    if (ix >= 0x3e400000) {  /* x >= 2**-27 */
        /* large ulp error near the first zero, x ~= 0.89 */
        z = x * x;
        u = u00 + z * (u01 + z * (u02 + z * (u03 + z * (u04 + z * (u05 + z * u06)))));
        v = 1.0 + z * (v01 + z * (v02 + z * (v03 + z * v04)));
        return u / v + tpi * (j0(x) * log(x));
    }
    return u00 + tpi * log(x);
}

/*********************************************************************
 *		_y1 (MSVCRT.@)
 */
double CDECL _y1(double x)
{
    static const double tpi = 6.36619772367581382433e-01,
        u00 =  -1.96057090646238940668e-01,
        u01 = 5.04438716639811282616e-02,
        u02 = -1.91256895875763547298e-03,
        u03 = 2.35252600561610495928e-05,
        u04 = -9.19099158039878874504e-08,
        v00 = 1.99167318236649903973e-02,
        v01 = 2.02552581025135171496e-04,
        v02 = 1.35608801097516229404e-06,
        v03 = 6.22741452364621501295e-09,
        v04 = 1.66559246207992079114e-11;

    double z, u, v;
    unsigned int ix, lx;

    ix = *(ULONGLONG*)&x >> 32;
    lx = *(ULONGLONG*)&x;

    /* y1(nan)=nan, y1(<0)=nan, y1(0)=-inf, y1(inf)=0 */
    if ((ix << 1 | lx) == 0)
        return math_error(_OVERFLOW, "_y1", x, 0, -INFINITY);
    if (isnan(x))
        return x;
    if (ix >> 31)
        return math_error(_DOMAIN, "_y1", x, 0, 0 / (x - x));
    if (ix >= 0x7ff00000)
        return 1 / x;

    if (ix >= 0x40000000)  /* x >= 2 */
        return j1_y1_approx(ix, x, TRUE, 0);
    if (ix < 0x3c900000)  /* x < 2**-54 */
        return -tpi / x;
    z = x * x;
    u = u00 + z * (u01 + z * (u02 + z * (u03 + z * u04)));
    v = 1 + z * (v00 + z * (v01 + z * (v02 + z * (v03 + z * v04))));
    return x * (u / v) + tpi * (j1(x) * log(x) - 1 / x);
}

/*********************************************************************
 *		_yn (MSVCRT.@)
 *
 * Copied from musl: src/math/jn.c
 */
double CDECL _yn(int n, double x)
{
    static const double invsqrtpi = 5.64189583547756279280e-01;

    unsigned int ix, lx, ib;
    int nm1, sign, i;
    double a, b, temp;

    ix = *(ULONGLONG*)&x >> 32;
    lx = *(ULONGLONG*)&x;
    sign = ix >> 31;
    ix &= 0x7fffffff;

    if ((ix | (lx | -lx) >> 31) > 0x7ff00000) /* nan */
        return x;
    if (sign && (ix | lx) != 0) /* x < 0 */
        return math_error(_DOMAIN, "_y1", x, 0, 0 / (x - x));
    if (ix == 0x7ff00000)
        return 0.0;

    if (n == 0)
        return y0(x);
    if (n < 0) {
        nm1 = -(n + 1);
        sign = n & 1;
    } else {
        nm1 = n - 1;
        sign = 0;
    }
    if (nm1 == 0)
        return sign ? -y1(x) : y1(x);

    if (ix >= 0x52d00000) { /* x > 2**302 */
        switch(nm1 & 3) {
        case 0:
            temp = -sin(x) - cos(x);
            break;
        case 1:
            temp = -sin(x) + cos(x);
            break;
        case 2:
            temp = sin(x) + cos(x);
            break;
        default:
            temp = sin(x) - cos(x);
            break;
        }
        b = invsqrtpi * temp / sqrt(x);
    } else {
        a = y0(x);
        b = y1(x);
        /* quit if b is -inf */
        ib = *(ULONGLONG*)&b >> 32;
        for (i = 0; i < nm1 && ib != 0xfff00000;) {
            i++;
            temp = b;
            b = (2.0 * i / x) * b - a;
            ib = *(ULONGLONG*)&b >> 32;
            a = temp;
        }
    }
    return sign ? -b : b;
}

#if _MSVCR_VER>=120

/*********************************************************************
 *		_nearbyint (MSVCR120.@)
 *
 * Based on musl: src/math/nearbyteint.c
 */
double CDECL nearbyint(double x)
{
    fenv_t env;

    fegetenv(&env);
    _control87(_MCW_EM, _MCW_EM);
    x = rint(x);
    feclearexcept(FE_INEXACT);
    feupdateenv(&env);
    return x;
}

/*********************************************************************
 *		_nearbyintf (MSVCR120.@)
 *
 * Based on musl: src/math/nearbyteintf.c
 */
float CDECL nearbyintf(float x)
{
    fenv_t env;

    fegetenv(&env);
    _control87(_MCW_EM, _MCW_EM);
    x = rintf(x);
    feclearexcept(FE_INEXACT);
    feupdateenv(&env);
    return x;
}

/*********************************************************************
 *              nexttoward (MSVCR120.@)
 */
double CDECL MSVCRT_nexttoward(double num, double next)
{
    return _nextafter(num, next);
}

/*********************************************************************
 *              nexttowardf (MSVCR120.@)
 *
 * Copied from musl: src/math/nexttowardf.c
 */
float CDECL MSVCRT_nexttowardf(float x, double y)
{
    unsigned int ix = *(unsigned int*)&x;
    unsigned int e;
    float ret;

    if (isnan(x) || isnan(y))
        return x + y;
    if (x == y)
        return y;
    if (x == 0) {
        ix = 1;
        if (signbit(y))
            ix |= 0x80000000;
    } else if (x < y) {
        if (signbit(x))
            ix--;
        else
            ix++;
    } else {
        if (signbit(x))
            ix++;
        else
            ix--;
    }
    e = ix & 0x7f800000;
    /* raise overflow if ix is infinite and x is finite */
    if (e == 0x7f800000) {
        fp_barrierf(x + x);
        *_errno() = ERANGE;
    }
    ret = *(float*)&ix;
    /* raise underflow if ret is subnormal or zero */
    if (e == 0) {
        fp_barrierf(x * x + ret * ret);
        *_errno() = ERANGE;
    }
    return ret;
}

#endif /* _MSVCR_VER>=120 */

/*********************************************************************
 *		_nextafter (MSVCRT.@)
 *
 * Copied from musl: src/math/nextafter.c
 */
double CDECL _nextafter(double x, double y)
{
    ULONGLONG llx = *(ULONGLONG*)&x;
    ULONGLONG lly = *(ULONGLONG*)&y;
    ULONGLONG ax, ay;
    int e;

    if (isnan(x) || isnan(y))
        return x + y;
    if (llx == lly) {
        if (_fpclass(y) & (_FPCLASS_ND | _FPCLASS_PD | _FPCLASS_NZ | _FPCLASS_PZ ))
            *_errno() = ERANGE;
        return y;
    }
    ax = llx & -1ULL / 2;
    ay = lly & -1ULL / 2;
    if (ax == 0) {
        if (ay == 0)
            return y;
        llx = (lly & 1ULL << 63) | 1;
    } else if (ax > ay || ((llx ^ lly) & 1ULL << 63))
        llx--;
    else
        llx++;
    e = llx >> 52 & 0x7ff;
    /* raise overflow if llx is infinite and x is finite */
    if (e == 0x7ff) {
        fp_barrier(x + x);
        *_errno() = ERANGE;
    }
    /* raise underflow if llx is subnormal or zero */
    y = *(double*)&llx;
    if (e == 0) {
        fp_barrier(x * x + y * y);
        *_errno() = ERANGE;
    }
    return y;
}

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
 *      cbrt (MSVCR120.@)
 *
 * Copied from musl: src/math/cbrt.c
 */
double CDECL cbrt(double x)
{
    static const UINT32 B1 = 715094163, B2 = 696219795;
    static const double P0 =  1.87595182427177009643,
                 P1 = -1.88497979543377169875,
                 P2 =  1.621429720105354466140,
                 P3 = -0.758397934778766047437,
                 P4 =  0.145996192886612446982;

    union {double f; UINT64 i;} u = {x};
    double r,s,t,w;
    UINT32 hx = u.i >> 32 & 0x7fffffff;

    if (hx >= 0x7ff00000)  /* cbrt(NaN,INF) is itself */
        return x + x;

    if (hx < 0x00100000) { /* zero or subnormal? */
        u.f = x * 0x1p54;
        hx = u.i>>32 & 0x7fffffff;
        if (hx == 0)
            return x;
        hx = hx / 3 + B2;
    } else
        hx = hx / 3 + B1;
    u.i &= 1ULL << 63;
    u.i |= (UINT64)hx << 32;
    t = u.f;

    r = (t * t) * (t / x);
    t = t * ((P0 + r * (P1 + r * P2)) + ((r * r) * r) * (P3 + r * P4));

    u.f = t;
    u.i = (u.i + 0x80000000) & 0xffffffffc0000000ULL;
    t = u.f;

    s = t * t;
    r = x / s;
    w = t + t;
    r = (r - t) / (w + r);
    t = t + t * r;
    return t;
}

/*********************************************************************
 *      cbrtf (MSVCR120.@)
 *
 * Copied from musl: src/math/cbrtf.c
 */
float CDECL cbrtf(float x)
{
    static const unsigned B1 = 709958130, B2 = 642849266;

    double r,T;
    union {float f; UINT32 i;} u = {x};
    UINT32 hx = u.i & 0x7fffffff;

    if (hx >= 0x7f800000)
        return x + x;

    if (hx < 0x00800000) {  /* zero or subnormal? */
        if (hx == 0)
            return x;
        u.f = x * 0x1p24f;
        hx = u.i & 0x7fffffff;
        hx = hx / 3 + B2;
    } else
        hx = hx / 3 + B1;
    u.i &= 0x80000000;
    u.i |= hx;

    T = u.f;
    r = T * T * T;
    T = T * (x + x + r) / (x + r + r);

    r = T * T * T;
    T = T * (x + x + r) / (x + r + r);
    return T;
}

/*********************************************************************
 *      exp2 (MSVCR120.@)
 *
 * Copied from musl: src/math/exp2.c
 */
double CDECL exp2(double x)
{
    static const double C[] = {
        0x1.62e42fefa39efp-1,
        0x1.ebfbdff82c424p-3,
        0x1.c6b08d70cf4b5p-5,
        0x1.3b2abd24650ccp-7,
        0x1.5d7e09b4e3a84p-10
    };

    UINT32 abstop;
    UINT64 ki, idx, top, sbits;
    double kd, r, r2, scale, tail, tmp;

    abstop = (*(UINT64*)&x >> 52) & 0x7ff;
    if (abstop - 0x3c9 >= 0x408 - 0x3c9) {
        if (abstop - 0x3c9 >= 0x80000000) {
            /* Avoid spurious underflow for tiny x. */
            /* Note: 0 is common input. */
            return 1.0 + x;
        }
        if (abstop >= 409) {
            if (*(UINT64*)&x == 0xfff0000000000000ull)
                return 0.0;
            if (abstop >= 0x7ff)
                return 1.0 + x;
            if (!(*(UINT64*)&x >> 63)) {
                *_errno() = ERANGE;
                return fp_barrier(DBL_MAX) * DBL_MAX;
            }
            else if (x <= -2147483648.0) {
                fp_barrier(x + 0x1p120f);
                return 0;
            }
            else if (*(UINT64*)&x >= 0xc090cc0000000000ull) {
                *_errno() = ERANGE;
                fp_barrier(x + 0x1p120f);
                return 0;
            }
        }
        if (2 * *(UINT64*)&x > 2 * 0x408d000000000000ull)
            /* Large x is special cased below. */
            abstop = 0;
    }

    /* exp2(x) = 2^(k/N) * 2^r, with 2^r in [2^(-1/2N),2^(1/2N)]. */
    /* x = k/N + r, with int k and r in [-1/2N, 1/2N]. */
    kd = fp_barrier(x + 0x1.8p52 / (1 << 7));
    ki = *(UINT64*)&kd; /* k. */
    kd -= 0x1.8p52 / (1 << 7); /* k/N for int k. */
    r = x - kd;
    /* 2^(k/N) ~= scale * (1 + tail). */
    idx = 2 * (ki % (1 << 7));
    top = ki << (52 - 7);
    tail = *(double*)&exp_T[idx];
    /* This is only a valid scale when -1023*N < k < 1024*N. */
    sbits = exp_T[idx + 1] + top;
    /* exp2(x) = 2^(k/N) * 2^r ~= scale + scale * (tail + 2^r - 1). */
    /* Evaluation is optimized assuming superscalar pipelined execution. */
    r2 = r * r;
    /* Without fma the worst case error is 0.5/N ulp larger. */
    /* Worst case error is less than 0.5+0.86/N+(abs poly error * 2^53) ulp. */
    tmp = tail + r * C[0] + r2 * (C[1] + r * C[2]) + r2 * r2 * (C[3] + r * C[4]);
    if (abstop == 0)
    {
        /* Handle cases that may overflow or underflow when computing the result that
           is scale*(1+TMP) without intermediate rounding. The bit representation of
           scale is in SBITS, however it has a computed exponent that may have
           overflown into the sign bit so that needs to be adjusted before using it as
           a double. (int32_t)KI is the k used in the argument reduction and exponent
           adjustment of scale, positive k here means the result may overflow and
           negative k means the result may underflow. */
        double scale, y;

        if ((ki & 0x80000000) == 0) {
            /* k > 0, the exponent of scale might have overflowed by 1. */
            sbits -= 1ull << 52;
            scale = *(double*)&sbits;
            y = 2 * (scale + scale * tmp);
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
        }
        y = 0x1p-1022 * y;
        return y;
    }
    scale = *(double*)&sbits;
    /* Note: tmp == 0 or |tmp| > 2^-65 and scale > 2^-928, so there
       is no spurious underflow here even without fma. */
    return scale + scale * tmp;
}

/*********************************************************************
 *      exp2f (MSVCR120.@)
 *
 * Copied from musl: src/math/exp2f.c
 */
float CDECL exp2f(float x)
{
    static const double C[] = {
        0x1.c6af84b912394p-5, 0x1.ebfce50fac4f3p-3, 0x1.62e42ff0c52d6p-1
    };
    static const double shift = 0x1.8p+52 / (1 << 5);

    double kd, xd, z, r, r2, y, s;
    UINT32 abstop;
    UINT64 ki, t;

    xd = x;
    abstop = (*(UINT32*)&x >> 20) & 0x7ff;
    if (abstop >= 0x430) {
        /* |x| >= 128 or x is nan.  */
        if (*(UINT32*)&x == 0xff800000)
            return 0.0f;
        if (abstop >= 0x7f8)
            return x + x;
        if (x > 0.0f) {
            *_errno() = ERANGE;
            return fp_barrierf(x * FLT_MAX);
        }
        if (x <= -150.0f) {
            fp_barrierf(x - 0x1p120);
            return 0;
        }
    }

    /* x = k/N + r with r in [-1/(2N), 1/(2N)] and int k, N = 1 << 5. */
    kd = xd + shift;
    ki = *(UINT64*)&kd;
    kd -= shift; /* k/(1<<5) for int k.  */
    r = xd - kd;

    /* exp2(x) = 2^(k/N) * 2^r ~= s * (C0*r^3 + C1*r^2 + C2*r + 1) */
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

/*********************************************************************
 *      expm1 (MSVCR120.@)
 */
double CDECL expm1(double x)
{
    return __expm1(x);
}

/*********************************************************************
 *      expm1f (MSVCR120.@)
 */
float CDECL expm1f(float x)
{
    return __expm1f(x);
}

/*********************************************************************
 *      log1p (MSVCR120.@)
 *
 * Copied from musl: src/math/log1p.c
 */
double CDECL log1p(double x)
{
    static const double ln2_hi = 6.93147180369123816490e-01,
        ln2_lo = 1.90821492927058770002e-10,
        Lg1 = 6.666666666666735130e-01,
        Lg2 = 3.999999999940941908e-01,
        Lg3 = 2.857142874366239149e-01,
        Lg4 = 2.222219843214978396e-01,
        Lg5 = 1.818357216161805012e-01,
        Lg6 = 1.531383769920937332e-01,
        Lg7 = 1.479819860511658591e-01;

    union {double f; UINT64 i;} u = {x};
    double hfsq, f, c, s, z, R, w, t1, t2, dk;
    UINT32 hx, hu;
    int k;

    hx = u.i >> 32;
    k = 1;
    if (hx < 0x3fda827a || hx >> 31) { /* 1+x < sqrt(2)+ */
        if (hx >= 0xbff00000) { /* x <= -1.0 */
            if (x == -1) {
                *_errno() = ERANGE;
                return x / 0.0; /* og1p(-1) = -inf */
            }
            *_errno() = EDOM;
            return (x-x) / 0.0; /* log1p(x<-1) = NaN */
        }
        if (hx << 1 < 0x3ca00000 << 1) { /* |x| < 2**-53 */
            fp_barrier(x + 0x1p120f);
            /* underflow if subnormal */
            if ((hx & 0x7ff00000) == 0)
                fp_barrierf(x);
            return x;
        }
        if (hx <= 0xbfd2bec4) { /* sqrt(2)/2- <= 1+x < sqrt(2)+ */
            k = 0;
            c = 0;
            f = x;
        }
    } else if (hx >= 0x7ff00000)
        return x;
    if (k) {
        u.f = 1 + x;
        hu = u.i >> 32;
        hu += 0x3ff00000 - 0x3fe6a09e;
        k = (int)(hu >> 20) - 0x3ff;
        /* correction term ~ log(1+x)-log(u), avoid underflow in c/u */
        if (k < 54) {
            c = k >= 2 ? 1 - (u.f - x) : x - (u.f - 1);
            c /= u.f;
        } else
            c = 0;
        /* reduce u into [sqrt(2)/2, sqrt(2)] */
        hu = (hu & 0x000fffff) + 0x3fe6a09e;
        u.i = (UINT64)hu << 32 | (u.i & 0xffffffff);
        f = u.f - 1;
    }
    hfsq = 0.5 * f * f;
    s = f / (2.0 + f);
    z = s * s;
    w = z * z;
    t1 = w * (Lg2 + w * (Lg4 + w * Lg6));
    t2 = z * (Lg1 + w * (Lg3 + w * (Lg5 + w * Lg7)));
    R = t2 + t1;
    dk = k;
    return s * (hfsq + R) + (dk * ln2_lo + c) - hfsq + f + dk * ln2_hi;
}

/*********************************************************************
 *      log1pf (MSVCR120.@)
 *
 * Copied from musl: src/math/log1pf.c
 */
float CDECL log1pf(float x)
{
    static const float ln2_hi = 6.9313812256e-01,
        ln2_lo = 9.0580006145e-06,
        Lg1 = 0xaaaaaa.0p-24,
        Lg2 = 0xccce13.0p-25,
        Lg3 = 0x91e9ee.0p-25,
        Lg4 = 0xf89e26.0p-26;

    union {float f; UINT32 i;} u = {x};
    float hfsq, f, c, s, z, R, w, t1, t2, dk;
    UINT32 ix, iu;
    int k;

    ix = u.i;
    k = 1;
    if (ix < 0x3ed413d0 || ix >> 31) { /* 1+x < sqrt(2)+ */
        if (ix >= 0xbf800000) { /* x <= -1.0 */
            if (x == -1) {
                *_errno() = ERANGE;
                return x / 0.0f; /* log1p(-1)=+inf */
            }
            *_errno() = EDOM;
            return (x - x) / 0.0f; /* log1p(x<-1)=NaN */
        }
        if (ix<<1 < 0x33800000<<1) { /* |x| < 2**-24 */
            /* underflow if subnormal */
            if ((ix & 0x7f800000) == 0)
                fp_barrierf(x * x);
            return x;
        }
        if (ix <= 0xbe95f619) { /* sqrt(2)/2- <= 1+x < sqrt(2)+ */
            k = 0;
            c = 0;
            f = x;
        }
    } else if (ix >= 0x7f800000)
        return x;
    if (k) {
        u.f = 1 + x;
        iu = u.i;
        iu += 0x3f800000 - 0x3f3504f3;
        k = (int)(iu >> 23) - 0x7f;
        /* correction term ~ log(1+x)-log(u), avoid underflow in c/u */
        if (k < 25) {
            c = k >= 2 ? 1 - (u.f - x) : x - (u.f - 1);
            c /= u.f;
        } else
            c = 0;
        /* reduce u into [sqrt(2)/2, sqrt(2)] */
        iu = (iu & 0x007fffff) + 0x3f3504f3;
        u.i = iu;
        f = u.f - 1;
    }
    s = f / (2.0f + f);
    z = s * s;
    w = z * z;
    t1= w * (Lg2 + w * Lg4);
    t2= z * (Lg1 + w * Lg3);
    R = t2 + t1;
    hfsq = 0.5f * f * f;
    dk = k;
    return s * (hfsq + R) + (dk * ln2_lo + c) - hfsq + f + dk * ln2_hi;
}

/*********************************************************************
 *      log2 (MSVCR120.@)
 *
 * Copied from musl: src/math/log2.c
 */
double CDECL log2(double x)
{
    static const double invln2hi = 0x1.7154765200000p+0,
        invln2lo = 0x1.705fc2eefa200p-33;
    static const double A[] = {
        -0x1.71547652b8339p-1,
        0x1.ec709dc3a04bep-2,
        -0x1.7154764702ffbp-2,
        0x1.2776c50034c48p-2,
        -0x1.ec7b328ea92bcp-3,
        0x1.a6225e117f92ep-3
    };
    static const double B[] = {
        -0x1.71547652b82fep-1,
        0x1.ec709dc3a03f7p-2,
        -0x1.71547652b7c3fp-2,
        0x1.2776c50f05be4p-2,
        -0x1.ec709dd768fe5p-3,
        0x1.a61761ec4e736p-3,
        -0x1.7153fbc64a79bp-3,
        0x1.484d154f01b4ap-3,
        -0x1.289e4a72c383cp-3,
        0x1.0b32f285aee66p-3
    };
    static const struct {
        double invc, logc;
    } T[] = {
        {0x1.724286bb1acf8p+0, -0x1.1095feecdb000p-1},
        {0x1.6e1f766d2cca1p+0, -0x1.08494bd76d000p-1},
        {0x1.6a13d0e30d48ap+0, -0x1.00143aee8f800p-1},
        {0x1.661ec32d06c85p+0, -0x1.efec5360b4000p-2},
        {0x1.623fa951198f8p+0, -0x1.dfdd91ab7e000p-2},
        {0x1.5e75ba4cf026cp+0, -0x1.cffae0cc79000p-2},
        {0x1.5ac055a214fb8p+0, -0x1.c043811fda000p-2},
        {0x1.571ed0f166e1ep+0, -0x1.b0b67323ae000p-2},
        {0x1.53909590bf835p+0, -0x1.a152f5a2db000p-2},
        {0x1.5014fed61adddp+0, -0x1.9217f5af86000p-2},
        {0x1.4cab88e487bd0p+0, -0x1.8304db0719000p-2},
        {0x1.49539b4334feep+0, -0x1.74189f9a9e000p-2},
        {0x1.460cbdfafd569p+0, -0x1.6552bb5199000p-2},
        {0x1.42d664ee4b953p+0, -0x1.56b23a29b1000p-2},
        {0x1.3fb01111dd8a6p+0, -0x1.483650f5fa000p-2},
        {0x1.3c995b70c5836p+0, -0x1.39de937f6a000p-2},
        {0x1.3991c4ab6fd4ap+0, -0x1.2baa1538d6000p-2},
        {0x1.3698e0ce099b5p+0, -0x1.1d98340ca4000p-2},
        {0x1.33ae48213e7b2p+0, -0x1.0fa853a40e000p-2},
        {0x1.30d191985bdb1p+0, -0x1.01d9c32e73000p-2},
        {0x1.2e025cab271d7p+0, -0x1.e857da2fa6000p-3},
        {0x1.2b404cf13cd82p+0, -0x1.cd3c8633d8000p-3},
        {0x1.288b02c7ccb50p+0, -0x1.b26034c14a000p-3},
        {0x1.25e2263944de5p+0, -0x1.97c1c2f4fe000p-3},
        {0x1.234563d8615b1p+0, -0x1.7d6023f800000p-3},
        {0x1.20b46e33eaf38p+0, -0x1.633a71a05e000p-3},
        {0x1.1e2eefdcda3ddp+0, -0x1.494f5e9570000p-3},
        {0x1.1bb4a580b3930p+0, -0x1.2f9e424e0a000p-3},
        {0x1.19453847f2200p+0, -0x1.162595afdc000p-3},
        {0x1.16e06c0d5d73cp+0, -0x1.f9c9a75bd8000p-4},
        {0x1.1485f47b7e4c2p+0, -0x1.c7b575bf9c000p-4},
        {0x1.12358ad0085d1p+0, -0x1.960c60ff48000p-4},
        {0x1.0fef00f532227p+0, -0x1.64ce247b60000p-4},
        {0x1.0db2077d03a8fp+0, -0x1.33f78b2014000p-4},
        {0x1.0b7e6d65980d9p+0, -0x1.0387d1a42c000p-4},
        {0x1.0953efe7b408dp+0, -0x1.a6f9208b50000p-5},
        {0x1.07325cac53b83p+0, -0x1.47a954f770000p-5},
        {0x1.05197e40d1b5cp+0, -0x1.d23a8c50c0000p-6},
        {0x1.03091c1208ea2p+0, -0x1.16a2629780000p-6},
        {0x1.0101025b37e21p+0, -0x1.720f8d8e80000p-8},
        {0x1.fc07ef9caa76bp-1, 0x1.6fe53b1500000p-7},
        {0x1.f4465d3f6f184p-1, 0x1.11ccce10f8000p-5},
        {0x1.ecc079f84107fp-1, 0x1.c4dfc8c8b8000p-5},
        {0x1.e573a99975ae8p-1, 0x1.3aa321e574000p-4},
        {0x1.de5d6f0bd3de6p-1, 0x1.918a0d08b8000p-4},
        {0x1.d77b681ff38b3p-1, 0x1.e72e9da044000p-4},
        {0x1.d0cb5724de943p-1, 0x1.1dcd2507f6000p-3},
        {0x1.ca4b2dc0e7563p-1, 0x1.476ab03dea000p-3},
        {0x1.c3f8ee8d6cb51p-1, 0x1.7074377e22000p-3},
        {0x1.bdd2b4f020c4cp-1, 0x1.98ede8ba94000p-3},
        {0x1.b7d6c006015cap-1, 0x1.c0db86ad2e000p-3},
        {0x1.b20366e2e338fp-1, 0x1.e840aafcee000p-3},
        {0x1.ac57026295039p-1, 0x1.0790ab4678000p-2},
        {0x1.a6d01bc2731ddp-1, 0x1.1ac056801c000p-2},
        {0x1.a16d3bc3ff18bp-1, 0x1.2db11d4fee000p-2},
        {0x1.9c2d14967feadp-1, 0x1.406464ec58000p-2},
        {0x1.970e4f47c9902p-1, 0x1.52dbe093af000p-2},
        {0x1.920fb3982bcf2p-1, 0x1.651902050d000p-2},
        {0x1.8d30187f759f1p-1, 0x1.771d2cdeaf000p-2},
        {0x1.886e5ebb9f66dp-1, 0x1.88e9c857d9000p-2},
        {0x1.83c97b658b994p-1, 0x1.9a80155e16000p-2},
        {0x1.7f405ffc61022p-1, 0x1.abe186ed3d000p-2},
        {0x1.7ad22181415cap-1, 0x1.bd0f2aea0e000p-2},
        {0x1.767dcf99eff8cp-1, 0x1.ce0a43dbf4000p-2}
    };
    static const struct {
        double chi, clo;
    } T2[] = {
        {0x1.6200012b90a8ep-1, 0x1.904ab0644b605p-55},
        {0x1.66000045734a6p-1, 0x1.1ff9bea62f7a9p-57},
        {0x1.69fffc325f2c5p-1, 0x1.27ecfcb3c90bap-55},
        {0x1.6e00038b95a04p-1, 0x1.8ff8856739326p-55},
        {0x1.71fffe09994e3p-1, 0x1.afd40275f82b1p-55},
        {0x1.7600015590e1p-1, -0x1.2fd75b4238341p-56},
        {0x1.7a00012655bd5p-1, 0x1.808e67c242b76p-56},
        {0x1.7e0003259e9a6p-1, -0x1.208e426f622b7p-57},
        {0x1.81fffedb4b2d2p-1, -0x1.402461ea5c92fp-55},
        {0x1.860002dfafcc3p-1, 0x1.df7f4a2f29a1fp-57},
        {0x1.89ffff78c6b5p-1, -0x1.e0453094995fdp-55},
        {0x1.8e00039671566p-1, -0x1.a04f3bec77b45p-55},
        {0x1.91fffe2bf1745p-1, -0x1.7fa34400e203cp-56},
        {0x1.95fffcc5c9fd1p-1, -0x1.6ff8005a0695dp-56},
        {0x1.9a0003bba4767p-1, 0x1.0f8c4c4ec7e03p-56},
        {0x1.9dfffe7b92da5p-1, 0x1.e7fd9478c4602p-55},
        {0x1.a1fffd72efdafp-1, -0x1.a0c554dcdae7ep-57},
        {0x1.a5fffde04ff95p-1, 0x1.67da98ce9b26bp-55},
        {0x1.a9fffca5e8d2bp-1, -0x1.284c9b54c13dep-55},
        {0x1.adfffddad03eap-1, 0x1.812c8ea602e3cp-58},
        {0x1.b1ffff10d3d4dp-1, -0x1.efaddad27789cp-55},
        {0x1.b5fffce21165ap-1, 0x1.3cb1719c61237p-58},
        {0x1.b9fffd950e674p-1, 0x1.3f7d94194cep-56},
        {0x1.be000139ca8afp-1, 0x1.50ac4215d9bcp-56},
        {0x1.c20005b46df99p-1, 0x1.beea653e9c1c9p-57},
        {0x1.c600040b9f7aep-1, -0x1.c079f274a70d6p-56},
        {0x1.ca0006255fd8ap-1, -0x1.a0b4076e84c1fp-56},
        {0x1.cdfffd94c095dp-1, 0x1.8f933f99ab5d7p-55},
        {0x1.d1ffff975d6cfp-1, -0x1.82c08665fe1bep-58},
        {0x1.d5fffa2561c93p-1, -0x1.b04289bd295f3p-56},
        {0x1.d9fff9d228b0cp-1, 0x1.70251340fa236p-55},
        {0x1.de00065bc7e16p-1, -0x1.5011e16a4d80cp-56},
        {0x1.e200002f64791p-1, 0x1.9802f09ef62ep-55},
        {0x1.e600057d7a6d8p-1, -0x1.e0b75580cf7fap-56},
        {0x1.ea00027edc00cp-1, -0x1.c848309459811p-55},
        {0x1.ee0006cf5cb7cp-1, -0x1.f8027951576f4p-55},
        {0x1.f2000782b7dccp-1, -0x1.f81d97274538fp-55},
        {0x1.f6000260c450ap-1, -0x1.071002727ffdcp-59},
        {0x1.f9fffe88cd533p-1, -0x1.81bdce1fda8bp-58},
        {0x1.fdfffd50f8689p-1, 0x1.7f91acb918e6ep-55},
        {0x1.0200004292367p+0, 0x1.b7ff365324681p-54},
        {0x1.05fffe3e3d668p+0, 0x1.6fa08ddae957bp-55},
        {0x1.0a0000a85a757p+0, -0x1.7e2de80d3fb91p-58},
        {0x1.0e0001a5f3fccp+0, -0x1.1823305c5f014p-54},
        {0x1.11ffff8afbaf5p+0, -0x1.bfabb6680bac2p-55},
        {0x1.15fffe54d91adp+0, -0x1.d7f121737e7efp-54},
        {0x1.1a00011ac36e1p+0, 0x1.c000a0516f5ffp-54},
        {0x1.1e00019c84248p+0, -0x1.082fbe4da5dap-54},
        {0x1.220000ffe5e6ep+0, -0x1.8fdd04c9cfb43p-55},
        {0x1.26000269fd891p+0, 0x1.cfe2a7994d182p-55},
        {0x1.2a00029a6e6dap+0, -0x1.00273715e8bc5p-56},
        {0x1.2dfffe0293e39p+0, 0x1.b7c39dab2a6f9p-54},
        {0x1.31ffff7dcf082p+0, 0x1.df1336edc5254p-56},
        {0x1.35ffff05a8b6p+0, -0x1.e03564ccd31ebp-54},
        {0x1.3a0002e0eaeccp+0, 0x1.5f0e74bd3a477p-56},
        {0x1.3e000043bb236p+0, 0x1.c7dcb149d8833p-54},
        {0x1.4200002d187ffp+0, 0x1.e08afcf2d3d28p-56},
        {0x1.460000d387cb1p+0, 0x1.20837856599a6p-55},
        {0x1.4a00004569f89p+0, -0x1.9fa5c904fbcd2p-55},
        {0x1.4e000043543f3p+0, -0x1.81125ed175329p-56},
        {0x1.51fffcc027f0fp+0, 0x1.883d8847754dcp-54},
        {0x1.55ffffd87b36fp+0, -0x1.709e731d02807p-55},
        {0x1.59ffff21df7bap+0, 0x1.7f79f68727b02p-55},
        {0x1.5dfffebfc3481p+0, -0x1.180902e30e93ep-54}
    };

    double z, r, r2, r4, y, invc, logc, kd, hi, lo, t1, t2, t3, p, rhi, rlo;
    UINT64 ix, iz, tmp;
    UINT32 top;
    int k, i;

    ix = *(UINT64*)&x;
    top = ix >> 48;
    if (ix - 0x3feea4af00000000ULL < 0x210aa00000000ULL) {
        /* Handle close to 1.0 inputs separately.  */
        /* Fix sign of zero with downward rounding when x==1.  */
        if (ix == 0x3ff0000000000000ULL)
            return 0;
        r = x - 1.0;
        *(UINT64*)&rhi = *(UINT64*)&r & -1ULL << 32;
        rlo = r - rhi;
        hi = rhi * invln2hi;
        lo = rlo * invln2hi + r * invln2lo;
        r2 = r * r; /* rounding error: 0x1p-62.  */
        r4 = r2 * r2;
        /* Worst-case error is less than 0.54 ULP (0.55 ULP without fma).  */
        p = r2 * (B[0] + r * B[1]);
        y = hi + p;
        lo += hi - y + p;
        lo += r4 * (B[2] + r * B[3] + r2 * (B[4] + r * B[5]) +
                r4 * (B[6] + r * B[7] + r2 * (B[8] + r * B[9])));
        y += lo;
        return y;
    }
    if (top - 0x0010 >= 0x7ff0 - 0x0010) {
        /* x < 0x1p-1022 or inf or nan.  */
        if (ix * 2 == 0) {
            *_errno() = ERANGE;
            return -1.0 / x;
        }
        if (ix == 0x7ff0000000000000ULL) /* log(inf) == inf.  */
            return x;
        if ((top & 0x7ff0) == 0x7ff0 && (ix & 0xfffffffffffffULL))
            return x;
        if (top & 0x8000) {
            *_errno() = EDOM;
            return (x - x) / (x - x);
        }
        /* x is subnormal, normalize it.  */
        x *= 0x1p52;
        ix = *(UINT64*)&x;
        ix -= 52ULL << 52;
    }

    /* x = 2^k z; where z is in range [OFF,2*OFF) and exact.
       The range is split into N subintervals.
       The ith subinterval contains z and c is near its center.  */
    tmp = ix - 0x3fe6000000000000ULL;
    i = (tmp >> (52 - 6)) % (1 << 6);
    k = (INT64)tmp >> 52; /* arithmetic shift */
    iz = ix - (tmp & 0xfffULL << 52);
    invc = T[i].invc;
    logc = T[i].logc;
    z = *(double*)&iz;
    kd = k;

    /* log2(x) = log2(z/c) + log2(c) + k.  */
    /* r ~= z/c - 1, |r| < 1/(2*N).  */
    /* rounding error: 0x1p-55/N + 0x1p-65.  */
    r = (z - T2[i].chi - T2[i].clo) * invc;
    *(UINT64*)&rhi = *(UINT64*)&r & -1ULL << 32;
    rlo = r - rhi;
    t1 = rhi * invln2hi;
    t2 = rlo * invln2hi + r * invln2lo;

    /* hi + lo = r/ln2 + log2(c) + k.  */
    t3 = kd + logc;
    hi = t3 + t1;
    lo = t3 - hi + t1 + t2;

    /* log2(r+1) = r/ln2 + r^2*poly(r).  */
    /* Evaluation is optimized assuming superscalar pipelined execution.  */
    r2 = r * r; /* rounding error: 0x1p-54/N^2.  */
    r4 = r2 * r2;
    /* Worst-case error if |y| > 0x1p-4: 0.547 ULP (0.550 ULP without fma).
       ~ 0.5 + 2/N/ln2 + abs-poly-error*0x1p56 ULP (+ 0.003 ULP without fma).  */
    p = A[0] + r * A[1] + r2 * (A[2] + r * A[3]) + r4 * (A[4] + r * A[5]);
    y = lo + r2 * p + hi;
    return y;
}

/*********************************************************************
 *      log2f (MSVCR120.@)
 *
 * Copied from musl: src/math/log2f.c
 */
float CDECL log2f(float x)
{
    static const double A[] = {
        -0x1.712b6f70a7e4dp-2,
        0x1.ecabf496832ep-2,
        -0x1.715479ffae3dep-1,
        0x1.715475f35c8b8p0
    };
    static const struct {
        double invc, logc;
    } T[] = {
        { 0x1.661ec79f8f3bep+0, -0x1.efec65b963019p-2 },
        { 0x1.571ed4aaf883dp+0, -0x1.b0b6832d4fca4p-2 },
        { 0x1.49539f0f010bp+0, -0x1.7418b0a1fb77bp-2 },
        { 0x1.3c995b0b80385p+0, -0x1.39de91a6dcf7bp-2 },
        { 0x1.30d190c8864a5p+0, -0x1.01d9bf3f2b631p-2 },
        { 0x1.25e227b0b8eap+0, -0x1.97c1d1b3b7afp-3 },
        { 0x1.1bb4a4a1a343fp+0, -0x1.2f9e393af3c9fp-3 },
        { 0x1.12358f08ae5bap+0, -0x1.960cbbf788d5cp-4 },
        { 0x1.0953f419900a7p+0, -0x1.a6f9db6475fcep-5 },
        { 0x1p+0, 0x0p+0 },
        { 0x1.e608cfd9a47acp-1, 0x1.338ca9f24f53dp-4 },
        { 0x1.ca4b31f026aap-1, 0x1.476a9543891bap-3 },
        { 0x1.b2036576afce6p-1, 0x1.e840b4ac4e4d2p-3 },
        { 0x1.9c2d163a1aa2dp-1, 0x1.40645f0c6651cp-2 },
        { 0x1.886e6037841edp-1, 0x1.88e9c2c1b9ff8p-2 },
        { 0x1.767dcf5534862p-1, 0x1.ce0a44eb17bccp-2 }
    };

    double z, r, r2, p, y, y0, invc, logc;
    UINT32 ix, iz, top, tmp;
    int k, i;

    ix = *(UINT32*)&x;
    /* Fix sign of zero with downward rounding when x==1. */
    if (ix == 0x3f800000)
        return 0;
    if (ix - 0x00800000 >= 0x7f800000 - 0x00800000) {
        /* x < 0x1p-126 or inf or nan. */
        if (ix * 2 == 0) {
            *_errno() = ERANGE;
            return -1.0f / x;
        }
        if (ix == 0x7f800000) /* log2(inf) == inf. */
            return x;
        if (ix * 2 > 0xff000000)
            return x;
        if (ix & 0x80000000) {
            *_errno() = EDOM;
            return (x - x) / (x - x);
        }
        /* x is subnormal, normalize it. */
        x *= 0x1p23f;
        ix = *(UINT32*)&x;
        ix -= 23 << 23;
    }

    /* x = 2^k z; where z is in range [OFF,2*OFF] and exact.
       The range is split into N subintervals.
       The ith subinterval contains z and c is near its center. */
    tmp = ix - 0x3f330000;
    i = (tmp >> (23 - 4)) % (1 << 4);
    top = tmp & 0xff800000;
    iz = ix - top;
    k = (INT32)tmp >> 23; /* arithmetic shift */
    invc = T[i].invc;
    logc = T[i].logc;
    z = *(float*)&iz;

    /* log2(x) = log1p(z/c-1)/ln2 + log2(c) + k */
    r = z * invc - 1;
    y0 = logc + (double)k;

    /* Pipelined polynomial evaluation to approximate log1p(r)/ln2. */
    r2 = r * r;
    y = A[1] * r + A[2];
    y = A[0] * r2 + y;
    p = A[3] * r + y0;
    y = y * r2 + p;
    return y;
}

/*********************************************************************
 *      rint (MSVCR120.@)
 */
double CDECL rint(double x)
{
    return __rint(x);
}

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
 *      round (MSVCR120.@)
 */
double CDECL round(double x)
{
    return __round(x);
}

/*********************************************************************
 *      roundf (MSVCR120.@)
 *
 * Copied from musl: src/math/roundf.c
 */
float CDECL roundf(float x)
{
    static const float toint = 1 / FLT_EPSILON;

    unsigned int ix = *(unsigned int*)&x;
    int e = ix >> 23 & 0xff;
    float y;

    if (e >= 0x7f + 23)
        return x;
    if (ix >> 31)
        x = -x;
    if (e < 0x7f - 1)
        return 0 * *(float*)&ix;
    y = fp_barrierf(x + toint) - toint - x;
    if (y > 0.5f)
        y = y + x - 1;
    else if (y <= -0.5f)
        y = y + x + 1;
    else
        y = y + x;
    if (ix >> 31)
        y = -y;
    return y;
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
 *      trunc (MSVCR120.@)
 *
 * Copied from musl: src/math/trunc.c
 */
double CDECL trunc(double x)
{
    union {double f; UINT64 i;} u = {x};
    int e = (u.i >> 52 & 0x7ff) - 0x3ff + 12;
    UINT64 m;

    if (e >= 52 + 12)
        return x;
    if (e < 12)
        e = 1;
    m = -1ULL >> e;
    if ((u.i & m) == 0)
        return x;
    u.i &= ~m;
    return u.f;
}

/*********************************************************************
 *      truncf (MSVCR120.@)
 *
 * Copied from musl: src/math/truncf.c
 */
float CDECL truncf(float x)
{
    union {float f; UINT32 i;} u = {x};
    int e = (u.i >> 23 & 0xff) - 0x7f + 9;
    UINT32 m;

    if (e >= 23 + 9)
        return x;
    if (e < 9)
        e = 1;
    m = -1U >> e;
    if ((u.i & m) == 0)
        return x;
    u.i &= ~m;
    return u.f;
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
 *      fmaxf (MSVCR120.@)
 */
float CDECL fmaxf(float x, float y)
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
double CDECL fmax(double x, double y)
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
float CDECL fdimf(float x, float y)
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
double CDECL fdim(double x, double y)
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
 *      fminf (MSVCR120.@)
 */
float CDECL fminf(float x, float y)
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
double CDECL fmin(double x, double y)
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
 *      remquo (MSVCR120.@)
 *
 * Copied from musl: src/math/remquo.c
 */
double CDECL remquo(double x, double y, int *quo)
{
    UINT64 uxi = *(UINT64*)&x;
    UINT64 uyi = *(UINT64*)&y;
    int ex = uxi >> 52 & 0x7ff;
    int ey = uyi >> 52 & 0x7ff;
    int sx = uxi >> 63;
    int sy = uyi >> 63;
    UINT32 q;
    UINT64 i;

    *quo = 0;
    if (y == 0 || isinf(x)) *_errno() = EDOM;
    if (uyi << 1 == 0 || isnan(y) || ex == 0x7ff)
        return (x * y) / (x * y);
    if (uxi << 1 == 0)
        return x;

    /* normalize x and y */
    if (!ex) {
        for (i = uxi << 12; i >> 63 == 0; ex--, i <<= 1);
        uxi <<= -ex + 1;
    } else {
        uxi &= -1ULL >> 12;
        uxi |= 1ULL << 52;
    }
    if (!ey) {
        for (i = uyi << 12; i >> 63 == 0; ey--, i <<= 1);
        uyi <<= -ey + 1;
    } else {
        uyi &= -1ULL >> 12;
        uyi |= 1ULL << 52;
    }

    q = 0;
    if (ex < ey) {
        if (ex+1 == ey)
            goto end;
        return x;
    }

    /* x mod y */
    for (; ex > ey; ex--) {
        i = uxi - uyi;
        if (i >> 63 == 0) {
            uxi = i;
            q++;
        }
        uxi <<= 1;
        q <<= 1;
    }
    i = uxi - uyi;
    if (i >> 63 == 0) {
        uxi = i;
        q++;
    }
    if (uxi == 0)
        ex = -60;
    else
        for (; uxi >> 52 == 0; uxi <<= 1, ex--);
end:
    /* scale result and decide between |x| and |x|-|y| */
    if (ex > 0) {
        uxi -= 1ULL << 52;
        uxi |= (UINT64)ex << 52;
    } else {
        uxi >>= -ex + 1;
    }
    x = *(double*)&uxi;
    if (sy)
        y = -y;
    if (ex == ey || (ex + 1 == ey && (2 * x > y || (2 * x == y && q % 2)))) {
        x -= y;
        q++;
    }
    q &= 0x7fffffff;
    *quo = sx ^ sy ? -(int)q : (int)q;
    return sx ? -x : x;
}

/*********************************************************************
 *      remquof (MSVCR120.@)
 *
 * Copied from musl: src/math/remquof.c
 */
float CDECL remquof(float x, float y, int *quo)
{
    UINT32 uxi = *(UINT32*)&x;
    UINT32 uyi = *(UINT32*)&y;
    int ex = uxi >> 23 & 0xff;
    int ey = uyi >> 23 & 0xff;
    int sx = uxi >> 31;
    int sy = uyi>> 31;
    UINT32 q, i;

    *quo = 0;
    if (y == 0 || isinf(x)) *_errno() = EDOM;
    if (uyi << 1 == 0 || isnan(y) || ex == 0xff)
        return (x * y) / (x * y);
    if (uxi << 1 == 0)
        return x;

    /* normalize x and y */
    if (!ex) {
        for (i = uxi << 9; i >> 31 == 0; ex--, i <<= 1);
        uxi <<= -ex + 1;
    } else {
        uxi &= -1U >> 9;
        uxi |= 1U << 23;
    }
    if (!ey) {
        for (i = uyi << 9; i >> 31 == 0; ey--, i <<= 1);
        uyi <<= -ey + 1;
    } else {
        uyi &= -1U >> 9;
        uyi |= 1U << 23;
    }

    q = 0;
    if (ex < ey) {
        if (ex + 1 == ey)
            goto end;
        return x;
    }

    /* x mod y */
    for (; ex > ey; ex--) {
        i = uxi - uyi;
        if (i >> 31 == 0) {
            uxi = i;
            q++;
        }
        uxi <<= 1;
        q <<= 1;
    }
    i = uxi - uyi;
    if (i >> 31 == 0) {
        uxi = i;
        q++;
    }
    if (uxi == 0)
        ex = -30;
    else
        for (; uxi >> 23 == 0; uxi <<= 1, ex--);
end:
    /* scale result and decide between |x| and |x|-|y| */
    if (ex > 0) {
        uxi -= 1U << 23;
        uxi |= (UINT32)ex << 23;
    } else {
        uxi >>= -ex + 1;
    }
    x = *(float*)&uxi;
    if (sy)
        y = -y;
    if (ex == ey || (ex + 1 == ey && (2 * x > y || (2 * x == y && q % 2)))) {
        x -= y;
        q++;
    }
    q &= 0x7fffffff;
    *quo = sx ^ sy ? -(int)q : (int)q;
    return sx ? -x : x;
}

/* sin(pi*x) assuming x > 2^-100, if sin(pi*x)==0 the sign is arbitrary */
static double sin_pi(double x)
{
    int n;

    /* spurious inexact if odd int */
    x = 2.0 * (x * 0.5 - floor(x * 0.5)); /* x mod 2.0 */

    n = x * 4.0;
    n = (n + 1) / 2;
    x -= n * 0.5f;
    x *= M_PI;

    switch (n) {
    default: /* case 4: */
    case 0: return __sin(x, 0.0, 0);
    case 1: return __cos(x, 0.0);
    case 2: return __sin(-x, 0.0, 0);
    case 3: return -__cos(x, 0.0);
    }
}

/*********************************************************************
 *      lgamma (MSVCR120.@)
 *
 * Copied from musl: src/math/lgamma_r.c
 */
double CDECL lgamma(double x)
{
    static const double pi = 3.14159265358979311600e+00,
        a0 = 7.72156649015328655494e-02,
        a1 = 3.22467033424113591611e-01,
        a2 = 6.73523010531292681824e-02,
        a3 = 2.05808084325167332806e-02,
        a4 = 7.38555086081402883957e-03,
        a5 = 2.89051383673415629091e-03,
        a6 = 1.19270763183362067845e-03,
        a7 = 5.10069792153511336608e-04,
        a8 = 2.20862790713908385557e-04,
        a9 = 1.08011567247583939954e-04,
        a10 = 2.52144565451257326939e-05,
        a11 = 4.48640949618915160150e-05,
        tc = 1.46163214496836224576e+00,
        tf = -1.21486290535849611461e-01,
        tt = -3.63867699703950536541e-18,
        t0 = 4.83836122723810047042e-01,
        t1 = -1.47587722994593911752e-01,
        t2 = 6.46249402391333854778e-02,
        t3 = -3.27885410759859649565e-02,
        t4 = 1.79706750811820387126e-02,
        t5 = -1.03142241298341437450e-02,
        t6 = 6.10053870246291332635e-03,
        t7 = -3.68452016781138256760e-03,
        t8 = 2.25964780900612472250e-03,
        t9 = -1.40346469989232843813e-03,
        t10 = 8.81081882437654011382e-04,
        t11 = -5.38595305356740546715e-04,
        t12 = 3.15632070903625950361e-04,
        t13 = -3.12754168375120860518e-04,
        t14 = 3.35529192635519073543e-04,
        u0 = -7.72156649015328655494e-02,
        u1 = 6.32827064025093366517e-01,
        u2 = 1.45492250137234768737e+00,
        u3 = 9.77717527963372745603e-01,
        u4 = 2.28963728064692451092e-01,
        u5 = 1.33810918536787660377e-02,
        v1 = 2.45597793713041134822e+00,
        v2 = 2.12848976379893395361e+00,
        v3 = 7.69285150456672783825e-01,
        v4 = 1.04222645593369134254e-01,
        v5 = 3.21709242282423911810e-03,
        s0 = -7.72156649015328655494e-02,
        s1 = 2.14982415960608852501e-01,
        s2 = 3.25778796408930981787e-01,
        s3 = 1.46350472652464452805e-01,
        s4 = 2.66422703033638609560e-02,
        s5 = 1.84028451407337715652e-03,
        s6 = 3.19475326584100867617e-05,
        r1 = 1.39200533467621045958e+00,
        r2 = 7.21935547567138069525e-01,
        r3 = 1.71933865632803078993e-01,
        r4 = 1.86459191715652901344e-02,
        r5 = 7.77942496381893596434e-04,
        r6 = 7.32668430744625636189e-06,
        w0 = 4.18938533204672725052e-01,
        w1 = 8.33333333333329678849e-02,
        w2 = -2.77777777728775536470e-03,
        w3 = 7.93650558643019558500e-04,
        w4 = -5.95187557450339963135e-04,
        w5 = 8.36339918996282139126e-04,
        w6 = -1.63092934096575273989e-03;

    union {double f; UINT64 i;} u = {x};
    double t, y, z, nadj, p, p1, p2, p3, q, r, w;
    UINT32 ix;
    int sign,i;

    /* purge off +-inf, NaN, +-0, tiny and negative arguments */
    sign = u.i >> 63;
    ix = u.i >> 32 & 0x7fffffff;
    if (ix >= 0x7ff00000)
        return x * x;
    if (ix < (0x3ff - 70) << 20) { /* |x|<2**-70, return -log(|x|) */
        if(sign)
            x = -x;
        return -log(x);
    }
    if (sign) {
        x = -x;
        t = sin_pi(x);
        if (t == 0.0) { /* -integer */
            *_errno() = ERANGE;
            return 1.0 / (x - x);
        }
        if (t <= 0.0)
            t = -t;
        nadj = log(pi / (t * x));
    }

    /* purge off 1 and 2 */
    if ((ix == 0x3ff00000 || ix == 0x40000000) && (UINT32)u.i == 0)
        r = 0;
    /* for x < 2.0 */
    else if (ix < 0x40000000) {
        if (ix <= 0x3feccccc) { /* lgamma(x) = lgamma(x+1)-log(x) */
            r = -log(x);
            if (ix >= 0x3FE76944) {
                y = 1.0 - x;
                i = 0;
            } else if (ix >= 0x3FCDA661) {
                y = x - (tc - 1.0);
                i = 1;
            } else {
                y = x;
                i = 2;
            }
        } else {
            r = 0.0;
            if (ix >= 0x3FFBB4C3) { /* [1.7316,2] */
                y = 2.0 - x;
                i = 0;
            } else if(ix >= 0x3FF3B4C4) { /* [1.23,1.73] */
                y = x - tc;
                i = 1;
            } else {
                y = x - 1.0;
                i = 2;
            }
        }
        switch (i) {
        case 0:
            z = y * y;
            p1 = a0 + z * (a2 + z * (a4 + z * (a6 + z * (a8 + z * a10))));
            p2 = z * (a1 + z * (a3 + z * (a5 + z * (a7 + z * (a9 + z * a11)))));
            p = y * p1 + p2;
            r += (p - 0.5 * y);
            break;
        case 1:
            z = y * y;
            w = z * y;
            p1 = t0 + w * (t3 + w * (t6 + w * (t9 + w * t12))); /* parallel comp */
            p2 = t1 + w * (t4 + w * (t7 + w * (t10 + w * t13)));
            p3 = t2 + w * (t5 + w * (t8 + w * (t11 + w * t14)));
            p = z * p1 - (tt - w * (p2 + y * p3));
            r += tf + p;
            break;
        case 2:
            p1 = y * (u0 + y * (u1 + y * (u2 + y * (u3 + y * (u4 + y * u5)))));
            p2 = 1.0 + y * (v1 + y * (v2 + y * (v3 + y * (v4 + y * v5))));
            r += -0.5 * y + p1 / p2;
        }
    } else if (ix < 0x40200000) { /* x < 8.0 */
        i = (int)x;
        y = x - (double)i;
        p = y * (s0 + y * (s1 + y * (s2 + y * (s3 + y * (s4 + y * (s5 + y * s6))))));
        q = 1.0 + y * (r1 + y * (r2 + y * (r3 + y * (r4 + y * (r5 + y * r6)))));
        r = 0.5 * y + p / q;
        z = 1.0; /* lgamma(1+s) = log(s) + lgamma(s) */
        switch (i) {
        case 7: z *= y + 6.0; /* fall through */
        case 6: z *= y + 5.0; /* fall through */
        case 5: z *= y + 4.0; /* fall through */
        case 4: z *= y + 3.0; /* fall through */
        case 3:
            z *= y + 2.0;
            r += log(z);
            break;
        }
    } else if (ix < 0x43900000) { /* 8.0 <= x < 2**58 */
        t = log(x);
        z = 1.0 / x;
        y = z * z;
        w = w0 + z * (w1 + y * (w2 + y * (w3 + y * (w4 + y * (w5 + y * w6)))));
        r = (x - 0.5) * (t - 1.0) + w;
    } else /* 2**58 <= x <= inf */
        r = x * (log(x) - 1.0);
    if (sign)
        r = nadj - r;
    return r;
}

/* sin(pi*x) assuming x > 2^-100, if sin(pi*x)==0 the sign is arbitrary */
static float sinf_pi(float x)
{
    double y;
    int n;

    /* spurious inexact if odd int */
    x = 2 * (x * 0.5f - floorf(x * 0.5f)); /* x mod 2.0 */

    n = (int)(x * 4);
    n = (n + 1) / 2;
    y = x - n * 0.5f;
    y *= M_PI;
    switch (n) {
    default: /* case 4: */
    case 0: return __sindf(y);
    case 1: return __cosdf(y);
    case 2: return __sindf(-y);
    case 3: return -__cosdf(y);
    }
}

/*********************************************************************
 *      lgammaf (MSVCR120.@)
 *
 * Copied from musl: src/math/lgammaf_r.c
 */
float CDECL lgammaf(float x)
{
    static const float pi = 3.1415927410e+00,
        a0 = 7.7215664089e-02,
        a1 = 3.2246702909e-01,
        a2 = 6.7352302372e-02,
        a3 = 2.0580807701e-02,
        a4 = 7.3855509982e-03,
        a5 = 2.8905137442e-03,
        a6 = 1.1927076848e-03,
        a7 = 5.1006977446e-04,
        a8 = 2.2086278477e-04,
        a9 = 1.0801156895e-04,
        a10 = 2.5214456400e-05,
        a11 = 4.4864096708e-05,
        tc = 1.4616321325e+00,
        tf = -1.2148628384e-01,
        tt = 6.6971006518e-09,
        t0 = 4.8383611441e-01,
        t1 = -1.4758771658e-01,
        t2 = 6.4624942839e-02,
        t3 = -3.2788541168e-02,
        t4 = 1.7970675603e-02,
        t5 = -1.0314224288e-02,
        t6 = 6.1005386524e-03,
        t7 = -3.6845202558e-03,
        t8 = 2.2596477065e-03,
        t9 = -1.4034647029e-03,
        t10 = 8.8108185446e-04,
        t11 = -5.3859531181e-04,
        t12 = 3.1563205994e-04,
        t13 = -3.1275415677e-04,
        t14 = 3.3552918467e-04,
        u0 = -7.7215664089e-02,
        u1 = 6.3282704353e-01,
        u2 = 1.4549225569e+00,
        u3 = 9.7771751881e-01,
        u4 = 2.2896373272e-01,
        u5 = 1.3381091878e-02,
        v1 = 2.4559779167e+00,
        v2 = 2.1284897327e+00,
        v3 = 7.6928514242e-01,
        v4 = 1.0422264785e-01,
        v5 = 3.2170924824e-03,
        s0 = -7.7215664089e-02,
        s1 = 2.1498242021e-01,
        s2 = 3.2577878237e-01,
        s3 = 1.4635047317e-01,
        s4 = 2.6642270386e-02,
        s5 = 1.8402845599e-03,
        s6 = 3.1947532989e-05,
        r1 = 1.3920053244e+00,
        r2 = 7.2193557024e-01,
        r3 = 1.7193385959e-01,
        r4 = 1.8645919859e-02,
        r5 = 7.7794247773e-04,
        r6 = 7.3266842264e-06,
        w0 = 4.1893854737e-01,
        w1 = 8.3333335817e-02,
        w2 = -2.7777778450e-03,
        w3 = 7.9365057172e-04,
        w4 = -5.9518753551e-04,
        w5 = 8.3633989561e-04,
        w6 = -1.6309292987e-03;

    union {float f; UINT32 i;} u = {x};
    float t, y, z, nadj, p, p1, p2, p3, q, r, w;
    UINT32 ix;
    int i, sign;

    /* purge off +-inf, NaN, +-0, tiny and negative arguments */
    sign = u.i >> 31;
    ix = u.i & 0x7fffffff;
    if (ix >= 0x7f800000)
        return x * x;
    if (ix < 0x35000000) { /* |x| < 2**-21, return -log(|x|) */
        if (sign)
            x = -x;
        return -logf(x);
    }
    if (sign) {
        x = -x;
        t = sinf_pi(x);
        if (t == 0.0f) { /* -integer */
            *_errno() = ERANGE;
            return 1.0f / (x - x);
        }
        if (t <= 0.0f)
            t = -t;
        nadj = logf(pi / (t * x));
    }

    /* purge off 1 and 2 */
    if (ix == 0x3f800000 || ix == 0x40000000)
        r = 0;
    /* for x < 2.0 */
    else if (ix < 0x40000000) {
        if (ix <= 0x3f666666) { /* lgamma(x) = lgamma(x+1)-log(x) */
            r = -logf(x);
            if (ix >= 0x3f3b4a20) {
                y = 1.0f - x;
                i = 0;
            } else if (ix >= 0x3e6d3308) {
                y = x - (tc - 1.0f);
                i = 1;
            } else {
                y = x;
                i = 2;
            }
        } else {
            r = 0.0f;
            if (ix >= 0x3fdda618) { /* [1.7316,2] */
                y = 2.0f - x;
                i = 0;
            } else if (ix >= 0x3F9da620) { /* [1.23,1.73] */
                y = x - tc;
                i = 1;
            } else {
                y = x - 1.0f;
                i = 2;
            }
        }
        switch(i) {
        case 0:
            z = y * y;
            p1 = a0 + z * (a2 + z * (a4 + z * (a6 + z * (a8 + z * a10))));
            p2 = z * (a1 + z * (a3 + z * (a5 + z * (a7 + z * (a9 + z * a11)))));
            p = y * p1 + p2;
            r += p - 0.5f * y;
            break;
        case 1:
            z = y * y;
            w = z * y;
            p1 = t0 + w * (t3 + w * (t6 + w * (t9 + w * t12))); /* parallel comp */
            p2 = t1 + w * (t4 + w * (t7 + w * (t10 + w * t13)));
            p3 = t2 + w * (t5 + w * (t8 + w * (t11 + w * t14)));
            p = z * p1 - (tt - w * (p2 + y * p3));
            r += (tf + p);
            break;
        case 2:
            p1 = y * (u0 + y * (u1 + y * (u2 + y * (u3 + y * (u4 + y * u5)))));
            p2 = 1.0f + y * (v1 + y * (v2 + y * (v3 + y * (v4 + y * v5))));
            r += -0.5f * y + p1 / p2;
        }
    } else if (ix < 0x41000000) { /* x < 8.0 */
        i = (int)x;
        y = x - (float)i;
        p = y * (s0 + y * (s1 + y * (s2 + y * (s3 + y * (s4 + y * (s5 + y * s6))))));
        q = 1.0f + y * (r1 + y * (r2 + y * (r3 + y * (r4 + y * (r5 + y * r6)))));
        r = 0.5f * y + p / q;
        z = 1.0f; /* lgamma(1+s) = log(s) + lgamma(s) */
        switch (i) {
        case 7: z *= y + 6.0f; /* fall through */
        case 6: z *= y + 5.0f; /* fall through */
        case 5: z *= y + 4.0f; /* fall through */
        case 4: z *= y + 3.0f; /* fall through */
        case 3:
            z *= y + 2.0f;
            r += logf(z);
            break;
        }
    } else if (ix < 0x5c800000) { /* 8.0 <= x < 2**58 */
        t = logf(x);
        z = 1.0f / x;
        y = z * z;
        w = w0 + z * (w1 + y * (w2 + y * (w3 + y * (w4 + y * (w5 + y * w6)))));
        r = (x - 0.5f) * (t - 1.0f) + w;
    } else /* 2**58 <= x <= inf */
        r = x * (logf(x) - 1.0f);
    if (sign)
        r = nadj - r;
    return r;
}

static double tgamma_S(double x)
{
    static const double Snum[] = {
        23531376880.410759688572007674451636754734846804940,
        42919803642.649098768957899047001988850926355848959,
        35711959237.355668049440185451547166705960488635843,
        17921034426.037209699919755754458931112671403265390,
        6039542586.3520280050642916443072979210699388420708,
        1439720407.3117216736632230727949123939715485786772,
        248874557.86205415651146038641322942321632125127801,
        31426415.585400194380614231628318205362874684987640,
        2876370.6289353724412254090516208496135991145378768,
        186056.26539522349504029498971604569928220784236328,
        8071.6720023658162106380029022722506138218516325024,
        210.82427775157934587250973392071336271166969580291,
        2.5066282746310002701649081771338373386264310793408,
    };
    static const double Sden[] = {
        0, 39916800, 120543840, 150917976, 105258076, 45995730, 13339535,
        2637558, 357423, 32670, 1925, 66, 1,
    };

    double num = 0, den = 0;
    int i;

    /* to avoid overflow handle large x differently */
    if (x < 8)
        for (i = ARRAY_SIZE(Snum) - 1; i >= 0; i--) {
            num = num * x + Snum[i];
            den = den * x + Sden[i];
        }
    else
        for (i = 0; i < ARRAY_SIZE(Snum); i++) {
            num = num / x + Snum[i];
            den = den / x + Sden[i];
        }
    return num / den;
}

/*********************************************************************
 *      tgamma (MSVCR120.@)
 *
 * Copied from musl: src/math/tgamma.c
 */
double CDECL tgamma(double x)
{
    static const double gmhalf = 5.524680040776729583740234375;
    static const double fact[] = {
        1, 1, 2, 6, 24, 120, 720, 5040.0, 40320.0, 362880.0, 3628800.0, 39916800.0,
        479001600.0, 6227020800.0, 87178291200.0, 1307674368000.0, 20922789888000.0,
        355687428096000.0, 6402373705728000.0, 121645100408832000.0,
        2432902008176640000.0, 51090942171709440000.0, 1124000727777607680000.0,
    };

    union {double f; UINT64 i;} u = {x};
    double absx, y, dy, z, r;
    UINT32 ix = u.i >> 32 & 0x7fffffff;
    int sign = u.i >> 63;

    /* special cases */
    if (ix >= 0x7ff00000) {
        /* tgamma(nan)=nan, tgamma(inf)=inf, tgamma(-inf)=nan with invalid */
        if (u.i == 0xfff0000000000000ULL)
            *_errno() = EDOM;
        return x + INFINITY;
    }
    if (ix < (0x3ff - 54) << 20) {
        /* |x| < 2^-54: tgamma(x) ~ 1/x, +-0 raises div-by-zero */
        if (x == 0.0)
            *_errno() = ERANGE;
        return 1 / x;
    }

    /* integer arguments */
    /* raise inexact when non-integer */
    if (x == floor(x)) {
        if (sign) {
            *_errno() = EDOM;
            return 0 / (x - x);
        }
        if (x <= ARRAY_SIZE(fact))
            return fact[(int)x - 1];
    }

    /* x >= 172: tgamma(x)=inf with overflow */
    /* x =< -184: tgamma(x)=+-0 with underflow */
    if (ix >= 0x40670000) { /* |x| >= 184 */
        *_errno() = ERANGE;
        if (sign) {
            fp_barrierf(0x1p-126 / x);
            return 0;
        }
        x *= 0x1p1023;
        return x;
    }

    absx = sign ? -x : x;

    /* handle the error of x + g - 0.5 */
    y = absx + gmhalf;
    if (absx > gmhalf) {
        dy = y - absx;
        dy -= gmhalf;
    } else {
        dy = y - gmhalf;
        dy -= absx;
    }

    z = absx - 0.5;
    r = tgamma_S(absx) * exp(-y);
    if (x < 0) {
        /* reflection formula for negative x */
        /* sinpi(absx) is not 0, integers are already handled */
        r = -M_PI / (sin_pi(absx) * absx * r);
        dy = -dy;
        z = -z;
    }
    r += dy * (gmhalf + 0.5) * r / y;
    z = pow(y, 0.5 * z);
    y = r * z * z;
    return y;
}

/*********************************************************************
 *      tgammaf (MSVCR120.@)
 *
 * Copied from musl: src/math/tgammaf.c
 */
float CDECL tgammaf(float x)
{
    return tgamma(x);
}

/*********************************************************************
 *      nan (MSVCR120.@)
 */
double CDECL nan(const char *tagp)
{
    /* Windows ignores input (MSDN) */
    return NAN;
}

/*********************************************************************
 *      nanf (MSVCR120.@)
 */
float CDECL nanf(const char *tagp)
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
    DWORD fpword = 0;
    WORD operation;
    int raise = 0;

    TRACE("(%x %x %lf %lf %x %p)\n", fpe, op, arg, res, cw, unk);

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
    _control87(fpword, 0xffffffff);

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

/*********************************************************************
 *      ilogb (MSVCR120.@)
 */
int CDECL ilogb(double x)
{
    return __ilogb(x);
}

/*********************************************************************
 *      ilogbf (MSVCR120.@)
 */
int CDECL ilogbf(float x)
{
    return __ilogbf(x);
}
#endif /* _MSVCR_VER>=120 */
