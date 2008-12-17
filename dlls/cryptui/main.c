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

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winuser.h"
#include "softpub.h"
#include "wingdi.h"
#include "richedit.h"
#include "ole2.h"
#include "richole.h"
#include "cryptuiapi.h"
#include "cryptuires.h"
#include "wine/debug.h"
#include "wine/unicode.h"

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

struct ReadStringStruct
{
    LPCWSTR buf;
    LONG pos;
    LONG len;
};

static DWORD CALLBACK read_text_callback(DWORD_PTR dwCookie, LPBYTE buf,
 LONG cb, LONG *pcb)
{
    struct ReadStringStruct *string = (struct ReadStringStruct *)dwCookie;
    LONG cch = min(cb / sizeof(WCHAR), string->len - string->pos);

    TRACE("(%p, %p, %d, %p)\n", string, buf, cb, pcb);

    memmove(buf, string->buf + string->pos, cch * sizeof(WCHAR));
    string->pos += cch;
    *pcb = cch * sizeof(WCHAR);
    return 0;
}

static void add_unformatted_text_to_control(HWND hwnd, LPCWSTR text, LONG len)
{
    struct ReadStringStruct string;
    EDITSTREAM editstream;

    TRACE("(%p, %s)\n", hwnd, debugstr_wn(text, len));

    string.buf = text;
    string.pos = 0;
    string.len = len;
    editstream.dwCookie = (DWORD_PTR)&string;
    editstream.dwError = 0;
    editstream.pfnCallback = read_text_callback;
    SendMessageW(hwnd, EM_STREAMIN, SF_TEXT | SFF_SELECTION | SF_UNICODE,
     (LPARAM)&editstream);
}

static void add_string_resource_to_control(HWND hwnd, int id)
{
    LPWSTR str;
    LONG len;

    len = LoadStringW(hInstance, id, (LPWSTR)&str, 0);
    add_unformatted_text_to_control(hwnd, str, len);
}

static void add_text_with_paraformat_to_control(HWND hwnd, LPCWSTR text,
 LONG len, const PARAFORMAT2 *fmt)
{
    add_unformatted_text_to_control(hwnd, text, len);
    SendMessageW(hwnd, EM_SETPARAFORMAT, 0, (LPARAM)fmt);
}

static void add_string_resource_with_paraformat_to_control(HWND hwnd, int id,
 const PARAFORMAT2 *fmt)
{
    LPWSTR str;
    LONG len;

    len = LoadStringW(hInstance, id, (LPWSTR)&str, 0);
    add_text_with_paraformat_to_control(hwnd, str, len, fmt);
}

static LPWSTR get_cert_name_string(PCCERT_CONTEXT pCertContext, DWORD dwType,
 DWORD dwFlags)
{
    LPWSTR buf = NULL;
    DWORD len;

    len = CertGetNameStringW(pCertContext, dwType, dwFlags, NULL, NULL, 0);
    if (len)
    {
        buf = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (buf)
            CertGetNameStringW(pCertContext, dwType, dwFlags, NULL, buf, len);
    }
    return buf;
}

static void add_cert_string_to_control(HWND hwnd, PCCERT_CONTEXT pCertContext,
 DWORD dwType, DWORD dwFlags)
{
    LPWSTR name = get_cert_name_string(pCertContext, dwType, dwFlags);

    if (name)
    {
        /* Don't include NULL-terminator in output */
        DWORD len = lstrlenW(name);

        add_unformatted_text_to_control(hwnd, name, len);
        HeapFree(GetProcessHeap(), 0, name);
    }
}

