/*
 * secur32 private definitions.
 *
 * Copyright (C) 2004 Juan Lang
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

#ifndef __SECUR32_PRIV_H__
#define __SECUR32_PRIV_H__

#include <sys/types.h>
#include <limits.h>
#include "schannel.h"
#include "wine/list.h"

extern HINSTANCE hsecur32 DECLSPEC_HIDDEN;

typedef struct _SecureProvider
{
    struct list             entry;
    BOOL                    loaded;
    PWSTR                   moduleName;
    HMODULE                 lib;
    SecurityFunctionTableA  fnTableA;
    SecurityFunctionTableW  fnTableW;
} SecureProvider;

typedef struct _SecurePackage
{
    struct list     entry;
    SecPkgInfoW     infoW;
    SecureProvider *provider;
} SecurePackage;

/* Allocates space for and initializes a new provider.  If fnTableA or fnTableW
 * is non-NULL, assumes the provider is built-in, and if moduleName is non-NULL,
 * means must load the LSA/user mode functions tables from external SSP/AP module.
 * Otherwise moduleName must not be NULL.
 * Returns a pointer to the stored provider entry, for use adding packages.
 */
SecureProvider *SECUR32_addProvider(const SecurityFunctionTableA *fnTableA,
 const SecurityFunctionTableW *fnTableW, PCWSTR moduleName) DECLSPEC_HIDDEN;

/* Allocates space for and adds toAdd packages with the given provider.
 * provider must not be NULL, and either infoA or infoW may be NULL, but not
 * both.
 */
void SECUR32_addPackages(SecureProvider *provider, ULONG toAdd,
 const SecPkgInfoA *infoA, const SecPkgInfoW *infoW) DECLSPEC_HIDDEN;

/* Tries to find the package named packageName.  If it finds it, implicitly
 * loads the package if it isn't already loaded.
 */
SecurePackage *SECUR32_findPackageW(PCWSTR packageName) DECLSPEC_HIDDEN;

/* Tries to find the package named packageName.  (Thunks to _findPackageW)
 */
SecurePackage *SECUR32_findPackageA(PCSTR packageName) DECLSPEC_HIDDEN;

/* Initialization functions for built-in providers */
void SECUR32_initSchannelSP(void) DECLSPEC_HIDDEN;
void SECUR32_initNegotiateSP(void) DECLSPEC_HIDDEN;
void load_auth_packages(void) DECLSPEC_HIDDEN;

/* Cleanup functions for built-in providers */
void SECUR32_deinitSchannelSP(void) DECLSPEC_HIDDEN;

/* schannel internal interface */
typedef struct schan_session_opaque *schan_session;

typedef struct schan_credentials
{
    ULONG credential_use;
    void *credentials;
    DWORD enabled_protocols;
} schan_credentials;

struct schan_transport;

struct schan_buffers
{
    SIZE_T offset;
    SIZE_T limit;
    const SecBufferDesc *desc;
    SecBuffer *alloc_buffer;
    int current_buffer_idx;
    int (*get_next_buffer)(const struct schan_transport *, struct schan_buffers *);
};

struct schan_transport
{
    struct schan_context *ctx;
    schan_session session;
    struct schan_buffers in;
    struct schan_buffers out;
};

struct session_params
{
    schan_session session;
};

struct allocate_certificate_credentials_params
{
    schan_credentials *c;
    const CERT_CONTEXT *ctx;
    const DATA_BLOB *key_blob;
};

struct create_session_params
{
    struct schan_transport *transport;
    schan_credentials *cred;
};

struct free_certificate_credentials_params
{
    schan_credentials *c;
};

struct get_application_protocol_params
{
    schan_session session;
    SecPkgContext_ApplicationProtocol *protocol;
};

struct get_connection_info_params
{
    schan_session session;
    SecPkgContext_ConnectionInfo *info;
};

struct get_session_peer_certificate_params
{
    schan_session session;
    CERT_BLOB *certs;
    ULONG *bufsize;
    ULONG *retcount;
};

struct get_unique_channel_binding_params
{
    schan_session session;
    void *buffer;
    ULONG *bufsize;
};

struct handshake_params
{
    schan_session session;
    SecBufferDesc *input;
    SIZE_T input_size;
    SecBufferDesc *output;
    SecBuffer *alloc_buffer;
};

struct recv_params
{
    schan_session session;
    SecBufferDesc *input;
    SIZE_T input_size;
    void *buffer;
    SIZE_T *length;
};

struct send_params
{
    schan_session session;
    SecBufferDesc *output;
    const void *buffer;
    SIZE_T *length;
};

struct set_application_protocols_params
{
    schan_session session;
    unsigned char *buffer;
    unsigned int buflen;
};

struct set_dtls_mtu_params
{
    schan_session session;
    unsigned int mtu;
};

struct set_session_target_params
{
    schan_session session;
    const char *target;
};

struct set_dtls_timeouts_params
{
    schan_session session;
    unsigned int retrans_timeout;
    unsigned int total_timeout;
};

enum schan_funcs
{
    unix_process_attach,
    unix_process_detach,
    unix_allocate_certificate_credentials,
    unix_create_session,
    unix_dispose_session,
    unix_free_certificate_credentials,
    unix_get_application_protocol,
    unix_get_connection_info,
    unix_get_enabled_protocols,
    unix_get_key_signature_algorithm,
    unix_get_max_message_size,
    unix_get_session_cipher_block_size,
    unix_get_session_peer_certificate,
    unix_get_unique_channel_binding,
    unix_handshake,
    unix_recv,
    unix_send,
    unix_set_application_protocols,
    unix_set_dtls_mtu,
    unix_set_session_target,
    unix_set_dtls_timeouts,
};

#endif /* __SECUR32_PRIV_H__ */
