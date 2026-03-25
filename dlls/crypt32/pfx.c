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

#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "bcrypt.h"
#include "ncrypt.h"
#include "snmp.h"
#include "crypt32_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

static HCRYPTPROV import_key( cert_store_data_t data, DWORD flags )
{
    HCRYPTPROV prov = 0;
    HCRYPTKEY cryptkey;
    DWORD size, acquire_flags;
    void *key;
    struct import_store_key_params params = { data, NULL, &size };

    if (CRYPT32_CALL( import_store_key, &params ) != STATUS_BUFFER_TOO_SMALL) return 0;

    acquire_flags = (flags & CRYPT_MACHINE_KEYSET) | CRYPT_NEWKEYSET;
    if (!CryptAcquireContextW( &prov, NULL, MS_ENHANCED_PROV_W, PROV_RSA_FULL, acquire_flags ))
    {
        if (GetLastError() != NTE_EXISTS) return 0;

        acquire_flags &= ~CRYPT_NEWKEYSET;
        if (!CryptAcquireContextW( &prov, NULL, MS_ENHANCED_PROV_W, PROV_RSA_FULL, acquire_flags ))
        {
            WARN( "CryptAcquireContextW failed %08lx\n", GetLastError() );
            return 0;
        }
    }

    params.buf = key = CryptMemAlloc( size );
    if (CRYPT32_CALL( import_store_key, &params ) ||
        !CryptImportKey( prov, key, size, 0, flags & CRYPT_EXPORTABLE, &cryptkey ))
    {
        WARN( "CryptImportKey failed %08lx\n", GetLastError() );
        CryptReleaseContext( prov, 0 );
        CryptMemFree( key );
        return 0;
    }
    CryptDestroyKey( cryptkey );
    CryptMemFree( key );
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
    unsigned int key_count = 0;
    HCERTSTORE store = NULL;
    HCRYPTPROV prov = 0;
    cert_store_data_t data = 0;
    struct open_cert_store_params open_params = { pfx, password, &data, &key_count };
    struct close_cert_store_params close_params;

    if (!pfx)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    if (flags & ~(CRYPT_EXPORTABLE|CRYPT_USER_KEYSET|CRYPT_MACHINE_KEYSET|PKCS12_NO_PERSIST_KEY|PKCS12_ALWAYS_CNG_KSP))
    {
        FIXME( "flags %08lx not supported\n", flags );
        return NULL;
    }
    if (flags & PKCS12_ALWAYS_CNG_KSP)
    {
        FIXME( "flag PKCS12_ALWAYS_CNG_KSP ignored\n" );
    }
    if (CRYPT32_CALL( open_cert_store, &open_params )) return NULL;

    if (key_count)
    {
        prov = import_key( data, flags );
        if (!prov) goto error;
    }

    if (!(store = CertOpenStore( CERT_STORE_PROV_MEMORY, 0, 0, 0, NULL )))
    {
        WARN( "CertOpenStore failed %08lx\n", GetLastError() );
        goto error;
    }

    for (;;)
    {
        const void *ctx = NULL;
        void *cert;
        struct import_store_cert_params import_params = { data, i, NULL, &size };

        if (CRYPT32_CALL( import_store_cert, &import_params ) != STATUS_BUFFER_TOO_SMALL) break;
        import_params.buf = cert = CryptMemAlloc( size );
        if (!CRYPT32_CALL( import_store_cert, &import_params ))
            ctx = CertCreateContext( CERT_STORE_CERTIFICATE_CONTEXT, X509_ASN_ENCODING, cert, size, 0, NULL );
        CryptMemFree( cert );
        if (!ctx)
        {
            WARN( "CertCreateContext failed %08lx\n", GetLastError() );
            goto error;
        }
        if (prov)
        {
            if (flags & PKCS12_NO_PERSIST_KEY)
            {
                if (!set_key_context( ctx, prov ))
                {
                    WARN( "failed to set context property %08lx\n", GetLastError() );
                    CertFreeCertificateContext( ctx );
                    goto error;
                }
            }
            else if (!set_key_prov_info( ctx, prov ))
            {
                WARN( "failed to set provider info property %08lx\n", GetLastError() );
                CertFreeCertificateContext( ctx );
                goto error;
            }
        }
        if (!CertAddCertificateContextToStore( store, ctx, CERT_STORE_ADD_ALWAYS, NULL ))
        {
            WARN( "CertAddCertificateContextToStore failed %08lx\n", GetLastError() );
            CertFreeCertificateContext( ctx );
            goto error;
        }
        CertFreeCertificateContext( ctx );
        i++;
    }
    close_params.data = data;
    CRYPT32_CALL( close_cert_store, &close_params );
    return store;

error:
    if (prov) CryptReleaseContext( prov, 0 );
    CertCloseStore( store, 0 );
    close_params.data = data;
    CRYPT32_CALL( close_cert_store, &close_params );
    return NULL;
}

