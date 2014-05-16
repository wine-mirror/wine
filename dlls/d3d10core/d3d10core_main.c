/*
 * Copyright 2008 Henri Verbeet for CodeWeavers
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

#define D3D10CORE_INIT_GUID
#include "d3d10core_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d10core);

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

HRESULT WINAPI D3D10CoreRegisterLayers(void)
{
    const struct dxgi_device_layer layers[] =
    {
        {DXGI_DEVICE_LAYER_D3D10_DEVICE, layer_init, layer_get_size, layer_create},
    };

    DXGID3D10RegisterLayers(layers, sizeof(layers)/sizeof(*layers));

    return S_OK;
}

HRESULT WINAPI D3D10CoreCreateDevice(IDXGIFactory *factory, IDXGIAdapter *adapter,
        UINT flags, void *unknown0, ID3D10Device **device)
{
    IUnknown *dxgi_device;
    HMODULE d3d10core;
    HRESULT hr;

    TRACE("factory %p, adapter %p, flags %#x, unknown0 %p, device %p.\n",
            factory, adapter, flags, unknown0, device);

    d3d10core = GetModuleHandleA("d3d10core.dll");
    hr = DXGID3D10CreateDevice(d3d10core, factory, adapter, flags, unknown0, (void **)&dxgi_device);
    if (FAILED(hr))
    {
        WARN("Failed to create device, returning %#x\n", hr);
        return hr;
    }

    hr = IUnknown_QueryInterface(dxgi_device, &IID_ID3D10Device, (void **)device);
    IUnknown_Release(dxgi_device);

    return hr;
}
