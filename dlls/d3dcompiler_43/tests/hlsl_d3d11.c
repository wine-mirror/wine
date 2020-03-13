/*
 * Copyright (C) 2020 Zebediah Figura for CodeWeavers
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

#include <limits.h>
#include <math.h>

#define COBJMACROS
#include "d3dcompiler.h"
#include "d3d11.h"
#include "wine/test.h"

static pD3DCompile ppD3DCompile;
static HRESULT (WINAPI *pD3DReflect)(const void *data, SIZE_T size, REFIID iid, void **out);

static HRESULT (WINAPI *pD3D11CreateDevice)(IDXGIAdapter *adapter, D3D_DRIVER_TYPE driver_type,
        HMODULE swrast, UINT flags, const D3D_FEATURE_LEVEL *feature_levels, UINT levels,
        UINT sdk_version, ID3D11Device **device_out, D3D_FEATURE_LEVEL *obtained_feature_level,
        ID3D11DeviceContext **immediate_context);

struct vec2
{
    float x, y;
};

struct vec4
{
    float x, y, z, w;
};

#define compile_shader(a, b) compile_shader_(__LINE__, a, b)
static ID3D10Blob *compile_shader_(unsigned int line, const char *source, const char *target)
{
    ID3D10Blob *blob = NULL, *errors = NULL;
    HRESULT hr;

    hr = ppD3DCompile(source, strlen(source), NULL, NULL, NULL, "main", target, 0, 0, &blob, &errors);
    ok_(__FILE__, line)(hr == S_OK, "Failed to compile shader, hr %#x.\n", hr);
    if (errors)
    {
        if (winetest_debug > 1)
            trace_(__FILE__, line)("%s\n", (char *)ID3D10Blob_GetBufferPointer(errors));
        ID3D10Blob_Release(errors);
    }
    return blob;
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

static BOOL compare_vec4(const struct vec4 *vec, float x, float y, float z, float w, unsigned int ulps)
{
    return compare_float(vec->x, x, ulps)
            && compare_float(vec->y, y, ulps)
            && compare_float(vec->z, z, ulps)
            && compare_float(vec->w, w, ulps);
}

struct test_context
{
    ID3D11Device *device;
    HWND window;
    IDXGISwapChain *swapchain;
    ID3D11Texture2D *rt;
    ID3D11RenderTargetView *rtv;
    ID3D11DeviceContext *immediate_context;

    ID3D11InputLayout *input_layout;
    ID3D11VertexShader *vs;
    ID3D11Buffer *vb;
};

static ID3D11Device *create_device(void)
{
    static const D3D_FEATURE_LEVEL feature_level[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    ID3D11Device *device;

    if (SUCCEEDED(pD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
            feature_level, ARRAY_SIZE(feature_level), D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;
    if (SUCCEEDED(pD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_WARP, NULL, 0,
            feature_level, ARRAY_SIZE(feature_level), D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;
    if (SUCCEEDED(pD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_REFERENCE, NULL, 0,
            feature_level, ARRAY_SIZE(feature_level), D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;

    return NULL;
}

static IDXGISwapChain *create_swapchain(ID3D11Device *device, HWND window)
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

    hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &dxgi_desc, &swapchain);
    ok(SUCCEEDED(hr), "Failed to create swapchain, hr %#x.\n", hr);
    IDXGIFactory_Release(factory);

    return swapchain;
}

#define init_test_context(a) init_test_context_(__LINE__, a)
static BOOL init_test_context_(unsigned int line, struct test_context *context)
{
    const D3D11_TEXTURE2D_DESC texture_desc =
    {
        .Width = 640,
        .Height = 480,
        .MipLevels = 1,
        .ArraySize = 1,
        .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
        .SampleDesc.Count = 1,
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = D3D11_BIND_RENDER_TARGET,
    };
    unsigned int rt_width, rt_height;
    D3D11_VIEWPORT vp;
    HRESULT hr;
    RECT rect;

    memset(context, 0, sizeof(*context));

    if (!(context->device = create_device()))
    {
        skip_(__FILE__, line)("Failed to create device.\n");
        return FALSE;
    }

    rt_width = 640;
    rt_height = 480;
    SetRect(&rect, 0, 0, rt_width, rt_height);
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    context->window = CreateWindowA("static", "d3dcompiler_test", WS_OVERLAPPEDWINDOW,
            0, 0, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, NULL, NULL);
    context->swapchain = create_swapchain(context->device, context->window);

    hr = ID3D11Device_CreateTexture2D(context->device, &texture_desc, NULL, &context->rt);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create texture, hr %#x.\n", hr);

    hr = ID3D11Device_CreateRenderTargetView(context->device, (ID3D11Resource *)context->rt, NULL, &context->rtv);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create rendertarget view, hr %#x.\n", hr);

    ID3D11Device_GetImmediateContext(context->device, &context->immediate_context);

    ID3D11DeviceContext_OMSetRenderTargets(context->immediate_context, 1, &context->rtv, NULL);

    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width = rt_width;
    vp.Height = rt_height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    ID3D11DeviceContext_RSSetViewports(context->immediate_context, 1, &vp);

    return TRUE;
}

#define release_test_context(context) release_test_context_(__LINE__, context)
static void release_test_context_(unsigned int line, struct test_context *context)
{
    ULONG ref;

    if (context->input_layout)
        ID3D11InputLayout_Release(context->input_layout);
    if (context->vs)
        ID3D11VertexShader_Release(context->vs);
    if (context->vb)
        ID3D11Buffer_Release(context->vb);

    ID3D11DeviceContext_Release(context->immediate_context);
    ID3D11RenderTargetView_Release(context->rtv);
    ID3D11Texture2D_Release(context->rt);
    IDXGISwapChain_Release(context->swapchain);
    DestroyWindow(context->window);

    ref = ID3D11Device_Release(context->device);
    ok_(__FILE__, line)(!ref, "Device has %u references left.\n", ref);
}

#define create_buffer(a, b, c, d) create_buffer_(__LINE__, a, b, c, d)
static ID3D11Buffer *create_buffer_(unsigned int line, ID3D11Device *device,
        unsigned int bind_flags, unsigned int size, const void *data)
{
    D3D11_SUBRESOURCE_DATA resource_data = {.pSysMem = data};
    D3D11_BUFFER_DESC buffer_desc =
    {
        .ByteWidth = size,
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = bind_flags,
    };
    ID3D11Buffer *buffer;
    HRESULT hr;

    hr = ID3D11Device_CreateBuffer(device, &buffer_desc, data ? &resource_data : NULL, &buffer);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create buffer, hr %#x.\n", hr);
    return buffer;
}

#define draw_quad(context, ps_code) draw_quad_(__LINE__, context, ps_code)
static void draw_quad_(unsigned int line, struct test_context *context, ID3D10Blob *ps_code)
{
    static const D3D11_INPUT_ELEMENT_DESC default_layout_desc[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    static const char vs_source[] =
        "float4 main(float4 position : POSITION) : SV_POSITION\n"
        "{\n"
        "    return position;\n"
        "}";

    static const struct vec2 quad[] =
    {
        {-1.0f, -1.0f},
        {-1.0f,  1.0f},
        { 1.0f, -1.0f},
        { 1.0f,  1.0f},
    };

    ID3D11Device *device = context->device;
    unsigned int stride, offset;
    ID3D11PixelShader *ps;
    HRESULT hr;

    if (!context->vs)
    {
        ID3D10Blob *vs_code = compile_shader_(line, vs_source, "vs_4_0");

        hr = ID3D11Device_CreateInputLayout(device, default_layout_desc, ARRAY_SIZE(default_layout_desc),
                ID3D10Blob_GetBufferPointer(vs_code), ID3D10Blob_GetBufferSize(vs_code), &context->input_layout);
        ok_(__FILE__, line)(hr == S_OK, "Failed to create input layout, hr %#x.\n", hr);

        hr = ID3D11Device_CreateVertexShader(device, ID3D10Blob_GetBufferPointer(vs_code),
                ID3D10Blob_GetBufferSize(vs_code), NULL, &context->vs);
        ok_(__FILE__, line)(hr == S_OK, "Failed to create vertex shader, hr %#x.\n", hr);
    }

    if (!context->vb)
        context->vb = create_buffer_(line, device, D3D11_BIND_VERTEX_BUFFER, sizeof(quad), quad);

    hr = ID3D11Device_CreatePixelShader(device, ID3D10Blob_GetBufferPointer(ps_code),
            ID3D10Blob_GetBufferSize(ps_code), NULL, &ps);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create pixel shader, hr %#x.\n", hr);

    ID3D11DeviceContext_IASetInputLayout(context->immediate_context, context->input_layout);
    ID3D11DeviceContext_IASetPrimitiveTopology(context->immediate_context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    stride = sizeof(*quad);
    offset = 0;
    ID3D11DeviceContext_IASetVertexBuffers(context->immediate_context, 0, 1, &context->vb, &stride, &offset);
    ID3D11DeviceContext_VSSetShader(context->immediate_context, context->vs, NULL, 0);
    ID3D11DeviceContext_PSSetShader(context->immediate_context, ps, NULL, 0);

    ID3D11DeviceContext_Draw(context->immediate_context, 4, 0);

    ID3D11PixelShader_Release(ps);
}

struct readback
{
    ID3D11Resource *resource;
    D3D11_MAPPED_SUBRESOURCE map_desc;
};

static void init_readback(struct test_context *context, struct readback *rb)
{
    D3D11_TEXTURE2D_DESC texture_desc;
    HRESULT hr;

    ID3D11Texture2D_GetDesc(context->rt, &texture_desc);
    texture_desc.Usage = D3D11_USAGE_STAGING;
    texture_desc.BindFlags = 0;
    texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    texture_desc.MiscFlags = 0;
    hr = ID3D11Device_CreateTexture2D(context->device, &texture_desc, NULL, (ID3D11Texture2D **)&rb->resource);
    ok(hr == S_OK, "Failed to create texture, hr %#x.\n", hr);

    ID3D11DeviceContext_CopyResource(context->immediate_context, rb->resource, (ID3D11Resource *)context->rt);
    hr = ID3D11DeviceContext_Map(context->immediate_context, rb->resource, 0, D3D11_MAP_READ, 0, &rb->map_desc);
    ok(hr == S_OK, "Failed to map texture, hr %#x.\n", hr);
}

static void release_readback(struct test_context *context, struct readback *rb)
{
    ID3D11DeviceContext_Unmap(context->immediate_context, rb->resource, 0);
    ID3D11Resource_Release(rb->resource);
}

static const struct vec4 *get_readback_vec4(struct readback *rb, unsigned int x, unsigned int y)
{
    return (struct vec4 *)((BYTE *)rb->map_desc.pData + y * rb->map_desc.RowPitch) + x;
}

static struct vec4 get_color_vec4(struct test_context *context, unsigned int x, unsigned int y)
{
    struct readback rb;
    struct vec4 ret;

    init_readback(context, &rb);
    ret = *get_readback_vec4(&rb, x, y);
    release_readback(context, &rb);
    return ret;
}

static void test_swizzle(void)
{
    static const struct vec4 uniform = {0.0303f, 0.0f, 0.0f, 0.0202f};
    struct test_context test_context;
    ID3D10Blob *ps_code = NULL;
    ID3D11Buffer *cb;
    struct vec4 v;

    static const char ps_source[] =
        "uniform float4 color;\n"
        "float4 main() : SV_TARGET\n"
        "{\n"
        "    float4 ret = color;\n"
        "    ret.gb = ret.ra;\n"
        "    ret.ra = float2(0.0101, 0.0404);\n"
        "    return ret;\n"
        "}";

    if (!init_test_context(&test_context))
        return;

    todo_wine ps_code = compile_shader(ps_source, "ps_4_0");
    if (ps_code)
    {
        cb = create_buffer(test_context.device, D3D11_BIND_CONSTANT_BUFFER, sizeof(uniform), &uniform);
        ID3D11DeviceContext_PSSetConstantBuffers(test_context.immediate_context, 0, 1, &cb);
        draw_quad(&test_context, ps_code);

        v = get_color_vec4(&test_context, 0, 0);
        ok(compare_vec4(&v, 0.0101f, 0.0303f, 0.0202f, 0.0404f, 0),
                "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v.x, v.y, v.z, v.w);

        ID3D11Buffer_Release(cb);
        ID3D10Blob_Release(ps_code);
    }
    release_test_context(&test_context);
}

static void test_math(void)
{
    static const float uniforms[8] = {2.5f, 0.3f, 0.2f, 0.7f, 0.1f, 1.5f};
    struct test_context test_context;
    ID3D10Blob *ps_code = NULL;
    ID3D11Buffer *cb;
    struct vec4 v;

    static const char ps_source[] =
        "float4 main(uniform float u, uniform float v, uniform float w, uniform float x,\n"
        "            uniform float y, uniform float z) : SV_TARGET\n"
        "{\n"
        "    return float4(x * y - z / w + --u / -v,\n"
        "            z * x / y + w / -v,\n"
        "            u + v - w,\n"
        "            x / y / w);\n"
        "}";

    if (!init_test_context(&test_context))
        return;

    todo_wine ps_code = compile_shader(ps_source, "ps_4_0");
    if (ps_code)
    {
        cb = create_buffer(test_context.device, D3D11_BIND_CONSTANT_BUFFER, sizeof(uniforms), uniforms);
        ID3D11DeviceContext_PSSetConstantBuffers(test_context.immediate_context, 0, 1, &cb);
        draw_quad(&test_context, ps_code);

        v = get_color_vec4(&test_context, 0, 0);
        ok(compare_vec4(&v, -12.43f, 9.833333f, 1.6f, 35.0f, 1),
                "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v.x, v.y, v.z, v.w);

        ID3D11Buffer_Release(cb);
        ID3D10Blob_Release(ps_code);
    }
    release_test_context(&test_context);
}

static void test_conditionals(void)
{
    struct test_context test_context;
    ID3D10Blob *ps_code = NULL;
    const struct vec4 *v;
    struct readback rb;
    unsigned int i;

    static const char ps_source[] =
        "float4 main(float4 pos : SV_POSITION) : SV_TARGET\n"
        "{\n"
        "    if(pos.x > 200.0)\n"
        "        return float4(0.1, 0.2, 0.3, 0.4);\n"
        "    else\n"
        "        return float4(0.9, 0.8, 0.7, 0.6);\n"
        "}";

    if (!init_test_context(&test_context))
        return;

    todo_wine ps_code = compile_shader(ps_source, "ps_4_0");
    if (ps_code)
    {
        draw_quad(&test_context, ps_code);
        init_readback(&test_context, &rb);

        for (i = 0; i < 200; i += 40)
        {
            v = get_readback_vec4(&rb, i, 0);
            ok(compare_vec4(v, 0.9f, 0.8f, 0.7f, 0.6f, 0),
                    "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v->x, v->y, v->z, v->w);
        }

        for (i = 240; i < 640; i += 40)
        {
            v = get_readback_vec4(&rb, i, 0);
            ok(compare_vec4(v, 0.1f, 0.2f, 0.3f, 0.4f, 0),
                    "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v->x, v->y, v->z, v->w);
        }

        release_readback(&test_context, &rb);
        ID3D10Blob_Release(ps_code);
    }
    release_test_context(&test_context);
}

static void test_trig(void)
{
    struct test_context test_context;
    ID3D10Blob *ps_code = NULL;
    const struct vec4 *v;
    struct readback rb;
    unsigned int i;

    static const char ps_source[] =
        "float4 main(float4 pos : SV_POSITION) : SV_TARGET\n"
        "{\n"
        "    return float4(sin(pos.x - 0.5), cos(pos.x - 0.5), 0, 0);\n"
        "}";

    if (!init_test_context(&test_context))
        return;

    todo_wine ps_code = compile_shader(ps_source, "ps_4_0");
    if (ps_code)
    {
        draw_quad(&test_context, ps_code);
        init_readback(&test_context, &rb);

        for (i = 0; i < 640; i += 20)
        {
            v = get_readback_vec4(&rb, i, 0);
            ok(compare_vec4(v, sinf(i), cosf(i), 0.0f, 0.0f, 8192),
                    "Test %u: Got {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                    i, v->x, v->y, v->z, v->w, sinf(i), cos(i), 0.0f, 0.0f);
        }

        release_readback(&test_context, &rb);
        ID3D10Blob_Release(ps_code);
    }
    release_test_context(&test_context);
}

static void check_type_desc(const char *prefix, const D3D11_SHADER_TYPE_DESC *type,
        const D3D11_SHADER_TYPE_DESC *expect)
{
    ok(type->Class == expect->Class, "%s: got class %#x.\n", prefix, type->Class);
    ok(type->Type == expect->Type, "%s: got type %#x.\n", prefix, type->Type);
    ok(type->Rows == expect->Rows, "%s: got %u rows.\n", prefix, type->Rows);
    ok(type->Columns == expect->Columns, "%s: got %u columns.\n", prefix, type->Columns);
    ok(type->Elements == expect->Elements, "%s: got %u elements.\n", prefix, type->Elements);
    ok(type->Members == expect->Members, "%s: got %u members.\n", prefix, type->Members);
    ok(type->Offset == expect->Offset, "%s: got %u members.\n", prefix, type->Members);
    ok(!strcmp(type->Name, expect->Name), "%s: got name %s.\n", prefix, debugstr_a(type->Name));
}

static void test_reflection(void)
{
    ID3D11ShaderReflectionConstantBuffer *cbuffer;
    ID3D11ShaderReflectionType *type, *field;
    D3D11_SHADER_BUFFER_DESC buffer_desc;
    ID3D11ShaderReflectionVariable *var;
    D3D11_SHADER_VARIABLE_DESC var_desc;
    ID3D11ShaderReflection *reflection;
    D3D11_SHADER_TYPE_DESC type_desc;
    ID3D10Blob *vs_code = NULL;
    unsigned int i, j, k;
    ULONG refcount;
    HRESULT hr;

    static const char vs_source[] =
        "typedef uint uint_t;\n"
        "cbuffer b1\n"
        "{\n"
        "    float a;\n"
        "    float2 b;\n"
        "    float4 c;\n"
        "    float d;\n"
        "    struct\n"
        "    {\n"
        "        float4 a;\n"
        "        float b;\n"
        "        float c;\n"
        "    } s;\n"
        "    float g;\n"
        "    float h[2];\n"
        "    int i;\n"
        "    uint_t j;\n"
        "    float3x1 k;\n"
        "    row_major float3x1 l;\n"
        "};\n"
        "\n"
        "float m;\n"
        "\n"
        "float4 main(uniform float4 n) : SV_POSITION\n"
        "{\n"
        "    return l._31 + m + n;\n"
        "}";

    struct shader_variable
    {
        D3D11_SHADER_VARIABLE_DESC var_desc;
        D3D11_SHADER_TYPE_DESC type_desc;
    };

    static const D3D11_SHADER_TYPE_DESC field_types[] =
    {
        {D3D_SVC_VECTOR, D3D_SVT_FLOAT, 1, 4, 0, 0, 0, "float4"},
        {D3D_SVC_SCALAR, D3D_SVT_FLOAT, 1, 1, 0, 0, 16, "float"},
        {D3D_SVC_SCALAR, D3D_SVT_FLOAT, 1, 1, 0, 0, 20, "float"},
    };

    static const struct shader_variable globals_vars =
        {{"m", 0, 4, D3D_SVF_USED}, {D3D_SVC_SCALAR, D3D_SVT_FLOAT, 1, 1, 0, 0, 0, "float"}};
    static const struct shader_variable params_vars =
        {{"n", 0, 16, D3D_SVF_USED}, {D3D_SVC_VECTOR, D3D_SVT_FLOAT, 1, 4, 0, 0, 0, "float4"}};
    static const struct shader_variable buffer_vars[] =
    {
        {{"a", 0, 4}, {D3D_SVC_SCALAR, D3D_SVT_FLOAT, 1, 1, 0, 0, 0, "float"}},
        {{"b", 4, 8}, {D3D_SVC_VECTOR, D3D_SVT_FLOAT, 1, 2, 0, 0, 0, "float2"}},
        {{"c", 16, 16}, {D3D_SVC_VECTOR, D3D_SVT_FLOAT, 1, 4, 0, 0, 0, "float4"}},
        {{"d", 32, 4}, {D3D_SVC_SCALAR, D3D_SVT_FLOAT, 1, 1, 0, 0, 0, "float"}},
        {{"s", 48, 24}, {D3D_SVC_STRUCT, D3D_SVT_VOID, 1, 6, 0, 3, 0, "<unnamed>"}},
        {{"g", 72, 4}, {D3D_SVC_SCALAR, D3D_SVT_FLOAT, 1, 1, 0, 0, 0, "float"}},
        {{"h", 80, 20}, {D3D_SVC_SCALAR, D3D_SVT_FLOAT, 1, 1, 2, 0, 0, "float"}},
        {{"i", 100, 4}, {D3D_SVC_SCALAR, D3D_SVT_INT, 1, 1, 0, 0, 0, "int"}},
        {{"j", 104, 4}, {D3D_SVC_SCALAR, D3D_SVT_UINT, 1, 1, 0, 0, 0, "uint_t"}},
        {{"k", 112, 12}, {D3D_SVC_MATRIX_COLUMNS, D3D_SVT_FLOAT, 3, 1, 0, 0, 0, "float3x1"}},
        {{"l", 128, 36, D3D_SVF_USED}, {D3D_SVC_MATRIX_ROWS, D3D_SVT_FLOAT, 3, 1, 0, 0, 0, "float3x1"}},
    };

    static const struct
    {
        D3D11_SHADER_BUFFER_DESC desc;
        const struct shader_variable *vars;
    }
    vs_buffers[] =
    {
        {{"$Globals", D3D_CT_CBUFFER, 1, 16}, &globals_vars},
        {{"$Params", D3D_CT_CBUFFER, 1, 16}, &params_vars},
        {{"b1", D3D_CT_CBUFFER, ARRAY_SIZE(buffer_vars), 176}, buffer_vars},
    };

    todo_wine vs_code = compile_shader(vs_source, "vs_5_0");
    if (!vs_code)
        return;

    hr = pD3DReflect(ID3D10Blob_GetBufferPointer(vs_code), ID3D10Blob_GetBufferSize(vs_code),
            &IID_ID3D11ShaderReflection, (void **)&reflection);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(vs_buffers); ++i)
    {
        cbuffer = reflection->lpVtbl->GetConstantBufferByIndex(reflection, i);
        hr = cbuffer->lpVtbl->GetDesc(cbuffer, &buffer_desc);
        ok(hr == S_OK, "Test %u: got hr %#x.\n", i, hr);
        ok(!strcmp(buffer_desc.Name, vs_buffers[i].desc.Name),
                "Test %u: got name %s.\n", i, debugstr_a(buffer_desc.Name));
        ok(buffer_desc.Type == vs_buffers[i].desc.Type, "Test %u: got type %#x.\n", i, buffer_desc.Type);
        ok(buffer_desc.Variables == vs_buffers[i].desc.Variables,
                "Test %u: got %u variables.\n", i, buffer_desc.Variables);
        ok(buffer_desc.Size == vs_buffers[i].desc.Size, "Test %u: got size %u.\n", i, buffer_desc.Size);
        ok(buffer_desc.uFlags == vs_buffers[i].desc.uFlags, "Test %u: got flags %#x.\n", i, buffer_desc.uFlags);

        for (j = 0; j < buffer_desc.Variables; ++j)
        {
            const struct shader_variable *expect = &vs_buffers[i].vars[j];
            char prefix[40];

            var = cbuffer->lpVtbl->GetVariableByIndex(cbuffer, j);
            hr = var->lpVtbl->GetDesc(var, &var_desc);
            ok(hr == S_OK, "Test %u, %u: got hr %#x.\n", i, j, hr);
            ok(!strcmp(var_desc.Name, expect->var_desc.Name),
                    "Test %u, %u: got name %s.\n", i, j, debugstr_a(var_desc.Name));
            ok(var_desc.StartOffset == expect->var_desc.StartOffset, "Test %u, %u: got offset %u.\n",
                    i, j, var_desc.StartOffset);
            ok(var_desc.Size == expect->var_desc.Size, "Test %u, %u: got size %u.\n", i, j, var_desc.Size);
            ok(var_desc.uFlags == expect->var_desc.uFlags, "Test %u, %u: got flags %#x.\n", i, j, var_desc.uFlags);
            ok(!var_desc.DefaultValue, "Test %u, %u: got default value %p.\n", i, j, var_desc.DefaultValue);

            type = var->lpVtbl->GetType(var);
            hr = type->lpVtbl->GetDesc(type, &type_desc);
            ok(hr == S_OK, "Test %u, %u: got hr %#x.\n", i, j, hr);
            sprintf(prefix, "Test %u, %u", i, j);
            check_type_desc(prefix, &type_desc, &expect->type_desc);

            if (!strcmp(type_desc.Name, "<unnamed>"))
            {
                for (k = 0; k < ARRAY_SIZE(field_types); ++k)
                {
                    field = type->lpVtbl->GetMemberTypeByIndex(type, k);
                    hr = field->lpVtbl->GetDesc(field, &type_desc);
                    ok(hr == S_OK, "Test %u, %u, %u: got hr %#x.\n", i, j, k, hr);
                    sprintf(prefix, "Test %u, %u, %u", i, j, k);
                    check_type_desc(prefix, &type_desc, &field_types[k]);
                }
            }
        }
    }

    ID3D10Blob_Release(vs_code);
    refcount = reflection->lpVtbl->Release(reflection);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
}

static BOOL load_d3dcompiler(void)
{
    HMODULE module;

#if D3D_COMPILER_VERSION == 47
    if (!(module = LoadLibraryA("d3dcompiler_47.dll"))) return FALSE;
#else
    if (!(module = LoadLibraryA("d3dcompiler_43.dll"))) return FALSE;
#endif

    ppD3DCompile = (void *)GetProcAddress(module, "D3DCompile");
    pD3DReflect = (void *)GetProcAddress(module, "D3DReflect");
    return TRUE;
}

START_TEST(hlsl_d3d11)
{
    HMODULE mod;

    if (!load_d3dcompiler())
    {
        win_skip("Could not load DLL.\n");
        return;
    }

    test_reflection();

    if (!(mod = LoadLibraryA("d3d11.dll")))
    {
        skip("Direct3D 11 is not available.\n");
        return;
    }
    pD3D11CreateDevice = (void *)GetProcAddress(mod, "D3D11CreateDevice");

    test_swizzle();
    test_math();
    test_conditionals();
    test_trig();
}
