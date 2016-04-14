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
#include "d3d11.h"
#include "wine/test.h"

enum frame_latency
{
    DEFAULT_FRAME_LATENCY =  3,
    MAX_FRAME_LATENCY     = 16,
};

static DEVMODEW registry_mode;

static HRESULT (WINAPI *pCreateDXGIFactory1)(REFIID iid, void **factory);

static ULONG get_refcount(IUnknown *iface)
{
    IUnknown_AddRef(iface);
    return IUnknown_Release(iface);
}

static IDXGIDevice *create_device(void)
{
    IDXGIDevice *dxgi_device;
    ID3D10Device *device;
    HRESULT hr;

    if (SUCCEEDED(D3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &device)))
        goto success;
    if (SUCCEEDED(D3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_WARP, NULL, 0, D3D10_SDK_VERSION, &device)))
        goto success;
    if (SUCCEEDED(D3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_REFERENCE, NULL, 0, D3D10_SDK_VERSION, &device)))
        goto success;

    return NULL;

success:
    hr = ID3D10Device_QueryInterface(device, &IID_IDXGIDevice, (void **)&dxgi_device);
    ok(SUCCEEDED(hr), "Created device does not implement IDXGIDevice\n");
    ID3D10Device_Release(device);

    return dxgi_device;
}

static void test_adapter_desc(void)
{
    DXGI_ADAPTER_DESC1 desc1;
    IDXGIAdapter1 *adapter1;
    DXGI_ADAPTER_DESC desc;
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

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

    hr = IDXGIAdapter_QueryInterface(adapter, &IID_IDXGIAdapter1, (void **)&adapter1);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE), "Got unexpected hr %#x.\n", hr);
    if (hr == E_NOINTERFACE)
        goto done;

    hr = IDXGIAdapter1_GetDesc1(adapter1, &desc1);
    ok(SUCCEEDED(hr), "GetDesc1 failed, hr %#x.\n", hr);

    ok(!lstrcmpW(desc.Description, desc1.Description),
            "Got unexpected description %s.\n", wine_dbgstr_w(desc1.Description));
    ok(desc1.VendorId == desc.VendorId, "Got unexpected vendor ID %04x.\n", desc1.VendorId);
    ok(desc1.DeviceId == desc.DeviceId, "Got unexpected device ID %04x.\n", desc1.DeviceId);
    ok(desc1.SubSysId == desc.SubSysId, "Got unexpected sub system ID %04x.\n", desc1.SubSysId);
    ok(desc1.Revision == desc.Revision, "Got unexpected revision %02x.\n", desc1.Revision);
    ok(desc1.DedicatedVideoMemory == desc.DedicatedVideoMemory,
            "Got unexpected dedicated video memory %lu.\n", desc1.DedicatedVideoMemory);
    ok(desc1.DedicatedSystemMemory == desc.DedicatedSystemMemory,
            "Got unexpected dedicated system memory %lu.\n", desc1.DedicatedSystemMemory);
    ok(desc1.SharedSystemMemory == desc.SharedSystemMemory,
            "Got unexpected shared system memory %lu.\n", desc1.SharedSystemMemory);
    ok(!memcmp(&desc.AdapterLuid, &desc1.AdapterLuid, sizeof(desc.AdapterLuid)),
            "Got unexpected adapter LUID %08x:%08x.\n", desc1.AdapterLuid.HighPart, desc1.AdapterLuid.LowPart);
    trace("Flags: %08x.\n", desc1.Flags);

    IDXGIAdapter1_Release(adapter1);

done:
    IDXGIAdapter_Release(adapter);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_check_interface_support(void)
{
    LARGE_INTEGER driver_version;
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    IUnknown *iface;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(SUCCEEDED(hr), "GetAdapter failed, hr %#x.\n", hr);

    hr = IDXGIAdapter_CheckInterfaceSupport(adapter, &IID_ID3D10Device, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDXGIAdapter_CheckInterfaceSupport(adapter, &IID_ID3D10Device, &driver_version);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    trace("UMD version: %u.%u.%u.%u.\n",
            HIWORD(U(driver_version).HighPart), LOWORD(U(driver_version).HighPart),
            HIWORD(U(driver_version).LowPart), LOWORD(U(driver_version).LowPart));

    hr = IDXGIDevice_QueryInterface(device, &IID_ID3D10Device1, (void **)&iface);
    if (SUCCEEDED(hr))
    {
        IUnknown_Release(iface);
        hr = IDXGIAdapter_CheckInterfaceSupport(adapter, &IID_ID3D10Device1, NULL);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        hr = IDXGIAdapter_CheckInterfaceSupport(adapter, &IID_ID3D10Device1, &driver_version);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    }
    else
    {
        win_skip("D3D10.1 is not supported.\n");
    }

    hr = IDXGIAdapter_CheckInterfaceSupport(adapter, &IID_ID3D11Device, NULL);
    ok(hr == DXGI_ERROR_UNSUPPORTED, "Got unexpected hr %#x.\n", hr);
    driver_version.HighPart = driver_version.LowPart = 0xdeadbeef;
    hr = IDXGIAdapter_CheckInterfaceSupport(adapter, &IID_ID3D11Device, &driver_version);
    ok(hr == DXGI_ERROR_UNSUPPORTED, "Got unexpected hr %#x.\n", hr);
    ok(driver_version.HighPart == 0xdeadbeef, "Got unexpected driver version %#x.\n", driver_version.HighPart);
    ok(driver_version.LowPart == 0xdeadbeef, "Got unexpected driver version %#x.\n", driver_version.LowPart);

    IDXGIAdapter_Release(adapter);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_surface(void)
{
    DXGI_SURFACE_DESC desc;
    IDXGISurface *surface;
    IDXGIDevice *device;
    IUnknown *surface1;
    IUnknown *texture;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    desc.Width = 512;
    desc.Height = 512;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    hr = IDXGIDevice_CreateSurface(device, &desc, 1, DXGI_USAGE_RENDER_TARGET_OUTPUT, NULL, &surface);
    ok(SUCCEEDED(hr), "Failed to create a dxgi surface, hr %#x\n", hr);

    hr = IDXGISurface_QueryInterface(surface, &IID_ID3D10Texture2D, (void **)&texture);
    ok(SUCCEEDED(hr), "Surface should implement ID3D10Texture2D\n");
    IUnknown_Release(texture);

    hr = IDXGISurface_QueryInterface(surface, &IID_ID3D11Texture2D, (void **)&texture);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Surface should implement ID3D11Texture2D.\n");
    if (SUCCEEDED(hr)) IUnknown_Release(texture);

    hr = IDXGISurface_QueryInterface(surface, &IID_IDXGISurface1, (void **)&surface1);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Surface should implement IDXGISurface1.\n");
    if (SUCCEEDED(hr)) IUnknown_Release(surface1);

    IDXGISurface_Release(surface);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_parents(void)
{
    DXGI_SURFACE_DESC surface_desc;
    IDXGISurface *surface;
    IDXGIFactory *factory;
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    IDXGIOutput *output;
    IUnknown *parent;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

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
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_output(void)
{
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    HRESULT hr;
    IDXGIOutput *output;
    ULONG refcount;
    UINT mode_count, mode_count_comp, i;
    DXGI_MODE_DESC *modes;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(SUCCEEDED(hr), "GetAdapter failed, hr %#x.\n", hr);

    hr = IDXGIAdapter_EnumOutputs(adapter, 0, &output);
    if (hr == DXGI_ERROR_NOT_FOUND)
    {
        skip("Adapter doesn't have any outputs, skipping tests.\n");
        IDXGIAdapter_Release(adapter);
        IDXGIDevice_Release(device);
        return;
    }
    ok(SUCCEEDED(hr), "EnumOutputs failed, hr %#x.\n", hr);

    hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM, 0, NULL, NULL);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);

    hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM, 0, &mode_count, NULL);
    ok(SUCCEEDED(hr)
            || broken(hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE), /* Remote Desktop Services / Win 7 testbot */
            "Failed to list modes, hr %#x.\n", hr);
    if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
    {
        skip("GetDisplayModeList() not supported, skipping tests.\n");
        IDXGIOutput_Release(output);
        IDXGIAdapter_Release(adapter);
        IDXGIDevice_Release(device);
        return;
    }
    mode_count_comp = mode_count;

    hr = IDXGIOutput_GetDisplayModeList(output, 0, 0, &mode_count, NULL);
    ok(SUCCEEDED(hr), "Failed to list modes, hr %#x.\n", hr);
    ok(!mode_count, "Got unexpected mode_count %u.\n", mode_count);

    hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_ENUM_MODES_SCALING, &mode_count, NULL);
    ok(SUCCEEDED(hr), "Failed to list modes, hr %#x.\n", hr);
    ok(mode_count >= mode_count_comp, "Got unexpected mode_count %u, expected >= %u.\n", mode_count, mode_count_comp);
    mode_count_comp = mode_count;

    modes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*modes) * (mode_count + 10));

    hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_ENUM_MODES_SCALING, NULL, modes);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);
    ok(!modes[0].Height, "No output was expected.\n");

    mode_count = 0;
    hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_ENUM_MODES_SCALING, &mode_count, modes);
    ok(hr == DXGI_ERROR_MORE_DATA, "Got unexpected hr %#x.\n", hr);
    ok(!modes[0].Height, "No output was expected.\n");

    mode_count = mode_count_comp;
    hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_ENUM_MODES_SCALING, &mode_count, modes);
    ok(SUCCEEDED(hr), "Failed to list modes, hr %#x.\n", hr);
    ok(mode_count == mode_count_comp, "Got unexpected mode_count %u, expected %u.\n", mode_count, mode_count_comp);

    for (i = 0; i < mode_count; i++)
    {
        ok(modes[i].Height && modes[i].Width, "Proper mode was expected\n");
    }

    mode_count += 5;
    hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_ENUM_MODES_SCALING, &mode_count, modes);
    ok(SUCCEEDED(hr), "Failed to list modes, hr %#x.\n", hr);
    ok(mode_count == mode_count_comp, "Got unexpected mode_count %u, expected %u.\n", mode_count, mode_count_comp);

    if (mode_count_comp)
    {
        mode_count = mode_count_comp - 1;
        hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM,
                DXGI_ENUM_MODES_SCALING, &mode_count, modes);
        ok(hr == DXGI_ERROR_MORE_DATA, "Got unexpected hr %#x.\n", hr);
        ok(mode_count == mode_count_comp - 1, "Got unexpected mode_count %u, expected %u.\n",
                mode_count, mode_count_comp - 1);
    }
    else
    {
        skip("Not enough modes for test, skipping.\n");
    }

    HeapFree(GetProcessHeap(), 0, modes);
    IDXGIOutput_Release(output);
    IDXGIAdapter_Release(adapter);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

