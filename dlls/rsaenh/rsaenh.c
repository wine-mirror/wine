/*
 * dlls/rsaenh/rsaenh.c
 * RSAENH - RSA encryption for Wine
 *
 * Copyright 2002 TransGaming Technologies (David Hammerton)
 * Copyright 2004 Mike McCormack for CodeWeavers
 * Copyright 2004 Michael Jung
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"
#include "wine/library.h"
#include "wine/debug.h"

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wincrypt.h"
#include "lmcons.h"
#include "handle.h"
#include "implglue.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

/******************************************************************************
 * CRYPTHASH - hash objects
 */
#define RSAENH_MAGIC_HASH           0x85938417u
#define RSAENH_MAX_HASH_SIZE        36
#define RSAENH_HASHSTATE_IDLE       0
#define RSAENH_HASHSTATE_HASHING    1
#define RSAENH_HASHSTATE_FINISHED   2
typedef struct tagCRYPTHASH
{
    OBJECTHDR    header;
    ALG_ID       aiAlgid;
    HCRYPTKEY    hKey;
    HCRYPTPROV   hProv;
    DWORD        dwHashSize;
    DWORD        dwState;
    HASH_CONTEXT context;
    BYTE         abHashValue[RSAENH_MAX_HASH_SIZE];
    PHMAC_INFO   pHMACInfo;
} CRYPTHASH;

/******************************************************************************
 * CRYPTKEY - key objects
 */
#define RSAENH_MAGIC_KEY           0x73620457u
#define RSAENH_MAX_KEY_SIZE        24
#define RSAENH_MAX_BLOCK_SIZE      24
#define RSAENH_KEYSTATE_IDLE       0
#define RSAENH_KEYSTATE_ENCRYPTING 1
#define RSAENH_KEYSTATE_DECRYPTING 2
typedef struct tagCRYPTKEY
{
    OBJECTHDR   header;
    ALG_ID      aiAlgid;
    HCRYPTPROV  hProv;
    DWORD       dwMode;
    DWORD       dwModeBits;
    DWORD       dwPermissions;
    DWORD       dwKeyLen;
    DWORD       dwSaltLen;
    DWORD       dwBlockLen;
    DWORD       dwState;
    KEY_CONTEXT context;    
    BYTE        abKeyValue[RSAENH_MAX_KEY_SIZE];
    BYTE        abInitVector[RSAENH_MAX_BLOCK_SIZE];
    BYTE        abChainVector[RSAENH_MAX_BLOCK_SIZE];
} CRYPTKEY;

/******************************************************************************
 * KEYCONTAINER - key containers
 */
#define RSAENH_PERSONALITY_BASE        0u
#define RSAENH_PERSONALITY_STRONG      1u
#define RSAENH_PERSONALITY_ENHANCED    2u

#define RSAENH_MAGIC_CONTAINER         0x26384993u
typedef struct tagKEYCONTAINER
{
    OBJECTHDR    header;
    DWORD        dwMode;
    DWORD        dwPersonality;
    DWORD        dwEnumAlgsCtr;
    CHAR         szName[MAX_PATH];
    CHAR         szProvName[MAX_PATH];
    HCRYPTKEY    hKeyExchangeKeyPair;
    HCRYPTKEY    hSignatureKeyPair;
} KEYCONTAINER;

/******************************************************************************
 * Some magic constants
 */
#define RSAENH_ENCRYPT                    1
#define RSAENH_DECRYPT                    0    
#define RSAENH_HMAC_DEF_IPAD_CHAR      0x36
#define RSAENH_HMAC_DEF_OPAD_CHAR      0x5c
#define RSAENH_HMAC_DEF_PAD_LEN          64
#define RSAENH_DES_EFFECTIVE_KEYLEN      56
#define RSAENH_DES_STORAGE_KEYLEN        64
#define RSAENH_3DES112_EFFECTIVE_KEYLEN 112
#define RSAENH_3DES112_STORAGE_KEYLEN   128
#define RSAENH_3DES_EFFECTIVE_KEYLEN    168
#define RSAENH_3DES_STORAGE_KEYLEN      192
#define RSAENH_MAGIC_RSA2        0x32415352
#define RSAENH_MAGIC_RSA1        0x31415352
#define RSAENH_PKC_BLOCKTYPE           0x02

#define RSAENH_MIN(a,b) ((a)<(b)?(a):(b))
/******************************************************************************
 * aProvEnumAlgsEx - Defines the capabilities of the CSP personalities.
 */
#define RSAENH_MAX_ENUMALGS 14
PROV_ENUMALGS_EX aProvEnumAlgsEx[3][RSAENH_MAX_ENUMALGS+1] =
{
 {
  {CALG_RC2,       40, 40,   56,0,                    4,"RC2",     24,"RSA Data Security's RC2"},
  {CALG_RC4,       40, 40,   56,0,                    4,"RC4",     24,"RSA Data Security's RC4"},
  {CALG_DES,       56, 56,   56,0,                    4,"DES",     31,"Data Encryption Standard (DES)"},
  {CALG_SHA,      160,160,  160,CRYPT_FLAG_SIGNING,   6,"SHA-1",   30,"Secure Hash Algorithm (SHA-1)"},
  {CALG_MD2,      128,128,  128,CRYPT_FLAG_SIGNING,   4,"MD2",     27,"MD2 Message Digest 2 (MD2)"},
  {CALG_MD4,      128,128,  128,CRYPT_FLAG_SIGNING,   4,"MD4",     27,"MD4 Message Digest 4 (MD4)"},
  {CALG_MD5,      128,128,  128,CRYPT_FLAG_SIGNING,   4,"MD5",     27,"MD5 Message Digest 5 (MD5)"},
  {CALG_SSL3_SHAMD5,288,288,288,0,                   12,"SSL3 SHAMD5",12,"SSL3 SHAMD5"},
  {CALG_MAC,        0,  0,    0,0,                    4,"MAC",     27,"Message Authentication Code"},
  {CALG_RSA_SIGN, 512,384,16384,CRYPT_FLAG_SIGNING|CRYPT_FLAG_IPSEC,9,"RSA_SIGN",14,"RSA Signature"},
  {CALG_RSA_KEYX, 512,384, 1024,CRYPT_FLAG_SIGNING|CRYPT_FLAG_IPSEC,9,"RSA_KEYX",18,"RSA Key Exchange"},
  {CALG_HMAC,       0,  0,    0,0,                    5,"HMAC",    23,"HMAC Hugo's MAC (HMAC)"},
  {0,               0,  0,    0,0,                    1,"",         1,""}
 },
 {
  {CALG_RC2,      128, 40,  128,0,                    4,"RC2",     24,"RSA Data Security's RC2"},
  {CALG_RC4,      128, 40,  128,0,                    4,"RC4",     24,"RSA Data Security's RC4"},
  {CALG_DES,       56, 56,   56,0,                    4,"DES",     31,"Data Encryption Standard (DES)"},
  {CALG_3DES_112, 112,112,  112,0,                   13,"3DES TWO KEY",19,"Two Key Triple DES"},
  {CALG_3DES,     168,168,  168,0,                    5,"3DES",    21,"Three Key Triple DES"},
  {CALG_SHA,      160,160,  160,CRYPT_FLAG_SIGNING,   6,"SHA-1",   30,"Secure Hash Algorithm (SHA-1)"},
  {CALG_MD2,      128,128,  128,CRYPT_FLAG_SIGNING,   4,"MD2",     27,"MD2 Message Digest 2 (MD2)"},
  {CALG_MD4,      128,128,  128,CRYPT_FLAG_SIGNING,   4,"MD4",     27,"MD4 Message Digest 4 (MD4)"},
  {CALG_MD5,      128,128,  128,CRYPT_FLAG_SIGNING,   4,"MD5",     27,"MD5 Message Digest 5 (MD5)"},
  {CALG_SSL3_SHAMD5,288,288,288,0,                   12,"SSL3 SHAMD5",12,"SSL3 SHAMD5"},
  {CALG_MAC,        0,  0,    0,0,                    4,"MAC",     27,"Message Authentication Code"},
  {CALG_RSA_SIGN,1024,384,16384,CRYPT_FLAG_SIGNING|CRYPT_FLAG_IPSEC,9,"RSA_SIGN",14,"RSA Signature"},
  {CALG_RSA_KEYX,1024,384,16384,CRYPT_FLAG_SIGNING|CRYPT_FLAG_IPSEC,9,"RSA_KEYX",18,"RSA Key Exchange"},
  {CALG_HMAC,       0,  0,    0,0,                    5,"HMAC",    23,"HMAC Hugo's MAC (HMAC)"},
  {0,               0,  0,    0,0,                    1,"",         1,""}
 },
 {
  {CALG_RC2,      128, 40,  128,0,                    4,"RC2",     24,"RSA Data Security's RC2"},
  {CALG_RC4,      128, 40,  128,0,                    4,"RC4",     24,"RSA Data Security's RC4"},
  {CALG_DES,       56, 56,   56,0,                    4,"DES",     31,"Data Encryption Standard (DES)"},
  {CALG_3DES_112, 112,112,  112,0,                   13,"3DES TWO KEY",19,"Two Key Triple DES"},
  {CALG_3DES,     168,168,  168,0,                    5,"3DES",    21,"Three Key Triple DES"},
  {CALG_SHA,      160,160,  160,CRYPT_FLAG_SIGNING,   6,"SHA-1",   30,"Secure Hash Algorithm (SHA-1)"},
  {CALG_MD2,      128,128,  128,CRYPT_FLAG_SIGNING,   4,"MD2",     27,"MD2 Message Digest 2 (MD2)"},
  {CALG_MD4,      128,128,  128,CRYPT_FLAG_SIGNING,   4,"MD4",     27,"MD4 Message Digest 4 (MD4)"},
  {CALG_MD5,      128,128,  128,CRYPT_FLAG_SIGNING,   4,"MD5",     27,"MD5 Message Digest 5 (MD5)"},
  {CALG_SSL3_SHAMD5,288,288,288,0,                   12,"SSL3 SHAMD5",12,"SSL3 SHAMD5"},
  {CALG_MAC,        0,  0,    0,0,                    4,"MAC",     27,"Message Authentication Code"},
  {CALG_RSA_SIGN,1024,384,16384,CRYPT_FLAG_SIGNING|CRYPT_FLAG_IPSEC,9,"RSA_SIGN",14,"RSA Signature"},
  {CALG_RSA_KEYX,1024,384,16384,CRYPT_FLAG_SIGNING|CRYPT_FLAG_IPSEC,9,"RSA_KEYX",18,"RSA Key Exchange"},
  {CALG_HMAC,       0,  0,    0,0,                    5,"HMAC",    23,"HMAC Hugo's MAC (HMAC)"},
  {0,               0,  0,    0,0,                    1,"",         1,""}
 }
};

/******************************************************************************
 * API forward declarations
 */
BOOL WINAPI 
RSAENH_CPGetKeyParam(
    HCRYPTPROV hProv, 
    HCRYPTKEY hKey, 
    DWORD dwParam, 
    BYTE *pbData, 
    DWORD *pdwDataLen, 
    DWORD dwFlags
);

BOOL WINAPI 
RSAENH_CPEncrypt(
    HCRYPTPROV hProv, 
    HCRYPTKEY hKey, 
    HCRYPTHASH hHash, 
    BOOL Final, 
    DWORD dwFlags, 
    BYTE *pbData,
    DWORD *pdwDataLen, 
    DWORD dwBufLen
);

BOOL WINAPI 
RSAENH_CPGetHashParam(
    HCRYPTPROV hProv, 
    HCRYPTHASH hHash, 
    DWORD dwParam, 
    BYTE *pbData, 
    DWORD *pdwDataLen, 
    DWORD dwFlags
);

BOOL WINAPI 
RSAENH_CPExportKey(
    HCRYPTPROV hProv, 
    HCRYPTKEY hKey, 
    HCRYPTKEY hPubKey, 
    DWORD dwBlobType, 
    DWORD dwFlags, 
    BYTE *pbData, 
    DWORD *pdwDataLen
);

BOOL WINAPI 
RSAENH_CPImportKey(
    HCRYPTPROV hProv, 
    CONST BYTE *pbData, 
    DWORD dwDataLen, 
    HCRYPTKEY hPubKey, 
    DWORD dwFlags, 
    HCRYPTKEY *phKey
);

/******************************************************************************
 * CSP's handle table (used by all acquired key containers)
 */
static HANDLETABLE handle_table;

/******************************************************************************
 * DllMain (RSAENH.@)
 *
 * Initializes and destroys the handle table for the CSP's handles.
 */
int WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            init_handle_table(&handle_table);
            break;

        case DLL_PROCESS_DETACH:
            destroy_handle_table(&handle_table);
            break;
    }
    return 1;
}

/******************************************************************************
 * copy_param [Internal]
 *
 * Helper function that supports the standard WINAPI protocol for querying data
 * of dynamic size.
 *
 * PARAMS
 *  pbBuffer      [O]   Buffer where the queried parameter is copied to, if it is large enough.
 *                      May be NUL if the required buffer size is to be queried only.
 *  pdwBufferSize [I/O] In: Size of the buffer at pbBuffer
 *                      Out: Size of parameter pbParam
 *  pbParam       [I]   Parameter value.
 *  dwParamSize   [I]   Size of pbParam
 *
 * RETURN
 *  Success: TRUE (pbParam was copied into pbBuffer or pbBuffer is NULL)
 *  Failure: FALSE (pbBuffer is not large enough to hold pbParam). Last error: ERROR_MORE_DATA
 */
static inline BOOL copy_param(
    BYTE *pbBuffer, DWORD *pdwBufferSize, CONST BYTE *pbParam, DWORD dwParamSize) 
{
    if (pbBuffer) 
    {
        if (dwParamSize > *pdwBufferSize) 
        {
            SetLastError(ERROR_MORE_DATA);
            *pdwBufferSize = dwParamSize;
            return FALSE;
        }
        memcpy(pbBuffer, pbParam, dwParamSize);
    }
    *pdwBufferSize = dwParamSize;
    return TRUE;
}

