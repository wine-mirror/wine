/*
 * WinTrust Cryptography functions
 *
 * Copyright 2006 James Hawkins
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wintrust.h"
#include "mscat.h"
#include "mssip.h"
#include "imagehlp.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wintrust);

/***********************************************************************
 *      CryptCATAdminAcquireContext (WINTRUST.@)
 *
 * Get a catalog administrator context handle.
 *
 * PARAMS
 *   catAdmin  [O] Pointer to the context handle.
 *   sysSystem [I] Pointer to a GUID for the needed subsystem.
 *   dwFlags   [I] Reserved.
 *
 * RETURNS
 *   Success: TRUE. catAdmin contains the context handle.
 *   Failure: FALSE.
 *
 */
BOOL WINAPI CryptCATAdminAcquireContext(HCATADMIN* catAdmin,
                    const GUID *sysSystem, DWORD dwFlags )
{
    FIXME("%p %s %x\n", catAdmin, debugstr_guid(sysSystem), dwFlags);

    if (catAdmin) *catAdmin = (HCATADMIN)0xdeadbeef;
    return TRUE;
}

/***********************************************************************
 *             CryptCATAdminAddCatalog (WINTRUST.@)
 */
BOOL WINAPI CryptCATAdminAddCatalog(HCATADMIN catAdmin, PWSTR catalogFile,
                                    PWSTR selectBaseName, DWORD flags)
{
    FIXME("%p %s %s %d\n", catAdmin, debugstr_w(catalogFile),
          debugstr_w(selectBaseName), flags);
    return TRUE;
}

/***********************************************************************
 *             CryptCATAdminCalcHashFromFileHandle (WINTRUST.@)
 */
BOOL WINAPI CryptCATAdminCalcHashFromFileHandle(HANDLE hFile, DWORD* pcbHash,
                                                BYTE* pbHash, DWORD dwFlags )
{
    FIXME("%p %p %p %x\n", hFile, pcbHash, pbHash, dwFlags);

    if (pbHash && pcbHash) memset(pbHash, 0, *pcbHash);
    return TRUE;
}

/***********************************************************************
 *             CryptCATAdminEnumCatalogFromHash (WINTRUST.@)
 */
HCATINFO WINAPI CryptCATAdminEnumCatalogFromHash(HCATADMIN hCatAdmin,
                                                 BYTE* pbHash,
                                                 DWORD cbHash,
                                                 DWORD dwFlags,
                                                 HCATINFO* phPrevCatInfo )
{
    FIXME("%p %p %d %d %p\n", hCatAdmin, pbHash, cbHash, dwFlags, phPrevCatInfo);
    return NULL;
}

/***********************************************************************
 *      CryptCATAdminReleaseCatalogContext (WINTRUST.@)
 *
 * Release a catalog context handle.
 *
 * PARAMS
 *   hCatAdmin [I] Context handle.
 *   hCatInfo  [I] Catalog handle.
 *   dwFlags   [I] Reserved.
 *
 * RETURNS
 *   Success: TRUE.
 *   Failure: FAIL.
 *
 */
BOOL WINAPI CryptCATAdminReleaseCatalogContext(HCATADMIN hCatAdmin,
                                               HCATINFO hCatInfo,
                                               DWORD dwFlags)
{
    FIXME("%p %p %x\n", hCatAdmin, hCatInfo, dwFlags);
    return TRUE;
}

/***********************************************************************
 *      CryptCATAdminReleaseContext (WINTRUST.@)
 *
 * Release a catalog administrator context handle.
 *
 * PARAMS
 *   catAdmin  [I] Context handle.
 *   dwFlags   [I] Reserved.
 *
 * RETURNS
 *   Success: TRUE.
 *   Failure: FAIL.
 *
 */
BOOL WINAPI CryptCATAdminReleaseContext(HCATADMIN hCatAdmin, DWORD dwFlags )
{
    FIXME("%p %x\n", hCatAdmin, dwFlags);
    return TRUE;
}

/***********************************************************************
 *      CryptCATAdminRemoveCatalog (WINTRUST.@)
 *
 * Remove a catalog file.
 *
 * PARAMS
 *   catAdmin         [I] Context handle.
 *   pwszCatalogFile  [I] Catalog file.
 *   dwFlags          [I] Reserved.
 *
 * RETURNS
 *   Success: TRUE.
 *   Failure: FALSE.
 *
 */
BOOL WINAPI CryptCATAdminRemoveCatalog(HCATADMIN hCatAdmin, LPCWSTR pwszCatalogFile, DWORD dwFlags)
{
    FIXME("%p %s %x\n", hCatAdmin, debugstr_w(pwszCatalogFile), dwFlags);
    return DeleteFileW(pwszCatalogFile);
}

/***********************************************************************
 *      CryptCATClose  (WINTRUST.@)
 */
BOOL WINAPI CryptCATClose(HANDLE hCatalog)
{
    FIXME("(%p) stub\n", hCatalog);
    return TRUE;
}

