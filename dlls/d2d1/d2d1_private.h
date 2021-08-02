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

#ifndef __WINE_D2D1_PRIVATE_H
#define __WINE_D2D1_PRIVATE_H

#include "wine/debug.h"
#include "wine/heap.h"

#include <assert.h>
#include <limits.h>
#include <math.h>
#define COBJMACROS
#include "d2d1_2.h"
#include "d3d11_1.h"
#ifdef D2D1_INIT_GUID
#include "initguid.h"
#endif
#include "dwrite_2.h"

enum d2d_brush_type
{
    D2D_BRUSH_TYPE_SOLID,
    D2D_BRUSH_TYPE_LINEAR,
    D2D_BRUSH_TYPE_RADIAL,
    D2D_BRUSH_TYPE_BITMAP,
    D2D_BRUSH_TYPE_COUNT,
};

enum d2d_shape_type
{
    D2D_SHAPE_TYPE_OUTLINE,
    D2D_SHAPE_TYPE_BEZIER_OUTLINE,
    D2D_SHAPE_TYPE_ARC_OUTLINE,
    D2D_SHAPE_TYPE_TRIANGLE,
    D2D_SHAPE_TYPE_CURVE,
    D2D_SHAPE_TYPE_COUNT,
};

struct d2d_settings
{
    unsigned int max_version_factory;
};
extern struct d2d_settings d2d_settings DECLSPEC_HIDDEN;

struct d2d_clip_stack
{
    D2D1_RECT_F *stack;
    size_t size;
    size_t count;
};

struct d2d_error_state
{
    HRESULT code;
    D2D1_TAG tag1, tag2;
};

struct d2d_shape_resources
{
    ID3D11InputLayout *il;
    ID3D11VertexShader *vs;
};

struct d2d_brush_cb
{
    enum d2d_brush_type type;
    float opacity;
    unsigned int pad[2];
    union
    {
        struct
        {
            D2D1_COLOR_F colour;
        } solid;
        struct
        {
            D2D1_POINT_2F start;
            D2D1_POINT_2F end;
            unsigned int stop_count;
        } linear;
        struct
        {
            D2D1_POINT_2F centre;
            D2D1_POINT_2F offset;
            D2D1_POINT_2F ra;
            D2D1_POINT_2F rb;
            unsigned int stop_count;
            float pad[3];
        } radial;
        struct
        {
            float _11, _21, _31, pad;
            float _12, _22, _32;
            BOOL ignore_alpha;
        } bitmap;
    } u;
};

struct d2d_ps_cb
{
    BOOL outline;
    BOOL is_arc;
    BOOL pad[2];
    struct d2d_brush_cb colour_brush;
    struct d2d_brush_cb opacity_brush;
};

struct d2d_vec4
{
    float x, y, z, w;
};

struct d2d_vs_cb
{
    struct
    {
        float _11, _21, _31, pad0;
        float _12, _22, _32, stroke_width;
    } transform_geometry;
    struct d2d_vec4 transform_rtx;
    struct d2d_vec4 transform_rty;
};

struct d2d_device_context_ops
{
    HRESULT (*device_context_present)(IUnknown *outer_unknown);
};

enum d2d_device_context_sampler_limits
{
    D2D_SAMPLER_INTERPOLATION_MODE_COUNT = 2,
    D2D_SAMPLER_EXTEND_MODE_COUNT = 3,
};

struct d2d_device_context
{
    ID2D1DeviceContext ID2D1DeviceContext_iface;
    ID2D1GdiInteropRenderTarget ID2D1GdiInteropRenderTarget_iface;
    IDWriteTextRenderer IDWriteTextRenderer_iface;
    IUnknown IUnknown_iface;
    LONG refcount;

    IUnknown *outer_unknown;
    const struct d2d_device_context_ops *ops;