/******************************************************************************
 * get_algid_info [Internal]
 *
 * Query CSP capabilities for a given crypto algorithm.
 * 
 * PARAMS
 *  pKeyContainer [I] Pointer to a key container of the CSP whose capabilities are to be queried.
 *  algid         [I] Identifier of the crypto algorithm about which information is requested.
 *
 * RETURNS
 *  Success: Pointer to a PROV_ENUMALGS_EX struct containing information about the crypto algorithm.
 *  Failure: NULL (algid not supported)
 */
static inline const PROV_ENUMALGS_EX* get_algid_info(KEYCONTAINER *pKeyContainer, ALG_ID algid) {
    PROV_ENUMALGS_EX *iterator;

    for (iterator = aProvEnumAlgsEx[pKeyContainer->dwPersonality]; iterator->aiAlgid; iterator++) {
        if (iterator->aiAlgid == algid) return iterator;
    }

    return NULL;
}

/******************************************************************************
 * free_hmac_info [Internal]
 *
 * Deeply free an HMAC_INFO struct.
 *
 * PARAMS
 *  hmac_info [I] Pointer to the HMAC_INFO struct to be freed.
 *
 * NOTES
 *  See Internet RFC 2104 for details on the HMAC algorithm.
 */
static inline void free_hmac_info(PHMAC_INFO hmac_info) {
    if (!hmac_info) return;
    if (hmac_info->pbInnerString) HeapFree(GetProcessHeap(), 0, hmac_info->pbInnerString);
    if (hmac_info->pbOuterString) HeapFree(GetProcessHeap(), 0, hmac_info->pbOuterString);
    HeapFree(GetProcessHeap(), 0, hmac_info);
}

/******************************************************************************
 * copy_hmac_info [Internal]
 *
 * Deeply copy an HMAC_INFO struct
 *
 * PARAMS
 *  dst [O] Pointer to a location where the pointer to the HMAC_INFO copy will be stored.
 *  src [I] Pointer to the HMAC_INFO struct to be copied.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  See Internet RFC 2104 for details on the HMAC algorithm.
 */
static BOOL copy_hmac_info(PHMAC_INFO *dst, PHMAC_INFO src) {
    if (!src) return FALSE;
    *dst = (PHMAC_INFO)HeapAlloc(GetProcessHeap(), 0, sizeof(HMAC_INFO));
    if (!*dst) return FALSE;
    memcpy(*dst, src, sizeof(HMAC_INFO));
    (*dst)->pbInnerString = NULL;
    (*dst)->pbOuterString = NULL;
    if ((*dst)->cbInnerString == 0) (*dst)->cbInnerString = RSAENH_HMAC_DEF_PAD_LEN;
    (*dst)->pbInnerString = (BYTE*)HeapAlloc(GetProcessHeap(), 0, (*dst)->cbInnerString);
    if (!(*dst)->pbInnerString) {
        free_hmac_info(*dst);
        return FALSE;
    }
    if (src->cbInnerString) 
        memcpy((*dst)->pbInnerString, src->pbInnerString, src->cbInnerString);
    else 
        memset((*dst)->pbInnerString, RSAENH_HMAC_DEF_IPAD_CHAR, RSAENH_HMAC_DEF_PAD_LEN);
    if ((*dst)->cbOuterString == 0) (*dst)->cbOuterString = RSAENH_HMAC_DEF_PAD_LEN;
    (*dst)->pbOuterString = (BYTE*)HeapAlloc(GetProcessHeap(), 0, (*dst)->cbOuterString);
    if (!(*dst)->pbOuterString) {
        free_hmac_info(*dst);
        return FALSE;
    }
    if (src->cbOuterString) 
        memcpy((*dst)->pbOuterString, src->pbOuterString, src->cbOuterString);
    else 
        memset((*dst)->pbOuterString, RSAENH_HMAC_DEF_OPAD_CHAR, RSAENH_HMAC_DEF_PAD_LEN);
    return TRUE;
}

/******************************************************************************
 * destroy_hash [Internal]
 *
 * Destructor for hash objects
 *
 * PARAMS
 *  pCryptHash [I] Pointer to the hash object to be destroyed. 
 *                 Will be invalid after function returns!
 */
static void destroy_hash(OBJECTHDR *pCryptHash)
{
    free_hmac_info(((CRYPTHASH*)pCryptHash)->pHMACInfo);
    HeapFree(GetProcessHeap(), 0, pCryptHash);
}

/******************************************************************************
 * init_hash [Internal]
 *
 * Initialize (or reset) a hash object
 *
 * PARAMS
 *  pKeyContainer [I] Pointer to the key container the hash object belongs to.
 *  pCryptHash    [I] The hash object to be initialized.
 */
static inline BOOL init_hash(KEYCONTAINER *pKeyContainer, CRYPTHASH *pCryptHash) {
    DWORD dwLen;
    const PROV_ENUMALGS_EX *pAlgInfo;
        
    switch (pCryptHash->aiAlgid) 
    {
        case CALG_HMAC:
            if (pCryptHash->pHMACInfo) { 
                pAlgInfo = get_algid_info(pKeyContainer, pCryptHash->pHMACInfo->HashAlgid);
                if (pAlgInfo) pCryptHash->dwHashSize = pAlgInfo->dwDefaultLen >> 3;
                return init_hash_impl(pCryptHash->pHMACInfo->HashAlgid, &pCryptHash->context);
            }
            return TRUE;

        case CALG_MAC:
            dwLen = sizeof(DWORD);
            RSAENH_CPGetKeyParam(pCryptHash->hProv, pCryptHash->hKey, KP_BLOCKLEN, 
                                 (BYTE*)&pCryptHash->dwHashSize, &dwLen, 0);
            pCryptHash->dwHashSize >>= 3;
            return TRUE;

        default:
            return init_hash_impl(pCryptHash->aiAlgid, &pCryptHash->context);
    }
}

/******************************************************************************
 * update_hash [Internal]
 *
 * Hashes the given data and updates the hash object's state accordingly
 *
 * PARAMS
 *  pCryptHash [I] Hash object to be updated.
 *  pbData     [I] Pointer to data stream to be hashed.
 *  dwDataLen  [I] Length of data stream.
 */
static inline void update_hash(CRYPTHASH *pCryptHash, CONST BYTE *pbData, DWORD dwDataLen) {
    BYTE *pbTemp;

    switch (pCryptHash->aiAlgid)
    {
        case CALG_HMAC:
            if (pCryptHash->pHMACInfo) 
                update_hash_impl(pCryptHash->pHMACInfo->HashAlgid, &pCryptHash->context, 
                                 pbData, dwDataLen);
            break;

        case CALG_MAC:
            pbTemp = (BYTE*)HeapAlloc(GetProcessHeap(), 0, dwDataLen);
            if (!pbTemp) return;
            memcpy(pbTemp, pbData, dwDataLen);
            RSAENH_CPEncrypt(pCryptHash->hProv, pCryptHash->hKey, (HCRYPTHASH)NULL, FALSE, 0, 
                             pbTemp, &dwDataLen, dwDataLen);
            HeapFree(GetProcessHeap(), 0, pbTemp);
            break;

        default:
            update_hash_impl(pCryptHash->aiAlgid, &pCryptHash->context, pbData, dwDataLen);
    }
}

/******************************************************************************
 * finalize_hash [Internal]
 *
 * Finalizes the hash, after all data has been hashed with update_hash.
 * No additional data can be hashed afterwards until the hash gets initialized again.
 *
 * PARAMS
 *  pCryptHash [I] Hash object to be finalized.
 */
static inline void finalize_hash(CRYPTHASH *pCryptHash) {
    DWORD dwDataLen;
        
    switch (pCryptHash->aiAlgid)
    {
        case CALG_HMAC:
            if (pCryptHash->pHMACInfo)
                finalize_hash_impl(pCryptHash->pHMACInfo->HashAlgid, &pCryptHash->context, 
                                   pCryptHash->abHashValue);
            break;

        case CALG_MAC:
            dwDataLen = 0;
            RSAENH_CPEncrypt(pCryptHash->hProv, pCryptHash->hKey, (HCRYPTHASH)NULL, TRUE, 0, 
                             pCryptHash->abHashValue, &dwDataLen, pCryptHash->dwHashSize);
            break;

        default:
            finalize_hash_impl(pCryptHash->aiAlgid, &pCryptHash->context, pCryptHash->abHashValue);
    }
}

/******************************************************************************
 * destroy_key [Internal]
 *
 * Destructor for key objects
 *
 * PARAMS
 *  pCryptKey [I] Pointer to the key object to be destroyed. 
 *                Will be invalid after function returns!
 */
static void destroy_key(OBJECTHDR *pCryptKey)
{
    free_key_impl(((CRYPTKEY*)pCryptKey)->aiAlgid, &((CRYPTKEY*)pCryptKey)->context);
    HeapFree(GetProcessHeap(), 0, pCryptKey);
}

/******************************************************************************
 * setup_key [Internal]
 *
 * Initialize (or reset) a key object
 *
 * PARAMS
 *  pCryptKey    [I] The key object to be initialized.
 */
static inline void setup_key(CRYPTKEY *pCryptKey) {
    pCryptKey->dwState = RSAENH_KEYSTATE_IDLE;
    memcpy(pCryptKey->abChainVector, pCryptKey->abInitVector, sizeof(pCryptKey->abChainVector));
    setup_key_impl(pCryptKey->aiAlgid, &pCryptKey->context, pCryptKey->dwKeyLen, 
                   pCryptKey->dwSaltLen, pCryptKey->abKeyValue);
}

/******************************************************************************
 * new_key [Internal]
 *
 * Creates a new key object. If neither pbKey nor hHash is given a random key
 * will be generated.
 *
 * PARAMS
 *  hProv   [I] Handle to the provider to which the created key will belong.
 *  aiAlgid [I] The new key shall use the crypto algorithm idenfied by aiAlgid.
 *  dwFlags [I] Upper 16 bits give the key length.
 *              Lower 16 bits: CRYPT_CREATE_SALT, CRYPT_NO_SALT
 *  pbKey   [I] Byte stream to be used as key material. May be NULL.
 *  hHash   [I] Handle to a hash object whose value will be used as key material. May be zero.
 *
 * RETURNS
 *  Success: Handle to the created key.
 *  Failure: INVALID_HANDLE_VALUE
 */
