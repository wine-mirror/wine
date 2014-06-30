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

static BOOL (WINAPI *pEnumDisplaySettingsExA)(const char *device_name,
        DWORD mode_idx, DEVMODEA *mode, DWORD flags);
static LONG (WINAPI *pChangeDisplaySettingsExA)(const char *device_name,
        DEVMODEA *mode, HWND window, DWORD flags, void *param);

static HRESULT (WINAPI *pDirect3DCreate9Ex)(UINT SDKVersion, IDirect3D9Ex **d3d9ex);

static HWND create_window(void)
{
    WNDCLASSA wc = {0};

    wc.lpfnWndProc = DefWindowProcA;
    wc.lpszClassName = "d3d9_test_wc";
    RegisterClassA(&wc);

    return CreateWindowA("d3d9_test_wc", "d3d9_test", WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION,
            0, 0, 640, 480, 0, 0, 0, 0);
}

static IDirect3DDevice9Ex *create_device(HWND device_window, HWND focus_window, BOOL windowed)
{
    D3DPRESENT_PARAMETERS present_parameters = {0};
    IDirect3DDevice9Ex *device;
    D3DDISPLAYMODEEX mode, *m;
    IDirect3D9Ex *d3d9;

    if (FAILED(pDirect3DCreate9Ex(D3D_SDK_VERSION, &d3d9)))
        return NULL;

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
            D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, m, &device))) goto done;

    present_parameters.AutoDepthStencilFormat = D3DFMT_D16;
    if (SUCCEEDED(IDirect3D9Ex_CreateDeviceEx(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, focus_window,
            D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, m, &device))) goto done;

    if (SUCCEEDED(IDirect3D9Ex_CreateDeviceEx(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, focus_window,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, m, &device))) goto done;

    device = NULL;

done:
    IDirect3D9Ex_Release(d3d9);
    return device;
}

static HRESULT reset_device(IDirect3DDevice9Ex *device, HWND device_window, BOOL windowed)
{
    D3DPRESENT_PARAMETERS present_parameters = {0};

    present_parameters.Windowed = windowed;
    present_parameters.hDeviceWindow = device_window;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.BackBufferWidth = 1024;
    present_parameters.BackBufferHeight = 768;
    present_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;

    return IDirect3DDevice9_Reset(device, &present_parameters);
}

static ULONG getref(IUnknown *obj) {
    IUnknown_AddRef(obj);
    return IUnknown_Release(obj);
}

