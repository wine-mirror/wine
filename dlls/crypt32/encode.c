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
 *
 * This file implements ASN.1 DER encoding and decoding of a limited set of
 * types.  It isn't a full ASN.1 implementation.  Microsoft implements BER
 * encoding of many of the basic types in msasn1.dll, but that interface is
 * undocumented, so I implement them here.
 *
 * References:
 * "A Layman's Guide to a Subset of ASN.1, BER, and DER", by Burton Kaliski
 * (available online, look for a PDF copy as the HTML versions tend to have
 * translation errors.)
 *
 * RFC3280, http://www.faqs.org/rfcs/rfc3280.html
 *
 * MSDN, especially:
 * http://msdn.microsoft.com/library/en-us/seccrypto/security/constants_for_cryptencodeobject_and_cryptdecodeobject.asp
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include "excpt.h"
#include "wincrypt.h"
#include "winreg.h"
#include "snmp.h"
#include "wine/debug.h"
#include "wine/exception.h"

/* This is a bit arbitrary, but to set some limit: */
#define MAX_ENCODED_LEN 0x02000000

/* a few asn.1 tags we need */
#define ASN_BOOL            (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x01)
#define ASN_BITSTRING       (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x03)
#define ASN_OCTETSTRING     (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x04)
#define ASN_ENUMERATED      (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x0a)
#define ASN_SETOF           (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x11)
#define ASN_NUMERICSTRING   (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x12)
#define ASN_PRINTABLESTRING (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x13)
#define ASN_IA5STRING       (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x16)
#define ASN_UTCTIME         (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x17)
#define ASN_GENERALTIME     (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x18)

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

static const WCHAR szDllName[] = { 'D','l','l',0 };

static BOOL WINAPI CRYPT_AsnEncodeOid(LPCSTR pszObjId, BYTE *pbEncoded,
 DWORD *pcbEncoded);