static HCRYPTKEY new_key(HCRYPTPROV hProv, ALG_ID aiAlgid, DWORD dwFlags, BYTE *pbKey, 
                         HCRYPTHASH hHash)
{
    KEYCONTAINER *pKeyContainer;
    HCRYPTKEY hCryptKey;
    CRYPTKEY *pCryptKey;
    CRYPTHASH *pCryptHash;
    DWORD dwKeyLen = HIWORD(dwFlags), i;
    const PROV_ENUMALGS_EX *peaAlgidInfo;
    BYTE abHashValue[RSAENH_MAX_HASH_SIZE*2];

    if (!lookup_handle(&handle_table, hProv, RSAENH_MAGIC_CONTAINER, (OBJECTHDR**)&pKeyContainer))
    {
        SetLastError(NTE_BAD_UID);
        return (HCRYPTKEY)INVALID_HANDLE_VALUE;
    }
    
    /* 
     * Retrieve the CSP's capabilities for the given ALG_ID value
     */
    peaAlgidInfo = get_algid_info(pKeyContainer, aiAlgid);
    if (!peaAlgidInfo) {
        SetLastError(NTE_BAD_ALGID);
        return (HCRYPTKEY)INVALID_HANDLE_VALUE;
    }

    /*
     * Assume the default key length, if none is specified explicitly
     */
    if (dwKeyLen == 0) dwKeyLen = peaAlgidInfo->dwDefaultLen;
    
    /*
     * Check if the requested key length is supported by the current CSP.
     * Adjust key length's for DES algorithms.
     */
    switch (aiAlgid) {
        case CALG_DES:
            if (dwKeyLen == RSAENH_DES_EFFECTIVE_KEYLEN) {
                dwKeyLen = RSAENH_DES_STORAGE_KEYLEN;
            }
            if (dwKeyLen != RSAENH_DES_STORAGE_KEYLEN) {
                SetLastError(NTE_BAD_FLAGS);
                return (HCRYPTKEY)INVALID_HANDLE_VALUE;
            }
            break;

        case CALG_3DES_112:
            if (dwKeyLen == RSAENH_3DES112_EFFECTIVE_KEYLEN) {
                dwKeyLen = RSAENH_3DES112_STORAGE_KEYLEN;
            }
            if (dwKeyLen != RSAENH_3DES112_STORAGE_KEYLEN) {
                SetLastError(NTE_BAD_FLAGS);
                return (HCRYPTKEY)INVALID_HANDLE_VALUE;
            }
            break;

        case CALG_3DES:
            if (dwKeyLen == RSAENH_3DES_EFFECTIVE_KEYLEN) {
                dwKeyLen = RSAENH_3DES_STORAGE_KEYLEN;
            }
            if (dwKeyLen != RSAENH_3DES_STORAGE_KEYLEN) {
                SetLastError(NTE_BAD_FLAGS);
                return (HCRYPTKEY)INVALID_HANDLE_VALUE;
            }
            break;
        
        default:
            if (dwKeyLen % 8 || 
                dwKeyLen > peaAlgidInfo->dwMaxLen || 
                dwKeyLen < peaAlgidInfo->dwMinLen) 
            {
                SetLastError(NTE_BAD_FLAGS);
                return (HCRYPTKEY)INVALID_HANDLE_VALUE;
            }
    }

    /* 
     * If a valid hash handle is supplied, we derive the key material from the hash.
     * If the hash value is not large enough for the claimed key, we have to construct
     * a larger binary value based on the hash. This is documented in MSDN: CryptDeriveKey.
     */
    if (lookup_handle(&handle_table, hHash, RSAENH_MAGIC_HASH, (OBJECTHDR**)&pCryptHash)) {
        DWORD dwLen = RSAENH_MAX_HASH_SIZE;

        RSAENH_CPGetHashParam(pCryptHash->hProv, hHash, HP_HASHVAL, abHashValue, &dwLen, 0);
    
        if (dwLen < (dwKeyLen >> 3)) {
            BYTE pad1[RSAENH_HMAC_DEF_PAD_LEN], pad2[RSAENH_HMAC_DEF_PAD_LEN], old_hashval[RSAENH_MAX_HASH_SIZE];

            memcpy(old_hashval, pCryptHash->abHashValue, RSAENH_MAX_HASH_SIZE);
            
            for (i=0; i<RSAENH_HMAC_DEF_PAD_LEN; i++) {
                pad1[i] = RSAENH_HMAC_DEF_IPAD_CHAR ^ (i<dwLen ? abHashValue[i] : 0);
                pad2[i] = RSAENH_HMAC_DEF_OPAD_CHAR ^ (i<dwLen ? abHashValue[i] : 0);
            }
                
            init_hash(pKeyContainer, pCryptHash);
            update_hash(pCryptHash, pad1, RSAENH_HMAC_DEF_PAD_LEN);
            finalize_hash(pCryptHash);
            memcpy(abHashValue, pCryptHash->abHashValue, pCryptHash->dwHashSize);

            init_hash(pKeyContainer, pCryptHash);
            update_hash(pCryptHash, pad2, RSAENH_HMAC_DEF_PAD_LEN);
            finalize_hash(pCryptHash);
            memcpy(abHashValue+pCryptHash->dwHashSize, pCryptHash->abHashValue, 
                   pCryptHash->dwHashSize);

            memcpy(pCryptHash->abHashValue, old_hashval, RSAENH_MAX_HASH_SIZE);
        }

        pbKey = abHashValue;
    }
    
    hCryptKey = (HCRYPTKEY)new_object(&handle_table, sizeof(CRYPTKEY), RSAENH_MAGIC_KEY, 
                                      destroy_key, (OBJECTHDR**)&pCryptKey);
    if (hCryptKey != (HCRYPTKEY)INVALID_HANDLE_VALUE)
    {
        pCryptKey->aiAlgid = aiAlgid;
        pCryptKey->hProv = hProv;
        pCryptKey->dwModeBits = 0;
        pCryptKey->dwPermissions = CRYPT_ENCRYPT | CRYPT_DECRYPT | CRYPT_READ | CRYPT_WRITE | 
                                   CRYPT_MAC;
        pCryptKey->dwKeyLen = dwKeyLen >> 3;
        if ((dwFlags & CRYPT_CREATE_SALT) || (dwKeyLen == 40 && !(dwFlags & CRYPT_NO_SALT))) 
            pCryptKey->dwSaltLen = 16 - pCryptKey->dwKeyLen;
        else
            pCryptKey->dwSaltLen = 0;
        memset(pCryptKey->abKeyValue, 0, sizeof(pCryptKey->abKeyValue));
        if (pbKey) memcpy(pCryptKey->abKeyValue, pbKey, RSAENH_MIN(pCryptKey->dwKeyLen, 
                          sizeof(pCryptKey->abKeyValue)));
        memset(pCryptKey->abInitVector, 0, sizeof(pCryptKey->abInitVector));
            
        switch(aiAlgid)
        {
            case CALG_RC4:
                pCryptKey->dwBlockLen = 0;
                pCryptKey->dwMode = 0;
                break;

            case CALG_RC2:
            case CALG_DES:
            case CALG_3DES_112:
            case CALG_3DES:
                pCryptKey->dwBlockLen = 8;
                pCryptKey->dwMode = CRYPT_MODE_CBC;
                break;

            case CALG_RSA_KEYX:
            case CALG_RSA_SIGN:
                pCryptKey->dwBlockLen = dwKeyLen >> 3;
                pCryptKey->dwMode = 0;
                break;
        }

        new_key_impl(pCryptKey->aiAlgid, &pCryptKey->context, pCryptKey->dwKeyLen);
        setup_key(pCryptKey);
    }

    return hCryptKey;
}

/******************************************************************************
 * destroy_key_container [Internal]
 *
 * Destructor for key containers. The user's signature and key exchange private
 * keys are stored in the registry _IN_PLAINTEXT_.
 * 
 * PARAMS
 *  pObjectHdr [I] Pointer to the key container to be destroyed.
 */