static void add_icon_to_control(HWND hwnd, int id)
{
    HRESULT hr;
    LPRICHEDITOLE richEditOle = NULL;
    LPOLEOBJECT object = NULL;
    CLSID clsid;
    LPOLECACHE oleCache = NULL;
    FORMATETC formatEtc;
    DWORD conn;
    LPDATAOBJECT dataObject = NULL;
    HBITMAP bitmap = NULL;
    RECT rect;
    STGMEDIUM stgm;
    REOBJECT reObject;

    TRACE("(%p, %d)\n", hwnd, id);

    SendMessageW(hwnd, EM_GETOLEINTERFACE, 0, (LPARAM)&richEditOle);
    if (!richEditOle)
        goto end;
    hr = OleCreateDefaultHandler(&CLSID_NULL, NULL, &IID_IOleObject,
     (void**)&object);
    if (FAILED(hr))
        goto end;
    hr = IOleObject_GetUserClassID(object, &clsid);
    if (FAILED(hr))
        goto end;
    hr = IOleObject_QueryInterface(object, &IID_IOleCache, (void**)&oleCache);
    if (FAILED(hr))
        goto end;
    formatEtc.cfFormat = CF_BITMAP;
    formatEtc.ptd = NULL;
    formatEtc.dwAspect = DVASPECT_CONTENT;
    formatEtc.lindex = -1;
    formatEtc.tymed = TYMED_GDI;
    hr = IOleCache_Cache(oleCache, &formatEtc, 0, &conn);
    if (FAILED(hr))
        goto end;
    hr = IOleObject_QueryInterface(object, &IID_IDataObject,
     (void**)&dataObject);
    if (FAILED(hr))
        goto end;
    bitmap = LoadImageW(hInstance, MAKEINTRESOURCEW(id), IMAGE_BITMAP, 0, 0,
     LR_DEFAULTSIZE | LR_LOADTRANSPARENT);
    if (!bitmap)
        goto end;
    rect.left = rect.top = 0;
    rect.right = GetSystemMetrics(SM_CXICON);
    rect.bottom = GetSystemMetrics(SM_CYICON);
    stgm.tymed = TYMED_GDI;
    stgm.u.hBitmap = bitmap;
    stgm.pUnkForRelease = NULL;
    hr = IDataObject_SetData(dataObject, &formatEtc, &stgm, TRUE);
    if (FAILED(hr))
        goto end;

    reObject.cbStruct = sizeof(reObject);
    reObject.cp = REO_CP_SELECTION;
    reObject.clsid = clsid;
    reObject.poleobj = object;
    reObject.pstg = NULL;
    reObject.polesite = NULL;
    reObject.sizel.cx = reObject.sizel.cy = 0;
    reObject.dvaspect = DVASPECT_CONTENT;
    reObject.dwFlags = 0;
    reObject.dwUser = 0;

    IRichEditOle_InsertObject(richEditOle, &reObject);

end:
    if (dataObject)
        IDataObject_Release(dataObject);
    if (oleCache)
        IOleCache_Release(oleCache);
    if (object)
        IOleObject_Release(object);
    if (richEditOle)
        IRichEditOle_Release(richEditOle);
}

#define MY_INDENT 200

static void add_oid_text_to_control(HWND hwnd, char *oid)
{
    WCHAR nl = '\n';
    PCCRYPT_OID_INFO oidInfo = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY, oid, 0);
    PARAFORMAT2 parFmt;

    parFmt.cbSize = sizeof(parFmt);
    parFmt.dwMask = PFM_STARTINDENT;
    parFmt.dxStartIndent = MY_INDENT * 3;
    if (oidInfo)
    {
        add_text_with_paraformat_to_control(hwnd, oidInfo->pwszName,
         lstrlenW(oidInfo->pwszName), &parFmt);
        add_unformatted_text_to_control(hwnd, &nl, 1);
    }
}

#define MAX_STRING_LEN 512

struct OIDToString
{
    LPCSTR oid;
    int    id;
};

