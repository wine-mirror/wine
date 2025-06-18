/*
 * IQueryAssociations helper functions
 *
 * Copyright 2002 Jon Griffiths
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
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "objbase.h"
#include "shlguid.h"
#include "shlobj.h"
#include "shlwapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/* Default IQueryAssociations::Init() flags */
#define SHLWAPI_DEF_ASSOCF (ASSOCF_INIT_BYEXENAME|ASSOCF_INIT_DEFAULTTOSTAR| \
                            ASSOCF_INIT_DEFAULTTOFOLDER)

/*************************************************************************
 * SHLWAPI_ParamAToW
 *
 * Internal helper function: Convert ANSI parameter to Unicode.
 */
static BOOL SHLWAPI_ParamAToW(LPCSTR lpszParam, LPWSTR lpszBuff, DWORD dwLen,
                              LPWSTR* lpszOut)
{
  if (lpszParam)
  {
    DWORD dwStrLen = MultiByteToWideChar(CP_ACP, 0, lpszParam, -1, NULL, 0);

    if (dwStrLen < dwLen)
    {
      *lpszOut = lpszBuff; /* Use Buffer, it is big enough */
    }
    else
    {
      /* Create a new buffer big enough for the string */
      *lpszOut = malloc(dwStrLen * sizeof(WCHAR));
      if (!*lpszOut)
        return FALSE;
    }
    MultiByteToWideChar(CP_ACP, 0, lpszParam, -1, *lpszOut, dwStrLen);
  }
  else
    *lpszOut = NULL;
  return TRUE;
}

/*************************************************************************
 * AssocCreate  [SHLWAPI.@]
 *
 * Create a new IQueryAssociations object.
 *
 * PARAMS
 *  clsid       [I] CLSID of object
 *  refiid      [I] REFIID of interface
 *  lpInterface [O] Destination for the created IQueryAssociations object
 *
 * RETURNS
 *  Success: S_OK. lpInterface contains the new object.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  clsid  must be equal to CLSID_QueryAssociations and
 *  refiid must be equal to IID_IQueryAssociations, IID_IUnknown or this function will fail
 */
HRESULT WINAPI AssocCreate(CLSID clsid, REFIID refiid, void **lpInterface)
{
  TRACE("(%s,%s,%p)\n", debugstr_guid(&clsid), debugstr_guid(refiid),
        lpInterface);

  if (!lpInterface)
    return E_INVALIDARG;

  *(DWORD*)lpInterface = 0;

  if (!IsEqualGUID(&clsid,  &CLSID_QueryAssociations))
    return CLASS_E_CLASSNOTAVAILABLE;

  return SHCoCreateInstance( NULL, &clsid, NULL, refiid, lpInterface );
}

/*************************************************************************
 * AssocGetPerceivedType  [SHLWAPI.@]
 *
 * Detect the type of a file by inspecting its extension
 *
 * PARAMS
 *  lpszExt     [I] File extension to evaluate.
 *  lpType      [O] Pointer to perceived type
 *  lpFlag      [O] Pointer to perceived type flag
 *  lppszType   [O] Address to pointer for perceived type text
 *
 * RETURNS
 *  Success: S_OK. lpType and lpFlag contain the perceived type and
 *           its information. If lppszType is not NULL, it will point
 *           to a string with perceived type text.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  lppszType is optional and it can be NULL.
 *  if lpType or lpFlag are NULL, the function will crash.
 *  if lpszExt is NULL, an error is returned.
 *
 * BUGS
 *   Unimplemented.
 */
HRESULT WINAPI AssocGetPerceivedType(LPCWSTR lpszExt, PERCEIVED *lpType,
                                     INT *lpFlag, LPWSTR *lppszType)
{
  FIXME("(%s, %p, %p, %p) not supported\n", debugstr_w(lpszExt), lpType, lpFlag, lppszType);

  if (lpszExt == NULL)
    return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

  return E_NOTIMPL;
}

/*************************************************************************
 * AssocQueryKeyW  [SHLWAPI.@]
 *
 * See AssocQueryKeyA.
 */
HRESULT WINAPI AssocQueryKeyW(ASSOCF cfFlags, ASSOCKEY assockey, LPCWSTR pszAssoc,
                              LPCWSTR pszExtra, HKEY *phkeyOut)
{
  HRESULT hRet;
  IQueryAssociations* lpAssoc;

  TRACE("(0x%lx,%d,%s,%s,%p)\n", cfFlags, assockey, debugstr_w(pszAssoc),
        debugstr_w(pszExtra), phkeyOut);

  hRet = AssocCreate( CLSID_QueryAssociations, &IID_IQueryAssociations, (void **)&lpAssoc );
  if (FAILED(hRet)) return hRet;

  cfFlags &= SHLWAPI_DEF_ASSOCF;
  hRet = IQueryAssociations_Init(lpAssoc, cfFlags, pszAssoc, NULL, NULL);

  if (SUCCEEDED(hRet))
    hRet = IQueryAssociations_GetKey(lpAssoc, cfFlags, assockey, pszExtra, phkeyOut);

  IQueryAssociations_Release(lpAssoc);
  return hRet;
}