static void destroy_key_container(OBJECTHDR *pObjectHdr)
{
    KEYCONTAINER *pKeyContainer = (KEYCONTAINER*)pObjectHdr;
    CRYPTKEY *pKey;
    CHAR szRSABase[MAX_PATH];
    HKEY hKey;
    DWORD dwLen;
    BYTE *pbKey;

    /* On WinXP, persistent keys are stored in a file located at: 
     * $AppData$\\Microsoft\\Crypto\\RSA\\$SID$\\some_hex_string 
     */
    sprintf(szRSABase, "Software\\Wine\\Crypto\\RSA\\%s", pKeyContainer->szName);

    if (RegCreateKeyExA(HKEY_CURRENT_USER, szRSABase, 0, NULL, REG_OPTION_NON_VOLATILE, 
                        KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        if (lookup_handle(&handle_table, pKeyContainer->hKeyExchangeKeyPair, RSAENH_MAGIC_KEY, 
                          (OBJECTHDR**)&pKey))
        {
            if (RSAENH_CPExportKey(pKey->hProv, pKeyContainer->hKeyExchangeKeyPair, 0, 
                                   PRIVATEKEYBLOB, 0, 0, &dwLen)) 
            {
                pbKey = (BYTE*)HeapAlloc(GetProcessHeap(), 0, dwLen);
                if (pbKey) 
                {
                    if (RSAENH_CPExportKey(pKey->hProv, pKeyContainer->hKeyExchangeKeyPair, 0,
                                           PRIVATEKEYBLOB, 0, pbKey, &dwLen))
                    {
                        RegSetValueExA(hKey, "KeyExchangeKeyPair", 0, REG_BINARY, pbKey, dwLen);
                    }
                    HeapFree(GetProcessHeap(), 0, pbKey);
                }
            }
            release_handle(&handle_table, (unsigned int)pKeyContainer->hKeyExchangeKeyPair, 
                           RSAENH_MAGIC_KEY);
        }

        if (lookup_handle(&handle_table, pKeyContainer->hSignatureKeyPair, RSAENH_MAGIC_KEY, 
                          (OBJECTHDR**)&pKey))
        {
            if (RSAENH_CPExportKey(pKey->hProv, pKeyContainer->hSignatureKeyPair, 0, PRIVATEKEYBLOB,
                                   0, 0, &dwLen)) 
            {
                pbKey = (BYTE*)HeapAlloc(GetProcessHeap(), 0, dwLen);
                if (pbKey) 
                {
                    if (RSAENH_CPExportKey(pKey->hProv, pKeyContainer->hSignatureKeyPair, 0, 
                                           PRIVATEKEYBLOB, 0, pbKey, &dwLen))
                    {
                        RegSetValueExA(hKey, "SignatureKeyPair", 0, REG_BINARY, pbKey, dwLen);
                    }
                    HeapFree(GetProcessHeap(), 0, pbKey);
                }
            }
            release_handle(&handle_table, (unsigned int)pKeyContainer->hSignatureKeyPair, 
                           RSAENH_MAGIC_KEY);
        }
        
        RegCloseKey(hKey);
    }
    
    HeapFree( GetProcessHeap(), 0, pKeyContainer );
}

/******************************************************************************
 * new_key_container [Internal]
 *
 * Create a new key container. The personality (RSA Base, Strong or Enhanced CP) 
 * of the CSP is determined via the pVTable->pszProvName string.
 *
 * PARAMS
 *  pszContainerName [I] Name of the key container.
 *  pVTable          [I] Callback functions and context info provided by the OS
 *
 * RETURNS
 *  Success: Handle to the new key container.
 *  Failure: INVALID_HANDLE_VALUE
 */
static HCRYPTPROV new_key_container(PCHAR pszContainerName, PVTableProvStruc pVTable)
{
    KEYCONTAINER *pKeyContainer;
    HCRYPTPROV hKeyContainer;

    hKeyContainer = (HCRYPTPROV)new_object(&handle_table, sizeof(KEYCONTAINER), RSAENH_MAGIC_CONTAINER,
                                           destroy_key_container, (OBJECTHDR**)&pKeyContainer);
    if (hKeyContainer != (HCRYPTPROV)INVALID_HANDLE_VALUE)
    {
        strncpy(pKeyContainer->szName, pszContainerName, MAX_PATH);
        pKeyContainer->szName[MAX_PATH-1] = '\0';
        pKeyContainer->dwMode = 0;
        pKeyContainer->dwEnumAlgsCtr = 0;
        pKeyContainer->hKeyExchangeKeyPair = (HCRYPTKEY)INVALID_HANDLE_VALUE;
        pKeyContainer->hSignatureKeyPair = (HCRYPTKEY)INVALID_HANDLE_VALUE;
        if (pVTable && pVTable->pszProvName) {
            strncpy(pKeyContainer->szProvName, pVTable->pszProvName, MAX_PATH);
            pKeyContainer->szProvName[MAX_PATH-1] = '\0';
            if (!strcmp(pVTable->pszProvName, MS_DEF_PROV_A)) {
                pKeyContainer->dwPersonality = RSAENH_PERSONALITY_BASE;
            } else if (!strcmp(pVTable->pszProvName, MS_ENHANCED_PROV_A)) {
                pKeyContainer->dwPersonality = RSAENH_PERSONALITY_ENHANCED;
            } else {
                pKeyContainer->dwPersonality = RSAENH_PERSONALITY_STRONG;
            }
        }
    }

    return hKeyContainer;
}

/******************************************************************************
 * read_key_container [Internal]
 *
 * Tries to read the persistent state of the key container (mainly the signature
 * and key exchange private keys) given by pszContainerName.
 *
 * PARAMS
 *  pszContainerName [I] Name of the key container to read from the registry
 *  pVTable          [I] Pointer to context data provided by the operating system
 *
 * RETURNS
 *  Success: Handle to the key container read from the registry
 *  Failure: INVALID_HANDLE_VALUE
 */
static HCRYPTPROV read_key_container(PCHAR pszContainerName, PVTableProvStruc pVTable)
{
    CHAR szRSABase[MAX_PATH];
    BYTE *pbKey;
    HKEY hKey;
    DWORD dwValueType, dwLen;
    KEYCONTAINER *pKeyContainer;
    HCRYPTPROV hKeyContainer;
    
    sprintf(szRSABase, "Software\\Wine\\Crypto\\RSA\\%s", pszContainerName);

    if (RegOpenKeyExA(HKEY_CURRENT_USER, szRSABase, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        SetLastError(NTE_BAD_KEYSET);
        return (HCRYPTPROV)INVALID_HANDLE_VALUE;
    }

    hKeyContainer = new_key_container(pszContainerName, pVTable);
    if (hKeyContainer != (HCRYPTPROV)INVALID_HANDLE_VALUE)
    {
        if (!lookup_handle(&handle_table, hKeyContainer, RSAENH_MAGIC_CONTAINER, 
                           (OBJECTHDR**)&pKeyContainer))
            return (HCRYPTPROV)INVALID_HANDLE_VALUE;
    
        if (RegQueryValueExA(hKey, "KeyExchangeKeyPair", 0, &dwValueType, NULL, &dwLen) == 
            ERROR_SUCCESS) 
        {
            pbKey = (BYTE*)HeapAlloc(GetProcessHeap(), 0, dwLen);
            if (pbKey) 
            {
                if (RegQueryValueExA(hKey, "KeyExchangeKeyPair", 0, &dwValueType, pbKey, &dwLen) ==
                    ERROR_SUCCESS)
                {
                    RSAENH_CPImportKey(hKeyContainer, pbKey, dwLen, 0, 0, 
                                       &pKeyContainer->hKeyExchangeKeyPair);
                }
                HeapFree(GetProcessHeap(), 0, pbKey);
            }
        }

        if (RegQueryValueExA(hKey, "SignatureKeyPair", 0, &dwValueType, NULL, &dwLen) == 
            ERROR_SUCCESS) 
        {
            pbKey = (BYTE*)HeapAlloc(GetProcessHeap(), 0, dwLen);
            if (pbKey) 
            {
                if (RegQueryValueExA(hKey, "SignatureKeyPair", 0, &dwValueType, pbKey, &dwLen) == 
                    ERROR_SUCCESS)
                {
                    RSAENH_CPImportKey(hKeyContainer, pbKey, dwLen, 0, 0, 
                                       &pKeyContainer->hSignatureKeyPair);
                }
                HeapFree(GetProcessHeap(), 0, pbKey);
            }
        }
    }

    return hKeyContainer;
}

/******************************************************************************
 * CPAcquireContext (RSAENH.@)
 *
 * Acquire a handle to the key container specified by pszContainer
 *
 * PARAMS
 *  phProv       [O] Pointer to the location the acquired handle will be written to.
 *  pszContainer [I] Name of the desired key container. See Notes
 *  dwFlags      [I] Flags. See Notes.
 *  pVTable      [I] Pointer to a PVTableProvStruct containing callbacks.
 * 
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  If pszContainer is NULL or points to a zero length string the user's login 
 *  name will be used as the key container name.
 *
 *  If the CRYPT_NEW_KEYSET flag is set in dwFlags a new keyset will be created.
 *  If a keyset with the given name already exists, the function fails and sets
 *  last error to NTE_EXISTS. If CRYPT_NEW_KEYSET is not set and the specified
 *  key container does not exist, function fails and sets last error to 
 *  NTE_BAD_KEYSET.
 */                         
BOOL WINAPI RSAENH_CPAcquireContext(HCRYPTPROV *phProv, LPSTR pszContainer,
                   DWORD dwFlags, PVTableProvStruc pVTable)
{
    DWORD dwLen;
    CHAR szKeyContainerName[MAX_PATH] = "";

    TRACE("(phProv=%p, pszContainer=%s, dwFlags=%08lx, pVTable=%p)\n", phProv, 
          debugstr_a(pszContainer), dwFlags, pVTable);

    SetLastError(ERROR_SUCCESS);
    
    if (pszContainer ? strlen(pszContainer) : 0) 
    {
        strncpy(szKeyContainerName, pszContainer, MAX_PATH);
        szKeyContainerName[MAX_PATH-1] = '\0';
    } 
    else
    {
        dwLen = MAX_PATH;
        if (!GetUserNameA(szKeyContainerName, &dwLen)) return FALSE;
    }
        
    switch (dwFlags) 
    {
        case 0:
            *phProv = read_key_container(szKeyContainerName, pVTable);
            break;

        case CRYPT_NEWKEYSET:
            *phProv = read_key_container(szKeyContainerName, pVTable);
            if (*phProv != (HCRYPTPROV)INVALID_HANDLE_VALUE) 
            {
                release_handle(&handle_table, (unsigned int)*phProv, RSAENH_MAGIC_CONTAINER);
                SetLastError(NTE_EXISTS);
                return FALSE;
            }
            *phProv = new_key_container(szKeyContainerName, pVTable);
            break;
                    
        default:
            *phProv = (unsigned int)INVALID_HANDLE_VALUE;
            SetLastError(NTE_BAD_FLAGS);
            return FALSE;
    }
                
    return *phProv != (unsigned int)INVALID_HANDLE_VALUE;
}

/******************************************************************************
 * CPCreateHash (RSAENH.@)
 *
 * CPCreateHash creates and initalizes a new hash object.
 *
 * PARAMS
 *  hProv   [I] Handle to the key container to which the new hash will belong.
 *  Algid   [I] Identifies the hash algorithm, which will be used for the hash.
 *  hKey    [I] Handle to a session key applied for keyed hashes.
 *  dwFlags [I] Currently no flags defined. Must be zero.
 *  phHash  [O] Points to the location where a handle to the new hash will be stored.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  hKey is a handle to a session key applied in keyed hashes like MAC and HMAC.
 *  If a normal hash object is to be created (like e.g. MD2 or SHA1) hKey must be zero.
 */
BOOL WINAPI RSAENH_CPCreateHash(HCRYPTPROV hProv, ALG_ID Algid, HCRYPTKEY hKey, DWORD dwFlags, 
                                HCRYPTHASH *phHash)
{
    KEYCONTAINER *pKeyContainer;
    CRYPTHASH *pCryptHash;
    const PROV_ENUMALGS_EX *peaAlgidInfo;
        
    TRACE("(hProv=%08lx, Algid=%08x, hKey=%08lx, dwFlags=%08lx, phHash=%p)\n", hProv, Algid, hKey, 
          dwFlags, phHash);

    if (!lookup_handle(&handle_table, hProv, RSAENH_MAGIC_CONTAINER, (OBJECTHDR**)&pKeyContainer)) 
    {
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }

    peaAlgidInfo = get_algid_info(pKeyContainer, Algid);
    if (!peaAlgidInfo)
    {
        SetLastError(NTE_BAD_ALGID);
        return FALSE;
    }

    if (dwFlags)
    {
        SetLastError(NTE_BAD_FLAGS);
        return FALSE;
    }

    if ((Algid == CALG_MAC || Algid == CALG_HMAC)) {
        CRYPTKEY *pCryptKey;

        if (!lookup_handle(&handle_table, hKey, RSAENH_MAGIC_KEY, (OBJECTHDR**)&pCryptKey)) {
            SetLastError(NTE_BAD_KEY);
            return FALSE;
        }

        if ((Algid == CALG_MAC) && (GET_ALG_TYPE(pCryptKey->aiAlgid) != ALG_TYPE_BLOCK)) {
            SetLastError(NTE_BAD_KEY);
            return FALSE;
        }
    }

    *phHash = (HCRYPTHASH)new_object(&handle_table, sizeof(CRYPTHASH), RSAENH_MAGIC_HASH,
                                     destroy_hash, (OBJECTHDR**)&pCryptHash);
    if (!pCryptHash) return FALSE;
    
    pCryptHash->aiAlgid = Algid;
    pCryptHash->hKey = hKey;
    pCryptHash->hProv = hProv;
    pCryptHash->dwState = RSAENH_HASHSTATE_IDLE;
    pCryptHash->pHMACInfo = (PHMAC_INFO)NULL;
    pCryptHash->dwHashSize = peaAlgidInfo->dwDefaultLen >> 3;
    
    return init_hash(pKeyContainer, pCryptHash);
}

/******************************************************************************
 * CPDestroyHash (RSAENH.@)
 * 
 * Releases the handle to a hash object. The object is destroyed if it's reference
 * count reaches zero.
 *
 * PARAMS
 *  hProv [I] Handle to the key container to which the hash object belongs.
 *  hHash [I] Handle to the hash object to be released.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE 
 */
BOOL WINAPI RSAENH_CPDestroyHash(HCRYPTPROV hProv, HCRYPTHASH hHash)
{
    TRACE("(hProv=%08lx, hHash=%08lx)\n", hProv, hHash);
     
    if (!is_valid_handle(&handle_table, hProv, RSAENH_MAGIC_CONTAINER))
    {
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }
        
    if (!release_handle(&handle_table, hHash, RSAENH_MAGIC_HASH)) 
    {
        SetLastError(NTE_BAD_HASH);
        return FALSE;
    }
    
    return TRUE;
}

/******************************************************************************
 * CPDestroyKey (RSAENH.@)
 *
 * Releases the handle to a key object. The object is destroyed if it's reference
 * count reaches zero.
 *
 * PARAMS
 *  hProv [I] Handle to the key container to which the key object belongs.
 *  hKey  [I] Handle to the key object to be released.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI RSAENH_CPDestroyKey(HCRYPTPROV hProv, HCRYPTKEY hKey)
{
    TRACE("(hProv=%08lx, hKey=%08lx)\n", hProv, hKey);
        
    if (!is_valid_handle(&handle_table, hProv, RSAENH_MAGIC_CONTAINER))
    {
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }
        
    if (!release_handle(&handle_table, hKey, RSAENH_MAGIC_KEY)) 
    {
        SetLastError(NTE_BAD_KEY);
        return FALSE;
    }
    
    return TRUE;
}

/******************************************************************************
 * CPDuplicateHash (RSAENH.@)
 *
 * Clones a hash object including it's current state.
 *
 * PARAMS
 *  hUID        [I] Handle to the key container the hash belongs to.
 *  hHash       [I] Handle to the hash object to be cloned.
 *  pdwReserved [I] Reserved. Must be NULL.
 *  dwFlags     [I] No flags are currently defined. Must be 0.
 *  phHash      [O] Handle to the cloned hash object.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 */
BOOL WINAPI RSAENH_CPDuplicateHash(HCRYPTPROV hUID, HCRYPTHASH hHash, DWORD *pdwReserved, 
                                   DWORD dwFlags, HCRYPTHASH *phHash)
{
    CRYPTHASH *pSrcHash, *pDestHash;
    
    TRACE("(hUID=%08lx, hHash=%08lx, pdwReserved=%p, dwFlags=%08lx, phHash=%p)\n", hUID, hHash, 
           pdwReserved, dwFlags, phHash);

    if (!is_valid_handle(&handle_table, hUID, RSAENH_MAGIC_CONTAINER))
    {
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }

    if (!lookup_handle(&handle_table, hHash, RSAENH_MAGIC_HASH, (OBJECTHDR**)&pSrcHash))
    {
        SetLastError(NTE_BAD_HASH);
        return FALSE;
    }

    if (!phHash || pdwReserved || dwFlags) 
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *phHash = (HCRYPTHASH)new_object(&handle_table, sizeof(CRYPTHASH), RSAENH_MAGIC_HASH, 
                                     destroy_hash, (OBJECTHDR**)&pDestHash);
    if (*phHash != (HCRYPTHASH)INVALID_HANDLE_VALUE)
    {
        memcpy(pDestHash, pSrcHash, sizeof(CRYPTHASH));
        duplicate_hash_impl(pSrcHash->aiAlgid, &pSrcHash->context, &pDestHash->context);
        copy_hmac_info(&pDestHash->pHMACInfo, pSrcHash->pHMACInfo);
    }

    return *phHash != (HCRYPTHASH)INVALID_HANDLE_VALUE;
}

/******************************************************************************
 * CPDuplicateKey (RSAENH.@)
 *
 * Clones a key object including it's current state.
 *
 * PARAMS
 *  hUID        [I] Handle to the key container the hash belongs to.
 *  hKey        [I] Handle to the key object to be cloned.
 *  pdwReserved [I] Reserved. Must be NULL.
 *  dwFlags     [I] No flags are currently defined. Must be 0.
 *  phHash      [O] Handle to the cloned key object.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 */
BOOL WINAPI RSAENH_CPDuplicateKey(HCRYPTPROV hUID, HCRYPTKEY hKey, DWORD *pdwReserved, 
                                  DWORD dwFlags, HCRYPTKEY *phKey)
{
    CRYPTKEY *pSrcKey, *pDestKey;
    
    TRACE("(hUID=%08lx, hKey=%08lx, pdwReserved=%p, dwFlags=%08lx, phKey=%p)\n", hUID, hKey, 
          pdwReserved, dwFlags, phKey);

    if (!is_valid_handle(&handle_table, hUID, RSAENH_MAGIC_CONTAINER))
    {
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }

    if (!lookup_handle(&handle_table, hKey, RSAENH_MAGIC_KEY, (OBJECTHDR**)&pSrcKey))
    {
        SetLastError(NTE_BAD_KEY);
        return FALSE;
    }

    if (!phKey || pdwReserved || dwFlags) 
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *phKey = (HCRYPTKEY)new_object(&handle_table, sizeof(CRYPTKEY), RSAENH_MAGIC_KEY, destroy_key, 
                                   (OBJECTHDR**)&pDestKey);
    if (*phKey != (HCRYPTKEY)INVALID_HANDLE_VALUE)
    {
        memcpy(pDestKey, pSrcKey, sizeof(CRYPTKEY));
        duplicate_key_impl(pSrcKey->aiAlgid, &pSrcKey->context, &pDestKey->context);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/******************************************************************************
 * CPEncrypt (RSAENH.@)
 *
 * Encrypt data.
 *
 * PARAMS
 *  hProv      [I]   The key container hKey and hHash belong to.
 *  hKey       [I]   The key used to encrypt the data.
 *  hHash      [I]   An optional hash object for parallel hashing. See notes.
 *  Final      [I]   Indicates if this is the last block of data to encrypt.
 *  dwFlags    [I]   Currently no flags defined. Must be zero.
 *  pbData     [I/O] Pointer to the data to encrypt. Encrypted data will also be stored there. 
 *  pdwDataLen [I/O] I: Length of data to encrypt, O: Length of encrypted data.
 *  dwBufLen   [I]   Size of the buffer at pbData.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 *
 * NOTES
 *  If a hash object handle is provided in hHash, it will be updated with the plaintext. 
 *  This is useful for message signatures.
 *
 *  This function uses the standard WINAPI protocol for querying data of dynamic length. 
 * 
 * FIXME
 *  Parallel hashing not yet implemented.
 */
BOOL WINAPI RSAENH_CPEncrypt(HCRYPTPROV hProv, HCRYPTKEY hKey, HCRYPTHASH hHash, BOOL Final, 
                             DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen, DWORD dwBufLen)
{
    CRYPTKEY *pCryptKey;
    BYTE *in, out[RSAENH_MAX_BLOCK_SIZE], o[RSAENH_MAX_BLOCK_SIZE];
    DWORD dwEncryptedLen, i, j, k;
        
    TRACE("(hProv=%08lx, hKey=%08lx, hHash=%08lx, Final=%d, dwFlags=%08lx, pbData=%p, "
          "pdwDataLen=%p, dwBufLen=%ld)\n", hProv, hKey, hHash, Final, dwFlags, pbData, pdwDataLen,
          dwBufLen);
    
    if (!is_valid_handle(&handle_table, hProv, RSAENH_MAGIC_CONTAINER))
    {
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }

    if (dwFlags)
    {
        SetLastError(NTE_BAD_FLAGS);
        return FALSE;
    }

    if (!lookup_handle(&handle_table, hKey, RSAENH_MAGIC_KEY, (OBJECTHDR**)&pCryptKey))
    {
        SetLastError(NTE_BAD_KEY);
        return FALSE;
    }

    if (pCryptKey->dwState == RSAENH_KEYSTATE_IDLE) 
        pCryptKey->dwState = RSAENH_KEYSTATE_ENCRYPTING;

    if (pCryptKey->dwState != RSAENH_KEYSTATE_ENCRYPTING) 
    {
        SetLastError(NTE_BAD_DATA);
        return FALSE;
    }

    if (GET_ALG_TYPE(pCryptKey->aiAlgid) == ALG_TYPE_BLOCK) {
        if (!Final && (*pdwDataLen % pCryptKey->dwBlockLen)) {
            SetLastError(NTE_BAD_DATA);
            return FALSE;
        }

        dwEncryptedLen = (*pdwDataLen/pCryptKey->dwBlockLen+(Final?1:0))*pCryptKey->dwBlockLen;
        for (i=*pdwDataLen; i<dwEncryptedLen; i++) pbData[i] = dwEncryptedLen - *pdwDataLen;
        *pdwDataLen = dwEncryptedLen; 

        if (*pdwDataLen > dwBufLen) 
        {
            SetLastError(ERROR_MORE_DATA);
            return FALSE;
        }
    
        for (i=0, in=pbData; i<*pdwDataLen; i+=pCryptKey->dwBlockLen, in+=pCryptKey->dwBlockLen) {
            switch (pCryptKey->dwMode) {
                case CRYPT_MODE_ECB:
                    encrypt_block_impl(pCryptKey->aiAlgid, &pCryptKey->context, in, out, 
                                       RSAENH_ENCRYPT);
                    break;
                
                case CRYPT_MODE_CBC:
                    for (j=0; j<pCryptKey->dwBlockLen; j++) in[j] ^= pCryptKey->abChainVector[j];
                    encrypt_block_impl(pCryptKey->aiAlgid, &pCryptKey->context, in, out, 
                                       RSAENH_ENCRYPT);
                    memcpy(pCryptKey->abChainVector, out, pCryptKey->dwBlockLen);
                    break;

                case CRYPT_MODE_CFB:
                    for (j=0; j<pCryptKey->dwBlockLen; j++) {
                        encrypt_block_impl(pCryptKey->aiAlgid, &pCryptKey->context, 
                                           pCryptKey->abChainVector, o, RSAENH_ENCRYPT);
                        out[j] = in[j] ^ o[0];
                        for (k=0; k<pCryptKey->dwBlockLen-1; k++) 
                            pCryptKey->abChainVector[k] = pCryptKey->abChainVector[k+1];
                        pCryptKey->abChainVector[k] = out[j];
                    }
                    break;
                    
                default:
                    SetLastError(NTE_BAD_ALGID);
                    return FALSE;
            }
            memcpy(in, out, pCryptKey->dwBlockLen); 
        }
    } else if (GET_ALG_TYPE(pCryptKey->aiAlgid) == ALG_TYPE_STREAM) {
        encrypt_stream_impl(pCryptKey->aiAlgid, &pCryptKey->context, pbData, *pdwDataLen);
    }

    if (Final) setup_key(pCryptKey);

    return TRUE;
}

/******************************************************************************
 * CPDecrypt (RSAENH.@)
 *
 * Decrypt data.
 *
 * PARAMS
 *  hProv      [I]   The key container hKey and hHash belong to.
 *  hKey       [I]   The key used to decrypt the data.
 *  hHash      [I]   An optional hash object for parallel hashing. See notes.
 *  Final      [I]   Indicates if this is the last block of data to decrypt.
 *  dwFlags    [I]   Currently no flags defined. Must be zero.
 *  pbData     [I/O] Pointer to the data to decrypt. Plaintext will also be stored there. 
 *  pdwDataLen [I/O] I: Length of ciphertext, O: Length of plaintext.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 *
 * NOTES
 *  If a hash object handle is provided in hHash, it will be updated with the plaintext. 
 *  This is useful for message signatures.
 *
 *  This function uses the standard WINAPI protocol for querying data of dynamic length. 
 * 
 * FIXME
 *  Parallel hashing not yet implemented.
 */
BOOL WINAPI RSAENH_CPDecrypt(HCRYPTPROV hProv, HCRYPTKEY hKey, HCRYPTHASH hHash, BOOL Final, 
                             DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen)
{
    CRYPTKEY *pCryptKey;
    BYTE *in, out[RSAENH_MAX_BLOCK_SIZE], o[RSAENH_MAX_BLOCK_SIZE];
    DWORD i, j, k;

    TRACE("(hProv=%08lx, hKey=%08lx, hHash=%08lx, Final=%d, dwFlags=%08lx, pbData=%p, "
          "pdwDataLen=%p)\n", hProv, hKey, hHash, Final, dwFlags, pbData, pdwDataLen);
    
    if (!is_valid_handle(&handle_table, hProv, RSAENH_MAGIC_CONTAINER))
    {
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }

    if (dwFlags)
    {
        SetLastError(NTE_BAD_FLAGS);
        return FALSE;
    }

    if (!lookup_handle(&handle_table, hKey, RSAENH_MAGIC_KEY, (OBJECTHDR**)&pCryptKey))
    {
        SetLastError(NTE_BAD_KEY);
        return FALSE;
    }

    if (pCryptKey->dwState == RSAENH_KEYSTATE_IDLE) 
        pCryptKey->dwState = RSAENH_KEYSTATE_DECRYPTING;

    if (pCryptKey->dwState != RSAENH_KEYSTATE_DECRYPTING)
    {
        SetLastError(NTE_BAD_DATA);
        return FALSE;
    }
    
    if (GET_ALG_TYPE(pCryptKey->aiAlgid) == ALG_TYPE_BLOCK) {
        for (i=0, in=pbData; i<*pdwDataLen; i+=pCryptKey->dwBlockLen, in+=pCryptKey->dwBlockLen) {
            switch (pCryptKey->dwMode) {
                case CRYPT_MODE_ECB:
                    encrypt_block_impl(pCryptKey->aiAlgid, &pCryptKey->context, in, out, 
                                       RSAENH_DECRYPT);
                    break;
                
                case CRYPT_MODE_CBC:
                    encrypt_block_impl(pCryptKey->aiAlgid, &pCryptKey->context, in, out, 
                                       RSAENH_DECRYPT);
                    for (j=0; j<pCryptKey->dwBlockLen; j++) out[j] ^= pCryptKey->abChainVector[j];
                    memcpy(pCryptKey->abChainVector, in, pCryptKey->dwBlockLen);
                    break;

                case CRYPT_MODE_CFB:
                    for (j=0; j<pCryptKey->dwBlockLen; j++) {
                        encrypt_block_impl(pCryptKey->aiAlgid, &pCryptKey->context, 
                                           pCryptKey->abChainVector, o, RSAENH_ENCRYPT);
                        out[j] = in[j] ^ o[0];
                        for (k=0; k<pCryptKey->dwBlockLen-1; k++) 
                            pCryptKey->abChainVector[k] = pCryptKey->abChainVector[k+1];
                        pCryptKey->abChainVector[k] = in[j];
                    }
                    break;
                    
                default:
                    SetLastError(NTE_BAD_ALGID);
                    return FALSE;
            }
            memcpy(in, out, pCryptKey->dwBlockLen);
        }
        if (Final) *pdwDataLen -= pbData[*pdwDataLen-1]; 

    } else if (GET_ALG_TYPE(pCryptKey->aiAlgid) == ALG_TYPE_STREAM) {
        encrypt_stream_impl(pCryptKey->aiAlgid, &pCryptKey->context, pbData, *pdwDataLen);
    }

    if (Final) setup_key(pCryptKey);

    return TRUE;
}

/******************************************************************************
 * CPExportKey (RSAENH.@)
 *
 * Export a key into a binary large object (BLOB).
 *
 * PARAMS
 *  hProv      [I]   Key container from which a key is to be exported.
 *  hKey       [I]   Key to be exported.
 *  hPubKey    [I]   Key used to encrypt sensitive BLOB data.
 *  dwBlobType [I]   SIMPLEBLOB, PUBLICKEYBLOB or PRIVATEKEYBLOB.
 *  dwFlags    [I]   Currently none defined.
 *  pbData     [O]   Pointer to a buffer where the BLOB will be written to.
 *  pdwDataLen [I/O] I: Size of buffer at pbData, O: Size of BLOB
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 */
BOOL WINAPI RSAENH_CPExportKey(HCRYPTPROV hProv, HCRYPTKEY hKey, HCRYPTKEY hPubKey, 
                               DWORD dwBlobType, DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen)
{
    CRYPTKEY *pCryptKey, *pPubKey;
    BLOBHEADER *pBlobHeader = (BLOBHEADER*)pbData;
    RSAPUBKEY *pRSAPubKey = (RSAPUBKEY*)(pBlobHeader+1);
    ALG_ID *pAlgid = (ALG_ID*)(pBlobHeader+1);
    DWORD dwDataLen, i;
    BYTE *pbRawData;
    
    TRACE("(hProv=%08lx, hKey=%08lx, hPubKey=%08lx, dwBlobType=%08lx, dwFlags=%08lx, pbData=%p,"
          "pdwDataLen=%p)\n", hProv, hKey, hPubKey, dwBlobType, dwFlags, pbData, pdwDataLen);
    
    if (!is_valid_handle(&handle_table, hProv, RSAENH_MAGIC_CONTAINER))
    {
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }

    if (!lookup_handle(&handle_table, hKey, RSAENH_MAGIC_KEY, (OBJECTHDR**)&pCryptKey))
    {
        SetLastError(NTE_BAD_KEY);
        return FALSE;
    }

    switch ((BYTE)dwBlobType)
    {
        case SIMPLEBLOB:
            if (!lookup_handle(&handle_table, hPubKey, RSAENH_MAGIC_KEY, (OBJECTHDR**)&pPubKey)){
                SetLastError(NTE_BAD_PUBLIC_KEY); /* FIXME: error_code? */
                return FALSE;
            }

            if (GET_ALG_CLASS(pCryptKey->aiAlgid) != ALG_CLASS_DATA_ENCRYPT) {
                SetLastError(NTE_BAD_KEY); /* FIXME: error code? */
                return FALSE;
            }

            dwDataLen = sizeof(BLOBHEADER) + sizeof(ALG_ID) + pPubKey->dwBlockLen;
            if (pbData) {
                if (*pdwDataLen < dwDataLen) {
                    SetLastError(ERROR_MORE_DATA);
                    *pdwDataLen = dwDataLen;
                    return FALSE;
                }

                pBlobHeader->bType = SIMPLEBLOB;
                pBlobHeader->bVersion = CUR_BLOB_VERSION;
                pBlobHeader->reserved = 0;
                pBlobHeader->aiKeyAlg = pCryptKey->aiAlgid;

                *pAlgid = pPubKey->aiAlgid;
        
                pbRawData = (BYTE*)(pAlgid+1);
                pbRawData[0] = 0x00;
                pbRawData[1] = RSAENH_PKC_BLOCKTYPE; 
                for (i=2; i < pPubKey->dwBlockLen - pCryptKey->dwKeyLen - 1; i++) 
                    do gen_rand_impl(&pbRawData[i], 1); while (!pbRawData[i]);
                pbRawData[i] = 0x00;
                for (i=0; i<pCryptKey->dwKeyLen; i++) 
                    pbRawData[pPubKey->dwBlockLen - pCryptKey->dwKeyLen + i] = 
                        pCryptKey->abKeyValue[i];
    
                encrypt_block_impl(pPubKey->aiAlgid, &pPubKey->context, pbRawData, pbRawData, 
                                   RSAENH_ENCRYPT); 
            }
            *pdwDataLen = dwDataLen;
            return TRUE;
            
        case PUBLICKEYBLOB:
            if (is_valid_handle(&handle_table, hPubKey, RSAENH_MAGIC_KEY)) {
                SetLastError(NTE_BAD_KEY); /* FIXME: error code? */
                return FALSE;
            }

            if ((pCryptKey->aiAlgid != CALG_RSA_KEYX) && (pCryptKey->aiAlgid != CALG_RSA_SIGN)) {
                SetLastError(NTE_BAD_KEY);
                return FALSE;
            }

            dwDataLen = sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) + pCryptKey->dwKeyLen;
            if (pbData) {
                if (*pdwDataLen < dwDataLen) {
                    SetLastError(ERROR_MORE_DATA);
                    *pdwDataLen = dwDataLen;
                    return FALSE;
                }

                pBlobHeader->bType = PUBLICKEYBLOB;
                pBlobHeader->bVersion = CUR_BLOB_VERSION;
                pBlobHeader->reserved = 0;
                pBlobHeader->aiKeyAlg = pCryptKey->aiAlgid;

                pRSAPubKey->magic = RSAENH_MAGIC_RSA1; 
                pRSAPubKey->bitlen = pCryptKey->dwKeyLen << 3;
        
                export_public_key_impl((BYTE*)(pRSAPubKey+1), &pCryptKey->context, 
                                       pCryptKey->dwKeyLen, &pRSAPubKey->pubexp);
            }
            *pdwDataLen = dwDataLen;
            return TRUE;

        case PRIVATEKEYBLOB:
            if ((pCryptKey->aiAlgid != CALG_RSA_KEYX) && (pCryptKey->aiAlgid != CALG_RSA_SIGN)) {
                SetLastError(NTE_BAD_KEY);
                return FALSE;
            }
    
            dwDataLen = sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) + 
                        2 * pCryptKey->dwKeyLen + 5 * ((pCryptKey->dwKeyLen + 1) >> 1);
            if (pbData) {
                if (*pdwDataLen < dwDataLen) {
                    SetLastError(ERROR_MORE_DATA);
                    *pdwDataLen = dwDataLen;
                    return FALSE;
                }
                
                pBlobHeader->bType = PRIVATEKEYBLOB;
                pBlobHeader->bVersion = CUR_BLOB_VERSION;
                pBlobHeader->reserved = 0;
                pBlobHeader->aiKeyAlg = pCryptKey->aiAlgid;

                pRSAPubKey->magic = RSAENH_MAGIC_RSA2;
                pRSAPubKey->bitlen = pCryptKey->dwKeyLen << 3;
                
                export_private_key_impl((BYTE*)(pRSAPubKey+1), &pCryptKey->context, 
                                        pCryptKey->dwKeyLen, &pRSAPubKey->pubexp);
            }
            *pdwDataLen = dwDataLen;
            return TRUE;
            
        default:
            SetLastError(NTE_BAD_TYPE); /* FIXME: error code? */
            return FALSE;
    }
}

/******************************************************************************
 * CPImportKey (RSAENH.@)
 *
 * Import a BLOB'ed key into a key container.
 *
 * PARAMS
 *  hProv     [I] Key container into which the key is to be imported.
 *  pbData    [I] Pointer to a buffer which holds the BLOB.
 *  dwDataLen [I] Length of data in buffer at pbData.
 *  hPubKey   [I] Key used to decrypt sensitive BLOB data.
 *  dwFlags   [I] Currently none defined.
 *  phKey     [O] Handle to the imported key.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 */
BOOL WINAPI RSAENH_CPImportKey(HCRYPTPROV hProv, CONST BYTE *pbData, DWORD dwDataLen, 
                               HCRYPTKEY hPubKey, DWORD dwFlags, HCRYPTKEY *phKey)
{
    KEYCONTAINER *pKeyContainer;
    CRYPTKEY *pCryptKey, *pPubKey;
    CONST BLOBHEADER *pBlobHeader = (CONST BLOBHEADER*)pbData;
    CONST RSAPUBKEY *pRSAPubKey = (CONST RSAPUBKEY*)(pBlobHeader+1);
    CONST ALG_ID *pAlgid = (CONST ALG_ID*)(pBlobHeader+1);
    CONST BYTE *pbKeyStream = (CONST BYTE*)(pAlgid + 1);
    BYTE *pbDecrypted, abKeyValue[RSAENH_MAX_KEY_SIZE];
    DWORD dwKeyLen, i;

    TRACE("(hProv=%08lx, pbData=%p, dwDataLen=%ld, hPubKey=%08lx, dwFlags=%08lx, phKey=%p)\n", 
        hProv, pbData, dwDataLen, hPubKey, dwFlags, phKey);
    
    if (!lookup_handle(&handle_table, hProv, RSAENH_MAGIC_CONTAINER, (OBJECTHDR**)&pKeyContainer))
    {
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }

    if (dwDataLen < sizeof(BLOBHEADER) || 
        pBlobHeader->bVersion != CUR_BLOB_VERSION ||
        pBlobHeader->reserved != 0) 
    {
        SetLastError(NTE_BAD_DATA);
        return FALSE;
    }

    switch (pBlobHeader->bType)
    {
        case PRIVATEKEYBLOB:    
            if ((dwDataLen < sizeof(BLOBHEADER) + sizeof(RSAPUBKEY)) || 
                (pRSAPubKey->magic != RSAENH_MAGIC_RSA2) ||
                (dwDataLen < sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) + 
                    (2 * pRSAPubKey->bitlen >> 3) + (5 * ((pRSAPubKey->bitlen+8)>>4)))) 
            {
                SetLastError(NTE_BAD_DATA);
                return FALSE;
            }
    
            *phKey = new_key(hProv, pBlobHeader->aiKeyAlg, MAKELONG(0,pRSAPubKey->bitlen), NULL, 
                             (HCRYPTHASH)0);
            if (!lookup_handle(&handle_table, *phKey, RSAENH_MAGIC_KEY, (OBJECTHDR**)&pCryptKey)) {
                SetLastError(NTE_FAIL);
                return FALSE;
            }
    
            return import_private_key_impl((CONST BYTE*)(pRSAPubKey+1), &pCryptKey->context, 
                                           pRSAPubKey->bitlen/8, pRSAPubKey->pubexp);
                
        case PUBLICKEYBLOB:
            if ((dwDataLen < sizeof(BLOBHEADER) + sizeof(RSAPUBKEY)) || 
                (pRSAPubKey->magic != RSAENH_MAGIC_RSA1) ||
                (dwDataLen < sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) + (pRSAPubKey->bitlen >> 3))) 
            {
                SetLastError(NTE_BAD_DATA);
                return FALSE;
            }
    
            *phKey = new_key(hProv, pBlobHeader->aiKeyAlg, MAKELONG(0,pRSAPubKey->bitlen), NULL, 
                             (HCRYPTHASH)0);
            if (!lookup_handle(&handle_table, *phKey, RSAENH_MAGIC_KEY, (OBJECTHDR**)&pCryptKey)) {
                SetLastError(NTE_FAIL);
                return FALSE;
            }
                
            return import_public_key_impl((CONST BYTE*)(pRSAPubKey+1), &pCryptKey->context, 
                                          pRSAPubKey->bitlen >> 3, pRSAPubKey->pubexp);
                
        case SIMPLEBLOB:
            if (!lookup_handle(&handle_table, hPubKey, RSAENH_MAGIC_KEY, (OBJECTHDR**)&pPubKey) ||
                pPubKey->aiAlgid != CALG_RSA_KEYX) 
            {
                SetLastError(NTE_BAD_PUBLIC_KEY); /* FIXME: error code? */
                return FALSE;
            }

            if (dwDataLen < sizeof(BLOBHEADER)+sizeof(ALG_ID)+pPubKey->dwBlockLen) 
            {
                SetLastError(NTE_BAD_DATA); /* FIXME: error code */
                return FALSE;
            }

            pbDecrypted = (BYTE*)HeapAlloc(GetProcessHeap(), 0, pPubKey->dwBlockLen);
            if (!pbDecrypted) return FALSE;
            encrypt_block_impl(pPubKey->aiAlgid, &pPubKey->context, pbKeyStream, pbDecrypted, 
                               RSAENH_DECRYPT);

            for (i=2; i<pPubKey->dwBlockLen && pbDecrypted[i]; i++); 
            if ((i==pPubKey->dwBlockLen) ||
                (pbDecrypted[0] != 0x00) ||
                (pbDecrypted[1] != RSAENH_PKC_BLOCKTYPE))
            {
                HeapFree(GetProcessHeap(), 0, pbDecrypted);
                SetLastError(NTE_BAD_DATA); /* FIXME: error code */
                return FALSE;
            }

            dwKeyLen = pPubKey->dwBlockLen-i-1;
            memcpy(abKeyValue, pbDecrypted+i+1, dwKeyLen);
            HeapFree(GetProcessHeap(), 0, pbDecrypted);
    
            *phKey = new_key(hProv, pBlobHeader->aiKeyAlg, dwKeyLen<<19, abKeyValue, 0);
            return *phKey != (HCRYPTKEY)INVALID_HANDLE_VALUE;

        default:
            SetLastError(NTE_BAD_TYPE); /* FIXME: error code? */
            return FALSE;
    }
}

