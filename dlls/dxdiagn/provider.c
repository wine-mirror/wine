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

HRESULT DXDiag_InitDXDiagSystemInfoContainer(IDxDiagContainer* pSubCont) {
  HRESULT hr = S_OK;
  static const WCHAR dwDirectXVersionMajor[] = {'d','w','D','i','r','e','c','t','X','V','e','r','s','i','o','n','M','a','j','o','r',0};
  static const WCHAR dwDirectXVersionMinor[] = {'d','w','D','i','r','e','c','t','X','V','e','r','s','i','o','n','M','i','n','o','r',0};
  static const WCHAR szDirectXVersionLetter[] = {'s','z','D','i','r','e','c','t','X','V','e','r','s','i','o','n','L','e','t','t','e','r',0};
  static const WCHAR szDirectXVersionLetter_v[] = {'c',0};
  static const WCHAR bDebug[] = {'b','D','e','b','u','g',0};
  static const WCHAR szDirectXVersionEnglish[] = {'s','z','D','i','r','e','c','t','X','V','e','r','s','i','o','n','E','n','g','l','i','s','h',0};
  static const WCHAR szDirectXVersionEnglish_v[] = {'4','.','0','9','.','0','0','0','0','.','0','9','0','4',0};
  static const WCHAR szDirectXVersionLongEnglish[] = {'s','z','D','i','r','e','c','t','X','V','e','r','s','i','o','n','L','o','n','g','E','n','g','l','i','s','h',0};
  static const WCHAR szDirectXVersionLongEnglish_v[] = {'=',' ','"','D','i','r','e','c','t','X',' ','9','.','0','c',' ','(','4','.','0','9','.','0','0','0','0','.','0','9','0','4',')',0};
  /*static const WCHAR szDxDiagVersion[] = {'s','z','D','x','D','i','a','g','V','e','r','s','i','o','n',0};*/
  /*szWindowsDir*/
  /*szWindowsDir*/
  /*"dwOSMajorVersion"*/
  /*"dwOSMinorVersion"*/
  /*"dwOSBuildNumber"*/
  /*"dwOSPlatformID"*/
  VARIANT v;

  V_VT(&v) = VT_UI4; V_UI4(&v) = 9;
  hr = IDxDiagContainerImpl_AddProp(pSubCont, dwDirectXVersionMajor, &v);
  V_VT(&v) = VT_UI4; V_UI4(&v) = 0;
  hr = IDxDiagContainerImpl_AddProp(pSubCont, dwDirectXVersionMinor, &v);
  V_VT(&v) = VT_BSTR; V_BSTR(&v) = SysAllocString(szDirectXVersionLetter_v);
  hr = IDxDiagContainerImpl_AddProp(pSubCont, szDirectXVersionLetter, &v);
  V_VT(&v) = VT_BSTR; V_BSTR(&v) = SysAllocString(szDirectXVersionEnglish_v);
  hr = IDxDiagContainerImpl_AddProp(pSubCont, szDirectXVersionEnglish, &v);
  V_VT(&v) = VT_BSTR; V_BSTR(&v) = SysAllocString(szDirectXVersionLongEnglish);
  hr = IDxDiagContainerImpl_AddProp(pSubCont, szDirectXVersionLongEnglish_v, &v);
  V_VT(&v) = VT_BOOL; V_BOOL(&v) = FALSE;
  hr = IDxDiagContainerImpl_AddProp(pSubCont, bDebug, &v);

  return hr;
}

HRESULT DXDiag_InitDXDiagSystemDevicesContainer(IDxDiagContainer* pSubCont) {
  HRESULT hr = S_OK;
  /*
  static const WCHAR szDescription[] = {'s','z','D','e','s','c','r','i','p','t','i','o','n',0};
  static const WCHAR szDeviceID[] = {'s','z','D','e','v','i','c','e','I','D',0};
  VARIANT v;
  IDxDiagContainer* pDeviceSubCont = NULL;
  
  hr = DXDiag_CreateDXDiagContainer(&IID_IDxDiagContainer, (void**) &pDeviceSubCont);
  if (FAILED(hr)) { return hr; }
  V_VT(pvarProp) = VT_BSTR;
  V_BSTR(pvarProp) = SysAllocString(property->psz);
  hr = IDxDiagContainerImpl_AddProp(pDeviceSubCont, szDescription, &v);
  V_VT(pvarProp) = VT_BSTR;
  V_BSTR(pvarProp) = SysAllocString(property->psz);
  hr = IDxDiagContainerImpl_AddProp(pDeviceSubCont, szDeviceID, &v);

  hr = IDxDiagContainerImpl_AddChildContainer(pSubCont, "", pDeviceSubCont);
  */
  return hr;
}

