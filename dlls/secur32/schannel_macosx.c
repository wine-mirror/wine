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

#if 0
#pragma makedep unix
#endif

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <pthread.h>
#ifdef HAVE_SECURITY_SECURITY_H
#include <Security/Security.h>
#define GetCurrentThread GetCurrentThread_Mac
#define LoadResource LoadResource_Mac
#include <CoreServices/CoreServices.h>
#undef GetCurrentThread
#undef LoadResource
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "sspi.h"
#include "schannel.h"
#include "winternl.h"
#include "secur32_priv.h"
#include "wine/debug.h"

#ifdef HAVE_SECURITY_SECURITY_H

WINE_DEFAULT_DEBUG_CHANNEL(secur32);

static const struct schan_callbacks *callbacks;

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

#if MAC_OS_X_VERSION_MAX_ALLOWED < 1080
/* Defined in <Security/CipherSuite.h> in the 10.8 SDK or later. */
enum {
    TLS_NULL_WITH_NULL_NULL                   = 0x0000,
    TLS_RSA_WITH_NULL_MD5                     = 0x0001,
    TLS_RSA_WITH_NULL_SHA                     = 0x0002,
    TLS_RSA_WITH_RC4_128_MD5                  = 0x0004,
    TLS_RSA_WITH_RC4_128_SHA                  = 0x0005,
    TLS_RSA_WITH_3DES_EDE_CBC_SHA             = 0x000A,
    TLS_RSA_WITH_NULL_SHA256                  = 0x003B,
    TLS_RSA_WITH_AES_128_CBC_SHA256           = 0x003C,
    TLS_RSA_WITH_AES_256_CBC_SHA256           = 0x003D,
    TLS_DH_DSS_WITH_3DES_EDE_CBC_SHA          = 0x000D,
    TLS_DH_RSA_WITH_3DES_EDE_CBC_SHA          = 0x0010,
    TLS_DHE_DSS_WITH_3DES_EDE_CBC_SHA         = 0x0013,
    TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA         = 0x0016,
    TLS_DH_DSS_WITH_AES_128_CBC_SHA256        = 0x003E,
    TLS_DH_RSA_WITH_AES_128_CBC_SHA256        = 0x003F,
    TLS_DHE_DSS_WITH_AES_128_CBC_SHA256       = 0x0040,
    TLS_DHE_RSA_WITH_AES_128_CBC_SHA256       = 0x0067,
    TLS_DH_DSS_WITH_AES_256_CBC_SHA256        = 0x0068,
    TLS_DH_RSA_WITH_AES_256_CBC_SHA256        = 0x0069,
    TLS_DHE_DSS_WITH_AES_256_CBC_SHA256       = 0x006A,
    TLS_DHE_RSA_WITH_AES_256_CBC_SHA256       = 0x006B,
    TLS_DH_anon_WITH_RC4_128_MD5              = 0x0018,
    TLS_DH_anon_WITH_3DES_EDE_CBC_SHA         = 0x001B,
    TLS_DH_anon_WITH_AES_128_CBC_SHA256       = 0x006C,
    TLS_DH_anon_WITH_AES_256_CBC_SHA256       = 0x006D,
    TLS_RSA_WITH_AES_128_GCM_SHA256           = 0x009C,
    TLS_RSA_WITH_AES_256_GCM_SHA384           = 0x009D,
    TLS_DHE_RSA_WITH_AES_128_GCM_SHA256       = 0x009E,
    TLS_DHE_RSA_WITH_AES_256_GCM_SHA384       = 0x009F,
    TLS_DH_RSA_WITH_AES_128_GCM_SHA256        = 0x00A0,
    TLS_DH_RSA_WITH_AES_256_GCM_SHA384        = 0x00A1,
    TLS_DHE_DSS_WITH_AES_128_GCM_SHA256       = 0x00A2,
    TLS_DHE_DSS_WITH_AES_256_GCM_SHA384       = 0x00A3,
    TLS_DH_DSS_WITH_AES_128_GCM_SHA256        = 0x00A4,
    TLS_DH_DSS_WITH_AES_256_GCM_SHA384        = 0x00A5,
    TLS_DH_anon_WITH_AES_128_GCM_SHA256       = 0x00A6,
    TLS_DH_anon_WITH_AES_256_GCM_SHA384       = 0x00A7,
    TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256   = 0xC023,
    TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384   = 0xC024,
    TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256    = 0xC025,
    TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384    = 0xC026,
    TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256     = 0xC027,
    TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384     = 0xC028,
    TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256      = 0xC029,
    TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384      = 0xC02A,
    TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256   = 0xC02B,
    TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384   = 0xC02C,
    TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256    = 0xC02D,
    TLS_ECDH_ECDSA_WITH_AES_256_GCM_SHA384    = 0xC02E,
    TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256     = 0xC02F,
    TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384     = 0xC030,
    TLS_ECDH_RSA_WITH_AES_128_GCM_SHA256      = 0xC031,
    TLS_ECDH_RSA_WITH_AES_256_GCM_SHA384      = 0xC032,
    TLS_EMPTY_RENEGOTIATION_INFO_SCSV         = 0x00FF,
};

