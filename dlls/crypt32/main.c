/*
 * Copyright 2002 Mike McCormack for CodeWeavers
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

#include "config.h"
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "winreg.h"
#include "winnls.h"
#include "mssip.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

/* this function is called by Internet Explorer when it is about to verify a downloaded component */
BOOL WINAPI I_CryptCreateLruCache(DWORD x, DWORD y)
{
    FIXME("stub!\n");
    return FALSE;
}

/* these functions all have an unknown number of args */
BOOL WINAPI I_CryptFindLruEntryData(DWORD x)
{
    FIXME("stub!\n");
    return FALSE;
}

BOOL WINAPI I_CryptFlushLruCache(DWORD x)
{
    FIXME("stub!\n");
    return FALSE;
}

BOOL WINAPI I_CryptFreeLruCache(DWORD x)
{
    FIXME("stub!\n");
    return FALSE;
}

BOOL WINAPI CryptProtectData(DATA_BLOB* pDataIn, LPCWSTR szDataDescr, DATA_BLOB* pOptionalEntropy,
                             PVOID pvReserved, CRYPTPROTECT_PROMPTSTRUCT* pPromptStruct,
                             DWORD dwFlags, DATA_BLOB* pDataOut)
{
    FIXME("stub!\n");
    return FALSE;
}

/*
 * (0x1001350, %eax, 0, 0, 9);
 *
 */
BOOL WINAPI CertOpenStore(LPSTR dw1, DWORD dw2, DWORD dw3, DWORD dw4, DWORD dw5)
{
  FIXME("(%s, %ld, %ld, %ld, %ld), stub.\n", debugstr_a(dw1), dw2, dw3, dw4, dw5);
  return TRUE;
}

BOOL WINAPI CryptSIPRemoveProvider(GUID *pgProv)
{
    FIXME("stub!\n");
    return FALSE;
}

/* convert a guid to a wide character string */
static void CRYPT_guid2wstr( LPGUID guid, LPWSTR wstr )
{
    char str[40];

    sprintf(str, "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
           guid->Data1, guid->Data2, guid->Data3,
           guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
           guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7] );
    MultiByteToWideChar( CP_ACP, 0, str, -1, wstr, 40 );
}

/*
 * Helper for CryptSIPAddProvider
 *
 * Add a registry key containing a dll name and function under
 *  "Software\\Microsoft\\Cryptography\\OID\\EncodingType 0\\<func>\\<guid>"
 */
static LONG CRYPT_SIPWriteFunction( LPGUID guid, LPCWSTR szKey, 
              LPCWSTR szDll, LPCWSTR szFunction )
{
    const WCHAR szOID[] = {
        'S','o','f','t','w','a','r','e','\\',
        'M','i','c','r','o','s','o','f','t','\\',
        'C','r','y','p','t','o','g','r','a','p','h','y','\\',
        'O','I','D','\\',
        'E','n','c','o','d','i','n','g','T','y','p','e',' ','0','\\',
        'C','r','y','p','t','S','I','P','D','l','l', 0 };
    const WCHAR szBackSlash[] = { '\\', 0 };
    const WCHAR szDllName[] = { 'D','l','l',0 };
    const WCHAR szFuncName[] = { 'F','u','n','c','N','a','m','e',0 };
    WCHAR szFullKey[ 0x100 ];
    LONG r;
    HKEY hKey;

    if( !szFunction )
         return ERROR_SUCCESS;

    /* max length of szFullKey depends on our code only, so we won't overrun */
    lstrcpyW( szFullKey, szOID );
    lstrcatW( szFullKey, szKey );
    lstrcatW( szFullKey, szBackSlash );
    CRYPT_guid2wstr( guid, &szFullKey[ lstrlenW( szFullKey ) ] );
    lstrcatW( szFullKey, szBackSlash );

    TRACE("key is %s\n", debugstr_w( szFullKey ) );

    r = RegCreateKeyW( HKEY_LOCAL_MACHINE, szFullKey, &hKey );
    if( r != ERROR_SUCCESS )
        return r;

    /* write the values */
    RegSetValueExW( hKey, szFuncName, 0, REG_SZ, (LPBYTE) szFunction,
                    ( lstrlenW( szFunction ) + 1 ) * sizeof (WCHAR) );
    RegSetValueExW( hKey, szDllName, 0, REG_SZ, (LPBYTE) szDll,
                    ( lstrlenW( szDll ) + 1) * sizeof (WCHAR) );

    RegCloseKey( hKey );

    return ERROR_SUCCESS;
}

