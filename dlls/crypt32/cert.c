/*
 * Copyright 2004-2006 Juan Lang
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
 *
 */

#include <assert.h>
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "winnls.h"
#include "rpc.h"
#include "wine/debug.h"
#include "crypt32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

PCRYPT_ATTRIBUTE WINAPI CertFindAttribute(LPCSTR pszObjId, DWORD cAttr,
 CRYPT_ATTRIBUTE rgAttr[])
{
    PCRYPT_ATTRIBUTE ret = NULL;
    DWORD i;

    TRACE("%s %ld %p\n", debugstr_a(pszObjId), cAttr, rgAttr);

    if (!cAttr)
        return NULL;
    if (!pszObjId)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    for (i = 0; !ret && i < cAttr; i++)
        if (rgAttr[i].pszObjId && !strcmp(pszObjId, rgAttr[i].pszObjId))
            ret = &rgAttr[i];
    return ret;
}

PCERT_EXTENSION WINAPI CertFindExtension(LPCSTR pszObjId, DWORD cExtensions,
 CERT_EXTENSION rgExtensions[])
{
    PCERT_EXTENSION ret = NULL;
    DWORD i;

    TRACE("%s %ld %p\n", debugstr_a(pszObjId), cExtensions, rgExtensions);

    if (!cExtensions)
        return NULL;
    if (!pszObjId)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    for (i = 0; !ret && i < cExtensions; i++)
        if (rgExtensions[i].pszObjId && !strcmp(pszObjId,
         rgExtensions[i].pszObjId))
            ret = &rgExtensions[i];
    return ret;
}

PCERT_RDN_ATTR WINAPI CertFindRDNAttr(LPCSTR pszObjId, PCERT_NAME_INFO pName)
{
    PCERT_RDN_ATTR ret = NULL;
    DWORD i, j;

    TRACE("%s %p\n", debugstr_a(pszObjId), pName);

    if (!pszObjId)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    for (i = 0; !ret && i < pName->cRDN; i++)
        for (j = 0; !ret && j < pName->rgRDN[i].cRDNAttr; j++)
            if (pName->rgRDN[i].rgRDNAttr[j].pszObjId && !strcmp(pszObjId,
             pName->rgRDN[i].rgRDNAttr[j].pszObjId))
                ret = &pName->rgRDN[i].rgRDNAttr[j];
    return ret;
}

LONG WINAPI CertVerifyTimeValidity(LPFILETIME pTimeToVerify,
 PCERT_INFO pCertInfo)
{
    FILETIME fileTime;
    LONG ret;

    if (!pTimeToVerify)
    {
        SYSTEMTIME sysTime;

        GetSystemTime(&sysTime);
        SystemTimeToFileTime(&sysTime, &fileTime);
        pTimeToVerify = &fileTime;
    }
    if ((ret = CompareFileTime(pTimeToVerify, &pCertInfo->NotBefore)) >= 0)
    {
        ret = CompareFileTime(pTimeToVerify, &pCertInfo->NotAfter);
        if (ret < 0)
            ret = 0;
    }
    return ret;
}

BOOL WINAPI CryptHashCertificate(HCRYPTPROV hCryptProv, ALG_ID Algid,
 DWORD dwFlags, const BYTE *pbEncoded, DWORD cbEncoded, BYTE *pbComputedHash,
 DWORD *pcbComputedHash)
{
    BOOL ret = TRUE;
    HCRYPTHASH hHash = 0;

    TRACE("(%ld, %d, %08lx, %p, %ld, %p, %p)\n", hCryptProv, Algid, dwFlags,
     pbEncoded, cbEncoded, pbComputedHash, pcbComputedHash);

    if (!hCryptProv)
        hCryptProv = CRYPT_GetDefaultProvider();
    if (!Algid)
        Algid = CALG_SHA1;
    if (ret)
    {
        ret = CryptCreateHash(hCryptProv, Algid, 0, 0, &hHash);
        if (ret)
        {
            ret = CryptHashData(hHash, pbEncoded, cbEncoded, 0);
            if (ret)
                ret = CryptGetHashParam(hHash, HP_HASHVAL, pbComputedHash,
                 pcbComputedHash, 0);
            CryptDestroyHash(hHash);
        }
    }
    return ret;
}