/* The following list MUST be lexicographically sorted by OID */
static struct OIDToString oidMap[] = {
 /* 1.3.6.1.4.1.311.10.3.1 */
 { szOID_KP_CTL_USAGE_SIGNING, IDS_PURPOSE_CTL_USAGE_SIGNING },
 /* 1.3.6.1.4.1.311.10.3.4 */
 { szOID_KP_EFS, IDS_PURPOSE_EFS },
 /* 1.3.6.1.4.1.311.10.3.4.1 */
 { szOID_EFS_RECOVERY, IDS_PURPOSE_EFS_RECOVERY },
 /* 1.3.6.1.4.1.311.10.3.5 */
 { szOID_WHQL_CRYPTO, IDS_PURPOSE_WHQL },
 /* 1.3.6.1.4.1.311.10.3.6 */
 { szOID_NT5_CRYPTO, IDS_PURPOSE_NT5 },
 /* 1.3.6.1.4.1.311.10.3.7 */
 { szOID_OEM_WHQL_CRYPTO, IDS_PURPOSE_OEM_WHQL },
 /* 1.3.6.1.4.1.311.10.3.8 */
 { szOID_EMBEDDED_NT_CRYPTO, IDS_PURPOSE_EMBEDDED_NT },
 /* 1.3.6.1.4.1.311.10.3.9 */
 { szOID_ROOT_LIST_SIGNER, IDS_PURPOSE_ROOT_LIST_SIGNER },
 /* 1.3.6.1.4.1.311.10.3.10 */
 { szOID_KP_QUALIFIED_SUBORDINATION, IDS_PURPOSE_QUALIFIED_SUBORDINATION },
 /* 1.3.6.1.4.1.311.10.3.11 */
 { szOID_KP_KEY_RECOVERY, IDS_PURPOSE_KEY_RECOVERY },
 /* 1.3.6.1.4.1.311.10.3.12 */
 { szOID_KP_DOCUMENT_SIGNING, IDS_PURPOSE_DOCUMENT_SIGNING },
 /* 1.3.6.1.4.1.311.10.3.13 */
 { szOID_KP_LIFETIME_SIGNING, IDS_PURPOSE_LIFETIME_SIGNING },
 /* 1.3.6.1.4.1.311.10.5.1 */
 { szOID_DRM, IDS_PURPOSE_DRM },
 /* 1.3.6.1.4.1.311.10.6.1 */
 { szOID_LICENSES, IDS_PURPOSE_LICENSES },
 /* 1.3.6.1.4.1.311.10.6.2 */
 { szOID_LICENSE_SERVER, IDS_PURPOSE_LICENSE_SERVER },
 /* 1.3.6.1.4.1.311.20.2.1 */
 { szOID_ENROLLMENT_AGENT, IDS_PURPOSE_ENROLLMENT_AGENT },
 /* 1.3.6.1.4.1.311.20.2.2 */
 { szOID_KP_SMARTCARD_LOGON, IDS_PURPOSE_SMARTCARD_LOGON },
 /* 1.3.6.1.4.1.311.21.5 */
 { szOID_KP_CA_EXCHANGE, IDS_PURPOSE_CA_EXCHANGE },
 /* 1.3.6.1.4.1.311.21.6 */
 { szOID_KP_KEY_RECOVERY_AGENT, IDS_PURPOSE_KEY_RECOVERY_AGENT },
 /* 1.3.6.1.4.1.311.21.19 */
 { szOID_DS_EMAIL_REPLICATION, IDS_PURPOSE_DS_EMAIL_REPLICATION },
 /* 1.3.6.1.5.5.7.3.1 */
 { szOID_PKIX_KP_SERVER_AUTH, IDS_PURPOSE_SERVER_AUTH },
 /* 1.3.6.1.5.5.7.3.2 */
 { szOID_PKIX_KP_CLIENT_AUTH, IDS_PURPOSE_CLIENT_AUTH },
 /* 1.3.6.1.5.5.7.3.3 */
 { szOID_PKIX_KP_CODE_SIGNING, IDS_PURPOSE_CODE_SIGNING },
 /* 1.3.6.1.5.5.7.3.4 */
 { szOID_PKIX_KP_EMAIL_PROTECTION, IDS_PURPOSE_EMAIL_PROTECTION },
 /* 1.3.6.1.5.5.7.3.5 */
 { szOID_PKIX_KP_IPSEC_END_SYSTEM, IDS_PURPOSE_IPSEC },
 /* 1.3.6.1.5.5.7.3.6 */
 { szOID_PKIX_KP_IPSEC_TUNNEL, IDS_PURPOSE_IPSEC },
 /* 1.3.6.1.5.5.7.3.7 */
 { szOID_PKIX_KP_IPSEC_USER, IDS_PURPOSE_IPSEC },
 /* 1.3.6.1.5.5.7.3.8 */
 { szOID_PKIX_KP_TIMESTAMP_SIGNING, IDS_PURPOSE_TIMESTAMP_SIGNING },
};

