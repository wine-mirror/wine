/*
 * Unit tests for rsaenh functions
 *
 * Copyright (c) 2004 Michael Jung
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

#include <string.h>
#include <stdio.h>
#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wincrypt.h"

static HCRYPTPROV hProv;
static const char szContainer[] = "winetest";
static const unsigned char pbData[] = "Wine rocks totally!";
static const char szProvider[] = MS_ENHANCED_PROV_A;

/*
static void trace_hex(BYTE *pbData, DWORD dwLen) {
    char szTemp[256];
    DWORD i, j;

    for (i = 0; i < dwLen-7; i+=8) {
        sprintf(szTemp, "0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x,\n", 
            pbData[i], pbData[i+1], pbData[i+2], pbData[i+3], pbData[i+4], pbData[i+5], 
            pbData[i+6], pbData[i+7]);
        trace(szTemp);
    }
    for (j=0; i<dwLen; j++,i++) {
        sprintf(szTemp+6*j, "0x%02x, \n", pbData[i]);
    }
    trace(szTemp);
}
*/

static int init_environment(void)
{
    HCRYPTKEY hKey;
    BOOL result;
        
    hProv = (HCRYPTPROV)INVALID_HANDLE_VALUE;

    if (!CryptAcquireContext(&hProv, szContainer, szProvider, PROV_RSA_FULL, 0))
    {
        ok(GetLastError()==NTE_BAD_KEYSET, "%08lx\n", GetLastError());
        if (GetLastError()!=NTE_BAD_KEYSET) return 0;
        result = CryptAcquireContext(&hProv, szContainer, szProvider, PROV_RSA_FULL, 
                                    CRYPT_NEWKEYSET);
        ok(result, "%08lx\n", GetLastError());
        if (!result) return 0;
        result = CryptGenKey(hProv, AT_KEYEXCHANGE, 0, &hKey);
        ok(result, "%08lx\n", GetLastError());
        if (result) CryptDestroyKey(hKey);
        result = CryptGenKey(hProv, AT_SIGNATURE, 0, &hKey);
        ok(result, "%08lx\n", GetLastError());
        if (result) CryptDestroyKey(hKey);
    }
    return 1;
}

static void clean_up_environment(void)
{
    CryptAcquireContext(&hProv, szContainer, szProvider, PROV_RSA_FULL, CRYPT_DELETEKEYSET);
}

static void test_prov() 
{
    BOOL result;
    DWORD dwLen, dwInc;
    
    dwLen = (DWORD)sizeof(DWORD);
    result = CryptGetProvParam(hProv, PP_SIG_KEYSIZE_INC, (BYTE*)&dwInc, &dwLen, 0);
    ok(result && dwInc==8, "%08lx, %ld\n", GetLastError(), dwInc);
    
    dwLen = (DWORD)sizeof(DWORD);
    result = CryptGetProvParam(hProv, PP_KEYX_KEYSIZE_INC, (BYTE*)&dwInc, &dwLen, 0);
    ok(result && dwInc==8, "%08lx, %ld\n", GetLastError(), dwInc);
}

static void test_gen_random()
{
    BOOL result;
    BYTE rnd1[16], rnd2[16];

    memset(rnd1, 0, sizeof(rnd1));
    memset(rnd2, 0, sizeof(rnd2));

    result = CryptGenRandom(hProv, sizeof(rnd1), rnd1);
    if (!result && GetLastError() == NTE_FAIL) {
        /* rsaenh compiled without OpenSSL */
        return;
    }
    
    ok(result, "%08lx\n", GetLastError());

    result = CryptGenRandom(hProv, sizeof(rnd2), rnd2);
    ok(result, "%08lx\n", GetLastError());

    ok(memcmp(rnd1, rnd2, sizeof(rnd1)), "CryptGenRandom generates non random data\n");
}

static BOOL derive_key(ALG_ID aiAlgid, HCRYPTKEY *phKey, DWORD len) 
{
    HCRYPTHASH hHash;
    BOOL result;
    unsigned char pbData[2000];
    int i;

    *phKey = (HCRYPTKEY)NULL;
    for (i=0; i<2000; i++) pbData[i] = (unsigned char)i;
    result = CryptCreateHash(hProv, CALG_MD2, 0, 0, &hHash);
    if (!result) {
        /* rsaenh compiled without OpenSSL */
        ok(GetLastError()==NTE_BAD_ALGID, "%08lx", GetLastError());
        return FALSE;
    } 
    ok(result, "%08lx\n", GetLastError());
    if (!result) return FALSE;
    result = CryptHashData(hHash, (BYTE*)pbData, sizeof(pbData), 0);
    ok(result, "%08lx\n", GetLastError());
    if (!result) return FALSE;
    result = CryptDeriveKey(hProv, aiAlgid, hHash, (len << 16) | CRYPT_EXPORTABLE, phKey);
    ok(result, "%08lx\n", GetLastError());
    if (!result) return FALSE;
    len = 2000;
    result = CryptGetHashParam(hHash, HP_HASHVAL, pbData, &len, 0);
    ok(result, "%08lx\n", GetLastError());
    CryptDestroyHash(hHash);
    return TRUE;
}

