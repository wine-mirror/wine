/*
 * GnuTLS-based implementation of the schannel (SSL/TLS) provider.
 *
 * Copyright 2005 Juan Lang
 * Copyright 2008 Henri Verbeet
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
#include <stdio.h>
#ifdef SONAME_LIBGNUTLS
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "sspi.h"
#include "schannel.h"
#include "secur32_priv.h"
#include "wine/debug.h"
#include "wine/library.h"

#if defined(SONAME_LIBGNUTLS) && !defined(HAVE_SECURITY_SECURITY_H)

WINE_DEFAULT_DEBUG_CHANNEL(secur32);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

/* Not present in gnutls version < 2.9.10. */
static int (*pgnutls_cipher_get_block_size)(gnutls_cipher_algorithm_t algorithm);

static void *libgnutls_handle;
#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(gnutls_alert_get);
MAKE_FUNCPTR(gnutls_alert_get_name);
MAKE_FUNCPTR(gnutls_certificate_allocate_credentials);
MAKE_FUNCPTR(gnutls_certificate_free_credentials);
MAKE_FUNCPTR(gnutls_certificate_get_peers);
MAKE_FUNCPTR(gnutls_cipher_get);
MAKE_FUNCPTR(gnutls_cipher_get_key_size);
MAKE_FUNCPTR(gnutls_credentials_set);
MAKE_FUNCPTR(gnutls_deinit);
MAKE_FUNCPTR(gnutls_global_deinit);
MAKE_FUNCPTR(gnutls_global_init);
MAKE_FUNCPTR(gnutls_global_set_log_function);
MAKE_FUNCPTR(gnutls_global_set_log_level);
MAKE_FUNCPTR(gnutls_handshake);
MAKE_FUNCPTR(gnutls_init);
MAKE_FUNCPTR(gnutls_kx_get);
MAKE_FUNCPTR(gnutls_mac_get);
MAKE_FUNCPTR(gnutls_mac_get_key_size);
MAKE_FUNCPTR(gnutls_perror);
MAKE_FUNCPTR(gnutls_protocol_get_version);
MAKE_FUNCPTR(gnutls_priority_set_direct);
MAKE_FUNCPTR(gnutls_record_get_max_size);
MAKE_FUNCPTR(gnutls_record_recv);
MAKE_FUNCPTR(gnutls_record_send);
MAKE_FUNCPTR(gnutls_server_name_set);
MAKE_FUNCPTR(gnutls_transport_get_ptr);
MAKE_FUNCPTR(gnutls_transport_set_errno);
MAKE_FUNCPTR(gnutls_transport_set_ptr);
MAKE_FUNCPTR(gnutls_transport_set_pull_function);
MAKE_FUNCPTR(gnutls_transport_set_push_function);
#undef MAKE_FUNCPTR

#if GNUTLS_VERSION_MAJOR < 3
#define GNUTLS_CIPHER_AES_192_CBC 92
#define GNUTLS_CIPHER_AES_128_GCM 93
#define GNUTLS_CIPHER_AES_256_GCM 94

#define GNUTLS_MAC_AEAD 200

#define GNUTLS_KX_ANON_ECDH     11
#define GNUTLS_KX_ECDHE_RSA     12
#define GNUTLS_KX_ECDHE_ECDSA   13
#define GNUTLS_KX_ECDHE_PSK     14
#endif

static int compat_cipher_get_block_size(gnutls_cipher_algorithm_t cipher)
{
    switch(cipher) {
    case GNUTLS_CIPHER_3DES_CBC:
        return 8;
    case GNUTLS_CIPHER_AES_128_CBC:
    case GNUTLS_CIPHER_AES_256_CBC:
        return 16;
    case GNUTLS_CIPHER_ARCFOUR_128:
    case GNUTLS_CIPHER_ARCFOUR_40:
        return 1;
    case GNUTLS_CIPHER_DES_CBC:
        return 8;
    case GNUTLS_CIPHER_NULL:
        return 1;
    case GNUTLS_CIPHER_RC2_40_CBC:
        return 8;
    default:
        FIXME("Unknown cipher %#x, returning 1\n", cipher);
        return 1;
    }
}

