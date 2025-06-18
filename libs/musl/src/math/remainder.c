#include <math.h>
#include "libm.h"

double __cdecl remainder(double x, double y)
{
	int q;
	return remquo(x, y, &q);
}

weak_alias(remainder, drem);
