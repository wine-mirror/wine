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

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wincrypt.h"
#include "bcrypt.h"
#include "wine/unixlib.h"

#define MAGIC_DSS1 ('D' | ('S' << 8) | ('S' << 16) | ('1' << 24))
#define MAGIC_DSS2 ('D' | ('S' << 8) | ('S' << 16) | ('2' << 24))

typedef struct
{
    ULONG64 len;
    DWORD h[8];
    UCHAR buf[64];
} SHA256_CTX;

void sha256_init(SHA256_CTX *ctx);
void sha256_update(SHA256_CTX *ctx, const UCHAR *buffer, ULONG len);
void sha256_finalize(SHA256_CTX *ctx, UCHAR *buffer);

typedef struct
{
  ULONG64 len;
  ULONG64 h[8];
  UCHAR buf[128];
} SHA512_CTX;

void sha512_init(SHA512_CTX *ctx);
void sha512_update(SHA512_CTX *ctx, const UCHAR *buffer, ULONG len);
void sha512_finalize(SHA512_CTX *ctx, UCHAR *buffer);

void sha384_init(SHA512_CTX *ctx);
#define sha384_update sha512_update
void sha384_finalize(SHA512_CTX *ctx, UCHAR *buffer);

typedef struct {
    unsigned char chksum[16], X[48], buf[16];
    unsigned long curlen;
} MD2_CTX;

void md2_init(MD2_CTX *ctx);
void md2_update(MD2_CTX *ctx, const unsigned char *buf, ULONG len);
void md2_finalize(MD2_CTX *ctx, unsigned char *hash);

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
#define MAGIC_SECRET (('S' << 24) | ('C' << 16) | ('R' << 8) | 'T')
struct object
{
    ULONG magic;
};

enum alg_id
{
    /* cipher */
    ALG_ID_3DES,
    ALG_ID_AES,
    ALG_ID_RC4,

    /* hash */
    ALG_ID_SHA256,
    ALG_ID_SHA384,
    ALG_ID_SHA512,
    ALG_ID_SHA1,
    ALG_ID_MD5,
    ALG_ID_MD4,
    ALG_ID_MD2,

    /* asymmetric encryption */
    ALG_ID_RSA,

    /* secret agreement */
    ALG_ID_DH,
    ALG_ID_ECDH_P256,
    ALG_ID_ECDH_P384,

    /* signature */
    ALG_ID_RSA_SIGN,
    ALG_ID_ECDSA_P256,
    ALG_ID_ECDSA_P384,
    ALG_ID_DSA,

    /* rng */
    ALG_ID_RNG,
};

enum chain_mode
{
    CHAIN_MODE_CBC,
    CHAIN_MODE_ECB,
    CHAIN_MODE_CFB,
    CHAIN_MODE_CCM,
    CHAIN_MODE_GCM,
};

struct algorithm
{
    struct object   hdr;
    enum alg_id     id;
    enum chain_mode mode;
    unsigned        flags;
};

struct key_symmetric
{
    enum chain_mode  mode;
    ULONG            block_size;
    UCHAR           *vector;
    ULONG            vector_len;
    UCHAR           *secret;
    unsigned         secret_len;
    CRITICAL_SECTION cs;
};

#define KEY_FLAG_LEGACY_DSA_V2  0x00000001
#define KEY_FLAG_FINALIZED      0x00000002

struct key_asymmetric
{
    ULONG             bitlen;     /* ignored for ECC keys */
    unsigned          flags;
    DSSSEED           dss_seed;
};

#define PRIVATE_DATA_SIZE 3
struct key
{
    struct object hdr;
    enum alg_id   alg_id;
    UINT64        private[PRIVATE_DATA_SIZE];  /* private data for backend */
    union
    {
        struct key_symmetric s;
        struct key_asymmetric a;
    } u;
};

struct secret
{
    struct object hdr;
    struct key *privkey;
    struct key *pubkey;
};

struct key_symmetric_set_auth_data_params
{
    struct key  *key;
    UCHAR       *auth_data;
    ULONG        len;
};

struct key_symmetric_encrypt_params
{
    struct key  *key;
    const UCHAR *input;
    unsigned     input_len;
    UCHAR       *output;
    ULONG        output_len;
};

struct key_symmetric_decrypt_params
{
    struct key  *key;
    const UCHAR *input;
    unsigned     input_len;
    UCHAR       *output;
    ULONG        output_len;
};

struct key_symmetric_get_tag_params
{
    struct key  *key;
    UCHAR       *tag;
    ULONG        len;
};

struct key_asymmetric_decrypt_params
{
    struct key  *key;
    UCHAR       *input;
    unsigned     input_len;
    UCHAR       *output;
    ULONG        output_len;
    ULONG       *ret_len;
};

struct key_asymmetric_encrypt_params
{
    struct key  *key;
    UCHAR       *input;
    unsigned    input_len;
    UCHAR       *output;
    ULONG       output_len;
    ULONG       *ret_len;
};

struct key_asymmetric_duplicate_params
{
    struct key *key_orig;
    struct key *key_copy;
};

struct key_asymmetric_sign_params
{
    struct key  *key;
    void        *padding;
    UCHAR       *input;
    unsigned     input_len;
    UCHAR       *output;
    ULONG        output_len;
    ULONG       *ret_len;
    unsigned     flags;
};

struct key_asymmetric_verify_params
{
    struct key *key;
    void       *padding;
    UCHAR      *hash;
    unsigned    hash_len;
    UCHAR      *signature;
    ULONG       signature_len;
    unsigned    flags;
};

#define KEY_EXPORT_FLAG_PUBLIC        0x00000001
#define KEY_EXPORT_FLAG_RSA_FULL      0x00000002
#define KEY_EXPORT_FLAG_DH_PARAMETERS 0x00000004

struct key_asymmetric_export_params
{
    struct key  *key;
    ULONG        flags;
    UCHAR       *buf;
    ULONG        len;
    ULONG       *ret_len;
};

#define KEY_IMPORT_FLAG_PUBLIC        0x00000001
#define KEY_IMPORT_FLAG_DH_PARAMETERS 0x00000002

struct key_asymmetric_import_params
{
    struct key  *key;
    ULONG        flags;
    UCHAR       *buf;
    ULONG        len;
};

struct key_asymmetric_derive_key_params
{
    struct key *privkey;
    struct key *pubkey;
    UCHAR      *output;
    ULONG       output_len;
    ULONG      *ret_len;
};

enum key_funcs
{
    unix_process_attach,
    unix_process_detach,
    unix_key_symmetric_vector_reset,
    unix_key_symmetric_set_auth_data,
    unix_key_symmetric_encrypt,
    unix_key_symmetric_decrypt,
    unix_key_symmetric_get_tag,
    unix_key_symmetric_destroy,
    unix_key_asymmetric_generate,
    unix_key_asymmetric_decrypt,
    unix_key_asymmetric_encrypt,
    unix_key_asymmetric_duplicate,
    unix_key_asymmetric_sign,
    unix_key_asymmetric_verify,
    unix_key_asymmetric_destroy,
    unix_key_asymmetric_export,
    unix_key_asymmetric_import,
    unix_key_asymmetric_derive_key,
    unix_funcs_count,
};

#endif /* __BCRYPT_INTERNAL_H */
