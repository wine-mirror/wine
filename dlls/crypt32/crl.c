/*
 * Copyright 2006 Juan Lang
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
 *
 */

#include <assert.h>
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "wine/debug.h"
#include "crypt32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

PCCRL_CONTEXT WINAPI CertCreateCRLContext(DWORD dwCertEncodingType,
 const BYTE* pbCrlEncoded, DWORD cbCrlEncoded)
{
    PCRL_CONTEXT crl = NULL;
    BOOL ret;
    PCERT_SIGNED_CONTENT_INFO signedCrl = NULL;
    PCRL_INFO crlInfo = NULL;
    DWORD size = 0;

    TRACE("(%08lx, %p, %ld)\n", dwCertEncodingType, pbCrlEncoded,
     cbCrlEncoded);

    if ((dwCertEncodingType & CERT_ENCODING_TYPE_MASK) != X509_ASN_ENCODING)
    {
        SetLastError(E_INVALIDARG);
        return NULL;
    }
    /* First try to decode it as a signed crl. */
    ret = CryptDecodeObjectEx(dwCertEncodingType, X509_CERT, pbCrlEncoded,
     cbCrlEncoded, CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&signedCrl, &size);
    if (ret)
    {
        size = 0;
        ret = CryptDecodeObjectEx(dwCertEncodingType,
         X509_CERT_CRL_TO_BE_SIGNED, signedCrl->ToBeSigned.pbData,
         signedCrl->ToBeSigned.cbData, CRYPT_DECODE_ALLOC_FLAG, NULL,
         (BYTE *)&crlInfo, &size);
        LocalFree(signedCrl);
    }
    /* Failing that, try it as an unsigned crl */
    if (!ret)
    {
        size = 0;
        ret = CryptDecodeObjectEx(dwCertEncodingType,
         X509_CERT_CRL_TO_BE_SIGNED, pbCrlEncoded, cbCrlEncoded,
         CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG, NULL,
         (BYTE *)&crlInfo, &size);
    }
    if (ret)
    {
        BYTE *data = NULL;

        crl = (PCRL_CONTEXT)Context_CreateDataContext(sizeof(CRL_CONTEXT));
        if (!crl)
            goto end;
        data = CryptMemAlloc(cbCrlEncoded);
        if (!data)
        {
            CryptMemFree(crl);
            crl = NULL;
            goto end;
        }
        memcpy(data, pbCrlEncoded, cbCrlEncoded);
        crl->dwCertEncodingType = dwCertEncodingType;
        crl->pbCrlEncoded       = data;
        crl->cbCrlEncoded       = cbCrlEncoded;
        crl->pCrlInfo           = crlInfo;
        crl->hCertStore         = 0;
    }

end:
    return (PCCRL_CONTEXT)crl;
}

BOOL WINAPI CertAddEncodedCRLToStore(HCERTSTORE hCertStore,
 DWORD dwCertEncodingType, const BYTE *pbCrlEncoded, DWORD cbCrlEncoded,
 DWORD dwAddDisposition, PCCRL_CONTEXT *ppCrlContext)
{
    PCCRL_CONTEXT crl = CertCreateCRLContext(dwCertEncodingType,
     pbCrlEncoded, cbCrlEncoded);
    BOOL ret;

    TRACE("(%p, %08lx, %p, %ld, %08lx, %p)\n", hCertStore, dwCertEncodingType,
     pbCrlEncoded, cbCrlEncoded, dwAddDisposition, ppCrlContext);

    if (crl)
    {
        ret = CertAddCRLContextToStore(hCertStore, crl, dwAddDisposition,
         ppCrlContext);
        CertFreeCRLContext(crl);
    }
    else
        ret = FALSE;
    return ret;
}

typedef BOOL (*CrlCompareFunc)(PCCRL_CONTEXT pCrlContext, DWORD dwType,
 DWORD dwFlags, const void *pvPara);

static BOOL compare_crl_any(PCCRL_CONTEXT pCrlContext, DWORD dwType,
 DWORD dwFlags, const void *pvPara)
{
    return TRUE;
}

static BOOL compare_crl_issued_by(PCCRL_CONTEXT pCrlContext, DWORD dwType,
 DWORD dwFlags, const void *pvPara)
{
    BOOL ret;

    if (pvPara)
    {
        PCCERT_CONTEXT issuer = (PCCERT_CONTEXT)pvPara;

        ret = CertCompareCertificateName(issuer->dwCertEncodingType,
         &issuer->pCertInfo->Issuer, &pCrlContext->pCrlInfo->Issuer);
    }
    else
        ret = TRUE;
    return ret;
}

static BOOL compare_crl_existing(PCCRL_CONTEXT pCrlContext, DWORD dwType,
 DWORD dwFlags, const void *pvPara)
{
    BOOL ret;

    if (pvPara)
    {
        PCCRL_CONTEXT crl = (PCCRL_CONTEXT)pvPara;

        ret = CertCompareCertificateName(pCrlContext->dwCertEncodingType,
         &pCrlContext->pCrlInfo->Issuer, &crl->pCrlInfo->Issuer);
    }
    else
        ret = TRUE;
    return ret;
}

