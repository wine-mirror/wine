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

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "objbase.h"
#include "shlguid.h"
#include "shlwapi.h"
#include "shobjidl.h"
#include "shell32_main.h"
#include "ver.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/**************************************************************************
 *  IQueryAssociations
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
  IQueryAssociations IQueryAssociations_iface;
  LONG ref;
  HKEY hkeySource;
  HKEY hkeyProgID;
} IQueryAssociationsImpl;

typedef struct
{
  IApplicationAssociationRegistration IApplicationAssociationRegistration_iface;
  LONG ref;
} IApplicationAssociationRegistrationImpl;


static inline IQueryAssociationsImpl *impl_from_IQueryAssociations(IQueryAssociations *iface)
{
  return CONTAINING_RECORD(iface, IQueryAssociationsImpl, IQueryAssociations_iface);
}

struct enumassochandlers
{
    IEnumAssocHandlers IEnumAssocHandlers_iface;
    LONG ref;
};

static inline struct enumassochandlers *impl_from_IEnumAssocHandlers(IEnumAssocHandlers *iface)
{
  return CONTAINING_RECORD(iface, struct enumassochandlers, IEnumAssocHandlers_iface);
}

static HRESULT ASSOC_GetValue(HKEY hkey, const WCHAR *name, void **data, DWORD *data_size);

/**************************************************************************
 *  IQueryAssociations_QueryInterface
 *
 * See IUnknown_QueryInterface.
 */
static HRESULT WINAPI IQueryAssociations_fnQueryInterface(
  IQueryAssociations* iface,
  REFIID riid,
  LPVOID *ppvObj)
{
  IQueryAssociationsImpl *This = impl_from_IQueryAssociations(iface);

  TRACE("(%p,%s,%p)\n",This, debugstr_guid(riid), ppvObj);

  if (ppvObj == NULL)
      return E_POINTER;

  *ppvObj = NULL;

  if (IsEqualIID(riid, &IID_IUnknown) ||
      IsEqualIID(riid, &IID_IQueryAssociations))
  {
    *ppvObj = &This->IQueryAssociations_iface;

    IQueryAssociations_AddRef((IQueryAssociations*)*ppvObj);
    TRACE("Returning IQueryAssociations (%p)\n", *ppvObj);
    return S_OK;
  }
  TRACE("Returning E_NOINTERFACE\n");
  return E_NOINTERFACE;
}

/**************************************************************************
 *  IQueryAssociations_AddRef
 *
 * See IUnknown_AddRef.
 */
static ULONG WINAPI IQueryAssociations_fnAddRef(IQueryAssociations *iface)
{
  IQueryAssociationsImpl *This = impl_from_IQueryAssociations(iface);
  ULONG refCount = InterlockedIncrement(&This->ref);

  TRACE("(%p)->(ref before=%lu)\n",This, refCount - 1);

  return refCount;
}

/**************************************************************************
 *  IQueryAssociations_Release
 *
 * See IUnknown_Release.
 */
static ULONG WINAPI IQueryAssociations_fnRelease(IQueryAssociations *iface)
{
  IQueryAssociationsImpl *This = impl_from_IQueryAssociations(iface);
  ULONG refCount = InterlockedDecrement(&This->ref);

  TRACE("(%p)->(ref before=%lu)\n",This, refCount + 1);

  if (!refCount)
  {
    TRACE("Destroying IQueryAssociations (%p)\n", This);
    RegCloseKey(This->hkeySource);
    RegCloseKey(This->hkeyProgID);
    SHFree(This);
  }

  return refCount;
}

