#include "libm.h"

#if FLT_EVAL_METHOD==0 || FLT_EVAL_METHOD==1
#define EPS DBL_EPSILON
#elif FLT_EVAL_METHOD==2
#define EPS LDBL_EPSILON
#endif

double __cdecl round(double x)
{
	union {double f; uint64_t i;} u = {x};
	uint64_t tmp;
	int e = (u.i >> 52 & 0x7ff) - 0x3ff;
	double_t y;

	if (e >= 52)
		return x;
	if (e < -1)
		return 0*u.f;
	if (e == -1)
		return (u.i >> 63) ? -1 : 1;

	tmp = 0x000fffffffffffffULL >> e;
	if (!(u.i & tmp))
		return x;
	u.i += 0x0008000000000000ULL >> e;
	u.i &= ~tmp;
	return u.f;
}
