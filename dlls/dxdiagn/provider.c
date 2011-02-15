/* 
 * IDxDiagProvider Implementation
 * 
 * Copyright 2004-2005 Raphael Junqueira
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

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "dxdiag_private.h"
#include "wine/unicode.h"
#include "winver.h"
#include "objidl.h"
#include "dshow.h"
#include "vfw.h"
#include "mmddk.h"
#include "ddraw.h"
#include "d3d9.h"
#include "strmif.h"
#include "initguid.h"
#include "fil_data.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxdiag);

static HRESULT build_information_tree(IDxDiagContainerImpl_Container **pinfo_root);
static void free_information_tree(IDxDiagContainerImpl_Container *node);

/* IDxDiagProvider IUnknown parts follow: */
static HRESULT WINAPI IDxDiagProviderImpl_QueryInterface(PDXDIAGPROVIDER iface, REFIID riid, LPVOID *ppobj)
{
    IDxDiagProviderImpl *This = (IDxDiagProviderImpl *)iface;

    if (!ppobj) return E_INVALIDARG;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDxDiagProvider)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDxDiagProviderImpl_AddRef(PDXDIAGPROVIDER iface) {
    IDxDiagProviderImpl *This = (IDxDiagProviderImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n", This, refCount - 1);

    DXDIAGN_LockModule();

    return refCount;
}

static ULONG WINAPI IDxDiagProviderImpl_Release(PDXDIAGPROVIDER iface) {
    IDxDiagProviderImpl *This = (IDxDiagProviderImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n", This, refCount + 1);

    if (!refCount) {
        free_information_tree(This->info_root);
        HeapFree(GetProcessHeap(), 0, This);
    }

    DXDIAGN_UnlockModule();
    
    return refCount;
}

/* IDxDiagProvider Interface follow: */
static HRESULT WINAPI IDxDiagProviderImpl_Initialize(PDXDIAGPROVIDER iface, DXDIAG_INIT_PARAMS* pParams) {
    IDxDiagProviderImpl *This = (IDxDiagProviderImpl *)iface;
    HRESULT hr;

    TRACE("(%p,%p)\n", iface, pParams);

    if (NULL == pParams) {
      return E_POINTER;
    }
    if (pParams->dwSize != sizeof(DXDIAG_INIT_PARAMS) ||
        pParams->dwDxDiagHeaderVersion != DXDIAG_DX9_SDK_VERSION) {
      return E_INVALIDARG;
    }

    if (!This->info_root)
    {
        hr = build_information_tree(&This->info_root);
        if (FAILED(hr))
            return hr;
    }

    This->init = TRUE;
    memcpy(&This->params, pParams, pParams->dwSize);
    return S_OK;
}

static HRESULT WINAPI IDxDiagProviderImpl_GetRootContainer(PDXDIAGPROVIDER iface, IDxDiagContainer** ppInstance) {
  IDxDiagProviderImpl *This = (IDxDiagProviderImpl *)iface;

  TRACE("(%p,%p)\n", iface, ppInstance);

  if (FALSE == This->init) {
    return CO_E_NOTINITIALIZED;
  }

  return DXDiag_CreateDXDiagContainer(&IID_IDxDiagContainer, This->info_root,
                                      (IDxDiagProvider *)This, (void **)ppInstance);
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

  *ppobj = NULL;
  if (punkOuter) return CLASS_E_NOAGGREGATION;

  provider = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDxDiagProviderImpl));
  if (NULL == provider) return E_OUTOFMEMORY;
  provider->lpVtbl = &DxDiagProvider_Vtbl;
  provider->ref = 0; /* will be inited with QueryInterface */
  return IDxDiagProviderImpl_QueryInterface ((PDXDIAGPROVIDER)provider, riid, ppobj);
}

static void get_display_device_id(WCHAR *szIdentifierBuffer)
{
    static const WCHAR szNA[] = {'n','/','a',0};

    HRESULT hr = E_FAIL;

    HMODULE                 d3d9_handle;
    IDirect3D9             *(WINAPI *pDirect3DCreate9)(UINT) = NULL;
    IDirect3D9             *pD3d = NULL;
    D3DADAPTER_IDENTIFIER9  adapter_ident;

    /* Retrieves the display device identifier from the d3d9 implementation. */
    d3d9_handle = LoadLibraryA("d3d9.dll");
    if(d3d9_handle)
        pDirect3DCreate9 = (void *)GetProcAddress(d3d9_handle, "Direct3DCreate9");
    if(pDirect3DCreate9)
        pD3d = pDirect3DCreate9(D3D_SDK_VERSION);
    if(pD3d)
        hr = IDirect3D9_GetAdapterIdentifier(pD3d, D3DADAPTER_DEFAULT, 0, &adapter_ident);
    if(SUCCEEDED(hr)) {
        StringFromGUID2(&adapter_ident.DeviceIdentifier, szIdentifierBuffer, 39);
    } else {
        memcpy(szIdentifierBuffer, szNA, sizeof(szNA));
    }

    if (pD3d)
        IDirect3D9_Release(pD3d);
    if (d3d9_handle)
        FreeLibrary(d3d9_handle);
}

static void free_property_information(IDxDiagContainerImpl_Property *prop)
{
    VariantClear(&prop->vProp);
    HeapFree(GetProcessHeap(), 0, prop->propName);
    HeapFree(GetProcessHeap(), 0, prop);
}

