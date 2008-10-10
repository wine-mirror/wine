/*
 * Copyright 2008 Maarten Lankhorst
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

#define NONAMELESSUNION

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "wincrypt.h"
#include "wintrust.h"
#include "winuser.h"
#include "cryptdlg.h"
#include "cryptuiapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cryptdlg);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
        case DLL_PROCESS_DETACH:
            break;
        default:
            break;
    }
    return TRUE;
}

/***********************************************************************
 *		GetFriendlyNameOfCertA (CRYPTDLG.@)
 */
DWORD WINAPI GetFriendlyNameOfCertA(PCCERT_CONTEXT pccert, LPSTR pchBuffer,
                             DWORD cchBuffer)
{
    return CertGetNameStringA(pccert, CERT_NAME_FRIENDLY_DISPLAY_TYPE, 0, NULL,
     pchBuffer, cchBuffer);
}

/***********************************************************************
 *		GetFriendlyNameOfCertW (CRYPTDLG.@)
 */
DWORD WINAPI GetFriendlyNameOfCertW(PCCERT_CONTEXT pccert, LPWSTR pchBuffer,
                             DWORD cchBuffer)
{
    return CertGetNameStringW(pccert, CERT_NAME_FRIENDLY_DISPLAY_TYPE, 0, NULL,
     pchBuffer, cchBuffer);
}

/***********************************************************************
 *		CertTrustInit (CRYPTDLG.@)
 */
HRESULT WINAPI CertTrustInit(CRYPT_PROVIDER_DATA *pProvData)
{
    HRESULT ret = S_FALSE;

    TRACE("(%p)\n", pProvData);

    if (pProvData->padwTrustStepErrors &&
     !pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_WVTINIT])
        ret = S_OK;
    TRACE("returning %08x\n", ret);
    return ret;
}

/***********************************************************************
 *		CertTrustCertPolicy (CRYPTDLG.@)
 */
BOOL WINAPI CertTrustCertPolicy(CRYPT_PROVIDER_DATA *pProvData, DWORD idxSigner, BOOL fCounterSignerChain, DWORD idxCounterSigner)
{
    FIXME("(%p, %d, %s, %d)\n", pProvData, idxSigner, fCounterSignerChain ? "TRUE" : "FALSE", idxCounterSigner);
    return FALSE;
}

/***********************************************************************
 *		CertTrustCleanup (CRYPTDLG.@)
 */
HRESULT WINAPI CertTrustCleanup(CRYPT_PROVIDER_DATA *pProvData)
{
    FIXME("(%p)\n", pProvData);
    return E_NOTIMPL;
}

static BOOL CRYPTDLG_CheckOnlineCRL(void)
{
    static const WCHAR policyFlagsKey[] = { 'S','o','f','t','w','a','r','e',
     '\\','M','i','c','r','o','s','o','f','t','\\','C','r','y','p','t','o','g',
     'r','a','p','h','y','\\','{','7','8','0','1','e','b','d','0','-','c','f',
     '4','b','-','1','1','d','0','-','8','5','1','f','-','0','0','6','0','9',
     '7','9','3','8','7','e','a','}',0 };
    static const WCHAR policyFlags[] = { 'P','o','l','i','c','y','F','l','a',
     'g','s',0 };
    HKEY key;
    BOOL ret = FALSE;

    if (!RegOpenKeyExW(HKEY_LOCAL_MACHINE, policyFlagsKey, 0, KEY_READ, &key))
    {
        DWORD type, flags, size = sizeof(flags);

        if (!RegQueryValueExW(key, policyFlags, NULL, &type, (BYTE *)&flags,
         &size) && type == REG_DWORD)
        {
            /* The flag values aren't defined in any header I'm aware of, but
             * this value is well documented on the net.
             */
            if (flags & 0x00010000)
                ret = TRUE;
        }
        RegCloseKey(key);
    }
    return ret;
}

/* Returns TRUE if pCert is not in the Disallowed system store, or FALSE if it
 * is.
 */
