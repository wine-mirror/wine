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
#include "commctrl.h"
#include "cryptuiapi.h"
#include "cryptuires.h"
#include "urlmon.h"
#include "hlink.h"
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

static CRYPT_OBJID_BLOB *find_policy_qualifier(CERT_POLICIES_INFO *policies,
 LPCSTR policyOid)
{
    CRYPT_OBJID_BLOB *ret = NULL;
    DWORD i;

    for (i = 0; !ret && i < policies->cPolicyInfo; i++)
    {
        DWORD j;

        for (j = 0; !ret && j < policies->rgPolicyInfo[i].cPolicyQualifier; j++)
            if (!strcmp(policies->rgPolicyInfo[i].rgPolicyQualifier[j].
             pszPolicyQualifierId, policyOid))
                ret = &policies->rgPolicyInfo[i].rgPolicyQualifier[j].
                 Qualifier;
    }
    return ret;
}

static WCHAR *get_cps_str_from_qualifier(CRYPT_OBJID_BLOB *qualifier)
{
    LPWSTR qualifierStr = NULL;
    CERT_NAME_VALUE *qualifierValue;
    DWORD size;

    if (CryptDecodeObjectEx(X509_ASN_ENCODING, X509_NAME_VALUE,
     qualifier->pbData, qualifier->cbData, CRYPT_DECODE_ALLOC_FLAG, NULL,
     &qualifierValue, &size))
    {
        size = CertRDNValueToStrW(qualifierValue->dwValueType,
         &qualifierValue->Value, NULL, 0);
        qualifierStr = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
        if (qualifierStr)
            CertRDNValueToStrW(qualifierValue->dwValueType,
             &qualifierValue->Value, qualifierStr, size);
        LocalFree(qualifierValue);
    }
    return qualifierStr;
}

static WCHAR *get_user_notice_from_qualifier(CRYPT_OBJID_BLOB *qualifier)
{
    LPWSTR str = NULL;
    CERT_POLICY_QUALIFIER_USER_NOTICE *qualifierValue;
    DWORD size;

    if (CryptDecodeObjectEx(X509_ASN_ENCODING,
     X509_PKIX_POLICY_QUALIFIER_USERNOTICE,
     qualifier->pbData, qualifier->cbData, CRYPT_DECODE_ALLOC_FLAG, NULL,
     &qualifierValue, &size))
    {
        str = HeapAlloc(GetProcessHeap(), 0,
         (strlenW(qualifierValue->pszDisplayText) + 1) * sizeof(WCHAR));
        if (str)
            strcpyW(str, qualifierValue->pszDisplayText);
        LocalFree(qualifierValue);
    }
    return str;
}

struct IssuerStatement
{
    LPWSTR cps;
    LPWSTR userNotice;
};

