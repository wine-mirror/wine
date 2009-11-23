/*
 * IQueryAssociations object and helper functions
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

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "objbase.h"
#include "shlguid.h"
#include "shlwapi.h"
#include "shell32_main.h"
#include "ver.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/**************************************************************************
 *  IQueryAssociations {SHELL32}
 *
 * DESCRIPTION
 *  This object provides a layer of abstraction over the system registry in
 *  order to simplify the process of parsing associations between files.
 *  Associations in this context means the registry entries that link (for
 *  example) the extension of a file with its description, list of
 *  applications to open the file with, and actions that can be performed on it
 *  (the shell displays such information in the context menu of explorer
 *  when you right-click on a file).
 *
 * HELPERS
 * You can use this object transparently by calling the helper functions
 * AssocQueryKeyA(), AssocQueryStringA() and AssocQueryStringByKeyA(). These
 * create an IQueryAssociations object, perform the requested actions
 * and then dispose of the object. Alternatively, you can create an instance
 * of the object using AssocCreate() and call the following methods on it:
 *
 * METHODS
 */

typedef struct
{
  const IQueryAssociationsVtbl *lpVtbl;
  LONG ref;
  HKEY hkeySource;
  HKEY hkeyProgID;
} IQueryAssociationsImpl;

/**************************************************************************
 *  IQueryAssociations_QueryInterface {SHLWAPI}
 *
 * See IUnknown_QueryInterface.
 */
static HRESULT WINAPI IQueryAssociations_fnQueryInterface(
  IQueryAssociations* iface,
  REFIID riid,
  LPVOID *ppvObj)
{
  IQueryAssociationsImpl *This = (IQueryAssociationsImpl *)iface;

  TRACE("(%p,%s,%p)\n",This, debugstr_guid(riid), ppvObj);

  *ppvObj = NULL;

  if (IsEqualIID(riid, &IID_IUnknown) ||
      IsEqualIID(riid, &IID_IQueryAssociations))
  {
    *ppvObj = This;

    IQueryAssociations_AddRef((IQueryAssociations*)*ppvObj);
    TRACE("Returning IQueryAssociations (%p)\n", *ppvObj);
    return S_OK;
  }
  TRACE("Returning E_NOINTERFACE\n");
  return E_NOINTERFACE;
}

/**************************************************************************
 *  IQueryAssociations_AddRef {SHLWAPI}
 *
 * See IUnknown_AddRef.
 */
static ULONG WINAPI IQueryAssociations_fnAddRef(IQueryAssociations *iface)
{
  IQueryAssociationsImpl *This = (IQueryAssociationsImpl *)iface;
  ULONG refCount = InterlockedIncrement(&This->ref);

  TRACE("(%p)->(ref before=%u)\n",This, refCount - 1);

  return refCount;
}

/**************************************************************************
 *  IQueryAssociations_Release {SHLWAPI}
 *
 * See IUnknown_Release.
 */
static ULONG WINAPI IQueryAssociations_fnRelease(IQueryAssociations *iface)
{
  IQueryAssociationsImpl *This = (IQueryAssociationsImpl *)iface;
  ULONG refCount = InterlockedDecrement(&This->ref);

  TRACE("(%p)->(ref before=%u)\n",This, refCount + 1);

  if (!refCount)
  {
    TRACE("Destroying IQueryAssociations (%p)\n", This);
    RegCloseKey(This->hkeySource);
    RegCloseKey(This->hkeyProgID);
    HeapFree(GetProcessHeap(), 0, This);
  }

  return refCount;
}

/**************************************************************************
 *  IQueryAssociations_Init {SHLWAPI}
 *
 * Initialise an IQueryAssociations object.
 *
 * PARAMS
 *  iface      [I] IQueryAssociations interface to initialise
 *  cfFlags    [I] ASSOCF_ flags from "shlwapi.h"
 *  pszAssoc   [I] String for the root key name, or NULL if hkeyProgid is given
 *  hkeyProgid [I] Handle for the root key, or NULL if pszAssoc is given
 *  hWnd       [I] Reserved, must be NULL.
 *
 * RETURNS
 *  Success: S_OK. iface is initialised with the parameters given.
 *  Failure: An HRESULT error code indicating the error.
 */