struct refresh_rates
{
    UINT numerator;
    UINT denominator;
    BOOL numerator_should_pass;
    BOOL denominator_should_pass;
};

static void test_create_swapchain(void)
{
    DXGI_SWAP_CHAIN_DESC creation_desc, result_desc;
    ULONG refcount, expected_refcount;
    IDXGISwapChain *swapchain;
    IUnknown *obj, *parent;
    IDXGIAdapter *adapter;
    IDXGIFactory *factory;
    IDXGIDevice *device;
    HRESULT hr;
    UINT i;

    const struct refresh_rates refresh_list[] =
    {
        {60, 60, FALSE, FALSE},
        {60,  0,  TRUE, FALSE},
        {60,  1,  TRUE,  TRUE},
        { 0, 60,  TRUE, FALSE},
        { 0,  0,  TRUE, FALSE},
    };

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

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
    creation_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0, 0, 0, 0, 0, 0, 0, 0, 0);
    creation_desc.Windowed = TRUE;
    creation_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    creation_desc.Flags = 0;

    hr = IDXGIDevice_QueryInterface(device, &IID_IUnknown, (void **)&obj);
    ok(SUCCEEDED(hr), "IDXGIDevice does not implement IUnknown\n");

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(SUCCEEDED(hr), "GetAdapter failed, hr %#x.\n", hr);

    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(SUCCEEDED(hr), "GetParent failed, hr %#x.\n", hr);

    expected_refcount = get_refcount((IUnknown *)adapter);
    refcount = get_refcount((IUnknown *)factory);
    ok(refcount == 2, "Got unexpected refcount %u.\n", refcount);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == 2, "Got unexpected refcount %u.\n", refcount);

    hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
    ok(SUCCEEDED(hr), "CreateSwapChain failed, hr %#x.\n", hr);

    refcount = get_refcount((IUnknown *)adapter);
    ok(refcount == expected_refcount, "Got refcount %u, expected %u.\n", refcount, expected_refcount);
    refcount = get_refcount((IUnknown *)factory);
    todo_wine ok(refcount == 4, "Got unexpected refcount %u.\n", refcount);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == 3, "Got unexpected refcount %u.\n", refcount);

    hr = IDXGISwapChain_GetDesc(swapchain, NULL);
    ok(hr == E_INVALIDARG, "GetDesc unexpectedly returned %#x.\n", hr);

    hr = IDXGISwapChain_GetParent(swapchain, &IID_IUnknown, (void **)&parent);
    ok(SUCCEEDED(hr), "GetParent failed %#x.\n", hr);
    ok(parent == (IUnknown *)factory, "Got unexpected parent interface pointer %p.\n", parent);
    refcount = IUnknown_Release(parent);
    todo_wine ok(refcount == 4, "Got unexpected refcount %u.\n", refcount);

    hr = IDXGISwapChain_GetParent(swapchain, &IID_IDXGIFactory, (void **)&parent);
    ok(SUCCEEDED(hr), "GetParent failed %#x.\n", hr);
    ok(parent == (IUnknown *)factory, "Got unexpected parent interface pointer %p.\n", parent);
    refcount = IUnknown_Release(parent);
    todo_wine ok(refcount == 4, "Got unexpected refcount %u.\n", refcount);

    IDXGISwapChain_Release(swapchain);

    refcount = get_refcount((IUnknown *)factory);
    ok(refcount == 2, "Got unexpected refcount %u.\n", refcount);

    for (i = 0; i < sizeof(refresh_list)/sizeof(refresh_list[0]); i++)
    {
        creation_desc.BufferDesc.RefreshRate.Numerator = refresh_list[i].numerator;
        creation_desc.BufferDesc.RefreshRate.Denominator = refresh_list[i].denominator;

        hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
        ok(SUCCEEDED(hr), "CreateSwapChain failed, hr %#x.\n", hr);

        hr = IDXGISwapChain_GetDesc(swapchain, &result_desc);
        ok(SUCCEEDED(hr), "GetDesc failed, hr %#x.\n", hr);

        todo_wine_if (!refresh_list[i].numerator_should_pass)
            ok(result_desc.BufferDesc.RefreshRate.Numerator == refresh_list[i].numerator,
                "Numerator %u is %u.\n", i, result_desc.BufferDesc.RefreshRate.Numerator);

        todo_wine_if (!refresh_list[i].denominator_should_pass)
            ok(result_desc.BufferDesc.RefreshRate.Denominator == refresh_list[i].denominator,
                    "Denominator %u is %u.\n", i ,result_desc.BufferDesc.RefreshRate.Denominator);

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

        todo_wine_if (!refresh_list[i].numerator_should_pass)
            ok(result_desc.BufferDesc.RefreshRate.Numerator == refresh_list[i].numerator,
                    "Numerator %u is %u.\n", i, result_desc.BufferDesc.RefreshRate.Numerator);

        todo_wine_if (!refresh_list[i].denominator_should_pass)
            ok(result_desc.BufferDesc.RefreshRate.Denominator == refresh_list[i].denominator,
                    "Denominator %u is %u.\n", i ,result_desc.BufferDesc.RefreshRate.Denominator);

        hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
        todo_wine ok(SUCCEEDED(hr), "SetFullscreenState failed, hr %#x.\n", hr);

        IDXGISwapChain_Release(swapchain);
    }

    IUnknown_Release(obj);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    refcount = IDXGIAdapter_Release(adapter);
    ok(!refcount, "Adapter has %u references left.\n", refcount);
    refcount = IDXGIFactory_Release(factory);
    ok(!refcount, "Factory has %u references left.\n", refcount);
    DestroyWindow(creation_desc.OutputWindow);
}

