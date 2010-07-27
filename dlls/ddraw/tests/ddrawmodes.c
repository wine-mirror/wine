/*
 * Unit tests for ddraw functions
 *
 *
 * Part of this test involves changing the screen resolution. But this is
 * really disrupting if the user is doing something else and is not very nice
 * to CRT screens. Plus, ideally it needs someone watching it to check that
 * each mode displays correctly.
 * So this is only done if the test is being run in interactive mode.
 *
 * Copyright (C) 2003 Sami Aario
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

#include <assert.h>
#include "wine/test.h"
#include "ddraw.h"

static LPDIRECTDRAW lpDD = NULL;
static LPDIRECTDRAWSURFACE lpDDSPrimary = NULL;
static LPDIRECTDRAWSURFACE lpDDSBack = NULL;
static WNDCLASS wc;
static HWND hwnd;
static int modes_cnt;
static int modes_size;
static LPDDSURFACEDESC modes;
static RECT rect_before_create;
static RECT rect_after_delete;

static HRESULT (WINAPI *pDirectDrawEnumerateA)(LPDDENUMCALLBACKA,LPVOID);
static HRESULT (WINAPI *pDirectDrawEnumerateW)(LPDDENUMCALLBACKW,LPVOID);
static HRESULT (WINAPI *pDirectDrawEnumerateExA)(LPDDENUMCALLBACKEXA,LPVOID,DWORD);
static HRESULT (WINAPI *pDirectDrawEnumerateExW)(LPDDENUMCALLBACKEXW,LPVOID,DWORD);

static void init_function_pointers(void)
{
    HMODULE hmod = GetModuleHandleA("ddraw.dll");
    pDirectDrawEnumerateA = (void*)GetProcAddress(hmod, "DirectDrawEnumerateA");
    pDirectDrawEnumerateW = (void*)GetProcAddress(hmod, "DirectDrawEnumerateW");
    pDirectDrawEnumerateExA = (void*)GetProcAddress(hmod, "DirectDrawEnumerateExA");
    pDirectDrawEnumerateExW = (void*)GetProcAddress(hmod, "DirectDrawEnumerateExW");
}

static void createwindow(void)
{
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = DefWindowProcA;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandleA(0);
    wc.hIcon = LoadIconA(wc.hInstance, IDI_APPLICATION);
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "TestWindowClass";
    if(!RegisterClassA(&wc))
        assert(0);

    hwnd = CreateWindowExA(0, "TestWindowClass", "TestWindowClass",
        WS_POPUP, 0, 0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, GetModuleHandleA(0), NULL);
    assert(hwnd != NULL);

    ShowWindow(hwnd, SW_HIDE);
    UpdateWindow(hwnd);
    SetFocus(hwnd);
}

static BOOL createdirectdraw(void)
{
    HRESULT rc;

    SetRect(&rect_before_create, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));

    rc = DirectDrawCreate(NULL, &lpDD, NULL);
    ok(rc==DD_OK || rc==DDERR_NODIRECTDRAWSUPPORT, "DirectDrawCreateEx returned: %x\n", rc);
    if (!lpDD) {
        trace("DirectDrawCreateEx() failed with an error %x\n", rc);
        return FALSE;
    }
    return TRUE;
}


static void releasedirectdraw(void)
{
    if( lpDD != NULL )
    {
        IDirectDraw_Release(lpDD);
        lpDD = NULL;
        SetRect(&rect_after_delete, 0, 0,
                GetSystemMetrics(SM_CXSCREEN),
                GetSystemMetrics(SM_CYSCREEN));
        ok(EqualRect(&rect_before_create, &rect_after_delete) != 0,
           "Original display mode was not restored\n");
    }
}

static BOOL WINAPI crash_callbackA(GUID *lpGUID, LPSTR lpDriverDescription,
                                  LPSTR lpDriverName, LPVOID lpContext)
{
    *(volatile char*)0 = 2;
    return TRUE;
}

static BOOL WINAPI test_nullcontext_callbackA(GUID *lpGUID, LPSTR lpDriverDescription,
                                              LPSTR lpDriverName, LPVOID lpContext)
{
    trace("test_nullcontext_callbackA: %p %s %s %p\n",
          lpGUID, lpDriverDescription, lpDriverName, lpContext);

    ok(!lpContext, "Expected NULL lpContext\n");

    return TRUE;
}

static BOOL WINAPI test_context_callbackA(GUID *lpGUID, LPSTR lpDriverDescription,
                                          LPSTR lpDriverName, LPVOID lpContext)
{
    trace("test_context_callbackA: %p %s %s %p\n",
          lpGUID, lpDriverDescription, lpDriverName, lpContext);

    ok(lpContext == (LPVOID)0xdeadbeef, "Expected non-NULL lpContext\n");

    return TRUE;
}

static void test_DirectDrawEnumerateA(void)
{
    HRESULT ret;

    if (!pDirectDrawEnumerateA)
    {
        win_skip("DirectDrawEnumerateA is not available\n");
        return;
    }

    /* Test with NULL callback parameter. */
    ret = pDirectDrawEnumerateA(NULL, NULL);
    ok(ret == DDERR_INVALIDPARAMS, "Expected DDERR_INVALIDPARAMS, got %d\n", ret);

    /* Test with invalid callback parameter. */
    ret = pDirectDrawEnumerateA((LPDDENUMCALLBACKA)0xdeadbeef, NULL);
    ok(ret == DDERR_INVALIDPARAMS, "Expected DDERR_INVALIDPARAMS, got %d\n", ret);

    if (pDirectDrawEnumerateExA)
    {
        /* Test with callback that crashes. */
        ret = pDirectDrawEnumerateA(crash_callbackA, NULL);
        ok(ret == DDERR_INVALIDPARAMS, "Expected DDERR_INVALIDPARAMS, got %d\n", ret);
    }
    else
        win_skip("Test would crash on older ddraw\n");

    /* Test with valid callback parameter and NULL context parameter. */
    trace("Calling DirectDrawEnumerateA with test_nullcontext_callbackA callback and NULL context.\n");
    ret = pDirectDrawEnumerateA(test_nullcontext_callbackA, NULL);
    ok(ret == DD_OK, "Expected DD_OK, got %d\n", ret);

    /* Test with valid callback parameter and valid context parameter. */
    trace("Calling DirectDrawEnumerateA with test_context_callbackA callback and non-NULL context.\n");
    ret = pDirectDrawEnumerateA(test_context_callbackA, (LPVOID)0xdeadbeef);
    ok(ret == DD_OK, "Expected DD_OK, got %d\n", ret);
}