static void test_qi_base_to_ex(void)
{
    IDirect3D9 *d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    IDirect3D9Ex *d3d9ex = (void *) 0xdeadbeef;
    IDirect3DDevice9 *device;
    IDirect3DDevice9Ex *deviceEx = (void *) 0xdeadbeef;
    IDirect3DSwapChain9 *swapchain = NULL;
    IDirect3DSwapChain9Ex *swapchainEx = (void *)0xdeadbeef;
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

    /* Get the implicit swapchain */
    hr = IDirect3DDevice9_GetSwapChain(device, 0, &swapchain);
    ok(SUCCEEDED(hr), "Failed to get the implicit swapchain (%08x).\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IDirect3DSwapChain9_QueryInterface(swapchain, &IID_IDirect3DSwapChain9Ex, (void **)&swapchainEx);
        ok(hr == E_NOINTERFACE,
                "IDirect3DSwapChain9::QueryInterface for IID_IDirect3DSwapChain9Ex returned %08x, expected E_NOINTERFACE.\n",
                hr);
        ok(swapchainEx == NULL, "QueryInterface returned interface %p, expected NULL.\n", swapchainEx);
        if (swapchainEx)
            IDirect3DSwapChain9Ex_Release(swapchainEx);
    }
    if (swapchain)
        IDirect3DSwapChain9_Release(swapchain);

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
    IDirect3DSwapChain9 *swapchain = NULL;
    IDirect3DSwapChain9Ex *swapchainEx = (void *)0xdeadbeef;
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

    /* Get the implicit swapchain */
    hr = IDirect3DDevice9_GetSwapChain(device, 0, &swapchain);
    ok(SUCCEEDED(hr), "Failed to get the implicit swapchain (%08x).\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IDirect3DSwapChain9_QueryInterface(swapchain, &IID_IDirect3DSwapChain9Ex, (void **)&swapchainEx);
        ok(hr == D3D_OK,
                "IDirect3DSwapChain9::QueryInterface for IID_IDirect3DSwapChain9Ex returned %08x, expected D3D_OK.\n",
                hr);
        ok(swapchainEx != NULL && swapchainEx != (void *)0xdeadbeef,
                "QueryInterface returned interface %p, expected != NULL && != 0xdeadbeef.\n", swapchainEx);
        if (swapchainEx)
            IDirect3DSwapChain9Ex_Release(swapchainEx);
    }
    if (swapchain)
        IDirect3DSwapChain9_Release(swapchain);

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

static void test_swapchain_get_displaymode_ex(void)
{
    IDirect3DSwapChain9 *swapchain = NULL;
    IDirect3DSwapChain9Ex *swapchainEx = NULL;
    IDirect3DDevice9Ex *device;
    D3DDISPLAYMODE mode;
    D3DDISPLAYMODEEX mode_ex;
    D3DDISPLAYROTATION rotation;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping swapchain GetDisplayModeEx tests.\n");
        goto out;
    }

    /* Get the implicit swapchain */
    hr = IDirect3DDevice9Ex_GetSwapChain(device, 0, &swapchain);
    if (FAILED(hr))
    {
        skip("Failed to get the implicit swapchain, skipping swapchain GetDisplayModeEx tests.\n");
        goto out;
    }

    hr = IDirect3DSwapChain9_QueryInterface(swapchain, &IID_IDirect3DSwapChain9Ex, (void **)&swapchainEx);
    IDirect3DSwapChain9_Release(swapchain);
    if (FAILED(hr))
    {
        skip("Failed to QI for IID_IDirect3DSwapChain9Ex, skipping swapchain GetDisplayModeEx tests.\n");
        goto out;
    }

    /* invalid size */
    memset(&mode_ex, 0, sizeof(mode_ex));
    hr = IDirect3DSwapChain9Ex_GetDisplayModeEx(swapchainEx, &mode_ex, &rotation);
    ok(hr == D3DERR_INVALIDCALL, "GetDisplayModeEx returned %#x instead of D3DERR_INVALIDCALL.\n", hr);

    mode_ex.Size = sizeof(D3DDISPLAYMODEEX);
    rotation = (D3DDISPLAYROTATION)0xdeadbeef;
    /* valid count and valid size */
    hr = IDirect3DSwapChain9Ex_GetDisplayModeEx(swapchainEx, &mode_ex, &rotation);
    ok(SUCCEEDED(hr), "GetDisplayModeEx failed, hr %#x.\n", hr);

    /* compare what GetDisplayMode returns with what GetDisplayModeEx returns */
    hr = IDirect3DSwapChain9Ex_GetDisplayMode(swapchainEx, &mode);
    ok(SUCCEEDED(hr), "GetDisplayMode failed, hr %#x.\n", hr);

    ok(mode_ex.Size == sizeof(D3DDISPLAYMODEEX), "Size is %d.\n", mode_ex.Size);
    ok(mode_ex.Width == mode.Width, "Width is %d instead of %d.\n", mode_ex.Width, mode.Width);
    ok(mode_ex.Height == mode.Height, "Height is %d instead of %d.\n", mode_ex.Height, mode.Height);
    ok(mode_ex.RefreshRate == mode.RefreshRate, "RefreshRate is %d instead of %d.\n",
            mode_ex.RefreshRate, mode.RefreshRate);
    ok(mode_ex.Format == mode.Format, "Format is %x instead of %x.\n", mode_ex.Format, mode.Format);
    /* Don't know yet how to test for ScanLineOrdering, just testing that it
     * is set to a value by GetDisplayModeEx(). */
    ok(mode_ex.ScanLineOrdering != 0, "ScanLineOrdering returned 0.\n");
    /* Don't know how to compare the rotation in this case, test that it is set */
    ok(rotation != (D3DDISPLAYROTATION)0xdeadbeef, "rotation is %d, expected != 0xdeadbeef.\n", rotation);

    trace("GetDisplayModeEx returned Width = %d, Height = %d, RefreshRate = %d, Format = %x, ScanLineOrdering = %x, rotation = %d.\n",
          mode_ex.Width, mode_ex.Height, mode_ex.RefreshRate, mode_ex.Format, mode_ex.ScanLineOrdering, rotation);

    /* test GetDisplayModeEx with null pointer for D3DDISPLAYROTATION */
    memset(&mode_ex, 0, sizeof(mode_ex));
    mode_ex.Size = sizeof(D3DDISPLAYMODEEX);

    hr = IDirect3DSwapChain9Ex_GetDisplayModeEx(swapchainEx, &mode_ex, NULL);
    ok(SUCCEEDED(hr), "GetDisplayModeEx failed, hr %#x.\n", hr);

    ok(mode_ex.Size == sizeof(D3DDISPLAYMODEEX), "Size is %d.\n", mode_ex.Size);
    ok(mode_ex.Width == mode.Width, "Width is %d instead of %d.\n", mode_ex.Width, mode.Width);
    ok(mode_ex.Height == mode.Height, "Height is %d instead of %d.\n", mode_ex.Height, mode.Height);
    ok(mode_ex.RefreshRate == mode.RefreshRate, "RefreshRate is %d instead of %d.\n",
            mode_ex.RefreshRate, mode.RefreshRate);
    ok(mode_ex.Format == mode.Format, "Format is %x instead of %x.\n", mode_ex.Format, mode.Format);
    /* Don't know yet how to test for ScanLineOrdering, just testing that it
     * is set to a value by GetDisplayModeEx(). */
    ok(mode_ex.ScanLineOrdering != 0, "ScanLineOrdering returned 0.\n");

    IDirect3DSwapChain9Ex_Release(swapchainEx);

out:
    if (device)
        IDirect3DDevice9Ex_Release(device);
    DestroyWindow(window);
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

static void test_user_memory(void)
{
    IDirect3DDevice9Ex *device;
    IDirect3DTexture9 *texture;
    IDirect3DCubeTexture9 *cube_texture;
    IDirect3DVolumeTexture9 *volume_texture;
    IDirect3DVertexBuffer9 *vertex_buffer;
    IDirect3DIndexBuffer9 *index_buffer;
    IDirect3DSurface9 *surface;
    D3DLOCKED_RECT locked_rect;
    UINT refcount;
    HWND window;
    HRESULT hr;
    void *mem;
    D3DCAPS9 caps;

    window = create_window();
    if (!(device = create_device(window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);

    mem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 128 * 128 * 4);
    hr = IDirect3DDevice9Ex_CreateTexture(device, 128, 128, 0, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_SYSTEMMEM, &texture, &mem);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CreateTexture(device, 1, 1, 0, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_SYSTEMMEM, &texture, &mem);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CreateTexture(device, 128, 128, 2, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_SYSTEMMEM, &texture, &mem);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CreateTexture(device, 128, 128, 1, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_SCRATCH, &texture, &mem);
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

    if (caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP)
    {
        hr = IDirect3DDevice9Ex_CreateCubeTexture(device, 2, 1, 0, D3DFMT_A8R8G8B8,
                D3DPOOL_SYSTEMMEM, &cube_texture, &mem);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    }
    if (caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP)
    {
        hr = IDirect3DDevice9Ex_CreateVolumeTexture(device, 2, 2, 2, 1, 0, D3DFMT_A8R8G8B8,
                D3DPOOL_SYSTEMMEM, &volume_texture, &mem);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    }

    hr = IDirect3DDevice9Ex_CreateIndexBuffer(device, 16, 0, D3DFMT_INDEX32, D3DPOOL_SYSTEMMEM,
            &index_buffer, &mem);
    ok(hr == D3DERR_NOTAVAILABLE, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CreateVertexBuffer(device, 16, 0, 0, D3DPOOL_SYSTEMMEM,
            &vertex_buffer, &mem);
    ok(hr == D3DERR_NOTAVAILABLE, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurface(device, 128, 128, D3DFMT_A8R8G8B8,
            D3DPOOL_SYSTEMMEM, &surface, &mem);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    ok(locked_rect.Pitch == 128 * 4, "Got unexpected pitch %d.\n", locked_rect.Pitch);
    ok(locked_rect.pBits == mem, "Got unexpected pBits %p, expected %p.\n", locked_rect.pBits, mem);
    hr = IDirect3DSurface9_UnlockRect(surface);
    IDirect3DSurface9_Release(surface);

    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurfaceEx(device, 128, 128, D3DFMT_A8R8G8B8,
            D3DPOOL_SYSTEMMEM, &surface, &mem, 0);
    todo_wine ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, 0);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
        ok(locked_rect.Pitch == 128 * 4, "Got unexpected pitch %d.\n", locked_rect.Pitch);
        ok(locked_rect.pBits == mem, "Got unexpected pBits %p, expected %p.\n", locked_rect.pBits, mem);
        hr = IDirect3DSurface9_UnlockRect(surface);
        IDirect3DSurface9_Release(surface);
    }

    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurface(device, 128, 128, D3DFMT_A8R8G8B8,
            D3DPOOL_SCRATCH, &surface, &mem);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurfaceEx(device, 128, 128, D3DFMT_A8R8G8B8,
            D3DPOOL_SCRATCH, &surface, &mem, 0);
    todo_wine ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    HeapFree(GetProcessHeap(), 0, mem);
    refcount = IDirect3DDevice9Ex_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);

done:
    DestroyWindow(window);
}

static void test_reset(void)
{
    static const DWORD simple_vs[] =
    {
        0xfffe0101,                                     /* vs_1_1             */
        0x0000001f, 0x80000000, 0x900f0000,             /* dcl_position0 v0   */
        0x00000009, 0xc0010000, 0x90e40000, 0xa0e40000, /* dp4 oPos.x, v0, c0 */
        0x00000009, 0xc0020000, 0x90e40000, 0xa0e40001, /* dp4 oPos.y, v0, c1 */
        0x00000009, 0xc0040000, 0x90e40000, 0xa0e40002, /* dp4 oPos.z, v0, c2 */
        0x00000009, 0xc0080000, 0x90e40000, 0xa0e40003, /* dp4 oPos.w, v0, c3 */
        0x0000ffff,                                     /* end                */
    };

    DWORD height, orig_height = GetSystemMetrics(SM_CYSCREEN);
    DWORD width, orig_width = GetSystemMetrics(SM_CXSCREEN);
    IDirect3DVertexShader9 *shader;
    IDirect3DSwapChain9 *swapchain;
    D3DDISPLAYMODE d3ddm, d3ddm2;
    D3DPRESENT_PARAMETERS d3dpp;
    IDirect3DDevice9Ex *device;
    IDirect3DSurface9 *surface;
    UINT i, adapter_mode_count;
    IDirect3D9 *d3d9;
    D3DVIEWPORT9 vp;
    D3DCAPS9 caps;
    UINT refcount;
    DWORD value;
    HWND window;
    HRESULT hr;
    RECT rect;
    struct
    {
        UINT w;
        UINT h;
    } *modes = NULL;
    UINT mode_count = 0;

    window = create_window();
    if (!(device = create_device(window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9Ex_GetDirect3D(device, &d3d9);
    ok(SUCCEEDED(hr), "Failed to get d3d9, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);

    IDirect3D9_GetAdapterDisplayMode(d3d9, D3DADAPTER_DEFAULT, &d3ddm);
    adapter_mode_count = IDirect3D9_GetAdapterModeCount(d3d9, D3DADAPTER_DEFAULT, d3ddm.Format);
    modes = HeapAlloc(GetProcessHeap(), 0, sizeof(*modes) * adapter_mode_count);
    for (i = 0; i < adapter_mode_count; ++i)
    {
        UINT j;

        hr = IDirect3D9_EnumAdapterModes(d3d9, D3DADAPTER_DEFAULT, d3ddm.Format, i, &d3ddm2);
        ok(SUCCEEDED(hr), "Failed to enumerate display mode, hr %#x.\n", hr);

        for (j = 0; j < mode_count; ++j)
        {
            if (modes[j].w == d3ddm2.Width && modes[j].h == d3ddm2.Height)
                break;
        }
        if (j == mode_count)
        {
            modes[j].w = d3ddm2.Width;
            modes[j].h = d3ddm2.Height;
            ++mode_count;
        }

        /* We use them as invalid modes. */
        if ((d3ddm2.Width == 801 && d3ddm2.Height == 600)
                || (d3ddm2.Width == 32 && d3ddm2.Height == 32))
        {
            skip("This system supports a screen resolution of %dx%d, not running mode tests.\n",
                 d3ddm2.Width, d3ddm2.Height);
            goto cleanup;
        }
    }

    if (mode_count < 2)
    {
        skip("Less than 2 modes supported, skipping mode tests.\n");
        goto cleanup;
    }

    i = 0;
    if (modes[i].w == orig_width && modes[i].h == orig_height)
        ++i;

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.Windowed = FALSE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferWidth = modes[i].w;
    d3dpp.BackBufferHeight = modes[i].h;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected cooperative level %#x.\n", hr);

    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == modes[i].w, "Got screen width %u, expected %u.\n", width, modes[i].w);
    ok(height == modes[i].h, "Got screen height %u, expected %u.\n", height, modes[i].h);

    hr = IDirect3DDevice9Ex_GetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to get scissor rect, hr %#x.\n", hr);
    ok(rect.left == 0 && rect.top == 0 && rect.right == modes[i].w && rect.bottom == modes[i].h,
            "Got unexpected scissor rect {%d, %d, %d, %d}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    hr = IDirect3DDevice9Ex_GetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to get viewport, hr %#x.\n", hr);
    ok(vp.X == 0, "Got unexpected vp.X %u.\n", vp.X);
    ok(vp.Y == 0, "Got unexpected vp.Y %u.\n", vp.Y);
    ok(vp.Width == modes[i].w, "Got vp.Width %u, expected %u.\n", vp.Width, modes[i].w);
    ok(vp.Height == modes[i].h, "Got vp.Height %u, expected %u.\n", vp.Height, modes[i].h);
    ok(vp.MinZ == 0.0f, "Got unexpected vp.MinZ %.8e.\n", vp.MinZ);
    ok(vp.MaxZ == 1.0f, "Got unexpected vp,MaxZ %.8e.\n", vp.MaxZ);

    i = 1;
    vp.X = 10;
    vp.Y = 20;
    vp.MinZ = 2.0f;
    vp.MaxZ = 3.0f;
    hr = IDirect3DDevice9Ex_SetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#x.\n", hr);

    SetRect(&rect, 10, 20, 30, 40);
    hr = IDirect3DDevice9Ex_SetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to set scissor rect, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetRenderState(device, D3DRS_LIGHTING, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
    ok(!!value, "Got unexpected value %#x for D3DRS_LIGHTING.\n", value);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = FALSE;
    d3dpp.BackBufferWidth = modes[i].w;
    d3dpp.BackBufferHeight = modes[i].h;
    d3dpp.BackBufferFormat = d3ddm.Format;
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected cooperative level %#x.\n", hr);

    /* Render states are preserved in d3d9ex. */
    hr = IDirect3DDevice9_GetRenderState(device, D3DRS_LIGHTING, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
    ok(!value, "Got unexpected value %#x for D3DRS_LIGHTING.\n", value);

    hr = IDirect3DDevice9Ex_GetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to get scissor rect, hr %#x.\n", hr);
    ok(rect.left == 0 && rect.top == 0 && rect.right == modes[i].w && rect.bottom == modes[i].h,
            "Got unexpected scissor rect {%d, %d, %d, %d}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    hr = IDirect3DDevice9Ex_GetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to get viewport, hr %#x.\n", hr);
    ok(vp.X == 0, "Got unexpected vp.X %u.\n", vp.X);
    ok(vp.Y == 0, "Got unexpected vp.Y %u.\n", vp.Y);
    ok(vp.Width == modes[i].w, "Got vp.Width %u, expected %u.\n", vp.Width, modes[i].w);
    ok(vp.Height == modes[i].h, "Got vp.Height %u, expected %u.\n", vp.Height, modes[i].h);
    ok(vp.MinZ == 2.0f, "Got unexpected vp.MinZ %.8e.\n", vp.MinZ);
    ok(vp.MaxZ == 3.0f, "Got unexpected vp,MaxZ %.8e.\n", vp.MaxZ);

    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == modes[i].w, "Got screen width %u, expected %u.\n", width, modes[i].w);
    ok(height == modes[i].h, "Got screen height %u, expected %u.\n", height, modes[i].h);

    hr = IDirect3DDevice9Ex_GetSwapChain(device, 0, &swapchain);
    ok(SUCCEEDED(hr), "Failed to get swapchain, hr %#x.\n", hr);
    hr = IDirect3DSwapChain9_GetPresentParameters(swapchain, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to get present parameters, hr %#x.\n", hr);
    ok(d3dpp.BackBufferWidth == modes[i].w, "Got backbuffer width %u, expected %u.\n",
            d3dpp.BackBufferWidth, modes[i].w);
    ok(d3dpp.BackBufferHeight == modes[i].h, "Got backbuffer height %u, expected %u.\n",
            d3dpp.BackBufferHeight, modes[i].h);
    IDirect3DSwapChain9_Release(swapchain);

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = TRUE;
    d3dpp.BackBufferWidth = 400;
    d3dpp.BackBufferHeight = 300;
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected cooperative level %#x.\n", hr);

    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == orig_width, "Got screen width %u, expected %u.\n", width, orig_width);
    ok(height == orig_height, "Got screen height %u, expected %u.\n", height, orig_height);

    hr = IDirect3DDevice9Ex_GetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to get scissor rect, hr %#x.\n", hr);
    ok(rect.left == 0 && rect.top == 0 && rect.right == 400 && rect.bottom == 300,
            "Got unexpected scissor rect {%d, %d, %d, %d}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    hr = IDirect3DDevice9Ex_GetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to get viewport, hr %#x.\n", hr);
    ok(vp.X == 0, "Got unexpected vp.X %u.\n", vp.X);
    ok(vp.Y == 0, "Got unexpected vp.Y %u.\n", vp.Y);
    ok(vp.Width == 400, "Got unexpected vp.Width %u.\n", vp.Width);
    ok(vp.Height == 300, "Got unexpected vp.Height %u.\n", vp.Height);
    ok(vp.MinZ == 2.0f, "Got unexpected vp.MinZ %.8e.\n", vp.MinZ);
    ok(vp.MaxZ == 3.0f, "Got unexpected vp,MaxZ %.8e.\n", vp.MaxZ);

    hr = IDirect3DDevice9Ex_GetSwapChain(device, 0, &swapchain);
    ok(SUCCEEDED(hr), "Failed to get swapchain, hr %#x.\n", hr);
    hr = IDirect3DSwapChain9_GetPresentParameters(swapchain, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to get present parameters, hr %#x.\n", hr);
    ok(d3dpp.BackBufferWidth == 400, "Got unexpected backbuffer width %u.\n", d3dpp.BackBufferWidth);
    ok(d3dpp.BackBufferHeight == 300, "Got unexpected backbuffer height %u.\n", d3dpp.BackBufferHeight);
    IDirect3DSwapChain9_Release(swapchain);

    SetRect(&rect, 0, 0, 200, 150);
    ok(AdjustWindowRect(&rect, GetWindowLongW(window, GWL_STYLE), FALSE), "Failed to adjust window rect.\n");
    ok(SetWindowPos(window, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
            SWP_NOMOVE | SWP_NOZORDER), "Failed to set window position.\n");

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = TRUE;
    d3dpp.BackBufferWidth = 0;
    d3dpp.BackBufferHeight = 0;
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected cooperative level %#x.\n", hr);

    hr = IDirect3DDevice9Ex_GetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to get scissor rect, hr %#x.\n", hr);
    ok(rect.left == 0 && rect.top == 0 && rect.right == 200 && rect.bottom == 150,
            "Got unexpected scissor rect {%d, %d, %d, %d}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    hr = IDirect3DDevice9Ex_GetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to get viewport, hr %#x.\n", hr);
    ok(vp.X == 0, "Got unexpected vp.X %u.\n", vp.X);
    ok(vp.Y == 0, "Got unexpected vp.Y %u.\n", vp.Y);
    ok(vp.Width == 200, "Got unexpected vp.Width %u.\n", vp.Width);
    ok(vp.Height == 150, "Got unexpected vp.Height %u.\n", vp.Height);
    ok(vp.MinZ == 2.0f, "Got unexpected vp.MinZ %.8e.\n", vp.MinZ);
    ok(vp.MaxZ == 3.0f, "Got unexpected vp,MaxZ %.8e.\n", vp.MaxZ);

    hr = IDirect3DDevice9Ex_GetSwapChain(device, 0, &swapchain);
    ok(SUCCEEDED(hr), "Failed to get swapchain, hr %#x.\n", hr);
    hr = IDirect3DSwapChain9_GetPresentParameters(swapchain, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to get present parameters, hr %#x.\n", hr);
    ok(d3dpp.BackBufferWidth == 200, "Got unexpected backbuffer width %u.\n", d3dpp.BackBufferWidth);
    ok(d3dpp.BackBufferHeight == 150, "Got unexpected backbuffer height %u.\n", d3dpp.BackBufferHeight);
    IDirect3DSwapChain9_Release(swapchain);

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = TRUE;
    d3dpp.BackBufferWidth = 400;
    d3dpp.BackBufferHeight = 300;

    /* Reset with resources in the default pool succeeds in d3d9ex. */
    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurface(device, 16, 16,
            D3DFMT_R5G6B5, D3DPOOL_DEFAULT, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected cooperative level %#x.\n", hr);
    IDirect3DSurface9_Release(surface);

    if (caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP)
    {
        IDirect3DVolumeTexture9 *volume_texture;

        hr = IDirect3DDevice9Ex_CreateVolumeTexture(device, 16, 16, 4, 1, 0,
                D3DFMT_R5G6B5, D3DPOOL_DEFAULT, &volume_texture, NULL);
        ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);
        hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
        ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);
        hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
        ok(hr == D3D_OK, "Got unexpected cooperative level %#x.\n", hr);
        IDirect3DVolumeTexture9_Release(volume_texture);
    }
    else
    {
        skip("Volume textures not supported.\n");
    }

    /* Scratch and sysmem pools are fine too. */
    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurface(device, 16, 16,
            D3DFMT_R5G6B5, D3DPOOL_SCRATCH, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected cooperative level %#x.\n", hr);
    IDirect3DSurface9_Release(surface);

    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurface(device, 16, 16,
            D3DFMT_R5G6B5, D3DPOOL_SYSTEMMEM, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected cooperative level %#x.\n", hr);
    IDirect3DSurface9_Release(surface);

    /* The depth stencil should get reset to the auto depth stencil when present. */
    hr = IDirect3DDevice9Ex_SetDepthStencilSurface(device, NULL);
    ok(SUCCEEDED(hr), "Failed to set depth/stencil surface, hr %#x.\n", hr);

    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_GetDepthStencilSurface(device, &surface);
    ok(SUCCEEDED(hr), "Failed to get depth/stencil surface, hr %#x.\n", hr);
    ok(!!surface, "Depth/stencil surface should not be NULL.\n");
    IDirect3DSurface9_Release(surface);

    d3dpp.EnableAutoDepthStencil = FALSE;
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_GetDepthStencilSurface(device, &surface);
    ok(hr == D3DERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    ok(!surface, "Depth/stencil surface should be NULL.\n");

    /* References to implicit surfaces are allowed in d3d9ex. */
    hr = IDirect3DDevice9Ex_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &surface);
    ok(SUCCEEDED(hr), "Failed to get backbuffer, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected cooperative level %#x.\n", hr);
    IDirect3DSurface9_Release(surface);

    /* Shaders are fine. */
    hr = IDirect3DDevice9Ex_CreateVertexShader(device, simple_vs, &shader);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);
    IDirect3DVertexShader9_Release(shader);

    /* Try setting invalid modes. */
    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = FALSE;
    d3dpp.BackBufferWidth = 32;
    d3dpp.BackBufferHeight = 32;
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected cooperative level %#x.\n", hr);

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = FALSE;
    d3dpp.BackBufferWidth = 801;
    d3dpp.BackBufferHeight = 600;
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected cooperative level %#x.\n", hr);

    hr = IDirect3D9_GetAdapterDisplayMode(d3d9, D3DADAPTER_DEFAULT, &d3ddm);
    ok(SUCCEEDED(hr), "Failed to get display mode, hr %#x.\n", hr);

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.EnableAutoDepthStencil = FALSE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;

cleanup:
    HeapFree(GetProcessHeap(), 0, modes);
    IDirect3D9_Release(d3d9);
    refcount = IDirect3DDevice9Ex_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_reset_resources(void)
{
    IDirect3DSurface9 *surface, *rt;
    IDirect3DTexture9 *texture;
    IDirect3DDevice9Ex *device;
    unsigned int i;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;
    ULONG ref;

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateDepthStencilSurface(device, 128, 128, D3DFMT_D24S8,
            D3DMULTISAMPLE_NONE, 0, TRUE, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create depth/stencil surface, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetDepthStencilSurface(device, surface);
    ok(SUCCEEDED(hr), "Failed to set depth/stencil surface, hr %#x.\n", hr);
    IDirect3DSurface9_Release(surface);

    for (i = 0; i < caps.NumSimultaneousRTs; ++i)
    {
        hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 1, D3DUSAGE_RENDERTARGET,
                D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL);
        ok(SUCCEEDED(hr), "Failed to create render target texture %u, hr %#x.\n", i, hr);
        hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
        ok(SUCCEEDED(hr), "Failed to get surface %u, hr %#x.\n", i, hr);
        IDirect3DTexture9_Release(texture);
        hr = IDirect3DDevice9_SetRenderTarget(device, i, surface);
        ok(SUCCEEDED(hr), "Failed to set render target surface %u, hr %#x.\n", i, hr);
        IDirect3DSurface9_Release(surface);
    }

    hr = reset_device(device, window, TRUE);
    ok(SUCCEEDED(hr), "Failed to reset device.\n");

    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &rt);
    ok(SUCCEEDED(hr), "Failed to get back buffer, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &surface);
    ok(SUCCEEDED(hr), "Failed to get render target surface, hr %#x.\n", hr);
    ok(surface == rt, "Got unexpected surface %p for render target.\n", surface);
    IDirect3DSurface9_Release(surface);
    IDirect3DSurface9_Release(rt);

    for (i = 1; i < caps.NumSimultaneousRTs; ++i)
    {
        hr = IDirect3DDevice9_GetRenderTarget(device, i, &surface);
        ok(hr == D3DERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    }

    ref = IDirect3DDevice9_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

done:
    DestroyWindow(window);
}

static void test_vidmem_accounting(void)
{
    IDirect3DDevice9Ex *device;
    unsigned int i;
    HWND window;
    HRESULT hr = D3D_OK;
    ULONG ref;
    UINT vidmem_start, vidmem_end;
    INT diff;
    IDirect3DTexture9 *textures[20];

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    vidmem_start = IDirect3DDevice9_GetAvailableTextureMem(device);
    memset(textures, 0, sizeof(textures));
    for (i = 0; i < 20 && SUCCEEDED(hr); i++)
    {
        hr = IDirect3DDevice9_CreateTexture(device, 1024, 1024, 1, D3DUSAGE_RENDERTARGET,
                D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &textures[i], NULL);
        /* No D3DERR_OUTOFVIDEOMEMORY in d3d9ex */
        ok(SUCCEEDED(hr) || hr == E_OUTOFMEMORY, "Failed to create texture, hr %#x.\n", hr);
    }
    vidmem_end = IDirect3DDevice9_GetAvailableTextureMem(device);

    diff = vidmem_start - vidmem_end;
    diff = abs(diff);
    ok(diff < 1024 * 1024, "Expected a video memory difference of less than 1 MB, got %u MB.\n",
            diff / 1024 / 1024);

    for (i = 0; i < 20; i++)
    {
        if (textures[i])
            IDirect3DTexture9_Release(textures[i]);
    }

    ref = IDirect3DDevice9_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

done:
    DestroyWindow(window);
}

static void test_user_memory_getdc(void)
{
    IDirect3DDevice9Ex *device;
    HWND window;
    HRESULT hr;
    ULONG ref;
    IDirect3DSurface9 *surface;
    DWORD *data;
    HDC dc;

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    data = HeapAlloc(GetProcessHeap(), 0, sizeof(*data) * 16 * 16);
    memset(data, 0xaa, sizeof(*data) * 16 * 16);
    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurface(device, 16, 16,
            D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &surface, (HANDLE *)&data);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirect3DSurface9_GetDC(surface, &dc);
    ok(SUCCEEDED(hr), "Failed to get dc, hr %#x.\n", hr);
    BitBlt(dc, 0, 0, 16, 8, NULL, 0, 0, WHITENESS);
    BitBlt(dc, 0, 8, 16, 8, NULL, 0, 0, BLACKNESS);
    hr = IDirect3DSurface9_ReleaseDC(surface, dc);
    ok(SUCCEEDED(hr), "Failed to release dc, hr %#x.\n", hr);

    ok(data[0] == 0xffffffff, "Expected color 0xffffffff, got %#x.\n", data[0]);
    ok(data[8 * 16] == 0x00000000, "Expected color 0x00000000, got %#x.\n", data[8 * 16]);

    IDirect3DSurface9_Release(surface);
    HeapFree(GetProcessHeap(), 0, data);

    ref = IDirect3DDevice9_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

done:
    DestroyWindow(window);
}

static void test_lost_device(void)
{
    IDirect3DDevice9Ex *device;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    BOOL ret;

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    if (!(device = create_device(window, window, FALSE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    ok(hr == S_PRESENT_OCCLUDED, "Got unexpected hr %#x.\n", hr);

    ret = SetForegroundWindow(GetDesktopWindow());
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == S_PRESENT_OCCLUDED, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    ok(hr == S_PRESENT_OCCLUDED, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    ok(hr == S_PRESENT_OCCLUDED, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    ret = SetForegroundWindow(window);
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    ok(hr == S_PRESENT_OCCLUDED, "Got unexpected hr %#x.\n", hr);

    hr = reset_device(device, window, FALSE);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    ok(hr == S_PRESENT_OCCLUDED, "Got unexpected hr %#x.\n", hr);

    hr = reset_device(device, window, TRUE);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    todo_wine ok(hr == S_PRESENT_MODE_CHANGED, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    todo_wine ok(hr == S_PRESENT_MODE_CHANGED, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    todo_wine ok(hr == S_PRESENT_MODE_CHANGED, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    todo_wine ok(hr == S_PRESENT_MODE_CHANGED, "Got unexpected hr %#x.\n", hr);

    hr = reset_device(device, window, TRUE);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    ret = SetForegroundWindow(GetDesktopWindow());
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    ret = SetForegroundWindow(window);
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = reset_device(device, window, FALSE);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    ok(hr == S_PRESENT_OCCLUDED, "Got unexpected hr %#x.\n", hr);

    refcount = IDirect3DDevice9Ex_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    DestroyWindow(window);
}

START_TEST(d3d9ex)
{
    d3d9_handle = LoadLibraryA("d3d9.dll");
    if (!d3d9_handle)
    {
        skip("Could not load d3d9.dll\n");
        return;
    }

    pDirect3DCreate9Ex = (void *)GetProcAddress(d3d9_handle, "Direct3DCreate9Ex");
    if (!pDirect3DCreate9Ex) {
        win_skip("Failed to get address of Direct3DCreate9Ex\n");
        return;
    }

    test_qi_base_to_ex();
    test_qi_ex_to_base();
    test_swapchain_get_displaymode_ex();
    test_get_adapter_luid();
    test_get_adapter_displaymode_ex();
    test_user_memory();
    test_reset();
    test_reset_resources();
    test_vidmem_accounting();
    test_user_memory_getdc();
    test_lost_device();
}
