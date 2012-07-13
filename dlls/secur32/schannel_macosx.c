/*
 * Mac OS X Secure Transport implementation of the schannel (SSL/TLS) provider.
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
#ifdef HAVE_SECURITY_SECURITY_H
#include <Security/Security.h>
#define GetCurrentThread GetCurrentThread_Mac
#define LoadResource LoadResource_Mac
#include <CoreServices/CoreServices.h>
#undef GetCurrentThread
#undef LoadResource
#undef DPRINTF
#endif

#include "windef.h"
#include "winbase.h"
#include "sspi.h"
#include "schannel.h"
#include "secur32_priv.h"
#include "wine/debug.h"
#include "wine/library.h"

WINE_DEFAULT_DEBUG_CHANNEL(secur32);

#ifdef HAVE_SECURITY_SECURITY_H

#if MAC_OS_X_VERSION_MAX_ALLOWED < 1060
/* Defined in <Security/CipherSuite.h> in the 10.6 SDK or later. */
enum {
    TLS_ECDH_ECDSA_WITH_NULL_SHA           =	0xC001,
    TLS_ECDH_ECDSA_WITH_RC4_128_SHA        =	0xC002,
    TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA   =	0xC003,
    TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA    =	0xC004,
    TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA    =	0xC005,
    TLS_ECDHE_ECDSA_WITH_NULL_SHA          =	0xC006,
    TLS_ECDHE_ECDSA_WITH_RC4_128_SHA       =	0xC007,
    TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA  =	0xC008,
    TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA   =	0xC009,
    TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA   =	0xC00A,
    TLS_ECDH_RSA_WITH_NULL_SHA             =	0xC00B,
    TLS_ECDH_RSA_WITH_RC4_128_SHA          =	0xC00C,
    TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA     =	0xC00D,
    TLS_ECDH_RSA_WITH_AES_128_CBC_SHA      =	0xC00E,
    TLS_ECDH_RSA_WITH_AES_256_CBC_SHA      =	0xC00F,
    TLS_ECDHE_RSA_WITH_NULL_SHA            =	0xC010,
    TLS_ECDHE_RSA_WITH_RC4_128_SHA         =	0xC011,
    TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA    =	0xC012,
    TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA     =	0xC013,
    TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA     =	0xC014,
    TLS_ECDH_anon_WITH_NULL_SHA            =	0xC015,
    TLS_ECDH_anon_WITH_RC4_128_SHA         =	0xC016,
    TLS_ECDH_anon_WITH_3DES_EDE_CBC_SHA    =	0xC017,
    TLS_ECDH_anon_WITH_AES_128_CBC_SHA     =	0xC018,
    TLS_ECDH_anon_WITH_AES_256_CBC_SHA     =	0xC019,
};
#endif

struct mac_session {
    SSLContextRef context;
    struct schan_transport *transport;
};


enum {
    schan_proto_SSL,
    schan_proto_TLS,
};

enum {
    schan_kx_DH_anon_EXPORT,
    schan_kx_DH_anon,
    schan_kx_DH_DSS_EXPORT,
    schan_kx_DH_DSS,
    schan_kx_DH_RSA_EXPORT,
    schan_kx_DH_RSA,
    schan_kx_DHE_DSS_EXPORT,
    schan_kx_DHE_DSS,
    schan_kx_DHE_RSA_EXPORT,
    schan_kx_DHE_RSA,
    schan_kx_ECDH_anon,
    schan_kx_ECDH_ECDSA,
    schan_kx_ECDH_RSA,
    schan_kx_ECDHE_ECDSA,
    schan_kx_ECDHE_RSA,
    schan_kx_FORTEZZA_DMS,
    schan_kx_NULL,
    schan_kx_RSA_EXPORT,
    schan_kx_RSA,
};

