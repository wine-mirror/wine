/*
 * dlls/rsaenh/rsaenh.c
 * RSAENH - RSA encryption for Wine
 *
 * Copyright 2002 TransGaming Technologies (David Hammerton)
 * Copyright 2004 Mike McCormack for CodeWeavers
 * Copyright 2004, 2005 Michael Jung
 * Copyright 2007 Vijay Kiran Kamuju
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

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "ntsecapi.h"
#include "wincrypt.h"
#include "handle.h"
#include "implglue.h"
#include "objbase.h"
#include "rpcproxy.h"
#include "aclapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

#define RSAENH_MAGIC_KEY           0x73620457u
#define RSAENH_MAX_KEY_SIZE        64
#define RSAENH_MAX_BLOCK_SIZE      24
#define RSAENH_KEYSTATE_IDLE       0
#define RSAENH_KEYSTATE_ENCRYPTING 1
#define RSAENH_KEYSTATE_MASTERKEY  2

/******************************************************************************
 * CRYPTHASH - hash objects
 */
#define RSAENH_MAGIC_HASH           0x85938417u
#define RSAENH_HASHSTATE_HASHING    1
#define RSAENH_HASHSTATE_FINISHED   2
typedef struct _RSAENH_TLS1PRF_PARAMS
{
    CRYPT_DATA_BLOB blobLabel;
    CRYPT_DATA_BLOB blobSeed;
} RSAENH_TLS1PRF_PARAMS;

typedef struct tagCRYPTHASH
{
    OBJECTHDR    header;
    ALG_ID       aiAlgid;
    HCRYPTKEY    hKey;
    HCRYPTPROV   hProv;
    DWORD        dwHashSize;
    DWORD        dwState;
    BCRYPT_HASH_HANDLE hash_handle;
    BYTE         abHashValue[RSAENH_MAX_HASH_SIZE];
    PHMAC_INFO   pHMACInfo;
    RSAENH_TLS1PRF_PARAMS tpPRFParams;
    DWORD        buffered_hash_bytes;
    ALG_ID       key_alg_id;
    KEY_CONTEXT  key_context;
    BYTE         abChainVector[RSAENH_MAX_BLOCK_SIZE];
} CRYPTHASH;

/******************************************************************************
 * CRYPTKEY - key objects
 */
typedef struct _RSAENH_SCHANNEL_INFO 
{
    SCHANNEL_ALG saEncAlg;
    SCHANNEL_ALG saMACAlg;
    CRYPT_DATA_BLOB blobClientRandom;
    CRYPT_DATA_BLOB blobServerRandom;
} RSAENH_SCHANNEL_INFO;

typedef struct tagCRYPTKEY
{
    OBJECTHDR   header;
    ALG_ID      aiAlgid;
    HCRYPTPROV  hProv;
    DWORD       dwMode;
    DWORD       dwModeBits;
    DWORD       dwPermissions;
    DWORD       dwKeyLen;
    DWORD       dwEffectiveKeyLen;
    DWORD       dwSaltLen;
    DWORD       dwBlockLen;
    DWORD       dwState;
    KEY_CONTEXT context;
    BYTE        abKeyValue[RSAENH_MAX_KEY_SIZE];
    BYTE        abInitVector[RSAENH_MAX_BLOCK_SIZE];
    BYTE        abChainVector[RSAENH_MAX_BLOCK_SIZE];
    RSAENH_SCHANNEL_INFO siSChannelInfo;
    CRYPT_DATA_BLOB blobHmacKey;
} CRYPTKEY;

/******************************************************************************
 * KEYCONTAINER - key containers
 */
#define RSAENH_PERSONALITY_BASE        0u
#define RSAENH_PERSONALITY_STRONG      1u
#define RSAENH_PERSONALITY_ENHANCED    2u
#define RSAENH_PERSONALITY_SCHANNEL    3u
#define RSAENH_PERSONALITY_AES         4u

#define RSAENH_MAGIC_CONTAINER         0x26384993u
typedef struct tagKEYCONTAINER
{
    OBJECTHDR    header;
    DWORD        dwFlags;
    DWORD        dwPersonality;
    DWORD        dwEnumAlgsCtr;
    DWORD        dwEnumContainersCtr;
    CHAR         szName[MAX_PATH];
    CHAR         szProvName[MAX_PATH];
    HCRYPTKEY    hKeyExchangeKeyPair;
    HCRYPTKEY    hSignatureKeyPair;
} KEYCONTAINER;

/******************************************************************************
 * Some magic constants
 */
#define RSAENH_HMAC_DEF_IPAD_CHAR      0x36
#define RSAENH_HMAC_DEF_OPAD_CHAR      0x5c
#define RSAENH_HMAC_DEF_PAD_LEN          64
#define RSAENH_HMAC_BLOCK_LEN            64
#define RSAENH_DES_EFFECTIVE_KEYLEN      56
#define RSAENH_DES_STORAGE_KEYLEN        64
#define RSAENH_3DES112_EFFECTIVE_KEYLEN 112
#define RSAENH_3DES112_STORAGE_KEYLEN   128
#define RSAENH_3DES_EFFECTIVE_KEYLEN    168
#define RSAENH_3DES_STORAGE_KEYLEN      192
#define RSAENH_MAGIC_RSA2        0x32415352
#define RSAENH_MAGIC_RSA1        0x31415352
#define RSAENH_PKC_BLOCKTYPE           0x02
#define RSAENH_SSL3_VERSION_MAJOR         3
#define RSAENH_SSL3_VERSION_MINOR         0
#define RSAENH_TLS1_VERSION_MAJOR         3
#define RSAENH_TLS1_VERSION_MINOR         1
#define RSAENH_REGKEY "Software\\Wine\\Crypto\\RSA\\%s"

#define RSAENH_MIN(a,b) ((a)<(b)?(a):(b))
/******************************************************************************
 * aProvEnumAlgsEx - Defines the capabilities of the CSP personalities.
 */
#define RSAENH_MAX_ENUMALGS 24
#define RSAENH_PCT1_SSL2_SSL3_TLS1 (CRYPT_FLAG_PCT1|CRYPT_FLAG_SSL2|CRYPT_FLAG_SSL3|CRYPT_FLAG_TLS1)
#define S(s) sizeof(s), s
static const PROV_ENUMALGS_EX aProvEnumAlgsEx[5][RSAENH_MAX_ENUMALGS+1] =
{
 {
  {CALG_RC2, 40, 40, 56, 0, S("RC2"), S("RSA Data Security's RC2")},
  {CALG_RC4, 40, 40, 56, 0, S("RC4"), S("RSA Data Security's RC4")},
  {CALG_DES, 56, 56, 56, 0, S("DES"), S("Data Encryption Standard (DES)")},
  {CALG_SHA, 160, 160, 160, CRYPT_FLAG_SIGNING, S("SHA-1"), S("Secure Hash Algorithm (SHA-1)")},
  {CALG_MD2, 128, 128, 128, CRYPT_FLAG_SIGNING, S("MD2"), S("Message Digest 2 (MD2)")},
  {CALG_MD4, 128, 128, 128, CRYPT_FLAG_SIGNING, S("MD4"), S("Message Digest 4 (MD4)")},
  {CALG_MD5, 128, 128, 128, CRYPT_FLAG_SIGNING, S("MD5"), S("Message Digest 5 (MD5)")},
  {CALG_SSL3_SHAMD5, 288, 288, 288, 0, S("SSL3 SHAMD5"), S("SSL3 SHAMD5")},
  {CALG_MAC, 0, 0, 0, 0, S("MAC"), S("Message Authentication Code")},
  {CALG_RSA_SIGN, 512, 384, 16384, CRYPT_FLAG_SIGNING|CRYPT_FLAG_IPSEC, S("RSA_SIGN"), S("RSA Signature")},
  {CALG_RSA_KEYX, 512, 384, 1024, CRYPT_FLAG_SIGNING|CRYPT_FLAG_IPSEC, S("RSA_KEYX"), S("RSA Key Exchange")},
  {CALG_HMAC, 0, 0, 0, 0, S("HMAC"), S("Hugo's MAC (HMAC)")},
  {0, 0, 0, 0, 0, S(""), S("")}
 },
 {
  {CALG_RC2, 128, 40, 128, 0, S("RC2"), S("RSA Data Security's RC2")},
  {CALG_RC4, 128, 40, 128, 0, S("RC4"), S("RSA Data Security's RC4")},
  {CALG_DES, 56, 56, 56, 0, S("DES"), S("Data Encryption Standard (DES)")},
  {CALG_3DES_112, 112, 112, 112, 0, S("3DES TWO KEY"), S("Two Key Triple DES")},
  {CALG_3DES, 168, 168, 168, 0, S("3DES"), S("Three Key Triple DES")},
  {CALG_SHA, 160, 160, 160, CRYPT_FLAG_SIGNING, S("SHA-1"), S("Secure Hash Algorithm (SHA-1)")},
  {CALG_MD2, 128, 128, 128, CRYPT_FLAG_SIGNING, S("MD2"), S("Message Digest 2 (MD2)")},
  {CALG_MD4, 128, 128, 128, CRYPT_FLAG_SIGNING, S("MD4"), S("Message Digest 4 (MD4)")},
  {CALG_MD5, 128, 128, 128, CRYPT_FLAG_SIGNING, S("MD5"), S("Message Digest 5 (MD5)")},
  {CALG_SSL3_SHAMD5, 288, 288, 288, 0, S("SSL3 SHAMD5"), S("SSL3 SHAMD5")},
  {CALG_MAC, 0, 0, 0, 0, S("MAC"), S("Message Authentication Code")},
  {CALG_RSA_SIGN, 1024, 384, 16384, CRYPT_FLAG_SIGNING|CRYPT_FLAG_IPSEC, S("RSA_SIGN"), S("RSA Signature")},
  {CALG_RSA_KEYX, 1024, 384, 16384, CRYPT_FLAG_SIGNING|CRYPT_FLAG_IPSEC, S("RSA_KEYX"), S("RSA Key Exchange")},
  {CALG_HMAC, 0, 0, 0, 0, S("HMAC"), S("Hugo's MAC (HMAC)")},
  {0, 0, 0, 0, 0, S(""), S("")}
 },
 {
  {CALG_RC2, 128, 40, 128, 0, S("RC2"), S("RSA Data Security's RC2")},
  {CALG_RC4, 128, 40, 128, 0, S("RC4"), S("RSA Data Security's RC4")},
  {CALG_DES, 56, 56, 56, 0, S("DES"), S("Data Encryption Standard (DES)")},
  {CALG_3DES_112, 112, 112, 112, 0, S("3DES TWO KEY"), S("Two Key Triple DES")},
  {CALG_3DES, 168, 168, 168, 0, S("3DES"), S("Three Key Triple DES")},
  {CALG_SHA, 160, 160, 160, CRYPT_FLAG_SIGNING, S("SHA-1"), S("Secure Hash Algorithm (SHA-1)")},
  {CALG_MD2, 128, 128, 128, CRYPT_FLAG_SIGNING, S("MD2"), S("Message Digest 2 (MD2)")},
  {CALG_MD4, 128, 128, 128, CRYPT_FLAG_SIGNING, S("MD4"), S("Message Digest 4 (MD4)")},
  {CALG_MD5, 128, 128, 128, CRYPT_FLAG_SIGNING, S("MD5"), S("Message Digest 5 (MD5)")},
  {CALG_SSL3_SHAMD5, 288, 288, 288, 0, S("SSL3 SHAMD5"), S("SSL3 SHAMD5")},
  {CALG_MAC, 0, 0, 0, 0, S("MAC"), S("Message Authentication Code")},
  {CALG_RSA_SIGN, 1024, 384, 16384, CRYPT_FLAG_SIGNING|CRYPT_FLAG_IPSEC, S("RSA_SIGN"), S("RSA Signature")},
  {CALG_RSA_KEYX, 1024, 384, 16384, CRYPT_FLAG_SIGNING|CRYPT_FLAG_IPSEC, S("RSA_KEYX"), S("RSA Key Exchange")},
  {CALG_HMAC, 0, 0, 0, 0, S("HMAC"), S("Hugo's MAC (HMAC)")},
  {0, 0, 0, 0, 0, S(""), S("")}
 },
 {
  {CALG_RC2, 128, 40, 128, RSAENH_PCT1_SSL2_SSL3_TLS1, S("RC2"), S("RSA Data Security's RC2")},
  {CALG_RC4, 128, 40, 128, RSAENH_PCT1_SSL2_SSL3_TLS1, S("RC4"), S("RSA Data Security's RC4")},
  {CALG_DES, 56, 56, 56, RSAENH_PCT1_SSL2_SSL3_TLS1, S("DES"), S("Data Encryption Standard (DES)")},
  {CALG_3DES_112, 112, 112, 112, RSAENH_PCT1_SSL2_SSL3_TLS1, S("3DES TWO KEY"), S("Two Key Triple DES")},
  {CALG_3DES, 168, 168, 168, RSAENH_PCT1_SSL2_SSL3_TLS1, S("3DES"), S("Three Key Triple DES")},
  {CALG_SHA, 160, 160, 160, CRYPT_FLAG_SIGNING|RSAENH_PCT1_SSL2_SSL3_TLS1, S("SHA-1"), S("Secure Hash Algorithm (SHA-1)")},
  {CALG_MD5, 128, 128, 128, CRYPT_FLAG_SIGNING|RSAENH_PCT1_SSL2_SSL3_TLS1, S("MD5"), S("Message Digest 5 (MD5)")},
  {CALG_SSL3_SHAMD5, 288, 288, 288, 0, S("SSL3 SHAMD5"), S("SSL3 SHAMD5")},
  {CALG_MAC, 0, 0, 0, 0, S("MAC"), S("Message Authentication Code")},
  {CALG_RSA_SIGN, 1024, 384, 16384, CRYPT_FLAG_SIGNING|RSAENH_PCT1_SSL2_SSL3_TLS1, S("RSA_SIGN"), S("RSA Signature")},
  {CALG_RSA_KEYX, 1024, 384, 16384, CRYPT_FLAG_SIGNING|RSAENH_PCT1_SSL2_SSL3_TLS1, S("RSA_KEYX"), S("RSA Key Exchange")},
  {CALG_HMAC, 0, 0, 0, 0, S("HMAC"), S("Hugo's MAC (HMAC)")},
  {CALG_PCT1_MASTER, 128, 128, 128, CRYPT_FLAG_PCT1, S("PCT1 MASTER"), S("PCT1 Master")},
  {CALG_SSL2_MASTER, 40, 40, 192, CRYPT_FLAG_SSL2, S("SSL2 MASTER"), S("SSL2 Master")},
  {CALG_SSL3_MASTER, 384, 384, 384, CRYPT_FLAG_SSL3, S("SSL3 MASTER"), S("SSL3 Master")},
  {CALG_TLS1_MASTER, 384, 384, 384, CRYPT_FLAG_TLS1, S("TLS1 MASTER"), S("TLS1 Master")},
  {CALG_SCHANNEL_MASTER_HASH, 0, 0, -1, 0, S("SCH MASTER HASH"), S("SChannel Master Hash")},
  {CALG_SCHANNEL_MAC_KEY, 0, 0, -1, 0, S("SCH MAC KEY"), S("SChannel MAC Key")},
  {CALG_SCHANNEL_ENC_KEY, 0, 0, -1, 0, S("SCH ENC KEY"), S("SChannel Encryption Key")},
  {CALG_TLS1PRF, 0, 0, -1, 0, S("TLS1 PRF"), S("TLS1 Pseudo Random Function")},
  {0, 0, 0, 0, 0, S(""), S("")}
 },
 {
  {CALG_RC2, 128, 40, 128, 0, S("RC2"), S("RSA Data Security's RC2")},
  {CALG_RC4, 128, 40, 128, 0, S("RC4"), S("RSA Data Security's RC4")},
  {CALG_DES, 56, 56, 56, 0, S("DES"), S("Data Encryption Standard (DES)")},
  {CALG_3DES_112, 112, 112, 112, 0, S("3DES TWO KEY"), S("Two Key Triple DES")},
  {CALG_3DES, 168, 168, 168, 0, S("3DES"), S("Three Key Triple DES")},
  {CALG_AES_128, 128, 128, 128, 0, S("AES-128"), S("Advanced Encryption Standard (AES-128)")},
  {CALG_AES_192, 192, 192, 192, 0, S("AES-192"), S("Advanced Encryption Standard (AES-192)")},
  {CALG_AES_256, 256, 256, 256, 0, S("AES-256"), S("Advanced Encryption Standard (AES-256)")},
  {CALG_SHA, 160, 160, 160, CRYPT_FLAG_SIGNING, S("SHA-1"), S("Secure Hash Algorithm (SHA-1)")},
  {CALG_SHA_256, 256, 256, 256, CRYPT_FLAG_SIGNING, S("SHA-256"), S("Secure Hash Algorithm (SHA-256)")},
  {CALG_SHA_384, 384, 384, 384, CRYPT_FLAG_SIGNING, S("SHA-384"), S("Secure Hash Algorithm (SHA-384)")},
  {CALG_SHA_512, 512, 512, 512, CRYPT_FLAG_SIGNING, S("SHA-512"), S("Secure Hash Algorithm (SHA-512)")},
  {CALG_MD2, 128, 128, 128, CRYPT_FLAG_SIGNING, S("MD2"), S("Message Digest 2 (MD2)")},
  {CALG_MD4, 128, 128, 128, CRYPT_FLAG_SIGNING, S("MD4"), S("Message Digest 4 (MD4)")},
  {CALG_MD5, 128, 128, 128, CRYPT_FLAG_SIGNING, S("MD5"), S("Message Digest 5 (MD5)")},
  {CALG_SSL3_SHAMD5, 288, 288, 288, 0, S("SSL3 SHAMD5"), S("SSL3 SHAMD5")},
  {CALG_MAC, 0, 0, 0, 0, S("MAC"), S("Message Authentication Code")},
  {CALG_RSA_SIGN, 1024, 384, 16384, CRYPT_FLAG_SIGNING|CRYPT_FLAG_IPSEC, S("RSA_SIGN"), S("RSA Signature")},
  {CALG_RSA_KEYX, 1024, 384, 16384, CRYPT_FLAG_SIGNING|CRYPT_FLAG_IPSEC, S("RSA_KEYX"), S("RSA Key Exchange")},
  {CALG_HMAC, 0, 0, 0, 0, S("HMAC"), S("Hugo's MAC (HMAC)")},
  {0, 0, 0, 0, 0, S(""), S("")}
 }
};
#undef S

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
RSAENH_CPCreateHash(
    HCRYPTPROV hProv, 
    ALG_ID Algid, 
    HCRYPTKEY hKey, 
    DWORD dwFlags, 
    HCRYPTHASH *phHash
);

BOOL WINAPI 
RSAENH_CPSetHashParam(
    HCRYPTPROV hProv, 
    HCRYPTHASH hHash, 
    DWORD dwParam, 
    BYTE *pbData, DWORD dwFlags
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
RSAENH_CPDestroyHash(
    HCRYPTPROV hProv, 
    HCRYPTHASH hHash
);

static BOOL crypt_export_key(
    CRYPTKEY *pCryptKey,
    HCRYPTKEY hPubKey, 
    DWORD dwBlobType, 
    DWORD dwFlags, 
    BOOL force,
    BYTE *pbData, 
    DWORD *pdwDataLen
);

static BOOL import_key(
    HCRYPTPROV hProv, 
    const BYTE *pbData,
    DWORD dwDataLen, 
    HCRYPTKEY hPubKey, 
    DWORD dwFlags, 
    BOOL fStoreKey,
    HCRYPTKEY *phKey
);

BOOL WINAPI 
RSAENH_CPHashData(
    HCRYPTPROV hProv, 
    HCRYPTHASH hHash, 
    const BYTE *pbData,
    DWORD dwDataLen, 
    DWORD dwFlags
);

/******************************************************************************
 * CSP's handle table (used by all acquired key containers)
 */
static struct handle_table handle_table;

/******************************************************************************
 * DllMain (RSAENH.@)
 *
 * Initializes and destroys the handle table for the CSP's handles.
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, PVOID reserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstance);
            init_handle_table(&handle_table);
            /* tomcrypt initialization */
            init_LTM();
            wprng = register_prng( &rc4_desc );
            rng_make_prng( 1024, wprng, &prng, NULL );
            break;

        case DLL_PROCESS_DETACH:
            if (reserved) break;
            destroy_handle_table(&handle_table);
            break;
    }
    return TRUE;
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
static inline BOOL copy_param(BYTE *pbBuffer, DWORD *pdwBufferSize, const BYTE *pbParam,
                              DWORD dwParamSize)
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

static inline KEYCONTAINER* get_key_container(HCRYPTPROV hProv)
{
    KEYCONTAINER *pKeyContainer;

    if (!lookup_handle(&handle_table, hProv, RSAENH_MAGIC_CONTAINER,
        (OBJECTHDR**)&pKeyContainer))
    {
        SetLastError(NTE_BAD_UID);
        return NULL;
    }
    return pKeyContainer;
}

/******************************************************************************
 * get_algid_info [Internal]
 *
 * Query CSP capabilities for a given crypto algorithm.
 * 
 * PARAMS
 *  hProv [I] Handle to a key container of the CSP whose capabilities are to be queried.
 *  algid [I] Identifier of the crypto algorithm about which information is requested.
 *
 * RETURNS
 *  Success: Pointer to a PROV_ENUMALGS_EX struct containing information about the crypto algorithm.
 *  Failure: NULL (algid not supported)
 */
static inline const PROV_ENUMALGS_EX* get_algid_info(HCRYPTPROV hProv, ALG_ID algid) {
    const PROV_ENUMALGS_EX *iterator;
    KEYCONTAINER *pKeyContainer;

    if (!(pKeyContainer = get_key_container(hProv))) return NULL;

    for (iterator = aProvEnumAlgsEx[pKeyContainer->dwPersonality]; iterator->aiAlgid; iterator++) {
        if (iterator->aiAlgid == algid) return iterator;
    }

    SetLastError(NTE_BAD_ALGID);
    return NULL;
}

