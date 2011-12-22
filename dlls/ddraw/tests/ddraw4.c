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
#include <limits.h>
#include "d3d.h"

struct vec3
{
    float x, y, z;
};

struct vec4
{
    float x, y, z, w;
};

static BOOL compare_float(float f, float g, unsigned int ulps)
{
    int x = *(int *)&f;
    int y = *(int *)&g;

    if (x < 0)
        x = INT_MIN - x;
    if (y < 0)
        y = INT_MIN - y;

    if (abs(x - y) > ulps)
        return FALSE;

    return TRUE;
}

static BOOL compare_vec4(struct vec4 *vec, float x, float y, float z, float w, unsigned int ulps)
{
    return compare_float(vec->x, x, ulps)
            && compare_float(vec->y, y, ulps)
            && compare_float(vec->z, z, ulps)
            && compare_float(vec->w, w, ulps);
}

static IDirectDraw4 *create_ddraw(void)
{
    IDirectDraw4 *ddraw4;
    IDirectDraw *ddraw1;
    HRESULT hr;

    if (FAILED(DirectDrawCreate(NULL, &ddraw1, NULL)))
        return NULL;

    hr = IDirectDraw_QueryInterface(ddraw1, &IID_IDirectDraw4, (void **)&ddraw4);
    IDirectDraw_Release(ddraw1);
    if (FAILED(hr))
        return NULL;

    return ddraw4;
}