enum {
    schan_enc_3DES_EDE_CBC,
    schan_enc_AES_128_CBC,
    schan_enc_AES_256_CBC,
    schan_enc_DES_CBC,
    schan_enc_DES40_CBC,
    schan_enc_FORTEZZA_CBC,
    schan_enc_IDEA_CBC,
    schan_enc_NULL,
    schan_enc_RC2_CBC,
    schan_enc_RC2_CBC_40,
    schan_enc_RC4_128,
    schan_enc_RC4_40,
};

enum {
    schan_mac_MD5,
    schan_mac_NULL,
    schan_mac_SHA,
};


struct cipher_suite {
    SSLCipherSuite suite;
    int protocol;
    int kx_alg;
    int enc_alg;
    int mac_alg;
};

/* This table corresponds to the enum in <Security/CipherSuite.h>. */
static const struct cipher_suite cipher_suites[] = {
#define CIPHER_SUITE(p, kx, enc, mac) { p##_##kx##_WITH_##enc##_##mac, schan_proto_##p, \
                                        schan_kx_##kx, schan_enc_##enc, schan_mac_##mac }
    CIPHER_SUITE(SSL, RSA, NULL, MD5),
    CIPHER_SUITE(SSL, RSA, NULL, MD5),
    CIPHER_SUITE(SSL, RSA, NULL, SHA),
    CIPHER_SUITE(SSL, RSA_EXPORT, RC4_40, MD5),
    CIPHER_SUITE(SSL, RSA, RC4_128, MD5),
    CIPHER_SUITE(SSL, RSA, RC4_128, SHA),
    CIPHER_SUITE(SSL, RSA_EXPORT, RC2_CBC_40, MD5),
    CIPHER_SUITE(SSL, RSA, IDEA_CBC, SHA),
    CIPHER_SUITE(SSL, RSA_EXPORT, DES40_CBC, SHA),
    CIPHER_SUITE(SSL, RSA, DES_CBC, SHA),
    CIPHER_SUITE(SSL, RSA, 3DES_EDE_CBC, SHA),
    CIPHER_SUITE(SSL, DH_DSS_EXPORT, DES40_CBC, SHA),
    CIPHER_SUITE(SSL, DH_DSS, DES_CBC, SHA),
    CIPHER_SUITE(SSL, DH_DSS, 3DES_EDE_CBC, SHA),
    CIPHER_SUITE(SSL, DH_RSA_EXPORT, DES40_CBC, SHA),
    CIPHER_SUITE(SSL, DH_RSA, DES_CBC, SHA),
    CIPHER_SUITE(SSL, DH_RSA, 3DES_EDE_CBC, SHA),
    CIPHER_SUITE(SSL, DHE_DSS_EXPORT, DES40_CBC, SHA),
    CIPHER_SUITE(SSL, DHE_DSS, DES_CBC, SHA),
    CIPHER_SUITE(SSL, DHE_DSS, 3DES_EDE_CBC, SHA),
    CIPHER_SUITE(SSL, DHE_RSA_EXPORT, DES40_CBC, SHA),
    CIPHER_SUITE(SSL, DHE_RSA, DES_CBC, SHA),
    CIPHER_SUITE(SSL, DHE_RSA, 3DES_EDE_CBC, SHA),
    CIPHER_SUITE(SSL, DH_anon_EXPORT, RC4_40, MD5),
    CIPHER_SUITE(SSL, DH_anon, RC4_128, MD5),
    CIPHER_SUITE(SSL, DH_anon_EXPORT, DES40_CBC, SHA),
    CIPHER_SUITE(SSL, DH_anon, DES_CBC, SHA),
    CIPHER_SUITE(SSL, DH_anon, 3DES_EDE_CBC, SHA),
    CIPHER_SUITE(SSL, FORTEZZA_DMS, NULL, SHA),
    CIPHER_SUITE(SSL, FORTEZZA_DMS, FORTEZZA_CBC, SHA),

    CIPHER_SUITE(TLS, RSA, AES_128_CBC, SHA),
    CIPHER_SUITE(TLS, DH_DSS, AES_128_CBC, SHA),
    CIPHER_SUITE(TLS, DH_RSA, AES_128_CBC, SHA),
    CIPHER_SUITE(TLS, DHE_DSS, AES_128_CBC, SHA),
    CIPHER_SUITE(TLS, DHE_RSA, AES_128_CBC, SHA),
    CIPHER_SUITE(TLS, DH_anon, AES_128_CBC, SHA),
    CIPHER_SUITE(TLS, RSA, AES_256_CBC, SHA),
    CIPHER_SUITE(TLS, DH_DSS, AES_256_CBC, SHA),
    CIPHER_SUITE(TLS, DH_RSA, AES_256_CBC, SHA),
    CIPHER_SUITE(TLS, DHE_DSS, AES_256_CBC, SHA),
    CIPHER_SUITE(TLS, DHE_RSA, AES_256_CBC, SHA),
    CIPHER_SUITE(TLS, DH_anon, AES_256_CBC, SHA),

    CIPHER_SUITE(TLS, ECDH_ECDSA, NULL, SHA),
    CIPHER_SUITE(TLS, ECDH_ECDSA, RC4_128, SHA),
    CIPHER_SUITE(TLS, ECDH_ECDSA, 3DES_EDE_CBC, SHA),
    CIPHER_SUITE(TLS, ECDH_ECDSA, AES_128_CBC, SHA),
    CIPHER_SUITE(TLS, ECDH_ECDSA, AES_256_CBC, SHA),
    CIPHER_SUITE(TLS, ECDHE_ECDSA, NULL, SHA),
    CIPHER_SUITE(TLS, ECDHE_ECDSA, RC4_128, SHA),
    CIPHER_SUITE(TLS, ECDHE_ECDSA, 3DES_EDE_CBC, SHA),
    CIPHER_SUITE(TLS, ECDHE_ECDSA, AES_128_CBC, SHA),
    CIPHER_SUITE(TLS, ECDHE_ECDSA, AES_256_CBC, SHA),
    CIPHER_SUITE(TLS, ECDH_RSA, NULL, SHA),
    CIPHER_SUITE(TLS, ECDH_RSA, RC4_128, SHA),
    CIPHER_SUITE(TLS, ECDH_RSA, 3DES_EDE_CBC, SHA),
    CIPHER_SUITE(TLS, ECDH_RSA, AES_128_CBC, SHA),
    CIPHER_SUITE(TLS, ECDH_RSA, AES_256_CBC, SHA),
    CIPHER_SUITE(TLS, ECDHE_RSA, NULL, SHA),
    CIPHER_SUITE(TLS, ECDHE_RSA, RC4_128, SHA),
    CIPHER_SUITE(TLS, ECDHE_RSA, 3DES_EDE_CBC, SHA),
    CIPHER_SUITE(TLS, ECDHE_RSA, AES_128_CBC, SHA),
    CIPHER_SUITE(TLS, ECDHE_RSA, AES_256_CBC, SHA),
    CIPHER_SUITE(TLS, ECDH_anon, NULL, SHA),
    CIPHER_SUITE(TLS, ECDH_anon, RC4_128, SHA),
    CIPHER_SUITE(TLS, ECDH_anon, 3DES_EDE_CBC, SHA),
    CIPHER_SUITE(TLS, ECDH_anon, AES_128_CBC, SHA),
    CIPHER_SUITE(TLS, ECDH_anon, AES_256_CBC, SHA),

    CIPHER_SUITE(SSL, RSA, RC2_CBC, MD5),
    CIPHER_SUITE(SSL, RSA, IDEA_CBC, MD5),
    CIPHER_SUITE(SSL, RSA, DES_CBC, MD5),
    CIPHER_SUITE(SSL, RSA, 3DES_EDE_CBC, MD5),
#undef CIPHER_SUITE
};


