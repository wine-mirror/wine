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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdarg.h>
#include <stdio.h>
#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "winreg.h"
#include "snmp.h"
#include "wine/debug.h"

/* a few asn.1 tags we need */
#define ASN_UTCTIME     (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x17)
#define ASN_GENERALTIME (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x18)

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

static const WCHAR szDllName[] = { 'D','l','l',0 };

static char *CRYPT_GetKeyName(DWORD dwEncodingType, LPCSTR pszFuncName,
 LPCSTR pszOID)
{
    static const char szEncodingTypeFmt[] =
     "Software\\Microsoft\\Cryptography\\OID\\EncodingType %ld\\%s\\%s";
    UINT len;
    char numericOID[7]; /* enough for "#65535" */
    const char *oid;
    LPSTR szKey;

    /* MSDN says the encoding type is a mask, but it isn't treated that way.
     * (E.g., if dwEncodingType were 3, the key names "EncodingType 1" and
     * "EncodingType 2" would be expected if it were a mask.  Instead native
     * stores values in "EncodingType 3".
     */
    if (!HIWORD(pszOID))
    {
        snprintf(numericOID, sizeof(numericOID), "#%d", (int)pszOID);
        oid = numericOID;
    }
    else
        oid = pszOID;

    /* This is enough: the lengths of the two string parameters are explicitly
     * counted, and we need up to five additional characters for the encoding
     * type.  These are covered by the "%d", "%s", and "%s" characters in the
     * format specifier that are removed by sprintf.
     */
    len = sizeof(szEncodingTypeFmt) + lstrlenA(pszFuncName) + lstrlenA(oid);
    szKey = HeapAlloc(GetProcessHeap(), 0, len);
    if (szKey)
        sprintf(szKey, szEncodingTypeFmt, dwEncodingType, pszFuncName, oid);
    return szKey;
}

BOOL WINAPI CryptRegisterOIDFunction(DWORD dwEncodingType, LPCSTR pszFuncName,
                  LPCSTR pszOID, LPCWSTR pwszDll, LPCSTR pszOverrideFuncName)
{
    LONG r;
    HKEY hKey;
    LPSTR szKey;

    TRACE("%lx %s %s %s %s\n", dwEncodingType, pszFuncName, pszOID,
          debugstr_w(pwszDll), pszOverrideFuncName);

    /* This only registers functions for encoding certs, not messages */
    if (!GET_CERT_ENCODING_TYPE(dwEncodingType))
        return TRUE;

    /* Native does nothing pwszDll is NULL */
    if (!pwszDll)
        return TRUE;

    /* I'm not matching MS bug for bug here, because I doubt any app depends on
     * it:
     * - native "succeeds" if pszFuncName is NULL, but the nonsensical entry
     *   it creates would never be used
     * - native returns an HRESULT rather than a Win32 error if pszOID is NULL.
     * Instead I disallow both of these with ERROR_INVALID_PARAMETER.
     */
    if (!pszFuncName || !pszOID)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    szKey = CRYPT_GetKeyName(dwEncodingType, pszFuncName, pszOID);
    TRACE("Key name is %s\n", debugstr_a(szKey));

    if (!szKey)
        return FALSE;

    r = RegCreateKeyA(HKEY_LOCAL_MACHINE, szKey, &hKey);
    HeapFree(GetProcessHeap(), 0, szKey);
    if(r != ERROR_SUCCESS)
        return FALSE;

    /* write the values */
    if (pszOverrideFuncName)
        RegSetValueExA(hKey, "FuncName", 0, REG_SZ, pszOverrideFuncName,
         lstrlenA(pszOverrideFuncName) + 1);
    RegSetValueExW(hKey, szDllName, 0, REG_SZ, (const BYTE*) pwszDll,
                    (lstrlenW(pwszDll) + 1) * sizeof (WCHAR));

    RegCloseKey(hKey);
    return TRUE;
}

