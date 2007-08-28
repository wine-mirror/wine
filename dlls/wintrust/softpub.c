/*
 * Copyright 2007 Juan Lang
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
#include "windef.h"
#include "winbase.h"
#include "wintrust.h"
#include "mssip.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wintrust);

HRESULT WINAPI SoftpubInitialize(CRYPT_PROVIDER_DATA *data)
{
    HRESULT ret = S_FALSE;

    TRACE("(%p)\n", data);

    if (data->padwTrustStepErrors &&
     !data->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_WVTINIT])
        ret = S_OK;
    TRACE("returning %08x\n", ret);
    return ret;
}

/* Assumes data->pWintrustData->pFile exists.  Makes sure a file handle is
 * open for the file.
 */
static BOOL SOFTPUB_OpenFile(CRYPT_PROVIDER_DATA *data)
{
    BOOL ret = TRUE;

    /* PSDK implies that all values should be initialized to NULL, so callers
     * typically have hFile as NULL rather than INVALID_HANDLE_VALUE.  Check
     * for both.
     */
    if (!data->pWintrustData->pFile->hFile ||
     data->pWintrustData->pFile->hFile == INVALID_HANDLE_VALUE)
    {
        data->pWintrustData->pFile->hFile =
         CreateFileW(data->pWintrustData->pFile->pcwszFilePath, GENERIC_READ,
          FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (data->pWintrustData->pFile->hFile != INVALID_HANDLE_VALUE)
            data->fOpenedFile = TRUE;
        else
            ret = FALSE;
    }
    TRACE("returning %d\n", ret);
    return ret;
}

/* Assumes data->pWintrustData->pFile exists.  Sets data->pPDSip->gSubject to
 * the file's subject GUID.
 */
static BOOL SOFTPUB_GetFileSubject(CRYPT_PROVIDER_DATA *data)
{
    BOOL ret;

    if (!data->pWintrustData->pFile->pgKnownSubject)
    {
        ret = CryptSIPRetrieveSubjectGuid(
         data->pWintrustData->pFile->pcwszFilePath,
         data->pWintrustData->pFile->hFile,
         &data->pPDSip->gSubject);
    }
    else
    {
        memcpy(&data->pPDSip->gSubject,
         data->pWintrustData->pFile->pgKnownSubject, sizeof(GUID));
        ret = TRUE;
    }
    TRACE("returning %d\n", ret);
    return ret;
}

/* Assumes data->pPDSip exists, and its gSubject member set.
 * Allocates data->pPDSip->pSip and loads it, if possible.
 */
static BOOL SOFTPUB_GetSIP(CRYPT_PROVIDER_DATA *data)
{
    BOOL ret;

    data->pPDSip->pSip = data->psPfns->pfnAlloc(sizeof(SIP_DISPATCH_INFO));
    if (data->pPDSip->pSip)
        ret = CryptSIPLoad(&data->pPDSip->gSubject, 0, data->pPDSip->pSip);
    else
    {
        SetLastError(ERROR_OUTOFMEMORY);
        ret = FALSE;
    }
    TRACE("returning %d\n", ret);
    return ret;
}

/* Assumes data->pPDSip has been loaded, and data->pPDSip->pSip allocated.
 * Calls data->pPDSip->pSip->pfGet to construct data->hMsg.
 */
static BOOL SOFTPUB_GetMessageFromFile(CRYPT_PROVIDER_DATA *data)
{
    BOOL ret;
    LPBYTE buf = NULL;
    DWORD size = 0;

    data->pPDSip->psSipSubjectInfo =
     data->psPfns->pfnAlloc(sizeof(SIP_SUBJECTINFO));
    if (!data->pPDSip->psSipSubjectInfo)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    data->pPDSip->psSipSubjectInfo->cbSize = sizeof(SIP_SUBJECTINFO);
    data->pPDSip->psSipSubjectInfo->pgSubjectType = &data->pPDSip->gSubject;
    data->pPDSip->psSipSubjectInfo->hFile = data->pWintrustData->pFile->hFile;
    data->pPDSip->psSipSubjectInfo->pwsFileName =
     data->pWintrustData->pFile->pcwszFilePath;
    data->pPDSip->psSipSubjectInfo->hProv = data->hProv;
    ret = data->pPDSip->pSip->pfGet(data->pPDSip->psSipSubjectInfo,
     &data->dwEncoding, 0, &size, 0);
    if (!ret)
    {
        SetLastError(TRUST_E_NOSIGNATURE);
        return FALSE;
    }

    buf = data->psPfns->pfnAlloc(size);
    if (!buf)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    ret = data->pPDSip->pSip->pfGet(data->pPDSip->psSipSubjectInfo,
     &data->dwEncoding, 0, &size, buf);
    if (ret)
    {
        data->hMsg = CryptMsgOpenToDecode(data->dwEncoding, 0, 0, data->hProv,
         NULL, NULL);
        if (data->hMsg)
            ret = CryptMsgUpdate(data->hMsg, buf, size, TRUE);
    }

    data->psPfns->pfnFree(buf);
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL SOFTPUB_CreateStoreFromMessage(CRYPT_PROVIDER_DATA *data)
{
    BOOL ret = FALSE;
    HCERTSTORE store;

    store = CertOpenStore(CERT_STORE_PROV_MSG, data->dwEncoding,
     data->hProv, CERT_STORE_NO_CRYPT_RELEASE_FLAG, data->hMsg);
    if (store)
    {
        ret = data->psPfns->pfnAddStore2Chain(data, store);
        CertCloseStore(store, 0);
    }
    TRACE("returning %d\n", ret);
    return ret;
}

static DWORD SOFTPUB_DecodeInnerContent(CRYPT_PROVIDER_DATA *data)
{
    BOOL ret;
    DWORD size;
    LPBYTE buf = NULL;

    ret = CryptMsgGetParam(data->hMsg, CMSG_INNER_CONTENT_TYPE_PARAM, 0, NULL,
     &size);
    if (!ret)
        goto error;
    buf = data->psPfns->pfnAlloc(size);
    if (!buf)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        ret = FALSE;
        goto error;
    }
    ret = CryptMsgGetParam(data->hMsg, CMSG_INNER_CONTENT_TYPE_PARAM, 0, buf,
     &size);
    if (!ret)
        goto error;
    if (!strcmp((LPCSTR)buf, SPC_INDIRECT_DATA_OBJID))
    {
        data->psPfns->pfnFree(buf);
        buf = NULL;
        ret = CryptMsgGetParam(data->hMsg, CMSG_CONTENT_PARAM, 0, NULL, &size);
        if (!ret)
            goto error;
        buf = data->psPfns->pfnAlloc(size);
        if (!buf)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            ret = FALSE;
            goto error;
        }
        ret = CryptMsgGetParam(data->hMsg, CMSG_CONTENT_PARAM, 0, buf, &size);
        if (!ret)
            goto error;
        ret = CryptDecodeObject(data->dwEncoding,
         SPC_INDIRECT_DATA_CONTENT_STRUCT, buf, size, 0, NULL, &size);
        if (!ret)
            goto error;
        data->pPDSip->psIndirectData = data->psPfns->pfnAlloc(size);
        if (!data->pPDSip->psIndirectData)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            ret = FALSE;
            goto error;
        }
        ret = CryptDecodeObject(data->dwEncoding,
         SPC_INDIRECT_DATA_CONTENT_STRUCT, buf, size, 0,
         data->pPDSip->psIndirectData, &size);
    }
    else
    {
        FIXME("unimplemented for OID %s\n", (LPCSTR)buf);
        SetLastError(TRUST_E_SUBJECT_FORM_UNKNOWN);
        ret = FALSE;
    }

