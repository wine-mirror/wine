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

#if 0
#pragma makedep unix
#endif

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#ifdef SONAME_LIBGNUTLS
#include <gnutls/pkcs12.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wincrypt.h"
#include "snmp.h"
#include "crypt32_private.h"

#include "wine/debug.h"
#include "wine/heap.h"

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

static DWORD import_key( gnutls_x509_privkey_t key, DWORD flags, void **data_ret )
{
    int i, ret;
    unsigned int bitlen;
    gnutls_datum_t m, e, d, p, q, u, e1, e2;
    BLOBHEADER *hdr;
    RSAPUBKEY *rsakey;
    BYTE *buf, *src, *dst;
    DWORD size;

    *data_ret = NULL;

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
    if (!(buf = RtlAllocateHeap( GetProcessHeap(), 0, size ))) goto done;

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

    *data_ret = buf;

done:
    free( m.data );
    free( e.data );
    free( d.data );
    free( p.data );
    free( q.data );
    free( u.data );
    free( e1.data );
    free( e2.data );
    if (!*data_ret) RtlFreeHeap( GetProcessHeap(), 0, buf );
    return size;
}

static char *password_to_ascii( const WCHAR *str )
{
    char *ret;
    unsigned int i = 0;

    if (!(ret = malloc( (lstrlenW(str) + 1) * sizeof(*ret) ))) return NULL;
    while (*str)
    {
        if (*str > 0x7f) WARN( "password contains non-ascii characters\n" );
        ret[i++] = *str++;
    }
    ret[i] = 0;
    return ret;
}

static BOOL WINAPI import_cert_store( CRYPT_DATA_BLOB *pfx, const WCHAR *password, DWORD flags,
                                      void **key_ret, void ***chain_ret, DWORD *count_ret )
{
    gnutls_pkcs12_t p12;
    gnutls_datum_t pfx_data;
    gnutls_x509_privkey_t key;
    gnutls_x509_crt_t *chain;
    unsigned int chain_len, i;
    char *pwd = NULL;
    int ret;

    if (password && !(pwd = password_to_ascii( password ))) return FALSE;

    if ((ret = pgnutls_pkcs12_init( &p12 )) < 0) goto error;

    pfx_data.data = pfx->pbData;
    pfx_data.size = pfx->cbData;
    if ((ret = pgnutls_pkcs12_import( p12, &pfx_data, GNUTLS_X509_FMT_DER, 0 )) < 0) goto error;

    if ((ret = pgnutls_pkcs12_simple_parse( p12, pwd ? pwd : "", &key, &chain, &chain_len, NULL, NULL, NULL, 0 )) < 0)
        goto error;

    if (!import_key( key, flags, key_ret )) goto error;

    *chain_ret = RtlAllocateHeap( GetProcessHeap(), 0, chain_len * sizeof(*chain_ret) );
    *count_ret = chain_len;
    for (i = 0; i < chain_len; i++)
    {
        size_t size = 0;

        if ((ret = pgnutls_x509_crt_export( chain[i], GNUTLS_X509_FMT_DER, NULL, &size )) != GNUTLS_E_SHORT_MEMORY_BUFFER)
            goto error;

        (*chain_ret)[i] = RtlAllocateHeap( GetProcessHeap(), 0, size );
        if ((ret = pgnutls_x509_crt_export( chain[i], GNUTLS_X509_FMT_DER, (*chain_ret)[i], &size )) < 0)
        {
            i++;
            while (i) RtlFreeHeap( GetProcessHeap(), 0, (*chain_ret)[--i] );
            RtlFreeHeap( GetProcessHeap(), 0, *chain_ret );
            goto error;
        }
    }
    pgnutls_pkcs12_deinit( p12 );
    return TRUE;

error:
    pgnutls_perror( ret );
    pgnutls_pkcs12_deinit( p12 );
    free( pwd );
    return FALSE;
}

static const struct unix_funcs funcs =
{
    import_cert_store
};

NTSTATUS CDECL __wine_init_unix_lib( HMODULE module, DWORD reason, const void *ptr_in, void *ptr_out )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        if (!gnutls_initialize()) return STATUS_DLL_NOT_FOUND;
        *(const struct unix_funcs **)ptr_out = &funcs;
        break;
    case DLL_PROCESS_DETACH:
        if (libgnutls_handle) gnutls_uninitialize();
        break;
    }
    return STATUS_SUCCESS;
}

#endif /* SONAME_LIBGNUTLS */