static void free_information_tree(IDxDiagContainerImpl_Container *node)
{
    IDxDiagContainerImpl_Container *ptr, *cursor2;

    if (!node)
        return;

    HeapFree(GetProcessHeap(), 0, node->contName);

    LIST_FOR_EACH_ENTRY_SAFE(ptr, cursor2, &node->subContainers, IDxDiagContainerImpl_Container, entry)
    {
        IDxDiagContainerImpl_Property *prop, *prop_cursor2;

        LIST_FOR_EACH_ENTRY_SAFE(prop, prop_cursor2, &ptr->properties, IDxDiagContainerImpl_Property, entry)
        {
            list_remove(&prop->entry);
            free_property_information(prop);
        }

        list_remove(&ptr->entry);
        free_information_tree(ptr);
    }

    HeapFree(GetProcessHeap(), 0, node);
}

static IDxDiagContainerImpl_Container *allocate_information_node(const WCHAR *name)
{
    IDxDiagContainerImpl_Container *ret;

    ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ret));
    if (!ret)
        return NULL;

    if (name)
    {
        ret->contName = HeapAlloc(GetProcessHeap(), 0, (strlenW(name) + 1) * sizeof(*name));
        if (!ret->contName)
        {
            HeapFree(GetProcessHeap(), 0, ret);
            return NULL;
        }
        strcpyW(ret->contName, name);
    }

    list_init(&ret->subContainers);
    list_init(&ret->properties);

    return ret;
}

static IDxDiagContainerImpl_Property *allocate_property_information(const WCHAR *name)
{
    IDxDiagContainerImpl_Property *ret;

    ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ret));
    if (!ret)
        return NULL;

    ret->propName = HeapAlloc(GetProcessHeap(), 0, (strlenW(name) + 1) * sizeof(*name));
    if (!ret->propName)
    {
        HeapFree(GetProcessHeap(), 0, ret);
        return NULL;
    }
    strcpyW(ret->propName, name);

    return ret;
}

static inline void add_subcontainer(IDxDiagContainerImpl_Container *node, IDxDiagContainerImpl_Container *subCont)
{
    list_add_tail(&node->subContainers, &subCont->entry);
    ++node->nSubContainers;
}

static inline HRESULT add_bstr_property(IDxDiagContainerImpl_Container *node, const WCHAR *propName, const WCHAR *str)
{
    IDxDiagContainerImpl_Property *prop;
    BSTR bstr;

    prop = allocate_property_information(propName);
    if (!prop)
        return E_OUTOFMEMORY;

    bstr = SysAllocString(str);
    if (!bstr)
    {
        free_property_information(prop);
        return E_OUTOFMEMORY;
    }

    V_VT(&prop->vProp) = VT_BSTR;
    V_BSTR(&prop->vProp) = bstr;

    list_add_tail(&node->properties, &prop->entry);
    ++node->nProperties;

    return S_OK;
}

static inline HRESULT add_ui4_property(IDxDiagContainerImpl_Container *node, const WCHAR *propName, DWORD data)
{
    IDxDiagContainerImpl_Property *prop;

    prop = allocate_property_information(propName);
    if (!prop)
        return E_OUTOFMEMORY;

    V_VT(&prop->vProp) = VT_UI4;
    V_UI4(&prop->vProp) = data;

    list_add_tail(&node->properties, &prop->entry);
    ++node->nProperties;

    return S_OK;
}

static inline HRESULT add_bool_property(IDxDiagContainerImpl_Container *node, const WCHAR *propName, BOOL data)
{
    IDxDiagContainerImpl_Property *prop;

    prop = allocate_property_information(propName);
    if (!prop)
        return E_OUTOFMEMORY;

    V_VT(&prop->vProp) = VT_BOOL;
    V_BOOL(&prop->vProp) = data;

    list_add_tail(&node->properties, &prop->entry);
    ++node->nProperties;

    return S_OK;
}

static inline HRESULT add_ull_as_bstr_property(IDxDiagContainerImpl_Container *node, const WCHAR *propName, ULONGLONG data )
{
    IDxDiagContainerImpl_Property *prop;

    prop = allocate_property_information(propName);
    if (!prop)
        return E_OUTOFMEMORY;

    V_VT(&prop->vProp) = VT_UI8;
    V_UI8(&prop->vProp) = data;

    VariantChangeType(&prop->vProp, &prop->vProp, 0, VT_BSTR);

    list_add_tail(&node->properties, &prop->entry);
    ++node->nProperties;

    return S_OK;
}