static void test_hashes(void)
{
    static const unsigned char md2hash[16] = {
        0x12, 0xcb, 0x1b, 0x08, 0xc8, 0x48, 0xa4, 0xa9, 
        0xaa, 0xf3, 0xf1, 0x9f, 0xfc, 0x29, 0x28, 0x68 };
    static const unsigned char md4hash[16] = {
        0x8e, 0x2a, 0x58, 0xbf, 0xf2, 0xf5, 0x26, 0x23, 
        0x79, 0xd2, 0x92, 0x36, 0x1b, 0x23, 0xe3, 0x81 };
    static const unsigned char md5hash[16] = { 
        0x15, 0x76, 0xa9, 0x4d, 0x6c, 0xb3, 0x34, 0xdd, 
        0x12, 0x6c, 0xb1, 0xc2, 0x7f, 0x19, 0xe0, 0xf2 };    
    static const unsigned char sha1hash[20] = { 
        0xf1, 0x0c, 0xcf, 0xde, 0x60, 0xc1, 0x7d, 0xb2, 0x6e, 0x7d, 
        0x85, 0xd3, 0x56, 0x65, 0xc7, 0x66, 0x1d, 0xbb, 0xeb, 0x2c };
    unsigned char pbData[2048];
    BOOL result;
    HCRYPTHASH hHash, hHashClone;
    BYTE pbHashValue[36];
    DWORD hashlen, len;
    int i;

    for (i=0; i<2048; i++) pbData[i] = (unsigned char)i;

    /* MD2 Hashing */
    result = CryptCreateHash(hProv, CALG_MD2, 0, 0, &hHash);
    if (!result) {
        /* rsaenh compiled without OpenSSL */
        ok(GetLastError() == NTE_BAD_ALGID, "%08lx\n", GetLastError());
    } else {
        result = CryptHashData(hHash, (BYTE*)pbData, sizeof(pbData), 0);
        ok(result, "%08lx\n", GetLastError());

        len = sizeof(DWORD);
        result = CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&hashlen, &len, 0);
           ok(result && (hashlen == 16), "%08lx, hashlen: %ld\n", GetLastError(), hashlen);

        len = 16;
        result = CryptGetHashParam(hHash, HP_HASHVAL, pbHashValue, &len, 0);
        ok(result, "%08lx\n", GetLastError());

        ok(!memcmp(pbHashValue, md2hash, 16), "Wrong MD2 hash!\n");

        result = CryptDestroyHash(hHash);
        ok(result, "%08lx\n", GetLastError());
    } 

    /* MD4 Hashing */
    result = CryptCreateHash(hProv, CALG_MD4, 0, 0, &hHash);
    ok(result, "%08lx\n", GetLastError());

    result = CryptHashData(hHash, (BYTE*)pbData, sizeof(pbData), 0);
    ok(result, "%08lx\n", GetLastError());

    len = sizeof(DWORD);
    result = CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&hashlen, &len, 0);
    ok(result && (hashlen == 16), "%08lx, hashlen: %ld\n", GetLastError(), hashlen);

    len = 16;
    result = CryptGetHashParam(hHash, HP_HASHVAL, pbHashValue, &len, 0);
    ok(result, "%08lx\n", GetLastError());

    ok(!memcmp(pbHashValue, md4hash, 16), "Wrong MD4 hash!\n");

    result = CryptDestroyHash(hHash);
    ok(result, "%08lx\n", GetLastError());

    /* MD5 Hashing */
    result = CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash);
    ok(result, "%08lx\n", GetLastError());

    result = CryptHashData(hHash, (BYTE*)pbData, sizeof(pbData), 0);
    ok(result, "%08lx\n", GetLastError());

    len = sizeof(DWORD);
    result = CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&hashlen, &len, 0);
    ok(result && (hashlen == 16), "%08lx, hashlen: %ld\n", GetLastError(), hashlen);

    len = 16;
    result = CryptGetHashParam(hHash, HP_HASHVAL, pbHashValue, &len, 0);
    ok(result, "%08lx\n", GetLastError());

    ok(!memcmp(pbHashValue, md5hash, 16), "Wrong MD5 hash!\n");

    result = CryptDestroyHash(hHash);
    ok(result, "%08lx\n", GetLastError());

    /* SHA1 Hashing */
    result = CryptCreateHash(hProv, CALG_SHA, 0, 0, &hHash);
    ok(result, "%08lx\n", GetLastError());

    result = CryptHashData(hHash, (BYTE*)pbData, 5, 0);
    ok(result, "%08lx\n", GetLastError());

    result = CryptDuplicateHash(hHash, 0, 0, &hHashClone);
    ok(result, "%08lx\n", GetLastError());
    
    result = CryptHashData(hHashClone, (BYTE*)pbData+5, sizeof(pbData)-5, 0);
    ok(result, "%08lx\n", GetLastError());

    len = sizeof(DWORD);
    result = CryptGetHashParam(hHashClone, HP_HASHSIZE, (BYTE*)&hashlen, &len, 0);
    ok(result && (hashlen == 20), "%08lx, hashlen: %ld\n", GetLastError(), hashlen);

    len = 20;
    result = CryptGetHashParam(hHashClone, HP_HASHVAL, pbHashValue, &len, 0);
    ok(result, "%08lx\n", GetLastError());

    ok(!memcmp(pbHashValue, sha1hash, 20), "Wrong SHA1 hash!\n");

    result = CryptDestroyHash(hHashClone);
    ok(result, "%08lx\n", GetLastError());

    result = CryptDestroyHash(hHash);
    ok(result, "%08lx\n", GetLastError());
}    