HRESULT DXDiag_InitDXDiagLogicalDisksContainer(IDxDiagContainer* pSubCont) {
  HRESULT hr = S_OK;
  /*
  static const WCHAR szDriveLetter[] = {'s','z','D','r','i','v','e','L','e','t','t','e','r',0};
  static const WCHAR szFreeSpace[] = {'s','z','F','r','e','e','S','p','a','c','e',0};
  static const WCHAR szMaxSpace[] = {'s','z','M','a','x','S','p','a','c','e',0};
  static const WCHAR szFileSystem[] = {'s','z','F','i','l','e','S','y','s','t','e','m',0};
  static const WCHAR szModel[] = {'s','z','M','o','d','e','l',0};
  static const WCHAR szPNPDeviceID[] = {'s','z','P','N','P','D','e','v','i','c','e','I','D',0};
  static const WCHAR dwHardDriveIndex[] = {'d','w','H','a','r','d','D','r','i','v','e','I','n','d','e','x',0};
  VARIANT v;
  IDxDiagContainer* pDiskSubCont = NULL;

  hr = DXDiag_CreateDXDiagContainer(&IID_IDxDiagContainer, (void**) &pDiskSubCont);
  if (FAILED(hr)) { return hr; }
  hr = IDxDiagContainerImpl_AddChildContainer(pSubCont, "" , pDiskSubCont);
  */
  return hr;
}
HRESULT DXDiag_InitDXDiagDirectXFilesContainer(IDxDiagContainer* pSubCont) {
  HRESULT hr = S_OK;
  /*
  static const WCHAR szName[] = {'s','z','N','a','m','e',0};
  static const WCHAR szVersion[] = {'s','z','V','e','r','s','i','o','n',0};
  VARIANT v;
  */
  return hr;
}

