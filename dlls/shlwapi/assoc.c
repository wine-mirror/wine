/*
 * SHLWAPI IQueryAssociations helper functions
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * NOTES
 *  These function simplify the process of using the IQueryAssociations
 *  interface, primarily by providing the means to get an interface pointer.
 *  For those not interested in the object, the AssocQuery* functions hide
 *  the object completely and simply provide the functionality.
 *
 *  We require the implementation from shell32 because there is no interface
 *  for exporting an IQueryAssociations object available from that dll -
 *  as it is, we need the constructor.
 */
#include "windef.h"
#include "winnls.h"
#include "winreg.h"
#include "shlguid.h"
#include "shlwapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/* Default IQueryAssociations::Init() flags */
#define SHLWAPI_DEF_ASSOCF (ASSOCF_INIT_BYEXENAME|ASSOCF_INIT_DEFAULTTOSTAR| \
                            ASSOCF_INIT_DEFAULTTOFOLDER)

/* FIXME:
 * This is a temporary placeholder. We need the whole IQueryAssociations object.
 */
static IQueryAssociations* IQueryAssociations_Constructor()
{
  FIXME("() stub!\n");
  return NULL;
}

/*************************************************************************
 * SHLWAPI_ParamAToW
 *
 * Internal helper function: Convert ASCII parameter to Unicode.
 */
static BOOL SHLWAPI_ParamAToW(LPCSTR lpszParam, LPWSTR lpszBuff, DWORD dwLen,
                              LPWSTR* lpszOut)
{
  if (lpszParam)
  {
    DWORD dwStrLen = lstrlenA(lpszParam);

    if (dwStrLen < dwLen)
    {
      *lpszOut = lpszBuff; /* Use Buffer, it is big enough */
    }
    else
    {
      /* Create a new buffer big enough for the string */
      *lpszOut = (LPWSTR)HeapAlloc(GetProcessHeap(), 0,
                                   (dwStrLen + 1) * sizeof(WCHAR));
      if (!*lpszOut)
        return FALSE;
    }
    MultiByteToWideChar(0, 0, lpszParam, -1, *lpszOut, -1);
  }
  else
    *lpszOut = NULL;
  return TRUE;
}

/*************************************************************************
 * AssocCreate	[SHLWAPI.253]
 *
 * Create a new IQueryAssociations object.
 *
 * PARAMS
 *  clsid       [I] CLSID of object
 *  refiid      [I] REFIID of interface
 *  lpInterface [O] Destination for the created IQueryAssociations object
 *
 * RETURNS
 *  Success: S_OK. lpInterface contains the new object
 *  Failure: An HRESULT error value
 */
HRESULT WINAPI AssocCreate(CLSID clsid, REFIID refiid, void **lpInterface)
{
  HRESULT hRet;
  IQueryAssociations* lpAssoc;

  TRACE("(%s,%s,%p)\n", debugstr_guid(&clsid), debugstr_guid(refiid),
        lpInterface);

  if (!lpInterface)
    return E_INVALIDARG;

  *(DWORD*)lpInterface = 0;

  if (!IsEqualGUID(&clsid, &IID_IQueryAssociations))
    return E_NOTIMPL;

  lpAssoc = IQueryAssociations_Constructor();

  if (!lpAssoc)
    return E_OUTOFMEMORY;

  hRet = IQueryAssociations_QueryInterface(lpAssoc, refiid, lpInterface);
  IQueryAssociations_Release(lpAssoc);
  return hRet;
}

/*************************************************************************
 * AssocQueryKeyW	[SHLWAPI.255]
 *
 * See AssocQueryKeyA.
 */
HRESULT WINAPI AssocQueryKeyW(ASSOCF flags, ASSOCKEY key, LPCWSTR pszAssoc,
                              LPCWSTR pszExtra, HKEY *phkeyOut)
{
  HRESULT hRet;
  IQueryAssociations* lpAssoc;

  lpAssoc = IQueryAssociations_Constructor();

  if (!lpAssoc)
    return E_OUTOFMEMORY;

  flags &= SHLWAPI_DEF_ASSOCF;
  hRet = IQueryAssociations_Init(lpAssoc, flags, pszAssoc, NULL, NULL);

  if (SUCCEEDED(hRet))
    hRet = IQueryAssociations_GetKey(lpAssoc, flags, key, pszExtra, phkeyOut);

  IQueryAssociations_Release(lpAssoc);
  return hRet;
}

