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

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    TRACE("fdwReason %u\n", fdwReason);

    switch(fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstDLL);
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
    FIXME("layers %p, layer_count %u stub!\n", layers, layer_count);

    return E_NOTIMPL;
}
