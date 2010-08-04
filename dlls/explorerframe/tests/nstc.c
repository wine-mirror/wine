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

static HRESULT (WINAPI *pSHCreateShellItem)(LPCITEMIDLIST,IShellFolder*,LPCITEMIDLIST,IShellItem**);
static HRESULT (WINAPI *pSHGetIDListFromObject)(IUnknown*, PIDLIST_ABSOLUTE*);

static void init_function_pointers(void)
{
    HMODULE hmod;

    hmod = GetModuleHandleA("shell32.dll");
    pSHCreateShellItem = (void*)GetProcAddress(hmod, "SHCreateShellItem");
    pSHGetIDListFromObject = (void*)GetProcAddress(hmod, "SHGetIDListFromObject");
}

/*******************************************************
 * INameSpaceTreeControlEvents implementation.
 */
enum { OnItemClick = 0, OnPropertyItemCommit, OnItemStateChanging, OnItemStateChanged,
       OnSelectionChanged, OnKeyboardInput, OnBeforeExpand, OnAfterExpand, OnBeginLabelEdit,
       OnEndLabelEdit, OnGetToolTip, OnBeforeItemDelete, OnItemAdded, OnItemDeleted,
       OnBeforeContextMenu, OnAfterContextMenu, OnBeforeStateImageChange, OnGetDefaultIconIndex,
       LastEvent };

typedef struct {
    const INameSpaceTreeControlEventsVtbl *lpVtbl;
    UINT qi_called_count;     /* Keep track of calls to QueryInterface */
    BOOL qi_enable_events;    /* If FALSE, QueryInterface returns only E_NOINTERFACE */
    UINT count[LastEvent];    /* Keep track of calls to all On* functions. */
    LONG ref;
} INameSpaceTreeControlEventsImpl;

#define NSTCE_IMPL(iface)                       \
    ((INameSpaceTreeControlEventsImpl*)iface)

