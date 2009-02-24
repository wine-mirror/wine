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

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_rendertarget_view_QueryInterface(ID3D10RenderTargetView *iface,
        REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D10RenderTargetView)
            || IsEqualGUID(riid, &IID_ID3D10View)
            || IsEqualGUID(riid, &IID_ID3D10DeviceChild)
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

static ULONG STDMETHODCALLTYPE d3d10_rendertarget_view_AddRef(ID3D10RenderTargetView *iface)
{
    struct d3d10_rendertarget_view *This = (struct d3d10_rendertarget_view *)iface;
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %u\n", This, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d10_rendertarget_view_Release(ID3D10RenderTargetView *iface)
{
    struct d3d10_rendertarget_view *This = (struct d3d10_rendertarget_view *)iface;
    ULONG refcount = InterlockedDecrement(&This->refcount);

    TRACE("%p decreasing refcount to %u\n", This, refcount);

    if (!refcount)
    {
        IWineD3DRendertargetView_Release(This->wined3d_view);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refcount;
}

/* ID3D10DeviceChild methods */

static void STDMETHODCALLTYPE d3d10_rendertarget_view_GetDevice(ID3D10RenderTargetView *iface, ID3D10Device **device)
{
    FIXME("iface %p, device %p stub!\n", iface, device);
}

static HRESULT STDMETHODCALLTYPE d3d10_rendertarget_view_GetPrivateData(ID3D10RenderTargetView *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    FIXME("iface %p, guid %s, data_size %p, data %p stub!\n",
            iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_rendertarget_view_SetPrivateData(ID3D10RenderTargetView *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    FIXME("iface %p, guid %s, data_size %u, data %p stub!\n",
            iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_rendertarget_view_SetPrivateDataInterface(ID3D10RenderTargetView *iface,
        REFGUID guid, const IUnknown *data)
{
    FIXME("iface %p, guid %s, data %p stub!\n", iface, debugstr_guid(guid), data);

    return E_NOTIMPL;
}

/* ID3D10View methods */

static void STDMETHODCALLTYPE d3d10_rendertarget_view_GetResource(ID3D10RenderTargetView *iface,
        ID3D10Resource **resource)
{
    struct d3d10_rendertarget_view *This = (struct d3d10_rendertarget_view *)iface;
    IWineD3DResource *wined3d_resource;
    IUnknown *parent;
    HRESULT hr;

    TRACE("iface %p, resource %p\n", iface, resource);

    hr = IWineD3DRendertargetView_GetResource(This->wined3d_view, &wined3d_resource);
    if (FAILED(hr))
    {
        ERR("Failed to get wined3d resource, hr %#x\n", hr);
        *resource = NULL;
        return;
    }

    hr = IWineD3DResource_GetParent(wined3d_resource, &parent);
    IWineD3DResource_Release(wined3d_resource);
    if (FAILED(hr))
    {
        ERR("Failed to get wined3d resource parent, hr %#x\n", hr);
        *resource = NULL;
        return;
    }

    hr = IUnknown_QueryInterface(parent, &IID_ID3D10Resource, (void **)&resource);
    IUnknown_Release(parent);
    if (FAILED(hr))
    {
        ERR("Resource parent isn't a d3d10 resource, hr %#x\n", hr);
        *resource = NULL;
        return;
    }
}

/* ID3D10RenderTargetView methods */

static void STDMETHODCALLTYPE d3d10_rendertarget_view_GetDesc(ID3D10RenderTargetView* iface,
        D3D10_RENDER_TARGET_VIEW_DESC *desc)
{
    struct d3d10_rendertarget_view *This = (struct d3d10_rendertarget_view *)iface;

    TRACE("iface %p, desc %p\n", iface, desc);

    *desc = This->desc;
}

const struct ID3D10RenderTargetViewVtbl d3d10_rendertarget_view_vtbl =
{
    /* IUnknown methods */
    d3d10_rendertarget_view_QueryInterface,
    d3d10_rendertarget_view_AddRef,
    d3d10_rendertarget_view_Release,
    /* ID3D10DeviceChild methods */
    d3d10_rendertarget_view_GetDevice,
    d3d10_rendertarget_view_GetPrivateData,
    d3d10_rendertarget_view_SetPrivateData,
    d3d10_rendertarget_view_SetPrivateDataInterface,
    /* ID3D10View methods */
    d3d10_rendertarget_view_GetResource,
    /* ID3D10RenderTargetView methods */
    d3d10_rendertarget_view_GetDesc,
};