static BOOL WINAPI test_callbackW(GUID *lpGUID, LPWSTR lpDriverDescription,
                                  LPWSTR lpDriverName, LPVOID lpContext)
{
    ok(0, "The callback should not be invoked by DirectDrawEnumerateW\n");
    return TRUE;
}

static void test_DirectDrawEnumerateW(void)
{
    HRESULT ret;

    if (!pDirectDrawEnumerateW)
    {
        win_skip("DirectDrawEnumerateW is not available\n");
        return;
    }

    /* DirectDrawEnumerateW is not implemented on Windows. */

    /* Test with NULL callback parameter. */
    ret = pDirectDrawEnumerateW(NULL, NULL);
    ok(ret == DDERR_INVALIDPARAMS ||
       ret == DDERR_UNSUPPORTED, /* Older ddraw */
       "Expected DDERR_INVALIDPARAMS or DDERR_UNSUPPORTED, got %d\n", ret);

    /* Test with invalid callback parameter. */
    ret = pDirectDrawEnumerateW((LPDDENUMCALLBACKW)0xdeadbeef, NULL);
    ok(ret == DDERR_INVALIDPARAMS /* XP */ ||
       ret == DDERR_UNSUPPORTED /* Win7 */,
       "Expected DDERR_INVALIDPARAMS or DDERR_UNSUPPORTED, got %d\n", ret);

    /* Test with valid callback parameter and NULL context parameter. */
    ret = pDirectDrawEnumerateW(test_callbackW, NULL);
    ok(ret == DDERR_UNSUPPORTED, "Expected DDERR_UNSUPPORTED, got %d\n", ret);
}

