/*
 * Copyright 2011-2012 Henri Verbeet for CodeWeavers
 * Copyright 2012-2014 Stefan Dösinger for CodeWeavers
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
#include <limits.h>
#include "d3d.h"

struct vec2
{
    float x, y;
};

struct vec3
{
    float x, y, z;
};

struct vec4
{
    float x, y, z, w;
};

struct create_window_thread_param
{
    HWND window;
    HANDLE window_created;
    HANDLE destroy_window;
    HANDLE thread;
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

        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
            DispatchMessageA(&msg);
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

    p->window_created = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(!!p->window_created, "CreateEvent failed, last error %#x.\n", GetLastError());
    p->destroy_window = CreateEventA(NULL, FALSE, FALSE, NULL);
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

static IDirectDrawSurface4 *get_depth_stencil(IDirect3DDevice3 *device)
{
    IDirectDrawSurface4 *rt, *ret;
    DDSCAPS2 caps = {DDSCAPS_ZBUFFER, 0, 0, 0};
    HRESULT hr;

    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get the render target, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetAttachedSurface(rt, &caps, &ret);
    ok(SUCCEEDED(hr) || hr == DDERR_NOTFOUND, "Failed to get the z buffer, hr %#x.\n", hr);
    IDirectDrawSurface4_Release(rt);
    return ret;
}

static HRESULT set_display_mode(IDirectDraw4 *ddraw, DWORD width, DWORD height)
{
    if (SUCCEEDED(IDirectDraw4_SetDisplayMode(ddraw, width, height, 32, 0, 0)))
        return DD_OK;
    return IDirectDraw4_SetDisplayMode(ddraw, width, height, 24, 0, 0);
}

static D3DCOLOR get_surface_color(IDirectDrawSurface4 *surface, UINT x, UINT y)
{
    RECT rect = {x, y, x + 1, y + 1};
    DDSURFACEDESC2 surface_desc;
    D3DCOLOR color;
    HRESULT hr;

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);

    hr = IDirectDrawSurface4_Lock(surface, &rect, &surface_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    if (FAILED(hr))
        return 0xdeadbeef;

    color = *((DWORD *)surface_desc.lpSurface) & 0x00ffffff;

    hr = IDirectDrawSurface4_Unlock(surface, &rect);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    return color;
}

static HRESULT CALLBACK enum_z_fmt(DDPIXELFORMAT *format, void *ctx)
{
    DDPIXELFORMAT *z_fmt = ctx;

    if (U1(*format).dwZBufferBitDepth > U1(*z_fmt).dwZBufferBitDepth)
        *z_fmt = *format;

    return DDENUMRET_OK;
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
    IDirectDrawSurface4 *surface, *ds;
    IDirect3DDevice3 *device = NULL;
    DDSURFACEDESC2 surface_desc;
    IDirectDraw4 *ddraw4;
    DDPIXELFORMAT z_fmt;
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

    memset(&z_fmt, 0, sizeof(z_fmt));
    hr = IDirect3D3_EnumZBufferFormats(d3d3, &IID_IDirect3DHALDevice, enum_z_fmt, &z_fmt);
    if (FAILED(hr) || !z_fmt.dwSize)
    {
        IDirect3D3_Release(d3d3);
        IDirectDrawSurface4_Release(surface);
        return NULL;
    }

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    U4(surface_desc).ddpfPixelFormat = z_fmt;
    surface_desc.dwWidth = 640;
    surface_desc.dwHeight = 480;
    hr = IDirectDraw4_CreateSurface(ddraw4, &surface_desc, &ds, NULL);
    ok(SUCCEEDED(hr), "Failed to create depth buffer, hr %#x.\n", hr);
    if (FAILED(hr))
    {
        IDirect3D3_Release(d3d3);
        IDirectDrawSurface4_Release(surface);
        return NULL;
    }

    hr = IDirectDrawSurface_AddAttachedSurface(surface, ds);
    ok(SUCCEEDED(hr), "Failed to attach depth buffer, hr %#x.\n", hr);
    IDirectDrawSurface4_Release(ds);
    if (FAILED(hr))
    {
        IDirect3D3_Release(d3d3);
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

static IDirect3DViewport3 *create_viewport(IDirect3DDevice3 *device, UINT x, UINT y, UINT w, UINT h)
{
    IDirect3DViewport3 *viewport;
    D3DVIEWPORT2 vp;
    IDirect3D3 *d3d;
    HRESULT hr;

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);
    hr = IDirect3D3_CreateViewport(d3d, &viewport, NULL);
    ok(SUCCEEDED(hr), "Failed to create viewport, hr %#x.\n", hr);
    hr = IDirect3DDevice3_AddViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to add viewport, hr %#x.\n", hr);
    memset(&vp, 0, sizeof(vp));
    vp.dwSize = sizeof(vp);
    vp.dwX = x;
    vp.dwY = y;
    vp.dwWidth = w;
    vp.dwHeight = h;
    vp.dvClipX = -1.0f;
    vp.dvClipY =  1.0f;
    vp.dvClipWidth = 2.0f;
    vp.dvClipHeight = 2.0f;
    vp.dvMinZ = 0.0f;
    vp.dvMaxZ = 1.0f;
    hr = IDirect3DViewport3_SetViewport2(viewport, &vp);
    ok(SUCCEEDED(hr), "Failed to set viewport data, hr %#x.\n", hr);
    IDirect3D3_Release(d3d);

    return viewport;
}

static void destroy_viewport(IDirect3DDevice3 *device, IDirect3DViewport3 *viewport)
{
    HRESULT hr;

    hr = IDirect3DDevice3_DeleteViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to delete viewport, hr %#x.\n", hr);
    IDirect3DViewport3_Release(viewport);
}

static IDirect3DMaterial3 *create_material(IDirect3DDevice3 *device, D3DMATERIAL *mat)
{
    IDirect3DMaterial3 *material;
    IDirect3D3 *d3d;
    HRESULT hr;

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);
    hr = IDirect3D3_CreateMaterial(d3d, &material, NULL);
    ok(SUCCEEDED(hr), "Failed to create material, hr %#x.\n", hr);
    hr = IDirect3DMaterial3_SetMaterial(material, mat);
    ok(SUCCEEDED(hr), "Failed to set material data, hr %#x.\n", hr);
    IDirect3D3_Release(d3d);

    return material;
}

static IDirect3DMaterial3 *create_diffuse_material(IDirect3DDevice3 *device, float r, float g, float b, float a)
{
    D3DMATERIAL mat;

    memset(&mat, 0, sizeof(mat));
    mat.dwSize = sizeof(mat);
    U1(U(mat).diffuse).r = r;
    U2(U(mat).diffuse).g = g;
    U3(U(mat).diffuse).b = b;
    U4(U(mat).diffuse).a = a;

    return create_material(device, &mat);
}

static IDirect3DMaterial3 *create_emissive_material(IDirect3DDevice3 *device, float r, float g, float b, float a)
{
    D3DMATERIAL mat;

    memset(&mat, 0, sizeof(mat));
    mat.dwSize = sizeof(mat);
    U1(U3(mat).emissive).r = r;
    U2(U3(mat).emissive).g = g;
    U3(U3(mat).emissive).b = b;
    U4(U3(mat).emissive).a = a;

    return create_material(device, &mat);
}

static void destroy_material(IDirect3DMaterial3 *material)
{
    IDirect3DMaterial3_Release(material);
}

static const UINT *expect_messages;

static LRESULT CALLBACK test_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (expect_messages && message == *expect_messages)
        ++expect_messages;

    return DefWindowProcA(hwnd, message, wparam, lparam);
}

/* Set the wndproc back to what ddraw expects it to be, and release the ddraw
 * interface. This prevents subsequent SetCooperativeLevel() calls on a
 * different window from failing with DDERR_HWNDALREADYSET. */
static void fix_wndproc(HWND window, LONG_PTR proc)
{
    IDirectDraw4 *ddraw;
    HRESULT hr;

    if (!(ddraw = create_ddraw()))
        return;

    SetWindowLongPtrA(window, GWLP_WNDPROC, proc);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    IDirectDraw4_Release(ddraw);
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
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

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
    ok(hr == DDERR_NOFOCUSWINDOW || broken(hr == DDERR_INVALIDPARAMS), "Got unexpected hr %#x.\n", hr);
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
    ok(hr == DDERR_NOHWND, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!!device_window, "Device window not found.\n");

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, focus_window, DDSCL_SETFOCUSWINDOW
            | DDSCL_CREATEDEVICEWINDOW | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!!device_window, "Device window not found.\n");

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DDERR_NOFOCUSWINDOW, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, focus_window, DDSCL_SETFOCUSWINDOW);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!!device_window, "Device window not found.\n");

    IDirectDraw4_Release(ddraw);
    DestroyWindow(focus_window);
}

