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
#include <assert.h>
#ifdef SONAME_LIBGNUTLS
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#include <gnutls/abstract.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "sspi.h"
#include "schannel.h"
#include "lmcons.h"
#include "winreg.h"
#include "secur32_priv.h"

#include "wine/debug.h"
#include "wine/unicode.h"

#if defined(SONAME_LIBGNUTLS) && !defined(HAVE_SECURITY_SECURITY_H)

WINE_DEFAULT_DEBUG_CHANNEL(secur32);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

/* Not present in gnutls version < 2.9.10. */
static int (*pgnutls_cipher_get_block_size)(gnutls_cipher_algorithm_t);

/* Not present in gnutls version < 3.2.0. */
static int (*pgnutls_alpn_get_selected_protocol)(gnutls_session_t, gnutls_datum_t *);
static int (*pgnutls_alpn_set_protocols)(gnutls_session_t, const gnutls_datum_t *,
                                         unsigned, unsigned int);

/* Not present in gnutls version < 3.3.0. */
static int (*pgnutls_privkey_import_rsa_raw)(gnutls_privkey_t, const gnutls_datum_t *,
                                        const gnutls_datum_t *, const gnutls_datum_t *,
                                        const gnutls_datum_t *, const gnutls_datum_t *,
                                        const gnutls_datum_t *, const gnutls_datum_t *,
                                        const gnutls_datum_t *);

/* Not present in gnutls version < 3.4.0. */
static int (*pgnutls_privkey_export_x509)(gnutls_privkey_t, gnutls_x509_privkey_t *);

static void *libgnutls_handle;
#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(gnutls_alert_get);
MAKE_FUNCPTR(gnutls_alert_get_name);
MAKE_FUNCPTR(gnutls_certificate_allocate_credentials);
MAKE_FUNCPTR(gnutls_certificate_free_credentials);
MAKE_FUNCPTR(gnutls_certificate_get_peers);
MAKE_FUNCPTR(gnutls_certificate_set_x509_key);
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
MAKE_FUNCPTR(gnutls_privkey_deinit);
MAKE_FUNCPTR(gnutls_privkey_init);
MAKE_FUNCPTR(gnutls_record_get_max_size);
MAKE_FUNCPTR(gnutls_record_recv);
MAKE_FUNCPTR(gnutls_record_send);
MAKE_FUNCPTR(gnutls_server_name_set);
MAKE_FUNCPTR(gnutls_transport_get_ptr);
MAKE_FUNCPTR(gnutls_transport_set_errno);
MAKE_FUNCPTR(gnutls_transport_set_ptr);
MAKE_FUNCPTR(gnutls_transport_set_pull_function);
MAKE_FUNCPTR(gnutls_transport_set_push_function);
MAKE_FUNCPTR(gnutls_x509_crt_deinit);
MAKE_FUNCPTR(gnutls_x509_crt_import);
MAKE_FUNCPTR(gnutls_x509_crt_init);
MAKE_FUNCPTR(gnutls_x509_privkey_deinit);
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

#if GNUTLS_VERSION_MAJOR < 3 || (GNUTLS_VERSION_MAJOR == 3 && GNUTLS_VERSION_MINOR < 5)
#define GNUTLS_ALPN_SERVER_PRECEDENCE (1<<1)
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

static int compat_gnutls_privkey_export_x509(gnutls_privkey_t privkey, gnutls_x509_privkey_t *key)
{
    FIXME("\n");
    return GNUTLS_E_UNKNOWN_PK_ALGORITHM;
}

static int compat_gnutls_privkey_import_rsa_raw(gnutls_privkey_t key, const gnutls_datum_t *p1,
                                        const gnutls_datum_t *p2, const gnutls_datum_t *p3,
                                        const gnutls_datum_t *p4, const gnutls_datum_t *p5,
                                        const gnutls_datum_t *p6, const gnutls_datum_t *p7,
                                        const gnutls_datum_t *p8)
{
    FIXME("\n");
    return GNUTLS_E_UNKNOWN_PK_ALGORITHM;
}

static int compat_gnutls_alpn_get_selected_protocol(gnutls_session_t session, gnutls_datum_t *protocol)
{
    FIXME("\n");
    return GNUTLS_E_INVALID_REQUEST;
}

