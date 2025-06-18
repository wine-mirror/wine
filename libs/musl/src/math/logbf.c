#include <math.h>
#include "libm.h"

float __cdecl logbf(float x)
{
	if (!isfinite(x))
		return x * x;
	if (x == 0) {
		errno = ERANGE;
		return -1/(x*x);
	}
	return ilogbf(x);
}
