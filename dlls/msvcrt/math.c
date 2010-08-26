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
 */
#include "config.h"

#include <stdio.h>
#define __USE_ISOC9X 1
#define __USE_ISOC99 1
#include <math.h>
#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif

#include "msvcrt.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

#ifndef HAVE_FINITE
#ifndef finite /* Could be a macro */
#ifdef isfinite
#define finite(x) isfinite(x)
#else
#define finite(x) (!isnan(x)) /* At least catch some cases */
#endif
#endif
#endif

#ifndef signbit
#define signbit(x) 0
#endif

typedef int (CDECL *MSVCRT_matherr_func)(struct MSVCRT__exception *);

static MSVCRT_matherr_func MSVCRT_default_matherr_func = NULL;

/*********************************************************************
 *      _set_SSE2_enable (MSVCRT.@)
 */
int CDECL MSVCRT__set_SSE2_enable(int flag)
{
    FIXME("(%x) stub\n", flag);
    return flag;
}

#ifdef __x86_64__

/*********************************************************************
 *      MSVCRT_acosf (MSVCRT.@)
 */
float CDECL MSVCRT_acosf( float x )
{
  if (x < -1.0 || x > 1.0 || !finitef(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  /* glibc implements acos() as the FPU equivalent of atan2(sqrt(1 - x ^ 2), x).
   * asin() uses a similar construction. This is bad because as x gets nearer to
   * 1 the error in the expression "1 - x^2" can get relatively large due to
   * cancellation. The sqrt() makes things worse. A safer way to calculate
   * acos() is to use atan2(sqrt((1 - x) * (1 + x)), x). */
  return atan2f(sqrtf((1 - x) * (1 + x)), x);
}

/*********************************************************************
 *      MSVCRT_asinf (MSVCRT.@)
 */
float CDECL MSVCRT_asinf( float x )
{
  if (x < -1.0 || x > 1.0 || !finitef(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return atan2f(x, sqrtf((1 - x) * (1 + x)));
}

/*********************************************************************
 *      MSVCRT_atanf (MSVCRT.@)
 */
float CDECL MSVCRT_atanf( float x )
{
  if (!finitef(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return atanf(x);
}

/*********************************************************************
 *              MSVCRT_atan2f (MSVCRT.@)
 */
float CDECL MSVCRT_atan2f( float x, float y )
{
  if (!finitef(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return atan2f(x,y);
}

/*********************************************************************
 *      MSVCRT_cosf (MSVCRT.@)
 */
float CDECL MSVCRT_cosf( float x )
{
  if (!finitef(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return cosf(x);
}

/*********************************************************************
 *      MSVCRT_coshf (MSVCRT.@)
 */
float CDECL MSVCRT_coshf( float x )
{
  if (!finitef(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return coshf(x);
}

/*********************************************************************
 *      MSVCRT_expf (MSVCRT.@)
 */
float CDECL MSVCRT_expf( float x )
{
  if (!finitef(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return expf(x);
}

/*********************************************************************
 *      MSVCRT_fmodf (MSVCRT.@)
 */
float CDECL MSVCRT_fmodf( float x, float y )
{
  if (!finitef(x) || !finitef(y)) *MSVCRT__errno() = MSVCRT_EDOM;
  return fmodf(x,y);
}

/*********************************************************************
 *      MSVCRT_logf (MSVCRT.@)
 */
float CDECL MSVCRT_logf( float x)
{
  if (x < 0.0 || !finitef(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  if (x == 0.0) *MSVCRT__errno() = MSVCRT_ERANGE;
  return logf(x);
}

/*********************************************************************
 *      MSVCRT_log10f (MSVCRT.@)
 */
float CDECL MSVCRT_log10f( float x )
{
  if (x < 0.0 || !finitef(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  if (x == 0.0) *MSVCRT__errno() = MSVCRT_ERANGE;
  return log10f(x);
}

/*********************************************************************
 *      MSVCRT_powf (MSVCRT.@)
 */
float CDECL MSVCRT_powf( float x, float y )
{
  /* FIXME: If x < 0 and y is not integral, set EDOM */
  float z = powf(x,y);
  if (!finitef(z)) *MSVCRT__errno() = MSVCRT_EDOM;
  return z;
}

/*********************************************************************
 *      MSVCRT_sinf (MSVCRT.@)
 */
float CDECL MSVCRT_sinf( float x )
{
  if (!finitef(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return sinf(x);
}

/*********************************************************************
 *      MSVCRT_sinhf (MSVCRT.@)
 */
float CDECL MSVCRT_sinhf( float x )
{
  if (!finitef(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return sinhf(x);
}

/*********************************************************************
 *      MSVCRT_sqrtf (MSVCRT.@)
 */
float CDECL MSVCRT_sqrtf( float x )
{
  if (x < 0.0 || !finitef(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return sqrtf(x);
}

/*********************************************************************
 *      MSVCRT_tanf (MSVCRT.@)
 */
float CDECL MSVCRT_tanf( float x )
{
  if (!finitef(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return tanf(x);
}

/*********************************************************************
 *      MSVCRT_tanhf (MSVCRT.@)
 */
float CDECL MSVCRT_tanhf( float x )
{
  if (!finitef(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return tanhf(x);
}

/*********************************************************************
 *      ceilf (MSVCRT.@)
 */
float CDECL MSVCRT_ceilf( float x )
{
  return ceilf(x);
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
 *      _scalbf (MSVCRT.@)
 */
float CDECL MSVCRT__scalbf(float num, MSVCRT_long power)
{
  if (!finitef(num)) *MSVCRT__errno() = MSVCRT_EDOM;
  return ldexpf(num, power);
}

/*********************************************************************
 *      modff (MSVCRT.@)
 */
double CDECL MSVCRT_modff( float x, float *iptr )
{
  return modff( x, iptr );
}

#endif

/*********************************************************************
 *		MSVCRT_acos (MSVCRT.@)
 */
double CDECL MSVCRT_acos( double x )
{
  if (x < -1.0 || x > 1.0 || !finite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  /* glibc implements acos() as the FPU equivalent of atan2(sqrt(1 - x ^ 2), x).
   * asin() uses a similar construction. This is bad because as x gets nearer to
   * 1 the error in the expression "1 - x^2" can get relatively large due to
   * cancellation. The sqrt() makes things worse. A safer way to calculate
   * acos() is to use atan2(sqrt((1 - x) * (1 + x)), x). */
  return atan2(sqrt((1 - x) * (1 + x)), x);
}

/*********************************************************************
 *		MSVCRT_asin (MSVCRT.@)
 */
double CDECL MSVCRT_asin( double x )
{
  if (x < -1.0 || x > 1.0 || !finite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return atan2(x, sqrt((1 - x) * (1 + x)));
}

/*********************************************************************
 *		MSVCRT_atan (MSVCRT.@)
 */
double CDECL MSVCRT_atan( double x )
{
  if (!finite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return atan(x);
}

/*********************************************************************
 *		MSVCRT_atan2 (MSVCRT.@)
 */
double CDECL MSVCRT_atan2( double x, double y )
{
  if (!finite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return atan2(x,y);
}

/*********************************************************************
 *		MSVCRT_cos (MSVCRT.@)
 */
double CDECL MSVCRT_cos( double x )
{
  if (!finite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return cos(x);
}

/*********************************************************************
 *		MSVCRT_cosh (MSVCRT.@)
 */
double CDECL MSVCRT_cosh( double x )
{
  if (!finite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return cosh(x);
}

/*********************************************************************
 *		MSVCRT_exp (MSVCRT.@)
 */
double CDECL MSVCRT_exp( double x )
{
  if (!finite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return exp(x);
}

/*********************************************************************
 *		MSVCRT_fmod (MSVCRT.@)
 */
double CDECL MSVCRT_fmod( double x, double y )
{
  if (!finite(x) || !finite(y)) *MSVCRT__errno() = MSVCRT_EDOM;
  return fmod(x,y);
}

/*********************************************************************
 *		MSVCRT_log (MSVCRT.@)
 */
double CDECL MSVCRT_log( double x)
{
  if (x < 0.0 || !finite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  if (x == 0.0) *MSVCRT__errno() = MSVCRT_ERANGE;
  return log(x);
}

/*********************************************************************
 *		MSVCRT_log10 (MSVCRT.@)
 */
double CDECL MSVCRT_log10( double x )
{
  if (x < 0.0 || !finite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  if (x == 0.0) *MSVCRT__errno() = MSVCRT_ERANGE;
  return log10(x);
}

/*********************************************************************
 *		MSVCRT_pow (MSVCRT.@)
 */
double CDECL MSVCRT_pow( double x, double y )
{
  /* FIXME: If x < 0 and y is not integral, set EDOM */
  double z = pow(x,y);
  if (!finite(z)) *MSVCRT__errno() = MSVCRT_EDOM;
  return z;
}

/*********************************************************************
 *		MSVCRT_sin (MSVCRT.@)
 */
double CDECL MSVCRT_sin( double x )
{
  if (!finite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return sin(x);
}

/*********************************************************************
 *		MSVCRT_sinh (MSVCRT.@)
 */
double CDECL MSVCRT_sinh( double x )
{
  if (!finite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return sinh(x);
}

/*********************************************************************
 *		MSVCRT_sqrt (MSVCRT.@)
 */
double CDECL MSVCRT_sqrt( double x )
{
  if (x < 0.0 || !finite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return sqrt(x);
}

/*********************************************************************
 *		MSVCRT_tan (MSVCRT.@)
 */
double CDECL MSVCRT_tan( double x )
{
  if (!finite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return tan(x);
}

/*********************************************************************
 *		MSVCRT_tanh (MSVCRT.@)
 */
double CDECL MSVCRT_tanh( double x )
{
  if (!finite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return tanh(x);
}


#if defined(__GNUC__) && defined(__i386__)

#define FPU_DOUBLE(var) double var; \
  __asm__ __volatile__( "fstpl %0;fwait" : "=m" (var) : )
#define FPU_DOUBLES(var1,var2) double var1,var2; \
  __asm__ __volatile__( "fstpl %0;fwait" : "=m" (var2) : ); \
  __asm__ __volatile__( "fstpl %0;fwait" : "=m" (var1) : )

/*********************************************************************
 *		_CIacos (MSVCRT.@)
 */
double CDECL _CIacos(void)
{
  FPU_DOUBLE(x);
  return MSVCRT_acos(x);
}

/*********************************************************************
 *		_CIasin (MSVCRT.@)
 */
double CDECL _CIasin(void)
{
  FPU_DOUBLE(x);
  return MSVCRT_asin(x);
}

/*********************************************************************
 *		_CIatan (MSVCRT.@)
 */
double CDECL _CIatan(void)
{
  FPU_DOUBLE(x);
  return MSVCRT_atan(x);
}

/*********************************************************************
 *		_CIatan2 (MSVCRT.@)
 */
double CDECL _CIatan2(void)
{
  FPU_DOUBLES(x,y);
  return MSVCRT_atan2(x,y);
}

/*********************************************************************
 *		_CIcos (MSVCRT.@)
 */
double CDECL _CIcos(void)
{
  FPU_DOUBLE(x);
  return MSVCRT_cos(x);
}

/*********************************************************************
 *		_CIcosh (MSVCRT.@)
 */
double CDECL _CIcosh(void)
{
  FPU_DOUBLE(x);
  return MSVCRT_cosh(x);
}

/*********************************************************************
 *		_CIexp (MSVCRT.@)
 */
double CDECL _CIexp(void)
{
  FPU_DOUBLE(x);
  return MSVCRT_exp(x);
}

/*********************************************************************
 *		_CIfmod (MSVCRT.@)
 */
double CDECL _CIfmod(void)
{
  FPU_DOUBLES(x,y);
  return MSVCRT_fmod(x,y);
}

/*********************************************************************
 *		_CIlog (MSVCRT.@)
 */
double CDECL _CIlog(void)
{
  FPU_DOUBLE(x);
  return MSVCRT_log(x);
}

/*********************************************************************
 *		_CIlog10 (MSVCRT.@)
 */
double CDECL _CIlog10(void)
{
  FPU_DOUBLE(x);
  return MSVCRT_log10(x);
}

/*********************************************************************
 *		_CIpow (MSVCRT.@)
 */
double CDECL _CIpow(void)
{
  FPU_DOUBLES(x,y);
  return MSVCRT_pow(x,y);
}

/*********************************************************************
 *		_CIsin (MSVCRT.@)
 */
double CDECL _CIsin(void)
{
  FPU_DOUBLE(x);
  return MSVCRT_sin(x);
}

/*********************************************************************
 *		_CIsinh (MSVCRT.@)
 */
double CDECL _CIsinh(void)
{
  FPU_DOUBLE(x);
  return MSVCRT_sinh(x);
}

/*********************************************************************
 *		_CIsqrt (MSVCRT.@)
 */
double CDECL _CIsqrt(void)
{
  FPU_DOUBLE(x);
  return MSVCRT_sqrt(x);
}

/*********************************************************************
 *		_CItan (MSVCRT.@)
 */
double CDECL _CItan(void)
{
  FPU_DOUBLE(x);
  return MSVCRT_tan(x);
}

/*********************************************************************
 *		_CItanh (MSVCRT.@)
 */
double CDECL _CItanh(void)
{
  FPU_DOUBLE(x);
  return MSVCRT_tanh(x);
}

#endif /* defined(__GNUC__) && defined(__i386__) */

/*********************************************************************
 *		_fpclass (MSVCRT.@)
 */
int CDECL _fpclass(double num)
{
#if defined(HAVE_FPCLASS) || defined(fpclass)
  switch (fpclass( num ))
  {
#ifdef FP_SNAN
  case FP_SNAN:  return MSVCRT__FPCLASS_SNAN;
#endif
#ifdef FP_QNAN
  case FP_QNAN:  return MSVCRT__FPCLASS_QNAN;
#endif
#ifdef FP_NINF
  case FP_NINF:  return MSVCRT__FPCLASS_NINF;
#endif
#ifdef FP_PINF
  case FP_PINF:  return MSVCRT__FPCLASS_PINF;
#endif
#ifdef FP_NDENORM
  case FP_NDENORM: return MSVCRT__FPCLASS_ND;
#endif
#ifdef FP_PDENORM
  case FP_PDENORM: return MSVCRT__FPCLASS_PD;
#endif
#ifdef FP_NZERO
  case FP_NZERO: return MSVCRT__FPCLASS_NZ;
#endif
#ifdef FP_PZERO
  case FP_PZERO: return MSVCRT__FPCLASS_PZ;
#endif
#ifdef FP_NNORM
  case FP_NNORM: return MSVCRT__FPCLASS_NN;
#endif
#ifdef FP_PNORM
  case FP_PNORM: return MSVCRT__FPCLASS_PN;
#endif
  default: return MSVCRT__FPCLASS_PN;
  }
#elif defined (fpclassify)
  switch (fpclassify( num ))
  {
  case FP_NAN: return MSVCRT__FPCLASS_QNAN;
  case FP_INFINITE: return signbit(num) ? MSVCRT__FPCLASS_NINF : MSVCRT__FPCLASS_PINF;
  case FP_SUBNORMAL: return signbit(num) ?MSVCRT__FPCLASS_ND : MSVCRT__FPCLASS_PD;
  case FP_ZERO: return signbit(num) ? MSVCRT__FPCLASS_NZ : MSVCRT__FPCLASS_PZ;
  }
  return signbit(num) ? MSVCRT__FPCLASS_NN : MSVCRT__FPCLASS_PN;
#else
  if (!finite(num))
    return MSVCRT__FPCLASS_QNAN;
  return num == 0.0 ? MSVCRT__FPCLASS_PZ : (num < 0 ? MSVCRT__FPCLASS_NN : MSVCRT__FPCLASS_PN);
#endif
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
 *		_logb (MSVCRT.@)
 */
double CDECL _logb(double num)
{
  if (!finite(num)) *MSVCRT__errno() = MSVCRT_EDOM;
  return logb(num);
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
 *		_scalb (MSVCRT.@)
 */
double CDECL MSVCRT__scalb(double num, MSVCRT_long power)
{
  if (!finite(num)) *MSVCRT__errno() = MSVCRT_EDOM;
  return ldexp(num, power);
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
float CDECL _hypotf(float x, float y)
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
 *		fabs (MSVCRT.@)
 */
double CDECL MSVCRT_fabs( double x )
{
  return fabs(x);
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

/*********************************************************************
 *		_matherr (MSVCRT.@)
 */
int CDECL MSVCRT__matherr(struct MSVCRT__exception *e)
{
  if (e)
    TRACE("(%p = %d, %s, %g %g %g)\n",e, e->type, e->name, e->arg1, e->arg2,
          e->retval);
  else
    TRACE("(null)\n");
  if (MSVCRT_default_matherr_func)
    return MSVCRT_default_matherr_func(e);
  ERR(":Unhandled math error!\n");
  return 0;
}

/*********************************************************************
 *		__setusermatherr (MSVCRT.@)
 */
void CDECL MSVCRT___setusermatherr(MSVCRT_matherr_func func)
{
  MSVCRT_default_matherr_func = func;
  TRACE(":new matherr handler %p\n", func);
}

/**********************************************************************
 *		_statusfp (MSVCRT.@)
 */
unsigned int CDECL _statusfp(void)
{
   unsigned int retVal = 0;
#if defined(__GNUC__) && defined(__i386__)
  unsigned int fpword;

  __asm__ __volatile__( "fstsw %0" : "=m" (fpword) : );
  if (fpword & 0x1)  retVal |= MSVCRT__SW_INVALID;
  if (fpword & 0x2)  retVal |= MSVCRT__SW_DENORMAL;
  if (fpword & 0x4)  retVal |= MSVCRT__SW_ZERODIVIDE;
  if (fpword & 0x8)  retVal |= MSVCRT__SW_OVERFLOW;
  if (fpword & 0x10) retVal |= MSVCRT__SW_UNDERFLOW;
  if (fpword & 0x20) retVal |= MSVCRT__SW_INEXACT;
#else
  FIXME(":Not implemented!\n");
#endif
  return retVal;
}

/*********************************************************************
 *		_clearfp (MSVCRT.@)
 */
unsigned int CDECL _clearfp(void)
{
  unsigned int retVal = _statusfp();
#if defined(__GNUC__) && defined(__i386__)
  __asm__ __volatile__( "fnclex" );
#else
  FIXME(":Not Implemented\n");
#endif
  return retVal;
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

  if (!finite(z))
    *MSVCRT__errno() = MSVCRT_ERANGE;
  else if (z == 0 && signbit(z))
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
double CDECL _chgsign(double num)
{
  /* FIXME: +-infinity,Nan not tested */
  return -num;
}

/*********************************************************************
 *		_control87 (MSVCRT.@)
 */
unsigned int CDECL _control87(unsigned int newval, unsigned int mask)
{
#if defined(__GNUC__) && defined(__i386__)
  unsigned int fpword = 0;
  unsigned int flags = 0;

  TRACE("(%08x, %08x): Called\n", newval, mask);

  /* Get fp control word */
  __asm__ __volatile__( "fstcw %0" : "=m" (fpword) : );

  TRACE("Control word before : %08x\n", fpword);

  /* Convert into mask constants */
  if (fpword & 0x1)  flags |= MSVCRT__EM_INVALID;
  if (fpword & 0x2)  flags |= MSVCRT__EM_DENORMAL;
  if (fpword & 0x4)  flags |= MSVCRT__EM_ZERODIVIDE;
  if (fpword & 0x8)  flags |= MSVCRT__EM_OVERFLOW;
  if (fpword & 0x10) flags |= MSVCRT__EM_UNDERFLOW;
  if (fpword & 0x20) flags |= MSVCRT__EM_INEXACT;
  switch(fpword & 0xC00) {
  case 0xC00: flags |= MSVCRT__RC_UP|MSVCRT__RC_DOWN; break;
  case 0x800: flags |= MSVCRT__RC_UP; break;
  case 0x400: flags |= MSVCRT__RC_DOWN; break;
  }
  switch(fpword & 0x300) {
  case 0x0:   flags |= MSVCRT__PC_24; break;
  case 0x200: flags |= MSVCRT__PC_53; break;
  case 0x300: flags |= MSVCRT__PC_64; break;
  }
  if (fpword & 0x1000) flags |= MSVCRT__IC_AFFINE;

  /* Mask with parameters */
  flags = (flags & ~mask) | (newval & mask);

  /* Convert (masked) value back to fp word */
  fpword = 0;
  if (flags & MSVCRT__EM_INVALID)    fpword |= 0x1;
  if (flags & MSVCRT__EM_DENORMAL)   fpword |= 0x2;
  if (flags & MSVCRT__EM_ZERODIVIDE) fpword |= 0x4;
  if (flags & MSVCRT__EM_OVERFLOW)   fpword |= 0x8;
  if (flags & MSVCRT__EM_UNDERFLOW)  fpword |= 0x10;
  if (flags & MSVCRT__EM_INEXACT)    fpword |= 0x20;
  switch(flags & (MSVCRT__RC_UP | MSVCRT__RC_DOWN)) {
  case MSVCRT__RC_UP|MSVCRT__RC_DOWN: fpword |= 0xC00; break;
  case MSVCRT__RC_UP:          fpword |= 0x800; break;
  case MSVCRT__RC_DOWN:        fpword |= 0x400; break;
  }
  switch (flags & (MSVCRT__PC_24 | MSVCRT__PC_53)) {
  case MSVCRT__PC_64: fpword |= 0x300; break;
  case MSVCRT__PC_53: fpword |= 0x200; break;
  case MSVCRT__PC_24: fpword |= 0x0; break;
  }
  if (flags & MSVCRT__IC_AFFINE) fpword |= 0x1000;

  TRACE("Control word after  : %08x\n", fpword);

  /* Put fp control word */
  __asm__ __volatile__( "fldcw %0" : : "m" (fpword) );

  return flags;
#else
  FIXME(":Not Implemented!\n");
  return 0;
#endif
}

/*********************************************************************
 *		_controlfp (MSVCRT.@)
 */
unsigned int CDECL _controlfp(unsigned int newval, unsigned int mask)
{
#ifdef __i386__
  return _control87( newval, mask & ~MSVCRT__EM_DENORMAL );
#else
  FIXME(":Not Implemented!\n");
  return 0;
#endif
}

/*********************************************************************
 *              _controlfp_s (MSVCRT.@)
 */
int CDECL _controlfp_s(unsigned int *cur, unsigned int newval, unsigned int mask)
{
#ifdef __i386__
    unsigned int flags;

    FIXME("(%p %u %u) semi-stub\n", cur, newval, mask);

    flags = _control87( newval, mask & ~MSVCRT__EM_DENORMAL );

    if(cur)
        *cur = flags;

    return 0;
#else
    FIXME(":Not Implemented!\n");
    return 0;
#endif
}

/*********************************************************************
 *		_copysign (MSVCRT.@)
 */
double CDECL _copysign(double num, double sign)
{
  /* FIXME: Behaviour for Nan/Inf? */
  if (sign < 0.0)
    return num < 0.0 ? num : -num;
  return num < 0.0 ? -num : num;
}

/*********************************************************************
 *		_finite (MSVCRT.@)
 */
int CDECL _finite(double num)
{
  return (finite(num)?1:0); /* See comment for _isnan() */
}

/*********************************************************************
 *		_fpreset (MSVCRT.@)
 */
void CDECL _fpreset(void)
{
#if defined(__GNUC__) && defined(__i386__)
  __asm__ __volatile__( "fninit" );
#else
  FIXME(":Not Implemented!\n");
#endif
}

/*********************************************************************
 *		_isnan (MSVCRT.@)
 */
INT CDECL _isnan(double num)
{
  /* Some implementations return -1 for true(glibc), msvcrt/crtdll return 1.
   * Do the same, as the result may be used in calculations
   */
  return isnan(num) ? 1 : 0;
}

/*********************************************************************
 *		_j0 (MSVCRT.@)
 */
double CDECL _j0(double num)
{
  /* FIXME: errno handling */
  return j0(num);
}

/*********************************************************************
 *		_j1 (MSVCRT.@)
 */
double CDECL _j1(double num)
{
  /* FIXME: errno handling */
  return j1(num);
}

/*********************************************************************
 *		jn (MSVCRT.@)
 */
double CDECL _jn(int n, double num)
{
  /* FIXME: errno handling */
  return jn(n, num);
}

/*********************************************************************
 *		_y0 (MSVCRT.@)
 */
double CDECL _y0(double num)
{
  double retval;
  if (!finite(num)) *MSVCRT__errno() = MSVCRT_EDOM;
  retval  = y0(num);
  if (_fpclass(retval) == MSVCRT__FPCLASS_NINF)
  {
    *MSVCRT__errno() = MSVCRT_EDOM;
    retval = sqrt(-1);
  }
  return retval;
}

/*********************************************************************
 *		_y1 (MSVCRT.@)
 */
double CDECL _y1(double num)
{
  double retval;
  if (!finite(num)) *MSVCRT__errno() = MSVCRT_EDOM;
  retval  = y1(num);
  if (_fpclass(retval) == MSVCRT__FPCLASS_NINF)
  {
    *MSVCRT__errno() = MSVCRT_EDOM;
    retval = sqrt(-1);
  }
  return retval;
}

/*********************************************************************
 *		_yn (MSVCRT.@)
 */
double CDECL _yn(int order, double num)
{
  double retval;
  if (!finite(num)) *MSVCRT__errno() = MSVCRT_EDOM;
  retval  = yn(order,num);
  if (_fpclass(retval) == MSVCRT__FPCLASS_NINF)
  {
    *MSVCRT__errno() = MSVCRT_EDOM;
    retval = sqrt(-1);
  }
  return retval;
}

/*********************************************************************
 *		_nextafter (MSVCRT.@)
 */
double CDECL _nextafter(double num, double next)
{
  double retval;
  if (!finite(num) || !finite(next)) *MSVCRT__errno() = MSVCRT_EDOM;
  retval = nextafter(num,next);
  return retval;
}

/*********************************************************************
 *		_ecvt (MSVCRT.@)
 */
char * CDECL _ecvt( double number, int ndigits, int *decpt, int *sign )
{
    int prec, len;
    thread_data_t *data = msvcrt_get_thread_data();
    /* FIXME: check better for overflow (native supports over 300 chars's) */
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
    len = snprintf(data->efcvt_buffer, 80, "%.*le", prec - 1, number);
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

/***********************************************************************
 *		_fcvt  (MSVCRT.@)
 */
char * CDECL _fcvt( double number, int ndigits, int *decpt, int *sign )
{
    thread_data_t *data = msvcrt_get_thread_data();
    int stop, dec1, dec2;
    char *ptr1, *ptr2, *first;
    char buf[80]; /* ought to be enough */

    if (!data->efcvt_buffer)
        data->efcvt_buffer = MSVCRT_malloc( 80 ); /* ought to be enough */

    if (number < 0)
    {
	*sign = 1;
	number = -number;
    } else *sign = 0;

    snprintf(buf, 80, "%.*f", ndigits < 0 ? 0 : ndigits, number);
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
	stop = strlen(buf) + ndigits;
    } else {
	stop = strlen(buf);
    }

    while (*ptr1 == '0') ptr1++; /* Skip leading zeroes */
    while (*ptr1 != '\0' && *ptr1 != '.') {
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
 *		_gcvt  (MSVCRT.@)
 */
char * CDECL _gcvt( double number, int ndigit, char *buff )
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
int CDECL _gcvt_s(char *buff, MSVCRT_size_t size, double number, int digits)
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
  div_t dt = div(num,denom);
  return ((unsigned __int64)dt.rem << 32) | (unsigned int)dt.quot;
}
#else
/*********************************************************************
 *		div (MSVCRT.@)
 * VERSION
 *	[!i386] Non-x86 can't run win32 apps so we don't need binary compatibility
 */
MSVCRT_div_t CDECL MSVCRT_div(int num, int denom)
{
  div_t dt = div(num,denom);
  MSVCRT_div_t     ret;
  ret.quot = dt.quot;
  ret.rem = dt.rem;

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
  ldiv_t ldt = ldiv(num,denom);
  return ((unsigned __int64)ldt.rem << 32) | (MSVCRT_ulong)ldt.quot;
}
#else
/*********************************************************************
 *		ldiv (MSVCRT.@)
 * VERSION
 *	[!i386] Non-x86 can't run win32 apps so we don't need binary compatibility
 */
MSVCRT_ldiv_t CDECL MSVCRT_ldiv(MSVCRT_long num, MSVCRT_long denom)
{
  ldiv_t result = ldiv(num,denom);

  MSVCRT_ldiv_t ret;
  ret.quot = result.quot;
  ret.rem = result.rem;

  return ret;
}
#endif /* ifdef __i386__ */

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

#endif  /* __i386__ */
