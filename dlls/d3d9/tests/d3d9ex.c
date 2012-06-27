/*
 * Copyright (C) 2008 Stefan Dösinger(for CodeWeavers)
 * Copyright (C) 2010 Louis Lenders
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

/* This file contains tests specific to IDirect3D9Ex and IDirect3DDevice9Ex, like
 * how to obtain them. For testing rendering with extended functions use visual.c
 */

#define COBJMACROS
#include "wine/test.h"
#include "winuser.h"
#include "wingdi.h"
#include <initguid.h>
#include <d3d9.h>

static HMODULE d3d9_handle = 0;

static BOOL (WINAPI *pEnumDisplaySettingsExA)(LPCSTR, DWORD, DEVMODEA *, DWORD);
static LONG (WINAPI *pChangeDisplaySettingsExA)(LPCSTR, LPDEVMODE, HWND, DWORD, LPVOID);

static IDirect3D9 * (WINAPI *pDirect3DCreate9)(UINT SDKVersion);
static HRESULT (WINAPI *pDirect3DCreate9Ex)(UINT SDKVersion, IDirect3D9Ex **d3d9ex);

static HWND create_window(void)
{
    WNDCLASS wc = {0};
    HWND ret;
    wc.lpfnWndProc = DefWindowProc;
    wc.lpszClassName = "d3d9_test_wc";
    RegisterClass(&wc);

    ret = CreateWindow("d3d9_test_wc", "d3d9_test",
                        WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION , 0, 0, 640, 480, 0, 0, 0, 0);
    return ret;
}

static IDirect3DDevice9Ex *create_device(IDirect3D9Ex *d3d9, HWND device_window, HWND focus_window, BOOL windowed)
{
    D3DPRESENT_PARAMETERS present_parameters = {0};
    IDirect3DDevice9Ex *device;
    D3DDISPLAYMODEEX mode, *m;

    present_parameters.Windowed = windowed;
    present_parameters.hDeviceWindow = device_window;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;

    mode.Size = sizeof(mode);
    mode.Width = 640;
    mode.Height = 480;
    mode.RefreshRate = 0;
    mode.Format = D3DFMT_A8R8G8B8;
    mode.ScanLineOrdering = 0;

    m = windowed ? NULL : &mode;
    if (SUCCEEDED(IDirect3D9Ex_CreateDeviceEx(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, focus_window,
            D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, m, &device))) return device;

    present_parameters.AutoDepthStencilFormat = D3DFMT_D16;
    if (SUCCEEDED(IDirect3D9Ex_CreateDeviceEx(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, focus_window,
            D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, m, &device))) return device;

    if (SUCCEEDED(IDirect3D9Ex_CreateDeviceEx(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, focus_window,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, m, &device))) return device;

    return NULL;
}

static ULONG getref(IUnknown *obj) {
    IUnknown_AddRef(obj);
    return IUnknown_Release(obj);
}

static void test_qi_base_to_ex(void)
{
    IDirect3D9 *d3d9 = pDirect3DCreate9(D3D_SDK_VERSION);
    IDirect3D9Ex *d3d9ex = (void *) 0xdeadbeef;
    IDirect3DDevice9 *device;
    IDirect3DDevice9Ex *deviceEx = (void *) 0xdeadbeef;
    HRESULT hr;
    HWND window = create_window();
    D3DPRESENT_PARAMETERS present_parameters;

    if (!d3d9)
    {
        skip("Direct3D9 is not available\n");
        return;
    }

    hr = IDirect3D9_QueryInterface(d3d9, &IID_IDirect3D9Ex, (void **) &d3d9ex);
    ok(hr == E_NOINTERFACE,
       "IDirect3D9::QueryInterface for IID_IDirect3D9Ex returned %08x, expected E_NOINTERFACE\n",
       hr);
    ok(d3d9ex == NULL, "QueryInterface returned interface %p, expected NULL\n", d3d9ex);
    if(d3d9ex) IDirect3D9Ex_Release(d3d9ex);

    memset(&present_parameters, 0, sizeof(present_parameters));
    present_parameters.Windowed = TRUE;
    present_parameters.hDeviceWindow = window;
    present_parameters.SwapEffect = D3DSWAPEFFECT_COPY;
    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.EnableAutoDepthStencil = FALSE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D16;
    hr = IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, present_parameters.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device);
    if(FAILED(hr)) {
        skip("Failed to create a regular Direct3DDevice9, skipping QI tests\n");
        goto out;
    }

    hr = IDirect3DDevice9_QueryInterface(device, &IID_IDirect3DDevice9Ex, (void **) &deviceEx);
    ok(hr == E_NOINTERFACE,
       "IDirect3D9Device::QueryInterface for IID_IDirect3DDevice9Ex returned %08x, expected E_NOINTERFACE\n",
       hr);
    ok(deviceEx == NULL, "QueryInterface returned interface %p, expected NULL\n", deviceEx);
    if(deviceEx) IDirect3DDevice9Ex_Release(deviceEx);

    IDirect3DDevice9_Release(device);

