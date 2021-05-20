/*
 * MSVCRT Unix interface
 *
 * Copyright 2020 Alexandre Julliard
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

#if 0
#pragma makedep unix
#endif

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <stdarg.h>
#define __USE_ISOC9X 1
#define __USE_ISOC99 1
#include <math.h>
#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "unixlib.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/*********************************************************************
 *      acosh
 */
static double CDECL unix_acosh(double x)
{
#ifdef HAVE_ACOSH
    return acosh(x);
#else
    if (!isfinite(x*x)) return log(2) + log(x);
    return log(x + sqrt(x*x-1));
#endif
}

/*********************************************************************
 *      acoshf
 */
static float CDECL unix_acoshf(float x)
{
#ifdef HAVE_ACOSHF
    return acoshf(x);
#else
    return unix_acosh(x);
#endif
}

/*********************************************************************
 *      asinh
 */
static double CDECL unix_asinh(double x)
{
#ifdef HAVE_ASINH
    return asinh(x);
#else
    if (!isfinite(x*x+1))
    {
        if (x > 0) return log(2) + log(x);
        else return -log(2) - log(-x);
    }
    return log(x + sqrt(x*x+1));
#endif
}

/*********************************************************************
 *      asinhf
 */
static float CDECL unix_asinhf(float x)
{
#ifdef HAVE_ASINHF
    return asinhf(x);
#else
    return unix_asinh(x);
#endif
}

/*********************************************************************
 *      atanh
 */
static double CDECL unix_atanh(double x)
{
#ifdef HAVE_ATANH
    return atanh(x);
#else
    if (-1e-6 < x && x < 1e-6) return x + x*x*x/3;
    else return (log(1+x) - log(1-x)) / 2;
#endif
}

/*********************************************************************
 *      atanhf
 */
static float CDECL unix_atanhf(float x)
{
#ifdef HAVE_ATANHF
    return atanhf(x);
#else
    return unix_atanh(x);
#endif
}

/*********************************************************************
 *      cos
 */
static double CDECL unix_cos( double x )
{
    return cos( x );
}

/*********************************************************************
 *      cosf
 */
static float CDECL unix_cosf( float x )
{
    return cosf( x );
}

/*********************************************************************
 *      cosh
 */
static double CDECL unix_cosh( double x )
{
    return cosh( x );
}

/*********************************************************************
 *      coshf
 */
static float CDECL unix_coshf( float x )
{
    return coshf( x );
}

/*********************************************************************
 *      exp
 */
static double CDECL unix_exp( double x )
{
    return exp( x );
}

/*********************************************************************
 *      expf
 */
static float CDECL unix_expf( float x )
{
    return expf( x );
}

/*********************************************************************
 *      exp2
 */
static double CDECL unix_exp2( double x )
{
#ifdef HAVE_EXP2
    return exp2(x);
#else
    return pow(2, x);
#endif
}

/*********************************************************************
 *      exp2f
 */
static float CDECL unix_exp2f( float x )
{
#ifdef HAVE_EXP2F
    return exp2f(x);
#else
    return unix_exp2(x);
#endif
}

/*********************************************************************
 *      expm1
 */
static double CDECL unix_expm1(double x)
{
#ifdef HAVE_EXPM1
    return expm1(x);
#else
    return exp(x) - 1;
#endif
}

/*********************************************************************
 *      expm1f
 */
static float CDECL unix_expm1f(float x)
{
#ifdef HAVE_EXPM1F
    return expm1f(x);
#else
    return exp(x) - 1;
#endif
}

/*********************************************************************
 *      fma
 */
static double CDECL unix_fma( double x, double y, double z )
{
#ifdef HAVE_FMA
    return fma(x, y, z);
#else
    return x * y + z;
#endif
}

/*********************************************************************
 *      fmaf
 */
static float CDECL unix_fmaf( float x, float y, float z )
{
#ifdef HAVE_FMAF
    return fmaf(x, y, z);
#else
    return x * y + z;
#endif
}

/*********************************************************************
 *      frexp
 */
static double CDECL unix_frexp( double x, int *exp )
{
    return frexp( x, exp );
}

/*********************************************************************
 *      frexpf
 */
static float CDECL unix_frexpf( float x, int *exp )
{
    return frexpf( x, exp );
}

/*********************************************************************
 *      hypot
 */
static double CDECL unix_hypot(double x, double y)
{
    return hypot( x, y );
}

/*********************************************************************
 *      hypotf
 */
static float CDECL unix_hypotf(float x, float y)
{
    return hypotf( x, y );
}

/*********************************************************************
 *      ldexp
 */
static double CDECL unix_ldexp(double num, int exp)
{
    return ldexp( num, exp );
}

/*********************************************************************
 *      lgamma
 */