static void test_clipper_blt(void)
{
    IDirectDrawSurface4 *src_surface, *dst_surface;
    RECT client_rect, src_rect;
    IDirectDrawClipper *clipper;
    DDSURFACEDESC2 surface_desc;
    unsigned int i, j, x, y;
    IDirectDraw4 *ddraw;
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
    /* Nvidia on Windows seems to have an off-by-one error
     * when processing source rectangles. Our left = 1 and
     * right = 5 input reads from x = {1, 2, 3}. x = 4 is
     * read as well, but only for the edge pixels on the
     * output image. The bug happens on the y axis as well,
     * but we only read one row there, and all source rows
     * contain the same data. This bug is not dependent on
     * the presence of a clipper. */
    static const D3DCOLOR expected1_broken[] =
    {
        0x000000ff, 0x000000ff, 0x00000000, 0x00000000,
        0x000000ff, 0x000000ff, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00ff0000, 0x00ff0000,
        0x00000000, 0x00000000, 0x0000ff00, 0x00ff0000,
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
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    ret = GetClientRect(window, &client_rect);
    ok(ret, "Failed to get client rect.\n");
    ret = MapWindowPoints(window, NULL, (POINT *)&client_rect, 2);
    ok(ret, "Failed to map client rect.\n");

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    hr = IDirectDraw4_CreateClipper(ddraw, 0, &clipper, NULL);
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
    ok(rgn_data->rdh.nCount >= 1, "Got unexpected count %u.\n", rgn_data->rdh.nCount);
    ok(EqualRect(&rgn_data->rdh.rcBound, &client_rect),
            "Got unexpected bounding rect {%d, %d, %d, %d}, expected {%d, %d, %d, %d}.\n",
            rgn_data->rdh.rcBound.left, rgn_data->rdh.rcBound.top,
            rgn_data->rdh.rcBound.right, rgn_data->rdh.rcBound.bottom,
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

    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &src_surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create source surface, hr %#x.\n", hr);
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &dst_surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create destination surface, hr %#x.\n", hr);

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    hr = IDirectDrawSurface4_Blt(src_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear source surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(dst_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear destination surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_Lock(src_surface, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock source surface, hr %#x.\n", hr);
    ok(U1(surface_desc).lPitch == 2560, "Got unexpected surface pitch %u.\n", U1(surface_desc).lPitch);
    ptr = surface_desc.lpSurface;
    memcpy(&ptr[   0], &src_data[ 0], 6 * sizeof(DWORD));
    memcpy(&ptr[ 640], &src_data[ 6], 6 * sizeof(DWORD));
    memcpy(&ptr[1280], &src_data[12], 6 * sizeof(DWORD));
    hr = IDirectDrawSurface4_Unlock(src_surface, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock source surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_SetClipper(dst_surface, clipper);
    ok(SUCCEEDED(hr), "Failed to set clipper, hr %#x.\n", hr);

    SetRect(&src_rect, 1, 1, 5, 2);
    hr = IDirectDrawSurface4_Blt(dst_surface, NULL, src_surface, &src_rect, DDBLT_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);
    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            x = 80 * ((2 * j) + 1);
            y = 60 * ((2 * i) + 1);
            color = get_surface_color(dst_surface, x, y);
            ok(compare_color(color, expected1[i * 4 + j], 1)
                    || broken(compare_color(color, expected1_broken[i * 4 + j], 1)),
                    "Expected color 0x%08x at %u,%u, got 0x%08x.\n", expected1[i * 4 + j], x, y, color);
        }
    }

    U5(fx).dwFillColor = 0xff0000ff;
    hr = IDirectDrawSurface4_Blt(dst_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
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

    hr = IDirectDrawSurface4_BltFast(dst_surface, 0, 0, src_surface, NULL, DDBLTFAST_WAIT);
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
    hr = IDirectDrawClipper_SetClipList(clipper, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to set clip list, hr %#x.\n", hr);
    hr = IDirectDrawClipper_GetClipList(clipper, NULL, NULL, &ret);
    ok(hr == DDERR_NOCLIPLIST, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(dst_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_NOCLIPLIST, "Got unexpected hr %#x.\n", hr);

    IDirectDrawSurface4_Release(dst_surface);
    IDirectDrawSurface4_Release(src_surface);
    IDirectDrawClipper_Release(clipper);
    IDirectDraw4_Release(ddraw);
}

static void test_coop_level_d3d_state(void)
{
    D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    IDirectDrawSurface4 *rt, *surface;
    IDirect3DViewport3 *viewport;
    IDirect3DDevice3 *device;
    IDirectDraw4 *ddraw;
    IDirect3D3 *d3d;
    D3DCOLOR color;
    DWORD value;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    viewport = create_viewport(device, 0, 0, 640, 480);

    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);
    hr = IDirect3DDevice3_GetRenderState(device, D3DRENDERSTATE_ZENABLE, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
    ok(!!value, "Got unexpected z-enable state %#x.\n", value);
    hr = IDirect3DDevice3_GetRenderState(device, D3DRENDERSTATE_ALPHABLENDENABLE, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
    ok(!value, "Got unexpected alpha blend enable state %#x.\n", value);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0xffff0000, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x00ff0000, 1), "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);
    hr = IDirect3D3_QueryInterface(d3d, &IID_IDirectDraw4, (void **)&ddraw);
    ok(SUCCEEDED(hr), "Failed to get ddraw interface, hr %#x.\n", hr);
    IDirect3D3_Release(d3d);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_IsLost(rt);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDraw4_RestoreAllSurfaces(ddraw);
    ok(SUCCEEDED(hr), "Failed to restore surfaces, hr %#x.\n", hr);
    IDirectDraw4_Release(ddraw);

    hr = IDirect3DDevice3_GetRenderTarget(device, &surface);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);
    ok(surface == rt, "Got unexpected surface %p.\n", surface);
    hr = IDirect3DDevice3_GetRenderState(device, D3DRENDERSTATE_ZENABLE, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
    ok(!!value, "Got unexpected z-enable state %#x.\n", value);
    hr = IDirect3DDevice3_GetRenderState(device, D3DRENDERSTATE_ALPHABLENDENABLE, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
    ok(!!value, "Got unexpected alpha blend enable state %#x.\n", value);
    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0xff00ff00, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x0000ff00, 1), "Got unexpected color 0x%08x.\n", color);

    destroy_viewport(device, viewport);
    IDirectDrawSurface4_Release(surface);
    IDirectDrawSurface4_Release(rt);
    IDirect3DDevice3_Release(device);
    DestroyWindow(window);
}

static void test_surface_interface_mismatch(void)
{
    IDirectDraw4 *ddraw = NULL;
    IDirect3D3 *d3d = NULL;
    IDirectDrawSurface4 *surface = NULL, *ds;
    IDirectDrawSurface3 *surface3 = NULL;
    IDirect3DDevice3 *device = NULL;
    IDirect3DViewport3 *viewport = NULL;
    DDSURFACEDESC2 surface_desc;
    DDPIXELFORMAT z_fmt;
    ULONG refcount;
    HRESULT hr;
    D3DCOLOR color;
    HWND window;
    D3DRECT clear_rect = {{0}, {0}, {640}, {480}};

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    surface_desc.dwWidth = 640;
    surface_desc.dwHeight = 480;

    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_QueryInterface(surface, &IID_IDirectDrawSurface3, (void **)&surface3);
    ok(SUCCEEDED(hr), "Failed to QI IDirectDrawSurface3, hr %#x.\n", hr);

    if (FAILED(hr = IDirectDraw4_QueryInterface(ddraw, &IID_IDirect3D3, (void **)&d3d)))
    {
        skip("D3D interface is not available, skipping test.\n");
        goto cleanup;
    }

    memset(&z_fmt, 0, sizeof(z_fmt));
    hr = IDirect3D3_EnumZBufferFormats(d3d, &IID_IDirect3DHALDevice, enum_z_fmt, &z_fmt);
    if (FAILED(hr) || !z_fmt.dwSize)
    {
        skip("No depth buffer formats available, skipping test.\n");
        goto cleanup;
    }

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    U4(surface_desc).ddpfPixelFormat = z_fmt;
    surface_desc.dwWidth = 640;
    surface_desc.dwHeight = 480;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &ds, NULL);
    ok(SUCCEEDED(hr), "Failed to create depth buffer, hr %#x.\n", hr);
    if (FAILED(hr))
        goto cleanup;

    /* Using a different surface interface version still works */
    hr = IDirectDrawSurface3_AddAttachedSurface(surface3, (IDirectDrawSurface3 *)ds);
    ok(SUCCEEDED(hr), "Failed to attach depth buffer, hr %#x.\n", hr);
    refcount = IDirectDrawSurface4_Release(ds);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    if (FAILED(hr))
        goto cleanup;

    /* Here too */
    hr = IDirect3D3_CreateDevice(d3d, &IID_IDirect3DHALDevice, (IDirectDrawSurface4 *)surface3, &device, NULL);
    ok(SUCCEEDED(hr), "Failed to create d3d device.\n");
    if (FAILED(hr))
        goto cleanup;

    viewport = create_viewport(device, 0, 0, 640, 480);

    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0xffff0000, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    color = get_surface_color(surface, 320, 240);
    ok(compare_color(color, 0x00ff0000, 1), "Got unexpected color 0x%08x.\n", color);

cleanup:
    if (viewport)
        destroy_viewport(device, viewport);
    if (surface3) IDirectDrawSurface3_Release(surface3);
    if (surface) IDirectDrawSurface4_Release(surface);
    if (device) IDirect3DDevice3_Release(device);
    if (d3d) IDirect3D3_Release(d3d);
    if (ddraw) IDirectDraw4_Release(ddraw);
    DestroyWindow(window);
}

static void test_coop_level_threaded(void)
{
    struct create_window_thread_param p;
    IDirectDraw4 *ddraw;
    HRESULT hr;

    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    create_window_thread(&p);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, p.window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    IDirectDraw4_Release(ddraw);
    destroy_window_thread(&p);
}

static void test_depth_blit(void)
{
    static struct
    {
        float x, y, z;
        DWORD color;
    }
    quad1[] =
    {
        { -1.0,  1.0, 0.50f, 0xff00ff00},
        {  1.0,  1.0, 0.50f, 0xff00ff00},
        { -1.0, -1.0, 0.50f, 0xff00ff00},
        {  1.0, -1.0, 0.50f, 0xff00ff00},
    };
    static const D3DCOLOR expected_colors[4][4] =
    {
        {0x00ff0000, 0x00ff0000, 0x0000ff00, 0x0000ff00},
        {0x00ff0000, 0x00ff0000, 0x0000ff00, 0x0000ff00},
        {0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00},
        {0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00},
    };
    DDSURFACEDESC2 ddsd_new, ddsd_existing;

    IDirect3DDevice3 *device;
    IDirectDrawSurface4 *ds1, *ds2, *ds3, *rt;
    IDirect3DViewport3 *viewport;
    RECT src_rect, dst_rect;
    unsigned int i, j;
    D3DCOLOR color;
    HRESULT hr;
    IDirect3D3 *d3d;
    IDirectDraw4 *ddraw;
    DDBLTFX fx;
    HWND window;
    D3DRECT d3drect;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get Direct3D3 interface, hr %#x.\n", hr);
    hr = IDirect3D3_QueryInterface(d3d, &IID_IDirectDraw4, (void **)&ddraw);
    ok(SUCCEEDED(hr), "Failed to get DirectDraw4 interface, hr %#x.\n", hr);
    IDirect3D3_Release(d3d);

    ds1 = get_depth_stencil(device);

    memset(&ddsd_new, 0, sizeof(ddsd_new));
    ddsd_new.dwSize = sizeof(ddsd_new);
    memset(&ddsd_existing, 0, sizeof(ddsd_existing));
    ddsd_existing.dwSize = sizeof(ddsd_existing);
    hr = IDirectDrawSurface4_GetSurfaceDesc(ds1, &ddsd_existing);
    ddsd_new.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    ddsd_new.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    ddsd_new.dwWidth = ddsd_existing.dwWidth;
    ddsd_new.dwHeight = ddsd_existing.dwHeight;
    U4(ddsd_new).ddpfPixelFormat = U4(ddsd_existing).ddpfPixelFormat;
    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd_new, &ds2, NULL);
    ok(SUCCEEDED(hr), "Failed to create a surface, hr %#x.\n", hr);
    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd_new, &ds3, NULL);
    ok(SUCCEEDED(hr), "Failed to create a surface, hr %#x.\n", hr);
    IDirectDraw4_Release(ddraw);

    viewport = create_viewport(device, 0, 0, ddsd_existing.dwWidth, ddsd_existing.dwHeight);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to activate the viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_ZENABLE, D3DZB_TRUE);
    ok(SUCCEEDED(hr), "Failed to enable z testing, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL);
    ok(SUCCEEDED(hr), "Failed to set the z function, hr %#x.\n", hr);

    U1(d3drect).x1 = U2(d3drect).y1 = 0;
    U3(d3drect).x2 = ddsd_existing.dwWidth; U4(d3drect).y2 = ddsd_existing.dwHeight;
    hr = IDirect3DViewport3_Clear2(viewport, 1, &d3drect, D3DCLEAR_ZBUFFER, 0, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear the z buffer, hr %#x.\n", hr);

    /* Partial blit. */
    SetRect(&src_rect, 0, 0, 320, 240);
    SetRect(&dst_rect, 0, 0, 320, 240);
    hr = IDirectDrawSurface4_Blt(ds2, &dst_rect, ds1, &src_rect, DDBLT_WAIT, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    /* Different locations. */
    SetRect(&src_rect, 0, 0, 320, 240);
    SetRect(&dst_rect, 320, 240, 640, 480);
    hr = IDirectDrawSurface4_Blt(ds2, &dst_rect, ds1, &src_rect, DDBLT_WAIT, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    /* Streched. */
    SetRect(&src_rect, 0, 0, 320, 240);
    SetRect(&dst_rect, 0, 0, 640, 480);
    hr = IDirectDrawSurface4_Blt(ds2, &dst_rect, ds1, &src_rect, DDBLT_WAIT, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    /* Flipped. */
    SetRect(&src_rect, 0, 480, 640, 0);
    SetRect(&dst_rect, 0, 0, 640, 480);
    hr = IDirectDrawSurface4_Blt(ds2, &dst_rect, ds1, &src_rect, DDBLT_WAIT, NULL);
    ok(hr == DDERR_INVALIDRECT, "Got unexpected hr %#x.\n", hr);
    SetRect(&src_rect, 0, 0, 640, 480);
    SetRect(&dst_rect, 0, 480, 640, 0);
    hr = IDirectDrawSurface4_Blt(ds2, &dst_rect, ds1, &src_rect, DDBLT_WAIT, NULL);
    ok(hr == DDERR_INVALIDRECT, "Got unexpected hr %#x.\n", hr);
    /* Full, explicit. */
    SetRect(&src_rect, 0, 0, 640, 480);
    SetRect(&dst_rect, 0, 0, 640, 480);
    hr = IDirectDrawSurface4_Blt(ds2, &dst_rect, ds1, &src_rect, DDBLT_WAIT, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    /* Depth -> color blit: Succeeds on Win7 + Radeon HD 5700, fails on WinXP + Radeon X1600 */

    /* Depth blit inside a BeginScene / EndScene pair */
    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to start a scene, hr %#x.\n", hr);
    /* From the current depth stencil */
    hr = IDirectDrawSurface4_Blt(ds2, NULL, ds1, NULL, DDBLT_WAIT, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    /* To the current depth stencil */
    hr = IDirectDrawSurface4_Blt(ds1, NULL, ds2, NULL, DDBLT_WAIT, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    /* Between unbound surfaces */
    hr = IDirectDrawSurface4_Blt(ds3, NULL, ds2, NULL, DDBLT_WAIT, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end a scene, hr %#x.\n", hr);

    /* Avoid changing the depth stencil, it doesn't work properly on Windows.
     * Instead use DDBLT_DEPTHFILL to clear the depth stencil. Unfortunately
     * drivers disagree on the meaning of dwFillDepth. Only 0 seems to produce
     * a reliable result(z = 0.0) */
    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    hr = IDirectDrawSurface4_Blt(ds2, NULL, NULL, NULL, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear the source z buffer, hr %#x.\n", hr);

    hr = IDirect3DViewport3_Clear2(viewport, 1, &d3drect, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, 0xffff0000, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear the color and z buffers, hr %#x.\n", hr);
    SetRect(&dst_rect, 0, 0, 320, 240);
    hr = IDirectDrawSurface4_Blt(ds1, &dst_rect, ds2, NULL, DDBLT_WAIT, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    IDirectDrawSurface4_Release(ds3);
    IDirectDrawSurface4_Release(ds2);
    IDirectDrawSurface4_Release(ds1);

    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to start a scene, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_DIFFUSE,
            quad1, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end a scene, hr %#x.\n", hr);

    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);
    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            unsigned int x = 80 * ((2 * j) + 1);
            unsigned int y = 60 * ((2 * i) + 1);
            color = get_surface_color(rt, x, y);
            ok(compare_color(color, expected_colors[i][j], 1),
                    "Expected color 0x%08x at %u,%u, got 0x%08x.\n", expected_colors[i][j], x, y, color);
        }
    }
    IDirectDrawSurface4_Release(rt);

    destroy_viewport(device, viewport);
    IDirect3DDevice3_Release(device);
    DestroyWindow(window);
}

static void test_texture_load_ckey(void)
{
    IDirectDraw4 *ddraw;
    IDirectDrawSurface4 *src;
    IDirectDrawSurface4 *dst;
    IDirect3DTexture2 *src_tex;
    IDirect3DTexture2 *dst_tex;
    DDSURFACEDESC2 ddsd;
    HRESULT hr;
    DDCOLORKEY ckey;

    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddsd.dwHeight = 128;
    ddsd.dwWidth = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &src, NULL);
    ok(SUCCEEDED(hr), "Failed to create source texture, hr %#x.\n", hr);
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &dst, NULL);
    ok(SUCCEEDED(hr), "Failed to create destination texture, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_QueryInterface(src, &IID_IDirect3DTexture2, (void **)&src_tex);
    ok(SUCCEEDED(hr) || hr == E_NOINTERFACE, "Failed to get Direct3DTexture2 interface, hr %#x.\n", hr);
    if (FAILED(hr))
    {
        /* 64 bit ddraw does not support d3d */
        skip("Could not get Direct3DTexture2 interface, skipping texture::Load color keying tests.\n");
        IDirectDrawSurface4_Release(dst);
        IDirectDrawSurface4_Release(src);
        IDirectDraw4_Release(ddraw);
        return;
    }
    hr = IDirectDrawSurface4_QueryInterface(dst, &IID_IDirect3DTexture2, (void **)&dst_tex);
    ok(SUCCEEDED(hr), "Failed to get Direct3DTexture2 interface, hr %#x.\n", hr);

    /* No surface has a color key */
    hr = IDirect3DTexture2_Load(dst_tex, src_tex);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    ckey.dwColorSpaceLowValue = ckey.dwColorSpaceHighValue = 0xdeadbeef;
    hr = IDirectDrawSurface4_GetColorKey(dst, DDCKEY_SRCBLT, &ckey);
    ok(hr == DDERR_NOCOLORKEY, "Got unexpected hr %#x.\n", hr);
    ok(ckey.dwColorSpaceLowValue == 0xdeadbeef, "dwColorSpaceLowValue is %#x.\n", ckey.dwColorSpaceLowValue);
    ok(ckey.dwColorSpaceHighValue == 0xdeadbeef, "dwColorSpaceHighValue is %#x.\n", ckey.dwColorSpaceHighValue);

    /* Source surface has a color key */
    ckey.dwColorSpaceLowValue = ckey.dwColorSpaceHighValue = 0x0000ff00;
    hr = IDirectDrawSurface4_SetColorKey(src, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);
    hr = IDirect3DTexture2_Load(dst_tex, src_tex);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetColorKey(dst, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    ok(ckey.dwColorSpaceLowValue == 0x0000ff00, "dwColorSpaceLowValue is %#x.\n", ckey.dwColorSpaceLowValue);
    ok(ckey.dwColorSpaceHighValue == 0x0000ff00, "dwColorSpaceHighValue is %#x.\n", ckey.dwColorSpaceHighValue);

    /* Both surfaces have a color key: Dest ckey is overwritten */
    ckey.dwColorSpaceLowValue = ckey.dwColorSpaceHighValue = 0x000000ff;
    hr = IDirectDrawSurface4_SetColorKey(dst, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);
    hr = IDirect3DTexture2_Load(dst_tex, src_tex);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetColorKey(dst, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    ok(ckey.dwColorSpaceLowValue == 0x0000ff00, "dwColorSpaceLowValue is %#x.\n", ckey.dwColorSpaceLowValue);
    ok(ckey.dwColorSpaceHighValue == 0x0000ff00, "dwColorSpaceHighValue is %#x.\n", ckey.dwColorSpaceHighValue);

    /* Only the destination has a color key: It is not deleted */
    hr = IDirectDrawSurface4_SetColorKey(src, DDCKEY_SRCBLT, NULL);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetColorKey(src, DDCKEY_SRCBLT, &ckey);
    ok(hr == DDERR_NOCOLORKEY, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DTexture2_Load(dst_tex, src_tex);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetColorKey(dst, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    ok(ckey.dwColorSpaceLowValue == 0x0000ff00, "dwColorSpaceLowValue is %#x.\n", ckey.dwColorSpaceLowValue);
    ok(ckey.dwColorSpaceHighValue == 0x0000ff00, "dwColorSpaceHighValue is %#x.\n", ckey.dwColorSpaceHighValue);

    IDirect3DTexture2_Release(dst_tex);
    IDirect3DTexture2_Release(src_tex);
    IDirectDrawSurface4_Release(dst);
    IDirectDrawSurface4_Release(src);
    IDirectDraw4_Release(ddraw);
}

static ULONG get_refcount(IUnknown *test_iface)
{
    IUnknown_AddRef(test_iface);
    return IUnknown_Release(test_iface);
}

static void test_viewport(void)
{
    IDirectDraw4 *ddraw;
    IDirect3D3 *d3d;
    HRESULT hr, old_d3d_ref;
    ULONG ref;
    IDirect3DViewport *viewport;
    IDirect3DViewport2 *viewport2;
    IDirect3DViewport3 *viewport3, *another_vp, *test_vp;
    IDirectDrawGammaControl *gamma;
    IUnknown *unknown;
    HWND window;
    IDirect3DDevice3 *device;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }
    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get Direct3D3 interface, hr %#x.\n", hr);
    hr = IDirect3D3_QueryInterface(d3d, &IID_IDirectDraw4, (void **)&ddraw);
    ok(SUCCEEDED(hr), "Failed to get DirectDraw4 interface, hr %#x.\n", hr);
    old_d3d_ref = get_refcount((IUnknown *) d3d);

    hr = IDirect3D3_CreateViewport(d3d, &viewport3, NULL);
    ok(SUCCEEDED(hr), "Failed to create viewport, hr %#x.\n", hr);
    ref = get_refcount((IUnknown *)viewport3);
    ok(ref == 1, "Initial IDirect3DViewport3 refcount is %u\n", ref);
    ref = get_refcount((IUnknown *)d3d);
    ok(ref == old_d3d_ref, "IDirect3D3 refcount is %u\n", ref);

    gamma = (IDirectDrawGammaControl *)0xdeadbeef;
    hr = IDirect3DViewport2_QueryInterface(viewport3, &IID_IDirectDrawGammaControl, (void **)&gamma);
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#x.\n", hr);
    ok(gamma == NULL, "Interface not set to NULL by failed QI call: %p\n", gamma);
    if (SUCCEEDED(hr)) IDirectDrawGammaControl_Release(gamma);
    /* NULL iid: Segfaults */

    hr = IDirect3DViewport3_QueryInterface(viewport3, &IID_IDirect3DViewport, (void **)&viewport);
    ok(SUCCEEDED(hr), "Failed to QI IDirect3DViewport, hr %#x.\n", hr);
    if (viewport)
    {
        ref = get_refcount((IUnknown *)viewport);
        ok(ref == 2, "IDirect3DViewport refcount is %u\n", ref);
        ref = get_refcount((IUnknown *)viewport3);
        ok(ref == 2, "IDirect3DViewport3 refcount is %u\n", ref);
        IDirect3DViewport_Release(viewport);
        viewport = NULL;
    }

    hr = IDirect3DViewport3_QueryInterface(viewport3, &IID_IDirect3DViewport3, (void **)&viewport2);
    ok(SUCCEEDED(hr), "Failed to QI IDirect3DViewport3, hr %#x.\n", hr);
    if (viewport2)
    {
        ref = get_refcount((IUnknown *)viewport2);
        ok(ref == 2, "IDirect3DViewport2 refcount is %u\n", ref);
        ref = get_refcount((IUnknown *)viewport3);
        ok(ref == 2, "IDirect3DViewport3 refcount is %u\n", ref);
        IDirect3DViewport3_Release(viewport2);
    }

    hr = IDirect3DViewport3_QueryInterface(viewport3, &IID_IUnknown, (void **)&unknown);
    ok(SUCCEEDED(hr), "Failed to QI IUnknown, hr %#x.\n", hr);
    if (unknown)
    {
        ref = get_refcount((IUnknown *)viewport3);
        ok(ref == 2, "IDirect3DViewport3 refcount is %u\n", ref);
        ref = get_refcount(unknown);
        ok(ref == 2, "IUnknown refcount is %u\n", ref);
        IUnknown_Release(unknown);
    }

    /* AddViewport(NULL): Segfault */
    hr = IDirect3DDevice3_DeleteViewport(device, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice3_GetCurrentViewport(device, NULL);
    ok(hr == D3DERR_NOCURRENTVIEWPORT, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3D3_CreateViewport(d3d, &another_vp, NULL);
    ok(SUCCEEDED(hr), "Failed to create viewport, hr %#x.\n", hr);

    /* Setting a viewport not in the viewport list fails */
    hr = IDirect3DDevice3_SetCurrentViewport(device, another_vp);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice3_AddViewport(device, viewport3);
    ok(SUCCEEDED(hr), "Failed to add viewport to device, hr %#x.\n", hr);
    ref = get_refcount((IUnknown *) viewport3);
    ok(ref == 2, "viewport3 refcount is %d\n", ref);
    hr = IDirect3DDevice3_AddViewport(device, another_vp);
    ok(SUCCEEDED(hr), "Failed to add viewport to device, hr %#x.\n", hr);
    ref = get_refcount((IUnknown *) another_vp);
    ok(ref == 2, "another_vp refcount is %d\n", ref);

    test_vp = (IDirect3DViewport3 *) 0xbaadc0de;
    hr = IDirect3DDevice3_GetCurrentViewport(device, &test_vp);
    ok(hr == D3DERR_NOCURRENTVIEWPORT, "Got unexpected hr %#x.\n", hr);
    ok(test_vp == (IDirect3DViewport3 *) 0xbaadc0de, "Got unexpected pointer %p\n", test_vp);

    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport3);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);
    ref = get_refcount((IUnknown *) viewport3);
    ok(ref == 3, "viewport3 refcount is %d\n", ref);
    ref = get_refcount((IUnknown *) device);
    ok(ref == 1, "device refcount is %d\n", ref);

    test_vp = NULL;
    hr = IDirect3DDevice3_GetCurrentViewport(device, &test_vp);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(test_vp == viewport3, "Got unexpected viewport %p\n", test_vp);
    ref = get_refcount((IUnknown *) viewport3);
    ok(ref == 4, "viewport3 refcount is %d\n", ref);
    if(test_vp) IDirect3DViewport3_Release(test_vp);

    /* GetCurrentViewport with a viewport set and NULL input param: Segfault */

    /* Cannot set the viewport to NULL */
    hr = IDirect3DDevice3_SetCurrentViewport(device, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "Failed to set viewport to NULL, hr %#x.\n", hr);
    test_vp = NULL;
    hr = IDirect3DDevice3_GetCurrentViewport(device, &test_vp);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(test_vp == viewport3, "Got unexpected viewport %p\n", test_vp);
    if(test_vp) IDirect3DViewport3_Release(test_vp);

    /* SetCurrentViewport properly releases the old viewport's reference */
    hr = IDirect3DDevice3_SetCurrentViewport(device, another_vp);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);
    ref = get_refcount((IUnknown *) viewport3);
    ok(ref == 2, "viewport3 refcount is %d\n", ref);
    ref = get_refcount((IUnknown *) another_vp);
    ok(ref == 3, "another_vp refcount is %d\n", ref);

    /* Unlike device2::DeleteViewport, device3::DeleteViewport releases the
     * reference held by SetCurrentViewport */
    hr = IDirect3DDevice3_DeleteViewport(device, another_vp);
    ok(SUCCEEDED(hr), "Failed to delete viewport from device, hr %#x.\n", hr);
    ref = get_refcount((IUnknown *) another_vp);
    ok(ref == 1, "another_vp refcount is %d\n", ref);

    /* GetCurrentViewport still fails */
    test_vp = NULL;
    hr = IDirect3DDevice3_GetCurrentViewport(device, &test_vp);
    ok(hr == D3DERR_NOCURRENTVIEWPORT, "Got unexpected hr %#x.\n", hr);
    ok(test_vp == NULL, "Got unexpected viewport %p\n", test_vp);
    if(test_vp) IDirect3DViewport3_Release(test_vp);

    /* Setting a different viewport doesn't have any surprises now */
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport3);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);
    ref = get_refcount((IUnknown *) viewport3);
    ok(ref == 3, "viewport3 refcount is %d\n", ref);
    ref = get_refcount((IUnknown *) another_vp);
    ok(ref == 1, "another_vp refcount is %d\n", ref);

    /* Destroying the device removes the viewport and releases the reference */
    IDirect3DDevice3_Release(device);
    ref = get_refcount((IUnknown *) viewport3);
    ok(ref == 1, "viewport3 refcount is %d\n", ref);

    ref = IDirect3DViewport3_Release(another_vp);
    ok(ref == 0, "Got unexpected ref %d\n", ref);
    ref = IDirect3DViewport3_Release(viewport3);
    ok(ref == 0, "Got unexpected ref %d\n", ref);
    IDirect3D3_Release(d3d);
    DestroyWindow(window);
    IDirectDraw4_Release(ddraw);
}

static void test_zenable(void)
{
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    static struct
    {
        struct vec4 position;
        D3DCOLOR diffuse;
    }
    tquad[] =
    {
        {{  0.0f, 480.0f, -0.5f, 1.0f}, 0xff00ff00},
        {{  0.0f,   0.0f, -0.5f, 1.0f}, 0xff00ff00},
        {{640.0f, 480.0f,  1.5f, 1.0f}, 0xff00ff00},
        {{640.0f,   0.0f,  1.5f, 1.0f}, 0xff00ff00},
    };
    IDirect3DViewport3 *viewport;
    IDirect3DDevice3 *device;
    IDirectDrawSurface4 *rt;
    D3DCOLOR color;
    HWND window;
    HRESULT hr;
    UINT x, y;
    UINT i, j;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    viewport = create_viewport(device, 0, 0, 640, 480);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_ZENABLE, D3DZB_FALSE);
    ok(SUCCEEDED(hr), "Failed to disable z-buffering, hr %#x.\n", hr);

    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffff0000, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, tquad, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);
    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            x = 80 * ((2 * j) + 1);
            y = 60 * ((2 * i) + 1);
            color = get_surface_color(rt, x, y);
            ok(compare_color(color, 0x0000ff00, 1),
                    "Expected color 0x0000ff00 at %u, %u, got 0x%08x.\n", x, y, color);
        }
    }
    IDirectDrawSurface4_Release(rt);

    destroy_viewport(device, viewport);
    IDirect3DDevice3_Release(device);
    DestroyWindow(window);
}

static void test_ck_rgba(void)
{
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    static struct
    {
        struct vec4 position;
        struct vec2 texcoord;
    }
    tquad[] =
    {
        {{  0.0f, 480.0f, 0.25f, 1.0f}, {0.0f, 0.0f}},
        {{  0.0f,   0.0f, 0.25f, 1.0f}, {0.0f, 1.0f}},
        {{640.0f, 480.0f, 0.25f, 1.0f}, {1.0f, 0.0f}},
        {{640.0f,   0.0f, 0.25f, 1.0f}, {1.0f, 1.0f}},
        {{  0.0f, 480.0f, 0.75f, 1.0f}, {0.0f, 0.0f}},
        {{  0.0f,   0.0f, 0.75f, 1.0f}, {0.0f, 1.0f}},
        {{640.0f, 480.0f, 0.75f, 1.0f}, {1.0f, 0.0f}},
        {{640.0f,   0.0f, 0.75f, 1.0f}, {1.0f, 1.0f}},
    };
    static const struct
    {
        D3DCOLOR fill_color;
        BOOL color_key;
        BOOL blend;
        D3DCOLOR result1;
        D3DCOLOR result2;
    }
    tests[] =
    {
        {0xff00ff00, TRUE,  TRUE,  0x00ff0000, 0x000000ff},
        {0xff00ff00, TRUE,  FALSE, 0x00ff0000, 0x000000ff},
        {0xff00ff00, FALSE, TRUE,  0x0000ff00, 0x0000ff00},
        {0xff00ff00, FALSE, FALSE, 0x0000ff00, 0x0000ff00},
        {0x7f00ff00, TRUE,  TRUE,  0x00807f00, 0x00807f00},
        {0x7f00ff00, TRUE,  FALSE, 0x0000ff00, 0x0000ff00},
        {0x7f00ff00, FALSE, TRUE,  0x00807f00, 0x00807f00},
        {0x7f00ff00, FALSE, FALSE, 0x0000ff00, 0x0000ff00},
    };

    IDirectDrawSurface4 *surface;
    IDirect3DViewport3 *viewport;
    DDSURFACEDESC2 surface_desc;
    IDirect3DTexture2 *texture;
    IDirect3DDevice3 *device;
    IDirectDrawSurface4 *rt;
    IDirectDraw4 *ddraw;
    IDirect3D3 *d3d;
    D3DCOLOR color;
    HWND window;
    DDBLTFX fx;
    HRESULT hr;
    UINT i;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    viewport = create_viewport(device, 0, 0, 640, 480);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);
    hr = IDirect3D3_QueryInterface(d3d, &IID_IDirectDraw4, (void **)&ddraw);
    ok(SUCCEEDED(hr), "Failed to get ddraw interface, hr %#x.\n", hr);
    IDirect3D3_Release(d3d);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CKSRCBLT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    surface_desc.dwWidth = 256;
    surface_desc.dwHeight = 256;
    U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 32;
    U2(U4(surface_desc).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(U4(surface_desc).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(U4(surface_desc).ddpfPixelFormat).dwBBitMask = 0x000000ff;
    U5(U4(surface_desc).ddpfPixelFormat).dwRGBAlphaBitMask = 0xff000000;
    surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue = 0xff00ff00;
    surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue = 0xff00ff00;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create destination surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_QueryInterface(surface, &IID_IDirect3DTexture2, (void **)&texture);
    ok(SUCCEEDED(hr), "Failed to get texture interface, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetTexture(device, 0, texture);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
    ok(SUCCEEDED(hr), "Failed to enable alpha blending, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
    ok(SUCCEEDED(hr), "Failed to enable alpha blending, hr %#x.\n", hr);

    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    for (i = 0; i < sizeof(tests) / sizeof(*tests); ++i)
    {
        hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_COLORKEYENABLE, tests[i].color_key);
        ok(SUCCEEDED(hr), "Failed to enable color keying, hr %#x.\n", hr);
        hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_ALPHABLENDENABLE, tests[i].blend);
        ok(SUCCEEDED(hr), "Failed to enable alpha blending, hr %#x.\n", hr);

        memset(&fx, 0, sizeof(fx));
        fx.dwSize = sizeof(fx);
        U5(fx).dwFillColor = tests[i].fill_color;
        hr = IDirectDrawSurface4_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
        ok(SUCCEEDED(hr), "Failed to fill texture, hr %#x.\n", hr);

        hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect,
                D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffff0000, 1.0f, 0);
        ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
        hr = IDirect3DDevice3_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
        hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZRHW | D3DFVF_TEX1, &tquad[0], 4, 0);
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
        hr = IDirect3DDevice3_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

        color = get_surface_color(rt, 320, 240);
        if (i == 2)
            todo_wine ok(compare_color(color, tests[i].result1, 1), "Expected color 0x%08x for test %u, got 0x%08x.\n",
                    tests[i].result1, i, color);
        else
            ok(compare_color(color, tests[i].result1, 1), "Expected color 0x%08x for test %u, got 0x%08x.\n",
                    tests[i].result1, i, color);

        U5(fx).dwFillColor = 0xff0000ff;
        hr = IDirectDrawSurface4_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
        ok(SUCCEEDED(hr), "Failed to fill texture, hr %#x.\n", hr);

        hr = IDirect3DDevice3_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
        hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZRHW | D3DFVF_TEX1, &tquad[4], 4, 0);
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
        hr = IDirect3DDevice3_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

        /* This tests that fragments that are masked out by the color key are
         * discarded, instead of just fully transparent. */
        color = get_surface_color(rt, 320, 240);
        if (i == 2)
            todo_wine ok(compare_color(color, tests[i].result2, 1), "Expected color 0x%08x for test %u, got 0x%08x.\n",
                    tests[i].result2, i, color);
        else
            ok(compare_color(color, tests[i].result2, 1), "Expected color 0x%08x for test %u, got 0x%08x.\n",
                    tests[i].result2, i, color);
    }

    IDirectDrawSurface4_Release(rt);
    IDirect3DTexture2_Release(texture);
    IDirectDrawSurface4_Release(surface);
    destroy_viewport(device, viewport);
    IDirectDraw4_Release(ddraw);
    IDirect3DDevice3_Release(device);
    DestroyWindow(window);
}

static void test_ck_default(void)
{
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    static struct
    {
        struct vec4 position;
        struct vec2 texcoord;
    }
    tquad[] =
    {
        {{  0.0f, 480.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{  0.0f,   0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{640.0f, 480.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{640.0f,   0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    };
    IDirectDrawSurface4 *surface, *rt;
    IDirect3DViewport3 *viewport;
    DDSURFACEDESC2 surface_desc;
    IDirect3DTexture2 *texture;
    IDirect3DDevice3 *device;
    IDirectDraw4 *ddraw;
    IDirect3D3 *d3d;
    D3DCOLOR color;
    DWORD value;
    HWND window;
    DDBLTFX fx;
    HRESULT hr;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);

    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);
    hr = IDirect3D3_QueryInterface(d3d, &IID_IDirectDraw4, (void **)&ddraw);
    ok(SUCCEEDED(hr), "Failed to get ddraw interface, hr %#x.\n", hr);
    IDirect3D3_Release(d3d);

    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    viewport = create_viewport(device, 0, 0, 640, 480);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CKSRCBLT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    surface_desc.dwWidth = 256;
    surface_desc.dwHeight = 256;
    U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 32;
    U2(U4(surface_desc).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(U4(surface_desc).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(U4(surface_desc).ddpfPixelFormat).dwBBitMask = 0x000000ff;
    surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue = 0x000000ff;
    surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue = 0x000000ff;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_QueryInterface(surface, &IID_IDirect3DTexture2, (void **)&texture);
    ok(SUCCEEDED(hr), "Failed to get texture interface, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTexture(device, 0, texture);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    U5(fx).dwFillColor = 0x000000ff;
    hr = IDirectDrawSurface4_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to fill surface, hr %#x.\n", hr);

    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0xff00ff00, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice3_GetRenderState(device, D3DRENDERSTATE_COLORKEYENABLE, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
    ok(!value, "Got unexpected color keying state %#x.\n", value);
    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZRHW | D3DFVF_TEX1, &tquad[0], 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x000000ff, 1), "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0xff00ff00, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_COLORKEYENABLE, TRUE);
    ok(SUCCEEDED(hr), "Failed to enable color keying, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZRHW | D3DFVF_TEX1, &tquad[0], 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_GetRenderState(device, D3DRENDERSTATE_COLORKEYENABLE, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
    ok(!!value, "Got unexpected color keying state %#x.\n", value);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x0000ff00, 1), "Got unexpected color 0x%08x.\n", color);

    IDirect3DTexture_Release(texture);
    IDirectDrawSurface4_Release(surface);
    destroy_viewport(device, viewport);
    IDirectDrawSurface4_Release(rt);
    IDirect3DDevice3_Release(device);
    IDirectDraw4_Release(ddraw);
    DestroyWindow(window);
}

static void test_ck_complex(void)
{
    IDirectDrawSurface4 *surface, *mipmap, *tmp;
    DDSCAPS2 caps = {DDSCAPS_COMPLEX, 0, 0, 0};
    DDSURFACEDESC2 surface_desc;
    IDirect3DDevice3 *device;
    DDCOLORKEY color_key;
    IDirectDraw4 *ddraw;
    IDirect3D3 *d3d;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }
    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);
    hr = IDirect3D3_QueryInterface(d3d, &IID_IDirectDraw4, (void **)&ddraw);
    ok(SUCCEEDED(hr), "Failed to get ddraw interface, hr %#x.\n", hr);
    IDirect3D3_Release(d3d);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
    surface_desc.dwWidth = 128;
    surface_desc.dwHeight = 128;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_GetColorKey(surface, DDCKEY_SRCBLT, &color_key);
    ok(hr == DDERR_NOCOLORKEY, "Got unexpected hr %#x.\n", hr);
    color_key.dwColorSpaceLowValue = 0x0000ff00;
    color_key.dwColorSpaceHighValue = 0x0000ff00;
    hr = IDirectDrawSurface4_SetColorKey(surface, DDCKEY_SRCBLT, &color_key);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);
    memset(&color_key, 0, sizeof(color_key));
    hr = IDirectDrawSurface4_GetColorKey(surface, DDCKEY_SRCBLT, &color_key);
    ok(SUCCEEDED(hr), "Failed to get color key, hr %#x.\n", hr);
    ok(color_key.dwColorSpaceLowValue == 0x0000ff00, "Got unexpected value 0x%08x.\n",
            color_key.dwColorSpaceLowValue);
    ok(color_key.dwColorSpaceHighValue == 0x0000ff00, "Got unexpected value 0x%08x.\n",
            color_key.dwColorSpaceHighValue);

    mipmap = surface;
    IDirectDrawSurface_AddRef(mipmap);
    for (i = 0; i < 7; ++i)
    {
        hr = IDirectDrawSurface4_GetAttachedSurface(mipmap, &caps, &tmp);
        ok(SUCCEEDED(hr), "Failed to get attached surface, i %u, hr %#x.\n", i, hr);

        hr = IDirectDrawSurface4_GetColorKey(tmp, DDCKEY_SRCBLT, &color_key);
        ok(hr == DDERR_NOCOLORKEY, "Got unexpected hr %#x, i %u.\n", hr, i);
        color_key.dwColorSpaceLowValue = 0x000000ff;
        color_key.dwColorSpaceHighValue = 0x000000ff;
        hr = IDirectDrawSurface4_SetColorKey(tmp, DDCKEY_SRCBLT, &color_key);
        ok(SUCCEEDED(hr), "Failed to set color key, hr %#x, i %u.\n", hr, i);
        memset(&color_key, 0, sizeof(color_key));
        hr = IDirectDrawSurface4_GetColorKey(tmp, DDCKEY_SRCBLT, &color_key);
        ok(SUCCEEDED(hr), "Failed to get color key, hr %#x, i %u.\n", hr, i);
        ok(color_key.dwColorSpaceLowValue == 0x000000ff, "Got unexpected value 0x%08x, i %u.\n",
                color_key.dwColorSpaceLowValue, i);
        ok(color_key.dwColorSpaceHighValue == 0x000000ff, "Got unexpected value 0x%08x, i %u.\n",
                color_key.dwColorSpaceHighValue, i);

        IDirectDrawSurface_Release(mipmap);
        mipmap = tmp;
    }

    memset(&color_key, 0, sizeof(color_key));
    hr = IDirectDrawSurface4_GetColorKey(surface, DDCKEY_SRCBLT, &color_key);
    ok(SUCCEEDED(hr), "Failed to get color key, hr %#x.\n", hr);
    ok(color_key.dwColorSpaceLowValue == 0x0000ff00, "Got unexpected value 0x%08x.\n",
            color_key.dwColorSpaceLowValue);
    ok(color_key.dwColorSpaceHighValue == 0x0000ff00, "Got unexpected value 0x%08x.\n",
            color_key.dwColorSpaceHighValue);

    hr = IDirectDrawSurface4_GetAttachedSurface(mipmap, &caps, &tmp);
    ok(hr == DDERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    IDirectDrawSurface_Release(mipmap);
    refcount = IDirectDrawSurface4_Release(surface);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;
    surface_desc.dwBackBufferCount = 1;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_GetColorKey(surface, DDCKEY_SRCBLT, &color_key);
    ok(hr == DDERR_NOCOLORKEY, "Got unexpected hr %#x.\n", hr);
    color_key.dwColorSpaceLowValue = 0x0000ff00;
    color_key.dwColorSpaceHighValue = 0x0000ff00;
    hr = IDirectDrawSurface4_SetColorKey(surface, DDCKEY_SRCBLT, &color_key);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);
    memset(&color_key, 0, sizeof(color_key));
    hr = IDirectDrawSurface4_GetColorKey(surface, DDCKEY_SRCBLT, &color_key);
    ok(SUCCEEDED(hr), "Failed to get color key, hr %#x.\n", hr);
    ok(color_key.dwColorSpaceLowValue == 0x0000ff00, "Got unexpected value 0x%08x.\n",
            color_key.dwColorSpaceLowValue);
    ok(color_key.dwColorSpaceHighValue == 0x0000ff00, "Got unexpected value 0x%08x.\n",
            color_key.dwColorSpaceHighValue);

    hr = IDirectDrawSurface4_GetAttachedSurface(surface, &caps, &tmp);
    ok(SUCCEEDED(hr), "Failed to get attached surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_GetColorKey(tmp, DDCKEY_SRCBLT, &color_key);
    ok(hr == DDERR_NOCOLORKEY, "Got unexpected hr %#x, i %u.\n", hr, i);
    color_key.dwColorSpaceLowValue = 0x0000ff00;
    color_key.dwColorSpaceHighValue = 0x0000ff00;
    hr = IDirectDrawSurface4_SetColorKey(tmp, DDCKEY_SRCBLT, &color_key);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);
    memset(&color_key, 0, sizeof(color_key));
    hr = IDirectDrawSurface4_GetColorKey(tmp, DDCKEY_SRCBLT, &color_key);
    ok(SUCCEEDED(hr), "Failed to get color key, hr %#x.\n", hr);
    ok(color_key.dwColorSpaceLowValue == 0x0000ff00, "Got unexpected value 0x%08x.\n",
            color_key.dwColorSpaceLowValue);
    ok(color_key.dwColorSpaceHighValue == 0x0000ff00, "Got unexpected value 0x%08x.\n",
            color_key.dwColorSpaceHighValue);

    IDirectDrawSurface_Release(tmp);

    refcount = IDirectDrawSurface4_Release(surface);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    IDirectDraw4_Release(ddraw);
    refcount = IDirect3DDevice3_Release(device);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

struct qi_test
{
    REFIID iid;
    REFIID refcount_iid;
    HRESULT hr;
};

static void test_qi(const char *test_name, IUnknown *base_iface,
        REFIID refcount_iid, const struct qi_test *tests, UINT entry_count)
{
    ULONG refcount, expected_refcount;
    IUnknown *iface1, *iface2;
    HRESULT hr;
    UINT i, j;

    for (i = 0; i < entry_count; ++i)
    {
        hr = IUnknown_QueryInterface(base_iface, tests[i].iid, (void **)&iface1);
        ok(hr == tests[i].hr, "Got hr %#x for test \"%s\" %u.\n", hr, test_name, i);
        if (SUCCEEDED(hr))
        {
            for (j = 0; j < entry_count; ++j)
            {
                hr = IUnknown_QueryInterface(iface1, tests[j].iid, (void **)&iface2);
                ok(hr == tests[j].hr, "Got hr %#x for test \"%s\" %u, %u.\n", hr, test_name, i, j);
                if (SUCCEEDED(hr))
                {
                    expected_refcount = 0;
                    if (IsEqualGUID(refcount_iid, tests[j].refcount_iid))
                        ++expected_refcount;
                    if (IsEqualGUID(tests[i].refcount_iid, tests[j].refcount_iid))
                        ++expected_refcount;
                    refcount = IUnknown_Release(iface2);
                    ok(refcount == expected_refcount, "Got refcount %u for test \"%s\" %u, %u, expected %u.\n",
                            refcount, test_name, i, j, expected_refcount);
                }
            }

            expected_refcount = 0;
            if (IsEqualGUID(refcount_iid, tests[i].refcount_iid))
                ++expected_refcount;
            refcount = IUnknown_Release(iface1);
            ok(refcount == expected_refcount, "Got refcount %u for test \"%s\" %u, expected %u.\n",
                    refcount, test_name, i, expected_refcount);
        }
    }
}

static void test_surface_qi(void)
{
    static const struct qi_test tests[] =
    {
        {&IID_IDirect3DTexture2,        &IID_IDirectDrawSurface4,       S_OK         },
        {&IID_IDirect3DTexture,         &IID_IDirectDrawSurface4,       S_OK         },
        {&IID_IDirectDrawGammaControl,  &IID_IDirectDrawGammaControl,   S_OK         },
        {&IID_IDirectDrawColorControl,  NULL,                           E_NOINTERFACE},
        {&IID_IDirectDrawSurface7,      &IID_IDirectDrawSurface7,       S_OK         },
        {&IID_IDirectDrawSurface4,      &IID_IDirectDrawSurface4,       S_OK         },
        {&IID_IDirectDrawSurface3,      &IID_IDirectDrawSurface3,       S_OK         },
        {&IID_IDirectDrawSurface2,      &IID_IDirectDrawSurface2,       S_OK         },
        {&IID_IDirectDrawSurface,       &IID_IDirectDrawSurface,        S_OK         },
        {&IID_IDirect3DDevice7,         NULL,                           E_INVALIDARG },
        {&IID_IDirect3DDevice3,         NULL,                           E_INVALIDARG },
        {&IID_IDirect3DDevice2,         NULL,                           E_INVALIDARG },
        {&IID_IDirect3DDevice,          NULL,                           E_INVALIDARG },
        {&IID_IDirect3D7,               NULL,                           E_INVALIDARG },
        {&IID_IDirect3D3,               NULL,                           E_INVALIDARG },
        {&IID_IDirect3D2,               NULL,                           E_INVALIDARG },
        {&IID_IDirect3D,                NULL,                           E_INVALIDARG },
        {&IID_IDirectDraw7,             NULL,                           E_INVALIDARG },
        {&IID_IDirectDraw4,             NULL,                           E_INVALIDARG },
        {&IID_IDirectDraw3,             NULL,                           E_INVALIDARG },
        {&IID_IDirectDraw2,             NULL,                           E_INVALIDARG },
        {&IID_IDirectDraw,              NULL,                           E_INVALIDARG },
        {&IID_IDirect3DLight,           NULL,                           E_INVALIDARG },
        {&IID_IDirect3DMaterial,        NULL,                           E_INVALIDARG },
        {&IID_IDirect3DMaterial2,       NULL,                           E_INVALIDARG },
        {&IID_IDirect3DMaterial3,       NULL,                           E_INVALIDARG },
        {&IID_IDirect3DExecuteBuffer,   NULL,                           E_INVALIDARG },
        {&IID_IDirect3DViewport,        NULL,                           E_INVALIDARG },
        {&IID_IDirect3DViewport2,       NULL,                           E_INVALIDARG },
        {&IID_IDirect3DViewport3,       NULL,                           E_INVALIDARG },
        {&IID_IDirect3DVertexBuffer,    NULL,                           E_INVALIDARG },
        {&IID_IDirect3DVertexBuffer7,   NULL,                           E_INVALIDARG },
        {&IID_IDirectDrawPalette,       NULL,                           E_INVALIDARG },
        {&IID_IDirectDrawClipper,       NULL,                           E_INVALIDARG },
        {&IID_IUnknown,                 &IID_IDirectDrawSurface,        S_OK         },
    };

    IDirectDrawSurface4 *surface;
    DDSURFACEDESC2 surface_desc;
    IDirect3DDevice3 *device;
    IDirectDraw4 *ddraw;
    HWND window;
    HRESULT hr;

    if (!GetProcAddress(GetModuleHandleA("ddraw.dll"), "DirectDrawCreateEx"))
    {
        win_skip("DirectDrawCreateEx not available, skipping test.\n");
        return;
    }

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    /* Try to create a D3D device to see if the ddraw implementation supports
     * D3D. 64-bit ddraw in particular doesn't seem to support D3D, and
     * doesn't support e.g. the IDirect3DTexture interfaces. */
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }
    IDirect3DDevice_Release(device);
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    surface_desc.dwWidth = 512;
    surface_desc.dwHeight = 512;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    test_qi("surface_qi", (IUnknown *)surface, &IID_IDirectDrawSurface4, tests, sizeof(tests) / sizeof(*tests));

    IDirectDrawSurface4_Release(surface);
    IDirectDraw4_Release(ddraw);
    DestroyWindow(window);
}

static void test_device_qi(void)
{
    static const struct qi_test tests[] =
    {
        {&IID_IDirect3DTexture2,        NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DTexture,         NULL,                           E_NOINTERFACE},
        {&IID_IDirectDrawGammaControl,  NULL,                           E_NOINTERFACE},
        {&IID_IDirectDrawColorControl,  NULL,                           E_NOINTERFACE},
        {&IID_IDirectDrawSurface7,      NULL,                           E_NOINTERFACE},
        {&IID_IDirectDrawSurface4,      NULL,                           E_NOINTERFACE},
        {&IID_IDirectDrawSurface3,      NULL,                           E_NOINTERFACE},
        {&IID_IDirectDrawSurface2,      NULL,                           E_NOINTERFACE},
        {&IID_IDirectDrawSurface,       NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DDevice7,         NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DDevice3,         &IID_IDirect3DDevice3,          S_OK         },
        {&IID_IDirect3DDevice2,         &IID_IDirect3DDevice3,          S_OK         },
        {&IID_IDirect3DDevice,          &IID_IDirect3DDevice3,          S_OK         },
        {&IID_IDirect3DRampDevice,      NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DRGBDevice,       NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DHALDevice,       NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DMMXDevice,       NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DRefDevice,       NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DTnLHalDevice,    NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DNullDevice,      NULL,                           E_NOINTERFACE},
        {&IID_IDirect3D7,               NULL,                           E_NOINTERFACE},
        {&IID_IDirect3D3,               NULL,                           E_NOINTERFACE},
        {&IID_IDirect3D2,               NULL,                           E_NOINTERFACE},
        {&IID_IDirect3D,                NULL,                           E_NOINTERFACE},
        {&IID_IDirectDraw7,             NULL,                           E_NOINTERFACE},
        {&IID_IDirectDraw4,             NULL,                           E_NOINTERFACE},
        {&IID_IDirectDraw3,             NULL,                           E_NOINTERFACE},
        {&IID_IDirectDraw2,             NULL,                           E_NOINTERFACE},
        {&IID_IDirectDraw,              NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DLight,           NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DMaterial,        NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DMaterial2,       NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DMaterial3,       NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DExecuteBuffer,   NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DViewport,        NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DViewport2,       NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DViewport3,       NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DVertexBuffer,    NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DVertexBuffer7,   NULL,                           E_NOINTERFACE},
        {&IID_IDirectDrawPalette,       NULL,                           E_NOINTERFACE},
        {&IID_IDirectDrawClipper,       NULL,                           E_NOINTERFACE},
        {&IID_IUnknown,                 &IID_IDirect3DDevice3,          S_OK         },
    };

    IDirect3DDevice3 *device;
    HWND window;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    test_qi("device_qi", (IUnknown *)device, &IID_IDirect3DDevice3, tests, sizeof(tests) / sizeof(*tests));

    IDirect3DDevice3_Release(device);
    DestroyWindow(window);
}

static void test_wndproc(void)
{
    LONG_PTR proc, ddraw_proc;
    IDirectDraw4 *ddraw;
    WNDCLASSA wc = {0};
    HWND window;
    HRESULT hr;
    ULONG ref;

    static const UINT messages[] =
    {
        WM_WINDOWPOSCHANGING,
        WM_MOVE,
        WM_SIZE,
        WM_WINDOWPOSCHANGING,
        WM_ACTIVATE,
        WM_SETFOCUS,
        0,
    };

    /* DDSCL_EXCLUSIVE replaces the window's window proc. */
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    wc.lpfnWndProc = test_proc;
    wc.lpszClassName = "ddraw_test_wndproc_wc";
    ok(RegisterClassA(&wc), "Failed to register window class.\n");

    window = CreateWindowA("ddraw_test_wndproc_wc", "ddraw_test",
            WS_MAXIMIZE | WS_CAPTION , 0, 0, 640, 480, 0, 0, 0, 0);

    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    expect_messages = messages;
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    ok(!*expect_messages, "Expected message %#x, but didn't receive it.\n", *expect_messages);
    expect_messages = NULL;
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    ref = IDirectDraw4_Release(ddraw);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    /* DDSCL_NORMAL doesn't. */
    ddraw = create_ddraw();
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    ref = IDirectDraw4_Release(ddraw);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    /* The original window proc is only restored by ddraw if the current
     * window proc matches the one ddraw set. This also affects switching
     * from DDSCL_NORMAL to DDSCL_EXCLUSIVE. */
    ddraw = create_ddraw();
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    ddraw_proc = proc;
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    proc = SetWindowLongPtrA(window, GWLP_WNDPROC, (LONG_PTR)DefWindowProcA);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)DefWindowProcA, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)DefWindowProcA, proc);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    proc = SetWindowLongPtrA(window, GWLP_WNDPROC, (LONG_PTR)ddraw_proc);
    ok(proc == (LONG_PTR)DefWindowProcA, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)DefWindowProcA, proc);
    ref = IDirectDraw4_Release(ddraw);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    ddraw = create_ddraw();
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    proc = SetWindowLongPtrA(window, GWLP_WNDPROC, (LONG_PTR)DefWindowProcA);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    ref = IDirectDraw4_Release(ddraw);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)DefWindowProcA, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)DefWindowProcA, proc);

    fix_wndproc(window, (LONG_PTR)test_proc);
    expect_messages = NULL;
    DestroyWindow(window);
    UnregisterClassA("ddraw_test_wndproc_wc", GetModuleHandleA(NULL));
}