static const struct cipher_suite* get_cipher_suite(SSLCipherSuite cipher_suite)
{
    int i;
    for (i = 0; i < sizeof(cipher_suites)/sizeof(cipher_suites[0]); i++)
    {
        if (cipher_suites[i].suite == cipher_suite)
            return &cipher_suites[i];
    }

    return NULL;
}


static DWORD schan_get_session_protocol(struct mac_session* s)
{
    SSLProtocol protocol;
    OSStatus status;

    TRACE("(%p/%p)\n", s, s->context);

    status = SSLGetNegotiatedProtocolVersion(s->context, &protocol);
    if (status != noErr)
    {
        ERR("Failed to get session protocol: %ld\n", status);
        return 0;
    }

    TRACE("protocol %d\n", protocol);

    switch (protocol)
    {
    case kSSLProtocol2: return SP_PROT_SSL2_CLIENT;
    case kSSLProtocol3: return SP_PROT_SSL3_CLIENT;
    case kTLSProtocol1: return SP_PROT_TLS1_CLIENT;
    default:
        FIXME("unknown protocol %d\n", protocol);
        return 0;
    }
}

static ALG_ID schan_get_cipher_algid(const struct cipher_suite* c)
{
    TRACE("(%#x)\n", (unsigned int)c->suite);

    switch (c->enc_alg)
    {
    case schan_enc_3DES_EDE_CBC:    return CALG_3DES;
    case schan_enc_AES_128_CBC:     return CALG_AES_128;
    case schan_enc_AES_256_CBC:     return CALG_AES_256;
    case schan_enc_DES_CBC:         return CALG_DES;
    case schan_enc_DES40_CBC:       return CALG_DES;
    case schan_enc_NULL:            return 0;
    case schan_enc_RC2_CBC_40:      return CALG_RC2;
    case schan_enc_RC2_CBC:         return CALG_RC2;
    case schan_enc_RC4_128:         return CALG_RC4;
    case schan_enc_RC4_40:          return CALG_RC4;

    case schan_enc_FORTEZZA_CBC:
    case schan_enc_IDEA_CBC:
        FIXME("Don't know CALG for encryption algorithm %d, returning 0\n", c->enc_alg);
        return 0;

    default:
        FIXME("Unknown encryption algorithm %d for cipher suite %#x, returning 0\n", c->enc_alg, (unsigned int)c->suite);
        return 0;
    }
}

