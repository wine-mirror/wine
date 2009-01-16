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

#define DXGI_INIT_GUID
#include "dxgi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxgi);

static CRITICAL_SECTION_DEBUG dxgi_cs_debug =
{
    0, 0, &dxgi_cs,
    {&dxgi_cs_debug.ProcessLocksList,
    &dxgi_cs_debug.ProcessLocksList},
    0, 0, {(DWORD_PTR)(__FILE__ ": dxgi_cs")}
};
CRITICAL_SECTION dxgi_cs = {&dxgi_cs_debug, -1, 0, 0, 0, 0};

struct dxgi_main
{
    HMODULE d3d10core;
    struct dxgi_device_layer *device_layers;
    UINT layer_count;
    LONG refcount;
};
static struct dxgi_main dxgi_main;

static void dxgi_main_cleanup(void)
{
    EnterCriticalSection(&dxgi_cs);

    HeapFree(GetProcessHeap(), 0, dxgi_main.device_layers);
    dxgi_main.device_layers = NULL;
    dxgi_main.layer_count = 0;

    FreeLibrary(dxgi_main.d3d10core);
    dxgi_main.d3d10core = NULL;

    LeaveCriticalSection(&dxgi_cs);
}

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    TRACE("fdwReason %u\n", fdwReason);

    switch(fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstDLL);
            ++dxgi_main.refcount;
            break;

        case DLL_PROCESS_DETACH:
            if (!--dxgi_main.refcount) dxgi_main_cleanup();
            break;
    }

    return TRUE;
}

HRESULT WINAPI CreateDXGIFactory(REFIID riid, void **factory)
{
    struct dxgi_factory *object;
    HRESULT hr;
    UINT i;

    TRACE("riid %s, factory %p\n", debugstr_guid(riid), factory);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate DXGI factory object memory\n");
        *factory = NULL;
        return E_OUTOFMEMORY;
    }

    object->vtbl = &dxgi_factory_vtbl;
    object->refcount = 1;

    EnterCriticalSection(&dxgi_cs);
    object->wined3d = WineDirect3DCreate(10, (IUnknown *)object);
    if(!object->wined3d)
    {
        hr = DXGI_ERROR_UNSUPPORTED;
        LeaveCriticalSection(&dxgi_cs);
        goto fail;
    }

    object->adapter_count = IWineD3D_GetAdapterCount(object->wined3d);
    LeaveCriticalSection(&dxgi_cs);
    object->adapters = HeapAlloc(GetProcessHeap(), 0, object->adapter_count * sizeof(*object->adapters));
    if (!object->adapters)
    {
        ERR("Failed to allocate DXGI adapter array memory\n");
        hr = E_OUTOFMEMORY;
        goto fail;
    }

    for (i = 0; i < object->adapter_count; ++i)
    {
        struct dxgi_adapter *adapter = HeapAlloc(GetProcessHeap(), 0, sizeof(*adapter));
        if (!adapter)
        {
            UINT j;
            ERR("Failed to allocate DXGI adapter memory\n");
            for (j = 0; j < i; ++j)
            {
                HeapFree(GetProcessHeap(), 0, object->adapters[j]);
            }
            hr = E_OUTOFMEMORY;
            goto fail;
        }

        adapter->vtbl = &dxgi_adapter_vtbl;
        adapter->refcount = 1;
        adapter->ordinal = i;
        adapter->parent = (IDXGIFactory *)object;
        object->adapters[i] = (IDXGIAdapter *)adapter;
    }

    TRACE("Created IDXGIFactory %p\n", object);

    hr = IDXGIFactory_QueryInterface((IDXGIFactory *)object, riid, factory);
    IDXGIFactory_Release((IDXGIFactory *)object);

    return hr;

fail:
    HeapFree(GetProcessHeap(), 0, object->adapters);
    if (object->wined3d)
    {
        EnterCriticalSection(&dxgi_cs);
        IWineD3D_Release(object->wined3d);
        LeaveCriticalSection(&dxgi_cs);
    }
    HeapFree(GetProcessHeap(), 0, object);
    *factory = NULL;
    return hr;

}