BOOL WINAPI PFXVerifyPassword( CRYPT_DATA_BLOB *pfx, const WCHAR *password, DWORD flags )
{
    FIXME( "(%p, %p, %08lx): stub\n", pfx, password, flags );
    return FALSE;
}

BOOL WINAPI PFXExportCertStore( HCERTSTORE store, CRYPT_DATA_BLOB *pfx, const WCHAR *password, DWORD flags )
{
    return PFXExportCertStoreEx( store, pfx, password, NULL, flags );
}

static void reverse_bytes( const BYTE *src, BYTE *dst, DWORD len )
{
    DWORD i;
    for (i = 0; i < len; i++) dst[i] = src[len - i - 1];
}

static BYTE *export_capi_key( HCRYPTPROV prov, DWORD key_spec, DWORD *out_size )
{
    HCRYPTKEY hkey;
    BYTE *capi_blob = NULL, *bcrypt_blob = NULL;
    DWORD capi_size, bitlen, mod_len, prime_len, pos, out_pos;
    BLOBHEADER *hdr;
    RSAPUBKEY *rsakey;
    BCRYPT_RSAKEY_BLOB *rsa_hdr;

    if (!CryptGetUserKey( prov, key_spec, &hkey ))
    {
        WARN( "CryptGetUserKey failed %08lx\n", GetLastError() );
        return NULL;
    }

    /* Query size. */
    capi_size = 0;
    if (!CryptExportKey( hkey, 0, PRIVATEKEYBLOB, 0, NULL, &capi_size ))
    {
        WARN( "CryptExportKey size query failed %08lx\n", GetLastError() );
        CryptDestroyKey( hkey );
        return NULL;
    }

    capi_blob = CryptMemAlloc( capi_size );
    if (!capi_blob)
    {
        CryptDestroyKey( hkey );
        return NULL;
    }

    if (!CryptExportKey( hkey, 0, PRIVATEKEYBLOB, 0, capi_blob, &capi_size ))
    {
        WARN( "CryptExportKey failed %08lx\n", GetLastError() );
        CryptMemFree( capi_blob );
        CryptDestroyKey( hkey );
        return NULL;
    }
    CryptDestroyKey( hkey );

    hdr = (BLOBHEADER *)capi_blob;
    rsakey = (RSAPUBKEY *)(hdr + 1);
    bitlen = rsakey->bitlen;
    mod_len = bitlen / 8;
    prime_len = bitlen / 16;

    /* Build BCRYPT_RSAFULLPRIVATE_BLOB. */
    *out_size = sizeof(BCRYPT_RSAKEY_BLOB) + sizeof(rsakey->pubexp) + mod_len * 2 + prime_len * 5;
    bcrypt_blob = CryptMemAlloc( *out_size );
    if (!bcrypt_blob)
    {
        CryptMemFree( capi_blob );
        return NULL;
    }

    rsa_hdr = (BCRYPT_RSAKEY_BLOB *)bcrypt_blob;
    rsa_hdr->Magic = BCRYPT_RSAFULLPRIVATE_MAGIC;
    rsa_hdr->BitLength = bitlen;
    rsa_hdr->cbPublicExp = sizeof(rsakey->pubexp);
    rsa_hdr->cbModulus = mod_len;
    rsa_hdr->cbPrime1 = prime_len;
    rsa_hdr->cbPrime2 = prime_len;

    out_pos = sizeof(BCRYPT_RSAKEY_BLOB);
    /* PublicExp (big-endian). */
    reverse_bytes( (const BYTE *)&rsakey->pubexp, bcrypt_blob + out_pos, sizeof(rsakey->pubexp) );
    out_pos += sizeof(rsakey->pubexp);

    /* CAPI data starts after BLOBHEADER + RSAPUBKEY. */
    pos = sizeof(BLOBHEADER) + sizeof(RSAPUBKEY);

    /* Modulus */
    reverse_bytes( capi_blob + pos, bcrypt_blob + out_pos, mod_len );
    pos += mod_len; out_pos += mod_len;
    /* Prime1 */
    reverse_bytes( capi_blob + pos, bcrypt_blob + out_pos, prime_len );
    pos += prime_len; out_pos += prime_len;
    /* Prime2 */
    reverse_bytes( capi_blob + pos, bcrypt_blob + out_pos, prime_len );
    pos += prime_len; out_pos += prime_len;
    /* Exponent1 */
    reverse_bytes( capi_blob + pos, bcrypt_blob + out_pos, prime_len );
    pos += prime_len; out_pos += prime_len;
    /* Exponent2 */
    reverse_bytes( capi_blob + pos, bcrypt_blob + out_pos, prime_len );
    pos += prime_len; out_pos += prime_len;
    /* Coefficient */
    reverse_bytes( capi_blob + pos, bcrypt_blob + out_pos, prime_len );
    pos += prime_len; out_pos += prime_len;
    /* PrivateExponent */
    reverse_bytes( capi_blob + pos, bcrypt_blob + out_pos, mod_len );

    CryptMemFree( capi_blob );
    return bcrypt_blob;
}

