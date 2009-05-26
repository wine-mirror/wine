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

#define COBJMACROS
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "objbase.h"

#include "d3d10.h"
#ifdef D3D10CORE_INIT_GUID
#include "initguid.h"
#endif
#include "wine/wined3d.h"
#include "wine/winedxgi.h"

#define MAKE_TAG(ch0, ch1, ch2, ch3) \
    ((DWORD)(ch0) | ((DWORD)(ch1) << 8) | \
    ((DWORD)(ch2) << 16) | ((DWORD)(ch3) << 24 ))
#define TAG_DXBC MAKE_TAG('D', 'X', 'B', 'C')
#define TAG_ISGN MAKE_TAG('I', 'S', 'G', 'N')
#define TAG_OSGN MAKE_TAG('O', 'S', 'G', 'N')
#define TAG_SHDR MAKE_TAG('S', 'H', 'D', 'R')

struct d3d10_shader_info
{
    const DWORD *shader_code;
    struct wined3d_shader_signature *output_signature;
};

/* TRACE helper functions */
const char *debug_d3d10_primitive_topology(D3D10_PRIMITIVE_TOPOLOGY topology);
const char *debug_dxgi_format(DXGI_FORMAT format);

DXGI_FORMAT dxgi_format_from_wined3dformat(WINED3DFORMAT format);
WINED3DFORMAT wined3dformat_from_dxgi_format(DXGI_FORMAT format);

static inline void read_dword(const char **ptr, DWORD *d)
{
    memcpy(d, *ptr, sizeof(*d));
    *ptr += sizeof(*d);
}

void skip_dword_unknown(const char **ptr, unsigned int count);

static inline void read_tag(const char **ptr, DWORD *t, char t_str[5])
{
    read_dword(ptr, t);
    memcpy(t_str, t, 4);
    t_str[4] = '\0';
}

HRESULT parse_dxbc(const char *data, SIZE_T data_size,
        HRESULT (*chunk_handler)(const char *data, DWORD data_size, DWORD tag, void *ctx), void *ctx);

/* IDirect3D10Device */
extern const struct ID3D10DeviceVtbl d3d10_device_vtbl;
extern const struct IUnknownVtbl d3d10_device_inner_unknown_vtbl;
extern const struct IWineD3DDeviceParentVtbl d3d10_wined3d_device_parent_vtbl;
struct d3d10_device
{
    const struct ID3D10DeviceVtbl *vtbl;
    const struct IUnknownVtbl *inner_unknown_vtbl;
    const struct IWineD3DDeviceParentVtbl *device_parent_vtbl;
    IUnknown *outer_unknown;
    LONG refcount;

    IWineD3DDevice *wined3d_device;
};

/* ID3D10Texture2D */
extern const struct ID3D10Texture2DVtbl d3d10_texture2d_vtbl;
struct d3d10_texture2d
{
    const struct ID3D10Texture2DVtbl *vtbl;
    LONG refcount;

    IUnknown *dxgi_surface;
    IWineD3DSurface *wined3d_surface;
    D3D10_TEXTURE2D_DESC desc;
};

/* ID3D10Buffer */
extern const struct ID3D10BufferVtbl d3d10_buffer_vtbl;
struct d3d10_buffer
{
    const struct ID3D10BufferVtbl *vtbl;
    LONG refcount;

    IWineD3DBuffer *wined3d_buffer;
};

/* ID3D10RenderTargetView */
extern const struct ID3D10RenderTargetViewVtbl d3d10_rendertarget_view_vtbl;
struct d3d10_rendertarget_view
{
    const struct ID3D10RenderTargetViewVtbl *vtbl;
    LONG refcount;

    IWineD3DRendertargetView *wined3d_view;
    D3D10_RENDER_TARGET_VIEW_DESC desc;
};

/* ID3D10InputLayout */
extern const struct ID3D10InputLayoutVtbl d3d10_input_layout_vtbl;
struct d3d10_input_layout
{
    const struct ID3D10InputLayoutVtbl *vtbl;
    LONG refcount;

    IWineD3DVertexDeclaration *wined3d_decl;
};

HRESULT d3d10_input_layout_to_wined3d_declaration(const D3D10_INPUT_ELEMENT_DESC *element_descs,
        UINT element_count, const void *shader_byte_code, SIZE_T shader_byte_code_length,
        WINED3DVERTEXELEMENT **wined3d_elements, UINT *wined3d_element_count);

/* ID3D10VertexShader */
extern const struct ID3D10VertexShaderVtbl d3d10_vertex_shader_vtbl;
struct d3d10_vertex_shader
{
    const struct ID3D10VertexShaderVtbl *vtbl;
    LONG refcount;

    IWineD3DVertexShader *wined3d_shader;
    struct wined3d_shader_signature output_signature;
};

/* ID3D10GeometryShader */
extern const struct ID3D10GeometryShaderVtbl d3d10_geometry_shader_vtbl;
struct d3d10_geometry_shader
{
    const struct ID3D10GeometryShaderVtbl *vtbl;
    LONG refcount;
};

/* ID3D10PixelShader */
extern const struct ID3D10PixelShaderVtbl d3d10_pixel_shader_vtbl;
struct d3d10_pixel_shader
{
    const struct ID3D10PixelShaderVtbl *vtbl;
    LONG refcount;

    IWineD3DPixelShader *wined3d_shader;
    struct wined3d_shader_signature output_signature;
};

HRESULT shader_extract_from_dxbc(const void *dxbc, SIZE_T dxbc_length, struct d3d10_shader_info *shader_info);
HRESULT shader_parse_signature(const char *data, DWORD data_size, struct wined3d_shader_signature *s);
void shader_free_signature(struct wined3d_shader_signature *s);

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
        UINT flags, DWORD unknown0, void **device);
HRESULT WINAPI DXGID3D10RegisterLayers(const struct dxgi_device_layer *layers, UINT layer_count);

#endif /* __WINE_D3D10CORE_PRIVATE_H */