static int compat_gnutls_alpn_set_protocols(gnutls_session_t session, const gnutls_datum_t *protocols,
                                            unsigned size, unsigned int flags)
{
    FIXME("\n");
    return GNUTLS_E_INVALID_REQUEST;
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
    else if (ret == GNUTLS_E_REHANDSHAKE)
    {
        TRACE("Rehandshake requested\n");
        return SEC_I_RENEGOTIATE;
    }
    else
    {
        pgnutls_perror(ret);
        return SEC_E_INTERNAL_ERROR;
    }

    return SEC_E_OK;
}

static unsigned int parse_alpn_protocol_list(unsigned char *buffer, unsigned int buflen, gnutls_datum_t *list)
{
    unsigned int len, offset = 0, count = 0;

    while (buflen)
    {
        len = buffer[offset++];
        buflen--;
        if (!len || len > buflen) return 0;
        if (list)
        {
            list[count].data = &buffer[offset];
            list[count].size = len;
        }
        buflen -= len;
        offset += len;
        count++;
    }

    return count;
}

void schan_imp_set_application_protocols(schan_imp_session session, unsigned char *buffer, unsigned int buflen)
{
    gnutls_session_t s = (gnutls_session_t)session;
    unsigned int extension_len, extension, count = 0, offset = 0;
    unsigned short list_len;
    gnutls_datum_t *protocols;
    int ret;

    if (sizeof(extension_len) > buflen) return;
    extension_len = *(unsigned int *)&buffer[offset];
    offset += sizeof(extension_len);

    if (offset + sizeof(extension) > buflen) return;
    extension = *(unsigned int *)&buffer[offset];
    if (extension != SecApplicationProtocolNegotiationExt_ALPN)
    {
        FIXME("extension %u not supported\n", extension);
        return;
    }
    offset += sizeof(extension);

    if (offset + sizeof(list_len) > buflen) return;
    list_len = *(unsigned short *)&buffer[offset];
    offset += sizeof(list_len);

    if (offset + list_len > buflen) return;
    count = parse_alpn_protocol_list(&buffer[offset], list_len, NULL);
    if (!count || !(protocols = heap_alloc(count * sizeof(*protocols)))) return;

    parse_alpn_protocol_list(&buffer[offset], list_len, protocols);
    if ((ret = pgnutls_alpn_set_protocols(s, protocols, count, GNUTLS_ALPN_SERVER_PRECEDENCE) < 0))
    {
        pgnutls_perror(ret);
    }

    heap_free(protocols);
}

SECURITY_STATUS schan_imp_get_application_protocol(schan_imp_session session,
                                                   SecPkgContext_ApplicationProtocol *protocol)
{
    gnutls_session_t s = (gnutls_session_t)session;
    gnutls_datum_t selected;

    memset(protocol, 0, sizeof(*protocol));
    if (pgnutls_alpn_get_selected_protocol(s, &selected) < 0) return SEC_E_OK;

    if (selected.size <= sizeof(protocol->ProtocolId))
    {
        protocol->ProtoNegoStatus = SecApplicationProtocolNegotiationStatus_Success;
        protocol->ProtoNegoExt    = SecApplicationProtocolNegotiationExt_ALPN;
        protocol->ProtocolIdSize  = selected.size;
        memcpy(protocol->ProtocolId, selected.data, selected.size);
        TRACE("returning %s\n", debugstr_an((const char *)selected.data, selected.size));
    }
    return SEC_E_OK;
}

