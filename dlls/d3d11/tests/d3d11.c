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

#define COBJMACROS
#include "initguid.h"
#include "d3d11.h"
#include "wine/test.h"

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

static ULONG get_refcount(IUnknown *iface)
{
    IUnknown_AddRef(iface);
    return IUnknown_Release(iface);
}

static ID3D11Device *create_device(const D3D_FEATURE_LEVEL *feature_level)
{
    ID3D11Device *device;
    UINT feature_level_count = feature_level ? 1 : 0;

    if (SUCCEEDED(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, feature_level, feature_level_count,
            D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;
    if (SUCCEEDED(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_WARP, NULL, 0, feature_level, feature_level_count,
            D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;
    if (SUCCEEDED(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_REFERENCE, NULL, 0, feature_level, feature_level_count,
            D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;

    return NULL;
}

static void test_create_device(void)
{
    D3D_FEATURE_LEVEL feature_level, supported_feature_level;
    DXGI_SWAP_CHAIN_DESC swapchain_desc, obtained_desc;
    ID3D11DeviceContext *immediate_context;
    IDXGISwapChain *swapchain;
    ID3D11Device *device;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &device,
            NULL, NULL);
    if (FAILED(hr))
    {
        skip("Failed to create HAL device.\n");
        return;
    }

    supported_feature_level = ID3D11Device_GetFeatureLevel(device);
    ID3D11Device_Release(device);

    hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "D3D11CreateDevice failed %#x.\n", hr);

    hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, NULL,
            &feature_level, NULL);
    ok(SUCCEEDED(hr), "D3D11CreateDevice failed %#x.\n", hr);
    ok(feature_level == supported_feature_level, "Got feature level %#x, expected %#x.\n",
            feature_level, supported_feature_level);

    hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, NULL, NULL,
            &immediate_context);
    ok(SUCCEEDED(hr), "D3D11CreateDevice failed %#x.\n", hr);

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
    ok(SUCCEEDED(hr), "D3D11CreateDeviceAndSwapChain failed %#x.\n", hr);

    hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
            &swapchain_desc, NULL, NULL, &feature_level, NULL);
    ok(SUCCEEDED(hr), "D3D11CreateDeviceAndSwapChain failed %#x.\n", hr);
    ok(feature_level == supported_feature_level, "Got feature level %#x, expected %#x.\n",
            feature_level, supported_feature_level);

    hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
            &swapchain_desc, &swapchain, &device, NULL, NULL);
    ok(SUCCEEDED(hr), "D3D11CreateDeviceAndSwapChain failed %#x.\n", hr);

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
    ok(SUCCEEDED(hr), "D3D11CreateDeviceAndSwapChain failed %#x.\n", hr);
    ID3D11Device_Release(device);

    hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
            NULL, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "D3D11CreateDeviceAndSwapChain failed %#x.\n", hr);

    hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
            NULL, NULL, NULL, &feature_level, NULL);
    ok(SUCCEEDED(hr), "D3D11CreateDeviceAndSwapChain failed %#x.\n", hr);
    ok(feature_level == supported_feature_level, "Got feature level %#x, expected %#x.\n",
            feature_level, supported_feature_level);

    hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
            &swapchain_desc, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "D3D11CreateDeviceAndSwapChain failed %#x.\n", hr);

    swapchain_desc.OutputWindow = NULL;
    hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
            &swapchain_desc, NULL, &device, NULL, NULL);
    ok(SUCCEEDED(hr), "D3D11CreateDeviceAndSwapChain failed %#x.\n", hr);
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
    todo_wine ok(hr == E_INVALIDARG, "D3D11CreateDeviceAndSwapChain returned %#x.\n", hr);
    ok(!swapchain, "Got unexpected swapchain pointer %p.\n", swapchain);
    ok(!device, "Got unexpected device pointer %p.\n", device);
    ok(!feature_level, "Got unexpected feature level %#x.\n", feature_level);
    ok(!immediate_context, "Got unexpected immediate context pointer %p.\n", immediate_context);

    DestroyWindow(window);
}

