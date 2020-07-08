/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the Wine project.
 */

#ifndef _INC_FENV
#define _INC_FENV

#include <float.h>

#define FE_TONEAREST   _RC_NEAR
#define FE_UPWARD      _RC_UP
#define FE_DOWNWARD    _RC_DOWN
#define FE_TOWARDZERO  _RC_CHOP

#ifdef __cplusplus
extern "C" {
#endif

_ACRTIMP int __cdecl fesetround(int);

#ifdef __cplusplus
}
#endif

#endif /* _INC_FENV */
