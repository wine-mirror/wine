/*
 * Copyright 2002 Mike McCormack for CodeWeavers
 * Copyright 2005 Juan Lang
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
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "winreg.h"
#include "winnls.h"
#include "mssip.h"
#include "winuser.h"
#include "advpub.h"
#include "crypt32_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

static HCRYPTPROV hDefProv;

static const WCHAR szOID[] = {
    'S','o','f','t','w','a','r','e','\\',
    'M','i','c','r','o','s','o','f','t','\\',
    'C','r','y','p','t','o','g','r','a','p','h','y','\\',
    'O','I','D','\\',
    'E','n','c','o','d','i','n','g','T','y','p','e',' ','0','\\',
    'C','r','y','p','t','S','I','P','D','l','l', 0 };

static const WCHAR szBackSlash[] = { '\\', 0 };

static const WCHAR szPutSigned[] = {
    'P','u','t','S','i','g','n','e','d','D','a','t','a','M','s','g',0};
static const WCHAR szGetSigned[] = {
    'G','e','t','S','i','g','n','e','d','D','a','t','a','M','s','g',0};
static const WCHAR szRemoveSigned[] = {
    'R','e','m','o','v','e','S','i','g','n','e','d','D','a','t','a','M','s','g',0};
static const WCHAR szCreate[] = {
    'C','r','e','a','t','e','I','n','d','i','r','e','c','t','D','a','t','a',0};
static const WCHAR szVerify[] = {
    'V','e','r','i','f','y','I','n','d','i','r','e','c','t','D','a','t','a',0};
static const WCHAR szIsMyFile[] = {
    'I','s','M','y','F','i','l','e','T','y','p','e', 0 };
static const WCHAR szIsMyFile2[] = {
    'I','s','M','y','F','i','l','e','T','y','p','e','2', 0};

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstance);
            crypt_oid_init(hInstance);
            break;
        case DLL_PROCESS_DETACH:
            crypt_oid_free();
            if (hDefProv) CryptReleaseContext(hDefProv, 0);
            break;
    }
    return TRUE;
}

HCRYPTPROV CRYPT_GetDefaultProvider(void)
{
    if (!hDefProv)
        CryptAcquireContextW(&hDefProv, NULL, MS_ENHANCED_PROV_W,
         PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    return hDefProv;
}

typedef void * HLRUCACHE;

/* this function is called by Internet Explorer when it is about to verify a
 * downloaded component.  The first parameter appears to be a pointer to an
 * unknown type, native fails unless it points to a buffer of at least 20 bytes.
 * The second parameter appears to be an out parameter, whatever it's set to is
 * passed (by cryptnet.dll) to I_CryptFlushLruCache.
 */
BOOL WINAPI I_CryptCreateLruCache(void *unknown, HLRUCACHE *out)
{
    FIXME("(%p, %p): stub!\n", unknown, out);
    *out = (void *)0xbaadf00d;
    return TRUE;
}

BOOL WINAPI I_CryptFindLruEntryData(DWORD unk0, DWORD unk1, DWORD unk2)
{
    FIXME("(%08lx, %08lx, %08lx): stub!\n", unk0, unk1, unk2);
    return FALSE;
}

DWORD WINAPI I_CryptFlushLruCache(HLRUCACHE h, DWORD unk0, DWORD unk1)
{
    FIXME("(%p, %08lx, %08lx): stub!\n", h, unk0, unk1);
    return 0;
}

