/*
 * dlls/rsaenh/implossl.c
 * Encapsulating the OpenSSL dependend parts of RSAENH
 *
 * Copyright (c) 2004 Michael Jung
 *
 * based on code by Mike McCormack and David Hammerton
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

#include "windef.h"
#include "wincrypt.h"

#include "implossl.h"

#include <stdio.h>

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

#ifndef SONAME_LIBCRYPTO
#define SONAME_LIBCRYPTO "libcrypto.so"
#endif

static void *libcrypto;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f

/* OpenSSL funtions that we use */
#ifdef HAVE_OPENSSL_MD2_H
MAKE_FUNCPTR(MD2_Init);
MAKE_FUNCPTR(MD2_Update);
MAKE_FUNCPTR(MD2_Final);
#endif
#ifdef HAVE_OPENSSL_RC2_H
MAKE_FUNCPTR(RC2_set_key);
MAKE_FUNCPTR(RC2_ecb_encrypt);
#endif
#ifdef HAVE_OPENSSL_RC4_H
MAKE_FUNCPTR(RC4_set_key);
MAKE_FUNCPTR(RC4);
#endif
#ifdef HAVE_OPENSSL_DES_H
MAKE_FUNCPTR(DES_set_odd_parity);
MAKE_FUNCPTR(DES_set_key_unchecked);
MAKE_FUNCPTR(DES_ecb_encrypt);
MAKE_FUNCPTR(DES_ecb3_encrypt);
#endif
#ifdef HAVE_OPENSSL_RSA_H
MAKE_FUNCPTR(RSA_generate_key);
MAKE_FUNCPTR(RSA_free);
MAKE_FUNCPTR(RSA_size);
MAKE_FUNCPTR(RSA_check_key);
MAKE_FUNCPTR(RSA_public_encrypt);
MAKE_FUNCPTR(RSA_private_encrypt);
MAKE_FUNCPTR(RSAPrivateKey_dup);
MAKE_FUNCPTR(BN_bn2bin);
MAKE_FUNCPTR(BN_bin2bn);
MAKE_FUNCPTR(BN_get_word);
MAKE_FUNCPTR(BN_set_word);
MAKE_FUNCPTR(BN_num_bits);
#endif

/* Function prototypes copied from dlls/advapi32/crypt_md4.c */
VOID WINAPI MD4Init( MD4_CTX *ctx );
VOID WINAPI MD4Update( MD4_CTX *ctx, const unsigned char *buf, unsigned int len );
VOID WINAPI MD4Final( MD4_CTX *ctx );
/* Function prototypes copied from dlls/advapi32/crypt_md5.c */
VOID WINAPI MD5Init( MD5_CTX *ctx );
VOID WINAPI MD5Update( MD5_CTX *ctx, const unsigned char *buf, unsigned int len );
VOID WINAPI MD5Final( MD5_CTX *ctx );
/* Function prototypes copied from dlls/advapi32/crypt_sha.c */
VOID WINAPI A_SHAInit(PSHA_CTX Context);
VOID WINAPI A_SHAUpdate(PSHA_CTX Context, PCHAR Buffer, UINT BufferSize);
VOID WINAPI A_SHAFinal(PSHA_CTX Context, PULONG Result);
        