    ID2D1Factory *factory;
    ID2D1Device *device;
    ID3D11Device1 *d3d_device;
    ID3DDeviceContextState *d3d_state;
    struct d2d_bitmap *target;
    struct d2d_shape_resources shape_resources[D2D_SHAPE_TYPE_COUNT];
    ID3D11Buffer *vs_cb;
    ID3D11PixelShader *ps;
    ID3D11Buffer *ps_cb;
    ID3D11Buffer *ib;
    unsigned int vb_stride;
    ID3D11Buffer *vb;
    ID3D11RasterizerState *rs;
    ID3D11BlendState *bs;
    ID3D11SamplerState *sampler_states
            [D2D_SAMPLER_INTERPOLATION_MODE_COUNT]
            [D2D_SAMPLER_EXTEND_MODE_COUNT]
            [D2D_SAMPLER_EXTEND_MODE_COUNT];

    struct d2d_error_state error;
    D2D1_DRAWING_STATE_DESCRIPTION1 drawing_state;
    IDWriteRenderingParams *text_rendering_params;
    IDWriteRenderingParams *default_text_rendering_params;

    D2D1_RENDER_TARGET_PROPERTIES desc;
    D2D1_SIZE_U pixel_size;
    struct d2d_clip_stack clip_stack;
};

HRESULT d2d_d3d_create_render_target(ID2D1Device *device, IDXGISurface *surface, IUnknown *outer_unknown,
        const struct d2d_device_context_ops *ops, const D2D1_RENDER_TARGET_PROPERTIES *desc,
        void **render_target) DECLSPEC_HIDDEN;

static inline BOOL d2d_device_context_is_dxgi_target(const struct d2d_device_context *context)
{
    return !context->ops;
}

struct d2d_wic_render_target
{
    IUnknown IUnknown_iface;
    LONG refcount;

    IDXGISurface *dxgi_surface;
    ID2D1RenderTarget *dxgi_target;
    IUnknown *dxgi_inner;
    ID3D10Texture2D *readback_texture;
    IWICBitmap *bitmap;

    unsigned int width;
    unsigned int height;
    unsigned int bpp;
};

HRESULT d2d_wic_render_target_init(struct d2d_wic_render_target *render_target, ID2D1Factory1 *factory,
        ID3D10Device1 *d3d_device, IWICBitmap *bitmap, const D2D1_RENDER_TARGET_PROPERTIES *desc) DECLSPEC_HIDDEN;

struct d2d_dc_render_target
{
    ID2D1DCRenderTarget ID2D1DCRenderTarget_iface;
    LONG refcount;

    IDXGISurface1 *dxgi_surface;
    D2D1_PIXEL_FORMAT pixel_format;
    ID3D10Device1 *d3d_device;
    ID2D1RenderTarget *dxgi_target;
    IUnknown *dxgi_inner;

    RECT dst_rect;
    HDC hdc;
};

HRESULT d2d_dc_render_target_init(struct d2d_dc_render_target *render_target, ID2D1Factory1 *factory,
        ID3D10Device1 *d3d_device, const D2D1_RENDER_TARGET_PROPERTIES *desc) DECLSPEC_HIDDEN;

struct d2d_hwnd_render_target
{
    ID2D1HwndRenderTarget ID2D1HwndRenderTarget_iface;
    LONG refcount;

    ID2D1RenderTarget *dxgi_target;
    IUnknown *dxgi_inner;
    IDXGISwapChain *swapchain;
    UINT sync_interval;
    HWND hwnd;
};

HRESULT d2d_hwnd_render_target_init(struct d2d_hwnd_render_target *render_target, ID2D1Factory1 *factory,
        ID3D10Device1 *d3d_device, const D2D1_RENDER_TARGET_PROPERTIES *desc,
        const D2D1_HWND_RENDER_TARGET_PROPERTIES *hwnd_desc) DECLSPEC_HIDDEN;

struct d2d_bitmap_render_target
{
    ID2D1BitmapRenderTarget ID2D1BitmapRenderTarget_iface;
    LONG refcount;

    ID2D1RenderTarget *dxgi_target;
    IUnknown *dxgi_inner;
    ID2D1Bitmap *bitmap;
};

HRESULT d2d_bitmap_render_target_init(struct d2d_bitmap_render_target *render_target,
        const struct d2d_device_context *parent_target, const D2D1_SIZE_F *size,
        const D2D1_SIZE_U *pixel_size, const D2D1_PIXEL_FORMAT *format,
        D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS options) DECLSPEC_HIDDEN;

struct d2d_gradient
{
    ID2D1GradientStopCollection ID2D1GradientStopCollection_iface;
    LONG refcount;