static struct OIDToString *findSupportedOID(LPCSTR oid)
{
    int indexHigh = sizeof(oidMap) / sizeof(oidMap[0]) - 1, indexLow = 0, i;
    struct OIDToString *ret = NULL;

    for (i = (indexLow + indexHigh) / 2; !ret && indexLow <= indexHigh;
     i = (indexLow + indexHigh) / 2)
    {
        int cmp;

        cmp = strcmp(oid, oidMap[i].oid);
        if (!cmp)
            ret = &oidMap[i];
        else if (cmp > 0)
            indexLow = i + 1;
        else
            indexHigh = i - 1;
    }
    return ret;
}

static void add_local_oid_text_to_control(HWND text, LPCSTR oid)
{
    struct OIDToString *entry;
    WCHAR nl = '\n';
    PARAFORMAT2 parFmt;

    parFmt.cbSize = sizeof(parFmt);
    parFmt.dwMask = PFM_STARTINDENT;
    parFmt.dxStartIndent = MY_INDENT * 3;
    if ((entry = findSupportedOID(oid)))
    {
        WCHAR *str, *linebreak, *ptr;
        BOOL multiline = FALSE;
        int len;

        len = LoadStringW(hInstance, entry->id, (LPWSTR)&str, 0);
        ptr = str;
        do {
            if ((linebreak = memchrW(ptr, '\n', len)))
            {
                WCHAR copy[MAX_STRING_LEN];

                multiline = TRUE;
                /* The source string contains a newline, which the richedit
                 * control won't find since it's interpreted as a paragraph
                 * break.  Therefore copy up to the newline.  lstrcpynW always
                 * NULL-terminates, so pass one more than the length of the
                 * source line so the copy includes the entire line and the
                 * NULL-terminator.
                 */
                lstrcpynW(copy, ptr, linebreak - ptr + 1);
                add_text_with_paraformat_to_control(text, copy,
                 linebreak - ptr, &parFmt);
                ptr = linebreak + 1;
                add_unformatted_text_to_control(text, &nl, 1);
            }
            else if (multiline && *ptr)
            {
                /* Add the last line */
                add_text_with_paraformat_to_control(text, ptr,
                 len - (ptr - str), &parFmt);
                add_unformatted_text_to_control(text, &nl, 1);
            }
        } while (linebreak);
        if (!multiline)
        {
            add_text_with_paraformat_to_control(text, str, len, &parFmt);
            add_unformatted_text_to_control(text, &nl, 1);
        }
    }
    else
    {
        WCHAR *oidW = HeapAlloc(GetProcessHeap(), 0,
         (strlen(oid) + 1) * sizeof(WCHAR));

        if (oidW)
        {
            LPCSTR src;
            WCHAR *dst;

            for (src = oid, dst = oidW; *src; src++, dst++)
                *dst = *src;
            *dst = 0;
            add_text_with_paraformat_to_control(text, oidW, lstrlenW(oidW),
             &parFmt);
            add_unformatted_text_to_control(text, &nl, 1);
            HeapFree(GetProcessHeap(), 0, oidW);
        }
    }
}