/******************************************************************************
 * CPGenKey (RSAENH.@)
 *
 * Generate a key in the key container
 *
 * PARAMS
 *  hProv   [I] Key container for which a key is to be generated.
 *  Algid   [I] Crypto algorithm identifier for the key to be generated.
 *  dwFlags [I] Upper 16 bits: Binary length of key. Lower 16 bits: Flags. See Notes
 *  phKey   [O] Handle to the generated key.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 *
 * FIXME
 *  Flags currently not considered.
 *
 * NOTES
 *  Private key-exchange- and signature-keys can be generated with Algid AT_KEYEXCHANGE
 *  and AT_SIGNATURE values.
 */
BOOL WINAPI RSAENH_CPGenKey(HCRYPTPROV hProv, ALG_ID Algid, DWORD dwFlags, HCRYPTKEY *phKey)
{
    KEYCONTAINER *pKeyContainer;
    BYTE abKeyValue[2048];

    TRACE("(hProv=%08lx, aiAlgid=%d, dwFlags=%08lx, phKey=%p)\n", hProv, Algid, dwFlags, phKey);

    if (!lookup_handle(&handle_table, (unsigned int)hProv, RSAENH_MAGIC_CONTAINER, 
                       (OBJECTHDR**)&pKeyContainer)) 
    {
        /* MSDN: hProv not containing valid context handle */
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }
    
    switch (Algid)
    {
        case AT_SIGNATURE:
            RSAENH_CPDestroyKey(hProv, pKeyContainer->hSignatureKeyPair);
            pKeyContainer->hSignatureKeyPair = 
                new_key(hProv, CALG_RSA_SIGN, dwFlags, NULL, 0);
            copy_handle(&handle_table, pKeyContainer->hSignatureKeyPair, RSAENH_MAGIC_KEY, 
                        (unsigned int*)phKey);
            break;

        case AT_KEYEXCHANGE:
            RSAENH_CPDestroyKey(hProv, pKeyContainer->hKeyExchangeKeyPair);
            pKeyContainer->hKeyExchangeKeyPair = new_key(hProv, CALG_RSA_KEYX, dwFlags, NULL, 0);
            copy_handle(&handle_table, pKeyContainer->hKeyExchangeKeyPair, RSAENH_MAGIC_KEY, 
                        (unsigned int*)phKey);
            break;
     
        case CALG_RSA_SIGN:
        case CALG_RSA_KEYX:
            *phKey = new_key(hProv, Algid, dwFlags, NULL, 0);
            break;
            
        case CALG_RC2:
        case CALG_RC4:
        case CALG_DES:
            gen_rand_impl(abKeyValue, 2048);
            *phKey = new_key(hProv, Algid, dwFlags, abKeyValue, 0);
            break;
            
        default:
            /* MSDN: Algorithm not supported specified by Algid */
            SetLastError(NTE_BAD_ALGID);
            return FALSE;
    }
            
    return *phKey != (unsigned int)INVALID_HANDLE_VALUE;
}

