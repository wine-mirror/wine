#include <math.h>
#include "libm.h"

float __cdecl lgammaf(float x)
{
	return __lgammaf_r(x, &__signgam);
}
