/* 
 * IDxDiagProvider Implementation
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
#include "dxdiag_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxdiag);

/* IDxDiagProvider IUnknown parts follow: */
HRESULT WINAPI IDxDiagProviderImpl_QueryInterface(PDXDIAGPROVIDER iface, REFIID riid, LPVOID *ppobj)
{
    ICOM_THIS(IDxDiagProviderImpl,iface);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDxDiagProvider)) {
        IDxDiagProviderImpl_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDxDiagProviderImpl_AddRef(PDXDIAGPROVIDER iface) {
    ICOM_THIS(IDxDiagProviderImpl,iface);
    TRACE("(%p) : AddRef from %ld\n", This, This->ref);
    return ++(This->ref);
}

ULONG WINAPI IDxDiagProviderImpl_Release(PDXDIAGPROVIDER iface) {
    ICOM_THIS(IDxDiagProviderImpl,iface);
    ULONG ref = --This->ref;
    TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
    if (ref == 0) {
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDxDiagProvider Interface follow: */
HRESULT WINAPI IDxDiagProviderImpl_Initialize(PDXDIAGPROVIDER iface, DXDIAG_INIT_PARAMS* pParams) {
  return S_OK;
}

HRESULT WINAPI IDxDiagProviderImpl_GetRootContainer(PDXDIAGPROVIDER iface, IDxDiagContainer** ppInstance) {
  return S_OK;
}

ICOM_VTABLE(IDxDiagProvider) DxDiagProvider_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IDxDiagProviderImpl_QueryInterface,
    IDxDiagProviderImpl_AddRef,
    IDxDiagProviderImpl_Release,
    IDxDiagProviderImpl_Initialize,
    IDxDiagProviderImpl_GetRootContainer
};

HRESULT DXDiag_CreateDXDiagProvider(LPCLASSFACTORY iface, LPUNKNOWN punkOuter, REFIID riid, LPVOID *ppobj) {
  IDxDiagProviderImpl* provider;

  TRACE("(%p, %s, %p)\n", punkOuter, debugstr_guid(riid), ppobj);
  
  provider = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDxDiagProviderImpl));
  if (NULL == provider) {
    *ppobj = NULL;
    return E_OUTOFMEMORY;
  }
  provider->lpVtbl = &DxDiagProvider_Vtbl;
  provider->ref = 0; /* will be inited with QueryInterface */
  return IDxDiagProviderImpl_QueryInterface ((PDXDIAGPROVIDER)provider, riid, ppobj);
}
