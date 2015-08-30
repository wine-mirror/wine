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

START_TEST(d3d11)
{
    test_create_device();
    test_device_interfaces();
    test_create_texture2d();
}
