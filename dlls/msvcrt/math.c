/*
 * msvcrt.dll math functions
 *
 * Copyright 2000 Jon Griffiths
 */
#include "config.h"
#include "msvcrt.h"
#include "ms_errno.h"

#define __USE_ISOC9X 1
#define __USE_ISOC99 1
#include <math.h>
#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif

DEFAULT_DEBUG_CHANNEL(msvcrt);

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


typedef int (__cdecl *MSVCRT_matherr_func)(MSVCRT_exception *);

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
double __cdecl MSVCRT__CIacos(void)
{
  FPU_DOUBLE(x);
  if (x < -1.0 || x > 1.0 || !finite(x)) SET_THREAD_VAR(errno,MSVCRT_EDOM);
  return acos(x);
}

/*********************************************************************
 *		_CIasin (MSVCRT.@)
 */
double __cdecl MSVCRT__CIasin(void)
{
  FPU_DOUBLE(x);
  if (x < -1.0 || x > 1.0 || !finite(x)) SET_THREAD_VAR(errno,MSVCRT_EDOM);
  return asin(x);
}

/*********************************************************************
 *		_CIatan (MSVCRT.@)
 */
double __cdecl MSVCRT__CIatan(void)
{
  FPU_DOUBLE(x);
  if (!finite(x)) SET_THREAD_VAR(errno,MSVCRT_EDOM);
  return atan(x);
}

/*********************************************************************
 *		_CIatan2 (MSVCRT.@)
 */
double __cdecl MSVCRT__CIatan2(void)
{
  FPU_DOUBLES(x,y);
  if (!finite(x)) SET_THREAD_VAR(errno,MSVCRT_EDOM);
  return atan2(x,y);
}

/*********************************************************************
 *		_CIcos (MSVCRT.@)
 */
double __cdecl MSVCRT__CIcos(void)
{
  FPU_DOUBLE(x);
  if (!finite(x)) SET_THREAD_VAR(errno,MSVCRT_EDOM);
  return cos(x);
}

/*********************************************************************
 *		_CIcosh (MSVCRT.@)
 */
double __cdecl MSVCRT__CIcosh(void)
{
  FPU_DOUBLE(x);
  if (!finite(x)) SET_THREAD_VAR(errno,MSVCRT_EDOM);
  return cosh(x);
}

/*********************************************************************
 *		_CIexp (MSVCRT.@)
 */
double __cdecl MSVCRT__CIexp(void)
{
  FPU_DOUBLE(x);
  if (!finite(x)) SET_THREAD_VAR(errno,MSVCRT_EDOM);
  return exp(x);
}

/*********************************************************************
 *		_CIfmod (MSVCRT.@)
 */
double __cdecl MSVCRT__CIfmod(void)
{
  FPU_DOUBLES(x,y);
  if (!finite(x) || !finite(y)) SET_THREAD_VAR(errno,MSVCRT_EDOM);
  return fmod(x,y);
}

/*********************************************************************
 *		_CIlog (MSVCRT.@)
 */
double __cdecl MSVCRT__CIlog(void)
{
  FPU_DOUBLE(x);
  if (x < 0.0 || !finite(x)) SET_THREAD_VAR(errno,MSVCRT_EDOM);
  if (x == 0.0) SET_THREAD_VAR(errno,MSVCRT_ERANGE);
  return log(x);
}

/*********************************************************************
 *		_CIlog10 (MSVCRT.@)
 */
double __cdecl MSVCRT__CIlog10(void)
{
  FPU_DOUBLE(x);
  if (x < 0.0 || !finite(x)) SET_THREAD_VAR(errno,MSVCRT_EDOM);
  if (x == 0.0) SET_THREAD_VAR(errno,MSVCRT_ERANGE);
  return log10(x);
}

/*********************************************************************
 *		_CIpow (MSVCRT.@)
 */
