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
    double          (CDECL *exp)(double x);
    float           (CDECL *expf)(float x);
    double          (CDECL *exp2)(double x);
    float           (CDECL *exp2f)(float x);
    float           (CDECL *fmaf)(float x, float y, float z);
    double          (CDECL *pow)(double x, double y);
    float           (CDECL *powf)(float x, float y);
    double          (CDECL *tgamma)(double x);
    float           (CDECL *tgammaf)(float x);
};

#endif /* __UNIXLIB_H */
