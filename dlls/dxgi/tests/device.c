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

HRESULT WINAPI DXGID3D10CreateDevice(HMODULE d3d10core, IDXGIFactory *factory,
        IDXGIAdapter *adapter, UINT flags, void *unknown0, void **device);

static IDXGIDevice *create_device(HMODULE d3d10core)
{
    IDXGIDevice *dxgi_device = NULL;
    IDXGIFactory *factory = NULL;
    IDXGIAdapter *adapter = NULL;
    IUnknown *device = NULL;
    HRESULT hr;

    hr = CreateDXGIFactory(&IID_IDXGIFactory, (void *)&factory);
    if (FAILED(hr)) goto cleanup;

    hr = IDXGIFactory_EnumAdapters(factory, 0, &adapter);
    if (SUCCEEDED(hr))
    {
        hr = DXGID3D10CreateDevice(d3d10core, factory, adapter, 0, NULL, (void **)&device);
    }

    if (FAILED(hr))
    {
        HMODULE d3d10ref;

        trace("Failed to create a HW device, trying REF\n");
        if (adapter) IDXGIAdapter_Release(adapter);
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

        hr = DXGID3D10CreateDevice(d3d10core, factory, adapter, 0, NULL, (void **)&device);
        ok(SUCCEEDED(hr), "Failed to create a REF device, hr %#x\n", hr);
        if (FAILED(hr)) goto cleanup;
    }

    hr = IUnknown_QueryInterface(device, &IID_IDXGIDevice, (void **)&dxgi_device);
    ok(SUCCEEDED(hr), "Created device does not implement IDXGIDevice\n");
    IUnknown_Release(device);

cleanup:
    if (adapter) IDXGIAdapter_Release(adapter);
    if (factory) IDXGIFactory_Release(factory);

    return dxgi_device;
}

static void test_device_interfaces(IDXGIDevice *device)
{
    IUnknown *obj;
    HRESULT hr;

    if (SUCCEEDED(hr = IDXGIDevice_QueryInterface(device, &IID_IUnknown, (void **)&obj)))
        IUnknown_Release(obj);
    ok(SUCCEEDED(hr), "IDXGIDevice does not implement IUnknown\n");

    if (SUCCEEDED(hr = IDXGIDevice_QueryInterface(device, &IID_IDXGIObject, (void **)&obj)))
        IUnknown_Release(obj);
    ok(SUCCEEDED(hr), "IDXGIDevice does not implement IDXGIObject\n");

    if (SUCCEEDED(hr = IDXGIDevice_QueryInterface(device, &IID_IDXGIDevice, (void **)&obj)))
        IUnknown_Release(obj);
    ok(SUCCEEDED(hr), "IDXGIDevice does not implement IDXGIDevice\n");

    if (SUCCEEDED(hr = IDXGIDevice_QueryInterface(device, &IID_ID3D10Device, (void **)&obj)))
        IUnknown_Release(obj);
    ok(SUCCEEDED(hr), "IDXGIDevice does not implement ID3D10Device\n");
}

static void test_adapter_desc(IDXGIDevice *device)
{
    DXGI_ADAPTER_DESC desc;
    IDXGIAdapter *adapter;
    HRESULT hr;

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(SUCCEEDED(hr), "GetAdapter failed, hr %#x.\n", hr);

    hr = IDXGIAdapter_GetDesc(adapter, NULL);
    ok(hr == E_INVALIDARG, "GetDesc returned %#x, expected %#x.\n",
            hr, E_INVALIDARG);

    hr = IDXGIAdapter_GetDesc(adapter, &desc);
    ok(SUCCEEDED(hr), "GetDesc failed, hr %#x.\n", hr);

    trace("%s.\n", wine_dbgstr_w(desc.Description));
    trace("%04x: %04x:%04x (rev %02x).\n",
            desc.SubSysId, desc.VendorId, desc.DeviceId, desc.Revision);
    trace("Dedicated video memory: %lu (%lu MB).\n",
            desc.DedicatedVideoMemory, desc.DedicatedVideoMemory / (1024 * 1024));
    trace("Dedicated system memory: %lu (%lu MB).\n",
            desc.DedicatedSystemMemory, desc.DedicatedSystemMemory / (1024 * 1024));
    trace("Shared system memory: %lu (%lu MB).\n",
            desc.SharedSystemMemory, desc.SharedSystemMemory / (1024 * 1024));
    trace("LUID: %08x:%08x.\n", desc.AdapterLuid.HighPart, desc.AdapterLuid.LowPart);

    IDXGIAdapter_Release(adapter);
}

