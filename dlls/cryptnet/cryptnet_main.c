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
#include "wininet.h"
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
            {
                SetLastError(E_INVALIDARG);
                ret = FALSE;
            }
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

static void WINAPI CRYPT_FreeBlob(LPCSTR pszObjectOid,
 PCRYPT_BLOB_ARRAY pObject, void *pvFreeContext)
{
    DWORD i;

    for (i = 0; i < pObject->cBlob; i++)
        CryptMemFree(pObject->rgBlob[i].pbData);
    CryptMemFree(pObject->rgBlob);
}

static BOOL WINAPI FTP_RetrieveEncodedObjectW(LPCWSTR pszURL,
 LPCSTR pszObjectOid, DWORD dwRetrievalFlags, DWORD dwTimeout,
 PCRYPT_BLOB_ARRAY pObject, PFN_FREE_ENCODED_OBJECT_FUNC *ppfnFreeObject,
 void **ppvFreeContext, HCRYPTASYNC hAsyncRetrieve,
 PCRYPT_CREDENTIALS pCredentials, PCRYPT_RETRIEVE_AUX_INFO pAuxInfo)
{
    FIXME("(%s, %s, %08x, %d, %p, %p, %p, %p, %p, %p)\n", debugstr_w(pszURL),
     debugstr_a(pszObjectOid), dwRetrievalFlags, dwTimeout, pObject,
     ppfnFreeObject, ppvFreeContext, hAsyncRetrieve, pCredentials, pAuxInfo);

    pObject->cBlob = 0;
    pObject->rgBlob = NULL;
    *ppfnFreeObject = CRYPT_FreeBlob;
    *ppvFreeContext = NULL;
    return FALSE;
}

static BOOL WINAPI HTTP_RetrieveEncodedObjectW(LPCWSTR pszURL,
 LPCSTR pszObjectOid, DWORD dwRetrievalFlags, DWORD dwTimeout,
 PCRYPT_BLOB_ARRAY pObject, PFN_FREE_ENCODED_OBJECT_FUNC *ppfnFreeObject,
 void **ppvFreeContext, HCRYPTASYNC hAsyncRetrieve,
 PCRYPT_CREDENTIALS pCredentials, PCRYPT_RETRIEVE_AUX_INFO pAuxInfo)
{
    FIXME("(%s, %s, %08x, %d, %p, %p, %p, %p, %p, %p)\n", debugstr_w(pszURL),
     debugstr_a(pszObjectOid), dwRetrievalFlags, dwTimeout, pObject,
     ppfnFreeObject, ppvFreeContext, hAsyncRetrieve, pCredentials, pAuxInfo);

    pObject->cBlob = 0;
    pObject->rgBlob = NULL;
    *ppfnFreeObject = CRYPT_FreeBlob;
    *ppvFreeContext = NULL;
    return FALSE;
}

