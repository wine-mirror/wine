/*
 * Copyright 2006 Jacek Caban for CodeWeavers
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

#define COBJMACROS

#include <wine/test.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "exdisp.h"
#include "htiframe.h"

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        expect_ ## func = FALSE; \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

DEFINE_EXPECT(GetContainer);
DEFINE_EXPECT(Site_GetWindow);
DEFINE_EXPECT(ShowObject);
DEFINE_EXPECT(CanInPlaceActivate);
DEFINE_EXPECT(OnInPlaceActivate);
DEFINE_EXPECT(OnUIActivate);
DEFINE_EXPECT(GetWindowContext);
DEFINE_EXPECT(Frame_GetWindow);
DEFINE_EXPECT(Frame_SetActiveObject);
DEFINE_EXPECT(UIWindow_SetActiveObject);
DEFINE_EXPECT(SetMenu);

static const WCHAR wszItem[] = {'i','t','e','m',0};

static HWND container_hwnd, shell_embedding_hwnd;

static HRESULT QueryInterface(REFIID,void**);

static HRESULT WINAPI OleContainer_QueryInterface(IOleContainer *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_ITargetContainer, riid))
        return E_NOINTERFACE; /* TODO */
    if(IsEqualGUID(&IID_IOleCommandTarget, riid))
        return E_NOINTERFACE; /* TODO */

    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI OleContainer_AddRef(IOleContainer *iface)
{
    return 2;
}

static ULONG WINAPI OleContainer_Release(IOleContainer *iface)
{
    return 1;
}

static HRESULT WINAPI OleContainer_ParseDisplayName(IOleContainer *iface, IBindCtx *pbc,
        LPOLESTR pszDiaplayName, ULONG *pchEaten, IMoniker **ppmkOut)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleContainer_EnumObjects(IOleContainer *iface, DWORD grfFlags,
        IEnumUnknown **ppenum)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleContainer_LockContainer(IOleContainer *iface, BOOL fLock)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IOleContainerVtbl OleContainerVtbl = {
    OleContainer_QueryInterface,
    OleContainer_AddRef,
    OleContainer_Release,
    OleContainer_ParseDisplayName,
    OleContainer_EnumObjects,
    OleContainer_LockContainer
};

static IOleContainer OleContainer = { &OleContainerVtbl };

