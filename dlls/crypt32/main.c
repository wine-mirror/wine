/*
 * Copyright 2002 Mike McCormack for Codeweavers
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
#include "winbase.h"
#include "wincrypt.h"
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

BOOL WINAPI CryptSIPAddProvider(SIP_ADD_NEWPROVIDER *psNewProv)
{
    FIXME("stub!\n");
    return FALSE;
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
    FIXME("(%lx,%s,%s,%s,%s) stub!\n", dwEncodingType, pszFuncName, pszOID,
          debugstr_w(pwszDll), pszOverrideFuncName);
    return FALSE;
}
