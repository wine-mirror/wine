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
#include "wine/heap.h"
#include "wine/list.h"
#include "schannel.h"

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

/* A few string helpers; will return NULL if str is NULL.  Free return with
 * HeapFree */
PWSTR SECUR32_AllocWideFromMultiByte(PCSTR str) DECLSPEC_HIDDEN;
PSTR  SECUR32_AllocMultiByteFromWide(PCWSTR str) DECLSPEC_HIDDEN;

/* Initialization functions for built-in providers */
void SECUR32_initSchannelSP(void) DECLSPEC_HIDDEN;
void SECUR32_initNegotiateSP(void) DECLSPEC_HIDDEN;
void load_auth_packages(void) DECLSPEC_HIDDEN;

/* Cleanup functions for built-in providers */
void SECUR32_deinitSchannelSP(void) DECLSPEC_HIDDEN;

/* schannel internal interface */
typedef struct schan_imp_session_opaque *schan_imp_session;

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

char *schan_get_buffer(const struct schan_transport *t, struct schan_buffers *s, SIZE_T *count) DECLSPEC_HIDDEN;
extern int schan_pull(struct schan_transport *t, void *buff, size_t *buff_len) DECLSPEC_HIDDEN;
extern int schan_push(struct schan_transport *t, const void *buff, size_t *buff_len) DECLSPEC_HIDDEN;

extern schan_imp_session schan_session_for_transport(struct schan_transport* t) DECLSPEC_HIDDEN;

/* schannel implementation interface */
extern BOOL schan_imp_create_session(schan_imp_session *session, schan_credentials *cred) DECLSPEC_HIDDEN;
extern void schan_imp_dispose_session(schan_imp_session session) DECLSPEC_HIDDEN;
extern void schan_imp_set_session_transport(schan_imp_session session,
                                            struct schan_transport *t) DECLSPEC_HIDDEN;
extern void schan_imp_set_session_target(schan_imp_session session, const char *target) DECLSPEC_HIDDEN;
extern SECURITY_STATUS schan_imp_handshake(schan_imp_session session) DECLSPEC_HIDDEN;
extern unsigned int schan_imp_get_session_cipher_block_size(schan_imp_session session) DECLSPEC_HIDDEN;
extern unsigned int schan_imp_get_max_message_size(schan_imp_session session) DECLSPEC_HIDDEN;
extern ALG_ID schan_imp_get_key_signature_algorithm(schan_imp_session session) DECLSPEC_HIDDEN;
extern SECURITY_STATUS schan_imp_get_connection_info(schan_imp_session session,
                                                     SecPkgContext_ConnectionInfo *info) DECLSPEC_HIDDEN;
extern SECURITY_STATUS schan_imp_get_unique_channel_binding(schan_imp_session session,
                                                            SecPkgContext_Bindings *bindings) DECLSPEC_HIDDEN;
extern SECURITY_STATUS schan_imp_get_session_peer_certificate(schan_imp_session session, HCERTSTORE,
                                                              PCCERT_CONTEXT *cert) DECLSPEC_HIDDEN;
extern SECURITY_STATUS schan_imp_send(schan_imp_session session, const void *buffer,
                                      SIZE_T *length) DECLSPEC_HIDDEN;
extern SECURITY_STATUS schan_imp_recv(schan_imp_session session, void *buffer,
                                      SIZE_T *length) DECLSPEC_HIDDEN;
extern BOOL schan_imp_allocate_certificate_credentials(schan_credentials *, const CERT_CONTEXT *) DECLSPEC_HIDDEN;
extern void schan_imp_free_certificate_credentials(schan_credentials*) DECLSPEC_HIDDEN;
extern DWORD schan_imp_enabled_protocols(void) DECLSPEC_HIDDEN;
extern BOOL schan_imp_init(void) DECLSPEC_HIDDEN;
extern void schan_imp_deinit(void) DECLSPEC_HIDDEN;
extern void schan_imp_set_application_protocols(schan_imp_session, unsigned char *, unsigned int) DECLSPEC_HIDDEN;
extern SECURITY_STATUS schan_imp_get_application_protocol(schan_imp_session,
                                                          SecPkgContext_ApplicationProtocol *) DECLSPEC_HIDDEN;
extern SECURITY_STATUS schan_imp_set_dtls_mtu(schan_imp_session, unsigned int) DECLSPEC_HIDDEN;

#endif /* ndef __SECUR32_PRIV_H__ */
