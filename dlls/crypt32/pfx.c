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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#ifdef SONAME_LIBGNUTLS
#include <gnutls/pkcs12.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "snmp.h"

#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

#ifdef SONAME_LIBGNUTLS
WINE_DECLARE_DEBUG_CHANNEL(winediag);

/* Not present in gnutls version < 3.0 */
int gnutls_pkcs12_simple_parse(gnutls_pkcs12_t p12, const char *password,
    gnutls_x509_privkey_t *key, gnutls_x509_crt_t **chain, unsigned int *chain_len,
    gnutls_x509_crt_t **extra_certs, unsigned int *extra_certs_len,
    gnutls_x509_crl_t * crl, unsigned int flags);

int gnutls_x509_privkey_get_pk_algorithm2(gnutls_x509_privkey_t, unsigned int*);

static void *libgnutls_handle;
#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(gnutls_global_deinit);
MAKE_FUNCPTR(gnutls_global_init);
MAKE_FUNCPTR(gnutls_global_set_log_function);
MAKE_FUNCPTR(gnutls_global_set_log_level);
MAKE_FUNCPTR(gnutls_perror);
MAKE_FUNCPTR(gnutls_pkcs12_deinit);
MAKE_FUNCPTR(gnutls_pkcs12_import);
MAKE_FUNCPTR(gnutls_pkcs12_init);
MAKE_FUNCPTR(gnutls_pkcs12_simple_parse);
MAKE_FUNCPTR(gnutls_x509_crt_export);
MAKE_FUNCPTR(gnutls_x509_privkey_export_rsa_raw2);
MAKE_FUNCPTR(gnutls_x509_privkey_get_pk_algorithm2);
#undef MAKE_FUNCPTR

static void gnutls_log( int level, const char *msg )
{
    TRACE( "<%d> %s", level, msg );
}

