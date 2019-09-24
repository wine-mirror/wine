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
#ifdef HAVE_COMMONCRYPTO_COMMONCRYPTOR_H
#include <AvailabilityMacros.h>
#include <CommonCrypto/CommonCryptor.h>
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

#if defined(HAVE_COMMONCRYPTO_COMMONCRYPTOR_H) && MAC_OS_X_VERSION_MAX_ALLOWED >= 1080 && !defined(HAVE_GNUTLS_CIPHER_INIT)
WINE_DEFAULT_DEBUG_CHANNEL(bcrypt);

NTSTATUS key_set_property( struct key *key, const WCHAR *prop, UCHAR *value, ULONG size, ULONG flags )
{
    if (!strcmpW( prop, BCRYPT_CHAINING_MODE ))
    {
        if (!strcmpW( (WCHAR *)value, BCRYPT_CHAIN_MODE_ECB ))
        {
            key->u.s.mode = MODE_ID_ECB;
            return STATUS_SUCCESS;
        }
        else if (!strcmpW( (WCHAR *)value, BCRYPT_CHAIN_MODE_CBC ))
        {
            key->u.s.mode = MODE_ID_CBC;
            return STATUS_SUCCESS;
        }
        else
        {
            FIXME( "unsupported mode %s\n", debugstr_w((WCHAR *)value) );
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
    switch (alg->id)
    {
    case ALG_ID_AES:
        switch (alg->mode)
        {
        case MODE_ID_ECB:
        case MODE_ID_CBC:
            break;
        default:
            FIXME( "mode %u not supported\n", alg->mode );
            return STATUS_NOT_SUPPORTED;
        }
        break;

    default:
        FIXME( "algorithm %u not supported\n", alg->id );
        return STATUS_NOT_SUPPORTED;
    }

    if (!(key->u.s.block_size = get_block_size( alg ))) return STATUS_INVALID_PARAMETER;
    if (!(key->u.s.secret = heap_alloc( secret_len ))) return STATUS_NO_MEMORY;
    memcpy( key->u.s.secret, secret, secret_len );
    key->u.s.secret_len = secret_len;

    key->alg_id          = alg->id;
    key->u.s.mode        = alg->mode;
    key->u.s.ref_encrypt = NULL;        /* initialized on first use */
    key->u.s.ref_decrypt = NULL;

    return STATUS_SUCCESS;
}

static CCMode get_cryptor_mode( struct key *key )
{
    switch (key->u.s.mode)
    {
    case MODE_ID_ECB: return kCCModeECB;
    case MODE_ID_CBC: return kCCModeCBC;
    default:
        FIXME( "unsupported mode %u\n", key->u.s.mode );
        return 0;
    }
}

NTSTATUS key_symmetric_set_params( struct key *key, UCHAR *iv, ULONG iv_len )
{
    CCCryptorStatus status;
    CCMode mode;

    if (!(mode = get_cryptor_mode( key ))) return STATUS_NOT_SUPPORTED;

    if (key->u.s.ref_encrypt)
    {
        CCCryptorRelease( key->u.s.ref_encrypt );
        key->u.s.ref_encrypt = NULL;
    }
    if (key->u.s.ref_decrypt)
    {
        CCCryptorRelease( key->u.s.ref_decrypt );
        key->u.s.ref_decrypt = NULL;
    }

    if ((status = CCCryptorCreateWithMode( kCCEncrypt, mode, kCCAlgorithmAES128, ccNoPadding, iv, key->u.s.secret,
                                           key->u.s.secret_len, NULL, 0, 0, 0, &key->u.s.ref_encrypt )) != kCCSuccess)
    {
        WARN( "CCCryptorCreateWithMode failed %d\n", status );
        return STATUS_INTERNAL_ERROR;
    }
    if ((status = CCCryptorCreateWithMode( kCCDecrypt, mode, kCCAlgorithmAES128, ccNoPadding, iv, key->u.s.secret,
                                           key->u.s.secret_len, NULL, 0, 0, 0, &key->u.s.ref_decrypt )) != kCCSuccess)
    {
        WARN( "CCCryptorCreateWithMode failed %d\n", status );
        CCCryptorRelease( key->u.s.ref_encrypt );
        key->u.s.ref_encrypt = NULL;
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

NTSTATUS key_symmetric_set_auth_data( struct key *key, UCHAR *auth_data, ULONG len )
{
    FIXME( "not implemented on Mac\n" );
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS key_symmetric_encrypt( struct key *key, const UCHAR *input, ULONG input_len, UCHAR *output, ULONG output_len  )
{
    CCCryptorStatus status;
    if ((status = CCCryptorUpdate( key->u.s.ref_encrypt, input, input_len, output, output_len, NULL  )) != kCCSuccess)
    {
        WARN( "CCCryptorUpdate failed %d\n", status );
        return STATUS_INTERNAL_ERROR;
    }
    return STATUS_SUCCESS;
}

NTSTATUS key_symmetric_decrypt( struct key *key, const UCHAR *input, ULONG input_len, UCHAR *output, ULONG output_len )
{
    CCCryptorStatus status;
    if ((status = CCCryptorUpdate( key->u.s.ref_decrypt, input, input_len, output, output_len, NULL  )) != kCCSuccess)
    {
        WARN( "CCCryptorUpdate failed %d\n", status );
        return STATUS_INTERNAL_ERROR;
    }
    return STATUS_SUCCESS;
}

NTSTATUS key_symmetric_get_tag( struct key *key, UCHAR *tag, ULONG len )
{
    FIXME( "not implemented on Mac\n" );
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS key_asymmetric_init( struct key *key, struct algorithm *alg, ULONG bitlen, const UCHAR *pubkey,
                              ULONG pubkey_len )
{
    FIXME( "not implemented on Mac\n" );
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS key_asymmetric_sign( struct key *key, void *padding, UCHAR *input, ULONG input_len, UCHAR *output,
                              ULONG output_len, ULONG *ret_len, ULONG flags )
{
    FIXME( "not implemented on Mac\n" );
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS key_asymmetric_verify( struct key *key, void *padding, UCHAR *hash, ULONG hash_len, UCHAR *signature,
                                ULONG signature_len, DWORD flags )
{
    FIXME( "not implemented on Mac\n" );
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS key_export_ecc( struct key *key, UCHAR *output, ULONG len, ULONG *ret_len )
{
    FIXME( "not implemented on Mac\n" );
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS key_import_ecc( struct key *key, UCHAR *input, ULONG len )
{
    FIXME( "not implemented on Mac\n" );
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS key_asymmetric_generate( struct key *key )
{
    FIXME( "not implemented on Mac\n" );
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS key_destroy( struct key *key )
{
    if (key->u.s.ref_encrypt) CCCryptorRelease( key->u.s.ref_encrypt );
    if (key->u.s.ref_decrypt) CCCryptorRelease( key->u.s.ref_decrypt );
    heap_free( key->u.s.secret );
    heap_free( key );
    return STATUS_SUCCESS;
}
#endif
