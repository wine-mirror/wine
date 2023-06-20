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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <dlfcn.h>
#ifdef SONAME_LIBGNUTLS
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#include <gnutls/abstract.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "sspi.h"
#include "secur32_priv.h"

#include "wine/unixlib.h"
#include "wine/debug.h"

#if defined(SONAME_LIBGNUTLS)

WINE_DEFAULT_DEBUG_CHANNEL(secur32);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

static const char *system_priority_file;

/* Not present in gnutls version < 2.9.10. */
static int (*pgnutls_cipher_get_block_size)(gnutls_cipher_algorithm_t);

/* Not present in gnutls version < 3.0. */
static void (*pgnutls_transport_set_pull_timeout_function)(gnutls_session_t,
                                                           int (*)(gnutls_transport_ptr_t, unsigned int));
static void (*pgnutls_dtls_set_mtu)(gnutls_session_t, unsigned int);
static void (*pgnutls_dtls_set_timeouts)(gnutls_session_t, unsigned int, unsigned int);

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
MAKE_FUNCPTR(gnutls_alert_send);
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
MAKE_FUNCPTR(gnutls_session_channel_binding);
MAKE_FUNCPTR(gnutls_set_default_priority);
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

#if GNUTLS_VERSION_MAJOR < 3 || (GNUTLS_VERSION_MAJOR == 3 && GNUTLS_VERSION_MINOR < 4)
#define GNUTLS_CIPHER_AES_128_CCM 19
#define GNUTLS_CIPHER_AES_256_CCM 20
#endif

#if GNUTLS_VERSION_MAJOR < 3 || (GNUTLS_VERSION_MAJOR == 3 && GNUTLS_VERSION_MINOR < 5)
#define GNUTLS_ALPN_SERVER_PRECEDENCE (1<<1)
#endif

static inline gnutls_session_t session_from_handle(UINT64 handle)
{
   return (gnutls_session_t)(ULONG_PTR)handle;
}

static inline gnutls_certificate_credentials_t certificate_creds_from_handle(UINT64 handle)
{
    return (gnutls_certificate_credentials_t)(ULONG_PTR)handle;
}

struct schan_buffers
{
    SIZE_T offset;
    SIZE_T limit;
    const SecBufferDesc *desc;
    int current_buffer_idx;
};

struct schan_transport
{
    gnutls_session_t session;
    struct schan_buffers in;
    struct schan_buffers out;
};

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

