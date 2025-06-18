#ifndef _COMPLEX_IMPL_H
#define _COMPLEX_IMPL_H

#include <complex.h>
#include "libm.h"

#undef CMPLX
#undef CMPLXF
#undef CMPLXL

#define CMPLX(x, y)  _Cbuild(x, y)
#define CMPLXF(x, y) _FCbuild(x, y)
#define CMPLXL(x, y) _LCbuild(x, y)

hidden _Dcomplex __ldexp_cexp(_Dcomplex,int);
hidden _Fcomplex __ldexp_cexpf(_Fcomplex,int);

#endif
