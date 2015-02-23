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

#include "d3d10core_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d10core);

static inline struct d3d10_query *impl_from_ID3D10Query(ID3D10Query *iface)
{
    return CONTAINING_RECORD(iface, struct d3d10_query, ID3D10Query_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_query_QueryInterface(ID3D10Query *iface, REFIID riid, void **object)
{
    struct d3d10_query *query = impl_from_ID3D10Query(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if ((IsEqualGUID(riid, &IID_ID3D10Predicate) && query->predicate)
            || IsEqualGUID(riid, &IID_ID3D10Query)
            || IsEqualGUID(riid, &IID_ID3D10Asynchronous)
            || IsEqualGUID(riid, &IID_ID3D10DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d10_query_AddRef(ID3D10Query *iface)
{
    struct d3d10_query *This = impl_from_ID3D10Query(iface);
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %u.\n", This, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d10_query_Release(ID3D10Query *iface)
{
    struct d3d10_query *This = impl_from_ID3D10Query(iface);
    ULONG refcount = InterlockedDecrement(&This->refcount);

    TRACE("%p decreasing refcount to %u.\n", This, refcount);

    if (!refcount)
    {
        ID3D10Device1_Release(This->device);
        wined3d_private_store_cleanup(&This->private_store);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refcount;
}

/* ID3D10DeviceChild methods */

static void STDMETHODCALLTYPE d3d10_query_GetDevice(ID3D10Query *iface, ID3D10Device **device)
{
    struct d3d10_query *query = impl_from_ID3D10Query(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (ID3D10Device *)query->device;
    ID3D10Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d10_query_GetPrivateData(ID3D10Query *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d10_query *query = impl_from_ID3D10Query(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d10_get_private_data(&query->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_query_SetPrivateData(ID3D10Query *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d10_query *query = impl_from_ID3D10Query(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d10_set_private_data(&query->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_query_SetPrivateDataInterface(ID3D10Query *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d10_query *query = impl_from_ID3D10Query(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d10_set_private_data_interface(&query->private_store, guid, data);
}

/* ID3D10Asynchronous methods */

static void STDMETHODCALLTYPE d3d10_query_Begin(ID3D10Query *iface)
{
    FIXME("iface %p stub!\n", iface);
}

static void STDMETHODCALLTYPE d3d10_query_End(ID3D10Query *iface)
{
    FIXME("iface %p stub!\n", iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_query_GetData(ID3D10Query *iface, void *data, UINT data_size, UINT flags)
{
    FIXME("iface %p, data %p, data_size %u, flags %#x stub!\n", iface, data, data_size, flags);

    return E_NOTIMPL;
}

static UINT STDMETHODCALLTYPE d3d10_query_GetDataSize(ID3D10Query *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

/* ID3D10Query methods */

static void STDMETHODCALLTYPE d3d10_query_GetDesc(ID3D10Query *iface, D3D10_QUERY_DESC *desc)
{
    FIXME("iface %p, desc %p stub!\n", iface, desc);
}

static const struct ID3D10QueryVtbl d3d10_query_vtbl =
{
    /* IUnknown methods */
    d3d10_query_QueryInterface,
    d3d10_query_AddRef,
    d3d10_query_Release,
    /* ID3D10DeviceChild methods */
    d3d10_query_GetDevice,
    d3d10_query_GetPrivateData,
    d3d10_query_SetPrivateData,
    d3d10_query_SetPrivateDataInterface,
    /* ID3D10Asynchronous methods */
    d3d10_query_Begin,
    d3d10_query_End,
    d3d10_query_GetData,
    d3d10_query_GetDataSize,
    /* ID3D10Query methods */
    d3d10_query_GetDesc,
};

struct d3d10_query *unsafe_impl_from_ID3D10Query(ID3D10Query *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d10_query_vtbl);
    return CONTAINING_RECORD(iface, struct d3d10_query, ID3D10Query_iface);
}

HRESULT d3d10_query_init(struct d3d10_query *query, struct d3d10_device *device,
        const D3D10_QUERY_DESC *desc, BOOL predicate)
{
    HRESULT hr;

    static const enum wined3d_query_type query_type_map[] =
    {
        /* D3D10_QUERY_EVENT                    */  WINED3D_QUERY_TYPE_EVENT,
        /* D3D10_QUERY_OCCLUSION                */  WINED3D_QUERY_TYPE_OCCLUSION,
        /* D3D10_QUERY_TIMESTAMP                */  WINED3D_QUERY_TYPE_TIMESTAMP,
        /* D3D10_QUERY_TIMESTAMP_DISJOINT       */  WINED3D_QUERY_TYPE_TIMESTAMP_DISJOINT,
        /* D3D10_QUERY_PIPELINE_STATISTICS      */  WINED3D_QUERY_TYPE_PIPELINE_STATISTICS,
        /* D3D10_QUERY_OCCLUSION_PREDICATE      */  WINED3D_QUERY_TYPE_OCCLUSION,
        /* D3D10_QUERY_SO_STATISTICS            */  WINED3D_QUERY_TYPE_SO_STATISTICS,
        /* D3D10_QUERY_SO_OVERFLOW_PREDICATE    */  WINED3D_QUERY_TYPE_SO_OVERFLOW,
    };

    if (desc->Query >= sizeof(query_type_map) / sizeof(*query_type_map))
    {
        FIXME("Unhandled query type %#x.\n", desc->Query);
        return E_INVALIDARG;
    }

    if (desc->MiscFlags)
        FIXME("Ignoring MiscFlags %#x.\n", desc->MiscFlags);

    query->ID3D10Query_iface.lpVtbl = &d3d10_query_vtbl;
    query->refcount = 1;
    wined3d_private_store_init(&query->private_store);

    if (FAILED(hr = wined3d_query_create(device->wined3d_device,
            query_type_map[desc->Query], query, &query->wined3d_query)))
    {
        WARN("Failed to create wined3d query, hr %#x.\n", hr);
        wined3d_private_store_cleanup(&query->private_store);
        return hr;
    }

    query->predicate = predicate;
    query->device = &device->ID3D10Device1_iface;
    ID3D10Device1_AddRef(query->device);

    return S_OK;
}