BOOL load_lib( void )
{
/* FIXME: Is this portable? */
#if defined HAVE_OPENSSL_MD2_H || defined HAVE_OPENSSL_RC2_H || defined HAVE_OPENSSL_RC4_H || \
    defined HAVE_OPENSSL_DES_H || defined HAVE_OPENSSL_RSA_H
    libcrypto = wine_dlopen(SONAME_LIBCRYPTO, RTLD_NOW, NULL, 0);
    if (!libcrypto)
    {
        MESSAGE("Couldn't load %s, RSA encryption not available.\n", SONAME_LIBCRYPTO);
        MESSAGE("Install the openssl package if you're have problems.\n");
        SetLastError(NTE_PROVIDER_DLL_FAIL);
        return FALSE;
    }

#define GETFUNC(x) p##x = wine_dlsym(libcrypto, #x, NULL, 0);

#ifdef HAVE_OPENSSL_MD2_H
    GETFUNC(MD2_Init);
    GETFUNC(MD2_Update);
    GETFUNC(MD2_Final);
#endif
#ifdef HAVE_OPENSSL_RC2_H
    GETFUNC(RC2_set_key);
    GETFUNC(RC2_ecb_encrypt);
#endif
#ifdef HAVE_OPENSSL_RC4_H
    GETFUNC(RC4_set_key);
    GETFUNC(RC4);
#endif
#ifdef HAVE_OPENSSL_DES_H
    GETFUNC(DES_set_odd_parity);
    GETFUNC(DES_set_key_unchecked);
    GETFUNC(DES_ecb_encrypt);
    GETFUNC(DES_ecb3_encrypt);
#endif
#ifdef HAVE_OPENSSL_RSA_H
    GETFUNC(RSA_generate_key);
    GETFUNC(RSA_free);
    GETFUNC(RSA_size);
    GETFUNC(RSA_check_key);
    GETFUNC(RSA_public_encrypt);
    GETFUNC(RSA_private_encrypt);
    GETFUNC(RSAPrivateKey_dup);
    GETFUNC(BN_bn2bin);
    GETFUNC(BN_bin2bn);
    GETFUNC(BN_get_word);
    GETFUNC(BN_set_word);
    GETFUNC(BN_num_bits);
#endif
   
#endif /* ifdef have any openssl header */
    return TRUE;
}

BOOL init_hash_impl(ALG_ID aiAlgid, HASH_CONTEXT *pHashContext) 
{
    switch (aiAlgid) 
    {
#ifdef HAVE_OPENSSL_MD2_H
        case CALG_MD2:
            if (!pMD2_Init) 
            {
                SetLastError(NTE_PROVIDER_DLL_FAIL);
                return FALSE;
            }
            pMD2_Init(&pHashContext->md2);
            break;
#endif
        case CALG_MD4:
            MD4Init(&pHashContext->md4);
            break;
        
        case CALG_MD5:
            MD5Init(&pHashContext->md5);
            break;
        
        case CALG_SHA:
            A_SHAInit(&pHashContext->sha);
            break;
        
        default:
            SetLastError(NTE_BAD_ALGID);
            return FALSE;
    }

    return TRUE;
}

BOOL update_hash_impl(ALG_ID aiAlgid, HASH_CONTEXT *pHashContext, CONST BYTE *pbData, 
                      DWORD dwDataLen) 
{
    switch (aiAlgid)
    {
#ifdef HAVE_OPENSSL_MD2_H
        case CALG_MD2:
            if (!pMD2_Update)
            {
                SetLastError(NTE_PROVIDER_DLL_FAIL);
                return FALSE;
            }
            pMD2_Update(&pHashContext->md2, pbData, dwDataLen);
            break;
#endif        
        case CALG_MD4:
            MD4Update(&pHashContext->md4, pbData, dwDataLen);
            break;
    
        case CALG_MD5:
            MD5Update(&pHashContext->md5, pbData, dwDataLen);
            break;
        
        case CALG_SHA:
            A_SHAUpdate(&pHashContext->sha, (PCHAR)pbData, dwDataLen);
            break;
        
        default:
            SetLastError(NTE_BAD_ALGID);
            return FALSE;
    }

    return TRUE;
}

BOOL finalize_hash_impl(ALG_ID aiAlgid, HASH_CONTEXT *pHashContext, BYTE *pbHashValue) 
{
    switch (aiAlgid)
    {
#ifdef HAVE_OPENSSL_MD2_H
        case CALG_MD2:
            if (!pMD2_Final) 
            {
                SetLastError(NTE_PROVIDER_DLL_FAIL);
                return FALSE;
            }
            pMD2_Final(pbHashValue, &pHashContext->md2);
            break;
#endif
        case CALG_MD4:
            MD4Final(&pHashContext->md4);
            memcpy(pbHashValue, pHashContext->md4.digest, 16);
            break;
        
        case CALG_MD5:
            MD5Final(&pHashContext->md5);
            memcpy(pbHashValue, pHashContext->md5.digest, 16);
            break;
        
        case CALG_SHA:
            A_SHAFinal(&pHashContext->sha, (PULONG)pbHashValue);
            break;
        
        default:
            SetLastError(NTE_BAD_ALGID);
            return FALSE;
    }

    return TRUE;
}

