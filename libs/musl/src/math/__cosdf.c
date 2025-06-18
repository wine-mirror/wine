/* origin: FreeBSD /usr/src/lib/msun/src/k_cosf.c */
/*
 * Conversion to float by Ian Lance Taylor, Cygnus Support, ian@cygnus.com.
 * Debugged and optimized by Bruce D. Evans.
 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#include "libm.h"

/* |cos(x) - c(x)| < 2**-34.1 (~[-5.37e-11, 5.295e-11]). */
static const double
C0 = -0x1.0000000000000p-1,
C1 =  0x1.5555555555555p-5,
C2 = -0x1.6c16c16c16c17p-10,
C3 =  0x1.a01a01a01a01ap-16,
C4 = -0x1.27e4fb7789f5cp-22;

float __cosdf(double x)
{
	double_t z;

	z = x*x;
	if (x > -7.8163146972656250e-03 && x < 7.8163146972656250e-03)
		return 1 + C0 * z;
	return 1.0 + z * (C0 + z * (C1 + z * (C2 + z * (C3 + z * C4))));
}