static ssize_t schan_pull_adapter(gnutls_transport_ptr_t transport,
                                      void *buff, size_t buff_len)
{
    struct schan_transport *t = (struct schan_transport*)transport;
    gnutls_session_t s = (gnutls_session_t)schan_session_for_transport(t);

    int ret = schan_pull(transport, buff, &buff_len);
    if (ret)
    {
        pgnutls_transport_set_errno(s, ret);
        return -1;
    }

    return buff_len;
}

static ssize_t schan_push_adapter(gnutls_transport_ptr_t transport,
                                      const void *buff, size_t buff_len)
{
    struct schan_transport *t = (struct schan_transport*)transport;
    gnutls_session_t s = (gnutls_session_t)schan_session_for_transport(t);

    int ret = schan_push(transport, buff, &buff_len);
    if (ret)
    {
        pgnutls_transport_set_errno(s, ret);
        return -1;
    }

    return buff_len;
}

static const struct {
    DWORD enable_flag;
    const char *gnutls_flag;
} protocol_priority_flags[] = {
    {SP_PROT_TLS1_3_CLIENT, "VERS-TLS1.3"},
    {SP_PROT_TLS1_2_CLIENT, "VERS-TLS1.2"},
    {SP_PROT_TLS1_1_CLIENT, "VERS-TLS1.1"},
    {SP_PROT_TLS1_0_CLIENT, "VERS-TLS1.0"},
    {SP_PROT_SSL3_CLIENT,   "VERS-SSL3.0"}
    /* {SP_PROT_SSL2_CLIENT} is not supported by GnuTLS */
};

static DWORD supported_protocols;

static void check_supported_protocols(void)
{
    gnutls_session_t session;
    char priority[64];
    unsigned i;
    int err;

    err = pgnutls_init(&session, GNUTLS_CLIENT);
    if (err != GNUTLS_E_SUCCESS)
    {
        pgnutls_perror(err);
        return;
    }

    for(i = 0; i < ARRAY_SIZE(protocol_priority_flags); i++)
    {
        sprintf(priority, "NORMAL:-%s", protocol_priority_flags[i].gnutls_flag);
        err = pgnutls_priority_set_direct(session, priority, NULL);
        if (err == GNUTLS_E_SUCCESS)
        {
            TRACE("%s is supported\n", protocol_priority_flags[i].gnutls_flag);
            supported_protocols |= protocol_priority_flags[i].enable_flag;
        }
        else
            TRACE("%s is not supported\n", protocol_priority_flags[i].gnutls_flag);
    }

    pgnutls_deinit(session);
}

DWORD schan_imp_enabled_protocols(void)
{
    return supported_protocols;
}

BOOL schan_imp_create_session(schan_imp_session *session, schan_credentials *cred)
{
    gnutls_session_t *s = (gnutls_session_t*)session;
    char priority[128] = "NORMAL:%LATEST_RECORD_VERSION", *p;
    BOOL using_vers_all = FALSE, disabled;
    unsigned i;

    int err = pgnutls_init(s, cred->credential_use == SECPKG_CRED_INBOUND ? GNUTLS_SERVER : GNUTLS_CLIENT);
    if (err != GNUTLS_E_SUCCESS)
    {
        pgnutls_perror(err);
        return FALSE;
    }

    p = priority + strlen(priority);

    /* VERS-ALL is nice to use for forward compatibility. It was introduced before support for TLS1.3,
     * so if TLS1.3 is supported, we may safely use it. Otherwise explicitly disable all known
     * disabled protocols. */
    if (supported_protocols & SP_PROT_TLS1_3_CLIENT)
    {
        strcpy(p, ":-VERS-ALL");
        p += strlen(p);
        using_vers_all = TRUE;
    }

    for (i = 0; i < ARRAY_SIZE(protocol_priority_flags); i++)
    {
        if (!(supported_protocols & protocol_priority_flags[i].enable_flag)) continue;

        disabled = !(cred->enabled_protocols & protocol_priority_flags[i].enable_flag);
        if (using_vers_all && disabled) continue;

        *p++ = ':';
        *p++ = disabled ? '-' : '+';
        strcpy(p, protocol_priority_flags[i].gnutls_flag);
        p += strlen(p);
    }

    TRACE("Using %s priority\n", debugstr_a(priority));
    err = pgnutls_priority_set_direct(*s, priority, NULL);
    if (err != GNUTLS_E_SUCCESS)
    {
        pgnutls_perror(err);
        pgnutls_deinit(*s);
        return FALSE;
    }

    err = pgnutls_credentials_set(*s, GNUTLS_CRD_CERTIFICATE,
                                  (gnutls_certificate_credentials_t)cred->credentials);
    if (err != GNUTLS_E_SUCCESS)
    {
        pgnutls_perror(err);
        pgnutls_deinit(*s);
        return FALSE;
    }

    pgnutls_transport_set_pull_function(*s, schan_pull_adapter);
    pgnutls_transport_set_push_function(*s, schan_push_adapter);

    return TRUE;
}