/*************************************************************************
 * AssocQueryKeyA  [SHLWAPI.@]
 *
 * Get a file association key from the registry.
 *
 * PARAMS
 *  cfFlags  [I] ASSOCF_ flags from "shlwapi.h"
 *  assockey [I] Type of key to get
 *  pszAssoc [I] Key name to search below
 *  pszExtra [I] Extra information about the key location
 *  phkeyOut [O] Destination for the association key
 *
 * RETURNS
 *  Success: S_OK. phkeyOut contains the key.
 *  Failure: An HRESULT error code indicating the error.
 */
HRESULT WINAPI AssocQueryKeyA(ASSOCF cfFlags, ASSOCKEY assockey, LPCSTR pszAssoc,
                              LPCSTR pszExtra, HKEY *phkeyOut)
{
  WCHAR szAssocW[MAX_PATH], *lpszAssocW = NULL;
  WCHAR szExtraW[MAX_PATH], *lpszExtraW = NULL;
  HRESULT hRet = E_OUTOFMEMORY;

  TRACE("(0x%lx,%d,%s,%s,%p)\n", cfFlags, assockey, debugstr_a(pszAssoc),
        debugstr_a(pszExtra), phkeyOut);

  if (SHLWAPI_ParamAToW(pszAssoc, szAssocW, MAX_PATH, &lpszAssocW) &&
      SHLWAPI_ParamAToW(pszExtra, szExtraW, MAX_PATH, &lpszExtraW))
  {
    hRet = AssocQueryKeyW(cfFlags, assockey, lpszAssocW, lpszExtraW, phkeyOut);
  }

  if (lpszAssocW != szAssocW)
    free(lpszAssocW);

  if (lpszExtraW != szExtraW)
    free(lpszExtraW);

  return hRet;
}

/*************************************************************************
 * AssocQueryStringW  [SHLWAPI.@]
 *
 * See AssocQueryStringA.
 */
HRESULT WINAPI AssocQueryStringW(ASSOCF cfFlags, ASSOCSTR str, LPCWSTR pszAssoc,
                                 LPCWSTR pszExtra, LPWSTR pszOut, DWORD *pcchOut)
{
  HRESULT hRet;
  IQueryAssociations* lpAssoc;

  TRACE("(0x%lx,%d,%s,%s,%p,%p)\n", cfFlags, str, debugstr_w(pszAssoc),
        debugstr_w(pszExtra), pszOut, pcchOut);

  if (!pcchOut)
    return E_UNEXPECTED;

  hRet = AssocCreate( CLSID_QueryAssociations, &IID_IQueryAssociations, (void **)&lpAssoc );
  if (FAILED(hRet)) return hRet;

  hRet = IQueryAssociations_Init(lpAssoc, cfFlags & SHLWAPI_DEF_ASSOCF,
                                 pszAssoc, NULL, NULL);

  if (SUCCEEDED(hRet))
    hRet = IQueryAssociations_GetString(lpAssoc, cfFlags, str, pszExtra,
                                        pszOut, pcchOut);

  IQueryAssociations_Release(lpAssoc);
  return hRet;
}

/*************************************************************************
 * AssocQueryStringA  [SHLWAPI.@]
 *
 * Get a file association string from the registry.
 *
 * PARAMS
 *  cfFlags  [I] ASSOCF_ flags from "shlwapi.h"
 *  str      [I] Type of string to get (ASSOCSTR enum from "shlwapi.h")
 *  pszAssoc [I] Key name to search below
 *  pszExtra [I] Extra information about the string location
 *  pszOut   [O] Destination for the association string
 *  pcchOut  [O] Length of pszOut
 *
 * RETURNS
 *  Success: S_OK. pszOut contains the string, pcchOut contains its length.
 *  Failure: An HRESULT error code indicating the error.
 */
HRESULT WINAPI AssocQueryStringA(ASSOCF cfFlags, ASSOCSTR str, LPCSTR pszAssoc,
                                 LPCSTR pszExtra, LPSTR pszOut, DWORD *pcchOut)
{
  WCHAR szAssocW[MAX_PATH], *lpszAssocW = NULL;
  WCHAR szExtraW[MAX_PATH], *lpszExtraW = NULL;
  HRESULT hRet = E_OUTOFMEMORY;

  TRACE("(0x%lx,0x%d,%s,%s,%p,%p)\n", cfFlags, str, debugstr_a(pszAssoc),
        debugstr_a(pszExtra), pszOut, pcchOut);

  if (!pcchOut)
    hRet = E_UNEXPECTED;
  else if (SHLWAPI_ParamAToW(pszAssoc, szAssocW, MAX_PATH, &lpszAssocW) &&
           SHLWAPI_ParamAToW(pszExtra, szExtraW, MAX_PATH, &lpszExtraW))
  {
    WCHAR szReturnW[MAX_PATH], *lpszReturnW = szReturnW;
    DWORD dwLenOut = *pcchOut;

    if (dwLenOut >= MAX_PATH)
      lpszReturnW = malloc((dwLenOut + 1) * sizeof(WCHAR));
    else
      dwLenOut = ARRAY_SIZE(szReturnW);

    if (!lpszReturnW)
      hRet = E_OUTOFMEMORY;
    else
    {
      hRet = AssocQueryStringW(cfFlags, str, lpszAssocW, lpszExtraW,
                               lpszReturnW, &dwLenOut);

      if (SUCCEEDED(hRet))
        dwLenOut = WideCharToMultiByte(CP_ACP, 0, lpszReturnW, -1,
                                       pszOut, *pcchOut, NULL, NULL);

      *pcchOut = dwLenOut;
      if (lpszReturnW != szReturnW)
        free(lpszReturnW);
    }
  }

  if (lpszAssocW != szAssocW)
    free(lpszAssocW);
  if (lpszExtraW != szExtraW)
    free(lpszExtraW);
  return hRet;
}

