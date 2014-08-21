/*
 * Copyright 2008-2009 Henri Verbeet for CodeWeavers
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

#ifndef __WINE_D3D10CORE_PRIVATE_H
#define __WINE_D3D10CORE_PRIVATE_H

#include "wine/debug.h"

#include <assert.h>

#define COBJMACROS
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "objbase.h"

#include "d3d10_1.h"
#ifdef D3D10CORE_INIT_GUID
#include "initguid.h"
#endif
#include "wine/wined3d.h"
#include "wine/winedxgi.h"
#include "wine/rbtree.h"

#define MAKE_TAG(ch0, ch1, ch2, ch3) \
    ((DWORD)(ch0) | ((DWORD)(ch1) << 8) | \
    ((DWORD)(ch2) << 16) | ((DWORD)(ch3) << 24 ))
#define TAG_DXBC MAKE_TAG('D', 'X', 'B', 'C')
#define TAG_ISGN MAKE_TAG('I', 'S', 'G', 'N')
#define TAG_OSGN MAKE_TAG('O', 'S', 'G', 'N')
#define TAG_SHDR MAKE_TAG('S', 'H', 'D', 'R')

struct d3d10_device;

struct d3d10_shader_info
{
    const DWORD *shader_code;
    struct wined3d_shader_signature *output_signature;
};

extern const struct wined3d_parent_ops d3d10_null_wined3d_parent_ops DECLSPEC_HIDDEN;

/* TRACE helper functions */
const char *debug_d3d10_primitive_topology(D3D10_PRIMITIVE_TOPOLOGY topology) DECLSPEC_HIDDEN;
const char *debug_dxgi_format(DXGI_FORMAT format) DECLSPEC_HIDDEN;

DXGI_FORMAT dxgi_format_from_wined3dformat(enum wined3d_format_id format) DECLSPEC_HIDDEN;
enum wined3d_format_id wined3dformat_from_dxgi_format(DXGI_FORMAT format) DECLSPEC_HIDDEN;
DWORD wined3d_usage_from_d3d10core(UINT bind_flags, enum D3D10_USAGE usage) DECLSPEC_HIDDEN;
struct wined3d_resource *wined3d_resource_from_resource(ID3D10Resource *resource) DECLSPEC_HIDDEN;

static inline void read_dword(const char **ptr, DWORD *d)
{
    memcpy(d, *ptr, sizeof(*d));
    *ptr += sizeof(*d);
}

void skip_dword_unknown(const char **ptr, unsigned int count) DECLSPEC_HIDDEN;

HRESULT parse_dxbc(const char *data, SIZE_T data_size,
        HRESULT (*chunk_handler)(const char *data, DWORD data_size, DWORD tag, void *ctx), void *ctx) DECLSPEC_HIDDEN;

/* ID3D10Texture2D */
struct d3d10_texture2d
{
    ID3D10Texture2D ID3D10Texture2D_iface;
    LONG refcount;

    IUnknown *dxgi_surface;
    struct wined3d_texture *wined3d_texture;
    D3D10_TEXTURE2D_DESC desc;
    ID3D10Device1 *device;
};

HRESULT d3d10_texture2d_init(struct d3d10_texture2d *texture, struct d3d10_device *device,
        const D3D10_TEXTURE2D_DESC *desc) DECLSPEC_HIDDEN;
struct d3d10_texture2d *unsafe_impl_from_ID3D10Texture2D(ID3D10Texture2D *iface) DECLSPEC_HIDDEN;

/* ID3D10Texture3D */
struct d3d10_texture3d
{
    ID3D10Texture3D ID3D10Texture3D_iface;
    LONG refcount;

    struct wined3d_texture *wined3d_texture;
    D3D10_TEXTURE3D_DESC desc;
    ID3D10Device1 *device;
};

HRESULT d3d10_texture3d_init(struct d3d10_texture3d *texture, struct d3d10_device *device,
        const D3D10_TEXTURE3D_DESC *desc) DECLSPEC_HIDDEN;

