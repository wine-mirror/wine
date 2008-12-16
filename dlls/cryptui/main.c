/*
 * Copyright 2008 Juan Lang
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

#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winuser.h"
#include "softpub.h"
#include "cryptuiapi.h"
#include "cryptuires.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cryptui);

static HINSTANCE hInstance;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            hInstance = hinstDLL;
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
 *		CryptUIDlgCertMgr (CRYPTUI.@)
 */
BOOL WINAPI CryptUIDlgCertMgr(PCCRYPTUI_CERT_MGR_STRUCT pCryptUICertMgr)
{
    FIXME("(%p): stub\n", pCryptUICertMgr);
    return FALSE;
}

/***********************************************************************
 *		CryptUIDlgViewCertificateA (CRYPTUI.@)
 */
BOOL WINAPI CryptUIDlgViewCertificateA(
 PCCRYPTUI_VIEWCERTIFICATE_STRUCTA pCertViewInfo, BOOL *pfPropertiesChanged)
{
    CRYPTUI_VIEWCERTIFICATE_STRUCTW viewInfo;
    LPWSTR title = NULL;
    BOOL ret;

    TRACE("(%p, %p)\n", pCertViewInfo, pfPropertiesChanged);

    memcpy(&viewInfo, pCertViewInfo, sizeof(viewInfo));
    if (pCertViewInfo->szTitle)
    {
        int len = MultiByteToWideChar(CP_ACP, 0, pCertViewInfo->szTitle, -1,
         NULL, 0);

        title = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (title)
        {
            MultiByteToWideChar(CP_ACP, 0, pCertViewInfo->szTitle, -1, title,
             len);
            viewInfo.szTitle = title;
        }
        else
        {
            ret = FALSE;
            goto error;
        }
    }
    if (pCertViewInfo->cPropSheetPages)
    {
        FIXME("ignoring additional prop sheet pages\n");
        viewInfo.cPropSheetPages = 0;
    }
    ret = CryptUIDlgViewCertificateW(&viewInfo, pfPropertiesChanged);
    HeapFree(GetProcessHeap(), 0, title);
error:
    return ret;
}

static LRESULT CALLBACK general_dlg_proc(HWND hwnd, UINT msg, WPARAM wp,
 LPARAM lp)
{
    PROPSHEETPAGEW *page;
    PCCRYPTUI_VIEWCERTIFICATE_STRUCTW pCertViewInfo;

    TRACE("(%p, %08x, %08lx, %08lx)\n", hwnd, msg, wp, lp);

    switch (msg)
    {
    case WM_INITDIALOG:
        page = (PROPSHEETPAGEW *)lp;
        pCertViewInfo = (PCCRYPTUI_VIEWCERTIFICATE_STRUCTW)page->lParam;
        if (pCertViewInfo->dwFlags & CRYPTUI_DISABLE_ADDTOSTORE)
            ShowWindow(GetDlgItem(hwnd, IDC_ADDTOSTORE), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_ISSUERSTATEMENT), FALSE);
        FIXME("show cert properties\n");
        break;
    case WM_COMMAND:
        switch (wp)
        {
        case IDC_ADDTOSTORE:
            FIXME("call CryptUIWizImport\n");
            break;
        }
        break;
    }
    return 0;
}

static void init_general_page(PCCRYPTUI_VIEWCERTIFICATE_STRUCTW pCertViewInfo,
 PROPSHEETPAGEW *page)
{
    memset(page, 0, sizeof(PROPSHEETPAGEW));
    page->dwSize = sizeof(PROPSHEETPAGEW);
    page->hInstance = hInstance;
    page->u.pszTemplate = MAKEINTRESOURCEW(IDD_GENERAL);
    page->pfnDlgProc = general_dlg_proc;
    page->lParam = (LPARAM)pCertViewInfo;
}

static int CALLBACK cert_prop_sheet_proc(HWND hwnd, UINT msg, LPARAM lp)
{
    RECT rc;
    POINT topLeft;

    TRACE("(%p, %08x, %08lx)\n", hwnd, msg, lp);

    switch (msg)
    {
    case PSCB_INITIALIZED:
        /* Get cancel button's position.. */
        GetWindowRect(GetDlgItem(hwnd, IDCANCEL), &rc);
        topLeft.x = rc.left;
        topLeft.y = rc.top;
        ScreenToClient(hwnd, &topLeft);
        /* hide the cancel button.. */
        ShowWindow(GetDlgItem(hwnd, IDCANCEL), FALSE);
        /* get the OK button's size.. */
        GetWindowRect(GetDlgItem(hwnd, IDOK), &rc);
        /* and move the OK button to the cancel button's original position. */
        MoveWindow(GetDlgItem(hwnd, IDOK), topLeft.x, topLeft.y,
         rc.right - rc.left, rc.bottom - rc.top, FALSE);
        GetWindowRect(GetDlgItem(hwnd, IDOK), &rc);
        break;
    }
    return 0;
}