static HRESULT build_systeminfo_tree(IDxDiagContainerImpl_Container *node)
{
    static const WCHAR dwDirectXVersionMajor[] = {'d','w','D','i','r','e','c','t','X','V','e','r','s','i','o','n','M','a','j','o','r',0};
    static const WCHAR dwDirectXVersionMinor[] = {'d','w','D','i','r','e','c','t','X','V','e','r','s','i','o','n','M','i','n','o','r',0};
    static const WCHAR szDirectXVersionLetter[] = {'s','z','D','i','r','e','c','t','X','V','e','r','s','i','o','n','L','e','t','t','e','r',0};
    static const WCHAR szDirectXVersionLetter_v[] = {'c',0};
    static const WCHAR bDebug[] = {'b','D','e','b','u','g',0};
    static const WCHAR szDirectXVersionEnglish[] = {'s','z','D','i','r','e','c','t','X','V','e','r','s','i','o','n','E','n','g','l','i','s','h',0};
    static const WCHAR szDirectXVersionEnglish_v[] = {'4','.','0','9','.','0','0','0','0','.','0','9','0','4',0};
    static const WCHAR szDirectXVersionLongEnglish[] = {'s','z','D','i','r','e','c','t','X','V','e','r','s','i','o','n','L','o','n','g','E','n','g','l','i','s','h',0};
    static const WCHAR szDirectXVersionLongEnglish_v[] = {'=',' ','"','D','i','r','e','c','t','X',' ','9','.','0','c',' ','(','4','.','0','9','.','0','0','0','0','.','0','9','0','4',')',0};
    static const WCHAR ullPhysicalMemory[] = {'u','l','l','P','h','y','s','i','c','a','l','M','e','m','o','r','y',0};
    static const WCHAR ullUsedPageFile[]   = {'u','l','l','U','s','e','d','P','a','g','e','F','i','l','e',0};
    static const WCHAR ullAvailPageFile[]  = {'u','l','l','A','v','a','i','l','P','a','g','e','F','i','l','e',0};
    static const WCHAR szWindowsDir[] = {'s','z','W','i','n','d','o','w','s','D','i','r',0};
    static const WCHAR dwOSMajorVersion[] = {'d','w','O','S','M','a','j','o','r','V','e','r','s','i','o','n',0};
    static const WCHAR dwOSMinorVersion[] = {'d','w','O','S','M','i','n','o','r','V','e','r','s','i','o','n',0};
    static const WCHAR dwOSBuildNumber[] = {'d','w','O','S','B','u','i','l','d','N','u','m','b','e','r',0};
    static const WCHAR dwOSPlatformID[] = {'d','w','O','S','P','l','a','t','f','o','r','m','I','D',0};
    static const WCHAR szCSDVersion[] = {'s','z','C','S','D','V','e','r','s','i','o','n',0};

    HRESULT hr;
    MEMORYSTATUSEX msex;
    OSVERSIONINFOW info;
    WCHAR buffer[MAX_PATH];

    hr = add_ui4_property(node, dwDirectXVersionMajor, 9);
    if (FAILED(hr))
        return hr;

    hr = add_ui4_property(node, dwDirectXVersionMinor, 0);
    if (FAILED(hr))
        return hr;

    hr = add_bstr_property(node, szDirectXVersionLetter, szDirectXVersionLetter_v);
    if (FAILED(hr))
        return hr;

    hr = add_bstr_property(node, szDirectXVersionEnglish, szDirectXVersionEnglish_v);
    if (FAILED(hr))
        return hr;

    hr = add_bstr_property(node, szDirectXVersionLongEnglish, szDirectXVersionLongEnglish_v);
    if (FAILED(hr))
        return hr;

    hr = add_bool_property(node, bDebug, FALSE);
    if (FAILED(hr))
        return hr;

    msex.dwLength = sizeof(msex);
    GlobalMemoryStatusEx(&msex);

    hr = add_ull_as_bstr_property(node, ullPhysicalMemory, msex.ullTotalPhys);
    if (FAILED(hr))
        return hr;

    hr = add_ull_as_bstr_property(node, ullUsedPageFile, msex.ullTotalPageFile - msex.ullAvailPageFile);
    if (FAILED(hr))
        return hr;

    hr = add_ull_as_bstr_property(node, ullAvailPageFile, msex.ullAvailPageFile);
    if (FAILED(hr))
        return hr;

    info.dwOSVersionInfoSize = sizeof(info);
    GetVersionExW(&info);

    hr = add_ui4_property(node, dwOSMajorVersion, info.dwMajorVersion);
    if (FAILED(hr))
        return hr;

    hr = add_ui4_property(node, dwOSMinorVersion, info.dwMinorVersion);
    if (FAILED(hr))
        return hr;

    hr = add_ui4_property(node, dwOSBuildNumber, info.dwBuildNumber);
    if (FAILED(hr))
        return hr;

    hr = add_ui4_property(node, dwOSPlatformID, info.dwPlatformId);
    if (FAILED(hr))
        return hr;

    hr = add_bstr_property(node, szCSDVersion, info.szCSDVersion);
    if (FAILED(hr))
        return hr;

    GetWindowsDirectoryW(buffer, MAX_PATH);

    hr = add_bstr_property(node, szWindowsDir, buffer);
    if (FAILED(hr))
        return hr;

    return S_OK;
}

