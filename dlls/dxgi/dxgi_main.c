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

#include "dxgi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxgi);

static CRITICAL_SECTION dxgi_cs;
static CRITICAL_SECTION_DEBUG dxgi_cs_debug =
{
    0, 0, &dxgi_cs,
    {&dxgi_cs_debug.ProcessLocksList,
    &dxgi_cs_debug.ProcessLocksList},
    0, 0, {(DWORD_PTR)(__FILE__ ": dxgi_cs")}
};
static CRITICAL_SECTION dxgi_cs = {&dxgi_cs_debug, -1, 0, 0, 0, 0};

struct dxgi_main
{
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

    FIXME("riid %s, factory %p partial stub!\n", debugstr_guid(riid), factory);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate DXGI factory object memory\n");
        return E_OUTOFMEMORY;
    }

    object->vtbl = &dxgi_factory_vtbl;
    object->refcount = 1;

    TRACE("Created IDXGIFactory %p\n", object);

    hr = IDXGIFactory_QueryInterface((IDXGIFactory *)object, riid, factory);
    IDXGIFactory_Release((IDXGIFactory *)object);

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
