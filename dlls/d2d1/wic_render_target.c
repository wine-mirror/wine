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
#include "initguid.h"
#include "wincodec.h"

WINE_DEFAULT_DEBUG_CHANNEL(d2d);

static void sync_bitmap(struct d2d_wic_render_target *render_target)
{
    D3D10_MAPPED_TEXTURE2D mapped_texture;
    ID3D10Resource *src_resource;
    IWICBitmapLock *bitmap_lock;
    UINT dst_size, dst_pitch;
    ID3D10Device *device;
    WICRect dst_rect;
    BYTE *src, *dst;
    unsigned int i;
    HRESULT hr;

    if (FAILED(hr = IDXGISurface_QueryInterface(render_target->dxgi_surface,
            &IID_ID3D10Resource, (void **)&src_resource)))
    {
        ERR("Failed to get source resource interface, hr %#x.\n", hr);
        return;
    }

    ID3D10Texture2D_GetDevice(render_target->readback_texture, &device);
    ID3D10Device_CopyResource(device, (ID3D10Resource *)render_target->readback_texture, src_resource);
    ID3D10Device_Release(device);
    ID3D10Resource_Release(src_resource);

    dst_rect.X = 0;
    dst_rect.Y = 0;
    dst_rect.Width = render_target->width;
    dst_rect.Height = render_target->height;
    if (FAILED(hr = IWICBitmap_Lock(render_target->bitmap, &dst_rect, WICBitmapLockWrite, &bitmap_lock)))
    {
        ERR("Failed to lock destination bitmap, hr %#x.\n", hr);
        return;
    }

    if (FAILED(hr = IWICBitmapLock_GetDataPointer(bitmap_lock, &dst_size, &dst)))
    {
        ERR("Failed to get data pointer, hr %#x.\n", hr);
        IWICBitmapLock_Release(bitmap_lock);
        return;
    }

    if (FAILED(hr = IWICBitmapLock_GetStride(bitmap_lock, &dst_pitch)))
    {
        ERR("Failed to get stride, hr %#x.\n", hr);
        IWICBitmapLock_Release(bitmap_lock);
        return;
    }

    if (FAILED(hr = ID3D10Texture2D_Map(render_target->readback_texture, 0, D3D10_MAP_READ, 0, &mapped_texture)))
    {
        ERR("Failed to map readback texture, hr %#x.\n", hr);
        IWICBitmapLock_Release(bitmap_lock);
        return;
    }

    src = mapped_texture.pData;

    for (i = 0; i < render_target->height; ++i)
    {
        memcpy(dst, src, render_target->bpp * render_target->width);
        src += mapped_texture.RowPitch;
        dst += dst_pitch;
    }

    ID3D10Texture2D_Unmap(render_target->readback_texture, 0);
    IWICBitmapLock_Release(bitmap_lock);
}

static inline struct d2d_wic_render_target *impl_from_ID2D1RenderTarget(ID2D1RenderTarget *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_wic_render_target, ID2D1RenderTarget_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_wic_render_target_QueryInterface(ID2D1RenderTarget *iface, REFIID iid, void **out)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1RenderTarget)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1RenderTarget_AddRef(iface);
        *out = iface;
        return S_OK;
    }
    else if (IsEqualGUID(iid, &IID_ID2D1GdiInteropRenderTarget))
        return ID2D1RenderTarget_QueryInterface(render_target->dxgi_target, iid, out);

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_wic_render_target_AddRef(ID2D1RenderTarget *iface)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);
    ULONG refcount = InterlockedIncrement(&render_target->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_wic_render_target_Release(ID2D1RenderTarget *iface)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);
    ULONG refcount = InterlockedDecrement(&render_target->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        IWICBitmap_Release(render_target->bitmap);
        ID3D10Texture2D_Release(render_target->readback_texture);
        ID2D1RenderTarget_Release(render_target->dxgi_target);
        IDXGISurface_Release(render_target->dxgi_surface);
        heap_free(render_target);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d2d_wic_render_target_GetFactory(ID2D1RenderTarget *iface, ID2D1Factory **factory)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    ID2D1RenderTarget_GetFactory(render_target->dxgi_target, factory);
}