double __cdecl MSVCRT__CIpow(void)
{
  double z;
  FPU_DOUBLES(x,y);
  /* FIXME: If x < 0 and y is not integral, set EDOM */
  z = pow(x,y);
  if (!finite(z)) SET_THREAD_VAR(errno,MSVCRT_EDOM);
  return z;
}

/*********************************************************************
 *		_CIsin (MSVCRT.@)
 */
double __cdecl MSVCRT__CIsin(void)
{
  FPU_DOUBLE(x);
  if (!finite(x)) SET_THREAD_VAR(errno,MSVCRT_EDOM);
  return sin(x);
}

/*********************************************************************
 *		_CIsinh (MSVCRT.@)
 */
double __cdecl MSVCRT__CIsinh(void)
{
  FPU_DOUBLE(x);
  if (!finite(x)) SET_THREAD_VAR(errno,MSVCRT_EDOM);
  return sinh(x);
}

/*********************************************************************
 *		_CIsqrt (MSVCRT.@)
 */
double __cdecl MSVCRT__CIsqrt(void)
{
  FPU_DOUBLE(x);
  if (x < 0.0 || !finite(x)) SET_THREAD_VAR(errno,MSVCRT_EDOM);
  return sqrt(x);
}

/*********************************************************************
 *		_CItan (MSVCRT.@)
 */
double __cdecl MSVCRT__CItan(void)
{
  FPU_DOUBLE(x);
  if (!finite(x)) SET_THREAD_VAR(errno,MSVCRT_EDOM);
  return tan(x);
}

/*********************************************************************
 *		_CItanh (MSVCRT.@)
 */
double __cdecl MSVCRT__CItanh(void)
{
  FPU_DOUBLE(x);
  if (!finite(x)) SET_THREAD_VAR(errno,MSVCRT_EDOM);
  return tanh(x);
}

#else /* defined(__GNUC__) && defined(__i386__) */

/* The above cannot be called on non x86 platforms, stub them for linking */

#define IX86_ONLY(func) double __cdecl MSVCRT_##func(void) { return 0.0; }

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
int __cdecl MSVCRT__fpclass(double num)
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
unsigned int __cdecl MSVCRT__rotl(unsigned int num, int shift)
{
  shift &= 31;
  return (num << shift) | (num >> (32-shift));
}

/*********************************************************************
 *		_logb (MSVCRT.@)
 */
double __cdecl MSVCRT__logb(double num)
{
  if (!finite(num)) SET_THREAD_VAR(errno,MSVCRT_EDOM);
  return logb(num);
}

/*********************************************************************
 *		_lrotl (MSVCRT.@)
 */
unsigned long __cdecl MSVCRT__lrotl(unsigned long num, int shift)
{
  shift &= 0x1f;
  return (num << shift) | (num >> (32-shift));
}

/*********************************************************************
 *		_lrotr (MSVCRT.@)
 */
unsigned long __cdecl MSVCRT__lrotr(unsigned long num, int shift)
{
  shift &= 0x1f;
  return (num >> shift) | (num << (32-shift));
}

/*********************************************************************
 *		_rotr (MSVCRT.@)
 */
unsigned int __cdecl MSVCRT__rotr(unsigned int num, int shift)
{
    shift &= 0x1f;
    return (num >> shift) | (num << (32-shift));
}

/*********************************************************************
 *		_scalb (MSVCRT.@)
 */
double __cdecl MSVCRT__scalb(double num, long power)
{
  /* Note - Can't forward directly as libc expects y as double */
  double dblpower = (double)power;
  if (!finite(num)) SET_THREAD_VAR(errno,MSVCRT_EDOM);
  return scalb(num, dblpower);
}

/*********************************************************************
 *		_matherr (MSVCRT.@)
 */
int __cdecl MSVCRT__matherr(MSVCRT_exception *e)
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
void __cdecl MSVCRT___setusermatherr(MSVCRT_matherr_func func)
{
  MSVCRT_default_matherr_func = func;
  TRACE(":new matherr handler %p\n", func);
}