/**************************************************************************
 *  IQueryAssociations_Init
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
    IQueryAssociationsImpl *This = impl_from_IQueryAssociations(iface);
    LONG ret;

    TRACE("(%p)->(%ld,%s,%p,%p)\n", iface,
                                    cfFlags,
                                    debugstr_w(pszAssoc),
                                    hkeyProgid,
                                    hWnd);
    if (hWnd != NULL)
        FIXME("hwnd != NULL not supported\n");
    if (cfFlags != 0)
	FIXME("unsupported flags: %lx\n", cfFlags);

    RegCloseKey(This->hkeySource);
    if (This->hkeySource != This->hkeyProgID)
        RegCloseKey(This->hkeyProgID);
    This->hkeySource = This->hkeyProgID = NULL;

    /* If the process of initializing hkeyProgID fails, just return S_OK. That's what Windows does. */
    if (pszAssoc != NULL)
    {
        WCHAR *progId;
        HRESULT hr;

        ret = RegOpenKeyExW(HKEY_CLASSES_ROOT,
                            pszAssoc,
                            0,
                            KEY_READ,
                            &This->hkeySource);
        if (ret)
            return S_OK;
        /* if this is a progid */
        if (*pszAssoc != '.' && *pszAssoc != '{')
        {
            This->hkeyProgID = This->hkeySource;
            return S_OK;
        }

        /* if it's not a progid, it's a file extension or clsid */
        if (*pszAssoc == '.')
        {
            /* for a file extension, the progid is the default value */
            hr = ASSOC_GetValue(This->hkeySource, NULL, (void**)&progId, NULL);
            if (FAILED(hr))
                return S_OK;
        }
        else /* if (*pszAssoc == '{') */
        {
            HKEY progIdKey;
            /* for a clsid, the progid is the default value of the ProgID subkey */
            ret = RegOpenKeyExW(This->hkeySource, L"ProgID", 0, KEY_READ, &progIdKey);
            if (ret != ERROR_SUCCESS)
                return S_OK;
            hr = ASSOC_GetValue(progIdKey, NULL, (void**)&progId, NULL);
            if (FAILED(hr))
                return S_OK;
            RegCloseKey(progIdKey);
        }

        /* open the actual progid key, the one with the shell subkey */
        ret = RegOpenKeyExW(HKEY_CLASSES_ROOT,
                            progId,
                            0,
                            KEY_READ,
                            &This->hkeyProgID);
        free(progId);

        return S_OK;
    }
    else if (hkeyProgid != NULL)
    {
        /* reopen the key so we don't end up closing a key owned by the caller */
        RegOpenKeyExW(hkeyProgid, NULL, 0, KEY_READ, &This->hkeyProgID);
        This->hkeySource = This->hkeyProgID;
        return S_OK;
    }
    else
        return E_INVALIDARG;
}