    ID2D1Factory *factory;
    ID3D11ShaderResourceView *view;
    D2D1_GRADIENT_STOP *stops;
    UINT32 stop_count;
};

HRESULT d2d_gradient_create(ID2D1Factory *factory, ID3D11Device1 *device, const D2D1_GRADIENT_STOP *stops,
        UINT32 stop_count, D2D1_GAMMA gamma, D2D1_EXTEND_MODE extend_mode,
        struct d2d_gradient **gradient) DECLSPEC_HIDDEN;

struct d2d_brush
{
    ID2D1Brush ID2D1Brush_iface;
    LONG refcount;

    ID2D1Factory *factory;
    float opacity;
    D2D1_MATRIX_3X2_F transform;

    enum d2d_brush_type type;
    union
    {
        struct
        {
            D2D1_COLOR_F color;
        } solid;
        struct
        {
            struct d2d_gradient *gradient;
            D2D1_POINT_2F start;
            D2D1_POINT_2F end;
        } linear;
        struct
        {
            struct d2d_gradient *gradient;
            D2D1_POINT_2F centre;
            D2D1_POINT_2F offset;
            D2D1_POINT_2F radius;
        } radial;
        struct
        {
            struct d2d_bitmap *bitmap;
            D2D1_EXTEND_MODE extend_mode_x;
            D2D1_EXTEND_MODE extend_mode_y;
            D2D1_INTERPOLATION_MODE interpolation_mode;
        } bitmap;
    } u;
};

HRESULT d2d_solid_color_brush_create(ID2D1Factory *factory, const D2D1_COLOR_F *color,
        const D2D1_BRUSH_PROPERTIES *desc, struct d2d_brush **brush) DECLSPEC_HIDDEN;
HRESULT d2d_linear_gradient_brush_create(ID2D1Factory *factory,
        const D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES *gradient_desc, const D2D1_BRUSH_PROPERTIES *brush_desc,
        ID2D1GradientStopCollection *gradient, struct d2d_brush **brush) DECLSPEC_HIDDEN;
HRESULT d2d_radial_gradient_brush_create(ID2D1Factory *factory,
        const D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES *gradient_desc, const D2D1_BRUSH_PROPERTIES *brush_desc,
        ID2D1GradientStopCollection *gradient, struct d2d_brush **brush) DECLSPEC_HIDDEN;
HRESULT d2d_bitmap_brush_create(ID2D1Factory *factory, ID2D1Bitmap *bitmap,
        const D2D1_BITMAP_BRUSH_PROPERTIES1 *bitmap_brush_desc, const D2D1_BRUSH_PROPERTIES *brush_desc,
        struct d2d_brush **brush) DECLSPEC_HIDDEN;
void d2d_brush_bind_resources(struct d2d_brush *brush, struct d2d_device_context *context,
        unsigned int brush_idx) DECLSPEC_HIDDEN;
BOOL d2d_brush_fill_cb(const struct d2d_brush *brush, struct d2d_brush_cb *cb) DECLSPEC_HIDDEN;
struct d2d_brush *unsafe_impl_from_ID2D1Brush(ID2D1Brush *iface) DECLSPEC_HIDDEN;

struct d2d_stroke_style
{
    ID2D1StrokeStyle ID2D1StrokeStyle_iface;
    LONG refcount;

    ID2D1Factory *factory;
    D2D1_STROKE_STYLE_PROPERTIES desc;
    float *dashes;
    UINT32 dash_count;
};

HRESULT d2d_stroke_style_init(struct d2d_stroke_style *style, ID2D1Factory *factory,
        const D2D1_STROKE_STYLE_PROPERTIES *desc, const float *dashes, UINT32 dash_count) DECLSPEC_HIDDEN;

struct d2d_layer
{
    ID2D1Layer ID2D1Layer_iface;
    LONG refcount;

    ID2D1Factory *factory;
    D2D1_SIZE_F size;
};

HRESULT d2d_layer_create(ID2D1Factory *factory, const D2D1_SIZE_F *size, struct d2d_layer **layer) DECLSPEC_HIDDEN;

struct d2d_mesh
{
    ID2D1Mesh ID2D1Mesh_iface;
    LONG refcount;

    ID2D1Factory *factory;
};

