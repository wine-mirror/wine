/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
 * Copyright 2018 Hans Leidekker for CodeWeavers
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#ifdef HAVE_GNUTLS_CIPHER_INIT
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#include <gnutls/abstract.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "ntsecapi.h"
#include "bcrypt.h"

#include "bcrypt_internal.h"

#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/library.h"
#include "wine/unicode.h"

#ifdef HAVE_GNUTLS_CIPHER_INIT
WINE_DEFAULT_DEBUG_CHANNEL(bcrypt);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

#if GNUTLS_VERSION_MAJOR < 3
#define GNUTLS_CIPHER_AES_192_CBC 92
#define GNUTLS_CIPHER_AES_128_GCM 93
#define GNUTLS_CIPHER_AES_256_GCM 94
#define GNUTLS_PK_ECC 4

typedef enum
{
    GNUTLS_ECC_CURVE_INVALID,
    GNUTLS_ECC_CURVE_SECP224R1,
    GNUTLS_ECC_CURVE_SECP256R1,
    GNUTLS_ECC_CURVE_SECP384R1,
    GNUTLS_ECC_CURVE_SECP521R1,
} gnutls_ecc_curve_t;
#endif

/* Not present in gnutls version < 3.0 */
static int (*pgnutls_cipher_tag)(gnutls_cipher_hd_t, void *, size_t);
static int (*pgnutls_cipher_add_auth)(gnutls_cipher_hd_t, const void *, size_t);
static int (*pgnutls_pubkey_import_ecc_raw)(gnutls_pubkey_t, gnutls_ecc_curve_t,
                                            const gnutls_datum_t *, const gnutls_datum_t *);
static gnutls_sign_algorithm_t (*pgnutls_pk_to_sign)(gnutls_pk_algorithm_t, gnutls_digest_algorithm_t);
static int (*pgnutls_pubkey_verify_hash2)(gnutls_pubkey_t, gnutls_sign_algorithm_t, unsigned int,
                                          const gnutls_datum_t *, const gnutls_datum_t *);

/* Not present in gnutls version < 2.11.0 */
static int (*pgnutls_pubkey_import_rsa_raw)(gnutls_pubkey_t key, const gnutls_datum_t *m, const gnutls_datum_t *e);

static void *libgnutls_handle;
#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(gnutls_cipher_decrypt2);
MAKE_FUNCPTR(gnutls_cipher_deinit);
MAKE_FUNCPTR(gnutls_cipher_encrypt2);
MAKE_FUNCPTR(gnutls_cipher_init);
MAKE_FUNCPTR(gnutls_global_deinit);
MAKE_FUNCPTR(gnutls_global_init);
MAKE_FUNCPTR(gnutls_global_set_log_function);
MAKE_FUNCPTR(gnutls_global_set_log_level);
MAKE_FUNCPTR(gnutls_perror);
MAKE_FUNCPTR(gnutls_pubkey_init);
MAKE_FUNCPTR(gnutls_pubkey_deinit);
#undef MAKE_FUNCPTR

static int compat_gnutls_cipher_tag(gnutls_cipher_hd_t handle, void *tag, size_t tag_size)
{
    return GNUTLS_E_UNKNOWN_CIPHER_TYPE;
}

static int compat_gnutls_cipher_add_auth(gnutls_cipher_hd_t handle, const void *ptext, size_t ptext_size)
{
    return GNUTLS_E_UNKNOWN_CIPHER_TYPE;
}

static int compat_gnutls_pubkey_import_ecc_raw(gnutls_pubkey_t key, gnutls_ecc_curve_t curve,
                                               const gnutls_datum_t *x, const gnutls_datum_t *y)
{
    return GNUTLS_E_UNKNOWN_CIPHER_TYPE;
}

static gnutls_sign_algorithm_t compat_gnutls_pk_to_sign(gnutls_pk_algorithm_t pk, gnutls_digest_algorithm_t hash)
{
    return GNUTLS_SIGN_UNKNOWN;
}

static int compat_gnutls_pubkey_verify_hash2(gnutls_pubkey_t key, gnutls_sign_algorithm_t algo,
                                             unsigned int flags, const gnutls_datum_t *hash,
                                             const gnutls_datum_t *signature)
{
    return GNUTLS_E_UNKNOWN_CIPHER_TYPE;
}

static int compat_gnutls_pubkey_import_rsa_raw(gnutls_pubkey_t key, const gnutls_datum_t *m, const gnutls_datum_t *e)
{
    return GNUTLS_E_UNKNOWN_CIPHER_TYPE;
}

