/*
 * Copyright 2005 Jacek Caban
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define COBJMACROS

#include <wine/test.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "mshtml.h"
#include "docobj.h"
#include "mshtmhst.h"
#include "shlguid.h"

#include "initguid.h"
DEFINE_SHLGUID(CGID_Undocumented, 0x000214D4L, 0, 0);

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT(func) \
    ok(expect_ ##func, "unexpected call\n"); \
    expect_ ## func = FALSE; \
    called_ ## func = TRUE

#define CHECK_CALLED(func) \
    ok(called_ ## func, "expected " #func "\n"); \
    expect_ ## func = called_ ## func = FALSE

static IOleDocumentView *view = NULL;
static HWND container_hwnd = NULL, hwnd = NULL, last_hwnd = NULL;

DEFINE_EXPECT(LockContainer);
DEFINE_EXPECT(SetActiveObject);
DEFINE_EXPECT(GetWindow);
DEFINE_EXPECT(CanInPlaceActivate);
DEFINE_EXPECT(OnInPlaceActivate);
DEFINE_EXPECT(OnUIActivate);
DEFINE_EXPECT(GetWindowContext);
DEFINE_EXPECT(OnUIDeactivate);
DEFINE_EXPECT(OnInPlaceDeactivate);
DEFINE_EXPECT(GetContainer);
DEFINE_EXPECT(ShowUI);
DEFINE_EXPECT(ActivateMe);
DEFINE_EXPECT(GetHostInfo);
DEFINE_EXPECT(HideUI);
DEFINE_EXPECT(GetOptionKeyPath);
DEFINE_EXPECT(GetOverrideKeyPath);
DEFINE_EXPECT(SetStatusText);
DEFINE_EXPECT(QueryStatus_SETPROGRESSTEXT);
DEFINE_EXPECT(QueryStatus_OPEN);
DEFINE_EXPECT(QueryStatus_NEW);
DEFINE_EXPECT(Exec_SETPROGRESSMAX);
DEFINE_EXPECT(Exec_SETPROGRESSPOS);

static BOOL expect_LockContainer_fLock;
static BOOL expect_SetActiveObject_active;

static HRESULT QueryInterface(REFIID riid, void **ppv);

static HRESULT WINAPI OleContainer_QueryInterface(IOleContainer *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
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
    CHECK_EXPECT(LockContainer);
    ok(expect_LockContainer_fLock == fLock, "fLock=%x, expected %x\n", fLock, expect_LockContainer_fLock);
    return S_OK;
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
    return QueryInterface(riid, ppv);
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

static HRESULT WINAPI InPlaceFrame_SetActiveObject(IOleInPlaceFrame *iface,
        IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    static const WCHAR wszHTML_Document[] =
        {'H','T','M','L',' ','D','o','c','u','m','e','n','t',0};

    ok(expect_SetActiveObject, "unexpected call\n");
    called_SetActiveObject = TRUE;

    if(expect_SetActiveObject_active) {
        ok(pActiveObject != NULL, "pActiveObject = NULL\n");
        ok(!lstrcmpW(wszHTML_Document, pszObjName), "pszObjName != \"HTML Document\"\n");
    }else {
        ok(pActiveObject == NULL, "pActiveObject=%p, expected NULL\n", pActiveObject);
        ok(pszObjName == NULL, "pszObjName=%p, expected NULL\n", pszObjName);
    }

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
    CHECK_EXPECT(SetStatusText);
    ok(pszStatusText == NULL, "pszStatusText=%p, expected NULL\n", pszStatusText);
    return S_OK;
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

static HRESULT WINAPI InPlaceSite_QueryInterface(IOleInPlaceSite *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI InPlaceSite_AddRef(IOleInPlaceSite *iface)
{
    return 2;
}

static ULONG WINAPI InPlaceSite_Release(IOleInPlaceSite *iface)
{
    return 1;
}

static HRESULT WINAPI InPlaceSite_GetWindow(IOleInPlaceSite *iface, HWND *phwnd)
{
    CHECK_EXPECT(GetWindow);
    ok(phwnd != NULL, "phwnd = NULL\n");
    *phwnd = container_hwnd;
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_ContextSensitiveHelp(IOleInPlaceSite *iface, BOOL fEnterMode)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_CanInPlaceActivate(IOleInPlaceSite *iface)
{
    CHECK_EXPECT(CanInPlaceActivate);
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_OnInPlaceActivate(IOleInPlaceSite *iface)
{
    CHECK_EXPECT(OnInPlaceActivate);
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_OnUIActivate(IOleInPlaceSite *iface)
{
    CHECK_EXPECT(OnUIActivate);
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_GetWindowContext(IOleInPlaceSite *iface,
        IOleInPlaceFrame **ppFrame, IOleInPlaceUIWindow **ppDoc, LPRECT lprcPosRect,
        LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    static const RECT rect = {0,0,500,500};

    CHECK_EXPECT(GetWindowContext);

    ok(ppFrame != NULL, "ppFrame = NULL\n");
    if(ppFrame)
        *ppFrame = &InPlaceFrame;
    ok(ppDoc != NULL, "ppDoc = NULL\n");
    if(ppDoc)
        *ppDoc = NULL;
    ok(lprcPosRect != NULL, "lprcPosRect = NULL\n");
    if(lprcPosRect)
        memcpy(lprcPosRect, &rect, sizeof(RECT));
    ok(lprcClipRect != NULL, "lprcClipRect = NULL\n");
    if(lprcClipRect)
        memcpy(lprcClipRect, &rect, sizeof(RECT));
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

static HRESULT WINAPI InPlaceSite_Scroll(IOleInPlaceSite *iface, SIZE scrollExtant)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnUIDeactivate(IOleInPlaceSite *iface, BOOL fUndoable)
{
    CHECK_EXPECT(OnUIDeactivate);
    ok(!fUndoable, "fUndoable = TRUE\n");
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_OnInPlaceDeactivate(IOleInPlaceSite *iface)
{
    CHECK_EXPECT(OnInPlaceDeactivate);
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_DiscardUndoState(IOleInPlaceSite *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_DeactivateAndUndo(IOleInPlaceSite *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnPosRectChange(IOleInPlaceSite *iface, LPCRECT lprcPosRect)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IOleInPlaceSiteVtbl InPlaceSiteVtbl = {
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
    InPlaceSite_OnPosRectChange
};

static IOleInPlaceSite InPlaceSite = { &InPlaceSiteVtbl };

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
    ok(ppContainer != NULL, "ppContainer = NULL\n");
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

static HRESULT WINAPI DocumentSite_QueryInterface(IOleDocumentSite *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI DocumentSite_AddRef(IOleDocumentSite *iface)
{
    return 2;
}

static ULONG WINAPI DocumentSite_Release(IOleDocumentSite *iface)
{
    return 1;
}

static BOOL call_UIActivate = TRUE;
static HRESULT WINAPI DocumentSite_ActivateMe(IOleDocumentSite *iface, IOleDocumentView *pViewToActivate)
{
    IOleDocument *document;
    HRESULT hres;

    CHECK_EXPECT(ActivateMe);
    ok(pViewToActivate != NULL, "pViewToActivate = NULL\n");

    hres = IOleDocumentView_QueryInterface(pViewToActivate, &IID_IOleDocument, (void**)&document);
    ok(hres == S_OK, "could not get IOleDocument: %08lx\n", hres);

    if(SUCCEEDED(hres)) {
        hres = IOleDocument_CreateView(document, &InPlaceSite, NULL, 0, &view);
        ok(hres == S_OK, "CreateView failed: %08lx\n", hres);

        if(SUCCEEDED(hres)) {
            IOleInPlaceActiveObject *activeobj = NULL;
            IOleInPlaceSite *inplacesite = NULL;
            HWND tmp_hwnd = NULL;
            static RECT rect = {0,0,400,500};

            hres = IOleDocumentView_GetInPlaceSite(view, &inplacesite);
            ok(hres == S_OK, "GetInPlaceSite failed: %08lx\n", hres);
            ok(inplacesite == &InPlaceSite, "inplacesite=%p, expected %p\n",
                    inplacesite, &InPlaceSite);

            hres = IOleDocumentView_SetInPlaceSite(view, &InPlaceSite);
            ok(hres == S_OK, "SetInPlaceSite failed: %08lx\n", hres);

            hres = IOleDocumentView_GetInPlaceSite(view, &inplacesite);
            ok(hres == S_OK, "GetInPlaceSite failed: %08lx\n", hres);
            ok(inplacesite == &InPlaceSite, "inplacesite=%p, expected %p\n",
                    inplacesite, &InPlaceSite);

            hres = IOleDocumentView_QueryInterface(view, &IID_IOleInPlaceActiveObject, (void**)&activeobj);
            ok(hres == S_OK, "Could not get IOleInPlaceActiveObject: %08lx\n", hres);

            if(activeobj) {
                IOleInPlaceActiveObject_GetWindow(activeobj, &hwnd);
                ok(hres == S_OK, "GetWindow failed: %08lx\n", hres);
                ok(hwnd == NULL, "hwnd=%p, expeted NULL\n", hwnd);
            }
            
            if(call_UIActivate) {
                SET_EXPECT(CanInPlaceActivate);
                SET_EXPECT(GetWindowContext);
                SET_EXPECT(GetWindow);
                SET_EXPECT(OnInPlaceActivate);
                SET_EXPECT(SetStatusText);
                SET_EXPECT(Exec_SETPROGRESSMAX);
                SET_EXPECT(Exec_SETPROGRESSPOS);
                SET_EXPECT(OnUIActivate);
                SET_EXPECT(SetActiveObject);
                SET_EXPECT(ShowUI);
                expect_SetActiveObject_active = TRUE;

                hres = IOleDocumentView_UIActivate(view, TRUE);

                if(FAILED(hres)) {
                    trace("UIActivate failed: %08lx\n", hres);
                    return hres;
                }
                ok(hres == S_OK, "UIActivate failed: %08lx\n", hres);

                CHECK_CALLED(CanInPlaceActivate);
                CHECK_CALLED(GetWindowContext);
                CHECK_CALLED(GetWindow);
                CHECK_CALLED(OnInPlaceActivate);
                CHECK_CALLED(SetStatusText);
                CHECK_CALLED(Exec_SETPROGRESSMAX);
                CHECK_CALLED(Exec_SETPROGRESSPOS);
                CHECK_CALLED(OnUIActivate);
                CHECK_CALLED(SetActiveObject);
                CHECK_CALLED(ShowUI);

                if(activeobj) {
                    hres = IOleInPlaceActiveObject_GetWindow(activeobj, &hwnd);
                    ok(hres == S_OK, "GetWindow failed: %08lx\n", hres);
                    ok(hwnd != NULL, "hwnd == NULL\n");
                    if(last_hwnd)
                        ok(hwnd == last_hwnd, "hwnd != last_hwnd\n");
                }

                hres = IOleDocumentView_UIActivate(view, TRUE);
                ok(hres == S_OK, "UIActivate failed: %08lx\n", hres);

                if(activeobj) {
                    hres = IOleInPlaceActiveObject_GetWindow(activeobj, &tmp_hwnd);
                    ok(hres == S_OK, "GetWindow failed: %08lx\n", hres);
                    ok(tmp_hwnd == hwnd, "tmp_hwnd=%p, expected %p\n", tmp_hwnd, hwnd);
                }
            }

            hres = IOleDocumentView_SetRect(view, &rect);
            ok(hres == S_OK, "SetRect failed: %08lx\n", hres);

            if(call_UIActivate) {
                hres = IOleDocumentView_Show(view, TRUE);
                ok(hres == S_OK, "Show failed: %08lx\n", hres);
            }else {
                SET_EXPECT(CanInPlaceActivate);
                SET_EXPECT(GetWindowContext);
                SET_EXPECT(GetWindow);
                SET_EXPECT(OnInPlaceActivate);
                SET_EXPECT(SetStatusText);
                SET_EXPECT(Exec_SETPROGRESSMAX);
                SET_EXPECT(Exec_SETPROGRESSPOS);
                SET_EXPECT(OnUIActivate);
                hres = IOleDocumentView_Show(view, TRUE);
                ok(hres == S_OK, "Show failed: %08lx\n", hres);
                CHECK_CALLED(CanInPlaceActivate);
                CHECK_CALLED(GetWindowContext);
                CHECK_CALLED(GetWindow);
                CHECK_CALLED(OnInPlaceActivate);
                CHECK_CALLED(SetStatusText);
                CHECK_CALLED(Exec_SETPROGRESSMAX);
                CHECK_CALLED(Exec_SETPROGRESSPOS);

                if(activeobj) {
                    hres = IOleInPlaceActiveObject_GetWindow(activeobj, &hwnd);
                    ok(hres == S_OK, "GetWindow failed: %08lx\n", hres);
                    ok(hwnd != NULL, "hwnd == NULL\n");
                    if(last_hwnd)
                        ok(hwnd == last_hwnd, "hwnd != last_hwnd\n");
                }
            }

            if(activeobj)
                IOleInPlaceActiveObject_Release(activeobj);
        }

        IOleDocument_Release(document);
    }

    return S_OK;
}

static const IOleDocumentSiteVtbl DocumentSiteVtbl = {
    DocumentSite_QueryInterface,
    DocumentSite_AddRef,
    DocumentSite_Release,
    DocumentSite_ActivateMe
};

static IOleDocumentSite DocumentSite = { &DocumentSiteVtbl };

static HRESULT WINAPI DocHostUIHandler_QueryInterface(IDocHostUIHandler2 *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI DocHostUIHandler_AddRef(IDocHostUIHandler2 *iface)
{
    return 2;
}

static ULONG WINAPI DocHostUIHandler_Release(IDocHostUIHandler2 *iface)
{
    return 1;
}

static HRESULT WINAPI DocHostUIHandler_ShowContextMenu(IDocHostUIHandler2 *iface, DWORD dwID, POINT *ppt,
        IUnknown *pcmdtReserved, IDispatch *pdicpReserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetHostInfo(IDocHostUIHandler2 *iface, DOCHOSTUIINFO *pInfo)
{
    CHECK_EXPECT(GetHostInfo);
    ok(pInfo != NULL, "pInfo=NULL\n");
    if(pInfo) {
        ok(pInfo->cbSize == sizeof(DOCHOSTUIINFO), "pInfo->cbSize=%lu, expected %u\n",
                pInfo->cbSize, sizeof(DOCHOSTUIINFO));
        ok(!pInfo->dwFlags, "pInfo->dwFlags=%08lx, expected 0\n", pInfo->dwFlags);
        pInfo->dwFlags = DOCHOSTUIFLAG_DISABLE_HELP_MENU | DOCHOSTUIFLAG_DISABLE_SCRIPT_INACTIVE
            | DOCHOSTUIFLAG_ACTIVATE_CLIENTHIT_ONLY | DOCHOSTUIFLAG_ENABLE_INPLACE_NAVIGATION
            | DOCHOSTUIFLAG_IME_ENABLE_RECONVERSION;
        ok(!pInfo->dwDoubleClick, "pInfo->dwDoubleClick=%08lx, expected 0\n", pInfo->dwDoubleClick);
        ok(!pInfo->pchHostCss, "pInfo->pchHostCss=%p, expected NULL\n", pInfo->pchHostCss);
        ok(!pInfo->pchHostNS, "pInfo->pchhostNS=%p, expected NULL\n", pInfo->pchHostNS);
    }
    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_ShowUI(IDocHostUIHandler2 *iface, DWORD dwID,
        IOleInPlaceActiveObject *pActiveObject, IOleCommandTarget *pCommandTarget,
        IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc)
{
    CHECK_EXPECT(ShowUI);

    ok(dwID == 0, "dwID=%ld, expected 0\n", dwID);
    ok(pActiveObject != NULL, "pActiveObject = NULL\n");
    ok(pCommandTarget != NULL, "pCommandTarget = NULL\n");
    ok(pFrame == &InPlaceFrame, "pFrame=%p, expected %p\n", pFrame, &InPlaceFrame);
    ok(pDoc == NULL, "pDoc=%p, expected NULL\n", pDoc);

    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_HideUI(IDocHostUIHandler2 *iface)
{
    CHECK_EXPECT(HideUI);
    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_UpdateUI(IDocHostUIHandler2 *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_EnableModeless(IDocHostUIHandler2 *iface, BOOL fEnable)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_OnDocWindowActivate(IDocHostUIHandler2 *iface, BOOL fActivate)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_OnFrameWindowActivate(IDocHostUIHandler2 *iface, BOOL fActivate)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_ResizeBorder(IDocHostUIHandler2 *iface, LPCRECT prcBorder,
        IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_TranslateAccelerator(IDocHostUIHandler2 *iface, LPMSG lpMsg,
        const GUID *pguidCmdGroup, DWORD nCmdID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetOptionKeyPath(IDocHostUIHandler2 *iface,
        LPOLESTR *pchKey, DWORD dw)
{
    CHECK_EXPECT(GetOptionKeyPath);
    ok(pchKey != NULL, "pchKey = NULL\n");
    ok(!dw, "dw=%ld, expected 0\n", dw);
    if(pchKey)
        ok(!*pchKey, "*pchKey=%p, expected NULL\n", *pchKey);
    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_GetDropTarget(IDocHostUIHandler2 *iface,
        IDropTarget *pDropTarget, IDropTarget **ppDropTarget)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetExternal(IDocHostUIHandler2 *iface, IDispatch **ppDispatch)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_TranslateUrl(IDocHostUIHandler2 *iface, DWORD dwTranslate,
        OLECHAR *pchURLIn, OLECHAR **ppchURLOut)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_FilterDataObject(IDocHostUIHandler2 *iface, IDataObject *pDO,
        IDataObject **ppPORet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetOverrideKeyPath(IDocHostUIHandler2 *iface,
        LPOLESTR *pchKey, DWORD dw)
{
    CHECK_EXPECT(GetOverrideKeyPath);
    ok(pchKey != NULL, "pchKey = NULL\n");
    if(pchKey)
        ok(!*pchKey, "*pchKey=%p, expected NULL\n", *pchKey);
    ok(!dw, "dw=%ld, xepected 0\n", dw);
    return S_OK;
}

static const IDocHostUIHandler2Vtbl DocHostUIHandlerVtbl = {
    DocHostUIHandler_QueryInterface,
    DocHostUIHandler_AddRef,
    DocHostUIHandler_Release,
    DocHostUIHandler_ShowContextMenu,
    DocHostUIHandler_GetHostInfo,
    DocHostUIHandler_ShowUI,
    DocHostUIHandler_HideUI,
    DocHostUIHandler_UpdateUI,
    DocHostUIHandler_EnableModeless,
    DocHostUIHandler_OnDocWindowActivate,
    DocHostUIHandler_OnFrameWindowActivate,
    DocHostUIHandler_ResizeBorder,
    DocHostUIHandler_TranslateAccelerator,
    DocHostUIHandler_GetOptionKeyPath,
    DocHostUIHandler_GetDropTarget,
    DocHostUIHandler_GetExternal,
    DocHostUIHandler_TranslateUrl,
    DocHostUIHandler_FilterDataObject,
    DocHostUIHandler_GetOverrideKeyPath
};

static IDocHostUIHandler2 DocHostUIHandler = { &DocHostUIHandlerVtbl };

static HRESULT WINAPI OleCommandTarget_QueryInterface(IOleCommandTarget *iface,
        REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI OleCommandTarget_AddRef(IOleCommandTarget *iface)
{
    return 2;
}

static ULONG WINAPI OleCommandTarget_Release(IOleCommandTarget *iface)
{
    return 1;
}

static HRESULT WINAPI OleCommandTarget_QueryStatus(IOleCommandTarget *iface, const GUID *pguidCmdGroup,
        ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    ok(!pguidCmdGroup, "pguidCmdGroup != MULL\n");
    ok(cCmds == 1, "cCmds=%ld, expected 1\n", cCmds);
    ok(!pCmdText, "pCmdText != NULL\n");

    switch(prgCmds[0].cmdID) {
    case OLECMDID_SETPROGRESSTEXT:
        CHECK_EXPECT(QueryStatus_SETPROGRESSTEXT);
        prgCmds[0].cmdf = OLECMDF_ENABLED;
        return S_OK;
    case OLECMDID_OPEN:
        CHECK_EXPECT(QueryStatus_OPEN);
        prgCmds[0].cmdf = 0;
        return S_OK;
    case OLECMDID_NEW:
        CHECK_EXPECT(QueryStatus_NEW);
        prgCmds[0].cmdf = 0;
        return S_OK;
    default:
        ok(0, "unexpected command %ld\n", prgCmds[0].cmdID);
    };

    return E_FAIL;
}

static HRESULT WINAPI OleCommandTarget_Exec(IOleCommandTarget *iface, const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    if(!pguidCmdGroup) {
        switch(nCmdID) {
        case OLECMDID_SETPROGRESSMAX:
            CHECK_EXPECT(Exec_SETPROGRESSMAX);
            ok(pvaIn != NULL, "pvaIn == NULL\n");
            if(pvaIn) {
                ok(V_VT(pvaIn) == VT_I4, "V_VT(pvaIn)=%d, expected VT_I4\n", V_VT(pvaIn));
                ok(V_I4(pvaIn) == 0, "V_I4(pvaIn)=%ld, expected 0\n", V_I4(pvaIn));
            }
            ok(pvaOut == NULL, "pvaOut=%p, expected NULL\n", pvaOut);
            return S_OK;
        case OLECMDID_SETPROGRESSPOS:
            CHECK_EXPECT(Exec_SETPROGRESSPOS);
            ok(pvaIn != NULL, "pvaIn == NULL\n");
            if(pvaIn) {
                ok(V_VT(pvaIn) == VT_I4, "V_VT(pvaIn)=%d, expected VT_I4\n", V_VT(pvaIn));
                ok(V_I4(pvaIn) == 0, "V_I4(pvaIn)=%ld, expected 0\n", V_I4(pvaIn));
            }
            ok(pvaOut == NULL, "pvaOut=%p, expected NULL\n", pvaOut);
            return S_OK;
        default:
            ok(0, "unexpected command %ld\n", nCmdID);
            return E_FAIL;
        };
    }

    if(IsEqualGUID(&CGID_Undocumented, pguidCmdGroup))
        return E_FAIL; /* TODO */

    ok(0, "unexpected call");

    return E_NOTIMPL;
}