static HRESULT build_displaydevices_tree(IDxDiagContainerImpl_Container *node)
{
    static const WCHAR szDescription[] = {'s','z','D','e','s','c','r','i','p','t','i','o','n',0};
    static const WCHAR szDeviceName[] = {'s','z','D','e','v','i','c','e','N','a','m','e',0};
    static const WCHAR szKeyDeviceID[] = {'s','z','K','e','y','D','e','v','i','c','e','I','D',0};
    static const WCHAR szKeyDeviceKey[] = {'s','z','K','e','y','D','e','v','i','c','e','K','e','y',0};
    static const WCHAR szVendorId[] = {'s','z','V','e','n','d','o','r','I','d',0};
    static const WCHAR szDeviceId[] = {'s','z','D','e','v','i','c','e','I','d',0};
    static const WCHAR szDeviceIdentifier[] = {'s','z','D','e','v','i','c','e','I','d','e','n','t','i','f','i','e','r',0};
    static const WCHAR dwWidth[] = {'d','w','W','i','d','t','h',0};
    static const WCHAR dwHeight[] = {'d','w','H','e','i','g','h','t',0};
    static const WCHAR dwBpp[] = {'d','w','B','p','p',0};
    static const WCHAR szDisplayMemoryLocalized[] = {'s','z','D','i','s','p','l','a','y','M','e','m','o','r','y','L','o','c','a','l','i','z','e','d',0};
    static const WCHAR szDisplayMemoryEnglish[] = {'s','z','D','i','s','p','l','a','y','M','e','m','o','r','y','E','n','g','l','i','s','h',0};

    static const WCHAR szAdapterID[] = {'0',0};
    static const WCHAR szEmpty[] = {0};

    IDxDiagContainerImpl_Container *display_adapter;
    HRESULT hr;
    IDirectDraw7 *pDirectDraw;
    DDSCAPS2 dd_caps;
    DISPLAY_DEVICEW disp_dev;
    DDSURFACEDESC2 surface_descr;
    DWORD tmp;
    WCHAR buffer[256];

    display_adapter = allocate_information_node(szAdapterID);
    if (!display_adapter)
        return E_OUTOFMEMORY;

    add_subcontainer(node, display_adapter);

    disp_dev.cb = sizeof(disp_dev);
    if (EnumDisplayDevicesW( NULL, 0, &disp_dev, 0 ))
    {
        hr = add_bstr_property(display_adapter, szDeviceName, disp_dev.DeviceName);
        if (FAILED(hr))
            return hr;

        hr = add_bstr_property(display_adapter, szDescription, disp_dev.DeviceString);
        if (FAILED(hr))
            return hr;
    }

    /* For now, silently ignore a failure from DirectDrawCreateEx. */
    hr = DirectDrawCreateEx(NULL, (LPVOID *)&pDirectDraw, &IID_IDirectDraw7, NULL);
    if (FAILED(hr))
        return S_OK;

    dd_caps.dwCaps = DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY;
    dd_caps.dwCaps2 = dd_caps.dwCaps3 = dd_caps.dwCaps4 = 0;
    hr = IDirectDraw7_GetAvailableVidMem(pDirectDraw, &dd_caps, &tmp, NULL);
    if (SUCCEEDED(hr))
    {
        static const WCHAR mem_fmt[] = {'%','.','1','f',' ','M','B',0};

        snprintfW(buffer, sizeof(buffer)/sizeof(buffer[0]), mem_fmt, ((float)tmp) / 1000000.0);

        hr = add_bstr_property(display_adapter, szDisplayMemoryLocalized, buffer);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(display_adapter, szDisplayMemoryEnglish, buffer);
        if (FAILED(hr))
            goto cleanup;
    }

    surface_descr.dwSize = sizeof(surface_descr);
    hr = IDirectDraw7_GetDisplayMode(pDirectDraw, &surface_descr);
    if (SUCCEEDED(hr))
    {
        if (surface_descr.dwFlags & DDSD_WIDTH)
        {
            hr = add_ui4_property(display_adapter, dwWidth, surface_descr.dwWidth);
            if (FAILED(hr))
                goto cleanup;
        }

        if (surface_descr.dwFlags & DDSD_HEIGHT)
        {
            hr = add_ui4_property(display_adapter, dwHeight, surface_descr.dwHeight);
            if (FAILED(hr))
                goto cleanup;
        }

        if (surface_descr.dwFlags & DDSD_PIXELFORMAT)
        {
            hr = add_ui4_property(display_adapter, dwBpp, surface_descr.u4.ddpfPixelFormat.u1.dwRGBBitCount);
            if (FAILED(hr))
                goto cleanup;
        }
    }

    get_display_device_id(buffer);

    hr = add_bstr_property(display_adapter, szDeviceIdentifier, buffer);
    if (FAILED(hr))
        goto cleanup;

    hr = add_bstr_property(display_adapter, szVendorId, szEmpty);
    if (FAILED(hr))
        goto cleanup;

    hr = add_bstr_property(display_adapter, szDeviceId, szEmpty);
    if (FAILED(hr))
        goto cleanup;

    hr = add_bstr_property(display_adapter, szKeyDeviceKey, szEmpty);
    if (FAILED(hr))
        goto cleanup;

    hr = add_bstr_property(display_adapter, szKeyDeviceID, szEmpty);
    if (FAILED(hr))
        goto cleanup;

    hr = S_OK;
cleanup:
    IDirectDraw7_Release(pDirectDraw);
    return hr;
}

static HRESULT build_directsound_tree(IDxDiagContainerImpl_Container *node)
{
    static const WCHAR DxDiag_SoundDevices[] = {'D','x','D','i','a','g','_','S','o','u','n','d','D','e','v','i','c','e','s',0};
    static const WCHAR DxDiag_SoundCaptureDevices[] = {'D','x','D','i','a','g','_','S','o','u','n','d','C','a','p','t','u','r','e','D','e','v','i','c','e','s',0};

    IDxDiagContainerImpl_Container *cont;

    cont = allocate_information_node(DxDiag_SoundDevices);
    if (!cont)
        return E_OUTOFMEMORY;

    add_subcontainer(node, cont);

    cont = allocate_information_node(DxDiag_SoundCaptureDevices);
    if (!cont)
        return E_OUTOFMEMORY;

    add_subcontainer(node, cont);

    return S_OK;
}

