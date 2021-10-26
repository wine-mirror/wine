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
            return (top & 0x8000 ? 1.0 : -1.0) / x;
        if (ix == 0x7ff0000000000000ULL) /* log(inf) == inf. */
            return x;
        if ((top & 0x7ff0) == 0x7ff0 && (ix & 0xfffffffffffffULL))
            return x;
        if (top & 0x8000)
            return (x - x) / (x - x);
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

    if (isinf(x)) return x - x;
    if (ix >= 0x7ff00000)
        return x - x;

    n = __rem_pio2(x, y);
    return __tan(y[0], y[1], n & 1);
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
