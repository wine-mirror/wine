/*
 * Helper functions for ntdll
 *
 * Copyright 2000 Juergen Schmied
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

#include <time.h>
#include <math.h>

#include "wine/debug.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);

#if defined(__GNUC__) && defined(__i386__)
#define DO_FPU(x,y) __asm__ __volatile__( x " %0;fwait" : "=m" (y) : )
#define POP_FPU(x) DO_FPU("fstpl",x)
#endif

void dump_ObjectAttributes (const OBJECT_ATTRIBUTES *oa)
{
	if (oa)
	  TRACE("%p:(name=%s, attr=0x%08lx, hRoot=%p, sd=%p)\n",
	    oa, debugstr_us(oa->ObjectName),
	    oa->Attributes, oa->RootDirectory, oa->SecurityDescriptor);
}

LPCSTR debugstr_us( const UNICODE_STRING *us )
{
    if (!us) return "<null>";
    return debugstr_wn(us->Buffer, us->Length / sizeof(WCHAR));
}

/*********************************************************************
 *                  _ftol   (NTDLL.@)
 *
 * VERSION
 *	[GNUC && i386]
 */
#if defined(__GNUC__) && defined(__i386__)
LONGLONG __cdecl NTDLL__ftol(void)
{
	/* don't just do DO_FPU("fistp",retval), because the rounding
	 * mode must also be set to "round towards zero"... */
	double fl;
	POP_FPU(fl);
	return (LONGLONG)fl;
}
#endif /* defined(__GNUC__) && defined(__i386__) */

/*********************************************************************
 *                  _ftol   (NTDLL.@)
 *
 * FIXME
 *	Should be register function
 * VERSION
 *	[!GNUC && i386]
 */
#if !defined(__GNUC__) && defined(__i386__)
LONGLONG __cdecl NTDLL__ftol(double fl)
{
	FIXME("should be register function\n");
	return (LONGLONG)fl;
}
#endif /* !defined(__GNUC__) && defined(__i386__) */

/*********************************************************************
 *                  _ftol   (NTDLL.@)
 * VERSION
 *	[!i386]
 */
#ifndef __i386__
LONG __cdecl NTDLL__ftol(double fl)
{
	return (LONG) fl;
}
#endif /* !defined(__i386__) */

/*********************************************************************
 *                  _CIpow   (NTDLL.@)
 * VERSION
 *	[GNUC && i386]
 */
#if defined(__GNUC__) && defined(__i386__)
double __cdecl NTDLL__CIpow(void)
{
	double x,y;
	POP_FPU(y);
	POP_FPU(x);
	return pow(x,y);
}
#endif /* defined(__GNUC__) && defined(__i386__) */


/*********************************************************************
 *                  _CIpow   (NTDLL.@)
 *
 * FIXME
 *	Should be register function
 *
 * VERSION
 *	[!GNUC && i386]
 */
#if !defined(__GNUC__) && defined(__i386__)
double __cdecl NTDLL__CIpow(double x,double y)
{
	FIXME("should be register function\n");
	return pow(x,y);
}
#endif /* !defined(__GNUC__) && defined(__i386__) */

/*********************************************************************
 *                  _CIpow   (NTDLL.@)
 * VERSION
 *	[!i386]
 */
#ifndef __i386__
double __cdecl NTDLL__CIpow(double x,double y)
{
	return pow(x,y);
}
#endif /* !defined(__i386__) */


/*********************************************************************
 *                  abs   (NTDLL.@)
 */
int NTDLL_abs( int i )
{
    return abs( i );
}

/*********************************************************************
 *                  labs   (NTDLL.@)
 */
long int NTDLL_labs( long int i )
{
    return labs( i );
}

/*********************************************************************
 *                  atan   (NTDLL.@)
 */
double NTDLL_atan( double d )
{
    return atan( d );
}

/*********************************************************************
 *                  ceil   (NTDLL.@)
 */
double NTDLL_ceil( double d )
{
    return ceil( d );
}

/*********************************************************************
 *                  cos   (NTDLL.@)
 */
double NTDLL_cos( double d )
{
    return cos( d );
}

/*********************************************************************
 *                  fabs   (NTDLL.@)
 */
double NTDLL_fabs( double d )
{
    return fabs( d );
}

/*********************************************************************
 *                  floor   (NTDLL.@)
 */
double NTDLL_floor( double d )
{
    return floor( d );
}

/*********************************************************************
 *                  log   (NTDLL.@)
 */
double NTDLL_log( double d )
{
    return log( d );
}

/*********************************************************************
 *                  pow   (NTDLL.@)
 */
double NTDLL_pow( double x, double y )
{
    return pow( x, y );
}

/*********************************************************************
 *                  sin   (NTDLL.@)
 */
double NTDLL_sin( double d )
{
    return sin( d );
}

/*********************************************************************
 *                  sqrt   (NTDLL.@)
 */
double NTDLL_sqrt( double d )
{
    return sqrt( d );
}

/*********************************************************************
 *                  tan   (NTDLL.@)
 */
double NTDLL_tan( double d )
{
    return tan( d );
}