static void set_issuer_statement(HWND hwnd,
 PCCRYPTUI_VIEWCERTIFICATE_STRUCTW pCertViewInfo)
{
    PCERT_EXTENSION policyExt;

    if (!(pCertViewInfo->dwFlags & CRYPTUI_DISABLE_ISSUERSTATEMENT) &&
     (policyExt = CertFindExtension(szOID_CERT_POLICIES,
     pCertViewInfo->pCertContext->pCertInfo->cExtension,
     pCertViewInfo->pCertContext->pCertInfo->rgExtension)))
    {
        CERT_POLICIES_INFO *policies;
        DWORD size;

        if (CryptDecodeObjectEx(X509_ASN_ENCODING, policyExt->pszObjId,
         policyExt->Value.pbData, policyExt->Value.cbData,
         CRYPT_DECODE_ALLOC_FLAG, NULL, &policies, &size))
        {
            CRYPT_OBJID_BLOB *qualifier;
            LPWSTR cps = NULL, userNotice = NULL;

            if ((qualifier = find_policy_qualifier(policies,
             szOID_PKIX_POLICY_QUALIFIER_CPS)))
                cps = get_cps_str_from_qualifier(qualifier);
            if ((qualifier = find_policy_qualifier(policies,
             szOID_PKIX_POLICY_QUALIFIER_USERNOTICE)))
                userNotice = get_user_notice_from_qualifier(qualifier);
            if (cps || userNotice)
            {
                struct IssuerStatement *issuerStatement =
                 HeapAlloc(GetProcessHeap(), 0, sizeof(struct IssuerStatement));

                if (issuerStatement)
                {
                    issuerStatement->cps = cps;
                    issuerStatement->userNotice = userNotice;
                    EnableWindow(GetDlgItem(hwnd, IDC_ISSUERSTATEMENT), TRUE);
                    SetWindowLongPtrW(hwnd, DWLP_USER,
                     (ULONG_PTR)issuerStatement);
                }
            }
            LocalFree(policies);
        }
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
        set_issuer_statement(hwnd, pCertViewInfo);
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

static LRESULT CALLBACK user_notice_dlg_proc(HWND hwnd, UINT msg, WPARAM wp,
 LPARAM lp)
{
    LRESULT ret = 0;
    HWND text;
    struct IssuerStatement *issuerStatement;

    switch (msg)
    {
    case WM_INITDIALOG:
        text = GetDlgItem(hwnd, IDC_USERNOTICE);
        issuerStatement = (struct IssuerStatement *)lp;
        add_unformatted_text_to_control(text, issuerStatement->userNotice,
         strlenW(issuerStatement->userNotice));
        if (issuerStatement->cps)
            SetWindowLongPtrW(hwnd, DWLP_USER, (LPARAM)issuerStatement->cps);
        else
            EnableWindow(GetDlgItem(hwnd, IDC_CPS), FALSE);
        break;
    case WM_COMMAND:
        switch (wp)
        {
        case IDOK:
            EndDialog(hwnd, IDOK);
            ret = TRUE;
            break;
        case IDC_CPS:
        {
            IBindCtx *bctx = NULL;
            LPWSTR cps;

            CreateBindCtx(0, &bctx);
            cps = (LPWSTR)GetWindowLongPtrW(hwnd, DWLP_USER);
            HlinkSimpleNavigateToString(cps, NULL, NULL, NULL, bctx, NULL,
             HLNF_OPENINNEWWINDOW, 0);
            IBindCtx_Release(bctx);
            break;
        }
        }
    }
    return ret;
}

static void show_user_notice(HWND hwnd, struct IssuerStatement *issuerStatement)
{
    DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_USERNOTICE), hwnd,
     user_notice_dlg_proc, (LPARAM)issuerStatement);
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
        case IDC_ISSUERSTATEMENT:
        {
            struct IssuerStatement *issuerStatement =
             (struct IssuerStatement *)GetWindowLongPtrW(hwnd, DWLP_USER);

            if (issuerStatement)
            {
                if (issuerStatement->userNotice)
                    show_user_notice(hwnd, issuerStatement);
                else if (issuerStatement->cps)
                {
                    IBindCtx *bctx = NULL;

                    CreateBindCtx(0, &bctx);
                    HlinkSimpleNavigateToString(issuerStatement->cps, NULL,
                     NULL, NULL, bctx, NULL, HLNF_OPENINNEWWINDOW, 0);
                    IBindCtx_Release(bctx);
                }
            }
            break;
        }
        }
        break;
    }
    return 0;
}

static UINT CALLBACK general_callback_proc(HWND hwnd, UINT msg,
 PROPSHEETPAGEW *page)
{
    struct IssuerStatement *issuerStatement;

    switch (msg)
    {
    case PSPCB_RELEASE:
        issuerStatement =
         (struct IssuerStatement *)GetWindowLongPtrW(hwnd, DWLP_USER);
        if (issuerStatement)
        {
            HeapFree(GetProcessHeap(), 0, issuerStatement->cps);
            HeapFree(GetProcessHeap(), 0, issuerStatement->userNotice);
            HeapFree(GetProcessHeap(), 0, issuerStatement);
        }
        break;
    }
    return 1;
}

