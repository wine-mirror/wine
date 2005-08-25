/*
 * Copyright 2002 Mike McCormack for CodeWeavers
 * Copyright 2004,2005 Juan Lang
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wincrypt.h"
#include "wine/debug.h"
#include "crypt32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

#define WINE_CRYPTCERTSTORE_MAGIC 0x74726563

typedef struct WINE_CRYPTCERTSTORE
{
    DWORD dwMagic;
} WINECRYPT_CERTSTORE;


/*
 * CertOpenStore
 *
 * System Store CA is
 *  HKLM\\Software\\Microsoft\\SystemCertificates\\CA\\
 *    Certificates\\<compressed guid>
 *         "Blob" = REG_BINARY
 *    CRLs\\<compressed guid>
 *         "Blob" = REG_BINARY
 *    CTLs\\<compressed guid>
 *         "Blob" = REG_BINARY
 */
HCERTSTORE WINAPI CertOpenStore( LPCSTR lpszStoreProvider,
              DWORD dwMsgAndCertEncodingType, HCRYPTPROV hCryptProv, 
              DWORD dwFlags, const void* pvPara )
{
    WINECRYPT_CERTSTORE *hcs;

    FIXME("%s %08lx %08lx %08lx %p stub\n", debugstr_a(lpszStoreProvider),
          dwMsgAndCertEncodingType, hCryptProv, dwFlags, pvPara);

    if( lpszStoreProvider == CERT_STORE_PROV_SYSTEM_A )
    {
        FIXME("pvPara = %s\n", debugstr_a( (LPCSTR) pvPara ) );
    }
    else if ( lpszStoreProvider == CERT_STORE_PROV_SYSTEM_W )
    {
        FIXME("pvPara = %s\n", debugstr_w( (LPCWSTR) pvPara ) );
    }

    hcs = HeapAlloc( GetProcessHeap(), 0, sizeof (WINECRYPT_CERTSTORE) );
    if( !hcs )
        return NULL;

    hcs->dwMagic = WINE_CRYPTCERTSTORE_MAGIC;

    return (HCERTSTORE) hcs;
}

HCERTSTORE WINAPI CertOpenSystemStoreA(HCRYPTPROV hProv,
 LPCSTR szSubSystemProtocol)
{
    return CertOpenStore( CERT_STORE_PROV_SYSTEM_A, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_SYSTEM_STORE_LOCAL_MACHINE |
     CERT_SYSTEM_STORE_USERS, szSubSystemProtocol );
}

HCERTSTORE WINAPI CertOpenSystemStoreW(HCRYPTPROV hProv,
 LPCWSTR szSubSystemProtocol)
{
    return CertOpenStore( CERT_STORE_PROV_SYSTEM_W, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_SYSTEM_STORE_LOCAL_MACHINE |
     CERT_SYSTEM_STORE_USERS, szSubSystemProtocol );
}

PCCERT_CONTEXT WINAPI CertEnumCertificatesInStore(HCERTSTORE hCertStore, PCCERT_CONTEXT pPrev)
{
    FIXME("(%p,%p)\n", hCertStore, pPrev);
    return NULL;
}

BOOL WINAPI CertSaveStore(HCERTSTORE hCertStore, DWORD dwMsgAndCertEncodingType,
             DWORD dwSaveAs, DWORD dwSaveTo, void* pvSaveToPara, DWORD dwFlags)
{
    FIXME("(%p,%ld,%ld,%ld,%p,%08lx) stub!\n", hCertStore, 
          dwMsgAndCertEncodingType, dwSaveAs, dwSaveTo, pvSaveToPara, dwFlags);
    return TRUE;
}

PCCRL_CONTEXT WINAPI CertCreateCRLContext( DWORD dwCertEncodingType,
  const BYTE* pbCrlEncoded, DWORD cbCrlEncoded)
{
    PCRL_CONTEXT pcrl;
    BYTE* data;

    TRACE("%08lx %p %08lx\n", dwCertEncodingType, pbCrlEncoded, cbCrlEncoded);

    pcrl = HeapAlloc( GetProcessHeap(), 0, sizeof (CRL_CONTEXT) );
    if( !pcrl )
        return NULL;

    data = HeapAlloc( GetProcessHeap(), 0, cbCrlEncoded );
    if( !data )
    {
        HeapFree( GetProcessHeap(), 0, pcrl );
        return NULL;
    }

    pcrl->dwCertEncodingType = dwCertEncodingType;
    pcrl->pbCrlEncoded       = data;
    pcrl->cbCrlEncoded       = cbCrlEncoded;
    pcrl->pCrlInfo           = NULL;
    pcrl->hCertStore         = 0;

    return pcrl;
}

BOOL WINAPI CertAddCRLContextToStore( HCERTSTORE hCertStore,
             PCCRL_CONTEXT pCrlContext, DWORD dwAddDisposition,
             PCCRL_CONTEXT* ppStoreContext )
{
    FIXME("%p %p %08lx %p\n", hCertStore, pCrlContext,
          dwAddDisposition, ppStoreContext);
    return TRUE;
}

BOOL WINAPI CertFreeCRLContext( PCCRL_CONTEXT pCrlContext)
{
    FIXME("%p\n", pCrlContext );

    return TRUE;
}