PCCRL_CONTEXT WINAPI CertFindCRLInStore(HCERTSTORE hCertStore,
 DWORD dwCertEncodingType, DWORD dwFindFlags, DWORD dwFindType,
 const void *pvFindPara, PCCRL_CONTEXT pPrevCrlContext)
{
    PCCRL_CONTEXT ret;
    CrlCompareFunc compare;

    TRACE("(%p, %ld, %ld, %ld, %p, %p)\n", hCertStore, dwCertEncodingType,
	 dwFindFlags, dwFindType, pvFindPara, pPrevCrlContext);

    switch (dwFindType)
    {
    case CRL_FIND_ANY:
        compare = compare_crl_any;
        break;
    case CRL_FIND_ISSUED_BY:
        compare = compare_crl_issued_by;
        break;
    case CRL_FIND_EXISTING:
        compare = compare_crl_existing;
        break;
    default:
        FIXME("find type %08lx unimplemented\n", dwFindType);
        compare = NULL;
    }

    if (compare)
    {
        BOOL matches = FALSE;

        ret = pPrevCrlContext;
        do {
            ret = CertEnumCRLsInStore(hCertStore, ret);
            if (ret)
                matches = compare(ret, dwFindType, dwFindFlags, pvFindPara);
        } while (ret != NULL && !matches);
        if (!ret)
            SetLastError(CRYPT_E_NOT_FOUND);
    }
    else
    {
        SetLastError(CRYPT_E_NOT_FOUND);
        ret = NULL;
    }
    return ret;
}

PCCRL_CONTEXT WINAPI CertDuplicateCRLContext(PCCRL_CONTEXT pCrlContext)
{
    TRACE("(%p)\n", pCrlContext);
    Context_AddRef((void *)pCrlContext, sizeof(CRL_CONTEXT));
    return pCrlContext;
}

static void CrlDataContext_Free(void *context)
{
    PCRL_CONTEXT crlContext = (PCRL_CONTEXT)context;

    CryptMemFree(crlContext->pbCrlEncoded);
    LocalFree(crlContext->pCrlInfo);
}

BOOL WINAPI CertFreeCRLContext( PCCRL_CONTEXT pCrlContext)
{
    TRACE("(%p)\n", pCrlContext);

    if (pCrlContext)
        Context_Release((void *)pCrlContext, sizeof(CRL_CONTEXT),
         CrlDataContext_Free);
    return TRUE;
}

DWORD WINAPI CertEnumCRLContextProperties(PCCRL_CONTEXT pCRLContext,
 DWORD dwPropId)
{
    PCONTEXT_PROPERTY_LIST properties = Context_GetProperties(
     (void *)pCRLContext, sizeof(CRL_CONTEXT));
    DWORD ret;

    TRACE("(%p, %ld)\n", pCRLContext, dwPropId);

    if (properties)
        ret = ContextPropertyList_EnumPropIDs(properties, dwPropId);
    else
        ret = 0;
    return ret;
}

static BOOL WINAPI CRLContext_SetProperty(void *context, DWORD dwPropId,
 DWORD dwFlags, const void *pvData);

static BOOL CRLContext_GetHashProp(void *context, DWORD dwPropId,
 ALG_ID algID, const BYTE *toHash, DWORD toHashLen, void *pvData,
 DWORD *pcbData)
{
    BOOL ret = CryptHashCertificate(0, algID, 0, toHash, toHashLen, pvData,
     pcbData);
    if (ret)
    {
        CRYPT_DATA_BLOB blob = { *pcbData, pvData };

        ret = CRLContext_SetProperty(context, dwPropId, 0, &blob);
    }
    return ret;
}