static HRESULT WINAPI IQueryAssociations_fnInit(
  IQueryAssociations *iface,
  ASSOCF cfFlags,
  LPCWSTR pszAssoc,
  HKEY hkeyProgid,
  HWND hWnd)
{
    static const WCHAR szProgID[] = {'P','r','o','g','I','D',0};
    IQueryAssociationsImpl *This = (IQueryAssociationsImpl *)iface;
    LONG ret;

    TRACE("(%p)->(%d,%s,%p,%p)\n", iface,
                                    cfFlags,
                                    debugstr_w(pszAssoc),
                                    hkeyProgid,
                                    hWnd);
    if (hWnd != NULL)
        FIXME("hwnd != NULL not supported\n");
    if (cfFlags != 0)
	FIXME("unsupported flags: %x\n", cfFlags);
    if (pszAssoc != NULL)
    {
        ret = RegOpenKeyExW(HKEY_CLASSES_ROOT,
                            pszAssoc,
                            0,
                            KEY_READ,
                            &This->hkeySource);
        if (ret != ERROR_SUCCESS)
            return E_FAIL;
        /* if this is not a prog id */
        if ((*pszAssoc == '.') || (*pszAssoc == '{'))
        {
            RegOpenKeyExW(This->hkeySource,
                          szProgID,
                          0,
                          KEY_READ,
                          &This->hkeyProgID);
        }
        else
            This->hkeyProgID = This->hkeySource;
        return S_OK;
    }
    else if (hkeyProgid != NULL)
    {
        This->hkeyProgID = hkeyProgid;
        return S_OK;
    }
    else
        return E_INVALIDARG;
}

