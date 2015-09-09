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
#include "d3d11_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d11);

static HRESULT set_dsdesc_from_resource(D3D11_DEPTH_STENCIL_VIEW_DESC *desc, ID3D11Resource *resource)
{
    D3D11_RESOURCE_DIMENSION dimension;

    ID3D11Resource_GetType(resource, &dimension);

    desc->Flags = 0;

    switch (dimension)
    {
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
        {
            D3D11_TEXTURE1D_DESC texture_desc;
            ID3D11Texture1D *texture;

            if (FAILED(ID3D11Resource_QueryInterface(resource, &IID_ID3D11Texture1D, (void **)&texture)))
            {
                ERR("Resource of type TEXTURE1D doesn't implement ID3D11Texture1D.\n");
                return E_INVALIDARG;
            }

            ID3D11Texture1D_GetDesc(texture, &texture_desc);
            ID3D11Texture1D_Release(texture);

            desc->Format = texture_desc.Format;
            if (texture_desc.ArraySize == 1)
            {
                desc->ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1D;
                desc->u.Texture1D.MipSlice = 0;
            }
            else
            {
                desc->ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1DARRAY;
                desc->u.Texture1DArray.MipSlice = 0;
                desc->u.Texture1DArray.FirstArraySlice = 0;
                desc->u.Texture1DArray.ArraySize = 1;
            }

            return S_OK;
        }

        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
        {
            D3D11_TEXTURE2D_DESC texture_desc;
            ID3D11Texture2D *texture;

            if (FAILED(ID3D11Resource_QueryInterface(resource, &IID_ID3D11Texture2D, (void **)&texture)))
            {
                ERR("Resource of type TEXTURE2D doesn't implement ID3D11Texture2D.\n");
                return E_INVALIDARG;
            }

            ID3D11Texture2D_GetDesc(texture, &texture_desc);
            ID3D11Texture2D_Release(texture);

            desc->Format = texture_desc.Format;
            if (texture_desc.ArraySize == 1)
            {
                if (texture_desc.SampleDesc.Count == 1)
                {
                    desc->ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                    desc->u.Texture2D.MipSlice = 0;
                }
                else
                {
                    desc->ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
                }
            }
            else
            {
                if (texture_desc.SampleDesc.Count == 1)
                {
                    desc->ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                    desc->u.Texture2DArray.MipSlice = 0;
                    desc->u.Texture2DArray.FirstArraySlice = 0;
                    desc->u.Texture2DArray.ArraySize = 1;
                }
                else
                {
                    desc->ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
                    desc->u.Texture2DMSArray.FirstArraySlice = 0;
                    desc->u.Texture2DMSArray.ArraySize = 1;
                }
            }

            return S_OK;
        }

        default:
            FIXME("Unhandled resource dimension %#x.\n", dimension);
        case D3D11_RESOURCE_DIMENSION_BUFFER:
        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
            return E_INVALIDARG;
    }
}

