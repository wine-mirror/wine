#include "libm.h"

float __cdecl sinhf(float x)
{
	union {float f; uint32_t i;} u = {.f = x};
	uint32_t sign = u.i & 0x80000000;
	uint32_t w;
	float t, h, absx;

	h = 0.5;
	if (u.i >> 31)
		h = -h;
	/* |x| */
	u.i &= 0x7fffffff;
	absx = u.f;
	w = u.i;

	/* |x| < log(FLT_MAX) */
	if (w < 0x42b17217) {
		t = expm1f(absx);
		if (w < 0x3f800000) {
			if (w < 0x3f800000 - (12<<23))
				return x;
			return h*(2*t - t*t/(t+1));
		}
		return h*(t + t/(t+1));
	}

	/* |x| > logf(FLT_MAX) or nan */
	if (w > 0x7f800000) {
            u.i = w | sign | 0x400000;
            return u.f;
	}
	t = __expo2f(absx, 2*h);
	return t;
}