void schan_imp_dispose_session(schan_imp_session session)
{
    gnutls_session_t s = (gnutls_session_t)session;
    pgnutls_deinit(s);
}

void schan_imp_set_session_transport(schan_imp_session session,
                                     struct schan_transport *t)
{
    gnutls_session_t s = (gnutls_session_t)session;
    pgnutls_transport_set_ptr(s, (gnutls_transport_ptr_t)t);
}

void schan_imp_set_session_target(schan_imp_session session, const char *target)
{
    gnutls_session_t s = (gnutls_session_t)session;

    pgnutls_server_name_set( s, GNUTLS_NAME_DNS, target, strlen(target) );
}

SECURITY_STATUS schan_imp_handshake(schan_imp_session session)
{
    gnutls_session_t s = (gnutls_session_t)session;
    int err;

    while(1) {
        err = pgnutls_handshake(s);
        switch(err) {
        case GNUTLS_E_SUCCESS:
            TRACE("Handshake completed\n");
            return SEC_E_OK;

        case GNUTLS_E_AGAIN:
            TRACE("Continue...\n");
            return SEC_I_CONTINUE_NEEDED;

        case GNUTLS_E_WARNING_ALERT_RECEIVED:
        {
            gnutls_alert_description_t alert = pgnutls_alert_get(s);

            WARN("WARNING ALERT: %d %s\n", alert, pgnutls_alert_get_name(alert));

            switch(alert) {
            case GNUTLS_A_UNRECOGNIZED_NAME:
                TRACE("Ignoring\n");
                continue;
            default:
                return SEC_E_INTERNAL_ERROR;
            }
        }

        case GNUTLS_E_FATAL_ALERT_RECEIVED:
        {
            gnutls_alert_description_t alert = pgnutls_alert_get(s);
            WARN("FATAL ALERT: %d %s\n", alert, pgnutls_alert_get_name(alert));
            return SEC_E_INTERNAL_ERROR;
        }

        default:
            pgnutls_perror(err);
            return SEC_E_INTERNAL_ERROR;
        }
    }

    /* Never reached */
    return SEC_E_OK;
}

static DWORD schannel_get_protocol(gnutls_protocol_t proto)
{
    /* FIXME: currently schannel only implements client connections, but
     * there's no reason it couldn't be used for servers as well.  The
     * context doesn't tell us which it is, so assume client for now.
     */
    switch (proto)
    {
    case GNUTLS_SSL3: return SP_PROT_SSL3_CLIENT;
    case GNUTLS_TLS1_0: return SP_PROT_TLS1_0_CLIENT;
    case GNUTLS_TLS1_1: return SP_PROT_TLS1_1_CLIENT;
    case GNUTLS_TLS1_2: return SP_PROT_TLS1_2_CLIENT;
    default:
        FIXME("unknown protocol %d\n", proto);
        return 0;
    }
}