/**********************************************************************
 *		_statusfp (MSVCRT.@)
 */
unsigned int __cdecl MSVCRT__statusfp(void)
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
unsigned int __cdecl MSVCRT__clearfp(void)
{
  unsigned int retVal = MSVCRT__statusfp();
#if defined(__GNUC__) && defined(__i386__)
  __asm__ __volatile__( "fnclex" );
#else
  FIXME(":Not Implemented\n");
#endif
  return retVal;
}

/*********************************************************************
 *		ldexp (MSVCRT.@)
 */
double __cdecl MSVCRT_ldexp(double num, long exp)
{
  double z = ldexp(num,exp);

  if (!finite(z))
    SET_THREAD_VAR(errno,MSVCRT_ERANGE);
  else if (z == 0 && signbit(z))
    z = 0.0; /* Convert -0 -> +0 */
  return z;
}

/*********************************************************************
 *		_cabs (MSVCRT.@)
 */
double __cdecl MSVCRT__cabs(MSVCRT_complex num)
{
  return sqrt(num.real * num.real + num.imaginary * num.imaginary);
}

/*********************************************************************
 *		_chgsign (MSVCRT.@)
 */
double __cdecl MSVCRT__chgsign(double num)
{
  /* FIXME: +-infinity,Nan not tested */
  return -num;
}

/*********************************************************************
 *		_control87 (MSVCRT.@)
 */
unsigned int __cdecl MSVCRT__control87(unsigned int newval, unsigned int mask)
{
#if defined(__GNUC__) && defined(__i386__)
   unsigned int fpword, flags = 0;

  /* Get fp control word */
  __asm__ __volatile__( "fstsw %0" : "=m" (fpword) : );

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
  if (!(flags & _IC_AFFINE)) fpword |= 0x1000;

  /* Put fp control word */
  __asm__ __volatile__( "fldcw %0" : : "m" (fpword) );
  return fpword;
#else
  return  MSVCRT__controlfp( newval, mask );
#endif
}

/*********************************************************************
 *		_controlfp (MSVCRT.@)
 */
unsigned int __cdecl MSVCRT__controlfp(unsigned int newval, unsigned int mask)
{
#if defined(__GNUC__) && defined(__i386__)
  return MSVCRT__control87( newval, mask );
#else
  FIXME(":Not Implemented!\n");
  return 0;
#endif
}

/*********************************************************************
 *		_copysign (MSVCRT.@)
 */
double __cdecl MSVCRT__copysign(double num, double sign)
{
  /* FIXME: Behaviour for Nan/Inf? */
  if (sign < 0.0)
    return num < 0.0 ? num : -num;
  return num < 0.0 ? -num : num;
}

/*********************************************************************
 *		_finite (MSVCRT.@)
 */
int __cdecl  MSVCRT__finite(double num)
{
  return (finite(num)?1:0); /* See comment for _isnan() */
}

/*********************************************************************
 *		_fpreset (MSVCRT.@)
 */
void __cdecl MSVCRT__fpreset(void)
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
INT __cdecl  MSVCRT__isnan(double num)
{
  /* Some implementations return -1 for true(glibc), msvcrt/crtdll return 1.
   * Do the same, as the result may be used in calculations
   */
  return isnan(num) ? 1 : 0;
}

/*********************************************************************
 *		_y0 (MSVCRT.@)
 */
double __cdecl MSVCRT__y0(double num)
{
  double retval;
  if (!finite(num)) SET_THREAD_VAR(errno,MSVCRT_EDOM);
  retval  = y0(num);
  if (MSVCRT__fpclass(retval) == _FPCLASS_NINF)
  {
    SET_THREAD_VAR(errno,MSVCRT_EDOM);
    retval = sqrt(-1);
  }
  return retval;
}

/*********************************************************************
 *		_y1 (MSVCRT.@)
 */