static BOOL show_cert_dialog(PCCRYPTUI_VIEWCERTIFICATE_STRUCTW pCertViewInfo,
 CRYPT_PROVIDER_CERT *provCert, BOOL *pfPropertiesChanged)
{
    static const WCHAR riched[] = { 'r','i','c','h','e','d','2','0',0 };
    DWORD nPages;
    PROPSHEETPAGEW *pages;
    BOOL ret = FALSE;
    HMODULE lib = LoadLibraryW(riched);

    nPages = pCertViewInfo->cPropSheetPages + 1; /* one for the General tab */
    if (!(pCertViewInfo->dwFlags & CRYPTUI_HIDE_DETAILPAGE))
        FIXME("show detail page\n");
    if (!(pCertViewInfo->dwFlags & CRYPTUI_HIDE_HIERARCHYPAGE))
        FIXME("show hierarchy page\n");
    pages = HeapAlloc(GetProcessHeap(), 0, nPages * sizeof(PROPSHEETPAGEW));
    if (pages)
    {
        PROPSHEETHEADERW hdr;
        CRYPTUI_INITDIALOG_STRUCT *init = NULL;
        DWORD i;

        memset(&hdr, 0, sizeof(hdr));
        hdr.dwSize = sizeof(hdr);
        hdr.dwFlags = PSH_NOAPPLYNOW | PSH_PROPSHEETPAGE | PSH_USECALLBACK;
        hdr.hInstance = hInstance;
        if (pCertViewInfo->szTitle)
            hdr.pszCaption = pCertViewInfo->szTitle;
        else
            hdr.pszCaption = MAKEINTRESOURCEW(IDS_CERTIFICATE);
        init_general_page(pCertViewInfo, &pages[hdr.nPages++]);
        /* Copy each additional page, and create the init dialog struct for it
         */
        if (pCertViewInfo->cPropSheetPages)
        {
            init = HeapAlloc(GetProcessHeap(), 0,
             pCertViewInfo->cPropSheetPages *
             sizeof(CRYPTUI_INITDIALOG_STRUCT));
            if (init)
            {
                for (i = 0; i < pCertViewInfo->cPropSheetPages; i++)
                {
                    memcpy(&pages[hdr.nPages + i],
                     &pCertViewInfo->rgPropSheetPages[i],
                     sizeof(PROPSHEETPAGEW));
                    init[i].lParam = pCertViewInfo->rgPropSheetPages[i].lParam;
                    init[i].pCertContext = pCertViewInfo->pCertContext;
                    pages[hdr.nPages + i].lParam = (LPARAM)&init[i];
                }
                if (pCertViewInfo->nStartPage & 0x8000)
                {
                    /* Start page index is relative to the number of default
                     * pages
                     */
                    hdr.u2.nStartPage = pCertViewInfo->nStartPage + hdr.nPages;
                }
                else
                    hdr.u2.nStartPage = pCertViewInfo->nStartPage;
                hdr.nPages = nPages;
                ret = TRUE;
            }
            else
                SetLastError(ERROR_OUTOFMEMORY);
        }
        else
        {
            /* Ignore the relative flag if there aren't any additional pages */
            hdr.u2.nStartPage = pCertViewInfo->nStartPage & 0x7fff;
            ret = TRUE;
        }
        if (ret)
        {
            INT_PTR l;

            hdr.u3.ppsp = pages;
            hdr.pfnCallback = cert_prop_sheet_proc;
            l = PropertySheetW(&hdr);
            if (l == 0)
            {
                SetLastError(ERROR_CANCELLED);
                ret = FALSE;
            }
        }
        HeapFree(GetProcessHeap(), 0, init);
        HeapFree(GetProcessHeap(), 0, pages);
    }
    else
        SetLastError(ERROR_OUTOFMEMORY);
    FreeLibrary(lib);
    return ret;
}

/***********************************************************************
 *		CryptUIDlgViewCertificateW (CRYPTUI.@)
 */