BOOL duplicate_hash_impl(ALG_ID aiAlgid, CONST HASH_CONTEXT *pSrcHashContext, 
                         HASH_CONTEXT *pDestHashContext) 
{
    memcpy(pDestHashContext, pSrcHashContext, sizeof(HASH_CONTEXT));

    return TRUE;
}

BOOL new_key_impl(ALG_ID aiAlgid, KEY_CONTEXT *pKeyContext, DWORD dwKeyLen) 
{
    switch (aiAlgid)
    {
#ifdef HAVE_OPENSSL_RSA_H
        case CALG_RSA_KEYX:
        case CALG_RSA_SIGN:
            if (!pRSA_generate_key) 
            {
                SetLastError(NTE_PROVIDER_DLL_FAIL);
                return FALSE;
            }
            pKeyContext->rsa = pRSA_generate_key((int)dwKeyLen*8, 65537, NULL, NULL);
            break;
#endif  
        default:
            SetLastError(NTE_BAD_ALGID);
            return FALSE;
    }

    return TRUE;
}

BOOL free_key_impl(ALG_ID aiAlgid, KEY_CONTEXT *pKeyContext)
{
    switch (aiAlgid)
    {
#ifdef HAVE_OPENSSL_RSA_H
        case CALG_RSA_KEYX:
        case CALG_RSA_SIGN:
            if (!pRSA_free) 
            {
                SetLastError(NTE_PROVIDER_DLL_FAIL);
                return FALSE;
            }
            if (pKeyContext->rsa) pRSA_free(pKeyContext->rsa);
            break;
#endif  
        default:
            SetLastError(NTE_BAD_ALGID);
            return FALSE;
    }

    return TRUE;
}

BOOL setup_key_impl(ALG_ID aiAlgid, KEY_CONTEXT *pKeyContext, DWORD dwKeyLen, DWORD dwSaltLen, 
                    BYTE *abKeyValue) 
{
    switch (aiAlgid) 
    {
#ifdef HAVE_OPENSSL_RC4_H
        case CALG_RC4:
            if (!pRC4_set_key) 
            {
                SetLastError(NTE_PROVIDER_DLL_FAIL);
                return FALSE;
            }
            pRC4_set_key(&pKeyContext->rc4, dwKeyLen + dwSaltLen, abKeyValue);
            break;
#endif
#ifdef HAVE_OPENSSL_RC2_H
        case CALG_RC2:
            if (!pRC2_set_key)
            {
                SetLastError(NTE_PROVIDER_DLL_FAIL);
                return FALSE;
            }
            pRC2_set_key(&pKeyContext->rc2, dwKeyLen + dwSaltLen, abKeyValue, dwKeyLen * 8);
            break;
#endif
#ifdef HAVE_OPENSSL_DES_H
        case CALG_3DES:
            if (!pDES_set_odd_parity || !pDES_set_key_unchecked)
            {
                SetLastError(NTE_PROVIDER_DLL_FAIL);
                return FALSE;
            }
            pDES_set_odd_parity(&((DES_cblock*)abKeyValue)[2]);
            pDES_set_key_unchecked(&((DES_cblock*)abKeyValue)[2], &pKeyContext->des[2]);

        case CALG_3DES_112:
            if (!pDES_set_odd_parity || !pDES_set_key_unchecked)
            {
                SetLastError(NTE_PROVIDER_DLL_FAIL);
                return FALSE;
            }
            pDES_set_odd_parity(&((DES_cblock*)abKeyValue)[1]);
            pDES_set_key_unchecked(&((DES_cblock*)abKeyValue)[1], &pKeyContext->des[1]);

        case CALG_DES:
            if (!pDES_set_odd_parity || !pDES_set_key_unchecked)
            {
                SetLastError(NTE_PROVIDER_DLL_FAIL);
                return FALSE;
            }
            pDES_set_odd_parity((DES_cblock*)abKeyValue);
            pDES_set_key_unchecked((DES_cblock*)abKeyValue, &pKeyContext->des[0]);
            break;
#endif  
        default:
            SetLastError(NTE_BAD_ALGID);
            return FALSE;
    }

    return TRUE;
}