static ALG_ID schannel_get_cipher_algid(gnutls_cipher_algorithm_t cipher)
{
    switch (cipher)
    {
    case GNUTLS_CIPHER_UNKNOWN:
    case GNUTLS_CIPHER_NULL: return 0;
    case GNUTLS_CIPHER_ARCFOUR_40:
    case GNUTLS_CIPHER_ARCFOUR_128: return CALG_RC4;
    case GNUTLS_CIPHER_DES_CBC: return CALG_DES;
    case GNUTLS_CIPHER_3DES_CBC: return CALG_3DES;
    case GNUTLS_CIPHER_AES_128_CBC:
    case GNUTLS_CIPHER_AES_128_GCM: return CALG_AES_128;
    case GNUTLS_CIPHER_AES_192_CBC: return CALG_AES_192;
    case GNUTLS_CIPHER_AES_256_GCM:
    case GNUTLS_CIPHER_AES_256_CBC: return CALG_AES_256;
    case GNUTLS_CIPHER_RC2_40_CBC: return CALG_RC2;
    default:
        FIXME("unknown algorithm %d\n", cipher);
        return 0;
    }
}

static ALG_ID schannel_get_mac_algid(gnutls_mac_algorithm_t mac, gnutls_cipher_algorithm_t cipher)
{
    switch (mac)
    {
    case GNUTLS_MAC_UNKNOWN:
    case GNUTLS_MAC_NULL: return 0;
    case GNUTLS_MAC_MD2: return CALG_MD2;
    case GNUTLS_MAC_MD5: return CALG_MD5;
    case GNUTLS_MAC_SHA1: return CALG_SHA1;
    case GNUTLS_MAC_SHA256: return CALG_SHA_256;
    case GNUTLS_MAC_SHA384: return CALG_SHA_384;
    case GNUTLS_MAC_SHA512: return CALG_SHA_512;
    case GNUTLS_MAC_AEAD:
        /* When using AEAD (such as GCM), we return PRF algorithm instead
           which is defined in RFC 5289. */
        switch (cipher)
        {
        case GNUTLS_CIPHER_AES_128_GCM: return CALG_SHA_256;
        case GNUTLS_CIPHER_AES_256_GCM: return CALG_SHA_384;
        default:
            break;
        }
        /* fall through */
    default:
        FIXME("unknown algorithm %d, cipher %d\n", mac, cipher);
        return 0;
    }
}

static ALG_ID schannel_get_kx_algid(int kx)
{
    switch (kx)
    {
    case GNUTLS_KX_UNKNOWN: return 0;
    case GNUTLS_KX_RSA:
    case GNUTLS_KX_RSA_EXPORT: return CALG_RSA_KEYX;
    case GNUTLS_KX_DHE_PSK:
    case GNUTLS_KX_DHE_DSS:
    case GNUTLS_KX_DHE_RSA: return CALG_DH_EPHEM;
    case GNUTLS_KX_ANON_ECDH: return CALG_ECDH;
    case GNUTLS_KX_ECDHE_RSA:
    case GNUTLS_KX_ECDHE_PSK:
    case GNUTLS_KX_ECDHE_ECDSA: return CALG_ECDH_EPHEM;
    default:
        FIXME("unknown algorithm %d\n", kx);
        return 0;
    }
}

unsigned int schan_imp_get_session_cipher_block_size(schan_imp_session session)
{
    gnutls_session_t s = (gnutls_session_t)session;
    return pgnutls_cipher_get_block_size(pgnutls_cipher_get(s));
}

unsigned int schan_imp_get_max_message_size(schan_imp_session session)
{
    return pgnutls_record_get_max_size((gnutls_session_t)session);
}

SECURITY_STATUS schan_imp_get_connection_info(schan_imp_session session,
                                              SecPkgContext_ConnectionInfo *info)
{
    gnutls_session_t s = (gnutls_session_t)session;
    gnutls_protocol_t proto = pgnutls_protocol_get_version(s);
    gnutls_cipher_algorithm_t alg = pgnutls_cipher_get(s);
    gnutls_mac_algorithm_t mac = pgnutls_mac_get(s);
    gnutls_kx_algorithm_t kx = pgnutls_kx_get(s);

    info->dwProtocol = schannel_get_protocol(proto);
    info->aiCipher = schannel_get_cipher_algid(alg);
    info->dwCipherStrength = pgnutls_cipher_get_key_size(alg) * 8;
    info->aiHash = schannel_get_mac_algid(mac, alg);
    info->dwHashStrength = pgnutls_mac_get_key_size(mac) * 8;
    info->aiExch = schannel_get_kx_algid(kx);
    /* FIXME: info->dwExchStrength? */
    info->dwExchStrength = 0;
    return SEC_E_OK;
}