static HRESULT STDMETHODCALLTYPE d2d_wic_render_target_CreateBitmap(ID2D1RenderTarget *iface,
        D2D1_SIZE_U size, const void *src_data, UINT32 pitch, const D2D1_BITMAP_PROPERTIES *desc, ID2D1Bitmap **bitmap)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, size {%u, %u}, src_data %p, pitch %u, desc %p, bitmap %p.\n",
            iface, size.width, size.height, src_data, pitch, desc, bitmap);

    return ID2D1RenderTarget_CreateBitmap(render_target->dxgi_target, size, src_data, pitch, desc, bitmap);
}

static HRESULT STDMETHODCALLTYPE d2d_wic_render_target_CreateBitmapFromWicBitmap(ID2D1RenderTarget *iface,
        IWICBitmapSource *bitmap_source, const D2D1_BITMAP_PROPERTIES *desc, ID2D1Bitmap **bitmap)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, bitmap_source %p, desc %p, bitmap %p.\n",
            iface, bitmap_source, desc, bitmap);

    return ID2D1RenderTarget_CreateBitmapFromWicBitmap(render_target->dxgi_target, bitmap_source, desc, bitmap);
}

static HRESULT STDMETHODCALLTYPE d2d_wic_render_target_CreateSharedBitmap(ID2D1RenderTarget *iface,
        REFIID iid, void *data, const D2D1_BITMAP_PROPERTIES *desc, ID2D1Bitmap **bitmap)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, iid %s, data %p, desc %p, bitmap %p.\n",
            iface, debugstr_guid(iid), data, desc, bitmap);

    return ID2D1RenderTarget_CreateSharedBitmap(render_target->dxgi_target, iid, data, desc, bitmap);
}

static HRESULT STDMETHODCALLTYPE d2d_wic_render_target_CreateBitmapBrush(ID2D1RenderTarget *iface,
        ID2D1Bitmap *bitmap, const D2D1_BITMAP_BRUSH_PROPERTIES *bitmap_brush_desc,
        const D2D1_BRUSH_PROPERTIES *brush_desc, ID2D1BitmapBrush **brush)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, bitmap %p, bitmap_brush_desc %p, brush_desc %p, brush %p.\n",
            iface, bitmap, bitmap_brush_desc, brush_desc, brush);

    return ID2D1RenderTarget_CreateBitmapBrush(render_target->dxgi_target,
            bitmap, bitmap_brush_desc, brush_desc, brush);
}

static HRESULT STDMETHODCALLTYPE d2d_wic_render_target_CreateSolidColorBrush(ID2D1RenderTarget *iface,
        const D2D1_COLOR_F *color, const D2D1_BRUSH_PROPERTIES *desc, ID2D1SolidColorBrush **brush)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, color %p, desc %p, brush %p.\n", iface, color, desc, brush);

    return ID2D1RenderTarget_CreateSolidColorBrush(render_target->dxgi_target, color, desc, brush);
}

static HRESULT STDMETHODCALLTYPE d2d_wic_render_target_CreateGradientStopCollection(ID2D1RenderTarget *iface,
        const D2D1_GRADIENT_STOP *stops, UINT32 stop_count, D2D1_GAMMA gamma, D2D1_EXTEND_MODE extend_mode,
        ID2D1GradientStopCollection **gradient)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, stops %p, stop_count %u, gamma %#x, extend_mode %#x, gradient %p.\n",
            iface, stops, stop_count, gamma, extend_mode, gradient);

    return ID2D1RenderTarget_CreateGradientStopCollection(render_target->dxgi_target,
            stops, stop_count, gamma, extend_mode, gradient);
}

static HRESULT STDMETHODCALLTYPE d2d_wic_render_target_CreateLinearGradientBrush(ID2D1RenderTarget *iface,
        const D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES *gradient_brush_desc, const D2D1_BRUSH_PROPERTIES *brush_desc,
        ID2D1GradientStopCollection *gradient, ID2D1LinearGradientBrush **brush)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, gradient_brush_desc %p, brush_desc %p, gradient %p, brush %p.\n",
            iface, gradient_brush_desc, brush_desc, gradient, brush);

    return ID2D1RenderTarget_CreateLinearGradientBrush(render_target->dxgi_target,
            gradient_brush_desc, brush_desc, gradient, brush);
}

