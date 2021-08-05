/*
 * Copyright 2015 Alistair Leslie-Hughes
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

#include "d3dx10.h"

DEFINE_GUID(IID_ID3DX10ThreadPump, 0xc93fecfa, 0x6967, 0x478a, 0xab, 0xbc, 0x40, 0x2d, 0x90, 0x62, 0x1f, 0xcb);

#define INTERFACE ID3DX10DataLoader
DECLARE_INTERFACE(ID3DX10DataLoader)
{
    STDMETHOD(Load)(THIS) PURE;
    STDMETHOD(Decompress)(THIS_ void **data, SIZE_T *bytes) PURE;
    STDMETHOD(Destroy)(THIS) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** ID3DX10DataLoader methods ***/
#define ID3DX10DataLoader_Load(p)           (p)->lpVtbl->Load(p)
#define ID3DX10DataLoader_Decompress(p,a,b) (p)->lpVtbl->Decompress(p,a,b)
#define ID3DX10DataLoader_Destroy(p)        (p)->lpVtbl->Destroy(p)
#else
/*** ID3DX10DataLoader methods ***/
#define ID3DX10DataLoader_Load(p)           (p)->Load()
#define ID3DX10DataLoader_Decompress(p,a,b) (p)->Decompress(a,b)
#define ID3DX10DataLoader_Destroy(p)        (p)->Destroy()
#endif

#define INTERFACE ID3DX10DataProcessor
DECLARE_INTERFACE(ID3DX10DataProcessor)
{
    STDMETHOD(Process)(THIS_ void *data, SIZE_T bytes) PURE;
    STDMETHOD(CreateDeviceObject)(THIS_ void **dataobject) PURE;
    STDMETHOD(Destroy)(THIS) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** ID3DX10DataProcessor methods ***/
#define ID3DX10DataProcessor_Process(p,a,b)          (p)->lpVtbl->Process(p,a,b)
#define ID3DX10DataProcessor_CreateDeviceObject(p,a) (p)->lpVtbl->CreateDeviceObject(p,a)
#define ID3DX10DataProcessor_Destroy(p)              (p)->lpVtbl->Destroy(p)
#else
/*** ID3DX10DataProcessor methods ***/
#define ID3DX10DataProcessor_Process(p,a,b)          (p)->Process(a,b)
#define ID3DX10DataProcessor_CreateDeviceObject(p,a) (p)->CreateDeviceObject(a)
#define ID3DX10DataProcessor_Destroy(p)              (p)->Destroy()
#endif

#define INTERFACE  ID3DX10ThreadPump
DECLARE_INTERFACE_(ID3DX10ThreadPump, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /*** ID3DX10ThreadPump methods ***/
    STDMETHOD(AddWorkItem)(THIS_ ID3DX10DataLoader *loader, ID3DX10DataProcessor *processor,
            HRESULT *result, void **object) PURE;
    STDMETHOD_(UINT, GetWorkItemCount)(THIS) PURE;
    STDMETHOD(WaitForAllItems)(THIS) PURE;
    STDMETHOD(ProcessDeviceWorkItems)(THIS_ UINT count) PURE;
    STDMETHOD(PurgeAllItems)(THIS) PURE;
    STDMETHOD(GetQueueStatus)(THIS_ UINT *queue, UINT *processqueue, UINT *devicequeue) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define ID3DX10ThreadPump_QueryInterface(p,a,b)       (p)->lpVtbl->QueryInterface(p,a,b)
#define ID3DX10ThreadPump_AddRef(p)                   (p)->lpVtbl->AddRef(p)
#define ID3DX10ThreadPump_Release(p)                  (p)->lpVtbl->Release(p)
/*** ID3DX10ThreadPump methods ***/
#define ID3DX10ThreadPump_AddWorkItem(p,a,b,c,d)      (p)->lpVtbl->AddWorkItem(p,a,b,c,d)
#define ID3DX10ThreadPump_GetWorkItemCount(p)         (p)->lpVtbl->GetWorkItemCount(p)
#define ID3DX10ThreadPump_WaitForAllItems(p)          (p)->lpVtbl->WaitForAllItems(p)
#define ID3DX10ThreadPump_ProcessDeviceWorkItems(p,a) (p)->lpVtbl->ProcessDeviceWorkItems(p,a)
#define ID3DX10ThreadPump_PurgeAllItems(p)            (p)->lpVtbl->PurgeAllItems(p)
#define ID3DX10ThreadPump_GetQueueStatus(p,a,b,c)     (p)->lpVtbl->GetQueueStatus(p,a,b,c)
#else
/*** IUnknown methods ***/
#define ID3DX10ThreadPump_QueryInterface(p,a,b)       (p)->QueryInterface(a,b)
#define ID3DX10ThreadPump_AddRef(p)                   (p)->AddRef()
#define ID3DX10ThreadPump_Release(p)                  (p)->Release()
/*** ID3DX10ThreadPump methods ***/
#define ID3DX10ThreadPump_AddWorkItem(p,a,b,c,d)      (p)->AddWorkItem(a,b,c,d)
#define ID3DX10ThreadPump_GetWorkItemCount(p)         (p)->GetWorkItemCount()
#define ID3DX10ThreadPump_WaitForAllItems(p)          (p)->WaitForAllItems()
#define ID3DX10ThreadPump_ProcessDeviceWorkItems(p,a) (p)->ProcessDeviceWorkItems(a)
#define ID3DX10ThreadPump_PurgeAllItems(p)            (p)->PurgeAllItems()
#define ID3DX10ThreadPump_GetQueueStatus(p,a,b,c)     (p)->GetQueueStatus(a,b,c)
#endif

HRESULT WINAPI D3DX10UnsetAllDeviceObjects(ID3D10Device *device);
HRESULT WINAPI D3DX10CreateDevice(IDXGIAdapter *adapter, D3D10_DRIVER_TYPE driver_type,
        HMODULE swrast, unsigned int flags, ID3D10Device **device);
HRESULT WINAPI D3DX10CreateDeviceAndSwapChain(IDXGIAdapter *adapter, D3D10_DRIVER_TYPE driver_type,
        HMODULE swrast, unsigned int flags, DXGI_SWAP_CHAIN_DESC *desc, IDXGISwapChain **swapchain,
        ID3D10Device **device);
typedef interface ID3D10Device1 ID3D10Device1;
HRESULT WINAPI D3DX10GetFeatureLevel1(ID3D10Device *device, ID3D10Device1 **device1);
