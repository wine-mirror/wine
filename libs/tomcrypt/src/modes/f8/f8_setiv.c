/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 */
#include "tomcrypt.h"

/**
   @file f8_setiv.c
   F8 implementation, set IV, Tom St Denis
*/

#ifdef LTC_F8_MODE

/**
   Set an initialization vector
   @param IV   The initialization vector
   @param len  The length of the vector (in octets)
   @param f8   The F8 state
   @return CRYPT_OK if successful
*/
int f8_setiv(const unsigned char *IV, unsigned long len, symmetric_F8 *f8)
{
   int err;

   LTC_ARGCHK(IV != NULL);
   LTC_ARGCHK(f8 != NULL);

   if ((err = cipher_is_valid(f8->cipher)) != CRYPT_OK) {
       return err;
   }

   if (len != (unsigned long)f8->blocklen) {
      return CRYPT_INVALID_ARG;
   }

   /* force next block */
   f8->padlen = 0;
   return cipher_descriptor[f8->cipher].ecb_encrypt(IV, f8->IV, &f8->key);
}

#endif