static HRESULT WINAPI NSTCEvents_fnQueryInterface(
    INameSpaceTreeControlEvents* iface,
    REFIID riid,
    void **ppvObject)
{
    NSTCE_IMPL(iface)->qi_called_count++;

    if(NSTCE_IMPL(iface)->qi_enable_events &&
       IsEqualIID(riid, &IID_INameSpaceTreeControlEvents))
    {
        IUnknown_AddRef(iface);
        *ppvObject = iface;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI NSTCEvents_fnAddRef(
    INameSpaceTreeControlEvents* iface)
{
    return InterlockedIncrement(&NSTCE_IMPL(iface)->ref);
}

static ULONG WINAPI NSTCEvents_fnRelease(
    INameSpaceTreeControlEvents* iface)
{
    return InterlockedDecrement(&NSTCE_IMPL(iface)->ref);
}

static HRESULT WINAPI NSTCEvents_fnOnItemClick(
    INameSpaceTreeControlEvents* iface,
    IShellItem *psi,
    NSTCEHITTEST nstceHitTest,
    NSTCECLICKTYPE nstceClickType)
{
    ok(psi != NULL, "NULL IShellItem\n");
    NSTCE_IMPL(iface)->count[OnItemClick]++;
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnPropertyItemCommit(
    INameSpaceTreeControlEvents* iface,
    IShellItem *psi)
{
    ok(psi != NULL, "NULL IShellItem\n");
    NSTCE_IMPL(iface)->count[OnPropertyItemCommit]++;
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnItemStateChanging(
    INameSpaceTreeControlEvents* iface,
    IShellItem *psi,
    NSTCITEMSTATE nstcisMask,
    NSTCITEMSTATE nstcisState)
{
    ok(psi != NULL, "NULL IShellItem\n");
    NSTCE_IMPL(iface)->count[OnItemStateChanging]++;
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnItemStateChanged(
    INameSpaceTreeControlEvents* iface,
    IShellItem *psi,
    NSTCITEMSTATE nstcisMask,
    NSTCITEMSTATE nstcisState)
{
    ok(psi != NULL, "NULL IShellItem\n");
    NSTCE_IMPL(iface)->count[OnItemStateChanged]++;
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnSelectionChanged(
    INameSpaceTreeControlEvents* iface,
    IShellItemArray *psiaSelection)
{
    ok(psiaSelection != NULL, "IShellItemArray was NULL.\n");
    if(psiaSelection)
    {
        HRESULT hr;
        DWORD count = 0xdeadbeef;
        hr = IShellItemArray_GetCount(psiaSelection, &count);
        ok(hr == S_OK, "Got 0x%08x\n", hr);
        ok(count == 1, "Got count 0x%x\n", count);
    }
    NSTCE_IMPL(iface)->count[OnSelectionChanged]++;
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnKeyboardInput(
    INameSpaceTreeControlEvents* iface,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    NSTCE_IMPL(iface)->count[OnKeyboardInput]++;
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnBeforeExpand(
    INameSpaceTreeControlEvents* iface,
    IShellItem *psi)
{
    ok(psi != NULL, "NULL IShellItem\n");
    NSTCE_IMPL(iface)->count[OnBeforeExpand]++;
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnAfterExpand(
    INameSpaceTreeControlEvents* iface,
    IShellItem *psi)
{
    ok(psi != NULL, "NULL IShellItem\n");
    NSTCE_IMPL(iface)->count[OnAfterExpand]++;
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnBeginLabelEdit(
    INameSpaceTreeControlEvents* iface,
    IShellItem *psi)
{
    ok(psi != NULL, "NULL IShellItem\n");
    NSTCE_IMPL(iface)->count[OnBeginLabelEdit]++;
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnEndLabelEdit(
    INameSpaceTreeControlEvents* iface,
    IShellItem *psi)
{
    ok(psi != NULL, "NULL IShellItem\n");
    NSTCE_IMPL(iface)->count[OnEndLabelEdit]++;
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnGetToolTip(
    INameSpaceTreeControlEvents* iface,
    IShellItem *psi,
    LPWSTR pszTip,
    int cchTip)
{
    ok(psi != NULL, "NULL IShellItem\n");
    NSTCE_IMPL(iface)->count[OnGetToolTip]++;
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnBeforeItemDelete(
    INameSpaceTreeControlEvents* iface,
    IShellItem *psi)
{
    ok(psi != NULL, "NULL IShellItem\n");
    NSTCE_IMPL(iface)->count[OnBeforeItemDelete]++;
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnItemAdded(
    INameSpaceTreeControlEvents* iface,
    IShellItem *psi,
    BOOL fIsRoot)
{
    ok(psi != NULL, "NULL IShellItem\n");
    NSTCE_IMPL(iface)->count[OnItemAdded]++;
    return S_OK;
}

static HRESULT WINAPI NSTCEvents_fnOnItemDeleted(
    INameSpaceTreeControlEvents* iface,
    IShellItem *psi,
    BOOL fIsRoot)
{
    ok(psi != NULL, "NULL IShellItem\n");
    NSTCE_IMPL(iface)->count[OnItemDeleted]++;
    return S_OK;
}

static HRESULT WINAPI NSTCEvents_fnOnBeforeContextMenu(
    INameSpaceTreeControlEvents* iface,
    IShellItem *psi,
    REFIID riid,
    void **ppv)
{
    NSTCE_IMPL(iface)->count[OnBeforeContextMenu]++;
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnAfterContextMenu(
    INameSpaceTreeControlEvents* iface,
    IShellItem *psi,
    IContextMenu *pcmIn,
    REFIID riid,
    void **ppv)
{
    NSTCE_IMPL(iface)->count[OnAfterContextMenu]++;
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnBeforeStateImageChange(
    INameSpaceTreeControlEvents* iface,
    IShellItem *psi,
    int *piDefaultIcon,
    int *piOpenIcon)
{
    ok(psi != NULL, "NULL IShellItem\n");
    NSTCE_IMPL(iface)->count[OnBeforeStateImageChange]++;
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTCEvents_fnOnGetDefaultIconIndex(
    INameSpaceTreeControlEvents* iface,
    IShellItem *psi,
    int *piDefaultIcon,
    int *piOpenIcon)
{
    ok(psi != NULL, "NULL IShellItem\n");
    NSTCE_IMPL(iface)->count[OnGetDefaultIconIndex]++;
    return E_NOTIMPL;
}

const INameSpaceTreeControlEventsVtbl vt_NSTCEvents = {
    NSTCEvents_fnQueryInterface,
    NSTCEvents_fnAddRef,
    NSTCEvents_fnRelease,
    NSTCEvents_fnOnItemClick,
    NSTCEvents_fnOnPropertyItemCommit,
    NSTCEvents_fnOnItemStateChanging,
    NSTCEvents_fnOnItemStateChanged,
    NSTCEvents_fnOnSelectionChanged,
    NSTCEvents_fnOnKeyboardInput,
    NSTCEvents_fnOnBeforeExpand,
    NSTCEvents_fnOnAfterExpand,
    NSTCEvents_fnOnBeginLabelEdit,
    NSTCEvents_fnOnEndLabelEdit,
    NSTCEvents_fnOnGetToolTip,
    NSTCEvents_fnOnBeforeItemDelete,
    NSTCEvents_fnOnItemAdded,
    NSTCEvents_fnOnItemDeleted,
    NSTCEvents_fnOnBeforeContextMenu,
    NSTCEvents_fnOnAfterContextMenu,
    NSTCEvents_fnOnBeforeStateImageChange,
    NSTCEvents_fnOnGetDefaultIconIndex
};
#undef NSTCE_IMPL

static INameSpaceTreeControlEventsImpl *create_nstc_events(void)
{
    INameSpaceTreeControlEventsImpl *This;
    This = HeapAlloc(GetProcessHeap(), 0, sizeof(INameSpaceTreeControlEventsImpl));
    This->lpVtbl = &vt_NSTCEvents;
    This->ref = 1;

    return This;
}

/* Process some messages */
static void process_msgs(void)
{
    MSG msg;
    BOOL got_msg;
    do {
        got_msg = FALSE;
        Sleep(100);
        while(PeekMessage( &msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            got_msg = TRUE;
        }
    } while(got_msg);

    /* There seem to be a timer that sometimes fires after about
       500ms, we need to wait for it. Failing to wait can result in
       seemingly sporadic selection change events. (Timer ID is 87,
       sending WM_TIMER manually does not seem to help us.) */
    Sleep(500);

    while(PeekMessage( &msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

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

static void test_basics(void)
{
    INameSpaceTreeControl *pnstc;
    INameSpaceTreeControl2 *pnstc2;
    IShellFolder *psfdesktop;
    IShellItem *psidesktop, *psidesktop2;
    IOleWindow *pow;
    LPITEMIDLIST pidl_desktop;
    HRESULT hr;
    UINT i, res;
    RECT rc;

    /* These should exist on platforms supporting the NSTC */
    ok(pSHCreateShellItem != NULL, "No SHCreateShellItem.\n");
    ok(pSHGetIDListFromObject != NULL, "No SHCreateShellItem.\n");

    /* Create ShellItems for testing. */
    SHGetDesktopFolder(&psfdesktop);
    hr = pSHGetIDListFromObject((IUnknown*)psfdesktop, &pidl_desktop);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = pSHCreateShellItem(NULL, NULL, pidl_desktop, &psidesktop);
        ok(hr == S_OK, "Got 0x%08x\n", hr);
        if(SUCCEEDED(hr))
        {
            hr = pSHCreateShellItem(NULL, NULL, pidl_desktop, &psidesktop2);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            if(FAILED(hr)) IShellItem_Release(psidesktop);
        }
        ILFree(pidl_desktop);
    }
    ok(psidesktop != psidesktop2, "psidesktop == psidesktop2\n");
    IShellFolder_Release(psfdesktop);

    if(FAILED(hr))
    {
        win_skip("Test setup failed.\n");
        return;
    }

    hr = CoCreateInstance(&CLSID_NamespaceTreeControl, NULL, CLSCTX_INPROC_SERVER,
                          &IID_INameSpaceTreeControl, (void**)&pnstc);
    ok(hr == S_OK, "Failed to initialize control (0x%08x)\n", hr);

    /* Some tests on an uninitialized control */
    hr = INameSpaceTreeControl_RemoveAllRoots(pnstc);
    ok(hr == E_INVALIDARG, "Got (0x%08x)\n", hr);
    hr = INameSpaceTreeControl_RemoveRoot(pnstc, psidesktop);
    ok(hr == E_FAIL, "Got (0x%08x)\n", hr);
    hr = INameSpaceTreeControl_RemoveRoot(pnstc, NULL);
    ok(hr == E_NOINTERFACE, "Got (0x%08x)\n", hr);
    hr = INameSpaceTreeControl_AppendRoot(pnstc, psidesktop, SHCONTF_NONFOLDERS, 0, NULL);
    ok(hr == E_FAIL, "Got (0x%08x)\n", hr);
    process_msgs();

    /* Initialize the control */
    rc.top = rc.left = 0; rc.right = rc.bottom = 200;
    hr = INameSpaceTreeControl_Initialize(pnstc, hwnd, &rc, 0);
    ok(hr == S_OK, "Got (0x%08x)\n", hr);


    /* Set/GetControlStyle(2) */
    hr = INameSpaceTreeControl_QueryInterface(pnstc, &IID_INameSpaceTreeControl2, (void**)&pnstc2);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE), "Got 0x%08x\n", hr);
    if(SUCCEEDED(hr))
    {
        DWORD tmp;
        NSTCSTYLE style;
        NSTCSTYLE2 style2;
        static const NSTCSTYLE2 styles2[] =
            { NSTCS2_INTERRUPTNOTIFICATIONS,NSTCS2_SHOWNULLSPACEMENU,
              NSTCS2_DISPLAYPADDING,NSTCS2_DISPLAYPINNEDONLY,
              NTSCS2_NOSINGLETONAUTOEXPAND,NTSCS2_NEVERINSERTNONENUMERATED, 0};


        /* We can use this to differentiate between two versions of
         * this interface. Windows 7 returns hr == S_OK. */
        hr = INameSpaceTreeControl2_SetControlStyle(pnstc2, 0, 0);
        ok(hr == S_OK || broken(hr == E_FAIL), "Got 0x%08x\n", hr);
        if(hr == S_OK)
        {
            static const NSTCSTYLE styles_setable[] =
                { NSTCS_HASEXPANDOS,NSTCS_HASLINES,NSTCS_SINGLECLICKEXPAND,
                  NSTCS_FULLROWSELECT,NSTCS_HORIZONTALSCROLL,
                  NSTCS_ROOTHASEXPANDO,NSTCS_SHOWSELECTIONALWAYS,NSTCS_NOINFOTIP,
                  NSTCS_EVENHEIGHT,NSTCS_NOREPLACEOPEN,NSTCS_DISABLEDRAGDROP,
                  NSTCS_NOORDERSTREAM,NSTCS_BORDER,NSTCS_NOEDITLABELS,
                  NSTCS_TABSTOP,NSTCS_FAVORITESMODE,NSTCS_EMPTYTEXT,NSTCS_CHECKBOXES,
                  NSTCS_ALLOWJUNCTIONS,NSTCS_SHOWTABSBUTTON,NSTCS_SHOWDELETEBUTTON,
                  NSTCS_SHOWREFRESHBUTTON, 0};
            static const NSTCSTYLE styles_nonsetable[] =
                { NSTCS_SPRINGEXPAND, NSTCS_RICHTOOLTIP, NSTCS_AUTOHSCROLL,
                  NSTCS_FADEINOUTEXPANDOS,
                  NSTCS_PARTIALCHECKBOXES, NSTCS_EXCLUSIONCHECKBOXES,
                  NSTCS_DIMMEDCHECKBOXES, NSTCS_NOINDENTCHECKS,0};

            /* Set/GetControlStyle */
            style = style2 = 0xdeadbeef;
            hr = INameSpaceTreeControl2_GetControlStyle(pnstc2, 0xFFFFFFFF, &style);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            ok(style == 0, "Got style %x\n", style);

            hr = INameSpaceTreeControl2_SetControlStyle(pnstc2, 0, 0xFFFFFFF);
            ok(hr == S_OK, "Got 0x%08x\n", hr);

            hr = INameSpaceTreeControl2_SetControlStyle(pnstc2, 0xFFFFFFFF, 0);
            ok(hr == E_FAIL, "Got 0x%08x\n", hr);
            hr = INameSpaceTreeControl2_SetControlStyle(pnstc2, 0xFFFFFFFF, 0xFFFFFFFF);
            ok(hr == E_FAIL, "Got 0x%08x\n", hr);

            tmp = 0;
            for(i = 0; styles_setable[i] != 0; i++)
            {
                hr = INameSpaceTreeControl2_SetControlStyle(pnstc2, styles_setable[i], styles_setable[i]);
                ok(hr == S_OK, "Got 0x%08x (%x)\n", hr, styles_setable[i]);
                if(SUCCEEDED(hr)) tmp |= styles_setable[i];
            }
            for(i = 0; styles_nonsetable[i] != 0; i++)
            {
                hr = INameSpaceTreeControl2_SetControlStyle(pnstc2, styles_nonsetable[i], styles_nonsetable[i]);
                ok(hr == E_FAIL, "Got 0x%08x (%x)\n", hr, styles_nonsetable[i]);
            }

            hr = INameSpaceTreeControl2_GetControlStyle(pnstc2, 0xFFFFFFFF, &style);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            ok(style == tmp, "Got style %x (expected %x)\n", style, tmp);
            if(SUCCEEDED(hr))
            {
                DWORD tmp2;
                for(i = 0; styles_setable[i] != 0; i++)
                {
                    hr = INameSpaceTreeControl2_GetControlStyle(pnstc2, styles_setable[i], &tmp2);
                    ok(hr == S_OK, "Got 0x%08x\n", hr);
                    ok(tmp2 == (style & styles_setable[i]), "Got %x\n", tmp2);
                }
            }

            for(i = 0; styles_setable[i] != 0; i++)
            {
                hr = INameSpaceTreeControl2_SetControlStyle(pnstc2, styles_setable[i], 0);
                ok(hr == S_OK, "Got 0x%08x (%x)\n", hr, styles_setable[i]);
            }
            for(i = 0; styles_nonsetable[i] != 0; i++)
            {
                hr = INameSpaceTreeControl2_SetControlStyle(pnstc2, styles_nonsetable[i], 0);
                ok(hr == E_FAIL, "Got 0x%08x (%x)\n", hr, styles_nonsetable[i]);
            }
            hr = INameSpaceTreeControl2_GetControlStyle(pnstc2, 0xFFFFFFFF, &style);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            ok(style == 0, "Got style %x\n", style);

            /* Set/GetControlStyle2 */
            hr = INameSpaceTreeControl2_GetControlStyle2(pnstc2, 0xFFFFFFFF, &style2);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            ok(style2 == 0, "Got style %x\n", style2);

            hr = INameSpaceTreeControl2_SetControlStyle2(pnstc2, 0, 0);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            hr = INameSpaceTreeControl2_SetControlStyle2(pnstc2, 0, 0xFFFFFFFF);
            ok(hr == S_OK, "Got 0x%08x\n", hr);

            hr = INameSpaceTreeControl2_SetControlStyle2(pnstc2, 0xFFFFFFFF, 0);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            hr = INameSpaceTreeControl2_SetControlStyle2(pnstc2, 0xFFFFFFFF, 0xFFFFFFFF);
            ok(hr == S_OK, "Got 0x%08x\n", hr);

            hr = INameSpaceTreeControl2_SetControlStyle2(pnstc2, 0xFFFFFFFF, 0);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            hr = INameSpaceTreeControl2_GetControlStyle2(pnstc2, 0xFFFFFFFF, &style2);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            ok(style2 == 0x00000000, "Got style %x\n", style2);

            hr = INameSpaceTreeControl2_GetControlStyle2(pnstc2, 0xFFFF, &style2);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            ok(style2 == 0, "Got style %x\n", style2);

            tmp = 0;
            for(i = 0; styles2[i] != 0; i++)
            {
                hr = INameSpaceTreeControl2_SetControlStyle2(pnstc2, styles2[i], styles2[i]);
                ok(hr == S_OK, "Got 0x%08x (%x)\n", hr, styles2[i]);
                if(SUCCEEDED(hr)) tmp |= styles2[i];
            }

            hr = INameSpaceTreeControl2_GetControlStyle2(pnstc2, 0xFFFF, &style2);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            ok(style2 == tmp, "Got style %x (expected %x)\n", style2, tmp);
            if(SUCCEEDED(hr))
            {
                DWORD tmp2;
                for(i = 0; styles2[i] != 0; i++)
                {
                    hr = INameSpaceTreeControl2_GetControlStyle2(pnstc2, styles2[i], &tmp2);
                    ok(hr == S_OK, "Got 0x%08x\n", hr);
                    ok(tmp2 == (style2 & styles2[i]), "Got %x\n", tmp2);
                }
            }

            for(i = 0; styles2[i] != 0; i++)
            {
                hr = INameSpaceTreeControl2_SetControlStyle2(pnstc2, styles2[i], 0);
                ok(hr == S_OK, "Got 0x%08x (%x)\n", hr, styles2[i]);
            }
            hr = INameSpaceTreeControl2_GetControlStyle2(pnstc2, 0xFFFF, &style2);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            ok(style2 == 0, "Got style %x (expected 0)\n", style2);
        }
        else
        {
            /* 64-bit Windows Vista (others?) seems to have a somewhat
             * different idea of how the methods of this interface
             * should behave. */

            static const NSTCSTYLE styles[] =
                { NSTCS_HASEXPANDOS,NSTCS_HASLINES,NSTCS_SINGLECLICKEXPAND,
                  NSTCS_FULLROWSELECT,NSTCS_SPRINGEXPAND,NSTCS_HORIZONTALSCROLL,
                  NSTCS_RICHTOOLTIP, NSTCS_AUTOHSCROLL,
                  NSTCS_FADEINOUTEXPANDOS,
                  NSTCS_PARTIALCHECKBOXES,NSTCS_EXCLUSIONCHECKBOXES,
                  NSTCS_DIMMEDCHECKBOXES, NSTCS_NOINDENTCHECKS,
                  NSTCS_ROOTHASEXPANDO,NSTCS_SHOWSELECTIONALWAYS,NSTCS_NOINFOTIP,
                  NSTCS_EVENHEIGHT,NSTCS_NOREPLACEOPEN,NSTCS_DISABLEDRAGDROP,
                  NSTCS_NOORDERSTREAM,NSTCS_BORDER,NSTCS_NOEDITLABELS,
                  NSTCS_TABSTOP,NSTCS_FAVORITESMODE,NSTCS_EMPTYTEXT,NSTCS_CHECKBOXES,
                  NSTCS_ALLOWJUNCTIONS,NSTCS_SHOWTABSBUTTON,NSTCS_SHOWDELETEBUTTON,
                  NSTCS_SHOWREFRESHBUTTON, 0};
            trace("Detected broken INameSpaceTreeControl2.\n");

            style = 0xdeadbeef;
            hr = INameSpaceTreeControl2_GetControlStyle(pnstc2, 0xFFFFFFFF, &style);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            ok(style == 0xdeadbeef, "Got style %x\n", style);

            hr = INameSpaceTreeControl2_GetControlStyle2(pnstc2, 0xFFFFFFFF, &style2);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            ok(style2 == 0, "Got style %x\n", style2);

            tmp = 0;
            for(i = 0; styles[i] != 0; i++)
            {
                hr = INameSpaceTreeControl2_SetControlStyle(pnstc2, styles[i], styles[i]);
                ok(hr == E_FAIL || ((styles[i] & NSTCS_SPRINGEXPAND) && hr == S_OK),
                   "Got 0x%08x (%x)\n", hr, styles[i]);
                if(SUCCEEDED(hr)) tmp |= styles[i];
            }

            style = 0xdeadbeef;
            hr = INameSpaceTreeControl2_GetControlStyle(pnstc2, tmp, &style);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            ok(style == 0xdeadbeef, "Got style %x\n", style);

            tmp = 0;
            for(i = 0; styles2[i] != 0; i++)
            {
                hr = INameSpaceTreeControl2_SetControlStyle2(pnstc2, styles2[i], styles2[i]);
                ok(hr == S_OK, "Got 0x%08x (%x)\n", hr, styles2[i]);
                if(SUCCEEDED(hr)) tmp |= styles2[i];
            }

            style2 = 0xdeadbeef;
            hr = INameSpaceTreeControl2_GetControlStyle2(pnstc2, 0xFFFFFFFF, &style2);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            ok(style2 == tmp, "Got style %x\n", style2);

        }

        INameSpaceTreeControl2_Release(pnstc2);
    }
    else
    {
        skip("INameSpaceTreeControl2 missing.\n");
    }

    hr = INameSpaceTreeControl_RemoveRoot(pnstc, NULL);
    ok(hr == E_NOINTERFACE, "Got (0x%08x)\n", hr);

    /* Append / Insert root */
    if(0)
    {
        /* Crashes under Windows 7 */
        hr = INameSpaceTreeControl_AppendRoot(pnstc, NULL, SHCONTF_FOLDERS, 0, NULL);
        hr = INameSpaceTreeControl_InsertRoot(pnstc, 0, NULL, SHCONTF_FOLDERS, 0, NULL);
    }

    /* Note the usage of psidesktop and psidesktop2 */
    hr = INameSpaceTreeControl_AppendRoot(pnstc, psidesktop, SHCONTF_FOLDERS, 0, NULL);
    ok(hr == S_OK, "Got (0x%08x)\n", hr);
    hr = INameSpaceTreeControl_AppendRoot(pnstc, psidesktop, SHCONTF_FOLDERS, 0, NULL);
    ok(hr == S_OK, "Got (0x%08x)\n", hr);
    hr = INameSpaceTreeControl_AppendRoot(pnstc, psidesktop2, SHCONTF_FOLDERS, 0, NULL);
    ok(hr == S_OK, "Got (0x%08x)\n", hr);
    process_msgs();

    hr = INameSpaceTreeControl_RemoveRoot(pnstc, psidesktop);
    ok(hr == S_OK, "Got (0x%08x)\n", hr);
    hr = INameSpaceTreeControl_RemoveRoot(pnstc, psidesktop);
    ok(hr == S_OK, "Got (0x%08x)\n", hr);
    hr = INameSpaceTreeControl_RemoveRoot(pnstc, psidesktop);
    ok(hr == S_OK, "Got (0x%08x)\n", hr);

    hr = INameSpaceTreeControl_RemoveRoot(pnstc, psidesktop);
    ok(hr == E_FAIL, "Got (0x%08x)\n", hr);
    hr = INameSpaceTreeControl_RemoveAllRoots(pnstc);
    ok(hr == E_INVALIDARG, "Got (0x%08x)\n", hr);

    hr = INameSpaceTreeControl_AppendRoot(pnstc, psidesktop, SHCONTF_FOLDERS, 0, NULL);
    ok(hr == S_OK, "Got (0x%08x)\n", hr);
    hr = INameSpaceTreeControl_RemoveAllRoots(pnstc);
    ok(hr == S_OK, "Got (0x%08x)\n", hr);

    hr = INameSpaceTreeControl_InsertRoot(pnstc, 0, psidesktop, SHCONTF_FOLDERS, 0, NULL);
    ok(hr == S_OK, "Got (0x%08x)\n", hr);
    hr = INameSpaceTreeControl_InsertRoot(pnstc, -1, psidesktop, SHCONTF_FOLDERS, 0, NULL);
    ok(hr == S_OK, "Got (0x%08x)\n", hr);
    hr = INameSpaceTreeControl_InsertRoot(pnstc, -1, psidesktop, SHCONTF_FOLDERS, 0, NULL);
    ok(hr == S_OK, "Got (0x%08x)\n", hr);
    hr = INameSpaceTreeControl_InsertRoot(pnstc, 50, psidesktop, SHCONTF_FOLDERS, 0, NULL);
    ok(hr == S_OK, "Got (0x%08x)\n", hr);
    hr = INameSpaceTreeControl_InsertRoot(pnstc, 1, psidesktop, SHCONTF_FOLDERS, 0, NULL);
    ok(hr == S_OK, "Got (0x%08x)\n", hr);

    hr = INameSpaceTreeControl_RemoveAllRoots(pnstc);
    ok(hr == S_OK, "Got (0x%08x)\n", hr);

    IShellItem_Release(psidesktop);
    IShellItem_Release(psidesktop2);

    hr = INameSpaceTreeControl_QueryInterface(pnstc, &IID_IOleWindow, (void**)&pow);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    if(SUCCEEDED(hr))
    {
        HWND hwnd_nstc;
        hr = IOleWindow_GetWindow(pow, &hwnd_nstc);
        ok(hr == S_OK, "Got 0x%08x\n", hr);
        DestroyWindow(hwnd_nstc);
        IOleWindow_Release(pow);
    }

    res = INameSpaceTreeControl_Release(pnstc);
    ok(!res, "res was %d!\n", res);
}

static void test_events(void)
{
    INameSpaceTreeControl *pnstc;
    INameSpaceTreeControlEventsImpl *pnstceimpl, *pnstceimpl2;
    INameSpaceTreeControlEvents *pnstce, *pnstce2;
    IShellFolder *psfdesktop;
    IShellItem *psidesktop;
    IOleWindow *pow;
    LPITEMIDLIST pidl_desktop;
    DWORD cookie1, cookie2;
    HRESULT hr;
    UINT res;

    hr = CoCreateInstance(&CLSID_NamespaceTreeControl, NULL, CLSCTX_INPROC_SERVER,
                          &IID_INameSpaceTreeControl, (void**)&pnstc);
    ok(hr == S_OK, "Failed to initialize control (0x%08x)\n", hr);

    ok(pSHCreateShellItem != NULL, "No SHCreateShellItem.\n");
    ok(pSHGetIDListFromObject != NULL, "No SHCreateShellItem.\n");

    SHGetDesktopFolder(&psfdesktop);
    hr = pSHGetIDListFromObject((IUnknown*)psfdesktop, &pidl_desktop);
    IShellFolder_Release(psfdesktop);
    ok(hr == S_OK, "Got (0x%08x)\n", hr);
    hr = pSHCreateShellItem(NULL, NULL, pidl_desktop, &psidesktop);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    ILFree(pidl_desktop);

    /* Create two instances of INameSpaceTreeControlEvents */
    pnstceimpl = create_nstc_events();
    pnstce = (INameSpaceTreeControlEvents*)pnstceimpl;
    pnstceimpl2 = create_nstc_events();
    pnstce2 = (INameSpaceTreeControlEvents*)pnstceimpl2;

    if(0)
    {
        /* Crashes native */
        hr = INameSpaceTreeControl_TreeAdvise(pnstc, NULL, NULL);
        hr = INameSpaceTreeControl_TreeAdvise(pnstc, NULL, &cookie1);
        hr = INameSpaceTreeControl_TreeAdvise(pnstc, (IUnknown*)pnstce, NULL);
    }

    /* TreeAdvise in NameSpaceTreeController seems to support only one
     * client at the time.
     */

    /* First, respond with E_NOINTERFACE to all QI's */
    pnstceimpl->qi_enable_events = FALSE;
    pnstceimpl->qi_called_count = 0;
    cookie1 = 0xDEADBEEF;
    hr = INameSpaceTreeControl_TreeAdvise(pnstc, (IUnknown*)pnstce, &cookie1);
    ok(hr == E_FAIL, "Got (0x%08x)\n", hr);
    ok(cookie1 == 0, "cookie now (0x%08x)\n", cookie1);
    todo_wine
    {
        ok(pnstceimpl->qi_called_count == 7 || pnstceimpl->qi_called_count == 4 /* Vista */,
           "QueryInterface called %d times.\n",
           pnstceimpl->qi_called_count);
    }
    ok(pnstceimpl->ref == 1, "refcount was %d\n", pnstceimpl->ref);

    /* Accept query for IID_INameSpaceTreeControlEvents */
    pnstceimpl->qi_enable_events = TRUE;
    pnstceimpl->qi_called_count = 0;
    cookie1 = 0xDEADBEEF;
    hr = INameSpaceTreeControl_TreeAdvise(pnstc, (IUnknown*)pnstce, &cookie1);
    ok(hr == S_OK, "Got (0x%08x)\n", hr);
    ok(cookie1 == 1, "cookie now (0x%08x)\n", cookie1);
    todo_wine
    {
        ok(pnstceimpl->qi_called_count == 7 || pnstceimpl->qi_called_count == 4 /* Vista */,
           "QueryInterface called %d times.\n",
           pnstceimpl->qi_called_count);
    }
    ok(pnstceimpl->ref == 2, "refcount was %d\n", pnstceimpl->ref);

    /* A second time, query interface will not be called at all. */
    pnstceimpl->qi_enable_events = TRUE;
    pnstceimpl->qi_called_count = 0;
    cookie2 = 0xDEADBEEF;
    hr = INameSpaceTreeControl_TreeAdvise(pnstc, (IUnknown*)pnstce, &cookie2);
    ok(hr == E_FAIL, "Got (0x%08x)\n", hr);
    ok(cookie2 == 0, "cookie now (0x%08x)\n", cookie2);
    ok(!pnstceimpl->qi_called_count, "QueryInterface called %d times.\n",
       pnstceimpl->qi_called_count);
    ok(pnstceimpl->ref == 2, "refcount was %d\n", pnstceimpl->ref);

    /* Using another "instance" does not help. */
    pnstceimpl2->qi_enable_events = TRUE;
    pnstceimpl2->qi_called_count = 0;
    cookie2 = 0xDEADBEEF;
    hr = INameSpaceTreeControl_TreeAdvise(pnstc, (IUnknown*)pnstce2, &cookie2);
    ok(hr == E_FAIL, "Got (0x%08x)\n", hr);
    ok(cookie2 == 0, "cookie now (0x%08x)\n", cookie2);
    ok(!pnstceimpl2->qi_called_count, "QueryInterface called %d times.\n",
       pnstceimpl2->qi_called_count);
    ok(pnstceimpl2->ref == 1, "refcount was %d\n", pnstceimpl->ref);

    /* Unadvise with bogus cookie (will actually unadvise properly) */
    pnstceimpl->qi_enable_events = TRUE;
    pnstceimpl->qi_called_count = 0;
    hr = INameSpaceTreeControl_TreeUnadvise(pnstc, 1234);
    ok(hr == S_OK, "Got (0x%08x)\n", hr);
    ok(!pnstceimpl->qi_called_count, "QueryInterface called %d times.\n",
       pnstceimpl->qi_called_count);
    ok(pnstceimpl->ref == 1, "refcount was %d\n", pnstceimpl->ref);

    /* Unadvise "properly" (will have no additional effect) */
    pnstceimpl->qi_enable_events = TRUE;
    pnstceimpl->qi_called_count = 0;
    hr = INameSpaceTreeControl_TreeUnadvise(pnstc, cookie1);
    ok(hr == S_OK, "Got (0x%08x)\n", hr);
    ok(!pnstceimpl->qi_called_count, "QueryInterface called %d times.\n",
       pnstceimpl->qi_called_count);
    ok(pnstceimpl->ref == 1, "refcount was %d\n", pnstceimpl->ref);


    hr = INameSpaceTreeControl_QueryInterface(pnstc, &IID_IOleWindow, (void**)&pow);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    if(SUCCEEDED(hr))
    {
        HWND hwnd_nstc;
        hr = IOleWindow_GetWindow(pow, &hwnd_nstc);
        ok(hr == S_OK, "Got 0x%08x\n", hr);
        DestroyWindow(hwnd_nstc);
        IOleWindow_Release(pow);
    }

    res = INameSpaceTreeControl_Release(pnstc);
    ok(!res, "res was %d!\n", res);

    if(!res)
    {
        /* Freeing these prematurely causes a crash. */
        HeapFree(GetProcessHeap(), 0, pnstceimpl);
        HeapFree(GetProcessHeap(), 0, pnstceimpl2);
    }
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
    init_function_pointers();

    if(test_initialization())
    {
        test_basics();
        test_events();
    }
    else
    {
        win_skip("No NamespaceTreeControl (or instantiation failed).\n");
    }

    destroy_window();
    OleUninitialize();
}