BOOL WINAPI CertCloseStore( HCERTSTORE hCertStore, DWORD dwFlags )
{
    WINECRYPT_CERTSTORE *hcs = (WINECRYPT_CERTSTORE *) hCertStore;

    FIXME("%p %08lx\n", hCertStore, dwFlags );
    if( ! hCertStore )
        return FALSE;

    if ( hcs->dwMagic != WINE_CRYPTCERTSTORE_MAGIC )
        return FALSE;

    hcs->dwMagic = 0;
    HeapFree( GetProcessHeap(), 0, hCertStore );

    return TRUE;
}

BOOL WINAPI CertFreeCertificateContext( PCCERT_CONTEXT pCertContext )
{
    FIXME("%p stub\n", pCertContext);
    return TRUE;
}

PCCERT_CONTEXT WINAPI CertFindCertificateInStore(HCERTSTORE hCertStore,
		DWORD dwCertEncodingType, DWORD dwFlags, DWORD dwType,
		const void *pvPara, PCCERT_CONTEXT pPrevCertContext)
{
    FIXME("stub: %p %ld %ld %ld %p %p\n", hCertStore, dwCertEncodingType,
	dwFlags, dwType, pvPara, pPrevCertContext);
    SetLastError(CRYPT_E_NOT_FOUND);
    return NULL;
}

PCRYPT_ATTRIBUTE WINAPI CertFindAttribute(LPCSTR pszObjId, DWORD cAttr,
 CRYPT_ATTRIBUTE rgAttr[])
{
    PCRYPT_ATTRIBUTE ret = NULL;
    DWORD i;

    TRACE("%s %ld %p\n", debugstr_a(pszObjId), cAttr, rgAttr);

    if (!cAttr)
        return NULL;
    if (!pszObjId)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    for (i = 0; !ret && i < cAttr; i++)
        if (rgAttr[i].pszObjId && !strcmp(pszObjId, rgAttr[i].pszObjId))
            ret = &rgAttr[i];
    return ret;
}

PCERT_EXTENSION WINAPI CertFindExtension(LPCSTR pszObjId, DWORD cExtensions,
 CERT_EXTENSION rgExtensions[])
{
    PCERT_EXTENSION ret = NULL;
    DWORD i;

    TRACE("%s %ld %p\n", debugstr_a(pszObjId), cExtensions, rgExtensions);

    if (!cExtensions)
        return NULL;
    if (!pszObjId)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    for (i = 0; !ret && i < cExtensions; i++)
        if (rgExtensions[i].pszObjId && !strcmp(pszObjId,
         rgExtensions[i].pszObjId))
            ret = &rgExtensions[i];
    return ret;
}

PCERT_RDN_ATTR WINAPI CertFindRDNAttr(LPCSTR pszObjId, PCERT_NAME_INFO pName)
{
    PCERT_RDN_ATTR ret = NULL;
    DWORD i, j;

    TRACE("%s %p\n", debugstr_a(pszObjId), pName);

    if (!pszObjId)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    for (i = 0; !ret && i < pName->cRDN; i++)
        for (j = 0; !ret && j < pName->rgRDN[i].cRDNAttr; j++)
            if (pName->rgRDN[i].rgRDNAttr[j].pszObjId && !strcmp(pszObjId,
             pName->rgRDN[i].rgRDNAttr[j].pszObjId))
                ret = &pName->rgRDN[i].rgRDNAttr[j];
    return ret;
}

LONG WINAPI CertVerifyTimeValidity(LPFILETIME pTimeToVerify,
 PCERT_INFO pCertInfo)
{
    FILETIME fileTime;
    LONG ret;

    if (!pTimeToVerify)
    {
        SYSTEMTIME sysTime;

        GetSystemTime(&sysTime);
        SystemTimeToFileTime(&sysTime, &fileTime);
        pTimeToVerify = &fileTime;
    }
    if ((ret = CompareFileTime(pTimeToVerify, &pCertInfo->NotBefore)) >= 0)
    {
        ret = CompareFileTime(pTimeToVerify, &pCertInfo->NotAfter);
        if (ret < 0)
            ret = 0;
    }
    return ret;
}

BOOL WINAPI CryptHashCertificate(HCRYPTPROV hCryptProv, ALG_ID Algid,
 DWORD dwFlags, const BYTE *pbEncoded, DWORD cbEncoded, BYTE *pbComputedHash,
 DWORD *pcbComputedHash)
{
    BOOL ret = TRUE;
    HCRYPTHASH hHash = 0;

    TRACE("(%ld, %d, %08lx, %p, %ld, %p, %p)\n", hCryptProv, Algid, dwFlags,
     pbEncoded, cbEncoded, pbComputedHash, pcbComputedHash);

    if (!hCryptProv)
        hCryptProv = CRYPT_GetDefaultProvider();
    if (!Algid)
        Algid = CALG_SHA1;
    if (ret)
    {
        ret = CryptCreateHash(hCryptProv, Algid, 0, 0, &hHash);
        if (ret)
        {
            ret = CryptHashData(hHash, pbEncoded, cbEncoded, 0);
            if (ret)
                ret = CryptGetHashParam(hHash, HP_HASHVAL, pbComputedHash,
                 pcbComputedHash, 0);
            CryptDestroyHash(hHash);
        }
    }
    return ret;
}