/* Defined in <Security/SecureTransport.h> in the 10.8 SDK or later. */
enum {
    kTLSProtocol11      = 7,    /* TLS 1.1 */
    kTLSProtocol12      = 8,    /* TLS 1.2 */
};
#endif

#if MAC_OS_X_VERSION_MAX_ALLOWED < 1090
/* Defined in <Security/CipherSuite.h> in the 10.9 SDK or later. */
enum {
    TLS_PSK_WITH_RC4_128_SHA                  = 0x008A,
    TLS_PSK_WITH_3DES_EDE_CBC_SHA             = 0x008B,
    TLS_PSK_WITH_AES_128_CBC_SHA              = 0x008C,
    TLS_PSK_WITH_AES_256_CBC_SHA              = 0x008D,
    TLS_DHE_PSK_WITH_RC4_128_SHA              = 0x008E,
    TLS_DHE_PSK_WITH_3DES_EDE_CBC_SHA         = 0x008F,
    TLS_DHE_PSK_WITH_AES_128_CBC_SHA          = 0x0090,
    TLS_DHE_PSK_WITH_AES_256_CBC_SHA          = 0x0091,
    TLS_RSA_PSK_WITH_RC4_128_SHA              = 0x0092,
    TLS_RSA_PSK_WITH_3DES_EDE_CBC_SHA         = 0x0093,
    TLS_RSA_PSK_WITH_AES_128_CBC_SHA          = 0x0094,
    TLS_RSA_PSK_WITH_AES_256_CBC_SHA          = 0x0095,
    TLS_PSK_WITH_NULL_SHA                     = 0x002C,
    TLS_DHE_PSK_WITH_NULL_SHA                 = 0x002D,
    TLS_RSA_PSK_WITH_NULL_SHA                 = 0x002E,
    TLS_PSK_WITH_AES_128_GCM_SHA256           = 0x00A8,
    TLS_PSK_WITH_AES_256_GCM_SHA384           = 0x00A9,
    TLS_DHE_PSK_WITH_AES_128_GCM_SHA256       = 0x00AA,
    TLS_DHE_PSK_WITH_AES_256_GCM_SHA384       = 0x00AB,
    TLS_RSA_PSK_WITH_AES_128_GCM_SHA256       = 0x00AC,
    TLS_RSA_PSK_WITH_AES_256_GCM_SHA384       = 0x00AD,
    TLS_PSK_WITH_AES_128_CBC_SHA256           = 0x00AE,
    TLS_PSK_WITH_AES_256_CBC_SHA384           = 0x00AF,
    TLS_PSK_WITH_NULL_SHA256                  = 0x00B0,
    TLS_PSK_WITH_NULL_SHA384                  = 0x00B1,
    TLS_DHE_PSK_WITH_AES_128_CBC_SHA256       = 0x00B2,
    TLS_DHE_PSK_WITH_AES_256_CBC_SHA384       = 0x00B3,
    TLS_DHE_PSK_WITH_NULL_SHA256              = 0x00B4,
    TLS_DHE_PSK_WITH_NULL_SHA384              = 0x00B5,
    TLS_RSA_PSK_WITH_AES_128_CBC_SHA256       = 0x00B6,
    TLS_RSA_PSK_WITH_AES_256_CBC_SHA384       = 0x00B7,
    TLS_RSA_PSK_WITH_NULL_SHA256              = 0x00B8,
    TLS_RSA_PSK_WITH_NULL_SHA384              = 0x00B9,
};
#endif

enum schan_mode {
    schan_mode_NONE,
    schan_mode_READ,
    schan_mode_WRITE,
    schan_mode_HANDSHAKE,
};

struct mac_session {
    SSLContextRef context;
    struct schan_transport *transport;
    enum schan_mode mode;
    pthread_mutex_t mutex;
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
    schan_kx_DHE_PSK,
    schan_kx_DHE_RSA_EXPORT,
    schan_kx_DHE_RSA,
    schan_kx_ECDH_anon,
    schan_kx_ECDH_ECDSA,
    schan_kx_ECDH_RSA,
    schan_kx_ECDHE_ECDSA,
    schan_kx_ECDHE_RSA,
    schan_kx_FORTEZZA_DMS,
    schan_kx_NULL,
    schan_kx_PSK,
    schan_kx_RSA_EXPORT,
    schan_kx_RSA_PSK,
    schan_kx_RSA,
};

enum {
    schan_enc_3DES_EDE_CBC,
    schan_enc_AES_128_CBC,
    schan_enc_AES_128_GCM,
    schan_enc_AES_256_CBC,
    schan_enc_AES_256_GCM,
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
    schan_mac_SHA256,
    schan_mac_SHA384,
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