HRESULT d2d_mesh_create(ID2D1Factory *factory, struct d2d_mesh **mesh) DECLSPEC_HIDDEN;

struct d2d_bitmap
{
    ID2D1Bitmap1 ID2D1Bitmap1_iface;
    LONG refcount;

    ID2D1Factory *factory;
    ID3D11ShaderResourceView *srv;
    ID3D11RenderTargetView *rtv;
    IDXGISurface *surface;
    ID3D11Resource *resource;
    D2D1_SIZE_U pixel_size;
    D2D1_PIXEL_FORMAT format;
    float dpi_x;
    float dpi_y;
    D2D1_BITMAP_OPTIONS options;
};

HRESULT d2d_bitmap_create(struct d2d_device_context *context, D2D1_SIZE_U size, const void *src_data,
        UINT32 pitch, const D2D1_BITMAP_PROPERTIES1 *desc, struct d2d_bitmap **bitmap) DECLSPEC_HIDDEN;
HRESULT d2d_bitmap_create_shared(struct d2d_device_context *context, REFIID iid, void *data,
        const D2D1_BITMAP_PROPERTIES1 *desc, struct d2d_bitmap **bitmap) DECLSPEC_HIDDEN;
HRESULT d2d_bitmap_create_from_wic_bitmap(struct d2d_device_context *context, IWICBitmapSource *bitmap_source,
        const D2D1_BITMAP_PROPERTIES1 *desc, struct d2d_bitmap **bitmap) DECLSPEC_HIDDEN;
struct d2d_bitmap *unsafe_impl_from_ID2D1Bitmap(ID2D1Bitmap *iface) DECLSPEC_HIDDEN;

struct d2d_state_block
{
    ID2D1DrawingStateBlock1 ID2D1DrawingStateBlock1_iface;
    LONG refcount;

    ID2D1Factory *factory;
    D2D1_DRAWING_STATE_DESCRIPTION1 drawing_state;
    IDWriteRenderingParams *text_rendering_params;
};

void d2d_state_block_init(struct d2d_state_block *state_block, ID2D1Factory *factory,
        const D2D1_DRAWING_STATE_DESCRIPTION1 *desc, IDWriteRenderingParams *text_rendering_params) DECLSPEC_HIDDEN;
struct d2d_state_block *unsafe_impl_from_ID2D1DrawingStateBlock(ID2D1DrawingStateBlock *iface) DECLSPEC_HIDDEN;

enum d2d_geometry_state
{
    D2D_GEOMETRY_STATE_INITIAL = 0,
    D2D_GEOMETRY_STATE_ERROR,
    D2D_GEOMETRY_STATE_OPEN,
    D2D_GEOMETRY_STATE_CLOSED,
    D2D_GEOMETRY_STATE_FIGURE,
};

struct d2d_curve_vertex
{
    D2D1_POINT_2F position;
    struct
    {
        float u, v, sign;
    } texcoord;
};

struct d2d_face
{
    UINT16 v[3];
};

struct d2d_outline_vertex
{
    D2D1_POINT_2F position;
    D2D1_POINT_2F prev;
    D2D1_POINT_2F next;
};

struct d2d_curve_outline_vertex
{
    D2D1_POINT_2F position;
    D2D1_POINT_2F p0, p1, p2;
    D2D1_POINT_2F prev, next;
};

struct d2d_geometry
{
    ID2D1Geometry ID2D1Geometry_iface;
    LONG refcount;

    ID2D1Factory *factory;

    D2D_MATRIX_3X2_F transform;

    struct
    {
        D2D1_POINT_2F *vertices;
        size_t vertex_count;

        struct d2d_face *faces;
        size_t faces_size;
        size_t face_count;

        struct d2d_curve_vertex *bezier_vertices;
        size_t bezier_vertices_size;
        size_t bezier_vertex_count;

        struct d2d_curve_vertex *arc_vertices;
        size_t arc_vertices_size;
        size_t arc_vertex_count;
    } fill;

    struct
    {
        struct d2d_outline_vertex *vertices;
        size_t vertices_size;
        size_t vertex_count;

        struct d2d_face *faces;
        size_t faces_size;
        size_t face_count;

        struct d2d_curve_outline_vertex *beziers;
        size_t beziers_size;
        size_t bezier_count;