static HRESULT ASSOC_GetValue(HKEY hkey, const WCHAR *name, void **data, DWORD *data_size)
{
  DWORD size;
  LONG ret;

  ret = RegQueryValueExW(hkey, name, 0, NULL, NULL, &size);
  if (ret != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(ret);
  if (!size)
    return E_FAIL;
  *data = malloc(size);
  if (!*data)
    return E_OUTOFMEMORY;
  ret = RegQueryValueExW(hkey, name, 0, NULL, (LPBYTE)*data, &size);
  if (ret != ERROR_SUCCESS)
  {
    free(*data);
    return HRESULT_FROM_WIN32(ret);
  }
  if(data_size)
      *data_size = size;
  return S_OK;
}

static HRESULT ASSOC_GetCommand(IQueryAssociationsImpl *This, const WCHAR *extra, WCHAR **command)
{
  HKEY hkeyCommand;
  HKEY hkeyShell;
  HKEY hkeyVerb;
  HRESULT hr;
  LONG ret;
  WCHAR *extra_from_reg = NULL;
  WCHAR *filetype;

  /* When looking for file extension it's possible to have a default value
     that points to another key that contains 'shell/<verb>/command' subtree. */
  hr = ASSOC_GetValue(This->hkeySource, NULL, (void**)&filetype, NULL);
  if (hr == S_OK)
  {
      HKEY hkeyFile;

      ret = RegOpenKeyExW(HKEY_CLASSES_ROOT, filetype, 0, KEY_READ, &hkeyFile);
      free(filetype);

      if (ret == ERROR_SUCCESS)
      {
          ret = RegOpenKeyExW(hkeyFile, L"shell", 0, KEY_READ, &hkeyShell);
          RegCloseKey(hkeyFile);
      }
      else
          ret = RegOpenKeyExW(This->hkeySource, L"shell", 0, KEY_READ, &hkeyShell);
  }
  else
      ret = RegOpenKeyExW(This->hkeySource, L"shell", 0, KEY_READ, &hkeyShell);

  if (ret) return HRESULT_FROM_WIN32(ret);

  if (!extra)
  {
      /* check for default verb */
      hr = ASSOC_GetValue(hkeyShell, NULL, (void**)&extra_from_reg, NULL);
      if (FAILED(hr))
      {
          /* no default verb, try first subkey */
          DWORD max_subkey_len;

          ret = RegQueryInfoKeyW(hkeyShell, NULL, NULL, NULL, NULL, &max_subkey_len, NULL, NULL, NULL, NULL, NULL, NULL);
          if (ret)
          {
              RegCloseKey(hkeyShell);
              return HRESULT_FROM_WIN32(ret);
          }

          max_subkey_len++;
          extra_from_reg = malloc(max_subkey_len * sizeof(WCHAR));
          if (!extra_from_reg)
          {
              RegCloseKey(hkeyShell);
              return E_OUTOFMEMORY;
          }

          ret = RegEnumKeyExW(hkeyShell, 0, extra_from_reg, &max_subkey_len, NULL, NULL, NULL, NULL);
          if (ret)
          {
              free(extra_from_reg);
              RegCloseKey(hkeyShell);
              return HRESULT_FROM_WIN32(ret);
          }
      }
      extra = extra_from_reg;
  }

  /* open verb subkey */
  ret = RegOpenKeyExW(hkeyShell, extra, 0, KEY_READ, &hkeyVerb);
  free(extra_from_reg);
  RegCloseKey(hkeyShell);
  if (ret) return HRESULT_FROM_WIN32(ret);

  /* open command subkey */
  ret = RegOpenKeyExW(hkeyVerb, L"command", 0, KEY_READ, &hkeyCommand);
  RegCloseKey(hkeyVerb);
  if (ret) return HRESULT_FROM_WIN32(ret);

  hr = ASSOC_GetValue(hkeyCommand, NULL, (void**)command, NULL);
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

  hr = ASSOC_GetCommand(This, pszExtra, &pszCommand);
  if (FAILED(hr))
    return hr;

  /* cleanup pszCommand */
  if (pszCommand[0] == '"')
  {
    pszStart = pszCommand + 1;
    pszEnd = wcschr(pszStart, '"');
    if (pszEnd)
      *pszEnd = 0;
    *len = SearchPathW(NULL, pszStart, NULL, pathlen, path, NULL);
  }
  else
  {
    pszStart = pszCommand;
    for (pszEnd = pszStart; (pszEnd = wcschr(pszEnd, ' ')); pszEnd++)
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

  free(pszCommand);
  if (!*len)
    return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
  return S_OK;
}

static HRESULT ASSOC_ReturnData(void *out, DWORD *outlen, const void *data,
                                DWORD datalen)
{
  if (out)
  {
    if (*outlen < datalen)
    {
      *outlen = datalen;
      return E_POINTER;
    }
    *outlen = datalen;
    memcpy(out, data, datalen);
    return S_OK;
  }
  else
  {
    *outlen = datalen;
    return S_FALSE;
  }
}

static HRESULT ASSOC_ReturnString(ASSOCF flags, LPWSTR out, DWORD *outlen, LPCWSTR data, DWORD datalen)
{
    HRESULT hr = S_OK;
    DWORD len;

    TRACE("flags=0x%08lx, data=%s\n", flags, debugstr_w(data));

    if (!out)
    {
        *outlen = datalen;
        return S_FALSE;
    }

    if (*outlen < datalen)
    {
        if (flags & ASSOCF_NOTRUNCATE)
        {
            len = 0;
            if (*outlen > 0) out[0] = 0;
            hr = E_POINTER;
        }
        else
        {
            len = min(*outlen, datalen);
            hr = E_NOT_SUFFICIENT_BUFFER;
        }
        *outlen = datalen;
    }
    else
        len = datalen;

    if (len)
        memcpy(out, data, len*sizeof(WCHAR));

    return hr;
}

/**************************************************************************
 *  IQueryAssociations_GetString
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
  ASSOCF flags,
  ASSOCSTR str,
  LPCWSTR pszExtra,
  LPWSTR pszOut,
  DWORD *pcchOut)
{
  IQueryAssociationsImpl *This = impl_from_IQueryAssociations(iface);
  const ASSOCF unimplemented_flags = ~ASSOCF_NOTRUNCATE;
  DWORD len = 0;
  HRESULT hr;
  WCHAR path[MAX_PATH];

  TRACE("(%p)->(0x%08lx, %u, %s, %p, %p)\n", This, flags, str, debugstr_w(pszExtra), pszOut, pcchOut);

  if (flags & unimplemented_flags)
    FIXME("%08lx: unimplemented flags\n", flags & unimplemented_flags);

  if (!pcchOut)
    return E_UNEXPECTED;

  if (!This->hkeySource && !This->hkeyProgID)
    return HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);

  switch (str)
  {
    case ASSOCSTR_COMMAND:
    {
      WCHAR *command;
      hr = ASSOC_GetCommand(This, pszExtra, &command);
      if (SUCCEEDED(hr))
      {
        hr = ASSOC_ReturnString(flags, pszOut, pcchOut, command, lstrlenW(command) + 1);
        free(command);
      }
      return hr;
    }

    case ASSOCSTR_EXECUTABLE:
    {
      hr = ASSOC_GetExecutable(This, pszExtra, path, MAX_PATH, &len);
      if (FAILED(hr))
        return hr;
      len++;
      return ASSOC_ReturnString(flags, pszOut, pcchOut, path, len);
    }

    case ASSOCSTR_FRIENDLYDOCNAME:
    {
      WCHAR *docName;

      hr = ASSOC_GetValue(This->hkeyProgID, NULL, (void**)&docName, NULL);
      if (FAILED(hr)) {
          /* hKeyProgID is NULL or there is no default value, so fail */
          return HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);
      }
      hr = ASSOC_ReturnString(flags, pszOut, pcchOut, docName, lstrlenW(docName) + 1);
      free(docName);
      return hr;
    }

    case ASSOCSTR_FRIENDLYAPPNAME:
    {
      PVOID verinfoW = NULL;
      DWORD size, retval = 0;
      UINT flen;
      WCHAR *bufW;
      WCHAR fileDescW[41];

      hr = ASSOC_GetExecutable(This, pszExtra, path, MAX_PATH, &len);
      if (FAILED(hr))
        return hr;

      retval = GetFileVersionInfoSizeW(path, &size);
      if (!retval)
        goto get_friendly_name_fail;
      verinfoW = calloc(1, retval);
      if (!verinfoW)
        return E_OUTOFMEMORY;
      if (!GetFileVersionInfoW(path, 0, retval, verinfoW))
        goto get_friendly_name_fail;
      if (VerQueryValueW(verinfoW, L"\\VarFileInfo\\Translation", (LPVOID *)&bufW, &flen))
      {
        UINT i;
        DWORD *langCodeDesc = (DWORD *)bufW;
        for (i = 0; i < flen / sizeof(DWORD); i++)
        {
          swprintf(fileDescW, ARRAY_SIZE(fileDescW), L"\\StringFileInfo\\%04x%04x\\FileDescription",
                   LOWORD(langCodeDesc[i]), HIWORD(langCodeDesc[i]));
          if (VerQueryValueW(verinfoW, fileDescW, (LPVOID *)&bufW, &flen))
          {
            /* Does lstrlenW(bufW) == 0 mean we use the filename? */
            len = lstrlenW(bufW) + 1;
            TRACE("found FileDescription: %s\n", debugstr_w(bufW));
            hr = ASSOC_ReturnString(flags, pszOut, pcchOut, bufW, len);
            free(verinfoW);
            return hr;
          }
        }
      }
get_friendly_name_fail:
      PathRemoveExtensionW(path);
      PathStripPathW(path);
      TRACE("using filename: %s\n", debugstr_w(path));
      hr = ASSOC_ReturnString(flags, pszOut, pcchOut, path, lstrlenW(path) + 1);
      free(verinfoW);
      return hr;
    }

    case ASSOCSTR_CONTENTTYPE:
    {
      WCHAR *contentType;
      DWORD ret;
      DWORD size;

      size = 0;
      ret = RegGetValueW(This->hkeySource, NULL, L"Content Type", RRF_RT_REG_SZ, NULL, NULL, &size);
      if (ret != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(ret);
      contentType = malloc(size);
      if (contentType != NULL)
      {
        ret = RegGetValueW(This->hkeySource, NULL, L"Content Type", RRF_RT_REG_SZ, NULL, contentType, &size);
        if (ret == ERROR_SUCCESS)
          hr = ASSOC_ReturnString(flags, pszOut, pcchOut, contentType, lstrlenW(contentType) + 1);
        else
          hr = HRESULT_FROM_WIN32(ret);
        free(contentType);
      }
      else
        hr = E_OUTOFMEMORY;
      return hr;
    }

    case ASSOCSTR_DEFAULTICON:
    {
      DWORD ret;
      DWORD size;

      size = 0;
      ret = RegGetValueW(This->hkeyProgID, L"DefaultIcon", NULL, RRF_RT_REG_SZ, NULL, NULL, &size);
      if (ret == ERROR_SUCCESS)
      {
        WCHAR *icon = malloc(size);
        if (icon)
        {
          ret = RegGetValueW(This->hkeyProgID, L"DefaultIcon", NULL, RRF_RT_REG_SZ, NULL, icon, &size);
          if (ret == ERROR_SUCCESS)
            hr = ASSOC_ReturnString(flags, pszOut, pcchOut, icon, lstrlenW(icon) + 1);
          else
            hr = HRESULT_FROM_WIN32(ret);
          free(icon);
        }
        else
          hr = E_OUTOFMEMORY;
      } else {
          /* there is no DefaultIcon subkey or hkeyProgID is NULL, so return the default document icon */
          if (This->hkeyProgID == NULL)
              hr = ASSOC_ReturnString(flags, pszOut, pcchOut, L"shell32.dll,0", lstrlenW(L"shell32.dll,0") + 1);
          else
              return HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);
      }
      return hr;
    }
    case ASSOCSTR_SHELLEXTENSION:
    {
        WCHAR keypath[ARRAY_SIZE(L"ShellEx\\") + 39], guid[39];
        CLSID clsid;
        HKEY hkey;
        DWORD size;
        LONG ret;

        hr = CLSIDFromString(pszExtra, &clsid);
        if (FAILED(hr)) return hr;

        lstrcpyW(keypath, L"ShellEx\\");
        lstrcatW(keypath, pszExtra);
        ret = RegOpenKeyExW(This->hkeySource, keypath, 0, KEY_READ, &hkey);
        if (ret) return HRESULT_FROM_WIN32(ret);

        size = sizeof(guid);
        ret = RegGetValueW(hkey, NULL, NULL, RRF_RT_REG_SZ, NULL, guid, &size);
        RegCloseKey(hkey);
        if (ret) return HRESULT_FROM_WIN32(ret);

        return ASSOC_ReturnString(flags, pszOut, pcchOut, guid, size / sizeof(WCHAR));
    }

    default:
      FIXME("assocstr %d unimplemented!\n", str);
      return E_NOTIMPL;
  }
}