static void test_create_surface(IDXGIDevice *device)
{
    ID3D10Texture2D *texture;
    IDXGISurface *surface;
    DXGI_SURFACE_DESC desc;
    HRESULT hr;

    desc.Width = 512;
    desc.Height = 512;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    hr = IDXGIDevice_CreateSurface(device, &desc, 1, DXGI_USAGE_RENDER_TARGET_OUTPUT, NULL, &surface);
    ok(SUCCEEDED(hr), "Failed to create a dxgi surface, hr %#x\n", hr);

    hr = IDXGISurface_QueryInterface(surface, &IID_ID3D10Texture2D, (void **)&texture);
    ok(SUCCEEDED(hr), "Surface should implement ID3D10Texture2D\n");
    if (SUCCEEDED(hr)) ID3D10Texture2D_Release(texture);

    IDXGISurface_Release(surface);
}

static void test_parents(IDXGIDevice *device)
{
    DXGI_SURFACE_DESC surface_desc;
    IDXGISurface *surface;
    IDXGIFactory *factory;
    IDXGIAdapter *adapter;
    IDXGIOutput *output;
    IUnknown *parent;
    HRESULT hr;

    surface_desc.Width = 512;
    surface_desc.Height = 512;
    surface_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    surface_desc.SampleDesc.Count = 1;
    surface_desc.SampleDesc.Quality = 0;

    hr = IDXGIDevice_CreateSurface(device, &surface_desc, 1, DXGI_USAGE_RENDER_TARGET_OUTPUT, NULL, &surface);
    ok(SUCCEEDED(hr), "Failed to create a dxgi surface, hr %#x\n", hr);

    hr = IDXGISurface_GetParent(surface, &IID_IDXGIDevice, (void **)&parent);
    IDXGISurface_Release(surface);
    ok(SUCCEEDED(hr), "GetParent failed, hr %#x.\n", hr);
    ok(parent == (IUnknown *)device, "Got parent %p, expected %p.\n", parent, device);
    IUnknown_Release(parent);

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(SUCCEEDED(hr), "GetAdapter failed, hr %#x.\n", hr);

    hr = IDXGIAdapter_EnumOutputs(adapter, 0, &output);
    if (hr == DXGI_ERROR_NOT_FOUND)
    {
        skip("Adapter has not outputs, skipping output tests.\n");
    }
    else
    {
        ok(SUCCEEDED(hr), "EnumOutputs failed, hr %#x.\n", hr);

        hr = IDXGIOutput_GetParent(output, &IID_IDXGIAdapter, (void **)&parent);
        IDXGIOutput_Release(output);
        ok(SUCCEEDED(hr), "GetParent failed, hr %#x.\n", hr);
        ok(parent == (IUnknown *)adapter, "Got parent %p, expected %p.\n", parent, adapter);
        IUnknown_Release(parent);
    }

    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(SUCCEEDED(hr), "GetParent failed, hr %#x.\n", hr);

    hr = IDXGIFactory_GetParent(factory, &IID_IUnknown, (void **)&parent);
    ok(hr == E_NOINTERFACE, "GetParent returned %#x, expected %#x.\n", hr, E_NOINTERFACE);
    ok(parent == NULL, "Got parent %p, expected %p.\n", parent, NULL);
    IDXGIFactory_Release(factory);

    hr = IDXGIDevice_GetParent(device, &IID_IDXGIAdapter, (void **)&parent);
    ok(SUCCEEDED(hr), "GetParent failed, hr %#x.\n", hr);
    ok(parent == (IUnknown *)adapter, "Got parent %p, expected %p.\n", parent, adapter);
    IUnknown_Release(parent);

    IDXGIAdapter_Release(adapter);
}

