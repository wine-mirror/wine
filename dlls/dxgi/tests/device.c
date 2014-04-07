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

static void test_device_interfaces(void)
{
    IDXGIDevice *device;
    IUnknown *iface;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = IDXGIDevice_QueryInterface(device, &IID_IUnknown, (void **)&iface);
    ok(SUCCEEDED(hr), "Failed to query IUnknown interface, hr %#x.\n", hr);
    IUnknown_Release(iface);

    hr = IDXGIDevice_QueryInterface(device, &IID_IDXGIObject, (void **)&iface);
    ok(SUCCEEDED(hr), "Failed to query IDXGIObject interface, hr %#x.\n", hr);
    IUnknown_Release(iface);

    hr = IDXGIDevice_QueryInterface(device, &IID_IDXGIDevice, (void **)&iface);
    ok(SUCCEEDED(hr), "Failed to query IDXGIDevice interface, hr %#x.\n", hr);
    IUnknown_Release(iface);

    hr = IDXGIDevice_QueryInterface(device, &IID_ID3D10Device, (void **)&iface);
    ok(SUCCEEDED(hr), "Failed to query ID3D10Device interface, hr %#x.\n", hr);
    IUnknown_Release(iface);

    hr = IDXGIDevice_QueryInterface(device, &IID_ID3D10Multithread, (void **)&iface);
    ok(SUCCEEDED(hr), "Failed to query ID3D10Multithread interface, hr %#x.\n", hr);
    IUnknown_Release(iface);

    if (SUCCEEDED(hr = IDXGIDevice_QueryInterface(device, &IID_ID3D10Device1, (void **)&iface)))
        IUnknown_Release(iface);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Failed to query ID3D10Device1 interface, hr %#x.\n", hr);

    if (SUCCEEDED(hr = IDXGIDevice_QueryInterface(device, &IID_ID3D11Device, (void **)&iface)))
        IUnknown_Release(iface);
    todo_wine ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Failed to query ID3D11Device interface, hr %#x.\n", hr);

    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
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

static void test_create_surface(void)
{
    ID3D10Texture2D *texture;
    IDXGISurface *surface;
    DXGI_SURFACE_DESC desc;
    IDXGIDevice *device;
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
    if (SUCCEEDED(hr)) ID3D10Texture2D_Release(texture);

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

static void test_createswapchain(void)
{
    IUnknown *obj;
    IDXGIAdapter *adapter;
    IDXGIFactory *factory;
    IDXGIDevice *device;
    ULONG refcount;
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

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

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
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
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
    todo_wine ok(hr == S_FALSE, "Got unexpected hr %#x.\n", hr);
    hr = IDXGIDevice_SetPrivateDataInterface(device, &dxgi_private_data_test_guid, NULL);
    todo_wine ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDXGIDevice_SetPrivateData(device, &dxgi_private_data_test_guid, ~0U, NULL);
    todo_wine ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDXGIDevice_SetPrivateData(device, &dxgi_private_data_test_guid, ~0U, NULL);
    todo_wine ok(hr == S_FALSE, "Got unexpected hr %#x.\n", hr);

    hr = IDXGIDevice_SetPrivateDataInterface(device, &dxgi_private_data_test_guid, NULL);
    todo_wine ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    size = sizeof(ptr) * 2;
    ptr = (IUnknown *)0xdeadbeef;
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid, &size, &ptr);
    todo_wine ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    todo_wine ok(!ptr, "Got unexpected pointer %p.\n", ptr);
    todo_wine ok(size == sizeof(IUnknown *), "Got unexpected size %u.\n", size);

    refcount = get_refcount((IUnknown *)test_object);
    hr = IDXGIDevice_SetPrivateDataInterface(device, &dxgi_private_data_test_guid,
            (IUnknown *)test_object);
    todo_wine ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    expected_refcount = refcount + 1;
    refcount = get_refcount((IUnknown *)test_object);
    todo_wine ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    hr = IDXGIDevice_SetPrivateDataInterface(device, &dxgi_private_data_test_guid,
            (IUnknown *)test_object);
    todo_wine ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)test_object);
    todo_wine ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);

    hr = IDXGIDevice_SetPrivateDataInterface(device, &dxgi_private_data_test_guid, NULL);
    todo_wine ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    expected_refcount--;
    refcount = get_refcount((IUnknown *)test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);

    hr = IDXGIDevice_SetPrivateDataInterface(device, &dxgi_private_data_test_guid,
            (IUnknown *)test_object);
    todo_wine ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    size = sizeof(data);
    hr = IDXGIDevice_SetPrivateData(device, &dxgi_private_data_test_guid, size, data);
    todo_wine ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    hr = IDXGIDevice_SetPrivateData(device, &dxgi_private_data_test_guid, 42, NULL);
    todo_wine ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDXGIDevice_SetPrivateData(device, &dxgi_private_data_test_guid, 42, NULL);
    todo_wine ok(hr == S_FALSE, "Got unexpected hr %#x.\n", hr);

    hr = IDXGIDevice_SetPrivateDataInterface(device, &dxgi_private_data_test_guid,
            (IUnknown *)test_object);
    todo_wine ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    expected_refcount++;
    size = 2 * sizeof(ptr);
    ptr = NULL;
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid, &size, &ptr);
    todo_wine ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    todo_wine ok(size == sizeof(test_object), "Got unexpected size %u.\n", size);
    expected_refcount++;
    refcount = get_refcount((IUnknown *)test_object);
    todo_wine ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    if (ptr)
        IUnknown_Release(ptr);
    expected_refcount--;

    ptr = (IUnknown *)0xdeadbeef;
    size = 1;
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid, &size, NULL);
    todo_wine ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    todo_wine ok(size == sizeof(device), "Got unexpected size %u.\n", size);
    size = 2 * sizeof(ptr);
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid, &size, NULL);
    todo_wine ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    todo_wine ok(size == sizeof(device), "Got unexpected size %u.\n", size);
    refcount = get_refcount((IUnknown *)test_object);
    todo_wine ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);

    size = 1;
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid, &size, &ptr);
    todo_wine ok(hr == DXGI_ERROR_MORE_DATA, "Got unexpected hr %#x.\n", hr);
    todo_wine ok(size == sizeof(device), "Got unexpected size %u.\n", size);
    ok(ptr == (IUnknown *)0xdeadbeef, "Got unexpected pointer %p.\n", ptr);
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid2, NULL, NULL);
    todo_wine ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    size = 0xdeadbabe;
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid2, &size, &ptr);
    todo_wine ok(hr == DXGI_ERROR_NOT_FOUND, "Got unexpected hr %#x.\n", hr);
    todo_wine ok(size == 0, "Got unexpected size %u.\n", size);
    ok(ptr == (IUnknown *)0xdeadbeef, "Got unexpected pointer %p.\n", ptr);
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid, NULL, &ptr);
    todo_wine ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ok(ptr == (IUnknown *)0xdeadbeef, "Got unexpected pointer %p.\n", ptr);

    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    refcount = IDXGIDevice_Release(test_object);
    ok(!refcount, "Test object has %u references left.\n", refcount);
}

START_TEST(device)
{
    pCreateDXGIFactory1 = (void *)GetProcAddress(GetModuleHandleA("dxgi.dll"), "CreateDXGIFactory1");

    test_adapter_desc();
    test_device_interfaces();
    test_create_surface();
    test_parents();
    test_output();
    test_createswapchain();
    test_create_factory();
    test_private_data();
}