static HRESULT STDMETHODCALLTYPE d2d_wic_render_target_CreateRadialGradientBrush(ID2D1RenderTarget *iface,
        const D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES *gradient_brush_desc, const D2D1_BRUSH_PROPERTIES *brush_desc,
        ID2D1GradientStopCollection *gradient, ID2D1RadialGradientBrush **brush)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, gradient_brush_desc %p, brush_desc %p, gradient %p, brush %p.\n",
            iface, gradient_brush_desc, brush_desc, gradient, brush);

    return ID2D1RenderTarget_CreateRadialGradientBrush(render_target->dxgi_target,
            gradient_brush_desc, brush_desc, gradient, brush);
}

static HRESULT STDMETHODCALLTYPE d2d_wic_render_target_CreateCompatibleRenderTarget(ID2D1RenderTarget *iface,
        const D2D1_SIZE_F *size, const D2D1_SIZE_U *pixel_size, const D2D1_PIXEL_FORMAT *format,
        D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS options, ID2D1BitmapRenderTarget **render_target)
{
    struct d2d_wic_render_target *rt = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, size %p, pixel_size %p, format %p, options %#x, render_target %p,\n",
            iface, size, pixel_size, format, options, render_target);

    return ID2D1RenderTarget_CreateCompatibleRenderTarget(rt->dxgi_target,
            size, pixel_size, format, options, render_target);
}

static HRESULT STDMETHODCALLTYPE d2d_wic_render_target_CreateLayer(ID2D1RenderTarget *iface,
        const D2D1_SIZE_F *size, ID2D1Layer **layer)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, size %p, layer %p.\n", iface, size, layer);

    return ID2D1RenderTarget_CreateLayer(render_target->dxgi_target, size, layer);
}

