/* d3d11_x.dll — Xbox One ERA Direct3D 11 extension DLL
 *
 * Maps Xbox-specific D3D11X entry points to Wine's d3d11.dll.
 * Reference: WinDurango/WinDurango projects/WinDurango.D3D11X (MIT)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#include <stdarg.h>
#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "d3d11.h"
#include "dxgi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d11x);

/* IID data exports (matched from WinDurango IIDExports.cpp) */
const GUID IID_ID3D11DeviceContextX = {0x48800095,0x7134,0x4BE7,{0x91,0x86,0xb8,0x6B,0xEC,0xB2,0x40,0x4D}};
const GUID IID_ID3D11DeviceX        = {0x177700F9,0x876A,0x4436,{0xB3,0x68,0x36,0xA6,0x04,0xF8,0x2C,0xEF}};
const GUID IID_ID3D11Texture2D      = {0x6f15AAF2,0xD208,0x4E89,{0x9A,0xB4,0x30,0x5F,0x35,0xD3,0x4F,0x9C}};
const GUID IID_IDXGIFactory2        = {0x50C83A1C,0xE072,0x4C48,{0x87,0xB0,0x36,0x30,0xFA,0x36,0xA6,0xD0}};
const GUID IID_IDXGIDevice          = {0x54EC77FA,0x1377,0x44E6,{0x8C,0x32,0x88,0xFD,0x5F,0x44,0xC8,0x4C}};
const GUID IID_IDXGIDevice1         = {0x77DB970F,0x6276,0x48BA,{0xBA,0x28,0x07,0x01,0x43,0xB4,0x39,0x2C}};

typedef struct D3D11X_CREATE_DEVICE_PARAMETERS
{
    UINT Version;
    UINT Flags;
    void *pOffchipTessellationBuffer;
    void *pTessellationFactorsBuffer;
    UINT DeferredDeletionThreadAffinityMask;
    UINT ImmediateContextDeRingSizeBytes;
    UINT ImmediateContextCeRingSizeBytes;
    UINT ImmediateContextDeSegmentSizeBytes;
    UINT ImmediateContextCeSegmentSizeBytes;
} D3D11X_CREATE_DEVICE_PARAMETERS;

typedef struct DXGIX_FRAME_STATISTICS
{
    UINT64 CPUTimePresentCalled;
    UINT64 CPUTimeAddedToQueue;
    UINT   QueueLengthAddedToQueue;
    UINT64 CPUTimeFrameComplete;
    UINT64 GPUTimeFrameComplete;
    UINT64 GPUCountTitleUsed;
    UINT64 GPUCountSystemUsed;
    UINT64 CPUTimeVSync;
    UINT64 GPUTimeVSync;
    UINT64 CPUTimeFlip;
    UINT64 GPUTimeFlip;
    UINT64 VSyncCount;
    float  PercentScanned;
    void  *Cookie[2];
} DXGIX_FRAME_STATISTICS;

typedef struct DXGIX_PRESENTARRAY_PARAMETERS
{
    BOOL   Disable;
    BOOL   UsePreviousBuffer;
    RECT   SourceRect;
    POINT  DestRectUpperLeft;
    FLOAT  ScaleFactorVert;
    FLOAT  ScaleFactorHorz;
    void  *Cookie;
    UINT   Flags;
} DXGIX_PRESENTARRAY_PARAMETERS;

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    TRACE("inst %p, reason %lu, reserved %p\n", inst, reason, reserved);
    return TRUE;
}

/***********************************************************************
 *   D3D10CreateBlob  (d3d11_x.@)
 */
HRESULT WINAPI D3D10CreateBlob(SIZE_T size, ID3DBlob **ppBlob)
{
    FIXME("size %lu, ppBlob %p stub!\n", size, ppBlob);
    return E_NOTIMPL;
}

/***********************************************************************
 *   D3D11CreateDevice  (d3d11_x.@)
 *
 * Xbox variant — same signature as PC, forwarded to d3d11.dll.
 */