error:
    TRACE("returning %d\n", ret);
    return ret;
}

HRESULT WINAPI SoftpubLoadMessage(CRYPT_PROVIDER_DATA *data)
{
    BOOL ret;

    TRACE("(%p)\n", data);

    if (!data->padwTrustStepErrors)
        return S_FALSE;

    switch (data->pWintrustData->dwUnionChoice)
    {
    case WTD_CHOICE_CERT:
        /* Do nothing!?  See the tests */
        ret = TRUE;
        break;
    case WTD_CHOICE_FILE:
        if (!data->pWintrustData->pFile)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            ret = FALSE;
            goto error;
        }
        ret = SOFTPUB_OpenFile(data);
        if (!ret)
            goto error;
        ret = SOFTPUB_GetFileSubject(data);
        if (!ret)
            goto error;
        ret = SOFTPUB_GetSIP(data);
        if (!ret)
            goto error;
        ret = SOFTPUB_GetMessageFromFile(data);
        if (!ret)
            goto error;
        ret = SOFTPUB_CreateStoreFromMessage(data);
        if (!ret)
            goto error;
        ret = SOFTPUB_DecodeInnerContent(data);
        break;
    default:
        FIXME("unimplemented for %d\n", data->pWintrustData->dwUnionChoice);
        SetLastError(ERROR_INVALID_PARAMETER);
        ret = FALSE;
    }