static void test_output(IDXGIDevice *device)
{
    IDXGIAdapter *adapter;
    HRESULT hr;
    IDXGIOutput *output;
    UINT mode_count, mode_count_comp, i;
    DXGI_MODE_DESC *modes;

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(SUCCEEDED(hr), "GetAdapter failed, hr %#x.\n", hr);

    hr = IDXGIAdapter_EnumOutputs(adapter, 0, &output);
    if (hr == DXGI_ERROR_NOT_FOUND)
    {
        skip("Adapter has not outputs, skipping output tests.\n");
        IDXGIAdapter_Release(adapter);
        return;
    }

    ok(SUCCEEDED(hr), "EnumOutputs failed, hr %#x.\n", hr);

    IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM, 0, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to list modes, hr %#x.\n", hr);

    IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM, 0, &mode_count, NULL);
    ok(SUCCEEDED(hr), "Failed to list modes, hr %#x.\n", hr);
    mode_count_comp = mode_count;

    IDXGIOutput_GetDisplayModeList(output, 0, 0, &mode_count, NULL);
    ok(SUCCEEDED(hr), "Failed to list modes, hr %#x.\n", hr);
    ok(!mode_count, "Expected 0 got %d\n", mode_count);

    IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_SCALING, &mode_count, NULL);
    ok(SUCCEEDED(hr), "Failed to list modes, hr %#x.\n", hr);
    ok(mode_count >= mode_count_comp, "Flag implies trying to enumerate more modes\n");
    mode_count_comp = mode_count;

    modes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DXGI_MODE_DESC) * mode_count+10);

    IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_SCALING, NULL, modes);
    ok(SUCCEEDED(hr), "Failed to list modes, hr %#x.\n", hr);
    ok(!modes[0].Height, "No output was expected\n");

    mode_count = 0;
    IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_SCALING, &mode_count, modes);
    ok(SUCCEEDED(hr), "Failed to list modes, hr %#x.\n", hr);
    ok(!modes[0].Height, "No output was expected\n");

    mode_count = mode_count_comp;
    IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_SCALING, &mode_count, modes);
    ok(SUCCEEDED(hr), "Failed to list modes, hr %#x.\n", hr);
    ok(mode_count == mode_count_comp, "Expected %d, got %d\n", mode_count_comp, mode_count);

    for (i = 0; i < mode_count; i++)
    {
        ok(modes[i].Height && modes[i].Width, "Proper mode was expected\n");
    }

    mode_count += 5;
    IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_SCALING, &mode_count, modes);
    ok(SUCCEEDED(hr), "Failed to list modes, hr %#x.\n", hr);
    ok(mode_count == mode_count_comp, "Expected %d, got %d\n", mode_count_comp, mode_count);

    if (mode_count_comp)
    {
        mode_count = mode_count_comp - 1;
        IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_SCALING, &mode_count, modes);
        ok(SUCCEEDED(hr), "Failed to list modes, hr %#x.\n", hr);
        ok(mode_count == mode_count_comp -1, "Expected %d, got %d\n", mode_count_comp, mode_count);
    }
    else
    {
        skip("Not enough modes for test, skipping\n");
    }

    HeapFree(GetProcessHeap(), 0, modes);
    IDXGIOutput_Release(output);
    IDXGIAdapter_Release(adapter);
}

struct refresh_rates
{
    UINT numerator;
    UINT denominator;
    BOOL numerator_should_pass;
    BOOL denominator_should_pass;
};

