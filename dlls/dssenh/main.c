/*
 * Copyright 2008 Maarten Lankhorst
 * Copyright 2020 Hans Leidekker for CodeWeavers
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
 */

#include <stdarg.h>

#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "winreg.h"
#include "objbase.h"
#include "rpcproxy.h"
#include "ntsecapi.h"

#include "symcrypt.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dssenh);

#define MAGIC_DSS1 ('D' | ('S' << 8) | ('S' << 16) | ('1' << 24))
#define MAGIC_DSS2 ('D' | ('S' << 8) | ('S' << 16) | ('2' << 24))

#define MAGIC_KEY (('K' << 24) | ('E' << 16) | ('Y' << 8) | '0')
struct key
{
    DWORD             magic;
    ALG_ID            algid;
    DWORD             flags;
    DWORD             bitlen;
    SYMCRYPT_DLGROUP *group;
    SYMCRYPT_DLKEY   *handle;
};

#define MAGIC_CONTAINER (('C' << 24) | ('O' << 16) | ('N' << 8) | 'T')
struct container
{
    DWORD       magic;
    DWORD       flags;
    DWORD       type;
    struct key *exch_key;
    struct key *sign_key;
    char        name[MAX_PATH];
    DWORD       alg_idx;
};

#define MAGIC_HASH (('H' << 24) | ('A' << 16) | ('S' << 8) | 'H')
struct hash
{
    DWORD                magic;
    const SYMCRYPT_HASH *desc;
    SYMCRYPT_HASH_STATE  state;
    DWORD                len;
    UCHAR                value[64];
    BOOL                 finished;
};

static const char dss_path_fmt[] = "Software\\Wine\\Crypto\\DSS\\%s";

#define S(s) sizeof(s), s
static const PROV_ENUMALGS_EX supported_base_algs[] =
{
    { CALG_SHA1, 160, 160, 160, CRYPT_FLAG_SIGNING, S("SHA-1"), S("Secure Hash Algorithm (SHA-1)") },
    { CALG_MD5, 128, 128, 128, 0, S("MD5"), S("Message Digest 5 (MD5)") },
    { CALG_DSS_SIGN, 1024, 512, 1024, CRYPT_FLAG_SIGNING, S("DSA_SIGN"), S("Digital Signature Algorithm") }
};
#undef S

SYMCRYPT_ENVIRONMENT_DEFS( WindowsUsermodeWin8_1nLater );

void * SYMCRYPT_CALL SymCryptCallbackAlloc( SIZE_T size )
{
    return malloc( size );
}

void SYMCRYPT_CALL SymCryptCallbackFree( void *ptr )
{
    free( ptr );
}

SYMCRYPT_ERROR SYMCRYPT_CALL SymCryptCallbackRandom( BYTE *buf, SIZE_T size )
{
    return RtlGenRandom( buf, size ) ? SYMCRYPT_NO_ERROR : SYMCRYPT_EXTERNAL_FAILURE;
}

void SYMCRYPT_CALL SymCryptDsaSelftest( void )
{
}

void SYMCRYPT_CALL SymCryptDhSecretAgreementSelftest( void )
{
}

SYMCRYPT_ERROR SYMCRYPT_CALL SymCryptDsaPct( const SYMCRYPT_DLKEY *key )
{
    return SYMCRYPT_NO_ERROR;
}

UINT32 g_SymCryptFipsSelftestsPerformed;

BOOL WINAPI DllMain( HINSTANCE instance, DWORD reason, void *reserved )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( instance );
        SymCryptInit();
        break;
    }
    return TRUE;
}

static BOOL create_container_regkey( struct container *container, REGSAM sam, HKEY *hkey )
{
    char path[sizeof(dss_path_fmt) + MAX_PATH];
    HKEY rootkey;

    sprintf( path, dss_path_fmt, container->name );

    if (container->flags & CRYPT_MACHINE_KEYSET)
        rootkey = HKEY_LOCAL_MACHINE;
    else
        rootkey = HKEY_CURRENT_USER;

    /* @@ Wine registry key: HKLM\Software\Wine\Crypto\DSS */
    /* @@ Wine registry key: HKCU\Software\Wine\Crypto\DSS */
    return !RegCreateKeyExA( rootkey, path, 0, NULL, REG_OPTION_NON_VOLATILE, sam, NULL, hkey, NULL );
}

static struct container *create_key_container( const char *name, DWORD flags, DWORD type )
{
    struct container *ret;

    if (!(ret = calloc( 1, sizeof(*ret) ))) return NULL;
    ret->magic = MAGIC_CONTAINER;
    ret->flags = flags;
    ret->type = type;
    if (name) strcpy( ret->name, name );
    ret->alg_idx = 0;

    if (!(flags & CRYPT_VERIFYCONTEXT))
    {
        HKEY hkey;
        if (create_container_regkey( ret, KEY_WRITE, &hkey )) RegCloseKey( hkey );
    }
    return ret;
}

