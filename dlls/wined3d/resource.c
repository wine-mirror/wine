/*
 * IWineD3DResource Implementation
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2009-2010 Henri Verbeet for CodeWeavers
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

#include "config.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

struct private_data
{
    struct list entry;

    GUID tag;
    DWORD flags; /* DDSPD_* */

    union
    {
        void *data;
        IUnknown *object;
    } ptr;

    DWORD size;
};

HRESULT resource_init(struct IWineD3DResourceImpl *resource, WINED3DRESOURCETYPE resource_type,
        IWineD3DDeviceImpl *device, UINT size, DWORD usage, const struct wined3d_format *format,
        WINED3DPOOL pool, void *parent, const struct wined3d_parent_ops *parent_ops)
{
    struct IWineD3DResourceClass *r = &resource->resource;

    r->device = device;
    r->resourceType = resource_type;
    r->ref = 1;
    r->pool = pool;
    r->format = format;
    r->usage = usage;
    r->size = size;
    r->priority = 0;
    r->parent = parent;
    r->parent_ops = parent_ops;
    list_init(&r->privateData);

    if (size)
    {
        r->heapMemory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size + RESOURCE_ALIGNMENT);
        if (!r->heapMemory)
        {
            ERR("Out of memory!\n");
            return WINED3DERR_OUTOFVIDEOMEMORY;
        }
    }
    else
    {
        r->heapMemory = NULL;
    }
    r->allocatedMemory = (BYTE *)(((ULONG_PTR)r->heapMemory + (RESOURCE_ALIGNMENT - 1)) & ~(RESOURCE_ALIGNMENT - 1));

    /* Check that we have enough video ram left */
    if (pool == WINED3DPOOL_DEFAULT)
    {
        if (size > IWineD3DDevice_GetAvailableTextureMem((IWineD3DDevice *)device))
        {
            ERR("Out of adapter memory\n");
            HeapFree(GetProcessHeap(), 0, r->heapMemory);
            return WINED3DERR_OUTOFVIDEOMEMORY;
        }
        WineD3DAdapterChangeGLRam(device, size);
    }

    device_resource_add(device, resource);

    return WINED3D_OK;
}

void resource_cleanup(struct IWineD3DResourceImpl *resource)
{
    struct private_data *data;
    struct list *e1, *e2;
    HRESULT hr;

    TRACE("Cleaning up resource %p.\n", resource);

    if (resource->resource.pool == WINED3DPOOL_DEFAULT)
    {
        TRACE("Decrementing device memory pool by %u.\n", resource->resource.size);
        WineD3DAdapterChangeGLRam(resource->resource.device, -resource->resource.size);
    }

    LIST_FOR_EACH_SAFE(e1, e2, &resource->resource.privateData)
    {
        data = LIST_ENTRY(e1, struct private_data, entry);
        hr = resource_free_private_data(resource, &data->tag);
        if (FAILED(hr))
            ERR("Failed to free private data when destroying resource %p, hr = %#x.\n", resource, hr);
    }

    HeapFree(GetProcessHeap(), 0, resource->resource.heapMemory);
    resource->resource.allocatedMemory = 0;
    resource->resource.heapMemory = 0;

    if (resource->resource.device)
        device_resource_released(resource->resource.device, (IWineD3DResource *)resource);
}

void resource_unload(IWineD3DResourceImpl *resource)
{
    context_resource_unloaded(resource->resource.device, (IWineD3DResource *)resource,
            resource->resource.resourceType);
}

static struct private_data *resource_find_private_data(IWineD3DResourceImpl *This, REFGUID tag)
{
    struct private_data *data;
    struct list *entry;

    TRACE("Searching for private data %s\n", debugstr_guid(tag));
    LIST_FOR_EACH(entry, &This->resource.privateData)
    {
        data = LIST_ENTRY(entry, struct private_data, entry);
        if (IsEqualGUID(&data->tag, tag)) {
            TRACE("Found %p\n", data);
            return data;
        }
    }
    TRACE("Not found\n");
    return NULL;
}