static IOleCommandTargetVtbl OleCommandTargetVtbl = {
    OleCommandTarget_QueryInterface,
    OleCommandTarget_AddRef,
    OleCommandTarget_Release,
    OleCommandTarget_QueryStatus,
    OleCommandTarget_Exec
};

static IOleCommandTarget OleCommandTarget = { &OleCommandTargetVtbl };

static HRESULT QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IOleClientSite, riid))
        *ppv = &ClientSite;
    else if(IsEqualGUID(&IID_IOleDocumentSite, riid))
        *ppv = &DocumentSite;
    else if(IsEqualGUID(&IID_IDocHostUIHandler, riid) || IsEqualGUID(&IID_IDocHostUIHandler2, riid))
        *ppv = &DocHostUIHandler;
    else if(IsEqualGUID(&IID_IOleContainer, riid))
        *ppv = &OleContainer;
    else if(IsEqualGUID(&IID_IOleWindow, riid) || IsEqualGUID(&IID_IOleInPlaceSite, riid))
        *ppv = &InPlaceSite;
    else if(IsEqualGUID(&IID_IOleInPlaceUIWindow, riid) || IsEqualGUID(&IID_IOleInPlaceFrame, riid))
        *ppv = &InPlaceFrame;
    else if(IsEqualGUID(&IID_IOleCommandTarget , riid))
        *ppv = &OleCommandTarget;

    /* TODO:
     * IDispatch
     * IServiceProvider
     * {D48A6EC6-6A4A-11CF-94A7-444553540000}
     * {7BB0B520-B1A7-11D2-BB23-00C04F79ABCD}
     * {000670BA-0000-0000-C000-000000000046}
     */

    if(*ppv)
        return S_OK;
    return E_NOINTERFACE;
}

