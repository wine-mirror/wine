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