static void display_app_usages(HWND text, PCCERT_CONTEXT cert,
 BOOL *anyUsageAdded)
{
    static char any_app_policy[] = szOID_ANY_APPLICATION_POLICY;
    WCHAR nl = '\n';
    CHARFORMATW charFmt;
    PCERT_EXTENSION policyExt;
    if (!*anyUsageAdded)
    {
        PARAFORMAT2 parFmt;

        parFmt.cbSize = sizeof(parFmt);
        parFmt.dwMask = PFM_STARTINDENT;
        parFmt.dxStartIndent = MY_INDENT;
        add_string_resource_with_paraformat_to_control(text,
         IDS_CERT_INFO_PURPOSES, &parFmt);
        add_unformatted_text_to_control(text, &nl, 1);
        *anyUsageAdded = TRUE;
    }
    memset(&charFmt, 0, sizeof(charFmt));
    charFmt.cbSize = sizeof(charFmt);
    charFmt.dwMask = CFM_BOLD;
    charFmt.dwEffects = 0;
    SendMessageW(text, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&charFmt);
    if ((policyExt = CertFindExtension(szOID_APPLICATION_CERT_POLICIES,
     cert->pCertInfo->cExtension, cert->pCertInfo->rgExtension)))
    {
        CERT_POLICIES_INFO *policies;
        DWORD size;

        if (CryptDecodeObjectEx(X509_ASN_ENCODING, X509_CERT_POLICIES,
         policyExt->Value.pbData, policyExt->Value.cbData,
         CRYPT_DECODE_ALLOC_FLAG, NULL, &policies, &size))
        {
            DWORD i;

            for (i = 0; i < policies->cPolicyInfo; i++)
            {
                DWORD j;

                for (j = 0; j < policies->rgPolicyInfo[i].cPolicyQualifier; j++)
                    add_local_oid_text_to_control(text,
                     policies->rgPolicyInfo[i].rgPolicyQualifier[j].
                     pszPolicyQualifierId);
            }
            LocalFree(policies);
        }
    }
    else
        add_oid_text_to_control(text, any_app_policy);
}

static BOOL display_cert_usages(HWND text, PCCERT_CONTEXT cert,
 BOOL *anyUsageAdded)
{
    WCHAR nl = '\n';
    DWORD size;
    BOOL badUsages = FALSE;

    if (CertGetEnhancedKeyUsage(cert, 0, NULL, &size))
    {
        CHARFORMATW charFmt;
        static char any_cert_policy[] = szOID_ANY_CERT_POLICY;
        PCERT_ENHKEY_USAGE usage = HeapAlloc(GetProcessHeap(), 0, size);

        if (usage)
        {
            if (CertGetEnhancedKeyUsage(cert, 0, usage, &size))
            {
                DWORD i;

                if (!*anyUsageAdded)
                {
                    PARAFORMAT2 parFmt;

                    parFmt.cbSize = sizeof(parFmt);
                    parFmt.dwMask = PFM_STARTINDENT;
                    parFmt.dxStartIndent = MY_INDENT;
                    add_string_resource_with_paraformat_to_control(text,
                     IDS_CERT_INFO_PURPOSES, &parFmt);
                    add_unformatted_text_to_control(text, &nl, 1);
                    *anyUsageAdded = TRUE;
                }
                memset(&charFmt, 0, sizeof(charFmt));
                charFmt.cbSize = sizeof(charFmt);
                charFmt.dwMask = CFM_BOLD;
                charFmt.dwEffects = 0;
                SendMessageW(text, EM_SETCHARFORMAT, SCF_SELECTION,
                 (LPARAM)&charFmt);
                if (!usage->cUsageIdentifier)
                    add_oid_text_to_control(text, any_cert_policy);
                else
                    for (i = 0; i < usage->cUsageIdentifier; i++)
                        add_local_oid_text_to_control(text,
                         usage->rgpszUsageIdentifier[i]);
            }
            else
                badUsages = TRUE;
            HeapFree(GetProcessHeap(), 0, usage);
        }
        else
            badUsages = TRUE;
    }
    else
        badUsages = TRUE;
    return badUsages;
}

