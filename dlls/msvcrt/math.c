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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "config.h"
#include "msvcrt.h"
#include "msvcrt/errno.h"

#include <stdio.h>
#define __USE_ISOC9X 1
#define __USE_ISOC99 1
#include <math.h>
#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif

#include "msvcrt/stdlib.h"

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

/* fpclass constants */
#define _FPCLASS_SNAN 1
#define _FPCLASS_QNAN 2
#define _FPCLASS_NINF 4
#define _FPCLASS_NN   8
#define _FPCLASS_ND   16
#define _FPCLASS_NZ   32
#define _FPCLASS_PZ   64
#define _FPCLASS_PD   128
#define _FPCLASS_PN   256
#define _FPCLASS_PINF 512

/* _statusfp bit flags */
#define _SW_INEXACT    0x1
#define _SW_UNDERFLOW  0x2
#define _SW_OVERFLOW   0x4
#define _SW_ZERODIVIDE 0x8
#define _SW_INVALID    0x10
#define _SW_DENORMAL   0x80000

/* _controlfp masks and bitflags - x86 only so far*/
#ifdef __i386__
#define _MCW_EM        0x8001f
#define _EM_INEXACT    0x1
#define _EM_UNDERFLOW  0x2
#define _EM_OVERFLOW   0x4
#define _EM_ZERODIVIDE 0x8
#define _EM_INVALID    0x10

#define _MCW_RC        0x300
#define _RC_NEAR       0x0
#define _RC_DOWN       0x100
#define _RC_UP         0x200
#define _RC_CHOP       0x300

#define _MCW_PC        0x30000
#define _PC_64         0x0
#define _PC_53         0x10000
#define _PC_24         0x20000

#define _MCW_IC        0x40000
#define _IC_AFFINE     0x40000
#define _IC_PROJECTIVE 0x0

#define _EM_DENORMAL   0x80000
#endif

typedef struct __MSVCRT_complex
{
  double real;
  double imaginary;
} MSVCRT_complex;

typedef struct __MSVCRT_exception
{
  int type;
  char *name;
  double arg1;
  double arg2;
  double retval;
} MSVCRT_exception;


typedef int (*MSVCRT_matherr_func)(MSVCRT_exception *);

static MSVCRT_matherr_func MSVCRT_default_matherr_func = NULL;

#if defined(__GNUC__) && defined(__i386__)

#define FPU_DOUBLE(var) double var; \
  __asm__ __volatile__( "fstpl %0;fwait" : "=m" (var) : )
#define FPU_DOUBLES(var1,var2) double var1,var2; \
  __asm__ __volatile__( "fstpl %0;fwait" : "=m" (var2) : ); \
  __asm__ __volatile__( "fstpl %0;fwait" : "=m" (var1) : )

/*********************************************************************
 *		_CIacos (MSVCRT.@)
 */