error:
    if (!ret)
        data->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV] =
         GetLastError();
    return ret ? S_OK : S_FALSE;
}

static CMSG_SIGNER_INFO *WINTRUST_GetSigner(CRYPT_PROVIDER_DATA *data,
 DWORD signerIdx)
{
    BOOL ret;
    CMSG_SIGNER_INFO *signerInfo = NULL;
    DWORD size;

    ret = CryptMsgGetParam(data->hMsg, CMSG_SIGNER_INFO_PARAM, signerIdx,
     NULL, &size);
    if (ret)
    {
        signerInfo = data->psPfns->pfnAlloc(size);
        if (signerInfo)
        {
            ret = CryptMsgGetParam(data->hMsg, CMSG_SIGNER_INFO_PARAM,
             signerIdx, signerInfo, &size);
            if (!ret)
            {
                data->psPfns->pfnFree(signerInfo);
                signerInfo = NULL;
            }
        }
        else
            SetLastError(ERROR_OUTOFMEMORY);
    }
    return signerInfo;
}

static BOOL WINTRUST_SaveSigner(CRYPT_PROVIDER_DATA *data, DWORD signerIdx)
{
    BOOL ret;
    CMSG_SIGNER_INFO *signerInfo = WINTRUST_GetSigner(data, signerIdx);

    if (signerInfo)
    {
        CRYPT_PROVIDER_SGNR sgnr = { sizeof(sgnr), { 0 } };

        sgnr.psSigner = signerInfo;
        ret = data->psPfns->pfnAddSgnr2Chain(data, FALSE, signerIdx, &sgnr);
    }
    else
        ret = FALSE;
    return ret;
}

static CERT_INFO *WINTRUST_GetSignerCertInfo(CRYPT_PROVIDER_DATA *data,
 DWORD signerIdx)
{
    BOOL ret;
    CERT_INFO *certInfo = NULL;
    DWORD size;

    ret = CryptMsgGetParam(data->hMsg, CMSG_SIGNER_CERT_INFO_PARAM, signerIdx,
     NULL, &size);
    if (ret)
    {
        certInfo = data->psPfns->pfnAlloc(size);
        if (certInfo)
        {
            ret = CryptMsgGetParam(data->hMsg, CMSG_SIGNER_CERT_INFO_PARAM,
             signerIdx, certInfo, &size);
            if (!ret)
            {
                data->psPfns->pfnFree(certInfo);
                certInfo = NULL;
            }
        }
        else
            SetLastError(ERROR_OUTOFMEMORY);
    }
    return certInfo;
}