/******************************************************************************
 * CPGenRandom (RSAENH.@)
 *
 * Generate a random byte stream.
 *
 * PARAMS
 *  hProv    [I] Key container that is used to generate random bytes.
 *  dwLen    [I] Specifies the number of requested random data bytes.
 *  pbBuffer [O] Random bytes will be stored here.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI RSAENH_CPGenRandom(HCRYPTPROV hProv, DWORD dwLen, BYTE *pbBuffer)
{
    KEYCONTAINER *pKeyContainer;

    TRACE("(hProv=%08lx, dwLen=%ld, pbBuffer=%p)\n", hProv, dwLen, pbBuffer);
    
    if (!lookup_handle(&handle_table, (unsigned int)hProv, RSAENH_MAGIC_CONTAINER, 
                       (OBJECTHDR**)&pKeyContainer)) 
    {
        /* MSDN: hProv not containing valid context handle */
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }

    return gen_rand_impl(pbBuffer, dwLen);
}

/******************************************************************************
 * CPGetHashParam (RSAENH.@)
 *
 * Query parameters of an hash object.
 *
 * PARAMS
 *  hProv      [I]   The kea container, which the hash belongs to.
 *  hHash      [I]   The hash object that is to be queried.
 *  dwParam    [I]   Specifies the parameter that is to be queried.
 *  pbData     [I]   Pointer to the buffer where the parameter value will be stored.
 *  pdwDataLen [I/O] I: Buffer length at pbData, O: Length of the parameter value.
 *  dwFlags    [I]   None currently defined.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  Valid dwParams are: HP_ALGID, HP_HASHSIZE, HP_HASHVALUE. The hash will be 
 *  finalized if HP_HASHVALUE is queried.
 */
BOOL WINAPI RSAENH_CPGetHashParam(HCRYPTPROV hProv, HCRYPTHASH hHash, DWORD dwParam, BYTE *pbData, 
                                  DWORD *pdwDataLen, DWORD dwFlags) 
{
    CRYPTHASH *pCryptHash;
    KEYCONTAINER *pKeyContainer;
    BYTE abHashValue[RSAENH_MAX_HASH_SIZE];
        
    TRACE("(hProv=%08lx, hHash=%08lx, dwParam=%08lx, pbData=%p, pdwDataLen=%p, dwFlags=%08lx)\n", 
        hProv, hHash, dwParam, pbData, pdwDataLen, dwFlags);
    
    if (!lookup_handle(&handle_table, (unsigned int)hProv, RSAENH_MAGIC_CONTAINER, 
                       (OBJECTHDR**)&pKeyContainer)) 
    {
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }

    if (dwFlags)
    {
        SetLastError(NTE_BAD_FLAGS);
        return FALSE;
    }
    
    if (!lookup_handle(&handle_table, (unsigned int)hHash, RSAENH_MAGIC_HASH, 
                       (OBJECTHDR**)&pCryptHash))
    {
        SetLastError(NTE_BAD_HASH);
        return FALSE;
    }

    if (!pdwDataLen)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    switch (dwParam)
    {
        case HP_ALGID:
            return copy_param(pbData, pdwDataLen, (CONST BYTE*)&pCryptHash->aiAlgid, 
                              sizeof(ALG_ID));

        case HP_HASHSIZE:
            return copy_param(pbData, pdwDataLen, (CONST BYTE*)&pCryptHash->dwHashSize, 
                              sizeof(DWORD));

        case HP_HASHVAL:
            if (pCryptHash->dwState == RSAENH_HASHSTATE_IDLE) {
                SetLastError(NTE_BAD_HASH_STATE);
                return FALSE;
            }
            
            if (pbData && (pCryptHash->dwState != RSAENH_HASHSTATE_FINISHED))
            {
                finalize_hash(pCryptHash);
                if (pCryptHash->aiAlgid == CALG_HMAC) {
                    memcpy(abHashValue, pCryptHash->abHashValue, pCryptHash->dwHashSize);
                    init_hash(pKeyContainer, pCryptHash);
                    update_hash(pCryptHash, pCryptHash->pHMACInfo->pbOuterString, 
                                pCryptHash->pHMACInfo->cbOuterString);
                    update_hash(pCryptHash, abHashValue, pCryptHash->dwHashSize);
                    finalize_hash(pCryptHash);
                } 
                        
                pCryptHash->dwState = RSAENH_HASHSTATE_FINISHED;
            }
            
            return copy_param(pbData, pdwDataLen, (CONST BYTE*)pCryptHash->abHashValue, 
                              pCryptHash->dwHashSize);

        default:
            SetLastError(NTE_BAD_TYPE);
            return FALSE;
    }
}

/******************************************************************************
 * CPSetKeyParam (RSAENH.@)
 *
 * Set a parameter of a key object
 *
 * PARAMS
 *  hProv   [I] The key container to which the key belongs.
 *  hKey    [I] The key for which a parameter is to be set.
 *  dwParam [I] Parameter type. See Notes.
 *  pbData  [I] Pointer to the parameter value.
 *  dwFlags [I] Currently none defined.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 *
 * NOTES:
 *  Defined dwParam types are:
 *   - KP_MODE: Values MODE_CBC, MODE_ECB, MODE_CFB.
 *   - KP_MODE_BITS: Shift width for cipher feedback mode. (Currently ignored by MS CSP's)
 *   - KP_PERMISSIONS: Or'ed combination of CRYPT_ENCRYPT, CRYPT_DECRYPT, 
 *                     CRYPT_EXPORT, CRYPT_READ, CRYPT_WRITE, CRYPT_MAC
 *   - KP_IV: Initialization vector
 */