static BOOL CRYPTDLG_IsCertAllowed(PCCERT_CONTEXT pCert)
{
    BOOL ret;
    BYTE hash[20];
    DWORD size = sizeof(hash);

    if ((ret = CertGetCertificateContextProperty(pCert,
     CERT_SIGNATURE_HASH_PROP_ID, hash, &size)))
    {
        static const WCHAR disallowedW[] =
         { 'D','i','s','a','l','l','o','w','e','d',0 };
        HCERTSTORE disallowed = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
         X509_ASN_ENCODING, 0, CERT_SYSTEM_STORE_CURRENT_USER, disallowedW);

        if (disallowed)
        {
            PCCERT_CONTEXT found = CertFindCertificateInStore(disallowed,
             X509_ASN_ENCODING, 0, CERT_FIND_SIGNATURE_HASH, hash, NULL);

            if (found)
            {
                ret = FALSE;
                CertFreeCertificateContext(found);
            }
            CertCloseStore(disallowed, 0);
        }
    }
    return ret;
}

static DWORD CRYPTDLG_TrustStatusToConfidence(DWORD errorStatus)
{
    DWORD confidence = 0;

    confidence = 0;
    if (!(errorStatus & CERT_TRUST_IS_NOT_SIGNATURE_VALID))
        confidence |= CERT_CONFIDENCE_SIG;
    if (!(errorStatus & CERT_TRUST_IS_NOT_TIME_VALID))
        confidence |= CERT_CONFIDENCE_TIME;
    if (!(errorStatus & CERT_TRUST_IS_NOT_TIME_NESTED))
        confidence |= CERT_CONFIDENCE_TIMENEST;
    return confidence;
}

static BOOL CRYPTDLG_CopyChain(CRYPT_PROVIDER_DATA *data,
 PCCERT_CHAIN_CONTEXT chain)
{
    BOOL ret;
    CRYPT_PROVIDER_SGNR signer;
    PCERT_SIMPLE_CHAIN simpleChain = chain->rgpChain[0];
    DWORD i;

    memset(&signer, 0, sizeof(signer));
    signer.cbStruct = sizeof(signer);
    ret = data->psPfns->pfnAddSgnr2Chain(data, FALSE, 0, &signer);
    if (ret)
    {
        CRYPT_PROVIDER_SGNR *sgnr = WTHelperGetProvSignerFromChain(data, 0,
         FALSE, 0);

        if (sgnr)
        {
            sgnr->dwError = simpleChain->TrustStatus.dwErrorStatus;
            sgnr->pChainContext = CertDuplicateCertificateChain(chain);
        }
        else
            ret = FALSE;
        for (i = 0; ret && i < simpleChain->cElement; i++)
        {
            ret = data->psPfns->pfnAddCert2Chain(data, 0, FALSE, 0,
             simpleChain->rgpElement[i]->pCertContext);
            if (ret)
            {
                CRYPT_PROVIDER_CERT *cert;

                if ((cert = WTHelperGetProvCertFromChain(sgnr, i)))
                {
                    CERT_CHAIN_ELEMENT *element = simpleChain->rgpElement[i];

                    cert->dwConfidence = CRYPTDLG_TrustStatusToConfidence(
                     element->TrustStatus.dwErrorStatus);
                    cert->dwError = element->TrustStatus.dwErrorStatus;
                    cert->pChainElement = element;
                }
                else
                    ret = FALSE;
            }
        }
    }
    return ret;
}

static CERT_VERIFY_CERTIFICATE_TRUST *CRYPTDLG_GetVerifyData(
 CRYPT_PROVIDER_DATA *data)
{
    CERT_VERIFY_CERTIFICATE_TRUST *pCert = NULL;

    /* This should always be true, but just in case the calling function is
     * called directly:
     */
    if (data->pWintrustData->dwUnionChoice == WTD_CHOICE_BLOB &&
     data->pWintrustData->u.pBlob && data->pWintrustData->u.pBlob->cbMemObject ==
     sizeof(CERT_VERIFY_CERTIFICATE_TRUST) &&
     data->pWintrustData->u.pBlob->pbMemObject)
         pCert = (CERT_VERIFY_CERTIFICATE_TRUST *)
          data->pWintrustData->u.pBlob->pbMemObject;
    return pCert;
}