static BOOL WINTRUST_VerifySigner(CRYPT_PROVIDER_DATA *data, DWORD signerIdx)
{
    BOOL ret;
    CERT_INFO *certInfo = WINTRUST_GetSignerCertInfo(data, signerIdx);

    if (certInfo)
    {
        CMSG_CTRL_VERIFY_SIGNATURE_EX_PARA para = { sizeof(para), 0, signerIdx,
         CMSG_VERIFY_SIGNER_CERT, NULL };

        para.pvSigner = (LPVOID)CertGetSubjectCertificateFromStore(
         data->pahStores[0], data->dwEncoding, certInfo);
        if (para.pvSigner)
        {
            ret = CryptMsgControl(data->hMsg, 0, CMSG_CTRL_VERIFY_SIGNATURE_EX,
             &para);
            if (!ret)
                SetLastError(TRUST_E_CERT_SIGNATURE);
        }
        else
        {
            SetLastError(TRUST_E_NO_SIGNER_CERT);
            ret = FALSE;
        }
        data->psPfns->pfnFree(certInfo);
    }
    else
        ret = FALSE;
    return ret;
}

HRESULT WINAPI SoftpubLoadSignature(CRYPT_PROVIDER_DATA *data)
{
    BOOL ret;
    DWORD signerCount, size;

    TRACE("(%p)\n", data);

    if (!data->padwTrustStepErrors)
        return S_FALSE;

    size = sizeof(signerCount);
    ret = CryptMsgGetParam(data->hMsg, CMSG_SIGNER_COUNT_PARAM, 0,
     &signerCount, &size);
    if (ret)
    {
        DWORD i;

        for (i = 0; ret && i < signerCount; i++)
        {
            if ((ret = WINTRUST_SaveSigner(data, i)))
                ret = WINTRUST_VerifySigner(data, i);
        }
    }
    else
        SetLastError(TRUST_E_NOSIGNATURE);
    if (!ret)
        data->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV] =
         GetLastError();
    return ret ? S_OK : S_FALSE;
}

BOOL WINAPI SoftpubCheckCert(CRYPT_PROVIDER_DATA *data, DWORD idxSigner,
 BOOL fCounterSignerChain, DWORD idxCounterSigner)
{
    BOOL ret;

    TRACE("(%p, %d, %d, %d)\n", data, idxSigner, fCounterSignerChain,
     idxCounterSigner);

    if (fCounterSignerChain)
    {
        FIXME("unimplemented for counter signers\n");
        ret = FALSE;
    }
    else
    {
        PCERT_SIMPLE_CHAIN simpleChain =
         data->pasSigners[idxSigner].pChainContext->rgpChain[0];
        DWORD i;

        ret = TRUE;
        for (i = 0; i < simpleChain->cElement; i++)
        {
            /* Set confidence */
            data->pasSigners[idxSigner].pasCertChain[i].dwConfidence = 0;
            /* The last element in the chain doesn't have an issuer, so it
             * can't have a valid time (with respect to its issuer)
             */
            if (i != simpleChain->cElement - 1 &&
             !(simpleChain->rgpElement[i]->TrustStatus.dwErrorStatus &
             CERT_TRUST_IS_NOT_TIME_VALID))
                data->pasSigners[idxSigner].pasCertChain[i].dwConfidence
                 |= CERT_CONFIDENCE_TIME;
            if (!(simpleChain->rgpElement[i]->TrustStatus.dwErrorStatus &
             CERT_TRUST_IS_NOT_TIME_NESTED))
                data->pasSigners[idxSigner].pasCertChain[i].dwConfidence
                 |= CERT_CONFIDENCE_TIMENEST;
            if (!(simpleChain->rgpElement[i]->TrustStatus.dwErrorStatus &
             CERT_TRUST_IS_NOT_SIGNATURE_VALID))
                data->pasSigners[idxSigner].pasCertChain[i].dwConfidence
                 |= CERT_CONFIDENCE_SIG;
            /* Set additional flags */
            if (!(simpleChain->rgpElement[i]->TrustStatus.dwErrorStatus &
             CERT_TRUST_IS_UNTRUSTED_ROOT))
                data->pasSigners[idxSigner].pasCertChain[i].fTrustedRoot = TRUE;
            if (simpleChain->rgpElement[i]->TrustStatus.dwInfoStatus &
             CERT_TRUST_IS_SELF_SIGNED)
                data->pasSigners[idxSigner].pasCertChain[i].fSelfSigned = TRUE;
            if (simpleChain->rgpElement[i]->TrustStatus.dwErrorStatus &
             CERT_TRUST_IS_CYCLIC)
                data->pasSigners[idxSigner].pasCertChain[i].fIsCyclic = TRUE;
        }
    }
    return ret;
}

