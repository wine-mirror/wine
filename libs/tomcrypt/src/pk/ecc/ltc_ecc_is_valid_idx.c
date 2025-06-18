/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 */

/* Implements ECC over Z/pZ for curve y^2 = x^3 - 3x + b
 *
 * All curves taken from NIST recommendation paper of July 1999
 * Available at http://csrc.nist.gov/cryptval/dss.htm
 */
#include "tomcrypt.h"

/**
  @file ltc_ecc_is_valid_idx.c
  ECC Crypto, Tom St Denis
*/

#ifdef LTC_MECC

/** Returns whether an ECC idx is valid or not
  @param n   The idx number to check
  @return 1 if valid, 0 if not
*/
int ltc_ecc_is_valid_idx(int n)
{
   int x;

   for (x = 0; ltc_ecc_sets[x].size != 0; x++);
   /* -1 is a valid index --- indicating that the domain params were supplied by the user */
   if ((n >= -1) && (n < x)) {
      return 1;
   }
   return 0;
}

#endif