static BOOL WINAPI crash_callbackExA(GUID *lpGUID, LPSTR lpDriverDescription,
                                     LPSTR lpDriverName, LPVOID lpContext,
                                     HMONITOR hm)
{
    *(volatile char*)0 = 2;
    return TRUE;
}

static BOOL WINAPI test_nullcontext_callbackExA(GUID *lpGUID, LPSTR lpDriverDescription,
                                                LPSTR lpDriverName, LPVOID lpContext,
                                                HMONITOR hm)
{
    trace("test_nullcontext_callbackExA: %p %s %s %p %p\n", lpGUID,
          lpDriverDescription, lpDriverName, lpContext, hm);

    ok(!lpContext, "Expected NULL lpContext\n");

    return TRUE;
}

static BOOL WINAPI test_context_callbackExA(GUID *lpGUID, LPSTR lpDriverDescription,
                                            LPSTR lpDriverName, LPVOID lpContext,
                                            HMONITOR hm)
{
    trace("test_context_callbackExA: %p %s %s %p %p\n", lpGUID,
          lpDriverDescription, lpDriverName, lpContext, hm);

    ok(lpContext == (LPVOID)0xdeadbeef, "Expected non-NULL lpContext\n");

    return TRUE;
}

static void test_DirectDrawEnumerateExA(void)
{
    HRESULT ret;

    if (!pDirectDrawEnumerateExA)
    {
        win_skip("DirectDrawEnumerateExA is not available\n");
        return;
    }

    /* Test with NULL callback parameter. */
    ret = pDirectDrawEnumerateExA(NULL, NULL, 0);
    ok(ret == DDERR_INVALIDPARAMS, "Expected DDERR_INVALIDPARAMS, got %d\n", ret);

    /* Test with invalid callback parameter. */
    ret = pDirectDrawEnumerateExA((LPDDENUMCALLBACKEXA)0xdeadbeef, NULL, 0);
    ok(ret == DDERR_INVALIDPARAMS, "Expected DDERR_INVALIDPARAMS, got %d\n", ret);

    /* Test with callback that crashes. */
    ret = pDirectDrawEnumerateExA(crash_callbackExA, NULL, 0);
    ok(ret == DDERR_INVALIDPARAMS, "Expected DDERR_INVALIDPARAMS, got %d\n", ret);

    /* Test with valid callback parameter and invalid flags */
    ret = pDirectDrawEnumerateExA(test_nullcontext_callbackExA, NULL, ~0);
    ok(ret == DDERR_INVALIDPARAMS, "Expected DDERR_INVALIDPARAMS, got %d\n", ret);

    /* Test with valid callback parameter and NULL context parameter. */
    trace("Calling DirectDrawEnumerateExA with empty flags and NULL context.\n");
    ret = pDirectDrawEnumerateExA(test_nullcontext_callbackExA, NULL, 0);
    ok(ret == DD_OK, "Expected DD_OK, got %d\n", ret);

    /* Test with valid callback parameter and non-NULL context parameter. */
    trace("Calling DirectDrawEnumerateExA with empty flags and non-NULL context.\n");
    ret = pDirectDrawEnumerateExA(test_context_callbackExA, (LPVOID)0xdeadbeef, 0);
    ok(ret == DD_OK, "Expected DD_OK, got %d\n", ret);

    /* Test with valid callback parameter, NULL context parameter, and all flags set. */
    trace("Calling DirectDrawEnumerateExA with all flags set and NULL context.\n");
    ret = pDirectDrawEnumerateExA(test_nullcontext_callbackExA, NULL,
                                  DDENUM_ATTACHEDSECONDARYDEVICES |
                                  DDENUM_DETACHEDSECONDARYDEVICES |
                                  DDENUM_NONDISPLAYDEVICES);
    ok(ret == DD_OK, "Expected DD_OK, got %d\n", ret);
}

static BOOL WINAPI test_callbackExW(GUID *lpGUID, LPWSTR lpDriverDescription,
                                    LPWSTR lpDriverName, LPVOID lpContext,
                                    HMONITOR hm)
{
    ok(0, "The callback should not be invoked by DirectDrawEnumerateExW.\n");
    return TRUE;
}