static BOOL open_container_regkey( const char *name, DWORD flags, REGSAM access, HKEY *hkey )
{
    char path[sizeof(dss_path_fmt) + MAX_PATH];
    HKEY rootkey;

    sprintf( path, dss_path_fmt, name );

    if (flags & CRYPT_MACHINE_KEYSET)
        rootkey = HKEY_LOCAL_MACHINE;
    else
        rootkey = HKEY_CURRENT_USER;

    /* @@ Wine registry key: HKLM\Software\Wine\Crypto\DSS */
    /* @@ Wine registry key: HKCU\Software\Wine\Crypto\DSS */
    return !RegOpenKeyExA( rootkey, path, 0, access, hkey );
}

static const WCHAR *map_keyspec_to_keypair_name( DWORD keyspec, ALG_ID *algid )
{
    const WCHAR *name;

    switch (keyspec)
    {
    case AT_KEYEXCHANGE:
        name = L"KeyExchangeKeyPair";
        if (algid) *algid = CALG_DH_SF;
        break;
    case AT_SIGNATURE:
        name = L"SignatureKeyPair";
        if (algid) *algid = CALG_DSS_SIGN;
        break;
    default:
        ERR( "invalid key spec %lu\n", keyspec );
        return NULL;
    }
    return name;
}

static struct key *create_key( ALG_ID algid, DWORD bitlen, DWORD flags )
{
    struct key *key;

    switch (algid)
    {
    case CALG_DSS_SIGN:
    case CALG_DH_SF:
        break;

    default:
        FIXME( "unhandled algorithm %08x\n", algid );
        SetLastError( NTE_BAD_ALGID );
        return NULL;
    }

    if (!(key = calloc( 1, sizeof(*key) ))) return NULL;

    key->magic  = MAGIC_KEY;
    key->algid  = algid;
    key->bitlen = bitlen;
    key->flags  = flags;
    return key;
}

static void destroy_key( struct key *key )
{
    if (!key) return;
    if (key->handle) SymCryptDlkeyFree( key->handle );
    if (key->group) SymCryptDlgroupFree( key->group );
    /* Ensure compiler doesn't optimize out the assignment with 0. */
    SecureZeroMemory( &key->magic, sizeof(key->magic) );
    free( key );
}

static struct key *import_key( ALG_ID algid, const BYTE *input, DWORD input_len, DWORD flags )
{
    const BLOBHEADER *hdr = (const BLOBHEADER *)input;
    const DSSPUBKEY *dsskey = (const DSSPUBKEY *)(hdr + 1);
    UINT32 priv_size = 0, pub_size = 0, set_flags = SYMCRYPT_FLAG_KEY_NO_FIPS | SYMCRYPT_FLAG_DLKEY_DSA;
    const UCHAR *prime_p, *prime_q, *generator, *pub = NULL, *priv = NULL, *seed, *count;
    ULONG size, key_size;
    struct key *key;

    if (input_len < sizeof(*hdr)) return NULL;
    if (hdr->bType != PRIVATEKEYBLOB && hdr->bType != PUBLICKEYBLOB) return NULL;
    if (hdr->bVersion != 2 || (hdr->aiKeyAlg != CALG_DSS_SIGN && hdr->aiKeyAlg != CALG_DH_SF)) return NULL;

    size = sizeof(*hdr) + sizeof(*dsskey);
    if (input_len < size || dsskey->bitlen & 0xf) return NULL;

    key_size = dsskey->bitlen / 8;
    size += key_size * 2 /* p and g */+ 20 /* q */ + 4 /* count */ + 20 /* seed */;
    size += (hdr->bType == PRIVATEKEYBLOB ? 20 /* x */ : key_size /* y */);
    if (input_len != size) return NULL;

    if (!(key = create_key( algid, dsskey->bitlen, flags ))) return NULL;
    if (!(key->group = SymCryptDlgroupAllocate( dsskey->bitlen, 0 )) ||
        !(key->handle = SymCryptDlkeyAllocate( key->group )))
    {
        destroy_key( key );
        return NULL;
    }

    prime_p = (const UCHAR *)(dsskey + 1);
    prime_q = prime_p + key_size;
    generator = prime_q + 20;
    count = generator + key_size + (hdr->bType == PRIVATEKEYBLOB ? 20 : key_size);
    seed = count + 4;

    if (SymCryptDlgroupSetValue( prime_p, key_size, prime_q, 20, generator, key_size, SYMCRYPT_NUMBER_FORMAT_LSB_FIRST,
                                 NULL, seed, 20, *(UINT32 *)count, SYMCRYPT_DLGROUP_FIPS_NONE, key->group ))
    {
        destroy_key( key );
        return NULL;
    }

    if (hdr->bType == PRIVATEKEYBLOB)
    {
        priv = generator + key_size;
        priv_size = 20;
    }
    else
    {
        pub = generator + key_size;
        pub_size = key_size;
    }

    if (SymCryptDlkeySetValue( priv, priv_size, pub, pub_size, SYMCRYPT_NUMBER_FORMAT_LSB_FIRST, set_flags, key->handle ))
    {
        destroy_key( key );
        return NULL;
    }

    return key;
}