double _CIacos(void)
{
  FPU_DOUBLE(x);
  if (x < -1.0 || x > 1.0 || !finite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return acos(x);
}

/*********************************************************************
 *		_CIasin (MSVCRT.@)
 */
double _CIasin(void)
{
  FPU_DOUBLE(x);
  if (x < -1.0 || x > 1.0 || !finite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return asin(x);
}

/*********************************************************************
 *		_CIatan (MSVCRT.@)
 */
double _CIatan(void)
{
  FPU_DOUBLE(x);
  if (!finite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return atan(x);
}

/*********************************************************************
 *		_CIatan2 (MSVCRT.@)
 */
double _CIatan2(void)
{
  FPU_DOUBLES(x,y);
  if (!finite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return atan2(x,y);
}

/*********************************************************************
 *		_CIcos (MSVCRT.@)
 */
double _CIcos(void)
{
  FPU_DOUBLE(x);
  if (!finite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return cos(x);
}

/*********************************************************************
 *		_CIcosh (MSVCRT.@)
 */
double _CIcosh(void)
{
  FPU_DOUBLE(x);
  if (!finite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return cosh(x);
}

/*********************************************************************
 *		_CIexp (MSVCRT.@)
 */
double _CIexp(void)
{
  FPU_DOUBLE(x);
  if (!finite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return exp(x);
}

/*********************************************************************
 *		_CIfmod (MSVCRT.@)
 */
double _CIfmod(void)
{
  FPU_DOUBLES(x,y);
  if (!finite(x) || !finite(y)) *MSVCRT__errno() = MSVCRT_EDOM;
  return fmod(x,y);
}

/*********************************************************************
 *		_CIlog (MSVCRT.@)
 */
double _CIlog(void)
{
  FPU_DOUBLE(x);
  if (x < 0.0 || !finite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  if (x == 0.0) *MSVCRT__errno() = MSVCRT_ERANGE;
  return log(x);
}

/*********************************************************************
 *		_CIlog10 (MSVCRT.@)
 */
double _CIlog10(void)
{
  FPU_DOUBLE(x);
  if (x < 0.0 || !finite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  if (x == 0.0) *MSVCRT__errno() = MSVCRT_ERANGE;
  return log10(x);
}

/*********************************************************************
 *		_CIpow (MSVCRT.@)
 */
double _CIpow(void)
{
  double z;
  FPU_DOUBLES(x,y);
  /* FIXME: If x < 0 and y is not integral, set EDOM */
  z = pow(x,y);
  if (!finite(z)) *MSVCRT__errno() = MSVCRT_EDOM;
  return z;
}

/*********************************************************************
 *		_CIsin (MSVCRT.@)
 */
double _CIsin(void)
{
  FPU_DOUBLE(x);
  if (!finite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return sin(x);
}

/*********************************************************************
 *		_CIsinh (MSVCRT.@)
 */
double _CIsinh(void)
{
  FPU_DOUBLE(x);
  if (!finite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return sinh(x);
}

/*********************************************************************
 *		_CIsqrt (MSVCRT.@)
 */
double _CIsqrt(void)
{
  FPU_DOUBLE(x);
  if (x < 0.0 || !finite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return sqrt(x);
}

/*********************************************************************
 *		_CItan (MSVCRT.@)
 */
double _CItan(void)
{
  FPU_DOUBLE(x);
  if (!finite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return tan(x);
}

/*********************************************************************
 *		_CItanh (MSVCRT.@)
 */
double _CItanh(void)
{
  FPU_DOUBLE(x);
  if (!finite(x)) *MSVCRT__errno() = MSVCRT_EDOM;
  return tanh(x);
}

#else /* defined(__GNUC__) && defined(__i386__) */

/* The above cannot be called on non x86 platforms, stub them for linking */

#define IX86_ONLY(func) double func(void) { return 0.0; }

IX86_ONLY(_CIacos)
IX86_ONLY(_CIasin)
IX86_ONLY(_CIatan)
IX86_ONLY(_CIatan2)
IX86_ONLY(_CIcos)
IX86_ONLY(_CIcosh)
IX86_ONLY(_CIexp)
IX86_ONLY(_CIfmod)
IX86_ONLY(_CIlog)
IX86_ONLY(_CIlog10)
IX86_ONLY(_CIpow)
IX86_ONLY(_CIsin)
IX86_ONLY(_CIsinh)
IX86_ONLY(_CIsqrt)
IX86_ONLY(_CItan)
IX86_ONLY(_CItanh)

#endif /* defined(__GNUC__) && defined(__i386__) */

/*********************************************************************
 *		_fpclass (MSVCRT.@)
 */
int _fpclass(double num)
{
#if defined(HAVE_FPCLASS) || defined(fpclass)
  switch (fpclass( num ))
  {
  case FP_SNAN:  return _FPCLASS_SNAN;
  case FP_QNAN:  return _FPCLASS_QNAN;
  case FP_NINF:  return _FPCLASS_NINF;
  case FP_PINF:  return _FPCLASS_PINF;
  case FP_NDENORM: return _FPCLASS_ND;
  case FP_PDENORM: return _FPCLASS_PD;
  case FP_NZERO: return _FPCLASS_NZ;
  case FP_PZERO: return _FPCLASS_PZ;
  case FP_NNORM: return _FPCLASS_NN;
  case FP_PNORM: return _FPCLASS_PN;
  }
  return _FPCLASS_PN;
#elif defined (fpclassify)
  switch (fpclassify( num ))
  {
  case FP_NAN: return _FPCLASS_QNAN;
  case FP_INFINITE: return signbit(num) ? _FPCLASS_NINF : _FPCLASS_PINF;
  case FP_SUBNORMAL: return signbit(num) ?_FPCLASS_ND : _FPCLASS_PD;
  case FP_ZERO: return signbit(num) ? _FPCLASS_NZ : _FPCLASS_PZ;
  }
  return signbit(num) ? _FPCLASS_NN : _FPCLASS_PN;
#else
  if (!finite(num))
    return _FPCLASS_QNAN;
  return num == 0.0 ? _FPCLASS_PZ : (num < 0 ? _FPCLASS_NN : _FPCLASS_PN);
#endif
}

/*********************************************************************
 *		_rotl (MSVCRT.@)
 */
unsigned int _rotl(unsigned int num, int shift)
{
  shift &= 31;
  return (num << shift) | (num >> (32-shift));
}

/*********************************************************************
 *		_logb (MSVCRT.@)
 */
double _logb(double num)
{
  if (!finite(num)) *MSVCRT__errno() = MSVCRT_EDOM;
  return logb(num);
}

/*********************************************************************
 *		_lrotl (MSVCRT.@)
 */
unsigned long _lrotl(unsigned long num, int shift)
{
  shift &= 0x1f;
  return (num << shift) | (num >> (32-shift));
}

/*********************************************************************
 *		_lrotr (MSVCRT.@)
 */
unsigned long _lrotr(unsigned long num, int shift)
{
  shift &= 0x1f;
  return (num >> shift) | (num << (32-shift));
}

/*********************************************************************
 *		_rotr (MSVCRT.@)
 */
unsigned int _rotr(unsigned int num, int shift)
{
    shift &= 0x1f;
    return (num >> shift) | (num << (32-shift));
}

/*********************************************************************
 *		_scalb (MSVCRT.@)
 */
double _scalb(double num, long power)
{
  /* Note - Can't forward directly as libc expects y as double */
  double dblpower = (double)power;
  if (!finite(num)) *MSVCRT__errno() = MSVCRT_EDOM;
  return scalb(num, dblpower);
}

/*********************************************************************
 *		_matherr (MSVCRT.@)
 */
int _matherr(MSVCRT_exception *e)
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
void MSVCRT___setusermatherr(MSVCRT_matherr_func func)
{
  MSVCRT_default_matherr_func = func;
  TRACE(":new matherr handler %p\n", func);
}

/**********************************************************************
 *		_statusfp (MSVCRT.@)
 */
unsigned int _statusfp(void)
{
   unsigned int retVal = 0;
#if defined(__GNUC__) && defined(__i386__)
  unsigned int fpword;

  __asm__ __volatile__( "fstsw %0" : "=m" (fpword) : );
  if (fpword & 0x1)  retVal |= _SW_INVALID;
  if (fpword & 0x2)  retVal |= _SW_DENORMAL;
  if (fpword & 0x4)  retVal |= _SW_ZERODIVIDE;
  if (fpword & 0x8)  retVal |= _SW_OVERFLOW;
  if (fpword & 0x10) retVal |= _SW_UNDERFLOW;
  if (fpword & 0x20) retVal |= _SW_INEXACT;
#else
  FIXME(":Not implemented!\n");
#endif
  return retVal;
}

/*********************************************************************
 *		_clearfp (MSVCRT.@)
 */
unsigned int _clearfp(void)
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
int *__fpecode(void)
{
    return &msvcrt_get_thread_data()->fpecode;
}

/*********************************************************************
 *		ldexp (MSVCRT.@)
 */
double MSVCRT_ldexp(double num, long exp)
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
double _cabs(MSVCRT_complex num)
{
  return sqrt(num.real * num.real + num.imaginary * num.imaginary);
}

/*********************************************************************
 *		_chgsign (MSVCRT.@)
 */
double _chgsign(double num)
{
  /* FIXME: +-infinity,Nan not tested */
  return -num;
}

/*********************************************************************
 *		_control87 (MSVCRT.@)
 */
unsigned int _control87(unsigned int newval, unsigned int mask)
{
#if defined(__GNUC__) && defined(__i386__)
  unsigned int fpword = 0;
  unsigned int flags = 0;

  TRACE("(%08x, %08x): Called\n", newval, mask);

  /* Get fp control word */
  __asm__ __volatile__( "fstcw %0" : "=m" (fpword) : );

  TRACE("Control word before : %08x\n", fpword);

  /* Convert into mask constants */
  if (fpword & 0x1)  flags |= _EM_INVALID;
  if (fpword & 0x2)  flags |= _EM_DENORMAL;
  if (fpword & 0x4)  flags |= _EM_ZERODIVIDE;
  if (fpword & 0x8)  flags |= _EM_OVERFLOW;
  if (fpword & 0x10) flags |= _EM_UNDERFLOW;
  if (fpword & 0x20) flags |= _EM_INEXACT;
  switch(fpword & 0xC00) {
  case 0xC00: flags |= _RC_UP|_RC_DOWN; break;
  case 0x800: flags |= _RC_UP; break;
  case 0x400: flags |= _RC_DOWN; break;
  }
  switch(fpword & 0x300) {
  case 0x0:   flags |= _PC_24; break;
  case 0x200: flags |= _PC_53; break;
  case 0x300: flags |= _PC_64; break;
  }
  if (fpword & 0x1000) flags |= _IC_AFFINE;

  /* Mask with parameters */
  flags = (flags & ~mask) | (newval & mask);

  /* Convert (masked) value back to fp word */
  fpword = 0;
  if (flags & _EM_INVALID)    fpword |= 0x1;
  if (flags & _EM_DENORMAL)   fpword |= 0x2;
  if (flags & _EM_ZERODIVIDE) fpword |= 0x4;
  if (flags & _EM_OVERFLOW)   fpword |= 0x8;
  if (flags & _EM_UNDERFLOW)  fpword |= 0x10;
  if (flags & _EM_INEXACT)    fpword |= 0x20;
  switch(flags & (_RC_UP | _RC_DOWN)) {
  case _RC_UP|_RC_DOWN: fpword |= 0xC00; break;
  case _RC_UP:          fpword |= 0x800; break;
  case _RC_DOWN:        fpword |= 0x400; break;
  }
  switch (flags & (_PC_24 | _PC_53)) {
  case _PC_64: fpword |= 0x300; break;
  case _PC_53: fpword |= 0x200; break;
  case _PC_24: fpword |= 0x0; break;
  }
  if (flags & _IC_AFFINE) fpword |= 0x1000;

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
unsigned int _controlfp(unsigned int newval, unsigned int mask)
{
#ifdef __i386__
  return _control87( newval, mask & ~_EM_DENORMAL );
#else
  FIXME(":Not Implemented!\n");
  return 0;
#endif
}

/*********************************************************************
 *		_copysign (MSVCRT.@)
 */
double _copysign(double num, double sign)
{
  /* FIXME: Behaviour for Nan/Inf? */
  if (sign < 0.0)
    return num < 0.0 ? num : -num;
  return num < 0.0 ? -num : num;
}

/*********************************************************************
 *		_finite (MSVCRT.@)
 */
int  _finite(double num)
{
  return (finite(num)?1:0); /* See comment for _isnan() */
}

/*********************************************************************
 *		_fpreset (MSVCRT.@)
 */
void _fpreset(void)
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
INT  _isnan(double num)
{
  /* Some implementations return -1 for true(glibc), msvcrt/crtdll return 1.
   * Do the same, as the result may be used in calculations
   */
  return isnan(num) ? 1 : 0;
}

/*********************************************************************
 *		_y0 (MSVCRT.@)
 */
double _y0(double num)
{
  double retval;
  if (!finite(num)) *MSVCRT__errno() = MSVCRT_EDOM;
  retval  = y0(num);
  if (_fpclass(retval) == _FPCLASS_NINF)
  {
    *MSVCRT__errno() = MSVCRT_EDOM;
    retval = sqrt(-1);
  }
  return retval;
}

/*********************************************************************
 *		_y1 (MSVCRT.@)
 */
double _y1(double num)
{
  double retval;
  if (!finite(num)) *MSVCRT__errno() = MSVCRT_EDOM;
  retval  = y1(num);
  if (_fpclass(retval) == _FPCLASS_NINF)
  {
    *MSVCRT__errno() = MSVCRT_EDOM;
    retval = sqrt(-1);
  }
  return retval;
}

/*********************************************************************
 *		_yn (MSVCRT.@)
 */
double _yn(int order, double num)
{
  double retval;
  if (!finite(num)) *MSVCRT__errno() = MSVCRT_EDOM;
  retval  = yn(order,num);
  if (_fpclass(retval) == _FPCLASS_NINF)
  {
    *MSVCRT__errno() = MSVCRT_EDOM;
    retval = sqrt(-1);
  }
  return retval;
}

/*********************************************************************
 *		_nextafter (MSVCRT.@)
 */
double _nextafter(double num, double next)
{
  double retval;
  if (!finite(num) || !finite(next)) *MSVCRT__errno() = MSVCRT_EDOM;
  retval = nextafter(num,next);
  return retval;
}

/*********************************************************************
 *		_ecvt (MSVCRT.@)
 */
char *_ecvt( double number, int ndigits, int *decpt, int *sign )
{
    MSVCRT_thread_data *data = msvcrt_get_thread_data();
    char *dec;

    if (!data->efcvt_buffer)
        data->efcvt_buffer = MSVCRT_malloc( 80 ); /* ought to be enough */

    snprintf(data->efcvt_buffer, 80, "%.*e", ndigits /* FIXME wrong */, number);
    *sign = (number < 0);
    dec = strchr(data->efcvt_buffer, '.');
    *decpt = (dec) ? dec - data->efcvt_buffer : -1;
    return data->efcvt_buffer;
}

/***********************************************************************
 *		_fcvt  (MSVCRT.@)
 */
char *_fcvt( double number, int ndigits, int *decpt, int *sign )
{
    MSVCRT_thread_data *data = msvcrt_get_thread_data();
    char *dec;

    if (!data->efcvt_buffer)
        data->efcvt_buffer = MSVCRT_malloc( 80 ); /* ought to be enough */

    snprintf(data->efcvt_buffer, 80, "%.*e", ndigits, number);
    *sign = (number < 0);
    dec = strchr(data->efcvt_buffer, '.');
    *decpt = (dec) ? dec - data->efcvt_buffer : -1;
    return data->efcvt_buffer;
}

/***********************************************************************
 *		_gcvt  (MSVCRT.@)
 *
 * FIXME: uses both E and F.
 */
char *_gcvt( double number, int ndigit, char *buff )
{
    sprintf(buff, "%.*E", ndigit, number);
    return buff;
}

#include <stdlib.h> /* div_t, ldiv_t */

/*********************************************************************
 *		div (MSVCRT.@)
 * VERSION
 *	[i386] Windows binary compatible - returns the struct in eax/edx.
 */
#ifdef __i386__
unsigned __int64 MSVCRT_div(int num, int denom)
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
MSVCRT_div_t MSVCRT_div(int num, int denom)
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
unsigned __int64 MSVCRT_ldiv(long num, long denom)
{
  ldiv_t ldt = ldiv(num,denom);
  return ((unsigned __int64)ldt.rem << 32) | (unsigned long)ldt.quot;
}
#else
/*********************************************************************
 *		ldiv (MSVCRT.@)
 * VERSION
 *	[!i386] Non-x86 can't run win32 apps so we don't need binary compatibility
 */
MSVCRT_ldiv_t MSVCRT_ldiv(long num, long denom)
{
  ldiv_t result = ldiv(num,denom);

  MSVCRT_ldiv_t ret;
  ret.quot = result.quot;
  ret.rem = result.rem;

  return ret;
}
#endif /* ifdef __i386__ */

/***********************************************************************
 *		_adj_fdiv_m16i (MSVCRT.@)
 * FIXME
 *    This function is likely to have the wrong number of arguments.
 *
 * NOTE
 *    I _think_ this function is intended to work around the Pentium
 *    fdiv bug.
 */
void _adj_fdiv_m16i(void)
{
  TRACE("(): stub\n");
}

/***********************************************************************
 *		_adj_fdiv_m32 (MSVCRT.@)
 * FIXME
 *    This function is likely to have the wrong number of arguments.
 *
 * NOTE
 *    I _think_ this function is intended to work around the Pentium
 *    fdiv bug.
 */
void _adj_fdiv_m32(void)
{
  TRACE("(): stub\n");
}

/***********************************************************************
 *		_adj_fdiv_m32i (MSVCRT.@)
 * FIXME
 *    This function is likely to have the wrong number of arguments.
 *
 * NOTE
 *    I _think_ this function is intended to work around the Pentium
 *    fdiv bug.
 */
void _adj_fdiv_m32i(void)
{
  TRACE("(): stub\n");
}

/***********************************************************************
 *		_adj_fdiv_m64 (MSVCRT.@)
 * FIXME
 *    This function is likely to have the wrong number of arguments.
 *
 * NOTE
 *    I _think_ this function is intended to work around the Pentium
 *    fdiv bug.
 */
void _adj_fdiv_m64(void)
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
 * FIXME
 *    This function is likely to have the wrong number of arguments.
 *
 * NOTE
 *    I _think_ this function is intended to work around the Pentium
 *    fdiv bug.
 */
void _adj_fdivr_m16i(void)
{
  TRACE("(): stub\n");
}

/***********************************************************************
 *		_adj_fdivr_m32 (MSVCRT.@)
 * FIXME
 *    This function is likely to have the wrong number of arguments.
 *
 * NOTE
 *    I _think_ this function is intended to work around the Pentium
 *    fdiv bug.
 */
void _adj_fdivr_m32(void)
{
  TRACE("(): stub\n");
}

/***********************************************************************
 *		_adj_fdivr_m32i (MSVCRT.@)
 * FIXME
 *    This function is likely to have the wrong number of arguments.
 *
 * NOTE
 *    I _think_ this function is intended to work around the Pentium
 *    fdiv bug.
 */
void _adj_fdivr_m32i(void)
{
  TRACE("(): stub\n");
}

/***********************************************************************
 *		_adj_fdivr_m64 (MSVCRT.@)
 * FIXME
 *    This function is likely to have the wrong number of arguments.
 *
 * NOTE
 *    I _think_ this function is intended to work around the Pentium
 *    fdiv bug.
 */
void _adj_fdivr_m64(void)
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
 *		_adjust_fdiv (MSVCRT.@)
 * FIXME
 *    I _think_ this function should be a variable indicating whether
 *    Pentium fdiv bug safe code should be used.
 */
void _adjust_fdiv(void)
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
