/* 
 * DirectPlay8 Address
 * 
 * Copyright 2004 Raphael Junqueira
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
 *
 */

#include "config.h"

#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "objbase.h"
#include "wine/debug.h"

#include "dplay8.h"
#include "dpnet_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dpnet);

/* IDirectPlay8Address IUnknown parts follow: */
static HRESULT WINAPI IDirectPlay8AddressImpl_QueryInterface(PDIRECTPLAY8ADDRESS iface, REFIID riid, LPVOID *ppobj)
{
    IDirectPlay8AddressImpl *This = (IDirectPlay8AddressImpl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirectPlay8Address)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return DPN_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirectPlay8AddressImpl_AddRef(PDIRECTPLAY8ADDRESS iface) {
    IDirectPlay8AddressImpl *This = (IDirectPlay8AddressImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IDirectPlay8AddressImpl_Release(PDIRECTPLAY8ADDRESS iface) {
    IDirectPlay8AddressImpl *This = (IDirectPlay8AddressImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n", This, refCount + 1);

    if (!refCount) {
        HeapFree(GetProcessHeap(), 0, This);
    }
    return refCount;
}

/* IDirectPlay8Address Interface follow: */

static HRESULT WINAPI IDirectPlay8AddressImpl_BuildFromURLW(PDIRECTPLAY8ADDRESS iface, WCHAR* pwszSourceURL) { 
  IDirectPlay8AddressImpl *This = (IDirectPlay8AddressImpl *)iface;
  TRACE("(%p, %s): stub\n", This, debugstr_w(pwszSourceURL));
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_BuildFromURLA(PDIRECTPLAY8ADDRESS iface, CHAR* pszSourceURL) { 
  IDirectPlay8AddressImpl *This = (IDirectPlay8AddressImpl *)iface;
  TRACE("(%p, %s): stub\n", This, pszSourceURL);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_Duplicate(PDIRECTPLAY8ADDRESS iface, PDIRECTPLAY8ADDRESS* ppdpaNewAddress) { 
  IDirectPlay8AddressImpl *This = (IDirectPlay8AddressImpl *)iface;
  TRACE("(%p, %p): stub\n", This, ppdpaNewAddress);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_SetEqual(PDIRECTPLAY8ADDRESS iface, PDIRECTPLAY8ADDRESS pdpaAddress) { 
  IDirectPlay8AddressImpl *This = (IDirectPlay8AddressImpl *)iface;
  TRACE("(%p, %p): stub\n", This, pdpaAddress);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_IsEqual(PDIRECTPLAY8ADDRESS iface, PDIRECTPLAY8ADDRESS pdpaAddress) { 
  IDirectPlay8AddressImpl *This = (IDirectPlay8AddressImpl *)iface;
  TRACE("(%p, %p): stub\n", This, pdpaAddress);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_Clear(PDIRECTPLAY8ADDRESS iface) { 
  IDirectPlay8AddressImpl *This = (IDirectPlay8AddressImpl *)iface;
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetURLW(PDIRECTPLAY8ADDRESS iface, WCHAR* pwszURL, PDWORD pdwNumChars) { 
  IDirectPlay8AddressImpl *This = (IDirectPlay8AddressImpl *)iface;
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetURLA(PDIRECTPLAY8ADDRESS iface, CHAR* pszURL, PDWORD pdwNumChars) { 
  IDirectPlay8AddressImpl *This = (IDirectPlay8AddressImpl *)iface;
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetSP(PDIRECTPLAY8ADDRESS iface, GUID* pguidSP) { 
  IDirectPlay8AddressImpl *This = (IDirectPlay8AddressImpl *)iface;
  TRACE("(%p, %p)\n", iface, pguidSP);
  memcpy(pguidSP, &This->SP_guid, sizeof(GUID));
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetUserData(PDIRECTPLAY8ADDRESS iface, LPVOID pvUserData, PDWORD pdwBufferSize) { 
  IDirectPlay8AddressImpl *This = (IDirectPlay8AddressImpl *)iface;
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_SetSP(PDIRECTPLAY8ADDRESS iface, CONST GUID* CONST pguidSP) { 
  IDirectPlay8AddressImpl *This = (IDirectPlay8AddressImpl *)iface;
  TRACE("(%p, %s)\n", iface, debugstr_SP(pguidSP));
  memcpy(&This->SP_guid, pguidSP, sizeof(GUID));
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_SetUserData(PDIRECTPLAY8ADDRESS iface, CONST void* CONST pvUserData, CONST DWORD dwDataSize) { 
  IDirectPlay8AddressImpl *This = (IDirectPlay8AddressImpl *)iface;
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetNumComponents(PDIRECTPLAY8ADDRESS iface, PDWORD pdwNumComponents) { 
  IDirectPlay8AddressImpl *This = (IDirectPlay8AddressImpl *)iface;
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetComponentByName(PDIRECTPLAY8ADDRESS iface, CONST WCHAR* CONST pwszName, LPVOID pvBuffer, PDWORD pdwBufferSize, PDWORD pdwDataType) { 
  IDirectPlay8AddressImpl *This = (IDirectPlay8AddressImpl *)iface;
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetComponentByIndex(PDIRECTPLAY8ADDRESS iface, CONST DWORD dwComponentID, WCHAR* pwszName, 
							   PDWORD pdwNameLen, void* pvBuffer, PDWORD pdwBufferSize, PDWORD pdwDataType) { 
  IDirectPlay8AddressImpl *This = (IDirectPlay8AddressImpl *)iface;
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_AddComponent(PDIRECTPLAY8ADDRESS iface, CONST WCHAR* CONST pwszName, 
						    CONST void* CONST lpvData, CONST DWORD dwDataSize, CONST DWORD dwDataType) { 
  IDirectPlay8AddressImpl *This = (IDirectPlay8AddressImpl *)iface;
  TRACE("(%p, %s, %p, %u, %x): stub\n", This, debugstr_w(pwszName), lpvData, dwDataSize, dwDataType);
  
  if (NULL == lpvData) return DPNERR_INVALIDPOINTER;
  switch (dwDataType) {
  case DPNA_DATATYPE_DWORD:
    if (sizeof(DWORD) != dwDataSize) return DPNERR_INVALIDPARAM;
    TRACE("(%p, %u): DWORD Type -> %u\n", lpvData, dwDataSize, *(const DWORD*) lpvData);
    break;
  case DPNA_DATATYPE_GUID:
    if (sizeof(GUID) != dwDataSize) return DPNERR_INVALIDPARAM;
    TRACE("(%p, %u): GUID Type -> %s\n", lpvData, dwDataSize, debugstr_guid((const GUID*) lpvData));
    break;
  case DPNA_DATATYPE_STRING:
    TRACE("(%p, %u): STRING Type -> %s\n", lpvData, dwDataSize, (const CHAR*) lpvData);
    break;
  case DPNA_DATATYPE_BINARY:
    TRACE("(%p, %u): BINARY Type\n", lpvData, dwDataSize);
    break;
  }
  
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetDevice(PDIRECTPLAY8ADDRESS iface, GUID* pDevGuid) {   
  IDirectPlay8AddressImpl *This = (IDirectPlay8AddressImpl *)iface;
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_SetDevice(PDIRECTPLAY8ADDRESS iface, CONST GUID* CONST devGuid) { 
  IDirectPlay8AddressImpl *This = (IDirectPlay8AddressImpl *)iface;
  TRACE("(%p, %s): stub\n", This, debugstr_guid(devGuid));
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_BuildFromDirectPlay4Address(PDIRECTPLAY8ADDRESS iface, LPVOID pvAddress, DWORD dwDataSize) { 
  IDirectPlay8AddressImpl *This = (IDirectPlay8AddressImpl *)iface;
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static const IDirectPlay8AddressVtbl DirectPlay8Address_Vtbl =
{
    IDirectPlay8AddressImpl_QueryInterface,
    IDirectPlay8AddressImpl_AddRef,
    IDirectPlay8AddressImpl_Release,
    IDirectPlay8AddressImpl_BuildFromURLW,
    IDirectPlay8AddressImpl_BuildFromURLA,
    IDirectPlay8AddressImpl_Duplicate,
    IDirectPlay8AddressImpl_SetEqual,
    IDirectPlay8AddressImpl_IsEqual,
    IDirectPlay8AddressImpl_Clear,
    IDirectPlay8AddressImpl_GetURLW,
    IDirectPlay8AddressImpl_GetURLA,
    IDirectPlay8AddressImpl_GetSP,
    IDirectPlay8AddressImpl_GetUserData,
    IDirectPlay8AddressImpl_SetSP,
    IDirectPlay8AddressImpl_SetUserData,
    IDirectPlay8AddressImpl_GetNumComponents,
    IDirectPlay8AddressImpl_GetComponentByName,
    IDirectPlay8AddressImpl_GetComponentByIndex,
    IDirectPlay8AddressImpl_AddComponent,
    IDirectPlay8AddressImpl_GetDevice,
    IDirectPlay8AddressImpl_SetDevice,
    IDirectPlay8AddressImpl_BuildFromDirectPlay4Address
};

HRESULT DPNET_CreateDirectPlay8Address(LPCLASSFACTORY iface, LPUNKNOWN punkOuter, REFIID riid, LPVOID *ppobj) {
  IDirectPlay8AddressImpl* client;

  TRACE("(%p, %s, %p)\n", punkOuter, debugstr_guid(riid), ppobj);
  
  client = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectPlay8AddressImpl));
  if (NULL == client) {
    *ppobj = NULL;
    return E_OUTOFMEMORY;
  }
  client->lpVtbl = &DirectPlay8Address_Vtbl;
  client->ref = 0; /* will be inited with QueryInterface */
  return IDirectPlay8AddressImpl_QueryInterface ((PDIRECTPLAY8ADDRESS)client, riid, ppobj);
}

/* returns name of given GUID */
const char *debugstr_SP(const GUID *id) {
  static const guid_info guids[] = {
    /* CLSIDs */
    GE(CLSID_DP8SP_IPX),
    GE(CLSID_DP8SP_TCPIP),
    GE(CLSID_DP8SP_SERIAL),
    GE(CLSID_DP8SP_MODEM)
  };      
  unsigned int i;

  if (!id) return "(null)";
  
  for (i = 0; i < sizeof(guids)/sizeof(guids[0]); i++) {
    if (IsEqualGUID(id, &guids[i].guid))
      return guids[i].name;
  }
  /* if we didn't find it, act like standard debugstr_guid */	
  return debugstr_guid(id);
}