static void test_DirectDrawEnumerateExW(void)
{
    HRESULT ret;

    if (!pDirectDrawEnumerateExW)
    {
        win_skip("DirectDrawEnumerateExW is not available\n");
        return;
    }

    /* DirectDrawEnumerateExW is not implemented on Windows. */

    /* Test with NULL callback parameter. */
    ret = pDirectDrawEnumerateExW(NULL, NULL, 0);
    ok(ret == DDERR_UNSUPPORTED, "Expected DDERR_UNSUPPORTED, got %d\n", ret);

    /* Test with invalid callback parameter. */
    ret = pDirectDrawEnumerateExW((LPDDENUMCALLBACKEXW)0xdeadbeef, NULL, 0);
    ok(ret == DDERR_UNSUPPORTED, "Expected DDERR_UNSUPPORTED, got %d\n", ret);

    /* Test with valid callback parameter and invalid flags */
    ret = pDirectDrawEnumerateExW(test_callbackExW, NULL, ~0);
    ok(ret == DDERR_UNSUPPORTED, "Expected DDERR_UNSUPPORTED, got %d\n", ret);

    /* Test with valid callback parameter and NULL context parameter. */
    ret = pDirectDrawEnumerateExW(test_callbackExW, NULL, 0);
    ok(ret == DDERR_UNSUPPORTED, "Expected DDERR_UNSUPPORTED, got %d\n", ret);

    /* Test with valid callback parameter, NULL context parameter, and all flags set. */
    ret = pDirectDrawEnumerateExW(test_callbackExW, NULL,
                                  DDENUM_ATTACHEDSECONDARYDEVICES |
                                  DDENUM_DETACHEDSECONDARYDEVICES |
                                  DDENUM_NONDISPLAYDEVICES);
    ok(ret == DDERR_UNSUPPORTED, "Expected DDERR_UNSUPPORTED, got %d\n", ret);
}

static void adddisplaymode(LPDDSURFACEDESC lpddsd)
{
    if (!modes)
        modes = HeapAlloc(GetProcessHeap(), 0, (modes_size = 2) * sizeof(DDSURFACEDESC));
    if (modes_cnt == modes_size)
        modes = HeapReAlloc(GetProcessHeap(), 0, modes, (modes_size *= 2) * sizeof(DDSURFACEDESC));
    assert(modes);
    modes[modes_cnt++] = *lpddsd;
}

static void flushdisplaymodes(void)
{
    HeapFree(GetProcessHeap(), 0, modes);
    modes = 0;
    modes_cnt = modes_size = 0;
}

static HRESULT WINAPI enummodescallback(LPDDSURFACEDESC lpddsd, LPVOID lpContext)
{
    trace("Width = %i, Height = %i, Refresh Rate = %i, Pitch = %i, flags =%02X\n",
        lpddsd->dwWidth, lpddsd->dwHeight,
          U2(*lpddsd).dwRefreshRate, U1(*lpddsd).lPitch, lpddsd->dwFlags);

    /* Check that the pitch is valid if applicable */
    if(lpddsd->dwFlags & DDSD_PITCH)
    {
        ok(U1(*lpddsd).lPitch != 0, "EnumDisplayModes callback with bad pitch\n");
    }

    /* Check that frequency is valid if applicable
     *
     * This fails on some Windows drivers or Windows versions, so it isn't important
     * apparently
    if(lpddsd->dwFlags & DDSD_REFRESHRATE)
    {
        ok(U2(*lpddsd).dwRefreshRate != 0, "EnumDisplayModes callback with bad refresh rate\n");
    }
     */

    adddisplaymode(lpddsd);

    return DDENUMRET_OK;
}