BOOL duplicate_key_impl(ALG_ID aiAlgid, CONST KEY_CONTEXT *pSrcKeyContext,
                        KEY_CONTEXT *pDestKeyContext) 
{
    switch (aiAlgid) 
    {
        case CALG_RC4:
        case CALG_RC2:
        case CALG_3DES:
        case CALG_3DES_112:
        case CALG_DES:
            memcpy(pDestKeyContext, pSrcKeyContext, sizeof(KEY_CONTEXT));
            break;
#ifdef HAVE_OPENSSL_RSA_H
        case CALG_RSA_KEYX:
        case CALG_RSA_SIGN:
            if (!pRSAPrivateKey_dup)
            {
                SetLastError(NTE_PROVIDER_DLL_FAIL);
                return FALSE;
            }
            pDestKeyContext->rsa = pRSAPrivateKey_dup(pSrcKeyContext->rsa);
            break;
#endif  
        default:
            SetLastError(NTE_BAD_ALGID);
            return FALSE;
    }

    return TRUE;
}

#ifdef HAVE_OPENSSL_RSA_H
static inline void reverse_bytes(BYTE *pbData, DWORD dwLen) {
    BYTE swap;
    DWORD i;

    for (i=0; i<dwLen/2; i++) {
        swap = pbData[i];
        pbData[i] = pbData[dwLen-i-1];
        pbData[dwLen-i-1] = swap;
    }
}
#endif

BOOL encrypt_block_impl(ALG_ID aiAlgid, KEY_CONTEXT *pKeyContext, CONST BYTE *in, BYTE *out, 
                        DWORD enc) 
{
#ifdef HAVE_OPENSSL_RSA_H
    int cLen;
#endif
        
    switch (aiAlgid) {
#ifdef HAVE_OPENSSL_RC2_H
        case CALG_RC2:
            if (!pRC2_ecb_encrypt)
            {
                SetLastError(NTE_PROVIDER_DLL_FAIL);
                return FALSE;
            }
            pRC2_ecb_encrypt(in, out, &pKeyContext->rc2, enc ? RC2_ENCRYPT : RC2_DECRYPT);
            break;
#endif
#ifdef HAVE_OPENSSL_DES_H
        case CALG_DES:
            if (!pDES_ecb_encrypt)
            {
                SetLastError(NTE_PROVIDER_DLL_FAIL);
                return FALSE;
            }
            pDES_ecb_encrypt((const_DES_cblock*)in, (DES_cblock*)out, &pKeyContext->des[0], 
                             enc ? DES_ENCRYPT : DES_DECRYPT);
            break;
        
        case CALG_3DES_112:
            if (!pDES_ecb3_encrypt)
            {
                SetLastError(NTE_PROVIDER_DLL_FAIL);
                return FALSE;
            }
            pDES_ecb3_encrypt((const_DES_cblock*)in, (DES_cblock*)out, 
                              &pKeyContext->des[0], &pKeyContext->des[1], &pKeyContext->des[0], 
                              enc ? DES_ENCRYPT : DES_DECRYPT);
            break;
        
        case CALG_3DES:
            if (!pDES_ecb3_encrypt)
            {
                SetLastError(NTE_PROVIDER_DLL_FAIL);
                return FALSE;
            }
            pDES_ecb3_encrypt((const_DES_cblock*)in, (DES_cblock*)out, 
                              &pKeyContext->des[0], &pKeyContext->des[1], &pKeyContext->des[2], 
                              enc ? DES_ENCRYPT : DES_DECRYPT);
            break;
#endif 
#ifdef HAVE_OPENSSL_RSA_H
        case CALG_RSA_KEYX:
            if (!pBN_num_bits || !pRSA_public_encrypt || !pRSA_private_encrypt)
            {
                SetLastError(NTE_PROVIDER_DLL_FAIL);
                return FALSE;
            }
            cLen = pBN_num_bits(pKeyContext->rsa->n)/8;
            if (enc) {
                pRSA_public_encrypt(cLen, in, out, pKeyContext->rsa, RSA_NO_PADDING);
                reverse_bytes((BYTE*)in, cLen);
            } else {
                reverse_bytes((BYTE*)in, cLen);
                pRSA_private_encrypt(cLen, in, out, pKeyContext->rsa, RSA_NO_PADDING);
            }
            break;    
        
        case CALG_RSA_SIGN:
            if (!pBN_num_bits || !pRSA_public_encrypt || !pRSA_private_encrypt)
            {
                SetLastError(NTE_PROVIDER_DLL_FAIL);
                return FALSE;
            }
            cLen = pBN_num_bits(pKeyContext->rsa->n)/8;
            if (enc) {
                pRSA_private_encrypt(cLen, in, out, pKeyContext->rsa, RSA_NO_PADDING);
                reverse_bytes((BYTE*)in, cLen);
            } else {
                reverse_bytes((BYTE*)in, cLen);
                pRSA_public_encrypt(cLen, in, out, pKeyContext->rsa, RSA_NO_PADDING);
            }
            break;
#endif
        default:
            SetLastError(NTE_BAD_ALGID);
            return FALSE;
    }

    return TRUE;
}