BOOL WINAPI RSAENH_CPSetKeyParam(HCRYPTPROV hProv, HCRYPTKEY hKey, DWORD dwParam, BYTE *pbData, 
                                 DWORD dwFlags)
{
    CRYPTKEY *pCryptKey;

    TRACE("(hProv=%08lx, hKey=%08lx, dwParam=%08lx, pbData=%p, dwFlags=%08lx)\n", hProv, hKey, 
          dwParam, pbData, dwFlags);

    if (!is_valid_handle(&handle_table, (unsigned int)hProv, RSAENH_MAGIC_CONTAINER))
    {
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }

    if (dwFlags) {
        SetLastError(NTE_BAD_FLAGS);
        return FALSE;
    }
    
    if (!lookup_handle(&handle_table, (unsigned int)hKey, RSAENH_MAGIC_KEY, (OBJECTHDR**)&pCryptKey))
    {
        SetLastError(NTE_BAD_KEY);
        return FALSE;
    }
    
    switch (dwParam) {
        case KP_MODE:
            pCryptKey->dwMode = *(DWORD*)pbData;
            return TRUE;

        case KP_MODE_BITS:
            pCryptKey->dwModeBits = *(DWORD*)pbData;
            return TRUE;

        case KP_PERMISSIONS:
            pCryptKey->dwPermissions = *(DWORD*)pbData;
            return TRUE;

        case KP_IV:
            memcpy(pCryptKey->abInitVector, pbData, pCryptKey->dwBlockLen);
            return TRUE;

        default:
            SetLastError(NTE_BAD_TYPE);
            return FALSE;
    }
}

/******************************************************************************
 * CPGetKeyParam (RSAENH.@)
 *
 * Query a key parameter.
 *
 * PARAMS
 *  hProv      [I]   The key container, which the key belongs to.
 *  hHash      [I]   The key object that is to be queried.
 *  dwParam    [I]   Specifies the parameter that is to be queried.
 *  pbData     [I]   Pointer to the buffer where the parameter value will be stored.
 *  pdwDataLen [I/O] I: Buffer length at pbData, O: Length of the parameter value.
 *  dwFlags    [I]   None currently defined.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  Defined dwParam types are:
 *   - KP_MODE: Values MODE_CBC, MODE_ECB, MODE_CFB.
 *   - KP_MODE_BITS: Shift width for cipher feedback mode. 
 *                   (Currently ignored by MS CSP's - always eight)
 *   - KP_PERMISSIONS: Or'ed combination of CRYPT_ENCRYPT, CRYPT_DECRYPT, 
 *                     CRYPT_EXPORT, CRYPT_READ, CRYPT_WRITE, CRYPT_MAC
 *   - KP_IV: Initialization vector.
 *   - KP_KEYLEN: Bitwidth of the key.
 *   - KP_BLOCKLEN: Size of a block cipher block.
 *   - KP_SALT: Salt value.
 */
BOOL WINAPI RSAENH_CPGetKeyParam(HCRYPTPROV hProv, HCRYPTKEY hKey, DWORD dwParam, BYTE *pbData, 
                                 DWORD *pdwDataLen, DWORD dwFlags)
{
    CRYPTKEY *pCryptKey;
    DWORD dwBitLen;
        
    TRACE("(hProv=%08lx, hKey=%08lx, dwParam=%08lx, pbData=%p, pdwDataLen=%p dwFlags=%08lx)\n", 
          hProv, hKey, dwParam, pbData, pdwDataLen, dwFlags);

    if (!is_valid_handle(&handle_table, (unsigned int)hProv, RSAENH_MAGIC_CONTAINER)) 
    {
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }

    if (dwFlags) {
        SetLastError(NTE_BAD_FLAGS);
        return FALSE;
    }

    if (!lookup_handle(&handle_table, (unsigned int)hKey, RSAENH_MAGIC_KEY, (OBJECTHDR**)&pCryptKey))
    {
        SetLastError(NTE_BAD_KEY);
        return FALSE;
    }

    switch (dwParam) 
    {
        case KP_IV:
            return copy_param(pbData, pdwDataLen, (CONST BYTE*)pCryptKey->abInitVector, 
                              pCryptKey->dwBlockLen);
        
        case KP_SALT:
            return copy_param(pbData, pdwDataLen, 
                    (CONST BYTE*)&pCryptKey->abKeyValue[pCryptKey->dwKeyLen], pCryptKey->dwSaltLen);
        
        case KP_KEYLEN:
            dwBitLen = pCryptKey->dwKeyLen << 3;
            return copy_param(pbData, pdwDataLen, (CONST BYTE*)&dwBitLen, sizeof(DWORD));
        
        case KP_BLOCKLEN:
            dwBitLen = pCryptKey->dwBlockLen << 3;
            return copy_param(pbData, pdwDataLen, (CONST BYTE*)&dwBitLen, sizeof(DWORD));
    
        case KP_MODE:
            return copy_param(pbData, pdwDataLen, (CONST BYTE*)&pCryptKey->dwMode, sizeof(DWORD));

        case KP_MODE_BITS:
            return copy_param(pbData, pdwDataLen, (CONST BYTE*)&pCryptKey->dwModeBits, 
                              sizeof(DWORD));
    
        case KP_PERMISSIONS:
            return copy_param(pbData, pdwDataLen, (CONST BYTE*)&pCryptKey->dwPermissions, 
                              sizeof(DWORD));

        default:
            SetLastError(NTE_BAD_TYPE);
            return FALSE;
    }
}
                        
/******************************************************************************
 * CPGetProvParam (RSAENH.@)
 *
 * Query a CSP parameter.
 *
 * PARAMS
 *  hProv      [I]   The key container that is to be queried.
 *  dwParam    [I]   Specifies the parameter that is to be queried.
 *  pbData     [I]   Pointer to the buffer where the parameter value will be stored.
 *  pdwDataLen [I/O] I: Buffer length at pbData, O: Length of the parameter value.
 *  dwFlags    [I]   CRYPT_FIRST: Start enumeration (for PP_ENUMALGS{_EX}).
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 * NOTES:
 *  Defined dwParam types:
 *   - PP_CONTAINER: Name of the key container.
 *   - PP_NAME: Name of the cryptographic service provider.
 *   - PP_SIG_KEYSIZE_INC: RSA signature keywidth granularity in bits.
 *   - PP_KEYX_KEYSIZE_INC: RSA key-exchange keywidth granularity in bits.
 *   - PP_ENUMALGS{_EX}: Query provider capabilities.
 */
BOOL WINAPI RSAENH_CPGetProvParam(HCRYPTPROV hProv, DWORD dwParam, BYTE *pbData, 
                                  DWORD *pdwDataLen, DWORD dwFlags)
{
    KEYCONTAINER *pKeyContainer;
    PROV_ENUMALGS provEnumalgs;
    DWORD dwTemp;
    
    TRACE("(hProv=%08lx, dwParam=%08lx, pbData=%p, pdwDataLen=%p, dwFlags=%08lx)\n", 
           hProv, dwParam, pbData, pdwDataLen, dwFlags);

    if (!lookup_handle(&handle_table, (unsigned int)hProv, RSAENH_MAGIC_CONTAINER, 
                       (OBJECTHDR**)&pKeyContainer)) 
    {
        /* MSDN: hProv not containing valid context handle */
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }

    switch (dwParam) 
    {
        case PP_CONTAINER:
            return copy_param(pbData, pdwDataLen, (CONST BYTE*)pKeyContainer->szName, 
                              strlen(pKeyContainer->szName)+1);

        case PP_NAME:
            return copy_param(pbData, pdwDataLen, (CONST BYTE*)pKeyContainer->szProvName, 
                              strlen(pKeyContainer->szProvName)+1);

        case PP_SIG_KEYSIZE_INC:
        case PP_KEYX_KEYSIZE_INC:
            dwTemp = 8;
            return copy_param(pbData, pdwDataLen, (CONST BYTE*)&dwTemp, sizeof(dwTemp));
            
        case PP_ENUMALGS:
        case PP_ENUMALGS_EX:
            if (((pKeyContainer->dwEnumAlgsCtr >= RSAENH_MAX_ENUMALGS-1) ||
                 (!aProvEnumAlgsEx[pKeyContainer->dwPersonality]
                   [pKeyContainer->dwEnumAlgsCtr+1].aiAlgid)) && 
                ((dwFlags & CRYPT_FIRST) != CRYPT_FIRST))
            {
                SetLastError(ERROR_NO_MORE_ITEMS);
                return FALSE;
            }

            if (dwParam == PP_ENUMALGS) {    
                if (pbData && (*pdwDataLen >= sizeof(PROV_ENUMALGS))) 
                    pKeyContainer->dwEnumAlgsCtr = ((dwFlags & CRYPT_FIRST) == CRYPT_FIRST) ? 
                        0 : pKeyContainer->dwEnumAlgsCtr+1;
            
                provEnumalgs.aiAlgid = aProvEnumAlgsEx
                    [pKeyContainer->dwPersonality][pKeyContainer->dwEnumAlgsCtr].aiAlgid;
                provEnumalgs.dwBitLen = aProvEnumAlgsEx
                    [pKeyContainer->dwPersonality][pKeyContainer->dwEnumAlgsCtr].dwDefaultLen;
                provEnumalgs.dwNameLen = aProvEnumAlgsEx
                    [pKeyContainer->dwPersonality][pKeyContainer->dwEnumAlgsCtr].dwNameLen;
                memcpy(provEnumalgs.szName, aProvEnumAlgsEx
                       [pKeyContainer->dwPersonality][pKeyContainer->dwEnumAlgsCtr].szName, 
                       20*sizeof(CHAR));
            
                return copy_param(pbData, pdwDataLen, (CONST BYTE*)&provEnumalgs, 
                                  sizeof(PROV_ENUMALGS));
            } else {
                if (pbData && (*pdwDataLen >= sizeof(PROV_ENUMALGS_EX))) 
                    pKeyContainer->dwEnumAlgsCtr = ((dwFlags & CRYPT_FIRST) == CRYPT_FIRST) ? 
                        0 : pKeyContainer->dwEnumAlgsCtr+1;
            
                return copy_param(pbData, pdwDataLen, 
                                  (CONST BYTE*)&aProvEnumAlgsEx
                                      [pKeyContainer->dwPersonality][pKeyContainer->dwEnumAlgsCtr], 
                                  sizeof(PROV_ENUMALGS_EX));
            }

        default:
            /* MSDN: Unknown parameter number in dwParam */
            SetLastError(NTE_BAD_TYPE);
            return FALSE;
    }
    return FALSE;
}

/******************************************************************************
 * CPDeriveKey (RSAENH.@)
 *
 * Derives a key from a hash value.
 *
 * PARAMS
 *  hProv     [I] Key container for which a key is to be generated.
 *  Algid     [I] Crypto algorithm identifier for the key to be generated.
 *  hBaseData [I] Hash from whose value the key will be derived.
 *  dwFlags   [I] See Notes.
 *  phKey     [O] The generated key.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  Defined flags:
 *   - CRYPT_EXPORTABLE: Key can be exported.
 *   - CRYPT_NO_SALT: No salt is used for 40 bit keys.
 *   - CRYPT_CREATE_SALT: Use remaining bits as salt value.
 */
BOOL WINAPI RSAENH_CPDeriveKey(HCRYPTPROV hProv, ALG_ID Algid, HCRYPTHASH hBaseData, 
                               DWORD dwFlags, HCRYPTKEY *phKey)
{
    KEYCONTAINER *pKeyContainer;
    
    TRACE("(hProv=%08lx, Algid=%d, hBaseData=%08lx, dwFlags=%08lx phKey=%p)\n", hProv, Algid, 
           hBaseData, dwFlags, phKey);
    
    if (!lookup_handle(&handle_table, (unsigned int)hProv, RSAENH_MAGIC_CONTAINER, 
                       (OBJECTHDR**)&pKeyContainer))
    {
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }

    if (!is_valid_handle(&handle_table, (unsigned int)hBaseData, RSAENH_MAGIC_HASH))
    {
        SetLastError(NTE_BAD_HASH);
        return FALSE;
    }

    if (!phKey)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *phKey = new_key(hProv, Algid, dwFlags, NULL, hBaseData);
    return *phKey == (HCRYPTKEY)INVALID_HANDLE_VALUE ? FALSE : TRUE;
}

/******************************************************************************
 * CPGetUserKey (RSAENH.@)
 *
 * Returns a handle to the user's private key-exchange- or signature-key.
 *
 * PARAMS
 *  hProv     [I] The key container from which a user key is requested.
 *  dwKeySpec [I] AT_KEYEXCHANGE or AT_SIGNATURE
 *  phUserKey [O] Handle to the requested key or INVALID_HANDLE_VALUE in case of failure.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 *
 * NOTE
 *  A newly created key container does not contain private user key. Create them with CPGenKey.
 */