/***********************************************************************
 *      CryptCATEnumerateMember  (WINTRUST.@)
 */
CRYPTCATMEMBER *WINAPI CryptCATEnumerateMember(HANDLE hCatalog, CRYPTCATMEMBER* pPrevMember)
{
    FIXME("(%p, %p) stub\n", hCatalog, pPrevMember);
    return NULL;
}

/***********************************************************************
 *      CryptCATOpen  (WINTRUST.@)
 */
HANDLE WINAPI CryptCATOpen(LPWSTR pwszFileName, DWORD fdwOpenFlags, HCRYPTPROV hProv,
                           DWORD dwPublicVersion, DWORD dwEncodingType)
{
    FIXME("(%s, %d, %ld, %d, %d) stub\n", debugstr_w(pwszFileName), fdwOpenFlags,
          hProv, dwPublicVersion, dwEncodingType);
    return 0;
}

/***********************************************************************
 *      CryptSIPCreateIndirectData  (WINTRUST.@)
 */
BOOL WINAPI CryptSIPCreateIndirectData(SIP_SUBJECTINFO* pSubjectInfo, DWORD* pcbIndirectData,
                                       SIP_INDIRECT_DATA* pIndirectData)
{
    FIXME("(%p %p %p) stub\n", pSubjectInfo, pcbIndirectData, pIndirectData);
 
    return FALSE;
}

/***********************************************************************
 *      CryptSIPGetSignedDataMsg  (WINTRUST.@)
 */
BOOL WINAPI CryptSIPGetSignedDataMsg(SIP_SUBJECTINFO* pSubjectInfo, DWORD* pdwEncodingType,
                                       DWORD dwIndex, DWORD* pcbSignedDataMsg, BYTE* pbSignedDataMsg)
{
    BOOL ret;
    WIN_CERTIFICATE *pCert = NULL;

    TRACE("(%p %p %d %p %p)\n", pSubjectInfo, pdwEncodingType, dwIndex,
          pcbSignedDataMsg, pbSignedDataMsg);
 
    if (!pbSignedDataMsg)
    {
        WIN_CERTIFICATE cert;

        /* app hasn't passed buffer, just get the length */
        ret = ImageGetCertificateHeader(pSubjectInfo->hFile, dwIndex, &cert);
        if (ret)
            *pcbSignedDataMsg = cert.dwLength;
    }
    else
    {
        DWORD len;

        ret = ImageGetCertificateData(pSubjectInfo->hFile, dwIndex, NULL, &len);
        if (!ret)
            goto error;
        pCert = HeapAlloc(GetProcessHeap(), 0, len);
        if (!pCert)
        {
            ret = FALSE;
            goto error;
        }
        ret = ImageGetCertificateData(pSubjectInfo->hFile, dwIndex, pCert,
         &len);
        if (!ret)
            goto error;
        if (*pcbSignedDataMsg < pCert->dwLength)
        {
            *pcbSignedDataMsg = pCert->dwLength;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            ret = FALSE;
        }
        else
        {
            memcpy(pbSignedDataMsg, pCert->bCertificate, pCert->dwLength);
            switch (pCert->wCertificateType)
            {
            case WIN_CERT_TYPE_X509:
                *pdwEncodingType = X509_ASN_ENCODING;
                break;
            case WIN_CERT_TYPE_PKCS_SIGNED_DATA:
                *pdwEncodingType = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;
                break;
            default:
                FIXME("don't know what to do for encoding type %d\n",
                 pCert->wCertificateType);
                *pdwEncodingType = 0;
            }
        }
    }

error:
    HeapFree(GetProcessHeap(), 0, pCert);
    TRACE("returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *      CryptSIPPutSignedDataMsg  (WINTRUST.@)
 */
BOOL WINAPI CryptSIPPutSignedDataMsg(SIP_SUBJECTINFO* pSubjectInfo, DWORD pdwEncodingType,
                                       DWORD* pdwIndex, DWORD cbSignedDataMsg, BYTE* pbSignedDataMsg)
{
    FIXME("(%p %d %p %d %p) stub\n", pSubjectInfo, pdwEncodingType, pdwIndex,
          cbSignedDataMsg, pbSignedDataMsg);
 
    return FALSE;
}

/***********************************************************************
 *      CryptSIPRemoveSignedDataMsg  (WINTRUST.@)
 */
BOOL WINAPI CryptSIPRemoveSignedDataMsg(SIP_SUBJECTINFO* pSubjectInfo,
                                       DWORD dwIndex)
{
    FIXME("(%p %d) stub\n", pSubjectInfo, dwIndex);
 
    return FALSE;
}

/***********************************************************************
 *      CryptSIPVerifyIndirectData  (WINTRUST.@)
 */
BOOL WINAPI CryptSIPVerifyIndirectData(SIP_SUBJECTINFO* pSubjectInfo,
                                       SIP_INDIRECT_DATA* pIndirectData)
{
    FIXME("(%p %p) stub\n", pSubjectInfo, pIndirectData);
 
    return FALSE;
}