static HRESULT WINAPI ClientSite_QueryInterface(IOleClientSite *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI ClientSite_AddRef(IOleClientSite *iface)
{
    return 2;
}

static ULONG WINAPI ClientSite_Release(IOleClientSite *iface)
{
    return 1;
}

static HRESULT WINAPI ClientSite_SaveObject(IOleClientSite *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_GetMoniker(IOleClientSite *iface, DWORD dwAsign, DWORD dwWhichMoniker,
        IMoniker **ppmon)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_GetContainer(IOleClientSite *iface, IOleContainer **ppContainer)
{
    CHECK_EXPECT(GetContainer);

    ok(ppContainer != NULL, "ppContainer == NULL\n");
    if(ppContainer)
        *ppContainer = &OleContainer;

    return S_OK;
}

static HRESULT WINAPI ClientSite_ShowObject(IOleClientSite *iface)
{
    CHECK_EXPECT(ShowObject);
    return S_OK;
}

static HRESULT WINAPI ClientSite_OnShowWindow(IOleClientSite *iface, BOOL fShow)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_RequestNewObjectLayout(IOleClientSite *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IOleClientSiteVtbl ClientSiteVtbl = {
    ClientSite_QueryInterface,
    ClientSite_AddRef,
    ClientSite_Release,
    ClientSite_SaveObject,
    ClientSite_GetMoniker,
    ClientSite_GetContainer,
    ClientSite_ShowObject,
    ClientSite_OnShowWindow,
    ClientSite_RequestNewObjectLayout
};

static HRESULT WINAPI InPlaceUIWindow_QueryInterface(IOleInPlaceFrame *iface,
                                                     REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI InPlaceUIWindow_AddRef(IOleInPlaceFrame *iface)
{
    return 2;
}

static ULONG WINAPI InPlaceUIWindow_Release(IOleInPlaceFrame *iface)
{
    return 1;
}

static HRESULT WINAPI InPlaceUIWindow_GetWindow(IOleInPlaceFrame *iface, HWND *phwnd)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_GetWindow(IOleInPlaceFrame *iface, HWND *phwnd)
{
    CHECK_EXPECT(Frame_GetWindow);
    ok(phwnd != NULL, "phwnd == NULL\n");
    if(phwnd)
        *phwnd = container_hwnd;
    return S_OK;
}

static HRESULT WINAPI InPlaceUIWindow_ContextSensitiveHelp(IOleInPlaceFrame *iface, BOOL fEnterMode)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceUIWindow_GetBorder(IOleInPlaceFrame *iface, LPRECT lprectBorder)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceUIWindow_RequestBorderSpace(IOleInPlaceFrame *iface,
        LPCBORDERWIDTHS pborderwidths)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceUIWindow_SetBorderSpace(IOleInPlaceFrame *iface,
        LPCBORDERWIDTHS pborderwidths)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceUIWindow_SetActiveObject(IOleInPlaceFrame *iface,
        IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    CHECK_EXPECT(UIWindow_SetActiveObject);
    ok(pActiveObject != NULL, "pActiveObject = NULL\n");
    ok(!lstrcmpW(pszObjName, wszItem), "unexpected pszObjName\n");
    return S_OK;
}

static HRESULT WINAPI InPlaceFrame_SetActiveObject(IOleInPlaceFrame *iface,
        IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    CHECK_EXPECT(Frame_SetActiveObject);
    ok(pActiveObject != NULL, "pActiveObject = NULL\n");
    ok(!lstrcmpW(pszObjName, wszItem), "unexpected pszObjName\n");
    return S_OK;
}

static HRESULT WINAPI InPlaceFrame_InsertMenus(IOleInPlaceFrame *iface, HMENU hmenuShared,
        LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetMenu(IOleInPlaceFrame *iface, HMENU hmenuShared,
        HOLEMENU holemenu, HWND hwndActiveObject)
{
    CHECK_EXPECT(SetMenu);
    ok(hmenuShared == NULL, "hmenuShared=%p\n", hmenuShared);
    ok(holemenu == NULL, "holemenu=%p\n", holemenu);
    ok(hwndActiveObject == shell_embedding_hwnd, "hwndActiveObject=%p, expected %p\n",
       hwndActiveObject, shell_embedding_hwnd);
    return S_OK;
}

static HRESULT WINAPI InPlaceFrame_RemoveMenus(IOleInPlaceFrame *iface, HMENU hmenuShared)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetStatusText(IOleInPlaceFrame *iface, LPCOLESTR pszStatusText)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_EnableModeless(IOleInPlaceFrame *iface, BOOL fEnable)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_TranslateAccelerator(IOleInPlaceFrame *iface, LPMSG lpmsg, WORD wID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IOleInPlaceFrameVtbl InPlaceUIWindowVtbl = {
    InPlaceUIWindow_QueryInterface,
    InPlaceUIWindow_AddRef,
    InPlaceUIWindow_Release,
    InPlaceUIWindow_GetWindow,
    InPlaceUIWindow_ContextSensitiveHelp,
    InPlaceUIWindow_GetBorder,
    InPlaceUIWindow_RequestBorderSpace,
    InPlaceUIWindow_SetBorderSpace,
    InPlaceUIWindow_SetActiveObject,
};

static IOleInPlaceUIWindow InPlaceUIWindow = { (IOleInPlaceUIWindowVtbl*)&InPlaceUIWindowVtbl };

static const IOleInPlaceFrameVtbl InPlaceFrameVtbl = {
    InPlaceUIWindow_QueryInterface,
    InPlaceUIWindow_AddRef,
    InPlaceUIWindow_Release,
    InPlaceFrame_GetWindow,
    InPlaceUIWindow_ContextSensitiveHelp,
    InPlaceUIWindow_GetBorder,
    InPlaceUIWindow_RequestBorderSpace,
    InPlaceUIWindow_SetBorderSpace,
    InPlaceFrame_SetActiveObject,
    InPlaceFrame_InsertMenus,
    InPlaceFrame_SetMenu,
    InPlaceFrame_RemoveMenus,
    InPlaceFrame_SetStatusText,
    InPlaceFrame_EnableModeless,
    InPlaceFrame_TranslateAccelerator
};

static IOleInPlaceFrame InPlaceFrame = { &InPlaceFrameVtbl };

static IOleClientSite ClientSite = { &ClientSiteVtbl };

static HRESULT WINAPI InPlaceSite_QueryInterface(IOleInPlaceSiteEx *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI InPlaceSite_AddRef(IOleInPlaceSiteEx *iface)
{
    return 2;
}

static ULONG WINAPI InPlaceSite_Release(IOleInPlaceSiteEx *iface)
{
    return 1;
}

static HRESULT WINAPI InPlaceSite_GetWindow(IOleInPlaceSiteEx *iface, HWND *phwnd)
{
    CHECK_EXPECT(Site_GetWindow);
    ok(phwnd != NULL, "phwnd == NULL\n");
    if(phwnd)
        *phwnd = container_hwnd;
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_ContextSensitiveHelp(IOleInPlaceSiteEx *iface, BOOL fEnterMode)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_CanInPlaceActivate(IOleInPlaceSiteEx *iface)
{
    CHECK_EXPECT(CanInPlaceActivate);
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_OnInPlaceActivate(IOleInPlaceSiteEx *iface)
{
    CHECK_EXPECT(OnInPlaceActivate);
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_OnUIActivate(IOleInPlaceSiteEx *iface)
{
    CHECK_EXPECT(OnUIActivate);
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_GetWindowContext(IOleInPlaceSiteEx *iface,
        IOleInPlaceFrame **ppFrame, IOleInPlaceUIWindow **ppDoc, LPRECT lprcPosRect,
        LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    static const RECT pos_rect = {2,1,1002,901};
    static const RECT clip_rect = {10,10,990,890};

    CHECK_EXPECT(GetWindowContext);

    ok(ppFrame != NULL, "ppFrame = NULL\n");
    if(ppFrame)
        *ppFrame = &InPlaceFrame;

    ok(ppDoc != NULL, "ppDoc = NULL\n");
    if(ppDoc)
        *ppDoc = &InPlaceUIWindow;

    ok(lprcPosRect != NULL, "lprcPosRect = NULL\n");
    if(lprcPosRect)
        memcpy(lprcPosRect, &pos_rect, sizeof(RECT));

    ok(lprcClipRect != NULL, "lprcClipRect = NULL\n");
    if(lprcClipRect)
        memcpy(lprcClipRect, &clip_rect, sizeof(RECT));

    ok(lpFrameInfo != NULL, "lpFrameInfo = NULL\n");
    if(lpFrameInfo) {
        lpFrameInfo->cb = sizeof(*lpFrameInfo);
        lpFrameInfo->fMDIApp = FALSE;
        lpFrameInfo->hwndFrame = container_hwnd;
        lpFrameInfo->haccel = NULL;
        lpFrameInfo->cAccelEntries = 0;
    }

    return S_OK;
}

static HRESULT WINAPI InPlaceSite_Scroll(IOleInPlaceSiteEx *iface, SIZE scrollExtant)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnUIDeactivate(IOleInPlaceSiteEx *iface, BOOL fUndoable)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnInPlaceDeactivate(IOleInPlaceSiteEx *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_DiscardUndoState(IOleInPlaceSiteEx *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_DeactivateAndUndo(IOleInPlaceSiteEx *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnPosRectChange(IOleInPlaceSiteEx *iface, LPCRECT lprcPosRect)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnInPlaceActivateEx(IOleInPlaceSiteEx *iface,
                                                      BOOL *pNoRedraw, DWORD dwFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnInPlaceDeactivateEx(IOleInPlaceSiteEx *iface,
                                                        BOOL fNoRedraw)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_RequestUIActivate(IOleInPlaceSiteEx *iface)
{
    ok(0, "unexpected call\n");
    return S_OK;
}

static const IOleInPlaceSiteExVtbl InPlaceSiteExVtbl = {
    InPlaceSite_QueryInterface,
    InPlaceSite_AddRef,
    InPlaceSite_Release,
    InPlaceSite_GetWindow,
    InPlaceSite_ContextSensitiveHelp,
    InPlaceSite_CanInPlaceActivate,
    InPlaceSite_OnInPlaceActivate,
    InPlaceSite_OnUIActivate,
    InPlaceSite_GetWindowContext,
    InPlaceSite_Scroll,
    InPlaceSite_OnUIDeactivate,
    InPlaceSite_OnInPlaceDeactivate,
    InPlaceSite_DiscardUndoState,
    InPlaceSite_DeactivateAndUndo,
    InPlaceSite_OnPosRectChange,
    InPlaceSite_OnInPlaceActivateEx,
    InPlaceSite_OnInPlaceDeactivateEx,
    InPlaceSite_RequestUIActivate
};

static IOleInPlaceSiteEx InPlaceSite = { &InPlaceSiteExVtbl };

static HRESULT QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)
            || IsEqualGUID(&IID_IOleClientSite, riid))
        *ppv = &ClientSite;
    else if(IsEqualGUID(&IID_IOleWindow, riid)
            || IsEqualGUID(&IID_IOleInPlaceSite, riid)
            || IsEqualGUID(&IID_IOleInPlaceSiteEx, riid))
        *ppv = &InPlaceSite;

    if(*ppv)
        return S_OK;

    return E_NOINTERFACE;
}

static LRESULT WINAPI wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static HWND create_container_window(void)
{
    static const WCHAR wszWebBrowserContainer[] =
        {'W','e','b','B','r','o','w','s','e','r','C','o','n','t','a','i','n','e','r',0};
    static WNDCLASSEXW wndclass = {
        sizeof(WNDCLASSEXW),
        0,
        wnd_proc,
        0, 0, NULL, NULL, NULL, NULL, NULL,
        wszWebBrowserContainer,
        NULL
    };

    RegisterClassExW(&wndclass);
    return CreateWindowW(wszWebBrowserContainer, wszWebBrowserContainer,
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, NULL, NULL, NULL, NULL);
}

static void test_DoVerb(IUnknown *unk)
{
    IOleObject *oleobj;
    RECT rect = {0,0,1000,1000};
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "QueryInterface(IID_OleObject) failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    SET_EXPECT(CanInPlaceActivate);
    SET_EXPECT(Site_GetWindow);
    SET_EXPECT(OnInPlaceActivate);
    SET_EXPECT(GetWindowContext);
    SET_EXPECT(ShowObject);
    SET_EXPECT(GetContainer);
    SET_EXPECT(Frame_GetWindow);
    SET_EXPECT(OnUIActivate);
    SET_EXPECT(Frame_SetActiveObject);
    SET_EXPECT(UIWindow_SetActiveObject);
    SET_EXPECT(SetMenu);

    hres = IOleObject_DoVerb(oleobj, OLEIVERB_SHOW, NULL, &ClientSite,
                             0, container_hwnd, &rect);
    ok(hres == S_OK, "DoVerb failed: %08lx\n", hres);

    CHECK_CALLED(CanInPlaceActivate);
    CHECK_CALLED(Site_GetWindow);
    CHECK_CALLED(OnInPlaceActivate);
    CHECK_CALLED(GetWindowContext);
    CHECK_CALLED(ShowObject);
    CHECK_CALLED(GetContainer);
    CHECK_CALLED(Frame_GetWindow);
    CHECK_CALLED(OnUIActivate);
    CHECK_CALLED(Frame_SetActiveObject);
    CHECK_CALLED(UIWindow_SetActiveObject);
    CHECK_CALLED(SetMenu);

    hres = IOleObject_DoVerb(oleobj, OLEIVERB_SHOW, NULL, &ClientSite,
                           0, container_hwnd, &rect);
    ok(hres == S_OK, "DoVerb failed: %08lx\n", hres);

    IOleObject_Release(oleobj);
}

static void test_GetMiscStatus(IOleObject *oleobj)
{
    DWORD st, i;
    HRESULT hres;

    for(i=0; i<10; i++) {
        st = 0xdeadbeef;
        hres = IOleObject_GetMiscStatus(oleobj, i, &st);
        ok(hres == S_OK, "GetMiscStatus failed: %08lx\n", hres);
        ok(st == (OLEMISC_SETCLIENTSITEFIRST|OLEMISC_ACTIVATEWHENVISIBLE|OLEMISC_INSIDEOUT
                  |OLEMISC_CANTLINKINSIDE|OLEMISC_RECOMPOSEONRESIZE),
           "st=%08lx, expected OLEMISC_SETCLIENTSITEFIRST|OLEMISC_ACTIVATEWHENVISIBLE|"
           "OLEMISC_INSIDEOUT|OLEMISC_CANTLINKINSIDE|OLEMISC_RECOMPOSEONRESIZE)\n", st);
    }
}

static void test_ClientSite(IUnknown *unk, IOleClientSite *client)
{
    IOleObject *oleobj;
    IOleInPlaceObject *inplace;
    HWND hwnd;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "QueryInterface(IID_OleObject) failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    test_GetMiscStatus(oleobj);

    hres = IUnknown_QueryInterface(unk, &IID_IOleInPlaceObject, (void**)&inplace);
    ok(hres == S_OK, "QueryInterface(IID_OleInPlaceObject) failed: %08lx\n", hres);
    if(FAILED(hres)) {
        IOleObject_Release(oleobj);
        return;
    }

    hres = IOleInPlaceObject_GetWindow(inplace, &hwnd);
    ok(hres == S_OK, "GetWindow failed: %08lx\n", hres);
    ok((hwnd == NULL) ^ (client == NULL), "unexpected hwnd %p\n", hwnd);

    if(client) {
        SET_EXPECT(GetContainer);
        SET_EXPECT(Site_GetWindow);
    }

    hres = IOleObject_SetClientSite(oleobj, client);
    ok(hres == S_OK, "SetClientSite failed: %08lx\n", hres);

    if(client) {
        CHECK_CALLED(GetContainer);
        CHECK_CALLED(Site_GetWindow);
    }

    hres = IOleInPlaceObject_GetWindow(inplace, &hwnd);
    ok(hres == S_OK, "GetWindow failed: %08lx\n", hres);
    ok((hwnd == NULL) == (client == NULL), "unexpected hwnd %p\n", hwnd);

    shell_embedding_hwnd = hwnd;

    IOleInPlaceObject_Release(inplace);
    IOleObject_Release(oleobj);
}

static void test_ClassInfo(IUnknown *unk)
{
    IProvideClassInfo2 *class_info;
    GUID guid;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IProvideClassInfo2, (void**)&class_info);
    ok(hres == S_OK, "QueryInterface(IID_IProvideClassInfo)  failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IProvideClassInfo2_GetGUID(class_info, GUIDKIND_DEFAULT_SOURCE_DISP_IID, &guid);
    ok(hres == S_OK, "GetGUID failed: %08lx\n", hres);
    ok(IsEqualGUID(&DIID_DWebBrowserEvents2, &guid), "wrong guid\n");

    hres = IProvideClassInfo2_GetGUID(class_info, 0, &guid);
    ok(hres == E_FAIL, "GetGUID failed: %08lx, expected E_FAIL\n", hres);
    ok(IsEqualGUID(&IID_NULL, &guid), "wrong guid\n");

    hres = IProvideClassInfo2_GetGUID(class_info, 2, &guid);
    ok(hres == E_FAIL, "GetGUID failed: %08lx, expected E_FAIL\n", hres);
    ok(IsEqualGUID(&IID_NULL, &guid), "wrong guid\n");

    hres = IProvideClassInfo2_GetGUID(class_info, GUIDKIND_DEFAULT_SOURCE_DISP_IID, NULL);
    ok(hres == E_POINTER, "GetGUID failed: %08lx, expected E_POINTER\n", hres);

    hres = IProvideClassInfo2_GetGUID(class_info, 0, NULL);
    ok(hres == E_POINTER, "GetGUID failed: %08lx, expected E_POINTER\n", hres);

    IProvideClassInfo2_Release(class_info);
}

static void test_ie_funcs(IUnknown *unk)
{
    IWebBrowser2 *wb;
    VARIANT_BOOL b;
    int i;
    long hwnd;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IWebBrowser2, (void**)&wb);
    ok(hres == S_OK, "Could not get IWebBrowser2 interface: %08lx\n", hres);
    if(FAILED(hres))
        return;

    /* HWND */

    hwnd = 0xdeadbeef;
    hres = IWebBrowser2_get_HWND(wb, &hwnd);
    ok(hres == E_FAIL, "get_HWND failed: %08lx, expected E_FAIL\n", hres);
    ok(hwnd == 0, "unexpected hwnd %lx\n", hwnd);

    /* MenuBar */

    hres = IWebBrowser2_get_MenuBar(wb, &b);
    ok(hres == S_OK, "get_MenuBar failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    hres = IWebBrowser2_put_MenuBar(wb, VARIANT_FALSE);
    ok(hres == S_OK, "put_MenuBar failed: %08lx\n", hres);

    hres = IWebBrowser2_get_MenuBar(wb, &b);
    ok(hres == S_OK, "get_MenuBar failed: %08lx\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    hres = IWebBrowser2_put_MenuBar(wb, 100);
    ok(hres == S_OK, "put_MenuBar failed: %08lx\n", hres);

    hres = IWebBrowser2_get_MenuBar(wb, &b);
    ok(hres == S_OK, "get_MenuBar failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    /* AddressBar */

    hres = IWebBrowser2_get_AddressBar(wb, &b);
    ok(hres == S_OK, "get_AddressBar failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    hres = IWebBrowser2_put_AddressBar(wb, VARIANT_FALSE);
    ok(hres == S_OK, "put_AddressBar failed: %08lx\n", hres);

    hres = IWebBrowser2_get_AddressBar(wb, &b);
    ok(hres == S_OK, "get_MenuBar failed: %08lx\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    hres = IWebBrowser2_put_AddressBar(wb, 100);
    ok(hres == S_OK, "put_AddressBar failed: %08lx\n", hres);

    hres = IWebBrowser2_get_AddressBar(wb, &b);
    ok(hres == S_OK, "get_AddressBar failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    hres = IWebBrowser2_put_AddressBar(wb, VARIANT_TRUE);
    ok(hres == S_OK, "put_MenuBar failed: %08lx\n", hres);

    /* StatusBar */

    hres = IWebBrowser2_get_StatusBar(wb, &b);
    ok(hres == S_OK, "get_StatusBar failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    hres = IWebBrowser2_put_StatusBar(wb, VARIANT_TRUE);
    ok(hres == S_OK, "put_StatusBar failed: %08lx\n", hres);

    hres = IWebBrowser2_get_StatusBar(wb, &b);
    ok(hres == S_OK, "get_StatusBar failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    hres = IWebBrowser2_put_StatusBar(wb, VARIANT_FALSE);
    ok(hres == S_OK, "put_StatusBar failed: %08lx\n", hres);

    hres = IWebBrowser2_get_StatusBar(wb, &b);
    ok(hres == S_OK, "get_StatusBar failed: %08lx\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    hres = IWebBrowser2_put_StatusBar(wb, 100);
    ok(hres == S_OK, "put_StatusBar failed: %08lx\n", hres);

    hres = IWebBrowser2_get_StatusBar(wb, &b);
    ok(hres == S_OK, "get_StatusBar failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    /* ToolBar */

    hres = IWebBrowser2_get_ToolBar(wb, &i);
    ok(hres == S_OK, "get_ToolBar failed: %08lx\n", hres);
    ok(i == VARIANT_TRUE, "i=%x\n", i);

    hres = IWebBrowser2_put_ToolBar(wb, VARIANT_FALSE);
    ok(hres == S_OK, "put_ToolBar failed: %08lx\n", hres);

    hres = IWebBrowser2_get_ToolBar(wb, &i);
    ok(hres == S_OK, "get_ToolBar failed: %08lx\n", hres);
    ok(i == VARIANT_FALSE, "b=%x\n", i);

    hres = IWebBrowser2_put_ToolBar(wb, 100);
    ok(hres == S_OK, "put_ToolBar failed: %08lx\n", hres);

    hres = IWebBrowser2_get_ToolBar(wb, &i);
    ok(hres == S_OK, "get_ToolBar failed: %08lx\n", hres);
    ok(i == VARIANT_TRUE, "i=%x\n", i);

    hres = IWebBrowser2_put_ToolBar(wb, VARIANT_TRUE);
    ok(hres == S_OK, "put_ToolBar failed: %08lx\n", hres);

    IWebBrowser2_Release(wb);
}

static void test_GetControlInfo(IUnknown *unk)
{
    IOleControl *control;
    CONTROLINFO info;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IOleControl, (void**)&control);
    ok(hres == S_OK, "Could not get IOleControl: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IOleControl_GetControlInfo(control, &info);
    ok(hres == E_NOTIMPL, "GetControlInfo failed: %08lx, exxpected E_NOTIMPL\n", hres);
    hres = IOleControl_GetControlInfo(control, NULL);
    ok(hres == E_NOTIMPL, "GetControlInfo failed: %08lx, exxpected E_NOTIMPL\n", hres);

    IOleControl_Release(control);
}

static void test_WebBrowser(void)
{
    IUnknown *unk = NULL;
    ULONG ref;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_WebBrowser, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "CoCreateInterface failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    test_ClassInfo(unk);
    test_ClientSite(unk, &ClientSite);
    test_DoVerb(unk);
    test_ClientSite(unk, NULL);
    test_ie_funcs(unk);
    test_GetControlInfo(unk);

    ref = IUnknown_Release(unk);
    ok(ref == 0, "ref=%ld, expected 0\n", ref);
}

START_TEST(webbrowser)
{
    container_hwnd = create_container_window();

    OleInitialize(NULL);

    test_WebBrowser();

    OleUninitialize();
}