static HRESULT STDMETHODCALLTYPE d2d_wic_render_target_CreateMesh(ID2D1RenderTarget *iface, ID2D1Mesh **mesh)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, mesh %p.\n", iface, mesh);

    return ID2D1RenderTarget_CreateMesh(render_target->dxgi_target, mesh);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_DrawLine(ID2D1RenderTarget *iface,
        D2D1_POINT_2F p0, D2D1_POINT_2F p1, ID2D1Brush *brush, float stroke_width, ID2D1StrokeStyle *stroke_style)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, p0 {%.8e, %.8e}, p1 {%.8e, %.8e}, brush %p, stroke_width %.8e, stroke_style %p.\n",
            iface, p0.x, p0.y, p1.x, p1.y, brush, stroke_width, stroke_style);

    ID2D1RenderTarget_DrawLine(render_target->dxgi_target, p0, p1, brush, stroke_width, stroke_style);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_DrawRectangle(ID2D1RenderTarget *iface,
        const D2D1_RECT_F *rect, ID2D1Brush *brush, float stroke_width, ID2D1StrokeStyle *stroke_style)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, rect %s, brush %p, stroke_width %.8e, stroke_style %p.\n",
            iface, debug_d2d_rect_f(rect), brush, stroke_width, stroke_style);

    ID2D1RenderTarget_DrawRectangle(render_target->dxgi_target, rect, brush, stroke_width, stroke_style);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_FillRectangle(ID2D1RenderTarget *iface,
        const D2D1_RECT_F *rect, ID2D1Brush *brush)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, rect %s, brush %p.\n", iface, debug_d2d_rect_f(rect), brush);

    ID2D1RenderTarget_FillRectangle(render_target->dxgi_target, rect, brush);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_DrawRoundedRectangle(ID2D1RenderTarget *iface,
        const D2D1_ROUNDED_RECT *rect, ID2D1Brush *brush, float stroke_width, ID2D1StrokeStyle *stroke_style)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, rect %p, brush %p, stroke_width %.8e, stroke_style %p.\n",
            iface, rect, brush, stroke_width, stroke_style);

    ID2D1RenderTarget_DrawRoundedRectangle(render_target->dxgi_target, rect, brush, stroke_width, stroke_style);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_FillRoundedRectangle(ID2D1RenderTarget *iface,
        const D2D1_ROUNDED_RECT *rect, ID2D1Brush *brush)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, rect %p, brush %p.\n", iface, rect, brush);

    ID2D1RenderTarget_FillRoundedRectangle(render_target->dxgi_target, rect, brush);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_DrawEllipse(ID2D1RenderTarget *iface,
        const D2D1_ELLIPSE *ellipse, ID2D1Brush *brush, float stroke_width, ID2D1StrokeStyle *stroke_style)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, ellipse %p, brush %p, stroke_width %.8e, stroke_style %p.\n",
            iface, ellipse, brush, stroke_width, stroke_style);

    ID2D1RenderTarget_DrawEllipse(render_target->dxgi_target, ellipse, brush, stroke_width, stroke_style);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_FillEllipse(ID2D1RenderTarget *iface,
        const D2D1_ELLIPSE *ellipse, ID2D1Brush *brush)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, ellipse %p, brush %p.\n", iface, ellipse, brush);

    ID2D1RenderTarget_FillEllipse(render_target->dxgi_target, ellipse, brush);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_DrawGeometry(ID2D1RenderTarget *iface,
        ID2D1Geometry *geometry, ID2D1Brush *brush, float stroke_width, ID2D1StrokeStyle *stroke_style)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, geometry %p, brush %p, stroke_width %.8e, stroke_style %p.\n",
            iface, geometry, brush, stroke_width, stroke_style);

    ID2D1RenderTarget_DrawGeometry(render_target->dxgi_target, geometry, brush, stroke_width, stroke_style);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_FillGeometry(ID2D1RenderTarget *iface,
        ID2D1Geometry *geometry, ID2D1Brush *brush, ID2D1Brush *opacity_brush)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, geometry %p, brush %p, opacity_brush %p.\n", iface, geometry, brush, opacity_brush);

    ID2D1RenderTarget_FillGeometry(render_target->dxgi_target, geometry, brush, opacity_brush);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_FillMesh(ID2D1RenderTarget *iface,
        ID2D1Mesh *mesh, ID2D1Brush *brush)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, mesh %p, brush %p.\n", iface, mesh, brush);

    ID2D1RenderTarget_FillMesh(render_target->dxgi_target, mesh, brush);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_FillOpacityMask(ID2D1RenderTarget *iface,
        ID2D1Bitmap *mask, ID2D1Brush *brush, D2D1_OPACITY_MASK_CONTENT content,
        const D2D1_RECT_F *dst_rect, const D2D1_RECT_F *src_rect)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, mask %p, brush %p, content %#x, dst_rect %s, src_rect %s.\n",
            iface, mask, brush, content, debug_d2d_rect_f(dst_rect), debug_d2d_rect_f(src_rect));

    ID2D1RenderTarget_FillOpacityMask(render_target->dxgi_target,
            mask, brush, content, dst_rect, src_rect);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_DrawBitmap(ID2D1RenderTarget *iface,
        ID2D1Bitmap *bitmap, const D2D1_RECT_F *dst_rect, float opacity,
        D2D1_BITMAP_INTERPOLATION_MODE interpolation_mode, const D2D1_RECT_F *src_rect)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, bitmap %p, dst_rect %s, opacity %.8e, interpolation_mode %#x, src_rect %s.\n",
            iface, bitmap, debug_d2d_rect_f(dst_rect), opacity, interpolation_mode, debug_d2d_rect_f(src_rect));

    ID2D1RenderTarget_DrawBitmap(render_target->dxgi_target,
            bitmap, dst_rect, opacity, interpolation_mode, src_rect);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_DrawText(ID2D1RenderTarget *iface,
        const WCHAR *string, UINT32 string_len, IDWriteTextFormat *text_format, const D2D1_RECT_F *layout_rect,
        ID2D1Brush *brush, D2D1_DRAW_TEXT_OPTIONS options, DWRITE_MEASURING_MODE measuring_mode)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, string %s, string_len %u, text_format %p, layout_rect %s, "
            "brush %p, options %#x, measuring_mode %#x.\n",
            iface, debugstr_wn(string, string_len), string_len, text_format, debug_d2d_rect_f(layout_rect),
            brush, options, measuring_mode);

    ID2D1RenderTarget_DrawText(render_target->dxgi_target, string, string_len,
            text_format, layout_rect, brush, options, measuring_mode);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_DrawTextLayout(ID2D1RenderTarget *iface,
        D2D1_POINT_2F origin, IDWriteTextLayout *layout, ID2D1Brush *brush, D2D1_DRAW_TEXT_OPTIONS options)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, origin {%.8e, %.8e}, layout %p, brush %p, options %#x.\n",
            iface, origin.x, origin.y, layout, brush, options);

    ID2D1RenderTarget_DrawTextLayout(render_target->dxgi_target, origin, layout, brush, options);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_DrawGlyphRun(ID2D1RenderTarget *iface,
        D2D1_POINT_2F baseline_origin, const DWRITE_GLYPH_RUN *glyph_run, ID2D1Brush *brush,
        DWRITE_MEASURING_MODE measuring_mode)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, baseline_origin {%.8e, %.8e}, glyph_run %p, brush %p, measuring_mode %#x.\n",
            iface, baseline_origin.x, baseline_origin.y, glyph_run, brush, measuring_mode);

    ID2D1RenderTarget_DrawGlyphRun(render_target->dxgi_target,
            baseline_origin, glyph_run, brush, measuring_mode);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_SetTransform(ID2D1RenderTarget *iface,
        const D2D1_MATRIX_3X2_F *transform)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, transform %p.\n", iface, transform);

    ID2D1RenderTarget_SetTransform(render_target->dxgi_target, transform);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_GetTransform(ID2D1RenderTarget *iface,
        D2D1_MATRIX_3X2_F *transform)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, transform %p.\n", iface, transform);

    ID2D1RenderTarget_GetTransform(render_target->dxgi_target, transform);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_SetAntialiasMode(ID2D1RenderTarget *iface,
        D2D1_ANTIALIAS_MODE antialias_mode)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, antialias_mode %#x.\n", iface, antialias_mode);

    ID2D1RenderTarget_SetAntialiasMode(render_target->dxgi_target, antialias_mode);
}

