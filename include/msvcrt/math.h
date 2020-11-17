/*
 * Math functions.
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Hans Leidekker.
 * This file is in the public domain.
 */

#ifndef __WINE_MATH_H
#define __WINE_MATH_H

#include <corecrt.h>

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
double __cdecl asinh(double);
double __cdecl acosh(double);
double __cdecl atanh(double);
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
double __cdecl fmin(double, double);
double __cdecl fmax(double, double);
double __cdecl erf(double);

double __cdecl _hypot(double, double);
double __cdecl _j0(double);
double __cdecl _j1(double);
double __cdecl _jn(int, double);
double __cdecl _y0(double);
double __cdecl _y1(double);
double __cdecl _yn(int, double);

double __cdecl cbrt(double);
double __cdecl exp2(double);
double __cdecl log2(double);
double __cdecl rint(double);
double __cdecl round(double);
double __cdecl trunc(double);

float __cdecl cbrtf(float);
float __cdecl exp2f(float);
float __cdecl log2f(float);
float __cdecl rintf(float);
float __cdecl roundf(float);
float __cdecl truncf(float);

long __cdecl lrint(double);
long __cdecl lrintf(float);
long __cdecl lround(double);
long __cdecl lroundf(float);

_ACRTIMP double __cdecl scalbn(double,int);
_ACRTIMP float  __cdecl scalbnf(float,int);

double __cdecl _copysign (double, double);
double __cdecl _chgsign (double);
double __cdecl _scalb(double, __msvcrt_long);
double __cdecl _logb(double);
double __cdecl _nextafter(double, double);
int    __cdecl _finite(double);
int    __cdecl _isnan(double);
int    __cdecl _fpclass(double);

#ifndef __i386__

float __cdecl sinf(float);
float __cdecl cosf(float);
float __cdecl tanf(float);
float __cdecl sinhf(float);
float __cdecl coshf(float);
float __cdecl tanhf(float);
float __cdecl asinf(float);
float __cdecl acosf(float);
float __cdecl atanf(float);
float __cdecl atan2f(float, float);
float __cdecl asinhf(float);
float __cdecl acoshf(float);
float __cdecl atanhf(float);
float __cdecl expf(float);
float __cdecl logf(float);
float __cdecl log10f(float);
float __cdecl powf(float, float);
float __cdecl sqrtf(float);
float __cdecl ceilf(float);
float __cdecl floorf(float);
float __cdecl fabsf(float);
float __cdecl frexpf(float, int*);
float __cdecl modff(float, float*);
float __cdecl fmodf(float, float);

float __cdecl _copysignf(float, float);
float __cdecl _chgsignf(float);
float __cdecl _logbf(float);
int   __cdecl _finitef(float);
int   __cdecl _isnanf(float);
int   __cdecl _fpclassf(float);

#else

static inline float sinf(float x) { return sin(x); }
static inline float cosf(float x) { return cos(x); }
static inline float tanf(float x) { return tan(x); }
static inline float sinhf(float x) { return sinh(x); }
static inline float coshf(float x) { return cosh(x); }
static inline float tanhf(float x) { return tanh(x); }
static inline float asinf(float x) { return asin(x); }
static inline float acosf(float x) { return acos(x); }
static inline float atanf(float x) { return atan(x); }
static inline float atan2f(float x, float y) { return atan2(x, y); }
static inline float asinhf(float x) { return asinh(x); }
static inline float acoshf(float x) { return acosh(x); }
static inline float atanhf(float x) { return atanh(x); }
static inline float expf(float x) { return exp(x); }
static inline float logf(float x) { return log(x); }
static inline float log10f(float x) { return log10(x); }
static inline float powf(float x, float y) { return pow(x, y); }
static inline float sqrtf(float x) { return sqrt(x); }
static inline float ceilf(float x) { return ceil(x); }
static inline float floorf(float x) { return floor(x); }
static inline float fabsf(float x) { return fabs(x); }
static inline float frexpf(float x, int *y) { return frexp(x, y); }
static inline float modff(float x, float *y) { double yd, ret = modf(x, &yd); *y = yd; return ret; }
static inline float fmodf(float x, float y) { return fmod(x, y); }

static inline float _copysignf(float x, float y) { return _copysign(x, y); }
static inline float _chgsignf(float x) { return _chgsign(x); }
static inline float _logbf(float x) { return _logb(x); }
static inline int   _finitef(float x) { return _finite(x); }
static inline int   _isnanf(float x) { return _isnan(x); }
static inline int   _fpclassf(float x) { return _fpclass(x); }

#endif

static inline float ldexpf(float x, int y) { return ldexp(x, y); }

