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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "objbase.h"
#include "wine/debug.h"

#include "dplay8.h"
#include "dpnet_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dpnet);

/* IDirectPlay8Address IUnknown parts follow: */
HRESULT WINAPI IDirectPlay8AddressImpl_QueryInterface(PDIRECTPLAY8ADDRESS iface, REFIID riid, LPVOID *ppobj)
{
    ICOM_THIS(IDirectPlay8AddressImpl,iface);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirectPlay8Address)) {
        IDirectPlay8AddressImpl_AddRef(iface);
        *ppobj = This;
        return DPN_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirectPlay8AddressImpl_AddRef(PDIRECTPLAY8ADDRESS iface) {
    ICOM_THIS(IDirectPlay8AddressImpl,iface);
    TRACE("(%p) : AddRef from %ld\n", This, This->ref);
    return ++(This->ref);
}

ULONG WINAPI IDirectPlay8AddressImpl_Release(PDIRECTPLAY8ADDRESS iface) {
    ICOM_THIS(IDirectPlay8AddressImpl,iface);
    ULONG ref = --This->ref;
    TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
    if (ref == 0) {
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirectPlay8Address Interface follow: */

HRESULT WINAPI IDirectPlay8AddressImpl_BuildFromURLW(PDIRECTPLAY8ADDRESS iface,  WCHAR* pwszSourceURL) { return DPN_OK; }
HRESULT WINAPI IDirectPlay8AddressImpl_BuildFromURLA(PDIRECTPLAY8ADDRESS iface,  CHAR* pszSourceURL) { return DPN_OK; }
HRESULT WINAPI IDirectPlay8AddressImpl_Duplicate(PDIRECTPLAY8ADDRESS iface,  PDIRECTPLAY8ADDRESS* ppdpaNewAddress) { return DPN_OK; }
HRESULT WINAPI IDirectPlay8AddressImpl_SetEqual(PDIRECTPLAY8ADDRESS iface,  PDIRECTPLAY8ADDRESS pdpaAddress) { return DPN_OK; }
HRESULT WINAPI IDirectPlay8AddressImpl_IsEqual(PDIRECTPLAY8ADDRESS iface,   PDIRECTPLAY8ADDRESS pdpaAddress) { return DPN_OK; }
HRESULT WINAPI IDirectPlay8AddressImpl_Clear(PDIRECTPLAY8ADDRESS iface) { return DPN_OK; }
HRESULT WINAPI IDirectPlay8AddressImpl_GetURLW(PDIRECTPLAY8ADDRESS iface,  WCHAR* pwszURL, PDWORD pdwNumChars) { return DPN_OK; }
HRESULT WINAPI IDirectPlay8AddressImpl_GetURLA(PDIRECTPLAY8ADDRESS iface,  CHAR* pszURL, PDWORD pdwNumChars) { return DPN_OK; }
HRESULT WINAPI IDirectPlay8AddressImpl_GetSP(PDIRECTPLAY8ADDRESS iface,  GUID* pguidSP) { return DPN_OK; }
HRESULT WINAPI IDirectPlay8AddressImpl_GetUserData(PDIRECTPLAY8ADDRESS iface,  LPVOID pvUserData, PDWORD pdwBufferSize) { return DPN_OK; }
HRESULT WINAPI IDirectPlay8AddressImpl_SetSP(PDIRECTPLAY8ADDRESS iface,  CONST GUID* CONST pguidSP) { return DPN_OK; }
HRESULT WINAPI IDirectPlay8AddressImpl_SetUserData(PDIRECTPLAY8ADDRESS iface,  CONST void* CONST pvUserData, CONST DWORD dwDataSize) { return DPN_OK; }
HRESULT WINAPI IDirectPlay8AddressImpl_GetNumComponents(PDIRECTPLAY8ADDRESS iface,  PDWORD pdwNumComponents) { return DPN_OK; }
HRESULT WINAPI IDirectPlay8AddressImpl_GetComponentByName(PDIRECTPLAY8ADDRESS iface,  CONST WCHAR* CONST pwszName, LPVOID pvBuffer, PDWORD pdwBufferSize, PDWORD pdwDataType) { return DPN_OK; }
HRESULT WINAPI IDirectPlay8AddressImpl_GetComponentByIndex(PDIRECTPLAY8ADDRESS iface,  CONST DWORD dwComponentID, WCHAR* pwszName, PDWORD pdwNameLen, void* pvBuffer, PDWORD pdwBufferSize, PDWORD pdwDataType) { return DPN_OK; }
HRESULT WINAPI IDirectPlay8AddressImpl_AddComponent(PDIRECTPLAY8ADDRESS iface,  CONST WCHAR* CONST pwszName, CONST void* CONST lpvData, CONST DWORD dwDataSize, CONST DWORD dwDataType) { return DPN_OK; }
HRESULT WINAPI IDirectPlay8AddressImpl_GetDevice(PDIRECTPLAY8ADDRESS iface,  GUID* pDevGuid) { return DPN_OK; }
HRESULT WINAPI IDirectPlay8AddressImpl_SetDevice(PDIRECTPLAY8ADDRESS iface,  CONST GUID* CONST devGuid) { return DPN_OK; }
HRESULT WINAPI IDirectPlay8AddressImpl_BuildFromDirectPlay4Address(PDIRECTPLAY8ADDRESS iface,  LPVOID pvAddress, DWORD dwDataSize) { return DPN_OK; }

ICOM_VTABLE(IDirectPlay8Address) DirectPlay8Address_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