static void test_block_cipher_modes()
{
    static const BYTE plain[23] = { 
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 
        0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16 };
    static const BYTE ecb[24] = {   
        0xc0, 0x9a, 0xe4, 0x2f, 0x0a, 0x47, 0x67, 0x11, 0xf2, 0xb2, 0x5d, 0x5f, 
        0x08, 0xff, 0x49, 0xa4, 0x45, 0x3a, 0x68, 0x14, 0xca, 0x18, 0xe5, 0xf4 };
    static const BYTE cbc[24] = {   
        0xc0, 0x9a, 0xe4, 0x2f, 0x0a, 0x47, 0x67, 0x11, 0x10, 0xf5, 0xda, 0x61,
        0x4e, 0x3d, 0xab, 0xc0, 0x97, 0x85, 0x01, 0x12, 0x97, 0xa4, 0xf7, 0xd3 };
    static const BYTE cfb[24] = {   
        0x29, 0xb5, 0x67, 0x85, 0x0b, 0x1b, 0xec, 0x07, 0x67, 0x2d, 0xa1, 0xa4,
        0x1a, 0x47, 0x24, 0x6a, 0x54, 0xe1, 0xe0, 0x92, 0xf9, 0x0e, 0xf6, 0xeb };
    HCRYPTKEY hKey;
    BOOL result;
    BYTE abData[24];
    DWORD dwMode, dwLen;

    result = derive_key(CALG_RC2, &hKey, 40);
    if (!result) return;

    memcpy(abData, plain, sizeof(abData));

    dwMode = CRYPT_MODE_ECB;
    result = CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, 0);
    ok(result, "%08lx\n", GetLastError());

    SetLastError(ERROR_SUCCESS);
    dwLen = 23;
    result = CryptEncrypt(hKey, (HCRYPTHASH)NULL, TRUE, 0, abData, &dwLen, 24);
    ok(result && dwLen == 24 && !memcmp(ecb, abData, sizeof(ecb)), 
       "%08lx, dwLen: %ld\n", GetLastError(), dwLen);

    result = CryptDecrypt(hKey, (HCRYPTHASH)NULL, TRUE, 0, abData, &dwLen);
    ok(result && dwLen == 23 && !memcmp(plain, abData, sizeof(plain)), 
       "%08lx, dwLen: %ld\n", GetLastError(), dwLen);

    dwMode = CRYPT_MODE_CBC;
    result = CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, 0);
    ok(result, "%08lx\n", GetLastError());
    
    dwLen = 23;
    result = CryptEncrypt(hKey, (HCRYPTHASH)NULL, TRUE, 0, abData, &dwLen, 24);
    ok(result && dwLen == 24 && !memcmp(cbc, abData, sizeof(cbc)), 
       "%08lx, dwLen: %ld\n", GetLastError(), dwLen);

    result = CryptDecrypt(hKey, (HCRYPTHASH)NULL, TRUE, 0, abData, &dwLen);
    ok(result && dwLen == 23 && !memcmp(plain, abData, sizeof(plain)), 
       "%08lx, dwLen: %ld\n", GetLastError(), dwLen);

    dwMode = CRYPT_MODE_CFB;
    result = CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, 0);
    ok(result, "%08lx\n", GetLastError());
    
    dwLen = 16;
    result = CryptEncrypt(hKey, (HCRYPTHASH)NULL, FALSE, 0, abData, &dwLen, 24);
    ok(result && dwLen == 16, "%08lx, dwLen: %ld\n", GetLastError(), dwLen);

    dwLen = 7;
    result = CryptEncrypt(hKey, (HCRYPTHASH)NULL, TRUE, 0, abData+16, &dwLen, 8);
    ok(result && dwLen == 8 && !memcmp(cfb, abData, sizeof(cfb)), 
       "%08lx, dwLen: %ld\n", GetLastError(), dwLen);
    
    dwLen = 8;
    result = CryptDecrypt(hKey, (HCRYPTHASH)NULL, FALSE, 0, abData, &dwLen);
    ok(result && dwLen == 8, "%08lx, dwLen: %ld\n", GetLastError(), dwLen);

    dwLen = 16;
    result = CryptDecrypt(hKey, (HCRYPTHASH)NULL, TRUE, 0, abData+8, &dwLen);
    ok(result && dwLen == 15 && !memcmp(plain, abData, sizeof(plain)), 
       "%08lx, dwLen: %ld\n", GetLastError(), dwLen);

    dwMode = CRYPT_MODE_OFB;
    result = CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, 0);
    ok(result, "%08lx\n", GetLastError());
    
    dwLen = 23;
    result = CryptEncrypt(hKey, (HCRYPTHASH)NULL, TRUE, 0, abData, &dwLen, 24);
    ok(!result && GetLastError() == NTE_BAD_ALGID, "%08lx\n", GetLastError());
}

static void test_3des112()
{
    HCRYPTKEY hKey;
    BOOL result;
    DWORD dwLen;
    unsigned char pbData[16];
    int i;

    result = derive_key(CALG_3DES_112, &hKey, 0);
    if (!result) {
        /* rsaenh compiled without OpenSSL */
        ok(GetLastError() == NTE_BAD_ALGID, "%08lx\n", GetLastError());
        return;
    }

    for (i=0; i<sizeof(pbData); i++) pbData[i] = (unsigned char)i;
    
    dwLen = 13;
    result = CryptEncrypt(hKey, (HCRYPTHASH)NULL, TRUE, 0, pbData, &dwLen, 16);
    ok(result, "%08lx\n", GetLastError());
    
    result = CryptDecrypt(hKey, (HCRYPTHASH)NULL, TRUE, 0, pbData, &dwLen);
    ok(result, "%08lx\n", GetLastError());

    result = CryptDestroyKey(hKey);
    ok(result, "%08lx\n", GetLastError());
}

