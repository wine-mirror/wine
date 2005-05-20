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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

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
    static const WCHAR szDllName[] = { 'D','l','l',0 };
    HKEY hKey;
    LPSTR szKey;

    TRACE("%lx %s %s %s %s\n", dwEncodingType, pszFuncName, pszOID,
          debugstr_w(pwszDll), pszOverrideFuncName);

    /* MSDN states dwEncodingType is a mask, but it isn't really treated as
     * such.  Values 65536 or greater are ignored.
     */
    if (dwEncodingType >= PKCS_7_ASN_ENCODING)
        return TRUE;

    /* I'm not matching MS bug for bug here, because I doubt any app depends on
     * it:
     * - native does nothing if pwszDll is NULL
     * - native "succeeds" if pszFuncName is NULL, but the nonsensical entry
     *   it creates would never be used
     * - native returns an HRESULT rather than a Win32 error if pszOID is NULL.
     * Instead I disallow all of these with ERROR_INVALID_PARAMETER.
     */
    if (!pszFuncName || !pszOID || !pwszDll)
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

    if (dwEncodingType >= PKCS_7_ASN_ENCODING)
        return TRUE;

    if (!pszFuncName || !pszOID)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    szKey = CRYPT_GetKeyName(dwEncodingType, pszFuncName, pszOID);
    rc = RegDeleteKeyA(HKEY_LOCAL_MACHINE, szKey);
    if (!rc)
        SetLastError(rc);
    return rc ? FALSE : TRUE;
}

BOOL WINAPI CryptEncodeObject(DWORD dwCertEncodingType, LPCSTR lpszStructType,
 const void *pvStructInfo, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    FIXME("(0x%08lx, %s, %p, %p, %p): stub\n",
     dwCertEncodingType, HIWORD(lpszStructType) ? debugstr_a(lpszStructType) :
     "(integer value)", pvStructInfo, pbEncoded, pcbEncoded);
    return FALSE;
}

BOOL WINAPI CryptEncodeObjectEx(DWORD dwCertEncodingType, LPCSTR lpszStructType,
 const void *pvStructInfo, DWORD dwFlags, PCRYPT_ENCODE_PARA pEncodePara,
 BYTE *pbEncoded, DWORD *pcbEncoded)
{
    FIXME("(0x%08lx, %s, %p, 0x%08lx, %p, %p, %p): stub\n",
     dwCertEncodingType, HIWORD(lpszStructType) ? debugstr_a(lpszStructType) :
     "(integer value)", pvStructInfo, dwFlags, pEncodePara, pbEncoded,
     pcbEncoded);
    return FALSE;
}

BOOL WINAPI CryptDecodeObject(DWORD dwCertEncodingType, LPCSTR lpszStructType,
 const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo,
 DWORD *pcbStructInfo)
{
    FIXME("(0x%08lx, %s, %p, %ld, 0x%08lx, %p, %p): stub\n",
     dwCertEncodingType, HIWORD(lpszStructType) ? debugstr_a(lpszStructType) :
     "(integer value)", pbEncoded, cbEncoded, dwFlags, pvStructInfo,
     pcbStructInfo);
    return FALSE;
}

BOOL WINAPI CryptDecodeObjectEx(DWORD dwCertEncodingType, LPCSTR lpszStructType,
 const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    FIXME("(0x%08lx, %s, %p, %ld, 0x%08lx, %p, %p, %p): stub\n",
     dwCertEncodingType, HIWORD(lpszStructType) ? debugstr_a(lpszStructType) :
     "(integer value)", pbEncoded, cbEncoded, dwFlags, pDecodePara,
     pvStructInfo, pcbStructInfo);
    return FALSE;
}
