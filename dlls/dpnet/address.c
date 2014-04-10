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

static inline IDirectPlay8AddressImpl *impl_from_IDirectPlay8Address(IDirectPlay8Address *iface)
{
    return CONTAINING_RECORD(iface, IDirectPlay8AddressImpl, IDirectPlay8Address_iface);
}

/* IDirectPlay8Address IUnknown parts follow: */
static HRESULT WINAPI IDirectPlay8AddressImpl_QueryInterface(IDirectPlay8Address *iface,
        REFIID riid, void **ppv)
{
    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectPlay8Address)) {
        IUnknown_AddRef(iface);
        *ppv = iface;
        return DPN_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirectPlay8AddressImpl_AddRef(IDirectPlay8Address *iface)
{
    IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IDirectPlay8AddressImpl_Release(IDirectPlay8Address *iface)
{
    IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n", This, refCount + 1);

    if (!refCount) {
        HeapFree(GetProcessHeap(), 0, This);
    }
    return refCount;
}

/* returns name of given GUID */
static const char *debugstr_SP(const GUID *id) {
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
    if (IsEqualGUID(id, guids[i].guid))
      return guids[i].name;
  }
  /* if we didn't find it, act like standard debugstr_guid */
  return debugstr_guid(id);
}

/* IDirectPlay8Address Interface follow: */

static HRESULT WINAPI IDirectPlay8AddressImpl_BuildFromURLW(IDirectPlay8Address *iface,
        WCHAR *pwszSourceURL)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p, %s): stub\n", This, debugstr_w(pwszSourceURL));
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_BuildFromURLA(IDirectPlay8Address *iface,
       CHAR *pszSourceURL)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p, %s): stub\n", This, pszSourceURL);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_Duplicate(IDirectPlay8Address *iface,
        IDirectPlay8Address **ppdpaNewAddress)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p, %p): stub\n", This, ppdpaNewAddress);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_SetEqual(IDirectPlay8Address *iface,
        IDirectPlay8Address *pdpaAddress)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p, %p): stub\n", This, pdpaAddress);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_IsEqual(IDirectPlay8Address *iface,
        IDirectPlay8Address *pdpaAddress)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p, %p): stub\n", This, pdpaAddress);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_Clear(IDirectPlay8Address *iface)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetURLW(IDirectPlay8Address *iface, WCHAR *pwszURL,
        DWORD *pdwNumChars)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetURLA(IDirectPlay8Address *iface, CHAR *pszURL,
        DWORD *pdwNumChars)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetSP(IDirectPlay8Address *iface, GUID *pguidSP)
{
    IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);

    TRACE("(%p, %p)\n", iface, pguidSP);

    if(!pguidSP)
        return DPNERR_INVALIDPOINTER;

    if(!This->init)
        return DPNERR_DOESNOTEXIST;

    *pguidSP = This->SP_guid;
    return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetUserData(IDirectPlay8Address *iface,
        void *pvUserData, DWORD *pdwBufferSize)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_SetSP(IDirectPlay8Address *iface,
        const GUID *const pguidSP)
{
    IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);

    TRACE("(%p, %s)\n", iface, debugstr_SP(pguidSP));

    if(!pguidSP)
        return DPNERR_INVALIDPOINTER;

    This->init = TRUE;
    This->SP_guid = *pguidSP;
    return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8AddressImpl_SetUserData(IDirectPlay8Address *iface,
        const void *const pvUserData, const DWORD dwDataSize)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetNumComponents(IDirectPlay8Address *iface,
        DWORD *pdwNumComponents)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetComponentByName(IDirectPlay8Address *iface,
        const WCHAR *const pwszName, void *pvBuffer, DWORD *pdwBufferSize, DWORD *pdwDataType)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetComponentByIndex(IDirectPlay8Address *iface,
        const DWORD dwComponentID, WCHAR *pwszName, DWORD *pdwNameLen, void *pvBuffer,
        DWORD *pdwBufferSize, DWORD *pdwDataType)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_AddComponent(IDirectPlay8Address *iface,
        const WCHAR *const pwszName, const void* const lpvData, const DWORD dwDataSize,
        const DWORD dwDataType)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p, %s, %p, %u, %x): stub\n", This, debugstr_w(pwszName), lpvData, dwDataSize, dwDataType);
  
  if (NULL == lpvData) return DPNERR_INVALIDPOINTER;
  switch (dwDataType) {
  case DPNA_DATATYPE_DWORD:
    if (sizeof(DWORD) != dwDataSize) return DPNERR_INVALIDPARAM;
    TRACE("(%p, %u): DWORD Type -> %u\n", lpvData, dwDataSize, *(const DWORD*) lpvData);
    break;
  case DPNA_DATATYPE_GUID:
    if (sizeof(GUID) != dwDataSize) return DPNERR_INVALIDPARAM;
    TRACE("(%p, %u): GUID Type -> %s\n", lpvData, dwDataSize, debugstr_guid(lpvData));
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

static HRESULT WINAPI IDirectPlay8AddressImpl_GetDevice(IDirectPlay8Address *iface, GUID *pDevGuid) {
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_SetDevice(IDirectPlay8Address *iface,
        const GUID *const devGuid)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p, %s): stub\n", This, debugstr_guid(devGuid));
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_BuildFromDirectPlay4Address(IDirectPlay8Address *iface,
        void *pvAddress, DWORD dwDataSize)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
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
  client->IDirectPlay8Address_iface.lpVtbl = &DirectPlay8Address_Vtbl;
  client->ref = 0; /* will be inited with QueryInterface */
  return IDirectPlay8AddressImpl_QueryInterface (&client->IDirectPlay8Address_iface, riid, ppobj);
}
