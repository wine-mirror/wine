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
};

struct d2d_clip_stack
{
    D2D1_RECT_F *stack;
    unsigned int size;
    unsigned int count;
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
    ID3D10InputLayout *il;
    unsigned int vb_stride;
    ID3D10Buffer *vb;
    ID3D10VertexShader *vs;
    ID3D10RasterizerState *rs;
    ID3D10BlendState *bs;

    ID3D10PixelShader *rect_solid_ps;
    ID3D10PixelShader *rect_bitmap_ps;

    IDWriteRenderingParams *text_rendering_params;

    D2D1_SIZE_U pixel_size;
    D2D1_MATRIX_3X2_F transform;
    struct d2d_clip_stack clip_stack;
    float dpi_x;
    float dpi_y;
};

HRESULT d2d_d3d_render_target_init(struct d2d_d3d_render_target *render_target, ID2D1Factory *factory,
        IDXGISurface *surface, const D2D1_RENDER_TARGET_PROPERTIES *desc) DECLSPEC_HIDDEN;

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
        IWICBitmap *bitmap, const D2D1_RENDER_TARGET_PROPERTIES *desc) DECLSPEC_HIDDEN;

struct d2d_gradient
{
    ID2D1GradientStopCollection ID2D1GradientStopCollection_iface;
    LONG refcount;

    D2D1_GRADIENT_STOP *stops;
    UINT32 stop_count;
};

HRESULT d2d_gradient_init(struct d2d_gradient *gradient, ID2D1RenderTarget *render_target,
        const D2D1_GRADIENT_STOP *stops, UINT32 stop_count, D2D1_GAMMA gamma,
        D2D1_EXTEND_MODE extend_mode) DECLSPEC_HIDDEN;

struct d2d_brush
{
    ID2D1Brush ID2D1Brush_iface;
    LONG refcount;

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
            ID3D10SamplerState *sampler_state;
        } bitmap;
    } u;
};

void d2d_solid_color_brush_init(struct d2d_brush *brush, ID2D1RenderTarget *render_target,
        const D2D1_COLOR_F *color, const D2D1_BRUSH_PROPERTIES *desc) DECLSPEC_HIDDEN;
void d2d_linear_gradient_brush_init(struct d2d_brush *brush, ID2D1RenderTarget *render_target,
        const D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES *gradient_brush_desc, const D2D1_BRUSH_PROPERTIES *brush_desc,
        ID2D1GradientStopCollection *gradient) DECLSPEC_HIDDEN;
HRESULT d2d_bitmap_brush_init(struct d2d_brush *brush, struct d2d_d3d_render_target *render_target,
        ID2D1Bitmap *bitmap, const D2D1_BITMAP_BRUSH_PROPERTIES *bitmap_brush_desc,
        const D2D1_BRUSH_PROPERTIES *brush_desc) DECLSPEC_HIDDEN;
void d2d_brush_bind_resources(struct d2d_brush *brush, ID3D10Device *device) DECLSPEC_HIDDEN;
struct d2d_brush *unsafe_impl_from_ID2D1Brush(ID2D1Brush *iface) DECLSPEC_HIDDEN;

struct d2d_stroke_style
{
    ID2D1StrokeStyle ID2D1StrokeStyle_iface;
    LONG refcount;
};

void d2d_stroke_style_init(struct d2d_stroke_style *style, ID2D1Factory *factory,
        const D2D1_STROKE_STYLE_PROPERTIES *desc, const float *dashes, UINT32 dash_count) DECLSPEC_HIDDEN;

struct d2d_mesh
{
    ID2D1Mesh ID2D1Mesh_iface;
    LONG refcount;
};

void d2d_mesh_init(struct d2d_mesh *mesh) DECLSPEC_HIDDEN;

struct d2d_bitmap
{
    ID2D1Bitmap ID2D1Bitmap_iface;
    LONG refcount;

    ID3D10ShaderResourceView *view;
    D2D1_SIZE_U pixel_size;
    float dpi_x;
    float dpi_y;
};

HRESULT d2d_bitmap_init(struct d2d_bitmap *bitmap, struct d2d_d3d_render_target *render_target,
        D2D1_SIZE_U size, const void *src_data, UINT32 pitch, const D2D1_BITMAP_PROPERTIES *desc) DECLSPEC_HIDDEN;
struct d2d_bitmap *unsafe_impl_from_ID2D1Bitmap(ID2D1Bitmap *iface) DECLSPEC_HIDDEN;

struct d2d_state_block
{
    ID2D1DrawingStateBlock ID2D1DrawingStateBlock_iface;
    LONG refcount;

    D2D1_DRAWING_STATE_DESCRIPTION drawing_state;
    IDWriteRenderingParams *text_rendering_params;
};

void d2d_state_block_init(struct d2d_state_block *state_block, const D2D1_DRAWING_STATE_DESCRIPTION *desc,
        IDWriteRenderingParams *text_rendering_params) DECLSPEC_HIDDEN;

#endif /* __WINE_D2D1_PRIVATE_H */