static void test_get_containing_output(void)
{
    DXGI_OUTPUT_DESC output_desc, output_desc2;
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    IDXGIOutput *output, *output2;
    unsigned int output_count;
    IDXGISwapChain *swapchain;
    IDXGIFactory *factory;
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(SUCCEEDED(hr), "GetAdapter failed, hr %#x.\n", hr);

    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(SUCCEEDED(hr), "GetParent failed, hr %#x.\n", hr);

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
    swapchain_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0, 0, 0, 0, 0, 0, 0, 0, 0);
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;

    output_count = 0;
    while (IDXGIAdapter_EnumOutputs(adapter, output_count, &output) != DXGI_ERROR_NOT_FOUND)
    {
        ok(SUCCEEDED(hr), "Failed to enumarate output %u, hr %#x.\n", output_count, hr);
        IDXGIOutput_Release(output);
        ++output_count;
    }

    if (output_count != 1)
    {
        skip("Adapter has %u outputs.\n", output_count);
        goto done;
    }

    hr = IDXGIAdapter_EnumOutputs(adapter, 0, &output);
    ok(SUCCEEDED(hr), "EnumOutputs failed, hr %#x.\n", hr);

    refcount = get_refcount((IUnknown *)output);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);

    hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &swapchain_desc, &swapchain);
    ok(SUCCEEDED(hr), "CreateSwapChain failed, hr %#x.\n", hr);

    hr = IDXGISwapChain_GetContainingOutput(swapchain, &output2);
    ok(SUCCEEDED(hr) || broken(hr == DXGI_ERROR_UNSUPPORTED) /* Win 7 testbot */,
            "GetContainingOutput failed, hr %#x.\n", hr);
    if (hr == DXGI_ERROR_UNSUPPORTED)
    {
        IDXGISwapChain_Release(swapchain);
        IDXGIOutput_Release(output);
        goto done;
    }

    refcount = get_refcount((IUnknown *)output);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    refcount = get_refcount((IUnknown *)output2);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);

    hr = IDXGIOutput_GetDesc(output, &output_desc);
    ok(SUCCEEDED(hr), "GetDesc failed, hr %#x.\n", hr);
    hr = IDXGIOutput_GetDesc(output2, &output_desc2);
    ok(SUCCEEDED(hr), "GetDesc failed, hr %#x.\n", hr);

    ok(!lstrcmpW(output_desc.DeviceName, output_desc2.DeviceName),
            "Got unexpected device name %s, expected %s.\n",
            wine_dbgstr_w(output_desc.DeviceName), wine_dbgstr_w(output_desc2.DeviceName));
    ok(!memcmp(&output_desc.DesktopCoordinates, &output_desc2.DesktopCoordinates,
            sizeof(output_desc.DesktopCoordinates)),
            "Got unexpected desktop coordinates {%d, %d, %d, %d}, expected {%d, %d, %d, %d}.\n",
            output_desc.DesktopCoordinates.left, output_desc.DesktopCoordinates.top,
            output_desc.DesktopCoordinates.right, output_desc.DesktopCoordinates.bottom,
            output_desc2.DesktopCoordinates.left, output_desc2.DesktopCoordinates.top,
            output_desc2.DesktopCoordinates.right, output_desc2.DesktopCoordinates.bottom);

    refcount = IDXGIOutput_Release(output2);
    ok(!refcount, "IDXGIOuput has %u references left.\n", refcount);

    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %u references left.\n", refcount);

    refcount = IDXGIOutput_Release(output);
    ok(!refcount, "IDXGIOuput has %u references left.\n", refcount);

done:
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    refcount = IDXGIAdapter_Release(adapter);
    ok(!refcount, "Adapter has %u references left.\n", refcount);
    refcount = IDXGIFactory_Release(factory);
    ok(!refcount, "Factory has %u references left.\n", refcount);
    DestroyWindow(swapchain_desc.OutputWindow);
}

static void test_create_factory(void)
{
    IDXGIFactory1 *factory;
    IUnknown *iface;
    HRESULT hr;

    iface = (void *)0xdeadbeef;
    hr = CreateDXGIFactory(&IID_IDXGIDevice, (void **)&iface);
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#x.\n", hr);
    ok(!iface, "Got unexpected iface %p.\n", iface);

    hr = CreateDXGIFactory(&IID_IUnknown, (void **)&iface);
    ok(SUCCEEDED(hr), "Failed to create factory with IID_IUnknown, hr %#x.\n", hr);
    IUnknown_Release(iface);

    hr = CreateDXGIFactory(&IID_IDXGIObject, (void **)&iface);
    ok(SUCCEEDED(hr), "Failed to create factory with IID_IDXGIObject, hr %#x.\n", hr);
    IUnknown_Release(iface);

    factory = (void *)0xdeadbeef;
    hr = CreateDXGIFactory(&IID_IDXGIFactory, (void **)&iface);
    ok(SUCCEEDED(hr), "Failed to create factory with IID_IDXGIFactory, hr %#x.\n", hr);
    hr = IUnknown_QueryInterface(iface, &IID_IDXGIFactory1, (void **)&factory);
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#x.\n", hr);
    ok(!factory, "Got unexpected factory %p.\n", factory);
    IUnknown_Release(iface);

    iface = (void *)0xdeadbeef;
    hr = CreateDXGIFactory(&IID_IDXGIFactory1, (void **)&iface);
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#x.\n", hr);
    ok(!iface, "Got unexpected iface %p.\n", iface);

    if (!pCreateDXGIFactory1)
    {
        win_skip("CreateDXGIFactory1 not available, skipping tests.\n");
        return;
    }

    iface = (void *)0xdeadbeef;
    hr = pCreateDXGIFactory1(&IID_IDXGIDevice, (void **)&iface);
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#x.\n", hr);
    ok(!iface, "Got unexpected iface %p.\n", iface);

    hr = pCreateDXGIFactory1(&IID_IUnknown, (void **)&iface);
    ok(SUCCEEDED(hr), "Failed to create factory with IID_IUnknown, hr %#x.\n", hr);
    IUnknown_Release(iface);

    hr = pCreateDXGIFactory1(&IID_IDXGIObject, (void **)&iface);
    ok(SUCCEEDED(hr), "Failed to create factory with IID_IDXGIObject, hr %#x.\n", hr);
    IUnknown_Release(iface);

    hr = pCreateDXGIFactory1(&IID_IDXGIFactory, (void **)&iface);
    ok(SUCCEEDED(hr), "Failed to create factory with IID_IDXGIFactory, hr %#x.\n", hr);
    hr = IUnknown_QueryInterface(iface, &IID_IDXGIFactory1, (void **)&factory);
    ok(SUCCEEDED(hr), "Failed to query IDXGIFactory1 interface, hr %#x.\n", hr);
    IDXGIFactory1_Release(factory);
    IUnknown_Release(iface);

    hr = pCreateDXGIFactory1(&IID_IDXGIFactory1, (void **)&iface);
    ok(SUCCEEDED(hr), "Failed to create factory with IID_IDXGIFactory1, hr %#x.\n", hr);
    IUnknown_Release(iface);
}