static HRESULT set_rtdesc_from_resource(D3D11_RENDER_TARGET_VIEW_DESC *desc, ID3D11Resource *resource)
{
    D3D11_RESOURCE_DIMENSION dimension;
    HRESULT hr;

    ID3D11Resource_GetType(resource, &dimension);

    switch(dimension)
    {
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
        {
            ID3D11Texture1D *texture;
            D3D11_TEXTURE1D_DESC texture_desc;

            hr = ID3D11Resource_QueryInterface(resource, &IID_ID3D11Texture1D, (void **)&texture);
            if (FAILED(hr))
            {
                ERR("Resource of type TEXTURE1D doesn't implement ID3D11Texture1D?\n");
                return E_INVALIDARG;
            }

            ID3D11Texture1D_GetDesc(texture, &texture_desc);
            ID3D11Texture1D_Release(texture);

            desc->Format = texture_desc.Format;
            if (texture_desc.ArraySize == 1)
            {
                desc->ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1D;
                desc->u.Texture1D.MipSlice = 0;
            }
            else
            {
                desc->ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1DARRAY;
                desc->u.Texture1DArray.MipSlice = 0;
                desc->u.Texture1DArray.FirstArraySlice = 0;
                desc->u.Texture1DArray.ArraySize = 1;
            }

            return S_OK;
        }

        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
        {
            ID3D11Texture2D *texture;
            D3D11_TEXTURE2D_DESC texture_desc;

            hr = ID3D11Resource_QueryInterface(resource, &IID_ID3D11Texture2D, (void **)&texture);
            if (FAILED(hr))
            {
                ERR("Resource of type TEXTURE2D doesn't implement ID3D11Texture2D?\n");
                return E_INVALIDARG;
            }

            ID3D11Texture2D_GetDesc(texture, &texture_desc);
            ID3D11Texture2D_Release(texture);

            desc->Format = texture_desc.Format;
            if (texture_desc.ArraySize == 1)
            {
                if (texture_desc.SampleDesc.Count == 1)
                {
                    desc->ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                    desc->u.Texture2D.MipSlice = 0;
                }
                else
                {
                    desc->ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
                }
            }
            else
            {
                if (texture_desc.SampleDesc.Count == 1)
                {
                    desc->ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                    desc->u.Texture2DArray.MipSlice = 0;
                    desc->u.Texture2DArray.FirstArraySlice = 0;
                    desc->u.Texture2DArray.ArraySize = 1;
                }
                else
                {
                    desc->ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
                    desc->u.Texture2DMSArray.FirstArraySlice = 0;
                    desc->u.Texture2DMSArray.ArraySize = 1;
                }
            }

            return S_OK;
        }

        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
        {
            ID3D11Texture3D *texture;
            D3D11_TEXTURE3D_DESC texture_desc;

            hr = ID3D11Resource_QueryInterface(resource, &IID_ID3D11Texture3D, (void **)&texture);
            if (FAILED(hr))
            {
                ERR("Resource of type TEXTURE3D doesn't implement ID3D11Texture3D?\n");
                return E_INVALIDARG;
            }

            ID3D11Texture3D_GetDesc(texture, &texture_desc);
            ID3D11Texture3D_Release(texture);

            desc->Format = texture_desc.Format;
            desc->ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
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

static HRESULT set_srdesc_from_resource(D3D11_SHADER_RESOURCE_VIEW_DESC *desc, ID3D11Resource *resource)
{
    D3D11_RESOURCE_DIMENSION dimension;

    ID3D11Resource_GetType(resource, &dimension);

    switch (dimension)
    {
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
        {
            D3D11_TEXTURE1D_DESC texture_desc;
            ID3D11Texture1D *texture;

            if (FAILED(ID3D11Resource_QueryInterface(resource, &IID_ID3D11Texture1D, (void **)&texture)))
            {
                ERR("Resource of type TEXTURE1D doesn't implement ID3D11Texture1D.\n");
                return E_INVALIDARG;
            }

            ID3D11Texture1D_GetDesc(texture, &texture_desc);
            ID3D11Texture1D_Release(texture);

            desc->Format = texture_desc.Format;
            if (texture_desc.ArraySize == 1)
            {
                desc->ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
                desc->u.Texture1D.MostDetailedMip = 0;
                desc->u.Texture1D.MipLevels = texture_desc.MipLevels;
            }
            else
            {
                desc->ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
                desc->u.Texture1DArray.MostDetailedMip = 0;
                desc->u.Texture1DArray.MipLevels = texture_desc.MipLevels;
                desc->u.Texture1DArray.FirstArraySlice = 0;
                desc->u.Texture1DArray.ArraySize = texture_desc.ArraySize;
            }

            return S_OK;
        }

        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
        {
            D3D11_TEXTURE2D_DESC texture_desc;
            ID3D11Texture2D *texture;

            if (FAILED(ID3D11Resource_QueryInterface(resource, &IID_ID3D11Texture2D, (void **)&texture)))
            {
                ERR("Resource of type TEXTURE2D doesn't implement ID3D11Texture2D.\n");
                return E_INVALIDARG;
            }

            ID3D11Texture2D_GetDesc(texture, &texture_desc);
            ID3D11Texture2D_Release(texture);

            desc->Format = texture_desc.Format;
            if (texture_desc.ArraySize == 1)
            {
                if (texture_desc.SampleDesc.Count == 1)
                {
                    desc->ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    desc->u.Texture2D.MostDetailedMip = 0;
                    desc->u.Texture2D.MipLevels = texture_desc.MipLevels;
                }
                else
                {
                    desc->ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
                }
            }
            else
            {
                if (texture_desc.SampleDesc.Count == 1)
                {
                    desc->ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                    desc->u.Texture2DArray.MostDetailedMip = 0;
                    desc->u.Texture2DArray.MipLevels = texture_desc.MipLevels;
                    desc->u.Texture2DArray.FirstArraySlice = 0;
                    desc->u.Texture2DArray.ArraySize = texture_desc.ArraySize;
                }
                else
                {
                    desc->ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
                    desc->u.Texture2DMSArray.FirstArraySlice = 0;
                    desc->u.Texture2DMSArray.ArraySize = texture_desc.ArraySize;
                }
            }

            return S_OK;
        }

        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
        {
            D3D11_TEXTURE3D_DESC texture_desc;
            ID3D11Texture3D *texture;

            if (FAILED(ID3D11Resource_QueryInterface(resource, &IID_ID3D11Texture3D, (void **)&texture)))
            {
                ERR("Resource of type TEXTURE3D doesn't implement ID3D11Texture3D.\n");
                return E_INVALIDARG;
            }

            ID3D11Texture3D_GetDesc(texture, &texture_desc);
            ID3D11Texture3D_Release(texture);

            desc->Format = texture_desc.Format;
            desc->ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
            desc->u.Texture3D.MostDetailedMip = 0;
            desc->u.Texture3D.MipLevels = texture_desc.MipLevels;

            return S_OK;
        }

        default:
            FIXME("Unhandled resource dimension %#x.\n", dimension);
        case D3D11_RESOURCE_DIMENSION_BUFFER:
            return E_INVALIDARG;
    }
}

/* ID3D11DepthStencilView methods */

static inline struct d3d_depthstencil_view *impl_from_ID3D11DepthStencilView(ID3D11DepthStencilView *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_depthstencil_view, ID3D11DepthStencilView_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_depthstencil_view_QueryInterface(ID3D11DepthStencilView *iface,
        REFIID riid, void **object)
{
    struct d3d_depthstencil_view *view = impl_from_ID3D11DepthStencilView(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D11DepthStencilView)
            || IsEqualGUID(riid, &IID_ID3D11View)
            || IsEqualGUID(riid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D11DepthStencilView_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_ID3D10DepthStencilView)
            || IsEqualGUID(riid, &IID_ID3D10View)
            || IsEqualGUID(riid, &IID_ID3D10DeviceChild))
    {
        ID3D10DepthStencilView_AddRef(&view->ID3D10DepthStencilView_iface);
        *object = &view->ID3D10DepthStencilView_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d11_depthstencil_view_AddRef(ID3D11DepthStencilView *iface)
{
    struct d3d_depthstencil_view *view = impl_from_ID3D11DepthStencilView(iface);
    ULONG refcount = InterlockedIncrement(&view->refcount);

    TRACE("%p increasing refcount to %u.\n", view, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_depthstencil_view_Release(ID3D11DepthStencilView *iface)
{
    struct d3d_depthstencil_view *view = impl_from_ID3D11DepthStencilView(iface);
    ULONG refcount = InterlockedDecrement(&view->refcount);

    TRACE("%p decreasing refcount to %u.\n", view, refcount);

    if (!refcount)
    {
        wined3d_mutex_lock();
        wined3d_rendertarget_view_decref(view->wined3d_view);
        ID3D11Resource_Release(view->resource);
        ID3D11Device_Release(view->device);
        wined3d_private_store_cleanup(&view->private_store);
        wined3d_mutex_unlock();
        HeapFree(GetProcessHeap(), 0, view);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d11_depthstencil_view_GetDevice(ID3D11DepthStencilView *iface,
        ID3D11Device **device)
{
    struct d3d_depthstencil_view *view = impl_from_ID3D11DepthStencilView(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = view->device;
    ID3D11Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d11_depthstencil_view_GetPrivateData(ID3D11DepthStencilView *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_depthstencil_view *view = impl_from_ID3D11DepthStencilView(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&view->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_depthstencil_view_SetPrivateData(ID3D11DepthStencilView *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_depthstencil_view *view = impl_from_ID3D11DepthStencilView(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&view->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_depthstencil_view_SetPrivateDataInterface(ID3D11DepthStencilView *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_depthstencil_view *view = impl_from_ID3D11DepthStencilView(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&view->private_store, guid, data);
}

static void STDMETHODCALLTYPE d3d11_depthstencil_view_GetResource(ID3D11DepthStencilView *iface,
        ID3D11Resource **resource)
{
    struct d3d_depthstencil_view *view = impl_from_ID3D11DepthStencilView(iface);

    TRACE("iface %p, resource %p.\n", iface, resource);

    *resource = view->resource;
    ID3D11Resource_AddRef(*resource);
}

static void STDMETHODCALLTYPE d3d11_depthstencil_view_GetDesc(ID3D11DepthStencilView *iface,
        D3D11_DEPTH_STENCIL_VIEW_DESC *desc)
{
    struct d3d_depthstencil_view *view = impl_from_ID3D11DepthStencilView(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    *desc = view->desc;
}

static const struct ID3D11DepthStencilViewVtbl d3d11_depthstencil_view_vtbl =
{
    /* IUnknown methods */
    d3d11_depthstencil_view_QueryInterface,
    d3d11_depthstencil_view_AddRef,
    d3d11_depthstencil_view_Release,
    /* ID3D11DeviceChild methods */
    d3d11_depthstencil_view_GetDevice,
    d3d11_depthstencil_view_GetPrivateData,
    d3d11_depthstencil_view_SetPrivateData,
    d3d11_depthstencil_view_SetPrivateDataInterface,
    /* ID3D11View methods */
    d3d11_depthstencil_view_GetResource,
    /* ID3D11DepthStencilView methods */
    d3d11_depthstencil_view_GetDesc,
};

/* ID3D10DepthStencilView methods */

static inline struct d3d_depthstencil_view *impl_from_ID3D10DepthStencilView(ID3D10DepthStencilView *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_depthstencil_view, ID3D10DepthStencilView_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_depthstencil_view_QueryInterface(ID3D10DepthStencilView *iface,
        REFIID riid, void **object)
{
    struct d3d_depthstencil_view *view = impl_from_ID3D10DepthStencilView(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return d3d11_depthstencil_view_QueryInterface(&view->ID3D11DepthStencilView_iface, riid, object);
}

static ULONG STDMETHODCALLTYPE d3d10_depthstencil_view_AddRef(ID3D10DepthStencilView *iface)
{
    struct d3d_depthstencil_view *view = impl_from_ID3D10DepthStencilView(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_depthstencil_view_AddRef(&view->ID3D11DepthStencilView_iface);
}

static ULONG STDMETHODCALLTYPE d3d10_depthstencil_view_Release(ID3D10DepthStencilView *iface)
{
    struct d3d_depthstencil_view *view = impl_from_ID3D10DepthStencilView(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_depthstencil_view_Release(&view->ID3D11DepthStencilView_iface);
}

/* ID3D10DeviceChild methods */

static void STDMETHODCALLTYPE d3d10_depthstencil_view_GetDevice(ID3D10DepthStencilView *iface, ID3D10Device **device)
{
    struct d3d_depthstencil_view *view = impl_from_ID3D10DepthStencilView(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    ID3D11Device_QueryInterface(view->device, &IID_ID3D10Device, (void **)device);
}

static HRESULT STDMETHODCALLTYPE d3d10_depthstencil_view_GetPrivateData(ID3D10DepthStencilView *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_depthstencil_view *view = impl_from_ID3D10DepthStencilView(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&view->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_depthstencil_view_SetPrivateData(ID3D10DepthStencilView *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_depthstencil_view *view = impl_from_ID3D10DepthStencilView(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&view->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_depthstencil_view_SetPrivateDataInterface(ID3D10DepthStencilView *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_depthstencil_view *view = impl_from_ID3D10DepthStencilView(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&view->private_store, guid, data);
}

/* ID3D10View methods */

static void STDMETHODCALLTYPE d3d10_depthstencil_view_GetResource(ID3D10DepthStencilView *iface,
        ID3D10Resource **resource)
{
    struct d3d_depthstencil_view *view = impl_from_ID3D10DepthStencilView(iface);

    TRACE("iface %p, resource %p.\n", iface, resource);

    ID3D11Resource_QueryInterface(view->resource, &IID_ID3D10Resource, (void **)resource);
}

/* ID3D10DepthStencilView methods */

static void STDMETHODCALLTYPE d3d10_depthstencil_view_GetDesc(ID3D10DepthStencilView *iface,
        D3D10_DEPTH_STENCIL_VIEW_DESC *desc)
{
    struct d3d_depthstencil_view *view = impl_from_ID3D10DepthStencilView(iface);
    const D3D11_DEPTH_STENCIL_VIEW_DESC *d3d11_desc = &view->desc;

    TRACE("iface %p, desc %p.\n", iface, desc);

    desc->Format = d3d11_desc->Format;
    desc->ViewDimension = (D3D10_DSV_DIMENSION)d3d11_desc->ViewDimension;
    memcpy(&desc->u, &d3d11_desc->u, sizeof(desc->u));
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

static void wined3d_depth_stencil_view_desc_from_d3d11(struct wined3d_rendertarget_view_desc *wined3d_desc,
        const D3D11_DEPTH_STENCIL_VIEW_DESC *desc)
{
    wined3d_desc->format_id = wined3dformat_from_dxgi_format(desc->Format);

    if (desc->Flags)
        FIXME("Unhandled depth stencil view flags %#x.\n", desc->Flags);

    switch (desc->ViewDimension)
    {
        case D3D11_DSV_DIMENSION_TEXTURE1D:
            wined3d_desc->u.texture.level_idx = desc->u.Texture1D.MipSlice;
            wined3d_desc->u.texture.layer_idx = 0;
            wined3d_desc->u.texture.layer_count = 1;
            break;

        case D3D11_DSV_DIMENSION_TEXTURE1DARRAY:
            wined3d_desc->u.texture.level_idx = desc->u.Texture1DArray.MipSlice;
            wined3d_desc->u.texture.layer_idx = desc->u.Texture1DArray.FirstArraySlice;
            wined3d_desc->u.texture.layer_count = desc->u.Texture1DArray.ArraySize;
            break;

        case D3D11_DSV_DIMENSION_TEXTURE2D:
            wined3d_desc->u.texture.level_idx = desc->u.Texture2D.MipSlice;
            wined3d_desc->u.texture.layer_idx = 0;
            wined3d_desc->u.texture.layer_count = 1;
            break;

        case D3D11_DSV_DIMENSION_TEXTURE2DARRAY:
            wined3d_desc->u.texture.level_idx = desc->u.Texture2DArray.MipSlice;
            wined3d_desc->u.texture.layer_idx = desc->u.Texture2DArray.FirstArraySlice;
            wined3d_desc->u.texture.layer_count = desc->u.Texture2DArray.ArraySize;
            break;

        case D3D11_DSV_DIMENSION_TEXTURE2DMS:
            wined3d_desc->u.texture.level_idx = 0;
            wined3d_desc->u.texture.layer_idx = 0;
            wined3d_desc->u.texture.layer_count = 1;
            break;

        case D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY:
            wined3d_desc->u.texture.level_idx = 0;
            wined3d_desc->u.texture.layer_idx = desc->u.Texture2DMSArray.FirstArraySlice;
            wined3d_desc->u.texture.layer_count = desc->u.Texture2DMSArray.ArraySize;
            break;

        default:
            FIXME("Unhandled view dimension %#x.\n", desc->ViewDimension);
            wined3d_desc->u.texture.level_idx = 0;
            wined3d_desc->u.texture.layer_idx = 0;
            wined3d_desc->u.texture.layer_count = 1;
            break;
    }
}

static HRESULT d3d_depthstencil_view_init(struct d3d_depthstencil_view *view, struct d3d_device *device,
        ID3D11Resource *resource, const D3D11_DEPTH_STENCIL_VIEW_DESC *desc)
{
    struct wined3d_rendertarget_view_desc wined3d_desc;
    struct wined3d_resource *wined3d_resource;
    HRESULT hr;

    view->ID3D11DepthStencilView_iface.lpVtbl = &d3d11_depthstencil_view_vtbl;
    view->ID3D10DepthStencilView_iface.lpVtbl = &d3d10_depthstencil_view_vtbl;
    view->refcount = 1;

    if (!desc)
    {
        if (FAILED(hr = set_dsdesc_from_resource(&view->desc, resource)))
            return hr;
    }
    else
    {
        view->desc = *desc;
    }

    wined3d_mutex_lock();
    if (!(wined3d_resource = wined3d_resource_from_d3d11_resource(resource)))
    {
        wined3d_mutex_unlock();
        ERR("Failed to get wined3d resource for d3d11 resource %p.\n", resource);
        return E_FAIL;
    }

    wined3d_depth_stencil_view_desc_from_d3d11(&wined3d_desc, &view->desc);
    if (FAILED(hr = wined3d_rendertarget_view_create(&wined3d_desc, wined3d_resource,
            view, &d3d10_null_wined3d_parent_ops, &view->wined3d_view)))
    {
        wined3d_mutex_unlock();
        WARN("Failed to create a wined3d rendertarget view, hr %#x.\n", hr);
        return hr;
    }

    wined3d_private_store_init(&view->private_store);
    wined3d_mutex_unlock();
    view->resource = resource;
    ID3D11Resource_AddRef(resource);
    view->device = &device->ID3D11Device_iface;
    ID3D11Device_AddRef(view->device);

    return S_OK;
}

HRESULT d3d_depthstencil_view_create(struct d3d_device *device, ID3D11Resource *resource,
        const D3D11_DEPTH_STENCIL_VIEW_DESC *desc, struct d3d_depthstencil_view **view)
{
    struct d3d_depthstencil_view *object;
    HRESULT hr;

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d3d_depthstencil_view_init(object, device, resource, desc)))
    {
        WARN("Failed to initialize depthstencil view, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created depthstencil view %p.\n", object);
    *view = object;

    return S_OK;
}

struct d3d_depthstencil_view *unsafe_impl_from_ID3D10DepthStencilView(ID3D10DepthStencilView *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d10_depthstencil_view_vtbl);

    return impl_from_ID3D10DepthStencilView(iface);
}

/* ID3D11RenderTargetView methods */

static inline struct d3d_rendertarget_view *impl_from_ID3D11RenderTargetView(ID3D11RenderTargetView *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_rendertarget_view, ID3D11RenderTargetView_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_rendertarget_view_QueryInterface(ID3D11RenderTargetView *iface,
        REFIID riid, void **object)
{
    struct d3d_rendertarget_view *view = impl_from_ID3D11RenderTargetView(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D11RenderTargetView)
            || IsEqualGUID(riid, &IID_ID3D11View)
            || IsEqualGUID(riid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D11RenderTargetView_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_ID3D10RenderTargetView)
            || IsEqualGUID(riid, &IID_ID3D10View)
            || IsEqualGUID(riid, &IID_ID3D10DeviceChild))
    {
        ID3D10RenderTargetView_AddRef(&view->ID3D10RenderTargetView_iface);
        *object = &view->ID3D10RenderTargetView_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d11_rendertarget_view_AddRef(ID3D11RenderTargetView *iface)
{
    struct d3d_rendertarget_view *view = impl_from_ID3D11RenderTargetView(iface);
    ULONG refcount = InterlockedIncrement(&view->refcount);

    TRACE("%p increasing refcount to %u.\n", view, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_rendertarget_view_Release(ID3D11RenderTargetView *iface)
{
    struct d3d_rendertarget_view *view = impl_from_ID3D11RenderTargetView(iface);
    ULONG refcount = InterlockedDecrement(&view->refcount);

    TRACE("%p decreasing refcount to %u.\n", view, refcount);

    if (!refcount)
    {
        wined3d_mutex_lock();
        wined3d_rendertarget_view_decref(view->wined3d_view);
        ID3D11Resource_Release(view->resource);
        ID3D11Device_Release(view->device);
        wined3d_private_store_cleanup(&view->private_store);
        wined3d_mutex_unlock();
        HeapFree(GetProcessHeap(), 0, view);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d11_rendertarget_view_GetDevice(ID3D11RenderTargetView *iface,
        ID3D11Device **device)
{
    struct d3d_rendertarget_view *view = impl_from_ID3D11RenderTargetView(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = view->device;
    ID3D11Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d11_rendertarget_view_GetPrivateData(ID3D11RenderTargetView *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_rendertarget_view *view = impl_from_ID3D11RenderTargetView(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&view->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_rendertarget_view_SetPrivateData(ID3D11RenderTargetView *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_rendertarget_view *view = impl_from_ID3D11RenderTargetView(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&view->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_rendertarget_view_SetPrivateDataInterface(ID3D11RenderTargetView *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_rendertarget_view *view = impl_from_ID3D11RenderTargetView(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&view->private_store, guid, data);
}

static void STDMETHODCALLTYPE d3d11_rendertarget_view_GetResource(ID3D11RenderTargetView *iface,
        ID3D11Resource **resource)
{
    struct d3d_rendertarget_view *view = impl_from_ID3D11RenderTargetView(iface);

    TRACE("iface %p, resource %p.\n", iface, resource);

    *resource = view->resource;
    ID3D11Resource_AddRef(*resource);
}

static void STDMETHODCALLTYPE d3d11_rendertarget_view_GetDesc(ID3D11RenderTargetView *iface,
        D3D11_RENDER_TARGET_VIEW_DESC *desc)
{
    struct d3d_rendertarget_view *view = impl_from_ID3D11RenderTargetView(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    *desc = view->desc;
}

static const struct ID3D11RenderTargetViewVtbl d3d11_rendertarget_view_vtbl =
{
    /* IUnknown methods */
    d3d11_rendertarget_view_QueryInterface,
    d3d11_rendertarget_view_AddRef,
    d3d11_rendertarget_view_Release,
    /* ID3D11DeviceChild methods */
    d3d11_rendertarget_view_GetDevice,
    d3d11_rendertarget_view_GetPrivateData,
    d3d11_rendertarget_view_SetPrivateData,
    d3d11_rendertarget_view_SetPrivateDataInterface,
    /* ID3D11View methods */
    d3d11_rendertarget_view_GetResource,
    /* ID3D11RenderTargetView methods */
    d3d11_rendertarget_view_GetDesc,
};

/* ID3D10RenderTargetView methods */

static inline struct d3d_rendertarget_view *impl_from_ID3D10RenderTargetView(ID3D10RenderTargetView *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_rendertarget_view, ID3D10RenderTargetView_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_rendertarget_view_QueryInterface(ID3D10RenderTargetView *iface,
        REFIID riid, void **object)
{
    struct d3d_rendertarget_view *view = impl_from_ID3D10RenderTargetView(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return d3d11_rendertarget_view_QueryInterface(&view->ID3D11RenderTargetView_iface, riid, object);
}

static ULONG STDMETHODCALLTYPE d3d10_rendertarget_view_AddRef(ID3D10RenderTargetView *iface)
{
    struct d3d_rendertarget_view *view = impl_from_ID3D10RenderTargetView(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_rendertarget_view_AddRef(&view->ID3D11RenderTargetView_iface);
}

static ULONG STDMETHODCALLTYPE d3d10_rendertarget_view_Release(ID3D10RenderTargetView *iface)
{
    struct d3d_rendertarget_view *view = impl_from_ID3D10RenderTargetView(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_rendertarget_view_Release(&view->ID3D11RenderTargetView_iface);
}

/* ID3D10DeviceChild methods */

static void STDMETHODCALLTYPE d3d10_rendertarget_view_GetDevice(ID3D10RenderTargetView *iface, ID3D10Device **device)
{
    struct d3d_rendertarget_view *view = impl_from_ID3D10RenderTargetView(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    ID3D11Device_QueryInterface(view->device, &IID_ID3D10Device, (void **)device);
}

static HRESULT STDMETHODCALLTYPE d3d10_rendertarget_view_GetPrivateData(ID3D10RenderTargetView *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_rendertarget_view *view = impl_from_ID3D10RenderTargetView(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&view->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_rendertarget_view_SetPrivateData(ID3D10RenderTargetView *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_rendertarget_view *view = impl_from_ID3D10RenderTargetView(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&view->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_rendertarget_view_SetPrivateDataInterface(ID3D10RenderTargetView *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_rendertarget_view *view = impl_from_ID3D10RenderTargetView(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&view->private_store, guid, data);
}

/* ID3D10View methods */

static void STDMETHODCALLTYPE d3d10_rendertarget_view_GetResource(ID3D10RenderTargetView *iface,
        ID3D10Resource **resource)
{
    struct d3d_rendertarget_view *view = impl_from_ID3D10RenderTargetView(iface);

    TRACE("iface %p, resource %p\n", iface, resource);

    ID3D11Resource_QueryInterface(view->resource, &IID_ID3D10Resource, (void **)resource);
}

/* ID3D10RenderTargetView methods */

static void STDMETHODCALLTYPE d3d10_rendertarget_view_GetDesc(ID3D10RenderTargetView *iface,
        D3D10_RENDER_TARGET_VIEW_DESC *desc)
{
    struct d3d_rendertarget_view *view = impl_from_ID3D10RenderTargetView(iface);

    TRACE("iface %p, desc %p\n", iface, desc);

    memcpy(desc, &view->desc, sizeof(*desc));
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

static void wined3d_rendertarget_view_desc_from_d3d11(struct wined3d_rendertarget_view_desc *wined3d_desc,
        const D3D11_RENDER_TARGET_VIEW_DESC *desc)
{
    wined3d_desc->format_id = wined3dformat_from_dxgi_format(desc->Format);

    switch (desc->ViewDimension)
    {
        case D3D11_RTV_DIMENSION_BUFFER:
            wined3d_desc->u.buffer.start_idx = desc->u.Buffer.u1.FirstElement;
            wined3d_desc->u.buffer.count = desc->u.Buffer.u2.NumElements;
            break;

        case D3D11_RTV_DIMENSION_TEXTURE1D:
            wined3d_desc->u.texture.level_idx = desc->u.Texture1D.MipSlice;
            wined3d_desc->u.texture.layer_idx = 0;
            wined3d_desc->u.texture.layer_count = 1;
            break;

        case D3D11_RTV_DIMENSION_TEXTURE1DARRAY:
            wined3d_desc->u.texture.level_idx = desc->u.Texture1DArray.MipSlice;
            wined3d_desc->u.texture.layer_idx = desc->u.Texture1DArray.FirstArraySlice;
            wined3d_desc->u.texture.layer_count = desc->u.Texture1DArray.ArraySize;
            break;

        case D3D11_RTV_DIMENSION_TEXTURE2D:
            wined3d_desc->u.texture.level_idx = desc->u.Texture2D.MipSlice;
            wined3d_desc->u.texture.layer_idx = 0;
            wined3d_desc->u.texture.layer_count = 1;
            break;

        case D3D11_RTV_DIMENSION_TEXTURE2DARRAY:
            wined3d_desc->u.texture.level_idx = desc->u.Texture2DArray.MipSlice;
            wined3d_desc->u.texture.layer_idx = desc->u.Texture2DArray.FirstArraySlice;
            wined3d_desc->u.texture.layer_count = desc->u.Texture2DArray.ArraySize;
            break;

        case D3D11_RTV_DIMENSION_TEXTURE2DMS:
            wined3d_desc->u.texture.level_idx = 0;
            wined3d_desc->u.texture.layer_idx = 0;
            wined3d_desc->u.texture.layer_count = 1;
            break;

        case D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY:
            wined3d_desc->u.texture.level_idx = 0;
            wined3d_desc->u.texture.layer_idx = desc->u.Texture2DMSArray.FirstArraySlice;
            wined3d_desc->u.texture.layer_count = desc->u.Texture2DMSArray.ArraySize;
            break;

        case D3D11_RTV_DIMENSION_TEXTURE3D:
            wined3d_desc->u.texture.level_idx = desc->u.Texture3D.MipSlice;
            wined3d_desc->u.texture.layer_idx = desc->u.Texture3D.FirstWSlice;
            wined3d_desc->u.texture.layer_count = desc->u.Texture3D.WSize;
            break;

        default:
            FIXME("Unhandled view dimension %#x.\n", desc->ViewDimension);
            wined3d_desc->u.texture.level_idx = 0;
            wined3d_desc->u.texture.layer_idx = 0;
            wined3d_desc->u.texture.layer_count = 1;
            break;
    }
}

static HRESULT d3d_rendertarget_view_init(struct d3d_rendertarget_view *view, struct d3d_device *device,
        ID3D11Resource *resource, const D3D11_RENDER_TARGET_VIEW_DESC *desc)
{
    struct wined3d_rendertarget_view_desc wined3d_desc;
    struct wined3d_resource *wined3d_resource;
    HRESULT hr;

    view->ID3D11RenderTargetView_iface.lpVtbl = &d3d11_rendertarget_view_vtbl;
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

    wined3d_mutex_lock();
    if (!(wined3d_resource = wined3d_resource_from_d3d11_resource(resource)))
    {
        wined3d_mutex_unlock();
        ERR("Failed to get wined3d resource for d3d11 resource %p.\n", resource);
        return E_FAIL;
    }

    wined3d_rendertarget_view_desc_from_d3d11(&wined3d_desc, &view->desc);
    if (FAILED(hr = wined3d_rendertarget_view_create(&wined3d_desc, wined3d_resource,
            view, &d3d10_null_wined3d_parent_ops, &view->wined3d_view)))
    {
        wined3d_mutex_unlock();
        WARN("Failed to create a wined3d rendertarget view, hr %#x.\n", hr);
        return hr;
    }

    wined3d_private_store_init(&view->private_store);
    wined3d_mutex_unlock();
    view->resource = resource;
    ID3D11Resource_AddRef(resource);
    view->device = &device->ID3D11Device_iface;
    ID3D11Device_AddRef(view->device);

    return S_OK;
}

HRESULT d3d_rendertarget_view_create(struct d3d_device *device, ID3D11Resource *resource,
        const D3D11_RENDER_TARGET_VIEW_DESC *desc, struct d3d_rendertarget_view **view)
{
    struct d3d_rendertarget_view *object;
    HRESULT hr;

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d3d_rendertarget_view_init(object, device, resource, desc)))
    {
        WARN("Failed to initialize rendertarget view, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created rendertarget view %p.\n", object);
    *view = object;

    return S_OK;
}

struct d3d_rendertarget_view *unsafe_impl_from_ID3D10RenderTargetView(ID3D10RenderTargetView *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d10_rendertarget_view_vtbl);

    return impl_from_ID3D10RenderTargetView(iface);
}

/* ID3D11ShaderResourceView methods */

struct d3d_shader_resource_view *impl_from_ID3D11ShaderResourceView(ID3D11ShaderResourceView *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_shader_resource_view, ID3D11ShaderResourceView_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_shader_resource_view_QueryInterface(ID3D11ShaderResourceView *iface,
        REFIID riid, void **object)
{
    struct d3d_shader_resource_view *view = impl_from_ID3D11ShaderResourceView(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D11ShaderResourceView)
            || IsEqualGUID(riid, &IID_ID3D11View)
            || IsEqualGUID(riid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D11ShaderResourceView_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_ID3D10ShaderResourceView)
            || IsEqualGUID(riid, &IID_ID3D10View)
            || IsEqualGUID(riid, &IID_ID3D10DeviceChild))
    {
        ID3D10ShaderResourceView_AddRef(&view->ID3D10ShaderResourceView_iface);
        *object = &view->ID3D10ShaderResourceView_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d11_shader_resource_view_AddRef(ID3D11ShaderResourceView *iface)
{
    struct d3d_shader_resource_view *view = impl_from_ID3D11ShaderResourceView(iface);
    ULONG refcount = InterlockedIncrement(&view->refcount);

    TRACE("%p increasing refcount to %u.\n", view, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_shader_resource_view_Release(ID3D11ShaderResourceView *iface)
{
    struct d3d_shader_resource_view *view = impl_from_ID3D11ShaderResourceView(iface);
    ULONG refcount = InterlockedDecrement(&view->refcount);

    TRACE("%p decreasing refcount to %u.\n", view, refcount);

    if (!refcount)
    {
        wined3d_mutex_lock();
        wined3d_shader_resource_view_decref(view->wined3d_view);
        ID3D11Resource_Release(view->resource);
        ID3D11Device_Release(view->device);
        wined3d_private_store_cleanup(&view->private_store);
        wined3d_mutex_unlock();
        HeapFree(GetProcessHeap(), 0, view);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d11_shader_resource_view_GetDevice(ID3D11ShaderResourceView *iface,
        ID3D11Device **device)
{
    struct d3d_shader_resource_view *view = impl_from_ID3D11ShaderResourceView(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = view->device;
    ID3D11Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d11_shader_resource_view_GetPrivateData(ID3D11ShaderResourceView *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_shader_resource_view *view = impl_from_ID3D11ShaderResourceView(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&view->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_shader_resource_view_SetPrivateData(ID3D11ShaderResourceView *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_shader_resource_view *view = impl_from_ID3D11ShaderResourceView(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&view->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_shader_resource_view_SetPrivateDataInterface(ID3D11ShaderResourceView *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_shader_resource_view *view = impl_from_ID3D11ShaderResourceView(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&view->private_store, guid, data);
}

static void STDMETHODCALLTYPE d3d11_shader_resource_view_GetResource(ID3D11ShaderResourceView *iface,
        ID3D11Resource **resource)
{
    struct d3d_shader_resource_view *view = impl_from_ID3D11ShaderResourceView(iface);

    TRACE("iface %p, resource %p.\n", iface, resource);

    *resource = view->resource;
    ID3D11Resource_AddRef(*resource);
}

static void STDMETHODCALLTYPE d3d11_shader_resource_view_GetDesc(ID3D11ShaderResourceView *iface,
        D3D11_SHADER_RESOURCE_VIEW_DESC *desc)
{
    struct d3d_shader_resource_view *view = impl_from_ID3D11ShaderResourceView(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    *desc = view->desc;
}

static const struct ID3D11ShaderResourceViewVtbl d3d11_shader_resource_view_vtbl =
{
    /* IUnknown methods */
    d3d11_shader_resource_view_QueryInterface,
    d3d11_shader_resource_view_AddRef,
    d3d11_shader_resource_view_Release,
    /* ID3D11DeviceChild methods */
    d3d11_shader_resource_view_GetDevice,
    d3d11_shader_resource_view_GetPrivateData,
    d3d11_shader_resource_view_SetPrivateData,
    d3d11_shader_resource_view_SetPrivateDataInterface,
    /* ID3D11View methods */
    d3d11_shader_resource_view_GetResource,
    /* ID3D11ShaderResourceView methods */
    d3d11_shader_resource_view_GetDesc,
};

/* ID3D10ShaderResourceView methods */

static inline struct d3d_shader_resource_view *impl_from_ID3D10ShaderResourceView(ID3D10ShaderResourceView *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_shader_resource_view, ID3D10ShaderResourceView_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_shader_resource_view_QueryInterface(ID3D10ShaderResourceView *iface,
        REFIID riid, void **object)
{
    struct d3d_shader_resource_view *view = impl_from_ID3D10ShaderResourceView(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return d3d11_shader_resource_view_QueryInterface(&view->ID3D11ShaderResourceView_iface, riid, object);
}

static ULONG STDMETHODCALLTYPE d3d10_shader_resource_view_AddRef(ID3D10ShaderResourceView *iface)
{
    struct d3d_shader_resource_view *view = impl_from_ID3D10ShaderResourceView(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_shader_resource_view_AddRef(&view->ID3D11ShaderResourceView_iface);
}

static ULONG STDMETHODCALLTYPE d3d10_shader_resource_view_Release(ID3D10ShaderResourceView *iface)
{
    struct d3d_shader_resource_view *view = impl_from_ID3D10ShaderResourceView(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_shader_resource_view_Release(&view->ID3D11ShaderResourceView_iface);
}

/* ID3D10DeviceChild methods */

static void STDMETHODCALLTYPE d3d10_shader_resource_view_GetDevice(ID3D10ShaderResourceView *iface,
        ID3D10Device **device)
{
    struct d3d_shader_resource_view *view = impl_from_ID3D10ShaderResourceView(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    ID3D11Device_QueryInterface(view->device, &IID_ID3D10Device, (void **)device);
}

static HRESULT STDMETHODCALLTYPE d3d10_shader_resource_view_GetPrivateData(ID3D10ShaderResourceView *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_shader_resource_view *view = impl_from_ID3D10ShaderResourceView(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&view->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_shader_resource_view_SetPrivateData(ID3D10ShaderResourceView *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_shader_resource_view *view = impl_from_ID3D10ShaderResourceView(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&view->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_shader_resource_view_SetPrivateDataInterface(ID3D10ShaderResourceView *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_shader_resource_view *view = impl_from_ID3D10ShaderResourceView(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&view->private_store, guid, data);
}

/* ID3D10View methods */

static void STDMETHODCALLTYPE d3d10_shader_resource_view_GetResource(ID3D10ShaderResourceView *iface,
        ID3D10Resource **resource)
{
    struct d3d_shader_resource_view *view = impl_from_ID3D10ShaderResourceView(iface);

    TRACE("iface %p, resource %p.\n", iface, resource);

    ID3D11Resource_QueryInterface(view->resource, &IID_ID3D10Resource, (void **)resource);
}

/* ID3D10ShaderResourceView methods */

static void STDMETHODCALLTYPE d3d10_shader_resource_view_GetDesc(ID3D10ShaderResourceView *iface,
        D3D10_SHADER_RESOURCE_VIEW_DESC *desc)
{
    struct d3d_shader_resource_view *view = impl_from_ID3D10ShaderResourceView(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    memcpy(desc, &view->desc, sizeof(*desc));
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

HRESULT d3d_shader_resource_view_init(struct d3d_shader_resource_view *view, struct d3d_device *device,
        ID3D11Resource *resource, const D3D11_SHADER_RESOURCE_VIEW_DESC *desc)
{
    struct wined3d_resource *wined3d_resource;
    HRESULT hr;

    view->ID3D11ShaderResourceView_iface.lpVtbl = &d3d11_shader_resource_view_vtbl;
    view->ID3D10ShaderResourceView_iface.lpVtbl = &d3d10_shader_resource_view_vtbl;
    view->refcount = 1;

    if (!desc)
    {
        if (FAILED(hr = set_srdesc_from_resource(&view->desc, resource)))
            return hr;
    }
    else
    {
        view->desc = *desc;
    }

    wined3d_mutex_lock();
    if (!(wined3d_resource = wined3d_resource_from_d3d11_resource(resource)))
    {
        ERR("Failed to get wined3d resource for d3d10 resource %p.\n", resource);
        return E_FAIL;
    }

    if (FAILED(hr = wined3d_shader_resource_view_create(wined3d_resource,
            view, &d3d10_null_wined3d_parent_ops, &view->wined3d_view)))
    {
        WARN("Failed to create wined3d shader resource view, hr %#x.\n", hr);
        return hr;
    }

    wined3d_private_store_init(&view->private_store);
    wined3d_mutex_unlock();
    view->resource = resource;
    ID3D11Resource_AddRef(resource);
    view->device = &device->ID3D11Device_iface;
    ID3D11Device_AddRef(view->device);

    return S_OK;
}

struct d3d_shader_resource_view *unsafe_impl_from_ID3D10ShaderResourceView(ID3D10ShaderResourceView *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d10_shader_resource_view_vtbl);
    return CONTAINING_RECORD(iface, struct d3d_shader_resource_view, ID3D10ShaderResourceView_iface);
}
