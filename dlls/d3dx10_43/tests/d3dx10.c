/*
 * Copyright 2016 Nikolay Sivov for CodeWeavers
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
#include "initguid.h"
#include "d3d10_1.h"
#include "d3dx10.h"
#include "wine/test.h"

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

static ID3D10Device *create_device(void)
{
    ID3D10Device *device;
    HMODULE d3d10_mod = LoadLibraryA("d3d10.dll");
    HRESULT (WINAPI *pD3D10CreateDevice)(IDXGIAdapter *, D3D10_DRIVER_TYPE, HMODULE, UINT, UINT, ID3D10Device **);

    if (!d3d10_mod)
    {
        win_skip("d3d10.dll not present\n");
        return NULL;
    }

    pD3D10CreateDevice = (void *)GetProcAddress(d3d10_mod, "D3D10CreateDevice");
    if (SUCCEEDED(pD3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &device)))
        return device;
    if (SUCCEEDED(pD3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_WARP, NULL, 0, D3D10_SDK_VERSION, &device)))
        return device;
    if (SUCCEEDED(pD3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_REFERENCE, NULL, 0, D3D10_SDK_VERSION, &device)))
        return device;

    return NULL;
}

static void test_D3DX10UnsetAllDeviceObjects(void)
{
    static const D3D10_INPUT_ELEMENT_DESC layout_desc[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0},
    };
#if 0
float4 main(float4 pos : POSITION) : POSITION
{
    return pos;
}
#endif
    static const DWORD simple_vs[] =
    {
        0x43425844, 0x66689e7c, 0x643f0971, 0xb7f67ff4, 0xabc48688, 0x00000001, 0x000000d4, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x49534f50, 0x4e4f4954, 0xababab00,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x49534f50, 0x4e4f4954, 0xababab00, 0x52444853, 0x00000038, 0x00010040,
        0x0000000e, 0x0300005f, 0x001010f2, 0x00000000, 0x03000065, 0x001020f2, 0x00000000, 0x05000036,
        0x001020f2, 0x00000000, 0x00101e46, 0x00000000, 0x0100003e,
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
    static const DWORD simple_gs[] =
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

#if 0
float4 main(float4 color : COLOR) : SV_TARGET
{
    return color;
}
#endif
    static const DWORD simple_ps[] =
    {
        0x43425844, 0x08c2b568, 0x17d33120, 0xb7d82948, 0x13a570fb, 0x00000001, 0x000000d0, 0x00000003,
        0x0000002c, 0x0000005c, 0x00000090, 0x4e475349, 0x00000028, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x4f4c4f43, 0xabab0052, 0x4e47534f,
        0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
        0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x00000038, 0x00000040, 0x0000000e,
        0x03001062, 0x001010f2, 0x00000000, 0x03000065, 0x001020f2, 0x00000000, 0x05000036, 0x001020f2,
        0x00000000, 0x00101e46, 0x00000000, 0x0100003e,
    };

    D3D10_VIEWPORT tmp_viewport[D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    ID3D10ShaderResourceView *tmp_srv[D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    ID3D10ShaderResourceView *srv[D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    ID3D10RenderTargetView *tmp_rtv[D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT];
    RECT tmp_rect[D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    ID3D10SamplerState *tmp_sampler[D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT];
    ID3D10RenderTargetView *rtv[D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT];
    ID3D10Texture2D *rt_texture[D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT];
    ID3D10Buffer *cb[D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    ID3D10Buffer *tmp_buffer[D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    ID3D10SamplerState *sampler[D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT];
    ID3D10Buffer *buffer[D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    unsigned int offset[D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    unsigned int stride[D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    ID3D10Buffer *so_buffer[D3D10_SO_BUFFER_SLOT_COUNT];
    ID3D10InputLayout *tmp_input_layout, *input_layout;
    ID3D10DepthStencilState *tmp_ds_state, *ds_state;
    ID3D10BlendState *tmp_blend_state, *blend_state;
    ID3D10RasterizerState *tmp_rs_state, *rs_state;
    ID3D10Predicate *tmp_predicate, *predicate;
    D3D10_SHADER_RESOURCE_VIEW_DESC srv_desc;
    ID3D10DepthStencilView *tmp_dsv, *dsv;
    D3D10_PRIMITIVE_TOPOLOGY topology;
    D3D10_TEXTURE2D_DESC texture_desc;
    ID3D10GeometryShader *tmp_gs, *gs;
    D3D10_DEPTH_STENCIL_DESC ds_desc;
    ID3D10VertexShader *tmp_vs, *vs;
    D3D10_SAMPLER_DESC sampler_desc;
    D3D10_QUERY_DESC predicate_desc;
    ID3D10PixelShader *tmp_ps, *ps;
    D3D10_RASTERIZER_DESC rs_desc;
    D3D10_BUFFER_DESC buffer_desc;
    D3D10_BLEND_DESC blend_desc;
    ID3D10Texture2D *ds_texture;
    unsigned int sample_mask;
    unsigned int stencil_ref;
    unsigned int count, i;
    float blend_factor[4];
    ID3D10Device *device;
    BOOL predicate_value;
    DXGI_FORMAT format;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    buffer_desc.ByteWidth = 1024;
    buffer_desc.Usage = D3D10_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.MiscFlags = 0;

    for (i = 0; i < D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++i)
    {
        hr = ID3D10Device_CreateBuffer(device, &buffer_desc, NULL, &cb[i]);
        ok(SUCCEEDED(hr), "Failed to create buffer, hr %#x.\n", hr);
    }

    buffer_desc.BindFlags = D3D10_BIND_VERTEX_BUFFER | D3D10_BIND_INDEX_BUFFER | D3D10_BIND_SHADER_RESOURCE;

    for (i = 0; i < D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        hr = ID3D10Device_CreateBuffer(device, &buffer_desc, NULL, &buffer[i]);
        ok(SUCCEEDED(hr), "Failed to create buffer, hr %#x.\n", hr);

        stride[i] = (i + 1) * 4;
        offset[i] = (i + 1) * 16;
    }

    buffer_desc.BindFlags = D3D10_BIND_STREAM_OUTPUT;

    for (i = 0; i < D3D10_SO_BUFFER_SLOT_COUNT; ++i)
    {
        hr = ID3D10Device_CreateBuffer(device, &buffer_desc, NULL, &so_buffer[i]);
        ok(SUCCEEDED(hr), "Failed to create buffer, hr %#x.\n", hr);
    }

    srv_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srv_desc.ViewDimension = D3D10_SRV_DIMENSION_BUFFER;
    U(srv_desc).Buffer.ElementOffset = 0;
    U(srv_desc).Buffer.ElementWidth = 64;

    for (i = 0; i < D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        hr = ID3D10Device_CreateShaderResourceView(device,
                (ID3D10Resource *)buffer[i % D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT], &srv_desc, &srv[i]);
        ok(SUCCEEDED(hr), "Failed to create shader resource view, hr %#x.\n", hr);
    }

    sampler_desc.Filter = D3D10_FILTER_MIN_MAG_MIP_LINEAR;
    sampler_desc.AddressU = D3D10_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D10_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D10_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.MipLODBias = 0.0f;
    sampler_desc.MaxAnisotropy = 16;
    sampler_desc.ComparisonFunc = D3D10_COMPARISON_NEVER;
    sampler_desc.BorderColor[0] = 0.0f;
    sampler_desc.BorderColor[1] = 0.0f;
    sampler_desc.BorderColor[2] = 0.0f;
    sampler_desc.BorderColor[3] = 0.0f;
    sampler_desc.MinLOD = 0.0f;
    sampler_desc.MaxLOD = 16.0f;

    for (i = 0; i < D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT; ++i)
    {
        sampler_desc.MinLOD = (float)i;

        hr = ID3D10Device_CreateSamplerState(device, &sampler_desc, &sampler[i]);
        ok(SUCCEEDED(hr), "Failed to create sampler state, hr %#x.\n", hr);
    }

    hr = ID3D10Device_CreateVertexShader(device, simple_vs, sizeof(simple_vs), &vs);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);

    hr = ID3D10Device_CreateGeometryShader(device, simple_gs, sizeof(simple_gs), &gs);
    ok(SUCCEEDED(hr), "Failed to create geometry shader, hr %#x.\n", hr);

    hr = ID3D10Device_CreatePixelShader(device, simple_ps, sizeof(simple_ps), &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    hr = ID3D10Device_CreateInputLayout(device, layout_desc, ARRAY_SIZE(layout_desc), simple_vs,
            sizeof(simple_vs), &input_layout);
    ok(SUCCEEDED(hr), "Failed to create input layout, hr %#x.\n", hr);

    blend_desc.AlphaToCoverageEnable = FALSE;
    blend_desc.BlendEnable[0] = FALSE;
    blend_desc.BlendEnable[1] = FALSE;
    blend_desc.BlendEnable[2] = FALSE;
    blend_desc.BlendEnable[3] = FALSE;
    blend_desc.BlendEnable[4] = FALSE;
    blend_desc.BlendEnable[5] = FALSE;
    blend_desc.BlendEnable[6] = FALSE;
    blend_desc.BlendEnable[7] = FALSE;
    blend_desc.SrcBlend = D3D10_BLEND_ONE;
    blend_desc.DestBlend = D3D10_BLEND_ZERO;
    blend_desc.BlendOp = D3D10_BLEND_OP_ADD;
    blend_desc.SrcBlendAlpha = D3D10_BLEND_ONE;
    blend_desc.DestBlendAlpha = D3D10_BLEND_ZERO;
    blend_desc.BlendOpAlpha = D3D10_BLEND_OP_ADD;
    blend_desc.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_ALL;
    blend_desc.RenderTargetWriteMask[1] = D3D10_COLOR_WRITE_ENABLE_ALL;
    blend_desc.RenderTargetWriteMask[2] = D3D10_COLOR_WRITE_ENABLE_ALL;
    blend_desc.RenderTargetWriteMask[3] = D3D10_COLOR_WRITE_ENABLE_ALL;
    blend_desc.RenderTargetWriteMask[4] = D3D10_COLOR_WRITE_ENABLE_ALL;
    blend_desc.RenderTargetWriteMask[5] = D3D10_COLOR_WRITE_ENABLE_ALL;
    blend_desc.RenderTargetWriteMask[6] = D3D10_COLOR_WRITE_ENABLE_ALL;
    blend_desc.RenderTargetWriteMask[7] = D3D10_COLOR_WRITE_ENABLE_ALL;

    hr = ID3D10Device_CreateBlendState(device, &blend_desc, &blend_state);
    ok(SUCCEEDED(hr), "Failed to create blend state, hr %#x.\n", hr);

    ds_desc.DepthEnable = TRUE;
    ds_desc.DepthWriteMask = D3D10_DEPTH_WRITE_MASK_ALL;
    ds_desc.DepthFunc = D3D10_COMPARISON_LESS;
    ds_desc.StencilEnable = FALSE;
    ds_desc.StencilReadMask = D3D10_DEFAULT_STENCIL_READ_MASK;
    ds_desc.StencilWriteMask = D3D10_DEFAULT_STENCIL_WRITE_MASK;
    ds_desc.FrontFace.StencilFailOp = D3D10_STENCIL_OP_KEEP;
    ds_desc.FrontFace.StencilDepthFailOp = D3D10_STENCIL_OP_KEEP;
    ds_desc.FrontFace.StencilPassOp = D3D10_STENCIL_OP_KEEP;
    ds_desc.FrontFace.StencilFunc = D3D10_COMPARISON_ALWAYS;
    ds_desc.BackFace.StencilFailOp = D3D10_STENCIL_OP_KEEP;
    ds_desc.BackFace.StencilDepthFailOp = D3D10_STENCIL_OP_KEEP;
    ds_desc.BackFace.StencilPassOp = D3D10_STENCIL_OP_KEEP;
    ds_desc.BackFace.StencilFunc = D3D10_COMPARISON_ALWAYS;

    hr = ID3D10Device_CreateDepthStencilState(device, &ds_desc, &ds_state);
    ok(SUCCEEDED(hr), "Failed to create depthstencil state, hr %#x.\n", hr);

    texture_desc.Width = 512;
    texture_desc.Height = 512;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_RENDER_TARGET;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    for (i = 0; i < D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &rt_texture[i]);
        ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    }

    texture_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    texture_desc.BindFlags = D3D10_BIND_DEPTH_STENCIL;

    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &ds_texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

    for (i = 0; i < D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        hr = ID3D10Device_CreateRenderTargetView(device, (ID3D10Resource *)rt_texture[i], NULL, &rtv[i]);
        ok(SUCCEEDED(hr), "Failed to create rendertarget view, hr %#x.\n", hr);
    }

    hr = ID3D10Device_CreateDepthStencilView(device, (ID3D10Resource *)ds_texture, NULL, &dsv);
    ok(SUCCEEDED(hr), "Failed to create depthstencil view, hr %#x.\n", hr);

    for (i = 0; i < D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE; ++i)
    {
        SetRect(&tmp_rect[i], i, i * 2, i + 1, (i + 1) * 2);

        tmp_viewport[i].TopLeftX = i * 3;
        tmp_viewport[i].TopLeftY = i * 4;
        tmp_viewport[i].Width = 3;
        tmp_viewport[i].Height = 4;
        tmp_viewport[i].MinDepth = i * 0.01f;
        tmp_viewport[i].MaxDepth = (i + 1) * 0.01f;
    }

    rs_desc.FillMode = D3D10_FILL_SOLID;
    rs_desc.CullMode = D3D10_CULL_BACK;
    rs_desc.FrontCounterClockwise = FALSE;
    rs_desc.DepthBias = 0;
    rs_desc.DepthBiasClamp = 0.0f;
    rs_desc.SlopeScaledDepthBias = 0.0f;
    rs_desc.DepthClipEnable = TRUE;
    rs_desc.ScissorEnable = FALSE;
    rs_desc.MultisampleEnable = FALSE;
    rs_desc.AntialiasedLineEnable = FALSE;

    hr = ID3D10Device_CreateRasterizerState(device, &rs_desc, &rs_state);
    ok(SUCCEEDED(hr), "Failed to create rasterizer state, hr %#x.\n", hr);

    predicate_desc.Query = D3D10_QUERY_OCCLUSION_PREDICATE;
    predicate_desc.MiscFlags = 0;

    hr = ID3D10Device_CreatePredicate(device, &predicate_desc, &predicate);
    ok(SUCCEEDED(hr), "Failed to create predicate, hr %#x.\n", hr);

    ID3D10Device_VSSetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, cb);
    ID3D10Device_VSSetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srv);
    ID3D10Device_VSSetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, sampler);
    ID3D10Device_VSSetShader(device, vs);

    ID3D10Device_GSSetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, cb);
    ID3D10Device_GSSetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srv);
    ID3D10Device_GSSetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, sampler);
    ID3D10Device_GSSetShader(device, gs);

    ID3D10Device_PSSetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, cb);
    ID3D10Device_PSSetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srv);
    ID3D10Device_PSSetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, sampler);
    ID3D10Device_PSSetShader(device, ps);

    ID3D10Device_IASetVertexBuffers(device, 0, D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, buffer, stride, offset);
    ID3D10Device_IASetIndexBuffer(device, buffer[0], DXGI_FORMAT_R32_UINT, offset[0]);
    ID3D10Device_IASetInputLayout(device, input_layout);
    ID3D10Device_IASetPrimitiveTopology(device, D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    blend_factor[0] = 0.1f;
    blend_factor[1] = 0.2f;
    blend_factor[2] = 0.3f;
    blend_factor[3] = 0.4f;
    ID3D10Device_OMSetBlendState(device, blend_state, blend_factor, 0xff00ff00);
    ID3D10Device_OMSetDepthStencilState(device, ds_state, 3);
    ID3D10Device_OMSetRenderTargets(device, D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT, rtv, dsv);

    ID3D10Device_RSSetScissorRects(device, D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, tmp_rect);
    ID3D10Device_RSSetViewports(device, D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, tmp_viewport);
    ID3D10Device_RSSetState(device, rs_state);

    ID3D10Device_SOSetTargets(device, D3D10_SO_BUFFER_SLOT_COUNT, so_buffer, offset);

    ID3D10Device_SetPredication(device, predicate, TRUE);

    hr = D3DX10UnsetAllDeviceObjects(device);
    ok(SUCCEEDED(hr), "D3DX10UnsetAllDeviceObjects() failed, %#x.\n", hr);

    ID3D10Device_VSGetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, tmp_buffer);
    for (i = 0; i < D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++i)
    {
        ok(!tmp_buffer[i], "Got unexpected constant buffer %p in slot %u.\n", tmp_buffer[i], i);
    }
    ID3D10Device_VSGetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, tmp_srv);
    for (i = 0; i < D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        ok(!tmp_srv[i], "Got unexpected shader resource view %p in slot %u.\n", tmp_srv[i], i);
    }
    ID3D10Device_VSGetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, tmp_sampler);
    for (i = 0; i < D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT; ++i)
    {
        ok(!tmp_sampler[i], "Got unexpected sampler %p in slot %u.\n", tmp_sampler[i], i);
    }
    ID3D10Device_VSGetShader(device, &tmp_vs);
    ok(!tmp_vs, "Got unexpected vertex shader %p.\n", tmp_vs);

    ID3D10Device_GSGetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, tmp_buffer);
    for (i = 0; i < D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++i)
    {
        ok(!tmp_buffer[i], "Got unexpected constant buffer %p in slot %u.\n", tmp_buffer[i], i);
    }
    ID3D10Device_GSGetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, tmp_srv);
    for (i = 0; i < D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        ok(!tmp_srv[i], "Got unexpected shader resource view %p in slot %u.\n", tmp_srv[i], i);
    }
    ID3D10Device_GSGetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, tmp_sampler);
    for (i = 0; i < D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT; ++i)
    {
        ok(!tmp_sampler[i], "Got unexpected sampler %p in slot %u.\n", tmp_sampler[i], i);
    }
    ID3D10Device_GSGetShader(device, &tmp_gs);
    ok(!tmp_gs, "Got unexpected geometry shader %p.\n", tmp_gs);

    ID3D10Device_PSGetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, tmp_buffer);
    for (i = 0; i < D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++i)
    {
        ok(!tmp_buffer[i], "Got unexpected constant buffer %p in slot %u.\n", tmp_buffer[i], i);
    }
    ID3D10Device_PSGetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, tmp_srv);
    for (i = 0; i < D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        ok(!tmp_srv[i], "Got unexpected shader resource view %p in slot %u.\n", tmp_srv[i], i);
    }
    ID3D10Device_PSGetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, tmp_sampler);
    for (i = 0; i < D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT; ++i)
    {
        ok(!tmp_sampler[i], "Got unexpected sampler %p in slot %u.\n", tmp_sampler[i], i);
    }
    ID3D10Device_PSGetShader(device, &tmp_ps);
    ok(!tmp_ps, "Got unexpected pixel shader %p.\n", tmp_ps);

    ID3D10Device_IAGetVertexBuffers(device, 0, D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, tmp_buffer, stride, offset);
    for (i = 0; i < D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        ok(!tmp_buffer[i], "Got unexpected vertex buffer %p in slot %u.\n", tmp_buffer[i], i);
        ok(!stride[i], "Got unexpected stride %u in slot %u.\n", stride[i], i);
        ok(!offset[i], "Got unexpected offset %u in slot %u.\n", offset[i], i);
    }
    ID3D10Device_IAGetIndexBuffer(device, tmp_buffer, &format, offset);
    ok(!tmp_buffer[0], "Got unexpected index buffer %p.\n", tmp_buffer[0]);
    ok(format == DXGI_FORMAT_R32_UINT, "Got unexpected index buffer format %#x.\n", format);
    ok(!offset[0], "Got unexpected index buffer offset %u.\n", offset[0]);
    ID3D10Device_IAGetInputLayout(device, &tmp_input_layout);
    ok(!tmp_input_layout, "Got unexpected input layout %p.\n", tmp_input_layout);
    ID3D10Device_IAGetPrimitiveTopology(device, &topology);
    ok(topology == D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, "Got unexpected primitive topology %#x.\n", topology);

    ID3D10Device_OMGetBlendState(device, &tmp_blend_state, blend_factor, &sample_mask);
    ok(!tmp_blend_state, "Got unexpected blend state %p.\n", tmp_blend_state);
    ok(blend_factor[0] == 0.0f && blend_factor[1] == 0.0f
            && blend_factor[2] == 0.0f && blend_factor[3] == 0.0f,
            "Got unexpected blend factor {%.8e, %.8e, %.8e, %.8e}.\n",
            blend_factor[0], blend_factor[1], blend_factor[2], blend_factor[3]);
    ok(sample_mask == 0, "Got unexpected sample mask %#x.\n", sample_mask);
    ID3D10Device_OMGetDepthStencilState(device, &tmp_ds_state, &stencil_ref);
    ok(!tmp_ds_state, "Got unexpected depth stencil state %p.\n", tmp_ds_state);
    ok(!stencil_ref, "Got unexpected stencil ref %u.\n", stencil_ref);
    ID3D10Device_OMGetRenderTargets(device, D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT, tmp_rtv, &tmp_dsv);
    for (i = 0; i < D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        ok(!tmp_rtv[i], "Got unexpected render target view %p in slot %u.\n", tmp_rtv[i], i);
    }
    ok(!tmp_dsv, "Got unexpected depth stencil view %p.\n", tmp_dsv);

    ID3D10Device_RSGetScissorRects(device, &count, NULL);
    ok(count == D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE,
            "Got unexpected scissor rect count %u.\n", count);
    memset(tmp_rect, 0x55, sizeof(tmp_rect));
    count = D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    ID3D10Device_RSGetScissorRects(device, &count, tmp_rect);
    for (i = 0; i < D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE; ++i)
    {
        ok(tmp_rect[i].left == i
                && tmp_rect[i].top == i * 2
                && tmp_rect[i].right == i + 1
                && tmp_rect[i].bottom == (i + 1) * 2,
                "Got unexpected scissor rect %s in slot %u.\n",
                wine_dbgstr_rect(&tmp_rect[i]), i);
    }
    ID3D10Device_RSGetViewports(device, &count, NULL);
    ok(count == D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE,
            "Got unexpected viewport count %u.\n", count);
    memset(tmp_viewport, 0x55, sizeof(tmp_viewport));
    count = D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    ID3D10Device_RSGetViewports(device, &count, tmp_viewport);
    for (i = 0; i < D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE; ++i)
    {
        ok(tmp_viewport[i].TopLeftX == i * 3
                    && tmp_viewport[i].TopLeftY == i * 4
                    && tmp_viewport[i].Width == 3
                    && tmp_viewport[i].Height == 4
                    && compare_float(tmp_viewport[i].MinDepth, i * 0.01f, 16)
                    && compare_float(tmp_viewport[i].MaxDepth, (i + 1) * 0.01f, 16),
                    "Got unexpected viewport {%d, %d, %u, %u, %.8e, %.8e} in slot %u.\n",
                    tmp_viewport[i].TopLeftX, tmp_viewport[i].TopLeftY, tmp_viewport[i].Width,
                    tmp_viewport[i].Height, tmp_viewport[i].MinDepth, tmp_viewport[i].MaxDepth, i);
    }
    ID3D10Device_RSGetState(device, &tmp_rs_state);
    ok(!tmp_rs_state, "Got unexpected rasterizer state %p.\n", tmp_rs_state);

    ID3D10Device_SOGetTargets(device, D3D10_SO_BUFFER_SLOT_COUNT, tmp_buffer, offset);
    for (i = 0; i < D3D10_SO_BUFFER_SLOT_COUNT; ++i)
    {
        ok(!tmp_buffer[i], "Got unexpected stream output %p in slot %u.\n", tmp_buffer[i], i);
        ok(offset[i] == ~0u, "Got unexpected stream output offset %u in slot %u.\n", offset[i], i);
    }

    ID3D10Device_GetPredication(device, &tmp_predicate, &predicate_value);
    ok(!tmp_predicate, "Got unexpected predicate %p.\n", tmp_predicate);
    ok(!predicate_value, "Got unexpected predicate value %#x.\n", predicate_value);

    ID3D10Predicate_Release(predicate);
    ID3D10RasterizerState_Release(rs_state);
    ID3D10DepthStencilView_Release(dsv);
    ID3D10Texture2D_Release(ds_texture);

    for (i = 0; i < D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        ID3D10RenderTargetView_Release(rtv[i]);
        ID3D10Texture2D_Release(rt_texture[i]);
    }

    ID3D10DepthStencilState_Release(ds_state);
    ID3D10BlendState_Release(blend_state);
    ID3D10InputLayout_Release(input_layout);
    ID3D10VertexShader_Release(vs);
    ID3D10GeometryShader_Release(gs);
    ID3D10PixelShader_Release(ps);

    for (i = 0; i < D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT; ++i)
    {
        ID3D10SamplerState_Release(sampler[i]);
    }

    for (i = 0; i < D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        ID3D10ShaderResourceView_Release(srv[i]);
    }

    for (i = 0; i < D3D10_SO_BUFFER_SLOT_COUNT; ++i)
    {
        ID3D10Buffer_Release(so_buffer[i]);
    }

    for (i = 0; i < D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        ID3D10Buffer_Release(buffer[i]);
    }

    for (i = 0; i < D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++i)
    {
        ID3D10Buffer_Release(cb[i]);
    }

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_D3DX10CreateAsyncMemoryLoader(void)
{
    ID3DX10DataLoader *loader;
    SIZE_T size;
    DWORD data;
    HRESULT hr;
    void *ptr;

    hr = D3DX10CreateAsyncMemoryLoader(NULL, 0, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);

    hr = D3DX10CreateAsyncMemoryLoader(NULL, 0, &loader);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);

    hr = D3DX10CreateAsyncMemoryLoader(&data, 0, &loader);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    size = 100;
    hr = ID3DX10DataLoader_Decompress(loader, &ptr, &size);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(ptr == &data, "Got data pointer %p, original %p.\n", ptr, &data);
    ok(!size, "Got unexpected data size.\n");

    /* Load() is no-op. */
    hr = ID3DX10DataLoader_Load(loader);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    hr = ID3DX10DataLoader_Destroy(loader);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    data = 0;
    hr = D3DX10CreateAsyncMemoryLoader(&data, sizeof(data), &loader);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    /* Load() is no-op. */
    hr = ID3DX10DataLoader_Load(loader);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    hr = ID3DX10DataLoader_Decompress(loader, &ptr, &size);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(ptr == &data, "Got data pointer %p, original %p.\n", ptr, &data);
    ok(size == sizeof(data), "Got unexpected data size.\n");

    hr = ID3DX10DataLoader_Destroy(loader);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
}

