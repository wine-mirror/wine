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

#include <assert.h>
#include <limits.h>
#define COBJMACROS
#include "d2d1.h"
#ifdef D2D1_INIT_GUID
#include "initguid.h"
#endif
#include "dwrite.h"

enum d2d_brush_type
{
    D2D_BRUSH_TYPE_SOLID,
    D2D_BRUSH_TYPE_LINEAR,
    D2D_BRUSH_TYPE_BITMAP,
    D2D_BRUSH_TYPE_COUNT,
};

enum d2d_shape_type
{
    D2D_SHAPE_TYPE_TRIANGLE,
    D2D_SHAPE_TYPE_BEZIER,
    D2D_SHAPE_TYPE_COUNT,
};

struct d2d_clip_stack
{
    D2D1_RECT_F *stack;
    unsigned int size;
    unsigned int count;
};

struct d2d_error_state
{
    HRESULT code;
    D2D1_TAG tag1, tag2;
};

struct d2d_shape_resources
{
    ID3D10InputLayout *il;
    ID3D10VertexShader *vs;
    ID3D10PixelShader *ps[D2D_BRUSH_TYPE_COUNT][D2D_BRUSH_TYPE_COUNT + 1];
};

struct d2d_d3d_render_target
{
    ID2D1RenderTarget ID2D1RenderTarget_iface;
    IDWriteTextRenderer IDWriteTextRenderer_iface;
    LONG refcount;

    ID2D1Factory *factory;
    ID3D10Device *device;
    ID3D10RenderTargetView *view;
    ID3D10StateBlock *stateblock;
    struct d2d_shape_resources shape_resources[D2D_SHAPE_TYPE_COUNT];
    ID3D10Buffer *ib;
    unsigned int vb_stride;
    ID3D10Buffer *vb;
    ID3D10RasterizerState *rs;
    ID3D10BlendState *bs;

    struct d2d_error_state error;
    D2D1_DRAWING_STATE_DESCRIPTION drawing_state;
    IDWriteRenderingParams *text_rendering_params;
    IDWriteRenderingParams *default_text_rendering_params;

    D2D1_PIXEL_FORMAT format;
    D2D1_SIZE_U pixel_size;
    struct d2d_clip_stack clip_stack;
    float dpi_x;
    float dpi_y;
};

HRESULT d2d_d3d_render_target_init(struct d2d_d3d_render_target *render_target, ID2D1Factory *factory,
        IDXGISurface *surface, const D2D1_RENDER_TARGET_PROPERTIES *desc) DECLSPEC_HIDDEN;
HRESULT d2d_d3d_render_target_create_rtv(ID2D1RenderTarget *render_target, IDXGISurface1 *surface) DECLSPEC_HIDDEN;

struct d2d_wic_render_target
{
    ID2D1RenderTarget ID2D1RenderTarget_iface;
    LONG refcount;

    IDXGISurface *dxgi_surface;
    ID2D1RenderTarget *dxgi_target;
    ID3D10Texture2D *readback_texture;
    IWICBitmap *bitmap;

    unsigned int width;
    unsigned int height;
    unsigned int bpp;
};

HRESULT d2d_wic_render_target_init(struct d2d_wic_render_target *render_target, ID2D1Factory *factory,
        ID3D10Device1 *device, IWICBitmap *bitmap, const D2D1_RENDER_TARGET_PROPERTIES *desc) DECLSPEC_HIDDEN;

struct d2d_dc_render_target
{
    ID2D1DCRenderTarget ID2D1DCRenderTarget_iface;
    LONG refcount;

    IDXGISurface1 *dxgi_surface;
    ID2D1RenderTarget *dxgi_target;

    RECT dst_rect;
    HDC hdc;
};

HRESULT d2d_dc_render_target_init(struct d2d_dc_render_target *render_target, ID2D1Factory *factory,
        ID3D10Device1 *device, const D2D1_RENDER_TARGET_PROPERTIES *desc) DECLSPEC_HIDDEN;