static void init_general_page(PCCRYPTUI_VIEWCERTIFICATE_STRUCTW pCertViewInfo,
 PROPSHEETPAGEW *page)
{
    memset(page, 0, sizeof(PROPSHEETPAGEW));
    page->dwSize = sizeof(PROPSHEETPAGEW);
    page->dwFlags = PSP_USECALLBACK;
    page->pfnCallback = general_callback_proc;
    page->hInstance = hInstance;
    page->u.pszTemplate = MAKEINTRESOURCEW(IDD_GENERAL);
    page->pfnDlgProc = general_dlg_proc;
    page->lParam = (LPARAM)pCertViewInfo;
}

typedef WCHAR * (*field_format_func)(PCCERT_CONTEXT cert);

static WCHAR *field_format_version(PCCERT_CONTEXT cert)
{
    static const WCHAR fmt[] = { 'V','%','d',0 };
    WCHAR *buf = HeapAlloc(GetProcessHeap(), 0, 12 * sizeof(WCHAR));

    if (buf)
        sprintfW(buf, fmt, cert->pCertInfo->dwVersion);
    return buf;
}

static WCHAR *format_hex_string(void *pb, DWORD cb)
{
    WCHAR *buf = HeapAlloc(GetProcessHeap(), 0, (cb * 3 + 1) * sizeof(WCHAR));

    if (buf)
    {
        static const WCHAR fmt[] = { '%','0','2','x',' ',0 };
        DWORD i;
        WCHAR *ptr;

        for (i = 0, ptr = buf; i < cb; i++, ptr += 3)
            sprintfW(ptr, fmt, ((BYTE *)pb)[i]);
    }
    return buf;
}

static WCHAR *field_format_serial_number(PCCERT_CONTEXT cert)
{
    return format_hex_string(cert->pCertInfo->SerialNumber.pbData,
     cert->pCertInfo->SerialNumber.cbData);
}

static WCHAR *field_format_issuer(PCCERT_CONTEXT cert)
{
    return get_cert_name_string(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE,
     CERT_NAME_ISSUER_FLAG);
}

static WCHAR *field_format_detailed_cert_name(PCERT_NAME_BLOB name)
{
    WCHAR *str = NULL;
    DWORD len = CertNameToStrW(X509_ASN_ENCODING, name,
     CERT_X500_NAME_STR | CERT_NAME_STR_CRLF_FLAG, NULL, 0);

    if (len)
    {
        str = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (str)
            CertNameToStrW(X509_ASN_ENCODING, name,
             CERT_X500_NAME_STR | CERT_NAME_STR_CRLF_FLAG, str, len);
    }
    return str;
}

static WCHAR *field_format_detailed_issuer(PCCERT_CONTEXT cert, void *param)
{
    return field_format_detailed_cert_name(&cert->pCertInfo->Issuer);
}

static WCHAR *field_format_subject(PCCERT_CONTEXT cert)
{
    return get_cert_name_string(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0);
}

static WCHAR *field_format_detailed_subject(PCCERT_CONTEXT cert, void *param)
{
    return field_format_detailed_cert_name(&cert->pCertInfo->Subject);
}

static WCHAR *format_long_date(const FILETIME *fileTime)
{
    WCHAR dateFmt[80]; /* long enough for LOCALE_SLONGDATE */
    DWORD len;
    WCHAR *buf = NULL;
    SYSTEMTIME sysTime;

    /* FIXME: format isn't quite right, want time too */
    GetLocaleInfoW(LOCALE_SYSTEM_DEFAULT, LOCALE_SLONGDATE, dateFmt,
     sizeof(dateFmt) / sizeof(dateFmt[0]));
    FileTimeToSystemTime(fileTime, &sysTime);
    len = GetDateFormatW(LOCALE_SYSTEM_DEFAULT, 0, &sysTime, dateFmt, NULL, 0);
    if (len)
    {
        buf = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (buf)
            GetDateFormatW(LOCALE_SYSTEM_DEFAULT, 0, &sysTime, dateFmt, buf,
             len);
    }
    return buf;
}