static struct key *read_key( HKEY hkey, DWORD keyspec, DWORD flags )
{
    const WCHAR *value;
    DWORD type, len;
    BYTE *data;
    DATA_BLOB blob_in, blob_out;
    ALG_ID algid;
    struct key *key = NULL;

    if (!(value = map_keyspec_to_keypair_name( keyspec, &algid ))) return NULL;
    if (RegQueryValueExW( hkey, value, 0, &type, NULL, &len )) return NULL;
    if (!(data = malloc( len ))) return NULL;

    if (!RegQueryValueExW( hkey, value, 0, &type, data, &len ))
    {
        blob_in.pbData = data;
        blob_in.cbData = len;
        if (CryptUnprotectData( &blob_in, NULL, NULL, NULL, NULL, flags, &blob_out ))
        {
            key = import_key( algid, blob_out.pbData, blob_out.cbData, flags );
            LocalFree( blob_out.pbData );
        }
    }

    free( data );
    return key;
}

static void destroy_container( struct container *container )
{
    if (!container) return;
    destroy_key( container->exch_key );
    destroy_key( container->sign_key );
    /* Ensure compiler doesn't optimize out the assignment with 0. */
    SecureZeroMemory( &container->magic, sizeof(container->magic) );
    free( container );
}

static struct container *read_key_container( const char *name, DWORD flags, DWORD type )
{
    DWORD protect_flags = (flags & CRYPT_MACHINE_KEYSET) ? CRYPTPROTECT_LOCAL_MACHINE : 0;
    struct container *ret;
    HKEY hkey;

    if (!open_container_regkey( name, flags, KEY_READ, &hkey )) return NULL;

    if ((ret = create_key_container( name, flags, type )))
    {
        ret->exch_key = read_key( hkey, AT_KEYEXCHANGE, protect_flags );
        ret->sign_key = read_key( hkey, AT_SIGNATURE, protect_flags );
    }

    RegCloseKey( hkey );
    return ret;
}

static void delete_key_container( const char *name, DWORD flags )
{
    char path[sizeof(dss_path_fmt) + MAX_PATH];
    HKEY rootkey;

    sprintf( path, dss_path_fmt, name );

    if (flags & CRYPT_MACHINE_KEYSET)
        rootkey = HKEY_LOCAL_MACHINE;
    else
        rootkey = HKEY_CURRENT_USER;

    /* @@ Wine registry key: HKLM\Software\Wine\Crypto\DSS */
    /* @@ Wine registry key: HKCU\Software\Wine\Crypto\DSS */
    RegDeleteKeyExA( rootkey, path, 0, 0 );
}

BOOL WINAPI CPAcquireContext( HCRYPTPROV *ret_prov, LPSTR container, DWORD flags, PVTableProvStruc vtable )
{
    struct container *ret;
    char name[MAX_PATH];

    TRACE( "%p, %s, %08lx, %p\n", ret_prov, debugstr_a(container), flags, vtable );

    if (container && *container)
    {
        if (lstrlenA( container ) >= sizeof(name)) return FALSE;
        lstrcpyA( name, container );
    }
    else
    {
        DWORD len = sizeof(name);
        if (!GetUserNameA( name, &len )) return FALSE;
    }

    switch (flags)
    {
    case 0:
    case 0 | CRYPT_MACHINE_KEYSET:
        if (!(ret = read_key_container( name, flags, vtable->dwProvType )))
            SetLastError( NTE_BAD_KEYSET );
        break;

    case CRYPT_NEWKEYSET:
    case CRYPT_NEWKEYSET | CRYPT_MACHINE_KEYSET:
        if ((ret = read_key_container( name, flags, vtable->dwProvType )))
        {
            free( ret );
            SetLastError( NTE_EXISTS );
            return FALSE;
        }
        ret = create_key_container( name, flags, vtable->dwProvType );
        break;

    case CRYPT_VERIFYCONTEXT:
    case CRYPT_VERIFYCONTEXT | CRYPT_MACHINE_KEYSET:
    case CRYPT_VERIFYCONTEXT | CRYPT_NEWKEYSET:
        ret = create_key_container( "", flags, vtable->dwProvType );
        break;

    case CRYPT_DELETEKEYSET:
    case CRYPT_DELETEKEYSET | CRYPT_MACHINE_KEYSET:
        delete_key_container( name, flags );
        *ret_prov = 0;
        return TRUE;

    default:
        FIXME( "unsupported flags %08lx\n", flags );
        return FALSE;
    }

    if (!ret) return FALSE;
    *ret_prov = (HCRYPTPROV)ret;
    return TRUE;
}

BOOL WINAPI CPReleaseContext( HCRYPTPROV hprov, DWORD flags )
{
    struct container *container = (struct container *)hprov;

    TRACE( "%IX, %08lx\n", hprov, flags );

    if (!container || container->magic != MAGIC_CONTAINER)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    destroy_container( container );
    return TRUE;
}