static void test_des() 
{
    HCRYPTKEY hKey;
    BOOL result;
    DWORD dwLen, dwMode;
    unsigned char pbData[16];
    int i;

    result = derive_key(CALG_DES, &hKey, 56);
    if (!result) {
        /* rsaenh compiled without OpenSSL */
        ok(GetLastError()==NTE_BAD_ALGID, "%08lx\n", GetLastError());
        return;
    }

    dwMode = CRYPT_MODE_ECB;
    result = CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, 0);
    ok(result, "%08lx\n", GetLastError());
    
    dwLen = sizeof(DWORD);
    result = CryptGetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, &dwLen, 0);
    ok(result, "%08lx\n", GetLastError());
    
    for (i=0; i<sizeof(pbData); i++) pbData[i] = (unsigned char)i;
    
    dwLen = 13;
    result = CryptEncrypt(hKey, (HCRYPTHASH)NULL, TRUE, 0, pbData, &dwLen, 16);
    ok(result, "%08lx\n", GetLastError());
    
    result = CryptDecrypt(hKey, (HCRYPTHASH)NULL, TRUE, 0, pbData, &dwLen);
    ok(result, "%08lx\n", GetLastError());

    result = CryptDestroyKey(hKey);
    ok(result, "%08lx\n", GetLastError());
}

static void test_3des()
{
    HCRYPTKEY hKey;
    BOOL result;
    DWORD dwLen;
    unsigned char pbData[16];
    static const BYTE des3[16] = { 
        0x7b, 0xba, 0xdd, 0xa2, 0x39, 0xd3, 0x7b, 0xb3, 
        0xc7, 0x51, 0x81, 0x41, 0x53, 0xe8, 0xcf, 0xeb };
    int i;

    result = derive_key(CALG_3DES, &hKey, 0);
    if (!result) return;

    for (i=0; i<sizeof(pbData); i++) pbData[i] = (unsigned char)i;
    
    dwLen = 13;
    result = CryptEncrypt(hKey, (HCRYPTHASH)NULL, TRUE, 0, pbData, &dwLen, 16);
    ok(result, "%08lx\n", GetLastError());
    
    ok(!memcmp(pbData, des3, sizeof(des3)), "3DES encryption failed!\n");
    
    result = CryptDecrypt(hKey, (HCRYPTHASH)NULL, TRUE, 0, pbData, &dwLen);
    ok(result, "%08lx\n", GetLastError());

    result = CryptDestroyKey(hKey);
    ok(result, "%08lx\n", GetLastError());
}

static void test_rc2()
{
    static const BYTE rc2encrypted[16] = { 
        0x02, 0x34, 0x7d, 0xf6, 0x1d, 0xc5, 0x9b, 0x8b, 
        0x2e, 0x0d, 0x63, 0x80, 0x72, 0xc1, 0xc2, 0xb1 };
    HCRYPTHASH hHash;
    HCRYPTKEY hKey;
    BOOL result;
    DWORD dwLen, dwKeyLen, dwDataLen, dwMode, dwModeBits;
    BYTE *pbTemp;
    unsigned char pbData[2000], pbHashValue[16];
    int i;
    
    for (i=0; i<2000; i++) pbData[i] = (unsigned char)i;

    /* MD2 Hashing */
    result = CryptCreateHash(hProv, CALG_MD2, 0, 0, &hHash);
    if (!result) {
        ok(GetLastError()==NTE_BAD_ALGID, "%08lx\n", GetLastError());
    } else {
        result = CryptHashData(hHash, (BYTE*)pbData, sizeof(pbData), 0);
        ok(result, "%08lx\n", GetLastError());

        dwLen = 16;
        result = CryptGetHashParam(hHash, HP_HASHVAL, pbHashValue, &dwLen, 0);
        ok(result, "%08lx\n", GetLastError());

        result = CryptDeriveKey(hProv, CALG_RC2, hHash, 56 << 16, &hKey);
        ok(result, "%08lx\n", GetLastError());

        dwLen = sizeof(DWORD);
        result = CryptGetKeyParam(hKey, KP_KEYLEN, (BYTE*)&dwKeyLen, &dwLen, 0);
        ok(result, "%08lx\n", GetLastError());

        dwMode = CRYPT_MODE_CBC;
        result = CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, 0);
        ok(result, "%08lx\n", GetLastError());

        dwLen = sizeof(DWORD);
        result = CryptGetKeyParam(hKey, KP_MODE_BITS, (BYTE*)&dwModeBits, &dwLen, 0);
        ok(result, "%08lx\n", GetLastError());

        dwLen = sizeof(DWORD);
        result = CryptGetKeyParam(hKey, KP_PERMISSIONS, (BYTE*)&dwModeBits, &dwLen, 0);
        ok(result, "%08lx\n", GetLastError());

        dwLen = sizeof(DWORD);
        result = CryptGetKeyParam(hKey, KP_BLOCKLEN, (BYTE*)&dwModeBits, &dwLen, 0);
        ok(result, "%08lx\n", GetLastError());

        result = CryptGetKeyParam(hKey, KP_IV, NULL, &dwLen, 0);
        ok(result, "%08lx\n", GetLastError());
        pbTemp = HeapAlloc(GetProcessHeap(), 0, dwLen);
        CryptGetKeyParam(hKey, KP_IV, pbTemp, &dwLen, 0);
        HeapFree(GetProcessHeap(), 0, pbTemp);

        result = CryptGetKeyParam(hKey, KP_SALT, NULL, &dwLen, 0);
        ok(result, "%08lx\n", GetLastError());
        pbTemp = HeapAlloc(GetProcessHeap(), 0, dwLen);
        CryptGetKeyParam(hKey, KP_SALT, pbTemp, &dwLen, 0);
        HeapFree(GetProcessHeap(), 0, pbTemp);

        dwLen = sizeof(DWORD);
        CryptGetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, &dwLen, 0);

        result = CryptDestroyHash(hHash);
        ok(result, "%08lx\n", GetLastError());

        dwDataLen = 13;
        result = CryptEncrypt(hKey, (HCRYPTHASH)NULL, TRUE, 0, pbData, &dwDataLen, 24);
        ok(result, "%08lx\n", GetLastError());

        ok(!memcmp(pbData, rc2encrypted, 8), "RC2 encryption failed!\n");

        result = CryptGetKeyParam(hKey, KP_IV, NULL, &dwLen, 0);
        ok(result, "%08lx\n", GetLastError());
        pbTemp = HeapAlloc(GetProcessHeap(), 0, dwLen);
        CryptGetKeyParam(hKey, KP_IV, pbTemp, &dwLen, 0);
        HeapFree(GetProcessHeap(), 0, pbTemp);

        result = CryptDecrypt(hKey, (HCRYPTHASH)NULL, TRUE, 0, pbData, &dwDataLen);
        ok(result, "%08lx\n", GetLastError());

        result = CryptDestroyKey(hKey);
        ok(result, "%08lx\n", GetLastError());
    }
}