/*************************************************************************
 * AssocQueryKeyA	[SHLWAPI.254]
 *
 * Get a file association key from the registry.
 *
 * PARAMS
 *  flags    [I] Flags for the search
 *  key      [I] Type of key to get
 *  pszAssoc [I] Key name to search below
 *  pszExtra [I] Extra information about the key location
 *  phkeyOut [O] Destination for the association key
 *
 * RETURNS
 *  Success: S_OK. phkeyOut contains the key
 *  Failure: An HRESULT error code
 */
HRESULT WINAPI AssocQueryKeyA(ASSOCF flags, ASSOCKEY key, LPCSTR pszAssoc,
                              LPCSTR pszExtra, HKEY *phkeyOut)
{
  WCHAR szAssocW[MAX_PATH], *lpszAssocW = NULL;
  WCHAR szExtraW[MAX_PATH], *lpszExtraW = NULL;
  HRESULT hRet = E_OUTOFMEMORY;

  if (SHLWAPI_ParamAToW(pszAssoc, szAssocW, MAX_PATH, &lpszAssocW) &&
      SHLWAPI_ParamAToW(pszExtra, szExtraW, MAX_PATH, &lpszExtraW))
  {
    hRet = AssocQueryKeyW(flags, key, lpszAssocW, lpszExtraW, phkeyOut);
  }

  if (lpszAssocW && lpszAssocW != szAssocW)
    HeapFree(GetProcessHeap(), 0, lpszAssocW);

  if (lpszExtraW && lpszExtraW != szExtraW)
    HeapFree(GetProcessHeap(), 0, lpszExtraW);

  return hRet;
}

/*************************************************************************
 * AssocQueryStringW	[SHLWAPI.384]
 *
 * See AssocQueryStringA.
 */
HRESULT WINAPI AssocQueryStringW(ASSOCF flags, ASSOCSTR str, LPCWSTR pszAssoc,
                                 LPCWSTR pszExtra, LPWSTR pszOut, DWORD *pcchOut)
{
  HRESULT hRet;
  IQueryAssociations* lpAssoc;

  if (!pcchOut)
    return E_INVALIDARG;

  lpAssoc = IQueryAssociations_Constructor();

  if (!lpAssoc)
    return E_OUTOFMEMORY;

  hRet = IQueryAssociations_Init(lpAssoc, flags & SHLWAPI_DEF_ASSOCF,
                                 pszAssoc, NULL, NULL);

  if (SUCCEEDED(hRet))
    hRet = IQueryAssociations_GetString(lpAssoc, flags, str, pszExtra,
                                        pszOut, pcchOut);

  IQueryAssociations_Release(lpAssoc);
  return hRet;
}

/*************************************************************************
 * AssocQueryStringA	[SHLWAPI.381]
 *
 * Get a file association string from the registry.
 *
 * PARAMS
 *  flags    [I] Flags for the search
 *  str      [I] Type of string to get
 *  pszAssoc [I] Key name to search below
 *  pszExtra [I] Extra information about the string location
 *  pszOut   [O] Destination for the association string
 *  pcchOut  [O] Length of pszOut
 *
 * RETURNS
 *  Success: S_OK. pszOut contains the string.
 *  Failure: An HRESULT error code.
 */