static BOOL WINAPI File_RetrieveEncodedObjectW(LPCWSTR pszURL,
 LPCSTR pszObjectOid, DWORD dwRetrievalFlags, DWORD dwTimeout,
 PCRYPT_BLOB_ARRAY pObject, PFN_FREE_ENCODED_OBJECT_FUNC *ppfnFreeObject,
 void **ppvFreeContext, HCRYPTASYNC hAsyncRetrieve,
 PCRYPT_CREDENTIALS pCredentials, PCRYPT_RETRIEVE_AUX_INFO pAuxInfo)
{
    URL_COMPONENTSW components = { sizeof(components), 0 };
    BOOL ret;

    TRACE("(%s, %s, %08x, %d, %p, %p, %p, %p, %p, %p)\n", debugstr_w(pszURL),
     debugstr_a(pszObjectOid), dwRetrievalFlags, dwTimeout, pObject,
     ppfnFreeObject, ppvFreeContext, hAsyncRetrieve, pCredentials, pAuxInfo);

    pObject->cBlob = 0;
    pObject->rgBlob = NULL;
    *ppfnFreeObject = CRYPT_FreeBlob;
    *ppvFreeContext = NULL;

    components.dwUrlPathLength = 1;
    ret = InternetCrackUrlW(pszURL, 0, ICU_DECODE, &components);
    if (ret)
    {
        LPWSTR path;

        /* 3 == lstrlenW(L"c:") + 1 */
        path = CryptMemAlloc((components.dwUrlPathLength + 3) * sizeof(WCHAR));
        if (path)
        {
            HANDLE hFile;

            /* Try to create the file directly - Wine handles / in pathnames */
            lstrcpynW(path, components.lpszUrlPath,
             components.dwUrlPathLength + 1);
            hFile = CreateFileW(path, GENERIC_READ, 0, NULL, OPEN_EXISTING,
             FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile == INVALID_HANDLE_VALUE)
            {
                /* Try again on the current drive */
                GetCurrentDirectoryW(components.dwUrlPathLength, path);
                if (path[1] == ':')
                {
                    lstrcpynW(path + 2, components.lpszUrlPath,
                     components.dwUrlPathLength + 1);
                    hFile = CreateFileW(path, GENERIC_READ, 0, NULL,
                     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                }
                if (hFile == INVALID_HANDLE_VALUE)
                {
                    /* Try again on the Windows drive */
                    GetWindowsDirectoryW(path, components.dwUrlPathLength);
                    if (path[1] == ':')
                    {
                        lstrcpynW(path + 2, components.lpszUrlPath,
                         components.dwUrlPathLength + 1);
                        hFile = CreateFileW(path, GENERIC_READ, 0, NULL,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                    }
                }
            }
            if (hFile != INVALID_HANDLE_VALUE)
            {
                LARGE_INTEGER size;

                if ((ret = GetFileSizeEx(hFile, &size)))
                {
                    if (size.HighPart)
                    {
                        WARN("file too big\n");
                        SetLastError(ERROR_INVALID_DATA);
                        ret = FALSE;
                    }
                    else
                    {
                        CRYPT_DATA_BLOB blob;

                        blob.pbData = CryptMemAlloc(size.LowPart);
                        if (blob.pbData)
                        {
                            blob.cbData = size.LowPart;
                            ret = ReadFile(hFile, blob.pbData, size.LowPart,
                             &blob.cbData, NULL);
                            if (ret)
                            {
                                pObject->rgBlob =
                                 CryptMemAlloc(sizeof(CRYPT_DATA_BLOB));
                                if (pObject->rgBlob)
                                {
                                    pObject->cBlob = 1;
                                    memcpy(pObject->rgBlob, &blob,
                                     sizeof(CRYPT_DATA_BLOB));
                                }
                                else
                                {
                                    SetLastError(ERROR_OUTOFMEMORY);
                                    ret = FALSE;
                                }
                            }
                            if (!ret)
                                CryptMemFree(blob.pbData);
                            else
                            {
                                if (pAuxInfo && pAuxInfo->cbSize >=
                                 offsetof(CRYPT_RETRIEVE_AUX_INFO,
                                 pLastSyncTime) + sizeof(PFILETIME) &&
                                 pAuxInfo->pLastSyncTime)
                                    GetFileTime(hFile, NULL, NULL,
                                     pAuxInfo->pLastSyncTime);
                            }
                        }
                        else
                        {
                            SetLastError(ERROR_OUTOFMEMORY);
                            ret = FALSE;
                        }
                    }
                }
                CloseHandle(hFile);
            }
            else
                ret = FALSE;
            CryptMemFree(path);
        }
    }
    return ret;
}

typedef BOOL (WINAPI *SchemeDllRetrieveEncodedObjectW)(LPCWSTR pwszUrl,
 LPCSTR pszObjectOid, DWORD dwRetrievalFlags, DWORD dwTimeout,
 PCRYPT_BLOB_ARRAY pObject, PFN_FREE_ENCODED_OBJECT_FUNC *ppfnFreeObject,
 void **ppvFreeContext, HCRYPTASYNC hAsyncRetrieve,
 PCRYPT_CREDENTIALS pCredentials, PCRYPT_RETRIEVE_AUX_INFO pAuxInfo);

static BOOL CRYPT_GetRetrieveFunction(LPCWSTR pszURL,
 SchemeDllRetrieveEncodedObjectW *pFunc, HCRYPTOIDFUNCADDR *phFunc)
{
    URL_COMPONENTSW components = { sizeof(components), 0 };
    BOOL ret;

    *pFunc = NULL;
    *phFunc = 0;
    components.dwSchemeLength = 1;
    ret = InternetCrackUrlW(pszURL, 0, ICU_DECODE, &components);
    if (ret)
    {
        /* Microsoft always uses CryptInitOIDFunctionSet/
         * CryptGetOIDFunctionAddress, but there doesn't seem to be a pressing
         * reason to do so for builtin schemes.
         */
        switch (components.nScheme)
        {
        case INTERNET_SCHEME_FTP:
            *pFunc = FTP_RetrieveEncodedObjectW;
            break;
        case INTERNET_SCHEME_HTTP:
            *pFunc = HTTP_RetrieveEncodedObjectW;
            break;
        case INTERNET_SCHEME_FILE:
            *pFunc = File_RetrieveEncodedObjectW;
            break;
        default:
        {
            int len = WideCharToMultiByte(CP_ACP, 0, components.lpszScheme,
             components.dwSchemeLength, NULL, 0, NULL, NULL);

            if (len)
            {
                LPSTR scheme = CryptMemAlloc(len);

                if (scheme)
                {
                    static HCRYPTOIDFUNCSET set = NULL;

                    if (!set)
                        set = CryptInitOIDFunctionSet(
                         SCHEME_OID_RETRIEVE_ENCODED_OBJECTW_FUNC, 0);
                    WideCharToMultiByte(CP_ACP, 0, components.lpszScheme,
                     components.dwSchemeLength, scheme, len, NULL, NULL);
                    ret = CryptGetOIDFunctionAddress(set, X509_ASN_ENCODING,
                     scheme, 0, (void **)pFunc, phFunc);
                    CryptMemFree(scheme);
                }
                else
                {
                    SetLastError(ERROR_OUTOFMEMORY);
                    ret = FALSE;
                }
            }
            else
                ret = FALSE;
        }
        }
    }
    return ret;
}

static BOOL WINAPI CRYPT_CreateBlob(LPCSTR pszObjectOid,
 DWORD dwRetrievalFlags, PCRYPT_BLOB_ARRAY pObject, void **ppvContext)
{
    DWORD size, i;
    CRYPT_BLOB_ARRAY *context;
    BOOL ret = FALSE;

    size = sizeof(CRYPT_BLOB_ARRAY) + pObject->cBlob * sizeof(CRYPT_DATA_BLOB);
    for (i = 0; i < pObject->cBlob; i++)
        size += pObject->rgBlob[i].cbData;
    context = CryptMemAlloc(size);
    if (context)
    {
        LPBYTE nextData;

        context->cBlob = 0;
        context->rgBlob =
         (CRYPT_DATA_BLOB *)((LPBYTE)context + sizeof(CRYPT_BLOB_ARRAY));
        nextData =
         (LPBYTE)context->rgBlob + pObject->cBlob * sizeof(CRYPT_DATA_BLOB);
        for (i = 0; i < pObject->cBlob; i++)
        {
            memcpy(nextData, pObject->rgBlob[i].pbData,
             pObject->rgBlob[i].cbData);
            context->rgBlob[i].pbData = nextData;
            context->rgBlob[i].cbData = pObject->rgBlob[i].cbData;
            nextData += pObject->rgBlob[i].cbData;
            context->cBlob++;
        }
        *ppvContext = context;
        ret = TRUE;
    }
    return ret;
}

typedef BOOL (WINAPI *AddContextToStore)(HCERTSTORE hCertStore,
 const void *pContext, DWORD dwAddDisposition, const void **ppStoreContext);

static BOOL CRYPT_CreateContext(PCRYPT_BLOB_ARRAY pObject,
 DWORD dwExpectedContentTypeFlags, AddContextToStore addFunc, void **ppvContext)
{
    BOOL ret = TRUE;

    if (!pObject->cBlob)
    {
        SetLastError(ERROR_INVALID_DATA);
        *ppvContext = NULL;
        ret = FALSE;
    }
    else if (pObject->cBlob == 1)
    {
        if (!CryptQueryObject(CERT_QUERY_OBJECT_BLOB, &pObject->rgBlob[0],
         dwExpectedContentTypeFlags, CERT_QUERY_FORMAT_FLAG_BINARY, 0, NULL,
         NULL, NULL, NULL, NULL, (const void **)ppvContext))
        {
            SetLastError(CRYPT_E_NO_MATCH);
            ret = FALSE;
        }
    }
    else
    {
        HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
         CERT_STORE_CREATE_NEW_FLAG, NULL);

        if (store)
        {
            DWORD i;
            const void *context;

            for (i = 0; i < pObject->cBlob; i++)
            {
                if (CryptQueryObject(CERT_QUERY_OBJECT_BLOB,
                 &pObject->rgBlob[i], dwExpectedContentTypeFlags,
                 CERT_QUERY_FORMAT_FLAG_BINARY, 0, NULL, NULL, NULL, NULL,
                 NULL, &context))
                {
                    if (!addFunc(store, context, CERT_STORE_ADD_ALWAYS, NULL))
                        ret = FALSE;
                }
                else
                {
                    SetLastError(CRYPT_E_NO_MATCH);
                    ret = FALSE;
                }
            }
        }
        else
            ret = FALSE;
        *ppvContext = store;
    }
    return ret;
}

static BOOL WINAPI CRYPT_CreateCert(LPCSTR pszObjectOid,
 DWORD dwRetrievalFlags, PCRYPT_BLOB_ARRAY pObject, void **ppvContext)
{
    return CRYPT_CreateContext(pObject, CERT_QUERY_CONTENT_FLAG_CERT,
     (AddContextToStore)CertAddCertificateContextToStore, ppvContext);
}

static BOOL WINAPI CRYPT_CreateCRL(LPCSTR pszObjectOid,
 DWORD dwRetrievalFlags, PCRYPT_BLOB_ARRAY pObject, void **ppvContext)
{
    return CRYPT_CreateContext(pObject, CERT_QUERY_CONTENT_FLAG_CRL,
     (AddContextToStore)CertAddCRLContextToStore, ppvContext);
}

static BOOL WINAPI CRYPT_CreateCTL(LPCSTR pszObjectOid,
 DWORD dwRetrievalFlags, PCRYPT_BLOB_ARRAY pObject, void **ppvContext)
{
    return CRYPT_CreateContext(pObject, CERT_QUERY_CONTENT_FLAG_CTL,
     (AddContextToStore)CertAddCTLContextToStore, ppvContext);
}

static BOOL WINAPI CRYPT_CreatePKCS7(LPCSTR pszObjectOid,
 DWORD dwRetrievalFlags, PCRYPT_BLOB_ARRAY pObject, void **ppvContext)
{
    BOOL ret;

    if (!pObject->cBlob)
    {
        SetLastError(ERROR_INVALID_DATA);
        *ppvContext = NULL;
        ret = FALSE;
    }
    else if (pObject->cBlob == 1)
        ret = CryptQueryObject(CERT_QUERY_OBJECT_BLOB, &pObject->rgBlob[0],
         CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED |
         CERT_QUERY_CONTENT_FLAG_PKCS7_UNSIGNED, CERT_QUERY_FORMAT_FLAG_BINARY,
         0, NULL, NULL, NULL, (HCERTSTORE *)ppvContext, NULL, NULL);
    else
    {
        FIXME("multiple messages unimplemented\n");
        ret = FALSE;
    }
    return ret;
}

static BOOL WINAPI CRYPT_CreateAny(LPCSTR pszObjectOid,
 DWORD dwRetrievalFlags, PCRYPT_BLOB_ARRAY pObject, void **ppvContext)
{
    BOOL ret;

    if (!pObject->cBlob)
    {
        SetLastError(ERROR_INVALID_DATA);
        *ppvContext = NULL;
        ret = FALSE;
    }
    else
    {
        HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, 0,
         CERT_STORE_CREATE_NEW_FLAG, NULL);

        if (store)
        {
            HCERTSTORE memStore = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
             CERT_STORE_CREATE_NEW_FLAG, NULL);

            if (memStore)
            {
                CertAddStoreToCollection(store, memStore,
                 CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
                CertCloseStore(memStore, 0);
            }
            else
            {
                CertCloseStore(store, 0);
                store = NULL;
            }
        }
        if (store)
        {
            DWORD i;

            ret = TRUE;
            for (i = 0; i < pObject->cBlob; i++)
            {
                DWORD contentType, expectedContentTypes =
                 CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED |
                 CERT_QUERY_CONTENT_FLAG_PKCS7_UNSIGNED |
                 CERT_QUERY_CONTENT_FLAG_CERT |
                 CERT_QUERY_CONTENT_FLAG_CRL |
                 CERT_QUERY_CONTENT_FLAG_CTL;
                HCERTSTORE contextStore;
                const void *context;

                if (CryptQueryObject(CERT_QUERY_OBJECT_BLOB,
                 &pObject->rgBlob[i], expectedContentTypes,
                 CERT_QUERY_FORMAT_FLAG_BINARY, 0, NULL, &contentType, NULL,
                 &contextStore, NULL, &context))
                {
                    switch (contentType)
                    {
                    case CERT_QUERY_CONTENT_CERT:
                        if (!CertAddCertificateContextToStore(store,
                         (PCCERT_CONTEXT)context, CERT_STORE_ADD_ALWAYS, NULL))
                            ret = FALSE;
                        break;
                    case CERT_QUERY_CONTENT_CRL:
                        if (!CertAddCRLContextToStore(store,
                         (PCCRL_CONTEXT)context, CERT_STORE_ADD_ALWAYS, NULL))
                             ret = FALSE;
                        break;
                    case CERT_QUERY_CONTENT_CTL:
                        if (!CertAddCTLContextToStore(store,
                         (PCCTL_CONTEXT)context, CERT_STORE_ADD_ALWAYS, NULL))
                             ret = FALSE;
                        break;
                    default:
                        CertAddStoreToCollection(store, contextStore, 0, 0);
                    }
                }
                else
                    ret = FALSE;
            }
        }
        else
            ret = FALSE;
        *ppvContext = store;
    }
    return ret;
}