BOOL WINAPI CryptSIPAddProvider(SIP_ADD_NEWPROVIDER *psNewProv)
{
    const WCHAR szCreate[] = { 
       'C','r','e','a','t','e',
       'I','n','d','i','r','e','c','t','D','a','t','a',0};
    const WCHAR szGetSigned[] = { 
       'G','e','t','S','i','g','n','e','d','D','a','t','a','M','s','g',0};
    const WCHAR szIsMyFile[] = { 
       'I','s','M','y','F','i','l','e','T','y','p','e', 0 };
    const WCHAR szPutSigned[] = { 
       'P','u','t','S','i','g','n','e','d','D','a','t','a','M','s','g',0};
    const WCHAR szRemoveSigned[] = {
       'R','e','m','o','v','e',
       'S','i','g','n','e','d','D','a','t','a','M','s','g',0};
    const WCHAR szVerify[] = {
       'V','e','r','i','f','y',
       'I','n','d','i','r','e','c','t','D','a','t','a',0};

    TRACE("%p\n", psNewProv);

    if( !psNewProv )
        return FALSE;

    TRACE("%s %s %s %s\n",
          debugstr_guid( psNewProv->pgSubject ),
          debugstr_w( psNewProv->pwszDLLFileName ),
          debugstr_w( psNewProv->pwszMagicNumber ),
          debugstr_w( psNewProv->pwszIsFunctionName ) );

#define CRYPT_SIPADDPROV( key, field ) \
    CRYPT_SIPWriteFunction( psNewProv->pgSubject, key, \
           psNewProv->pwszDLLFileName, psNewProv->field)

    CRYPT_SIPADDPROV( szGetSigned, pwszGetFuncName );
    CRYPT_SIPADDPROV( szPutSigned, pwszPutFuncName );
    CRYPT_SIPADDPROV( szCreate, pwszCreateFuncName );
    CRYPT_SIPADDPROV( szVerify, pwszVerifyFuncName );
    CRYPT_SIPADDPROV( szRemoveSigned, pwszRemoveFuncName );
    CRYPT_SIPADDPROV( szIsMyFile, pwszIsFunctionNameFmt2 );

#undef CRYPT_SIPADDPROV

    return TRUE;
}

BOOL WINAPI CryptSIPRetrieveSubjectGuid
      (LPCWSTR FileName, HANDLE hFileIn, GUID *pgSubject)
{
    FIXME("stub!\n");
    return FALSE;
}

BOOL WINAPI CryptSIPLoad
       (const GUID *pgSubject, DWORD dwFlags, SIP_DISPATCH_INFO *pSipDispatch)
{
    FIXME("stub!\n");
    return FALSE;
}

BOOL WINAPI CryptGetOIDFunctionValue(DWORD dwEncodingType, LPCSTR pszFuncName,
                                     LPCSTR pszOID, LPCWSTR pwszValueName,
                                     DWORD *pdwValueType, BYTE *pbValueData,
                                     DWORD *pcbValueData)
{
    FIXME("(%lx,%s,%s,%s,%p,%p,%p) stub!\n", dwEncodingType, pszFuncName, pszOID,
          debugstr_w(pwszValueName), pdwValueType, pbValueData, pcbValueData);
    return FALSE;
}

BOOL WINAPI CryptRegisterDefaultOIDFunction(DWORD dwEncodingType,
                                     LPCSTR pszFuncName, DWORD dwIndex,
				     LPCWSTR pwszDll)
{
    FIXME("(%lx,%s,%lx,%s) stub!\n", dwEncodingType, pszFuncName, dwIndex,
          debugstr_w(pwszDll));
    return FALSE;
}

BOOL WINAPI CryptRegisterOIDFunction(DWORD dwEncodingType, LPCSTR pszFuncName,
                  LPCSTR pszOID, LPCWSTR pwszDll, LPCSTR pszOverrideFuncName)
{
    LONG r;
    const char szOID[] = "Software\\Microsoft\\Cryptography\\OID";
    const char szType1[] = "EncodingType 1";
    const WCHAR szDllName[] = { 'D','l','l',0 };
    HKEY hKey;
    LPSTR szKey;
    UINT len;

    TRACE("%lx %s %s %s %s\n", dwEncodingType, pszFuncName, pszOID,
          debugstr_w(pwszDll), pszOverrideFuncName);

    if( dwEncodingType & PKCS_7_ASN_ENCODING )
        FIXME("PKCS_7_ASN_ENCODING not implemented\n");

    if( dwEncodingType & X509_ASN_ENCODING )
    {
        /* construct the name of the key */
        len = sizeof szOID + sizeof szType1 + 2 +
              lstrlenA( pszFuncName ) + lstrlenA( pszOID );
        szKey = HeapAlloc( GetProcessHeap(), 0, len );
        if( !szKey )
            return FALSE;
        sprintf( szKey, "%s\\%s\\%s\\%s",
                   szOID, szType1, pszFuncName, pszOID );

        TRACE("Key name is %s\n", debugstr_a( szKey ) );

        r = RegCreateKeyA( HKEY_LOCAL_MACHINE, szKey, &hKey );
        HeapFree( GetProcessHeap(), 0, szKey );
        if( r != ERROR_SUCCESS )
            return FALSE;

        /* write the values */
        RegSetValueExA( hKey, "FuncName", 0, REG_SZ, pszOverrideFuncName,
                        lstrlenA( pszOverrideFuncName ) + 1 );
        RegSetValueExW( hKey, szDllName, 0, REG_SZ, (LPBYTE) pwszDll,
                        (lstrlenW( pwszDll ) + 1) * sizeof (WCHAR) );

        RegCloseKey( hKey );
    }

    return TRUE;
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
    FIXME("%08lx %p %08lx\n", dwCertEncodingType, pbCrlEncoded, cbCrlEncoded);
    return NULL;
}

BOOL WINAPI CertCloseStore( HCERTSTORE hCertStore, DWORD dwFlags )
{
    FIXME("%p %08lx\n", hCertStore, dwFlags );
    return TRUE;
}

BOOL WINAPI CertFreeCertificateContext( PCCERT_CONTEXT pCertContext )
{
    FIXME("%p stub\n", pCertContext);
    return TRUE;
}