BOOL WINAPI CryptUnregisterOIDFunction(DWORD dwEncodingType, LPCSTR pszFuncName,
 LPCSTR pszOID)
{
    LPSTR szKey;
    LONG rc;

    TRACE("%lx %s %s\n", dwEncodingType, pszFuncName, pszOID);

    if (!GET_CERT_ENCODING_TYPE(dwEncodingType))
        return TRUE;

    if (!pszFuncName || !pszOID)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    szKey = CRYPT_GetKeyName(dwEncodingType, pszFuncName, pszOID);
    rc = RegDeleteKeyA(HKEY_LOCAL_MACHINE, szKey);
    HeapFree(GetProcessHeap(), 0, szKey);
    if (rc)
        SetLastError(rc);
    return rc ? FALSE : TRUE;
}

BOOL WINAPI CryptGetOIDFunctionValue(DWORD dwEncodingType, LPCSTR pszFuncName,
 LPCSTR pszOID, LPCWSTR pwszValueName, DWORD *pdwValueType, BYTE *pbValueData,
 DWORD *pcbValueData)
{
    LPSTR szKey;
    LONG rc;
    HKEY hKey;

    TRACE("%lx %s %s %s %p %p %p\n", dwEncodingType, debugstr_a(pszFuncName),
     debugstr_a(pszOID), debugstr_w(pwszValueName), pdwValueType, pbValueData,
     pcbValueData);

    if (!GET_CERT_ENCODING_TYPE(dwEncodingType))
        return TRUE;

    if (!pszFuncName || !pszOID || !pwszValueName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    szKey = CRYPT_GetKeyName(dwEncodingType, pszFuncName, pszOID);
    rc = RegOpenKeyA(HKEY_LOCAL_MACHINE, szKey, &hKey);
    HeapFree(GetProcessHeap(), 0, szKey);
    if (rc)
        SetLastError(rc);
    else
    {
        rc = RegQueryValueExW(hKey, pwszValueName, NULL, pdwValueType,
         pbValueData, pcbValueData);
        if (rc)
            SetLastError(rc);
        RegCloseKey(hKey);
    }
    return rc ? FALSE : TRUE;
}

BOOL WINAPI CryptSetOIDFunctionValue(DWORD dwEncodingType, LPCSTR pszFuncName,
 LPCSTR pszOID, LPCWSTR pwszValueName, DWORD dwValueType,
 const BYTE *pbValueData, DWORD cbValueData)
{
    LPSTR szKey;
    LONG rc;
    HKEY hKey;

    TRACE("%lx %s %s %s %ld %p %ld\n", dwEncodingType, debugstr_a(pszFuncName),
     debugstr_a(pszOID), debugstr_w(pwszValueName), dwValueType, pbValueData,
     cbValueData);

    if (!GET_CERT_ENCODING_TYPE(dwEncodingType))
        return TRUE;

    if (!pszFuncName || !pszOID || !pwszValueName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    szKey = CRYPT_GetKeyName(dwEncodingType, pszFuncName, pszOID);
    rc = RegOpenKeyA(HKEY_LOCAL_MACHINE, szKey, &hKey);
    HeapFree(GetProcessHeap(), 0, szKey);
    if (rc)
        SetLastError(rc);
    else
    {
        rc = RegSetValueExW(hKey, pwszValueName, 0, dwValueType, pbValueData,
         cbValueData);
        if (rc)
            SetLastError(rc);
        RegCloseKey(hKey);
    }
    return rc ? FALSE : TRUE;
}

/* Gets the registered function named szFuncName for dwCertEncodingType and
 * lpszStructType, or NULL if one could not be found.  *lib will be set to the
 * handle of the module it's in, or NULL if no module was loaded.  If the
 * return value is NULL, *lib will also be NULL, to simplify error handling.
 */
static void *CRYPT_GetFunc(DWORD dwCertEncodingType, LPCSTR lpszStructType,
 LPCSTR szFuncName, HMODULE *lib)
{
    void *ret = NULL;
    char *szKey = CRYPT_GetKeyName(dwCertEncodingType, szFuncName,
     lpszStructType);
    const char *funcName;
    long r;
    HKEY hKey;
    DWORD type, size = 0;

    *lib = NULL;
    r = RegOpenKeyA(HKEY_LOCAL_MACHINE, szKey, &hKey);
    HeapFree(GetProcessHeap(), 0, szKey);
    if(r != ERROR_SUCCESS)
        return NULL;

    RegQueryValueExA(hKey, "FuncName", NULL, &type, NULL, &size);
    if (GetLastError() == ERROR_MORE_DATA && type == REG_SZ)
    {
        funcName = HeapAlloc(GetProcessHeap(), 0, size);
        RegQueryValueExA(hKey, "FuncName", NULL, &type, (LPBYTE)funcName,
         &size);
    }
    else
        funcName = szFuncName;
    RegQueryValueExW(hKey, szDllName, NULL, &type, NULL, &size);
    if (GetLastError() == ERROR_MORE_DATA && type == REG_SZ)
    {
        LPWSTR dllName = HeapAlloc(GetProcessHeap(), 0, size);

        RegQueryValueExW(hKey, szDllName, NULL, &type, (LPBYTE)dllName,
         &size);
        *lib = LoadLibraryW(dllName);
        if (*lib)
        {
             ret = GetProcAddress(*lib, funcName);
             if (!ret)
             {
                 /* Unload the library, the caller doesn't want to unload it
                  * when the return value is NULL.
                  */
                 FreeLibrary(*lib);
                 *lib = NULL;
             }
        }
        HeapFree(GetProcessHeap(), 0, dllName);
    }
    if (funcName != szFuncName)
        HeapFree(GetProcessHeap(), 0, (char *)funcName);
    return ret;
}

typedef BOOL (WINAPI *CryptEncodeObjectFunc)(DWORD, LPCSTR, const void *,
 BYTE *, DWORD *);

BOOL WINAPI CryptEncodeObject(DWORD dwCertEncodingType, LPCSTR lpszStructType,
 const void *pvStructInfo, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret = FALSE;
    HMODULE lib;
    CryptEncodeObjectFunc pCryptEncodeObject;

    TRACE("(0x%08lx, %s, %p, %p, %p)\n",
     dwCertEncodingType, HIWORD(lpszStructType) ? debugstr_a(lpszStructType) :
     "(integer value)", pvStructInfo, pbEncoded, pcbEncoded);

    if (!pbEncoded && !pcbEncoded)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Try registered DLL first.. */
    pCryptEncodeObject =
     (CryptEncodeObjectFunc)CRYPT_GetFunc(dwCertEncodingType,
     lpszStructType, "CryptEncodeObject", &lib);
    if (pCryptEncodeObject)
    {
        ret = pCryptEncodeObject(dwCertEncodingType, lpszStructType,
         pvStructInfo, pbEncoded, pcbEncoded);
        FreeLibrary(lib);
    }
    else
    {
        /* If not, use CryptEncodeObjectEx */
        ret = CryptEncodeObjectEx(dwCertEncodingType, lpszStructType,
         pvStructInfo, 0, NULL, pbEncoded, pcbEncoded);
    }
    return ret;
}

/* Helper function to check *pcbEncoded, set it to the required size, and
 * optionally to allocate memory.  Assumes pbEncoded is not NULL.
 * If CRYPT_ENCODE_ALLOC_FLAG is set in dwFlags, *pbEncoded will be set to a
 * pointer to the newly allocated memory.
 */
static BOOL CRYPT_EncodeEnsureSpace(DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded,
 DWORD bytesNeeded)
{
    BOOL ret = TRUE;

    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
    {
        if (pEncodePara && pEncodePara->pfnAlloc)
            *(BYTE **)pbEncoded = pEncodePara->pfnAlloc(bytesNeeded);
        else
            *(BYTE **)pbEncoded = LocalAlloc(0, bytesNeeded);
        if (!*(BYTE **)pbEncoded)
            ret = FALSE;
    }
    else if (bytesNeeded > *pcbEncoded)
    {
        *pcbEncoded = bytesNeeded;
        SetLastError(ERROR_MORE_DATA);
        ret = FALSE;
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeInt(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    INT val, i;
    BYTE significantBytes, padByte = 0, bytesNeeded;
    BOOL neg = FALSE, pad = FALSE;

    if (!pvStructInfo)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    memcpy(&val, pvStructInfo, sizeof(val));
    /* Count the number of significant bytes.  Temporarily swap sign for
     * negatives so I count the minimum number of bytes.
     */
    if (val < 0)
    {
        neg = TRUE;
        val = -val;
    }
    for (significantBytes = sizeof(val); !(val & 0xff000000);
     val <<= 8, significantBytes--)
        ;
    if (neg)
    {
        val = -val;
        if ((val & 0xff000000) < 0x80000000)
        {
            padByte = 0xff;
            pad = TRUE;
        }
    }
    else if ((val & 0xff000000) > 0x7f000000)
    {
        padByte = 0;
        pad = TRUE;
    }
    bytesNeeded = 2 + significantBytes;
    if (pad)
        bytesNeeded++;
    if (!pbEncoded)
    {
        *pcbEncoded = bytesNeeded;
        return TRUE;
    }
    if (!CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded, pcbEncoded,
     bytesNeeded))
        return FALSE;
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        pbEncoded = *(BYTE **)pbEncoded;
    *pbEncoded++ = ASN_INTEGER;
    if (pad)
    {
        *pbEncoded++ = significantBytes + 1;
        *pbEncoded++ = padByte;
    }
    else
        *pbEncoded++ = significantBytes;
    for (i = 0; i < significantBytes; i++, val <<= 8)
        *(pbEncoded + i) = (BYTE)((val & 0xff000000) >> 24);
    return TRUE;
}

static BOOL WINAPI CRYPT_AsnEncodeUtcTime(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    SYSTEMTIME sysTime;
    /* sorry, magic number: enough for tag, len, YYMMDDHHMMSSZ\0.  I use a
     * temporary buffer because the output buffer is not NULL-terminated.
     */
    char buf[16];
    static const DWORD bytesNeeded = sizeof(buf) - 1;

    if (!pvStructInfo)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    /* Sanity check the year, this is a two-digit year format */
    if (!FileTimeToSystemTime((const FILETIME *)pvStructInfo, &sysTime))
        return FALSE;
    if (sysTime.wYear < 1950 || sysTime.wYear > 2050)
    {
        SetLastError(CRYPT_E_BAD_ENCODE);
        return FALSE;
    }
    if (!pbEncoded)
    {
        *pcbEncoded = bytesNeeded;
        return TRUE;
    }
    if (!CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded, pcbEncoded,
     bytesNeeded))
        return FALSE;
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        pbEncoded = *(BYTE **)pbEncoded;
    buf[0] = ASN_UTCTIME;
    buf[1] = bytesNeeded - 2;
    snprintf(buf + 2, sizeof(buf) - 2, "%02d%02d%02d%02d%02d%02dZ",
     sysTime.wYear >= 2000 ? sysTime.wYear - 2000 : sysTime.wYear - 1900,
     sysTime.wDay, sysTime.wMonth, sysTime.wHour, sysTime.wMinute,
     sysTime.wSecond);
    memcpy(pbEncoded, buf, bytesNeeded);
    return TRUE;
}

static BOOL WINAPI CRYPT_AsnEncodeGeneralizedTime(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    SYSTEMTIME sysTime;
    /* sorry, magic number: enough for tag, len, YYYYMMDDHHMMSSZ\0.  I use a
     * temporary buffer because the output buffer is not NULL-terminated.
     */
    char buf[18];
    static const DWORD bytesNeeded = sizeof(buf) - 1;

    if (!pvStructInfo)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (!pbEncoded)
    {
        *pcbEncoded = bytesNeeded;
        return TRUE;
    }
    if (!FileTimeToSystemTime((const FILETIME *)pvStructInfo, &sysTime))
        return FALSE;
    if (!CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded, pcbEncoded,
     bytesNeeded))
        return FALSE;
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        pbEncoded = *(BYTE **)pbEncoded;
    buf[0] = ASN_GENERALTIME;
    buf[1] = bytesNeeded - 2;
    snprintf(buf + 2, sizeof(buf) - 2, "%04d%02d%02d%02d%02d%02dZ",
     sysTime.wYear, sysTime.wDay, sysTime.wMonth, sysTime.wHour,
     sysTime.wMinute, sysTime.wSecond);
    memcpy(pbEncoded, buf, bytesNeeded);
    return TRUE;
}

static BOOL WINAPI CRYPT_AsnEncodeChoiceOfTime(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    SYSTEMTIME sysTime;
    BOOL ret;

    if (!pvStructInfo)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    /* Check the year, if it's in the UTCTime range call that encode func */
    if (!FileTimeToSystemTime((const FILETIME *)pvStructInfo, &sysTime))
        return FALSE;
    if (sysTime.wYear >= 1950 && sysTime.wYear <= 2050)
        ret = CRYPT_AsnEncodeUtcTime(dwCertEncodingType, lpszStructType,
         pvStructInfo, dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    else
        ret = CRYPT_AsnEncodeGeneralizedTime(dwCertEncodingType,
         lpszStructType, pvStructInfo, dwFlags, pEncodePara, pbEncoded,
         pcbEncoded);
    return ret;
}

typedef BOOL (WINAPI *CryptEncodeObjectExFunc)(DWORD, LPCSTR, const void *,
 DWORD, PCRYPT_ENCODE_PARA, BYTE *, DWORD *);

BOOL WINAPI CryptEncodeObjectEx(DWORD dwCertEncodingType, LPCSTR lpszStructType,
 const void *pvStructInfo, DWORD dwFlags, PCRYPT_ENCODE_PARA pEncodePara,
 BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret = FALSE;
    HMODULE lib = NULL;
    CryptEncodeObjectExFunc encodeFunc = NULL;

    TRACE("(0x%08lx, %s, %p, 0x%08lx, %p, %p, %p): semi-stub\n",
     dwCertEncodingType, HIWORD(lpszStructType) ? debugstr_a(lpszStructType) :
     "(integer value)", pvStructInfo, dwFlags, pEncodePara, pbEncoded,
     pcbEncoded);

    if (!pbEncoded && !pcbEncoded)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if ((dwCertEncodingType & CERT_ENCODING_TYPE_MASK) != X509_ASN_ENCODING
     && (dwCertEncodingType & CMSG_ENCODING_TYPE_MASK) != PKCS_7_ASN_ENCODING)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    SetLastError(NOERROR);
    if (!HIWORD(lpszStructType))
    {
        switch (LOWORD(lpszStructType))
        {
        case (WORD)X509_INTEGER:
            encodeFunc = CRYPT_AsnEncodeInt;
            break;
        case (WORD)X509_CHOICE_OF_TIME:
            encodeFunc = CRYPT_AsnEncodeChoiceOfTime;
            break;
        case (WORD)PKCS_UTC_TIME:
            encodeFunc = CRYPT_AsnEncodeUtcTime;
            break;
        default:
            FIXME("%d: unimplemented\n", LOWORD(lpszStructType));
        }
    }
    else if (!strcmp(lpszStructType, szOID_RSA_signingTime))
        encodeFunc = CRYPT_AsnEncodeUtcTime;
    if (!encodeFunc)
        encodeFunc = (CryptEncodeObjectExFunc)CRYPT_GetFunc(dwCertEncodingType,
         lpszStructType, "CryptEncodeObjectEx", &lib);
    if (encodeFunc)
        ret = encodeFunc(dwCertEncodingType, lpszStructType, pvStructInfo,
         dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    else
        SetLastError(ERROR_FILE_NOT_FOUND);
    if (lib)
        FreeLibrary(lib);
    return ret;
}

typedef BOOL (WINAPI *CryptDecodeObjectFunc)(DWORD, LPCSTR, const BYTE *,
 DWORD, DWORD, void *, DWORD *);

BOOL WINAPI CryptDecodeObject(DWORD dwCertEncodingType, LPCSTR lpszStructType,
 const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo,
 DWORD *pcbStructInfo)
{
    BOOL ret = FALSE;
    HMODULE lib;
    CryptDecodeObjectFunc pCryptDecodeObject;

    TRACE("(0x%08lx, %s, %p, %ld, 0x%08lx, %p, %p)\n",
     dwCertEncodingType, HIWORD(lpszStructType) ? debugstr_a(lpszStructType) :
     "(integer value)", pbEncoded, cbEncoded, dwFlags, pvStructInfo,
     pcbStructInfo);

    if (!pvStructInfo && !pcbStructInfo)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Try registered DLL first.. */
    pCryptDecodeObject =
     (CryptDecodeObjectFunc)CRYPT_GetFunc(dwCertEncodingType,
     lpszStructType, "CryptDecodeObject", &lib);
    if (pCryptDecodeObject)
    {
        ret = pCryptDecodeObject(dwCertEncodingType, lpszStructType,
         pbEncoded, cbEncoded, dwFlags, pvStructInfo, pcbStructInfo);
        FreeLibrary(lib);
    }
    else
    {
        /* If not, use CryptDecodeObjectEx */
        ret = CryptDecodeObjectEx(dwCertEncodingType, lpszStructType, pbEncoded,
         cbEncoded, dwFlags, NULL, pvStructInfo, pcbStructInfo);
    }
    return ret;
}

/* Helper function to check *pcbStructInfo, set it to the required size, and
 * optionally to allocate memory.  Assumes pvStructInfo is not NULL.
 * If CRYPT_DECODE_ALLOC_FLAG is set in dwFlags, *pvStructInfo will be set to a
 * pointer to the newly allocated memory.
 */
static BOOL CRYPT_DecodeEnsureSpace(DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD bytesNeeded)
{
    BOOL ret = TRUE;

    if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
    {
        if (pDecodePara && pDecodePara->pfnAlloc)
            *(BYTE **)pvStructInfo = pDecodePara->pfnAlloc(bytesNeeded);
        else
            *(BYTE **)pvStructInfo = LocalAlloc(0, bytesNeeded);
        if (!*(BYTE **)pvStructInfo)
            ret = FALSE;
    }
    else if (*pcbStructInfo < bytesNeeded)
    {
        *pcbStructInfo = bytesNeeded;
        SetLastError(ERROR_MORE_DATA);
        ret = FALSE;
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeInt(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    int val, i;

    if (!pbEncoded || !cbEncoded)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (!pvStructInfo)
    {
        *pcbStructInfo = sizeof(int);
        return TRUE;
    }
    if (pbEncoded[0] != ASN_INTEGER)
    {
        SetLastError(CRYPT_E_ASN1_BADTAG);
        return FALSE;
    }
    if (pbEncoded[1] == 0)
    {
        SetLastError(CRYPT_E_ASN1_CORRUPT);
        return FALSE;
    }
    if (pbEncoded[1] > sizeof(int))
    {
        SetLastError(CRYPT_E_ASN1_LARGE);
        return FALSE;
    }
    if (pbEncoded[2] & 0x80)
    {
        /* initialize to a negative value to sign-extend */
        val = -1;
    }
    else
        val = 0;
    for (i = 0; i < pbEncoded[1]; i++)
    {
        val <<= 8;
        val |= pbEncoded[2 + i];
    }
    if (!CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara, pvStructInfo,
     pcbStructInfo, sizeof(int)))
        return FALSE;
    if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
        pvStructInfo = *(BYTE **)pvStructInfo;
    *pcbStructInfo = sizeof(int);
    memcpy(pvStructInfo, &val, sizeof(int));
    return TRUE;
}

#define CRYPT_TIME_GET_DIGITS(pbEncoded, len, numDigits, word) \
 do { \
    BYTE i; \
 \
    (word) = 0; \
    for (i = 0; (len) > 0 && i < (numDigits); i++, (len)--) \
    { \
        if (!isdigit(*(pbEncoded))) \
        { \
            SetLastError(CRYPT_E_ASN1_CORRUPT); \
            return FALSE; \
        } \
        (word) *= 10; \
        (word) += *(pbEncoded)++ - '0'; \
    } \
 } while (0)

static BOOL WINAPI CRYPT_AsnDecodeUtcTime(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    SYSTEMTIME sysTime = { 0 };
    BYTE len;

    if (!pbEncoded || !cbEncoded)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (!pvStructInfo)
    {
        *pcbStructInfo = sizeof(FILETIME);
        return TRUE;
    }
    if (pbEncoded[0] != ASN_UTCTIME)
    {
        SetLastError(CRYPT_E_ASN1_BADTAG);
        return FALSE;
    }
    len = pbEncoded[1];
    /* FIXME: magic # */
    if (len < 10)
    {
        SetLastError(CRYPT_E_ASN1_CORRUPT);
        return FALSE;
    }
    pbEncoded += 2;
    CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2, sysTime.wYear);
    if (sysTime.wYear >= 50)
        sysTime.wYear += 1900;
    else
        sysTime.wYear += 2000;
    CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2, sysTime.wMonth);
    CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2, sysTime.wDay);
    CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2, sysTime.wHour);
    CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2, sysTime.wMinute);
    if (len > 0)
    {
        if (len >= 2 && isdigit(*pbEncoded) && isdigit(*(pbEncoded + 1)))
            CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2, sysTime.wSecond);
        else if (isdigit(*pbEncoded))
            CRYPT_TIME_GET_DIGITS(pbEncoded, len, 1, sysTime.wSecond);
        if (len > 0)
        {
            /* FIXME: get timezone, for now assuming UTC (no adjustment) */
        }
    }

    if (!CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara, pvStructInfo,
     pcbStructInfo, sizeof(FILETIME)))
        return FALSE;
    if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
        pvStructInfo = *(BYTE **)pvStructInfo;
    *pcbStructInfo = sizeof(FILETIME);
    return SystemTimeToFileTime(&sysTime, (FILETIME *)pvStructInfo);
}

