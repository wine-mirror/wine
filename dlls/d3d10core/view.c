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

#define NONAMELESSUNION
#include "d3d10core_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d10core);

static struct wined3d_resource *wined3d_resource_from_resource(ID3D10Resource *resource)
{
    D3D10_RESOURCE_DIMENSION dimension;

    ID3D10Resource_GetType(resource, &dimension);

    switch(dimension)
    {
        case D3D10_RESOURCE_DIMENSION_BUFFER:
            return wined3d_buffer_get_resource(((struct d3d10_buffer *)resource)->wined3d_buffer);

        case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
            return wined3d_texture_get_resource(((struct d3d10_texture2d *)resource)->wined3d_texture);

        default:
            FIXME("Unhandled resource dimension %#x.\n", dimension);
            return NULL;
    }
}

static HRESULT set_rtdesc_from_resource(D3D10_RENDER_TARGET_VIEW_DESC *desc, ID3D10Resource *resource)
{
    D3D10_RESOURCE_DIMENSION dimension;
    HRESULT hr;

    ID3D10Resource_GetType(resource, &dimension);

    switch(dimension)
    {
        case D3D10_RESOURCE_DIMENSION_TEXTURE1D:
        {
            ID3D10Texture1D *texture;
            D3D10_TEXTURE1D_DESC texture_desc;

            hr = ID3D10Resource_QueryInterface(resource, &IID_ID3D10Texture1D, (void **)&texture);
            if (FAILED(hr))
            {
                ERR("Resource of type TEXTURE1D doesn't implement ID3D10Texture1D?\n");
                return E_INVALIDARG;
            }

            ID3D10Texture1D_GetDesc(texture, &texture_desc);
            ID3D10Texture1D_Release(texture);

            desc->Format = texture_desc.Format;
            if (texture_desc.ArraySize == 1)
            {
                desc->ViewDimension = D3D10_RTV_DIMENSION_TEXTURE1D;
                desc->u.Texture1D.MipSlice = 0;
            }
            else
            {
                desc->ViewDimension = D3D10_RTV_DIMENSION_TEXTURE1DARRAY;
                desc->u.Texture1DArray.MipSlice = 0;
                desc->u.Texture1DArray.FirstArraySlice = 0;
                desc->u.Texture1DArray.ArraySize = 1;
            }

            return S_OK;
        }

        case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
        {
            ID3D10Texture2D *texture;
            D3D10_TEXTURE2D_DESC texture_desc;

            hr = ID3D10Resource_QueryInterface(resource, &IID_ID3D10Texture2D, (void **)&texture);
            if (FAILED(hr))
            {
                ERR("Resource of type TEXTURE2D doesn't implement ID3D10Texture2D?\n");
                return E_INVALIDARG;
            }

            ID3D10Texture2D_GetDesc(texture, &texture_desc);
            ID3D10Texture2D_Release(texture);

            desc->Format = texture_desc.Format;
            if (texture_desc.ArraySize == 1)
            {
                if (texture_desc.SampleDesc.Count == 1)
                {
                    desc->ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;
                    desc->u.Texture2D.MipSlice = 0;
                }
                else
                {
                    desc->ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2DMS;
                }
            }
            else
            {
                if (texture_desc.SampleDesc.Count == 1)
                {
                    desc->ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2DARRAY;
                    desc->u.Texture2DArray.MipSlice = 0;
                    desc->u.Texture2DArray.FirstArraySlice = 0;
                    desc->u.Texture2DArray.ArraySize = 1;
                }
                else
                {
                    desc->ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2DMSARRAY;
                    desc->u.Texture2DMSArray.FirstArraySlice = 0;
                    desc->u.Texture2DMSArray.ArraySize = 1;
                }
            }

            return S_OK;
        }

        case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
        {
            ID3D10Texture3D *texture;
            D3D10_TEXTURE3D_DESC texture_desc;

            hr = ID3D10Resource_QueryInterface(resource, &IID_ID3D10Texture3D, (void **)&texture);
            if (FAILED(hr))
            {
                ERR("Resource of type TEXTURE3D doesn't implement ID3D10Texture3D?\n");
                return E_INVALIDARG;
            }

            ID3D10Texture3D_GetDesc(texture, &texture_desc);
            ID3D10Texture3D_Release(texture);

            desc->Format = texture_desc.Format;
            desc->ViewDimension = D3D10_RTV_DIMENSION_TEXTURE3D;
            desc->u.Texture3D.MipSlice = 0;
            desc->u.Texture3D.FirstWSlice = 0;
            desc->u.Texture3D.WSize = 1;

            return S_OK;
        }

        default:
            FIXME("Unhandled resource dimension %#x.\n", dimension);
            return E_INVALIDARG;
    }
}

