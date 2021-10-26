/*
 * Math functions
 *
 * Copyright 2021 Alexandre Julliard
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

#include <math.h>
#include <float.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "ntdll_misc.h"

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


/* Copied from musl: src/math/rint.c */
static double __rint(double x)
{
    static const double toint = 1 / DBL_EPSILON;

    ULONGLONG llx = *(ULONGLONG*)&x;
    int e = llx >> 52 & 0x7ff;
    int s = llx >> 63;
    double y;

    if (e >= 0x3ff+52)
        return x;
    if (s)
        y = fp_barrier(x - toint) + toint;
    else
        y = fp_barrier(x + toint) - toint;
    if (y == 0)
        return s ? -0.0 : 0;
    return y;
}

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
 *                  abs   (NTDLL.@)
 */
int CDECL abs( int i )
{
    return i >= 0 ? i : -i;
}

/*********************************************************************
 *                  atan   (NTDLL.@)
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
 *                  ceil   (NTDLL.@)
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
 *                  cos   (NTDLL.@)
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
    if (isinf(x))
        return x - x;
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

/*********************************************************************
 *                  fabs   (NTDLL.@)
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
 *                  floor   (NTDLL.@)
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
 *                  log   (NTDLL.@)
 */
double CDECL log( double d )
{
    return unix_funcs->log( d );
}

/*********************************************************************
 *                  pow   (NTDLL.@)
 */
double CDECL pow( double x, double y )
{
    return unix_funcs->pow( x, y );
}

/*********************************************************************
 *                  sin   (NTDLL.@)
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
        return x - x;
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
 *                  sqrt   (NTDLL.@)
 */
double CDECL sqrt( double d )
{
    return unix_funcs->sqrt( d );
}

/*********************************************************************
 *                  tan   (NTDLL.@)
 */
double CDECL tan( double d )
{
    return unix_funcs->tan( d );
}

#if (defined(__GNUC__) || defined(__clang__)) && defined(__i386__)

#define FPU_DOUBLE(var) double var; \
    __asm__ __volatile__( "fstpl %0;fwait" : "=m" (var) : )
#define FPU_DOUBLES(var1,var2) double var1,var2; \
    __asm__ __volatile__( "fstpl %0;fwait" : "=m" (var2) : ); \
    __asm__ __volatile__( "fstpl %0;fwait" : "=m" (var1) : )

/*********************************************************************
 *		_CIcos (NTDLL.@)
 */
double CDECL _CIcos(void)
{
    FPU_DOUBLE(x);
    return cos(x);
}

/*********************************************************************
 *		_CIlog (NTDLL.@)
 */
double CDECL _CIlog(void)
{
    FPU_DOUBLE(x);
    return log(x);
}

/*********************************************************************
 *		_CIpow (NTDLL.@)
 */
double CDECL _CIpow(void)
{
    FPU_DOUBLES(x,y);
    return pow(x,y);
}

/*********************************************************************
 *		_CIsin (NTDLL.@)
 */
double CDECL _CIsin(void)
{
    FPU_DOUBLE(x);
    return sin(x);
}

/*********************************************************************
 *		_CIsqrt (NTDLL.@)
 */
double CDECL _CIsqrt(void)
{
    FPU_DOUBLE(x);
    return sqrt(x);
}

/*********************************************************************
 *                  _ftol   (NTDLL.@)
 */
LONGLONG CDECL _ftol(void)
{
    FPU_DOUBLE(x);
    return (LONGLONG)x;
}

#endif /* (defined(__GNUC__) || defined(__clang__)) && defined(__i386__) */