static HCERTCHAINENGINE CRYPTDLG_MakeEngine(CERT_VERIFY_CERTIFICATE_TRUST *cert)
{
    HCERTCHAINENGINE engine = NULL;
    HCERTSTORE root = NULL, trust = NULL;
    DWORD i;

    if (cert->cRootStores)
    {
        root = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, 0,
         CERT_STORE_CREATE_NEW_FLAG, NULL);
        if (root)
        {
            for (i = 0; i < cert->cRootStores; i++)
                CertAddStoreToCollection(root, cert->rghstoreRoots[i], 0, 0);
        }
    }
    if (cert->cTrustStores)
    {
        trust = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, 0,
         CERT_STORE_CREATE_NEW_FLAG, NULL);
        if (root)
        {
            for (i = 0; i < cert->cTrustStores; i++)
                CertAddStoreToCollection(trust, cert->rghstoreTrust[i], 0, 0);
        }
    }
    if (cert->cRootStores || cert->cStores || cert->cTrustStores)
    {
        CERT_CHAIN_ENGINE_CONFIG config;

        memset(&config, 0, sizeof(config));
        config.cbSize = sizeof(config);
        config.hRestrictedRoot = root;
        config.hRestrictedTrust = trust;
        config.cAdditionalStore = cert->cStores;
        config.rghAdditionalStore = cert->rghstoreCAs;
        config.hRestrictedRoot = root;
        CertCreateCertificateChainEngine(&config, &engine);
        CertCloseStore(root, 0);
        CertCloseStore(trust, 0);
    }
    return engine;
}

/***********************************************************************
 *		CertTrustFinalPolicy (CRYPTDLG.@)
 */
HRESULT WINAPI CertTrustFinalPolicy(CRYPT_PROVIDER_DATA *data)
{
    BOOL ret;
    DWORD err = S_OK;
    CERT_VERIFY_CERTIFICATE_TRUST *pCert = CRYPTDLG_GetVerifyData(data);

    TRACE("(%p)\n", data);

    if (data->pWintrustData->dwUIChoice != WTD_UI_NONE)
        FIXME("unimplemented for UI choice %d\n",
         data->pWintrustData->dwUIChoice);
    if (pCert)
    {
        DWORD flags = 0;
        CERT_CHAIN_PARA chainPara;
        HCERTCHAINENGINE engine;

        memset(&chainPara, 0, sizeof(chainPara));
        chainPara.cbSize = sizeof(chainPara);
        if (CRYPTDLG_CheckOnlineCRL())
            flags |= CERT_CHAIN_REVOCATION_CHECK_END_CERT;
        engine = CRYPTDLG_MakeEngine(pCert);
        GetSystemTimeAsFileTime(&data->sftSystemTime);
        ret = CRYPTDLG_IsCertAllowed(pCert->pccert);
        if (ret)
        {
            PCCERT_CHAIN_CONTEXT chain;

            ret = CertGetCertificateChain(engine, pCert->pccert,
             &data->sftSystemTime, NULL, &chainPara, flags, NULL, &chain);
            if (ret)
            {
                if (chain->cChain != 1)
                {
                    FIXME("unimplemented for more than 1 simple chain\n");
                    err = TRUST_E_SUBJECT_FORM_UNKNOWN;
                    ret = FALSE;
                }
                else if ((ret = CRYPTDLG_CopyChain(data, chain)))
                {
                    if (CertVerifyTimeValidity(&data->sftSystemTime,
                     pCert->pccert->pCertInfo))
                    {
                        ret = FALSE;
                        err = CERT_E_EXPIRED;
                    }
                }
                else
                    err = TRUST_E_SYSTEM_ERROR;
                CertFreeCertificateChain(chain);
            }
            else
                err = TRUST_E_SUBJECT_NOT_TRUSTED;
        }
        CertFreeCertificateChainEngine(engine);
    }
    else
    {
        ret = FALSE;
        err = TRUST_E_NOSIGNATURE;
    }
    /* Oddly, native doesn't set the error in the trust step error location,
     * probably because this action is more advisory than anything else.
     * Instead it stores it as the final error, but the function "succeeds" in
     * any case.
     */
    if (!ret)
        data->dwFinalError = err;
    TRACE("returning %d (%08x)\n", S_OK, data->dwFinalError);
    return S_OK;
}

