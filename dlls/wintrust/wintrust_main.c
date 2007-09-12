/*
 * Copyright 2001 Rein Klazes
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

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "guiddef.h"
#include "wintrust.h"
#include "softpub.h"
#include "mscat.h"
#include "objbase.h"
#include "winuser.h"
#include "wintrust_priv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wintrust);


/***********************************************************************
 *		DllMain  (WINTRUST.@)
 */
BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
    switch(reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( inst );
        break;
    }
    return TRUE;
}

/***********************************************************************
 *		TrustIsCertificateSelfSigned (WINTRUST.@)
 */
BOOL WINAPI TrustIsCertificateSelfSigned( PCCERT_CONTEXT cert )
{
    BOOL ret;

    TRACE("%p\n", cert);
    ret = CertCompareCertificateName(cert->dwCertEncodingType,
     &cert->pCertInfo->Subject, &cert->pCertInfo->Issuer);
    return ret;
}

static LONG WINTRUST_DefaultVerify(HWND hwnd, GUID *actionID,
 WINTRUST_DATA *data)
{
    DWORD err = ERROR_SUCCESS;
    CRYPT_PROVIDER_DATA *provData;
    BOOL ret;

    TRACE("(%p, %s, %p)\n", hwnd, debugstr_guid(actionID), data);

    provData = WINTRUST_Alloc(sizeof(CRYPT_PROVIDER_DATA));
    if (!provData)
        goto oom;
    provData->cbStruct = sizeof(CRYPT_PROVIDER_DATA);

    provData->padwTrustStepErrors =
     WINTRUST_Alloc(TRUSTERROR_MAX_STEPS * sizeof(DWORD));
    if (!provData->padwTrustStepErrors)
        goto oom;
    provData->cdwTrustStepErrors = TRUSTERROR_MAX_STEPS;

    provData->pPDSip = WINTRUST_Alloc(sizeof(PROVDATA_SIP));
    if (!provData->pPDSip)
        goto oom;
    provData->pPDSip->cbStruct = sizeof(PROVDATA_SIP);

    provData->psPfns = WINTRUST_Alloc(sizeof(CRYPT_PROVIDER_FUNCTIONS));
    if (!provData->psPfns)
        goto oom;
    provData->psPfns->cbStruct = sizeof(CRYPT_PROVIDER_FUNCTIONS);
    ret = WintrustLoadFunctionPointers(actionID, provData->psPfns);
    if (!ret)
    {
        err = GetLastError();
        goto error;
    }

    data->hWVTStateData = (HANDLE)provData;
    provData->pWintrustData = data;
    if (hwnd == INVALID_HANDLE_VALUE)
        provData->hWndParent = GetDesktopWindow();
    else
        provData->hWndParent = hwnd;
    provData->pgActionID = actionID;
    WintrustGetRegPolicyFlags(&provData->dwRegPolicySettings);

    err = provData->psPfns->pfnInitialize(provData);
    if (err)
        goto done;
    err = provData->psPfns->pfnObjectTrust(provData);
    if (err)
        goto done;
    err = provData->psPfns->pfnSignatureTrust(provData);
    if (err)
        goto done;
    err = provData->psPfns->pfnCertificateTrust(provData);
    if (err)
        goto done;
    err = provData->psPfns->pfnFinalPolicy(provData);
    goto done;

oom:
    err = ERROR_OUTOFMEMORY;
error:
    if (provData)
    {
        WINTRUST_Free(provData->padwTrustStepErrors);
        WINTRUST_Free(provData->pPDSip);
        WINTRUST_Free(provData->psPfns);
        WINTRUST_Free(provData);
    }
done:
    TRACE("returning %08x\n", err);
    return err;
}

static LONG WINTRUST_DefaultClose(HWND hwnd, GUID *actionID,
 WINTRUST_DATA *data)
{
    DWORD err = ERROR_SUCCESS;
    CRYPT_PROVIDER_DATA *provData = (CRYPT_PROVIDER_DATA *)data->hWVTStateData;

    TRACE("(%p, %s, %p)\n", hwnd, debugstr_guid(actionID), data);

    if (provData)
    {
        err = provData->psPfns->pfnCleanupPolicy(provData);
        WINTRUST_Free(provData->padwTrustStepErrors);
        WINTRUST_Free(provData->pPDSip);
        WINTRUST_Free(provData->psPfns);
        WINTRUST_Free(provData);
        data->hWVTStateData = NULL;
    }
    TRACE("returning %08x\n", err);
    return err;
}

