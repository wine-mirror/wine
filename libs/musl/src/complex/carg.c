#include "complex_impl.h"

double carg(_Dcomplex z)
{
	return atan2(cimag(z), creal(z));
}