BOOL gnutls_initialize(void)
{
    int ret;

    if (!(libgnutls_handle = dlopen( SONAME_LIBGNUTLS, RTLD_NOW )))
    {
        ERR_(winediag)( "failed to load libgnutls, no support for pfx import/export\n" );
        return FALSE;
    }

#define LOAD_FUNCPTR(f) \
    if (!(p##f = dlsym( libgnutls_handle, #f ))) \
    { \
        ERR( "failed to load %s\n", #f ); \
        goto fail; \
    }

    LOAD_FUNCPTR(gnutls_global_deinit)
    LOAD_FUNCPTR(gnutls_global_init)
    LOAD_FUNCPTR(gnutls_global_set_log_function)
    LOAD_FUNCPTR(gnutls_global_set_log_level)
    LOAD_FUNCPTR(gnutls_perror)
    LOAD_FUNCPTR(gnutls_pkcs12_deinit)
    LOAD_FUNCPTR(gnutls_pkcs12_import)
    LOAD_FUNCPTR(gnutls_pkcs12_init)
    LOAD_FUNCPTR(gnutls_pkcs12_simple_parse)
    LOAD_FUNCPTR(gnutls_x509_crt_export)
    LOAD_FUNCPTR(gnutls_x509_privkey_export_rsa_raw2)
    LOAD_FUNCPTR(gnutls_x509_privkey_get_pk_algorithm2)
#undef LOAD_FUNCPTR

    if ((ret = pgnutls_global_init()) != GNUTLS_E_SUCCESS)
    {
        pgnutls_perror( ret );
        goto fail;
    }

    if (TRACE_ON( crypt ))
    {
        pgnutls_global_set_log_level( 4 );
        pgnutls_global_set_log_function( gnutls_log );
    }

    return TRUE;

fail:
    dlclose( libgnutls_handle );
    libgnutls_handle = NULL;
    return FALSE;
}

void gnutls_uninitialize(void)
{
    pgnutls_global_deinit();
    dlclose( libgnutls_handle );
    libgnutls_handle = NULL;
}

#define RSA_MAGIC_KEY  ('R' | ('S' << 8) | ('A' << 16) | ('2' << 24))
#define RSA_PUBEXP     65537

static HCRYPTPROV import_key( gnutls_x509_privkey_t key, DWORD flags )
{
    int i, ret;
    unsigned int bitlen;
    gnutls_datum_t m, e, d, p, q, u, e1, e2;
    BLOBHEADER *hdr;
    RSAPUBKEY *rsakey;
    HCRYPTPROV prov = 0;
    HCRYPTKEY cryptkey;
    BYTE *buf, *src, *dst;
    DWORD size, acquire_flags;

    if ((ret = pgnutls_x509_privkey_get_pk_algorithm2( key, &bitlen )) < 0)
    {
        pgnutls_perror( ret );
        return 0;
    }

    if (ret != GNUTLS_PK_RSA)
    {
        FIXME( "key algorithm %u not supported\n", ret );
        return 0;
    }

    if ((ret = pgnutls_x509_privkey_export_rsa_raw2( key, &m, &e, &d, &p, &q, &u, &e1, &e2 )) < 0)
    {
        pgnutls_perror( ret );
        return 0;
    }

    size = sizeof(*hdr) + sizeof(*rsakey) + (bitlen * 9 / 16);
    if (!(buf = heap_alloc( size ))) goto done;

    hdr = (BLOBHEADER *)buf;
    hdr->bType    = PRIVATEKEYBLOB;
    hdr->bVersion = CUR_BLOB_VERSION;
    hdr->reserved = 0;
    hdr->aiKeyAlg = CALG_RSA_KEYX;

    rsakey = (RSAPUBKEY *)(hdr + 1);
    rsakey->magic  = RSA_MAGIC_KEY;
    rsakey->bitlen = bitlen;
    rsakey->pubexp = RSA_PUBEXP;

    dst = (BYTE *)(rsakey + 1);
    if (m.size == bitlen / 8 + 1 && !m.data[0]) src = m.data + 1;
    else if (m.size != bitlen / 8) goto done;
    else src = m.data;
    for (i = bitlen / 8 - 1; i >= 0; i--) *dst++ = src[i];

    if (p.size == bitlen / 16 + 1 && !p.data[0]) src = p.data + 1;
    else if (p.size != bitlen / 16) goto done;
    else src = p.data;
    for (i = bitlen / 16 - 1; i >= 0; i--) *dst++ = src[i];

    if (q.size == bitlen / 16 + 1 && !q.data[0]) src = q.data + 1;
    else if (q.size != bitlen / 16) goto done;
    else src = q.data;
    for (i = bitlen / 16 - 1; i >= 0; i--) *dst++ = src[i];

    if (e1.size == bitlen / 16 + 1 && !e1.data[0]) src = e1.data + 1;
    else if (e1.size != bitlen / 16) goto done;
    else src = e1.data;
    for (i = bitlen / 16 - 1; i >= 0; i--) *dst++ = src[i];

    if (e2.size == bitlen / 16 + 1 && !e2.data[0]) src = e2.data + 1;
    else if (e2.size != bitlen / 16) goto done;
    else src = e2.data;
    for (i = bitlen / 16 - 1; i >= 0; i--) *dst++ = src[i];

    if (u.size == bitlen / 16 + 1 && !u.data[0]) src = u.data + 1;
    else if (u.size != bitlen / 16) goto done;
    else src = u.data;
    for (i = bitlen / 16 - 1; i >= 0; i--) *dst++ = src[i];

    if (d.size == bitlen / 8 + 1 && !d.data[0]) src = d.data + 1;
    else if (d.size != bitlen / 8) goto done;
    else src = d.data;
    for (i = bitlen / 8 - 1; i >= 0; i--) *dst++ = src[i];

    acquire_flags = (flags & CRYPT_MACHINE_KEYSET) | CRYPT_NEWKEYSET;
    if (!CryptAcquireContextW( &prov, NULL, MS_ENHANCED_PROV_W, PROV_RSA_FULL, acquire_flags ))
    {
        if (GetLastError() != NTE_EXISTS) goto done;

        acquire_flags &= ~CRYPT_NEWKEYSET;
        if (!CryptAcquireContextW( &prov, NULL, MS_ENHANCED_PROV_W, PROV_RSA_FULL, acquire_flags ))
        {
            WARN( "CryptAcquireContextW failed %08x\n", GetLastError() );
            goto done;
        }
    }

    if (!CryptImportKey( prov, buf, size, 0, flags & CRYPT_EXPORTABLE, &cryptkey ))
    {
        WARN( "CryptImportKey failed %08x\n", GetLastError() );
        CryptReleaseContext( prov, 0 );
        prov = 0;
    }
    else CryptDestroyKey( cryptkey );

done:
    free( m.data );
    free( e.data );
    free( d.data );
    free( p.data );
    free( q.data );
    free( u.data );
    free( e1.data );
    free( e2.data );
    heap_free( buf );
    return prov;
}

static char *password_to_ascii( const WCHAR *str )
{
    char *ret;
    unsigned int i = 0;

    if (!(ret = heap_alloc( (strlenW(str) + 1) * sizeof(*ret) ))) return NULL;
    while (*str)
    {
        if (*str > 0x7f) WARN( "password contains non-ascii characters\n" );
        ret[i++] = *str++;
    }
    ret[i] = 0;
    return ret;
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
    strcpyW( prov_info->pwszContainerName, container );

    ptr += len_container;
    prov_info->pwszProvName = ptr;
    strcpyW( prov_info->pwszProvName, name );

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

#endif

HCERTSTORE WINAPI PFXImportCertStore( CRYPT_DATA_BLOB *pfx, const WCHAR *password, DWORD flags )
{
#ifdef SONAME_LIBGNUTLS
    gnutls_pkcs12_t p12;
    gnutls_datum_t pfx_data;
    gnutls_x509_privkey_t key;
    gnutls_x509_crt_t *chain;
    unsigned int chain_len, i;
    HCERTSTORE store = NULL;
    HCRYPTPROV prov = 0;
    char *pwd = NULL;
    int ret;

    TRACE( "(%p, %p, %08x)\n", pfx, password, flags );
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
    if (password && !(pwd = password_to_ascii( password ))) return NULL;

    if ((ret = pgnutls_pkcs12_init( &p12 )) < 0)
    {
        pgnutls_perror( ret );
        goto error;
    }

    pfx_data.data = pfx->pbData;
    pfx_data.size = pfx->cbData;
    if ((ret = pgnutls_pkcs12_import( p12, &pfx_data, GNUTLS_X509_FMT_DER, 0 )) < 0)
    {
        pgnutls_perror( ret );
        goto error;
    }

    if ((ret = pgnutls_pkcs12_simple_parse( p12, pwd ? pwd : "", &key, &chain, &chain_len, NULL, NULL, NULL, 0 )) < 0)
    {
        pgnutls_perror( ret );
        goto error;
    }

    if (!(prov = import_key( key, flags ))) goto error;
    if (!(store = CertOpenStore( CERT_STORE_PROV_MEMORY, 0, 0, 0, NULL )))
    {
        WARN( "CertOpenStore failed %08x\n", GetLastError() );
        goto error;
    }

    if (chain_len > 1) FIXME( "handle certificate chain\n" );
    for (i = 0; i < chain_len; i++)
    {
        const void *ctx;
        BYTE *crt_data;
        size_t size = 0;

        if ((ret = pgnutls_x509_crt_export( chain[i], GNUTLS_X509_FMT_DER, NULL, &size )) != GNUTLS_E_SHORT_MEMORY_BUFFER)
        {
            pgnutls_perror( ret );
            goto error;
        }

        if (!(crt_data = heap_alloc( size ))) goto error;
        if ((ret = pgnutls_x509_crt_export( chain[i], GNUTLS_X509_FMT_DER, crt_data, &size )) < 0)
        {
            pgnutls_perror( ret );
            heap_free( crt_data );
            goto error;
        }

        if (!(ctx = CertCreateContext( CERT_STORE_CERTIFICATE_CONTEXT, X509_ASN_ENCODING, crt_data, size, 0, NULL )))
        {
            WARN( "CertCreateContext failed %08x\n", GetLastError() );
            heap_free( crt_data );
            goto error;
        }
        heap_free( crt_data );

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

    pgnutls_pkcs12_deinit( p12 );
    return store;

error:
    CryptReleaseContext( prov, 0 );
    CertCloseStore( store, 0 );
    pgnutls_pkcs12_deinit( p12 );
    heap_free( pwd );
    return NULL;

#endif
    FIXME( "(%p, %p, %08x)\n", pfx, password, flags );
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
