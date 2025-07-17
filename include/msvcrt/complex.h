/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the Wine project.
 */

#ifndef _COMPLEX_H_DEFINED
#define _COMPLEX_H_DEFINED

#include <corecrt.h>

#ifndef _C_COMPLEX_T
#define _C_COMPLEX_T
typedef struct _C_double_complex
{
    double _Val[2];
} _C_double_complex;

typedef struct _C_float_complex
{
    float _Val[2];
} _C_float_complex;
#endif

typedef _C_double_complex _Dcomplex;
typedef _C_float_complex _Fcomplex;

_ACRTIMP _Dcomplex __cdecl _Cbuild(double, double);
_ACRTIMP _Dcomplex __cdecl cexp(_Dcomplex);

_ACRTIMP double __cdecl carg(_Dcomplex);
_ACRTIMP double __cdecl cimag(_Dcomplex);
_ACRTIMP double __cdecl creal(_Dcomplex);

#if defined(__i386__) && !defined(__MINGW32__) && !defined(_MSC_VER)
/* Note: this should return a _Fcomplex, but calling convention for returning
 * structures is different between Windows and gcc on i386. */
_ACRTIMP unsigned __int64 __cdecl _FCbuild(float, float);

static inline _Fcomplex __cdecl __wine__FCbuild(float r, float i)
{
    union {
        _Fcomplex c;
        unsigned __int64 ull;
    } u;
    u.ull = _FCbuild(r, i);
    return u.c;
}
#define _FCbuild(r, i) __wine__FCbuild(r, i)

#else
_ACRTIMP _Fcomplex __cdecl _FCbuild(float, float);
#endif

_ACRTIMP float __cdecl cimagf(_Fcomplex);
_ACRTIMP float __cdecl crealf(_Fcomplex);

#endif /* _COMPLEX_H_DEFINED */
