/*
 * Unit test of the IShellView
 *
 * Copyright 2010 Nikolay Sivov for CodeWeavers
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

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS
#define CONST_VTABLE

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "shellapi.h"

#include "shlguid.h"
#include "shlobj.h"
#include "shobjidl.h"
#include "shlwapi.h"
#include "ocidl.h"
#include "oleauto.h"

#include "wine/test.h"

#include "msg.h"

#define LISTVIEW_SEQ_INDEX  0
#define NUM_MSG_SEQUENCES   1

static struct msg_sequence *sequences[NUM_MSG_SEQUENCES];

static LRESULT WINAPI listview_subclass_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldproc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    static LONG defwndproc_counter = 0;
    LRESULT ret;
    struct message msg;

    trace("listview: %p, %04x, %08lx, %08lx\n", hwnd, message, wParam, lParam);

    msg.message = message;
    msg.flags = sent|wparam|lparam;
    if (defwndproc_counter) msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.lParam = lParam;
    add_message(sequences, LISTVIEW_SEQ_INDEX, &msg);

    defwndproc_counter++;
    ret = CallWindowProcA(oldproc, hwnd, message, wParam, lParam);
    defwndproc_counter--;
    return ret;
}

static HWND subclass_listview(HWND hwnd)
{
    WNDPROC oldproc;
    HWND listview;

    /* listview is a first child */
    listview = FindWindowExA(hwnd, NULL, WC_LISTVIEWA, NULL);

    oldproc = (WNDPROC)SetWindowLongPtrA(listview, GWLP_WNDPROC,
                                        (LONG_PTR)listview_subclass_proc);
    SetWindowLongPtrA(listview, GWLP_USERDATA, (LONG_PTR)oldproc);

    return listview;
}

typedef struct {

    const IShellBrowserVtbl *lpVtbl;
    LONG ref;
} IShellBrowserImpl;

/* dummy IShellBrowser implementation */
static const IShellBrowserVtbl IShellBrowserImpl_Vtbl;

IShellBrowser* IShellBrowserImpl_Construct(void)
{
    IShellBrowserImpl *browser;

    browser = HeapAlloc(GetProcessHeap(), 0, sizeof(*browser));
    browser->lpVtbl = &IShellBrowserImpl_Vtbl;
    browser->ref = 1;

    return (IShellBrowser*)browser;
}

