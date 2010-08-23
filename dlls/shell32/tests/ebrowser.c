/*
 *    Unit tests for the Explorer Browser control
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

/*********************************************************************
 * Some simple helpers
 */
static HRESULT ebrowser_instantiate(IExplorerBrowser **peb)
{
    return CoCreateInstance(&CLSID_ExplorerBrowser, NULL, CLSCTX_INPROC_SERVER,
                            &IID_IExplorerBrowser, (void**)peb);
}

static HRESULT ebrowser_initialize(IExplorerBrowser *peb)
{
    RECT rc;
    rc.top = rc.left = 0; rc.bottom = rc.right = 500;
    return IExplorerBrowser_Initialize(peb, hwnd, &rc, NULL);
}

static void test_QueryInterface(void)
{
    IExplorerBrowser *peb;
    IUnknown *punk;
    HRESULT hr;
    LONG lres;

    hr = ebrowser_instantiate(&peb);
    ok(hr == S_OK, "Got 0x%08x\n", hr);

#define test_qinterface(iid, exp)                                       \
    do {                                                                \
        hr = IExplorerBrowser_QueryInterface(peb, &iid, (void**)&punk); \
        ok(hr == exp, "(%s:)Expected (0x%08x), got (0x%08x)\n",         \
           #iid, exp, hr);                                              \
        if(SUCCEEDED(hr)) IUnknown_Release(punk);                       \
    } while(0)

    test_qinterface(IID_IUnknown, S_OK);
    test_qinterface(IID_IExplorerBrowser, S_OK);
    test_qinterface(IID_IShellBrowser, S_OK);
    todo_wine test_qinterface(IID_IOleWindow, S_OK);
    todo_wine test_qinterface(IID_ICommDlgBrowser, S_OK);
    todo_wine test_qinterface(IID_ICommDlgBrowser2, S_OK);
    todo_wine test_qinterface(IID_ICommDlgBrowser3, S_OK);
    todo_wine test_qinterface(IID_IServiceProvider, S_OK);
    todo_wine test_qinterface(IID_IObjectWithSite, S_OK);
    todo_wine test_qinterface(IID_IConnectionPointContainer, S_OK);
    test_qinterface(IID_IOleObject, E_NOINTERFACE);
    test_qinterface(IID_IViewObject, E_NOINTERFACE);
    test_qinterface(IID_IViewObject2, E_NOINTERFACE);
    test_qinterface(IID_IViewObjectEx, E_NOINTERFACE);
    test_qinterface(IID_IConnectionPoint, E_NOINTERFACE);
    test_qinterface(IID_IShellView, E_NOINTERFACE);
    test_qinterface(IID_INameSpaceTreeControlEvents, E_NOINTERFACE);

#undef test_qinterface

    lres = IExplorerBrowser_Release(peb);
    ok(lres == 0, "Got %d\n", lres);
}

static void test_SB_misc(void)
{
    IExplorerBrowser *peb;
    IShellBrowser *psb;
    HRESULT hr;
    HWND retHwnd;
    LONG lres;

    ebrowser_instantiate(&peb);
    hr = IExplorerBrowser_QueryInterface(peb, &IID_IShellBrowser, (void**)&psb);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    if(FAILED(hr))
    {
        skip("Failed to get IShellBrowser interface.\n");
        return;
    }

    /* Some unimplemented methods */
    retHwnd = (HWND)0xDEADBEEF;
    hr = IShellBrowser_GetControlWindow(psb, FCW_TOOLBAR, &retHwnd);
    ok(hr == E_NOTIMPL, "got (0x%08x)\n", hr);
    ok(retHwnd == (HWND)0xDEADBEEF, "HWND overwritten\n");

    hr = IShellBrowser_GetControlWindow(psb, FCW_STATUS, &retHwnd);
    ok(hr == E_NOTIMPL, "got (0x%08x)\n", hr);
    ok(retHwnd == (HWND)0xDEADBEEF, "HWND overwritten\n");

    hr = IShellBrowser_GetControlWindow(psb, FCW_TREE, &retHwnd);
    ok(hr == E_NOTIMPL, "got (0x%08x)\n", hr);
    ok(retHwnd == (HWND)0xDEADBEEF, "HWND overwritten\n");

    hr = IShellBrowser_GetControlWindow(psb, FCW_PROGRESS, &retHwnd);
    ok(hr == E_NOTIMPL, "got (0x%08x)\n", hr);
    ok(retHwnd == (HWND)0xDEADBEEF, "HWND overwritten\n");

    /* ::InsertMenuSB */
    hr = IShellBrowser_InsertMenusSB(psb, NULL, NULL);
    ok(hr == E_NOTIMPL, "got (0x%08x)\n", hr);

    /* ::RemoveMenusSB */
    hr = IShellBrowser_RemoveMenusSB(psb, NULL);
    ok(hr == E_NOTIMPL, "got (0x%08x)\n", hr);

    /* ::SetMenuSB */
    hr = IShellBrowser_SetMenuSB(psb, NULL, NULL, NULL);
    ok(hr == E_NOTIMPL, "got (0x%08x)\n", hr);

    IShellBrowser_Release(psb);
    lres = IExplorerBrowser_Release(peb);
    ok(lres == 0, "Got %d\n", lres);
}

static void test_initialization(void)
{
    IExplorerBrowser *peb;
    IShellBrowser *psb;
    HRESULT hr;
    ULONG lres;
    RECT rc;

    ebrowser_instantiate(&peb);

    if(0)
    {
        /* Crashes on Windows 7 */
        hr = IExplorerBrowser_Initialize(peb, NULL, NULL, NULL);
        hr = IExplorerBrowser_Initialize(peb, hwnd, NULL, NULL);
    }

    ZeroMemory(&rc, sizeof(RECT));

    hr = IExplorerBrowser_Initialize(peb, NULL, &rc, NULL);
    ok(hr == E_INVALIDARG, "got (0x%08x)\n", hr);

    hr = IExplorerBrowser_Initialize(peb, hwnd, &rc, NULL);
    ok(hr == S_OK, "got (0x%08x)\n", hr);

    /* Initialize twice */
    hr = IExplorerBrowser_Initialize(peb, hwnd, &rc, NULL);
    ok(hr == E_UNEXPECTED, "got (0x%08x)\n", hr);

    hr = IExplorerBrowser_Destroy(peb);
    ok(hr == S_OK, "got (0x%08x)\n", hr);

    /* Initialize again */
    hr = IExplorerBrowser_Initialize(peb, hwnd, &rc, NULL);
    ok(hr == E_UNEXPECTED, "got (0x%08x)\n", hr);

    /* Destroy again */
    hr = IExplorerBrowser_Destroy(peb);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    lres = IExplorerBrowser_Release(peb);
    ok(lres == 0, "Got %d\n", lres);

    /* Initialize with a few different rectangles */
    peb = NULL;
    ebrowser_instantiate(&peb);
    rc.left = 50; rc.top = 20; rc.right = 100; rc.bottom = 80;
    hr = IExplorerBrowser_Initialize(peb, hwnd, &rc, NULL);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    hr = IExplorerBrowser_QueryInterface(peb, &IID_IShellBrowser, (void**)&psb);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    if(SUCCEEDED(hr))
    {
        HWND eb_hwnd;
        RECT eb_rc;
        char buf[1024];
        LONG style, expected_style;
        static const RECT exp_rc = {0, 0, 48, 58};

        hr = IShellBrowser_GetWindow(psb, &eb_hwnd);
        ok(hr == S_OK, "Got 0x%08x\n", hr);

        GetClientRect(eb_hwnd, &eb_rc);
        ok(EqualRect(&eb_rc, &exp_rc), "Got client rect (%d, %d)-(%d, %d)\n",
           eb_rc.left, eb_rc.top, eb_rc.right, eb_rc.bottom);

        GetWindowRect(eb_hwnd, &eb_rc);
        ok(eb_rc.right - eb_rc.left == 50, "Got window width %d\n", eb_rc.right - eb_rc.left);
        ok(eb_rc.bottom - eb_rc.top == 60, "Got window height %d\n", eb_rc.bottom - eb_rc.top);

        buf[0] = '\0';
        GetClassNameA(eb_hwnd, buf, 1024);
        ok(!lstrcmpA(buf, "ExplorerBrowserControl"), "Unexpected classname %s\n", buf);

        expected_style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_BORDER;
        style = GetWindowLongPtrW(eb_hwnd, GWL_STYLE);
        todo_wine ok(style == expected_style, "Got style 0x%08x, expected 0x%08x\n", style, expected_style);

        expected_style = WS_EX_CONTROLPARENT;
        style = GetWindowLongPtrW(eb_hwnd, GWL_EXSTYLE);
        ok(style == expected_style, "Got exstyle 0x%08x, expected 0x%08x\n", style, expected_style);

        ok(GetParent(eb_hwnd) == hwnd, "GetParent returns %p\n", GetParent(eb_hwnd));

        /* ::Destroy() destroys the window. */
        ok(IsWindow(eb_hwnd), "eb_hwnd invalid.\n");
        IExplorerBrowser_Destroy(peb);
        ok(!IsWindow(eb_hwnd), "eb_hwnd valid.\n");

        IShellBrowser_Release(psb);
        lres = IExplorerBrowser_Release(peb);
        ok(lres == 0, "Got refcount %d\n", lres);
    }
    else
    {
        skip("Skipping some tests.\n");

        IExplorerBrowser_Destroy(peb);
        lres = IExplorerBrowser_Release(peb);
        ok(lres == 0, "Got refcount %d\n", lres);
    }

    ebrowser_instantiate(&peb);
    rc.left = 0; rc.top = 0; rc.right = 0; rc.bottom = 0;
    hr = IExplorerBrowser_Initialize(peb, hwnd, &rc, NULL);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    IExplorerBrowser_Destroy(peb);
    lres = IExplorerBrowser_Release(peb);
    ok(lres == 0, "Got refcount %d\n", lres);

    ebrowser_instantiate(&peb);
    rc.left = -1; rc.top = -1; rc.right = 1; rc.bottom = 1;
    hr = IExplorerBrowser_Initialize(peb, hwnd, &rc, NULL);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    IExplorerBrowser_Destroy(peb);
    lres = IExplorerBrowser_Release(peb);
    ok(lres == 0, "Got refcount %d\n", lres);

    ebrowser_instantiate(&peb);
    rc.left = 10; rc.top = 10; rc.right = 5; rc.bottom = 5;
    hr = IExplorerBrowser_Initialize(peb, hwnd, &rc, NULL);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    IExplorerBrowser_Destroy(peb);
    lres = IExplorerBrowser_Release(peb);
    ok(lres == 0, "Got refcount %d\n", lres);

    ebrowser_instantiate(&peb);
    rc.left = 10; rc.top = 10; rc.right = 5; rc.bottom = 5;
    hr = IExplorerBrowser_Initialize(peb, hwnd, &rc, NULL);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    IExplorerBrowser_Destroy(peb);
    lres = IExplorerBrowser_Release(peb);
    ok(lres == 0, "Got refcount %d\n", lres);
}

static void test_basics(void)
{
    IExplorerBrowser *peb;
    IShellBrowser *psb;
    ULONG lres;
    DWORD flags;
    HDWP hdwp;
    RECT rc;
    HRESULT hr;

    ebrowser_instantiate(&peb);
    ebrowser_initialize(peb);

    /* SetRect */
    rc.left = 0; rc.top = 0; rc.right = 0; rc.bottom = 0;
    hr = IExplorerBrowser_SetRect(peb, NULL, rc);
    ok(hr == S_OK, "got (0x%08x)\n", hr);

    rc.left = 100; rc.top = 100; rc.right = 10; rc.bottom = 10;
    hr = IExplorerBrowser_SetRect(peb, NULL, rc);
    ok(hr == S_OK, "got (0x%08x)\n", hr);

    /* SetRect with DeferWindowPos */
    rc.left = rc.top = 0; rc.right = rc.bottom = 10;
    hdwp = BeginDeferWindowPos(1);
    hr = IExplorerBrowser_SetRect(peb, &hdwp, rc);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    lres = EndDeferWindowPos(hdwp);
    ok(lres, "EndDeferWindowPos failed.\n");

    hdwp = NULL;
    hr = IExplorerBrowser_SetRect(peb, &hdwp, rc);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    ok(hdwp == NULL, "got %p\n", hdwp);
    lres = EndDeferWindowPos(hdwp);
    ok(!lres, "EndDeferWindowPos succeeded unexpectedly.\n");

    /* Test positioning */
    rc.left = 10; rc.top = 20; rc.right = 50; rc.bottom = 50;
    hr = IExplorerBrowser_SetRect(peb, NULL, rc);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    hr = IExplorerBrowser_QueryInterface(peb, &IID_IShellBrowser, (void**)&psb);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    if(SUCCEEDED(hr))
    {
        HWND eb_hwnd;
        RECT eb_rc;
        static const RECT exp_rc = {11, 21, 49, 49};

        hr = IShellBrowser_GetWindow(psb, &eb_hwnd);
        ok(hr == S_OK, "Got 0x%08x\n", hr);

        GetClientRect(eb_hwnd, &eb_rc);
        MapWindowPoints(eb_hwnd, hwnd, (POINT*)&eb_rc, 2);
        ok(EqualRect(&eb_rc, &exp_rc), "Got rect (%d, %d) - (%d, %d)\n",
           eb_rc.left, eb_rc.top, eb_rc.right, eb_rc.bottom);

        IShellBrowser_Release(psb);
    }

    IExplorerBrowser_Destroy(peb);
    IExplorerBrowser_Release(peb);

    /* GetOptions/SetOptions*/
    ebrowser_instantiate(&peb);

    if(0) {
        /* Crashes on Windows 7 */
        IExplorerBrowser_GetOptions(peb, NULL);
    }

    hr = IExplorerBrowser_GetOptions(peb, &flags);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    ok(flags == 0, "got (0x%08x)\n", flags);

    /* Settings preserved through Initialize. */
    hr = IExplorerBrowser_SetOptions(peb, 0xDEADBEEF);
    ok(hr == S_OK, "got (0x%08x)\n", hr);

    ebrowser_initialize(peb);

    hr = IExplorerBrowser_GetOptions(peb, &flags);
    ok(flags == 0xDEADBEEF, "got (0x%08x)\n", flags);
    ok(hr == S_OK, "got (0x%08x)\n", hr);

    IExplorerBrowser_Destroy(peb);
    lres = IExplorerBrowser_Release(peb);
    ok(lres == 0, "Got %d\n", lres);
}

static BOOL test_instantiate_control(void)
{
    IExplorerBrowser *peb;
    HRESULT hr;

    hr = ebrowser_instantiate(&peb);
    ok(hr == S_OK || hr == REGDB_E_CLASSNOTREG, "Got (0x%08x)\n", hr);
    if(FAILED(hr))
        return FALSE;

    IExplorerBrowser_Release(peb);
    return TRUE;
}

static void setup_window(void)
{
    WNDCLASSW wc;
    static const WCHAR ebtestW[] = {'e','b','t','e','s','t',0};

    ZeroMemory(&wc, sizeof(WNDCLASSW));
    wc.lpfnWndProc      = DefWindowProcW;
    wc.lpszClassName    = ebtestW;
    RegisterClassW(&wc);
    hwnd = CreateWindowExW(0, ebtestW, NULL, 0,
                           0, 0, 500, 500,
                           NULL, 0, 0, NULL);
    ok(hwnd != NULL, "Failed to create window for tests.\n");
}

START_TEST(ebrowser)
{
    OleInitialize(NULL);

    if(!test_instantiate_control())
    {
        win_skip("No ExplorerBrowser control..\n");
        OleUninitialize();
        return;
    }

    setup_window();

    test_QueryInterface();
    test_SB_misc();
    test_initialization();
    test_basics();

    DestroyWindow(hwnd);
    OleUninitialize();
}
