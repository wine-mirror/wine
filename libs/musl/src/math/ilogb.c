#include <limits.h>
#include "libm.h"

int __cdecl ilogb(double x)
{
	union {double f; uint64_t i;} u = {x};
	uint64_t i = u.i;
	int e = i>>52 & 0x7ff;

	if (!e) {
		i <<= 12;
		if (i == 0) {
			return FP_ILOGB0;
		}
		/* subnormal x */
		for (e = -0x3ff; i>>63 == 0; e--, i<<=1);
		return e;
	}
	if (e == 0x7ff) {
		return i<<12 ? FP_ILOGBNAN : INT_MAX;
	}
	return e - 0x3ff;
}
