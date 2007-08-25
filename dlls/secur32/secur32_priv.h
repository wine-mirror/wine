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
#include "wine/list.h"

/* Memory allocation functions for memory accessible by callers of secur32.
 * The details are implementation specific.
 */
#define SECUR32_ALLOC(bytes) HeapAlloc(GetProcessHeap(), 0, (bytes))
#define SECUR32_FREE(p)      HeapFree(GetProcessHeap(), 0, (p))

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
    unsigned long neg_flags;
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

typedef enum _sign_direction {
    NTLM_SEND,
    NTLM_RECV
} SignDirection;

/* Allocates space for and initializes a new provider.  If fnTableA or fnTableW
 * is non-NULL, assumes the provider is built-in (and is thus already loaded.)
 * Otherwise moduleName must not be NULL.
 * Returns a pointer to the stored provider entry, for use adding packages.
 */
SecureProvider *SECUR32_addProvider(const SecurityFunctionTableA *fnTableA,
 const SecurityFunctionTableW *fnTableW, PCWSTR moduleName);

/* Allocates space for and adds toAdd packages with the given provider.
 * provider must not be NULL, and either infoA or infoW may be NULL, but not
 * both.
 */
void SECUR32_addPackages(SecureProvider *provider, ULONG toAdd,
 const SecPkgInfoA *infoA, const SecPkgInfoW *infoW);

/* Tries to find the package named packageName.  If it finds it, implicitly
 * loads the package if it isn't already loaded.
 */
SecurePackage *SECUR32_findPackageW(PCWSTR packageName);

/* Tries to find the package named packageName.  (Thunks to _findPackageW)
 */
SecurePackage *SECUR32_findPackageA(PCSTR packageName);

/* A few string helpers; will return NULL if str is NULL.  Free return with
 * SECUR32_FREE */
PWSTR SECUR32_strdupW(PCWSTR str);
PWSTR SECUR32_AllocWideFromMultiByte(PCSTR str);
PSTR  SECUR32_AllocMultiByteFromWide(PCWSTR str);

/* Initialization functions for built-in providers */
void SECUR32_initSchannelSP(void);
void SECUR32_initNegotiateSP(void);
void SECUR32_initNTLMSP(void);

/* Functions from dispatcher.c used elsewhere in the code */
SECURITY_STATUS fork_helper(PNegoHelper *new_helper, const char *prog,
        char * const argv[]);

SECURITY_STATUS run_helper(PNegoHelper helper, char *buffer,
        unsigned int max_buflen, int *buflen);

void cleanup_helper(PNegoHelper helper);

void check_version(PNegoHelper helper);

/* Functions from base64_codec.c used elsewhere */
SECURITY_STATUS encodeBase64(PBYTE in_buf, int in_len, char* out_buf, 
        int max_len, int *out_len);

SECURITY_STATUS decodeBase64(char *in_buf, int in_len, BYTE *out_buf,
        int max_len, int *out_len);

/* Functions from util.c */
ULONG ComputeCrc32(const BYTE *pData, INT iLen, ULONG initial_crc);
SECURITY_STATUS SECUR32_CreateNTLMv1SessionKey(PBYTE password, int len, PBYTE session_key);
SECURITY_STATUS SECUR32_CreateNTLMv2SubKeys(PNegoHelper helper);
arc4_info *SECUR32_arc4Alloc(void);
void SECUR32_arc4Init(arc4_info *a4i, const BYTE *key, unsigned int keyLen);
void SECUR32_arc4Process(arc4_info *a4i, BYTE *inoutString, unsigned int length);
void SECUR32_arc4Cleanup(arc4_info *a4i);

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
#endif /* ndef __SECUR32_PRIV_H__ */