static void enumdisplaymodes(void)
{
    DDSURFACEDESC ddsd;
    HRESULT rc;

    ZeroMemory(&ddsd, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    rc = IDirectDraw_EnumDisplayModes(lpDD,
        DDEDM_STANDARDVGAMODES, &ddsd, 0, enummodescallback);
    ok(rc==DD_OK || rc==E_INVALIDARG,"EnumDisplayModes returned: %x\n",rc);
}

static void setdisplaymode(int i)
{
    HRESULT rc;
    RECT orig_rect;

    SetRect(&orig_rect, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));

    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_ALLOWMODEX | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(rc==DD_OK,"SetCooperativeLevel returned: %x\n",rc);
    if (modes[i].dwFlags & DDSD_PIXELFORMAT)
    {
        if (modes[i].ddpfPixelFormat.dwFlags & DDPF_RGB)
        {
            rc = IDirectDraw_SetDisplayMode(lpDD,
                modes[i].dwWidth, modes[i].dwHeight,
                U1(modes[i].ddpfPixelFormat).dwRGBBitCount);
            ok(DD_OK==rc || DDERR_UNSUPPORTED==rc,"SetDisplayMode returned: %x\n",rc);
            if (rc == DD_OK)
            {
                RECT r, scrn, test, virt;

                SetRect(&virt, 0, 0, GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN));
                OffsetRect(&virt, GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN));
                SetRect(&scrn, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
                trace("Mode (%dx%d) [%dx%d] (%d %d)x(%d %d)\n", modes[i].dwWidth, modes[i].dwHeight,
                      scrn.right, scrn.bottom, virt.left, virt.top, virt.right, virt.bottom);
                if (!EqualRect(&scrn, &orig_rect))
                {
                    HRESULT rect_result;

                    /* Check that the client rect was resized */
                    rc = GetClientRect(hwnd, &test);
                    ok(rc!=0, "GetClientRect returned %x\n", rc);
                    rc = EqualRect(&scrn, &test);
                    todo_wine ok(rc!=0, "Fullscreen window has wrong size\n");

                    /* Check that switching to normal cooperative level
                       does not restore the display mode */
                    rc = IDirectDraw_SetCooperativeLevel(lpDD, hwnd, DDSCL_NORMAL);
                    ok(rc==DD_OK, "SetCooperativeLevel returned %x\n", rc);
                    SetRect(&test, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
                    rect_result = EqualRect(&scrn, &test);
                    ok(rect_result!=0, "Setting cooperative level to DDSCL_NORMAL changed the display mode\n");

                    /* Go back to fullscreen */
                    rc = IDirectDraw_SetCooperativeLevel(lpDD,
                        hwnd, DDSCL_ALLOWMODEX | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
                    ok(rc==DD_OK, "SetCooperativeLevel returned: %x\n",rc);

                    /* If the display mode was changed, set the correct mode
                       to avoid irrelevant failures */
                    if (rect_result == 0)
                    {
                        rc = IDirectDraw_SetDisplayMode(lpDD,
                            modes[i].dwWidth, modes[i].dwHeight,
                            U1(modes[i].ddpfPixelFormat).dwRGBBitCount);
                        ok(DD_OK==rc, "SetDisplayMode returned: %x\n",rc);
                    }
                }
                ok(GetClipCursor(&r), "GetClipCursor() failed\n");
                /* ddraw sets clip rect here to the screen size, even for
                   multiple monitors */
                ok(EqualRect(&r, &scrn), "Invalid clip rect: (%d %d) x (%d %d)\n",
                   r.left, r.top, r.right, r.bottom);

                ok(ClipCursor(NULL), "ClipCursor() failed\n");
                ok(GetClipCursor(&r), "GetClipCursor() failed\n");
                ok(EqualRect(&r, &virt), "Invalid clip rect: (%d %d) x (%d %d)\n",
                   r.left, r.top, r.right, r.bottom);

                rc = IDirectDraw_RestoreDisplayMode(lpDD);
                ok(DD_OK==rc,"RestoreDisplayMode returned: %x\n",rc);
            }
        }
    }
}

static void createsurface(void)
{
    DDSURFACEDESC ddsd;
    DDSCAPS ddscaps;
    HRESULT rc;

    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE |
        DDSCAPS_FLIP |
        DDSCAPS_COMPLEX;
    ddsd.dwBackBufferCount = 1;
    rc = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpDDSPrimary, NULL );
    ok(rc==DD_OK,"CreateSurface returned: %x\n",rc);
    ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
    rc = IDirectDrawSurface_GetAttachedSurface(lpDDSPrimary, &ddscaps, &lpDDSBack);
    ok(rc==DD_OK,"GetAttachedSurface returned: %x\n",rc);
}