/******************************************************************************
 * copy_data_blob [Internal] 
 *
 * deeply copies a DATA_BLOB
 *
 * PARAMS
 *  dst [O] That's where the blob will be copied to
 *  src [I] Source blob
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE (GetLastError() == NTE_NO_MEMORY
 *
 * NOTES
 *  Use free_data_blob to release resources occupied by copy_data_blob.
 */
static inline BOOL copy_data_blob(PCRYPT_DATA_BLOB dst, const PCRYPT_DATA_BLOB src)
{
    dst->pbData = malloc(src->cbData);
    if (!dst->pbData) {
        SetLastError(NTE_NO_MEMORY);
        return FALSE;
    }    
    dst->cbData = src->cbData;
    memcpy(dst->pbData, src->pbData, src->cbData);
    return TRUE;
}

/******************************************************************************
 * concat_data_blobs [Internal]
 *
 * Concatenates two blobs
 *
 * PARAMS
 *  dst  [O] The new blob will be copied here
 *  src1 [I] Prefix blob
 *  src2 [I] Appendix blob
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE (GetLastError() == NTE_NO_MEMORY)
 *
 * NOTES
 *  Release resources occupied by concat_data_blobs with free_data_blobs
 */
static inline BOOL concat_data_blobs(PCRYPT_DATA_BLOB dst, const PCRYPT_DATA_BLOB src1,
                                     const PCRYPT_DATA_BLOB src2)
{
    dst->cbData = src1->cbData + src2->cbData;
    dst->pbData = malloc(dst->cbData);
    if (!dst->pbData) {
        SetLastError(NTE_NO_MEMORY);
        return FALSE;
    }
    memcpy(dst->pbData, src1->pbData, src1->cbData);
    memcpy(dst->pbData + src1->cbData, src2->pbData, src2->cbData);
    return TRUE;
}

/******************************************************************************
 * free_data_blob [Internal]
 *
 * releases resource occupied by a dynamically allocated CRYPT_DATA_BLOB
 * 
 * PARAMS
 *  pBlob [I] Heap space occupied by pBlob->pbData is released
 */
static inline void free_data_blob(PCRYPT_DATA_BLOB pBlob) {
    free(pBlob->pbData);
}

/******************************************************************************
 * init_data_blob [Internal]
 */
static inline void init_data_blob(PCRYPT_DATA_BLOB pBlob) {
    pBlob->pbData = NULL;
    pBlob->cbData = 0;
}

/******************************************************************************
 * block_encrypt [Internal]
 */
