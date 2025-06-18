#include "libm.h"

#if FLT_EVAL_METHOD==0 || FLT_EVAL_METHOD==1
#define EPS DBL_EPSILON
#elif FLT_EVAL_METHOD==2
#define EPS LDBL_EPSILON
#endif

double __cdecl floor(double x)
{
	union {double f; uint64_t i;} u = {x};
	int e = (u.i >> 52 & 0x7ff) - 0x3ff;
	double_t y;

	if (e >= 52)
		return x;
	if (e >= 0) {
		uint64_t m = 0x000fffffffffffffULL >> e;
		if ((u.i & m) == 0)
			return x;
		if (u.i >> 63)
			u.i += m;
		u.i &= ~m;
	} else {
		if (u.i >> 63 == 0)
			return 0;
		if (u.i << 1)
			return -1;
	}
	return u.f;
}
