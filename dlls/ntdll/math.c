/*
 * Math functions
 *
 * Copyright 2021 Alexandre Julliard
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

#include <math.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "ntdll_misc.h"

/*********************************************************************
 *                  abs   (NTDLL.@)
 */
int CDECL abs( int i )
{
    return i >= 0 ? i : -i;
}

/*********************************************************************
 *                  atan   (NTDLL.@)
 */
double CDECL atan( double d )
{
    return unix_funcs->atan( d );
}

/*********************************************************************
 *                  ceil   (NTDLL.@)
 */
double CDECL ceil( double d )
{
    return unix_funcs->ceil( d );
}

/*********************************************************************
 *                  cos   (NTDLL.@)
 */
double CDECL cos( double d )
{
    return unix_funcs->cos( d );
}

/*********************************************************************
 *                  fabs   (NTDLL.@)
 */
double CDECL fabs( double d )
{
    return unix_funcs->fabs( d );
}

/*********************************************************************
 *                  floor   (NTDLL.@)
 */
double CDECL floor( double d )
{
    return unix_funcs->floor( d );
}

/*********************************************************************
 *                  log   (NTDLL.@)
 */
double CDECL log( double d )
{
    return unix_funcs->log( d );
}

/*********************************************************************
 *                  pow   (NTDLL.@)
 */
double CDECL pow( double x, double y )
{
    return unix_funcs->pow( x, y );
}

/*********************************************************************
 *                  sin   (NTDLL.@)
 */
double CDECL sin( double d )
{
    return unix_funcs->sin( d );
}

/*********************************************************************
 *                  sqrt   (NTDLL.@)
 */
double CDECL sqrt( double d )
{
    return unix_funcs->sqrt( d );
}

/*********************************************************************
 *                  tan   (NTDLL.@)
 */
double CDECL tan( double d )
{
    return unix_funcs->tan( d );
}

#if (defined(__GNUC__) || defined(__clang__)) && defined(__i386__)

#define FPU_DOUBLE(var) double var; \
    __asm__ __volatile__( "fstpl %0;fwait" : "=m" (var) : )
#define FPU_DOUBLES(var1,var2) double var1,var2; \
    __asm__ __volatile__( "fstpl %0;fwait" : "=m" (var2) : ); \
    __asm__ __volatile__( "fstpl %0;fwait" : "=m" (var1) : )

/*********************************************************************
 *		_CIcos (NTDLL.@)
 */
double CDECL _CIcos(void)
{
    FPU_DOUBLE(x);
    return cos(x);
}

/*********************************************************************
 *		_CIlog (NTDLL.@)
 */
double CDECL _CIlog(void)
{
    FPU_DOUBLE(x);
    return log(x);
}

/*********************************************************************
 *		_CIpow (NTDLL.@)
 */
double CDECL _CIpow(void)
{
    FPU_DOUBLES(x,y);
    return pow(x,y);
}

/*********************************************************************
 *		_CIsin (NTDLL.@)
 */
double CDECL _CIsin(void)
{
    FPU_DOUBLE(x);
    return sin(x);
}

/*********************************************************************
 *		_CIsqrt (NTDLL.@)
 */
double CDECL _CIsqrt(void)
{
    FPU_DOUBLE(x);
    return sqrt(x);
}

/*********************************************************************
 *                  _ftol   (NTDLL.@)
 */
LONGLONG CDECL _ftol(void)
{
    FPU_DOUBLE(x);
    return (LONGLONG)x;
}

#endif /* (defined(__GNUC__) || defined(__clang__)) && defined(__i386__) */
