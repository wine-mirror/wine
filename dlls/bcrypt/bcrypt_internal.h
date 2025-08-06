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
    ALG_ID_ECDH_P521,

    /* signature */
    ALG_ID_RSA_SIGN,
    ALG_ID_ECDSA_P256,
    ALG_ID_ECDSA_P384,
    ALG_ID_ECDSA_P521,
    ALG_ID_DSA,

    /* rng */
    ALG_ID_RNG,

    /* key derivation */
    ALG_ID_PBKDF2,
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
    void        *padding;
    UCHAR       *output;
    ULONG        output_len;
    ULONG       *ret_len;
    ULONG        flags;
};

struct key_asymmetric_encrypt_params
{
    struct key  *key;
    UCHAR       *input;
    unsigned     input_len;
    void        *padding;
    UCHAR       *output;
    ULONG        output_len;
    ULONG       *ret_len;
    ULONG        flags;
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

static inline ULONG len_from_bitlen( ULONG bitlen )
{
    return (bitlen + 7) / 8;
}

#endif /* __BCRYPT_INTERNAL_H */
