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

static HRESULT (WINAPI *pSHCreateShellItem)(LPCITEMIDLIST,IShellFolder*,LPCITEMIDLIST,IShellItem**);
static HRESULT (WINAPI *pSHParseDisplayName)(LPCWSTR,IBindCtx*,LPITEMIDLIST*,SFGAOF,SFGAOF*);

static void init_function_pointers(void)
{
    HMODULE hmod;

    hmod = GetModuleHandleA("shell32.dll");
    pSHCreateShellItem = (void*)GetProcAddress(hmod, "SHCreateShellItem");
    pSHParseDisplayName = (void*)GetProcAddress(hmod, "SHParseDisplayName");
}

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

static HRESULT ebrowser_browse_to_desktop(IExplorerBrowser *peb)
{
    LPITEMIDLIST pidl_desktop;
    SHGetSpecialFolderLocation (hwnd, CSIDL_DESKTOP, &pidl_desktop);
    return IExplorerBrowser_BrowseToIDList(peb, pidl_desktop, 0);
}

/* Process some messages */
static void process_msgs(void)
{
    MSG msg;
    while(PeekMessage( &msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

/*********************************************************************
 * IExplorerBrowserEvents implementation
 */
typedef struct {
    const IExplorerBrowserEventsVtbl *lpVtbl;
    LONG ref;
    UINT pending, created, completed, failed;
} IExplorerBrowserEventsImpl;

static IExplorerBrowserEventsImpl ebev;

static HRESULT WINAPI IExplorerBrowserEvents_fnQueryInterface(IExplorerBrowserEvents *iface,
                                                              REFIID riid, void **ppvObj)
{
    ok(0, "Never called.\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI IExplorerBrowserEvents_fnAddRef(IExplorerBrowserEvents *iface)
{
    IExplorerBrowserEventsImpl *This = (IExplorerBrowserEventsImpl*)iface;
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IExplorerBrowserEvents_fnRelease(IExplorerBrowserEvents *iface)
{
    IExplorerBrowserEventsImpl *This = (IExplorerBrowserEventsImpl*)iface;
    return InterlockedDecrement(&This->ref);
}

static HRESULT WINAPI IExplorerBrowserEvents_fnOnNavigationPending(IExplorerBrowserEvents *iface,
                                                                   PCIDLIST_ABSOLUTE pidlFolder)
{
    IExplorerBrowserEventsImpl *This = (IExplorerBrowserEventsImpl*)iface;
    This->pending++;
    return S_OK;
}

static HRESULT WINAPI IExplorerBrowserEvents_fnOnNavigationComplete(IExplorerBrowserEvents *iface,
                                                                    PCIDLIST_ABSOLUTE pidlFolder)
{
    IExplorerBrowserEventsImpl *This = (IExplorerBrowserEventsImpl*)iface;
    This->completed++;
    return S_OK;
}
static HRESULT WINAPI IExplorerBrowserEvents_fnOnNavigationFailed(IExplorerBrowserEvents *iface,
                                                                  PCIDLIST_ABSOLUTE pidlFolder)
{
    IExplorerBrowserEventsImpl *This = (IExplorerBrowserEventsImpl*)iface;
    This->failed++;
    return S_OK;
}
static HRESULT WINAPI IExplorerBrowserEvents_fnOnViewCreated(IExplorerBrowserEvents *iface,
                                                             IShellView *psv)
{
    IExplorerBrowserEventsImpl *This = (IExplorerBrowserEventsImpl*)iface;
    This->created++;
    return S_OK;
}

static const IExplorerBrowserEventsVtbl ebevents =
{
    IExplorerBrowserEvents_fnQueryInterface,
    IExplorerBrowserEvents_fnAddRef,
    IExplorerBrowserEvents_fnRelease,
    IExplorerBrowserEvents_fnOnNavigationPending,
    IExplorerBrowserEvents_fnOnViewCreated,
    IExplorerBrowserEvents_fnOnNavigationComplete,
    IExplorerBrowserEvents_fnOnNavigationFailed
};

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
    IUnknown *punk;
    HRESULT hr;
    HWND retHwnd;
    LRESULT lres;
    LONG ref;

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

    /***** Before EB::Initialize *****/

    /* ::GetWindow */
    retHwnd = (HWND)0xDEADBEEF;
    hr = IShellBrowser_GetWindow(psb, &retHwnd);
    ok(hr == E_FAIL, "got (0x%08x)\n", hr);
    ok(retHwnd == (HWND)0xDEADBEEF, "HWND overwritten\n");

    todo_wine
    {

        /* ::SendControlMsg */
        lres = 0xDEADBEEF;
        hr = IShellBrowser_SendControlMsg(psb, FCW_STATUS, 0, 0, 0, &lres);
        ok(hr == S_OK, "got (0x%08x)\n", hr);
        ok(lres == 0, "lres was %ld\n", lres);

        lres = 0xDEADBEEF;
        hr = IShellBrowser_SendControlMsg(psb, FCW_TOOLBAR, TB_CHECKBUTTON,
                                          FCIDM_TB_SMALLICON, TRUE, &lres);
        ok(hr == S_OK, "got (0x%08x)\n", hr);
        ok(lres == 0, "lres was %ld\n", lres);

        hr = IShellBrowser_SendControlMsg(psb, FCW_STATUS, 0, 0, 0, NULL);
        ok(hr == S_OK, "got (0x%08x)\n", hr);

        hr = IShellBrowser_SendControlMsg(psb, FCW_TREE, 0, 0, 0, NULL);
        ok(hr == S_OK, "got (0x%08x)\n", hr);

        hr = IShellBrowser_SendControlMsg(psb, FCW_PROGRESS, 0, 0, 0, NULL);
        ok(hr == S_OK, "got (0x%08x)\n", hr);
    }

    /* ::QueryActiveShellView */
    hr = IShellBrowser_QueryActiveShellView(psb, (IShellView**)&punk);
    ok(hr == E_FAIL, "got (0x%08x)\n", hr);

    /* Initialize ExplorerBrowser */
    ebrowser_initialize(peb);

    /***** After EB::Initialize *****/

    /* ::GetWindow */
    hr = IShellBrowser_GetWindow(psb, &retHwnd);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    ok(GetParent(retHwnd) == hwnd, "The HWND returned is not our child.\n");

    todo_wine
    {
        /* ::SendControlMsg */
        hr = IShellBrowser_SendControlMsg(psb, FCW_STATUS, 0, 0, 0, NULL);
        ok(hr == S_OK, "got (0x%08x)\n", hr);

        lres = 0xDEADBEEF;
        hr = IShellBrowser_SendControlMsg(psb, FCW_TOOLBAR, 0, 0, 0, &lres);
        ok(hr == S_OK, "got (0x%08x)\n", hr);
        ok(lres == 0, "lres was %ld\n", lres);

        lres = 0xDEADBEEF;
        hr = IShellBrowser_SendControlMsg(psb, FCW_STATUS, 0, 0, 0, &lres);
        ok(hr == S_OK, "got (0x%08x)\n", hr);
        ok(lres == 0, "lres was %ld\n", lres);

        lres = 0xDEADBEEF;
        hr = IShellBrowser_SendControlMsg(psb, 1234, 0, 0, 0, &lres);
        ok(hr == S_OK, "got (0x%08x)\n", hr);
        ok(lres == 0, "lres was %ld\n", lres);

        /* Returns S_OK */
        hr = IShellBrowser_SetStatusTextSB(psb, NULL);
        ok(hr == S_OK, "got (0x%08x)\n", hr);

        hr = IShellBrowser_ContextSensitiveHelp(psb, FALSE);
        ok(hr == S_OK, "got (0x%08x)\n", hr);

        hr = IShellBrowser_EnableModelessSB(psb, TRUE);
        ok(hr == S_OK, "got (0x%08x)\n", hr);

        hr = IShellBrowser_SetToolbarItems(psb, NULL, 1, 1);
        ok(hr == S_OK, "got (0x%08x)\n", hr);
    }

    hr = IShellBrowser_QueryActiveShellView(psb, (IShellView**)&punk);
    ok(hr == E_FAIL, "got (0x%08x)\n", hr);

    IShellBrowser_Release(psb);
    IExplorerBrowser_Destroy(peb);
    IExplorerBrowser_Release(peb);

    /* Browse to the desktop. */
    ebrowser_instantiate(&peb);
    ebrowser_initialize(peb);
    IExplorerBrowser_QueryInterface(peb, &IID_IShellBrowser, (void**)&psb);

    process_msgs();
    hr = ebrowser_browse_to_desktop(peb);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    process_msgs();

    /****** After Browsing *****/

    hr = IShellBrowser_QueryActiveShellView(psb, (IShellView**)&punk);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    if(SUCCEEDED(hr)) IUnknown_Release(punk);

    IShellBrowser_Release(psb);
    IExplorerBrowser_Destroy(peb);
    ref = IExplorerBrowser_Release(peb);
    ok(ref == 0, "Got %d\n", ref);
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
    FOLDERSETTINGS fs;
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
    IExplorerBrowser_Release(peb);

    ebrowser_instantiate(&peb);
    ebrowser_initialize(peb);

    /* SetFolderSettings */
    hr = IExplorerBrowser_SetFolderSettings(peb, NULL);
    ok(hr == E_INVALIDARG, "got (0x%08x)\n", hr);
    fs.ViewMode = 0; fs.fFlags = 0;
    hr = IExplorerBrowser_SetFolderSettings(peb, &fs);
    todo_wine ok(hr == E_INVALIDARG, "got (0x%08x)\n", hr);

    /* TODO: Test after browsing somewhere. */

    IExplorerBrowser_Destroy(peb);
    lres = IExplorerBrowser_Release(peb);
    ok(lres == 0, "Got %d\n", lres);
}

static void test_Advise(void)
{
    IExplorerBrowser *peb;
    IExplorerBrowserEvents *pebe;
    DWORD cookies[10];
    HRESULT hr;
    UINT i, ref;

    /* Set up our IExplorerBrowserEvents implementation */
    ebev.lpVtbl = &ebevents;
    pebe = (IExplorerBrowserEvents*) &ebev;

    ebrowser_instantiate(&peb);

    if(0)
    {
        /* Crashes on Windows 7 */
        IExplorerBrowser_Advise(peb, pebe, NULL);
        IExplorerBrowser_Advise(peb, NULL, &cookies[0]);
    }

    /* Using Unadvise with a cookie that has yet to be given out
     * results in E_INVALIDARG */
    hr = IExplorerBrowser_Unadvise(peb, 11);
    ok(hr == E_INVALIDARG, "got (0x%08x)\n", hr);

    /* Add some before initialization */
    for(i = 0; i < 5; i++)
    {
        hr = IExplorerBrowser_Advise(peb, pebe, &cookies[i]);
        ok(hr == S_OK, "got (0x%08x)\n", hr);
    }

    ebrowser_initialize(peb);

    /* Add some after initialization */
    for(i = 5; i < 10; i++)
    {
        hr = IExplorerBrowser_Advise(peb, pebe, &cookies[i]);
        ok(hr == S_OK, "got (0x%08x)\n", hr);
    }

    ok(ebev.ref == 10, "Got %d\n", ebev.ref);

    ebev.completed = 0;
    ebrowser_browse_to_desktop(peb);
    process_msgs();
    ok(ebev.completed == 10, "Got %d\n", ebev.completed);

    /* Remove a bunch somewhere in the middle */
    for(i = 4; i < 8; i++)
    {
        hr = IExplorerBrowser_Unadvise(peb, cookies[i]);
        ok(hr == S_OK, "got (0x%08x)\n", hr);
    }

    ebev.completed = 0;
    ebrowser_browse_to_desktop(peb);
    process_msgs();
    ok(ebev.completed == 6, "Got %d\n", ebev.completed);

    if(0)
    {
        /* Using unadvise with a previously unadvised cookie results
         * in a crash. */
        hr = IExplorerBrowser_Unadvise(peb, cookies[5]);
    }

    /* Remove the rest. */
    for(i = 0; i < 10; i++)
    {
        if(i<4||i>7)
        {
            hr = IExplorerBrowser_Unadvise(peb, cookies[i]);
            ok(hr == S_OK, "%d: got (0x%08x)\n", i, hr);
        }
    }

    ok(ebev.ref == 0, "Got %d\n", ebev.ref);

    ebev.completed = 0;
    ebrowser_browse_to_desktop(peb);
    process_msgs();
    ok(ebev.completed == 0, "Got %d\n", ebev.completed);

    /* ::Destroy implies ::Unadvise. */
    hr = IExplorerBrowser_Advise(peb, pebe, &cookies[0]);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    ok(ebev.ref == 1, "Got %d\n", ebev.ref);

    hr = IExplorerBrowser_Destroy(peb);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    ok(ebev.ref == 0, "Got %d\n", ebev.ref);

    ref = IExplorerBrowser_Release(peb);
    ok(!ref, "Got %d", ref);
}

/* Based on PathAddBackslashW from dlls/shlwapi/path.c */
static LPWSTR myPathAddBackslashW( LPWSTR lpszPath )
{
  size_t iLen;

  if (!lpszPath || (iLen = lstrlenW(lpszPath)) >= MAX_PATH)
    return NULL;

  if (iLen)
  {
    lpszPath += iLen;
    if (lpszPath[-1] != '\\')
    {
      *lpszPath++ = '\\';
      *lpszPath = '\0';
    }
  }
  return lpszPath;
}

static void test_browse_pidl_(IExplorerBrowser *peb, IExplorerBrowserEventsImpl *ebev,
                              LPITEMIDLIST pidl, UINT uFlags,
                              HRESULT hr_exp, UINT pending, UINT created, UINT failed, UINT completed,
                              const char *file, int line)
{
    HRESULT hr;
    ebev->completed = ebev->created = ebev->pending = ebev->failed = 0;

    hr = IExplorerBrowser_BrowseToIDList(peb, pidl, uFlags);
    ok_(file, line) (hr == hr_exp, "BrowseToIDList returned 0x%08x\n", hr);
    process_msgs();

    ok_(file, line)
        (ebev->pending == pending && ebev->created == created &&
         ebev->failed == failed && ebev->completed == completed,
         "Events occurred: %d, %d, %d, %d\n",
         ebev->pending, ebev->created, ebev->failed, ebev->completed);
}
#define test_browse_pidl(peb, ebev, pidl, uFlags, hr, p, cr, f, co)     \
    test_browse_pidl_(peb, ebev, pidl, uFlags, hr, p, cr, f, co, __FILE__, __LINE__)

static void test_browse_pidl_sb_(IExplorerBrowser *peb, IExplorerBrowserEventsImpl *ebev,
                                 LPITEMIDLIST pidl, UINT uFlags,
                                 HRESULT hr_exp, UINT pending, UINT created, UINT failed, UINT completed,
                                 const char *file, int line)
{
    IShellBrowser *psb;
    HRESULT hr;

    hr = IExplorerBrowser_QueryInterface(peb, &IID_IShellBrowser, (void**)&psb);
    ok_(file, line) (hr == S_OK, "QueryInterface returned 0x%08x\n", hr);
    if(SUCCEEDED(hr))
    {
        ebev->completed = ebev->created = ebev->pending = ebev->failed = 0;

        hr = IShellBrowser_BrowseObject(psb, pidl, uFlags);
        ok_(file, line) (hr == hr_exp, "BrowseObject returned 0x%08x\n", hr);
        process_msgs();

        ok_(file, line)
            (ebev->pending == pending && ebev->created == created &&
             ebev->failed == failed && ebev->completed == completed,
             "Events occurred: %d, %d, %d, %d\n",
             ebev->pending, ebev->created, ebev->failed, ebev->completed);

        IShellBrowser_Release(psb);
    }
}
#define test_browse_pidl_sb(peb, ebev, pidl, uFlags, hr, p, cr, f, co)  \
    test_browse_pidl_sb_(peb, ebev, pidl, uFlags, hr, p, cr, f, co, __FILE__, __LINE__)

static void test_navigation(void)
{
    IExplorerBrowser *peb, *peb2;
    IFolderView *pfv;
    IShellItem *psi;
    IShellFolder *psf;
    LPITEMIDLIST pidl_current, pidl_child;
    DWORD cookie, cookie2;
    HRESULT hr;
    LONG lres;
    WCHAR current_path[MAX_PATH];
    WCHAR child_path[MAX_PATH];
    static const WCHAR testfolderW[] =
        {'w','i','n','e','t','e','s','t','f','o','l','d','e','r','\0'};

    ok(pSHParseDisplayName != NULL, "pSHParseDisplayName unexpectedly missing.\n");
    ok(pSHCreateShellItem != NULL, "pSHCreateShellItem unexpectedly missing.\n");

    GetCurrentDirectoryW(MAX_PATH, current_path);
    if(!lstrlenW(current_path))
    {
        skip("Failed to create test-directory.\n");
        return;
    }

    lstrcpyW(child_path, current_path);
    myPathAddBackslashW(child_path);
    lstrcatW(child_path, testfolderW);

    CreateDirectoryW(child_path, NULL);

    pSHParseDisplayName(current_path, NULL, &pidl_current, 0, NULL);
    pSHParseDisplayName(child_path, NULL, &pidl_child, 0, NULL);

    ebrowser_instantiate(&peb);
    ebrowser_initialize(peb);

    ebrowser_instantiate(&peb2);
    ebrowser_initialize(peb2);

    /* Set up our IExplorerBrowserEvents implementation */
    ebev.lpVtbl = &ebevents;

    IExplorerBrowser_Advise(peb, (IExplorerBrowserEvents*)&ebev, &cookie);
    IExplorerBrowser_Advise(peb2, (IExplorerBrowserEvents*)&ebev, &cookie2);

    /* These should all fail */
    test_browse_pidl(peb, &ebev, 0, SBSP_ABSOLUTE | SBSP_RELATIVE, E_FAIL, 0, 0, 0, 0);
    test_browse_pidl_sb(peb2, &ebev, 0, SBSP_ABSOLUTE | SBSP_RELATIVE, E_FAIL, 0, 0, 0, 0);
    test_browse_pidl(peb, &ebev, 0, SBSP_ABSOLUTE, E_INVALIDARG, 0, 0, 0, 0);
    test_browse_pidl_sb(peb2, &ebev, 0, SBSP_ABSOLUTE, E_INVALIDARG, 0, 0, 0, 0);
    test_browse_pidl(peb, &ebev, 0, SBSP_RELATIVE, E_FAIL, 0, 0, 0, 0);
    test_browse_pidl_sb(peb2, &ebev, 0, SBSP_RELATIVE, E_FAIL, 0, 0, 0, 0);
    test_browse_pidl(peb, &ebev, 0, SBSP_PARENT, E_FAIL, 0, 0, 0, 0);
    test_browse_pidl_sb(peb2, &ebev, 0, SBSP_PARENT, E_FAIL, 0, 0, 0, 0);

    /* "The first browse is synchronous" */
    test_browse_pidl(peb, &ebev, pidl_child, SBSP_ABSOLUTE, S_OK, 1, 1, 0, 1);
    test_browse_pidl_sb(peb2, &ebev, pidl_child, SBSP_ABSOLUTE, S_OK, 1, 1, 0, 1);

    /* Relative navigation */
    test_browse_pidl(peb, &ebev, pidl_current, SBSP_ABSOLUTE, S_OK, 1, 1, 0, 1);
    test_browse_pidl_sb(peb2, &ebev, pidl_current, SBSP_ABSOLUTE, S_OK, 1, 1, 0, 1);

    hr = IExplorerBrowser_GetCurrentView(peb, &IID_IFolderView, (void**)&pfv);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    if(SUCCEEDED(hr))
    {
        LPITEMIDLIST pidl_relative;

        hr = IFolderView_GetFolder(pfv, &IID_IShellFolder, (void**)&psf);
        ok(hr == S_OK, "Got 0x%08x\n", hr);
        hr = IShellFolder_ParseDisplayName(psf, NULL, NULL, (LPWSTR)testfolderW,
                                           NULL, &pidl_relative, NULL);
        ok(hr == S_OK, "Got 0x%08x\n", hr);

        /* Browsing to another location here before using the
         * pidl_relative would make ExplorerBrowser in Windows 7 show a
         * not-available dialog. Also, passing a relative pidl without
         * specifying SBSP_RELATIVE makes it look for the pidl on the
         * desktop
         */

        test_browse_pidl(peb, &ebev, pidl_relative, SBSP_RELATIVE, S_OK, 1, 1, 0, 1);
        test_browse_pidl_sb(peb2, &ebev, pidl_relative, SBSP_RELATIVE, S_OK, 1, 1, 0, 1);

        ILFree(pidl_relative);
        /* IShellFolder_Release(psf); */
        IFolderView_Release(pfv);
    }

    /* misc **/
    test_browse_pidl(peb, &ebev, NULL, SBSP_ABSOLUTE, S_OK, 0, 0, 0, 0);
    test_browse_pidl_sb(peb2, &ebev, NULL, SBSP_ABSOLUTE, S_OK, 0, 0, 0, 0);
    test_browse_pidl(peb, &ebev, NULL, SBSP_DEFBROWSER, S_OK, 0, 0, 0, 0);
    test_browse_pidl_sb(peb2, &ebev, NULL, SBSP_DEFBROWSER, S_OK, 0, 0, 0, 0);
    test_browse_pidl(peb, &ebev, pidl_current, SBSP_SAMEBROWSER, S_OK, 1, 1, 0, 1);
    test_browse_pidl_sb(peb2, &ebev, pidl_current, SBSP_SAMEBROWSER, S_OK, 1, 1, 0, 1);
    test_browse_pidl(peb, &ebev, pidl_current, SBSP_SAMEBROWSER, S_OK, 1, 0, 0, 1);
    test_browse_pidl_sb(peb2, &ebev, pidl_current, SBSP_SAMEBROWSER, S_OK, 1, 0, 0, 1);

    test_browse_pidl(peb, &ebev, pidl_current, SBSP_EXPLOREMODE, E_INVALIDARG, 0, 0, 0, 0);
    test_browse_pidl_sb(peb2, &ebev, pidl_current, SBSP_EXPLOREMODE, E_INVALIDARG, 0, 0, 0, 0);
    test_browse_pidl(peb, &ebev, pidl_current, SBSP_OPENMODE, S_OK, 1, 0, 0, 1);
    test_browse_pidl_sb(peb2, &ebev, pidl_current, SBSP_OPENMODE, S_OK, 1, 0, 0, 1);

    /* SBSP_NEWBROWSER will return E_INVALIDARG, claims MSDN, but in
     * reality it works as one would expect (Windows 7 only?).
     */
    if(0)
    {
        IExplorerBrowser_BrowseToIDList(peb, NULL, SBSP_NEWBROWSER);
    }

    hr = IExplorerBrowser_Unadvise(peb, cookie);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    IExplorerBrowser_Destroy(peb);
    process_msgs();
    hr = IExplorerBrowser_Unadvise(peb2, cookie2);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    IExplorerBrowser_Destroy(peb2);
    process_msgs();

    /* Attempt browsing after destroyed */
    test_browse_pidl(peb, &ebev, pidl_child, SBSP_ABSOLUTE, HRESULT_FROM_WIN32(ERROR_BUSY), 0, 0, 0, 0);
    test_browse_pidl_sb(peb2, &ebev, pidl_child, SBSP_ABSOLUTE, HRESULT_FROM_WIN32(ERROR_BUSY), 0, 0, 0, 0);

    lres = IExplorerBrowser_Release(peb);
    ok(lres == 0, "Got lres %d\n", lres);
    lres = IExplorerBrowser_Release(peb2);
    ok(lres == 0, "Got lres %d\n", lres);

    /******************************************/
    /* Test some options that affect browsing */

    ebrowser_instantiate(&peb);
    hr = IExplorerBrowser_Advise(peb, (IExplorerBrowserEvents*)&ebev, &cookie);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    hr = IExplorerBrowser_SetOptions(peb, EBO_NAVIGATEONCE);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    ebrowser_initialize(peb);

    test_browse_pidl(peb, &ebev, pidl_current, 0, S_OK, 1, 1, 0, 1);
    test_browse_pidl(peb, &ebev, pidl_current, 0, E_FAIL, 0, 0, 0, 0);

    hr = IExplorerBrowser_SetOptions(peb, 0);
    ok(hr == S_OK, "got (0x%08x)\n", hr);

    test_browse_pidl(peb, &ebev, pidl_current, 0, S_OK, 1, 0, 0, 1);
    test_browse_pidl(peb, &ebev, pidl_current, 0, S_OK, 1, 0, 0, 1);

    /* Difference in behavior lies where? */
    hr = IExplorerBrowser_SetOptions(peb, EBO_ALWAYSNAVIGATE);
    ok(hr == S_OK, "got (0x%08x)\n", hr);

    test_browse_pidl(peb, &ebev, pidl_current, 0, S_OK, 1, 0, 0, 1);
    test_browse_pidl(peb, &ebev, pidl_current, 0, S_OK, 1, 0, 0, 1);

    hr = IExplorerBrowser_Unadvise(peb, cookie);
    ok(hr == S_OK, "Got 0x%08x\n", hr);

    IExplorerBrowser_Destroy(peb);
    lres = IExplorerBrowser_Release(peb);
    ok(lres == 0, "Got lres %d\n", lres);

    /* BrowseToObject tests */
    ebrowser_instantiate(&peb);
    ebrowser_initialize(peb);

    /* Browse to the desktop by passing an IShellFolder */
    hr = SHGetDesktopFolder(&psf);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IExplorerBrowser_BrowseToObject(peb, (IUnknown*)psf, SBSP_DEFBROWSER);
        ok(hr == S_OK, "got (0x%08x)\n", hr);
        if(hr == S_OK) process_msgs();

        IShellFolder_Release(psf);
    }

    /* Browse to the current directory by passing a ShellItem */
    hr = pSHCreateShellItem(NULL, NULL, pidl_current, &psi);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IExplorerBrowser_BrowseToObject(peb, (IUnknown*)psi, SBSP_DEFBROWSER);
        ok(hr == S_OK, "got (0x%08x)\n", hr);
        process_msgs();

        IShellItem_Release(psi);
    }

    IExplorerBrowser_Destroy(peb);
    lres = IExplorerBrowser_Release(peb);
    ok(lres == 0, "Got lres %d\n", lres);

    /* Cleanup */
    RemoveDirectoryW(child_path);
    ILFree(pidl_current);
    ILFree(pidl_child);
}

static void test_GetCurrentView(void)
{
    IExplorerBrowser *peb;
    IUnknown *punk;
    HRESULT hr;

    /* GetCurrentView */
    ebrowser_instantiate(&peb);

    if(0)
    {
        /* Crashes under Windows 7 */
        hr = IExplorerBrowser_GetCurrentView(peb, NULL, NULL);
    }
    hr = IExplorerBrowser_GetCurrentView(peb, NULL, (void**)&punk);
    ok(hr == E_FAIL, "Got 0x%08x\n", hr);

#define test_gcv(iid, exp)                                              \
    do {                                                                \
        hr = IExplorerBrowser_GetCurrentView(peb, &iid, (void**)&punk); \
        ok(hr == exp, "(%s:)Expected (0x%08x), got: (0x%08x)\n",        \
           #iid ,exp, hr);                                              \
        if(SUCCEEDED(hr)) IUnknown_Release(punk);                       \
    } while(0)

    test_gcv(IID_IUnknown, E_FAIL);
    test_gcv(IID_IUnknown, E_FAIL);
    test_gcv(IID_IShellView, E_FAIL);
    test_gcv(IID_IShellView2, E_FAIL);
    test_gcv(IID_IFolderView, E_FAIL);
    test_gcv(IID_IPersistFolder, E_FAIL);
    test_gcv(IID_IPersistFolder2, E_FAIL);
    test_gcv(IID_ICommDlgBrowser, E_FAIL);
    test_gcv(IID_ICommDlgBrowser2, E_FAIL);
    test_gcv(IID_ICommDlgBrowser3, E_FAIL);

    ebrowser_initialize(peb);
    ebrowser_browse_to_desktop(peb);

    test_gcv(IID_IUnknown, S_OK);
    test_gcv(IID_IUnknown, S_OK);
    test_gcv(IID_IShellView, S_OK);
    test_gcv(IID_IShellView2, S_OK);
    test_gcv(IID_IFolderView, S_OK);
    todo_wine test_gcv(IID_IPersistFolder, S_OK);
    test_gcv(IID_IPersistFolder2, E_NOINTERFACE);
    test_gcv(IID_ICommDlgBrowser, E_NOINTERFACE);
    test_gcv(IID_ICommDlgBrowser2, E_NOINTERFACE);
    test_gcv(IID_ICommDlgBrowser3, E_NOINTERFACE);

#undef test_gcv

    IExplorerBrowser_Destroy(peb);
    IExplorerBrowser_Release(peb);
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
    init_function_pointers();

    test_QueryInterface();
    test_SB_misc();
    test_initialization();
    test_basics();
    test_Advise();
    test_navigation();
    test_GetCurrentView();

    DestroyWindow(hwnd);
    OleUninitialize();
}
