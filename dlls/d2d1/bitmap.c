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
        HeapFree(GetProcessHeap(), 0, bitmap);

    return refcount;
}

static void STDMETHODCALLTYPE d2d_bitmap_GetFactory(ID2D1Bitmap *iface, ID2D1Factory **factory)
{
    FIXME("iface %p, factory %p stub!\n", iface, factory);

    *factory = NULL;
}

static D2D1_SIZE_F * STDMETHODCALLTYPE d2d_bitmap_GetSize(ID2D1Bitmap *iface, D2D1_SIZE_F *size)
{
    FIXME("iface %p, size %p stub!\n", iface, size);

    size->width = 0.0f;
    size->height = 0.0f;
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
    FIXME("iface %p, dpi_x %p, dpi_y %p stub!\n", iface, dpi_x, dpi_y);

    *dpi_x = 96.0f;
    *dpi_y = 96.0f;
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

void d2d_bitmap_init(struct d2d_bitmap *bitmap, D2D1_SIZE_U size, const void *src_data,
        UINT32 pitch, const D2D1_BITMAP_PROPERTIES *desc)
{
    FIXME("Ignoring bitmap properties.\n");

    bitmap->ID2D1Bitmap_iface.lpVtbl = &d2d_bitmap_vtbl;
    bitmap->refcount = 1;

    bitmap->pixel_size = size;
}