static void create_testfile(WCHAR *path, const void *data, int data_len)
{
    static const WCHAR test_filename[] = {'a','s','y','n','c','l','o','a','d','e','r','.','d','a','t','a',0};
    DWORD written;
    HANDLE file;
    BOOL ret;

    GetTempPathW(MAX_PATH, path);
    lstrcatW(path, test_filename);

    file = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "Test file creation failed, at %s, error %d.\n", wine_dbgstr_w(path),
        GetLastError());

    ret = WriteFile(file, data, data_len, &written, NULL);
    ok(ret, "Write to test file failed.\n");

    CloseHandle(file);
}

static void test_D3DX10CreateAsyncFileLoader(void)
{
    static const char test_data1[] = "test data";
    static const char test_data2[] = "more test data";
    ID3DX10DataLoader *loader;
    WCHAR path[MAX_PATH];
    SIZE_T size;
    HRESULT hr;
    void *ptr;
    BOOL ret;

    hr = D3DX10CreateAsyncFileLoaderA(NULL, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);

    hr = D3DX10CreateAsyncFileLoaderA(NULL, &loader);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);

    hr = D3DX10CreateAsyncFileLoaderA("nonexistentfilename", &loader);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    hr = ID3DX10DataLoader_Decompress(loader, &ptr, &size);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);

    hr = ID3DX10DataLoader_Load(loader);
    ok(hr == D3D10_ERROR_FILE_NOT_FOUND, "Got unexpected hr %#x.\n", hr);

    hr = ID3DX10DataLoader_Decompress(loader, &ptr, &size);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);

    hr = ID3DX10DataLoader_Destroy(loader);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    /* Test file sharing using dummy empty file. */
    create_testfile(path, test_data1, sizeof(test_data1));

    hr = D3DX10CreateAsyncFileLoaderW(path, &loader);
    ok(SUCCEEDED(hr), "Failed to create file loader, hr %#x.\n", hr);

    ret = DeleteFileW(path);
    ok(ret, "DeleteFile() failed, ret %d, error %d.\n", ret, GetLastError());

    /* File was removed before Load(). */
    hr = ID3DX10DataLoader_Load(loader);
    ok(hr == D3D10_ERROR_FILE_NOT_FOUND, "Load() returned unexpected result, hr %#x.\n", hr);

    /* Create it again. */
    create_testfile(path, test_data1, sizeof(test_data1));
    hr = ID3DX10DataLoader_Load(loader);
    ok(SUCCEEDED(hr), "Load() failed, hr %#x.\n", hr);

    /* Already loaded. */
    hr = ID3DX10DataLoader_Load(loader);
    ok(SUCCEEDED(hr), "Load() failed, hr %#x.\n", hr);

    ret = DeleteFileW(path);
    ok(ret, "DeleteFile() failed, ret %d, error %d.\n", ret, GetLastError());

    /* Already loaded, file removed. */
    hr = ID3DX10DataLoader_Load(loader);
    ok(hr == D3D10_ERROR_FILE_NOT_FOUND, "Load() returned unexpected result, hr %#x.\n", hr);

    /* Decompress still works. */
    ptr = NULL;
    hr = ID3DX10DataLoader_Decompress(loader, &ptr, &size);
    ok(SUCCEEDED(hr), "Decompress() failed, hr %#x.\n", hr);
    ok(ptr != NULL, "Got unexpected ptr %p.\n", ptr);
    ok(size == sizeof(test_data1), "Got unexpected decompressed size.\n");
    if (size == sizeof(test_data1))
        ok(!memcmp(ptr, test_data1, size), "Got unexpected file data.\n");

    /* Create it again, with different data. */
    create_testfile(path, test_data2, sizeof(test_data2));

    hr = ID3DX10DataLoader_Load(loader);
    ok(SUCCEEDED(hr), "Load() failed, hr %#x.\n", hr);

    ptr = NULL;
    hr = ID3DX10DataLoader_Decompress(loader, &ptr, &size);
    ok(SUCCEEDED(hr), "Decompress() failed, hr %#x.\n", hr);
    ok(ptr != NULL, "Got unexpected ptr %p.\n", ptr);
    ok(size == sizeof(test_data2), "Got unexpected decompressed size.\n");
    if (size == sizeof(test_data2))
        ok(!memcmp(ptr, test_data2, size), "Got unexpected file data.\n");

    hr = ID3DX10DataLoader_Destroy(loader);
    ok(SUCCEEDED(hr), "Destroy() failed, hr %#x.\n", hr);

    ret = DeleteFileW(path);
    ok(ret, "DeleteFile() failed, ret %d, error %d.\n", ret, GetLastError());
}