static void set_policy_text(HWND text,
 PCCRYPTUI_VIEWCERTIFICATE_STRUCTW pCertViewInfo)
{
    BOOL includeCertUsages = FALSE, includeAppUsages = FALSE;
    BOOL badUsages = FALSE, anyUsageAdded = FALSE;

    if (pCertViewInfo->cPurposes)
    {
        DWORD i;

        for (i = 0; i < pCertViewInfo->cPurposes; i++)
        {
            if (!strcmp(pCertViewInfo->rgszPurposes[i], szOID_ANY_CERT_POLICY))
                includeCertUsages = TRUE;
            else if (!strcmp(pCertViewInfo->rgszPurposes[i],
             szOID_ANY_APPLICATION_POLICY))
                includeAppUsages = TRUE;
            else
                badUsages = TRUE;
        }
    }
    else
        includeAppUsages = includeCertUsages = TRUE;
    if (includeAppUsages)
        display_app_usages(text, pCertViewInfo->pCertContext, &anyUsageAdded);
    if (includeCertUsages)
        badUsages = display_cert_usages(text, pCertViewInfo->pCertContext,
         &anyUsageAdded);
    if (badUsages)
    {
        PARAFORMAT2 parFmt;

        parFmt.cbSize = sizeof(parFmt);
        parFmt.dwMask = PFM_STARTINDENT;
        parFmt.dxStartIndent = MY_INDENT;
        add_string_resource_with_paraformat_to_control(text,
         IDS_CERT_INFO_BAD_PURPOSES, &parFmt);
    }
}

static void set_cert_info(HWND hwnd,
 PCCRYPTUI_VIEWCERTIFICATE_STRUCTW pCertViewInfo)
{
    CHARFORMATW charFmt;
    PARAFORMAT2 parFmt;
    HWND icon = GetDlgItem(hwnd, IDC_CERTIFICATE_ICON);
    HWND text = GetDlgItem(hwnd, IDC_CERTIFICATE_INFO);
    CRYPT_PROVIDER_SGNR *provSigner = WTHelperGetProvSignerFromChain(
     (CRYPT_PROVIDER_DATA *)pCertViewInfo->u.pCryptProviderData,
     pCertViewInfo->idxSigner, pCertViewInfo->fCounterSigner,
     pCertViewInfo->idxCounterSigner);
    CRYPT_PROVIDER_CERT *root =
     &provSigner->pasCertChain[provSigner->csCertChain - 1];

    if (provSigner->pChainContext->TrustStatus.dwErrorStatus &
     CERT_TRUST_IS_PARTIAL_CHAIN)
        add_icon_to_control(icon, IDB_CERT_WARNING);
    else if (!root->fTrustedRoot)
        add_icon_to_control(icon, IDB_CERT_ERROR);
    else
        add_icon_to_control(icon, IDB_CERT);

    memset(&charFmt, 0, sizeof(charFmt));
    charFmt.cbSize = sizeof(charFmt);
    charFmt.dwMask = CFM_BOLD;
    charFmt.dwEffects = CFE_BOLD;
    SendMessageW(text, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&charFmt);
    /* FIXME: vertically center text */
    parFmt.cbSize = sizeof(parFmt);
    parFmt.dwMask = PFM_STARTINDENT;
    parFmt.dxStartIndent = MY_INDENT;
    add_string_resource_with_paraformat_to_control(text,
     IDS_CERTIFICATEINFORMATION, &parFmt);

    text = GetDlgItem(hwnd, IDC_CERTIFICATE_STATUS);
    SendMessageW(text, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&charFmt);
    if (provSigner->dwError == TRUST_E_CERT_SIGNATURE)
        add_string_resource_with_paraformat_to_control(text,
         IDS_CERT_INFO_BAD_SIG, &parFmt);
    else if (provSigner->pChainContext->TrustStatus.dwErrorStatus &
     CERT_TRUST_IS_PARTIAL_CHAIN)
        add_string_resource_with_paraformat_to_control(text,
         IDS_CERT_INFO_PARTIAL_CHAIN, &parFmt);
    else if (!root->fTrustedRoot)
    {
        if (provSigner->csCertChain == 1 && root->fSelfSigned)
            add_string_resource_with_paraformat_to_control(text,
             IDS_CERT_INFO_UNTRUSTED_CA, &parFmt);
        else
            add_string_resource_with_paraformat_to_control(text,
             IDS_CERT_INFO_UNTRUSTED_ROOT, &parFmt);
    }
    else
    {
        set_policy_text(text, pCertViewInfo);
        FIXME("show issuer statement\n");
    }
}

