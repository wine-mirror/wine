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

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winuser.h"
#include "cryptuiapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cryptui);

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
 *		CryptUIDlgCertMgr (CRYPTUI.@)
 */
BOOL WINAPI CryptUIDlgCertMgr(PCCRYPTUI_CERT_MGR_STRUCT pCryptUICertMgr)
{
    FIXME("(%p): stub\n", pCryptUICertMgr);
    return FALSE;
}

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
    ret = CryptUIDlgViewCertificateW(&viewInfo, pfPropertiesChanged);
    HeapFree(GetProcessHeap(), 0, title);
error:
    return ret;
}

BOOL WINAPI CryptUIDlgViewCertificateW(PCCRYPTUI_VIEWCERTIFICATE_STRUCTW pCertViewInfo,
                                       BOOL *pfPropertiesChanged)
{
    FIXME("(%p, %p): stub\n", pCertViewInfo, pfPropertiesChanged);
    if (pfPropertiesChanged) *pfPropertiesChanged = FALSE;
    return TRUE;
}

BOOL WINAPI CryptUIWizImport(DWORD dwFlags, HWND hwndParent, LPCWSTR pwszWizardTitle,
                             PCCRYPTUI_WIZ_IMPORT_SRC_INFO pImportSrc, HCERTSTORE hDestCertStore)
{
    static const WCHAR Root[] = {'R','o','o','t',0};
    BOOL ret;
    HANDLE file;
    HCERTSTORE store;
    BYTE *buffer;
    DWORD size, encoding = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;
    const CERT_CONTEXT *cert;

    TRACE("(0x%08x, %p, %s, %p, %p)\n", dwFlags, hwndParent, debugstr_w(pwszWizardTitle),
          pImportSrc, hDestCertStore);

    FIXME("only certificate files are supported\n");

    if (!(dwFlags & CRYPTUI_WIZ_NO_UI)) FIXME("UI not implemented\n");

    if (!pImportSrc ||
     pImportSrc->dwSize != sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO))
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }

    if (pImportSrc->dwSubjectChoice != CRYPTUI_WIZ_IMPORT_SUBJECT_FILE)
    {
        FIXME("source type not implemented: %u\n", pImportSrc->dwSubjectChoice);
        return FALSE;
    }
    file = CreateFileW(pImportSrc->pwszFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        WARN("can't open certificate file %s\n", debugstr_w(pImportSrc->pwszFileName));
        return FALSE;
    }
    if ((size = GetFileSize(file, NULL)))
    {
        if ((buffer = HeapAlloc(GetProcessHeap(), 0, size)))
        {
            DWORD read;
            if (!ReadFile(file, buffer, size, &read, NULL) || read != size)
            {
                WARN("can't read certificate file %s\n", debugstr_w(pImportSrc->pwszFileName));
                HeapFree(GetProcessHeap(), 0, buffer);
                CloseHandle(file);
                return FALSE;
            }
        }
    }
    else
    {
        WARN("empty file %s\n", debugstr_w(pImportSrc->pwszFileName));
        CloseHandle(file);
        return FALSE;
    }
    CloseHandle(file);
    if (!(cert = CertCreateCertificateContext(encoding, buffer, size)))
    {
        WARN("unable to create certificate context\n");
        HeapFree(GetProcessHeap(), 0, buffer);
        return FALSE;
    }
    if (hDestCertStore) store = hDestCertStore;
    else
    {
        FIXME("certificate store should be determined dynamically, picking Root store\n");
        if (!(store = CertOpenStore(CERT_STORE_PROV_SYSTEM_W, 0, 0, CERT_SYSTEM_STORE_CURRENT_USER, Root)))
        {
            WARN("unable to open certificate store\n");
            CertFreeCertificateContext(cert);
            HeapFree(GetProcessHeap(), 0, buffer);
            return FALSE;
        }
    }
    ret = CertAddCertificateContextToStore(store, cert, CERT_STORE_ADD_REPLACE_EXISTING, NULL);

    if (!hDestCertStore) CertCloseStore(store, 0);
    CertFreeCertificateContext(cert);
    HeapFree(GetProcessHeap(), 0, buffer);
    return ret;
}