static WCHAR *get_key_container_path(const CERT_CONTEXT *ctx)
{
    static const WCHAR rsabaseW[] =
        {'S','o','f','t','w','a','r','e','\\','W','i','n','e','\\','C','r','y','p','t','o','\\','R','S','A','\\',0};
    CERT_KEY_CONTEXT keyctx;
    DWORD size = sizeof(keyctx), prov_size = 0;
    CRYPT_KEY_PROV_INFO *prov;
    WCHAR username[UNLEN + 1], *ret = NULL;
    DWORD len = ARRAY_SIZE(username);

    if (CertGetCertificateContextProperty(ctx, CERT_KEY_CONTEXT_PROP_ID, &keyctx, &size))
    {
        char *str;
        if (!CryptGetProvParam(keyctx.hCryptProv, PP_CONTAINER, NULL, &size, 0)) return NULL;
        if (!(str = heap_alloc(size))) return NULL;
        if (!CryptGetProvParam(keyctx.hCryptProv, PP_CONTAINER, (BYTE *)str, &size, 0)) return NULL;

        len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
        if (!(ret = heap_alloc(sizeof(rsabaseW) + len * sizeof(WCHAR))))
        {
            heap_free(str);
            return NULL;
        }
        strcpyW(ret, rsabaseW);
        MultiByteToWideChar(CP_ACP, 0, str, -1, ret + strlenW(ret), len);
        heap_free(str);
    }
    else if (CertGetCertificateContextProperty(ctx, CERT_KEY_PROV_INFO_PROP_ID, NULL, &prov_size))
    {
        if (!(prov = heap_alloc(prov_size))) return NULL;
        if (!CertGetCertificateContextProperty(ctx, CERT_KEY_PROV_INFO_PROP_ID, prov, &prov_size))
        {
            heap_free(prov);
            return NULL;
        }
        if (!(ret = heap_alloc(sizeof(rsabaseW) + strlenW(prov->pwszContainerName) * sizeof(WCHAR))))
        {
            heap_free(prov);
            return NULL;
        }
        strcpyW(ret, rsabaseW);
        strcatW(ret, prov->pwszContainerName);
        heap_free(prov);
    }

    if (!ret && GetUserNameW(username, &len) && (ret = heap_alloc(sizeof(rsabaseW) + len * sizeof(WCHAR))))
    {
        strcpyW(ret, rsabaseW);
        strcatW(ret, username);
    }

    return ret;
}

#define MAX_LEAD_BYTES 8
static BYTE *get_key_blob(const CERT_CONTEXT *ctx, ULONG *size)
{
    static const WCHAR keyexchangeW[] =
        {'K','e','y','E','x','c','h','a','n','g','e','K','e','y','P','a','i','r',0};
    static const WCHAR signatureW[] =
        {'S','i','g','n','a','t','u','r','e','K','e','y','P','a','i','r',0};
    BYTE *buf, *ret = NULL;
    DATA_BLOB blob_in, blob_out;
    DWORD spec = 0, type, len;
    WCHAR *path;
    HKEY hkey;

    if (!(path = get_key_container_path(ctx))) return NULL;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, path, 0, KEY_READ, &hkey))
    {
        heap_free(path);
        return NULL;
    }
    heap_free(path);

    if (!RegQueryValueExW(hkey, keyexchangeW, 0, &type, NULL, &len)) spec = AT_KEYEXCHANGE;
    else if (!RegQueryValueExW(hkey, signatureW, 0, &type, NULL, &len)) spec = AT_SIGNATURE;
    else
    {
        RegCloseKey(hkey);
        return NULL;
    }

    if (!(buf = heap_alloc(len + MAX_LEAD_BYTES)))
    {
        RegCloseKey(hkey);
        return NULL;
    }

    if (!RegQueryValueExW(hkey, (spec == AT_KEYEXCHANGE) ? keyexchangeW : signatureW, 0, &type, buf, &len))
    {
        blob_in.pbData = buf;
        blob_in.cbData = len;
        if (CryptUnprotectData(&blob_in, NULL, NULL, NULL, NULL, 0, &blob_out))
        {
            assert(blob_in.cbData >= blob_out.cbData);
            memcpy(buf, blob_out.pbData, blob_out.cbData);
            LocalFree(blob_out.pbData);
            *size = blob_out.cbData + MAX_LEAD_BYTES;
            ret = buf;
        }
    }
    else heap_free(buf);

    RegCloseKey(hkey);
    return ret;
}

static inline void reverse_bytes(BYTE *buf, ULONG len)
{
    BYTE tmp;
    ULONG i;
    for (i = 0; i < len / 2; i++)
    {
        tmp = buf[i];
        buf[i] = buf[len - i - 1];
        buf[len - i - 1] = tmp;
    }
}

static ULONG set_component(gnutls_datum_t *comp, BYTE *data, ULONG len, ULONG *buflen)
{
    comp->data = data;
    comp->size = len;
    reverse_bytes(comp->data, comp->size);
    if (comp->data[0] & 0x80) /* add leading 0 byte if most significant bit is set */
    {
        memmove(comp->data + 1, comp->data, *buflen);
        comp->data[0] = 0;
        comp->size++;
    }
    *buflen -= comp->size;
    return comp->size;
}