static WCHAR *field_format_from_date(PCCERT_CONTEXT cert)
{
    return format_long_date(&cert->pCertInfo->NotBefore);
}

static WCHAR *field_format_to_date(PCCERT_CONTEXT cert)
{
    return format_long_date(&cert->pCertInfo->NotAfter);
}

static WCHAR *field_format_public_key(PCCERT_CONTEXT cert)
{
    PCCRYPT_OID_INFO oidInfo;
    WCHAR *buf = NULL;

    oidInfo = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY,
     cert->pCertInfo->SubjectPublicKeyInfo.Algorithm.pszObjId, 0);
    if (oidInfo)
    {
        WCHAR fmt[MAX_STRING_LEN];

        if (LoadStringW(hInstance, IDS_FIELD_PUBLIC_KEY_FORMAT, fmt,
         sizeof(fmt) / sizeof(fmt[0])))
        {
            /* Allocate the output buffer.  Use the number of bytes in the
             * public key as a conservative (high) estimate for the number of
             * digits in its output.
             * The output is of the form (in English)
             * "<public key algorithm> (<public key bit length> bits)".
             * Ordinarily having two positional parameters in a string is not a
             * good idea, but as this isn't a sentence fragment, it shouldn't
             * be word-order dependent.
             */
            buf = HeapAlloc(GetProcessHeap(), 0,
             (strlenW(fmt) + strlenW(oidInfo->pwszName) +
             cert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData * 8)
             * sizeof(WCHAR));
            if (buf)
                sprintfW(buf, fmt, oidInfo->pwszName,
                 CertGetPublicKeyLength(X509_ASN_ENCODING,
                  &cert->pCertInfo->SubjectPublicKeyInfo));
        }
    }
    return buf;
}

static WCHAR *field_format_detailed_public_key(PCCERT_CONTEXT cert, void *param)
{
    return format_hex_string(
     cert->pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
     cert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData);
}

struct field_value_data;
struct detail_data
{
    PCCRYPTUI_VIEWCERTIFICATE_STRUCTW pCertViewInfo;
    BOOL *pfPropertiesChanged;
    int cFields;
    struct field_value_data *fields;
};

typedef void (*add_fields_func)(HWND hwnd, struct detail_data *data);

typedef WCHAR *(*create_detailed_value_func)(PCCERT_CONTEXT cert, void *param);

struct field_value_data
{
    create_detailed_value_func create;
    LPWSTR detailed_value;
    void *param;
};

static void add_field_value_data(struct detail_data *data,
 create_detailed_value_func create, void *param)
{
    if (data->cFields)
        data->fields = HeapReAlloc(GetProcessHeap(), 0, data->fields,
         (data->cFields + 1) * sizeof(struct field_value_data));
    else
        data->fields = HeapAlloc(GetProcessHeap(), 0,
         sizeof(struct field_value_data));
    if (data->fields)
    {
        data->fields[data->cFields].create = create;
        data->fields[data->cFields].detailed_value = NULL;
        data->fields[data->cFields].param = param;
        data->cFields++;
    }
}

static void add_field_and_value_to_list(HWND hwnd, struct detail_data *data,
 LPWSTR field, LPWSTR value, create_detailed_value_func create, void *param)
{
    LVITEMW item;
    int iItem = SendMessageW(hwnd, LVM_GETITEMCOUNT, 0, 0);

    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.iItem = iItem;
    item.iSubItem = 0;
    item.pszText = field;
    item.lParam = (LPARAM)data;
    SendMessageW(hwnd, LVM_INSERTITEMW, 0, (LPARAM)&item);
    if (value)
    {
        item.pszText = value;
        item.iSubItem = 1;
        SendMessageW(hwnd, LVM_SETITEMTEXTW, iItem, (LPARAM)&item);
    }
    add_field_value_data(data, create, param);
}

