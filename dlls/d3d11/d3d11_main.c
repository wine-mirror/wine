/*
 * Direct3D 11
 *
 * Copyright 2008 Henri Verbeet for CodeWeavers
 * Copyright 2013 Austin English
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

#define D3D11_INIT_GUID
#include "d3d11_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d11);

static const char *debug_d3d_driver_type(D3D_DRIVER_TYPE driver_type)
{
    switch (driver_type)
    {
#define D3D11_TO_STR(x) case x: return #x
        D3D11_TO_STR(D3D_DRIVER_TYPE_UNKNOWN);
        D3D11_TO_STR(D3D_DRIVER_TYPE_HARDWARE);
        D3D11_TO_STR(D3D_DRIVER_TYPE_REFERENCE);
        D3D11_TO_STR(D3D_DRIVER_TYPE_NULL);
        D3D11_TO_STR(D3D_DRIVER_TYPE_SOFTWARE);
        D3D11_TO_STR(D3D_DRIVER_TYPE_WARP);
#undef D3D11_TO_STR
        default:
            return wine_dbg_sprintf("Unrecognized D3D_DRIVER_TYPE %#x\n", driver_type);
    }
}

static HRESULT WINAPI layer_init(enum dxgi_device_layer_id id, DWORD *count, DWORD *values)
{
    TRACE("id %#x, count %p, values %p\n", id, count, values);

    if (id != DXGI_DEVICE_LAYER_D3D10_DEVICE)
    {
        WARN("Unknown layer id %#x\n", id);
        return E_NOTIMPL;
    }

    return S_OK;
}

static UINT WINAPI layer_get_size(enum dxgi_device_layer_id id, struct layer_get_size_args *args, DWORD unknown0)
{
    TRACE("id %#x, args %p, unknown0 %#x\n", id, args, unknown0);

    if (id != DXGI_DEVICE_LAYER_D3D10_DEVICE)
    {
        WARN("Unknown layer id %#x\n", id);
        return 0;
    }

    return sizeof(struct d3d10_device);
}

static HRESULT WINAPI layer_create(enum dxgi_device_layer_id id, void **layer_base, DWORD unknown0,
        void *device_object, REFIID riid, void **device_layer)
{
    struct d3d10_device *object;
    HRESULT hr;

    TRACE("id %#x, layer_base %p, unknown0 %#x, device_object %p, riid %s, device_layer %p\n",
            id, layer_base, unknown0, device_object, debugstr_guid(riid), device_layer);

    if (id != DXGI_DEVICE_LAYER_D3D10_DEVICE)
    {
        WARN("Unknown layer id %#x\n", id);
        *device_layer = NULL;
        return E_NOTIMPL;
    }

    object = *layer_base;
    if (FAILED(hr = d3d10_device_init(object, device_object)))
    {
        WARN("Failed to initialize device, hr %#x.\n", hr);
        *device_layer = NULL;
        return hr;
    }
    *device_layer = &object->IUnknown_inner;

    TRACE("Created d3d10 device at %p\n", object);

    return S_OK;
}

HRESULT WINAPI D3D11CoreRegisterLayers(void)
{
    const struct dxgi_device_layer layers[] =
    {
        {DXGI_DEVICE_LAYER_D3D10_DEVICE, layer_init, layer_get_size, layer_create},
    };

    DXGID3D10RegisterLayers(layers, sizeof(layers)/sizeof(*layers));

    return S_OK;
}

HRESULT WINAPI D3D11CoreCreateDevice(IDXGIFactory *factory, IDXGIAdapter *adapter, UINT flags,
        const D3D_FEATURE_LEVEL *feature_levels, UINT levels, ID3D11Device **device)
{
    IUnknown *dxgi_device;
    HMODULE d3d11;
    HRESULT hr;

    TRACE("factory %p, adapter %p, flags %#x, feature_levels %p, levels %u, device %p.\n",
            factory, adapter, flags, feature_levels, levels, device);

    FIXME("Ignoring feature levels.\n");

    d3d11 = GetModuleHandleA("d3d11.dll");
    hr = DXGID3D10CreateDevice(d3d11, factory, adapter, flags, 0, (void **)&dxgi_device);
    if (FAILED(hr))
    {
        WARN("Failed to create device, returning %#x.\n", hr);
        return hr;
    }

    hr = IUnknown_QueryInterface(dxgi_device, &IID_ID3D11Device, (void **)device);
    IUnknown_Release(dxgi_device);
    if (FAILED(hr))
    {
        ERR("Failed to query ID3D11Device interface, returning E_FAIL.\n");
        return E_FAIL;
    }

    return S_OK;
}

HRESULT WINAPI D3D11CreateDevice(IDXGIAdapter *adapter, D3D_DRIVER_TYPE driver_type, HMODULE swrast, UINT flags,
        const D3D_FEATURE_LEVEL *feature_levels, UINT levels, UINT sdk_version, ID3D11Device **device,
        D3D_FEATURE_LEVEL *feature_level, ID3D11DeviceContext **context)
{
    FIXME("adapter %p, driver_type %s, swrast %p, flags %#x, feature_levels %p, levels %#x, sdk_version %u, "
            "device %p, feature_level %p, context %p stub!\n", adapter, debug_d3d_driver_type(driver_type), swrast,
            flags, feature_levels, levels, sdk_version, device, feature_level, context);
    return E_OUTOFMEMORY;
}

HRESULT WINAPI D3D11CreateDeviceAndSwapChain(IDXGIAdapter *adapter, D3D_DRIVER_TYPE driver_type,
        HMODULE swrast, UINT flags, const D3D_FEATURE_LEVEL *feature_levels, UINT levels,
        UINT sdk_version, const DXGI_SWAP_CHAIN_DESC *swapchain_desc, IDXGISwapChain **swapchain,
        ID3D11Device **device, D3D_FEATURE_LEVEL *feature_level, ID3D11DeviceContext **immediate_context)
{
    FIXME("adapter %p, driver_type %s, swrast %p, flags %#x, feature_levels %p, levels %#x, sdk_version %u, "
            "swapchain_desc %p, swapchain %p, device %p, feature_level %p, immediate_context %p stub!\n",
            adapter, debug_d3d_driver_type(driver_type), swrast, flags, feature_levels, levels, sdk_version,
            swapchain_desc, swapchain, device, feature_level, immediate_context);
    return E_NOTIMPL;
}
