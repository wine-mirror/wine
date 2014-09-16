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

#define INITIAL_CLIP_STACK_SIZE 4

static void d2d_point_transform(D2D1_POINT_2F *dst, const D2D1_MATRIX_3X2_F *matrix, float x, float y)
{
    dst->x = x * matrix->_11 + y * matrix->_21 + matrix->_31;
    dst->y = x * matrix->_12 + y * matrix->_22 + matrix->_32;
}

static void d2d_rect_expand(D2D1_RECT_F *dst, const D2D1_POINT_2F *point)
{
    if (point->x < dst->left)
        dst->left = point->x;
    if (point->y < dst->top)
        dst->top = point->y;
    if (point->x > dst->right)
        dst->right = point->x;
    if (point->y > dst->bottom)
        dst->bottom = point->y;
}

static void d2d_rect_intersect(D2D1_RECT_F *dst, const D2D1_RECT_F *src)
{
    if (src->left > dst->left)
        dst->left = src->left;
    if (src->top > dst->top)
        dst->top = src->top;
    if (src->right < dst->right)
        dst->right = src->right;
    if (src->bottom < dst->bottom)
        dst->bottom = src->bottom;
}

static void d2d_rect_set(D2D1_RECT_F *dst, float left, float top, float right, float bottom)
{
    dst->left = left;
    dst->top = top;
    dst->right = right;
    dst->bottom = bottom;
}

static BOOL d2d_clip_stack_init(struct d2d_clip_stack *stack)
{
    if (!(stack->stack = HeapAlloc(GetProcessHeap(), 0, INITIAL_CLIP_STACK_SIZE * sizeof(*stack->stack))))
        return FALSE;

    stack->size = INITIAL_CLIP_STACK_SIZE;
    stack->count = 0;

    return TRUE;
}

static void d2d_clip_stack_cleanup(struct d2d_clip_stack *stack)
{
    HeapFree(GetProcessHeap(), 0, stack->stack);
}

static BOOL d2d_clip_stack_push(struct d2d_clip_stack *stack, const D2D1_RECT_F *rect)
{
    D2D1_RECT_F r;

    if (stack->count == stack->size)
    {
        D2D1_RECT_F *new_stack;
        unsigned int new_size;

        if (stack->size > UINT_MAX / 2)
            return FALSE;

        new_size = stack->size * 2;
        if (!(new_stack = HeapReAlloc(GetProcessHeap(), 0, stack->stack, new_size * sizeof(*stack->stack))))
            return FALSE;

        stack->stack = new_stack;
        stack->size = new_size;
    }

    r = *rect;
    if (stack->count)
        d2d_rect_intersect(&r, &stack->stack[stack->count - 1]);
    stack->stack[stack->count++] = r;

    return TRUE;
}

