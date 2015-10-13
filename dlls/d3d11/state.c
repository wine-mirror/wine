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

#include "d3d11_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d11);

#define D3D10_FILTER_MIP_MASK       0x01
#define D3D10_FILTER_MAG_MASK       0x04
#define D3D10_FILTER_MIN_MASK       0x10
#define D3D10_FILTER_ANISO_MASK     0x40
#define D3D10_FILTER_COMPARE_MASK   0x80

/* ID3D11BlendState methods */

static inline struct d3d_blend_state *impl_from_ID3D11BlendState(ID3D11BlendState *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_blend_state, ID3D11BlendState_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_blend_state_QueryInterface(ID3D11BlendState *iface,
        REFIID riid, void **object)
{
    struct d3d_blend_state *state = impl_from_ID3D11BlendState(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D11BlendState)
            || IsEqualGUID(riid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D11BlendState_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_ID3D10BlendState)
            || IsEqualGUID(riid, &IID_ID3D10DeviceChild))
    {
        ID3D10BlendState_AddRef(&state->ID3D10BlendState_iface);
        *object = &state->ID3D10BlendState_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d11_blend_state_AddRef(ID3D11BlendState *iface)
{
    struct d3d_blend_state *state = impl_from_ID3D11BlendState(iface);
    ULONG refcount = InterlockedIncrement(&state->refcount);

    TRACE("%p increasing refcount to %u.\n", state, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_blend_state_Release(ID3D11BlendState *iface)
{
    struct d3d_blend_state *state = impl_from_ID3D11BlendState(iface);
    ULONG refcount = InterlockedDecrement(&state->refcount);

    TRACE("%p decreasing refcount to %u.\n", state, refcount);

    if (!refcount)
    {
        struct d3d_device *device = impl_from_ID3D11Device(state->device);
        wined3d_mutex_lock();
        wine_rb_remove(&device->blend_states, &state->desc);
        ID3D11Device_Release(state->device);
        wined3d_private_store_cleanup(&state->private_store);
        wined3d_mutex_unlock();
        HeapFree(GetProcessHeap(), 0, state);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d11_blend_state_GetDevice(ID3D11BlendState *iface,
        ID3D11Device **device)
{
    struct d3d_blend_state *state = impl_from_ID3D11BlendState(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = state->device;
    ID3D11Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d11_blend_state_GetPrivateData(ID3D11BlendState *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_blend_state *state = impl_from_ID3D11BlendState(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_blend_state_SetPrivateData(ID3D11BlendState *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_blend_state *state = impl_from_ID3D11BlendState(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_blend_state_SetPrivateDataInterface(ID3D11BlendState *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_blend_state *state = impl_from_ID3D11BlendState(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&state->private_store, guid, data);
}

static void STDMETHODCALLTYPE d3d11_blend_state_GetDesc(ID3D11BlendState *iface, D3D11_BLEND_DESC *desc)
{
    struct d3d_blend_state *state = impl_from_ID3D11BlendState(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    *desc = state->desc;
}

static const struct ID3D11BlendStateVtbl d3d11_blend_state_vtbl =
{
    /* IUnknown methods */
    d3d11_blend_state_QueryInterface,
    d3d11_blend_state_AddRef,
    d3d11_blend_state_Release,
    /* ID3D11DeviceChild methods */
    d3d11_blend_state_GetDevice,
    d3d11_blend_state_GetPrivateData,
    d3d11_blend_state_SetPrivateData,
    d3d11_blend_state_SetPrivateDataInterface,
    /* ID3D11BlendState methods */
    d3d11_blend_state_GetDesc,
};

/* ID3D10BlendState methods */

static inline struct d3d_blend_state *impl_from_ID3D10BlendState(ID3D10BlendState *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_blend_state, ID3D10BlendState_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_blend_state_QueryInterface(ID3D10BlendState *iface,
        REFIID riid, void **object)
{
    struct d3d_blend_state *state = impl_from_ID3D10BlendState(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return d3d11_blend_state_QueryInterface(&state->ID3D11BlendState_iface, riid, object);
}

static ULONG STDMETHODCALLTYPE d3d10_blend_state_AddRef(ID3D10BlendState *iface)
{
    struct d3d_blend_state *state = impl_from_ID3D10BlendState(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_blend_state_AddRef(&state->ID3D11BlendState_iface);
}

static ULONG STDMETHODCALLTYPE d3d10_blend_state_Release(ID3D10BlendState *iface)
{
    struct d3d_blend_state *state = impl_from_ID3D10BlendState(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_blend_state_Release(&state->ID3D11BlendState_iface);
}

/* ID3D10DeviceChild methods */

static void STDMETHODCALLTYPE d3d10_blend_state_GetDevice(ID3D10BlendState *iface, ID3D10Device **device)
{
    struct d3d_blend_state *state = impl_from_ID3D10BlendState(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    ID3D11Device_QueryInterface(state->device, &IID_ID3D10Device, (void **)device);
}

static HRESULT STDMETHODCALLTYPE d3d10_blend_state_GetPrivateData(ID3D10BlendState *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_blend_state *state = impl_from_ID3D10BlendState(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_blend_state_SetPrivateData(ID3D10BlendState *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_blend_state *state = impl_from_ID3D10BlendState(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_blend_state_SetPrivateDataInterface(ID3D10BlendState *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_blend_state *state = impl_from_ID3D10BlendState(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&state->private_store, guid, data);
}

/* ID3D10BlendState methods */

static void STDMETHODCALLTYPE d3d10_blend_state_GetDesc(ID3D10BlendState *iface,
        D3D10_BLEND_DESC *desc)
{
    struct d3d_blend_state *state = impl_from_ID3D10BlendState(iface);
    const D3D11_BLEND_DESC *d3d11_desc = &state->desc;
    unsigned int i;

    TRACE("iface %p, desc %p.\n", iface, desc);

    desc->AlphaToCoverageEnable = d3d11_desc->AlphaToCoverageEnable;
    desc->SrcBlend = d3d11_desc->RenderTarget[0].SrcBlend;
    desc->DestBlend = d3d11_desc->RenderTarget[0].DestBlend;
    desc->BlendOp = d3d11_desc->RenderTarget[0].BlendOp;
    desc->SrcBlendAlpha = d3d11_desc->RenderTarget[0].SrcBlendAlpha;
    desc->DestBlendAlpha = d3d11_desc->RenderTarget[0].DestBlendAlpha;
    desc->BlendOpAlpha = d3d11_desc->RenderTarget[0].BlendOpAlpha;
    for (i = 0; i < D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        desc->BlendEnable[i] = d3d11_desc->RenderTarget[i].BlendEnable;
        desc->RenderTargetWriteMask[i] = d3d11_desc->RenderTarget[i].RenderTargetWriteMask;
    }
}

static const struct ID3D10BlendStateVtbl d3d10_blend_state_vtbl =
{
    /* IUnknown methods */
    d3d10_blend_state_QueryInterface,
    d3d10_blend_state_AddRef,
    d3d10_blend_state_Release,
    /* ID3D10DeviceChild methods */
    d3d10_blend_state_GetDevice,
    d3d10_blend_state_GetPrivateData,
    d3d10_blend_state_SetPrivateData,
    d3d10_blend_state_SetPrivateDataInterface,
    /* ID3D10BlendState methods */
    d3d10_blend_state_GetDesc,
};

HRESULT d3d_blend_state_init(struct d3d_blend_state *state, struct d3d_device *device,
        const D3D11_BLEND_DESC *desc)
{
    state->ID3D11BlendState_iface.lpVtbl = &d3d11_blend_state_vtbl;
    state->ID3D10BlendState_iface.lpVtbl = &d3d10_blend_state_vtbl;
    state->refcount = 1;
    wined3d_mutex_lock();
    wined3d_private_store_init(&state->private_store);
    state->desc = *desc;

    if (wine_rb_put(&device->blend_states, desc, &state->entry) == -1)
    {
        ERR("Failed to insert blend state entry.\n");
        wined3d_private_store_cleanup(&state->private_store);
        wined3d_mutex_unlock();
        return E_FAIL;
    }
    wined3d_mutex_unlock();

    state->device = &device->ID3D11Device_iface;
    ID3D11Device_AddRef(state->device);

    return S_OK;
}

struct d3d_blend_state *unsafe_impl_from_ID3D10BlendState(ID3D10BlendState *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d10_blend_state_vtbl);

    return impl_from_ID3D10BlendState(iface);
}

/* ID3D11DepthStencilState methods */

static inline struct d3d_depthstencil_state *impl_from_ID3D11DepthStencilState(ID3D11DepthStencilState *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_depthstencil_state, ID3D11DepthStencilState_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_depthstencil_state_QueryInterface(ID3D11DepthStencilState *iface,
        REFIID riid, void **object)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D11DepthStencilState(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D11DepthStencilState)
            || IsEqualGUID(riid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D11DepthStencilState_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_ID3D10DepthStencilState)
            || IsEqualGUID(riid, &IID_ID3D10DeviceChild))
    {
        ID3D10DepthStencilState_AddRef(&state->ID3D10DepthStencilState_iface);
        *object = &state->ID3D10DepthStencilState_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d11_depthstencil_state_AddRef(ID3D11DepthStencilState *iface)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D11DepthStencilState(iface);
    ULONG refcount = InterlockedIncrement(&state->refcount);

    TRACE("%p increasing refcount to %u.\n", state, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_depthstencil_state_Release(ID3D11DepthStencilState *iface)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D11DepthStencilState(iface);
    ULONG refcount = InterlockedDecrement(&state->refcount);

    TRACE("%p decreasing refcount to %u.\n", state, refcount);

    if (!refcount)
    {
        struct d3d_device *device = impl_from_ID3D11Device(state->device);
        wined3d_mutex_lock();
        wine_rb_remove(&device->depthstencil_states, &state->desc);
        ID3D11Device_Release(state->device);
        wined3d_private_store_cleanup(&state->private_store);
        wined3d_mutex_unlock();
        HeapFree(GetProcessHeap(), 0, state);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d11_depthstencil_state_GetDevice(ID3D11DepthStencilState *iface,
        ID3D11Device **device)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D11DepthStencilState(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = state->device;
    ID3D11Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d11_depthstencil_state_GetPrivateData(ID3D11DepthStencilState *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D11DepthStencilState(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_depthstencil_state_SetPrivateData(ID3D11DepthStencilState *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D11DepthStencilState(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_depthstencil_state_SetPrivateDataInterface(ID3D11DepthStencilState *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D11DepthStencilState(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&state->private_store, guid, data);
}

static void STDMETHODCALLTYPE d3d11_depthstencil_state_GetDesc(ID3D11DepthStencilState *iface,
        D3D11_DEPTH_STENCIL_DESC *desc)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D11DepthStencilState(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    *desc = state->desc;
}

static const struct ID3D11DepthStencilStateVtbl d3d11_depthstencil_state_vtbl =
{
    /* IUnknown methods */
    d3d11_depthstencil_state_QueryInterface,
    d3d11_depthstencil_state_AddRef,
    d3d11_depthstencil_state_Release,
    /* ID3D11DeviceChild methods */
    d3d11_depthstencil_state_GetDevice,
    d3d11_depthstencil_state_GetPrivateData,
    d3d11_depthstencil_state_SetPrivateData,
    d3d11_depthstencil_state_SetPrivateDataInterface,
    /* ID3D11DepthStencilState methods */
    d3d11_depthstencil_state_GetDesc,
};

/* ID3D10DepthStencilState methods */

static inline struct d3d_depthstencil_state *impl_from_ID3D10DepthStencilState(ID3D10DepthStencilState *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_depthstencil_state, ID3D10DepthStencilState_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_depthstencil_state_QueryInterface(ID3D10DepthStencilState *iface,
        REFIID riid, void **object)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D10DepthStencilState(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return d3d11_depthstencil_state_QueryInterface(&state->ID3D11DepthStencilState_iface, riid, object);
}

static ULONG STDMETHODCALLTYPE d3d10_depthstencil_state_AddRef(ID3D10DepthStencilState *iface)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D10DepthStencilState(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_depthstencil_state_AddRef(&state->ID3D11DepthStencilState_iface);
}

static ULONG STDMETHODCALLTYPE d3d10_depthstencil_state_Release(ID3D10DepthStencilState *iface)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D10DepthStencilState(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_depthstencil_state_Release(&state->ID3D11DepthStencilState_iface);
}

/* ID3D10DeviceChild methods */

static void STDMETHODCALLTYPE d3d10_depthstencil_state_GetDevice(ID3D10DepthStencilState *iface, ID3D10Device **device)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D10DepthStencilState(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    ID3D11Device_QueryInterface(state->device, &IID_ID3D10Device, (void **)device);
}

static HRESULT STDMETHODCALLTYPE d3d10_depthstencil_state_GetPrivateData(ID3D10DepthStencilState *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D10DepthStencilState(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_depthstencil_state_SetPrivateData(ID3D10DepthStencilState *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D10DepthStencilState(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_depthstencil_state_SetPrivateDataInterface(ID3D10DepthStencilState *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D10DepthStencilState(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&state->private_store, guid, data);
}

/* ID3D10DepthStencilState methods */

static void STDMETHODCALLTYPE d3d10_depthstencil_state_GetDesc(ID3D10DepthStencilState *iface,
        D3D10_DEPTH_STENCIL_DESC *desc)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D10DepthStencilState(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    memcpy(desc, &state->desc, sizeof(*desc));
}

static const struct ID3D10DepthStencilStateVtbl d3d10_depthstencil_state_vtbl =
{
    /* IUnknown methods */
    d3d10_depthstencil_state_QueryInterface,
    d3d10_depthstencil_state_AddRef,
    d3d10_depthstencil_state_Release,
    /* ID3D10DeviceChild methods */
    d3d10_depthstencil_state_GetDevice,
    d3d10_depthstencil_state_GetPrivateData,
    d3d10_depthstencil_state_SetPrivateData,
    d3d10_depthstencil_state_SetPrivateDataInterface,
    /* ID3D10DepthStencilState methods */
    d3d10_depthstencil_state_GetDesc,
};

HRESULT d3d_depthstencil_state_init(struct d3d_depthstencil_state *state, struct d3d_device *device,
        const D3D11_DEPTH_STENCIL_DESC *desc)
{
    state->ID3D11DepthStencilState_iface.lpVtbl = &d3d11_depthstencil_state_vtbl;
    state->ID3D10DepthStencilState_iface.lpVtbl = &d3d10_depthstencil_state_vtbl;
    state->refcount = 1;
    wined3d_mutex_lock();
    wined3d_private_store_init(&state->private_store);
    state->desc = *desc;

    if (wine_rb_put(&device->depthstencil_states, desc, &state->entry) == -1)
    {
        ERR("Failed to insert depthstencil state entry.\n");
        wined3d_private_store_cleanup(&state->private_store);
        wined3d_mutex_unlock();
        return E_FAIL;
    }
    wined3d_mutex_unlock();

    state->device = &device->ID3D11Device_iface;
    ID3D11Device_AddRef(state->device);

    return S_OK;
}

struct d3d_depthstencil_state *unsafe_impl_from_ID3D10DepthStencilState(ID3D10DepthStencilState *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d10_depthstencil_state_vtbl);

    return impl_from_ID3D10DepthStencilState(iface);
}

/* ID3D11RasterizerState methods */

static inline struct d3d_rasterizer_state *impl_from_ID3D11RasterizerState(ID3D11RasterizerState *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_rasterizer_state, ID3D11RasterizerState_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_rasterizer_state_QueryInterface(ID3D11RasterizerState *iface,
        REFIID riid, void **object)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D11RasterizerState(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D11RasterizerState)
            || IsEqualGUID(riid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D11RasterizerState_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_ID3D10RasterizerState)
            || IsEqualGUID(riid, &IID_ID3D10DeviceChild))
    {
        ID3D10RasterizerState_AddRef(&state->ID3D10RasterizerState_iface);
        *object = &state->ID3D10RasterizerState_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d11_rasterizer_state_AddRef(ID3D11RasterizerState *iface)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D11RasterizerState(iface);
    ULONG refcount = InterlockedIncrement(&state->refcount);

    TRACE("%p increasing refcount to %u.\n", state, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_rasterizer_state_Release(ID3D11RasterizerState *iface)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D11RasterizerState(iface);
    ULONG refcount = InterlockedDecrement(&state->refcount);

    TRACE("%p decreasing refcount to %u.\n", state, refcount);

    if (!refcount)
    {
        struct d3d_device *device = impl_from_ID3D11Device(state->device);
        wined3d_mutex_lock();
        wine_rb_remove(&device->rasterizer_states, &state->desc);
        ID3D11Device_Release(state->device);
        wined3d_private_store_cleanup(&state->private_store);
        wined3d_mutex_unlock();
        HeapFree(GetProcessHeap(), 0, state);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d11_rasterizer_state_GetDevice(ID3D11RasterizerState *iface,
        ID3D11Device **device)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D11RasterizerState(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = state->device;
    ID3D11Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d11_rasterizer_state_GetPrivateData(ID3D11RasterizerState *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D11RasterizerState(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_rasterizer_state_SetPrivateData(ID3D11RasterizerState *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D11RasterizerState(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_rasterizer_state_SetPrivateDataInterface(ID3D11RasterizerState *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D11RasterizerState(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&state->private_store, guid, data);
}

static void STDMETHODCALLTYPE d3d11_rasterizer_state_GetDesc(ID3D11RasterizerState *iface,
        D3D11_RASTERIZER_DESC *desc)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D11RasterizerState(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    *desc = state->desc;
}

static const struct ID3D11RasterizerStateVtbl d3d11_rasterizer_state_vtbl =
{
    /* IUnknown methods */
    d3d11_rasterizer_state_QueryInterface,
    d3d11_rasterizer_state_AddRef,
    d3d11_rasterizer_state_Release,
    /* ID3D11DeviceChild methods */
    d3d11_rasterizer_state_GetDevice,
    d3d11_rasterizer_state_GetPrivateData,
    d3d11_rasterizer_state_SetPrivateData,
    d3d11_rasterizer_state_SetPrivateDataInterface,
    /* ID3D11RasterizerState methods */
    d3d11_rasterizer_state_GetDesc,
};

/* ID3D10RasterizerState methods */

static inline struct d3d_rasterizer_state *impl_from_ID3D10RasterizerState(ID3D10RasterizerState *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_rasterizer_state, ID3D10RasterizerState_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_rasterizer_state_QueryInterface(ID3D10RasterizerState *iface,
        REFIID riid, void **object)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D10RasterizerState(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return d3d11_rasterizer_state_QueryInterface(&state->ID3D11RasterizerState_iface, riid, object);
}

static ULONG STDMETHODCALLTYPE d3d10_rasterizer_state_AddRef(ID3D10RasterizerState *iface)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D10RasterizerState(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_rasterizer_state_AddRef(&state->ID3D11RasterizerState_iface);
}

static ULONG STDMETHODCALLTYPE d3d10_rasterizer_state_Release(ID3D10RasterizerState *iface)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D10RasterizerState(iface);

    TRACE("iface %p.\n", state);

    return d3d11_rasterizer_state_Release(&state->ID3D11RasterizerState_iface);
}

/* ID3D10DeviceChild methods */

static void STDMETHODCALLTYPE d3d10_rasterizer_state_GetDevice(ID3D10RasterizerState *iface, ID3D10Device **device)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D10RasterizerState(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    ID3D11Device_QueryInterface(state->device, &IID_ID3D10Device, (void **)device);
}

static HRESULT STDMETHODCALLTYPE d3d10_rasterizer_state_GetPrivateData(ID3D10RasterizerState *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D10RasterizerState(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_rasterizer_state_SetPrivateData(ID3D10RasterizerState *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D10RasterizerState(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_rasterizer_state_SetPrivateDataInterface(ID3D10RasterizerState *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D10RasterizerState(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&state->private_store, guid, data);
}

/* ID3D10RasterizerState methods */

static void STDMETHODCALLTYPE d3d10_rasterizer_state_GetDesc(ID3D10RasterizerState *iface,
        D3D10_RASTERIZER_DESC *desc)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D10RasterizerState(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    memcpy(desc, &state->desc, sizeof(*desc));
}

static const struct ID3D10RasterizerStateVtbl d3d10_rasterizer_state_vtbl =
{
    /* IUnknown methods */
    d3d10_rasterizer_state_QueryInterface,
    d3d10_rasterizer_state_AddRef,
    d3d10_rasterizer_state_Release,
    /* ID3D10DeviceChild methods */
    d3d10_rasterizer_state_GetDevice,
    d3d10_rasterizer_state_GetPrivateData,
    d3d10_rasterizer_state_SetPrivateData,
    d3d10_rasterizer_state_SetPrivateDataInterface,
    /* ID3D10RasterizerState methods */
    d3d10_rasterizer_state_GetDesc,
};

HRESULT d3d_rasterizer_state_init(struct d3d_rasterizer_state *state, struct d3d_device *device,
        const D3D11_RASTERIZER_DESC *desc)
{
    state->ID3D11RasterizerState_iface.lpVtbl = &d3d11_rasterizer_state_vtbl;
    state->ID3D10RasterizerState_iface.lpVtbl = &d3d10_rasterizer_state_vtbl;
    state->refcount = 1;
    wined3d_mutex_lock();
    wined3d_private_store_init(&state->private_store);
    state->desc = *desc;

    if (wine_rb_put(&device->rasterizer_states, desc, &state->entry) == -1)
    {
        ERR("Failed to insert rasterizer state entry.\n");
        wined3d_private_store_cleanup(&state->private_store);
        wined3d_mutex_unlock();
        return E_FAIL;
    }
    wined3d_mutex_unlock();

    state->device = &device->ID3D11Device_iface;
    ID3D11Device_AddRef(state->device);

    return S_OK;
}

struct d3d_rasterizer_state *unsafe_impl_from_ID3D10RasterizerState(ID3D10RasterizerState *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d10_rasterizer_state_vtbl);

    return impl_from_ID3D10RasterizerState(iface);
}

static inline struct d3d_sampler_state *impl_from_ID3D10SamplerState(ID3D10SamplerState *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_sampler_state, ID3D10SamplerState_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_sampler_state_QueryInterface(ID3D10SamplerState *iface,
        REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D10SamplerState)
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

static ULONG STDMETHODCALLTYPE d3d10_sampler_state_AddRef(ID3D10SamplerState *iface)
{
    struct d3d_sampler_state *This = impl_from_ID3D10SamplerState(iface);
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %u.\n", This, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d10_sampler_state_Release(ID3D10SamplerState *iface)
{
    struct d3d_sampler_state *state = impl_from_ID3D10SamplerState(iface);
    ULONG refcount = InterlockedDecrement(&state->refcount);

    TRACE("%p decreasing refcount to %u.\n", state, refcount);

    if (!refcount)
    {
        struct d3d_device *device = impl_from_ID3D10Device(state->device);

        wined3d_mutex_lock();
        wined3d_sampler_decref(state->wined3d_sampler);
        wine_rb_remove(&device->sampler_states, &state->desc);
        ID3D10Device1_Release(state->device);
        wined3d_private_store_cleanup(&state->private_store);
        wined3d_mutex_unlock();
        HeapFree(GetProcessHeap(), 0, state);
    }

    return refcount;
}

/* ID3D10DeviceChild methods */

static void STDMETHODCALLTYPE d3d10_sampler_state_GetDevice(ID3D10SamplerState *iface, ID3D10Device **device)
{
    struct d3d_sampler_state *state = impl_from_ID3D10SamplerState(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (ID3D10Device *)state->device;
    ID3D10Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d10_sampler_state_GetPrivateData(ID3D10SamplerState *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_sampler_state *state = impl_from_ID3D10SamplerState(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_sampler_state_SetPrivateData(ID3D10SamplerState *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_sampler_state *state = impl_from_ID3D10SamplerState(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_sampler_state_SetPrivateDataInterface(ID3D10SamplerState *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_sampler_state *state = impl_from_ID3D10SamplerState(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&state->private_store, guid, data);
}

/* ID3D10SamplerState methods */

static void STDMETHODCALLTYPE d3d10_sampler_state_GetDesc(ID3D10SamplerState *iface,
        D3D10_SAMPLER_DESC *desc)
{
    struct d3d_sampler_state *state = impl_from_ID3D10SamplerState(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    *desc = state->desc;
}

static const struct ID3D10SamplerStateVtbl d3d10_sampler_state_vtbl =
{
    /* IUnknown methods */
    d3d10_sampler_state_QueryInterface,
    d3d10_sampler_state_AddRef,
    d3d10_sampler_state_Release,
    /* ID3D10DeviceChild methods */
    d3d10_sampler_state_GetDevice,
    d3d10_sampler_state_GetPrivateData,
    d3d10_sampler_state_SetPrivateData,
    d3d10_sampler_state_SetPrivateDataInterface,
    /* ID3D10SamplerState methods */
    d3d10_sampler_state_GetDesc,
};

static enum wined3d_texture_address wined3d_texture_address_from_d3d10core(enum D3D10_TEXTURE_ADDRESS_MODE t)
{
    return (enum wined3d_texture_address)t;
}

static enum wined3d_texture_filter_type wined3d_texture_filter_mip_from_d3d10core(enum D3D10_FILTER f)
{
    if (f & D3D10_FILTER_MIP_MASK)
        return WINED3D_TEXF_LINEAR;
    return WINED3D_TEXF_POINT;
}

static enum wined3d_texture_filter_type wined3d_texture_filter_mag_from_d3d10core(enum D3D10_FILTER f)
{
    if (f & D3D10_FILTER_MAG_MASK)
        return WINED3D_TEXF_LINEAR;
    return WINED3D_TEXF_POINT;
}

static enum wined3d_texture_filter_type wined3d_texture_filter_min_from_d3d10core(enum D3D10_FILTER f)
{
    if (f & D3D10_FILTER_MIN_MASK)
        return WINED3D_TEXF_LINEAR;
    return WINED3D_TEXF_POINT;
}

static BOOL wined3d_texture_compare_from_d3d10core(enum D3D10_FILTER f)
{
    return f & D3D10_FILTER_COMPARE_MASK;
}

static enum wined3d_cmp_func wined3d_cmp_func_from_d3d10core(D3D10_COMPARISON_FUNC f)
{
    return (enum wined3d_cmp_func)f;
}

HRESULT d3d_sampler_state_init(struct d3d_sampler_state *state, struct d3d_device *device,
        const D3D10_SAMPLER_DESC *desc)
{
    struct wined3d_sampler_desc wined3d_desc;
    HRESULT hr;

    state->ID3D10SamplerState_iface.lpVtbl = &d3d10_sampler_state_vtbl;
    state->refcount = 1;
    wined3d_mutex_lock();
    wined3d_private_store_init(&state->private_store);
    state->desc = *desc;

    wined3d_desc.address_u = wined3d_texture_address_from_d3d10core(desc->AddressU);
    wined3d_desc.address_v = wined3d_texture_address_from_d3d10core(desc->AddressV);
    wined3d_desc.address_w = wined3d_texture_address_from_d3d10core(desc->AddressW);
    memcpy(wined3d_desc.border_color, desc->BorderColor, sizeof(wined3d_desc.border_color));
    wined3d_desc.mag_filter = wined3d_texture_filter_mag_from_d3d10core(desc->Filter);
    wined3d_desc.min_filter = wined3d_texture_filter_min_from_d3d10core(desc->Filter);
    wined3d_desc.mip_filter = wined3d_texture_filter_mip_from_d3d10core(desc->Filter);
    wined3d_desc.lod_bias = desc->MipLODBias;
    wined3d_desc.min_lod = desc->MinLOD;
    wined3d_desc.max_lod = desc->MaxLOD;
    wined3d_desc.max_anisotropy = desc->Filter & D3D10_FILTER_ANISO_MASK ? desc->MaxAnisotropy : 1;
    wined3d_desc.compare = wined3d_texture_compare_from_d3d10core(desc->Filter);
    wined3d_desc.comparison_func = wined3d_cmp_func_from_d3d10core(desc->ComparisonFunc);
    wined3d_desc.srgb_decode = FALSE;

    if (FAILED(hr = wined3d_sampler_create(device->wined3d_device, &wined3d_desc, state, &state->wined3d_sampler)))
    {
        WARN("Failed to create wined3d sampler, hr %#x.\n", hr);
        wined3d_private_store_cleanup(&state->private_store);
        wined3d_mutex_unlock();
        return hr;
    }

    if (wine_rb_put(&device->sampler_states, desc, &state->entry) == -1)
    {
        ERR("Failed to insert sampler state entry.\n");
        wined3d_sampler_decref(state->wined3d_sampler);
        wined3d_private_store_cleanup(&state->private_store);
        wined3d_mutex_unlock();
        return E_FAIL;
    }
    wined3d_mutex_unlock();

    state->device = &device->ID3D10Device1_iface;
    ID3D10Device1_AddRef(state->device);

    return S_OK;
}

struct d3d_sampler_state *unsafe_impl_from_ID3D10SamplerState(ID3D10SamplerState *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d10_sampler_state_vtbl);

    return impl_from_ID3D10SamplerState(iface);
}