static void test_D3DX10CreateAsyncResourceLoader(void)
{
    static const WCHAR resource_name[] = {'n','o','n','a','m','e',0};
    ID3DX10DataLoader *loader;
    HRESULT hr;

    hr = D3DX10CreateAsyncResourceLoaderA(NULL, NULL, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);

    hr = D3DX10CreateAsyncResourceLoaderA(NULL, NULL, &loader);
    ok(hr == D3DX10_ERR_INVALID_DATA, "Got unexpected hr %#x.\n", hr);

    hr = D3DX10CreateAsyncResourceLoaderA(NULL, "noname", &loader);
    ok(hr == D3DX10_ERR_INVALID_DATA, "Got unexpected hr %#x.\n", hr);

    hr = D3DX10CreateAsyncResourceLoaderW(NULL, NULL, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);

    hr = D3DX10CreateAsyncResourceLoaderW(NULL, NULL, &loader);
    ok(hr == D3DX10_ERR_INVALID_DATA, "Got unexpected hr %#x.\n", hr);

    hr = D3DX10CreateAsyncResourceLoaderW(NULL, resource_name, &loader);
    ok(hr == D3DX10_ERR_INVALID_DATA, "Got unexpected hr %#x.\n", hr);
}

START_TEST(d3dx10)
{
    test_D3DX10UnsetAllDeviceObjects();
    test_D3DX10CreateAsyncMemoryLoader();
    test_D3DX10CreateAsyncFileLoader();
    test_D3DX10CreateAsyncResourceLoader();
}