static gnutls_x509_privkey_t get_x509_key(const CERT_CONTEXT *ctx)
{
    gnutls_privkey_t key = NULL;
    gnutls_x509_privkey_t x509key = NULL;
    gnutls_datum_t m, e, d, p, q, u, e1, e2;
    BYTE *ptr, *buffer;
    RSAPUBKEY *rsakey;
    DWORD size;
    int ret;

    if (!(buffer = get_key_blob(ctx, &size))) return NULL;
    if (size < sizeof(BLOBHEADER)) goto done;

    rsakey = (RSAPUBKEY *)(buffer + sizeof(BLOBHEADER));
    TRACE("RSA key bitlen %u pubexp %u\n", rsakey->bitlen, rsakey->pubexp);

    size -= sizeof(BLOBHEADER) + FIELD_OFFSET(RSAPUBKEY, pubexp);
    set_component(&e, (BYTE *)&rsakey->pubexp, sizeof(rsakey->pubexp), &size);

    ptr = (BYTE *)(rsakey + 1);
    ptr += set_component(&m, ptr, rsakey->bitlen / 8, &size);
    ptr += set_component(&p, ptr, rsakey->bitlen / 16, &size);
    ptr += set_component(&q, ptr, rsakey->bitlen / 16, &size);
    ptr += set_component(&e1, ptr, rsakey->bitlen / 16, &size);
    ptr += set_component(&e2, ptr, rsakey->bitlen / 16, &size);
    ptr += set_component(&u, ptr, rsakey->bitlen / 16, &size);
    ptr += set_component(&d, ptr, rsakey->bitlen / 8, &size);

    if ((ret = pgnutls_privkey_init(&key)) < 0)
    {
        pgnutls_perror(ret);
        goto done;
    }

    if ((ret = pgnutls_privkey_import_rsa_raw(key, &m, &e, &d, &p, &q, &u, &e1, &e2)) < 0)
    {
        pgnutls_perror(ret);
        goto done;
    }

    if ((ret = pgnutls_privkey_export_x509(key, &x509key)) < 0)
    {
        pgnutls_perror(ret);
    }

done:
    heap_free(buffer);
    pgnutls_privkey_deinit(key);
    return x509key;
}

static gnutls_x509_crt_t get_x509_crt(const CERT_CONTEXT *ctx)
{
    gnutls_datum_t data;
    gnutls_x509_crt_t crt;
    int ret;

    if (!ctx) return FALSE;
    if (ctx->dwCertEncodingType != X509_ASN_ENCODING)
    {
        FIXME("encoding type %u not supported\n", ctx->dwCertEncodingType);
        return NULL;
    }

    if ((ret = pgnutls_x509_crt_init(&crt)) < 0)
    {
        pgnutls_perror(ret);
        return NULL;
    }

    data.data = ctx->pbCertEncoded;
    data.size = ctx->cbCertEncoded;
    if ((ret = pgnutls_x509_crt_import(crt, &data, GNUTLS_X509_FMT_DER)) < 0)
    {
        pgnutls_perror(ret);
        pgnutls_x509_crt_deinit(crt);
        return NULL;
    }

    return crt;
}

