/*
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

static ID3D11Device *create_device(D3D_FEATURE_LEVEL feature_level)
{
    ID3D11Device *device;

    if (SUCCEEDED(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, &feature_level, 1, D3D11_SDK_VERSION,
            &device, NULL, NULL)))
        return device;
    if (SUCCEEDED(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_WARP, NULL, 0, &feature_level, 1, D3D11_SDK_VERSION,
            &device, NULL, NULL)))
        return device;
    if (SUCCEEDED(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_REFERENCE, NULL, 0, &feature_level, 1, D3D11_SDK_VERSION,
            &device, NULL, NULL)))
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
        if (!(device = create_device(feature_levels[i])))
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

START_TEST(d3d11)
{
    test_create_device();
    test_device_interfaces();
}