static void test_rc4()
{
    static const BYTE rc4[16] = { 
        0x17, 0x0c, 0x44, 0x8e, 0xae, 0x90, 0xcd, 0xb0, 
        0x7f, 0x87, 0xf5, 0x7a, 0xec, 0xb2, 0x2e, 0x35 };    
    BOOL result;
    HCRYPTHASH hHash;
    HCRYPTKEY hKey;
    DWORD dwDataLen = 5, dwKeyLen, dwLen = sizeof(DWORD), dwMode;
    unsigned char pbData[2000], *pbTemp;
    unsigned char pszBuffer[256];
    int i;

    for (i=0; i<2000; i++) pbData[i] = (unsigned char)i;

    /* MD2 Hashing */
    result = CryptCreateHash(hProv, CALG_MD2, 0, 0, &hHash);
    if (!result) {
        /* rsaenh compiled without OpenSSL */
        ok(GetLastError() == NTE_BAD_ALGID, "%08lx\n", GetLastError());
    } else {
        result = CryptHashData(hHash, (BYTE*)pbData, sizeof(pbData), 0);
           ok(result, "%08lx\n", GetLastError());

        dwLen = 16;
        result = CryptGetHashParam(hHash, HP_HASHVAL, pszBuffer, &dwLen, 0);
        ok(result, "%08lx\n", GetLastError());

        result = CryptDeriveKey(hProv, CALG_RC4, hHash, 56 << 16, &hKey);
        ok(result, "%08lx\n", GetLastError());

        dwLen = sizeof(DWORD);
        result = CryptGetKeyParam(hKey, KP_KEYLEN, (BYTE*)&dwKeyLen, &dwLen, 0);
        ok(result, "%08lx\n", GetLastError());

        dwLen = sizeof(DWORD);
        result = CryptGetKeyParam(hKey, KP_BLOCKLEN, (BYTE*)&dwKeyLen, &dwLen, 0);
        ok(result, "%08lx\n", GetLastError());

        result = CryptGetKeyParam(hKey, KP_IV, NULL, &dwLen, 0);
        ok(result, "%08lx\n", GetLastError());
        pbTemp = HeapAlloc(GetProcessHeap(), 0, dwLen);
        CryptGetKeyParam(hKey, KP_IV, pbTemp, &dwLen, 0);
        HeapFree(GetProcessHeap(), 0, pbTemp);

        result = CryptGetKeyParam(hKey, KP_SALT, NULL, &dwLen, 0);
        ok(result, "%08lx\n", GetLastError());
        pbTemp = HeapAlloc(GetProcessHeap(), 0, dwLen);
        CryptGetKeyParam(hKey, KP_SALT, pbTemp, &dwLen, 0);
        HeapFree(GetProcessHeap(), 0, pbTemp);

        dwLen = sizeof(DWORD);
        CryptGetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, &dwLen, 0);

        result = CryptDestroyHash(hHash);
        ok(result, "%08lx\n", GetLastError());

        dwDataLen = 16;
        result = CryptEncrypt(hKey, (HCRYPTHASH)NULL, TRUE, 0, pbData, &dwDataLen, 24);
        ok(result, "%08lx\n", GetLastError());

        ok(!memcmp(pbData, rc4, dwDataLen), "RC4 encryption failed!\n");

        result = CryptDecrypt(hKey, (HCRYPTHASH)NULL, TRUE, 0, pbData, &dwDataLen);
        ok(result, "%08lx\n", GetLastError());

        result = CryptDestroyKey(hKey);
        ok(result, "%08lx\n", GetLastError());
    }
}

