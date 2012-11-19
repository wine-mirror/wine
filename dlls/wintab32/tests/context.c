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

static HCTX (WINAPI *pWTOpenA)(HWND, LPLOGCONTEXTA, BOOL);
static BOOL (WINAPI *pWTClose)(HCTX);

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

START_TEST(context)
{
    HMODULE hWintab = load_functions();

    if (!hWintab)
    {
        skip("Wintab32.dll not available\n");
        return;
    }

    test_WTOpenContextValidation();

    FreeLibrary(hWintab);
}
