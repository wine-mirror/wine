#include <math.h>
#include "libm.h"

float __cdecl remainderf(float x, float y)
{
	int q;
	return remquof(x, y, &q);
}

weak_alias(remainderf, dremf);
