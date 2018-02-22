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

typedef enum _helper_mode {
    NTLM_SERVER,
    NTLM_CLIENT,
    NUM_HELPER_MODES
} HelperMode;

typedef struct tag_arc4_info {
    unsigned char x, y;
    unsigned char state[256];
} arc4_info;

typedef struct _NegoHelper {
    pid_t helper_pid;
    HelperMode mode;
    int pipe_in;
    int pipe_out;
    int major;
    int minor;
    int micro;
    char *com_buf;
    int com_buf_size;
    int com_buf_offset;
    BYTE *session_key;
    ULONG neg_flags;
    struct {
        struct {
            ULONG seq_num;
            arc4_info *a4i;
        } ntlm;
        struct {
            BYTE *send_sign_key;
            BYTE *send_seal_key;
            BYTE *recv_sign_key;
            BYTE *recv_seal_key;
            ULONG send_seq_no;
            ULONG recv_seq_no;
            arc4_info *send_a4i;
            arc4_info *recv_a4i;
        } ntlm2;
    } crypt;
} NegoHelper, *PNegoHelper;

typedef struct _NtlmCredentials
{
    HelperMode mode;

    /* these are all in the Unix codepage */
    char *username_arg;
    char *domain_arg;
    char *password; /* not nul-terminated */
    int pwlen;
    int no_cached_credentials; /* don't try to use cached Samba credentials */
} NtlmCredentials, *PNtlmCredentials;

typedef enum _sign_direction {
    NTLM_SEND,
    NTLM_RECV
} SignDirection;

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
void SECUR32_initNTLMSP(void) DECLSPEC_HIDDEN;
void load_auth_packages(void) DECLSPEC_HIDDEN;

/* Cleanup functions for built-in providers */
void SECUR32_deinitSchannelSP(void) DECLSPEC_HIDDEN;

/* Functions from dispatcher.c used elsewhere in the code */
SECURITY_STATUS fork_helper(PNegoHelper *new_helper, const char *prog,
        char * const argv[]) DECLSPEC_HIDDEN;

SECURITY_STATUS run_helper(PNegoHelper helper, char *buffer,
        unsigned int max_buflen, int *buflen) DECLSPEC_HIDDEN;

void cleanup_helper(PNegoHelper helper) DECLSPEC_HIDDEN;

void check_version(PNegoHelper helper) DECLSPEC_HIDDEN;

/* Functions from base64_codec.c used elsewhere */
SECURITY_STATUS encodeBase64(PBYTE in_buf, int in_len, char* out_buf,
        int max_len, int *out_len) DECLSPEC_HIDDEN;

SECURITY_STATUS decodeBase64(char *in_buf, int in_len, BYTE *out_buf,
        int max_len, int *out_len) DECLSPEC_HIDDEN;

/* Functions from util.c */
ULONG ComputeCrc32(const BYTE *pData, INT iLen, ULONG initial_crc) DECLSPEC_HIDDEN;
SECURITY_STATUS SECUR32_CreateNTLM1SessionKey(PBYTE password, int len, PBYTE session_key) DECLSPEC_HIDDEN;
SECURITY_STATUS SECUR32_CreateNTLM2SubKeys(PNegoHelper helper) DECLSPEC_HIDDEN;
arc4_info *SECUR32_arc4Alloc(void) DECLSPEC_HIDDEN;
void SECUR32_arc4Init(arc4_info *a4i, const BYTE *key, unsigned int keyLen) DECLSPEC_HIDDEN;
void SECUR32_arc4Process(arc4_info *a4i, BYTE *inoutString, unsigned int length) DECLSPEC_HIDDEN;
void SECUR32_arc4Cleanup(arc4_info *a4i) DECLSPEC_HIDDEN;

/* NTLMSSP flags indicating the negotiated features */
#define NTLMSSP_NEGOTIATE_UNICODE                   0x00000001
#define NTLMSSP_NEGOTIATE_OEM                       0x00000002
#define NTLMSSP_REQUEST_TARGET                      0x00000004
#define NTLMSSP_NEGOTIATE_SIGN                      0x00000010
#define NTLMSSP_NEGOTIATE_SEAL                      0x00000020
#define NTLMSSP_NEGOTIATE_DATAGRAM_STYLE            0x00000040
#define NTLMSSP_NEGOTIATE_LM_SESSION_KEY            0x00000080
#define NTLMSSP_NEGOTIATE_NTLM                      0x00000200
#define NTLMSSP_NEGOTIATE_DOMAIN_SUPPLIED           0x00001000
#define NTLMSSP_NEGOTIATE_WORKSTATION_SUPPLIED      0x00002000
#define NTLMSSP_NEGOTIATE_LOCAL_CALL                0x00004000
#define NTLMSSP_NEGOTIATE_ALWAYS_SIGN               0x00008000
#define NTLMSSP_NEGOTIATE_TARGET_TYPE_DOMAIN        0x00010000
#define NTLMSSP_NEGOTIATE_TARGET_TYPE_SERVER        0x00020000
#define NTLMSSP_NEGOTIATE_NTLM2                     0x00080000
#define NTLMSSP_NEGOTIATE_TARGET_INFO               0x00800000
#define NTLMSSP_NEGOTIATE_128                       0x20000000
#define NTLMSSP_NEGOTIATE_KEY_EXCHANGE              0x40000000
#define NTLMSSP_NEGOTIATE_56                        0x80000000


SecPkgInfoW *ntlm_package_infoW DECLSPEC_HIDDEN;
SecPkgInfoA *ntlm_package_infoA DECLSPEC_HIDDEN;

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
extern SECURITY_STATUS schan_imp_get_session_peer_certificate(schan_imp_session session, HCERTSTORE,
                                                              PCCERT_CONTEXT *cert) DECLSPEC_HIDDEN;
extern SECURITY_STATUS schan_imp_send(schan_imp_session session, const void *buffer,
                                      SIZE_T *length) DECLSPEC_HIDDEN;
extern SECURITY_STATUS schan_imp_recv(schan_imp_session session, void *buffer,
                                      SIZE_T *length) DECLSPEC_HIDDEN;
extern BOOL schan_imp_allocate_certificate_credentials(schan_credentials*) DECLSPEC_HIDDEN;
extern void schan_imp_free_certificate_credentials(schan_credentials*) DECLSPEC_HIDDEN;
extern DWORD schan_imp_enabled_protocols(void) DECLSPEC_HIDDEN;
extern BOOL schan_imp_init(void) DECLSPEC_HIDDEN;
extern void schan_imp_deinit(void) DECLSPEC_HIDDEN;


#endif /* ndef __SECUR32_PRIV_H__ */