double __cdecl MSVCRT__y1(double num)
{
  double retval;
  if (!finite(num)) SET_THREAD_VAR(errno,MSVCRT_EDOM);
  retval  = y1(num);
  if (MSVCRT__fpclass(retval) == _FPCLASS_NINF)
  {
    SET_THREAD_VAR(errno,MSVCRT_EDOM);
    retval = sqrt(-1);
  }
  return retval;
}

/*********************************************************************
 *		_yn (MSVCRT.@)
 */
double __cdecl MSVCRT__yn(int order, double num)
{
  double retval;
  if (!finite(num)) SET_THREAD_VAR(errno,MSVCRT_EDOM);
  retval  = yn(order,num);
  if (MSVCRT__fpclass(retval) == _FPCLASS_NINF)
  {
    SET_THREAD_VAR(errno,MSVCRT_EDOM);
    retval = sqrt(-1);
  }
  return retval;
}

/*********************************************************************
 *		_nextafter (MSVCRT.@)
 */
double __cdecl MSVCRT__nextafter(double num, double next)
{
  double retval;
  if (!finite(num) || !finite(next)) SET_THREAD_VAR(errno,MSVCRT_EDOM);
  retval = nextafter(num,next);
  return retval;
}

#include <stdlib.h> /* div_t, ldiv_t */

/*********************************************************************
 *		div (MSVCRT.@)
 * VERSION
 *	[i386] Windows binary compatible - returns the struct in eax/edx.
 */
#ifdef __i386__
LONGLONG __cdecl MSVCRT_div(int num, int denom)
{
  LONGLONG retval;
  div_t dt = div(num,denom);
  retval = ((LONGLONG)dt.rem << 32) | dt.quot;
  return retval;
}
#else
/*********************************************************************
 *		div (MSVCRT.@)
 * VERSION
 *	[!i386] Non-x86 can't run win32 apps so we don't need binary compatibility
 */
div_t __cdecl MSVCRT_div(int num, int denom)
{
  return div(num,denom);
}
#endif /* ifdef __i386__ */


/*********************************************************************
 *		ldiv (MSVCRT.@)
 * VERSION
 * 	[i386] Windows binary compatible - returns the struct in eax/edx.
 */
#ifdef __i386__
ULONGLONG __cdecl MSVCRT_ldiv(long num, long denom)
{
  ULONGLONG retval;
  ldiv_t ldt = ldiv(num,denom);
  retval = ((ULONGLONG)ldt.rem << 32) | (ULONG)ldt.quot;
  return retval;
}
#else
/*********************************************************************
 *		ldiv (MSVCRT.@)
 * VERSION
 *	[!i386] Non-x86 can't run win32 apps so we don't need binary compatibility
 */
ldiv_t __cdecl MSVCRT_ldiv(long num, long denom)
{
  return ldiv(num,denom);
}
#endif /* ifdef __i386__ */

/* I _think_ these functions are intended to work around the pentium fdiv bug */
#define DUMMY_FUNC(x) void __cdecl MSVCRT_##x(void) { TRACE("stub"); }
DUMMY_FUNC(_adj_fdiv_m16i)
DUMMY_FUNC(_adj_fdiv_m32)
DUMMY_FUNC(_adj_fdiv_m32i)
DUMMY_FUNC(_adj_fdiv_m64)
DUMMY_FUNC(_adj_fdiv_r)
DUMMY_FUNC(_adj_fdivr_m16i)
DUMMY_FUNC(_adj_fdivr_m32)
DUMMY_FUNC(_adj_fdivr_m32i)
DUMMY_FUNC(_adj_fdivr_m64)
DUMMY_FUNC(_adj_fpatan)
DUMMY_FUNC(_adj_fprem)
DUMMY_FUNC(_adj_fprem1)
DUMMY_FUNC(_adj_fptan)
DUMMY_FUNC(_adjust_fdiv)
DUMMY_FUNC(_safe_fdiv);
DUMMY_FUNC(_safe_fdivr);
DUMMY_FUNC(_safe_fprem);
DUMMY_FUNC(_safe_fprem1);