HRESULT WINAPI AssocQueryStringA(ASSOCF flags, ASSOCSTR str, LPCSTR pszAssoc,
                                 LPCSTR pszExtra, LPSTR pszOut, DWORD *pcchOut)
{
  WCHAR szAssocW[MAX_PATH], *lpszAssocW = NULL;
  WCHAR szExtraW[MAX_PATH], *lpszExtraW = NULL;
  HRESULT hRet = E_OUTOFMEMORY;

  if (!pcchOut)
    hRet = E_INVALIDARG;
  else if (SHLWAPI_ParamAToW(pszAssoc, szAssocW, MAX_PATH, &lpszAssocW) &&
           SHLWAPI_ParamAToW(pszExtra, szExtraW, MAX_PATH, &lpszExtraW))
  {
    WCHAR szReturnW[MAX_PATH], *lpszReturnW = szReturnW;
    DWORD dwLenOut = *pcchOut;

    if (dwLenOut >= MAX_PATH)
      lpszReturnW = (LPWSTR)HeapAlloc(GetProcessHeap(), 0,
                                      (dwLenOut + 1) * sizeof(WCHAR));

    if (!lpszReturnW)
      hRet = E_OUTOFMEMORY;
    else
    {
      hRet = AssocQueryStringW(flags, str, lpszAssocW, lpszExtraW,
                               lpszReturnW, &dwLenOut);

      if (SUCCEEDED(hRet))
        WideCharToMultiByte(CP_ACP,0,szReturnW,-1,pszOut,dwLenOut,0,0);
      *pcchOut = dwLenOut;

      if (lpszReturnW && lpszReturnW != szReturnW)
        HeapFree(GetProcessHeap(), 0, lpszReturnW);
    }
  }

  if (lpszAssocW && lpszAssocW != szAssocW)
    HeapFree(GetProcessHeap(), 0, lpszAssocW);
  if (lpszExtraW && lpszExtraW != szExtraW)
    HeapFree(GetProcessHeap(), 0, lpszExtraW);
  return hRet;
}

/*************************************************************************
 * AssocQueryStringByKeyW	[SHLWAPI.383]
 *
 * See AssocQueryStringByKeyA.
 */
HRESULT WINAPI AssocQueryStringByKeyW(ASSOCF flags, ASSOCSTR str, HKEY hkAssoc,
                                      LPCWSTR pszExtra, LPWSTR pszOut,
                                      DWORD *pcchOut)
{
  HRESULT hRet;
  IQueryAssociations* lpAssoc;

  lpAssoc = IQueryAssociations_Constructor();

  if (!lpAssoc)
    return E_OUTOFMEMORY;

  flags &= SHLWAPI_DEF_ASSOCF;
  hRet = IQueryAssociations_Init(lpAssoc, flags, 0, hkAssoc, NULL);

  if (SUCCEEDED(hRet))
    hRet = IQueryAssociations_GetString(lpAssoc, flags, str, pszExtra,
                                        pszOut, pcchOut);

  IQueryAssociations_Release(lpAssoc);
  return hRet;
}

/*************************************************************************
 * AssocQueryStringByKeyA	[SHLWAPI.382]
 *
 * Get a file association string from the registry, given a starting key.
 *
 * PARAMS
 *  flags    [I] Flags for the search
 *  str      [I] Type of string to get
 *  hkAssoc  [I] Key to search below
 *  pszExtra [I] Extra information about the string location
 *  pszOut   [O] Destination for the association string
 *  pcchOut  [O] Length of pszOut
 *
 * RETURNS
 *  Success: S_OK. pszOut contains the string.
 *  Failure: An HRESULT error code.
 */
HRESULT WINAPI AssocQueryStringByKeyA(ASSOCF flags, ASSOCSTR str, HKEY hkAssoc,
                                      LPCSTR pszExtra, LPSTR pszOut,
                                      DWORD *pcchOut)
{
  WCHAR szExtraW[MAX_PATH], *lpszExtraW;
  WCHAR szReturnW[MAX_PATH], *lpszReturnW = szReturnW;
  HRESULT hRet = E_OUTOFMEMORY;

  if (!pcchOut)
    hRet = E_INVALIDARG;
  else if (SHLWAPI_ParamAToW(pszExtra, szExtraW, MAX_PATH, &lpszExtraW))
  {
    DWORD dwLenOut = *pcchOut;
    if (dwLenOut >= MAX_PATH)
      lpszReturnW = (LPWSTR)HeapAlloc(GetProcessHeap(), 0,
                                      (dwLenOut + 1) * sizeof(WCHAR));

    if (lpszReturnW)
    {
      hRet = AssocQueryStringByKeyW(flags, str, hkAssoc, lpszExtraW,
                                    lpszReturnW, &dwLenOut);

      if (SUCCEEDED(hRet))
        WideCharToMultiByte(CP_ACP,0,szReturnW,-1,pszOut,dwLenOut,0,0);
      *pcchOut = dwLenOut;

      if (lpszReturnW != szReturnW)
        HeapFree(GetProcessHeap(), 0, lpszReturnW);
    }
  }

  if (lpszExtraW && lpszExtraW != szExtraW)
    HeapFree(GetProcessHeap(), 0, lpszExtraW);
  return hRet;
}