#ifdef _UCRT
_ACRTIMP double __cdecl copysign(double, double);
_ACRTIMP float  __cdecl copysignf(float, float);
#else
#define copysign(x,y)  _copysign(x,y)
#define copysignf(x,y) _copysignf(x,y)
#endif

double __cdecl nearbyint(double);
float __cdecl nearbyintf(float);

float __cdecl _hypotf(float, float);

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

#if (defined(__GNUC__) && ((__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 3)))) || defined(__clang__)
# define INFINITY __builtin_inff()
# define NAN      __builtin_nanf("")
#else
static const union {
    unsigned int __i;
    float __f;
} __inff = { 0x7f800000 }, __nanf = { 0x7fc00000 };
# define INFINITY (__inff.__f)
# define NAN      (__nanf.__f)
#endif

#define FP_INFINITE   1
#define FP_NAN        2
#define FP_NORMAL    -1
#define FP_SUBNORMAL -2
#define FP_ZERO       0

short __cdecl _dclass(double);
short __cdecl _fdclass(float);
int   __cdecl _dsign(double);
int   __cdecl _fdsign(float);

static inline int __isnanf(float x)
{
    union { float x; unsigned int i; } u = { x };
    return (u.i & 0x7fffffff) > 0x7f800000;
}
static inline int __isnan(double x)
{
    union { double x; unsigned __int64 i; } u = { x };
    return (u.i & ~0ull >> 1) > 0x7ffull << 52;
}
static inline int __isinff(float x)
{
    union { float x; unsigned int i; } u = { x };
    return (u.i & 0x7fffffff) == 0x7f800000;
}
static inline int __isinf(double x)
{
    union { double x; unsigned __int64 i; } u = { x };
    return (u.i & ~0ull >> 1) == 0x7ffull << 52;
}
static inline int __isnormalf(float x)
{
    union { float x; unsigned int i; } u = { x };
    return ((u.i + 0x00800000) & 0x7fffffff) >= 0x01000000;
}
static inline int __isnormal(double x)
{
    union { double x; unsigned __int64 i; } u = { x };
    return ((u.i + (1ull << 52)) & ~0ull >> 1) >= 1ull << 53;
}
static inline int __signbitf(float x)
{
    union { float x; unsigned int i; } u = { x };
    return (int)(u.i >> 31);
}
static inline int __signbit(double x)
{
    union { double x; unsigned __int64 i; } u = { x };
    return (int)(u.i >> 63);
}

#define isinf(x)    (sizeof(x) == sizeof(float) ? __isinff(x) : __isinf(x))
#define isnan(x)    (sizeof(x) == sizeof(float) ? __isnanf(x) : __isnan(x))
#define isnormal(x) (sizeof(x) == sizeof(float) ? __isnormalf(x) : __isnormal(x))
#define signbit(x)  (sizeof(x) == sizeof(float) ? __signbitf(x) : __signbit(x))
#define isfinite(x) (!isinf(x) && !isnan(x))

#ifdef __cplusplus
}
#endif

#include <poppack.h>

#if !defined(__STRICT_ANSI__) || defined(_POSIX_C_SOURCE) || defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE) || defined(_GNU_SOURCE) || defined(_BSD_SOURCE) || defined(_USE_MATH_DEFINES)
#ifndef _MATH_DEFINES_DEFINED
#define _MATH_DEFINES_DEFINED
#define M_E         2.71828182845904523536
#define M_LOG2E     1.44269504088896340736
#define M_LOG10E    0.434294481903251827651
#define M_LN2       0.693147180559945309417
#define M_LN10      2.30258509299404568402
#define M_PI        3.14159265358979323846
#define M_PI_2      1.57079632679489661923
#define M_PI_4      0.785398163397448309616
#define M_1_PI      0.318309886183790671538
#define M_2_PI      0.636619772367581343076
#define M_2_SQRTPI  1.12837916709551257390
#define M_SQRT2     1.41421356237309504880
#define M_SQRT1_2   0.707106781186547524401
#endif /* !_MATH_DEFINES_DEFINED */
#endif /* _USE_MATH_DEFINES */

static inline double hypot( double x, double y ) { return _hypot( x, y ); }
static inline double j0( double x ) { return _j0( x ); }
static inline double j1( double x ) { return _j1( x ); }
static inline double jn( int n, double x ) { return _jn( n, x ); }
static inline double y0( double x ) { return _y0( x ); }
static inline double y1( double x ) { return _y1( x ); }
static inline double yn( int n, double x ) { return _yn( n, x ); }

static inline float hypotf( float x, float y ) { return _hypotf( x, y ); }

#endif /* __WINE_MATH_H */
