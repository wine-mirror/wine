/*
 * Copyright (C) 2006 Maarten Lankhorst
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

#include "config.h"
#include "wine/port.h"
#include <stdio.h>
#include "windef.h"
#include "wine/debug.h"
#include "winbase.h"
#include "winnt.h"
#include "winnls.h"
#define NONAMELESSUNION
#include "wincrypt.h"

WINE_DEFAULT_DEBUG_CHANNEL(cryptnet);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
   TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

   switch (fdwReason) {
      case DLL_WINE_PREATTACH:
         return FALSE;  /* prefer native version */
      case DLL_PROCESS_ATTACH:
         DisableThreadLibraryCalls(hinstDLL);
         break;
      case DLL_PROCESS_DETACH:
         /* Do uninitialisation here */
         break;
      default: break;
   }
   return TRUE;
}

static const WCHAR cryptNet[] = { 'c','r','y','p','t','n','e','t','.',
   'd','l','l',0 };
static const WCHAR ldapProvOpenStore[] = { 'L','d','a','p','P','r','o','v',
   'O','p','e','S','t','o','r','e',0 };

/***********************************************************************
 *    DllRegisterServer (CRYPTNET.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
   TRACE("\n");
   CryptRegisterDefaultOIDFunction(X509_ASN_ENCODING,
    CRYPT_OID_VERIFY_REVOCATION_FUNC, 0, cryptNet);
   CryptRegisterOIDFunction(0, CRYPT_OID_OPEN_STORE_PROV_FUNC, "Ldap",
    cryptNet, "LdapProvOpenStore");
   CryptRegisterOIDFunction(0, CRYPT_OID_OPEN_STORE_PROV_FUNC,
    CERT_STORE_PROV_LDAP_W, cryptNet, "LdapProvOpenStore");
   return S_OK;
}

/***********************************************************************
 *    DllUnregisterServer (CRYPTNET.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
   TRACE("\n");
   CryptUnregisterDefaultOIDFunction(X509_ASN_ENCODING,
    CRYPT_OID_VERIFY_REVOCATION_FUNC, cryptNet);
   CryptUnregisterOIDFunction(0, CRYPT_OID_OPEN_STORE_PROV_FUNC, "Ldap");
   CryptUnregisterOIDFunction(0, CRYPT_OID_OPEN_STORE_PROV_FUNC,
    CERT_STORE_PROV_LDAP_W);
   return S_OK;
}

static const char *url_oid_to_str(LPCSTR oid)
{
    if (HIWORD(oid))
        return oid;
    else
    {
        static char buf[10];

        switch (LOWORD(oid))
        {
#define _x(oid) case LOWORD(oid): return #oid
        _x(URL_OID_CERTIFICATE_ISSUER);
        _x(URL_OID_CERTIFICATE_CRL_DIST_POINT);
        _x(URL_OID_CTL_ISSUER);
        _x(URL_OID_CTL_NEXT_UPDATE);
        _x(URL_OID_CRL_ISSUER);
        _x(URL_OID_CERTIFICATE_FRESHEST_CRL);
        _x(URL_OID_CRL_FRESHEST_CRL);
        _x(URL_OID_CROSS_CERT_DIST_POINT);
#undef _x
        default:
            snprintf(buf, sizeof(buf), "%d", LOWORD(oid));
            return buf;
        }
    }
}

typedef BOOL (WINAPI *UrlDllGetObjectUrlFunc)(LPCSTR, LPVOID, DWORD,
 PCRYPT_URL_ARRAY, DWORD *, PCRYPT_URL_INFO, DWORD *, LPVOID);

static BOOL WINAPI CRYPT_GetUrlFromCertificateIssuer(LPCSTR pszUrlOid,
 LPVOID pvPara, DWORD dwFlags, PCRYPT_URL_ARRAY pUrlArray, DWORD *pcbUrlArray,
 PCRYPT_URL_INFO pUrlInfo, DWORD *pcbUrlInfo, LPVOID pvReserved)
{
    /* FIXME: This depends on the AIA (authority info access) extension being
     * supported in crypt32.
     */
    FIXME("\n");
    SetLastError(CRYPT_E_NOT_FOUND);
    return FALSE;
}

