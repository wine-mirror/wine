/*
 * Copyright 2017 Józef Kucia for CodeWeavers
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

#include <stdlib.h>
#define COBJMACROS
#include "initguid.h"
#include "d3d12.h"
#include "dxgi1_6.h"
#include "wine/test.h"

static ID3D12Device *create_device(void)
{
    ID3D12Device *device;

    if (FAILED(D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device, (void **)&device)))
        return NULL;

    return device;
}

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#x, expected %#x.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

static void test_interfaces(void)
{
    D3D12_COMMAND_QUEUE_DESC desc;
    ID3D12CommandQueue *queue;
    ID3D12Device *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    check_interface(device, &IID_ID3D12Object, TRUE);
    check_interface(device, &IID_ID3D12DeviceChild, FALSE);
    check_interface(device, &IID_ID3D12Pageable, FALSE);
    check_interface(device, &IID_ID3D12Device, TRUE);

    check_interface(device, &IID_IDXGIObject, FALSE);
    check_interface(device, &IID_IDXGIDeviceSubObject, FALSE);
    check_interface(device, &IID_IDXGIDevice, FALSE);
    check_interface(device, &IID_IDXGIDevice1, FALSE);
    check_interface(device, &IID_IDXGIDevice2, FALSE);
    check_interface(device, &IID_IDXGIDevice3, FALSE);
    check_interface(device, &IID_IDXGIDevice4, FALSE);

    desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;
    hr = ID3D12Device_CreateCommandQueue(device, &desc, &IID_ID3D12CommandQueue, (void **)&queue);
    ok(hr == S_OK, "Failed to create command queue, hr %#x.\n", hr);

    check_interface(queue, &IID_ID3D12Object, TRUE);
    check_interface(queue, &IID_ID3D12DeviceChild, TRUE);
    check_interface(queue, &IID_ID3D12Pageable, TRUE);
    check_interface(queue, &IID_ID3D12CommandQueue, TRUE);

    check_interface(queue, &IID_IDXGIObject, FALSE);
    check_interface(queue, &IID_IDXGIDeviceSubObject, FALSE);
    check_interface(queue, &IID_IDXGIDevice, FALSE);
    check_interface(queue, &IID_IDXGIDevice1, FALSE);
    check_interface(queue, &IID_IDXGIDevice2, FALSE);
    check_interface(queue, &IID_IDXGIDevice3, FALSE);
    check_interface(queue, &IID_IDXGIDevice4, FALSE);

    refcount = ID3D12CommandQueue_Release(queue);
    ok(!refcount, "Command queue has %u references left.\n", refcount);
    refcount = ID3D12Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

START_TEST(d3d12)
{
    test_interfaces();
}