static void add_string_id_and_value_to_list(HWND hwnd, struct detail_data *data,
 int id, LPWSTR value, create_detailed_value_func create, void *param)
{
    WCHAR buf[MAX_STRING_LEN];

    LoadStringW(hInstance, id, buf, sizeof(buf) / sizeof(buf[0]));
    add_field_and_value_to_list(hwnd, data, buf, value, create, param);
}

struct v1_field
{
    int id;
    field_format_func format;
    create_detailed_value_func create_detailed_value;
};

static void add_v1_field(HWND hwnd, struct detail_data *data,
 const struct v1_field *field)
{
    WCHAR *val = field->format(data->pCertViewInfo->pCertContext);

    if (val)
    {
        add_string_id_and_value_to_list(hwnd, data, field->id, val,
         field->create_detailed_value, NULL);
        HeapFree(GetProcessHeap(), 0, val);
    }
}

static const struct v1_field v1_fields[] = {
 { IDS_FIELD_VERSION, field_format_version, NULL },
 { IDS_FIELD_SERIAL_NUMBER, field_format_serial_number, NULL },
 { IDS_FIELD_ISSUER, field_format_issuer, field_format_detailed_issuer },
 { IDS_FIELD_VALID_FROM, field_format_from_date, NULL },
 { IDS_FIELD_VALID_TO, field_format_to_date, NULL },
 { IDS_FIELD_SUBJECT, field_format_subject, field_format_detailed_subject },
 { IDS_FIELD_PUBLIC_KEY, field_format_public_key,
   field_format_detailed_public_key }
};

static void add_v1_fields(HWND hwnd, struct detail_data *data)
{
    int i;
    PCCERT_CONTEXT cert = data->pCertViewInfo->pCertContext;

    /* The last item in v1_fields is the public key, which is not in the loop
     * because it's a special case.
     */
    for (i = 0; i < sizeof(v1_fields) / sizeof(v1_fields[0]) - 1; i++)
        add_v1_field(hwnd, data, &v1_fields[i]);
    if (cert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData)
        add_v1_field(hwnd, data, &v1_fields[i]);
}

static WCHAR *crypt_format_extension(PCERT_EXTENSION ext, DWORD formatStrType)
{
    WCHAR *str = NULL;
    DWORD size;

    if (CryptFormatObject(X509_ASN_ENCODING, 0, formatStrType, NULL,
     ext->pszObjId, ext->Value.pbData, ext->Value.cbData, NULL, &size))
    {
        str = HeapAlloc(GetProcessHeap(), 0, size);
        CryptFormatObject(X509_ASN_ENCODING, 0, formatStrType, NULL,
         ext->pszObjId, ext->Value.pbData, ext->Value.cbData, str, &size);
    }
    return str;
}