static LONG WINTRUST_DefaultVerifyAndClose(HWND hwnd, GUID *actionID,
 WINTRUST_DATA *data)
{
    LONG err;

    TRACE("(%p, %s, %p)\n", hwnd, debugstr_guid(actionID), data);

    err = WINTRUST_DefaultVerify(hwnd, actionID, data);
    WINTRUST_DefaultClose(hwnd, actionID, data);
    TRACE("returning %08x\n", err);
    return err;
}

static LONG WINTRUST_PublishedSoftware(HWND hwnd, GUID *actionID,
 WINTRUST_DATA *data)
{
    WINTRUST_DATA wintrust_data = { sizeof(wintrust_data), 0 };
    /* Undocumented: the published software action is passed a path,
     * and pSIPClientData points to a WIN_TRUST_SUBJECT_FILE.
     */
    LPCWSTR path = (LPCWSTR)data->pFile;
    LPWIN_TRUST_SUBJECT_FILE subjectFile =
     (LPWIN_TRUST_SUBJECT_FILE)data->pSIPClientData;
    WINTRUST_FILE_INFO fileInfo = { sizeof(fileInfo), 0 };

    TRACE("subjectFile->hFile: %p\n", subjectFile->hFile);
    TRACE("subjectFile->lpPath: %s\n", debugstr_w(subjectFile->lpPath));
    fileInfo.pcwszFilePath = path;
    fileInfo.hFile = subjectFile->hFile;
    wintrust_data.pFile = &fileInfo;
    wintrust_data.dwUnionChoice = WTD_CHOICE_FILE;
    wintrust_data.dwUIChoice = WTD_UI_NONE;

    return WINTRUST_DefaultVerifyAndClose(hwnd, actionID, &wintrust_data);
}

/***********************************************************************
 *		WinVerifyTrust (WINTRUST.@)
 *
 * Verifies an object by calling the specified trust provider.
 *
 * PARAMS
 *   hwnd       [I] Handle to a caller window.
 *   ActionID   [I] Pointer to a GUID that identifies the action to perform.
 *   ActionData [I] Information used by the trust provider to verify the object.
 *
 * RETURNS
 *   Success: Zero.
 *   Failure: A TRUST_E_* error code.
 *
 * NOTES
 *   Trust providers can be found at:
 *   HKLM\SOFTWARE\Microsoft\Cryptography\Providers\Trust\
 */
LONG WINAPI WinVerifyTrust( HWND hwnd, GUID *ActionID, LPVOID ActionData )
{
    static const GUID unknown = { 0xC689AAB8, 0x8E78, 0x11D0, { 0x8C,0x47,
     0x00,0xC0,0x4F,0xC2,0x95,0xEE } };
    static const GUID published_software = WIN_SPUB_ACTION_PUBLISHED_SOFTWARE;
    static const GUID generic_verify_v2 = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    LONG err = ERROR_SUCCESS;
    WINTRUST_DATA *actionData = (WINTRUST_DATA *)ActionData;

    TRACE("(%p, %s, %p)\n", hwnd, debugstr_guid(ActionID), ActionData);

    /* Support for known old-style callers: */
    if (IsEqualGUID(ActionID, &published_software))
        err = WINTRUST_PublishedSoftware(hwnd, ActionID, ActionData);
    else
    {
        /* Check known actions to warn of possible problems */
        if (!IsEqualGUID(ActionID, &unknown) &&
         !IsEqualGUID(ActionID, &generic_verify_v2))
            WARN("unknown action %s, default behavior may not be right\n",
             debugstr_guid(ActionID));
        switch (actionData->dwStateAction)
        {
        case WTD_STATEACTION_IGNORE:
            err = WINTRUST_DefaultVerifyAndClose(hwnd, ActionID, ActionData);
            break;
        case WTD_STATEACTION_VERIFY:
            err = WINTRUST_DefaultVerify(hwnd, ActionID, ActionData);
            break;
        case WTD_STATEACTION_CLOSE:
            err = WINTRUST_DefaultClose(hwnd, ActionID, ActionData);
            break;
        default:
            FIXME("unimplemented for %d\n", actionData->dwStateAction);
        }
    }

    TRACE("returning %08x\n", err);
    return err;
}

/***********************************************************************
 *		WinVerifyTrustEx (WINTRUST.@)
 */
HRESULT WINAPI WinVerifyTrustEx( HWND hwnd, GUID *ActionID,
 WINTRUST_DATA* ActionData )
{
    return WinVerifyTrust(hwnd, ActionID, ActionData);
}

