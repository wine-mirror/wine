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
    int current_buffer_idx;
    BOOL allow_buffer_resize;
    int (*get_next_buffer)(const struct schan_transport *, struct schan_buffers *);
};

struct schan_transport
{
    struct schan_context *ctx;
    struct schan_buffers in;
    struct schan_buffers out;
};

struct schan_cert_list
{
    unsigned int count;
    CERT_BLOB   *certs;
};

struct schan_funcs
{
    BOOL (CDECL *allocate_certificate_credentials)(schan_credentials *, const CERT_CONTEXT *, const DATA_BLOB *);
    BOOL (CDECL *create_session)(schan_session *, schan_credentials *);
    void (CDECL *dispose_session)(schan_session);
    void (CDECL *free_certificate_credentials)(schan_credentials *);
    SECURITY_STATUS (CDECL *get_application_protocol)(schan_session, SecPkgContext_ApplicationProtocol *);
    SECURITY_STATUS (CDECL *get_connection_info)(schan_session, SecPkgContext_ConnectionInfo *);
    DWORD (CDECL *get_enabled_protocols)(void);
    ALG_ID (CDECL *get_key_signature_algorithm)(schan_session);
    unsigned int (CDECL *get_max_message_size)(schan_session);
    unsigned int (CDECL *get_session_cipher_block_size)(schan_session);
    SECURITY_STATUS (CDECL *get_session_peer_certificate)(schan_session, struct schan_cert_list *);
    SECURITY_STATUS (CDECL *get_unique_channel_binding)(schan_session, SecPkgContext_Bindings *);
    SECURITY_STATUS (CDECL *handshake)(schan_session session);
    SECURITY_STATUS (CDECL *recv)(schan_session, void *, SIZE_T *);
    SECURITY_STATUS (CDECL *send)(schan_session, const void *, SIZE_T *);
    void (CDECL *set_application_protocols)(schan_session, unsigned char *, unsigned int);
    SECURITY_STATUS (CDECL *set_dtls_mtu)(schan_session, unsigned int);
    void (CDECL *set_session_target)(schan_session, const char *);
    void (CDECL *set_session_transport)(schan_session, struct schan_transport *);
};

struct schan_callbacks
{
    char * (CDECL *get_buffer)(const struct schan_transport *, struct schan_buffers *, SIZE_T *);
    schan_session (CDECL *get_session_for_transport)(struct schan_transport *);
    int CDECL (CDECL *pull)(struct schan_transport *, void *, size_t *);
    int CDECL (CDECL *push)(struct schan_transport *, const void *, size_t *);
};

extern const struct schan_funcs *schan_funcs;

#endif /* __SECUR32_PRIV_H__ */