static WCHAR *field_format_extension_hex_with_ascii(PCERT_EXTENSION ext)
{
    WCHAR *str = NULL;

    if (ext->Value.cbData)
    {
        /* The output is formatted as:
         * <hex bytes>  <ascii bytes>\n
         * where <hex bytes> is a string of up to 8 bytes, output as %02x,
         * and <ascii bytes> is the ASCII equivalent of each byte, or '.' if
         * the byte is not printable.
         * So, for example, the extension value consisting of the following
         * bytes:
         *   0x30,0x14,0x31,0x12,0x30,0x10,0x06,0x03,0x55,0x04,0x03,
         *   0x13,0x09,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67
         * is output as:
         *   30 14 31 12 30 10 06 03  0.1.0...
         *   55 04 03 13 09 4a 75 61  U....Jua
         *   6e 20 4c 61 6e 67        n Lang
         * The allocation size therefore requires:
         * - 4 characters per character in an 8-byte line
         *   (2 for the hex format, one for the space, one for the ASCII value)
         * - 3 more characters per 8-byte line (two spaces and a newline)
         * - 1 character for the terminating nul
         * FIXME: should use a fixed-width font for this
         */
        DWORD lines = (ext->Value.cbData + 7) / 8;

        str = HeapAlloc(GetProcessHeap(), 0,
         (lines * 8 * 4 + lines * 3 + 1) * sizeof(WCHAR));
        if (str)
        {
            static const WCHAR fmt[] = { '%','0','2','x',' ',0 };
            DWORD i, j;
            WCHAR *ptr;

            for (i = 0, ptr = str; i < ext->Value.cbData; i += 8)
            {
                /* Output as hex bytes first */
                for (j = i; j < min(i + 8, ext->Value.cbData); j++, ptr += 3)
                    sprintfW(ptr, fmt, ext->Value.pbData[j]);
                /* Pad the hex output with spaces for alignment */
                if (j == ext->Value.cbData && j % 8)
                {
                    static const WCHAR pad[] = { ' ',' ',' ' };

                    for (; j % 8; j++, ptr += sizeof(pad) / sizeof(pad[0]))
                        memcpy(ptr, pad, sizeof(pad));
                }
                /* The last sprintfW included a space, so just insert one
                 * more space between the hex bytes and the ASCII output
                 */
                *ptr++ = ' ';
                /* Output as ASCII bytes */
                for (j = i; j < min(i + 8, ext->Value.cbData); j++, ptr++)
                {
                    if (isprintW(ext->Value.pbData[j]) &&
                     !isspaceW(ext->Value.pbData[j]))
                        *ptr = ext->Value.pbData[j];
                    else
                        *ptr = '.';
                }
                *ptr++ = '\n';
            }
            *ptr++ = '\0';
        }
    }
    return str;
}

static WCHAR *field_format_detailed_extension(PCCERT_CONTEXT cert, void *param)
{
    PCERT_EXTENSION ext = param;
    LPWSTR str = crypt_format_extension(ext,
     CRYPT_FORMAT_STR_MULTI_LINE | CRYPT_FORMAT_STR_NO_HEX);

    if (!str)
        str = field_format_extension_hex_with_ascii(ext);
    return str;
}

static void add_cert_extension_detail(HWND hwnd, struct detail_data *data,
 PCERT_EXTENSION ext)
{
    PCCRYPT_OID_INFO oidInfo = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY,
     ext->pszObjId, 0);
    LPWSTR val = crypt_format_extension(ext, 0);

    if (oidInfo)
        add_field_and_value_to_list(hwnd, data, (LPWSTR)oidInfo->pwszName,
         val, field_format_detailed_extension, ext);
    else
    {
        DWORD len = strlen(ext->pszObjId);
        LPWSTR oidW = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));

        if (oidW)
        {
            DWORD i;

            for (i = 0; i <= len; i++)
                oidW[i] = ext->pszObjId[i];
            add_field_and_value_to_list(hwnd, data, oidW, val,
             field_format_detailed_extension, ext);
            HeapFree(GetProcessHeap(), 0, oidW);
        }
    }
    HeapFree(GetProcessHeap(), 0, val);
}

static void add_all_extensions(HWND hwnd, struct detail_data *data)
{
    DWORD i;
    PCCERT_CONTEXT cert = data->pCertViewInfo->pCertContext;

    for (i = 0; i < cert->pCertInfo->cExtension; i++)
        add_cert_extension_detail(hwnd, data, &cert->pCertInfo->rgExtension[i]);
}

static void add_critical_extensions(HWND hwnd, struct detail_data *data)
{
    DWORD i;
    PCCERT_CONTEXT cert = data->pCertViewInfo->pCertContext;

    for (i = 0; i < cert->pCertInfo->cExtension; i++)
        if (cert->pCertInfo->rgExtension[i].fCritical)
            add_cert_extension_detail(hwnd, data,
             &cert->pCertInfo->rgExtension[i]);
}

static void add_all_fields(HWND hwnd, struct detail_data *data)
{
    add_v1_fields(hwnd, data);
    add_all_extensions(hwnd, data);
    FIXME("add properties\n");
}

struct selection_list_item
{
    int id;
    add_fields_func add;
};

