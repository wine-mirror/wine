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

#ifndef MSVCRT
# ifdef USE_MSVCRT_PREFIX
#  define MSVCRT(x)    MSVCRT_##x
# else
#  define MSVCRT(x)    x
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define _DOMAIN         1       /* domain error in argument */
#define _SING           2       /* singularity */
#define _OVERFLOW       3       /* range overflow */
#define _UNDERFLOW      4       /* range underflow */
#define _TLOSS          5       /* total loss of precision */
#define _PLOSS          6       /* partial loss of precision */

#ifndef MSVCRT_EXCEPTION_DEFINED
#define MSVCRT_EXCEPTION_DEFINED
struct MSVCRT(_exception)
{
  int     type;
  char    *name;
  double  arg1;
  double  arg2;
  double  retval;
};
#endif /* MSVCRT_EXCEPTION_DEFINED */

#ifndef MSVCRT_COMPLEX_DEFINED
#define MSVCRT_COMPLEX_DEFINED
struct MSVCRT(_complex)
{
  double x;      /* Real part */
  double y;      /* Imaginary part */
};
#endif /* MSVCRT_COMPLEX_DEFINED */

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

int MSVCRT(_matherr)(struct MSVCRT(_exception)*);
double MSVCRT(_cabs)(struct MSVCRT(_complex));

#ifdef __cplusplus
}
#endif

#endif /* __WINE_MATH_H */