static unsigned int schan_get_cipher_key_size(const struct cipher_suite* c)
{
    TRACE("(%#x)\n", (unsigned int)c->suite);

    switch (c->enc_alg)
    {
    case schan_enc_3DES_EDE_CBC:    return 168;
    case schan_enc_AES_128_CBC:     return 128;
    case schan_enc_AES_256_CBC:     return 256;
    case schan_enc_DES_CBC:         return 56;
    case schan_enc_DES40_CBC:       return 40;
    case schan_enc_NULL:            return 0;
    case schan_enc_RC2_CBC_40:      return 40;
    case schan_enc_RC2_CBC:         return 128;
    case schan_enc_RC4_128:         return 128;
    case schan_enc_RC4_40:          return 40;

    case schan_enc_FORTEZZA_CBC:
    case schan_enc_IDEA_CBC:
        FIXME("Don't know key size for encryption algorithm %d, returning 0\n", c->enc_alg);
        return 0;

    default:
        FIXME("Unknown encryption algorithm %d for cipher suite %#x, returning 0\n", c->enc_alg, (unsigned int)c->suite);
        return 0;
    }
}

static ALG_ID schan_get_mac_algid(const struct cipher_suite* c)
{
    TRACE("(%#x)\n", (unsigned int)c->suite);

    switch (c->mac_alg)
    {
    case schan_mac_MD5:     return CALG_MD5;
    case schan_mac_NULL:    return 0;
    case schan_mac_SHA:     return CALG_SHA;

    default:
        FIXME("Unknown hashing algorithm %d for cipher suite %#x, returning 0\n", c->mac_alg, (unsigned)c->suite);
        return 0;
    }
}

static unsigned int schan_get_mac_key_size(const struct cipher_suite* c)
{
    TRACE("(%#x)\n", (unsigned int)c->suite);

    switch (c->mac_alg)
    {
    case schan_mac_MD5:     return 128;
    case schan_mac_NULL:    return 0;
    case schan_mac_SHA:     return 160;

    default:
        FIXME("Unknown hashing algorithm %d for cipher suite %#x, returning 0\n", c->mac_alg, (unsigned)c->suite);
        return 0;
    }
}

