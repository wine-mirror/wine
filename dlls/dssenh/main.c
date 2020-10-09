/*
 * Copyright 2008 Maarten Lankhorst
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

#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "winreg.h"
#include "bcrypt.h"
#include "objbase.h"
#include "rpcproxy.h"

#include "wine/debug.h"
#include "wine/heap.h"

static HINSTANCE instance;

WINE_DEFAULT_DEBUG_CHANNEL(dssenh);

#define MAGIC_KEY (('K' << 24) | ('E' << 16) | ('Y' << 8) | '0')
struct key
{
    DWORD             magic;
    DWORD             algid;
    DWORD             flags;
    BCRYPT_ALG_HANDLE alg_handle;
    BCRYPT_KEY_HANDLE handle;
};

#define MAGIC_CONTAINER (('C' << 24) | ('O' << 16) | ('N' << 8) | 'T')
struct container
{
    DWORD       magic;
    DWORD       flags;
    struct key *exch_key;
    struct key *sign_key;
    char        name[MAX_PATH];
};

static const char dss_path_fmt[] = "Software\\Wine\\Crypto\\DSS\\%s";

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

static struct container *create_key_container( const char *name, DWORD flags )
{
    struct container *ret;

    if (!(ret = heap_alloc_zero( sizeof(*ret) ))) return NULL;
    ret->magic = MAGIC_CONTAINER;
    ret->flags = flags;
    if (name) strcpy( ret->name, name );

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

static const WCHAR *map_keyspec_to_keypair_name( DWORD keyspec )
{
    const WCHAR *name;

    switch (keyspec)
    {
    case AT_KEYEXCHANGE:
        name = L"KeyExchangeKeyPair";
        break;
    case AT_SIGNATURE:
        name = L"SignatureKeyPair";
        break;
    default:
        ERR( "invalid key spec %u\n", keyspec );
        return NULL;
    }
    return name;
}

static struct key *create_key( ALG_ID algid, DWORD flags )
{
    struct key *ret;
    const WCHAR *alg;

    switch (algid)
    {
    case AT_SIGNATURE:
    case CALG_DSS_SIGN:
        alg = BCRYPT_DSA_ALGORITHM;
        break;

    default:
        FIXME( "unhandled algorithm %08x\n", algid );
        return NULL;
    }

    if (!(ret = heap_alloc_zero( sizeof(*ret) ))) return NULL;

    ret->magic = MAGIC_KEY;
    ret->algid = algid;
    ret->flags = flags;
    if (BCryptOpenAlgorithmProvider( &ret->alg_handle, alg, MS_PRIMITIVE_PROVIDER, 0 ))
    {
        heap_free( ret );
        return NULL;
    }
    return ret;
}

static void destroy_key( struct key *key )
{
    if (!key) return;
    BCryptDestroyKey( key->handle );
    BCryptCloseAlgorithmProvider( key->alg_handle, 0 );
    key->magic = 0;
    heap_free( key );
}

static struct key *import_key( DWORD keyspec, BYTE *data, DWORD len )
{
    struct key *ret;

    if (!(ret = create_key( keyspec, 0 ))) return NULL;

    if (BCryptImportKeyPair( ret->alg_handle, NULL, LEGACY_DSA_V2_PRIVATE_BLOB, &ret->handle, data, len, 0 ))
    {
        WARN( "failed to import key\n" );
        destroy_key( ret );
        return NULL;
    }
    return ret;
}

static struct key *read_key( HKEY hkey, DWORD keyspec, DWORD flags )
{
    const WCHAR *value;
    DWORD type, len;
    BYTE *data;
    DATA_BLOB blob_in, blob_out;
    struct key *ret = NULL;

    if (!(value = map_keyspec_to_keypair_name( keyspec ))) return NULL;
    if (RegQueryValueExW( hkey, value, 0, &type, NULL, &len )) return NULL;
    if (!(data = heap_alloc( len ))) return NULL;

    if (!RegQueryValueExW( hkey, value, 0, &type, data, &len ))
    {
        blob_in.pbData = data;
        blob_in.cbData = len;
        if (CryptUnprotectData( &blob_in, NULL, NULL, NULL, NULL, flags, &blob_out ))
        {
            ret = import_key( keyspec, blob_out.pbData, blob_out.cbData );
            LocalFree( blob_out.pbData );
        }
    }

    heap_free( data );
    return ret;
}

static void destroy_container( struct container *container )
{
    if (!container) return;
    destroy_key( container->exch_key );
    destroy_key( container->sign_key );
    container->magic = 0;
    heap_free( container );
}

static struct container *read_key_container( const char *name, DWORD flags )
{
    DWORD protect_flags = (flags & CRYPT_MACHINE_KEYSET) ? CRYPTPROTECT_LOCAL_MACHINE : 0;
    struct container *ret;
    HKEY hkey;

    if (!open_container_regkey( name, flags, KEY_READ, &hkey )) return NULL;

    if ((ret = create_key_container( name, flags )))
    {
        ret->exch_key = read_key( hkey, AT_KEYEXCHANGE, protect_flags );
        ret->sign_key = read_key( hkey, AT_SIGNATURE, protect_flags );
    }

    RegCloseKey( hkey );
    return ret;
}

BOOL WINAPI CPAcquireContext( HCRYPTPROV *ret_prov, LPSTR container, DWORD flags, PVTableProvStruc vtable )
{
    struct container *ret;
    char name[MAX_PATH];

    TRACE( "%p, %s, %08x, %p\n", ret_prov, debugstr_a(container), flags, vtable );

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
        ret = read_key_container( name, flags );
        break;

    case CRYPT_NEWKEYSET:
        if ((ret = read_key_container( name, flags )))
        {
            heap_free( ret );
            SetLastError( NTE_EXISTS );
            return FALSE;
        }
        ret = create_key_container( name, flags );
        break;

    case CRYPT_VERIFYCONTEXT:
        ret = create_key_container( "", flags );
        break;

    default:
        FIXME( "unsupported flags %08x\n", flags );
        return FALSE;
    }

    if (!ret) return FALSE;
    *ret_prov = (HCRYPTPROV)ret;
    return TRUE;
}

BOOL WINAPI CPReleaseContext( HCRYPTPROV hprov, DWORD flags )
{
    struct container *container = (struct container *)hprov;

    TRACE( "%p, %08x\n", (void *)hprov, flags );

    if (container->magic != MAGIC_CONTAINER) return FALSE;
    destroy_container( container );
    return TRUE;
}

BOOL WINAPI CPGetProvParam( HCRYPTPROV hprov, DWORD param, BYTE *data, DWORD *len, DWORD flags )
{
    return FALSE;
}

BOOL WINAPI CPGenKey( HCRYPTPROV hprov, ALG_ID algid, DWORD flags, HCRYPTKEY *ret_key )
{
    return FALSE;
}

BOOL WINAPI CPDestroyKey( HCRYPTPROV hprov, HCRYPTKEY hkey )
{
    return FALSE;
}

BOOL WINAPI CPImportKey( HCRYPTPROV hprov, const BYTE *data, DWORD len, HCRYPTKEY hpubkey, DWORD flags,
                         HCRYPTKEY *ret_key )
{
    return FALSE;
}

BOOL WINAPI CPExportKey( HCRYPTPROV hprov, HCRYPTKEY hkey, HCRYPTKEY hexpkey, DWORD blobtype, DWORD flags,
                         BYTE *data, DWORD *len )
{
    return FALSE;
}

BOOL WINAPI CPCreateHash( HCRYPTPROV hprov, ALG_ID algid, HCRYPTKEY hkey, DWORD flags, HCRYPTHASH *ret_hash )
{
    return FALSE;
}

BOOL WINAPI CPDestroyHash( HCRYPTPROV hprov, HCRYPTHASH hhash )
{
    return FALSE;
}

BOOL WINAPI CPHashData( HCRYPTPROV hprov, HCRYPTHASH hhash, const BYTE *data, DWORD len, DWORD flags )
{
    return FALSE;
}

BOOL WINAPI CPGetHashParam( HCRYPTPROV hprov, HCRYPTHASH hhash, DWORD param, BYTE *data, DWORD *len, DWORD flags )
{
    return FALSE;
}

BOOL WINAPI CPDeriveKey( HCRYPTPROV hprov, ALG_ID algid, HCRYPTHASH hhash, DWORD flags, HCRYPTKEY *ret_key )
{
    return FALSE;
}

BOOL WINAPI CPSignHash( HCRYPTPROV hprov, HCRYPTHASH hhash, DWORD keyspec, const WCHAR *desc, DWORD flags, BYTE *sig,
                        DWORD *siglen )
{
    return FALSE;
}

BOOL WINAPI CPVerifySignature( HCRYPTPROV hprov, HCRYPTHASH hhash, const BYTE *sig, DWORD siglen, HCRYPTKEY hpubkey,
                               const WCHAR *desc, DWORD flags )
{
    return FALSE;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;    /* prefer native version */
    case DLL_PROCESS_ATTACH:
        instance = hinstDLL;
        DisableThreadLibraryCalls(hinstDLL);
        break;
    }
    return TRUE;
}

/*****************************************************
 *    DllRegisterServer (DSSENH.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources( instance );
}

/*****************************************************
 *    DllUnregisterServer (DSSENH.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( instance );
}
