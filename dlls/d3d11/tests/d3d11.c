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
    ID3D11DeviceContext *immediate_context = NULL;
    ID3D11Device *device;
    ULONG refcount;
    HRESULT hr;

    hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &device,
            NULL, NULL);
    if (FAILED(hr))
    {
        skip("Failed to create HAL device, skipping tests.\n");
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

    todo_wine ok(!!immediate_context, "Immediate context is NULL.\n");
    if (!immediate_context) return;

    refcount = get_refcount((IUnknown *)immediate_context);
    ok(refcount == 1, "Got refcount %u, expected 1.\n", refcount);

    ID3D11DeviceContext_GetDevice(immediate_context, &device);
    refcount = ID3D11Device_Release(device);
    ok(refcount == 1, "Got refcount %u, expected 1.\n", refcount);

    refcount = ID3D11DeviceContext_Release(immediate_context);
    ok(!refcount, "ID3D11DeviceContext has %u references left.\n", refcount);
}

static void test_device_interfaces(void)
{
    static const D3D_FEATURE_LEVEL feature_levels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1
    };
    ID3D11Device *device;
    IUnknown *iface;
    ULONG refcount;
    unsigned int i;
    HRESULT hr;

    for (i = 0; i < sizeof(feature_levels) / sizeof(*feature_levels); i++)
    {
        if (!(device = create_device(&feature_levels[i])))
        {
            skip("Failed to create device for feature level %#x, skipping tests.\n", feature_levels[i]);
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

START_TEST(d3d11)
{
    test_create_device();
    test_device_interfaces();
    test_create_texture2d();
    test_texture2d_interfaces();
    test_create_texture3d();
    test_texture3d_interfaces();
    test_buffer_interfaces();
    test_depthstencil_view_interfaces();
    test_create_rendertarget_view();
    test_create_shader_resource_view();
}