static HRESULT build_directmusic_tree(IDxDiagContainerImpl_Container *node)
{
    return S_OK;
}

static HRESULT build_directinput_tree(IDxDiagContainerImpl_Container *node)
{
    return S_OK;
}

static HRESULT build_directplay_tree(IDxDiagContainerImpl_Container *node)
{
    return S_OK;
}

static HRESULT build_systemdevices_tree(IDxDiagContainerImpl_Container *node)
{
    return S_OK;
}

static HRESULT fill_file_description(IDxDiagContainerImpl_Container *node, const WCHAR *szFilePath, const WCHAR *szFileName)
{
    static const WCHAR szSlashSep[] = {'\\',0};
    static const WCHAR szPath[] = {'s','z','P','a','t','h',0};
    static const WCHAR szName[] = {'s','z','N','a','m','e',0};
    static const WCHAR szVersion[] = {'s','z','V','e','r','s','i','o','n',0};
    static const WCHAR szAttributes[] = {'s','z','A','t','t','r','i','b','u','t','e','s',0};
    static const WCHAR szLanguageEnglish[] = {'s','z','L','a','n','g','u','a','g','e','E','n','g','l','i','s','h',0};
    static const WCHAR dwFileTimeHigh[] = {'d','w','F','i','l','e','T','i','m','e','H','i','g','h',0};
    static const WCHAR dwFileTimeLow[] = {'d','w','F','i','l','e','T','i','m','e','L','o','w',0};
    static const WCHAR bBeta[] = {'b','B','e','t','a',0};
    static const WCHAR bDebug[] = {'b','D','e','b','u','g',0};
    static const WCHAR bExists[] = {'b','E','x','i','s','t','s',0};

    /* Values */
    static const WCHAR szFinal_Retail_v[] = {'F','i','n','a','l',' ','R','e','t','a','i','l',0};
    static const WCHAR szEnglish_v[] = {'E','n','g','l','i','s','h',0};
    static const WCHAR szVersionFormat[] = {'%','u','.','%','0','2','u','.','%','0','4','u','.','%','0','4','u',0};

    HRESULT hr;
    WCHAR *szFile;
    WCHAR szVersion_v[1024];
    DWORD retval, hdl;
    void *pVersionInfo = NULL;
    BOOL boolret = FALSE;
    UINT uiLength;
    VS_FIXEDFILEINFO *pFileInfo;

    TRACE("Filling container %p for %s in %s\n", node,
          debugstr_w(szFileName), debugstr_w(szFilePath));

    szFile = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * (lstrlenW(szFilePath) +
                                            lstrlenW(szFileName) + 2 /* slash + terminator */));
    if (!szFile)
        return E_OUTOFMEMORY;

    lstrcpyW(szFile, szFilePath);
    lstrcatW(szFile, szSlashSep);
    lstrcatW(szFile, szFileName);

    retval = GetFileVersionInfoSizeW(szFile, &hdl);
    if (retval)
    {
        pVersionInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, retval);
        if (!pVersionInfo)
        {
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }

        if (GetFileVersionInfoW(szFile, 0, retval, pVersionInfo) &&
            VerQueryValueW(pVersionInfo, szSlashSep, (void **)&pFileInfo, &uiLength))
            boolret = TRUE;
    }

    hr = add_bstr_property(node, szPath, szFile);
    if (FAILED(hr))
        goto cleanup;

    hr = add_bstr_property(node, szName, szFileName);
    if (FAILED(hr))
        goto cleanup;

    hr = add_bool_property(node, bExists, boolret);
    if (FAILED(hr))
        goto cleanup;

    if (boolret)
    {
        snprintfW(szVersion_v, sizeof(szVersion_v)/sizeof(szVersion_v[0]),
                  szVersionFormat,
                  HIWORD(pFileInfo->dwFileVersionMS),
                  LOWORD(pFileInfo->dwFileVersionMS),
                  HIWORD(pFileInfo->dwFileVersionLS),
                  LOWORD(pFileInfo->dwFileVersionLS));

        TRACE("Found version as (%s)\n", debugstr_w(szVersion_v));

        hr = add_bstr_property(node, szVersion, szVersion_v);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(node, szAttributes, szFinal_Retail_v);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(node, szLanguageEnglish, szEnglish_v);
        if (FAILED(hr))
            goto cleanup;

        hr = add_ui4_property(node, dwFileTimeHigh, pFileInfo->dwFileDateMS);
        if (FAILED(hr))
            goto cleanup;

        hr = add_ui4_property(node, dwFileTimeLow, pFileInfo->dwFileDateLS);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bool_property(node, bBeta, ((pFileInfo->dwFileFlags & pFileInfo->dwFileFlagsMask) & VS_FF_PRERELEASE) != 0);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bool_property(node, bDebug, ((pFileInfo->dwFileFlags & pFileInfo->dwFileFlagsMask) & VS_FF_DEBUG) != 0);
        if (FAILED(hr))
            goto cleanup;
    }

    hr = S_OK;