static BOOL get_layer(enum dxgi_device_layer_id id, struct dxgi_device_layer *layer)
{
    UINT i;

    EnterCriticalSection(&dxgi_cs);

    for (i = 0; i < dxgi_main.layer_count; ++i)
    {
        if (dxgi_main.device_layers[i].id == id)
        {
            *layer = dxgi_main.device_layers[i];
            LeaveCriticalSection(&dxgi_cs);
            return TRUE;
        }
    }

    LeaveCriticalSection(&dxgi_cs);
    return FALSE;
}

static HRESULT register_d3d10core_layers(HMODULE d3d10core)
{
    EnterCriticalSection(&dxgi_cs);

    if (!dxgi_main.d3d10core)
    {
        HRESULT hr;
        HRESULT (WINAPI *d3d10core_register_layers)(void);
        HMODULE mod;
        BOOL ret;

        ret = GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)d3d10core, &mod);
        if (!ret)
        {
            LeaveCriticalSection(&dxgi_cs);
            return E_FAIL;
        }

        d3d10core_register_layers = (HRESULT (WINAPI *)(void))GetProcAddress(mod, "D3D10CoreRegisterLayers");
        hr = d3d10core_register_layers();
        if (FAILED(hr))
        {
            ERR("Failed to register d3d10core layers, returning %#x\n", hr);
            LeaveCriticalSection(&dxgi_cs);
            return hr;
        }

        dxgi_main.d3d10core = mod;
    }

    LeaveCriticalSection(&dxgi_cs);

    return S_OK;
}

HRESULT WINAPI DXGID3D10CreateDevice(HMODULE d3d10core, IDXGIFactory *factory, IDXGIAdapter *adapter,
        UINT flags, DWORD unknown0, void **device)
{
    IWineD3DDeviceParent *wined3d_device_parent;
    struct layer_get_size_args get_size_args;
    struct dxgi_device *dxgi_device;
    struct dxgi_device_layer d3d10_layer;
    IWineDXGIAdapter *wine_adapter;
    UINT adapter_ordinal;
    IWineD3D *wined3d;
    void *layer_base;
    UINT device_size;
    DWORD count;
    HRESULT hr;

    TRACE("d3d10core %p, factory %p, adapter %p, flags %#x, unknown0 %#x, device %p\n",
            d3d10core, factory, adapter, flags, unknown0, device);

    hr = register_d3d10core_layers(d3d10core);
    if (FAILED(hr))
    {
        ERR("Failed to register d3d10core layers, returning %#x\n", hr);
        return hr;
    }

    if (!get_layer(DXGI_DEVICE_LAYER_D3D10_DEVICE, &d3d10_layer))
    {
        ERR("Failed to get D3D10 device layer\n");
        return E_FAIL;
    }

    count = 0;
    hr = d3d10_layer.init(d3d10_layer.id, &count, NULL);
    if (FAILED(hr))
    {
        WARN("Failed to initialize D3D10 device layer\n");
        return E_FAIL;
    }

    get_size_args.unknown0 = 0;
    get_size_args.unknown1 = 0;
    get_size_args.unknown2 = NULL;
    get_size_args.unknown3 = NULL;
    get_size_args.adapter = adapter;
    get_size_args.interface_major = 10;
    get_size_args.interface_minor = 1;
    get_size_args.version_build = 4;
    get_size_args.version_revision = 6000;

    device_size = d3d10_layer.get_size(d3d10_layer.id, &get_size_args, 0);
    device_size += sizeof(*dxgi_device);

    dxgi_device = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, device_size);
    if (!dxgi_device)
    {
        ERR("Failed to allocate device memory\n");
        return E_OUTOFMEMORY;
    }

    dxgi_device->vtbl = &dxgi_device_vtbl;
    dxgi_device->refcount = 1;

    layer_base = dxgi_device + 1;

    hr = d3d10_layer.create(d3d10_layer.id, &layer_base, 0,
            dxgi_device, &IID_IUnknown, (void **)&dxgi_device->child_layer);
    if (FAILED(hr))
    {
        WARN("Failed to create device, returning %#x\n", hr);
        goto fail;
    }

    hr = IDXGIFactory_QueryInterface(factory, &IID_IWineDXGIFactory, (void **)&dxgi_device->factory);
    if (FAILED(hr))
    {
        WARN("This is not the factory we're looking for, returning %#x\n", hr);
        goto fail;
    }
    wined3d = IWineDXGIFactory_get_wined3d(dxgi_device->factory);

    hr = IDXGIAdapter_QueryInterface(adapter, &IID_IWineDXGIAdapter, (void **)&wine_adapter);
    if (FAILED(hr))
    {
        WARN("This is not the adapter we're looking for, returning %#x\n", hr);
        EnterCriticalSection(&dxgi_cs);
        IWineD3D_Release(wined3d);
        LeaveCriticalSection(&dxgi_cs);
        goto fail;
    }
    adapter_ordinal = IWineDXGIAdapter_get_ordinal(wine_adapter);
    IWineDXGIAdapter_Release(wine_adapter);

    hr = IUnknown_QueryInterface((IUnknown *)dxgi_device, &IID_IWineD3DDeviceParent, (void **)&wined3d_device_parent);
    if (FAILED(hr))
    {
        ERR("DXGI device should implement IWineD3DDeviceParent\n");
        goto fail;
    }

    FIXME("Ignoring adapter type\n");
    EnterCriticalSection(&dxgi_cs);
    hr = IWineD3D_CreateDevice(wined3d, adapter_ordinal, WINED3DDEVTYPE_HAL, NULL, 0,
            (IUnknown *)dxgi_device, wined3d_device_parent, &dxgi_device->wined3d_device);
    IWineD3DDeviceParent_Release(wined3d_device_parent);
    IWineD3D_Release(wined3d);
    LeaveCriticalSection(&dxgi_cs);
    if (FAILED(hr))
    {
        WARN("Failed to create a WineD3D device, returning %#x\n", hr);
        goto fail;
    }

    *device = dxgi_device;

    return hr;