const struct selection_list_item listItems[] = {
 { IDS_FIELDS_ALL, add_all_fields },
 { IDS_FIELDS_V1, add_v1_fields },
 { IDS_FIELDS_EXTENSIONS, add_all_extensions },
 { IDS_FIELDS_CRITICAL_EXTENSIONS, add_critical_extensions },
 { IDS_FIELDS_PROPERTIES, NULL },
};

static void create_show_list(HWND hwnd, struct detail_data *data)
{
    HWND cb = GetDlgItem(hwnd, IDC_DETAIL_SELECT);
    WCHAR buf[MAX_STRING_LEN];
    int i;

    for (i = 0; i < sizeof(listItems) / sizeof(listItems[0]); i++)
    {
        int index;

        LoadStringW(hInstance, listItems[i].id, buf,
         sizeof(buf) / sizeof(buf[0]));
        index = SendMessageW(cb, CB_INSERTSTRING, -1, (LPARAM)buf);
        SendMessageW(cb, CB_SETITEMDATA, index, (LPARAM)data);
    }
    SendMessageW(cb, CB_SETCURSEL, 0, 0);
}

static void create_listview_columns(HWND hwnd)
{
    HWND lv = GetDlgItem(hwnd, IDC_DETAIL_LIST);
    RECT rc;
    WCHAR buf[MAX_STRING_LEN];
    LVCOLUMNW column;

    SendMessageW(lv, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);
    GetWindowRect(lv, &rc);
    LoadStringW(hInstance, IDS_FIELD, buf, sizeof(buf) / sizeof(buf[0]));
    column.mask = LVCF_WIDTH | LVCF_TEXT;
    column.cx = (rc.right - rc.left) / 2 - 2;
    column.pszText = buf;
    SendMessageW(lv, LVM_INSERTCOLUMNW, 0, (LPARAM)&column);
    LoadStringW(hInstance, IDS_VALUE, buf, sizeof(buf) / sizeof(buf[0]));
    SendMessageW(lv, LVM_INSERTCOLUMNW, 1, (LPARAM)&column);
}

static void set_fields_selection(HWND hwnd, struct detail_data *data, int sel)
{
    HWND list = GetDlgItem(hwnd, IDC_DETAIL_LIST);

    if (sel >= 0 && sel < sizeof(listItems) / sizeof(listItems[0]))
    {
        SendMessageW(list, LVM_DELETEALLITEMS, 0, 0);
        if (listItems[sel].add)
            listItems[sel].add(list, data);
    }
}

static void create_cert_details_list(HWND hwnd, struct detail_data *data)
{
    create_show_list(hwnd, data);
    create_listview_columns(hwnd);
    set_fields_selection(hwnd, data, 0);
}

static void free_detail_fields(struct detail_data *data)
{
    DWORD i;

    for (i = 0; i < data->cFields; i++)
        HeapFree(GetProcessHeap(), 0, data->fields[i].detailed_value);
    HeapFree(GetProcessHeap(), 0, data->fields);
    data->fields = NULL;
    data->cFields = 0;
}

static void refresh_details_view(HWND hwnd)
{
    HWND cb = GetDlgItem(hwnd, IDC_DETAIL_SELECT);
    int curSel;
    struct detail_data *data;

    curSel = SendMessageW(cb, CB_GETCURSEL, 0, 0);
    /* Actually, any index will do, since they all store the same data value */
    data = (struct detail_data *)SendMessageW(cb, CB_GETITEMDATA, curSel, 0);
    free_detail_fields(data);
    set_fields_selection(hwnd, data, curSel);
}