static ALG_ID schan_get_kx_algid(const struct cipher_suite* c)
{
    TRACE("(%#x)\n", (unsigned int)c->suite);

    switch (c->kx_alg)
    {
    case schan_kx_DHE_DSS_EXPORT:
    case schan_kx_DHE_DSS:
    case schan_kx_DHE_RSA_EXPORT:
    case schan_kx_DHE_RSA:          return CALG_DH_EPHEM;
    case schan_kx_ECDH_anon:
    case schan_kx_ECDH_ECDSA:
    case schan_kx_ECDH_RSA:
    case schan_kx_ECDHE_ECDSA:
    case schan_kx_ECDHE_RSA:        return CALG_ECDH;
    case schan_kx_NULL:             return 0;
    case schan_kx_RSA:              return CALG_RSA_KEYX;

    case schan_kx_DH_anon_EXPORT:
    case schan_kx_DH_anon:
    case schan_kx_DH_DSS_EXPORT:
    case schan_kx_DH_DSS:
    case schan_kx_DH_RSA_EXPORT:
    case schan_kx_DH_RSA:
    case schan_kx_FORTEZZA_DMS:
    case schan_kx_RSA_EXPORT:
        FIXME("Don't know CALG for key exchange algorithm %d for cipher suite %#x, returning 0\n", c->kx_alg, (unsigned)c->suite);
        return 0;

    default:
        FIXME("Unknown key exchange algorithm %d for cipher suite %#x, returning 0\n", c->kx_alg, (unsigned)c->suite);
        return 0;
    }
}


/* schan_pull_adapter
 *      Callback registered with SSLSetIOFuncs as the read function for a
 *      session.  Reads data from the session connection.  Conforms to the
 *      SSLReadFunc type.
 *
 *  transport - The session connection
 *  buff - The buffer into which to store the read data.  Must be at least
 *         *buff_len bytes in length.
 *  *buff_len - On input, the desired length to read.  On successful return,
 *              the number of bytes actually read.
 *
 *  Returns:
 *      noErr on complete success meaning the requested length was successfully
 *          read.
 *      errSSLWouldBlock when the requested length could not be read without
 *          blocking.  *buff_len indicates how much was actually read.  The
 *          caller should try again if/when they want to read more.
 *      errSSLClosedGraceful when the connection has closed and there's no
 *          more data to be read.
 *      other error code for failure.
 */
static OSStatus schan_pull_adapter(SSLConnectionRef transport, void *buff,
                                   SIZE_T *buff_len)
{
    struct mac_session *s = (struct mac_session*)transport;
    size_t requested = *buff_len;
    int status;
    OSStatus ret;

    TRACE("(%p/%p, %p, %p/%lu)\n", s, s->transport, buff, buff_len, *buff_len);

    status = schan_pull(s->transport, buff, buff_len);
    if (status == 0)
    {
        if (*buff_len == 0)
        {
            TRACE("Connection closed\n");
            ret = errSSLClosedGraceful;
        }
        else if (*buff_len < requested)
        {
            TRACE("Pulled %lu bytes before would block\n", *buff_len);
            ret = errSSLWouldBlock;
        }
        else
        {
            TRACE("Pulled %lu bytes\n", *buff_len);
            ret = noErr;
        }
    }
    else if (status == EAGAIN)
    {
        TRACE("Would block before being able to pull anything\n");
        ret = errSSLWouldBlock;
    }
    else
    {
        FIXME("Unknown status code from schan_pull: %d\n", status);
        ret = ioErr;
    }

    return ret;
}