typedef BOOL (WINAPI *ContextDllCreateObjectContext)(LPCSTR pszObjectOid,
 DWORD dwRetrievalFlags, PCRYPT_BLOB_ARRAY pObject, void **ppvContext);

static BOOL CRYPT_GetCreateFunction(LPCSTR pszObjectOid,
 ContextDllCreateObjectContext *pFunc, HCRYPTOIDFUNCADDR *phFunc)
{
    BOOL ret = TRUE;

    *pFunc = NULL;
    *phFunc = 0;
    if (!HIWORD(pszObjectOid))
    {
        switch (LOWORD(pszObjectOid))
        {
        case 0:
            *pFunc = CRYPT_CreateBlob;
            break;
        case LOWORD(CONTEXT_OID_CERTIFICATE):
            *pFunc = CRYPT_CreateCert;
            break;
        case LOWORD(CONTEXT_OID_CRL):
            *pFunc = CRYPT_CreateCRL;
            break;
        case LOWORD(CONTEXT_OID_CTL):
            *pFunc = CRYPT_CreateCTL;
            break;
        case LOWORD(CONTEXT_OID_PKCS7):
            *pFunc = CRYPT_CreatePKCS7;
            break;
        case LOWORD(CONTEXT_OID_CAPI2_ANY):
            *pFunc = CRYPT_CreateAny;
            break;
        }
    }
    if (!*pFunc)
    {
        static HCRYPTOIDFUNCSET set = NULL;

        if (!set)
            set = CryptInitOIDFunctionSet(
             CONTEXT_OID_CREATE_OBJECT_CONTEXT_FUNC, 0);
        ret = CryptGetOIDFunctionAddress(set, X509_ASN_ENCODING, pszObjectOid,
         0, (void **)pFunc, phFunc);
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
    BOOL ret;
    SchemeDllRetrieveEncodedObjectW retrieve;
    ContextDllCreateObjectContext create;
    HCRYPTOIDFUNCADDR hRetrieve = 0, hCreate = 0;

    TRACE("(%s, %s, %08x, %d, %p, %p, %p, %p, %p)\n", debugstr_w(pszURL),
     debugstr_a(pszObjectOid), dwRetrievalFlags, dwTimeout, ppvObject,
     hAsyncRetrieve, pCredentials, pvVerify, pAuxInfo);

    if (!pszURL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    ret = CRYPT_GetRetrieveFunction(pszURL, &retrieve, &hRetrieve);
    if (ret)
        ret = CRYPT_GetCreateFunction(pszObjectOid, &create, &hCreate);
    if (ret)
    {
        CRYPT_BLOB_ARRAY object = { 0, NULL };
        PFN_FREE_ENCODED_OBJECT_FUNC freeObject;
        void *freeContext;

        ret = retrieve(pszURL, pszObjectOid, dwRetrievalFlags, dwTimeout,
         &object, &freeObject, &freeContext, hAsyncRetrieve, pCredentials,
         pAuxInfo);
        if (ret)
        {
            ret = create(pszObjectOid, dwRetrievalFlags, &object, ppvObject);
            freeObject(pszObjectOid, &object, freeContext);
        }
    }
    if (hCreate)
        CryptFreeOIDFunctionAddress(hCreate, 0);
    if (hRetrieve)
        CryptFreeOIDFunctionAddress(hRetrieve, 0);
    return ret;
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