cleanup:
    HeapFree(GetProcessHeap(), 0, pVersionInfo);
    HeapFree(GetProcessHeap(), 0, szFile);

    return hr;
}
static HRESULT build_directxfiles_tree(IDxDiagContainerImpl_Container *node)
{
    static const WCHAR dlls[][15] =
    {
        {'d','3','d','8','.','d','l','l',0},
        {'d','3','d','9','.','d','l','l',0},
        {'d','d','r','a','w','.','d','l','l',0},
        {'d','e','v','e','n','u','m','.','d','l','l',0},
        {'d','i','n','p','u','t','8','.','d','l','l',0},
        {'d','i','n','p','u','t','.','d','l','l',0},
        {'d','m','b','a','n','d','.','d','l','l',0},
        {'d','m','c','o','m','p','o','s','.','d','l','l',0},
        {'d','m','i','m','e','.','d','l','l',0},
        {'d','m','l','o','a','d','e','r','.','d','l','l',0},
        {'d','m','s','c','r','i','p','t','.','d','l','l',0},
        {'d','m','s','t','y','l','e','.','d','l','l',0},
        {'d','m','s','y','n','t','h','.','d','l','l',0},
        {'d','m','u','s','i','c','.','d','l','l',0},
        {'d','p','l','a','y','x','.','d','l','l',0},
        {'d','p','n','e','t','.','d','l','l',0},
        {'d','s','o','u','n','d','.','d','l','l',0},
        {'d','s','w','a','v','e','.','d','l','l',0},
        {'d','x','d','i','a','g','n','.','d','l','l',0},
        {'q','u','a','r','t','z','.','d','l','l',0}
    };

    HRESULT hr;
    WCHAR szFilePath[MAX_PATH];
    INT i;

    GetSystemDirectoryW(szFilePath, MAX_PATH);

    for (i = 0; i < sizeof(dlls) / sizeof(dlls[0]); i++)
    {
        static const WCHAR szFormat[] = {'%','d',0};

        WCHAR szFileID[5];
        IDxDiagContainerImpl_Container *file_container;

        snprintfW(szFileID, sizeof(szFileID)/sizeof(szFileID[0]), szFormat, i);

        file_container = allocate_information_node(szFileID);
        if (!file_container)
            return E_OUTOFMEMORY;

        hr = fill_file_description(file_container, szFilePath, dlls[i]);
        if (FAILED(hr))
        {
            free_information_tree(file_container);
            continue;
        }

        add_subcontainer(node, file_container);
    }

    return S_OK;
}

static HRESULT read_property_names(IPropertyBag *pPropBag, VARIANT *friendly_name, VARIANT *clsid_name)
{
    static const WCHAR wszFriendlyName[] = {'F','r','i','e','n','d','l','y','N','a','m','e',0};
    static const WCHAR wszClsidName[] = {'C','L','S','I','D',0};

    HRESULT hr;

    VariantInit(friendly_name);
    VariantInit(clsid_name);

    hr = IPropertyBag_Read(pPropBag, wszFriendlyName, friendly_name, 0);
    if (FAILED(hr))
        return hr;

    hr = IPropertyBag_Read(pPropBag, wszClsidName, clsid_name, 0);
    if (FAILED(hr))
    {
        VariantClear(friendly_name);
        return hr;
    }

    return S_OK;
}

static HRESULT fill_filter_data_information(IDxDiagContainerImpl_Container *subcont, BYTE *pData, ULONG cb)
{
    static const WCHAR szVersionW[] = {'s','z','V','e','r','s','i','o','n',0};
    static const WCHAR dwInputs[] = {'d','w','I','n','p','u','t','s',0};
    static const WCHAR dwOutputs[] = {'d','w','O','u','t','p','u','t','s',0};
    static const WCHAR dwMeritW[] = {'d','w','M','e','r','i','t',0};
    static const WCHAR szVersionFormat[] = {'v','%','d',0};

    HRESULT hr;
    IFilterMapper2 *pFileMapper = NULL;
    IAMFilterData *pFilterData = NULL;
    REGFILTER2 *pRF = NULL;
    WCHAR bufferW[10];
    ULONG j;
    DWORD dwNOutputs = 0;
    DWORD dwNInputs = 0;

    hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC, &IID_IFilterMapper2,
                          (void **)&pFileMapper);
    if (FAILED(hr))
        return hr;

    hr = IFilterMapper2_QueryInterface(pFileMapper, &IID_IAMFilterData, (void **)&pFilterData);
    if (FAILED(hr))
        goto cleanup;

    hr = IAMFilterData_ParseFilterData(pFilterData, pData, cb, (BYTE **)&pRF);
    if (FAILED(hr))
        goto cleanup;

    snprintfW(bufferW, sizeof(bufferW)/sizeof(bufferW[0]), szVersionFormat, pRF->dwVersion);
    hr = add_bstr_property(subcont, szVersionW, bufferW);
    if (FAILED(hr))
        goto cleanup;

    if (pRF->dwVersion == 1)
    {
        for (j = 0; j < pRF->u.s.cPins; j++)
            if (pRF->u.s.rgPins[j].bOutput)
                dwNOutputs++;
            else
                dwNInputs++;
    }
    else if (pRF->dwVersion == 2)
    {
        for (j = 0; j < pRF->u.s1.cPins2; j++)
            if (pRF->u.s1.rgPins2[j].dwFlags & REG_PINFLAG_B_OUTPUT)
                dwNOutputs++;
            else
                dwNInputs++;
    }

    hr = add_ui4_property(subcont, dwInputs, dwNInputs);
    if (FAILED(hr))
        goto cleanup;

    hr = add_ui4_property(subcont, dwOutputs, dwNOutputs);
    if (FAILED(hr))
        goto cleanup;

    hr = add_ui4_property(subcont, dwMeritW, pRF->dwMerit);
    if (FAILED(hr))
        goto cleanup;

    hr = S_OK;