/***********************************************************************
 *		CertViewPropertiesA (CRYPTDLG.@)
 */
BOOL WINAPI CertViewPropertiesA(CERT_VIEWPROPERTIES_STRUCT_A *info)
{
    CERT_VIEWPROPERTIES_STRUCT_W infoW;
    LPWSTR title = NULL;
    BOOL ret;

    TRACE("(%p)\n", info);

    memcpy(&infoW, info, sizeof(infoW));
    if (info->szTitle)
    {
        int len = MultiByteToWideChar(CP_ACP, 0, info->szTitle, -1, NULL, 0);

        title = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (title)
        {
            MultiByteToWideChar(CP_ACP, 0, info->szTitle, -1, title, len);
            infoW.szTitle = title;
        }
        else
        {
            ret = FALSE;
            goto error;
        }
    }
    ret = CertViewPropertiesW(&infoW);
    HeapFree(GetProcessHeap(), 0, title);
error:
    return ret;
}

/***********************************************************************
 *		CertViewPropertiesW (CRYPTDLG.@)
 */
BOOL WINAPI CertViewPropertiesW(CERT_VIEWPROPERTIES_STRUCT_W *info)
{
    static GUID cert_action_verify = CERT_CERTIFICATE_ACTION_VERIFY;
    CERT_VERIFY_CERTIFICATE_TRUST trust;
    WINTRUST_BLOB_INFO blob;
    WINTRUST_DATA wtd;
    LONG err;
    BOOL ret;

    TRACE("(%p)\n", info);

    memset(&trust, 0, sizeof(trust));
    trust.cbSize = sizeof(trust);
    trust.pccert = info->pCertContext;
    trust.cRootStores = info->cRootStores;
    trust.rghstoreRoots = info->rghstoreRoots;
    trust.cStores = info->cStores;
    trust.rghstoreCAs = info->rghstoreCAs;
    trust.cTrustStores = info->cTrustStores;
    trust.rghstoreTrust = info->rghstoreTrust;
    memset(&blob, 0, sizeof(blob));
    blob.cbStruct = sizeof(blob);
    blob.cbMemObject = sizeof(trust);
    blob.pbMemObject = (BYTE *)&trust;
    memset(&wtd, 0, sizeof(wtd));
    wtd.cbStruct = sizeof(wtd);
    wtd.dwUIChoice = WTD_UI_NONE;
    wtd.dwUnionChoice = WTD_CHOICE_BLOB;
    wtd.u.pBlob = &blob;
    wtd.dwStateAction = WTD_STATEACTION_VERIFY;
    err = WinVerifyTrust(NULL, &cert_action_verify, &wtd);
    if (err == ERROR_SUCCESS)
    {
        CRYPTUI_VIEWCERTIFICATE_STRUCTW uiInfo;
        BOOL propsChanged = FALSE;

        memset(&uiInfo, 0, sizeof(uiInfo));
        uiInfo.dwSize = sizeof(uiInfo);
        uiInfo.hwndParent = info->hwndParent;
        uiInfo.dwFlags =
         CRYPTUI_DISABLE_ADDTOSTORE | CRYPTUI_ENABLE_EDITPROPERTIES;
        uiInfo.szTitle = info->szTitle;
        uiInfo.pCertContext = info->pCertContext;
        uiInfo.cPurposes = info->cArrayPurposes;
        uiInfo.rgszPurposes = (LPCSTR *)info->arrayPurposes;
        uiInfo.u.hWVTStateData = wtd.hWVTStateData;
        uiInfo.fpCryptProviderDataTrustedUsage = TRUE;
        uiInfo.cPropSheetPages = info->cArrayPropSheetPages;
        uiInfo.rgPropSheetPages = info->arrayPropSheetPages;
        uiInfo.nStartPage = info->nStartPage;
        ret = CryptUIDlgViewCertificateW(&uiInfo, &propsChanged);
        wtd.dwStateAction = WTD_STATEACTION_CLOSE;
        WinVerifyTrust(NULL, &cert_action_verify, &wtd);
    }
    else
        ret = FALSE;
    return ret;
}

