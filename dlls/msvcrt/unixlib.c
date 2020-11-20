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
 *      cbrt
 */
static double CDECL unix_cbrt(double x)
{
#ifdef HAVE_CBRT
    return cbrt(x);
#else
    return x < 0 ? -pow(-x, 1.0 / 3.0) : pow(x, 1.0 / 3.0);
#endif
}

/*********************************************************************
 *      cbrtf
 */
static float CDECL unix_cbrtf(float x)
{
#ifdef HAVE_CBRTF
    return cbrtf(x);
#else
    return unix_cbrt(x);
#endif
}

/*********************************************************************
 *      ceil
 */
static double CDECL unix_ceil( double x )
{
    return ceil( x );
}

/*********************************************************************
 *      ceilf
 */
static float CDECL unix_ceilf( float x )
{
    return ceilf( x );
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
 *      erf
 */
static double CDECL unix_erf(double x)
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
 *      erff
 */
static float CDECL unix_erff(float x)
{
#ifdef HAVE_ERFF
    return erff(x);
#else
    return unix_erf(x);
#endif
}

/*********************************************************************
 *      erfc
 */
static double CDECL unix_erfc(double x)
{
#ifdef HAVE_ERFC
    return erfc(x);
#else
    return 1 - unix_erf(x);
#endif
}

/*********************************************************************
 *      erfcf
 */
static float CDECL unix_erfcf(float x)
{
#ifdef HAVE_ERFCF
    return erfcf(x);
#else
    return unix_erfc(x);
#endif
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
 *      floor
 */
static double CDECL unix_floor( double x )
{
    return floor( x );
}

/*********************************************************************
 *      floorf
 */
static float CDECL unix_floorf( float x )
{
    return floorf( x );
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
 *      fmod
 */
static double CDECL unix_fmod( double x, double y )
{
    return fmod( x, y );
}

/*********************************************************************
 *      fmodf
 */
static float CDECL unix_fmodf( float x, float y )
{
    return fmodf( x, y );
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
 *      j0
 */
static double CDECL unix_j0(double num)
{
#ifdef HAVE_J0
    return j0(num);
#else
    FIXME("not implemented\n");
    return 0;
#endif
}

/*********************************************************************
 *      j1
 */
static double CDECL unix_j1(double num)
{
#ifdef HAVE_J1
    return j1(num);
#else
    FIXME("not implemented\n");
    return 0;
#endif
}

/*********************************************************************
 *      jn
 */
static double CDECL unix_jn(int n, double num)
{
#ifdef HAVE_JN
    return jn(n, num);
#else
    FIXME("not implemented\n");
    return 0;
#endif
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
 *      llrint
 */
static __int64 CDECL unix_llrint(double x)
{
    return llrint(x);
}

/*********************************************************************
 *      llrintf
 */
static __int64 CDECL unix_llrintf(float x)
{
    return llrintf(x);
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
 *      logb
 */
static double CDECL unix_logb( double x )
{
    return logb( x );
}

/*********************************************************************
 *      logbf
 */
static float CDECL unix_logbf( float x )
{
    return logbf( x );
}

/*********************************************************************
 *      lrint
 */
static int CDECL unix_lrint(double x)
{
    return lrint(x);
}

/*********************************************************************
 *      lrintf
 */
static int CDECL unix_lrintf(float x)
{
    return lrintf(x);
}

/*********************************************************************
 *      modf
 */
static double CDECL unix_modf( double x, double *iptr )
{
    return modf( x, iptr );
}

/*********************************************************************
 *      modff
 */
static float CDECL unix_modff( float x, float *iptr )
{
    return modff( x, iptr );
}

/*********************************************************************
 *      nearbyint
 */
static double CDECL unix_nearbyint(double num)
{
#ifdef HAVE_NEARBYINT
    return nearbyint(num);
#else
    return num >= 0 ? floor(num + 0.5) : ceil(num - 0.5);
#endif
}

/*********************************************************************
 *      nearbyintf
 */
static float CDECL unix_nearbyintf(float num)
{
#ifdef HAVE_NEARBYINTF
    return nearbyintf(num);
#else
    return unix_nearbyint(num);
#endif
}

/*********************************************************************
 *      nextafter
 */
static double CDECL unix_nextafter(double num, double next)
{
  return nextafter(num,next);
}

/*********************************************************************
 *      nextafterf
 */
static float CDECL unix_nextafterf(float num, float next)
{
  return nextafterf(num,next);
}

/*********************************************************************
 *      nexttoward
 */
static double CDECL unix_nexttoward(double num, double next)
{
#ifdef HAVE_NEXTTOWARD
    return nexttoward(num, next);
#else
    FIXME("not implemented\n");
    return 0;
#endif
}

/*********************************************************************
 *      nexttowardf
 */
static float CDECL unix_nexttowardf(float num, double next)
{
#ifdef HAVE_NEXTTOWARDF
    return nexttowardf(num, next);
#else
    FIXME("not implemented\n");
    return 0;
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
 *      remainder
 */
static double CDECL unix_remainder(double x, double y)
{
#ifdef HAVE_REMAINDER
    return remainder(x, y);
#else
    FIXME( "not implemented\n" );
    return 0;
#endif
}

/*********************************************************************
 *      remainderf
 */
static float CDECL unix_remainderf(float x, float y)
{
#ifdef HAVE_REMAINDERF
    return remainderf(x, y);
#else
    FIXME( "not implemented\n" );
    return 0;
#endif
}

/*********************************************************************
 *      remquo
 */
static double CDECL unix_remquo(double x, double y, int *quo)
{
#ifdef HAVE_REMQUO
    return remquo(x, y, quo);
#else
    FIXME( "not implemented\n" );
    return 0;
#endif
}

/*********************************************************************
 *      remquof
 */
static float CDECL unix_remquof(float x, float y, int *quo)
{
#ifdef HAVE_REMQUOF
    return remquof(x, y, quo);
#else
    FIXME( "not implemented\n" );
    return 0;
#endif
}

/*********************************************************************
 *      rint
 */
static double CDECL unix_rint(double x)
{
    return rint(x);
}

/*********************************************************************
 *      rintf
 */
static float CDECL unix_rintf(float x)
{
    return rintf(x);
}

/*********************************************************************
 *      round
 */
static double CDECL unix_round(double x)
{
#ifdef HAVE_ROUND
    return round(x);
#else
    return unix_rint(x);
#endif
}

/*********************************************************************
 *      roundf
 */
static float CDECL unix_roundf(float x)
{
#ifdef HAVE_ROUNDF
    return roundf(x);
#else
    return unix_round(x);
#endif
}

/*********************************************************************
 *      lround
 */
static int CDECL unix_lround(double x)
{
#ifdef HAVE_LROUND
    return lround(x);
#else
    return unix_round(x);
#endif
}

/*********************************************************************
 *      lroundf
 */
static int CDECL unix_lroundf(float x)
{
#ifdef HAVE_LROUNDF
    return lroundf(x);
#else
    return unix_lround(x);
#endif
}

/*********************************************************************
 *      llround
 */
static __int64 CDECL unix_llround(double x)
{
#ifdef HAVE_LLROUND
    return llround(x);
#else
    return unix_round(x);
#endif
}

/*********************************************************************
 *      llroundf
 */
static __int64 CDECL unix_llroundf(float x)
{
#ifdef HAVE_LLROUNDF
    return llroundf(x);
#else
    return unix_llround(x);
#endif
}

/*********************************************************************
 *      sin
 */
static double CDECL unix_sin( double x )
{
    return sin( x );
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
 *      trunc
 */
static double CDECL unix_trunc(double x)
{
#ifdef HAVE_TRUNC
    return trunc(x);
#else
    return (x > 0) ? floor(x) : ceil(x);
#endif
}

/*********************************************************************
 *      truncf
 */
static float CDECL unix_truncf(float x)
{
#ifdef HAVE_TRUNCF
    return truncf(x);
#else
    return unix_trunc(x);
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

/*********************************************************************
 *      y0
 */
static double CDECL unix_y0(double num)
{
#ifdef HAVE_Y0
    return y0(num);
#else
    FIXME("not implemented\n");
    return 0;
#endif
}

/*********************************************************************
 *      y1
 */
static double CDECL unix_y1(double num)
{
#ifdef HAVE_Y1
    return y1(num);
#else
    FIXME("not implemented\n");
    return 0;
#endif
}

/*********************************************************************
 *      yn
 */
static double CDECL unix_yn(int order, double num)
{
#ifdef HAVE_YN
    return yn(order,num);
#else
    FIXME("not implemented\n");
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
    unix_cbrt,
    unix_cbrtf,
    unix_ceil,
    unix_ceilf,
    unix_cos,
    unix_cosf,
    unix_cosh,
    unix_coshf,
    unix_erf,
    unix_erfc,
    unix_erfcf,
    unix_erff,
    unix_exp,
    unix_expf,
    unix_exp2,
    unix_exp2f,
    unix_expm1,
    unix_expm1f,
    unix_floor,
    unix_floorf,
    unix_fma,
    unix_fmaf,
    unix_fmod,
    unix_fmodf,
    unix_frexp,
    unix_frexpf,
    unix_hypot,
    unix_hypotf,
    unix_j0,
    unix_j1,
    unix_jn,
    unix_ldexp,
    unix_lgamma,
    unix_lgammaf,
    unix_llrint,
    unix_llrintf,
    unix_llround,
    unix_llroundf,
    unix_log,
    unix_logf,
    unix_log10,
    unix_log10f,
    unix_log1p,
    unix_log1pf,
    unix_log2,
    unix_log2f,
    unix_logb,
    unix_logbf,
    unix_lrint,
    unix_lrintf,
    unix_lround,
    unix_lroundf,
    unix_modf,
    unix_modff,
    unix_nearbyint,
    unix_nearbyintf,
    unix_nextafter,
    unix_nextafterf,
    unix_nexttoward,
    unix_nexttowardf,
    unix_pow,
    unix_powf,
    unix_remainder,
    unix_remainderf,
    unix_remquo,
    unix_remquof,
    unix_rint,
    unix_rintf,
    unix_round,
    unix_roundf,
    unix_sin,
    unix_sinf,
    unix_sinh,
    unix_sinhf,
    unix_tan,
    unix_tanf,
    unix_tanh,
    unix_tanhf,
    unix_tgamma,
    unix_tgammaf,
    unix_trunc,
    unix_truncf,
    unix_y0,
    unix_y1,
    unix_yn
};

NTSTATUS CDECL __wine_init_unix_lib( HMODULE module, DWORD reason, const void *ptr_in, void *ptr_out )
{
    if (reason != DLL_PROCESS_ATTACH) return STATUS_SUCCESS;
    TRACE( "\n" );
    *(const struct unix_funcs **)ptr_out = &funcs;
    return STATUS_SUCCESS;
}