cleanup:
    CoTaskMemFree(pRF);
    if (pFilterData) IAMFilterData_Release(pFilterData);
    if (pFileMapper) IFilterMapper2_Release(pFileMapper);

    return hr;
}

static HRESULT fill_filter_container(IDxDiagContainerImpl_Container *subcont, IMoniker *pMoniker)
{
    static const WCHAR szName[] = {'s','z','N','a','m','e',0};
    static const WCHAR ClsidFilterW[] = {'C','l','s','i','d','F','i','l','t','e','r',0};
    static const WCHAR wszFilterDataName[] = {'F','i','l','t','e','r','D','a','t','a',0};

    HRESULT hr;
    IPropertyBag *pPropFilterBag = NULL;
    BYTE *pData;
    VARIANT friendly_name;
    VARIANT clsid_name;
    VARIANT v;

    VariantInit(&friendly_name);
    VariantInit(&clsid_name);
    VariantInit(&v);

    hr = IMoniker_BindToStorage(pMoniker, NULL, NULL, &IID_IPropertyBag, (void **)&pPropFilterBag);
    if (FAILED(hr))
        return hr;

    hr = read_property_names(pPropFilterBag, &friendly_name, &clsid_name);
    if (FAILED(hr))
        goto cleanup;

    TRACE("Name = %s\n", debugstr_w(V_BSTR(&friendly_name)));
    TRACE("CLSID = %s\n", debugstr_w(V_BSTR(&clsid_name)));

    hr = add_bstr_property(subcont, szName, V_BSTR(&friendly_name));
    if (FAILED(hr))
        goto cleanup;

    hr = add_bstr_property(subcont, ClsidFilterW, V_BSTR(&clsid_name));
    if (FAILED(hr))
        goto cleanup;

    hr = IPropertyBag_Read(pPropFilterBag, wszFilterDataName, &v, NULL);
    if (FAILED(hr))
        goto cleanup;

    hr = SafeArrayAccessData(V_ARRAY(&v), (void **)&pData);
    if (FAILED(hr))
        goto cleanup;

    hr = fill_filter_data_information(subcont, pData, V_ARRAY(&v)->rgsabound->cElements);
    SafeArrayUnaccessData(V_ARRAY(&v));
    if (FAILED(hr))
        goto cleanup;

    hr = S_OK;
cleanup:
    VariantClear(&v);
    VariantClear(&clsid_name);
    VariantClear(&friendly_name);
    if (pPropFilterBag) IPropertyBag_Release(pPropFilterBag);

    return hr;
}

static HRESULT build_directshowfilters_tree(IDxDiagContainerImpl_Container *node)
{
    static const WCHAR szCatName[] = {'s','z','C','a','t','N','a','m','e',0};
    static const WCHAR ClsidCatW[] = {'C','l','s','i','d','C','a','t',0};
    static const WCHAR szIdFormat[] = {'%','d',0};

    HRESULT hr;
    int i = 0;
    ICreateDevEnum *pCreateDevEnum;
    IEnumMoniker *pEmCat = NULL;
    IMoniker *pMCat = NULL;
	IEnumMoniker *pEnum = NULL;

    hr = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ICreateDevEnum, (void **)&pCreateDevEnum);
    if (FAILED(hr))
        return hr;

    hr = ICreateDevEnum_CreateClassEnumerator(pCreateDevEnum, &CLSID_ActiveMovieCategories, &pEmCat, 0);
    if (FAILED(hr))
        goto cleanup;

    while (IEnumMoniker_Next(pEmCat, 1, &pMCat, NULL) == S_OK)
    {
        VARIANT vCatName;
        VARIANT vCatClsid;
        IPropertyBag *pPropBag;
        CLSID clsidCat;
        IMoniker *pMoniker = NULL;

        hr = IMoniker_BindToStorage(pMCat, NULL, NULL, &IID_IPropertyBag, (void **)&pPropBag);
        if (FAILED(hr))
        {
            IMoniker_Release(pMCat);
            break;
        }

        hr = read_property_names(pPropBag, &vCatName, &vCatClsid);
        IPropertyBag_Release(pPropBag);
        if (FAILED(hr))
        {
            IMoniker_Release(pMCat);
            break;
        }

        hr = CLSIDFromString(V_BSTR(&vCatClsid), &clsidCat);
        if (FAILED(hr))
        {
            IMoniker_Release(pMCat);
            VariantClear(&vCatClsid);
            VariantClear(&vCatName);
            break;
        }

        hr = ICreateDevEnum_CreateClassEnumerator(pCreateDevEnum, &clsidCat, &pEnum, 0);
        if (hr != S_OK)
        {
            IMoniker_Release(pMCat);
            VariantClear(&vCatClsid);
            VariantClear(&vCatName);
            continue;
        }

        TRACE("Enumerating class %s\n", debugstr_guid(&clsidCat));

        while (IEnumMoniker_Next(pEnum, 1, &pMoniker, NULL) == S_OK)
        {
            WCHAR bufferW[10];
            IDxDiagContainerImpl_Container *subcont;

            snprintfW(bufferW, sizeof(bufferW)/sizeof(bufferW[0]), szIdFormat, i);
            subcont = allocate_information_node(bufferW);
            if (!subcont)
            {
                hr = E_OUTOFMEMORY;
                IMoniker_Release(pMoniker);
                break;
            }

            hr = add_bstr_property(subcont, szCatName, V_BSTR(&vCatName));
            if (FAILED(hr))
            {
                free_information_tree(subcont);
                IMoniker_Release(pMoniker);
                break;
            }

            hr = add_bstr_property(subcont, ClsidCatW, V_BSTR(&vCatClsid));
            if (FAILED(hr))
            {
                free_information_tree(subcont);
                IMoniker_Release(pMoniker);
                break;
            }

            hr = fill_filter_container(subcont, pMoniker);
            if (FAILED(hr))
            {
                free_information_tree(subcont);
                IMoniker_Release(pMoniker);
                break;
            }

            add_subcontainer(node, subcont);
            i++;
            IMoniker_Release(pMoniker);
        }

        IEnumMoniker_Release(pEnum);
        IMoniker_Release(pMCat);
        VariantClear(&vCatClsid);
        VariantClear(&vCatName);

        if (FAILED(hr))
            break;
    }

