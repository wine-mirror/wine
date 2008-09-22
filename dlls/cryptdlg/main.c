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

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
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
    FIXME("(%p)\n", pProvData);
    return E_NOTIMPL;
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

/***********************************************************************
 *		CertTrustFinalPolicy (CRYPTDLG.@)
 */
HRESULT WINAPI CertTrustFinalPolicy(CRYPT_PROVIDER_DATA *pProvData)
{
    FIXME("(%p)\n", pProvData);
    return E_NOTIMPL;
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
    wtd.pBlob = &blob;
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
        uiInfo.hWVTStateData = wtd.hWVTStateData;
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
