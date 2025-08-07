/*
 * Copyright (C) 2025 Mohamad Al-Jaf
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

#include <stdarg.h>

#define COBJMACROS
#include "initguid.h"
#include "dxcore.h"

#include "wine/test.h"

static HRESULT (WINAPI *pDXCoreCreateAdapterFactory)(REFIID riid, void **out);

#define check_interface(iface, riid, supported) check_interface_(__LINE__, iface, riid, supported)
static void check_interface_(unsigned int line, void *iface, REFIID riid, BOOL supported)
{
    IUnknown *unknown = iface, *out;
    HRESULT hr, expected_hr;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(unknown, riid, (void **)&out);
    ok_(__FILE__, line)(hr == expected_hr, "got hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(out);
}

static BOOL is_any_display_available(void)
{
    DISPLAY_DEVICEW display_device;
    display_device.cb = sizeof(display_device);
    return EnumDisplayDevicesW(NULL, 0, &display_device, 0);
}

static void test_DXCoreCreateAdapterFactory(void)
{
    IDXCoreAdapterFactory *factory2 = (void *)0xdeadbeef;
    IDXCoreAdapterFactory *factory = (void *)0xdeadbeef;
    IDXCoreAdapterList *list2 = (void *)0xdeadbeef;
    IDXCoreAdapterList *list = (void *)0xdeadbeef;
    IDXCoreAdapter *adapter2 = (void *)0xdeadbeef;
    IDXCoreAdapter *adapter = (void *)0xdeadbeef;
    struct DXCoreHardwareID *hardware_id;
    void *buffer = (void *)0xdeadbeef;
    uint32_t adapter_count = 0;
    LONG refcount;
    HRESULT hr;

    if (0) /* Crashes on w1064v1909 */
    {
        hr = pDXCoreCreateAdapterFactory(NULL, NULL);
        ok(hr == E_POINTER, "got hr %#lx.\n", hr);
    }
    hr = pDXCoreCreateAdapterFactory(&IID_IDXCoreAdapterFactory, NULL);
    ok(hr == E_POINTER || broken(hr == E_NOINTERFACE) /* w1064v1909 */, "got hr %#lx.\n", hr);
    hr = pDXCoreCreateAdapterFactory(&DXCORE_ADAPTER_ATTRIBUTE_D3D11_GRAPHICS, (void **)&factory);
    ok(hr == E_NOINTERFACE, "got hr %#lx.\n", hr);
    ok(factory == NULL || broken(factory == (void *)0xdeadbeef) /* w1064v1909 */, "got factory %p.\n", factory);

    hr = pDXCoreCreateAdapterFactory(&IID_IDXCoreAdapterFactory, (void **)&factory);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* w1064v1909 */, "got hr %#lx.\n", hr);
    if (FAILED(hr))
        return;

    hr = pDXCoreCreateAdapterFactory(&IID_IDXCoreAdapterFactory, (void **)&factory2);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    ok(factory == factory2, "got factory %p, factory2 %p.\n", factory, factory2);
    refcount = IDXCoreAdapterFactory_Release(factory2);
    ok(refcount == 1, "got refcount %ld.\n", refcount);

    check_interface(factory, &IID_IUnknown, TRUE);
    check_interface(factory, &IID_IDXCoreAdapterFactory, TRUE);
    check_interface(factory, &IID_IAgileObject, FALSE);
    check_interface(factory, &IID_IDXCoreAdapter, FALSE);
    check_interface(factory, &IID_IDXCoreAdapterList, FALSE);

    hr = IDXCoreAdapterFactory_CreateAdapterList(factory, 0, &DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRAPHICS, &IID_IDXCoreAdapterList, (void **)&list);
    ok(hr == E_INVALIDARG, "got hr %#lx.\n", hr);
    ok(list == NULL, "got list %p.\n", list);
    hr = IDXCoreAdapterFactory_CreateAdapterList(factory, 1, &DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRAPHICS, &IID_IDXCoreAdapterFactory, (void **)&list);
    ok(hr == E_NOINTERFACE, "got hr %#lx.\n", hr);
    hr = IDXCoreAdapterFactory_CreateAdapterList(factory, 1, NULL, &IID_IDXCoreAdapterFactory, (void **)&list);
    ok(hr == E_INVALIDARG, "got hr %#lx.\n", hr);
    hr = IDXCoreAdapterFactory_CreateAdapterList(factory, 1, &DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRAPHICS, &IID_IDXCoreAdapterFactory, NULL);
    ok(hr == E_POINTER, "got hr %#lx.\n", hr);

    hr = IDXCoreAdapterFactory_CreateAdapterList(factory, 1, &DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRAPHICS, &IID_IDXCoreAdapterList, (void **)&list);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    hr = IDXCoreAdapterFactory_CreateAdapterList(factory, 1, &DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRAPHICS, &IID_IDXCoreAdapterList, (void **)&list2);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    ok(list != list2, "got same list %p, list2 %p.\n", list, list);
    refcount = IDXCoreAdapterList_Release(list2);
    ok(refcount == 0, "got refcount %ld.\n", refcount);

    check_interface(list, &IID_IUnknown, TRUE);
    check_interface(list, &IID_IDXCoreAdapterList, TRUE);
    check_interface(list, &IID_IAgileObject, FALSE);
    check_interface(list, &IID_IDXCoreAdapter, FALSE);
    check_interface(list, &IID_IDXCoreAdapterFactory, FALSE);

    adapter_count = IDXCoreAdapterList_GetAdapterCount(list);
    ok(adapter_count != 0, "got adapter_count 0.\n");

    hr = IDXCoreAdapterList_GetAdapter(list, 0xdeadbeef, &IID_IDXCoreAdapter, NULL);
    ok(hr == E_POINTER, "got hr %#lx.\n", hr);
    hr = IDXCoreAdapterList_GetAdapter(list, adapter_count, &IID_IDXCoreAdapter, (void **)&adapter);
    ok(hr == E_INVALIDARG, "got hr %#lx.\n", hr);
    ok(adapter == NULL, "got adapter %p.\n", adapter);
    hr = IDXCoreAdapterList_GetAdapter(list, 0, &IID_IDXCoreAdapterList, (void **)&adapter);
    ok(hr == E_NOINTERFACE, "got hr %#lx.\n", hr);

    hr = IDXCoreAdapterList_GetAdapter(list, 0, &IID_IDXCoreAdapter, (void **)&adapter);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    hr = IDXCoreAdapterList_GetAdapter(list, 0, &IID_IDXCoreAdapter, (void **)&adapter2);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    ok(adapter == adapter2, "got adapter %p, adapter2 %p.\n", adapter, adapter2);
    refcount = IDXCoreAdapter_Release(adapter2);
    todo_wine
    ok(refcount == 3, "got refcount %ld.\n", refcount);

    check_interface(adapter, &IID_IUnknown, TRUE);
    check_interface(adapter, &IID_IDXCoreAdapter, TRUE);
    check_interface(adapter, &IID_IAgileObject, FALSE);
    check_interface(adapter, &IID_IDXCoreAdapterList, FALSE);
    check_interface(adapter, &IID_IDXCoreAdapterFactory, FALSE);

    hr = IDXCoreAdapter_GetProperty(adapter, HardwareID, 0, NULL);
    ok(hr == E_POINTER, "got hr %#lx.\n", hr);
    hr = IDXCoreAdapter_GetProperty(adapter, HardwareID, 0, buffer);
    ok(hr == E_INVALIDARG, "got hr %#lx.\n", hr);
    hr = IDXCoreAdapter_GetProperty(adapter, 0xdeadbeef, 0, buffer);
    ok(hr == DXGI_ERROR_INVALID_CALL, "got hr %#lx.\n", hr);

    buffer = calloc(1, sizeof(HardwareID));
    ok(buffer != NULL, "failed to allocate memory for buffer.\n");
    hr = IDXCoreAdapter_GetProperty(adapter, HardwareID, sizeof(HardwareID), buffer);
    ok(hr == E_INVALIDARG, "got hr %#lx.\n", hr);
    free(buffer);
    buffer = calloc(1, sizeof(DXCoreHardwareID));
    ok(buffer != NULL, "failed to allocate memory for buffer.\n");
    hr = IDXCoreAdapter_GetProperty(adapter, 0xdeadbeef, sizeof(DXCoreHardwareID), buffer);
    ok(hr == DXGI_ERROR_INVALID_CALL, "got hr %#lx.\n", hr);
    free(buffer);

    buffer = calloc(1, sizeof(DXCoreHardwareID));
    ok(buffer != NULL, "failed to allocate memory for buffer.\n");
    hr = IDXCoreAdapter_GetProperty(adapter, HardwareID, sizeof(DXCoreHardwareID), buffer);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    hardware_id = buffer;
    ok(hardware_id->vendorID != 0, "failed to get vendorID\n");
    ok(hardware_id->deviceID != 0, "failed to get deviceID\n");
    free(buffer);

    refcount = IDXCoreAdapter_Release(adapter);
    todo_wine
    ok(refcount == 2, "got refcount %ld.\n", refcount);
    refcount = IDXCoreAdapterList_Release(list);
    ok(refcount == 0, "got refcount %ld.\n", refcount);
    refcount = IDXCoreAdapterFactory_Release(factory);
    ok(refcount == 0, "got refcount %ld.\n", refcount);
}

START_TEST(dxcore)
{
    HMODULE dxcore_handle = LoadLibraryA("dxcore.dll");
    if (!dxcore_handle)
    {
        win_skip("Could not load dxcore.dll\n");
        return;
    }
    pDXCoreCreateAdapterFactory = (void *)GetProcAddress(dxcore_handle, "DXCoreCreateAdapterFactory");
    if (!pDXCoreCreateAdapterFactory)
    {
        win_skip("Failed to get DXCoreCreateAdapterFactory address, skipping dxcore tests\n");
        FreeLibrary(dxcore_handle);
        return;
    }

    if (!is_any_display_available())
    {
        skip("No display available.\n");
        return;
    }

    test_DXCoreCreateAdapterFactory();

    FreeLibrary(dxcore_handle);
}