/***********************************************************************
 *		DllRegisterServer (CRYPTDLG.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    static WCHAR cryptdlg[] = { 'c','r','y','p','t','d','l','g','.',
     'd','l','l',0 };
    static WCHAR wintrust[] = { 'w','i','n','t','r','u','s','t','.',
     'd','l','l',0 };
    static WCHAR certTrustInit[] = { 'C','e','r','t','T','r','u','s','t',
     'I','n','i','t',0 };
    static WCHAR wintrustCertificateTrust[] = { 'W','i','n','t','r','u','s','t',
     'C','e','r','t','i','f','i','c','a','t','e','T','r','u','s','t',0 };
    static WCHAR certTrustCertPolicy[] = { 'C','e','r','t','T','r','u','s','t',
     'C','e','r','t','P','o','l','i','c','y',0 };
    static WCHAR certTrustFinalPolicy[] = { 'C','e','r','t','T','r','u','s','t',
     'F','i','n','a','l','P','o','l','i','c','y',0 };
    static WCHAR certTrustCleanup[] = { 'C','e','r','t','T','r','u','s','t',
     'C','l','e','a','n','u','p',0 };
    CRYPT_REGISTER_ACTIONID reg;
    GUID guid = CERT_CERTIFICATE_ACTION_VERIFY;
    HRESULT hr = S_OK;

    memset(&reg, 0, sizeof(reg));
    reg.cbStruct = sizeof(reg);
    reg.sInitProvider.cbStruct = sizeof(CRYPT_TRUST_REG_ENTRY);
    reg.sInitProvider.pwszDLLName = cryptdlg;
    reg.sInitProvider.pwszFunctionName = certTrustInit;
    reg.sCertificateProvider.cbStruct = sizeof(CRYPT_TRUST_REG_ENTRY);
    reg.sCertificateProvider.pwszDLLName = wintrust;
    reg.sCertificateProvider.pwszFunctionName = wintrustCertificateTrust;
    reg.sCertificatePolicyProvider.cbStruct = sizeof(CRYPT_TRUST_REG_ENTRY);
    reg.sCertificatePolicyProvider.pwszDLLName = cryptdlg;
    reg.sCertificatePolicyProvider.pwszFunctionName = certTrustCertPolicy;
    reg.sFinalPolicyProvider.cbStruct = sizeof(CRYPT_TRUST_REG_ENTRY);
    reg.sFinalPolicyProvider.pwszDLLName = cryptdlg;
    reg.sFinalPolicyProvider.pwszFunctionName = certTrustFinalPolicy;
    reg.sCleanupProvider.cbStruct = sizeof(CRYPT_TRUST_REG_ENTRY);
    reg.sCleanupProvider.pwszDLLName = cryptdlg;
    reg.sCleanupProvider.pwszFunctionName = certTrustCleanup;
    if (!WintrustAddActionID(&guid, WT_ADD_ACTION_ID_RET_RESULT_FLAG, &reg))
        hr = GetLastError();
    return hr;
}

/***********************************************************************
 *		DllUnregisterServer (CRYPTDLG.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    GUID guid = CERT_CERTIFICATE_ACTION_VERIFY;

    WintrustRemoveActionID(&guid);
    return S_OK;
}