HLRUCACHE WINAPI I_CryptFreeLruCache(HLRUCACHE h, DWORD unk0, DWORD unk1)
{
    FIXME("(%p, %08lx, %08lx): stub!\n", h, unk0, unk1);
    return h;
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

/***********************************************************************
 *              CRYPT_SIPDeleteFunction
 *
 * Helper function for CryptSIPRemoveProvider
 */
static LONG CRYPT_SIPDeleteFunction( LPGUID guid, LPCWSTR szKey )
{
    WCHAR szFullKey[ 0x100 ];
    LONG r = ERROR_SUCCESS;

    /* max length of szFullKey depends on our code only, so we won't overrun */
    lstrcpyW( szFullKey, szOID );
    lstrcatW( szFullKey, szKey );
    lstrcatW( szFullKey, szBackSlash );
    CRYPT_guid2wstr( guid, &szFullKey[ lstrlenW( szFullKey ) ] );
    lstrcatW( szFullKey, szBackSlash );

    r = RegDeleteKeyW(HKEY_LOCAL_MACHINE, szFullKey);

    return r;
}

/***********************************************************************
 *             CryptSIPRemoveProvider (CRYPT32.@)
 *
 * Remove a SIP provider and its functions from the registry.
 *
 * PARAMS
 *  pgProv     [I] Pointer to a GUID for this SIP provider
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE. (Look at GetLastError()).
 *
 * NOTES
 *  Registry errors are always reported via SetLastError(). Every registry
 *  deletion will be tried.
 */
BOOL WINAPI CryptSIPRemoveProvider(GUID *pgProv)
{
    LONG r = ERROR_SUCCESS;
    LONG remove_error = ERROR_SUCCESS;

    TRACE("%s\n", debugstr_guid(pgProv));

    if (!pgProv)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }


#define CRYPT_SIPREMOVEPROV( key ) \
    r = CRYPT_SIPDeleteFunction( pgProv, key); \
    if (r != ERROR_SUCCESS) remove_error = r

    CRYPT_SIPREMOVEPROV( szPutSigned);
    CRYPT_SIPREMOVEPROV( szGetSigned);
    CRYPT_SIPREMOVEPROV( szRemoveSigned);
    CRYPT_SIPREMOVEPROV( szCreate);
    CRYPT_SIPREMOVEPROV( szVerify);
    CRYPT_SIPREMOVEPROV( szIsMyFile);
    CRYPT_SIPREMOVEPROV( szIsMyFile2);

#undef CRYPT_SIPREMOVEPROV

    if (remove_error != ERROR_SUCCESS)
    {
        SetLastError(remove_error);
        return FALSE;
    }

    return TRUE;
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
    static const WCHAR szDllName[] = { 'D','l','l',0 };
    static const WCHAR szFuncName[] = { 'F','u','n','c','N','a','m','e',0 };
    WCHAR szFullKey[ 0x100 ];
    LONG r = ERROR_SUCCESS;
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
    if( r != ERROR_SUCCESS ) goto error_close_key;

    /* write the values */
    r = RegSetValueExW( hKey, szFuncName, 0, REG_SZ, (const BYTE*) szFunction,
                        ( lstrlenW( szFunction ) + 1 ) * sizeof (WCHAR) );
    if( r != ERROR_SUCCESS ) goto error_close_key;
    r = RegSetValueExW( hKey, szDllName, 0, REG_SZ, (const BYTE*) szDll,
                        ( lstrlenW( szDll ) + 1) * sizeof (WCHAR) );

error_close_key:

    RegCloseKey( hKey );

    return r;
}

/***********************************************************************
 *             CryptSIPAddProvider (CRYPT32.@)
 *
 * Add a SIP provider and its functions to the registry.
 *
 * PARAMS
 *  psNewProv       [I] Pointer to a structure with information about
 *                      the functions this SIP provider can perform.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE. (Look at GetLastError()).
 *
 * NOTES
 *  Registry errors are always reported via SetLastError(). If a
 *  registry error occurs the rest of the registry write operations
 *  will be skipped.
 */
