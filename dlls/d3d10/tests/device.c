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

static ID3D10Device *create_device(void)
{
    ID3D10Device *device;

    if (SUCCEEDED(D3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &device)))
    {
        trace("Created a HW device\n");
        return device;
    }

    trace("Failed to create a HW device, trying REF\n");
    if (SUCCEEDED(D3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_REFERENCE, NULL, 0, D3D10_SDK_VERSION, &device)))
    {
        trace("Created a REF device\n");
        return device;
    }

    trace("Failed to create a device, returning NULL\n");
    return NULL;
}

static void test_device_interfaces(ID3D10Device *device)
{
    HRESULT hr;
    IUnknown *obj;

    if (SUCCEEDED(hr = ID3D10Device_QueryInterface(device, &IID_IUnknown, (void **)&obj)))
        IUnknown_Release(obj);
    ok(SUCCEEDED(hr), "ID3D10Device does not implement IUnknown (%#x)\n", hr);

    if (SUCCEEDED(hr = ID3D10Device_QueryInterface(device, &IID_ID3D10Device, (void **)&obj)))
        IUnknown_Release(obj);
    ok(SUCCEEDED(hr), "ID3D10Device does not implement ID3D10Device (%#x)\n", hr);

    if (SUCCEEDED(hr = ID3D10Device_QueryInterface(device, &IID_IDXGIObject, (void **)&obj)))
        IUnknown_Release(obj);
    ok(SUCCEEDED(hr), "ID3D10Device does not implement IDXGIObject (%#x)\n", hr);

    if (SUCCEEDED(hr = ID3D10Device_QueryInterface(device, &IID_IDXGIDevice, (void **)&obj)))
        IUnknown_Release(obj);
    ok(SUCCEEDED(hr), "ID3D10Device does not implement IDXGIDevice (%#x)\n", hr);
}

static void test_stateblock_mask(void)
{
    D3D10_STATE_BLOCK_MASK mask_x, mask_y, result;
    HRESULT hr;

    memset(&mask_x, 0, sizeof(mask_x));
    memset(&mask_y, 0, sizeof(mask_y));
    memset(&result, 0, sizeof(result));

    mask_x.VS = 0x33;
    mask_y.VS = 0x55;
    mask_x.Predication = 0x99;
    mask_y.Predication = 0xaa;

    hr = D3D10StateBlockMaskDifference(&mask_x, &mask_y, &result);
    ok(SUCCEEDED(hr), "D3D10StateBlockMaskDifference failed, hr %#x.\n", hr);
    ok(result.VS == 0x66, "Got unexpected result.VS %#x.\n", result.VS);
    ok(result.Predication == 0x33, "Got unexpected result.Predication %#x.\n", result.Predication);
    hr = D3D10StateBlockMaskDifference(NULL, &mask_y, &result);
    ok(hr == E_INVALIDARG, "Got unexpect hr %#x.\n", hr);
    hr = D3D10StateBlockMaskDifference(&mask_x, NULL, &result);
    ok(hr == E_INVALIDARG, "Got unexpect hr %#x.\n", hr);
    hr = D3D10StateBlockMaskDifference(&mask_x, &mask_y, NULL);
    ok(hr == E_INVALIDARG, "Got unexpect hr %#x.\n", hr);
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
    test_stateblock_mask();

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left\n", refcount);
}
