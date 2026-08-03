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
#include <stdarg.h>
#include <initguid.h>
#include <windef.h>
#include <winbase.h>
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

#define CHECK_CALLED_OR_BROKEN(func) \
    do { \
        ok(called_ ## func || broken(1), "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

DEFINE_EXPECT(GetContainer);
DEFINE_EXPECT(GetExtendedControl);
DEFINE_EXPECT(GetWindow);
DEFINE_EXPECT(Invoke_USERMODE);
DEFINE_EXPECT(CanWindowlessActivate);
DEFINE_EXPECT(OnInPlaceActivateEx);
DEFINE_EXPECT(OnInPlaceDeactivate);
DEFINE_EXPECT(GetWindowContext);
DEFINE_EXPECT(ShowObject);
DEFINE_EXPECT(OnShowWindow_FALSE);

static const WCHAR mp3file[] = L"test.mp3";
static inline WCHAR *load_resource(const WCHAR *name)
{
    static WCHAR pathW[MAX_PATH];
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    GetTempPathW(ARRAY_SIZE(pathW), pathW);
    lstrcatW(pathW, name);

    file = CreateFileW(pathW, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "file creation failed, at %s, error %ld\n", wine_dbgstr_w(pathW),
        GetLastError());

    res = FindResourceW(NULL, name, (LPCWSTR)RT_RCDATA);
    ok( res != 0, "couldn't find resource\n" );
    ptr = LockResource( LoadResource( GetModuleHandleA(NULL), res ));
    WriteFile( file, ptr, SizeofResource( GetModuleHandleA(NULL), res ), &written, NULL );
    ok( written == SizeofResource( GetModuleHandleA(NULL), res ), "couldn't write resource\n" );
    CloseHandle( file );

    return pathW;
}

static HWND container_hwnd;

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
        LPOLESTR pszDisplayName, ULONG *pchEaten, IMoniker **ppmkOut)
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

static HRESULT WINAPI InPlaceFrame_QueryInterface(IOleInPlaceFrame *iface, REFIID riid, void **ppv)
{
    ok(0, "Unexpected QI(%s)\n", wine_dbgstr_guid(riid));
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI InPlaceFrame_AddRef(IOleInPlaceFrame *iface)
{
    return 2;
}

static ULONG WINAPI InPlaceFrame_Release(IOleInPlaceFrame *iface)
{
    return 1;
}

static HRESULT WINAPI InPlaceFrame_GetWindow(IOleInPlaceFrame *iface, HWND *phwnd)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_ContextSensitiveHelp(IOleInPlaceFrame *iface, BOOL fEnterMode)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_GetBorder(IOleInPlaceFrame *iface, LPRECT lprectBorder)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_RequestBorderSpace(IOleInPlaceFrame *iface,
        LPCBORDERWIDTHS pborderwidths)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetBorderSpace(IOleInPlaceFrame *iface,
        LPCBORDERWIDTHS pborderwidths)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceUIWindow_SetActiveObject(IOleInPlaceFrame *iface,
        IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetActiveObject(IOleInPlaceFrame *iface,
        IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
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
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
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

static const IOleInPlaceFrameVtbl InPlaceFrameVtbl = {
    InPlaceFrame_QueryInterface,
    InPlaceFrame_AddRef,
    InPlaceFrame_Release,
    InPlaceFrame_GetWindow,
    InPlaceFrame_ContextSensitiveHelp,
    InPlaceFrame_GetBorder,
    InPlaceFrame_RequestBorderSpace,
    InPlaceFrame_SetBorderSpace,
    InPlaceFrame_SetActiveObject,
    InPlaceFrame_InsertMenus,
    InPlaceFrame_SetMenu,
    InPlaceFrame_RemoveMenus,
    InPlaceFrame_SetStatusText,
    InPlaceFrame_EnableModeless,
    InPlaceFrame_TranslateAccelerator
};

static IOleInPlaceFrame InPlaceFrame = { &InPlaceFrameVtbl };

static const IOleInPlaceFrameVtbl InPlaceUIWindowVtbl = {
    InPlaceFrame_QueryInterface,
    InPlaceFrame_AddRef,
    InPlaceFrame_Release,
    InPlaceFrame_GetWindow,
    InPlaceFrame_ContextSensitiveHelp,
    InPlaceFrame_GetBorder,
    InPlaceFrame_RequestBorderSpace,
    InPlaceFrame_SetBorderSpace,
    InPlaceUIWindow_SetActiveObject,
};

static IOleInPlaceFrame InPlaceUIWindow = { &InPlaceUIWindowVtbl };

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
    CHECK_EXPECT(ShowObject);
    return S_OK;
}

static HRESULT WINAPI ClientSite_OnShowWindow(IOleClientSite *iface, BOOL fShow)
{
    if(fShow)
        ok(0, "unexpected call\n");
    else
        CHECK_EXPECT(OnShowWindow_FALSE);
    return S_OK;
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
        ok(0, "unexpected call Invoke(%ld)\n", dispIdMember);
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

static HRESULT WINAPI WMPOCXEvents_QueryInterface(IDispatch *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID__WMPOCXEvents, riid) || IsEqualGUID(&IID_IDispatch, riid)) {
        *ppv = iface;
        return S_OK;
    }

    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static HRESULT WINAPI WMPOCXEvents_Invoke(IDispatch *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    switch(dispIdMember) {
    default:
        ok(0, "unexpected call Invoke(%ld)\n", dispIdMember);
    }

    return E_NOTIMPL;
}

static IDispatchVtbl WMPOcxEventsVtbl = {
    WMPOCXEvents_QueryInterface,
    Dispatch_AddRef,
    Dispatch_Release,
    Dispatch_GetTypeInfoCount,
    Dispatch_GetTypeInfo,
    Dispatch_GetIDsOfNames,
    WMPOCXEvents_Invoke,
};

static IDispatch WMPOCXEvents = { &WMPOcxEventsVtbl };

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
    *phwnd = container_hwnd;
    return S_OK;
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
    static const RECT rect = {0,0,400,410};

    CHECK_EXPECT(GetWindowContext);

    ok(ppFrame != NULL, "ppFrame = NULL\n");
    if(ppFrame)
        *ppFrame = &InPlaceFrame;
    ok(ppDoc != NULL, "ppDoc = NULL\n");
    if(ppDoc)
        *ppDoc = (IOleInPlaceUIWindow*)&InPlaceUIWindow;
    ok(lprcPosRect != NULL, "lprcPosRect = NULL\n");
    if(lprcPosRect)
        memcpy(lprcPosRect, &rect, sizeof(RECT));
    ok(lprcClipRect != NULL, "lprcClipRect = NULL\n");
    if(lprcClipRect)
        memcpy(lprcClipRect, &rect, sizeof(RECT));
    ok(lpFrameInfo != NULL, "lpFrameInfo = NULL\n");
    if(lpFrameInfo) {
        ok(lpFrameInfo->cb == sizeof(*lpFrameInfo), "lpFrameInfo->cb = %u, expected %u\n", lpFrameInfo->cb, (unsigned)sizeof(*lpFrameInfo));
        lpFrameInfo->fMDIApp = FALSE;
        lpFrameInfo->hwndFrame = container_hwnd;
        lpFrameInfo->haccel = NULL;
        lpFrameInfo->cAccelEntries = 0;
    }

    return S_OK;
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
    CHECK_EXPECT(OnInPlaceDeactivate);
    return S_OK;
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
    CHECK_EXPECT(OnInPlaceActivateEx);
    ok(!dwFlags, "dwFlags = %lx\n", dwFlags);
    ok(pfNoRedraw != NULL, "pfNoRedraw = NULL\n");
    return S_OK;
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
    CHECK_EXPECT(CanWindowlessActivate);
    return S_FALSE;
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

static void test_ConnectionPoint(IOleObject *unk)
{
    IConnectionPointContainer *container;
    IConnectionPoint *point;
    HRESULT hres;

    static DWORD dw = 100;

    hres = IOleObject_QueryInterface(unk, &IID_IConnectionPointContainer, (void**)&container);
    ok(hres == S_OK, "QueryInterface(IID_IConnectionPointContainer) failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IConnectionPointContainer_FindConnectionPoint(container, &IID__WMPOCXEvents, &point);
    IConnectionPointContainer_Release(container);
    ok(hres == S_OK, "FindConnectionPoint failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IConnectionPoint_Advise(point, (IUnknown*)&WMPOCXEvents, &dw);
    ok(hres == S_OK, "Advise failed: %08lx\n", hres);
    ok(dw == 1, "dw=%ld, expected 1\n", dw);
    hres = IConnectionPoint_Unadvise(point, dw);
    ok(hres == S_OK, "Unadvise failed: %08lx\n", hres);

    IConnectionPoint_Release(point);
}

static void test_wmp_ifaces(IOleObject *oleobj)
{
    IWMPSettings *settings, *settings_qi;
    IWMPPlayer4 *player4;
    IWMPPlayer *player;
    IWMPPlaylist *playlist;
    IWMPMedia *media;
    IWMPControls *controls;
    VARIANT_BOOL vbool;
    IWMPNetwork *network;
    IUnknown *unk;
    HRESULT hres;
    BSTR filename;
    BSTR url;

    hres = IOleObject_QueryInterface(oleobj, &IID_IWMPPlayer4, (void**)&player4);
    ok(hres == S_OK, "Could not get IWMPPlayer4 iface: %08lx\n", hres);

    controls = NULL;
    hres = IWMPPlayer4_get_controls(player4, &controls);
    ok(hres == S_OK, "get_controls failed: %08lx\n", hres);
    ok(controls != NULL, "controls = NULL\n");

    player = NULL;
    hres = IWMPControls_QueryInterface(controls, &IID_IWMPPlayer, (void**)&player);
    ok(hres == E_NOINTERFACE, "Getting IWMPPlayer from IWMPControls SUCCEEDED\n");
    ok(player == NULL, "player != NULL\n");

    unk = NULL;
    hres = IWMPControls_QueryInterface(controls, &IID_IUnknown, (void **)&unk);
    ok(hres == S_OK, "Failed to get IUnknown, hr %#lx.\n", hres);
    ok(unk != NULL, "Unexpected interface pointer.\n");
    IUnknown_Release(unk);

    IWMPControls_Release(controls);

    /* IWPNetwork */
    network = NULL;
    hres = IWMPPlayer4_get_network(player4, &network);
    ok(hres == S_OK, "get_network failed: %08lx\n", hres);
    ok(network != NULL, "network = NULL\n");

    player = NULL;
    hres = IWMPNetwork_QueryInterface(network, &IID_IWMPPlayer, (void**)&player);
    ok(hres != S_OK, "Getting IWMPPlayer from IWMPNetwork SUCCEEDED\n");
    ok(player == NULL, "player != NULL\n");

    IWMPNetwork_Release(network);

    playlist = NULL;
    hres = IWMPPlayer4_QueryInterface(player4, &IID_IWMPPlaylist, (void**)&playlist);
    ok(hres == E_NOINTERFACE, "Getting IWMPPlaylist from IWMPPlayer4 succeeded: %08lx\n", hres);
    ok(playlist == NULL, "playlist != NULL\n");

    playlist = NULL;
    hres = IWMPPlayer4_get_currentPlaylist(player4, &playlist);
    ok(hres == S_OK, "IWMPPlayer4_get_currentPlaylist failed: %08lx\n", hres);
    ok(playlist != NULL, "playlist != NULL\n");

    IWMPPlaylist_Release(playlist);

    media = NULL;
    hres = IWMPPlayer4_QueryInterface(player4, &IID_IWMPMedia, (void**)&media);
    ok(hres == E_NOINTERFACE, "get_currentMedia SUCCEEDED: %08lx\n", hres);
    ok(media == NULL, "media != NULL\n");

    /* Test media put/get */
    media = NULL;
    hres = IWMPPlayer4_get_currentMedia(player4, &media);
    ok(hres == S_FALSE, "get_currentMedia SUCCEEDED\n");
    ok(media == NULL, "media != NULL\n");

    filename = SysAllocString(load_resource(mp3file));

    SET_EXPECT(GetContainer);
    SET_EXPECT(Invoke_USERMODE);
    hres = IWMPPlayer4_put_URL(player4, filename);
    ok(hres == S_OK, "IWMPPlayer4_put_URL failed: %08lx\n", hres);
    todo_wine CHECK_CALLED_OR_BROKEN(GetContainer);
    todo_wine CHECK_CALLED(Invoke_USERMODE);

    url = NULL;
    SET_EXPECT(Invoke_USERMODE);
    hres = IWMPPlayer4_get_URL(player4, &url);
    ok(hres == S_OK, "IWMPPlayer4_get_URL failed: %08lx\n", hres);
    ok(0 == lstrcmpW(url, filename), "%s != %s\n", wine_dbgstr_w(url), wine_dbgstr_w(filename));
    todo_wine CHECK_CALLED(Invoke_USERMODE);
    SysFreeString(url);

    hres = IWMPPlayer4_get_currentMedia(player4, &media);
    ok(hres == S_OK, "get_currentMedia failed: %08lx\n", hres);
    ok(media != NULL, "media = (%p)\n", media);

    url = NULL;
    hres = IWMPMedia_get_sourceURL(media, &url);
    ok(hres == S_OK, "IWMPMedia_get_sourceURL failed: %08lx\n", hres);
    ok(0 == lstrcmpW(url, filename), "%s != %s\n", wine_dbgstr_w(url), wine_dbgstr_w(filename));
    SysFreeString(url);

    SET_EXPECT(GetContainer);
    hres = IWMPPlayer4_put_currentMedia(player4, media);
    ok(hres == S_OK, "put_currentMedia failed: %08lx\n", hres);
    todo_wine CHECK_CALLED_OR_BROKEN(GetContainer);

    IWMPMedia_Release(media);

    hres = IWMPPlayer4_get_currentMedia(player4, &media);
    ok(hres == S_OK, "get_currentMedia failed: %08lx\n", hres);
    ok(media != NULL, "media = (%p)\n", media);

    IWMPMedia_Release(media);

    settings = NULL;
    hres = IWMPPlayer4_get_settings(player4, &settings);
    ok(hres == S_OK, "get_settings failed: %08lx\n", hres);
    ok(settings != NULL, "settings = NULL\n");

    hres = IOleObject_QueryInterface(oleobj, &IID_IWMPSettings, (void**)&settings_qi);
    ok(hres == S_OK, "Could not get IWMPSettings iface: %08lx\n", hres);
    ok(settings == settings_qi, "settings != settings_qi\n");
    IWMPSettings_Release(settings_qi);

    /* Test few settings put/gets */
    hres = IWMPSettings_get_autoStart(settings, &vbool);
    ok(hres == S_OK, "Could not get autoStart from IWMPSettings: %08lx\n", hres);
    ok(vbool == VARIANT_TRUE, "autoStart = %x\n", vbool);
    hres = IWMPSettings_put_autoStart(settings, VARIANT_FALSE);
    ok(hres == S_OK, "Could not put autoStart in IWMPSettings: %08lx\n", hres);
    hres = IWMPSettings_get_autoStart(settings, &vbool);
    ok(hres == S_OK, "Could not get autoStart from IWMPSettings: %08lx\n", hres);
    ok(!vbool, "autoStart = %x\n", vbool);

    hres = IWMPSettings_get_invokeURLs(settings, &vbool);
    ok(hres == S_OK, "Could not get invokeURLs from IWMPSettings: %08lx\n", hres);
    ok(vbool == VARIANT_TRUE, "invokeURLs = %x\n", vbool);
    hres = IWMPSettings_put_invokeURLs(settings, VARIANT_FALSE);
    ok(hres == S_OK, "Could not put invokeURLs in IWMPSettings: %08lx\n", hres);
    hres = IWMPSettings_get_invokeURLs(settings, &vbool);
    ok(hres == S_OK, "Could not get invokeURLs from IWMPSettings: %08lx\n", hres);
    ok(!vbool, "invokeURLs = %x\n", vbool);

    hres = IWMPSettings_get_enableErrorDialogs(settings, &vbool);
    ok(hres == S_OK, "Could not get enableErrorDialogs from IWMPSettings: %08lx\n", hres);
    ok(vbool == VARIANT_FALSE, "enableErrorDialogs = %x\n", vbool);
    hres = IWMPSettings_put_enableErrorDialogs(settings, VARIANT_TRUE);
    ok(hres == S_OK, "Could not put enableErrorDialogs in IWMPSettings: %08lx\n", hres);
    hres = IWMPSettings_get_enableErrorDialogs(settings, &vbool);
    ok(hres == S_OK, "Could not get enableErrorDialogs from IWMPSettings: %08lx\n", hres);
    ok(vbool == VARIANT_TRUE, "enableErrorDialogs = %x\n", vbool);

    IWMPSettings_Release(settings);
    IWMPPlayer4_Release(player4);

    hres = IOleObject_QueryInterface(oleobj, &IID_IWMPPlayer, (void**)&player);
    ok(hres == S_OK, "Could not get IWMPPlayer iface: %08lx\n", hres);

    settings = NULL;
    hres = IWMPPlayer_get_settings(player, &settings);
    ok(hres == S_OK, "get_settings failed: %08lx\n", hres);
    ok(settings != NULL, "settings = NULL\n");

    hres = IOleObject_QueryInterface(oleobj, &IID_IWMPSettings, (void**)&settings_qi);
    ok(hres == S_OK, "Could not get IWMPSettings iface: %08lx\n", hres);
    ok(settings == settings_qi, "settings != settings_qi\n");
    IWMPSettings_Release(settings_qi);

    IWMPSettings_Release(settings);
    IWMPPlayer_Release(player);
    DeleteFileW(filename);
    SysFreeString(filename);
}

#define test_rect_size(a,b,c) _test_rect_size(__LINE__,a,b,c)
static void _test_rect_size(unsigned line, const RECT *r, int width, int height)
{
    ok_(__FILE__,line)(r->right-r->left == width, "width = %ld, expected %d\n", r->right-r->left, width);
    ok_(__FILE__,line)(r->bottom-r->top == height, "height = %ld, expected %d\n", r->bottom-r->top, height);
}

static void test_window(HWND hwnd)
{
    WINDOWINFO wi = {sizeof(wi)};
    char class_name[100];
    int i;
    BOOL res;

    i = RealGetWindowClassA(hwnd, class_name, sizeof(class_name));
    ok(!strncmp(class_name, "ATL:", 4), "class_name = %s %d\n", class_name, i);

    res = GetWindowInfo(hwnd, &wi);
    ok(res, "GetWindowInfo failed: %lu\n", GetLastError());

    test_rect_size(&wi.rcWindow, 400, 410);
    test_rect_size(&wi.rcClient, 400, 410);

    ok(wi.dwStyle == (WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_VISIBLE|WS_CHILD), "dwStyle = %lx\n", wi.dwStyle);
    ok(!(wi.dwExStyle & ~0x800 /* undocumented, set by some versions */), "dwExStyle = %lx\n", wi.dwExStyle);

    ok(IsWindowVisible(hwnd), "Window is not visible\n");
}

static void test_QI(IUnknown *unk)
{
    IUnknown *tmp;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IQuickActivate, (void**)&tmp);
    ok(hres == E_NOINTERFACE, "Got IQuickActivate iface when no expected\n");

    hres = IUnknown_QueryInterface(unk, &IID_IMarshal, (void**)&tmp);
    ok(hres == E_NOINTERFACE, "Could not get IMarshal iface: %08lx\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IOleInPlaceObjectWindowless, (void**)&tmp);
    ok(hres == S_OK, "Could not get IOleInPlaceObjectWindowless iface: %08lx\n", hres);
    IUnknown_Release(tmp);
}

static void test_IConnectionPointContainer(IOleObject *oleobj)
{
    IConnectionPointContainer *container;
    IConnectionPoint *point;
    HRESULT hres;

    hres = IOleObject_QueryInterface(oleobj, &IID_IConnectionPointContainer, (void**)&container);
    ok(hres == S_OK, "Could not get IConnectionPointContainer iface: %08lx\n", hres);

    hres = IConnectionPointContainer_FindConnectionPoint(container, &IID_IPropertyNotifySink, &point);
    ok(hres == CONNECT_E_NOCONNECTION, "got: %08lx\n", hres);

    point = NULL;
    hres = IConnectionPointContainer_FindConnectionPoint(container, &IID_IWMPEvents, &point);
    todo_wine ok(hres == S_OK, "got: %08lx\n", hres);
    if(point)
        IConnectionPoint_Release(point);

    point = NULL;
    hres = IConnectionPointContainer_FindConnectionPoint(container, &IID_IWMPEvents2, &point);
    todo_wine ok(hres == S_OK, "got: %08lx\n", hres);
    if(point)
        IConnectionPoint_Release(point);

    point = NULL;
    hres = IConnectionPointContainer_FindConnectionPoint(container, &IID_IWMPEvents3, &point);
    if(FAILED(hres))
        todo_wine win_skip("IWMPEvents3 not supported\n");
    if(point)
        IConnectionPoint_Release(point);

    point = NULL;
    hres = IConnectionPointContainer_FindConnectionPoint(container, &IID_IWMPEvents4, &point);
    if(FAILED(hres))
        todo_wine win_skip("IWMPEvents4 not supported\n");
    if(point)
        IConnectionPoint_Release(point);

    point = NULL;
    hres = IConnectionPointContainer_FindConnectionPoint(container, &IID__WMPOCXEvents, &point);
    ok(hres == S_OK, "got: %08lx\n", hres);
    if(point)
        IConnectionPoint_Release(point);

    IConnectionPointContainer_Release(container);
}

static void test_extent(IOleObject *oleobj)
{
    SIZE expected, extent;
    DWORD dpi_x;
    DWORD dpi_y;
    HDC hdc;
    HRESULT hres;

    /* default aspect ratio is 96dpi / 96dpi */
    hdc = GetDC(0);
    dpi_x = GetDeviceCaps(hdc, LOGPIXELSX);
    dpi_y = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(0, hdc);
    if (dpi_x != 96 || dpi_y != 96)
        trace("dpi: %ld / %ld\n", dpi_y, dpi_y);

    extent.cx = extent.cy = 0xdeadbeef;
    hres = IOleObject_GetExtent(oleobj, DVASPECT_CONTENT, &extent);
    ok(hres == S_OK, "GetExtent failed: %08lx\n", hres);
    expected.cx = MulDiv(192, 2540, dpi_x);
    expected.cy = MulDiv(192, 2540, dpi_y);
    ok(extent.cx == expected.cx && extent.cy == expected.cy, "size = {%ld %ld} (expected %ld %ld)\n",
       extent.cx, extent.cy, expected.cx, expected.cy );

    extent.cx = 800;
    extent.cy = 700;
    hres = IOleObject_SetExtent(oleobj, DVASPECT_CONTENT, &extent);
    ok(hres == S_OK, "SetExtent failed: %08lx\n", hres);

    extent.cx = extent.cy = 0xdeadbeef;
    hres = IOleObject_GetExtent(oleobj, DVASPECT_CONTENT, &extent);
    ok(hres == S_OK, "GetExtent failed: %08lx\n", hres);
    ok(extent.cx == 800 && extent.cy == 700, "size = {%ld %ld}\n", extent.cx, extent.cy);

    hres = IOleObject_GetExtent(oleobj, 0, &extent);
    ok(hres == E_FAIL, "GetExtent failed: %08lx\n", hres);
    hres = IOleObject_GetExtent(oleobj, 7, &extent);
    ok(hres == E_FAIL, "GetExtent failed: %08lx\n", hres);

    extent.cx = 900;
    extent.cy = 800;
    hres = IOleObject_SetExtent(oleobj, DVASPECT_CONTENT, &expected);
    ok(hres == S_OK, "SetExtent failed: %08lx\n", hres);

    hres = IOleObject_SetExtent(oleobj, 0, &expected);
    ok(hres == DV_E_DVASPECT, "SetExtent failed: %08lx\n", hres);
    hres = IOleObject_SetExtent(oleobj, 7, &expected);
    ok(hres == DV_E_DVASPECT, "SetExtent failed: %08lx\n", hres);
}

static void test_wmp(void)
{
    IProvideClassInfo2 *class_info;
    IOleClientSite *client_site;
    IOleInPlaceObject *ipobj;
    IPersistStreamInit *psi;
    IOleObject *oleobj;
    IWMPCore *wmpcore;
    DWORD misc_status;
    TYPEATTR *attr;
    ITypeInfo *ti;
    RECT pos = {0,0,100,100};
    HWND hwnd;
    GUID guid;
    LONG ref;
    HRESULT hres;
    BSTR str;

    hres = CoCreateInstance(&CLSID_WindowsMediaPlayer, NULL, CLSCTX_INPROC_SERVER, &IID_IOleObject, (void**)&oleobj);
    if(hres == REGDB_E_CLASSNOTREG) {
        win_skip("CLSID_WindowsMediaPlayer not registered\n");
        return;
    }
    ok(hres == S_OK, "Could not create CLSID_WindowsMediaPlayer instance: %08lx\n", hres);

    hres = IOleObject_QueryInterface(oleobj, &IID_IWMPCore, (void**)&wmpcore);
    ok(hres == S_OK, "got 0x%08lx\n", hres);

    hres = IWMPCore_get_versionInfo(wmpcore, NULL);
    ok(hres == E_POINTER, "got 0x%08lx\n", hres);

    hres = IWMPCore_get_versionInfo(wmpcore, &str);
    ok(hres == S_OK, "got 0x%08lx\n", hres);
    SysFreeString(str);

    IWMPCore_Release(wmpcore);

    hres = IOleObject_QueryInterface(oleobj, &IID_IProvideClassInfo2, (void**)&class_info);
    ok(hres == S_OK, "Could not get IProvideClassInfo2 iface: %08lx\n", hres);

    hres = IProvideClassInfo2_GetGUID(class_info, GUIDKIND_DEFAULT_SOURCE_DISP_IID, &guid);
    ok(hres == S_OK, "GetGUID failed: %08lx\n", hres);
    ok(IsEqualGUID(&guid, &IID__WMPOCXEvents), "guid = %s\n", wine_dbgstr_guid(&guid));

    hres = IProvideClassInfo2_GetClassInfo(class_info, &ti);
    ok(hres == S_OK, "Failed to get class info, hr %#lx.\n", hres);
    hres = ITypeInfo_GetTypeAttr(ti, &attr);
    ok(hres == S_OK, "Failed to get type attributes, hr %#lx.\n", hres);
    ok(IsEqualGUID(&CLSID_WindowsMediaPlayer, &attr->guid), "Unexpected typeinfo guid %s\n", wine_dbgstr_guid(&attr->guid));
    ITypeInfo_ReleaseTypeAttr(ti, attr);
    ITypeInfo_Release(ti);

    IProvideClassInfo2_Release(class_info);

    test_QI((IUnknown*)oleobj);
    test_IConnectionPointContainer(oleobj);
    test_extent(oleobj);

    hres = IOleObject_GetMiscStatus(oleobj, DVASPECT_CONTENT, &misc_status);
    ok(hres == S_OK, "GetMiscStatus failed: %08lx\n", hres);
    ok(misc_status == (OLEMISC_SETCLIENTSITEFIRST|OLEMISC_ACTIVATEWHENVISIBLE|OLEMISC_INSIDEOUT
                       |OLEMISC_CANTLINKINSIDE|OLEMISC_RECOMPOSEONRESIZE), "misc_status = %lx\n", misc_status);

    hres = IOleObject_QueryInterface(oleobj, &IID_IPersistStreamInit, (void**)&psi);
    ok(hres == S_OK, "Could not get IPersistStreamInit iface: %08lx\n", hres);

    hres = IOleObject_QueryInterface(oleobj, &IID_IOleInPlaceObject, (void**)&ipobj);
    ok(hres == S_OK, "Could not get IOleInPlaceObject iface: %08lx\n", hres);

    hres = IPersistStreamInit_InitNew(psi);
    ok(hres == E_FAIL || broken(hres == S_OK /* Old WMP */), "InitNew failed: %08lx\n", hres);

    SET_EXPECT(GetContainer);
    SET_EXPECT(GetExtendedControl);
    SET_EXPECT(GetWindow);
    SET_EXPECT(Invoke_USERMODE);
    hres = IOleObject_SetClientSite(oleobj, &ClientSite);
    ok(hres == S_OK, "SetClientSite failed: %08lx\n", hres);
    todo_wine CHECK_CALLED(GetContainer);
    CHECK_CALLED(GetExtendedControl);
    todo_wine CHECK_CALLED(GetWindow);
    todo_wine CHECK_CALLED(Invoke_USERMODE);

    client_site = NULL;
    hres = IOleObject_GetClientSite(oleobj, &client_site);
    ok(hres == S_OK, "GetClientSite failed: %08lx\n", hres);
    ok(client_site == &ClientSite, "client_site != ClientSite\n");

    SET_EXPECT(GetWindow);
    hres = IPersistStreamInit_InitNew(psi);
    ok(hres == S_OK, "InitNew failed: %08lx\n", hres);
    CHECK_CALLED(GetWindow);

    hwnd = (HWND)0xdeadbeef;
    hres = IOleInPlaceObject_GetWindow(ipobj, &hwnd);
    ok(hres == E_UNEXPECTED, "GetWindow failed: %08lx\n", hres);
    ok(!hwnd, "hwnd = %p\n", hwnd);

    SET_EXPECT(GetWindow);
    SET_EXPECT(CanWindowlessActivate);
    SET_EXPECT(OnInPlaceActivateEx);
    SET_EXPECT(GetWindowContext);
    SET_EXPECT(ShowObject);
    hres = IOleObject_DoVerb(oleobj, OLEIVERB_INPLACEACTIVATE, NULL, &ClientSite, 0, container_hwnd, &pos);
    ok(hres == S_OK, "DoVerb failed: %08lx\n", hres);
    CHECK_CALLED(GetWindow);
    CHECK_CALLED(CanWindowlessActivate);
    CHECK_CALLED(OnInPlaceActivateEx);
    CHECK_CALLED(GetWindowContext);
    CHECK_CALLED(ShowObject);

    hwnd = NULL;
    hres = IOleInPlaceObject_GetWindow(ipobj, &hwnd);
    ok(hres == S_OK, "GetWindow failed: %08lx\n", hres);
    ok(hwnd != NULL, "hwnd = NULL\n");

    test_window(hwnd);

    SetRect(&pos, 1, 2, 301, 312);
    hres = IOleInPlaceObject_SetObjectRects(ipobj, &pos, &pos);
    ok(hres == S_OK, "SetObjectRects failed: %08lx\n", hres);
    GetClientRect(hwnd, &pos);
    test_rect_size(&pos, 300, 310);

    test_wmp_ifaces(oleobj);

    hres = IOleObject_DoVerb(oleobj, OLEIVERB_HIDE, NULL, &ClientSite, 0, container_hwnd, &pos);
    ok(hres == S_OK, "DoVerb failed: %08lx\n", hres);
    ok(!IsWindowVisible(hwnd), "Window is visible\n");

    SET_EXPECT(OnShowWindow_FALSE);
    SET_EXPECT(OnInPlaceDeactivate);
    hres = IOleObject_Close(oleobj, 0);
    ok(hres == S_OK, "Close failed: %08lx\n", hres);
    todo_wine CHECK_CALLED(OnShowWindow_FALSE);
    CHECK_CALLED(OnInPlaceDeactivate);

    hwnd = (HWND)0xdeadbeef;
    hres = IOleInPlaceObject_GetWindow(ipobj, &hwnd);
    ok(hres == E_UNEXPECTED, "GetWindow failed: %08lx\n", hres);
    ok(!hwnd, "hwnd = %p\n", hwnd);

    hres = IOleObject_Close(oleobj, 0);
    ok(hres == S_OK, "Close failed: %08lx\n", hres);

    hres = IOleObject_SetClientSite(oleobj, NULL);
    ok(hres == S_OK, "SetClientSite failed: %08lx\n", hres);

    client_site = (void*)0xdeadbeef;
    hres = IOleObject_GetClientSite(oleobj, &client_site);
    ok(hres == E_FAIL || broken(hres == S_OK), "GetClientSite failed: %08lx\n", hres);
    ok(!client_site, "client_site = %p\n", client_site);

    test_ConnectionPoint(oleobj);

    IPersistStreamInit_Release(psi);
    IOleInPlaceObject_Release(ipobj);

    ref = IOleObject_Release(oleobj);
    ok(!ref, "ref = %ld\n", ref);
}

static LRESULT WINAPI wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static void create_container_window(void)
{
    static WNDCLASSEXW wndclass = {
        sizeof(WNDCLASSEXW),
        0,
        wnd_proc,
        0, 0, NULL, NULL, NULL, NULL, NULL,
        L"WMPTest",
        NULL
    };

    RegisterClassExW(&wndclass);
    container_hwnd = CreateWindowW(L"WMPTest", L"WMPTest",
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
            515, 530, NULL, NULL, NULL, NULL);
    ShowWindow(container_hwnd, SW_SHOW);
}

START_TEST(oleobj)
{
    CoInitialize(NULL);
    create_container_window();

    test_wmp();

    DestroyWindow(container_hwnd);
    CoUninitialize();
}