struct d2d_hwnd_render_target
{
    ID2D1HwndRenderTarget ID2D1HwndRenderTarget_iface;
    LONG refcount;

    ID2D1RenderTarget *dxgi_target;
    IDXGISwapChain *swapchain;
    UINT sync_interval;
    HWND hwnd;
};

HRESULT d2d_hwnd_render_target_init(struct d2d_hwnd_render_target *render_target, ID2D1Factory *factory,
        ID3D10Device1 *device, const D2D1_RENDER_TARGET_PROPERTIES *desc,
        const D2D1_HWND_RENDER_TARGET_PROPERTIES *hwnd_desc) DECLSPEC_HIDDEN;

struct d2d_gradient
{
    ID2D1GradientStopCollection ID2D1GradientStopCollection_iface;
    LONG refcount;

    ID2D1Factory *factory;
    D2D1_GRADIENT_STOP *stops;
    UINT32 stop_count;
};

HRESULT d2d_gradient_create(ID2D1Factory *factory, const D2D1_GRADIENT_STOP *stops,
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
            struct d2d_bitmap *bitmap;
            D2D1_EXTEND_MODE extend_mode_x;
            D2D1_EXTEND_MODE extend_mode_y;
            D2D1_BITMAP_INTERPOLATION_MODE interpolation_mode;
            ID3D10SamplerState *sampler_state;
        } bitmap;
    } u;
};

HRESULT d2d_solid_color_brush_create(ID2D1Factory *factory, const D2D1_COLOR_F *color,
        const D2D1_BRUSH_PROPERTIES *desc, struct d2d_brush **brush) DECLSPEC_HIDDEN;
HRESULT d2d_linear_gradient_brush_create(ID2D1Factory *factory, const D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES *gradient_brush_desc,
        const D2D1_BRUSH_PROPERTIES *brush_desc, ID2D1GradientStopCollection *gradient,
        struct d2d_brush **brush) DECLSPEC_HIDDEN;
HRESULT d2d_bitmap_brush_create(ID2D1Factory *factory, ID2D1Bitmap *bitmap, const D2D1_BITMAP_BRUSH_PROPERTIES *bitmap_brush_desc,
        const D2D1_BRUSH_PROPERTIES *brush_desc, struct d2d_brush **brush) DECLSPEC_HIDDEN;
void d2d_brush_bind_resources(struct d2d_brush *brush, struct d2d_brush *opacity_brush,
        struct d2d_d3d_render_target *render_target, enum d2d_shape_type shape_type) DECLSPEC_HIDDEN;
HRESULT d2d_brush_get_ps_cb(struct d2d_brush *brush, struct d2d_brush *opacity_brush,
        struct d2d_d3d_render_target *render_target, ID3D10Buffer **ps_cb) DECLSPEC_HIDDEN;
struct d2d_brush *unsafe_impl_from_ID2D1Brush(ID2D1Brush *iface) DECLSPEC_HIDDEN;

struct d2d_stroke_style
{
    ID2D1StrokeStyle ID2D1StrokeStyle_iface;
    LONG refcount;

    ID2D1Factory *factory;
};

void d2d_stroke_style_init(struct d2d_stroke_style *style, ID2D1Factory *factory,
        const D2D1_STROKE_STYLE_PROPERTIES *desc, const float *dashes, UINT32 dash_count) DECLSPEC_HIDDEN;

struct d2d_mesh
{
    ID2D1Mesh ID2D1Mesh_iface;
    LONG refcount;

    ID2D1Factory *factory;
};

HRESULT d2d_mesh_create(ID2D1Factory *factory, struct d2d_mesh **mesh) DECLSPEC_HIDDEN;

struct d2d_bitmap
{
    ID2D1Bitmap ID2D1Bitmap_iface;
    LONG refcount;

    ID2D1Factory *factory;
    ID3D10ShaderResourceView *view;
    D2D1_SIZE_U pixel_size;
    D2D1_PIXEL_FORMAT format;
    float dpi_x;
    float dpi_y;
};