static BOOL copy_param( void *dst, DWORD *dst_size, const void *src, DWORD src_size )
{
    if (!dst_size)
    {
        SetLastError( ERROR_MORE_DATA );
        return FALSE;
    }

    if (!dst)
    {
        *dst_size = src_size;
        return TRUE;
    }

    if (*dst_size < src_size)
    {
        SetLastError( ERROR_MORE_DATA );
        return FALSE;
    }

    *dst_size = src_size;
    memcpy( dst, src, src_size );
    return TRUE;
}

BOOL WINAPI CPGetProvParam( HCRYPTPROV hprov, DWORD param, BYTE *data, DWORD *len, DWORD flags )
{
    struct container *container = (struct container *)hprov;

    TRACE( "%IX, %lu, %p, %p, %08lx\n", hprov, param, data, len, flags );

    if (!container || container->magic != MAGIC_CONTAINER)
    {
        SetLastError( NTE_BAD_UID );
        return FALSE;
    }
    if (!len)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    switch (param)
    {
    case PP_NAME:
    {
        const char *name;

        switch (container->type)
        {
        default:
        case PROV_DSS:
            name = MS_DEF_DSS_PROV_A;
            break;
        case PROV_DSS_DH:
            name = MS_ENH_DSS_DH_PROV_A;
            break;
        case PROV_DH_SCHANNEL:
            name = MS_DEF_DH_SCHANNEL_PROV_A;
            break;
        }
        return copy_param( data, len, name, strlen(name) + 1 );
    }

    case PP_ENUMALGS:
    case PP_ENUMALGS_EX:
    {
        if (flags & CRYPT_FIRST)
            container->alg_idx = 0;
        else
            container->alg_idx++;

        if (container->alg_idx >= ARRAY_SIZE(supported_base_algs))
        {
            SetLastError( ERROR_NO_MORE_ITEMS );
            return FALSE;
        }

        if (param == PP_ENUMALGS)
        {
            PROV_ENUMALGS alg;

            alg.aiAlgid   = supported_base_algs[container->alg_idx].aiAlgid;
            alg.dwBitLen  = supported_base_algs[container->alg_idx].dwDefaultLen;
            alg.dwNameLen = supported_base_algs[container->alg_idx].dwNameLen;
            strcpy( alg.szName, supported_base_algs[container->alg_idx].szName );
            return copy_param( data, len, &alg, sizeof(alg) );
        }
        else
            return copy_param( data, len, &supported_base_algs[container->alg_idx], sizeof(PROV_ENUMALGS_EX) );
    }

    default:
        FIXME( "param %lu is not supported\n", param );
        SetLastError( NTE_BAD_TYPE );
        return FALSE;
    }
}

static BOOL export_key( const struct key *key, DWORD type, BYTE *output, DWORD output_len, DWORD *ret_len )
{
    BLOBHEADER *hdr = (BLOBHEADER *)output;
    DSSPUBKEY *dsskey = (DSSPUBKEY *)(hdr + 1);
    UINT32 key_size = key->bitlen / 8, pub_size = 0, priv_size = 0;
    UCHAR *prime_p, *prime_q, *generator, *pub = NULL, *priv = NULL, *seed, *count;
    ULONG size;

    size = sizeof(*hdr) + sizeof(*dsskey) + key_size * 2 /* p and g */ + 20 /* q */ + 4 /* count */ + 20 /* seed */;
    size += (type == PRIVATEKEYBLOB) ? 20 /* x */ : key_size; /* y */
    if (output_len < size || !output)
    {
        *ret_len = size;
        if (output) return FALSE;
        return TRUE;
    }

    hdr->bType    = type;
    hdr->bVersion = 2;
    hdr->reserved = 0;
    hdr->aiKeyAlg = CALG_DSS_SIGN;

    dsskey->magic  = (type == PRIVATEKEYBLOB) ? MAGIC_DSS2 : MAGIC_DSS1;
    dsskey->bitlen = key->bitlen;

    prime_p = (UCHAR *)(dsskey + 1);
    prime_q = prime_p + key_size;
    generator = prime_q + 20;
    count = generator + key_size + (type == PRIVATEKEYBLOB ? 20 : key_size);
    seed = count + 4;

    if (SymCryptDlgroupGetValue( key->group, prime_p, key_size, prime_q, 20, generator, key_size,
                                 SYMCRYPT_NUMBER_FORMAT_LSB_FIRST, NULL, seed, 20, (UINT32 *)count )) return FALSE;

    if (type == PRIVATEKEYBLOB)
    {
        priv = generator + key_size;
        priv_size = 20;
    }
    else
    {
        pub = generator + key_size;
        pub_size = key_size;
    }

    if (SymCryptDlkeyGetValue( key->handle, priv, priv_size, pub, pub_size, SYMCRYPT_NUMBER_FORMAT_LSB_FIRST, 0 ))
        return FALSE;

    *ret_len = size;
    return TRUE;
}