static void destroysurface(void)
{
    if( lpDDSPrimary != NULL )
    {
        IDirectDrawSurface_Release(lpDDSPrimary);
        lpDDSPrimary = NULL;
    }
}

static void testsurface(void)
{
    const char* testMsg = "ddraw device context test";
    HDC hdc;
    HRESULT rc;

    rc = IDirectDrawSurface_GetDC(lpDDSBack, &hdc);
    ok(rc==DD_OK, "IDirectDrawSurface_GetDC returned: %x\n",rc);
    SetBkColor(hdc, RGB(0, 0, 255));
    SetTextColor(hdc, RGB(255, 255, 0));
    TextOut(hdc, 0, 0, testMsg, lstrlen(testMsg));
    IDirectDrawSurface_ReleaseDC(lpDDSBack, hdc);
    ok(rc==DD_OK, "IDirectDrawSurface_ReleaseDC returned: %x\n",rc);

    while (1)
    {
        rc = IDirectDrawSurface_Flip(lpDDSPrimary, NULL, DDFLIP_WAIT);
        ok(rc==DD_OK || rc==DDERR_SURFACELOST, "IDirectDrawSurface_BltFast returned: %x\n",rc);

        if (rc == DD_OK)
        {
            break;
        }
        else if (rc == DDERR_SURFACELOST)
        {
            rc = IDirectDrawSurface_Restore(lpDDSPrimary);
            ok(rc==DD_OK, "IDirectDrawSurface_Restore returned: %x\n",rc);
        }
    }
}

static void testdisplaymodes(void)
{
    int i;

    for (i = 0; i < modes_cnt; ++i)
    {
        setdisplaymode(i);
        createsurface();
        testsurface();
        destroysurface();
    }
}