/* schan_push_adapter
 *      Callback registered with SSLSetIOFuncs as the write function for a
 *      session.  Writes data to the session connection.  Conforms to the
 *      SSLWriteFunc type.
 *
 *  transport - The session connection
 *  buff - The buffer of data to write.  Must be at least *buff_len bytes in length.
 *  *buff_len - On input, the desired length to write.  On successful return,
 *              the number of bytes actually written.
 *
 *  Returns:
 *      noErr on complete or partial success; *buff_len indicates how much data
 *          was actually written, which may be less than requrested.
 *      errSSLWouldBlock when no data could be written without blocking.  The
 *          caller should try again.
 *      other error code for failure.
 */
static OSStatus schan_push_adapter(SSLConnectionRef transport, const void *buff,
                                       SIZE_T *buff_len)
{
    struct mac_session *s = (struct mac_session*)transport;
    int status;
    OSStatus ret;

    TRACE("(%p/%p, %p, %p/%lu)\n", s, s->transport, buff, buff_len, *buff_len);

    status = schan_push(s->transport, buff, buff_len);
    if (status == 0)
    {
        TRACE("Pushed %lu bytes\n", *buff_len);
        ret = noErr;
    }
    else if (status == EAGAIN)
    {
        TRACE("Would block before being able to push anything\n");
        ret = errSSLWouldBlock;
    }
    else
    {
        FIXME("Unknown status code from schan_push: %d\n", status);
        ret = ioErr;
    }

    return ret;
}


BOOL schan_imp_create_session(schan_imp_session *session, BOOL is_server,
                              schan_imp_certificate_credentials cred)
{
    struct mac_session *s;
    OSStatus status;

    TRACE("(%p, %d)\n", session, is_server);

    s = HeapAlloc(GetProcessHeap(), 0, sizeof(*s));
    if (!s)
        return FALSE;

    status = SSLNewContext(is_server, &s->context);
    if (status != noErr)
    {
        ERR("Failed to create session context: %ld\n", (long)status);
        goto fail;
    }

    status = SSLSetConnection(s->context, s);
    if (status != noErr)
    {
        ERR("Failed to set session connection: %ld\n", (long)status);
        goto fail;
    }

    status = SSLSetEnableCertVerify(s->context, FALSE);
    if (status != noErr)
    {
        ERR("Failed to disable certificate verification: %ld\n", (long)status);
        goto fail;
    }

    status = SSLSetProtocolVersionEnabled(s->context, kSSLProtocol2, FALSE);
    if (status != noErr)
    {
        ERR("Failed to disable SSL version 2: %ld\n", (long)status);
        goto fail;
    }

    status = SSLSetIOFuncs(s->context, schan_pull_adapter, schan_push_adapter);
    if (status != noErr)
    {
        ERR("Failed to set session I/O funcs: %ld\n", (long)status);
        goto fail;
    }

    TRACE("    -> %p/%p\n", s, s->context);

    *session = (schan_imp_session)s;
    return TRUE;

fail:
    HeapFree(GetProcessHeap(), 0, s);
    return FALSE;
}

void schan_imp_dispose_session(schan_imp_session session)
{
    struct mac_session *s = (struct mac_session*)session;
    OSStatus status;

    TRACE("(%p/%p)\n", s, s->context);

    status = SSLDisposeContext(s->context);
    if (status != noErr)
        ERR("Failed to dispose of session context: %ld\n", status);
    HeapFree(GetProcessHeap(), 0, s);
}

void schan_imp_set_session_transport(schan_imp_session session,
                                     struct schan_transport *t)
{
    struct mac_session *s = (struct mac_session*)session;

    TRACE("(%p/%p, %p)\n", s, s->context, t);

    s->transport = t;
}

SECURITY_STATUS schan_imp_handshake(schan_imp_session session)
{
    struct mac_session *s = (struct mac_session*)session;
    OSStatus status;

    TRACE("(%p/%p)\n", s, s->context);

    status = SSLHandshake(s->context);
    if (status == noErr)
    {
        TRACE("Handshake completed\n");
        return SEC_E_OK;
    }
    else if (status == errSSLWouldBlock)
    {
        TRACE("Continue...\n");
        return SEC_I_CONTINUE_NEEDED;
    }
    else if (errSecErrnoBase <= status && status <= errSecErrnoLimit)
    {
        ERR("Handshake failed: %s\n", strerror(status));
        return SEC_E_INTERNAL_ERROR;
    }
    else
    {
        ERR("Handshake failed: %ld\n", (long)status);
        cssmPerror("SSLHandshake", status);
        return SEC_E_INTERNAL_ERROR;
    }

    /* Never reached */
    return SEC_E_OK;
}