static BOOL WINAPI CRYPT_GetUrlFromCertificateCRLDistPoint(LPCSTR pszUrlOid,
 LPVOID pvPara, DWORD dwFlags, PCRYPT_URL_ARRAY pUrlArray, DWORD *pcbUrlArray,
 PCRYPT_URL_INFO pUrlInfo, DWORD *pcbUrlInfo, LPVOID pvReserved)
{
    PCCERT_CONTEXT cert = (PCCERT_CONTEXT)pvPara;
    PCERT_EXTENSION ext;
    BOOL ret = FALSE;

    /* The only applicable flag is CRYPT_GET_URL_FROM_EXTENSION */
    if (dwFlags && !(dwFlags & CRYPT_GET_URL_FROM_EXTENSION))
    {
        SetLastError(CRYPT_E_NOT_FOUND);
        return FALSE;
    }
    if ((ext = CertFindExtension(szOID_CRL_DIST_POINTS,
     cert->pCertInfo->cExtension, cert->pCertInfo->rgExtension)))
    {
        CRL_DIST_POINTS_INFO *info;
        DWORD size;

        ret = CryptDecodeObjectEx(X509_ASN_ENCODING, X509_CRL_DIST_POINTS,
         ext->Value.pbData, ext->Value.cbData, CRYPT_DECODE_ALLOC_FLAG, NULL,
         &info, &size);
        if (ret)
        {
            DWORD i, cUrl, bytesNeeded = sizeof(CRYPT_URL_ARRAY);

            for (i = 0, cUrl = 0; i < info->cDistPoint; i++)
                if (info->rgDistPoint[i].DistPointName.dwDistPointNameChoice
                 == CRL_DIST_POINT_FULL_NAME)
                {
                    DWORD j;
                    CERT_ALT_NAME_INFO *name =
                     &info->rgDistPoint[i].DistPointName.FullName;

                    for (j = 0; j < name->cAltEntry; j++)
                        if (name->rgAltEntry[j].dwAltNameChoice ==
                         CERT_ALT_NAME_URL)
                        {
                            if (name->rgAltEntry[j].pwszURL)
                            {
                                cUrl++;
                                bytesNeeded += sizeof(LPWSTR) +
                                 (lstrlenW(name->rgAltEntry[j].pwszURL) + 1)
                                 * sizeof(WCHAR);
                            }
                        }
                }
            if (!pcbUrlArray)
                SetLastError(E_INVALIDARG);
            else if (!pUrlArray)
                *pcbUrlArray = bytesNeeded;
            else if (*pcbUrlArray < bytesNeeded)
            {
                SetLastError(ERROR_MORE_DATA);
                *pcbUrlArray = bytesNeeded;
                ret = FALSE;
            }
            else
            {
                LPWSTR nextUrl;

                *pcbUrlArray = bytesNeeded;
                pUrlArray->cUrl = 0;
                pUrlArray->rgwszUrl =
                 (LPWSTR *)((BYTE *)pUrlArray + sizeof(CRYPT_URL_ARRAY));
                nextUrl = (LPWSTR)((BYTE *)pUrlArray + sizeof(CRYPT_URL_ARRAY)
                 + cUrl * sizeof(LPWSTR));
                for (i = 0; i < info->cDistPoint; i++)
                    if (info->rgDistPoint[i].DistPointName.dwDistPointNameChoice
                     == CRL_DIST_POINT_FULL_NAME)
                    {
                        DWORD j;
                        CERT_ALT_NAME_INFO *name =
                         &info->rgDistPoint[i].DistPointName.FullName;

                        for (j = 0; j < name->cAltEntry; j++)
                            if (name->rgAltEntry[j].dwAltNameChoice ==
                             CERT_ALT_NAME_URL)
                            {
                                if (name->rgAltEntry[j].pwszURL)
                                {
                                    lstrcpyW(nextUrl,
                                     name->rgAltEntry[j].pwszURL);
                                    pUrlArray->rgwszUrl[pUrlArray->cUrl++] =
                                     nextUrl;
                                    nextUrl +=
                                     (lstrlenW(name->rgAltEntry[j].pwszURL) + 1)
                                     * sizeof(WCHAR);
                                }
                            }
                    }
            }
            if (ret)
            {
                if (pcbUrlInfo)
                {
                    FIXME("url info: stub\n");
                    if (!pUrlInfo)
                        *pcbUrlInfo = sizeof(CRYPT_URL_INFO);
                    else if (*pcbUrlInfo < sizeof(CRYPT_URL_INFO))
                    {
                        *pcbUrlInfo = sizeof(CRYPT_URL_INFO);
                        SetLastError(ERROR_MORE_DATA);
                        ret = FALSE;
                    }
                    else
                    {
                        *pcbUrlInfo = sizeof(CRYPT_URL_INFO);
                        memset(pUrlInfo, 0, sizeof(CRYPT_URL_INFO));
                    }
                }
            }
            LocalFree(info);
        }
    }
    else
        SetLastError(CRYPT_E_NOT_FOUND);
    return ret;
}

/***********************************************************************
 *    CryptGetObjectUrl (CRYPTNET.@)
 */