static void gnutls_log( int level, const char *msg )
{
    TRACE( "<%d> %s", level, msg );
}

BOOL gnutls_initialize(void)
{
    int ret;

    if (!(libgnutls_handle = wine_dlopen( SONAME_LIBGNUTLS, RTLD_NOW, NULL, 0 )))
    {
        ERR_(winediag)( "failed to load libgnutls, no support for encryption\n" );
        return FALSE;
    }

#define LOAD_FUNCPTR(f) \
    if (!(p##f = wine_dlsym( libgnutls_handle, #f, NULL, 0 ))) \
    { \
        ERR( "failed to load %s\n", #f ); \
        goto fail; \
    }

    LOAD_FUNCPTR(gnutls_cipher_decrypt2)
    LOAD_FUNCPTR(gnutls_cipher_deinit)
    LOAD_FUNCPTR(gnutls_cipher_encrypt2)
    LOAD_FUNCPTR(gnutls_cipher_init)
    LOAD_FUNCPTR(gnutls_global_deinit)
    LOAD_FUNCPTR(gnutls_global_init)
    LOAD_FUNCPTR(gnutls_global_set_log_function)
    LOAD_FUNCPTR(gnutls_global_set_log_level)
    LOAD_FUNCPTR(gnutls_perror)
    LOAD_FUNCPTR(gnutls_pubkey_init);
    LOAD_FUNCPTR(gnutls_pubkey_deinit);
#undef LOAD_FUNCPTR

    if (!(pgnutls_cipher_tag = wine_dlsym( libgnutls_handle, "gnutls_cipher_tag", NULL, 0 )))
    {
        WARN("gnutls_cipher_tag not found\n");
        pgnutls_cipher_tag = compat_gnutls_cipher_tag;
    }
    if (!(pgnutls_cipher_add_auth = wine_dlsym( libgnutls_handle, "gnutls_cipher_add_auth", NULL, 0 )))
    {
        WARN("gnutls_cipher_add_auth not found\n");
        pgnutls_cipher_add_auth = compat_gnutls_cipher_add_auth;
    }

    if ((ret = pgnutls_global_init()) != GNUTLS_E_SUCCESS)
    {
        pgnutls_perror( ret );
        goto fail;
    }
    if (!(pgnutls_pubkey_import_ecc_raw = wine_dlsym( libgnutls_handle, "gnutls_pubkey_import_ecc_raw", NULL, 0 )))
    {
        WARN("gnutls_pubkey_import_ecc_raw not found\n");
        pgnutls_pubkey_import_ecc_raw = compat_gnutls_pubkey_import_ecc_raw;
    }
    if (!(pgnutls_pk_to_sign = wine_dlsym( libgnutls_handle, "gnutls_pk_to_sign", NULL, 0 )))
    {
        WARN("gnutls_pk_to_sign not found\n");
        pgnutls_pk_to_sign = compat_gnutls_pk_to_sign;
    }
    if (!(pgnutls_pubkey_verify_hash2 = wine_dlsym( libgnutls_handle, "gnutls_pubkey_verify_hash2", NULL, 0 )))
    {
        WARN("gnutls_pubkey_verify_hash2 not found\n");
        pgnutls_pubkey_verify_hash2 = compat_gnutls_pubkey_verify_hash2;
    }
    if (!(pgnutls_pubkey_import_rsa_raw = wine_dlsym( libgnutls_handle, "gnutls_pubkey_import_rsa_raw", NULL, 0 )))
    {
        WARN("gnutls_pubkey_import_rsa_raw not found\n");
        pgnutls_pubkey_import_rsa_raw = compat_gnutls_pubkey_import_rsa_raw;
    }

    if (TRACE_ON( bcrypt ))
    {
        pgnutls_global_set_log_level( 4 );
        pgnutls_global_set_log_function( gnutls_log );
    }

    return TRUE;

fail:
    wine_dlclose( libgnutls_handle, NULL, 0 );
    libgnutls_handle = NULL;
    return FALSE;
}

void gnutls_uninitialize(void)
{
    pgnutls_global_deinit();
    wine_dlclose( libgnutls_handle, NULL, 0 );
    libgnutls_handle = NULL;
}

struct buffer
{
    BYTE  *buffer;
    DWORD  length;
    DWORD  pos;
    BOOL   error;
};

static void buffer_init( struct buffer *buffer )
{
    buffer->buffer = NULL;
    buffer->length = 0;
    buffer->pos    = 0;
    buffer->error  = FALSE;
}

static void buffer_free( struct buffer *buffer )
{
    heap_free( buffer->buffer );
}

static void buffer_append( struct buffer *buffer, BYTE *data, DWORD len )
{
    if (!len) return;

    if (buffer->pos + len > buffer->length)
    {
        DWORD new_length = max( max( buffer->pos + len, buffer->length * 2 ), 64 );
        BYTE *new_buffer;

        if (!(new_buffer = heap_realloc( buffer->buffer, new_length )))
        {
            ERR( "out of memory\n" );
            buffer->error = TRUE;
            return;
        }

        buffer->buffer = new_buffer;
        buffer->length = new_length;
    }

    memcpy( &buffer->buffer[buffer->pos], data, len );
    buffer->pos += len;
}

static void buffer_append_byte( struct buffer *buffer, BYTE value )
{
    buffer_append( buffer, &value, sizeof(value) );
}

static void buffer_append_asn1_length( struct buffer *buffer, DWORD length )
{
    DWORD num_bytes;

    if (length < 128)
    {
        buffer_append_byte( buffer, length );
        return;
    }

    if (length <= 0xff) num_bytes = 1;
    else if (length <= 0xffff) num_bytes = 2;
    else if (length <= 0xffffff) num_bytes = 3;
    else num_bytes = 4;

    buffer_append_byte( buffer, 0x80 | num_bytes );
    while (num_bytes--) buffer_append_byte( buffer, length >> (num_bytes * 8) );
}

static void buffer_append_asn1_integer( struct buffer *buffer, BYTE *data, DWORD len )
{
    DWORD leading_zero = (*data & 0x80) != 0;

    buffer_append_byte( buffer, 0x02 );  /* tag */
    buffer_append_asn1_length( buffer, len + leading_zero );
    if (leading_zero) buffer_append_byte( buffer, 0 );
    buffer_append( buffer, data, len );
}

static void buffer_append_asn1_sequence( struct buffer *buffer, struct buffer *content )
{
    if (content->error)
    {
        buffer->error = TRUE;
        return;
    }

    buffer_append_byte( buffer, 0x30 );  /* tag */
    buffer_append_asn1_length( buffer, content->pos );
    buffer_append( buffer, content->buffer, content->pos );
}

static void buffer_append_asn1_r_s( struct buffer *buffer, BYTE *r, DWORD r_len, BYTE *s, DWORD s_len )
{
    struct buffer value;

    buffer_init( &value );
    buffer_append_asn1_integer( &value, r, r_len );
    buffer_append_asn1_integer( &value, s, s_len );
    buffer_append_asn1_sequence( buffer, &value );
    buffer_free( &value );
}

NTSTATUS key_set_property( struct key *key, const WCHAR *prop, UCHAR *value, ULONG size, ULONG flags )
{
    if (!strcmpW( prop, BCRYPT_CHAINING_MODE ))
    {
        if (!strncmpW( (WCHAR *)value, BCRYPT_CHAIN_MODE_ECB, size ))
        {
            key->u.s.mode = MODE_ID_ECB;
            return STATUS_SUCCESS;
        }
        else if (!strncmpW( (WCHAR *)value, BCRYPT_CHAIN_MODE_CBC, size ))
        {
            key->u.s.mode = MODE_ID_CBC;
            return STATUS_SUCCESS;
        }
        else if (!strncmpW( (WCHAR *)value, BCRYPT_CHAIN_MODE_GCM, size ))
        {
            key->u.s.mode = MODE_ID_GCM;
            return STATUS_SUCCESS;
        }
        else
        {
            FIXME( "unsupported mode %s\n", debugstr_wn( (WCHAR *)value, size ) );
            return STATUS_NOT_IMPLEMENTED;
        }
    }

    FIXME( "unsupported key property %s\n", debugstr_w(prop) );
    return STATUS_NOT_IMPLEMENTED;
}

static ULONG get_block_size( struct algorithm *alg )
{
    ULONG ret = 0, size = sizeof(ret);
    get_alg_property( alg, BCRYPT_BLOCK_LENGTH, (UCHAR *)&ret, sizeof(ret), &size );
    return ret;
}

NTSTATUS key_symmetric_init( struct key *key, struct algorithm *alg, const UCHAR *secret, ULONG secret_len )
{
    if (!libgnutls_handle) return STATUS_INTERNAL_ERROR;

    switch (alg->id)
    {
    case ALG_ID_AES:
        break;

    default:
        FIXME( "algorithm %u not supported\n", alg->id );
        return STATUS_NOT_SUPPORTED;
    }

    if (!(key->u.s.block_size = get_block_size( alg ))) return STATUS_INVALID_PARAMETER;
    if (!(key->u.s.secret = heap_alloc( secret_len ))) return STATUS_NO_MEMORY;
    memcpy( key->u.s.secret, secret, secret_len );
    key->u.s.secret_len = secret_len;

    key->alg_id         = alg->id;
    key->u.s.mode       = alg->mode;
    key->u.s.handle     = 0;        /* initialized on first use */

    return STATUS_SUCCESS;
}

static gnutls_cipher_algorithm_t get_gnutls_cipher( const struct key *key )
{
    switch (key->alg_id)
    {
    case ALG_ID_AES:
        WARN( "handle block size\n" );
        switch (key->u.s.mode)
        {
        case MODE_ID_GCM:
            if (key->u.s.secret_len == 16) return GNUTLS_CIPHER_AES_128_GCM;
            if (key->u.s.secret_len == 32) return GNUTLS_CIPHER_AES_256_GCM;
            break;
        case MODE_ID_ECB: /* can be emulated with CBC + empty IV */
        case MODE_ID_CBC:
            if (key->u.s.secret_len == 16) return GNUTLS_CIPHER_AES_128_CBC;
            if (key->u.s.secret_len == 24) return GNUTLS_CIPHER_AES_192_CBC;
            if (key->u.s.secret_len == 32) return GNUTLS_CIPHER_AES_256_CBC;
            break;
        default:
            break;
        }
        FIXME( "AES mode %u with key length %u not supported\n", key->u.s.mode, key->u.s.secret_len );
        return GNUTLS_CIPHER_UNKNOWN;

    default:
        FIXME( "algorithm %u not supported\n", key->alg_id );
        return GNUTLS_CIPHER_UNKNOWN;
    }
}

NTSTATUS key_symmetric_set_params( struct key *key, UCHAR *iv, ULONG iv_len )
{
    gnutls_cipher_algorithm_t cipher;
    gnutls_datum_t secret, vector;
    int ret;

    if (key->u.s.handle)
    {
        pgnutls_cipher_deinit( key->u.s.handle );
        key->u.s.handle = NULL;
    }

    if ((cipher = get_gnutls_cipher( key )) == GNUTLS_CIPHER_UNKNOWN)
        return STATUS_NOT_SUPPORTED;

    secret.data = key->u.s.secret;
    secret.size = key->u.s.secret_len;
    if (iv)
    {
        vector.data = iv;
        vector.size = iv_len;
    }

    if ((ret = pgnutls_cipher_init( &key->u.s.handle, cipher, &secret, iv ? &vector : NULL )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

NTSTATUS key_symmetric_set_auth_data( struct key *key, UCHAR *auth_data, ULONG len )
{
    int ret;
    if (!auth_data) return STATUS_SUCCESS;
    if ((ret = pgnutls_cipher_add_auth( key->u.s.handle, auth_data, len )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }
    return STATUS_SUCCESS;
}

NTSTATUS key_symmetric_encrypt( struct key *key, const UCHAR *input, ULONG input_len, UCHAR *output, ULONG output_len )
{
    int ret;
    if ((ret = pgnutls_cipher_encrypt2( key->u.s.handle, input, input_len, output, output_len )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }
    return STATUS_SUCCESS;
}

NTSTATUS key_symmetric_decrypt( struct key *key, const UCHAR *input, ULONG input_len, UCHAR *output, ULONG output_len  )
{
    int ret;
    if ((ret = pgnutls_cipher_decrypt2( key->u.s.handle, input, input_len, output, output_len )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }
    return STATUS_SUCCESS;
}

NTSTATUS key_symmetric_get_tag( struct key *key, UCHAR *tag, ULONG len )
{
    int ret;
    if ((ret = pgnutls_cipher_tag( key->u.s.handle, tag, len )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }
    return STATUS_SUCCESS;
}

NTSTATUS key_asymmetric_init( struct key *key, struct algorithm *alg, const UCHAR *pubkey, ULONG pubkey_len )
{
    if (!libgnutls_handle) return STATUS_INTERNAL_ERROR;

    switch (alg->id)
    {
    case ALG_ID_ECDSA_P256:
    case ALG_ID_ECDSA_P384:
    case ALG_ID_RSA:
        break;

    default:
        FIXME( "algorithm %u not supported\n", alg->id );
        return STATUS_NOT_SUPPORTED;
    }

    if (!(key->u.a.pubkey = heap_alloc( pubkey_len ))) return STATUS_NO_MEMORY;
    memcpy( key->u.a.pubkey, pubkey, pubkey_len );
    key->u.a.pubkey_len = pubkey_len;
    key->alg_id         = alg->id;

    return STATUS_SUCCESS;
}

static NTSTATUS import_gnutls_pubkey_ecc( struct key *key, gnutls_pubkey_t *gnutls_key )
{
    BCRYPT_ECCKEY_BLOB *ecc_blob;
    gnutls_ecc_curve_t curve;
    gnutls_datum_t x, y;
    int ret;

    switch (key->alg_id)
    {
    case ALG_ID_ECDSA_P256: curve = GNUTLS_ECC_CURVE_SECP256R1; break;
    case ALG_ID_ECDSA_P384: curve = GNUTLS_ECC_CURVE_SECP384R1; break;

    default:
        FIXME( "algorithm %u not yet supported\n", key->alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }

    if ((ret = pgnutls_pubkey_init( gnutls_key )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    ecc_blob = (BCRYPT_ECCKEY_BLOB *)key->u.a.pubkey;
    x.data = key->u.a.pubkey + sizeof(*ecc_blob);
    x.size = ecc_blob->cbKey;
    y.data = key->u.a.pubkey + sizeof(*ecc_blob) + ecc_blob->cbKey;
    y.size = ecc_blob->cbKey;

    if ((ret = pgnutls_pubkey_import_ecc_raw( *gnutls_key, curve, &x, &y )))
    {
        pgnutls_perror( ret );
        pgnutls_pubkey_deinit( *gnutls_key );
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS import_gnutls_pubkey_rsa( struct key *key, gnutls_pubkey_t *gnutls_key )
{
    BCRYPT_RSAKEY_BLOB *rsa_blob;
    gnutls_datum_t m, e;
    int ret;

    if ((ret = pgnutls_pubkey_init( gnutls_key )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    rsa_blob = (BCRYPT_RSAKEY_BLOB *)key->u.a.pubkey;
    e.data = key->u.a.pubkey + sizeof(*rsa_blob);
    e.size = rsa_blob->cbPublicExp;
    m.data = key->u.a.pubkey + sizeof(*rsa_blob) + rsa_blob->cbPublicExp;
    m.size = rsa_blob->cbModulus;

    if ((ret = pgnutls_pubkey_import_rsa_raw( *gnutls_key, &m, &e )))
    {
        pgnutls_perror( ret );
        pgnutls_pubkey_deinit( *gnutls_key );
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS import_gnutls_pubkey( struct key *key, gnutls_pubkey_t *gnutls_key )
{
    switch (key->alg_id)
    {
    case ALG_ID_ECDSA_P256:
    case ALG_ID_ECDSA_P384:
        return import_gnutls_pubkey_ecc( key, gnutls_key );

    case ALG_ID_RSA:
        return import_gnutls_pubkey_rsa( key, gnutls_key );

    default:
        FIXME("algorithm %u not yet supported\n", key->alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }
}

static NTSTATUS prepare_gnutls_signature_ecc( struct key *key, UCHAR *signature, ULONG signature_len,
                                              gnutls_datum_t *gnutls_signature )
{
    struct buffer buffer;
    DWORD r_len = signature_len / 2;
    DWORD s_len = r_len;
    BYTE *r = signature;
    BYTE *s = signature + r_len;

    buffer_init( &buffer );
    buffer_append_asn1_r_s( &buffer, r, r_len, s, s_len );
    if (buffer.error)
    {
        buffer_free( &buffer );
        return STATUS_NO_MEMORY;
    }

    gnutls_signature->data = buffer.buffer;
    gnutls_signature->size = buffer.pos;
    return STATUS_SUCCESS;
}

static NTSTATUS prepare_gnutls_signature_rsa( struct key *key, UCHAR *signature, ULONG signature_len,
                                              gnutls_datum_t *gnutls_signature )
{
    gnutls_signature->data = signature;
    gnutls_signature->size = signature_len;
    return STATUS_SUCCESS;
}

static NTSTATUS prepare_gnutls_signature( struct key *key, UCHAR *signature, ULONG signature_len,
                                          gnutls_datum_t *gnutls_signature )
{
    switch (key->alg_id)
    {
    case ALG_ID_ECDSA_P256:
    case ALG_ID_ECDSA_P384:
        return prepare_gnutls_signature_ecc( key, signature, signature_len, gnutls_signature );

    case ALG_ID_RSA:
        return prepare_gnutls_signature_rsa( key, signature, signature_len, gnutls_signature );

    default:
        FIXME( "algorithm %u not yet supported\n", key->alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }
}

NTSTATUS key_asymmetric_verify( struct key *key, void *padding, UCHAR *hash, ULONG hash_len, UCHAR *signature,
                                ULONG signature_len, DWORD flags )
{
    gnutls_digest_algorithm_t hash_alg;
    gnutls_sign_algorithm_t sign_alg;
    gnutls_datum_t gnutls_hash, gnutls_signature;
    gnutls_pk_algorithm_t pk_alg;
    gnutls_pubkey_t gnutls_key;
    NTSTATUS status;
    int ret;

    switch (key->alg_id)
    {
    case ALG_ID_ECDSA_P256:
    case ALG_ID_ECDSA_P384:
    {
        if (flags) FIXME( "flags %08x not supported\n", flags );

        /* only the hash size must match, not the actual hash function */
        switch (hash_len)
        {
        case 32: hash_alg = GNUTLS_DIG_SHA256; break;
        case 48: hash_alg = GNUTLS_DIG_SHA384; break;

        default:
            FIXME( "hash size %u not yet supported\n", hash_len );
            return STATUS_INVALID_SIGNATURE;
        }
        pk_alg = GNUTLS_PK_ECC;
        break;
    }
    case ALG_ID_RSA:
    {
        BCRYPT_PKCS1_PADDING_INFO *info = (BCRYPT_PKCS1_PADDING_INFO *)padding;

        if (!(flags & BCRYPT_PAD_PKCS1) || !info) return STATUS_INVALID_PARAMETER;
        if (!info->pszAlgId) return STATUS_INVALID_SIGNATURE;

        if (!strcmpW( info->pszAlgId, BCRYPT_SHA1_ALGORITHM )) hash_alg = GNUTLS_DIG_SHA1;
        else if (!strcmpW( info->pszAlgId, BCRYPT_SHA256_ALGORITHM )) hash_alg = GNUTLS_DIG_SHA256;
        else if (!strcmpW( info->pszAlgId, BCRYPT_SHA384_ALGORITHM )) hash_alg = GNUTLS_DIG_SHA384;
        else if (!strcmpW( info->pszAlgId, BCRYPT_SHA512_ALGORITHM )) hash_alg = GNUTLS_DIG_SHA512;
        else
        {
            FIXME( "hash algorithm %s not supported\n", debugstr_w(info->pszAlgId) );
            return STATUS_NOT_SUPPORTED;
        }
        pk_alg = GNUTLS_PK_RSA;
        break;
    }
    default:
        FIXME( "algorithm %u not yet supported\n", key->alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }

    if ((sign_alg = pgnutls_pk_to_sign( pk_alg, hash_alg )) == GNUTLS_SIGN_UNKNOWN)
    {
        FIXME("GnuTLS does not support algorithm %u with hash len %u\n", key->alg_id, hash_len );
        return STATUS_NOT_IMPLEMENTED;
    }

    if ((status = import_gnutls_pubkey( key, &gnutls_key ))) return status;
    if ((status = prepare_gnutls_signature( key, signature, signature_len, &gnutls_signature )))
    {
        pgnutls_pubkey_deinit( gnutls_key );
        return status;
    }

    gnutls_hash.data = hash;
    gnutls_hash.size = hash_len;
    ret = pgnutls_pubkey_verify_hash2( gnutls_key, sign_alg, 0, &gnutls_hash, &gnutls_signature );

    if (gnutls_signature.data != signature) heap_free( gnutls_signature.data );
    pgnutls_pubkey_deinit( gnutls_key );
    return (ret < 0) ? STATUS_INVALID_SIGNATURE : STATUS_SUCCESS;
}

NTSTATUS key_destroy( struct key *key )
{
    if (key_is_symmetric( key ))
    {
        if (key->u.s.handle) pgnutls_cipher_deinit( key->u.s.handle );
        heap_free( key->u.s.secret );
    }
    else heap_free( key->u.a.pubkey );
    heap_free( key );
    return STATUS_SUCCESS;
}
#endif
