/*
 * Copyright 2014 Henri Verbeet for CodeWeavers
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
#include "wine/port.h"

#include "d2d1_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d2d);

static inline struct d2d_bitmap *impl_from_ID2D1Bitmap(ID2D1Bitmap *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_bitmap, ID2D1Bitmap_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_bitmap_QueryInterface(ID2D1Bitmap *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1Bitmap)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1Bitmap_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_bitmap_AddRef(ID2D1Bitmap *iface)
{
    struct d2d_bitmap *bitmap = impl_from_ID2D1Bitmap(iface);
    ULONG refcount = InterlockedIncrement(&bitmap->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_bitmap_Release(ID2D1Bitmap *iface)
{
    struct d2d_bitmap *bitmap = impl_from_ID2D1Bitmap(iface);
    ULONG refcount = InterlockedDecrement(&bitmap->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        ID3D10ShaderResourceView_Release(bitmap->view);
        HeapFree(GetProcessHeap(), 0, bitmap);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d2d_bitmap_GetFactory(ID2D1Bitmap *iface, ID2D1Factory **factory)
{
    FIXME("iface %p, factory %p stub!\n", iface, factory);

    *factory = NULL;
}

static D2D1_SIZE_F * STDMETHODCALLTYPE d2d_bitmap_GetSize(ID2D1Bitmap *iface, D2D1_SIZE_F *size)
{
    struct d2d_bitmap *bitmap = impl_from_ID2D1Bitmap(iface);

    TRACE("iface %p, size %p.\n", iface, size);

    size->width = bitmap->pixel_size.width / (bitmap->dpi_x / 96.0f);
    size->height = bitmap->pixel_size.height / (bitmap->dpi_y / 96.0f);
    return size;
}

static D2D1_SIZE_U * STDMETHODCALLTYPE d2d_bitmap_GetPixelSize(ID2D1Bitmap *iface, D2D1_SIZE_U *pixel_size)
{
    struct d2d_bitmap *bitmap = impl_from_ID2D1Bitmap(iface);

    TRACE("iface %p, pixel_size %p.\n", iface, pixel_size);

    *pixel_size = bitmap->pixel_size;
    return pixel_size;
}

static D2D1_PIXEL_FORMAT * STDMETHODCALLTYPE d2d_bitmap_GetPixelFormat(ID2D1Bitmap *iface, D2D1_PIXEL_FORMAT *format)
{
    FIXME("iface %p stub!\n", iface);

    format->format = DXGI_FORMAT_UNKNOWN;
    format->alphaMode = D2D1_ALPHA_MODE_UNKNOWN;
    return format;
}

static void STDMETHODCALLTYPE d2d_bitmap_GetDpi(ID2D1Bitmap *iface, float *dpi_x, float *dpi_y)
{
    struct d2d_bitmap *bitmap = impl_from_ID2D1Bitmap(iface);

    TRACE("iface %p, dpi_x %p, dpi_y %p.\n", iface, dpi_x, dpi_y);

    *dpi_x = bitmap->dpi_x;
    *dpi_y = bitmap->dpi_y;
}

static HRESULT STDMETHODCALLTYPE d2d_bitmap_CopyFromBitmap(ID2D1Bitmap *iface,
        const D2D1_POINT_2U *dst_point, ID2D1Bitmap *bitmap, const D2D1_RECT_U *src_rect)
{
    FIXME("iface %p, dst_point %p, bitmap %p, src_rect %p stub!\n", iface, dst_point, bitmap, src_rect);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_bitmap_CopyFromRenderTarget(ID2D1Bitmap *iface,
        const D2D1_POINT_2U *dst_point, ID2D1RenderTarget *render_target, const D2D1_RECT_U *src_rect)
{
    FIXME("iface %p, dst_point %p, render_target %p, src_rect %p stub!\n", iface, dst_point, render_target, src_rect);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_bitmap_CopyFromMemory(ID2D1Bitmap *iface,
        const D2D1_RECT_U *dst_rect, const void *src_data, UINT32 pitch)
{
    FIXME("iface %p, dst_rect %p, src_data %p, pitch %u stub!\n", iface, dst_rect, src_data, pitch);

    return E_NOTIMPL;
}

static const struct ID2D1BitmapVtbl d2d_bitmap_vtbl =
{
    d2d_bitmap_QueryInterface,
    d2d_bitmap_AddRef,
    d2d_bitmap_Release,
    d2d_bitmap_GetFactory,
    d2d_bitmap_GetSize,
    d2d_bitmap_GetPixelSize,
    d2d_bitmap_GetPixelFormat,
    d2d_bitmap_GetDpi,
    d2d_bitmap_CopyFromBitmap,
    d2d_bitmap_CopyFromRenderTarget,
    d2d_bitmap_CopyFromMemory,
};

HRESULT d2d_bitmap_init(struct d2d_bitmap *bitmap, struct d2d_d3d_render_target *render_target,
        D2D1_SIZE_U size, const void *src_data, UINT32 pitch, const D2D1_BITMAP_PROPERTIES *desc)
{
    D3D10_SUBRESOURCE_DATA resource_data;
    D3D10_TEXTURE2D_DESC texture_desc;
    ID3D10Texture2D *texture;
    HRESULT hr;

    FIXME("Ignoring bitmap properties.\n");

    bitmap->ID2D1Bitmap_iface.lpVtbl = &d2d_bitmap_vtbl;
    bitmap->refcount = 1;

    texture_desc.Width = size.width;
    texture_desc.Height = size.height;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = desc->pixelFormat.format;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    resource_data.pSysMem = src_data;
    resource_data.SysMemPitch = pitch;

    if (FAILED(hr = ID3D10Device_CreateTexture2D(render_target->device, &texture_desc, &resource_data, &texture)))
    {
        ERR("Failed to create texture, hr %#x.\n", hr);
        return hr;
    }

    hr = ID3D10Device_CreateShaderResourceView(render_target->device, (ID3D10Resource *)texture, NULL, &bitmap->view);
    ID3D10Texture2D_Release(texture);
    if (FAILED(hr))
    {
        ERR("Failed to create view, hr %#x.\n", hr);
        return hr;
    }

    bitmap->pixel_size = size;
    bitmap->dpi_x = desc->dpiX;
    bitmap->dpi_y = desc->dpiY;

    if (bitmap->dpi_x == 0.0f && bitmap->dpi_y == 0.0f)
    {
        bitmap->dpi_x = 96.0f;
        bitmap->dpi_y = 96.0f;
    }

    return S_OK;
}

struct d2d_bitmap *unsafe_impl_from_ID2D1Bitmap(ID2D1Bitmap *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d2d_bitmap_vtbl);
    return CONTAINING_RECORD(iface, struct d2d_bitmap, ID2D1Bitmap_iface);
}
