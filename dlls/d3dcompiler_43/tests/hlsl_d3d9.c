/*
 * Copyright (C) 2010 Travis Athougies
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
#define COBJMACROS
#include "wine/test.h"
#include "d3dx9.h"
#include "d3dcompiler.h"

#include <math.h>

static pD3DCompile ppD3DCompile;

static HRESULT (WINAPI *pD3DXGetShaderConstantTable)(const DWORD *byte_code, ID3DXConstantTable **constant_table);

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
    ok_(__FILE__, line)(hr == D3D_OK, "Failed to compile shader, hr %#x.\n", hr);
    if (errors)
    {
        if (winetest_debug > 1)
            trace_(__FILE__, line)("%s\n", (char *)ID3D10Blob_GetBufferPointer(errors));
        ID3D10Blob_Release(errors);
    }
    return blob;
}

static IDirect3DDevice9 *create_device(HWND window)
{
    D3DPRESENT_PARAMETERS present_parameters =
    {
        .Windowed = TRUE,
        .hDeviceWindow = window,
        .SwapEffect = D3DSWAPEFFECT_DISCARD,
        .BackBufferWidth = 640,
        .BackBufferHeight = 480,
        .BackBufferFormat = D3DFMT_A8R8G8B8,
    };
    IDirect3DDevice9 *device;
    IDirect3DSurface9 *rt;
    IDirect3D9 *d3d;
    D3DCAPS9 caps;
    HRESULT hr;

    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
            D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, &device);
    IDirect3D9_Release(d3d);
    if (FAILED(hr))
    {
        skip("Failed to create a 3D device, hr %#x.\n", hr);
        return NULL;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(hr == D3D_OK, "Failed to get device caps, hr %#x.\n", hr);
    if (caps.PixelShaderVersion < D3DPS_VERSION(2, 0) || caps.VertexShaderVersion < D3DVS_VERSION(2, 0))
    {
        skip("No shader model 2 support.\n");
        IDirect3DDevice9_Release(device);
        return NULL;
    }

    if (FAILED(hr = IDirect3DDevice9_CreateRenderTarget(device, 640, 480, D3DFMT_A32B32G32R32F,
            D3DMULTISAMPLE_NONE, 0, FALSE, &rt, NULL)))
    {
        skip("Failed to create an A32B32G32R32F surface, hr %#x.\n", hr);
        IDirect3DDevice9_Release(device);
        return NULL;
    }
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderTarget(device, 0, rt);
    ok(hr == D3D_OK, "Failed to set render target, hr %#x.\n", hr);
    IDirect3DSurface9_Release(rt);

    return device;
}

struct test_context
{
    IDirect3DDevice9 *device;
    HWND window;
};

#define init_test_context(a) init_test_context_(__LINE__, a)
static BOOL init_test_context_(unsigned int line, struct test_context *context)
{
    RECT rect = {0, 0, 640, 480};

    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    context->window = CreateWindowA("static", "d3dcompiler_test", WS_OVERLAPPEDWINDOW,
            0, 0, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, NULL, NULL);
    ok(!!context->window, "Failed to create a window.\n");

    if (!(context->device = create_device(context->window)))
    {
        DestroyWindow(context->window);
        return FALSE;
    }

    return TRUE;
}

#define release_test_context(context) release_test_context_(__LINE__, context)
static void release_test_context_(unsigned int line, struct test_context *context)
{
    ULONG ref = IDirect3DDevice9_Release(context->device);
    ok_(__FILE__, line)(!ref, "Device has %u references left.\n", ref);
    DestroyWindow(context->window);
}

#define draw_quad(device, ps_code) draw_quad_(__LINE__, device, ps_code)
static void draw_quad_(unsigned int line, IDirect3DDevice9 *device, ID3D10Blob *ps_code)
{
    IDirect3DVertexDeclaration9 *vertex_declaration;
    IDirect3DVertexShader9 *vs;
    IDirect3DPixelShader9 *ps;
    ID3D10Blob *vs_code;
    HRESULT hr;

    static const D3DVERTEXELEMENT9 decl_elements[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 8, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };

    static const struct
    {
        struct vec2 position;
        struct vec2 t0;
    }
    quad[] =
    {
        {{-1.0f, -1.0f}, {0.0f, 1.0f}},
        {{-1.0f,  1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f}, {1.0f, 1.0f}},
        {{ 1.0f,  1.0f}, {1.0f, 0.0f}},
    };

    static const char vs_source[] =
        "float4 main(float4 pos : POSITION, inout float2 texcoord : TEXCOORD0) : POSITION\n"
        "{\n"
        "   return pos;\n"
        "}";

    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements, &vertex_declaration);
    ok_(__FILE__, line)(hr == D3D_OK, "Failed to create vertex declaration, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetVertexDeclaration(device, vertex_declaration);
    ok_(__FILE__, line)(hr == D3D_OK, "Failed to set vertex declaration, hr %#x.\n", hr);

    vs_code = compile_shader(vs_source, "vs_2_0");

    hr = IDirect3DDevice9_CreateVertexShader(device, ID3D10Blob_GetBufferPointer(vs_code), &vs);
    ok_(__FILE__, line)(hr == D3D_OK, "Failed to create vertex shader, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetVertexShader(device, vs);
    ok_(__FILE__, line)(hr == D3D_OK, "Failed to set vertex shader, hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreatePixelShader(device, ID3D10Blob_GetBufferPointer(ps_code), &ps);
    ok_(__FILE__, line)(hr == D3D_OK, "Failed to create pixel shader, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetPixelShader(device, ps);
    ok_(__FILE__, line)(hr == D3D_OK, "Failed to set pixel shader, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok_(__FILE__, line)(hr == D3D_OK, "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok_(__FILE__, line)(hr == D3D_OK, "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice9_EndScene(device);
    ok_(__FILE__, line)(hr == D3D_OK, "Failed to draw, hr %#x.\n", hr);

    IDirect3DVertexDeclaration9_Release(vertex_declaration);
    IDirect3DVertexShader9_Release(vs);
    IDirect3DPixelShader9_Release(ps);
    ID3D10Blob_Release(vs_code);
}

struct readback
{
    IDirect3DSurface9 *surface;
    D3DLOCKED_RECT rect;
};

static void init_readback(IDirect3DDevice9 *device, struct readback *rb)
{
    IDirect3DSurface9 *rt;
    D3DSURFACE_DESC desc;
    HRESULT hr;

    hr = IDirect3DDevice9Ex_GetRenderTarget(device, 0, &rt);
    ok(hr == D3D_OK, "Failed to get render target, hr %#x.\n", hr);

    hr = IDirect3DSurface9_GetDesc(rt, &desc);
    ok(hr == D3D_OK, "Failed to get surface desc, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurface(device, desc.Width, desc.Height,
            desc.Format, D3DPOOL_SYSTEMMEM, &rb->surface, NULL);
    ok(hr == D3D_OK, "Failed to create surface, hr %#x.\n", hr);

    hr = IDirect3DDevice9Ex_GetRenderTargetData(device, rt, rb->surface);
    ok(hr == D3D_OK, "Failed to get render target data, hr %#x.\n", hr);

    hr = IDirect3DSurface9_LockRect(rb->surface, &rb->rect, NULL, D3DLOCK_READONLY);
    ok(hr == D3D_OK, "Failed to lock surface, hr %#x.\n", hr);

    IDirect3DSurface9_Release(rt);
}

static const struct vec4 *get_readback_vec4(const struct readback *rb, unsigned int x, unsigned int y)
{
    return (struct vec4 *)((BYTE *)rb->rect.pBits + y * rb->rect.Pitch + x * sizeof(struct vec4));
}

static void release_readback(struct readback *rb)
{
    IDirect3DSurface9_UnlockRect(rb->surface);
    IDirect3DSurface9_Release(rb->surface);
}

static struct vec4 get_color_vec4(IDirect3DDevice9 *device, unsigned int x, unsigned int y)
{
    struct readback rb;
    struct vec4 ret;

    init_readback(device, &rb);
    ret = *get_readback_vec4(&rb, x, y);
    release_readback(&rb);

    return ret;
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

static void test_swizzle(void)
{
    static const D3DXVECTOR4 color = {0.0303f, 0.0f, 0.0f, 0.0202f};
    struct test_context test_context;
    ID3DXConstantTable *constants;
    ID3D10Blob *ps_code = NULL;
    IDirect3DDevice9 *device;
    unsigned int i;
    struct vec4 v;
    HRESULT hr;

    static const struct
    {
        const char *source;
        struct vec4 color;
    }
    tests[] =
    {
        {
            "uniform float4 color;\n"
            "float4 main() : COLOR\n"
            "{\n"
            "    float4 ret = color;\n"
            "    ret.gb = ret.ra;\n"
            "    ret.ra = float2(0.0101, 0.0404);\n"
            "    return ret;\n"
            "}",
            {0.0101f, 0.0303f, 0.0202f, 0.0404f}
        },
        {
            "float4 main() : COLOR\n"
            "{\n"
            "    float4 ret = float4(0.1, 0.2, 0.3, 0.4);\n"
            "    ret.wyz.yx = float2(0.5, 0.6).yx;\n"
            "    return ret;\n"
            "}",
            {0.1f, 0.6f, 0.3f, 0.5f}
        },
        {
            "float4 main() : COLOR\n"
            "{\n"
            "    float4 ret;\n"
            "    ret.zwyx = float4(0.1, 0.2, 0.3, 0.4);\n"
            "    return ret;\n"
            "}",
            {0.4f, 0.3f, 0.1f, 0.2f}
        },
        {
            "float4 main() : COLOR\n"
            "{\n"
            "    float4 ret;\n"
            "    ret.yw.y = 0.1;\n"
            "    ret.xzy.yz.y.x = 0.2;\n"
            "    ret.yzwx.yzwx.wz.y = 0.3;\n"
            "    ret.zxy.xyz.zxy.xy.y = 0.4;\n"
            "    return ret;\n"
            "}",
            {0.3f, 0.2f, 0.4f, 0.1f}
        },
        {
            "float4 main() : COLOR\n"
            "{\n"
            "    float4 ret;\n"
            "    ret.yxz.yx = float2(0.1, 0.2);\n"
            "    ret.w.x = 0.3;\n"
            "    ret.wzyx.zyx.yx.x = 0.4;\n"
            "    return ret;\n"
            "}",
            {0.1f, 0.2f, 0.4f, 0.3f}
        },
        {
            "float4 main() : COLOR\n"
            "{\n"
            "    float4 ret = float4(0.1, 0.2, 0.3, 0.4).ywxz.zyyz;\n"
            "    return ret;\n"
            "}",
            {0.1f, 0.4f, 0.4f, 0.1f}
        },
        {
            "float4 main() : COLOR\n"
            "{\n"
            "    float4 ret = float4(0.1, 0.2, 0.3, 0.4);\n"
            "    ret.yxwz = ret;\n"
            "    ret = ret.wyzx;\n"
            "    return ret;\n"
            "}",
            {0.3f, 0.1f, 0.4f, 0.2f}
        },
        {
            "float4 main() : COLOR\n"
            "{\n"
            "    float4 ret;\n"
            "    ret.xyzw.xyzw = float4(0.1, 0.2, 0.3, 0.4);\n"
            "    return ret;\n"
            "}",
            {0.1f, 0.2f, 0.3f, 0.4f}
        },
    };

    if (!init_test_context(&test_context))
        return;
    device = test_context.device;

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        todo_wine ps_code = compile_shader(tests[i].source, "ps_2_0");
        if (ps_code)
        {
            if (i == 0)
            {
                hr = pD3DXGetShaderConstantTable(ID3D10Blob_GetBufferPointer(ps_code), &constants);
                ok(hr == D3D_OK, "Failed to get constant table, hr %#x.\n", hr);
                hr = ID3DXConstantTable_SetVector(constants, device, "color", &color);
                ok(hr == D3D_OK, "Failed to set constant, hr %#x.\n", hr);
                ID3DXConstantTable_Release(constants);
            }
            draw_quad(device, ps_code);

            v = get_color_vec4(device, 0, 0);
            ok(compare_vec4(&v, tests[i].color.x, tests[i].color.y, tests[i].color.z, tests[i].color.w, 0),
                    "Test %u: Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", i, v.x, v.y, v.z, v.w);

            ID3D10Blob_Release(ps_code);
        }
    }

    release_test_context(&test_context);
}

static void test_math(void)
{
    struct test_context test_context;
    ID3DXConstantTable *constants;
    ID3D10Blob *ps_code = NULL;
    IDirect3DDevice9 *device;
    struct vec4 v;
    HRESULT hr;

    static const char ps_source[] =
        "float4 main(uniform float u, uniform float v, uniform float w, uniform float x,\n"
        "            uniform float y, uniform float z): COLOR\n"
        "{\n"
        "    return float4(x * y - z / w + --u / -v,\n"
        "            z * x / y + w / -v,\n"
        "            u + v - w,\n"
        "            x / y / w);\n"
        "}";

    if (!init_test_context(&test_context))
        return;
    device = test_context.device;

    todo_wine ps_code = compile_shader(ps_source, "ps_2_0");
    if (ps_code)
    {
        hr = pD3DXGetShaderConstantTable(ID3D10Blob_GetBufferPointer(ps_code), &constants);
        ok(hr == D3D_OK, "Failed to get constant table, hr %#x.\n", hr);
        hr = ID3DXConstantTable_SetFloat(constants, device, "$u", 2.5f);
        ok(hr == D3D_OK, "Failed to set constant, hr %#x.\n", hr);
        hr = ID3DXConstantTable_SetFloat(constants, device, "$v", 0.3f);
        ok(hr == D3D_OK, "Failed to set constant, hr %#x.\n", hr);
        hr = ID3DXConstantTable_SetFloat(constants, device, "$w", 0.2f);
        ok(hr == D3D_OK, "Failed to set constant, hr %#x.\n", hr);
        hr = ID3DXConstantTable_SetFloat(constants, device, "$x", 0.7f);
        ok(hr == D3D_OK, "Failed to set constant, hr %#x.\n", hr);
        hr = ID3DXConstantTable_SetFloat(constants, device, "$y", 0.1f);
        ok(hr == D3D_OK, "Failed to set constant, hr %#x.\n", hr);
        hr = ID3DXConstantTable_SetFloat(constants, device, "$z", 1.5f);
        ok(hr == D3D_OK, "Failed to set constant, hr %#x.\n", hr);
        ID3DXConstantTable_Release(constants);

        draw_quad(device, ps_code);

        v = get_color_vec4(device, 0, 0);
        ok(compare_vec4(&v, -12.43f, 9.833333f, 1.6f, 35.0f, 1),
                "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v.x, v.y, v.z, v.w);

        ID3D10Blob_Release(ps_code);
    }
    release_test_context(&test_context);
}

static void test_conditionals(void)
{
    struct test_context test_context;
    ID3D10Blob *ps_code = NULL;
    IDirect3DDevice9 *device;
    const struct vec4 *v;
    struct readback rb;
    unsigned int i;

    static const char ps_if_source[] =
        "float4 main(float2 pos : TEXCOORD0) : COLOR\n"
        "{\n"
        "    if((pos.x * 640.0) > 200.0)\n"
        "        return float4(0.1, 0.2, 0.3, 0.4);\n"
        "    else\n"
        "        return float4(0.9, 0.8, 0.7, 0.6);\n"
        "}";

    static const char ps_ternary_source[] =
        "float4 main(float2 pos : TEXCOORD0) : COLOR\n"
        "{\n"
        "    return (pos.x < 0.5 ? float4(0.5, 0.25, 0.5, 0.75) : float4(0.6, 0.8, 0.1, 0.2));\n"
        "}";

    if (!init_test_context(&test_context))
        return;
    device = test_context.device;

    todo_wine ps_code = compile_shader(ps_if_source, "ps_2_0");
    if (ps_code)
    {
        draw_quad(device, ps_code);
        init_readback(device, &rb);

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

        release_readback(&rb);
        ID3D10Blob_Release(ps_code);
    }

    todo_wine ps_code = compile_shader(ps_ternary_source, "ps_2_0");
    if (ps_code)
    {
        draw_quad(device, ps_code);
        init_readback(device, &rb);

        for (i = 0; i < 320; i += 40)
        {
            v = get_readback_vec4(&rb, i, 0);
            ok(compare_vec4(v, 0.5f, 0.25f, 0.5f, 0.75f, 0),
                    "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v->x, v->y, v->z, v->w);
        }

        for (i = 360; i < 640; i += 40)
        {
            v = get_readback_vec4(&rb, i, 0);
            ok(compare_vec4(v, 0.6f, 0.8f, 0.1f, 0.2f, 0),
                    "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v->x, v->y, v->z, v->w);
        }

        release_readback(&rb);
        ID3D10Blob_Release(ps_code);
    }

    release_test_context(&test_context);
}

static void test_float_vectors(void)
{
    struct test_context test_context;
    ID3DXConstantTable *constants;
    ID3D10Blob *ps_code = NULL;
    IDirect3DDevice9 *device;
    struct vec4 v;
    HRESULT hr;

    static const char ps_indexing_source[] =
        "float4 main() : COLOR\n"
        "{\n"
        "    float4 color;\n"
        "    color[0] = 0.020;\n"
        "    color[1] = 0.245;\n"
        "    color[2] = 0.351;\n"
        "    color[3] = 1.0;\n"
        "    return color;\n"
        "}";

    /* A uniform index is used so that the compiler can't optimize. */
    static const char ps_uniform_indexing_source[] =
        "uniform int i;\n"
        "float4 main() : COLOR\n"
        "{\n"
        "    float4 color = float4(0.5, 0.4, 0.3, 0.2);\n"
        "    color.g = color[i];\n"
        "    color.b = 0.8;\n"
        "    return color;\n"
        "}";

    if (!init_test_context(&test_context))
        return;
    device = test_context.device;

    todo_wine ps_code = compile_shader(ps_indexing_source, "ps_2_0");
    if (ps_code)
    {
        draw_quad(device, ps_code);

        v = get_color_vec4(device, 0, 0);
        ok(compare_vec4(&v, 0.02f, 0.245f, 0.351f, 1.0f, 0),
                "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v.x, v.y, v.z, v.w);

        ID3D10Blob_Release(ps_code);
    }

    todo_wine ps_code = compile_shader(ps_uniform_indexing_source, "ps_2_0");
    if (ps_code)
    {
        hr = pD3DXGetShaderConstantTable(ID3D10Blob_GetBufferPointer(ps_code), &constants);
        ok(hr == D3D_OK, "Failed to get constants, hr %#x.\n", hr);
        hr = ID3DXConstantTable_SetInt(constants, device, "i", 2);
        ok(hr == D3D_OK, "Failed to set constant, hr %#x.\n", hr);
        ID3DXConstantTable_Release(constants);
        draw_quad(device, ps_code);

        v = get_color_vec4(device, 0, 0);
        ok(compare_vec4(&v, 0.5f, 0.3f, 0.8f, 0.2f, 0),
                "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v.x, v.y, v.z, v.w);

        ID3D10Blob_Release(ps_code);
    }

    release_test_context(&test_context);
}

static void test_trig(void)
{
    struct test_context test_context;
    ID3D10Blob *ps_code = NULL;
    IDirect3DDevice9 *device;
    const struct vec4 *v;
    struct readback rb;
    unsigned int i;

    static const char ps_source[] =
        "float4 main(float x : TEXCOORD0) : COLOR\n"
        "{\n"
        "    const float pi2 = 6.2831853;\n"
        "    float calcd_sin = (sin(x * pi2) + 1)/2;\n"
        "    float calcd_cos = (cos(x * pi2) + 1)/2;\n"
        "    return float4(calcd_sin, calcd_cos, 0, 0);\n"
        "}";

    if (!init_test_context(&test_context))
        return;
    device = test_context.device;

    todo_wine ps_code = compile_shader(ps_source, "ps_2_0");
    if (ps_code)
    {
        draw_quad(device, ps_code);
        init_readback(device, &rb);

        for (i = 0; i < 32; ++i)
        {
            float expect_x = (sinf(i * 2 * M_PI / 32) + 1.0f) / 2.0f;
            float expect_y = (cosf(i * 2 * M_PI / 32) + 1.0f) / 2.0f;
            v = get_readback_vec4(&rb, i * 640 / 32, 0);
            ok(compare_vec4(v, expect_x, expect_y, 0.0f, 0.0f, 4096),
                    "Test %u: Got {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                    i, v->x, v->y, v->z, v->w, expect_x, expect_y, 0.0f, 0.0f);
        }

        release_readback(&rb);
        ID3D10Blob_Release(ps_code);
    }
    release_test_context(&test_context);
}

static void test_comma(void)
{
    struct test_context test_context;
    ID3D10Blob *ps_code = NULL;
    struct vec4 v;

    static const char ps_source[] =
        "float4 main(float x: TEXCOORD0): COLOR\n"
        "{\n"
        "    float4 ret;\n"
        "    return (ret = float4(0.1, 0.2, 0.3, 0.4)), ret + 0.5;\n"
        "}";

    if (!init_test_context(&test_context))
        return;

    todo_wine ps_code = compile_shader(ps_source, "ps_2_0");
    if (ps_code)
    {
        draw_quad(test_context.device, ps_code);

        v = get_color_vec4(test_context.device, 0, 0);
        ok(compare_vec4(&v, 0.6f, 0.7f, 0.8f, 0.9f, 0),
                "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v.x, v.y, v.z, v.w);

        ID3D10Blob_Release(ps_code);
    }
    release_test_context(&test_context);
}

static void test_return(void)
{
    struct test_context test_context;
    ID3D10Blob *ps_code = NULL;
    struct vec4 v;

    static const char *void_source =
        "void main(float x : TEXCOORD0, out float4 ret : COLOR)\n"
        "{\n"
        "    ret = float4(0.1, 0.2, 0.3, 0.4);\n"
        "    return;\n"
        "    ret = float4(0.5, 0.6, 0.7, 0.8);\n"
        "}";

    static const char *implicit_conversion_source =
        "float4 main(float x : TEXCOORD0) : COLOR\n"
        "{\n"
        "    return float2x2(0.4, 0.3, 0.2, 0.1);\n"
        "}";

    if (!init_test_context(&test_context))
        return;

    todo_wine ps_code = compile_shader(void_source, "ps_2_0");
    if (ps_code)
    {
        draw_quad(test_context.device, ps_code);

        v = get_color_vec4(test_context.device, 0, 0);
        ok(compare_vec4(&v, 0.1f, 0.2f, 0.3f, 0.4f, 0),
                "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v.x, v.y, v.z, v.w);

        ID3D10Blob_Release(ps_code);
    }

    todo_wine ps_code = compile_shader(implicit_conversion_source, "ps_2_0");
    if (ps_code)
    {
        draw_quad(test_context.device, ps_code);

        v = get_color_vec4(test_context.device, 0, 0);
        ok(compare_vec4(&v, 0.4f, 0.3f, 0.2f, 0.1f, 0),
                "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v.x, v.y, v.z, v.w);

        ID3D10Blob_Release(ps_code);
    }

    release_test_context(&test_context);
}

static void test_array_dimensions(void)
{
    struct test_context test_context;
    ID3D10Blob *ps_code = NULL;
    struct vec4 v;

    static const char shader[] =
        "float4 main(float x : TEXCOORD0) : COLOR\n"
        "{\n"
        "    const int dim = 4;\n"
        "    float a[2 * 2] = {0.1, 0.2, 0.3, 0.4};\n"
        "    float b[4.1] = a;\n"
        "    float c[dim] = b;\n"
        "    float d[true] = {c[0]};\n"
        "    float e[65536];\n"
        "    return float4(d[0], c[0], c[1], c[3]);\n"
        "}";

    if (!init_test_context(&test_context))
        return;

    todo_wine ps_code = compile_shader(shader, "ps_2_0");
    if (ps_code)
    {
        draw_quad(test_context.device, ps_code);

        v = get_color_vec4(test_context.device, 0, 0);
        ok(compare_vec4(&v, 0.1f, 0.1f, 0.2f, 0.4f, 0),
                "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v.x, v.y, v.z, v.w);

        ID3D10Blob_Release(ps_code);
    }

    release_test_context(&test_context);
}

static void check_constant_desc(const char *prefix, const D3DXCONSTANT_DESC *desc,
        const D3DXCONSTANT_DESC *expect, BOOL nonzero_defaultvalue)
{
    ok(!strcmp(desc->Name, expect->Name), "%s: got Name %s.\n", prefix, debugstr_a(desc->Name));
    ok(desc->RegisterSet == expect->RegisterSet, "%s: got RegisterSet %#x.\n", prefix, desc->RegisterSet);
    ok(desc->RegisterCount == expect->RegisterCount, "%s: got RegisterCount %u.\n", prefix, desc->RegisterCount);
    ok(desc->Class == expect->Class, "%s: got Class %#x.\n", prefix, desc->Class);
    ok(desc->Type == expect->Type, "%s: got Type %#x.\n", prefix, desc->Type);
    ok(desc->Rows == expect->Rows, "%s: got Rows %u.\n", prefix, desc->Rows);
    ok(desc->Columns == expect->Columns, "%s: got Columns %u.\n", prefix, desc->Columns);
    ok(desc->Elements == expect->Elements, "%s: got Elements %u.\n", prefix, desc->Elements);
    ok(desc->StructMembers == expect->StructMembers, "%s: got StructMembers %u.\n", prefix, desc->StructMembers);
    ok(desc->Bytes == expect->Bytes, "%s: got Bytes %u.\n", prefix, desc->Bytes);
    ok(!!desc->DefaultValue == nonzero_defaultvalue, "%s: got DefaultValue %p.\n", prefix, desc->DefaultValue);
}

static void test_constant_table(void)
{
    static const char *source =
        "uniform float4 a;\n"
        "uniform float b;\n"
        "uniform float unused;\n"
        "uniform float3x1 c;\n"
        "uniform row_major float3x1 d;\n"
        "uniform uint e;\n"
        "uniform struct\n"
        "{\n"
        "    float2x2 a;\n"
        "    float b;\n"
        "    float c;\n"
        "} f;\n"
        "uniform float g[5];\n"
        "float4 main(uniform float4 h) : COLOR\n"
        "{\n"
        "    return a + b + c._31 + d._31 + f.c + g[e] + h;\n"
        "}";

    D3DXCONSTANTTABLE_DESC table_desc;
    ID3DXConstantTable *constants;
    ID3D10Blob *ps_code = NULL;
    D3DXHANDLE handle, field;
    D3DXCONSTANT_DESC desc;
    unsigned int i, j;
    HRESULT hr;
    UINT count;

    static const D3DXCONSTANT_DESC expect_constants[] =
    {
        {"$h", D3DXRS_FLOAT4, 0, 1, D3DXPC_VECTOR, D3DXPT_FLOAT, 1, 4, 1, 0, 16},
        {"a", D3DXRS_FLOAT4, 0, 1, D3DXPC_VECTOR, D3DXPT_FLOAT, 1, 4, 1, 0, 16},
        {"b", D3DXRS_FLOAT4, 0, 1, D3DXPC_SCALAR, D3DXPT_FLOAT, 1, 1, 1, 0, 4},
        {"c", D3DXRS_FLOAT4, 0, 1, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 3, 1, 1, 0, 12},
        {"d", D3DXRS_FLOAT4, 0, 3, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 3, 1, 1, 0, 12},
        {"e", D3DXRS_FLOAT4, 0, 1, D3DXPC_SCALAR, D3DXPT_INT, 1, 1, 1, 0, 4},
        {"f", D3DXRS_FLOAT4, 0, 4, D3DXPC_STRUCT, D3DXPT_VOID, 1, 6, 1, 3, 24},
        {"g", D3DXRS_FLOAT4, 0, 5, D3DXPC_SCALAR, D3DXPT_FLOAT, 1, 1, 5, 0, 20},
    };

    static const D3DXCONSTANT_DESC expect_fields[] =
    {
        {"a", D3DXRS_FLOAT4, 0, 2, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 2, 2, 1, 0, 16},
        {"b", D3DXRS_FLOAT4, 0, 1, D3DXPC_SCALAR, D3DXPT_FLOAT, 1, 1, 1, 0, 4},
        {"c", D3DXRS_FLOAT4, 0, 1, D3DXPC_SCALAR, D3DXPT_FLOAT, 1, 1, 1, 0, 4},
    };

    todo_wine ps_code = compile_shader(source, "ps_2_0");
    if (!ps_code)
        return;

    hr = pD3DXGetShaderConstantTable(ID3D10Blob_GetBufferPointer(ps_code), &constants);
    ok(hr == D3D_OK, "Got hr %#x.\n", hr);

    hr = ID3DXConstantTable_GetDesc(constants, &table_desc);
    ok(hr == D3D_OK, "Got hr %#x.\n", hr);
    ok(table_desc.Version == D3DPS_VERSION(2, 0), "Got Version %#x.\n", table_desc.Version);
    ok(table_desc.Constants == 8, "Got %u constants.\n", table_desc.Constants);

    for (i = 0; i < table_desc.Constants; ++i)
    {
        char prefix[30];

        handle = ID3DXConstantTable_GetConstant(constants, NULL, i);
        ok(!!handle, "Failed to get constant.\n");
        memset(&desc, 0xcc, sizeof(desc));
        count = 1;
        hr = ID3DXConstantTable_GetConstantDesc(constants, handle, &desc, &count);
        ok(hr == D3D_OK, "Got hr %#x.\n", hr);
        ok(count == 1, "Got count %u.\n", count);
        sprintf(prefix, "Test %u", i);
        check_constant_desc(prefix, &desc, &expect_constants[i], FALSE);

        if (!strcmp(desc.Name, "f"))
        {
            for (j = 0; j < ARRAY_SIZE(expect_fields); ++j)
            {
                field = ID3DXConstantTable_GetConstant(constants, handle, j);
                ok(!!field, "Failed to get constant.\n");
                memset(&desc, 0xcc, sizeof(desc));
                count = 1;
                hr = ID3DXConstantTable_GetConstantDesc(constants, field, &desc, &count);
                ok(hr == D3D_OK, "Got hr %#x.\n", hr);
                ok(count == 1, "Got count %u.\n", count);
                sprintf(prefix, "Test %u, %u", i, j);
                check_constant_desc(prefix, &desc, &expect_fields[j], !!j);
            }
        }
    }

    ID3DXConstantTable_Release(constants);
    ID3D10Blob_Release(ps_code);
}

static void test_fail(void)
{
    static const char *tests[] =
    {
        /* 0 */
        "float4 test() : SV_TARGET\n"
        "{\n"
        "   return y;\n"
        "}",

        "float4 test() : SV_TARGET\n"
        "{\n"
        "  float4 x = float4(0, 0, 0, 0);\n"
        "  x.xzzx = float4(1, 2, 3, 4);\n"
        "  return x;\n"
        "}",

        "float4 test(float2 pos: TEXCOORD0) : SV_TARGET\n"
        "{\n"
        "  float4 x = pos;\n"
        "  return x;\n"
        "}",

        "float4 test(float2 pos, TEXCOORD0) ; SV_TARGET\n"
        "{\n"
        "  pos = float4 x;\n"
        "  mul(float4(5, 4, 3, 2), mvp) = x;\n"
        "  return float4;\n"
        "}",

        "float4 563r(float2 45s: TEXCOORD0) : SV_TARGET\n"
        "{\n"
        "  float2 x = 45s;\n"
        "  return float4(x.x, x.y, 0, 0);\n"
        "}",

        /* 5 */
        "float4 test() : SV_TARGET\n"
        "{\n"
        "   struct { int b,c; } x = {0};\n"
        "   return y;\n"
        "}",

        "float4 test() : SV_TARGET\n"
        "{\n"
        "   struct {} x = {};\n"
        "   return y;\n"
        "}",

        "float4 test(float2 pos : TEXCOORD0) : SV_TARGET\n"
        "{\n"
        "    return;\n"
        "}",

        "void test(float2 pos : TEXCOORD0)\n"
        "{\n"
        "    return pos;\n"
        "}",

        "float4 test(float2 pos : TEXCOORD0) : SV_TARGET\n"
        "{\n"
        "    return pos;\n"
        "}",

        /* 10 */
        "float4 test(float2 pos: TEXCOORD0) : SV_TARGET\n"
        "{\n"
        "    float a[0];\n"
        "    return float4(0, 0, 0, 0);\n"
        "}",

        "float4 test(float2 pos: TEXCOORD0) : SV_TARGET\n"
        "{\n"
        "    float a[65537];\n"
        "    return float4(0, 0, 0, 0);\n"
        "}",

        "float4 test(float2 pos: TEXCOORD0) : SV_TARGET\n"
        "{\n"
        "    int x;\n"
        "    float a[(x = 2)];\n"
        "    return float4(0, 0, 0, 0);\n"
        "}",

        "uniform float4 test() : SV_TARGET\n"
        "{\n"
        "    return float4(0, 0, 0, 0);\n"
        "}",
    };

    static const char *targets[] = {"ps_2_0", "ps_3_0", "ps_4_0"};

    ID3D10Blob *compiled, *errors;
    unsigned int i, j;
    HRESULT hr;

    for (j = 0; j < ARRAY_SIZE(targets); ++j)
    {
        for (i = 0; i < ARRAY_SIZE(tests); ++i)
        {
            compiled = errors = NULL;
            hr = ppD3DCompile(tests[i], strlen(tests[i]), NULL, NULL, NULL, "test", targets[j], 0, 0, &compiled, &errors);
            todo_wine ok(hr == E_FAIL, "Test %u, target %s, got unexpected hr %#x.\n", i, targets[j], hr);
            ok(!!errors, "Test %u, target %s, expected non-NULL error blob.\n", i, targets[j]);
            ok(!compiled, "Test %u, target %s, expected no compiled shader blob.\n", i, targets[j]);
            ID3D10Blob_Release(errors);
        }
    }
}

static BOOL load_d3dcompiler(void)
{
    HMODULE module;

#if D3D_COMPILER_VERSION == 47
    if (!(module = LoadLibraryA("d3dcompiler_47.dll"))) return FALSE;
#else
    if (!(module = LoadLibraryA("d3dcompiler_43.dll"))) return FALSE;
#endif

    ppD3DCompile = (void*)GetProcAddress(module, "D3DCompile");
    return TRUE;
}

START_TEST(hlsl_d3d9)
{
    HMODULE mod;

    if (!load_d3dcompiler())
    {
        win_skip("Could not load DLL.\n");
        return;
    }

    if (!(mod = LoadLibraryA("d3dx9_36.dll")))
    {
        win_skip("Failed to load d3dx9_36.dll.\n");
        return;
    }
    pD3DXGetShaderConstantTable = (void *)GetProcAddress(mod, "D3DXGetShaderConstantTable");

    test_swizzle();
    test_math();
    test_conditionals();
    test_float_vectors();
    test_trig();
    test_comma();
    test_return();
    test_array_dimensions();
    test_constant_table();
    test_fail();
}
