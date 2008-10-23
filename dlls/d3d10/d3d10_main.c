/*
 * Direct3D 10
 *
 * Copyright 2007 Andras Kovacs
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
#include "wine/port.h"

#include "d3d10_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d10);

/* At process attach */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    TRACE("fdwReason=%d\n", fdwReason);
    switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hInstDLL );
        break;
    }
    return TRUE;
}

HRESULT WINAPI D3D10CreateDevice(IDXGIAdapter *adapter, D3D10_DRIVER_TYPE driver_type,
        HMODULE swrast, UINT flags, UINT sdk_version, ID3D10Device **device)
{
    struct d3d10_device *object;

    FIXME("adapter %p, driver_type %s, swrast %p, flags %#x, sdk_version %d, device %p partial stub!\n",
            adapter, debug_d3d10_driver_type(driver_type), swrast, flags, sdk_version, device);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate D3D device object memory\n");
        return E_OUTOFMEMORY;
    }

    object->vtbl = &d3d10_device_vtbl;
    object->refcount = 1;
    *device = (ID3D10Device *)object;

    TRACE("Created ID3D10Device %p\n", object);

    return S_OK;
}

HRESULT WINAPI D3D10CreateDeviceAndSwapChain(IDXGIAdapter *adapter, D3D10_DRIVER_TYPE driver_type,
        HMODULE swrast, UINT flags, UINT sdk_version, DXGI_SWAP_CHAIN_DESC *swapchain_desc,
        IDXGISwapChain **swapchain, ID3D10Device **device)
{
    IDXGIFactory *factory;
    HRESULT hr;

    TRACE("adapter %p, driver_type %s, swrast %p, flags %#x, sdk_version %d,\n"
            "\tswapchain_desc %p, swapchain %p, device %p\n",
            adapter, debug_d3d10_driver_type(driver_type), swrast, flags, sdk_version,
            swapchain_desc, swapchain, device);

    hr = D3D10CreateDevice(adapter, driver_type, swrast, flags, sdk_version, device);
    if (FAILED(hr))
    {
        WARN("Failed to create a device, returning %#x\n", hr);
        return hr;
    }

    TRACE("Created ID3D10Device %p\n", *device);

    hr = CreateDXGIFactory(&IID_IDXGIFactory, (void **)&factory);
    if (FAILED(hr))
    {
        ID3D10Device_Release(*device);
        *device = NULL;

        WARN("Failed to create a DXGI factory, returning %#x\n", hr);
        return hr;
    }

    TRACE("Created IDXGIFactory %p\n", factory);

    hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)*device, swapchain_desc, swapchain);
    IDXGIFactory_Release(factory);
    if (FAILED(hr))
    {
        ID3D10Device_Release(*device);
        *device = NULL;

        WARN("Failed to create a swapchain, returning %#x\n", hr);
        return hr;
    }

    TRACE("Created IDXGISwapChain %p\n", *swapchain);

    return S_OK;
}