BOOL WINAPI PFXExportCertStoreEx( HCERTSTORE store, CRYPT_DATA_BLOB *pfx, const WCHAR *password, void *reserved,
                                  DWORD flags )
{
    const CERT_CONTEXT *cert;
    CERT_KEY_CONTEXT key_ctx;
    struct export_cert_store_params params;
    DWORD key_ctx_size, key_blob_size;
    BYTE *key_blob = NULL;
    SECURITY_STATUS sec_status;
    NTSTATUS status;
    BOOL ret = FALSE;

    TRACE( "(%p, %p, %p, %p, %08lx)\n", store, pfx, password, reserved, flags );

    if (!store || !pfx)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    /* Find the first certificate in the store (may be NULL for empty stores). */
    cert = CertEnumCertificatesInStore( store, NULL );

    /* Get the private key if EXPORT_PRIVATE_KEYS is requested. */
    if (cert && (flags & EXPORT_PRIVATE_KEYS))
    {
        key_ctx_size = sizeof(key_ctx);
        if (!CertGetCertificateContextProperty( cert, CERT_KEY_CONTEXT_PROP_ID, &key_ctx, &key_ctx_size ))
        {
            WARN( "no key context on certificate, error %08lx\n", GetLastError() );
            if (flags & REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY)
            {
                SetLastError( NTE_NOT_FOUND );
                CertFreeCertificateContext( cert );
                return FALSE;
            }
        }
        else if (key_ctx.dwKeySpec == CERT_NCRYPT_KEY_SPEC)
        {
            /* Query key blob size first. */
            sec_status = NCryptExportKey( key_ctx.hCryptProv, 0, BCRYPT_RSAFULLPRIVATE_BLOB, NULL,
                                          NULL, 0, &key_blob_size, 0 );
            if (sec_status)
            {
                WARN( "NCryptExportKey size query failed %08lx\n", sec_status );
                SetLastError( sec_status );
                CertFreeCertificateContext( cert );
                return FALSE;
            }

            key_blob = CryptMemAlloc( key_blob_size );
            if (!key_blob)
            {
                SetLastError( ERROR_OUTOFMEMORY );
                CertFreeCertificateContext( cert );
                return FALSE;
            }

            sec_status = NCryptExportKey( key_ctx.hCryptProv, 0, BCRYPT_RSAFULLPRIVATE_BLOB, NULL,
                                          key_blob, key_blob_size, &key_blob_size, 0 );
            if (sec_status)
            {
                WARN( "NCryptExportKey failed %08lx\n", sec_status );
                SetLastError( sec_status );
                CryptMemFree( key_blob );
                CertFreeCertificateContext( cert );
                return FALSE;
            }
        }
        else if (key_ctx.dwKeySpec == AT_KEYEXCHANGE || key_ctx.dwKeySpec == AT_SIGNATURE)
        {
            key_blob = export_capi_key( key_ctx.hCryptProv, key_ctx.dwKeySpec, &key_blob_size );
            if (!key_blob)
            {
                WARN( "failed to export CAPI key\n" );
                if (flags & REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY)
                {
                    SetLastError( NTE_NOT_FOUND );
                    CertFreeCertificateContext( cert );
                    return FALSE;
                }
            }
        }
        else
        {
            FIXME( "dwKeySpec %lu not supported\n", key_ctx.dwKeySpec );
            if (flags & REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY)
            {
                SetLastError( NTE_NOT_FOUND );
                CertFreeCertificateContext( cert );
                return FALSE;
            }
        }
    }

    params.cert_data     = cert ? cert->pbCertEncoded : NULL;
    params.cert_size     = cert ? cert->cbCertEncoded : 0;
    params.key_blob      = key_blob;
    params.key_blob_size = key_blob ? key_blob_size : 0;
    params.password      = password;
    params.pfx_data      = pfx->pbData;
    params.pfx_size      = &pfx->cbData;
    status = CRYPT32_CALL( export_cert_store, &params );
    if (status == STATUS_BUFFER_TOO_SMALL) status = STATUS_SUCCESS;
    else if (status) WARN( "unix export_cert_store failed %08lx\n", status );
    else ret = TRUE;

    CryptMemFree( key_blob );
    CertFreeCertificateContext( cert );
    SetLastError( RtlNtStatusToDosError( status ) );
    return ret;
}