static void test_window_style(void)
{
    LONG style, exstyle, tmp;
    RECT fullscreen_rect, r;
    IDirectDraw4 *ddraw;
    HWND window;
    HRESULT hr;
    ULONG ref;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 100, 100, 0, 0, 0, 0);
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    style = GetWindowLongA(window, GWL_STYLE);
    exstyle = GetWindowLongA(window, GWL_EXSTYLE);
    SetRect(&fullscreen_rect, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    tmp = GetWindowLongA(window, GWL_STYLE);
    todo_wine ok(tmp == style, "Expected window style %#x, got %#x.\n", style, tmp);
    tmp = GetWindowLongA(window, GWL_EXSTYLE);
    todo_wine ok(tmp == exstyle, "Expected window extended style %#x, got %#x.\n", exstyle, tmp);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &fullscreen_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            fullscreen_rect.left, fullscreen_rect.top, fullscreen_rect.right, fullscreen_rect.bottom,
            r.left, r.top, r.right, r.bottom);
    GetClientRect(window, &r);
    todo_wine ok(!EqualRect(&r, &fullscreen_rect), "Client rect and window rect are equal.\n");

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    tmp = GetWindowLongA(window, GWL_STYLE);
    todo_wine ok(tmp == style, "Expected window style %#x, got %#x.\n", style, tmp);
    tmp = GetWindowLongA(window, GWL_EXSTYLE);
    todo_wine ok(tmp == exstyle, "Expected window extended style %#x, got %#x.\n", exstyle, tmp);

    ref = IDirectDraw4_Release(ddraw);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);

    DestroyWindow(window);
}

static void test_redundant_mode_set(void)
{
    DDSURFACEDESC2 surface_desc = {0};
    IDirectDraw4 *ddraw;
    HWND window;
    HRESULT hr;
    RECT r, s;
    ULONG ref;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 100, 100, 0, 0, 0, 0);
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDraw4_GetDisplayMode(ddraw, &surface_desc);
    ok(SUCCEEDED(hr), "GetDipslayMode failed, hr %#x.\n", hr);

    hr = IDirectDraw4_SetDisplayMode(ddraw, surface_desc.dwWidth, surface_desc.dwHeight,
            U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount, 0, 0);
    ok(SUCCEEDED(hr), "SetDisplayMode failed, hr %#x.\n", hr);

    GetWindowRect(window, &r);
    r.right /= 2;
    r.bottom /= 2;
    SetWindowPos(window, HWND_TOP, r.left, r.top, r.right, r.bottom, 0);
    GetWindowRect(window, &s);
    ok(EqualRect(&r, &s), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            r.left, r.top, r.right, r.bottom,
            s.left, s.top, s.right, s.bottom);

    hr = IDirectDraw4_SetDisplayMode(ddraw, surface_desc.dwWidth, surface_desc.dwHeight,
            U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount, 0, 0);
    ok(SUCCEEDED(hr), "SetDisplayMode failed, hr %#x.\n", hr);

    GetWindowRect(window, &s);
    ok(EqualRect(&r, &s), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            r.left, r.top, r.right, r.bottom,
            s.left, s.top, s.right, s.bottom);

    ref = IDirectDraw4_Release(ddraw);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);

    DestroyWindow(window);
}

static SIZE screen_size, screen_size2;

static LRESULT CALLBACK mode_set_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (message == WM_SIZE)
    {
        screen_size.cx = GetSystemMetrics(SM_CXSCREEN);
        screen_size.cy = GetSystemMetrics(SM_CYSCREEN);
    }

    return test_proc(hwnd, message, wparam, lparam);
}

static LRESULT CALLBACK mode_set_proc2(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (message == WM_SIZE)
    {
        screen_size2.cx = GetSystemMetrics(SM_CXSCREEN);
        screen_size2.cy = GetSystemMetrics(SM_CYSCREEN);
    }

    return test_proc(hwnd, message, wparam, lparam);
}

static void test_coop_level_mode_set(void)
{
    IDirectDrawSurface4 *primary;
    RECT fullscreen_rect, r, s;
    IDirectDraw4 *ddraw;
    DDSURFACEDESC2 ddsd;
    WNDCLASSA wc = {0};
    HWND window, window2;
    HRESULT hr;
    ULONG ref;
    MSG msg;

    static const UINT exclusive_messages[] =
    {
        WM_WINDOWPOSCHANGING,
        WM_WINDOWPOSCHANGED,
        WM_SIZE,
        WM_DISPLAYCHANGE,
        0,
    };

    static const UINT normal_messages[] =
    {
        WM_DISPLAYCHANGE,
        0,
    };

    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    wc.lpfnWndProc = mode_set_proc;
    wc.lpszClassName = "ddraw_test_wndproc_wc";
    ok(RegisterClassA(&wc), "Failed to register window class.\n");
    wc.lpfnWndProc = mode_set_proc2;
    wc.lpszClassName = "ddraw_test_wndproc_wc2";
    ok(RegisterClassA(&wc), "Failed to register window class.\n");

    window = CreateWindowA("ddraw_test_wndproc_wc", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 100, 100, 0, 0, 0, 0);
    window2 = CreateWindowA("ddraw_test_wndproc_wc2", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 100, 100, 0, 0, 0, 0);

    SetRect(&fullscreen_rect, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    SetRect(&s, 0, 0, 640, 480);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &fullscreen_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            fullscreen_rect.left, fullscreen_rect.top, fullscreen_rect.right, fullscreen_rect.bottom,
            r.left, r.top, r.right, r.bottom);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == fullscreen_rect.right - fullscreen_rect.left, "Expected surface width %u, got %u.\n",
            fullscreen_rect.right - fullscreen_rect.left, ddsd.dwWidth);
    ok(ddsd.dwHeight == fullscreen_rect.bottom - fullscreen_rect.top, "Expected surface height %u, got %u.\n",
            fullscreen_rect.bottom - fullscreen_rect.top, ddsd.dwHeight);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &fullscreen_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            fullscreen_rect.left, fullscreen_rect.top, fullscreen_rect.right, fullscreen_rect.bottom,
            r.left, r.top, r.right, r.bottom);

    PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE);
    expect_messages = exclusive_messages;
    screen_size.cx = 0;
    screen_size.cy = 0;

    hr = set_display_mode(ddraw, 640, 480);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);

    ok(!*expect_messages, "Expected message %#x, but didn't receive it.\n", *expect_messages);
    expect_messages = NULL;
    ok(screen_size.cx == s.right && screen_size.cy == s.bottom,
            "Expected screen size %ux%u, got %ux%u.\n",
            s.right, s.bottom, screen_size.cx, screen_size.cy);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &s), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            s.left, s.top, s.right, s.bottom,
            r.left, r.top, r.right, r.bottom);

    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == fullscreen_rect.right - fullscreen_rect.left, "Expected surface width %u, got %u.\n",
            fullscreen_rect.right - fullscreen_rect.left, ddsd.dwWidth);
    ok(ddsd.dwHeight == fullscreen_rect.bottom - fullscreen_rect.top, "Expected surface height %u, got %u.\n",
            fullscreen_rect.bottom - fullscreen_rect.top, ddsd.dwHeight);
    IDirectDrawSurface4_Release(primary);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == s.right - s.left, "Expected surface width %u, got %u.\n",
            s.right - s.left, ddsd.dwWidth);
    ok(ddsd.dwHeight == s.bottom - s.top, "Expected surface height %u, got %u.\n",
            s.bottom - s.top, ddsd.dwHeight);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &s), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            s.left, s.top, s.right, s.bottom,
            r.left, r.top, r.right, r.bottom);

    PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE);
    expect_messages = exclusive_messages;
    screen_size.cx = 0;
    screen_size.cy = 0;

    hr = IDirectDraw_RestoreDisplayMode(ddraw);
    ok(SUCCEEDED(hr), "RestoreDisplayMode failed, hr %#x.\n", hr);

    ok(!*expect_messages, "Expected message %#x, but didn't receive it.\n", *expect_messages);
    expect_messages = NULL;
    ok(screen_size.cx == fullscreen_rect.right && screen_size.cy == fullscreen_rect.bottom,
            "Expected screen size %ux%u, got %ux%u.\n",
            fullscreen_rect.right, fullscreen_rect.bottom, screen_size.cx, screen_size.cy);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &fullscreen_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            fullscreen_rect.left, fullscreen_rect.top, fullscreen_rect.right, fullscreen_rect.bottom,
            r.left, r.top, r.right, r.bottom);

    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == s.right - s.left, "Expected surface width %u, got %u.\n",
            s.right - s.left, ddsd.dwWidth);
    ok(ddsd.dwHeight == s.bottom - s.top, "Expected surface height %u, got %u.\n",
            s.bottom - s.top, ddsd.dwHeight);
    IDirectDrawSurface4_Release(primary);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == fullscreen_rect.right - fullscreen_rect.left, "Expected surface width %u, got %u.\n",
            fullscreen_rect.right - fullscreen_rect.left, ddsd.dwWidth);
    ok(ddsd.dwHeight == fullscreen_rect.bottom - fullscreen_rect.top, "Expected surface height %u, got %u.\n",
            fullscreen_rect.bottom - fullscreen_rect.top, ddsd.dwHeight);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &fullscreen_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            fullscreen_rect.left, fullscreen_rect.top, fullscreen_rect.right, fullscreen_rect.bottom,
            r.left, r.top, r.right, r.bottom);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &fullscreen_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            fullscreen_rect.left, fullscreen_rect.top, fullscreen_rect.right, fullscreen_rect.bottom,
            r.left, r.top, r.right, r.bottom);

    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == fullscreen_rect.right - fullscreen_rect.left, "Expected surface width %u, got %u.\n",
            fullscreen_rect.right - fullscreen_rect.left, ddsd.dwWidth);
    ok(ddsd.dwHeight == fullscreen_rect.bottom - fullscreen_rect.top, "Expected surface height %u, got %u.\n",
            fullscreen_rect.bottom - fullscreen_rect.top, ddsd.dwHeight);
    IDirectDrawSurface4_Release(primary);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == fullscreen_rect.right - fullscreen_rect.left, "Expected surface width %u, got %u.\n",
            fullscreen_rect.right - fullscreen_rect.left, ddsd.dwWidth);
    ok(ddsd.dwHeight == fullscreen_rect.bottom - fullscreen_rect.top, "Expected surface height %u, got %u.\n",
            fullscreen_rect.bottom - fullscreen_rect.top, ddsd.dwHeight);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &fullscreen_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            fullscreen_rect.left, fullscreen_rect.top, fullscreen_rect.right, fullscreen_rect.bottom,
            r.left, r.top, r.right, r.bottom);

    PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE);
    expect_messages = normal_messages;
    screen_size.cx = 0;
    screen_size.cy = 0;

    hr = set_display_mode(ddraw, 640, 480);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);

    ok(!*expect_messages, "Expected message %#x, but didn't receive it.\n", *expect_messages);
    expect_messages = NULL;
    ok(!screen_size.cx && !screen_size.cy, "Got unexpected screen size %ux%u.\n", screen_size.cx, screen_size.cy);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &fullscreen_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            fullscreen_rect.left, fullscreen_rect.top, fullscreen_rect.right, fullscreen_rect.bottom,
            r.left, r.top, r.right, r.bottom);

    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == fullscreen_rect.right - fullscreen_rect.left, "Expected surface width %u, got %u.\n",
            fullscreen_rect.right - fullscreen_rect.left, ddsd.dwWidth);
    ok(ddsd.dwHeight == fullscreen_rect.bottom - fullscreen_rect.top, "Expected surface height %u, got %u.\n",
            fullscreen_rect.bottom - fullscreen_rect.top, ddsd.dwHeight);
    IDirectDrawSurface4_Release(primary);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == s.right - s.left, "Expected surface width %u, got %u.\n",
            s.right - s.left, ddsd.dwWidth);
    ok(ddsd.dwHeight == s.bottom - s.top, "Expected surface height %u, got %u.\n",
            s.bottom - s.top, ddsd.dwHeight);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &fullscreen_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            fullscreen_rect.left, fullscreen_rect.top, fullscreen_rect.right, fullscreen_rect.bottom,
            r.left, r.top, r.right, r.bottom);

    PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE);
    expect_messages = normal_messages;
    screen_size.cx = 0;
    screen_size.cy = 0;

    hr = IDirectDraw_RestoreDisplayMode(ddraw);
    ok(SUCCEEDED(hr), "RestoreDisplayMode failed, hr %#x.\n", hr);

    ok(!*expect_messages, "Expected message %#x, but didn't receive it.\n", *expect_messages);
    expect_messages = NULL;
    ok(!screen_size.cx && !screen_size.cy, "Got unexpected screen size %ux%u.\n", screen_size.cx, screen_size.cy);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &fullscreen_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            fullscreen_rect.left, fullscreen_rect.top, fullscreen_rect.right, fullscreen_rect.bottom,
            r.left, r.top, r.right, r.bottom);

    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == s.right - s.left, "Expected surface width %u, got %u.\n",
            s.right - s.left, ddsd.dwWidth);
    ok(ddsd.dwHeight == s.bottom - s.top, "Expected surface height %u, got %u.\n",
            s.bottom - s.top, ddsd.dwHeight);
    IDirectDrawSurface4_Release(primary);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == fullscreen_rect.right - fullscreen_rect.left, "Expected surface width %u, got %u.\n",
            fullscreen_rect.right - fullscreen_rect.left, ddsd.dwWidth);
    ok(ddsd.dwHeight == fullscreen_rect.bottom - fullscreen_rect.top, "Expected surface height %u, got %u.\n",
            fullscreen_rect.bottom - fullscreen_rect.top, ddsd.dwHeight);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &fullscreen_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            fullscreen_rect.left, fullscreen_rect.top, fullscreen_rect.right, fullscreen_rect.bottom,
            r.left, r.top, r.right, r.bottom);

    /* DDSCL_NORMAL | DDSCL_FULLSCREEN behaves the same as just DDSCL_NORMAL.
     * Resizing the window on mode changes is a property of DDSCL_EXCLUSIVE,
     * not DDSCL_FULLSCREEN. */
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &fullscreen_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            fullscreen_rect.left, fullscreen_rect.top, fullscreen_rect.right, fullscreen_rect.bottom,
            r.left, r.top, r.right, r.bottom);

    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == fullscreen_rect.right - fullscreen_rect.left, "Expected surface width %u, got %u.\n",
            fullscreen_rect.right - fullscreen_rect.left, ddsd.dwWidth);
    ok(ddsd.dwHeight == fullscreen_rect.bottom - fullscreen_rect.top, "Expected surface height %u, got %u.\n",
            fullscreen_rect.bottom - fullscreen_rect.top, ddsd.dwHeight);
    IDirectDrawSurface4_Release(primary);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == fullscreen_rect.right - fullscreen_rect.left, "Expected surface width %u, got %u.\n",
            fullscreen_rect.right - fullscreen_rect.left, ddsd.dwWidth);
    ok(ddsd.dwHeight == fullscreen_rect.bottom - fullscreen_rect.top, "Expected surface height %u, got %u.\n",
            fullscreen_rect.bottom - fullscreen_rect.top, ddsd.dwHeight);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &fullscreen_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            fullscreen_rect.left, fullscreen_rect.top, fullscreen_rect.right, fullscreen_rect.bottom,
            r.left, r.top, r.right, r.bottom);

    PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE);
    expect_messages = normal_messages;
    screen_size.cx = 0;
    screen_size.cy = 0;

    hr = set_display_mode(ddraw, 640, 480);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);

    ok(!*expect_messages, "Expected message %#x, but didn't receive it.\n", *expect_messages);
    expect_messages = NULL;
    ok(!screen_size.cx && !screen_size.cy, "Got unexpected screen size %ux%u.\n", screen_size.cx, screen_size.cy);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &fullscreen_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            fullscreen_rect.left, fullscreen_rect.top, fullscreen_rect.right, fullscreen_rect.bottom,
            r.left, r.top, r.right, r.bottom);

    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == fullscreen_rect.right - fullscreen_rect.left, "Expected surface width %u, got %u.\n",
            fullscreen_rect.right - fullscreen_rect.left, ddsd.dwWidth);
    ok(ddsd.dwHeight == fullscreen_rect.bottom - fullscreen_rect.top, "Expected surface height %u, got %u.\n",
            fullscreen_rect.bottom - fullscreen_rect.top, ddsd.dwHeight);
    IDirectDrawSurface4_Release(primary);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == s.right - s.left, "Expected surface width %u, got %u.\n",
            s.right - s.left, ddsd.dwWidth);
    ok(ddsd.dwHeight == s.bottom - s.top, "Expected surface height %u, got %u.\n",
            s.bottom - s.top, ddsd.dwHeight);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &fullscreen_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            fullscreen_rect.left, fullscreen_rect.top, fullscreen_rect.right, fullscreen_rect.bottom,
            r.left, r.top, r.right, r.bottom);

    PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE);
    expect_messages = normal_messages;
    screen_size.cx = 0;
    screen_size.cy = 0;

    hr = IDirectDraw_RestoreDisplayMode(ddraw);
    ok(SUCCEEDED(hr), "RestoreDisplayMode failed, hr %#x.\n", hr);

    ok(!*expect_messages, "Expected message %#x, but didn't receive it.\n", *expect_messages);
    expect_messages = NULL;
    ok(!screen_size.cx && !screen_size.cy, "Got unexpected screen size %ux%u.\n", screen_size.cx, screen_size.cy);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &fullscreen_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            fullscreen_rect.left, fullscreen_rect.top, fullscreen_rect.right, fullscreen_rect.bottom,
            r.left, r.top, r.right, r.bottom);

    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == s.right - s.left, "Expected surface width %u, got %u.\n",
            s.right - s.left, ddsd.dwWidth);
    ok(ddsd.dwHeight == s.bottom - s.top, "Expected surface height %u, got %u.\n",
            s.bottom - s.top, ddsd.dwHeight);
    IDirectDrawSurface4_Release(primary);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == fullscreen_rect.right - fullscreen_rect.left, "Expected surface width %u, got %u.\n",
            fullscreen_rect.right - fullscreen_rect.left, ddsd.dwWidth);
    ok(ddsd.dwHeight == fullscreen_rect.bottom - fullscreen_rect.top, "Expected surface height %u, got %u.\n",
            fullscreen_rect.bottom - fullscreen_rect.top, ddsd.dwHeight);
    IDirectDrawSurface4_Release(primary);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &fullscreen_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            fullscreen_rect.left, fullscreen_rect.top, fullscreen_rect.right, fullscreen_rect.bottom,
            r.left, r.top, r.right, r.bottom);

    /* Changing the coop level from EXCLUSIVE to NORMAL restores the screen resolution */
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    hr = set_display_mode(ddraw, 640, 480);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);

    PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE);
    expect_messages = exclusive_messages;
    screen_size.cx = 0;
    screen_size.cy = 0;

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    ok(!*expect_messages, "Expected message %#x, but didn't receive it.\n", *expect_messages);
    expect_messages = NULL;
    ok(screen_size.cx == fullscreen_rect.right && screen_size.cy == fullscreen_rect.bottom,
            "Expected screen size %ux%u, got %ux%u.\n",
            fullscreen_rect.right, fullscreen_rect.bottom, screen_size.cx, screen_size.cy);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &fullscreen_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            fullscreen_rect.left, fullscreen_rect.top, fullscreen_rect.right, fullscreen_rect.bottom,
            r.left, r.top, r.right, r.bottom);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == fullscreen_rect.right - fullscreen_rect.left, "Expected surface width %u, got %u.\n",
            fullscreen_rect.right - fullscreen_rect.left, ddsd.dwWidth);
    ok(ddsd.dwHeight == fullscreen_rect.bottom - fullscreen_rect.top, "Expected surface height %u, got %u.\n",
            fullscreen_rect.bottom - fullscreen_rect.top, ddsd.dwHeight);
    IDirectDrawSurface_Release(primary);

    /* The screen restore is a property of DDSCL_EXCLUSIVE  */
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    hr = set_display_mode(ddraw, 640, 480);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == s.right - s.left, "Expected surface width %u, got %u.\n",
            s.right - s.left, ddsd.dwWidth);
    ok(ddsd.dwHeight == s.bottom - s.top, "Expected surface height %u, got %u.\n",
            s.bottom - s.top, ddsd.dwHeight);
    IDirectDrawSurface_Release(primary);

    hr = IDirectDraw_RestoreDisplayMode(ddraw);
    ok(SUCCEEDED(hr), "RestoreDisplayMode failed, hr %#x.\n", hr);

    /* If the window is changed at the same time, messages are sent to the new window. */
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    hr = set_display_mode(ddraw, 640, 480);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);

    PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE);
    expect_messages = exclusive_messages;
    screen_size.cx = 0;
    screen_size.cy = 0;
    screen_size2.cx = 0;
    screen_size2.cy = 0;

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window2, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    ok(!*expect_messages, "Expected message %#x, but didn't receive it.\n", *expect_messages);
    expect_messages = NULL;
    ok(!screen_size.cx && !screen_size.cy, "Got unexpected screen size %ux%u.\n",
            screen_size.cx, screen_size.cy);
    ok(screen_size2.cx == fullscreen_rect.right && screen_size2.cy == fullscreen_rect.bottom,
            "Expected screen size 2 %ux%u, got %ux%u.\n",
            fullscreen_rect.right, fullscreen_rect.bottom, screen_size2.cx, screen_size2.cy);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &s), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            s.left, s.top, s.right, s.bottom,
            r.left, r.top, r.right, r.bottom);
    GetWindowRect(window2, &r);
    ok(EqualRect(&r, &fullscreen_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            fullscreen_rect.left, fullscreen_rect.top, fullscreen_rect.right, fullscreen_rect.bottom,
            r.left, r.top, r.right, r.bottom);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == fullscreen_rect.right - fullscreen_rect.left, "Expected surface width %u, got %u.\n",
            fullscreen_rect.right - fullscreen_rect.left, ddsd.dwWidth);
    ok(ddsd.dwHeight == fullscreen_rect.bottom - fullscreen_rect.top, "Expected surface height %u, got %u.\n",
            fullscreen_rect.bottom - fullscreen_rect.top, ddsd.dwHeight);
    IDirectDrawSurface_Release(primary);

    ref = IDirectDraw4_Release(ddraw);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &s), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            s.left, s.top, s.right, s.bottom,
            r.left, r.top, r.right, r.bottom);

    expect_messages = NULL;
    DestroyWindow(window);
    DestroyWindow(window2);
    UnregisterClassA("ddraw_test_wndproc_wc", GetModuleHandleA(NULL));
    UnregisterClassA("ddraw_test_wndproc_wc2", GetModuleHandleA(NULL));
}

static void test_coop_level_mode_set_multi(void)
{
    IDirectDraw4 *ddraw1, *ddraw2;
    UINT orig_w, orig_h, w, h;
    HWND window;
    HRESULT hr;
    ULONG ref;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 100, 100, 0, 0, 0, 0);
    ddraw1 = create_ddraw();
    ok(!!ddraw1, "Failed to create a ddraw object.\n");

    orig_w = GetSystemMetrics(SM_CXSCREEN);
    orig_h = GetSystemMetrics(SM_CYSCREEN);

    /* With just a single ddraw object, the display mode is restored on
     * release. */
    hr = set_display_mode(ddraw1, 800, 600);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == 800, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == 600, "Got unexpected screen height %u.\n", h);

    ref = IDirectDraw4_Release(ddraw1);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == orig_w, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == orig_h, "Got unexpected screen height %u.\n", h);

    /* When there are multiple ddraw objects, the display mode is restored to
     * the initial mode, before the first SetDisplayMode() call. */
    ddraw1 = create_ddraw();
    hr = set_display_mode(ddraw1, 800, 600);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == 800, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == 600, "Got unexpected screen height %u.\n", h);

    ddraw2 = create_ddraw();
    hr = set_display_mode(ddraw2, 640, 480);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == 640, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == 480, "Got unexpected screen height %u.\n", h);

    ref = IDirectDraw4_Release(ddraw2);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == orig_w, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == orig_h, "Got unexpected screen height %u.\n", h);

    ref = IDirectDraw4_Release(ddraw1);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == orig_w, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == orig_h, "Got unexpected screen height %u.\n", h);

    /* Regardless of release ordering. */
    ddraw1 = create_ddraw();
    hr = set_display_mode(ddraw1, 800, 600);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == 800, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == 600, "Got unexpected screen height %u.\n", h);

    ddraw2 = create_ddraw();
    hr = set_display_mode(ddraw2, 640, 480);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == 640, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == 480, "Got unexpected screen height %u.\n", h);

    ref = IDirectDraw4_Release(ddraw1);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == orig_w, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == orig_h, "Got unexpected screen height %u.\n", h);

    ref = IDirectDraw4_Release(ddraw2);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == orig_w, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == orig_h, "Got unexpected screen height %u.\n", h);

    /* But only for ddraw objects that called SetDisplayMode(). */
    ddraw1 = create_ddraw();
    ddraw2 = create_ddraw();
    hr = set_display_mode(ddraw2, 640, 480);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == 640, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == 480, "Got unexpected screen height %u.\n", h);

    ref = IDirectDraw4_Release(ddraw1);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == 640, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == 480, "Got unexpected screen height %u.\n", h);

    ref = IDirectDraw4_Release(ddraw2);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == orig_w, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == orig_h, "Got unexpected screen height %u.\n", h);

    /* If there's a ddraw object that's currently in exclusive mode, it blocks
     * restoring the display mode. */
    ddraw1 = create_ddraw();
    hr = set_display_mode(ddraw1, 800, 600);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == 800, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == 600, "Got unexpected screen height %u.\n", h);

    ddraw2 = create_ddraw();
    hr = set_display_mode(ddraw2, 640, 480);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == 640, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == 480, "Got unexpected screen height %u.\n", h);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw2, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    ref = IDirectDraw4_Release(ddraw1);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == 640, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == 480, "Got unexpected screen height %u.\n", h);

    ref = IDirectDraw4_Release(ddraw2);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == orig_w, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == orig_h, "Got unexpected screen height %u.\n", h);

    /* Exclusive mode blocks mode setting on other ddraw objects in general. */
    ddraw1 = create_ddraw();
    hr = set_display_mode(ddraw1, 800, 600);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == 800, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == 600, "Got unexpected screen height %u.\n", h);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw1, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    ddraw2 = create_ddraw();
    hr = set_display_mode(ddraw2, 640, 480);
    ok(hr == DDERR_NOEXCLUSIVEMODE, "Got unexpected hr %#x.\n", hr);

    ref = IDirectDraw4_Release(ddraw1);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == orig_w, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == orig_h, "Got unexpected screen height %u.\n", h);

    ref = IDirectDraw4_Release(ddraw2);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == orig_w, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == orig_h, "Got unexpected screen height %u.\n", h);

    DestroyWindow(window);
}

static void test_initialize(void)
{
    IDirectDraw4 *ddraw;
    HRESULT hr;

    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    hr = IDirectDraw4_Initialize(ddraw, NULL);
    ok(hr == DDERR_ALREADYINITIALIZED, "Initialize returned hr %#x.\n", hr);
    IDirectDraw4_Release(ddraw);

    CoInitialize(NULL);
    hr = CoCreateInstance(&CLSID_DirectDraw, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectDraw4, (void **)&ddraw);
    ok(SUCCEEDED(hr), "Failed to create IDirectDraw4 instance, hr %#x.\n", hr);
    hr = IDirectDraw4_Initialize(ddraw, NULL);
    ok(hr == DD_OK, "Initialize returned hr %#x, expected DD_OK.\n", hr);
    hr = IDirectDraw4_Initialize(ddraw, NULL);
    ok(hr == DDERR_ALREADYINITIALIZED, "Initialize returned hr %#x, expected DDERR_ALREADYINITIALIZED.\n", hr);
    IDirectDraw4_Release(ddraw);
    CoUninitialize();
}

static void test_coop_level_surf_create(void)
{
    IDirectDrawSurface4 *surface;
    IDirectDraw4 *ddraw;
    DDSURFACEDESC2 ddsd;
    HRESULT hr;

    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &surface, NULL);
    ok(hr == DDERR_NOCOOPERATIVELEVELSET, "Surface creation returned hr %#x.\n", hr);

    IDirectDraw4_Release(ddraw);
}

