/*
 * Copyright 2006 Stefan Dösinger for CodeWeavers
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

static HRESULT (WINAPI *pDirectDrawCreateEx)(GUID *guid, void **ddraw, REFIID iid, IUnknown *outer_unknown);

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

static BOOL compare_vec3(struct vec3 *vec, float x, float y, float z, unsigned int ulps)
{
    return compare_float(vec->x, x, ulps)
            && compare_float(vec->y, y, ulps)
            && compare_float(vec->z, z, ulps);
}

static BOOL compare_vec4(struct vec4 *vec, float x, float y, float z, float w, unsigned int ulps)
{
    return compare_float(vec->x, x, ulps)
            && compare_float(vec->y, y, ulps)
            && compare_float(vec->z, z, ulps)
            && compare_float(vec->w, w, ulps);
}

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

static D3DCOLOR get_surface_color(IDirectDrawSurface7 *surface, UINT x, UINT y)
{
    RECT rect = {x, y, x + 1, y + 1};
    DDSURFACEDESC2 surface_desc;
    D3DCOLOR color;
    HRESULT hr;

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);

    hr = IDirectDrawSurface7_Lock(surface, &rect, &surface_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    if (FAILED(hr))
        return 0xdeadbeef;

    color = *((DWORD *)surface_desc.lpSurface) & 0x00ffffff;

    hr = IDirectDrawSurface7_Unlock(surface, &rect);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    return color;
}

static IDirectDraw7 *create_ddraw(void)
{
    IDirectDraw7 *ddraw;

    if (FAILED(pDirectDrawCreateEx(NULL, (void **)&ddraw, &IID_IDirectDraw7, NULL)))
        return NULL;

    return ddraw;
}

static IDirect3DDevice7 *create_device(HWND window, DWORD coop_level)
{
    IDirect3DDevice7 *device = NULL;
    IDirectDrawSurface7 *surface;
    DDSURFACEDESC2 surface_desc;
    IDirectDraw7 *ddraw;
    IDirect3D7 *d3d7;
    HRESULT hr;

    if (!(ddraw = create_ddraw()))
        return NULL;

    hr = IDirectDraw7_SetCooperativeLevel(ddraw, window, coop_level);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    surface_desc.dwWidth = 640;
    surface_desc.dwHeight = 480;

    hr = IDirectDraw7_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    if (coop_level & DDSCL_NORMAL)
    {
        IDirectDrawClipper *clipper;

        hr = IDirectDraw7_CreateClipper(ddraw, 0, &clipper, NULL);
        ok(SUCCEEDED(hr), "Failed to create clipper, hr %#x.\n", hr);
        hr = IDirectDrawClipper_SetHWnd(clipper, 0, window);
        ok(SUCCEEDED(hr), "Failed to set clipper window, hr %#x.\n", hr);
        hr = IDirectDrawSurface7_SetClipper(surface, clipper);
        ok(SUCCEEDED(hr), "Failed to set surface clipper, hr %#x.\n", hr);
        IDirectDrawClipper_Release(clipper);
    }

    hr = IDirectDraw7_QueryInterface(ddraw, &IID_IDirect3D7, (void **)&d3d7);
    IDirectDraw7_Release(ddraw);
    if (FAILED(hr))
    {
        IDirectDrawSurface7_Release(surface);
        return NULL;
    }

    hr = IDirect3D7_CreateDevice(d3d7, &IID_IDirect3DTnLHalDevice, surface, &device);
    IDirect3D7_Release(d3d7);
    IDirectDrawSurface7_Release(surface);
    if (FAILED(hr))
        return NULL;

    return device;
}

static void test_process_vertices(void)
{
    IDirect3DVertexBuffer7 *src_vb, *dst_vb1, *dst_vb2;
    D3DVERTEXBUFFERDESC vb_desc;
    IDirect3DDevice7 *device;
    struct vec4 *dst_data;
    struct vec3 *dst_data2;
    struct vec3 *src_data;
    IDirect3D7 *d3d7;
    D3DVIEWPORT7 vp;
    HWND window;
    HRESULT hr;

    static D3DMATRIX world =
    {
        0.0f,  1.0f, 0.0f, 0.0f,
        1.0f,  0.0f, 0.0f, 0.0f,
        0.0f,  0.0f, 0.0f, 1.0f,
        0.0f,  1.0f, 1.0f, 1.0f,
    };
    static D3DMATRIX view =
    {
        2.0f,  0.0f, 0.0f, 0.0f,
        0.0f, -1.0f, 0.0f, 0.0f,
        0.0f,  0.0f, 1.0f, 0.0f,
        0.0f,  0.0f, 0.0f, 3.0f,
    };
    static D3DMATRIX proj =
    {
        1.0f,  0.0f, 0.0f, 1.0f,
        0.0f,  1.0f, 1.0f, 0.0f,
        0.0f,  1.0f, 1.0f, 0.0f,
        1.0f,  0.0f, 0.0f, 1.0f,
    };

    window = CreateWindowA("static", "d3d7_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a ddraw object, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice7_GetDirect3D(device, &d3d7);
    ok(SUCCEEDED(hr), "Failed to get Direct3D7 interface, hr %#x.\n", hr);

    memset(&vb_desc, 0, sizeof(vb_desc));
    vb_desc.dwSize = sizeof(vb_desc);
    vb_desc.dwFVF = D3DFVF_XYZ;
    vb_desc.dwNumVertices = 4;
    hr = IDirect3D7_CreateVertexBuffer(d3d7, &vb_desc, &src_vb, 0);
    ok(SUCCEEDED(hr), "Failed to create source vertex buffer, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer7_Lock(src_vb, 0, (void **)&src_data, NULL);
    ok(SUCCEEDED(hr), "Failed to lock source vertex buffer, hr %#x.\n", hr);
    src_data[0].x = 0.0f;
    src_data[0].y = 0.0f;
    src_data[0].z = 0.0f;
    src_data[1].x = 1.0f;
    src_data[1].y = 1.0f;
    src_data[1].z = 1.0f;
    src_data[2].x = -1.0f;
    src_data[2].y = -1.0f;
    src_data[2].z = 0.5f;
    src_data[3].x = 0.5f;
    src_data[3].y = -0.5f;
    src_data[3].z = 0.25f;
    hr = IDirect3DVertexBuffer7_Unlock(src_vb);
    ok(SUCCEEDED(hr), "Failed to unlock source vertex buffer, hr %#x.\n", hr);

    memset(&vb_desc, 0, sizeof(vb_desc));
    vb_desc.dwSize = sizeof(vb_desc);
    vb_desc.dwFVF = D3DFVF_XYZRHW;
    vb_desc.dwNumVertices = 4;
    /* MSDN says that the last parameter must be 0 - check that. */
    hr = IDirect3D7_CreateVertexBuffer(d3d7, &vb_desc, &dst_vb1, 4);
    ok(SUCCEEDED(hr), "Failed to create vertex buffer, hr %#x.\n", hr);

    memset(&vb_desc, 0, sizeof(vb_desc));
    vb_desc.dwSize = sizeof(vb_desc);
    vb_desc.dwFVF = D3DFVF_XYZ;
    vb_desc.dwNumVertices = 5;
    /* MSDN says that the last parameter must be 0 - check that. */
    hr = IDirect3D7_CreateVertexBuffer(d3d7, &vb_desc, &dst_vb2, 12345678);
    ok(SUCCEEDED(hr), "Failed to create vertex buffer, hr %#x.\n", hr);

    memset(&vp, 0, sizeof(vp));
    vp.dwX = 64;
    vp.dwY = 64;
    vp.dwWidth = 128;
    vp.dwHeight = 128;
    vp.dvMinZ = 0.0f;
    vp.dvMaxZ = 1.0f;
    hr = IDirect3DDevice7_SetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer7_ProcessVertices(dst_vb1, D3DVOP_TRANSFORM, 0, 4, src_vb, 0, device, 0);
    ok(SUCCEEDED(hr), "Failed to process vertices, hr %#x.\n", hr);
    hr = IDirect3DVertexBuffer7_ProcessVertices(dst_vb2, D3DVOP_TRANSFORM, 0, 4, src_vb, 0, device, 0);
    ok(SUCCEEDED(hr), "Failed to process vertices, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer7_Lock(dst_vb1, 0, (void **)&dst_data, NULL);
    ok(SUCCEEDED(hr), "Failed to lock destination vertex buffer, hr %#x.\n", hr);
    ok(compare_vec4(&dst_data[0], +1.280e+2f, +1.280e+2f, +0.000e+0f, +1.000e+0f, 4096),
            "Got unexpected vertex 0 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[0].x, dst_data[0].y, dst_data[0].z, dst_data[0].w);
    ok(compare_vec4(&dst_data[1], +1.920e+2f, +6.400e+1f, +1.000e+0f, +1.000e+0f, 4096),
            "Got unexpected vertex 1 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[1].x, dst_data[1].y, dst_data[1].z, dst_data[1].w);
    ok(compare_vec4(&dst_data[2], +6.400e+1f, +1.920e+2f, +5.000e-1f, +1.000e+0f, 4096),
            "Got unexpected vertex 2 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[2].x, dst_data[2].y, dst_data[2].z, dst_data[2].w);
    ok(compare_vec4(&dst_data[3], +1.600e+2f, +1.600e+2f, +2.500e-1f, +1.000e+0f, 4096),
            "Got unexpected vertex 3 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[3].x, dst_data[3].y, dst_data[3].z, dst_data[3].w);
    hr = IDirect3DVertexBuffer7_Unlock(dst_vb1);
    ok(SUCCEEDED(hr), "Failed to unlock destination vertex buffer, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer7_Lock(dst_vb2, 0, (void **)&dst_data2, NULL);
    ok(SUCCEEDED(hr), "Failed to lock destination vertex buffer, hr %#x.\n", hr);
    /* Small thing without much practical meaning, but I stumbled upon it,
     * so let's check for it: If the output vertex buffer has no RHW value,
     * the RHW value of the last vertex is written into the next vertex. */
    ok(compare_vec3(&dst_data2[4], +1.000e+0f, +0.000e+0f, +0.000e+0f, 4096),
            "Got unexpected vertex 4 {%.8e, %.8e, %.8e}.\n",
            dst_data2[4].x, dst_data2[4].y, dst_data2[4].z);
    hr = IDirect3DVertexBuffer7_Unlock(dst_vb2);
    ok(SUCCEEDED(hr), "Failed to unlock destination vertex buffer, hr %#x.\n", hr);

    /* Try a more complicated viewport, same vertices. */
    memset(&vp, 0, sizeof(vp));
    vp.dwX = 10;
    vp.dwY = 5;
    vp.dwWidth = 246;
    vp.dwHeight = 130;
    vp.dvMinZ = -2.0f;
    vp.dvMaxZ = 4.0f;
    hr = IDirect3DDevice7_SetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer7_ProcessVertices(dst_vb1, D3DVOP_TRANSFORM, 0, 4, src_vb, 0, device, 0);
    ok(SUCCEEDED(hr), "Failed to process vertices, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer7_Lock(dst_vb1, 0, (void **)&dst_data, NULL);
    ok(SUCCEEDED(hr), "Failed to lock destination vertex buffer, hr %#x.\n", hr);
    ok(compare_vec4(&dst_data[0], +1.330e+2f, +7.000e+1f, -2.000e+0f, +1.000e+0f, 4096),
            "Got unexpected vertex 0 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[0].x, dst_data[0].y, dst_data[0].z, dst_data[0].w);
    ok(compare_vec4(&dst_data[1], +2.560e+2f, +5.000e+0f, +4.000e+0f, +1.000e+0f, 4096),
            "Got unexpected vertex 1 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[1].x, dst_data[1].y, dst_data[1].z, dst_data[1].w);
    ok(compare_vec4(&dst_data[2], +1.000e+1f, +1.350e+2f, +1.000e+0f, +1.000e+0f, 4096),
            "Got unexpected vertex 2 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[2].x, dst_data[2].y, dst_data[2].z, dst_data[2].w);
    ok(compare_vec4(&dst_data[3], +1.945e+2f, +1.025e+2f, -5.000e-1f, +1.000e+0f, 4096),
            "Got unexpected vertex 3 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[3].x, dst_data[3].y, dst_data[3].z, dst_data[3].w);
    hr = IDirect3DVertexBuffer7_Unlock(dst_vb1);
    ok(SUCCEEDED(hr), "Failed to unlock destination vertex buffer, hr %#x.\n", hr);

    hr = IDirect3DDevice7_SetTransform(device, D3DTRANSFORMSTATE_WORLD, &world);
    ok(SUCCEEDED(hr), "Failed to set world transform, hr %#x.\n", hr);
    hr = IDirect3DDevice7_SetTransform(device, D3DTRANSFORMSTATE_VIEW, &view);
    ok(SUCCEEDED(hr), "Failed to set view transform, hr %#x.\n", hr);
    hr = IDirect3DDevice7_SetTransform(device, D3DTRANSFORMSTATE_PROJECTION, &proj);
    ok(SUCCEEDED(hr), "Failed to set projection transform, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer7_ProcessVertices(dst_vb1, D3DVOP_TRANSFORM, 0, 4, src_vb, 0, device, 0);
    ok(SUCCEEDED(hr), "Failed to process vertices, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer7_Lock(dst_vb1, 0, (void **)&dst_data, NULL);
    ok(SUCCEEDED(hr), "Failed to lock destination vertex buffer, hr %#x.\n", hr);
    ok(compare_vec4(&dst_data[0], +2.560e+2f, +7.000e+1f, -2.000e+0f, +3.333e-1f, 4096),
            "Got unexpected vertex 0 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[0].x, dst_data[0].y, dst_data[0].z, dst_data[0].w);
    ok(compare_vec4(&dst_data[1], +2.560e+2f, +7.813e+1f, -2.750e+0f, +1.250e-1f, 4096),
            "Got unexpected vertex 1 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[1].x, dst_data[1].y, dst_data[1].z, dst_data[1].w);
    ok(compare_vec4(&dst_data[2], +2.560e+2f, +4.400e+1f, +4.000e-1f, +4.000e-1f, 4096),
            "Got unexpected vertex 2 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[2].x, dst_data[2].y, dst_data[2].z, dst_data[2].w);
    ok(compare_vec4(&dst_data[3], +2.560e+2f, +8.182e+1f, -3.091e+0f, +3.636e-1f, 4096),
            "Got unexpected vertex 3 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[3].x, dst_data[3].y, dst_data[3].z, dst_data[3].w);
    hr = IDirect3DVertexBuffer7_Unlock(dst_vb1);
    ok(SUCCEEDED(hr), "Failed to unlock destination vertex buffer, hr %#x.\n", hr);

    IDirect3DVertexBuffer7_Release(dst_vb2);
    IDirect3DVertexBuffer7_Release(dst_vb1);
    IDirect3DVertexBuffer7_Release(src_vb);
    IDirect3D7_Release(d3d7);
    IDirect3DDevice7_Release(device);
    DestroyWindow(window);
}

static void test_coop_level_create_device_window(void)
{
    HWND focus_window, device_window;
    IDirectDraw7 *ddraw;
    HRESULT hr;

    focus_window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(ddraw = create_ddraw()))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(focus_window);
        return;
    }

    hr = IDirectDraw7_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw7_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw7_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW | DDSCL_NORMAL);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw7_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW | DDSCL_NORMAL | DDSCL_FULLSCREEN);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw7_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DDERR_NOFOCUSWINDOW || broken(hr == DDERR_INVALIDPARAMS), "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");

    /* Windows versions before 98 / NT5 don't support DDSCL_CREATEDEVICEWINDOW. */
    if (broken(hr == DDERR_INVALIDPARAMS))
    {
        win_skip("DDSCL_CREATEDEVICEWINDOW not supported, skipping test.\n");
        IDirectDraw7_Release(ddraw);
        DestroyWindow(focus_window);
        return;
    }

    hr = IDirectDraw7_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw7_SetCooperativeLevel(ddraw, focus_window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");

    hr = IDirectDraw7_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw7_SetCooperativeLevel(ddraw, NULL, DDSCL_SETFOCUSWINDOW
            | DDSCL_CREATEDEVICEWINDOW | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DDERR_NOHWND, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!!device_window, "Device window not found.\n");

    hr = IDirectDraw7_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw7_SetCooperativeLevel(ddraw, focus_window, DDSCL_SETFOCUSWINDOW
            | DDSCL_CREATEDEVICEWINDOW | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!!device_window, "Device window not found.\n");

    hr = IDirectDraw7_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw7_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DDERR_NOFOCUSWINDOW, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw7_SetCooperativeLevel(ddraw, focus_window, DDSCL_SETFOCUSWINDOW);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw7_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!!device_window, "Device window not found.\n");

    IDirectDraw7_Release(ddraw);
    DestroyWindow(focus_window);
}

static void test_clipper_blt(void)
{
    IDirectDrawSurface7 *src_surface, *dst_surface;
    RECT client_rect, src_rect, *rect;
    IDirectDrawClipper *clipper;
    DDSURFACEDESC2 surface_desc;
    unsigned int i, j, x, y;
    IDirectDraw7 *ddraw;
    RGNDATA *rgn_data;
    D3DCOLOR color;
    HRGN r1, r2;
    HWND window;
    DDBLTFX fx;
    HRESULT hr;
    DWORD ret;

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

    hr = IDirectDraw7_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    hr = IDirectDraw7_CreateClipper(ddraw, 0, &clipper, NULL);
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
    ok(rgn_data->rdh.nRgnSize == 16, "Got unexpected region size %u.\n", rgn_data->rdh.nRgnSize);
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
    U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 32;
    U2(U4(surface_desc).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(U4(surface_desc).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(U4(surface_desc).ddpfPixelFormat).dwBBitMask = 0x000000ff;

    hr = IDirectDraw7_CreateSurface(ddraw, &surface_desc, &src_surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create source surface, hr %#x.\n", hr);
    hr = IDirectDraw7_CreateSurface(ddraw, &surface_desc, &dst_surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create destination surface, hr %#x.\n", hr);

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    hr = IDirectDrawSurface7_Blt(src_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear source surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface7_Blt(dst_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear destination surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface7_Lock(src_surface, NULL, &surface_desc, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to lock source surface, hr %#x.\n", hr);
    ((DWORD *)surface_desc.lpSurface)[0] = 0xff0000ff;
    ((DWORD *)surface_desc.lpSurface)[1] = 0xff00ff00;
    ((DWORD *)surface_desc.lpSurface)[2] = 0xffff0000;
    ((DWORD *)surface_desc.lpSurface)[3] = 0xffffffff;
    hr = IDirectDrawSurface7_Unlock(src_surface, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock source surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface7_SetClipper(dst_surface, clipper);
    ok(SUCCEEDED(hr), "Failed to set clipper, hr %#x.\n", hr);

    SetRect(&src_rect, 0, 0, 4, 1);
    hr = IDirectDrawSurface7_Blt(dst_surface, NULL, src_surface, &src_rect, DDBLT_WAIT, NULL);
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
    hr = IDirectDrawSurface7_Blt(dst_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
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

    hr = IDirectDrawSurface7_BltFast(dst_surface, 0, 0, src_surface, NULL, DDBLTFAST_WAIT);
    ok(hr == DDERR_BLTFASTCANTCLIP, "Got unexpected hr %#x.\n", hr);

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

    IDirectDrawSurface7_Release(dst_surface);
    IDirectDrawSurface7_Release(src_surface);
    IDirectDrawClipper_Release(clipper);
    IDirectDraw7_Release(ddraw);
}

START_TEST(ddraw7)
{
    HMODULE module = GetModuleHandleA("ddraw.dll");

    if (!(pDirectDrawCreateEx = (void *)GetProcAddress(module, "DirectDrawCreateEx")))
    {
        win_skip("DirectDrawCreateEx not available, skipping tests.\n");
        return;
    }

    test_process_vertices();
    test_coop_level_create_device_window();
    test_clipper_blt();
}