    CIPHER_SUITE(TLS, NULL, NULL, NULL),
    CIPHER_SUITE(TLS, RSA, NULL, MD5),
    CIPHER_SUITE(TLS, RSA, NULL, SHA),
    CIPHER_SUITE(TLS, RSA, RC4_128, MD5),
    CIPHER_SUITE(TLS, RSA, RC4_128, SHA),
    CIPHER_SUITE(TLS, RSA, 3DES_EDE_CBC, SHA),
    CIPHER_SUITE(TLS, RSA, NULL, SHA256),
    CIPHER_SUITE(TLS, RSA, AES_128_CBC, SHA256),
    CIPHER_SUITE(TLS, RSA, AES_256_CBC, SHA256),
    CIPHER_SUITE(TLS, DH_DSS, 3DES_EDE_CBC, SHA),
    CIPHER_SUITE(TLS, DH_RSA, 3DES_EDE_CBC, SHA),
    CIPHER_SUITE(TLS, DHE_DSS, 3DES_EDE_CBC, SHA),
    CIPHER_SUITE(TLS, DHE_RSA, 3DES_EDE_CBC, SHA),
    CIPHER_SUITE(TLS, DH_DSS, AES_128_CBC, SHA256),
    CIPHER_SUITE(TLS, DH_RSA, AES_128_CBC, SHA256),
    CIPHER_SUITE(TLS, DHE_DSS, AES_128_CBC, SHA256),
    CIPHER_SUITE(TLS, DHE_RSA, AES_128_CBC, SHA256),
    CIPHER_SUITE(TLS, DH_DSS, AES_256_CBC, SHA256),
    CIPHER_SUITE(TLS, DH_RSA, AES_256_CBC, SHA256),
    CIPHER_SUITE(TLS, DHE_DSS, AES_256_CBC, SHA256),
    CIPHER_SUITE(TLS, DHE_RSA, AES_256_CBC, SHA256),
    CIPHER_SUITE(TLS, DH_anon, RC4_128, MD5),
    CIPHER_SUITE(TLS, DH_anon, 3DES_EDE_CBC, SHA),
    CIPHER_SUITE(TLS, DH_anon, AES_128_CBC, SHA256),
    CIPHER_SUITE(TLS, DH_anon, AES_256_CBC, SHA256),

    CIPHER_SUITE(TLS, PSK, RC4_128, SHA),
    CIPHER_SUITE(TLS, PSK, 3DES_EDE_CBC, SHA),
    CIPHER_SUITE(TLS, PSK, AES_128_CBC, SHA),
    CIPHER_SUITE(TLS, PSK, AES_256_CBC, SHA),
    CIPHER_SUITE(TLS, DHE_PSK, RC4_128, SHA),
    CIPHER_SUITE(TLS, DHE_PSK, 3DES_EDE_CBC, SHA),
    CIPHER_SUITE(TLS, DHE_PSK, AES_128_CBC, SHA),
    CIPHER_SUITE(TLS, DHE_PSK, AES_256_CBC, SHA),
    CIPHER_SUITE(TLS, RSA_PSK, RC4_128, SHA),
    CIPHER_SUITE(TLS, RSA_PSK, 3DES_EDE_CBC, SHA),
    CIPHER_SUITE(TLS, RSA_PSK, AES_128_CBC, SHA),
    CIPHER_SUITE(TLS, RSA_PSK, AES_256_CBC, SHA),
    CIPHER_SUITE(TLS, PSK, NULL, SHA),
    CIPHER_SUITE(TLS, DHE_PSK, NULL, SHA),
    CIPHER_SUITE(TLS, RSA_PSK, NULL, SHA),

    CIPHER_SUITE(TLS, RSA, AES_128_GCM, SHA256),
    CIPHER_SUITE(TLS, RSA, AES_256_GCM, SHA384),
    CIPHER_SUITE(TLS, DHE_RSA, AES_128_GCM, SHA256),
    CIPHER_SUITE(TLS, DHE_RSA, AES_256_GCM, SHA384),
    CIPHER_SUITE(TLS, DH_RSA, AES_128_GCM, SHA256),
    CIPHER_SUITE(TLS, DH_RSA, AES_256_GCM, SHA384),
    CIPHER_SUITE(TLS, DHE_DSS, AES_128_GCM, SHA256),
    CIPHER_SUITE(TLS, DHE_DSS, AES_256_GCM, SHA384),
    CIPHER_SUITE(TLS, DH_DSS, AES_128_GCM, SHA256),
    CIPHER_SUITE(TLS, DH_DSS, AES_256_GCM, SHA384),
    CIPHER_SUITE(TLS, DH_anon, AES_128_GCM, SHA256),
    CIPHER_SUITE(TLS, DH_anon, AES_256_GCM, SHA384),