static D2D1_ANTIALIAS_MODE STDMETHODCALLTYPE d2d_wic_render_target_GetAntialiasMode(ID2D1RenderTarget *iface)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p.\n", iface);

    return ID2D1RenderTarget_GetAntialiasMode(render_target->dxgi_target);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_SetTextAntialiasMode(ID2D1RenderTarget *iface,
        D2D1_TEXT_ANTIALIAS_MODE antialias_mode)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, antialias_mode %#x.\n", iface, antialias_mode);

    ID2D1RenderTarget_SetTextAntialiasMode(render_target->dxgi_target, antialias_mode);
}

static D2D1_TEXT_ANTIALIAS_MODE STDMETHODCALLTYPE d2d_wic_render_target_GetTextAntialiasMode(ID2D1RenderTarget *iface)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p.\n", iface);

    return ID2D1RenderTarget_GetTextAntialiasMode(render_target->dxgi_target);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_SetTextRenderingParams(ID2D1RenderTarget *iface,
        IDWriteRenderingParams *text_rendering_params)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, text_rendering_params %p.\n", iface, text_rendering_params);

    ID2D1RenderTarget_SetTextRenderingParams(render_target->dxgi_target, text_rendering_params);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_GetTextRenderingParams(ID2D1RenderTarget *iface,
        IDWriteRenderingParams **text_rendering_params)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, text_rendering_params %p.\n", iface, text_rendering_params);

    ID2D1RenderTarget_GetTextRenderingParams(render_target->dxgi_target, text_rendering_params);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_SetTags(ID2D1RenderTarget *iface, D2D1_TAG tag1, D2D1_TAG tag2)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, tag1 %s, tag2 %s.\n", iface, wine_dbgstr_longlong(tag1), wine_dbgstr_longlong(tag2));

    ID2D1RenderTarget_SetTags(render_target->dxgi_target, tag1, tag2);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_GetTags(ID2D1RenderTarget *iface, D2D1_TAG *tag1, D2D1_TAG *tag2)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, tag1 %p, tag2 %p.\n", iface, tag1, tag2);

    ID2D1RenderTarget_GetTags(render_target->dxgi_target, tag1, tag2);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_PushLayer(ID2D1RenderTarget *iface,
        const D2D1_LAYER_PARAMETERS *layer_parameters, ID2D1Layer *layer)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, layer_parameters %p, layer %p.\n", iface, layer_parameters, layer);

    ID2D1RenderTarget_PushLayer(render_target->dxgi_target, layer_parameters, layer);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_PopLayer(ID2D1RenderTarget *iface)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p.\n", iface);

    ID2D1RenderTarget_PopLayer(render_target->dxgi_target);
}

