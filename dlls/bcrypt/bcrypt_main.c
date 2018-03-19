/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
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
#elif defined(HAVE_GNUTLS_CIPHER_INIT)
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
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

WINE_DEFAULT_DEBUG_CHANNEL(bcrypt);

static HINSTANCE instance;

#if defined(HAVE_GNUTLS_CIPHER_INIT) && !defined(HAVE_COMMONCRYPTO_COMMONCRYPTOR_H)
WINE_DECLARE_DEBUG_CHANNEL(winediag);

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
#undef MAKE_FUNCPTR

static void gnutls_log( int level, const char *msg )
{
    TRACE( "<%d> %s", level, msg );
}

static BOOL gnutls_initialize(void)
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
#undef LOAD_FUNCPTR

    if ((ret = pgnutls_global_init()) != GNUTLS_E_SUCCESS)
    {
        pgnutls_perror( ret );
        goto fail;
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

static void gnutls_uninitialize(void)
{
    pgnutls_global_deinit();
    wine_dlclose( libgnutls_handle, NULL, 0 );
    libgnutls_handle = NULL;
}
#endif /* HAVE_GNUTLS_CIPHER_INIT && !HAVE_COMMONCRYPTO_COMMONCRYPTOR_H */

NTSTATUS WINAPI BCryptAddContextFunction(ULONG table, LPCWSTR context, ULONG iface, LPCWSTR function, ULONG pos)
{
    FIXME("%08x, %s, %08x, %s, %u: stub\n", table, debugstr_w(context), iface, debugstr_w(function), pos);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptAddContextFunctionProvider(ULONG table, LPCWSTR context, ULONG iface, LPCWSTR function, LPCWSTR provider, ULONG pos)
{
    FIXME("%08x, %s, %08x, %s, %s, %u: stub\n", table, debugstr_w(context), iface, debugstr_w(function), debugstr_w(provider), pos);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptRemoveContextFunction(ULONG table, LPCWSTR context, ULONG iface, LPCWSTR function)
{
    FIXME("%08x, %s, %08x, %s: stub\n", table, debugstr_w(context), iface, debugstr_w(function));
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI BCryptRemoveContextFunctionProvider(ULONG table, LPCWSTR context, ULONG iface, LPCWSTR function, LPCWSTR provider)
{
    FIXME("%08x, %s, %08x, %s, %s: stub\n", table, debugstr_w(context), iface, debugstr_w(function), debugstr_w(provider));
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI BCryptRegisterProvider(LPCWSTR provider, ULONG flags, PCRYPT_PROVIDER_REG reg)
{
    FIXME("%s, %08x, %p: stub\n", debugstr_w(provider), flags, reg);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptUnregisterProvider(LPCWSTR provider)
{
    FIXME("%s: stub\n", debugstr_w(provider));
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI BCryptEnumAlgorithms(ULONG dwAlgOperations, ULONG *pAlgCount,
                                     BCRYPT_ALGORITHM_IDENTIFIER **ppAlgList, ULONG dwFlags)
{
    FIXME("%08x, %p, %p, %08x - stub\n", dwAlgOperations, pAlgCount, ppAlgList, dwFlags);

    *ppAlgList=NULL;
    *pAlgCount=0;

    return STATUS_NOT_IMPLEMENTED;
}

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
    ALG_ID_SHA1,
    ALG_ID_SHA256,
    ALG_ID_SHA384,
    ALG_ID_SHA512
};

enum mode_id
{
    MODE_ID_CBC,
    MODE_ID_GCM
};

#define MAX_HASH_OUTPUT_BYTES 64
#define MAX_HASH_BLOCK_BITS 1024

static const struct {
    ULONG object_length;
    ULONG hash_length;
    ULONG block_bits;
    const WCHAR *alg_name;
} alg_props[] = {
    /* ALG_ID_AES    */ {  654,    0,    0, BCRYPT_AES_ALGORITHM },
    /* ALG_ID_MD2    */ {  270,   16,  128, BCRYPT_MD2_ALGORITHM },
    /* ALG_ID_MD4    */ {  270,   16,  512, BCRYPT_MD4_ALGORITHM },
    /* ALG_ID_MD5    */ {  274,   16,  512, BCRYPT_MD5_ALGORITHM },
    /* ALG_ID_RNG    */ {    0,    0,    0, BCRYPT_RNG_ALGORITHM },
    /* ALG_ID_SHA1   */ {  278,   20,  512, BCRYPT_SHA1_ALGORITHM },
    /* ALG_ID_SHA256 */ {  286,   32,  512, BCRYPT_SHA256_ALGORITHM },
    /* ALG_ID_SHA384 */ {  382,   48, 1024, BCRYPT_SHA384_ALGORITHM },
    /* ALG_ID_SHA512 */ {  382,   64, 1024, BCRYPT_SHA512_ALGORITHM }
};

struct algorithm
{
    struct object hdr;
    enum alg_id   id;
    enum mode_id  mode;
    BOOL hmac;
};

NTSTATUS WINAPI BCryptGenRandom(BCRYPT_ALG_HANDLE handle, UCHAR *buffer, ULONG count, ULONG flags)
{
    const DWORD supported_flags = BCRYPT_USE_SYSTEM_PREFERRED_RNG;
    struct algorithm *algorithm = handle;

    TRACE("%p, %p, %u, %08x - semi-stub\n", handle, buffer, count, flags);

    if (!algorithm)
    {
        /* It's valid to call without an algorithm if BCRYPT_USE_SYSTEM_PREFERRED_RNG
         * is set. In this case the preferred system RNG is used.
         */
        if (!(flags & BCRYPT_USE_SYSTEM_PREFERRED_RNG))
            return STATUS_INVALID_HANDLE;
    }
    else if (algorithm->hdr.magic != MAGIC_ALG || algorithm->id != ALG_ID_RNG)
        return STATUS_INVALID_HANDLE;

    if (!buffer)
        return STATUS_INVALID_PARAMETER;

    if (flags & ~supported_flags)
        FIXME("unsupported flags %08x\n", flags & ~supported_flags);

    if (algorithm)
        FIXME("ignoring selected algorithm\n");

    /* When zero bytes are requested the function returns success too. */
    if (!count)
        return STATUS_SUCCESS;

    if (algorithm || (flags & BCRYPT_USE_SYSTEM_PREFERRED_RNG))
    {
        if (RtlGenRandom(buffer, count))
            return STATUS_SUCCESS;
    }

    FIXME("called with unsupported parameters, returning error\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI BCryptOpenAlgorithmProvider( BCRYPT_ALG_HANDLE *handle, LPCWSTR id, LPCWSTR implementation, DWORD flags )
{
    const DWORD supported_flags = BCRYPT_ALG_HANDLE_HMAC_FLAG;
    struct algorithm *alg;
    enum alg_id alg_id;

    TRACE( "%p, %s, %s, %08x\n", handle, wine_dbgstr_w(id), wine_dbgstr_w(implementation), flags );

    if (!handle || !id) return STATUS_INVALID_PARAMETER;
    if (flags & ~supported_flags)
    {
        FIXME( "unsupported flags %08x\n", flags & ~supported_flags);
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!strcmpW( id, BCRYPT_AES_ALGORITHM )) alg_id = ALG_ID_AES;
    else if (!strcmpW( id, BCRYPT_MD2_ALGORITHM )) alg_id = ALG_ID_MD2;
    else if (!strcmpW( id, BCRYPT_MD4_ALGORITHM )) alg_id = ALG_ID_MD4;
    else if (!strcmpW( id, BCRYPT_MD5_ALGORITHM )) alg_id = ALG_ID_MD5;
    else if (!strcmpW( id, BCRYPT_RNG_ALGORITHM )) alg_id = ALG_ID_RNG;
    else if (!strcmpW( id, BCRYPT_SHA1_ALGORITHM )) alg_id = ALG_ID_SHA1;
    else if (!strcmpW( id, BCRYPT_SHA256_ALGORITHM )) alg_id = ALG_ID_SHA256;
    else if (!strcmpW( id, BCRYPT_SHA384_ALGORITHM )) alg_id = ALG_ID_SHA384;
    else if (!strcmpW( id, BCRYPT_SHA512_ALGORITHM )) alg_id = ALG_ID_SHA512;
    else
    {
        FIXME( "algorithm %s not supported\n", debugstr_w(id) );
        return STATUS_NOT_IMPLEMENTED;
    }
    if (implementation && strcmpW( implementation, MS_PRIMITIVE_PROVIDER ))
    {
        FIXME( "implementation %s not supported\n", debugstr_w(implementation) );
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!(alg = heap_alloc( sizeof(*alg) ))) return STATUS_NO_MEMORY;
    alg->hdr.magic = MAGIC_ALG;
    alg->id        = alg_id;
    alg->mode      = MODE_ID_CBC;
    alg->hmac      = flags & BCRYPT_ALG_HANDLE_HMAC_FLAG;

    *handle = alg;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptCloseAlgorithmProvider( BCRYPT_ALG_HANDLE handle, DWORD flags )
{
    struct algorithm *alg = handle;

    TRACE( "%p, %08x\n", handle, flags );

    if (!alg || alg->hdr.magic != MAGIC_ALG) return STATUS_INVALID_HANDLE;
    heap_free( alg );
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptGetFipsAlgorithmMode(BOOLEAN *enabled)
{
    FIXME("%p - semi-stub\n", enabled);

    if (!enabled)
        return STATUS_INVALID_PARAMETER;

    *enabled = FALSE;
    return STATUS_SUCCESS;
}

struct hash_impl
{
    union
    {
        MD2_CTX md2;
        MD4_CTX md4;
        MD5_CTX md5;
        SHA_CTX sha1;
        SHA256_CTX sha256;
        SHA512_CTX sha512;
    } u;
};

static NTSTATUS hash_init( struct hash_impl *hash, enum alg_id alg_id )
{
    switch (alg_id)
    {
    case ALG_ID_MD2:
        md2_init( &hash->u.md2 );
        break;

    case ALG_ID_MD4:
        MD4Init( &hash->u.md4 );
        break;

    case ALG_ID_MD5:
        MD5Init( &hash->u.md5 );
        break;

    case ALG_ID_SHA1:
        A_SHAInit( &hash->u.sha1 );
        break;

    case ALG_ID_SHA256:
        sha256_init( &hash->u.sha256 );
        break;

    case ALG_ID_SHA384:
        sha384_init( &hash->u.sha512 );
        break;

    case ALG_ID_SHA512:
        sha512_init( &hash->u.sha512 );
        break;

    default:
        ERR( "unhandled id %u\n", alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS hash_update( struct hash_impl *hash, enum alg_id alg_id,
                             UCHAR *input, ULONG size )
{
    switch (alg_id)
    {
    case ALG_ID_MD2:
        md2_update( &hash->u.md2, input, size );
        break;

    case ALG_ID_MD4:
        MD4Update( &hash->u.md4, input, size );
        break;

    case ALG_ID_MD5:
        MD5Update( &hash->u.md5, input, size );
        break;

    case ALG_ID_SHA1:
        A_SHAUpdate( &hash->u.sha1, input, size );
        break;

    case ALG_ID_SHA256:
        sha256_update( &hash->u.sha256, input, size );
        break;

    case ALG_ID_SHA384:
        sha384_update( &hash->u.sha512, input, size );
        break;

    case ALG_ID_SHA512:
        sha512_update( &hash->u.sha512, input, size );
        break;

    default:
        ERR( "unhandled id %u\n", alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS hash_finish( struct hash_impl *hash, enum alg_id alg_id,
                             UCHAR *output, ULONG size )
{
    switch (alg_id)
    {
    case ALG_ID_MD2:
        md2_finalize( &hash->u.md2, output );
        break;

    case ALG_ID_MD4:
        MD4Final( &hash->u.md4 );
        memcpy( output, hash->u.md4.digest, 16 );
        break;

    case ALG_ID_MD5:
        MD5Final( &hash->u.md5 );
        memcpy( output, hash->u.md5.digest, 16 );
        break;

    case ALG_ID_SHA1:
        A_SHAFinal( &hash->u.sha1, (ULONG *)output );
        break;

    case ALG_ID_SHA256:
        sha256_finalize( &hash->u.sha256, output );
        break;

    case ALG_ID_SHA384:
        sha384_finalize( &hash->u.sha512, output );
        break;

    case ALG_ID_SHA512:
        sha512_finalize( &hash->u.sha512, output );
        break;

    default:
        ERR( "unhandled id %u\n", alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }
    return STATUS_SUCCESS;
}

struct hash
{
    struct object    hdr;
    enum alg_id      alg_id;
    BOOL             hmac;
    struct hash_impl outer;
    struct hash_impl inner;
};

#define BLOCK_LENGTH_AES        16

static NTSTATUS generic_alg_property( enum alg_id id, const WCHAR *prop, UCHAR *buf, ULONG size, ULONG *ret_size )
{
    if (!strcmpW( prop, BCRYPT_OBJECT_LENGTH ))
    {
        if (!alg_props[id].object_length)
            return STATUS_NOT_SUPPORTED;
        *ret_size = sizeof(ULONG);
        if (size < sizeof(ULONG))
            return STATUS_BUFFER_TOO_SMALL;
        if (buf)
            *(ULONG *)buf = alg_props[id].object_length;
        return STATUS_SUCCESS;
    }

    if (!strcmpW( prop, BCRYPT_HASH_LENGTH ))
    {
        if (!alg_props[id].hash_length)
            return STATUS_NOT_SUPPORTED;
        *ret_size = sizeof(ULONG);
        if (size < sizeof(ULONG))
            return STATUS_BUFFER_TOO_SMALL;
        if(buf)
            *(ULONG*)buf = alg_props[id].hash_length;
        return STATUS_SUCCESS;
    }

    if (!strcmpW( prop, BCRYPT_ALGORITHM_NAME ))
    {
        *ret_size = (strlenW(alg_props[id].alg_name)+1)*sizeof(WCHAR);
        if (size < *ret_size)
            return STATUS_BUFFER_TOO_SMALL;
        if(buf)
            memcpy(buf, alg_props[id].alg_name, *ret_size);
        return STATUS_SUCCESS;
    }

    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS get_alg_property( const struct algorithm *alg, const WCHAR *prop, UCHAR *buf, ULONG size, ULONG *ret_size )
{
    NTSTATUS status;

    status = generic_alg_property( alg->id, prop, buf, size, ret_size );
    if (status != STATUS_NOT_IMPLEMENTED)
        return status;

    switch (alg->id)
    {
    case ALG_ID_AES:
        if (!strcmpW( prop, BCRYPT_BLOCK_LENGTH ))
        {
            *ret_size = sizeof(ULONG);
            if (size < sizeof(ULONG))
                return STATUS_BUFFER_TOO_SMALL;
            if (buf)
                *(ULONG *)buf = BLOCK_LENGTH_AES;
            return STATUS_SUCCESS;
        }
        if (!strcmpW( prop, BCRYPT_CHAINING_MODE ))
        {
            const WCHAR *mode;
            switch (alg->mode)
            {
                case MODE_ID_GCM: mode = BCRYPT_CHAIN_MODE_GCM; break;
                case MODE_ID_CBC: mode = BCRYPT_CHAIN_MODE_CBC; break;
                default: return STATUS_NOT_IMPLEMENTED;
            }

            *ret_size = 64;
            if (size < *ret_size) return STATUS_BUFFER_TOO_SMALL;
            memcpy( buf, mode, (strlenW(mode) + 1) * sizeof(WCHAR) );
            return STATUS_SUCCESS;
        }
        if (!strcmpW( prop, BCRYPT_KEY_LENGTHS ))
        {
            BCRYPT_KEY_LENGTHS_STRUCT *key_lengths = (void *)buf;
            *ret_size = sizeof(*key_lengths);
            if (key_lengths && size < *ret_size) return STATUS_BUFFER_TOO_SMALL;
            if (key_lengths)
            {
                key_lengths->dwMinLength = 128;
                key_lengths->dwMaxLength = 256;
                key_lengths->dwIncrement = 64;
            }
            return STATUS_SUCCESS;
        }
        if (!strcmpW( prop, BCRYPT_AUTH_TAG_LENGTH ))
        {
            BCRYPT_AUTH_TAG_LENGTHS_STRUCT *tag_length = (void *)buf;
            if (alg->mode != MODE_ID_GCM) return STATUS_NOT_SUPPORTED;
            *ret_size = sizeof(*tag_length);
            if (tag_length && size < *ret_size) return STATUS_BUFFER_TOO_SMALL;
            if (tag_length)
            {
                tag_length->dwMinLength = 12;
                tag_length->dwMaxLength = 16;
                tag_length->dwIncrement =  1;
            }
            return STATUS_SUCCESS;
        }
        break;

    default:
        break;
    }

    FIXME( "unsupported property %s\n", debugstr_w(prop) );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS set_alg_property( struct algorithm *alg, const WCHAR *prop, UCHAR *value, ULONG size, ULONG flags )
{
    switch (alg->id)
    {
    case ALG_ID_AES:
        if (!strcmpW( prop, BCRYPT_CHAINING_MODE ))
        {
            if (!strncmpW( (WCHAR *)value, BCRYPT_CHAIN_MODE_CBC, size ))
            {
                alg->mode = MODE_ID_CBC;
                return STATUS_SUCCESS;
            }
            else if (!strncmpW( (WCHAR *)value, BCRYPT_CHAIN_MODE_GCM, size ))
            {
                alg->mode = MODE_ID_GCM;
                return STATUS_SUCCESS;
            }
            else
            {
                FIXME( "unsupported mode %s\n", debugstr_wn( (WCHAR *)value, size ) );
                return STATUS_NOT_IMPLEMENTED;
            }
        }
        FIXME( "unsupported aes algorithm property %s\n", debugstr_w(prop) );
        return STATUS_NOT_IMPLEMENTED;

    default:
        FIXME( "unsupported algorithm %u\n", alg->id );
        return STATUS_NOT_IMPLEMENTED;
    }
}

static NTSTATUS get_hash_property( const struct hash *hash, const WCHAR *prop, UCHAR *buf, ULONG size, ULONG *ret_size )
{
    NTSTATUS status;

    status = generic_alg_property( hash->alg_id, prop, buf, size, ret_size );
    if (status == STATUS_NOT_IMPLEMENTED)
        FIXME( "unsupported property %s\n", debugstr_w(prop) );
    return status;
}

NTSTATUS WINAPI BCryptGetProperty( BCRYPT_HANDLE handle, LPCWSTR prop, UCHAR *buffer, ULONG count, ULONG *res, ULONG flags )
{
    struct object *object = handle;

    TRACE( "%p, %s, %p, %u, %p, %08x\n", handle, wine_dbgstr_w(prop), buffer, count, res, flags );

    if (!object) return STATUS_INVALID_HANDLE;
    if (!prop || !res) return STATUS_INVALID_PARAMETER;

    switch (object->magic)
    {
    case MAGIC_ALG:
    {
        const struct algorithm *alg = (const struct algorithm *)object;
        return get_alg_property( alg, prop, buffer, count, res );
    }
    case MAGIC_HASH:
    {
        const struct hash *hash = (const struct hash *)object;
        return get_hash_property( hash, prop, buffer, count, res );
    }
    default:
        WARN( "unknown magic %08x\n", object->magic );
        return STATUS_INVALID_HANDLE;
    }
}

NTSTATUS WINAPI BCryptSetProperty( BCRYPT_HANDLE handle, const WCHAR *prop, UCHAR *value, ULONG size, ULONG flags )
{
    struct object *object = handle;

    TRACE( "%p, %s, %p, %u, %08x\n", handle, debugstr_w(prop), value, size, flags );

    if (!object) return STATUS_INVALID_HANDLE;

    switch (object->magic)
    {
    case MAGIC_ALG:
    {
        struct algorithm *alg = (struct algorithm *)object;
        return set_alg_property( alg, prop, value, size, flags );
    }
    case MAGIC_KEY:
    {
        FIXME( "keys not implemented yet\n" );
        return STATUS_NOT_IMPLEMENTED;
    }
    default:
        WARN( "unknown magic %08x\n", object->magic );
        return STATUS_INVALID_HANDLE;
    }
}

NTSTATUS WINAPI BCryptCreateHash( BCRYPT_ALG_HANDLE algorithm, BCRYPT_HASH_HANDLE *handle, UCHAR *object, ULONG objectlen,
                                  UCHAR *secret, ULONG secretlen, ULONG flags )
{
    struct algorithm *alg = algorithm;
    UCHAR buffer[MAX_HASH_BLOCK_BITS / 8] = {0};
    struct hash *hash;
    int block_bytes;
    NTSTATUS status;
    int i;

    TRACE( "%p, %p, %p, %u, %p, %u, %08x - stub\n", algorithm, handle, object, objectlen,
           secret, secretlen, flags );
    if (flags)
    {
        FIXME( "unimplemented flags %08x\n", flags );
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!alg || alg->hdr.magic != MAGIC_ALG) return STATUS_INVALID_HANDLE;
    if (object) FIXME( "ignoring object buffer\n" );

    if (!(hash = heap_alloc( sizeof(*hash) ))) return STATUS_NO_MEMORY;
    hash->hdr.magic = MAGIC_HASH;
    hash->alg_id    = alg->id;
    hash->hmac      = alg->hmac;

    /* initialize hash */
    if ((status = hash_init( &hash->inner, hash->alg_id ))) goto end;
    if (!hash->hmac) goto end;

    /* initialize hmac */
    if ((status = hash_init( &hash->outer, hash->alg_id ))) goto end;
    block_bytes = alg_props[hash->alg_id].block_bits / 8;
    if (secretlen > block_bytes)
    {
        struct hash_impl temp;
        if ((status = hash_init( &temp, hash->alg_id ))) goto end;
        if ((status = hash_update( &temp, hash->alg_id, secret, secretlen ))) goto end;
        if ((status = hash_finish( &temp, hash->alg_id, buffer,
                                   alg_props[hash->alg_id].hash_length ))) goto end;
    }
    else
    {
        memcpy( buffer, secret, secretlen );
    }
    for (i = 0; i < block_bytes; i++) buffer[i] ^= 0x5c;
    if ((status = hash_update( &hash->outer, hash->alg_id, buffer, block_bytes ))) goto end;
    for (i = 0; i < block_bytes; i++) buffer[i] ^= (0x5c ^ 0x36);
    status = hash_update( &hash->inner, hash->alg_id, buffer, block_bytes );

end:
    if (status != STATUS_SUCCESS)
    {
        heap_free( hash );
        return status;
    }

    *handle = hash;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptDuplicateHash( BCRYPT_HASH_HANDLE handle, BCRYPT_HASH_HANDLE *handle_copy,
                                     UCHAR *object, ULONG objectlen, ULONG flags )
{
    struct hash *hash_orig = handle;
    struct hash *hash_copy;

    TRACE( "%p, %p, %p, %u, %u\n", handle, handle_copy, object, objectlen, flags );

    if (!hash_orig || hash_orig->hdr.magic != MAGIC_HASH) return STATUS_INVALID_HANDLE;
    if (!handle_copy) return STATUS_INVALID_PARAMETER;
    if (object) FIXME( "ignoring object buffer\n" );

    if (!(hash_copy = heap_alloc( sizeof(*hash_copy) )))
        return STATUS_NO_MEMORY;

    memcpy( hash_copy, hash_orig, sizeof(*hash_orig) );

    *handle_copy = hash_copy;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptDestroyHash( BCRYPT_HASH_HANDLE handle )
{
    struct hash *hash = handle;

    TRACE( "%p\n", handle );

    if (!hash || hash->hdr.magic != MAGIC_HASH) return STATUS_INVALID_HANDLE;
    heap_free( hash );
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptHashData( BCRYPT_HASH_HANDLE handle, UCHAR *input, ULONG size, ULONG flags )
{
    struct hash *hash = handle;

    TRACE( "%p, %p, %u, %08x\n", handle, input, size, flags );

    if (!hash || hash->hdr.magic != MAGIC_HASH) return STATUS_INVALID_HANDLE;
    if (!input) return STATUS_SUCCESS;

    return hash_update( &hash->inner, hash->alg_id, input, size );
}

NTSTATUS WINAPI BCryptFinishHash( BCRYPT_HASH_HANDLE handle, UCHAR *output, ULONG size, ULONG flags )
{
    UCHAR buffer[MAX_HASH_OUTPUT_BYTES];
    struct hash *hash = handle;
    NTSTATUS status;
    int hash_length;

    TRACE( "%p, %p, %u, %08x\n", handle, output, size, flags );

    if (!hash || hash->hdr.magic != MAGIC_HASH) return STATUS_INVALID_HANDLE;
    if (!output) return STATUS_INVALID_PARAMETER;

    if (!hash->hmac)
        return hash_finish( &hash->inner, hash->alg_id, output, size );

    hash_length = alg_props[hash->alg_id].hash_length;
    if ((status = hash_finish( &hash->inner, hash->alg_id, buffer, hash_length ))) return status;
    if ((status = hash_update( &hash->outer, hash->alg_id, buffer, hash_length ))) return status;
    return hash_finish( &hash->outer, hash->alg_id, output, size );
}

NTSTATUS WINAPI BCryptHash( BCRYPT_ALG_HANDLE algorithm, UCHAR *secret, ULONG secretlen,
                            UCHAR *input, ULONG inputlen, UCHAR *output, ULONG outputlen )
{
    NTSTATUS status;
    BCRYPT_HASH_HANDLE handle;

    TRACE( "%p, %p, %u, %p, %u, %p, %u\n", algorithm, secret, secretlen,
           input, inputlen, output, outputlen );

    status = BCryptCreateHash( algorithm, &handle, NULL, 0, secret, secretlen, 0);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    status = BCryptHashData( handle, input, inputlen, 0 );
    if (status != STATUS_SUCCESS)
    {
        BCryptDestroyHash( handle );
        return status;
    }

    status = BCryptFinishHash( handle, output, outputlen, 0 );
    if (status != STATUS_SUCCESS)
    {
        BCryptDestroyHash( handle );
        return status;
    }

    return BCryptDestroyHash( handle );
}

#if defined(HAVE_GNUTLS_CIPHER_INIT) && !defined(HAVE_COMMONCRYPTO_COMMONCRYPTOR_H)
struct key
{
    struct object      hdr;
    enum alg_id        alg_id;
    ULONG              block_size;
    gnutls_cipher_hd_t handle;
    UCHAR             *secret;
    ULONG              secret_len;
};
#elif defined(HAVE_COMMONCRYPTO_COMMONCRYPTOR_H) && MAC_OS_X_VERSION_MAX_ALLOWED >= 1080
struct key
{
    struct object  hdr;
    enum alg_id    alg_id;
    ULONG          block_size;
    CCCryptorRef   ref_encrypt;
    CCCryptorRef   ref_decrypt;
    UCHAR         *secret;
    ULONG          secret_len;
};
#else
struct key
{
    struct object hdr;
    ULONG         block_size;
};
#endif

#if defined(HAVE_GNUTLS_CIPHER_INIT) || defined(HAVE_COMMONCRYPTO_COMMONCRYPTOR_H) && MAC_OS_X_VERSION_MAX_ALLOWED >= 1080
static ULONG get_block_size( struct algorithm *alg )
{
    ULONG ret = 0, size = sizeof(ret);
    get_alg_property( alg, BCRYPT_BLOCK_LENGTH, (UCHAR *)&ret, sizeof(ret), &size );
    return ret;
}
static NTSTATUS key_export( struct key *key, const WCHAR *type, UCHAR *output, ULONG output_len, ULONG *size )
{
    if (!strcmpW( type, BCRYPT_KEY_DATA_BLOB ))
    {
        BCRYPT_KEY_DATA_BLOB_HEADER *header = (BCRYPT_KEY_DATA_BLOB_HEADER *)output;
        ULONG req_size = sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + key->secret_len;

        *size = req_size;
        if (output_len < req_size) return STATUS_BUFFER_TOO_SMALL;

        header->dwMagic   = BCRYPT_KEY_DATA_BLOB_MAGIC;
        header->dwVersion = BCRYPT_KEY_DATA_BLOB_VERSION1;
        header->cbKeyData = key->secret_len;
        memcpy( &header[1], key->secret, key->secret_len );
        return STATUS_SUCCESS;
    }

    FIXME( "unsupported key type %s\n", debugstr_w(type) );
    return STATUS_NOT_IMPLEMENTED;
}
#endif

#if defined(HAVE_GNUTLS_CIPHER_INIT) && !defined(HAVE_COMMONCRYPTO_COMMONCRYPTOR_H)
static NTSTATUS key_init( struct key *key, struct algorithm *alg, const UCHAR *secret, ULONG secret_len )
{
    UCHAR *buffer;

    if (!libgnutls_handle) return STATUS_INTERNAL_ERROR;

    switch (alg->id)
    {
    case ALG_ID_AES:
        break;

    default:
        FIXME( "algorithm %u not supported\n", alg->id );
        return STATUS_NOT_SUPPORTED;
    }

    if (!(key->block_size = get_block_size( alg ))) return STATUS_INVALID_PARAMETER;
    if (!(buffer = heap_alloc( secret_len ))) return STATUS_NO_MEMORY;
    memcpy( buffer, secret, secret_len );

    key->alg_id     = alg->id;
    key->handle     = 0;        /* initialized on first use */
    key->secret     = buffer;
    key->secret_len = secret_len;

    return STATUS_SUCCESS;
}

static gnutls_cipher_algorithm_t get_gnutls_cipher( const struct key *key )
{
    switch (key->alg_id)
    {
    case ALG_ID_AES:
        FIXME( "handle block size and chaining mode\n" );
        return GNUTLS_CIPHER_AES_128_CBC;

    default:
        FIXME( "algorithm %u not supported\n", key->alg_id );
        return GNUTLS_CIPHER_UNKNOWN;
    }
}

static NTSTATUS key_set_params( struct key *key, UCHAR *iv, ULONG iv_len )
{
    gnutls_cipher_algorithm_t cipher;
    gnutls_datum_t secret, vector;
    int ret;

    if (key->handle)
    {
        pgnutls_cipher_deinit( key->handle );
        key->handle = NULL;
    }

    if ((cipher = get_gnutls_cipher( key )) == GNUTLS_CIPHER_UNKNOWN)
        return STATUS_NOT_SUPPORTED;

    secret.data = key->secret;
    secret.size = key->secret_len;
    if (iv)
    {
        vector.data = iv;
        vector.size = iv_len;
    }

    if ((ret = pgnutls_cipher_init( &key->handle, cipher, &secret, iv ? &vector : NULL )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS key_encrypt( struct key *key, const UCHAR *input, ULONG input_len, UCHAR *output,
                             ULONG output_len )
{
    int ret;

    if ((ret = pgnutls_cipher_encrypt2( key->handle, input, input_len, output, output_len )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS key_decrypt( struct key *key, const UCHAR *input, ULONG input_len, UCHAR *output,
                             ULONG output_len  )
{
    int ret;

    if ((ret = pgnutls_cipher_decrypt2( key->handle, input, input_len, output, output_len )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS key_destroy( struct key *key )
{
    if (key->handle) pgnutls_cipher_deinit( key->handle );
    heap_free( key->secret );
    heap_free( key );
    return STATUS_SUCCESS;
}
#elif defined(HAVE_COMMONCRYPTO_COMMONCRYPTOR_H) && MAC_OS_X_VERSION_MAX_ALLOWED >= 1080
static NTSTATUS key_init( struct key *key, struct algorithm *alg, const UCHAR *secret, ULONG secret_len )
{
    UCHAR *buffer;

    switch (alg->id)
    {
    case ALG_ID_AES:
        break;

    default:
        FIXME( "algorithm %u not supported\n", alg->id );
        return STATUS_NOT_SUPPORTED;
    }

    if (!(key->block_size = get_block_size( alg ))) return STATUS_INVALID_PARAMETER;
    if (!(buffer = heap_alloc( secret_len ))) return STATUS_NO_MEMORY;
    memcpy( buffer, secret, secret_len );

    key->alg_id      = alg->id;
    key->ref_encrypt = NULL;        /* initialized on first use */
    key->ref_decrypt = NULL;
    key->secret      = buffer;
    key->secret_len  = secret_len;

    return STATUS_SUCCESS;
}

static NTSTATUS key_set_params( struct key *key, UCHAR *iv, ULONG iv_len )
{
    CCCryptorStatus status;

    if (key->ref_encrypt)
    {
        CCCryptorRelease( key->ref_encrypt );
        key->ref_encrypt = NULL;
    }
    if (key->ref_decrypt)
    {
        CCCryptorRelease( key->ref_decrypt );
        key->ref_decrypt = NULL;
    }

    if ((status = CCCryptorCreateWithMode( kCCEncrypt, kCCModeCBC, kCCAlgorithmAES128, ccNoPadding, iv,
                                           key->secret, key->secret_len, NULL, 0, 0, 0, &key->ref_encrypt )) != kCCSuccess)
    {
        WARN( "CCCryptorCreateWithMode failed %d\n", status );
        return STATUS_INTERNAL_ERROR;
    }
    if ((status = CCCryptorCreateWithMode( kCCDecrypt, kCCModeCBC, kCCAlgorithmAES128, ccNoPadding, iv,
                                           key->secret, key->secret_len, NULL, 0, 0, 0, &key->ref_decrypt )) != kCCSuccess)
    {
        WARN( "CCCryptorCreateWithMode failed %d\n", status );
        CCCryptorRelease( key->ref_encrypt );
        key->ref_encrypt = NULL;
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS key_encrypt( struct key *key, const UCHAR *input, ULONG input_len, UCHAR *output,
                             ULONG output_len  )
{
    CCCryptorStatus status;

    if ((status = CCCryptorUpdate( key->ref_encrypt, input, input_len, output, output_len, NULL  )) != kCCSuccess)
    {
        WARN( "CCCryptorUpdate failed %d\n", status );
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS key_decrypt( struct key *key, const UCHAR *input, ULONG input_len, UCHAR *output,
                             ULONG output_len )
{
    CCCryptorStatus status;

    if ((status = CCCryptorUpdate( key->ref_decrypt, input, input_len, output, output_len, NULL  )) != kCCSuccess)
    {
        WARN( "CCCryptorUpdate failed %d\n", status );
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS key_destroy( struct key *key )
{
    if (key->ref_encrypt) CCCryptorRelease( key->ref_encrypt );
    if (key->ref_decrypt) CCCryptorRelease( key->ref_decrypt );
    heap_free( key->secret );
    heap_free( key );
    return STATUS_SUCCESS;
}
#else
static NTSTATUS key_init( struct key *key, struct algorithm *alg, const UCHAR *secret, ULONG secret_len )
{
    ERR( "support for keys not available at build time\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS key_set_params( struct key *key, UCHAR *iv, ULONG iv_len )
{
    ERR( "support for keys not available at build time\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS key_encrypt( struct key *key, const UCHAR *input, ULONG input_len, UCHAR *output,
                             ULONG output_len  )
{
    ERR( "support for keys not available at build time\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS key_decrypt( struct key *key, const UCHAR *input, ULONG input_len, UCHAR *output,
                             ULONG output_len )
{
    ERR( "support for keys not available at build time\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS key_destroy( struct key *key )
{
    ERR( "support for keys not available at build time\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS key_export( struct key *key, const WCHAR *type, UCHAR *output, ULONG output_len, ULONG *size )
{
    ERR( "support for keys not available at build time\n" );
    return STATUS_NOT_IMPLEMENTED;
}
#endif

NTSTATUS WINAPI BCryptGenerateSymmetricKey( BCRYPT_ALG_HANDLE algorithm, BCRYPT_KEY_HANDLE *handle,
                                            UCHAR *object, ULONG object_len, UCHAR *secret, ULONG secret_len,
                                            ULONG flags )
{
    struct algorithm *alg = algorithm;
    struct key *key;
    NTSTATUS status;

    TRACE( "%p, %p, %p, %u, %p, %u, %08x\n", algorithm, handle, object, object_len, secret, secret_len, flags );

    if (!alg || alg->hdr.magic != MAGIC_ALG) return STATUS_INVALID_HANDLE;
    if (object) FIXME( "ignoring object buffer\n" );

    if (!(key = heap_alloc( sizeof(*key) ))) return STATUS_NO_MEMORY;
    key->hdr.magic = MAGIC_KEY;

    if ((status = key_init( key, alg, secret, secret_len )))
    {
        heap_free( key );
        return status;
    }

    *handle = key;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptImportKey(BCRYPT_ALG_HANDLE algorithm, BCRYPT_KEY_HANDLE decrypt_key, LPCWSTR type,
                                BCRYPT_KEY_HANDLE *key, PUCHAR object, ULONG object_len, PUCHAR input,
                                ULONG input_len, ULONG flags)
{
    struct algorithm *alg = algorithm;

    TRACE("%p, %p, %s, %p, %p, %u, %p, %u, %u\n", algorithm, decrypt_key, debugstr_w(type), key, object,
          object_len, input, input_len, flags);

    if (!alg || alg->hdr.magic != MAGIC_ALG) return STATUS_INVALID_HANDLE;
    if (!key || !type || !input) return STATUS_INVALID_PARAMETER;

    if (decrypt_key)
    {
        FIXME("Decrypting of key not yet supported\n");
        return STATUS_NO_MEMORY;
    }

    if (!strcmpW(type, BCRYPT_KEY_DATA_BLOB))
    {
        BCRYPT_KEY_DATA_BLOB_HEADER *key_header = (BCRYPT_KEY_DATA_BLOB_HEADER*)input;

        if (input_len < sizeof(BCRYPT_KEY_DATA_BLOB_HEADER))
            return STATUS_BUFFER_TOO_SMALL;

        if (key_header->dwMagic != BCRYPT_KEY_DATA_BLOB_MAGIC)
            return STATUS_INVALID_PARAMETER;

        if (key_header->dwVersion != BCRYPT_KEY_DATA_BLOB_VERSION1)
        {
            FIXME("Unknown key data blob version: %d\n", key_header->dwVersion);
            return STATUS_INVALID_PARAMETER;
        }

        if (key_header->cbKeyData + sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) > input_len)
            return STATUS_INVALID_PARAMETER;

        return BCryptGenerateSymmetricKey(algorithm, key, object, object_len, (UCHAR*)&key_header[1], key_header->cbKeyData, 0);
    }

    FIXME("Unsupported key type: %s\n", debugstr_w(type));
    return STATUS_INVALID_PARAMETER;
}

NTSTATUS WINAPI BCryptExportKey(BCRYPT_KEY_HANDLE export_key, BCRYPT_KEY_HANDLE encrypt_key, LPCWSTR type,
                                PUCHAR output, ULONG output_len, ULONG *size, ULONG flags)
{
    struct key *key = export_key;

    TRACE("%p, %p, %s, %p, %u, %p, %u\n", key, encrypt_key, debugstr_w(type), output, output_len, size, flags);

    if (!key || key->hdr.magic != MAGIC_KEY) return STATUS_INVALID_HANDLE;
    if (!output || !output_len || !size) return STATUS_INVALID_PARAMETER;

    if (encrypt_key)
    {
        FIXME("Encryption of key not yet supported\n");
        return STATUS_NO_MEMORY;
    }

    return key_export( key, type, output, output_len, size );
}

NTSTATUS WINAPI BCryptDestroyKey( BCRYPT_KEY_HANDLE handle )
{
    struct key *key = handle;

    TRACE( "%p\n", handle );

    if (!key || key->hdr.magic != MAGIC_KEY) return STATUS_INVALID_HANDLE;
    return key_destroy( key );
}

NTSTATUS WINAPI BCryptEncrypt( BCRYPT_KEY_HANDLE handle, UCHAR *input, ULONG input_len,
                               void *padding, UCHAR *iv, ULONG iv_len, UCHAR *output,
                               ULONG output_len, ULONG *ret_len, ULONG flags )
{
    struct key *key = handle;
    ULONG bytes_left = input_len;
    UCHAR *buf, *src, *dst;
    NTSTATUS status;

    TRACE( "%p, %p, %u, %p, %p, %u, %p, %u, %p, %08x\n", handle, input, input_len,
           padding, iv, iv_len, output, output_len, ret_len, flags );

    if (!key || key->hdr.magic != MAGIC_KEY) return STATUS_INVALID_HANDLE;
    if (padding)
    {
        FIXME( "padding info not implemented\n" );
        return STATUS_NOT_IMPLEMENTED;
    }
    if (flags & ~BCRYPT_BLOCK_PADDING)
    {
        FIXME( "flags %08x not implemented\n", flags );
        return STATUS_NOT_IMPLEMENTED;
    }

    if ((status = key_set_params( key, iv, iv_len ))) return status;

    *ret_len = input_len;

    if (flags & BCRYPT_BLOCK_PADDING)
        *ret_len = (input_len + key->block_size) & ~(key->block_size - 1);
    else if (input_len & (key->block_size - 1))
        return STATUS_INVALID_BUFFER_SIZE;

    if (!output) return STATUS_SUCCESS;
    if (output_len < *ret_len) return STATUS_BUFFER_TOO_SMALL;

    src = input;
    dst = output;
    while (bytes_left >= key->block_size)
    {
        if ((status = key_encrypt( key, src, key->block_size, dst, key->block_size ))) return status;
        bytes_left -= key->block_size;
        src += key->block_size;
        dst += key->block_size;
    }

    if (flags & BCRYPT_BLOCK_PADDING)
    {
        if (!(buf = heap_alloc( key->block_size ))) return STATUS_NO_MEMORY;
        memcpy( buf, src, bytes_left );
        memset( buf + bytes_left, key->block_size - bytes_left, key->block_size - bytes_left );
        status = key_encrypt( key, buf, key->block_size, dst, key->block_size );
        heap_free( buf );
    }

    return status;
}

NTSTATUS WINAPI BCryptDecrypt( BCRYPT_KEY_HANDLE handle, UCHAR *input, ULONG input_len,
                               void *padding, UCHAR *iv, ULONG iv_len, UCHAR *output,
                               ULONG output_len, ULONG *ret_len, ULONG flags )
{
    struct key *key = handle;
    ULONG bytes_left = input_len;
    UCHAR *buf, *src, *dst;
    NTSTATUS status;

    TRACE( "%p, %p, %u, %p, %p, %u, %p, %u, %p, %08x\n", handle, input, input_len,
           padding, iv, iv_len, output, output_len, ret_len, flags );

    if (!key || key->hdr.magic != MAGIC_KEY) return STATUS_INVALID_HANDLE;
    if (padding)
    {
        FIXME( "padding info not implemented\n" );
        return STATUS_NOT_IMPLEMENTED;
    }
    if (flags & ~BCRYPT_BLOCK_PADDING)
    {
        FIXME( "flags %08x not supported\n", flags );
        return STATUS_NOT_IMPLEMENTED;
    }

    if ((status = key_set_params( key, iv, iv_len ))) return status;

    *ret_len = input_len;

    if (input_len & (key->block_size - 1)) return STATUS_INVALID_BUFFER_SIZE;
    if (!output) return STATUS_SUCCESS;
    if (flags & BCRYPT_BLOCK_PADDING)
    {
        if (output_len + key->block_size < *ret_len) return STATUS_BUFFER_TOO_SMALL;
        if (input_len < key->block_size) return STATUS_BUFFER_TOO_SMALL;
        bytes_left -= key->block_size;
    }
    else if (output_len < *ret_len)
        return STATUS_BUFFER_TOO_SMALL;

    src = input;
    dst = output;
    while (bytes_left >= key->block_size)
    {
        if ((status = key_decrypt( key, src, key->block_size, dst, key->block_size ))) return status;
        bytes_left -= key->block_size;
        src += key->block_size;
        dst += key->block_size;
    }

    if (flags & BCRYPT_BLOCK_PADDING)
    {
        if (!(buf = heap_alloc( key->block_size ))) return STATUS_NO_MEMORY;
        status = key_decrypt( key, src, key->block_size, buf, key->block_size );
        if (!status && buf[ key->block_size - 1 ] <= key->block_size)
        {
            *ret_len -= buf[ key->block_size - 1 ];
            if (output_len < *ret_len) status = STATUS_BUFFER_TOO_SMALL;
            else memcpy( dst, buf, key->block_size - buf[ key->block_size - 1 ] );
        }
        else
            status = STATUS_UNSUCCESSFUL; /* FIXME: invalid padding */
        heap_free( buf );
    }

    return status;
}

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        instance = hinst;
        DisableThreadLibraryCalls( hinst );
#if defined(HAVE_GNUTLS_CIPHER_INIT) && !defined(HAVE_COMMONCRYPTO_COMMONCRYPTOR_H)
        gnutls_initialize();
#endif
        break;

    case DLL_PROCESS_DETACH:
        if (reserved) break;
#if defined(HAVE_GNUTLS_CIPHER_INIT) && !defined(HAVE_COMMONCRYPTO_COMMONCRYPTOR_H)
        gnutls_uninitialize();
#endif
        break;
    }
    return TRUE;
}