static void d2d_clip_stack_pop(struct d2d_clip_stack *stack)
{
    if (!stack->count)
        return;
    --stack->count;
}

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
    {
        d2d_clip_stack_cleanup(&render_target->clip_stack);
        ID3D10RenderTargetView_Release(render_target->view);
        ID3D10RasterizerState_Release(render_target->clear_rs);
        ID3D10PixelShader_Release(render_target->clear_ps);
        ID3D10VertexShader_Release(render_target->clear_vs);
        ID3D10Buffer_Release(render_target->clear_vb);
        ID3D10InputLayout_Release(render_target->clear_il);
        render_target->stateblock->lpVtbl->Release(render_target->stateblock);
        ID3D10Device_Release(render_target->device);
        HeapFree(GetProcessHeap(), 0, render_target);
    }

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
    struct d2d_bitmap *object;

    TRACE("iface %p, size {%u, %u}, src_data %p, pitch %u, desc %p, bitmap %p.\n",
            iface, size.width, size.height, src_data, pitch, desc, bitmap);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    d2d_bitmap_init(object, size, src_data, pitch, desc);

    TRACE("Created bitmap %p.\n", object);
    *bitmap = &object->ID2D1Bitmap_iface;

    return S_OK;
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
    struct d2d_brush *object;

    TRACE("iface %p, color %p, desc %p, brush %p.\n", iface, color, desc, brush);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    d2d_solid_color_brush_init(object, iface, color, desc);

    TRACE("Created brush %p.\n", object);
    *brush = (ID2D1SolidColorBrush *)&object->ID2D1Brush_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_d3d_render_target_CreateGradientStopCollection(ID2D1RenderTarget *iface,
        const D2D1_GRADIENT_STOP *stops, UINT32 stop_count, D2D1_GAMMA gamma, D2D1_EXTEND_MODE extend_mode,
        ID2D1GradientStopCollection **gradient)
{
    struct d2d_gradient *object;

    TRACE("iface %p, stops %p, stop_count %u, gamma %#x, extend_mode %#x, gradient %p.\n",
            iface, stops, stop_count, gamma, extend_mode, gradient);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    d2d_gradient_init(object, iface, stops, stop_count, gamma, extend_mode);

    TRACE("Created gradient %p.\n", object);
    *gradient = &object->ID2D1GradientStopCollection_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_d3d_render_target_CreateLinearGradientBrush(ID2D1RenderTarget *iface,
        const D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES *gradient_brush_desc, const D2D1_BRUSH_PROPERTIES *brush_desc,
        ID2D1GradientStopCollection *gradient, ID2D1LinearGradientBrush **brush)
{
    struct d2d_brush *object;

    TRACE("iface %p, gradient_brush_desc %p, brush_desc %p, gradient %p, brush %p.\n",
            iface, gradient_brush_desc, brush_desc, gradient, brush);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    d2d_linear_gradient_brush_init(object, iface, gradient_brush_desc, brush_desc, gradient);

    TRACE("Created brush %p.\n", object);
    *brush = (ID2D1LinearGradientBrush *)&object->ID2D1Brush_iface;

    return S_OK;
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
    struct d2d_mesh *object;

    TRACE("iface %p, mesh %p.\n", iface, mesh);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    d2d_mesh_init(object);

    TRACE("Created mesh %p.\n", object);
    *mesh = &object->ID2D1Mesh_iface;

    return S_OK;
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
    struct d2d_d3d_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, transform %p.\n", iface, transform);

    render_target->transform = *transform;
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_GetTransform(ID2D1RenderTarget *iface,
        D2D1_MATRIX_3X2_F *transform)
{
    struct d2d_d3d_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, transform %p.\n", iface, transform);

    *transform = render_target->transform;
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
    struct d2d_d3d_render_target *render_target = impl_from_ID2D1RenderTarget(iface);
    D2D1_RECT_F transformed_rect;
    float x_scale, y_scale;
    D2D1_POINT_2F point;

    TRACE("iface %p, clip_rect %p, antialias_mode %#x.\n", iface, clip_rect, antialias_mode);

    if (antialias_mode != D2D1_ANTIALIAS_MODE_ALIASED)
        FIXME("Ignoring antialias_mode %#x.\n", antialias_mode);

    x_scale = render_target->dpi_x / 96.0f;
    y_scale = render_target->dpi_y / 96.0f;
    d2d_point_transform(&point, &render_target->transform, clip_rect->left * x_scale, clip_rect->top * y_scale);
    d2d_rect_set(&transformed_rect, point.x, point.y, point.x, point.y);
    d2d_point_transform(&point, &render_target->transform, clip_rect->left * x_scale, clip_rect->bottom * y_scale);
    d2d_rect_expand(&transformed_rect, &point);
    d2d_point_transform(&point, &render_target->transform, clip_rect->right * x_scale, clip_rect->top * y_scale);
    d2d_rect_expand(&transformed_rect, &point);
    d2d_point_transform(&point, &render_target->transform, clip_rect->right * x_scale, clip_rect->bottom * y_scale);
    d2d_rect_expand(&transformed_rect, &point);

    if (!d2d_clip_stack_push(&render_target->clip_stack, &transformed_rect))
        WARN("Failed to push clip rect.\n");
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_PopAxisAlignedClip(ID2D1RenderTarget *iface)
{
    struct d2d_d3d_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p.\n", iface);

    d2d_clip_stack_pop(&render_target->clip_stack);
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_Clear(ID2D1RenderTarget *iface, const D2D1_COLOR_F *color)
{
    struct d2d_d3d_render_target *render_target = impl_from_ID2D1RenderTarget(iface);
    D3D10_SUBRESOURCE_DATA buffer_data;
    D3D10_BUFFER_DESC buffer_desc;
    unsigned int offset;
    D3D10_VIEWPORT vp;
    ID3D10Buffer *cb;
    HRESULT hr;

    TRACE("iface %p, color %p.\n", iface, color);

    buffer_desc.ByteWidth = sizeof(*color);
    buffer_desc.Usage = D3D10_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.MiscFlags = 0;

    buffer_data.pSysMem = color;
    buffer_data.SysMemPitch = 0;
    buffer_data.SysMemSlicePitch = 0;

    if (FAILED(hr = ID3D10Device_CreateBuffer(render_target->device, &buffer_desc, &buffer_data, &cb)))
    {
        WARN("Failed to create constant buffer, hr %#x.\n", hr);
        return;
    }

    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width = render_target->pixel_size.width;
    vp.Height = render_target->pixel_size.height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    if (FAILED(hr = render_target->stateblock->lpVtbl->Capture(render_target->stateblock)))
    {
        WARN("Failed to capture stateblock, hr %#x.\n", hr);
        ID3D10Buffer_Release(cb);
        return;
    }

    ID3D10Device_ClearState(render_target->device);

    ID3D10Device_IASetInputLayout(render_target->device, render_target->clear_il);
    ID3D10Device_IASetPrimitiveTopology(render_target->device, D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    offset = 0;
    ID3D10Device_IASetVertexBuffers(render_target->device, 0, 1,
            &render_target->clear_vb, &render_target->clear_vb_stride, &offset);
    ID3D10Device_VSSetShader(render_target->device, render_target->clear_vs);
    ID3D10Device_PSSetConstantBuffers(render_target->device, 0, 1, &cb);
    ID3D10Device_PSSetShader(render_target->device, render_target->clear_ps);
    ID3D10Device_RSSetViewports(render_target->device, 1, &vp);
    if (render_target->clip_stack.count)
    {
        const D2D1_RECT_F *clip_rect;
        D3D10_RECT scissor_rect;

        clip_rect = &render_target->clip_stack.stack[render_target->clip_stack.count - 1];
        scissor_rect.left = clip_rect->left + 0.5f;
        scissor_rect.top = clip_rect->top + 0.5f;
        scissor_rect.right = clip_rect->right + 0.5f;
        scissor_rect.bottom = clip_rect->bottom + 0.5f;
        ID3D10Device_RSSetScissorRects(render_target->device, 1, &scissor_rect);
        ID3D10Device_RSSetState(render_target->device, render_target->clear_rs);
    }
    ID3D10Device_OMSetRenderTargets(render_target->device, 1, &render_target->view, NULL);

    ID3D10Device_Draw(render_target->device, 4, 0);

    if (FAILED(hr = render_target->stateblock->lpVtbl->Apply(render_target->stateblock)))
        WARN("Failed to apply stateblock, hr %#x.\n", hr);

    ID3D10Buffer_Release(cb);
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_BeginDraw(ID2D1RenderTarget *iface)
{
    TRACE("iface %p.\n", iface);
}

static HRESULT STDMETHODCALLTYPE d2d_d3d_render_target_EndDraw(ID2D1RenderTarget *iface,
        D2D1_TAG *tag1, D2D1_TAG *tag2)
{
    TRACE("iface %p, tag1 %p, tag2 %p.\n", iface, tag1, tag2);

    if (tag1)
        *tag1 = 0;
    if (tag2)
        *tag2 = 0;

    return S_OK;
}

static D2D1_PIXEL_FORMAT * STDMETHODCALLTYPE d2d_d3d_render_target_GetPixelFormat(ID2D1RenderTarget *iface,
        D2D1_PIXEL_FORMAT *format)
{
    FIXME("iface %p, format %p stub!\n", iface, format);

    format->format = DXGI_FORMAT_UNKNOWN;
    format->alphaMode = D2D1_ALPHA_MODE_UNKNOWN;
    return format;
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_SetDpi(ID2D1RenderTarget *iface, float dpi_x, float dpi_y)
{
    struct d2d_d3d_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, dpi_x %.8e, dpi_y %.8e.\n", iface, dpi_x, dpi_y);

    if (dpi_x == 0.0f && dpi_y == 0.0f)
    {
        dpi_x = 96.0f;
        dpi_y = 96.0f;
    }

    render_target->dpi_x = dpi_x;
    render_target->dpi_y = dpi_y;
}

static void STDMETHODCALLTYPE d2d_d3d_render_target_GetDpi(ID2D1RenderTarget *iface, float *dpi_x, float *dpi_y)
{
    struct d2d_d3d_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, dpi_x %p, dpi_y %p.\n", iface, dpi_x, dpi_y);

    *dpi_x = render_target->dpi_x;
    *dpi_y = render_target->dpi_y;
}

static D2D1_SIZE_F * STDMETHODCALLTYPE d2d_d3d_render_target_GetSize(ID2D1RenderTarget *iface, D2D1_SIZE_F *size)
{
    struct d2d_d3d_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, size %p.\n", iface, size);

    size->width = render_target->pixel_size.width / (render_target->dpi_x / 96.0f);
    size->height = render_target->pixel_size.height / (render_target->dpi_y / 96.0f);
    return size;
}

static D2D1_SIZE_U * STDMETHODCALLTYPE d2d_d3d_render_target_GetPixelSize(ID2D1RenderTarget *iface,
        D2D1_SIZE_U *pixel_size)
{
    struct d2d_d3d_render_target *render_target = impl_from_ID2D1RenderTarget(iface);

    TRACE("iface %p, pixel_size %p.\n", iface, pixel_size);

    *pixel_size = render_target->pixel_size;
    return pixel_size;
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

HRESULT d2d_d3d_render_target_init(struct d2d_d3d_render_target *render_target, ID2D1Factory *factory,
        IDXGISurface *surface, const D2D1_RENDER_TARGET_PROPERTIES *desc)
{
    D3D10_SUBRESOURCE_DATA buffer_data;
    D3D10_STATE_BLOCK_MASK state_mask;
    DXGI_SURFACE_DESC surface_desc;
    D3D10_RASTERIZER_DESC rs_desc;
    D3D10_BUFFER_DESC buffer_desc;
    ID3D10Resource *resource;
    HRESULT hr;

    static const D3D10_INPUT_ELEMENT_DESC clear_il_desc[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0},
    };
    static const DWORD clear_vs_code[] =
    {
        /* float4 main(float4 position : POSITION) : SV_POSITION
         * {
         *     return position;
         * } */
        0x43425844, 0x1fa8c27f, 0x52d2f21d, 0xc196fdb7, 0x376f283a, 0x00000001, 0x000001b4, 0x00000005,
        0x00000034, 0x0000008c, 0x000000c0, 0x000000f4, 0x00000138, 0x46454452, 0x00000050, 0x00000000,
        0x00000000, 0x00000000, 0x0000001c, 0xfffe0400, 0x00000100, 0x0000001c, 0x7263694d, 0x666f736f,
        0x52282074, 0x4c482029, 0x53204c53, 0x65646168, 0x6f432072, 0x6c69706d, 0x39207265, 0x2e30332e,
        0x30303239, 0x3336312e, 0xab003438, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x49534f50, 0x4e4f4954, 0xababab00,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000001, 0x00000003,
        0x00000000, 0x0000000f, 0x505f5653, 0x5449534f, 0x004e4f49, 0x52444853, 0x0000003c, 0x00010040,
        0x0000000f, 0x0300005f, 0x001010f2, 0x00000000, 0x04000067, 0x001020f2, 0x00000000, 0x00000001,
        0x05000036, 0x001020f2, 0x00000000, 0x00101e46, 0x00000000, 0x0100003e, 0x54415453, 0x00000074,
        0x00000002, 0x00000000, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x00000001,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    };
    static const DWORD clear_ps_code[] =
    {
        /* float4 color;
         *
         * float4 main(float4 position : SV_POSITION) : SV_Target
         * {
         *     return color;
         * } */
        0x43425844, 0xecd3cc9d, 0x0025bc77, 0x7a333165, 0x5b04c7e4, 0x00000001, 0x0000022c, 0x00000005,
        0x00000034, 0x00000100, 0x00000134, 0x00000168, 0x000001b0, 0x46454452, 0x000000c4, 0x00000001,
        0x00000048, 0x00000001, 0x0000001c, 0xffff0400, 0x00000100, 0x00000090, 0x0000003c, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x6f6c4724, 0x736c6162,
        0xababab00, 0x0000003c, 0x00000001, 0x00000060, 0x00000010, 0x00000000, 0x00000000, 0x00000078,
        0x00000000, 0x00000010, 0x00000002, 0x00000080, 0x00000000, 0x6f6c6f63, 0xabab0072, 0x00030001,
        0x00040001, 0x00000000, 0x00000000, 0x7263694d, 0x666f736f, 0x52282074, 0x4c482029, 0x53204c53,
        0x65646168, 0x6f432072, 0x6c69706d, 0x39207265, 0x2e30332e, 0x30303239, 0x3336312e, 0xab003438,
        0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000001, 0x00000003,
        0x00000000, 0x0000000f, 0x505f5653, 0x5449534f, 0x004e4f49, 0x4e47534f, 0x0000002c, 0x00000001,
        0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x0000000f, 0x545f5653,
        0x65677261, 0xabab0074, 0x52444853, 0x00000040, 0x00000040, 0x00000010, 0x04000059, 0x00208e46,
        0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x06000036, 0x001020f2, 0x00000000,
        0x00208e46, 0x00000000, 0x00000000, 0x0100003e, 0x54415453, 0x00000074, 0x00000002, 0x00000000,
        0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000,
    };
    static const struct
    {
        float x, y;
    }
    clear_quad[] =
    {
        {-1.0f, -1.0f},
        {-1.0f,  1.0f},
        { 1.0f, -1.0f},
        { 1.0f,  1.0f},
    };
    static const D2D1_MATRIX_3X2_F identity =
    {
        1.0f, 0.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
    };

    FIXME("Ignoring render target properties.\n");

    render_target->ID2D1RenderTarget_iface.lpVtbl = &d2d_d3d_render_target_vtbl;
    render_target->refcount = 1;

    if (FAILED(hr = IDXGISurface_GetDevice(surface, &IID_ID3D10Device, (void **)&render_target->device)))
    {
        WARN("Failed to get device interface, hr %#x.\n", hr);
        return hr;
    }

    if (FAILED(hr = IDXGISurface_QueryInterface(surface, &IID_ID3D10Resource, (void **)&resource)))
    {
        WARN("Failed to get ID3D10Resource interface, hr %#x.\n", hr);
        goto err;
    }

    hr = ID3D10Device_CreateRenderTargetView(render_target->device, resource, NULL, &render_target->view);
    ID3D10Resource_Release(resource);
    if (FAILED(hr))
    {
        WARN("Failed to create rendertarget view, hr %#x.\n", hr);
        goto err;
    }

    if (FAILED(hr = D3D10StateBlockMaskEnableAll(&state_mask)))
    {
        WARN("Failed to create stateblock mask, hr %#x.\n", hr);
        goto err;
    }

    if (FAILED(hr = D3D10CreateStateBlock(render_target->device, &state_mask, &render_target->stateblock)))
    {
        WARN("Failed to create stateblock, hr %#x.\n", hr);
        goto err;
    }

    if (FAILED(hr = ID3D10Device_CreateInputLayout(render_target->device, clear_il_desc,
            sizeof(clear_il_desc) / sizeof(*clear_il_desc), clear_vs_code, sizeof(clear_vs_code),
            &render_target->clear_il)))
    {
        WARN("Failed to create clear input layout, hr %#x.\n", hr);
        goto err;
    }

    buffer_desc.ByteWidth = sizeof(clear_quad);
    buffer_desc.Usage = D3D10_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.MiscFlags = 0;

    buffer_data.pSysMem = clear_quad;
    buffer_data.SysMemPitch = 0;
    buffer_data.SysMemSlicePitch = 0;

    render_target->clear_vb_stride = sizeof(*clear_quad);
    if (FAILED(hr = ID3D10Device_CreateBuffer(render_target->device,
            &buffer_desc, &buffer_data, &render_target->clear_vb)))
    {
        WARN("Failed to create clear vertex buffer, hr %#x.\n", hr);
        goto err;
    }

    if (FAILED(hr = ID3D10Device_CreateVertexShader(render_target->device,
            clear_vs_code, sizeof(clear_vs_code), &render_target->clear_vs)))
    {
        WARN("Failed to create clear vertex shader, hr %#x.\n", hr);
        goto err;
    }

    if (FAILED(hr = ID3D10Device_CreatePixelShader(render_target->device,
            clear_ps_code, sizeof(clear_ps_code), &render_target->clear_ps)))
    {
        WARN("Failed to create clear pixel shader, hr %#x.\n", hr);
        goto err;
    }

    rs_desc.FillMode = D3D10_FILL_SOLID;
    rs_desc.CullMode = D3D10_CULL_BACK;
    rs_desc.FrontCounterClockwise = FALSE;
    rs_desc.DepthBias = 0;
    rs_desc.DepthBiasClamp = 0.0f;
    rs_desc.SlopeScaledDepthBias = 0.0f;
    rs_desc.DepthClipEnable = TRUE;
    rs_desc.ScissorEnable = TRUE;
    rs_desc.MultisampleEnable = FALSE;
    rs_desc.AntialiasedLineEnable = FALSE;
    if (FAILED(hr = ID3D10Device_CreateRasterizerState(render_target->device, &rs_desc, &render_target->clear_rs)))
    {
        WARN("Failed to create clear rasterizer state, hr %#x.\n", hr);
        goto err;
    }

    if (FAILED(hr = IDXGISurface_GetDesc(surface, &surface_desc)))
    {
        WARN("Failed to get surface desc, hr %#x.\n", hr);
        goto err;
    }

    render_target->pixel_size.width = surface_desc.Width;
    render_target->pixel_size.height = surface_desc.Height;
    render_target->transform = identity;

    if (!d2d_clip_stack_init(&render_target->clip_stack))
    {
        WARN("Failed to initialize clip stack.\n");
        hr = E_FAIL;
        goto err;
    }

    render_target->dpi_x = desc->dpiX;
    render_target->dpi_y = desc->dpiY;

    if (render_target->dpi_x == 0.0f && render_target->dpi_y == 0.0f)
    {
        render_target->dpi_x = 96.0f;
        render_target->dpi_y = 96.0f;
    }

    return S_OK;

err:
    if (render_target->view)
        ID3D10RenderTargetView_Release(render_target->view);
    if (render_target->clear_rs)
        ID3D10RasterizerState_Release(render_target->clear_rs);
    if (render_target->clear_ps)
        ID3D10PixelShader_Release(render_target->clear_ps);
    if (render_target->clear_vs)
        ID3D10VertexShader_Release(render_target->clear_vs);
    if (render_target->clear_vb)
        ID3D10Buffer_Release(render_target->clear_vb);
    if (render_target->clear_il)
        ID3D10InputLayout_Release(render_target->clear_il);
    if (render_target->stateblock)
        render_target->stateblock->lpVtbl->Release(render_target->stateblock);
    if (render_target->device)
        ID3D10Device_Release(render_target->device);
    return hr;
}