BOOL WINAPI CryptUIDlgViewCertificateW(
 PCCRYPTUI_VIEWCERTIFICATE_STRUCTW pCertViewInfo, BOOL *pfPropertiesChanged)
{
    static GUID generic_cert_verify = WINTRUST_ACTION_GENERIC_CERT_VERIFY;
    CRYPTUI_VIEWCERTIFICATE_STRUCTW viewInfo;
    WINTRUST_DATA wvt;
    WINTRUST_CERT_INFO cert;
    BOOL ret = FALSE;
    CRYPT_PROVIDER_SGNR *signer;
    CRYPT_PROVIDER_CERT *provCert = NULL;

    TRACE("(%p, %p)\n", pCertViewInfo, pfPropertiesChanged);

    if (pCertViewInfo->dwSize != sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCTW))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    /* Make a local copy in case we have to call WinVerifyTrust ourselves */
    memcpy(&viewInfo, pCertViewInfo, sizeof(viewInfo));
    if (!viewInfo.u.hWVTStateData)
    {
        memset(&wvt, 0, sizeof(wvt));
        wvt.cbStruct = sizeof(wvt);
        wvt.dwUIChoice = WTD_UI_NONE;
        if (viewInfo.dwFlags &
         CRYPTUI_ENABLE_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT)
            wvt.fdwRevocationChecks |= WTD_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT;
        if (viewInfo.dwFlags & CRYPTUI_ENABLE_REVOCATION_CHECK_END_CERT)
            wvt.fdwRevocationChecks |= WTD_REVOCATION_CHECK_END_CERT;
        if (viewInfo.dwFlags & CRYPTUI_ENABLE_REVOCATION_CHECK_CHAIN)
            wvt.fdwRevocationChecks |= WTD_REVOCATION_CHECK_CHAIN;
        wvt.dwUnionChoice = WTD_CHOICE_CERT;
        memset(&cert, 0, sizeof(cert));
        cert.cbStruct = sizeof(cert);
        cert.psCertContext = (CERT_CONTEXT *)viewInfo.pCertContext;
        cert.chStores = viewInfo.cStores;
        cert.pahStores = viewInfo.rghStores;
        wvt.u.pCert = &cert;
        wvt.dwStateAction = WTD_STATEACTION_VERIFY;
        WinVerifyTrust(NULL, &generic_cert_verify, &wvt);
        viewInfo.u.pCryptProviderData =
         WTHelperProvDataFromStateData(wvt.hWVTStateData);
        signer = WTHelperGetProvSignerFromChain(
         (CRYPT_PROVIDER_DATA *)viewInfo.u.pCryptProviderData, 0, FALSE, 0);
        provCert = WTHelperGetProvCertFromChain(signer, 0);
        ret = TRUE;
    }
    else
    {
        viewInfo.u.pCryptProviderData =
         WTHelperProvDataFromStateData(viewInfo.u.hWVTStateData);
        signer = WTHelperGetProvSignerFromChain(
         (CRYPT_PROVIDER_DATA *)viewInfo.u.pCryptProviderData,
         viewInfo.idxSigner, viewInfo.fCounterSigner,
         viewInfo.idxCounterSigner);
        provCert = WTHelperGetProvCertFromChain(signer, viewInfo.idxCert);
        ret = TRUE;
    }
    if (ret)
    {
        ret = show_cert_dialog(&viewInfo, provCert, pfPropertiesChanged);
        if (!viewInfo.u.hWVTStateData)
        {
            wvt.dwStateAction = WTD_STATEACTION_CLOSE;
            WinVerifyTrust(NULL, &generic_cert_verify, &wvt);
        }
    }
    return ret;
}

static PCCERT_CONTEXT make_cert_from_file(LPCWSTR fileName)
{
    HANDLE file;
    DWORD size, encoding = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;
    BYTE *buffer;
    PCCERT_CONTEXT cert;

    file = CreateFileW(fileName, GENERIC_READ, FILE_SHARE_READ, NULL,
     OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        WARN("can't open certificate file %s\n", debugstr_w(fileName));
        return NULL;
    }
    if ((size = GetFileSize(file, NULL)))
    {
        if ((buffer = HeapAlloc(GetProcessHeap(), 0, size)))
        {
            DWORD read;
            if (!ReadFile(file, buffer, size, &read, NULL) || read != size)
            {
                WARN("can't read certificate file %s\n", debugstr_w(fileName));
                HeapFree(GetProcessHeap(), 0, buffer);
                CloseHandle(file);
                return NULL;
            }
        }
    }
    else
    {
        WARN("empty file %s\n", debugstr_w(fileName));
        CloseHandle(file);
        return NULL;
    }
    CloseHandle(file);
    cert = CertCreateCertificateContext(encoding, buffer, size);
    HeapFree(GetProcessHeap(), 0, buffer);
    return cert;
}