static LRESULT WINAPI wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void test_Persist(IUnknown *unk)
{
    IPersistMoniker *persist_mon;
    IPersistFile *persist_file;
    GUID guid;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IPersistFile, (void**)&persist_file);
    ok(hres == S_OK, "QueryInterface(IID_IPersist) failed: %08lx\n", hres);
    if(SUCCEEDED(hres)) {
        hres = IPersist_GetClassID(persist_file, NULL);
        ok(hres == E_INVALIDARG, "GetClassID returned: %08lx, expected E_INVALIDARG\n", hres);

        hres = IPersist_GetClassID(persist_file, &guid);
        ok(hres == S_OK, "GetClassID failed: %08lx\n", hres);
        ok(IsEqualGUID(&CLSID_HTMLDocument, &guid), "guid != CLSID_HTMLDocument\n");

        IPersist_Release(persist_file);
    }

    hres = IUnknown_QueryInterface(unk, &IID_IPersistMoniker, (void**)&persist_mon);
    ok(hres == S_OK, "QueryInterface(IID_IPersistMoniker) failed: %08lx\n", hres);
    if(SUCCEEDED(hres)) {
        hres = IPersistMoniker_GetClassID(persist_mon, NULL);
        ok(hres == E_INVALIDARG, "GetClassID returned: %08lx, expected E_INVALIDARG\n", hres);

        hres = IPersistMoniker_GetClassID(persist_mon, &guid);
        ok(hres == S_OK, "GetClassID failed: %08lx\n", hres);
        ok(IsEqualGUID(&CLSID_HTMLDocument, &guid), "guid != CLSID_HTMLDocument\n");

        IPersistMoniker_Release(persist_mon);
    }
}