HRESULT WINAPI D3D11CreateDevice(IDXGIAdapter *pAdapter, D3D_DRIVER_TYPE DriverType,
        HMODULE Software, UINT Flags, const D3D_FEATURE_LEVEL *pFeatureLevels,
        UINT FeatureLevels, UINT SDKVersion, ID3D11Device **ppDevice,
        D3D_FEATURE_LEVEL *pFeatureLevel, ID3D11DeviceContext **ppImmediateContext)
{
    static HRESULT (WINAPI *p_D3D11CreateDevice)(IDXGIAdapter*, D3D_DRIVER_TYPE,
            HMODULE, UINT, const D3D_FEATURE_LEVEL*, UINT, UINT,
            ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);

    TRACE("pAdapter %p, DriverType %d, Flags %#x\n", pAdapter, DriverType, Flags);

    if (!p_D3D11CreateDevice)
    {
        HMODULE hd3d11 = LoadLibraryA("d3d11.dll");
        if (!hd3d11) return E_FAIL;
        p_D3D11CreateDevice = (void *)GetProcAddress(hd3d11, "D3D11CreateDevice");
        if (!p_D3D11CreateDevice) return E_FAIL;
    }
    return p_D3D11CreateDevice(pAdapter, DriverType, Software, Flags,
            pFeatureLevels, FeatureLevels, SDKVersion,
            ppDevice, pFeatureLevel, ppImmediateContext);
}

/***********************************************************************
 *   D3D11CreateDeviceAndSwapChain  (d3d11_x.@)
 */
HRESULT WINAPI D3D11CreateDeviceAndSwapChain(IDXGIAdapter *pAdapter,
        D3D_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags,
        const D3D_FEATURE_LEVEL *pFeatureLevels, UINT FeatureLevels,
        UINT SDKVersion, const DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
        IDXGISwapChain **ppSwapChain, ID3D11Device **ppDevice,
        D3D_FEATURE_LEVEL *pFeatureLevel, ID3D11DeviceContext **ppImmediateContext)
{
    static HRESULT (WINAPI *p_D3D11CreateDeviceAndSwapChain)(IDXGIAdapter*,
            D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL*, UINT, UINT,
            const DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**, ID3D11Device**,
            D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);

    TRACE("pAdapter %p, DriverType %d\n", pAdapter, DriverType);

    if (!p_D3D11CreateDeviceAndSwapChain)
    {
        HMODULE hd3d11 = LoadLibraryA("d3d11.dll");
        if (!hd3d11) return E_FAIL;
        p_D3D11CreateDeviceAndSwapChain = (void *)GetProcAddress(hd3d11,
                "D3D11CreateDeviceAndSwapChain");
        if (!p_D3D11CreateDeviceAndSwapChain) return E_FAIL;
    }
    return p_D3D11CreateDeviceAndSwapChain(pAdapter, DriverType, Software, Flags,
            pFeatureLevels, FeatureLevels, SDKVersion, pSwapChainDesc,
            ppSwapChain, ppDevice, pFeatureLevel, ppImmediateContext);
}

/***********************************************************************
 *   D3D11XCreateDeviceX  (d3d11_x.@)
 *
 * Xbox ERA device creation entry point. pParameters contains Xbox-specific
 * ring buffer sizing; ignored on PC — we just create a standard D3D11 device.
 */
HRESULT WINAPI D3D11XCreateDeviceX(const D3D11X_CREATE_DEVICE_PARAMETERS *pParameters,
        ID3D11Device **ppDevice, ID3D11DeviceContext **ppImmediateContext)
{
    D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL_11_0;
    UINT flags = pParameters ? pParameters->Flags : 0;

    TRACE("pParameters %p, ppDevice %p, ppContext %p\n",
          pParameters, ppDevice, ppImmediateContext);

    /* Strip Xbox-only flags unknown to Wine's D3D11 */
    flags &= ~0xFF000000u;

    return D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags,
            &level, 1, D3D11_SDK_VERSION, ppDevice, NULL, ppImmediateContext);
}

/***********************************************************************
 *   D3D11XCreateDeviceXAndSwapChain1  (d3d11_x.@)
 */
HRESULT WINAPI D3D11XCreateDeviceXAndSwapChain1(
        const D3D11X_CREATE_DEVICE_PARAMETERS *pParameters,
        const void *pSwapChainDesc,
        void **ppSwapChain, ID3D11Device **ppDevice,
        ID3D11DeviceContext **ppImmediateContext)
{
    FIXME("pParameters %p, pSwapChainDesc %p — swap chain creation not implemented\n",
          pParameters, pSwapChainDesc);

    /* Create device; swap chain not yet implemented on Wine */
    return D3D11XCreateDeviceX(pParameters, ppDevice, ppImmediateContext);
}

/***********************************************************************
 *   D3DAllocateGraphicsMemory  (d3d11_x.@)
 *
 * Allocates GPU-accessible memory. On PC we just use VirtualAlloc.
 */