static void test_private_data(void)
{
    ULONG refcount, expected_refcount;
    IDXGIDevice *device;
    HRESULT hr;
    IDXGIDevice *test_object;
    IUnknown *ptr;
    static const DWORD data[] = {1, 2, 3, 4};
    UINT size;
    static const GUID dxgi_private_data_test_guid =
    {
        0xfdb37466,
        0x428f,
        0x4edf,
        {0xa3, 0x7f, 0x9b, 0x1d, 0xf4, 0x88, 0xc5, 0xfc}
    };
    static const GUID dxgi_private_data_test_guid2 =
    {
        0x2e5afac2,
        0x87b5,
        0x4c10,
        {0x9b, 0x4b, 0x89, 0xd7, 0xd1, 0x12, 0xe7, 0x2b}
    };

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    test_object = create_device();

    /* SetPrivateData with a pointer of NULL has the purpose of FreePrivateData in previous
     * d3d versions. A successful clear returns S_OK. A redundant clear S_FALSE. Setting a
     * NULL interface is not considered a clear but as setting an interface pointer that
     * happens to be NULL. */
    hr = IDXGIDevice_SetPrivateData(device, &dxgi_private_data_test_guid, 0, NULL);
    ok(hr == S_FALSE, "Got unexpected hr %#x.\n", hr);
    hr = IDXGIDevice_SetPrivateDataInterface(device, &dxgi_private_data_test_guid, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDXGIDevice_SetPrivateData(device, &dxgi_private_data_test_guid, ~0U, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDXGIDevice_SetPrivateData(device, &dxgi_private_data_test_guid, ~0U, NULL);
    ok(hr == S_FALSE, "Got unexpected hr %#x.\n", hr);

    hr = IDXGIDevice_SetPrivateDataInterface(device, &dxgi_private_data_test_guid, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    size = sizeof(ptr) * 2;
    ptr = (IUnknown *)0xdeadbeef;
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid, &size, &ptr);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(!ptr, "Got unexpected pointer %p.\n", ptr);
    ok(size == sizeof(IUnknown *), "Got unexpected size %u.\n", size);

    refcount = get_refcount((IUnknown *)test_object);
    hr = IDXGIDevice_SetPrivateDataInterface(device, &dxgi_private_data_test_guid,
            (IUnknown *)test_object);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    expected_refcount = refcount + 1;
    refcount = get_refcount((IUnknown *)test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    hr = IDXGIDevice_SetPrivateDataInterface(device, &dxgi_private_data_test_guid,
            (IUnknown *)test_object);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);

    hr = IDXGIDevice_SetPrivateDataInterface(device, &dxgi_private_data_test_guid, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    expected_refcount--;
    refcount = get_refcount((IUnknown *)test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);

    hr = IDXGIDevice_SetPrivateDataInterface(device, &dxgi_private_data_test_guid,
            (IUnknown *)test_object);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    size = sizeof(data);
    hr = IDXGIDevice_SetPrivateData(device, &dxgi_private_data_test_guid, size, data);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    hr = IDXGIDevice_SetPrivateData(device, &dxgi_private_data_test_guid, 42, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDXGIDevice_SetPrivateData(device, &dxgi_private_data_test_guid, 42, NULL);
    ok(hr == S_FALSE, "Got unexpected hr %#x.\n", hr);

    hr = IDXGIDevice_SetPrivateDataInterface(device, &dxgi_private_data_test_guid,
            (IUnknown *)test_object);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    expected_refcount++;
    size = 2 * sizeof(ptr);
    ptr = NULL;
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid, &size, &ptr);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(size == sizeof(test_object), "Got unexpected size %u.\n", size);
    expected_refcount++;
    refcount = get_refcount((IUnknown *)test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    if (ptr)
        IUnknown_Release(ptr);
    expected_refcount--;

    ptr = (IUnknown *)0xdeadbeef;
    size = 1;
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid, &size, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(size == sizeof(device), "Got unexpected size %u.\n", size);
    size = 2 * sizeof(ptr);
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid, &size, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(size == sizeof(device), "Got unexpected size %u.\n", size);
    refcount = get_refcount((IUnknown *)test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);

    size = 1;
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid, &size, &ptr);
    ok(hr == DXGI_ERROR_MORE_DATA, "Got unexpected hr %#x.\n", hr);
    ok(size == sizeof(device), "Got unexpected size %u.\n", size);
    ok(ptr == (IUnknown *)0xdeadbeef, "Got unexpected pointer %p.\n", ptr);
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid2, NULL, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    size = 0xdeadbabe;
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid2, &size, &ptr);
    ok(hr == DXGI_ERROR_NOT_FOUND, "Got unexpected hr %#x.\n", hr);
    ok(size == 0, "Got unexpected size %u.\n", size);
    ok(ptr == (IUnknown *)0xdeadbeef, "Got unexpected pointer %p.\n", ptr);
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid, NULL, &ptr);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ok(ptr == (IUnknown *)0xdeadbeef, "Got unexpected pointer %p.\n", ptr);

    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    refcount = IDXGIDevice_Release(test_object);
    ok(!refcount, "Test object has %u references left.\n", refcount);
}

static void test_swapchain_resize(void)
{
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    D3D10_TEXTURE2D_DESC texture_desc;
    DXGI_SURFACE_DESC surface_desc;
    IDXGISwapChain *swapchain;
    ID3D10Texture2D *texture;
    IDXGISurface *surface;
    IDXGIAdapter *adapter;
    IDXGIFactory *factory;
    IDXGIDevice *device;
    RECT client_rect, r;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    BOOL ret;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }
    window = CreateWindowA("static", "dxgi_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    ret = GetClientRect(window, &client_rect);
    ok(ret, "Failed to get client rect.\n");

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(SUCCEEDED(hr), "Failed to get adapter, hr %#x.\n", hr);
    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(SUCCEEDED(hr), "Failed to get factory, hr %#x.\n", hr);
    IDXGIAdapter_Release(adapter);

    swapchain_desc.BufferDesc.Width = 640;
    swapchain_desc.BufferDesc.Height = 480;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 1;
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

    hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &swapchain_desc, &swapchain);
    ok(SUCCEEDED(hr), "Failed to create swapchain, hr %#x.\n", hr);
    IDXGIFactory_Release(factory);
    hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_IDXGISurface, (void **)&surface);
    ok(SUCCEEDED(hr), "Failed to get buffer, hr %#x.\n", hr);
    hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_ID3D10Texture2D, (void **)&texture);
    ok(SUCCEEDED(hr), "Failed to get buffer, hr %#x.\n", hr);

    ret = GetClientRect(window, &r);
    ok(ret, "Failed to get client rect.\n");
    ok(EqualRect(&r, &client_rect), "Got unexpected rect {%d, %d, %d, %d}, expected {%d, %d, %d, %d}.\n",
            r.left, r.top, r.right, r.bottom,
            client_rect.left, client_rect.top, client_rect.right, client_rect.bottom);

    hr = IDXGISwapChain_GetDesc(swapchain, &swapchain_desc);
    ok(SUCCEEDED(hr), "Failed to get swapchain desc, hr %#x.\n", hr);
    ok(swapchain_desc.BufferDesc.Width == 640,
            "Got unexpected BufferDesc.Width %u.\n", swapchain_desc.BufferDesc.Width);
    ok(swapchain_desc.BufferDesc.Height == 480,
            "Got unexpected bufferDesc.Height %u.\n", swapchain_desc.BufferDesc.Height);
    ok(swapchain_desc.BufferDesc.RefreshRate.Numerator == 60,
            "Got unexpected BufferDesc.RefreshRate.Numerator %u.\n",
            swapchain_desc.BufferDesc.RefreshRate.Numerator);
    ok(swapchain_desc.BufferDesc.RefreshRate.Denominator == 1,
            "Got unexpected BufferDesc.RefreshRate.Denominator %u.\n",
            swapchain_desc.BufferDesc.RefreshRate.Denominator);
    ok(swapchain_desc.BufferDesc.Format == DXGI_FORMAT_R8G8B8A8_UNORM,
            "Got unexpected BufferDesc.Format %#x.\n", swapchain_desc.BufferDesc.Format);
    ok(swapchain_desc.BufferDesc.ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
            "Got unexpected BufferDesc.ScanlineOrdering %#x.\n", swapchain_desc.BufferDesc.ScanlineOrdering);
    ok(swapchain_desc.BufferDesc.Scaling == DXGI_MODE_SCALING_UNSPECIFIED,
            "Got unexpected BufferDesc.Scaling %#x.\n", swapchain_desc.BufferDesc.Scaling);
    ok(swapchain_desc.SampleDesc.Count == 1,
            "Got unexpected SampleDesc.Count %u.\n", swapchain_desc.SampleDesc.Count);
    ok(!swapchain_desc.SampleDesc.Quality,
            "Got unexpected SampleDesc.Quality %u.\n", swapchain_desc.SampleDesc.Quality);
    ok(swapchain_desc.BufferUsage == DXGI_USAGE_RENDER_TARGET_OUTPUT,
            "Got unexpected BufferUsage %#x.\n", swapchain_desc.BufferUsage);
    ok(swapchain_desc.BufferCount == 1,
            "Got unexpected BufferCount %u.\n", swapchain_desc.BufferCount);
    ok(swapchain_desc.OutputWindow == window,
            "Got unexpected OutputWindow %p, expected %p.\n", swapchain_desc.OutputWindow, window);
    ok(swapchain_desc.Windowed,
            "Got unexpected Windowed %#x.\n", swapchain_desc.Windowed);
    ok(swapchain_desc.SwapEffect == DXGI_SWAP_EFFECT_DISCARD,
            "Got unexpected SwapEffect %#x.\n", swapchain_desc.SwapEffect);
    ok(!swapchain_desc.Flags,
            "Got unexpected Flags %#x.\n", swapchain_desc.Flags);

    hr = IDXGISurface_GetDesc(surface, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(surface_desc.Width == 640, "Got unexpected Width %u.\n", surface_desc.Width);
    ok(surface_desc.Height == 480, "Got unexpected Height %u.\n", surface_desc.Height);
    ok(surface_desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM, "Got unexpected Format %#x.\n", surface_desc.Format);
    ok(surface_desc.SampleDesc.Count == 1, "Got unexpected SampleDesc.Count %u.\n", surface_desc.SampleDesc.Count);
    ok(!surface_desc.SampleDesc.Quality, "Got unexpected SampleDesc.Quality %u.\n", surface_desc.SampleDesc.Quality);

    ID3D10Texture2D_GetDesc(texture, &texture_desc);
    ok(texture_desc.Width == 640, "Got unexpected Width %u.\n", texture_desc.Width);
    ok(texture_desc.Height == 480, "Got unexpected Height %u.\n", texture_desc.Height);
    ok(texture_desc.MipLevels == 1, "Got unexpected MipLevels %u.\n", texture_desc.MipLevels);
    ok(texture_desc.ArraySize == 1, "Got unexpected ArraySize %u.\n", texture_desc.ArraySize);
    ok(texture_desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM, "Got unexpected Format %#x.\n", texture_desc.Format);
    ok(texture_desc.SampleDesc.Count == 1, "Got unexpected SampleDesc.Count %u.\n", texture_desc.SampleDesc.Count);
    ok(!texture_desc.SampleDesc.Quality, "Got unexpected SampleDesc.Quality %u.\n", texture_desc.SampleDesc.Quality);
    ok(texture_desc.Usage == D3D10_USAGE_DEFAULT, "Got unexpected Usage %#x.\n", texture_desc.Usage);
    ok(texture_desc.BindFlags == D3D10_BIND_RENDER_TARGET, "Got unexpected BindFlags %#x.\n", texture_desc.BindFlags);
    ok(!texture_desc.CPUAccessFlags, "Got unexpected CPUAccessFlags %#x.\n", texture_desc.CPUAccessFlags);
    ok(!texture_desc.MiscFlags, "Got unexpected MiscFlags %#x.\n", texture_desc.MiscFlags);

    hr = IDXGISwapChain_ResizeBuffers(swapchain, 1, 320, 240, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 0);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);

    ret = GetClientRect(window, &r);
    ok(ret, "Failed to get client rect.\n");
    ok(EqualRect(&r, &client_rect), "Got unexpected rect {%d, %d, %d, %d}, expected {%d, %d, %d, %d}.\n",
            r.left, r.top, r.right, r.bottom,
            client_rect.left, client_rect.top, client_rect.right, client_rect.bottom);

    hr = IDXGISwapChain_GetDesc(swapchain, &swapchain_desc);
    ok(SUCCEEDED(hr), "Failed to get swapchain desc, hr %#x.\n", hr);
    ok(swapchain_desc.BufferDesc.Width == 640,
            "Got unexpected BufferDesc.Width %u.\n", swapchain_desc.BufferDesc.Width);
    ok(swapchain_desc.BufferDesc.Height == 480,
            "Got unexpected bufferDesc.Height %u.\n", swapchain_desc.BufferDesc.Height);
    ok(swapchain_desc.BufferDesc.RefreshRate.Numerator == 60,
            "Got unexpected BufferDesc.RefreshRate.Numerator %u.\n",
            swapchain_desc.BufferDesc.RefreshRate.Numerator);
    ok(swapchain_desc.BufferDesc.RefreshRate.Denominator == 1,
            "Got unexpected BufferDesc.RefreshRate.Denominator %u.\n",
            swapchain_desc.BufferDesc.RefreshRate.Denominator);
    ok(swapchain_desc.BufferDesc.Format == DXGI_FORMAT_R8G8B8A8_UNORM,
            "Got unexpected BufferDesc.Format %#x.\n", swapchain_desc.BufferDesc.Format);
    ok(swapchain_desc.BufferDesc.ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
            "Got unexpected BufferDesc.ScanlineOrdering %#x.\n", swapchain_desc.BufferDesc.ScanlineOrdering);
    ok(swapchain_desc.BufferDesc.Scaling == DXGI_MODE_SCALING_UNSPECIFIED,
            "Got unexpected BufferDesc.Scaling %#x.\n", swapchain_desc.BufferDesc.Scaling);
    ok(swapchain_desc.SampleDesc.Count == 1,
            "Got unexpected SampleDesc.Count %u.\n", swapchain_desc.SampleDesc.Count);
    ok(!swapchain_desc.SampleDesc.Quality,
            "Got unexpected SampleDesc.Quality %u.\n", swapchain_desc.SampleDesc.Quality);
    ok(swapchain_desc.BufferUsage == DXGI_USAGE_RENDER_TARGET_OUTPUT,
            "Got unexpected BufferUsage %#x.\n", swapchain_desc.BufferUsage);
    ok(swapchain_desc.BufferCount == 1,
            "Got unexpected BufferCount %u.\n", swapchain_desc.BufferCount);
    ok(swapchain_desc.OutputWindow == window,
            "Got unexpected OutputWindow %p, expected %p.\n", swapchain_desc.OutputWindow, window);
    ok(swapchain_desc.Windowed,
            "Got unexpected Windowed %#x.\n", swapchain_desc.Windowed);
    ok(swapchain_desc.SwapEffect == DXGI_SWAP_EFFECT_DISCARD,
            "Got unexpected SwapEffect %#x.\n", swapchain_desc.SwapEffect);
    ok(!swapchain_desc.Flags,
            "Got unexpected Flags %#x.\n", swapchain_desc.Flags);

    hr = IDXGISurface_GetDesc(surface, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(surface_desc.Width == 640, "Got unexpected Width %u.\n", surface_desc.Width);
    ok(surface_desc.Height == 480, "Got unexpected Height %u.\n", surface_desc.Height);
    ok(surface_desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM, "Got unexpected Format %#x.\n", surface_desc.Format);
    ok(surface_desc.SampleDesc.Count == 1, "Got unexpected SampleDesc.Count %u.\n", surface_desc.SampleDesc.Count);
    ok(!surface_desc.SampleDesc.Quality, "Got unexpected SampleDesc.Quality %u.\n", surface_desc.SampleDesc.Quality);

    ID3D10Texture2D_GetDesc(texture, &texture_desc);
    ok(texture_desc.Width == 640, "Got unexpected Width %u.\n", texture_desc.Width);
    ok(texture_desc.Height == 480, "Got unexpected Height %u.\n", texture_desc.Height);
    ok(texture_desc.MipLevels == 1, "Got unexpected MipLevels %u.\n", texture_desc.MipLevels);
    ok(texture_desc.ArraySize == 1, "Got unexpected ArraySize %u.\n", texture_desc.ArraySize);
    ok(texture_desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM, "Got unexpected Format %#x.\n", texture_desc.Format);
    ok(texture_desc.SampleDesc.Count == 1, "Got unexpected SampleDesc.Count %u.\n", texture_desc.SampleDesc.Count);
    ok(!texture_desc.SampleDesc.Quality, "Got unexpected SampleDesc.Quality %u.\n", texture_desc.SampleDesc.Quality);
    ok(texture_desc.Usage == D3D10_USAGE_DEFAULT, "Got unexpected Usage %#x.\n", texture_desc.Usage);
    ok(texture_desc.BindFlags == D3D10_BIND_RENDER_TARGET, "Got unexpected BindFlags %#x.\n", texture_desc.BindFlags);
    ok(!texture_desc.CPUAccessFlags, "Got unexpected CPUAccessFlags %#x.\n", texture_desc.CPUAccessFlags);
    ok(!texture_desc.MiscFlags, "Got unexpected MiscFlags %#x.\n", texture_desc.MiscFlags);

    ID3D10Texture2D_Release(texture);
    IDXGISurface_Release(surface);
    hr = IDXGISwapChain_ResizeBuffers(swapchain, 1, 320, 240, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 0);
    ok(SUCCEEDED(hr), "Failed to resize buffers, hr %#x.\n", hr);
    hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_IDXGISurface, (void **)&surface);
    ok(SUCCEEDED(hr), "Failed to get buffer, hr %#x.\n", hr);
    hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_ID3D10Texture2D, (void **)&texture);
    ok(SUCCEEDED(hr), "Failed to get buffer, hr %#x.\n", hr);

    ret = GetClientRect(window, &r);
    ok(ret, "Failed to get client rect.\n");
    ok(EqualRect(&r, &client_rect), "Got unexpected rect {%d, %d, %d, %d}, expected {%d, %d, %d, %d}.\n",
            r.left, r.top, r.right, r.bottom,
            client_rect.left, client_rect.top, client_rect.right, client_rect.bottom);

    hr = IDXGISwapChain_GetDesc(swapchain, &swapchain_desc);
    ok(SUCCEEDED(hr), "Failed to get swapchain desc, hr %#x.\n", hr);
    ok(swapchain_desc.BufferDesc.Width == 320,
            "Got unexpected BufferDesc.Width %u.\n", swapchain_desc.BufferDesc.Width);
    ok(swapchain_desc.BufferDesc.Height == 240,
            "Got unexpected bufferDesc.Height %u.\n", swapchain_desc.BufferDesc.Height);
    ok(swapchain_desc.BufferDesc.RefreshRate.Numerator == 60,
            "Got unexpected BufferDesc.RefreshRate.Numerator %u.\n",
            swapchain_desc.BufferDesc.RefreshRate.Numerator);
    ok(swapchain_desc.BufferDesc.RefreshRate.Denominator == 1,
            "Got unexpected BufferDesc.RefreshRate.Denominator %u.\n",
            swapchain_desc.BufferDesc.RefreshRate.Denominator);
    ok(swapchain_desc.BufferDesc.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            "Got unexpected BufferDesc.Format %#x.\n", swapchain_desc.BufferDesc.Format);
    ok(swapchain_desc.BufferDesc.ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
            "Got unexpected BufferDesc.ScanlineOrdering %#x.\n", swapchain_desc.BufferDesc.ScanlineOrdering);
    ok(swapchain_desc.BufferDesc.Scaling == DXGI_MODE_SCALING_UNSPECIFIED,
            "Got unexpected BufferDesc.Scaling %#x.\n", swapchain_desc.BufferDesc.Scaling);
    ok(swapchain_desc.SampleDesc.Count == 1,
            "Got unexpected SampleDesc.Count %u.\n", swapchain_desc.SampleDesc.Count);
    ok(!swapchain_desc.SampleDesc.Quality,
            "Got unexpected SampleDesc.Quality %u.\n", swapchain_desc.SampleDesc.Quality);
    ok(swapchain_desc.BufferUsage == DXGI_USAGE_RENDER_TARGET_OUTPUT,
            "Got unexpected BufferUsage %#x.\n", swapchain_desc.BufferUsage);
    ok(swapchain_desc.BufferCount == 1,
            "Got unexpected BufferCount %u.\n", swapchain_desc.BufferCount);
    ok(swapchain_desc.OutputWindow == window,
            "Got unexpected OutputWindow %p, expected %p.\n", swapchain_desc.OutputWindow, window);
    ok(swapchain_desc.Windowed,
            "Got unexpected Windowed %#x.\n", swapchain_desc.Windowed);
    ok(swapchain_desc.SwapEffect == DXGI_SWAP_EFFECT_DISCARD,
            "Got unexpected SwapEffect %#x.\n", swapchain_desc.SwapEffect);
    ok(!swapchain_desc.Flags,
            "Got unexpected Flags %#x.\n", swapchain_desc.Flags);

    hr = IDXGISurface_GetDesc(surface, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(surface_desc.Width == 320, "Got unexpected Width %u.\n", surface_desc.Width);
    ok(surface_desc.Height == 240, "Got unexpected Height %u.\n", surface_desc.Height);
    ok(surface_desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, "Got unexpected Format %#x.\n", surface_desc.Format);
    ok(surface_desc.SampleDesc.Count == 1, "Got unexpected SampleDesc.Count %u.\n", surface_desc.SampleDesc.Count);
    ok(!surface_desc.SampleDesc.Quality, "Got unexpected SampleDesc.Quality %u.\n", surface_desc.SampleDesc.Quality);

    ID3D10Texture2D_GetDesc(texture, &texture_desc);
    ok(texture_desc.Width == 320, "Got unexpected Width %u.\n", texture_desc.Width);
    ok(texture_desc.Height == 240, "Got unexpected Height %u.\n", texture_desc.Height);
    ok(texture_desc.MipLevels == 1, "Got unexpected MipLevels %u.\n", texture_desc.MipLevels);
    ok(texture_desc.ArraySize == 1, "Got unexpected ArraySize %u.\n", texture_desc.ArraySize);
    ok(texture_desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, "Got unexpected Format %#x.\n", texture_desc.Format);
    ok(texture_desc.SampleDesc.Count == 1, "Got unexpected SampleDesc.Count %u.\n", texture_desc.SampleDesc.Count);
    ok(!texture_desc.SampleDesc.Quality, "Got unexpected SampleDesc.Quality %u.\n", texture_desc.SampleDesc.Quality);
    ok(texture_desc.Usage == D3D10_USAGE_DEFAULT, "Got unexpected Usage %#x.\n", texture_desc.Usage);
    ok(texture_desc.BindFlags == D3D10_BIND_RENDER_TARGET, "Got unexpected BindFlags %#x.\n", texture_desc.BindFlags);
    ok(!texture_desc.CPUAccessFlags, "Got unexpected CPUAccessFlags %#x.\n", texture_desc.CPUAccessFlags);
    ok(!texture_desc.MiscFlags, "Got unexpected MiscFlags %#x.\n", texture_desc.MiscFlags);

    ID3D10Texture2D_Release(texture);
    IDXGISurface_Release(surface);

    hr = IDXGISwapChain_ResizeBuffers(swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
    ok(SUCCEEDED(hr), "Failed to resize buffers, hr %#x.\n", hr);

    hr = IDXGISwapChain_GetDesc(swapchain, &swapchain_desc);
    ok(SUCCEEDED(hr), "Failed to get swapchain desc, hr %#x.\n", hr);
    ok(swapchain_desc.BufferDesc.Width == client_rect.right - client_rect.left,
            "Got unexpected BufferDesc.Width %u, expected %u.\n",
            swapchain_desc.BufferDesc.Width, client_rect.right - client_rect.left);
    ok(swapchain_desc.BufferDesc.Height == client_rect.bottom - client_rect.top,
            "Got unexpected bufferDesc.Height %u, expected %u.\n",
            swapchain_desc.BufferDesc.Height, client_rect.bottom - client_rect.top);
    ok(swapchain_desc.BufferDesc.RefreshRate.Numerator == 60,
            "Got unexpected BufferDesc.RefreshRate.Numerator %u.\n",
            swapchain_desc.BufferDesc.RefreshRate.Numerator);
    ok(swapchain_desc.BufferDesc.RefreshRate.Denominator == 1,
            "Got unexpected BufferDesc.RefreshRate.Denominator %u.\n",
            swapchain_desc.BufferDesc.RefreshRate.Denominator);
    ok(swapchain_desc.BufferDesc.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            "Got unexpected BufferDesc.Format %#x.\n", swapchain_desc.BufferDesc.Format);
    ok(swapchain_desc.BufferDesc.ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
            "Got unexpected BufferDesc.ScanlineOrdering %#x.\n", swapchain_desc.BufferDesc.ScanlineOrdering);
    ok(swapchain_desc.BufferDesc.Scaling == DXGI_MODE_SCALING_UNSPECIFIED,
            "Got unexpected BufferDesc.Scaling %#x.\n", swapchain_desc.BufferDesc.Scaling);
    ok(swapchain_desc.SampleDesc.Count == 1,
            "Got unexpected SampleDesc.Count %u.\n", swapchain_desc.SampleDesc.Count);
    ok(!swapchain_desc.SampleDesc.Quality,
            "Got unexpected SampleDesc.Quality %u.\n", swapchain_desc.SampleDesc.Quality);
    ok(swapchain_desc.BufferUsage == DXGI_USAGE_RENDER_TARGET_OUTPUT,
            "Got unexpected BufferUsage %#x.\n", swapchain_desc.BufferUsage);
    ok(swapchain_desc.BufferCount == 1,
            "Got unexpected BufferCount %u.\n", swapchain_desc.BufferCount);
    ok(swapchain_desc.OutputWindow == window,
            "Got unexpected OutputWindow %p, expected %p.\n", swapchain_desc.OutputWindow, window);
    ok(swapchain_desc.Windowed,
            "Got unexpected Windowed %#x.\n", swapchain_desc.Windowed);
    ok(swapchain_desc.SwapEffect == DXGI_SWAP_EFFECT_DISCARD,
            "Got unexpected SwapEffect %#x.\n", swapchain_desc.SwapEffect);
    ok(!swapchain_desc.Flags,
            "Got unexpected Flags %#x.\n", swapchain_desc.Flags);

    IDXGISwapChain_Release(swapchain);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_swapchain_parameters(void)
{
    IDXGISwapChain *swapchain;
    IUnknown *obj;
    IDXGIAdapter *adapter;
    IDXGIFactory *factory;
    IDXGIDevice *device;
    IDXGIResource *resource;
    DXGI_SWAP_CHAIN_DESC desc;
    HRESULT hr;
    unsigned int i, j;
    ULONG refcount;
    DXGI_USAGE usage, expected_usage, broken_usage;
    HWND window;
    static const struct
    {
        BOOL windowed;
        UINT buffer_count;
        DXGI_SWAP_EFFECT swap_effect;
        HRESULT hr, vista_hr;
        UINT highest_accessible_buffer;
    }
    tests[] =
    {
        {TRUE,   0, DXGI_SWAP_EFFECT_DISCARD,         DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   1, DXGI_SWAP_EFFECT_DISCARD,         S_OK,                    S_OK,                     0},
        {TRUE,   2, DXGI_SWAP_EFFECT_DISCARD,         S_OK,                    S_OK,                     0},
        {TRUE,   0, DXGI_SWAP_EFFECT_SEQUENTIAL,      DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   1, DXGI_SWAP_EFFECT_SEQUENTIAL,      S_OK,                    S_OK,                     0},
        {TRUE,   2, DXGI_SWAP_EFFECT_SEQUENTIAL,      S_OK,                    S_OK,                     1},
        {TRUE,   3, DXGI_SWAP_EFFECT_SEQUENTIAL,      S_OK,                    S_OK,                     2},
        {TRUE,   0, 2 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   1, 2 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   2, 2 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   0, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   1, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   2, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, S_OK,                    DXGI_ERROR_INVALID_CALL,  1},
        {TRUE,   3, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, S_OK,                    DXGI_ERROR_INVALID_CALL,  2},
        {TRUE,   0, DXGI_SWAP_EFFECT_FLIP_DISCARD,    DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   1, DXGI_SWAP_EFFECT_FLIP_DISCARD,    DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   2, DXGI_SWAP_EFFECT_FLIP_DISCARD,    S_OK,                    DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   0, 5 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   1, 5 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   2, 5 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,  16, DXGI_SWAP_EFFECT_DISCARD,         S_OK,                    S_OK,                     0},
        {TRUE,  16, DXGI_SWAP_EFFECT_SEQUENTIAL,      S_OK,                    S_OK,                    15},
        {TRUE,  16, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, S_OK,                    DXGI_ERROR_INVALID_CALL, 15},
        {TRUE,  16, DXGI_SWAP_EFFECT_FLIP_DISCARD,    S_OK,                    DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,  17, DXGI_SWAP_EFFECT_DISCARD,         DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,  17, DXGI_SWAP_EFFECT_SEQUENTIAL,      DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,  17, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,  17, DXGI_SWAP_EFFECT_DISCARD,         DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},

        {FALSE,  0, DXGI_SWAP_EFFECT_DISCARD,         DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  1, DXGI_SWAP_EFFECT_DISCARD,         S_OK,                    S_OK,                     0},
        {FALSE,  2, DXGI_SWAP_EFFECT_DISCARD,         S_OK,                    S_OK,                     0},
        {FALSE,  0, DXGI_SWAP_EFFECT_SEQUENTIAL,      DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  1, DXGI_SWAP_EFFECT_SEQUENTIAL,      S_OK,                    S_OK,                     0},
        {FALSE,  2, DXGI_SWAP_EFFECT_SEQUENTIAL,      S_OK,                    S_OK,                     1},
        {FALSE,  3, DXGI_SWAP_EFFECT_SEQUENTIAL,      S_OK,                    S_OK,                     2},
        {FALSE,  0, 2 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  1, 2 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  2, 2 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  0, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  1, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  2, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, S_OK,                    DXGI_ERROR_INVALID_CALL,  1},
        {FALSE,  3, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, S_OK,                    DXGI_ERROR_INVALID_CALL,  2},
        {FALSE,  0, DXGI_SWAP_EFFECT_FLIP_DISCARD,    DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  1, DXGI_SWAP_EFFECT_FLIP_DISCARD,    DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  2, DXGI_SWAP_EFFECT_FLIP_DISCARD,    S_OK,                    DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  0, 5 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  1, 5 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  2, 5 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE, 16, DXGI_SWAP_EFFECT_DISCARD,         S_OK,                    S_OK,                     0},
        {FALSE, 16, DXGI_SWAP_EFFECT_SEQUENTIAL,      S_OK,                    S_OK,                    15},
        {FALSE, 16, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, S_OK,                    DXGI_ERROR_INVALID_CALL, 15},
        /* The following test fails on Nvidia with E_OUTOFMEMORY and leaks device references in the
         * process. Disable it for now.
        {FALSE, 16, DXGI_SWAP_EFFECT_FLIP_DISCARD,    S_OK,                    DXGI_ERROR_INVALID_CALL,  0},
         */
        {FALSE, 17, DXGI_SWAP_EFFECT_DISCARD,         DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE, 17, DXGI_SWAP_EFFECT_SEQUENTIAL,      DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE, 17, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE, 17, DXGI_SWAP_EFFECT_FLIP_DISCARD,    DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
    };

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }
    window = CreateWindowA("static", "dxgi_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, 0, 0, 0, 0);

    hr = IDXGIDevice_QueryInterface(device, &IID_IUnknown, (void **)&obj);
    ok(SUCCEEDED(hr), "IDXGIDevice does not implement IUnknown\n");

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(SUCCEEDED(hr), "GetAdapter failed, hr %#x.\n", hr);

    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(SUCCEEDED(hr), "GetParent failed, hr %#x.\n", hr);

    for (i = 0; i < sizeof(tests) / sizeof(*tests); ++i)
    {
        memset(&desc, 0, sizeof(desc));
        desc.BufferDesc.Width = registry_mode.dmPelsWidth;
        desc.BufferDesc.Height = registry_mode.dmPelsHeight;
        desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.OutputWindow = window;

        desc.Windowed = tests[i].windowed;
        desc.BufferCount = tests[i].buffer_count;
        desc.SwapEffect = tests[i].swap_effect;

        hr = IDXGIFactory_CreateSwapChain(factory, obj, &desc, &swapchain);
        ok(hr == tests[i].hr || broken(hr == tests[i].vista_hr)
                || (SUCCEEDED(tests[i].hr) && hr == DXGI_STATUS_OCCLUDED),
                "Got unexpected hr %#x, test %u.\n", hr, i);
        if (FAILED(hr))
            continue;

        hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_IDXGIResource, (void **)&resource);
        todo_wine ok(SUCCEEDED(hr), "GetBuffer(0) failed, hr %#x, test %u.\n", hr, i);
        if (FAILED(hr))
        {
            hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
            todo_wine ok(SUCCEEDED(hr), "SetFullscreenState failed, hr %#x.\n", hr);

            IDXGISwapChain_Release(swapchain);
            continue;
        }

        expected_usage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER;
        if (tests[i].swap_effect == DXGI_SWAP_EFFECT_DISCARD)
            expected_usage |= DXGI_USAGE_DISCARD_ON_PRESENT;
        hr = IDXGIResource_GetUsage(resource, &usage);
        ok(SUCCEEDED(hr), "Failed to get resource usage, hr %#x, test %u.\n", hr, i);
        ok(usage == expected_usage, "Got usage %x, expected %x, test %u.\n", usage, expected_usage, i);

        IDXGIResource_Release(resource);

        hr = IDXGISwapChain_GetDesc(swapchain, &desc);
        ok(SUCCEEDED(hr), "Failed to get swapchain desc, hr %#x.\n", hr);

        for (j = 1; j <= tests[i].highest_accessible_buffer; j++)
        {
            hr = IDXGISwapChain_GetBuffer(swapchain, j, &IID_IDXGIResource, (void **)&resource);
            ok(SUCCEEDED(hr), "GetBuffer(%u) failed, hr %#x, test %u.\n", hr, i, j);

            /* Buffers > 0 are supposed to be read only. This is the case except that in
             * fullscreen mode on Windows <= 8 the last backbuffer (BufferCount - 1) is
             * writable. This is not the case if an unsupported refresh rate is passed
             * for some reason, probably because the invalid refresh rate triggers a
             * kinda-sorta windowed mode.
             *
             * On Windows 10 all buffers > 0 are read-only. Mark the earlier behavior
             * broken.
             *
             * This last buffer acts as a shadow frontbuffer. Writing to it doesn't show
             * the draw on the screen right away (Aero on or off doesn't matter), but
             * Present with DXGI_PRESENT_DO_NOT_SEQUENCE will show the modifications.
             *
             * Note that if the application doesn't have focused creating a fullscreen
             * swapchain returns DXGI_STATUS_OCCLUDED and we get a windowed swapchain,
             * so use the Windowed property of the swapchain that was actually created. */
            expected_usage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_READ_ONLY;
            broken_usage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER;

            if (desc.Windowed || j < tests[i].highest_accessible_buffer)
                broken_usage |= DXGI_USAGE_READ_ONLY;

            hr = IDXGIResource_GetUsage(resource, &usage);
            ok(SUCCEEDED(hr), "Failed to get resource usage, hr %#x, test %u, buffer %u.\n", hr, i, j);
            ok(usage == expected_usage || broken(usage == broken_usage), "Got usage %x, expected %x, test %u, buffer %u.\n",
                    usage, expected_usage, i, j);

            IDXGIResource_Release(resource);
        }
        hr = IDXGISwapChain_GetBuffer(swapchain, j, &IID_IDXGIResource, (void **)&resource);
        ok(hr == DXGI_ERROR_INVALID_CALL, "GetBuffer(%u) returned unexpected hr %#x, test %u.\n", j, hr, i);

        hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
        todo_wine ok(SUCCEEDED(hr), "SetFullscreenState failed, hr %#x.\n", hr);

        IDXGISwapChain_Release(swapchain);
    }

    IDXGIFactory_Release(factory);
    IDXGIAdapter_Release(adapter);
    IUnknown_Release(obj);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_maximum_frame_latency(void)
{
    IDXGIDevice1 *device1;
    IDXGIDevice *device;
    UINT max_latency;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    if (SUCCEEDED(IDXGIDevice_QueryInterface(device, &IID_IDXGIDevice1, (void **)&device1)))
    {
        hr = IDXGIDevice1_GetMaximumFrameLatency(device1, &max_latency);
        todo_wine ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        ok(max_latency == DEFAULT_FRAME_LATENCY, "Got unexpected maximum frame latency %u.\n", max_latency);

        hr = IDXGIDevice1_SetMaximumFrameLatency(device1, MAX_FRAME_LATENCY);
        todo_wine ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        hr = IDXGIDevice1_GetMaximumFrameLatency(device1, &max_latency);
        todo_wine ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        todo_wine ok(max_latency == MAX_FRAME_LATENCY, "Got unexpected maximum frame latency %u.\n", max_latency);

        hr = IDXGIDevice1_SetMaximumFrameLatency(device1, MAX_FRAME_LATENCY + 1);
        ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);
        hr = IDXGIDevice1_GetMaximumFrameLatency(device1, &max_latency);
        todo_wine ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        todo_wine ok(max_latency == MAX_FRAME_LATENCY, "Got unexpected maximum frame latency %u.\n", max_latency);

        hr = IDXGIDevice1_SetMaximumFrameLatency(device1, 0);
        todo_wine ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        hr = IDXGIDevice1_GetMaximumFrameLatency(device1, &max_latency);
        todo_wine ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        /* 0 does not reset to the default frame latency on all Windows versions. */
        ok(max_latency == DEFAULT_FRAME_LATENCY || broken(!max_latency),
                "Got unexpected maximum frame latency %u.\n", max_latency);

        IDXGIDevice1_Release(device1);
    }
    else
    {
        win_skip("IDXGIDevice1 is not implemented.\n");
    }

    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_output_desc(void)
{
    IDXGIAdapter *adapter, *adapter2;
    IDXGIOutput *output, *output2;
    DXGI_OUTPUT_DESC desc;
    IDXGIFactory *factory;
    unsigned int i, j;
    ULONG refcount;
    HRESULT hr;

    hr = CreateDXGIFactory(&IID_IDXGIFactory, (void **)&factory);
    ok(SUCCEEDED(hr), "Failed to create DXGI factory, hr %#x.\n", hr);

    for (i = 0; ; ++i)
    {
        hr = IDXGIFactory_EnumAdapters(factory, i, &adapter);
        if (hr == DXGI_ERROR_NOT_FOUND)
            break;
        ok(SUCCEEDED(hr), "Failed to enumerate adapter %u, hr %#x.\n", i, hr);

        hr = IDXGIFactory_EnumAdapters(factory, i, &adapter2);
        ok(SUCCEEDED(hr), "Failed to enumerate adapter %u, hr %#x.\n", i, hr);
        ok(adapter != adapter2, "Expected to get new instance of IDXGIAdapter, %p == %p.\n", adapter, adapter2);
        refcount = get_refcount((IUnknown *)adapter);
        ok(refcount == 1, "Get unexpected refcount %u for adapter %u.\n", refcount, i);
        IDXGIAdapter_Release(adapter2);

        refcount = get_refcount((IUnknown *)factory);
        ok(refcount == 2, "Get unexpected refcount %u.\n", refcount);
        refcount = get_refcount((IUnknown *)adapter);
        ok(refcount == 1, "Get unexpected refcount %u for adapter %u.\n", refcount, i);

        for (j = 0; ; ++j)
        {
            MONITORINFOEXW monitor_info;
            BOOL ret;

            hr = IDXGIAdapter_EnumOutputs(adapter, j, &output);
            if (hr == DXGI_ERROR_NOT_FOUND)
                break;
            ok(SUCCEEDED(hr), "Failed to enumerate output %u on adapter %u, hr %#x.\n", j, i, hr);

            hr = IDXGIAdapter_EnumOutputs(adapter, j, &output2);
            ok(SUCCEEDED(hr), "Failed to enumerate output %u on adapter %u, hr %#x.\n", j, i, hr);
            ok(output != output2, "Expected to get new instance of IDXGIOuput, %p == %p.\n", output, output2);
            refcount = get_refcount((IUnknown *)output);
            ok(refcount == 1, "Get unexpected refcount %u for output %u, adapter %u.\n", refcount, j, i);
            IDXGIOutput_Release(output2);

            refcount = get_refcount((IUnknown *)factory);
            ok(refcount == 2, "Get unexpected refcount %u.\n", refcount);
            refcount = get_refcount((IUnknown *)adapter);
            ok(refcount == 2, "Get unexpected refcount %u for adapter %u.\n", refcount, i);
            refcount = get_refcount((IUnknown *)output);
            ok(refcount == 1, "Get unexpected refcount %u for output %u, adapter %u.\n", refcount, j, i);

            hr = IDXGIOutput_GetDesc(output, NULL);
            ok(hr == E_INVALIDARG, "Got unexpected hr %#x for output %u on adapter %u.\n", hr, j, i);
            hr = IDXGIOutput_GetDesc(output, &desc);
            ok(SUCCEEDED(hr), "Failed to get desc for output %u on adapter %u, hr %#x.\n", j, i, hr);

            monitor_info.cbSize = sizeof(monitor_info);
            ret = GetMonitorInfoW(desc.Monitor, (MONITORINFO *)&monitor_info);
            ok(ret, "Failed to get monitor info.\n");
            ok(!lstrcmpW(desc.DeviceName, monitor_info.szDevice), "Got unexpected device name %s, expected %s.\n",
                    wine_dbgstr_w(desc.DeviceName), wine_dbgstr_w(monitor_info.szDevice));
            ok(!memcmp(&desc.DesktopCoordinates, &monitor_info.rcMonitor, sizeof(desc.DesktopCoordinates)),
                    "Got unexpected desktop coordinates {%d, %d, %d, %d}, expected {%d, %d, %d, %d}.\n",
                    desc.DesktopCoordinates.left, desc.DesktopCoordinates.top,
                    desc.DesktopCoordinates.right, desc.DesktopCoordinates.bottom,
                    monitor_info.rcMonitor.left, monitor_info.rcMonitor.top,
                    monitor_info.rcMonitor.right, monitor_info.rcMonitor.bottom);

            IDXGIOutput_Release(output);
            refcount = get_refcount((IUnknown *)adapter);
            ok(refcount == 1, "Get unexpected refcount %u for adapter %u.\n", refcount, i);
        }

        IDXGIAdapter_Release(adapter);
        refcount = get_refcount((IUnknown *)factory);
        ok(refcount == 1, "Get unexpected refcount %u.\n", refcount);
    }

    refcount = IDXGIFactory_Release(factory);
    ok(!refcount, "IDXGIFactory has %u references left.\n", refcount);
}

START_TEST(device)
{
    pCreateDXGIFactory1 = (void *)GetProcAddress(GetModuleHandleA("dxgi.dll"), "CreateDXGIFactory1");

    registry_mode.dmSize = sizeof(registry_mode);
    ok(EnumDisplaySettingsW(NULL, ENUM_REGISTRY_SETTINGS, &registry_mode), "Failed to get display mode.\n");

    test_adapter_desc();
    test_check_interface_support();
    test_create_surface();
    test_parents();
    test_output();
    test_create_swapchain();
    test_get_containing_output();
    test_create_factory();
    test_private_data();
    test_swapchain_resize();
    test_swapchain_parameters();
    test_maximum_frame_latency();
    test_output_desc();
}