static BOOL WINTRUST_CopyChain(CRYPT_PROVIDER_DATA *data, DWORD signerIdx)
{
    BOOL ret;
    PCERT_SIMPLE_CHAIN simpleChain =
     data->pasSigners[signerIdx].pChainContext->rgpChain[0];
    DWORD i;

    data->pasSigners[signerIdx].pasCertChain[0].pChainElement =
     simpleChain->rgpElement[0];
    ret = TRUE;
    for (i = 1; ret && i < simpleChain->cElement; i++)
    {
        ret = data->psPfns->pfnAddCert2Chain(data, signerIdx, FALSE, 0,
         simpleChain->rgpElement[i]->pCertContext);
        if (ret)
            data->pasSigners[signerIdx].pasCertChain[i].pChainElement =
             simpleChain->rgpElement[i];
    }
    return ret;
}

HRESULT WINAPI WintrustCertificateTrust(CRYPT_PROVIDER_DATA *data)
{
    BOOL ret;

    if (!data->csSigners)
    {
        ret = FALSE;
        SetLastError(TRUST_E_NOSIGNATURE);
    }
    else
    {
        DWORD i;

        ret = TRUE;
        for (i = 0; i < data->csSigners; i++)
        {
            CERT_CHAIN_PARA chainPara = { sizeof(chainPara), { 0 } };
            DWORD flags;

            if (data->pRequestUsage)
                memcpy(&chainPara.RequestedUsage, data->pRequestUsage,
                 sizeof(CERT_USAGE_MATCH));
            if (data->dwProvFlags & CPD_REVOCATION_CHECK_END_CERT)
                flags = CERT_CHAIN_REVOCATION_CHECK_END_CERT;
            else if (data->dwProvFlags & CPD_REVOCATION_CHECK_CHAIN)
                flags = CERT_CHAIN_REVOCATION_CHECK_CHAIN;
            else if (data->dwProvFlags & CPD_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT)
                flags = CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT;
            else
                flags = 0;
            /* Expect the end certificate for each signer to be the only
             * cert in the chain:
             */
            if (data->pasSigners[i].csCertChain)
            {
                /* Create a certificate chain for each signer */
                ret = CertGetCertificateChain(NULL,
                 data->pasSigners[i].pasCertChain[0].pCert,
                 NULL, /* FIXME: use data->pasSigners[i].sftVerifyAsOf? */
                 data->chStores ? data->pahStores[0] : NULL,
                 &chainPara, flags, NULL, &data->pasSigners[i].pChainContext);
                if (ret)
                {
                    if (data->pasSigners[i].pChainContext->cChain != 1)
                    {
                        FIXME("unimplemented for more than 1 simple chain\n");
                        ret = FALSE;
                    }
                    else
                    {
                        if ((ret = WINTRUST_CopyChain(data, i)))
                            ret = data->psPfns->pfnCertCheckPolicy(data, i,
                             FALSE, 0);
                    }
                }
            }
        }
    }
    if (!ret)
        data->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] =
         GetLastError();
    return ret ? S_OK : S_FALSE;
}