unsigned int schan_imp_get_session_cipher_block_size(schan_imp_session session)
{
    struct mac_session* s = (struct mac_session*)session;
    SSLCipherSuite cipherSuite;
    const struct cipher_suite* c;
    OSStatus status;

    TRACE("(%p/%p)\n", s, s->context);

    status = SSLGetNegotiatedCipher(s->context, &cipherSuite);
    if (status != noErr)
    {
        ERR("Failed to get session cipher suite: %ld\n", status);
        return 0;
    }

    c = get_cipher_suite(cipherSuite);
    if (!c)
    {
        ERR("Unknown session cipher suite: %#x\n", (unsigned int)cipherSuite);
        return 0;
    }

    switch (c->enc_alg)
    {
    case schan_enc_3DES_EDE_CBC:    return 64;
    case schan_enc_AES_128_CBC:     return 128;
    case schan_enc_AES_256_CBC:     return 128;
    case schan_enc_DES_CBC:         return 64;
    case schan_enc_DES40_CBC:       return 64;
    case schan_enc_NULL:            return 0;
    case schan_enc_RC2_CBC_40:      return 64;
    case schan_enc_RC2_CBC:         return 64;
    case schan_enc_RC4_128:         return 0;
    case schan_enc_RC4_40:          return 0;

    case schan_enc_FORTEZZA_CBC:
    case schan_enc_IDEA_CBC:
        FIXME("Don't know block size for encryption algorithm %d, returning 0\n", c->enc_alg);
        return 0;

    default:
        FIXME("Unknown encryption algorithm %d for cipher suite %#x, returning 0\n", c->enc_alg, (unsigned int)c->suite);
        return 0;
    }
}

unsigned int schan_imp_get_max_message_size(schan_imp_session session)
{
    FIXME("Returning 1 << 14.\n");
    return 1 << 14;
}

SECURITY_STATUS schan_imp_get_connection_info(schan_imp_session session,
                                              SecPkgContext_ConnectionInfo *info)
{
    struct mac_session* s = (struct mac_session*)session;
    SSLCipherSuite cipherSuite;
    const struct cipher_suite* c;
    OSStatus status;

    TRACE("(%p/%p, %p)\n", s, s->context, info);

    status = SSLGetNegotiatedCipher(s->context, &cipherSuite);
    if (status != noErr)
    {
        ERR("Failed to get session cipher suite: %ld\n", status);
        return SEC_E_INTERNAL_ERROR;
    }

    c = get_cipher_suite(cipherSuite);
    if (!c)
    {
        ERR("Unknown session cipher suite: %#x\n", (unsigned int)cipherSuite);
        return SEC_E_INTERNAL_ERROR;
    }

    info->dwProtocol = schan_get_session_protocol(s);
    info->aiCipher = schan_get_cipher_algid(c);
    info->dwCipherStrength = schan_get_cipher_key_size(c);
    info->aiHash = schan_get_mac_algid(c);
    info->dwHashStrength = schan_get_mac_key_size(c);
    info->aiExch = schan_get_kx_algid(c);
    /* FIXME: info->dwExchStrength? */
    info->dwExchStrength = 0;

    return SEC_E_OK;
}

#ifndef HAVE_SSLCOPYPEERCERTIFICATES
static void schan_imp_cf_release(const void *arg, void *ctx)
{
    CFRelease(arg);
}
#endif

