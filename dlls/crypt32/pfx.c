/*
 * Copyright 2019 Hans Leidekker for CodeWeavers
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
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "snmp.h"
#include "crypt32_private.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

static HCRYPTPROV import_key( struct cert_store_data *data, DWORD flags )
{
    HCRYPTPROV prov = 0;
    HCRYPTKEY cryptkey;
    DWORD size, acquire_flags;
    void *key;

    if (unix_funcs->import_store_key( data, NULL, &size ) != STATUS_BUFFER_TOO_SMALL) return 0;

    acquire_flags = (flags & CRYPT_MACHINE_KEYSET) | CRYPT_NEWKEYSET;
    if (!CryptAcquireContextW( &prov, NULL, MS_ENHANCED_PROV_W, PROV_RSA_FULL, acquire_flags ))
    {
        if (GetLastError() != NTE_EXISTS) return 0;

        acquire_flags &= ~CRYPT_NEWKEYSET;
        if (!CryptAcquireContextW( &prov, NULL, MS_ENHANCED_PROV_W, PROV_RSA_FULL, acquire_flags ))
        {
            WARN( "CryptAcquireContextW failed %08x\n", GetLastError() );
            return 0;
        }
    }

    key = malloc( size );
    if (unix_funcs->import_store_key( data, key, &size ) ||
        !CryptImportKey( prov, key, size, 0, flags & CRYPT_EXPORTABLE, &cryptkey ))
    {
        WARN( "CryptImportKey failed %08x\n", GetLastError() );
        CryptReleaseContext( prov, 0 );
        free( key );
        return 0;
    }
    CryptDestroyKey( cryptkey );
    free( key );
    return prov;
}

static BOOL set_key_context( const void *ctx, HCRYPTPROV prov )
{
    CERT_KEY_CONTEXT key_ctx;
    key_ctx.cbSize     = sizeof(key_ctx);
    key_ctx.hCryptProv = prov;
    key_ctx.dwKeySpec  = AT_KEYEXCHANGE;
    return CertSetCertificateContextProperty( ctx, CERT_KEY_CONTEXT_PROP_ID, 0, &key_ctx );
}

static WCHAR *get_provider_property( HCRYPTPROV prov, DWORD prop_id, DWORD *len )
{
    DWORD size = 0;
    WCHAR *ret;
    char *str;

    CryptGetProvParam( prov, prop_id, NULL, &size, 0 );
    if (!size) return NULL;
    if (!(str = CryptMemAlloc( size ))) return NULL;
    CryptGetProvParam( prov, prop_id, (BYTE *)str, &size, 0 );

    *len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
    if ((ret = CryptMemAlloc( *len * sizeof(WCHAR) ))) MultiByteToWideChar( CP_ACP, 0, str, -1, ret, *len );
    CryptMemFree( str );
    return ret;
}

static BOOL set_key_prov_info( const void *ctx, HCRYPTPROV prov )
{
    CRYPT_KEY_PROV_INFO *prov_info;
    DWORD size, len_container, len_name;
    WCHAR *ptr, *container, *name;
    BOOL ret;

    if (!(container = get_provider_property( prov, PP_CONTAINER, &len_container ))) return FALSE;
    if (!(name = get_provider_property( prov, PP_NAME, &len_name )))
    {
        CryptMemFree( container );
        return FALSE;
    }
    if (!(prov_info = CryptMemAlloc( sizeof(*prov_info) + (len_container + len_name) * sizeof(WCHAR) )))
    {
        CryptMemFree( container );
        CryptMemFree( name );
        return FALSE;
    }

    ptr = (WCHAR *)(prov_info + 1);
    prov_info->pwszContainerName = ptr;
    lstrcpyW( prov_info->pwszContainerName, container );

    ptr += len_container;
    prov_info->pwszProvName = ptr;
    lstrcpyW( prov_info->pwszProvName, name );

    size = sizeof(prov_info->dwProvType);
    CryptGetProvParam( prov, PP_PROVTYPE, (BYTE *)&prov_info->dwProvType, &size, 0 );

    prov_info->dwFlags     = 0;
    prov_info->cProvParam  = 0;
    prov_info->rgProvParam = NULL;
    size = sizeof(prov_info->dwKeySpec);
    CryptGetProvParam( prov, PP_KEYSPEC, (BYTE *)&prov_info->dwKeySpec, &size, 0 );

    ret = CertSetCertificateContextProperty( ctx, CERT_KEY_PROV_INFO_PROP_ID, 0, prov_info );

    CryptMemFree( prov_info );
    CryptMemFree( name );
    CryptMemFree( container );
    return ret;
}

HCERTSTORE WINAPI PFXImportCertStore( CRYPT_DATA_BLOB *pfx, const WCHAR *password, DWORD flags )
{
    DWORD i = 0, size;
    HCERTSTORE store = NULL;
    HCRYPTPROV prov = 0;
    struct cert_store_data *data = NULL;

    if (!pfx)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    if (flags & ~(CRYPT_EXPORTABLE|CRYPT_USER_KEYSET|CRYPT_MACHINE_KEYSET|PKCS12_NO_PERSIST_KEY))
    {
        FIXME( "flags %08x not supported\n", flags );
        return NULL;
    }
    if (!unix_funcs->open_cert_store)
    {
        FIXME( "(%p, %p, %08x)\n", pfx, password, flags );
        return NULL;
    }
    if (!unix_funcs->open_cert_store( pfx, password, &data )) return NULL;

    prov = import_key( data, flags );
    if (!prov) goto error;

    if (!(store = CertOpenStore( CERT_STORE_PROV_MEMORY, 0, 0, 0, NULL )))
    {
        WARN( "CertOpenStore failed %08x\n", GetLastError() );
        goto error;
    }

    for (;;)
    {
        const void *ctx = NULL;
        void *cert;

        if (unix_funcs->import_store_cert( data, i, NULL, &size ) != STATUS_BUFFER_TOO_SMALL) break;
        cert = malloc( size );
        if (!unix_funcs->import_store_cert( data, i++, cert, &size ))
            ctx = CertCreateContext( CERT_STORE_CERTIFICATE_CONTEXT, X509_ASN_ENCODING, cert, size, 0, NULL );
        free( cert );
        if (!ctx)
        {
            WARN( "CertCreateContext failed %08x\n", GetLastError() );
            goto error;
        }
        if (flags & PKCS12_NO_PERSIST_KEY)
        {
            if (!set_key_context( ctx, prov ))
            {
                WARN( "failed to set context property %08x\n", GetLastError() );
                CertFreeCertificateContext( ctx );
                goto error;
            }
        }
        else if (!set_key_prov_info( ctx, prov ))
        {
            WARN( "failed to set provider info property %08x\n", GetLastError() );
            CertFreeCertificateContext( ctx );
            goto error;
        }
        if (!CertAddCertificateContextToStore( store, ctx, CERT_STORE_ADD_ALWAYS, NULL ))
        {
            WARN( "CertAddCertificateContextToStore failed %08x\n", GetLastError() );
            CertFreeCertificateContext( ctx );
            goto error;
        }
        CertFreeCertificateContext( ctx );
    }
    unix_funcs->close_cert_store( data );
    return store;

error:
    CryptReleaseContext( prov, 0 );
    CertCloseStore( store, 0 );
    if (data) unix_funcs->close_cert_store( data );
    return NULL;
}

BOOL WINAPI PFXVerifyPassword( CRYPT_DATA_BLOB *pfx, const WCHAR *password, DWORD flags )
{
    FIXME( "(%p, %p, %08x): stub\n", pfx, password, flags );
    return FALSE;
}

BOOL WINAPI PFXExportCertStore( HCERTSTORE store, CRYPT_DATA_BLOB *pfx, const WCHAR *password, DWORD flags )
{
    return PFXExportCertStoreEx( store, pfx, password, NULL, flags );
}

BOOL WINAPI PFXExportCertStoreEx( HCERTSTORE store, CRYPT_DATA_BLOB *pfx, const WCHAR *password, void *reserved,
                                  DWORD flags )
{
    FIXME( "(%p, %p, %p, %p, %08x): stub\n", store, pfx, password, reserved, flags );
    return FALSE;
}
