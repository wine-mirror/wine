/* origin: FreeBSD /usr/src/lib/msun/src/e_asinf.c */
/*
 * Conversion to float by Ian Lance Taylor, Cygnus Support, ian@cygnus.com.
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

static const double
pio2 = 1.570796326794896558e+00;

static const float
pio4_hi = 0.785398125648,
pio2_lo = 7.54978941586e-08,
/* coefficients for R(x^2) */
pS0 =  1.66666672e-01,
pS1 = -5.11644611e-02,
pS2 = -1.21124933e-02,
pS3 = -3.58742251e-03,
qS1 = -7.56982703e-01;

static float R(float z)
{
	float_t p, q;
	p = z*(pS0+z*(pS1+z*(pS2+z*pS3)));
	q = 1.0f+z*qS1;
	return p/q;
}

float __cdecl asinf(float x)
{
	float s, z, f, c;
	uint32_t hx,ix;

	GET_FLOAT_WORD(hx, x);
	ix = hx & 0x7fffffff;
	if (ix >= 0x3f800000) {  /* |x| >= 1 */
		if (ix == 0x3f800000)  /* |x| == 1 */
			return x*pio2 + 0x1p-120f;  /* asin(+-1) = +-pi/2 with inexact */
		if (isnan(x)) return x;
		return math_error(_DOMAIN, "asinf", x, 0, 0 / (x - x));
	}
	if (ix < 0x3f000000) {  /* |x| < 0.5 */
		/* if 0x1p-126 <= |x| < 0x1p-12, avoid raising underflow */
		if (ix < 0x39800000 && ix >= 0x00800000)
			return x;
		return x + x*R(x*x);
	}
	/* 1 > |x| >= 0.5 */
	z = (1 - fabsf(x))*0.5f;
	s = sqrtf(z);
	/* f+c = sqrt(z) */
	*(unsigned int*)&f = *(unsigned int*)&s & 0xffff0000;
	c = (z - f * f) / (s + f);
	x = pio4_hi - (2*s*R(z) - (pio2_lo - 2*c) - (pio4_hi - 2*f));
	if (hx >> 31)
		return -x;
	return x;
}
