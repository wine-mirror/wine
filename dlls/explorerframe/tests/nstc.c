/*
 *    Unit tests for the NamespaceTree Control
 *
 * Copyright 2010 David Hedberg
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

#include <stdio.h>

#define COBJMACROS

#include "shlobj.h"
#include "wine/test.h"

static HWND hwnd;

/* "Intended for internal use" */
#define TVS_EX_NOSINGLECOLLAPSE 0x1

/* Returns FALSE if the NamespaceTreeControl failed to be instantiated. */
static BOOL test_initialization(void)
{
    INameSpaceTreeControl *pnstc;
    IOleWindow *pow;
    IUnknown *punk;
    HWND hwnd_host1;
    LONG lres;
    HRESULT hr;
    RECT rc;

    hr = CoCreateInstance(&CLSID_NamespaceTreeControl, NULL, CLSCTX_INPROC_SERVER,
                          &IID_INameSpaceTreeControl, (void**)&pnstc);
    ok(hr == S_OK || hr == REGDB_E_CLASSNOTREG, "Got 0x%08x\n", hr);
    if(FAILED(hr))
    {
        return FALSE;
    }

    hr = INameSpaceTreeControl_Initialize(pnstc, NULL, NULL, 0);
    ok(hr == HRESULT_FROM_WIN32(ERROR_TLW_WITH_WSCHILD), "Got (0x%08x)\n", hr);

    hr = INameSpaceTreeControl_Initialize(pnstc, (HWND)0xDEADBEEF, NULL, 0);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE), "Got (0x%08x)\n", hr);

    ZeroMemory(&rc, sizeof(RECT));
    hr = INameSpaceTreeControl_Initialize(pnstc, NULL, &rc, 0);
    ok(hr == HRESULT_FROM_WIN32(ERROR_TLW_WITH_WSCHILD), "Got (0x%08x)\n", hr);

    hr = INameSpaceTreeControl_Initialize(pnstc, (HWND)0xDEADBEEF, &rc, 0);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE), "Got (0x%08x)\n", hr);

    hr = INameSpaceTreeControl_QueryInterface(pnstc, &IID_IOleWindow, (void**)&pow);
    ok(hr == S_OK, "Got (0x%08x)\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IOleWindow_GetWindow(pow, &hwnd_host1);
        ok(hr == S_OK, "Got (0x%08x)\n", hr);
        ok(hwnd_host1 == NULL, "hwnd is not null.\n");

        hr = IOleWindow_ContextSensitiveHelp(pow, TRUE);
        ok(hr == E_NOTIMPL, "Got (0x%08x)\n", hr);
        hr = IOleWindow_ContextSensitiveHelp(pow, FALSE);
        ok(hr == E_NOTIMPL, "Got (0x%08x)\n", hr);
        IOleWindow_Release(pow);
    }

    hr = INameSpaceTreeControl_Initialize(pnstc, hwnd, NULL, 0);
    ok(hr == S_OK, "Got (0x%08x)\n", hr);
    hr = INameSpaceTreeControl_QueryInterface(pnstc, &IID_IOleWindow, (void**)&pow);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    if(SUCCEEDED(hr))
    {
        static const CHAR namespacetree[] = "NamespaceTreeControl";
        char buf[1024];
        LONG style, expected_style;
        HWND hwnd_tv;
        hr = IOleWindow_GetWindow(pow, &hwnd_host1);
        ok(hr == S_OK, "Got (0x%08x)\n", hr);
        ok(hwnd_host1 != NULL, "hwnd_host1 is null.\n");
        buf[0] = '\0';
        GetClassNameA(hwnd_host1, buf, 1024);
        ok(!lstrcmpA(namespacetree, buf), "Class name was %s\n", buf);

        expected_style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
        style = GetWindowLongPtrW(hwnd_host1, GWL_STYLE);
        ok(style == expected_style, "Got style %08x\n", style);

        expected_style = 0;
        style = GetWindowLongPtrW(hwnd_host1, GWL_EXSTYLE);
        ok(style == expected_style, "Got style %08x\n", style);

        expected_style = 0;
        style = SendMessageW(hwnd_host1, TVM_GETEXTENDEDSTYLE, 0, 0);
        ok(style == expected_style, "Got 0x%08x\n", style);

        hwnd_tv = FindWindowExW(hwnd_host1, NULL, WC_TREEVIEWW, NULL);
        ok(hwnd_tv != NULL, "Failed to get treeview hwnd.\n");
        if(hwnd_tv)
        {
            expected_style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
                WS_CLIPCHILDREN | WS_TABSTOP | TVS_NOHSCROLL |
                TVS_NONEVENHEIGHT | TVS_INFOTIP | TVS_TRACKSELECT | TVS_EDITLABELS;
            style = GetWindowLongPtrW(hwnd_tv, GWL_STYLE);
            ok(style == expected_style, "Got style %08x\n", style);

            expected_style = 0;
            style = GetWindowLongPtrW(hwnd_tv, GWL_EXSTYLE);
            ok(style == expected_style, "Got style %08x\n", style);

            expected_style = TVS_EX_NOSINGLECOLLAPSE | TVS_EX_DOUBLEBUFFER |
                TVS_EX_RICHTOOLTIP | TVS_EX_DRAWIMAGEASYNC;
            style = SendMessageW(hwnd_tv, TVM_GETEXTENDEDSTYLE, 0, 0);
            todo_wine ok(style == expected_style, "Got 0x%08x\n", style);
        }

        IOleWindow_Release(pow);
    }

    if(0)
    {
        /* The control can be initialized again without crashing, but
         * the reference counting will break. */
        hr = INameSpaceTreeControl_Initialize(pnstc, hwnd, &rc, 0);
        ok(hr == S_OK, "Got (0x%08x)\n", hr);
        hr = INameSpaceTreeControl_QueryInterface(pnstc, &IID_IOleWindow, (void**)&pow);
        if(SUCCEEDED(hr))
        {
            HWND hwnd_host2;
            hr = IOleWindow_GetWindow(pow, &hwnd_host2);
            ok(hr == S_OK, "Got (0x%08x)\n", hr);
            ok(hwnd_host1 != hwnd_host2, "Same hwnd.\n");
            IOleWindow_Release(pow);
        }
    }

    /* Some "random" interfaces */
    hr = INameSpaceTreeControl_QueryInterface(pnstc, &IID_IOleInPlaceObject, (void**)&punk);
    ok(hr == E_NOINTERFACE || hr == S_OK /* vista, w2k8 */, "Got (0x%08x)\n", hr);
    if(SUCCEEDED(hr)) IUnknown_Release(punk);
    hr = INameSpaceTreeControl_QueryInterface(pnstc, &IID_IOleInPlaceActiveObject, (void**)&punk);
    ok(hr == E_NOINTERFACE || hr == S_OK /* vista, w2k8 */, "Got (0x%08x)\n", hr);
    if(SUCCEEDED(hr)) IUnknown_Release(punk);
    hr = INameSpaceTreeControl_QueryInterface(pnstc, &IID_IOleInPlaceObjectWindowless, (void**)&punk);
    ok(hr == E_NOINTERFACE || hr == S_OK /* vista, w2k8 */, "Got (0x%08x)\n", hr);
    if(SUCCEEDED(hr)) IUnknown_Release(punk);

    hr = INameSpaceTreeControl_QueryInterface(pnstc, &IID_IOleInPlaceUIWindow, (void**)&punk);
    ok(hr == E_NOINTERFACE, "Got (0x%08x)\n", hr);
    hr = INameSpaceTreeControl_QueryInterface(pnstc, &IID_IOleInPlaceFrame, (void**)&punk);
    ok(hr == E_NOINTERFACE, "Got (0x%08x)\n", hr);
    hr = INameSpaceTreeControl_QueryInterface(pnstc, &IID_IOleInPlaceSite, (void**)&punk);
    ok(hr == E_NOINTERFACE, "Got (0x%08x)\n", hr);
    hr = INameSpaceTreeControl_QueryInterface(pnstc, &IID_IOleInPlaceSiteEx, (void**)&punk);
    ok(hr == E_NOINTERFACE, "Got (0x%08x)\n", hr);
    hr = INameSpaceTreeControl_QueryInterface(pnstc, &IID_IOleInPlaceSiteWindowless, (void**)&punk);
    ok(hr == E_NOINTERFACE, "Got (0x%08x)\n", hr);

    /* On windows, the reference count won't go to zero until the
     * window is destroyed. */
    INameSpaceTreeControl_AddRef(pnstc);
    lres = INameSpaceTreeControl_Release(pnstc);
    ok(lres > 1, "Reference count was (%d).\n", lres);

    DestroyWindow(hwnd_host1);
    lres = INameSpaceTreeControl_Release(pnstc);
    ok(!lres, "lres was %d\n", lres);

    return TRUE;
}

static void setup_window(void)
{
    WNDCLASSA wc;
    static const char nstctest_wnd_name[] = "nstctest_wnd";

    ZeroMemory(&wc, sizeof(WNDCLASSA));
    wc.lpfnWndProc      = DefWindowProcA;
    wc.lpszClassName    = nstctest_wnd_name;
    RegisterClassA(&wc);
    hwnd = CreateWindowA(nstctest_wnd_name, NULL, WS_TABSTOP,
                         0, 0, 200, 200, NULL, 0, 0, NULL);
    ok(hwnd != NULL, "Failed to create window for test (lasterror: %d).\n",
       GetLastError());
}

static void destroy_window(void)
{
    DestroyWindow(hwnd);
}

START_TEST(nstc)
{
    OleInitialize(NULL);
    setup_window();

    test_initialization();

    destroy_window();
    OleUninitialize();
}
