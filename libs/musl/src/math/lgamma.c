#include <math.h>
#include "libm.h"

double __cdecl lgamma(double x)
{
	return __lgamma_r(x, &__signgam);
}
