/*
 * Copyright 2016 Michael Müller
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#ifndef __BCRYPT_INTERNAL_H
#define __BCRYPT_INTERNAL_H

#include <stdarg.h>
#ifdef HAVE_GNUTLS_CIPHER_INIT
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#include <gnutls/abstract.h>
#elif HAVE_COMMONCRYPTO_COMMONCRYPTOR_H
#include <AvailabilityMacros.h>
#include <CommonCrypto/CommonCryptor.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "bcrypt.h"

typedef struct
{
    ULONG64 len;
    DWORD h[8];
    UCHAR buf[64];
} SHA256_CTX;

void sha256_init(SHA256_CTX *ctx) DECLSPEC_HIDDEN;
void sha256_update(SHA256_CTX *ctx, const UCHAR *buffer, ULONG len) DECLSPEC_HIDDEN;
void sha256_finalize(SHA256_CTX *ctx, UCHAR *buffer) DECLSPEC_HIDDEN;

typedef struct
{
  ULONG64 len;
  ULONG64 h[8];
  UCHAR buf[128];
} SHA512_CTX;

void sha512_init(SHA512_CTX *ctx) DECLSPEC_HIDDEN;
void sha512_update(SHA512_CTX *ctx, const UCHAR *buffer, ULONG len) DECLSPEC_HIDDEN;
void sha512_finalize(SHA512_CTX *ctx, UCHAR *buffer) DECLSPEC_HIDDEN;

void sha384_init(SHA512_CTX *ctx) DECLSPEC_HIDDEN;
#define sha384_update sha512_update
void sha384_finalize(SHA512_CTX *ctx, UCHAR *buffer) DECLSPEC_HIDDEN;

typedef struct {
    unsigned char chksum[16], X[48], buf[16];
    unsigned long curlen;
} MD2_CTX;

void md2_init(MD2_CTX *ctx) DECLSPEC_HIDDEN;
void md2_update(MD2_CTX *ctx, const unsigned char *buf, ULONG len) DECLSPEC_HIDDEN;
void md2_finalize(MD2_CTX *ctx, unsigned char *hash) DECLSPEC_HIDDEN;

/* Definitions from advapi32 */
typedef struct tagMD4_CTX {
    unsigned int buf[4];
    unsigned int i[2];
    unsigned char in[64];
    unsigned char digest[16];
} MD4_CTX;

VOID WINAPI MD4Init(MD4_CTX *ctx);
VOID WINAPI MD4Update(MD4_CTX *ctx, const unsigned char *buf, unsigned int len);
VOID WINAPI MD4Final(MD4_CTX *ctx);

typedef struct
{
    unsigned int i[2];
    unsigned int buf[4];
    unsigned char in[64];
    unsigned char digest[16];
} MD5_CTX;

VOID WINAPI MD5Init(MD5_CTX *ctx);
VOID WINAPI MD5Update(MD5_CTX *ctx, const unsigned char *buf, unsigned int len);
VOID WINAPI MD5Final(MD5_CTX *ctx);

typedef struct
{
   ULONG Unknown[6];
   ULONG State[5];
   ULONG Count[2];
   UCHAR Buffer[64];
} SHA_CTX;

VOID WINAPI A_SHAInit(SHA_CTX *ctx);
VOID WINAPI A_SHAUpdate(SHA_CTX *ctx, const UCHAR *buffer, UINT size);
VOID WINAPI A_SHAFinal(SHA_CTX *ctx, PULONG result);

#define MAGIC_ALG  (('A' << 24) | ('L' << 16) | ('G' << 8) | '0')
#define MAGIC_HASH (('H' << 24) | ('A' << 16) | ('S' << 8) | 'H')
#define MAGIC_KEY  (('K' << 24) | ('E' << 16) | ('Y' << 8) | '0')
struct object
{
    ULONG magic;
};

