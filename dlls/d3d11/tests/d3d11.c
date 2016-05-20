/*
 * Copyright 2008 Henri Verbeet for CodeWeavers
 * Copyright 2015 Józef Kucia for CodeWeavers
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

#include <assert.h>
#define COBJMACROS
#include "initguid.h"
#include "d3d11.h"
#include "wine/test.h"
#include <limits.h>

#define SWAPCHAIN_FLAG_SHADER_INPUT             0x1

static const D3D_FEATURE_LEVEL d3d11_feature_levels[] =
{
    D3D_FEATURE_LEVEL_11_1,
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_1,
    D3D_FEATURE_LEVEL_10_0,
    D3D_FEATURE_LEVEL_9_3,
    D3D_FEATURE_LEVEL_9_2,
    D3D_FEATURE_LEVEL_9_1
};

struct vec2
{
    float x, y;
};

struct vec3
{
    float x, y, z;
};

struct vec4
{
    float x, y, z, w;
};

struct device_desc
{
    const D3D_FEATURE_LEVEL *feature_level;
    UINT flags;
};

struct swapchain_desc
{
    BOOL windowed;
    UINT buffer_count;
    DXGI_SWAP_EFFECT swap_effect;
    DWORD flags;
};

static void set_box(D3D11_BOX *box, UINT left, UINT top, UINT front, UINT right, UINT bottom, UINT back)
{
    box->left = left;
    box->top = top;
    box->front = front;
    box->right = right;
    box->bottom = bottom;
    box->back = back;
}

static ULONG get_refcount(IUnknown *iface)
{
    IUnknown_AddRef(iface);
    return IUnknown_Release(iface);
}

static BOOL compare_float(float f, float g, unsigned int ulps)
{
    int x = *(int *)&f;
    int y = *(int *)&g;

    if (x < 0)
        x = INT_MIN - x;
    if (y < 0)
        y = INT_MIN - y;

    if (abs(x - y) > ulps)
        return FALSE;

    return TRUE;
}

static BOOL compare_color(DWORD c1, DWORD c2, BYTE max_diff)
{
    if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff)
        return FALSE;
    c1 >>= 8; c2 >>= 8;
    if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff)
        return FALSE;
    c1 >>= 8; c2 >>= 8;
    if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff)
        return FALSE;
    c1 >>= 8; c2 >>= 8;
    if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff)
        return FALSE;
    return TRUE;
}

#define create_buffer(a, b, c, d) create_buffer_(__LINE__, a, b, c, d)
static ID3D11Buffer *create_buffer_(unsigned int line, ID3D11Device *device,
        unsigned int bind_flags, unsigned int size, const void *data)
{
    D3D11_SUBRESOURCE_DATA resource_data;
    D3D11_BUFFER_DESC buffer_desc;
    ID3D11Buffer *buffer;
    HRESULT hr;

    buffer_desc.ByteWidth = size;
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.BindFlags = bind_flags;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.MiscFlags = 0;
    buffer_desc.StructureByteStride = 0;

    resource_data.pSysMem = data;
    resource_data.SysMemPitch = 0;
    resource_data.SysMemSlicePitch = 0;

    hr = ID3D11Device_CreateBuffer(device, &buffer_desc, data ? &resource_data : NULL, &buffer);
    ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to create buffer, hr %#x.\n", hr);
    return buffer;
}

struct texture_readback
{
    ID3D11Resource *texture;
    D3D11_MAPPED_SUBRESOURCE map_desc;
    ID3D11DeviceContext *immediate_context;
    unsigned int width, height, sub_resource_idx;
};

static void get_texture_readback(ID3D11Texture2D *texture, unsigned int sub_resource_idx,
        struct texture_readback *rb)
{
    D3D11_TEXTURE2D_DESC texture_desc;
    unsigned int miplevel;
    ID3D11Device *device;
    HRESULT hr;

    memset(rb, 0, sizeof(*rb));

    ID3D11Texture2D_GetDevice(texture, &device);

    ID3D11Texture2D_GetDesc(texture, &texture_desc);
    texture_desc.Usage = D3D11_USAGE_STAGING;
    texture_desc.BindFlags = 0;
    texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    texture_desc.MiscFlags = 0;
    if (FAILED(hr = ID3D11Device_CreateTexture2D(device, &texture_desc, NULL, (ID3D11Texture2D **)&rb->texture)))
    {
        trace("Failed to create texture, hr %#x.\n", hr);
        ID3D11Device_Release(device);
        return;
    }

    miplevel = sub_resource_idx % texture_desc.MipLevels;
    rb->width = max(1, texture_desc.Width >> miplevel);
    rb->height = max(1, texture_desc.Height >> miplevel);
    rb->sub_resource_idx = sub_resource_idx;

    ID3D11Device_GetImmediateContext(device, &rb->immediate_context);

    ID3D11DeviceContext_CopyResource(rb->immediate_context, rb->texture, (ID3D11Resource *)texture);
    if (FAILED(hr = ID3D11DeviceContext_Map(rb->immediate_context, rb->texture, sub_resource_idx,
            D3D11_MAP_READ, 0, &rb->map_desc)))
    {
        trace("Failed to map sub-resource %u, hr %#x.\n", sub_resource_idx, hr);
        ID3D11Resource_Release(rb->texture);
        rb->texture = NULL;
        ID3D11DeviceContext_Release(rb->immediate_context);
        rb->immediate_context = NULL;
    }

    ID3D11Device_Release(device);
}

static DWORD get_readback_color(struct texture_readback *rb, unsigned int x, unsigned int y)
{
    return ((DWORD *)rb->map_desc.pData)[rb->map_desc.RowPitch * y / sizeof(DWORD) + x];
}

static float get_readback_float(struct texture_readback *rb, unsigned int x, unsigned int y)
{
    return ((float *)rb->map_desc.pData)[rb->map_desc.RowPitch * y / sizeof(float) + x];
}

static void release_texture_readback(struct texture_readback *rb)
{
    ID3D11DeviceContext_Unmap(rb->immediate_context, rb->texture, rb->sub_resource_idx);
    ID3D11Resource_Release(rb->texture);
    ID3D11DeviceContext_Release(rb->immediate_context);
}

static DWORD get_texture_color(ID3D11Texture2D *texture, unsigned int x, unsigned int y)
{
    struct texture_readback rb;
    DWORD color;

    get_texture_readback(texture, 0, &rb);
    color = get_readback_color(&rb, x, y);
    release_texture_readback(&rb);

    return color;
}

#define check_texture_sub_resource_color(t, s, c, d) check_texture_sub_resource_color_(__LINE__, t, s, c, d)
static void check_texture_sub_resource_color_(unsigned int line, ID3D11Texture2D *texture,
        unsigned int sub_resource_idx, DWORD expected_color, BYTE max_diff)
{
    struct texture_readback rb;
    unsigned int x = 0, y = 0;
    BOOL all_match = TRUE;
    DWORD color = 0;

    get_texture_readback(texture, sub_resource_idx, &rb);
    for (y = 0; y < rb.height; ++y)
    {
        for (x = 0; x < rb.width; ++x)
        {
            color = get_readback_color(&rb, x, y);
            if (!compare_color(color, expected_color, max_diff))
            {
                all_match = FALSE;
                break;
            }
        }
        if (!all_match)
            break;
    }
    release_texture_readback(&rb);
    ok_(__FILE__, line)(all_match,
            "Got unexpected color 0x%08x at (%u, %u), sub-resource %u.\n",
            color, x, y, sub_resource_idx);
}

#define check_texture_color(t, c, d) check_texture_color_(__LINE__, t, c, d)
static void check_texture_color_(unsigned int line, ID3D11Texture2D *texture,
        DWORD expected_color, BYTE max_diff)
{
    unsigned int sub_resource_idx, sub_resource_count;
    D3D11_TEXTURE2D_DESC texture_desc;

    ID3D11Texture2D_GetDesc(texture, &texture_desc);
    sub_resource_count = texture_desc.ArraySize * texture_desc.MipLevels;
    for (sub_resource_idx = 0; sub_resource_idx < sub_resource_count; ++sub_resource_idx)
        check_texture_sub_resource_color_(line, texture, sub_resource_idx, expected_color, max_diff);
}

#define check_texture_sub_resource_float(t, s, c, d) check_texture_sub_resource_float_(__LINE__, t, s, c, d)
static void check_texture_sub_resource_float_(unsigned int line, ID3D11Texture2D *texture,
        unsigned int sub_resource_idx, float expected_value, BYTE max_diff)
{
    struct texture_readback rb;
    unsigned int x = 0, y = 0;
    BOOL all_match = TRUE;
    float value = 0.0f;

    get_texture_readback(texture, sub_resource_idx, &rb);
    for (y = 0; y < rb.height; ++y)
    {
        for (x = 0; x < rb.width; ++x)
        {
            value = get_readback_float(&rb, x, y);
            if (!compare_float(value, expected_value, max_diff))
            {
                all_match = FALSE;
                break;
            }
        }
        if (!all_match)
            break;
    }
    release_texture_readback(&rb);
    ok_(__FILE__, line)(all_match,
            "Got unexpected value %.8e at (%u, %u), sub-resource %u.\n",
            value, x, y, sub_resource_idx);
}

#define check_texture_float(r, f, d) check_texture_float_(__LINE__, r, f, d)
static void check_texture_float_(unsigned int line, ID3D11Texture2D *texture,
        float expected_value, BYTE max_diff)
{
    unsigned int sub_resource_idx, sub_resource_count;
    D3D11_TEXTURE2D_DESC texture_desc;

    ID3D11Texture2D_GetDesc(texture, &texture_desc);
    sub_resource_count = texture_desc.ArraySize * texture_desc.MipLevels;
    for (sub_resource_idx = 0; sub_resource_idx < sub_resource_count; ++sub_resource_idx)
        check_texture_sub_resource_float_(line, texture, sub_resource_idx, expected_value, max_diff);
}

static ID3D11Device *create_device(const struct device_desc *desc)
{
    static const D3D_FEATURE_LEVEL default_feature_level[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };
    const D3D_FEATURE_LEVEL *feature_level;
    UINT flags = desc ? desc->flags : 0;
    unsigned int feature_level_count;
    ID3D11Device *device;

    if (desc && desc->feature_level)
    {
        feature_level = desc->feature_level;
        feature_level_count = 1;
    }
    else
    {
        feature_level = default_feature_level;
        feature_level_count = sizeof(default_feature_level) / sizeof(default_feature_level[0]);
    }

    if (SUCCEEDED(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, feature_level, feature_level_count,
            D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;
    if (SUCCEEDED(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_WARP, NULL, flags, feature_level, feature_level_count,
            D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;
    if (SUCCEEDED(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_REFERENCE, NULL, flags, feature_level, feature_level_count,
            D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;

    return NULL;
}

static BOOL is_warp_device(ID3D11Device *device)
{
    DXGI_ADAPTER_DESC adapter_desc;
    IDXGIDevice *dxgi_device;
    IDXGIAdapter *adapter;
    HRESULT hr;

    hr = ID3D11Device_QueryInterface(device, &IID_IDXGIDevice, (void **)&dxgi_device);
    ok(SUCCEEDED(hr), "Failed to query IDXGIDevice interface, hr %#x.\n", hr);
    hr = IDXGIDevice_GetAdapter(dxgi_device, &adapter);
    ok(SUCCEEDED(hr), "Failed to get adapter, hr %#x.\n", hr);
    IDXGIDevice_Release(dxgi_device);
    hr = IDXGIAdapter_GetDesc(adapter, &adapter_desc);
    ok(SUCCEEDED(hr), "Failed to get adapter desc, hr %#x.\n", hr);
    IDXGIAdapter_Release(adapter);

    return !adapter_desc.SubSysId && !adapter_desc.Revision
            && ((!adapter_desc.VendorId && !adapter_desc.DeviceId)
            || (adapter_desc.VendorId == 0x1414 && adapter_desc.DeviceId == 0x008c));
}

static IDXGISwapChain *create_swapchain(ID3D11Device *device, HWND window, const struct swapchain_desc *swapchain_desc)
{
    DXGI_SWAP_CHAIN_DESC dxgi_desc;
    IDXGISwapChain *swapchain;
    IDXGIDevice *dxgi_device;
    IDXGIAdapter *adapter;
    IDXGIFactory *factory;
    HRESULT hr;

    hr = ID3D11Device_QueryInterface(device, &IID_IDXGIDevice, (void **)&dxgi_device);
    ok(SUCCEEDED(hr), "Failed to get DXGI device, hr %#x.\n", hr);
    hr = IDXGIDevice_GetAdapter(dxgi_device, &adapter);
    ok(SUCCEEDED(hr), "Failed to get adapter, hr %#x.\n", hr);
    IDXGIDevice_Release(dxgi_device);
    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(SUCCEEDED(hr), "Failed to get factory, hr %#x.\n", hr);
    IDXGIAdapter_Release(adapter);

    dxgi_desc.BufferDesc.Width = 640;
    dxgi_desc.BufferDesc.Height = 480;
    dxgi_desc.BufferDesc.RefreshRate.Numerator = 60;
    dxgi_desc.BufferDesc.RefreshRate.Denominator = 1;
    dxgi_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    dxgi_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    dxgi_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    dxgi_desc.SampleDesc.Count = 1;
    dxgi_desc.SampleDesc.Quality = 0;
    dxgi_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    dxgi_desc.BufferCount = 1;
    dxgi_desc.OutputWindow = window;
    dxgi_desc.Windowed = TRUE;
    dxgi_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    dxgi_desc.Flags = 0;

    if (swapchain_desc)
    {
        dxgi_desc.Windowed = swapchain_desc->windowed;
        dxgi_desc.SwapEffect = swapchain_desc->swap_effect;
        dxgi_desc.BufferCount = swapchain_desc->buffer_count;

        if (swapchain_desc->flags & SWAPCHAIN_FLAG_SHADER_INPUT)
            dxgi_desc.BufferUsage |= DXGI_USAGE_SHADER_INPUT;
    }

    hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &dxgi_desc, &swapchain);
    ok(SUCCEEDED(hr), "Failed to create swapchain, hr %#x.\n", hr);
    IDXGIFactory_Release(factory);

    return swapchain;
}

struct d3d11_test_context
{
    ID3D11Device *device;
    HWND window;
    IDXGISwapChain *swapchain;
    ID3D11Texture2D *backbuffer;
    ID3D11RenderTargetView *backbuffer_rtv;
    ID3D11DeviceContext *immediate_context;

    ID3D11InputLayout *input_layout;
    ID3D11VertexShader *vs;
    ID3D11Buffer *vb;

    ID3D11PixelShader *ps;
    ID3D11Buffer *ps_cb;
};

#define init_test_context(c, l) init_test_context_(__LINE__, c, l)
static BOOL init_test_context_(unsigned int line, struct d3d11_test_context *context,
        const D3D_FEATURE_LEVEL *feature_level)
{
    struct device_desc device_desc;
    D3D11_VIEWPORT vp;
    HRESULT hr;

    memset(context, 0, sizeof(*context));

    device_desc.feature_level = feature_level;
    device_desc.flags = 0;
    if (!(context->device = create_device(&device_desc)))
    {
        skip_(__FILE__, line)("Failed to create device.\n");
        return FALSE;
    }
    context->window = CreateWindowA("static", "d3d11_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    context->swapchain = create_swapchain(context->device, context->window, NULL);
    hr = IDXGISwapChain_GetBuffer(context->swapchain, 0, &IID_ID3D11Texture2D, (void **)&context->backbuffer);
    ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to get backbuffer, hr %#x.\n", hr);

    hr = ID3D11Device_CreateRenderTargetView(context->device, (ID3D11Resource *)context->backbuffer,
            NULL, &context->backbuffer_rtv);
    ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to create rendertarget view, hr %#x.\n", hr);

    ID3D11Device_GetImmediateContext(context->device, &context->immediate_context);

    ID3D11DeviceContext_OMSetRenderTargets(context->immediate_context, 1, &context->backbuffer_rtv, NULL);

    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width = 640.0f;
    vp.Height = 480.0f;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    ID3D11DeviceContext_RSSetViewports(context->immediate_context, 1, &vp);

    return TRUE;
}

#define release_test_context(c) release_test_context_(__LINE__, c)
static void release_test_context_(unsigned int line, struct d3d11_test_context *context)
{
    ULONG ref;

    if (context->input_layout)
        ID3D11InputLayout_Release(context->input_layout);
    if (context->vs)
        ID3D11VertexShader_Release(context->vs);
    if (context->vb)
        ID3D11Buffer_Release(context->vb);
    if (context->ps)
        ID3D11PixelShader_Release(context->ps);
    if (context->ps_cb)
        ID3D11Buffer_Release(context->ps_cb);

    ID3D11DeviceContext_Release(context->immediate_context);
    ID3D11RenderTargetView_Release(context->backbuffer_rtv);
    ID3D11Texture2D_Release(context->backbuffer);
    IDXGISwapChain_Release(context->swapchain);
    DestroyWindow(context->window);

    ref = ID3D11Device_Release(context->device);
    ok_(__FILE__, line)(!ref, "Device has %u references left.\n", ref);
}

#define draw_quad(c) draw_quad_(__LINE__, c)
static void draw_quad_(unsigned int line, struct d3d11_test_context *context)
{
    static const D3D11_INPUT_ELEMENT_DESC default_layout_desc[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    static const DWORD default_vs_code[] =
    {
#if 0
        float4 main(float4 position : POSITION) : SV_POSITION
        {
            return position;
        }
#endif
        0x43425844, 0xa7a2f22d, 0x83ff2560, 0xe61638bd, 0x87e3ce90, 0x00000001, 0x000000d8, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x49534f50, 0x4e4f4954, 0xababab00,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000001, 0x00000003,
        0x00000000, 0x0000000f, 0x505f5653, 0x5449534f, 0x004e4f49, 0x52444853, 0x0000003c, 0x00010040,
        0x0000000f, 0x0300005f, 0x001010f2, 0x00000000, 0x04000067, 0x001020f2, 0x00000000, 0x00000001,
        0x05000036, 0x001020f2, 0x00000000, 0x00101e46, 0x00000000, 0x0100003e,
    };
    static const struct
    {
        struct vec2 position;
    }
    quad[] =
    {
        {{-1.0f, -1.0f}},
        {{-1.0f,  1.0f}},
        {{ 1.0f, -1.0f}},
        {{ 1.0f,  1.0f}},
    };

    ID3D11Device *device = context->device;
    unsigned int stride, offset;
    HRESULT hr;

    if (!context->input_layout)
    {
        hr = ID3D11Device_CreateInputLayout(device, default_layout_desc,
                sizeof(default_layout_desc) / sizeof(*default_layout_desc),
                default_vs_code, sizeof(default_vs_code), &context->input_layout);
        ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to create input layout, hr %#x.\n", hr);

        context->vb = create_buffer(device, D3D11_BIND_VERTEX_BUFFER, sizeof(quad), quad);

        hr = ID3D11Device_CreateVertexShader(device, default_vs_code, sizeof(default_vs_code), NULL, &context->vs);
        ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);
    }

    ID3D11DeviceContext_IASetInputLayout(context->immediate_context, context->input_layout);
    ID3D11DeviceContext_IASetPrimitiveTopology(context->immediate_context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    stride = sizeof(*quad);
    offset = 0;
    ID3D11DeviceContext_IASetVertexBuffers(context->immediate_context, 0, 1, &context->vb, &stride, &offset);
    ID3D11DeviceContext_VSSetShader(context->immediate_context, context->vs, NULL, 0);

    ID3D11DeviceContext_Draw(context->immediate_context, 4, 0);
}

#define draw_color_quad(c, color) draw_color_quad_(__LINE__, c, color)
static void draw_color_quad_(unsigned int line, struct d3d11_test_context *context, const struct vec4 *color)
{
    static const DWORD ps_color_code[] =
    {
#if 0
        float4 color;

        float4 main() : SV_TARGET
        {
            return color;
        }
#endif
        0x43425844, 0x80f1c810, 0xdacbbc8b, 0xe07b133e, 0x3059cbfa, 0x00000001, 0x000000b8, 0x00000003,
        0x0000002c, 0x0000003c, 0x00000070, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
        0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x00000040, 0x00000040, 0x00000010,
        0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x06000036,
        0x001020f2, 0x00000000, 0x00208e46, 0x00000000, 0x00000000, 0x0100003e,
    };

    ID3D11Device *device = context->device;
    HRESULT hr;

    if (!context->ps)
    {
        hr = ID3D11Device_CreatePixelShader(device, ps_color_code, sizeof(ps_color_code), NULL, &context->ps);
        ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);
    }

    if (!context->ps_cb)
        context->ps_cb = create_buffer(device, D3D11_BIND_CONSTANT_BUFFER, sizeof(*color), NULL);

    ID3D11DeviceContext_PSSetShader(context->immediate_context, context->ps, NULL, 0);
    ID3D11DeviceContext_PSSetConstantBuffers(context->immediate_context, 0, 1, &context->ps_cb);

    ID3D11DeviceContext_UpdateSubresource(context->immediate_context, (ID3D11Resource *)context->ps_cb, 0,
            NULL, color, 0, 0);

    draw_quad_(line, context);
}

static void test_create_device(void)
{
    static const D3D_FEATURE_LEVEL default_feature_levels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1,
    };
    D3D_FEATURE_LEVEL feature_level, supported_feature_level;
    DXGI_SWAP_CHAIN_DESC swapchain_desc, obtained_desc;
    ID3D11DeviceContext *immediate_context;
    IDXGISwapChain *swapchain;
    ID3D11Device *device;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    if (FAILED(hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
            &device, NULL, NULL)))
    {
        skip("Failed to create HAL device.\n");
        if ((device = create_device(NULL)))
        {
            trace("Feature level %#x.\n", ID3D11Device_GetFeatureLevel(device));
            ID3D11Device_Release(device);
        }
        return;
    }

    supported_feature_level = ID3D11Device_GetFeatureLevel(device);
    trace("Feature level %#x.\n", supported_feature_level);
    ID3D11Device_Release(device);

    hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, NULL, NULL, NULL);
    ok(hr == S_FALSE, "Got unexpected hr %#x.\n", hr);

    hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, NULL,
            &feature_level, NULL);
    ok(hr == S_FALSE, "Got unexpected hr %#x.\n", hr);
    ok(feature_level == supported_feature_level, "Got feature level %#x, expected %#x.\n",
            feature_level, supported_feature_level);

    hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, default_feature_levels,
            sizeof(default_feature_levels) / sizeof(default_feature_levels[0]), D3D11_SDK_VERSION, NULL,
            &feature_level, NULL);
    ok(hr == S_FALSE, "Got unexpected hr %#x.\n", hr);
    ok(feature_level == supported_feature_level, "Got feature level %#x, expected %#x.\n",
            feature_level, supported_feature_level);

    hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, NULL, NULL,
            &immediate_context);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    ok(!!immediate_context, "Expected immediate device context pointer, got NULL.\n");
    refcount = get_refcount((IUnknown *)immediate_context);
    ok(refcount == 1, "Got refcount %u, expected 1.\n", refcount);

    ID3D11DeviceContext_GetDevice(immediate_context, &device);
    refcount = ID3D11Device_Release(device);
    ok(refcount == 1, "Got refcount %u, expected 1.\n", refcount);

    refcount = ID3D11DeviceContext_Release(immediate_context);
    ok(!refcount, "ID3D11DeviceContext has %u references left.\n", refcount);

    device = (ID3D11Device *)0xdeadbeef;
    feature_level = 0xdeadbeef;
    immediate_context = (ID3D11DeviceContext *)0xdeadbeef;
    hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
            &device, &feature_level, &immediate_context);
    todo_wine ok(hr == E_INVALIDARG, "D3D11CreateDevice returned %#x.\n", hr);
    ok(!device, "Got unexpected device pointer %p.\n", device);
    ok(!feature_level, "Got unexpected feature level %#x.\n", feature_level);
    ok(!immediate_context, "Got unexpected immediate context pointer %p.\n", immediate_context);

    window = CreateWindowA("static", "d3d11_test", 0, 0, 0, 0, 0, 0, 0, 0, 0);

    swapchain_desc.BufferDesc.Width = 800;
    swapchain_desc.BufferDesc.Height = 600;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 60;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = 1;
    swapchain_desc.OutputWindow = window;
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;

    hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
            &swapchain_desc, NULL, NULL, NULL, NULL);
    ok(hr == S_FALSE, "Got unexpected hr %#x.\n", hr);

    hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
            &swapchain_desc, NULL, NULL, &feature_level, NULL);
    ok(hr == S_FALSE, "Got unexpected hr %#x.\n", hr);
    ok(feature_level == supported_feature_level, "Got feature level %#x, expected %#x.\n",
            feature_level, supported_feature_level);

    hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
            &swapchain_desc, &swapchain, &device, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    memset(&obtained_desc, 0, sizeof(obtained_desc));
    hr = IDXGISwapChain_GetDesc(swapchain, &obtained_desc);
    ok(SUCCEEDED(hr), "GetDesc failed %#x.\n", hr);
    ok(obtained_desc.BufferDesc.Width == swapchain_desc.BufferDesc.Width,
            "Got unexpected BufferDesc.Width %u.\n", obtained_desc.BufferDesc.Width);
    ok(obtained_desc.BufferDesc.Height == swapchain_desc.BufferDesc.Height,
            "Got unexpected BufferDesc.Height %u.\n", obtained_desc.BufferDesc.Height);
    todo_wine ok(obtained_desc.BufferDesc.RefreshRate.Numerator == swapchain_desc.BufferDesc.RefreshRate.Numerator,
            "Got unexpected BufferDesc.RefreshRate.Numerator %u.\n",
            obtained_desc.BufferDesc.RefreshRate.Numerator);
    todo_wine ok(obtained_desc.BufferDesc.RefreshRate.Denominator == swapchain_desc.BufferDesc.RefreshRate.Denominator,
            "Got unexpected BufferDesc.RefreshRate.Denominator %u.\n",
            obtained_desc.BufferDesc.RefreshRate.Denominator);
    ok(obtained_desc.BufferDesc.Format == swapchain_desc.BufferDesc.Format,
            "Got unexpected BufferDesc.Format %#x.\n", obtained_desc.BufferDesc.Format);
    ok(obtained_desc.BufferDesc.ScanlineOrdering == swapchain_desc.BufferDesc.ScanlineOrdering,
            "Got unexpected BufferDesc.ScanlineOrdering %#x.\n", obtained_desc.BufferDesc.ScanlineOrdering);
    ok(obtained_desc.BufferDesc.Scaling == swapchain_desc.BufferDesc.Scaling,
            "Got unexpected BufferDesc.Scaling %#x.\n", obtained_desc.BufferDesc.Scaling);
    ok(obtained_desc.SampleDesc.Count == swapchain_desc.SampleDesc.Count,
            "Got unexpected SampleDesc.Count %u.\n", obtained_desc.SampleDesc.Count);
    ok(obtained_desc.SampleDesc.Quality == swapchain_desc.SampleDesc.Quality,
            "Got unexpected SampleDesc.Quality %u.\n", obtained_desc.SampleDesc.Quality);
    todo_wine ok(obtained_desc.BufferUsage == swapchain_desc.BufferUsage,
            "Got unexpected BufferUsage %#x.\n", obtained_desc.BufferUsage);
    ok(obtained_desc.BufferCount == swapchain_desc.BufferCount,
            "Got unexpected BufferCount %u.\n", obtained_desc.BufferCount);
    ok(obtained_desc.OutputWindow == swapchain_desc.OutputWindow,
            "Got unexpected OutputWindow %p.\n", obtained_desc.OutputWindow);
    ok(obtained_desc.Windowed == swapchain_desc.Windowed,
            "Got unexpected Windowed %#x.\n", obtained_desc.Windowed);
    ok(obtained_desc.SwapEffect == swapchain_desc.SwapEffect,
            "Got unexpected SwapEffect %#x.\n", obtained_desc.SwapEffect);
    ok(obtained_desc.Flags == swapchain_desc.Flags,
            "Got unexpected Flags %#x.\n", obtained_desc.Flags);

    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "Swapchain has %u references left.\n", refcount);

    feature_level = ID3D11Device_GetFeatureLevel(device);
    ok(feature_level == supported_feature_level, "Got feature level %#x, expected %#x.\n",
            feature_level, supported_feature_level);

    refcount = ID3D11Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);

    hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
            NULL, NULL, &device, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ID3D11Device_Release(device);

    hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
            NULL, NULL, NULL, NULL, NULL);
    ok(hr == S_FALSE, "Got unexpected hr %#x.\n", hr);

    hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
            NULL, NULL, NULL, &feature_level, NULL);
    ok(hr == S_FALSE, "Got unexpected hr %#x.\n", hr);
    ok(feature_level == supported_feature_level, "Got feature level %#x, expected %#x.\n",
            feature_level, supported_feature_level);

    hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
            NULL, NULL, NULL, NULL, &immediate_context);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ID3D11DeviceContext_Release(immediate_context);

    hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
            &swapchain_desc, NULL, NULL, NULL, NULL);
    ok(hr == S_FALSE, "Got unexpected hr %#x.\n", hr);

    hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
            &swapchain_desc, &swapchain, NULL, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    IDXGISwapChain_Release(swapchain);

    swapchain_desc.OutputWindow = NULL;
    hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
            &swapchain_desc, NULL, NULL, NULL, NULL);
    ok(hr == S_FALSE, "D3D11CreateDeviceAndSwapChain returned %#x.\n", hr);
    hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
            &swapchain_desc, NULL, &device, NULL, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ID3D11Device_Release(device);

    swapchain = (IDXGISwapChain *)0xdeadbeef;
    device = (ID3D11Device *)0xdeadbeef;
    feature_level = 0xdeadbeef;
    immediate_context = (ID3D11DeviceContext *)0xdeadbeef;
    swapchain_desc.OutputWindow = NULL;
    hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
            &swapchain_desc, &swapchain, &device, &feature_level, &immediate_context);
    todo_wine ok(hr == DXGI_ERROR_INVALID_CALL, "D3D11CreateDeviceAndSwapChain returned %#x.\n", hr);
    ok(!swapchain, "Got unexpected swapchain pointer %p.\n", swapchain);
    ok(!device, "Got unexpected device pointer %p.\n", device);
    ok(!feature_level, "Got unexpected feature level %#x.\n", feature_level);
    ok(!immediate_context, "Got unexpected immediate context pointer %p.\n", immediate_context);

    swapchain = (IDXGISwapChain *)0xdeadbeef;
    device = (ID3D11Device *)0xdeadbeef;
    feature_level = 0xdeadbeef;
    immediate_context = (ID3D11DeviceContext *)0xdeadbeef;
    swapchain_desc.OutputWindow = window;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_BC5_UNORM;
    hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
            &swapchain_desc, &swapchain, &device, &feature_level, &immediate_context);
    ok(hr == E_INVALIDARG, "D3D11CreateDeviceAndSwapChain returned %#x.\n", hr);
    ok(!swapchain, "Got unexpected swapchain pointer %p.\n", swapchain);
    ok(!device, "Got unexpected device pointer %p.\n", device);
    ok(!feature_level, "Got unexpected feature level %#x.\n", feature_level);
    ok(!immediate_context, "Got unexpected immediate context pointer %p.\n", immediate_context);

    DestroyWindow(window);
}

static void test_device_interfaces(void)
{
    IDXGIAdapter *dxgi_adapter;
    IDXGIDevice *dxgi_device;
    ID3D11Device *device;
    IUnknown *iface;
    ULONG refcount;
    unsigned int i;
    HRESULT hr;

    for (i = 0; i < sizeof(d3d11_feature_levels) / sizeof(*d3d11_feature_levels); ++i)
    {
        struct device_desc device_desc;

        device_desc.feature_level = &d3d11_feature_levels[i];
        device_desc.flags = 0;
        if (!(device = create_device(&device_desc)))
        {
            skip("Failed to create device for feature level %#x.\n", d3d11_feature_levels[i]);
            continue;
        }

        hr = ID3D11Device_QueryInterface(device, &IID_IUnknown, (void **)&iface);
        ok(SUCCEEDED(hr), "Device should implement IUnknown interface, hr %#x.\n", hr);
        IUnknown_Release(iface);

        hr = ID3D11Device_QueryInterface(device, &IID_IDXGIObject, (void **)&iface);
        ok(SUCCEEDED(hr), "Device should implement IDXGIObject interface, hr %#x.\n", hr);
        IUnknown_Release(iface);

        hr = ID3D11Device_QueryInterface(device, &IID_IDXGIDevice, (void **)&dxgi_device);
        ok(SUCCEEDED(hr), "Device should implement IDXGIDevice.\n");
        hr = IDXGIDevice_GetParent(dxgi_device, &IID_IDXGIAdapter, (void **)&dxgi_adapter);
        ok(SUCCEEDED(hr), "Device parent should implement IDXGIAdapter.\n");
        hr = IDXGIAdapter_GetParent(dxgi_adapter, &IID_IDXGIFactory, (void **)&iface);
        ok(SUCCEEDED(hr), "Adapter parent should implement IDXGIFactory.\n");
        IUnknown_Release(iface);
        IDXGIAdapter_Release(dxgi_adapter);
        hr = IDXGIDevice_GetParent(dxgi_device, &IID_IDXGIAdapter1, (void **)&dxgi_adapter);
        ok(SUCCEEDED(hr), "Device parent should implement IDXGIAdapter1.\n");
        hr = IDXGIAdapter_GetParent(dxgi_adapter, &IID_IDXGIFactory1, (void **)&iface);
        ok(SUCCEEDED(hr), "Adapter parent should implement IDXGIFactory1.\n");
        IUnknown_Release(iface);
        IDXGIAdapter_Release(dxgi_adapter);
        IDXGIDevice_Release(dxgi_device);

        hr = ID3D11Device_QueryInterface(device, &IID_IDXGIDevice1, (void **)&iface);
        ok(SUCCEEDED(hr), "Device should implement IDXGIDevice1.\n");
        IUnknown_Release(iface);

        hr = ID3D11Device_QueryInterface(device, &IID_ID3D10Multithread, (void **)&iface);
        ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
                "Device should implement ID3D10Multithread interface, hr %#x.\n", hr);
        if (SUCCEEDED(hr)) IUnknown_Release(iface);

        hr = ID3D11Device_QueryInterface(device, &IID_ID3D10Device, (void **)&iface);
        todo_wine ok(hr == E_NOINTERFACE, "Device should not implement ID3D10Device interface, hr %#x.\n", hr);
        if (SUCCEEDED(hr)) IUnknown_Release(iface);

        hr = ID3D11Device_QueryInterface(device, &IID_ID3D10Device1, (void **)&iface);
        todo_wine ok(hr == E_NOINTERFACE, "Device should not implement ID3D10Device1 interface, hr %#x.\n", hr);
        if (SUCCEEDED(hr)) IUnknown_Release(iface);

        refcount = ID3D11Device_Release(device);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
}

static void test_get_immediate_context(void)
{
    ID3D11DeviceContext *immediate_context, *previous_immediate_context;
    ULONG expected_refcount, refcount;
    ID3D11Device *device;

    if (!(device = create_device(NULL)))
    {
        skip("Failed to create device.\n");
        return;
    }

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    ID3D11Device_GetImmediateContext(device, &immediate_context);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u.\n", refcount);
    previous_immediate_context = immediate_context;

    ID3D11Device_GetImmediateContext(device, &immediate_context);
    ok(immediate_context == previous_immediate_context, "Got different immediate device context objects.\n");
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u.\n", refcount);

    refcount = ID3D11DeviceContext_Release(previous_immediate_context);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    refcount = ID3D11DeviceContext_Release(immediate_context);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    ID3D11Device_GetImmediateContext(device, &immediate_context);
    ok(immediate_context == previous_immediate_context, "Got different immediate device context objects.\n");
    refcount = ID3D11DeviceContext_Release(immediate_context);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    refcount = ID3D11Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_texture2d(void)
{
    ULONG refcount, expected_refcount;
    D3D11_SUBRESOURCE_DATA data = {0};
    D3D_FEATURE_LEVEL feature_level;
    ID3D11Device *device, *tmp;
    D3D11_TEXTURE2D_DESC desc;
    ID3D11Texture2D *texture;
    UINT quality_level_count;
    IDXGISurface *surface;
    unsigned int i;
    HRESULT hr;

    static const struct
    {
        DXGI_FORMAT format;
        UINT array_size;
        D3D11_BIND_FLAG bind_flags;
        UINT misc_flags;
        BOOL succeeds;
        BOOL todo;
    }
    tests[] =
    {
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  1, D3D11_BIND_VERTEX_BUFFER,   0, FALSE, TRUE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  1, D3D11_BIND_INDEX_BUFFER,    0, FALSE, TRUE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  1, D3D11_BIND_CONSTANT_BUFFER, 0, FALSE, TRUE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  0, D3D11_BIND_SHADER_RESOURCE, 0, FALSE, FALSE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  1, D3D11_BIND_SHADER_RESOURCE, 0, TRUE,  FALSE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  2, D3D11_BIND_SHADER_RESOURCE, 0, TRUE,  FALSE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  3, D3D11_BIND_SHADER_RESOURCE, 0, TRUE,  FALSE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  3, D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_TEXTURECUBE,
                FALSE, TRUE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  1, D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_TEXTURECUBE,
                FALSE, TRUE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  5, D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_TEXTURECUBE,
                FALSE, TRUE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  6, D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_TEXTURECUBE,
                TRUE,  FALSE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  7, D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_TEXTURECUBE,
                TRUE,  FALSE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS, 10, D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_TEXTURECUBE,
                TRUE,  FALSE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS, 12, D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_TEXTURECUBE,
                TRUE,  FALSE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  0, D3D11_BIND_RENDER_TARGET,   0, FALSE, FALSE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  1, D3D11_BIND_RENDER_TARGET,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  2, D3D11_BIND_RENDER_TARGET,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  9, D3D11_BIND_RENDER_TARGET,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  1, D3D11_BIND_DEPTH_STENCIL,   0, FALSE, FALSE},
        {DXGI_FORMAT_R32G32B32_TYPELESS,     1, D3D11_BIND_SHADER_RESOURCE, 0, TRUE,  FALSE},
        {DXGI_FORMAT_R16G16B16A16_TYPELESS,  1, D3D11_BIND_SHADER_RESOURCE, 0, TRUE,  FALSE},
        {DXGI_FORMAT_R16G16B16A16_TYPELESS,  1, D3D11_BIND_RENDER_TARGET,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R32G32_TYPELESS,        1, D3D11_BIND_SHADER_RESOURCE, 0, TRUE,  FALSE},
        {DXGI_FORMAT_R32G8X24_TYPELESS,      1, D3D11_BIND_DEPTH_STENCIL,   0, TRUE,  TRUE},
        {DXGI_FORMAT_R10G10B10A2_TYPELESS,   1, D3D11_BIND_SHADER_RESOURCE, 0, TRUE,  FALSE},
        {DXGI_FORMAT_R10G10B10A2_TYPELESS,   1, D3D11_BIND_RENDER_TARGET,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R32_TYPELESS,           0, D3D11_BIND_SHADER_RESOURCE, 0, FALSE, FALSE},
        {DXGI_FORMAT_R32_TYPELESS,           1, D3D11_BIND_SHADER_RESOURCE, 0, TRUE,  FALSE},
        {DXGI_FORMAT_R32_TYPELESS,           9, D3D11_BIND_SHADER_RESOURCE, 0, TRUE,  FALSE},
        {DXGI_FORMAT_R32_TYPELESS,           9, D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_TEXTURECUBE,
                TRUE,  FALSE},
        {DXGI_FORMAT_R24G8_TYPELESS,         1, D3D11_BIND_VERTEX_BUFFER,   0, FALSE, TRUE},
        {DXGI_FORMAT_R24G8_TYPELESS,         1, D3D11_BIND_INDEX_BUFFER,    0, FALSE, TRUE},
        {DXGI_FORMAT_R24G8_TYPELESS,         1, D3D11_BIND_CONSTANT_BUFFER, 0, FALSE, TRUE},
        {DXGI_FORMAT_R24G8_TYPELESS,         1, D3D11_BIND_SHADER_RESOURCE, 0, TRUE,  FALSE},
        {DXGI_FORMAT_R24G8_TYPELESS,         1, D3D11_BIND_DEPTH_STENCIL,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R8G8_TYPELESS,          1, D3D11_BIND_SHADER_RESOURCE, 0, TRUE,  FALSE},
        {DXGI_FORMAT_R16_TYPELESS,           1, D3D11_BIND_SHADER_RESOURCE, 0, TRUE,  FALSE},
        {DXGI_FORMAT_R8_TYPELESS,            1, D3D11_BIND_SHADER_RESOURCE, 0, TRUE,  FALSE},
        {DXGI_FORMAT_R8G8B8A8_UNORM,         1, D3D11_BIND_DEPTH_STENCIL,   0, FALSE, FALSE},
        {DXGI_FORMAT_D24_UNORM_S8_UINT,      1, D3D11_BIND_RENDER_TARGET,   0, FALSE, FALSE},
        {DXGI_FORMAT_D32_FLOAT,              1, D3D11_BIND_RENDER_TARGET,   0, FALSE, FALSE},
    };

    if (!(device = create_device(NULL)))
    {
        skip("Failed to create device.\n");
        return;
    }

    feature_level = ID3D11Device_GetFeatureLevel(device);

    desc.Width = 512;
    desc.Height = 512;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    hr = ID3D11Device_CreateTexture2D(device, &desc, &data, &texture);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D11Device_CreateTexture2D(device, &desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 2d texture, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D11Texture2D_GetDevice(texture, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D11Device_Release(tmp);

    hr = ID3D11Texture2D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface);
    ok(SUCCEEDED(hr), "Texture should implement IDXGISurface.\n");
    IDXGISurface_Release(surface);
    ID3D11Texture2D_Release(texture);

    desc.MipLevels = 0;
    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D11Device_CreateTexture2D(device, &desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 2d texture, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D11Texture2D_GetDevice(texture, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D11Device_Release(tmp);

    ID3D11Texture2D_GetDesc(texture, &desc);
    ok(desc.Width == 512, "Got unexpected Width %u.\n", desc.Width);
    ok(desc.Height == 512, "Got unexpected Height %u.\n", desc.Height);
    ok(desc.MipLevels == 10, "Got unexpected MipLevels %u.\n", desc.MipLevels);
    ok(desc.ArraySize == 1, "Got unexpected ArraySize %u.\n", desc.ArraySize);
    ok(desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM, "Got unexpected Format %#x.\n", desc.Format);
    ok(desc.SampleDesc.Count == 1, "Got unexpected SampleDesc.Count %u.\n", desc.SampleDesc.Count);
    ok(desc.SampleDesc.Quality == 0, "Got unexpected SampleDesc.Quality %u.\n", desc.SampleDesc.Quality);
    ok(desc.Usage == D3D11_USAGE_DEFAULT, "Got unexpected Usage %u.\n", desc.Usage);
    ok(desc.BindFlags == D3D11_BIND_RENDER_TARGET, "Got unexpected BindFlags %#x.\n", desc.BindFlags);
    ok(desc.CPUAccessFlags == 0, "Got unexpected CPUAccessFlags %#x.\n", desc.CPUAccessFlags);
    ok(desc.MiscFlags == 0, "Got unexpected MiscFlags %#x.\n", desc.MiscFlags);

    hr = ID3D11Texture2D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface);
    ok(FAILED(hr), "Texture should not implement IDXGISurface.\n");
    ID3D11Texture2D_Release(texture);

    desc.MipLevels = 1;
    desc.ArraySize = 2;
    hr = ID3D11Device_CreateTexture2D(device, &desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 2d texture, hr %#x.\n", hr);

    hr = ID3D11Texture2D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface);
    ok(FAILED(hr), "Texture should not implement IDXGISurface.\n");
    ID3D11Texture2D_Release(texture);

    ID3D11Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_R8G8B8A8_UNORM, 2, &quality_level_count);
    desc.ArraySize = 1;
    desc.SampleDesc.Count = 2;
    hr = ID3D11Device_CreateTexture2D(device, &desc, NULL, &texture);
    if (quality_level_count)
    {
        ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
        ID3D11Texture2D_Release(texture);
        desc.SampleDesc.Quality = quality_level_count;
        hr = ID3D11Device_CreateTexture2D(device, &desc, NULL, &texture);
    }
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    /* We assume 15 samples multisampling is never supported in practice. */
    desc.SampleDesc.Count = 15;
    desc.SampleDesc.Quality = 0;
    hr = ID3D11Device_CreateTexture2D(device, &desc, NULL, &texture);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    desc.SampleDesc.Count = 1;
    for (i = 0; i < sizeof(tests) / sizeof(*tests); ++i)
    {
        HRESULT expected_hr = tests[i].succeeds ? S_OK : E_INVALIDARG;
        BOOL todo = tests[i].todo;

        if (feature_level < D3D_FEATURE_LEVEL_10_1
                && (tests[i].misc_flags & D3D11_RESOURCE_MISC_TEXTURECUBE)
                && tests[i].array_size > 6)
        {
            expected_hr = E_INVALIDARG;
            todo = TRUE;
        }

        desc.ArraySize = tests[i].array_size;
        desc.Format = tests[i].format;
        desc.BindFlags = tests[i].bind_flags;
        desc.MiscFlags = tests[i].misc_flags;
        hr = ID3D11Device_CreateTexture2D(device, &desc, NULL, (ID3D11Texture2D **)&texture);

        todo_wine_if(todo)
        ok(hr == expected_hr, "Test %u: Got unexpected hr %#x.\n", i, hr);

        if (SUCCEEDED(hr))
            ID3D11Texture2D_Release(texture);
    }

    refcount = ID3D11Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_texture2d_interfaces(void)
{
    ID3D10Texture2D *d3d10_texture;
    D3D11_TEXTURE2D_DESC desc;
    ID3D11Texture2D *texture;
    IDXGISurface *surface;
    ID3D11Device *device;
    unsigned int i;
    ULONG refcount;
    HRESULT hr;

    static const struct test
    {
        BOOL implements_d3d10_interfaces;
        UINT bind_flags;
        UINT misc_flags;
        UINT expected_bind_flags;
        UINT expected_misc_flags;
    }
    desc_conversion_tests[] =
    {
        {
            TRUE,
            D3D11_BIND_SHADER_RESOURCE, 0,
            D3D10_BIND_SHADER_RESOURCE, 0
        },
        {
            TRUE,
            D3D11_BIND_UNORDERED_ACCESS, 0,
            D3D11_BIND_UNORDERED_ACCESS, 0
        },
        {
            FALSE,
            0, D3D11_RESOURCE_MISC_RESOURCE_CLAMP,
            0, 0
        },
        {
            TRUE,
            0, D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX,
            0, D3D10_RESOURCE_MISC_SHARED_KEYEDMUTEX
        },
        {
            TRUE,
            0, D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX | D3D11_RESOURCE_MISC_SHARED_NTHANDLE,
            0, D3D10_RESOURCE_MISC_SHARED_KEYEDMUTEX
        },
    };

    if (!(device = create_device(NULL)))
    {
        skip("Failed to create ID3D11Device, skipping tests.\n");
        return;
    }

    desc.Width = 512;
    desc.Height = 512;
    desc.MipLevels = 0;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    hr = ID3D11Device_CreateTexture2D(device, &desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 2d texture, hr %#x.\n", hr);

    hr = ID3D11Texture2D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface);
    ok(hr == E_NOINTERFACE, "Texture should not implement IDXGISurface.\n");

    hr = ID3D11Texture2D_QueryInterface(texture, &IID_ID3D10Texture2D, (void **)&d3d10_texture);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Texture should implement ID3D10Texture2D.\n");
    if (SUCCEEDED(hr)) ID3D10Texture2D_Release(d3d10_texture);
    ID3D11Texture2D_Release(texture);

    if (FAILED(hr))
    {
        win_skip("2D textures do not implement ID3D10Texture2D, skipping tests.\n");
        ID3D11Device_Release(device);
        return;
    }

    for (i = 0; i < sizeof(desc_conversion_tests) / sizeof(*desc_conversion_tests); ++i)
    {
        const struct test *current = &desc_conversion_tests[i];
        D3D10_TEXTURE2D_DESC d3d10_desc;
        ID3D10Device *d3d10_device;

        desc.Width = 512;
        desc.Height = 512;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = current->bind_flags;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = current->misc_flags;

        hr = ID3D11Device_CreateTexture2D(device, &desc, NULL, &texture);
        /* Shared resources are not supported by REF and WARP devices. */
        ok(SUCCEEDED(hr) || broken(hr == E_OUTOFMEMORY),
                "Test %u: Failed to create a 2d texture, hr %#x.\n", i, hr);
        if (FAILED(hr))
        {
            win_skip("Failed to create ID3D11Texture2D, skipping test %u.\n", i);
            continue;
        }

        hr = ID3D11Texture2D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface);
        ok(SUCCEEDED(hr), "Test %u: Texture should implement IDXGISurface.\n", i);
        IDXGISurface_Release(surface);

        hr = ID3D11Texture2D_QueryInterface(texture, &IID_ID3D10Texture2D, (void **)&d3d10_texture);
        ID3D11Texture2D_Release(texture);

        if (current->implements_d3d10_interfaces)
        {
            ok(SUCCEEDED(hr), "Test %u: Texture should implement ID3D10Texture2D.\n", i);
        }
        else
        {
            todo_wine ok(hr == E_NOINTERFACE, "Test %u: Texture should not implement ID3D10Texture2D.\n", i);
            if (SUCCEEDED(hr)) ID3D10Texture2D_Release(d3d10_texture);
            continue;
        }

        ID3D10Texture2D_GetDesc(d3d10_texture, &d3d10_desc);

        ok(d3d10_desc.Width == desc.Width,
                "Test %u: Got unexpected Width %u.\n", i, d3d10_desc.Width);
        ok(d3d10_desc.Height == desc.Height,
                "Test %u: Got unexpected Height %u.\n", i, d3d10_desc.Height);
        ok(d3d10_desc.MipLevels == desc.MipLevels,
                "Test %u: Got unexpected MipLevels %u.\n", i, d3d10_desc.MipLevels);
        ok(d3d10_desc.ArraySize == desc.ArraySize,
                "Test %u: Got unexpected ArraySize %u.\n", i, d3d10_desc.ArraySize);
        ok(d3d10_desc.Format == desc.Format,
                "Test %u: Got unexpected Format %u.\n", i, d3d10_desc.Format);
        ok(d3d10_desc.SampleDesc.Count == desc.SampleDesc.Count,
                "Test %u: Got unexpected SampleDesc.Count %u.\n", i, d3d10_desc.SampleDesc.Count);
        ok(d3d10_desc.SampleDesc.Quality == desc.SampleDesc.Quality,
                "Test %u: Got unexpected SampleDesc.Quality %u.\n", i, d3d10_desc.SampleDesc.Quality);
        ok(d3d10_desc.Usage == (D3D10_USAGE)desc.Usage,
                "Test %u: Got unexpected Usage %u.\n", i, d3d10_desc.Usage);
        ok(d3d10_desc.BindFlags == current->expected_bind_flags,
                "Test %u: Got unexpected BindFlags %#x.\n", i, d3d10_desc.BindFlags);
        ok(d3d10_desc.CPUAccessFlags == desc.CPUAccessFlags,
                "Test %u: Got unexpected CPUAccessFlags %#x.\n", i, d3d10_desc.CPUAccessFlags);
        ok(d3d10_desc.MiscFlags == current->expected_misc_flags,
                "Test %u: Got unexpected MiscFlags %#x.\n", i, d3d10_desc.MiscFlags);

        d3d10_device = (ID3D10Device *)0xdeadbeef;
        ID3D10Texture2D_GetDevice(d3d10_texture, &d3d10_device);
        todo_wine ok(!d3d10_device, "Test %u: Got unexpected device pointer %p, expected NULL.\n", i, d3d10_device);
        if (d3d10_device) ID3D10Device_Release(d3d10_device);

        ID3D10Texture2D_Release(d3d10_texture);
    }

    refcount = ID3D11Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_texture3d(void)
{
    ULONG refcount, expected_refcount;
    D3D11_SUBRESOURCE_DATA data = {0};
    ID3D11Device *device, *tmp;
    D3D11_TEXTURE3D_DESC desc;
    ID3D11Texture3D *texture;
    IDXGISurface *surface;
    unsigned int i;
    HRESULT hr;

    static const struct
    {
        DXGI_FORMAT format;
        D3D11_BIND_FLAG bind_flags;
        BOOL succeeds;
        BOOL todo;
    }
    tests[] =
    {
        {DXGI_FORMAT_R32G32B32A32_TYPELESS, D3D11_BIND_VERTEX_BUFFER,   FALSE, TRUE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS, D3D11_BIND_INDEX_BUFFER,    FALSE, TRUE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS, D3D11_BIND_CONSTANT_BUFFER, FALSE, TRUE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS, D3D11_BIND_SHADER_RESOURCE, TRUE,  FALSE},
        {DXGI_FORMAT_R16G16B16A16_TYPELESS, D3D11_BIND_SHADER_RESOURCE, TRUE,  FALSE},
        {DXGI_FORMAT_R10G10B10A2_TYPELESS,  D3D11_BIND_SHADER_RESOURCE, TRUE,  FALSE},
        {DXGI_FORMAT_R8G8B8A8_UNORM,        D3D11_BIND_DEPTH_STENCIL,   FALSE, FALSE},
        {DXGI_FORMAT_D24_UNORM_S8_UINT,     D3D11_BIND_RENDER_TARGET,   FALSE, FALSE},
        {DXGI_FORMAT_D32_FLOAT,             D3D11_BIND_RENDER_TARGET,   FALSE, FALSE},
    };

    if (!(device = create_device(NULL)))
    {
        skip("Failed to create ID3D11Device, skipping tests.\n");
        return;
    }

    desc.Width = 64;
    desc.Height = 64;
    desc.Depth = 64;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    hr = ID3D11Device_CreateTexture3D(device, &desc, &data, &texture);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D11Device_CreateTexture3D(device, &desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 3d texture, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D11Texture3D_GetDevice(texture, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D11Device_Release(tmp);

    hr = ID3D11Texture3D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface);
    ok(FAILED(hr), "Texture should not implement IDXGISurface.\n");
    ID3D11Texture3D_Release(texture);

    desc.MipLevels = 0;
    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D11Device_CreateTexture3D(device, &desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 3d texture, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D11Texture3D_GetDevice(texture, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D11Device_Release(tmp);

    ID3D11Texture3D_GetDesc(texture, &desc);
    ok(desc.Width == 64, "Got unexpected Width %u.\n", desc.Width);
    ok(desc.Height == 64, "Got unexpected Height %u.\n", desc.Height);
    ok(desc.Depth == 64, "Got unexpected Depth %u.\n", desc.Depth);
    ok(desc.MipLevels == 7, "Got unexpected MipLevels %u.\n", desc.MipLevels);
    ok(desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM, "Got unexpected Format %#x.\n", desc.Format);
    ok(desc.Usage == D3D11_USAGE_DEFAULT, "Got unexpected Usage %u.\n", desc.Usage);
    ok(desc.BindFlags == D3D11_BIND_RENDER_TARGET, "Got unexpected BindFlags %u.\n", desc.BindFlags);
    ok(desc.CPUAccessFlags == 0, "Got unexpected CPUAccessFlags %u.\n", desc.CPUAccessFlags);
    ok(desc.MiscFlags == 0, "Got unexpected MiscFlags %u.\n", desc.MiscFlags);

    hr = ID3D11Texture3D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface);
    ok(FAILED(hr), "Texture should not implement IDXGISurface.\n");
    ID3D11Texture3D_Release(texture);

    desc.MipLevels = 1;
    for (i = 0; i < sizeof(tests) / sizeof(*tests); ++i)
    {
        desc.Format = tests[i].format;
        desc.BindFlags = tests[i].bind_flags;
        hr = ID3D11Device_CreateTexture3D(device, &desc, NULL, (ID3D11Texture3D **)&texture);

        todo_wine_if(tests[i].todo)
        ok(hr == (tests[i].succeeds ? S_OK : E_INVALIDARG), "Test %u: Got unexpected hr %#x.\n", i, hr);

        if (SUCCEEDED(hr))
            ID3D11Texture3D_Release(texture);
    }

    refcount = ID3D11Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_texture3d_interfaces(void)
{
    ID3D10Texture3D *d3d10_texture;
    D3D11_TEXTURE3D_DESC desc;
    ID3D11Texture3D *texture;
    IDXGISurface *surface;
    ID3D11Device *device;
    unsigned int i;
    ULONG refcount;
    HRESULT hr;

    static const struct test
    {
        BOOL implements_d3d10_interfaces;
        UINT bind_flags;
        UINT misc_flags;
        UINT expected_bind_flags;
        UINT expected_misc_flags;
    }
    desc_conversion_tests[] =
    {
        {
            TRUE,
            D3D11_BIND_SHADER_RESOURCE, 0,
            D3D10_BIND_SHADER_RESOURCE, 0
        },
        {
            TRUE,
            D3D11_BIND_UNORDERED_ACCESS, 0,
            D3D11_BIND_UNORDERED_ACCESS, 0
        },
        {
            FALSE,
            0, D3D11_RESOURCE_MISC_RESOURCE_CLAMP,
            0, 0
        },
        {
            TRUE,
            0, D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX,
            0, D3D10_RESOURCE_MISC_SHARED_KEYEDMUTEX
        },
    };

    if (!(device = create_device(NULL)))
    {
        skip("Failed to create ID3D11Device.\n");
        return;
    }

    desc.Width = 64;
    desc.Height = 64;
    desc.Depth = 64;
    desc.MipLevels = 0;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    hr = ID3D11Device_CreateTexture3D(device, &desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 3d texture, hr %#x.\n", hr);

    hr = ID3D11Texture3D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface);
    ok(hr == E_NOINTERFACE, "Texture should not implement IDXGISurface.\n");

    hr = ID3D11Texture3D_QueryInterface(texture, &IID_ID3D10Texture3D, (void **)&d3d10_texture);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Texture should implement ID3D10Texture3D.\n");
    if (SUCCEEDED(hr)) ID3D10Texture3D_Release(d3d10_texture);
    ID3D11Texture3D_Release(texture);

    if (FAILED(hr))
    {
        win_skip("3D textures do not implement ID3D10Texture3D.\n");
        ID3D11Device_Release(device);
        return;
    }

    for (i = 0; i < sizeof(desc_conversion_tests) / sizeof(*desc_conversion_tests); ++i)
    {
        const struct test *current = &desc_conversion_tests[i];
        D3D10_TEXTURE3D_DESC d3d10_desc;
        ID3D10Device *d3d10_device;

        desc.Width = 64;
        desc.Height = 64;
        desc.Depth = 64;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = current->bind_flags;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = current->misc_flags;

        hr = ID3D11Device_CreateTexture3D(device, &desc, NULL, &texture);
        /* Shared resources are not supported by REF and WARP devices. */
        ok(SUCCEEDED(hr) || broken(hr == E_OUTOFMEMORY),
                "Test %u: Failed to create a 3d texture, hr %#x.\n", i, hr);
        if (FAILED(hr))
        {
            win_skip("Failed to create ID3D11Texture3D, skipping test %u.\n", i);
            continue;
        }

        hr = ID3D11Texture3D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface);
        ok(hr == E_NOINTERFACE, "Texture should not implement IDXGISurface.\n");

        hr = ID3D11Texture3D_QueryInterface(texture, &IID_ID3D10Texture3D, (void **)&d3d10_texture);
        ID3D11Texture3D_Release(texture);

        if (current->implements_d3d10_interfaces)
        {
            ok(SUCCEEDED(hr), "Test %u: Texture should implement ID3D10Texture3D.\n", i);
        }
        else
        {
            todo_wine ok(hr == E_NOINTERFACE, "Test %u: Texture should not implement ID3D10Texture3D.\n", i);
            if (SUCCEEDED(hr)) ID3D10Texture3D_Release(d3d10_texture);
            continue;
        }

        ID3D10Texture3D_GetDesc(d3d10_texture, &d3d10_desc);

        ok(d3d10_desc.Width == desc.Width,
                "Test %u: Got unexpected Width %u.\n", i, d3d10_desc.Width);
        ok(d3d10_desc.Height == desc.Height,
                "Test %u: Got unexpected Height %u.\n", i, d3d10_desc.Height);
        ok(d3d10_desc.Depth == desc.Depth,
                "Test %u: Got unexpected Depth %u.\n", i, d3d10_desc.Depth);
        ok(d3d10_desc.MipLevels == desc.MipLevels,
                "Test %u: Got unexpected MipLevels %u.\n", i, d3d10_desc.MipLevels);
        ok(d3d10_desc.Format == desc.Format,
                "Test %u: Got unexpected Format %u.\n", i, d3d10_desc.Format);
        ok(d3d10_desc.Usage == (D3D10_USAGE)desc.Usage,
                "Test %u: Got unexpected Usage %u.\n", i, d3d10_desc.Usage);
        ok(d3d10_desc.BindFlags == current->expected_bind_flags,
                "Test %u: Got unexpected BindFlags %#x.\n", i, d3d10_desc.BindFlags);
        ok(d3d10_desc.CPUAccessFlags == desc.CPUAccessFlags,
                "Test %u: Got unexpected CPUAccessFlags %#x.\n", i, d3d10_desc.CPUAccessFlags);
        ok(d3d10_desc.MiscFlags == current->expected_misc_flags,
                "Test %u: Got unexpected MiscFlags %#x.\n", i, d3d10_desc.MiscFlags);

        d3d10_device = (ID3D10Device *)0xdeadbeef;
        ID3D10Texture3D_GetDevice(d3d10_texture, &d3d10_device);
        todo_wine ok(!d3d10_device, "Test %u: Got unexpected device pointer %p, expected NULL.\n", i, d3d10_device);
        if (d3d10_device) ID3D10Device_Release(d3d10_device);

        ID3D10Texture3D_Release(d3d10_texture);
    }

    refcount = ID3D11Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_buffer_interfaces(void)
{
    ID3D10Buffer *d3d10_buffer;
    D3D11_BUFFER_DESC desc;
    ID3D11Buffer *buffer;
    ID3D11Device *device;
    unsigned int i;
    ULONG refcount;
    HRESULT hr;

    static const struct test
    {
        BOOL implements_d3d10_interfaces;
        UINT bind_flags;
        UINT misc_flags;
        UINT structure_stride;
        UINT expected_bind_flags;
        UINT expected_misc_flags;
    }
    desc_conversion_tests[] =
    {
        {
            TRUE,
            D3D11_BIND_VERTEX_BUFFER, 0, 0,
            D3D10_BIND_VERTEX_BUFFER, 0
        },
        {
            TRUE,
            D3D11_BIND_INDEX_BUFFER, 0, 0,
            D3D10_BIND_INDEX_BUFFER, 0
        },
        {
            TRUE,
            D3D11_BIND_CONSTANT_BUFFER, 0, 0,
            D3D10_BIND_CONSTANT_BUFFER, 0
        },
        {
            TRUE,
            D3D11_BIND_SHADER_RESOURCE, 0, 0,
            D3D10_BIND_SHADER_RESOURCE, 0
        },
        {
            TRUE,
            D3D11_BIND_STREAM_OUTPUT, 0, 0,
            D3D10_BIND_STREAM_OUTPUT, 0
        },
        {
            TRUE,
            D3D11_BIND_RENDER_TARGET, 0, 0,
            D3D10_BIND_RENDER_TARGET, 0
        },
        {
            TRUE,
            D3D11_BIND_UNORDERED_ACCESS, 0, 0,
            D3D11_BIND_UNORDERED_ACCESS, 0
        },
        {
            TRUE,
            0, D3D11_RESOURCE_MISC_SHARED, 0,
            0, D3D10_RESOURCE_MISC_SHARED
        },
        {
            TRUE,
            0, D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS, 0,
            0, 0
        },
        {
            TRUE,
            D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS, 0,
            D3D10_BIND_SHADER_RESOURCE, 0
        },
        {
            FALSE /* Structured buffers do not implement ID3D10Buffer. */,
            0, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, 16,
            0, 0
        },
        {
            TRUE,
            0, D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX, 0,
            0, D3D10_RESOURCE_MISC_SHARED_KEYEDMUTEX
        },
    };

    if (!(device = create_device(NULL)))
    {
        skip("Failed to create ID3D11Device.\n");
        return;
    }

    buffer = create_buffer(device, D3D11_BIND_VERTEX_BUFFER, 1024, NULL);
    hr = ID3D11Buffer_QueryInterface(buffer, &IID_ID3D10Buffer, (void **)&d3d10_buffer);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Buffer should implement ID3D10Buffer.\n");
    if (SUCCEEDED(hr)) ID3D10Buffer_Release(d3d10_buffer);
    ID3D11Buffer_Release(buffer);

    if (FAILED(hr))
    {
        win_skip("Buffers do not implement ID3D10Buffer.\n");
        ID3D11Device_Release(device);
        return;
    }

    for (i = 0; i < sizeof(desc_conversion_tests) / sizeof(*desc_conversion_tests); ++i)
    {
        const struct test *current = &desc_conversion_tests[i];
        D3D10_BUFFER_DESC d3d10_desc;
        ID3D10Device *d3d10_device;

        desc.ByteWidth = 1024;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = current->bind_flags;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = current->misc_flags;
        desc.StructureByteStride = current->structure_stride;

        hr = ID3D11Device_CreateBuffer(device, &desc, NULL, &buffer);
        /* Shared resources are not supported by REF and WARP devices. */
        ok(SUCCEEDED(hr) || broken(hr == E_OUTOFMEMORY), "Test %u: Failed to create a buffer, hr %#x.\n", i, hr);
        if (FAILED(hr))
        {
            win_skip("Failed to create a buffer, skipping test %u.\n", i);
            continue;
        }

        hr = ID3D11Buffer_QueryInterface(buffer, &IID_ID3D10Buffer, (void **)&d3d10_buffer);
        ID3D11Buffer_Release(buffer);

        if (current->implements_d3d10_interfaces)
        {
            ok(SUCCEEDED(hr), "Test %u: Buffer should implement ID3D10Buffer.\n", i);
        }
        else
        {
            todo_wine ok(hr == E_NOINTERFACE, "Test %u: Buffer should not implement ID3D10Buffer.\n", i);
            if (SUCCEEDED(hr)) ID3D10Buffer_Release(d3d10_buffer);
            continue;
        }

        ID3D10Buffer_GetDesc(d3d10_buffer, &d3d10_desc);

        ok(d3d10_desc.ByteWidth == desc.ByteWidth,
                "Test %u: Got unexpected ByteWidth %u.\n", i, d3d10_desc.ByteWidth);
        ok(d3d10_desc.Usage == (D3D10_USAGE)desc.Usage,
                "Test %u: Got unexpected Usage %u.\n", i, d3d10_desc.Usage);
        ok(d3d10_desc.BindFlags == current->expected_bind_flags,
                "Test %u: Got unexpected BindFlags %#x.\n", i, d3d10_desc.BindFlags);
        ok(d3d10_desc.CPUAccessFlags == desc.CPUAccessFlags,
                "Test %u: Got unexpected CPUAccessFlags %#x.\n", i, d3d10_desc.CPUAccessFlags);
        ok(d3d10_desc.MiscFlags == current->expected_misc_flags,
                "Test %u: Got unexpected MiscFlags %#x.\n", i, d3d10_desc.MiscFlags);

        d3d10_device = (ID3D10Device *)0xdeadbeef;
        ID3D10Buffer_GetDevice(d3d10_buffer, &d3d10_device);
        todo_wine ok(!d3d10_device, "Test %u: Got unexpected device pointer %p, expected NULL.\n", i, d3d10_device);
        if (d3d10_device) ID3D10Device_Release(d3d10_device);

        ID3D10Buffer_Release(d3d10_buffer);
    }

    refcount = ID3D11Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_depthstencil_view(void)
{
    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
    D3D11_TEXTURE2D_DESC texture_desc;
    ULONG refcount, expected_refcount;
    ID3D11DepthStencilView *dsview;
    ID3D11Device *device, *tmp;
    ID3D11Texture2D *texture;
    HRESULT hr;

    if (!(device = create_device(NULL)))
    {
        skip("Failed to create device.\n");
        return;
    }

    texture_desc.Width = 512;
    texture_desc.Height = 512;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    hr = ID3D11Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 2d texture, hr %#x.\n", hr);

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D11Device_CreateDepthStencilView(device, (ID3D11Resource *)texture, NULL, &dsview);
    ok(SUCCEEDED(hr), "Failed to create a depthstencil view, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D11DepthStencilView_GetDevice(dsview, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D11Device_Release(tmp);

    memset(&dsv_desc, 0, sizeof(dsv_desc));
    ID3D11DepthStencilView_GetDesc(dsview, &dsv_desc);
    ok(dsv_desc.Format == texture_desc.Format, "Got unexpected format %#x.\n", dsv_desc.Format);
    ok(dsv_desc.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE2D,
            "Got unexpected view dimension %#x.\n", dsv_desc.ViewDimension);
    ok(!dsv_desc.Flags, "Got unexpected flags %#x.\n", dsv_desc.Flags);
    ok(!U(dsv_desc).Texture2D.MipSlice, "Got unexpected mip slice %u.\n", U(dsv_desc).Texture2D.MipSlice);

    ID3D11DepthStencilView_Release(dsview);

    dsv_desc.Format = DXGI_FORMAT_UNKNOWN;

    hr = ID3D11Device_CreateDepthStencilView(device, (ID3D11Resource *)texture, &dsv_desc, &dsview);
    ok(SUCCEEDED(hr), "Failed to create a depthstencil view, hr %#x.\n", hr);

    memset(&dsv_desc, 0, sizeof(dsv_desc));
    ID3D11DepthStencilView_GetDesc(dsview, &dsv_desc);
    ok(dsv_desc.Format == texture_desc.Format, "Got unexpected format %#x.\n", dsv_desc.Format);
    ok(dsv_desc.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE2D,
            "Got unexpected view dimension %#x.\n", dsv_desc.ViewDimension);
    ok(!dsv_desc.Flags, "Got unexpected flags %#x.\n", dsv_desc.Flags);
    ok(!U(dsv_desc).Texture2D.MipSlice, "Got unexpected mip slice %u.\n", U(dsv_desc).Texture2D.MipSlice);

    ID3D11DepthStencilView_Release(dsview);
    ID3D11Texture2D_Release(texture);

    texture_desc.ArraySize = 4;

    hr = ID3D11Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create 2d texture, hr %#x.\n", hr);

    hr = ID3D11Device_CreateDepthStencilView(device, (ID3D11Resource *)texture, NULL, &dsview);
    ok(SUCCEEDED(hr), "Failed to create depthstencil view, hr %#x.\n", hr);

    memset(&dsv_desc, 0, sizeof(dsv_desc));
    ID3D11DepthStencilView_GetDesc(dsview, &dsv_desc);
    ok(dsv_desc.Format == texture_desc.Format, "Got unexpected format %#x.\n", dsv_desc.Format);
    ok(dsv_desc.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE2DARRAY,
            "Got unexpected view dimension %#x.\n", dsv_desc.ViewDimension);
    ok(!dsv_desc.Flags, "Got unexpected flags %#x.\n", dsv_desc.Flags);
    ok(!U(dsv_desc).Texture2DArray.MipSlice, "Got unexpected mip slice %u.\n",
            U(dsv_desc).Texture2DArray.MipSlice);
    ok(!U(dsv_desc).Texture2DArray.FirstArraySlice, "Got unexpected first array slice %u.\n",
            U(dsv_desc).Texture2DArray.FirstArraySlice);
    ok(U(dsv_desc).Texture2DArray.ArraySize == texture_desc.ArraySize,
            "Got unexpected array size %u.\n", U(dsv_desc).Texture2DArray.ArraySize);

    ID3D11DepthStencilView_Release(dsview);
    ID3D11Texture2D_Release(texture);

    refcount = ID3D11Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_depthstencil_view_interfaces(void)
{
    D3D10_DEPTH_STENCIL_VIEW_DESC d3d10_dsv_desc;
    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
    ID3D10DepthStencilView *d3d10_dsview;
    D3D11_TEXTURE2D_DESC texture_desc;
    ID3D11DepthStencilView *dsview;
    ID3D11Texture2D *texture;
    ID3D11Device *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device(NULL)))
    {
        skip("Failed to create device.\n");
        return;
    }

    texture_desc.Width = 512;
    texture_desc.Height = 512;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    hr = ID3D11Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 2d texture, hr %#x.\n", hr);

    dsv_desc.Format = texture_desc.Format;
    dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsv_desc.Flags = 0;
    U(dsv_desc).Texture2D.MipSlice = 0;

    hr = ID3D11Device_CreateDepthStencilView(device, (ID3D11Resource *)texture, &dsv_desc, &dsview);
    ok(SUCCEEDED(hr), "Failed to create a depthstencil view, hr %#x.\n", hr);

    hr = ID3D11DepthStencilView_QueryInterface(dsview, &IID_ID3D10DepthStencilView, (void **)&d3d10_dsview);
    ID3D11DepthStencilView_Release(dsview);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
        "Depth stencil view should implement ID3D10DepthStencilView.\n");

    if (FAILED(hr))
    {
        win_skip("Depth stencil view does not implement ID3D10DepthStencilView.\n");
        goto done;
    }

    ID3D10DepthStencilView_GetDesc(d3d10_dsview, &d3d10_dsv_desc);
    ok(d3d10_dsv_desc.Format == dsv_desc.Format, "Got unexpected format %#x.\n", d3d10_dsv_desc.Format);
    ok(d3d10_dsv_desc.ViewDimension == (D3D10_DSV_DIMENSION)dsv_desc.ViewDimension,
            "Got unexpected view dimension %u.\n", d3d10_dsv_desc.ViewDimension);
    ok(U(d3d10_dsv_desc).Texture2D.MipSlice == U(dsv_desc).Texture2D.MipSlice,
            "Got unexpected mip slice %u.\n", U(d3d10_dsv_desc).Texture2D.MipSlice);

    ID3D10DepthStencilView_Release(d3d10_dsview);

done:
    ID3D11Texture2D_Release(texture);

    refcount = ID3D11Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_rendertarget_view(void)
{
    D3D11_RENDER_TARGET_VIEW_DESC rtv_desc;
    D3D11_SUBRESOURCE_DATA data = {0};
    D3D11_TEXTURE2D_DESC texture_desc;
    ULONG refcount, expected_refcount;
    D3D11_BUFFER_DESC buffer_desc;
    ID3D11RenderTargetView *rtview;
    ID3D11Device *device, *tmp;
    ID3D11Texture2D *texture;
    ID3D11Buffer *buffer;
    IUnknown *iface;
    HRESULT hr;

    if (!(device = create_device(NULL)))
    {
        skip("Failed to create device.\n");
        return;
    }

    buffer_desc.ByteWidth = 1024;
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.MiscFlags = 0;
    buffer_desc.StructureByteStride = 0;

    hr = ID3D11Device_CreateBuffer(device, &buffer_desc, &data, &buffer);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D11Device_CreateBuffer(device, &buffer_desc, NULL, &buffer);
    ok(SUCCEEDED(hr), "Failed to create a buffer, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D11Buffer_GetDevice(buffer, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D11Device_Release(tmp);

    rtv_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_BUFFER;
    U(rtv_desc).Buffer.ElementOffset = 0;
    U(rtv_desc).Buffer.ElementWidth = 64;

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D11Device_CreateRenderTargetView(device, (ID3D11Resource *)buffer, &rtv_desc, &rtview);
    ok(SUCCEEDED(hr), "Failed to create a rendertarget view, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D11RenderTargetView_GetDevice(rtview, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D11Device_Release(tmp);

    hr = ID3D11RenderTargetView_QueryInterface(rtview, &IID_ID3D10RenderTargetView, (void **)&iface);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Render target view should implement ID3D10RenderTargetView.\n");
    if (SUCCEEDED(hr)) IUnknown_Release(iface);

    ID3D11RenderTargetView_Release(rtview);
    ID3D11Buffer_Release(buffer);

    texture_desc.Width = 512;
    texture_desc.Height = 512;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    hr = ID3D11Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 2d texture, hr %#x.\n", hr);

    /* For texture resources it's allowed to specify NULL as desc */
    hr = ID3D11Device_CreateRenderTargetView(device, (ID3D11Resource *)texture, NULL, &rtview);
    ok(SUCCEEDED(hr), "Failed to create a rendertarget view, hr %#x.\n", hr);

    memset(&rtv_desc, 0, sizeof(rtv_desc));
    ID3D11RenderTargetView_GetDesc(rtview, &rtv_desc);
    ok(rtv_desc.Format == texture_desc.Format, "Got unexpected format %#x.\n", rtv_desc.Format);
    ok(rtv_desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2D, "Got unexpected view dimension %#x.\n",
            rtv_desc.ViewDimension);
    ok(!U(rtv_desc).Texture2D.MipSlice, "Got unexpected mip slice %u.\n", U(rtv_desc).Texture2D.MipSlice);

    hr = ID3D11RenderTargetView_QueryInterface(rtview, &IID_ID3D10RenderTargetView, (void **)&iface);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Render target view should implement ID3D10RenderTargetView.\n");
    if (SUCCEEDED(hr)) IUnknown_Release(iface);

    ID3D11RenderTargetView_Release(rtview);

    rtv_desc.Format = DXGI_FORMAT_UNKNOWN;

    hr = ID3D11Device_CreateRenderTargetView(device, (ID3D11Resource *)texture, &rtv_desc, &rtview);
    ok(SUCCEEDED(hr), "Failed to create a rendertarget view, hr %#x.\n", hr);

    memset(&rtv_desc, 0, sizeof(rtv_desc));
    ID3D11RenderTargetView_GetDesc(rtview, &rtv_desc);
    ok(rtv_desc.Format == texture_desc.Format, "Got unexpected format %#x.\n", rtv_desc.Format);
    ok(rtv_desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2D, "Got unexpected view dimension %#x.\n",
            rtv_desc.ViewDimension);
    ok(!U(rtv_desc).Texture2D.MipSlice, "Got unexpected mip slice %u.\n", U(rtv_desc).Texture2D.MipSlice);

    ID3D11RenderTargetView_Release(rtview);
    ID3D11Texture2D_Release(texture);

    texture_desc.ArraySize = 4;

    hr = ID3D11Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create 2d texture, hr %#x.\n", hr);

    hr = ID3D11Device_CreateRenderTargetView(device, (ID3D11Resource *)texture, NULL, &rtview);
    ok(SUCCEEDED(hr), "Failed to create rendertarget view, hr %#x.\n", hr);

    memset(&rtv_desc, 0, sizeof(rtv_desc));
    ID3D11RenderTargetView_GetDesc(rtview, &rtv_desc);
    ok(rtv_desc.Format == texture_desc.Format, "Got unexpected format %#x.\n", rtv_desc.Format);
    ok(rtv_desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DARRAY, "Got unexpected view dimension %#x.\n",
            rtv_desc.ViewDimension);
    ok(!U(rtv_desc).Texture2DArray.MipSlice, "Got unexpected mip slice %u.\n",
            U(rtv_desc).Texture2DArray.MipSlice);
    ok(!U(rtv_desc).Texture2DArray.FirstArraySlice, "Got unexpected first array slice %u.\n",
            U(rtv_desc).Texture2DArray.FirstArraySlice);
    ok(U(rtv_desc).Texture2DArray.ArraySize == texture_desc.ArraySize, "Got unexpected array size %u.\n",
            U(rtv_desc).Texture2DArray.ArraySize);

    ID3D11RenderTargetView_Release(rtview);
    ID3D11Texture2D_Release(texture);

    refcount = ID3D11Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_shader_resource_view(void)
{
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
    D3D11_TEXTURE2D_DESC texture_desc;
    ULONG refcount, expected_refcount;
    ID3D11ShaderResourceView *srview;
    ID3D11Device *device, *tmp;
    ID3D11Texture2D *texture;
    ID3D11Buffer *buffer;
    IUnknown *iface;
    HRESULT hr;

    if (!(device = create_device(NULL)))
    {
        skip("Failed to create device.\n");
        return;
    }

    buffer = create_buffer(device, D3D11_BIND_SHADER_RESOURCE, 1024, NULL);

    hr = ID3D11Device_CreateShaderResourceView(device, (ID3D11Resource *)buffer, NULL, &srview);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    srv_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    U(srv_desc).Buffer.ElementOffset = 0;
    U(srv_desc).Buffer.ElementWidth = 64;

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D11Device_CreateShaderResourceView(device, (ID3D11Resource *)buffer, &srv_desc, &srview);
    ok(SUCCEEDED(hr), "Failed to create a shader resource view, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D11ShaderResourceView_GetDevice(srview, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D11Device_Release(tmp);

    hr = ID3D11ShaderResourceView_QueryInterface(srview, &IID_ID3D10ShaderResourceView, (void **)&iface);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Shader resource view should implement ID3D10ShaderResourceView.\n");
    if (SUCCEEDED(hr)) IUnknown_Release(iface);
    hr = ID3D11ShaderResourceView_QueryInterface(srview, &IID_ID3D10ShaderResourceView1, (void **)&iface);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Shader resource view should implement ID3D10ShaderResourceView1.\n");
    if (SUCCEEDED(hr)) IUnknown_Release(iface);

    ID3D11ShaderResourceView_Release(srview);
    ID3D11Buffer_Release(buffer);

    texture_desc.Width = 512;
    texture_desc.Height = 512;
    texture_desc.MipLevels = 0;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    hr = ID3D11Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 2d texture, hr %#x.\n", hr);

    hr = ID3D11Device_CreateShaderResourceView(device, (ID3D11Resource *)texture, NULL, &srview);
    ok(SUCCEEDED(hr), "Failed to create a shader resource view, hr %#x.\n", hr);

    hr = ID3D11ShaderResourceView_QueryInterface(srview, &IID_ID3D10ShaderResourceView, (void **)&iface);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Shader resource view should implement ID3D10ShaderResourceView.\n");
    if (SUCCEEDED(hr)) IUnknown_Release(iface);
    hr = ID3D11ShaderResourceView_QueryInterface(srview, &IID_ID3D10ShaderResourceView1, (void **)&iface);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Shader resource view should implement ID3D10ShaderResourceView1.\n");
    if (SUCCEEDED(hr)) IUnknown_Release(iface);

    memset(&srv_desc, 0, sizeof(srv_desc));
    ID3D11ShaderResourceView_GetDesc(srview, &srv_desc);
    ok(srv_desc.Format == texture_desc.Format, "Got unexpected format %#x.\n", srv_desc.Format);
    ok(srv_desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2D,
            "Got unexpected view dimension %#x.\n", srv_desc.ViewDimension);
    ok(U(srv_desc).Texture2D.MostDetailedMip == 0, "Got unexpected MostDetailedMip %u.\n",
            U(srv_desc).Texture2D.MostDetailedMip);
    ok(U(srv_desc).Texture2D.MipLevels == 10, "Got unexpected MipLevels %u.\n", U(srv_desc).Texture2D.MipLevels);

    ID3D11ShaderResourceView_Release(srview);

    srv_desc.Format = DXGI_FORMAT_UNKNOWN;
    U(srv_desc).Texture2D.MipLevels = -1;

    hr = ID3D11Device_CreateShaderResourceView(device, (ID3D11Resource *)texture, &srv_desc, &srview);
    ok(SUCCEEDED(hr), "Failed to create a shader resource view, hr %#x.\n", hr);

    memset(&srv_desc, 0, sizeof(srv_desc));
    ID3D11ShaderResourceView_GetDesc(srview, &srv_desc);
    ok(srv_desc.Format == texture_desc.Format, "Got unexpected format %#x.\n", srv_desc.Format);
    ok(srv_desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2D,
            "Got unexpected view dimension %#x.\n", srv_desc.ViewDimension);
    ok(U(srv_desc).Texture2D.MostDetailedMip == 0, "Got unexpected MostDetailedMip %u.\n",
            U(srv_desc).Texture2D.MostDetailedMip);
    todo_wine ok(U(srv_desc).Texture2D.MipLevels == 10, "Got unexpected MipLevels %u.\n",
            U(srv_desc).Texture2D.MipLevels);

    ID3D11ShaderResourceView_Release(srview);
    ID3D11Texture2D_Release(texture);

    texture_desc.ArraySize = 4;

    hr = ID3D11Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create 2d texture, hr %#x.\n", hr);

    hr = ID3D11Device_CreateShaderResourceView(device, (ID3D11Resource *)texture, NULL, &srview);
    ok(SUCCEEDED(hr), "Failed to create shader resource view, hr %#x.\n", hr);

    memset(&srv_desc, 0, sizeof(srv_desc));
    ID3D11ShaderResourceView_GetDesc(srview, &srv_desc);
    ok(srv_desc.Format == texture_desc.Format, "Got unexpected format %#x.\n", srv_desc.Format);
    ok(srv_desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2DARRAY,
            "Got unexpected view dimension %#x.\n", srv_desc.ViewDimension);
    ok(!U(srv_desc).Texture2DArray.MostDetailedMip, "Got unexpected MostDetailedMip %u.\n",
            U(srv_desc).Texture2DArray.MostDetailedMip);
    ok(U(srv_desc).Texture2DArray.MipLevels == 10, "Got unexpected MipLevels %u.\n",
            U(srv_desc).Texture2DArray.MipLevels);
    ok(!U(srv_desc).Texture2DArray.FirstArraySlice, "Got unexpected FirstArraySlice %u.\n",
            U(srv_desc).Texture2DArray.FirstArraySlice);
    ok(U(srv_desc).Texture2DArray.ArraySize == texture_desc.ArraySize, "Got unexpected ArraySize %u.\n",
            U(srv_desc).Texture2DArray.ArraySize);

    ID3D11ShaderResourceView_Release(srview);
    ID3D11Texture2D_Release(texture);

    refcount = ID3D11Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_shader(void)
{
#if 0
    float4 light;
    float4x4 mat;

    struct input
    {
        float4 position : POSITION;
        float3 normal : NORMAL;
    };

    struct output
    {
        float4 position : POSITION;
        float4 diffuse : COLOR;
    };

    output main(const input v)
    {
        output o;

        o.position = mul(v.position, mat);
        o.diffuse = dot((float3)light, v.normal);

        return o;
    }
#endif
    static const DWORD vs_4_1[] =
    {
        0x43425844, 0xfce5b27c, 0x965db93d, 0x8c3d0459, 0x9890ebac, 0x00000001, 0x000001c4, 0x00000003,
        0x0000002c, 0x0000007c, 0x000000cc, 0x4e475349, 0x00000048, 0x00000002, 0x00000008, 0x00000038,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x00000041, 0x00000000, 0x00000000,
        0x00000003, 0x00000001, 0x00000707, 0x49534f50, 0x4e4f4954, 0x524f4e00, 0x004c414d, 0x4e47534f,
        0x00000048, 0x00000002, 0x00000008, 0x00000038, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
        0x0000000f, 0x00000041, 0x00000000, 0x00000000, 0x00000003, 0x00000001, 0x0000000f, 0x49534f50,
        0x4e4f4954, 0x4c4f4300, 0xab00524f, 0x52444853, 0x000000f0, 0x00010041, 0x0000003c, 0x0100086a,
        0x04000059, 0x00208e46, 0x00000000, 0x00000005, 0x0300005f, 0x001010f2, 0x00000000, 0x0300005f,
        0x00101072, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x03000065, 0x001020f2, 0x00000001,
        0x08000011, 0x00102012, 0x00000000, 0x00101e46, 0x00000000, 0x00208e46, 0x00000000, 0x00000001,
        0x08000011, 0x00102022, 0x00000000, 0x00101e46, 0x00000000, 0x00208e46, 0x00000000, 0x00000002,
        0x08000011, 0x00102042, 0x00000000, 0x00101e46, 0x00000000, 0x00208e46, 0x00000000, 0x00000003,
        0x08000011, 0x00102082, 0x00000000, 0x00101e46, 0x00000000, 0x00208e46, 0x00000000, 0x00000004,
        0x08000010, 0x001020f2, 0x00000001, 0x00208246, 0x00000000, 0x00000000, 0x00101246, 0x00000001,
        0x0100003e,
    };
    static const DWORD vs_4_0[] =
    {
        0x43425844, 0x3ae813ca, 0x0f034b91, 0x790f3226, 0x6b4a718a, 0x00000001, 0x000001c0,
        0x00000003, 0x0000002c, 0x0000007c, 0x000000cc, 0x4e475349, 0x00000048, 0x00000002,
        0x00000008, 0x00000038, 0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f,
        0x00000041, 0x00000000, 0x00000000, 0x00000003, 0x00000001, 0x00000707, 0x49534f50,
        0x4e4f4954, 0x524f4e00, 0x004c414d, 0x4e47534f, 0x00000048, 0x00000002, 0x00000008,
        0x00000038, 0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x0000000f, 0x00000041,
        0x00000000, 0x00000000, 0x00000003, 0x00000001, 0x0000000f, 0x49534f50, 0x4e4f4954,
        0x4c4f4300, 0xab00524f, 0x52444853, 0x000000ec, 0x00010040, 0x0000003b, 0x04000059,
        0x00208e46, 0x00000000, 0x00000005, 0x0300005f, 0x001010f2, 0x00000000, 0x0300005f,
        0x00101072, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x03000065, 0x001020f2,
        0x00000001, 0x08000011, 0x00102012, 0x00000000, 0x00101e46, 0x00000000, 0x00208e46,
        0x00000000, 0x00000001, 0x08000011, 0x00102022, 0x00000000, 0x00101e46, 0x00000000,
        0x00208e46, 0x00000000, 0x00000002, 0x08000011, 0x00102042, 0x00000000, 0x00101e46,
        0x00000000, 0x00208e46, 0x00000000, 0x00000003, 0x08000011, 0x00102082, 0x00000000,
        0x00101e46, 0x00000000, 0x00208e46, 0x00000000, 0x00000004, 0x08000010, 0x001020f2,
        0x00000001, 0x00208246, 0x00000000, 0x00000000, 0x00101246, 0x00000001, 0x0100003e,
    };
    static const DWORD vs_3_0[] =
    {
        0xfffe0300, 0x002bfffe, 0x42415443, 0x0000001c, 0x00000077, 0xfffe0300, 0x00000002,
        0x0000001c, 0x00000100, 0x00000070, 0x00000044, 0x00040002, 0x00000001, 0x0000004c,
        0x00000000, 0x0000005c, 0x00000002, 0x00000004, 0x00000060, 0x00000000, 0x6867696c,
        0xabab0074, 0x00030001, 0x00040001, 0x00000001, 0x00000000, 0x0074616d, 0x00030003,
        0x00040004, 0x00000001, 0x00000000, 0x335f7376, 0x4d00305f, 0x6f726369, 0x74666f73,
        0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x2e392072,
        0x392e3932, 0x332e3235, 0x00313131, 0x0200001f, 0x80000000, 0x900f0000, 0x0200001f,
        0x80000003, 0x900f0001, 0x0200001f, 0x80000000, 0xe00f0000, 0x0200001f, 0x8000000a,
        0xe00f0001, 0x03000009, 0xe0010000, 0x90e40000, 0xa0e40000, 0x03000009, 0xe0020000,
        0x90e40000, 0xa0e40001, 0x03000009, 0xe0040000, 0x90e40000, 0xa0e40002, 0x03000009,
        0xe0080000, 0x90e40000, 0xa0e40003, 0x03000008, 0xe00f0001, 0xa0e40004, 0x90e40001,
        0x0000ffff,
    };
    static const DWORD vs_2_0[] =
    {
        0xfffe0200, 0x002bfffe, 0x42415443, 0x0000001c, 0x00000077, 0xfffe0200, 0x00000002,
        0x0000001c, 0x00000100, 0x00000070, 0x00000044, 0x00040002, 0x00000001, 0x0000004c,
        0x00000000, 0x0000005c, 0x00000002, 0x00000004, 0x00000060, 0x00000000, 0x6867696c,
        0xabab0074, 0x00030001, 0x00040001, 0x00000001, 0x00000000, 0x0074616d, 0x00030003,
        0x00040004, 0x00000001, 0x00000000, 0x325f7376, 0x4d00305f, 0x6f726369, 0x74666f73,
        0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x2e392072,
        0x392e3932, 0x332e3235, 0x00313131, 0x0200001f, 0x80000000, 0x900f0000, 0x0200001f,
        0x80000003, 0x900f0001, 0x03000009, 0xc0010000, 0x90e40000, 0xa0e40000, 0x03000009,
        0xc0020000, 0x90e40000, 0xa0e40001, 0x03000009, 0xc0040000, 0x90e40000, 0xa0e40002,
        0x03000009, 0xc0080000, 0x90e40000, 0xa0e40003, 0x03000008, 0xd00f0000, 0xa0e40004,
        0x90e40001, 0x0000ffff,
    };

#if 0
    float4 main(const float4 color : COLOR) : SV_TARGET
    {
        float4 o;

        o = color;

        return o;
    }
#endif
    static const DWORD ps_4_1[] =
    {
        0x43425844, 0xa1a44423, 0xa4cfcec2, 0x64610832, 0xb7a852bd, 0x00000001, 0x000000d4, 0x00000003,
        0x0000002c, 0x0000005c, 0x00000090, 0x4e475349, 0x00000028, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x4f4c4f43, 0xabab0052, 0x4e47534f,
        0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
        0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x0000003c, 0x00000041, 0x0000000f,
        0x0100086a, 0x03001062, 0x001010f2, 0x00000000, 0x03000065, 0x001020f2, 0x00000000, 0x05000036,
        0x001020f2, 0x00000000, 0x00101e46, 0x00000000, 0x0100003e,
    };
    static const DWORD ps_4_0[] =
    {
        0x43425844, 0x4da9446f, 0xfbe1f259, 0x3fdb3009, 0x517521fa, 0x00000001, 0x000001ac,
        0x00000005, 0x00000034, 0x0000008c, 0x000000bc, 0x000000f0, 0x00000130, 0x46454452,
        0x00000050, 0x00000000, 0x00000000, 0x00000000, 0x0000001c, 0xffff0400, 0x00000100,
        0x0000001c, 0x7263694d, 0x666f736f, 0x52282074, 0x4c482029, 0x53204c53, 0x65646168,
        0x6f432072, 0x6c69706d, 0x39207265, 0x2e39322e, 0x2e323539, 0x31313133, 0xababab00,
        0x4e475349, 0x00000028, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000,
        0x00000003, 0x00000000, 0x00000f0f, 0x4f4c4f43, 0xabab0052, 0x4e47534f, 0x0000002c,
        0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
        0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x00000038, 0x00000040,
        0x0000000e, 0x03001062, 0x001010f2, 0x00000000, 0x03000065, 0x001020f2, 0x00000000,
        0x05000036, 0x001020f2, 0x00000000, 0x00101e46, 0x00000000, 0x0100003e, 0x54415453,
        0x00000074, 0x00000002, 0x00000000, 0x00000000, 0x00000002, 0x00000000, 0x00000000,
        0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000,
    };
    static const DWORD ps_4_0_level_9_0[] =
    {
        0x43425844, 0xbc6626e7, 0x7778dc9d, 0xc8a43be2, 0xe4b53f7a, 0x00000001, 0x00000170,
        0x00000005, 0x00000034, 0x00000080, 0x000000cc, 0x0000010c, 0x0000013c, 0x53414e58,
        0x00000044, 0x00000044, 0xffff0200, 0x00000020, 0x00000024, 0x00240000, 0x00240000,
        0x00240000, 0x00240000, 0x00240000, 0xffff0200, 0x0200001f, 0x80000000, 0xb00f0000,
        0x02000001, 0x800f0800, 0x80e40000, 0x0000ffff, 0x396e6f41, 0x00000044, 0x00000044,
        0xffff0200, 0x00000020, 0x00000024, 0x00240000, 0x00240000, 0x00240000, 0x00240000,
        0x00240000, 0xffff0200, 0x0200001f, 0x80000000, 0xb00f0000, 0x02000001, 0x800f0800,
        0xb0e40000, 0x0000ffff, 0x52444853, 0x00000038, 0x00000040, 0x0000000e, 0x03001062,
        0x001010f2, 0x00000000, 0x03000065, 0x001020f2, 0x00000000, 0x05000036, 0x001020f2,
        0x00000000, 0x00101e46, 0x00000000, 0x0100003e, 0x4e475349, 0x00000028, 0x00000001,
        0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f,
        0x4f4c4f43, 0xabab0052, 0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x0000000f, 0x545f5653, 0x45475241,
        0xabab0054,
    };
    static const DWORD ps_4_0_level_9_1[] =
    {
        0x43425844, 0x275ecf38, 0x4349ff01, 0xa6b0e324, 0x6e54a4fc, 0x00000001, 0x00000120,
        0x00000004, 0x00000030, 0x0000007c, 0x000000bc, 0x000000ec, 0x396e6f41, 0x00000044,
        0x00000044, 0xffff0200, 0x00000020, 0x00000024, 0x00240000, 0x00240000, 0x00240000,
        0x00240000, 0x00240000, 0xffff0200, 0x0200001f, 0x80000000, 0xb00f0000, 0x02000001,
        0x800f0800, 0xb0e40000, 0x0000ffff, 0x52444853, 0x00000038, 0x00000040, 0x0000000e,
        0x03001062, 0x001010f2, 0x00000000, 0x03000065, 0x001020f2, 0x00000000, 0x05000036,
        0x001020f2, 0x00000000, 0x00101e46, 0x00000000, 0x0100003e, 0x4e475349, 0x00000028,
        0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
        0x00000f0f, 0x4f4c4f43, 0xabab0052, 0x4e47534f, 0x0000002c, 0x00000001, 0x00000008,
        0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x0000000f, 0x545f5653,
        0x45475241, 0xabab0054,
    };
    static const DWORD ps_4_0_level_9_3[] =
    {
        0x43425844, 0xc7d541c4, 0x961d4e0e, 0x9ce7ec57, 0x70f47dcb, 0x00000001, 0x00000120,
        0x00000004, 0x00000030, 0x0000007c, 0x000000bc, 0x000000ec, 0x396e6f41, 0x00000044,
        0x00000044, 0xffff0200, 0x00000020, 0x00000024, 0x00240000, 0x00240000, 0x00240000,
        0x00240000, 0x00240000, 0xffff0201, 0x0200001f, 0x80000000, 0xb00f0000, 0x02000001,
        0x800f0800, 0xb0e40000, 0x0000ffff, 0x52444853, 0x00000038, 0x00000040, 0x0000000e,
        0x03001062, 0x001010f2, 0x00000000, 0x03000065, 0x001020f2, 0x00000000, 0x05000036,
        0x001020f2, 0x00000000, 0x00101e46, 0x00000000, 0x0100003e, 0x4e475349, 0x00000028,
        0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
        0x00000f0f, 0x4f4c4f43, 0xabab0052, 0x4e47534f, 0x0000002c, 0x00000001, 0x00000008,
        0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x0000000f, 0x545f5653,
        0x45475241, 0xabab0054,
    };

#if 0
    struct gs_out
    {
        float4 pos : SV_POSITION;
    };

    [maxvertexcount(4)]
    void main(point float4 vin[1] : POSITION, inout TriangleStream<gs_out> vout)
    {
        float offset = 0.1 * vin[0].w;
        gs_out v;

        v.pos = float4(vin[0].x - offset, vin[0].y - offset, vin[0].z, vin[0].w);
        vout.Append(v);
        v.pos = float4(vin[0].x - offset, vin[0].y + offset, vin[0].z, vin[0].w);
        vout.Append(v);
        v.pos = float4(vin[0].x + offset, vin[0].y - offset, vin[0].z, vin[0].w);
        vout.Append(v);
        v.pos = float4(vin[0].x + offset, vin[0].y + offset, vin[0].z, vin[0].w);
        vout.Append(v);
    }
#endif
    static const DWORD gs_4_1[] =
    {
        0x43425844, 0x779daaf5, 0x7e154197, 0xcf5e99da, 0xb502b4d2, 0x00000001, 0x00000240, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x49534f50, 0x4e4f4954, 0xababab00,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000001, 0x00000003,
        0x00000000, 0x0000000f, 0x505f5653, 0x5449534f, 0x004e4f49, 0x52444853, 0x000001a4, 0x00020041,
        0x00000069, 0x0100086a, 0x0400005f, 0x002010f2, 0x00000001, 0x00000000, 0x02000068, 0x00000001,
        0x0100085d, 0x0100285c, 0x04000067, 0x001020f2, 0x00000000, 0x00000001, 0x0200005e, 0x00000004,
        0x0f000032, 0x00100032, 0x00000000, 0x80201ff6, 0x00000041, 0x00000000, 0x00000000, 0x00004002,
        0x3dcccccd, 0x3dcccccd, 0x00000000, 0x00000000, 0x00201046, 0x00000000, 0x00000000, 0x05000036,
        0x00102032, 0x00000000, 0x00100046, 0x00000000, 0x06000036, 0x001020c2, 0x00000000, 0x00201ea6,
        0x00000000, 0x00000000, 0x01000013, 0x05000036, 0x00102012, 0x00000000, 0x0010000a, 0x00000000,
        0x0e000032, 0x00100052, 0x00000000, 0x00201ff6, 0x00000000, 0x00000000, 0x00004002, 0x3dcccccd,
        0x00000000, 0x3dcccccd, 0x00000000, 0x00201106, 0x00000000, 0x00000000, 0x05000036, 0x00102022,
        0x00000000, 0x0010002a, 0x00000000, 0x06000036, 0x001020c2, 0x00000000, 0x00201ea6, 0x00000000,
        0x00000000, 0x01000013, 0x05000036, 0x00102012, 0x00000000, 0x0010000a, 0x00000000, 0x05000036,
        0x00102022, 0x00000000, 0x0010001a, 0x00000000, 0x06000036, 0x001020c2, 0x00000000, 0x00201ea6,
        0x00000000, 0x00000000, 0x01000013, 0x05000036, 0x00102032, 0x00000000, 0x00100086, 0x00000000,
        0x06000036, 0x001020c2, 0x00000000, 0x00201ea6, 0x00000000, 0x00000000, 0x01000013, 0x0100003e,
    };
    static const DWORD gs_4_0[] =
    {
        0x43425844, 0x000ee786, 0xc624c269, 0x885a5cbe, 0x444b3b1f, 0x00000001, 0x0000023c, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x49534f50, 0x4e4f4954, 0xababab00,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000001, 0x00000003,
        0x00000000, 0x0000000f, 0x505f5653, 0x5449534f, 0x004e4f49, 0x52444853, 0x000001a0, 0x00020040,
        0x00000068, 0x0400005f, 0x002010f2, 0x00000001, 0x00000000, 0x02000068, 0x00000001, 0x0100085d,
        0x0100285c, 0x04000067, 0x001020f2, 0x00000000, 0x00000001, 0x0200005e, 0x00000004, 0x0f000032,
        0x00100032, 0x00000000, 0x80201ff6, 0x00000041, 0x00000000, 0x00000000, 0x00004002, 0x3dcccccd,
        0x3dcccccd, 0x00000000, 0x00000000, 0x00201046, 0x00000000, 0x00000000, 0x05000036, 0x00102032,
        0x00000000, 0x00100046, 0x00000000, 0x06000036, 0x001020c2, 0x00000000, 0x00201ea6, 0x00000000,
        0x00000000, 0x01000013, 0x05000036, 0x00102012, 0x00000000, 0x0010000a, 0x00000000, 0x0e000032,
        0x00100052, 0x00000000, 0x00201ff6, 0x00000000, 0x00000000, 0x00004002, 0x3dcccccd, 0x00000000,
        0x3dcccccd, 0x00000000, 0x00201106, 0x00000000, 0x00000000, 0x05000036, 0x00102022, 0x00000000,
        0x0010002a, 0x00000000, 0x06000036, 0x001020c2, 0x00000000, 0x00201ea6, 0x00000000, 0x00000000,
        0x01000013, 0x05000036, 0x00102012, 0x00000000, 0x0010000a, 0x00000000, 0x05000036, 0x00102022,
        0x00000000, 0x0010001a, 0x00000000, 0x06000036, 0x001020c2, 0x00000000, 0x00201ea6, 0x00000000,
        0x00000000, 0x01000013, 0x05000036, 0x00102032, 0x00000000, 0x00100086, 0x00000000, 0x06000036,
        0x001020c2, 0x00000000, 0x00201ea6, 0x00000000, 0x00000000, 0x01000013, 0x0100003e,
    };

    ULONG refcount, expected_refcount;
    ID3D11Device *device, *tmp;
    ID3D11GeometryShader *gs;
    ID3D11VertexShader *vs;
    ID3D11PixelShader *ps;
    IUnknown *iface;
    unsigned int i;
    HRESULT hr;

    for (i = 0; i < sizeof(d3d11_feature_levels) / sizeof(*d3d11_feature_levels); ++i)
    {
        D3D_FEATURE_LEVEL feature_level = d3d11_feature_levels[i];
        BOOL todo = feature_level <= D3D_FEATURE_LEVEL_9_3;
        struct device_desc device_desc;

        device_desc.feature_level = &feature_level;
        device_desc.flags = 0;
        if (!(device = create_device(&device_desc)))
        {
            skip("Failed to create device for feature level %#x.\n", feature_level);
            continue;
        }

        /* level_9 shaders */
        hr = ID3D11Device_CreatePixelShader(device, ps_4_0_level_9_0, sizeof(ps_4_0_level_9_0), NULL, &ps);
        todo_wine_if(todo)
            ok(SUCCEEDED(hr), "Failed to create ps_4_0_level_9_0 shader, hr %#x, feature level %#x.\n", hr, feature_level);
        if (SUCCEEDED(hr))
            ID3D11PixelShader_Release(ps);

        hr = ID3D11Device_CreatePixelShader(device, ps_4_0_level_9_1, sizeof(ps_4_0_level_9_1), NULL, &ps);
        todo_wine_if(todo)
            ok(SUCCEEDED(hr), "Failed to create ps_4_0_level_9_1 shader, hr %#x, feature level %#x.\n", hr, feature_level);
        if (SUCCEEDED(hr))
            ID3D11PixelShader_Release(ps);

        hr = ID3D11Device_CreatePixelShader(device, ps_4_0_level_9_3, sizeof(ps_4_0_level_9_3), NULL, &ps);
        todo_wine_if(todo)
            ok(SUCCEEDED(hr), "Failed to create ps_4_0_level_9_3 shader, hr %#x, feature level %#x.\n", hr, feature_level);
        if (SUCCEEDED(hr))
            ID3D11PixelShader_Release(ps);

        /* vertex shader */
        hr = ID3D11Device_CreateVertexShader(device, vs_2_0, sizeof(vs_2_0), NULL, &vs);
        ok(hr == E_INVALIDARG, "Created a SM2 vertex shader, hr %#x, feature level %#x.\n", hr, feature_level);

        hr = ID3D11Device_CreateVertexShader(device, vs_3_0, sizeof(vs_3_0), NULL, &vs);
        ok(hr == E_INVALIDARG, "Created a SM3 vertex shader, hr %#x, feature level %#x.\n", hr, feature_level);

        hr = ID3D11Device_CreateVertexShader(device, ps_4_0, sizeof(ps_4_0), NULL, &vs);
        ok(hr == E_INVALIDARG, "Created a SM4 vertex shader from a pixel shader source, hr %#x, feature level %#x.\n",
                hr, feature_level);

        expected_refcount = get_refcount((IUnknown *)device) + (feature_level >= D3D_FEATURE_LEVEL_10_0);
        hr = ID3D11Device_CreateVertexShader(device, vs_4_0, sizeof(vs_4_0), NULL, &vs);
        if (feature_level >= D3D_FEATURE_LEVEL_10_0)
            ok(SUCCEEDED(hr), "Failed to create SM4 vertex shader, hr %#x, feature level %#x.\n", hr, feature_level);
        else
            ok(hr == E_INVALIDARG, "Created a SM4 vertex shader, hr %#x, feature level %#x.\n", hr, feature_level);

        refcount = get_refcount((IUnknown *)device);
        ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n",
                refcount, expected_refcount);
        if (feature_level >= D3D_FEATURE_LEVEL_10_0)
        {
            tmp = NULL;
            expected_refcount = refcount + 1;
            ID3D11VertexShader_GetDevice(vs, &tmp);
            ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
            refcount = get_refcount((IUnknown *)device);
            ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n",
                    refcount, expected_refcount);
            ID3D11Device_Release(tmp);

            hr = ID3D11VertexShader_QueryInterface(vs, &IID_ID3D10VertexShader, (void **)&iface);
            ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
                    "Vertex shader should implement ID3D10VertexShader.\n");
            if (SUCCEEDED(hr)) IUnknown_Release(iface);

            refcount = ID3D11VertexShader_Release(vs);
            ok(!refcount, "Vertex shader has %u references left.\n", refcount);
        }

        hr = ID3D11Device_CreateVertexShader(device, vs_4_1, sizeof(vs_4_1), NULL, &vs);
        if (feature_level >= D3D_FEATURE_LEVEL_10_1)
        {
            ok(SUCCEEDED(hr), "Failed to create SM4.1 vertex shader, hr %#x, feature level %#x.\n",
                    hr, feature_level);
            refcount = ID3D11VertexShader_Release(vs);
            ok(!refcount, "Vertex shader has %u references left.\n", refcount);
        }
        else
        {
            todo_wine_if(feature_level >= D3D_FEATURE_LEVEL_10_0)
            ok(hr == E_INVALIDARG, "Created a SM4.1 vertex shader, hr %#x, feature level %#x.\n", hr, feature_level);
            if (SUCCEEDED(hr))
                ID3D11VertexShader_Release(vs);
        }

        /* pixel shader */
        expected_refcount = get_refcount((IUnknown *)device) + (feature_level >= D3D_FEATURE_LEVEL_10_0);
        hr = ID3D11Device_CreatePixelShader(device, ps_4_0, sizeof(ps_4_0), NULL, &ps);
        if (feature_level >= D3D_FEATURE_LEVEL_10_0)
            ok(SUCCEEDED(hr), "Failed to create SM4 pixel shader, hr %#x, feature level %#x.\n", hr, feature_level);
        else
            ok(hr == E_INVALIDARG, "Created a SM4 pixel shader, hr %#x, feature level %#x.\n", hr, feature_level);

        refcount = get_refcount((IUnknown *)device);
        ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n",
                refcount, expected_refcount);
        if (feature_level >= D3D_FEATURE_LEVEL_10_0)
        {
            tmp = NULL;
            expected_refcount = refcount + 1;
            ID3D11PixelShader_GetDevice(ps, &tmp);
            ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
            refcount = get_refcount((IUnknown *)device);
            ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n",
                    refcount, expected_refcount);
            ID3D11Device_Release(tmp);

            hr = ID3D11PixelShader_QueryInterface(ps, &IID_ID3D10PixelShader, (void **)&iface);
            ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
                    "Pixel shader should implement ID3D10PixelShader.\n");
            if (SUCCEEDED(hr)) IUnknown_Release(iface);

            refcount = ID3D11PixelShader_Release(ps);
            ok(!refcount, "Pixel shader has %u references left.\n", refcount);
        }

        hr = ID3D11Device_CreatePixelShader(device, ps_4_1, sizeof(ps_4_1), NULL, &ps);
        if (feature_level >= D3D_FEATURE_LEVEL_10_1)
        {
            ok(SUCCEEDED(hr), "Failed to create SM4.1 pixel shader, hr %#x, feature level %#x.\n",
                    hr, feature_level);
            refcount = ID3D11PixelShader_Release(ps);
            ok(!refcount, "Pixel shader has %u references left.\n", refcount);
        }
        else
        {
            todo_wine_if(feature_level >= D3D_FEATURE_LEVEL_10_0)
            ok(hr == E_INVALIDARG, "Created a SM4.1 pixel shader, hr %#x, feature level %#x.\n", hr, feature_level);
            if (SUCCEEDED(hr))
                ID3D11PixelShader_Release(ps);
        }

        /* geometry shader */
        expected_refcount = get_refcount((IUnknown *)device) + (feature_level >= D3D_FEATURE_LEVEL_10_0);
        hr = ID3D11Device_CreateGeometryShader(device, gs_4_0, sizeof(gs_4_0), NULL, &gs);
        if (feature_level >= D3D_FEATURE_LEVEL_10_0)
            ok(SUCCEEDED(hr), "Failed to create SM4 geometry shader, hr %#x, feature level %#x.\n", hr, feature_level);
        else
            ok(hr == E_INVALIDARG, "Created a SM4 geometry shader, hr %#x, feature level %#x.\n", hr, feature_level);

        refcount = get_refcount((IUnknown *)device);
        ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n",
                refcount, expected_refcount);
        if (feature_level >= D3D_FEATURE_LEVEL_10_0)
        {
            tmp = NULL;
            expected_refcount = refcount + 1;
            ID3D11GeometryShader_GetDevice(gs, &tmp);
            ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
            refcount = get_refcount((IUnknown *)device);
            ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n",
                    refcount, expected_refcount);
            ID3D11Device_Release(tmp);

            hr = ID3D11GeometryShader_QueryInterface(gs, &IID_ID3D10GeometryShader, (void **)&iface);
            ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
                    "Geometry shader should implement ID3D10GeometryShader.\n");
            if (SUCCEEDED(hr)) IUnknown_Release(iface);

            refcount = ID3D11GeometryShader_Release(gs);
            ok(!refcount, "Geometry shader has %u references left.\n", refcount);
        }

        hr = ID3D11Device_CreateGeometryShader(device, gs_4_1, sizeof(gs_4_1), NULL, &gs);
        if (feature_level >= D3D_FEATURE_LEVEL_10_1)
        {
            ok(SUCCEEDED(hr), "Failed to create SM4.1 geometry shader, hr %#x, feature level %#x.\n",
                    hr, feature_level);
            refcount = ID3D11GeometryShader_Release(gs);
            ok(!refcount, "Geometry shader has %u references left.\n", refcount);
        }
        else
        {
            todo_wine_if(feature_level >= D3D_FEATURE_LEVEL_10_0)
            ok(hr == E_INVALIDARG, "Created a SM4.1 geometry shader, hr %#x, feature level %#x.\n",
                    hr, feature_level);
            if (SUCCEEDED(hr))
                ID3D11GeometryShader_Release(gs);
        }

        refcount = ID3D11Device_Release(device);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
}

static void test_create_sampler_state(void)
{
    static const struct test
    {
        D3D11_FILTER filter;
        D3D10_FILTER expected_filter;
    }
    desc_conversion_tests[] =
    {
        {D3D11_FILTER_MIN_MAG_MIP_POINT, D3D10_FILTER_MIN_MAG_MIP_POINT},
        {D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR, D3D10_FILTER_MIN_MAG_POINT_MIP_LINEAR},
        {D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT, D3D10_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT},
        {D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR, D3D10_FILTER_MIN_POINT_MAG_MIP_LINEAR},
        {D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT, D3D10_FILTER_MIN_LINEAR_MAG_MIP_POINT},
        {D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR, D3D10_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR},
        {D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D10_FILTER_MIN_MAG_LINEAR_MIP_POINT},
        {D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D10_FILTER_MIN_MAG_MIP_LINEAR},
        {D3D11_FILTER_ANISOTROPIC, D3D10_FILTER_ANISOTROPIC},
        {D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT, D3D10_FILTER_COMPARISON_MIN_MAG_MIP_POINT},
        {D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR, D3D10_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR},
        {
            D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT,
            D3D10_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT
        },
        {D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR, D3D10_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR},
        {D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT, D3D10_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT},
        {
            D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
            D3D10_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR
        },
        {D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, D3D10_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT},
        {D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D10_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR},
        {D3D11_FILTER_COMPARISON_ANISOTROPIC, D3D10_FILTER_COMPARISON_ANISOTROPIC},
    };

    ID3D11SamplerState *sampler_state1, *sampler_state2;
    ID3D10SamplerState *d3d10_sampler_state;
    ULONG refcount, expected_refcount;
    ID3D11Device *device, *tmp;
    D3D11_SAMPLER_DESC desc;
    unsigned int i;
    HRESULT hr;

    if (!(device = create_device(NULL)))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = ID3D11Device_CreateSamplerState(device, NULL, &sampler_state1);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.MipLODBias = 0.0f;
    desc.MaxAnisotropy = 16;
    desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    desc.BorderColor[0] = 0.0f;
    desc.BorderColor[1] = 1.0f;
    desc.BorderColor[2] = 0.0f;
    desc.BorderColor[3] = 1.0f;
    desc.MinLOD = 0.0f;
    desc.MaxLOD = 16.0f;

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D11Device_CreateSamplerState(device, &desc, &sampler_state1);
    ok(SUCCEEDED(hr), "Failed to create sampler state, hr %#x.\n", hr);
    hr = ID3D11Device_CreateSamplerState(device, &desc, &sampler_state2);
    ok(SUCCEEDED(hr), "Failed to create sampler state, hr %#x.\n", hr);
    ok(sampler_state1 == sampler_state2, "Got different sampler state objects.\n");
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D11SamplerState_GetDevice(sampler_state1, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D11Device_Release(tmp);

    ID3D11SamplerState_GetDesc(sampler_state1, &desc);
    ok(desc.Filter == D3D11_FILTER_MIN_MAG_MIP_LINEAR, "Got unexpected filter %#x.\n", desc.Filter);
    ok(desc.AddressU == D3D11_TEXTURE_ADDRESS_WRAP, "Got unexpected address u %u.\n", desc.AddressU);
    ok(desc.AddressV == D3D11_TEXTURE_ADDRESS_WRAP, "Got unexpected address v %u.\n", desc.AddressV);
    ok(desc.AddressW == D3D11_TEXTURE_ADDRESS_WRAP, "Got unexpected address w %u.\n", desc.AddressW);
    ok(!desc.MipLODBias, "Got unexpected mip LOD bias %f.\n", desc.MipLODBias);
    ok(!desc.MaxAnisotropy, "Got unexpected max anisotropy %u.\n", desc.MaxAnisotropy);
    ok(desc.ComparisonFunc == D3D11_COMPARISON_NEVER, "Got unexpected comparison func %u.\n", desc.ComparisonFunc);
    ok(!desc.BorderColor[0] && !desc.BorderColor[1] && !desc.BorderColor[2] && !desc.BorderColor[3],
            "Got unexpected border color {%.8e, %.8e, %.8e, %.8e}.\n",
            desc.BorderColor[0], desc.BorderColor[1], desc.BorderColor[2], desc.BorderColor[3]);
    ok(!desc.MinLOD, "Got unexpected min LOD %f.\n", desc.MinLOD);
    ok(desc.MaxLOD == 16.0f, "Got unexpected max LOD %f.\n", desc.MaxLOD);

    refcount = ID3D11SamplerState_Release(sampler_state2);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    refcount = ID3D11SamplerState_Release(sampler_state1);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    for (i = 0; i < sizeof(desc_conversion_tests) / sizeof(*desc_conversion_tests); ++i)
    {
        const struct test *current = &desc_conversion_tests[i];
        D3D10_SAMPLER_DESC d3d10_desc, expected_desc;

        desc.Filter = current->filter;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
        desc.MipLODBias = 0.0f;
        desc.MaxAnisotropy = 16;
        desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        desc.BorderColor[0] = 0.0f;
        desc.BorderColor[1] = 1.0f;
        desc.BorderColor[2] = 0.0f;
        desc.BorderColor[3] = 1.0f;
        desc.MinLOD = 0.0f;
        desc.MaxLOD = 16.0f;

        hr = ID3D11Device_CreateSamplerState(device, &desc, &sampler_state1);
        ok(SUCCEEDED(hr), "Test %u: Failed to create sampler state, hr %#x.\n", i, hr);

        hr = ID3D11SamplerState_QueryInterface(sampler_state1, &IID_ID3D10SamplerState,
                (void **)&d3d10_sampler_state);
        ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
                "Test %u: Sampler state should implement ID3D10SamplerState.\n", i);
        if (FAILED(hr))
        {
            win_skip("Sampler state does not implement ID3D10SamplerState.\n");
            ID3D11SamplerState_Release(sampler_state1);
            break;
        }

        memcpy(&expected_desc, &desc, sizeof(expected_desc));
        expected_desc.Filter = current->expected_filter;
        if (!D3D11_DECODE_IS_ANISOTROPIC_FILTER(current->filter))
            expected_desc.MaxAnisotropy = 0;
        if (!D3D11_DECODE_IS_COMPARISON_FILTER(current->filter))
            expected_desc.ComparisonFunc = D3D10_COMPARISON_NEVER;

        ID3D10SamplerState_GetDesc(d3d10_sampler_state, &d3d10_desc);
        ok(d3d10_desc.Filter == expected_desc.Filter,
                "Test %u: Got unexpected filter %#x.\n", i, d3d10_desc.Filter);
        ok(d3d10_desc.AddressU == expected_desc.AddressU,
                "Test %u: Got unexpected address u %u.\n", i, d3d10_desc.AddressU);
        ok(d3d10_desc.AddressV == expected_desc.AddressV,
                "Test %u: Got unexpected address v %u.\n", i, d3d10_desc.AddressV);
        ok(d3d10_desc.AddressW == expected_desc.AddressW,
                "Test %u: Got unexpected address w %u.\n", i, d3d10_desc.AddressW);
        ok(d3d10_desc.MipLODBias == expected_desc.MipLODBias,
                "Test %u: Got unexpected mip LOD bias %f.\n", i, d3d10_desc.MipLODBias);
        ok(d3d10_desc.MaxAnisotropy == expected_desc.MaxAnisotropy,
                "Test %u: Got unexpected max anisotropy %u.\n", i, d3d10_desc.MaxAnisotropy);
        ok(d3d10_desc.ComparisonFunc == expected_desc.ComparisonFunc,
                "Test %u: Got unexpected comparison func %u.\n", i, d3d10_desc.ComparisonFunc);
        ok(d3d10_desc.BorderColor[0] == expected_desc.BorderColor[0]
                && d3d10_desc.BorderColor[1] == expected_desc.BorderColor[1]
                && d3d10_desc.BorderColor[2] == expected_desc.BorderColor[2]
                && d3d10_desc.BorderColor[3] == expected_desc.BorderColor[3],
                "Test %u: Got unexpected border color {%.8e, %.8e, %.8e, %.8e}.\n", i,
                d3d10_desc.BorderColor[0], d3d10_desc.BorderColor[1],
                d3d10_desc.BorderColor[2], d3d10_desc.BorderColor[3]);
        ok(d3d10_desc.MinLOD == expected_desc.MinLOD,
                "Test %u: Got unexpected min LOD %f.\n", i, d3d10_desc.MinLOD);
        ok(d3d10_desc.MaxLOD == expected_desc.MaxLOD,
                "Test %u: Got unexpected max LOD %f.\n", i, d3d10_desc.MaxLOD);

        refcount = ID3D10SamplerState_Release(d3d10_sampler_state);
        ok(refcount == 1, "Test %u: Got unexpected refcount %u.\n", i, refcount);
        refcount = ID3D11SamplerState_Release(sampler_state1);
        ok(!refcount, "Test %u: Got unexpected refcount %u.\n", i, refcount);
    }

    refcount = ID3D11Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_blend_state(void)
{
    static const D3D11_BLEND_DESC desc_conversion_tests[] =
    {
        {
            FALSE, FALSE,
            {
                {
                    FALSE, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD,
                    D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD
                },
            },
        },
        {
            FALSE, TRUE,
            {
                {
                    TRUE, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD,
                    D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_ALL
                },
                {
                    FALSE, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD,
                    D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_RED
                },
                {
                    TRUE, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD,
                    D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_ALL
                },
                {
                    FALSE, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD,
                    D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_GREEN
                },
                {
                    TRUE, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD,
                    D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_ALL
                },
                {
                    TRUE, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD,
                    D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_ALL
                },
                {
                    TRUE, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD,
                    D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_ALL
                },
                {
                    TRUE, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD,
                    D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_ALL
                },
            },
        },
        {
            FALSE, TRUE,
            {
                {
                    TRUE, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD,
                    D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_ALL
                },
                {
                    TRUE, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_SUBTRACT,
                    D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_ALL
                },
                {
                    TRUE, D3D11_BLEND_ZERO, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD,
                    D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_ALL
                },
                {
                    TRUE, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD,
                    D3D11_BLEND_ZERO, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_ALL
                },
                {
                    TRUE, D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_OP_MAX,
                    D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_ALL
                },
                {
                    TRUE, D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_OP_MIN,
                    D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_ALL
                },
                {
                    FALSE, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD,
                    D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_ALL
                },
                {
                    FALSE, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD,
                    D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_ALL
                },
            },
        },
    };

    ID3D11BlendState *blend_state1, *blend_state2;
    D3D11_BLEND_DESC desc, obtained_desc;
    ID3D10BlendState *d3d10_blend_state;
    D3D10_BLEND_DESC d3d10_blend_desc;
    ULONG refcount, expected_refcount;
    ID3D11Device *device, *tmp;
    unsigned int i, j;
    IUnknown *iface;
    HRESULT hr;

    if (!(device = create_device(NULL)))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = ID3D11Device_CreateBlendState(device, NULL, &blend_state1);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    memset(&desc, 0, sizeof(desc));
    desc.AlphaToCoverageEnable = FALSE;
    desc.IndependentBlendEnable = FALSE;
    desc.RenderTarget[0].BlendEnable = FALSE;
    desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
    desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D11Device_CreateBlendState(device, &desc, &blend_state1);
    ok(SUCCEEDED(hr), "Failed to create blend state, hr %#x.\n", hr);
    hr = ID3D11Device_CreateBlendState(device, &desc, &blend_state2);
    ok(SUCCEEDED(hr), "Failed to create blend state, hr %#x.\n", hr);
    ok(blend_state1 == blend_state2, "Got different blend state objects.\n");
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D11BlendState_GetDevice(blend_state1, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D11Device_Release(tmp);

    ID3D11BlendState_GetDesc(blend_state1, &obtained_desc);
    ok(obtained_desc.AlphaToCoverageEnable == FALSE, "Got unexpected alpha to coverage enable %#x.\n",
            obtained_desc.AlphaToCoverageEnable);
    ok(obtained_desc.IndependentBlendEnable == FALSE, "Got unexpected independent blend enable %#x.\n",
            obtained_desc.IndependentBlendEnable);
    for (i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        ok(obtained_desc.RenderTarget[i].BlendEnable == FALSE,
                "Got unexpected blend enable %#x for render target %u.\n",
                obtained_desc.RenderTarget[i].BlendEnable, i);
        ok(obtained_desc.RenderTarget[i].SrcBlend == D3D11_BLEND_ONE,
                "Got unexpected src blend %u for render target %u.\n",
                obtained_desc.RenderTarget[i].SrcBlend, i);
        ok(obtained_desc.RenderTarget[i].DestBlend == D3D11_BLEND_ZERO,
                "Got unexpected dest blend %u for render target %u.\n",
                obtained_desc.RenderTarget[i].DestBlend, i);
        ok(obtained_desc.RenderTarget[i].BlendOp == D3D11_BLEND_OP_ADD,
                "Got unexpected blend op %u for render target %u.\n",
                obtained_desc.RenderTarget[i].BlendOp, i);
        ok(obtained_desc.RenderTarget[i].SrcBlendAlpha == D3D11_BLEND_ONE,
                "Got unexpected src blend alpha %u for render target %u.\n",
                obtained_desc.RenderTarget[i].SrcBlendAlpha, i);
        ok(obtained_desc.RenderTarget[i].DestBlendAlpha == D3D11_BLEND_ZERO,
                "Got unexpected dest blend alpha %u for render target %u.\n",
                obtained_desc.RenderTarget[i].DestBlendAlpha, i);
        ok(obtained_desc.RenderTarget[i].BlendOpAlpha == D3D11_BLEND_OP_ADD,
                "Got unexpected blend op alpha %u for render target %u.\n",
                obtained_desc.RenderTarget[i].BlendOpAlpha, i);
        ok(obtained_desc.RenderTarget[i].RenderTargetWriteMask == D3D11_COLOR_WRITE_ENABLE_ALL,
                "Got unexpected render target write mask %#x for render target %u.\n",
                obtained_desc.RenderTarget[0].RenderTargetWriteMask, i);
    }

    hr = ID3D11BlendState_QueryInterface(blend_state1, &IID_ID3D10BlendState, (void **)&iface);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Blend state should implement ID3D10BlendState.\n");
    if (SUCCEEDED(hr)) IUnknown_Release(iface);
    hr = ID3D11BlendState_QueryInterface(blend_state1, &IID_ID3D10BlendState1, (void **)&iface);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Blend state should implement ID3D10BlendState1.\n");
    if (SUCCEEDED(hr)) IUnknown_Release(iface);

    refcount = ID3D11BlendState_Release(blend_state1);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    refcount = ID3D11BlendState_Release(blend_state2);
    ok(!refcount, "Blend state has %u references left.\n", refcount);

    for (i = 0; i < sizeof(desc_conversion_tests) / sizeof(*desc_conversion_tests); ++i)
    {
        const D3D11_BLEND_DESC *current_desc = &desc_conversion_tests[i];

        hr = ID3D11Device_CreateBlendState(device, current_desc, &blend_state1);
        ok(SUCCEEDED(hr), "Failed to create blend state, hr %#x.\n", hr);

        hr = ID3D11BlendState_QueryInterface(blend_state1, &IID_ID3D10BlendState, (void **)&d3d10_blend_state);
        ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
                "Blend state should implement ID3D10BlendState.\n");
        if (FAILED(hr))
        {
            win_skip("Blend state does not implement ID3D10BlendState.\n");
            ID3D11BlendState_Release(blend_state1);
            break;
        }

        ID3D10BlendState_GetDesc(d3d10_blend_state, &d3d10_blend_desc);
        ok(d3d10_blend_desc.AlphaToCoverageEnable == current_desc->AlphaToCoverageEnable,
                "Got unexpected alpha to coverage enable %#x for test %u.\n",
                d3d10_blend_desc.AlphaToCoverageEnable, i);
        ok(d3d10_blend_desc.SrcBlend == (D3D10_BLEND)current_desc->RenderTarget[0].SrcBlend,
                "Got unexpected src blend %u for test %u.\n", d3d10_blend_desc.SrcBlend, i);
        ok(d3d10_blend_desc.DestBlend == (D3D10_BLEND)current_desc->RenderTarget[0].DestBlend,
                "Got unexpected dest blend %u for test %u.\n", d3d10_blend_desc.DestBlend, i);
        ok(d3d10_blend_desc.BlendOp == (D3D10_BLEND_OP)current_desc->RenderTarget[0].BlendOp,
                "Got unexpected blend op %u for test %u.\n", d3d10_blend_desc.BlendOp, i);
        ok(d3d10_blend_desc.SrcBlendAlpha == (D3D10_BLEND)current_desc->RenderTarget[0].SrcBlendAlpha,
                "Got unexpected src blend alpha %u for test %u.\n", d3d10_blend_desc.SrcBlendAlpha, i);
        ok(d3d10_blend_desc.DestBlendAlpha == (D3D10_BLEND)current_desc->RenderTarget[0].DestBlendAlpha,
                "Got unexpected dest blend alpha %u for test %u.\n", d3d10_blend_desc.DestBlendAlpha, i);
        ok(d3d10_blend_desc.BlendOpAlpha == (D3D10_BLEND_OP)current_desc->RenderTarget[0].BlendOpAlpha,
                "Got unexpected blend op alpha %u for test %u.\n", d3d10_blend_desc.BlendOpAlpha, i);
        for (j = 0; j < D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT; j++)
        {
            unsigned int k = current_desc->IndependentBlendEnable ? j : 0;
            ok(d3d10_blend_desc.BlendEnable[j] == current_desc->RenderTarget[k].BlendEnable,
                    "Got unexpected blend enable %#x for test %u, render target %u.\n",
                    d3d10_blend_desc.BlendEnable[j], i, j);
            ok(d3d10_blend_desc.RenderTargetWriteMask[j] == current_desc->RenderTarget[k].RenderTargetWriteMask,
                    "Got unexpected render target write mask %#x for test %u, render target %u.\n",
                    d3d10_blend_desc.RenderTargetWriteMask[j], i, j);
        }

        ID3D10BlendState_Release(d3d10_blend_state);

        refcount = ID3D11BlendState_Release(blend_state1);
        ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    }

    refcount = ID3D11Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_depthstencil_state(void)
{
    ID3D11DepthStencilState *ds_state1, *ds_state2;
    ID3D10DepthStencilState *d3d10_ds_state;
    ULONG refcount, expected_refcount;
    D3D11_DEPTH_STENCIL_DESC ds_desc;
    ID3D11Device *device, *tmp;
    HRESULT hr;

    if (!(device = create_device(NULL)))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = ID3D11Device_CreateDepthStencilState(device, NULL, &ds_state1);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    ds_desc.DepthEnable = TRUE;
    ds_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    ds_desc.DepthFunc = D3D11_COMPARISON_LESS;
    ds_desc.StencilEnable = FALSE;
    ds_desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
    ds_desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
    ds_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    ds_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    ds_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    ds_desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    ds_desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    ds_desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    ds_desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    ds_desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D11Device_CreateDepthStencilState(device, &ds_desc, &ds_state1);
    ok(SUCCEEDED(hr), "Failed to create depthstencil state, hr %#x.\n", hr);
    hr = ID3D11Device_CreateDepthStencilState(device, &ds_desc, &ds_state2);
    ok(SUCCEEDED(hr), "Failed to create depthstencil state, hr %#x.\n", hr);
    ok(ds_state1 == ds_state2, "Got different depthstencil state objects.\n");
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D11DepthStencilState_GetDevice(ds_state1, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D11Device_Release(tmp);

    hr = ID3D11DepthStencilState_QueryInterface(ds_state1, &IID_ID3D10DepthStencilState, (void **)&d3d10_ds_state);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Depth stencil state should implement ID3D10DepthStencilState.\n");
    if (SUCCEEDED(hr)) ID3D10DepthStencilState_Release(d3d10_ds_state);

    refcount = ID3D11DepthStencilState_Release(ds_state2);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    refcount = ID3D11DepthStencilState_Release(ds_state1);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    ds_desc.DepthEnable = FALSE;
    ds_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    ds_desc.DepthFunc = D3D11_COMPARISON_NEVER;
    ds_desc.StencilEnable = FALSE;
    ds_desc.StencilReadMask = 0;
    ds_desc.StencilWriteMask = 0;
    ds_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_ZERO;
    ds_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_ZERO;
    ds_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_ZERO;
    ds_desc.FrontFace.StencilFunc = D3D11_COMPARISON_NEVER;
    ds_desc.BackFace = ds_desc.FrontFace;

    hr = ID3D11Device_CreateDepthStencilState(device, &ds_desc, &ds_state1);
    ok(SUCCEEDED(hr), "Failed to create depthstencil state, hr %#x.\n", hr);

    memset(&ds_desc, 0, sizeof(ds_desc));
    ID3D11DepthStencilState_GetDesc(ds_state1, &ds_desc);
    ok(!ds_desc.DepthEnable, "Got unexpected depth enable %#x.\n", ds_desc.DepthEnable);
    ok(ds_desc.DepthWriteMask == D3D11_DEPTH_WRITE_MASK_ALL,
            "Got unexpected depth write mask %#x.\n", ds_desc.DepthWriteMask);
    ok(ds_desc.DepthFunc == D3D11_COMPARISON_LESS, "Got unexpected depth func %#x.\n", ds_desc.DepthFunc);
    ok(!ds_desc.StencilEnable, "Got unexpected stencil enable %#x.\n", ds_desc.StencilEnable);
    ok(ds_desc.StencilReadMask == D3D11_DEFAULT_STENCIL_READ_MASK,
            "Got unexpected stencil read mask %#x.\n", ds_desc.StencilReadMask);
    ok(ds_desc.StencilWriteMask == D3D11_DEFAULT_STENCIL_WRITE_MASK,
            "Got unexpected stencil write mask %#x.\n", ds_desc.StencilWriteMask);
    ok(ds_desc.FrontFace.StencilDepthFailOp == D3D11_STENCIL_OP_KEEP,
            "Got unexpected front face stencil depth fail op %#x.\n", ds_desc.FrontFace.StencilDepthFailOp);
    ok(ds_desc.FrontFace.StencilPassOp == D3D11_STENCIL_OP_KEEP,
            "Got unexpected front face stencil pass op %#x.\n", ds_desc.FrontFace.StencilPassOp);
    ok(ds_desc.FrontFace.StencilFailOp == D3D11_STENCIL_OP_KEEP,
            "Got unexpected front face stencil fail op %#x.\n", ds_desc.FrontFace.StencilFailOp);
    ok(ds_desc.FrontFace.StencilFunc == D3D11_COMPARISON_ALWAYS,
            "Got unexpected front face stencil func %#x.\n", ds_desc.FrontFace.StencilFunc);
    ok(ds_desc.BackFace.StencilDepthFailOp == D3D11_STENCIL_OP_KEEP,
            "Got unexpected back face stencil depth fail op %#x.\n", ds_desc.BackFace.StencilDepthFailOp);
    ok(ds_desc.BackFace.StencilPassOp == D3D11_STENCIL_OP_KEEP,
            "Got unexpected back face stencil pass op %#x.\n", ds_desc.BackFace.StencilPassOp);
    ok(ds_desc.BackFace.StencilFailOp == D3D11_STENCIL_OP_KEEP,
            "Got unexpected back face stencil fail op %#x.\n", ds_desc.BackFace.StencilFailOp);
    ok(ds_desc.BackFace.StencilFunc == D3D11_COMPARISON_ALWAYS,
            "Got unexpected back face stencil func %#x.\n", ds_desc.BackFace.StencilFunc);

    ID3D11DepthStencilState_Release(ds_state1);

    refcount = ID3D11Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_rasterizer_state(void)
{
    ID3D11RasterizerState *rast_state1, *rast_state2;
    ID3D10RasterizerState *d3d10_rast_state;
    ULONG refcount, expected_refcount;
    D3D10_RASTERIZER_DESC d3d10_desc;
    D3D11_RASTERIZER_DESC desc;
    ID3D11Device *device, *tmp;
    HRESULT hr;

    if (!(device = create_device(NULL)))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = ID3D11Device_CreateRasterizerState(device, NULL, &rast_state1);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    desc.FillMode = D3D11_FILL_SOLID;
    desc.CullMode = D3D11_CULL_BACK;
    desc.FrontCounterClockwise = FALSE;
    desc.DepthBias = 0;
    desc.DepthBiasClamp = 0.0f;
    desc.SlopeScaledDepthBias = 0.0f;
    desc.DepthClipEnable = TRUE;
    desc.ScissorEnable = FALSE;
    desc.MultisampleEnable = FALSE;
    desc.AntialiasedLineEnable = FALSE;

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D11Device_CreateRasterizerState(device, &desc, &rast_state1);
    ok(SUCCEEDED(hr), "Failed to create rasterizer state, hr %#x.\n", hr);
    hr = ID3D11Device_CreateRasterizerState(device, &desc, &rast_state2);
    ok(SUCCEEDED(hr), "Failed to create rasterizer state, hr %#x.\n", hr);
    ok(rast_state1 == rast_state2, "Got different rasterizer state objects.\n");
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D11RasterizerState_GetDevice(rast_state1, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D11Device_Release(tmp);

    hr = ID3D11RasterizerState_QueryInterface(rast_state1, &IID_ID3D10RasterizerState, (void **)&d3d10_rast_state);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Rasterizer state should implement ID3D10RasterizerState.\n");
    if (SUCCEEDED(hr))
    {
        ID3D10RasterizerState_GetDesc(d3d10_rast_state, &d3d10_desc);
        ok(d3d10_desc.FillMode == D3D10_FILL_SOLID, "Got unexpected fill mode %u.\n", d3d10_desc.FillMode);
        ok(d3d10_desc.CullMode == D3D10_CULL_BACK, "Got unexpected cull mode %u.\n", d3d10_desc.CullMode);
        ok(!d3d10_desc.FrontCounterClockwise, "Got unexpected front counter clockwise %#x.\n",
                d3d10_desc.FrontCounterClockwise);
        ok(!d3d10_desc.DepthBias, "Got unexpected depth bias %d.\n", d3d10_desc.DepthBias);
        ok(!d3d10_desc.DepthBiasClamp, "Got unexpected depth bias clamp %f.\n", d3d10_desc.DepthBiasClamp);
        ok(!d3d10_desc.SlopeScaledDepthBias, "Got unexpected slope scaled depth bias %f.\n",
                d3d10_desc.SlopeScaledDepthBias);
        ok(!!d3d10_desc.DepthClipEnable, "Got unexpected depth clip enable %#x.\n", d3d10_desc.DepthClipEnable);
        ok(!d3d10_desc.ScissorEnable, "Got unexpected scissor enable %#x.\n", d3d10_desc.ScissorEnable);
        ok(!d3d10_desc.MultisampleEnable, "Got unexpected multisample enable %#x.\n",
                d3d10_desc.MultisampleEnable);
        ok(!d3d10_desc.AntialiasedLineEnable, "Got unexpected antialiased line enable %#x.\n",
                d3d10_desc.AntialiasedLineEnable);

        refcount = ID3D10RasterizerState_Release(d3d10_rast_state);
        ok(refcount == 2, "Got unexpected refcount %u.\n", refcount);
    }

    refcount = ID3D11RasterizerState_Release(rast_state2);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    refcount = ID3D11RasterizerState_Release(rast_state1);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    refcount = ID3D11Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_query(void)
{
    static const struct
    {
        D3D11_QUERY query;
        D3D_FEATURE_LEVEL required_feature_level;
        BOOL is_predicate;
        BOOL can_use_create_predicate;
        BOOL todo;
    }
    tests[] =
    {
        {D3D11_QUERY_EVENT,                         D3D_FEATURE_LEVEL_10_0, FALSE, FALSE, FALSE},
        {D3D11_QUERY_OCCLUSION,                     D3D_FEATURE_LEVEL_10_0, FALSE, FALSE, FALSE},
        {D3D11_QUERY_TIMESTAMP,                     D3D_FEATURE_LEVEL_10_0, FALSE, FALSE, FALSE},
        {D3D11_QUERY_TIMESTAMP_DISJOINT,            D3D_FEATURE_LEVEL_10_0, FALSE, FALSE, FALSE},
        {D3D11_QUERY_PIPELINE_STATISTICS,           D3D_FEATURE_LEVEL_10_0, FALSE, FALSE, TRUE},
        {D3D11_QUERY_OCCLUSION_PREDICATE,           D3D_FEATURE_LEVEL_10_0, TRUE,  TRUE,  FALSE},
        {D3D11_QUERY_SO_STATISTICS,                 D3D_FEATURE_LEVEL_10_0, FALSE, FALSE, TRUE},
        {D3D11_QUERY_SO_OVERFLOW_PREDICATE,         D3D_FEATURE_LEVEL_10_0, TRUE,  TRUE,  TRUE},
        {D3D11_QUERY_SO_STATISTICS_STREAM0,         D3D_FEATURE_LEVEL_11_0, FALSE, FALSE, TRUE},
        {D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM0, D3D_FEATURE_LEVEL_11_0, TRUE,  FALSE, TRUE},
        {D3D11_QUERY_SO_STATISTICS_STREAM1,         D3D_FEATURE_LEVEL_11_0, FALSE, FALSE, TRUE},
        {D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM1, D3D_FEATURE_LEVEL_11_0, TRUE,  FALSE, TRUE},
        {D3D11_QUERY_SO_STATISTICS_STREAM2,         D3D_FEATURE_LEVEL_11_0, FALSE, FALSE, TRUE},
        {D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM2, D3D_FEATURE_LEVEL_11_0, TRUE,  FALSE, TRUE},
        {D3D11_QUERY_SO_STATISTICS_STREAM3,         D3D_FEATURE_LEVEL_11_0, FALSE, FALSE, TRUE},
        {D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM3, D3D_FEATURE_LEVEL_11_0, TRUE,  FALSE, TRUE},
    };

    ULONG refcount, expected_refcount;
    D3D_FEATURE_LEVEL feature_level;
    D3D11_QUERY_DESC query_desc;
    ID3D11Predicate *predicate;
    ID3D11Device *device, *tmp;
    HRESULT hr, expected_hr;
    ID3D11Query *query;
    IUnknown *iface;
    unsigned int i;

    if (!(device = create_device(NULL)))
    {
        skip("Failed to create device.\n");
        return;
    }
    feature_level = ID3D11Device_GetFeatureLevel(device);

    hr = ID3D11Device_CreateQuery(device, NULL, &query);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    hr = ID3D11Device_CreatePredicate(device, NULL, &predicate);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    for (i = 0; i < sizeof(tests) / sizeof(*tests); ++i)
    {
        if (tests[i].required_feature_level > feature_level)
        {
            skip("Query type %u requires feature level %#x.\n", tests[i].query, tests[i].required_feature_level);
            continue;
        }

        query_desc.Query = tests[i].query;
        query_desc.MiscFlags = 0;

        hr = ID3D11Device_CreateQuery(device, &query_desc, NULL);
        todo_wine_if(tests[i].todo)
        ok(hr == S_FALSE, "Got unexpected hr %#x for query type %u.\n", hr, query_desc.Query);

        query_desc.Query = tests[i].query;
        hr = ID3D11Device_CreateQuery(device, &query_desc, &query);
        todo_wine_if(tests[i].todo)
        ok(hr == S_OK, "Got unexpected hr %#x for query type %u.\n", hr, query_desc.Query);
        if (FAILED(hr))
            continue;

        expected_hr = tests[i].is_predicate ? S_OK : E_NOINTERFACE;
        hr = ID3D11Query_QueryInterface(query, &IID_ID3D11Predicate, (void **)&predicate);
        ID3D11Query_Release(query);
        ok(hr == expected_hr, "Got unexpected hr %#x for query type %u.\n", hr, query_desc.Query);
        if (SUCCEEDED(hr))
            ID3D11Predicate_Release(predicate);

        expected_hr = tests[i].can_use_create_predicate ? S_FALSE : E_INVALIDARG;
        hr = ID3D11Device_CreatePredicate(device, &query_desc, NULL);
        ok(hr == expected_hr, "Got unexpected hr %#x for query type %u.\n", hr, query_desc.Query);

        expected_hr = tests[i].can_use_create_predicate ? S_OK : E_INVALIDARG;
        hr = ID3D11Device_CreatePredicate(device, &query_desc, &predicate);
        ok(hr == expected_hr, "Got unexpected hr %#x for query type %u.\n", hr, query_desc.Query);
        if (SUCCEEDED(hr))
            ID3D11Predicate_Release(predicate);
    }

    query_desc.Query = D3D11_QUERY_OCCLUSION_PREDICATE;
    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D11Device_CreatePredicate(device, &query_desc, &predicate);
    ok(SUCCEEDED(hr), "Failed to create predicate, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D11Predicate_GetDevice(predicate, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D11Device_Release(tmp);
    hr = ID3D11Predicate_QueryInterface(predicate, &IID_ID3D10Predicate, (void **)&iface);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Predicate should implement ID3D10Predicate.\n");
    if (SUCCEEDED(hr)) IUnknown_Release(iface);
    ID3D11Predicate_Release(predicate);

    refcount = ID3D11Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_device_removed_reason(void)
{
    ID3D11Device *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device(NULL)))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = ID3D11Device_GetDeviceRemovedReason(device);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = ID3D11Device_GetDeviceRemovedReason(device);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    refcount = ID3D11Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_private_data(void)
{
    ULONG refcount, expected_refcount;
    D3D11_TEXTURE2D_DESC texture_desc;
    ID3D10Texture2D *d3d10_texture;
    ID3D11Device *test_object;
    ID3D11Texture2D *texture;
    IDXGIDevice *dxgi_device;
    IDXGISurface *surface;
    ID3D11Device *device;
    IUnknown *ptr;
    HRESULT hr;
    UINT size;

    static const GUID test_guid =
            {0xfdb37466, 0x428f, 0x4edf, {0xa3, 0x7f, 0x9b, 0x1d, 0xf4, 0x88, 0xc5, 0xfc}};
    static const GUID test_guid2 =
            {0x2e5afac2, 0x87b5, 0x4c10, {0x9b, 0x4b, 0x89, 0xd7, 0xd1, 0x12, 0xe7, 0x2b}};
    static const DWORD data[] = {1, 2, 3, 4};

    if (!(device = create_device(NULL)))
    {
        skip("Failed to create device.\n");
        return;
    }

    test_object = create_device(NULL);

    texture_desc.Width = 512;
    texture_desc.Height = 512;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    hr = ID3D11Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    hr = ID3D11Texture2D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface);
    ok(SUCCEEDED(hr), "Failed to get IDXGISurface, hr %#x.\n", hr);

    hr = ID3D11Device_SetPrivateData(device, &test_guid, 0, NULL);
    ok(hr == S_FALSE, "Got unexpected hr %#x.\n", hr);
    hr = ID3D11Device_SetPrivateDataInterface(device, &test_guid, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = ID3D11Device_SetPrivateData(device, &test_guid, ~0u, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = ID3D11Device_SetPrivateData(device, &test_guid, ~0u, NULL);
    ok(hr == S_FALSE, "Got unexpected hr %#x.\n", hr);

    hr = ID3D11Device_SetPrivateDataInterface(device, &test_guid, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    size = sizeof(ptr) * 2;
    ptr = (IUnknown *)0xdeadbeef;
    hr = ID3D11Device_GetPrivateData(device, &test_guid, &size, &ptr);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(!ptr, "Got unexpected pointer %p.\n", ptr);
    ok(size == sizeof(IUnknown *), "Got unexpected size %u.\n", size);

    hr = ID3D11Device_QueryInterface(device, &IID_IDXGIDevice, (void **)&dxgi_device);
    ok(SUCCEEDED(hr), "Failed to get DXGI device, hr %#x.\n", hr);
    size = sizeof(ptr) * 2;
    ptr = (IUnknown *)0xdeadbeef;
    hr = IDXGIDevice_GetPrivateData(dxgi_device, &test_guid, &size, &ptr);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(!ptr, "Got unexpected pointer %p.\n", ptr);
    ok(size == sizeof(IUnknown *), "Got unexpected size %u.\n", size);
    IDXGIDevice_Release(dxgi_device);

    refcount = get_refcount((IUnknown *)test_object);
    hr = ID3D11Device_SetPrivateDataInterface(device, &test_guid, (IUnknown *)test_object);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    expected_refcount = refcount + 1;
    refcount = get_refcount((IUnknown *)test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    hr = ID3D11Device_SetPrivateDataInterface(device, &test_guid, (IUnknown *)test_object);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);

    hr = ID3D11Device_SetPrivateDataInterface(device, &test_guid, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    --expected_refcount;
    refcount = get_refcount((IUnknown *)test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);

    hr = ID3D11Device_SetPrivateDataInterface(device, &test_guid, (IUnknown *)test_object);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    size = sizeof(data);
    hr = ID3D11Device_SetPrivateData(device, &test_guid, size, data);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    hr = ID3D11Device_SetPrivateData(device, &test_guid, 42, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = ID3D11Device_SetPrivateData(device, &test_guid, 42, NULL);
    ok(hr == S_FALSE, "Got unexpected hr %#x.\n", hr);

    hr = ID3D11Device_SetPrivateDataInterface(device, &test_guid, (IUnknown *)test_object);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ++expected_refcount;
    size = 2 * sizeof(ptr);
    ptr = NULL;
    hr = ID3D11Device_GetPrivateData(device, &test_guid, &size, &ptr);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(size == sizeof(test_object), "Got unexpected size %u.\n", size);
    ++expected_refcount;
    refcount = get_refcount((IUnknown *)test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    IUnknown_Release(ptr);
    --expected_refcount;

    ptr = (IUnknown *)0xdeadbeef;
    size = 1;
    hr = ID3D11Device_GetPrivateData(device, &test_guid, &size, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(size == sizeof(device), "Got unexpected size %u.\n", size);
    size = 2 * sizeof(ptr);
    hr = ID3D11Device_GetPrivateData(device, &test_guid, &size, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(size == sizeof(device), "Got unexpected size %u.\n", size);
    refcount = get_refcount((IUnknown *)test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);

    size = 1;
    hr = ID3D11Device_GetPrivateData(device, &test_guid, &size, &ptr);
    ok(hr == DXGI_ERROR_MORE_DATA, "Got unexpected hr %#x.\n", hr);
    ok(size == sizeof(device), "Got unexpected size %u.\n", size);
    ok(ptr == (IUnknown *)0xdeadbeef, "Got unexpected pointer %p.\n", ptr);
    hr = ID3D11Device_GetPrivateData(device, &test_guid2, NULL, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    size = 0xdeadbabe;
    hr = ID3D11Device_GetPrivateData(device, &test_guid2, &size, &ptr);
    ok(hr == DXGI_ERROR_NOT_FOUND, "Got unexpected hr %#x.\n", hr);
    ok(size == 0, "Got unexpected size %u.\n", size);
    ok(ptr == (IUnknown *)0xdeadbeef, "Got unexpected pointer %p.\n", ptr);
    hr = ID3D11Device_GetPrivateData(device, &test_guid, NULL, &ptr);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ok(ptr == (IUnknown *)0xdeadbeef, "Got unexpected pointer %p.\n", ptr);

    hr = ID3D11Texture2D_SetPrivateDataInterface(texture, &test_guid, (IUnknown *)test_object);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ptr = NULL;
    size = sizeof(ptr);
    hr = IDXGISurface_GetPrivateData(surface, &test_guid, &size, &ptr);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(ptr == (IUnknown *)test_object, "Got unexpected ptr %p, expected %p.\n", ptr, test_object);
    IUnknown_Release(ptr);

    hr = ID3D11Texture2D_QueryInterface(texture, &IID_ID3D10Texture2D, (void **)&d3d10_texture);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Texture should implement ID3D10Texture2D.\n");
    if (SUCCEEDED(hr))
    {
        ptr = NULL;
        size = sizeof(ptr);
        hr = ID3D10Texture2D_GetPrivateData(d3d10_texture, &test_guid, &size, &ptr);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        ok(ptr == (IUnknown *)test_object, "Got unexpected ptr %p, expected %p.\n", ptr, test_object);
        IUnknown_Release(ptr);
        ID3D10Texture2D_Release(d3d10_texture);
    }

    IDXGISurface_Release(surface);
    ID3D11Texture2D_Release(texture);
    refcount = ID3D11Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    refcount = ID3D11Device_Release(test_object);
    ok(!refcount, "Test object has %u references left.\n", refcount);
}

static void test_blend(void)
{
    ID3D11BlendState *src_blend, *dst_blend;
    struct d3d11_test_context test_context;
    ID3D11RenderTargetView *offscreen_rtv;
    D3D11_TEXTURE2D_DESC texture_desc;
    ID3D11InputLayout *input_layout;
    ID3D11DeviceContext *context;
    D3D11_BLEND_DESC blend_desc;
    unsigned int stride, offset;
    ID3D11Texture2D *offscreen;
    ID3D11VertexShader *vs;
    ID3D11PixelShader *ps;
    ID3D11Device *device;
    D3D11_VIEWPORT vp;
    ID3D11Buffer *vb;
    DWORD color;
    HRESULT hr;

    static const DWORD vs_code[] =
    {
#if 0
        struct vs_out
        {
            float4 position : SV_POSITION;
            float4 color : COLOR;
        };

        struct vs_out main(float4 position : POSITION, float4 color : COLOR)
        {
            struct vs_out o;

            o.position = position;
            o.color = color;

            return o;
        }
#endif
        0x43425844, 0x5c73b061, 0x5c71125f, 0x3f8b345f, 0xce04b9ab, 0x00000001, 0x00000140, 0x00000003,
        0x0000002c, 0x0000007c, 0x000000d0, 0x4e475349, 0x00000048, 0x00000002, 0x00000008, 0x00000038,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x00000041, 0x00000000, 0x00000000,
        0x00000003, 0x00000001, 0x00000f0f, 0x49534f50, 0x4e4f4954, 0x4c4f4300, 0xab00524f, 0x4e47534f,
        0x0000004c, 0x00000002, 0x00000008, 0x00000038, 0x00000000, 0x00000001, 0x00000003, 0x00000000,
        0x0000000f, 0x00000044, 0x00000000, 0x00000000, 0x00000003, 0x00000001, 0x0000000f, 0x505f5653,
        0x5449534f, 0x004e4f49, 0x4f4c4f43, 0xabab0052, 0x52444853, 0x00000068, 0x00010040, 0x0000001a,
        0x0300005f, 0x001010f2, 0x00000000, 0x0300005f, 0x001010f2, 0x00000001, 0x04000067, 0x001020f2,
        0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000001, 0x05000036, 0x001020f2, 0x00000000,
        0x00101e46, 0x00000000, 0x05000036, 0x001020f2, 0x00000001, 0x00101e46, 0x00000001, 0x0100003e,
    };
    static const DWORD ps_code[] =
    {
#if 0
        struct vs_out
        {
            float4 position : SV_POSITION;
            float4 color : COLOR;
        };

        float4 main(struct vs_out i) : SV_TARGET
        {
            return i.color;
        }
#endif
        0x43425844, 0xe2087fa6, 0xa35fbd95, 0x8e585b3f, 0x67890f54, 0x00000001, 0x000000f4, 0x00000003,
        0x0000002c, 0x00000080, 0x000000b4, 0x4e475349, 0x0000004c, 0x00000002, 0x00000008, 0x00000038,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000000f, 0x00000044, 0x00000000, 0x00000000,
        0x00000003, 0x00000001, 0x00000f0f, 0x505f5653, 0x5449534f, 0x004e4f49, 0x4f4c4f43, 0xabab0052,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x00000038, 0x00000040,
        0x0000000e, 0x03001062, 0x001010f2, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x05000036,
        0x001020f2, 0x00000000, 0x00101e46, 0x00000001, 0x0100003e,
    };
    static const struct
    {
        struct vec3 position;
        DWORD diffuse;
    }
    quads[] =
    {
        /* quad1 */
        {{-1.0f, -1.0f, 0.1f}, 0x4000ff00},
        {{-1.0f,  0.0f, 0.1f}, 0x4000ff00},
        {{ 1.0f, -1.0f, 0.1f}, 0x4000ff00},
        {{ 1.0f,  0.0f, 0.1f}, 0x4000ff00},
        /* quad2 */
        {{-1.0f,  0.0f, 0.1f}, 0xc0ff0000},
        {{-1.0f,  1.0f, 0.1f}, 0xc0ff0000},
        {{ 1.0f,  0.0f, 0.1f}, 0xc0ff0000},
        {{ 1.0f,  1.0f, 0.1f}, 0xc0ff0000},
    };
    static const D3D11_INPUT_ELEMENT_DESC layout_desc[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    static const float blend_factor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    static const float red[] = {1.0f, 0.0f, 0.0f, 0.5f};

    if (!init_test_context(&test_context, NULL))
        return;

    device = test_context.device;
    context = test_context.immediate_context;

    hr = ID3D11Device_CreateInputLayout(device, layout_desc, sizeof(layout_desc) / sizeof(*layout_desc),
            vs_code, sizeof(vs_code), &input_layout);
    ok(SUCCEEDED(hr), "Failed to create input layout, hr %#x.\n", hr);

    vb = create_buffer(device, D3D11_BIND_VERTEX_BUFFER, sizeof(quads), quads);

    hr = ID3D11Device_CreateVertexShader(device, vs_code, sizeof(vs_code), NULL, &vs);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);
    hr = ID3D11Device_CreatePixelShader(device, ps_code, sizeof(ps_code), NULL, &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    memset(&blend_desc, 0, sizeof(blend_desc));
    blend_desc.RenderTarget[0].BlendEnable = TRUE;
    blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
    blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = ID3D11Device_CreateBlendState(device, &blend_desc, &src_blend);
    ok(SUCCEEDED(hr), "Failed to create blend state, hr %#x.\n", hr);

    blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_ALPHA;
    blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_DEST_ALPHA;
    blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_DEST_ALPHA;
    blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA;

    hr = ID3D11Device_CreateBlendState(device, &blend_desc, &dst_blend);
    ok(SUCCEEDED(hr), "Failed to create blend state, hr %#x.\n", hr);

    ID3D11DeviceContext_IASetInputLayout(context, input_layout);
    ID3D11DeviceContext_IASetPrimitiveTopology(context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    stride = sizeof(*quads);
    offset = 0;
    ID3D11DeviceContext_IASetVertexBuffers(context, 0, 1, &vb, &stride, &offset);
    ID3D11DeviceContext_VSSetShader(context, vs, NULL, 0);
    ID3D11DeviceContext_PSSetShader(context, ps, NULL, 0);

    ID3D11DeviceContext_ClearRenderTargetView(context, test_context.backbuffer_rtv, red);

    ID3D11DeviceContext_OMSetBlendState(context, src_blend, blend_factor, D3D11_DEFAULT_SAMPLE_MASK);
    ID3D11DeviceContext_Draw(context, 4, 0);
    ID3D11DeviceContext_OMSetBlendState(context, dst_blend, blend_factor, D3D11_DEFAULT_SAMPLE_MASK);
    ID3D11DeviceContext_Draw(context, 4, 4);

    color = get_texture_color(test_context.backbuffer, 320, 360);
    ok(compare_color(color, 0x700040bf, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 320, 120);
    ok(compare_color(color, 0xa080007f, 1), "Got unexpected color 0x%08x.\n", color);

    texture_desc.Width = 128;
    texture_desc.Height = 128;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_B8G8R8X8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    /* DXGI_FORMAT_B8G8R8X8_UNORM is not supported on all implementations. */
    if (FAILED(ID3D11Device_CreateTexture2D(device, &texture_desc, NULL, &offscreen)))
    {
        skip("DXGI_FORMAT_B8G8R8X8_UNORM not supported.\n");
        goto done;
    }

    hr = ID3D11Device_CreateRenderTargetView(device, (ID3D11Resource *)offscreen, NULL, &offscreen_rtv);
    ok(SUCCEEDED(hr), "Failed to create rendertarget view, hr %#x.\n", hr);

    ID3D11DeviceContext_OMSetRenderTargets(context, 1, &offscreen_rtv, NULL);

    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width = 128.0f;
    vp.Height = 128.0f;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    ID3D11DeviceContext_RSSetViewports(context, 1, &vp);

    ID3D11DeviceContext_ClearRenderTargetView(context, offscreen_rtv, red);

    ID3D11DeviceContext_OMSetBlendState(context, src_blend, blend_factor, D3D11_DEFAULT_SAMPLE_MASK);
    ID3D11DeviceContext_Draw(context, 4, 0);
    ID3D11DeviceContext_OMSetBlendState(context, dst_blend, blend_factor, D3D11_DEFAULT_SAMPLE_MASK);
    ID3D11DeviceContext_Draw(context, 4, 4);

    color = get_texture_color(offscreen, 64, 96) & 0x00ffffff;
    ok(compare_color(color, 0x00bf4000, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(offscreen, 64, 32) & 0x00ffffff;
    ok(compare_color(color, 0x000000ff, 1), "Got unexpected color 0x%08x.\n", color);

    ID3D11RenderTargetView_Release(offscreen_rtv);
    ID3D11Texture2D_Release(offscreen);
done:
    ID3D11BlendState_Release(dst_blend);
    ID3D11BlendState_Release(src_blend);
    ID3D11PixelShader_Release(ps);
    ID3D11VertexShader_Release(vs);
    ID3D11Buffer_Release(vb);
    ID3D11InputLayout_Release(input_layout);
    release_test_context(&test_context);
}

static void test_texture(void)
{
    struct shader
    {
        const DWORD *code;
        size_t size;
    };
    struct texture
    {
        UINT width;
        UINT height;
        UINT miplevel_count;
        UINT array_size;
        DXGI_FORMAT format;
        D3D11_SUBRESOURCE_DATA data[3];
    };

    struct d3d11_test_context test_context;
    const struct texture *current_texture;
    D3D11_TEXTURE2D_DESC texture_desc;
    D3D11_SAMPLER_DESC sampler_desc;
    const struct shader *current_ps;
    ID3D11ShaderResourceView *srv;
    ID3D11DeviceContext *context;
    ID3D11SamplerState *sampler;
    struct texture_readback rb;
    ID3D11Texture2D *texture;
    struct vec4 ps_constant;
    ID3D11PixelShader *ps;
    ID3D11Device *device;
    unsigned int i, x, y;
    ID3D11Buffer *cb;
    DWORD color;
    HRESULT hr;

    static const DWORD ps_ld_code[] =
    {
#if 0
        Texture2D t;

        float miplevel;

        float4 main(float4 position : SV_POSITION) : SV_TARGET
        {
            float3 p;
            t.GetDimensions(miplevel, p.x, p.y, p.z);
            p.z = miplevel;
            p *= float3(position.x / 640.0f, position.y / 480.0f, 1.0f);
            return t.Load(int3(p));
        }
#endif
        0x43425844, 0xbdda6bdf, 0xc6ffcdf1, 0xa58596b3, 0x822383f0, 0x00000001, 0x000001ac, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x00000110, 0x00000040,
        0x00000044, 0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x04001858, 0x00107000, 0x00000000,
        0x00005555, 0x04002064, 0x00101032, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000,
        0x02000068, 0x00000001, 0x0600001c, 0x00100012, 0x00000000, 0x0020800a, 0x00000000, 0x00000000,
        0x0700003d, 0x001000f2, 0x00000000, 0x0010000a, 0x00000000, 0x00107e46, 0x00000000, 0x07000038,
        0x00100032, 0x00000000, 0x00100046, 0x00000000, 0x00101046, 0x00000000, 0x06000036, 0x001000c2,
        0x00000000, 0x00208006, 0x00000000, 0x00000000, 0x0a000038, 0x001000f2, 0x00000000, 0x00100e46,
        0x00000000, 0x00004002, 0x3acccccd, 0x3b088889, 0x3f800000, 0x3f800000, 0x0500001b, 0x001000f2,
        0x00000000, 0x00100e46, 0x00000000, 0x0700002d, 0x001020f2, 0x00000000, 0x00100e46, 0x00000000,
        0x00107e46, 0x00000000, 0x0100003e,
    };
    static const DWORD ps_ld_sint8_code[] =
    {
#if 0
        Texture2D<int4> t;

        float4 main(float4 position : SV_POSITION) : SV_TARGET
        {
            float3 p, s;
            int4 c;

            p = float3(position.x / 640.0f, position.y / 480.0f, 0.0f);
            t.GetDimensions(0, s.x, s.y, s.z);
            p *= s;

            c = t.Load(int3(p));
            return (max(c / (float4)127, (float4)-1) + (float4)1) / 2.0f;
        }
#endif
        0x43425844, 0xb3d0b0fc, 0x0e486f4a, 0xf67eec12, 0xfb9dd52f, 0x00000001, 0x00000240, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x000001a4, 0x00000040,
        0x00000069, 0x04001858, 0x00107000, 0x00000000, 0x00003333, 0x04002064, 0x00101032, 0x00000000,
        0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000002, 0x0700003d, 0x001000f2,
        0x00000000, 0x00004001, 0x00000000, 0x00107e46, 0x00000000, 0x0a000038, 0x00100032, 0x00000001,
        0x00101046, 0x00000000, 0x00004002, 0x3acccccd, 0x3b088889, 0x00000000, 0x00000000, 0x08000036,
        0x001000c2, 0x00000001, 0x00004002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x07000038,
        0x001000f2, 0x00000000, 0x00100f46, 0x00000000, 0x00100e46, 0x00000001, 0x0500001b, 0x001000f2,
        0x00000000, 0x00100e46, 0x00000000, 0x0700002d, 0x001000f2, 0x00000000, 0x00100e46, 0x00000000,
        0x00107e46, 0x00000000, 0x0500002b, 0x001000f2, 0x00000000, 0x00100e46, 0x00000000, 0x0a000038,
        0x001000f2, 0x00000000, 0x00100e46, 0x00000000, 0x00004002, 0x3c010204, 0x3c010204, 0x3c010204,
        0x3c010204, 0x0a000034, 0x001000f2, 0x00000000, 0x00100e46, 0x00000000, 0x00004002, 0xbf800000,
        0xbf800000, 0xbf800000, 0xbf800000, 0x0a000000, 0x001000f2, 0x00000000, 0x00100e46, 0x00000000,
        0x00004002, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000, 0x0a000038, 0x001020f2, 0x00000000,
        0x00100e46, 0x00000000, 0x00004002, 0x3f000000, 0x3f000000, 0x3f000000, 0x3f000000, 0x0100003e,
    };
    static const DWORD ps_ld_uint8_code[] =
    {
#if 0
        Texture2D<uint4> t;

        float4 main(float4 position : SV_POSITION) : SV_TARGET
        {
            float3 p, s;

            p = float3(position.x / 640.0f, position.y / 480.0f, 0.0f);
            t.GetDimensions(0, s.x, s.y, s.z);
            p *= s;

            return t.Load(int3(p)) / (float4)255;
        }
#endif
        0x43425844, 0xd09917eb, 0x4508a07e, 0xb0b7250a, 0x228c1f0e, 0x00000001, 0x000001c8, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x0000012c, 0x00000040,
        0x0000004b, 0x04001858, 0x00107000, 0x00000000, 0x00004444, 0x04002064, 0x00101032, 0x00000000,
        0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000002, 0x0700003d, 0x001000f2,
        0x00000000, 0x00004001, 0x00000000, 0x00107e46, 0x00000000, 0x0a000038, 0x00100032, 0x00000001,
        0x00101046, 0x00000000, 0x00004002, 0x3acccccd, 0x3b088889, 0x00000000, 0x00000000, 0x08000036,
        0x001000c2, 0x00000001, 0x00004002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x07000038,
        0x001000f2, 0x00000000, 0x00100f46, 0x00000000, 0x00100e46, 0x00000001, 0x0500001b, 0x001000f2,
        0x00000000, 0x00100e46, 0x00000000, 0x0700002d, 0x001000f2, 0x00000000, 0x00100e46, 0x00000000,
        0x00107e46, 0x00000000, 0x05000056, 0x001000f2, 0x00000000, 0x00100e46, 0x00000000, 0x0a000038,
        0x001020f2, 0x00000000, 0x00100e46, 0x00000000, 0x00004002, 0x3b808081, 0x3b808081, 0x3b808081,
        0x3b808081, 0x0100003e,
    };
    static const DWORD ps_sample_code[] =
    {
#if 0
        Texture2D t;
        SamplerState s;

        float4 main(float4 position : SV_POSITION) : SV_Target
        {
            float2 p;

            p.x = position.x / 640.0f;
            p.y = position.y / 480.0f;
            return t.Sample(s, p);
        }
#endif
        0x43425844, 0x1ce9b612, 0xc8176faa, 0xd37844af, 0xdb515605, 0x00000001, 0x00000134, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x00000098, 0x00000040,
        0x00000026, 0x0300005a, 0x00106000, 0x00000000, 0x04001858, 0x00107000, 0x00000000, 0x00005555,
        0x04002064, 0x00101032, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x02000068,
        0x00000001, 0x0a000038, 0x00100032, 0x00000000, 0x00101046, 0x00000000, 0x00004002, 0x3acccccd,
        0x3b088889, 0x00000000, 0x00000000, 0x09000045, 0x001020f2, 0x00000000, 0x00100046, 0x00000000,
        0x00107e46, 0x00000000, 0x00106000, 0x00000000, 0x0100003e,
    };
    static const DWORD ps_sample_b_code[] =
    {
#if 0
        Texture2D t;
        SamplerState s;

        float bias;

        float4 main(float4 position : SV_POSITION) : SV_Target
        {
            float2 p;

            p.x = position.x / 640.0f;
            p.y = position.y / 480.0f;
            return t.SampleBias(s, p, bias);
        }
#endif
        0x43425844, 0xc39b0686, 0x8244a7fc, 0x14c0b97a, 0x2900b3b7, 0x00000001, 0x00000150, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x000000b4, 0x00000040,
        0x0000002d, 0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x0300005a, 0x00106000, 0x00000000,
        0x04001858, 0x00107000, 0x00000000, 0x00005555, 0x04002064, 0x00101032, 0x00000000, 0x00000001,
        0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000001, 0x0a000038, 0x00100032, 0x00000000,
        0x00101046, 0x00000000, 0x00004002, 0x3acccccd, 0x3b088889, 0x00000000, 0x00000000, 0x0c00004a,
        0x001020f2, 0x00000000, 0x00100046, 0x00000000, 0x00107e46, 0x00000000, 0x00106000, 0x00000000,
        0x0020800a, 0x00000000, 0x00000000, 0x0100003e,
    };
    static const DWORD ps_sample_l_code[] =
    {
#if 0
        Texture2D t;
        SamplerState s;

        float level;

        float4 main(float4 position : SV_POSITION) : SV_Target
        {
            float2 p;

            p.x = position.x / 640.0f;
            p.y = position.y / 480.0f;
            return t.SampleLevel(s, p, level);
        }
#endif
        0x43425844, 0x61e05d85, 0x2a7300fb, 0x0a83706b, 0x889d1683, 0x00000001, 0x00000150, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x000000b4, 0x00000040,
        0x0000002d, 0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x0300005a, 0x00106000, 0x00000000,
        0x04001858, 0x00107000, 0x00000000, 0x00005555, 0x04002064, 0x00101032, 0x00000000, 0x00000001,
        0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000001, 0x0a000038, 0x00100032, 0x00000000,
        0x00101046, 0x00000000, 0x00004002, 0x3acccccd, 0x3b088889, 0x00000000, 0x00000000, 0x0c000048,
        0x001020f2, 0x00000000, 0x00100046, 0x00000000, 0x00107e46, 0x00000000, 0x00106000, 0x00000000,
        0x0020800a, 0x00000000, 0x00000000, 0x0100003e,
    };
    static const DWORD ps_sample_2d_array_code[] =
    {
#if 0
        Texture2DArray t;
        SamplerState s;

        float layer;

        float4 main(float4 position : SV_POSITION) : SV_TARGET
        {
            float3 d;
            float3 p = float3(position.x / 640.0f, position.y / 480.0f, 1.0f);
            t.GetDimensions(d.x, d.y, d.z);
            d.z = layer;
            return t.Sample(s, p * d);
        }
#endif
        0x43425844, 0xa9457e44, 0xc0b3ef8e, 0x3d751ae8, 0x23fa4807, 0x00000001, 0x00000194, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x000000f8, 0x00000040,
        0x0000003e, 0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x0300005a, 0x00106000, 0x00000000,
        0x04004058, 0x00107000, 0x00000000, 0x00005555, 0x04002064, 0x00101032, 0x00000000, 0x00000001,
        0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000001, 0x0700003d, 0x001000f2, 0x00000000,
        0x00004001, 0x00000000, 0x00107e46, 0x00000000, 0x0a000038, 0x001000c2, 0x00000000, 0x00101406,
        0x00000000, 0x00004002, 0x00000000, 0x00000000, 0x3acccccd, 0x3b088889, 0x07000038, 0x00100032,
        0x00000000, 0x00100046, 0x00000000, 0x00100ae6, 0x00000000, 0x06000036, 0x00100042, 0x00000000,
        0x0020800a, 0x00000000, 0x00000000, 0x09000045, 0x001020f2, 0x00000000, 0x00100246, 0x00000000,
        0x00107e46, 0x00000000, 0x00106000, 0x00000000, 0x0100003e,
    };
    static const struct shader ps_ld = {ps_ld_code, sizeof(ps_ld_code)};
    static const struct shader ps_ld_sint8 = {ps_ld_sint8_code, sizeof(ps_ld_sint8_code)};
    static const struct shader ps_ld_uint8 = {ps_ld_uint8_code, sizeof(ps_ld_uint8_code)};
    static const struct shader ps_sample = {ps_sample_code, sizeof(ps_sample_code)};
    static const struct shader ps_sample_b = {ps_sample_b_code, sizeof(ps_sample_b_code)};
    static const struct shader ps_sample_l = {ps_sample_l_code, sizeof(ps_sample_l_code)};
    static const struct shader ps_sample_2d_array = {ps_sample_2d_array_code, sizeof(ps_sample_2d_array_code)};
    static const DWORD red_data[] =
    {
        0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 0x00000000, 0x00000000,
        0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 0x00000000, 0x00000000,
        0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 0x00000000, 0x00000000,
        0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 0x00000000, 0x00000000,
    };
    static const DWORD green_data[] =
    {
        0xff00ff00, 0xff00ff00, 0xff00ff00, 0xff00ff00,
        0xff00ff00, 0xff00ff00, 0xff00ff00, 0xff00ff00,
        0xff00ff00, 0xff00ff00, 0xff00ff00, 0xff00ff00,
        0xff00ff00, 0xff00ff00, 0xff00ff00, 0xff00ff00,
    };
    static const DWORD blue_data[] =
    {
        0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0x00000000,
        0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0x00000000,
        0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0x00000000,
        0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0x00000000,
    };
    static const DWORD rgba_level_0[] =
    {
        0xff0000ff, 0xff00ffff, 0xff00ff00, 0xffffff00,
        0xffff0000, 0xffff00ff, 0xff000000, 0xff7f7f7f,
        0xffffffff, 0xffffffff, 0xffffffff, 0xff000000,
        0xffffffff, 0xff000000, 0xff000000, 0xff000000,
    };
    static const DWORD rgba_level_1[] =
    {
        0xffffffff, 0xff0000ff,
        0xff000000, 0xff00ff00,
    };
    static const DWORD rgba_level_2[] =
    {
        0xffff0000,
    };
    static const DWORD srgb_data[] =
    {
        0x00000000, 0xffffffff, 0xff000000, 0x7f7f7f7f,
        0xff010203, 0xff102030, 0xff0a0b0c, 0xff8090a0,
        0xffb1c4de, 0xfff0f1f2, 0xfffafdfe, 0xff5a560f,
        0xffd5ff00, 0xffc8f99f, 0xffaa00aa, 0xffdd55bb,
    };
    static const BYTE a8_data[] =
    {
        0x00, 0x10, 0x20, 0x30,
        0x40, 0x50, 0x60, 0x70,
        0x80, 0x90, 0xa0, 0xb0,
        0xc0, 0xd0, 0xe0, 0xf0,
    };
    static const BYTE bc1_data[] =
    {
        0x00, 0xf8, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x00,
        0xe0, 0x07, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00,
        0x1f, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
    };
    static const BYTE bc2_data[] =
    {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xf8, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x07, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
    };
    static const BYTE bc3_data[] =
    {
        0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x07, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
    };
    static const BYTE bc4_data[] =
    {
        0x10, 0x7f, 0x77, 0x39, 0x05, 0x00, 0x00, 0x00,
        0x10, 0x7f, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24,
        0x10, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x10, 0x7f, 0xb6, 0x6d, 0xdb, 0xb6, 0x6d, 0xdb,
    };
    static const BYTE bc5_data[] =
    {
        0x10, 0x7f, 0x77, 0x39, 0x05, 0x00, 0x00, 0x00, 0x10, 0x7f, 0x77, 0x39, 0x05, 0x00, 0x00, 0x00,
        0x10, 0x7f, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24, 0x10, 0x7f, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24,
        0x10, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x10, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x10, 0x7f, 0xb6, 0x6d, 0xdb, 0xb6, 0x6d, 0xdb, 0x10, 0x7f, 0xb6, 0x6d, 0xdb, 0xb6, 0x6d, 0xdb,
    };
    static const struct texture rgba_texture =
    {
        4, 4, 3, 1, DXGI_FORMAT_R8G8B8A8_UNORM,
        {
            {rgba_level_0, 4 * sizeof(*rgba_level_0), 0},
            {rgba_level_1, 2 * sizeof(*rgba_level_1), 0},
            {rgba_level_2,     sizeof(*rgba_level_2), 0},
        }
    };
    static const struct texture srgb_texture = {4, 4, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            {{srgb_data, 4 * sizeof(*srgb_data)}}};
    static const struct texture a8_texture = {4, 4, 1, 1, DXGI_FORMAT_A8_UNORM,
            {{a8_data, 4 * sizeof(*a8_data)}}};
    static const struct texture bc1_texture = {8, 8, 1, 1, DXGI_FORMAT_BC1_UNORM, {{bc1_data, 2 * 8}}};
    static const struct texture bc2_texture = {8, 8, 1, 1, DXGI_FORMAT_BC2_UNORM, {{bc2_data, 2 * 16}}};
    static const struct texture bc3_texture = {8, 8, 1, 1, DXGI_FORMAT_BC3_UNORM, {{bc3_data, 2 * 16}}};
    static const struct texture bc4_texture = {8, 8, 1, 1, DXGI_FORMAT_BC4_UNORM, {{bc4_data, 2 * 8}}};
    static const struct texture bc5_texture = {8, 8, 1, 1, DXGI_FORMAT_BC5_UNORM, {{bc5_data, 2 * 16}}};
    static const struct texture bc1_texture_srgb = {8, 8, 1, 1, DXGI_FORMAT_BC1_UNORM_SRGB, {{bc1_data, 2 * 8}}};
    static const struct texture bc2_texture_srgb = {8, 8, 1, 1, DXGI_FORMAT_BC2_UNORM_SRGB, {{bc2_data, 2 * 16}}};
    static const struct texture bc3_texture_srgb = {8, 8, 1, 1, DXGI_FORMAT_BC3_UNORM_SRGB, {{bc3_data, 2 * 16}}};
    static const struct texture sint8_texture = {4, 4, 1, 1, DXGI_FORMAT_R8G8B8A8_SINT,
            {{rgba_level_0, 4 * sizeof(*rgba_level_0)}}};
    static const struct texture uint8_texture = {4, 4, 1, 1, DXGI_FORMAT_R8G8B8A8_UINT,
            {{rgba_level_0, 4 * sizeof(*rgba_level_0)}}};
    static const struct texture array_2d_texture =
    {
        4, 4, 1, 3, DXGI_FORMAT_R8G8B8A8_UNORM,
        {
            {red_data,   6 * sizeof(*red_data)},
            {green_data, 4 * sizeof(*green_data)},
            {blue_data,  5 * sizeof(*blue_data)},
        }
    };
    static const DWORD red_colors[] =
    {
        0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff,
        0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff,
        0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff,
        0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff,
    };
    static const DWORD blue_colors[] =
    {
        0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
        0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
        0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
        0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
    };
    static const DWORD level_1_colors[] =
    {
        0xffffffff, 0xffffffff, 0xff0000ff, 0xff0000ff,
        0xffffffff, 0xffffffff, 0xff0000ff, 0xff0000ff,
        0xff000000, 0xff000000, 0xff00ff00, 0xff00ff00,
        0xff000000, 0xff000000, 0xff00ff00, 0xff00ff00,
    };
    static const DWORD lerp_1_2_colors[] =
    {
        0xffff7f7f, 0xffff7f7f, 0xff7f007f, 0xff7f007f,
        0xffff7f7f, 0xffff7f7f, 0xff7f007f, 0xff7f007f,
        0xff7f0000, 0xff7f0000, 0xff7f7f00, 0xff7f7f00,
        0xff7f0000, 0xff7f0000, 0xff7f7f00, 0xff7f7f00,
    };
    static const DWORD level_2_colors[] =
    {
        0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
        0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
        0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
        0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
    };
    static const DWORD srgb_colors[] =
    {
        0x00000001, 0xffffffff, 0xff000000, 0x7f363636,
        0xff000000, 0xff010408, 0xff010101, 0xff37475a,
        0xff708cba, 0xffdee0e2, 0xfff3fbfd, 0xff1a1801,
        0xffa9ff00, 0xff93f159, 0xff670067, 0xffb8177f,
    };
    static const DWORD a8_colors[] =
    {
        0x00000000, 0x10000000, 0x20000000, 0x30000000,
        0x40000000, 0x50000000, 0x60000000, 0x70000000,
        0x80000000, 0x90000000, 0xa0000000, 0xb0000000,
        0xc0000000, 0xd0000000, 0xe0000000, 0xf0000000,
    };
    static const DWORD bc_colors[] =
    {
        0xff0000ff, 0xff0000ff, 0xff00ff00, 0xff00ff00,
        0xff0000ff, 0xff0000ff, 0xff00ff00, 0xff00ff00,
        0xffff0000, 0xffff0000, 0xffffffff, 0xffffffff,
        0xffff0000, 0xffff0000, 0xffffffff, 0xffffffff,
    };
    static const DWORD bc4_colors[] =
    {
        0xff000026, 0xff000010, 0xff00007f, 0xff00007f,
        0xff000010, 0xff000010, 0xff00007f, 0xff00007f,
        0xff0000ff, 0xff0000ff, 0xff000000, 0xff000000,
        0xff0000ff, 0xff0000ff, 0xff000000, 0xff000000,
    };
    static const DWORD bc5_colors[] =
    {
        0xff002626, 0xff001010, 0xff007f7f, 0xff007f7f,
        0xff001010, 0xff001010, 0xff007f7f, 0xff007f7f,
        0xff00ffff, 0xff00ffff, 0xff000000, 0xff000000,
        0xff00ffff, 0xff00ffff, 0xff000000, 0xff000000,
    };
    static const DWORD sint8_colors[] =
    {
        0x7e80807e, 0x7e807e7e, 0x7e807e80, 0x7e7e7e80,
        0x7e7e8080, 0x7e7e7f7f, 0x7e808080, 0x7effffff,
        0x7e7e7e7e, 0x7e7e7e7e, 0x7e7e7e7e, 0x7e808080,
        0x7e7e7e7e, 0x7e7f7f7f, 0x7e7f7f7f, 0x7e7f7f7f,
    };
    static const DWORD zero_colors[4 * 4] = {0};
    static const float red[] = {1.0f, 0.0f, 0.0f, 0.5f};

    static const struct test
    {
        const struct shader *ps;
        const struct texture *texture;
        D3D11_FILTER filter;
        float lod_bias;
        float min_lod;
        float max_lod;
        float ps_constant;
        const DWORD *expected_colors;
    }
    tests[] =
    {
#define POINT        D3D11_FILTER_MIN_MAG_MIP_POINT
#define POINT_LINEAR D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR
#define MIP_MAX      D3D11_FLOAT32_MAX
        {&ps_ld,              &rgba_texture,     POINT,        0.0f, 0.0f,    0.0f,  0.0f, rgba_level_0},
        {&ps_ld,              &rgba_texture,     POINT,        0.0f, 0.0f,    0.0f,  1.0f, level_1_colors},
        {&ps_ld,              &rgba_texture,     POINT,        0.0f, 0.0f,    0.0f,  2.0f, level_2_colors},
        {&ps_ld,              &rgba_texture,     POINT,        0.0f, 0.0f,    0.0f,  3.0f, zero_colors},
        {&ps_ld,              &srgb_texture,     POINT,        0.0f, 0.0f,    0.0f,  0.0f, srgb_colors},
        {&ps_ld,              &bc1_texture,      POINT,        0.0f, 0.0f,    0.0f,  0.0f, bc_colors},
        {&ps_ld,              &bc1_texture,      POINT,        0.0f, 0.0f,    0.0f,  1.0f, zero_colors},
        {&ps_ld,              &bc2_texture,      POINT,        0.0f, 0.0f,    0.0f,  0.0f, bc_colors},
        {&ps_ld,              &bc2_texture,      POINT,        0.0f, 0.0f,    0.0f,  1.0f, zero_colors},
        {&ps_ld,              &bc3_texture,      POINT,        0.0f, 0.0f,    0.0f,  0.0f, bc_colors},
        {&ps_ld,              &bc3_texture,      POINT,        0.0f, 0.0f,    0.0f,  1.0f, zero_colors},
        {&ps_ld,              &bc1_texture_srgb, POINT,        0.0f, 0.0f,    0.0f,  0.0f, bc_colors},
        {&ps_ld,              &bc2_texture_srgb, POINT,        0.0f, 0.0f,    0.0f,  0.0f, bc_colors},
        {&ps_ld,              &bc3_texture_srgb, POINT,        0.0f, 0.0f,    0.0f,  0.0f, bc_colors},
        {&ps_ld,              &bc4_texture,      POINT,        0.0f, 0.0f,    0.0f,  0.0f, bc4_colors},
        {&ps_ld,              &bc5_texture,      POINT,        0.0f, 0.0f,    0.0f,  0.0f, bc5_colors},
        {&ps_ld_sint8,        &sint8_texture,    POINT,        0.0f, 0.0f,    0.0f,  0.0f, sint8_colors},
        {&ps_ld_uint8,        &uint8_texture,    POINT,        0.0f, 0.0f,    0.0f,  0.0f, rgba_level_0},
        {&ps_sample,          &bc1_texture,      POINT,        0.0f, 0.0f,    0.0f,  0.0f, bc_colors},
        {&ps_sample,          &bc2_texture,      POINT,        0.0f, 0.0f,    0.0f,  0.0f, bc_colors},
        {&ps_sample,          &bc3_texture,      POINT,        0.0f, 0.0f,    0.0f,  0.0f, bc_colors},
        {&ps_sample,          &bc4_texture,      POINT,        0.0f, 0.0f,    0.0f,  0.0f, bc4_colors},
        {&ps_sample,          &bc5_texture,      POINT,        0.0f, 0.0f,    0.0f,  0.0f, bc5_colors},
        {&ps_sample,          &rgba_texture,     POINT,        0.0f, 0.0f,    0.0f,  0.0f, rgba_level_0},
        {&ps_sample,          &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  0.0f, rgba_level_0},
        {&ps_sample,          &rgba_texture,     POINT,        2.0f, 0.0f, MIP_MAX,  0.0f, rgba_level_0},
        {&ps_sample,          &rgba_texture,     POINT,        8.0f, 0.0f, MIP_MAX,  0.0f, level_1_colors},
        {&ps_sample,          &srgb_texture,     POINT,        0.0f, 0.0f,    0.0f,  0.0f, srgb_colors},
        {&ps_sample,          &a8_texture,       POINT,        0.0f, 0.0f,    0.0f,  0.0f, a8_colors},
        {&ps_sample_b,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  0.0f, rgba_level_0},
        {&ps_sample_b,        &rgba_texture,     POINT,        8.0f, 0.0f, MIP_MAX,  0.0f, level_1_colors},
        {&ps_sample_b,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  8.0f, level_1_colors},
        {&ps_sample_b,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  8.4f, level_1_colors},
        {&ps_sample_b,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  8.5f, level_2_colors},
        {&ps_sample_b,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  9.0f, level_2_colors},
        {&ps_sample_b,        &rgba_texture,     POINT,        0.0f, 0.0f,    2.0f,  1.0f, rgba_level_0},
        {&ps_sample_b,        &rgba_texture,     POINT,        0.0f, 0.0f,    2.0f,  9.0f, level_2_colors},
        {&ps_sample_b,        &rgba_texture,     POINT,        0.0f, 0.0f,    1.0f,  9.0f, level_1_colors},
        {&ps_sample_b,        &rgba_texture,     POINT,        0.0f, 0.0f,    0.0f,  9.0f, rgba_level_0},
        {&ps_sample_l,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX, -1.0f, rgba_level_0},
        {&ps_sample_l,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  0.0f, rgba_level_0},
        {&ps_sample_l,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  0.4f, rgba_level_0},
        {&ps_sample_l,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  0.5f, level_1_colors},
        {&ps_sample_l,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  1.0f, level_1_colors},
        {&ps_sample_l,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  1.4f, level_1_colors},
        {&ps_sample_l,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  1.5f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  2.0f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  3.0f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  4.0f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT_LINEAR, 0.0f, 0.0f, MIP_MAX,  1.5f, lerp_1_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT_LINEAR, 2.0f, 0.0f, MIP_MAX, -2.0f, rgba_level_0},
        {&ps_sample_l,        &rgba_texture,     POINT_LINEAR, 2.0f, 0.0f, MIP_MAX, -1.0f, level_1_colors},
        {&ps_sample_l,        &rgba_texture,     POINT_LINEAR, 2.0f, 0.0f, MIP_MAX,  0.0f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT_LINEAR, 2.0f, 0.0f, MIP_MAX,  1.0f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT_LINEAR, 2.0f, 0.0f, MIP_MAX,  1.5f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT_LINEAR, 2.0f, 2.0f,    2.0f, -9.0f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT_LINEAR, 2.0f, 2.0f,    2.0f, -1.0f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT_LINEAR, 2.0f, 2.0f,    2.0f,  0.0f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT_LINEAR, 2.0f, 2.0f,    2.0f,  1.0f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT_LINEAR, 2.0f, 2.0f,    2.0f,  9.0f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT,        2.0f, 2.0f,    2.0f, -9.0f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT,        2.0f, 2.0f,    2.0f, -1.0f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT,        2.0f, 2.0f,    2.0f,  0.0f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT,        2.0f, 2.0f,    2.0f,  1.0f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT,        2.0f, 2.0f,    2.0f,  9.0f, level_2_colors},
        {&ps_sample_2d_array, &array_2d_texture, POINT,        0.0f, 0.0f, MIP_MAX, -9.0f, red_colors},
        {&ps_sample_2d_array, &array_2d_texture, POINT,        0.0f, 0.0f, MIP_MAX, -1.0f, red_colors},
        {&ps_sample_2d_array, &array_2d_texture, POINT,        0.0f, 0.0f, MIP_MAX,  0.0f, red_colors},
        {&ps_sample_2d_array, &array_2d_texture, POINT,        0.0f, 0.0f, MIP_MAX,  0.4f, red_colors},
        {&ps_sample_2d_array, &array_2d_texture, POINT,        0.0f, 0.0f, MIP_MAX,  0.5f, red_colors},
        {&ps_sample_2d_array, &array_2d_texture, POINT,        0.0f, 0.0f, MIP_MAX,  1.0f, green_data},
        {&ps_sample_2d_array, &array_2d_texture, POINT,        0.0f, 0.0f, MIP_MAX,  1.4f, green_data},
        {&ps_sample_2d_array, &array_2d_texture, POINT,        0.0f, 0.0f, MIP_MAX,  2.0f, blue_colors},
        {&ps_sample_2d_array, &array_2d_texture, POINT,        0.0f, 0.0f, MIP_MAX,  2.1f, blue_colors},
        {&ps_sample_2d_array, &array_2d_texture, POINT,        0.0f, 0.0f, MIP_MAX,  3.0f, blue_colors},
        {&ps_sample_2d_array, &array_2d_texture, POINT,        0.0f, 0.0f, MIP_MAX,  3.1f, blue_colors},
        {&ps_sample_2d_array, &array_2d_texture, POINT,        0.0f, 0.0f, MIP_MAX,  9.0f, blue_colors},
        {&ps_sample_2d_array, NULL,              POINT,        0.0f, 0.0f,    0.0f,  0.0f, zero_colors},
        {&ps_sample_2d_array, NULL,              POINT,        0.0f, 0.0f, MIP_MAX,  0.0f, zero_colors},
#undef POINT
#undef POINT_LINEAR
#undef MIP_MAX
    };

    if (!init_test_context(&test_context, NULL))
        return;

    device = test_context.device;
    context = test_context.immediate_context;

    cb = create_buffer(device, D3D11_BIND_CONSTANT_BUFFER, sizeof(ps_constant), NULL);

    ID3D11DeviceContext_PSSetConstantBuffers(context, 0, 1, &cb);

    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.MipLODBias = 0.0f;
    sampler_desc.MaxAnisotropy = 0;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampler_desc.BorderColor[0] = 0.0f;
    sampler_desc.BorderColor[1] = 0.0f;
    sampler_desc.BorderColor[2] = 0.0f;
    sampler_desc.BorderColor[3] = 0.0f;
    sampler_desc.MinLOD = 0.0f;
    sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

    ps = NULL;
    srv = NULL;
    sampler = NULL;
    texture = NULL;
    current_ps = NULL;
    current_texture = NULL;
    for (i = 0; i < sizeof(tests) / sizeof(*tests); ++i)
    {
        const struct test *test = &tests[i];

        if (current_ps != test->ps)
        {
            if (ps)
                ID3D11PixelShader_Release(ps);

            current_ps = test->ps;

            hr = ID3D11Device_CreatePixelShader(device, current_ps->code, current_ps->size, NULL, &ps);
            ok(SUCCEEDED(hr), "Test %u: Failed to create pixel shader, hr %#x.\n", i, hr);

            ID3D11DeviceContext_PSSetShader(context, ps, NULL, 0);
        }

        if (current_texture != test->texture)
        {
            if (texture)
                ID3D11Texture2D_Release(texture);
            if (srv)
                ID3D11ShaderResourceView_Release(srv);

            current_texture = test->texture;

            if (current_texture)
            {
                texture_desc.Width = current_texture->width;
                texture_desc.Height = current_texture->height;
                texture_desc.MipLevels = current_texture->miplevel_count;
                texture_desc.ArraySize = current_texture->array_size;
                texture_desc.Format = current_texture->format;

                hr = ID3D11Device_CreateTexture2D(device, &texture_desc, current_texture->data, &texture);
                ok(SUCCEEDED(hr), "Test %u: Failed to create 2d texture, hr %#x.\n", i, hr);

                hr = ID3D11Device_CreateShaderResourceView(device, (ID3D11Resource *)texture, NULL, &srv);
                ok(SUCCEEDED(hr), "Test %u: Failed to create shader resource view, hr %#x.\n", i, hr);
            }
            else
            {
                texture = NULL;
                srv = NULL;
            }

            ID3D11DeviceContext_PSSetShaderResources(context, 0, 1, &srv);
        }

        if (!sampler || (sampler_desc.Filter != test->filter
                || sampler_desc.MipLODBias != test->lod_bias
                || sampler_desc.MinLOD != test->min_lod
                || sampler_desc.MaxLOD != test->max_lod))
        {
            if (sampler)
                ID3D11SamplerState_Release(sampler);

            sampler_desc.Filter = test->filter;
            sampler_desc.MipLODBias = test->lod_bias;
            sampler_desc.MinLOD = test->min_lod;
            sampler_desc.MaxLOD = test->max_lod;

            hr = ID3D11Device_CreateSamplerState(device, &sampler_desc, &sampler);
            ok(SUCCEEDED(hr), "Test %u: Failed to create sampler state, hr %#x.\n", i, hr);

            ID3D11DeviceContext_PSSetSamplers(context, 0, 1, &sampler);
        }

        ps_constant.x = test->ps_constant;
        ID3D11DeviceContext_UpdateSubresource(context, (ID3D11Resource *)cb, 0, NULL, &ps_constant, 0, 0);

        ID3D11DeviceContext_ClearRenderTargetView(context, test_context.backbuffer_rtv, red);

        draw_quad(&test_context);

        get_texture_readback(test_context.backbuffer, 0, &rb);
        for (y = 0; y < 4; ++y)
        {
            for (x = 0; x < 4; ++x)
            {
                color = get_readback_color(&rb, 80 + x * 160, 60 + y * 120);
                ok(compare_color(color, test->expected_colors[y * 4 + x], 1),
                        "Test %u: Got unexpected color 0x%08x at (%u, %u).\n", i, color, x, y);
            }
        }
        release_texture_readback(&rb);
    }
    if (srv)
        ID3D11ShaderResourceView_Release(srv);
    ID3D11SamplerState_Release(sampler);
    if (texture)
        ID3D11Texture2D_Release(texture);
    ID3D11PixelShader_Release(ps);

    ID3D11Buffer_Release(cb);
    release_test_context(&test_context);
}

static void test_multiple_render_targets(void)
{
    D3D11_TEXTURE2D_DESC texture_desc;
    ID3D11InputLayout *input_layout;
    unsigned int stride, offset, i;
    ID3D11RenderTargetView *rtv[4];
    ID3D11DeviceContext *context;
    ID3D11Texture2D *rt[4];
    ID3D11VertexShader *vs;
    ID3D11PixelShader *ps;
    ID3D11Device *device;
    D3D11_VIEWPORT vp;
    ID3D11Buffer *vb;
    ULONG refcount;
    HRESULT hr;

    static const D3D11_INPUT_ELEMENT_DESC layout_desc[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    static const DWORD vs_code[] =
    {
#if 0
        float4 main(float4 position : POSITION) : SV_POSITION
        {
            return position;
        }
#endif
        0x43425844, 0xa7a2f22d, 0x83ff2560, 0xe61638bd, 0x87e3ce90, 0x00000001, 0x000000d8, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x49534f50, 0x4e4f4954, 0xababab00,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000001, 0x00000003,
        0x00000000, 0x0000000f, 0x505f5653, 0x5449534f, 0x004e4f49, 0x52444853, 0x0000003c, 0x00010040,
        0x0000000f, 0x0300005f, 0x001010f2, 0x00000000, 0x04000067, 0x001020f2, 0x00000000, 0x00000001,
        0x05000036, 0x001020f2, 0x00000000, 0x00101e46, 0x00000000, 0x0100003e,
    };
    static const DWORD ps_code[] =
    {
#if 0
        struct output
        {
            float4 t1 : SV_TARGET0;
            float4 t2 : SV_Target1;
            float4 t3 : SV_TARGET2;
            float4 t4 : SV_Target3;
        };

        output main(float4 position : SV_POSITION)
        {
            struct output o;
            o.t1 = (float4)1.0f;
            o.t2 = (float4)0.5f;
            o.t3 = (float4)0.2f;
            o.t4 = float4(0.0f, 0.2f, 0.5f, 1.0f);
            return o;
        }
#endif
        0x43425844, 0x8701ad18, 0xe3d5291d, 0x7b4288a6, 0x01917515, 0x00000001, 0x000001a8, 0x00000003,
        0x0000002c, 0x00000060, 0x000000e4, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000000f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000007c, 0x00000004, 0x00000008, 0x00000068, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x00000072, 0x00000001, 0x00000000, 0x00000003, 0x00000001, 0x0000000f,
        0x00000068, 0x00000002, 0x00000000, 0x00000003, 0x00000002, 0x0000000f, 0x00000072, 0x00000003,
        0x00000000, 0x00000003, 0x00000003, 0x0000000f, 0x545f5653, 0x45475241, 0x56530054, 0x7261545f,
        0x00746567, 0x52444853, 0x000000bc, 0x00000040, 0x0000002f, 0x03000065, 0x001020f2, 0x00000000,
        0x03000065, 0x001020f2, 0x00000001, 0x03000065, 0x001020f2, 0x00000002, 0x03000065, 0x001020f2,
        0x00000003, 0x08000036, 0x001020f2, 0x00000000, 0x00004002, 0x3f800000, 0x3f800000, 0x3f800000,
        0x3f800000, 0x08000036, 0x001020f2, 0x00000001, 0x00004002, 0x3f000000, 0x3f000000, 0x3f000000,
        0x3f000000, 0x08000036, 0x001020f2, 0x00000002, 0x00004002, 0x3e4ccccd, 0x3e4ccccd, 0x3e4ccccd,
        0x3e4ccccd, 0x08000036, 0x001020f2, 0x00000003, 0x00004002, 0x00000000, 0x3e4ccccd, 0x3f000000,
        0x3f800000, 0x0100003e,
    };
    static const struct
    {
        struct vec2 position;
    }
    quad[] =
    {
        {{-1.0f, -1.0f}},
        {{-1.0f,  1.0f}},
        {{ 1.0f, -1.0f}},
        {{ 1.0f,  1.0f}},
    };
    static const float red[] = {1.0f, 0.0f, 0.0f, 1.0f};

    if (!(device = create_device(NULL)))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = ID3D11Device_CreateInputLayout(device, layout_desc, sizeof(layout_desc) / sizeof(*layout_desc),
            vs_code, sizeof(vs_code), &input_layout);
    ok(SUCCEEDED(hr), "Failed to create input layout, hr %#x.\n", hr);

    vb = create_buffer(device, D3D11_BIND_VERTEX_BUFFER, sizeof(quad), quad);

    texture_desc.Width = 640;
    texture_desc.Height = 480;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    for (i = 0; i < sizeof(rt) / sizeof(*rt); ++i)
    {
        hr = ID3D11Device_CreateTexture2D(device, &texture_desc, NULL, &rt[i]);
        ok(SUCCEEDED(hr), "Failed to create texture %u, hr %#x.\n", i, hr);

        hr = ID3D11Device_CreateRenderTargetView(device, (ID3D11Resource *)rt[i], NULL, &rtv[i]);
        ok(SUCCEEDED(hr), "Failed to create rendertarget view %u, hr %#x.\n", i, hr);
    }

    hr = ID3D11Device_CreateVertexShader(device, vs_code, sizeof(vs_code), NULL, &vs);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);
    hr = ID3D11Device_CreatePixelShader(device, ps_code, sizeof(ps_code), NULL, &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    ID3D11Device_GetImmediateContext(device, &context);

    ID3D11DeviceContext_OMSetRenderTargets(context, 4, rtv, NULL);
    ID3D11DeviceContext_IASetInputLayout(context, input_layout);
    ID3D11DeviceContext_IASetPrimitiveTopology(context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    stride = sizeof(*quad);
    offset = 0;
    ID3D11DeviceContext_IASetVertexBuffers(context, 0, 1, &vb, &stride, &offset);
    ID3D11DeviceContext_VSSetShader(context, vs, NULL, 0);
    ID3D11DeviceContext_PSSetShader(context, ps, NULL, 0);

    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width = 640.0f;
    vp.Height = 480.0f;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    ID3D11DeviceContext_RSSetViewports(context, 1, &vp);

    for (i = 0; i < sizeof(rtv) / sizeof(*rtv); ++i)
        ID3D11DeviceContext_ClearRenderTargetView(context, rtv[i], red);

    ID3D11DeviceContext_Draw(context, 4, 0);

    check_texture_color(rt[0], 0xffffffff, 2);
    check_texture_color(rt[1], 0x7f7f7f7f, 2);
    check_texture_color(rt[2], 0x33333333, 2);
    check_texture_color(rt[3], 0xff7f3300, 2);

    ID3D11Buffer_Release(vb);
    ID3D11PixelShader_Release(ps);
    ID3D11VertexShader_Release(vs);
    ID3D11InputLayout_Release(input_layout);
    for (i = 0; i < sizeof(rtv) / sizeof(*rtv); ++i)
        ID3D11RenderTargetView_Release(rtv[i]);
    for (i = 0; i < sizeof(rt) / sizeof(*rt); ++i)
        ID3D11Texture2D_Release(rt[i]);
    ID3D11DeviceContext_Release(context);
    refcount = ID3D11Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_render_target_views(void)
{
    struct texture
    {
        UINT miplevel_count;
        UINT array_size;
    };
    struct rtv
    {
        DXGI_FORMAT format;
        D3D11_RTV_DIMENSION dimension;
        unsigned int miplevel_idx;
        unsigned int layer_idx;
        unsigned int layer_count;
    };

    static const struct vec4 red = {1.0f, 0.0f, 0.0f, 1.0f};
    static struct test
    {
        struct texture texture;
        struct rtv rtv;
        DWORD expected_colors[4];
    }
    tests[] =
    {
        {{2, 1}, {DXGI_FORMAT_UNKNOWN, D3D11_RTV_DIMENSION_TEXTURE2D, 0},
                {0xff0000ff, 0x00000000}},
        {{2, 1}, {DXGI_FORMAT_UNKNOWN, D3D11_RTV_DIMENSION_TEXTURE2D, 1},
                {0x00000000, 0xff0000ff}},
        {{2, 1}, {DXGI_FORMAT_UNKNOWN, D3D11_RTV_DIMENSION_TEXTURE2DARRAY, 0, 0, 1},
                {0xff0000ff, 0x00000000}},
        {{2, 1}, {DXGI_FORMAT_UNKNOWN, D3D11_RTV_DIMENSION_TEXTURE2DARRAY, 1, 0, 1},
                {0x00000000, 0xff0000ff}},
        {{1, 4}, {DXGI_FORMAT_UNKNOWN, D3D11_RTV_DIMENSION_TEXTURE2D, 0},
                {0xff0000ff, 0x00000000, 0x00000000, 0x00000000}},
        {{1, 4}, {DXGI_FORMAT_UNKNOWN, D3D11_RTV_DIMENSION_TEXTURE2DARRAY, 0, 0, 1},
                {0xff0000ff, 0x00000000, 0x00000000, 0x00000000}},
        {{1, 4}, {DXGI_FORMAT_UNKNOWN, D3D11_RTV_DIMENSION_TEXTURE2DARRAY, 0, 1, 1},
                {0x00000000, 0xff0000ff, 0x00000000, 0x00000000}},
        {{1, 4}, {DXGI_FORMAT_UNKNOWN, D3D11_RTV_DIMENSION_TEXTURE2DARRAY, 0, 2, 1},
                {0x00000000, 0x00000000, 0xff0000ff, 0x00000000}},
        {{1, 4}, {DXGI_FORMAT_UNKNOWN, D3D11_RTV_DIMENSION_TEXTURE2DARRAY, 0, 3, 1},
                {0x00000000, 0x00000000, 0x00000000, 0xff0000ff}},
        {{1, 4}, {DXGI_FORMAT_UNKNOWN, D3D11_RTV_DIMENSION_TEXTURE2DARRAY, 0, 0, 4},
                {0xff0000ff, 0x00000000, 0x00000000, 0x00000000}},
        {{2, 2}, {DXGI_FORMAT_UNKNOWN, D3D11_RTV_DIMENSION_TEXTURE2D, 0},
                {0xff0000ff, 0x00000000, 0x00000000, 0x00000000}},
        {{2, 2}, {DXGI_FORMAT_UNKNOWN, D3D11_RTV_DIMENSION_TEXTURE2DARRAY, 0, 0, 1},
                {0xff0000ff, 0x00000000, 0x00000000, 0x00000000}},
        {{2, 2}, {DXGI_FORMAT_UNKNOWN, D3D11_RTV_DIMENSION_TEXTURE2DARRAY, 0, 1, 1},
                {0x00000000, 0x00000000, 0xff0000ff, 0x00000000}},
        {{2, 2}, {DXGI_FORMAT_UNKNOWN, D3D11_RTV_DIMENSION_TEXTURE2DARRAY, 1, 0, 1},
                {0x00000000, 0xff0000ff, 0x00000000, 0x00000000}},
        {{2, 2}, {DXGI_FORMAT_UNKNOWN, D3D11_RTV_DIMENSION_TEXTURE2DARRAY, 1, 1, 1},
                {0x00000000, 0x00000000, 0x00000000, 0xff0000ff}},
    };

    struct d3d11_test_context test_context;
    D3D11_RENDER_TARGET_VIEW_DESC rtv_desc;
    D3D11_TEXTURE2D_DESC texture_desc;
    ID3D11DeviceContext *context;
    ID3D11RenderTargetView *rtv;
    ID3D11Texture2D *texture;
    ID3D11Device *device;
    unsigned int i, j;
    HRESULT hr;

    if (!init_test_context(&test_context, NULL))
        return;

    device = test_context.device;
    context = test_context.immediate_context;

    texture_desc.Width = 32;
    texture_desc.Height = 32;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    for (i = 0; i < sizeof(tests) / sizeof(*tests); ++i)
    {
        const struct test *test = &tests[i];
        unsigned int sub_resource_count;

        texture_desc.MipLevels = test->texture.miplevel_count;
        texture_desc.ArraySize = test->texture.array_size;

        hr = ID3D11Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
        ok(SUCCEEDED(hr), "Test %u: Failed to create texture, hr %#x.\n", i, hr);

        rtv_desc.Format = texture_desc.Format;
        rtv_desc.ViewDimension = test->rtv.dimension;
        if (test->rtv.dimension == D3D11_RTV_DIMENSION_TEXTURE2D)
        {
            U(rtv_desc).Texture2D.MipSlice = test->rtv.miplevel_idx;
        }
        else if (test->rtv.dimension == D3D11_RTV_DIMENSION_TEXTURE2DARRAY)
        {
            U(rtv_desc).Texture2DArray.MipSlice = test->rtv.miplevel_idx;
            U(rtv_desc).Texture2DArray.FirstArraySlice = test->rtv.layer_idx;
            U(rtv_desc).Texture2DArray.ArraySize = test->rtv.layer_count;
        }
        else
        {
            trace("Test %u: Unhandled view dimension %#x.\n", i, test->rtv.dimension);
        }

        hr = ID3D11Device_CreateRenderTargetView(device, (ID3D11Resource *)texture, &rtv_desc, &rtv);
        ok(SUCCEEDED(hr), "Test %u: Failed to create render target view, hr %#x.\n", i, hr);

        ID3D11DeviceContext_OMSetRenderTargets(context, 1, &rtv, NULL);
        draw_color_quad(&test_context, &red);

        sub_resource_count = texture_desc.MipLevels * texture_desc.ArraySize;
        assert(sub_resource_count <= sizeof(test->expected_colors) / sizeof(*test->expected_colors));
        for (j = 0; j < sub_resource_count; ++j)
            check_texture_sub_resource_color(texture, j, test->expected_colors[j], 1);

        ID3D11RenderTargetView_Release(rtv);
        ID3D11Texture2D_Release(texture);
    }

    release_test_context(&test_context);
}

static void test_scissor(void)
{
    struct d3d11_test_context test_context;
    ID3D11DeviceContext *immediate_context;
    D3D11_RASTERIZER_DESC rs_desc;
    ID3D11RasterizerState *rs;
    D3D11_RECT scissor_rect;
    ID3D11PixelShader *ps;
    ID3D11Device *device;
    DWORD color;
    HRESULT hr;

    static const float red[] = {1.0f, 0.0f, 0.0f, 1.0f};
    static const DWORD ps_code[] =
    {
#if 0
        float4 main(float4 position : SV_POSITION) : SV_Target
        {
            return float4(0.0, 1.0, 0.0, 1.0);
        }
#endif
        0x43425844, 0x30240e72, 0x012f250c, 0x8673c6ea, 0x392e4cec, 0x00000001, 0x000000d4, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000000f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x00000038, 0x00000040,
        0x0000000e, 0x03000065, 0x001020f2, 0x00000000, 0x08000036, 0x001020f2, 0x00000000, 0x00004002,
        0x00000000, 0x3f800000, 0x00000000, 0x3f800000, 0x0100003e,
    };

    if (!init_test_context(&test_context, NULL))
        return;

    device = test_context.device;
    immediate_context = test_context.immediate_context;

    hr = ID3D11Device_CreatePixelShader(device, ps_code, sizeof(ps_code), NULL, &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    rs_desc.FillMode = D3D11_FILL_SOLID;
    rs_desc.CullMode = D3D11_CULL_BACK;
    rs_desc.FrontCounterClockwise = FALSE;
    rs_desc.DepthBias = 0;
    rs_desc.DepthBiasClamp = 0.0f;
    rs_desc.SlopeScaledDepthBias = 0.0f;
    rs_desc.DepthClipEnable = TRUE;
    rs_desc.ScissorEnable = TRUE;
    rs_desc.MultisampleEnable = FALSE;
    rs_desc.AntialiasedLineEnable = FALSE;
    hr = ID3D11Device_CreateRasterizerState(device, &rs_desc, &rs);
    ok(SUCCEEDED(hr), "Failed to create rasterizer state, hr %#x.\n", hr);

    ID3D11DeviceContext_PSSetShader(immediate_context, ps, NULL, 0);

    scissor_rect.left = 160;
    scissor_rect.top = 120;
    scissor_rect.right = 480;
    scissor_rect.bottom = 360;
    ID3D11DeviceContext_RSSetScissorRects(immediate_context, 1, &scissor_rect);

    ID3D11DeviceContext_ClearRenderTargetView(immediate_context, test_context.backbuffer_rtv, red);
    check_texture_color(test_context.backbuffer, 0xff0000ff, 1);

    draw_quad(&test_context);
    color = get_texture_color(test_context.backbuffer, 320, 60);
    ok(compare_color(color, 0xff00ff00, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 80, 240);
    ok(compare_color(color, 0xff00ff00, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 320, 240);
    ok(compare_color(color, 0xff00ff00, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 560, 240);
    ok(compare_color(color, 0xff00ff00, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 320, 420);
    ok(compare_color(color, 0xff00ff00, 1), "Got unexpected color 0x%08x.\n", color);

    ID3D11DeviceContext_ClearRenderTargetView(immediate_context, test_context.backbuffer_rtv, red);
    ID3D11DeviceContext_RSSetState(immediate_context, rs);
    draw_quad(&test_context);
    color = get_texture_color(test_context.backbuffer, 320, 60);
    ok(compare_color(color, 0xff0000ff, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 80, 240);
    ok(compare_color(color, 0xff0000ff, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 320, 240);
    ok(compare_color(color, 0xff00ff00, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 560, 240);
    ok(compare_color(color, 0xff0000ff, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 320, 420);
    ok(compare_color(color, 0xff0000ff, 1), "Got unexpected color 0x%08x.\n", color);

    ID3D11RasterizerState_Release(rs);
    ID3D11PixelShader_Release(ps);
    release_test_context(&test_context);
}

static void test_il_append_aligned(void)
{
    struct d3d11_test_context test_context;
    ID3D11InputLayout *input_layout;
    ID3D11DeviceContext *context;
    unsigned int stride, offset;
    ID3D11VertexShader *vs;
    ID3D11PixelShader *ps;
    ID3D11Device *device;
    ID3D11Buffer *vb[3];
    DWORD color;
    HRESULT hr;

    static const D3D11_INPUT_ELEMENT_DESC layout_desc[] =
    {
        {"COLOR",    2, DXGI_FORMAT_R32G32_FLOAT,       1, D3D11_APPEND_ALIGNED_ELEMENT,
                D3D11_INPUT_PER_INSTANCE_DATA, 2},
        {"COLOR",    3, DXGI_FORMAT_R32G32_FLOAT,       2, D3D11_APPEND_ALIGNED_ELEMENT,
                D3D11_INPUT_PER_INSTANCE_DATA, 1},
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,
                D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R32G32_FLOAT,       2, D3D11_APPEND_ALIGNED_ELEMENT,
                D3D11_INPUT_PER_INSTANCE_DATA, 1},
        {"COLOR",    1, DXGI_FORMAT_R32G32_FLOAT,       1, D3D11_APPEND_ALIGNED_ELEMENT,
                D3D11_INPUT_PER_INSTANCE_DATA, 2},
    };
    static const DWORD vs_code[] =
    {
#if 0
        struct vs_in
        {
            float4 position : POSITION;
            float2 color_xy : COLOR0;
            float2 color_zw : COLOR1;
            unsigned int instance_id : SV_INSTANCEID;
        };

        struct vs_out
        {
            float4 position : SV_POSITION;
            float2 color_xy : COLOR0;
            float2 color_zw : COLOR1;
        };

        struct vs_out main(struct vs_in i)
        {
            struct vs_out o;

            o.position = i.position;
            o.position.x += i.instance_id * 0.5;
            o.color_xy = i.color_xy;
            o.color_zw = i.color_zw;

            return o;
        }
#endif
        0x43425844, 0x52e3bf46, 0x6300403d, 0x624cffe4, 0xa4fc0013, 0x00000001, 0x00000214, 0x00000003,
        0x0000002c, 0x000000bc, 0x00000128, 0x4e475349, 0x00000088, 0x00000004, 0x00000008, 0x00000068,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x00000071, 0x00000000, 0x00000000,
        0x00000003, 0x00000001, 0x00000303, 0x00000071, 0x00000001, 0x00000000, 0x00000003, 0x00000002,
        0x00000303, 0x00000077, 0x00000000, 0x00000008, 0x00000001, 0x00000003, 0x00000101, 0x49534f50,
        0x4e4f4954, 0x4c4f4300, 0x5300524f, 0x4e495f56, 0x4e415453, 0x44494543, 0xababab00, 0x4e47534f,
        0x00000064, 0x00000003, 0x00000008, 0x00000050, 0x00000000, 0x00000001, 0x00000003, 0x00000000,
        0x0000000f, 0x0000005c, 0x00000000, 0x00000000, 0x00000003, 0x00000001, 0x00000c03, 0x0000005c,
        0x00000001, 0x00000000, 0x00000003, 0x00000001, 0x0000030c, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4f4c4f43, 0xabab0052, 0x52444853, 0x000000e4, 0x00010040, 0x00000039, 0x0300005f, 0x001010f2,
        0x00000000, 0x0300005f, 0x00101032, 0x00000001, 0x0300005f, 0x00101032, 0x00000002, 0x04000060,
        0x00101012, 0x00000003, 0x00000008, 0x04000067, 0x001020f2, 0x00000000, 0x00000001, 0x03000065,
        0x00102032, 0x00000001, 0x03000065, 0x001020c2, 0x00000001, 0x02000068, 0x00000001, 0x05000056,
        0x00100012, 0x00000000, 0x0010100a, 0x00000003, 0x09000032, 0x00102012, 0x00000000, 0x0010000a,
        0x00000000, 0x00004001, 0x3f000000, 0x0010100a, 0x00000000, 0x05000036, 0x001020e2, 0x00000000,
        0x00101e56, 0x00000000, 0x05000036, 0x00102032, 0x00000001, 0x00101046, 0x00000001, 0x05000036,
        0x001020c2, 0x00000001, 0x00101406, 0x00000002, 0x0100003e,
    };
    static const DWORD ps_code[] =
    {
#if 0
        struct vs_out
        {
            float4 position : SV_POSITION;
            float2 color_xy : COLOR0;
            float2 color_zw : COLOR1;
        };

        float4 main(struct vs_out i) : SV_TARGET
        {
            return float4(i.color_xy.xy, i.color_zw.xy);
        }
#endif
        0x43425844, 0x64e48a09, 0xaa484d46, 0xe40a6e78, 0x9885edf3, 0x00000001, 0x00000118, 0x00000003,
        0x0000002c, 0x00000098, 0x000000cc, 0x4e475349, 0x00000064, 0x00000003, 0x00000008, 0x00000050,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000000f, 0x0000005c, 0x00000000, 0x00000000,
        0x00000003, 0x00000001, 0x00000303, 0x0000005c, 0x00000001, 0x00000000, 0x00000003, 0x00000001,
        0x00000c0c, 0x505f5653, 0x5449534f, 0x004e4f49, 0x4f4c4f43, 0xabab0052, 0x4e47534f, 0x0000002c,
        0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x0000000f,
        0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x00000044, 0x00000040, 0x00000011, 0x03001062,
        0x00101032, 0x00000001, 0x03001062, 0x001010c2, 0x00000001, 0x03000065, 0x001020f2, 0x00000000,
        0x05000036, 0x001020f2, 0x00000000, 0x00101e46, 0x00000001, 0x0100003e,
    };
    static const struct
    {
        struct vec4 position;
    }
    stream0[] =
    {
        {{-1.0f, -1.0f, 0.0f, 1.0f}},
        {{-1.0f,  1.0f, 0.0f, 1.0f}},
        {{-0.5f, -1.0f, 0.0f, 1.0f}},
        {{-0.5f,  1.0f, 0.0f, 1.0f}},
    };
    static const struct
    {
        struct vec2 color2;
        struct vec2 color1;
    }
    stream1[] =
    {
        {{0.5f, 0.5f}, {0.0f, 1.0f}},
        {{0.5f, 0.5f}, {1.0f, 1.0f}},
    };
    static const struct
    {
        struct vec2 color3;
        struct vec2 color0;
    }
    stream2[] =
    {
        {{0.5f, 0.5f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 1.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f}},
        {{0.5f, 0.5f}, {1.0f, 0.0f}},
    };
    static const float red[] = {1.0f, 0.0f, 0.0f, 0.5f};

    if (!init_test_context(&test_context, NULL))
        return;

    device = test_context.device;
    context = test_context.immediate_context;

    hr = ID3D11Device_CreateInputLayout(device, layout_desc, sizeof(layout_desc) / sizeof(*layout_desc),
            vs_code, sizeof(vs_code), &input_layout);
    ok(SUCCEEDED(hr), "Failed to create input layout, hr %#x.\n", hr);

    vb[0] = create_buffer(device, D3D11_BIND_VERTEX_BUFFER, sizeof(stream0), stream0);
    vb[1] = create_buffer(device, D3D11_BIND_VERTEX_BUFFER, sizeof(stream1), stream1);
    vb[2] = create_buffer(device, D3D11_BIND_VERTEX_BUFFER, sizeof(stream2), stream2);

    hr = ID3D11Device_CreateVertexShader(device, vs_code, sizeof(vs_code), NULL, &vs);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);
    hr = ID3D11Device_CreatePixelShader(device, ps_code, sizeof(ps_code), NULL, &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    ID3D11DeviceContext_IASetInputLayout(context, input_layout);
    ID3D11DeviceContext_IASetPrimitiveTopology(context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    offset = 0;
    stride = sizeof(*stream0);
    ID3D11DeviceContext_IASetVertexBuffers(context, 0, 1, &vb[0], &stride, &offset);
    stride = sizeof(*stream1);
    ID3D11DeviceContext_IASetVertexBuffers(context, 1, 1, &vb[1], &stride, &offset);
    stride = sizeof(*stream2);
    ID3D11DeviceContext_IASetVertexBuffers(context, 2, 1, &vb[2], &stride, &offset);
    ID3D11DeviceContext_VSSetShader(context, vs, NULL, 0);
    ID3D11DeviceContext_PSSetShader(context, ps, NULL, 0);

    ID3D11DeviceContext_ClearRenderTargetView(context, test_context.backbuffer_rtv, red);

    ID3D11DeviceContext_DrawInstanced(context, 4, 4, 0, 0);

    color = get_texture_color(test_context.backbuffer,  80, 240);
    ok(compare_color(color, 0xff0000ff, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 240, 240);
    ok(compare_color(color, 0xff00ff00, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 400, 240);
    ok(compare_color(color, 0xffff0000, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 560, 240);
    ok(compare_color(color, 0xffff00ff, 1), "Got unexpected color 0x%08x.\n", color);

    ID3D11PixelShader_Release(ps);
    ID3D11VertexShader_Release(vs);
    ID3D11Buffer_Release(vb[2]);
    ID3D11Buffer_Release(vb[1]);
    ID3D11Buffer_Release(vb[0]);
    ID3D11InputLayout_Release(input_layout);
    release_test_context(&test_context);
}

static void test_fragment_coords(void)
{
    struct d3d11_test_context test_context;
    ID3D11PixelShader *ps, *ps_frac;
    ID3D11DeviceContext *context;
    ID3D11Device *device;
    ID3D11Buffer *ps_cb;
    DWORD color;
    HRESULT hr;

    static const DWORD ps_code[] =
    {
#if 0
        float2 cutoff;

        float4 main(float4 position : SV_POSITION) : SV_TARGET
        {
            float4 ret = float4(0.0, 0.0, 0.0, 1.0);

            if (position.x > cutoff.x)
                ret.y = 1.0;
            if (position.y > cutoff.y)
                ret.z = 1.0;

            return ret;
        }
#endif
        0x43425844, 0x49fc9e51, 0x8068867d, 0xf20cfa39, 0xb8099e6b, 0x00000001, 0x00000144, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x000000a8, 0x00000040,
        0x0000002a, 0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x04002064, 0x00101032, 0x00000000,
        0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000001, 0x08000031, 0x00100032,
        0x00000000, 0x00208046, 0x00000000, 0x00000000, 0x00101046, 0x00000000, 0x0a000001, 0x00102062,
        0x00000000, 0x00100106, 0x00000000, 0x00004002, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000,
        0x08000036, 0x00102092, 0x00000000, 0x00004002, 0x00000000, 0x00000000, 0x00000000, 0x3f800000,
        0x0100003e,
    };
    static const DWORD ps_frac_code[] =
    {
#if 0
        float4 main(float4 position : SV_POSITION) : SV_TARGET
        {
            return float4(frac(position.xy), 0.0, 1.0);
        }
#endif
        0x43425844, 0x86d9d78a, 0x190b72c2, 0x50841fd6, 0xdc24022e, 0x00000001, 0x000000f8, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x0000005c, 0x00000040,
        0x00000017, 0x04002064, 0x00101032, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000,
        0x0500001a, 0x00102032, 0x00000000, 0x00101046, 0x00000000, 0x08000036, 0x001020c2, 0x00000000,
        0x00004002, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x0100003e,
    };
    static const float red[] = {1.0f, 0.0f, 0.0f, 0.5f};
    struct vec4 cutoff = {320.0f, 240.0f, 0.0f, 0.0f};

    if (!init_test_context(&test_context, NULL))
        return;

    device = test_context.device;
    context = test_context.immediate_context;

    ps_cb = create_buffer(device, D3D11_BIND_CONSTANT_BUFFER, sizeof(cutoff), &cutoff);

    hr = ID3D11Device_CreatePixelShader(device, ps_code, sizeof(ps_code), NULL, &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);
    hr = ID3D11Device_CreatePixelShader(device, ps_frac_code, sizeof(ps_frac_code), NULL, &ps_frac);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    ID3D11DeviceContext_PSSetConstantBuffers(context, 0, 1, &ps_cb);
    ID3D11DeviceContext_PSSetShader(context, ps, NULL, 0);

    ID3D11DeviceContext_ClearRenderTargetView(context, test_context.backbuffer_rtv, red);

    draw_quad(&test_context);

    color = get_texture_color(test_context.backbuffer, 319, 239);
    ok(compare_color(color, 0xff000000, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 320, 239);
    ok(compare_color(color, 0xff00ff00, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 319, 240);
    ok(compare_color(color, 0xffff0000, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 320, 240);
    ok(compare_color(color, 0xffffff00, 1), "Got unexpected color 0x%08x.\n", color);

    ID3D11Buffer_Release(ps_cb);
    cutoff.x = 16.0f;
    cutoff.y = 16.0f;
    ps_cb = create_buffer(device, D3D11_BIND_CONSTANT_BUFFER, sizeof(cutoff), &cutoff);
    ID3D11DeviceContext_PSSetConstantBuffers(context, 0, 1, &ps_cb);

    draw_quad(&test_context);

    color = get_texture_color(test_context.backbuffer, 14, 14);
    ok(compare_color(color, 0xff000000, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 18, 14);
    ok(compare_color(color, 0xff00ff00, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 14, 18);
    ok(compare_color(color, 0xffff0000, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 18, 18);
    ok(compare_color(color, 0xffffff00, 1), "Got unexpected color 0x%08x.\n", color);

    ID3D11DeviceContext_PSSetShader(context, ps_frac, NULL, 0);
    ID3D11DeviceContext_ClearRenderTargetView(context, test_context.backbuffer_rtv, red);

    ID3D11DeviceContext_Draw(context, 4, 0);

    color = get_texture_color(test_context.backbuffer, 14, 14);
    ok(compare_color(color, 0xff008080, 1), "Got unexpected color 0x%08x.\n", color);

    ID3D11Buffer_Release(ps_cb);
    ID3D11PixelShader_Release(ps_frac);
    ID3D11PixelShader_Release(ps);
    release_test_context(&test_context);
}

static void test_update_subresource(void)
{
    struct d3d11_test_context test_context;
    D3D11_TEXTURE2D_DESC texture_desc;
    ID3D11SamplerState *sampler_state;
    ID3D11ShaderResourceView *ps_srv;
    D3D11_SAMPLER_DESC sampler_desc;
    ID3D11DeviceContext *context;
    struct texture_readback rb;
    ID3D11Texture2D *texture;
    ID3D11PixelShader *ps;
    ID3D11Device *device;
    unsigned int i, j;
    D3D11_BOX box;
    DWORD color;
    HRESULT hr;

    static const DWORD ps_code[] =
    {
#if 0
        Texture2D t;
        SamplerState s;

        float4 main(float4 position : SV_POSITION) : SV_Target
        {
            float2 p;

            p.x = position.x / 640.0f;
            p.y = position.y / 480.0f;
            return t.Sample(s, p);
        }
#endif
        0x43425844, 0x1ce9b612, 0xc8176faa, 0xd37844af, 0xdb515605, 0x00000001, 0x00000134, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x00000098, 0x00000040,
        0x00000026, 0x0300005a, 0x00106000, 0x00000000, 0x04001858, 0x00107000, 0x00000000, 0x00005555,
        0x04002064, 0x00101032, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x02000068,
        0x00000001, 0x0a000038, 0x00100032, 0x00000000, 0x00101046, 0x00000000, 0x00004002, 0x3acccccd,
        0x3b088889, 0x00000000, 0x00000000, 0x09000045, 0x001020f2, 0x00000000, 0x00100046, 0x00000000,
        0x00107e46, 0x00000000, 0x00106000, 0x00000000, 0x0100003e,
    };
    static const float red[] = {1.0f, 0.0f, 0.0f, 0.5f};
    static const DWORD bitmap_data[] =
    {
        0xff0000ff, 0xff00ffff, 0xff00ff00, 0xffffff00,
        0xffff0000, 0xffff00ff, 0xff000000, 0xff7f7f7f,
        0xffffffff, 0xffffffff, 0xffffffff, 0xff000000,
        0xffffffff, 0xff000000, 0xff000000, 0xff000000,
    };
    static const DWORD expected_colors[] =
    {
        0xffffffff, 0xff000000, 0xffffffff, 0xff000000,
        0xff00ff00, 0xff0000ff, 0xff00ffff, 0x00000000,
        0xffffff00, 0xffff0000, 0xffff00ff, 0x00000000,
        0xff000000, 0xff7f7f7f, 0xffffffff, 0x00000000,
    };

    if (!init_test_context(&test_context, NULL))
        return;

    device = test_context.device;
    context = test_context.immediate_context;

    texture_desc.Width = 4;
    texture_desc.Height = 4;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    hr = ID3D11Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create 2d texture, hr %#x.\n", hr);

    hr = ID3D11Device_CreateShaderResourceView(device, (ID3D11Resource *)texture, NULL, &ps_srv);
    ok(SUCCEEDED(hr), "Failed to create shader resource view, hr %#x.\n", hr);

    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.MipLODBias = 0.0f;
    sampler_desc.MaxAnisotropy = 0;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampler_desc.BorderColor[0] = 0.0f;
    sampler_desc.BorderColor[1] = 0.0f;
    sampler_desc.BorderColor[2] = 0.0f;
    sampler_desc.BorderColor[3] = 0.0f;
    sampler_desc.MinLOD = 0.0f;
    sampler_desc.MaxLOD = 0.0f;

    hr = ID3D11Device_CreateSamplerState(device, &sampler_desc, &sampler_state);
    ok(SUCCEEDED(hr), "Failed to create sampler state, hr %#x.\n", hr);

    hr = ID3D11Device_CreatePixelShader(device, ps_code, sizeof(ps_code), NULL, &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    ID3D11DeviceContext_PSSetShaderResources(context, 0, 1, &ps_srv);
    ID3D11DeviceContext_PSSetSamplers(context, 0, 1, &sampler_state);
    ID3D11DeviceContext_PSSetShader(context, ps, NULL, 0);

    ID3D11DeviceContext_ClearRenderTargetView(context, test_context.backbuffer_rtv, red);
    check_texture_color(test_context.backbuffer, 0x7f0000ff, 1);

    draw_quad(&test_context);
    check_texture_color(test_context.backbuffer, 0x00000000, 0);

    set_box(&box, 1, 1, 0, 3, 3, 1);
    ID3D11DeviceContext_UpdateSubresource(context, (ID3D11Resource *)texture, 0, &box,
            bitmap_data, 4 * sizeof(*bitmap_data), 0);
    set_box(&box, 0, 3, 0, 3, 4, 1);
    ID3D11DeviceContext_UpdateSubresource(context, (ID3D11Resource *)texture, 0, &box,
            &bitmap_data[6], 4 * sizeof(*bitmap_data), 0);
    set_box(&box, 0, 0, 0, 4, 1, 1);
    ID3D11DeviceContext_UpdateSubresource(context, (ID3D11Resource *)texture, 0, &box,
            &bitmap_data[10], 4 * sizeof(*bitmap_data), 0);
    set_box(&box, 0, 1, 0, 1, 3, 1);
    ID3D11DeviceContext_UpdateSubresource(context, (ID3D11Resource *)texture, 0, &box,
            &bitmap_data[2], sizeof(*bitmap_data), 0);
    set_box(&box, 4, 4, 0, 3, 1, 1);
    ID3D11DeviceContext_UpdateSubresource(context, (ID3D11Resource *)texture, 0, &box,
            bitmap_data, sizeof(*bitmap_data), 0);
    set_box(&box, 0, 0, 0, 4, 4, 0);
    ID3D11DeviceContext_UpdateSubresource(context, (ID3D11Resource *)texture, 0, &box,
            bitmap_data, 4 * sizeof(*bitmap_data), 0);
    draw_quad(&test_context);
    get_texture_readback(test_context.backbuffer, 0, &rb);
    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            color = get_readback_color(&rb, 80 + j * 160, 60 + i * 120);
            ok(compare_color(color, expected_colors[j + i * 4], 1),
                    "Got unexpected color 0x%08x at (%u, %u), expected 0x%08x.\n",
                    color, j, i, expected_colors[j + i * 4]);
        }
    }
    release_texture_readback(&rb);

    ID3D11DeviceContext_UpdateSubresource(context, (ID3D11Resource *)texture, 0, NULL,
            bitmap_data, 4 * sizeof(*bitmap_data), 0);
    draw_quad(&test_context);
    get_texture_readback(test_context.backbuffer, 0, &rb);
    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            color = get_readback_color(&rb, 80 + j * 160, 60 + i * 120);
            ok(compare_color(color, bitmap_data[j + i * 4], 1),
                    "Got unexpected color 0x%08x at (%u, %u), expected 0x%08x.\n",
                    color, j, i, bitmap_data[j + i * 4]);
        }
    }
    release_texture_readback(&rb);

    ID3D11PixelShader_Release(ps);
    ID3D11SamplerState_Release(sampler_state);
    ID3D11ShaderResourceView_Release(ps_srv);
    ID3D11Texture2D_Release(texture);
    release_test_context(&test_context);
}

static void test_copy_subresource_region(void)
{
    ID3D11Texture2D *dst_texture, *src_texture;
    struct d3d11_test_context test_context;
    ID3D11Buffer *dst_buffer, *src_buffer;
    D3D11_SUBRESOURCE_DATA resource_data;
    D3D11_TEXTURE2D_DESC texture_desc;
    ID3D11SamplerState *sampler_state;
    ID3D11ShaderResourceView *ps_srv;
    D3D11_SAMPLER_DESC sampler_desc;
    ID3D11DeviceContext *context;
    struct vec4 float_colors[16];
    struct texture_readback rb;
    ID3D11PixelShader *ps;
    ID3D11Device *device;
    unsigned int i, j;
    D3D11_BOX box;
    DWORD color;
    HRESULT hr;

    static const DWORD ps_code[] =
    {
#if 0
        Texture2D t;
        SamplerState s;

        float4 main(float4 position : SV_POSITION) : SV_Target
        {
            float2 p;

            p.x = position.x / 640.0f;
            p.y = position.y / 480.0f;
            return t.Sample(s, p);
        }
#endif
        0x43425844, 0x1ce9b612, 0xc8176faa, 0xd37844af, 0xdb515605, 0x00000001, 0x00000134, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x00000098, 0x00000040,
        0x00000026, 0x0300005a, 0x00106000, 0x00000000, 0x04001858, 0x00107000, 0x00000000, 0x00005555,
        0x04002064, 0x00101032, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x02000068,
        0x00000001, 0x0a000038, 0x00100032, 0x00000000, 0x00101046, 0x00000000, 0x00004002, 0x3acccccd,
        0x3b088889, 0x00000000, 0x00000000, 0x09000045, 0x001020f2, 0x00000000, 0x00100046, 0x00000000,
        0x00107e46, 0x00000000, 0x00106000, 0x00000000, 0x0100003e,
    };
    static const DWORD ps_buffer_code[] =
    {
#if 0
        float4 buffer[16];

        float4 main(float4 position : SV_POSITION) : SV_TARGET
        {
            float2 p = (float2)4;
            p *= float2(position.x / 640.0f, position.y / 480.0f);
            return buffer[(int)p.y * 4 + (int)p.x];
        }
#endif
        0x43425844, 0x57e7139f, 0x4f0c9e52, 0x598b77e3, 0x5a239132, 0x00000001, 0x0000016c, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x000000d0, 0x00000040,
        0x00000034, 0x04000859, 0x00208e46, 0x00000000, 0x00000010, 0x04002064, 0x00101032, 0x00000000,
        0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000001, 0x0a000038, 0x00100032,
        0x00000000, 0x00101516, 0x00000000, 0x00004002, 0x3c088889, 0x3bcccccd, 0x00000000, 0x00000000,
        0x0500001b, 0x00100032, 0x00000000, 0x00100046, 0x00000000, 0x07000029, 0x00100012, 0x00000000,
        0x0010000a, 0x00000000, 0x00004001, 0x00000002, 0x0700001e, 0x00100012, 0x00000000, 0x0010000a,
        0x00000000, 0x0010001a, 0x00000000, 0x07000036, 0x001020f2, 0x00000000, 0x04208e46, 0x00000000,
        0x0010000a, 0x00000000, 0x0100003e,
    };
    static const float red[] = {1.0f, 0.0f, 0.0f, 0.5f};
    static const DWORD bitmap_data[] =
    {
        0xff0000ff, 0xff00ffff, 0xff00ff00, 0xffffff00,
        0xffff0000, 0xffff00ff, 0xff000000, 0xff7f7f7f,
        0xffffffff, 0xffffffff, 0xffffffff, 0xff000000,
        0xffffffff, 0xff000000, 0xff000000, 0xff000000,
    };
    static const DWORD expected_colors[] =
    {
        0xffffffff, 0xff000000, 0xff000000, 0xff000000,
        0xffffff00, 0xff0000ff, 0xff00ffff, 0x00000000,
        0xff7f7f7f, 0xffff0000, 0xffff00ff, 0xff7f7f7f,
        0xffffffff, 0xffffffff, 0xff000000, 0x00000000,
    };

    if (!init_test_context(&test_context, NULL))
        return;

    device = test_context.device;
    context = test_context.immediate_context;

    texture_desc.Width = 4;
    texture_desc.Height = 4;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    hr = ID3D11Device_CreateTexture2D(device, &texture_desc, NULL, &dst_texture);
    ok(SUCCEEDED(hr), "Failed to create 2d texture, hr %#x.\n", hr);

    texture_desc.Usage = D3D11_USAGE_IMMUTABLE;

    resource_data.pSysMem = bitmap_data;
    resource_data.SysMemPitch = 4 * sizeof(*bitmap_data);
    resource_data.SysMemSlicePitch = 0;

    hr = ID3D11Device_CreateTexture2D(device, &texture_desc, &resource_data, &src_texture);
    ok(SUCCEEDED(hr), "Failed to create 2d texture, hr %#x.\n", hr);

    hr = ID3D11Device_CreateShaderResourceView(device, (ID3D11Resource *)dst_texture, NULL, &ps_srv);
    ok(SUCCEEDED(hr), "Failed to create shader resource view, hr %#x.\n", hr);

    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.MipLODBias = 0.0f;
    sampler_desc.MaxAnisotropy = 0;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampler_desc.BorderColor[0] = 0.0f;
    sampler_desc.BorderColor[1] = 0.0f;
    sampler_desc.BorderColor[2] = 0.0f;
    sampler_desc.BorderColor[3] = 0.0f;
    sampler_desc.MinLOD = 0.0f;
    sampler_desc.MaxLOD = 0.0f;

    hr = ID3D11Device_CreateSamplerState(device, &sampler_desc, &sampler_state);
    ok(SUCCEEDED(hr), "Failed to create sampler state, hr %#x.\n", hr);

    hr = ID3D11Device_CreatePixelShader(device, ps_code, sizeof(ps_code), NULL, &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    ID3D11DeviceContext_PSSetShaderResources(context, 0, 1, &ps_srv);
    ID3D11DeviceContext_PSSetSamplers(context, 0, 1, &sampler_state);
    ID3D11DeviceContext_PSSetShader(context, ps, NULL, 0);

    ID3D11DeviceContext_ClearRenderTargetView(context, test_context.backbuffer_rtv, red);

    set_box(&box, 0, 0, 0, 2, 2, 1);
    ID3D11DeviceContext_CopySubresourceRegion(context, (ID3D11Resource *)dst_texture, 0,
            1, 1, 0, (ID3D11Resource *)src_texture, 0, &box);
    set_box(&box, 1, 2, 0, 4, 3, 1);
    ID3D11DeviceContext_CopySubresourceRegion(context, (ID3D11Resource *)dst_texture, 0,
            0, 3, 0, (ID3D11Resource *)src_texture, 0, &box);
    set_box(&box, 0, 3, 0, 4, 4, 1);
    ID3D11DeviceContext_CopySubresourceRegion(context, (ID3D11Resource *)dst_texture, 0,
            0, 0, 0, (ID3D11Resource *)src_texture, 0, &box);
    set_box(&box, 3, 0, 0, 4, 2, 1);
    ID3D11DeviceContext_CopySubresourceRegion(context, (ID3D11Resource *)dst_texture, 0,
            0, 1, 0, (ID3D11Resource *)src_texture, 0, &box);
    set_box(&box, 3, 1, 0, 4, 2, 1);
    ID3D11DeviceContext_CopySubresourceRegion(context, (ID3D11Resource *)dst_texture, 0,
            3, 2, 0, (ID3D11Resource *)src_texture, 0, &box);
    set_box(&box, 0, 0, 0, 4, 4, 0);
    ID3D11DeviceContext_CopySubresourceRegion(context, (ID3D11Resource *)dst_texture, 0,
            0, 0, 0, (ID3D11Resource *)src_texture, 0, &box);
    draw_quad(&test_context);
    get_texture_readback(test_context.backbuffer, 0, &rb);
    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            color = get_readback_color(&rb, 80 + j * 160, 60 + i * 120);
            ok(compare_color(color, expected_colors[j + i * 4], 1),
                    "Got unexpected color 0x%08x at (%u, %u), expected 0x%08x.\n",
                    color, j, i, expected_colors[j + i * 4]);
        }
    }
    release_texture_readback(&rb);

    ID3D11DeviceContext_CopySubresourceRegion(context, (ID3D11Resource *)dst_texture, 0,
            0, 0, 0, (ID3D11Resource *)src_texture, 0, NULL);
    draw_quad(&test_context);
    get_texture_readback(test_context.backbuffer, 0, &rb);
    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            color = get_readback_color(&rb, 80 + j * 160, 60 + i * 120);
            ok(compare_color(color, bitmap_data[j + i * 4], 1),
                    "Got unexpected color 0x%08x at (%u, %u), expected 0x%08x.\n",
                    color, j, i, bitmap_data[j + i * 4]);
        }
    }
    release_texture_readback(&rb);

    ID3D11PixelShader_Release(ps);
    hr = ID3D11Device_CreatePixelShader(device, ps_buffer_code, sizeof(ps_buffer_code), NULL, &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    ID3D11ShaderResourceView_Release(ps_srv);
    ps_srv = NULL;

    ID3D11SamplerState_Release(sampler_state);
    sampler_state = NULL;

    ID3D11Texture2D_Release(dst_texture);
    ID3D11Texture2D_Release(src_texture);

    ID3D11DeviceContext_PSSetShaderResources(context, 0, 1, &ps_srv);
    ID3D11DeviceContext_PSSetSamplers(context, 0, 1, &sampler_state);
    ID3D11DeviceContext_PSSetShader(context, ps, NULL, 0);

    dst_buffer = create_buffer(device, D3D11_BIND_CONSTANT_BUFFER, sizeof(float_colors), NULL);
    ID3D11DeviceContext_PSSetConstantBuffers(context, 0, 1, &dst_buffer);

    src_buffer = create_buffer(device, 0, 256 * sizeof(*float_colors), NULL);

    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            float_colors[j + i * 4].x = ((bitmap_data[j + i * 4] >>  0) & 0xff) / 255.0f;
            float_colors[j + i * 4].y = ((bitmap_data[j + i * 4] >>  8) & 0xff) / 255.0f;
            float_colors[j + i * 4].z = ((bitmap_data[j + i * 4] >> 16) & 0xff) / 255.0f;
            float_colors[j + i * 4].w = ((bitmap_data[j + i * 4] >> 24) & 0xff) / 255.0f;
        }
    }
    set_box(&box, 0, 0, 0, sizeof(float_colors), 1, 1);
    ID3D11DeviceContext_UpdateSubresource(context, (ID3D11Resource *)src_buffer, 0, &box, float_colors, 0, 0);

    set_box(&box, 0, 0, 0, sizeof(float_colors), 0, 1);
    ID3D11DeviceContext_CopySubresourceRegion(context, (ID3D11Resource *)dst_buffer, 0,
            0, 0, 0, (ID3D11Resource *)src_buffer, 0, &box);
    draw_quad(&test_context);
    check_texture_color(test_context.backbuffer, 0x00000000, 0);

    set_box(&box, 0, 0, 0, sizeof(float_colors), 1, 0);
    ID3D11DeviceContext_CopySubresourceRegion(context, (ID3D11Resource *)dst_buffer, 0,
            0, 0, 0, (ID3D11Resource *)src_buffer, 0, &box);
    draw_quad(&test_context);
    check_texture_color(test_context.backbuffer, 0x00000000, 0);

    set_box(&box, 0, 0, 0, sizeof(float_colors), 0, 0);
    ID3D11DeviceContext_CopySubresourceRegion(context, (ID3D11Resource *)dst_buffer, 0,
            0, 0, 0, (ID3D11Resource *)src_buffer, 0, &box);
    draw_quad(&test_context);
    check_texture_color(test_context.backbuffer, 0x00000000, 0);

    set_box(&box, 0, 0, 0, sizeof(float_colors), 1, 1);
    ID3D11DeviceContext_CopySubresourceRegion(context, (ID3D11Resource *)dst_buffer, 0,
            0, 0, 0, (ID3D11Resource *)src_buffer, 0, &box);
    draw_quad(&test_context);
    get_texture_readback(test_context.backbuffer, 0, &rb);
    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            color = get_readback_color(&rb, 80 + j * 160, 60 + i * 120);
            ok(compare_color(color, bitmap_data[j + i * 4], 1),
                    "Got unexpected color 0x%08x at (%u, %u), expected 0x%08x.\n",
                    color, j, i, bitmap_data[j + i * 4]);
        }
    }
    release_texture_readback(&rb);

    ID3D11Buffer_Release(dst_buffer);
    ID3D11Buffer_Release(src_buffer);
    ID3D11PixelShader_Release(ps);
    release_test_context(&test_context);
}

static void test_resource_map(void)
{
    D3D11_MAPPED_SUBRESOURCE mapped_subresource;
    D3D11_TEXTURE3D_DESC texture3d_desc;
    D3D11_TEXTURE2D_DESC texture2d_desc;
    D3D11_BUFFER_DESC buffer_desc;
    ID3D11DeviceContext *context;
    ID3D11Texture3D *texture3d;
    ID3D11Texture2D *texture2d;
    ID3D11Buffer *buffer;
    ID3D11Device *device;
    ULONG refcount;
    HRESULT hr;
    DWORD data;

    if (!(device = create_device(NULL)))
    {
        skip("Failed to create device.\n");
        return;
    }

    ID3D11Device_GetImmediateContext(device, &context);

    buffer_desc.ByteWidth = 1024;
    buffer_desc.Usage = D3D11_USAGE_STAGING;
    buffer_desc.BindFlags = 0;
    buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    buffer_desc.MiscFlags = 0;
    buffer_desc.StructureByteStride = 0;

    hr = ID3D11Device_CreateBuffer(device, &buffer_desc, NULL, &buffer);
    ok(SUCCEEDED(hr), "Failed to create a buffer, hr %#x.\n", hr);

    hr = ID3D11DeviceContext_Map(context, (ID3D11Resource *)buffer, 1, D3D11_MAP_READ, 0, &mapped_subresource);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    memset(&mapped_subresource, 0, sizeof(mapped_subresource));
    hr = ID3D11DeviceContext_Map(context, (ID3D11Resource *)buffer, 0, D3D11_MAP_WRITE, 0, &mapped_subresource);
    ok(SUCCEEDED(hr), "Failed to map buffer, hr %#x.\n", hr);
    ok(mapped_subresource.RowPitch == 1024, "Got unexpected row pitch %u.\n", mapped_subresource.RowPitch);
    ok(mapped_subresource.DepthPitch == 1024, "Got unexpected depth pitch %u.\n", mapped_subresource.DepthPitch);
    *((DWORD *)mapped_subresource.pData) = 0xdeadbeef;
    ID3D11DeviceContext_Unmap(context, (ID3D11Resource *)buffer, 0);

    memset(&mapped_subresource, 0, sizeof(mapped_subresource));
    hr = ID3D11DeviceContext_Map(context, (ID3D11Resource *)buffer, 0, D3D11_MAP_READ, 0, &mapped_subresource);
    ok(SUCCEEDED(hr), "Failed to map buffer, hr %#x.\n", hr);
    ok(mapped_subresource.RowPitch == 1024, "Got unexpected row pitch %u.\n", mapped_subresource.RowPitch);
    ok(mapped_subresource.DepthPitch == 1024, "Got unexpected depth pitch %u.\n", mapped_subresource.DepthPitch);
    data = *((DWORD *)mapped_subresource.pData);
    ok(data == 0xdeadbeef, "Got unexpected data %#x.\n", data);
    ID3D11DeviceContext_Unmap(context, (ID3D11Resource *)buffer, 0);

    refcount = ID3D11Buffer_Release(buffer);
    ok(!refcount, "Buffer has %u references left.\n", refcount);

    texture2d_desc.Width = 512;
    texture2d_desc.Height = 512;
    texture2d_desc.MipLevels = 1;
    texture2d_desc.ArraySize = 1;
    texture2d_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture2d_desc.SampleDesc.Count = 1;
    texture2d_desc.SampleDesc.Quality = 0;
    texture2d_desc.Usage = D3D11_USAGE_STAGING;
    texture2d_desc.BindFlags = 0;
    texture2d_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    texture2d_desc.MiscFlags = 0;

    hr = ID3D11Device_CreateTexture2D(device, &texture2d_desc, NULL, &texture2d);
    ok(SUCCEEDED(hr), "Failed to create 2d texture, hr %#x.\n", hr);

    hr = ID3D11DeviceContext_Map(context, (ID3D11Resource *)texture2d, 1, D3D11_MAP_READ, 0, &mapped_subresource);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    memset(&mapped_subresource, 0, sizeof(mapped_subresource));
    hr = ID3D11DeviceContext_Map(context, (ID3D11Resource *)texture2d, 0, D3D11_MAP_WRITE, 0, &mapped_subresource);
    ok(SUCCEEDED(hr), "Failed to map texture, hr %#x.\n", hr);
    ok(mapped_subresource.RowPitch == 4 * 512, "Got unexpected row pitch %u.\n", mapped_subresource.RowPitch);
    ok(mapped_subresource.DepthPitch == 4 * 512 * 512, "Got unexpected depth pitch %u.\n",
            mapped_subresource.DepthPitch);
    *((DWORD *)mapped_subresource.pData) = 0xdeadbeef;
    ID3D11DeviceContext_Unmap(context, (ID3D11Resource *)texture2d, 0);

    memset(&mapped_subresource, 0, sizeof(mapped_subresource));
    hr = ID3D11DeviceContext_Map(context, (ID3D11Resource *)texture2d, 0, D3D11_MAP_READ, 0, &mapped_subresource);
    ok(SUCCEEDED(hr), "Failed to map texture, hr %#x.\n", hr);
    ok(mapped_subresource.RowPitch == 4 * 512, "Got unexpected row pitch %u.\n", mapped_subresource.RowPitch);
    ok(mapped_subresource.DepthPitch == 4 * 512 * 512, "Got unexpected depth pitch %u.\n",
            mapped_subresource.DepthPitch);
    data = *((DWORD *)mapped_subresource.pData);
    ok(data == 0xdeadbeef, "Got unexpected data %#x.\n", data);
    ID3D11DeviceContext_Unmap(context, (ID3D11Resource *)texture2d, 0);

    refcount = ID3D11Texture2D_Release(texture2d);
    ok(!refcount, "2D texture has %u references left.\n", refcount);

    texture3d_desc.Width = 64;
    texture3d_desc.Height = 64;
    texture3d_desc.Depth = 64;
    texture3d_desc.MipLevels = 1;
    texture3d_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture3d_desc.Usage = D3D11_USAGE_STAGING;
    texture3d_desc.BindFlags = 0;
    texture3d_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    texture3d_desc.MiscFlags = 0;

    hr = ID3D11Device_CreateTexture3D(device, &texture3d_desc, NULL, &texture3d);
    ok(SUCCEEDED(hr), "Failed to create 3d texture, hr %#x.\n", hr);

    hr = ID3D11DeviceContext_Map(context, (ID3D11Resource *)texture3d, 1, D3D11_MAP_READ, 0, &mapped_subresource);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    memset(&mapped_subresource, 0, sizeof(mapped_subresource));
    hr = ID3D11DeviceContext_Map(context, (ID3D11Resource *)texture3d, 0, D3D11_MAP_WRITE, 0, &mapped_subresource);
    todo_wine ok(SUCCEEDED(hr), "Failed to map texture, hr %#x.\n", hr);
    if (FAILED(hr)) goto done;
    ok(mapped_subresource.RowPitch == 4 * 64, "Got unexpected row pitch %u.\n", mapped_subresource.RowPitch);
    ok(mapped_subresource.DepthPitch == 4 * 64 * 64, "Got unexpected depth pitch %u.\n",
            mapped_subresource.DepthPitch);
    *((DWORD *)mapped_subresource.pData) = 0xdeadbeef;
    ID3D11DeviceContext_Unmap(context, (ID3D11Resource *)texture3d, 0);

    memset(&mapped_subresource, 0, sizeof(mapped_subresource));
    hr = ID3D11DeviceContext_Map(context, (ID3D11Resource *)texture3d, 0, D3D11_MAP_READ, 0, &mapped_subresource);
    ok(SUCCEEDED(hr), "Failed to map texture, hr %#x.\n", hr);
    ok(mapped_subresource.RowPitch == 4 * 64, "Got unexpected row pitch %u.\n", mapped_subresource.RowPitch);
    ok(mapped_subresource.DepthPitch == 4 * 64 * 64, "Got unexpected depth pitch %u.\n",
            mapped_subresource.DepthPitch);
    data = *((DWORD *)mapped_subresource.pData);
    ok(data == 0xdeadbeef, "Got unexpected data %#x.\n", data);
    ID3D11DeviceContext_Unmap(context, (ID3D11Resource *)texture3d, 0);

done:
    refcount = ID3D11Texture3D_Release(texture3d);
    ok(!refcount, "3D texture has %u references left.\n", refcount);

    ID3D11DeviceContext_Release(context);

    refcount = ID3D11Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_texture_data_init(void)
{
    static const float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
    struct d3d11_test_context test_context;
    ID3D11DeviceContext *context;
    D3D11_TEXTURE2D_DESC desc;
    ID3D11Texture2D *texture;
    ID3D11Device *device;
    UINT count = 0;
    HRESULT hr;

    if (!init_test_context(&test_context, NULL))
        return;

    device = test_context.device;
    context = test_context.immediate_context;

    desc.Width = 640;
    desc.Height = 480;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    hr = ID3D11Device_CreateTexture2D(device, &desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    check_texture_color(texture, 0x00000000, 0);
    ID3D11Texture2D_Release(texture);

    desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    hr = ID3D11Device_CreateTexture2D(device, &desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    check_texture_color(texture, 0x00000000, 0);
    ID3D11Texture2D_Release(texture);

    desc.Format = DXGI_FORMAT_D32_FLOAT;
    hr = ID3D11Device_CreateTexture2D(device, &desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    check_texture_float(texture, 0.0f, 0);
    ID3D11Texture2D_Release(texture);

    desc.ArraySize = 4;

    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    hr = ID3D11Device_CreateTexture2D(device, &desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    check_texture_float(texture, 0.0f, 0);
    ID3D11Texture2D_Release(texture);

    desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    hr = ID3D11Device_CreateTexture2D(device, &desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    check_texture_color(texture, 0x00000000, 0);
    ID3D11Texture2D_Release(texture);

    desc.Format = DXGI_FORMAT_D32_FLOAT;
    hr = ID3D11Device_CreateTexture2D(device, &desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    check_texture_float(texture, 0.0f, 0);
    ID3D11Texture2D_Release(texture);

    hr = ID3D11Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_R8G8B8A8_UNORM, 2, &count);
    ok(SUCCEEDED(hr), "Failed to get quality levels, hr %#x.\n", hr);
    if (!count)
    {
        skip("Multisampling not supported for DXGI_FORMAT_R8G8B8A8_UNORM, skipping tests.\n");
        release_test_context(&test_context);
        return;
    }

    ID3D11DeviceContext_ClearRenderTargetView(context, test_context.backbuffer_rtv, white);

    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 2;
    desc.SampleDesc.Quality = 0;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    hr = ID3D11Device_CreateTexture2D(device, &desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

    ID3D11DeviceContext_ResolveSubresource(context, (ID3D11Resource *)test_context.backbuffer, 0,
            (ID3D11Resource *)texture, 0, DXGI_FORMAT_R8G8B8A8_UNORM);

    todo_wine check_texture_color(test_context.backbuffer, 0x00000000, 0);

    ID3D11Texture2D_Release(texture);

    release_test_context(&test_context);
}

static void test_check_multisample_quality_levels(void)
{
    ID3D11Device *device;
    UINT quality_levels;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device(NULL)))
    {
        skip("Failed to create device.\n");
        return;
    }

    ID3D11Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_R8G8B8A8_UNORM, 2, &quality_levels);
    if (!quality_levels)
    {
        skip("Multisampling not supported for DXGI_FORMAT_R8G8B8A8_UNORM, skipping test.\n");
        goto done;
    }

    quality_levels = 0xdeadbeef;
    hr = ID3D11Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_UNKNOWN, 2, &quality_levels);
    todo_wine ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    ok(!quality_levels, "Got unexpected quality_levels %u.\n", quality_levels);
    quality_levels = 0xdeadbeef;
    hr = ID3D11Device_CheckMultisampleQualityLevels(device, 65536, 2, &quality_levels);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    todo_wine ok(quality_levels == 0xdeadbeef, "Got unexpected quality_levels %u.\n", quality_levels);

    quality_levels = 0xdeadbeef;
    hr = ID3D11Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_R8G8B8A8_UNORM, 0, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    hr = ID3D11Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_R8G8B8A8_UNORM, 0, &quality_levels);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);
    ok(!quality_levels, "Got unexpected quality_levels %u.\n", quality_levels);

    quality_levels = 0xdeadbeef;
    hr = ID3D11Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_R8G8B8A8_UNORM, 1, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    hr = ID3D11Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_R8G8B8A8_UNORM, 1, &quality_levels);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    ok(quality_levels == 1, "Got unexpected quality_levels %u.\n", quality_levels);

    quality_levels = 0xdeadbeef;
    hr = ID3D11Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_R8G8B8A8_UNORM, 2, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    hr = ID3D11Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_R8G8B8A8_UNORM, 2, &quality_levels);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    ok(quality_levels, "Got unexpected quality_levels %u.\n", quality_levels);

    /* We assume 15 samples multisampling is never supported in practice. */
    quality_levels = 0xdeadbeef;
    hr = ID3D11Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_R8G8B8A8_UNORM, 15, &quality_levels);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    ok(!quality_levels, "Got unexpected quality_levels %u.\n", quality_levels);
    hr = ID3D11Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_R8G8B8A8_UNORM, 32, &quality_levels);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    quality_levels = 0xdeadbeef;
    hr = ID3D11Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_R8G8B8A8_UNORM, 33, &quality_levels);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);
    ok(!quality_levels, "Got unexpected quality_levels %u.\n", quality_levels);
    quality_levels = 0xdeadbeef;
    hr = ID3D11Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_R8G8B8A8_UNORM, 64, &quality_levels);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);
    ok(!quality_levels, "Got unexpected quality_levels %u.\n", quality_levels);

    hr = ID3D11Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_BC3_UNORM, 2, &quality_levels);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    ok(!quality_levels, "Got unexpected quality_levels %u.\n", quality_levels);

done:
    refcount = ID3D11Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_swapchain_flip(void)
{
    ID3D11Texture2D *backbuffer_0, *backbuffer_1, *backbuffer_2, *offscreen;
    ID3D11ShaderResourceView *backbuffer_0_srv, *backbuffer_1_srv;
    ID3D11RenderTargetView *backbuffer_0_rtv, *offscreen_rtv;
    D3D11_TEXTURE2D_DESC texture_desc;
    ID3D11InputLayout *input_layout;
    ID3D11DeviceContext *context;
    unsigned int stride, offset;
    struct swapchain_desc desc;
    IDXGISwapChain *swapchain;
    ID3D11VertexShader *vs;
    ID3D11PixelShader *ps;
    ID3D11Device *device;
    D3D11_VIEWPORT vp;
    ID3D11Buffer *vb;
    ULONG refcount;
    DWORD color;
    HWND window;
    HRESULT hr;

    static const D3D11_INPUT_ELEMENT_DESC layout_desc[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    static const DWORD vs_code[] =
    {
#if 0
        float4 main(float4 position : POSITION) : SV_POSITION
        {
            return position;
        }
#endif
        0x43425844, 0xa7a2f22d, 0x83ff2560, 0xe61638bd, 0x87e3ce90, 0x00000001, 0x000000d8, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x49534f50, 0x4e4f4954, 0xababab00,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000001, 0x00000003,
        0x00000000, 0x0000000f, 0x505f5653, 0x5449534f, 0x004e4f49, 0x52444853, 0x0000003c, 0x00010040,
        0x0000000f, 0x0300005f, 0x001010f2, 0x00000000, 0x04000067, 0x001020f2, 0x00000000, 0x00000001,
        0x05000036, 0x001020f2, 0x00000000, 0x00101e46, 0x00000000, 0x0100003e,
    };

    static const DWORD ps_code[] =
    {
#if 0
        Texture2D t0, t1;
        SamplerState s;

        float4 main(float4 position : SV_POSITION) : SV_Target
        {
            float2 p;

            p.x = 0.5;
            p.y = 0.5;
            if (position.x < 320)
                return t0.Sample(s, p);
            return t1.Sample(s, p);
        }
#endif
        0x43425844, 0x1733542c, 0xf74c6b6a, 0x0fb11eac, 0x76f6a999, 0x00000001, 0x000002cc, 0x00000005,
        0x00000034, 0x000000f4, 0x00000128, 0x0000015c, 0x00000250, 0x46454452, 0x000000b8, 0x00000000,
        0x00000000, 0x00000003, 0x0000001c, 0xffff0400, 0x00000100, 0x00000084, 0x0000007c, 0x00000003,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x0000007e, 0x00000002,
        0x00000005, 0x00000004, 0xffffffff, 0x00000000, 0x00000001, 0x0000000c, 0x00000081, 0x00000002,
        0x00000005, 0x00000004, 0xffffffff, 0x00000001, 0x00000001, 0x0000000c, 0x30740073, 0x00317400,
        0x7263694d, 0x666f736f, 0x52282074, 0x4c482029, 0x53204c53, 0x65646168, 0x6f432072, 0x6c69706d,
        0x39207265, 0x2e39322e, 0x2e323539, 0x31313133, 0xababab00, 0x4e475349, 0x0000002c, 0x00000001,
        0x00000008, 0x00000020, 0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000010f, 0x505f5653,
        0x5449534f, 0x004e4f49, 0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000,
        0x00000000, 0x00000003, 0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853,
        0x000000ec, 0x00000040, 0x0000003b, 0x0300005a, 0x00106000, 0x00000000, 0x04001858, 0x00107000,
        0x00000000, 0x00005555, 0x04001858, 0x00107000, 0x00000001, 0x00005555, 0x04002064, 0x00101012,
        0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000001, 0x07000031,
        0x00100012, 0x00000000, 0x0010100a, 0x00000000, 0x00004001, 0x43a00000, 0x0304001f, 0x0010000a,
        0x00000000, 0x0c000045, 0x001020f2, 0x00000000, 0x00004002, 0x3f000000, 0x3f000000, 0x00000000,
        0x00000000, 0x00107e46, 0x00000000, 0x00106000, 0x00000000, 0x0100003e, 0x01000015, 0x0c000045,
        0x001020f2, 0x00000000, 0x00004002, 0x3f000000, 0x3f000000, 0x00000000, 0x00000000, 0x00107e46,
        0x00000001, 0x00106000, 0x00000000, 0x0100003e, 0x54415453, 0x00000074, 0x00000007, 0x00000001,
        0x00000000, 0x00000002, 0x00000001, 0x00000000, 0x00000000, 0x00000002, 0x00000001, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000
    };
    static const struct
    {
        struct vec2 position;
    }
    quad[] =
    {
        {{-1.0f, -1.0f}},
        {{-1.0f,  1.0f}},
        {{ 1.0f, -1.0f}},
        {{ 1.0f,  1.0f}},
    };
    static const float red[] = {1.0f, 0.0f, 0.0f, 0.5f};
    static const float green[] = {0.0f, 1.0f, 0.0f, 0.5f};
    static const float blue[] = {0.0f, 0.0f, 1.0f, 0.5f};

    if (!(device = create_device(NULL)))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }
    window = CreateWindowA("static", "d3d11_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    desc.buffer_count = 3;
    desc.swap_effect = DXGI_SWAP_EFFECT_SEQUENTIAL;
    desc.windowed = TRUE;
    desc.flags = SWAPCHAIN_FLAG_SHADER_INPUT;
    swapchain = create_swapchain(device, window, &desc);

    hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_ID3D11Texture2D, (void **)&backbuffer_0);
    ok(SUCCEEDED(hr), "Failed to get buffer, hr %#x.\n", hr);
    hr = IDXGISwapChain_GetBuffer(swapchain, 1, &IID_ID3D11Texture2D, (void **)&backbuffer_1);
    ok(SUCCEEDED(hr), "Failed to get buffer, hr %#x.\n", hr);
    hr = IDXGISwapChain_GetBuffer(swapchain, 2, &IID_ID3D11Texture2D, (void **)&backbuffer_2);
    ok(SUCCEEDED(hr), "Failed to get buffer, hr %#x.\n", hr);

    hr = ID3D11Device_CreateRenderTargetView(device, (ID3D11Resource *)backbuffer_0, NULL, &backbuffer_0_rtv);
    ok(SUCCEEDED(hr), "Failed to create rendertarget view, hr %#x.\n", hr);
    hr = ID3D11Device_CreateShaderResourceView(device, (ID3D11Resource *)backbuffer_0, NULL, &backbuffer_0_srv);
    ok(SUCCEEDED(hr), "Failed to create shader resource view, hr %#x.\n", hr);
    hr = ID3D11Device_CreateShaderResourceView(device, (ID3D11Resource *)backbuffer_1, NULL, &backbuffer_1_srv);
    ok(SUCCEEDED(hr), "Failed to create shader resource view, hr %#x.\n", hr);

    ID3D11Texture2D_GetDesc(backbuffer_0, &texture_desc);
    todo_wine ok((texture_desc.BindFlags & (D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE))
            == (D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE),
            "Got unexpected bind flags %x.\n", texture_desc.BindFlags);
    ok(texture_desc.Usage == D3D11_USAGE_DEFAULT, "Got unexpected usage %u.\n", texture_desc.Usage);

    ID3D11Texture2D_GetDesc(backbuffer_1, &texture_desc);
    todo_wine ok((texture_desc.BindFlags & (D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE))
            == (D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE),
            "Got unexpected bind flags %x.\n", texture_desc.BindFlags);
    ok(texture_desc.Usage == D3D11_USAGE_DEFAULT, "Got unexpected usage %u.\n", texture_desc.Usage);

    hr = ID3D11Device_CreateRenderTargetView(device, (ID3D11Resource *)backbuffer_1, NULL, &offscreen_rtv);
    todo_wine ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    if (SUCCEEDED(hr))
        ID3D11RenderTargetView_Release(offscreen_rtv);

    ID3D11Device_GetImmediateContext(device, &context);

    ID3D11DeviceContext_PSSetShaderResources(context, 0, 1, &backbuffer_0_srv);
    ID3D11DeviceContext_PSSetShaderResources(context, 1, 1, &backbuffer_1_srv);

    texture_desc.Width = 640;
    texture_desc.Height = 480;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;
    hr = ID3D11Device_CreateTexture2D(device, &texture_desc, NULL, &offscreen);
    ok(SUCCEEDED(hr), "Failed to create a 2d texture, hr %#x.\n", hr);
    hr = ID3D11Device_CreateRenderTargetView(device, (ID3D11Resource *)offscreen, NULL, &offscreen_rtv);
    ok(SUCCEEDED(hr), "Failed to create rendertarget view, hr %#x.\n", hr);
    ID3D11DeviceContext_OMSetRenderTargets(context, 1, &offscreen_rtv, NULL);
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width = 640;
    vp.Height = 480;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    ID3D11DeviceContext_RSSetViewports(context, 1, &vp);

    vb = create_buffer(device, D3D11_BIND_VERTEX_BUFFER, sizeof(quad), quad);

    hr = ID3D11Device_CreateVertexShader(device, vs_code, sizeof(vs_code), NULL, &vs);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);
    hr = ID3D11Device_CreateInputLayout(device, layout_desc, sizeof(layout_desc) / sizeof(*layout_desc),
            vs_code, sizeof(vs_code), &input_layout);
    ok(SUCCEEDED(hr), "Failed to create input layout, hr %#x.\n", hr);
    ID3D11DeviceContext_IASetInputLayout(context, input_layout);
    ID3D11DeviceContext_IASetPrimitiveTopology(context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    ID3D11DeviceContext_VSSetShader(context, vs, NULL, 0);
    stride = sizeof(*quad);
    offset = 0;
    ID3D11DeviceContext_IASetVertexBuffers(context, 0, 1, &vb, &stride, &offset);

    hr = ID3D11Device_CreatePixelShader(device, ps_code, sizeof(ps_code), NULL, &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);
    ID3D11DeviceContext_PSSetShader(context, ps, NULL, 0);

    ID3D11DeviceContext_ClearRenderTargetView(context, backbuffer_0_rtv, red);

    ID3D11DeviceContext_Draw(context, 4, 0);
    color = get_texture_color(offscreen, 120, 240);
    todo_wine ok(compare_color(color, 0x7f0000ff, 1), "Got unexpected color 0x%08x.\n", color);

    /* DXGI moves buffers in the same direction as earlier versions. Buffer 2
     * becomes buffer 1, buffer 1 becomes the new buffer 0, and buffer 0
     * becomes buffer n - 1. However, only buffer 0 can be rendered to.
     *
     * What is this good for? I don't know. Ad-hoc tests suggest that
     * Present() always waits for the next V-sync interval, even if there are
     * still untouched buffers. Buffer 0 is the buffer that is shown on the
     * screen, just like in <= d3d9. Present() also doesn't discard buffers if
     * rendering finishes before the V-sync interval is over. I haven't found
     * any productive use for more than one buffer. */
    IDXGISwapChain_Present(swapchain, 0, 0);

    ID3D11DeviceContext_ClearRenderTargetView(context, backbuffer_0_rtv, green);

    ID3D11DeviceContext_Draw(context, 4, 0);
    color = get_texture_color(offscreen, 120, 240); /* green, buf 0 */
    todo_wine ok(compare_color(color, 0x7f00ff00, 1), "Got unexpected color 0x%08x.\n", color);
    /* Buffer 1 is still untouched. */

    color = get_texture_color(backbuffer_0, 320, 240); /* green */
    ok(compare_color(color, 0x7f00ff00, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(backbuffer_2, 320, 240); /* red */
    ok(compare_color(color, 0x7f0000ff, 1), "Got unexpected color 0x%08x.\n", color);

    IDXGISwapChain_Present(swapchain, 0, 0);

    ID3D11DeviceContext_ClearRenderTargetView(context, backbuffer_0_rtv, blue);

    ID3D11DeviceContext_Draw(context, 4, 0);
    color = get_texture_color(offscreen, 120, 240); /* blue, buf 0 */
    todo_wine ok(compare_color(color, 0x7fff0000, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(offscreen, 360, 240); /* red, buf 1 */
    todo_wine ok(compare_color(color, 0x7f0000ff, 1), "Got unexpected color 0x%08x.\n", color);

    color = get_texture_color(backbuffer_0, 320, 240); /* blue */
    ok(compare_color(color, 0x7fff0000, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(backbuffer_1, 320, 240); /* red */
    ok(compare_color(color, 0x7f0000ff, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(backbuffer_2, 320, 240); /* green */
    ok(compare_color(color, 0x7f00ff00, 1), "Got unexpected color 0x%08x.\n", color);

    ID3D11VertexShader_Release(vs);
    ID3D11PixelShader_Release(ps);
    ID3D11Buffer_Release(vb);
    ID3D11InputLayout_Release(input_layout);
    ID3D11ShaderResourceView_Release(backbuffer_0_srv);
    ID3D11ShaderResourceView_Release(backbuffer_1_srv);
    ID3D11RenderTargetView_Release(backbuffer_0_rtv);
    ID3D11RenderTargetView_Release(offscreen_rtv);
    ID3D11Texture2D_Release(offscreen);
    ID3D11Texture2D_Release(backbuffer_0);
    ID3D11Texture2D_Release(backbuffer_1);
    ID3D11Texture2D_Release(backbuffer_2);
    IDXGISwapChain_Release(swapchain);

    ID3D11DeviceContext_Release(context);
    refcount = ID3D11Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_clear_render_target_view(void)
{
    static const DWORD expected_color = 0xbf4c7f19, expected_srgb_color = 0xbf95bc59;
    static const float color[] = {0.1f, 0.5f, 0.3f, 0.75f};
    static const float green[] = {0.0f, 1.0f, 0.0f, 0.5f};

    ID3D11Texture2D *texture, *srgb_texture;
    struct d3d11_test_context test_context;
    ID3D11RenderTargetView *rtv, *srgb_rtv;
    D3D11_RENDER_TARGET_VIEW_DESC rtv_desc;
    D3D11_TEXTURE2D_DESC texture_desc;
    ID3D11DeviceContext *context;
    ID3D11Device *device;
    HRESULT hr;

    if (!init_test_context(&test_context, NULL))
        return;

    device = test_context.device;
    context = test_context.immediate_context;

    texture_desc.Width = 640;
    texture_desc.Height = 480;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;
    hr = ID3D11Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create depth texture, hr %#x.\n", hr);

    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    hr = ID3D11Device_CreateTexture2D(device, &texture_desc, NULL, &srgb_texture);
    ok(SUCCEEDED(hr), "Failed to create depth texture, hr %#x.\n", hr);

    hr = ID3D11Device_CreateRenderTargetView(device, (ID3D11Resource *)texture, NULL, &rtv);
    ok(SUCCEEDED(hr), "Failed to create render target view, hr %#x.\n", hr);

    hr = ID3D11Device_CreateRenderTargetView(device, (ID3D11Resource *)srgb_texture, NULL, &srgb_rtv);
    ok(SUCCEEDED(hr), "Failed to create render target view, hr %#x.\n", hr);

    ID3D11DeviceContext_ClearRenderTargetView(context, test_context.backbuffer_rtv, color);
    check_texture_color(test_context.backbuffer, expected_color, 1);

    ID3D11DeviceContext_ClearRenderTargetView(context, rtv, color);
    check_texture_color(texture, expected_color, 1);

    ID3D11DeviceContext_ClearRenderTargetView(context, NULL, green);
    check_texture_color(texture, expected_color, 1);

    ID3D11DeviceContext_ClearRenderTargetView(context, srgb_rtv, color);
    check_texture_color(srgb_texture, expected_srgb_color, 1);

    ID3D11RenderTargetView_Release(srgb_rtv);
    ID3D11RenderTargetView_Release(rtv);
    ID3D11Texture2D_Release(srgb_texture);
    ID3D11Texture2D_Release(texture);

    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
    hr = ID3D11Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create depth texture, hr %#x.\n", hr);

    rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    U(rtv_desc).Texture2D.MipSlice = 0;
    hr = ID3D11Device_CreateRenderTargetView(device, (ID3D11Resource *)texture, &rtv_desc, &srgb_rtv);
    ok(SUCCEEDED(hr), "Failed to create render target view, hr %#x.\n", hr);

    rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    U(rtv_desc).Texture2D.MipSlice = 0;
    hr = ID3D11Device_CreateRenderTargetView(device, (ID3D11Resource *)texture, &rtv_desc, &rtv);
    ok(SUCCEEDED(hr), "Failed to create render target view, hr %#x.\n", hr);

    ID3D11DeviceContext_ClearRenderTargetView(context, rtv, color);
    check_texture_color(texture, expected_color, 1);

    if (!is_warp_device(device))
    {
        ID3D11DeviceContext_ClearRenderTargetView(context, srgb_rtv, color);
        todo_wine check_texture_color(texture, expected_srgb_color, 1);
    }
    else
    {
        win_skip("Skipping broken test on WARP.\n");
    }


    ID3D11RenderTargetView_Release(srgb_rtv);
    ID3D11RenderTargetView_Release(rtv);
    ID3D11Texture2D_Release(texture);
    release_test_context(&test_context);
}

static void test_clear_depth_stencil_view(void)
{
    D3D11_TEXTURE2D_DESC texture_desc;
    ID3D11Texture2D *depth_texture;
    ID3D11DeviceContext *context;
    ID3D11DepthStencilView *dsv;
    ID3D11Device *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device(NULL)))
    {
        skip("Failed to create device.\n");
        return;
    }

    ID3D11Device_GetImmediateContext(device, &context);

    texture_desc.Width = 640;
    texture_desc.Height = 480;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_D32_FLOAT;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;
    hr = ID3D11Device_CreateTexture2D(device, &texture_desc, NULL, &depth_texture);
    ok(SUCCEEDED(hr), "Failed to create depth texture, hr %#x.\n", hr);

    hr = ID3D11Device_CreateDepthStencilView(device, (ID3D11Resource *)depth_texture, NULL, &dsv);
    ok(SUCCEEDED(hr), "Failed to create depth stencil view, hr %#x.\n", hr);

    ID3D11DeviceContext_ClearDepthStencilView(context, dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
    check_texture_float(depth_texture, 1.0f, 0);

    ID3D11DeviceContext_ClearDepthStencilView(context, dsv, D3D11_CLEAR_DEPTH, 0.25f, 0);
    check_texture_float(depth_texture, 0.25f, 0);

    ID3D11DeviceContext_ClearDepthStencilView(context, NULL, D3D11_CLEAR_DEPTH, 1.0f, 0);
    check_texture_float(depth_texture, 0.25f, 0);

    ID3D11Texture2D_Release(depth_texture);
    ID3D11DepthStencilView_Release(dsv);

    texture_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    hr = ID3D11Device_CreateTexture2D(device, &texture_desc, NULL, &depth_texture);
    ok(SUCCEEDED(hr), "Failed to create depth texture, hr %#x.\n", hr);

    hr = ID3D11Device_CreateDepthStencilView(device, (ID3D11Resource *)depth_texture, NULL, &dsv);
    ok(SUCCEEDED(hr), "Failed to create depth stencil view, hr %#x.\n", hr);

    ID3D11DeviceContext_ClearDepthStencilView(context, dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    todo_wine check_texture_color(depth_texture, 0x00ffffff, 0);

    ID3D11DeviceContext_ClearDepthStencilView(context, dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.0f, 0xff);
    todo_wine check_texture_color(depth_texture, 0xff000000, 0);

    ID3D11DeviceContext_ClearDepthStencilView(context, dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0xff);
    check_texture_color(depth_texture, 0xffffffff, 0);

    ID3D11DeviceContext_ClearDepthStencilView(context, dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.0f, 0);
    check_texture_color(depth_texture, 0x00000000, 0);

    ID3D11DeviceContext_ClearDepthStencilView(context, dsv, D3D11_CLEAR_DEPTH, 1.0f, 0xff);
    todo_wine check_texture_color(depth_texture, 0x00ffffff, 0);

    ID3D11DeviceContext_ClearDepthStencilView(context, dsv, D3D11_CLEAR_STENCIL, 0.0f, 0xff);
    check_texture_color(depth_texture, 0xffffffff, 0);

    ID3D11Texture2D_Release(depth_texture);
    ID3D11DepthStencilView_Release(dsv);

    ID3D11DeviceContext_Release(context);

    refcount = ID3D11Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_draw_depth_only(void)
{
    ID3D11DepthStencilState *depth_stencil_state;
    D3D11_DEPTH_STENCIL_DESC depth_stencil_desc;
    struct d3d11_test_context test_context;
    ID3D11PixelShader *ps_color, *ps_depth;
    D3D11_TEXTURE2D_DESC texture_desc;
    ID3D11DeviceContext *context;
    ID3D11DepthStencilView *dsv;
    struct texture_readback rb;
    ID3D11Texture2D *texture;
    ID3D11Device *device;
    unsigned int i, j;
    D3D11_VIEWPORT vp;
    struct vec4 depth;
    ID3D11Buffer *cb;
    HRESULT hr;

    static const DWORD ps_color_code[] =
    {
#if 0
        float4 main(float4 position : SV_POSITION) : SV_Target
        {
            return float4(0.0, 1.0, 0.0, 1.0);
        }
#endif
        0x43425844, 0x30240e72, 0x012f250c, 0x8673c6ea, 0x392e4cec, 0x00000001, 0x000000d4, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000000f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x00000038, 0x00000040,
        0x0000000e, 0x03000065, 0x001020f2, 0x00000000, 0x08000036, 0x001020f2, 0x00000000, 0x00004002,
        0x00000000, 0x3f800000, 0x00000000, 0x3f800000, 0x0100003e,
    };
    static const DWORD ps_depth_code[] =
    {
#if 0
        float depth;

        float main() : SV_Depth
        {
            return depth;
        }
#endif
        0x43425844, 0x91af6cd0, 0x7e884502, 0xcede4f54, 0x6f2c9326, 0x00000001, 0x000000b0, 0x00000003,
        0x0000002c, 0x0000003c, 0x00000070, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0xffffffff,
        0x00000e01, 0x445f5653, 0x68747065, 0xababab00, 0x52444853, 0x00000038, 0x00000040, 0x0000000e,
        0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x02000065, 0x0000c001, 0x05000036, 0x0000c001,
        0x0020800a, 0x00000000, 0x00000000, 0x0100003e,
    };

    if (!init_test_context(&test_context, NULL))
        return;

    device = test_context.device;
    context = test_context.immediate_context;

    cb = create_buffer(device, D3D11_BIND_CONSTANT_BUFFER, sizeof(depth), NULL);

    texture_desc.Width = 640;
    texture_desc.Height = 480;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_D32_FLOAT;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    hr = ID3D11Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

    hr = ID3D11Device_CreateDepthStencilView(device, (ID3D11Resource *)texture, NULL, &dsv);
    ok(SUCCEEDED(hr), "Failed to create depth stencil view, hr %#x.\n", hr);

    depth_stencil_desc.DepthEnable = TRUE;
    depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS;
    depth_stencil_desc.StencilEnable = FALSE;

    hr = ID3D11Device_CreateDepthStencilState(device, &depth_stencil_desc, &depth_stencil_state);
    ok(SUCCEEDED(hr), "Failed to create depth stencil state, hr %#x.\n", hr);

    hr = ID3D11Device_CreatePixelShader(device, ps_color_code, sizeof(ps_color_code), NULL, &ps_color);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);
    hr = ID3D11Device_CreatePixelShader(device, ps_depth_code, sizeof(ps_depth_code), NULL, &ps_depth);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    ID3D11DeviceContext_PSSetConstantBuffers(context, 0, 1, &cb);
    ID3D11DeviceContext_PSSetShader(context, ps_color, NULL, 0);
    ID3D11DeviceContext_OMSetRenderTargets(context, 0, NULL, dsv);
    ID3D11DeviceContext_OMSetDepthStencilState(context, depth_stencil_state, 0);

    ID3D11DeviceContext_ClearDepthStencilView(context, dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
    check_texture_float(texture, 1.0f, 1);
    draw_quad(&test_context);
    check_texture_float(texture, 0.0f, 1);

    ID3D11DeviceContext_PSSetShader(context, ps_depth, NULL, 0);

    depth.x = 0.7f;
    ID3D11DeviceContext_UpdateSubresource(context, (ID3D11Resource *)cb, 0, NULL, &depth, 0, 0);
    draw_quad(&test_context);
    check_texture_float(texture, 0.0f, 1);
    ID3D11DeviceContext_ClearDepthStencilView(context, dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
    check_texture_float(texture, 1.0f, 1);
    draw_quad(&test_context);
    check_texture_float(texture, 0.7f, 1);
    depth.x = 0.8f;
    ID3D11DeviceContext_UpdateSubresource(context, (ID3D11Resource *)cb, 0, NULL, &depth, 0, 0);
    draw_quad(&test_context);
    check_texture_float(texture, 0.7f, 1);
    depth.x = 0.5f;
    ID3D11DeviceContext_UpdateSubresource(context, (ID3D11Resource *)cb, 0, NULL, &depth, 0, 0);
    draw_quad(&test_context);
    check_texture_float(texture, 0.5f, 1);

    ID3D11DeviceContext_ClearDepthStencilView(context, dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
    depth.x = 0.1f;
    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            depth.x = 1.0f / 16.0f * (j + 4 * i);
            ID3D11DeviceContext_UpdateSubresource(context, (ID3D11Resource *)cb, 0, NULL, &depth, 0, 0);

            vp.TopLeftX = 160.0f * j;
            vp.TopLeftY = 120.0f * i;
            vp.Width = 160.0f;
            vp.Height = 120.0f;
            vp.MinDepth = 0.0f;
            vp.MaxDepth = 1.0f;
            ID3D11DeviceContext_RSSetViewports(context, 1, &vp);

            draw_quad(&test_context);
        }
    }
    get_texture_readback(texture, 0, &rb);
    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            float obtained_depth, expected_depth;

            obtained_depth = get_readback_float(&rb, 80 + j * 160, 60 + i * 120);
            expected_depth = 1.0f / 16.0f * (j + 4 * i);
            ok(compare_float(obtained_depth, expected_depth, 1),
                    "Got unexpected depth %.8e at (%u, %u), expected %.8e.\n",
                    obtained_depth, j, i, expected_depth);
        }
    }
    release_texture_readback(&rb);

    ID3D11Buffer_Release(cb);
    ID3D11PixelShader_Release(ps_color);
    ID3D11PixelShader_Release(ps_depth);
    ID3D11DepthStencilView_Release(dsv);
    ID3D11DepthStencilState_Release(depth_stencil_state);
    ID3D11Texture2D_Release(texture);
    release_test_context(&test_context);
}

static void test_cb_relative_addressing(void)
{
    struct d3d11_test_context test_context;
    ID3D11Buffer *colors_cb, *index_cb;
    unsigned int i, index[4] = {0};
    ID3D11DeviceContext *context;
    ID3D11PixelShader *ps;
    ID3D11Device *device;
    HRESULT hr;

    static const D3D11_INPUT_ELEMENT_DESC layout_desc[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    static const DWORD vs_code[] =
    {
#if 0
int color_index;

cbuffer colors
{
    float4 colors[8];
};

struct vs_in
{
    float4 position : POSITION;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

vs_out main(const vs_in v)
{
    vs_out o;

    o.position = v.position;
    o.color = colors[color_index];

    return o;
}
#endif
        0x43425844, 0xc2eb30bf, 0x2868c855, 0xaa34b609, 0x1f4957d4, 0x00000001, 0x00000164, 0x00000003,
        0x0000002c, 0x00000060, 0x000000b4, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x49534f50, 0x4e4f4954, 0xababab00,
        0x4e47534f, 0x0000004c, 0x00000002, 0x00000008, 0x00000038, 0x00000000, 0x00000001, 0x00000003,
        0x00000000, 0x0000000f, 0x00000044, 0x00000000, 0x00000000, 0x00000003, 0x00000001, 0x0000000f,
        0x505f5653, 0x5449534f, 0x004e4f49, 0x4f4c4f43, 0xabab0052, 0x58454853, 0x000000a8, 0x00010050,
        0x0000002a, 0x0100086a, 0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x04000859, 0x00208e46,
        0x00000001, 0x00000008, 0x0300005f, 0x001010f2, 0x00000000, 0x04000067, 0x001020f2, 0x00000000,
        0x00000001, 0x03000065, 0x001020f2, 0x00000001, 0x02000068, 0x00000001, 0x05000036, 0x001020f2,
        0x00000000, 0x00101e46, 0x00000000, 0x06000036, 0x00100012, 0x00000000, 0x0020800a, 0x00000000,
        0x00000000, 0x07000036, 0x001020f2, 0x00000001, 0x04208e46, 0x00000001, 0x0010000a, 0x00000000,
        0x0100003e,
    };
    static const DWORD ps_code[] =
    {
#if 0
struct ps_in
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

float4 main(const ps_in v) : SV_TARGET
{
    return v.color;
}
#endif
        0x43425844, 0x1a6def50, 0x9c069300, 0x7cce68f0, 0x621239b9, 0x00000001, 0x000000f8, 0x00000003,
        0x0000002c, 0x00000080, 0x000000b4, 0x4e475349, 0x0000004c, 0x00000002, 0x00000008, 0x00000038,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000000f, 0x00000044, 0x00000000, 0x00000000,
        0x00000003, 0x00000001, 0x00000f0f, 0x505f5653, 0x5449534f, 0x004e4f49, 0x4f4c4f43, 0xabab0052,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x58454853, 0x0000003c, 0x00000050,
        0x0000000f, 0x0100086a, 0x03001062, 0x001010f2, 0x00000001, 0x03000065, 0x001020f2, 0x00000000,
        0x05000036, 0x001020f2, 0x00000000, 0x00101e46, 0x00000001, 0x0100003e,
    };
    static const struct
    {
        struct vec2 position;
    }
    quad[] =
    {
        {{-1.0f, -1.0f}},
        {{-1.0f,  1.0f}},
        {{ 1.0f, -1.0f}},
        {{ 1.0f,  1.0f}},
    };
    static const struct
    {
        float color[4];
    }
    colors[10] =
    {
        {{0.0f, 0.0f, 0.0f, 1.0f}},
        {{0.0f, 0.0f, 1.0f, 0.0f}},
        {{0.0f, 0.0f, 1.0f, 1.0f}},
        {{0.0f, 1.0f, 0.0f, 0.0f}},
        {{0.0f, 1.0f, 0.0f, 1.0f}},
        {{0.0f, 1.0f, 1.0f, 0.0f}},
        {{0.0f, 1.0f, 1.0f, 1.0f}},
        {{1.0f, 0.0f, 0.0f, 0.0f}},
        {{1.0f, 0.0f, 0.0f, 1.0f}},
        {{1.0f, 0.0f, 1.0f, 0.0f}},
    };
    static const struct
    {
        unsigned int index;
        DWORD expected;
    }
    test_data[] =
    {
        {0, 0xff000000},
        {1, 0x00ff0000},
        {2, 0xffff0000},
        {3, 0x0000ff00},
        {4, 0xff00ff00},
        {5, 0x00ffff00},
        {6, 0xffffff00},
        {7, 0x000000ff},

        {8, 0xff0000ff},
        {9, 0x00ff00ff},
    };
    static const float white_color[] = {1.0f, 1.0f, 1.0f, 1.0f};
    static const D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;

    if (!init_test_context(&test_context, &feature_level))
        return;

    device = test_context.device;
    context = test_context.immediate_context;

    hr = ID3D11Device_CreateInputLayout(device, layout_desc, sizeof(layout_desc) / sizeof(*layout_desc),
            vs_code, sizeof(vs_code), &test_context.input_layout);
    ok(SUCCEEDED(hr), "Failed to create input layout, hr %#x.\n", hr);

    test_context.vb = create_buffer(device, D3D11_BIND_VERTEX_BUFFER, sizeof(quad), quad);
    colors_cb = create_buffer(device, D3D11_BIND_CONSTANT_BUFFER, sizeof(colors), &colors);
    index_cb = create_buffer(device, D3D11_BIND_CONSTANT_BUFFER, sizeof(index), NULL);

    hr = ID3D11Device_CreateVertexShader(device, vs_code, sizeof(vs_code), NULL, &test_context.vs);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);
    hr = ID3D11Device_CreatePixelShader(device, ps_code, sizeof(ps_code), NULL, &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    ID3D11DeviceContext_VSSetConstantBuffers(context, 0, 1, &index_cb);
    ID3D11DeviceContext_VSSetConstantBuffers(context, 1, 1, &colors_cb);
    ID3D11DeviceContext_PSSetShader(context, ps, NULL, 0);

    for (i = 0; i < sizeof(test_data) / sizeof(*test_data); ++i)
    {
        ID3D11DeviceContext_ClearRenderTargetView(context, test_context.backbuffer_rtv, white_color);

        index[0] = test_data[i].index;
        ID3D11DeviceContext_UpdateSubresource(context, (ID3D11Resource *)index_cb, 0, NULL, &index, 0, 0);

        draw_quad(&test_context);
        check_texture_color(test_context.backbuffer, test_data[i].expected, 1);
    }

    ID3D11Buffer_Release(index_cb);
    ID3D11Buffer_Release(colors_cb);
    ID3D11PixelShader_Release(ps);

    release_test_context(&test_context);
}

static void test_getdc(void)
{
    struct device_desc device_desc;
    D3D11_TEXTURE2D_DESC desc;
    ID3D11Texture2D *texture;
    IDXGISurface1 *surface1;
    ID3D11Device *device;
    ULONG refcount;
    HRESULT hr;
    HDC dc;

    device_desc.feature_level = NULL;
    device_desc.flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    if (!(device = create_device(&device_desc)))
    {
        skip("Failed to create device.\n");
        return;
    }

    /* Without D3D11_RESOURCE_MISC_GDI_COMPATIBLE. */
    desc.Width = 512;
    desc.Height = 512;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    hr = ID3D11Device_CreateTexture2D(device, &desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

    hr = ID3D11Texture2D_QueryInterface(texture, &IID_IDXGISurface1, (void**)&surface1);
    ok(SUCCEEDED(hr), "Failed to get IDXGISurface1 interface, hr %#x.\n", hr);

    hr = IDXGISurface1_GetDC(surface1, FALSE, &dc);
    todo_wine ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);

    IDXGISurface1_Release(surface1);
    ID3D11Texture2D_Release(texture);

    desc.MiscFlags = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;
    hr = ID3D11Device_CreateTexture2D(device, &desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

    hr = ID3D11Texture2D_QueryInterface(texture, &IID_IDXGISurface1, (void**)&surface1);
    ok(SUCCEEDED(hr), "Failed to get IDXGISurface1 interface, hr %#x.\n", hr);

    hr = IDXGISurface1_ReleaseDC(surface1, NULL);
    todo_wine ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);

    hr = IDXGISurface1_GetDC(surface1, FALSE, &dc);
    todo_wine ok(SUCCEEDED(hr), "Failed to get DC, hr %#x.\n", hr);

    /* One more time. */
    dc = (HDC)0xdeadbeef;
    hr = IDXGISurface1_GetDC(surface1, FALSE, &dc);
    todo_wine ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);
    ok(dc == (HDC)0xdeadbeef, "Got unexpected dc %p.\n", dc);

    hr = IDXGISurface1_ReleaseDC(surface1, NULL);
    todo_wine ok(SUCCEEDED(hr), "Failed to release DC, hr %#x.\n", hr);

    hr = IDXGISurface1_ReleaseDC(surface1, NULL);
    todo_wine ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);

    IDXGISurface1_Release(surface1);
    ID3D11Texture2D_Release(texture);

    refcount = ID3D11Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

START_TEST(d3d11)
{
    test_create_device();
    test_device_interfaces();
    test_get_immediate_context();
    test_create_texture2d();
    test_texture2d_interfaces();
    test_create_texture3d();
    test_texture3d_interfaces();
    test_buffer_interfaces();
    test_create_depthstencil_view();
    test_depthstencil_view_interfaces();
    test_create_rendertarget_view();
    test_create_shader_resource_view();
    test_create_shader();
    test_create_sampler_state();
    test_create_blend_state();
    test_create_depthstencil_state();
    test_create_rasterizer_state();
    test_create_query();
    test_device_removed_reason();
    test_private_data();
    test_blend();
    test_texture();
    test_multiple_render_targets();
    test_render_target_views();
    test_scissor();
    test_il_append_aligned();
    test_fragment_coords();
    test_update_subresource();
    test_copy_subresource_region();
    test_resource_map();
    test_texture_data_init();
    test_check_multisample_quality_levels();
    test_swapchain_flip();
    test_clear_render_target_view();
    test_clear_depth_stencil_view();
    test_draw_depth_only();
    test_cb_relative_addressing();
    test_getdc();
}
