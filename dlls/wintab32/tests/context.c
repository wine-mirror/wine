/*
 * tests for Wintab context behavior
 *
 * Copyright 2009 John Klehm
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
#include <windows.h>
#include <wintab.h>

#include "wine/test.h"

static const CHAR wintab32Dll[] = "Wintab32.dll";
static const CHAR wintabTestWindowClassName[] = "WintabTestWnd";
static const CHAR contextName[] = "TestContext";
static const UINT X = 0;
static const UINT Y = 0;
static const UINT WIDTH = 200;
static const UINT HEIGHT = 200;

static LOGCONTEXTA glogContext;

static HCTX (WINAPI *pWTOpenA)(HWND, LPLOGCONTEXTA, BOOL);
static BOOL (WINAPI *pWTClose)(HCTX);
static UINT (WINAPI *pWTInfoA)(UINT, UINT, void*);

static HMODULE load_functions(void)
{
#define GET_PROC(func) \
    (p ## func = (void *)GetProcAddress(hWintab, #func))

    HMODULE hWintab = LoadLibraryA(wintab32Dll);

    if (!hWintab)
    {
        trace("LoadLibraryA(%s) failed\n",
            wintab32Dll);
        return NULL;
    }

    if (GET_PROC(WTOpenA) &&
        GET_PROC(WTInfoA) &&
        GET_PROC(WTClose) )
    {
        return hWintab;
    }
    else
    {
        FreeLibrary(hWintab);
        trace("Library loaded but failed to load function pointers\n");
        return NULL;
    }

#undef GET_PROC
}

static LRESULT CALLBACK wintabTestWndProc(HWND hwnd, UINT msg, WPARAM wParam,
                LPARAM lParam)
{
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static void wintab_create_window(HWND* pHwnd)
{
    WNDCLASSA testWindowClass;

    if (!pHwnd) return;

    *pHwnd = NULL;

    ZeroMemory(&testWindowClass, sizeof(testWindowClass));

    testWindowClass.lpfnWndProc   = wintabTestWndProc;
    testWindowClass.hInstance     = NULL;
    testWindowClass.hIcon         = NULL;
    testWindowClass.hCursor       = NULL;
    testWindowClass.hbrBackground = NULL;
    testWindowClass.lpszMenuName  = NULL;
    testWindowClass.lpszClassName = wintabTestWindowClassName;

    assert(RegisterClassA(&testWindowClass));

    *pHwnd = CreateWindowA(wintabTestWindowClassName, NULL,
                            WS_OVERLAPPED, X, Y, WIDTH, HEIGHT, NULL, NULL,
                            NULL, NULL);

    assert(*pHwnd != NULL);
}

static void wintab_destroy_window(HWND hwnd)
{
    DestroyWindow(hwnd);
    UnregisterClassA(wintabTestWindowClassName, NULL);
}

/* test how a logcontext is validated by wtopen */
static void test_WTOpenContextValidation(void)
{
    HWND defaultWindow = NULL;
    HCTX hCtx = NULL;
    LOGCONTEXTA testLogCtx;
    LOGCONTEXTA refLogCtx;
    int memdiff;

    wintab_create_window(&defaultWindow);

    ZeroMemory(&testLogCtx, sizeof(testLogCtx));
    strcpy(testLogCtx.lcName, contextName);

    ZeroMemory(&refLogCtx, sizeof(refLogCtx));
    strcpy(refLogCtx.lcName, contextName);

    /* a zeroed out context has values which makes WTOpen return null
     * on wacom tablets but not on uclogic tablets */
    hCtx = pWTOpenA(defaultWindow, &testLogCtx, TRUE);

    /* check if the context gets updated */
    memdiff = memcmp(&testLogCtx, &refLogCtx, sizeof(LOGCONTEXTA));
    ok(0 == memdiff, "Expected 0 == memcmp(testLogCtx, refLogCtx), got %i\n",
        memdiff);

    if (hCtx)
        pWTClose(hCtx);

    wintab_destroy_window(defaultWindow);
}