enum alg_id
{
    ALG_ID_AES,
    ALG_ID_MD2,
    ALG_ID_MD4,
    ALG_ID_MD5,
    ALG_ID_RNG,
    ALG_ID_RSA,
    ALG_ID_SHA1,
    ALG_ID_SHA256,
    ALG_ID_SHA384,
    ALG_ID_SHA512,
    ALG_ID_ECDSA_P256,
    ALG_ID_ECDSA_P384,
};

enum mode_id
{
    MODE_ID_ECB,
    MODE_ID_CBC,
    MODE_ID_GCM
};

struct algorithm
{
    struct object hdr;
    enum alg_id   id;
    enum mode_id  mode;
    BOOL          hmac;
};

#if defined(HAVE_GNUTLS_CIPHER_INIT)
struct key_symmetric
{
    enum mode_id        mode;
    ULONG               block_size;
    gnutls_cipher_hd_t  handle;
    UCHAR              *secret;
    ULONG               secret_len;
};

struct key_asymmetric
{
    UCHAR *pubkey;
    ULONG  pubkey_len;
};

struct key
{
    struct object hdr;
    enum alg_id   alg_id;
    union
    {
        struct key_symmetric  s;
        struct key_asymmetric a;
    } u;
};
#elif defined(HAVE_COMMONCRYPTO_COMMONCRYPTOR_H) && MAC_OS_X_VERSION_MAX_ALLOWED >= 1080
struct key_symmetric
{
    enum mode_id   mode;
    ULONG          block_size;
    CCCryptorRef   ref_encrypt;
    CCCryptorRef   ref_decrypt;
    UCHAR         *secret;
    ULONG          secret_len;
};

struct key_asymmetric
{
    UCHAR *pubkey;
    ULONG  pubkey_len;
};

struct key
{
    struct object hdr;
    enum alg_id   alg_id;
    union
    {
        struct key_symmetric  s;
        struct key_asymmetric a;
    } u;
};
#else
struct key
{
    struct object hdr;
};
#endif

NTSTATUS get_alg_property( const struct algorithm *, const WCHAR *, UCHAR *, ULONG, ULONG * ) DECLSPEC_HIDDEN;

NTSTATUS key_set_property( struct key *, const WCHAR *, UCHAR *, ULONG, ULONG ) DECLSPEC_HIDDEN;
NTSTATUS key_symmetric_init( struct key *, struct algorithm *, const UCHAR *, ULONG ) DECLSPEC_HIDDEN;
NTSTATUS key_symmetric_set_params( struct key *, UCHAR *, ULONG ) DECLSPEC_HIDDEN;
NTSTATUS key_symmetric_set_auth_data( struct key *, UCHAR *, ULONG ) DECLSPEC_HIDDEN;
NTSTATUS key_symmetric_encrypt( struct key *, const UCHAR *, ULONG, UCHAR *, ULONG ) DECLSPEC_HIDDEN;
NTSTATUS key_symmetric_decrypt( struct key *, const UCHAR *, ULONG, UCHAR *, ULONG ) DECLSPEC_HIDDEN;
NTSTATUS key_symmetric_get_tag( struct key *, UCHAR *, ULONG ) DECLSPEC_HIDDEN;
NTSTATUS key_asymmetric_init( struct key *, struct algorithm *, const UCHAR *, ULONG ) DECLSPEC_HIDDEN;
NTSTATUS key_asymmetric_verify( struct key *, void *, UCHAR *, ULONG, UCHAR *, ULONG, DWORD ) DECLSPEC_HIDDEN;
NTSTATUS key_destroy( struct key * ) DECLSPEC_HIDDEN;
BOOL key_is_symmetric( struct key * ) DECLSPEC_HIDDEN;

BOOL gnutls_initialize(void) DECLSPEC_HIDDEN;
void gnutls_uninitialize(void) DECLSPEC_HIDDEN;

#endif /* __BCRYPT_INTERNAL_H */