    CIPHER_SUITE(TLS, PSK, AES_128_GCM, SHA256),
    CIPHER_SUITE(TLS, PSK, AES_256_GCM, SHA384),
    CIPHER_SUITE(TLS, DHE_PSK, AES_128_GCM, SHA256),
    CIPHER_SUITE(TLS, DHE_PSK, AES_256_GCM, SHA384),
    CIPHER_SUITE(TLS, RSA_PSK, AES_128_GCM, SHA256),
    CIPHER_SUITE(TLS, RSA_PSK, AES_256_GCM, SHA384),
    CIPHER_SUITE(TLS, PSK, AES_128_CBC, SHA256),
    CIPHER_SUITE(TLS, PSK, AES_256_CBC, SHA384),
    CIPHER_SUITE(TLS, PSK, NULL, SHA256),
    CIPHER_SUITE(TLS, PSK, NULL, SHA384),
    CIPHER_SUITE(TLS, DHE_PSK, AES_128_CBC, SHA256),
    CIPHER_SUITE(TLS, DHE_PSK, AES_256_CBC, SHA384),
    CIPHER_SUITE(TLS, DHE_PSK, NULL, SHA256),
    CIPHER_SUITE(TLS, DHE_PSK, NULL, SHA384),
    CIPHER_SUITE(TLS, RSA_PSK, AES_128_CBC, SHA256),
    CIPHER_SUITE(TLS, RSA_PSK, AES_256_CBC, SHA384),
    CIPHER_SUITE(TLS, RSA_PSK, NULL, SHA256),
    CIPHER_SUITE(TLS, RSA_PSK, NULL, SHA384),

    CIPHER_SUITE(TLS, ECDHE_ECDSA, AES_128_CBC, SHA256),
    CIPHER_SUITE(TLS, ECDHE_ECDSA, AES_256_CBC, SHA384),
    CIPHER_SUITE(TLS, ECDH_ECDSA, AES_128_CBC, SHA256),
    CIPHER_SUITE(TLS, ECDH_ECDSA, AES_256_CBC, SHA384),
    CIPHER_SUITE(TLS, ECDHE_RSA, AES_128_CBC, SHA256),
    CIPHER_SUITE(TLS, ECDHE_RSA, AES_256_CBC, SHA384),
    CIPHER_SUITE(TLS, ECDH_RSA, AES_128_CBC, SHA256),
    CIPHER_SUITE(TLS, ECDH_RSA, AES_256_CBC, SHA384),
    CIPHER_SUITE(TLS, ECDHE_ECDSA, AES_128_GCM, SHA256),
    CIPHER_SUITE(TLS, ECDHE_ECDSA, AES_256_GCM, SHA384),
    CIPHER_SUITE(TLS, ECDH_ECDSA, AES_128_GCM, SHA256),
    CIPHER_SUITE(TLS, ECDH_ECDSA, AES_256_GCM, SHA384),
    CIPHER_SUITE(TLS, ECDHE_RSA, AES_128_GCM, SHA256),
    CIPHER_SUITE(TLS, ECDHE_RSA, AES_256_GCM, SHA384),
    CIPHER_SUITE(TLS, ECDH_RSA, AES_128_GCM, SHA256),
    CIPHER_SUITE(TLS, ECDH_RSA, AES_256_GCM, SHA384),

    CIPHER_SUITE(SSL, RSA, RC2_CBC, MD5),
    CIPHER_SUITE(SSL, RSA, IDEA_CBC, MD5),
    CIPHER_SUITE(SSL, RSA, DES_CBC, MD5),
    CIPHER_SUITE(SSL, RSA, 3DES_EDE_CBC, MD5),
#undef CIPHER_SUITE
};


static const struct cipher_suite* get_cipher_suite(SSLCipherSuite cipher_suite)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(cipher_suites); i++)
    {
        if (cipher_suites[i].suite == cipher_suite)
            return &cipher_suites[i];
    }

    return NULL;
}


static DWORD get_session_protocol(struct mac_session* s)
{
    SSLProtocol protocol;
    int status;

    TRACE("(%p/%p)\n", s, s->context);

    status = SSLGetNegotiatedProtocolVersion(s->context, &protocol);
    if (status != noErr)
    {
        ERR("Failed to get session protocol: %d\n", status);
        return 0;
    }

    TRACE("protocol %d\n", protocol);

    switch (protocol)
    {
    case kSSLProtocol2:     return SP_PROT_SSL2_CLIENT;
    case kSSLProtocol3:     return SP_PROT_SSL3_CLIENT;
    case kTLSProtocol1:     return SP_PROT_TLS1_CLIENT;
    case kTLSProtocol11:    return SP_PROT_TLS1_1_CLIENT;
    case kTLSProtocol12:    return SP_PROT_TLS1_2_CLIENT;
    default:
        FIXME("unknown protocol %d\n", protocol);
        return 0;
    }
}

static ALG_ID get_cipher_algid(const struct cipher_suite* c)
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

    case schan_enc_AES_128_GCM:
    case schan_enc_AES_256_GCM:
    case schan_enc_FORTEZZA_CBC:
    case schan_enc_IDEA_CBC:
        FIXME("Don't know CALG for encryption algorithm %d, returning 0\n", c->enc_alg);
        return 0;

    default:
        FIXME("Unknown encryption algorithm %d for cipher suite %#x, returning 0\n", c->enc_alg, (unsigned int)c->suite);
        return 0;
    }
}