static void compat_gnutls_transport_set_pull_timeout_function(gnutls_session_t session,
                                                              int (*func)(gnutls_transport_ptr_t, unsigned int))
{
    FIXME("\n");
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

static void compat_gnutls_dtls_set_mtu(gnutls_session_t session, unsigned int mtu)
{
    FIXME("\n");
}

static void compat_gnutls_dtls_set_timeouts(gnutls_session_t session, unsigned int retrans_timeout,
        unsigned int total_timeout)
{
    FIXME("\n");
}

static void init_schan_buffers(struct schan_buffers *s, const PSecBufferDesc desc)
{
    s->offset = 0;
    s->limit = ~0UL;
    s->desc = desc;
    s->current_buffer_idx = -1;
}

static int get_next_buffer(struct schan_buffers *s)
{
    if (s->current_buffer_idx == -1)
        return s->desc->cBuffers ? 0 : -1;
    if (s->current_buffer_idx == s->desc->cBuffers - 1)
        return -1;
    return s->current_buffer_idx + 1;
}

static char *get_buffer(struct schan_buffers *s, SIZE_T *count)
{
    SIZE_T max_count;
    PSecBuffer buffer;

    if (!s->desc)
    {
        TRACE("No desc\n");
        return NULL;
    }

    if (s->current_buffer_idx == -1)
    {
        /* Initial buffer */
        int buffer_idx = get_next_buffer(s);
        if (buffer_idx == -1)
        {
            TRACE("No next buffer\n");
            return NULL;
        }
        s->current_buffer_idx = buffer_idx;
    }

    buffer = &s->desc->pBuffers[s->current_buffer_idx];
    TRACE("Using buffer %d: cbBuffer %d, BufferType %#x, pvBuffer %p\n",
          s->current_buffer_idx, (unsigned)buffer->cbBuffer, (unsigned)buffer->BufferType, buffer->pvBuffer);

    max_count = buffer->cbBuffer - s->offset;
    if (s->limit != ~0UL && s->limit < max_count)
        max_count = s->limit;

    while (!max_count)
    {
        int buffer_idx;

        buffer_idx = get_next_buffer(s);
        if (buffer_idx == -1)
        {
            TRACE("No next buffer\n");
            return NULL;
        }
        s->current_buffer_idx = buffer_idx;
        s->offset = 0;
        buffer = &s->desc->pBuffers[buffer_idx];
        max_count = buffer->cbBuffer;
        if (s->limit != ~0UL && s->limit < max_count)
            max_count = s->limit;
    }

    if (*count > max_count)
        *count = max_count;
    if (s->limit != ~0UL)
        s->limit -= *count;

    return (char *)buffer->pvBuffer + s->offset;
}

static ssize_t pull_adapter(gnutls_transport_ptr_t transport, void *buff, size_t buff_len)
{
    struct schan_transport *t = (struct schan_transport*)transport;
    SIZE_T len = buff_len;
    char *b;

    TRACE("Pull %lu bytes\n", len);

    b = get_buffer(&t->in, &len);
    if (!b)
    {
        pgnutls_transport_set_errno(t->session, EAGAIN);
        return -1;
    }
    memcpy(buff, b, len);
    t->in.offset += len;
    TRACE("Read %lu bytes\n", len);
    return len;
}

static ssize_t push_adapter(gnutls_transport_ptr_t transport, const void *buff, size_t buff_len)
{
    struct schan_transport *t = (struct schan_transport*)transport;
    SIZE_T len = buff_len;
    char *b;

    TRACE("Push %lu bytes\n", len);

    b = get_buffer(&t->out, &len);
    if (!b)
    {
        pgnutls_transport_set_errno(t->session, EAGAIN);
        return -1;
    }
    memcpy(b, buff, len);
    t->out.offset += len;
    TRACE("Wrote %lu bytes\n", len);
    return len;
}

struct protocol_priority_flag {
    DWORD enable_flag;
    const char *gnutls_flag;
};

static const struct protocol_priority_flag client_protocol_priority_flags[] = {
    {SP_PROT_DTLS1_2_CLIENT, "VERS-DTLS1.2"},
    {SP_PROT_DTLS1_0_CLIENT, "VERS-DTLS1.0"},
    {SP_PROT_TLS1_3_CLIENT, "VERS-TLS1.3"},
    {SP_PROT_TLS1_2_CLIENT, "VERS-TLS1.2"},
    {SP_PROT_TLS1_1_CLIENT, "VERS-TLS1.1"},
    {SP_PROT_TLS1_0_CLIENT, "VERS-TLS1.0"},
    {SP_PROT_SSL3_CLIENT,   "VERS-SSL3.0"}
    /* {SP_PROT_SSL2_CLIENT} is not supported by GnuTLS */
};

static const struct protocol_priority_flag server_protocol_priority_flags[] = {
    {SP_PROT_DTLS1_2_SERVER, "VERS-DTLS1.2"},
    {SP_PROT_DTLS1_0_SERVER, "VERS-DTLS1.0"},
    {SP_PROT_TLS1_3_SERVER, "VERS-TLS1.3"},
    {SP_PROT_TLS1_2_SERVER, "VERS-TLS1.2"},
    {SP_PROT_TLS1_1_SERVER, "VERS-TLS1.1"},
    {SP_PROT_TLS1_0_SERVER, "VERS-TLS1.0"},
    {SP_PROT_SSL3_SERVER,   "VERS-SSL3.0"}
    /* {SP_PROT_SSL2_SERVER} is not supported by GnuTLS */
};

static DWORD supported_protocols;

static void check_supported_protocols(
 const struct protocol_priority_flag *flags, int num_flags, BOOLEAN server)
{
    const char *type_desc = server ? "server" : "client";
    gnutls_session_t session;
    char priority[64];
    unsigned i;
    int err;

    err = pgnutls_init(&session, server ? GNUTLS_SERVER : GNUTLS_CLIENT);
    if (err != GNUTLS_E_SUCCESS)
    {
        pgnutls_perror(err);
        return;
    }

    for(i = 0; i < num_flags; i++)
    {
        sprintf(priority, "NORMAL:-%s", flags[i].gnutls_flag);
        err = pgnutls_priority_set_direct(session, priority, NULL);
        if (err == GNUTLS_E_SUCCESS)
        {
            TRACE("%s %s is supported\n", type_desc, flags[i].gnutls_flag);
            supported_protocols |= flags[i].enable_flag;
        }
        else
            TRACE("%s %s is not supported\n", type_desc, flags[i].gnutls_flag);
    }

    pgnutls_deinit(session);
}

static NTSTATUS schan_get_enabled_protocols( void *args )
{
    return supported_protocols;
}

static int pull_timeout(gnutls_transport_ptr_t transport, unsigned int timeout)
{
    struct schan_transport *t = (struct schan_transport*)transport;
    SIZE_T count = 0;

    TRACE("\n");

    if (get_buffer(&t->in, &count)) return 1;

    return 0;
}

static NTSTATUS set_priority(schan_credentials *cred, gnutls_session_t session)
{
    char priority[128] = "NORMAL:%LATEST_RECORD_VERSION", *p;
    BOOL server = !!(cred->credential_use & SECPKG_CRED_INBOUND);
    const struct protocol_priority_flag *protocols =
        server ? server_protocol_priority_flags : client_protocol_priority_flags;
    int num_protocols = server ? ARRAYSIZE(server_protocol_priority_flags)
                               : ARRAYSIZE(client_protocol_priority_flags);
    BOOL using_vers_all = FALSE, disabled;
    int i, err;

    if (system_priority_file && strcmp(system_priority_file, "/dev/null"))
    {
        TRACE("Using defaults with system priority file override\n");
        err = pgnutls_set_default_priority(session);
        if (err != GNUTLS_E_SUCCESS)
        {
            pgnutls_perror(err);
            return STATUS_INTERNAL_ERROR;
        }
        return STATUS_SUCCESS;
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

    for (i = 0; i < num_protocols; i++)
    {
        if (!(supported_protocols & protocols[i].enable_flag)) continue;

        disabled = !(cred->enabled_protocols & protocols[i].enable_flag);
        if (using_vers_all && disabled) continue;

        *p++ = ':';
        *p++ = disabled ? '-' : '+';
        strcpy(p, protocols[i].gnutls_flag);
        p += strlen(p);
    }

    TRACE("Using %s priority\n", debugstr_a(priority));
    err = pgnutls_priority_set_direct(session, priority, NULL);
    if (err != GNUTLS_E_SUCCESS)
    {
        pgnutls_perror(err);
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS schan_create_session( void *args )
{
    const struct create_session_params *params = args;
    schan_credentials *cred = params->cred;
    unsigned int flags = (cred->credential_use == SECPKG_CRED_INBOUND) ? GNUTLS_SERVER : GNUTLS_CLIENT;
    struct schan_transport *transport;
    gnutls_session_t s;
    NTSTATUS status;
    int err;

    *params->session = 0;

    if (cred->enabled_protocols & SP_PROT_DTLS1_X)
    {
        flags |= GNUTLS_DATAGRAM | GNUTLS_NONBLOCK;
    }

    err = pgnutls_init(&s, flags);
    if (err != GNUTLS_E_SUCCESS)
    {
        pgnutls_perror(err);
        return STATUS_INTERNAL_ERROR;
    }

    if (!(transport = calloc(1, sizeof(*transport))))
    {
        pgnutls_deinit(s);
        return STATUS_INTERNAL_ERROR;
    }
    transport->session = s;

    if ((status = set_priority(cred, s)))
    {
        pgnutls_deinit(s);
        free(transport);
        return status;
    }

    err = pgnutls_credentials_set(s, GNUTLS_CRD_CERTIFICATE, certificate_creds_from_handle(cred->credentials));
    if (err != GNUTLS_E_SUCCESS)
    {
        pgnutls_perror(err);
        pgnutls_deinit(s);
        free(transport);
        return STATUS_INTERNAL_ERROR;
    }

    pgnutls_transport_set_pull_function(s, pull_adapter);
    if (flags & GNUTLS_DATAGRAM) pgnutls_transport_set_pull_timeout_function(s, pull_timeout);
    pgnutls_transport_set_push_function(s, push_adapter);
    pgnutls_transport_set_ptr(s, (gnutls_transport_ptr_t)transport);
    *params->session = (ULONG_PTR)s;

    return STATUS_SUCCESS;
}

static NTSTATUS schan_dispose_session( void *args )
{
    const struct session_params *params = args;
    gnutls_session_t s = session_from_handle(params->session);
    struct schan_transport *t = (struct schan_transport *)pgnutls_transport_get_ptr(s);
    pgnutls_transport_set_ptr(s, NULL);
    pgnutls_deinit(s);
    free(t);
    return STATUS_SUCCESS;
}

static NTSTATUS schan_set_session_target( void *args )
{
    const struct set_session_target_params *params = args;
    gnutls_session_t s = session_from_handle(params->session);
    pgnutls_server_name_set( s, GNUTLS_NAME_DNS, params->target, strlen(params->target) );
    return STATUS_SUCCESS;
}

static gnutls_alert_level_t map_alert_type(unsigned int type)
{
    switch (type)
    {
    case TLS1_ALERT_WARNING: return GNUTLS_AL_WARNING;
    case TLS1_ALERT_FATAL:   return GNUTLS_AL_FATAL;
    default:
        FIXME( "unknown type %u\n", type );
        return -1;
    }
}

static gnutls_alert_description_t map_alert_number(unsigned int number)
{
    switch (number)
    {
    case TLS1_ALERT_CLOSE_NOTIFY:           return GNUTLS_A_CLOSE_NOTIFY;
    case TLS1_ALERT_UNEXPECTED_MESSAGE:     return GNUTLS_A_UNEXPECTED_MESSAGE;
    case TLS1_ALERT_BAD_RECORD_MAC:         return GNUTLS_A_BAD_RECORD_MAC;
    case TLS1_ALERT_DECRYPTION_FAILED:      return GNUTLS_A_DECRYPTION_FAILED;
    case TLS1_ALERT_RECORD_OVERFLOW:        return GNUTLS_A_RECORD_OVERFLOW;
    case TLS1_ALERT_DECOMPRESSION_FAIL:     return GNUTLS_A_DECOMPRESSION_FAILURE;
    case TLS1_ALERT_HANDSHAKE_FAILURE:      return GNUTLS_A_HANDSHAKE_FAILURE;
    case TLS1_ALERT_BAD_CERTIFICATE:        return GNUTLS_A_BAD_CERTIFICATE;
    case TLS1_ALERT_UNSUPPORTED_CERT:       return GNUTLS_A_UNSUPPORTED_CERTIFICATE;
    case TLS1_ALERT_CERTIFICATE_REVOKED:    return GNUTLS_A_CERTIFICATE_REVOKED;
    case TLS1_ALERT_CERTIFICATE_EXPIRED:    return GNUTLS_A_CERTIFICATE_EXPIRED;
    case TLS1_ALERT_CERTIFICATE_UNKNOWN:    return GNUTLS_A_CERTIFICATE_UNKNOWN;
    case TLS1_ALERT_ILLEGAL_PARAMETER:      return GNUTLS_A_ILLEGAL_PARAMETER;
    case TLS1_ALERT_UNKNOWN_CA:             return GNUTLS_A_UNKNOWN_CA;
    case TLS1_ALERT_ACCESS_DENIED:          return GNUTLS_A_ACCESS_DENIED;
    case TLS1_ALERT_DECODE_ERROR:           return GNUTLS_A_DECODE_ERROR;
    case TLS1_ALERT_DECRYPT_ERROR:          return GNUTLS_A_DECRYPT_ERROR;
    case TLS1_ALERT_EXPORT_RESTRICTION:     return GNUTLS_A_EXPORT_RESTRICTION;
    case TLS1_ALERT_PROTOCOL_VERSION:       return GNUTLS_A_PROTOCOL_VERSION;
    case TLS1_ALERT_INSUFFIENT_SECURITY:    return GNUTLS_A_INSUFFICIENT_SECURITY;
    case TLS1_ALERT_INTERNAL_ERROR:         return GNUTLS_A_INTERNAL_ERROR;
    case TLS1_ALERT_USER_CANCELED:          return GNUTLS_A_USER_CANCELED;
    case TLS1_ALERT_NO_RENEGOTIATION:       return GNUTLS_A_NO_RENEGOTIATION;
    case TLS1_ALERT_UNSUPPORTED_EXT:        return GNUTLS_A_UNSUPPORTED_EXTENSION;
    case TLS1_ALERT_UNKNOWN_PSK_IDENTITY:   return GNUTLS_A_UNKNOWN_PSK_IDENTITY;
    case TLS1_ALERT_NO_APP_PROTOCOL:        return GNUTLS_A_NO_APPLICATION_PROTOCOL;
    default:
        FIXME("unhandled alert %u\n", number);
        return -1;
    }
}

static NTSTATUS send_alert(gnutls_session_t session, unsigned int type, unsigned int number)
{
    gnutls_alert_level_t level = map_alert_type(type);
    gnutls_alert_description_t desc = map_alert_number(number);
    int ret;

    do
    {
        ret = pgnutls_alert_send(session, level, desc);
    }
    while (ret == GNUTLS_E_INTERRUPTED || ret == GNUTLS_E_AGAIN);

    if (ret < 0)
    {
        pgnutls_perror(ret);
        return SEC_E_INTERNAL_ERROR;
    }
    return SEC_E_OK;
}

static NTSTATUS schan_handshake( void *args )
{
    const struct handshake_params *params = args;
    gnutls_session_t s = session_from_handle(params->session);
    struct schan_transport *t = (struct schan_transport *)pgnutls_transport_get_ptr(s);
    NTSTATUS status;
    int err;

    init_schan_buffers(&t->in, params->input);
    t->in.limit = params->input_size;
    init_schan_buffers(&t->out, params->output);

    if (params->control_token)
    {
        status = send_alert(s, params->alert_type, params->alert_number);
        goto done;
    }

    while (1)
    {
        err = pgnutls_handshake(s);
        if (err == GNUTLS_E_SUCCESS)
        {
            TRACE("Handshake completed\n");
            status = SEC_E_OK;
        }
        else if (err == GNUTLS_E_AGAIN)
        {
            TRACE("Continue...\n");
            status = SEC_I_CONTINUE_NEEDED;
        }
        else if (err == GNUTLS_E_WARNING_ALERT_RECEIVED)
        {
            gnutls_alert_description_t alert = pgnutls_alert_get(s);

            WARN("WARNING ALERT: %d %s\n", alert, pgnutls_alert_get_name(alert));

            if (alert == GNUTLS_A_UNRECOGNIZED_NAME)
            {
                TRACE("Ignoring\n");
                continue;
            }
            else
                status = SEC_E_INTERNAL_ERROR;
        }
        else if (err == GNUTLS_E_FATAL_ALERT_RECEIVED)
        {
            gnutls_alert_description_t alert = pgnutls_alert_get(s);
            WARN("FATAL ALERT: %d %s\n", alert, pgnutls_alert_get_name(alert));
            status = SEC_E_INTERNAL_ERROR;
        }
        else
        {
            pgnutls_perror(err);
            status = SEC_E_INTERNAL_ERROR;
        }
        break;
    }

done:
    *params->input_offset = t->in.offset;
    *params->output_buffer_idx = t->out.current_buffer_idx;
    *params->output_offset = t->out.offset;

    return status;
}

static DWORD get_protocol(gnutls_protocol_t proto)
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
    case GNUTLS_DTLS1_0: return SP_PROT_DTLS1_0_CLIENT;
    case GNUTLS_DTLS1_2: return SP_PROT_DTLS1_2_CLIENT;
    default:
        FIXME("unknown protocol %d\n", proto);
        return 0;
    }
}

static ALG_ID get_cipher_algid(gnutls_cipher_algorithm_t cipher)
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

static ALG_ID get_mac_algid(gnutls_mac_algorithm_t mac, gnutls_cipher_algorithm_t cipher)
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

static ALG_ID get_kx_algid(int kx)
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

static NTSTATUS schan_get_session_cipher_block_size( void *args )
{
    const struct session_params *params = args;
    gnutls_session_t s = session_from_handle(params->session);
    return pgnutls_cipher_get_block_size(pgnutls_cipher_get(s));
}

static NTSTATUS schan_get_max_message_size( void *args )
{
    const struct session_params *params = args;
    gnutls_session_t s = session_from_handle(params->session);
    return pgnutls_record_get_max_size(s);
}

static NTSTATUS schan_get_connection_info( void *args )
{
    const struct get_connection_info_params *params = args;
    gnutls_session_t s = session_from_handle(params->session);
    SecPkgContext_ConnectionInfo *info = params->info;
    gnutls_protocol_t proto = pgnutls_protocol_get_version(s);
    gnutls_cipher_algorithm_t alg = pgnutls_cipher_get(s);
    gnutls_mac_algorithm_t mac = pgnutls_mac_get(s);
    gnutls_kx_algorithm_t kx = pgnutls_kx_get(s);

    info->dwProtocol = get_protocol(proto);
    info->aiCipher = get_cipher_algid(alg);
    info->dwCipherStrength = pgnutls_cipher_get_key_size(alg) * 8;
    info->aiHash = get_mac_algid(mac, alg);
    info->dwHashStrength = pgnutls_mac_get_key_size(mac) * 8;
    info->aiExch = get_kx_algid(kx);
    /* FIXME: info->dwExchStrength? */
    info->dwExchStrength = 0;
    return SEC_E_OK;
}

static DWORD get_protocol_version( gnutls_session_t session )
{
    gnutls_protocol_t proto = pgnutls_protocol_get_version( session );

    switch (proto)
    {
    case GNUTLS_SSL3:    return 0x300;
    case GNUTLS_TLS1_0:  return 0x301;
    case GNUTLS_TLS1_1:  return 0x302;
    case GNUTLS_TLS1_2:  return 0x303;
    case GNUTLS_DTLS1_0: return 0x201;
    case GNUTLS_DTLS1_2: return 0x202;
    default:
        FIXME( "unknown protocol %u\n", proto );
        return 0;
    }
}

static const WCHAR *get_cipher_str( gnutls_session_t session )
{
    static const WCHAR aesW[] = {'A','E','S',0};
    static const WCHAR unknownW[] = {'<','u','n','k','n','o','w','n','>',0};
    gnutls_cipher_algorithm_t cipher = pgnutls_cipher_get( session );

    switch (cipher)
    {
    case GNUTLS_CIPHER_AES_128_CBC:
    case GNUTLS_CIPHER_AES_192_CBC:
    case GNUTLS_CIPHER_AES_256_CBC:
    case GNUTLS_CIPHER_AES_128_GCM:
    case GNUTLS_CIPHER_AES_256_GCM:
    case GNUTLS_CIPHER_AES_128_CCM:
    case GNUTLS_CIPHER_AES_256_CCM:
        return aesW;
    default:
        FIXME( "unknown cipher %u\n", cipher );
        return unknownW;
    }
}

static DWORD get_cipher_len( gnutls_session_t session )
{
    gnutls_cipher_algorithm_t cipher = pgnutls_cipher_get( session );

    switch (cipher)
    {
    case GNUTLS_CIPHER_AES_128_CBC:
    case GNUTLS_CIPHER_AES_128_GCM:
    case GNUTLS_CIPHER_AES_128_CCM:
        return 128;
    case GNUTLS_CIPHER_AES_192_CBC:
        return 192;
    case GNUTLS_CIPHER_AES_256_CBC:
    case GNUTLS_CIPHER_AES_256_GCM:
    case GNUTLS_CIPHER_AES_256_CCM:
        return 256;
    default:
        FIXME( "unknown cipher %u\n", cipher );
        return 0;
    }
}

static DWORD get_cipher_block_len( gnutls_session_t session )
{
    gnutls_cipher_algorithm_t cipher = pgnutls_cipher_get( session );
    return pgnutls_cipher_get_block_size( cipher );
}

static const WCHAR *get_hash_str( gnutls_session_t session, BOOL full )
{
    static const WCHAR shaW[] = {'S','H','A',0};
    static const WCHAR sha1W[] = {'S','H','A','1',0};
    static const WCHAR sha224W[] = {'S','H','A','2','2','4',0};
    static const WCHAR sha256W[] = {'S','H','A','2','5','6',0};
    static const WCHAR sha384W[] = {'S','H','A','3','8','4',0};
    static const WCHAR sha512W[] = {'S','H','A','5','1','2',0};
    static const WCHAR unknownW[] = {'<','u','n','k','n','o','w','n','>',0};
    gnutls_mac_algorithm_t mac = pgnutls_mac_get( session );

    switch (mac)
    {
    case GNUTLS_MAC_SHA1:   return full ? sha1W : shaW;
    case GNUTLS_MAC_SHA224: return sha224W;
    case GNUTLS_MAC_SHA256: return sha256W;
    case GNUTLS_MAC_SHA384: return sha384W;
    case GNUTLS_MAC_SHA512: return sha512W;
    default:
        FIXME( "unknown mac %u\n", mac );
        return unknownW;
    }
}

static DWORD get_hash_len( gnutls_session_t session )
{
    gnutls_mac_algorithm_t mac = pgnutls_mac_get( session );
    return pgnutls_mac_get_key_size( mac ) * 8;
}

static const WCHAR *get_exchange_str( gnutls_session_t session, BOOL full )
{
    static const WCHAR ecdhW[] = {'E','C','D','H',0};
    static const WCHAR ecdheW[] = {'E','C','D','H','E',0};
    static const WCHAR unknownW[] = {'<','u','n','k','n','o','w','n','>',0};
    gnutls_kx_algorithm_t kx = pgnutls_kx_get( session );

    switch (kx)
    {
    case GNUTLS_KX_ECDHE_RSA:
    case GNUTLS_KX_ECDHE_ECDSA:
        return full ? ecdheW : ecdhW;
    default:
        FIXME( "unknown kx %u\n", kx );
        return unknownW;
    }
}

static const WCHAR *get_certificate_str( gnutls_session_t session )
{
    static const WCHAR rsaW[] = {'R','S','A',0};
    static const WCHAR ecdsaW[] = {'E','C','D','S','A',0};
    static const WCHAR unknownW[] = {'<','u','n','k','n','o','w','n','>',0};
    gnutls_kx_algorithm_t kx = pgnutls_kx_get( session );

    switch (kx)
    {
    case GNUTLS_KX_RSA:
    case GNUTLS_KX_RSA_EXPORT:
    case GNUTLS_KX_DHE_RSA:
    case GNUTLS_KX_ECDHE_RSA:   return rsaW;
    case GNUTLS_KX_ECDHE_ECDSA: return ecdsaW;
    default:
        FIXME( "unknown kx %u\n", kx );
        return unknownW;
    }
}

static const WCHAR *get_chaining_mode_str( gnutls_session_t session )
{
    static const WCHAR cbcW[] = {'C','B','C',0};
    static const WCHAR ccmW[] = {'C','C','M',0};
    static const WCHAR gcmW[] = {'G','C','M',0};
    static const WCHAR unknownW[] = {'<','u','n','k','n','o','w','n','>',0};
    gnutls_cipher_algorithm_t cipher = pgnutls_cipher_get( session );

    switch (cipher)
    {
    case GNUTLS_CIPHER_AES_128_CBC:
    case GNUTLS_CIPHER_AES_192_CBC:
    case GNUTLS_CIPHER_AES_256_CBC:
        return cbcW;
    case GNUTLS_CIPHER_AES_128_GCM:
    case GNUTLS_CIPHER_AES_256_GCM:
        return gcmW;
    case GNUTLS_CIPHER_AES_128_CCM:
    case GNUTLS_CIPHER_AES_256_CCM:
        return ccmW;
    default:
        FIXME( "unknown cipher %u\n", cipher );
        return unknownW;
    }
}

static NTSTATUS schan_get_cipher_info( void *args )
{
    const WCHAR tlsW[] = {'T','L','S','_',0};
    const WCHAR underscoreW[] = {'_',0};
    const WCHAR widthW[] = {'_','W','I','T','H','_',0};
    const struct get_cipher_info_params *params = args;
    gnutls_session_t session = session_from_handle( params->session );
    SecPkgContext_CipherInfo *info = params->info;
    char buf[11];
    WCHAR *ptr;
    int len;

    info->dwProtocol = get_protocol_version( session );
    info->dwCipherSuite = 0; /* FIXME */
    info->dwBaseCipherSuite = 0; /* FIXME */
    wcscpy( info->szCipher, get_cipher_str( session ) );
    info->dwCipherLen = get_cipher_len( session );
    info->dwCipherBlockLen = get_cipher_block_len( session );
    wcscpy( info->szHash, get_hash_str( session, TRUE ) );
    info->dwHashLen = get_hash_len( session );
    wcscpy( info->szExchange, get_exchange_str( session, FALSE ) );
    info->dwMinExchangeLen = 0;
    info->dwMaxExchangeLen = 65536;
    wcscpy( info->szCertificate, get_certificate_str( session ) );
    info->dwKeyType = 0; /* FIXME */

    wcscpy( info->szCipherSuite, tlsW );
    wcscat( info->szCipherSuite, get_exchange_str( session, TRUE ) );
    wcscat( info->szCipherSuite, underscoreW );
    wcscat( info->szCipherSuite, info->szCertificate );
    wcscat( info->szCipherSuite, widthW );
    wcscat( info->szCipherSuite, info->szCipher );
    wcscat( info->szCipherSuite, underscoreW );
    len = sprintf( buf, "%u", (unsigned int)info->dwCipherLen ) + 1;
    ptr = info->szCipherSuite + wcslen( info->szCipherSuite );
    ntdll_umbstowcs( buf, len, ptr, len );
    wcscat( info->szCipherSuite, underscoreW );
    wcscat( info->szCipherSuite, get_chaining_mode_str( session ) );
    wcscat( info->szCipherSuite, underscoreW );
    wcscat( info->szCipherSuite, get_hash_str( session, FALSE ) );
    return SEC_E_OK;
}

static NTSTATUS schan_get_unique_channel_binding( void *args )
{
    const struct get_unique_channel_binding_params *params = args;
    gnutls_session_t s = session_from_handle(params->session);
    gnutls_datum_t datum;
    int rc;
    SECURITY_STATUS ret;

    rc = pgnutls_session_channel_binding(s, GNUTLS_CB_TLS_UNIQUE, &datum);
    if (rc)
    {
        pgnutls_perror(rc);
        return SEC_E_INTERNAL_ERROR;
    }
    if (params->buffer && *params->bufsize >= datum.size)
    {
        memcpy( params->buffer, datum.data, datum.size );
        ret = SEC_E_OK;
    }
    else ret = SEC_E_BUFFER_TOO_SMALL;

    *params->bufsize = datum.size;
    free(datum.data);
    return ret;
}

static NTSTATUS schan_get_key_signature_algorithm( void *args )
{
    const struct session_params *params = args;
    gnutls_session_t s = session_from_handle(params->session);
    gnutls_kx_algorithm_t kx = pgnutls_kx_get(s);

    TRACE("(%p)\n", s);

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

static NTSTATUS schan_get_session_peer_certificate( void *args )
{
    const struct get_session_peer_certificate_params *params = args;
    gnutls_session_t s = session_from_handle(params->session);
    const gnutls_datum_t *datum;
    unsigned int i, size;
    BYTE *ptr;
    unsigned int count;
    ULONG *sizes;

    if (!(datum = pgnutls_certificate_get_peers(s, &count))) return SEC_E_INTERNAL_ERROR;

    size = count * sizeof(*sizes);
    for (i = 0; i < count; i++) size += datum[i].size;

    if (!params->buffer || *params->bufsize < size)
    {
        *params->bufsize = size;
        return SEC_E_BUFFER_TOO_SMALL;
    }
    sizes = (ULONG *)params->buffer;
    ptr = params->buffer + count * sizeof(*sizes);
    for (i = 0; i < count; i++)
    {
        sizes[i] = datum[i].size;
        memcpy(ptr, datum[i].data, datum[i].size);
        ptr += datum[i].size;
    }

    *params->bufsize = size;
    *params->retcount = count;
    return SEC_E_OK;
}

static NTSTATUS schan_send( void *args )
{
    const struct send_params *params = args;
    gnutls_session_t s = session_from_handle(params->session);
    struct schan_transport *t = (struct schan_transport *)pgnutls_transport_get_ptr(s);
    SSIZE_T ret, total = 0;

    init_schan_buffers(&t->out, params->output);

    for (;;)
    {
        ret = pgnutls_record_send(s, (const char *)params->buffer + total, params->length - total);
        if (ret >= 0)
        {
            total += ret;
            TRACE( "sent %ld now %ld/%u\n", ret, total, (unsigned)params->length );
            if (total == params->length) break;
        }
        else if (ret == GNUTLS_E_AGAIN)
        {
            SIZE_T count = 0;

            if (get_buffer(&t->out, &count)) continue;
            return SEC_I_CONTINUE_NEEDED;
        }
        else
        {
            pgnutls_perror(ret);
            return SEC_E_INTERNAL_ERROR;
        }
    }

    *params->output_buffer_idx = t->out.current_buffer_idx;
    *params->output_offset = t->out.offset;
    return SEC_E_OK;
}

static NTSTATUS schan_recv( void *args )
{
    const struct recv_params *params = args;
    gnutls_session_t s = session_from_handle(params->session);
    struct schan_transport *t = (struct schan_transport *)pgnutls_transport_get_ptr(s);
    size_t data_size = *params->length;
    size_t received = 0;
    ssize_t ret;
    SECURITY_STATUS status = SEC_E_OK;

    init_schan_buffers(&t->in, params->input);
    t->in.limit = params->input_size;

    while (received < data_size)
    {
        ret = pgnutls_record_recv(s, (char *)params->buffer + received, data_size - received);

        if (ret > 0) received += ret;
        else if (!ret) break;
        else if (ret == GNUTLS_E_AGAIN)
        {
            SIZE_T count = 0;

            if (!get_buffer(&t->in, &count)) break;
        }
        else if (ret == GNUTLS_E_REHANDSHAKE)
        {
            TRACE("Rehandshake requested\n");
            status = SEC_I_RENEGOTIATE;
            break;
        }
        else
        {
            pgnutls_perror(ret);
            return SEC_E_INTERNAL_ERROR;
        }
    }

    *params->length = received;
    return status;
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

static NTSTATUS schan_set_application_protocols( void *args )
{
    const struct set_application_protocols_params *params = args;
    gnutls_session_t s = session_from_handle(params->session);
    unsigned int extension_len, extension, count = 0, offset = 0;
    unsigned short list_len;
    gnutls_datum_t *protocols;
    int ret;

    if (sizeof(extension_len) > params->buflen) return STATUS_INVALID_PARAMETER;
    extension_len = *(unsigned int *)&params->buffer[offset];
    offset += sizeof(extension_len);

    if (offset + sizeof(extension) > params->buflen) return STATUS_INVALID_PARAMETER;
    extension = *(unsigned int *)&params->buffer[offset];
    if (extension != SecApplicationProtocolNegotiationExt_ALPN)
    {
        FIXME("extension %u not supported\n", extension);
        return STATUS_NOT_SUPPORTED;
    }
    offset += sizeof(extension);

    if (offset + sizeof(list_len) > params->buflen) return STATUS_INVALID_PARAMETER;
    list_len = *(unsigned short *)&params->buffer[offset];
    offset += sizeof(list_len);

    if (offset + list_len > params->buflen) return STATUS_INVALID_PARAMETER;
    count = parse_alpn_protocol_list(&params->buffer[offset], list_len, NULL);
    if (!count || !(protocols = malloc(count * sizeof(*protocols)))) return STATUS_NO_MEMORY;

    parse_alpn_protocol_list(&params->buffer[offset], list_len, protocols);
    if ((ret = pgnutls_alpn_set_protocols(s, protocols, count, GNUTLS_ALPN_SERVER_PRECEDENCE) < 0))
    {
        pgnutls_perror(ret);
    }

    free(protocols);
    return STATUS_SUCCESS;
}

static NTSTATUS schan_get_application_protocol( void *args )
{
    const struct get_application_protocol_params *params = args;
    gnutls_session_t s = session_from_handle(params->session);
    SecPkgContext_ApplicationProtocol *protocol = params->protocol;
    gnutls_datum_t selected;

    memset(protocol, 0, sizeof(*protocol));
    if (pgnutls_alpn_get_selected_protocol(s, &selected) < 0) return SEC_E_OK;

    if (selected.size <= sizeof(protocol->ProtocolId))
    {
        protocol->ProtoNegoStatus = SecApplicationProtocolNegotiationStatus_Success;
        protocol->ProtoNegoExt    = SecApplicationProtocolNegotiationExt_ALPN;
        protocol->ProtocolIdSize  = selected.size;
        memcpy(protocol->ProtocolId, selected.data, selected.size);
        TRACE("returning %s\n", wine_dbgstr_an((const char *)selected.data, selected.size));
    }
    return SEC_E_OK;
}

static NTSTATUS schan_set_dtls_mtu( void *args )
{
    const struct set_dtls_mtu_params *params = args;
    gnutls_session_t s = session_from_handle(params->session);

    pgnutls_dtls_set_mtu(s, params->mtu);
    TRACE("MTU set to %u\n", params->mtu);
    return SEC_E_OK;
}

static NTSTATUS schan_set_dtls_timeouts( void *args )
{
    const struct set_dtls_timeouts_params *params = args;
    gnutls_session_t s = session_from_handle(params->session);

    pgnutls_dtls_set_timeouts(s, params->retrans_timeout, params->total_timeout);
    return SEC_E_OK;
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

static gnutls_x509_privkey_t get_x509_key(ULONG key_size, const BYTE *key_blob)
{
    gnutls_privkey_t key = NULL;
    gnutls_x509_privkey_t x509key = NULL;
    gnutls_datum_t m, e, d, p, q, u, e1, e2;
    BYTE *ptr;
    RSAPUBKEY *rsakey;
    DWORD size = key_size;
    int ret;

    if (size < sizeof(BLOBHEADER)) return NULL;

    rsakey = (RSAPUBKEY *)(key_blob + sizeof(BLOBHEADER));
    TRACE("RSA key bitlen %u pubexp %u\n", (unsigned)rsakey->bitlen, (unsigned)rsakey->pubexp);

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
        return NULL;
    }

    if (((ret = pgnutls_privkey_import_rsa_raw(key, &m, &e, &d, &p, &q, &u, &e1, &e2)) < 0) ||
         (ret = pgnutls_privkey_export_x509(key, &x509key)) < 0)
    {
        pgnutls_perror(ret);
        pgnutls_privkey_deinit(key);
        return NULL;
    }

    return x509key;
}

static gnutls_x509_crt_t get_x509_crt(const struct allocate_certificate_credentials_params *params)
{
    gnutls_datum_t data;
    gnutls_x509_crt_t crt;
    int ret;

    if (params->cert_encoding != X509_ASN_ENCODING)
    {
        FIXME("encoding type %u not supported\n", (unsigned)params->cert_encoding);
        return NULL;
    }

    if ((ret = pgnutls_x509_crt_init(&crt)) < 0)
    {
        pgnutls_perror(ret);
        return NULL;
    }

    data.data = params->cert_blob;
    data.size = params->cert_size;
    if ((ret = pgnutls_x509_crt_import(crt, &data, GNUTLS_X509_FMT_DER)) < 0)
    {
        pgnutls_perror(ret);
        pgnutls_x509_crt_deinit(crt);
        return NULL;
    }

    return crt;
}

static NTSTATUS schan_allocate_certificate_credentials( void *args )
{
    const struct allocate_certificate_credentials_params *params = args;
    gnutls_certificate_credentials_t creds;
    gnutls_x509_crt_t crt;
    gnutls_x509_privkey_t key;
    int ret;

    ret = pgnutls_certificate_allocate_credentials(&creds);
    if (ret != GNUTLS_E_SUCCESS)
    {
        pgnutls_perror(ret);
        return STATUS_INTERNAL_ERROR;
    }

    if (!params->cert_blob)
    {
        params->c->credentials = (ULONG_PTR)creds;
        return STATUS_SUCCESS;
    }

    if (!(crt = get_x509_crt(params)))
    {
        pgnutls_certificate_free_credentials(creds);
        return STATUS_INTERNAL_ERROR;
    }

    if (!(key = get_x509_key(params->key_size, params->key_blob)))
    {
        pgnutls_x509_crt_deinit(crt);
        pgnutls_certificate_free_credentials(creds);
        return STATUS_INTERNAL_ERROR;
    }

    ret = pgnutls_certificate_set_x509_key(creds, &crt, 1, key);
    pgnutls_x509_privkey_deinit(key);
    pgnutls_x509_crt_deinit(crt);
    if (ret != GNUTLS_E_SUCCESS)
    {
        pgnutls_perror(ret);
        pgnutls_certificate_free_credentials(creds);
        return STATUS_INTERNAL_ERROR;
    }

    params->c->credentials = (ULONG_PTR)creds;
    return STATUS_SUCCESS;
}

static NTSTATUS schan_free_certificate_credentials( void *args )
{
    const struct free_certificate_credentials_params *params = args;
    pgnutls_certificate_free_credentials(certificate_creds_from_handle(params->c->credentials));
    return STATUS_SUCCESS;
}

static void gnutls_log(int level, const char *msg)
{
    TRACE("<%d> %s", level, msg);
}

static NTSTATUS process_attach( void *args )
{
    int ret;

    if ((system_priority_file = getenv("GNUTLS_SYSTEM_PRIORITY_FILE")))
    {
        TRACE("GNUTLS_SYSTEM_PRIORITY_FILE is %s.\n", debugstr_a(system_priority_file));
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
        return STATUS_DLL_NOT_FOUND;
    }

#define LOAD_FUNCPTR(f) \
    if (!(p##f = dlsym(libgnutls_handle, #f))) \
    { \
        ERR("Failed to load %s\n", #f); \
        goto fail; \
    }

    LOAD_FUNCPTR(gnutls_alert_get)
    LOAD_FUNCPTR(gnutls_alert_get_name)
    LOAD_FUNCPTR(gnutls_alert_send)
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
    LOAD_FUNCPTR(gnutls_session_channel_binding)
    LOAD_FUNCPTR(gnutls_set_default_priority)
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
    if (!(pgnutls_transport_set_pull_timeout_function = dlsym(libgnutls_handle, "gnutls_transport_set_pull_timeout_function")))
    {
        WARN("gnutls_transport_set_pull_timeout_function not found\n");
        pgnutls_transport_set_pull_timeout_function = compat_gnutls_transport_set_pull_timeout_function;
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
    if (!(pgnutls_dtls_set_mtu = dlsym(libgnutls_handle, "gnutls_dtls_set_mtu")))
    {
        WARN("gnutls_dtls_set_mtu not found\n");
        pgnutls_dtls_set_mtu = compat_gnutls_dtls_set_mtu;
    }
    if (!(pgnutls_dtls_set_timeouts = dlsym(libgnutls_handle, "gnutls_dtls_set_timeouts")))
    {
        WARN("gnutls_dtls_set_timeouts not found\n");
        pgnutls_dtls_set_timeouts = compat_gnutls_dtls_set_timeouts;
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
        pgnutls_global_set_log_function(gnutls_log);
    }

    check_supported_protocols(client_protocol_priority_flags, ARRAYSIZE(client_protocol_priority_flags), FALSE);
    check_supported_protocols(server_protocol_priority_flags, ARRAYSIZE(server_protocol_priority_flags), TRUE);
    return STATUS_SUCCESS;

fail:
    dlclose(libgnutls_handle);
    libgnutls_handle = NULL;
    return STATUS_DLL_NOT_FOUND;
}

static NTSTATUS process_detach( void *args )
{
    pgnutls_global_deinit();
    dlclose(libgnutls_handle);
    libgnutls_handle = NULL;
    return STATUS_SUCCESS;
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    process_attach,
    process_detach,
    schan_allocate_certificate_credentials,
    schan_create_session,
    schan_dispose_session,
    schan_free_certificate_credentials,
    schan_get_application_protocol,
    schan_get_cipher_info,
    schan_get_connection_info,
    schan_get_enabled_protocols,
    schan_get_key_signature_algorithm,
    schan_get_max_message_size,
    schan_get_session_cipher_block_size,
    schan_get_session_peer_certificate,
    schan_get_unique_channel_binding,
    schan_handshake,
    schan_recv,
    schan_send,
    schan_set_application_protocols,
    schan_set_dtls_mtu,
    schan_set_session_target,
    schan_set_dtls_timeouts,
};

#ifdef _WIN64

typedef ULONG PTR32;

typedef struct SecBufferDesc32
{
    ULONG ulVersion;
    ULONG cBuffers;
    PTR32 pBuffers;
} SecBufferDesc32;

typedef struct SecBuffer32
{
    ULONG cbBuffer;
    ULONG BufferType;
    PTR32 pvBuffer;
} SecBuffer32;

static NTSTATUS wow64_schan_allocate_certificate_credentials( void *args )
{
    struct
    {
        PTR32 c;
        ULONG cert_encoding;
        ULONG cert_size;
        PTR32 cert_blob;
        ULONG key_size;
        PTR32 key_blob;
    } const *params32 = args;
    struct allocate_certificate_credentials_params params =
    {
        ULongToPtr(params32->c),
        params32->cert_encoding,
        params32->cert_size,
        ULongToPtr(params32->cert_blob),
        params32->key_size,
        ULongToPtr(params32->key_blob),
    };
    return schan_allocate_certificate_credentials(&params);
}

static NTSTATUS wow64_schan_create_session( void *args )
{
    struct
    {
        PTR32 cred;
        PTR32 session;
    } const *params32 = args;
    struct create_session_params params =
    {
        ULongToPtr(params32->cred),
        ULongToPtr(params32->session),
    };
    return schan_create_session(&params);
}

static NTSTATUS wow64_schan_free_certificate_credentials( void *args )
{
    struct
    {
        PTR32 c;
    } const *params32 = args;
    struct free_certificate_credentials_params params =
    {
        ULongToPtr(params32->c),
    };
    return schan_free_certificate_credentials(&params);
}

static NTSTATUS wow64_schan_get_application_protocol( void *args )
{
    struct
    {
        schan_session session;
        PTR32 protocol;
    } const *params32 = args;
    struct get_application_protocol_params params =
    {
        params32->session,
        ULongToPtr(params32->protocol),
    };
    return schan_get_application_protocol(&params);
}

static NTSTATUS wow64_schan_get_connection_info( void *args )
{
    struct
    {
        schan_session session;
        PTR32 info;
    } const *params32 = args;
    struct get_connection_info_params params =
    {
        params32->session,
        ULongToPtr(params32->info),
    };
    return schan_get_connection_info(&params);
}

static NTSTATUS wow64_schan_get_cipher_info( void *args )
{
    struct
    {
        schan_session session;
        PTR32 info;
    } const *params32 = args;
    struct get_cipher_info_params params =
    {
        params32->session,
        ULongToPtr(params32->info),
    };
    return schan_get_cipher_info(&params);
}

static NTSTATUS wow64_schan_get_session_peer_certificate( void *args )
{
    struct
    {
        schan_session session;
        PTR32 buffer;
        PTR32 bufsize;
        PTR32 retcount;
    } const *params32 = args;
    struct get_session_peer_certificate_params params =
    {
        params32->session,
        ULongToPtr(params32->buffer),
        ULongToPtr(params32->bufsize),
        ULongToPtr(params32->retcount),
    };
    return schan_get_session_peer_certificate(&params);
}

static NTSTATUS wow64_schan_get_unique_channel_binding( void *args )
{
    struct
    {
        schan_session session;
        PTR32 buffer;
        PTR32 bufsize;
    } const *params32 = args;
    struct get_unique_channel_binding_params params =
    {
        params32->session,
        ULongToPtr(params32->buffer),
        ULongToPtr(params32->bufsize),
    };
    return schan_get_unique_channel_binding(&params);
}

static void secbufferdesc_32to64(const SecBufferDesc32 *desc32, SecBufferDesc *desc)
{
    unsigned int i;

    desc->ulVersion = desc32->ulVersion;
    desc->cBuffers = desc32->cBuffers;
    for (i = 0; i < desc->cBuffers; ++i)
    {
        SecBuffer32 *buffer32 = ULongToPtr(desc32->pBuffers + i * sizeof(*buffer32));
        desc->pBuffers[i].cbBuffer = buffer32->cbBuffer;
        desc->pBuffers[i].BufferType = buffer32->BufferType;
        desc->pBuffers[i].pvBuffer = ULongToPtr(buffer32->pvBuffer);
    }
}

static NTSTATUS wow64_schan_handshake( void *args )
{
    SecBuffer input_buffers[3];
    SecBufferDesc input = { 0, 0, input_buffers };
    SecBuffer output_buffers[3];
    SecBufferDesc output = { 0, 0, output_buffers };

    struct
    {
        schan_session session;
        PTR32 input;
        ULONG input_size;
        PTR32 output;
        PTR32 input_offset;
        PTR32 output_buffer_idx;
        PTR32 output_offset;
        enum control_token control_token;
        unsigned int alert_type;
        unsigned int alert_number;
    } const *params32 = args;
    struct handshake_params params =
    {
        params32->session,
        params32->input ? &input : NULL,
        params32->input_size,
        params32->output ? &output : NULL,
        ULongToPtr(params32->input_offset),
        ULongToPtr(params32->output_buffer_idx),
        ULongToPtr(params32->output_offset),
        params32->control_token,
        params32->alert_type,
        params32->alert_number,
    };
    if (params32->input)
    {
        SecBufferDesc32 *desc32 = ULongToPtr(params32->input);
        assert(desc32->cBuffers <= ARRAY_SIZE(input_buffers));
        secbufferdesc_32to64(desc32, &input);
    }
    if (params32->output)
    {
        SecBufferDesc32 *desc32 = ULongToPtr(params32->output);
        assert(desc32->cBuffers <= ARRAY_SIZE(output_buffers));
        secbufferdesc_32to64(desc32, &output);
    }
    return schan_handshake(&params);
}

static NTSTATUS wow64_schan_recv( void *args )
{
    SecBuffer buffers[3];
    SecBufferDesc input = { 0, 0, buffers };

    struct
    {
        schan_session session;
        PTR32 input;
        ULONG input_size;
        PTR32 buffer;
        PTR32 length;
    } const *params32 = args;
    struct recv_params params =
    {
        params32->session,
        params32->input ? &input : NULL,
        params32->input_size,
        ULongToPtr(params32->buffer),
        ULongToPtr(params32->length),
    };
    if (params32->input)
    {
        SecBufferDesc32 *desc32 = ULongToPtr(params32->input);
        assert(desc32->cBuffers <= ARRAY_SIZE(buffers));
        secbufferdesc_32to64(desc32, &input);
    }
    return schan_recv(&params);
}

static NTSTATUS wow64_schan_send( void *args )
{
    SecBuffer buffers[3];
    SecBufferDesc output = { 0, 0, buffers };

    struct
    {
        schan_session session;
        PTR32 output;
        PTR32 buffer;
        ULONG length;
        PTR32 output_buffer_idx;
        PTR32 output_offset;
    } const *params32 = args;
    struct send_params params =
    {
        params32->session,
        params32->output ? &output : NULL,
        ULongToPtr(params32->buffer),
        params32->length,
        ULongToPtr(params32->output_buffer_idx),
        ULongToPtr(params32->output_offset),
    };
    if (params32->output)
    {
        SecBufferDesc32 *desc32 = ULongToPtr(params32->output);
        assert(desc32->cBuffers <= ARRAY_SIZE(buffers));
        secbufferdesc_32to64(desc32, &output);
    }
    return schan_send(&params);
}

static NTSTATUS wow64_schan_set_application_protocols( void *args )
{
    struct
    {
        schan_session session;
        PTR32 buffer;
        unsigned int buflen;
    } const *params32 = args;
    struct set_application_protocols_params params =
    {
        params32->session,
        ULongToPtr(params32->buffer),
        params32->buflen,
    };
    return schan_set_application_protocols(&params);
}

static NTSTATUS wow64_schan_set_session_target( void *args )
{
    struct
    {
        schan_session session;
        PTR32 target;
    } const *params32 = args;
    struct set_session_target_params params =
    {
        params32->session,
        ULongToPtr(params32->target),
    };
    return schan_set_session_target(&params);
}

const unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
    process_attach,
    process_detach,
    wow64_schan_allocate_certificate_credentials,
    wow64_schan_create_session,
    schan_dispose_session,
    wow64_schan_free_certificate_credentials,
    wow64_schan_get_application_protocol,
    wow64_schan_get_cipher_info,
    wow64_schan_get_connection_info,
    schan_get_enabled_protocols,
    schan_get_key_signature_algorithm,
    schan_get_max_message_size,
    schan_get_session_cipher_block_size,
    wow64_schan_get_session_peer_certificate,
    wow64_schan_get_unique_channel_binding,
    wow64_schan_handshake,
    wow64_schan_recv,
    wow64_schan_send,
    wow64_schan_set_application_protocols,
    schan_set_dtls_mtu,
    wow64_schan_set_session_target,
    schan_set_dtls_timeouts,
};

#endif /* _WIN64 */

#endif /* SONAME_LIBGNUTLS */
