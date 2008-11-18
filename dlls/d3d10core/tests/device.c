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

HRESULT WINAPI D3D10CoreCreateDevice(IDXGIFactory *factory, IDXGIAdapter *adapter,
        UINT flags, DWORD unknown0, ID3D10Device **device);

static ID3D10Device *create_device(void)
{
    IDXGIFactory *factory = NULL;
    IDXGIAdapter *adapter = NULL;
    ID3D10Device *device = NULL;
    HRESULT hr;

    hr = CreateDXGIFactory(&IID_IDXGIFactory, (void *)&factory);
    ok(SUCCEEDED(hr), "CreateDXGIFactory failed, hr %#x\n", hr);
    if (FAILED(hr)) goto cleanup;

    hr = IDXGIFactory_EnumAdapters(factory, 0, &adapter);
    ok(SUCCEEDED(hr), "EnumAdapters failed, hr %#x\n", hr);
    if (FAILED(hr)) goto cleanup;

    hr = D3D10CoreCreateDevice(factory, adapter, 0, 0, &device);
    if (FAILED(hr))
    {
        HMODULE d3d10ref;

        trace("Failed to create a HW device, trying REF\n");
        IDXGIAdapter_Release(adapter);
        adapter = NULL;

        d3d10ref = LoadLibraryA("d3d10ref.dll");
        if (!d3d10ref)
        {
            trace("d3d10ref.dll not available, unable to create a REF device\n");
            goto cleanup;
        }

        hr = IDXGIFactory_CreateSoftwareAdapter(factory, d3d10ref, &adapter);
        FreeLibrary(d3d10ref);
        ok(SUCCEEDED(hr), "CreateSoftwareAdapter failed, hr %#x\n", hr);
        if (FAILED(hr)) goto cleanup;

        hr = D3D10CoreCreateDevice(factory, adapter, 0, 0, &device);
        ok(SUCCEEDED(hr), "Failed to create a REF device, hr %#x\n", hr);
        if (FAILED(hr)) goto cleanup;
    }

cleanup:
    if (adapter) IDXGIAdapter_Release(adapter);
    if (factory) IDXGIFactory_Release(factory);

    return device;
}

static void test_device_interfaces(ID3D10Device *device)
{
    IUnknown *obj;
    HRESULT hr;

    if (SUCCEEDED(hr = ID3D10Device_QueryInterface(device, &IID_IUnknown, (void **)&obj)))
        IUnknown_Release(obj);
    ok(SUCCEEDED(hr), "ID3D10Device does not implement IUnknown\n");

    if (SUCCEEDED(hr = ID3D10Device_QueryInterface(device, &IID_IDXGIObject, (void **)&obj)))
        IUnknown_Release(obj);
    ok(SUCCEEDED(hr), "ID3D10Device does not implement IDXGIObject\n");

    if (SUCCEEDED(hr = ID3D10Device_QueryInterface(device, &IID_IDXGIDevice, (void **)&obj)))
        IUnknown_Release(obj);
    ok(SUCCEEDED(hr), "ID3D10Device does not implement IDXGIDevice\n");

    if (SUCCEEDED(hr = ID3D10Device_QueryInterface(device, &IID_ID3D10Device, (void **)&obj)))
        IUnknown_Release(obj);
    ok(SUCCEEDED(hr), "ID3D10Device does not implement ID3D10Device\n");
}

START_TEST(device)
{
    ID3D10Device *device;
    ULONG refcount;

    device = create_device();
    if (!device)
    {
        skip("Failed to create device, skipping tests\n");
        return;
    }

    test_device_interfaces(device);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left\n", refcount);
}
