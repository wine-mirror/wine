/*
 * Copyright 2014 Jacek Caban for CodeWeavers
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

#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#include <initguid.h>
#include <windows.h>
#include <wmp.h>
#include <olectl.h>

#include "wine/test.h"

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

DEFINE_EXPECT(GetContainer);
DEFINE_EXPECT(GetExtendedControl);
DEFINE_EXPECT(GetWindow);
DEFINE_EXPECT(Invoke_USERMODE);

static HRESULT WINAPI OleContainer_QueryInterface(IOleContainer *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(riid, &IID_IUnknown)) {
        *ppv = iface;
    }else if(IsEqualGUID(riid, &IID_IOleContainer)) {
        *ppv = iface;
    }else {
        trace("OleContainer QI(%s)\n", wine_dbgstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    return S_OK;
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

static HRESULT cs_qi(REFIID,void**);

static HRESULT WINAPI ClientSite_QueryInterface(IOleClientSite *iface, REFIID riid, void **ppv)
{
    return cs_qi(riid, ppv);
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

static HRESULT WINAPI ClientSite_GetMoniker(IOleClientSite *iface, DWORD dwAssign, DWORD dwWhichMoniker,
        IMoniker **ppmon)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_GetContainer(IOleClientSite *iface, IOleContainer **ppContainer)
{
    CHECK_EXPECT2(GetContainer);

    *ppContainer = &OleContainer;
    return S_OK;
}

static HRESULT WINAPI ClientSite_ShowObject(IOleClientSite *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
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

static IOleClientSite ClientSite = { &ClientSiteVtbl };

static HRESULT WINAPI ServiceProvider_QueryInterface(IServiceProvider *iface,
                                                     REFIID riid, void **ppv)
{
    return cs_qi(riid, ppv);
}

static ULONG WINAPI ServiceProvider_AddRef(IServiceProvider *iface)
{
    return 2;
}

static ULONG WINAPI ServiceProvider_Release(IServiceProvider *iface)
{
    return 1;
}

static HRESULT WINAPI ServiceProvider_QueryService(IServiceProvider *iface, REFGUID guidService,
                                    REFIID riid, void **ppv)
{
    trace("QS(%s)\n", wine_dbgstr_guid(guidService));
    *ppv = NULL;
    return E_NOINTERFACE;
}

static const IServiceProviderVtbl ServiceProviderVtbl = {
    ServiceProvider_QueryInterface,
    ServiceProvider_AddRef,
    ServiceProvider_Release,
    ServiceProvider_QueryService
};

static IServiceProvider ServiceProvider = { &ServiceProviderVtbl };

static HRESULT WINAPI OleControlSite_QueryInterface(IOleControlSite *iface, REFIID riid, void **ppv)
{
    return cs_qi(riid, ppv);
}

static ULONG WINAPI OleControlSite_AddRef(IOleControlSite *iface)
{
    return 2;
}

static ULONG WINAPI OleControlSite_Release(IOleControlSite *iface)
{
    return 1;
}

static HRESULT WINAPI OleControlSite_OnControlInfoChanged(IOleControlSite *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleControlSite_LockInPlaceActive(IOleControlSite *iface, BOOL fLock)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleControlSite_GetExtendedControl(IOleControlSite *iface, IDispatch **ppDisp)
{
    CHECK_EXPECT(GetExtendedControl);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleControlSite_TransformCoords(IOleControlSite *iface, POINTL *pPtHimetric,
        POINTF *pPtfContainer, DWORD dwFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleControlSite_TranslateAccelerator(IOleControlSite *iface,
        MSG *pMsg, DWORD grfModifiers)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleControlSite_OnFocus(IOleControlSite *iface, BOOL fGotFocus)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleControlSite_ShowPropertyFrame(IOleControlSite *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IOleControlSiteVtbl OleControlSiteVtbl = {
    OleControlSite_QueryInterface,
    OleControlSite_AddRef,
    OleControlSite_Release,
    OleControlSite_OnControlInfoChanged,
    OleControlSite_LockInPlaceActive,
    OleControlSite_GetExtendedControl,
    OleControlSite_TransformCoords,
    OleControlSite_TranslateAccelerator,
    OleControlSite_OnFocus,
    OleControlSite_ShowPropertyFrame
};

static IOleControlSite OleControlSite = { &OleControlSiteVtbl };

static HRESULT WINAPI Dispatch_QueryInterface(IDispatch *iface, REFIID riid, void **ppv)
{
    return cs_qi(riid, ppv);
}

static ULONG WINAPI Dispatch_AddRef(IDispatch *iface)
{
    return 2;
}

static ULONG WINAPI Dispatch_Release(IDispatch *iface)
{
    return 1;
}

static HRESULT WINAPI Dispatch_GetTypeInfoCount(IDispatch *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Dispatch_GetTypeInfo(IDispatch *iface, UINT iTInfo, LCID lcid,
        ITypeInfo **ppTInfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Dispatch_GetIDsOfNames(IDispatch *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Dispatch_Invoke(IDispatch *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    switch(dispIdMember) {
    case DISPID_AMBIENT_USERMODE:
        CHECK_EXPECT(Invoke_USERMODE);
        break;
    default:
        ok(0, "unexpected call Invoke(%d)\n", dispIdMember);
    }

    return E_NOTIMPL;
}

static const IDispatchVtbl DispatchVtbl = {
    Dispatch_QueryInterface,
    Dispatch_AddRef,
    Dispatch_Release,
    Dispatch_GetTypeInfoCount,
    Dispatch_GetTypeInfo,
    Dispatch_GetIDsOfNames,
    Dispatch_Invoke
};

static IDispatch Dispatch = { &DispatchVtbl };

static HRESULT WINAPI InPlaceSiteWindowless_QueryInterface(IOleInPlaceSiteWindowless *iface, REFIID riid, void **ppv)
{
    return cs_qi(riid, ppv);
}

static ULONG WINAPI InPlaceSiteWindowless_AddRef(IOleInPlaceSiteWindowless *iface)
{
    return 2;
}

static ULONG WINAPI InPlaceSiteWindowless_Release(IOleInPlaceSiteWindowless *iface)
{
    return 1;
}

static HRESULT WINAPI InPlaceSiteWindowless_GetWindow(IOleInPlaceSiteWindowless *iface, HWND *phwnd)
{
    CHECK_EXPECT2(GetWindow);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_ContextSensitiveHelp(IOleInPlaceSiteWindowless *iface, BOOL fEnterMode)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_CanInPlaceActivate(IOleInPlaceSiteWindowless *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_OnInPlaceActivate(IOleInPlaceSiteWindowless *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_OnUIActivate(IOleInPlaceSiteWindowless *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_GetWindowContext(IOleInPlaceSiteWindowless *iface, IOleInPlaceFrame **ppFrame,
        IOleInPlaceUIWindow **ppDoc, LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_Scroll(
        IOleInPlaceSiteWindowless *iface, SIZE scrollExtent)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_OnUIDeactivate(
        IOleInPlaceSiteWindowless *iface, BOOL fUndoable)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_OnInPlaceDeactivate(
        IOleInPlaceSiteWindowless *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_DiscardUndoState(
        IOleInPlaceSiteWindowless *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_DeactivateAndUndo(
        IOleInPlaceSiteWindowless *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_OnPosRectChange(
        IOleInPlaceSiteWindowless *iface, LPCRECT lprcPosRect)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_OnInPlaceActivateEx(
        IOleInPlaceSiteWindowless *iface, BOOL *pfNoRedraw, DWORD dwFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_OnInPlaceDeactivateEx(
        IOleInPlaceSiteWindowless *iface, BOOL fNoRedraw)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_RequestUIActivate(
        IOleInPlaceSiteWindowless *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_CanWindowlessActivate(
        IOleInPlaceSiteWindowless *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_GetCapture(
        IOleInPlaceSiteWindowless *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_SetCapture(
        IOleInPlaceSiteWindowless *iface, BOOL fCapture)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_GetFocus(
        IOleInPlaceSiteWindowless *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_SetFocus(
        IOleInPlaceSiteWindowless *iface, BOOL fFocus)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_GetDC(
        IOleInPlaceSiteWindowless *iface, LPCRECT pRect,
        DWORD grfFlags, HDC *phDC)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_ReleaseDC(
        IOleInPlaceSiteWindowless *iface, HDC hDC)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_InvalidateRect(
        IOleInPlaceSiteWindowless *iface, LPCRECT pRect, BOOL fErase)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_InvalidateRgn(
        IOleInPlaceSiteWindowless *iface, HRGN hRGN, BOOL fErase)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_ScrollRect(
        IOleInPlaceSiteWindowless *iface, INT dx, INT dy,
        LPCRECT pRectScroll, LPCRECT pRectClip)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_AdjustRect(
        IOleInPlaceSiteWindowless *iface, LPRECT prc)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_OnDefWindowMessage(
        IOleInPlaceSiteWindowless *iface, UINT msg,
        WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IOleInPlaceSiteWindowlessVtbl InPlaceSiteWindowlessVtbl = {
    InPlaceSiteWindowless_QueryInterface,
    InPlaceSiteWindowless_AddRef,
    InPlaceSiteWindowless_Release,
    InPlaceSiteWindowless_GetWindow,
    InPlaceSiteWindowless_ContextSensitiveHelp,
    InPlaceSiteWindowless_CanInPlaceActivate,
    InPlaceSiteWindowless_OnInPlaceActivate,
    InPlaceSiteWindowless_OnUIActivate,
    InPlaceSiteWindowless_GetWindowContext,
    InPlaceSiteWindowless_Scroll,
    InPlaceSiteWindowless_OnUIDeactivate,
    InPlaceSiteWindowless_OnInPlaceDeactivate,
    InPlaceSiteWindowless_DiscardUndoState,
    InPlaceSiteWindowless_DeactivateAndUndo,
    InPlaceSiteWindowless_OnPosRectChange,
    InPlaceSiteWindowless_OnInPlaceActivateEx,
    InPlaceSiteWindowless_OnInPlaceDeactivateEx,
    InPlaceSiteWindowless_RequestUIActivate,
    InPlaceSiteWindowless_CanWindowlessActivate,
    InPlaceSiteWindowless_GetCapture,
    InPlaceSiteWindowless_SetCapture,
    InPlaceSiteWindowless_GetFocus,
    InPlaceSiteWindowless_SetFocus,
    InPlaceSiteWindowless_GetDC,
    InPlaceSiteWindowless_ReleaseDC,
    InPlaceSiteWindowless_InvalidateRect,
    InPlaceSiteWindowless_InvalidateRgn,
    InPlaceSiteWindowless_ScrollRect,
    InPlaceSiteWindowless_AdjustRect,
    InPlaceSiteWindowless_OnDefWindowMessage
};

static IOleInPlaceSiteWindowless InPlaceSiteWindowless = { &InPlaceSiteWindowlessVtbl };

static HRESULT cs_qi(REFIID riid, void **ppv)
{
    if(IsEqualGUID(riid, &IID_IUnknown)) {
        *ppv = &ClientSite;
    }else if(IsEqualGUID(riid, &IID_IOleClientSite)) {
        *ppv = &ClientSite;
    }else if(IsEqualGUID(riid, &IID_IServiceProvider)) {
        *ppv = &ServiceProvider;
    }else if(IsEqualGUID(riid, &IID_IOleControlSite)) {
        *ppv = &OleControlSite;
    }else if(IsEqualGUID(riid, &IID_IDispatch)) {
        *ppv = &Dispatch;
    }else if(IsEqualGUID(&IID_IOleWindow, riid)) {
        *ppv = &InPlaceSiteWindowless;
    }else if(IsEqualGUID(&IID_IOleInPlaceSite, riid)) {
        *ppv = &InPlaceSiteWindowless;
    }else if(IsEqualGUID(&IID_IOleInPlaceSiteEx, riid)) {
        *ppv = &InPlaceSiteWindowless;
    }else if(IsEqualGUID(&IID_IOleInPlaceSiteWindowless, riid)) {
        *ppv = &InPlaceSiteWindowless;
    }else {
        trace("QI(%s)\n", wine_dbgstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    return S_OK;
}

static void test_QI(IUnknown *unk)
{
    IUnknown *tmp;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IQuickActivate, (void**)&tmp);
    ok(hres == E_NOINTERFACE, "Got IQuickActivate iface when no expected\n");

    hres = IUnknown_QueryInterface(unk, &IID_IOleInPlaceObjectWindowless, (void**)&tmp);
    ok(hres == S_OK, "Could not get IOleInPlaceObjectWindowless iface: %08x\n", hres);
    IUnknown_Release(tmp);

    hres = IUnknown_QueryInterface(unk, &IID_IConnectionPointContainer, (void**)&tmp);
    ok(hres == S_OK, "Could not get IConnectionPointContainer iface: %08x\n", hres);
    IUnknown_Release(tmp);
}

static void test_wmp(void)
{
    IProvideClassInfo2 *class_info;
    IOleClientSite *client_site;
    IPersistStreamInit *psi;
    IOleObject *oleobj;
    DWORD misc_status;
    GUID guid;
    LONG ref;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_WindowsMediaPlayer, NULL, CLSCTX_INPROC_SERVER, &IID_IOleObject, (void**)&oleobj);
    if(hres == REGDB_E_CLASSNOTREG) {
        win_skip("CLSID_WindowsMediaPlayer not registered\n");
        return;
    }
    ok(hres == S_OK, "Coult not create CLSID_WindowsMediaPlayer instance: %08x\n", hres);

    hres = IOleObject_QueryInterface(oleobj, &IID_IProvideClassInfo2, (void**)&class_info);
    ok(hres == S_OK, "Could not get IProvideClassInfo2 iface: %08x\n", hres);

    hres = IProvideClassInfo2_GetGUID(class_info, GUIDKIND_DEFAULT_SOURCE_DISP_IID, &guid);
    ok(hres == S_OK, "GetGUID failed: %08x\n", hres);
    ok(IsEqualGUID(&guid, &IID__WMPOCXEvents), "guid = %s\n", wine_dbgstr_guid(&guid));

    IProvideClassInfo2_Release(class_info);

    test_QI((IUnknown*)oleobj);

    hres = IOleObject_GetMiscStatus(oleobj, DVASPECT_CONTENT, &misc_status);
    ok(hres == S_OK, "GetMiscStatus failed: %08x\n", hres);
    ok(misc_status == (OLEMISC_SETCLIENTSITEFIRST|OLEMISC_ACTIVATEWHENVISIBLE|OLEMISC_INSIDEOUT
                       |OLEMISC_CANTLINKINSIDE|OLEMISC_RECOMPOSEONRESIZE), "misc_status = %x\n", misc_status);

    hres = IOleObject_QueryInterface(oleobj, &IID_IPersistStreamInit, (void**)&psi);
    ok(hres == S_OK, "Could not get IPersistStreamInit iface: %08x\n", hres);

    IPersistStreamInit_Release(psi);

    SET_EXPECT(GetContainer);
    SET_EXPECT(GetExtendedControl);
    SET_EXPECT(GetWindow);
    SET_EXPECT(Invoke_USERMODE);
    hres = IOleObject_SetClientSite(oleobj, &ClientSite);
    ok(hres == S_OK, "SetClientSite failed: %08x\n", hres);
    todo_wine CHECK_CALLED(GetContainer);
    CHECK_CALLED(GetExtendedControl);
    todo_wine CHECK_CALLED(GetWindow);
    todo_wine CHECK_CALLED(Invoke_USERMODE);

    client_site = NULL;
    hres = IOleObject_GetClientSite(oleobj, &client_site);
    ok(hres == S_OK, "GetClientSite failed: %08x\n", hres);
    ok(client_site == &ClientSite, "client_site != ClientSite\n");

    hres = IOleObject_SetClientSite(oleobj, NULL);
    ok(hres == S_OK, "SetClientSite failed: %08x\n", hres);

    client_site = (void*)0xdeadbeef;
    hres = IOleObject_GetClientSite(oleobj, &client_site);
    ok(hres == E_FAIL, "GetClientSite failed: %08x\n", hres);
    ok(!client_site, "client_site = %p\n", client_site);

    ref = IOleObject_Release(oleobj);
    ok(!ref, "ref = %d\n", ref);
}

START_TEST(oleobj)
{
    CoInitialize(NULL);

    test_wmp();

    CoUninitialize();
}