BOOL schan_imp_allocate_certificate_credentials(schan_credentials *c, const CERT_CONTEXT *ctx)
{
    gnutls_certificate_credentials_t creds;
    gnutls_x509_crt_t crt;
    gnutls_x509_privkey_t key;
    int ret;

    ret = pgnutls_certificate_allocate_credentials(&creds);
    if (ret != GNUTLS_E_SUCCESS)
    {
        pgnutls_perror(ret);
        return FALSE;
    }

    if (!ctx)
    {
        c->credentials = creds;
        return TRUE;
    }

    if (!(crt = get_x509_crt(ctx)))
    {
        pgnutls_certificate_free_credentials(creds);
        return FALSE;
    }

    if (!(key = get_x509_key(ctx)))
    {
        pgnutls_x509_crt_deinit(crt);
        pgnutls_certificate_free_credentials(creds);
        return FALSE;
    }

    ret = pgnutls_certificate_set_x509_key(creds, &crt, 1, key);
    pgnutls_x509_privkey_deinit(key);
    pgnutls_x509_crt_deinit(crt);
    if (ret != GNUTLS_E_SUCCESS)
    {
        pgnutls_perror(ret);
        pgnutls_certificate_free_credentials(creds);
        return FALSE;
    }

    c->credentials = creds;
    return TRUE;
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
    const char *env_str;
    int ret;

    if ((env_str = getenv("GNUTLS_SYSTEM_PRIORITY_FILE")))
    {
        WARN("GNUTLS_SYSTEM_PRIORITY_FILE is %s.\n", debugstr_a(env_str));
    }
    else
    {
        WARN("Setting GNUTLS_SYSTEM_PRIORITY_FILE to \"/dev/null\".\n");
        setenv("GNUTLS_SYSTEM_PRIORITY_FILE", "/dev/null", 0);
    }

    libgnutls_handle = dlopen(SONAME_LIBGNUTLS, RTLD_NOW);
    if (!libgnutls_handle)
    {
        ERR_(winediag)("Failed to load libgnutls, secure connections will not be available.\n");
        return FALSE;
    }

#define LOAD_FUNCPTR(f) \
    if (!(p##f = dlsym(libgnutls_handle, #f))) \
    { \
        ERR("Failed to load %s\n", #f); \
        goto fail; \
    }

    LOAD_FUNCPTR(gnutls_alert_get)
    LOAD_FUNCPTR(gnutls_alert_get_name)
    LOAD_FUNCPTR(gnutls_certificate_allocate_credentials)
    LOAD_FUNCPTR(gnutls_certificate_free_credentials)
    LOAD_FUNCPTR(gnutls_certificate_get_peers)
    LOAD_FUNCPTR(gnutls_certificate_set_x509_key)
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
    LOAD_FUNCPTR(gnutls_privkey_deinit)
    LOAD_FUNCPTR(gnutls_privkey_init)
    LOAD_FUNCPTR(gnutls_record_get_max_size);
    LOAD_FUNCPTR(gnutls_record_recv);
    LOAD_FUNCPTR(gnutls_record_send);
    LOAD_FUNCPTR(gnutls_server_name_set)
    LOAD_FUNCPTR(gnutls_transport_get_ptr)
    LOAD_FUNCPTR(gnutls_transport_set_errno)
    LOAD_FUNCPTR(gnutls_transport_set_ptr)
    LOAD_FUNCPTR(gnutls_transport_set_pull_function)
    LOAD_FUNCPTR(gnutls_transport_set_push_function)
    LOAD_FUNCPTR(gnutls_x509_crt_deinit)
    LOAD_FUNCPTR(gnutls_x509_crt_import)
    LOAD_FUNCPTR(gnutls_x509_crt_init)
    LOAD_FUNCPTR(gnutls_x509_privkey_deinit)
#undef LOAD_FUNCPTR

    if (!(pgnutls_cipher_get_block_size = dlsym(libgnutls_handle, "gnutls_cipher_get_block_size")))
    {
        WARN("gnutls_cipher_get_block_size not found\n");
        pgnutls_cipher_get_block_size = compat_cipher_get_block_size;
    }
    if (!(pgnutls_alpn_set_protocols = dlsym(libgnutls_handle, "gnutls_alpn_set_protocols")))
    {
        WARN("gnutls_alpn_set_protocols not found\n");
        pgnutls_alpn_set_protocols = compat_gnutls_alpn_set_protocols;
    }
    if (!(pgnutls_alpn_get_selected_protocol = dlsym(libgnutls_handle, "gnutls_alpn_get_selected_protocol")))
    {
        WARN("gnutls_alpn_get_selected_protocol not found\n");
        pgnutls_alpn_get_selected_protocol = compat_gnutls_alpn_get_selected_protocol;
    }
    if (!(pgnutls_privkey_export_x509 = dlsym(libgnutls_handle, "gnutls_privkey_export_x509")))
    {
        WARN("gnutls_privkey_export_x509 not found\n");
        pgnutls_privkey_export_x509 = compat_gnutls_privkey_export_x509;
    }
    if (!(pgnutls_privkey_import_rsa_raw = dlsym(libgnutls_handle, "gnutls_privkey_import_rsa_raw")))
    {
        WARN("gnutls_privkey_import_rsa_raw not found\n");
        pgnutls_privkey_import_rsa_raw = compat_gnutls_privkey_import_rsa_raw;
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
    dlclose(libgnutls_handle);
    libgnutls_handle = NULL;
    return FALSE;
}

void schan_imp_deinit(void)
{
    pgnutls_global_deinit();
    dlclose(libgnutls_handle);
    libgnutls_handle = NULL;
}

#endif /* SONAME_LIBGNUTLS && !HAVE_SECURITY_SECURITY_H */