static double CDECL unix_lgamma(double x)
{
#ifdef HAVE_LGAMMA
    return lgamma(x);
#else
    FIXME( "not implemented\n" );
    return 0;
#endif
}

/*********************************************************************
 *      lgammaf
 */
static float CDECL unix_lgammaf(float x)
{
#ifdef HAVE_LGAMMAF
    return lgammaf(x);
#else
    FIXME( "not implemented\n" );
    return 0;
#endif
}

/*********************************************************************
 *      log
 */
static double CDECL unix_log( double x )
{
    return log( x );
}

/*********************************************************************
 *      logf
 */
static float CDECL unix_logf( float x )
{
    return logf( x );
}

/*********************************************************************
 *      log10
 */
static double CDECL unix_log10( double x )
{
    return log10( x );
}

/*********************************************************************
 *      log10f
 */
static float CDECL unix_log10f( float x )
{
    return log10f( x );
}

/*********************************************************************
 *      log1p
 */
static double CDECL unix_log1p(double x)
{
#ifdef HAVE_LOG1P
    return log1p(x);
#else
    return log(1 + x);
#endif
}

/*********************************************************************
 *      log1pf
 */
static float CDECL unix_log1pf(float x)
{
#ifdef HAVE_LOG1PF
    return log1pf(x);
#else
    return log(1 + x);
#endif
}

/*********************************************************************
 *      log2
 */
static double CDECL unix_log2(double x)
{
#ifdef HAVE_LOG2
    return log2(x);
#else
    return log(x) / log(2);
#endif
}

/*********************************************************************
 *      log2f
 */
static float CDECL unix_log2f(float x)
{
#ifdef HAVE_LOG2F
    return log2f(x);
#else
    return unix_log2(x);
#endif
}

/*********************************************************************
 *      pow
 */
static double CDECL unix_pow( double x, double y )
{
    return pow( x, y );
}

/*********************************************************************
 *      powf
 */
static float CDECL unix_powf( float x, float y )
{
    return powf( x, y );
}

/*********************************************************************
 *      sinf
 */
static float CDECL unix_sinf( float x )
{
    return sinf( x );
}

/*********************************************************************
 *      sinh
 */
static double CDECL unix_sinh( double x )
{
    return sinh( x );
}

/*********************************************************************
 *      sinhf
 */
static float CDECL unix_sinhf( float x )
{
    return sinhf( x );
}

/*********************************************************************
 *      tan
 */
static double CDECL unix_tan( double x )
{
    return tan( x );
}

/*********************************************************************
 *      tanf
 */
static float CDECL unix_tanf( float x )
{
    return tanf( x );
}

/*********************************************************************
 *      tanh
 */
static double CDECL unix_tanh( double x )
{
    return tanh( x );
}

/*********************************************************************
 *      tanhf
 */
static float CDECL unix_tanhf( float x )
{
    return tanhf( x );
}

/*********************************************************************
 *      tgamma
 */
static double CDECL unix_tgamma(double x)
{
#ifdef HAVE_TGAMMA
    return tgamma(x);
#else
    FIXME( "not implemented\n" );
    return 0;
#endif
}

/*********************************************************************
 *      tgammaf
 */
static float CDECL unix_tgammaf(float x)
{
#ifdef HAVE_TGAMMAF
    return tgammaf(x);
#else
    FIXME( "not implemented\n" );
    return 0;
#endif
}

static const struct unix_funcs funcs =
{
    unix_acosh,
    unix_acoshf,
    unix_asinh,
    unix_asinhf,
    unix_atanh,
    unix_atanhf,
    unix_cos,
    unix_cosf,
    unix_cosh,
    unix_coshf,
    unix_exp,
    unix_expf,
    unix_exp2,
    unix_exp2f,
    unix_expm1,
    unix_expm1f,
    unix_fma,
    unix_fmaf,
    unix_frexp,
    unix_frexpf,
    unix_hypot,
    unix_hypotf,
    unix_ldexp,
    unix_lgamma,
    unix_lgammaf,
    unix_log,
    unix_logf,
    unix_log10,
    unix_log10f,
    unix_log1p,
    unix_log1pf,
    unix_log2,
    unix_log2f,
    unix_pow,
    unix_powf,
    unix_sinf,
    unix_sinh,
    unix_sinhf,
    unix_tan,
    unix_tanf,
    unix_tanh,
    unix_tanhf,
    unix_tgamma,
    unix_tgammaf,
};

NTSTATUS CDECL __wine_init_unix_lib( HMODULE module, DWORD reason, const void *ptr_in, void *ptr_out )
{
    if (reason != DLL_PROCESS_ATTACH) return STATUS_SUCCESS;
    TRACE( "\n" );
    *(const struct unix_funcs **)ptr_out = &funcs;
    return STATUS_SUCCESS;
}