static BOOL store_key_pair( struct key *key, HKEY hkey, DWORD keyspec, DWORD flags )
{
    const WCHAR *value;
    DATA_BLOB blob_in, blob_out;
    DWORD len;
    BYTE *data;
    BOOL ret = TRUE;

    if (!key) return TRUE;
    if (!(value = map_keyspec_to_keypair_name( keyspec, NULL ))) return FALSE;

    if (!export_key( key, PRIVATEKEYBLOB, NULL, 0, &len )) return FALSE;
    if (!(data = malloc( len ))) return FALSE;
    if (export_key( key, PRIVATEKEYBLOB, data, len, &len ))
    {
        blob_in.pbData = data;
        blob_in.cbData = len;
        if ((ret = CryptProtectData( &blob_in, NULL, NULL, NULL, NULL, flags, &blob_out )))
        {
            ret = !RegSetValueExW( hkey, value, 0, REG_BINARY, blob_out.pbData, blob_out.cbData );
            LocalFree( blob_out.pbData );
        }
    }

    free( data );
    return ret;
}

static BOOL store_key_container_keys( struct container *container )
{
    HKEY hkey;
    DWORD flags = (container->flags & CRYPT_MACHINE_KEYSET) ? CRYPTPROTECT_LOCAL_MACHINE : 0;
    BOOL ret;

    if (!create_container_regkey( container, KEY_WRITE, &hkey )) return FALSE;

    ret = store_key_pair( container->exch_key, hkey, AT_KEYEXCHANGE, flags );
    if (ret) ret = store_key_pair( container->sign_key, hkey, AT_SIGNATURE, flags );
    RegCloseKey( hkey );
    return ret;
}

static struct key *duplicate_key( const struct key *src )
{
    struct key *dst;
    DWORD len, type = SymCryptDlkeyHasPrivateKey( src->handle ) ? PRIVATEKEYBLOB : PUBLICKEYBLOB;
    BYTE *data;

    if (!export_key( src, type, NULL, 0, &len )) return NULL;
    if (!(data = malloc( len ))) return NULL;
    if (!export_key( src, type, data, len, &len ) || !(dst = import_key( src->algid, data, len, src->flags )))
    {
        free( data );
        return NULL;
    }

    free( data );
    return dst;
}

static BOOL generate_key( struct container *container, ALG_ID algid, DWORD bitlen, DWORD flags, HCRYPTKEY *ret_key )
{
    UINT32 generate_flags = SYMCRYPT_FLAG_KEY_NO_FIPS | SYMCRYPT_FLAG_DLKEY_DSA;
    struct key *key, *sign_key, *exch_key;

    if (algid == AT_SIGNATURE) algid = CALG_DSS_SIGN;
    else if (algid == AT_KEYEXCHANGE) algid = CALG_DH_SF;

    if (!(key = create_key( algid, bitlen, flags ))) return FALSE;
    if (!(key->group = SymCryptDlgroupAllocate( bitlen, 0 ))) goto error;
    if (SymCryptDlgroupGenerate( NULL, SYMCRYPT_DLGROUP_FIPS_186_2, key->group )) goto error;
    if (!(key->handle = SymCryptDlkeyAllocate( key->group ))) goto error;
    if (SymCryptDlkeyGenerate( generate_flags, key->handle )) goto error;

    switch (algid)
    {
    case CALG_DSS_SIGN:
        if (!(sign_key = duplicate_key( key )))
        {
            destroy_key( key );
            return FALSE;
        }
        destroy_key( container->sign_key );
        container->sign_key = sign_key;
        break;

    case CALG_DH_SF:
        if (!(exch_key = duplicate_key( key )))
        {
            destroy_key( key );
            return FALSE;
        }
        destroy_key( container->exch_key );
        container->exch_key = exch_key;
        break;

    default:
        FIXME( "unhandled algorithm %08x\n", algid );
        SetLastError( NTE_BAD_ALGID );
        return FALSE;
    }

    if (!store_key_container_keys( container )) return FALSE;

    *ret_key = (HCRYPTKEY)key;
    return TRUE;

error:
    SetLastError( NTE_BAD_FLAGS );
    destroy_key( key );
    return FALSE;
}

BOOL WINAPI CPGenKey( HCRYPTPROV hprov, ALG_ID algid, DWORD flags, HCRYPTKEY *ret_key )
{
    static const unsigned int supported_key_lengths[] = { 512, 768, 1024 };
    struct container *container = (struct container *)hprov;
    ULONG i, bitlen = HIWORD(flags) ? HIWORD(flags) : 1024;

    TRACE( "%IX, %08x, %08lx, %p\n", hprov, algid, flags, ret_key );

    if (!container || container->magic != MAGIC_CONTAINER || bitlen % 2 || !ret_key)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    for (i = 0; i < ARRAY_SIZE(supported_key_lengths); i++)
    {
        if (bitlen == supported_key_lengths[i]) break;
    }
    if (i >= ARRAY_SIZE(supported_key_lengths))
    {
        SetLastError( NTE_BAD_FLAGS );
        return FALSE;
    }

    return generate_key( container, algid, bitlen, LOWORD(flags), ret_key );
}