BOOL WINAPI CryptSignCertificate(HCRYPTPROV hCryptProv, DWORD dwKeySpec,
 DWORD dwCertEncodingType, const BYTE *pbEncodedToBeSigned,
 DWORD cbEncodedToBeSigned, PCRYPT_ALGORITHM_IDENTIFIER pSignatureAlgorithm,
 const void *pvHashAuxInfo, BYTE *pbSignature, DWORD *pcbSignature)
{
    BOOL ret;
    ALG_ID algID;
    HCRYPTHASH hHash;

    TRACE("(%08lx, %ld, %ld, %p, %ld, %p, %p, %p, %p)\n", hCryptProv,
     dwKeySpec, dwCertEncodingType, pbEncodedToBeSigned, cbEncodedToBeSigned,
     pSignatureAlgorithm, pvHashAuxInfo, pbSignature, pcbSignature);

    algID = CertOIDToAlgId(pSignatureAlgorithm->pszObjId);
    if (!algID)
    {
        SetLastError(NTE_BAD_ALGID);
        return FALSE;
    }
    if (!hCryptProv)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ret = CryptCreateHash(hCryptProv, algID, 0, 0, &hHash);
    if (ret)
    {
        ret = CryptHashData(hHash, pbEncodedToBeSigned, cbEncodedToBeSigned, 0);
        if (ret)
            ret = CryptSignHashW(hHash, dwKeySpec, NULL, 0, pbSignature,
             pcbSignature);
        CryptDestroyHash(hHash);
    }
    return ret;
}

BOOL WINAPI CryptVerifyCertificateSignature(HCRYPTPROV hCryptProv,
 DWORD dwCertEncodingType, const BYTE *pbEncoded, DWORD cbEncoded,
 PCERT_PUBLIC_KEY_INFO pPublicKey)
{
    return CryptVerifyCertificateSignatureEx(hCryptProv, dwCertEncodingType,
     CRYPT_VERIFY_CERT_SIGN_SUBJECT_BLOB, (void *)pbEncoded,
     CRYPT_VERIFY_CERT_SIGN_ISSUER_PUBKEY, pPublicKey, 0, NULL);
}

static BOOL CRYPT_VerifyCertSignatureFromPublicKeyInfo(HCRYPTPROV hCryptProv,
 DWORD dwCertEncodingType, PCERT_PUBLIC_KEY_INFO pubKeyInfo,
 PCERT_SIGNED_CONTENT_INFO signedCert)
{
    BOOL ret;
    ALG_ID algID = CertOIDToAlgId(pubKeyInfo->Algorithm.pszObjId);
    HCRYPTKEY key;

    /* Load the default provider if necessary */
    if (!hCryptProv)
        hCryptProv = CRYPT_GetDefaultProvider();
    ret = CryptImportPublicKeyInfoEx(hCryptProv, dwCertEncodingType,
     pubKeyInfo, algID, 0, NULL, &key);
    if (ret)
    {
        HCRYPTHASH hash;

        /* Some key algorithms aren't hash algorithms, so map them */
        if (algID == CALG_RSA_SIGN || algID == CALG_RSA_KEYX)
            algID = CALG_SHA1;
        ret = CryptCreateHash(hCryptProv, algID, 0, 0, &hash);
        if (ret)
        {
            ret = CryptHashData(hash, signedCert->ToBeSigned.pbData,
             signedCert->ToBeSigned.cbData, 0);
            if (ret)
                ret = CryptVerifySignatureW(hash, signedCert->Signature.pbData,
                 signedCert->Signature.cbData, key, NULL, 0);
            CryptDestroyHash(hash);
        }
        CryptDestroyKey(key);
    }
    return ret;
}

BOOL WINAPI CryptVerifyCertificateSignatureEx(HCRYPTPROV hCryptProv,
 DWORD dwCertEncodingType, DWORD dwSubjectType, void *pvSubject,
 DWORD dwIssuerType, void *pvIssuer, DWORD dwFlags, void *pvReserved)
{
    BOOL ret = TRUE;
    CRYPT_DATA_BLOB subjectBlob;

    TRACE("(%08lx, %ld, %ld, %p, %ld, %p, %08lx, %p)\n", hCryptProv,
     dwCertEncodingType, dwSubjectType, pvSubject, dwIssuerType, pvIssuer,
     dwFlags, pvReserved);

    switch (dwSubjectType)
    {
    case CRYPT_VERIFY_CERT_SIGN_SUBJECT_BLOB:
    {
        PCRYPT_DATA_BLOB blob = (PCRYPT_DATA_BLOB)pvSubject;

        subjectBlob.pbData = blob->pbData;
        subjectBlob.cbData = blob->cbData;
        break;
    }
    case CRYPT_VERIFY_CERT_SIGN_SUBJECT_CERT:
    {
        PCERT_CONTEXT context = (PCERT_CONTEXT)pvSubject;

        subjectBlob.pbData = context->pbCertEncoded;
        subjectBlob.cbData = context->cbCertEncoded;
        break;
    }
    case CRYPT_VERIFY_CERT_SIGN_SUBJECT_CRL:
    {
        PCRL_CONTEXT context = (PCRL_CONTEXT)pvSubject;

        subjectBlob.pbData = context->pbCrlEncoded;
        subjectBlob.cbData = context->cbCrlEncoded;
        break;
    }
    default:
        SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
        ret = FALSE;
    }

    if (ret)
    {
        PCERT_SIGNED_CONTENT_INFO signedCert = NULL;
        DWORD size = 0;

        ret = CryptDecodeObjectEx(dwCertEncodingType, X509_CERT,
         subjectBlob.pbData, subjectBlob.cbData,
         CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG, NULL,
         (BYTE *)&signedCert, &size);
        if (ret)
        {
            switch (dwIssuerType)
            {
            case CRYPT_VERIFY_CERT_SIGN_ISSUER_PUBKEY:
                ret = CRYPT_VerifyCertSignatureFromPublicKeyInfo(hCryptProv,
                 dwCertEncodingType, (PCERT_PUBLIC_KEY_INFO)pvIssuer,
                 signedCert);
                break;
            case CRYPT_VERIFY_CERT_SIGN_ISSUER_CERT:
                ret = CRYPT_VerifyCertSignatureFromPublicKeyInfo(hCryptProv,
                 dwCertEncodingType,
                 &((PCCERT_CONTEXT)pvIssuer)->pCertInfo->SubjectPublicKeyInfo,
                 signedCert);
                break;
            case CRYPT_VERIFY_CERT_SIGN_ISSUER_CHAIN:
                FIXME("CRYPT_VERIFY_CERT_SIGN_ISSUER_CHAIN: stub\n");
                ret = FALSE;
                break;
            case CRYPT_VERIFY_CERT_SIGN_ISSUER_NULL:
                if (pvIssuer)
                {
                    SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
                    ret = FALSE;
                }
                else
                {
                    FIXME("unimplemented for NULL signer\n");
                    SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
                    ret = FALSE;
                }
                break;
            default:
                SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
                ret = FALSE;
            }
            LocalFree(signedCert);
        }
    }
    return ret;
}