static HRESULT ASSOC_GetValue(HKEY hkey, WCHAR ** pszText)
{
  DWORD len;
  LONG ret;

  assert(pszText);
  ret = RegQueryValueExW(hkey, NULL, 0, NULL, NULL, &len);
  if (ret != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(ret);
  if (!len)
    return E_FAIL;
  *pszText = HeapAlloc(GetProcessHeap(), 0, len);
  if (!*pszText)
    return E_OUTOFMEMORY;
  ret = RegQueryValueExW(hkey, NULL, 0, NULL, (LPBYTE)*pszText,
                         &len);
  if (ret != ERROR_SUCCESS)
  {
    HeapFree(GetProcessHeap(), 0, *pszText);
    return HRESULT_FROM_WIN32(ret);
  }
  return S_OK;
}

static HRESULT ASSOC_GetCommand(IQueryAssociationsImpl *This,
                                LPCWSTR pszExtra, WCHAR **ppszCommand)
{
  HKEY hkeyCommand;
  HKEY hkeyFile;
  HKEY hkeyShell;
  HKEY hkeyVerb;
  HRESULT hr;
  LONG ret;
  WCHAR * pszExtraFromReg = NULL;
  WCHAR * pszFileType;
  static const WCHAR commandW[] = { 'c','o','m','m','a','n','d',0 };
  static const WCHAR shellW[] = { 's','h','e','l','l',0 };

  hr = ASSOC_GetValue(This->hkeySource, &pszFileType);
  if (FAILED(hr))
    return hr;
  ret = RegOpenKeyExW(HKEY_CLASSES_ROOT, pszFileType, 0, KEY_READ, &hkeyFile);
  HeapFree(GetProcessHeap(), 0, pszFileType);
  if (ret != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(ret);

  ret = RegOpenKeyExW(hkeyFile, shellW, 0, KEY_READ, &hkeyShell);
  RegCloseKey(hkeyFile);
  if (ret != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(ret);

  if (!pszExtra)
  {
    hr = ASSOC_GetValue(hkeyShell, &pszExtraFromReg);
    /* if no default action */
    if (hr == E_FAIL || hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
    {
      DWORD rlen;
      ret = RegQueryInfoKeyW(hkeyShell, 0, 0, 0, 0, &rlen, 0, 0, 0, 0, 0, 0);
      if (ret != ERROR_SUCCESS)
      {
        RegCloseKey(hkeyShell);
        return HRESULT_FROM_WIN32(ret);
      }
      rlen++;
      pszExtraFromReg = HeapAlloc(GetProcessHeap(), 0, rlen * sizeof(WCHAR));
      if (!pszExtraFromReg)
      {
        RegCloseKey(hkeyShell);
        return E_OUTOFMEMORY;
      }
      ret = RegEnumKeyExW(hkeyShell, 0, pszExtraFromReg, &rlen, 0, NULL, NULL, NULL);
      if (ret != ERROR_SUCCESS)
      {
        RegCloseKey(hkeyShell);
        return HRESULT_FROM_WIN32(ret);
      }
    }
    else if (FAILED(hr))
    {
      RegCloseKey(hkeyShell);
      return hr;
    }
  }

  ret = RegOpenKeyExW(hkeyShell, pszExtra ? pszExtra : pszExtraFromReg, 0,
                      KEY_READ, &hkeyVerb);
  HeapFree(GetProcessHeap(), 0, pszExtraFromReg);
  RegCloseKey(hkeyShell);
  if (ret != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(ret);

  ret = RegOpenKeyExW(hkeyVerb, commandW, 0, KEY_READ, &hkeyCommand);
  RegCloseKey(hkeyVerb);
  if (ret != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(ret);
  hr = ASSOC_GetValue(hkeyCommand, ppszCommand);
  RegCloseKey(hkeyCommand);
  return hr;
}

static HRESULT ASSOC_GetExecutable(IQueryAssociationsImpl *This,
                                   LPCWSTR pszExtra, LPWSTR path,
                                   DWORD pathlen, DWORD *len)
{
  WCHAR *pszCommand;
  WCHAR *pszStart;
  WCHAR *pszEnd;
  HRESULT hr;

  assert(len);

  hr = ASSOC_GetCommand(This, pszExtra, &pszCommand);
  if (FAILED(hr))
    return hr;

  /* cleanup pszCommand */
  if (pszCommand[0] == '"')
  {
    pszStart = pszCommand + 1;
    pszEnd = strchrW(pszStart, '"');
    if (pszEnd)
      *pszEnd = 0;
    *len = SearchPathW(NULL, pszStart, NULL, pathlen, path, NULL);
  }
  else
  {
    pszStart = pszCommand;
    for (pszEnd = pszStart; (pszEnd = strchrW(pszEnd, ' ')); pszEnd++)
    {
      WCHAR c = *pszEnd;
      *pszEnd = 0;
      if ((*len = SearchPathW(NULL, pszStart, NULL, pathlen, path, NULL)))
        break;
      *pszEnd = c;
    }
    if (!pszEnd)
      *len = SearchPathW(NULL, pszStart, NULL, pathlen, path, NULL);
  }

  HeapFree(GetProcessHeap(), 0, pszCommand);
  if (!*len)
    return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
  return S_OK;
}

static HRESULT ASSOC_ReturnData(LPWSTR out, DWORD *outlen, LPCWSTR data,
                                DWORD datalen)
{
  assert(outlen);

  if (out)
  {
    if (*outlen < datalen)
    {
      *outlen = datalen;
      return E_POINTER;
    }
    *outlen = datalen;
    lstrcpynW(out, data, datalen);
    return S_OK;
  }
  else
  {
    *outlen = datalen;
    return S_FALSE;
  }
}

/**************************************************************************
 *  IQueryAssociations_GetString {SHLWAPI}
 *
 * Get a file association string from the registry.
 *
 * PARAMS
 *  iface    [I]   IQueryAssociations interface to query
 *  cfFlags  [I]   ASSOCF_ flags from "shlwapi.h"
 *  str      [I]   Type of string to get (ASSOCSTR enum from "shlwapi.h")
 *  pszExtra [I]   Extra information about the string location
 *  pszOut   [O]   Destination for the association string
 *  pcchOut  [I/O] Length of pszOut
 *
 * RETURNS
 *  Success: S_OK. pszOut contains the string, pcchOut contains its length.
 *  Failure: An HRESULT error code indicating the error.
 */
static HRESULT WINAPI IQueryAssociations_fnGetString(
  IQueryAssociations *iface,
  ASSOCF cfFlags,
  ASSOCSTR str,
  LPCWSTR pszExtra,
  LPWSTR pszOut,
  DWORD *pcchOut)
{
  IQueryAssociationsImpl *This = (IQueryAssociationsImpl *)iface;
  const ASSOCF cfUnimplemented = ~(0);
  DWORD len = 0;
  HRESULT hr;
  WCHAR path[MAX_PATH];

  TRACE("(%p,0x%8x,0x%8x,%s,%p,%p)\n", This, cfFlags, str,
        debugstr_w(pszExtra), pszOut, pcchOut);

  if (cfFlags & cfUnimplemented)
    FIXME("%08x: unimplemented flags!\n", cfFlags & cfUnimplemented);

  if (!pcchOut)
    return E_UNEXPECTED;

  switch (str)
  {
    case ASSOCSTR_COMMAND:
    {
      WCHAR *command;
      hr = ASSOC_GetCommand(This, pszExtra, &command);
      if (SUCCEEDED(hr))
      {
        hr = ASSOC_ReturnData(pszOut, pcchOut, command, strlenW(command) + 1);
        HeapFree(GetProcessHeap(), 0, command);
      }
      return hr;
    }

    case ASSOCSTR_EXECUTABLE:
    {
      hr = ASSOC_GetExecutable(This, pszExtra, path, MAX_PATH, &len);
      if (FAILED(hr))
        return hr;
      len++;
      return ASSOC_ReturnData(pszOut, pcchOut, path, len);
    }

    case ASSOCSTR_FRIENDLYDOCNAME:
    {
      WCHAR *pszFileType;
      DWORD ret;
      DWORD size;

      hr = ASSOC_GetValue(This->hkeySource, &pszFileType);
      if (FAILED(hr))
        return hr;
      size = 0;
      ret = RegGetValueW(HKEY_CLASSES_ROOT, pszFileType, NULL, RRF_RT_REG_SZ, NULL, NULL, &size);
      if (ret == ERROR_SUCCESS)
      {
        WCHAR *docName = HeapAlloc(GetProcessHeap(), 0, size);
        if (docName)
        {
          ret = RegGetValueW(HKEY_CLASSES_ROOT, pszFileType, NULL, RRF_RT_REG_SZ, NULL, docName, &size);
          if (ret == ERROR_SUCCESS)
            hr = ASSOC_ReturnData(pszOut, pcchOut, docName, strlenW(docName) + 1);
          else
            hr = HRESULT_FROM_WIN32(ret);
          HeapFree(GetProcessHeap(), 0, docName);
        }
        else
          hr = E_OUTOFMEMORY;
      }
      else
        hr = HRESULT_FROM_WIN32(ret);
      HeapFree(GetProcessHeap(), 0, pszFileType);
      return hr;
    }

    case ASSOCSTR_FRIENDLYAPPNAME:
    {
      PVOID verinfoW = NULL;
      DWORD size, retval = 0;
      UINT flen;
      WCHAR *bufW;
      static const WCHAR translationW[] = {
        '\\','V','a','r','F','i','l','e','I','n','f','o',
        '\\','T','r','a','n','s','l','a','t','i','o','n',0
      };
      static const WCHAR fileDescFmtW[] = {
        '\\','S','t','r','i','n','g','F','i','l','e','I','n','f','o',
        '\\','%','0','4','x','%','0','4','x',
        '\\','F','i','l','e','D','e','s','c','r','i','p','t','i','o','n',0
      };
      WCHAR fileDescW[41];

      hr = ASSOC_GetExecutable(This, pszExtra, path, MAX_PATH, &len);
      if (FAILED(hr))
        return hr;

      retval = GetFileVersionInfoSizeW(path, &size);
      if (!retval)
        goto get_friendly_name_fail;
      verinfoW = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, retval);
      if (!verinfoW)
        return E_OUTOFMEMORY;
      if (!GetFileVersionInfoW(path, 0, retval, verinfoW))
        goto get_friendly_name_fail;
      if (VerQueryValueW(verinfoW, translationW, (LPVOID *)&bufW, &flen))
      {
        UINT i;
        DWORD *langCodeDesc = (DWORD *)bufW;
        for (i = 0; i < flen / sizeof(DWORD); i++)
        {
          sprintfW(fileDescW, fileDescFmtW, LOWORD(langCodeDesc[i]),
                   HIWORD(langCodeDesc[i]));
          if (VerQueryValueW(verinfoW, fileDescW, (LPVOID *)&bufW, &flen))
          {
            /* Does strlenW(bufW) == 0 mean we use the filename? */
            len = strlenW(bufW) + 1;
            TRACE("found FileDescription: %s\n", debugstr_w(bufW));
            return ASSOC_ReturnData(pszOut, pcchOut, bufW, len);
          }
        }
      }
get_friendly_name_fail:
      PathRemoveExtensionW(path);
      PathStripPathW(path);
      TRACE("using filename: %s\n", debugstr_w(path));
      return ASSOC_ReturnData(pszOut, pcchOut, path, strlenW(path) + 1);
    }

    case ASSOCSTR_CONTENTTYPE:
    {
      static const WCHAR Content_TypeW[] = {'C','o','n','t','e','n','t',' ','T','y','p','e',0};
      WCHAR *contentType;
      DWORD ret;
      DWORD size;

      size = 0;
      ret = RegGetValueW(This->hkeySource, NULL, Content_TypeW, RRF_RT_REG_SZ, NULL, NULL, &size);
      if (ret != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(ret);
      contentType = HeapAlloc(GetProcessHeap(), 0, size);
      if (contentType != NULL)
      {
        ret = RegGetValueW(This->hkeySource, NULL, Content_TypeW, RRF_RT_REG_SZ, NULL, contentType, &size);
        if (ret == ERROR_SUCCESS)
          hr = ASSOC_ReturnData(pszOut, pcchOut, contentType, strlenW(contentType) + 1);
        else
          hr = HRESULT_FROM_WIN32(ret);
        HeapFree(GetProcessHeap(), 0, contentType);
      }
      else
        hr = E_OUTOFMEMORY;
      return hr;
    }

    case ASSOCSTR_DEFAULTICON:
    {
      static const WCHAR DefaultIconW[] = {'D','e','f','a','u','l','t','I','c','o','n',0};
      WCHAR *pszFileType;
      DWORD ret;
      DWORD size;
      HKEY hkeyFile;

      hr = ASSOC_GetValue(This->hkeySource, &pszFileType);
      if (FAILED(hr))
        return hr;
      ret = RegOpenKeyExW(HKEY_CLASSES_ROOT, pszFileType, 0, KEY_READ, &hkeyFile);
      if (ret == ERROR_SUCCESS)
      {
        size = 0;
        ret = RegGetValueW(hkeyFile, DefaultIconW, NULL, RRF_RT_REG_SZ, NULL, NULL, &size);
        if (ret == ERROR_SUCCESS)
        {
          WCHAR *icon = HeapAlloc(GetProcessHeap(), 0, size);
          if (icon)
          {
            ret = RegGetValueW(hkeyFile, DefaultIconW, NULL, RRF_RT_REG_SZ, NULL, icon, &size);
            if (ret == ERROR_SUCCESS)
              hr = ASSOC_ReturnData(pszOut, pcchOut, icon, strlenW(icon) + 1);
            else
              hr = HRESULT_FROM_WIN32(ret);
            HeapFree(GetProcessHeap(), 0, icon);
          }
          else
            hr = E_OUTOFMEMORY;
        }
        else
          hr = HRESULT_FROM_WIN32(ret);
        RegCloseKey(hkeyFile);
      }
      else
        hr = HRESULT_FROM_WIN32(ret);
      HeapFree(GetProcessHeap(), 0, pszFileType);
      return hr;
    }

    default:
      FIXME("assocstr %d unimplemented!\n", str);
      return E_NOTIMPL;
  }
}

/**************************************************************************
 *  IQueryAssociations_GetKey {SHLWAPI}
 *
 * Get a file association key from the registry.
 *
 * PARAMS
 *  iface    [I] IQueryAssociations interface to query
 *  cfFlags  [I] ASSOCF_ flags from "shlwapi.h"
 *  assockey [I] Type of key to get (ASSOCKEY enum from "shlwapi.h")
 *  pszExtra [I] Extra information about the key location
 *  phkeyOut [O] Destination for the association key
 *
 * RETURNS
 *  Success: S_OK. phkeyOut contains a handle to the key.
 *  Failure: An HRESULT error code indicating the error.
 */
static HRESULT WINAPI IQueryAssociations_fnGetKey(
  IQueryAssociations *iface,
  ASSOCF cfFlags,
  ASSOCKEY assockey,
  LPCWSTR pszExtra,
  HKEY *phkeyOut)
{
  IQueryAssociationsImpl *This = (IQueryAssociationsImpl *)iface;

  FIXME("(%p,0x%8x,0x%8x,%s,%p)-stub!\n", This, cfFlags, assockey,
        debugstr_w(pszExtra), phkeyOut);
  return E_NOTIMPL;
}

/**************************************************************************
 *  IQueryAssociations_GetData {SHLWAPI}
 *
 * Get the data for a file association key from the registry.
 *
 * PARAMS
 *  iface     [I]   IQueryAssociations interface to query
 *  cfFlags   [I]   ASSOCF_ flags from "shlwapi.h"
 *  assocdata [I]   Type of data to get (ASSOCDATA enum from "shlwapi.h")
 *  pszExtra  [I]   Extra information about the data location
 *  pvOut     [O]   Destination for the association key
 *  pcbOut    [I/O] Size of pvOut
 *
 * RETURNS
 *  Success: S_OK. pszOut contains the data, pcbOut contains its length.
 *  Failure: An HRESULT error code indicating the error.
 */
static HRESULT WINAPI IQueryAssociations_fnGetData(
  IQueryAssociations *iface,
  ASSOCF cfFlags,
  ASSOCDATA assocdata,
  LPCWSTR pszExtra,
  LPVOID pvOut,
  DWORD *pcbOut)
{
  IQueryAssociationsImpl *This = (IQueryAssociationsImpl *)iface;

  FIXME("(%p,0x%8x,0x%8x,%s,%p,%p)-stub!\n", This, cfFlags, assocdata,
        debugstr_w(pszExtra), pvOut, pcbOut);
  return E_NOTIMPL;
}

/**************************************************************************
 *  IQueryAssociations_GetEnum {SHLWAPI}
 *
 * Not yet implemented in native Win32.
 *
 * PARAMS
 *  iface     [I] IQueryAssociations interface to query
 *  cfFlags   [I] ASSOCF_ flags from "shlwapi.h"
 *  assocenum [I] Type of enum to get (ASSOCENUM enum from "shlwapi.h")
 *  pszExtra  [I] Extra information about the enum location
 *  riid      [I] REFIID to look for
 *  ppvOut    [O] Destination for the interface.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  Presumably this function returns an enumerator object.
 */
static HRESULT WINAPI IQueryAssociations_fnGetEnum(
  IQueryAssociations *iface,
  ASSOCF cfFlags,
  ASSOCENUM assocenum,
  LPCWSTR pszExtra,
  REFIID riid,
  LPVOID *ppvOut)
{
  IQueryAssociationsImpl *This = (IQueryAssociationsImpl *)iface;

  FIXME("(%p,0x%8x,0x%8x,%s,%s,%p)-stub!\n", This, cfFlags, assocenum,
        debugstr_w(pszExtra), debugstr_guid(riid), ppvOut);
  return E_NOTIMPL;
}

static const IQueryAssociationsVtbl IQueryAssociations_vtbl =
{
  IQueryAssociations_fnQueryInterface,
  IQueryAssociations_fnAddRef,
  IQueryAssociations_fnRelease,
  IQueryAssociations_fnInit,
  IQueryAssociations_fnGetString,
  IQueryAssociations_fnGetKey,
  IQueryAssociations_fnGetData,
  IQueryAssociations_fnGetEnum
};

/**************************************************************************
 *  IQueryAssociations_Constructor [internal]
 *
 * Construct a new IQueryAssociations object.
 */
HRESULT WINAPI QueryAssociations_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppOutput)
{
    IQueryAssociationsImpl* this;
    HRESULT ret;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    if (!(this = SHAlloc(sizeof(*this)))) return E_OUTOFMEMORY;
    this->lpVtbl = &IQueryAssociations_vtbl;
    this->ref = 0;
    this->hkeySource = 0;
    this->hkeyProgID = 0;
    if (FAILED(ret = IUnknown_QueryInterface((IUnknown *)this, riid, ppOutput))) SHFree( this );
    TRACE("returning %p\n", *ppOutput);
    return ret;
}