static unsigned int get_cipher_key_size(const struct cipher_suite* c)
{
    TRACE("(%#x)\n", (unsigned int)c->suite);

    switch (c->enc_alg)
    {
    case schan_enc_3DES_EDE_CBC:    return 168;
    case schan_enc_AES_128_CBC:     return 128;
    case schan_enc_AES_128_GCM:     return 128;
    case schan_enc_AES_256_CBC:     return 256;
    case schan_enc_AES_256_GCM:     return 256;
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

static ALG_ID get_mac_algid(const struct cipher_suite* c)
{
    TRACE("(%#x)\n", (unsigned int)c->suite);

    switch (c->mac_alg)
    {
    case schan_mac_MD5:     return CALG_MD5;
    case schan_mac_NULL:    return 0;
    case schan_mac_SHA:     return CALG_SHA;
    case schan_mac_SHA256:  return CALG_SHA_256;
    case schan_mac_SHA384:  return CALG_SHA_384;

    default:
        FIXME("Unknown hashing algorithm %d for cipher suite %#x, returning 0\n", c->mac_alg, (unsigned)c->suite);
        return 0;
    }
}

static unsigned int get_mac_key_size(const struct cipher_suite* c)
{
    TRACE("(%#x)\n", (unsigned int)c->suite);

    switch (c->mac_alg)
    {
    case schan_mac_MD5:     return 128;
    case schan_mac_NULL:    return 0;
    case schan_mac_SHA:     return 160;
    case schan_mac_SHA256:  return 256;
    case schan_mac_SHA384:  return 384;

    default:
        FIXME("Unknown hashing algorithm %d for cipher suite %#x, returning 0\n", c->mac_alg, (unsigned)c->suite);
        return 0;
    }
}

static ALG_ID get_kx_algid(const struct cipher_suite* c)
{
    TRACE("(%#x)\n", (unsigned int)c->suite);

    switch (c->kx_alg)
    {
    case schan_kx_DHE_DSS_EXPORT:
    case schan_kx_DHE_DSS:
    case schan_kx_DHE_PSK:
    case schan_kx_DHE_RSA_EXPORT:
    case schan_kx_DHE_RSA:          return CALG_DH_EPHEM;
    case schan_kx_ECDH_anon:
    case schan_kx_ECDH_ECDSA:
    case schan_kx_ECDH_RSA:         return CALG_ECDH;
    case schan_kx_ECDHE_ECDSA:
    case schan_kx_ECDHE_RSA:        return CALG_ECDH_EPHEM;
    case schan_kx_NULL:             return 0;
    case schan_kx_RSA:
    case schan_kx_RSA_EXPORT:
    case schan_kx_RSA_PSK:          return CALG_RSA_KEYX;

    case schan_kx_DH_anon_EXPORT:
    case schan_kx_DH_anon:
    case schan_kx_DH_DSS_EXPORT:
    case schan_kx_DH_DSS:
    case schan_kx_DH_RSA_EXPORT:
    case schan_kx_DH_RSA:
    case schan_kx_FORTEZZA_DMS:
    case schan_kx_PSK:
        FIXME("Don't know CALG for key exchange algorithm %d for cipher suite %#x, returning 0\n", c->kx_alg, (unsigned)c->suite);
        return 0;

    default:
        FIXME("Unknown key exchange algorithm %d for cipher suite %#x, returning 0\n", c->kx_alg, (unsigned)c->suite);
        return 0;
    }
}


/* pull_adapter
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
static OSStatus pull_adapter(SSLConnectionRef transport, void *buff, SIZE_T *buff_len)
{
    struct mac_session *s = (struct mac_session*)transport;
    size_t requested = *buff_len;
    int status;
    OSStatus ret;

    TRACE("(%p/%p, %p, %p/%lu)\n", s, s->transport, buff, buff_len, *buff_len);

    if (s->mode != schan_mode_READ && s->mode != schan_mode_HANDSHAKE)
    {
        WARN("called in mode %u\n", s->mode);
        return noErr;
    }

    status = callbacks->pull(s->transport, buff, buff_len);
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

/* push_adapter
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
 *          was actually written, which may be less than requested.
 *      errSSLWouldBlock when no data could be written without blocking.  The
 *          caller should try again.
 *      other error code for failure.
 */
static OSStatus push_adapter(SSLConnectionRef transport, const void *buff, SIZE_T *buff_len)
{
    struct mac_session *s = (struct mac_session*)transport;
    int status;
    OSStatus ret;

    TRACE("(%p/%p, %p, %p/%lu)\n", s, s->transport, buff, buff_len, *buff_len);

    if (s->mode != schan_mode_WRITE && s->mode != schan_mode_HANDSHAKE)
    {
        WARN("called in mode %u\n", s->mode);
        return noErr;
    }

    status = callbacks->push(s->transport, buff, buff_len);
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

static const struct {
    DWORD enable_flag;
    SSLProtocol mac_version;
} protocol_priority_flags[] = {
    {SP_PROT_TLS1_2_CLIENT, kTLSProtocol12},
    {SP_PROT_TLS1_1_CLIENT, kTLSProtocol11},
    {SP_PROT_TLS1_0_CLIENT, kTLSProtocol1},
    {SP_PROT_SSL3_CLIENT,   kSSLProtocol3},
    {SP_PROT_SSL2_CLIENT,   kSSLProtocol2}
};

static DWORD supported_protocols;

static DWORD CDECL schan_get_enabled_protocols(void)
{
    return supported_protocols;
}

static BOOL CDECL schan_create_session(schan_session *session, schan_credentials *cred)
{
    struct mac_session *s;
    unsigned i;
    int status;

    TRACE("(%p)\n", session);

    if (!(s = RtlAllocateHeap(GetProcessHeap(), 0, sizeof(*s)))) return FALSE;

    pthread_mutex_init(&s->mutex, NULL);

    status = SSLNewContext(cred->credential_use == SECPKG_CRED_INBOUND, &s->context);
    if (status != noErr)
    {
        ERR("Failed to create session context: %d\n", status);
        goto fail;
    }

    status = SSLSetConnection(s->context, s);
    if (status != noErr)
    {
        ERR("Failed to set session connection: %d\n", status);
        goto fail;
    }

    status = SSLSetEnableCertVerify(s->context, FALSE);
    if (status != noErr)
    {
        ERR("Failed to disable certificate verification: %d\n", status);
        goto fail;
    }

    for(i = 0; i < ARRAY_SIZE(protocol_priority_flags); i++) {
        if(!(protocol_priority_flags[i].enable_flag & supported_protocols))
           continue;

        status = SSLSetProtocolVersionEnabled(s->context, protocol_priority_flags[i].mac_version,
                (cred->enabled_protocols & protocol_priority_flags[i].enable_flag) != 0);
        if (status != noErr)
        {
            ERR("Failed to set SSL version %d: %d\n", protocol_priority_flags[i].mac_version, status);
            goto fail;
        }
    }

    status = SSLSetIOFuncs(s->context, pull_adapter, push_adapter);
    if (status != noErr)
    {
        ERR("Failed to set session I/O funcs: %d\n", status);
        goto fail;
    }

    s->mode = schan_mode_NONE;

    TRACE("    -> %p/%p\n", s, s->context);

    *session = (schan_session)s;
    return TRUE;

fail:
    RtlFreeHeap(GetProcessHeap(), 0, s);
    return FALSE;
}

static void CDECL schan_dispose_session(schan_session session)
{
    struct mac_session *s = (struct mac_session*)session;
    int status;

    TRACE("(%p/%p)\n", s, s->context);

    status = SSLDisposeContext(s->context);
    if (status != noErr)
        ERR("Failed to dispose of session context: %d\n", status);
    pthread_mutex_destroy(&s->mutex);
    RtlFreeHeap(GetProcessHeap(), 0, s);
}

static void CDECL schan_set_session_transport(schan_session session, struct schan_transport *t)
{
    struct mac_session *s = (struct mac_session*)session;

    TRACE("(%p/%p, %p)\n", s, s->context, t);

    s->transport = t;
}

static void CDECL schan_set_session_target(schan_session session, const char *target)
{
    struct mac_session *s = (struct mac_session*)session;

    TRACE("(%p/%p, %s)\n", s, s->context, debugstr_a(target));

    SSLSetPeerDomainName( s->context, target, strlen(target) );
}

static SECURITY_STATUS CDECL schan_handshake(schan_session session)
{
    struct mac_session *s = (struct mac_session*)session;
    int status;

    TRACE("(%p/%p)\n", s, s->context);

    s->mode = schan_mode_HANDSHAKE;
    status = SSLHandshake(s->context);
    s->mode = schan_mode_NONE;

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
        ERR("Handshake failed: %d\n", status);
        cssmPerror("SSLHandshake", status);
        return SEC_E_INTERNAL_ERROR;
    }

    /* Never reached */
    return SEC_E_OK;
}

static unsigned int CDECL schan_get_session_cipher_block_size(schan_session session)
{
    struct mac_session* s = (struct mac_session*)session;
    SSLCipherSuite cipherSuite;
    const struct cipher_suite* c;
    int status;

    TRACE("(%p/%p)\n", s, s->context);

    status = SSLGetNegotiatedCipher(s->context, &cipherSuite);
    if (status != noErr)
    {
        ERR("Failed to get session cipher suite: %d\n", status);
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
    case schan_enc_AES_128_GCM:     return 128;
    case schan_enc_AES_256_CBC:     return 128;
    case schan_enc_AES_256_GCM:     return 128;
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

static unsigned int CDECL schan_get_max_message_size(schan_session session)
{
    FIXME("Returning 1 << 14.\n");
    return 1 << 14;
}

static ALG_ID CDECL schan_get_key_signature_algorithm(schan_session session)
{
    struct mac_session* s = (struct mac_session*)session;
    SSLCipherSuite cipherSuite;
    const struct cipher_suite* c;
    int status;

    TRACE("(%p/%p)\n", s, s->context);

    status = SSLGetNegotiatedCipher(s->context, &cipherSuite);
    if (status != noErr)
    {
        ERR("Failed to get session cipher suite: %d\n", status);
        return 0;
    }

    c = get_cipher_suite(cipherSuite);
    if (!c)
    {
        ERR("Unknown session cipher suite: %#x\n", (unsigned int)cipherSuite);
        return 0;
    }

    switch (c->kx_alg)
    {
    case schan_kx_DH_DSS_EXPORT:
    case schan_kx_DH_DSS:
    case schan_kx_DHE_DSS_EXPORT:
    case schan_kx_DHE_DSS:
        return CALG_DSS_SIGN;

    case schan_kx_DH_RSA_EXPORT:
    case schan_kx_DH_RSA:
    case schan_kx_DHE_RSA_EXPORT:
    case schan_kx_DHE_RSA:
    case schan_kx_ECDH_RSA:
    case schan_kx_ECDHE_RSA:
    case schan_kx_RSA_EXPORT:
    case schan_kx_RSA:
        return CALG_RSA_SIGN;

    case schan_kx_ECDH_ECDSA:
    case schan_kx_ECDHE_ECDSA:
        return CALG_ECDSA;

    case schan_kx_DH_anon_EXPORT:
    case schan_kx_DH_anon:
    case schan_kx_DHE_PSK:
    case schan_kx_ECDH_anon:
    case schan_kx_FORTEZZA_DMS:
    case schan_kx_NULL:
    case schan_kx_PSK:
    case schan_kx_RSA_PSK:
        FIXME("Don't know key signature algorithm for key exchange algorithm %d, returning 0\n", c->kx_alg);
        return 0;

    default:
        FIXME("Unknown key exchange algorithm %d for cipher suite %#x, returning 0\n", c->kx_alg, (unsigned int)c->suite);
        return 0;
    }
}

static SECURITY_STATUS CDECL schan_get_connection_info(schan_session session, SecPkgContext_ConnectionInfo *info)
{
    struct mac_session* s = (struct mac_session*)session;
    SSLCipherSuite cipherSuite;
    const struct cipher_suite* c;
    int status;

    TRACE("(%p/%p, %p)\n", s, s->context, info);

    status = SSLGetNegotiatedCipher(s->context, &cipherSuite);
    if (status != noErr)
    {
        ERR("Failed to get session cipher suite: %d\n", status);
        return SEC_E_INTERNAL_ERROR;
    }

    c = get_cipher_suite(cipherSuite);
    if (!c)
    {
        ERR("Unknown session cipher suite: %#x\n", (unsigned int)cipherSuite);
        return SEC_E_INTERNAL_ERROR;
    }

    info->dwProtocol = get_session_protocol(s);
    info->aiCipher = get_cipher_algid(c);
    info->dwCipherStrength = get_cipher_key_size(c);
    info->aiHash = get_mac_algid(c);
    info->dwHashStrength = get_mac_key_size(c);
    info->aiExch = get_kx_algid(c);
    /* FIXME: info->dwExchStrength? */
    info->dwExchStrength = 0;

    return SEC_E_OK;
}

static SECURITY_STATUS CDECL schan_get_unique_channel_binding(schan_session session, SecPkgContext_Bindings *bindings)
{
    FIXME("SECPKG_ATTR_UNIQUE_BINDINGS is unsupported on MacOS\n");
    return SEC_E_UNSUPPORTED_FUNCTION;
}

#ifndef HAVE_SSLCOPYPEERCERTIFICATES
static void cf_release(const void *arg, void *ctx)
{
    CFRelease(arg);
}
#endif

static SECURITY_STATUS CDECL schan_get_session_peer_certificate(schan_session session, struct schan_cert_list *list)
{
    struct mac_session *s = (struct mac_session *)session;
    SECURITY_STATUS ret = SEC_E_OK;
    SecCertificateRef cert;
    CFArrayRef cert_array;
    int status;
    unsigned int size;
    CFIndex i;
    CFDataRef data;
    BYTE *ptr;

    TRACE("(%p/%p, %p)\n", s, s->context, list);

#ifdef HAVE_SSLCOPYPEERCERTIFICATES
    status = SSLCopyPeerCertificates(s->context, &cert_array);
#else
    status = SSLGetPeerCertificates(s->context, &cert_array);
#endif
    if (status != noErr || !cert_array)
    {
        WARN("SSLCopyPeerCertificates failed: %d\n", status);
        return SEC_E_INTERNAL_ERROR;
    }

    list->count = CFArrayGetCount(cert_array);
    size = list->count * sizeof(list->certs[0]);

    for (i = 0; i < list->count; i++)
    {
        if (!(cert = (SecCertificateRef)CFArrayGetValueAtIndex(cert_array, i)) ||
            (SecKeychainItemExport(cert, kSecFormatX509Cert, 0, NULL, &data) != noErr))
        {
            WARN("Couldn't extract certificate data\n");
            ret = SEC_E_INTERNAL_ERROR;
            goto done;
        }
        size += CFDataGetLength(data);
        CFRelease(data);
    }

    if (!(list->certs = RtlAllocateHeap(GetProcessHeap(), 0, size)))
    {
        ret = SEC_E_INSUFFICIENT_MEMORY;
        goto done;
    }

    ptr = (BYTE *)&list->certs[list->count];
    for (i = 0; i < list->count; i++)
    {
        if (!(cert = (SecCertificateRef)CFArrayGetValueAtIndex(cert_array, i)) ||
            (SecKeychainItemExport(cert, kSecFormatX509Cert, 0, NULL, &data) != noErr))
        {
            WARN("Couldn't extract certificate data\n");
            ret = SEC_E_INTERNAL_ERROR;
            goto done;
        }
        list->certs[i].cbData = CFDataGetLength(data);
        list->certs[i].pbData = ptr;
        memcpy(list->certs[i].pbData, CFDataGetBytePtr(data), CFDataGetLength(data));
        ptr += CFDataGetLength(data);
        CFRelease(data);
    }

done:
#ifndef HAVE_SSLCOPYPEERCERTIFICATES
    /* This is why SSLGetPeerCertificates was deprecated */
    CFArrayApplyFunction(cert_array, CFRangeMake(0, CFArrayGetCount(cert_array)), cf_release, NULL);
#endif
    CFRelease(cert_array);
    return ret;
}

static SECURITY_STATUS CDECL schan_send(schan_session session, const void *buffer, SIZE_T *length)
{
    struct mac_session* s = (struct mac_session*)session;
    int status;

    TRACE("(%p/%p, %p, %p/%lu)\n", s, s->context, buffer, length, *length);

    pthread_mutex_lock(&s->mutex);
    s->mode = schan_mode_WRITE;

    status = SSLWrite(s->context, buffer, *length, length);

    s->mode = schan_mode_NONE;
    pthread_mutex_unlock(&s->mutex);

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
        WARN("SSLWrite failed: %d\n", status);
        return SEC_E_INTERNAL_ERROR;
    }

    return SEC_E_OK;
}

static SECURITY_STATUS CDECL schan_recv(schan_session session, void *buffer, SIZE_T *length)
{
    struct mac_session* s = (struct mac_session*)session;
    int status;

    TRACE("(%p/%p, %p, %p/%lu)\n", s, s->context, buffer, length, *length);

    pthread_mutex_lock(&s->mutex);
    s->mode = schan_mode_READ;

    status = SSLRead(s->context, buffer, *length, length);

    s->mode = schan_mode_NONE;
    pthread_mutex_unlock(&s->mutex);

    if (status == noErr || status == errSSLClosedGraceful)
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
        WARN("SSLRead failed: %d\n", status);
        return SEC_E_INTERNAL_ERROR;
    }

    return SEC_E_OK;
}

static BOOL CDECL schan_allocate_certificate_credentials(schan_credentials *c, const CERT_CONTEXT *cert,
                                                         const DATA_BLOB *key_blob)
{
    if (cert) FIXME("no support for certificate credentials on this platform\n");
    c->credentials = NULL;
    return TRUE;
}

static void CDECL schan_free_certificate_credentials(schan_credentials *c)
{
}

static void CDECL schan_set_application_protocols(schan_session session, unsigned char *buffer, unsigned int buflen)
{
    FIXME("no support for application protocols on this platform\n");
}

static SECURITY_STATUS CDECL schan_get_application_protocol(schan_session session,
                                                            SecPkgContext_ApplicationProtocol *protocol)
{
    FIXME("no support for application protocols on this platform\n");
    return SEC_E_UNSUPPORTED_FUNCTION;
}

static SECURITY_STATUS CDECL schan_set_dtls_mtu(schan_session session, unsigned int mtu)
{
    FIXME("no support for setting dtls mtu on this platform\n");
    return SEC_E_UNSUPPORTED_FUNCTION;
}

static void ssl_init(void)
{
    TRACE("()\n");

    supported_protocols = SP_PROT_SSL2_CLIENT | SP_PROT_SSL3_CLIENT | SP_PROT_TLS1_0_CLIENT;

#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1080
    if(&SSLGetProtocolVersionMax != NULL) {
        SSLProtocol max_protocol;
        SSLContextRef ctx;
        OSStatus status;

        status = SSLNewContext(FALSE, &ctx);
        if(status == noErr) {
            status = SSLGetProtocolVersionMax(ctx, &max_protocol);
            if(status == noErr) {
                if(max_protocol >= kTLSProtocol11)
                    supported_protocols |= SP_PROT_TLS1_1_CLIENT;
                if(max_protocol >= kTLSProtocol12)
                    supported_protocols |= SP_PROT_TLS1_2_CLIENT;
            }
            SSLDisposeContext(ctx);
        }else {
            WARN("SSLNewContext failed\n");
        }
    }
#endif
}

static const struct schan_funcs funcs =
{
    schan_allocate_certificate_credentials,
    schan_create_session,
    schan_dispose_session,
    schan_free_certificate_credentials,
    schan_get_application_protocol,
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
    schan_set_session_transport,
};

NTSTATUS CDECL __wine_init_unix_lib( HMODULE module, DWORD reason, const void *ptr_in, void *ptr_out )
{
    if (reason != DLL_PROCESS_ATTACH) return STATUS_SUCCESS;
    ssl_init();
    callbacks = ptr_in;
    *(const struct schan_funcs **)ptr_out = &funcs;
    return STATUS_SUCCESS;
}

#endif /* HAVE_SECURITY_SECURITY_H */