BOOL encrypt_stream_impl(ALG_ID aiAlgid, KEY_CONTEXT *pKeyContext, BYTE *stream, DWORD dwLen)
{
    switch (aiAlgid) {
#ifdef HAVE_OPENSSL_RC4_H
        case CALG_RC4:
            if (!pRC4) 
            {
                SetLastError(NTE_PROVIDER_DLL_FAIL);
                return FALSE;
            }
            pRC4(&pKeyContext->rc4, (unsigned long)dwLen, stream, stream);
            break;
#endif 
        default:
            SetLastError(NTE_BAD_ALGID);
            return FALSE;
    }

    return TRUE;
}

BOOL gen_rand_impl(BYTE *pbBuffer, DWORD dwLen)
{
    FILE *dev_random;

    /* FIXME: /dev/urandom does not provide random numbers of a sufficient
     * quality for cryptographic applications. /dev/random is much better,  
     * but it blocks if the kernel has not yet collected enough entropy for
     * the request, which will suspend the calling thread for an indefinite
     * amount of time. */
    dev_random = fopen("/dev/urandom", "r");
    if (dev_random) 
    {
        if (fread(pbBuffer, (size_t)dwLen, 1, dev_random) == 1)
        {
            fclose(dev_random);
            return TRUE;
        }
        fclose(dev_random);
    }
    SetLastError(NTE_FAIL);
    return FALSE;
}

BOOL export_public_key_impl(BYTE *pbDest, KEY_CONTEXT *pKeyContext, DWORD dwKeyLen,DWORD *pdwPubExp)
{
#ifdef HAVE_OPENSSL_RSA_H
    if (!pBN_bn2bin || !pBN_get_word)
    {
        SetLastError(NTE_PROVIDER_DLL_FAIL);
        return FALSE;
    }

    pBN_bn2bin(pKeyContext->rsa->n, pbDest);
    reverse_bytes(pbDest, dwKeyLen);
    *pdwPubExp = (DWORD)pBN_get_word(pKeyContext->rsa->e);

    return TRUE;
#else
    SetLastError(NTE_FAIL);
    return FALSE;
#endif
}

BOOL import_public_key_impl(CONST BYTE *pbSrc, KEY_CONTEXT *pKeyContext, DWORD dwKeyLen, 
                            DWORD dwPubExp)
{
#ifdef HAVE_OPENSSL_RSA_H
    BYTE *pbTemp;

    if (!pBN_bin2bn || !pBN_set_word)
    {
        SetLastError(NTE_PROVIDER_DLL_FAIL);
        return FALSE;
    }

    pbTemp = (BYTE*)HeapAlloc(GetProcessHeap(), 0, dwKeyLen);
    if (!pbTemp) return FALSE;
    memcpy(pbTemp, pbSrc, dwKeyLen);
    reverse_bytes(pbTemp, dwKeyLen);
    pBN_bin2bn(pbTemp, dwKeyLen, pKeyContext->rsa->n);
    HeapFree(GetProcessHeap(), 0, pbTemp);
    
    pBN_set_word(pKeyContext->rsa->e, (BN_ULONG)dwPubExp);

    return TRUE;
#else
    SetLastError(NTE_FAIL);
    return FALSE;
#endif
}

