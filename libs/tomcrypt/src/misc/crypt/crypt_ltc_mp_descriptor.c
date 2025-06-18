/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 */
#include "tomcrypt.h"

/* Initialize ltc_mp to nulls, to force allocation on all platforms, including macOS. */
ltc_math_descriptor ltc_mp = { 0 };