static void test_vb_discard(void)
{
    static const struct vec4 quad[] =
    {
        {  0.0f, 480.0f, 0.0f, 1.0f},
        {  0.0f,   0.0f, 0.0f, 1.0f},
        {640.0f, 480.0f, 0.0f, 1.0f},
        {640.0f,   0.0f, 0.0f, 1.0f},
    };

    IDirect3DDevice3 *device;
    IDirect3D3 *d3d;
    IDirect3DVertexBuffer *buffer;
    HWND window;
    HRESULT hr;
    D3DVERTEXBUFFERDESC desc;
    BYTE *data;
    static const unsigned int vbsize = 16;
    unsigned int i;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);

    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwCaps = D3DVBCAPS_WRITEONLY;
    desc.dwFVF = D3DFVF_XYZRHW;
    desc.dwNumVertices = vbsize;
    hr = IDirect3D3_CreateVertexBuffer(d3d, &desc, &buffer, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to create vertex buffer, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer_Lock(buffer, DDLOCK_DISCARDCONTENTS, (void **)&data, NULL);
    ok(SUCCEEDED(hr), "Failed to lock vertex buffer, hr %#x.\n", hr);
    memcpy(data, quad, sizeof(quad));
    hr = IDirect3DVertexBuffer_Unlock(buffer);
    ok(SUCCEEDED(hr), "Failed to unlock vertex buffer, hr %#x.\n", hr);

    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitiveVB(device, D3DPT_TRIANGLESTRIP, buffer, 0, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer_Lock(buffer, DDLOCK_DISCARDCONTENTS, (void **)&data, NULL);
    ok(SUCCEEDED(hr), "Failed to lock vertex buffer, hr %#x.\n", hr);
    memset(data, 0xaa, sizeof(struct vec4) * vbsize);
    hr = IDirect3DVertexBuffer_Unlock(buffer);
    ok(SUCCEEDED(hr), "Failed to unlock vertex buffer, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer_Lock(buffer, DDLOCK_DISCARDCONTENTS, (void **)&data, NULL);
    ok(SUCCEEDED(hr), "Failed to lock vertex buffer, hr %#x.\n", hr);
    for (i = 0; i < sizeof(struct vec4) * vbsize; i++)
    {
        if (data[i] != 0xaa)
        {
            ok(FALSE, "Vertex buffer data byte %u is 0x%02x, expected 0xaa\n", i, data[i]);
            break;
        }
    }
    hr = IDirect3DVertexBuffer_Unlock(buffer);
    ok(SUCCEEDED(hr), "Failed to unlock vertex buffer, hr %#x.\n", hr);

    IDirect3DVertexBuffer_Release(buffer);
    IDirect3D3_Release(d3d);
    IDirect3DDevice3_Release(device);
    DestroyWindow(window);
}

static void test_coop_level_multi_window(void)
{
    HWND window1, window2;
    IDirectDraw4 *ddraw;
    HRESULT hr;

    window1 = CreateWindowA("static", "ddraw_test1", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    window2 = CreateWindowA("static", "ddraw_test2", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window1, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window2, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    ok(IsWindow(window1), "Window 1 was destroyed.\n");
    ok(IsWindow(window2), "Window 2 was destroyed.\n");

    IDirectDraw4_Release(ddraw);
    DestroyWindow(window2);
    DestroyWindow(window1);
}

static void test_draw_strided(void)
{
    static struct vec3 position[] =
    {
        {-1.0,   -1.0,   0.0},
        {-1.0,    1.0,   0.0},
        { 1.0,    1.0,   0.0},
        { 1.0,   -1.0,   0.0},
    };
    static DWORD diffuse[] =
    {
        0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00,
    };
    static WORD indices[] =
    {
        0, 1, 2, 2, 3, 0
    };

    IDirectDrawSurface4 *rt;
    IDirect3DDevice3 *device;
    D3DCOLOR color;
    HWND window;
    HRESULT hr;
    D3DDRAWPRIMITIVESTRIDEDDATA strided;
    IDirect3DViewport3 *viewport;
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);

    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);
    viewport = create_viewport(device, 0, 0, 640, 480);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to activate the viewport, hr %#x.\n", hr);
    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0x00000000, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear the viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);

    memset(&strided, 0x55, sizeof(strided));
    strided.position.lpvData = position;
    strided.position.dwStride = sizeof(*position);
    strided.diffuse.lpvData = diffuse;
    strided.diffuse.dwStride = sizeof(*diffuse);
    hr = IDirect3DDevice3_DrawIndexedPrimitiveStrided(device, D3DPT_TRIANGLELIST, D3DFVF_XYZ | D3DFVF_DIFFUSE,
            &strided, 4, indices, 6, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x0000ff00, 1), "Got unexpected color 0x%08x.\n", color);

    IDirect3DViewport3_Release(viewport);
    IDirectDrawSurface4_Release(rt);
    IDirect3DDevice3_Release(device);
    DestroyWindow(window);
}

static void test_clear_rect_count(void)
{
    IDirectDrawSurface4 *rt;
    IDirect3DDevice3 *device;
    D3DCOLOR color;
    HWND window;
    HRESULT hr;
    IDirect3DViewport3 *viewport;
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    viewport = create_viewport(device, 0, 0, 640, 480);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to activate the viewport, hr %#x.\n", hr);
    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0x00ffffff, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear the viewport, hr %#x.\n", hr);
    hr = IDirect3DViewport3_Clear2(viewport, 0, &clear_rect, D3DCLEAR_TARGET, 0x00ff0000, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear the viewport, hr %#x.\n", hr);
    hr = IDirect3DViewport3_Clear2(viewport, 0, NULL, D3DCLEAR_TARGET, 0x0000ff00, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear the viewport, hr %#x.\n", hr);
    hr = IDirect3DViewport3_Clear2(viewport, 1, NULL, D3DCLEAR_TARGET, 0x000000ff, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear the viewport, hr %#x.\n", hr);

    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x00ffffff, 1), "Got unexpected color 0x%08x.\n", color);

    IDirect3DViewport3_Release(viewport);
    IDirectDrawSurface4_Release(rt);
    IDirect3DDevice3_Release(device);
    DestroyWindow(window);
}

static BOOL test_mode_restored(IDirectDraw4 *ddraw, HWND window)
{
    DDSURFACEDESC2 ddsd1, ddsd2;
    HRESULT hr;

    memset(&ddsd1, 0, sizeof(ddsd1));
    ddsd1.dwSize = sizeof(ddsd1);
    hr = IDirectDraw4_GetDisplayMode(ddraw, &ddsd1);
    ok(SUCCEEDED(hr), "GetDisplayMode failed, hr %#x.\n", hr);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    hr = set_display_mode(ddraw, 640, 480);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);
    hr = IDirectDraw4_GetDisplayMode(ddraw, &ddsd2);
    ok(SUCCEEDED(hr), "GetDisplayMode failed, hr %#x.\n", hr);
    hr = IDirectDraw4_RestoreDisplayMode(ddraw);
    ok(SUCCEEDED(hr), "RestoreDisplayMode failed, hr %#x.\n", hr);

    return ddsd1.dwWidth == ddsd2.dwWidth && ddsd1.dwHeight == ddsd2.dwHeight;
}

static void test_coop_level_versions(void)
{
    HWND window;
    IDirectDraw *ddraw;
    HRESULT hr;
    BOOL restored;
    IDirectDrawSurface *surface;
    IDirectDraw4 *ddraw4;
    DDSURFACEDESC ddsd;

    window = CreateWindowA("static", "ddraw_test1", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);

    ddraw4 = create_ddraw();
    ok(!!ddraw4, "Failed to create a ddraw object.\n");
    /* Newly created ddraw objects restore the mode on ddraw2+::SetCooperativeLevel(NORMAL) */
    restored = test_mode_restored(ddraw4, window);
    ok(restored, "Display mode not restored in new ddraw object\n");

    /* A failing ddraw1::SetCooperativeLevel call does not have an effect */
    hr = IDirectDraw4_QueryInterface(ddraw4, &IID_IDirectDraw, (void **)&ddraw);
    ok(SUCCEEDED(hr), "QueryInterface failed, hr %#x.\n", hr);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE);
    ok(FAILED(hr), "SetCooperativeLevel returned %#x, expected failure.\n", hr);
    restored = test_mode_restored(ddraw4, window);
    ok(restored, "Display mode not restored after bad ddraw1::SetCooperativeLevel call\n");

    /* A successful one does */
    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    restored = test_mode_restored(ddraw4, window);
    ok(!restored, "Display mode restored after good ddraw1::SetCooperativeLevel call\n");

    IDirectDraw_Release(ddraw);
    IDirectDraw4_Release(ddraw4);

    ddraw4 = create_ddraw();
    ok(!!ddraw4, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_QueryInterface(ddraw4, &IID_IDirectDraw, (void **)&ddraw);
    ok(SUCCEEDED(hr), "QueryInterface failed, hr %#x.\n", hr);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_SETFOCUSWINDOW);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    restored = test_mode_restored(ddraw4, window);
    ok(!restored, "Display mode restored after ddraw1::SetCooperativeLevel(SETFOCUSWINDOW) call\n");

    IDirectDraw_Release(ddraw);
    IDirectDraw4_Release(ddraw4);

    /* A failing call does not restore the ddraw2+ behavior */
    ddraw4 = create_ddraw();
    ok(!!ddraw4, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_QueryInterface(ddraw4, &IID_IDirectDraw, (void **)&ddraw);
    ok(SUCCEEDED(hr), "QueryInterface failed, hr %#x.\n", hr);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE);
    ok(FAILED(hr), "SetCooperativeLevel returned %#x, expected failure.\n", hr);
    restored = test_mode_restored(ddraw4, window);
    ok(!restored, "Display mode restored after good-bad ddraw1::SetCooperativeLevel() call sequence\n");

    IDirectDraw_Release(ddraw);
    IDirectDraw4_Release(ddraw4);

    /* Neither does a sequence of successful calls with the new interface */
    ddraw4 = create_ddraw();
    ok(!!ddraw4, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_QueryInterface(ddraw4, &IID_IDirectDraw, (void **)&ddraw);
    ok(SUCCEEDED(hr), "QueryInterface failed, hr %#x.\n", hr);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw4, window, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw4, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    restored = test_mode_restored(ddraw4, window);
    ok(!restored, "Display mode restored after ddraw1-ddraw4 SetCooperativeLevel() call sequence\n");
    IDirectDraw_Release(ddraw);
    IDirectDraw4_Release(ddraw4);

    /* ddraw1::CreateSurface does not triger the ddraw1 behavior */
    ddraw4 = create_ddraw();
    ok(!!ddraw4, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_QueryInterface(ddraw4, &IID_IDirectDraw, (void **)&ddraw);
    ok(SUCCEEDED(hr), "QueryInterface failed, hr %#x.\n", hr);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw4, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = ddsd.dwHeight = 8;
    hr = IDirectDraw_CreateSurface(ddraw, &ddsd, &surface, NULL);
    ok(SUCCEEDED(hr), "CreateSurface failed, hr %#x.\n", hr);
    IDirectDrawSurface_Release(surface);
    restored = test_mode_restored(ddraw4, window);
    ok(restored, "Display mode not restored after ddraw1::CreateSurface() call\n");

    IDirectDraw_Release(ddraw);
    IDirectDraw4_Release(ddraw4);
    DestroyWindow(window);
}

static void test_lighting_interface_versions(void)
{
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    IDirect3DMaterial3 *emissive;
    IDirect3DViewport3 *viewport;
    IDirect3DDevice3 *device;
    IDirectDrawSurface4 *rt;
    D3DCOLOR color;
    HWND window;
    HRESULT hr;
    D3DMATERIALHANDLE mat_handle;
    DWORD rs;
    unsigned int i;
    ULONG ref;
    static D3DVERTEX quad[] =
    {
        {{-1.0f}, { 1.0f}, {0.0f}, {1.0f}, {0.0f}, {0.0f}},
        {{ 1.0f}, { 1.0f}, {0.0f}, {1.0f}, {0.0f}, {0.0f}},
        {{-1.0f}, {-1.0f}, {0.0f}, {1.0f}, {0.0f}, {0.0f}},
        {{ 1.0f}, {-1.0f}, {0.0f}, {1.0f}, {0.0f}, {0.0f}},
    };

#define FVF_COLORVERTEX (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_SPECULAR)
    static struct
    {
        struct vec3 position;
        struct vec3 normal;
        DWORD diffuse, specular;
    }
    quad2[] =
    {
        {{-1.0f,  1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0xffff0000, 0xff808080},
        {{ 1.0f,  1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0xffff0000, 0xff808080},
        {{-1.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0xffff0000, 0xff808080},
        {{ 1.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0xffff0000, 0xff808080},
    };

    static D3DLVERTEX lquad[] =
    {
        {{-1.0f}, { 1.0f}, {0.0f}, 0, {0xffff0000}, {0xff808080}},
        {{ 1.0f}, { 1.0f}, {0.0f}, 0, {0xffff0000}, {0xff808080}},
        {{-1.0f}, {-1.0f}, {0.0f}, 0, {0xffff0000}, {0xff808080}},
        {{ 1.0f}, {-1.0f}, {0.0f}, 0, {0xffff0000}, {0xff808080}},
    };

#define FVF_LVERTEX2 (D3DFVF_LVERTEX & ~D3DFVF_RESERVED1)
    static struct
    {
        struct vec3 position;
        DWORD diffuse, specular;
        struct vec2 texcoord;
    }
    lquad2[] =
    {
        {{-1.0f,  1.0f, 0.0f}, 0xffff0000, 0xff808080},
        {{ 1.0f,  1.0f, 0.0f}, 0xffff0000, 0xff808080},
        {{-1.0f, -1.0f, 0.0f}, 0xffff0000, 0xff808080},
        {{ 1.0f, -1.0f, 0.0f}, 0xffff0000, 0xff808080},
    };

    static D3DTLVERTEX tlquad[] =
    {
        {{   0.0f}, { 480.0f}, {0.0f}, {1.0f}, {0xff0000ff}, {0xff808080}},
        {{   0.0f}, {   0.0f}, {0.0f}, {1.0f}, {0xff0000ff}, {0xff808080}},
        {{ 640.0f}, { 480.0f}, {0.0f}, {1.0f}, {0xff0000ff}, {0xff808080}},
        {{ 640.0f}, {   0.0f}, {0.0f}, {1.0f}, {0xff0000ff}, {0xff808080}},
    };

    static const struct
    {
        DWORD vertextype;
        void *data;
        DWORD d3drs_lighting, d3drs_specular;
        DWORD draw_flags;
        D3DCOLOR color;
    }
    tests[] =
    {
        /* Lighting is enabled when all of these conditions are met:
         * 1) No pretransformed position(D3DFVF_XYZRHW)
         * 2) Normals are available (D3DFVF_NORMAL)
         * 3) D3DDP_DONOTLIGHT is not set.
         *
         * D3DRENDERSTATE_LIGHTING is ignored, it is not defined
         * in this d3d version */

        /* 0 */
        { D3DFVF_VERTEX,    quad,       FALSE,  FALSE,  0,                  0x0000ff00},
        { D3DFVF_VERTEX,    quad,       TRUE,   FALSE,  0,                  0x0000ff00},
        { D3DFVF_VERTEX,    quad,       FALSE,  FALSE,  D3DDP_DONOTLIGHT,   0x00ffffff},
        { D3DFVF_VERTEX,    quad,       TRUE,   FALSE,  D3DDP_DONOTLIGHT,   0x00ffffff},
        { D3DFVF_VERTEX,    quad,       FALSE,  TRUE,   0,                  0x0000ff00},
        { D3DFVF_VERTEX,    quad,       TRUE,   TRUE,   0,                  0x0000ff00},
        { D3DFVF_VERTEX,    quad,       FALSE,  TRUE,   D3DDP_DONOTLIGHT,   0x00ffffff},
        { D3DFVF_VERTEX,    quad,       TRUE,   TRUE,   D3DDP_DONOTLIGHT,   0x00ffffff},

        /* 8 */
        { FVF_COLORVERTEX,  quad2,      FALSE,  FALSE,  0,                  0x0000ff00},
        { FVF_COLORVERTEX,  quad2,      TRUE,   FALSE,  0,                  0x0000ff00},
        { FVF_COLORVERTEX,  quad2,      FALSE,  FALSE,  D3DDP_DONOTLIGHT,   0x00ff0000},
        { FVF_COLORVERTEX,  quad2,      TRUE,   FALSE,  D3DDP_DONOTLIGHT,   0x00ff0000},
        /* The specular color in the vertex is ignored because
         * D3DRENDERSTATE_COLORVERTEX is not enabled */
        { FVF_COLORVERTEX,  quad2,      FALSE,  TRUE,   0,                  0x0000ff00},
        { FVF_COLORVERTEX,  quad2,      TRUE,   TRUE,   0,                  0x0000ff00},
        { FVF_COLORVERTEX,  quad2,      FALSE,  TRUE,   D3DDP_DONOTLIGHT,   0x00ff8080},
        { FVF_COLORVERTEX,  quad2,      TRUE,   TRUE,   D3DDP_DONOTLIGHT,   0x00ff8080},

        /* 16 */
        { D3DFVF_LVERTEX,   lquad,      FALSE,  FALSE,  0,                  0x00ff0000},
        { D3DFVF_LVERTEX,   lquad,      TRUE,   FALSE,  0,                  0x00ff0000},
        { D3DFVF_LVERTEX,   lquad,      FALSE,  FALSE,  D3DDP_DONOTLIGHT,   0x00ff0000},
        { D3DFVF_LVERTEX,   lquad,      TRUE,   FALSE,  D3DDP_DONOTLIGHT,   0x00ff0000},
        { D3DFVF_LVERTEX,   lquad,      FALSE,  TRUE,   0,                  0x00ff8080},
        { D3DFVF_LVERTEX,   lquad,      TRUE,   TRUE,   0,                  0x00ff8080},
        { D3DFVF_LVERTEX,   lquad,      FALSE,  TRUE,   D3DDP_DONOTLIGHT,   0x00ff8080},
        { D3DFVF_LVERTEX,   lquad,      TRUE,   TRUE,   D3DDP_DONOTLIGHT,   0x00ff8080},

        /* 24 */
        { FVF_LVERTEX2,     lquad2,     FALSE,  FALSE,  0,                  0x00ff0000},
        { FVF_LVERTEX2,     lquad2,     TRUE,   FALSE,  0,                  0x00ff0000},
        { FVF_LVERTEX2,     lquad2,     FALSE,  FALSE,  D3DDP_DONOTLIGHT,   0x00ff0000},
        { FVF_LVERTEX2,     lquad2,     TRUE,   FALSE,  D3DDP_DONOTLIGHT,   0x00ff0000},
        { FVF_LVERTEX2,     lquad2,     FALSE,  TRUE,   0,                  0x00ff8080},
        { FVF_LVERTEX2,     lquad2,     TRUE,   TRUE,   0,                  0x00ff8080},
        { FVF_LVERTEX2,     lquad2,     FALSE,  TRUE,   D3DDP_DONOTLIGHT,   0x00ff8080},
        { FVF_LVERTEX2,     lquad2,     TRUE,   TRUE,   D3DDP_DONOTLIGHT,   0x00ff8080},

        /* 32 */
        { D3DFVF_TLVERTEX,  tlquad,     FALSE,  FALSE,  0,                  0x000000ff},
        { D3DFVF_TLVERTEX,  tlquad,     TRUE,   FALSE,  0,                  0x000000ff},
        { D3DFVF_TLVERTEX,  tlquad,     FALSE,  FALSE,  D3DDP_DONOTLIGHT,   0x000000ff},
        { D3DFVF_TLVERTEX,  tlquad,     TRUE,   FALSE,  D3DDP_DONOTLIGHT,   0x000000ff},
        { D3DFVF_TLVERTEX,  tlquad,     FALSE,  TRUE,   0,                  0x008080ff},
        { D3DFVF_TLVERTEX,  tlquad,     TRUE,   TRUE,   0,                  0x008080ff},
        { D3DFVF_TLVERTEX,  tlquad,     FALSE,  TRUE,   D3DDP_DONOTLIGHT,   0x008080ff},
        { D3DFVF_TLVERTEX,  tlquad,     TRUE,   TRUE,   D3DDP_DONOTLIGHT,   0x008080ff},
    };

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);

    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    viewport = create_viewport(device, 0, 0, 640, 480);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);

    emissive = create_emissive_material(device, 0.0f, 1.0f, 0.0f, 0.0f);
    hr = IDirect3DMaterial3_GetHandle(emissive, device, &mat_handle);
    ok(SUCCEEDED(hr), "Failed to get material handle, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetLightState(device, D3DLIGHTSTATE_MATERIAL, mat_handle);
    ok(SUCCEEDED(hr), "Failed to set material state, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_ZENABLE, D3DZB_FALSE);
    ok(SUCCEEDED(hr), "Failed to disable z test, hr %#x.\n", hr);

    hr = IDirect3DDevice3_GetRenderState(device, D3DRENDERSTATE_SPECULARENABLE, &rs);
    ok(SUCCEEDED(hr), "Failed to get specularenable render state, hr %#x.\n", hr);
    ok(rs == FALSE, "Initial D3DRENDERSTATE_SPECULARENABLE is %#x, expected FALSE.\n", rs);

    for (i = 0; i < sizeof(tests) / sizeof(*tests); i++)
    {
        hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0xff202020, 0.0f, 0);
        ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);

        hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_LIGHTING, tests[i].d3drs_lighting);
        ok(SUCCEEDED(hr), "Failed to set lighting render state, hr %#x.\n", hr);
        hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_SPECULARENABLE,
                tests[i].d3drs_specular);
        ok(SUCCEEDED(hr), "Failed to set specularenable render state, hr %#x.\n", hr);

        hr = IDirect3DDevice3_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
        hr = IDirect3DDevice2_DrawPrimitive(device, D3DPT_TRIANGLESTRIP,
                tests[i].vertextype, tests[i].data, 4, tests[i].draw_flags | D3DDP_WAIT);
        hr = IDirect3DDevice3_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

        color = get_surface_color(rt, 320, 240);
        ok(compare_color(color, tests[i].color, 1),
                "Got unexpected color 0x%08x, expected 0x%08x, test %u.\n",
                color, tests[i].color, i);
    }

    destroy_material(emissive);
    IDirectDrawSurface4_Release(rt);
    ref = IDirect3DDevice3_Release(device);
    ok(ref == 0, "Device not properly released, refcount %u.\n", ref);
    DestroyWindow(window);
}

static struct
{
    BOOL received;
    IDirectDraw4 *ddraw;
    HWND window;
    DWORD coop_level;
} activateapp_testdata;

static LRESULT CALLBACK activateapp_test_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (message == WM_ACTIVATEAPP)
    {
        if (activateapp_testdata.ddraw)
        {
            HRESULT hr;
            activateapp_testdata.received = FALSE;
            hr = IDirectDraw4_SetCooperativeLevel(activateapp_testdata.ddraw,
                    activateapp_testdata.window, activateapp_testdata.coop_level);
            ok(SUCCEEDED(hr), "Recursive SetCooperativeLevel call failed, hr %#x.\n", hr);
            ok(!activateapp_testdata.received, "Received WM_ACTIVATEAPP during recursive SetCooperativeLevel call.\n");
        }
        activateapp_testdata.received = TRUE;
    }

    return DefWindowProcA(hwnd, message, wparam, lparam);
}

static void test_coop_level_activateapp(void)
{
    IDirectDraw4 *ddraw;
    HRESULT hr;
    HWND window;
    WNDCLASSA wc = {0};
    DDSURFACEDESC2 ddsd;
    IDirectDrawSurface4 *surface;

    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    wc.lpfnWndProc = activateapp_test_proc;
    wc.lpszClassName = "ddraw_test_wndproc_wc";
    ok(RegisterClassA(&wc), "Failed to register window class.\n");

    window = CreateWindowA("ddraw_test_wndproc_wc", "ddraw_test",
            WS_MAXIMIZE | WS_CAPTION , 0, 0, 640, 480, 0, 0, 0, 0);

    /* Exclusive with window already active. */
    SetActiveWindow(window);
    activateapp_testdata.received = FALSE;
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    ok(!activateapp_testdata.received, "Received WM_ACTIVATEAPP although window was already active.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    /* Exclusive with window not active. */
    SetActiveWindow(NULL);
    activateapp_testdata.received = FALSE;
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    ok(activateapp_testdata.received, "Expected WM_ACTIVATEAPP, but did not receive it.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    /* Normal with window not active, then exclusive with the same window. */
    SetActiveWindow(NULL);
    activateapp_testdata.received = FALSE;
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    ok(!activateapp_testdata.received, "Received WM_ACTIVATEAPP when setting DDSCL_NORMAL.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    /* Except in the first SetCooperativeLevel call, Windows XP randomly does not send
     * WM_ACTIVATEAPP. Windows 7 sends the message reliably. Mark the XP behavior broken. */
    ok(activateapp_testdata.received || broken(!activateapp_testdata.received),
            "Expected WM_ACTIVATEAPP, but did not receive it.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    /* Recursive set of DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN. */
    SetActiveWindow(NULL);
    activateapp_testdata.received = FALSE;
    activateapp_testdata.ddraw = ddraw;
    activateapp_testdata.window = window;
    activateapp_testdata.coop_level = DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN;
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    ok(activateapp_testdata.received || broken(!activateapp_testdata.received),
            "Expected WM_ACTIVATEAPP, but did not receive it.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    /* The recursive call seems to have some bad effect on native ddraw, despite (apparently)
     * succeeding. Another switch to exclusive and back to normal is needed to release the
     * window properly. Without doing this, SetCooperativeLevel(EXCLUSIVE) will not send
     * WM_ACTIVATEAPP messages. */
    activateapp_testdata.ddraw = NULL;
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    /* Setting DDSCL_NORMAL with recursive invocation. */
    SetActiveWindow(NULL);
    activateapp_testdata.received = FALSE;
    activateapp_testdata.ddraw = ddraw;
    activateapp_testdata.window = window;
    activateapp_testdata.coop_level = DDSCL_NORMAL;
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    ok(activateapp_testdata.received || broken(!activateapp_testdata.received),
            "Expected WM_ACTIVATEAPP, but did not receive it.\n");

    /* DDraw is in exlusive mode now. */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    ddsd.dwBackBufferCount = 1;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;
    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    IDirectDrawSurface4_Release(surface);

    /* Recover again, just to be sure. */
    activateapp_testdata.ddraw = NULL;
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    DestroyWindow(window);
    UnregisterClassA("ddraw_test_wndproc_wc", GetModuleHandleA(NULL));
    IDirectDraw4_Release(ddraw);
}

static void test_texturemanage(void)
{
    IDirectDraw4 *ddraw;
    HRESULT hr;
    DDSURFACEDESC2 ddsd;
    IDirectDrawSurface4 *surface;
    unsigned int i;
    DDCAPS hal_caps, hel_caps;
    DWORD needed_caps = DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY;
    static const struct
    {
        DWORD caps_in, caps2_in;
        HRESULT hr;
        DWORD caps_out, caps2_out;
    }
    tests[] =
    {
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE, DDSCAPS2_TEXTUREMANAGE, DDERR_INVALIDCAPS,
                ~0U, ~0U},
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE, DDSCAPS2_D3DTEXTUREMANAGE, DDERR_INVALIDCAPS,
                ~0U, ~0U},
        {DDSCAPS_VIDEOMEMORY | DDSCAPS_TEXTURE, DDSCAPS2_TEXTUREMANAGE, DDERR_INVALIDCAPS,
                ~0U, ~0U},
        {DDSCAPS_VIDEOMEMORY | DDSCAPS_TEXTURE, DDSCAPS2_D3DTEXTUREMANAGE, DDERR_INVALIDCAPS,
                ~0U, ~0U},
        {DDSCAPS_TEXTURE, DDSCAPS2_TEXTUREMANAGE, DD_OK,
                DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE, DDSCAPS2_TEXTUREMANAGE},
        {DDSCAPS_TEXTURE, DDSCAPS2_D3DTEXTUREMANAGE, DD_OK,
                DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE, DDSCAPS2_D3DTEXTUREMANAGE},
        {DDSCAPS_VIDEOMEMORY | DDSCAPS_TEXTURE, 0, DD_OK,
                DDSCAPS_VIDEOMEMORY | DDSCAPS_TEXTURE | DDSCAPS_LOCALVIDMEM, 0},
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE, 0, DD_OK,
                DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE, 0},

        {0, DDSCAPS2_TEXTUREMANAGE, DDERR_INVALIDCAPS,
                ~0U, ~0U},
        {0, DDSCAPS2_D3DTEXTUREMANAGE, DDERR_INVALIDCAPS,
                ~0U, ~0U},
        {DDSCAPS_SYSTEMMEMORY, DDSCAPS2_TEXTUREMANAGE, DDERR_INVALIDCAPS,
                ~0U, ~0U},
        {DDSCAPS_SYSTEMMEMORY, DDSCAPS2_D3DTEXTUREMANAGE, DDERR_INVALIDCAPS,
                ~0U, ~0U},
        {DDSCAPS_VIDEOMEMORY, DDSCAPS2_TEXTUREMANAGE, DDERR_INVALIDCAPS,
                ~0U, ~0U},
        {DDSCAPS_VIDEOMEMORY, DDSCAPS2_D3DTEXTUREMANAGE, DDERR_INVALIDCAPS,
                ~0U, ~0U},
        {DDSCAPS_VIDEOMEMORY, 0, DD_OK,
                DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY, 0},
        {DDSCAPS_SYSTEMMEMORY, 0, DD_OK,
                DDSCAPS_SYSTEMMEMORY, 0},
    };

    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&hal_caps, 0, sizeof(hal_caps));
    hal_caps.dwSize = sizeof(hal_caps);
    memset(&hel_caps, 0, sizeof(hel_caps));
    hel_caps.dwSize = sizeof(hel_caps);
    hr = IDirectDraw4_GetCaps(ddraw, &hal_caps, &hel_caps);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    if ((hal_caps.ddsCaps.dwCaps & needed_caps) != needed_caps)
    {
        skip("Managed textures not supported, skipping managed texture test.\n");
        IDirectDraw4_Release(ddraw);
        return;
    }

    for (i = 0; i < sizeof(tests) / sizeof(*tests); i++)
    {
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
        ddsd.ddsCaps.dwCaps = tests[i].caps_in;
        ddsd.ddsCaps.dwCaps2 = tests[i].caps2_in;
        ddsd.dwWidth = 4;
        ddsd.dwHeight = 4;

        hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &surface, NULL);
        ok(hr == tests[i].hr, "Got unexpected, hr %#x, case %u.\n", hr, i);
        if (FAILED(hr))
            continue;

        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        hr = IDirectDrawSurface4_GetSurfaceDesc(surface, &ddsd);
        ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);

        ok(ddsd.ddsCaps.dwCaps == tests[i].caps_out,
                "Input caps %#x, %#x, expected output caps %#x, got %#x, case %u.\n",
                tests[i].caps_in, tests[i].caps2_in, tests[i].caps_out, ddsd.ddsCaps.dwCaps, i);
        ok(ddsd.ddsCaps.dwCaps2 == tests[i].caps2_out,
                "Input caps %#x, %#x, expected output caps %#x, got %#x, case %u.\n",
                tests[i].caps_in, tests[i].caps2_in, tests[i].caps2_out, ddsd.ddsCaps.dwCaps2, i);

        IDirectDrawSurface4_Release(surface);
    }

    IDirectDraw4_Release(ddraw);
}

#define SUPPORT_DXT1    0x01
#define SUPPORT_DXT2    0x02
#define SUPPORT_DXT3    0x04
#define SUPPORT_DXT4    0x08
#define SUPPORT_DXT5    0x10
#define SUPPORT_YUY2    0x20
#define SUPPORT_UYVY    0x40

static HRESULT WINAPI test_block_formats_creation_cb(DDPIXELFORMAT *fmt, void *ctx)
{
    DWORD *supported_fmts = ctx;

    if (!(fmt->dwFlags & DDPF_FOURCC))
        return DDENUMRET_OK;

    switch (fmt->dwFourCC)
    {
        case MAKEFOURCC('D','X','T','1'):
            *supported_fmts |= SUPPORT_DXT1;
            break;
        case MAKEFOURCC('D','X','T','2'):
            *supported_fmts |= SUPPORT_DXT2;
            break;
        case MAKEFOURCC('D','X','T','3'):
            *supported_fmts |= SUPPORT_DXT3;
            break;
        case MAKEFOURCC('D','X','T','4'):
            *supported_fmts |= SUPPORT_DXT4;
            break;
        case MAKEFOURCC('D','X','T','5'):
            *supported_fmts |= SUPPORT_DXT5;
            break;
        case MAKEFOURCC('Y','U','Y','2'):
            *supported_fmts |= SUPPORT_YUY2;
            break;
        case MAKEFOURCC('U','Y','V','Y'):
            *supported_fmts |= SUPPORT_UYVY;
            break;
        default:
            break;
    }

    return DDENUMRET_OK;
}

