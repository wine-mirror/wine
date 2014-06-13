/*
 * Copyright 2008 Henri Verbeet for CodeWeavers
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
#include "d3d10.h"
#include "wine/test.h"

static ULONG get_refcount(IUnknown *iface)
{
    IUnknown_AddRef(iface);
    return IUnknown_Release(iface);
}

static ID3D10Device *create_device(void)
{
    ID3D10Device *device;

    if (SUCCEEDED(D3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &device)))
        return device;
    if (SUCCEEDED(D3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_WARP, NULL, 0, D3D10_SDK_VERSION, &device)))
        return device;
    if (SUCCEEDED(D3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_REFERENCE, NULL, 0, D3D10_SDK_VERSION, &device)))
        return device;

    return NULL;
}

static void test_create_texture2d(void)
{
    ULONG refcount, expected_refcount;
    ID3D10Device *device, *tmp;
    D3D10_TEXTURE2D_DESC desc;
    ID3D10Texture2D *texture;
    IDXGISurface *surface;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    desc.Width = 512;
    desc.Height = 512;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D10_USAGE_DEFAULT;
    desc.BindFlags = D3D10_BIND_RENDER_TARGET;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreateTexture2D(device, &desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 2d texture, hr %#x\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10Texture2D_GetDevice(texture, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);

    hr = ID3D10Texture2D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface);
    ok(SUCCEEDED(hr), "Texture should implement IDXGISurface\n");
    if (SUCCEEDED(hr)) IDXGISurface_Release(surface);
    ID3D10Texture2D_Release(texture);

    desc.MipLevels = 0;
    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreateTexture2D(device, &desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 2d texture, hr %#x\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10Texture2D_GetDevice(texture, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);

    ID3D10Texture2D_GetDesc(texture, &desc);
    ok(desc.Width == 512, "Got unexpected Width %u.\n", desc.Width);
    ok(desc.Height == 512, "Got unexpected Height %u.\n", desc.Height);
    ok(desc.MipLevels == 10, "Got unexpected MipLevels %u.\n", desc.MipLevels);
    ok(desc.ArraySize == 1, "Got unexpected ArraySize %u.\n", desc.ArraySize);
    ok(desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM, "Got unexpected Format %#x.\n", desc.Format);
    ok(desc.SampleDesc.Count == 1, "Got unexpected SampleDesc.Count %u.\n", desc.SampleDesc.Count);
    ok(desc.SampleDesc.Quality == 0, "Got unexpected SampleDesc.Quality %u.\n", desc.SampleDesc.Quality);
    ok(desc.Usage == D3D10_USAGE_DEFAULT, "Got unexpected MipLevels %u.\n", desc.Usage);
    ok(desc.BindFlags == D3D10_BIND_RENDER_TARGET, "Got unexpected BindFlags %u.\n", desc.BindFlags);
    ok(desc.CPUAccessFlags == 0, "Got unexpected CPUAccessFlags %u.\n", desc.CPUAccessFlags);
    ok(desc.MiscFlags == 0, "Got unexpected MiscFlags %u.\n", desc.MiscFlags);

    hr = ID3D10Texture2D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface);
    ok(FAILED(hr), "Texture should not implement IDXGISurface\n");
    if (SUCCEEDED(hr)) IDXGISurface_Release(surface);
    ID3D10Texture2D_Release(texture);

    desc.MipLevels = 1;
    desc.ArraySize = 2;
    hr = ID3D10Device_CreateTexture2D(device, &desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 2d texture, hr %#x\n", hr);

    hr = ID3D10Texture2D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface);
    ok(FAILED(hr), "Texture should not implement IDXGISurface\n");
    if (SUCCEEDED(hr)) IDXGISurface_Release(surface);
    ID3D10Texture2D_Release(texture);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_texture3d(void)
{
    ULONG refcount, expected_refcount;
    ID3D10Device *device, *tmp;
    D3D10_TEXTURE3D_DESC desc;
    ID3D10Texture3D *texture;
    IDXGISurface *surface;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    desc.Width = 64;
    desc.Height = 64;
    desc.Depth = 64;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage = D3D10_USAGE_DEFAULT;
    desc.BindFlags = D3D10_BIND_RENDER_TARGET;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreateTexture3D(device, &desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 3d texture, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10Texture3D_GetDevice(texture, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);

    hr = ID3D10Texture3D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface);
    ok(FAILED(hr), "Texture should not implement IDXGISurface.\n");
    if (SUCCEEDED(hr)) IDXGISurface_Release(surface);
    ID3D10Texture3D_Release(texture);

    desc.MipLevels = 0;
    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreateTexture3D(device, &desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 3d texture, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10Texture3D_GetDevice(texture, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);

    ID3D10Texture3D_GetDesc(texture, &desc);
    ok(desc.Width == 64, "Got unexpected Width %u.\n", desc.Width);
    ok(desc.Height == 64, "Got unexpected Height %u.\n", desc.Height);
    ok(desc.Depth == 64, "Got unexpected Depth %u.\n", desc.Depth);
    ok(desc.MipLevels == 7, "Got unexpected MipLevels %u.\n", desc.MipLevels);
    ok(desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM, "Got unexpected Format %#x.\n", desc.Format);
    ok(desc.Usage == D3D10_USAGE_DEFAULT, "Got unexpected MipLevels %u.\n", desc.Usage);
    ok(desc.BindFlags == D3D10_BIND_RENDER_TARGET, "Got unexpected BindFlags %u.\n", desc.BindFlags);
    ok(desc.CPUAccessFlags == 0, "Got unexpected CPUAccessFlags %u.\n", desc.CPUAccessFlags);
    ok(desc.MiscFlags == 0, "Got unexpected MiscFlags %u.\n", desc.MiscFlags);

    hr = ID3D10Texture3D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface);
    ok(FAILED(hr), "Texture should not implement IDXGISurface.\n");
    if (SUCCEEDED(hr)) IDXGISurface_Release(surface);
    ID3D10Texture3D_Release(texture);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_depthstencil_view(void)
{
    D3D10_DEPTH_STENCIL_VIEW_DESC dsv_desc;
    D3D10_TEXTURE2D_DESC texture_desc;
    ULONG refcount, expected_refcount;
    ID3D10DepthStencilView *dsview;
    ID3D10Device *device, *tmp;
    ID3D10Texture2D *texture;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    texture_desc.Width = 512;
    texture_desc.Height = 512;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_DEPTH_STENCIL;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 2d texture, hr %#x\n", hr);

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreateDepthStencilView(device, (ID3D10Resource *)texture, NULL, &dsview);
    ok(SUCCEEDED(hr), "Failed to create a depthstencil view, hr %#x\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10DepthStencilView_GetDevice(dsview, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);

    ID3D10DepthStencilView_GetDesc(dsview, &dsv_desc);
    ok(dsv_desc.Format == texture_desc.Format, "Got unexpected format %#x.\n", dsv_desc.Format);
    ok(dsv_desc.ViewDimension == D3D10_DSV_DIMENSION_TEXTURE2D,
            "Got unexpected view dimension %#x.\n", dsv_desc.ViewDimension);
    ok(U(dsv_desc).Texture2D.MipSlice == 0, "Got Unexpected mip slice %u.\n", U(dsv_desc).Texture2D.MipSlice);

    ID3D10DepthStencilView_Release(dsview);
    ID3D10Texture2D_Release(texture);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_rendertarget_view(void)
{
    D3D10_RENDER_TARGET_VIEW_DESC rtv_desc;
    D3D10_TEXTURE2D_DESC texture_desc;
    ULONG refcount, expected_refcount;
    D3D10_BUFFER_DESC buffer_desc;
    ID3D10RenderTargetView *rtview;
    ID3D10Device *device, *tmp;
    ID3D10Texture2D *texture;
    ID3D10Buffer *buffer;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    buffer_desc.ByteWidth = 1024;
    buffer_desc.Usage = D3D10_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D10_BIND_RENDER_TARGET;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.MiscFlags = 0;

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreateBuffer(device, &buffer_desc, NULL, &buffer);
    ok(SUCCEEDED(hr), "Failed to create a buffer, hr %#x\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10Buffer_GetDevice(buffer, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);

    rtv_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    rtv_desc.ViewDimension = D3D10_RTV_DIMENSION_BUFFER;
    U(rtv_desc).Buffer.ElementOffset = 0;
    U(rtv_desc).Buffer.ElementWidth = 64;

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreateRenderTargetView(device, (ID3D10Resource *)buffer, &rtv_desc, &rtview);
    ok(SUCCEEDED(hr), "Failed to create a rendertarget view, hr %#x\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10RenderTargetView_GetDevice(rtview, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);

    ID3D10RenderTargetView_Release(rtview);
    ID3D10Buffer_Release(buffer);

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

    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 2d texture, hr %#x\n", hr);

    /* For texture resources it's allowed to specify NULL as desc */
    hr = ID3D10Device_CreateRenderTargetView(device, (ID3D10Resource *)texture, NULL, &rtview);
    ok(SUCCEEDED(hr), "Failed to create a rendertarget view, hr %#x\n", hr);

    ID3D10RenderTargetView_GetDesc(rtview, &rtv_desc);
    ok(rtv_desc.Format == texture_desc.Format, "Expected format %#x, got %#x\n", texture_desc.Format, rtv_desc.Format);
    ok(rtv_desc.ViewDimension == D3D10_RTV_DIMENSION_TEXTURE2D,
            "Expected view dimension D3D10_RTV_DIMENSION_TEXTURE2D, got %#x\n", rtv_desc.ViewDimension);
    ok(U(rtv_desc).Texture2D.MipSlice == 0, "Expected mip slice 0, got %#x\n", U(rtv_desc).Texture2D.MipSlice);

    ID3D10RenderTargetView_Release(rtview);
    ID3D10Texture2D_Release(texture);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_shader_resource_view(void)
{
    D3D10_SHADER_RESOURCE_VIEW_DESC srv_desc;
    D3D10_TEXTURE2D_DESC texture_desc;
    ULONG refcount, expected_refcount;
    ID3D10ShaderResourceView *srview;
    D3D10_BUFFER_DESC buffer_desc;
    ID3D10Device *device, *tmp;
    ID3D10Texture2D *texture;
    ID3D10Buffer *buffer;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    buffer_desc.ByteWidth = 1024;
    buffer_desc.Usage = D3D10_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.MiscFlags = 0;

    hr = ID3D10Device_CreateBuffer(device, &buffer_desc, NULL, &buffer);
    ok(SUCCEEDED(hr), "Failed to create a buffer, hr %#x\n", hr);

    hr = ID3D10Device_CreateShaderResourceView(device, (ID3D10Resource *)buffer, NULL, &srview);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    srv_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srv_desc.ViewDimension = D3D10_SRV_DIMENSION_BUFFER;
    U(srv_desc).Buffer.ElementOffset = 0;
    U(srv_desc).Buffer.ElementWidth = 64;

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreateShaderResourceView(device, (ID3D10Resource *)buffer, &srv_desc, &srview);
    ok(SUCCEEDED(hr), "Failed to create a shader resource view, hr %#x\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10ShaderResourceView_GetDevice(srview, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);

    ID3D10ShaderResourceView_Release(srview);
    ID3D10Buffer_Release(buffer);

    texture_desc.Width = 512;
    texture_desc.Height = 512;
    texture_desc.MipLevels = 0;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 2d texture, hr %#x\n", hr);

    hr = ID3D10Device_CreateShaderResourceView(device, (ID3D10Resource *)texture, NULL, &srview);
    ok(SUCCEEDED(hr), "Failed to create a shader resource view, hr %#x\n", hr);

    ID3D10ShaderResourceView_GetDesc(srview, &srv_desc);
    ok(srv_desc.Format == texture_desc.Format, "Got unexpected format %#x.\n", srv_desc.Format);
    ok(srv_desc.ViewDimension == D3D10_SRV_DIMENSION_TEXTURE2D,
            "Got unexpected view dimension %#x.\n", srv_desc.ViewDimension);
    ok(U(srv_desc).Texture2D.MostDetailedMip == 0, "Got unexpected MostDetailedMip %u.\n",
            U(srv_desc).Texture2D.MostDetailedMip);
    ok(U(srv_desc).Texture2D.MipLevels == 10, "Got unexpected MipLevels %u.\n", U(srv_desc).Texture2D.MipLevels);

    ID3D10ShaderResourceView_Release(srview);
    ID3D10Texture2D_Release(texture);

    refcount = ID3D10Device_Release(device);
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

#if 0
float4 main(const float4 color : COLOR) : SV_TARGET
{
    float4 o;

    o = color;

    return o;
}
#endif
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

    ULONG refcount, expected_refcount;
    ID3D10VertexShader *vs = NULL;
    ID3D10PixelShader *ps = NULL;
    ID3D10Device *device, *tmp;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreateVertexShader(device, vs_4_0, sizeof(vs_4_0), &vs);
    ok(SUCCEEDED(hr), "Failed to create SM4 vertex shader, hr %#x\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10VertexShader_GetDevice(vs, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);
    ID3D10VertexShader_Release(vs);

    hr = ID3D10Device_CreateVertexShader(device, vs_2_0, sizeof(vs_2_0), &vs);
    ok(hr == E_INVALIDARG, "Created a SM2 vertex shader, hr %#x\n", hr);

    hr = ID3D10Device_CreateVertexShader(device, vs_3_0, sizeof(vs_3_0), &vs);
    ok(hr == E_INVALIDARG, "Created a SM3 vertex shader, hr %#x\n", hr);

    hr = ID3D10Device_CreateVertexShader(device, ps_4_0, sizeof(ps_4_0), &vs);
    ok(hr == E_INVALIDARG, "Created a SM4 vertex shader from a pixel shader source, hr %#x\n", hr);

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreatePixelShader(device, ps_4_0, sizeof(ps_4_0), &ps);
    ok(SUCCEEDED(hr), "Failed to create SM4 vertex shader, hr %#x\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10PixelShader_GetDevice(ps, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);
    ID3D10PixelShader_Release(ps);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_sampler_state(void)
{
    ID3D10SamplerState *sampler_state1, *sampler_state2;
    ULONG refcount, expected_refcount;
    D3D10_SAMPLER_DESC sampler_desc;
    ID3D10Device *device, *tmp;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = ID3D10Device_CreateSamplerState(device, NULL, &sampler_state1);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    sampler_desc.Filter = D3D10_FILTER_MIN_MAG_MIP_LINEAR;
    sampler_desc.AddressU = D3D10_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressV = D3D10_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressW = D3D10_TEXTURE_ADDRESS_WRAP;
    sampler_desc.MipLODBias = 0.0f;
    sampler_desc.MaxAnisotropy = 16;
    sampler_desc.ComparisonFunc = D3D10_COMPARISON_ALWAYS;
    sampler_desc.BorderColor[0] = 0.0f;
    sampler_desc.BorderColor[1] = 1.0f;
    sampler_desc.BorderColor[2] = 0.0f;
    sampler_desc.BorderColor[3] = 1.0f;
    sampler_desc.MinLOD = 0.0f;
    sampler_desc.MaxLOD = 16.0f;

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreateSamplerState(device, &sampler_desc, &sampler_state1);
    ok(SUCCEEDED(hr), "Failed to create sampler state, hr %#x.\n", hr);
    hr = ID3D10Device_CreateSamplerState(device, &sampler_desc, &sampler_state2);
    ok(SUCCEEDED(hr), "Failed to create sampler state, hr %#x.\n", hr);
    ok(sampler_state1 == sampler_state2, "Got different sampler state objects.\n");
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10SamplerState_GetDevice(sampler_state1, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);

    refcount = ID3D10SamplerState_Release(sampler_state2);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    refcount = ID3D10SamplerState_Release(sampler_state1);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_blend_state(void)
{
    ID3D10BlendState *blend_state1, *blend_state2;
    ULONG refcount, expected_refcount;
    D3D10_BLEND_DESC blend_desc;
    ID3D10Device *device, *tmp;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = ID3D10Device_CreateBlendState(device, NULL, &blend_state1);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

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

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreateBlendState(device, &blend_desc, &blend_state1);
    ok(SUCCEEDED(hr), "Failed to create blend state, hr %#x.\n", hr);
    hr = ID3D10Device_CreateBlendState(device, &blend_desc, &blend_state2);
    ok(SUCCEEDED(hr), "Failed to create blend state, hr %#x.\n", hr);
    ok(blend_state1 == blend_state2, "Got different blend state objects.\n");
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10BlendState_GetDevice(blend_state1, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);

    refcount = ID3D10BlendState_Release(blend_state2);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    refcount = ID3D10BlendState_Release(blend_state1);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_depthstencil_state(void)
{
    ID3D10DepthStencilState *ds_state1, *ds_state2;
    ULONG refcount, expected_refcount;
    D3D10_DEPTH_STENCIL_DESC ds_desc;
    ID3D10Device *device, *tmp;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = ID3D10Device_CreateDepthStencilState(device, NULL, &ds_state1);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

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

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreateDepthStencilState(device, &ds_desc, &ds_state1);
    ok(SUCCEEDED(hr), "Failed to create depthstencil state, hr %#x.\n", hr);
    hr = ID3D10Device_CreateDepthStencilState(device, &ds_desc, &ds_state2);
    ok(SUCCEEDED(hr), "Failed to create depthstencil state, hr %#x.\n", hr);
    ok(ds_state1 == ds_state2, "Got different depthstencil state objects.\n");
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10DepthStencilState_GetDevice(ds_state1, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);

    refcount = ID3D10DepthStencilState_Release(ds_state2);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    refcount = ID3D10DepthStencilState_Release(ds_state1);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_rasterizer_state(void)
{
    ID3D10RasterizerState *rast_state1, *rast_state2;
    ULONG refcount, expected_refcount;
    D3D10_RASTERIZER_DESC rast_desc;
    ID3D10Device *device, *tmp;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = ID3D10Device_CreateRasterizerState(device, NULL, &rast_state1);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    rast_desc.FillMode = D3D10_FILL_SOLID;
    rast_desc.CullMode = D3D10_CULL_BACK;
    rast_desc.FrontCounterClockwise = FALSE;
    rast_desc.DepthBias = 0;
    rast_desc.DepthBiasClamp = 0.0f;
    rast_desc.SlopeScaledDepthBias = 0.0f;
    rast_desc.DepthClipEnable = TRUE;
    rast_desc.ScissorEnable = FALSE;
    rast_desc.MultisampleEnable = FALSE;
    rast_desc.AntialiasedLineEnable = FALSE;

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreateRasterizerState(device, &rast_desc, &rast_state1);
    ok(SUCCEEDED(hr), "Failed to create rasterizer state, hr %#x.\n", hr);
    hr = ID3D10Device_CreateRasterizerState(device, &rast_desc, &rast_state2);
    ok(SUCCEEDED(hr), "Failed to create rasterizer state, hr %#x.\n", hr);
    ok(rast_state1 == rast_state2, "Got different rasterizer state objects.\n");
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10RasterizerState_GetDevice(rast_state1, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);

    refcount = ID3D10RasterizerState_Release(rast_state2);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    refcount = ID3D10RasterizerState_Release(rast_state1);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_predicate(void)
{
    ULONG refcount, expected_refcount;
    D3D10_QUERY_DESC query_desc;
    ID3D10Predicate *predicate;
    ID3D10Device *device, *tmp;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = ID3D10Device_CreatePredicate(device, NULL, &predicate);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    query_desc.Query = D3D10_QUERY_OCCLUSION;
    query_desc.MiscFlags = 0;
    hr = ID3D10Device_CreatePredicate(device, &query_desc, &predicate);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    query_desc.Query = D3D10_QUERY_OCCLUSION_PREDICATE;
    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreatePredicate(device, &query_desc, &predicate);
    ok(SUCCEEDED(hr), "Failed to create predicate, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10Predicate_GetDevice(predicate, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);
    ID3D10Predicate_Release(predicate);

    query_desc.Query = D3D10_QUERY_SO_OVERFLOW_PREDICATE;
    hr = ID3D10Device_CreatePredicate(device, &query_desc, &predicate);
    ok(SUCCEEDED(hr), "Failed to create predicate, hr %#x.\n", hr);
    ID3D10Predicate_Release(predicate);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_device_removed_reason(void)
{
    ID3D10Device *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = ID3D10Device_GetDeviceRemovedReason(device);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = ID3D10Device_GetDeviceRemovedReason(device);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

START_TEST(device)
{
    test_create_texture2d();
    test_create_texture3d();
    test_create_depthstencil_view();
    test_create_rendertarget_view();
    test_create_shader_resource_view();
    test_create_shader();
    test_create_sampler_state();
    test_create_blend_state();
    test_create_depthstencil_state();
    test_create_rasterizer_state();
    test_create_predicate();
    test_device_removed_reason();
}