/**************************************************************************
 *  IQueryAssociations_GetKey
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
  IQueryAssociationsImpl *This = impl_from_IQueryAssociations(iface);

  FIXME("(%p,0x%8lx,0x%8x,%s,%p)-stub!\n", This, cfFlags, assockey,
        debugstr_w(pszExtra), phkeyOut);
  return E_NOTIMPL;
}

/**************************************************************************
 *  IQueryAssociations_GetData
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
static HRESULT WINAPI IQueryAssociations_fnGetData(IQueryAssociations *iface,
        ASSOCF cfFlags, ASSOCDATA assocdata, LPCWSTR pszExtra, LPVOID pvOut,
        DWORD *pcbOut)
{
    IQueryAssociationsImpl *This = impl_from_IQueryAssociations(iface);
    void *data = NULL;
    DWORD size;
    HRESULT hres;

    TRACE("(%p,0x%8lx,0x%8x,%s,%p,%p)\n", This, cfFlags, assocdata,
            debugstr_w(pszExtra), pvOut, pcbOut);

    if(cfFlags)
        FIXME("Unsupported flags: %lx\n", cfFlags);

    switch(assocdata) {
    case ASSOCDATA_EDITFLAGS:
        if(!This->hkeyProgID)
            return HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);

        hres = ASSOC_GetValue(This->hkeyProgID, L"EditFlags", &data, &size);
        if(SUCCEEDED(hres) && pcbOut)
            hres = ASSOC_ReturnData(pvOut, pcbOut, data, size);
        free(data);
        return hres;
    default:
        FIXME("Unsupported ASSOCDATA value: %d\n", assocdata);
        return E_NOTIMPL;
    }
}

/**************************************************************************
 *  IQueryAssociations_GetEnum
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
  IQueryAssociationsImpl *This = impl_from_IQueryAssociations(iface);

  FIXME("(%p,0x%8lx,0x%8x,%s,%s,%p)-stub!\n", This, cfFlags, assocenum,
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
 * IApplicationAssociationRegistration implementation
 */
