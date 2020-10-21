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

static unsigned int get_refcount(void *object)
{
    IUnknown *iface = object;
    IUnknown_AddRef(iface);
    return IUnknown_Release(iface);
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

static void test_surface_desc(IDirect3DSurface9 *surface)
{
    D3DSURFACE_DESC desc = { 0 };
    HRESULT hr;

    hr = IDirect3DSurface9_GetDesc(surface, &desc);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    ok(desc.Format == D3DFMT_X8R8G8B8, "Unexpected format %d.\n", desc.Format);
    ok(desc.Type == D3DRTYPE_SURFACE, "Unexpected type %d.\n", desc.Type);
    ok(desc.Usage == 0, "Unexpected usage %d.\n", desc.Usage);
    ok(desc.Pool == D3DPOOL_DEFAULT, "Unexpected pool %d.\n", desc.Pool);
    ok(desc.MultiSampleType == D3DMULTISAMPLE_NONE, "Unexpected multisample type %d.\n", desc.MultiSampleType);
    ok(desc.MultiSampleQuality == 0, "Unexpected multisample quality %d.\n", desc.MultiSampleQuality);
    ok(desc.Width == 64, "Unexpected width %u.\n", desc.Width);
    ok(desc.Height == 64, "Unexpected height %u.\n", desc.Height);
}

static void test_device_manager(void)
{
    IDirectXVideoProcessorService *processor_service;
    IDirectXVideoAccelerationService *accel_service;
    IDirect3DDevice9 *device, *device2, *device3;
    IDirectXVideoProcessorService *proc_service;
    IDirect3DDeviceManager9 *manager;
    IDirect3DSurface9 *surfaces[2];
    DXVA2_VideoDesc video_desc;
    int refcount, refcount2;
    HANDLE handle, handle1;
    D3DFORMAT *formats;
    UINT token, count;
    IDirect3D9 *d3d;
    HWND window;
    GUID *guids;
    HRESULT hr;
    RECT rect;

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
    ok(hr == DXVA2_E_NOT_INITIALIZED, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_LockDevice(manager, 0, &device2, FALSE);
    ok(hr == DXVA2_E_NOT_INITIALIZED, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, 0);
    ok(hr == E_HANDLE, "Unexpected hr %#x.\n", hr);

    /* Invalid token. */
    hr = IDirect3DDeviceManager9_ResetDevice(manager, device, token + 1);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_ResetDevice(manager, device, token);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    refcount = get_refcount(device);

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, 0);
    ok(hr == E_HANDLE, "Unexpected hr %#x.\n", hr);

    handle1 = NULL;
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle1);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!!handle1, "Unexpected handle value.\n");

    refcount2 = get_refcount(device);
    ok(refcount2 == refcount, "Unexpected refcount %d.\n", refcount);

    handle = NULL;
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(handle != handle1, "Unexpected handle.\n");

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    /* Already closed. */
    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle);
    ok(hr == E_HANDLE, "Unexpected hr %#x.\n", hr);

    handle = NULL;
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle1);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_TestDevice(manager, handle1);
    ok(hr == E_HANDLE, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_TestDevice(manager, 0);
    ok(hr == E_HANDLE, "Unexpected hr %#x.\n", hr);

    handle = NULL;
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    handle1 = NULL;
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle1);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_GetVideoService(manager, handle, &IID_IDirectXVideoProcessorService,
            (void **)&processor_service);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    IDirectXVideoProcessorService_Release(processor_service);

    device2 = create_device(d3d, window);

    hr = IDirect3DDeviceManager9_ResetDevice(manager, device2, token);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_GetVideoService(manager, handle, &IID_IDirectXVideoProcessorService,
            (void **)&processor_service);
    ok(hr == DXVA2_E_NEW_VIDEO_DEVICE, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_TestDevice(manager, handle);
    ok(hr == DXVA2_E_NEW_VIDEO_DEVICE, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    /* Lock/Unlock. */
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_LockDevice(manager, handle, &device3, FALSE);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(device2 == device3, "Unexpected device pointer.\n");
    IDirect3DDevice9_Release(device3);

    hr = IDirect3DDeviceManager9_UnlockDevice(manager, handle, FALSE);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_UnlockDevice(manager, handle, FALSE);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_UnlockDevice(manager, (HANDLE)((ULONG_PTR)handle + 100), FALSE);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    /* Locked with one handle, unlock with another. */
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle1);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_LockDevice(manager, handle, &device3, FALSE);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(device2 == device3, "Unexpected device pointer.\n");
    IDirect3DDevice9_Release(device3);

    hr = IDirect3DDeviceManager9_UnlockDevice(manager, handle1, FALSE);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    /* Closing unlocks the device. */
    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_LockDevice(manager, handle1, &device3, FALSE);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(device2 == device3, "Unexpected device pointer.\n");
    IDirect3DDevice9_Release(device3);

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle1);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    /* Open two handles. */
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle1);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_LockDevice(manager, handle, &device3, FALSE);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(device2 == device3, "Unexpected device pointer.\n");
    IDirect3DDevice9_Release(device3);

    hr = IDirect3DDeviceManager9_LockDevice(manager, handle1, &device3, FALSE);
    ok(hr == DXVA2_E_VIDEO_DEVICE_LOCKED, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle1);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    /* State saving function. */
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_LockDevice(manager, handle, &device3, FALSE);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(device2 == device3, "Unexpected device pointer.\n");

    SetRect(&rect, 50, 60, 70, 80);
    hr = IDirect3DDevice9_SetScissorRect(device3, &rect);
    ok(SUCCEEDED(hr), "Failed to set scissor rect, hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_UnlockDevice(manager, handle, TRUE);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    SetRect(&rect, 30, 60, 70, 80);
    hr = IDirect3DDevice9_SetScissorRect(device3, &rect);
    ok(SUCCEEDED(hr), "Failed to set scissor rect, hr %#x.\n", hr);

    IDirect3DDevice9_Release(device3);

    hr = IDirect3DDeviceManager9_LockDevice(manager, handle, &device3, FALSE);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(device2 == device3, "Unexpected device pointer.\n");

    hr = IDirect3DDevice9_GetScissorRect(device3, &rect);
    ok(SUCCEEDED(hr), "Failed to get scissor rect, hr %#x.\n", hr);
    ok(rect.left == 50 && rect.top == 60 && rect.right == 70 && rect.bottom == 80,
            "Got unexpected scissor rect %s.\n", wine_dbgstr_rect(&rect));

    IDirect3DDevice9_Release(device3);

    hr = IDirect3DDeviceManager9_UnlockDevice(manager, handle, TRUE);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    /* Acceleration service. */
    hr = DXVA2CreateVideoService(device, &IID_IDirectXVideoAccelerationService, (void **)&accel_service);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    surfaces[0] = surfaces[1] = NULL;
    hr = IDirectXVideoAccelerationService_CreateSurface(accel_service, 64, 64, 0, D3DFMT_X8R8G8B8,
            D3DPOOL_DEFAULT, 0, DXVA2_VideoProcessorRenderTarget, surfaces, NULL);
    ok(hr == S_OK, "Failed to create a surface, hr %#x.\n", hr);
    ok(!!surfaces[0] && !surfaces[1], "Unexpected surfaces.\n");
    IDirect3DSurface9_Release(surfaces[0]);

    hr = IDirectXVideoAccelerationService_CreateSurface(accel_service, 64, 64, 1, D3DFMT_X8R8G8B8,
            D3DPOOL_DEFAULT, 0, DXVA2_VideoProcessorRenderTarget, surfaces, NULL);
    ok(hr == S_OK, "Failed to create a surface, hr %#x.\n", hr);
    ok(!!surfaces[0] && !!surfaces[1], "Unexpected surfaces.\n");
    test_surface_desc(surfaces[0]);
    IDirect3DSurface9_Release(surfaces[0]);
    IDirect3DSurface9_Release(surfaces[1]);

    IDirectXVideoAccelerationService_Release(accel_service);

    /* RT formats. */
    hr = DXVA2CreateVideoService(device, &IID_IDirectXVideoProcessorService, (void **)&proc_service);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    memset(&video_desc, 0, sizeof(video_desc));
    video_desc.SampleWidth = 64;
    video_desc.SampleHeight = 64;
    video_desc.Format = D3DFMT_A8R8G8B8;

    hr = IDirectXVideoProcessorService_GetVideoProcessorDeviceGuids(proc_service, &video_desc, &count, &guids);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(count, "Unexpected format count %u.\n", count);
    CoTaskMemFree(guids);

    count = 0;
    hr = IDirectXVideoProcessorService_GetVideoProcessorRenderTargets(proc_service, &DXVA2_VideoProcSoftwareDevice,
            &video_desc, &count, &formats);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(count, "Unexpected format count %u.\n", count);
    CoTaskMemFree(formats);

    IDirectXVideoProcessorService_Release(proc_service);

    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_GetVideoService(manager, handle, &IID_IDirectXVideoAccelerationService,
            (void **)&accel_service);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirectXVideoAccelerationService_CreateSurface(accel_service, 64, 64, 0, D3DFMT_X8R8G8B8,
            D3DPOOL_DEFAULT, 0, DXVA2_VideoProcessorRenderTarget, surfaces, NULL);
    ok(hr == S_OK, "Failed to create a surface, hr %#x.\n", hr);

    hr = IDirect3DSurface9_GetDevice(surfaces[0], &device3);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(device2 == device3, "Unexpected device.\n");
    IDirect3DDevice9_Release(device3);

    IDirect3DSurface9_Release(surfaces[0]);

    IDirectXVideoAccelerationService_Release(accel_service);

    IDirect3DDevice9_Release(device);
    IDirect3DDevice9_Release(device2);

    IDirect3DDeviceManager9_Release(manager);

done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_video_processor(void)
{
    IDirectXVideoProcessorService *service, *service2;
    IDirect3DDevice9 *device;
    IDirect3DDeviceManager9 *manager;
    HANDLE handle, handle1;
    IDirect3D9 *d3d;
    HWND window;
    UINT token;
    HRESULT hr;
    IDirectXVideoProcessor *processor, *processor2;
    DXVA2_VideoDesc video_desc;
    GUID guid;
    D3DFORMAT format;

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

    hr = IDirect3DDeviceManager9_ResetDevice(manager, device, token);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    handle = NULL;
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    handle1 = NULL;
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle1);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    ok(get_refcount(manager) == 1, "Unexpected refcount %u.\n", get_refcount(manager));

    hr = IDirect3DDeviceManager9_GetVideoService(manager, handle, &IID_IDirectXVideoProcessorService,
            (void **)&service);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    memset(&video_desc, 0, sizeof(video_desc));
    video_desc.SampleWidth = 64;
    video_desc.SampleHeight = 64;
    video_desc.Format = D3DFMT_A8R8G8B8;

    hr = IDirectXVideoProcessorService_CreateVideoProcessor(service, &DXVA2_VideoProcSoftwareDevice, &video_desc,
            D3DFMT_A8R8G8B8, 1, &processor);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirectXVideoProcessorService_CreateVideoProcessor(service, &DXVA2_VideoProcSoftwareDevice, &video_desc,
            D3DFMT_A8R8G8B8, 1, &processor2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(processor2 != processor, "Unexpected instance.\n");

    hr = IDirectXVideoProcessor_GetCreationParameters(processor, NULL, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IDirectXVideoProcessor_GetCreationParameters(processor, &guid, NULL, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(IsEqualGUID(&guid, &DXVA2_VideoProcSoftwareDevice), "Unexpected device guid.\n");

    hr = IDirectXVideoProcessor_GetCreationParameters(processor, NULL, NULL, &format, NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(format == D3DFMT_A8R8G8B8, "Unexpected format %u.\n", format);

    IDirectXVideoProcessor_Release(processor);
    IDirectXVideoProcessor_Release(processor2);

    hr = IDirect3DDeviceManager9_GetVideoService(manager, handle, &IID_IDirectXVideoProcessorService,
            (void **)&service2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(service == service2, "Unexpected pointer.\n");

    IDirectXVideoProcessorService_Release(service2);
    IDirectXVideoProcessorService_Release(service);

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle1);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    IDirect3DDevice9_Release(device);
    IDirect3DDeviceManager9_Release(manager);

done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

START_TEST(dxva2)
{
    test_device_manager();
    test_video_processor();
}