static BOOL WINAPI CRLContext_GetProperty(void *context, DWORD dwPropId,
 void *pvData, DWORD *pcbData)
{
    PCCRL_CONTEXT pCRLContext = (PCCRL_CONTEXT)context;
    PCONTEXT_PROPERTY_LIST properties =
     Context_GetProperties(context, sizeof(CRL_CONTEXT));
    BOOL ret;
    CRYPT_DATA_BLOB blob;

    TRACE("(%p, %ld, %p, %p)\n", context, dwPropId, pvData, pcbData);

    if (properties)
        ret = ContextPropertyList_FindProperty(properties, dwPropId, &blob);
    else
        ret = FALSE;
    if (ret)
    {
        if (!pvData)
        {
            *pcbData = blob.cbData;
            ret = TRUE;
        }
        else if (*pcbData < blob.cbData)
        {
            SetLastError(ERROR_MORE_DATA);
            *pcbData = blob.cbData;
        }
        else
        {
            memcpy(pvData, blob.pbData, blob.cbData);
            *pcbData = blob.cbData;
            ret = TRUE;
        }
    }
    else
    {
        /* Implicit properties */
        switch (dwPropId)
        {
        case CERT_SHA1_HASH_PROP_ID:
            ret = CRLContext_GetHashProp(context, dwPropId, CALG_SHA1,
             pCRLContext->pbCrlEncoded, pCRLContext->cbCrlEncoded, pvData,
             pcbData);
            break;
        case CERT_MD5_HASH_PROP_ID:
            ret = CRLContext_GetHashProp(context, dwPropId, CALG_MD5,
             pCRLContext->pbCrlEncoded, pCRLContext->cbCrlEncoded, pvData,
             pcbData);
            break;
        default:
            SetLastError(CRYPT_E_NOT_FOUND);
        }
    }
    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI CertGetCRLContextProperty(PCCRL_CONTEXT pCRLContext,
 DWORD dwPropId, void *pvData, DWORD *pcbData)
{
    BOOL ret;

    TRACE("(%p, %ld, %p, %p)\n", pCRLContext, dwPropId, pvData, pcbData);

    switch (dwPropId)
    {
    case 0:
    case CERT_CERT_PROP_ID:
    case CERT_CRL_PROP_ID:
    case CERT_CTL_PROP_ID:
        SetLastError(E_INVALIDARG);
        ret = FALSE;
        break;
    case CERT_ACCESS_STATE_PROP_ID:
        if (!pvData)
        {
            *pcbData = sizeof(DWORD);
            ret = TRUE;
        }
        else if (*pcbData < sizeof(DWORD))
        {
            SetLastError(ERROR_MORE_DATA);
            *pcbData = sizeof(DWORD);
            ret = FALSE;
        }
        else
        {
            *(DWORD *)pvData =
             CertStore_GetAccessState(pCRLContext->hCertStore);
            ret = TRUE;
        }
        break;
    default:
        ret = CRLContext_GetProperty((void *)pCRLContext, dwPropId, pvData,
         pcbData);
    }
    return ret;
}

static BOOL WINAPI CRLContext_SetProperty(void *context, DWORD dwPropId,
 DWORD dwFlags, const void *pvData)
{
    PCONTEXT_PROPERTY_LIST properties =
     Context_GetProperties(context, sizeof(CERT_CONTEXT));
    BOOL ret;

    TRACE("(%p, %ld, %08lx, %p)\n", context, dwPropId, dwFlags, pvData);

    if (!properties)
        ret = FALSE;
    else if (!pvData)
    {
        ContextPropertyList_RemoveProperty(properties, dwPropId);
        ret = TRUE;
    }
    else
    {
        switch (dwPropId)
        {
        case CERT_AUTO_ENROLL_PROP_ID:
        case CERT_CTL_USAGE_PROP_ID: /* same as CERT_ENHKEY_USAGE_PROP_ID */
        case CERT_DESCRIPTION_PROP_ID:
        case CERT_FRIENDLY_NAME_PROP_ID:
        case CERT_HASH_PROP_ID:
        case CERT_KEY_IDENTIFIER_PROP_ID:
        case CERT_MD5_HASH_PROP_ID:
        case CERT_NEXT_UPDATE_LOCATION_PROP_ID:
        case CERT_PUBKEY_ALG_PARA_PROP_ID:
        case CERT_PVK_FILE_PROP_ID:
        case CERT_SIGNATURE_HASH_PROP_ID:
        case CERT_ISSUER_PUBLIC_KEY_MD5_HASH_PROP_ID:
        case CERT_SUBJECT_NAME_MD5_HASH_PROP_ID:
        case CERT_SUBJECT_PUBLIC_KEY_MD5_HASH_PROP_ID:
        case CERT_ENROLLMENT_PROP_ID:
        case CERT_CROSS_CERT_DIST_POINTS_PROP_ID:
        case CERT_RENEWAL_PROP_ID:
        {
            PCRYPT_DATA_BLOB blob = (PCRYPT_DATA_BLOB)pvData;

            ret = ContextPropertyList_SetProperty(properties, dwPropId,
             blob->pbData, blob->cbData);
            break;
        }
        case CERT_DATE_STAMP_PROP_ID:
            ret = ContextPropertyList_SetProperty(properties, dwPropId,
             (LPBYTE)pvData, sizeof(FILETIME));
            break;
        default:
            FIXME("%ld: stub\n", dwPropId);
            ret = FALSE;
        }
    }
    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI CertSetCRLContextProperty(PCCRL_CONTEXT pCRLContext,
 DWORD dwPropId, DWORD dwFlags, const void *pvData)
{
    BOOL ret;

    TRACE("(%p, %ld, %08lx, %p)\n", pCRLContext, dwPropId, dwFlags, pvData);

    /* Handle special cases for "read-only"/invalid prop IDs.  Windows just
     * crashes on most of these, I'll be safer.
     */
    switch (dwPropId)
    {
    case 0:
    case CERT_ACCESS_STATE_PROP_ID:
    case CERT_CERT_PROP_ID:
    case CERT_CRL_PROP_ID:
    case CERT_CTL_PROP_ID:
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    ret = CRLContext_SetProperty((void *)pCRLContext, dwPropId, dwFlags,
     pvData);
    TRACE("returning %d\n", ret);
    return ret;
}