        struct d2d_face *bezier_faces;
        size_t bezier_faces_size;
        size_t bezier_face_count;

        struct d2d_curve_outline_vertex *arcs;
        size_t arcs_size;
        size_t arc_count;

        struct d2d_face *arc_faces;
        size_t arc_faces_size;
        size_t arc_face_count;
    } outline;

    union
    {
        struct
        {
            D2D1_ELLIPSE ellipse;
        } ellipse;
        struct
        {
            ID2D1GeometrySink ID2D1GeometrySink_iface;

            struct d2d_figure *figures;
            size_t figures_size;
            size_t figure_count;

            enum d2d_geometry_state state;
            D2D1_FILL_MODE fill_mode;
            UINT32 segment_count;

            D2D1_RECT_F bounds;
        } path;
        struct
        {
            D2D1_RECT_F rect;
        } rectangle;
        struct
        {
            D2D1_ROUNDED_RECT rounded_rect;
        } rounded_rectangle;
        struct
        {
            ID2D1Geometry *src_geometry;
            D2D_MATRIX_3X2_F transform;
        } transformed;
        struct
        {
            ID2D1Geometry **src_geometries;
            UINT32 geometry_count;
            D2D1_FILL_MODE fill_mode;
        } group;
    } u;
};

HRESULT d2d_ellipse_geometry_init(struct d2d_geometry *geometry,
        ID2D1Factory *factory, const D2D1_ELLIPSE *ellipse) DECLSPEC_HIDDEN;
void d2d_path_geometry_init(struct d2d_geometry *geometry, ID2D1Factory *factory) DECLSPEC_HIDDEN;
HRESULT d2d_rectangle_geometry_init(struct d2d_geometry *geometry,
        ID2D1Factory *factory, const D2D1_RECT_F *rect) DECLSPEC_HIDDEN;
HRESULT d2d_rounded_rectangle_geometry_init(struct d2d_geometry *geometry,
        ID2D1Factory *factory, const D2D1_ROUNDED_RECT *rounded_rect) DECLSPEC_HIDDEN;
void d2d_transformed_geometry_init(struct d2d_geometry *geometry, ID2D1Factory *factory,
        ID2D1Geometry *src_geometry, const D2D_MATRIX_3X2_F *transform) DECLSPEC_HIDDEN;
HRESULT d2d_geometry_group_init(struct d2d_geometry *geometry, ID2D1Factory *factory,
        D2D1_FILL_MODE fill_mode, ID2D1Geometry **src_geometries, unsigned int geometry_count) DECLSPEC_HIDDEN;
struct d2d_geometry *unsafe_impl_from_ID2D1Geometry(ID2D1Geometry *iface) DECLSPEC_HIDDEN;

struct d2d_device
{
    ID2D1Device ID2D1Device_iface;
    LONG refcount;
    ID2D1Factory1 *factory;
    IDXGIDevice *dxgi_device;
};

void d2d_device_init(struct d2d_device *device, ID2D1Factory1 *factory, IDXGIDevice *dxgi_device) DECLSPEC_HIDDEN;

struct d2d_effect
{
    ID2D1Effect ID2D1Effect_iface;
    ID2D1Image ID2D1Image_iface;
    LONG refcount;

    ID2D1Factory *factory;
};

void d2d_effect_init(struct d2d_effect *effect, ID2D1Factory *factory) DECLSPEC_HIDDEN;

