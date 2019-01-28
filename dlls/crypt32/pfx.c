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
#include "wine/library.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

#ifdef SONAME_LIBGNUTLS
WINE_DECLARE_DEBUG_CHANNEL(winediag);

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
#undef MAKE_FUNCPTR

static void gnutls_log( int level, const char *msg )
{
    TRACE( "<%d> %s", level, msg );
}

BOOL gnutls_initialize(void)
{
    int ret;

    if (!(libgnutls_handle = wine_dlopen( SONAME_LIBGNUTLS, RTLD_NOW, NULL, 0 )))
    {
        ERR_(winediag)( "failed to load libgnutls, no support for pfx import/export\n" );
        return FALSE;
    }

#define LOAD_FUNCPTR(f) \
    if (!(p##f = wine_dlsym( libgnutls_handle, #f, NULL, 0 ))) \
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
    int ret;

    TRACE( "(%p, %p, %08x)\n", pfx, password, flags );
    if (!pfx)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    if (password)
    {
        FIXME( "password not supported\n" );
        return NULL;
    }
    if (flags & ~(CRYPT_EXPORTABLE|CRYPT_USER_KEYSET|PKCS12_NO_PERSIST_KEY))
    {
        FIXME( "flags %08x not supported\n", flags );
        return NULL;
    }

    if ((ret = pgnutls_pkcs12_init( &p12 )) < 0)
    {
        pgnutls_perror( ret );
        return NULL;
    }

    pfx_data.data = pfx->pbData;
    pfx_data.size = pfx->cbData;
    if ((ret = pgnutls_pkcs12_import( p12, &pfx_data, GNUTLS_X509_FMT_DER, 0 )) < 0)
    {
        pgnutls_perror( ret );
        return NULL;
    }

    if ((ret = pgnutls_pkcs12_simple_parse( p12, "", &key, &chain, &chain_len, NULL, NULL, NULL, 0 )) < 0)
    {
        pgnutls_perror( ret );
        goto error;
    }

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
    CertCloseStore( store, 0 );
    pgnutls_pkcs12_deinit( p12 );
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