/*************************************************************************
 * AssocQueryStringByKeyW  [SHLWAPI.@]
 *
 * See AssocQueryStringByKeyA.
 */
HRESULT WINAPI AssocQueryStringByKeyW(ASSOCF cfFlags, ASSOCSTR str, HKEY hkAssoc,
                                      LPCWSTR pszExtra, LPWSTR pszOut,
                                      DWORD *pcchOut)
{
  HRESULT hRet;
  IQueryAssociations* lpAssoc;

  TRACE("(0x%lx,0x%d,%p,%s,%p,%p)\n", cfFlags, str, hkAssoc,
        debugstr_w(pszExtra), pszOut, pcchOut);

  hRet = AssocCreate( CLSID_QueryAssociations, &IID_IQueryAssociations, (void **)&lpAssoc );
  if (FAILED(hRet)) return hRet;

  cfFlags &= SHLWAPI_DEF_ASSOCF;
  hRet = IQueryAssociations_Init(lpAssoc, cfFlags, 0, hkAssoc, NULL);

  if (SUCCEEDED(hRet))
    hRet = IQueryAssociations_GetString(lpAssoc, cfFlags, str, pszExtra,
                                        pszOut, pcchOut);

  IQueryAssociations_Release(lpAssoc);
  return hRet;
}

/*************************************************************************
 * AssocQueryStringByKeyA  [SHLWAPI.@]
 *
 * Get a file association string from the registry, given a starting key.
 *
 * PARAMS
 *  cfFlags  [I] ASSOCF_ flags from "shlwapi.h"
 *  str      [I] Type of string to get
 *  hkAssoc  [I] Key to search below
 *  pszExtra [I] Extra information about the string location
 *  pszOut   [O] Destination for the association string
 *  pcchOut  [O] Length of pszOut
 *
 * RETURNS
 *  Success: S_OK. pszOut contains the string, pcchOut contains its length.
 *  Failure: An HRESULT error code indicating the error.
 */
HRESULT WINAPI AssocQueryStringByKeyA(ASSOCF cfFlags, ASSOCSTR str, HKEY hkAssoc,
                                      LPCSTR pszExtra, LPSTR pszOut,
                                      DWORD *pcchOut)
{
  WCHAR szExtraW[MAX_PATH], *lpszExtraW = szExtraW;
  WCHAR szReturnW[MAX_PATH], *lpszReturnW = szReturnW;
  HRESULT hRet = E_OUTOFMEMORY;

  TRACE("(0x%lx,0x%d,%p,%s,%p,%p)\n", cfFlags, str, hkAssoc,
        debugstr_a(pszExtra), pszOut, pcchOut);

  if (!pcchOut)
    hRet = E_INVALIDARG;
  else if (SHLWAPI_ParamAToW(pszExtra, szExtraW, MAX_PATH, &lpszExtraW))
  {
    DWORD dwLenOut = *pcchOut;
    if (dwLenOut >= MAX_PATH)
      lpszReturnW = malloc((dwLenOut + 1) * sizeof(WCHAR));

    if (lpszReturnW)
    {
      hRet = AssocQueryStringByKeyW(cfFlags, str, hkAssoc, lpszExtraW,
                                    lpszReturnW, &dwLenOut);

      if (SUCCEEDED(hRet))
        WideCharToMultiByte(CP_ACP,0,szReturnW,-1,pszOut,dwLenOut,0,0);
      *pcchOut = dwLenOut;

      if (lpszReturnW != szReturnW)
        free(lpszReturnW);
    }
  }

  if (lpszExtraW != szExtraW)
    free(lpszExtraW);
  return hRet;
}


/**************************************************************************
 *  AssocIsDangerous  (SHLWAPI.@)
 *  
 * Determine if a file association is dangerous (potentially malware).
 *
 * PARAMS
 *  lpszAssoc [I] Name of file or file extension to check.
 *
 * RETURNS
 *  TRUE, if lpszAssoc may potentially be malware (executable),
 *  FALSE, Otherwise.
 */
BOOL WINAPI AssocIsDangerous(LPCWSTR lpszAssoc)
{
    FIXME("%s\n", debugstr_w(lpszAssoc));
    return FALSE;
}