static inline BOOL d2d_array_reserve(void **elements, size_t *capacity, size_t count, size_t size)
{
    size_t new_capacity, max_capacity;
    void *new_elements;

    if (count <= *capacity)
        return TRUE;

    max_capacity = ~(SIZE_T)0 / size;
    if (count > max_capacity)
        return FALSE;

    new_capacity = max(4, *capacity);
    while (new_capacity < count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;
    if (new_capacity < count)
        new_capacity = max_capacity;

    if (!(new_elements = heap_realloc(*elements, new_capacity * size)))
        return FALSE;

    *elements = new_elements;
    *capacity = new_capacity;
    return TRUE;
}

static inline void d2d_matrix_multiply(D2D_MATRIX_3X2_F *a, const D2D_MATRIX_3X2_F *b)
{
    D2D_MATRIX_3X2_F tmp = *a;

    a->_11 = tmp._11 * b->_11 + tmp._12 * b->_21;
    a->_12 = tmp._11 * b->_12 + tmp._12 * b->_22;
    a->_21 = tmp._21 * b->_11 + tmp._22 * b->_21;
    a->_22 = tmp._21 * b->_12 + tmp._22 * b->_22;
    a->_31 = tmp._31 * b->_11 + tmp._32 * b->_21 + b->_31;
    a->_32 = tmp._31 * b->_12 + tmp._32 * b->_22 + b->_32;
}

/* Dst must be different from src. */
static inline BOOL d2d_matrix_invert(D2D_MATRIX_3X2_F *dst, const D2D_MATRIX_3X2_F *src)
{
    float d = src->_11 * src->_22 - src->_21 * src->_12;

    if (d == 0.0f)
        return FALSE;
    dst->_11 = src->_22 / d;
    dst->_21 = -src->_21 / d;
    dst->_31 = (src->_21 * src->_32 - src->_31 * src->_22) / d;
    dst->_12 = -src->_12 / d;
    dst->_22 = src->_11 / d;
    dst->_32 = -(src->_11 * src->_32 - src->_31 * src->_12) / d;

    return TRUE;
}

static inline void d2d_point_set(D2D1_POINT_2F *dst, float x, float y)
{
    dst->x = x;
    dst->y = y;
}

static inline float d2d_point_dot(const D2D1_POINT_2F *p0, const D2D1_POINT_2F *p1)
{
    return p0->x * p1->x + p0->y * p1->y;
}

static inline void d2d_point_transform(D2D1_POINT_2F *dst, const D2D1_MATRIX_3X2_F *matrix, float x, float y)
{
    dst->x = x * matrix->_11 + y * matrix->_21 + matrix->_31;
    dst->y = x * matrix->_12 + y * matrix->_22 + matrix->_32;
}

static inline void d2d_rect_expand(D2D1_RECT_F *dst, const D2D1_POINT_2F *point)
{
    if (point->x < dst->left)
        dst->left = point->x;
    if (point->x > dst->right)
        dst->right = point->x;
    if (point->y < dst->top)
        dst->top = point->y;
    if (point->y > dst->bottom)
        dst->bottom = point->y;
}

static inline D2D1_INTERPOLATION_MODE d2d1_1_interp_mode_from_d2d1(D2D1_BITMAP_INTERPOLATION_MODE mode)
{
    return (D2D1_INTERPOLATION_MODE)mode;
}

static inline const char *debug_d2d_color_f(const D2D1_COLOR_F *colour)
{
    if (!colour)
        return "(null)";
    return wine_dbg_sprintf("{%.8e, %.8e, %.8e, %.8e}", colour->r, colour->g, colour->b, colour->a);
}

static inline const char *debug_d2d_point_2f(const D2D1_POINT_2F *point)
{
    if (!point)
        return "(null)";
    return wine_dbg_sprintf("{%.8e, %.8e}", point->x, point->y);
}

static inline const char *debug_d2d_rect_f(const D2D1_RECT_F *rect)
{
    if (!rect)
        return "(null)";
    return wine_dbg_sprintf("(%.8e, %.8e)-(%.8e, %.8e)", rect->left, rect->top, rect->right, rect->bottom);
}

static inline const char *debug_d2d_rounded_rect(const D2D1_ROUNDED_RECT *rounded_rect)
{
    if (!rounded_rect)
        return "(null)";
    return wine_dbg_sprintf("(%.8e, %.8e)-(%.8e, %.8e)[%.8e, %.8e]", rounded_rect->rect.left, rounded_rect->rect.top,
            rounded_rect->rect.right, rounded_rect->rect.bottom, rounded_rect->radiusX, rounded_rect->radiusY);
}

static inline const char *debug_d2d_ellipse(const D2D1_ELLIPSE *ellipse)
{
    if (!ellipse)
        return "(null)";
    return wine_dbg_sprintf("(%.8e, %.8e)[%.8e, %.8e]",
            ellipse->point.x, ellipse->point.y, ellipse->radiusX, ellipse->radiusY);
}

#endif /* __WINE_D2D1_PRIVATE_H */