cleanup:
    if (pEmCat) IEnumMoniker_Release(pEmCat);
    ICreateDevEnum_Release(pCreateDevEnum);
    return hr;
}

static HRESULT build_logicaldisks_tree(IDxDiagContainerImpl_Container *node)
{
    return S_OK;
}

static HRESULT build_information_tree(IDxDiagContainerImpl_Container **pinfo_root)
{
    static const WCHAR DxDiag_SystemInfo[] = {'D','x','D','i','a','g','_','S','y','s','t','e','m','I','n','f','o',0};
    static const WCHAR DxDiag_DisplayDevices[] = {'D','x','D','i','a','g','_','D','i','s','p','l','a','y','D','e','v','i','c','e','s',0};
    static const WCHAR DxDiag_DirectSound[] = {'D','x','D','i','a','g','_','D','i','r','e','c','t','S','o','u','n','d',0};
    static const WCHAR DxDiag_DirectMusic[] = {'D','x','D','i','a','g','_','D','i','r','e','c','t','M','u','s','i','c',0};
    static const WCHAR DxDiag_DirectInput[] = {'D','x','D','i','a','g','_','D','i','r','e','c','t','I','n','p','u','t',0};
    static const WCHAR DxDiag_DirectPlay[] = {'D','x','D','i','a','g','_','D','i','r','e','c','t','P','l','a','y',0};
    static const WCHAR DxDiag_SystemDevices[] = {'D','x','D','i','a','g','_','S','y','s','t','e','m','D','e','v','i','c','e','s',0};
    static const WCHAR DxDiag_DirectXFiles[] = {'D','x','D','i','a','g','_','D','i','r','e','c','t','X','F','i','l','e','s',0};
    static const WCHAR DxDiag_DirectShowFilters[] = {'D','x','D','i','a','g','_','D','i','r','e','c','t','S','h','o','w','F','i','l','t','e','r','s',0};
    static const WCHAR DxDiag_LogicalDisks[] = {'D','x','D','i','a','g','_','L','o','g','i','c','a','l','D','i','s','k','s',0};

    static const struct
    {
        const WCHAR *name;
        HRESULT (*initfunc)(IDxDiagContainerImpl_Container *);
    } root_children[] =
    {
        {DxDiag_SystemInfo, build_systeminfo_tree},
        {DxDiag_DisplayDevices, build_displaydevices_tree},
        {DxDiag_DirectSound, build_directsound_tree},
        {DxDiag_DirectMusic, build_directmusic_tree},
        {DxDiag_DirectInput, build_directinput_tree},
        {DxDiag_DirectPlay, build_directplay_tree},
        {DxDiag_SystemDevices, build_systemdevices_tree},
        {DxDiag_DirectXFiles, build_directxfiles_tree},
        {DxDiag_DirectShowFilters, build_directshowfilters_tree},
        {DxDiag_LogicalDisks, build_logicaldisks_tree},
    };

    IDxDiagContainerImpl_Container *info_root;
    size_t index;

    info_root = allocate_information_node(NULL);
    if (!info_root)
        return E_OUTOFMEMORY;

    for (index = 0; index < sizeof(root_children)/sizeof(root_children[0]); index++)
    {
        IDxDiagContainerImpl_Container *node;
        HRESULT hr;

        node = allocate_information_node(root_children[index].name);
        if (!node)
        {
            free_information_tree(info_root);
            return E_OUTOFMEMORY;
        }

        hr = root_children[index].initfunc(node);
        if (FAILED(hr))
        {
            free_information_tree(node);
            free_information_tree(info_root);
            return hr;
        }

        add_subcontainer(info_root, node);
    }

    *pinfo_root = info_root;
    return S_OK;
}