static void test_block_formats_creation(void)
{
    HRESULT hr, expect_hr;
    unsigned int i, j, w, h;
    HWND window;
    IDirectDraw4 *ddraw;
    IDirect3D3 *d3d;
    IDirect3DDevice3 *device;
    IDirectDrawSurface4 *surface;
    DWORD supported_fmts = 0, supported_overlay_fmts = 0;
    DWORD num_fourcc_codes = 0, *fourcc_codes;
    DDSURFACEDESC2 ddsd;
    DDCAPS hal_caps;

    static const struct
    {
        DWORD fourcc;
        const char *name;
        DWORD support_flag;
        unsigned int block_width;
        unsigned int block_height;
        BOOL create_size_checked, overlay;
    }
    formats[] =
    {
        {MAKEFOURCC('D','X','T','1'), "D3DFMT_DXT1", SUPPORT_DXT1, 4, 4, TRUE,  FALSE},
        {MAKEFOURCC('D','X','T','2'), "D3DFMT_DXT2", SUPPORT_DXT2, 4, 4, TRUE,  FALSE},
        {MAKEFOURCC('D','X','T','3'), "D3DFMT_DXT3", SUPPORT_DXT3, 4, 4, TRUE,  FALSE},
        {MAKEFOURCC('D','X','T','4'), "D3DFMT_DXT4", SUPPORT_DXT4, 4, 4, TRUE,  FALSE},
        {MAKEFOURCC('D','X','T','5'), "D3DFMT_DXT5", SUPPORT_DXT5, 4, 4, TRUE,  FALSE},
        {MAKEFOURCC('Y','U','Y','2'), "D3DFMT_YUY2", SUPPORT_YUY2, 2, 1, FALSE, TRUE },
        {MAKEFOURCC('U','Y','V','Y'), "D3DFMT_UYVY", SUPPORT_UYVY, 2, 1, FALSE, TRUE },
    };
    const struct
    {
        DWORD caps, caps2;
        const char *name;
        BOOL overlay;
    }
    types[] =
    {
        /* DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY fails to create any fourcc
         * surface with DDERR_INVALIDPIXELFORMAT. Don't care about it for now.
         *
         * Nvidia returns E_FAIL on DXTN DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY.
         * Other hw / drivers successfully create those surfaces. Ignore them, this
         * suggests that no game uses this, otherwise Nvidia would support it. */
        {
            DDSCAPS_VIDEOMEMORY | DDSCAPS_TEXTURE, 0,
            "videomemory texture", FALSE
        },
        {
            DDSCAPS_VIDEOMEMORY | DDSCAPS_OVERLAY, 0,
            "videomemory overlay", TRUE
        },
        {
            DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE, 0,
            "systemmemory texture", FALSE
        },
        {
            DDSCAPS_TEXTURE, DDSCAPS2_TEXTUREMANAGE,
            "managed texture", FALSE
        }
    };

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);

    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);
    hr = IDirect3D3_QueryInterface(d3d, &IID_IDirectDraw4, (void **) &ddraw);
    ok(SUCCEEDED(hr), "Failed to get ddraw interface, hr %#x.\n", hr);
    IDirect3D3_Release(d3d);

    hr = IDirect3DDevice3_EnumTextureFormats(device, test_block_formats_creation_cb,
            &supported_fmts);
    ok(SUCCEEDED(hr), "Failed to enumerate texture formats %#x.\n", hr);

    hr = IDirectDraw4_GetFourCCCodes(ddraw, &num_fourcc_codes, NULL);
    ok(SUCCEEDED(hr), "Failed to get fourcc codes %#x.\n", hr);
    fourcc_codes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            num_fourcc_codes * sizeof(*fourcc_codes));
    if (!fourcc_codes)
        goto cleanup;
    hr = IDirectDraw4_GetFourCCCodes(ddraw, &num_fourcc_codes, fourcc_codes);
    ok(SUCCEEDED(hr), "Failed to get fourcc codes %#x.\n", hr);
    for (i = 0; i < num_fourcc_codes; i++)
    {
        for (j = 0; j < sizeof(formats) / sizeof(*formats); j++)
        {
            if (fourcc_codes[i] == formats[j].fourcc)
                supported_overlay_fmts |= formats[j].support_flag;
        }
    }
    HeapFree(GetProcessHeap(), 0, fourcc_codes);

    memset(&hal_caps, 0, sizeof(hal_caps));
    hal_caps.dwSize = sizeof(hal_caps);
    hr = IDirectDraw4_GetCaps(ddraw, &hal_caps, NULL);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);

    for (i = 0; i < sizeof(formats) / sizeof(*formats); i++)
    {
        for (j = 0; j < sizeof(types) / sizeof(*types); j++)
        {
            BOOL support;

            if (formats[i].overlay != types[j].overlay
                    || (types[j].overlay && !(hal_caps.dwCaps & DDCAPS_OVERLAY)))
                continue;

            if (formats[i].overlay)
                support = supported_overlay_fmts & formats[i].support_flag;
            else
                support = supported_fmts & formats[i].support_flag;

            for (w = 1; w <= 8; w++)
            {
                for (h = 1; h <= 8; h++)
                {
                    BOOL block_aligned = TRUE;
                    BOOL todo = FALSE;

                    if (w & (formats[i].block_width - 1) || h & (formats[i].block_height - 1))
                        block_aligned = FALSE;

                    memset(&ddsd, 0, sizeof(ddsd));
                    ddsd.dwSize = sizeof(ddsd);
                    ddsd.dwFlags = DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
                    ddsd.ddsCaps.dwCaps = types[j].caps;
                    ddsd.ddsCaps.dwCaps2 = types[j].caps2;
                    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
                    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_FOURCC;
                    U4(ddsd).ddpfPixelFormat.dwFourCC = formats[i].fourcc;
                    ddsd.dwWidth = w;
                    ddsd.dwHeight = h;

                    /* TODO: Handle power of two limitations. I cannot test the pow2
                     * behavior on windows because I have no hardware that doesn't at
                     * least support np2_conditional. There's probably no HW that
                     * supports DXTN textures but no conditional np2 textures. */
                    if (!support && !(types[j].caps & DDSCAPS_SYSTEMMEMORY))
                        expect_hr = DDERR_INVALIDPARAMS;
                    else if (formats[i].create_size_checked && !block_aligned)
                    {
                        expect_hr = DDERR_INVALIDPARAMS;
                        if (!(types[j].caps & DDSCAPS_TEXTURE))
                            todo = TRUE;
                    }
                    else
                        expect_hr = D3D_OK;

                    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &surface, NULL);
                    if (todo)
                        todo_wine ok(hr == expect_hr,
                                "Got unexpected hr %#x for format %s, resource type %s, size %ux%u, expected %#x.\n",
                                hr, formats[i].name, types[j].name, w, h, expect_hr);
                    else
                        ok(hr == expect_hr,
                                "Got unexpected hr %#x for format %s, resource type %s, size %ux%u, expected %#x.\n",
                                hr, formats[i].name, types[j].name, w, h, expect_hr);

                    if (SUCCEEDED(hr))
                        IDirectDrawSurface4_Release(surface);
                }
            }
        }
    }

cleanup:
    IDirectDraw4_Release(ddraw);
    IDirect3DDevice3_Release(device);
    DestroyWindow(window);
}

struct format_support_check
{
    const DDPIXELFORMAT *format;
    BOOL supported;
};

static HRESULT WINAPI test_unsupported_formats_cb(DDPIXELFORMAT *fmt, void *ctx)
{
    struct format_support_check *format = ctx;

    if (!memcmp(format->format, fmt, sizeof(*fmt)))
    {
        format->supported = TRUE;
        return DDENUMRET_CANCEL;
    }

    return DDENUMRET_OK;
}

static void test_unsupported_formats(void)
{
    HRESULT hr;
    BOOL expect_success;
    HWND window;
    IDirectDraw4 *ddraw;
    IDirect3D3 *d3d;
    IDirect3DDevice3 *device;
    IDirectDrawSurface4 *surface;
    DDSURFACEDESC2 ddsd;
    unsigned int i, j;
    DWORD expected_caps;
    static const struct
    {
        const char *name;
        DDPIXELFORMAT fmt;
    }
    formats[] =
    {
        {
            "D3DFMT_A8R8G8B8",
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_ALPHAPIXELS, 0,
                {32}, {0x00ff0000}, {0x0000ff00}, {0x000000ff}, {0xff000000}
            }
        },
        {
            "D3DFMT_P8",
            {
                sizeof(DDPIXELFORMAT), DDPF_PALETTEINDEXED8 | DDPF_RGB, 0,
                {8 }, {0x00000000}, {0x00000000}, {0x00000000}, {0x00000000}
            }
        },
    };
    static const DWORD caps[] = {0, DDSCAPS_SYSTEMMEMORY, DDSCAPS_VIDEOMEMORY};

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);

    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);
    hr = IDirect3D3_QueryInterface(d3d, &IID_IDirectDraw4, (void **) &ddraw);
    ok(SUCCEEDED(hr), "Failed to get ddraw interface, hr %#x.\n", hr);
    IDirect3D3_Release(d3d);

    for (i = 0; i < sizeof(formats) / sizeof(*formats); i++)
    {
        struct format_support_check check = {&formats[i].fmt, FALSE};
        hr = IDirect3DDevice3_EnumTextureFormats(device, test_unsupported_formats_cb, &check);
        ok(SUCCEEDED(hr), "Failed to enumerate texture formats %#x.\n", hr);

        for (j = 0; j < sizeof(caps) / sizeof(*caps); j++)
        {
            memset(&ddsd, 0, sizeof(ddsd));
            ddsd.dwSize = sizeof(ddsd);
            ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
            U4(ddsd).ddpfPixelFormat = formats[i].fmt;
            ddsd.dwWidth = 4;
            ddsd.dwHeight = 4;
            ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | caps[j];

            if (caps[j] & DDSCAPS_VIDEOMEMORY && !check.supported)
                expect_success = FALSE;
            else
                expect_success = TRUE;

            hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &surface, NULL);
            ok(SUCCEEDED(hr) == expect_success,
                    "Got unexpected hr %#x for format %s, caps %#x, expected %s.\n",
                    hr, formats[i].name, caps[j], expect_success ? "success" : "failure");
            if (FAILED(hr))
                continue;

            memset(&ddsd, 0, sizeof(ddsd));
            ddsd.dwSize = sizeof(ddsd);
            hr = IDirectDrawSurface4_GetSurfaceDesc(surface, &ddsd);
            ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);

            if (caps[j] & DDSCAPS_VIDEOMEMORY)
                expected_caps = DDSCAPS_VIDEOMEMORY;
            else if (caps[j] & DDSCAPS_SYSTEMMEMORY)
                expected_caps = DDSCAPS_SYSTEMMEMORY;
            else if (check.supported)
                expected_caps = DDSCAPS_VIDEOMEMORY;
            else
                expected_caps = DDSCAPS_SYSTEMMEMORY;

            ok(ddsd.ddsCaps.dwCaps & expected_caps,
                    "Expected capability %#x, format %s, input cap %#x.\n",
                    expected_caps, formats[i].name, caps[j]);

            IDirectDrawSurface4_Release(surface);
        }
    }

    IDirectDraw4_Release(ddraw);
    IDirect3DDevice3_Release(device);
    DestroyWindow(window);
}

static void test_rt_caps(void)
{
    PALETTEENTRY palette_entries[256];
    IDirectDrawPalette *palette;
    IDirectDraw4 *ddraw;
    DDPIXELFORMAT z_fmt;
    IDirect3D3 *d3d;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    static const DDPIXELFORMAT p8_fmt =
    {
        sizeof(DDPIXELFORMAT), DDPF_PALETTEINDEXED8 | DDPF_RGB, 0,
        {8}, {0x00000000}, {0x00000000}, {0x00000000}, {0x00000000},
    };

    const struct
    {
        const DDPIXELFORMAT *pf;
        DWORD caps_in;
        DWORD caps_out;
        HRESULT create_device_hr;
        HRESULT set_rt_hr, alternative_set_rt_hr;
    }
    test_data[] =
    {
        {
            NULL,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
            D3D_OK,
            D3D_OK,
            D3D_OK,
        },
        {
            NULL,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
            D3D_OK,
            D3D_OK,
            D3D_OK,
        },
        {
            NULL,
            DDSCAPS_OFFSCREENPLAIN,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
        },
        {
            NULL,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY | DDSCAPS_3DDEVICE,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY | DDSCAPS_3DDEVICE,
            D3DERR_SURFACENOTINVIDMEM,
            D3D_OK,
            D3D_OK,
        },
        {
            NULL,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
        },
        {
            NULL,
            DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY,
            DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
            D3D_OK,
            D3D_OK,
            D3D_OK,
        },
        {
            NULL,
            DDSCAPS_3DDEVICE,
            DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
            D3D_OK,
            D3D_OK,
            D3D_OK,
        },
        {
            NULL,
            0,
            DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
        },
        {
            NULL,
            DDSCAPS_SYSTEMMEMORY | DDSCAPS_3DDEVICE,
            DDSCAPS_SYSTEMMEMORY | DDSCAPS_3DDEVICE,
            D3DERR_SURFACENOTINVIDMEM,
            D3D_OK,
            D3D_OK,
        },
        {
            NULL,
            DDSCAPS_SYSTEMMEMORY,
            DDSCAPS_SYSTEMMEMORY,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
        },
        {
            &p8_fmt,
            0,
            DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
        },
        {
            &p8_fmt,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE,
            ~0U /* AMD r200 */,
            DDERR_NOPALETTEATTACHED,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
        },
        {
            &p8_fmt,
            DDSCAPS_OFFSCREENPLAIN,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
        },
        {
            &p8_fmt,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY | DDSCAPS_3DDEVICE,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY | DDSCAPS_3DDEVICE,
            DDERR_NOPALETTEATTACHED,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
        },
        {
            &p8_fmt,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
        },
        {
            &z_fmt,
            DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY | DDSCAPS_ZBUFFER,
            DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY | DDSCAPS_ZBUFFER | DDSCAPS_LOCALVIDMEM,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDPIXELFORMAT,
            D3D_OK /* r200 */,
        },
        {
            &z_fmt,
            DDSCAPS_3DDEVICE | DDSCAPS_ZBUFFER,
            DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY | DDSCAPS_ZBUFFER | DDSCAPS_LOCALVIDMEM,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDPIXELFORMAT,
            D3D_OK /* r200 */,
        },
        {
            &z_fmt,
            DDSCAPS_ZBUFFER,
            DDSCAPS_VIDEOMEMORY | DDSCAPS_ZBUFFER | DDSCAPS_LOCALVIDMEM,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
        },
        {
            &z_fmt,
            DDSCAPS_SYSTEMMEMORY | DDSCAPS_3DDEVICE | DDSCAPS_ZBUFFER,
            DDSCAPS_SYSTEMMEMORY | DDSCAPS_3DDEVICE | DDSCAPS_ZBUFFER,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDPIXELFORMAT,
            D3D_OK /* r200 */,
        },
        {
            &z_fmt,
            DDSCAPS_SYSTEMMEMORY | DDSCAPS_ZBUFFER,
            DDSCAPS_SYSTEMMEMORY | DDSCAPS_ZBUFFER,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
        },
    };

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    if (FAILED(hr = IDirectDraw4_QueryInterface(ddraw, &IID_IDirect3D3, (void **)&d3d)))
    {
        skip("D3D interface is not available, skipping test.\n");
        goto done;
    }

    memset(&z_fmt, 0, sizeof(z_fmt));
    hr = IDirect3D3_EnumZBufferFormats(d3d, &IID_IDirect3DHALDevice, enum_z_fmt, &z_fmt);
    if (FAILED(hr) || !z_fmt.dwSize)
    {
        skip("No depth buffer formats available, skipping test.\n");
        IDirect3D3_Release(d3d);
        goto done;
    }

    memset(palette_entries, 0, sizeof(palette_entries));
    hr = IDirectDraw4_CreatePalette(ddraw, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, palette_entries, &palette, NULL);
    ok(SUCCEEDED(hr), "Failed to create palette, hr %#x.\n", hr);

    for (i = 0; i < sizeof(test_data) / sizeof(*test_data); ++i)
    {
        IDirectDrawSurface4 *surface, *rt, *expected_rt, *tmp;
        DDSURFACEDESC2 surface_desc;
        IDirect3DDevice3 *device;

        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        surface_desc.ddsCaps.dwCaps = test_data[i].caps_in;
        if (test_data[i].pf)
        {
            surface_desc.dwFlags |= DDSD_PIXELFORMAT;
            U4(surface_desc).ddpfPixelFormat = *test_data[i].pf;
        }
        surface_desc.dwWidth = 640;
        surface_desc.dwHeight = 480;
        hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
        ok(SUCCEEDED(hr), "Test %u: Failed to create surface with caps %#x, hr %#x.\n",
                i, test_data[i].caps_in, hr);

        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        hr = IDirectDrawSurface4_GetSurfaceDesc(surface, &surface_desc);
        ok(SUCCEEDED(hr), "Test %u: Failed to get surface desc, hr %#x.\n", i, hr);
        ok(test_data[i].caps_out == ~0U || surface_desc.ddsCaps.dwCaps == test_data[i].caps_out,
                "Test %u: Got unexpected caps %#x, expected %#x.\n",
                i, surface_desc.ddsCaps.dwCaps, test_data[i].caps_out);

        hr = IDirect3D3_CreateDevice(d3d, &IID_IDirect3DHALDevice, surface, &device, NULL);
        ok(hr == test_data[i].create_device_hr, "Test %u: Got unexpected hr %#x, expected %#x.\n",
                i, hr, test_data[i].create_device_hr);
        if (FAILED(hr))
        {
            if (hr == DDERR_NOPALETTEATTACHED)
            {
                hr = IDirectDrawSurface4_SetPalette(surface, palette);
                ok(SUCCEEDED(hr), "Test %u: Failed to set palette, hr %#x.\n", i, hr);
                hr = IDirect3D3_CreateDevice(d3d, &IID_IDirect3DHALDevice, surface, &device, NULL);
                if (surface_desc.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY)
                    ok(hr == DDERR_INVALIDPIXELFORMAT, "Test %u: Got unexpected hr %#x.\n", i, hr);
                else
                    ok(hr == D3DERR_SURFACENOTINVIDMEM, "Test %u: Got unexpected hr %#x.\n", i, hr);
            }
            IDirectDrawSurface4_Release(surface);

            memset(&surface_desc, 0, sizeof(surface_desc));
            surface_desc.dwSize = sizeof(surface_desc);
            surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
            surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
            surface_desc.dwWidth = 640;
            surface_desc.dwHeight = 480;
            hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
            ok(SUCCEEDED(hr), "Test %u: Failed to create surface, hr %#x.\n", i, hr);

            hr = IDirect3D3_CreateDevice(d3d, &IID_IDirect3DHALDevice, surface, &device, NULL);
            ok(SUCCEEDED(hr), "Test %u: Failed to create device, hr %#x.\n", i, hr);
        }

        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        surface_desc.ddsCaps.dwCaps = test_data[i].caps_in;
        if (test_data[i].pf)
        {
            surface_desc.dwFlags |= DDSD_PIXELFORMAT;
            U4(surface_desc).ddpfPixelFormat = *test_data[i].pf;
        }
        surface_desc.dwWidth = 640;
        surface_desc.dwHeight = 480;
        hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &rt, NULL);
        ok(SUCCEEDED(hr), "Test %u: Failed to create surface with caps %#x, hr %#x.\n",
                i, test_data[i].caps_in, hr);

        hr = IDirect3DDevice3_SetRenderTarget(device, rt, 0);
        ok(hr == test_data[i].set_rt_hr || broken(hr == test_data[i].alternative_set_rt_hr),
                "Test %u: Got unexpected hr %#x, expected %#x.\n",
                i, hr, test_data[i].set_rt_hr);
        if (SUCCEEDED(hr) || hr == DDERR_INVALIDPIXELFORMAT)
            expected_rt = rt;
        else
            expected_rt = surface;

        hr = IDirect3DDevice3_GetRenderTarget(device, &tmp);
        ok(SUCCEEDED(hr), "Test %u: Failed to get render target, hr %#x.\n", i, hr);
        ok(tmp == expected_rt, "Test %u: Got unexpected rt %p.\n", i, tmp);

        IDirectDrawSurface4_Release(tmp);
        IDirectDrawSurface4_Release(rt);
        refcount = IDirect3DDevice3_Release(device);
        ok(refcount == 0, "Test %u: The device was not properly freed, refcount %u.\n", i, refcount);
        refcount = IDirectDrawSurface4_Release(surface);
        ok(refcount == 0, "Test %u: The surface was not properly freed, refcount %u.\n", i, refcount);
    }

    IDirectDrawPalette_Release(palette);
    IDirect3D3_Release(d3d);