BOOL WINAPI RSAENH_CPGetUserKey(HCRYPTPROV hProv, DWORD dwKeySpec, HCRYPTKEY *phUserKey)
{
    KEYCONTAINER *pKeyContainer;

    TRACE("(hProv=%08lx, dwKeySpec=%08lx, phUserKey=%p)\n", hProv, dwKeySpec, phUserKey);
    
    if (!lookup_handle(&handle_table, (unsigned int)hProv, RSAENH_MAGIC_CONTAINER, 
                       (OBJECTHDR**)&pKeyContainer)) 
    {
        /* MSDN: hProv not containing valid context handle */
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }

    switch (dwKeySpec)
    {
        case AT_KEYEXCHANGE:
            copy_handle(&handle_table, pKeyContainer->hKeyExchangeKeyPair, RSAENH_MAGIC_KEY, 
                        (unsigned int*)phUserKey);
            break;

        case AT_SIGNATURE:
            copy_handle(&handle_table, pKeyContainer->hSignatureKeyPair, RSAENH_MAGIC_KEY, 
                        (unsigned int*)phUserKey);
            break;

        default:
            *phUserKey = (HCRYPTKEY)INVALID_HANDLE_VALUE;
    }

    if (*phUserKey == (HCRYPTKEY)INVALID_HANDLE_VALUE)
    {
        /* MSDN: dwKeySpec parameter specifies non existent key */
        SetLastError(NTE_NO_KEY);
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************
 * CPHashData (RSAENH.@)
 *
 * Updates a hash object with the given data.
 *
 * PARAMS
 *  hProv     [I] Key container to which the hash object belongs.
 *  hHash     [I] Hash object which is to be updated.
 *  pbData    [I] Pointer to data with which the hash object is to be updated.
 *  dwDataLen [I] Length of the data.
 *  dwFlags   [I] Currently none defined.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 *
 * NOTES
 *  The actual hash value is queried with CPGetHashParam, which will finalize 
 *  the hash. Updating a finalized hash will fail with a last error NTE_BAD_HASH_STATE.
 */
BOOL WINAPI RSAENH_CPHashData(HCRYPTPROV hProv, HCRYPTHASH hHash, CONST BYTE *pbData, 
                              DWORD dwDataLen, DWORD dwFlags)
{
    CRYPTHASH *pCryptHash;
    KEYCONTAINER *pKeyContainer;
        
    TRACE("(hProv=%08lx, hHash=%08lx, pbData=%p, dwDataLen=%ld, dwFlags=%08lx)\n", 
          hProv, hHash, pbData, dwDataLen, dwFlags);

    if (!lookup_handle(&handle_table, (unsigned int)hProv, RSAENH_MAGIC_CONTAINER, 
                       (OBJECTHDR**)&pKeyContainer))
    {
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }

    if (dwFlags)
    {
        SetLastError(NTE_BAD_FLAGS);
        return FALSE;
    }

    if (!lookup_handle(&handle_table, (unsigned int)hHash, RSAENH_MAGIC_HASH, 
                       (OBJECTHDR**)&pCryptHash))
    {
        SetLastError(NTE_BAD_HASH);
        return FALSE;
    }

    if (!get_algid_info(pKeyContainer, pCryptHash->aiAlgid) || 
        pCryptHash->aiAlgid == CALG_SSL3_SHAMD5) 
    {
        SetLastError(NTE_BAD_ALGID);
        return FALSE;
    }
    
    if (pCryptHash->dwState == RSAENH_HASHSTATE_IDLE)
        pCryptHash->dwState = RSAENH_HASHSTATE_HASHING;
    
    if (pCryptHash->dwState != RSAENH_HASHSTATE_HASHING)
    {
        SetLastError(NTE_BAD_HASH_STATE);
        return FALSE;
    }

    update_hash(pCryptHash, pbData, dwDataLen);
    return TRUE;
}

/******************************************************************************
 * CPHashSessionKey (RSAENH.@)
 */
BOOL WINAPI RSAENH_CPHashSessionKey(HCRYPTPROV hProv, HCRYPTHASH hHash, HCRYPTKEY hKey, 
                                    DWORD dwFlags)
{
    FIXME("(stub)\n");
    return FALSE;
}

/******************************************************************************
 * CPReleaseContext (RSAENH.@)
 *
 * Release a key container.
 *
 * PARAMS
 *  hProv   [I] Key container to be released.
 *  dwFlags [I] Currently none defined.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI RSAENH_CPReleaseContext(HCRYPTPROV hProv, DWORD dwFlags)
{
    TRACE("(hProv=%08lx, dwFlags=%08lx)\n", hProv, dwFlags);

    if (!release_handle(&handle_table, (unsigned int)hProv, RSAENH_MAGIC_CONTAINER)) 
    {
        /* MSDN: hProv not containing valid context handle */
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************
 * CPSetHashParam (RSAENH.@)
 * 
 * Set a parameter of a hash object
 *
 * PARAMS
 *  hProv   [I] The key container to which the key belongs.
 *  hHash   [I] The hash object for which a parameter is to be set.
 *  dwParam [I] Parameter type. See Notes.
 *  pbData  [I] Pointer to the parameter value.
 *  dwFlags [I] Currently none defined.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 *
 * NOTES:
 *  Currently only the HP_HMAC_INFO dwParam type is defined. 
 *  The HMAC_INFO struct will be deep copied into the hash object.
 *  See Internet RFC 2104 for details on the HMAC algorithm.
 */
BOOL WINAPI RSAENH_CPSetHashParam(HCRYPTPROV hProv, HCRYPTHASH hHash, DWORD dwParam, 
                                  BYTE *pbData, DWORD dwFlags)
{
    CRYPTHASH *pCryptHash;
    CRYPTKEY *pCryptKey;
    KEYCONTAINER *pKeyContainer;
    int i;

    TRACE("(hProv=%08lx, hHash=%08lx, dwParam=%08lx, pbData=%p, dwFlags=%08lx)\n", 
           hProv, hHash, dwParam, pbData, dwFlags);

    if (!lookup_handle(&handle_table, (unsigned int)hProv, RSAENH_MAGIC_CONTAINER, 
                       (OBJECTHDR**)&pKeyContainer))
    {
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }

    if (dwFlags) {
        SetLastError(NTE_BAD_FLAGS);
        return FALSE;
    }
    
    if (!lookup_handle(&handle_table, (unsigned int)hHash, RSAENH_MAGIC_HASH, 
                       (OBJECTHDR**)&pCryptHash))
    {
        SetLastError(NTE_BAD_HASH);
        return FALSE;
    }
    
    switch (dwParam) {
        case HP_HMAC_INFO:
            free_hmac_info(pCryptHash->pHMACInfo);
            if (!copy_hmac_info(&pCryptHash->pHMACInfo, (PHMAC_INFO)pbData)) return FALSE;
            init_hash(pKeyContainer, pCryptHash);

            if (!lookup_handle(&handle_table, pCryptHash->hKey, RSAENH_MAGIC_KEY, 
                               (OBJECTHDR**)&pCryptKey)) 
            {
                SetLastError(NTE_FAIL); /* FIXME: correct error code? */
                return FALSE;
            }

            for (i=0; i<RSAENH_MIN(pCryptKey->dwKeyLen,pCryptHash->pHMACInfo->cbInnerString); i++) {
                pCryptHash->pHMACInfo->pbInnerString[i] ^= pCryptKey->abKeyValue[i];
            }
            for (i=0; i<RSAENH_MIN(pCryptKey->dwKeyLen,pCryptHash->pHMACInfo->cbOuterString); i++) {
                pCryptHash->pHMACInfo->pbOuterString[i] ^= pCryptKey->abKeyValue[i];
            }
            
            return RSAENH_CPHashData(hProv, hHash, pCryptHash->pHMACInfo->pbInnerString, 
                                     pCryptHash->pHMACInfo->cbInnerString, 0);
            
        default:
            SetLastError(NTE_BAD_TYPE);
            return FALSE;
    }
}

/******************************************************************************
 * CPSetProvParam (RSAENH.@)
 */
BOOL WINAPI RSAENH_CPSetProvParam(HCRYPTPROV hProv, DWORD dwParam, BYTE *pbData, DWORD dwFlags)
{
    FIXME("(stub)\n");
    return FALSE;
}

/******************************************************************************
 * CPSignHash (RSAENH.@)
 */
BOOL WINAPI RSAENH_CPSignHash(HCRYPTPROV hProv, HCRYPTHASH hHash, DWORD dwKeySpec, 
                              LPCWSTR sDescription, DWORD dwFlags, BYTE *pbSignature, 
                              DWORD *pdwSigLen)
{
    FIXME("(stub)\n");
    return FALSE;
}

/******************************************************************************
 * CPVerifySignature (RSAENH.@)
 */
BOOL WINAPI RSAENH_CPVerifySignature(HCRYPTPROV hProv, HCRYPTHASH hHash, CONST BYTE *pbSignature, 
                                     DWORD dwSigLen, HCRYPTKEY hPubKey, LPCWSTR sDescription, 
                                     DWORD dwFlags)
{
    FIXME("(stub)\n");
    return FALSE;
}

static const WCHAR szProviderKeys[3][97] = {
    {   'S','o','f','t','w','a','r','e','\\',
        'M','i','c','r','o','s','o','f','t','\\','C','r','y','p','t','o','g','r',
        'a','p','h','y','\\','D','e','f','a','u','l','t','s','\\','P','r','o','v',
        'i','d','e','r','\\','M','i','c','r','o','s','o','f','t',' ','B','a','s',
        'e',' ','C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r',
        'o','v','i','d','e','r',' ','v','1','.','0',0 },
    {   'S','o','f','t','w','a','r','e','\\',
        'M','i','c','r','o','s','o','f','t','\\','C','r','y','p','t','o','g','r',
        'a','p','h','y','\\','D','e','f','a','u','l','t','s','\\','P','r','o','v',
        'i','d','e','r','\\','M','i','c','r','o','s','o','f','t',' ',
        'E','n','h','a','n','c','e','d',
        ' ','C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r',
        'o','v','i','d','e','r',' ','v','1','.','0',0 },
    {   'S','o','f','t','w','a','r','e','\\',
        'M','i','c','r','o','s','o','f','t','\\','C','r','y','p','t','o','g','r',
        'a','p','h','y','\\','D','e','f','a','u','l','t','s','\\','P','r','o','v',
        'i','d','e','r','\\','M','i','c','r','o','s','o','f','t',' ','S','t','r','o','n','g',
        ' ','C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r',
        'o','v','i','d','e','r',0 }
};
static const WCHAR szDefaultKey[] = { 'S','o','f','t','w','a','r','e','\\',
 'M','i','c','r','o','s','o','f','t','\\','C','r','y','p','t','o','g','r',
 'a','p','h','y','\\','D','e','f','a','u','l','t','s','\\','P','r','o','v',
 'i','d','e','r',' ','T','y','p','e','s','\\','T','y','p','e',' ','0','0','1',0};

/******************************************************************************
 * DllRegisterServer (RSAENH.@)
 *
 * Dll self registration. 
 *
 * PARAMS
 *
 * RETURNS
 *  Success: S_OK.
 *    Failure: != S_OK
 * 
 * NOTES
 *  Registers the following keys:
 *   - HKLM\Software\Microsoft\Cryptography\Defaults\Provider\
 *       Microsoft Base Cryptographic Provider v1.0
 *   - HKLM\Software\Microsoft\Cryptography\Defaults\Provider\
 *       Microsoft Enhanced Cryptographic Provider
 *   - HKLM\Software\Microsoft\Cryptography\Defaults\Provider\
 *       Microsoft Strong Cryptographpic Provider
 *   - HKLM\Software\Microsoft\Cryptography\Defaults\Provider Types\Type 001
 */
HRESULT WINAPI RSAENH_DllRegisterServer()
{
    HKEY key;
    DWORD dp;
    long apiRet;
    int i;

    for (i=0; i<3; i++) {
        apiRet = RegCreateKeyExW(HKEY_LOCAL_MACHINE, szProviderKeys[i], 0, NULL,
            REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, &dp);

        if (apiRet == ERROR_SUCCESS)
        {
            if (dp == REG_CREATED_NEW_KEY)
            {
                static const WCHAR szImagePath[] = { 'I','m','a','g','e',' ','P','a','t','h',0 };
                static const WCHAR szRSABase[] = { 'r','s','a','e','n','h','.','d','l','l',0 };
                static const WCHAR szType[] = { 'T','y','p','e',0 };
                static const WCHAR szSignature[] = { 'S','i','g','n','a','t','u','r','e',0 };
                DWORD type = 1;
                DWORD sign = 0xdeadbeef;
                RegSetValueExW(key, szImagePath, 0, REG_SZ, (LPBYTE)szRSABase, 
                               (lstrlenW(szRSABase) + 1) * sizeof(WCHAR));
                RegSetValueExW(key, szType, 0, REG_DWORD, (LPBYTE)&type, sizeof(type));
                RegSetValueExW(key, szSignature, 0, REG_BINARY, (LPBYTE)&sign, sizeof(sign));
            }
            RegCloseKey(key);
        }
    }
    if (apiRet == ERROR_SUCCESS)
        apiRet = RegCreateKeyExW(HKEY_LOCAL_MACHINE, szDefaultKey, 0, NULL, REG_OPTION_NON_VOLATILE,
                                 KEY_ALL_ACCESS, NULL, &key, &dp);
    if (apiRet == ERROR_SUCCESS)
    {
        if (dp == REG_CREATED_NEW_KEY)
        {
            static const WCHAR szName[] = { 'N','a','m','e',0 };
            static const WCHAR szRSAName[] = {
              'M','i','c','r','o','s','o','f','t',' ','S','t','r','o','n','g',' ',
              'C','r','y','p','t','o','g','r','a','p','h','i','c',' ',
              'P','r','o','v','i','d','e','r',0 };
            static const WCHAR szTypeName[] = { 'T','y','p','e','N','a','m','e',0 };
            static const WCHAR szRSATypeName[] = {
              'R','S','A',' ','F','u','l','l',' ',
              '(','S','i','g','n','a','t','u','r','e',' ','a','n','d',' ',
              'K','e','y',' ','E','x','c','h','a','n','g','e',')',0 };

            RegSetValueExW(key, szName, 0, REG_SZ, (LPBYTE)szRSAName, sizeof(szRSAName));
            RegSetValueExW(key, szTypeName, 0, REG_SZ, (LPBYTE)szRSATypeName,sizeof(szRSATypeName));
        }
        RegCloseKey(key);
    }
    return HRESULT_FROM_WIN32(apiRet);
}

/******************************************************************************
 * DllUnregisterServer (RSAENH.@)
 *
 * Dll self unregistration. 
 *
 * PARAMS
 *
 * RETURNS
 *  Success: S_OK
 *
 * NOTES
 *  For the relevant keys see DllRegisterServer.
 */
HRESULT WINAPI RSAENH_DllUnregisterServer()
{
    RegDeleteKeyW(HKEY_LOCAL_MACHINE, szProviderKeys[0]);
    RegDeleteKeyW(HKEY_LOCAL_MACHINE, szProviderKeys[1]);
    RegDeleteKeyW(HKEY_LOCAL_MACHINE, szProviderKeys[2]);
    RegDeleteKeyW(HKEY_LOCAL_MACHINE, szDefaultKey);
    return S_OK;
}
