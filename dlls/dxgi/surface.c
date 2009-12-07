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

#include "dxgi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxgi);

/* Inner IUnknown methods */

static inline struct dxgi_surface *dxgi_surface_from_inner_unknown(IUnknown *iface)
{
    return (struct dxgi_surface *)((char*)iface - FIELD_OFFSET(struct dxgi_surface, inner_unknown_vtbl));
}

static HRESULT STDMETHODCALLTYPE dxgi_surface_inner_QueryInterface(IUnknown *iface, REFIID riid, void **object)
{
    struct dxgi_surface *This = dxgi_surface_from_inner_unknown(iface);

    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IDXGISurface)
            || IsEqualGUID(riid, &IID_IDXGIDeviceSubObject)
            || IsEqualGUID(riid, &IID_IDXGIObject)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef((IUnknown *)This);
        *object = This;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE dxgi_surface_inner_AddRef(IUnknown *iface)
{
    struct dxgi_surface *This = dxgi_surface_from_inner_unknown(iface);
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %u\n", This, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE dxgi_surface_inner_Release(IUnknown *iface)
{
    struct dxgi_surface *This = dxgi_surface_from_inner_unknown(iface);
    ULONG refcount = InterlockedDecrement(&This->refcount);

    TRACE("%p decreasing refcount to %u\n", This, refcount);

    if (!refcount)
    {
        IDXGIDevice_Release(This->device);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refcount;
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE dxgi_surface_QueryInterface(IDXGISurface *iface, REFIID riid, void **object)
{
    struct dxgi_surface *This = (struct dxgi_surface *)iface;
    TRACE("Forwarding to outer IUnknown\n");
    return IUnknown_QueryInterface(This->outer_unknown, riid, object);
}

static ULONG STDMETHODCALLTYPE dxgi_surface_AddRef(IDXGISurface *iface)
{
    struct dxgi_surface *This = (struct dxgi_surface *)iface;
    TRACE("Forwarding to outer IUnknown\n");
    return IUnknown_AddRef(This->outer_unknown);
}

static ULONG STDMETHODCALLTYPE dxgi_surface_Release(IDXGISurface *iface)
{
    struct dxgi_surface *This = (struct dxgi_surface *)iface;
    TRACE("Forwarding to outer IUnknown\n");
    return IUnknown_Release(This->outer_unknown);
}

/* IDXGIObject methods */

static HRESULT STDMETHODCALLTYPE dxgi_surface_SetPrivateData(IDXGISurface *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    FIXME("iface %p, guid %s, data_size %u, data %p stub!\n", iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_surface_SetPrivateDataInterface(IDXGISurface *iface,
        REFGUID guid, const IUnknown *object)
{
    FIXME("iface %p, guid %s, object %p stub!\n", iface, debugstr_guid(guid), object);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_surface_GetPrivateData(IDXGISurface *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    FIXME("iface %p, guid %s, data_size %p, data %p stub!\n", iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_surface_GetParent(IDXGISurface *iface, REFIID riid, void **parent)
{
    struct dxgi_surface *This = (struct dxgi_surface *)iface;

    TRACE("iface %p, riid %s, parent %p.\n", iface, debugstr_guid(riid), parent);

    return IDXGIDevice_QueryInterface(This->device, riid, parent);
}

/* IDXGIDeviceSubObject methods */

static HRESULT STDMETHODCALLTYPE dxgi_surface_GetDevice(IDXGISurface *iface, REFIID riid, void **device)
{
    struct dxgi_surface *This = (struct dxgi_surface *)iface;

    TRACE("iface %p, riid %s, device %p.\n", iface, debugstr_guid(riid), device);

    return IDXGIDevice_QueryInterface(This->device, riid, device);
}

/* IDXGISurface methods */
static HRESULT STDMETHODCALLTYPE dxgi_surface_GetDesc(IDXGISurface *iface, DXGI_SURFACE_DESC *desc)
{
    FIXME("iface %p, desc %p stub!\n", iface, desc);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_surface_Map(IDXGISurface *iface, DXGI_MAPPED_RECT *mapped_rect, UINT flags)
{
    FIXME("iface %p, mapped_rect %p, flags %#x stub!\n", iface, mapped_rect, flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_surface_Unmap(IDXGISurface *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static const struct IDXGISurfaceVtbl dxgi_surface_vtbl =
{
    /* IUnknown methods */
    dxgi_surface_QueryInterface,
    dxgi_surface_AddRef,
    dxgi_surface_Release,
    /* IDXGIObject methods */
    dxgi_surface_SetPrivateData,
    dxgi_surface_SetPrivateDataInterface,
    dxgi_surface_GetPrivateData,
    dxgi_surface_GetParent,
    /* IDXGIDeviceSubObject methods */
    dxgi_surface_GetDevice,
    /* IDXGISurface methods */
    dxgi_surface_GetDesc,
    dxgi_surface_Map,
    dxgi_surface_Unmap,
};

static const struct IUnknownVtbl dxgi_surface_inner_unknown_vtbl =
{
    /* IUnknown methods */
    dxgi_surface_inner_QueryInterface,
    dxgi_surface_inner_AddRef,
    dxgi_surface_inner_Release,
};

HRESULT dxgi_surface_init(struct dxgi_surface *surface, IDXGIDevice *device, IUnknown *outer)
{
    surface->vtbl = &dxgi_surface_vtbl;
    surface->inner_unknown_vtbl = &dxgi_surface_inner_unknown_vtbl;
    surface->refcount = 1;
    surface->outer_unknown = outer ? outer : (IUnknown *)&surface->inner_unknown_vtbl;
    surface->device = device;
    IDXGIDevice_AddRef(device);

    return S_OK;
}
