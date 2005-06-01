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
    IDxDiagProviderImpl *This = (IDxDiagProviderImpl *)iface;

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
    IDxDiagProviderImpl *This = (IDxDiagProviderImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before=%lu)\n", This, refCount - 1);

    DXDIAGN_LockModule();

    return refCount;
}

ULONG WINAPI IDxDiagProviderImpl_Release(PDXDIAGPROVIDER iface) {
    IDxDiagProviderImpl *This = (IDxDiagProviderImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before=%lu)\n", This, refCount + 1);

    if (!refCount) {
        HeapFree(GetProcessHeap(), 0, This);
    }

    DXDIAGN_UnlockModule();
    
    return refCount;
}

/* IDxDiagProvider Interface follow: */
HRESULT WINAPI IDxDiagProviderImpl_Initialize(PDXDIAGPROVIDER iface, DXDIAG_INIT_PARAMS* pParams) {
    IDxDiagProviderImpl *This = (IDxDiagProviderImpl *)iface;
    TRACE("(%p,%p)\n", iface, pParams);

    if (NULL == pParams) {
      return E_POINTER;
    }
    if (pParams->dwSize != sizeof(DXDIAG_INIT_PARAMS)) {
      return E_INVALIDARG;
    }

    This->init = TRUE;
    memcpy(&This->params, pParams, pParams->dwSize);
    return S_OK;
}

HRESULT WINAPI IDxDiagProviderImpl_GetRootContainer(PDXDIAGPROVIDER iface, IDxDiagContainer** ppInstance) {
  HRESULT hr = S_OK;
  IDxDiagProviderImpl *This = (IDxDiagProviderImpl *)iface;
  TRACE("(%p,%p)\n", iface, ppInstance);

  if (NULL == ppInstance) {
    return E_INVALIDARG;
  }
  if (FALSE == This->init) {
    return E_INVALIDARG; /* should be E_CO_UNINITIALIZED */
  }
  if (NULL == This->pRootContainer) {
    hr = DXDiag_CreateDXDiagContainer(&IID_IDxDiagContainer, (void**) &This->pRootContainer);
    if (FAILED(hr)) {
      return hr;
    }
    hr = DXDiag_InitRootDXDiagContainer((PDXDIAGCONTAINER)This->pRootContainer);
  }
  return IDxDiagContainerImpl_QueryInterface((PDXDIAGCONTAINER)This->pRootContainer, &IID_IDxDiagContainer, (void**) ppInstance);
}

static const IDxDiagProviderVtbl DxDiagProvider_Vtbl =
{
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

HRESULT DXDiag_InitRootDXDiagContainer(IDxDiagContainer* pRootCont) {
  static const WCHAR DxDiag_SystemInfo[] = {'D','x','D','i','a','g','_','S','y','s','t','e','m','I','n','f','o',0};
  static const WCHAR dwDirectXVersionMajor[] = {'d','w','D','i','r','e','c','t','X','V','e','r','s','i','o','n','M','a','j','o','r',0};
  static const WCHAR dwDirectXVersionMinor[] = {'d','w','D','i','r','e','c','t','X','V','e','r','s','i','o','n','M','i','n','o','r',0};
  static const WCHAR szDirectXVersionLetter[] = {'s','z','D','i','r','e','c','t','X','V','e','r','s','i','o','n','L','e','t','t','e','r',0};
  static const WCHAR szDirectXVersionLetter_v[] = {'c',0};

  IDxDiagContainer* pSubCont = NULL;
  VARIANT v;
  HRESULT hr = S_OK;
  
  TRACE("(%p)\n", pRootCont);

  hr = DXDiag_CreateDXDiagContainer(&IID_IDxDiagContainer, (void**) &pSubCont);
  if (FAILED(hr)) {
    return hr;
  }
  V_VT(&v) = VT_UI4; V_UI4(&v) = 9;
  hr = IDxDiagContainerImpl_AddProp(pSubCont, dwDirectXVersionMajor, &v);
  V_VT(&v) = VT_UI4; V_UI4(&v) = 0;
  hr = IDxDiagContainerImpl_AddProp(pSubCont, dwDirectXVersionMinor, &v);
  V_VT(&v) = VT_BSTR; V_BSTR(&v) = SysAllocString(szDirectXVersionLetter_v);
  hr = IDxDiagContainerImpl_AddProp(pSubCont, szDirectXVersionLetter, &v);

  hr = IDxDiagContainerImpl_AddChildContainer(pRootCont, DxDiag_SystemInfo, pSubCont);
  return hr;
}