static HRESULT STDMETHODCALLTYPE d2d_wic_render_target_Flush(ID2D1RenderTarget *iface, D2D1_TAG *tag1, D2D1_TAG *tag2)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);
    HRESULT hr;

    TRACE("iface %p, tag1 %p, tag2 %p.\n", iface, tag1, tag2);

    hr = ID2D1RenderTarget_Flush(render_target->dxgi_target, tag1, tag2);
    sync_bitmap(render_target);

    return hr;
}

static void STDMETHODCALLTYPE d2d_wic_render_target_SaveDrawingState(ID2D1RenderTarget *iface,
        ID2D1DrawingStateBlock *state_block)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, state_block %p.\n", iface, state_block);

    ID2D1RenderTarget_SaveDrawingState(render_target->dxgi_target, state_block);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_RestoreDrawingState(ID2D1RenderTarget *iface,
        ID2D1DrawingStateBlock *state_block)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, state_block %p.\n", iface, state_block);

    ID2D1RenderTarget_RestoreDrawingState(render_target->dxgi_target, state_block);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_PushAxisAlignedClip(ID2D1RenderTarget *iface,
        const D2D1_RECT_F *clip_rect, D2D1_ANTIALIAS_MODE antialias_mode)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, clip_rect %s, antialias_mode %#x.\n", iface, debug_d2d_rect_f(clip_rect), antialias_mode);

    ID2D1RenderTarget_PushAxisAlignedClip(render_target->dxgi_target, clip_rect, antialias_mode);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_PopAxisAlignedClip(ID2D1RenderTarget *iface)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p.\n", iface);

    ID2D1RenderTarget_PopAxisAlignedClip(render_target->dxgi_target);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_Clear(ID2D1RenderTarget *iface, const D2D1_COLOR_F *color)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, color %p.\n", iface, color);

    ID2D1RenderTarget_Clear(render_target->dxgi_target, color);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_BeginDraw(ID2D1RenderTarget *iface)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p.\n", iface);

    ID2D1RenderTarget_BeginDraw(render_target->dxgi_target);
}

static HRESULT STDMETHODCALLTYPE d2d_wic_render_target_EndDraw(ID2D1RenderTarget *iface,
        D2D1_TAG *tag1, D2D1_TAG *tag2)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);
    HRESULT hr;

    TRACE("iface %p, tag1 %p, tag2 %p.\n", iface, tag1, tag2);

    hr = ID2D1RenderTarget_EndDraw(render_target->dxgi_target, tag1, tag2);
    sync_bitmap(render_target);

    return hr;
}

static D2D1_PIXEL_FORMAT * STDMETHODCALLTYPE d2d_wic_render_target_GetPixelFormat(ID2D1RenderTarget *iface,
        D2D1_PIXEL_FORMAT *format)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, format %p.\n", iface, format);

    *format = ID2D1RenderTarget_GetPixelFormat(render_target->dxgi_target);
    return format;
}

static void STDMETHODCALLTYPE d2d_wic_render_target_SetDpi(ID2D1RenderTarget *iface, float dpi_x, float dpi_y)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, dpi_x %.8e, dpi_y %.8e.\n", iface, dpi_x, dpi_y);

    ID2D1RenderTarget_SetDpi(render_target->dxgi_target, dpi_x, dpi_y);
}

static void STDMETHODCALLTYPE d2d_wic_render_target_GetDpi(ID2D1RenderTarget *iface, float *dpi_x, float *dpi_y)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, dpi_x %p, dpi_y %p.\n", iface, dpi_x, dpi_y);

    ID2D1RenderTarget_GetDpi(render_target->dxgi_target, dpi_x, dpi_y);
}