HRESULT WINAPI D3DAllocateGraphicsMemory(SIZE_T SizeInBytes, UINT64 AlignmentBytes,
        UINT64 DesiredGpuVirtualAddress, UINT Flags, void **ppAddress)
{
    DWORD flAllocationType = MEM_RESERVE | MEM_COMMIT;
    DWORD flProtect = PAGE_READWRITE;

    TRACE("size %lu, align %I64u, desired %I64x, flags %#x\n",
          SizeInBytes, AlignmentBytes, DesiredGpuVirtualAddress, Flags);

    if (!ppAddress || AlignmentBytes > 0x20000)
        return E_INVALIDARG;

    switch (Flags & 0x3)
    {
    case 0: /* CPU_CACHE_COHERENT */   flProtect = PAGE_READWRITE; break;
    case 1: /* CPU_WRITECOMBINE */     flProtect = PAGE_WRITECOMBINE; break;
    case 2: /* CPU_NONCOHERENT_GPURO*/flProtect = PAGE_READONLY; break;
    default: break;
    }

    *ppAddress = VirtualAlloc((void *)(ULONG_PTR)DesiredGpuVirtualAddress,
            SizeInBytes, flAllocationType, flProtect);
    if (!*ppAddress)
    {
        /* Retry without desired address hint */
        *ppAddress = VirtualAlloc(NULL, SizeInBytes, flAllocationType, flProtect);
    }
    if (!*ppAddress) return E_OUTOFMEMORY;
    return S_OK;
}

/***********************************************************************
 *   D3DFreeGraphicsMemory  (d3d11_x.@)
 */
HRESULT WINAPI D3DFreeGraphicsMemory(void *pAddress)
{
    TRACE("pAddress %p\n", pAddress);
    if (pAddress) VirtualFree(pAddress, 0, MEM_RELEASE);
    return S_OK;
}

/***********************************************************************
 *   D3DConfigureVirtualMemory  (d3d11_x.@)
 */
HRESULT WINAPI D3DConfigureVirtualMemory(UINT64 Flags)
{
    FIXME("flags %I64x stub!\n", Flags);
    return S_OK;
}

/***********************************************************************
 *   D3DMapEsramMemory  (d3d11_x.@)
 *
 * Xbox ESRAM mapping — no ESRAM on PC, return stub success.
 */
HRESULT WINAPI D3DMapEsramMemory(UINT Flags, void *pVirtualAddress,
        UINT NumPages, const UINT *pPageList)
{
    FIXME("flags %#x, addr %p, pages %u stub!\n", Flags, pVirtualAddress, NumPages);
    return S_OK;
}

/***********************************************************************
 *   DXGIXGetFrameStatistics  (d3d11_x.@)
 */
HRESULT WINAPI DXGIXGetFrameStatistics(UINT NumberFramesRequested,
        DXGIX_FRAME_STATISTICS *pFrameStatistics)
{
    TRACE("count %u, stats %p\n", NumberFramesRequested, pFrameStatistics);
    if (pFrameStatistics)
        memset(pFrameStatistics, 0,
               NumberFramesRequested * sizeof(DXGIX_FRAME_STATISTICS));
    return S_OK;
}

/***********************************************************************
 *   DXGIXPresentArray  (d3d11_x.@)
 *
 * Presents multiple swap chains in a single call.
 */
HRESULT WINAPI DXGIXPresentArray(UINT SyncInterval, UINT PresentImmediateThreshold,
        UINT Flags, UINT NumSwapChains, IDXGISwapChain **ppSwapChains,
        const DXGIX_PRESENTARRAY_PARAMETERS *pPresentParameters)
{
    HRESULT hr = S_OK;
    UINT i;

    TRACE("sync %u, threshold %u, flags %#x, count %u\n",
          SyncInterval, PresentImmediateThreshold, Flags, NumSwapChains);

    for (i = 0; i < NumSwapChains; i++)
    {
        if (ppSwapChains[i])
        {
            HRESULT cur = IDXGISwapChain_Present(ppSwapChains[i], SyncInterval, Flags);
            if (FAILED(cur)) hr = cur;
        }
    }
    return hr;
}

/***********************************************************************
 *   DXGIXSetVLineNotification  (d3d11_x.@)
 */
HRESULT WINAPI DXGIXSetVLineNotification(UINT VLineCounter, UINT VLineNum, HANDLE hEvent)
{
    FIXME("counter %u, vline %u, event %p stub!\n", VLineCounter, VLineNum, hEvent);
    return S_OK;
}