HRESULT d2d_bitmap_create(ID2D1Factory *factory, ID3D10Device *device, D2D1_SIZE_U size, const void *src_data,
        UINT32 pitch, const D2D1_BITMAP_PROPERTIES *desc, struct d2d_bitmap **bitmap) DECLSPEC_HIDDEN;
HRESULT d2d_bitmap_create_shared(ID2D1Factory *factory, ID3D10Device *device, REFIID iid, void *data,
        const D2D1_BITMAP_PROPERTIES *desc, struct d2d_bitmap **bitmap) DECLSPEC_HIDDEN;
HRESULT d2d_bitmap_create_from_wic_bitmap(ID2D1Factory *factory, ID3D10Device *device, IWICBitmapSource *bitmap_source,
        const D2D1_BITMAP_PROPERTIES *desc, struct d2d_bitmap **bitmap) DECLSPEC_HIDDEN;
struct d2d_bitmap *unsafe_impl_from_ID2D1Bitmap(ID2D1Bitmap *iface) DECLSPEC_HIDDEN;

struct d2d_state_block
{
    ID2D1DrawingStateBlock ID2D1DrawingStateBlock_iface;
    LONG refcount;

    ID2D1Factory *factory;
    D2D1_DRAWING_STATE_DESCRIPTION drawing_state;
    IDWriteRenderingParams *text_rendering_params;
};

void d2d_state_block_init(struct d2d_state_block *state_block, ID2D1Factory *factory,
        const D2D1_DRAWING_STATE_DESCRIPTION *desc, IDWriteRenderingParams *text_rendering_params) DECLSPEC_HIDDEN;
struct d2d_state_block *unsafe_impl_from_ID2D1DrawingStateBlock(ID2D1DrawingStateBlock *iface) DECLSPEC_HIDDEN;

enum d2d_geometry_state
{
    D2D_GEOMETRY_STATE_INITIAL = 0,
    D2D_GEOMETRY_STATE_ERROR,
    D2D_GEOMETRY_STATE_OPEN,
    D2D_GEOMETRY_STATE_CLOSED,
    D2D_GEOMETRY_STATE_FIGURE,
};

struct d2d_bezier
{
    struct
    {
        D2D1_POINT_2F position;
        struct
        {
            float u, v, sign;
        } texcoord;
    } v[3];
};

struct d2d_face
{
    UINT16 v[3];
};

struct d2d_geometry
{
    ID2D1Geometry ID2D1Geometry_iface;
    LONG refcount;

    ID2D1Factory *factory;

    D2D_MATRIX_3X2_F transform;

    D2D1_POINT_2F *vertices;
    size_t vertex_count;

    struct d2d_face *faces;
    size_t faces_size;
    size_t face_count;

    struct d2d_bezier *beziers;
    size_t bezier_count;

    union
    {
        struct
        {
            ID2D1GeometrySink ID2D1GeometrySink_iface;

            struct d2d_figure *figures;
            size_t figures_size;
            size_t figure_count;

            enum d2d_geometry_state state;
            D2D1_FILL_MODE fill_mode;
            UINT32 segment_count;
        } path;
        struct
        {
            D2D1_RECT_F rect;
        } rectangle;
        struct
        {
            ID2D1Geometry *src_geometry;
        } transformed;
    } u;
};

void d2d_path_geometry_init(struct d2d_geometry *geometry, ID2D1Factory *factory) DECLSPEC_HIDDEN;
HRESULT d2d_rectangle_geometry_init(struct d2d_geometry *geometry,
        ID2D1Factory *factory, const D2D1_RECT_F *rect) DECLSPEC_HIDDEN;
void d2d_transformed_geometry_init(struct d2d_geometry *geometry, ID2D1Factory *factory,
        ID2D1Geometry *src_geometry, const D2D_MATRIX_3X2_F *transform) DECLSPEC_HIDDEN;
struct d2d_geometry *unsafe_impl_from_ID2D1Geometry(ID2D1Geometry *iface) DECLSPEC_HIDDEN;

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

#endif /* __WINE_D2D1_PRIVATE_H */