static HRESULT WINAPI IShellBrowserImpl_QueryInterface(IShellBrowser *iface,
                                            REFIID riid,
                                            LPVOID *ppvObj)
{
    IShellBrowserImpl *This = (IShellBrowserImpl *)iface;

    *ppvObj = NULL;

    if(IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = This;
    }
    else if(IsEqualIID(riid, &IID_IOleWindow))
    {
        *ppvObj = This;
    }
    else if(IsEqualIID(riid, &IID_IShellBrowser))
    {
        *ppvObj = This;
    }

    if(*ppvObj)
    {
        IUnknown_AddRef( (IShellBrowser*) *ppvObj);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI IShellBrowserImpl_AddRef(IShellBrowser * iface)
{
    IShellBrowserImpl *This = (IShellBrowserImpl *)iface;
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IShellBrowserImpl_Release(IShellBrowser * iface)
{
    IShellBrowserImpl *This = (IShellBrowserImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
        return 0;
    }
    return ref;
}

static HRESULT WINAPI IShellBrowserImpl_GetWindow(IShellBrowser *iface,
                                           HWND *phwnd)
{
    if (phwnd) *phwnd = GetDesktopWindow();
    return S_OK;
}

static HRESULT WINAPI IShellBrowserImpl_ContextSensitiveHelp(IShellBrowser *iface,
                                                      BOOL fEnterMode)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowserImpl_BrowseObject(IShellBrowser *iface,
                                              LPCITEMIDLIST pidl,
                                              UINT wFlags)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowserImpl_EnableModelessSB(IShellBrowser *iface,
                                              BOOL fEnable)

{
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowserImpl_GetControlWindow(IShellBrowser *iface,
                                              UINT id,
                                              HWND *lphwnd)

{
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowserImpl_GetViewStateStream(IShellBrowser *iface,
                                                DWORD mode,
                                                LPSTREAM *pStrm)

{
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowserImpl_InsertMenusSB(IShellBrowser *iface,
                                           HMENU hmenuShared,
                                           LPOLEMENUGROUPWIDTHS lpMenuWidths)

{
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowserImpl_OnViewWindowActive(IShellBrowser *iface,
                                                IShellView *ppshv)

{
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowserImpl_QueryActiveShellView(IShellBrowser *iface,
                                                  IShellView **ppshv)

{
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowserImpl_RemoveMenusSB(IShellBrowser *iface,
                                           HMENU hmenuShared)

{
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowserImpl_SendControlMsg(IShellBrowser *iface,
                                            UINT id,
                                            UINT uMsg,
                                            WPARAM wParam,
                                            LPARAM lParam,
                                            LRESULT *pret)

{
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowserImpl_SetMenuSB(IShellBrowser *iface,
                                       HMENU hmenuShared,
                                       HOLEMENU holemenuReserved,
                                       HWND hwndActiveObject)

{
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowserImpl_SetStatusTextSB(IShellBrowser *iface,
                                             LPCOLESTR lpszStatusText)

{
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowserImpl_SetToolbarItems(IShellBrowser *iface,
                                             LPTBBUTTON lpButtons,
                                             UINT nButtons,
                                             UINT uFlags)

{
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellBrowserImpl_TranslateAcceleratorSB(IShellBrowser *iface,
                                                    LPMSG lpmsg,
                                                    WORD wID)

{
    return E_NOTIMPL;
}

static const IShellBrowserVtbl IShellBrowserImpl_Vtbl =
{
    IShellBrowserImpl_QueryInterface,
    IShellBrowserImpl_AddRef,
    IShellBrowserImpl_Release,
    IShellBrowserImpl_GetWindow,
    IShellBrowserImpl_ContextSensitiveHelp,
    IShellBrowserImpl_InsertMenusSB,
    IShellBrowserImpl_SetMenuSB,
    IShellBrowserImpl_RemoveMenusSB,
    IShellBrowserImpl_SetStatusTextSB,
    IShellBrowserImpl_EnableModelessSB,
    IShellBrowserImpl_TranslateAcceleratorSB,
    IShellBrowserImpl_BrowseObject,
    IShellBrowserImpl_GetViewStateStream,
    IShellBrowserImpl_GetControlWindow,
    IShellBrowserImpl_SendControlMsg,
    IShellBrowserImpl_QueryActiveShellView,
    IShellBrowserImpl_OnViewWindowActive,
    IShellBrowserImpl_SetToolbarItems
};

static const struct message empty_seq[] = {
    { 0 }
};

static const struct message folderview_getspacing_seq[] = {
    { LVM_GETITEMSPACING, wparam|sent, FALSE },
    { 0 }
};

static const struct message folderview_getselectionmarked_seq[] = {
    { LVM_GETSELECTIONMARK, sent },
    { 0 }
};

static const struct message folderview_getfocused_seq[] = {
    { LVM_GETNEXTITEM, sent|wparam|lparam, -1, LVNI_FOCUSED },
    { 0 }
};

static void test_IShellView_CreateViewWindow(void)
{
    IShellFolder *desktop;
    FOLDERSETTINGS settings;
    IShellView *view;
    HWND hwnd_view;
    HRESULT hr;
    RECT r = {0};

    hr = SHGetDesktopFolder(&desktop);
    ok(hr == S_OK, "got (0x%08x)\n", hr);

    hr = IShellFolder_CreateViewObject(desktop, NULL, &IID_IShellView, (void**)&view);
    ok(hr == S_OK, "got (0x%08x)\n", hr);

if (0)
{
    /* crashes on native */
    hr = IShellView_CreateViewWindow(view, NULL, &settings, NULL, NULL, NULL);
}

    settings.ViewMode = FVM_ICON;
    settings.fFlags = 0;
    hwnd_view = (HWND)0xdeadbeef;
    hr = IShellView_CreateViewWindow(view, NULL, &settings, NULL, NULL, &hwnd_view);
    ok(hr == E_UNEXPECTED, "got (0x%08x)\n", hr);
    ok(hwnd_view == 0, "got %p\n", hwnd_view);

    hwnd_view = (HWND)0xdeadbeef;
    hr = IShellView_CreateViewWindow(view, NULL, &settings, NULL, &r, &hwnd_view);
    ok(hr == E_UNEXPECTED, "got (0x%08x)\n", hr);
    ok(hwnd_view == 0, "got %p\n", hwnd_view);

    IShellView_Release(view);
    IShellFolder_Release(desktop);
}

static void test_IFolderView(void)
{
    IShellFolder *desktop, *folder;
    FOLDERSETTINGS settings;
    IShellView *view;
    IShellBrowser *browser;
    IFolderView *fv;
    HWND hwnd_view, hwnd_list;
    HRESULT hr;
    INT ret;
    POINT pt;
    LONG ref1, ref2;
    RECT r;

    hr = SHGetDesktopFolder(&desktop);
    ok(hr == S_OK, "got (0x%08x)\n", hr);

    hr = IShellFolder_CreateViewObject(desktop, NULL, &IID_IShellView, (void**)&view);
    ok(hr == S_OK, "got (0x%08x)\n", hr);

    hr = IShellView_QueryInterface(view, &IID_IFolderView, (void**)&fv);
    if (hr != S_OK)
    {
        win_skip("IFolderView not supported by desktop folder\n");
        IShellView_Release(view);
        IShellFolder_Release(desktop);
        return;
    }

    /* call methods before window creation */
    hr = IFolderView_GetSpacing(fv, NULL);
    ok(hr == S_FALSE || broken(hr == S_OK) /* win7 */, "got (0x%08x)\n", hr);

if (0)
{
    /* crashes on Vista and Win2k8 - List not created yet case */
    hr = IFolderView_GetSpacing(fv, &pt);

    /* crashes on XP */
    hr = IFolderView_GetSelectionMarkedItem(fv, NULL);
    hr = IFolderView_GetFocusedItem(fv, NULL);
}

    browser = IShellBrowserImpl_Construct();

    settings.ViewMode = FVM_ICON;
    settings.fFlags = 0;
    hwnd_view = (HWND)0xdeadbeef;
    r.left = r.top = 0;
    r.right = r.bottom = 100;
    hr = IShellView_CreateViewWindow(view, NULL, &settings, browser, &r, &hwnd_view);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    ok(IsWindow(hwnd_view), "got %p\n", hwnd_view);

    hwnd_list = subclass_listview(hwnd_view);
    if (!hwnd_list)
    {
        win_skip("Failed to subclass ListView control\n");
        IShellBrowser_Release(browser);
        IFolderView_Release(fv);
        IShellView_Release(view);
        IShellFolder_Release(desktop);
        return;
    }

    /* IFolderView::GetSpacing */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    hr = IFolderView_GetSpacing(fv, NULL);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, empty_seq, "IFolderView::GetSpacing, empty", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    hr = IFolderView_GetSpacing(fv, &pt);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    /* fails with empty sequence on win7 for unknown reason */
    if (sequences[LISTVIEW_SEQ_INDEX]->count)
    {
        ok_sequence(sequences, LISTVIEW_SEQ_INDEX, folderview_getspacing_seq, "IFolderView::GetSpacing", FALSE);
        ok(pt.x > 0, "got %d\n", pt.x);
        ok(pt.y > 0, "got %d\n", pt.y);
        ret = SendMessageA(hwnd_list, LVM_GETITEMSPACING, 0, 0);
        ok(pt.x == LOWORD(ret) && pt.y == HIWORD(ret), "got (%d, %d)\n", LOWORD(ret), HIWORD(ret));
    }

    /* IFolderView::GetSelectionMarkedItem */
if (0)
{
    /* crashes on XP */
    hr = IFolderView_GetSelectionMarkedItem(fv, NULL);
}

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    hr = IFolderView_GetSelectionMarkedItem(fv, &ret);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, folderview_getselectionmarked_seq,
                                  "IFolderView::GetSelectionMarkedItem", FALSE);

    /* IFolderView::GetFocusedItem */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    hr = IFolderView_GetFocusedItem(fv, &ret);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, folderview_getfocused_seq,
                                  "IFolderView::GetFocusedItem", FALSE);

    /* IFolderView::GetFolder, just return pointer */
if (0)
{
    /* crashes on XP */
    hr = IFolderView_GetFolder(fv, NULL, (void**)&folder);
    hr = IFolderView_GetFolder(fv, NULL, NULL);
}

    hr = IFolderView_GetFolder(fv, &IID_IShellFolder, NULL);
    ok(hr == E_POINTER, "got (0x%08x)\n", hr);

    ref1 = IShellFolder_AddRef(desktop);
    IShellFolder_Release(desktop);
    hr = IFolderView_GetFolder(fv, &IID_IShellFolder, (void**)&folder);
    ok(hr == S_OK, "got (0x%08x)\n", hr);
    ref2 = IShellFolder_AddRef(desktop);
    IShellFolder_Release(desktop);
    ok(ref1 == ref2, "expected same refcount, got %d\n", ref2);
    ok(desktop == folder, "\n");

    IShellBrowser_Release(browser);
    IFolderView_Release(fv);
    IShellView_Release(view);
    IShellFolder_Release(desktop);
}

START_TEST(shlview)
{
    OleInitialize(NULL);

    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);

    test_IShellView_CreateViewWindow();
    test_IFolderView();

    OleUninitialize();
}