static BOOL WINAPI CRYPT_AsnDecodeGeneralizedTime(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    SYSTEMTIME sysTime = { 0 };
    BYTE len;

    if (!pbEncoded || !cbEncoded)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (!pvStructInfo)
    {
        *pcbStructInfo = sizeof(FILETIME);
        return TRUE;
    }
    if (pbEncoded[0] != ASN_GENERALTIME)
    {
        SetLastError(CRYPT_E_ASN1_BADTAG);
        return FALSE;
    }
    len = pbEncoded[1];
    /* FIXME: magic # */
    if (len < 10)
    {
        SetLastError(CRYPT_E_ASN1_CORRUPT);
        return FALSE;
    }
    pbEncoded += 2;
    CRYPT_TIME_GET_DIGITS(pbEncoded, len, 4, sysTime.wYear);
    CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2, sysTime.wMonth);
    CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2, sysTime.wDay);
    CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2, sysTime.wHour);
    if (len > 0)
    {
        CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2, sysTime.wMinute);
        if (len > 0)
            CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2, sysTime.wSecond);
        if (len > 0 && (*pbEncoded == '.' || *pbEncoded == ','))
        {
            BYTE digits;

            pbEncoded++;
            len--;
            digits = min(len, 3); /* workaround macro weirdness */
            CRYPT_TIME_GET_DIGITS(pbEncoded, len, digits,
             sysTime.wMilliseconds);
        }
        if (len >= 5 && (*pbEncoded == '+' || *pbEncoded == '-'))
        {
            WORD hours, minutes;
            BYTE sign = *pbEncoded++;

            len--;
            CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2, hours);
            if (hours >= 24)
                return CRYPT_E_ASN1_CORRUPT;
            CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2, minutes);
            if (minutes >= 60)
                return CRYPT_E_ASN1_CORRUPT;
            if (sign == '+')
            {
                sysTime.wHour += hours;
                sysTime.wMinute += minutes;
            }
            else
            {
                if (hours > sysTime.wHour)
                {
                    sysTime.wDay--;
                    sysTime.wHour = 24 - (hours - sysTime.wHour);
                }
                else
                    sysTime.wHour -= hours;
                if (minutes > sysTime.wMinute)
                {
                    sysTime.wHour--;
                    sysTime.wMinute = 60 - (minutes - sysTime.wMinute);
                }
                else
                    sysTime.wMinute -= minutes;
            }
        }
    }

    if (!CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara, pvStructInfo,
     pcbStructInfo, sizeof(FILETIME)))
        return FALSE;
    if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
        pvStructInfo = *(BYTE **)pvStructInfo;
    *pcbStructInfo = sizeof(FILETIME);
    return SystemTimeToFileTime(&sysTime, (FILETIME *)pvStructInfo);
}