out:
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_qi_ex_to_base(void)
{
    IDirect3D9 *d3d9 = (void *) 0xdeadbeef;
    IDirect3D9Ex *d3d9ex;
    IDirect3DDevice9 *device;
    IDirect3DDevice9Ex *deviceEx = (void *) 0xdeadbeef;
    HRESULT hr;
    HWND window = create_window();
    D3DPRESENT_PARAMETERS present_parameters;
    ULONG ref;

    hr = pDirect3DCreate9Ex(D3D_SDK_VERSION, &d3d9ex);
    ok(hr == D3D_OK || hr == D3DERR_NOTAVAILABLE, "Direct3DCreate9Ex returned %08x\n", hr);
    if(FAILED(hr)) {
        skip("Direct3D9Ex is not available\n");
        goto out;
    }

    hr = IDirect3D9Ex_QueryInterface(d3d9ex, &IID_IDirect3D9, (void **) &d3d9);
    ok(hr == D3D_OK,
       "IDirect3D9Ex::QueryInterface for IID_IDirect3D9 returned %08x, expected D3D_OK\n",
       hr);
    ok(d3d9 != NULL && d3d9 != (void *) 0xdeadbeef,
       "QueryInterface returned interface %p, expected != NULL && != 0xdeadbeef\n", d3d9);
    ref = getref((IUnknown *) d3d9ex);
    ok(ref == 2, "IDirect3D9Ex refcount is %d, expected 2\n", ref);
    ref = getref((IUnknown *) d3d9);
    ok(ref == 2, "IDirect3D9 refcount is %d, expected 2\n", ref);

    memset(&present_parameters, 0, sizeof(present_parameters));
    present_parameters.Windowed = TRUE;
    present_parameters.hDeviceWindow = window;
    present_parameters.SwapEffect = D3DSWAPEFFECT_COPY;
    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.EnableAutoDepthStencil = FALSE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D16;

    /* First, try to create a normal device with IDirect3D9Ex::CreateDevice and QI it for IDirect3DDevice9Ex */
    hr = IDirect3D9Ex_CreateDevice(d3d9ex, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, present_parameters.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device);
    if(FAILED(hr)) {
        skip("Failed to create a regular Direct3DDevice9, skipping QI tests\n");
        goto out;
    }

    hr = IDirect3DDevice9_QueryInterface(device, &IID_IDirect3DDevice9Ex, (void **) &deviceEx);
    ok(hr == D3D_OK,
       "IDirect3D9Device::QueryInterface for IID_IDirect3DDevice9Ex returned %08x, expected D3D_OK\n",
       hr);
    ok(deviceEx != NULL && deviceEx != (void *) 0xdeadbeef,
       "QueryInterface returned interface %p, expected != NULL && != 0xdeadbeef\n", deviceEx);
    ref = getref((IUnknown *) device);
    ok(ref == 2, "IDirect3DDevice9 refcount is %d, expected 2\n", ref);
    ref = getref((IUnknown *) deviceEx);
    ok(ref == 2, "IDirect3DDevice9Ex refcount is %d, expected 2\n", ref);
    if(deviceEx) IDirect3DDevice9Ex_Release(deviceEx);
    IDirect3DDevice9_Release(device);

    /* Next, try to create a normal device with IDirect3D9::CreateDevice(non-ex) and QI it */
    hr = IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, present_parameters.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device);
    if(FAILED(hr)) {
        skip("Failed to create a regular Direct3DDevice9, skipping QI tests\n");
        goto out;
    }

    hr = IDirect3DDevice9_QueryInterface(device, &IID_IDirect3DDevice9Ex, (void **) &deviceEx);
    ok(hr == D3D_OK,
       "IDirect3D9Device::QueryInterface for IID_IDirect3DDevice9Ex returned %08x, expected D3D_OK\n",
       hr);
    ok(deviceEx != NULL && deviceEx != (void *) 0xdeadbeef,
       "QueryInterface returned interface %p, expected != NULL && != 0xdeadbeef\n", deviceEx);
    ref = getref((IUnknown *) device);
    ok(ref == 2, "IDirect3DDevice9 refcount is %d, expected 2\n", ref);
    ref = getref((IUnknown *) deviceEx);
    ok(ref == 2, "IDirect3DDevice9Ex refcount is %d, expected 2\n", ref);
    if(deviceEx) IDirect3DDevice9Ex_Release(deviceEx);
    IDirect3DDevice9_Release(device);

    IDirect3D9_Release(d3d9);
    IDirect3D9Ex_Release(d3d9ex);