BOOL WINAPI CertGetEnhancedKeyUsage(PCCERT_CONTEXT pCertContext, DWORD dwFlags,
 PCERT_ENHKEY_USAGE pUsage, DWORD *pcbUsage)
{
    PCERT_ENHKEY_USAGE usage = NULL;
    DWORD bytesNeeded;
    BOOL ret = TRUE;

    if (!pCertContext || !pcbUsage)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    TRACE("(%p, %08lx, %p, %ld)\n", pCertContext, dwFlags, pUsage, *pcbUsage);

    if (!(dwFlags & CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG))
    {
        DWORD propSize = 0;

        if (CertGetCertificateContextProperty(pCertContext,
         CERT_ENHKEY_USAGE_PROP_ID, NULL, &propSize))
        {
            LPBYTE buf = CryptMemAlloc(propSize);

            if (buf)
            {
                if (CertGetCertificateContextProperty(pCertContext,
                 CERT_ENHKEY_USAGE_PROP_ID, buf, &propSize))
                {
                    ret = CryptDecodeObjectEx(pCertContext->dwCertEncodingType,
                     X509_ENHANCED_KEY_USAGE, buf, propSize,
                     CRYPT_ENCODE_ALLOC_FLAG, NULL, &usage, &bytesNeeded);
                }
                CryptMemFree(buf);
            }
        }
    }
    if (!usage && !(dwFlags & CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG))
    {
        PCERT_EXTENSION ext = CertFindExtension(szOID_ENHANCED_KEY_USAGE,
         pCertContext->pCertInfo->cExtension,
         pCertContext->pCertInfo->rgExtension);

        if (ext)
        {
            ret = CryptDecodeObjectEx(pCertContext->dwCertEncodingType,
             X509_ENHANCED_KEY_USAGE, ext->Value.pbData, ext->Value.cbData,
             CRYPT_ENCODE_ALLOC_FLAG, NULL, &usage, &bytesNeeded);
        }
    }
    if (!usage)
    {
        /* If a particular location is specified, this should fail.  Otherwise
         * it should succeed with an empty usage.  (This is true on Win2k and
         * later, which we emulate.)
         */
        if (dwFlags)
        {
            SetLastError(CRYPT_E_NOT_FOUND);
            ret = FALSE;
        }
        else
            bytesNeeded = sizeof(CERT_ENHKEY_USAGE);
    }

    if (ret)
    {
        if (!pUsage)
            *pcbUsage = bytesNeeded;
        else if (*pcbUsage < bytesNeeded)
        {
            SetLastError(ERROR_MORE_DATA);
            *pcbUsage = bytesNeeded;
            ret = FALSE;
        }
        else
        {
            *pcbUsage = bytesNeeded;
            if (usage)
            {
                DWORD i;
                LPSTR nextOID = (LPSTR)((LPBYTE)pUsage +
                 sizeof(CERT_ENHKEY_USAGE) +
                 usage->cUsageIdentifier * sizeof(LPSTR));

                pUsage->cUsageIdentifier = usage->cUsageIdentifier;
                pUsage->rgpszUsageIdentifier = (LPSTR *)((LPBYTE)pUsage +
                 sizeof(CERT_ENHKEY_USAGE));
                for (i = 0; i < usage->cUsageIdentifier; i++)
                {
                    pUsage->rgpszUsageIdentifier[i] = nextOID;
                    strcpy(nextOID, usage->rgpszUsageIdentifier[i]);
                    nextOID += strlen(nextOID) + 1;
                }
            }
            else
                pUsage->cUsageIdentifier = 0;
        }
    }
    if (usage)
        LocalFree(usage);
    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI CertSetEnhancedKeyUsage(PCCERT_CONTEXT pCertContext,
 PCERT_ENHKEY_USAGE pUsage)
{
    BOOL ret;

    TRACE("(%p, %p)\n", pCertContext, pUsage);

    if (pUsage)
    {
        CRYPT_DATA_BLOB blob = { 0, NULL };

        ret = CryptEncodeObjectEx(X509_ASN_ENCODING, X509_ENHANCED_KEY_USAGE,
         pUsage, CRYPT_ENCODE_ALLOC_FLAG, NULL, &blob.pbData, &blob.cbData);
        if (ret)
        {
            ret = CertSetCertificateContextProperty(pCertContext,
             CERT_ENHKEY_USAGE_PROP_ID, 0, &blob);
            LocalFree(blob.pbData);
        }
    }
    else
        ret = CertSetCertificateContextProperty(pCertContext,
         CERT_ENHKEY_USAGE_PROP_ID, 0, NULL);
    return ret;
}

BOOL WINAPI CertAddEnhancedKeyUsageIdentifier(PCCERT_CONTEXT pCertContext,
 LPCSTR pszUsageIdentifier)
{
    BOOL ret;
    DWORD size;

    TRACE("(%p, %s)\n", pCertContext, debugstr_a(pszUsageIdentifier));

    if (CertGetEnhancedKeyUsage(pCertContext,
     CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG, NULL, &size))
    {
        PCERT_ENHKEY_USAGE usage = CryptMemAlloc(size);

        if (usage)
        {
            ret = CertGetEnhancedKeyUsage(pCertContext,
             CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG, usage, &size);
            if (ret)
            {
                PCERT_ENHKEY_USAGE newUsage = CryptMemAlloc(size +
                 sizeof(LPSTR) + strlen(pszUsageIdentifier) + 1);

                if (newUsage)
                {
                    LPSTR nextOID;
                    DWORD i;

                    newUsage->rgpszUsageIdentifier =
                     (LPSTR *)((LPBYTE)newUsage + sizeof(CERT_ENHKEY_USAGE));
                    nextOID = (LPSTR)((LPBYTE)newUsage->rgpszUsageIdentifier +
                     (usage->cUsageIdentifier + 1) * sizeof(LPSTR));
                    for (i = 0; i < usage->cUsageIdentifier; i++)
                    {
                        newUsage->rgpszUsageIdentifier[i] = nextOID;
                        strcpy(nextOID, usage->rgpszUsageIdentifier[i]);
                        nextOID += strlen(nextOID) + 1;
                    }
                    newUsage->rgpszUsageIdentifier[i] = nextOID;
                    strcpy(nextOID, pszUsageIdentifier);
                    newUsage->cUsageIdentifier = i + 1;
                    ret = CertSetEnhancedKeyUsage(pCertContext, newUsage);
                    CryptMemFree(newUsage);
                }
            }
            CryptMemFree(usage);
        }
        else
            ret = FALSE;
    }
    else
    {
        PCERT_ENHKEY_USAGE usage = CryptMemAlloc(sizeof(CERT_ENHKEY_USAGE) +
         sizeof(LPSTR) + strlen(pszUsageIdentifier) + 1);

        if (usage)
        {
            usage->rgpszUsageIdentifier =
             (LPSTR *)((LPBYTE)usage + sizeof(CERT_ENHKEY_USAGE));
            usage->rgpszUsageIdentifier[0] = (LPSTR)((LPBYTE)usage +
             sizeof(CERT_ENHKEY_USAGE) + sizeof(LPSTR));
            strcpy(usage->rgpszUsageIdentifier[0], pszUsageIdentifier);
            usage->cUsageIdentifier = 1;
            ret = CertSetEnhancedKeyUsage(pCertContext, usage);
            CryptMemFree(usage);
        }
        else
            ret = FALSE;
    }
    return ret;
}

BOOL WINAPI CertRemoveEnhancedKeyUsageIdentifier(PCCERT_CONTEXT pCertContext,
 LPCSTR pszUsageIdentifier)
{
    BOOL ret;
    DWORD size;
    CERT_ENHKEY_USAGE usage;

    TRACE("(%p, %s)\n", pCertContext, debugstr_a(pszUsageIdentifier));

    size = sizeof(usage);
    ret = CertGetEnhancedKeyUsage(pCertContext,
     CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG, &usage, &size);
    if (!ret && GetLastError() == ERROR_MORE_DATA)
    {
        PCERT_ENHKEY_USAGE pUsage = CryptMemAlloc(size);

        if (pUsage)
        {
            ret = CertGetEnhancedKeyUsage(pCertContext,
             CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG, pUsage, &size);
            if (ret)
            {
                if (pUsage->cUsageIdentifier)
                {
                    DWORD i;
                    BOOL found = FALSE;

                    for (i = 0; i < pUsage->cUsageIdentifier; i++)
                    {
                        if (!strcmp(pUsage->rgpszUsageIdentifier[i],
                         pszUsageIdentifier))
                            found = TRUE;
                        if (found && i < pUsage->cUsageIdentifier - 1)
                            pUsage->rgpszUsageIdentifier[i] =
                             pUsage->rgpszUsageIdentifier[i + 1];
                    }
                    pUsage->cUsageIdentifier--;
                    /* Remove the usage if it's empty */
                    if (pUsage->cUsageIdentifier)
                        ret = CertSetEnhancedKeyUsage(pCertContext, pUsage);
                    else
                        ret = CertSetEnhancedKeyUsage(pCertContext, NULL);
                }
            }
            CryptMemFree(pUsage);
        }
        else
            ret = FALSE;
    }
    else
    {
        /* it fit in an empty usage, therefore there's nothing to remove */
        ret = TRUE;
    }
    return ret;
}

BOOL WINAPI CertGetValidUsages(DWORD cCerts, PCCERT_CONTEXT *rghCerts,
 int *cNumOIDSs, LPSTR *rghOIDs, DWORD *pcbOIDs)
{
    BOOL ret = TRUE;
    DWORD i, cbOIDs = 0;
    BOOL allUsagesValid = TRUE;
    CERT_ENHKEY_USAGE validUsages = { 0, NULL };

    TRACE("(%ld, %p, %p, %p, %ld)\n", cCerts, *rghCerts, cNumOIDSs,
     rghOIDs, *pcbOIDs);

    for (i = 0; ret && i < cCerts; i++)
    {
        CERT_ENHKEY_USAGE usage;
        DWORD size = sizeof(usage);

        ret = CertGetEnhancedKeyUsage(rghCerts[i], 0, &usage, &size);
        /* Success is deliberately ignored: it implies all usages are valid */
        if (!ret && GetLastError() == ERROR_MORE_DATA)
        {
            PCERT_ENHKEY_USAGE pUsage = CryptMemAlloc(size);

            allUsagesValid = FALSE;
            if (pUsage)
            {
                ret = CertGetEnhancedKeyUsage(rghCerts[i], 0, pUsage, &size);
                if (ret)
                {
                    if (!validUsages.cUsageIdentifier)
                    {
                        DWORD j;

                        cbOIDs = pUsage->cUsageIdentifier * sizeof(LPSTR);
                        validUsages.cUsageIdentifier = pUsage->cUsageIdentifier;
                        for (j = 0; j < validUsages.cUsageIdentifier; j++)
                            cbOIDs += lstrlenA(pUsage->rgpszUsageIdentifier[j])
                             + 1;
                        validUsages.rgpszUsageIdentifier =
                         CryptMemAlloc(cbOIDs);
                        if (validUsages.rgpszUsageIdentifier)
                        {
                            LPSTR nextOID = (LPSTR)
                             ((LPBYTE)validUsages.rgpszUsageIdentifier +
                             validUsages.cUsageIdentifier * sizeof(LPSTR));

                            for (j = 0; j < validUsages.cUsageIdentifier; j++)
                            {
                                validUsages.rgpszUsageIdentifier[j] = nextOID;
                                lstrcpyA(validUsages.rgpszUsageIdentifier[j],
                                 pUsage->rgpszUsageIdentifier[j]);
                                nextOID += lstrlenA(nextOID) + 1;
                            }
                        }
                        else
                            ret = FALSE;
                    }
                    else
                    {
                        DWORD j, k, validIndexes = 0, numRemoved = 0;

                        /* Merge: build a bitmap of all the indexes of
                         * validUsages.rgpszUsageIdentifier that are in pUsage.
                         */
                        for (j = 0; j < pUsage->cUsageIdentifier; j++)
                        {
                            for (k = 0; k < validUsages.cUsageIdentifier; k++)
                            {
                                if (!strcmp(pUsage->rgpszUsageIdentifier[j],
                                 validUsages.rgpszUsageIdentifier[k]))
                                {
                                    validIndexes |= (1 << k);
                                    break;
                                }
                            }
                        }
                        /* Merge by removing from validUsages those that are
                         * not in the bitmap.
                         */
                        for (j = 0; j < validUsages.cUsageIdentifier; j++)
                        {
                            if (!(validIndexes & (1 << j)))
                            {
                                if (j < validUsages.cUsageIdentifier - 1)
                                {
                                    memcpy(&validUsages.rgpszUsageIdentifier[j],
                                     &validUsages.rgpszUsageIdentifier[j +
                                     numRemoved + 1],
                                     (validUsages.cUsageIdentifier - numRemoved
                                     - j - 1) * sizeof(LPSTR));
                                    cbOIDs -= lstrlenA(
                                     validUsages.rgpszUsageIdentifier[j]) + 1 +
                                     sizeof(LPSTR);
                                    numRemoved++;
                                }
                                else
                                    validUsages.cUsageIdentifier--;
                            }
                        }
                    }
                }
                CryptMemFree(pUsage);
            }
            else
                ret = FALSE;
        }
    }
    if (ret)
    {
        if (allUsagesValid)
        {
            *cNumOIDSs = -1;
            *pcbOIDs = 0;
        }
        else
        {
            if (!rghOIDs || *pcbOIDs < cbOIDs)
            {
                *pcbOIDs = cbOIDs;
                SetLastError(ERROR_MORE_DATA);
                ret = FALSE;
            }
            else
            {
                LPSTR nextOID = (LPSTR)((LPBYTE)rghOIDs +
                 validUsages.cUsageIdentifier * sizeof(LPSTR));

                *pcbOIDs = cbOIDs;
                *cNumOIDSs = validUsages.cUsageIdentifier;
                for (i = 0; i < validUsages.cUsageIdentifier; i++)
                {
                    rghOIDs[i] = nextOID;
                    lstrcpyA(nextOID, validUsages.rgpszUsageIdentifier[i]);
                    nextOID += lstrlenA(nextOID) + 1;
                }
            }
        }
    }
    CryptMemFree(validUsages.rgpszUsageIdentifier);
    return ret;
}

/* Sets the CERT_KEY_PROV_INFO_PROP_ID property of context from pInfo, or, if
 * pInfo is NULL, from the attributes of hProv.
 */
static void CertContext_SetKeyProvInfo(PCCERT_CONTEXT context,
 PCRYPT_KEY_PROV_INFO pInfo, HCRYPTPROV hProv)
{
    CRYPT_KEY_PROV_INFO info = { 0 };
    BOOL ret;

    if (!pInfo)
    {
        DWORD size;
        int len;

        ret = CryptGetProvParam(hProv, PP_CONTAINER, NULL, &size, 0);
        if (ret)
        {
            LPSTR szContainer = CryptMemAlloc(size);

            if (szContainer)
            {
                ret = CryptGetProvParam(hProv, PP_CONTAINER,
                 (BYTE *)szContainer, &size, 0);
                if (ret)
                {
                    len = MultiByteToWideChar(CP_ACP, 0, szContainer, -1,
                     NULL, 0);
                    if (len)
                    {
                        info.pwszContainerName = CryptMemAlloc(len *
                         sizeof(WCHAR));
                        len = MultiByteToWideChar(CP_ACP, 0, szContainer, -1,
                         info.pwszContainerName, len);
                    }
                }
                CryptMemFree(szContainer);
            }
        }
        ret = CryptGetProvParam(hProv, PP_NAME, NULL, &size, 0);
        if (ret)
        {
            LPSTR szProvider = CryptMemAlloc(size);

            if (szProvider)
            {
                ret = CryptGetProvParam(hProv, PP_NAME, (BYTE *)szProvider,
                 &size, 0);
                if (ret)
                {
                    len = MultiByteToWideChar(CP_ACP, 0, szProvider, -1,
                     NULL, 0);
                    if (len)
                    {
                        info.pwszProvName = CryptMemAlloc(len *
                         sizeof(WCHAR));
                        len = MultiByteToWideChar(CP_ACP, 0, szProvider, -1,
                         info.pwszProvName, len);
                    }
                }
                CryptMemFree(szProvider);
            }
        }
        size = sizeof(info.dwKeySpec);
        ret = CryptGetProvParam(hProv, PP_KEYSPEC, (LPBYTE)&info.dwKeySpec,
         &size, 0);
        if (!ret)
            info.dwKeySpec = AT_SIGNATURE;
        size = sizeof(info.dwProvType);
        ret = CryptGetProvParam(hProv, PP_PROVTYPE, (LPBYTE)&info.dwProvType,
         &size, 0);
        if (!ret)
            info.dwProvType = PROV_RSA_FULL;
        pInfo = &info;
    }

    ret = CertSetCertificateContextProperty(context, CERT_KEY_PROV_INFO_PROP_ID,
     0, pInfo);

    if (pInfo == &info)
    {
        CryptMemFree(info.pwszContainerName);
        CryptMemFree(info.pwszProvName);
    }
}

/* Creates a signed certificate context from the unsigned, encoded certificate
 * in blob, using the crypto provider hProv and the signature algorithm sigAlgo.
 */
static PCCERT_CONTEXT CRYPT_CreateSignedCert(PCRYPT_DER_BLOB blob,
 HCRYPTPROV hProv, PCRYPT_ALGORITHM_IDENTIFIER sigAlgo)
{
    PCCERT_CONTEXT context = NULL;
    BOOL ret;
    DWORD sigSize = 0;

    ret = CryptSignCertificate(hProv, AT_SIGNATURE, X509_ASN_ENCODING,
     blob->pbData, blob->cbData, sigAlgo, NULL, NULL, &sigSize);
    if (ret)
    {
        LPBYTE sig = CryptMemAlloc(sigSize);

        ret = CryptSignCertificate(hProv, AT_SIGNATURE, X509_ASN_ENCODING,
         blob->pbData, blob->cbData, sigAlgo, NULL, sig, &sigSize);
        if (ret)
        {
            CERT_SIGNED_CONTENT_INFO signedInfo;
            BYTE *encodedSignedCert = NULL;
            DWORD encodedSignedCertSize = 0;

            signedInfo.ToBeSigned.cbData = blob->cbData;
            signedInfo.ToBeSigned.pbData = blob->pbData;
            memcpy(&signedInfo.SignatureAlgorithm, sigAlgo,
             sizeof(signedInfo.SignatureAlgorithm));
            signedInfo.Signature.cbData = sigSize;
            signedInfo.Signature.pbData = sig;
            signedInfo.Signature.cUnusedBits = 0;
            ret = CryptEncodeObjectEx(X509_ASN_ENCODING, X509_CERT,
             &signedInfo, CRYPT_ENCODE_ALLOC_FLAG, NULL,
             (BYTE *)&encodedSignedCert, &encodedSignedCertSize);
            if (ret)
            {
                context = CertCreateCertificateContext(X509_ASN_ENCODING,
                 encodedSignedCert, encodedSignedCertSize);
                LocalFree(encodedSignedCert);
            }
        }
        CryptMemFree(sig);
    }
    return context;
}

/* Copies data from the parameters into info, where:
 * pSubjectIssuerBlob: Specifies both the subject and issuer for info.
 *                     Must not be NULL
 * pSignatureAlgorithm: Optional.
 * pStartTime: The starting time of the certificate.  If NULL, the current
 *             system time is used.
 * pEndTime: The ending time of the certificate.  If NULL, one year past the
 *           starting time is used.
 * pubKey: The public key of the certificate.  Must not be NULL.
 * pExtensions: Extensions to be included with the certificate.  Optional.
 */
static void CRYPT_MakeCertInfo(PCERT_INFO info,
 PCERT_NAME_BLOB pSubjectIssuerBlob,
 PCRYPT_ALGORITHM_IDENTIFIER pSignatureAlgorithm, PSYSTEMTIME pStartTime,
 PSYSTEMTIME pEndTime, PCERT_PUBLIC_KEY_INFO pubKey,
 PCERT_EXTENSIONS pExtensions)
{
    /* FIXME: what serial number to use? */
    static const BYTE serialNum[] = { 1 };

    assert(info);
    assert(pSubjectIssuerBlob);
    assert(pubKey);

    info->dwVersion = CERT_V3;
    info->SerialNumber.cbData = sizeof(serialNum);
    info->SerialNumber.pbData = (LPBYTE)serialNum;
    if (pSignatureAlgorithm)
        memcpy(&info->SignatureAlgorithm, pSignatureAlgorithm,
         sizeof(info->SignatureAlgorithm));
    else
    {
        info->SignatureAlgorithm.pszObjId = szOID_RSA_SHA1RSA;
        info->SignatureAlgorithm.Parameters.cbData = 0;
        info->SignatureAlgorithm.Parameters.pbData = NULL;
    }
    info->Issuer.cbData = pSubjectIssuerBlob->cbData;
    info->Issuer.pbData = pSubjectIssuerBlob->pbData;
    if (pStartTime)
        SystemTimeToFileTime(pStartTime, &info->NotBefore);
    else
        GetSystemTimeAsFileTime(&info->NotBefore);
    if (pEndTime)
        SystemTimeToFileTime(pStartTime, &info->NotAfter);
    else
    {
        SYSTEMTIME endTime;

        if (FileTimeToSystemTime(&info->NotBefore, &endTime))
        {
            endTime.wYear++;
            SystemTimeToFileTime(&endTime, &info->NotAfter);
        }
    }
    info->Subject.cbData = pSubjectIssuerBlob->cbData;
    info->Subject.pbData = pSubjectIssuerBlob->pbData;
    memcpy(&info->SubjectPublicKeyInfo, pubKey,
     sizeof(info->SubjectPublicKeyInfo));
    if (pExtensions)
    {
        info->cExtension = pExtensions->cExtension;
        info->rgExtension = pExtensions->rgExtension;
    }
    else
    {
        info->cExtension = 0;
        info->rgExtension = NULL;
    }
}
 
typedef RPC_STATUS (RPC_ENTRY *UuidCreateFunc)(UUID *);
typedef RPC_STATUS (RPC_ENTRY *UuidToStringFunc)(UUID *, unsigned char **);
typedef RPC_STATUS (RPC_ENTRY *RpcStringFreeFunc)(unsigned char **);

static HCRYPTPROV CRYPT_CreateKeyProv(void)
{
    HCRYPTPROV hProv = 0;
    HMODULE rpcrt = LoadLibraryA("rpcrt4");

    if (rpcrt)
    {
        UuidCreateFunc uuidCreate = (UuidCreateFunc)GetProcAddress(rpcrt,
         "UuidCreate");
        UuidToStringFunc uuidToString = (UuidToStringFunc)GetProcAddress(rpcrt,
         "UuidToString");
        RpcStringFreeFunc rpcStringFree = (RpcStringFreeFunc)GetProcAddress(
         rpcrt, "RpcStringFree");

        if (uuidCreate && uuidToString && rpcStringFree)
        {
            UUID uuid;
            RPC_STATUS status = uuidCreate(&uuid);

            if (status == RPC_S_OK || status == RPC_S_UUID_LOCAL_ONLY)
            {
                unsigned char *uuidStr;

                status = uuidToString(&uuid, &uuidStr);
                if (status == RPC_S_OK)
                {
                    BOOL ret = CryptAcquireContextA(&hProv, (LPCSTR)uuidStr,
                     MS_DEF_PROV_A, PROV_RSA_FULL, CRYPT_NEWKEYSET);

                    if (ret)
                    {
                        HCRYPTKEY key;

                        ret = CryptGenKey(hProv, AT_SIGNATURE, 0, &key);
                        if (ret)
                            CryptDestroyKey(key);
                    }
                    rpcStringFree(&uuidStr);
                }
            }
        }
        FreeLibrary(rpcrt);
    }
    return hProv;
}

PCCERT_CONTEXT WINAPI CertCreateSelfSignCertificate(HCRYPTPROV hProv,
 PCERT_NAME_BLOB pSubjectIssuerBlob, DWORD dwFlags,
 PCRYPT_KEY_PROV_INFO pKeyProvInfo,
 PCRYPT_ALGORITHM_IDENTIFIER pSignatureAlgorithm, PSYSTEMTIME pStartTime,
 PSYSTEMTIME pEndTime, PCERT_EXTENSIONS pExtensions)
{
    PCCERT_CONTEXT context = NULL;
    BOOL ret, releaseContext = FALSE;
    PCERT_PUBLIC_KEY_INFO pubKey = NULL;
    DWORD pubKeySize = 0;

    TRACE("(0x%08lx, %p, %08lx, %p, %p, %p, %p, %p)\n", hProv,
     pSubjectIssuerBlob, dwFlags, pKeyProvInfo, pSignatureAlgorithm, pStartTime,
     pExtensions, pExtensions);

    if (!hProv)
    {
        hProv = CRYPT_CreateKeyProv();
        releaseContext = TRUE;
    }

    CryptExportPublicKeyInfo(hProv, AT_SIGNATURE, X509_ASN_ENCODING, NULL,
     &pubKeySize);
    pubKey = CryptMemAlloc(pubKeySize);
    if (pubKey)
    {
        ret = CryptExportPublicKeyInfo(hProv, AT_SIGNATURE, X509_ASN_ENCODING,
         pubKey, &pubKeySize);
        if (ret)
        {
            CERT_INFO info = { 0 };
            CRYPT_DER_BLOB blob = { 0, NULL };
            BOOL ret;

            CRYPT_MakeCertInfo(&info, pSubjectIssuerBlob, pSignatureAlgorithm,
             pStartTime, pEndTime, pubKey, pExtensions);
            ret = CryptEncodeObjectEx(X509_ASN_ENCODING, X509_CERT_TO_BE_SIGNED,
             &info, CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&blob.pbData,
             &blob.cbData);
            if (ret)
            {
                if (!(dwFlags & CERT_CREATE_SELFSIGN_NO_SIGN))
                    context = CRYPT_CreateSignedCert(&blob, hProv,
                     &info.SignatureAlgorithm);
                else
                    context = CertCreateCertificateContext(X509_ASN_ENCODING,
                     blob.pbData, blob.cbData);
                if (context && !(dwFlags & CERT_CREATE_SELFSIGN_NO_KEY_INFO))
                    CertContext_SetKeyProvInfo(context, pKeyProvInfo, hProv);
                LocalFree(blob.pbData);
            }
        }
        CryptMemFree(pubKey);
    }
    if (releaseContext)
        CryptReleaseContext(hProv, 0);
    return context;
}
