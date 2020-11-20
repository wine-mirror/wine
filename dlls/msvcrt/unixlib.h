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

#ifndef __UNIXLIB_H
#define __UNIXLIB_H

struct unix_funcs
{
    double          (CDECL *acosh)(double x);
    float           (CDECL *acoshf)(float x);
    double          (CDECL *asinh)(double x);
    float           (CDECL *asinhf)(float x);
    double          (CDECL *atanh)(double x);
    float           (CDECL *atanhf)(float x);
    double          (CDECL *cbrt)(double x);
    float           (CDECL *cbrtf)(float x);
    double          (CDECL *ceil)(double x);
    float           (CDECL *ceilf)(float x);
    double          (CDECL *cos)(double x);
    float           (CDECL *cosf)(float x);
    double          (CDECL *cosh)(double x);
    float           (CDECL *coshf)(float x);
    double          (CDECL *erf)(double x);
    double          (CDECL *erfc)(double x);
    float           (CDECL *erfcf)(float x);
    float           (CDECL *erff)(float x);
    double          (CDECL *exp)(double x);
    float           (CDECL *expf)(float x);
    double          (CDECL *exp2)(double x);
    float           (CDECL *exp2f)(float x);
    double          (CDECL *expm1)(double x);
    float           (CDECL *expm1f)(float x);
    double          (CDECL *floor)(double x);
    float           (CDECL *floorf)(float x);
    double          (CDECL *fma)(double x, double y, double z);
    float           (CDECL *fmaf)(float x, float y, float z);
    double          (CDECL *fmod)(double x, double y);
    float           (CDECL *fmodf)(float x, float y);
    double          (CDECL *frexp)(double x, int *exp);
    float           (CDECL *frexpf)(float x, int *exp);
    double          (CDECL *hypot)(double x, double y);
    float           (CDECL *hypotf)(float x, float y);
    double          (CDECL *j0)(double num);
    double          (CDECL *j1)(double num);
    double          (CDECL *jn)(int n, double num);
    double          (CDECL *ldexp)(double x, int exp);
    double          (CDECL *lgamma)(double x);
    float           (CDECL *lgammaf)(float x);
    __int64         (CDECL *llrint)(double x);
    __int64         (CDECL *llrintf)(float x);
    __int64         (CDECL *llround)(double x);
    __int64         (CDECL *llroundf)(float x);
    double          (CDECL *log)(double x);
    float           (CDECL *logf)(float x);
    double          (CDECL *log10)(double x);
    float           (CDECL *log10f)(float x);
    double          (CDECL *log1p)(double x);
    float           (CDECL *log1pf)(float x);
    double          (CDECL *log2)(double x);
    float           (CDECL *log2f)(float x);
    double          (CDECL *logb)(double x);
    float           (CDECL *logbf)(float x);
    int             (CDECL *lrint)(double x);
    int             (CDECL *lrintf)(float x);
    int             (CDECL *lround)(double x);
    int             (CDECL *lroundf)(float x);
    double          (CDECL *modf)(double x, double *iptr);
    float           (CDECL *modff)(float x, float *iptr);
    double          (CDECL *nearbyint)(double num);
    float           (CDECL *nearbyintf)(float num);
    double          (CDECL *nextafter)(double x, double y);
    float           (CDECL *nextafterf)(float x, float y);
    double          (CDECL *nexttoward)(double x, double y);
    float           (CDECL *nexttowardf)(float x, double y);
    double          (CDECL *pow)(double x, double y);
    float           (CDECL *powf)(float x, float y);
    double          (CDECL *remainder)(double x, double y);
    float           (CDECL *remainderf)(float x, float y);
    double          (CDECL *remquo)(double x, double y, int *quo);
    float           (CDECL *remquof)(float x, float y, int *quo);
    double          (CDECL *rint)(double x);
    float           (CDECL *rintf)(float x);
    double          (CDECL *round)(double x);
    float           (CDECL *roundf)(float x);
    double          (CDECL *sin)(double x);
    float           (CDECL *sinf)(float x);
    double          (CDECL *sinh)(double x);
    float           (CDECL *sinhf)(float x);
    double          (CDECL *tan)(double x);
    float           (CDECL *tanf)(float x);
    double          (CDECL *tanh)(double x);
    float           (CDECL *tanhf)(float x);
    double          (CDECL *tgamma)(double x);
    float           (CDECL *tgammaf)(float x);
    double          (CDECL *trunc)(double x);
    float           (CDECL *truncf)(float x);
    double          (CDECL *y0)(double num);
    double          (CDECL *y1)(double num);
    double          (CDECL *yn)(int order, double num);
};

#endif /* __UNIXLIB_H */