static inline struct d3d10_depthstencil_view *impl_from_ID3D10DepthStencilView(ID3D10DepthStencilView *iface)
{
    return CONTAINING_RECORD(iface, struct d3d10_depthstencil_view, ID3D10DepthStencilView_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_depthstencil_view_QueryInterface(ID3D10DepthStencilView *iface,
        REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D10DepthStencilView)
            || IsEqualGUID(riid, &IID_ID3D10View)
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

static ULONG STDMETHODCALLTYPE d3d10_depthstencil_view_AddRef(ID3D10DepthStencilView *iface)
{
    struct d3d10_depthstencil_view *This = impl_from_ID3D10DepthStencilView(iface);
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %u.\n", This, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d10_depthstencil_view_Release(ID3D10DepthStencilView *iface)
{
    struct d3d10_depthstencil_view *This = impl_from_ID3D10DepthStencilView(iface);
    ULONG refcount = InterlockedDecrement(&This->refcount);

    TRACE("%p decreasing refcount to %u.\n", This, refcount);

    if (!refcount)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refcount;
}

/* ID3D10DeviceChild methods */

static void STDMETHODCALLTYPE d3d10_depthstencil_view_GetDevice(ID3D10DepthStencilView *iface, ID3D10Device **device)
{
    FIXME("iface %p, device %p stub!\n", iface, device);
}

static HRESULT STDMETHODCALLTYPE d3d10_depthstencil_view_GetPrivateData(ID3D10DepthStencilView *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    FIXME("iface %p, guid %s, data_size %p, data %p stub!\n",
            iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_depthstencil_view_SetPrivateData(ID3D10DepthStencilView *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    FIXME("iface %p, guid %s, data_size %u, data %p stub!\n",
            iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_depthstencil_view_SetPrivateDataInterface(ID3D10DepthStencilView *iface,
        REFGUID guid, const IUnknown *data)
{
    FIXME("iface %p, guid %s, data %p stub!\n", iface, debugstr_guid(guid), data);

    return E_NOTIMPL;
}

/* ID3D10View methods */

static void STDMETHODCALLTYPE d3d10_depthstencil_view_GetResource(ID3D10DepthStencilView *iface,
        ID3D10Resource **resource)
{
    FIXME("iface %p, resource %p stub!\n", iface, resource);
}

/* ID3D10DepthStencilView methods */

static void STDMETHODCALLTYPE d3d10_depthstencil_view_GetDesc(ID3D10DepthStencilView *iface,
        D3D10_DEPTH_STENCIL_VIEW_DESC *desc)
{
    FIXME("iface %p, desc %p stub!\n", iface, desc);
}

static const struct ID3D10DepthStencilViewVtbl d3d10_depthstencil_view_vtbl =
{
    /* IUnknown methods */
    d3d10_depthstencil_view_QueryInterface,
    d3d10_depthstencil_view_AddRef,
    d3d10_depthstencil_view_Release,
    /* ID3D10DeviceChild methods */
    d3d10_depthstencil_view_GetDevice,
    d3d10_depthstencil_view_GetPrivateData,
    d3d10_depthstencil_view_SetPrivateData,
    d3d10_depthstencil_view_SetPrivateDataInterface,
    /* ID3D10View methods */
    d3d10_depthstencil_view_GetResource,
    /* ID3D10DepthStencilView methods */
    d3d10_depthstencil_view_GetDesc,
};

HRESULT d3d10_depthstencil_view_init(struct d3d10_depthstencil_view *view)
{
    view->ID3D10DepthStencilView_iface.lpVtbl = &d3d10_depthstencil_view_vtbl;
    view->refcount = 1;

    return S_OK;
}

static inline struct d3d10_rendertarget_view *impl_from_ID3D10RenderTargetView(ID3D10RenderTargetView *iface)
{
    return CONTAINING_RECORD(iface, struct d3d10_rendertarget_view, ID3D10RenderTargetView_iface);
}

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
    struct d3d10_rendertarget_view *This = impl_from_ID3D10RenderTargetView(iface);
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %u\n", This, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d10_rendertarget_view_Release(ID3D10RenderTargetView *iface)
{
    struct d3d10_rendertarget_view *This = impl_from_ID3D10RenderTargetView(iface);
    ULONG refcount = InterlockedDecrement(&This->refcount);

    TRACE("%p decreasing refcount to %u\n", This, refcount);

    if (!refcount)
    {
        wined3d_rendertarget_view_decref(This->wined3d_view);
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
    struct d3d10_rendertarget_view *This = impl_from_ID3D10RenderTargetView(iface);
    struct wined3d_resource *wined3d_resource;
    IUnknown *parent;
    HRESULT hr;

    TRACE("iface %p, resource %p\n", iface, resource);

    wined3d_resource = wined3d_rendertarget_view_get_resource(This->wined3d_view);
    if (!wined3d_resource)
    {
        ERR("Failed to get wined3d resource.\n");
        *resource = NULL;
        return;
    }

    parent = wined3d_resource_get_parent(wined3d_resource);
    hr = IUnknown_QueryInterface(parent, &IID_ID3D10Resource, (void **)&resource);
    if (FAILED(hr))
    {
        ERR("Resource parent isn't a d3d10 resource, hr %#x\n", hr);
        *resource = NULL;
        return;
    }
}

/* ID3D10RenderTargetView methods */

static void STDMETHODCALLTYPE d3d10_rendertarget_view_GetDesc(ID3D10RenderTargetView *iface,
        D3D10_RENDER_TARGET_VIEW_DESC *desc)
{
    struct d3d10_rendertarget_view *This = impl_from_ID3D10RenderTargetView(iface);

    TRACE("iface %p, desc %p\n", iface, desc);

    *desc = This->desc;
}

static const struct ID3D10RenderTargetViewVtbl d3d10_rendertarget_view_vtbl =
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

HRESULT d3d10_rendertarget_view_init(struct d3d10_rendertarget_view *view,
        ID3D10Resource *resource, const D3D10_RENDER_TARGET_VIEW_DESC *desc)
{
    struct wined3d_resource *wined3d_resource;
    HRESULT hr;

    view->ID3D10RenderTargetView_iface.lpVtbl = &d3d10_rendertarget_view_vtbl;
    view->refcount = 1;

    if (!desc)
    {
        HRESULT hr = set_rtdesc_from_resource(&view->desc, resource);
        if (FAILED(hr)) return hr;
    }
    else
    {
        view->desc = *desc;
    }

    wined3d_resource = wined3d_resource_from_resource(resource);
    if (!wined3d_resource)
    {
        ERR("Failed to get wined3d resource for d3d10 resource %p.\n", resource);
        return E_FAIL;
    }

    hr = wined3d_rendertarget_view_create(wined3d_resource, view, &view->wined3d_view);
    if (FAILED(hr))
    {
        WARN("Failed to create a wined3d rendertarget view, hr %#x.\n", hr);
        return hr;
    }

    return S_OK;
}

struct d3d10_rendertarget_view *unsafe_impl_from_ID3D10RenderTargetView(ID3D10RenderTargetView *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d10_rendertarget_view_vtbl);

    return impl_from_ID3D10RenderTargetView(iface);
}

static inline struct d3d10_shader_resource_view *impl_from_ID3D10ShaderResourceView(ID3D10ShaderResourceView *iface)
{
    return CONTAINING_RECORD(iface, struct d3d10_shader_resource_view, ID3D10ShaderResourceView_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_shader_resource_view_QueryInterface(ID3D10ShaderResourceView *iface,
        REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D10ShaderResourceView)
            || IsEqualGUID(riid, &IID_ID3D10View)
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

static ULONG STDMETHODCALLTYPE d3d10_shader_resource_view_AddRef(ID3D10ShaderResourceView *iface)
{
    struct d3d10_shader_resource_view *This = impl_from_ID3D10ShaderResourceView(iface);
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %u.\n", This, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d10_shader_resource_view_Release(ID3D10ShaderResourceView *iface)
{
    struct d3d10_shader_resource_view *This = impl_from_ID3D10ShaderResourceView(iface);
    ULONG refcount = InterlockedDecrement(&This->refcount);

    TRACE("%p decreasing refcount to %u.\n", This, refcount);

    if (!refcount)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refcount;
}

/* ID3D10DeviceChild methods */

static void STDMETHODCALLTYPE d3d10_shader_resource_view_GetDevice(ID3D10ShaderResourceView *iface,
        ID3D10Device **device)
{
    FIXME("iface %p, device %p stub!\n", iface, device);
}

static HRESULT STDMETHODCALLTYPE d3d10_shader_resource_view_GetPrivateData(ID3D10ShaderResourceView *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    FIXME("iface %p, guid %s, data_size %p, data %p stub!\n",
            iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_shader_resource_view_SetPrivateData(ID3D10ShaderResourceView *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    FIXME("iface %p, guid %s, data_size %u, data %p stub!\n",
            iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_shader_resource_view_SetPrivateDataInterface(ID3D10ShaderResourceView *iface,
        REFGUID guid, const IUnknown *data)
{
    FIXME("iface %p, guid %s, data %p stub!\n", iface, debugstr_guid(guid), data);

    return E_NOTIMPL;
}

/* ID3D10View methods */

static void STDMETHODCALLTYPE d3d10_shader_resource_view_GetResource(ID3D10ShaderResourceView *iface,
        ID3D10Resource **resource)
{
    FIXME("iface %p, resource %p stub!\n", iface, resource);
}

/* ID3D10ShaderResourceView methods */

static void STDMETHODCALLTYPE d3d10_shader_resource_view_GetDesc(ID3D10ShaderResourceView *iface,
        D3D10_SHADER_RESOURCE_VIEW_DESC *desc)
{
    FIXME("iface %p, desc %p stub!\n", iface, desc);
}

static const struct ID3D10ShaderResourceViewVtbl d3d10_shader_resource_view_vtbl =
{
    /* IUnknown methods */
    d3d10_shader_resource_view_QueryInterface,
    d3d10_shader_resource_view_AddRef,
    d3d10_shader_resource_view_Release,
    /* ID3D10DeviceChild methods */
    d3d10_shader_resource_view_GetDevice,
    d3d10_shader_resource_view_GetPrivateData,
    d3d10_shader_resource_view_SetPrivateData,
    d3d10_shader_resource_view_SetPrivateDataInterface,
    /* ID3D10View methods */
    d3d10_shader_resource_view_GetResource,
    /* ID3D10ShaderResourceView methods */
    d3d10_shader_resource_view_GetDesc,
};

HRESULT d3d10_shader_resource_view_init(struct d3d10_shader_resource_view *view)
{
    view->ID3D10ShaderResourceView_iface.lpVtbl = &d3d10_shader_resource_view_vtbl;
    view->refcount = 1;

    return S_OK;
}