static BOOL CRYPT_EncodeBool(BOOL val, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodeOctets(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodeBits(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodeInt(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodeInteger(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnDecodeOid(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, LPSTR pszObjId, DWORD *pcbObjId);
static BOOL WINAPI CRYPT_DecodeBool(const BYTE *pbEncoded, DWORD cbEncoded,
 BOOL *val);
static BOOL WINAPI CRYPT_AsnDecodeOctets(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo);
static BOOL WINAPI CRYPT_AsnDecodeBits(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo);
static BOOL WINAPI CRYPT_AsnDecodeInt(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo);
static BOOL WINAPI CRYPT_AsnDecodeInteger(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo);

/* filter for page-fault exceptions */
static WINE_EXCEPTION_FILTER(page_fault)
{
    if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}

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

    TRACE("(%08lx %s %s %p)\n", dwCertEncodingType, debugstr_a(lpszStructType),
     debugstr_a(szFuncName), lib);

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
    TRACE("returning %p\n", ret);
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
        else
            *pcbEncoded = bytesNeeded;
    }
    else if (bytesNeeded > *pcbEncoded)
    {
        *pcbEncoded = bytesNeeded;
        SetLastError(ERROR_MORE_DATA);
        ret = FALSE;
    }
    return ret;
}

static BOOL CRYPT_EncodeLen(DWORD len, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    DWORD bytesNeeded, significantBytes = 0;

    if (len <= 0x7f)
        bytesNeeded = 1;
    else
    {
        DWORD temp;

        for (temp = len, significantBytes = sizeof(temp); !(temp & 0xff000000);
         temp <<= 8, significantBytes--)
            ;
        bytesNeeded = significantBytes + 1;
    }
    if (!pbEncoded)
    {
        *pcbEncoded = bytesNeeded;
        return TRUE;
    }
    if (*pcbEncoded < bytesNeeded)
    {
        SetLastError(ERROR_MORE_DATA);
        return FALSE;
    }
    if (len <= 0x7f)
        *pbEncoded = (BYTE)len;
    else
    {
        DWORD i;

        *pbEncoded++ = significantBytes | 0x80;
        for (i = 0; i < significantBytes; i++)
        {
            *(pbEncoded + significantBytes - i - 1) = (BYTE)(len & 0xff);
            len >>= 8;
        }
    }
    *pcbEncoded = bytesNeeded;
    return TRUE;
}

static BOOL CRYPT_AsnEncodeExtension(CERT_EXTENSION *ext, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    BOOL ret;
    DWORD dataLen, octetsLen, lenBytes, size;

    ret = CRYPT_AsnEncodeOid(ext->pszObjId, NULL, &size);
    if (ret)
    {
        dataLen = size;
        if (ext->fCritical)
        {
            ret = CRYPT_EncodeBool(TRUE, NULL, &size);
            dataLen += size;
        }
        if (ret)
        {
            ret = CRYPT_AsnEncodeOctets(X509_ASN_ENCODING, X509_OCTET_STRING,
             &ext->Value, 0, NULL, NULL, &octetsLen);
            dataLen += octetsLen;
        }
        CRYPT_EncodeLen(dataLen, NULL, &lenBytes);
        *pcbEncoded = 1 + lenBytes + dataLen;
    }
    if (ret && pbEncoded)
    {
        *pbEncoded++ = ASN_SEQUENCE;
        CRYPT_EncodeLen(dataLen, pbEncoded, &lenBytes);
        pbEncoded += lenBytes;
        ret = CRYPT_AsnEncodeOid(ext->pszObjId, pbEncoded, &size);
        if (ret)
        {
            pbEncoded += size;
            if (ext->fCritical)
            {
                ret = CRYPT_EncodeBool(TRUE, pbEncoded, &size);
                pbEncoded += size;
            }
            if (ret)
            {
                ret = CRYPT_AsnEncodeOctets(X509_ASN_ENCODING,
                 X509_OCTET_STRING, &ext->Value, 0, NULL, pbEncoded,
                 &octetsLen);
                pbEncoded += octetsLen;
            }
        }
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeExtensions(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        DWORD bytesNeeded, dataLen, lenBytes, i;
        const CERT_EXTENSIONS *exts = (const CERT_EXTENSIONS *)pvStructInfo;

        ret = TRUE;
        for (i = 0, dataLen = 0; ret && i < exts->cExtension; i++)
        {
            DWORD size;

            ret = CRYPT_AsnEncodeExtension(&exts->rgExtension[i], NULL, &size);
            if (ret)
                dataLen += size;
        }
        CRYPT_EncodeLen(dataLen, NULL, &lenBytes);
        bytesNeeded = 1 + lenBytes + dataLen;
        if (!pbEncoded)
            *pcbEncoded = bytesNeeded;
        else
        {
            if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
             pcbEncoded, bytesNeeded)))
            {
                if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                    pbEncoded = *(BYTE **)pbEncoded;
                *pbEncoded++ = ASN_SEQUENCEOF;
                CRYPT_EncodeLen(dataLen, pbEncoded, &lenBytes);
                pbEncoded += lenBytes;
                for (i = 0; i < exts->cExtension; i++)
                {
                    DWORD size;

                    ret = CRYPT_AsnEncodeExtension(&exts->rgExtension[i],
                     pbEncoded, &size);
                    if (ret)
                        pbEncoded += size;
                }
            }
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeOid(LPCSTR pszObjId, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    DWORD bytesNeeded = 0, lenBytes;
    BOOL ret = TRUE;
    int firstPos = 0;
    BYTE firstByte = 0;

    if (pszObjId)
    {
        const char *ptr;
        int val1, val2;

        if (sscanf(pszObjId, "%d.%d.%n", &val1, &val2, &firstPos) != 2)
        {
            SetLastError(CRYPT_E_ASN1_ERROR);
            return FALSE;
        }
        bytesNeeded++;
        firstByte = val1 * 40 + val2;
        ptr = pszObjId + firstPos;
        while (ret && *ptr)
        {
            int pos;

            /* note I assume each component is at most 32-bits long in base 2 */
            if (sscanf(ptr, "%d%n", &val1, &pos) == 1)
            {
                if (val1 >= 0x10000000)
                    bytesNeeded += 5;
                else if (val1 >= 0x200000)
                    bytesNeeded += 4;
                else if (val1 >= 0x4000)
                    bytesNeeded += 3;
                else if (val1 >= 0x80)
                    bytesNeeded += 2;
                else
                    bytesNeeded += 1;
                ptr += pos;
                if (*ptr == '.')
                    ptr++;
            }
            else
            {
                SetLastError(CRYPT_E_ASN1_ERROR);
                return FALSE;
            }
        }
        CRYPT_EncodeLen(bytesNeeded, NULL, &lenBytes);
    }
    else
        lenBytes = 1;
    bytesNeeded += 1 + lenBytes;
    if (pbEncoded)
    {
        if (*pbEncoded < bytesNeeded)
        {
            SetLastError(ERROR_MORE_DATA);
            ret = FALSE;
        }
        else
        {
            *pbEncoded++ = ASN_OBJECTIDENTIFIER;
            CRYPT_EncodeLen(bytesNeeded - 1 - lenBytes, pbEncoded, &lenBytes);
            pbEncoded += lenBytes;
            if (pszObjId)
            {
                const char *ptr;
                int val, pos;

                *pbEncoded++ = firstByte;
                ptr = pszObjId + firstPos;
                while (ret && *ptr)
                {
                    sscanf(ptr, "%d%n", &val, &pos);
                    {
                        unsigned char outBytes[5];
                        int numBytes, i;

                        if (val >= 0x10000000)
                            numBytes = 5;
                        else if (val >= 0x200000)
                            numBytes = 4;
                        else if (val >= 0x4000)
                            numBytes = 3;
                        else if (val >= 0x80)
                            numBytes = 2;
                        else
                            numBytes = 1;
                        for (i = numBytes; i > 0; i--)
                        {
                            outBytes[i - 1] = val & 0x7f;
                            val >>= 7;
                        }
                        for (i = 0; i < numBytes - 1; i++)
                            *pbEncoded++ = outBytes[i] | 0x80;
                        *pbEncoded++ = outBytes[i];
                        ptr += pos;
                        if (*ptr == '.')
                            ptr++;
                    }
                }
            }
        }
    }
    *pcbEncoded = bytesNeeded;
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeNameValue(DWORD dwCertEncodingType,
 CERT_NAME_VALUE *value, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BYTE tag;
    DWORD bytesNeeded, lenBytes, encodedLen;
    BOOL ret = TRUE;

    switch (value->dwValueType)
    {
    case CERT_RDN_NUMERIC_STRING:
        tag = ASN_NUMERICSTRING;
        encodedLen = value->Value.cbData;
        break;
    case CERT_RDN_PRINTABLE_STRING:
        tag = ASN_PRINTABLESTRING;
        encodedLen = value->Value.cbData;
        break;
    case CERT_RDN_IA5_STRING:
        tag = ASN_IA5STRING;
        encodedLen = value->Value.cbData;
        break;
    case CERT_RDN_ANY_TYPE:
        /* explicitly disallowed */
        SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
        return FALSE;
    default:
        FIXME("String type %ld unimplemented\n", value->dwValueType);
        return FALSE;
    }
    CRYPT_EncodeLen(encodedLen, NULL, &lenBytes);
    bytesNeeded = 1 + lenBytes + encodedLen;
    if (pbEncoded)
    {
        if (*pcbEncoded < bytesNeeded)
        {
            SetLastError(ERROR_MORE_DATA);
            ret = FALSE;
        }
        else
        {
            *pbEncoded++ = tag;
            CRYPT_EncodeLen(encodedLen, pbEncoded, &lenBytes);
            pbEncoded += lenBytes;
            switch (value->dwValueType)
            {
            case CERT_RDN_NUMERIC_STRING:
            case CERT_RDN_PRINTABLE_STRING:
            case CERT_RDN_IA5_STRING:
                memcpy(pbEncoded, value->Value.pbData, value->Value.cbData);
            }
        }
    }
    *pcbEncoded = bytesNeeded;
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeRdnAttr(DWORD dwCertEncodingType,
 CERT_RDN_ATTR *attr, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    DWORD bytesNeeded = 0, lenBytes, size;
    BOOL ret;

    ret = CRYPT_AsnEncodeOid(attr->pszObjId, NULL, &size);
    if (ret)
    {
        bytesNeeded += size;
        /* hack: a CERT_RDN_ATTR is identical to a CERT_NAME_VALUE beginning
         * with dwValueType, so "cast" it to get its encoded size
         */
        ret = CRYPT_AsnEncodeNameValue(dwCertEncodingType,
         (CERT_NAME_VALUE *)&attr->dwValueType, NULL, &size);
        if (ret)
        {
            bytesNeeded += size;
            CRYPT_EncodeLen(bytesNeeded, NULL, &lenBytes);
            bytesNeeded += 1 + lenBytes;
            if (pbEncoded)
            {
                if (*pcbEncoded < bytesNeeded)
                {
                    SetLastError(ERROR_MORE_DATA);
                    ret = FALSE;
                }
                else
                {
                    *pbEncoded++ = ASN_CONSTRUCTOR | ASN_SEQUENCE;
                    CRYPT_EncodeLen(bytesNeeded - lenBytes - 1, pbEncoded,
                     &lenBytes);
                    pbEncoded += lenBytes;
                    size = bytesNeeded - 1 - lenBytes;
                    ret = CRYPT_AsnEncodeOid(attr->pszObjId, pbEncoded, &size);
                    if (ret)
                    {
                        pbEncoded += size;
                        size = bytesNeeded - 1 - lenBytes - size;
                        ret = CRYPT_AsnEncodeNameValue(dwCertEncodingType,
                         (CERT_NAME_VALUE *)&attr->dwValueType, pbEncoded,
                         &size);
                    }
                }
            }
            *pcbEncoded = bytesNeeded;
        }
    }
    return ret;
}

static int BLOBComp(const void *l, const void *r)
{
    CRYPT_DER_BLOB *a = (CRYPT_DER_BLOB *)l, *b = (CRYPT_DER_BLOB *)r;
    int ret;

    if (!(ret = memcmp(a->pbData, b->pbData, min(a->cbData, b->cbData))))
        ret = a->cbData - b->cbData;
    return ret;
}

/* This encodes as a SET OF, which in DER must be lexicographically sorted.
 */
static BOOL WINAPI CRYPT_AsnEncodeRdn(DWORD dwCertEncodingType, CERT_RDN *rdn,
 BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;
    CRYPT_DER_BLOB *blobs = NULL;
   
    __TRY
    {
        DWORD bytesNeeded = 0, lenBytes, i;

        ret = TRUE;
        if (rdn->cRDNAttr)
        {
            blobs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
             rdn->cRDNAttr * sizeof(CRYPT_DER_BLOB));
            if (!blobs)
                ret = FALSE;
        }
        for (i = 0; ret && i < rdn->cRDNAttr; i++)
        {
            ret = CRYPT_AsnEncodeRdnAttr(dwCertEncodingType, &rdn->rgRDNAttr[i],
             NULL, &blobs[i].cbData);
            if (ret)
                bytesNeeded += blobs[i].cbData;
        }
        if (ret)
        {
            CRYPT_EncodeLen(bytesNeeded, NULL, &lenBytes);
            bytesNeeded += 1 + lenBytes;
            if (pbEncoded)
            {
                if (*pcbEncoded < bytesNeeded)
                {
                    SetLastError(ERROR_MORE_DATA);
                    ret = FALSE;
                }
                else
                {
                    for (i = 0; ret && i < rdn->cRDNAttr; i++)
                    {
                        blobs[i].pbData = HeapAlloc(GetProcessHeap(), 0,
                         blobs[i].cbData);
                        if (!blobs[i].pbData)
                            ret = FALSE;
                        else
                            ret = CRYPT_AsnEncodeRdnAttr(dwCertEncodingType,
                             &rdn->rgRDNAttr[i], blobs[i].pbData,
                             &blobs[i].cbData);
                    }
                    if (ret)
                    {
                        qsort(blobs, rdn->cRDNAttr, sizeof(CRYPT_DER_BLOB),
                         BLOBComp);
                        *pbEncoded++ = ASN_CONSTRUCTOR | ASN_SETOF;
                        CRYPT_EncodeLen(bytesNeeded - lenBytes - 1, pbEncoded,
                         &lenBytes);
                        pbEncoded += lenBytes;
                        for (i = 0; ret && i < rdn->cRDNAttr; i++)
                        {
                            memcpy(pbEncoded, blobs[i].pbData, blobs[i].cbData);
                            pbEncoded += blobs[i].cbData;
                        }
                    }
                }
            }
            *pcbEncoded = bytesNeeded;
        }
        if (blobs)
        {
            for (i = 0; i < rdn->cRDNAttr; i++)
                HeapFree(GetProcessHeap(), 0, blobs[i].pbData);
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    HeapFree(GetProcessHeap(), 0, blobs);
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeName(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CERT_NAME_INFO *info = (const CERT_NAME_INFO *)pvStructInfo;
        DWORD bytesNeeded = 0, lenBytes, size, i;

        TRACE("encoding name with %ld RDNs\n", info->cRDN);
        ret = TRUE;
        for (i = 0; ret && i < info->cRDN; i++)
        {
            ret = CRYPT_AsnEncodeRdn(dwCertEncodingType, &info->rgRDN[i], NULL,
             &size);
            if (ret)
                bytesNeeded += size;
        }
        CRYPT_EncodeLen(bytesNeeded, NULL, &lenBytes);
        bytesNeeded += 1 + lenBytes;
        if (ret)
        {
            if (!pbEncoded)
                *pcbEncoded = bytesNeeded;
            else
            {
                if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
                 pbEncoded, pcbEncoded, bytesNeeded)))
                {
                    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                        pbEncoded = *(BYTE **)pbEncoded;
                    *pbEncoded++ = ASN_SEQUENCEOF;
                    CRYPT_EncodeLen(bytesNeeded - lenBytes - 1, pbEncoded,
                     &lenBytes);
                    pbEncoded += lenBytes;
                    for (i = 0; ret && i < info->cRDN; i++)
                    {
                        size = bytesNeeded;
                        ret = CRYPT_AsnEncodeRdn(dwCertEncodingType,
                         &info->rgRDN[i], pbEncoded, &size);
                        if (ret)
                        {
                            pbEncoded += size;
                            bytesNeeded -= size;
                        }
                    }
                }
            }
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL CRYPT_AsnEncodeAlgorithmIdentifier(
 const CRYPT_ALGORITHM_IDENTIFIER *algo, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;
    DWORD bytesNeeded, dataBytes, lenBytes;

    ret = CRYPT_AsnEncodeOid(algo->pszObjId, NULL, &dataBytes);
    if (ret)
    {
        if (algo->Parameters.cbData)
            dataBytes += algo->Parameters.cbData;
        else
            dataBytes += 2; /* enough space for ASN_NULL */
        CRYPT_EncodeLen(dataBytes, NULL, &lenBytes);
        bytesNeeded = 1 + lenBytes + dataBytes;
        if (!pbEncoded)
            *pcbEncoded = bytesNeeded;
        else if (*pcbEncoded < bytesNeeded)
        {
            *pcbEncoded = bytesNeeded;
            SetLastError(ERROR_MORE_DATA);
            ret = FALSE;
        }
        else
        {
            *pcbEncoded = bytesNeeded;
            *pbEncoded++ = ASN_SEQUENCE;
            CRYPT_EncodeLen(dataBytes, pbEncoded, &lenBytes);
            pbEncoded += lenBytes;
            ret = CRYPT_AsnEncodeOid(algo->pszObjId, pbEncoded, &dataBytes);
            if (ret)
            {
                pbEncoded += dataBytes;
                if (algo->Parameters.cbData)
                    memcpy(pbEncoded, algo->Parameters.pbData,
                     algo->Parameters.cbData);
                else
                {
                    *pbEncoded++ = ASN_NULL;
                    *pbEncoded++ = 0;
                }
            }
        }
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodePubKeyInfo(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CERT_PUBLIC_KEY_INFO *info =
         (const CERT_PUBLIC_KEY_INFO *)pvStructInfo;
        DWORD bitBlobLen, lenBytes, dataBytes, bytesNeeded;

        TRACE("Encoding public key with OID %s\n",
         debugstr_a(info->Algorithm.pszObjId));
        ret = CRYPT_AsnEncodeAlgorithmIdentifier(&info->Algorithm, NULL,
         &dataBytes);
        if (ret)
        {
            ret = CRYPT_AsnEncodeBits(dwCertEncodingType, X509_BITS,
             &info->PublicKey, 0, NULL, NULL, &bitBlobLen);
            if (ret)
            {
                dataBytes += bitBlobLen;
                CRYPT_EncodeLen(dataBytes, NULL, &lenBytes);
                bytesNeeded = 1 + lenBytes + dataBytes;
                if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
                 pbEncoded, pcbEncoded, bytesNeeded)))
                {
                    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                        pbEncoded = *(BYTE **)pbEncoded;
                    *pbEncoded++ = ASN_SEQUENCE;
                    CRYPT_EncodeLen(dataBytes, pbEncoded, &lenBytes);
                    pbEncoded += lenBytes;
                    ret = CRYPT_AsnEncodeAlgorithmIdentifier(&info->Algorithm,
                     pbEncoded, &dataBytes);
                    if (ret)
                    {
                        pbEncoded += dataBytes;
                        ret = CRYPT_AsnEncodeBits(dwCertEncodingType, X509_BITS,
                         &info->PublicKey, 0, NULL, pbEncoded, &bitBlobLen);
                    }
                }
            }
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL CRYPT_EncodeBool(BOOL val, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    if (!pbEncoded)
    {
        *pcbEncoded = 3;
        return TRUE;
    }
    if (*pcbEncoded < 3)
    {
        *pcbEncoded = 3;
        SetLastError(ERROR_MORE_DATA);
        return FALSE;
    }
    *pcbEncoded = 3;
    *pbEncoded++ = ASN_BOOL;
    *pbEncoded++ = 1;
    *pbEncoded++ = val ? 0xff : 0;
    return TRUE;
}

static BOOL WINAPI CRYPT_AsnEncodeBasicConstraints2(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CERT_BASIC_CONSTRAINTS2_INFO *info =
         (const CERT_BASIC_CONSTRAINTS2_INFO *)pvStructInfo;
        DWORD bytesNeeded = 0, lenBytes, caLen = 0, pathConstraintLen = 0;

        ret = TRUE;
        if (info->fCA)
        {
            ret = CRYPT_EncodeBool(TRUE, NULL, &caLen);
            if (ret)
                bytesNeeded += caLen;
        }
        if (info->fPathLenConstraint)
        {
            ret = CRYPT_AsnEncodeInt(dwCertEncodingType, X509_INTEGER,
             &info->dwPathLenConstraint, 0, NULL, NULL, &pathConstraintLen);
            if (ret)
                bytesNeeded += pathConstraintLen;
        }
        if (ret)
        {
            CRYPT_EncodeLen(bytesNeeded, NULL, &lenBytes);
            bytesNeeded += 1 + lenBytes;
            if (!pbEncoded)
                *pcbEncoded = bytesNeeded;
            else
            {
                if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
                 pbEncoded, pcbEncoded, bytesNeeded)))
                {
                    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                        pbEncoded = *(BYTE **)pbEncoded;
                    *pbEncoded++ = ASN_SEQUENCE;
                    CRYPT_EncodeLen(bytesNeeded - 1 - lenBytes, pbEncoded,
                     &lenBytes);
                    pbEncoded += lenBytes;
                    if (info->fCA)
                    {
                        ret = CRYPT_EncodeBool(TRUE, pbEncoded, &caLen);
                        if (ret)
                            pbEncoded += caLen;
                    }
                    if (info->fPathLenConstraint)
                    {
                        ret = CRYPT_AsnEncodeInt(dwCertEncodingType,
                         X509_INTEGER, &info->dwPathLenConstraint, 0, NULL,
                         pbEncoded, &pathConstraintLen);
                    }
                }
            }
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeOctets(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CRYPT_DATA_BLOB *blob = (const CRYPT_DATA_BLOB *)pvStructInfo;
        DWORD bytesNeeded, lenBytes;

        CRYPT_EncodeLen(blob->cbData, NULL, &lenBytes);
        bytesNeeded = 1 + lenBytes + blob->cbData;
        if (!pbEncoded)
        {
            *pcbEncoded = bytesNeeded;
            ret = TRUE;
        }
        else
        {
            if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
             pcbEncoded, bytesNeeded)))
            {
                if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                    pbEncoded = *(BYTE **)pbEncoded;
                *pbEncoded++ = ASN_OCTETSTRING;
                CRYPT_EncodeLen(blob->cbData, pbEncoded, &lenBytes);
                pbEncoded += lenBytes;
                if (blob->cbData)
                    memcpy(pbEncoded, blob->pbData, blob->cbData);
            }
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeBits(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CRYPT_BIT_BLOB *blob = (const CRYPT_BIT_BLOB *)pvStructInfo;
        DWORD bytesNeeded, lenBytes, dataBytes;
        BYTE unusedBits;

        /* yep, MS allows cUnusedBits to be >= 8 */
        if (!blob->cUnusedBits)
        {
            dataBytes = blob->cbData;
            unusedBits = 0;
        }
        else if (blob->cbData * 8 > blob->cUnusedBits)
        {
            dataBytes = (blob->cbData * 8 - blob->cUnusedBits) / 8 + 1;
            unusedBits = blob->cUnusedBits >= 8 ? blob->cUnusedBits / 8 :
             blob->cUnusedBits;
        }
        else
        {
            dataBytes = 0;
            unusedBits = 0;
        }
        CRYPT_EncodeLen(dataBytes + 1, NULL, &lenBytes);
        bytesNeeded = 1 + lenBytes + dataBytes + 1;
        if (!pbEncoded)
        {
            *pcbEncoded = bytesNeeded;
            ret = TRUE;
        }
        else
        {
            if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
             pcbEncoded, bytesNeeded)))
            {
                if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                    pbEncoded = *(BYTE **)pbEncoded;
                *pbEncoded++ = ASN_BITSTRING;
                CRYPT_EncodeLen(dataBytes + 1, pbEncoded, &lenBytes);
                pbEncoded += lenBytes;
                *pbEncoded++ = unusedBits;
                if (dataBytes)
                {
                    BYTE mask = 0xff << unusedBits;

                    if (dataBytes > 1)
                    {
                        memcpy(pbEncoded, blob->pbData, dataBytes - 1);
                        pbEncoded += dataBytes - 1;
                    }
                    *pbEncoded = *(blob->pbData + dataBytes - 1) & mask;
                }
            }
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeInt(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    CRYPT_INTEGER_BLOB blob = { sizeof(INT), (BYTE *)pvStructInfo };

    return CRYPT_AsnEncodeInteger(dwCertEncodingType, X509_MULTI_BYTE_INTEGER,
     &blob, dwFlags, pEncodePara, pbEncoded, pcbEncoded);
}

static BOOL WINAPI CRYPT_AsnEncodeInteger(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        DWORD significantBytes, lenBytes;
        BYTE padByte = 0, bytesNeeded;
        BOOL pad = FALSE;
        const CRYPT_INTEGER_BLOB *blob =
         (const CRYPT_INTEGER_BLOB *)pvStructInfo;

        significantBytes = blob->cbData;
        if (significantBytes)
        {
            if (blob->pbData[significantBytes - 1] & 0x80)
            {
                /* negative, lop off leading (little-endian) 0xffs */
                for (; significantBytes > 0 &&
                 blob->pbData[significantBytes - 1] == 0xff; significantBytes--)
                    ;
                if (blob->pbData[significantBytes - 1] < 0x80)
                {
                    padByte = 0xff;
                    pad = TRUE;
                }
            }
            else
            {
                /* positive, lop off leading (little-endian) zeroes */
                for (; significantBytes > 0 &&
                 !blob->pbData[significantBytes - 1]; significantBytes--)
                    ;
                if (significantBytes == 0)
                    significantBytes = 1;
                if (blob->pbData[significantBytes - 1] > 0x7f)
                {
                    padByte = 0;
                    pad = TRUE;
                }
            }
        }
        if (pad)
            CRYPT_EncodeLen(significantBytes + 1, NULL, &lenBytes);
        else
            CRYPT_EncodeLen(significantBytes, NULL, &lenBytes);
        bytesNeeded = 1 + lenBytes + significantBytes;
        if (pad)
            bytesNeeded++;
        if (!pbEncoded)
        {
            *pcbEncoded = bytesNeeded;
            ret = TRUE;
        }
        else
        {
            if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
             pcbEncoded, bytesNeeded)))
            {
                if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                    pbEncoded = *(BYTE **)pbEncoded;
                *pbEncoded++ = ASN_INTEGER;
                if (pad)
                {
                    CRYPT_EncodeLen(significantBytes + 1, pbEncoded, &lenBytes);
                    pbEncoded += lenBytes;
                    *pbEncoded++ = padByte;
                }
                else
                {
                    CRYPT_EncodeLen(significantBytes, pbEncoded, &lenBytes);
                    pbEncoded += lenBytes;
                }
                for (; significantBytes > 0; significantBytes--)
                    *(pbEncoded++) = blob->pbData[significantBytes - 1];
            }
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeUnsignedInteger(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        DWORD significantBytes, lenBytes;
        BYTE bytesNeeded;
        BOOL pad = FALSE;
        const CRYPT_INTEGER_BLOB *blob =
         (const CRYPT_INTEGER_BLOB *)pvStructInfo;

        significantBytes = blob->cbData;
        if (significantBytes)
        {
            /* positive, lop off leading (little-endian) zeroes */
            for (; significantBytes > 0 && !blob->pbData[significantBytes - 1];
             significantBytes--)
                ;
            if (significantBytes == 0)
                significantBytes = 1;
            if (blob->pbData[significantBytes - 1] > 0x7f)
                pad = TRUE;
        }
        if (pad)
            CRYPT_EncodeLen(significantBytes + 1, NULL, &lenBytes);
        else
            CRYPT_EncodeLen(significantBytes, NULL, &lenBytes);
        bytesNeeded = 1 + lenBytes + significantBytes;
        if (pad)
            bytesNeeded++;
        if (!pbEncoded)
        {
            *pcbEncoded = bytesNeeded;
            ret = TRUE;
        }
        else
        {
            if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
             pcbEncoded, bytesNeeded)))
            {
                if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                    pbEncoded = *(BYTE **)pbEncoded;
                *pbEncoded++ = ASN_INTEGER;
                if (pad)
                {
                    CRYPT_EncodeLen(significantBytes + 1, pbEncoded, &lenBytes);
                    pbEncoded += lenBytes;
                    *pbEncoded++ = 0;
                }
                else
                {
                    CRYPT_EncodeLen(significantBytes, pbEncoded, &lenBytes);
                    pbEncoded += lenBytes;
                }
                for (; significantBytes > 0; significantBytes--)
                    *(pbEncoded++) = blob->pbData[significantBytes - 1];
            }
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeEnumerated(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    CRYPT_INTEGER_BLOB blob;
    BOOL ret;

    /* Encode as an unsigned integer, then change the tag to enumerated */
    blob.cbData = sizeof(DWORD);
    blob.pbData = (BYTE *)pvStructInfo;
    ret = CRYPT_AsnEncodeUnsignedInteger(dwCertEncodingType,
     X509_MULTI_BYTE_UINT, &blob, dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    if (ret && pbEncoded)
    {
        if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
            pbEncoded = *(BYTE **)pbEncoded;
        pbEncoded[0] = ASN_ENUMERATED;
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeUtcTime(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        SYSTEMTIME sysTime;
        /* sorry, magic number: enough for tag, len, YYMMDDHHMMSSZ\0.  I use a
         * temporary buffer because the output buffer is not NULL-terminated.
         */
        char buf[16];
        static const DWORD bytesNeeded = sizeof(buf) - 1;

        if (!pbEncoded)
        {
            *pcbEncoded = bytesNeeded;
            ret = TRUE;
        }
        else
        {
            /* Sanity check the year, this is a two-digit year format */
            ret = FileTimeToSystemTime((const FILETIME *)pvStructInfo,
             &sysTime);
            if (ret && (sysTime.wYear < 1950 || sysTime.wYear > 2050))
            {
                SetLastError(CRYPT_E_BAD_ENCODE);
                ret = FALSE;
            }
            if (ret)
            {
                if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
                 pbEncoded, pcbEncoded, bytesNeeded)))
                {
                    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                        pbEncoded = *(BYTE **)pbEncoded;
                    buf[0] = ASN_UTCTIME;
                    buf[1] = bytesNeeded - 2;
                    snprintf(buf + 2, sizeof(buf) - 2,
                     "%02d%02d%02d%02d%02d%02dZ", sysTime.wYear >= 2000 ?
                     sysTime.wYear - 2000 : sysTime.wYear - 1900,
                     sysTime.wDay, sysTime.wMonth, sysTime.wHour,
                     sysTime.wMinute, sysTime.wSecond);
                    memcpy(pbEncoded, buf, bytesNeeded);
                }
            }
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeGeneralizedTime(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        SYSTEMTIME sysTime;
        /* sorry, magic number: enough for tag, len, YYYYMMDDHHMMSSZ\0.  I use a
         * temporary buffer because the output buffer is not NULL-terminated.
         */
        char buf[18];
        static const DWORD bytesNeeded = sizeof(buf) - 1;

        if (!pbEncoded)
        {
            *pcbEncoded = bytesNeeded;
            ret = TRUE;
        }
        else
        {
            ret = FileTimeToSystemTime((const FILETIME *)pvStructInfo,
             &sysTime);
            if (ret)
                ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
                 pcbEncoded, bytesNeeded);
            if (ret)
            {
                if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                    pbEncoded = *(BYTE **)pbEncoded;
                buf[0] = ASN_GENERALTIME;
                buf[1] = bytesNeeded - 2;
                snprintf(buf + 2, sizeof(buf) - 2, "%04d%02d%02d%02d%02d%02dZ",
                 sysTime.wYear, sysTime.wDay, sysTime.wMonth, sysTime.wHour,
                 sysTime.wMinute, sysTime.wSecond);
                memcpy(pbEncoded, buf, bytesNeeded);
            }
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeChoiceOfTime(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        SYSTEMTIME sysTime;

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
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeSequenceOfAny(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        DWORD bytesNeeded, dataLen, lenBytes, i;
        const CRYPT_SEQUENCE_OF_ANY *seq =
         (const CRYPT_SEQUENCE_OF_ANY *)pvStructInfo;

        for (i = 0, dataLen = 0; i < seq->cValue; i++)
            dataLen += seq->rgValue[i].cbData;
        CRYPT_EncodeLen(dataLen, NULL, &lenBytes);
        bytesNeeded = 1 + lenBytes + dataLen;
        if (!pbEncoded)
        {
            *pcbEncoded = bytesNeeded;
            ret = TRUE;
        }
        else
        {
            if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
             pcbEncoded, bytesNeeded)))
            {
                if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                    pbEncoded = *(BYTE **)pbEncoded;
                *pbEncoded++ = ASN_SEQUENCEOF;
                CRYPT_EncodeLen(dataLen, pbEncoded, &lenBytes);
                pbEncoded += lenBytes;
                for (i = 0; i < seq->cValue; i++)
                {
                    memcpy(pbEncoded, seq->rgValue[i].pbData,
                     seq->rgValue[i].cbData);
                    pbEncoded += seq->rgValue[i].cbData;
                }
            }
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

typedef BOOL (WINAPI *CryptEncodeObjectExFunc)(DWORD, LPCSTR, const void *,
 DWORD, PCRYPT_ENCODE_PARA, BYTE *, DWORD *);

BOOL WINAPI CryptEncodeObjectEx(DWORD dwCertEncodingType, LPCSTR lpszStructType,
 const void *pvStructInfo, DWORD dwFlags, PCRYPT_ENCODE_PARA pEncodePara,
 void *pvEncoded, DWORD *pcbEncoded)
{
    BOOL ret = FALSE;
    HMODULE lib = NULL;
    CryptEncodeObjectExFunc encodeFunc = NULL;

    TRACE("(0x%08lx, %s, %p, 0x%08lx, %p, %p, %p)\n",
     dwCertEncodingType, HIWORD(lpszStructType) ? debugstr_a(lpszStructType) :
     "(integer value)", pvStructInfo, dwFlags, pEncodePara, pvEncoded,
     pcbEncoded);

    if (!pvEncoded && !pcbEncoded)
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
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG && pvEncoded)
        *(BYTE **)pvEncoded = NULL;
    if (!HIWORD(lpszStructType))
    {
        switch (LOWORD(lpszStructType))
        {
        case (WORD)X509_EXTENSIONS:
            encodeFunc = CRYPT_AsnEncodeExtensions;
            break;
        case (WORD)X509_NAME:
            encodeFunc = CRYPT_AsnEncodeName;
            break;
        case (WORD)X509_PUBLIC_KEY_INFO:
            encodeFunc = CRYPT_AsnEncodePubKeyInfo;
            break;
        case (WORD)X509_BASIC_CONSTRAINTS2:
            encodeFunc = CRYPT_AsnEncodeBasicConstraints2;
            break;
        case (WORD)X509_OCTET_STRING:
            encodeFunc = CRYPT_AsnEncodeOctets;
            break;
        case (WORD)X509_BITS:
        case (WORD)X509_KEY_USAGE:
            encodeFunc = CRYPT_AsnEncodeBits;
            break;
        case (WORD)X509_INTEGER:
            encodeFunc = CRYPT_AsnEncodeInt;
            break;
        case (WORD)X509_MULTI_BYTE_INTEGER:
            encodeFunc = CRYPT_AsnEncodeInteger;
            break;
        case (WORD)X509_MULTI_BYTE_UINT:
            encodeFunc = CRYPT_AsnEncodeUnsignedInteger;
            break;
        case (WORD)X509_ENUMERATED:
            encodeFunc = CRYPT_AsnEncodeEnumerated;
            break;
        case (WORD)X509_CHOICE_OF_TIME:
            encodeFunc = CRYPT_AsnEncodeChoiceOfTime;
            break;
        case (WORD)X509_SEQUENCE_OF_ANY:
            encodeFunc = CRYPT_AsnEncodeSequenceOfAny;
            break;
        case (WORD)PKCS_UTC_TIME:
            encodeFunc = CRYPT_AsnEncodeUtcTime;
            break;
        default:
            FIXME("%d: unimplemented\n", LOWORD(lpszStructType));
        }
    }
    else if (!strcmp(lpszStructType, szOID_CERT_EXTENSIONS))
        encodeFunc = CRYPT_AsnEncodeExtensions;
    else if (!strcmp(lpszStructType, szOID_RSA_signingTime))
        encodeFunc = CRYPT_AsnEncodeUtcTime;
    else if (!strcmp(lpszStructType, szOID_CRL_REASON_CODE))
        encodeFunc = CRYPT_AsnEncodeEnumerated;
    else if (!strcmp(lpszStructType, szOID_KEY_USAGE))
        encodeFunc = CRYPT_AsnEncodeBits;
    else if (!strcmp(lpszStructType, szOID_SUBJECT_KEY_IDENTIFIER))
        encodeFunc = CRYPT_AsnEncodeOctets;
    else if (!strcmp(lpszStructType, szOID_BASIC_CONSTRAINTS2))
        encodeFunc = CRYPT_AsnEncodeBasicConstraints2;
    else
        TRACE("OID %s not found or unimplemented, looking for DLL\n",
         debugstr_a(lpszStructType));
    if (!encodeFunc)
        encodeFunc = (CryptEncodeObjectExFunc)CRYPT_GetFunc(dwCertEncodingType,
         lpszStructType, "CryptEncodeObjectEx", &lib);
    if (encodeFunc)
        ret = encodeFunc(dwCertEncodingType, lpszStructType, pvStructInfo,
         dwFlags, pEncodePara, pvEncoded, pcbEncoded);
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

/* Gets the number of length bytes from the given (leading) length byte */
#define GET_LEN_BYTES(b) ((b) <= 0x7f ? 1 : 1 + ((b) & 0x7f))

/* Helper function to get the encoded length of the data starting at pbEncoded,
 * where pbEncoded[0] is the tag.  If the data are too short to contain a
 * length or if the length is too large for cbEncoded, sets an appropriate
 * error code and returns FALSE.
 */
static BOOL WINAPI CRYPT_GetLen(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD *len)
{
    BOOL ret;

    if (cbEncoded <= 1)
    {
        SetLastError(CRYPT_E_ASN1_EOD);
        ret = FALSE;
    }
    else if (pbEncoded[1] <= 0x7f)
    {
        *len = pbEncoded[1];
        ret = TRUE;
    }
    else
    {
        BYTE lenLen = GET_LEN_BYTES(pbEncoded[1]);

        if (lenLen > sizeof(DWORD) + 1)
        {
            SetLastError(CRYPT_E_ASN1_LARGE);
            ret = FALSE;
        }
        else if (lenLen + 2 > cbEncoded)
        {
            SetLastError(CRYPT_E_ASN1_CORRUPT);
            ret = FALSE;
        }
        else
        {
            DWORD out = 0;

            pbEncoded += 2;
            while (--lenLen)
            {
                out <<= 8;
                out |= *pbEncoded++;
            }
            if (out + lenLen + 1 > cbEncoded)
            {
                SetLastError(CRYPT_E_ASN1_EOD);
                ret = FALSE;
            }
            else
            {
                *len = out;
                ret = TRUE;
            }
        }
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
        else
            *pcbStructInfo = bytesNeeded;
    }
    else if (*pcbStructInfo < bytesNeeded)
    {
        *pcbStructInfo = bytesNeeded;
        SetLastError(ERROR_MORE_DATA);
        ret = FALSE;
    }
    return ret;
}

/* Warning:  assumes ext->Value.pbData is set ahead of time! */
static BOOL CRYPT_AsnDecodeExtension(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, CERT_EXTENSION *ext, DWORD *pcbExt)
{
    BOOL ret = TRUE;

    if (pbEncoded[0] == ASN_SEQUENCE)
    {
        DWORD dataLen, bytesNeeded;

        if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
        {
            BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]), oidLenBytes = 0;

            bytesNeeded = sizeof(CERT_EXTENSION);
            if (dataLen)
            {
                const BYTE *ptr = pbEncoded + 1 + lenBytes;
                DWORD encodedOidLen, oidLen;

                CRYPT_GetLen(ptr, cbEncoded - (ptr - pbEncoded),
                 &encodedOidLen);
                oidLenBytes = GET_LEN_BYTES(ptr[1]);
                ret = CRYPT_AsnDecodeOid(ptr, cbEncoded - (ptr - pbEncoded), 
                 dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, &oidLen);
                if (ret)
                {
                    bytesNeeded += oidLen;
                    ptr += 1 + encodedOidLen + oidLenBytes;
                    if (*ptr == ASN_BOOL)
                        ptr += 3;
                    ret = CRYPT_AsnDecodeOctets(X509_ASN_ENCODING,
                     X509_OCTET_STRING, ptr, cbEncoded - (ptr - pbEncoded),
                     0, NULL, NULL, &dataLen);
                    bytesNeeded += dataLen;
                    if (ret)
                    {
                        if (!ext)
                            *pcbExt = bytesNeeded;
                        else if (*pcbExt < bytesNeeded)
                        {
                            SetLastError(ERROR_MORE_DATA);
                            ret = FALSE;
                        }
                        else
                        {
                            ptr = pbEncoded + 2 + lenBytes + encodedOidLen +
                             oidLenBytes;
                            if (*ptr == ASN_BOOL)
                            {
                                CRYPT_DecodeBool(ptr, cbEncoded -
                                 (ptr - pbEncoded), &ext->fCritical);
                                ptr += 3;
                            }
                            else
                                ext->fCritical = FALSE;
                            ret = CRYPT_AsnDecodeOctets(X509_ASN_ENCODING,
                             X509_OCTET_STRING, ptr,
                             cbEncoded - (ptr - pbEncoded),
                             dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL,
                             &ext->Value, &dataLen);
                            if (ret)
                            {
                                ext->pszObjId = ext->Value.pbData +
                                 ext->Value.cbData;
                                ptr = pbEncoded + 1 + lenBytes;
                                ret = CRYPT_AsnDecodeOid(ptr,
                                 cbEncoded - (ptr - pbEncoded), 
                                 dwFlags & ~CRYPT_DECODE_ALLOC_FLAG,
                                 ext->pszObjId, &oidLen);
                            }
                        }
                    }
                }
            }
            else
            {
                SetLastError(CRYPT_E_ASN1_EOD);
                ret = FALSE;
            }
        }
    }
    else
    {
        SetLastError(CRYPT_E_ASN1_BADTAG);
        ret = FALSE;
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeExtensions(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;

    __TRY
    {
        if (pbEncoded[0] == ASN_SEQUENCEOF)
        {
            DWORD dataLen, bytesNeeded;

            if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
            {
                DWORD cExtension = 0;
                BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);

                bytesNeeded = sizeof(CERT_EXTENSIONS);
                if (dataLen)
                {
                    const BYTE *ptr;
                    DWORD size;

                    for (ptr = pbEncoded + 1 + lenBytes; ret &&
                     ptr - pbEncoded - 1 - lenBytes < dataLen; )
                    {
                        ret = CRYPT_AsnDecodeExtension(ptr,
                         cbEncoded - (ptr - pbEncoded), dwFlags, NULL, &size);
                        if (ret)
                        {
                            DWORD nextLen;

                            cExtension++;
                            bytesNeeded += size;
                            ret = CRYPT_GetLen(ptr,
                             cbEncoded - (ptr - pbEncoded), &nextLen);
                            if (ret)
                                ptr += nextLen + 1 + GET_LEN_BYTES(ptr[1]);
                        }
                    }
                }
                if (ret)
                {
                    if (!pvStructInfo)
                        *pcbStructInfo = bytesNeeded;
                    else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags,
                     pDecodePara, pvStructInfo, pcbStructInfo, bytesNeeded)))
                    {
                        DWORD size, i;
                        BYTE *nextData;
                        const BYTE *ptr;
                        CERT_EXTENSIONS *exts;

                        if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                            pvStructInfo = *(BYTE **)pvStructInfo;
                        *pcbStructInfo = bytesNeeded;
                        exts = (CERT_EXTENSIONS *)pvStructInfo;
                        exts->cExtension = cExtension;
                        exts->rgExtension = (CERT_EXTENSION *)((BYTE *)exts +
                         sizeof(CERT_EXTENSIONS));
                        nextData = (BYTE *)exts->rgExtension +
                         exts->cExtension * sizeof(CERT_EXTENSION);
                        for (i = 0, ptr = pbEncoded + 1 + lenBytes; ret &&
                         i < cExtension && ptr - pbEncoded - 1 - lenBytes <
                         dataLen; i++)
                        {
                            exts->rgExtension[i].Value.pbData = nextData;
                            size = bytesNeeded;
                            ret = CRYPT_AsnDecodeExtension(ptr,
                             cbEncoded - (ptr - pbEncoded), dwFlags,
                             &exts->rgExtension[i], &size);
                            if (ret)
                            {
                                DWORD nextLen;

                                bytesNeeded -= size;
                                /* If dwFlags & CRYPT_DECODE_NOCOPY_FLAG, the
                                 * data may not have been copied.
                                 */
                                if (exts->rgExtension[i].Value.pbData ==
                                 nextData)
                                    nextData +=
                                     exts->rgExtension[i].Value.cbData;
                                /* Ugly: the OID, if copied, is stored in
                                 * memory after the value, so increment by its
                                 * string length if it's set and points here.
                                 */
                                if ((const BYTE *)exts->rgExtension[i].pszObjId
                                 == nextData)
                                    nextData += strlen(
                                     exts->rgExtension[i].pszObjId) + 1;
                                ret = CRYPT_GetLen(ptr,
                                 cbEncoded - (ptr - pbEncoded), &nextLen);
                                if (ret)
                                    ptr += nextLen + 1 + GET_LEN_BYTES(ptr[1]);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            SetLastError(CRYPT_E_ASN1_BADTAG);
            ret = FALSE;
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

/* FIXME: honor the CRYPT_DECODE_SHARE_OID_STRING_FLAG. */
static BOOL WINAPI CRYPT_AsnDecodeOid(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, LPSTR pszObjId, DWORD *pcbObjId)
{
    BOOL ret = TRUE;

    __TRY
    {
        if (pbEncoded[0] == ASN_OBJECTIDENTIFIER)
        {
            DWORD dataLen;

            if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
            {
                BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);
                DWORD bytesNeeded;

                if (dataLen)
                {
                    /* The largest possible string for the first two components
                     * is 2.175 (= 2 * 40 + 175 = 255), so this is big enough.
                     */
                    char firstTwo[6];
                    const BYTE *ptr;

                    snprintf(firstTwo, sizeof(firstTwo), "%d.%d",
                     pbEncoded[1 + lenBytes] / 40,
                     pbEncoded[1 + lenBytes] - (pbEncoded[1 + lenBytes] / 40)
                     * 40);
                    bytesNeeded = strlen(firstTwo) + 1;
                    for (ptr = pbEncoded + 2 + lenBytes; ret &&
                     ptr - pbEncoded - 1 - lenBytes < dataLen; )
                    {
                        /* large enough for ".4000000" */
                        char str[9];
                        int val = 0;

                        while (ptr - pbEncoded - 1 - lenBytes < dataLen &&
                         (*ptr & 0x80))
                        {
                            val <<= 7;
                            val |= *ptr & 0x7f;
                            ptr++;
                        }
                        if (ptr - pbEncoded - 1 - lenBytes >= dataLen ||
                         (*ptr & 0x80))
                        {
                            SetLastError(CRYPT_E_ASN1_CORRUPT);
                            ret = FALSE;
                        }
                        else
                        {
                            val <<= 7;
                            val |= *ptr++;
                            snprintf(str, sizeof(str), ".%d", val);
                            bytesNeeded += strlen(str);
                        }
                    }
                    if (!pszObjId)
                        *pcbObjId = bytesNeeded;
                    else if (*pcbObjId < bytesNeeded)
                    {
                        *pcbObjId = bytesNeeded;
                        SetLastError(ERROR_MORE_DATA);
                        ret = FALSE;
                    }
                    else
                    {
                        sprintf(pszObjId, "%d.%d", pbEncoded[1 + lenBytes] / 40,
                         pbEncoded[1 + lenBytes] - (pbEncoded[1 + lenBytes] /
                         40) * 40);
                        pszObjId += strlen(pszObjId);
                        for (ptr = pbEncoded + 2 + lenBytes; ret &&
                         ptr - pbEncoded - 1 - lenBytes < dataLen; )
                        {
                            int val = 0;

                            while (ptr - pbEncoded - 1 - lenBytes < dataLen &&
                             (*ptr & 0x80))
                            {
                                val <<= 7;
                                val |= *ptr & 0x7f;
                                ptr++;
                            }
                            val <<= 7;
                            val |= *ptr++;
                            sprintf(pszObjId, ".%d", val);
                            pszObjId += strlen(pszObjId);
                        }
                    }
                }
                else
                    bytesNeeded = 0;
                *pcbObjId = bytesNeeded;
            }
        }
        else
        {
            SetLastError(CRYPT_E_ASN1_BADTAG);
            ret = FALSE;
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

/* Warning: this assumes the address of value->Value.pbData is already set, in
 * order to avoid overwriting memory.  (In some cases, it may change it, if it
 * doesn't copy anything to memory.)  Be sure to set it correctly!
 */
static BOOL WINAPI CRYPT_AsnDecodeNameValue(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, CERT_NAME_VALUE *value, DWORD *pcbValue)
{
    BOOL ret = TRUE;

    __TRY
    {
        DWORD dataLen;

        if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
        {
            BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);

            switch (pbEncoded[0])
            {
            case ASN_NUMERICSTRING:
            case ASN_PRINTABLESTRING:
            case ASN_IA5STRING:
                break;
            default:
                FIXME("Unimplemented string type %02x\n", pbEncoded[0]);
                SetLastError(OSS_UNIMPLEMENTED);
                ret = FALSE;
            }
            if (ret)
            {
                DWORD bytesNeeded = sizeof(CERT_NAME_VALUE);

                switch (pbEncoded[0])
                {
                case ASN_NUMERICSTRING:
                case ASN_PRINTABLESTRING:
                case ASN_IA5STRING:
                    if (!(dwFlags & CRYPT_DECODE_NOCOPY_FLAG))
                        bytesNeeded += dataLen;
                    break;
                }
                if (!value)
                    *pcbValue = bytesNeeded;
                else if (*pcbValue < bytesNeeded)
                {
                    *pcbValue = bytesNeeded;
                    SetLastError(ERROR_MORE_DATA);
                    ret = FALSE;
                }
                else
                {
                    *pcbValue = bytesNeeded;
                    switch (pbEncoded[0])
                    {
                    case ASN_NUMERICSTRING:
                        value->dwValueType = CERT_RDN_NUMERIC_STRING;
                        break;
                    case ASN_PRINTABLESTRING:
                        value->dwValueType = CERT_RDN_PRINTABLE_STRING;
                        break;
                    case ASN_IA5STRING:
                        value->dwValueType = CERT_RDN_IA5_STRING;
                        break;
                    }
                    if (dataLen)
                    {
                        switch (pbEncoded[0])
                        {
                        case ASN_NUMERICSTRING:
                        case ASN_PRINTABLESTRING:
                        case ASN_IA5STRING:
                            value->Value.cbData = dataLen;
                            if (dwFlags & CRYPT_DECODE_NOCOPY_FLAG)
                                value->Value.pbData = (BYTE *)pbEncoded + 1 +
                                 lenBytes;
                            else
                            {
                                if (!value->Value.pbData)
                                {
                                    SetLastError(CRYPT_E_ASN1_INTERNAL);
                                    ret = FALSE;
                                }
                                else
                                    memcpy(value->Value.pbData,
                                     pbEncoded + 1 + lenBytes, dataLen);
                            }
                            break;
                        }
                    }
                    else
                    {
                        value->Value.cbData = 0;
                        value->Value.pbData = NULL;
                    }
                }
            }
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeRdnAttr(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, CERT_RDN_ATTR *attr, DWORD *pcbAttr)
{
    BOOL ret;

    __TRY
    {
        if (pbEncoded[0] == (ASN_CONSTRUCTOR | ASN_SEQUENCE))
        {
            DWORD bytesNeeded, dataLen, size;
            BYTE lenBytes;

            if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
            {
                /* The data length must be at least 4, two for the tag and
                 * length for the OID, and two for the string (assuming both
                 * have short-form lengths.)
                 */
                if (dataLen < 4)
                {
                    SetLastError(CRYPT_E_ASN1_EOD);
                    ret = FALSE;
                }
                else
                {
                    bytesNeeded = sizeof(CERT_RDN_ATTR);
                    lenBytes = GET_LEN_BYTES(pbEncoded[1]);
                    ret = CRYPT_AsnDecodeOid(pbEncoded + 1 + lenBytes,
                     cbEncoded - 1 - lenBytes, dwFlags, NULL, &size);
                    if (ret)
                    {
                        /* ugly: need to know the size of the next element of
                         * the sequence, so get it directly
                         */
                        DWORD objIdOfset = 1 + lenBytes, objIdLen,
                         nameValueOffset = 0;

                        ret = CRYPT_GetLen(pbEncoded + objIdOfset,
                         cbEncoded - objIdOfset, &objIdLen);
                        bytesNeeded += size;
                        /* hack: like encoding, this takes advantage of the
                         * fact that the rest of the structure is identical to
                         * a CERT_NAME_VALUE.
                         */
                        if (ret)
                        {
                            nameValueOffset = objIdOfset + objIdLen + 1 +
                             GET_LEN_BYTES(pbEncoded[objIdOfset]);
                            ret = CRYPT_AsnDecodeNameValue(
                             pbEncoded + nameValueOffset,
                             cbEncoded - nameValueOffset, dwFlags, NULL, &size);
                        }
                        if (ret)
                        {
                            bytesNeeded += size;
                            if (!attr)
                                *pcbAttr = bytesNeeded;
                            else if (*pcbAttr < bytesNeeded)
                            {
                                *pcbAttr = bytesNeeded;
                                SetLastError(ERROR_MORE_DATA);
                                ret = FALSE;
                            }
                            else
                            {
                                BYTE *originalData = attr->Value.pbData;

                                *pcbAttr = bytesNeeded;
                                /* strange: decode the value first, because it
                                 * has a counted size, and we can store the OID
                                 * after it.  Keep track of the original data
                                 * pointer, we'll need to know whether it was
                                 * changed.
                                 */
                                size = bytesNeeded;
                                ret = CRYPT_AsnDecodeNameValue(
                                 pbEncoded + nameValueOffset,
                                 cbEncoded - nameValueOffset, dwFlags,
                                 (CERT_NAME_VALUE *)&attr->dwValueType, &size);
                                if (ret)
                                {
                                    if (objIdLen)
                                    {
                                        /* if the data were copied to the
                                         * original location, the OID goes
                                         * after.  Otherwise it goes in the
                                         * spot originally reserved for the
                                         * data.
                                         */
                                        if (attr->Value.pbData == originalData)
                                            attr->pszObjId =
                                             (LPSTR)(attr->Value.pbData +
                                             attr->Value.cbData);
                                        else
                                            attr->pszObjId = originalData;
                                        size = bytesNeeded - size;
                                        ret = CRYPT_AsnDecodeOid(
                                         pbEncoded + objIdOfset,
                                         cbEncoded - objIdOfset,
                                         dwFlags, attr->pszObjId, &size);
                                    }
                                    else
                                        attr->pszObjId = NULL;
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {
            SetLastError(CRYPT_E_ASN1_BADTAG);
            ret = FALSE;
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeRdn(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, CERT_RDN *rdn, DWORD *pcbRdn)
{
    BOOL ret = TRUE;

    __TRY
    {
        if (pbEncoded[0] == (ASN_CONSTRUCTOR | ASN_SETOF))
        {
            DWORD dataLen;

            if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
            {
                DWORD bytesNeeded, cRDNAttr = 0;
                BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);

                bytesNeeded = sizeof(CERT_RDN);
                if (dataLen)
                {
                    const BYTE *ptr;
                    DWORD size;

                    for (ptr = pbEncoded + 1 + lenBytes; ret &&
                     ptr - pbEncoded - 1 - lenBytes < dataLen; )
                    {
                        ret = CRYPT_AsnDecodeRdnAttr(ptr,
                         cbEncoded - (ptr - pbEncoded), dwFlags, NULL, &size);
                        if (ret)
                        {
                            DWORD nextLen;

                            cRDNAttr++;
                            bytesNeeded += size;
                            ret = CRYPT_GetLen(ptr,
                             cbEncoded - (ptr - pbEncoded), &nextLen);
                            if (ret)
                                ptr += nextLen + 1 + GET_LEN_BYTES(ptr[1]);
                        }
                    }
                }
                if (ret)
                {
                    if (!rdn)
                        *pcbRdn = bytesNeeded;
                    else if (*pcbRdn < bytesNeeded)
                    {
                        *pcbRdn = bytesNeeded;
                        SetLastError(ERROR_MORE_DATA);
                        ret = FALSE;
                    }
                    else
                    {
                        DWORD size, i;
                        BYTE *nextData;
                        const BYTE *ptr;

                        *pcbRdn = bytesNeeded;
                        rdn->cRDNAttr = cRDNAttr;
                        rdn->rgRDNAttr = (CERT_RDN_ATTR *)((BYTE *)rdn +
                         sizeof(CERT_RDN));
                        nextData = (BYTE *)rdn->rgRDNAttr +
                         rdn->cRDNAttr * sizeof(CERT_RDN_ATTR);
                        for (i = 0, ptr = pbEncoded + 1 + lenBytes; ret &&
                         i < cRDNAttr && ptr - pbEncoded - 1 - lenBytes <
                         dataLen; i++)
                        {
                            rdn->rgRDNAttr[i].Value.pbData = nextData;
                            size = bytesNeeded;
                            ret = CRYPT_AsnDecodeRdnAttr(ptr,
                             cbEncoded - (ptr - pbEncoded), dwFlags,
                             &rdn->rgRDNAttr[i], &size);
                            if (ret)
                            {
                                DWORD nextLen;

                                bytesNeeded -= size;
                                /* If dwFlags & CRYPT_DECODE_NOCOPY_FLAG, the
                                 * data may not have been copied.
                                 */
                                if (rdn->rgRDNAttr[i].Value.pbData == nextData)
                                    nextData +=
                                     rdn->rgRDNAttr[i].Value.cbData;
                                /* Ugly: the OID, if copied, is stored in
                                 * memory after the value, so increment by its
                                 * string length if it's set and points here.
                                 */
                                if ((const BYTE *)rdn->rgRDNAttr[i].pszObjId
                                 == nextData)
                                    nextData += strlen(
                                     rdn->rgRDNAttr[i].pszObjId) + 1;
                                ret = CRYPT_GetLen(ptr,
                                 cbEncoded - (ptr - pbEncoded), &nextLen);
                                if (ret)
                                    ptr += nextLen + 1 + GET_LEN_BYTES(ptr[1]);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            SetLastError(CRYPT_E_ASN1_BADTAG);
            ret = FALSE;
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeName(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;

    __TRY
    {
        if (pbEncoded[0] == (ASN_CONSTRUCTOR | ASN_SEQUENCEOF))
        {
            DWORD dataLen;

            if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
            {
                DWORD bytesNeeded, cRDN = 0;
                BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);

                bytesNeeded = sizeof(CERT_NAME_INFO);
                if (dataLen)
                {
                    const BYTE *ptr;

                    for (ptr = pbEncoded + 1 + lenBytes; ret &&
                     ptr - pbEncoded - 1 - lenBytes < dataLen; )
                    {
                        DWORD size;

                        ret = CRYPT_AsnDecodeRdn(ptr,
                         cbEncoded - (ptr - pbEncoded), dwFlags, NULL, &size);
                        if (ret)
                        {
                            DWORD nextLen;

                            cRDN++;
                            bytesNeeded += size;
                            ret = CRYPT_GetLen(ptr,
                             cbEncoded - (ptr - pbEncoded), &nextLen);
                            if (ret)
                                ptr += nextLen + 1 + GET_LEN_BYTES(ptr[1]);
                        }
                    }
                }
                if (ret)
                {
                    if (!pvStructInfo)
                        *pcbStructInfo = bytesNeeded;
                    else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags,
                     pDecodePara, pvStructInfo, pcbStructInfo, bytesNeeded)))
                    {
                        CERT_NAME_INFO *info;

                        if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                            pvStructInfo = *(BYTE **)pvStructInfo;
                        info = (CERT_NAME_INFO *)pvStructInfo;
                        info->cRDN = cRDN;
                        if (info->cRDN == 0)
                            info->rgRDN = NULL;
                        else
                        {
                            DWORD size, i;
                            BYTE *nextData;
                            const BYTE *ptr;

                            info->rgRDN = (CERT_RDN *)((BYTE *)pvStructInfo +
                             sizeof(CERT_NAME_INFO));
                            nextData = (BYTE *)info->rgRDN +
                             info->cRDN * sizeof(CERT_RDN);
                            for (i = 0, ptr = pbEncoded + 1 + lenBytes; ret &&
                             i < cRDN && ptr - pbEncoded - 1 - lenBytes <
                             dataLen; i++)
                            {
                                info->rgRDN[i].rgRDNAttr =
                                 (CERT_RDN_ATTR *)nextData;
                                size = bytesNeeded;
                                ret = CRYPT_AsnDecodeRdn(ptr,
                                 cbEncoded - (ptr - pbEncoded), dwFlags,
                                 &info->rgRDN[i], &size);
                                if (ret)
                                {
                                    DWORD nextLen;

                                    nextData += size;
                                    bytesNeeded -= size;
                                    ret = CRYPT_GetLen(ptr,
                                     cbEncoded - (ptr - pbEncoded), &nextLen);
                                    if (ret)
                                        ptr += nextLen + 1 +
                                         GET_LEN_BYTES(ptr[1]);
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {
            SetLastError(CRYPT_E_ASN1_BADTAG);
            ret = FALSE;
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

/* Warning:  assumes algo->Parameters.pbData is set ahead of time! */
static BOOL CRYPT_AsnDecodeAlgorithmIdentifier(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, CRYPT_ALGORITHM_IDENTIFIER *algo,
 DWORD *pcbAlgo)
{
    BOOL ret = TRUE;

    if (pbEncoded[0] == ASN_SEQUENCE)
    {
        DWORD dataLen, bytesNeeded;

        if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
        {
            BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]), oidLenBytes = 0;

            bytesNeeded = sizeof(CRYPT_ALGORITHM_IDENTIFIER);
            if (dataLen)
            {
                const BYTE *ptr = pbEncoded + 1 + lenBytes;
                DWORD encodedOidLen, oidLen;

                CRYPT_GetLen(ptr, cbEncoded - (ptr - pbEncoded),
                 &encodedOidLen);
                oidLenBytes = GET_LEN_BYTES(ptr[1]);
                ret = CRYPT_AsnDecodeOid(ptr, cbEncoded - (ptr - pbEncoded), 
                 dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, &oidLen);
                if (ret)
                {
                    bytesNeeded += oidLen;
                    ptr += 1 + encodedOidLen + oidLenBytes;
                    /* The remaining bytes are just copied as-is, no decoding
                     * is done.
                     * FIXME: is the CRYPT_DECODE_NOCOPY_FLAG used?
                     */
                    bytesNeeded += cbEncoded - (ptr - pbEncoded);
                    if (!algo)
                        *pcbAlgo = bytesNeeded;
                    else if (*pcbAlgo < bytesNeeded)
                    {
                        SetLastError(ERROR_MORE_DATA);
                        ret = FALSE;
                    }
                    else
                    {
                        DWORD paramsLen;

                        /* Get and sanity-check parameters first */
                        if ((ret = CRYPT_GetLen(ptr, cbEncoded -
                         (ptr - pbEncoded), &paramsLen)))
                        {
                            BYTE paramsLenBytes = GET_LEN_BYTES(ptr[1]);

                            if (paramsLen != dataLen - encodedOidLen - 1 -
                             oidLenBytes - 1 - paramsLenBytes)
                            {
                                SetLastError(CRYPT_E_ASN1_CORRUPT);
                                ret = FALSE;
                            }
                            else
                            {
                                *pcbAlgo = bytesNeeded;
                                algo->Parameters.cbData = dataLen -
                                 encodedOidLen - 1 - oidLenBytes;
                                if (algo->Parameters.cbData)
                                {
                                    memcpy(algo->Parameters.pbData, ptr,
                                     algo->Parameters.cbData);
                                    algo->pszObjId = algo->Parameters.pbData +
                                     algo->Parameters.cbData;
                                }
                                else
                                    algo->pszObjId = algo->Parameters.pbData;
                                ptr = pbEncoded + 1 + lenBytes;
                                ret = CRYPT_AsnDecodeOid(ptr,
                                 cbEncoded - (ptr - pbEncoded), 
                                 dwFlags & ~CRYPT_DECODE_ALLOC_FLAG,
                                 algo->pszObjId, &oidLen);
                            }
                        }
                    }
                }
            }
            else
            {
                SetLastError(CRYPT_E_ASN1_EOD);
                ret = FALSE;
            }
        }
    }
    else
    {
        SetLastError(CRYPT_E_ASN1_BADTAG);
        ret = FALSE;
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodePubKeyInfo(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;

    __TRY
    {
        if (pbEncoded[0] == ASN_SEQUENCE)
        {
            DWORD dataLen;

            if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
            {
                DWORD algoLen, bitLen, bytesNeeded = 0;
                BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);

                /* at least enough for the outer sequence and the inner
                 * sequence/OID combo
                 */
                if (dataLen >= 6)
                {
                    DWORD encodedAlgoLen;
                    BYTE algoLenBytes;

                    CRYPT_GetLen(pbEncoded + 1 + lenBytes,
                     cbEncoded - 1 - lenBytes, &encodedAlgoLen);
                    algoLenBytes = GET_LEN_BYTES(pbEncoded[2 + lenBytes]);
                    ret = CRYPT_AsnDecodeAlgorithmIdentifier(
                     pbEncoded + 1 + lenBytes, cbEncoded - 1 - lenBytes,
                     dwFlags, NULL, &algoLen);
                    if (ret)
                    {
                        bytesNeeded += algoLen;
                        ret = CRYPT_AsnDecodeBits(dwCertEncodingType,
                         X509_BITS, pbEncoded + 2 + lenBytes + algoLenBytes +
                         encodedAlgoLen, cbEncoded - (2 + lenBytes +
                         algoLenBytes + encodedAlgoLen), dwFlags, pDecodePara,
                         NULL, &bitLen);
                        if (ret)
                        {
                            bytesNeeded += bitLen;
                            if (!pvStructInfo)
                                *pcbStructInfo = bytesNeeded;
                            else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags,
                             pDecodePara, pvStructInfo, pcbStructInfo,
                             bytesNeeded)))
                            {
                                CERT_PUBLIC_KEY_INFO *info;

                                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                                    pvStructInfo = *(BYTE **)pvStructInfo;
                                info = (CERT_PUBLIC_KEY_INFO *)pvStructInfo;
                                memset(info, 0, sizeof(*info));
                                ret = CRYPT_AsnDecodeBits(
                                 dwCertEncodingType, X509_BITS,
                                 pbEncoded + 2 + lenBytes + algoLenBytes +
                                 encodedAlgoLen,
                                 cbEncoded - (2 + lenBytes + algoLenBytes +
                                 encodedAlgoLen),
                                 dwFlags & ~CRYPT_DECODE_ALLOC_FLAG,
                                 pDecodePara, &info->PublicKey, &bitLen);
                                if (ret)
                                {
                                    if (info->PublicKey.cbData)
                                        info->Algorithm.Parameters.pbData =
                                         info->PublicKey.pbData +
                                         info->PublicKey.cbData;
                                    else
                                        info->Algorithm.Parameters.pbData =
                                         (BYTE *)pvStructInfo +
                                         sizeof(CERT_PUBLIC_KEY_INFO);
                                    ret = CRYPT_AsnDecodeAlgorithmIdentifier(
                                     pbEncoded + 1 + lenBytes, cbEncoded - 1 -
                                     lenBytes, dwFlags, &info->Algorithm,
                                     &algoLen);
                                }
                            }
                        }
                    }
                }
                else
                {
                    SetLastError(CRYPT_E_ASN1_EOD);
                    ret = FALSE;
                }
            }
        }
        else
        {
            SetLastError(CRYPT_E_ASN1_BADTAG);
            ret = FALSE;
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_DecodeBool(const BYTE *pbEncoded, DWORD cbEncoded,
 BOOL *val)
{
    if (cbEncoded < 3)
    {
        SetLastError(CRYPT_E_ASN1_CORRUPT);
        return FALSE;
    }
    if (pbEncoded[0] != ASN_BOOL)
    {
        SetLastError(CRYPT_E_ASN1_BADTAG);
        return FALSE;
    }
    if (GET_LEN_BYTES(pbEncoded[1]) > 1)
    {
        SetLastError(CRYPT_E_ASN1_CORRUPT);
        return FALSE;
    }
    if (pbEncoded[1] > 1)
    {
        SetLastError(CRYPT_E_ASN1_CORRUPT);
        return FALSE;
    }
    *val = pbEncoded[2] ? TRUE : FALSE;
    return TRUE;
}

static BOOL WINAPI CRYPT_AsnDecodeBasicConstraints2(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    __TRY
    {
        if (pbEncoded[0] == ASN_SEQUENCE)
        {
            DWORD dataLen;

            if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
            {
                /* sanity-check length, space enough for 7 bytes of integer and
                 * 3 bytes of bool
                 */
                if (dataLen > 10)
                {
                    SetLastError(CRYPT_E_ASN1_CORRUPT);
                    ret = FALSE;
                }
                else
                {
                    DWORD bytesNeeded = sizeof(CERT_BASIC_CONSTRAINTS2_INFO);

                    if (!pvStructInfo)
                        *pcbStructInfo = bytesNeeded;
                    else
                    {
                        BYTE lenBytes;
                        CERT_BASIC_CONSTRAINTS2_INFO info = { 0 };

                        lenBytes = GET_LEN_BYTES(pbEncoded[1]);
                        pbEncoded += 1 + lenBytes;
                        cbEncoded -= 1 + lenBytes;
                        if (cbEncoded)
                        {
                            DWORD size;

                            if (pbEncoded[0] == ASN_BOOL)
                            {
                                ret = CRYPT_DecodeBool(pbEncoded, cbEncoded,
                                 &info.fCA);
                                if (ret)
                                {
                                    cbEncoded -= 2 + pbEncoded[1];
                                    pbEncoded += 2 + pbEncoded[1];
                                }
                            }
                            if (ret && cbEncoded && pbEncoded[0] == ASN_INTEGER)
                            {
                                size = sizeof(info.dwPathLenConstraint);
                                ret = CRYPT_AsnDecodeInt(dwCertEncodingType,
                                 X509_INTEGER, pbEncoded, cbEncoded, 0, NULL,
                                 &info.dwPathLenConstraint, &size);
                                if (ret)
                                {
                                    cbEncoded -= 2 + pbEncoded[1];
                                    pbEncoded += 2 + pbEncoded[1];
                                    if (cbEncoded)
                                    {
                                        SetLastError(CRYPT_E_ASN1_CORRUPT);
                                        ret = FALSE;
                                    }
                                    else
                                        info.fPathLenConstraint = TRUE;
                                }
                            }
                        }
                        if (ret)
                        {
                            if ((ret = CRYPT_DecodeEnsureSpace(dwFlags,
                             pDecodePara, pvStructInfo, pcbStructInfo,
                             bytesNeeded)))
                            {
                                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                                    pvStructInfo = *(BYTE **)pvStructInfo;
                                memcpy(pvStructInfo, &info,
                                 sizeof(CERT_BASIC_CONSTRAINTS2_INFO));
                            }
                        }
                    }
                }
            }
        }
        else
        {
            SetLastError(CRYPT_E_ASN1_BADTAG);
            ret = FALSE;
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeOctets(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    __TRY
    {
        if (pbEncoded[0] == ASN_OCTETSTRING)
        {
            DWORD bytesNeeded, dataLen;

            if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
            {
                if (dwFlags & CRYPT_DECODE_NOCOPY_FLAG)
                    bytesNeeded = sizeof(CRYPT_DATA_BLOB);
                else
                    bytesNeeded = dataLen + sizeof(CRYPT_DATA_BLOB);
                if (!pvStructInfo)
                    *pcbStructInfo = bytesNeeded;
                else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
                 pvStructInfo, pcbStructInfo, bytesNeeded)))
                {
                    CRYPT_DATA_BLOB *blob;
                    BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);

                    if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                        pvStructInfo = *(BYTE **)pvStructInfo;
                    blob = (CRYPT_DATA_BLOB *)pvStructInfo;
                    blob->cbData = dataLen;
                    if (dwFlags & CRYPT_DECODE_NOCOPY_FLAG)
                        blob->pbData = (BYTE *)pbEncoded + 1 + lenBytes;
                    else
                    {
                        blob->pbData = (BYTE *)pvStructInfo +
                         sizeof(CRYPT_DATA_BLOB);
                        if (blob->cbData)
                            memcpy(blob->pbData, pbEncoded + 1 + lenBytes,
                             blob->cbData);
                    }
                }
            }
        }
        else
        {
            SetLastError(CRYPT_E_ASN1_BADTAG);
            ret = FALSE;
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeBits(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    TRACE("(0x%08lx, %s, %p, %ld, 0x%08lx, %p, %p, %p)\n",
     dwCertEncodingType, HIWORD(lpszStructType) ? debugstr_a(lpszStructType) :
     "(integer value)", pbEncoded, cbEncoded, dwFlags, pDecodePara,
     pvStructInfo, pcbStructInfo);

    __TRY
    {
        if (pbEncoded[0] == ASN_BITSTRING)
        {
            DWORD bytesNeeded, dataLen;

            if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
            {
                if (dwFlags & CRYPT_DECODE_NOCOPY_FLAG)
                    bytesNeeded = sizeof(CRYPT_BIT_BLOB);
                else
                    bytesNeeded = dataLen - 1 + sizeof(CRYPT_BIT_BLOB);
                if (!pvStructInfo)
                    *pcbStructInfo = bytesNeeded;
                else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
                 pvStructInfo, pcbStructInfo, bytesNeeded)))
                {
                    CRYPT_BIT_BLOB *blob;

                    if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                        pvStructInfo = *(BYTE **)pvStructInfo;
                    blob = (CRYPT_BIT_BLOB *)pvStructInfo;
                    blob->cbData = dataLen - 1;
                    blob->cUnusedBits = *(pbEncoded + 1 +
                     GET_LEN_BYTES(pbEncoded[1]));
                    if (dwFlags & CRYPT_DECODE_NOCOPY_FLAG)
                        blob->pbData = (BYTE *)pbEncoded + 2 +
                         GET_LEN_BYTES(pbEncoded[1]);
                    else
                    {
                        blob->pbData = (BYTE *)pvStructInfo +
                         sizeof(CRYPT_BIT_BLOB);
                        if (blob->cbData)
                        {
                            BYTE mask = 0xff << blob->cUnusedBits;

                            memcpy(blob->pbData, pbEncoded + 2 +
                             GET_LEN_BYTES(pbEncoded[1]), blob->cbData);
                            blob->pbData[blob->cbData - 1] &= mask;
                        }
                    }
                }
            }
        }
        else
        {
            SetLastError(CRYPT_E_ASN1_BADTAG);
            ret = FALSE;
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeInt(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    if (!pvStructInfo)
    {
        *pcbStructInfo = sizeof(int);
        return TRUE;
    }
    __TRY
    {
        BYTE buf[sizeof(CRYPT_INTEGER_BLOB) + sizeof(int)];
        DWORD size = sizeof(buf);

        ret = CRYPT_AsnDecodeInteger(dwCertEncodingType,
         X509_MULTI_BYTE_INTEGER, pbEncoded, cbEncoded, 0, NULL, &buf, &size);
        if (ret)
        {
            CRYPT_INTEGER_BLOB *blob = (CRYPT_INTEGER_BLOB *)buf;

            if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
             pvStructInfo, pcbStructInfo, sizeof(int))))
            {
                int val, i;

                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;
                if (blob->pbData[blob->cbData - 1] & 0x80)
                {
                    /* initialize to a negative value to sign-extend */
                    val = -1;
                }
                else
                    val = 0;
                for (i = 0; i < blob->cbData; i++)
                {
                    val <<= 8;
                    val |= blob->pbData[blob->cbData - i - 1];
                }
                memcpy(pvStructInfo, &val, sizeof(int));
            }
        }
        else if (GetLastError() == ERROR_MORE_DATA)
            SetLastError(CRYPT_E_ASN1_LARGE);
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeInteger(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    __TRY
    {
        if (pbEncoded[0] == ASN_INTEGER)
        {
            DWORD bytesNeeded, dataLen;

            if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
            {
                BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);

                bytesNeeded = dataLen + sizeof(CRYPT_INTEGER_BLOB);
                if (!pvStructInfo)
                    *pcbStructInfo = bytesNeeded;
                else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
                 pvStructInfo, pcbStructInfo, bytesNeeded)))
                {
                    CRYPT_INTEGER_BLOB *blob;

                    if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                        pvStructInfo = *(BYTE **)pvStructInfo;
                    blob = (CRYPT_INTEGER_BLOB *)pvStructInfo;
                    blob->cbData = dataLen;
                    blob->pbData = (BYTE *)pvStructInfo +
                     sizeof(CRYPT_INTEGER_BLOB);
                    if (blob->cbData)
                    {
                        DWORD i;

                        for (i = 0; i < blob->cbData; i++)
                            blob->pbData[i] = *(pbEncoded + 1 + lenBytes +
                             dataLen - i - 1);
                    }
                }
            }
        }
        else
        {
            SetLastError(CRYPT_E_ASN1_BADTAG);
            ret = FALSE;
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeUnsignedInteger(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    __TRY
    {
        if (pbEncoded[0] == ASN_INTEGER)
        {
            DWORD bytesNeeded, dataLen;
            CRYPT_INTEGER_BLOB *blob;

            if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
            {
                bytesNeeded = dataLen + sizeof(CRYPT_INTEGER_BLOB);
                if (!pvStructInfo)
                    *pcbStructInfo = bytesNeeded;
                else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
                 pvStructInfo, pcbStructInfo, bytesNeeded)))
                {
                    BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);

                    if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                        pvStructInfo = *(BYTE **)pvStructInfo;
                    blob = (CRYPT_INTEGER_BLOB *)pvStructInfo;
                    blob->cbData = dataLen;
                    blob->pbData = (BYTE *)pvStructInfo +
                     sizeof(CRYPT_INTEGER_BLOB);
                    /* remove leading zero byte if it exists */
                    if (blob->cbData && *(pbEncoded + 1 + lenBytes) == 0)
                    {
                        blob->cbData--;
                        blob->pbData++;
                    }
                    if (blob->cbData)
                    {
                        DWORD i;

                        for (i = 0; i < blob->cbData; i++)
                            blob->pbData[i] = *(pbEncoded + 1 + lenBytes +
                             pbEncoded[1] - i - 1);
                    }
                }
            }
        }
        else
        {
            SetLastError(CRYPT_E_ASN1_BADTAG);
            ret = FALSE;
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeEnumerated(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    if (!pvStructInfo)
    {
        *pcbStructInfo = sizeof(int);
        return TRUE;
    }
    __TRY
    {
        if (pbEncoded[0] == ASN_ENUMERATED)
        {
            unsigned int val = 0, i;

            if (cbEncoded <= 1)
            {
                SetLastError(CRYPT_E_ASN1_EOD);
                ret = FALSE;
            }
            else if (pbEncoded[1] == 0)
            {
                SetLastError(CRYPT_E_ASN1_CORRUPT);
                ret = FALSE;
            }
            else
            {
                /* A little strange looking, but we have to accept a sign byte:
                 * 0xffffffff gets encoded as 0a 05 00 ff ff ff ff.  Also,
                 * assuming a small length is okay here, it has to be in short
                 * form.
                 */
                if (pbEncoded[1] > sizeof(unsigned int) + 1)
                {
                    SetLastError(CRYPT_E_ASN1_LARGE);
                    return FALSE;
                }
                for (i = 0; i < pbEncoded[1]; i++)
                {
                    val <<= 8;
                    val |= pbEncoded[2 + i];
                }
                if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
                 pvStructInfo, pcbStructInfo, sizeof(unsigned int))))
                {
                    if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                        pvStructInfo = *(BYTE **)pvStructInfo;
                    memcpy(pvStructInfo, &val, sizeof(unsigned int));
                }
            }
        }
        else
        {
            SetLastError(CRYPT_E_ASN1_BADTAG);
            ret = FALSE;
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

/* Modifies word, pbEncoded, and len, and magically sets a value ret to FALSE
 * if it fails.
 */
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
            ret = FALSE; \
        } \
        else \
        { \
            (word) *= 10; \
            (word) += *(pbEncoded)++ - '0'; \
        } \
    } \
 } while (0)

static BOOL CRYPT_AsnDecodeTimeZone(const BYTE *pbEncoded, DWORD len,
 SYSTEMTIME *sysTime)
{
    BOOL ret = TRUE;

    __TRY
    {
        if (len >= 3 && (*pbEncoded == '+' || *pbEncoded == '-'))
        {
            WORD hours, minutes = 0;
            BYTE sign = *pbEncoded++;

            len--;
            CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2, hours);
            if (ret && hours >= 24)
            {
                SetLastError(CRYPT_E_ASN1_CORRUPT);
                ret = FALSE;
            }
            else if (len >= 2)
            {
                CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2, minutes);
                if (ret && minutes >= 60)
                {
                    SetLastError(CRYPT_E_ASN1_CORRUPT);
                    ret = FALSE;
                }
            }
            if (ret)
            {
                if (sign == '+')
                {
                    sysTime->wHour += hours;
                    sysTime->wMinute += minutes;
                }
                else
                {
                    if (hours > sysTime->wHour)
                    {
                        sysTime->wDay--;
                        sysTime->wHour = 24 - (hours - sysTime->wHour);
                    }
                    else
                        sysTime->wHour -= hours;
                    if (minutes > sysTime->wMinute)
                    {
                        sysTime->wHour--;
                        sysTime->wMinute = 60 - (minutes - sysTime->wMinute);
                    }
                    else
                        sysTime->wMinute -= minutes;
                }
            }
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

#define MIN_ENCODED_TIME_LENGTH 10

static BOOL WINAPI CRYPT_AsnDecodeUtcTime(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;

    if (!pvStructInfo)
    {
        *pcbStructInfo = sizeof(FILETIME);
        return TRUE;
    }
    __TRY
    {
        if (pbEncoded[0] == ASN_UTCTIME)
        {
            if (cbEncoded <= 1)
            {
                SetLastError(CRYPT_E_ASN1_EOD);
                ret = FALSE;
            }
            else if (pbEncoded[1] > 0x7f)
            {
                /* long-form date strings really can't be valid */
                SetLastError(CRYPT_E_ASN1_CORRUPT);
                ret = FALSE;
            }
            else
            {
                SYSTEMTIME sysTime = { 0 };
                BYTE len = pbEncoded[1];

                if (len < MIN_ENCODED_TIME_LENGTH)
                {
                    SetLastError(CRYPT_E_ASN1_CORRUPT);
                    ret = FALSE;
                }
                else
                {
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
                    if (ret && len > 0)
                    {
                        if (len >= 2 && isdigit(*pbEncoded) &&
                         isdigit(*(pbEncoded + 1)))
                            CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2,
                             sysTime.wSecond);
                        else if (isdigit(*pbEncoded))
                            CRYPT_TIME_GET_DIGITS(pbEncoded, len, 1,
                             sysTime.wSecond);
                        if (ret)
                            ret = CRYPT_AsnDecodeTimeZone(pbEncoded, len,
                             &sysTime);
                    }
                    if (ret && (ret = CRYPT_DecodeEnsureSpace(dwFlags,
                     pDecodePara, pvStructInfo, pcbStructInfo,
                     sizeof(FILETIME))))
                    {
                        if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                            pvStructInfo = *(BYTE **)pvStructInfo;
                        ret = SystemTimeToFileTime(&sysTime,
                         (FILETIME *)pvStructInfo);
                    }
                }
            }
        }
        else
        {
            SetLastError(CRYPT_E_ASN1_BADTAG);
            ret = FALSE;
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeGeneralizedTime(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;

    if (!pvStructInfo)
    {
        *pcbStructInfo = sizeof(FILETIME);
        return TRUE;
    }
    __TRY
    {
        if (pbEncoded[0] == ASN_GENERALTIME)
        {
            if (cbEncoded <= 1)
            {
                SetLastError(CRYPT_E_ASN1_EOD);
                ret = FALSE;
            }
            else if (pbEncoded[1] > 0x7f)
            {
                /* long-form date strings really can't be valid */
                SetLastError(CRYPT_E_ASN1_CORRUPT);
                ret = FALSE;
            }
            else
            {
                BYTE len = pbEncoded[1];

                if (len < MIN_ENCODED_TIME_LENGTH)
                {
                    SetLastError(CRYPT_E_ASN1_CORRUPT);
                    ret = FALSE;
                }
                else
                {
                    SYSTEMTIME sysTime = { 0 };

                    pbEncoded += 2;
                    CRYPT_TIME_GET_DIGITS(pbEncoded, len, 4, sysTime.wYear);
                    CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2, sysTime.wMonth);
                    CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2, sysTime.wDay);
                    CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2, sysTime.wHour);
                    if (ret && len > 0)
                    {
                        CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2,
                         sysTime.wMinute);
                        if (ret && len > 0)
                            CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2,
                             sysTime.wSecond);
                        if (ret && len > 0 && (*pbEncoded == '.' ||
                         *pbEncoded == ','))
                        {
                            BYTE digits;

                            pbEncoded++;
                            len--;
                            /* workaround macro weirdness */
                            digits = min(len, 3);
                            CRYPT_TIME_GET_DIGITS(pbEncoded, len, digits,
                             sysTime.wMilliseconds);
                        }
                        if (ret)
                            ret = CRYPT_AsnDecodeTimeZone(pbEncoded, len,
                             &sysTime);
                    }
                    if (ret && (ret = CRYPT_DecodeEnsureSpace(dwFlags,
                     pDecodePara, pvStructInfo, pcbStructInfo,
                     sizeof(FILETIME))))
                    {
                        if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                            pvStructInfo = *(BYTE **)pvStructInfo;
                        ret = SystemTimeToFileTime(&sysTime,
                         (FILETIME *)pvStructInfo);
                    }
                }
            }
        }
        else
        {
            SetLastError(CRYPT_E_ASN1_BADTAG);
            ret = FALSE;
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeChoiceOfTime(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    if (!pvStructInfo)
    {
        *pcbStructInfo = sizeof(FILETIME);
        return TRUE;
    }

    __TRY
    {
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
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeSequenceOfAny(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;

    __TRY
    {
        if (pbEncoded[0] == ASN_SEQUENCEOF)
        {
            DWORD bytesNeeded, dataLen, remainingLen, cValue;

            if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
            {
                BYTE lenBytes;
                const BYTE *ptr;

                lenBytes = GET_LEN_BYTES(pbEncoded[1]);
                bytesNeeded = sizeof(CRYPT_SEQUENCE_OF_ANY);
                cValue = 0;
                ptr = pbEncoded + 1 + lenBytes;
                remainingLen = dataLen;
                while (ret && remainingLen)
                {
                    DWORD nextLen;

                    ret = CRYPT_GetLen(ptr, remainingLen, &nextLen);
                    if (ret)
                    {
                        DWORD nextLenBytes = GET_LEN_BYTES(ptr[1]);

                        remainingLen -= 1 + nextLenBytes + nextLen;
                        ptr += 1 + nextLenBytes + nextLen;
                        bytesNeeded += sizeof(CRYPT_DER_BLOB);
                        if (!(dwFlags & CRYPT_DECODE_NOCOPY_FLAG))
                            bytesNeeded += 1 + nextLenBytes + nextLen;
                        cValue++;
                    }
                }
                if (ret)
                {
                    CRYPT_SEQUENCE_OF_ANY *seq;
                    BYTE *nextPtr;
                    DWORD i;

                    if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
                     pvStructInfo, pcbStructInfo, bytesNeeded)))
                    {
                        if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                            pvStructInfo = *(BYTE **)pvStructInfo;
                        seq = (CRYPT_SEQUENCE_OF_ANY *)pvStructInfo;
                        seq->cValue = cValue;
                        seq->rgValue = (CRYPT_DER_BLOB *)((BYTE *)seq +
                         sizeof(*seq));
                        nextPtr = (BYTE *)seq->rgValue +
                         cValue * sizeof(CRYPT_DER_BLOB);
                        ptr = pbEncoded + 1 + lenBytes;
                        remainingLen = dataLen;
                        i = 0;
                        while (ret && remainingLen)
                        {
                            DWORD nextLen;

                            ret = CRYPT_GetLen(ptr, remainingLen, &nextLen);
                            if (ret)
                            {
                                DWORD nextLenBytes = GET_LEN_BYTES(ptr[1]);

                                seq->rgValue[i].cbData = 1 + nextLenBytes +
                                 nextLen;
                                if (dwFlags & CRYPT_DECODE_NOCOPY_FLAG)
                                    seq->rgValue[i].pbData = (BYTE *)ptr;
                                else
                                {
                                    seq->rgValue[i].pbData = nextPtr;
                                    memcpy(nextPtr, ptr, 1 + nextLenBytes +
                                     nextLen);
                                    nextPtr += 1 + nextLenBytes + nextLen;
                                }
                                remainingLen -= 1 + nextLenBytes + nextLen;
                                ptr += 1 + nextLenBytes + nextLen;
                                i++;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            SetLastError(CRYPT_E_ASN1_BADTAG);
            return FALSE;
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
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

    TRACE("(0x%08lx, %s, %p, %ld, 0x%08lx, %p, %p, %p)\n",
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
    if (!pbEncoded || !cbEncoded)
    {
        SetLastError(CRYPT_E_ASN1_EOD);
        return FALSE;
    }
    if (cbEncoded > MAX_ENCODED_LEN)
    {
        SetLastError(CRYPT_E_ASN1_LARGE);
        return FALSE;
    }

    SetLastError(NOERROR);
    if (dwFlags & CRYPT_DECODE_ALLOC_FLAG && pvStructInfo)
        *(BYTE **)pvStructInfo = NULL;
    if (!HIWORD(lpszStructType))
    {
        switch (LOWORD(lpszStructType))
        {
        case (WORD)X509_EXTENSIONS:
            decodeFunc = CRYPT_AsnDecodeExtensions;
            break;
        case (WORD)X509_NAME:
            decodeFunc = CRYPT_AsnDecodeName;
            break;
        case (WORD)X509_PUBLIC_KEY_INFO:
            decodeFunc = CRYPT_AsnDecodePubKeyInfo;
            break;
        case (WORD)X509_BASIC_CONSTRAINTS2:
            decodeFunc = CRYPT_AsnDecodeBasicConstraints2;
            break;
        case (WORD)X509_OCTET_STRING:
            decodeFunc = CRYPT_AsnDecodeOctets;
            break;
        case (WORD)X509_BITS:
        case (WORD)X509_KEY_USAGE:
            decodeFunc = CRYPT_AsnDecodeBits;
            break;
        case (WORD)X509_INTEGER:
            decodeFunc = CRYPT_AsnDecodeInt;
            break;
        case (WORD)X509_MULTI_BYTE_INTEGER:
            decodeFunc = CRYPT_AsnDecodeInteger;
            break;
        case (WORD)X509_MULTI_BYTE_UINT:
            decodeFunc = CRYPT_AsnDecodeUnsignedInteger;
            break;
        case (WORD)X509_ENUMERATED:
            decodeFunc = CRYPT_AsnDecodeEnumerated;
            break;
        case (WORD)X509_CHOICE_OF_TIME:
            decodeFunc = CRYPT_AsnDecodeChoiceOfTime;
            break;
        case (WORD)X509_SEQUENCE_OF_ANY:
            decodeFunc = CRYPT_AsnDecodeSequenceOfAny;
            break;
        case (WORD)PKCS_UTC_TIME:
            decodeFunc = CRYPT_AsnDecodeUtcTime;
            break;
        default:
            FIXME("%d: unimplemented\n", LOWORD(lpszStructType));
        }
    }
    else if (!strcmp(lpszStructType, szOID_CERT_EXTENSIONS))
        decodeFunc = CRYPT_AsnDecodeExtensions;
    else if (!strcmp(lpszStructType, szOID_RSA_signingTime))
        decodeFunc = CRYPT_AsnDecodeUtcTime;
    else if (!strcmp(lpszStructType, szOID_CRL_REASON_CODE))
        decodeFunc = CRYPT_AsnDecodeEnumerated;
    else if (!strcmp(lpszStructType, szOID_KEY_USAGE))
        decodeFunc = CRYPT_AsnDecodeBits;
    else if (!strcmp(lpszStructType, szOID_SUBJECT_KEY_IDENTIFIER))
        decodeFunc = CRYPT_AsnDecodeOctets;
    else if (!strcmp(lpszStructType, szOID_BASIC_CONSTRAINTS2))
        decodeFunc = CRYPT_AsnDecodeBasicConstraints2;
    else
        TRACE("OID %s not found or unimplemented, looking for DLL\n",
         debugstr_a(lpszStructType));
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