HRESULT resource_set_private_data(struct IWineD3DResourceImpl *resource, REFGUID guid,
        const void *data, DWORD data_size, DWORD flags)
{
    struct private_data *d;

    TRACE("resource %p, riid %s, data %p, data_size %u, flags %#x.\n",
            resource, debugstr_guid(guid), data, data_size, flags);

    resource_free_private_data(resource, guid);

    d = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*d));
    if (!d) return E_OUTOFMEMORY;

    d->tag = *guid;
    d->flags = flags;

    if (flags & WINED3DSPD_IUNKNOWN)
    {
        if (data_size != sizeof(IUnknown *))
        {
            WARN("IUnknown data with size %u, returning WINED3DERR_INVALIDCALL.\n", data_size);
            HeapFree(GetProcessHeap(), 0, d);
            return WINED3DERR_INVALIDCALL;
        }
        d->ptr.object = (IUnknown *)data;
        d->size = sizeof(IUnknown *);
        IUnknown_AddRef(d->ptr.object);
    }
    else
    {
        d->ptr.data = HeapAlloc(GetProcessHeap(), 0, data_size);
        if (!d->ptr.data)
        {
            HeapFree(GetProcessHeap(), 0, d);
            return E_OUTOFMEMORY;
        }
        d->size = data_size;
        memcpy(d->ptr.data, data, data_size);
    }
    list_add_tail(&resource->resource.privateData, &d->entry);

    return WINED3D_OK;
}

HRESULT resource_get_private_data(struct IWineD3DResourceImpl *resource, REFGUID guid, void *data, DWORD *data_size)
{
    const struct private_data *d;

    TRACE("resource %p, guid %s, data %p, data_size %p.\n",
            resource, debugstr_guid(guid), data, data_size);

    d = resource_find_private_data(resource, guid);
    if (!d) return WINED3DERR_NOTFOUND;

    if (*data_size < d->size)
    {
        *data_size = d->size;
        return WINED3DERR_MOREDATA;
    }

    if (d->flags & WINED3DSPD_IUNKNOWN)
    {
        *(IUnknown **)data = d->ptr.object;
        if (((IWineD3DImpl *)resource->resource.device->wined3d)->dxVersion != 7)
        {
            /* D3D8 and D3D9 addref the private data, DDraw does not. This
             * can't be handled in ddraw because it doesn't know if the
             * pointer returned is an IUnknown * or just a blob. */
            IUnknown_AddRef(d->ptr.object);
        }
    }
    else
    {
        memcpy(data, d->ptr.data, d->size);
    }

    return WINED3D_OK;
}
HRESULT resource_free_private_data(struct IWineD3DResourceImpl *resource, REFGUID guid)
{
    struct private_data *data;

    TRACE("resource %p, guid %s.\n", resource, debugstr_guid(guid));

    data = resource_find_private_data(resource, guid);
    if (!data) return WINED3DERR_NOTFOUND;

    if (data->flags & WINED3DSPD_IUNKNOWN)
    {
        if (data->ptr.object)
            IUnknown_Release(data->ptr.object);
    }
    else
    {
        HeapFree(GetProcessHeap(), 0, data->ptr.data);
    }
    list_remove(&data->entry);

    HeapFree(GetProcessHeap(), 0, data);

    return WINED3D_OK;
}

DWORD resource_set_priority(struct IWineD3DResourceImpl *resource, DWORD priority)
{
    DWORD prev = resource->resource.priority;
    resource->resource.priority = priority;
    TRACE("resource %p, new priority %u, returning old priority %u.\n", resource, priority, prev);
    return prev;
}

DWORD resource_get_priority(struct IWineD3DResourceImpl *resource)
{
    TRACE("resource %p, returning %u.\n", resource, resource->resource.priority);
    return resource->resource.priority;
}

WINED3DRESOURCETYPE resource_get_type(struct IWineD3DResourceImpl *resource)
{
    TRACE("resource %p, returning %#x.\n", resource, resource->resource.resourceType);
    return resource->resource.resourceType;
}
