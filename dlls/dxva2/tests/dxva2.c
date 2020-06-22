/*
 * Copyright 2020 Nikolay Sivov
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
#include "wine/test.h"
#include "d3d9.h"

#include "initguid.h"
#include "dxva2api.h"

static int get_refcount(IUnknown *object)
{
    IUnknown_AddRef(object);
    return IUnknown_Release(object);
}

static HWND create_window(void)
{
    RECT r = {0, 0, 640, 480};

    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW | WS_VISIBLE, FALSE);

    return CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, r.right - r.left, r.bottom - r.top, NULL, NULL, NULL, NULL);
}

static IDirect3DDevice9 *create_device(IDirect3D9 *d3d9, HWND focus_window)
{
    D3DPRESENT_PARAMETERS present_parameters = {0};
    IDirect3DDevice9 *device = NULL;

    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.hDeviceWindow = focus_window;
    present_parameters.Windowed = TRUE;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;

    IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, focus_window,
            D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, &device);

    return device;
}

static void test_device_manager(void)
{
    IDirectXVideoProcessorService *processor_service;
    IDirect3DDevice9 *device, *device2;
    IDirect3DDeviceManager9 *manager;
    int refcount, refcount2;
    HANDLE handle, handle1;
    IDirect3D9 *d3d;
    HWND window;
    UINT token;
    HRESULT hr;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = DXVA2CreateDirect3DDeviceManager9(&token, &manager);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle);
todo_wine
    ok(hr == DXVA2_E_NOT_INITIALIZED, "Unexpected hr %#x.\n", hr);

    /* Invalid token. */
    hr = IDirect3DDeviceManager9_ResetDevice(manager, device, token + 1);
todo_wine
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_ResetDevice(manager, device, token);
todo_wine
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    refcount = get_refcount((IUnknown *)device);

    handle1 = NULL;
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle1);
todo_wine
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    refcount2 = get_refcount((IUnknown *)device);
    ok(refcount2 == refcount, "Unexpected refcount %d.\n", refcount);

    handle = NULL;
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle);
todo_wine {
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(handle != handle1, "Unexpected handle.\n");
}
    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle);
todo_wine
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    /* Already closed. */
    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle);
todo_wine
    ok(hr == E_HANDLE, "Unexpected hr %#x.\n", hr);

    handle = NULL;
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle);
todo_wine
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle1);
todo_wine
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_TestDevice(manager, handle1);
todo_wine
    ok(hr == E_HANDLE, "Unexpected hr %#x.\n", hr);

    handle = NULL;
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle);
todo_wine
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    handle1 = NULL;
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle1);
todo_wine
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_GetVideoService(manager, handle, &IID_IDirectXVideoProcessorService,
            (void **)&processor_service);
todo_wine
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    if (SUCCEEDED(hr))
        IDirectXVideoProcessorService_Release(processor_service);

    device2 = create_device(d3d, window);
    hr = IDirect3DDeviceManager9_ResetDevice(manager, device2, token);
todo_wine
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_GetVideoService(manager, handle, &IID_IDirectXVideoProcessorService,
            (void **)&processor_service);
todo_wine
    ok(hr == DXVA2_E_NEW_VIDEO_DEVICE, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_TestDevice(manager, handle);
todo_wine
    ok(hr == DXVA2_E_NEW_VIDEO_DEVICE, "Unexpected hr %#x.\n", hr);

    IDirect3DDevice9_Release(device);
    IDirect3DDevice9_Release(device2);

    IDirect3DDeviceManager9_Release(manager);

done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

START_TEST(dxva2)
{
    test_device_manager();
}