static const OLECMDF expect_cmds[OLECMDID_GETPRINTTEMPLATE+1] = {
    0,
    OLECMDF_SUPPORTED,                  /* OLECMDID_OPEN */
    OLECMDF_SUPPORTED,                  /* OLECMDID_NEW */
    OLECMDF_SUPPORTED,                  /* OLECMDID_SAVE */
    OLECMDF_SUPPORTED|OLECMDF_ENABLED,  /* OLECMDID_SAVEAS */
    OLECMDF_SUPPORTED,                  /* OLECMDID_SAVECOPYAS */
    OLECMDF_SUPPORTED|OLECMDF_ENABLED,  /* OLECMDID_PRINT */
    OLECMDF_SUPPORTED|OLECMDF_ENABLED,  /* OLECMDID_PRINTPREVIEW */
    OLECMDF_SUPPORTED|OLECMDF_ENABLED,  /* OLECMDID_PAGESETUP */
    OLECMDF_SUPPORTED,                  /* OLECMDID_SPELL */
    OLECMDF_SUPPORTED|OLECMDF_ENABLED,  /* OLECMDID_PROPERTIES */
    OLECMDF_SUPPORTED,                  /* OLECMDID_CUT */
    OLECMDF_SUPPORTED,                  /* OLECMDID_COPY */
    OLECMDF_SUPPORTED,                  /* OLECMDID_PASTE */
    OLECMDF_SUPPORTED,                  /* OLECMDID_PASTESPECIAL */
    OLECMDF_SUPPORTED,                  /* OLECMDID_UNDO */
    OLECMDF_SUPPORTED,                  /* OLECMDID_REDO */
    OLECMDF_SUPPORTED|OLECMDF_ENABLED,  /* OLECMDID_SELECTALL */
    OLECMDF_SUPPORTED,                  /* OLECMDID_CLEARSELECTION */
    OLECMDF_SUPPORTED,                  /* OLECMDID_ZOOM */
    OLECMDF_SUPPORTED,                  /* OLECMDID_GETZOOMRANGE */
    0,
    OLECMDF_SUPPORTED|OLECMDF_ENABLED,  /* OLECMDID_REFRESH */
    OLECMDF_SUPPORTED|OLECMDF_ENABLED,  /* OLECMDID_STOP */
    0,0,0,0,0,0,
    OLECMDF_SUPPORTED,                  /* OLECMDID_STOPDOWNLOAD */
    0,0,
    OLECMDF_SUPPORTED,                  /* OLECMDID_DELETE */
    0,0,
    OLECMDF_SUPPORTED,                  /* OLECMDID_ENABLE_INTERACTION */
    OLECMDF_SUPPORTED,                  /* OLECMDID_ONUNLOAD */
    0,0,0,0,0,
    OLECMDF_SUPPORTED,                  /* OLECMDID_SHOWPAGESETUP */
    OLECMDF_SUPPORTED,                  /* OLECMDID_SHOWPRINT */
    0,0,
    OLECMDF_SUPPORTED,                  /* OLECMDID_CLOSE */
    0,0,0,
    OLECMDF_SUPPORTED,                  /* OLECMDID_SETPRINTTEMPLATE */
    OLECMDF_SUPPORTED                   /* OLECMDID_GETPRINTTEMPLATE */
};