BOOL WINAPI CPDestroyKey( HCRYPTPROV hprov, HCRYPTKEY hkey )
{
    struct key *key = (struct key *)hkey;

    TRACE( "%IX, %IX\n", hprov, hkey );

    if (!key || key->magic != MAGIC_KEY)
    {
        SetLastError( NTE_BAD_KEY );
        return FALSE;
    }

    destroy_key( key );
    return TRUE;
}

static BOOL import_key_dss2( struct container *container, ALG_ID algid, const BYTE *data, DWORD len, DWORD flags,
                             HCRYPTKEY *ret_key )
{
    struct key *key, *exch_key, *sign_key;

    if (!(key = import_key( algid, data, len, flags ))) return FALSE;

    if (SymCryptDlkeyHasPrivateKey( key->handle ))
    {
        switch (algid)
        {
        case CALG_DH_SF:
            if (!(exch_key = duplicate_key( key )))
            {
                destroy_key( key );
                return FALSE;
            }
            destroy_key( container->exch_key );
            container->exch_key = exch_key;
            break;

        case CALG_DSS_SIGN:
            if (!(sign_key = duplicate_key( key )))
            {
                destroy_key( key );
                return FALSE;
            }
            destroy_key( container->sign_key );
            container->sign_key = sign_key;
            break;

        default:
            FIXME( "unhandled key algorithm %u\n", algid );
            destroy_key( key );
            return FALSE;
        }

        if (!store_key_container_keys( container ))
        {
            destroy_key( key );
            return FALSE;
        }
    }

    *ret_key = (HCRYPTKEY)key;
    return TRUE;
}

BOOL WINAPI CPImportKey( HCRYPTPROV hprov, const BYTE *data, DWORD len, HCRYPTKEY hpubkey, DWORD flags,
                         HCRYPTKEY *ret_key )
{
    struct container *container = (struct container *)hprov;
    const BLOBHEADER *hdr;
    BOOL ret;

    TRACE( "%IX, %p, %lu, %IX, %08lx, %p\n", hprov, data, len, hpubkey, flags, ret_key );

    if (!container || container->magic != MAGIC_CONTAINER || !data || len < sizeof(*hdr) || !ret_key)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    hdr = (const BLOBHEADER *)data;
    if ((hdr->bType != PRIVATEKEYBLOB && hdr->bType != PUBLICKEYBLOB) || hdr->aiKeyAlg != CALG_DSS_SIGN)
    {
        FIXME( "bType %u aiKeyAlg %08x not supported\n", hdr->bType, hdr->aiKeyAlg );
        return FALSE;
    }

    switch (hdr->bVersion)
    {
    case 2:
        ret = import_key_dss2( container, hdr->aiKeyAlg, data, len, flags, ret_key );
        break;

    default:
        FIXME( "version %u not supported\n", hdr->bVersion );
        return FALSE;
    }

    return ret;
}

BOOL WINAPI CPExportKey( HCRYPTPROV hprov, HCRYPTKEY hkey, HCRYPTKEY hexpkey, DWORD type, DWORD flags, BYTE *data,
                         DWORD *len )
{
    struct key *key = (struct key *)hkey;

    TRACE( "%IX, %IX, %IX, %08lx, %08lx, %p, %p\n", hprov, hkey, hexpkey, type, flags, data, len );

    if (!key || key->magic != MAGIC_KEY || (type != PUBLICKEYBLOB && type != PRIVATEKEYBLOB) || !len)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (hexpkey)
    {
        FIXME( "export key not supported\n" );
        return FALSE;
    }
    if (flags)
    {
        FIXME( "flags %08lx not supported\n", flags );
        return FALSE;
    }

    return export_key( key, type, data, *len, len );
}

BOOL WINAPI CPDuplicateKey( HCRYPTPROV hprov, HCRYPTKEY hkey, DWORD *reserved, DWORD flags, HCRYPTKEY *ret_key )
{
    struct key *src = (struct key *)hkey, *dst;

    TRACE( "%IX, %IX, %p, %08lx, %p\n", hprov, hkey, reserved, flags, ret_key );

    if (!src || src->magic != MAGIC_KEY || !ret_key)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (!(dst = duplicate_key( src ))) return FALSE;
    *ret_key = (HCRYPTKEY)dst;
    return TRUE;
}

BOOL WINAPI CPGetUserKey( HCRYPTPROV hprov, DWORD keyspec, HCRYPTKEY *ret_key )
{
    struct container *container = (struct container *)hprov;
    BOOL ret = FALSE;

    TRACE( "%IX, %08lx, %p\n", hprov, keyspec, ret_key );

    if (!container || container->magic != MAGIC_CONTAINER || !ret_key)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    switch (keyspec)
    {
    case AT_KEYEXCHANGE:
        if (!container->exch_key) SetLastError( NTE_NO_KEY );
        else if ((*ret_key = (HCRYPTKEY)duplicate_key( container->exch_key ))) ret = TRUE;
        break;

    case AT_SIGNATURE:
        if (!container->sign_key) SetLastError( NTE_NO_KEY );
        else if ((*ret_key = (HCRYPTKEY)duplicate_key( container->sign_key ))) ret = TRUE;
        break;

    default:
        SetLastError( NTE_NO_KEY );
        return FALSE;
    }

    return ret;
}