ALG_ID schan_imp_get_key_signature_algorithm(schan_imp_session session)
{
    gnutls_session_t s = (gnutls_session_t)session;
    gnutls_kx_algorithm_t kx = pgnutls_kx_get(s);

    TRACE("(%p)\n", session);

    switch (kx)
    {
    case GNUTLS_KX_UNKNOWN: return 0;
    case GNUTLS_KX_RSA:
    case GNUTLS_KX_RSA_EXPORT:
    case GNUTLS_KX_DHE_RSA:
    case GNUTLS_KX_ECDHE_RSA: return CALG_RSA_SIGN;
    case GNUTLS_KX_ECDHE_ECDSA: return CALG_ECDSA;
    default:
        FIXME("unknown algorithm %d\n", kx);
        return 0;
    }
}

SECURITY_STATUS schan_imp_get_session_peer_certificate(schan_imp_session session, HCERTSTORE store,
                                                       PCCERT_CONTEXT *ret)
{
    gnutls_session_t s = (gnutls_session_t)session;
    PCCERT_CONTEXT cert = NULL;
    const gnutls_datum_t *datum;
    unsigned list_size, i;
    BOOL res;

    datum = pgnutls_certificate_get_peers(s, &list_size);
    if(!datum)
        return SEC_E_INTERNAL_ERROR;

    for(i = 0; i < list_size; i++) {
        res = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING, datum[i].data, datum[i].size,
                CERT_STORE_ADD_REPLACE_EXISTING, i ? NULL : &cert);
        if(!res) {
            if(i)
                CertFreeCertificateContext(cert);
            return GetLastError();
        }
    }

    *ret = cert;
    return SEC_E_OK;
}

SECURITY_STATUS schan_imp_send(schan_imp_session session, const void *buffer,
                               SIZE_T *length)
{
    gnutls_session_t s = (gnutls_session_t)session;
    SSIZE_T ret, total = 0;

    for (;;)
    {
        ret = pgnutls_record_send(s, (const char *)buffer + total, *length - total);
        if (ret >= 0)
        {
            total += ret;
            TRACE( "sent %ld now %ld/%ld\n", ret, total, *length );
            if (total == *length) return SEC_E_OK;
        }
        else if (ret == GNUTLS_E_AGAIN)
        {
            struct schan_transport *t = (struct schan_transport *)pgnutls_transport_get_ptr(s);
            SIZE_T count = 0;

            if (schan_get_buffer(t, &t->out, &count)) continue;
            return SEC_I_CONTINUE_NEEDED;
        }
        else
        {
            pgnutls_perror(ret);
            return SEC_E_INTERNAL_ERROR;
        }
    }
}

SECURITY_STATUS schan_imp_recv(schan_imp_session session, void *buffer,
                               SIZE_T *length)
{
    gnutls_session_t s = (gnutls_session_t)session;
    ssize_t ret;

again:
    ret = pgnutls_record_recv(s, buffer, *length);

    if (ret >= 0)
        *length = ret;
    else if (ret == GNUTLS_E_AGAIN)
    {
        struct schan_transport *t = (struct schan_transport *)pgnutls_transport_get_ptr(s);
        SIZE_T count = 0;

        if (schan_get_buffer(t, &t->in, &count))
            goto again;

        return SEC_I_CONTINUE_NEEDED;
    }
    else
    {
        pgnutls_perror(ret);
        return SEC_E_INTERNAL_ERROR;
    }

    return SEC_E_OK;
}

BOOL schan_imp_allocate_certificate_credentials(schan_credentials *c)
{
    int ret = pgnutls_certificate_allocate_credentials((gnutls_certificate_credentials_t*)&c->credentials);
    if (ret != GNUTLS_E_SUCCESS)
        pgnutls_perror(ret);
    return (ret == GNUTLS_E_SUCCESS);
}