static void set_cert_name_string(HWND hwnd, PCCERT_CONTEXT cert,
 DWORD nameFlags, int heading)
{
    WCHAR nl = '\n';
    HWND text = GetDlgItem(hwnd, IDC_CERTIFICATE_NAMES);
    CHARFORMATW charFmt;
    PARAFORMAT2 parFmt;

    memset(&charFmt, 0, sizeof(charFmt));
    charFmt.cbSize = sizeof(charFmt);
    charFmt.dwMask = CFM_BOLD;
    charFmt.dwEffects = CFE_BOLD;
    SendMessageW(text, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&charFmt);
    parFmt.cbSize = sizeof(parFmt);
    parFmt.dwMask = PFM_STARTINDENT;
    parFmt.dxStartIndent = MY_INDENT * 3;
    add_string_resource_with_paraformat_to_control(text, heading, &parFmt);
    charFmt.dwEffects = 0;
    SendMessageW(text, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&charFmt);
    add_cert_string_to_control(text, cert, CERT_NAME_SIMPLE_DISPLAY_TYPE,
     nameFlags);
    add_unformatted_text_to_control(text, &nl, 1);
    add_unformatted_text_to_control(text, &nl, 1);
    add_unformatted_text_to_control(text, &nl, 1);

}

static void add_date_string_to_control(HWND hwnd, const FILETIME *fileTime)
{
    WCHAR dateFmt[80]; /* sufficient for all versions of LOCALE_SSHORTDATE */
    WCHAR date[80];
    SYSTEMTIME sysTime;

    GetLocaleInfoW(LOCALE_SYSTEM_DEFAULT, LOCALE_SSHORTDATE, dateFmt,
     sizeof(dateFmt) / sizeof(dateFmt[0]));
    FileTimeToSystemTime(fileTime, &sysTime);
    GetDateFormatW(LOCALE_SYSTEM_DEFAULT, 0, &sysTime, dateFmt, date,
     sizeof(date) / sizeof(date[0]));
    add_unformatted_text_to_control(hwnd, date, lstrlenW(date));
}

static void set_cert_validity_period(HWND hwnd, PCCERT_CONTEXT cert)
{
    WCHAR nl = '\n';
    HWND text = GetDlgItem(hwnd, IDC_CERTIFICATE_NAMES);
    CHARFORMATW charFmt;
    PARAFORMAT2 parFmt;

    memset(&charFmt, 0, sizeof(charFmt));
    charFmt.cbSize = sizeof(charFmt);
    charFmt.dwMask = CFM_BOLD;
    charFmt.dwEffects = CFE_BOLD;
    SendMessageW(text, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&charFmt);
    parFmt.cbSize = sizeof(parFmt);
    parFmt.dwMask = PFM_STARTINDENT;
    parFmt.dxStartIndent = MY_INDENT * 3;
    add_string_resource_with_paraformat_to_control(text, IDS_VALID_FROM,
     &parFmt);
    charFmt.dwEffects = 0;
    SendMessageW(text, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&charFmt);
    add_date_string_to_control(text, &cert->pCertInfo->NotBefore);
    charFmt.dwEffects = CFE_BOLD;
    SendMessageW(text, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&charFmt);
    add_string_resource_to_control(text, IDS_VALID_TO);
    charFmt.dwEffects = 0;
    SendMessageW(text, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&charFmt);
    add_date_string_to_control(text, &cert->pCertInfo->NotAfter);
    add_unformatted_text_to_control(text, &nl, 1);
}

static void set_general_info(HWND hwnd,
 PCCRYPTUI_VIEWCERTIFICATE_STRUCTW pCertViewInfo)
{
    set_cert_info(hwnd, pCertViewInfo);
    set_cert_name_string(hwnd, pCertViewInfo->pCertContext, 0,
     IDS_SUBJECT_HEADING);
    set_cert_name_string(hwnd, pCertViewInfo->pCertContext,
     CERT_NAME_ISSUER_FLAG, IDS_ISSUER_HEADING);
    set_cert_validity_period(hwnd, pCertViewInfo->pCertContext);
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
        set_general_info(hwnd, pCertViewInfo);
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