fail:
    if (dxgi_device->wined3d_device)
    {
        EnterCriticalSection(&dxgi_cs);
        IWineD3DDevice_Release(dxgi_device->wined3d_device);
        LeaveCriticalSection(&dxgi_cs);
    }
    if (dxgi_device->factory) IWineDXGIFactory_Release(dxgi_device->factory);
    if (dxgi_device->child_layer) IUnknown_Release(dxgi_device->child_layer);
    HeapFree(GetProcessHeap(), 0, dxgi_device);
    *device = NULL;
    return hr;
}

HRESULT WINAPI DXGID3D10RegisterLayers(const struct dxgi_device_layer *layers, UINT layer_count)
{
    UINT i;
    struct dxgi_device_layer *new_layers;

    TRACE("layers %p, layer_count %u\n", layers, layer_count);

    EnterCriticalSection(&dxgi_cs);

    if (!dxgi_main.layer_count)
        new_layers = HeapAlloc(GetProcessHeap(), 0, layer_count * sizeof(*new_layers));
    else
        new_layers = HeapReAlloc(GetProcessHeap(), 0, dxgi_main.device_layers,
                (dxgi_main.layer_count + layer_count) * sizeof(*new_layers));

    if (!new_layers)
    {
        LeaveCriticalSection(&dxgi_cs);
        ERR("Failed to allocate layer memory\n");
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < layer_count; ++i)
    {
        const struct dxgi_device_layer *layer = &layers[i];

        TRACE("layer %d: id %#x, init %p, get_size %p, create %p\n",
                i, layer->id, layer->init, layer->get_size, layer->create);

        new_layers[dxgi_main.layer_count + i] = *layer;
    }

    dxgi_main.device_layers = new_layers;
    dxgi_main.layer_count += layer_count;

    LeaveCriticalSection(&dxgi_cs);

    return S_OK;
}