static void test_hmac() {
    HCRYPTKEY hKey;
    HCRYPTHASH hHash;
    BOOL result;
    HMAC_INFO hmacInfo = { CALG_MD2, NULL, 0, NULL, 0 };
    DWORD dwLen;
    BYTE abData[256];
    static const BYTE hmac[16] = { 
        0xfd, 0x16, 0xb5, 0xb6, 0x13, 0x1c, 0x2b, 0xd6, 
        0x0a, 0xc7, 0xae, 0x92, 0x76, 0xa3, 0x05, 0x71 };
    int i;

    for (i=0; i<sizeof(abData)/sizeof(BYTE); i++) abData[i] = (BYTE)i;

    if (!derive_key(CALG_RC2, &hKey, 56)) return;

    result = CryptCreateHash(hProv, CALG_HMAC, hKey, 0, &hHash);
    ok(result, "%08lx\n", GetLastError());
    if (!result) return;

    result = CryptSetHashParam(hHash, HP_HMAC_INFO, (BYTE*)&hmacInfo, 0);
    ok(result, "%08lx\n", GetLastError());

    result = CryptHashData(hHash, (BYTE*)abData, sizeof(abData), 0);
    ok(result, "%08lx\n", GetLastError());

    dwLen = sizeof(abData)/sizeof(BYTE);
    result = CryptGetHashParam(hHash, HP_HASHVAL, abData, &dwLen, 0);
    ok(result, "%08lx\n", GetLastError());

    ok(!memcmp(abData, hmac, sizeof(hmac)), "HMAC failed!\n");
    
    result = CryptDestroyHash(hHash);
    ok(result, "%08lx\n", GetLastError());
    
    result = CryptDestroyKey(hKey);
    ok(result, "%08lx\n", GetLastError());

    /* Provoke errors */
    result = CryptCreateHash(hProv, CALG_HMAC, 0, 0, &hHash);
    ok(!result && GetLastError() == NTE_BAD_KEY, "%08lx\n", GetLastError());
}

static void test_mac() {
    HCRYPTKEY hKey;
    HCRYPTHASH hHash;
    BOOL result;
    DWORD dwLen;
    BYTE abData[256], abEnc[264];
    static const BYTE mac[8] = { 0x0d, 0x3e, 0x15, 0x6b, 0x85, 0x63, 0x5c, 0x11 };
    int i;

    for (i=0; i<sizeof(abData)/sizeof(BYTE); i++) abData[i] = (BYTE)i;
    for (i=0; i<sizeof(abData)/sizeof(BYTE); i++) abEnc[i] = (BYTE)i;

    if (!derive_key(CALG_RC2, &hKey, 56)) return;

    dwLen = 256;
    result = CryptEncrypt(hKey, (HCRYPTHASH)NULL, TRUE, 0, abEnc, &dwLen, 264);
    ok (result && dwLen == 264, "%08lx, dwLen: %ld\n", GetLastError(), dwLen);
    
    result = CryptCreateHash(hProv, CALG_MAC, hKey, 0, &hHash);
    ok(result, "%08lx\n", GetLastError());
    if (!result) return;

    result = CryptHashData(hHash, (BYTE*)abData, sizeof(abData), 0);
    ok(result, "%08lx\n", GetLastError());

    dwLen = sizeof(abData)/sizeof(BYTE);
    result = CryptGetHashParam(hHash, HP_HASHVAL, abData, &dwLen, 0);
    ok(result && dwLen == 8, "%08lx, dwLen: %ld\n", GetLastError(), dwLen);

    ok(!memcmp(abData, mac, sizeof(mac)), "MAC failed!\n");
    
    result = CryptDestroyHash(hHash);
    ok(result, "%08lx\n", GetLastError());
    
    result = CryptDestroyKey(hKey);
    ok(result, "%08lx\n", GetLastError());
    
    /* Provoke errors */
    if (!derive_key(CALG_RC4, &hKey, 56)) return;

    result = CryptCreateHash(hProv, CALG_MAC, hKey, 0, &hHash);
    ok(!result && GetLastError() == NTE_BAD_KEY, "%08lx\n", GetLastError());

    result = CryptDestroyKey(hKey);
    ok(result, "%08lx\n", GetLastError());
}