/* ID3D10Buffer */
struct d3d10_buffer
{
    ID3D10Buffer ID3D10Buffer_iface;
    LONG refcount;

    struct wined3d_buffer *wined3d_buffer;
    ID3D10Device1 *device;
};

HRESULT d3d10_buffer_init(struct d3d10_buffer *buffer, struct d3d10_device *device,
        const D3D10_BUFFER_DESC *desc, const D3D10_SUBRESOURCE_DATA *data) DECLSPEC_HIDDEN;
struct d3d10_buffer *unsafe_impl_from_ID3D10Buffer(ID3D10Buffer *iface) DECLSPEC_HIDDEN;

/* ID3D10DepthStencilView */
struct d3d10_depthstencil_view
{
    ID3D10DepthStencilView ID3D10DepthStencilView_iface;
    LONG refcount;

    D3D10_DEPTH_STENCIL_VIEW_DESC desc;
    ID3D10Resource *resource;
    ID3D10Device1 *device;
};

HRESULT d3d10_depthstencil_view_init(struct d3d10_depthstencil_view *view, struct d3d10_device *device,
        ID3D10Resource *resource, const D3D10_DEPTH_STENCIL_VIEW_DESC *desc) DECLSPEC_HIDDEN;

/* ID3D10RenderTargetView */
struct d3d10_rendertarget_view
{
    ID3D10RenderTargetView ID3D10RenderTargetView_iface;
    LONG refcount;

    struct wined3d_rendertarget_view *wined3d_view;
    D3D10_RENDER_TARGET_VIEW_DESC desc;
    ID3D10Resource *resource;
    ID3D10Device1 *device;
};

HRESULT d3d10_rendertarget_view_init(struct d3d10_rendertarget_view *view, struct d3d10_device *device,
        ID3D10Resource *resource, const D3D10_RENDER_TARGET_VIEW_DESC *desc) DECLSPEC_HIDDEN;
struct d3d10_rendertarget_view *unsafe_impl_from_ID3D10RenderTargetView(ID3D10RenderTargetView *iface) DECLSPEC_HIDDEN;

/* ID3D10ShaderResourceView */
struct d3d10_shader_resource_view
{
    ID3D10ShaderResourceView ID3D10ShaderResourceView_iface;
    LONG refcount;

    D3D10_SHADER_RESOURCE_VIEW_DESC desc;
    ID3D10Resource *resource;
    ID3D10Device1 *device;
};

HRESULT d3d10_shader_resource_view_init(struct d3d10_shader_resource_view *view, struct d3d10_device *device,
        ID3D10Resource *resource, const D3D10_SHADER_RESOURCE_VIEW_DESC *desc) DECLSPEC_HIDDEN;

/* ID3D10InputLayout */
struct d3d10_input_layout
{
    ID3D10InputLayout ID3D10InputLayout_iface;
    LONG refcount;

    struct wined3d_vertex_declaration *wined3d_decl;
};

HRESULT d3d10_input_layout_init(struct d3d10_input_layout *layout, struct d3d10_device *device,
        const D3D10_INPUT_ELEMENT_DESC *element_descs, UINT element_count,
        const void *shader_byte_code, SIZE_T shader_byte_code_length) DECLSPEC_HIDDEN;
struct d3d10_input_layout *unsafe_impl_from_ID3D10InputLayout(ID3D10InputLayout *iface) DECLSPEC_HIDDEN;

/* ID3D10VertexShader */
struct d3d10_vertex_shader
{
    ID3D10VertexShader ID3D10VertexShader_iface;
    LONG refcount;

    struct wined3d_shader *wined3d_shader;
    struct wined3d_shader_signature output_signature;
    ID3D10Device1 *device;
};

HRESULT d3d10_vertex_shader_init(struct d3d10_vertex_shader *shader, struct d3d10_device *device,
        const void *byte_code, SIZE_T byte_code_length) DECLSPEC_HIDDEN;
