/*
 * Copyright 2011 Henri Verbeet for CodeWeavers
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

#include "wine/test.h"
#include "d3d.h"

struct create_window_thread_param
{
    HWND window;
    HANDLE window_created;
    HANDLE destroy_window;
    HANDLE thread;
};

static BOOL compare_color(D3DCOLOR c1, D3DCOLOR c2, BYTE max_diff)
{
    if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff) return FALSE;
    c1 >>= 8; c2 >>= 8;
    if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff) return FALSE;
    c1 >>= 8; c2 >>= 8;
    if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff) return FALSE;
    c1 >>= 8; c2 >>= 8;
    if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff) return FALSE;
    return TRUE;
}

static DWORD WINAPI create_window_thread_proc(void *param)
{
    struct create_window_thread_param *p = param;
    DWORD res;
    BOOL ret;

    p->window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    ret = SetEvent(p->window_created);
    ok(ret, "SetEvent failed, last error %#x.\n", GetLastError());

    for (;;)
    {
        MSG msg;

        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
            DispatchMessage(&msg);
        res = WaitForSingleObject(p->destroy_window, 100);
        if (res == WAIT_OBJECT_0)
            break;
        if (res != WAIT_TIMEOUT)
        {
            ok(0, "Wait failed (%#x), last error %#x.\n", res, GetLastError());
            break;
        }
    }

    DestroyWindow(p->window);

    return 0;
}

static void create_window_thread(struct create_window_thread_param *p)
{
    DWORD res, tid;

    p->window_created = CreateEvent(NULL, FALSE, FALSE, NULL);
    ok(!!p->window_created, "CreateEvent failed, last error %#x.\n", GetLastError());
    p->destroy_window = CreateEvent(NULL, FALSE, FALSE, NULL);
    ok(!!p->destroy_window, "CreateEvent failed, last error %#x.\n", GetLastError());
    p->thread = CreateThread(NULL, 0, create_window_thread_proc, p, 0, &tid);
    ok(!!p->thread, "Failed to create thread, last error %#x.\n", GetLastError());
    res = WaitForSingleObject(p->window_created, INFINITE);
    ok(res == WAIT_OBJECT_0, "Wait failed (%#x), last error %#x.\n", res, GetLastError());
}

static void destroy_window_thread(struct create_window_thread_param *p)
{
    SetEvent(p->destroy_window);
    WaitForSingleObject(p->thread, INFINITE);
    CloseHandle(p->destroy_window);
    CloseHandle(p->window_created);
    CloseHandle(p->thread);
}

static D3DCOLOR get_surface_color(IDirectDrawSurface *surface, UINT x, UINT y)
{
    RECT rect = {x, y, x + 1, y + 1};
    DDSURFACEDESC surface_desc;
    D3DCOLOR color;
    HRESULT hr;

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);

    hr = IDirectDrawSurface_Lock(surface, &rect, &surface_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    if (FAILED(hr))
        return 0xdeadbeef;

    color = *((DWORD *)surface_desc.lpSurface) & 0x00ffffff;

    hr = IDirectDrawSurface_Unlock(surface, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    return color;
}

static HRESULT CALLBACK enum_z_fmt(GUID *guid, char *description, char *name,
        D3DDEVICEDESC *hal_desc, D3DDEVICEDESC *hel_desc, void *ctx)
{
    DWORD *z_depth = ctx;

    if (!IsEqualGUID(&IID_IDirect3DHALDevice, guid))
        return D3DENUMRET_OK;

    if (hal_desc->dwDeviceZBufferBitDepth & DDBD_32)
        *z_depth = 32;
    else if (hal_desc->dwDeviceZBufferBitDepth & DDBD_24)
        *z_depth = 24;
    else if (hal_desc->dwDeviceZBufferBitDepth & DDBD_16)
        *z_depth = 16;

    return DDENUMRET_OK;
}

static IDirectDraw2 *create_ddraw(void)
{
    IDirectDraw2 *ddraw2;
    IDirectDraw *ddraw1;
    HRESULT hr;

    if (FAILED(DirectDrawCreate(NULL, &ddraw1, NULL)))
        return NULL;

    hr = IDirectDraw_QueryInterface(ddraw1, &IID_IDirectDraw2, (void **)&ddraw2);
    IDirectDraw_Release(ddraw1);
    if (FAILED(hr))
        return NULL;

    return ddraw2;
}

static IDirect3DDevice2 *create_device(IDirectDraw2 *ddraw, HWND window, DWORD coop_level)
{
    IDirectDrawSurface *surface, *ds;
    IDirect3DDevice2 *device = NULL;
    DDSURFACEDESC surface_desc;
    DWORD z_depth = 0;
    IDirect3D2 *d3d;
    HRESULT hr;

    hr = IDirectDraw2_SetCooperativeLevel(ddraw, window, coop_level);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    surface_desc.dwWidth = 640;
    surface_desc.dwHeight = 480;

    hr = IDirectDraw2_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    if (coop_level & DDSCL_NORMAL)
    {
        IDirectDrawClipper *clipper;

        hr = IDirectDraw2_CreateClipper(ddraw, 0, &clipper, NULL);
        ok(SUCCEEDED(hr), "Failed to create clipper, hr %#x.\n", hr);
        hr = IDirectDrawClipper_SetHWnd(clipper, 0, window);
        ok(SUCCEEDED(hr), "Failed to set clipper window, hr %#x.\n", hr);
        hr = IDirectDrawSurface_SetClipper(surface, clipper);
        ok(SUCCEEDED(hr), "Failed to set surface clipper, hr %#x.\n", hr);
        IDirectDrawClipper_Release(clipper);
    }

    hr = IDirectDraw2_QueryInterface(ddraw, &IID_IDirect3D2, (void **)&d3d);
    if (FAILED(hr))
    {
        IDirectDrawSurface_Release(surface);
        return NULL;
    }

    hr = IDirect3D2_EnumDevices(d3d, enum_z_fmt, &z_depth);
    ok(SUCCEEDED(hr), "Failed to enumerate z-formats, hr %#x.\n", hr);
    if (FAILED(hr) || !z_depth)
    {
        IDirect3D2_Release(d3d);
        IDirectDrawSurface_Release(surface);
        return NULL;
    }

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_ZBUFFERBITDEPTH | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    U2(surface_desc).dwZBufferBitDepth = z_depth;
    surface_desc.dwWidth = 640;
    surface_desc.dwHeight = 480;
    hr = IDirectDraw2_CreateSurface(ddraw, &surface_desc, &ds, NULL);
    ok(SUCCEEDED(hr), "Failed to create depth buffer, hr %#x.\n", hr);
    if (FAILED(hr))
    {
        IDirect3D2_Release(d3d);
        IDirectDrawSurface_Release(surface);
        return NULL;
    }

    hr = IDirectDrawSurface_AddAttachedSurface(surface, ds);
    ok(SUCCEEDED(hr), "Failed to attach depth buffer, hr %#x.\n", hr);
    IDirectDrawSurface_Release(ds);
    if (FAILED(hr))
    {
        IDirect3D2_Release(d3d);
        IDirectDrawSurface_Release(surface);
        return NULL;
    }

    hr = IDirect3D2_CreateDevice(d3d, &IID_IDirect3DHALDevice, surface, &device);
    IDirect3D2_Release(d3d);
    IDirectDrawSurface_Release(surface);
    if (FAILED(hr))
        return NULL;

    return device;
}

static HRESULT CALLBACK restore_callback(IDirectDrawSurface *surface, DDSURFACEDESC *desc, void *context)
{
    HRESULT hr = IDirectDrawSurface_Restore(surface);
    ok(SUCCEEDED(hr), "Failed to restore surface, hr %#x.\n", hr);
    IDirectDrawSurface_Release(surface);

    return DDENUMRET_OK;
}

static HRESULT restore_surfaces(IDirectDraw2 *ddraw)
{
    return IDirectDraw2_EnumSurfaces(ddraw, DDENUMSURFACES_ALL | DDENUMSURFACES_DOESEXIST,
            NULL, NULL, restore_callback);
}

static void test_coop_level_create_device_window(void)
{
    HWND focus_window, device_window;
    IDirectDraw2 *ddraw;
    HRESULT hr;

    focus_window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(ddraw = create_ddraw()))
    {
        skip("Failed to create a ddraw object, skipping test.\n");
        DestroyWindow(focus_window);
        return;
    }

    hr = IDirectDraw2_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw2_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw2_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW | DDSCL_NORMAL);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw2_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW | DDSCL_NORMAL | DDSCL_FULLSCREEN);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw2_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DDERR_NOFOCUSWINDOW || broken(hr == DDERR_INVALIDPARAMS), "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");

    /* Windows versions before 98 / NT5 don't support DDSCL_CREATEDEVICEWINDOW. */
    if (broken(hr == DDERR_INVALIDPARAMS))
    {
        win_skip("DDSCL_CREATEDEVICEWINDOW not supported, skipping test.\n");
        IDirectDraw2_Release(ddraw);
        DestroyWindow(focus_window);
        return;
    }

    hr = IDirectDraw2_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw2_SetCooperativeLevel(ddraw, focus_window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");

    hr = IDirectDraw2_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw2_SetCooperativeLevel(ddraw, NULL, DDSCL_SETFOCUSWINDOW
            | DDSCL_CREATEDEVICEWINDOW | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DDERR_NOHWND, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!!device_window, "Device window not found.\n");

    hr = IDirectDraw2_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw2_SetCooperativeLevel(ddraw, focus_window, DDSCL_SETFOCUSWINDOW
            | DDSCL_CREATEDEVICEWINDOW | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!!device_window, "Device window not found.\n");

    hr = IDirectDraw2_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw2_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DDERR_NOFOCUSWINDOW, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw2_SetCooperativeLevel(ddraw, focus_window, DDSCL_SETFOCUSWINDOW);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw2_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!!device_window, "Device window not found.\n");

    IDirectDraw2_Release(ddraw);
    DestroyWindow(focus_window);
}