SECURITY_STATUS schan_imp_get_session_peer_certificate(schan_imp_session session,
                                                       PCCERT_CONTEXT *cert)
{
    struct mac_session* s = (struct mac_session*)session;
    SECURITY_STATUS ret = SEC_E_INTERNAL_ERROR;
    CFArrayRef certs;
    OSStatus status;

    TRACE("(%p/%p, %p)\n", s, s->context, cert);

#ifdef HAVE_SSLCOPYPEERCERTIFICATES
    status = SSLCopyPeerCertificates(s->context, &certs);
#else
    status = SSLGetPeerCertificates(s->context, &certs);
#endif
    if (status == noErr && certs)
    {
        SecCertificateRef mac_cert;
        CFDataRef data;
        if (CFArrayGetCount(certs) &&
            (mac_cert = (SecCertificateRef)CFArrayGetValueAtIndex(certs, 0)) &&
            (SecKeychainItemExport(mac_cert, kSecFormatX509Cert, 0, NULL, &data) == noErr))
        {
            *cert = CertCreateCertificateContext(X509_ASN_ENCODING,
                    CFDataGetBytePtr(data), CFDataGetLength(data));
            if (*cert)
                ret = SEC_E_OK;
            else
            {
                ret = GetLastError();
                WARN("CertCreateCertificateContext failed: %x\n", ret);
            }
            CFRelease(data);
        }
        else
            WARN("Couldn't extract certificate data\n");
#ifndef HAVE_SSLCOPYPEERCERTIFICATES
        /* This is why SSLGetPeerCertificates was deprecated */
        CFArrayApplyFunction(certs, CFRangeMake(0, CFArrayGetCount(certs)),
                             schan_imp_cf_release, NULL);
#endif
        CFRelease(certs);
    }
    else
        WARN("SSLCopyPeerCertificates failed: %ld\n", (long)status);

    return ret;
}

SECURITY_STATUS schan_imp_send(schan_imp_session session, const void *buffer,
                               SIZE_T *length)
{
    struct mac_session* s = (struct mac_session*)session;
    OSStatus status;

    TRACE("(%p/%p, %p, %p/%lu)\n", s, s->context, buffer, length, *length);

    status = SSLWrite(s->context, buffer, *length, length);
    if (status == noErr)
        TRACE("Wrote %lu bytes\n", *length);
    else if (status == errSSLWouldBlock)
    {
        if (!*length)
        {
            TRACE("Would block before being able to write anything\n");
            return SEC_I_CONTINUE_NEEDED;
        }
        else
            TRACE("Wrote %lu bytes before would block\n", *length);
    }
    else
    {
        WARN("SSLWrite failed: %ld\n", (long)status);
        return SEC_E_INTERNAL_ERROR;
    }

    return SEC_E_OK;
}

SECURITY_STATUS schan_imp_recv(schan_imp_session session, void *buffer,
                               SIZE_T *length)
{
    struct mac_session* s = (struct mac_session*)session;
    OSStatus status;

    TRACE("(%p/%p, %p, %p/%lu)\n", s, s->context, buffer, length, *length);

    status = SSLRead(s->context, buffer, *length, length);
    if (status == noErr)
        TRACE("Read %lu bytes\n", *length);
    else if (status == errSSLWouldBlock)
    {
        if (!*length)
        {
            TRACE("Would block before being able to read anything\n");
            return SEC_I_CONTINUE_NEEDED;
        }
        else
            TRACE("Read %lu bytes before would block\n", *length);
    }
    else
    {
        WARN("SSLRead failed: %ld\n", (long)status);
        return SEC_E_INTERNAL_ERROR;
    }

    return SEC_E_OK;
}

BOOL schan_imp_allocate_certificate_credentials(schan_imp_certificate_credentials *c)
{
    /* The certificate is never really used for anything. */
    *c = NULL;
    return TRUE;
}

void schan_imp_free_certificate_credentials(schan_imp_certificate_credentials c)
{
}

BOOL schan_imp_init(void)
{
    TRACE("()\n");
    return TRUE;
}

void schan_imp_deinit(void)
{
    TRACE("()\n");
}

#endif /* HAVE_SECURITY_SECURITY_H */
