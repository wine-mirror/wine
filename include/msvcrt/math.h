/*
 * Math functions.
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Hans Leidekker.
 * This file is in the public domain.
 */

#ifndef __WINE_MATH_H
#define __WINE_MATH_H

#include <crtdefs.h>

#include <pshpack8.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _DOMAIN         1       /* domain error in argument */
#define _SING           2       /* singularity */
#define _OVERFLOW       3       /* range overflow */
#define _UNDERFLOW      4       /* range underflow */
#define _TLOSS          5       /* total loss of precision */
#define _PLOSS          6       /* partial loss of precision */

#ifndef _EXCEPTION_DEFINED
#define _EXCEPTION_DEFINED
struct _exception
{
  int     type;
  char    *name;
  double  arg1;
  double  arg2;
  double  retval;
};
#endif /* _EXCEPTION_DEFINED */

#ifndef _COMPLEX_DEFINED
#define _COMPLEX_DEFINED
struct _complex
{
  double x;      /* Real part */
  double y;      /* Imaginary part */
};
#endif /* _COMPLEX_DEFINED */

double __cdecl sin(double);
double __cdecl cos(double);
double __cdecl tan(double);
double __cdecl sinh(double);
double __cdecl cosh(double);
double __cdecl tanh(double);
double __cdecl asin(double);
double __cdecl acos(double);
double __cdecl atan(double);
double __cdecl atan2(double, double);
double __cdecl exp(double);
double __cdecl log(double);
double __cdecl log10(double);
double __cdecl pow(double, double);
double __cdecl sqrt(double);
double __cdecl ceil(double);
double __cdecl floor(double);
double __cdecl fabs(double);
double __cdecl ldexp(double, int);
double __cdecl frexp(double, int*);
double __cdecl modf(double, double*);
double __cdecl fmod(double, double);

double __cdecl hypot(double, double);
double __cdecl j0(double);
double __cdecl j1(double);
double __cdecl jn(int, double);
double __cdecl y0(double);
double __cdecl y1(double);
double __cdecl yn(int, double);

int __cdecl _matherr(struct _exception*);
double __cdecl _cabs(struct _complex);

#ifndef HUGE_VAL
#  if defined(__GNUC__) && (__GNUC__ >= 3)
#    define HUGE_VAL    (__extension__ 0x1.0p2047)
#  else
static const union {
    unsigned char __c[8];
    double __d;
} __huge_val = { { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f } };
#    define HUGE_VAL    (__huge_val.__d)
#  endif
#endif

#ifdef __cplusplus
}
#endif

#include <poppack.h>

#endif /* __WINE_MATH_H */
