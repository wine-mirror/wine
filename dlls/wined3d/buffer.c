/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
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

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE buffer_QueryInterface(IWineD3DBuffer *iface,
        REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IWineD3DBuffer)
            || IsEqualGUID(riid, &IID_IWineD3DResource)
            || IsEqualGUID(riid, &IID_IWineD3DBase)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *object = NULL;

    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE buffer_AddRef(IWineD3DBuffer *iface)
{
    struct wined3d_buffer *This = (struct wined3d_buffer *)iface;
    ULONG refcount = InterlockedIncrement(&This->resource.ref);

    TRACE("%p increasing refcount to %u\n", This, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE buffer_Release(IWineD3DBuffer *iface)
{
    struct wined3d_buffer *This = (struct wined3d_buffer *)iface;
    ULONG refcount = InterlockedDecrement(&This->resource.ref);

    TRACE("%p decreasing refcount to %u\n", This, refcount);

    if (!refcount)
    {
        resource_cleanup((IWineD3DResource *)iface);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refcount;
}

/* IWineD3DBase methods */

static HRESULT STDMETHODCALLTYPE buffer_GetParent(IWineD3DBuffer *iface, IUnknown **parent)
{
    return resource_get_parent((IWineD3DResource *)iface, parent);
}

/* IWineD3DResource methods */

static HRESULT STDMETHODCALLTYPE buffer_GetDevice(IWineD3DBuffer *iface, IWineD3DDevice **device)
{
    return resource_get_device((IWineD3DResource *)iface, device);
}

static HRESULT STDMETHODCALLTYPE buffer_SetPrivateData(IWineD3DBuffer *iface,
        REFGUID guid, const void *data, DWORD data_size, DWORD flags)
{
    return resource_set_private_data((IWineD3DResource *)iface, guid, data, data_size, flags);
}

static HRESULT STDMETHODCALLTYPE buffer_GetPrivateData(IWineD3DBuffer *iface,
        REFGUID guid, void *data, DWORD *data_size)
{
    return resource_get_private_data((IWineD3DResource *)iface, guid, data, data_size);
}

static HRESULT STDMETHODCALLTYPE buffer_FreePrivateData(IWineD3DBuffer *iface, REFGUID guid)
{
    return resource_free_private_data((IWineD3DResource *)iface, guid);
}

static DWORD STDMETHODCALLTYPE buffer_SetPriority(IWineD3DBuffer *iface, DWORD priority)
{
    return resource_set_priority((IWineD3DResource *)iface, priority);
}

static DWORD STDMETHODCALLTYPE buffer_GetPriority(IWineD3DBuffer *iface)
{
    return resource_get_priority((IWineD3DResource *)iface);
}

static void STDMETHODCALLTYPE buffer_PreLoad(IWineD3DBuffer *iface)
{
    TRACE("iface %p\n", iface);
}

static void STDMETHODCALLTYPE buffer_UnLoad(IWineD3DBuffer *iface)
{
    TRACE("iface %p\n", iface);
}

static WINED3DRESOURCETYPE STDMETHODCALLTYPE buffer_GetType(IWineD3DBuffer *iface)
{
    return resource_get_type((IWineD3DResource *)iface);
}

const struct IWineD3DBufferVtbl wined3d_buffer_vtbl =
{
    /* IUnknown methods */
    buffer_QueryInterface,
    buffer_AddRef,
    buffer_Release,
    /* IWineD3DBase methods */
    buffer_GetParent,
    /* IWineD3DResource methods */
    buffer_GetDevice,
    buffer_SetPrivateData,
    buffer_GetPrivateData,
    buffer_FreePrivateData,
    buffer_SetPriority,
    buffer_GetPriority,
    buffer_PreLoad,
    buffer_UnLoad,
    buffer_GetType,
};