/***********************************************************************
 *		WTHelperGetProvSignerFromChain (WINTRUST.@)
 */
CRYPT_PROVIDER_SGNR * WINAPI WTHelperGetProvSignerFromChain(
 CRYPT_PROVIDER_DATA *pProvData, DWORD idxSigner, BOOL fCounterSigner,
 DWORD idxCounterSigner)
{
    CRYPT_PROVIDER_SGNR *sgnr;

    TRACE("(%p %d %d %d)\n", pProvData, idxSigner, fCounterSigner,
     idxCounterSigner);

    if (idxSigner >= pProvData->csSigners || !pProvData->pasSigners)
        return NULL;
    sgnr = &pProvData->pasSigners[idxSigner];
    if (fCounterSigner)
    {
        if (idxCounterSigner >= sgnr->csCounterSigners ||
         !sgnr->pasCounterSigners)
            return NULL;
        sgnr = &sgnr->pasCounterSigners[idxCounterSigner];
    }
    TRACE("returning %p\n", sgnr);
    return sgnr;
}

/***********************************************************************
 *		WTHelperGetProvCertFromChain (WINTRUST.@)
 */
CRYPT_PROVIDER_CERT * WINAPI WTHelperGetProvCertFromChain(
 CRYPT_PROVIDER_SGNR *pSgnr, DWORD idxCert)
{
    CRYPT_PROVIDER_CERT *cert;

    TRACE("(%p %d)\n", pSgnr, idxCert);

    if (idxCert >= pSgnr->csCertChain || !pSgnr->pasCertChain)
        return NULL;
    cert = &pSgnr->pasCertChain[idxCert];
    TRACE("returning %p\n", cert);
    return cert;
}

/***********************************************************************
 *		WTHelperProvDataFromStateData (WINTRUST.@)
 */
CRYPT_PROVIDER_DATA * WINAPI WTHelperProvDataFromStateData(HANDLE hStateData)
{
    TRACE("%p\n", hStateData);
    return (CRYPT_PROVIDER_DATA *)hStateData;
}

static const WCHAR Software_Publishing[] = {
 'S','o','f','t','w','a','r','e','\\',
 'M','i','c','r','o','s','o','f','t','\\',
 'W','i','n','d','o','w','s','\\',
 'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
 'W','i','n','t','r','u','s','t','\\',
 'T','r','u','s','t',' ','P','r','o','v','i','d','e','r','s','\\',
 'S','o','f','t','w','a','r','e',' ',
 'P','u','b','l','i','s','h','i','n','g',0 };
static const WCHAR State[] = { 'S','t','a','t','e',0 };

/***********************************************************************
 *		WintrustGetRegPolicyFlags (WINTRUST.@)
 */
void WINAPI WintrustGetRegPolicyFlags( DWORD* pdwPolicyFlags )
{
    HKEY key;
    LONG r;

    TRACE("%p\n", pdwPolicyFlags);

    *pdwPolicyFlags = 0;
    r = RegCreateKeyExW(HKEY_CURRENT_USER, Software_Publishing, 0, NULL, 0,
     KEY_READ, NULL, &key, NULL);
    if (!r)
    {
        DWORD size = sizeof(DWORD);

        r = RegQueryValueExW(key, State, NULL, NULL, (LPBYTE)pdwPolicyFlags,
         &size);
        RegCloseKey(key);
        if (r)
        {
            /* Failed to query, create and return default value */
            *pdwPolicyFlags = WTPF_IGNOREREVOCATIONONTS |
             WTPF_OFFLINEOKNBU_COM |
             WTPF_OFFLINEOKNBU_IND |
             WTPF_OFFLINEOK_COM |
             WTPF_OFFLINEOK_IND;
            WintrustSetRegPolicyFlags(*pdwPolicyFlags);
        }
    }
}

/***********************************************************************
 *		WintrustSetRegPolicyFlags (WINTRUST.@)
 */
BOOL WINAPI WintrustSetRegPolicyFlags( DWORD dwPolicyFlags)
{
    HKEY key;
    LONG r;

    TRACE("%x\n", dwPolicyFlags);

    r = RegCreateKeyExW(HKEY_CURRENT_USER, Software_Publishing, 0,
     NULL, 0, KEY_WRITE, NULL, &key, NULL);
    if (!r)
    {
        r = RegSetValueExW(key, State, 0, REG_DWORD, (LPBYTE)&dwPolicyFlags,
         sizeof(DWORD));
        RegCloseKey(key);
    }
    if (r) SetLastError(r);
    return r == ERROR_SUCCESS;
}