static IDirect3DDevice3 *create_device(HWND window, DWORD coop_level)
{
    IDirect3DDevice3 *device = NULL;
    IDirectDrawSurface4 *surface;
    DDSURFACEDESC2 surface_desc;
    IDirectDraw4 *ddraw4;
    IDirect3D3 *d3d3;
    HRESULT hr;

    if (!(ddraw4 = create_ddraw()))
        return NULL;

    hr = IDirectDraw4_SetCooperativeLevel(ddraw4, window, coop_level);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    surface_desc.dwWidth = 640;
    surface_desc.dwHeight = 480;

    hr = IDirectDraw4_CreateSurface(ddraw4, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    if (coop_level & DDSCL_NORMAL)
    {
        IDirectDrawClipper *clipper;

        hr = IDirectDraw4_CreateClipper(ddraw4, 0, &clipper, NULL);
        ok(SUCCEEDED(hr), "Failed to create clipper, hr %#x.\n", hr);
        hr = IDirectDrawClipper_SetHWnd(clipper, 0, window);
        ok(SUCCEEDED(hr), "Failed to set clipper window, hr %#x.\n", hr);
        hr = IDirectDrawSurface4_SetClipper(surface, clipper);
        ok(SUCCEEDED(hr), "Failed to set surface clipper, hr %#x.\n", hr);
        IDirectDrawClipper_Release(clipper);
    }

    hr = IDirectDraw4_QueryInterface(ddraw4, &IID_IDirect3D3, (void **)&d3d3);
    IDirectDraw4_Release(ddraw4);
    if (FAILED(hr))
    {
        IDirectDrawSurface4_Release(surface);
        return NULL;
    }

    hr = IDirect3D3_CreateDevice(d3d3, &IID_IDirect3DHALDevice, surface, &device, NULL);
    IDirect3D3_Release(d3d3);
    IDirectDrawSurface4_Release(surface);
    if (FAILED(hr))
        return NULL;

    return device;
}

static void test_process_vertices(void)
{
    IDirect3DVertexBuffer *src_vb, *dst_vb;
    IDirect3DViewport3 *viewport;
    D3DVERTEXBUFFERDESC vb_desc;
    IDirect3DDevice3 *device;
    struct vec3 *src_data;
    struct vec4 *dst_data;
    IDirect3D3 *d3d3;
    D3DVIEWPORT2 vp2;
    D3DVIEWPORT vp1;
    HWND window;
    HRESULT hr;

    static D3DMATRIX identity =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    static D3DMATRIX projection =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        6.0f, 7.0f, 8.0f, 1.0f,
    };

    window = CreateWindowA("static", "d3d7_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d3);
    ok(SUCCEEDED(hr), "Failed to get Direct3D3 interface, hr %#x.\n", hr);

    memset(&vb_desc, 0, sizeof(vb_desc));
    vb_desc.dwSize = sizeof(vb_desc);
    vb_desc.dwFVF = D3DFVF_XYZ;
    vb_desc.dwNumVertices = 3;
    hr = IDirect3D3_CreateVertexBuffer(d3d3, &vb_desc, &src_vb, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to create source vertex buffer, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer_Lock(src_vb, DDLOCK_WRITEONLY, (void **)&src_data, NULL);
    ok(SUCCEEDED(hr), "Failed to lock source vertex buffer, hr %#x.\n", hr);
    src_data[0].x = -1.0f;
    src_data[0].y = -1.0f;
    src_data[0].z = -1.0f;
    src_data[1].x = 0.0f;
    src_data[1].y = 0.0f;
    src_data[1].z = 0.0f;
    src_data[2].x = 1.0f;
    src_data[2].y = 1.0f;
    src_data[2].z = 1.0f;
    hr = IDirect3DVertexBuffer_Unlock(src_vb);
    ok(SUCCEEDED(hr), "Failed to unlock source vertex buffer, hr %#x.\n", hr);

    memset(&vb_desc, 0, sizeof(vb_desc));
    vb_desc.dwSize = sizeof(vb_desc);
    vb_desc.dwFVF = D3DFVF_XYZRHW;
    vb_desc.dwNumVertices = 3;
    hr = IDirect3D3_CreateVertexBuffer(d3d3, &vb_desc, &dst_vb, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to create destination vertex buffer, hr %#x.\n", hr);

    hr = IDirect3D3_CreateViewport(d3d3, &viewport, NULL);
    ok(SUCCEEDED(hr), "Failed to create viewport, hr %#x.\n", hr);
    hr = IDirect3DDevice3_AddViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to add viewport, hr %#x.\n", hr);
    vp2.dwSize = sizeof(vp2);
    vp2.dwX = 10;
    vp2.dwY = 20;
    vp2.dwWidth = 100;
    vp2.dwHeight = 200;
    vp2.dvClipX = 2.0f;
    vp2.dvClipY = 3.0f;
    vp2.dvClipWidth = 4.0f;
    vp2.dvClipHeight = 5.0f;
    vp2.dvMinZ = -2.0f;
    vp2.dvMaxZ = 3.0f;
    hr = IDirect3DViewport3_SetViewport2(viewport, &vp2);
    ok(SUCCEEDED(hr), "Failed to set viewport data, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetTransform(device, D3DTRANSFORMSTATE_WORLD, &identity);
    ok(SUCCEEDED(hr), "Failed to set world transformation, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTransform(device, D3DTRANSFORMSTATE_VIEW, &identity);
    ok(SUCCEEDED(hr), "Failed to set view transformation, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTransform(device, D3DTRANSFORMSTATE_PROJECTION, &identity);
    ok(SUCCEEDED(hr), "Failed to set projection transformation, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer_ProcessVertices(dst_vb, D3DVOP_TRANSFORM, 0, 3, src_vb, 0, device, 0);
    ok(SUCCEEDED(hr), "Failed to process vertices, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer_Lock(dst_vb, DDLOCK_READONLY, (void **)&dst_data, NULL);
    ok(SUCCEEDED(hr), "Failed to lock destination vertex buffer, hr %#x.\n", hr);
    ok(compare_vec4(&dst_data[0], -6.500e+1f, +1.800e+2f, +2.000e-1f, +1.000e+0f, 4096),
            "Got unexpected vertex 0 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[0].x, dst_data[0].y, dst_data[0].z, dst_data[0].w);
    ok(compare_vec4(&dst_data[1], -4.000e+1f, +1.400e+2f, +4.000e-1f, +1.000e+0f, 4096),
            "Got unexpected vertex 1 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[1].x, dst_data[1].y, dst_data[1].z, dst_data[1].w);
    ok(compare_vec4(&dst_data[2], -1.500e+1f, +1.000e+2f, +6.000e-1f, +1.000e+0f, 4096),
            "Got unexpected vertex 2 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[2].x, dst_data[2].y, dst_data[2].z, dst_data[2].w);
    hr = IDirect3DVertexBuffer_Unlock(dst_vb);
    ok(SUCCEEDED(hr), "Failed to unlock destination vertex buffer, hr %#x.\n", hr);

    hr = IDirect3DDevice3_MultiplyTransform(device, D3DTRANSFORMSTATE_PROJECTION, &projection);
    ok(SUCCEEDED(hr), "Failed to set projection transformation, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer_ProcessVertices(dst_vb, D3DVOP_TRANSFORM, 0, 3, src_vb, 0, device, 0);
    ok(SUCCEEDED(hr), "Failed to process vertices, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer_Lock(dst_vb, DDLOCK_READONLY, (void **)&dst_data, NULL);
    ok(SUCCEEDED(hr), "Failed to lock destination vertex buffer, hr %#x.\n", hr);
    ok(compare_vec4(&dst_data[0], +8.500e+1f, -1.000e+2f, +1.800e+0f, +1.000e+0f, 4096),
            "Got unexpected vertex 0 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[0].x, dst_data[0].y, dst_data[0].z, dst_data[0].w);
    ok(compare_vec4(&dst_data[1], +1.100e+2f, -1.400e+2f, +2.000e+0f, +1.000e+0f, 4096),
            "Got unexpected vertex 1 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[1].x, dst_data[1].y, dst_data[1].z, dst_data[1].w);
    ok(compare_vec4(&dst_data[2], +1.350e+2f, -1.800e+2f, +2.200e+0f, +1.000e+0f, 4096),
            "Got unexpected vertex 2 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[2].x, dst_data[2].y, dst_data[2].z, dst_data[2].w);
    hr = IDirect3DVertexBuffer_Unlock(dst_vb);
    ok(SUCCEEDED(hr), "Failed to unlock destination vertex buffer, hr %#x.\n", hr);

    vp2.dwSize = sizeof(vp2);
    vp2.dwX = 30;
    vp2.dwY = 40;
    vp2.dwWidth = 90;
    vp2.dwHeight = 80;
    vp2.dvClipX = 4.0f;
    vp2.dvClipY = 6.0f;
    vp2.dvClipWidth = 2.0f;
    vp2.dvClipHeight = 4.0f;
    vp2.dvMinZ = 3.0f;
    vp2.dvMaxZ = -2.0f;
    hr = IDirect3DViewport3_SetViewport2(viewport, &vp2);
    ok(SUCCEEDED(hr), "Failed to set viewport data, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer_ProcessVertices(dst_vb, D3DVOP_TRANSFORM, 0, 3, src_vb, 0, device, 0);
    ok(SUCCEEDED(hr), "Failed to process vertices, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer_Lock(dst_vb, DDLOCK_READONLY, (void **)&dst_data, NULL);
    ok(SUCCEEDED(hr), "Failed to lock destination vertex buffer, hr %#x.\n", hr);
    ok(compare_vec4(&dst_data[0], +7.500e+1f, +4.000e+1f, -8.000e-1f, +1.000e+0f, 4096),
            "Got unexpected vertex 0 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[0].x, dst_data[0].y, dst_data[0].z, dst_data[0].w);
    ok(compare_vec4(&dst_data[1], +1.200e+2f, +2.000e+1f, -1.000e+0f, +1.000e+0f, 4096),
            "Got unexpected vertex 1 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[1].x, dst_data[1].y, dst_data[1].z, dst_data[1].w);
    ok(compare_vec4(&dst_data[2], +1.650e+2f, +0.000e+0f, -1.200e+0f, +1.000e+0f, 4096),
            "Got unexpected vertex 2 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[2].x, dst_data[2].y, dst_data[2].z, dst_data[2].w);
    hr = IDirect3DVertexBuffer_Unlock(dst_vb);
    ok(SUCCEEDED(hr), "Failed to unlock destination vertex buffer, hr %#x.\n", hr);

    vp1.dwSize = sizeof(vp1);
    vp1.dwX = 30;
    vp1.dwY = 40;
    vp1.dwWidth = 90;
    vp1.dwHeight = 80;
    vp1.dvScaleX = 7.0f;
    vp1.dvScaleY = 2.0f;
    vp1.dvMaxX = 6.0f;
    vp1.dvMaxY = 10.0f;
    vp1.dvMinZ = -2.0f;
    vp1.dvMaxZ = 3.0f;
    hr = IDirect3DViewport3_SetViewport(viewport, &vp1);
    ok(SUCCEEDED(hr), "Failed to set viewport data, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer_ProcessVertices(dst_vb, D3DVOP_TRANSFORM, 0, 3, src_vb, 0, device, 0);
    ok(SUCCEEDED(hr), "Failed to process vertices, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer_Lock(dst_vb, DDLOCK_READONLY, (void **)&dst_data, NULL);
    ok(SUCCEEDED(hr), "Failed to lock destination vertex buffer, hr %#x.\n", hr);
    ok(compare_vec4(&dst_data[0], +1.100e+2f, +6.800e+1f, +7.000e+0f, +1.000e+0f, 4096),
            "Got unexpected vertex 0 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[0].x, dst_data[0].y, dst_data[0].z, dst_data[0].w);
    ok(compare_vec4(&dst_data[1], +1.170e+2f, +6.600e+1f, +8.000e+0f, +1.000e+0f, 4096),
            "Got unexpected vertex 1 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[1].x, dst_data[1].y, dst_data[1].z, dst_data[1].w);
    ok(compare_vec4(&dst_data[2], +1.240e+2f, +6.400e+1f, +9.000e+0f, +1.000e+0f, 4096),
            "Got unexpected vertex 2 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[2].x, dst_data[2].y, dst_data[2].z, dst_data[2].w);
    hr = IDirect3DVertexBuffer_Unlock(dst_vb);
    ok(SUCCEEDED(hr), "Failed to unlock destination vertex buffer, hr %#x.\n", hr);

    hr = IDirect3DDevice3_DeleteViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to delete viewport, hr %#x.\n", hr);

    IDirect3DVertexBuffer_Release(dst_vb);
    IDirect3DVertexBuffer_Release(src_vb);
    IDirect3DViewport3_Release(viewport);
    IDirect3D3_Release(d3d3);
    IDirect3DDevice3_Release(device);
    DestroyWindow(window);
}

static void test_coop_level_create_device_window(void)
{
    HWND focus_window, device_window;
    IDirectDraw4 *ddraw;
    HRESULT hr;

    focus_window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(ddraw = create_ddraw()))
    {
        skip("Failed to create a ddraw object, skipping test.\n");
        DestroyWindow(focus_window);
        return;
    }

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW | DDSCL_NORMAL);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW | DDSCL_NORMAL | DDSCL_FULLSCREEN);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    todo_wine ok(hr == DDERR_NOFOCUSWINDOW || broken(hr == DDERR_INVALIDPARAMS), "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");

    /* Windows versions before 98 / NT5 don't support DDSCL_CREATEDEVICEWINDOW. */
    if (broken(hr == DDERR_INVALIDPARAMS))
    {
        win_skip("DDSCL_CREATEDEVICEWINDOW not supported, skipping test.\n");
        IDirectDraw4_Release(ddraw);
        DestroyWindow(focus_window);
        return;
    }

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, focus_window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_SETFOCUSWINDOW
            | DDSCL_CREATEDEVICEWINDOW | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    todo_wine ok(hr == DDERR_NOHWND, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    todo_wine ok(!!device_window, "Device window not found.\n");

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, focus_window, DDSCL_SETFOCUSWINDOW
            | DDSCL_CREATEDEVICEWINDOW | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    todo_wine ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    todo_wine ok(!!device_window, "Device window not found.\n");

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    todo_wine ok(hr == DDERR_NOFOCUSWINDOW, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, focus_window, DDSCL_SETFOCUSWINDOW);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    todo_wine ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    todo_wine ok(!!device_window, "Device window not found.\n");

    IDirectDraw4_Release(ddraw);
    DestroyWindow(focus_window);
}

START_TEST(ddraw4)
{
    test_process_vertices();
    test_coop_level_create_device_window();
}