static void testcooperativelevels_normal(void)
{
    HRESULT rc;
    DDSURFACEDESC surfacedesc;
    IDirectDrawSurface *surface = (IDirectDrawSurface *) 0xdeadbeef;

    memset(&surfacedesc, 0, sizeof(surfacedesc));
    surfacedesc.dwSize = sizeof(surfacedesc);
    surfacedesc.ddpfPixelFormat.dwSize = sizeof(surfacedesc.ddpfPixelFormat);
    surfacedesc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    surfacedesc.dwBackBufferCount = 1;
    surfacedesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;

    /* Do some tests with DDSCL_NORMAL mode */

    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_NORMAL);
    ok(rc==DD_OK,"SetCooperativeLevel(DDSCL_NORMAL) returned: %x\n",rc);

    /* Try creating a double buffered primary in normal mode */
    rc = IDirectDraw_CreateSurface(lpDD, &surfacedesc, &surface, NULL);
    if (rc == DDERR_UNSUPPORTEDMODE)
        skip("Unsupported mode\n");
    else
    {
        ok(rc == DDERR_NOEXCLUSIVEMODE, "IDirectDraw_CreateSurface returned %08x\n", rc);
        ok(surface == NULL, "Returned surface pointer is %p\n", surface);
    }
    if(surface && surface != (IDirectDrawSurface *)0xdeadbeef) IDirectDrawSurface_Release(surface);

    /* Set the focus window */
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_SETFOCUSWINDOW);

    if (rc == DDERR_INVALIDPARAMS)
    {
        win_skip("NT4/Win95 do not support cooperative levels DDSCL_SETDEVICEWINDOW and DDSCL_SETFOCUSWINDOW\n");
        return;
    }

    ok(rc==DD_OK,"SetCooperativeLevel(DDSCL_SETFOCUSWINDOW) returned: %x\n",rc);

    /* Set the focus window a second time*/
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_SETFOCUSWINDOW);
    ok(rc==DD_OK,"SetCooperativeLevel(DDSCL_SETFOCUSWINDOW) the second time returned: %x\n",rc);

    /* Test DDSCL_SETFOCUSWINDOW with the other flags. They should all fail, except of DDSCL_NOWINDOWCHANGES */
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_NORMAL | DDSCL_SETFOCUSWINDOW);
    ok(rc==DDERR_INVALIDPARAMS,"SetCooperativeLevel(DDSCL_NORMAL | DDSCL_SETFOCUSWINDOW) returned: %x\n",rc);

    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_SETFOCUSWINDOW);
    ok(rc==DDERR_INVALIDPARAMS,"SetCooperativeLevel(DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_SETFOCUSWINDOW) returned: %x\n",rc);

    /* This one succeeds */
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_NOWINDOWCHANGES | DDSCL_SETFOCUSWINDOW);
    ok(rc==DD_OK,"SetCooperativeLevel(DDSCL_NOWINDOWCHANGES | DDSCL_SETFOCUSWINDOW) returned: %x\n",rc);

    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_MULTITHREADED | DDSCL_SETFOCUSWINDOW);
    ok(rc==DDERR_INVALIDPARAMS,"SetCooperativeLevel(DDSCL_MULTITHREADED | DDSCL_SETFOCUSWINDOW) returned: %x\n",rc);

    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_FPUSETUP | DDSCL_SETFOCUSWINDOW);
    ok(rc==DDERR_INVALIDPARAMS,"SetCooperativeLevel(DDSCL_FPUSETUP | DDSCL_SETFOCUSWINDOW) returned: %x\n",rc);

    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_FPUPRESERVE | DDSCL_SETFOCUSWINDOW);
    ok(rc==DDERR_INVALIDPARAMS,"SetCooperativeLevel(DDSCL_FPUPRESERVE | DDSCL_SETFOCUSWINDOW) returned: %x\n",rc);

    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_ALLOWREBOOT | DDSCL_SETFOCUSWINDOW);
    ok(rc==DDERR_INVALIDPARAMS,"SetCooperativeLevel(DDSCL_ALLOWREBOOT | DDSCL_SETFOCUSWINDOW) returned: %x\n",rc);

    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_ALLOWMODEX | DDSCL_SETFOCUSWINDOW);
    ok(rc==DDERR_INVALIDPARAMS,"SetCooperativeLevel(DDSCL_ALLOWMODEX | DDSCL_SETFOCUSWINDOW) returned: %x\n",rc);

    /* Set the device window without any other flags. Should give an error */
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_SETDEVICEWINDOW);
    ok(rc==DDERR_INVALIDPARAMS,"SetCooperativeLevel(DDSCL_SETDEVICEWINDOW) returned: %x\n",rc);

    /* Set device window with DDSCL_NORMAL */
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_NORMAL | DDSCL_SETDEVICEWINDOW);
    ok(rc==DD_OK,"SetCooperativeLevel(DDSCL_NORMAL | DDSCL_SETDEVICEWINDOW) returned: %x\n",rc);

    /* Also set the focus window. Should give an error */
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_ALLOWMODEX | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_SETDEVICEWINDOW | DDSCL_SETFOCUSWINDOW);
    ok(rc==DDERR_INVALIDPARAMS,"SetCooperativeLevel(DDSCL_NORMAL | DDSCL_SETDEVICEWINDOW | DDSCL_SETFOCUSWINDOW) returned: %x\n",rc);

    /* All done */
}

static void testcooperativelevels_exclusive(void)
{
    BOOL success;
    HRESULT rc;
    RECT window_rect;

    /* Do some tests with DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN mode */

    /* First, resize the window so it is not the same size as any screen */
    success = SetWindowPos(hwnd, 0, 0, 0, 281, 92, 0);
    ok(success, "SetWindowPos failed\n");

    /* Try to set exclusive mode only */
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_EXCLUSIVE);
    ok(rc==DDERR_INVALIDPARAMS,"SetCooperativeLevel(DDSCL_EXCLUSIVE) returned: %x\n",rc);

    /* Full screen mode only */
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_FULLSCREEN);
    ok(rc==DDERR_INVALIDPARAMS,"SetCooperativeLevel(DDSCL_FULLSCREEN) returned: %x\n",rc);

    /* Full screen mode + exclusive mode */
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE);
    ok(rc==DD_OK,"SetCooperativeLevel(DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN) returned: %x\n",rc);
    GetClientRect(hwnd, &window_rect);
    /* rect_before_create is assumed to hold the screen rect */
    rc = EqualRect(&rect_before_create, &window_rect);
    todo_wine ok(rc!=0, "Fullscreen window has wrong size\n");

    /* Set the focus window. Should fail */
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_SETFOCUSWINDOW);
    ok(rc==DDERR_HWNDALREADYSET ||
       broken(rc==DDERR_INVALIDPARAMS) /* NT4/Win95 */,"SetCooperativeLevel(DDSCL_SETFOCUSWINDOW) returned: %x\n",rc);


    /* All done */
}