BOOL block_encrypt(CRYPTKEY *key, BYTE *data, DWORD *data_len, DWORD buf_len,
                   BOOL final, KEY_CONTEXT *context, BYTE *chain_vector)
{
    BYTE *in, out[RSAENH_MAX_BLOCK_SIZE], o[RSAENH_MAX_BLOCK_SIZE];
    unsigned int encrypted_len, i, j, k;

    if (!final && (*data_len % key->dwBlockLen))
    {
        SetLastError(NTE_BAD_DATA);
        return FALSE;
    }

    encrypted_len = (*data_len / key->dwBlockLen + (final ? 1 : 0)) * key->dwBlockLen;

    if (data == NULL)
    {
        *data_len = encrypted_len;
        return TRUE;
    }
    if (encrypted_len > buf_len)
    {
        *data_len = encrypted_len;
        SetLastError(ERROR_MORE_DATA);
        return FALSE;
    }

    /* Pad final block with length bytes */
    for (i = *data_len; i < encrypted_len; i++) data[i] = encrypted_len - *data_len;
    *data_len = encrypted_len;

    for (i = 0, in = data; i < *data_len; i += key->dwBlockLen, in += key->dwBlockLen)
    {
        switch (key->dwMode) {
            case CRYPT_MODE_ECB:
                encrypt_block_impl(key->aiAlgid, 0, context, in, out);
                break;
            case CRYPT_MODE_CBC:
                for (j = 0; j < key->dwBlockLen; j++)
                    in[j] ^= chain_vector[j];
                encrypt_block_impl(key->aiAlgid, 0, context, in, out);
                memcpy(chain_vector, out, key->dwBlockLen);
                break;
            case CRYPT_MODE_CFB:
                for (j = 0; j < key->dwBlockLen; j++)
                {
                    encrypt_block_impl(key->aiAlgid, 0, context, chain_vector, o);
                    out[j] = in[j] ^ o[0];
                    for (k = 0; k < key->dwBlockLen - 1; k++)
                        chain_vector[k] = chain_vector[k+1];
                    chain_vector[k] = out[j];
                }
                break;
            default:
                SetLastError(NTE_BAD_ALGID);
                return FALSE;
        }
        memcpy(in, out, key->dwBlockLen);
    }
    return TRUE;
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
    free(hmac_info->pbInnerString);
    free(hmac_info->pbOuterString);
    free(hmac_info);
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
static BOOL copy_hmac_info(PHMAC_INFO *dst, const HMAC_INFO *src) {
    if (!src) return FALSE;
    *dst = malloc(sizeof(HMAC_INFO));
    if (!*dst) return FALSE;
    **dst = *src;
    (*dst)->pbInnerString = NULL;
    (*dst)->pbOuterString = NULL;
    if ((*dst)->cbInnerString == 0) (*dst)->cbInnerString = RSAENH_HMAC_DEF_PAD_LEN;
    (*dst)->pbInnerString = malloc((*dst)->cbInnerString);
    if (!(*dst)->pbInnerString) {
        free_hmac_info(*dst);
        return FALSE;
    }
    if (src->cbInnerString) 
        memcpy((*dst)->pbInnerString, src->pbInnerString, src->cbInnerString);
    else 
        memset((*dst)->pbInnerString, RSAENH_HMAC_DEF_IPAD_CHAR, RSAENH_HMAC_DEF_PAD_LEN);
    if ((*dst)->cbOuterString == 0) (*dst)->cbOuterString = RSAENH_HMAC_DEF_PAD_LEN;
    (*dst)->pbOuterString = malloc((*dst)->cbOuterString);
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
static void destroy_hash(OBJECTHDR *pObject)
{
    CRYPTHASH *pCryptHash = (CRYPTHASH*)pObject;

    BCryptDestroyHash(pCryptHash->hash_handle);
    free_hmac_info(pCryptHash->pHMACInfo);
    free_data_blob(&pCryptHash->tpPRFParams.blobLabel);
    free_data_blob(&pCryptHash->tpPRFParams.blobSeed);
    if (pCryptHash->aiAlgid == CALG_MAC)
        free_key_impl(pCryptHash->key_alg_id, &pCryptHash->key_context);
    free(pCryptHash);
}

/******************************************************************************
 * init_hash [Internal]
 *
 * Initialize (or reset) a hash object
 *
 * PARAMS
 *  pCryptHash    [I] The hash object to be initialized.
 */
static inline BOOL init_hash(CRYPTHASH *pCryptHash) {
    DWORD dwLen;
        
    switch (pCryptHash->aiAlgid) 
    {
        case CALG_HMAC:
            if (pCryptHash->pHMACInfo) { 
                const PROV_ENUMALGS_EX *pAlgInfo;
                
                pAlgInfo = get_algid_info(pCryptHash->hProv, pCryptHash->pHMACInfo->HashAlgid);
                if (!pAlgInfo)
                {
                    /* A number of hash algorithms (e. g., _SHA256) are supported for HMAC even for providers
                     * which don't list the algorithm, so print a fixme here. */
                    FIXME("Hash algroithm %#x not found.\n", pCryptHash->pHMACInfo->HashAlgid);
                    return FALSE;
                }
                pCryptHash->dwHashSize = pAlgInfo->dwDefaultLen >> 3;
                init_hash_impl(pCryptHash->pHMACInfo->HashAlgid, &pCryptHash->hash_handle);
                update_hash_impl(pCryptHash->hash_handle,
                                 pCryptHash->pHMACInfo->pbInnerString, 
                                 pCryptHash->pHMACInfo->cbInnerString);
            }
            return TRUE;
            
        case CALG_MAC:
            dwLen = sizeof(DWORD);
            RSAENH_CPGetKeyParam(pCryptHash->hProv, pCryptHash->hKey, KP_BLOCKLEN, 
                                 (BYTE*)&pCryptHash->dwHashSize, &dwLen, 0);
            pCryptHash->dwHashSize >>= 3;
            return TRUE;

        default:
            return init_hash_impl(pCryptHash->aiAlgid, &pCryptHash->hash_handle);
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
static inline void update_hash(CRYPTHASH *pCryptHash, const BYTE *pbData, DWORD dwDataLen)
{
    CRYPTKEY *key;
    BYTE *pbTemp;
    DWORD len;

    switch (pCryptHash->aiAlgid)
    {
        case CALG_HMAC:
            if (pCryptHash->pHMACInfo) 
                update_hash_impl(pCryptHash->hash_handle, pbData, dwDataLen);
            break;

        case CALG_MAC:
            if (!lookup_handle(&handle_table, pCryptHash->hKey, RSAENH_MAGIC_KEY, (OBJECTHDR**)&key))
            {
                FIXME("Key lookup failed.\n");
                return;
            }
            if (pCryptHash->buffered_hash_bytes)
            {
                len = min(pCryptHash->dwHashSize - pCryptHash->buffered_hash_bytes, dwDataLen);
                memcpy(pCryptHash->abHashValue + pCryptHash->buffered_hash_bytes, pbData, len);
                pbData += len;
                dwDataLen -= len;
                pCryptHash->buffered_hash_bytes += len;
                if (pCryptHash->buffered_hash_bytes < pCryptHash->dwHashSize)
                    return;
                pCryptHash->buffered_hash_bytes = 0;
                len = pCryptHash->dwHashSize;
                if (!block_encrypt(key, pCryptHash->abHashValue, &len, len, FALSE,
                                   &pCryptHash->key_context, pCryptHash->abChainVector))
                {
                    FIXME("block_encrypt failed.\n");
                    return;
                }
            }
            len = dwDataLen - dwDataLen % pCryptHash->dwHashSize;
            if (len)
            {
                pbTemp = malloc(len);
                if (!pbTemp)
                {
                    ERR("No memory.\n");
                    return;
                }
                memcpy(pbTemp, pbData, len);
                pbData += len;
                dwDataLen -= len;
                if (!block_encrypt(key, pbTemp, &len, len, FALSE,
                                   &pCryptHash->key_context, pCryptHash->abChainVector))
                {
                    FIXME("block_encrypt failed.\n");
                    return;
                }
                free(pbTemp);
            }
            if (dwDataLen)
            {
                memcpy(pCryptHash->abHashValue, pbData, dwDataLen);
                pCryptHash->buffered_hash_bytes = dwDataLen;
            }
            break;

        default:
            update_hash_impl(pCryptHash->hash_handle, pbData, dwDataLen);
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
static inline void finalize_hash(CRYPTHASH *pCryptHash)
{
    DWORD dwDataLen;
    CRYPTKEY *key;
        
    switch (pCryptHash->aiAlgid)
    {
        case CALG_HMAC:
            if (pCryptHash->pHMACInfo) {
                BYTE abHashValue[RSAENH_MAX_HASH_SIZE];

                finalize_hash_impl(pCryptHash->hash_handle, pCryptHash->abHashValue, pCryptHash->dwHashSize);
                memcpy(abHashValue, pCryptHash->abHashValue, pCryptHash->dwHashSize);
                init_hash_impl(pCryptHash->pHMACInfo->HashAlgid, &pCryptHash->hash_handle);
                update_hash_impl(pCryptHash->hash_handle,
                                 pCryptHash->pHMACInfo->pbOuterString, 
                                 pCryptHash->pHMACInfo->cbOuterString);
                update_hash_impl(pCryptHash->hash_handle,
                                 abHashValue, pCryptHash->dwHashSize);
                finalize_hash_impl(pCryptHash->hash_handle, pCryptHash->abHashValue, pCryptHash->dwHashSize);
                pCryptHash->hash_handle = NULL;
            } 
            break;

        case CALG_MAC:
            if (!lookup_handle(&handle_table, pCryptHash->hKey, RSAENH_MAGIC_KEY, (OBJECTHDR**)&key))
            {
                FIXME("Key lookup failed.\n");
                return;
            }
            dwDataLen = pCryptHash->buffered_hash_bytes;
            if (!block_encrypt(key, pCryptHash->abHashValue, &dwDataLen, pCryptHash->dwHashSize, TRUE,
                               &pCryptHash->key_context, pCryptHash->abChainVector))
                FIXME("block_encrypt failed.\n");
            break;

        default:
            finalize_hash_impl(pCryptHash->hash_handle, pCryptHash->abHashValue, pCryptHash->dwHashSize);
            pCryptHash->hash_handle = NULL;
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
static void destroy_key(OBJECTHDR *pObject)
{
    CRYPTKEY *pCryptKey = (CRYPTKEY*)pObject;
        
    free_key_impl(pCryptKey->aiAlgid, &pCryptKey->context);
    free_data_blob(&pCryptKey->siSChannelInfo.blobClientRandom);
    free_data_blob(&pCryptKey->siSChannelInfo.blobServerRandom);
    free_data_blob(&pCryptKey->blobHmacKey);
    free(pCryptKey);
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
                   pCryptKey->dwEffectiveKeyLen, pCryptKey->dwSaltLen,
                   pCryptKey->abKeyValue);
}

/******************************************************************************
 * alloc_key [Internal]
 *
 */
static HCRYPTKEY alloc_key(HCRYPTPROV hprov, ALG_ID alg_id, DWORD flags, DWORD key_len, CRYPTKEY **ret_key)
{
    KEYCONTAINER *key_container = get_key_container(hprov);
    HCRYPTKEY hkey;
    CRYPTKEY *key;

    hkey = new_object(&handle_table, sizeof(CRYPTKEY), RSAENH_MAGIC_KEY,
                      destroy_key, (OBJECTHDR**)&key);
    if (hkey == (HCRYPTKEY)INVALID_HANDLE_VALUE)
        return hkey;

    key->aiAlgid = alg_id;
    key->hProv = hprov;
    key->dwModeBits = 0;
    key->dwPermissions = CRYPT_ENCRYPT | CRYPT_DECRYPT | CRYPT_READ | CRYPT_WRITE | CRYPT_MAC;
    if (flags & CRYPT_EXPORTABLE)
        key->dwPermissions |= CRYPT_EXPORT;
    key->dwKeyLen = key_len >> 3;
    key->dwEffectiveKeyLen = 0;

    /*
     * For compatibility reasons a 40 bit key on the Enhanced
     * provider will not have salt
     */
    if (key_container->dwPersonality == RSAENH_PERSONALITY_ENHANCED
        && (alg_id == CALG_RC2 || alg_id == CALG_RC4)
        && (flags & CRYPT_CREATE_SALT) && key_len == 40)
        key->dwSaltLen = 0;
    else if ((flags & CRYPT_CREATE_SALT) || (key_len == 40 && !(flags & CRYPT_NO_SALT)))
        key->dwSaltLen = 16 /*FIXME*/ - key->dwKeyLen;
    else
        key->dwSaltLen = 0;
    memset(key->abKeyValue, 0, sizeof(key->abKeyValue));
    memset(key->abInitVector, 0, sizeof(key->abInitVector));
    memset(&key->siSChannelInfo.saEncAlg, 0, sizeof(key->siSChannelInfo.saEncAlg));
    memset(&key->siSChannelInfo.saMACAlg, 0, sizeof(key->siSChannelInfo.saMACAlg));
    init_data_blob(&key->siSChannelInfo.blobClientRandom);
    init_data_blob(&key->siSChannelInfo.blobServerRandom);
    init_data_blob(&key->blobHmacKey);

    switch(alg_id)
    {
        case CALG_PCT1_MASTER:
        case CALG_SSL2_MASTER:
        case CALG_SSL3_MASTER:
        case CALG_TLS1_MASTER:
        case CALG_RC4:
            key->dwBlockLen = 0;
            key->dwMode = 0;
            break;

        case CALG_RC2:
        case CALG_DES:
        case CALG_3DES_112:
        case CALG_3DES:
            key->dwBlockLen = 8;
            key->dwMode = CRYPT_MODE_CBC;
            break;

        case CALG_AES_128:
        case CALG_AES_192:
        case CALG_AES_256:
            key->dwBlockLen = 16;
            key->dwMode = CRYPT_MODE_CBC;
            break;

        case CALG_RSA_KEYX:
        case CALG_RSA_SIGN:
            key->dwBlockLen = key_len >> 3;
            key->dwMode = 0;
            break;

        case CALG_HMAC:
            key->dwBlockLen = 0;
            key->dwMode = 0;
            break;
    }

    *ret_key = key;
    return hkey;
}

/******************************************************************************
 * new_key [Internal]
 *
 * Creates a new key object without assigning the actual binary key value. 
 * This is done by CPDeriveKey, CPGenKey or CPImportKey, which call this function.
 *
 * PARAMS
 *  hProv      [I] Handle to the provider to which the created key will belong.
 *  aiAlgid    [I] The new key shall use the crypto algorithm identified by aiAlgid.
 *  dwFlags    [I] Upper 16 bits give the key length.
 *                 Lower 16 bits: CRYPT_EXPORTABLE, CRYPT_CREATE_SALT,
 *                 CRYPT_NO_SALT
 *  ppCryptKey [O] Pointer to the created key
 *
 * RETURNS
 *  Success: Handle to the created key.
 *  Failure: INVALID_HANDLE_VALUE
 */
static HCRYPTKEY new_key(HCRYPTPROV hProv, ALG_ID aiAlgid, DWORD dwFlags, CRYPTKEY **ppCryptKey)
{
    DWORD dwKeyLen = HIWORD(dwFlags);
    const PROV_ENUMALGS_EX *peaAlgidInfo;

    *ppCryptKey = NULL;
    
    /* 
     * Retrieve the CSP's capabilities for the given ALG_ID value
     */
    peaAlgidInfo = get_algid_info(hProv, aiAlgid);
    if (!peaAlgidInfo) return (HCRYPTKEY)INVALID_HANDLE_VALUE;

    TRACE("alg = %s, dwKeyLen = %ld\n", debugstr_a(peaAlgidInfo->szName),
          dwKeyLen);
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

        case CALG_HMAC:
            /* Avoid the key length check for HMAC keys, which have unlimited
             * length.
             */
            break;

        default:
            if (dwKeyLen % 8 || 
                dwKeyLen > peaAlgidInfo->dwMaxLen || 
                dwKeyLen < peaAlgidInfo->dwMinLen) 
            {
                TRACE("key len %ld out of bounds (%ld, %ld)\n", dwKeyLen,
                      peaAlgidInfo->dwMinLen, peaAlgidInfo->dwMaxLen);
                SetLastError(NTE_BAD_DATA);
                return (HCRYPTKEY)INVALID_HANDLE_VALUE;
            }
    }

    return alloc_key(hProv, aiAlgid, dwFlags, dwKeyLen, ppCryptKey);
}

/******************************************************************************
 * map_key_spec_to_key_pair_name [Internal]
 *
 * Returns the name of the registry value associated with a key spec.
 *
 * PARAMS
 *  dwKeySpec     [I] AT_KEYEXCHANGE or AT_SIGNATURE
 *
 * RETURNS
 *  Success: Name of registry value.
 *  Failure: NULL
 */
static LPCSTR map_key_spec_to_key_pair_name(DWORD dwKeySpec)
{
    LPCSTR szValueName;

    switch (dwKeySpec)
    {
    case AT_KEYEXCHANGE:
        szValueName = "KeyExchangeKeyPair";
        break;
    case AT_SIGNATURE:
        szValueName = "SignatureKeyPair";
        break;
    default:
        WARN("invalid key spec %ld\n", dwKeySpec);
        szValueName = NULL;
    }
    return szValueName;
}

/******************************************************************************
 * store_key_pair [Internal]
 *
 * Stores a key pair to the registry
 * 
 * PARAMS
 *  hCryptKey     [I] Handle to the key to be stored
 *  hKey          [I] Registry key where the key pair is to be stored
 *  dwKeySpec     [I] AT_KEYEXCHANGE or AT_SIGNATURE
 *  dwFlags       [I] Flags for protecting the key
 */
static void store_key_pair(HCRYPTKEY hCryptKey, HKEY hKey, DWORD dwKeySpec, DWORD dwFlags)
{
    LPCSTR szValueName;
    DATA_BLOB blobIn, blobOut;
    CRYPTKEY *pKey;
    DWORD dwLen;
    BYTE *pbKey;

    if (!(szValueName = map_key_spec_to_key_pair_name(dwKeySpec)))
        return;
    if (lookup_handle(&handle_table, hCryptKey, RSAENH_MAGIC_KEY,
                      (OBJECTHDR**)&pKey))
    {
        if (crypt_export_key(pKey, 0, PRIVATEKEYBLOB, 0, TRUE, 0, &dwLen))
        {
            pbKey = malloc(dwLen);
            if (pbKey)
            {
                if (crypt_export_key(pKey, 0, PRIVATEKEYBLOB, 0, TRUE, pbKey,
                    &dwLen))
                {
                    blobIn.pbData = pbKey;
                    blobIn.cbData = dwLen;

                    if (CryptProtectData(&blobIn, NULL, NULL, NULL, NULL,
                        dwFlags, &blobOut))
                    {
                        RegSetValueExA(hKey, szValueName, 0, REG_BINARY,
                                       blobOut.pbData, blobOut.cbData);
                        LocalFree(blobOut.pbData);
                    }
                }
                free(pbKey);
            }
        }
    }
}

/******************************************************************************
 * map_key_spec_to_permissions_name [Internal]
 *
 * Returns the name of the registry value associated with the permissions for
 * a key spec.
 *
 * PARAMS
 *  dwKeySpec     [I] AT_KEYEXCHANGE or AT_SIGNATURE
 *
 * RETURNS
 *  Success: Name of registry value.
 *  Failure: NULL
 */
static LPCSTR map_key_spec_to_permissions_name(DWORD dwKeySpec)
{
    LPCSTR szValueName;

    switch (dwKeySpec)
    {
    case AT_KEYEXCHANGE:
        szValueName = "KeyExchangePermissions";
        break;
    case AT_SIGNATURE:
        szValueName = "SignaturePermissions";
        break;
    default:
        WARN("invalid key spec %ld\n", dwKeySpec);
        szValueName = NULL;
    }
    return szValueName;
}

/******************************************************************************
 * store_key_permissions [Internal]
 *
 * Stores a key's permissions to the registry
 *
 * PARAMS
 *  hCryptKey     [I] Handle to the key whose permissions are to be stored
 *  hKey          [I] Registry key where the key permissions are to be stored
 *  dwKeySpec     [I] AT_KEYEXCHANGE or AT_SIGNATURE
 */
static void store_key_permissions(HCRYPTKEY hCryptKey, HKEY hKey, DWORD dwKeySpec)
{
    LPCSTR szValueName;
    CRYPTKEY *pKey;

    if (!(szValueName = map_key_spec_to_permissions_name(dwKeySpec)))
        return;
    if (lookup_handle(&handle_table, hCryptKey, RSAENH_MAGIC_KEY,
                      (OBJECTHDR**)&pKey))
        RegSetValueExA(hKey, szValueName, 0, REG_DWORD,
                       (BYTE *)&pKey->dwPermissions,
                       sizeof(pKey->dwPermissions));
}

/******************************************************************************
 * create_container_key [Internal]
 *
 * Creates the registry key for a key container's persistent storage.
 * 
 * PARAMS
 *  pKeyContainer [I] Pointer to the key container
 *  sam           [I] Desired registry access
 *  phKey         [O] Returned key
 */
static BOOL create_container_key(KEYCONTAINER *pKeyContainer, REGSAM sam, HKEY *phKey)
{
    CHAR szRSABase[sizeof(RSAENH_REGKEY) + MAX_PATH];
    HKEY hRootKey;

    sprintf(szRSABase, RSAENH_REGKEY, pKeyContainer->szName);

    if (pKeyContainer->dwFlags & CRYPT_MACHINE_KEYSET)
        hRootKey = HKEY_LOCAL_MACHINE;
    else
        hRootKey = HKEY_CURRENT_USER;

    /* @@ Wine registry key: HKLM\Software\Wine\Crypto\RSA */
    /* @@ Wine registry key: HKCU\Software\Wine\Crypto\RSA */
    return RegCreateKeyExA(hRootKey, szRSABase, 0, NULL,
                           REG_OPTION_NON_VOLATILE, sam, NULL, phKey, NULL)
                           == ERROR_SUCCESS;
}

/******************************************************************************
 * open_container_key [Internal]
 *
 * Opens a key container's persistent storage for reading.
 *
 * PARAMS
 *  pszContainerName [I] Name of the container to be opened.  May be the empty
 *                       string if the parent key of all containers is to be
 *                       opened.
 *  dwFlags          [I] Flags indicating which keyset to be opened.
 *  phKey            [O] Returned key
 */
static BOOL open_container_key(LPCSTR pszContainerName, DWORD dwFlags, REGSAM access, HKEY *phKey)
{
    CHAR szRSABase[sizeof(RSAENH_REGKEY) + MAX_PATH];
    HKEY hRootKey;

    sprintf(szRSABase, RSAENH_REGKEY, pszContainerName);

    if (dwFlags & CRYPT_MACHINE_KEYSET)
        hRootKey = HKEY_LOCAL_MACHINE;
    else
        hRootKey = HKEY_CURRENT_USER;

    /* @@ Wine registry key: HKLM\Software\Wine\Crypto\RSA */
    /* @@ Wine registry key: HKCU\Software\Wine\Crypto\RSA */
    return RegOpenKeyExA(hRootKey, szRSABase, 0, access, phKey) ==
                         ERROR_SUCCESS;
}

/******************************************************************************
 * delete_container_key [Internal]
 *
 * Deletes a key container's persistent storage.
 *
 * PARAMS
 *  pszContainerName [I] Name of the container to be opened.
 *  dwFlags          [I] Flags indicating which keyset to be opened.
 */
static BOOL delete_container_key(LPCSTR pszContainerName, DWORD dwFlags)
{
    CHAR szRegKey[sizeof(RSAENH_REGKEY) + MAX_PATH];
    HKEY hRootKey;

    sprintf(szRegKey, RSAENH_REGKEY, pszContainerName);

    if (dwFlags & CRYPT_MACHINE_KEYSET)
        hRootKey = HKEY_LOCAL_MACHINE;
    else
        hRootKey = HKEY_CURRENT_USER;
    if (!RegDeleteKeyA(hRootKey, szRegKey)) {
        SetLastError(ERROR_SUCCESS);
        return TRUE;
    } else {
        SetLastError(NTE_BAD_KEYSET);
        return FALSE;
    }
}

/******************************************************************************
 * store_key_container_keys [Internal]
 *
 * Stores key container's keys in a persistent location.
 *
 * PARAMS
 *  pKeyContainer [I] Pointer to the key container whose keys are to be saved
 */
static void store_key_container_keys(KEYCONTAINER *pKeyContainer)
{
    HKEY hKey;
    DWORD dwFlags;

    /* On WinXP, persistent keys are stored in a file located at:
     * $AppData$\\Microsoft\\Crypto\\RSA\\$SID$\\some_hex_string
     */

    if (pKeyContainer->dwFlags & CRYPT_MACHINE_KEYSET)
        dwFlags = CRYPTPROTECT_LOCAL_MACHINE;
    else
        dwFlags = 0;

    if (create_container_key(pKeyContainer, KEY_WRITE, &hKey))
    {
        store_key_pair(pKeyContainer->hKeyExchangeKeyPair, hKey,
                       AT_KEYEXCHANGE, dwFlags);
        store_key_pair(pKeyContainer->hSignatureKeyPair, hKey,
                       AT_SIGNATURE, dwFlags);
        RegCloseKey(hKey);
    }
}

/******************************************************************************
 * store_key_container_permissions [Internal]
 *
 * Stores key container's key permissions in a persistent location.
 *
 * PARAMS
 *  pKeyContainer [I] Pointer to the key container whose key permissions are to
 *                    be saved
 */
static void store_key_container_permissions(KEYCONTAINER *pKeyContainer)
{
    HKEY hKey;

    if (create_container_key(pKeyContainer, KEY_WRITE, &hKey))
    {
        store_key_permissions(pKeyContainer->hKeyExchangeKeyPair, hKey,
                       AT_KEYEXCHANGE);
        store_key_permissions(pKeyContainer->hSignatureKeyPair, hKey,
                       AT_SIGNATURE);
        RegCloseKey(hKey);
    }
}

/******************************************************************************
 * release_key_container_keys [Internal]
 *
 * Releases key container's keys.
 *
 * PARAMS
 *  pKeyContainer [I] Pointer to the key container whose keys are to be released.
 */
static void release_key_container_keys(KEYCONTAINER *pKeyContainer)
{
    release_handle(&handle_table, pKeyContainer->hKeyExchangeKeyPair,
                   RSAENH_MAGIC_KEY);
    release_handle(&handle_table, pKeyContainer->hSignatureKeyPair,
                   RSAENH_MAGIC_KEY);
}

/******************************************************************************
 * destroy_key_container [Internal]
 *
 * Destructor for key containers.
 *
 * PARAMS
 *  pObjectHdr [I] Pointer to the key container to be destroyed.
 */
static void destroy_key_container(OBJECTHDR *pObjectHdr)
{
    KEYCONTAINER *pKeyContainer = (KEYCONTAINER*)pObjectHdr;

    if (!(pKeyContainer->dwFlags & CRYPT_VERIFYCONTEXT))
    {
        store_key_container_keys(pKeyContainer);
        store_key_container_permissions(pKeyContainer);
        release_key_container_keys(pKeyContainer);
    }
    else
        release_key_container_keys(pKeyContainer);
    free( pKeyContainer );
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
static HCRYPTPROV new_key_container(PCCH pszContainerName, DWORD dwFlags, const VTableProvStruc *pVTable)
{
    KEYCONTAINER *pKeyContainer;
    HCRYPTPROV hKeyContainer;

    hKeyContainer = new_object(&handle_table, sizeof(KEYCONTAINER), RSAENH_MAGIC_CONTAINER,
                               destroy_key_container, (OBJECTHDR**)&pKeyContainer);
    if (hKeyContainer != (HCRYPTPROV)INVALID_HANDLE_VALUE)
    {
        lstrcpynA(pKeyContainer->szName, pszContainerName, MAX_PATH);
        pKeyContainer->dwFlags = dwFlags;
        pKeyContainer->dwEnumAlgsCtr = 0;
        pKeyContainer->hKeyExchangeKeyPair = (HCRYPTKEY)INVALID_HANDLE_VALUE;
        pKeyContainer->hSignatureKeyPair = (HCRYPTKEY)INVALID_HANDLE_VALUE;
        if (pVTable && pVTable->pszProvName) {
            lstrcpynA(pKeyContainer->szProvName, pVTable->pszProvName, MAX_PATH);
            if (!strcmp(pVTable->pszProvName, MS_DEF_PROV_A)) {
                pKeyContainer->dwPersonality = RSAENH_PERSONALITY_BASE;
            } else if (!strcmp(pVTable->pszProvName, MS_ENHANCED_PROV_A)) {
                pKeyContainer->dwPersonality = RSAENH_PERSONALITY_ENHANCED;
            } else if (!strcmp(pVTable->pszProvName, MS_DEF_RSA_SCHANNEL_PROV_A)) { 
                pKeyContainer->dwPersonality = RSAENH_PERSONALITY_SCHANNEL;
            } else if (!strcmp(pVTable->pszProvName, MS_ENH_RSA_AES_PROV_A) ||
                       !strcmp(pVTable->pszProvName, MS_ENH_RSA_AES_PROV_XP_A)) {
                pKeyContainer->dwPersonality = RSAENH_PERSONALITY_AES;
            } else {
                pKeyContainer->dwPersonality = RSAENH_PERSONALITY_STRONG;
            }
        }

        /* The new key container has to be inserted into the CSP immediately 
         * after creation to be available for CPGetProvParam's PP_ENUMCONTAINERS. */
        if (!(dwFlags & CRYPT_VERIFYCONTEXT)) {
            HKEY hKey;

            if (create_container_key(pKeyContainer, KEY_WRITE, &hKey))
                RegCloseKey(hKey);
        }
    }

    return hKeyContainer;
}

/******************************************************************************
 * read_key_value [Internal]
 *
 * Reads a key pair value from the registry
 *
 * PARAMS
 *  hKeyContainer [I] Crypt provider to use to import the key
 *  hKey          [I] Registry key from which to read the key pair
 *  dwKeySpec     [I] AT_KEYEXCHANGE or AT_SIGNATURE
 *  dwFlags       [I] Flags for unprotecting the key
 *  phCryptKey    [O] Returned key
 */
static BOOL read_key_value(HCRYPTPROV hKeyContainer, HKEY hKey, DWORD dwKeySpec, DWORD dwFlags, HCRYPTKEY *phCryptKey)
{
    LPCSTR szValueName;
    DWORD dwValueType, dwLen;
    BYTE *pbKey;
    DATA_BLOB blobIn, blobOut;
    BOOL ret = FALSE;

    if (!(szValueName = map_key_spec_to_key_pair_name(dwKeySpec)))
        return FALSE;
    if (RegQueryValueExA(hKey, szValueName, 0, &dwValueType, NULL, &dwLen) ==
        ERROR_SUCCESS)
    {
        pbKey = malloc(dwLen);
        if (pbKey)
        {
            if (RegQueryValueExA(hKey, szValueName, 0, &dwValueType, pbKey, &dwLen) ==
                ERROR_SUCCESS)
            {
                blobIn.pbData = pbKey;
                blobIn.cbData = dwLen;

                if (CryptUnprotectData(&blobIn, NULL, NULL, NULL, NULL,
                    dwFlags, &blobOut))
                {
                    ret = import_key(hKeyContainer, blobOut.pbData, blobOut.cbData, 0, 0,
                                     FALSE, phCryptKey);
                    LocalFree(blobOut.pbData);
                }
            }
            free(pbKey);
        }
    }
    if (ret)
    {
        CRYPTKEY *pKey;

        if (lookup_handle(&handle_table, *phCryptKey, RSAENH_MAGIC_KEY,
                          (OBJECTHDR**)&pKey))
        {
            if ((szValueName = map_key_spec_to_permissions_name(dwKeySpec)))
            {
                dwLen = sizeof(pKey->dwPermissions);
                RegQueryValueExA(hKey, szValueName, 0, NULL,
                                 (BYTE *)&pKey->dwPermissions, &dwLen);
            }
        }
    }
    return ret;
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
static HCRYPTPROV read_key_container(PCHAR pszContainerName, DWORD dwFlags, const VTableProvStruc *pVTable)
{
    HKEY hKey;
    KEYCONTAINER *pKeyContainer;
    HCRYPTPROV hKeyContainer;
    HCRYPTKEY hCryptKey;

    if (!open_container_key(pszContainerName, dwFlags, KEY_READ, &hKey))
    {
        SetLastError(NTE_BAD_KEYSET);
        return (HCRYPTPROV)INVALID_HANDLE_VALUE;
    }

    hKeyContainer = new_key_container(pszContainerName, dwFlags, pVTable);
    if (hKeyContainer != (HCRYPTPROV)INVALID_HANDLE_VALUE)
    {
        DWORD dwProtectFlags = (dwFlags & CRYPT_MACHINE_KEYSET) ?
            CRYPTPROTECT_LOCAL_MACHINE : 0;

        if (!lookup_handle(&handle_table, hKeyContainer, RSAENH_MAGIC_CONTAINER, 
                           (OBJECTHDR**)&pKeyContainer))
            return (HCRYPTPROV)INVALID_HANDLE_VALUE;
    
        /* read_key_value calls import_key, which calls import_private_key,
         * which implicitly installs the key value into the appropriate key
         * container key.  Thus the ref count is incremented twice, once for
         * the output key value, and once for the implicit install, and needs
         * to be decremented to balance the two.
         */
        if (read_key_value(hKeyContainer, hKey, AT_KEYEXCHANGE,
            dwProtectFlags, &hCryptKey))
            release_handle(&handle_table, hCryptKey, RSAENH_MAGIC_KEY);
        if (read_key_value(hKeyContainer, hKey, AT_SIGNATURE,
            dwProtectFlags, &hCryptKey))
            release_handle(&handle_table, hCryptKey, RSAENH_MAGIC_KEY);
    }

    return hKeyContainer;
}

/******************************************************************************
 * build_hash_signature [Internal]
 *
 * Builds a padded version of a hash to match the length of the RSA key modulus.
 *
 * PARAMS
 *  pbSignature [O] The padded hash object is stored here.
 *  dwLen       [I] Length of the pbSignature buffer.
 *  aiAlgid     [I] Algorithm identifier of the hash to be padded.
 *  abHashValue [I] The value of the hash object.
 *  dwHashLen   [I] Length of the hash value.
 *  dwFlags     [I] Selection of padding algorithm.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE (NTE_BAD_ALGID)
 */
static BOOL build_hash_signature(BYTE *pbSignature, DWORD dwLen, ALG_ID aiAlgid, 
                                 const BYTE *abHashValue, DWORD dwHashLen, DWORD dwFlags)
{
    /* These prefixes are meant to be concatenated with hash values of the
     * respective kind to form a PKCS #7 DigestInfo. */
    static const struct tagOIDDescriptor {
        ALG_ID aiAlgid;
        DWORD dwLen;
        const BYTE abOID[19];
    } aOIDDescriptor[] = {
        { CALG_MD2, 18, { 0x30, 0x20, 0x30, 0x0c, 0x06, 0x08, 0x2a, 0x86, 0x48,
                          0x86, 0xf7, 0x0d, 0x02, 0x02, 0x05, 0x00, 0x04, 0x10 } },
        { CALG_MD4, 18, { 0x30, 0x20, 0x30, 0x0c, 0x06, 0x08, 0x2a, 0x86, 0x48, 
                          0x86, 0xf7, 0x0d, 0x02, 0x04, 0x05, 0x00, 0x04, 0x10 } },
        { CALG_MD5, 18, { 0x30, 0x20, 0x30, 0x0c, 0x06, 0x08, 0x2a, 0x86, 0x48,
                          0x86, 0xf7, 0x0d, 0x02, 0x05, 0x05, 0x00, 0x04, 0x10 } },
        { CALG_SHA, 15, { 0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e, 0x03, 
                          0x02, 0x1a, 0x05, 0x00, 0x04, 0x14 } },
        { CALG_SHA_256, 19, { 0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86,
                              0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01,
                              0x05, 0x00, 0x04, 0x20 } },
        { CALG_SHA_384, 19, { 0x30, 0x41, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86,
                              0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x02,
                              0x05, 0x00, 0x04, 0x30 } },
        { CALG_SHA_512, 19, { 0x30, 0x51, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86,
                              0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03,
                              0x05, 0x00, 0x04, 0x40 } },
        { CALG_SSL3_SHAMD5, 0, { 0 } },
        { 0,        0,  { 0 } }
    };
    DWORD dwIdxOID, i, j;

    for (dwIdxOID = 0; aOIDDescriptor[dwIdxOID].aiAlgid; dwIdxOID++) {
        if (aOIDDescriptor[dwIdxOID].aiAlgid == aiAlgid) break;
    }
    
    if (!aOIDDescriptor[dwIdxOID].aiAlgid) {
        SetLastError(NTE_BAD_ALGID);
        return FALSE;
    }

    /* Build the padded signature */
    if (dwFlags & CRYPT_X931_FORMAT) {
        pbSignature[0] = 0x6b;
        for (i=1; i < dwLen - dwHashLen - 3; i++) {
            pbSignature[i] = 0xbb;
        }
        pbSignature[i++] = 0xba;
        for (j=0; j < dwHashLen; j++, i++) {
            pbSignature[i] = abHashValue[j];
        }
        pbSignature[i++] = 0x33;
        pbSignature[i++] = 0xcc;
    } else {
        pbSignature[0] = 0x00;
        pbSignature[1] = 0x01;
        if (dwFlags & CRYPT_NOHASHOID) {
            for (i=2; i < dwLen - 1 - dwHashLen; i++) {
                pbSignature[i] = 0xff;
            }
            pbSignature[i++] = 0x00;
        } else {
            for (i=2; i < dwLen - 1 - aOIDDescriptor[dwIdxOID].dwLen - dwHashLen; i++) {
                pbSignature[i] = 0xff;
            }
            pbSignature[i++] = 0x00;
            for (j=0; j < aOIDDescriptor[dwIdxOID].dwLen; j++) {
                pbSignature[i++] = aOIDDescriptor[dwIdxOID].abOID[j];
            }
        }
        for (j=0; j < dwHashLen; j++) {
            pbSignature[i++] = abHashValue[j];
        }
    }
    
    return TRUE;
}

/******************************************************************************
 * tls1_p [Internal]
 *
 * This is an implementation of the 'P_hash' helper function for TLS1's PRF.
 * It is used exclusively by tls1_prf. For details see RFC 2246, chapter 5.
 * The pseudo random stream generated by this function is exclusive or'ed with
 * the data in pbBuffer.
 *
 * PARAMS
 *  hHMAC       [I]   HMAC object, which will be used in pseudo random generation
 *  pblobSeed   [I]   Seed value
 *  pbBuffer    [I/O] Pseudo random stream will be xor'ed to the provided data
 *  dwBufferLen [I]   Number of pseudo random bytes desired
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
static BOOL tls1_p(HCRYPTHASH hHMAC, const PCRYPT_DATA_BLOB pblobSeed, BYTE *pbBuffer,
                   DWORD dwBufferLen)
{
    CRYPTHASH *pHMAC;
    BYTE abAi[RSAENH_MAX_HASH_SIZE];
    DWORD i = 0;

    if (!lookup_handle(&handle_table, hHMAC, RSAENH_MAGIC_HASH, (OBJECTHDR**)&pHMAC)) {
        SetLastError(NTE_BAD_HASH);
        return FALSE;
    }
    
    /* compute A_1 = HMAC(seed) */
    init_hash(pHMAC);
    update_hash(pHMAC, pblobSeed->pbData, pblobSeed->cbData);
    finalize_hash(pHMAC);
    memcpy(abAi, pHMAC->abHashValue, pHMAC->dwHashSize);

    do {
        /* compute HMAC(A_i + seed) */
        init_hash(pHMAC);
        update_hash(pHMAC, abAi, pHMAC->dwHashSize);
        update_hash(pHMAC, pblobSeed->pbData, pblobSeed->cbData);
        finalize_hash(pHMAC);

        /* pseudo random stream := CONCAT_{i=1..n} ( HMAC(A_i + seed) ) */
        do {
            if (i >= dwBufferLen) break;
            pbBuffer[i] ^= pHMAC->abHashValue[i % pHMAC->dwHashSize];
            i++;
        } while (i % pHMAC->dwHashSize);

        /* compute A_{i+1} = HMAC(A_i) */
        init_hash(pHMAC);
        update_hash(pHMAC, abAi, pHMAC->dwHashSize);
        finalize_hash(pHMAC);
        memcpy(abAi, pHMAC->abHashValue, pHMAC->dwHashSize);
    } while (i < dwBufferLen);

    return TRUE;
}

/******************************************************************************
 * tls1_prf [Internal]
 *
 * TLS1 pseudo random function as specified in RFC 2246, chapter 5
 *
 * PARAMS
 *  hProv       [I] Key container used to compute the pseudo random stream
 *  hSecret     [I] Key that holds the (pre-)master secret
 *  pblobLabel  [I] Descriptive label
 *  pblobSeed   [I] Seed value
 *  pbBuffer    [O] Pseudo random numbers will be stored here
 *  dwBufferLen [I] Number of pseudo random bytes desired
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */ 
static BOOL tls1_prf(HCRYPTPROV hProv, HCRYPTPROV hSecret, const PCRYPT_DATA_BLOB pblobLabel,
                     const PCRYPT_DATA_BLOB pblobSeed, BYTE *pbBuffer, DWORD dwBufferLen)
{
    HMAC_INFO hmacInfo = { 0, NULL, 0, NULL, 0 };
    HCRYPTHASH hHMAC = (HCRYPTHASH)INVALID_HANDLE_VALUE;
    HCRYPTKEY hHalfSecret = (HCRYPTKEY)INVALID_HANDLE_VALUE;
    CRYPTKEY *pHalfSecret, *pSecret;
    DWORD dwHalfSecretLen;
    BOOL result = FALSE;
    CRYPT_DATA_BLOB blobLabelSeed;

    TRACE("(hProv=%08Ix, hSecret=%08Ix, pblobLabel=%p, pblobSeed=%p, pbBuffer=%p, dwBufferLen=%ld)\n",
          hProv, hSecret, pblobLabel, pblobSeed, pbBuffer, dwBufferLen);

    if (!lookup_handle(&handle_table, hSecret, RSAENH_MAGIC_KEY, (OBJECTHDR**)&pSecret)) {
        SetLastError(NTE_FAIL);
        return FALSE;
    }

    dwHalfSecretLen = (pSecret->dwKeyLen+1)/2;
    
    /* concatenation of the label and the seed */
    if (!concat_data_blobs(&blobLabelSeed, pblobLabel, pblobSeed)) goto exit;
   
    /* zero out the buffer, since two random streams will be xor'ed into it. */
    memset(pbBuffer, 0, dwBufferLen);
   
    /* build a 'fake' key, to hold the secret. CALG_SSL2_MASTER is used since it provides
     * the biggest range of valid key lengths. */
    hHalfSecret = new_key(hProv, CALG_SSL2_MASTER, MAKELONG(0,dwHalfSecretLen*8), &pHalfSecret);
    if (hHalfSecret == (HCRYPTKEY)INVALID_HANDLE_VALUE) goto exit;

    /* Derive an HMAC_MD5 hash and call the helper function. */
    memcpy(pHalfSecret->abKeyValue, pSecret->abKeyValue, dwHalfSecretLen);
    if (!RSAENH_CPCreateHash(hProv, CALG_HMAC, hHalfSecret, 0, &hHMAC)) goto exit;
    hmacInfo.HashAlgid = CALG_MD5;
    if (!RSAENH_CPSetHashParam(hProv, hHMAC, HP_HMAC_INFO, (BYTE*)&hmacInfo, 0)) goto exit;
    if (!tls1_p(hHMAC, &blobLabelSeed, pbBuffer, dwBufferLen)) goto exit;

    /* Reconfigure to HMAC_SHA hash and call helper function again. */
    memcpy(pHalfSecret->abKeyValue, pSecret->abKeyValue + (pSecret->dwKeyLen/2), dwHalfSecretLen);
    hmacInfo.HashAlgid = CALG_SHA;
    if (!RSAENH_CPSetHashParam(hProv, hHMAC, HP_HMAC_INFO, (BYTE*)&hmacInfo, 0)) goto exit;
    if (!tls1_p(hHMAC, &blobLabelSeed, pbBuffer, dwBufferLen)) goto exit;
    
    result = TRUE;
exit:
    release_handle(&handle_table, hHalfSecret, RSAENH_MAGIC_KEY);
    if (hHMAC != (HCRYPTHASH)INVALID_HANDLE_VALUE) RSAENH_CPDestroyHash(hProv, hHMAC);
    free_data_blob(&blobLabelSeed);
    return result;
}

/******************************************************************************
 * pad_data_pkcs1 [Internal]
 *
 * Helper function for data padding according to PKCS1 #2
 *
 * PARAMS
 *  abData      [I] The data to be padded
 *  dwDataLen   [I] Length of the data 
 *  abBuffer    [O] Padded data will be stored here
 *  dwBufferLen [I] Length of the buffer (also length of padded data)
 *  dwFlags     [I] Padding format (CRYPT_SSL2_FALLBACK)
 *
 * RETURN
 *  Success: TRUE
 *  Failure: FALSE (NTE_BAD_LEN, too much data to pad)
 */
static BOOL pad_data_pkcs1(const BYTE *abData, DWORD dwDataLen, BYTE *abBuffer, DWORD dwBufferLen, DWORD dwFlags)
{
    DWORD i;
    
    /* Ensure there is enough space for PKCS1 #2 padding */
    if (dwDataLen > dwBufferLen-11) {
        SetLastError(NTE_BAD_LEN);
        return FALSE;
    }

    memmove(abBuffer + dwBufferLen - dwDataLen, abData, dwDataLen);            
    
    abBuffer[0] = 0x00;
    abBuffer[1] = RSAENH_PKC_BLOCKTYPE; 
    for (i=2; i < dwBufferLen - dwDataLen - 1; i++) 
        do RtlGenRandom(&abBuffer[i], 1); while (!abBuffer[i]);
    if (dwFlags & CRYPT_SSL2_FALLBACK) 
        for (i-=8; i < dwBufferLen - dwDataLen - 1; i++) 
            abBuffer[i] = 0x03;
    abBuffer[i] = 0x00;
    
    return TRUE; 
}

/******************************************************************************
 * pkcs1_mgf1 [Internal]
 *
 * MGF function for RSA EM-OAEP as specified in RFC 8017 PKCS #1 V2.2, Appendix B.2.1. MGF1
 *
 * PARAMS
 *  hProv        [I] Cryptographic provider handle
 *  pbSeed       [I] Seed from which mask is generated
 *  dwSeedLength [I] Length of pbSeed
 *  dwLength     [I] Intended length in octets of the mask
 *  pbMask       [O] Generated mask if success. Caller is responsible for freeing the mask when it's done
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
static BOOL pkcs1_mgf1(HCRYPTPROV hProv, const BYTE *pbSeed, DWORD dwSeedLength, DWORD dwLength, PCRYPT_DATA_BLOB pbMask)
{
    HCRYPTHASH hHash;
    BYTE *pbHashInput, *pbCounter;
    DWORD dwCounter;
    DWORD dwLen, dwHashLen;

    RSAENH_CPCreateHash(hProv, CALG_SHA1, 0, 0, &hHash);
    RSAENH_CPHashData(hProv, hHash, 0, 0, 0);
    dwLen = sizeof(dwHashLen);
    RSAENH_CPGetHashParam(hProv, hHash, HP_HASHSIZE, (BYTE *)&dwHashLen, &dwLen, 0);
    RSAENH_CPDestroyHash(hProv, hHash);

    /* Allocate multiples of hash value */
    pbMask->pbData = malloc((dwLength + dwHashLen - 1) / dwHashLen * dwHashLen);
    if (!pbMask->pbData)
    {
        SetLastError(NTE_NO_MEMORY);
        return FALSE;
    }
    pbMask->cbData = dwLength;

    pbHashInput = malloc(dwSeedLength + sizeof(DWORD));
    if (!pbHashInput)
    {
        free_data_blob(pbMask);
        SetLastError(NTE_NO_MEMORY);
        return FALSE;
    }

    dwLen = dwHashLen;
    memcpy(pbHashInput, pbSeed, dwSeedLength);
    pbCounter = pbHashInput + dwSeedLength;
    for (dwCounter = 0; dwCounter < (dwLength + dwHashLen - 1) / dwHashLen; dwCounter++)
    {
        *(pbCounter) = (BYTE)((dwCounter >> 24) & 0xff);
        *(pbCounter + 1) = (BYTE)((dwCounter >> 16) & 0xff);
        *(pbCounter + 2) = (BYTE)((dwCounter >> 8) & 0xff);
        *(pbCounter + 3) = (BYTE)(dwCounter & 0xff);
        RSAENH_CPCreateHash(hProv, CALG_SHA1, 0, 0, &hHash);
        RSAENH_CPHashData(hProv, hHash, pbHashInput, dwSeedLength + sizeof(DWORD), 0);
        /* pbMask->pbData = old pbMask->pbData || Hash(Seed || Counter) */
        RSAENH_CPGetHashParam(hProv, hHash, HP_HASHVAL, pbMask->pbData + dwCounter * dwHashLen, &dwLen, 0);
        RSAENH_CPDestroyHash(hProv, hHash);
    }

    free(pbHashInput);
    return TRUE;
}

/******************************************************************************
 * pad_data_oaep [Internal]
 *
 * Helper function for data OAEP padding scheme according to RFC 8017 PKCS #1 V2.2
 *
 * PARAMS
 *  hProv       [I] Cryptographic provider handle
 *  abData      [I] The data to be padded
 *  dwDataLen   [I] Length of the data
 *  abBuffer    [O] Padded data will be stored here
 *  dwBufferLen [I] Length of the buffer (also length of padded data)
 *  dwFlags     [I] Currently only CRYPT_OAEP is defined
 *
 * RETURN
 *  Success: TRUE
 *  Failure: FALSE
 */
static BOOL pad_data_oaep(HCRYPTPROV hProv, const BYTE *abData, DWORD dwDataLen, BYTE *abBuffer, DWORD dwBufferLen,
                          DWORD dwFlags)
{
    CRYPT_DATA_BLOB blobDbMask = {0}, blobSeedMask = {0};
    HCRYPTHASH hHash;
    BYTE *pbPadded = NULL, *pbDb, *pbSeed;
    DWORD dwLen, dwHashLen;
    DWORD dwDbLen, dwSeedLen;
    BOOL result, ret = FALSE;
    DWORD i;

    RSAENH_CPCreateHash(hProv, CALG_SHA1, 0, 0, &hHash);
    /* Empty label */
    RSAENH_CPHashData(hProv, hHash, 0, 0, 0);
    dwLen = sizeof(dwHashLen);
    RSAENH_CPGetHashParam(hProv, hHash, HP_HASHSIZE, (BYTE *)&dwHashLen, &dwLen, 0);

    if (dwDataLen > dwBufferLen - 2 * dwHashLen - 2)
    {
        SetLastError(NTE_BAD_LEN);
        goto done;
    }

    if (dwBufferLen < 2 * dwHashLen + 2)
    {
        SetLastError(ERROR_MORE_DATA);
        goto done;
    }

    pbPadded = malloc(dwBufferLen);
    if (!pbPadded)
    {
        SetLastError(NTE_NO_MEMORY);
        goto done;
    }

    /* EM = 00 || maskedSeed || maskedDB */
    pbPadded[0] = 0;
    pbSeed = pbPadded + 1;
    dwSeedLen = dwHashLen;
    pbDb = pbPadded + 1 + dwHashLen;
    dwDbLen = dwBufferLen - dwSeedLen - 1;

    /* DB = pHash || PS || 01 || M */
    /* Set pHash in DB */
    dwLen = dwHashLen;
    RSAENH_CPGetHashParam(hProv, hHash, HP_HASHVAL, pbDb, &dwLen, 0);
    /* Set PS(zeros) in DB */
    memset(pbDb + dwHashLen, 0, dwDbLen - dwHashLen - 1 - dwDataLen);
    /* Set 01 in DB */
    pbDb[dwDbLen - dwDataLen - 1] = 1;
    /* Set M in DB */
    memcpy(pbDb + dwDbLen - dwDataLen, abData, dwDataLen);

    /* Get seed */
    RtlGenRandom(pbSeed, dwHashLen);
    /* Get masked DB */
    result = pkcs1_mgf1(hProv, pbSeed, dwHashLen, dwDbLen, &blobDbMask);
    if (!result) goto done;
    for (i = 0; i < dwDbLen; i++) pbDb[i] ^= blobDbMask.pbData[i];

    /* Get masked seed */
    result = pkcs1_mgf1(hProv, pbDb, dwDbLen, dwHashLen, &blobSeedMask);
    if (!result) goto done;
    for (i = 0; i < dwHashLen; i++) pbSeed[i] ^= blobSeedMask.pbData[i];

    memcpy(abBuffer, pbPadded, dwBufferLen);
    ret = TRUE;
done:
    RSAENH_CPDestroyHash(hProv, hHash);
    free(pbPadded);
    free_data_blob(&blobDbMask);
    free_data_blob(&blobSeedMask);
    return ret;
}

/******************************************************************************
 * pad_data [Internal]
 *
 * Helper function for data padding according to padding format
 *
 * PARAMS
 *  hProv       [I] Cryptographic provider handle
 *  abData      [I] The data to be padded
 *  dwDataLen   [I] Length of the data
 *  abBuffer    [O] Padded data will be stored here
 *  dwBufferLen [I] Length of the buffer (also length of padded data)
 *  dwFlags     [I] 0, CRYPT_SSL2_FALLBACK or CRYPT_OAEP
 *
 * RETURN
 *  Success: TRUE
 *  Failure: FALSE
 */
static BOOL pad_data(HCRYPTPROV hProv, const BYTE *abData, DWORD dwDataLen, BYTE *abBuffer, DWORD dwBufferLen,
                     DWORD dwFlags)
{
    if (dwFlags == CRYPT_OAEP)
        return pad_data_oaep(hProv, abData, dwDataLen, abBuffer, dwBufferLen, dwFlags);
    else
        return pad_data_pkcs1(abData, dwDataLen, abBuffer, dwBufferLen, dwFlags);
}

/******************************************************************************
 * unpad_data_pkcs1 [Internal]
 *
 * Remove the PKCS1 padding from RSA decrypted data
 *
 * PARAMS
 *  abData      [I]   The padded data
 *  dwDataLen   [I]   Length of the padded data
 *  abBuffer    [O]   Data without padding will be stored here
 *  dwBufferLen [I/O] I: Length of the buffer, O: Length of unpadded data
 *  dwFlags     [I]   Currently none defined
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE, (NTE_BAD_DATA, no valid PKCS1 padding or buffer too small)
 */
static BOOL unpad_data_pkcs1(const BYTE *abData, DWORD dwDataLen, BYTE *abBuffer, DWORD *dwBufferLen, DWORD dwFlags)
{
    DWORD i;
    
    if (dwDataLen < 3)
    {
        SetLastError(NTE_BAD_DATA);
        return FALSE;
    }
    for (i=2; i<dwDataLen; i++)
        if (!abData[i])
            break;

    if ((i == dwDataLen) || (*dwBufferLen < dwDataLen - i - 1) ||
        (abData[0] != 0x00) || (abData[1] != RSAENH_PKC_BLOCKTYPE))
    {
        SetLastError(NTE_BAD_DATA);
        return FALSE;
    }

    *dwBufferLen = dwDataLen - i - 1;
    memmove(abBuffer, abData + i + 1, *dwBufferLen);
    return TRUE;
}

/******************************************************************************
 * unpad_data_oaep [Internal]
 *
 * Remove the OAEP padding from RSA decrypted data
 *
 * PARAMS
 *  hProv       [I]   Cryptographic provider handle
 *  abData      [I]   The padded data
 *  dwDataLen   [I]   Length of the padded data
 *  abBuffer    [O]   Data without padding will be stored here
 *  dwBufferLen [I/O] I: Length of the buffer, O: Length of unpadded data
 *  dwFlags     [I]   Currently only CRYPT_OAEP is defined
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
static BOOL unpad_data_oaep(HCRYPTPROV hProv, const BYTE *abData, DWORD dwDataLen, BYTE *abBuffer, DWORD *dwBufferLen,
                            DWORD dwFlags)
{
    CRYPT_DATA_BLOB blobDbMask = {0}, blobSeedMask = {0};
    HCRYPTHASH hHash;
    BYTE *pbBuffer = NULL, *pbHashValue = NULL;
    const BYTE *pbPaddedSeed, *pbPaddedDb;
    BYTE *pbUnpaddedSeed, *pbUnpaddedDb;
    DWORD dwLen, dwHashLen;
    DWORD dwSeedLen, dwDbLen;
    DWORD dwZeroCount, dwMsgCount;
    BOOL result, ret = FALSE;
    DWORD i;

    RSAENH_CPCreateHash(hProv, CALG_SHA1, 0, 0, &hHash);
    RSAENH_CPHashData(hProv, hHash, 0, 0, 0);
    dwLen = sizeof(dwHashLen);
    RSAENH_CPGetHashParam(hProv, hHash, HP_HASHSIZE, (BYTE *)&dwHashLen, &dwLen, 0);
    if (dwDataLen < 2 * dwHashLen + 2)
    {
        SetLastError(NTE_BAD_DATA);
        goto done;
    }

    /* Get default hash value */
    pbHashValue = malloc(dwHashLen);
    if (!pbHashValue)
    {
        SetLastError(NTE_NO_MEMORY);
        goto done;
    }
    dwLen = dwHashLen;
    RSAENH_CPGetHashParam(hProv, hHash, HP_HASHVAL, pbHashValue, &dwLen, 0);

    /* Store seed and DB */
    pbBuffer = malloc(dwDataLen - 1);
    if (!pbBuffer)
    {
        SetLastError(NTE_NO_MEMORY);
        goto done;
    }

    pbPaddedSeed = abData + 1;
    pbPaddedDb = abData + 1 + dwHashLen;
    pbUnpaddedSeed = pbBuffer;
    pbUnpaddedDb = pbBuffer + dwHashLen;
    dwSeedLen = dwHashLen;
    dwDbLen = dwDataLen - dwHashLen - 1;

    /* Get unpadded seed */
    result = pkcs1_mgf1(hProv, pbPaddedDb, dwDbLen, dwSeedLen, &blobSeedMask);
    if (!result) goto done;
    for (i = 0; i < dwSeedLen; i++) pbUnpaddedSeed[i] = pbPaddedSeed[i] ^ blobSeedMask.pbData[i];

    /* Get unpadded DB */
    result = pkcs1_mgf1(hProv, pbUnpaddedSeed, dwSeedLen, dwDbLen, &blobDbMask);
    if (!result) goto done;
    for (i = 0; i < dwDbLen; i++) pbUnpaddedDb[i] = pbPaddedDb[i] ^ blobDbMask.pbData[i];

    /* Compare hash in DB */
    result = memcmp(pbUnpaddedDb, pbHashValue, dwHashLen);

    /* Get count of zero paddings(PS) */
    dwZeroCount = 0;
    while (dwHashLen + dwZeroCount + 1 <= dwDbLen && pbUnpaddedDb[dwHashLen + dwZeroCount] == 0) dwZeroCount++;
    dwMsgCount = dwDbLen - dwHashLen - dwZeroCount - 1;

    if (dwHashLen + dwZeroCount + 1 > dwDbLen || abData[0] || result || pbUnpaddedDb[dwHashLen + dwZeroCount] != 1
        || *dwBufferLen < dwMsgCount)
    {
        SetLastError(NTE_BAD_DATA);
        goto done;
    }

    *dwBufferLen = dwMsgCount;
    memcpy(abBuffer, pbUnpaddedDb + dwHashLen + dwZeroCount + 1, dwMsgCount);
    ret = TRUE;
done:
    RSAENH_CPDestroyHash(hProv, hHash);
    free(pbHashValue);
    free(pbBuffer);
    free_data_blob(&blobDbMask);
    free_data_blob(&blobSeedMask);
    return ret;
}

/******************************************************************************
 * unpad_data [Internal]
 *
 * Remove the padding from RSA decrypted data according to padding format
 *
 * PARAMS
 *  hProv       [I]   Cryptographic provider handle
 *  abData      [I]   The padded data
 *  dwDataLen   [I]   Length of the padded data
 *  abBuffer    [O]   Data without padding will be stored here
 *  dwBufferLen [I/O] I: Length of the buffer, O: Length of unpadded data
 *  dwFlags     [I]   0 or CRYPT_OAEP
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
static BOOL unpad_data(HCRYPTPROV hProv, const BYTE *abData, DWORD dwDataLen, BYTE *abBuffer, DWORD *dwBufferLen,
                       DWORD dwFlags)
{
    if (dwFlags == CRYPT_OAEP)
        return unpad_data_oaep(hProv, abData, dwDataLen, abBuffer, dwBufferLen, dwFlags);
    else
        return unpad_data_pkcs1(abData, dwDataLen, abBuffer, dwBufferLen, dwFlags);
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
    CHAR szKeyContainerName[MAX_PATH];

    TRACE("(phProv=%p, pszContainer=%s, dwFlags=%08lx, pVTable=%p)\n", phProv,
          debugstr_a(pszContainer), dwFlags, pVTable);

    if (pszContainer && *pszContainer)
    {
        lstrcpynA(szKeyContainerName, pszContainer, MAX_PATH);
    } 
    else
    {
        DWORD dwLen = sizeof(szKeyContainerName);
        if (!GetUserNameA(szKeyContainerName, &dwLen)) return FALSE;
    }

    switch (dwFlags & (CRYPT_NEWKEYSET|CRYPT_VERIFYCONTEXT|CRYPT_DELETEKEYSET)) 
    {
        case 0:
            *phProv = read_key_container(szKeyContainerName, dwFlags, pVTable);
            break;

        case CRYPT_DELETEKEYSET:
            return delete_container_key(szKeyContainerName, dwFlags);

        case CRYPT_NEWKEYSET:
            *phProv = read_key_container(szKeyContainerName, dwFlags, pVTable);
            if (*phProv != (HCRYPTPROV)INVALID_HANDLE_VALUE) 
            {
                release_handle(&handle_table, *phProv, RSAENH_MAGIC_CONTAINER);
                TRACE("Can't create new keyset, already exists\n");
                SetLastError(NTE_EXISTS);
                return FALSE;
            }
            *phProv = new_key_container(szKeyContainerName, dwFlags, pVTable);
            break;

        case CRYPT_VERIFYCONTEXT|CRYPT_NEWKEYSET:
        case CRYPT_VERIFYCONTEXT:
            if (pszContainer && *pszContainer) {
                TRACE("pszContainer should be empty\n");
                SetLastError(NTE_BAD_FLAGS);
                return FALSE;
            }
            *phProv = new_key_container("", dwFlags, pVTable);
            break;
            
        default:
            *phProv = (HCRYPTPROV)INVALID_HANDLE_VALUE;
            SetLastError(NTE_BAD_FLAGS);
            return FALSE;
    }
                
    if (*phProv != (HCRYPTPROV)INVALID_HANDLE_VALUE) {
        SetLastError(ERROR_SUCCESS);
        return TRUE;
    } else {
        return FALSE;
    }
}

/******************************************************************************
 * CPCreateHash (RSAENH.@)
 *
 * CPCreateHash creates and initializes a new hash object.
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
    CRYPTKEY *pCryptKey = NULL;
    CRYPTHASH *pCryptHash;
    const PROV_ENUMALGS_EX *peaAlgidInfo;
        
    TRACE("(hProv=%08Ix, Algid=%08x, hKey=%08Ix, dwFlags=%08lx, phHash=%p)\n", hProv, Algid, hKey,
          dwFlags, phHash);

    peaAlgidInfo = get_algid_info(hProv, Algid);
    if (!peaAlgidInfo) return FALSE;

    if (dwFlags)
    {
        SetLastError(NTE_BAD_FLAGS);
        return FALSE;
    }

    if (Algid == CALG_MAC || Algid == CALG_HMAC || Algid == CALG_SCHANNEL_MASTER_HASH || 
        Algid == CALG_TLS1PRF) 
    {
        if (!lookup_handle(&handle_table, hKey, RSAENH_MAGIC_KEY, (OBJECTHDR**)&pCryptKey)) {
            SetLastError(NTE_BAD_KEY);
            return FALSE;
        }

        if ((Algid == CALG_MAC) && (GET_ALG_TYPE(pCryptKey->aiAlgid) != ALG_TYPE_BLOCK)) {
            SetLastError(NTE_BAD_KEY);
            return FALSE;
        }

        if ((Algid == CALG_SCHANNEL_MASTER_HASH || Algid == CALG_TLS1PRF) && 
            (pCryptKey->aiAlgid != CALG_TLS1_MASTER)) 
        {
            SetLastError(NTE_BAD_KEY);
            return FALSE;
        }
        if (Algid == CALG_SCHANNEL_MASTER_HASH &&
            ((!pCryptKey->siSChannelInfo.blobClientRandom.cbData) ||
             (!pCryptKey->siSChannelInfo.blobServerRandom.cbData)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        if ((Algid == CALG_TLS1PRF) && (pCryptKey->dwState != RSAENH_KEYSTATE_MASTERKEY)) {
            SetLastError(NTE_BAD_KEY_STATE);
            return FALSE;
        }
    }

    *phHash = new_object(&handle_table, sizeof(CRYPTHASH), RSAENH_MAGIC_HASH,
                         destroy_hash, (OBJECTHDR**)&pCryptHash);
    if (!pCryptHash) return FALSE;

    pCryptHash->aiAlgid = Algid;
    pCryptHash->hKey = hKey;
    if (Algid == CALG_MAC)
    {
        pCryptHash->key_alg_id = pCryptKey->aiAlgid;
        setup_key(pCryptKey);
        duplicate_key_impl(pCryptKey->aiAlgid, &pCryptKey->context, &pCryptHash->key_context);
        memcpy(pCryptHash->abChainVector, pCryptKey->abChainVector, sizeof(pCryptHash->abChainVector));
    }
    pCryptHash->hProv = hProv;
    pCryptHash->dwState = RSAENH_HASHSTATE_HASHING;
    pCryptHash->pHMACInfo = NULL;
    pCryptHash->hash_handle = NULL;
    pCryptHash->dwHashSize = peaAlgidInfo->dwDefaultLen >> 3;
    pCryptHash->buffered_hash_bytes = 0;
    init_data_blob(&pCryptHash->tpPRFParams.blobLabel);
    init_data_blob(&pCryptHash->tpPRFParams.blobSeed);

    if (Algid == CALG_SCHANNEL_MASTER_HASH) {
        static const char keyex[] = "key expansion";
        BYTE key_expansion[sizeof keyex];
        CRYPT_DATA_BLOB blobRandom, blobKeyExpansion = { 13, key_expansion };

        memcpy( key_expansion, keyex, sizeof keyex );
        
        if (pCryptKey->dwState != RSAENH_KEYSTATE_MASTERKEY) {
            static const char msec[] = "master secret";
            BYTE master_secret[sizeof msec];
            CRYPT_DATA_BLOB blobLabel = { 13, master_secret };
            BYTE abKeyValue[48];

            memcpy( master_secret, msec, sizeof msec );
    
            /* See RFC 2246, chapter 8.1 */
            if (!concat_data_blobs(&blobRandom, 
                                   &pCryptKey->siSChannelInfo.blobClientRandom, 
                                   &pCryptKey->siSChannelInfo.blobServerRandom))
            {
                return FALSE;
            }
            tls1_prf(hProv, hKey, &blobLabel, &blobRandom, abKeyValue, 48);
            pCryptKey->dwState = RSAENH_KEYSTATE_MASTERKEY; 
            memcpy(pCryptKey->abKeyValue, abKeyValue, 48);
            free_data_blob(&blobRandom);
        }

        /* See RFC 2246, chapter 6.3 */
        if (!concat_data_blobs(&blobRandom, 
                                  &pCryptKey->siSChannelInfo.blobServerRandom, 
                                  &pCryptKey->siSChannelInfo.blobClientRandom))
        {
            return FALSE;
        }
        tls1_prf(hProv, hKey, &blobKeyExpansion, &blobRandom, pCryptHash->abHashValue, 
                 RSAENH_MAX_HASH_SIZE);
        free_data_blob(&blobRandom);
    }

    return init_hash(pCryptHash);
}

/******************************************************************************
 * CPDestroyHash (RSAENH.@)
 * 
 * Releases the handle to a hash object. The object is destroyed if its reference
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
    TRACE("(hProv=%08Ix, hHash=%08Ix)\n", hProv, hHash);
     
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
 * Releases the handle to a key object. The object is destroyed if its reference
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
    TRACE("(hProv=%08Ix, hKey=%08Ix)\n", hProv, hKey);
        
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
 * Clones a hash object including its current state.
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
    
    TRACE("(hUID=%08Ix, hHash=%08Ix, pdwReserved=%p, dwFlags=%08lx, phHash=%p)\n", hUID, hHash,
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

    *phHash = new_object(&handle_table, sizeof(CRYPTHASH), RSAENH_MAGIC_HASH,
                         destroy_hash, (OBJECTHDR**)&pDestHash);
    if (*phHash != (HCRYPTHASH)INVALID_HANDLE_VALUE)
    {
        *pDestHash = *pSrcHash;
        duplicate_hash_impl(pSrcHash->hash_handle, &pDestHash->hash_handle);
        copy_hmac_info(&pDestHash->pHMACInfo, pSrcHash->pHMACInfo);
        copy_data_blob(&pDestHash->tpPRFParams.blobLabel, &pSrcHash->tpPRFParams.blobLabel);
        copy_data_blob(&pDestHash->tpPRFParams.blobSeed, &pSrcHash->tpPRFParams.blobSeed);
    }

    return *phHash != (HCRYPTHASH)INVALID_HANDLE_VALUE;
}

/******************************************************************************
 * CPDuplicateKey (RSAENH.@)
 *
 * Clones a key object including its current state.
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
    
    TRACE("(hUID=%08Ix, hKey=%08Ix, pdwReserved=%p, dwFlags=%08lx, phKey=%p)\n", hUID, hKey,
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

    *phKey = new_object(&handle_table, sizeof(CRYPTKEY), RSAENH_MAGIC_KEY, destroy_key,
                        (OBJECTHDR**)&pDestKey);
    if (*phKey != (HCRYPTKEY)INVALID_HANDLE_VALUE)
    {
        *pDestKey = *pSrcKey;
        copy_data_blob(&pDestKey->siSChannelInfo.blobServerRandom,
                       &pSrcKey->siSChannelInfo.blobServerRandom);
        copy_data_blob(&pDestKey->siSChannelInfo.blobClientRandom, 
                       &pSrcKey->siSChannelInfo.blobClientRandom);
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
 *  dwFlags    [I]   Must be zero or CRYPT_OAEP
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
 */
BOOL WINAPI RSAENH_CPEncrypt(HCRYPTPROV hProv, HCRYPTKEY hKey, HCRYPTHASH hHash, BOOL Final, 
                             DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen, DWORD dwBufLen)
{
    CRYPTKEY *pCryptKey;
        
    TRACE("(hProv=%08Ix, hKey=%08Ix, hHash=%08Ix, Final=%d, dwFlags=%08lx, pbData=%p, "
          "pdwDataLen=%p, dwBufLen=%ld)\n", hProv, hKey, hHash, Final, dwFlags, pbData, pdwDataLen,
          dwBufLen);
    
    if (!is_valid_handle(&handle_table, hProv, RSAENH_MAGIC_CONTAINER))
    {
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }

    if (dwFlags != 0 && dwFlags != CRYPT_OAEP)
    {
        SetLastError(NTE_BAD_FLAGS);
        return FALSE;
    }

    if (!lookup_handle(&handle_table, hKey, RSAENH_MAGIC_KEY, (OBJECTHDR**)&pCryptKey))
    {
        SetLastError(NTE_BAD_KEY);
        return FALSE;
    }

    if (pCryptKey->aiAlgid == CALG_RC2)
    {
        const PROV_ENUMALGS_EX *info;

        if (!(info = get_algid_info(hProv, pCryptKey->aiAlgid)))
        {
            FIXME("Can't get algid info.\n");
            SetLastError(NTE_BAD_KEY);
            return FALSE;
        }
        if (pCryptKey->dwKeyLen > info->dwMaxLen / 8)
        {
            SetLastError(NTE_BAD_KEY);
            return FALSE;
        }
    }

    if (pCryptKey->dwState == RSAENH_KEYSTATE_IDLE) 
        pCryptKey->dwState = RSAENH_KEYSTATE_ENCRYPTING;

    if (pCryptKey->dwState != RSAENH_KEYSTATE_ENCRYPTING) 
    {
        SetLastError(NTE_BAD_DATA);
        return FALSE;
    }

    if (is_valid_handle(&handle_table, hHash, RSAENH_MAGIC_HASH)) {
        if (!RSAENH_CPHashData(hProv, hHash, pbData, *pdwDataLen, 0)) return FALSE;
    }
    
    if (GET_ALG_TYPE(pCryptKey->aiAlgid) == ALG_TYPE_BLOCK) {
        if (!block_encrypt(pCryptKey, pbData, pdwDataLen, dwBufLen, Final,
                           &pCryptKey->context, pCryptKey->abChainVector))
            return FALSE;
    } else if (GET_ALG_TYPE(pCryptKey->aiAlgid) == ALG_TYPE_STREAM) {
        if (pbData == NULL) {
            *pdwDataLen = dwBufLen;
            return TRUE;
        }
        encrypt_stream_impl(pCryptKey->aiAlgid, &pCryptKey->context, pbData, *pdwDataLen);
    } else if (GET_ALG_TYPE(pCryptKey->aiAlgid) == ALG_TYPE_RSA) {
        if (pCryptKey->aiAlgid == CALG_RSA_SIGN) {
            SetLastError(NTE_BAD_KEY);
            return FALSE;
        }
        if (!pbData) {
            *pdwDataLen = pCryptKey->dwBlockLen;
            return TRUE;
        }
        if (dwBufLen < pCryptKey->dwBlockLen) {
            SetLastError(ERROR_MORE_DATA);
            return FALSE;
        }
        if (!pad_data(hProv, pbData, *pdwDataLen, pbData, pCryptKey->dwBlockLen, dwFlags)) return FALSE;
        encrypt_block_impl(pCryptKey->aiAlgid, PK_PUBLIC, &pCryptKey->context, pbData, pbData);
        *pdwDataLen = pCryptKey->dwBlockLen;
        Final = TRUE;
    } else {
        SetLastError(NTE_BAD_TYPE);
        return FALSE;
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
 *  dwFlags    [I]   Must be zero or CRYPT_OAEP
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
 */
BOOL WINAPI RSAENH_CPDecrypt(HCRYPTPROV hProv, HCRYPTKEY hKey, HCRYPTHASH hHash, BOOL Final, 
                             DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen)
{
    CRYPTKEY *pCryptKey;
    BYTE *in, out[RSAENH_MAX_BLOCK_SIZE], o[RSAENH_MAX_BLOCK_SIZE];
    DWORD i, j, k;
    DWORD dwMax;

    TRACE("(hProv=%08Ix, hKey=%08Ix, hHash=%08Ix, Final=%d, dwFlags=%08lx, pbData=%p, "
          "pdwDataLen=%p)\n", hProv, hKey, hHash, Final, dwFlags, pbData, pdwDataLen);
    
    if (!is_valid_handle(&handle_table, hProv, RSAENH_MAGIC_CONTAINER))
    {
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }

    if (dwFlags != 0 && dwFlags != CRYPT_OAEP)
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

    if (Final && !*pdwDataLen)
    {
        SetLastError(NTE_BAD_LEN);
        return FALSE;
    }

    dwMax=*pdwDataLen;

    if (GET_ALG_TYPE(pCryptKey->aiAlgid) == ALG_TYPE_BLOCK) {
        for (i=0, in=pbData; i<*pdwDataLen; i+=pCryptKey->dwBlockLen, in+=pCryptKey->dwBlockLen) {
            switch (pCryptKey->dwMode) {
                case CRYPT_MODE_ECB:
                    decrypt_block_impl(pCryptKey->aiAlgid, 0, &pCryptKey->context, in, out);
                    break;
                
                case CRYPT_MODE_CBC:
                    decrypt_block_impl(pCryptKey->aiAlgid, 0, &pCryptKey->context, in, out);
                    for (j=0; j<pCryptKey->dwBlockLen; j++) out[j] ^= pCryptKey->abChainVector[j];
                    memcpy(pCryptKey->abChainVector, in, pCryptKey->dwBlockLen);
                    break;

                case CRYPT_MODE_CFB:
                    for (j=0; j<pCryptKey->dwBlockLen; j++) {
                        encrypt_block_impl(pCryptKey->aiAlgid, 0, &pCryptKey->context, pCryptKey->abChainVector, o);
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
        if (Final) {
            if (pbData[*pdwDataLen-1] &&
             pbData[*pdwDataLen-1] <= pCryptKey->dwBlockLen &&
             pbData[*pdwDataLen-1] <= *pdwDataLen) {
                BOOL padOkay = TRUE;

                /* check that every pad byte has the same value */
                for (i = 1; padOkay && i < pbData[*pdwDataLen-1]; i++)
                    if (pbData[*pdwDataLen - i - 1] != pbData[*pdwDataLen - 1])
                        padOkay = FALSE;
                if (padOkay)
                    *pdwDataLen -= pbData[*pdwDataLen-1];
                else {
                    SetLastError(NTE_BAD_DATA);
                    setup_key(pCryptKey);
                    return FALSE;
                }
            }
            else {
                SetLastError(NTE_BAD_DATA);
                setup_key(pCryptKey);
                return FALSE;
            }
        }

    } else if (GET_ALG_TYPE(pCryptKey->aiAlgid) == ALG_TYPE_STREAM) {
        encrypt_stream_impl(pCryptKey->aiAlgid, &pCryptKey->context, pbData, *pdwDataLen);
    } else if (GET_ALG_TYPE(pCryptKey->aiAlgid) == ALG_TYPE_RSA) {
        if (pCryptKey->aiAlgid == CALG_RSA_SIGN) {
            SetLastError(NTE_BAD_KEY);
            return FALSE;
        }
        decrypt_block_impl(pCryptKey->aiAlgid, PK_PRIVATE, &pCryptKey->context, pbData, pbData);
        if (!unpad_data(hProv, pbData, pCryptKey->dwBlockLen, pbData, pdwDataLen, dwFlags)) return FALSE;
        Final = TRUE;
    } else {
        SetLastError(NTE_BAD_TYPE);
        return FALSE;
    } 
    
    if (Final) setup_key(pCryptKey);

    if (is_valid_handle(&handle_table, hHash, RSAENH_MAGIC_HASH)) {
        if (*pdwDataLen>dwMax ||
            !RSAENH_CPHashData(hProv, hHash, pbData, *pdwDataLen, 0)) return FALSE;
    }
    
    return TRUE;
}

static BOOL crypt_export_simple(CRYPTKEY *pCryptKey, CRYPTKEY *pPubKey,
    DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen)
{
    BLOBHEADER *pBlobHeader = (BLOBHEADER*)pbData;
    ALG_ID *pAlgid = (ALG_ID*)(pBlobHeader+1);
    DWORD dwDataLen;

    if (!(GET_ALG_CLASS(pCryptKey->aiAlgid)&(ALG_CLASS_DATA_ENCRYPT|ALG_CLASS_MSG_ENCRYPT))) {
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

        if (!pad_data(pCryptKey->hProv, pCryptKey->abKeyValue, pCryptKey->dwKeyLen, (BYTE*)(pAlgid+1),
                      pPubKey->dwBlockLen, dwFlags))
        {
            return FALSE;
        }

        encrypt_block_impl(pPubKey->aiAlgid, PK_PUBLIC, &pPubKey->context, (BYTE*)(pAlgid+1), (BYTE*)(pAlgid+1));
    }
    *pdwDataLen = dwDataLen;
    return TRUE;
}

static BOOL crypt_export_public_key(CRYPTKEY *pCryptKey, BYTE *pbData,
    DWORD *pdwDataLen)
{
    BLOBHEADER *pBlobHeader = (BLOBHEADER*)pbData;
    RSAPUBKEY *pRSAPubKey = (RSAPUBKEY*)(pBlobHeader+1);
    DWORD dwDataLen;

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
}

static BOOL crypt_export_private_key(CRYPTKEY *pCryptKey, BOOL force,
    BYTE *pbData, DWORD *pdwDataLen)
{
    BLOBHEADER *pBlobHeader = (BLOBHEADER*)pbData;
    RSAPUBKEY *pRSAPubKey = (RSAPUBKEY*)(pBlobHeader+1);
    DWORD dwDataLen;

    if ((pCryptKey->aiAlgid != CALG_RSA_KEYX) && (pCryptKey->aiAlgid != CALG_RSA_SIGN)) {
        SetLastError(NTE_BAD_KEY);
        return FALSE;
    }
    if (!force && !(pCryptKey->dwPermissions & CRYPT_EXPORT))
    {
        SetLastError(NTE_BAD_KEY_STATE);
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
}

static BOOL crypt_export_plaintext_key(CRYPTKEY *pCryptKey, BYTE *pbData,
    DWORD *pdwDataLen)
{
    BLOBHEADER *pBlobHeader = (BLOBHEADER*)pbData;
    DWORD *pKeyLen = (DWORD*)(pBlobHeader+1);
    BYTE *pbKey = (BYTE*)(pKeyLen+1);
    DWORD dwDataLen;

    dwDataLen = sizeof(BLOBHEADER) + sizeof(DWORD) + pCryptKey->dwKeyLen;
    if (pbData) {
        if (*pdwDataLen < dwDataLen) {
            SetLastError(ERROR_MORE_DATA);
            *pdwDataLen = dwDataLen;
            return FALSE;
        }

        pBlobHeader->bType = PLAINTEXTKEYBLOB;
        pBlobHeader->bVersion = CUR_BLOB_VERSION;
        pBlobHeader->reserved = 0;
        pBlobHeader->aiKeyAlg = pCryptKey->aiAlgid;

        *pKeyLen = pCryptKey->dwKeyLen;
        memcpy(pbKey, pCryptKey->abKeyValue, pCryptKey->dwKeyLen);
    }
    *pdwDataLen = dwDataLen;
    return TRUE;
}
/******************************************************************************
 * crypt_export_key [Internal]
 *
 * Export a key into a binary large object (BLOB).  Called by CPExportKey and
 * by store_key_pair.
 *
 * PARAMS
 *  pCryptKey  [I]   Key to be exported.
 *  hPubKey    [I]   Key used to encrypt sensitive BLOB data.
 *  dwBlobType [I]   SIMPLEBLOB, PUBLICKEYBLOB or PRIVATEKEYBLOB.
 *  dwFlags    [I]   Currently none defined.
 *  force      [I]   If TRUE, the key is written no matter what the key's
 *                   permissions are.  Otherwise the key's permissions are
 *                   checked before exporting.
 *  pbData     [O]   Pointer to a buffer where the BLOB will be written to.
 *  pdwDataLen [I/O] I: Size of buffer at pbData, O: Size of BLOB
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 */
static BOOL crypt_export_key(CRYPTKEY *pCryptKey, HCRYPTKEY hPubKey,
                             DWORD dwBlobType, DWORD dwFlags, BOOL force,
                             BYTE *pbData, DWORD *pdwDataLen)
{
    CRYPTKEY *pPubKey;
    
    if (dwFlags & CRYPT_SSL2_FALLBACK) {
        if (pCryptKey->aiAlgid != CALG_SSL2_MASTER) {
            SetLastError(NTE_BAD_KEY);
            return FALSE;
        }
    }
    
    switch ((BYTE)dwBlobType)
    {
        case SIMPLEBLOB:
            if (!lookup_handle(&handle_table, hPubKey, RSAENH_MAGIC_KEY, (OBJECTHDR**)&pPubKey)){
                SetLastError(NTE_BAD_PUBLIC_KEY); /* FIXME: error_code? */
                return FALSE;
            }
            return crypt_export_simple(pCryptKey, pPubKey, dwFlags, pbData,
                                       pdwDataLen);
            
        case PUBLICKEYBLOB:
            if (is_valid_handle(&handle_table, hPubKey, RSAENH_MAGIC_KEY)) {
                SetLastError(NTE_BAD_KEY); /* FIXME: error code? */
                return FALSE;
            }

            return crypt_export_public_key(pCryptKey, pbData, pdwDataLen);

        case PRIVATEKEYBLOB:
            return crypt_export_private_key(pCryptKey, force, pbData, pdwDataLen);

        case PLAINTEXTKEYBLOB:
            return crypt_export_plaintext_key(pCryptKey, pbData, pdwDataLen);
            
        default:
            SetLastError(NTE_BAD_TYPE); /* FIXME: error code? */
            return FALSE;
    }
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
    CRYPTKEY *pCryptKey;

    TRACE("(hProv=%08Ix, hKey=%08Ix, hPubKey=%08Ix, dwBlobType=%08lx, dwFlags=%08lx, pbData=%p,"
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

    return crypt_export_key(pCryptKey, hPubKey, dwBlobType, dwFlags, FALSE,
        pbData, pdwDataLen);
}

/******************************************************************************
 * release_and_install_key [Internal]
 *
 * Release an existing key, if present, and replaces it with a new one.
 *
 * PARAMS
 *  hProv     [I] Key container into which the key is to be imported.
 *  src       [I] Key which will replace *dest
 *  dest      [I] Points to key to be released and replaced with src
 *  fStoreKey [I] If TRUE, the newly installed key is stored to the registry.
 */
static void release_and_install_key(HCRYPTPROV hProv, HCRYPTKEY src,
                                    HCRYPTKEY *dest, DWORD fStoreKey)
{
    RSAENH_CPDestroyKey(hProv, *dest);
    copy_handle(&handle_table, src, RSAENH_MAGIC_KEY, dest);
    if (fStoreKey)
    {
        KEYCONTAINER *pKeyContainer;

        if ((pKeyContainer = get_key_container(hProv)))
        {
            store_key_container_keys(pKeyContainer);
            store_key_container_permissions(pKeyContainer);
        }
    }
}

/******************************************************************************
 * import_private_key [Internal]
 *
 * Import a BLOB'ed private key into a key container.
 *
 * PARAMS
 *  hProv     [I] Key container into which the private key is to be imported.
 *  pbData    [I] Pointer to a buffer which holds the private key BLOB.
 *  dwDataLen [I] Length of data in buffer at pbData.
 *  dwFlags   [I] One of:
 *                CRYPT_EXPORTABLE: the imported key is marked exportable
 *  fStoreKey [I] If TRUE, the imported key is stored to the registry.
 *  phKey     [O] Handle to the imported key.
 *
 *
 * NOTES
 *  Assumes the caller has already checked the BLOBHEADER at pbData to ensure
 *  it's a PRIVATEKEYBLOB.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 */
static BOOL import_private_key(HCRYPTPROV hProv, const BYTE *pbData, DWORD dwDataLen,
                               DWORD dwFlags, BOOL fStoreKey, HCRYPTKEY *phKey)
{
    KEYCONTAINER *pKeyContainer;
    CRYPTKEY *pCryptKey;
    const BLOBHEADER *pBlobHeader = (const BLOBHEADER*)pbData;
    const RSAPUBKEY *pRSAPubKey = (const RSAPUBKEY*)(pBlobHeader+1);
    BOOL ret;

    if (dwFlags & CRYPT_IPSEC_HMAC_KEY)
    {
        FIXME("unimplemented for CRYPT_IPSEC_HMAC_KEY\n");
        SetLastError(NTE_BAD_FLAGS);
        return FALSE;
    }
    if (!(pKeyContainer = get_key_container(hProv)))
        return FALSE;

    if ((dwDataLen < sizeof(BLOBHEADER) + sizeof(RSAPUBKEY)))
    {
        ERR("datalen %ld not long enough for a BLOBHEADER + RSAPUBKEY\n",
            dwDataLen);
        SetLastError(NTE_BAD_DATA);
        return FALSE;
    }
    if (pRSAPubKey->magic != RSAENH_MAGIC_RSA2)
    {
        ERR("unexpected magic %08lx\n", pRSAPubKey->magic);
        SetLastError(NTE_BAD_DATA);
        return FALSE;
    }
    if ((dwDataLen < sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) +
            (pRSAPubKey->bitlen >> 3) + (5 * ((pRSAPubKey->bitlen+8)>>4))))
    {
        DWORD expectedLen = sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) +
            (pRSAPubKey->bitlen >> 3) + (5 * ((pRSAPubKey->bitlen+8)>>4));

        ERR("blob too short for pub key: expect %ld, got %ld\n",
            expectedLen, dwDataLen);
        SetLastError(NTE_BAD_DATA);
        return FALSE;
    }

    *phKey = new_key(hProv, pBlobHeader->aiKeyAlg, MAKELONG(0,pRSAPubKey->bitlen), &pCryptKey);
    if (*phKey == (HCRYPTKEY)INVALID_HANDLE_VALUE) return FALSE;
    setup_key(pCryptKey);
    ret = import_private_key_impl((const BYTE*)(pRSAPubKey+1), &pCryptKey->context,
                                   pRSAPubKey->bitlen/8, dwDataLen, pRSAPubKey->pubexp);
    if (ret) {
        if (dwFlags & CRYPT_EXPORTABLE)
            pCryptKey->dwPermissions |= CRYPT_EXPORT;
        switch (pBlobHeader->aiKeyAlg)
        {
        case AT_SIGNATURE:
        case CALG_RSA_SIGN:
            TRACE("installing signing key\n");
            release_and_install_key(hProv, *phKey, &pKeyContainer->hSignatureKeyPair,
                                    fStoreKey);
            break;
        case AT_KEYEXCHANGE:
        case CALG_RSA_KEYX:
            TRACE("installing key exchange key\n");
            release_and_install_key(hProv, *phKey, &pKeyContainer->hKeyExchangeKeyPair,
                                    fStoreKey);
            break;
        }
    }
    return ret;
}

/******************************************************************************
 * import_public_key [Internal]
 *
 * Import a BLOB'ed public key.
 *
 * PARAMS
 *  hProv     [I] A CSP.
 *  pbData    [I] Pointer to a buffer which holds the public key BLOB.
 *  dwDataLen [I] Length of data in buffer at pbData.
 *  dwFlags   [I] One of:
 *                CRYPT_EXPORTABLE: the imported key is marked exportable
 *  phKey     [O] Handle to the imported key.
 *
 *
 * NOTES
 *  Assumes the caller has already checked the BLOBHEADER at pbData to ensure
 *  it's a PUBLICKEYBLOB.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 */
static BOOL import_public_key(HCRYPTPROV hProv, const BYTE *pbData, DWORD dwDataLen,
                              DWORD dwFlags, HCRYPTKEY *phKey)
{
    CRYPTKEY *pCryptKey;
    const BLOBHEADER *pBlobHeader = (const BLOBHEADER*)pbData;
    const RSAPUBKEY *pRSAPubKey = (const RSAPUBKEY*)(pBlobHeader+1);
    ALG_ID algID;
    BOOL ret;

    if (dwFlags & CRYPT_IPSEC_HMAC_KEY)
    {
        FIXME("unimplemented for CRYPT_IPSEC_HMAC_KEY\n");
        SetLastError(NTE_BAD_FLAGS);
        return FALSE;
    }

    if ((dwDataLen < sizeof(BLOBHEADER) + sizeof(RSAPUBKEY)) ||
        (pRSAPubKey->magic != RSAENH_MAGIC_RSA1) ||
        (dwDataLen < sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) + (pRSAPubKey->bitlen >> 3)))
    {
        SetLastError(NTE_BAD_DATA);
        return FALSE;
    }

    /* Since this is a public key blob, only the public key is
     * available, so only signature verification is possible.
     */
    algID = pBlobHeader->aiKeyAlg;
    *phKey = new_key(hProv, algID, MAKELONG(0,pRSAPubKey->bitlen), &pCryptKey);
    if (*phKey == (HCRYPTKEY)INVALID_HANDLE_VALUE) return FALSE;
    setup_key(pCryptKey);
    ret = import_public_key_impl((const BYTE*)(pRSAPubKey+1), &pCryptKey->context,
                                  pRSAPubKey->bitlen >> 3, pRSAPubKey->pubexp);
    if (ret) {
        if (dwFlags & CRYPT_EXPORTABLE)
            pCryptKey->dwPermissions |= CRYPT_EXPORT;
    }
    return ret;
}

/******************************************************************************
 * import_symmetric_key [Internal]
 *
 * Import a BLOB'ed symmetric key into a key container.
 *
 * PARAMS
 *  hProv     [I] Key container into which the symmetric key is to be imported.
 *  pbData    [I] Pointer to a buffer which holds the symmetric key BLOB.
 *  dwDataLen [I] Length of data in buffer at pbData.
 *  hPubKey   [I] Key used to decrypt sensitive BLOB data.
 *  dwFlags   [I] One of:
 *                CRYPT_EXPORTABLE: the imported key is marked exportable
 *  phKey     [O] Handle to the imported key.
 *
 *
 * NOTES
 *  Assumes the caller has already checked the BLOBHEADER at pbData to ensure
 *  it's a SIMPLEBLOB.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 */
static BOOL import_symmetric_key(HCRYPTPROV hProv, const BYTE *pbData, DWORD dwDataLen,
                                 HCRYPTKEY hPubKey, DWORD dwFlags, HCRYPTKEY *phKey)
{
    CRYPTKEY *pCryptKey, *pPubKey;
    const BLOBHEADER *pBlobHeader = (const BLOBHEADER*)pbData;
    const ALG_ID *pAlgid = (const ALG_ID*)(pBlobHeader+1);
    const BYTE *pbKeyStream = (const BYTE*)(pAlgid + 1);
    BYTE *pbDecrypted;
    DWORD dwKeyLen;

    if (dwFlags & CRYPT_IPSEC_HMAC_KEY)
    {
        FIXME("unimplemented for CRYPT_IPSEC_HMAC_KEY\n");
        SetLastError(NTE_BAD_FLAGS);
        return FALSE;
    }
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

    pbDecrypted = malloc(pPubKey->dwBlockLen);
    if (!pbDecrypted) return FALSE;
    decrypt_block_impl(pPubKey->aiAlgid, PK_PRIVATE, &pPubKey->context, pbKeyStream, pbDecrypted);

    dwKeyLen = RSAENH_MAX_KEY_SIZE;
    if (!unpad_data(hProv, pbDecrypted, pPubKey->dwBlockLen, pbDecrypted, &dwKeyLen, dwFlags)) {
        free(pbDecrypted);
        return FALSE;
    }

    if (pBlobHeader->aiKeyAlg == CALG_RC2)
    {
        const PROV_ENUMALGS_EX *info;

        info = get_algid_info(hProv, CALG_RC2);
        assert(info);
        if (!dwKeyLen)
            dwKeyLen = info->dwDefaultLen;
        if (dwKeyLen < info->dwMinLen / 8 || dwKeyLen > 128)
        {
            WARN("Invalid RC2 key, len %ld.\n", dwKeyLen);
            *phKey = (HCRYPTKEY)INVALID_HANDLE_VALUE;
            SetLastError(NTE_BAD_DATA);
        }
        else if ((*phKey = alloc_key(hProv, pBlobHeader->aiKeyAlg, 0, dwKeyLen << 3, &pCryptKey))
                  != (HCRYPTKEY)INVALID_HANDLE_VALUE && info->dwDefaultLen == 40 && dwKeyLen > info->dwMaxLen / 8)
        {
            pCryptKey->dwEffectiveKeyLen = 40;
        }
    }
    else
    {
        *phKey = new_key(hProv, pBlobHeader->aiKeyAlg, dwKeyLen<<19, &pCryptKey);
    }
    if (*phKey == (HCRYPTKEY)INVALID_HANDLE_VALUE)
    {
        free(pbDecrypted);
        return FALSE;
    }
    memcpy(pCryptKey->abKeyValue, pbDecrypted, dwKeyLen);
    free(pbDecrypted);
    setup_key(pCryptKey);
    if (dwFlags & CRYPT_EXPORTABLE)
        pCryptKey->dwPermissions |= CRYPT_EXPORT;
    return TRUE;
}

/******************************************************************************
 * import_plaintext_key [Internal]
 *
 * Import a plaintext key into a key container.
 *
 * PARAMS
 *  hProv     [I] Key container into which the symmetric key is to be imported.
 *  pbData    [I] Pointer to a buffer which holds the plaintext key BLOB.
 *  dwDataLen [I] Length of data in buffer at pbData.
 *  dwFlags   [I] One of:
 *                CRYPT_EXPORTABLE: the imported key is marked exportable
 *  phKey     [O] Handle to the imported key.
 *
 *
 * NOTES
 *  Assumes the caller has already checked the BLOBHEADER at pbData to ensure
 *  it's a PLAINTEXTKEYBLOB.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 */
static BOOL import_plaintext_key(HCRYPTPROV hProv, const BYTE *pbData, DWORD dwDataLen,
                                 DWORD dwFlags, HCRYPTKEY *phKey)
{
    CRYPTKEY *pCryptKey;
    const BLOBHEADER *pBlobHeader = (const BLOBHEADER*)pbData;
    const DWORD *pKeyLen = (const DWORD *)(pBlobHeader + 1);
    const BYTE *pbKeyStream = (const BYTE*)(pKeyLen + 1);

    if (dwDataLen < sizeof(BLOBHEADER)+sizeof(DWORD)+*pKeyLen)
    {
        SetLastError(NTE_BAD_DATA); /* FIXME: error code */
        return FALSE;
    }

    if (dwFlags & CRYPT_IPSEC_HMAC_KEY)
    {
        *phKey = new_key(hProv, CALG_HMAC, 0, &pCryptKey);
        if (*phKey == (HCRYPTKEY)INVALID_HANDLE_VALUE)
            return FALSE;
        if (*pKeyLen <= RSAENH_MIN(sizeof(pCryptKey->abKeyValue), RSAENH_HMAC_BLOCK_LEN))
        {
            memcpy(pCryptKey->abKeyValue, pbKeyStream, *pKeyLen);
            pCryptKey->dwKeyLen = *pKeyLen;
        }
        else
        {
            CRYPT_DATA_BLOB blobHmacKey = { *pKeyLen, (BYTE *)pbKeyStream };

            /* In order to initialize an HMAC key, the key material is hashed,
             * and the output of the hash function is used as the key material.
             * Unfortunately, the way the Crypto API is designed, we don't know
             * the hash algorithm yet, so we have to copy the entire key
             * material.
             */
            if (!copy_data_blob(&pCryptKey->blobHmacKey, &blobHmacKey))
            {
                release_handle(&handle_table, *phKey, RSAENH_MAGIC_KEY);
                *phKey = (HCRYPTKEY)INVALID_HANDLE_VALUE;
                return FALSE;
            }
        }
        setup_key(pCryptKey);
        if (dwFlags & CRYPT_EXPORTABLE)
            pCryptKey->dwPermissions |= CRYPT_EXPORT;
    }
    else
    {
        *phKey = new_key(hProv, pBlobHeader->aiKeyAlg, *pKeyLen<<19, &pCryptKey);
        if (*phKey == (HCRYPTKEY)INVALID_HANDLE_VALUE)
            return FALSE;
        memcpy(pCryptKey->abKeyValue, pbKeyStream, *pKeyLen);
        setup_key(pCryptKey);
        if (dwFlags & CRYPT_EXPORTABLE)
            pCryptKey->dwPermissions |= CRYPT_EXPORT;
    }
    return TRUE;
}

/******************************************************************************
 * import_key [Internal]
 *
 * Import a BLOB'ed key into a key container, optionally storing the key's
 * value to the registry.
 *
 * PARAMS
 *  hProv     [I] Key container into which the key is to be imported.
 *  pbData    [I] Pointer to a buffer which holds the BLOB.
 *  dwDataLen [I] Length of data in buffer at pbData.
 *  hPubKey   [I] Key used to decrypt sensitive BLOB data.
 *  dwFlags   [I] One of:
 *                CRYPT_EXPORTABLE: the imported key is marked exportable
 *  fStoreKey [I] If TRUE, the imported key is stored to the registry.
 *  phKey     [O] Handle to the imported key.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 */
static BOOL import_key(HCRYPTPROV hProv, const BYTE *pbData, DWORD dwDataLen, HCRYPTKEY hPubKey,
                       DWORD dwFlags, BOOL fStoreKey, HCRYPTKEY *phKey)
{
    KEYCONTAINER *pKeyContainer;
    const BLOBHEADER *pBlobHeader = (const BLOBHEADER*)pbData;

    if (!(pKeyContainer = get_key_container(hProv)))
        return FALSE;

    if (dwDataLen < sizeof(BLOBHEADER) || 
        pBlobHeader->bVersion != CUR_BLOB_VERSION)
    {
        TRACE("bVersion = %d", pBlobHeader->bVersion);
        SetLastError(NTE_BAD_DATA);
        return FALSE;
    }
    if (pBlobHeader->reserved != 0)
        WARN("reserved != 0: %d\n", pBlobHeader->reserved);

    /* If this is a verify-only context, the key is not persisted regardless of
     * fStoreKey's original value.
     */
    fStoreKey = fStoreKey && !(dwFlags & CRYPT_VERIFYCONTEXT);
    TRACE("blob type: %x\n", pBlobHeader->bType);
    switch (pBlobHeader->bType)
    {
        case PRIVATEKEYBLOB:    
            return import_private_key(hProv, pbData, dwDataLen, dwFlags,
                                      fStoreKey, phKey);
                
        case PUBLICKEYBLOB:
            return import_public_key(hProv, pbData, dwDataLen, dwFlags,
                                     phKey);
                
        case SIMPLEBLOB:
            return import_symmetric_key(hProv, pbData, dwDataLen, hPubKey,
                                        dwFlags, phKey);

        case PLAINTEXTKEYBLOB:
            return import_plaintext_key(hProv, pbData, dwDataLen, dwFlags,
                                        phKey);

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
 *  dwFlags   [I] One of:
 *                CRYPT_EXPORTABLE: the imported key is marked exportable
 *  phKey     [O] Handle to the imported key.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 */
BOOL WINAPI RSAENH_CPImportKey(HCRYPTPROV hProv, const BYTE *pbData, DWORD dwDataLen,
                               HCRYPTKEY hPubKey, DWORD dwFlags, HCRYPTKEY *phKey)
{
    TRACE("(hProv=%08Ix, pbData=%p, dwDataLen=%ld, hPubKey=%08Ix, dwFlags=%08lx, phKey=%p)\n",
        hProv, pbData, dwDataLen, hPubKey, dwFlags, phKey);

    return import_key(hProv, pbData, dwDataLen, hPubKey, dwFlags, TRUE, phKey);
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
    CRYPTKEY *pCryptKey;

    TRACE("(hProv=%08Ix, aiAlgid=%d, dwFlags=%08lx, phKey=%p)\n", hProv, Algid, dwFlags, phKey);

    if (!(pKeyContainer = get_key_container(hProv)))
    {
        /* MSDN: hProv not containing valid context handle */
        return FALSE;
    }
    
    switch (Algid)
    {
        case AT_SIGNATURE:
        case CALG_RSA_SIGN:
            *phKey = new_key(hProv, CALG_RSA_SIGN, dwFlags, &pCryptKey);
            if (pCryptKey) { 
                new_key_impl(pCryptKey->aiAlgid, &pCryptKey->context, pCryptKey->dwKeyLen);
                setup_key(pCryptKey);
                release_and_install_key(hProv, *phKey,
                                        &pKeyContainer->hSignatureKeyPair,
                                        FALSE);
            }
            break;

        case AT_KEYEXCHANGE:
        case CALG_RSA_KEYX:
            *phKey = new_key(hProv, CALG_RSA_KEYX, dwFlags, &pCryptKey);
            if (pCryptKey) { 
                new_key_impl(pCryptKey->aiAlgid, &pCryptKey->context, pCryptKey->dwKeyLen);
                setup_key(pCryptKey);
                release_and_install_key(hProv, *phKey,
                                        &pKeyContainer->hKeyExchangeKeyPair,
                                        FALSE);
            }
            break;
            
        case CALG_RC2:
        case CALG_RC4:
        case CALG_DES:
        case CALG_3DES_112:
        case CALG_3DES:
        case CALG_AES_128:
        case CALG_AES_192:
        case CALG_AES_256:
        case CALG_PCT1_MASTER:
        case CALG_SSL2_MASTER:
        case CALG_SSL3_MASTER:
        case CALG_TLS1_MASTER:
            *phKey = new_key(hProv, Algid, dwFlags, &pCryptKey);
            if (pCryptKey) {
                RtlGenRandom(pCryptKey->abKeyValue, RSAENH_MAX_KEY_SIZE);
                switch (Algid) {
                    case CALG_SSL3_MASTER:
                        pCryptKey->abKeyValue[0] = RSAENH_SSL3_VERSION_MAJOR;
                        pCryptKey->abKeyValue[1] = RSAENH_SSL3_VERSION_MINOR;
                        break;

                    case CALG_TLS1_MASTER:
                        pCryptKey->abKeyValue[0] = RSAENH_TLS1_VERSION_MAJOR;
                        pCryptKey->abKeyValue[1] = RSAENH_TLS1_VERSION_MINOR;
                        break;
                    case CALG_RC4:
                        if (!(dwFlags & CRYPT_CREATE_SALT))
                            memset(pCryptKey->abKeyValue + pCryptKey->dwKeyLen, 0, pCryptKey->dwSaltLen);
                        break;
                }
                setup_key(pCryptKey);
            }
            break;
            
        default:
            /* MSDN: Algorithm not supported specified by Algid */
            SetLastError(NTE_BAD_ALGID);
            return FALSE;
    }
            
    return *phKey != (HCRYPTKEY)INVALID_HANDLE_VALUE;
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
    TRACE("(hProv=%08Ix, dwLen=%ld, pbBuffer=%p)\n", hProv, dwLen, pbBuffer);
    
    if (!is_valid_handle(&handle_table, hProv, RSAENH_MAGIC_CONTAINER))
    {
        /* MSDN: hProv not containing valid context handle */
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }

    return RtlGenRandom(pbBuffer, dwLen);
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
        
    TRACE("(hProv=%08Ix, hHash=%08Ix, dwParam=%08lx, pbData=%p, pdwDataLen=%p, dwFlags=%08lx)\n",
        hProv, hHash, dwParam, pbData, pdwDataLen, dwFlags);
    
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
    
    if (!lookup_handle(&handle_table, hHash, RSAENH_MAGIC_HASH,
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
            return copy_param(pbData, pdwDataLen, (const BYTE*)&pCryptHash->aiAlgid,
                              sizeof(ALG_ID));

        case HP_HASHSIZE:
            return copy_param(pbData, pdwDataLen, (const BYTE*)&pCryptHash->dwHashSize,
                              sizeof(DWORD));

        case HP_HASHVAL:
            if (pCryptHash->aiAlgid == CALG_TLS1PRF) {
                return tls1_prf(hProv, pCryptHash->hKey, &pCryptHash->tpPRFParams.blobLabel,
                                &pCryptHash->tpPRFParams.blobSeed, pbData, *pdwDataLen);
            }

            if (pCryptHash->dwState != RSAENH_HASHSTATE_FINISHED)
            {
                finalize_hash(pCryptHash);
                pCryptHash->dwState = RSAENH_HASHSTATE_FINISHED;
            }

            if (!pbData)
            {
                *pdwDataLen = pCryptHash->dwHashSize;
                return TRUE;
            }

            return copy_param(pbData, pdwDataLen, pCryptHash->abHashValue,
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

    TRACE("(hProv=%08Ix, hKey=%08Ix, dwParam=%08lx, pbData=%p, dwFlags=%08lx)\n", hProv, hKey,
          dwParam, pbData, dwFlags);

    if (!is_valid_handle(&handle_table, hProv, RSAENH_MAGIC_CONTAINER))
    {
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }

    if (dwFlags) {
        SetLastError(NTE_BAD_FLAGS);
        return FALSE;
    }

    if (!lookup_handle(&handle_table, hKey, RSAENH_MAGIC_KEY, (OBJECTHDR**)&pCryptKey))
    {
        SetLastError(NTE_BAD_KEY);
        return FALSE;
    }

    if (!pbData)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    switch (dwParam) {
        case KP_PADDING:
            /* The MS providers only support PKCS5_PADDING */
            if (*(DWORD *)pbData != PKCS5_PADDING) {
                SetLastError(NTE_BAD_DATA);
                return FALSE;
            }
            return TRUE;

        case KP_MODE:
            pCryptKey->dwMode = *(DWORD*)pbData;
            return TRUE;

        case KP_MODE_BITS:
            pCryptKey->dwModeBits = *(DWORD*)pbData;
            return TRUE;

        case KP_PERMISSIONS:
        {
            DWORD perms = *(DWORD *)pbData;

            if ((perms & CRYPT_EXPORT) &&
                !(pCryptKey->dwPermissions & CRYPT_EXPORT))
            {
                SetLastError(NTE_BAD_DATA);
                return FALSE;
            }
            else if (!(perms & CRYPT_EXPORT) &&
                (pCryptKey->dwPermissions & CRYPT_EXPORT))
            {
                /* Clearing the export permission appears to be ignored,
                 * see tests.
                 */
                perms |= CRYPT_EXPORT;
            }
            pCryptKey->dwPermissions = perms;
            return TRUE;
        }

        case KP_IV:
            memcpy(pCryptKey->abInitVector, pbData, pCryptKey->dwBlockLen);
            setup_key(pCryptKey);
            return TRUE;

        case KP_SALT:
            switch (pCryptKey->aiAlgid) {
                case CALG_RC2:
                case CALG_RC4:
                {
                    KEYCONTAINER *pKeyContainer = get_key_container(pCryptKey->hProv);
                    if (!pbData)
                    {
                        SetLastError(ERROR_INVALID_PARAMETER);
                        return FALSE;
                    }
                    /* MSDN: the base provider always sets eleven bytes of
                     * salt value.
                     */
                    memcpy(pCryptKey->abKeyValue + pCryptKey->dwKeyLen,
                           pbData, 11);
                    pCryptKey->dwSaltLen = 11;
                    setup_key(pCryptKey);
                    /* After setting the salt value if the provider is not base or
                     * strong the salt length will be reset. */
                    if (pKeyContainer->dwPersonality != RSAENH_PERSONALITY_BASE &&
                        pKeyContainer->dwPersonality != RSAENH_PERSONALITY_STRONG)
                        pCryptKey->dwSaltLen = 0;
                    break;
                }
                default:
                    SetLastError(NTE_BAD_KEY);
                    return FALSE;
            }
            return TRUE;

        case KP_SALT_EX:
        {
            CRYPT_INTEGER_BLOB *blob = (CRYPT_INTEGER_BLOB *)pbData;

            /* salt length can't be greater than 184 bits = 24 bytes */
            if (blob->cbData > 24)
            {
                SetLastError(NTE_BAD_DATA);
                return FALSE;
            }
            memcpy(pCryptKey->abKeyValue + pCryptKey->dwKeyLen, blob->pbData,
                   blob->cbData);
            pCryptKey->dwSaltLen = blob->cbData;
            setup_key(pCryptKey);
            return TRUE;
        }

        case KP_EFFECTIVE_KEYLEN:
            switch (pCryptKey->aiAlgid) {
                case CALG_RC2:
                {
                    DWORD keylen, deflen;
                    BOOL ret = TRUE;
                    KEYCONTAINER *pKeyContainer = get_key_container(pCryptKey->hProv);

                    if (!pbData)
                    {
                        SetLastError(ERROR_INVALID_PARAMETER);
                        return FALSE;
                    }
                    keylen = *(DWORD *)pbData;
                    if (!keylen || keylen > 1024)
                    {
                        SetLastError(NTE_BAD_DATA);
                        return FALSE;
                    }

                    /*
                     * The Base provider will force the key length to default
                     * and set an error state if a key length different from
                     * the default is tried.
                     */
                    deflen = aProvEnumAlgsEx[pKeyContainer->dwPersonality]->dwDefaultLen;
                    if (pKeyContainer->dwPersonality == RSAENH_PERSONALITY_BASE
                        && keylen != deflen)
                    {
                        keylen = deflen;
                        SetLastError(NTE_BAD_DATA);
                        ret = FALSE;
                    }
                    pCryptKey->dwEffectiveKeyLen = keylen;
                    setup_key(pCryptKey);
                    return ret;
                }
                default:
                    SetLastError(NTE_BAD_TYPE);
                    return FALSE;
            }
            return TRUE;

        case KP_SCHANNEL_ALG:
            switch (((PSCHANNEL_ALG)pbData)->dwUse) {
                case SCHANNEL_ENC_KEY:
                    memcpy(&pCryptKey->siSChannelInfo.saEncAlg, pbData, sizeof(SCHANNEL_ALG));
                    break;

                case SCHANNEL_MAC_KEY:
                    memcpy(&pCryptKey->siSChannelInfo.saMACAlg, pbData, sizeof(SCHANNEL_ALG));
                    break;

                default:
                    SetLastError(NTE_FAIL); /* FIXME: error code */
                    return FALSE;
            }
            return TRUE;

        case KP_CLIENT_RANDOM:
            return copy_data_blob(&pCryptKey->siSChannelInfo.blobClientRandom, (PCRYPT_DATA_BLOB)pbData);
            
        case KP_SERVER_RANDOM:
            return copy_data_blob(&pCryptKey->siSChannelInfo.blobServerRandom, (PCRYPT_DATA_BLOB)pbData);

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
    DWORD dwValue;
        
    TRACE("(hProv=%08Ix, hKey=%08Ix, dwParam=%08lx, pbData=%p, pdwDataLen=%p dwFlags=%08lx)\n",
          hProv, hKey, dwParam, pbData, pdwDataLen, dwFlags);

    if (!is_valid_handle(&handle_table, hProv, RSAENH_MAGIC_CONTAINER))
    {
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }

    if (dwFlags) {
        SetLastError(NTE_BAD_FLAGS);
        return FALSE;
    }

    if (!lookup_handle(&handle_table, hKey, RSAENH_MAGIC_KEY, (OBJECTHDR**)&pCryptKey))
    {
        SetLastError(NTE_BAD_KEY);
        return FALSE;
    }

    switch (dwParam) 
    {
        case KP_IV:
            return copy_param(pbData, pdwDataLen, pCryptKey->abInitVector,
                              pCryptKey->dwBlockLen);
        
        case KP_SALT:
            switch (pCryptKey->aiAlgid) {
                case CALG_RC2:
                case CALG_RC4:
                    return copy_param(pbData, pdwDataLen,
                            &pCryptKey->abKeyValue[pCryptKey->dwKeyLen],
                            pCryptKey->dwSaltLen);
                default:
                    SetLastError(NTE_BAD_KEY);
                    return FALSE;
            }

        case KP_PADDING:
            dwValue = PKCS5_PADDING;
            return copy_param(pbData, pdwDataLen, (const BYTE*)&dwValue, sizeof(DWORD));

        case KP_KEYLEN:
            dwValue = pCryptKey->dwKeyLen << 3;
            return copy_param(pbData, pdwDataLen, (const BYTE*)&dwValue, sizeof(DWORD));

        case KP_EFFECTIVE_KEYLEN:
            if (pCryptKey->dwEffectiveKeyLen)
                dwValue = pCryptKey->dwEffectiveKeyLen;
            else
                dwValue = pCryptKey->dwKeyLen << 3;
            return copy_param(pbData, pdwDataLen, (const BYTE*)&dwValue, sizeof(DWORD));

        case KP_BLOCKLEN:
            dwValue = pCryptKey->dwBlockLen << 3;
            return copy_param(pbData, pdwDataLen, (const BYTE*)&dwValue, sizeof(DWORD));

        case KP_MODE:
            return copy_param(pbData, pdwDataLen, (const BYTE*)&pCryptKey->dwMode, sizeof(DWORD));

        case KP_MODE_BITS:
            return copy_param(pbData, pdwDataLen, (const BYTE*)&pCryptKey->dwModeBits,
                              sizeof(DWORD));

        case KP_PERMISSIONS:
            return copy_param(pbData, pdwDataLen, (const BYTE*)&pCryptKey->dwPermissions,
                              sizeof(DWORD));

        case KP_ALGID:
            return copy_param(pbData, pdwDataLen, (const BYTE*)&pCryptKey->aiAlgid, sizeof(DWORD));

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
 *   - PP_KEYSET_SEC_DESCR: Retrieve security descriptor on container.
 */
BOOL WINAPI RSAENH_CPGetProvParam(HCRYPTPROV hProv, DWORD dwParam, BYTE *pbData, 
                                  DWORD *pdwDataLen, DWORD dwFlags)
{
    KEYCONTAINER *pKeyContainer;
    PROV_ENUMALGS provEnumalgs;
    DWORD dwTemp;
    HKEY hKey;
   
    /* This is for dwParam PP_CRYPT_COUNT_KEY_USE.
     * IE6 SP1 asks for it in the 'About' dialog.
     * Returning this BLOB seems to satisfy IE. The marked 0x00 seem 
     * to be 'don't care's. If you know anything more specific about
     * this provider parameter, please contact the Wine developers */
    static const BYTE abWTF[96] = {
        0xb0, 0x25,     0x63,     0x86, 0x9c, 0xab,     0xb6,     0x37, 
        0xe8, 0x82, /**/0x00,/**/ 0x72, 0x06, 0xb2, /**/0x00,/**/ 0x3b, 
        0x60, 0x35, /**/0x00,/**/ 0x3b, 0x88, 0xce, /**/0x00,/**/ 0x82, 
        0xbc, 0x7a, /**/0x00,/**/ 0xb7, 0x4f, 0x7e, /**/0x00,/**/ 0xde, 
        0x92, 0xf1, /**/0x00,/**/ 0x83, 0xea, 0x5e, /**/0x00,/**/ 0xc8, 
        0x12, 0x1e,     0xd4,     0x06, 0xf7, 0x66, /**/0x00,/**/ 0x01, 
        0x29, 0xa4, /**/0x00,/**/ 0xf8, 0x24, 0x0c, /**/0x00,/**/ 0x33, 
        0x06, 0x80, /**/0x00,/**/ 0x02, 0x46, 0x0b, /**/0x00,/**/ 0x6d, 
        0x5b, 0xca, /**/0x00,/**/ 0x9a, 0x10, 0xf0, /**/0x00,/**/ 0x05, 
        0x19, 0xd0, /**/0x00,/**/ 0x2c, 0xf6, 0x27, /**/0x00,/**/ 0xaa, 
        0x7c, 0x6f, /**/0x00,/**/ 0xb9, 0xd8, 0x72, /**/0x00,/**/ 0x03, 
        0xf3, 0x81, /**/0x00,/**/ 0xfa, 0xe8, 0x26, /**/0x00,/**/ 0xca 
    };

    TRACE("(hProv=%08Ix, dwParam=%08lx, pbData=%p, pdwDataLen=%p, dwFlags=%08lx)\n",
           hProv, dwParam, pbData, pdwDataLen, dwFlags);

    if (!pdwDataLen) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    if (!(pKeyContainer = get_key_container(hProv)))
    {
        /* MSDN: hProv not containing valid context handle */
        return FALSE;
    }

    switch (dwParam) 
    {
        case PP_CONTAINER:
        case PP_UNIQUE_CONTAINER:/* MSDN says we can return the same value as PP_CONTAINER */
            return copy_param(pbData, pdwDataLen, (const BYTE*)pKeyContainer->szName,
                              strlen(pKeyContainer->szName)+1);

        case PP_NAME:
            return copy_param(pbData, pdwDataLen, (const BYTE*)pKeyContainer->szProvName,
                              strlen(pKeyContainer->szProvName)+1);

        case PP_PROVTYPE:
            dwTemp = PROV_RSA_FULL;
            return copy_param(pbData, pdwDataLen, (const BYTE*)&dwTemp, sizeof(dwTemp));

        case PP_KEYSPEC:
            dwTemp = AT_SIGNATURE | AT_KEYEXCHANGE;
            return copy_param(pbData, pdwDataLen, (const BYTE*)&dwTemp, sizeof(dwTemp));

        case PP_KEYSET_TYPE:
            dwTemp = pKeyContainer->dwFlags & CRYPT_MACHINE_KEYSET;
            return copy_param(pbData, pdwDataLen, (const BYTE*)&dwTemp, sizeof(dwTemp));

        case PP_KEYSTORAGE:
            dwTemp = CRYPT_SEC_DESCR;
            return copy_param(pbData, pdwDataLen, (const BYTE*)&dwTemp, sizeof(dwTemp));

        case PP_SIG_KEYSIZE_INC:
        case PP_KEYX_KEYSIZE_INC:
            dwTemp = 8;
            return copy_param(pbData, pdwDataLen, (const BYTE*)&dwTemp, sizeof(dwTemp));

        case PP_IMPTYPE:
            dwTemp = CRYPT_IMPL_SOFTWARE;
            return copy_param(pbData, pdwDataLen, (const BYTE*)&dwTemp, sizeof(dwTemp));

        case PP_VERSION:
            dwTemp = 0x00000200;
            return copy_param(pbData, pdwDataLen, (const BYTE*)&dwTemp, sizeof(dwTemp));

        case PP_ENUMCONTAINERS:
            if ((dwFlags & CRYPT_FIRST) == CRYPT_FIRST) pKeyContainer->dwEnumContainersCtr = 0;

            if (!pbData) {
                *pdwDataLen = (DWORD)MAX_PATH + 1;
                return TRUE;
            }
 
            if (!open_container_key("", dwFlags, KEY_READ, &hKey))
            {
                SetLastError(ERROR_NO_MORE_ITEMS);
                return FALSE;
            }

            dwTemp = *pdwDataLen;
            switch (RegEnumKeyExA(hKey, pKeyContainer->dwEnumContainersCtr, (LPSTR)pbData, &dwTemp,
                    NULL, NULL, NULL, NULL))
            {
                case ERROR_MORE_DATA:
                    *pdwDataLen = (DWORD)MAX_PATH + 1;
 
                case ERROR_SUCCESS:
                    pKeyContainer->dwEnumContainersCtr++;
                    RegCloseKey(hKey);
                    return TRUE;

                case ERROR_NO_MORE_ITEMS:
                default:
                    SetLastError(ERROR_NO_MORE_ITEMS);
                    RegCloseKey(hKey);
                    return FALSE;
            }
 
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

                return copy_param(pbData, pdwDataLen, (const BYTE*)&provEnumalgs,
                                  sizeof(PROV_ENUMALGS));
            } else {
                if (pbData && (*pdwDataLen >= sizeof(PROV_ENUMALGS_EX))) 
                    pKeyContainer->dwEnumAlgsCtr = ((dwFlags & CRYPT_FIRST) == CRYPT_FIRST) ? 
                        0 : pKeyContainer->dwEnumAlgsCtr+1;

                return copy_param(pbData, pdwDataLen, 
                                  (const BYTE*)&aProvEnumAlgsEx
                                      [pKeyContainer->dwPersonality][pKeyContainer->dwEnumAlgsCtr], 
                                  sizeof(PROV_ENUMALGS_EX));
            }

        case PP_CRYPT_COUNT_KEY_USE: /* Asked for by IE About dialog */
            return copy_param(pbData, pdwDataLen, abWTF, sizeof(abWTF));

        case PP_KEYSET_SEC_DESCR:
        {
            SECURITY_DESCRIPTOR *sd;
            DWORD err, len, flags = (pKeyContainer->dwFlags & CRYPT_MACHINE_KEYSET);

            if (!open_container_key(pKeyContainer->szName, flags, KEY_READ, &hKey))
            {
                SetLastError(NTE_BAD_KEYSET);
                return FALSE;
            }

            err = GetSecurityInfo(hKey, SE_REGISTRY_KEY, dwFlags, NULL, NULL, NULL, NULL, (void **)&sd);
            RegCloseKey(hKey);
            if (err)
            {
                SetLastError(err);
                return FALSE;
            }

            len = GetSecurityDescriptorLength(sd);
            if (*pdwDataLen >= len) memcpy(pbData, sd, len);
            else SetLastError(ERROR_INSUFFICIENT_BUFFER);
            *pdwDataLen = len;

            LocalFree(sd);
            return TRUE;
        }

        default:
            /* MSDN: Unknown parameter number in dwParam */
            SetLastError(NTE_BAD_TYPE);
            return FALSE;
    }
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
    CRYPTKEY *pCryptKey, *pMasterKey;
    CRYPTHASH *pCryptHash;
    BYTE abHashValue[RSAENH_MAX_HASH_SIZE*2];
    DWORD dwLen;
    
    TRACE("(hProv=%08Ix, Algid=%d, hBaseData=%08Ix, dwFlags=%08lx phKey=%p)\n", hProv, Algid,
           hBaseData, dwFlags, phKey);
    
    if (!is_valid_handle(&handle_table, hProv, RSAENH_MAGIC_CONTAINER))
    {
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }

    if (!lookup_handle(&handle_table, hBaseData, RSAENH_MAGIC_HASH,
                       (OBJECTHDR**)&pCryptHash))
    {
        SetLastError(NTE_BAD_HASH);
        return FALSE;
    }

    if (!phKey)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    switch (GET_ALG_CLASS(Algid))
    {
        case ALG_CLASS_DATA_ENCRYPT:
        {
            int need_padding, copy_len;
            *phKey = new_key(hProv, Algid, dwFlags, &pCryptKey);
            if (*phKey == (HCRYPTKEY)INVALID_HANDLE_VALUE) return FALSE;

            /* 
             * We derive the key material from the hash.
             * If the hash value is not large enough for the claimed key, we have to construct
             * a larger binary value based on the hash. This is documented in MSDN: CryptDeriveKey.
             */
            dwLen = RSAENH_MAX_HASH_SIZE;
            RSAENH_CPGetHashParam(pCryptHash->hProv, hBaseData, HP_HASHVAL, abHashValue, &dwLen, 0);

            /*
             * The usage of padding seems to vary from algorithm to algorithm.
             * For now the only different case found was for AES with 128 bit key.
             */
            switch(Algid)
            {
                case CALG_AES_128:
                    /* To reduce the chance of regressions we will only deviate
                     * from the old behavior for the tested hash lengths */
                    if (dwLen == 16 || dwLen == 20)
                    {
                        need_padding = 1;
                        break;
                    }
                default:
                    need_padding = dwLen < pCryptKey->dwKeyLen;
            }

            copy_len = pCryptKey->dwKeyLen;
            if (need_padding)
            {
                BYTE pad1[RSAENH_HMAC_DEF_PAD_LEN], pad2[RSAENH_HMAC_DEF_PAD_LEN];
                BYTE old_hashval[RSAENH_MAX_HASH_SIZE];
                DWORD i;

                memcpy(old_hashval, pCryptHash->abHashValue, RSAENH_MAX_HASH_SIZE);
            
                for (i=0; i<RSAENH_HMAC_DEF_PAD_LEN; i++) {
                    pad1[i] = RSAENH_HMAC_DEF_IPAD_CHAR ^ (i<dwLen ? abHashValue[i] : 0);
                    pad2[i] = RSAENH_HMAC_DEF_OPAD_CHAR ^ (i<dwLen ? abHashValue[i] : 0);
                }
                
                init_hash(pCryptHash);
                update_hash(pCryptHash, pad1, RSAENH_HMAC_DEF_PAD_LEN);
                finalize_hash(pCryptHash);
                memcpy(abHashValue, pCryptHash->abHashValue, pCryptHash->dwHashSize);

                init_hash(pCryptHash);
                update_hash(pCryptHash, pad2, RSAENH_HMAC_DEF_PAD_LEN);
                finalize_hash(pCryptHash);
                memcpy(abHashValue+pCryptHash->dwHashSize, pCryptHash->abHashValue, 
                       pCryptHash->dwHashSize);

                memcpy(pCryptHash->abHashValue, old_hashval, RSAENH_MAX_HASH_SIZE);
            }
            /*
             * Padding was not required, we have more hash than needed.
             * Do we need to use the remaining hash as salt?
             */
            else if((dwFlags & CRYPT_CREATE_SALT) &&
                    (Algid == CALG_RC2 || Algid == CALG_RC4))
            {
                copy_len += pCryptKey->dwSaltLen;
            }
    
            memcpy(pCryptKey->abKeyValue, abHashValue, 
                   RSAENH_MIN(copy_len, sizeof(pCryptKey->abKeyValue)));
            break;
        }
        case ALG_CLASS_MSG_ENCRYPT:
            if (!lookup_handle(&handle_table, pCryptHash->hKey, RSAENH_MAGIC_KEY,
                               (OBJECTHDR**)&pMasterKey)) 
            {
                SetLastError(NTE_FAIL); /* FIXME error code */
                return FALSE;
            }
                
            switch (Algid) 
            {
                /* See RFC 2246, chapter 6.3 Key calculation */
                case CALG_SCHANNEL_ENC_KEY:
                    if (!pMasterKey->siSChannelInfo.saEncAlg.Algid ||
                        !pMasterKey->siSChannelInfo.saEncAlg.cBits)
                    {
                        SetLastError(NTE_BAD_FLAGS);
                        return FALSE;
                    }
                    *phKey = new_key(hProv, pMasterKey->siSChannelInfo.saEncAlg.Algid, 
                                     MAKELONG(LOWORD(dwFlags),pMasterKey->siSChannelInfo.saEncAlg.cBits),
                                     &pCryptKey);
                    if (*phKey == (HCRYPTKEY)INVALID_HANDLE_VALUE) return FALSE;
                    memcpy(pCryptKey->abKeyValue, 
                           pCryptHash->abHashValue + (
                               2 * (pMasterKey->siSChannelInfo.saMACAlg.cBits / 8) +
                               ((dwFlags & CRYPT_SERVER) ? 
                                   (pMasterKey->siSChannelInfo.saEncAlg.cBits / 8) : 0)),
                           pMasterKey->siSChannelInfo.saEncAlg.cBits / 8);
                    memcpy(pCryptKey->abInitVector,
                           pCryptHash->abHashValue + (
                               2 * (pMasterKey->siSChannelInfo.saMACAlg.cBits / 8) +
                               2 * (pMasterKey->siSChannelInfo.saEncAlg.cBits / 8) +
                               ((dwFlags & CRYPT_SERVER) ? pCryptKey->dwBlockLen : 0)),
                           pCryptKey->dwBlockLen);
                    break;
                    
                case CALG_SCHANNEL_MAC_KEY:
                    *phKey = new_key(hProv, Algid, 
                                     MAKELONG(LOWORD(dwFlags),pMasterKey->siSChannelInfo.saMACAlg.cBits),
                                     &pCryptKey);
                    if (*phKey == (HCRYPTKEY)INVALID_HANDLE_VALUE) return FALSE;
                    memcpy(pCryptKey->abKeyValue,
                           pCryptHash->abHashValue + ((dwFlags & CRYPT_SERVER) ? 
                               pMasterKey->siSChannelInfo.saMACAlg.cBits / 8 : 0),
                           pMasterKey->siSChannelInfo.saMACAlg.cBits / 8);
                    break;
                    
                default:
                    SetLastError(NTE_BAD_ALGID);
                    return FALSE;
            }
            break;

        default:
            SetLastError(NTE_BAD_ALGID);
            return FALSE;
    }

    setup_key(pCryptKey);
    return TRUE;    
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

    TRACE("(hProv=%08Ix, dwKeySpec=%08lx, phUserKey=%p)\n", hProv, dwKeySpec, phUserKey);
    
    if (!(pKeyContainer = get_key_container(hProv)))
    {
        /* MSDN: hProv not containing valid context handle */
        return FALSE;
    }

    switch (dwKeySpec)
    {
        case AT_KEYEXCHANGE:
            copy_handle(&handle_table, pKeyContainer->hKeyExchangeKeyPair, RSAENH_MAGIC_KEY, 
                        phUserKey);
            break;

        case AT_SIGNATURE:
            copy_handle(&handle_table, pKeyContainer->hSignatureKeyPair, RSAENH_MAGIC_KEY, 
                        phUserKey);
            break;

        default:
            *phUserKey = (HCRYPTKEY)INVALID_HANDLE_VALUE;
    }

    if (*phUserKey == (HCRYPTKEY)INVALID_HANDLE_VALUE)
    {
        /* MSDN: dwKeySpec parameter specifies nonexistent key */
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
BOOL WINAPI RSAENH_CPHashData(HCRYPTPROV hProv, HCRYPTHASH hHash, const BYTE *pbData,
                              DWORD dwDataLen, DWORD dwFlags)
{
    CRYPTHASH *pCryptHash;
        
    TRACE("(hProv=%08Ix, hHash=%08Ix, pbData=%p, dwDataLen=%ld, dwFlags=%08lx)\n",
          hProv, hHash, pbData, dwDataLen, dwFlags);

    if (dwFlags & ~CRYPT_USERDATA)
    {
        SetLastError(NTE_BAD_FLAGS);
        return FALSE;
    }

    if (!lookup_handle(&handle_table, hHash, RSAENH_MAGIC_HASH,
                       (OBJECTHDR**)&pCryptHash))
    {
        SetLastError(NTE_BAD_HASH);
        return FALSE;
    }

    if (!get_algid_info(hProv, pCryptHash->aiAlgid) || pCryptHash->aiAlgid == CALG_SSL3_SHAMD5)
    {
        SetLastError(NTE_BAD_ALGID);
        return FALSE;
    }
    
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
 *
 * Updates a hash object with the binary representation of a symmetric key.
 *
 * PARAMS
 *  hProv     [I] Key container to which the hash object belongs.
 *  hHash     [I] Hash object which is to be updated.
 *  hKey      [I] The symmetric key, whose binary value will be added to the hash.
 *  dwFlags   [I] CRYPT_LITTLE_ENDIAN, if the binary key value shall be interpreted as little endian.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 */
BOOL WINAPI RSAENH_CPHashSessionKey(HCRYPTPROV hProv, HCRYPTHASH hHash, HCRYPTKEY hKey, 
                                    DWORD dwFlags)
{
    BYTE abKeyValue[RSAENH_MAX_KEY_SIZE], bTemp;
    CRYPTKEY *pKey;
    DWORD i;

    TRACE("(hProv=%08Ix, hHash=%08Ix, hKey=%08Ix, dwFlags=%08lx)\n", hProv, hHash, hKey, dwFlags);

    if (!lookup_handle(&handle_table, hKey, RSAENH_MAGIC_KEY, (OBJECTHDR**)&pKey) ||
        (GET_ALG_CLASS(pKey->aiAlgid) != ALG_CLASS_DATA_ENCRYPT)) 
    {
        SetLastError(NTE_BAD_KEY);
        return FALSE;
    }

    if (dwFlags & ~CRYPT_LITTLE_ENDIAN) {
        SetLastError(NTE_BAD_FLAGS);
        return FALSE;
    }

    memcpy(abKeyValue, pKey->abKeyValue, pKey->dwKeyLen);
    if (!(dwFlags & CRYPT_LITTLE_ENDIAN)) {
        for (i=0; i<pKey->dwKeyLen/2; i++) {
            bTemp = abKeyValue[i];
            abKeyValue[i] = abKeyValue[pKey->dwKeyLen-i-1];
            abKeyValue[pKey->dwKeyLen-i-1] = bTemp;
        }
    }

    return RSAENH_CPHashData(hProv, hHash, abKeyValue, pKey->dwKeyLen, 0);
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
    TRACE("(hProv=%08Ix, dwFlags=%08lx)\n", hProv, dwFlags);

    if (!release_handle(&handle_table, hProv, RSAENH_MAGIC_CONTAINER))
    {
        /* MSDN: hProv not containing valid context handle */
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }

    if (dwFlags) {
        SetLastError(NTE_BAD_FLAGS);
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
 * NOTES
 *  Currently only the HP_HMAC_INFO dwParam type is defined. 
 *  The HMAC_INFO struct will be deep copied into the hash object.
 *  See Internet RFC 2104 for details on the HMAC algorithm.
 */
BOOL WINAPI RSAENH_CPSetHashParam(HCRYPTPROV hProv, HCRYPTHASH hHash, DWORD dwParam, 
                                  BYTE *pbData, DWORD dwFlags)
{
    CRYPTHASH *pCryptHash;
    CRYPTKEY *pCryptKey;
    DWORD i;

    TRACE("(hProv=%08Ix, hHash=%08Ix, dwParam=%08lx, pbData=%p, dwFlags=%08lx)\n",
           hProv, hHash, dwParam, pbData, dwFlags);

    if (!is_valid_handle(&handle_table, hProv, RSAENH_MAGIC_CONTAINER))
    {
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }

    if (dwFlags) {
        SetLastError(NTE_BAD_FLAGS);
        return FALSE;
    }
    
    if (!lookup_handle(&handle_table, hHash, RSAENH_MAGIC_HASH,
                       (OBJECTHDR**)&pCryptHash))
    {
        SetLastError(NTE_BAD_HASH);
        return FALSE;
    }
    
    switch (dwParam) {
        case HP_HMAC_INFO:
            free_hmac_info(pCryptHash->pHMACInfo);
            if (!copy_hmac_info(&pCryptHash->pHMACInfo, (PHMAC_INFO)pbData)) return FALSE;

            if (!lookup_handle(&handle_table, pCryptHash->hKey, RSAENH_MAGIC_KEY, 
                               (OBJECTHDR**)&pCryptKey)) 
            {
                SetLastError(NTE_FAIL); /* FIXME: correct error code? */
                return FALSE;
            }

            if (pCryptKey->aiAlgid == CALG_HMAC && !pCryptKey->dwKeyLen) {
                HCRYPTHASH hKeyHash;
                DWORD keyLen;

                if (!RSAENH_CPCreateHash(hProv, ((PHMAC_INFO)pbData)->HashAlgid, 0, 0,
                    &hKeyHash))
                    return FALSE;
                if (!RSAENH_CPHashData(hProv, hKeyHash, pCryptKey->blobHmacKey.pbData,
                    pCryptKey->blobHmacKey.cbData, 0))
                {
                    RSAENH_CPDestroyHash(hProv, hKeyHash);
                    return FALSE;
                }
                keyLen = sizeof(pCryptKey->abKeyValue);
                if (!RSAENH_CPGetHashParam(hProv, hKeyHash, HP_HASHVAL, pCryptKey->abKeyValue,
                    &keyLen, 0))
                {
                    RSAENH_CPDestroyHash(hProv, hKeyHash);
                    return FALSE;
                }
                pCryptKey->dwKeyLen = keyLen;
                RSAENH_CPDestroyHash(hProv, hKeyHash);
            }
            for (i=0; i<RSAENH_MIN(pCryptKey->dwKeyLen,pCryptHash->pHMACInfo->cbInnerString); i++) {
                pCryptHash->pHMACInfo->pbInnerString[i] ^= pCryptKey->abKeyValue[i];
            }
            for (i=0; i<RSAENH_MIN(pCryptKey->dwKeyLen,pCryptHash->pHMACInfo->cbOuterString); i++) {
                pCryptHash->pHMACInfo->pbOuterString[i] ^= pCryptKey->abKeyValue[i];
            }
            
            init_hash(pCryptHash);
            return TRUE;

        case HP_HASHVAL:
            memcpy(pCryptHash->abHashValue, pbData, pCryptHash->dwHashSize);
            pCryptHash->dwState = RSAENH_HASHSTATE_FINISHED;
            return TRUE;
           
        case HP_TLS1PRF_SEED:
            return copy_data_blob(&pCryptHash->tpPRFParams.blobSeed, (PCRYPT_DATA_BLOB)pbData);

        case HP_TLS1PRF_LABEL:
            return copy_data_blob(&pCryptHash->tpPRFParams.blobLabel, (PCRYPT_DATA_BLOB)pbData);
            
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
    KEYCONTAINER *pKeyContainer;
    HKEY hKey;

    TRACE("(hProv=%08Ix, dwParam=%08lx, pbData=%p, dwFlags=%08lx)\n", hProv, dwParam, pbData, dwFlags);

    if (!(pKeyContainer = get_key_container(hProv)))
        return FALSE;

    switch (dwParam)
    {
    case PP_KEYSET_SEC_DESCR:
    {
        SECURITY_DESCRIPTOR *sd = (SECURITY_DESCRIPTOR *)pbData;
        DWORD err, flags = (pKeyContainer->dwFlags & CRYPT_MACHINE_KEYSET);
        BOOL def, present;
        REGSAM access = WRITE_DAC | WRITE_OWNER | ACCESS_SYSTEM_SECURITY;
        PSID owner = NULL, group = NULL;
        PACL dacl = NULL, sacl = NULL;

        if (!open_container_key(pKeyContainer->szName, flags, access, &hKey))
        {
            SetLastError(NTE_BAD_KEYSET);
            return FALSE;
        }

        if ((dwFlags & OWNER_SECURITY_INFORMATION && !GetSecurityDescriptorOwner(sd, &owner, &def)) ||
            (dwFlags & GROUP_SECURITY_INFORMATION && !GetSecurityDescriptorGroup(sd, &group, &def)) ||
            (dwFlags & DACL_SECURITY_INFORMATION && !GetSecurityDescriptorDacl(sd, &present, &dacl, &def)) ||
            (dwFlags & SACL_SECURITY_INFORMATION && !GetSecurityDescriptorSacl(sd, &present, &sacl, &def)))
        {
            RegCloseKey(hKey);
            return FALSE;
        }

        err = SetSecurityInfo(hKey, SE_REGISTRY_KEY, dwFlags, owner, group, dacl, sacl);
        RegCloseKey(hKey);
        if (err)
        {
            SetLastError(err);
            return FALSE;
        }
        return TRUE;
    }
    default:
        FIXME("unimplemented parameter %08lx\n", dwParam);
        return FALSE;
    }
}

/******************************************************************************
 * CPSignHash (RSAENH.@)
 *
 * Sign a hash object
 *
 * PARAMS
 *  hProv        [I]   The key container, to which the hash object belongs.
 *  hHash        [I]   The hash object to be signed.
 *  dwKeySpec    [I]   AT_SIGNATURE or AT_KEYEXCHANGE: Key used to generate the signature.
 *  sDescription [I]   Should be NULL for security reasons. 
 *  dwFlags      [I]   0, CRYPT_NOHASHOID or CRYPT_X931_FORMAT: Format of the signature.
 *  pbSignature  [O]   Buffer, to which the signature will be stored. May be NULL to query SigLen.
 *  pdwSigLen    [I/O] Size of the buffer (in), Length of the signature (out)
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI RSAENH_CPSignHash(HCRYPTPROV hProv, HCRYPTHASH hHash, DWORD dwKeySpec, 
                              LPCWSTR sDescription, DWORD dwFlags, BYTE *pbSignature, 
                              DWORD *pdwSigLen)
{
    HCRYPTKEY hCryptKey = (HCRYPTKEY)INVALID_HANDLE_VALUE;
    CRYPTKEY *pCryptKey;
    DWORD dwHashLen;
    BYTE abHashValue[RSAENH_MAX_HASH_SIZE];
    ALG_ID aiAlgid;
    BOOL ret = FALSE;

    TRACE("(hProv=%08Ix, hHash=%08Ix, dwKeySpec=%08lx, sDescription=%s, dwFlags=%08lx, "
        "pbSignature=%p, pdwSigLen=%p)\n", hProv, hHash, dwKeySpec, debugstr_w(sDescription),
        dwFlags, pbSignature, pdwSigLen);

    if (dwFlags & ~(CRYPT_NOHASHOID|CRYPT_X931_FORMAT)) {
        SetLastError(NTE_BAD_FLAGS);
        return FALSE;
    }
    
    if (!RSAENH_CPGetUserKey(hProv, dwKeySpec, &hCryptKey)) return FALSE;
            
    if (!lookup_handle(&handle_table, hCryptKey, RSAENH_MAGIC_KEY,
                       (OBJECTHDR**)&pCryptKey))
    {
        SetLastError(NTE_NO_KEY);
        goto out;
    }

    if (!pbSignature) {
        *pdwSigLen = pCryptKey->dwKeyLen;
        ret = TRUE;
        goto out;
    }
    if (pCryptKey->dwKeyLen > *pdwSigLen)
    {
        SetLastError(ERROR_MORE_DATA);
        *pdwSigLen = pCryptKey->dwKeyLen;
        goto out;
    }
    *pdwSigLen = pCryptKey->dwKeyLen;

    if (sDescription) {
        if (!RSAENH_CPHashData(hProv, hHash, (const BYTE*)sDescription,
                                (DWORD)lstrlenW(sDescription)*sizeof(WCHAR), 0))
        {
            goto out;
        }
    }
    
    dwHashLen = sizeof(DWORD);
    if (!RSAENH_CPGetHashParam(hProv, hHash, HP_ALGID, (BYTE*)&aiAlgid, &dwHashLen, 0)) goto out;
    
    dwHashLen = RSAENH_MAX_HASH_SIZE;
    if (!RSAENH_CPGetHashParam(hProv, hHash, HP_HASHVAL, abHashValue, &dwHashLen, 0)) goto out;
 

    if (!build_hash_signature(pbSignature, *pdwSigLen, aiAlgid, abHashValue, dwHashLen, dwFlags)) {
        goto out;
    }

    ret = encrypt_block_impl(pCryptKey->aiAlgid, PK_PRIVATE, &pCryptKey->context, pbSignature, pbSignature);
out:
    RSAENH_CPDestroyKey(hProv, hCryptKey);
    return ret;
}

/******************************************************************************
 * CPVerifySignature (RSAENH.@)
 *
 * Verify the signature of a hash object.
 * 
 * PARAMS
 *  hProv        [I] The key container, to which the hash belongs.
 *  hHash        [I] The hash for which the signature is verified.
 *  pbSignature  [I] The binary signature.
 *  dwSigLen     [I] Length of the signature BLOB.
 *  hPubKey      [I] Public key used to verify the signature.
 *  sDescription [I] Should be NULL for security reasons.
 *  dwFlags      [I] 0, CRYPT_NOHASHOID or CRYPT_X931_FORMAT: Format of the signature.
 *
 * RETURNS
 *  Success: TRUE  (Signature is valid)
 *  Failure: FALSE (GetLastError() == NTE_BAD_SIGNATURE, if signature is invalid)
 */
BOOL WINAPI RSAENH_CPVerifySignature(HCRYPTPROV hProv, HCRYPTHASH hHash, const BYTE *pbSignature,
                                     DWORD dwSigLen, HCRYPTKEY hPubKey, LPCWSTR sDescription, 
                                     DWORD dwFlags)
{
    BYTE *pbConstructed = NULL, *pbDecrypted = NULL;
    CRYPTKEY *pCryptKey;
    DWORD dwHashLen;
    ALG_ID aiAlgid;
    BYTE abHashValue[RSAENH_MAX_HASH_SIZE];
    BOOL res = FALSE;

    TRACE("(hProv=%08Ix, hHash=%08Ix, pbSignature=%p, dwSigLen=%ld, hPubKey=%08Ix, sDescription=%s, "
          "dwFlags=%08lx)\n", hProv, hHash, pbSignature, dwSigLen, hPubKey, debugstr_w(sDescription),
          dwFlags);
        
    if (dwFlags & ~(CRYPT_NOHASHOID|CRYPT_X931_FORMAT)) {
        SetLastError(NTE_BAD_FLAGS);
        return FALSE;
    }
    
    if (!is_valid_handle(&handle_table, hProv, RSAENH_MAGIC_CONTAINER))
    {
        SetLastError(NTE_BAD_UID);
        return FALSE;
    }
 
    if (!lookup_handle(&handle_table, hPubKey, RSAENH_MAGIC_KEY,
                       (OBJECTHDR**)&pCryptKey))
    {
        SetLastError(NTE_BAD_KEY);
        return FALSE;
    }

    /* in Microsoft implementation, the signature length is checked before
     * the signature pointer.
     */
    if (dwSigLen != pCryptKey->dwKeyLen)
    {
        SetLastError(NTE_BAD_SIGNATURE);
        return FALSE;
    }

    if (!hHash || !pbSignature)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (sDescription) {
        if (!RSAENH_CPHashData(hProv, hHash, (const BYTE*)sDescription,
                                (DWORD)lstrlenW(sDescription)*sizeof(WCHAR), 0))
        {
            return FALSE;
        }
    }
    
    dwHashLen = sizeof(DWORD);
    if (!RSAENH_CPGetHashParam(hProv, hHash, HP_ALGID, (BYTE*)&aiAlgid, &dwHashLen, 0)) return FALSE;
    
    dwHashLen = RSAENH_MAX_HASH_SIZE;
    if (!RSAENH_CPGetHashParam(hProv, hHash, HP_HASHVAL, abHashValue, &dwHashLen, 0)) return FALSE;

    pbConstructed = malloc(dwSigLen);
    if (!pbConstructed) {
        SetLastError(NTE_NO_MEMORY);
        goto cleanup;
    }

    pbDecrypted = malloc(dwSigLen);
    if (!pbDecrypted) {
        SetLastError(NTE_NO_MEMORY);
        goto cleanup;
    }

    if (!decrypt_block_impl(pCryptKey->aiAlgid, PK_PUBLIC, &pCryptKey->context, pbSignature, pbDecrypted))
    {
        goto cleanup;
    }

    if (build_hash_signature(pbConstructed, dwSigLen, aiAlgid, abHashValue, dwHashLen, dwFlags) &&
        !memcmp(pbDecrypted, pbConstructed, dwSigLen)) {
        res = TRUE;
        goto cleanup;
    }

    if (!(dwFlags & CRYPT_NOHASHOID) &&
        build_hash_signature(pbConstructed, dwSigLen, aiAlgid, abHashValue, dwHashLen, dwFlags|CRYPT_NOHASHOID) &&
        !memcmp(pbDecrypted, pbConstructed, dwSigLen)) {
        res = TRUE;
        goto cleanup;
    }

    SetLastError(NTE_BAD_SIGNATURE);

cleanup:
    free(pbConstructed);
    free(pbDecrypted);
    return res;
}