static BOOL WINAPI CRYPT_AsnDecodeChoiceOfTime(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    if (!pbEncoded || !cbEncoded)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (!pvStructInfo)
    {
        *pcbStructInfo = sizeof(FILETIME);
        return TRUE;
    }

    if (pbEncoded[0] == ASN_UTCTIME)
        ret = CRYPT_AsnDecodeUtcTime(dwCertEncodingType, lpszStructType,
         pbEncoded, cbEncoded, dwFlags, pDecodePara, pvStructInfo,
         pcbStructInfo);
    else if (pbEncoded[0] == ASN_GENERALTIME)
        ret = CRYPT_AsnDecodeGeneralizedTime(dwCertEncodingType,
         lpszStructType, pbEncoded, cbEncoded, dwFlags, pDecodePara,
         pvStructInfo, pcbStructInfo);
    else
    {
        SetLastError(CRYPT_E_ASN1_BADTAG);
        ret = FALSE;
    }
    return ret;
}

typedef BOOL (WINAPI *CryptDecodeObjectExFunc)(DWORD, LPCSTR, const BYTE *,
 DWORD, DWORD, PCRYPT_DECODE_PARA, void *, DWORD *);

BOOL WINAPI CryptDecodeObjectEx(DWORD dwCertEncodingType, LPCSTR lpszStructType,
 const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = FALSE;
    HMODULE lib = NULL;
    CryptDecodeObjectExFunc decodeFunc = NULL;

    TRACE("(0x%08lx, %s, %p, %ld, 0x%08lx, %p, %p, %p): semi-stub\n",
     dwCertEncodingType, HIWORD(lpszStructType) ? debugstr_a(lpszStructType) :
     "(integer value)", pbEncoded, cbEncoded, dwFlags, pDecodePara,
     pvStructInfo, pcbStructInfo);

    if (!pvStructInfo && !pcbStructInfo)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if ((dwCertEncodingType & CERT_ENCODING_TYPE_MASK) != X509_ASN_ENCODING
     && (dwCertEncodingType & CMSG_ENCODING_TYPE_MASK) != PKCS_7_ASN_ENCODING)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    SetLastError(NOERROR);
    if (!HIWORD(lpszStructType))
    {
        switch (LOWORD(lpszStructType))
        {
        case (WORD)X509_INTEGER:
            decodeFunc = CRYPT_AsnDecodeInt;
            break;
        case (WORD)X509_CHOICE_OF_TIME:
            decodeFunc = CRYPT_AsnDecodeChoiceOfTime;
            break;
        case (WORD)PKCS_UTC_TIME:
            decodeFunc = CRYPT_AsnDecodeUtcTime;
            break;
        default:
            FIXME("%d: unimplemented\n", LOWORD(lpszStructType));
        }
    }
    else if (!strcmp(lpszStructType, szOID_RSA_signingTime))
        decodeFunc = CRYPT_AsnDecodeUtcTime;
    if (!decodeFunc)
        decodeFunc = (CryptDecodeObjectExFunc)CRYPT_GetFunc(dwCertEncodingType,
         lpszStructType, "CryptDecodeObjectEx", &lib);
    if (decodeFunc)
        ret = decodeFunc(dwCertEncodingType, lpszStructType, pbEncoded,
         cbEncoded, dwFlags, pDecodePara, pvStructInfo, pcbStructInfo);
    else
        SetLastError(ERROR_FILE_NOT_FOUND);
    if (lib)
        FreeLibrary(lib);
    return ret;
}