void schan_imp_free_certificate_credentials(schan_credentials *c)
{
    pgnutls_certificate_free_credentials(c->credentials);
}

static void schan_gnutls_log(int level, const char *msg)
{
    TRACE("<%d> %s", level, msg);
}

BOOL schan_imp_init(void)
{
    int ret;

    libgnutls_handle = wine_dlopen(SONAME_LIBGNUTLS, RTLD_NOW, NULL, 0);
    if (!libgnutls_handle)
    {
        ERR_(winediag)("Failed to load libgnutls, secure connections will not be available.\n");
        return FALSE;
    }

#define LOAD_FUNCPTR(f) \
    if (!(p##f = wine_dlsym(libgnutls_handle, #f, NULL, 0))) \
    { \
        ERR("Failed to load %s\n", #f); \
        goto fail; \
    }

    LOAD_FUNCPTR(gnutls_alert_get)
    LOAD_FUNCPTR(gnutls_alert_get_name)
    LOAD_FUNCPTR(gnutls_certificate_allocate_credentials)
    LOAD_FUNCPTR(gnutls_certificate_free_credentials)
    LOAD_FUNCPTR(gnutls_certificate_get_peers)
    LOAD_FUNCPTR(gnutls_cipher_get)
    LOAD_FUNCPTR(gnutls_cipher_get_key_size)
    LOAD_FUNCPTR(gnutls_credentials_set)
    LOAD_FUNCPTR(gnutls_deinit)
    LOAD_FUNCPTR(gnutls_global_deinit)
    LOAD_FUNCPTR(gnutls_global_init)
    LOAD_FUNCPTR(gnutls_global_set_log_function)
    LOAD_FUNCPTR(gnutls_global_set_log_level)
    LOAD_FUNCPTR(gnutls_handshake)
    LOAD_FUNCPTR(gnutls_init)
    LOAD_FUNCPTR(gnutls_kx_get)
    LOAD_FUNCPTR(gnutls_mac_get)
    LOAD_FUNCPTR(gnutls_mac_get_key_size)
    LOAD_FUNCPTR(gnutls_perror)
    LOAD_FUNCPTR(gnutls_protocol_get_version)
    LOAD_FUNCPTR(gnutls_priority_set_direct)
    LOAD_FUNCPTR(gnutls_record_get_max_size);
    LOAD_FUNCPTR(gnutls_record_recv);
    LOAD_FUNCPTR(gnutls_record_send);
    LOAD_FUNCPTR(gnutls_server_name_set)
    LOAD_FUNCPTR(gnutls_transport_get_ptr)
    LOAD_FUNCPTR(gnutls_transport_set_errno)
    LOAD_FUNCPTR(gnutls_transport_set_ptr)
    LOAD_FUNCPTR(gnutls_transport_set_pull_function)
    LOAD_FUNCPTR(gnutls_transport_set_push_function)
#undef LOAD_FUNCPTR

    if (!(pgnutls_cipher_get_block_size = wine_dlsym(libgnutls_handle, "gnutls_cipher_get_block_size", NULL, 0)))
    {
        WARN("gnutls_cipher_get_block_size not found\n");
        pgnutls_cipher_get_block_size = compat_cipher_get_block_size;
    }

    ret = pgnutls_global_init();
    if (ret != GNUTLS_E_SUCCESS)
    {
        pgnutls_perror(ret);
        goto fail;
    }

    if (TRACE_ON(secur32))
    {
        pgnutls_global_set_log_level(4);
        pgnutls_global_set_log_function(schan_gnutls_log);
    }

    check_supported_protocols();
    return TRUE;

fail:
    wine_dlclose(libgnutls_handle, NULL, 0);
    libgnutls_handle = NULL;
    return FALSE;
}

void schan_imp_deinit(void)
{
    pgnutls_global_deinit();
    wine_dlclose(libgnutls_handle, NULL, 0);
    libgnutls_handle = NULL;
}

#endif /* SONAME_LIBGNUTLS && !HAVE_SECURITY_SECURITY_H */