BOOL WINAPI CryptSIPAddProvider(SIP_ADD_NEWPROVIDER *psNewProv)
{
    LONG r = ERROR_SUCCESS;

    TRACE("%p\n", psNewProv);

    if (!psNewProv ||
        psNewProv->cbStruct != sizeof(SIP_ADD_NEWPROVIDER) ||
        !psNewProv->pwszGetFuncName ||
        !psNewProv->pwszPutFuncName ||
        !psNewProv->pwszCreateFuncName ||
        !psNewProv->pwszVerifyFuncName ||
        !psNewProv->pwszRemoveFuncName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    TRACE("%s %s %s %s\n",
          debugstr_guid( psNewProv->pgSubject ),
          debugstr_w( psNewProv->pwszDLLFileName ),
          debugstr_w( psNewProv->pwszMagicNumber ),
          debugstr_w( psNewProv->pwszIsFunctionName ) );

#define CRYPT_SIPADDPROV( key, field ) \
    r = CRYPT_SIPWriteFunction( psNewProv->pgSubject, key, \
           psNewProv->pwszDLLFileName, psNewProv->field); \
    if (r != ERROR_SUCCESS) goto end_function

    CRYPT_SIPADDPROV( szPutSigned, pwszPutFuncName );
    CRYPT_SIPADDPROV( szGetSigned, pwszGetFuncName );
    CRYPT_SIPADDPROV( szRemoveSigned, pwszRemoveFuncName );
    CRYPT_SIPADDPROV( szCreate, pwszCreateFuncName );
    CRYPT_SIPADDPROV( szVerify, pwszVerifyFuncName );
    CRYPT_SIPADDPROV( szIsMyFile, pwszIsFunctionNameFmt2 );

#undef CRYPT_SIPADDPROV

end_function:

    if (r != ERROR_SUCCESS)
    {
        SetLastError(r);
        return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *             CryptSIPRetrieveSubjectGuid (CRYPT32.@)
 *
 * Determine the right SIP GUID for the given file.
 *
 * PARAMS
 *  FileName   [I] Filename.
 *  hFileIn    [I] Optional handle to the file.
 *  pgSubject  [O] The SIP's GUID.
 *
 * RETURNS
 *  Success: TRUE. pgSubject contains the SIP GUID.
 *  Failure: FALSE. (Look at GetLastError()).
 *
 */
BOOL WINAPI CryptSIPRetrieveSubjectGuid
      (LPCWSTR FileName, HANDLE hFileIn, GUID *pgSubject)
{
    FIXME("(%s %p %p) stub!\n", wine_dbgstr_w(FileName), hFileIn, pgSubject);
    return FALSE;
}

/***********************************************************************
 *             CryptSIPLoad (CRYPT32.@)
 *
 * Load the functions for the given SIP.
 *
 * PARAMS
 *  pgSubject    [I] The GUID.
 *  dwFlags      [I] Flags.
 *  pSipDispatch [I] The loaded functions.
 *
 * RETURNS
 *  Success: TRUE. pSipDispatch contains the functions.
 *  Failure: FALSE. (Look at GetLastError()).
 *
 * NOTES
 *  Testing shows that (somehow) the table of functions is cached between
 *  calls.
 *
 */
BOOL WINAPI CryptSIPLoad
       (const GUID *pgSubject, DWORD dwFlags, SIP_DISPATCH_INFO *pSipDispatch)
{
    FIXME("(%s %ld %p) stub!\n", debugstr_guid(pgSubject), dwFlags, pSipDispatch);
    return FALSE;
}

LPVOID WINAPI CryptMemAlloc(ULONG cbSize)
{
    return HeapAlloc(GetProcessHeap(), 0, cbSize);
}

LPVOID WINAPI CryptMemRealloc(LPVOID pv, ULONG cbSize)
{
    return HeapReAlloc(GetProcessHeap(), 0, pv, cbSize);
}

VOID WINAPI CryptMemFree(LPVOID pv)
{
    HeapFree(GetProcessHeap(), 0, pv);
}

DWORD WINAPI I_CryptAllocTls(void)
{
    return TlsAlloc();
}

LPVOID WINAPI I_CryptDetachTls(DWORD dwTlsIndex)
{
    LPVOID ret;

    ret = TlsGetValue(dwTlsIndex);
    TlsSetValue(dwTlsIndex, NULL);
    return ret;
}

LPVOID WINAPI I_CryptGetTls(DWORD dwTlsIndex)
{
    return TlsGetValue(dwTlsIndex);
}

BOOL WINAPI I_CryptSetTls(DWORD dwTlsIndex, LPVOID lpTlsValue)
{
    return TlsSetValue(dwTlsIndex, lpTlsValue);
}

BOOL WINAPI I_CryptFreeTls(DWORD dwTlsIndex, DWORD unknown)
{
    TRACE("(%ld, %ld)\n", dwTlsIndex, unknown);
    return TlsFree(dwTlsIndex);
}

BOOL WINAPI I_CryptGetOssGlobal(DWORD x)
{
    FIXME("%08lx\n", x);
    return FALSE;
}

HCRYPTPROV WINAPI I_CryptGetDefaultCryptProv(DWORD reserved)
{
    HCRYPTPROV ret;

    TRACE("(%08lx)\n", reserved);

    if (reserved)
    {
        SetLastError(E_INVALIDARG);
        return (HCRYPTPROV)0;
    }
    ret = CRYPT_GetDefaultProvider();
    CryptContextAddRef(ret, NULL, 0);
    return ret;
}

BOOL WINAPI I_CryptReadTrustedPublisherDWORDValueFromRegistry(LPCWSTR name,
 DWORD *value)
{
    static const WCHAR safer[] = { 
     'S','o','f','t','w','a','r','e','\\','P','o','l','i','c','i','e','s','\\',
     'M','i','c','r','o','s','o','f','t','\\','S','y','s','t','e','m',
     'C','e','r','t','i','f','i','c','a','t','e','s','\\',
     'T','r','u','s','t','e','d','P','u','b','l','i','s','h','e','r','\\',
     'S','a','f','e','r',0 };
    HKEY key;
    LONG rc;
    BOOL ret = FALSE;

    TRACE("(%s, %p)\n", debugstr_w(name), value);

    *value = 0;
    rc = RegCreateKeyW(HKEY_LOCAL_MACHINE, safer, &key);
    if (rc == ERROR_SUCCESS)
    {
        DWORD size = sizeof(DWORD);

        if (!RegQueryValueExW(key, name, NULL, NULL, (LPBYTE)value, &size))
            ret = TRUE;
        RegCloseKey(key);
    }
    return ret;
}

BOOL WINAPI I_CryptInstallOssGlobal(DWORD x, DWORD y, DWORD z)
{
    FIXME("%08lx %08lx %08lx\n", x, y, z);
    return FALSE;
}

BOOL WINAPI I_CryptInstallAsn1Module(void *x, DWORD y, DWORD z)
{
    FIXME("%p %08lx %08lx\n", x, y, z);
    return TRUE;
}

BOOL WINAPI I_CryptUninstallAsn1Module(void *x)
{
    FIXME("%p\n", x);
    return TRUE;
}

BOOL WINAPI CryptFormatObject(DWORD dwCertEncodingType, DWORD dwFormatType,
 DWORD dwFormatStrType, void *pFormatStruct, LPCSTR lpszStructType,
 const BYTE *pbEncoded, DWORD cbEncoded, void *pbFormat, DWORD *pcbFormat)
{
    FIXME("(%08lx, %ld, %ld, %p, %s, %p, %ld, %p, %p): stub\n",
     dwCertEncodingType, dwFormatType, dwFormatStrType, pFormatStruct,
     debugstr_a(lpszStructType), pbEncoded, cbEncoded, pbFormat, pcbFormat);
    return FALSE;
}

BOOL WINAPI CryptQueryObject(DWORD dwObjectType, const void* pvObject,
    DWORD dwExpectedContentTypeFlags, DWORD dwExpectedFormatTypeFlags,
    DWORD dwFlags, DWORD* pdwMsgAndCertEncodingType, DWORD* pdwContentType,
    DWORD* pdwFormatType, HCERTSTORE* phCertStore, HCRYPTMSG* phMsg,
    const void** ppvContext)
{
    FIXME( "%08lx %p %08lx %08lx %08lx %p %p %p %p %p %p", dwObjectType,
           pvObject, dwExpectedContentTypeFlags, dwExpectedFormatTypeFlags,
           dwFlags, pdwMsgAndCertEncodingType, pdwContentType, pdwFormatType,
           phCertStore, phMsg, ppvContext);
    return FALSE;
}

BOOL WINAPI CryptVerifyMessageSignature(PCRYPT_VERIFY_MESSAGE_PARA pVerifyPara,
          DWORD dwSignerIndex, const BYTE* pbSignedBlob, DWORD cbSignedBlob,
          BYTE* pbDecoded, DWORD* pcbDecoded, PCCERT_CONTEXT* ppSignerCert)
{
    FIXME("stub: %p, %ld, %p, %ld, %p, %p, %p\n",
        pVerifyPara, dwSignerIndex, pbSignedBlob, cbSignedBlob,
        pbDecoded, pcbDecoded, ppSignerCert);
    return FALSE;
}