struct d3d10_vertex_shader *unsafe_impl_from_ID3D10VertexShader(ID3D10VertexShader *iface) DECLSPEC_HIDDEN;

/* ID3D10GeometryShader */
struct d3d10_geometry_shader
{
    ID3D10GeometryShader ID3D10GeometryShader_iface;
    LONG refcount;

    struct wined3d_shader *wined3d_shader;
    struct wined3d_shader_signature output_signature;
};

HRESULT d3d10_geometry_shader_init(struct d3d10_geometry_shader *shader, struct d3d10_device *device,
        const void *byte_code, SIZE_T byte_code_length) DECLSPEC_HIDDEN;
struct d3d10_geometry_shader *unsafe_impl_from_ID3D10GeometryShader(ID3D10GeometryShader *iface) DECLSPEC_HIDDEN;

/* ID3D10PixelShader */
struct d3d10_pixel_shader
{
    ID3D10PixelShader ID3D10PixelShader_iface;
    LONG refcount;

    struct wined3d_shader *wined3d_shader;
    struct wined3d_shader_signature output_signature;
    ID3D10Device1 *device;
};

HRESULT d3d10_pixel_shader_init(struct d3d10_pixel_shader *shader, struct d3d10_device *device,
        const void *byte_code, SIZE_T byte_code_length) DECLSPEC_HIDDEN;
struct d3d10_pixel_shader *unsafe_impl_from_ID3D10PixelShader(ID3D10PixelShader *iface) DECLSPEC_HIDDEN;

HRESULT shader_parse_signature(const char *data, DWORD data_size, struct wined3d_shader_signature *s) DECLSPEC_HIDDEN;
void shader_free_signature(struct wined3d_shader_signature *s) DECLSPEC_HIDDEN;

/* ID3D10BlendState */
struct d3d10_blend_state
{
    ID3D10BlendState ID3D10BlendState_iface;
    LONG refcount;

    D3D10_BLEND_DESC desc;
    struct wine_rb_entry entry;
    ID3D10Device1 *device;
};

HRESULT d3d10_blend_state_init(struct d3d10_blend_state *state, struct d3d10_device *device,
        const D3D10_BLEND_DESC *desc) DECLSPEC_HIDDEN;
struct d3d10_blend_state *unsafe_impl_from_ID3D10BlendState(ID3D10BlendState *iface) DECLSPEC_HIDDEN;

/* ID3D10DepthStencilState */
struct d3d10_depthstencil_state
{
    ID3D10DepthStencilState ID3D10DepthStencilState_iface;
    LONG refcount;

    D3D10_DEPTH_STENCIL_DESC desc;
    struct wine_rb_entry entry;
    ID3D10Device1 *device;
};

HRESULT d3d10_depthstencil_state_init(struct d3d10_depthstencil_state *state, struct d3d10_device *device,
        const D3D10_DEPTH_STENCIL_DESC *desc) DECLSPEC_HIDDEN;
struct d3d10_depthstencil_state *unsafe_impl_from_ID3D10DepthStencilState(
        ID3D10DepthStencilState *iface) DECLSPEC_HIDDEN;

/* ID3D10RasterizerState */
struct d3d10_rasterizer_state
{
    ID3D10RasterizerState ID3D10RasterizerState_iface;
    LONG refcount;

    D3D10_RASTERIZER_DESC desc;
    struct wine_rb_entry entry;
    ID3D10Device1 *device;
};

HRESULT d3d10_rasterizer_state_init(struct d3d10_rasterizer_state *state, struct d3d10_device *device,
        const D3D10_RASTERIZER_DESC *desc) DECLSPEC_HIDDEN;
struct d3d10_rasterizer_state *unsafe_impl_from_ID3D10RasterizerState(ID3D10RasterizerState *iface) DECLSPEC_HIDDEN;