static D2D1_SIZE_F * STDMETHODCALLTYPE d2d_wic_render_target_GetSize(ID2D1RenderTarget *iface, D2D1_SIZE_F *size)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, size %p.\n", iface, size);

    *size = ID2D1RenderTarget_GetSize(render_target->dxgi_target);
    return size;
}

static D2D1_SIZE_U * STDMETHODCALLTYPE d2d_wic_render_target_GetPixelSize(ID2D1RenderTarget *iface,
        D2D1_SIZE_U *pixel_size)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, pixel_size %p.\n", iface, pixel_size);

    *pixel_size = ID2D1RenderTarget_GetPixelSize(render_target->dxgi_target);
    return pixel_size;
}

static UINT32 STDMETHODCALLTYPE d2d_wic_render_target_GetMaximumBitmapSize(ID2D1RenderTarget *iface)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p.\n", iface);

    return ID2D1RenderTarget_GetMaximumBitmapSize(render_target->dxgi_target);
}

static BOOL STDMETHODCALLTYPE d2d_wic_render_target_IsSupported(ID2D1RenderTarget *iface,
        const D2D1_RENDER_TARGET_PROPERTIES *desc)
{
    struct d2d_wic_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    return ID2D1RenderTarget_IsSupported(render_target->dxgi_target, desc);
}

static const struct ID2D1RenderTargetVtbl d2d_wic_render_target_vtbl =
{
    d2d_wic_render_target_QueryInterface,
    d2d_wic_render_target_AddRef,
    d2d_wic_render_target_Release,
    d2d_wic_render_target_GetFactory,
    d2d_wic_render_target_CreateBitmap,
    d2d_wic_render_target_CreateBitmapFromWicBitmap,
    d2d_wic_render_target_CreateSharedBitmap,
    d2d_wic_render_target_CreateBitmapBrush,
    d2d_wic_render_target_CreateSolidColorBrush,
    d2d_wic_render_target_CreateGradientStopCollection,
    d2d_wic_render_target_CreateLinearGradientBrush,
    d2d_wic_render_target_CreateRadialGradientBrush,
    d2d_wic_render_target_CreateCompatibleRenderTarget,
    d2d_wic_render_target_CreateLayer,
    d2d_wic_render_target_CreateMesh,
    d2d_wic_render_target_DrawLine,
    d2d_wic_render_target_DrawRectangle,
    d2d_wic_render_target_FillRectangle,
    d2d_wic_render_target_DrawRoundedRectangle,
    d2d_wic_render_target_FillRoundedRectangle,
    d2d_wic_render_target_DrawEllipse,
    d2d_wic_render_target_FillEllipse,
    d2d_wic_render_target_DrawGeometry,
    d2d_wic_render_target_FillGeometry,
    d2d_wic_render_target_FillMesh,
    d2d_wic_render_target_FillOpacityMask,
    d2d_wic_render_target_DrawBitmap,
    d2d_wic_render_target_DrawText,
    d2d_wic_render_target_DrawTextLayout,
    d2d_wic_render_target_DrawGlyphRun,
    d2d_wic_render_target_SetTransform,
    d2d_wic_render_target_GetTransform,
    d2d_wic_render_target_SetAntialiasMode,
    d2d_wic_render_target_GetAntialiasMode,
    d2d_wic_render_target_SetTextAntialiasMode,
    d2d_wic_render_target_GetTextAntialiasMode,
    d2d_wic_render_target_SetTextRenderingParams,
    d2d_wic_render_target_GetTextRenderingParams,
    d2d_wic_render_target_SetTags,
    d2d_wic_render_target_GetTags,
    d2d_wic_render_target_PushLayer,
    d2d_wic_render_target_PopLayer,
    d2d_wic_render_target_Flush,
    d2d_wic_render_target_SaveDrawingState,
    d2d_wic_render_target_RestoreDrawingState,
    d2d_wic_render_target_PushAxisAlignedClip,
    d2d_wic_render_target_PopAxisAlignedClip,
    d2d_wic_render_target_Clear,
    d2d_wic_render_target_BeginDraw,
    d2d_wic_render_target_EndDraw,
    d2d_wic_render_target_GetPixelFormat,
    d2d_wic_render_target_SetDpi,
    d2d_wic_render_target_GetDpi,
    d2d_wic_render_target_GetSize,
    d2d_wic_render_target_GetPixelSize,
    d2d_wic_render_target_GetMaximumBitmapSize,
    d2d_wic_render_target_IsSupported,
};

