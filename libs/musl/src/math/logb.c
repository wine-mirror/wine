#include <math.h>
#include "libm.h"

/*
special cases:
	logb(+-0) = -inf, and raise divbyzero
	logb(+-inf) = +inf
	logb(nan) = nan
*/

double __cdecl logb(double x)
{
	if (!isfinite(x))
		return x * x;
	if (x == 0)
		return math_error(_SING, "_logb", x, 0, -1 / (x * x));
	return ilogb(x);
}