static void test_OleCommandTarget(IUnknown *unk)
{
    IOleCommandTarget *cmdtrg;
    OLECMD cmds[OLECMDID_GETPRINTTEMPLATE];
    int i;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IOleCommandTarget, (void**)&cmdtrg);
    ok(hres == S_OK, "QueryInterface(IIDIOleM=CommandTarget failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    for(i=0; i<OLECMDID_GETPRINTTEMPLATE; i++) {
        cmds[i].cmdID = i+1;
        cmds[i].cmdf = 0xf0f0;
    }

    SET_EXPECT(QueryStatus_OPEN);
    SET_EXPECT(QueryStatus_NEW);
    hres = IOleCommandTarget_QueryStatus(cmdtrg, NULL, sizeof(cmds)/sizeof(cmds[0]), cmds, NULL);
    ok(hres == S_OK, "QueryStatus failed: %08lx\n", hres);
    CHECK_CALLED(QueryStatus_OPEN);
    CHECK_CALLED(QueryStatus_NEW);

    for(i=0; i<OLECMDID_GETPRINTTEMPLATE; i++) {
        ok(cmds[i].cmdID == i+1, "cmds[%d].cmdID canged to %lx\n", i, cmds[i].cmdID);
        ok(cmds[i].cmdf == expect_cmds[i+1], "cmds[%d].cmdf=%lx, expected %x\n",
                i+1, cmds[i].cmdf, expect_cmds[i+1]);
    }

    IOleCommandTarget_Release(cmdtrg);
}