HRESULT d2d_wic_render_target_init(struct d2d_wic_render_target *render_target, ID2D1Factory *factory,
        ID3D10Device1 *device, IWICBitmap *bitmap, const D2D1_RENDER_TARGET_PROPERTIES *desc)
{
    D3D10_TEXTURE2D_DESC texture_desc;
    ID3D10Texture2D *texture;
    HRESULT hr;

    render_target->ID2D1RenderTarget_iface.lpVtbl = &d2d_wic_render_target_vtbl;
    render_target->refcount = 1;

    if (FAILED(hr = IWICBitmap_GetSize(bitmap, &render_target->width, &render_target->height)))
    {
        WARN("Failed to get bitmap dimensions, hr %#x.\n", hr);
        return hr;
    }

    texture_desc.Width = render_target->width;
    texture_desc.Height = render_target->height;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;

    texture_desc.Format = desc->pixelFormat.format;
    if (texture_desc.Format == DXGI_FORMAT_UNKNOWN)
    {
        WICPixelFormatGUID bitmap_format;

        if (FAILED(hr = IWICBitmap_GetPixelFormat(bitmap, &bitmap_format)))
        {
            WARN("Failed to get bitmap format, hr %#x.\n", hr);
            return hr;
        }

        if (IsEqualGUID(&bitmap_format, &GUID_WICPixelFormat32bppPBGRA)
                || IsEqualGUID(&bitmap_format, &GUID_WICPixelFormat32bppBGR))
        {
            texture_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        }
        else
        {
            WARN("Unsupported WIC bitmap format %s.\n", debugstr_guid(&bitmap_format));
            return D2DERR_UNSUPPORTED_PIXEL_FORMAT;
        }
    }

    switch (texture_desc.Format)
    {
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            render_target->bpp = 4;
            break;

        default:
            FIXME("Unhandled format %#x.\n", texture_desc.Format);
            return D2DERR_UNSUPPORTED_PIXEL_FORMAT;
    }

    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = desc->usage & D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE ?
            D3D10_RESOURCE_MISC_GDI_COMPATIBLE : 0;

    if (FAILED(hr = ID3D10Device1_CreateTexture2D(device, &texture_desc, NULL, &texture)))
    {
        WARN("Failed to create texture, hr %#x.\n", hr);
        return hr;
    }

    hr = ID3D10Texture2D_QueryInterface(texture, &IID_IDXGISurface, (void **)&render_target->dxgi_surface);
    ID3D10Texture2D_Release(texture);
    if (FAILED(hr))
    {
        WARN("Failed to get DXGI surface interface, hr %#x.\n", hr);
        return hr;
    }

    texture_desc.Usage = D3D10_USAGE_STAGING;
    texture_desc.BindFlags = 0;
    texture_desc.CPUAccessFlags = D3D10_CPU_ACCESS_READ;
    texture_desc.MiscFlags = 0;

    if (FAILED(hr = ID3D10Device1_CreateTexture2D(device, &texture_desc, NULL, &render_target->readback_texture)))
    {
        WARN("Failed to create readback texture, hr %#x.\n", hr);
        IDXGISurface_Release(render_target->dxgi_surface);
        return hr;
    }

    if (FAILED(hr = d2d_d3d_create_render_target(factory, render_target->dxgi_surface,
            (IUnknown *)&render_target->ID2D1RenderTarget_iface, desc, &render_target->dxgi_target)))
    {
        WARN("Failed to create DXGI surface render target, hr %#x.\n", hr);
        ID3D10Texture2D_Release(render_target->readback_texture);
        IDXGISurface_Release(render_target->dxgi_surface);
        return hr;
    }

    render_target->bitmap = bitmap;
    IWICBitmap_AddRef(bitmap);

    return S_OK;
}
