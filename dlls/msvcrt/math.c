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
#if !defined(__i386__) || _MSVCR_VER>=120
static inline float fp_barrierf(float x)
{
    volatile float y = x;
    return y;
}
#endif

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

/*********************************************************************
 *      atanf (MSVCRT.@)
 */
#if _MSVCR_VER == 0  /* other versions call atanf() directly */
float CDECL MSVCRT_atanf( float x )
{
    if (isnan(x)) return math_error(_DOMAIN, "atanf", x, 0, x);
    return atanf( x );
}
#endif

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
 *		asin (MSVCRT.@)
 */
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

double CDECL MSVCRT_asin( double x )
{
#ifdef __i386__
    unsigned int x87_cw, sse2_cw;
    unsigned int hx = *(ULONGLONG*)&x >> 32;
    unsigned int ix = hx & 0x7fffffff;

    if (isnan(x)) return math_error(_DOMAIN, "asin", x, 0, x);

    /* |x| < 1 */
    if (ix < 0x3ff00000)
    {
        __control87_2(0, 0, &x87_cw, &sse2_cw);
        if (!sse2_enabled || (x87_cw & _MCW_EM) != _MCW_EM
            || (sse2_cw & (_MCW_EM | _MCW_RC)) != _MCW_EM)
            return x87_asin(x);
    }
#else
    if (isnan(x)) return x;
#endif

    return asin( x );
}

/*********************************************************************
 *		atan (MSVCRT.@)
 */
#if _MSVCR_VER == 0  /* other versions call atan() directly */
double CDECL MSVCRT_atan( double x )
{
    if (isnan(x)) return math_error(_DOMAIN, "atan", x, 0, x);
    return atan( x );
}
#endif

/*********************************************************************
 *		exp (MSVCRT.@)
 */
#if _MSVCR_VER == 0  /* other versions call exp() directly */
double CDECL MSVCRT_exp( double x )
{
    if (isnan( x )) return math_error(_DOMAIN, "exp", x, 0, 1.0 + x);
    return exp( x );
}
#endif

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
 *      rint (MSVCR120.@)
 */
double CDECL MSVCRT_rint(double x)
{
    unsigned cw;
    double y;

    cw = _controlfp(0, 0);
    if ((cw & _MCW_PC) != _PC_53)
        _controlfp(_PC_53, _MCW_PC);
    y = rint(x);
    if ((cw & _MCW_PC) != _PC_53)
        _controlfp(cw, _MCW_PC);
    return y;
}

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
    x = MSVCRT_rint(x);
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
 *      lrint (MSVCR120.@)
 */
__msvcrt_long CDECL lrint(double x)
{
    double d;

    d = MSVCRT_rint(x);
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

    d = MSVCRT_rint(x);
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
