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

static inline struct d2d_d3d_render_target *impl_from_ID2D1RenderTarget(ID2D1RenderTarget *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_d3d_render_target, ID2D1RenderTarget_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_d3d_render_target_QueryInterface(ID2D1RenderTarget *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1RenderTarget)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1RenderTarget_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_d3d_render_target_AddRef(ID2D1RenderTarget *iface)
{
    struct d2d_d3d_render_target *render_target = impl_from_ID2D1RenderTarget(iface);
    ULONG refcount = InterlockedIncrement(&render_target->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_d3d_render_target_Release(ID2D1RenderTarget *iface)
{
    struct d2d_d3d_render_target *render_target = impl_from_ID2D1RenderTarget(iface);
    ULONG refcount = InterlockedDecrement(&render_target->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
        HeapFree(GetProcessHeap(), 0, render_target);

    return refcount;
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_GetFactory(ID2D1RenderTarget *iface, ID2D1Factory **factory)
{
    FIXME("iface %p, factory %p stub!\n", iface, factory);

    *factory = NULL;
}

static HRESULT STDMETHODCALLTYPE d2d_d3d_render_target_CreateBitmap(ID2D1RenderTarget *iface,
        D2D1_SIZE_U size, const void *src_data, UINT32 pitch, const D2D1_BITMAP_PROPERTIES *desc, ID2D1Bitmap **bitmap)
{
    FIXME("iface %p, size {%u, %u}, src_data %p, pitch %u, desc %p, bitmap %p stub!\n",
            iface, size.width, size.height, src_data, pitch, desc, bitmap);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_d3d_render_target_CreateBitmapFromWicBitmap(ID2D1RenderTarget *iface,
        IWICBitmapSource *bitmap_source, const D2D1_BITMAP_PROPERTIES *desc, ID2D1Bitmap **bitmap)
{
    FIXME("iface %p, bitmap_source %p, desc %p, bitmap %p stub!\n",
            iface, bitmap_source, desc, bitmap);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_d3d_render_target_CreateSharedBitmap(ID2D1RenderTarget *iface,
        REFIID iid, void *data, const D2D1_BITMAP_PROPERTIES *desc, ID2D1Bitmap **bitmap)
{
    FIXME("iface %p, iid %s, data %p, desc %p, bitmap %p stub!\n",
        iface, debugstr_guid(iid), data, desc, bitmap);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_d3d_render_target_CreateBitmapBrush(ID2D1RenderTarget *iface,
        ID2D1Bitmap *bitmap, const D2D1_BITMAP_BRUSH_PROPERTIES *bitmap_brush_desc,
        const D2D1_BRUSH_PROPERTIES *brush_desc, ID2D1BitmapBrush **brush)
{
    FIXME("iface %p, bitmap %p, bitmap_brush_desc %p, brush_desc %p, brush %p stub!\n",
            iface, bitmap, bitmap_brush_desc, brush_desc, brush);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_d3d_render_target_CreateSolidColorBrush(ID2D1RenderTarget *iface,
        const D2D1_COLOR_F *color, const D2D1_BRUSH_PROPERTIES *desc, ID2D1SolidColorBrush **brush)
{
    FIXME("iface %p, color %p, desc %p, brush %p stub!\n", iface, color, desc, brush);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_d3d_render_target_CreateGradientStopCollection(ID2D1RenderTarget *iface,
        const D2D1_GRADIENT_STOP *stops, UINT32 stop_count, D2D1_GAMMA gamma, D2D1_EXTEND_MODE extend_mode,
        ID2D1GradientStopCollection **gradient)
{
    FIXME("iface %p, stops %p, stop_count %u, gamma %#x, extend_mode %#x, gradient %p stub!\n",
            iface, stops, stop_count, gamma, extend_mode, gradient);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_d3d_render_target_CreateLinearGradientBrush(ID2D1RenderTarget *iface,
        const D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES *gradient_brush_desc, const D2D1_BRUSH_PROPERTIES *brush_desc,
        ID2D1GradientStopCollection *gradient, ID2D1LinearGradientBrush **brush)
{
    FIXME("iface %p, gradient_brush_desc %p, brush_desc %p, gradient %p, brush %p stub!\n",
            iface, gradient_brush_desc, brush_desc, gradient, brush);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_d3d_render_target_CreateRadialGradientBrush(ID2D1RenderTarget *iface,
        const D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES *gradient_brush_desc, const D2D1_BRUSH_PROPERTIES *brush_desc,
        ID2D1GradientStopCollection *gradient, ID2D1RadialGradientBrush **brush)
{
    FIXME("iface %p, gradient_brush_desc %p, brush_desc %p, gradient %p, brush %p stub!\n",
            iface, gradient_brush_desc, brush_desc, gradient, brush);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_d3d_render_target_CreateCompatibleRenderTarget(ID2D1RenderTarget *iface,
        const D2D1_SIZE_F *size, const D2D1_SIZE_U *pixel_size, const D2D1_PIXEL_FORMAT *format,
        D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS options, ID2D1BitmapRenderTarget **render_target)
{
    FIXME("iface %p, size %p, pixel_size %p, format %p, options %#x, render_target %p stub!\n",
            iface, size, pixel_size, format, options, render_target);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_d3d_render_target_CreateLayer(ID2D1RenderTarget *iface,
        const D2D1_SIZE_F *size, ID2D1Layer **layer)
{
    FIXME("iface %p, size %p, layer %p stub!\n", iface, size, layer);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_d3d_render_target_CreateMesh(ID2D1RenderTarget *iface, ID2D1Mesh **mesh)
{
    FIXME("iface %p, mesh %p stub!\n", iface, mesh);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_DrawLine(ID2D1RenderTarget *iface,
        D2D1_POINT_2F p0, D2D1_POINT_2F p1, ID2D1Brush *brush, float stroke_width, ID2D1StrokeStyle *stroke_style)
{
    FIXME("iface %p, p0 {%.8e, %.8e}, p1 {%.8e, %.8e}, brush %p, stroke_width %.8e, stroke_style %p stub!\n",
            iface, p0.x, p0.y, p1.x, p1.y, brush, stroke_width, stroke_style);
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_DrawRectangle(ID2D1RenderTarget *iface,
        const D2D1_RECT_F *rect, ID2D1Brush *brush, float stroke_width, ID2D1StrokeStyle *stroke_style)
{
    FIXME("iface %p, rect %p, brush %p, stroke_width %.8e, stroke_style %p stub!\n",
            iface, rect, brush, stroke_width, stroke_style);
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_FillRectangle(ID2D1RenderTarget *iface,
        const D2D1_RECT_F *rect, ID2D1Brush *brush)
{
    FIXME("iface %p, rect %p, brush %p stub!\n", iface, rect, brush);
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_DrawRoundedRectangle(ID2D1RenderTarget *iface,
        const D2D1_ROUNDED_RECT *rect, ID2D1Brush *brush, float stroke_width, ID2D1StrokeStyle *stroke_style)
{
    FIXME("iface %p, rect %p, brush %p, stroke_width %.8e, stroke_style %p stub!\n",
            iface, rect, brush, stroke_width, stroke_style);
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_FillRoundedRectangle(ID2D1RenderTarget *iface,
        const D2D1_ROUNDED_RECT *rect, ID2D1Brush *brush)
{
    FIXME("iface %p, rect %p, brush %p stub!\n", iface, rect, brush);
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_DrawEllipse(ID2D1RenderTarget *iface,
        const D2D1_ELLIPSE *ellipse, ID2D1Brush *brush, float stroke_width, ID2D1StrokeStyle *stroke_style)
{
    FIXME("iface %p, ellipse %p, brush %p, stroke_width %.8e, stroke_style %p stub!\n",
            iface, ellipse, brush, stroke_width, stroke_style);
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_FillEllipse(ID2D1RenderTarget *iface,
        const D2D1_ELLIPSE *ellipse, ID2D1Brush *brush)
{
    FIXME("iface %p, ellipse %p, brush %p stub!\n", iface, ellipse, brush);
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_DrawGeometry(ID2D1RenderTarget *iface,
        ID2D1Geometry *geometry, ID2D1Brush *brush, float stroke_width, ID2D1StrokeStyle *stroke_style)
{
    FIXME("iface %p, geometry %p, brush %p, stroke_width %.8e, stroke_style %p stub!\n",
            iface, geometry, brush, stroke_width, stroke_style);
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_FillGeometry(ID2D1RenderTarget *iface,
        ID2D1Geometry *geometry, ID2D1Brush *brush, ID2D1Brush *opacity_brush)
{
    FIXME("iface %p, geometry %p, brush %p, opacity_brush %p stub!\n", iface, geometry, brush, opacity_brush);
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_FillMesh(ID2D1RenderTarget *iface,
        ID2D1Mesh *mesh, ID2D1Brush *brush)
{
    FIXME("iface %p, mesh %p, brush %p stub!\n", iface, mesh, brush);
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_FillOpacityMask(ID2D1RenderTarget *iface,
        ID2D1Bitmap *mask, ID2D1Brush *brush, D2D1_OPACITY_MASK_CONTENT content,
        const D2D1_RECT_F *dst_rect, const D2D1_RECT_F *src_rect)
{
    FIXME("iface %p, mask %p, brush %p, content %#x, dst_rect %p, src_rect %p stub!\n",
            iface, mask, brush, content, dst_rect, src_rect);
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_DrawBitmap(ID2D1RenderTarget *iface,
        ID2D1Bitmap *bitmap, const D2D1_RECT_F *dst_rect, float opacity,
        D2D1_BITMAP_INTERPOLATION_MODE interpolation_mode, const D2D1_RECT_F *src_rect)
{
    FIXME("iface %p, bitmap %p, dst_rect %p, opacity %.8e, interpolation_mode %#x, src_rect %p stub!\n",
            iface, bitmap, dst_rect, opacity, interpolation_mode, src_rect);
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_DrawText(ID2D1RenderTarget *iface,
        const WCHAR *string, UINT32 string_len, IDWriteTextFormat *text_format, const D2D1_RECT_F *layout_rect,
        ID2D1Brush *brush, D2D1_DRAW_TEXT_OPTIONS options, DWRITE_MEASURING_MODE measuring_mode)
{
    FIXME("iface %p, string %s, string_len %u, text_format %p, layout_rect %p, "
            "brush %p, options %#x, measuring_mode %#x stub!\n",
            iface, debugstr_wn(string, string_len), string_len, text_format, layout_rect,
            brush, options, measuring_mode);
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_DrawTextLayout(ID2D1RenderTarget *iface,
        D2D1_POINT_2F origin, IDWriteTextLayout *layout, ID2D1Brush *brush, D2D1_DRAW_TEXT_OPTIONS options)
{
    FIXME("iface %p, origin {%.8e, %.8e}, layout %p, brush %p, options %#x stub!\n",
            iface, origin.x, origin.y, layout, brush, options);
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_DrawGlyphRun(ID2D1RenderTarget *iface,
        D2D1_POINT_2F baseline_origin, const DWRITE_GLYPH_RUN *glyph_run, ID2D1Brush *brush,
        DWRITE_MEASURING_MODE measuring_mode)
{
    FIXME("iface %p, baseline_origin {%.8e, %.8e}, glyph_run %p, brush %p, measuring_mode %#x stub!\n",
            iface, baseline_origin.x, baseline_origin.y, glyph_run, brush, measuring_mode);
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_SetTransform(ID2D1RenderTarget *iface,
        const D2D1_MATRIX_3X2_F *transform)
{
    FIXME("iface %p, transform %p stub!\n", iface, transform);
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_GetTransform(ID2D1RenderTarget *iface,
        D2D1_MATRIX_3X2_F *transform)
{
    FIXME("iface %p, transform %p stub!\n", iface, transform);
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_SetAntialiasMode(ID2D1RenderTarget *iface,
        D2D1_ANTIALIAS_MODE antialias_mode)
{
    FIXME("iface %p, antialias_mode %#x stub!\n", iface, antialias_mode);
}

static D2D1_ANTIALIAS_MODE STDMETHODCALLTYPE d2d_d3d_render_target_GetAntialiasMode(ID2D1RenderTarget *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D2D1_ANTIALIAS_MODE_PER_PRIMITIVE;
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_SetTextAntialiasMode(ID2D1RenderTarget *iface,
        D2D1_TEXT_ANTIALIAS_MODE antialias_mode)
{
    FIXME("iface %p, antialias_mode %#x stub!\n", iface, antialias_mode);
}

static D2D1_TEXT_ANTIALIAS_MODE STDMETHODCALLTYPE d2d_d3d_render_target_GetTextAntialiasMode(ID2D1RenderTarget *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D2D1_TEXT_ANTIALIAS_MODE_DEFAULT;
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_SetTextRenderingParams(ID2D1RenderTarget *iface,
        IDWriteRenderingParams *text_rendering_params)
{
    FIXME("iface %p, text_rendering_params %p stub!\n", iface, text_rendering_params);
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_GetTextRenderingParams(ID2D1RenderTarget *iface,
        IDWriteRenderingParams **text_rendering_params)
{
    FIXME("iface %p, text_rendering_params %p stub!\n", iface, text_rendering_params);

    *text_rendering_params = NULL;
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_SetTags(ID2D1RenderTarget *iface, D2D1_TAG tag1, D2D1_TAG tag2)
{
    FIXME("iface %p, tag1 %s, tag2 %s stub!\n", iface, wine_dbgstr_longlong(tag1), wine_dbgstr_longlong(tag2));
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_GetTags(ID2D1RenderTarget *iface, D2D1_TAG *tag1, D2D1_TAG *tag2)
{
    FIXME("iface %p, tag1 %p, tag2 %p stub!\n", iface, tag1, tag2);

    *tag1 = 0;
    *tag2 = 0;
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_PushLayer(ID2D1RenderTarget *iface,
        const D2D1_LAYER_PARAMETERS *layer_parameters, ID2D1Layer *layer)
{
    FIXME("iface %p, layer_parameters %p, layer %p stub!\n", iface, layer_parameters, layer);
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_PopLayer(ID2D1RenderTarget *iface)
{
    FIXME("iface %p stub!\n", iface);
}

static HRESULT STDMETHODCALLTYPE d2d_d3d_render_target_Flush(ID2D1RenderTarget *iface, D2D1_TAG *tag1, D2D1_TAG *tag2)
{
    FIXME("iface %p, tag1 %p, tag2 %p stub!\n", iface, tag1, tag2);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_SaveDrawingState(ID2D1RenderTarget *iface,
        ID2D1DrawingStateBlock *state_block)
{
    FIXME("iface %p, state_block %p stub!\n", iface, state_block);
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_RestoreDrawingState(ID2D1RenderTarget *iface,
        ID2D1DrawingStateBlock *state_block)
{
    FIXME("iface %p, state_block %p stub!\n", iface, state_block);
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_PushAxisAlignedClip(ID2D1RenderTarget *iface,
        const D2D1_RECT_F *clip_rect, D2D1_ANTIALIAS_MODE antialias_mode)
{
    FIXME("iface %p, clip_rect %p, antialias_mode %#x stub!\n", iface, clip_rect, antialias_mode);
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_PopAxisAlignedClip(ID2D1RenderTarget *iface)
{
    FIXME("iface %p stub!\n", iface);
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_Clear(ID2D1RenderTarget *iface, const D2D1_COLOR_F *color)
{
    FIXME("iface %p, color %p stub!\n", iface, color);
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_BeginDraw(ID2D1RenderTarget *iface)
{
    FIXME("iface %p stub!\n", iface);
}

static HRESULT STDMETHODCALLTYPE d2d_d3d_render_target_EndDraw(ID2D1RenderTarget *iface,
        D2D1_TAG *tag1, D2D1_TAG *tag2)
{
    FIXME("iface %p, tag1 %p, tag2 %p stub!\n", iface, tag1, tag2);

    return E_NOTIMPL;
}

static D2D1_PIXEL_FORMAT STDMETHODCALLTYPE d2d_d3d_render_target_GetPixelFormat(ID2D1RenderTarget *iface)
{
    static const D2D1_PIXEL_FORMAT format = {DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_UNKNOWN};

    FIXME("iface %p stub!\n", iface);

    return format;
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_SetDpi(ID2D1RenderTarget *iface, float dpi_x, float dpi_y)
{
    FIXME("iface %p, dpi_x %.8e, dpi_y %.8e stub!\n", iface, dpi_x, dpi_y);
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_GetDpi(ID2D1RenderTarget *iface, float *dpi_x, float *dpi_y)
{
    FIXME("iface %p, dpi_x %p, dpi_y %p stub!\n", iface, dpi_x, dpi_y);

    *dpi_x = 96.0f;
    *dpi_y = 96.0f;
}

static D2D1_SIZE_F STDMETHODCALLTYPE d2d_d3d_render_target_GetSize(ID2D1RenderTarget *iface)
{
    static const D2D1_SIZE_F size = {0.0f, 0.0f};

    FIXME("iface %p stub!\n", iface);

    return size;
}

static D2D1_SIZE_U STDMETHODCALLTYPE d2d_d3d_render_target_GetPixelSize(ID2D1RenderTarget *iface)
{
    static const D2D1_SIZE_U size = {0, 0};

    FIXME("iface %p stub!\n", iface);

    return size;
}

static UINT32 STDMETHODCALLTYPE d2d_d3d_render_target_GetMaximumBitmapSize(ID2D1RenderTarget *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static BOOL STDMETHODCALLTYPE d2d_d3d_render_target_IsSupported(ID2D1RenderTarget *iface,
        const D2D1_RENDER_TARGET_PROPERTIES *desc)
{
    FIXME("iface %p, desc %p stub!\n", iface, desc);

    return FALSE;
}

static const struct ID2D1RenderTargetVtbl d2d_d3d_render_target_vtbl =
{
    d2d_d3d_render_target_QueryInterface,
    d2d_d3d_render_target_AddRef,
    d2d_d3d_render_target_Release,
    d2d_d3d_render_target_GetFactory,
    d2d_d3d_render_target_CreateBitmap,
    d2d_d3d_render_target_CreateBitmapFromWicBitmap,
    d2d_d3d_render_target_CreateSharedBitmap,
    d2d_d3d_render_target_CreateBitmapBrush,
    d2d_d3d_render_target_CreateSolidColorBrush,
    d2d_d3d_render_target_CreateGradientStopCollection,
    d2d_d3d_render_target_CreateLinearGradientBrush,
    d2d_d3d_render_target_CreateRadialGradientBrush,
    d2d_d3d_render_target_CreateCompatibleRenderTarget,
    d2d_d3d_render_target_CreateLayer,
    d2d_d3d_render_target_CreateMesh,
    d2d_d3d_render_target_DrawLine,
    d2d_d3d_render_target_DrawRectangle,
    d2d_d3d_render_target_FillRectangle,
    d2d_d3d_render_target_DrawRoundedRectangle,
    d2d_d3d_render_target_FillRoundedRectangle,
    d2d_d3d_render_target_DrawEllipse,
    d2d_d3d_render_target_FillEllipse,
    d2d_d3d_render_target_DrawGeometry,
    d2d_d3d_render_target_FillGeometry,
    d2d_d3d_render_target_FillMesh,
    d2d_d3d_render_target_FillOpacityMask,
    d2d_d3d_render_target_DrawBitmap,
    d2d_d3d_render_target_DrawText,
    d2d_d3d_render_target_DrawTextLayout,
    d2d_d3d_render_target_DrawGlyphRun,
    d2d_d3d_render_target_SetTransform,
    d2d_d3d_render_target_GetTransform,
    d2d_d3d_render_target_SetAntialiasMode,
    d2d_d3d_render_target_GetAntialiasMode,
    d2d_d3d_render_target_SetTextAntialiasMode,
    d2d_d3d_render_target_GetTextAntialiasMode,
    d2d_d3d_render_target_SetTextRenderingParams,
    d2d_d3d_render_target_GetTextRenderingParams,
    d2d_d3d_render_target_SetTags,
    d2d_d3d_render_target_GetTags,
    d2d_d3d_render_target_PushLayer,
    d2d_d3d_render_target_PopLayer,
    d2d_d3d_render_target_Flush,
    d2d_d3d_render_target_SaveDrawingState,
    d2d_d3d_render_target_RestoreDrawingState,
    d2d_d3d_render_target_PushAxisAlignedClip,
    d2d_d3d_render_target_PopAxisAlignedClip,
    d2d_d3d_render_target_Clear,
    d2d_d3d_render_target_BeginDraw,
    d2d_d3d_render_target_EndDraw,
    d2d_d3d_render_target_GetPixelFormat,
    d2d_d3d_render_target_SetDpi,
    d2d_d3d_render_target_GetDpi,
    d2d_d3d_render_target_GetSize,
    d2d_d3d_render_target_GetPixelSize,
    d2d_d3d_render_target_GetMaximumBitmapSize,
    d2d_d3d_render_target_IsSupported,
};

void d2d_d3d_render_target_init(struct d2d_d3d_render_target *render_target, ID2D1Factory *factory,
        IDXGISurface *surface, const D2D1_RENDER_TARGET_PROPERTIES *desc)
{
    FIXME("Ignoring render target properties.\n");

    render_target->ID2D1RenderTarget_iface.lpVtbl = &d2d_d3d_render_target_vtbl;
    render_target->refcount = 1;
}