BOOL WINAPI CryptGetObjectUrl(LPCSTR pszUrlOid, LPVOID pvPara, DWORD dwFlags,
 PCRYPT_URL_ARRAY pUrlArray, DWORD *pcbUrlArray, PCRYPT_URL_INFO pUrlInfo,
 DWORD *pcbUrlInfo, LPVOID pvReserved)
{
    UrlDllGetObjectUrlFunc func = NULL;
    HCRYPTOIDFUNCADDR hFunc = NULL;
    BOOL ret = FALSE;

    TRACE("(%s, %p, %08x, %p, %p, %p, %p, %p)\n", debugstr_a(pszUrlOid),
     pvPara, dwFlags, pUrlArray, pcbUrlArray, pUrlInfo, pcbUrlInfo, pvReserved);

    if (!HIWORD(pszUrlOid))
    {
        switch (LOWORD(pszUrlOid))
        {
        case LOWORD(URL_OID_CERTIFICATE_ISSUER):
            func = CRYPT_GetUrlFromCertificateIssuer;
            break;
        case LOWORD(URL_OID_CERTIFICATE_CRL_DIST_POINT):
            func = CRYPT_GetUrlFromCertificateCRLDistPoint;
            break;
        default:
            FIXME("unimplemented for %s\n", url_oid_to_str(pszUrlOid));
            SetLastError(ERROR_FILE_NOT_FOUND);
        }
    }
    else
    {
        static HCRYPTOIDFUNCSET set = NULL;

        if (!set)
            set = CryptInitOIDFunctionSet(URL_OID_GET_OBJECT_URL_FUNC, 0);
        CryptGetOIDFunctionAddress(set, X509_ASN_ENCODING, pszUrlOid, 0,
         (void **)&func, &hFunc);
    }
    if (func)
        ret = func(pszUrlOid, pvPara, dwFlags, pUrlArray, pcbUrlArray,
         pUrlInfo, pcbUrlInfo, pvReserved);
    if (hFunc)
        CryptFreeOIDFunctionAddress(hFunc, 0);
    return ret;
}

/***********************************************************************
 *    CryptRetrieveObjectByUrlA (CRYPTNET.@)
 */
BOOL WINAPI CryptRetrieveObjectByUrlA(LPCSTR pszURL, LPCSTR pszObjectOid,
 DWORD dwRetrievalFlags, DWORD dwTimeout, LPVOID *ppvObject,
 HCRYPTASYNC hAsyncRetrieve, PCRYPT_CREDENTIALS pCredentials, LPVOID pvVerify,
 PCRYPT_RETRIEVE_AUX_INFO pAuxInfo)
{
    BOOL ret = FALSE;
    int len;

    TRACE("(%s, %s, %08x, %d, %p, %p, %p, %p, %p)\n", debugstr_a(pszURL),
     debugstr_a(pszObjectOid), dwRetrievalFlags, dwTimeout, ppvObject,
     hAsyncRetrieve, pCredentials, pvVerify, pAuxInfo);

    if (!pszURL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    len = MultiByteToWideChar(CP_ACP, 0, pszURL, -1, NULL, 0);
    if (len)
    {
        LPWSTR url = CryptMemAlloc(len * sizeof(WCHAR));

        if (url)
        {
            MultiByteToWideChar(CP_ACP, 0, pszURL, -1, url, len);
            ret = CryptRetrieveObjectByUrlW(url, pszObjectOid,
             dwRetrievalFlags, dwTimeout, ppvObject, hAsyncRetrieve,
             pCredentials, pvVerify, pAuxInfo);
            CryptMemFree(url);
        }
        else
            SetLastError(ERROR_OUTOFMEMORY);
    }
    return ret;
}

/***********************************************************************
 *    CryptRetrieveObjectByUrlW (CRYPTNET.@)
 */
BOOL WINAPI CryptRetrieveObjectByUrlW(LPCWSTR pszURL, LPCSTR pszObjectOid,
 DWORD dwRetrievalFlags, DWORD dwTimeout, LPVOID *ppvObject,
 HCRYPTASYNC hAsyncRetrieve, PCRYPT_CREDENTIALS pCredentials, LPVOID pvVerify,
 PCRYPT_RETRIEVE_AUX_INFO pAuxInfo)
{
    FIXME("(%s, %s, %08x, %d, %p, %p, %p, %p, %p)\n", debugstr_w(pszURL),
     debugstr_a(pszObjectOid), dwRetrievalFlags, dwTimeout, ppvObject,
     hAsyncRetrieve, pCredentials, pvVerify, pAuxInfo);
    return FALSE;
}

/***********************************************************************
 *    CertDllVerifyRevocation (CRYPTNET.@)
 */
BOOL WINAPI CertDllVerifyRevocation(DWORD dwEncodingType, DWORD dwRevType,
 DWORD cContext, void *rgpvContext[], DWORD dwFlags,
 PCERT_REVOCATION_PARA pRevPara, PCERT_REVOCATION_STATUS pRevStatus)
{
   FIXME("(%08x, %d, %d, %p, %08x, %p, %p): stub\n", dwEncodingType, dwRevType,
    cContext, rgpvContext, dwFlags, pRevPara, pRevStatus);
   return FALSE;
}
