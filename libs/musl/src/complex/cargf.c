#include "complex_impl.h"

float cargf(_Fcomplex z)
{
	return atan2f(cimagf(z), crealf(z));
}