static LRESULT CALLBACK detail_dlg_proc(HWND hwnd, UINT msg, WPARAM wp,
 LPARAM lp)
{
    PROPSHEETPAGEW *page;
    struct detail_data *data;

    TRACE("(%p, %08x, %08lx, %08lx)\n", hwnd, msg, wp, lp);

    switch (msg)
    {
    case WM_INITDIALOG:
        page = (PROPSHEETPAGEW *)lp;
        data = (struct detail_data *)page->lParam;
        create_cert_details_list(hwnd, data);
        if (!(data->pCertViewInfo->dwFlags & CRYPTUI_ENABLE_EDITPROPERTIES))
            EnableWindow(GetDlgItem(hwnd, IDC_EDITPROPERTIES), FALSE);
        if (data->pCertViewInfo->dwFlags & CRYPTUI_DISABLE_EXPORT)
            EnableWindow(GetDlgItem(hwnd, IDC_EXPORT), FALSE);
        break;
    case WM_NOTIFY:
    {
        NMITEMACTIVATE *nm;
        HWND list = GetDlgItem(hwnd, IDC_DETAIL_LIST);

        nm = (NMITEMACTIVATE*)lp;
        if (nm->hdr.hwndFrom == list && nm->uNewState & LVN_ITEMACTIVATE
         && nm->hdr.code == LVN_ITEMCHANGED)
        {
            data = (struct detail_data *)nm->lParam;
            if (nm->iItem >= 0 && data && nm->iItem < data->cFields)
            {
                WCHAR buf[MAX_STRING_LEN], *val = NULL;
                HWND valueCtl = GetDlgItem(hwnd, IDC_DETAIL_VALUE);

                if (data->fields[nm->iItem].create)
                    val = data->fields[nm->iItem].create(
                     data->pCertViewInfo->pCertContext,
                     data->fields[nm->iItem].param);
                else
                {
                    LVITEMW item;
                    int res;

                    item.cchTextMax = sizeof(buf) / sizeof(buf[0]);
                    item.mask = LVIF_TEXT;
                    item.pszText = buf;
                    item.iItem = nm->iItem;
                    item.iSubItem = 1;
                    res = SendMessageW(list, LVM_GETITEMW, 0, (LPARAM)&item);
                    if (res)
                        val = buf;
                }
                /* Select all the text in the control, the next update will
                 * replace it
                 */
                SendMessageW(valueCtl, EM_SETSEL, 0, -1);
                add_unformatted_text_to_control(valueCtl, val,
                 val ? strlenW(val) : 0);
                if (val != buf)
                    HeapFree(GetProcessHeap(), 0, val);
            }
        }
        break;
    }
    case WM_COMMAND:
        switch (wp)
        {
        case IDC_EXPORT:
            FIXME("call CryptUIWizExport\n");
            break;
        case IDC_EDITPROPERTIES:
            FIXME("show edit properties dialog\n");
            break;
        case ((CBN_SELCHANGE << 16) | IDC_DETAIL_SELECT):
            refresh_details_view(hwnd);
            break;
        }
        break;
    }
    return 0;
}

static BOOL init_detail_page(PCCRYPTUI_VIEWCERTIFICATE_STRUCTW pCertViewInfo,
 BOOL *pfPropertiesChanged, PROPSHEETPAGEW *page)
{
    BOOL ret;
    struct detail_data *data = HeapAlloc(GetProcessHeap(), 0,
     sizeof(struct detail_data));

    if (data)
    {
        data->pCertViewInfo = pCertViewInfo;
        data->pfPropertiesChanged = pfPropertiesChanged;
        data->cFields = 0;
        data->fields = NULL;
        memset(page, 0, sizeof(PROPSHEETPAGEW));
        page->dwSize = sizeof(PROPSHEETPAGEW);
        page->hInstance = hInstance;
        page->u.pszTemplate = MAKEINTRESOURCEW(IDD_DETAIL);
        page->pfnDlgProc = detail_dlg_proc;
        page->lParam = (LPARAM)data;
        ret = TRUE;
    }
    else
        ret = FALSE;
    return ret;
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
        nPages++;
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
        if (!(pCertViewInfo->dwFlags & CRYPTUI_HIDE_DETAILPAGE))
        {
            if (init_detail_page(pCertViewInfo, pfPropertiesChanged,
             &pages[hdr.nPages]))
                hdr.nPages++;
        }
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