BOOL export_private_key_impl(BYTE *pbDest, KEY_CONTEXT *pKeyContext, DWORD dwKeyLen, 
                             DWORD *pdwPubExp)
{
#ifdef HAVE_OPENSSL_RSA_H
    if (!pBN_bn2bin || !pBN_get_word)
    {
        SetLastError(NTE_PROVIDER_DLL_FAIL);
        return FALSE;
    }
    
    pBN_bn2bin(pKeyContext->rsa->n, pbDest);
    reverse_bytes(pbDest, dwKeyLen);
    pbDest += dwKeyLen;
    pBN_bn2bin(pKeyContext->rsa->p, pbDest);
    reverse_bytes(pbDest, (dwKeyLen+1)>>1);
    pbDest += (dwKeyLen+1)>>1;
    pBN_bn2bin(pKeyContext->rsa->q, pbDest);
    reverse_bytes(pbDest, (dwKeyLen+1)>>1);
    pbDest += (dwKeyLen+1)>>1;
    pBN_bn2bin(pKeyContext->rsa->dmp1, pbDest);
    reverse_bytes(pbDest, (dwKeyLen+1)>>1);
    pbDest += (dwKeyLen+1)>>1;
    pBN_bn2bin(pKeyContext->rsa->dmq1, pbDest);
    reverse_bytes(pbDest, (dwKeyLen+1)>>1);
    pbDest += (dwKeyLen+1)>>1;
    pBN_bn2bin(pKeyContext->rsa->iqmp, pbDest);
    reverse_bytes(pbDest, (dwKeyLen+1)>>1);
    pbDest += (dwKeyLen+1)>>1;
    pBN_bn2bin(pKeyContext->rsa->d, pbDest);
    reverse_bytes(pbDest, dwKeyLen);
    *pdwPubExp = (DWORD)pBN_get_word(pKeyContext->rsa->e);

    return TRUE;
#else
    SetLastError(NTE_FAIL);
    return FALSE;
#endif
}

BOOL import_private_key_impl(CONST BYTE *pbSrc, KEY_CONTEXT *pKeyContext, DWORD dwKeyLen, 
                             DWORD dwPubExp)
{
#ifdef HAVE_OPENSSL_RSA_H
    BYTE *pbTemp, *pbBigNum;
        
    if (!pBN_bin2bn || !pBN_set_word)
    {
        SetLastError(NTE_PROVIDER_DLL_FAIL);
        return FALSE;
    }

    pbTemp = HeapAlloc(GetProcessHeap(), 0, 2*dwKeyLen+5*((dwKeyLen+1)>>1));
    if (!pbTemp) return FALSE;
    memcpy(pbTemp, pbSrc, 2*dwKeyLen+5*((dwKeyLen+1)>>1));
    pbBigNum = pbTemp;
    
    reverse_bytes(pbBigNum, dwKeyLen);
    pBN_bin2bn(pbBigNum, dwKeyLen, pKeyContext->rsa->n);
    pbBigNum += dwKeyLen;
    reverse_bytes(pbBigNum, (dwKeyLen+1)>>1);
    pBN_bin2bn(pbBigNum, (dwKeyLen+1)>>1, pKeyContext->rsa->p);
    pbBigNum += (dwKeyLen+1)>>1;
    reverse_bytes(pbBigNum, (dwKeyLen+1)>>1);
    pBN_bin2bn(pbBigNum, (dwKeyLen+1)>>1, pKeyContext->rsa->q);
    pbBigNum += (dwKeyLen+1)>>1;
    reverse_bytes(pbBigNum, (dwKeyLen+1)>>1);
    pBN_bin2bn(pbBigNum, (dwKeyLen+1)>>1, pKeyContext->rsa->dmp1);
    pbBigNum += (dwKeyLen+1)>>1;
    reverse_bytes(pbBigNum, (dwKeyLen+1)>>1);
    pBN_bin2bn(pbBigNum, (dwKeyLen+1)>>1, pKeyContext->rsa->dmq1);
    pbBigNum += (dwKeyLen+1)>>1;
    reverse_bytes(pbBigNum, (dwKeyLen+1)>>1);
    pBN_bin2bn(pbBigNum, (dwKeyLen+1)>>1, pKeyContext->rsa->iqmp);
    pbBigNum += (dwKeyLen+1)>>1;
    reverse_bytes(pbBigNum, dwKeyLen);
    pBN_bin2bn(pbBigNum, dwKeyLen, pKeyContext->rsa->d);
    pBN_set_word(pKeyContext->rsa->e, (BN_ULONG)dwPubExp);

    HeapFree(GetProcessHeap(), 0, pbTemp);

    return TRUE;
#else
    SetLastError(NTE_FAIL);
    return FALSE;
#endif
}