static void test_clipper_blt(void)
{
    IDirectDrawSurface *src_surface, *dst_surface;
    RECT client_rect, src_rect, *rect;
    IDirectDrawClipper *clipper;
    DDSURFACEDESC surface_desc;
    unsigned int i, j, x, y;
    IDirectDraw2 *ddraw;
    RGNDATA *rgn_data;
    D3DCOLOR color;
    HRGN r1, r2;
    HWND window;
    DDBLTFX fx;
    HRESULT hr;
    DWORD *ptr;
    DWORD ret;

    static const DWORD src_data[] =
    {
        0xff0000ff, 0xff0000ff, 0xff00ff00, 0xffff0000, 0xffffffff, 0xffffffff,
        0xff0000ff, 0xff0000ff, 0xff00ff00, 0xffff0000, 0xffffffff, 0xffffffff,
        0xff0000ff, 0xff0000ff, 0xff00ff00, 0xffff0000, 0xffffffff, 0xffffffff,
    };
    static const D3DCOLOR expected1[] =
    {
        0x000000ff, 0x0000ff00, 0x00000000, 0x00000000,
        0x000000ff, 0x0000ff00, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00ff0000, 0x00ffffff,
        0x00000000, 0x00000000, 0x00ff0000, 0x00ffffff,
    };
    static const D3DCOLOR expected2[] =
    {
        0x000000ff, 0x000000ff, 0x00000000, 0x00000000,
        0x000000ff, 0x000000ff, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x000000ff, 0x000000ff,
        0x00000000, 0x00000000, 0x000000ff, 0x000000ff,
    };

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            10, 10, 640, 480, 0, 0, 0, 0);
    ShowWindow(window, SW_SHOW);
    if (!(ddraw = create_ddraw()))
    {
        skip("Failed to create a ddraw object, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    ret = GetClientRect(window, &client_rect);
    ok(ret, "Failed to get client rect.\n");
    ret = MapWindowPoints(window, NULL, (POINT *)&client_rect, 2);
    ok(ret, "Failed to map client rect.\n");

    hr = IDirectDraw2_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    hr = IDirectDraw2_CreateClipper(ddraw, 0, &clipper, NULL);
    ok(SUCCEEDED(hr), "Failed to create clipper, hr %#x.\n", hr);
    hr = IDirectDrawClipper_GetClipList(clipper, NULL, NULL, &ret);
    ok(hr == DDERR_NOCLIPLIST, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawClipper_SetHWnd(clipper, 0, window);
    ok(SUCCEEDED(hr), "Failed to set clipper window, hr %#x.\n", hr);
    hr = IDirectDrawClipper_GetClipList(clipper, NULL, NULL, &ret);
    ok(SUCCEEDED(hr), "Failed to get clip list size, hr %#x.\n", hr);
    rgn_data = HeapAlloc(GetProcessHeap(), 0, ret);
    hr = IDirectDrawClipper_GetClipList(clipper, NULL, rgn_data, &ret);
    ok(SUCCEEDED(hr), "Failed to get clip list, hr %#x.\n", hr);
    ok(rgn_data->rdh.dwSize == sizeof(rgn_data->rdh), "Got unexpected structure size %#x.\n", rgn_data->rdh.dwSize);
    ok(rgn_data->rdh.iType == RDH_RECTANGLES, "Got unexpected type %#x.\n", rgn_data->rdh.iType);
    ok(rgn_data->rdh.nCount == 1, "Got unexpected count %u.\n", rgn_data->rdh.nCount);
    ok(rgn_data->rdh.nRgnSize == 16 || broken(rgn_data->rdh.nRgnSize == 168 /* NT4 */),
            "Got unexpected region size %u.\n", rgn_data->rdh.nRgnSize);
    ok(EqualRect(&rgn_data->rdh.rcBound, &client_rect),
            "Got unexpected bounding rect {%d, %d, %d, %d}, expected {%d, %d, %d, %d}.\n",
            rgn_data->rdh.rcBound.left, rgn_data->rdh.rcBound.top,
            rgn_data->rdh.rcBound.right, rgn_data->rdh.rcBound.bottom,
            client_rect.left, client_rect.top, client_rect.right, client_rect.bottom);
    rect = (RECT *)&rgn_data->Buffer[0];
    ok(EqualRect(rect, &client_rect),
            "Got unexpected clip rect {%d, %d, %d, %d}, expected {%d, %d, %d, %d}.\n",
            rect->left, rect->top, rect->right, rect->bottom,
            client_rect.left, client_rect.top, client_rect.right, client_rect.bottom);
    HeapFree(GetProcessHeap(), 0, rgn_data);

    r1 = CreateRectRgn(0, 0, 320, 240);
    ok(!!r1, "Failed to create region.\n");
    r2 = CreateRectRgn(320, 240, 640, 480);
    ok(!!r2, "Failed to create region.\n");
    CombineRgn(r1, r1, r2, RGN_OR);
    ret = GetRegionData(r1, 0, NULL);
    rgn_data = HeapAlloc(GetProcessHeap(), 0, ret);
    ret = GetRegionData(r1, ret, rgn_data);
    ok(!!ret, "Failed to get region data.\n");

    DeleteObject(r2);
    DeleteObject(r1);

    hr = IDirectDrawClipper_SetClipList(clipper, rgn_data, 0);
    ok(hr == DDERR_CLIPPERISUSINGHWND, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawClipper_SetHWnd(clipper, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to set clipper window, hr %#x.\n", hr);
    hr = IDirectDrawClipper_SetClipList(clipper, rgn_data, 0);
    ok(SUCCEEDED(hr), "Failed to set clip list, hr %#x.\n", hr);

    HeapFree(GetProcessHeap(), 0, rgn_data);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    surface_desc.dwWidth = 640;
    surface_desc.dwHeight = 480;
    surface_desc.ddpfPixelFormat.dwSize = sizeof(surface_desc.ddpfPixelFormat);
    surface_desc.ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(surface_desc.ddpfPixelFormat).dwRGBBitCount = 32;
    U2(surface_desc.ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(surface_desc.ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(surface_desc.ddpfPixelFormat).dwBBitMask = 0x000000ff;

    hr = IDirectDraw2_CreateSurface(ddraw, &surface_desc, &src_surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create source surface, hr %#x.\n", hr);
    hr = IDirectDraw2_CreateSurface(ddraw, &surface_desc, &dst_surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create destination surface, hr %#x.\n", hr);

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    hr = IDirectDrawSurface_Blt(src_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear source surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface_Blt(dst_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear destination surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_Lock(src_surface, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock source surface, hr %#x.\n", hr);
    ok(U1(surface_desc).lPitch == 2560, "Got unexpected surface pitch %u.\n", U1(surface_desc).lPitch);
    ptr = surface_desc.lpSurface;
    memcpy(&ptr[   0], &src_data[ 0], 6 * sizeof(DWORD));
    memcpy(&ptr[ 640], &src_data[ 6], 6 * sizeof(DWORD));
    memcpy(&ptr[1280], &src_data[12], 6 * sizeof(DWORD));
    hr = IDirectDrawSurface_Unlock(src_surface, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock source surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_SetClipper(dst_surface, clipper);
    ok(SUCCEEDED(hr), "Failed to set clipper, hr %#x.\n", hr);

    SetRect(&src_rect, 1, 1, 5, 2);
    hr = IDirectDrawSurface_Blt(dst_surface, NULL, src_surface, &src_rect, DDBLT_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);
    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            x = 80 * ((2 * j) + 1);
            y = 60 * ((2 * i) + 1);
            color = get_surface_color(dst_surface, x, y);
            ok(compare_color(color, expected1[i * 4 + j], 1),
                    "Expected color 0x%08x at %u,%u, got 0x%08x.\n", expected1[i * 4 + j], x, y, color);
        }
    }

    U5(fx).dwFillColor = 0xff0000ff;
    hr = IDirectDrawSurface_Blt(dst_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear destination surface, hr %#x.\n", hr);
    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            x = 80 * ((2 * j) + 1);
            y = 60 * ((2 * i) + 1);
            color = get_surface_color(dst_surface, x, y);
            ok(compare_color(color, expected2[i * 4 + j], 1),
                    "Expected color 0x%08x at %u,%u, got 0x%08x.\n", expected2[i * 4 + j], x, y, color);
        }
    }

    hr = IDirectDrawSurface_BltFast(dst_surface, 0, 0, src_surface, NULL, DDBLTFAST_WAIT);
    ok(hr == DDERR_BLTFASTCANTCLIP || broken(hr == E_NOTIMPL /* NT4 */), "Got unexpected hr %#x.\n", hr);

    hr = IDirectDrawClipper_SetHWnd(clipper, 0, window);
    ok(SUCCEEDED(hr), "Failed to set clipper window, hr %#x.\n", hr);
    hr = IDirectDrawClipper_GetClipList(clipper, NULL, NULL, &ret);
    ok(SUCCEEDED(hr), "Failed to get clip list size, hr %#x.\n", hr);
    DestroyWindow(window);
    hr = IDirectDrawClipper_GetClipList(clipper, NULL, NULL, &ret);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawClipper_SetHWnd(clipper, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to set clipper window, hr %#x.\n", hr);
    hr = IDirectDrawClipper_GetClipList(clipper, NULL, NULL, &ret);
    ok(SUCCEEDED(hr), "Failed to get clip list size, hr %#x.\n", hr);
    hr = IDirectDrawClipper_SetClipList(clipper, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to set clip list, hr %#x.\n", hr);
    hr = IDirectDrawClipper_GetClipList(clipper, NULL, NULL, &ret);
    ok(hr == DDERR_NOCLIPLIST, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Blt(dst_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_NOCLIPLIST, "Got unexpected hr %#x.\n", hr);

    IDirectDrawSurface_Release(dst_surface);
    IDirectDrawSurface_Release(src_surface);
    IDirectDrawClipper_Release(clipper);
    IDirectDraw2_Release(ddraw);
}

static void test_coop_level_d3d_state(void)
{
    D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    D3DMATERIALHANDLE background_handle;
    IDirectDrawSurface *rt, *surface;
    IDirect3DMaterial2 *background;
    IDirect3DViewport2 *viewport;
    IDirect3DDevice2 *device;
    D3DMATERIAL material;
    IDirectDraw2 *ddraw;
    D3DVIEWPORT2 vp;
    IDirect3D2 *d3d;
    D3DCOLOR color;
    DWORD value;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(ddraw = create_ddraw()))
    {
        skip("Failed to create ddraw object, skipping test.\n");
        DestroyWindow(window);
        return;
    }
    if (!(device = create_device(ddraw, window, DDSCL_NORMAL)))
    {
        skip("Failed to create D3D device, skipping test.\n");
        IDirectDraw2_Release(ddraw);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice2_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);
    hr = IDirect3D2_CreateViewport(d3d, &viewport, NULL);
    ok(SUCCEEDED(hr), "Failed to create viewport, hr %#x.\n", hr);
    hr = IDirect3D2_CreateMaterial(d3d, &background, NULL);
    ok(SUCCEEDED(hr), "Failed to create material, hr %#x.\n", hr);
    IDirect3D2_Release(d3d);

    hr = IDirect3DDevice2_AddViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to add viewport, hr %#x.\n", hr);
    memset(&vp, 0, sizeof(vp));
    vp.dwSize = sizeof(vp);
    vp.dwX = 0;
    vp.dwY = 0;
    vp.dwWidth = 640;
    vp.dwHeight = 480;
    vp.dvClipX = -1.0f;
    vp.dvClipY =  1.0f;
    vp.dvClipWidth = 2.0f;
    vp.dvClipHeight = 2.0f;
    vp.dvMinZ = 0.0f;
    vp.dvMaxZ = 1.0f;
    hr = IDirect3DViewport2_SetViewport2(viewport, &vp);
    ok(SUCCEEDED(hr), "Failed to set viewport data, hr %#x.\n", hr);

    memset(&material, 0, sizeof(material));
    material.dwSize = sizeof(material);
    U1(U(material).diffuse).r = 1.0f;
    U2(U(material).diffuse).g = 0.0f;
    U3(U(material).diffuse).b = 0.0f;
    U4(U(material).diffuse).a = 1.0f;
    hr = IDirect3DMaterial2_SetMaterial(background, &material);
    ok(SUCCEEDED(hr), "Failed to set material data, hr %#x.\n", hr);
    hr = IDirect3DMaterial2_GetHandle(background, device, &background_handle);
    ok(SUCCEEDED(hr), "Failed to get material handle, hr %#x.\n", hr);
    hr = IDirect3DViewport2_SetBackground(viewport, background_handle);
    ok(SUCCEEDED(hr), "Failed to set viewport background, hr %#x.\n", hr);

    hr = IDirect3DDevice2_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);
    hr = IDirect3DDevice2_GetRenderState(device, D3DRENDERSTATE_ZENABLE, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
    ok(!!value, "Got unexpected z-enable state %#x.\n", value);
    hr = IDirect3DDevice2_GetRenderState(device, D3DRENDERSTATE_ALPHABLENDENABLE, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
    ok(!value, "Got unexpected alpha blend enable state %#x.\n", value);
    hr = IDirect3DDevice2_SetRenderState(device, D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DViewport2_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x00ff0000, 1), "Got unexpected color 0x%08x.\n", color);

    hr = IDirectDraw2_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    hr = IDirectDrawSurface_IsLost(rt);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);
    hr = restore_surfaces(ddraw);
    ok(SUCCEEDED(hr), "Failed to restore surfaces, hr %#x.\n", hr);

    memset(&material, 0, sizeof(material));
    material.dwSize = sizeof(material);
    U1(U(material).diffuse).r = 0.0f;
    U2(U(material).diffuse).g = 1.0f;
    U3(U(material).diffuse).b = 0.0f;
    U4(U(material).diffuse).a = 1.0f;
    hr = IDirect3DMaterial2_SetMaterial(background, &material);
    ok(SUCCEEDED(hr), "Failed to set material data, hr %#x.\n", hr);

    hr = IDirect3DDevice2_GetRenderTarget(device, &surface);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);
    ok(surface == rt, "Got unexpected surface %p.\n", surface);
    hr = IDirect3DDevice2_GetRenderState(device, D3DRENDERSTATE_ZENABLE, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
    ok(!!value, "Got unexpected z-enable state %#x.\n", value);
    hr = IDirect3DDevice2_GetRenderState(device, D3DRENDERSTATE_ALPHABLENDENABLE, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
    ok(!!value, "Got unexpected alpha blend enable state %#x.\n", value);
    hr = IDirect3DViewport2_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x0000ff00, 1), "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DDevice2_DeleteViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to delete viewport, hr %#x.\n", hr);
    IDirect3DMaterial2_Release(background);
    IDirect3DViewport2_Release(viewport);
    IDirectDrawSurface_Release(surface);
    IDirectDrawSurface_Release(rt);
    IDirect3DDevice2_Release(device);
    IDirectDraw2_Release(ddraw);
    DestroyWindow(window);
}

static void test_surface_interface_mismatch(void)
{
    IDirectDraw2 *ddraw = NULL;
    IDirect3D2 *d3d = NULL;
    IDirectDrawSurface *surface = NULL, *ds;
    IDirectDrawSurface3 *surface3 = NULL;
    IDirect3DDevice2 *device = NULL;
    IDirect3DViewport2 *viewport = NULL;
    IDirect3DMaterial2 *background = NULL;
    DDSURFACEDESC surface_desc;
    DWORD z_depth = 0;
    HRESULT hr;
    D3DCOLOR color;
    HWND window;
    D3DVIEWPORT2 vp;
    D3DMATERIAL material;
    D3DMATERIALHANDLE background_handle;
    D3DRECT clear_rect = {{0}, {0}, {640}, {480}};

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);

    if (!(ddraw = create_ddraw()))
    {
        skip("Failed to create a ddraw object, skipping test.\n");
        goto cleanup;
    }

    hr = IDirectDraw2_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    surface_desc.dwWidth = 640;
    surface_desc.dwHeight = 480;

    hr = IDirectDraw2_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface2_QueryInterface(surface, &IID_IDirectDrawSurface3, (void **)&surface3);
    if (FAILED(hr))
    {
        skip("Failed to get the IDirectDrawSurface3 interface, skipping test.\n");
        goto cleanup;
    }

    hr = IDirectDraw2_QueryInterface(ddraw, &IID_IDirect3D2, (void **)&d3d);
    if (FAILED(hr))
    {
        skip("Failed to get the IDirect3D2 interface, skipping test.\n");
        goto cleanup;
    }

    hr = IDirect3D2_EnumDevices(d3d, enum_z_fmt, &z_depth);
    if (FAILED(hr) || !z_depth)
    {
        skip("No depth buffer formats available, skipping test.\n");
        goto cleanup;
    }

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_ZBUFFERBITDEPTH | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    surface_desc.dwZBufferBitDepth = z_depth;
    surface_desc.dwWidth = 640;
    surface_desc.dwHeight = 480;
    hr = IDirectDraw2_CreateSurface(ddraw, &surface_desc, &ds, NULL);
    ok(SUCCEEDED(hr), "Failed to create depth buffer, hr %#x.\n", hr);
    if (FAILED(hr))
        goto cleanup;

    /* Using a different surface interface version still works */
    hr = IDirectDrawSurface3_AddAttachedSurface(surface3, (IDirectDrawSurface3 *)ds);
    ok(SUCCEEDED(hr), "Failed to attach depth buffer, hr %#x.\n", hr);
    IDirectDrawSurface_Release(ds);
    if (FAILED(hr))
        goto cleanup;

    /* Here too */
    hr = IDirect3D2_CreateDevice(d3d, &IID_IDirect3DHALDevice, (IDirectDrawSurface *)surface3, &device);
    ok(SUCCEEDED(hr), "Failed to create d3d device.\n");
    if (FAILED(hr))
        goto cleanup;

    hr = IDirect3D2_CreateViewport(d3d, &viewport, NULL);
    ok(SUCCEEDED(hr), "Failed to create viewport, hr %#x.\n", hr);
    hr = IDirect3D2_CreateMaterial(d3d, &background, NULL);
    ok(SUCCEEDED(hr), "Failed to create material, hr %#x.\n", hr);

    hr = IDirect3DDevice2_AddViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to add viewport, hr %#x.\n", hr);
    memset(&vp, 0, sizeof(vp));
    vp.dwSize = sizeof(vp);
    vp.dwX = 0;
    vp.dwY = 0;
    vp.dwWidth = 640;
    vp.dwHeight = 480;
    vp.dvClipX = -1.0f;
    vp.dvClipY =  1.0f;
    vp.dvClipWidth = 2.0f;
    vp.dvClipHeight = 2.0f;
    vp.dvMinZ = 0.0f;
    vp.dvMaxZ = 1.0f;
    hr = IDirect3DViewport2_SetViewport2(viewport, &vp);
    ok(SUCCEEDED(hr), "Failed to set viewport data, hr %#x.\n", hr);

    memset(&material, 0, sizeof(material));
    material.dwSize = sizeof(material);
    U1(U(material).diffuse).r = 1.0f;
    U2(U(material).diffuse).g = 0.0f;
    U3(U(material).diffuse).b = 0.0f;
    U4(U(material).diffuse).a = 1.0f;
    hr = IDirect3DMaterial2_SetMaterial(background, &material);
    ok(SUCCEEDED(hr), "Failed to set material data, hr %#x.\n", hr);
    hr = IDirect3DMaterial2_GetHandle(background, device, &background_handle);
    ok(SUCCEEDED(hr), "Failed to get material handle, hr %#x.\n", hr);
    hr = IDirect3DViewport2_SetBackground(viewport, background_handle);
    ok(SUCCEEDED(hr), "Failed to set viewport background, hr %#x.\n", hr);

    hr = IDirect3DViewport2_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear render target, hr %#x.\n", hr);
    color = get_surface_color(surface, 320, 240);
    ok(compare_color(color, 0x00ff0000, 1), "Got unexpected color 0x%08x.\n", color);

cleanup:
    if (viewport)
    {
        IDirect3DDevice2_DeleteViewport(device, viewport);
        IDirect3DViewport2_Release(viewport);
    }
    if (background) IDirect3DMaterial2_Release(background);
    if (surface3) IDirectDrawSurface3_Release(surface3);
    if (surface) IDirectDrawSurface7_Release(surface);
    if (device) IDirect3DDevice2_Release(device);
    if (d3d) IDirect3D7_Release(d3d);
    if (ddraw) IDirectDraw7_Release(ddraw);
    DestroyWindow(window);
}

static void test_coop_level_threaded(void)
{
    struct create_window_thread_param p;
    IDirectDraw2 *ddraw;
    HRESULT hr;

    if (!(ddraw = create_ddraw()))
    {
        skip("Failed to create a ddraw object, skipping test.\n");
        return;
    }
    create_window_thread(&p);

    hr = IDirectDraw2_SetCooperativeLevel(ddraw, p.window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    IDirectDraw2_Release(ddraw);
    destroy_window_thread(&p);
}

static IDirectDrawSurface *get_depth_stencil(IDirect3DDevice2 *device)
{
    IDirectDrawSurface *rt, *ret;
    DDSCAPS caps = {DDSCAPS_ZBUFFER};
    HRESULT hr;

    hr = IDirect3DDevice2_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);
    hr = IDirectDrawSurface_GetAttachedSurface(rt, &caps, &ret);
    ok(SUCCEEDED(hr) || hr == DDERR_NOTFOUND, "Failed to get the z buffer, hr %#x.\n", hr);
    IDirectDrawSurface_Release(rt);
    return ret;
}

static void test_depth_blit(void)
{
    static D3DLVERTEX quad1[] =
    {
        {{-1.0}, { 1.0}, {0.50f}, 0, {0xff00ff00}},
        {{ 1.0}, { 1.0}, {0.50f}, 0, {0xff00ff00}},
        {{-1.0}, {-1.0}, {0.50f}, 0, {0xff00ff00}},
        {{ 1.0}, {-1.0}, {0.50f}, 0, {0xff00ff00}},
    };
    static const D3DCOLOR expected_colors[4][4] =
    {
        {0x00ff0000, 0x00ff0000, 0x0000ff00, 0x0000ff00},
        {0x00ff0000, 0x00ff0000, 0x0000ff00, 0x0000ff00},
        {0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00},
        {0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00},
    };
    static const BOOL todo[4][4] =
    {
        {FALSE, FALSE, TRUE, TRUE},
        {FALSE, FALSE, TRUE, TRUE},
        {TRUE,  TRUE,  TRUE, TRUE},
        {TRUE,  TRUE,  TRUE, TRUE},
    };
    DDSURFACEDESC ddsd_new, ddsd_existing;

    IDirect3DDevice2 *device;
    IDirectDrawSurface *ds1, *ds2, *ds3, *rt;
    IDirect3DViewport2 *viewport;
    D3DVIEWPORT2 vp_data;
    RECT src_rect, dst_rect;
    unsigned int i, j;
    D3DCOLOR color;
    HRESULT hr;
    IDirect3D2 *d3d;
    IDirectDraw2 *ddraw;
    DDBLTFX fx;
    HWND window;
    D3DRECT d3drect;
    IDirect3DMaterial2 *background;
    D3DMATERIALHANDLE background_handle;
    D3DMATERIAL material;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(ddraw = create_ddraw()))
    {
        skip("Failed to create ddraw object, skipping test.\n");
        DestroyWindow(window);
        return;
    }
    if (!(device = create_device(ddraw, window, DDSCL_NORMAL)))
    {
        skip("Failed to create D3D device, skipping test.\n");
        IDirectDraw2_Release(ddraw);
        DestroyWindow(window);
        return;
    }

    hr = IDirectDraw2_QueryInterface(ddraw, &IID_IDirect3D2, (void **)&d3d);
    ok(SUCCEEDED(hr), "Failed to get Direct3D2 interface, hr %#x.\n", hr);
    hr = IDirect3D2_CreateViewport(d3d, &viewport, NULL);
    ok(SUCCEEDED(hr), "Failed to create a viewport, hr %#x.\n", hr);
    hr = IDirect3D2_CreateMaterial(d3d, &background, NULL);
    ok(SUCCEEDED(hr), "Failed to create a material, hr %#x.\n", hr);

    ds1 = get_depth_stencil(device);

    memset(&ddsd_new, 0, sizeof(ddsd_new));
    ddsd_new.dwSize = sizeof(ddsd_new);
    memset(&ddsd_existing, 0, sizeof(ddsd_existing));
    ddsd_existing.dwSize = sizeof(ddsd_existing);
    hr = IDirectDrawSurface_GetSurfaceDesc(ds1, &ddsd_existing);
    ddsd_new.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    ddsd_new.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    ddsd_new.dwWidth = ddsd_existing.dwWidth;
    ddsd_new.dwHeight = ddsd_existing.dwHeight;
    U4(ddsd_new).ddpfPixelFormat = U4(ddsd_existing).ddpfPixelFormat;
    hr = IDirectDraw2_CreateSurface(ddraw, &ddsd_new, &ds2, NULL);
    ok(SUCCEEDED(hr), "Failed to create a surface, hr %#x.\n", hr);
    hr = IDirectDraw2_CreateSurface(ddraw, &ddsd_new, &ds3, NULL);
    ok(SUCCEEDED(hr), "Failed to create a surface, hr %#x.\n", hr);

    hr = IDirect3DDevice2_AddViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to add viewport to device, hr %#x.\n", hr);
    memset(&vp_data, 0, sizeof(vp_data));
    vp_data.dwSize = sizeof(vp_data);
    vp_data.dwWidth = ddsd_existing.dwWidth;
    vp_data.dwHeight = ddsd_existing.dwHeight;
    vp_data.dvMaxZ = 1.0;
    vp_data.dvClipX = -1.0f;
    vp_data.dvClipWidth = 2.0f;
    vp_data.dvClipY = 1.0f;
    vp_data.dvClipHeight = 2.0f;
    hr = IDirect3DViewport2_SetViewport2(viewport, &vp_data);
    ok(SUCCEEDED(hr), "Failed to set viewport data, hr %#x.\n", hr);
    hr = IDirect3DDevice2_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to activate the viewport, hr %#x.\n", hr);

    memset(&material, 0, sizeof(material));
    material.dwSize = sizeof(material);
    U1(U(material).diffuse).r = 1.0f;
    U2(U(material).diffuse).g = 0.0f;
    U3(U(material).diffuse).b = 0.0f;
    U4(U(material).diffuse).a = 1.0f;
    hr = IDirect3DMaterial2_SetMaterial(background, &material);
    ok(SUCCEEDED(hr), "Failed to set material data, hr %#x.\n", hr);
    hr = IDirect3DMaterial2_GetHandle(background, device, &background_handle);
    ok(SUCCEEDED(hr), "Failed to get material handle, hr %#x.\n", hr);
    hr = IDirect3DViewport2_SetBackground(viewport, background_handle);
    ok(SUCCEEDED(hr), "Failed to set viewport background, hr %#x.\n", hr);

    hr = IDirect3DDevice2_SetRenderState(device, D3DRENDERSTATE_ZENABLE, D3DZB_TRUE);
    ok(SUCCEEDED(hr), "Failed to enable z testing, hr %#x.\n", hr);
    hr = IDirect3DDevice2_SetRenderState(device, D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL);
    ok(SUCCEEDED(hr), "Failed to set the z function, hr %#x.\n", hr);

    U1(d3drect).x1 = U2(d3drect).y1 = 0;
    U3(d3drect).x2 = vp_data.dwWidth; U4(d3drect).y2 = vp_data.dwHeight;
    hr = IDirect3DViewport2_Clear(viewport, 1, &d3drect, D3DCLEAR_ZBUFFER);
    ok(SUCCEEDED(hr), "Failed to clear the z buffer, hr %#x.\n", hr);

    /* Partial blit. */
    SetRect(&src_rect, 0, 0, 320, 240);
    SetRect(&dst_rect, 0, 0, 320, 240);
    hr = IDirectDrawSurface_Blt(ds2, &dst_rect, ds1, &src_rect, DDBLT_WAIT, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    /* Different locations. */
    SetRect(&src_rect, 0, 0, 320, 240);
    SetRect(&dst_rect, 320, 240, 640, 480);
    hr = IDirectDrawSurface_Blt(ds2, &dst_rect, ds1, &src_rect, DDBLT_WAIT, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    /* Streched. */
    SetRect(&src_rect, 0, 0, 320, 240);
    SetRect(&dst_rect, 0, 0, 640, 480);
    hr = IDirectDrawSurface_Blt(ds2, &dst_rect, ds1, &src_rect, DDBLT_WAIT, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    /* Flipped. */
    SetRect(&src_rect, 0, 480, 640, 0);
    SetRect(&dst_rect, 0, 0, 640, 480);
    hr = IDirectDrawSurface_Blt(ds2, &dst_rect, ds1, &src_rect, DDBLT_WAIT, NULL);
    ok(hr == DDERR_INVALIDRECT, "Got unexpected hr %#x.\n", hr);
    SetRect(&src_rect, 0, 0, 640, 480);
    SetRect(&dst_rect, 0, 480, 640, 0);
    hr = IDirectDrawSurface_Blt(ds2, &dst_rect, ds1, &src_rect, DDBLT_WAIT, NULL);
    ok(hr == DDERR_INVALIDRECT, "Got unexpected hr %#x.\n", hr);
    /* Full, explicit. */
    SetRect(&src_rect, 0, 0, 640, 480);
    SetRect(&dst_rect, 0, 0, 640, 480);
    hr = IDirectDrawSurface_Blt(ds2, &dst_rect, ds1, &src_rect, DDBLT_WAIT, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    /* Depth -> color blit: Succeeds on Win7 + Radeon HD 5700, fails on WinXP + Radeon X1600 */

    /* Depth blit inside a BeginScene / EndScene pair */
    hr = IDirect3DDevice2_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to start a scene, hr %#x.\n", hr);
    /* From the current depth stencil */
    hr = IDirectDrawSurface_Blt(ds2, NULL, ds1, NULL, DDBLT_WAIT, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    /* To the current depth stencil */
    hr = IDirectDrawSurface_Blt(ds1, NULL, ds2, NULL, DDBLT_WAIT, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    /* Between unbound surfaces */
    hr = IDirectDrawSurface_Blt(ds3, NULL, ds2, NULL, DDBLT_WAIT, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice2_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end a scene, hr %#x.\n", hr);

    /* Avoid changing the depth stencil, it doesn't work properly on Windows.
     * Instead use DDBLT_DEPTHFILL to clear the depth stencil. Unfortunately
     * drivers disagree on the meaning of dwFillDepth. Only 0 seems to produce
     * a reliable result(z = 0.0) */
    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    fx.dwFillDepth = 0;
    hr = IDirectDrawSurface_Blt(ds2, NULL, NULL, NULL, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear the source z buffer, hr %#x.\n", hr);

    /* This clears the Z buffer with 1.0 */
    hr = IDirect3DViewport2_Clear(viewport, 1, &d3drect, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear the color and z buffers, hr %#x.\n", hr);

    SetRect(&dst_rect, 0, 0, 320, 240);
    hr = IDirectDrawSurface_Blt(ds1, &dst_rect, ds2, NULL, DDBLT_WAIT, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    IDirectDrawSurface_Release(ds3);
    IDirectDrawSurface_Release(ds2);
    IDirectDrawSurface_Release(ds1);

    hr = IDirect3DDevice2_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to start a scene, hr %#x.\n", hr);
    hr = IDirect3DDevice2_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DVT_LVERTEX, quad1, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice2_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end a scene, hr %#x.\n", hr);

    hr = IDirect3DDevice2_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);
    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            unsigned int x = 80 * ((2 * j) + 1);
            unsigned int y = 60 * ((2 * i) + 1);
            color = get_surface_color(rt, x, y);
            if (todo[i][j])
                todo_wine ok(compare_color(color, expected_colors[i][j], 1),
                        "Expected color 0x%08x at %u,%u, got 0x%08x.\n", expected_colors[i][j], x, y, color);
            else
                ok(compare_color(color, expected_colors[i][j], 1),
                        "Expected color 0x%08x at %u,%u, got 0x%08x.\n", expected_colors[i][j], x, y, color);
        }
    }
    IDirectDrawSurface_Release(rt);

    hr = IDirect3DDevice2_DeleteViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to delete viewport from device, hr %#x.\n", hr);
    IDirect3DMaterial2_Release(background);
    IDirect3DViewport2_Release(viewport);
    IDirect3D2_Release(d3d);
    IDirect3DDevice2_Release(device);
    IDirectDraw2_Release(ddraw);
    DestroyWindow(window);
}

START_TEST(ddraw2)
{
    test_coop_level_create_device_window();
    test_clipper_blt();
    test_coop_level_d3d_state();
    test_surface_interface_mismatch();
    test_coop_level_threaded();
    test_depth_blit();
}
