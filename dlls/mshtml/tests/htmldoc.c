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

static HRESULT QueryInterface(REFIID riid, void **ppv);

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
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
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

static HRESULT WINAPI DocumentSite_ActivateMe(IOleDocumentSite *iface, IOleDocumentView *pViewToActivate)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
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

static BOOL expect_GetHostInfo = FALSE, called_GetHostInfo = FALSE;
static HRESULT WINAPI DocHostUIHandler_GetHostInfo(IDocHostUIHandler2 *iface, DOCHOSTUIINFO *pInfo)
{
    ok(expect_GetHostInfo, "unexpected call\n");
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
    called_GetHostInfo = TRUE;
    expect_GetHostInfo = FALSE;
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_ShowUI(IDocHostUIHandler2 *iface, DWORD dwID,
        IOleInPlaceActiveObject *pActiveObject, IOleCommandTarget *pCommandTarget,
        IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_HideUI(IDocHostUIHandler2 *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
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

static BOOL expect_GetOptionKeyPath = FALSE, called_GetOptionKeyPath = FALSE;
static HRESULT WINAPI DocHostUIHandler_GetOptionKeyPath(IDocHostUIHandler2 *iface,
        LPOLESTR *pchKey, DWORD dw)
{
    ok(expect_GetOptionKeyPath, "unexpected call\n");
    ok(pchKey != NULL, "pchKey = NULL\n");
    ok(!dw, "dw=%ld, expected 0\n", dw);
    if(pchKey)
        ok(!*pchKey, "*pchKey=%p, expected NULL\n", *pchKey);
    called_GetOptionKeyPath = TRUE;
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

static BOOL expect_GetOverrideKeyPath = FALSE, called_GetOverrideKeyPath = FALSE;
static HRESULT WINAPI DocHostUIHandler_GetOverrideKeyPath(IDocHostUIHandler2 *iface,
        LPOLESTR *pchKey, DWORD dw)
{
    ok(expect_GetOverrideKeyPath, "unexpected call\n");
    ok(pchKey != NULL, "pchKey = NULL\n");
    if(pchKey)
        ok(!*pchKey, "*pchKey=%p, expected NULL\n", *pchKey);
    ok(!dw, "dw=%ld, xepected 0\n", dw);
    called_GetOverrideKeyPath = TRUE;
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

static HRESULT QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IOleClientSite, riid))
        *ppv = &ClientSite;
    else if(IsEqualGUID(&IID_IOleDocumentSite, riid))
        *ppv = &DocumentSite;
    else if(IsEqualGUID(&IID_IDocHostUIHandler, riid) || IsEqualGUID(&IID_IDocHostUIHandler2, riid))
        *ppv = &DocHostUIHandler;

    /* TODO:
     * IDispatch
     * IServiceProvider
     * IOleCommandTarget
     * IOleWindow
     * {D48A6EC6-6A4A-11CF-94A7-444553540000}
     * {7BB0B520-B1A7-11D2-BB23-00C04F79ABCD}
     * {000670BA-0000-0000-C000-000000000046}
     */

    if(*ppv)
        return S_OK;
    return E_NOINTERFACE;
}

static void test_Persist(IUnknown *punk)
{
    IPersistMoniker *persist_mon;
    IPersistFile *persist_file;
    GUID guid;
    HRESULT hres;

    hres = IUnknown_QueryInterface(punk, &IID_IPersistFile, (void**)&persist_file);
    ok(hres == S_OK, "QueryInterface(IID_IPersist) failed: %08lx\n", hres);
    if(SUCCEEDED(hres)) {
        hres = IPersist_GetClassID(persist_file, NULL);
        ok(hres == E_INVALIDARG, "GetClassID returned: %08lx, expected E_INVALIDARG\n", hres);

        hres = IPersist_GetClassID(persist_file, &guid);
        ok(hres == S_OK, "GetClassID failed: %08lx\n", hres);
        ok(IsEqualGUID(&CLSID_HTMLDocument, &guid), "guid != CLSID_HTMLDocument\n");

        IPersist_Release(persist_file);
    }

    hres = IUnknown_QueryInterface(punk, &IID_IPersistMoniker, (void**)&persist_mon);
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

static void test_OleObj(IUnknown *punk)
{
    IOleObject *oleobj;
    IOleClientSite *clientsite = (LPVOID)0xdeadbeef;
    HRESULT hres;
    GUID guid;

    hres = IUnknown_QueryInterface(punk, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "QueryInterface(IID_IOleObject) failed: %08lx\n", hres);
    if(SUCCEEDED(hres)) {
        hres = IOleObject_GetUserClassID(oleobj, NULL);
        ok(hres == E_INVALIDARG, "GetUserClassID returned: %08lx, expected E_INVALIDARG\n", hres);

        hres = IOleObject_GetUserClassID(oleobj, &guid);
        ok(hres == S_OK, "GetUserClassID failed: %08lx\n", hres);
        ok(IsEqualGUID(&guid, &CLSID_HTMLDocument), "guid != CLSID_HTMLDocument\n");

        hres = IOleObject_GetClientSite(oleobj, &clientsite);
        ok(hres == S_OK, "GetClientSite failed: %08lx\n", hres);
        ok(clientsite == NULL, "GetClientSite() = %p, expected NULL\n", clientsite);

        expect_GetHostInfo = TRUE;
        expect_GetOptionKeyPath = TRUE;
        expect_GetOverrideKeyPath = TRUE;
        hres = IOleObject_SetClientSite(oleobj, &ClientSite);
        ok(hres == S_OK, "SetClientSite failed: %08lx\n", hres);
        ok(called_GetHostInfo, "expected GetHostInfo\n");
        ok(called_GetOptionKeyPath, "expected GetOptionKeyPath\n");
        ok(called_GetOverrideKeyPath, "expected GetOverrideKeyPath\n");
        expect_GetHostInfo = called_GetHostInfo = FALSE;
        expect_GetOptionKeyPath = called_GetOptionKeyPath = FALSE;
        expect_GetOverrideKeyPath = called_GetOverrideKeyPath = FALSE;

        hres = IOleObject_GetClientSite(oleobj, &clientsite);
        ok(hres == S_OK, "GetClientSite failed: %08lx\n", hres);
        ok(clientsite == &ClientSite, "GetClientSite() = %p, expected %p\n", clientsite, &ClientSite);

        IOleObject_Release(oleobj);
    }
}

static void test_HTMLDocument()
{
    IUnknown *htmldoc_unk = NULL;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_HTMLDocument, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IUnknown, (void**)&htmldoc_unk);
    ok(hres == S_OK, "CoCreateInstance failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    test_Persist(htmldoc_unk);
    test_OleObj(htmldoc_unk);
    
    IUnknown_Release(htmldoc_unk);
}

START_TEST(htmldoc)
{
    CoInitialize(NULL);

    test_HTMLDocument();

    CoUninitialize();
}
