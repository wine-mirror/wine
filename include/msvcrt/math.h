/*
 * Math functions.
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Hans Leidekker.
 * This file is in the public domain.
 */

#ifndef __WINE_MATH_H
#define __WINE_MATH_H
#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
#endif

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

double sin(double);
double cos(double);
double tan(double);
double sinh(double);
double cosh(double);
double tanh(double);
double asin(double);
double acos(double);
double atan(double);
double atan2(double, double);
double exp(double);
double log(double);
double log10(double);
double pow(double, double);
double sqrt(double);
double ceil(double);
double floor(double);
double fabs(double);
double ldexp(double, int);
double frexp(double, int*);
double modf(double, double*);
double fmod(double, double);

double hypot(double, double);
double j0(double);
double j1(double);
double jn(int, double);
double y0(double);
double y1(double);
double yn(int, double);

int _matherr(struct _exception*);
double _cabs(struct _complex);

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