static void test_device_interfaces(void)
{
    ID3D11Device *device;
    IUnknown *iface;
    ULONG refcount;
    unsigned int i;
    HRESULT hr;

    for (i = 0; i < sizeof(d3d11_feature_levels) / sizeof(*d3d11_feature_levels); ++i)
    {
        if (!(device = create_device(&d3d11_feature_levels[i])))
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

        hr = ID3D11Device_QueryInterface(device, &IID_IDXGIDevice, (void **)&iface);
        ok(SUCCEEDED(hr), "Device should implement IDXGIDevice interface, hr %#x.\n", hr);
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
    ID3D11Device *device, *tmp;
    D3D11_TEXTURE2D_DESC desc;
    ID3D11Texture2D *texture;
    IDXGISurface *surface;
    HRESULT hr;

    if (!(device = create_device(NULL)))
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
    HRESULT hr;

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

    desc.ByteWidth = 1024;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    hr = ID3D11Device_CreateBuffer(device, &desc, NULL, &buffer);
    ok(SUCCEEDED(hr), "Failed to create a buffer, hr %#x.\n", hr);

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

    ID3D11DepthStencilView_GetDesc(dsview, &dsv_desc);
    ok(dsv_desc.Format == texture_desc.Format, "Got unexpected format %#x.\n", dsv_desc.Format);
    ok(dsv_desc.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE2D,
            "Got unexpected view dimension %#x.\n", dsv_desc.ViewDimension);
    ok(!dsv_desc.Flags, "Got unexpected flags %#x.\n", dsv_desc.Flags);
    ok(U(dsv_desc).Texture2D.MipSlice == 0, "Got Unexpected mip slice %u.\n", U(dsv_desc).Texture2D.MipSlice);

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

    ID3D11RenderTargetView_GetDesc(rtview, &rtv_desc);
    ok(rtv_desc.Format == texture_desc.Format, "Got unexpected format %#x.\n", rtv_desc.Format);
    ok(rtv_desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2D, "Got unexpected view dimension %#x.\n",
            rtv_desc.ViewDimension);
    ok(U(rtv_desc).Texture2D.MipSlice == 0, "Got unexpected mip slice %#x.\n", U(rtv_desc).Texture2D.MipSlice);

    hr = ID3D11RenderTargetView_QueryInterface(rtview, &IID_ID3D10RenderTargetView, (void **)&iface);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Render target view should implement ID3D10RenderTargetView.\n");
    if (SUCCEEDED(hr)) IUnknown_Release(iface);

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
    D3D11_BUFFER_DESC buffer_desc;
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
    buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.MiscFlags = 0;
    buffer_desc.StructureByteStride = 0;

    hr = ID3D11Device_CreateBuffer(device, &buffer_desc, NULL, &buffer);
    ok(SUCCEEDED(hr), "Failed to create a buffer, hr %#x.\n", hr);

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

    ID3D11ShaderResourceView_GetDesc(srview, &srv_desc);
    ok(srv_desc.Format == texture_desc.Format, "Got unexpected format %#x.\n", srv_desc.Format);
    ok(srv_desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2D,
            "Got unexpected view dimension %#x.\n", srv_desc.ViewDimension);
    ok(U(srv_desc).Texture2D.MostDetailedMip == 0, "Got unexpected MostDetailedMip %u.\n",
            U(srv_desc).Texture2D.MostDetailedMip);
    ok(U(srv_desc).Texture2D.MipLevels == 10, "Got unexpected MipLevels %u.\n", U(srv_desc).Texture2D.MipLevels);

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
        if (!(device = create_device(&feature_level)))
        {
            skip("Failed to create device for feature level %#x.\n", feature_level);
            continue;
        }

        /* vertex shader */
        hr = ID3D11Device_CreateVertexShader(device, vs_2_0, sizeof(vs_2_0), NULL, &vs);
        ok(hr == E_INVALIDARG, "Created a SM2 vertex shader, hr %#x, feature level %#x.\n", hr, feature_level);

        hr = ID3D11Device_CreateVertexShader(device, vs_3_0, sizeof(vs_3_0), NULL, &vs);
        ok(hr == E_INVALIDARG, "Created a SM3 vertex shader, hr %#x, feature level %#x.\n", hr, feature_level);

        hr = ID3D11Device_CreateVertexShader(device, ps_4_0, sizeof(ps_4_0), NULL, &vs);
        ok(hr == E_INVALIDARG, "Created a SM4 vertex shader from a pixel shader source, hr %#x, feature level %#x.\n",
                hr, feature_level);

        if (feature_level < D3D_FEATURE_LEVEL_10_0)
        {
            refcount = ID3D11Device_Release(device);
            ok(!refcount, "Device has %u references left.\n", refcount);
            continue;
        }

        expected_refcount = get_refcount((IUnknown *)device) + 1;
        hr = ID3D11Device_CreateVertexShader(device, vs_4_0, sizeof(vs_4_0), NULL, &vs);
        ok(SUCCEEDED(hr), "Failed to create SM4 vertex shader, hr %#x, feature level %#x.\n", hr, feature_level);

        refcount = get_refcount((IUnknown *)device);
        ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n",
                refcount, expected_refcount);
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

        /* pixel shader */
        expected_refcount = get_refcount((IUnknown *)device) + 1;
        hr = ID3D11Device_CreatePixelShader(device, ps_4_0, sizeof(ps_4_0), NULL, &ps);
        ok(SUCCEEDED(hr), "Failed to create SM4 vertex shader, hr %#x, feature level %#x.\n", hr, feature_level);

        refcount = get_refcount((IUnknown *)device);
        ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n",
                refcount, expected_refcount);
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

        /* geometry shader */
        expected_refcount = get_refcount((IUnknown *)device) + 1;
        hr = ID3D11Device_CreateGeometryShader(device, gs_4_0, sizeof(gs_4_0), NULL, &gs);
        ok(SUCCEEDED(hr), "Failed to create SM4 geometry shader, hr %#x.\n", hr);

        refcount = get_refcount((IUnknown *)device);
        ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n",
                refcount, expected_refcount);
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

        refcount = ID3D11Device_Release(device);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
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
    test_create_blend_state();
    test_create_depthstencil_state();
    test_create_rasterizer_state();
    test_device_removed_reason();
    test_private_data();
}