/* Decodes a cert's basic constraints extension (either szOID_BASIC_CONSTRAINTS
 * or szOID_BASIC_CONSTRAINTS2, whichever is present) to determine if it
 * should be a CA.  If neither extension is present, returns
 * defaultIfNotSpecified.
 */
static BOOL is_ca_cert(PCCERT_CONTEXT cert, BOOL defaultIfNotSpecified)
{
    BOOL isCA = defaultIfNotSpecified;
    PCERT_EXTENSION ext = CertFindExtension(szOID_BASIC_CONSTRAINTS,
     cert->pCertInfo->cExtension, cert->pCertInfo->rgExtension);

    if (ext)
    {
        CERT_BASIC_CONSTRAINTS_INFO *info;
        DWORD size = 0;

        if (CryptDecodeObjectEx(X509_ASN_ENCODING, szOID_BASIC_CONSTRAINTS,
         ext->Value.pbData, ext->Value.cbData, CRYPT_DECODE_ALLOC_FLAG,
         NULL, (LPBYTE)&info, &size))
        {
            if (info->SubjectType.cbData == 1)
                isCA = info->SubjectType.pbData[0] & CERT_CA_SUBJECT_FLAG;
            LocalFree(info);
        }
    }
    else
    {
        ext = CertFindExtension(szOID_BASIC_CONSTRAINTS2,
         cert->pCertInfo->cExtension, cert->pCertInfo->rgExtension);
        if (ext)
        {
            CERT_BASIC_CONSTRAINTS2_INFO info;
            DWORD size = sizeof(CERT_BASIC_CONSTRAINTS2_INFO);

            if (CryptDecodeObjectEx(X509_ASN_ENCODING,
             szOID_BASIC_CONSTRAINTS2, ext->Value.pbData, ext->Value.cbData,
             0, NULL, &info, &size))
                isCA = info.fCA;
        }
    }
    return isCA;
}

static HCERTSTORE choose_store_for_cert(PCCERT_CONTEXT cert)
{
    static const WCHAR AddressBook[] = { 'A','d','d','r','e','s','s',
     'B','o','o','k',0 };
    static const WCHAR CA[] = { 'C','A',0 };
    LPCWSTR storeName;

    if (is_ca_cert(cert, TRUE))
        storeName = CA;
    else
        storeName = AddressBook;
    return CertOpenStore(CERT_STORE_PROV_SYSTEM_W, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER, storeName);
}

BOOL WINAPI CryptUIWizImport(DWORD dwFlags, HWND hwndParent, LPCWSTR pwszWizardTitle,
                             PCCRYPTUI_WIZ_IMPORT_SRC_INFO pImportSrc, HCERTSTORE hDestCertStore)
{
    BOOL ret;
    HCERTSTORE store;
    const CERT_CONTEXT *cert;
    BOOL freeCert = FALSE;

    TRACE("(0x%08x, %p, %s, %p, %p)\n", dwFlags, hwndParent, debugstr_w(pwszWizardTitle),
          pImportSrc, hDestCertStore);

    if (!(dwFlags & CRYPTUI_WIZ_NO_UI)) FIXME("UI not implemented\n");

    if (!pImportSrc ||
     pImportSrc->dwSize != sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO))
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }

    switch (pImportSrc->dwSubjectChoice)
    {
    case CRYPTUI_WIZ_IMPORT_SUBJECT_FILE:
        if (!(cert = make_cert_from_file(pImportSrc->u.pwszFileName)))
        {
            WARN("unable to create certificate context\n");
            return FALSE;
        }
        else
            freeCert = TRUE;
        break;
    case CRYPTUI_WIZ_IMPORT_SUBJECT_CERT_CONTEXT:
        cert = pImportSrc->u.pCertContext;
        if (!cert)
        {
            SetLastError(E_INVALIDARG);
            return FALSE;
        }
        break;
    default:
        FIXME("source type not implemented: %u\n", pImportSrc->dwSubjectChoice);
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    if (hDestCertStore) store = hDestCertStore;
    else
    {
        if (!(store = choose_store_for_cert(cert)))
        {
            WARN("unable to open certificate store\n");
            CertFreeCertificateContext(cert);
            return FALSE;
        }
    }
    ret = CertAddCertificateContextToStore(store, cert, CERT_STORE_ADD_REPLACE_EXISTING, NULL);

    if (!hDestCertStore) CertCloseStore(store, 0);
    if (freeCert)
        CertFreeCertificateContext(cert);
    return ret;
}