static inline IApplicationAssociationRegistrationImpl *impl_from_IApplicationAssociationRegistration(IApplicationAssociationRegistration *iface)
{
  return CONTAINING_RECORD(iface, IApplicationAssociationRegistrationImpl, IApplicationAssociationRegistration_iface);
}

static HRESULT WINAPI ApplicationAssociationRegistration_QueryInterface(
                        IApplicationAssociationRegistration* iface, REFIID riid, LPVOID *ppv)
{
    IApplicationAssociationRegistrationImpl *This = impl_from_IApplicationAssociationRegistration(iface);

    TRACE("(%p, %s, %p)\n",This, debugstr_guid(riid), ppv);

    if (ppv == NULL)
        return E_POINTER;

    if (IsEqualGUID(&IID_IUnknown, riid) ||
        IsEqualGUID(&IID_IApplicationAssociationRegistration, riid)) {
        *ppv = &This->IApplicationAssociationRegistration_iface;
        IUnknown_AddRef((IUnknown*)*ppv);
        TRACE("returning IApplicationAssociationRegistration: %p\n", *ppv);
        return S_OK;
    }

    *ppv = NULL;
    FIXME("(%p)->(%s %p) interface not supported\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI ApplicationAssociationRegistration_AddRef(IApplicationAssociationRegistration *iface)
{
    IApplicationAssociationRegistrationImpl *This = impl_from_IApplicationAssociationRegistration(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);
    return ref;
}

static ULONG WINAPI ApplicationAssociationRegistration_Release(IApplicationAssociationRegistration *iface)
{
    IApplicationAssociationRegistrationImpl *This = impl_from_IApplicationAssociationRegistration(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref) {
        SHFree(This);
    }
    return ref;
}

static HRESULT WINAPI ApplicationAssociationRegistration_QueryCurrentDefault(IApplicationAssociationRegistration *iface, LPCWSTR query,
                                                                             ASSOCIATIONTYPE type, ASSOCIATIONLEVEL level, LPWSTR *association)
{
    IApplicationAssociationRegistrationImpl *This = impl_from_IApplicationAssociationRegistration(iface);
    WCHAR path[MAX_PATH];
    DWORD ret, keytype, size;
    HKEY hkey = NULL;
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);

    TRACE("(%p)->(%s, %d, %d, %p)\n", This, debugstr_w(query), type, level, association);

    if(!association)
        return E_INVALIDARG;

    *association = NULL;

    if((type == AT_URLPROTOCOL || type == AT_FILEEXTENSION) && !query[0])
        return E_INVALIDARG;
    else if(type == AT_FILEEXTENSION && query[0] != '.')
        return E_INVALIDARG;

    if(type == AT_FILEEXTENSION)
    {
        ret = RegOpenKeyExW(HKEY_CLASSES_ROOT, query, 0, KEY_READ, &hkey);
        if(ret == ERROR_SUCCESS)
        {
            ret = RegGetValueW(hkey, NULL, NULL, RRF_RT_REG_SZ, &keytype, NULL, &size);
            if(ret == ERROR_SUCCESS)
            {
                *association = CoTaskMemAlloc(size);
                if(*association)
                {
                    ret = RegGetValueW(hkey, NULL, NULL, RRF_RT_REG_SZ, &keytype, *association, &size);
                    if(ret == ERROR_SUCCESS)
                        hr = S_OK;
                    else
                    {
                        CoTaskMemFree(*association);
                        *association = NULL;
                    }
                }
                else
                    hr = E_OUTOFMEMORY;
            }
        }
    }
    else
    {
        ret = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Associations", 0, KEY_READ, &hkey);
        if(ret == ERROR_SUCCESS)
        {
            if(type == AT_URLPROTOCOL)
                swprintf( path, ARRAY_SIZE(path), L"UrlAssociations\\%s\\UserChoice", query );
            else if(type == AT_MIMETYPE)
                swprintf( path, ARRAY_SIZE(path), L"MIMEAssociations\\%s\\UserChoice", query );
            else
            {
                WARN("Unsupported type (%d).\n", type);
                RegCloseKey(hkey);
                return hr;
            }

            ret = RegGetValueW(hkey, path, L"Progid", RRF_RT_REG_SZ, &keytype, NULL, &size);
            if(ret == ERROR_SUCCESS)
            {
                *association = CoTaskMemAlloc(size);
                if(*association)
                {
                    ret = RegGetValueW(hkey, path, L"Progid", RRF_RT_REG_SZ, &keytype, *association, &size);
                    if(ret == ERROR_SUCCESS)
                        hr = S_OK;
                    else
                    {
                        CoTaskMemFree(*association);
                        *association = NULL;
                    }
                }
                else
                    hr = E_OUTOFMEMORY;
            }
        }
    }

    RegCloseKey(hkey);

    return hr;
}

static HRESULT WINAPI ApplicationAssociationRegistration_QueryAppIsDefault(IApplicationAssociationRegistration* This, LPCWSTR query,
                                                                           ASSOCIATIONTYPE type, ASSOCIATIONLEVEL level, LPCWSTR appname, BOOL *is_default)
{
    FIXME("(%p)->(%s, %d, %d, %s, %p)\n", This, debugstr_w(query), type, level, debugstr_w(appname), is_default);
    return E_NOTIMPL;
}

static HRESULT WINAPI ApplicationAssociationRegistration_QueryAppIsDefaultAll(IApplicationAssociationRegistration* This, ASSOCIATIONLEVEL level,
                                                                              LPCWSTR appname, BOOL *is_default)
{
    FIXME("(%p)->(%d, %s, %p)\n", This, level, debugstr_w(appname), is_default);
    return E_NOTIMPL;
}

static HRESULT WINAPI ApplicationAssociationRegistration_SetAppAsDefault(IApplicationAssociationRegistration* This, LPCWSTR appname,
                                                                         LPCWSTR set, ASSOCIATIONTYPE set_type)
{
    FIXME("(%p)->(%s, %s, %d)\n", This, debugstr_w(appname), debugstr_w(set), set_type);
    return E_NOTIMPL;
}

static HRESULT WINAPI ApplicationAssociationRegistration_SetAppAsDefaultAll(IApplicationAssociationRegistration* This, LPCWSTR appname)
{
    FIXME("(%p)->(%s)\n", This, debugstr_w(appname));
    return E_NOTIMPL;
}


static HRESULT WINAPI ApplicationAssociationRegistration_ClearUserAssociations(IApplicationAssociationRegistration* This)
{
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}


static const IApplicationAssociationRegistrationVtbl IApplicationAssociationRegistration_vtbl =
{
    ApplicationAssociationRegistration_QueryInterface,
    ApplicationAssociationRegistration_AddRef,
    ApplicationAssociationRegistration_Release,
    ApplicationAssociationRegistration_QueryCurrentDefault,
    ApplicationAssociationRegistration_QueryAppIsDefault,
    ApplicationAssociationRegistration_QueryAppIsDefaultAll,
    ApplicationAssociationRegistration_SetAppAsDefault,
    ApplicationAssociationRegistration_SetAppAsDefaultAll,
    ApplicationAssociationRegistration_ClearUserAssociations
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
    this->IQueryAssociations_iface.lpVtbl = &IQueryAssociations_vtbl;
    this->ref = 0;
    this->hkeySource = 0;
    this->hkeyProgID = 0;
    if (FAILED(ret = IQueryAssociations_QueryInterface(&this->IQueryAssociations_iface, riid, ppOutput))) SHFree( this );
    TRACE("returning %p\n", *ppOutput);
    return ret;
}

/**************************************************************************
 * ApplicationAssociationRegistration_Constructor [internal]
 *
 * Construct a IApplicationAssociationRegistration object.
 */
HRESULT WINAPI ApplicationAssociationRegistration_Constructor(IUnknown *outer, REFIID riid, LPVOID *ppv)
{
    IApplicationAssociationRegistrationImpl *This;
    HRESULT hr;

    if (outer)
        return CLASS_E_NOAGGREGATION;

    if (!(This = SHAlloc(sizeof(*This))))
        return E_OUTOFMEMORY;

    This->IApplicationAssociationRegistration_iface.lpVtbl = &IApplicationAssociationRegistration_vtbl;
    This->ref = 0;

    hr = IApplicationAssociationRegistration_QueryInterface(&This->IApplicationAssociationRegistration_iface, riid, ppv);
    if (FAILED(hr))
        SHFree(This);

    TRACE("returning 0x%lx with %p\n", hr, *ppv);
    return hr;
}

static HRESULT WINAPI enumassochandlers_QueryInterface(IEnumAssocHandlers *iface, REFIID riid, void **obj)
{
    struct enumassochandlers *This = impl_from_IEnumAssocHandlers(iface);

    TRACE("(%p %s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IEnumAssocHandlers) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IEnumAssocHandlers_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI enumassochandlers_AddRef(IEnumAssocHandlers *iface)
{
    struct enumassochandlers *This = impl_from_IEnumAssocHandlers(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(%lu)\n", This, ref);
    return ref;
}

static ULONG WINAPI enumassochandlers_Release(IEnumAssocHandlers *iface)
{
    struct enumassochandlers *This = impl_from_IEnumAssocHandlers(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%lu)\n", This, ref);

    if (!ref)
        SHFree(This);

    return ref;
}

static HRESULT WINAPI enumassochandlers_Next(IEnumAssocHandlers *iface, ULONG count, IAssocHandler **handlers,
    ULONG *fetched)
{
    struct enumassochandlers *This = impl_from_IEnumAssocHandlers(iface);

    FIXME("(%p)->(%lu %p %p): stub\n", This, count, handlers, fetched);

    return E_NOTIMPL;
}

static const IEnumAssocHandlersVtbl enumassochandlersvtbl = {
    enumassochandlers_QueryInterface,
    enumassochandlers_AddRef,
    enumassochandlers_Release,
    enumassochandlers_Next
};

/**************************************************************************
 * SHCreateAssociationRegistration [SHELL32.@]
 */
HRESULT WINAPI SHCreateAssociationRegistration(REFIID riid, LPVOID *ppv)
{
    return ApplicationAssociationRegistration_Constructor(NULL, riid, ppv);
}

/**************************************************************************
 * SHAssocEnumHandlers            [SHELL32.@]
 */
HRESULT WINAPI SHAssocEnumHandlers(const WCHAR *extra, ASSOC_FILTER filter, IEnumAssocHandlers **enumhandlers)
{
    struct enumassochandlers *enumassoc;

    FIXME("(%s %d %p): stub\n", debugstr_w(extra), filter, enumhandlers);

    *enumhandlers = NULL;

    enumassoc = SHAlloc(sizeof(*enumassoc));
    if (!enumassoc)
        return E_OUTOFMEMORY;

    enumassoc->IEnumAssocHandlers_iface.lpVtbl = &enumassochandlersvtbl;
    enumassoc->ref = 1;

    *enumhandlers = &enumassoc->IEnumAssocHandlers_iface;
    return S_OK;
}

/**************************************************************************
 * SHAssocEnumHandlersForProtocolByApplication            [SHELL32.@]
 */
HRESULT WINAPI SHAssocEnumHandlersForProtocolByApplication(const WCHAR *protocol, REFIID riid, void **handlers)
{
    FIXME("(%s %s %p): stub\n", debugstr_w(protocol), debugstr_guid(riid), handlers);
    return E_NOTIMPL;
}
