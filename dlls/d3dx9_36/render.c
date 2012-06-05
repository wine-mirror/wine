/*
 * Copyright (C) 2012 Józef Kucia
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

#include "wine/debug.h"
#include "d3dx9_36_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

struct render_to_surface
{
    ID3DXRenderToSurface ID3DXRenderToSurface_iface;
    LONG ref;

    IDirect3DDevice9 *device;
    D3DXRTS_DESC desc;
};

static inline struct render_to_surface *impl_from_ID3DXRenderToSurface(ID3DXRenderToSurface *iface)
{
    return CONTAINING_RECORD(iface, struct render_to_surface, ID3DXRenderToSurface_iface);
}

static HRESULT WINAPI D3DXRenderToSurface_QueryInterface(ID3DXRenderToSurface *iface,
                                                         REFIID riid,
                                                         void **out)
{
    TRACE("iface %p, riid %s, out %p\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_ID3DXRenderToSurface)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI D3DXRenderToSurface_AddRef(ID3DXRenderToSurface *iface)
{
    struct render_to_surface *render = impl_from_ID3DXRenderToSurface(iface);
    ULONG ref = InterlockedIncrement(&render->ref);

    TRACE("%p increasing refcount to %u\n", iface, ref);

    return ref;
}

static ULONG WINAPI D3DXRenderToSurface_Release(ID3DXRenderToSurface *iface)
{
    struct render_to_surface *render = impl_from_ID3DXRenderToSurface(iface);
    ULONG ref = InterlockedDecrement(&render->ref);

    TRACE("%p decreasing refcount to %u\n", iface, ref);

    if (!ref)
    {
        IDirect3DDevice9_Release(render->device);
        HeapFree(GetProcessHeap(), 0, render);
    }

    return ref;
}

static HRESULT WINAPI D3DXRenderToSurface_GetDevice(ID3DXRenderToSurface *iface,
                                                    IDirect3DDevice9 **device)
{
    FIXME("(%p)->(%p): stub\n", iface, device);
    return E_NOTIMPL;
}

static HRESULT WINAPI D3DXRenderToSurface_GetDesc(ID3DXRenderToSurface *iface,
                                                  D3DXRTS_DESC *desc)
{
    struct render_to_surface *render = impl_from_ID3DXRenderToSurface(iface);

    TRACE("(%p)->(%p)\n", iface, desc);

    if (!desc) return D3DERR_INVALIDCALL;

    *desc = render->desc;
    return D3D_OK;
}

static HRESULT WINAPI D3DXRenderToSurface_BeginScene(ID3DXRenderToSurface *iface,
                                                     IDirect3DSurface9 *surface,
                                                     const D3DVIEWPORT9 *viewport)
{
    FIXME("(%p)->(%p, %p): stub\n", iface, surface, viewport);
    return E_NOTIMPL;
}

static HRESULT WINAPI D3DXRenderToSurface_EndScene(ID3DXRenderToSurface *iface,
                                                   DWORD mip_filter)
{
    FIXME("(%p)->(%#x): stub\n", iface, mip_filter);
    return E_NOTIMPL;
}

static HRESULT WINAPI D3DXRenderToSurface_OnLostDevice(ID3DXRenderToSurface *iface)
{
    FIXME("(%p)->(): stub\n", iface);
    return D3D_OK;
}

static HRESULT WINAPI D3DXRenderToSurface_OnResetDevice(ID3DXRenderToSurface *iface)
{
    FIXME("(%p)->(): stub\n", iface);
    return D3D_OK;
}

static const ID3DXRenderToSurfaceVtbl d3dx_render_to_surface_vtbl =
{
    /* IUnknown methods */
    D3DXRenderToSurface_QueryInterface,
    D3DXRenderToSurface_AddRef,
    D3DXRenderToSurface_Release,
    /* ID3DXRenderToSurface methods */
    D3DXRenderToSurface_GetDevice,
    D3DXRenderToSurface_GetDesc,
    D3DXRenderToSurface_BeginScene,
    D3DXRenderToSurface_EndScene,
    D3DXRenderToSurface_OnLostDevice,
    D3DXRenderToSurface_OnResetDevice
};

HRESULT WINAPI D3DXCreateRenderToSurface(IDirect3DDevice9 *device,
                                         UINT width,
                                         UINT height,
                                         D3DFORMAT format,
                                         BOOL depth_stencil,
                                         D3DFORMAT depth_stencil_format,
                                         ID3DXRenderToSurface **out)
{
    struct render_to_surface *render;

    TRACE("(%p, %u, %u, %#x, %d, %#x, %p)\n", device, width, height, format,
            depth_stencil, depth_stencil_format, out);

    if (!device || !out) return D3DERR_INVALIDCALL;

    render = HeapAlloc(GetProcessHeap(), 0, sizeof(struct render_to_surface));
    if (!render) return E_OUTOFMEMORY;

    render->ID3DXRenderToSurface_iface.lpVtbl = &d3dx_render_to_surface_vtbl;
    render->ref = 1;

    IDirect3DDevice9_AddRef(device);
    render->device = device;

    render->desc.Width = width;
    render->desc.Height = height;
    render->desc.Format = format;
    render->desc.DepthStencil = depth_stencil;
    render->desc.DepthStencilFormat = depth_stencil_format;

    *out = &render->ID3DXRenderToSurface_iface;
    return D3D_OK;
}