static void test_import_private() 
{
    DWORD dwLen;
    HCRYPTKEY hKeyExchangeKey, hSessionKey;
    BOOL result;
    BYTE abPlainPrivateKey[596] = {
        0x07, 0x02, 0x00, 0x00, 0x00, 0xa4, 0x00, 0x00,
        0x52, 0x53, 0x41, 0x32, 0x00, 0x04, 0x00, 0x00,
        0x01, 0x00, 0x01, 0x00, 0x9b, 0x64, 0xef, 0xce,
        0x31, 0x7c, 0xad, 0x56, 0xe2, 0x1e, 0x9b, 0x96,
        0xb3, 0xf0, 0x29, 0x88, 0x6e, 0xa8, 0xc2, 0x11,
        0x33, 0xd6, 0xcc, 0x8c, 0x69, 0xb2, 0x1a, 0xfd,
        0xfc, 0x23, 0x21, 0x30, 0x4d, 0x29, 0x45, 0xb6,
        0x3a, 0x67, 0x11, 0x80, 0x1a, 0x91, 0xf2, 0x9f,
        0x01, 0xac, 0xc0, 0x11, 0x50, 0x5f, 0xcd, 0xb9,
        0xad, 0x76, 0x9f, 0x6e, 0x91, 0x55, 0x71, 0xda,
        0x97, 0x96, 0x96, 0x22, 0x75, 0xb4, 0x83, 0x44,
        0x89, 0x9e, 0xf8, 0x44, 0x40, 0x7c, 0xd6, 0xcd,
        0x9d, 0x88, 0xd6, 0x88, 0xbc, 0x56, 0xb7, 0x64,
        0xe9, 0x2c, 0x24, 0x2f, 0x0d, 0x78, 0x55, 0x1c,
        0xb2, 0x67, 0xb1, 0x5e, 0xbc, 0x0c, 0xcf, 0x1c,
        0xe9, 0xd3, 0x9e, 0xa2, 0x15, 0x24, 0x73, 0xd6,
        0xdb, 0x6f, 0x83, 0xb2, 0xf8, 0xbc, 0xe7, 0x47,
        0x3b, 0x01, 0xef, 0x49, 0x08, 0x98, 0xd6, 0xa3,
        0xf9, 0x25, 0x57, 0xe9, 0x39, 0x3c, 0x53, 0x30,
        0x1b, 0xf2, 0xc9, 0x62, 0x31, 0x43, 0x5d, 0x84,
        0x24, 0x30, 0x21, 0x9a, 0xad, 0xdb, 0x62, 0x91,
        0xc8, 0x07, 0xd9, 0x2f, 0xd6, 0xb5, 0x37, 0x6f,
        0xfe, 0x7a, 0x12, 0xbc, 0xd9, 0xd2, 0x2b, 0xbf,
        0xd7, 0xb1, 0xfa, 0x7d, 0xc0, 0x48, 0xdd, 0x74,
        0xdd, 0x55, 0x04, 0xa1, 0x8b, 0xc1, 0x0a, 0xc4,
        0xa5, 0x57, 0x62, 0xee, 0x08, 0x8b, 0xf9, 0x19,
        0x6c, 0x52, 0x06, 0xf8, 0x73, 0x0f, 0x24, 0xc9,
        0x71, 0x9f, 0xc5, 0x45, 0x17, 0x3e, 0xae, 0x06,
        0x81, 0xa2, 0x96, 0x40, 0x06, 0xbf, 0xeb, 0x9e,
        0x80, 0x2b, 0x27, 0x20, 0x8f, 0x38, 0xcf, 0xeb,
        0xff, 0x3b, 0x38, 0x41, 0x35, 0x69, 0x66, 0x13,
        0x1d, 0x3c, 0x01, 0x3b, 0xf6, 0x37, 0xca, 0x9c,
        0x61, 0x74, 0x98, 0xcf, 0xc9, 0x6e, 0xe8, 0x90,
        0xc7, 0xb7, 0x33, 0xc0, 0x07, 0x3c, 0xf8, 0xc8,
        0xf6, 0xf2, 0xd7, 0xf0, 0x21, 0x62, 0x58, 0x8a,
        0x55, 0xbf, 0xa1, 0x2d, 0x3d, 0xa6, 0x69, 0xc5,
        0x02, 0x19, 0x31, 0xf0, 0x94, 0x0f, 0x45, 0x5c,
        0x95, 0x1b, 0x53, 0xbc, 0xf5, 0xb0, 0x1a, 0x8f,
        0xbf, 0x40, 0xe0, 0xc7, 0x73, 0xe7, 0x72, 0x6e,
        0xeb, 0xb1, 0x0f, 0x38, 0xc5, 0xf8, 0xee, 0x04,
        0xed, 0x34, 0x1a, 0x10, 0xf9, 0x53, 0x34, 0xf3,
        0x3e, 0xe6, 0x5c, 0xd1, 0x47, 0x65, 0xcd, 0xbd,
        0xf1, 0x06, 0xcb, 0xb4, 0xb1, 0x26, 0x39, 0x9f,
        0x71, 0xfe, 0x3d, 0xf8, 0x62, 0xab, 0x22, 0x8b,
        0x0e, 0xdc, 0xb9, 0xe8, 0x74, 0x06, 0xfc, 0x8c,
        0x25, 0xa1, 0xa9, 0xcf, 0x07, 0xf9, 0xac, 0x21,
        0x01, 0x7b, 0x1c, 0xdc, 0x94, 0xbd, 0x47, 0xe1,
        0xa0, 0x86, 0x59, 0x35, 0x6a, 0x6f, 0xb9, 0x70,
        0x26, 0x7c, 0x3c, 0xfd, 0xbd, 0x81, 0x39, 0x36,
        0x42, 0xc2, 0xbd, 0xbe, 0x84, 0x27, 0x9a, 0x69,
        0x81, 0xda, 0x99, 0x27, 0xc2, 0x4f, 0x62, 0x33,
        0xf4, 0x79, 0x30, 0xc5, 0x63, 0x54, 0x71, 0xf1,
        0x47, 0x22, 0x25, 0x9b, 0x6c, 0x00, 0x2f, 0x1c,
        0xf4, 0x1f, 0x85, 0xbc, 0xf6, 0x67, 0x6a, 0xe3,
        0xf6, 0x55, 0x8a, 0xef, 0xd0, 0x0b, 0xd3, 0xa2,
        0xc5, 0x51, 0x70, 0x15, 0x0a, 0xf0, 0x98, 0x4c,
        0xb7, 0x19, 0x62, 0x0e, 0x2d, 0x2a, 0x4a, 0x7d,
        0x7a, 0x0a, 0xc4, 0x17, 0xe3, 0x5d, 0x20, 0x52,
        0xa9, 0x98, 0xc3, 0xaa, 0x11, 0xf6, 0xbf, 0x4c,
        0x94, 0x99, 0x81, 0x89, 0xf0, 0x7f, 0x66, 0xaa,
        0xc8, 0x88, 0xd7, 0x31, 0x84, 0x71, 0xb6, 0x64,
        0x09, 0x76, 0x0b, 0x7f, 0x1a, 0x1f, 0x2e, 0xfe,
        0xcd, 0x59, 0x2a, 0x54, 0x11, 0x84, 0xd4, 0x6a,
        0x61, 0xdf, 0xaa, 0x76, 0x66, 0x9d, 0x82, 0x11,
        0x56, 0x3d, 0xd2, 0x52, 0xe6, 0x42, 0x5a, 0x77,
        0x92, 0x98, 0x34, 0xf3, 0x56, 0x6c, 0x96, 0x10,
        0x40, 0x59, 0x16, 0xcb, 0x77, 0x61, 0xe3, 0xbf,
        0x4b, 0xd4, 0x39, 0xfb, 0xb1, 0x4e, 0xc1, 0x74,
        0xec, 0x7a, 0xea, 0x3d, 0x68, 0xbb, 0x0b, 0xe6,
        0xc6, 0x06, 0xbf, 0xdd, 0x7f, 0x94, 0x42, 0xc0,
        0x0f, 0xe4, 0x92, 0x33, 0x6c, 0x6e, 0x1b, 0xba,
        0x73, 0xf9, 0x79, 0x84, 0xdf, 0x45, 0x00, 0xe4,
        0x94, 0x88, 0x9d, 0x08, 0x89, 0xcf, 0xf2, 0xa4,
        0xc5, 0x47, 0x45, 0x85, 0x86, 0xa5, 0xcc, 0xa8,
        0xf2, 0x5d, 0x58, 0x07
    };
    BYTE abSessionKey[148] = {
        0x01, 0x02, 0x00, 0x00, 0x01, 0x68, 0x00, 0x00,
        0x00, 0xa4, 0x00, 0x00, 0xb8, 0xa4, 0xdf, 0x5e,
        0x9e, 0xb1, 0xbf, 0x85, 0x3d, 0x24, 0x2d, 0x1e,
        0x69, 0xb7, 0x67, 0x13, 0x8e, 0x78, 0xf2, 0xdf,
        0xc6, 0x69, 0xce, 0x46, 0x7e, 0xf2, 0xf2, 0x33,
        0x20, 0x6f, 0xa1, 0xa5, 0x59, 0x83, 0x25, 0xcb,
        0x3a, 0xb1, 0x8a, 0x12, 0x63, 0x02, 0x3c, 0xfb,
        0x4a, 0xfa, 0xef, 0x8e, 0xf7, 0x29, 0x57, 0xb1,
        0x9e, 0xa7, 0xf3, 0x02, 0xfd, 0xca, 0xdf, 0x5a,
        0x1f, 0x71, 0xb6, 0x26, 0x09, 0x24, 0x39, 0xda,
        0xc0, 0xde, 0x2a, 0x0e, 0xcd, 0x1f, 0xe5, 0xb6,
        0x4f, 0x82, 0xa0, 0xa9, 0x90, 0xd3, 0xd9, 0x6a,
        0x43, 0x14, 0x2a, 0xf7, 0xac, 0xd5, 0xa0, 0x54,
        0x93, 0xc4, 0xb9, 0xe7, 0x24, 0x84, 0x4d, 0x69,
        0x5e, 0xcc, 0x2a, 0x32, 0xb5, 0xfb, 0xe4, 0xb4,
        0x08, 0xd5, 0x36, 0x58, 0x59, 0x40, 0xfb, 0x29,
        0x7f, 0xa7, 0x17, 0x25, 0xc4, 0x0d, 0x78, 0x37,
        0x04, 0x8c, 0x49, 0x92
    };
    BYTE abEncryptedMessage[12] = {
        0x40, 0x64, 0x28, 0xe8, 0x8a, 0xe7, 0xa4, 0xd4,
        0x1c, 0xfd, 0xde, 0x71
    };
            
    dwLen = (DWORD)sizeof(abPlainPrivateKey);
    result = CryptImportKey(hProv, abPlainPrivateKey, dwLen, 0, 0, &hKeyExchangeKey);
    if (!result) {
        /* rsaenh compiled without OpenSSL */
        ok(GetLastError() == NTE_FAIL, "%08lx\n", GetLastError());
        return;
    }

    dwLen = (DWORD)sizeof(abSessionKey);
    result = CryptImportKey(hProv, abSessionKey, dwLen, hKeyExchangeKey, 0, &hSessionKey);
    ok(result, "%08lx\n", GetLastError());
    if (!result) return;

    dwLen = (DWORD)sizeof(abEncryptedMessage);
    result = CryptDecrypt(hSessionKey, 0, TRUE, 0, abEncryptedMessage, &dwLen);
    ok(result && dwLen == 12 && !strcmp(abEncryptedMessage, "Wine rocks!"), 
       "%08lx, len: %ld\n", GetLastError(), dwLen);
    
    if (!derive_key(CALG_RC4, &hSessionKey, 56)) return;

    dwLen = (DWORD)sizeof(abSessionKey);
    result = CryptExportKey(hSessionKey, hKeyExchangeKey, SIMPLEBLOB, 0, abSessionKey, &dwLen);
    ok(result, "%08lx\n", GetLastError());
    if (!result) return;

    dwLen = (DWORD)sizeof(abSessionKey);
    result = CryptImportKey(hProv, abSessionKey, dwLen, hKeyExchangeKey, 0, &hSessionKey);
    ok(result, "%08lx\n", GetLastError());
    if (!result) return;
}

START_TEST(rsaenh)
{
    if (!init_environment()) 
        return;
    test_prov();
    test_gen_random();
    test_hashes();
    test_rc4();
    test_rc2();
    test_des();
    test_3des112();
    test_3des();
    test_hmac();
    test_mac();
    test_block_cipher_modes();
    test_import_private();
    clean_up_environment();
}