out:
    DestroyWindow(window);
}

static void test_get_adapter_luid(void)
{
    HWND window = create_window();
    IDirect3D9Ex *d3d9ex;
    UINT count;
    HRESULT hr;
    LUID luid;

    hr = pDirect3DCreate9Ex(D3D_SDK_VERSION, &d3d9ex);
    if (FAILED(hr))
    {
        skip("Direct3D9Ex is not available.\n");
        DestroyWindow(window);
        return;
    }

    count = IDirect3D9Ex_GetAdapterCount(d3d9ex);
    if (!count)
    {
        skip("No adapters available.\n");
        IDirect3D9Ex_Release(d3d9ex);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3D9Ex_GetAdapterLUID(d3d9ex, D3DADAPTER_DEFAULT, &luid);
    ok(SUCCEEDED(hr), "GetAdapterLUID failed, hr %#x.\n", hr);
    trace("adapter luid: %08x:%08x.\n", luid.HighPart, luid.LowPart);

    IDirect3D9Ex_Release(d3d9ex);
}

static void test_get_adapter_displaymode_ex(void)
{
    HWND window = create_window();
    IDirect3D9 *d3d9 = (void *) 0xdeadbeef;
    IDirect3D9Ex *d3d9ex;
    UINT count;
    HRESULT hr;
    D3DDISPLAYMODE mode;
    D3DDISPLAYMODEEX mode_ex;
    D3DDISPLAYROTATION rotation;
    HANDLE hdll;
    DEVMODEA startmode;
    LONG retval;

    hr = pDirect3DCreate9Ex(D3D_SDK_VERSION, &d3d9ex);
    if (FAILED(hr))
    {
        skip("Direct3D9Ex is not available (%#x)\n", hr);
        DestroyWindow(window);
        return;
    }

    count = IDirect3D9Ex_GetAdapterCount(d3d9ex);
    if (!count)
    {
        skip("No adapters available.\n");
        IDirect3D9Ex_Release(d3d9ex);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3D9Ex_QueryInterface(d3d9ex, &IID_IDirect3D9, (void **) &d3d9);
    ok(hr == D3D_OK,
       "IDirect3D9Ex::QueryInterface for IID_IDirect3D9 returned %08x, expected D3D_OK\n",
       hr);
    ok(d3d9 != NULL && d3d9 != (void *) 0xdeadbeef,
       "QueryInterface returned interface %p, expected != NULL && != 0xdeadbeef\n", d3d9);
    /* change displayorientation*/
    hdll = GetModuleHandleA("user32.dll");
    pEnumDisplaySettingsExA = (void*)GetProcAddress(hdll, "EnumDisplaySettingsExA");
    pChangeDisplaySettingsExA = (void*)GetProcAddress(hdll, "ChangeDisplaySettingsExA");

    if (!pEnumDisplaySettingsExA || !pChangeDisplaySettingsExA) goto out;

    memset(&startmode, 0, sizeof(startmode));
    startmode.dmSize = sizeof(startmode);
    retval = pEnumDisplaySettingsExA(NULL, ENUM_CURRENT_SETTINGS, &startmode, 0);
    ok(retval, "Failed to retrieve current display mode, retval %d.\n", retval);
    if (!retval) goto out;

    startmode.dmFields = DM_DISPLAYORIENTATION | DM_PELSWIDTH | DM_PELSHEIGHT;
    S2(U1(startmode)).dmDisplayOrientation = DMDO_180;
    retval = pChangeDisplaySettingsExA(NULL, &startmode, NULL, 0, NULL);

    if(retval == DISP_CHANGE_BADMODE)
    {
        trace(" Test skipped: graphics mode is not supported\n");
        goto out;
    }

    ok(retval == DISP_CHANGE_SUCCESSFUL,"ChangeDisplaySettingsEx failed with %d\n", retval);
    /* try retrieve orientation info with EnumDisplaySettingsEx*/
    startmode.dmFields = 0;
    S2(U1(startmode)).dmDisplayOrientation = 0;
    ok(pEnumDisplaySettingsExA(NULL, ENUM_CURRENT_SETTINGS, &startmode, EDS_ROTATEDMODE), "EnumDisplaySettingsEx failed\n");

    /*now that orientation has changed start tests for GetAdapterDisplayModeEx: invalid Size*/
    memset(&mode_ex, 0, sizeof(mode_ex));
    hr = IDirect3D9Ex_GetAdapterDisplayModeEx(d3d9ex, D3DADAPTER_DEFAULT, &mode_ex, &rotation);
    ok(hr == D3DERR_INVALIDCALL, "GetAdapterDisplayModeEx returned %#x instead of D3DERR_INVALIDCALL\n", hr);

    mode_ex.Size = sizeof(D3DDISPLAYMODEEX);
    /* invalid count*/
    hr = IDirect3D9Ex_GetAdapterDisplayModeEx(d3d9ex, count + 1, &mode_ex, &rotation);
    ok(hr == D3DERR_INVALIDCALL, "GetAdapterDisplayModeEx returned %#x instead of D3DERR_INVALIDCALL\n", hr);
    /*valid count and valid Size*/
    hr = IDirect3D9Ex_GetAdapterDisplayModeEx(d3d9ex, D3DADAPTER_DEFAULT, &mode_ex, &rotation);
    ok(SUCCEEDED(hr), "GetAdapterDisplayModeEx failed, hr %#x.\n", hr);

    /* Compare what GetAdapterDisplayMode returns with what GetAdapterDisplayModeEx returns*/
    hr = IDirect3D9_GetAdapterDisplayMode(d3d9, D3DADAPTER_DEFAULT, &mode);
    ok(SUCCEEDED(hr), "GetAdapterDisplayMode failed, hr %#x.\n", hr);

    ok(mode_ex.Size == sizeof(D3DDISPLAYMODEEX), "size is %d\n", mode_ex.Size);
    ok(mode_ex.Width == mode.Width, "width is %d instead of %d\n", mode_ex.Width, mode.Width);
    ok(mode_ex.Height == mode.Height, "height is %d instead of %d\n", mode_ex.Height, mode.Height);
    ok(mode_ex.RefreshRate == mode.RefreshRate, "RefreshRate is %d instead of %d\n",
            mode_ex.RefreshRate, mode.RefreshRate);
    ok(mode_ex.Format == mode.Format, "format is %x instead of %x\n", mode_ex.Format, mode.Format);
    /* Don't know yet how to test for ScanLineOrdering, just testing that it
     * is set to a value by GetAdapterDisplayModeEx(). */
    ok(mode_ex.ScanLineOrdering != 0, "ScanLineOrdering returned 0\n");
    /* Check that orientation is returned correctly by GetAdapterDisplayModeEx
     * and EnumDisplaySettingsEx(). */
    todo_wine ok(S2(U1(startmode)).dmDisplayOrientation == DMDO_180 && rotation == D3DDISPLAYROTATION_180,
            "rotation is %d instead of %d\n", rotation, S2(U1(startmode)).dmDisplayOrientation);

    trace("GetAdapterDisplayModeEx returned Width = %d,Height = %d, RefreshRate = %d, Format = %x, ScanLineOrdering = %x, rotation = %d\n",
          mode_ex.Width, mode_ex.Height, mode_ex.RefreshRate, mode_ex.Format, mode_ex.ScanLineOrdering, rotation);

    /* test GetAdapterDisplayModeEx with null pointer for D3DDISPLAYROTATION */
    memset(&mode_ex, 0, sizeof(mode_ex));
    mode_ex.Size = sizeof(D3DDISPLAYMODEEX);

    hr = IDirect3D9Ex_GetAdapterDisplayModeEx(d3d9ex, D3DADAPTER_DEFAULT, &mode_ex, NULL);
    ok(SUCCEEDED(hr), "GetAdapterDisplayModeEx failed, hr %#x.\n", hr);

    ok(mode_ex.Size == sizeof(D3DDISPLAYMODEEX), "size is %d\n", mode_ex.Size);
    ok(mode_ex.Width == mode.Width, "width is %d instead of %d\n", mode_ex.Width, mode.Width);
    ok(mode_ex.Height == mode.Height, "height is %d instead of %d\n", mode_ex.Height, mode.Height);
    ok(mode_ex.RefreshRate == mode.RefreshRate, "RefreshRate is %d instead of %d\n",
            mode_ex.RefreshRate, mode.RefreshRate);
    ok(mode_ex.Format == mode.Format, "format is %x instead of %x\n", mode_ex.Format, mode.Format);
    /* Don't know yet how to test for ScanLineOrdering, just testing that it
     * is set to a value by GetAdapterDisplayModeEx(). */
    ok(mode_ex.ScanLineOrdering != 0, "ScanLineOrdering returned 0\n");

    /* return to the default mode */
    pChangeDisplaySettingsExA(NULL, NULL, NULL, 0, NULL);
out:
    IDirect3D9_Release(d3d9);
    IDirect3D9Ex_Release(d3d9ex);
}

static void test_texture_sysmem_create(void)
{
    IDirect3DDevice9Ex *device;
    IDirect3DTexture9 *texture;
    D3DLOCKED_RECT locked_rect;
    IDirect3D9Ex *d3d9;
    UINT refcount;
    HWND window;
    HRESULT hr;
    void *mem;

    if (FAILED(hr = pDirect3DCreate9Ex(D3D_SDK_VERSION, &d3d9)))
    {
        skip("Failed to create IDirect3D9Ex object (hr %#x), skipping tests.\n", hr);
        return;
    }

    window = create_window();
    device = create_device(d3d9, window, window, TRUE);
    if (!device)
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    mem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 128 * 128 * 4);
    hr = IDirect3DDevice9Ex_CreateTexture(device, 128, 128, 0, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_SYSTEMMEM, &texture, &mem);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CreateTexture(device, 128, 128, 1, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_SYSTEMMEM, &texture, &mem);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    hr = IDirect3DTexture9_LockRect(texture, 0, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock texture, hr %#x.\n", hr);
    ok(locked_rect.Pitch == 128 * 4, "Got unexpected pitch %d.\n", locked_rect.Pitch);
    ok(locked_rect.pBits == mem, "Got unexpected pBits %p, expected %p.\n", locked_rect.pBits, mem);
    hr = IDirect3DTexture9_UnlockRect(texture, 0);
    ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#x.\n", hr);
    IDirect3DTexture9_Release(texture);
    HeapFree(GetProcessHeap(), 0, mem);

    refcount = IDirect3DDevice9Ex_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);

done:
    DestroyWindow(window);
    if (d3d9)
        IDirect3D9Ex_Release(d3d9);
}

START_TEST(d3d9ex)
{
    d3d9_handle = LoadLibraryA("d3d9.dll");
    if (!d3d9_handle)
    {
        skip("Could not load d3d9.dll\n");
        return;
    }
    pDirect3DCreate9 = (void *)GetProcAddress(d3d9_handle, "Direct3DCreate9");
    ok(pDirect3DCreate9 != NULL, "Failed to get address of Direct3DCreate9\n");
    if(!pDirect3DCreate9) {
        return;
    }

    pDirect3DCreate9Ex = (void *)GetProcAddress(d3d9_handle, "Direct3DCreate9Ex");
    if (!pDirect3DCreate9Ex) {
        win_skip("Failed to get address of Direct3DCreate9Ex\n");
        return;
    }

    test_qi_base_to_ex();
    test_qi_ex_to_base();
    test_get_adapter_luid();
    test_get_adapter_displaymode_ex();
    test_texture_sysmem_create();
}