BOOL WINAPI CPSetKeyParam( HCRYPTPROV hprov, HCRYPTKEY hkey, DWORD param, BYTE *data, DWORD flags )
{
    FIXME( "%IX, %IX, %lu, %p, %08lx\n", hprov, hkey, param, data, flags );
    SetLastError( ERROR_INVALID_PARAMETER );
    return FALSE;
}

BOOL WINAPI CPGenRandom( HCRYPTPROV hprov, DWORD len, BYTE *buffer )
{
    struct container *container = (struct container *)hprov;

    TRACE( "%IX, %lu, %p\n", hprov, len, buffer );

    if (!container || container->magic != MAGIC_CONTAINER || !buffer)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    return RtlGenRandom( buffer, len );
}

static struct hash *create_hash( ALG_ID algid )
{
    struct hash *hash;
    const SYMCRYPT_HASH *desc;
    DWORD len;

    switch (algid)
    {
    case CALG_MD5:
        desc = SymCryptMd5Algorithm;
        len = 16;
        break;

    case CALG_SHA1:
        desc = SymCryptSha1Algorithm;
        len = 20;
        break;

    default:
        FIXME( "unhandled algorithm %u\n", algid );
        return NULL;
    }

    if (!(hash = calloc( 1, sizeof(*hash) ))) return NULL;

    hash->magic = MAGIC_HASH;
    hash->desc  = desc;
    hash->len   = len;

    SymCryptHashInit( hash->desc, &hash->state );
    return hash;
}

BOOL WINAPI CPCreateHash( HCRYPTPROV hprov, ALG_ID algid, HCRYPTKEY hkey, DWORD flags, HCRYPTHASH *ret_hash )
{
    struct hash *hash;

    TRACE( "%IX, %08x, %IX, %08lx, %p\n", hprov, algid, hkey, flags, ret_hash );

    if (!ret_hash)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    switch (algid)
    {
    case CALG_MD5:
    case CALG_SHA1:
        break;

    default:
        FIXME( "algorithm %u not supported\n", algid );
        SetLastError( NTE_BAD_ALGID );
        return FALSE;
    }

    if (!(hash = create_hash( algid ))) return FALSE;

    *ret_hash = (HCRYPTHASH)hash;
    return TRUE;
}

static void destroy_hash( struct hash *hash )
{
    if (!hash) return;
    /* Ensure compiler doesn't optimize out the assignment with 0. */
    SecureZeroMemory( &hash->magic, sizeof(hash->magic) );
    free( hash );
}

BOOL WINAPI CPDestroyHash( HCRYPTPROV hprov, HCRYPTHASH hhash )
{
    struct hash *hash = (struct hash *)hhash;

    TRACE( "%IX, %IX\n", hprov, hhash);

    if (!hash || hash->magic != MAGIC_HASH)
    {
        SetLastError( NTE_BAD_HASH );
        return FALSE;
    }

    destroy_hash( hash );
    return TRUE;
}

static struct hash *duplicate_hash( const struct hash *src )
{
    struct hash *dst;

    if (!(dst = malloc( sizeof(*dst) ))) return NULL;

    dst->magic    = src->magic;
    dst->desc     = src->desc;
    dst->len      = src->len;
    dst->finished = src->finished;

    SymCryptHashStateCopy( src->desc, &src->state, &dst->state );
    memcpy( dst->value, src->value, sizeof(src->value) );
    return dst;
}

BOOL WINAPI CPDuplicateHash( HCRYPTPROV hprov, HCRYPTHASH hhash, DWORD *reserved, DWORD flags, HCRYPTHASH *ret_hash )
{
    struct hash *src = (struct hash *)hhash, *dst;

    TRACE( "%IX, %IX, %p, %08lx, %p\n", hprov, hhash, reserved, flags, ret_hash );

    if (!src || src->magic != MAGIC_HASH)
    {
        SetLastError( NTE_BAD_HASH );
        return FALSE;
    }

    if (!(dst = duplicate_hash( src ))) return FALSE;
    *ret_hash = (HCRYPTHASH)dst;
    return TRUE;
}

BOOL WINAPI CPHashData( HCRYPTPROV hprov, HCRYPTHASH hhash, const BYTE *data, DWORD len, DWORD flags )
{
    struct hash *hash = (struct hash *)hhash;

    TRACE("%IX, %IX, %p, %lu, %08lx\n", hprov, hhash, data, len, flags );

    if (!hash || hash->magic != MAGIC_HASH)
    {
        SetLastError( NTE_BAD_HASH );
        return FALSE;
    }

    if (hash->finished)
    {
        SetLastError( NTE_BAD_HASH_STATE );
        return FALSE;
    }

    SymCryptHashAppend( hash->desc, &hash->state, data, len );
    return TRUE;
}