/* ID3D10SamplerState */
struct d3d10_sampler_state
{
    ID3D10SamplerState ID3D10SamplerState_iface;
    LONG refcount;

    struct wined3d_sampler *wined3d_sampler;
    D3D10_SAMPLER_DESC desc;
    struct wine_rb_entry entry;
    ID3D10Device1 *device;
};

HRESULT d3d10_sampler_state_init(struct d3d10_sampler_state *state, struct d3d10_device *device,
        const D3D10_SAMPLER_DESC *desc) DECLSPEC_HIDDEN;
struct d3d10_sampler_state *unsafe_impl_from_ID3D10SamplerState(ID3D10SamplerState *iface) DECLSPEC_HIDDEN;

/* ID3D10Query */
struct d3d10_query
{
    ID3D10Query ID3D10Query_iface;
    LONG refcount;

    BOOL predicate;
    ID3D10Device1 *device;
};

HRESULT d3d10_query_init(struct d3d10_query *query, struct d3d10_device *device, BOOL predicate) DECLSPEC_HIDDEN;

/* IDirect3D10Device1 */
struct d3d10_device
{
    IUnknown IUnknown_inner;
    ID3D10Device1 ID3D10Device1_iface;
    ID3D10Multithread ID3D10Multithread_iface;
    IWineDXGIDeviceParent IWineDXGIDeviceParent_iface;
    IUnknown *outer_unk;
    LONG refcount;

    struct wined3d_device_parent device_parent;
    struct wined3d_device *wined3d_device;

    struct wine_rb_tree blend_states;
    struct wine_rb_tree depthstencil_states;
    struct wine_rb_tree rasterizer_states;
    struct wine_rb_tree sampler_states;

    struct d3d10_blend_state *blend_state;
    float blend_factor[4];
    UINT sample_mask;
    struct d3d10_depthstencil_state *depth_stencil_state;
    UINT stencil_ref;
    struct d3d10_rasterizer_state *rasterizer_state;
};

static inline struct d3d10_device *impl_from_ID3D10Device(ID3D10Device1 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d10_device, ID3D10Device1_iface);
}

HRESULT d3d10_device_init(struct d3d10_device *device, void *outer_unknown) DECLSPEC_HIDDEN;

/* Layered device */
enum dxgi_device_layer_id
{
    DXGI_DEVICE_LAYER_DEBUG1        = 0x8,
    DXGI_DEVICE_LAYER_THREAD_SAFE   = 0x10,
    DXGI_DEVICE_LAYER_DEBUG2        = 0x20,
    DXGI_DEVICE_LAYER_SWITCH_TO_REF = 0x30,
    DXGI_DEVICE_LAYER_D3D10_DEVICE  = 0xffffffff,
};

struct layer_get_size_args
{
    DWORD unknown0;
    DWORD unknown1;
    DWORD *unknown2;
    DWORD *unknown3;
    IDXGIAdapter *adapter;
    WORD interface_major;
    WORD interface_minor;
    WORD version_build;
    WORD version_revision;
};

struct dxgi_device_layer
{
    enum dxgi_device_layer_id id;
    HRESULT (WINAPI *init)(enum dxgi_device_layer_id id, DWORD *count, DWORD *values);
    UINT (WINAPI *get_size)(enum dxgi_device_layer_id id, struct layer_get_size_args *args, DWORD unknown0);
    HRESULT (WINAPI *create)(enum dxgi_device_layer_id id, void **layer_base, DWORD unknown0,
            void *device_object, REFIID riid, void **device_layer);
};

HRESULT WINAPI DXGID3D10CreateDevice(HMODULE d3d10core, IDXGIFactory *factory, IDXGIAdapter *adapter,
        UINT flags, void *unknown0, void **device);
HRESULT WINAPI DXGID3D10RegisterLayers(const struct dxgi_device_layer *layers, UINT layer_count);

#endif /* __WINE_D3D10CORE_PRIVATE_H */