static void test_OleCommandTarget_fail(IUnknown *unk)
{
    IOleCommandTarget *cmdtrg;
    int i;
    HRESULT hres;

    OLECMD cmd[2] = {
        {OLECMDID_OPEN, 0xf0f0},
        {OLECMDID_GETPRINTTEMPLATE+1, 0xf0f0}
    };

    hres = IUnknown_QueryInterface(unk, &IID_IOleCommandTarget, (void**)&cmdtrg);
    ok(hres == S_OK, "QueryInterface(IIDIOleM=CommandTarget failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IOleCommandTarget_QueryStatus(cmdtrg, NULL, 0, NULL, NULL);
    ok(hres == S_OK, "QueryStatus failed: %08lx\n", hres);

    SET_EXPECT(QueryStatus_OPEN);
    hres = IOleCommandTarget_QueryStatus(cmdtrg, NULL, 2, cmd, NULL);
    CHECK_CALLED(QueryStatus_OPEN);

    ok(hres == OLECMDERR_E_NOTSUPPORTED,
            "QueryStatus failed: %08lx, expected OLECMDERR_E_NOTSUPPORTED\n", hres);
    ok(cmd[1].cmdID == OLECMDID_GETPRINTTEMPLATE+1,
            "cmd[0].cmdID=%ld, expected OLECMDID_GETPRINTTEMPLATE+1\n", cmd[0].cmdID);
    ok(cmd[1].cmdf == 0, "cmd[0].cmdf=%lx, expected 0\n", cmd[0].cmdf);
    ok(cmd[0].cmdf == OLECMDF_SUPPORTED,
            "cmd[1].cmdf=%lx, expected OLECMDF_SUPPORTED\n", cmd[1].cmdf);

    hres = IOleCommandTarget_QueryStatus(cmdtrg, &IID_IHTMLDocument2, 2, cmd, NULL);
    ok(hres == OLECMDERR_E_UNKNOWNGROUP,
            "QueryStatus failed: %08lx, expected OLECMDERR_E_UNKNOWNGROUP\n", hres);

    for(i=0; i<OLECMDID_GETPRINTTEMPLATE; i++) {
        if(!expect_cmds[i]) {
            hres = IOleCommandTarget_Exec(cmdtrg, NULL, OLECMDID_UPDATECOMMANDS,
                    OLECMDEXECOPT_DODEFAULT, NULL, NULL);
            ok(hres == OLECMDERR_E_NOTSUPPORTED,
                    "Exec failed: %08lx, expected OLECMDERR_E_NOTSUPPORTED\n", hres);
        }
    }

    hres = IOleCommandTarget_Exec(cmdtrg, NULL, OLECMDID_GETPRINTTEMPLATE+1,
            OLECMDEXECOPT_DODEFAULT, NULL, NULL);
    ok(hres == OLECMDERR_E_NOTSUPPORTED,
            "Exec failed: %08lx, expected OLECMDERR_E_NOTSUPPORTED\n", hres);

    IOleCommandTarget_Release(cmdtrg);
}

static HWND create_container_window(void)
{
    static const WCHAR wszHTMLDocumentTest[] =
        {'H','T','M','L','D','o','c','u','m','e','n','t','T','e','s','t',0};
    static WNDCLASSEXW wndclass = {
        sizeof(WNDCLASSEXW),
        0,
        wnd_proc,
        0, 0, NULL, NULL, NULL, NULL, NULL,
        wszHTMLDocumentTest,
        NULL
    };

    RegisterClassExW(&wndclass);
    return CreateWindowW(wszHTMLDocumentTest, wszHTMLDocumentTest,
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, NULL, NULL, NULL, NULL);
}

static HRESULT test_DoVerb(IOleObject *oleobj)
{
    RECT rect = {0,0,500,500};
    HRESULT hres;

    SET_EXPECT(GetContainer);
    SET_EXPECT(LockContainer);
    SET_EXPECT(ActivateMe);
    expect_LockContainer_fLock = TRUE;

    hres = IOleObject_DoVerb(oleobj, OLEIVERB_SHOW, NULL, &ClientSite, -1, container_hwnd, &rect);
    if(FAILED(hres))
        return hres;

    ok(hres == S_OK, "DoVerb failed: %08lx\n", hres);
    CHECK_CALLED(GetContainer);
    CHECK_CALLED(LockContainer);
    CHECK_CALLED(ActivateMe);

    return hres;
}

#define CLIENTSITE_EXPECTPATH 0x00000001
#define CLIENTSITE_SETNULL    0x00000002
#define CLIENTSITE_DONTSET    0x00000004

static void test_ClientSite(IOleObject *oleobj, DWORD flags)
{
    IOleClientSite *clientsite;
    HRESULT hres;

    if(flags & CLIENTSITE_SETNULL) {
        hres = IOleObject_GetClientSite(oleobj, &clientsite);
        ok(clientsite == &ClientSite, "clientsite=%p, expected %p\n", clientsite, &ClientSite);

        hres = IOleObject_SetClientSite(oleobj, NULL);
        ok(hres == S_OK, "SetClientSite failed: %08lx\n", hres);
    }

    if(flags & CLIENTSITE_DONTSET)
        return;

    hres = IOleObject_GetClientSite(oleobj, &clientsite);
    ok(hres == S_OK, "GetClientSite failed: %08lx\n", hres);
    ok(clientsite == NULL, "GetClientSite() = %p, expected NULL\n", clientsite);

    SET_EXPECT(GetHostInfo);
    if(flags & CLIENTSITE_EXPECTPATH) {
        SET_EXPECT(GetOptionKeyPath);
        SET_EXPECT(GetOverrideKeyPath);
    }
    SET_EXPECT(GetWindow);
    SET_EXPECT(QueryStatus_SETPROGRESSTEXT);
    SET_EXPECT(Exec_SETPROGRESSMAX);
    SET_EXPECT(Exec_SETPROGRESSPOS);
    hres = IOleObject_SetClientSite(oleobj, &ClientSite);
    ok(hres == S_OK, "SetClientSite failed: %08lx\n", hres);
    CHECK_CALLED(GetHostInfo);
    if(flags & CLIENTSITE_EXPECTPATH) {
        CHECK_CALLED(GetOptionKeyPath);
        CHECK_CALLED(GetOverrideKeyPath);
    }
    CHECK_CALLED(GetWindow);
    CHECK_CALLED(QueryStatus_SETPROGRESSTEXT);
    CHECK_CALLED(Exec_SETPROGRESSMAX);
    CHECK_CALLED(Exec_SETPROGRESSPOS);

    hres = IOleObject_GetClientSite(oleobj, &clientsite);
    ok(hres == S_OK, "GetClientSite failed: %08lx\n", hres);
    ok(clientsite == &ClientSite, "GetClientSite() = %p, expected %p\n", clientsite, &ClientSite);
}

static void test_Close(IUnknown *unk, BOOL set_client)
{
    IOleObject *oleobj = NULL;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "QueryInterface(IID_IOleObject) failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    SET_EXPECT(GetContainer);
    SET_EXPECT(LockContainer);
    expect_LockContainer_fLock = FALSE;
    hres = IOleObject_Close(oleobj, OLECLOSE_NOSAVE);
    ok(hres == S_OK, "Close failed: %08lx\n", hres);
    CHECK_CALLED(GetContainer);
    CHECK_CALLED(LockContainer);

    if(set_client)
        test_ClientSite(oleobj, CLIENTSITE_SETNULL|CLIENTSITE_DONTSET);

    IOleObject_Release(oleobj);
}

static void test_InPlaceDeactivate(IUnknown *unk, BOOL expect_call)
{
    IOleInPlaceObjectWindowless *windowlessobj = NULL;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IOleInPlaceObjectWindowless,
            (void**)&windowlessobj);
    ok(hres == S_OK, "QueryInterface(IID_IOleInPlaceObjectWindowless) failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    if(expect_call) SET_EXPECT(OnInPlaceDeactivate);
    hres = IOleInPlaceObjectWindowless_InPlaceDeactivate(windowlessobj);
    ok(hres == S_OK, "InPlaceDeactivate failed: %08lx\n", hres);
    if(expect_call) CHECK_CALLED(OnInPlaceDeactivate);

    IOleInPlaceObjectWindowless_Release(windowlessobj);
}

static HRESULT test_Activate(IUnknown *unk, DWORD flags)
{
    IOleObject *oleobj = NULL;
    GUID guid;
    HRESULT hres;

    last_hwnd = hwnd;

    if(view)
        IOleDocumentView_Release(view);
    view = NULL;

    hres = IUnknown_QueryInterface(unk, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "QueryInterface(IID_IOleObject) failed: %08lx\n", hres);
    if(FAILED(hres))
        return hres;

    hres = IOleObject_GetUserClassID(oleobj, NULL);
    ok(hres == E_INVALIDARG, "GetUserClassID returned: %08lx, expected E_INVALIDARG\n", hres);

    hres = IOleObject_GetUserClassID(oleobj, &guid);
    ok(hres == S_OK, "GetUserClassID failed: %08lx\n", hres);
    ok(IsEqualGUID(&guid, &CLSID_HTMLDocument), "guid != CLSID_HTMLDocument\n");

    test_ClientSite(oleobj, flags);
    test_InPlaceDeactivate(unk, FALSE);

    hres = test_DoVerb(oleobj);

    IOleObject_Release(oleobj);
    return hres;
}

static void test_Window(IUnknown *unk, BOOL expect_success)
{
    IOleInPlaceActiveObject *activeobject = NULL;
    HWND tmp_hwnd;
    HRESULT hres;

    hres = IOleDocumentView_QueryInterface(view, &IID_IOleInPlaceActiveObject, (void**)&activeobject);
    ok(hres == S_OK, "Could not get IOleInPlaceActiveObject interface: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IOleInPlaceActiveObject_GetWindow(activeobject, &tmp_hwnd);

    if(expect_success) {
        ok(hres == S_OK, "GetWindow failed: %08lx\n", hres);
        ok(tmp_hwnd == hwnd, "tmp_hwnd=%p, expected %p\n", tmp_hwnd, hwnd);
    }else {
        ok(hres == E_FAIL, "GetWindow returned %08lx, expected E_FAIL\n", hres);
        ok(IsWindow(hwnd), "hwnd is destroyed\n");
    }

    IOleInPlaceActiveObject_Release(activeobject);
}

static void test_CloseView(void)
{
    IOleInPlaceSite *inplacesite = (IOleInPlaceSite*)0xff00ff00;
    HRESULT hres;

    if(!view)
        return;

    hres = IOleDocumentView_Show(view, FALSE);
    ok(hres == S_OK, "Show failed: %08lx\n", hres);

    hres = IOleDocumentView_CloseView(view, 0);
    ok(hres == S_OK, "CloseView failed: %08lx\n", hres);

    hres = IOleDocumentView_SetInPlaceSite(view, NULL);
    ok(hres == S_OK, "SetInPlaceSite failed: %08lx\n", hres);

    hres = IOleDocumentView_GetInPlaceSite(view, &inplacesite);
    ok(hres == S_OK, "SetInPlaceSite failed: %08lx\n", hres);
    ok(inplacesite == NULL, "inplacesite=%p, expected NULL\n", inplacesite);
}

static void test_UIDeactivate(void)
{
    HRESULT hres;

    if(call_UIActivate) {
        SET_EXPECT(SetActiveObject);
        SET_EXPECT(HideUI);
        SET_EXPECT(OnUIDeactivate);
    }

    expect_SetActiveObject_active = FALSE;
    hres = IOleDocumentView_UIActivate(view, FALSE);
    ok(hres == S_OK, "UIActivate failed: %08lx\n", hres);

    if(call_UIActivate) {
        CHECK_CALLED(SetActiveObject);
        CHECK_CALLED(HideUI);
        CHECK_CALLED(OnUIDeactivate);
    }
}

static void test_Hide(void)
{
    HRESULT hres;

    if(!view)
        return;

    hres = IOleDocumentView_Show(view, FALSE);
    ok(hres == S_OK, "Show failed: %08lx\n", hres);
}

static void test_HTMLDocument(void)
{
    IUnknown *unk;
    HRESULT hres;
    ULONG ref;

    hres = CoCreateInstance(&CLSID_HTMLDocument, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "CoCreateInstance failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    test_Persist(unk);

    hres = test_Activate(unk, CLIENTSITE_EXPECTPATH);
    if(FAILED(hres)) {
        IUnknown_Release(unk);
        return;
    }

    test_OleCommandTarget_fail(unk);
    test_OleCommandTarget(unk);
    test_Window(unk, TRUE);
    test_UIDeactivate();
    test_OleCommandTarget(unk);
    test_Window(unk, TRUE);
    test_InPlaceDeactivate(unk, TRUE);

    /* Calling test_OleCommandTarget here couses Segmentation Fault with native
     * MSHTML. It doesn't with Wine. */

    test_Window(unk, FALSE);
    test_Hide();
    test_InPlaceDeactivate(unk, FALSE);
    test_CloseView();
    test_Close(unk, FALSE);

    /* Activate HTMLDocument again */
    test_Activate(unk, CLIENTSITE_SETNULL);
    test_Window(unk, TRUE);
    test_OleCommandTarget(unk);
    test_UIDeactivate();
    test_InPlaceDeactivate(unk, TRUE);
    test_Close(unk, FALSE);

    /* Activate HTMLDocument again, this time without UIActivate */
    call_UIActivate = FALSE;
    test_Activate(unk, CLIENTSITE_SETNULL);
    test_Window(unk, TRUE);
    test_UIDeactivate();
    test_InPlaceDeactivate(unk, TRUE);
    test_CloseView();
    test_CloseView();
    test_Close(unk, TRUE);

    if(view)
        IOleDocumentView_Release(view);
    view = NULL;

    ok(IsWindow(hwnd), "hwnd is destroyed\n");

    ref = IUnknown_Release(unk);
    ok(ref == 0, "ref=%ld, expected 0\n", ref);

    ok(!IsWindow(hwnd), "hwnd is not destroyed\n");

}

START_TEST(htmldoc)
{
    CoInitialize(NULL);
    container_hwnd = create_container_window();

    test_HTMLDocument();

    DestroyWindow(container_hwnd);
    CoUninitialize();
}