/* Utility functions */
void * WINAPI WINTRUST_Alloc(DWORD cb)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb);
}

void * WINAPI WINTRUST_ReAlloc(void *ptr, DWORD cb)
{
    return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ptr, cb);
}

void WINAPI WINTRUST_Free(void *p)
{
    HeapFree(GetProcessHeap(), 0, p);
}

BOOL WINAPI WINTRUST_AddStore(CRYPT_PROVIDER_DATA *data, HCERTSTORE store)
{
    BOOL ret = FALSE;

    if (data->chStores)
        data->pahStores = WINTRUST_ReAlloc(data->pahStores,
         (data->chStores + 1) * sizeof(HCERTSTORE));
    else
    {
        data->pahStores = WINTRUST_Alloc(sizeof(HCERTSTORE));
        data->chStores = 0;
    }
    if (data->pahStores)
    {
        data->pahStores[data->chStores++] = CertDuplicateStore(store);
        ret = TRUE;
    }
    else
        SetLastError(ERROR_OUTOFMEMORY);
    return ret;
}

BOOL WINAPI WINTRUST_AddSgnr(CRYPT_PROVIDER_DATA *data,
 BOOL fCounterSigner, DWORD idxSigner, CRYPT_PROVIDER_SGNR *sgnr)
{
    BOOL ret = FALSE;

    if (sgnr->cbStruct > sizeof(CRYPT_PROVIDER_SGNR))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (fCounterSigner)
    {
        FIXME("unimplemented for counter signers\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (data->csSigners)
        data->pasSigners = WINTRUST_ReAlloc(data->pasSigners,
         (data->csSigners + 1) * sizeof(CRYPT_PROVIDER_SGNR));
    else
    {
        data->pasSigners = WINTRUST_Alloc(sizeof(CRYPT_PROVIDER_SGNR));
        data->csSigners = 0;
    }
    if (data->pasSigners)
    {
        if (idxSigner < data->csSigners)
            memmove(&data->pasSigners[idxSigner],
             &data->pasSigners[idxSigner + 1],
             (data->csSigners - idxSigner) * sizeof(CRYPT_PROVIDER_SGNR));
        ret = TRUE;
        if (sgnr->cbStruct == sizeof(CRYPT_PROVIDER_SGNR))
        {
            /* The PSDK says psSigner should be allocated using pfnAlloc, but
             * it doesn't say anything about ownership.  Since callers are
             * internal, assume ownership is passed, and just store the
             * pointer.
             */
            memcpy(&data->pasSigners[idxSigner], sgnr,
             sizeof(CRYPT_PROVIDER_SGNR));
        }
        else
            memset(&data->pasSigners[idxSigner], 0,
             sizeof(CRYPT_PROVIDER_SGNR));
        data->csSigners++;
    }
    else
        SetLastError(ERROR_OUTOFMEMORY);
    return ret;
}

BOOL WINAPI WINTRUST_AddCert(CRYPT_PROVIDER_DATA *data, DWORD idxSigner,
 BOOL fCounterSigner, DWORD idxCounterSigner, PCCERT_CONTEXT pCert2Add)
{
    BOOL ret = FALSE;

    if (fCounterSigner)
    {
        FIXME("unimplemented for counter signers\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (data->pasSigners[idxSigner].csCertChain)
        data->pasSigners[idxSigner].pasCertChain =
         WINTRUST_ReAlloc(data->pasSigners[idxSigner].pasCertChain,
         (data->pasSigners[idxSigner].csCertChain + 1) *
         sizeof(CRYPT_PROVIDER_CERT));
    else
    {
        data->pasSigners[idxSigner].pasCertChain =
         WINTRUST_Alloc(sizeof(CRYPT_PROVIDER_CERT));
        data->pasSigners[idxSigner].csCertChain = 0;
    }
    if (data->pasSigners[idxSigner].pasCertChain)
    {
        CRYPT_PROVIDER_CERT *cert = &data->pasSigners[idxSigner].pasCertChain[
         data->pasSigners[idxSigner].csCertChain];

        cert->cbStruct = sizeof(CRYPT_PROVIDER_CERT);
        cert->pCert = CertDuplicateCertificateContext(pCert2Add);
        data->pasSigners[idxSigner].csCertChain++;
        ret = TRUE;
    }
    else
        SetLastError(ERROR_OUTOFMEMORY);
    return ret;
}
