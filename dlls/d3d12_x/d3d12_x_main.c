/* d3d12_x.dll — Xbox One ERA Direct3D 12 extension DLL
 *
 * Maps Xbox-specific D3D12X entry points to Wine's d3d12.dll.
 * Reference: WinDurango/WinDurango projects/WinDurango.D3D12X (MIT)
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
#include "d3d12.h"
#include "dxgi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d12x);

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
 *   D3D12CreateDevice  (d3d12_x.@)
 */
HRESULT WINAPI D3D12CreateDevice(IUnknown *pAdapter,
        D3D_FEATURE_LEVEL MinimumFeatureLevel, REFIID riid, void **ppDevice)
{
    static HRESULT (WINAPI *p_D3D12CreateDevice)(IUnknown*, D3D_FEATURE_LEVEL, REFIID, void**);

    TRACE("pAdapter %p, level %#x, riid %s, ppDevice %p\n",
          pAdapter, MinimumFeatureLevel, debugstr_guid(riid), ppDevice);

    if (!p_D3D12CreateDevice)
    {
        HMODULE hd3d12 = LoadLibraryA("d3d12.dll");
        if (!hd3d12) return E_FAIL;
        p_D3D12CreateDevice = (void *)GetProcAddress(hd3d12, "D3D12CreateDevice");
        if (!p_D3D12CreateDevice) return E_FAIL;
    }
    return p_D3D12CreateDevice(pAdapter, MinimumFeatureLevel, riid, ppDevice);
}

/***********************************************************************
 *   D3D12GetDebugInterface  (d3d12_x.@)
 */
HRESULT WINAPI D3D12GetDebugInterface(REFIID riid, void **ppvDebug)
{
    static HRESULT (WINAPI *p_D3D12GetDebugInterface)(REFIID, void**);

    TRACE("riid %s, ppvDebug %p\n", debugstr_guid(riid), ppvDebug);

    if (!p_D3D12GetDebugInterface)
    {
        HMODULE hd3d12 = LoadLibraryA("d3d12.dll");
        if (!hd3d12) return E_FAIL;
        p_D3D12GetDebugInterface = (void *)GetProcAddress(hd3d12,
                "D3D12GetDebugInterface");
        if (!p_D3D12GetDebugInterface) return E_FAIL;
    }
    return p_D3D12GetDebugInterface(riid, ppvDebug);
}

/***********************************************************************
 *   D3D12SerializeRootSignature  (d3d12_x.@)
 */
HRESULT WINAPI D3D12SerializeRootSignature(const D3D12_ROOT_SIGNATURE_DESC *pRootSignature,
        D3D_ROOT_SIGNATURE_VERSION Version, ID3DBlob **ppBlob, ID3DBlob **ppErrorBlob)
{
    static HRESULT (WINAPI *p_D3D12SerializeRootSignature)(
            const D3D12_ROOT_SIGNATURE_DESC*, D3D_ROOT_SIGNATURE_VERSION,
            ID3DBlob**, ID3DBlob**);

    TRACE("pRootSignature %p, version %d\n", pRootSignature, Version);

    if (!p_D3D12SerializeRootSignature)
    {
        HMODULE hd3d12 = LoadLibraryA("d3d12.dll");
        if (!hd3d12) return E_FAIL;
        p_D3D12SerializeRootSignature = (void *)GetProcAddress(hd3d12,
                "D3D12SerializeRootSignature");
        if (!p_D3D12SerializeRootSignature) return E_FAIL;
    }
    return p_D3D12SerializeRootSignature(pRootSignature, Version, ppBlob, ppErrorBlob);
}

/***********************************************************************
 *   D3D12XboxSetProcessDebugFlags  (d3d12_x.@)
 */
HRESULT WINAPI D3D12XboxSetProcessDebugFlags(UINT Flags)
{
    FIXME("flags %#x stub!\n", Flags);
    return S_OK;
}

/***********************************************************************
 *   D3DMapEsramMemory  (d3d12_x.@)
 */
HRESULT WINAPI D3DMapEsramMemory(UINT Flags, void *pVirtualAddress,
        UINT NumPages, const UINT *pPageList)
{
    FIXME("flags %#x, addr %p, pages %u stub!\n", Flags, pVirtualAddress, NumPages);
    return S_OK;
}

/***********************************************************************
 *   DXGIXPresentArray  (d3d12_x.@)
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
 *   DXGIXSetVLineNotification  (d3d12_x.@)
 */
HRESULT WINAPI DXGIXSetVLineNotification(UINT VLineCounter, UINT VLineNum, HANDLE hEvent)
{
    TRACE("counter %u, vline %u, event %p\n", VLineCounter, VLineNum, hEvent);

    /* Xbox validation: VLineCounter must be < 2; if 0 then VLineNum must be 0;
     * if event provided, VLineNum must be < 0x465 (1125 — Xbox One scanlines) */
    if (VLineCounter >= 2 || (!VLineCounter && VLineNum) ||
        (hEvent && VLineNum >= 0x465))
        return E_INVALIDARG;

    return S_OK;
}