done:
    refcount = IDirectDraw4_Release(ddraw);
    ok(refcount == 0, "The ddraw object was not properly freed, refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_primary_caps(void)
{
    const DWORD placement = DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY | DDSCAPS_SYSTEMMEMORY;
    IDirectDrawSurface4 *surface;
    DDSURFACEDESC2 surface_desc;
    IDirectDraw4 *ddraw;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    static const struct
    {
        DWORD coop_level;
        DWORD caps_in;
        DWORD back_buffer_count;
        HRESULT hr;
        DWORD caps_out;
    }
    test_data[] =
    {
        {
            DDSCL_NORMAL,
            DDSCAPS_PRIMARYSURFACE,
            ~0u,
            DD_OK,
            DDSCAPS_VISIBLE | DDSCAPS_PRIMARYSURFACE,
        },
        {
            DDSCL_NORMAL,
            DDSCAPS_PRIMARYSURFACE | DDSCAPS_TEXTURE,
            ~0u,
            DDERR_INVALIDCAPS,
            ~0u,
        },
        {
            DDSCL_NORMAL,
            DDSCAPS_PRIMARYSURFACE | DDSCAPS_FRONTBUFFER,
            ~0u,
            DDERR_INVALIDCAPS,
            ~0u,
        },
        {
            DDSCL_NORMAL,
            DDSCAPS_PRIMARYSURFACE | DDSCAPS_BACKBUFFER,
            ~0u,
            DDERR_INVALIDCAPS,
            ~0u,
        },
        {
            DDSCL_NORMAL,
            DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP,
            ~0u,
            DDERR_INVALIDCAPS,
            ~0u,
        },
        {
            DDSCL_NORMAL,
            DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX,
            ~0u,
            DDERR_INVALIDCAPS,
            ~0u,
        },
        {
            DDSCL_NORMAL,
            DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP,
            ~0u,
            DDERR_INVALIDCAPS,
            ~0u,
        },
        {
            DDSCL_NORMAL,
            DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP,
            0,
            DDERR_INVALIDCAPS,
            ~0u,
        },
        {
            DDSCL_NORMAL,
            DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP,
            1,
            DDERR_NOEXCLUSIVEMODE,
            ~0u,
        },
        {
            DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN,
            DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP,
            0,
            DDERR_INVALIDCAPS,
            ~0u,
        },
        {
            DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN,
            DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP,
            1,
            DD_OK,
            DDSCAPS_VISIBLE | DDSCAPS_PRIMARYSURFACE | DDSCAPS_FRONTBUFFER | DDSCAPS_FLIP | DDSCAPS_COMPLEX,
        },
        {
            DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN,
            DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP | DDSCAPS_FRONTBUFFER,
            1,
            DDERR_INVALIDCAPS,
            ~0u,
        },
        {
            DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN,
            DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP | DDSCAPS_BACKBUFFER,
            1,
            DDERR_INVALIDCAPS,
            ~0u,
        },
    };

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    for (i = 0; i < sizeof(test_data) / sizeof(*test_data); ++i)
    {
        hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, test_data[i].coop_level);
        ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS;
        if (test_data[i].back_buffer_count != ~0u)
            surface_desc.dwFlags |= DDSD_BACKBUFFERCOUNT;
        surface_desc.ddsCaps.dwCaps = test_data[i].caps_in;
        surface_desc.dwBackBufferCount = test_data[i].back_buffer_count;
        hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
        ok(hr == test_data[i].hr, "Test %u: Got unexpected hr %#x, expected %#x.\n", i, hr, test_data[i].hr);
        if (FAILED(hr))
            continue;

        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        hr = IDirectDrawSurface4_GetSurfaceDesc(surface, &surface_desc);
        ok(SUCCEEDED(hr), "Test %u: Failed to get surface desc, hr %#x.\n", i, hr);
        ok((surface_desc.ddsCaps.dwCaps & ~placement) == test_data[i].caps_out,
                "Test %u: Got unexpected caps %#x, expected %#x.\n",
                i, surface_desc.ddsCaps.dwCaps, test_data[i].caps_out);

        IDirectDrawSurface4_Release(surface);
    }

    refcount = IDirectDraw4_Release(ddraw);
    ok(refcount == 0, "The ddraw object was not properly freed, refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_surface_lock(void)
{
    IDirectDraw4 *ddraw;
    IDirect3D3 *d3d = NULL;
    IDirectDrawSurface4 *surface;
    HRESULT hr;
    HWND window;
    unsigned int i;
    DDSURFACEDESC2 ddsd;
    ULONG refcount;
    DDPIXELFORMAT z_fmt;
    static const struct
    {
        DWORD caps;
        DWORD caps2;
        const char *name;
    }
    tests[] =
    {
        {
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY,
            0,
            "videomemory offscreenplain"
        },
        {
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY,
            0,
            "systemmemory offscreenplain"
        },
        {
            DDSCAPS_PRIMARYSURFACE,
            0,
            "primary"
        },
        {
            DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY,
            0,
            "videomemory texture"
        },
        {
            DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY,
            DDSCAPS2_OPAQUE,
            "opaque videomemory texture"
        },
        {
            DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY,
            0,
            "systemmemory texture"
        },
        {
            DDSCAPS_TEXTURE,
            DDSCAPS2_TEXTUREMANAGE,
            "managed texture"
        },
        {
            DDSCAPS_TEXTURE,
            DDSCAPS2_D3DTEXTUREMANAGE,
            "managed texture"
        },
        {
            DDSCAPS_TEXTURE,
            DDSCAPS2_TEXTUREMANAGE | DDSCAPS2_OPAQUE,
            "opaque managed texture"
        },
        {
            DDSCAPS_TEXTURE,
            DDSCAPS2_D3DTEXTUREMANAGE | DDSCAPS2_OPAQUE,
            "opaque managed texture"
        },
        {
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE,
            0,
            "render target"
        },
        {
            DDSCAPS_ZBUFFER,
            0,
            "Z buffer"
        },
    };

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    if (FAILED(hr = IDirectDraw4_QueryInterface(ddraw, &IID_IDirect3D3, (void **)&d3d)))
    {
        skip("D3D interface is not available, skipping test.\n");
        goto done;
    }

    memset(&z_fmt, 0, sizeof(z_fmt));
    hr = IDirect3D3_EnumZBufferFormats(d3d, &IID_IDirect3DHALDevice, enum_z_fmt, &z_fmt);
    if (FAILED(hr) || !z_fmt.dwSize)
    {
        skip("No depth buffer formats available, skipping test.\n");
        goto done;
    }

    for (i = 0; i < sizeof(tests) / sizeof(*tests); i++)
    {
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS;
        if (!(tests[i].caps & DDSCAPS_PRIMARYSURFACE))
        {
            ddsd.dwFlags |= DDSD_WIDTH | DDSD_HEIGHT;
            ddsd.dwWidth = 64;
            ddsd.dwHeight = 64;
        }
        if (tests[i].caps & DDSCAPS_ZBUFFER)
        {
            ddsd.dwFlags |= DDSD_PIXELFORMAT;
            U4(ddsd).ddpfPixelFormat = z_fmt;
        }
        ddsd.ddsCaps.dwCaps = tests[i].caps;
        ddsd.ddsCaps.dwCaps2 = tests[i].caps2;

        hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &surface, NULL);
        ok(SUCCEEDED(hr), "Failed to create surface, type %s, hr %#x.\n", tests[i].name, hr);

        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        hr = IDirectDrawSurface4_Lock(surface, NULL, &ddsd, DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to lock surface, type %s, hr %#x.\n", tests[i].name, hr);
        if (SUCCEEDED(hr))
        {
            hr = IDirectDrawSurface4_Unlock(surface, NULL);
            ok(SUCCEEDED(hr), "Failed to unlock surface, type %s, hr %#x.\n", tests[i].name, hr);
        }

        IDirectDrawSurface4_Release(surface);
    }

done:
    if (d3d)
        IDirect3D3_Release(d3d);
    refcount = IDirectDraw4_Release(ddraw);
    ok(refcount == 0, "The ddraw object was not properly freed, refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_surface_discard(void)
{
    IDirect3DDevice3 *device;
    IDirect3D3 *d3d;
    IDirectDraw4 *ddraw;
    HRESULT hr;
    HWND window;
    DDSURFACEDESC2 ddsd;
    IDirectDrawSurface4 *surface, *target;
    void *addr;
    static const struct
    {
        DWORD caps, caps2;
        BOOL discard;
    }
    tests[] =
    {
        {DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY, 0, TRUE},
        {DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY, 0, FALSE},
        {DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY, 0, TRUE},
        {DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY, 0, FALSE},
        {DDSCAPS_TEXTURE, DDSCAPS2_TEXTUREMANAGE, FALSE},
        {DDSCAPS_TEXTURE, DDSCAPS2_TEXTUREMANAGE | DDSCAPS2_HINTDYNAMIC, FALSE},
        {DDSCAPS_TEXTURE, DDSCAPS2_D3DTEXTUREMANAGE, FALSE},
        {DDSCAPS_TEXTURE, DDSCAPS2_D3DTEXTUREMANAGE | DDSCAPS2_HINTDYNAMIC, FALSE},
    };
    unsigned int i;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);

    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }
    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);
    hr = IDirect3D3_QueryInterface(d3d, &IID_IDirectDraw4, (void **)&ddraw);
    ok(SUCCEEDED(hr), "Failed to get ddraw interface, hr %#x.\n", hr);
    hr = IDirect3DDevice3_GetRenderTarget(device, &target);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    for (i = 0; i < sizeof(tests) / sizeof(*tests); i++)
    {
        BOOL discarded;

        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        ddsd.ddsCaps.dwCaps = tests[i].caps;
        ddsd.ddsCaps.dwCaps2 = tests[i].caps2;
        ddsd.dwWidth = 64;
        ddsd.dwHeight = 64;
        hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &surface, NULL);
        ok(SUCCEEDED(hr), "Failed to create offscreen surface, hr %#x, case %u.\n", hr, i);

        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        hr = IDirectDrawSurface4_Lock(surface, NULL, &ddsd, 0, NULL);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
        addr = ddsd.lpSurface;
        hr = IDirectDrawSurface4_Unlock(surface, NULL);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        hr = IDirectDrawSurface4_Lock(surface, NULL, &ddsd, DDLOCK_DISCARDCONTENTS, NULL);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
        discarded = ddsd.lpSurface != addr;
        hr = IDirectDrawSurface4_Unlock(surface, NULL);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

        hr = IDirectDrawSurface4_Blt(target, NULL, surface, NULL, DDBLT_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        hr = IDirectDrawSurface4_Lock(surface, NULL, &ddsd, DDLOCK_DISCARDCONTENTS, NULL);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
        discarded |= ddsd.lpSurface != addr;
        hr = IDirectDrawSurface4_Unlock(surface, NULL);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

        IDirectDrawSurface4_Release(surface);

        /* Windows 7 reliably changes the address of surfaces that are discardable (Nvidia Kepler,
         * AMD r500, evergreen). Windows XP, at least on AMD r200, does not. */
        ok(!discarded || tests[i].discard, "Expected surface not to be discarded, case %u\n", i);
    }

    IDirectDrawSurface4_Release(target);
    IDirectDraw4_Release(ddraw);
    IDirect3D3_Release(d3d);
    IDirect3DDevice3_Release(device);
    DestroyWindow(window);
}

static void test_flip(void)
{
    const DWORD placement = DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY | DDSCAPS_SYSTEMMEMORY;
    IDirectDrawSurface4 *primary, *backbuffer1, *backbuffer2, *backbuffer3, *surface;
    DDSCAPS2 caps = {DDSCAPS_FLIP, 0, 0, 0};
    DDSURFACEDESC2 surface_desc;
    BOOL sysmem_primary;
    IDirectDraw4 *ddraw;
    D3DCOLOR color;
    ULONG refcount;
    HWND window;
    DDBLTFX fx;
    HRESULT hr;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    hr = set_display_mode(ddraw, 640, 480);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;
    surface_desc.dwBackBufferCount = 3;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok((surface_desc.ddsCaps.dwCaps & ~placement)
            == (DDSCAPS_VISIBLE | DDSCAPS_PRIMARYSURFACE | DDSCAPS_FRONTBUFFER | DDSCAPS_FLIP | DDSCAPS_COMPLEX),
            "Got unexpected caps %#x.\n", surface_desc.ddsCaps.dwCaps);
    sysmem_primary = surface_desc.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY;

    hr = IDirectDrawSurface4_GetAttachedSurface(primary, &caps, &backbuffer1);
    ok(SUCCEEDED(hr), "Failed to get attached surface, hr %#x.\n", hr);
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface4_GetSurfaceDesc(backbuffer1, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(!surface_desc.dwBackBufferCount, "Got unexpected back buffer count %u.\n", surface_desc.dwBackBufferCount);
    ok((surface_desc.ddsCaps.dwCaps & ~placement) == (DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_BACKBUFFER),
            "Got unexpected caps %#x.\n", surface_desc.ddsCaps.dwCaps);

    hr = IDirectDrawSurface4_GetAttachedSurface(backbuffer1, &caps, &backbuffer2);
    ok(SUCCEEDED(hr), "Failed to get attached surface, hr %#x.\n", hr);
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface4_GetSurfaceDesc(backbuffer2, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(!surface_desc.dwBackBufferCount, "Got unexpected back buffer count %u.\n", surface_desc.dwBackBufferCount);
    ok((surface_desc.ddsCaps.dwCaps & ~placement) == (DDSCAPS_FLIP | DDSCAPS_COMPLEX),
            "Got unexpected caps %#x.\n", surface_desc.ddsCaps.dwCaps);

    hr = IDirectDrawSurface4_GetAttachedSurface(backbuffer2, &caps, &backbuffer3);
    ok(SUCCEEDED(hr), "Failed to get attached surface, hr %#x.\n", hr);
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface4_GetSurfaceDesc(backbuffer3, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(!surface_desc.dwBackBufferCount, "Got unexpected back buffer count %u.\n", surface_desc.dwBackBufferCount);
    ok((surface_desc.ddsCaps.dwCaps & ~placement) == (DDSCAPS_FLIP | DDSCAPS_COMPLEX),
            "Got unexpected caps %#x.\n", surface_desc.ddsCaps.dwCaps);

    hr = IDirectDrawSurface4_GetAttachedSurface(backbuffer3, &caps, &surface);
    ok(SUCCEEDED(hr), "Failed to get attached surface, hr %#x.\n", hr);
    ok(surface == primary, "Got unexpected surface %p, expected %p.\n", surface, primary);
    IDirectDrawSurface4_Release(surface);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = 0;
    surface_desc.dwWidth = 640;
    surface_desc.dwHeight = 480;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Flip(primary, surface, DDFLIP_WAIT);
    ok(hr == DDERR_NOTFLIPPABLE, "Got unexpected hr %#x.\n", hr);
    IDirectDrawSurface4_Release(surface);

    hr = IDirectDrawSurface4_Flip(primary, primary, DDFLIP_WAIT);
    ok(hr == DDERR_NOTFLIPPABLE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Flip(backbuffer1, NULL, DDFLIP_WAIT);
    ok(hr == DDERR_NOTFLIPPABLE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Flip(backbuffer2, NULL, DDFLIP_WAIT);
    ok(hr == DDERR_NOTFLIPPABLE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Flip(backbuffer3, NULL, DDFLIP_WAIT);
    ok(hr == DDERR_NOTFLIPPABLE, "Got unexpected hr %#x.\n", hr);

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    U5(fx).dwFillColor = 0xffff0000;
    hr = IDirectDrawSurface4_Blt(backbuffer1, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to fill surface, hr %#x.\n", hr);
    U5(fx).dwFillColor = 0xff00ff00;
    hr = IDirectDrawSurface4_Blt(backbuffer2, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to fill surface, hr %#x.\n", hr);
    U5(fx).dwFillColor = 0xff0000ff;
    hr = IDirectDrawSurface4_Blt(backbuffer3, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to fill surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_Flip(primary, NULL, DDFLIP_WAIT);
    ok(SUCCEEDED(hr), "Failed to flip, hr %#x.\n", hr);
    color = get_surface_color(backbuffer1, 320, 240);
    /* The testbot seems to just copy the contents of one surface to all the
     * others, instead of properly flipping. */
    ok(compare_color(color, 0x0000ff00, 1) || broken(sysmem_primary && compare_color(color, 0x000000ff, 1)),
            "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(backbuffer2, 320, 240);
    ok(compare_color(color, 0x000000ff, 1), "Got unexpected color 0x%08x.\n", color);
    U5(fx).dwFillColor = 0xffff0000;
    hr = IDirectDrawSurface4_Blt(backbuffer3, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to fill surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_Flip(primary, NULL, DDFLIP_WAIT);
    ok(SUCCEEDED(hr), "Failed to flip, hr %#x.\n", hr);
    color = get_surface_color(backbuffer1, 320, 240);
    ok(compare_color(color, 0x000000ff, 1) || broken(sysmem_primary && compare_color(color, 0x00ff0000, 1)),
            "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(backbuffer2, 320, 240);
    ok(compare_color(color, 0x00ff0000, 1), "Got unexpected color 0x%08x.\n", color);
    U5(fx).dwFillColor = 0xff00ff00;
    hr = IDirectDrawSurface4_Blt(backbuffer3, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to fill surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_Flip(primary, NULL, DDFLIP_WAIT);
    ok(SUCCEEDED(hr), "Failed to flip, hr %#x.\n", hr);
    color = get_surface_color(backbuffer1, 320, 240);
    ok(compare_color(color, 0x00ff0000, 1) || broken(sysmem_primary && compare_color(color, 0x0000ff00, 1)),
            "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(backbuffer2, 320, 240);
    ok(compare_color(color, 0x0000ff00, 1), "Got unexpected color 0x%08x.\n", color);
    U5(fx).dwFillColor = 0xff0000ff;
    hr = IDirectDrawSurface4_Blt(backbuffer3, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to fill surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_Flip(primary, backbuffer1, DDFLIP_WAIT);
    ok(SUCCEEDED(hr), "Failed to flip, hr %#x.\n", hr);
    color = get_surface_color(backbuffer2, 320, 240);
    ok(compare_color(color, 0x0000ff00, 1) || broken(sysmem_primary && compare_color(color, 0x000000ff, 1)),
            "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(backbuffer3, 320, 240);
    ok(compare_color(color, 0x000000ff, 1), "Got unexpected color 0x%08x.\n", color);
    U5(fx).dwFillColor = 0xffff0000;
    hr = IDirectDrawSurface4_Blt(backbuffer1, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to fill surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_Flip(primary, backbuffer2, DDFLIP_WAIT);
    ok(SUCCEEDED(hr), "Failed to flip, hr %#x.\n", hr);
    color = get_surface_color(backbuffer1, 320, 240);
    ok(compare_color(color, 0x00ff0000, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(backbuffer3, 320, 240);
    ok(compare_color(color, 0x000000ff, 1) || broken(sysmem_primary && compare_color(color, 0x00ff0000, 1)),
            "Got unexpected color 0x%08x.\n", color);
    U5(fx).dwFillColor = 0xff00ff00;
    hr = IDirectDrawSurface4_Blt(backbuffer2, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to fill surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_Flip(primary, backbuffer3, DDFLIP_WAIT);
    ok(SUCCEEDED(hr), "Failed to flip, hr %#x.\n", hr);
    color = get_surface_color(backbuffer1, 320, 240);
    ok(compare_color(color, 0x00ff0000, 1) || broken(sysmem_primary && compare_color(color, 0x0000ff00, 1)),
            "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(backbuffer2, 320, 240);
    ok(compare_color(color, 0x0000ff00, 1), "Got unexpected color 0x%08x.\n", color);

    IDirectDrawSurface4_Release(backbuffer3);
    IDirectDrawSurface4_Release(backbuffer2);
    IDirectDrawSurface4_Release(backbuffer1);
    IDirectDrawSurface4_Release(primary);
    refcount = IDirectDraw4_Release(ddraw);
    ok(refcount == 0, "The ddraw object was not properly freed, refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void reset_ddsd(DDSURFACEDESC2 *ddsd)
{
    memset(ddsd, 0, sizeof(*ddsd));
    ddsd->dwSize = sizeof(*ddsd);
}

static void test_set_surface_desc(void)
{
    IDirectDraw4 *ddraw;
    HWND window;
    HRESULT hr;
    DDSURFACEDESC2 ddsd;
    IDirectDrawSurface4 *surface;
    BYTE data[16*16*4];
    ULONG ref;
    unsigned int i;
    static const struct
    {
        DWORD caps, caps2;
        BOOL supported;
        const char *name;
    }
    invalid_caps_tests[] =
    {
        {DDSCAPS_VIDEOMEMORY, 0, FALSE, "videomemory plain"},
        {DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY, 0, TRUE, "systemmemory texture"},
        {DDSCAPS_TEXTURE, DDSCAPS2_D3DTEXTUREMANAGE, FALSE, "managed texture"},
        {DDSCAPS_TEXTURE, DDSCAPS2_TEXTUREMANAGE, FALSE, "managed texture"},
        {DDSCAPS_PRIMARYSURFACE | DDSCAPS_SYSTEMMEMORY, 0, FALSE, "systemmemory primary"},
    };

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT;
    ddsd.dwWidth = 8;
    ddsd.dwHeight = 8;
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 32;
    U2(U4(ddsd).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(U4(ddsd).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(U4(ddsd).ddpfPixelFormat).dwBBitMask = 0x000000ff;
    ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;

    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_LPSURFACE;
    ddsd.lpSurface = data;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(SUCCEEDED(hr), "Failed to set surface desc, hr %#x.\n", hr);

    /* Redundantly setting the same lpSurface is not an error. */
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(SUCCEEDED(hr), "Failed to set surface desc, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetSurfaceDesc(surface, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(!(ddsd.dwFlags & DDSD_LPSURFACE), "DDSD_LPSURFACE is set.\n");
    ok(ddsd.lpSurface == NULL, "lpSurface is %p, expected NULL.\n", ddsd.lpSurface);

    hr = IDirectDrawSurface4_Lock(surface, NULL, &ddsd, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    ok(!(ddsd.dwFlags & DDSD_LPSURFACE), "DDSD_LPSURFACE is set.\n");
    ok(ddsd.lpSurface == data, "lpSurface is %p, expected %p.\n", data, data);
    hr = IDirectDrawSurface4_Unlock(surface, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_LPSURFACE;
    ddsd.lpSurface = data;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 1);
    ok(hr == DDERR_INVALIDPARAMS, "SetSurfaceDesc with flags=1 returned %#x.\n", hr);

    ddsd.lpSurface = NULL;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting lpSurface=NULL returned %#x.\n", hr);

    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, NULL, 0);
    ok(hr == DDERR_INVALIDPARAMS, "SetSurfaceDesc with NULL desc returned %#x.\n", hr);

    hr = IDirectDrawSurface4_GetSurfaceDesc(surface, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.ddsCaps.dwCaps == (DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN),
            "Got unexpected caps %#x.\n", ddsd.ddsCaps.dwCaps);
    ok(ddsd.ddsCaps.dwCaps2 == 0, "Got unexpected caps2 %#x.\n", 0);

    /* Setting the caps is an error. This also means the original description cannot be reapplied. */
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting the original desc returned %#x.\n", hr);

    ddsd.dwFlags = DDSD_CAPS;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting DDSD_CAPS returned %#x.\n", hr);

    /* dwCaps = 0 is allowed, but ignored. Caps2 can be anything and is ignored too. */
    ddsd.dwFlags = DDSD_CAPS | DDSD_LPSURFACE;
    ddsd.lpSurface = data;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDCAPS, "Setting DDSD_CAPS returned %#x.\n", hr);
    ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDCAPS, "Setting DDSD_CAPS returned %#x.\n", hr);
    ddsd.ddsCaps.dwCaps = 0;
    ddsd.ddsCaps.dwCaps2 = 0xdeadbeef;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(SUCCEEDED(hr), "Failed to set surface desc, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_GetSurfaceDesc(surface, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.ddsCaps.dwCaps == (DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN),
            "Got unexpected caps %#x.\n", ddsd.ddsCaps.dwCaps);
    ok(ddsd.ddsCaps.dwCaps2 == 0, "Got unexpected caps2 %#x.\n", 0);

    /* Setting the height is allowed, but it cannot be set to 0, and only if LPSURFACE is set too. */
    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_HEIGHT;
    ddsd.dwHeight = 16;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting height without lpSurface returned %#x.\n", hr);

    ddsd.lpSurface = data;
    ddsd.dwFlags = DDSD_HEIGHT | DDSD_LPSURFACE;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(SUCCEEDED(hr), "Failed to set surface desc, hr %#x.\n", hr);

    ddsd.dwHeight = 0;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting height=0 returned %#x.\n", hr);

    reset_ddsd(&ddsd);
    hr = IDirectDrawSurface4_GetSurfaceDesc(surface, &ddsd);
    ok(SUCCEEDED(hr), "GetSurfaceDesc failed, hr %#x.\n", hr);
    ok(ddsd.dwWidth == 8, "SetSurfaceDesc: Expected width 8, got %u.\n", ddsd.dwWidth);
    ok(ddsd.dwHeight == 16, "SetSurfaceDesc: Expected height 16, got %u.\n", ddsd.dwHeight);

    /* Pitch and width can be set, but only together, and only with LPSURFACE. They must not be 0 */
    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_PITCH;
    U1(ddsd).lPitch = 8 * 4;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting pitch without lpSurface or width returned %#x.\n", hr);

    ddsd.dwFlags = DDSD_WIDTH;
    ddsd.dwWidth = 16;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting width without lpSurface or pitch returned %#x.\n", hr);

    ddsd.dwFlags = DDSD_PITCH | DDSD_LPSURFACE;
    ddsd.lpSurface = data;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting pitch and lpSurface without width returned %#x.\n", hr);

    ddsd.dwFlags = DDSD_WIDTH | DDSD_LPSURFACE;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting width and lpSurface without pitch returned %#x.\n", hr);

    ddsd.dwFlags = DDSD_WIDTH | DDSD_PITCH | DDSD_LPSURFACE;
    U1(ddsd).lPitch = 16 * 4;
    ddsd.dwWidth = 16;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(SUCCEEDED(hr), "Failed to set surface desc, hr %#x.\n", hr);

    reset_ddsd(&ddsd);
    hr = IDirectDrawSurface4_GetSurfaceDesc(surface, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == 16, "SetSurfaceDesc: Expected width 8, got %u.\n", ddsd.dwWidth);
    ok(ddsd.dwHeight == 16, "SetSurfaceDesc: Expected height 16, got %u.\n", ddsd.dwHeight);
    ok(U1(ddsd).lPitch == 16 * 4, "SetSurfaceDesc: Expected pitch 64, got %u.\n", U1(ddsd).lPitch);

    /* The pitch must be 32 bit aligned and > 0, but is not verified for sanity otherwise.
     *
     * VMware rejects those calls, but all real drivers accept it. Mark the VMware behavior broken. */
    ddsd.dwFlags = DDSD_WIDTH | DDSD_PITCH | DDSD_LPSURFACE;
    U1(ddsd).lPitch = 4 * 4;
    ddsd.lpSurface = data;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(SUCCEEDED(hr) || broken(hr == DDERR_INVALIDPARAMS), "Failed to set surface desc, hr %#x.\n", hr);

    U1(ddsd).lPitch = 4;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(SUCCEEDED(hr) || broken(hr == DDERR_INVALIDPARAMS), "Failed to set surface desc, hr %#x.\n", hr);

    U1(ddsd).lPitch = 16 * 4 + 1;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting misaligned pitch returned %#x.\n", hr);

    U1(ddsd).lPitch = 16 * 4 + 3;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting misaligned pitch returned %#x.\n", hr);

    U1(ddsd).lPitch = -4;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting negative pitch returned %#x.\n", hr);

    U1(ddsd).lPitch = 16 * 4;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(SUCCEEDED(hr), "Failed to set surface desc, hr %#x.\n", hr);

    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_PITCH | DDSD_LPSURFACE;
    U1(ddsd).lPitch = 0;
    ddsd.dwWidth = 16;
    ddsd.lpSurface = data;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting zero pitch returned %#x.\n", hr);

    ddsd.dwFlags = DDSD_WIDTH | DDSD_PITCH | DDSD_LPSURFACE;
    U1(ddsd).lPitch = 16 * 4;
    ddsd.dwWidth = 0;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting zero width returned %#x.\n", hr);

    /* Setting the pixelformat without LPSURFACE is an error, but with LPSURFACE it works. */
    ddsd.dwFlags = DDSD_PIXELFORMAT;
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 32;
    U2(U4(ddsd).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(U4(ddsd).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(U4(ddsd).ddpfPixelFormat).dwBBitMask = 0x000000ff;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting the pixel format returned %#x.\n", hr);

    ddsd.dwFlags = DDSD_PIXELFORMAT | DDSD_LPSURFACE;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(SUCCEEDED(hr), "Failed to set surface desc, hr %#x.\n", hr);

    /* Can't set color keys. */
    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_CKSRCBLT;
    ddsd.ddckCKSrcBlt.dwColorSpaceLowValue = 0x00ff0000;
    ddsd.ddckCKSrcBlt.dwColorSpaceHighValue = 0x00ff0000;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting ddckCKSrcBlt returned %#x.\n", hr);

    ddsd.dwFlags = DDSD_CKSRCBLT | DDSD_LPSURFACE;
    ddsd.lpSurface = data;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting ddckCKSrcBlt returned %#x.\n", hr);

    IDirectDrawSurface4_Release(surface);

    /* SetSurfaceDesc needs systemmemory surfaces.
     *
     * As a sidenote, fourcc surfaces aren't allowed in sysmem, thus testing DDSD_LINEARSIZE is moot. */
    for (i = 0; i < sizeof(invalid_caps_tests) / sizeof(*invalid_caps_tests); i++)
    {
        reset_ddsd(&ddsd);
        ddsd.dwFlags = DDSD_CAPS;
        ddsd.ddsCaps.dwCaps = invalid_caps_tests[i].caps;
        ddsd.ddsCaps.dwCaps2 = invalid_caps_tests[i].caps2;
        if (!(invalid_caps_tests[i].caps & DDSCAPS_PRIMARYSURFACE))
        {
            ddsd.dwFlags |= DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
            ddsd.dwWidth = 8;
            ddsd.dwHeight = 8;
            U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
            U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB;
            U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 32;
            U2(U4(ddsd).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
            U3(U4(ddsd).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
            U4(U4(ddsd).ddpfPixelFormat).dwBBitMask = 0x000000ff;
        }

        hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &surface, NULL);
        ok(SUCCEEDED(hr) || hr == DDERR_NODIRECTDRAWHW, "Failed to create surface, hr %#x.\n", hr);
        if (FAILED(hr))
        {
            skip("Cannot create a %s surface, skipping vidmem SetSurfaceDesc test.\n",
                    invalid_caps_tests[i].name);
            goto done;
        }

        reset_ddsd(&ddsd);
        ddsd.dwFlags = DDSD_LPSURFACE;
        ddsd.lpSurface = data;
        hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
        if (invalid_caps_tests[i].supported)
        {
            ok(SUCCEEDED(hr), "Failed to set surface desc, hr %#x.\n", hr);
        }
        else
        {
            ok(hr == DDERR_INVALIDSURFACETYPE, "SetSurfaceDesc on a %s surface returned %#x.\n",
                    invalid_caps_tests[i].name, hr);

            /* Check priority of error conditions. */
            ddsd.dwFlags = DDSD_WIDTH;
            hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
            ok(hr == DDERR_INVALIDSURFACETYPE, "SetSurfaceDesc on a %s surface returned %#x.\n",
                    invalid_caps_tests[i].name, hr);
        }

        IDirectDrawSurface4_Release(surface);
    }

done:
    ref = IDirectDraw4_Release(ddraw);
    ok(ref == 0, "Ddraw object not properly released, refcount %u.\n", ref);
    DestroyWindow(window);
}

static void test_user_memory_getdc(void)
{
    IDirectDraw4 *ddraw;
    HWND window;
    HRESULT hr;
    DDSURFACEDESC2 ddsd;
    IDirectDrawSurface4 *surface;
    DWORD data[16][16];
    ULONG ref;
    HDC dc;
    unsigned int x, y;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT;
    ddsd.dwWidth = 16;
    ddsd.dwHeight = 16;
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 32;
    U2(U4(ddsd).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(U4(ddsd).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(U4(ddsd).ddpfPixelFormat).dwBBitMask = 0x000000ff;
    ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;
    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    memset(data, 0xaa, sizeof(data));
    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_LPSURFACE;
    ddsd.lpSurface = data;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(SUCCEEDED(hr), "Failed to set surface desc, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_GetDC(surface, &dc);
    ok(SUCCEEDED(hr), "Failed to get DC, hr %#x.\n", hr);
    BitBlt(dc, 0, 0, 16, 8, NULL, 0, 0, WHITENESS);
    BitBlt(dc, 0, 8, 16, 8, NULL, 0, 0, BLACKNESS);
    hr = IDirectDrawSurface4_ReleaseDC(surface, dc);
    ok(SUCCEEDED(hr), "Failed to release DC, hr %#x.\n", hr);

    ok(data[0][0] == 0xffffffff, "Expected color 0xffffffff, got %#x.\n", data[0][0]);
    ok(data[15][15] == 0x00000000, "Expected color 0x00000000, got %#x.\n", data[15][15]);

    ddsd.dwFlags = DDSD_LPSURFACE | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH;
    ddsd.lpSurface = data;
    ddsd.dwWidth = 4;
    ddsd.dwHeight = 8;
    U1(ddsd).lPitch = sizeof(*data);
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(SUCCEEDED(hr), "Failed to set surface desc, hr %#x.\n", hr);

    memset(data, 0xaa, sizeof(data));
    hr = IDirectDrawSurface4_GetDC(surface, &dc);
    ok(SUCCEEDED(hr), "Failed to get DC, hr %#x.\n", hr);
    BitBlt(dc, 0, 0, 4, 8, NULL, 0, 0, BLACKNESS);
    BitBlt(dc, 1, 1, 2, 2, NULL, 0, 0, WHITENESS);
    hr = IDirectDrawSurface4_ReleaseDC(surface, dc);
    ok(SUCCEEDED(hr), "Failed to release DC, hr %#x.\n", hr);

    for (y = 0; y < 4; y++)
    {
        for (x = 0; x < 4; x++)
        {
            if ((x == 1 || x == 2) && (y == 1 || y == 2))
                ok(data[y][x] == 0xffffffff, "Expected color 0xffffffff on position %ux%u, got %#x.\n",
                        x, y, data[y][x]);
            else
                ok(data[y][x] == 0x00000000, "Expected color 0x00000000 on position %ux%u, got %#x.\n",
                        x, y, data[y][x]);
        }
    }
    ok(data[0][5] == 0xaaaaaaaa, "Expected color 0xaaaaaaaa on position 5x0, got %#x.\n",
            data[0][5]);
    ok(data[7][3] == 0x00000000, "Expected color 0x00000000 on position 3x7, got %#x.\n",
            data[7][3]);
    ok(data[7][4] == 0xaaaaaaaa, "Expected color 0xaaaaaaaa on position 4x7, got %#x.\n",
            data[7][4]);
    ok(data[8][0] == 0xaaaaaaaa, "Expected color 0xaaaaaaaa on position 0x8, got %#x.\n",
            data[8][0]);

    IDirectDrawSurface4_Release(surface);
    ref = IDirectDraw4_Release(ddraw);
    ok(ref == 0, "Ddraw object not properly released, refcount %u.\n", ref);
    DestroyWindow(window);
}

static void test_sysmem_overlay(void)
{
    IDirectDraw4 *ddraw;
    HWND window;
    HRESULT hr;
    DDSURFACEDESC2 ddsd;
    IDirectDrawSurface4 *surface;
    ULONG ref;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.dwWidth = 16;
    ddsd.dwHeight = 16;
    ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OVERLAY;
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 32;
    U2(U4(ddsd).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(U4(ddsd).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(U4(ddsd).ddpfPixelFormat).dwBBitMask = 0x000000ff;
    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &surface, NULL);
    ok(hr == DDERR_NOOVERLAYHW, "Got unexpected hr %#x.\n", hr);

    ref = IDirectDraw4_Release(ddraw);
    ok(ref == 0, "Ddraw object not properly released, refcount %u.\n", ref);
    DestroyWindow(window);
}

static void test_primary_palette(void)
{
    DDSCAPS2 surface_caps = {DDSCAPS_FLIP, 0, 0, 0};
    IDirectDrawSurface4 *primary, *backbuffer;
    PALETTEENTRY palette_entries[256];
    IDirectDrawPalette *palette, *tmp;
    DDSURFACEDESC2 surface_desc;
    IDirectDraw4 *ddraw;
    DWORD palette_caps;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (FAILED(hr = IDirectDraw4_SetDisplayMode(ddraw, 640, 480, 8, 0, 0)))
    {
        win_skip("Failed to set 8 bpp display mode, skipping test.\n");
        IDirectDraw4_Release(ddraw);
        DestroyWindow(window);
        return;
    }
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;
    surface_desc.dwBackBufferCount = 1;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetAttachedSurface(primary, &surface_caps, &backbuffer);
    ok(SUCCEEDED(hr), "Failed to get attached surface, hr %#x.\n", hr);

    memset(palette_entries, 0, sizeof(palette_entries));
    hr = IDirectDraw4_CreatePalette(ddraw, DDPCAPS_8BIT | DDPCAPS_ALLOW256, palette_entries, &palette, NULL);
    ok(SUCCEEDED(hr), "Failed to create palette, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)palette);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);

    hr = IDirectDrawPalette_GetCaps(palette, &palette_caps);
    ok(SUCCEEDED(hr), "Failed to get palette caps, hr %#x.\n", hr);
    ok(palette_caps == (DDPCAPS_8BIT | DDPCAPS_ALLOW256), "Got unexpected palette caps %#x.\n", palette_caps);

    hr = IDirectDrawSurface4_SetPalette(primary, palette);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);

    /* The Windows 8 testbot attaches the palette to the backbuffer as well,
     * and is generally somewhat broken with respect to 8 bpp / palette
     * handling. */
    if (SUCCEEDED(IDirectDrawSurface4_GetPalette(backbuffer, &tmp)))
    {
        win_skip("Broken palette handling detected, skipping tests.\n");
        IDirectDrawPalette_Release(tmp);
        IDirectDrawPalette_Release(palette);
        /* The Windows 8 testbot keeps extra references to the primary and
         * backbuffer while in 8 bpp mode. */
        hr = IDirectDraw4_RestoreDisplayMode(ddraw);
        ok(SUCCEEDED(hr), "Failed to restore display mode, hr %#x.\n", hr);
        goto done;
    }

    refcount = get_refcount((IUnknown *)palette);
    ok(refcount == 2, "Got unexpected refcount %u.\n", refcount);

    hr = IDirectDrawPalette_GetCaps(palette, &palette_caps);
    ok(SUCCEEDED(hr), "Failed to get palette caps, hr %#x.\n", hr);
    ok(palette_caps == (DDPCAPS_8BIT | DDPCAPS_PRIMARYSURFACE | DDPCAPS_ALLOW256),
            "Got unexpected palette caps %#x.\n", palette_caps);

    hr = IDirectDrawSurface4_SetPalette(primary, NULL);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)palette);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);

    hr = IDirectDrawPalette_GetCaps(palette, &palette_caps);
    ok(SUCCEEDED(hr), "Failed to get palette caps, hr %#x.\n", hr);
    ok(palette_caps == (DDPCAPS_8BIT | DDPCAPS_ALLOW256), "Got unexpected palette caps %#x.\n", palette_caps);

    hr = IDirectDrawSurface4_SetPalette(primary, palette);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)palette);
    ok(refcount == 2, "Got unexpected refcount %u.\n", refcount);

    hr = IDirectDrawSurface4_GetPalette(primary, &tmp);
    ok(SUCCEEDED(hr), "Failed to get palette, hr %#x.\n", hr);
    ok(tmp == palette, "Got unexpected palette %p, expected %p.\n", tmp, palette);
    IDirectDrawPalette_Release(tmp);
    hr = IDirectDrawSurface4_GetPalette(backbuffer, &tmp);
    ok(hr == DDERR_NOPALETTEATTACHED, "Got unexpected hr %#x.\n", hr);

    refcount = IDirectDrawPalette_Release(palette);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDrawPalette_Release(palette);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    /* Note that this only seems to work when the palette is attached to the
     * primary surface. When attached to a regular surface, attempting to get
     * the palette here will cause an access violation. */
    hr = IDirectDrawSurface4_GetPalette(primary, &tmp);
    ok(hr == DDERR_NOPALETTEATTACHED, "Got unexpected hr %#x.\n", hr);

done:
    refcount = IDirectDrawSurface4_Release(backbuffer);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDrawSurface4_Release(primary);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDraw4_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

static HRESULT WINAPI surface_counter(IDirectDrawSurface4 *surface, DDSURFACEDESC2 *desc, void *context)
{
    UINT *surface_count = context;

    ++(*surface_count);
    IDirectDrawSurface_Release(surface);

    return DDENUMRET_OK;
}

static void test_surface_attachment(void)
{
    IDirectDrawSurface4 *surface1, *surface2, *surface3, *surface4;
    IDirectDrawSurface *surface1v1, *surface2v1;
    DDSCAPS2 caps = {DDSCAPS_TEXTURE, 0, 0, 0};
    DDSURFACEDESC2 surface_desc;
    IDirectDraw4 *ddraw;
    UINT surface_count;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_MIPMAPCOUNT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
    U2(surface_desc).dwMipMapCount = 3;
    surface_desc.dwWidth = 128;
    surface_desc.dwHeight = 128;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface1, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_GetAttachedSurface(surface1, &caps, &surface2);
    ok(SUCCEEDED(hr), "Failed to get mip level, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetAttachedSurface(surface2, &caps, &surface3);
    ok(SUCCEEDED(hr), "Failed to get mip level, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetAttachedSurface(surface3, &caps, &surface4);
    ok(hr == DDERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);

    surface_count = 0;
    IDirectDrawSurface4_EnumAttachedSurfaces(surface1, &surface_count, surface_counter);
    ok(surface_count == 1, "Got unexpected surface_count %u.\n", surface_count);
    surface_count = 0;
    IDirectDrawSurface4_EnumAttachedSurfaces(surface2, &surface_count, surface_counter);
    ok(surface_count == 1, "Got unexpected surface_count %u.\n", surface_count);
    surface_count = 0;
    IDirectDrawSurface4_EnumAttachedSurfaces(surface3, &surface_count, surface_counter);
    ok(!surface_count, "Got unexpected surface_count %u.\n", surface_count);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    surface_desc.dwWidth = 16;
    surface_desc.dwHeight = 16;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface4, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_AddAttachedSurface(surface1, surface4);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_AddAttachedSurface(surface4, surface1);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_AddAttachedSurface(surface3, surface4);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_AddAttachedSurface(surface4, surface3);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_AddAttachedSurface(surface2, surface4);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_AddAttachedSurface(surface4, surface2);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);

    IDirectDrawSurface4_Release(surface4);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;
    surface_desc.dwWidth = 16;
    surface_desc.dwHeight = 16;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface4, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    if (SUCCEEDED(hr = IDirectDrawSurface4_AddAttachedSurface(surface1, surface4)))
    {
        skip("Running on refrast, skipping some tests.\n");
        hr = IDirectDrawSurface4_DeleteAttachedSurface(surface1, 0, surface4);
        ok(SUCCEEDED(hr), "Failed to detach surface, hr %#x.\n", hr);
    }
    else
    {
        ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
        hr = IDirectDrawSurface4_AddAttachedSurface(surface4, surface1);
        ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
        hr = IDirectDrawSurface4_AddAttachedSurface(surface3, surface4);
        ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
        hr = IDirectDrawSurface4_AddAttachedSurface(surface4, surface3);
        ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
        hr = IDirectDrawSurface4_AddAttachedSurface(surface2, surface4);
        ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
        hr = IDirectDrawSurface4_AddAttachedSurface(surface4, surface2);
        ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
    }

    IDirectDrawSurface4_Release(surface4);
    IDirectDrawSurface4_Release(surface3);
    IDirectDrawSurface4_Release(surface2);
    IDirectDrawSurface4_Release(surface1);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    /* Try a single primary and two offscreen plain surfaces. */
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface1, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    surface_desc.dwWidth = GetSystemMetrics(SM_CXSCREEN);
    surface_desc.dwHeight = GetSystemMetrics(SM_CYSCREEN);
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface2, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    surface_desc.dwWidth = GetSystemMetrics(SM_CXSCREEN);
    surface_desc.dwHeight = GetSystemMetrics(SM_CYSCREEN);
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface3, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    /* This one has a different size. */
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    surface_desc.dwWidth = 128;
    surface_desc.dwHeight = 128;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface4, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_AddAttachedSurface(surface1, surface2);
    ok(SUCCEEDED(hr), "Failed to attach surface, hr %#x.\n", hr);
    /* Try the reverse without detaching first. */
    hr = IDirectDrawSurface4_AddAttachedSurface(surface2, surface1);
    ok(hr == DDERR_SURFACEALREADYATTACHED, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_DeleteAttachedSurface(surface1, 0, surface2);
    ok(SUCCEEDED(hr), "Failed to detach surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_AddAttachedSurface(surface2, surface1);
    ok(SUCCEEDED(hr), "Failed to attach surface, hr %#x.\n", hr);
    /* Try to detach reversed. */
    hr = IDirectDrawSurface4_DeleteAttachedSurface(surface1, 0, surface2);
    ok(hr == DDERR_CANNOTDETACHSURFACE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_DeleteAttachedSurface(surface2, 0, surface1);
    ok(SUCCEEDED(hr), "Failed to detach surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_AddAttachedSurface(surface2, surface3);
    ok(SUCCEEDED(hr), "Failed to attach surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_DeleteAttachedSurface(surface2, 0, surface3);
    ok(SUCCEEDED(hr), "Failed to detach surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_AddAttachedSurface(surface1, surface4);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_AddAttachedSurface(surface4, surface1);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);

    IDirectDrawSurface4_Release(surface4);
    IDirectDrawSurface4_Release(surface3);
    IDirectDrawSurface4_Release(surface2);
    IDirectDrawSurface4_Release(surface1);

    /* Test DeleteAttachedSurface() and automatic detachment of attached surfaces on release. */
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface_desc.dwWidth = 64;
    surface_desc.dwHeight = 64;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_RGB; /* D3DFMT_R5G6B5 */
    U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 16;
    U2(U4(surface_desc).ddpfPixelFormat).dwRBitMask = 0xf800;
    U3(U4(surface_desc).ddpfPixelFormat).dwGBitMask = 0x07e0;
    U4(U4(surface_desc).ddpfPixelFormat).dwBBitMask = 0x001f;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface1, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface3, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    surface_desc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
    U1(U4(surface_desc).ddpfPixelFormat).dwZBufferBitDepth = 16;
    U3(U4(surface_desc).ddpfPixelFormat).dwZBitMask = 0x0000ffff;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface2, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_QueryInterface(surface1, &IID_IDirectDrawSurface, (void **)&surface1v1);
    ok(SUCCEEDED(hr), "Failed to get interface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_QueryInterface(surface2, &IID_IDirectDrawSurface, (void **)&surface2v1);
    ok(SUCCEEDED(hr), "Failed to get interface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_AddAttachedSurface(surface1, surface2);
    ok(SUCCEEDED(hr), "Failed to attach surface, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)surface2);
    ok(refcount == 2, "Got unexpected refcount %u.\n", refcount);
    refcount = get_refcount((IUnknown *)surface2v1);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    hr = IDirectDrawSurface4_AddAttachedSurface(surface1, surface2);
    ok(hr == DDERR_SURFACEALREADYATTACHED, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_AddAttachedSurface(surface1v1, surface2v1);
    todo_wine ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_DeleteAttachedSurface(surface1v1, 0, surface2v1);
    ok(hr == DDERR_SURFACENOTATTACHED, "Got unexpected hr %#x.\n", hr);

    /* Attaching while already attached to other surface. */
    hr = IDirectDrawSurface4_AddAttachedSurface(surface3, surface2);
    todo_wine ok(SUCCEEDED(hr), "Failed to attach surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_DeleteAttachedSurface(surface3, 0, surface2);
    todo_wine ok(SUCCEEDED(hr), "Failed to detach surface, hr %#x.\n", hr);
    IDirectDrawSurface4_Release(surface3);

    hr = IDirectDrawSurface4_DeleteAttachedSurface(surface1, 0, surface2);
    ok(SUCCEEDED(hr), "Failed to detach surface, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)surface2);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    refcount = get_refcount((IUnknown *)surface2v1);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);

    /* DeleteAttachedSurface() when attaching via IDirectDrawSurface. */
    hr = IDirectDrawSurface_AddAttachedSurface(surface1v1, surface2v1);
    ok(SUCCEEDED(hr), "Failed to attach surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_DeleteAttachedSurface(surface1, 0, surface2);
    ok(hr == DDERR_SURFACENOTATTACHED, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_DeleteAttachedSurface(surface1v1, 0, surface2v1);
    ok(SUCCEEDED(hr), "Failed to detach surface, hr %#x.\n", hr);
    refcount = IDirectDrawSurface4_Release(surface2);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDrawSurface4_Release(surface1);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    /* Automatic detachment on release. */
    hr = IDirectDrawSurface_AddAttachedSurface(surface1v1, surface2v1);
    ok(SUCCEEDED(hr), "Failed to attach surface, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)surface2v1);
    ok(refcount == 2, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDrawSurface_Release(surface1v1);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDrawSurface_Release(surface2v1);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDraw4_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_private_data(void)
{
    IDirectDraw4 *ddraw;
    IDirectDrawSurface4 *surface, *surface2;
    DDSURFACEDESC2 surface_desc;
    ULONG refcount, refcount2, refcount3;
    IUnknown *ptr;
    DWORD size = sizeof(ptr);
    HRESULT hr;
    HWND window;
    DDSCAPS2 caps = {DDSCAPS_COMPLEX, 0, 0, 0};
    DWORD data[] = {1, 2, 3, 4};
    DDCAPS hal_caps;
    static const GUID ddraw_private_data_test_guid =
    {
        0xfdb37466,
        0x428f,
        0x4edf,
        {0xa3,0x7f,0x9b,0x1d,0xf4,0x88,0xc5,0xfc}
    };
    static const GUID ddraw_private_data_test_guid2 =
    {
        0x2e5afac2,
        0x87b5,
        0x4c10,
        {0x9b,0x4b,0x89,0xd7,0xd1,0x12,0xe7,0x2b}
    };

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    reset_ddsd(&surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    surface_desc.ddsCaps.dwCaps |= DDSCAPS_OFFSCREENPLAIN;
    surface_desc.dwHeight = 4;
    surface_desc.dwWidth = 4;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    /* NULL pointers are not valid, but don't cause a crash. */
    hr = IDirectDrawSurface7_SetPrivateData(surface, &ddraw_private_data_test_guid, NULL,
            sizeof(IUnknown *), DDSPD_IUNKNOWNPOINTER);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface7_SetPrivateData(surface, &ddraw_private_data_test_guid, NULL, 0, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface7_SetPrivateData(surface, &ddraw_private_data_test_guid, NULL, 1, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    /* DDSPD_IUNKNOWNPOINTER needs sizeof(IUnknown *) bytes of data. */
    hr = IDirectDrawSurface4_SetPrivateData(surface, &ddraw_private_data_test_guid, ddraw,
            0, DDSPD_IUNKNOWNPOINTER);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_SetPrivateData(surface, &ddraw_private_data_test_guid, ddraw,
            5, DDSPD_IUNKNOWNPOINTER);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_SetPrivateData(surface, &ddraw_private_data_test_guid, ddraw,
            sizeof(ddraw) * 2, DDSPD_IUNKNOWNPOINTER);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    /* Note that with a size != 0 and size != sizeof(IUnknown *) and
     * DDSPD_IUNKNOWNPOINTER set SetPrivateData in ddraw4 and ddraw7
     * erases the old content and returns an error. This behavior has
     * been fixed in d3d8 and d3d9. Unless an application is found
     * that depends on this we don't care about this behavior. */
    hr = IDirectDrawSurface4_SetPrivateData(surface, &ddraw_private_data_test_guid, ddraw,
            sizeof(ddraw), DDSPD_IUNKNOWNPOINTER);
    ok(SUCCEEDED(hr), "Failed to set private data, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_SetPrivateData(surface, &ddraw_private_data_test_guid, ddraw,
            0, DDSPD_IUNKNOWNPOINTER);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    size = sizeof(ptr);
    hr = IDirectDrawSurface4_GetPrivateData(surface, &ddraw_private_data_test_guid, &ptr, &size);
    ok(SUCCEEDED(hr), "Failed to get private data, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_FreePrivateData(surface, &ddraw_private_data_test_guid);
    ok(SUCCEEDED(hr), "Failed to free private data, hr %#x.\n", hr);

    refcount = get_refcount((IUnknown *)ddraw);
    hr = IDirectDrawSurface4_SetPrivateData(surface, &ddraw_private_data_test_guid, ddraw,
            sizeof(ddraw), DDSPD_IUNKNOWNPOINTER);
    ok(SUCCEEDED(hr), "Failed to set private data, hr %#x.\n", hr);
    refcount2 = get_refcount((IUnknown *)ddraw);
    ok(refcount2 == refcount + 1, "Got unexpected refcount %u.\n", refcount2);

    hr = IDirectDrawSurface4_FreePrivateData(surface, &ddraw_private_data_test_guid);
    ok(SUCCEEDED(hr), "Failed to free private data, hr %#x.\n", hr);
    refcount2 = get_refcount((IUnknown *)ddraw);
    ok(refcount2 == refcount, "Got unexpected refcount %u.\n", refcount2);

    hr = IDirectDrawSurface4_SetPrivateData(surface, &ddraw_private_data_test_guid, ddraw,
            sizeof(ddraw), DDSPD_IUNKNOWNPOINTER);
    ok(SUCCEEDED(hr), "Failed to set private data, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_SetPrivateData(surface, &ddraw_private_data_test_guid, surface,
            sizeof(surface), DDSPD_IUNKNOWNPOINTER);
    ok(SUCCEEDED(hr), "Failed to set private data, hr %#x.\n", hr);
    refcount2 = get_refcount((IUnknown *)ddraw);
    ok(refcount2 == refcount, "Got unexpected refcount %u.\n", refcount2);

    hr = IDirectDrawSurface4_SetPrivateData(surface, &ddraw_private_data_test_guid, ddraw,
            sizeof(ddraw), DDSPD_IUNKNOWNPOINTER);
    ok(SUCCEEDED(hr), "Failed to set private data, hr %#x.\n", hr);
    size = 2 * sizeof(ptr);
    hr = IDirectDrawSurface4_GetPrivateData(surface, &ddraw_private_data_test_guid, &ptr, &size);
    ok(SUCCEEDED(hr), "Failed to get private data, hr %#x.\n", hr);
    ok(size == sizeof(ddraw), "Got unexpected size %u.\n", size);
    refcount2 = get_refcount(ptr);
    /* Object is NOT addref'ed by the getter. */
    ok(ptr == (IUnknown *)ddraw, "Returned interface pointer is %p, expected %p.\n", ptr, ddraw);
    ok(refcount2 == refcount + 1, "Got unexpected refcount %u.\n", refcount2);

    ptr = (IUnknown *)0xdeadbeef;
    size = 1;
    hr = IDirectDrawSurface4_GetPrivateData(surface, &ddraw_private_data_test_guid, NULL, &size);
    ok(hr == DDERR_MOREDATA, "Got unexpected hr %#x.\n", hr);
    ok(size == sizeof(ddraw), "Got unexpected size %u.\n", size);
    size = 2 * sizeof(ptr);
    hr = IDirectDrawSurface4_GetPrivateData(surface, &ddraw_private_data_test_guid, NULL, &size);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    ok(size == 2 * sizeof(ptr), "Got unexpected size %u.\n", size);
    size = 1;
    hr = IDirectDrawSurface4_GetPrivateData(surface, &ddraw_private_data_test_guid, &ptr, &size);
    ok(hr == DDERR_MOREDATA, "Got unexpected hr %#x.\n", hr);
    ok(size == sizeof(ddraw), "Got unexpected size %u.\n", size);
    ok(ptr == (IUnknown *)0xdeadbeef, "Got unexpected pointer %p.\n", ptr);
    hr = IDirectDrawSurface4_GetPrivateData(surface, &ddraw_private_data_test_guid2, NULL, NULL);
    ok(hr == DDERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    size = 0xdeadbabe;
    hr = IDirectDrawSurface4_GetPrivateData(surface, &ddraw_private_data_test_guid2, &ptr, &size);
    ok(hr == DDERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    ok(ptr == (IUnknown *)0xdeadbeef, "Got unexpected pointer %p.\n", ptr);
    ok(size == 0xdeadbabe, "Got unexpected size %u.\n", size);
    hr = IDirectDrawSurface4_GetPrivateData(surface, &ddraw_private_data_test_guid, NULL, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    refcount3 = IDirectDrawSurface4_Release(surface);
    ok(!refcount3, "Got unexpected refcount %u.\n", refcount3);

    /* Destroying the surface frees the reference held on the private data. It also frees
     * the reference the surface is holding on its creating object. */
    refcount2 = get_refcount((IUnknown *)ddraw);
    ok(refcount2 == refcount - 1, "Got unexpected refcount %u.\n", refcount2);

    memset(&hal_caps, 0, sizeof(hal_caps));
    hal_caps.dwSize = sizeof(hal_caps);
    hr = IDirectDraw7_GetCaps(ddraw, &hal_caps, NULL);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    if ((hal_caps.ddsCaps.dwCaps & (DDSCAPS_TEXTURE | DDSCAPS_MIPMAP)) == (DDSCAPS_TEXTURE | DDSCAPS_MIPMAP))
    {
        reset_ddsd(&surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_MIPMAPCOUNT;
        surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
        surface_desc.dwHeight = 4;
        surface_desc.dwWidth = 4;
        U2(surface_desc).dwMipMapCount = 2;
        hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
        ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
        hr = IDirectDrawSurface4_GetAttachedSurface(surface, &caps, &surface2);
        ok(SUCCEEDED(hr), "Failed to get attached surface, hr %#x.\n", hr);

        hr = IDirectDrawSurface4_SetPrivateData(surface, &ddraw_private_data_test_guid, data, sizeof(data), 0);
        ok(SUCCEEDED(hr), "Failed to set private data, hr %#x.\n", hr);
        hr = IDirectDrawSurface4_GetPrivateData(surface2, &ddraw_private_data_test_guid, NULL, NULL);
        ok(hr == DDERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);

        IDirectDrawSurface4_Release(surface2);
        IDirectDrawSurface4_Release(surface);
    }
    else
        skip("Mipmapped textures not supported, skipping mipmap private data test.\n");

    refcount = IDirectDraw4_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_pixel_format(void)
{
    HWND window, window2 = NULL;
    HDC hdc, hdc2 = NULL;
    HMODULE gl = NULL;
    int format, test_format;
    PIXELFORMATDESCRIPTOR pfd;
    IDirectDraw4 *ddraw = NULL;
    IDirectDrawClipper *clipper = NULL;
    DDSURFACEDESC2 ddsd;
    IDirectDrawSurface4 *primary = NULL;
    DDBLTFX fx;
    HRESULT hr;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            100, 100, 160, 160, NULL, NULL, NULL, NULL);
    if (!window)
    {
        skip("Failed to create window\n");
        return;
    }

    window2 = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            100, 100, 160, 160, NULL, NULL, NULL, NULL);

    hdc = GetDC(window);
    if (!hdc)
    {
        skip("Failed to get DC\n");
        goto cleanup;
    }

    if (window2)
        hdc2 = GetDC(window2);

    gl = LoadLibraryA("opengl32.dll");
    ok(!!gl, "failed to load opengl32.dll; SetPixelFormat()/GetPixelFormat() may not work right\n");

    format = GetPixelFormat(hdc);
    ok(format == 0, "new window has pixel format %d\n", format);

    ZeroMemory(&pfd, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.iLayerType = PFD_MAIN_PLANE;
    format = ChoosePixelFormat(hdc, &pfd);
    if (format <= 0)
    {
        skip("no pixel format available\n");
        goto cleanup;
    }

    if (!SetPixelFormat(hdc, format, &pfd) || GetPixelFormat(hdc) != format)
    {
        skip("failed to set pixel format\n");
        goto cleanup;
    }

    if (!hdc2 || !SetPixelFormat(hdc2, format, &pfd) || GetPixelFormat(hdc2) != format)
    {
        skip("failed to set pixel format on second window\n");
        if (hdc2)
        {
            ReleaseDC(window2, hdc2);
            hdc2 = NULL;
        }
    }

    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    test_format = GetPixelFormat(hdc);
    ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    if (FAILED(hr))
    {
        skip("Failed to set cooperative level, hr %#x.\n", hr);
        goto cleanup;
    }

    test_format = GetPixelFormat(hdc);
    ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

    if (hdc2)
    {
        hr = IDirectDraw4_CreateClipper(ddraw, 0, &clipper, NULL);
        ok(SUCCEEDED(hr), "Failed to create clipper, hr %#x.\n", hr);
        hr = IDirectDrawClipper_SetHWnd(clipper, 0, window2);
        ok(SUCCEEDED(hr), "Failed to set clipper window, hr %#x.\n", hr);

        test_format = GetPixelFormat(hdc);
        ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

        test_format = GetPixelFormat(hdc2);
        ok(test_format == format, "second window has pixel format %d, expected %d\n", test_format, format);
    }

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);

    test_format = GetPixelFormat(hdc);
    ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

    if (hdc2)
    {
        test_format = GetPixelFormat(hdc2);
        ok(test_format == format, "second window has pixel format %d, expected %d\n", test_format, format);
    }

    if (clipper)
    {
        hr = IDirectDrawSurface4_SetClipper(primary, clipper);
        ok(SUCCEEDED(hr), "Failed to set clipper, hr %#x.\n", hr);

        test_format = GetPixelFormat(hdc);
        ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

        test_format = GetPixelFormat(hdc2);
        ok(test_format == format, "second window has pixel format %d, expected %d\n", test_format, format);
    }

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    hr = IDirectDrawSurface4_Blt(primary, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear source surface, hr %#x.\n", hr);

    test_format = GetPixelFormat(hdc);
    ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

    if (hdc2)
    {
        test_format = GetPixelFormat(hdc2);
        ok(test_format == format, "second window has pixel format %d, expected %d\n", test_format, format);
    }

cleanup:
    if (primary) IDirectDrawSurface4_Release(primary);
    if (clipper) IDirectDrawClipper_Release(clipper);
    if (ddraw) IDirectDraw4_Release(ddraw);
    if (gl) FreeLibrary(gl);
    if (hdc) ReleaseDC(window, hdc);
    if (hdc2) ReleaseDC(window2, hdc2);
    if (window) DestroyWindow(window);
    if (window2) DestroyWindow(window2);
}

static void test_create_surface_pitch(void)
{
    IDirectDrawSurface4 *surface;
    DDSURFACEDESC2 surface_desc;
    IDirectDraw4 *ddraw;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    void *mem;

    static const struct
    {
        DWORD placement;
        DWORD flags_in;
        DWORD pitch_in;
        HRESULT hr;
        DWORD flags_out;
        DWORD pitch_out;
    }
    test_data[] =
    {
        {DDSCAPS_VIDEOMEMORY,   0,                              0,      DD_OK,
                                DDSD_PITCH,                     0x100},
        {DDSCAPS_VIDEOMEMORY,   DDSD_PITCH,                     0x104,  DD_OK,
                                DDSD_PITCH,                     0x100},
        {DDSCAPS_VIDEOMEMORY,   DDSD_PITCH,                     0x0f8,  DD_OK,
                                DDSD_PITCH,                     0x100},
        {DDSCAPS_VIDEOMEMORY,   DDSD_LPSURFACE | DDSD_PITCH,    0x100,  DDERR_INVALIDCAPS,
                                0,                              0    },
        {DDSCAPS_SYSTEMMEMORY,  0,                              0,      DD_OK,
                                DDSD_PITCH,                     0x100},
        {DDSCAPS_SYSTEMMEMORY,  DDSD_PITCH,                     0x104,  DD_OK,
                                DDSD_PITCH,                     0x100},
        {DDSCAPS_SYSTEMMEMORY,  DDSD_PITCH,                     0x0f8,  DD_OK,
                                DDSD_PITCH,                     0x100},
        {DDSCAPS_SYSTEMMEMORY,  DDSD_LPSURFACE,                 0,      DDERR_INVALIDPARAMS,
                                0,                              0    },
        {DDSCAPS_SYSTEMMEMORY,  DDSD_LPSURFACE | DDSD_PITCH,    0x100,  DD_OK,
                                DDSD_PITCH,                     0x100},
        {DDSCAPS_SYSTEMMEMORY,  DDSD_LPSURFACE | DDSD_PITCH,    0x0fe,  DDERR_INVALIDPARAMS,
                                0,                              0    },
        {DDSCAPS_SYSTEMMEMORY,  DDSD_LPSURFACE | DDSD_PITCH,    0x0fc,  DD_OK,
                                DDSD_PITCH,                     0x0fc},
        {DDSCAPS_SYSTEMMEMORY,  DDSD_LPSURFACE | DDSD_PITCH,    0x0f8,  DDERR_INVALIDPARAMS,
                                0,                              0    },
    };
    DWORD flags_mask = DDSD_PITCH | DDSD_LPSURFACE;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    mem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ((63 * 4) + 8) * 63);

    for (i = 0; i < sizeof(test_data) / sizeof(*test_data); ++i)
    {
        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | test_data[i].flags_in;
        surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | test_data[i].placement;
        surface_desc.dwWidth = 63;
        surface_desc.dwHeight = 63;
        U1(surface_desc).lPitch = test_data[i].pitch_in;
        surface_desc.lpSurface = mem;
        U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
        U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_RGB;
        U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 32;
        U2(U4(surface_desc).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
        U3(U4(surface_desc).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
        U4(U4(surface_desc).ddpfPixelFormat).dwBBitMask = 0x000000ff;
        hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
        ok(hr == test_data[i].hr || (test_data[i].placement == DDSCAPS_VIDEOMEMORY && hr == DDERR_NODIRECTDRAWHW),
                "Test %u: Got unexpected hr %#x, expected %#x.\n", i, hr, test_data[i].hr);
        if (FAILED(hr))
            continue;

        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        hr = IDirectDrawSurface4_GetSurfaceDesc(surface, &surface_desc);
        ok(SUCCEEDED(hr), "Test %u: Failed to get surface desc, hr %#x.\n", i, hr);
        ok((surface_desc.dwFlags & flags_mask) == test_data[i].flags_out,
                "Test %u: Got unexpected flags %#x, expected %#x.\n",
                i, surface_desc.dwFlags & flags_mask, test_data[i].flags_out);
        ok(U1(surface_desc).lPitch == test_data[i].pitch_out,
                "Test %u: Got unexpected pitch %u, expected %u.\n",
                i, U1(surface_desc).lPitch, test_data[i].pitch_out);

        IDirectDrawSurface4_Release(surface);
    }

    HeapFree(GetProcessHeap(), 0, mem);
    refcount = IDirectDraw4_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_mipmap_lock(void)
{
    IDirectDrawSurface4 *surface, *surface2;
    DDSURFACEDESC2 surface_desc;
    IDirectDraw4 *ddraw;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    DDSCAPS2 caps = {DDSCAPS_COMPLEX, 0, 0, 0};
    DDCAPS hal_caps;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&hal_caps, 0, sizeof(hal_caps));
    hal_caps.dwSize = sizeof(hal_caps);
    hr = IDirectDraw4_GetCaps(ddraw, &hal_caps, NULL);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    if ((hal_caps.ddsCaps.dwCaps & (DDSCAPS_TEXTURE | DDSCAPS_MIPMAP)) != (DDSCAPS_TEXTURE | DDSCAPS_MIPMAP))
    {
        skip("Mipmapped textures not supported, skipping mipmap lock test.\n");
        IDirectDraw4_Release(ddraw);
        DestroyWindow(window);
        return;
    }

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_MIPMAPCOUNT;
    surface_desc.dwWidth = 4;
    surface_desc.dwHeight = 4;
    U2(surface_desc).dwMipMapCount = 2;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP
            | DDSCAPS_SYSTEMMEMORY;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetAttachedSurface(surface, &caps, &surface2);
    ok(SUCCEEDED(hr), "Failed to get attached surface, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface4_Lock(surface, NULL, &surface_desc, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface4_Lock(surface2, NULL, &surface_desc, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    IDirectDrawSurface4_Unlock(surface2, NULL);
    IDirectDrawSurface4_Unlock(surface, NULL);

    IDirectDrawSurface4_Release(surface2);
    IDirectDrawSurface4_Release(surface);
    refcount = IDirectDraw4_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_palette_complex(void)
{
    IDirectDrawSurface4 *surface, *mipmap, *tmp;
    DDSURFACEDESC2 surface_desc;
    IDirectDraw4 *ddraw;
    IDirectDrawPalette *palette, *palette2, *palette_mipmap;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    DDSCAPS2 caps = {DDSCAPS_COMPLEX, 0, 0, 0};
    DDCAPS hal_caps;
    PALETTEENTRY palette_entries[256];
    unsigned int i;
    HDC dc;
    RGBQUAD rgbquad;
    UINT count;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&hal_caps, 0, sizeof(hal_caps));
    hal_caps.dwSize = sizeof(hal_caps);
    hr = IDirectDraw4_GetCaps(ddraw, &hal_caps, NULL);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    if ((hal_caps.ddsCaps.dwCaps & (DDSCAPS_TEXTURE | DDSCAPS_MIPMAP)) != (DDSCAPS_TEXTURE | DDSCAPS_MIPMAP))
    {
        skip("Mipmapped textures not supported, skipping mipmap palette test.\n");
        IDirectDraw4_Release(ddraw);
        DestroyWindow(window);
        return;
    }

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface_desc.dwWidth = 128;
    surface_desc.dwHeight = 128;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
    U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
    U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 8;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    memset(palette_entries, 0, sizeof(palette_entries));
    hr = IDirectDraw4_CreatePalette(ddraw, DDPCAPS_8BIT | DDPCAPS_ALLOW256,
            palette_entries, &palette, NULL);
    ok(SUCCEEDED(hr), "Failed to create palette, hr %#x.\n", hr);

    memset(palette_entries, 0, sizeof(palette_entries));
    palette_entries[1].peRed = 0xff;
    palette_entries[1].peGreen = 0x80;
    hr = IDirectDraw4_CreatePalette(ddraw, DDPCAPS_8BIT | DDPCAPS_ALLOW256,
            palette_entries, &palette_mipmap, NULL);
    ok(SUCCEEDED(hr), "Failed to create palette, hr %#x.\n", hr);

    palette2 = (void *)0xdeadbeef;
    hr = IDirectDrawSurface4_GetPalette(surface, &palette2);
    ok(hr == DDERR_NOPALETTEATTACHED, "Got unexpected hr %#x.\n", hr);
    ok(!palette2, "Got unexpected palette %p.\n", palette2);
    hr = IDirectDrawSurface4_SetPalette(surface, palette);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetPalette(surface, &palette2);
    ok(SUCCEEDED(hr), "Failed to get palette, hr %#x.\n", hr);
    ok(palette == palette2, "Got unexpected palette %p.\n", palette2);
    IDirectDrawPalette_Release(palette2);

    mipmap = surface;
    IDirectDrawSurface4_AddRef(mipmap);
    for (i = 0; i < 7; ++i)
    {
        hr = IDirectDrawSurface4_GetAttachedSurface(mipmap, &caps, &tmp);
        ok(SUCCEEDED(hr), "Failed to get attached surface, i %u, hr %#x.\n", i, hr);
        palette2 = (void *)0xdeadbeef;
        hr = IDirectDrawSurface4_GetPalette(tmp, &palette2);
        ok(hr == DDERR_NOPALETTEATTACHED, "Got unexpected hr %#x, i %u.\n", hr, i);
        ok(!palette2, "Got unexpected palette %p, i %u.\n", palette2, i);

        hr = IDirectDrawSurface4_SetPalette(tmp, palette_mipmap);
        ok(SUCCEEDED(hr), "Failed to set palette, i %u, hr %#x.\n", i, hr);

        hr = IDirectDrawSurface4_GetPalette(tmp, &palette2);
        ok(SUCCEEDED(hr), "Failed to get palette, i %u, hr %#x.\n", i, hr);
        ok(palette_mipmap == palette2, "Got unexpected palette %p.\n", palette2);
        IDirectDrawPalette_Release(palette2);

        hr = IDirectDrawSurface4_GetDC(tmp, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC, i %u, hr %#x.\n", i, hr);
        count = GetDIBColorTable(dc, 1, 1, &rgbquad);
        ok(count == 1, "Expected count 1, got %u.\n", count);
        ok(rgbquad.rgbRed == 0xff, "Expected rgbRed = 0xff, got %#x.\n", rgbquad.rgbRed);
        ok(rgbquad.rgbGreen == 0x80, "Expected rgbGreen = 0x80, got %#x.\n", rgbquad.rgbGreen);
        ok(rgbquad.rgbBlue == 0x0, "Expected rgbBlue = 0x0, got %#x.\n", rgbquad.rgbBlue);
        hr = IDirectDrawSurface4_ReleaseDC(tmp, dc);
        ok(SUCCEEDED(hr), "Failed to release DC, i %u, hr %#x.\n", i, hr);

        IDirectDrawSurface4_Release(mipmap);
        mipmap = tmp;
    }

    hr = IDirectDrawSurface4_GetAttachedSurface(mipmap, &caps, &tmp);
    ok(hr == DDERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    IDirectDrawSurface4_Release(mipmap);
    refcount = IDirectDrawSurface4_Release(surface);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDrawPalette_Release(palette_mipmap);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDrawPalette_Release(palette);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    refcount = IDirectDraw4_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_p8_rgb_blit(void)
{
    IDirectDrawSurface4 *src, *dst;
    DDSURFACEDESC2 surface_desc;
    IDirectDraw4 *ddraw;
    IDirectDrawPalette *palette;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    PALETTEENTRY palette_entries[256];
    unsigned int x;
    static const BYTE src_data[] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0xff, 0x80};
    static const D3DCOLOR expected[] =
    {
        0x00000000, 0x00010101, 0x00020202, 0x00030303,
        0x00040404, 0x00050505, 0x00ffffff, 0x00808080,
    };
    D3DCOLOR color;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(palette_entries, 0, sizeof(palette_entries));
    palette_entries[0].peRed = 0xff;
    palette_entries[1].peGreen = 0xff;
    palette_entries[2].peBlue = 0xff;
    palette_entries[3].peFlags = 0xff;
    hr = IDirectDraw4_CreatePalette(ddraw, DDPCAPS_8BIT | DDPCAPS_ALLOW256,
            palette_entries, &palette, NULL);
    ok(SUCCEEDED(hr), "Failed to create palette, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface_desc.dwWidth = 8;
    surface_desc.dwHeight = 1;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
    U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 8;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &src, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface_desc.dwWidth = 8;
    surface_desc.dwHeight = 1;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 32;
    U2(U4(surface_desc).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(U4(surface_desc).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(U4(surface_desc).ddpfPixelFormat).dwBBitMask = 0x000000ff;
    U5(U4(surface_desc).ddpfPixelFormat).dwRGBAlphaBitMask = 0xff000000;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &dst, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface4_Lock(src, NULL, &surface_desc, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to lock source surface, hr %#x.\n", hr);
    memcpy(surface_desc.lpSurface, src_data, sizeof(src_data));
    hr = IDirectDrawSurface4_Unlock(src, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock source surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_SetPalette(src, palette);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(dst, NULL, src, NULL, DDBLT_WAIT, NULL);
    /* The r500 Windows 7 driver returns E_NOTIMPL. r200 on Windows XP works.
     * The Geforce 7 driver on Windows Vista returns E_FAIL. Newer Nvidia GPUs work. */
    ok(SUCCEEDED(hr) || broken(hr == E_NOTIMPL) || broken(hr == E_FAIL),
            "Failed to blit, hr %#x.\n", hr);

    if (SUCCEEDED(hr))
    {
        for (x = 0; x < sizeof(expected) / sizeof(*expected); x++)
        {
            color = get_surface_color(dst, x, 0);
            todo_wine ok(compare_color(color, expected[x], 0),
                    "Pixel %u: Got color %#x, expected %#x.\n",
                    x, color, expected[x]);
        }
    }

    IDirectDrawSurface4_Release(src);
    IDirectDrawSurface4_Release(dst);
    IDirectDrawPalette_Release(palette);

    refcount = IDirectDraw4_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_material(void)
{
    D3DMATERIALHANDLE mat_handle, tmp;
    IDirect3DMaterial3 *material;
    IDirect3DViewport3 *viewport;
    IDirect3DDevice3 *device;
    IDirectDrawSurface4 *rt;
    D3DCOLOR color;
    ULONG refcount;
    unsigned int i;
    HWND window;
    HRESULT hr;
    BOOL valid;

    static struct
    {
        struct vec3 position;
        struct vec3 normal;
        D3DCOLOR diffuse;
    }
    quad1[] =
    {
        {{-1.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0xffffffff},
        {{-1.0f,  1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0xffffffff},
        {{ 1.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0xffffffff},
        {{ 1.0f,  1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0xffffffff},
    },
    quad2[] =
    {
        {{-1.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0xffff0000},
        {{-1.0f,  1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0xffff0000},
        {{ 1.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0xffff0000},
        {{ 1.0f,  1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0xffff0000},
    };
    static const struct
    {
        void *data;
        BOOL material;
        D3DCOLOR expected_color;
    }
    test_data[] =
    {
        {quad1, TRUE,  0x0000ff00},
        {quad2, TRUE,  0x0000ff00},
        {quad1, FALSE, 0x00ffffff},
        {quad2, FALSE, 0x00ff0000},
    };
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    viewport = create_viewport(device, 0, 0, 640, 480);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);

    material = create_emissive_material(device, 0.0f, 1.0f, 0.0f, 0.0f);
    hr = IDirect3DMaterial3_GetHandle(material, device, &mat_handle);
    ok(SUCCEEDED(hr), "Failed to get material handle, hr %#x.\n", hr);

    hr = IDirect3DDevice3_GetLightState(device, D3DLIGHTSTATE_MATERIAL, &tmp);
    ok(SUCCEEDED(hr), "Failed to get light state, hr %#x.\n", hr);
    ok(!tmp, "Got unexpected material handle %#x.\n", tmp);
    hr = IDirect3DDevice3_SetLightState(device, D3DLIGHTSTATE_MATERIAL, mat_handle);
    ok(SUCCEEDED(hr), "Failed to set material state, hr %#x.\n", hr);
    hr = IDirect3DDevice3_GetLightState(device, D3DLIGHTSTATE_MATERIAL, &tmp);
    ok(SUCCEEDED(hr), "Failed to get light state, hr %#x.\n", hr);
    ok(tmp == mat_handle, "Got unexpected material handle %#x, expected %#x.\n", tmp, mat_handle);
    hr = IDirect3DDevice3_SetLightState(device, D3DLIGHTSTATE_MATERIAL, 0);
    ok(SUCCEEDED(hr), "Failed to set material state, hr %#x.\n", hr);
    hr = IDirect3DDevice3_GetLightState(device, D3DLIGHTSTATE_MATERIAL, &tmp);
    ok(SUCCEEDED(hr), "Failed to get light state, hr %#x.\n", hr);
    ok(!tmp, "Got unexpected material handle %#x.\n", tmp);

    for (i = 0; i < sizeof(test_data) / sizeof(*test_data); ++i)
    {
        hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect,
                D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff0000ff, 1.0f, 0);
        ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);

        hr = IDirect3DDevice3_SetLightState(device, D3DLIGHTSTATE_MATERIAL, test_data[i].material ? mat_handle : 0);
        ok(SUCCEEDED(hr), "Failed to set material state, hr %#x.\n", hr);

        hr = IDirect3DDevice3_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
        hr = IDirect3DDevice2_DrawPrimitive(device, D3DPT_TRIANGLESTRIP,
                D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE, test_data[i].data, 4, 0);
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
        hr = IDirect3DDevice3_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);
        color = get_surface_color(rt, 320, 240);
        ok(compare_color(color, test_data[i].expected_color, 1),
                "Got unexpected color 0x%08x, test %u.\n", color, i);
    }

    destroy_material(material);
    material = create_diffuse_material(device, 1.0f, 0.0f, 0.0f, 1.0f);
    hr = IDirect3DMaterial3_GetHandle(material, device, &mat_handle);
    ok(SUCCEEDED(hr), "Failed to get material handle, hr %#x.\n", hr);

    hr = IDirect3DViewport3_SetBackground(viewport, mat_handle);
    ok(SUCCEEDED(hr), "Failed to set viewport background, hr %#x.\n", hr);
    hr = IDirect3DViewport3_GetBackground(viewport, &tmp, &valid);
    ok(SUCCEEDED(hr), "Failed to get viewport background, hr %#x.\n", hr);
    ok(tmp == mat_handle, "Got unexpected material handle %#x, expected %#x.\n", tmp, mat_handle);
    ok(valid, "Got unexpected valid %#x.\n", valid);
    hr = IDirect3DViewport3_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x00ff0000, 1), "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DViewport3_SetBackground(viewport, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DViewport3_GetBackground(viewport, &tmp, &valid);
    ok(SUCCEEDED(hr), "Failed to get viewport background, hr %#x.\n", hr);
    ok(tmp == mat_handle, "Got unexpected material handle %#x, expected %#x.\n", tmp, mat_handle);
    ok(valid, "Got unexpected valid %#x.\n", valid);
    hr = IDirect3DViewport3_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x00ff0000, 1), "Got unexpected color 0x%08x.\n", color);

    destroy_viewport(device, viewport);
    viewport = create_viewport(device, 0, 0, 640, 480);

    hr = IDirect3DViewport3_GetBackground(viewport, &tmp, &valid);
    ok(SUCCEEDED(hr), "Failed to get viewport background, hr %#x.\n", hr);
    ok(!tmp, "Got unexpected material handle %#x.\n", tmp);
    ok(!valid, "Got unexpected valid %#x.\n", valid);
    hr = IDirect3DViewport3_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x00000000, 1), "Got unexpected color 0x%08x.\n", color);

    destroy_viewport(device, viewport);
    destroy_material(material);
    IDirectDrawSurface4_Release(rt);
    refcount = IDirect3DDevice3_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_palette_gdi(void)
{
    IDirectDrawSurface4 *surface, *primary;
    DDSURFACEDESC2 surface_desc;
    IDirectDraw4 *ddraw;
    IDirectDrawPalette *palette, *palette2;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    PALETTEENTRY palette_entries[256];
    UINT i;
    HDC dc;
    /* On the Windows 8 testbot palette index 0 of the onscreen palette is forced to
     * r = 0, g = 0, b = 0. Do not attempt to set it to something else as this is
     * not the point of this test. */
    static const RGBQUAD expected1[] =
    {
        {0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x01, 0x00}, {0x00, 0x02, 0x00, 0x00},
        {0x03, 0x00, 0x00, 0x00}, {0x15, 0x14, 0x13, 0x00},
    };
    static const RGBQUAD expected2[] =
    {
        {0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x01, 0x00}, {0x00, 0x02, 0x00, 0x00},
        {0x03, 0x00, 0x00, 0x00}, {0x25, 0x24, 0x23, 0x00},
    };
    static const RGBQUAD expected3[] =
    {
        {0x00, 0x00, 0x00, 0x00}, {0x40, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x40, 0x00},
        {0x00, 0x40, 0x00, 0x00}, {0x56, 0x34, 0x12, 0x00},
    };
    HPALETTE ddraw_palette_handle;
    /* Similar to index 0, index 255 is r = 0xff, g = 0xff, b = 0xff on the Win8 VMs. */
    RGBQUAD rgbquad[255];
    static const RGBQUAD rgb_zero = {0, 0, 0, 0};

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface_desc.dwWidth = 16;
    surface_desc.dwHeight = 16;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
    U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 8;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    /* Avoid colors from the Windows default palette. */
    memset(palette_entries, 0, sizeof(palette_entries));
    palette_entries[1].peRed = 0x01;
    palette_entries[2].peGreen = 0x02;
    palette_entries[3].peBlue = 0x03;
    palette_entries[4].peRed = 0x13;
    palette_entries[4].peGreen = 0x14;
    palette_entries[4].peBlue = 0x15;
    hr = IDirectDraw4_CreatePalette(ddraw, DDPCAPS_8BIT | DDPCAPS_ALLOW256,
            palette_entries, &palette, NULL);
    ok(SUCCEEDED(hr), "Failed to create palette, hr %#x.\n", hr);

    /* If there is no palette assigned and the display mode is not 8 bpp, some
     * drivers refuse to create a DC while others allow it. If a DC is created,
     * the DIB color table is uninitialized and contains random colors. No error
     * is generated when trying to read pixels and random garbage is returned.
     *
     * The most likely explanation is that if the driver creates a DC, it (or
     * the higher-level runtime) uses GetSystemPaletteEntries to find the
     * palette, but GetSystemPaletteEntries fails when bpp > 8 and the palette
     * contains uninitialized garbage. See comments below for the P8 case. */

    hr = IDirectDrawSurface4_SetPalette(surface, palette);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetDC(surface, &dc);
    ok(SUCCEEDED(hr), "Failed to get DC, hr %#x.\n", hr);
    ddraw_palette_handle = SelectPalette(dc, GetStockObject(DEFAULT_PALETTE), FALSE);
    ok(ddraw_palette_handle == GetStockObject(DEFAULT_PALETTE),
            "Got unexpected palette %p, expected %p.\n",
            ddraw_palette_handle, GetStockObject(DEFAULT_PALETTE));

    i = GetDIBColorTable(dc, 0, sizeof(rgbquad) / sizeof(*rgbquad), rgbquad);
    ok(i == sizeof(rgbquad) / sizeof(*rgbquad), "Expected count 255, got %u.\n", i);
    for (i = 0; i < sizeof(expected1) / sizeof(*expected1); i++)
    {
        ok(!memcmp(&rgbquad[i], &expected1[i], sizeof(rgbquad[i])),
                "Got color table entry %u r=%#x g=%#x b=%#x, expected r=%#x g=%#x b=%#x.\n",
                i, rgbquad[i].rgbRed, rgbquad[i].rgbGreen, rgbquad[i].rgbBlue,
                expected1[i].rgbRed, expected1[i].rgbGreen, expected1[i].rgbBlue);
    }
    for (; i < sizeof(rgbquad) / sizeof(*rgbquad); i++)
    {
        ok(!memcmp(&rgbquad[i], &rgb_zero, sizeof(rgbquad[i])),
                "Got color table entry %u r=%#x g=%#x b=%#x, expected r=0 g=0 b=0.\n",
                i, rgbquad[i].rgbRed, rgbquad[i].rgbGreen, rgbquad[i].rgbBlue);
    }

    /* Update the palette while the DC is in use. This does not modify the DC. */
    palette_entries[4].peRed = 0x23;
    palette_entries[4].peGreen = 0x24;
    palette_entries[4].peBlue = 0x25;
    hr = IDirectDrawPalette_SetEntries(palette, 0, 4, 1, &palette_entries[4]);
    ok(SUCCEEDED(hr), "Failed to set palette entries, hr %#x.\n", hr);

    i = GetDIBColorTable(dc, 4, 1, &rgbquad[4]);
    ok(i == 1, "Expected count 1, got %u.\n", i);
    todo_wine ok(!memcmp(&rgbquad[4], &expected1[4], sizeof(rgbquad[4])),
            "Got color table entry %u r=%#x g=%#x b=%#x, expected r=%#x g=%#x b=%#x.\n",
            i, rgbquad[4].rgbRed, rgbquad[4].rgbGreen, rgbquad[4].rgbBlue,
            expected1[4].rgbRed, expected1[4].rgbGreen, expected1[4].rgbBlue);

    /* Neither does re-setting the palette. */
    hr = IDirectDrawSurface4_SetPalette(surface, NULL);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_SetPalette(surface, palette);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);

    i = GetDIBColorTable(dc, 4, 1, &rgbquad[4]);
    ok(i == 1, "Expected count 1, got %u.\n", i);
    todo_wine ok(!memcmp(&rgbquad[4], &expected1[4], sizeof(rgbquad[4])),
            "Got color table entry %u r=%#x g=%#x b=%#x, expected r=%#x g=%#x b=%#x.\n",
            i, rgbquad[4].rgbRed, rgbquad[4].rgbGreen, rgbquad[4].rgbBlue,
            expected1[4].rgbRed, expected1[4].rgbGreen, expected1[4].rgbBlue);

    hr = IDirectDrawSurface4_ReleaseDC(surface, dc);
    ok(SUCCEEDED(hr), "Failed to release DC, hr %#x.\n", hr);

    /* Refresh the DC. This updates the palette. */
    hr = IDirectDrawSurface4_GetDC(surface, &dc);
    ok(SUCCEEDED(hr), "Failed to get DC, hr %#x.\n", hr);
    i = GetDIBColorTable(dc, 0, sizeof(rgbquad) / sizeof(*rgbquad), rgbquad);
    ok(i == sizeof(rgbquad) / sizeof(*rgbquad), "Expected count 255, got %u.\n", i);
    for (i = 0; i < sizeof(expected2) / sizeof(*expected2); i++)
    {
        ok(!memcmp(&rgbquad[i], &expected2[i], sizeof(rgbquad[i])),
                "Got color table entry %u r=%#x g=%#x b=%#x, expected r=%#x g=%#x b=%#x.\n",
                i, rgbquad[i].rgbRed, rgbquad[i].rgbGreen, rgbquad[i].rgbBlue,
                expected2[i].rgbRed, expected2[i].rgbGreen, expected2[i].rgbBlue);
    }
    for (; i < sizeof(rgbquad) / sizeof(*rgbquad); i++)
    {
        ok(!memcmp(&rgbquad[i], &rgb_zero, sizeof(rgbquad[i])),
                "Got color table entry %u r=%#x g=%#x b=%#x, expected r=0 g=0 b=0.\n",
                i, rgbquad[i].rgbRed, rgbquad[i].rgbGreen, rgbquad[i].rgbBlue);
    }
    hr = IDirectDrawSurface4_ReleaseDC(surface, dc);
    ok(SUCCEEDED(hr), "Failed to release DC, hr %#x.\n", hr);

    refcount = IDirectDrawSurface4_Release(surface);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    if (FAILED(hr = IDirectDraw4_SetDisplayMode(ddraw, 640, 480, 8, 0, 0)))
    {
        win_skip("Failed to set 8 bpp display mode, skipping test.\n");
        IDirectDrawPalette_Release(palette);
        IDirectDraw4_Release(ddraw);
        DestroyWindow(window);
        return;
    }
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_SetPalette(primary, palette);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetDC(primary, &dc);
    ok(SUCCEEDED(hr), "Failed to get DC, hr %#x.\n", hr);
    ddraw_palette_handle = SelectPalette(dc, GetStockObject(DEFAULT_PALETTE), FALSE);
    /* Windows 2000 on the testbot assigns a different palette to the primary. Refrast? */
    ok(ddraw_palette_handle == GetStockObject(DEFAULT_PALETTE) || broken(TRUE),
            "Got unexpected palette %p, expected %p.\n",
            ddraw_palette_handle, GetStockObject(DEFAULT_PALETTE));
    SelectPalette(dc, ddraw_palette_handle, FALSE);

    /* The primary uses the system palette. In exclusive mode, the system palette matches
     * the ddraw palette attached to the primary, so the result is what you would expect
     * from a regular surface. Tests for the interaction between the ddraw palette and
     * the system palette are not included pending an application that depends on this.
     * The relation between those causes problems on Windows Vista and newer for games
     * like Age of Empires or Starcraft. Don't emulate it without a real need. */
    i = GetDIBColorTable(dc, 0, sizeof(rgbquad) / sizeof(*rgbquad), rgbquad);
    ok(i == sizeof(rgbquad) / sizeof(*rgbquad), "Expected count 255, got %u.\n", i);
    for (i = 0; i < sizeof(expected2) / sizeof(*expected2); i++)
    {
        ok(!memcmp(&rgbquad[i], &expected2[i], sizeof(rgbquad[i])),
                "Got color table entry %u r=%#x g=%#x b=%#x, expected r=%#x g=%#x b=%#x.\n",
                i, rgbquad[i].rgbRed, rgbquad[i].rgbGreen, rgbquad[i].rgbBlue,
                expected2[i].rgbRed, expected2[i].rgbGreen, expected2[i].rgbBlue);
    }
    for (; i < sizeof(rgbquad) / sizeof(*rgbquad); i++)
    {
        ok(!memcmp(&rgbquad[i], &rgb_zero, sizeof(rgbquad[i])),
                "Got color table entry %u r=%#x g=%#x b=%#x, expected r=0 g=0 b=0.\n",
                i, rgbquad[i].rgbRed, rgbquad[i].rgbGreen, rgbquad[i].rgbBlue);
    }
    hr = IDirectDrawSurface4_ReleaseDC(primary, dc);
    ok(SUCCEEDED(hr), "Failed to release DC, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.dwWidth = 16;
    surface_desc.dwHeight = 16;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    /* Here the offscreen surface appears to use the primary's palette,
     * but in all likelyhood it is actually the system palette. */
    hr = IDirectDrawSurface4_GetDC(surface, &dc);
    ok(SUCCEEDED(hr), "Failed to get DC, hr %#x.\n", hr);
    i = GetDIBColorTable(dc, 0, sizeof(rgbquad) / sizeof(*rgbquad), rgbquad);
    ok(i == sizeof(rgbquad) / sizeof(*rgbquad), "Expected count 255, got %u.\n", i);
    for (i = 0; i < sizeof(expected2) / sizeof(*expected2); i++)
    {
        ok(!memcmp(&rgbquad[i], &expected2[i], sizeof(rgbquad[i])),
                "Got color table entry %u r=%#x g=%#x b=%#x, expected r=%#x g=%#x b=%#x.\n",
                i, rgbquad[i].rgbRed, rgbquad[i].rgbGreen, rgbquad[i].rgbBlue,
                expected2[i].rgbRed, expected2[i].rgbGreen, expected2[i].rgbBlue);
    }
    for (; i < sizeof(rgbquad) / sizeof(*rgbquad); i++)
    {
        ok(!memcmp(&rgbquad[i], &rgb_zero, sizeof(rgbquad[i])),
                "Got color table entry %u r=%#x g=%#x b=%#x, expected r=0 g=0 b=0.\n",
                i, rgbquad[i].rgbRed, rgbquad[i].rgbGreen, rgbquad[i].rgbBlue);
    }
    hr = IDirectDrawSurface4_ReleaseDC(surface, dc);
    ok(SUCCEEDED(hr), "Failed to release DC, hr %#x.\n", hr);

    /* On real hardware a change to the primary surface's palette applies immediately,
     * even on device contexts from offscreen surfaces that do not have their own
     * palette. On the testbot VMs this is not the case. Don't test this until we
     * know of an application that depends on this. */

    memset(palette_entries, 0, sizeof(palette_entries));
    palette_entries[1].peBlue = 0x40;
    palette_entries[2].peRed = 0x40;
    palette_entries[3].peGreen = 0x40;
    palette_entries[4].peRed = 0x12;
    palette_entries[4].peGreen = 0x34;
    palette_entries[4].peBlue = 0x56;
    hr = IDirectDraw4_CreatePalette(ddraw, DDPCAPS_8BIT | DDPCAPS_ALLOW256,
            palette_entries, &palette2, NULL);
    ok(SUCCEEDED(hr), "Failed to create palette, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_SetPalette(surface, palette2);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);

    /* A palette assigned to the offscreen surface overrides the primary / system
     * palette. */
    hr = IDirectDrawSurface4_GetDC(surface, &dc);
    ok(SUCCEEDED(hr), "Failed to get DC, hr %#x.\n", hr);
    i = GetDIBColorTable(dc, 0, sizeof(rgbquad) / sizeof(*rgbquad), rgbquad);
    ok(i == sizeof(rgbquad) / sizeof(*rgbquad), "Expected count 255, got %u.\n", i);
    for (i = 0; i < sizeof(expected3) / sizeof(*expected3); i++)
    {
        ok(!memcmp(&rgbquad[i], &expected3[i], sizeof(rgbquad[i])),
                "Got color table entry %u r=%#x g=%#x b=%#x, expected r=%#x g=%#x b=%#x.\n",
                i, rgbquad[i].rgbRed, rgbquad[i].rgbGreen, rgbquad[i].rgbBlue,
                expected3[i].rgbRed, expected3[i].rgbGreen, expected3[i].rgbBlue);
    }
    for (; i < sizeof(rgbquad) / sizeof(*rgbquad); i++)
    {
        ok(!memcmp(&rgbquad[i], &rgb_zero, sizeof(rgbquad[i])),
                "Got color table entry %u r=%#x g=%#x b=%#x, expected r=0 g=0 b=0.\n",
                i, rgbquad[i].rgbRed, rgbquad[i].rgbGreen, rgbquad[i].rgbBlue);
    }
    hr = IDirectDrawSurface4_ReleaseDC(surface, dc);
    ok(SUCCEEDED(hr), "Failed to release DC, hr %#x.\n", hr);

    refcount = IDirectDrawSurface4_Release(surface);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    /* The Windows 8 testbot keeps extra references to the primary and
     * backbuffer while in 8 bpp mode. */
    hr = IDirectDraw4_RestoreDisplayMode(ddraw);
    ok(SUCCEEDED(hr), "Failed to restore display mode, hr %#x.\n", hr);

    refcount = IDirectDrawSurface4_Release(primary);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDrawPalette_Release(palette2);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDrawPalette_Release(palette);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDraw4_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_palette_alpha(void)
{
    IDirectDrawSurface4 *surface;
    DDSURFACEDESC2 surface_desc;
    IDirectDraw4 *ddraw;
    IDirectDrawPalette *palette;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    PALETTEENTRY palette_entries[256];
    unsigned int i;
    static const struct
    {
        DWORD caps, flags;
        BOOL attach_allowed;
        const char *name;
    }
    test_data[] =
    {
        {DDSCAPS_OFFSCREENPLAIN, DDSD_WIDTH | DDSD_HEIGHT, FALSE, "offscreenplain"},
        {DDSCAPS_TEXTURE, DDSD_WIDTH | DDSD_HEIGHT, TRUE, "texture"},
        {DDSCAPS_PRIMARYSURFACE, 0, FALSE, "primary"}
    };

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (FAILED(hr = IDirectDraw4_SetDisplayMode(ddraw, 640, 480, 8, 0, 0)))
    {
        win_skip("Failed to set 8 bpp display mode, skipping test.\n");
        IDirectDraw4_Release(ddraw);
        DestroyWindow(window);
        return;
    }
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(palette_entries, 0, sizeof(palette_entries));
    palette_entries[1].peFlags = 0x42;
    palette_entries[2].peFlags = 0xff;
    palette_entries[3].peFlags = 0x80;
    hr = IDirectDraw4_CreatePalette(ddraw, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, palette_entries, &palette, NULL);
    ok(SUCCEEDED(hr), "Failed to create palette, hr %#x.\n", hr);

    memset(palette_entries, 0x66, sizeof(palette_entries));
    hr = IDirectDrawPalette_GetEntries(palette, 0, 1, 4, palette_entries);
    ok(SUCCEEDED(hr), "Failed to get palette entries, hr %#x.\n", hr);
    ok(palette_entries[0].peFlags == 0x42, "Got unexpected peFlags 0x%02x, expected 0xff.\n",
            palette_entries[0].peFlags);
    ok(palette_entries[1].peFlags == 0xff, "Got unexpected peFlags 0x%02x, expected 0xff.\n",
            palette_entries[1].peFlags);
    ok(palette_entries[2].peFlags == 0x80, "Got unexpected peFlags 0x%02x, expected 0x80.\n",
            palette_entries[2].peFlags);
    ok(palette_entries[3].peFlags == 0x00, "Got unexpected peFlags 0x%02x, expected 0x00.\n",
            palette_entries[3].peFlags);

    IDirectDrawPalette_Release(palette);

    memset(palette_entries, 0, sizeof(palette_entries));
    palette_entries[1].peFlags = 0x42;
    palette_entries[1].peRed   = 0xff;
    palette_entries[2].peFlags = 0xff;
    palette_entries[3].peFlags = 0x80;
    hr = IDirectDraw4_CreatePalette(ddraw, DDPCAPS_ALLOW256 | DDPCAPS_8BIT | DDPCAPS_ALPHA,
            palette_entries, &palette, NULL);
    ok(SUCCEEDED(hr), "Failed to create palette, hr %#x.\n", hr);

    memset(palette_entries, 0x66, sizeof(palette_entries));
    hr = IDirectDrawPalette_GetEntries(palette, 0, 1, 4, palette_entries);
    ok(SUCCEEDED(hr), "Failed to get palette entries, hr %#x.\n", hr);
    ok(palette_entries[0].peFlags == 0x42, "Got unexpected peFlags 0x%02x, expected 0xff.\n",
            palette_entries[0].peFlags);
    ok(palette_entries[1].peFlags == 0xff, "Got unexpected peFlags 0x%02x, expected 0xff.\n",
            palette_entries[1].peFlags);
    ok(palette_entries[2].peFlags == 0x80, "Got unexpected peFlags 0x%02x, expected 0x80.\n",
            palette_entries[2].peFlags);
    ok(palette_entries[3].peFlags == 0x00, "Got unexpected peFlags 0x%02x, expected 0x00.\n",
            palette_entries[3].peFlags);

    for (i = 0; i < sizeof(test_data) / sizeof(*test_data); i++)
    {
        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | test_data[i].flags;
        surface_desc.dwWidth = 128;
        surface_desc.dwHeight = 128;
        surface_desc.ddsCaps.dwCaps = test_data[i].caps;
        hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
        ok(SUCCEEDED(hr), "Failed to create %s surface, hr %#x.\n", test_data[i].name, hr);

        hr = IDirectDrawSurface4_SetPalette(surface, palette);
        if (test_data[i].attach_allowed)
            ok(SUCCEEDED(hr), "Failed to attach palette to %s surface, hr %#x.\n", test_data[i].name, hr);
        else
            ok(hr == DDERR_INVALIDSURFACETYPE, "Got unexpected hr %#x, %s surface.\n", hr, test_data[i].name);

        if (SUCCEEDED(hr))
        {
            HDC dc;
            RGBQUAD rgbquad;
            UINT retval;

            hr = IDirectDrawSurface4_GetDC(surface, &dc);
            ok(SUCCEEDED(hr), "Failed to get DC, hr %#x, %s surface.\n", hr, test_data[i].name);
            retval = GetDIBColorTable(dc, 1, 1, &rgbquad);
            ok(retval == 1, "GetDIBColorTable returned unexpected result %u.\n", retval);
            ok(rgbquad.rgbRed == 0xff, "Expected rgbRed = 0xff, got %#x, %s surface.\n",
                    rgbquad.rgbRed, test_data[i].name);
            ok(rgbquad.rgbGreen == 0, "Expected rgbGreen = 0, got %#x, %s surface.\n",
                    rgbquad.rgbGreen, test_data[i].name);
            ok(rgbquad.rgbBlue == 0, "Expected rgbBlue = 0, got %#x, %s surface.\n",
                    rgbquad.rgbBlue, test_data[i].name);
            todo_wine ok(rgbquad.rgbReserved == 0, "Expected rgbReserved = 0, got %u, %s surface.\n",
                    rgbquad.rgbReserved, test_data[i].name);
            hr = IDirectDrawSurface4_ReleaseDC(surface, dc);
            ok(SUCCEEDED(hr), "Failed to release DC, hr %#x.\n", hr);
        }
        IDirectDrawSurface4_Release(surface);
    }

    /* Test INVALIDSURFACETYPE vs INVALIDPIXELFORMAT. */
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface_desc.dwWidth = 128;
    surface_desc.dwHeight = 128;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 32;
    U2(U4(surface_desc).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(U4(surface_desc).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(U4(surface_desc).ddpfPixelFormat).dwBBitMask = 0x000000ff;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_SetPalette(surface, palette);
    ok(hr == DDERR_INVALIDSURFACETYPE, "Got unexpected hr %#x.\n", hr);
    IDirectDrawSurface4_Release(surface);

    /* The Windows 8 testbot keeps extra references to the primary
     * while in 8 bpp mode. */
    hr = IDirectDraw4_RestoreDisplayMode(ddraw);
    ok(SUCCEEDED(hr), "Failed to restore display mode, hr %#x.\n", hr);

    refcount = IDirectDrawPalette_Release(palette);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDraw4_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

START_TEST(ddraw4)
{
    IDirectDraw4 *ddraw;

    if (!(ddraw = create_ddraw()))
    {
        skip("Failed to create a ddraw object, skipping tests.\n");
        return;
    }
    IDirectDraw4_Release(ddraw);

    test_process_vertices();
    test_coop_level_create_device_window();
    test_clipper_blt();
    test_coop_level_d3d_state();
    test_surface_interface_mismatch();
    test_coop_level_threaded();
    test_depth_blit();
    test_texture_load_ckey();
    test_viewport();
    test_zenable();
    test_ck_rgba();
    test_ck_default();
    test_ck_complex();
    test_surface_qi();
    test_device_qi();
    test_wndproc();
    test_window_style();
    test_redundant_mode_set();
    test_coop_level_mode_set();
    test_coop_level_mode_set_multi();
    test_initialize();
    test_coop_level_surf_create();
    test_vb_discard();
    test_coop_level_multi_window();
    test_draw_strided();
    test_clear_rect_count();
    test_coop_level_versions();
    test_lighting_interface_versions();
    test_coop_level_activateapp();
    test_texturemanage();
    test_block_formats_creation();
    test_unsupported_formats();
    test_rt_caps();
    test_primary_caps();
    test_surface_lock();
    test_surface_discard();
    test_flip();
    test_set_surface_desc();
    test_user_memory_getdc();
    test_sysmem_overlay();
    test_primary_palette();
    test_surface_attachment();
    test_private_data();
    test_pixel_format();
    test_create_surface_pitch();
    test_mipmap_lock();
    test_palette_complex();
    test_p8_rgb_blit();
    test_material();
    test_palette_gdi();
    test_palette_alpha();
}