HRESULT DXDiag_InitDXDiagDirectSoundContainer(IDxDiagContainer* pSubCont) {
  HRESULT hr = S_OK;
  static const WCHAR DxDiag_SoundDevices[] = {'D','x','D','i','a','g','_','S','o','u','n','d','D','e','v','i','c','e','s',0};
  static const WCHAR DxDiag_SoundCaptureDevices[] = {'D','x','D','i','a','g','_','S','o','u','n','d','C','a','p','t','u','r','e','D','e','v','i','c','e','s',0};
  IDxDiagContainer* pSubSubCont = NULL;

  hr = DXDiag_CreateDXDiagContainer(&IID_IDxDiagContainer, (void**) &pSubSubCont);
  if (FAILED(hr)) { return hr; }
  hr = IDxDiagContainerImpl_AddChildContainer(pSubCont, DxDiag_SoundDevices, pSubSubCont);

  hr = DXDiag_CreateDXDiagContainer(&IID_IDxDiagContainer, (void**) &pSubSubCont);
  if (FAILED(hr)) { return hr; }
  hr = IDxDiagContainerImpl_AddChildContainer(pSubCont, DxDiag_SoundCaptureDevices, pSubSubCont);

  return hr;
}
HRESULT DXDiag_InitRootDXDiagContainer(IDxDiagContainer* pRootCont) {
  HRESULT hr = S_OK;
  static const WCHAR DxDiag_SystemInfo[] = {'D','x','D','i','a','g','_','S','y','s','t','e','m','I','n','f','o',0};
  static const WCHAR DxDiag_SystemDevices[] = {'D','x','D','i','a','g','_','S','y','s','t','e','m','D','e','v','i','c','e','s',0};
  static const WCHAR DxDiag_LogicalDisks[] = {'D','x','D','i','a','g','_','L','o','g','i','c','a','l','D','i','s','k','s',0};
  static const WCHAR DxDiag_DirectXFiles[] = {'D','x','D','i','a','g','_','D','i','r','e','c','t','X','F','i','l','e','s',0};
  static const WCHAR DxDiag_DisplayDevices[] = {'D','x','D','i','a','g','_','D','i','s','p','l','a','y','D','e','v','i','c','e','s',0};
  static const WCHAR DxDiag_DirectSound[] = {'D','x','D','i','a','g','_','D','i','r','e','c','t','S','o','u','n','d',0};
  static const WCHAR DxDiag_DirectMusic[] = {'D','x','D','i','a','g','_','D','i','r','e','c','t','M','u','s','i','c',0};
  static const WCHAR DxDiag_DirectInput[] = {'D','x','D','i','a','g','_','D','i','r','e','c','t','I','n','p','u','t',0};
  static const WCHAR DxDiag_DirectPlay[] = {'D','x','D','i','a','g','_','D','i','r','e','c','t','P','l','a','y',0};
  static const WCHAR DxDiag_DirectShowFilters[] = {'D','x','D','i','a','g','_','D','i','r','e','c','t','S','h','o','w','F','i','l','t','e','r','s',0};
  IDxDiagContainer* pSubCont = NULL;
  
  TRACE("(%p)\n", pRootCont);

  hr = DXDiag_CreateDXDiagContainer(&IID_IDxDiagContainer, (void**) &pSubCont);
  if (FAILED(hr)) { return hr; }
  hr = DXDiag_InitDXDiagSystemInfoContainer(pSubCont);
  hr = IDxDiagContainerImpl_AddChildContainer(pRootCont, DxDiag_SystemInfo, pSubCont);

  hr = DXDiag_CreateDXDiagContainer(&IID_IDxDiagContainer, (void**) &pSubCont);
  if (FAILED(hr)) { return hr; }
  hr = DXDiag_InitDXDiagSystemDevicesContainer(pSubCont);
  hr = IDxDiagContainerImpl_AddChildContainer(pRootCont, DxDiag_SystemDevices, pSubCont);

  hr = DXDiag_CreateDXDiagContainer(&IID_IDxDiagContainer, (void**) &pSubCont);
  if (FAILED(hr)) { return hr; }
  hr = DXDiag_InitDXDiagLogicalDisksContainer(pSubCont);
  hr = IDxDiagContainerImpl_AddChildContainer(pRootCont, DxDiag_LogicalDisks, pSubCont);

  hr = DXDiag_CreateDXDiagContainer(&IID_IDxDiagContainer, (void**) &pSubCont);
  if (FAILED(hr)) { return hr; }
  hr = DXDiag_InitDXDiagDirectXFilesContainer(pSubCont);
  hr = IDxDiagContainerImpl_AddChildContainer(pRootCont, DxDiag_DirectXFiles, pSubCont);

  hr = DXDiag_CreateDXDiagContainer(&IID_IDxDiagContainer, (void**) &pSubCont);
  if (FAILED(hr)) { return hr; }
  hr = IDxDiagContainerImpl_AddChildContainer(pRootCont, DxDiag_DisplayDevices, pSubCont);

  hr = DXDiag_CreateDXDiagContainer(&IID_IDxDiagContainer, (void**) &pSubCont);
  if (FAILED(hr)) { return hr; }
  hr = DXDiag_InitDXDiagDirectSoundContainer(pSubCont);
  hr = IDxDiagContainerImpl_AddChildContainer(pRootCont, DxDiag_DirectSound, pSubCont);

  hr = DXDiag_CreateDXDiagContainer(&IID_IDxDiagContainer, (void**) &pSubCont);
  if (FAILED(hr)) { return hr; }
  hr = IDxDiagContainerImpl_AddChildContainer(pRootCont, DxDiag_DirectMusic, pSubCont);

  hr = DXDiag_CreateDXDiagContainer(&IID_IDxDiagContainer, (void**) &pSubCont);
  if (FAILED(hr)) { return hr; }
  hr = IDxDiagContainerImpl_AddChildContainer(pRootCont, DxDiag_DirectInput, pSubCont);

  hr = DXDiag_CreateDXDiagContainer(&IID_IDxDiagContainer, (void**) &pSubCont);
  if (FAILED(hr)) { return hr; }
  hr = IDxDiagContainerImpl_AddChildContainer(pRootCont, DxDiag_DirectPlay, pSubCont);

  hr = DXDiag_CreateDXDiagContainer(&IID_IDxDiagContainer, (void**) &pSubCont);
  if (FAILED(hr)) { return hr; }
  hr = IDxDiagContainerImpl_AddChildContainer(pRootCont, DxDiag_DirectShowFilters, pSubCont);

  return hr;
}