static void testddraw3(void)
{
    const GUID My_IID_IDirectDraw3 = {
        0x618f8ad4,
        0x8b7a,
        0x11d0,
        { 0x8f,0xcc,0x0,0xc0,0x4f,0xd9,0x18,0x9d }
    };
    IDirectDraw3 *dd3;
    HRESULT hr;
    hr = IDirectDraw_QueryInterface(lpDD, &My_IID_IDirectDraw3, (void **) &dd3);
    ok(hr == E_NOINTERFACE, "QueryInterface for IID_IDirectDraw3 returned 0x%08x, expected E_NOINTERFACE\n", hr);
    if(SUCCEEDED(hr) && dd3) IDirectDraw3_Release(dd3);
}

static void testddraw7(void)
{
    IDirectDraw7 *dd7;
    HRESULT hr;
    DDDEVICEIDENTIFIER2 *pdddi2;
    DWORD dddi2Bytes;
    DWORD *pend;

    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw7, (void **) &dd7);
    if (hr==E_NOINTERFACE)
    {
        win_skip("DirectDraw7 is not supported\n");
        return;
    }
    ok(hr==DD_OK, "IDirectDraw7_QueryInterface returned %08x\n", hr);

    if (hr==DD_OK)
    {
         dddi2Bytes = FIELD_OFFSET(DDDEVICEIDENTIFIER2, dwWHQLLevel) + sizeof(DWORD);

         pdddi2 = HeapAlloc( GetProcessHeap(), 0, dddi2Bytes + 2*sizeof(DWORD) );
         pend = (DWORD *)((char *)pdddi2 + dddi2Bytes);
         pend[0] = 0xdeadbeef;
         pend[1] = 0xdeadbeef;

         hr = IDirectDraw7_GetDeviceIdentifier(dd7, pdddi2, 0);
         ok(hr==DD_OK, "get device identifier failed with %08x\n", hr);

         if (hr==DD_OK)
         {
             /* check how strings are copied into the structure */
             ok(pdddi2->szDriver[MAX_DDDEVICEID_STRING - 1]==0, "szDriver not cleared\n");
             ok(pdddi2->szDescription[MAX_DDDEVICEID_STRING - 1]==0, "szDescription not cleared\n");
             /* verify that 8 byte structure size alignment will not overwrite memory */
             ok(pend[0]==0xdeadbeef || broken(pend[0] != 0xdeadbeef), /* win2k */
                "memory beyond DDDEVICEIDENTIFIER2 overwritten\n");
             ok(pend[1]==0xdeadbeef, "memory beyond DDDEVICEIDENTIFIER2 overwritten\n");
         }

         IDirectDraw_Release(dd7);
         HeapFree( GetProcessHeap(), 0, pdddi2 );
    }
}

START_TEST(ddrawmodes)
{
    init_function_pointers();

    createwindow();
    if (!createdirectdraw())
        return;

    test_DirectDrawEnumerateA();
    test_DirectDrawEnumerateW();
    test_DirectDrawEnumerateExA();
    test_DirectDrawEnumerateExW();

    enumdisplaymodes();
    if (winetest_interactive)
        testdisplaymodes();
    flushdisplaymodes();
    testddraw3();
    testddraw7();
    releasedirectdraw();

    createdirectdraw();
    testcooperativelevels_normal();
    releasedirectdraw();

    createdirectdraw();
    testcooperativelevels_exclusive();
    releasedirectdraw();
}