static void test_WTInfoA(void)
{
    char name[LCNAMELEN];
    UINT ret;
    AXIS value;
    AXIS orientation[3];

    glogContext.lcOptions |= CXO_SYSTEM;

    ret = pWTInfoA(WTI_DEFSYSCTX, 0, &glogContext);
    if(!ret)
    {
        skip("No tablet connected\n");
        return;
    }
    ok(ret == sizeof( LOGCONTEXTA ), "incorrect size\n" );
    ok(glogContext.lcOptions & CXO_SYSTEM, "Wrong options 0x%08x\n", glogContext.lcOptions);

    ret = pWTInfoA( WTI_DEVICES, DVC_NAME, name );
    trace("DVC_NAME %s\n", name);

    ret = pWTInfoA( WTI_DEVICES, DVC_HARDWARE, name );
    trace("DVC_HARDWARE %s\n", name);

    ret = pWTInfoA( WTI_DEVICES, DVC_X, &value );
    ok(ret == sizeof( AXIS ), "Wrong DVC_X size %d\n", ret);
    trace("DVC_X %ld, %ld, %d\n", value.axMin, value.axMax, value.axUnits);

    ret = pWTInfoA( WTI_DEVICES, DVC_Y, &value );
    ok(ret == sizeof( AXIS ), "Wrong DVC_Y size %d\n", ret);
    trace("DVC_Y %ld, %ld, %d\n", value.axMin, value.axMax, value.axUnits);

    ret = pWTInfoA( WTI_DEVICES, DVC_Z, &value );
    if(ret)
        trace("DVC_Z %ld, %ld, %d\n", value.axMin, value.axMax, value.axUnits);
    else
        trace("DVC_Z not supported\n");

    ret = pWTInfoA( WTI_DEVICES, DVC_NPRESSURE, &value );
    ok(ret == sizeof( AXIS ), "Wrong DVC_NPRESSURE, size %d\n", ret);
    trace("DVC_NPRESSURE %ld, %ld, %d\n", value.axMin, value.axMax, value.axUnits);

    ret = pWTInfoA( WTI_DEVICES, DVC_TPRESSURE, &value );
    if(ret)
        trace("DVC_TPRESSURE %ld, %ld, %d\n", value.axMin, value.axMax, value.axUnits);
    else
        trace("DVC_TPRESSURE not supported\n");

    ret = pWTInfoA( WTI_DEVICES, DVC_ORIENTATION, &orientation );
    ok(ret == sizeof( AXIS )*3, "Wrong DVC_ORIENTATION, size %d\n", ret);
    trace("DVC_ORIENTATION0 %ld, %ld, %d\n", orientation[0].axMin, orientation[0].axMax, orientation[0].axUnits);
    trace("DVC_ORIENTATION1 %ld, %ld, %d\n", orientation[1].axMin, orientation[1].axMax, orientation[1].axUnits);
    trace("DVC_ORIENTATION2 %ld, %ld, %d\n", orientation[2].axMin, orientation[2].axMax, orientation[2].axUnits);

    ret = pWTInfoA( WTI_DEVICES, DVC_ROTATION, &orientation );
    if(ret)
    {
        trace("DVC_ROTATION0 %ld, %ld, %d\n", orientation[0].axMin, orientation[0].axMax, orientation[0].axUnits);
        trace("DVC_ROTATION1 %ld, %ld, %d\n", orientation[1].axMin, orientation[1].axMax, orientation[1].axUnits);
        trace("DVC_ROTATION2 %ld, %ld, %d\n", orientation[2].axMin, orientation[2].axMax, orientation[2].axUnits);
    }
    else
        trace("DVC_ROTATION not supported\n");

}

START_TEST(context)
{
    HMODULE hWintab = load_functions();

    if (!hWintab)
    {
        win_skip("Wintab32.dll not available\n");
        return;
    }

    test_WTOpenContextValidation();
    test_WTInfoA();

    FreeLibrary(hWintab);
}