BOOL WINAPI CPGetHashParam( HCRYPTPROV hprov, HCRYPTHASH hhash, DWORD param, BYTE *data, DWORD *len, DWORD flags )
{
    struct hash *hash = (struct hash *)hhash;

    TRACE( "%IX, %IX, %08lx, %p, %p, %08lx\n", hprov, hhash, param, data, len, flags );

    if (!hash || hash->magic != MAGIC_HASH)
    {
        SetLastError( NTE_BAD_HASH );
        return FALSE;
    }

    switch (param)
    {
    case HP_HASHSIZE:
        if (sizeof(hash->len) > *len)
        {
            *len = sizeof(hash->len);
            SetLastError( ERROR_MORE_DATA );
            return FALSE;
        }
        *(DWORD *)data = hash->len;
        *len = sizeof(hash->len);
        return TRUE;

    case HP_HASHVAL:
        if (!hash->finished)
        {
            SymCryptHashResult( hash->desc, &hash->state, hash->value, hash->len );
            hash->finished = TRUE;
        }
        if (hash->len > *len)
        {
            *len = hash->len;
            SetLastError( ERROR_MORE_DATA );
            return FALSE;
        }
        if (data) memcpy( data, hash->value, hash->len );
        *len = hash->len;
        return TRUE;

    default:
        SetLastError( NTE_BAD_TYPE );
        return FALSE;
    }
}

BOOL WINAPI CPSetHashParam( HCRYPTPROV hprov, HCRYPTHASH hhash, DWORD param, const BYTE *data, DWORD flags )
{
    struct hash *hash = (struct hash *)hhash;

    TRACE( "%IX, %IX, %08lx, %p, %08lx\n", hprov, hhash, param, data, flags );

    if (!hash || hash->magic != MAGIC_HASH)
    {
        SetLastError( NTE_BAD_HASH );
        return FALSE;
    }

    switch (param)
    {
    case HP_HASHVAL:
        memcpy( hash->value, data, hash->len );
        return TRUE;

    default:
        FIXME( "param %lu not supported\n", param );
        SetLastError( NTE_BAD_TYPE );
        return FALSE;
    }
}

BOOL WINAPI CPDeriveKey( HCRYPTPROV hprov, ALG_ID algid, HCRYPTHASH hhash, DWORD flags, HCRYPTKEY *ret_key )
{
    FIXME( "%p, %p, %08lx, %p\n", (void *)hprov, (void *)hhash, flags, ret_key );
    return FALSE;
}

static DWORD get_signature_length( DWORD algid )
{
    switch (algid)
    {
    case AT_SIGNATURE:
    case CALG_DSS_SIGN: return 40;
    default:
        FIXME( "unhandled algorithm %lu\n", algid );
        return 0;
    }
}

#define MAX_HASH_LEN 20
BOOL WINAPI CPSignHash( HCRYPTPROV hprov, HCRYPTHASH hhash, DWORD keyspec, const WCHAR *desc, DWORD flags, BYTE *sig,
                        DWORD *siglen )
{
    struct container *container = (struct container *)hprov;
    struct hash *hash = (struct hash *)hhash;
    ULONG len;

    TRACE( "%IX, %IX, %lu, %s, %08lx, %p, %p\n", hprov, hhash, keyspec, debugstr_w(desc), flags, sig, siglen );

    if (!container || container->magic != MAGIC_CONTAINER || !container->sign_key || !siglen)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!hash || hash->magic != MAGIC_HASH)
    {
        SetLastError( NTE_BAD_HASH );
        return FALSE;
    }
    if (!(len = get_signature_length( container->sign_key->algid )))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (*siglen < len)
    {
        *siglen = len;
        return TRUE;
    }

    if (!hash->finished)
    {
        SymCryptHashResult( hash->desc, &hash->state, hash->value, hash->len );
        hash->finished = TRUE;
    }

    if (SymCryptDsaSign( container->sign_key->handle, hash->value, hash->len, SYMCRYPT_NUMBER_FORMAT_LSB_FIRST, 0,
                         sig, len )) return FALSE;

    *siglen = len;
    return TRUE;
}

BOOL WINAPI CPVerifySignature( HCRYPTPROV hprov, HCRYPTHASH hhash, const BYTE *sig, DWORD siglen, HCRYPTKEY hpubkey,
                               const WCHAR *desc, DWORD flags )
{
    struct hash *hash = (struct hash *)hhash;
    struct key *key = (struct key *)hpubkey;
    ULONG len;

    TRACE( "%IX, %IX, %p, %lu %IX, %s, %08lx\n", hprov, hhash, sig, siglen, hpubkey, debugstr_w(desc), flags );

    if (!hash || hash->magic != MAGIC_HASH || !key || key->magic != MAGIC_KEY)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!(len = get_signature_length( key->algid )) || len != siglen)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (flags)
    {
        FIXME( "flags %08lx not supported\n", flags );
        return FALSE;
    }

    if (!hash->finished)
    {
        SymCryptHashResult( hash->desc, &hash->state, hash->value, hash->len );
        hash->finished = TRUE;
    }

    if (SymCryptDsaVerify( key->handle, hash->value, hash->len, sig, siglen, SYMCRYPT_NUMBER_FORMAT_LSB_FIRST, 0 ))
        return FALSE;

    return TRUE;
}