static void test_createswapchain(IDXGIDevice *device)
{
    IUnknown *obj;
    IDXGIAdapter *adapter;
    IDXGIFactory *factory;
    IDXGISwapChain *swapchain;
    DXGI_SWAP_CHAIN_DESC creation_desc, result_desc;
    HRESULT hr;
    WNDCLASSA wc = {0};
    UINT i;

    const struct refresh_rates refresh_list[] =
    {
        {60, 60, FALSE, FALSE},
        {60,  0,  TRUE, FALSE},
        {60,  1,  TRUE,  TRUE},
        { 0, 60,  TRUE, FALSE},
        { 0,  0,  TRUE, FALSE},
    };


    wc.lpfnWndProc = DefWindowProcA;
    wc.lpszClassName = "dxgi_test_wc";

    RegisterClassA(&wc);

    creation_desc.OutputWindow = 0;
    creation_desc.BufferDesc.Width = 800;
    creation_desc.BufferDesc.Height = 600;
    creation_desc.BufferDesc.RefreshRate.Numerator = 60;
    creation_desc.BufferDesc.RefreshRate.Denominator = 60;
    creation_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    creation_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    creation_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    creation_desc.SampleDesc.Count = 1;
    creation_desc.SampleDesc.Quality = 0;
    creation_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    creation_desc.BufferCount = 1;
    creation_desc.OutputWindow = CreateWindowA("dxgi_test_wc", "dxgi_test", 0, 0, 0, 0, 0, 0, 0, 0, 0);
    creation_desc.Windowed = TRUE;
    creation_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    creation_desc.Flags = 0;

    hr = IDXGIDevice_QueryInterface(device, &IID_IUnknown, (void **)&obj);
    ok(SUCCEEDED(hr), "IDXGIDevice does not implement IUnknown\n");

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(SUCCEEDED(hr), "GetAdapter failed, hr %#x.\n", hr);

    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(SUCCEEDED(hr), "GetParent failed, hr %#x.\n", hr);

    hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
    ok(SUCCEEDED(hr), "CreateSwapChain failed, hr %#x.\n", hr);

    hr = IDXGISwapChain_GetDesc(swapchain, NULL);
    ok(hr == E_INVALIDARG, "GetDesc unexpectedly returned %#x.\n", hr);

    IDXGISwapChain_Release(swapchain);

    for (i = 0; i < sizeof(refresh_list)/sizeof(refresh_list[0]); i++)
    {
        creation_desc.BufferDesc.RefreshRate.Numerator = refresh_list[i].numerator;
        creation_desc.BufferDesc.RefreshRate.Denominator = refresh_list[i].denominator;

        hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
        ok(SUCCEEDED(hr), "CreateSwapChain failed, hr %#x.\n", hr);

        hr = IDXGISwapChain_GetDesc(swapchain, &result_desc);
        ok(SUCCEEDED(hr), "GetDesc failed, hr %#x.\n", hr);

        if (refresh_list[i].numerator_should_pass)
            ok(result_desc.BufferDesc.RefreshRate.Numerator == refresh_list[i].numerator,
                "Numerator %u is %u.\n", i, result_desc.BufferDesc.RefreshRate.Numerator);
        else
            todo_wine ok(result_desc.BufferDesc.RefreshRate.Numerator == refresh_list[i].numerator,
                "Numerator %u is %u.\n", i, result_desc.BufferDesc.RefreshRate.Numerator);

        if (refresh_list[i].denominator_should_pass)
            ok(result_desc.BufferDesc.RefreshRate.Denominator == refresh_list[i].denominator,
                    "Denominator %u is %u.\n", i ,result_desc.BufferDesc.RefreshRate.Denominator);
        else
            todo_wine ok(result_desc.BufferDesc.RefreshRate.Denominator == refresh_list[i].denominator,
                    "Denominator %u is %u.\n", i, result_desc.BufferDesc.RefreshRate.Denominator);

        IDXGISwapChain_Release(swapchain);
    }

    creation_desc.Windowed = FALSE;

    for (i = 0; i < sizeof(refresh_list)/sizeof(refresh_list[0]); i++)
    {
        creation_desc.BufferDesc.RefreshRate.Numerator = refresh_list[i].numerator;
        creation_desc.BufferDesc.RefreshRate.Denominator = refresh_list[i].denominator;

        hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
        ok(SUCCEEDED(hr), "CreateSwapChain failed, hr %#x.\n", hr);

        hr = IDXGISwapChain_GetDesc(swapchain, &result_desc);
        ok(SUCCEEDED(hr), "GetDesc failed, hr %#x.\n", hr);

        if (refresh_list[i].numerator_should_pass)
            ok(result_desc.BufferDesc.RefreshRate.Numerator == refresh_list[i].numerator,
                    "Numerator %u is %u.\n", i, result_desc.BufferDesc.RefreshRate.Numerator);
        else
            todo_wine ok(result_desc.BufferDesc.RefreshRate.Numerator == refresh_list[i].numerator,
                    "Numerator %u is %u.\n", i, result_desc.BufferDesc.RefreshRate.Numerator);

        if (refresh_list[i].denominator_should_pass)
            ok(result_desc.BufferDesc.RefreshRate.Denominator == refresh_list[i].denominator,
                    "Denominator %u is %u.\n", i ,result_desc.BufferDesc.RefreshRate.Denominator);
        else
            todo_wine ok(result_desc.BufferDesc.RefreshRate.Denominator == refresh_list[i].denominator,
                    "Denominator %u is %u.\n", i, result_desc.BufferDesc.RefreshRate.Denominator);

        hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
        todo_wine ok(SUCCEEDED(hr), "SetFullscreenState failed, hr %#x.\n", hr);

        IDXGISwapChain_Release(swapchain);
    }

    IDXGIFactory_Release(factory);
    IDXGIAdapter_Release(adapter);
    IUnknown_Release(obj);
}

START_TEST(device)
{
    HMODULE d3d10core = LoadLibraryA("d3d10core.dll");
    IDXGIDevice *device;
    ULONG refcount;

    if (!d3d10core)
    {
        win_skip("d3d10core.dll not available, skipping tests\n");
        return;
    }

    device = create_device(d3d10core);
    if (!device)
    {
        skip("Failed to create device, skipping tests\n");
        FreeLibrary(d3d10core);
        return;
    }

    test_adapter_desc(device);
    test_device_interfaces(device);
    test_create_surface(device);
    test_parents(device);
    test_output(device);
    test_createswapchain(device);

    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left\n", refcount);
    FreeLibrary(d3d10core);
}
