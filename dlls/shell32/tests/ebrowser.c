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
#define CONST_VTABLE

#include "shlobj.h"
#include "shlwapi.h"

#include "wine/test.h"

#include "initguid.h"
#include "mshtml.h"

static HWND hwnd;

static HRESULT (WINAPI *pSHCreateShellItem)(LPCITEMIDLIST,IShellFolder*,LPCITEMIDLIST,IShellItem**);

static void init_function_pointers(void)
{
    HMODULE hmod;

    hmod = GetModuleHandleA("shell32.dll");
    pSHCreateShellItem = (void*)GetProcAddress(hmod, "SHCreateShellItem");
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
    SetRect(&rc, 0, 0, 500, 500);
    return IExplorerBrowser_Initialize(peb, hwnd, &rc, NULL);
}

static HRESULT ebrowser_browse_to_desktop(IExplorerBrowser *peb)
{
    LPITEMIDLIST pidl_desktop;
    HRESULT hr;
    SHGetSpecialFolderLocation (hwnd, CSIDL_DESKTOP, &pidl_desktop);
    hr = IExplorerBrowser_BrowseToIDList(peb, pidl_desktop, 0);
    ILFree(pidl_desktop);
    return hr;
}

/* Process some messages */
static void process_msgs(void)
{
    MSG msg;
    while(PeekMessageA( &msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

/*********************************************************************
 * IExplorerBrowserEvents implementation
 */
typedef struct {
    IExplorerBrowserEvents IExplorerBrowserEvents_iface;
    LONG ref;
    UINT pending, created, completed, failed;
} IExplorerBrowserEventsImpl;

static IExplorerBrowserEventsImpl ebev;

static inline IExplorerBrowserEventsImpl *impl_from_IExplorerBrowserEvents(IExplorerBrowserEvents *iface)
{
    return CONTAINING_RECORD(iface, IExplorerBrowserEventsImpl, IExplorerBrowserEvents_iface);
}

static HRESULT WINAPI IExplorerBrowserEvents_fnQueryInterface(IExplorerBrowserEvents *iface,
                                                              REFIID riid, void **ppvObj)
{
    ok(0, "Never called.\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI IExplorerBrowserEvents_fnAddRef(IExplorerBrowserEvents *iface)
{
    IExplorerBrowserEventsImpl *This = impl_from_IExplorerBrowserEvents(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IExplorerBrowserEvents_fnRelease(IExplorerBrowserEvents *iface)
{
    IExplorerBrowserEventsImpl *This = impl_from_IExplorerBrowserEvents(iface);
    return InterlockedDecrement(&This->ref);
}

static HRESULT WINAPI IExplorerBrowserEvents_fnOnNavigationPending(IExplorerBrowserEvents *iface,
                                                                   PCIDLIST_ABSOLUTE pidlFolder)
{
    IExplorerBrowserEventsImpl *This = impl_from_IExplorerBrowserEvents(iface);
    This->pending++;
    return S_OK;
}

static HRESULT WINAPI IExplorerBrowserEvents_fnOnNavigationComplete(IExplorerBrowserEvents *iface,
                                                                    PCIDLIST_ABSOLUTE pidlFolder)
{
    IExplorerBrowserEventsImpl *This = impl_from_IExplorerBrowserEvents(iface);
    This->completed++;
    return S_OK;
}
static HRESULT WINAPI IExplorerBrowserEvents_fnOnNavigationFailed(IExplorerBrowserEvents *iface,
                                                                  PCIDLIST_ABSOLUTE pidlFolder)
{
    IExplorerBrowserEventsImpl *This = impl_from_IExplorerBrowserEvents(iface);
    This->failed++;
    return S_OK;
}
static HRESULT WINAPI IExplorerBrowserEvents_fnOnViewCreated(IExplorerBrowserEvents *iface,
                                                             IShellView *psv)
{
    IExplorerBrowserEventsImpl *This = impl_from_IExplorerBrowserEvents(iface);
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

/*********************************************************************
 * IExplorerPaneVisibility implementation
 */
typedef struct
{
    IExplorerPaneVisibility IExplorerPaneVisibility_iface;
    LONG ref;
    LONG count;
    LONG np, cp, cp_o, cp_v, dp, pp, qp, aqp, unk; /* The panes */
} IExplorerPaneVisibilityImpl;

static inline IExplorerPaneVisibilityImpl *impl_from_IExplorerPaneVisibility(IExplorerPaneVisibility *iface)
{
    return CONTAINING_RECORD(iface, IExplorerPaneVisibilityImpl, IExplorerPaneVisibility_iface);
}

static HRESULT WINAPI IExplorerPaneVisibility_fnQueryInterface(IExplorerPaneVisibility *iface,
                                                               REFIID riid, LPVOID *ppvObj)
{
    ok(0, "unexpected, %s\n", wine_dbgstr_guid(riid));
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IExplorerPaneVisibility_fnAddRef(IExplorerPaneVisibility *iface)
{
    IExplorerPaneVisibilityImpl *This = impl_from_IExplorerPaneVisibility(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IExplorerPaneVisibility_fnRelease(IExplorerPaneVisibility *iface)
{
    IExplorerPaneVisibilityImpl *This = impl_from_IExplorerPaneVisibility(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    if(!ref)
        free(This);

    return ref;
}

static HRESULT WINAPI IExplorerPaneVisibility_fnGetPaneState(IExplorerPaneVisibility *iface,
                                                             REFEXPLORERPANE ep,
                                                             EXPLORERPANESTATE *peps)
{
    IExplorerPaneVisibilityImpl *This = impl_from_IExplorerPaneVisibility(iface);
    This->count++;

    ok(ep != NULL, "ep is NULL.\n");
    ok(peps != NULL, "peps is NULL.\n");
    ok(*peps == 0, "got %ld\n", *peps);

    *peps = EPS_FORCE;
    if(IsEqualGUID(&EP_NavPane, ep))                 This->np++;
    else if(IsEqualGUID(&EP_Commands, ep))           This->cp++;
    else if(IsEqualGUID(&EP_Commands_Organize, ep))  This->cp_o++;
    else if(IsEqualGUID(&EP_Commands_View, ep))      This->cp_v++;
    else if(IsEqualGUID(&EP_DetailsPane, ep))        This->dp++;
    else if(IsEqualGUID(&EP_PreviewPane, ep))        This->pp++;
    else if(IsEqualGUID(&EP_QueryPane, ep))          This->qp++;
    else if(IsEqualGUID(&EP_AdvQueryPane, ep))       This->aqp++;
    else
    {
        trace("Unknown explorer pane: %s\n", wine_dbgstr_guid(ep));
        This->unk++;
    }

    return S_OK;
}

static const IExplorerPaneVisibilityVtbl epvvt =
{
    IExplorerPaneVisibility_fnQueryInterface,
    IExplorerPaneVisibility_fnAddRef,
    IExplorerPaneVisibility_fnRelease,
    IExplorerPaneVisibility_fnGetPaneState
};

static IExplorerPaneVisibilityImpl *create_explorerpanevisibility(void)
{
    IExplorerPaneVisibilityImpl *epv;

    epv = calloc(1, sizeof(*epv));
    epv->IExplorerPaneVisibility_iface.lpVtbl = &epvvt;
    epv->ref = 1;

    return epv;
}

/*********************************************************************
 * ICommDlgBrowser3 implementation
 */
typedef struct
{
    ICommDlgBrowser3 ICommDlgBrowser3_iface;
    LONG ref;
    UINT OnDefaultCommand, OnStateChange, IncludeObject;
    UINT Notify, GetDefaultMenuText, GetViewFlags;
    UINT OnColumnClicked, GetCurrentFilter, OnPreviewCreated;
} ICommDlgBrowser3Impl;

static inline ICommDlgBrowser3Impl *impl_from_ICommDlgBrowser3(ICommDlgBrowser3 *iface)
{
    return CONTAINING_RECORD(iface, ICommDlgBrowser3Impl, ICommDlgBrowser3_iface);
}

static HRESULT WINAPI ICommDlgBrowser3_fnQueryInterface(ICommDlgBrowser3 *iface, REFIID riid, LPVOID *ppvObj)
{
    ok(0, "unexpected %s\n", wine_dbgstr_guid(riid));
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ICommDlgBrowser3_fnAddRef(ICommDlgBrowser3 *iface)
{
    ICommDlgBrowser3Impl *This = impl_from_ICommDlgBrowser3(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ICommDlgBrowser3_fnRelease(ICommDlgBrowser3 *iface)
{
    ICommDlgBrowser3Impl *This = impl_from_ICommDlgBrowser3(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    if(!ref)
        free(This);

    return ref;
}

static HRESULT WINAPI ICommDlgBrowser3_fnOnDefaultCommand(ICommDlgBrowser3* iface, IShellView *shv)
{
    ICommDlgBrowser3Impl *This = impl_from_ICommDlgBrowser3(iface);
    This->OnDefaultCommand++;
    return E_NOTIMPL;
}

static HRESULT WINAPI ICommDlgBrowser3_fnOnStateChange(
    ICommDlgBrowser3* iface,
    IShellView *shv,
    ULONG uChange)
{
    ICommDlgBrowser3Impl *This = impl_from_ICommDlgBrowser3(iface);
    This->OnStateChange++;
    return E_NOTIMPL;
}

static HRESULT WINAPI ICommDlgBrowser3_fnIncludeObject(
    ICommDlgBrowser3* iface,
    IShellView *shv,
    LPCITEMIDLIST pidl)
{
    ICommDlgBrowser3Impl *This = impl_from_ICommDlgBrowser3(iface);
    This->IncludeObject++;
    return S_OK;
}

static HRESULT WINAPI ICommDlgBrowser3_fnNotify(
    ICommDlgBrowser3* iface,
    IShellView *ppshv,
    DWORD dwNotifyType)
{
    ICommDlgBrowser3Impl *This = impl_from_ICommDlgBrowser3(iface);
    This->Notify++;
    return E_NOTIMPL;
}

static HRESULT WINAPI ICommDlgBrowser3_fnGetDefaultMenuText(
    ICommDlgBrowser3* iface,
    IShellView *ppshv,
    LPWSTR pszText,
    int cchMax)
{
    ICommDlgBrowser3Impl *This = impl_from_ICommDlgBrowser3(iface);
    This->GetDefaultMenuText++;
    return E_NOTIMPL;
}

static HRESULT WINAPI ICommDlgBrowser3_fnGetViewFlags(
    ICommDlgBrowser3* iface,
    DWORD *pdwFlags)
{
    ICommDlgBrowser3Impl *This = impl_from_ICommDlgBrowser3(iface);
    This->GetViewFlags++;
    return E_NOTIMPL;
}

static HRESULT WINAPI ICommDlgBrowser3_fnOnColumnClicked(
    ICommDlgBrowser3* iface,
    IShellView *ppshv,
    int iColumn)
{
    ICommDlgBrowser3Impl *This = impl_from_ICommDlgBrowser3(iface);
    This->OnColumnClicked++;
    return E_NOTIMPL;
}

static HRESULT WINAPI ICommDlgBrowser3_fnGetCurrentFilter(
    ICommDlgBrowser3* iface,
    LPWSTR pszFileSpec,
    int cchFileSpec)
{
    ICommDlgBrowser3Impl *This = impl_from_ICommDlgBrowser3(iface);
    This->GetCurrentFilter++;
    return E_NOTIMPL;
}

static HRESULT WINAPI ICommDlgBrowser3_fnOnPreviewCreated(
    ICommDlgBrowser3* iface,
    IShellView *ppshv)
{
    ICommDlgBrowser3Impl *This = impl_from_ICommDlgBrowser3(iface);
    This->OnPreviewCreated++;
    return E_NOTIMPL;
}

static const ICommDlgBrowser3Vtbl cdbvtbl =
{
    ICommDlgBrowser3_fnQueryInterface,
    ICommDlgBrowser3_fnAddRef,
    ICommDlgBrowser3_fnRelease,
    ICommDlgBrowser3_fnOnDefaultCommand,
    ICommDlgBrowser3_fnOnStateChange,
    ICommDlgBrowser3_fnIncludeObject,
    ICommDlgBrowser3_fnNotify,
    ICommDlgBrowser3_fnGetDefaultMenuText,
    ICommDlgBrowser3_fnGetViewFlags,
    ICommDlgBrowser3_fnOnColumnClicked,
    ICommDlgBrowser3_fnGetCurrentFilter,
    ICommDlgBrowser3_fnOnPreviewCreated
};

static ICommDlgBrowser3Impl *create_commdlgbrowser3(void)
{
    ICommDlgBrowser3Impl *cdb;

    cdb = calloc(1, sizeof(*cdb));
    cdb->ICommDlgBrowser3_iface.lpVtbl = &cdbvtbl;
    cdb->ref = 1;

    return cdb;
}

/*********************************************************************
 * IServiceProvider Implementation
 */
typedef struct {
    IServiceProvider IServiceProvider_iface;
    LONG ref;
    struct services {
        REFGUID service;
        REFIID id;
        void *punk;
    } *interfaces;
} IServiceProviderImpl;

static inline IServiceProviderImpl *impl_from_IServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, IServiceProviderImpl, IServiceProvider_iface);
}

static HRESULT WINAPI IServiceProvider_fnQueryInterface(IServiceProvider *iface, REFIID riid, LPVOID *ppvObj)
{
    *ppvObj = NULL;
    if(IsEqualIID(riid, &IID_IServiceProvider))
    {
        *ppvObj = iface;
        IServiceProvider_AddRef(iface);
        return S_OK;
    }

    if(IsEqualIID(riid, &IID_IOleCommandTarget))
    {
        /* Windows Vista. */
        return E_NOINTERFACE;
    }

    ok(0, "Unexpected interface requested, %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI IServiceProvider_fnAddRef(IServiceProvider *iface)
{
    IServiceProviderImpl *This = impl_from_IServiceProvider(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IServiceProvider_fnRelease(IServiceProvider *iface)
{
    IServiceProviderImpl *This = impl_from_IServiceProvider(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    if(!ref)
        free(This);

    return ref;
}

static HRESULT WINAPI IServiceProvider_fnQueryService(IServiceProvider *iface,
                                                      REFGUID guidService,
                                                      REFIID riid,
                                                      void **ppv)
{
    IServiceProviderImpl *This = impl_from_IServiceProvider(iface);
    UINT i;

    if (winetest_debug > 1)
            trace("QueryService(service %s, iid %s)\n", debugstr_guid(guidService), debugstr_guid(riid));

    for(i = 0; This->interfaces[i].service != NULL; i++)
    {
        if(IsEqualGUID(This->interfaces[i].service, guidService) &&
           IsEqualIID(This->interfaces[i].id, riid))
        {
            *ppv = This->interfaces[i].punk;
            IUnknown_AddRef((IUnknown *)*ppv);
            return S_OK;
        }
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static const IServiceProviderVtbl spvtbl =
{
    IServiceProvider_fnQueryInterface,
    IServiceProvider_fnAddRef,
    IServiceProvider_fnRelease,
    IServiceProvider_fnQueryService
};

static IServiceProviderImpl *create_serviceprovider(void)
{
    IServiceProviderImpl *sp = malloc(sizeof(*sp));
    sp->IServiceProvider_iface.lpVtbl = &spvtbl;
    sp->ref = 1;
    return sp;
}

static void test_QueryInterface(void)
{
    IExplorerBrowser *peb;
    IUnknown *punk;
    HRESULT hr;
    LONG lres;

    hr = ebrowser_instantiate(&peb);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);

#define test_qinterface(iid, exp)                                       \
    do {                                                                \
        hr = IExplorerBrowser_QueryInterface(peb, &iid, (void**)&punk); \
        ok(hr == exp, "(%s:)Expected (0x%08lx), got (0x%08lx)\n",         \
           #iid, exp, hr);                                              \
        if(SUCCEEDED(hr)) IUnknown_Release(punk);                       \
    } while(0)

    test_qinterface(IID_IUnknown, S_OK);
    test_qinterface(IID_IExplorerBrowser, S_OK);
    test_qinterface(IID_IShellBrowser, S_OK);
    test_qinterface(IID_IOleWindow, S_OK);
    test_qinterface(IID_ICommDlgBrowser, S_OK);
    test_qinterface(IID_ICommDlgBrowser2, S_OK);
    test_qinterface(IID_ICommDlgBrowser3, S_OK);
    todo_wine test_qinterface(IID_IServiceProvider, S_OK);
    test_qinterface(IID_IObjectWithSite, S_OK);
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
    ok(lres == 0, "Got %ld\n", lres);
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
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    if(FAILED(hr))
    {
        skip("Failed to get IShellBrowser interface.\n");
        return;
    }

    /* Some unimplemented methods */
    retHwnd = (HWND)0xdeadbeef;
    hr = IShellBrowser_GetControlWindow(psb, FCW_TOOLBAR, &retHwnd);
    ok(hr == E_NOTIMPL, "got (0x%08lx)\n", hr);
    ok(retHwnd == NULL || broken(retHwnd == (HWND)0xdeadbeef), "got %p\n", retHwnd);

    retHwnd = (HWND)0xdeadbeef;
    hr = IShellBrowser_GetControlWindow(psb, FCW_STATUS, &retHwnd);
    ok(hr == E_NOTIMPL, "got (0x%08lx)\n", hr);
    ok(retHwnd == NULL || broken(retHwnd == (HWND)0xdeadbeef), "got %p\n", retHwnd);

    retHwnd = (HWND)0xdeadbeef;
    hr = IShellBrowser_GetControlWindow(psb, FCW_TREE, &retHwnd);
    ok(hr == E_NOTIMPL, "got (0x%08lx)\n", hr);
    ok(retHwnd == NULL || broken(retHwnd == (HWND)0xdeadbeef), "got %p\n", retHwnd);

    retHwnd = (HWND)0xdeadbeef;
    hr = IShellBrowser_GetControlWindow(psb, FCW_PROGRESS, &retHwnd);
    ok(hr == E_NOTIMPL, "got (0x%08lx)\n", hr);
    ok(retHwnd == NULL || broken(retHwnd == (HWND)0xdeadbeef), "got %p\n", retHwnd);

    /* ::InsertMenuSB */
    hr = IShellBrowser_InsertMenusSB(psb, NULL, NULL);
    ok(hr == E_NOTIMPL, "got (0x%08lx)\n", hr);

    /* ::RemoveMenusSB */
    hr = IShellBrowser_RemoveMenusSB(psb, NULL);
    ok(hr == E_NOTIMPL, "got (0x%08lx)\n", hr);

    /* ::SetMenuSB */
    hr = IShellBrowser_SetMenuSB(psb, NULL, NULL, NULL);
    ok(hr == E_NOTIMPL, "got (0x%08lx)\n", hr);

    /***** Before EB::Initialize *****/

    /* ::GetWindow */
    retHwnd = (HWND)0xDEADBEEF;
    hr = IShellBrowser_GetWindow(psb, &retHwnd);
    ok(hr == E_FAIL, "got (0x%08lx)\n", hr);
    ok(retHwnd == (HWND)0xDEADBEEF, "HWND overwritten\n");

    todo_wine
    {

        /* ::SendControlMsg */
        lres = 0xDEADBEEF;
        hr = IShellBrowser_SendControlMsg(psb, FCW_STATUS, 0, 0, 0, &lres);
        ok(hr == S_OK, "got (0x%08lx)\n", hr);
        ok(lres == 0, "lres was %Id\n", lres);

        lres = 0xDEADBEEF;
        hr = IShellBrowser_SendControlMsg(psb, FCW_TOOLBAR, TB_CHECKBUTTON,
                                          FCIDM_TB_SMALLICON, TRUE, &lres);
        ok(hr == S_OK, "got (0x%08lx)\n", hr);
        ok(lres == 0, "lres was %Id\n", lres);

        hr = IShellBrowser_SendControlMsg(psb, FCW_STATUS, 0, 0, 0, NULL);
        ok(hr == S_OK, "got (0x%08lx)\n", hr);

        hr = IShellBrowser_SendControlMsg(psb, FCW_TREE, 0, 0, 0, NULL);
        ok(hr == S_OK, "got (0x%08lx)\n", hr);

        hr = IShellBrowser_SendControlMsg(psb, FCW_PROGRESS, 0, 0, 0, NULL);
        ok(hr == S_OK, "got (0x%08lx)\n", hr);
    }

    /* ::QueryActiveShellView */
    hr = IShellBrowser_QueryActiveShellView(psb, (IShellView**)&punk);
    ok(hr == E_FAIL, "got (0x%08lx)\n", hr);

    /* Initialize ExplorerBrowser */
    ebrowser_initialize(peb);

    /***** After EB::Initialize *****/

    /* ::GetWindow */
    hr = IShellBrowser_GetWindow(psb, &retHwnd);
    ok(hr == S_OK, "got (0x%08lx)\n", hr);
    ok(GetParent(retHwnd) == hwnd, "The HWND returned is not our child.\n");

    todo_wine
    {
        /* ::SendControlMsg */
        hr = IShellBrowser_SendControlMsg(psb, FCW_STATUS, 0, 0, 0, NULL);
        ok(hr == S_OK, "got (0x%08lx)\n", hr);

        lres = 0xDEADBEEF;
        hr = IShellBrowser_SendControlMsg(psb, FCW_TOOLBAR, 0, 0, 0, &lres);
        ok(hr == S_OK, "got (0x%08lx)\n", hr);
        ok(lres == 0, "lres was %Id\n", lres);

        lres = 0xDEADBEEF;
        hr = IShellBrowser_SendControlMsg(psb, FCW_STATUS, 0, 0, 0, &lres);
        ok(hr == S_OK, "got (0x%08lx)\n", hr);
        ok(lres == 0, "lres was %Id\n", lres);

        lres = 0xDEADBEEF;
        hr = IShellBrowser_SendControlMsg(psb, 1234, 0, 0, 0, &lres);
        ok(hr == S_OK, "got (0x%08lx)\n", hr);
        ok(lres == 0, "lres was %Id\n", lres);

        /* Returns S_OK */
        hr = IShellBrowser_SetStatusTextSB(psb, NULL);
        ok(hr == S_OK, "got (0x%08lx)\n", hr);

        hr = IShellBrowser_ContextSensitiveHelp(psb, FALSE);
        ok(hr == S_OK, "got (0x%08lx)\n", hr);

        hr = IShellBrowser_EnableModelessSB(psb, TRUE);
        ok(hr == S_OK, "got (0x%08lx)\n", hr);

        hr = IShellBrowser_SetToolbarItems(psb, NULL, 1, 1);
        ok(hr == S_OK, "got (0x%08lx)\n", hr);
    }

    hr = IShellBrowser_QueryActiveShellView(psb, (IShellView**)&punk);
    ok(hr == E_FAIL, "got (0x%08lx)\n", hr);

    IShellBrowser_Release(psb);
    IExplorerBrowser_Destroy(peb);
    IExplorerBrowser_Release(peb);

    /* Browse to the desktop. */
    ebrowser_instantiate(&peb);
    ebrowser_initialize(peb);
    IExplorerBrowser_QueryInterface(peb, &IID_IShellBrowser, (void**)&psb);

    process_msgs();
    hr = ebrowser_browse_to_desktop(peb);
    ok(hr == S_OK, "got (0x%08lx)\n", hr);
    process_msgs();

    /****** After Browsing *****/

    hr = IShellBrowser_QueryActiveShellView(psb, (IShellView**)&punk);
    ok(hr == S_OK, "got (0x%08lx)\n", hr);
    if(SUCCEEDED(hr)) IUnknown_Release(punk);

    IShellBrowser_Release(psb);
    IExplorerBrowser_Destroy(peb);
    ref = IExplorerBrowser_Release(peb);
    ok(ref == 0, "Got %ld\n", ref);
}

static void test_initialization(void)
{
    IExplorerBrowser *peb;
    IShellBrowser *psb;
    HWND eb_hwnd;
    HRESULT hr;
    ULONG lres;
    LONG style;
    RECT rc;

    ebrowser_instantiate(&peb);

    if(0)
    {
        /* Crashes on Windows 7 */
        IExplorerBrowser_Initialize(peb, NULL, NULL, NULL);
        IExplorerBrowser_Initialize(peb, hwnd, NULL, NULL);
    }

    ZeroMemory(&rc, sizeof(RECT));

    hr = IExplorerBrowser_Initialize(peb, NULL, &rc, NULL);
    ok(hr == E_INVALIDARG, "got (0x%08lx)\n", hr);

    hr = IExplorerBrowser_Initialize(peb, hwnd, &rc, NULL);
    ok(hr == S_OK, "got (0x%08lx)\n", hr);

    /* Initialize twice */
    hr = IExplorerBrowser_Initialize(peb, hwnd, &rc, NULL);
    ok(hr == E_UNEXPECTED, "got (0x%08lx)\n", hr);

    hr = IExplorerBrowser_Destroy(peb);
    ok(hr == S_OK, "got (0x%08lx)\n", hr);

    /* Initialize again */
    hr = IExplorerBrowser_Initialize(peb, hwnd, &rc, NULL);
    ok(hr == E_UNEXPECTED, "got (0x%08lx)\n", hr);

    /* Destroy again */
    hr = IExplorerBrowser_Destroy(peb);
    ok(hr == S_OK, "got (0x%08lx)\n", hr);
    lres = IExplorerBrowser_Release(peb);
    ok(lres == 0, "Got %ld\n", lres);

    /* Initialize with a few different rectangles */
    peb = NULL;
    ebrowser_instantiate(&peb);
    SetRect(&rc, 50, 20, 100, 80);
    hr = IExplorerBrowser_Initialize(peb, hwnd, &rc, NULL);
    ok(hr == S_OK, "got (0x%08lx)\n", hr);
    hr = IExplorerBrowser_QueryInterface(peb, &IID_IShellBrowser, (void**)&psb);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    if(SUCCEEDED(hr))
    {
        RECT eb_rc;
        char buf[1024];
        LONG expected_style;
        static const RECT exp_rc = {0, 0, 48, 58};

        hr = IShellBrowser_GetWindow(psb, &eb_hwnd);
        ok(hr == S_OK, "Got 0x%08lx\n", hr);

        GetClientRect(eb_hwnd, &eb_rc);
        ok(EqualRect(&eb_rc, &exp_rc), "Got client rect %s\n", wine_dbgstr_rect(&eb_rc));

        GetWindowRect(eb_hwnd, &eb_rc);
        ok(eb_rc.right - eb_rc.left == 50, "Got window width %ld\n", eb_rc.right - eb_rc.left);
        ok(eb_rc.bottom - eb_rc.top == 60, "Got window height %ld\n", eb_rc.bottom - eb_rc.top);

        buf[0] = '\0';
        GetClassNameA(eb_hwnd, buf, 1024);
        ok(!lstrcmpA(buf, "ExplorerBrowserControl"), "Unexpected classname %s\n", buf);

        expected_style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_BORDER;
        style = GetWindowLongPtrW(eb_hwnd, GWL_STYLE);
        todo_wine ok(style == expected_style, "Got style 0x%08lx, expected 0x%08lx\n", style, expected_style);

        expected_style = WS_EX_CONTROLPARENT;
        style = GetWindowLongPtrW(eb_hwnd, GWL_EXSTYLE);
        ok(style == expected_style, "Got exstyle 0x%08lx, expected 0x%08lx\n", style, expected_style);

        ok(GetParent(eb_hwnd) == hwnd, "GetParent returns %p\n", GetParent(eb_hwnd));

        /* ::Destroy() destroys the window. */
        ok(IsWindow(eb_hwnd), "eb_hwnd invalid.\n");
        IExplorerBrowser_Destroy(peb);
        ok(!IsWindow(eb_hwnd), "eb_hwnd valid.\n");

        IShellBrowser_Release(psb);
        lres = IExplorerBrowser_Release(peb);
        ok(lres == 0, "Got refcount %ld\n", lres);
    }
    else
    {
        skip("Skipping some tests.\n");

        IExplorerBrowser_Destroy(peb);
        lres = IExplorerBrowser_Release(peb);
        ok(lres == 0, "Got refcount %ld\n", lres);
    }

    /* check window style with EBO_NOBORDER */
    ebrowser_instantiate(&peb);
    hr = IExplorerBrowser_SetOptions(peb, EBO_NOBORDER);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    SetRect(&rc, 50, 20, 100, 80);

    hr = IExplorerBrowser_Initialize(peb, hwnd, &rc, NULL);
    ok(hr == S_OK, "got (0x%08lx)\n", hr);

    hr = IExplorerBrowser_QueryInterface(peb, &IID_IShellBrowser, (void**)&psb);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);

    hr = IShellBrowser_GetWindow(psb, &eb_hwnd);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);

    style = GetWindowLongPtrW(eb_hwnd, GWL_STYLE);
    ok(!(style & WS_BORDER) || broken(style & WS_BORDER) /* before win8 */, "got style 0x%08lx\n", style);

    IShellBrowser_Release(psb);
    IExplorerBrowser_Destroy(peb);
    IExplorerBrowser_Release(peb);

    /* empty rectangle */
    ebrowser_instantiate(&peb);
    SetRectEmpty(&rc);
    hr = IExplorerBrowser_Initialize(peb, hwnd, &rc, NULL);
    ok(hr == S_OK, "got (0x%08lx)\n", hr);
    IExplorerBrowser_Destroy(peb);
    lres = IExplorerBrowser_Release(peb);
    ok(lres == 0, "Got refcount %ld\n", lres);

    ebrowser_instantiate(&peb);
    SetRect(&rc, -1, -1, 1, 1);
    hr = IExplorerBrowser_Initialize(peb, hwnd, &rc, NULL);
    ok(hr == S_OK, "got (0x%08lx)\n", hr);
    IExplorerBrowser_Destroy(peb);
    lres = IExplorerBrowser_Release(peb);
    ok(lres == 0, "Got refcount %ld\n", lres);

    ebrowser_instantiate(&peb);
    SetRect(&rc, 10, 10, 5, 5);
    hr = IExplorerBrowser_Initialize(peb, hwnd, &rc, NULL);
    ok(hr == S_OK, "got (0x%08lx)\n", hr);
    IExplorerBrowser_Destroy(peb);
    lres = IExplorerBrowser_Release(peb);
    ok(lres == 0, "Got refcount %ld\n", lres);

    ebrowser_instantiate(&peb);
    SetRect(&rc, 10, 10, 5, 5);
    hr = IExplorerBrowser_Initialize(peb, hwnd, &rc, NULL);
    ok(hr == S_OK, "got (0x%08lx)\n", hr);
    IExplorerBrowser_Destroy(peb);
    lres = IExplorerBrowser_Release(peb);
    ok(lres == 0, "Got refcount %ld\n", lres);
}

static void test_SetSite(void)
{
    IExplorerBrowser *peb;
    IServiceProviderImpl *spimpl = create_serviceprovider();
    ICommDlgBrowser3Impl *cdbimpl = create_commdlgbrowser3();
    IExplorerPaneVisibilityImpl *epvimpl = create_explorerpanevisibility();
    IObjectWithSite *pow;
    HRESULT hr;
    LONG ref;
    UINT i;
    struct services expected[] =
    {
        {&SID_STopLevelBrowser,        &IID_ICommDlgBrowser2,          cdbimpl},
        {&SID_ExplorerPaneVisibility,  &IID_IExplorerPaneVisibility,   epvimpl},
        {&SID_SExplorerBrowserFrame,   &IID_ICommDlgBrowser2,          cdbimpl},
        {&SID_SExplorerBrowserFrame,   &IID_ICommDlgBrowser3,          cdbimpl},
        {&IID_ICommDlgBrowser,         &IID_ICommDlgBrowser,           cdbimpl},
        {NULL}
    };

    ebrowser_instantiate(&peb);
    IExplorerBrowser_SetOptions(peb, EBO_SHOWFRAMES);

    hr = IExplorerBrowser_QueryInterface(peb, &IID_IObjectWithSite, (void**)&pow);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    if(SUCCEEDED(hr))
    {
        spimpl->interfaces = expected;

        hr = IObjectWithSite_SetSite(pow, (IUnknown*)&spimpl->IServiceProvider_iface);
        ok(hr == S_OK, "Got 0x%08lx\n", hr);

        if(FAILED(hr))
            IObjectWithSite_Release(pow);
    }

    if(FAILED(hr))
    {
        skip("Failed to set site.\n");

        IServiceProvider_Release(&spimpl->IServiceProvider_iface);
        ICommDlgBrowser3_Release(&cdbimpl->ICommDlgBrowser3_iface);
        IExplorerPaneVisibility_Release(&epvimpl->IExplorerPaneVisibility_iface);
        IExplorerBrowser_Destroy(peb);
        ref = IExplorerBrowser_Release(peb);
        ok(ref == 0, "Got ref %ld\n", ref);

        return;
    }

    ShowWindow(hwnd, TRUE);
    ebrowser_initialize(peb);
    ebrowser_browse_to_desktop(peb);

    for(i = 0; i < 10; i++)
    {
        Sleep(100);
        process_msgs();
    }
    ShowWindow(hwnd, FALSE);

    /* ICommDlgBrowser3 */
    ok(!cdbimpl->OnDefaultCommand, "Got %d\n", cdbimpl->OnDefaultCommand);
    todo_wine ok(cdbimpl->OnStateChange, "Got %d\n", cdbimpl->OnStateChange);
    ok(cdbimpl->IncludeObject, "Got %d\n", cdbimpl->IncludeObject);
    ok(!cdbimpl->Notify, "Got %d\n", cdbimpl->Notify);
    ok(!cdbimpl->GetDefaultMenuText, "Got %d\n", cdbimpl->GetDefaultMenuText);
    todo_wine ok(cdbimpl->GetViewFlags, "Got %d\n", cdbimpl->GetViewFlags);
    ok(!cdbimpl->OnColumnClicked, "Got %d\n", cdbimpl->OnColumnClicked);
    ok(!cdbimpl->GetCurrentFilter, "Got %d\n", cdbimpl->GetCurrentFilter);
    todo_wine ok(cdbimpl->OnPreviewCreated, "Got %d\n", cdbimpl->OnPreviewCreated);

    /* IExplorerPaneVisibility */
    ok(epvimpl->np, "Got %ld\n", epvimpl->np);
    todo_wine ok(epvimpl->cp, "Got %ld\n", epvimpl->cp);
    todo_wine ok(epvimpl->cp_o, "Got %ld\n", epvimpl->cp_o);
    todo_wine ok(epvimpl->cp_v, "Got %ld\n", epvimpl->cp_v);
    todo_wine ok(epvimpl->dp, "Got %ld\n", epvimpl->dp);
    todo_wine ok(epvimpl->pp, "Got %ld\n", epvimpl->pp);
    ok(!epvimpl->qp, "Got %ld\n", epvimpl->qp);
    ok(!epvimpl->aqp, "Got %ld\n", epvimpl->aqp);
    ok(!epvimpl->unk, "Got %ld\n", epvimpl->unk);

    /* Test when IServiceProvider is released. */
    IServiceProvider_AddRef(&spimpl->IServiceProvider_iface);
    ref = IServiceProvider_Release(&spimpl->IServiceProvider_iface);
    ok(ref == 2, "Got ref %ld\n", ref);

    hr = IObjectWithSite_SetSite(pow, NULL);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);

    IServiceProvider_AddRef(&spimpl->IServiceProvider_iface);
    ref = IServiceProvider_Release(&spimpl->IServiceProvider_iface);
    ok(ref == 1, "Got ref %ld\n", ref);

    hr = IObjectWithSite_SetSite(pow, (IUnknown*)&spimpl->IServiceProvider_iface);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);

    IServiceProvider_AddRef(&spimpl->IServiceProvider_iface);
    ref = IServiceProvider_Release(&spimpl->IServiceProvider_iface);
    ok(ref == 2, "Got ref %ld\n", ref);

    IExplorerBrowser_Destroy(peb);

    IServiceProvider_AddRef(&spimpl->IServiceProvider_iface);
    ref = IServiceProvider_Release(&spimpl->IServiceProvider_iface);
    ok(ref == 2, "Got ref %ld\n", ref);

    IObjectWithSite_Release(pow);
    ref = IExplorerBrowser_Release(peb);
    ok(ref == 0, "Got ref %ld\n", ref);

    ref = IServiceProvider_Release(&spimpl->IServiceProvider_iface);
    ok(ref == 0, "Got ref %ld\n", ref);

    ref = ICommDlgBrowser3_Release(&cdbimpl->ICommDlgBrowser3_iface);
    ok(ref == 0, "Got ref %ld\n", ref);
    ref = IExplorerPaneVisibility_Release(&epvimpl->IExplorerPaneVisibility_iface);
    ok(ref == 0, "Got ref %ld\n", ref);
}

static void test_basics(void)
{
    IExplorerBrowser *peb;
    IShellBrowser *psb;
    ULONG lres;
    EXPLORER_BROWSER_OPTIONS flags;
    HDWP hdwp;
    RECT rc;
    HRESULT hr;

    ebrowser_instantiate(&peb);
    ebrowser_initialize(peb);

    /* SetRect */
    SetRectEmpty(&rc);
    hr = IExplorerBrowser_SetRect(peb, NULL, rc);
    ok(hr == S_OK, "got (0x%08lx)\n", hr);

    SetRect(&rc, 100, 100, 10, 10);
    hr = IExplorerBrowser_SetRect(peb, NULL, rc);
    ok(hr == S_OK, "got (0x%08lx)\n", hr);

    /* SetRect with DeferWindowPos */
    SetRect(&rc, 0, 0, 10, 10);
    hdwp = BeginDeferWindowPos(1);
    hr = IExplorerBrowser_SetRect(peb, &hdwp, rc);
    ok(hr == S_OK, "got (0x%08lx)\n", hr);
    lres = EndDeferWindowPos(hdwp);
    ok(lres, "EndDeferWindowPos failed.\n");

    hdwp = NULL;
    hr = IExplorerBrowser_SetRect(peb, &hdwp, rc);
    ok(hr == S_OK, "got (0x%08lx)\n", hr);
    ok(hdwp == NULL, "got %p\n", hdwp);
    lres = EndDeferWindowPos(hdwp);
    ok(!lres, "EndDeferWindowPos succeeded unexpectedly.\n");

    /* Test positioning */
    SetRect(&rc, 10, 20, 50, 50);
    hr = IExplorerBrowser_SetRect(peb, NULL, rc);
    ok(hr == S_OK, "got (0x%08lx)\n", hr);
    hr = IExplorerBrowser_QueryInterface(peb, &IID_IShellBrowser, (void**)&psb);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    if(SUCCEEDED(hr))
    {
        HWND eb_hwnd;
        RECT eb_rc;
        static const RECT exp_rc = {11, 21, 49, 49};
        static const RECT exp_rc2 = {11, 21, 49, 24};

        hr = IShellBrowser_GetWindow(psb, &eb_hwnd);
        ok(hr == S_OK, "Got 0x%08lx\n", hr);

        GetClientRect(eb_hwnd, &eb_rc);
        MapWindowPoints(eb_hwnd, hwnd, (POINT*)&eb_rc, 2);
        ok(EqualRect(&eb_rc, &exp_rc), "Got rect %s\n", wine_dbgstr_rect(&eb_rc));

        /* Try resizing with invalid hdwp */
        rc.bottom = 25;
        hdwp = (HDWP)0xdeadbeef;
        hr = IExplorerBrowser_SetRect(peb, &hdwp, rc);
        ok(hr == E_FAIL, "Got 0x%08lx\n", hr);
        GetClientRect(eb_hwnd, &eb_rc);
        MapWindowPoints(eb_hwnd, hwnd, (POINT*)&eb_rc, 2);
        ok(EqualRect(&eb_rc, &exp_rc), "Got rect %s\n", wine_dbgstr_rect(&eb_rc));

        hdwp = NULL;
        hr = IExplorerBrowser_SetRect(peb, &hdwp, rc);
        ok(hr == S_OK, "Got 0x%08lx\n", hr);
        GetClientRect(eb_hwnd, &eb_rc);
        MapWindowPoints(eb_hwnd, hwnd, (POINT*)&eb_rc, 2);
        ok(EqualRect(&eb_rc, &exp_rc2), "Got rect %s\n", wine_dbgstr_rect(&eb_rc));

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
    ok(hr == S_OK, "got (0x%08lx)\n", hr);
    ok(flags == 0, "got (0x%08x)\n", flags);

    /* Settings preserved through Initialize. */
    hr = IExplorerBrowser_SetOptions(peb, 0xDEADBEEF);
    ok(hr == S_OK, "got (0x%08lx)\n", hr);

    ebrowser_initialize(peb);

    hr = IExplorerBrowser_GetOptions(peb, &flags);
    ok(flags == 0xDEADBEEF, "got (0x%08x)\n", flags);
    ok(hr == S_OK, "got (0x%08lx)\n", hr);

    IExplorerBrowser_Destroy(peb);
    IExplorerBrowser_Release(peb);

    ebrowser_instantiate(&peb);
    ebrowser_initialize(peb);

    /* SetPropertyBag */
    hr = IExplorerBrowser_SetPropertyBag(peb, NULL);
    ok(hr == E_INVALIDARG, "Got 0x%08lx\n", hr);
    hr = IExplorerBrowser_SetPropertyBag(peb, L"WineTest");
    ok(hr == S_OK, "Got 0x%08lx\n", hr);

    /* TODO: Test after browsing somewhere. */

    IExplorerBrowser_Destroy(peb);
    lres = IExplorerBrowser_Release(peb);
    ok(lres == 0, "Got %ld\n", lres);
}

static void test_Advise(void)
{
    IExplorerBrowser *peb;
    IExplorerBrowserEvents *pebe;
    DWORD cookies[10];
    HRESULT hr;
    UINT i, ref;

    /* Set up our IExplorerBrowserEvents implementation */
    ebev.IExplorerBrowserEvents_iface.lpVtbl = &ebevents;
    pebe = &ebev.IExplorerBrowserEvents_iface;

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
    ok(hr == E_INVALIDARG, "got (0x%08lx)\n", hr);

    /* Add some before initialization */
    for(i = 0; i < 5; i++)
    {
        hr = IExplorerBrowser_Advise(peb, pebe, &cookies[i]);
        ok(hr == S_OK, "got (0x%08lx)\n", hr);
    }

    ebrowser_initialize(peb);

    /* Add some after initialization */
    for(i = 5; i < 10; i++)
    {
        hr = IExplorerBrowser_Advise(peb, pebe, &cookies[i]);
        ok(hr == S_OK, "got (0x%08lx)\n", hr);
    }

    ok(ebev.ref == 10, "Got %ld\n", ebev.ref);

    ebev.completed = 0;
    ebrowser_browse_to_desktop(peb);
    process_msgs();
    ok(ebev.completed == 10, "Got %d\n", ebev.completed);

    /* Remove a bunch somewhere in the middle */
    for(i = 4; i < 8; i++)
    {
        hr = IExplorerBrowser_Unadvise(peb, cookies[i]);
        ok(hr == S_OK, "got (0x%08lx)\n", hr);
    }

    ebev.completed = 0;
    ebrowser_browse_to_desktop(peb);
    process_msgs();
    ok(ebev.completed == 6, "Got %d\n", ebev.completed);

    if(0)
    {
        /* Using unadvise with a previously unadvised cookie results
         * in a crash. */
        IExplorerBrowser_Unadvise(peb, cookies[5]);
    }

    /* Remove the rest. */
    for(i = 0; i < 10; i++)
    {
        if(i<4||i>7)
        {
            hr = IExplorerBrowser_Unadvise(peb, cookies[i]);
            ok(hr == S_OK, "%d: got (0x%08lx)\n", i, hr);
        }
    }

    ok(ebev.ref == 0, "Got %ld\n", ebev.ref);

    ebev.completed = 0;
    ebrowser_browse_to_desktop(peb);
    process_msgs();
    ok(ebev.completed == 0, "Got %d\n", ebev.completed);

    /* ::Destroy implies ::Unadvise. */
    hr = IExplorerBrowser_Advise(peb, pebe, &cookies[0]);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    ok(ebev.ref == 1, "Got %ld\n", ebev.ref);

    hr = IExplorerBrowser_Destroy(peb);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    ok(ebev.ref == 0, "Got %ld\n", ebev.ref);

    ref = IExplorerBrowser_Release(peb);
    ok(!ref, "Got %d\n", ref);
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
    ok_(file, line) (hr == hr_exp, "BrowseToIDList returned 0x%08lx\n", hr);
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
    ok_(file, line) (hr == S_OK, "QueryInterface returned 0x%08lx\n", hr);
    if(SUCCEEDED(hr))
    {
        ebev->completed = ebev->created = ebev->pending = ebev->failed = 0;

        hr = IShellBrowser_BrowseObject(psb, pidl, uFlags);
        ok_(file, line) (hr == hr_exp, "BrowseObject returned 0x%08lx\n", hr);
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
    static WCHAR testfolderW[] = L"winetestfolder";

    ok(pSHCreateShellItem != NULL, "pSHCreateShellItem unexpectedly missing.\n");

    GetCurrentDirectoryW(MAX_PATH, current_path);
    if(!current_path[0])
    {
        skip("Failed to create test-directory.\n");
        return;
    }

    lstrcpyW(child_path, current_path);
    myPathAddBackslashW(child_path);
    lstrcatW(child_path, testfolderW);

    CreateDirectoryW(child_path, NULL);

    hr = SHParseDisplayName(current_path, NULL, &pidl_current, 0, NULL);
    ok(hr == S_OK, "Failed to parse a path, hr %#lx.\n", hr);
    hr = SHParseDisplayName(child_path, NULL, &pidl_child, 0, NULL);
    ok(hr == S_OK, "Failed to parse a path, hr %#lx.\n", hr);

    ebrowser_instantiate(&peb);
    ebrowser_initialize(peb);

    ebrowser_instantiate(&peb2);
    ebrowser_initialize(peb2);

    /* Set up our IExplorerBrowserEvents implementation */
    ebev.IExplorerBrowserEvents_iface.lpVtbl = &ebevents;

    IExplorerBrowser_Advise(peb, &ebev.IExplorerBrowserEvents_iface, &cookie);
    IExplorerBrowser_Advise(peb2, &ebev.IExplorerBrowserEvents_iface, &cookie2);

    /* These should all fail */
    test_browse_pidl(peb, &ebev, 0, SBSP_ABSOLUTE | SBSP_RELATIVE, E_FAIL, 0, 0, 0, 0);
    test_browse_pidl_sb(peb2, &ebev, 0, SBSP_ABSOLUTE | SBSP_RELATIVE, E_FAIL, 0, 0, 0, 0);
    test_browse_pidl(peb, &ebev, 0, SBSP_ABSOLUTE, E_INVALIDARG, 0, 0, 0, 0);
    test_browse_pidl_sb(peb2, &ebev, 0, SBSP_ABSOLUTE, E_INVALIDARG, 0, 0, 0, 0);
    test_browse_pidl(peb, &ebev, 0, SBSP_RELATIVE, E_FAIL, 0, 0, 0, 0);
    test_browse_pidl_sb(peb2, &ebev, 0, SBSP_RELATIVE, E_FAIL, 0, 0, 0, 0);
    test_browse_pidl(peb, &ebev, 0, SBSP_PARENT, E_FAIL, 0, 0, 0, 0);
    test_browse_pidl_sb(peb2, &ebev, 0, SBSP_PARENT, E_FAIL, 0, 0, 0, 0);
    test_browse_pidl(peb, &ebev, 0, SBSP_NAVIGATEFORWARD, E_FAIL, 0, 0, 0, 0);
    test_browse_pidl_sb(peb2, &ebev, 0, SBSP_NAVIGATEFORWARD, E_FAIL, 0, 0, 0, 0);
    test_browse_pidl(peb, &ebev, 0, SBSP_NAVIGATEBACK, E_FAIL, 0, 0, 0, 0);
    test_browse_pidl_sb(peb2, &ebev, 0, SBSP_NAVIGATEBACK, E_FAIL, 0, 0, 0, 0);

    /* "The first browse is synchronous" */
    test_browse_pidl(peb, &ebev, pidl_child, SBSP_ABSOLUTE, S_OK, 1, 1, 0, 1);
    test_browse_pidl_sb(peb2, &ebev, pidl_child, SBSP_ABSOLUTE, S_OK, 1, 1, 0, 1);

    /* Navigate empty history */
    test_browse_pidl(peb, &ebev, 0, SBSP_NAVIGATEFORWARD, S_OK, 0, 0, 0, 0);
    test_browse_pidl_sb(peb2, &ebev, 0, SBSP_NAVIGATEFORWARD, S_OK, 0, 0, 0, 0);
    test_browse_pidl(peb, &ebev, 0, SBSP_NAVIGATEBACK, S_OK, 0, 0, 0, 0);
    test_browse_pidl_sb(peb2, &ebev, 0, SBSP_NAVIGATEBACK, S_OK, 0, 0, 0, 0);

    /* Navigate history */
    test_browse_pidl(peb, &ebev, 0, SBSP_PARENT, S_OK, 1, 1, 0, 1);
    test_browse_pidl_sb(peb2, &ebev, 0, SBSP_PARENT, S_OK, 1, 1, 0, 1);
    test_browse_pidl(peb, &ebev, 0, SBSP_NAVIGATEBACK, S_OK, 1, 1, 0, 1);
    test_browse_pidl_sb(peb2, &ebev, 0, SBSP_NAVIGATEBACK, S_OK, 1, 1, 0, 1);
    test_browse_pidl(peb, &ebev, 0, SBSP_NAVIGATEFORWARD, S_OK, 1, 1, 0, 1);
    test_browse_pidl_sb(peb2, &ebev, 0, SBSP_NAVIGATEFORWARD, S_OK, 1, 1, 0, 1);
    test_browse_pidl(peb, &ebev, 0, SBSP_ABSOLUTE, S_OK, 0, 0, 0, 0);
    test_browse_pidl_sb(peb2, &ebev, 0, SBSP_ABSOLUTE, S_OK, 0, 0, 0, 0);

    /* Relative navigation */
    test_browse_pidl(peb, &ebev, pidl_current, SBSP_ABSOLUTE, S_OK, 1, 0, 0, 1);
    test_browse_pidl_sb(peb2, &ebev, pidl_current, SBSP_ABSOLUTE, S_OK, 1, 0, 0, 1);

    hr = IExplorerBrowser_GetCurrentView(peb, &IID_IFolderView, (void**)&pfv);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    if(SUCCEEDED(hr))
    {
        LPITEMIDLIST pidl_relative;

        hr = IFolderView_GetFolder(pfv, &IID_IShellFolder, (void**)&psf);
        ok(hr == S_OK, "Got 0x%08lx\n", hr);
        hr = IShellFolder_ParseDisplayName(psf, NULL, NULL, testfolderW, NULL, &pidl_relative, NULL);
        ok(hr == S_OK, "Got 0x%08lx\n", hr);

        /* Browsing to another location here before using the
         * pidl_relative would make ExplorerBrowser in Windows 7 show a
         * not-available dialog. Also, passing a relative pidl without
         * specifying SBSP_RELATIVE makes it look for the pidl on the
         * desktop
         */

        test_browse_pidl(peb, &ebev, pidl_relative, SBSP_RELATIVE, S_OK, 1, 1, 0, 1);
        test_browse_pidl_sb(peb2, &ebev, pidl_relative, SBSP_RELATIVE, S_OK, 1, 1, 0, 1);

        ILFree(pidl_relative);
        IShellFolder_Release(psf);
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
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    IExplorerBrowser_Destroy(peb);
    process_msgs();
    hr = IExplorerBrowser_Unadvise(peb2, cookie2);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    IExplorerBrowser_Destroy(peb2);
    process_msgs();

    /* Attempt browsing after destroyed */
    test_browse_pidl(peb, &ebev, pidl_child, SBSP_ABSOLUTE, HRESULT_FROM_WIN32(ERROR_BUSY), 0, 0, 0, 0);
    test_browse_pidl_sb(peb2, &ebev, pidl_child, SBSP_ABSOLUTE, HRESULT_FROM_WIN32(ERROR_BUSY), 0, 0, 0, 0);

    lres = IExplorerBrowser_Release(peb);
    ok(lres == 0, "Got lres %ld\n", lres);
    lres = IExplorerBrowser_Release(peb2);
    ok(lres == 0, "Got lres %ld\n", lres);

    /******************************************/
    /* Test some options that affect browsing */

    ebrowser_instantiate(&peb);
    hr = IExplorerBrowser_Advise(peb, &ebev.IExplorerBrowserEvents_iface, &cookie);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    hr = IExplorerBrowser_SetOptions(peb, EBO_NAVIGATEONCE);
    ok(hr == S_OK, "got (0x%08lx)\n", hr);
    ebrowser_initialize(peb);

    test_browse_pidl(peb, &ebev, pidl_current, 0, S_OK, 1, 1, 0, 1);
    test_browse_pidl(peb, &ebev, pidl_current, 0, E_FAIL, 0, 0, 0, 0);

    hr = IExplorerBrowser_SetOptions(peb, 0);
    ok(hr == S_OK, "got (0x%08lx)\n", hr);

    test_browse_pidl(peb, &ebev, pidl_current, 0, S_OK, 1, 0, 0, 1);
    test_browse_pidl(peb, &ebev, pidl_current, 0, S_OK, 1, 0, 0, 1);

    /* Difference in behavior lies where? */
    hr = IExplorerBrowser_SetOptions(peb, EBO_ALWAYSNAVIGATE);
    ok(hr == S_OK, "got (0x%08lx)\n", hr);

    test_browse_pidl(peb, &ebev, pidl_current, 0, S_OK, 1, 0, 0, 1);
    test_browse_pidl(peb, &ebev, pidl_current, 0, S_OK, 1, 0, 0, 1);

    hr = IExplorerBrowser_Unadvise(peb, cookie);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);

    IExplorerBrowser_Destroy(peb);
    lres = IExplorerBrowser_Release(peb);
    ok(lres == 0, "Got lres %ld\n", lres);

    /* BrowseToObject tests */
    ebrowser_instantiate(&peb);
    ebrowser_initialize(peb);

    /* Browse to the desktop by passing an IShellFolder */
    hr = SHGetDesktopFolder(&psf);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IExplorerBrowser_BrowseToObject(peb, (IUnknown*)psf, SBSP_DEFBROWSER);
        ok(hr == S_OK, "got (0x%08lx)\n", hr);
        if(hr == S_OK) process_msgs();

        IShellFolder_Release(psf);
    }

    /* Browse to the current directory by passing a ShellItem */
    hr = pSHCreateShellItem(NULL, NULL, pidl_current, &psi);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IExplorerBrowser_BrowseToObject(peb, (IUnknown*)psi, SBSP_DEFBROWSER);
        ok(hr == S_OK, "got (0x%08lx)\n", hr);
        process_msgs();

        IShellItem_Release(psi);
    }

    IExplorerBrowser_Destroy(peb);
    lres = IExplorerBrowser_Release(peb);
    ok(lres == 0, "Got lres %ld\n", lres);

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
        IExplorerBrowser_GetCurrentView(peb, NULL, NULL);
    }
    hr = IExplorerBrowser_GetCurrentView(peb, NULL, (void**)&punk);
    ok(hr == E_FAIL, "Got 0x%08lx\n", hr);

#define test_gcv(iid, exp)                                              \
    do {                                                                \
        hr = IExplorerBrowser_GetCurrentView(peb, &iid, (void**)&punk); \
        ok(hr == exp, "(%s:)Expected (0x%08lx), got: (0x%08lx)\n",        \
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

static void test_InputObject(void)
{
    IExplorerBrowser *peb;
    IShellFolder *psf;
    IInputObject *pio;
    HRESULT hr;
    RECT rc;
    UINT i;
    WPARAM supported_key_accels_mode1[] = {
        VK_BACK, VK_TAB, VK_RETURN, VK_PRIOR, VK_NEXT, VK_END, VK_HOME,
        VK_LEFT, VK_UP, VK_RIGHT, VK_DOWN, VK_DELETE, VK_F1, VK_F2,
        VK_F5, VK_F6, VK_F10, 0 };
    WPARAM supported_key_accels_mode2[] = {
        VK_RETURN, VK_PRIOR, VK_NEXT, VK_END, VK_HOME,
        VK_LEFT, VK_UP, VK_RIGHT, VK_DOWN, VK_DELETE, VK_F1, VK_F2,
        VK_F10, 0 };
    WPARAM *key_accels;
    MSG msg_a = {
        hwnd,
        WM_KEYDOWN,
        VK_F5, 0,
        GetTickCount(),
        {5, 2}
    };

    ebrowser_instantiate(&peb);
    hr = IExplorerBrowser_QueryInterface(peb, &IID_IInputObject, (void**)&pio);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    if(FAILED(hr))
    {
        win_skip("IInputObject not supported.\n");
        return;
    }

    /* Before initializing */
    hr = IInputObject_TranslateAcceleratorIO(pio, &msg_a);
    todo_wine ok(hr == E_FAIL, "Got 0x%08lx\n", hr);

    hr = IInputObject_HasFocusIO(pio);
    todo_wine ok(hr == E_FAIL, "Got 0x%08lx\n", hr);

    hr = IInputObject_UIActivateIO(pio, TRUE, &msg_a);
    todo_wine ok(hr == S_OK, "Got 0x%08lx\n", hr);

    hr = IInputObject_HasFocusIO(pio);
    todo_wine ok(hr == E_FAIL, "Got 0x%08lx\n", hr);

    hr = IInputObject_TranslateAcceleratorIO(pio, &msg_a);
    todo_wine ok(hr == E_FAIL, "Got 0x%08lx\n", hr);

    SetRect(&rc, 0, 0, 100, 100);
    hr = IExplorerBrowser_Initialize(peb, hwnd, &rc, NULL);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);

    hr = IInputObject_HasFocusIO(pio);
    todo_wine ok(hr == E_FAIL, "Got 0x%08lx\n", hr);

    hr = IInputObject_TranslateAcceleratorIO(pio, &msg_a);
    todo_wine ok(hr == E_FAIL, "Got 0x%08lx\n", hr);

    /* Browse to the desktop */
    SHGetDesktopFolder(&psf);
    hr = IExplorerBrowser_BrowseToObject(peb, (IUnknown*)psf, SBSP_DEFBROWSER);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    IShellFolder_Release(psf);

    hr = IInputObject_UIActivateIO(pio, TRUE, &msg_a);
    todo_wine ok(hr == S_OK, "Got 0x%08lx\n", hr);

    hr = IInputObject_HasFocusIO(pio);
    todo_wine ok(hr == S_OK, "Got 0x%08lx\n", hr);

    hr = IInputObject_UIActivateIO(pio, FALSE, &msg_a);
    todo_wine ok(hr == S_OK, "Got 0x%08lx\n", hr);

    hr = IInputObject_HasFocusIO(pio);
    todo_wine ok(hr == S_OK, "Got 0x%08lx\n", hr);

    hr = IInputObject_TranslateAcceleratorIO(pio, &msg_a);
    if(hr == S_OK)
        key_accels = supported_key_accels_mode1;
    else
        key_accels = supported_key_accels_mode2;

    for(i = 0; i < 0x100; i++)
    {
        BOOL found = FALSE;
        UINT j;
        for(j = 0; key_accels[j] != 0; j++)
            if(key_accels[j] == i)
            {
                found = TRUE;
                break;
            }

        msg_a.wParam = i;
        process_msgs();
        hr = IInputObject_TranslateAcceleratorIO(pio, &msg_a);
        todo_wine ok(hr == (found ? S_OK : S_FALSE), "Got 0x%08lx (%04x)\n", hr, i);
        if(i == VK_F5)
            Sleep(1000); /* Needed for w2k8 (64bit) */
    }

    process_msgs();

    IInputObject_Release(pio);
    IExplorerBrowser_Destroy(peb);
    IExplorerBrowser_Release(peb);
}

static BOOL test_instantiate_control(void)
{
    IExplorerBrowser *peb;
    HRESULT hr;

    hr = ebrowser_instantiate(&peb);
    ok(hr == S_OK || hr == REGDB_E_CLASSNOTREG, "Got (0x%08lx)\n", hr);
    if(FAILED(hr))
        return FALSE;

    IExplorerBrowser_Release(peb);
    return TRUE;
}

static void setup_window(void)
{
    WNDCLASSW wc;

    ZeroMemory(&wc, sizeof(WNDCLASSW));
    wc.lpfnWndProc      = DefWindowProcW;
    wc.lpszClassName    = L"ebtest";
    RegisterClassW(&wc);
    hwnd = CreateWindowExW(0, L"ebtest", NULL, 0,
                           0, 0, 500, 500,
                           NULL, 0, 0, NULL);
    ok(hwnd != NULL, "Failed to create window for tests.\n");
}

#define CHECK_SETTINGS(browser,expected) _check_settings(browser, expected, __LINE__)
static void _check_settings(IExplorerBrowser *browser, FOLDERSETTINGS expected, int line)
{
    FOLDERSETTINGS settings;
    IShellView *view;
    HRESULT hr;

    hr = IExplorerBrowser_GetCurrentView(browser, &IID_IShellView, (void **)&view);
    ok_(__FILE__,line)(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IShellView_GetCurrentInfo(view, &settings);
    ok_(__FILE__,line)(hr == S_OK, "Got hr %#lx.\n", hr);
    ok_(__FILE__,line)(!memcmp(&settings, &expected, sizeof(settings)),
            "Got settings {view mode: %d, flags: %#x}, expected {view mode: %d, flags: %#x}.\n",
            settings.ViewMode, settings.fFlags, expected.ViewMode, expected.fFlags);
    IShellView_Release(view);
}

static void test_folder_settings(void)
{
    IExplorerBrowser *browser;
    FOLDERSETTINGS settings;
    HRESULT hr;

    ebrowser_instantiate(&browser);
    ebrowser_initialize(browser);

    hr = IExplorerBrowser_SetFolderSettings(browser, NULL);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    settings.ViewMode = 0; settings.fFlags = FWF_NONE;
    hr = IExplorerBrowser_SetFolderSettings(browser, &settings);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    settings.ViewMode = FVM_ICON; settings.fFlags = FWF_SNAPTOGRID;
    hr = IExplorerBrowser_SetFolderSettings(browser, &settings);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    ebrowser_browse_to_desktop(browser);
    settings.fFlags |= FWF_USESEARCHFOLDER | FWF_FULLROWSELECT | FWF_NOCLIENTEDGE;
    CHECK_SETTINGS(browser, settings);

    settings.ViewMode = FVM_AUTO; settings.fFlags = FWF_AUTOARRANGE;
    hr = IExplorerBrowser_SetFolderSettings(browser, &settings);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(settings.ViewMode == FVM_AUTO && settings.fFlags == FWF_AUTOARRANGE,
            "Got view mode %d, flags %#x.\n", settings.ViewMode, settings.fFlags);
    settings.ViewMode = FVM_ICON; settings.fFlags = FWF_AUTOARRANGE;
    CHECK_SETTINGS(browser, settings);

    settings.ViewMode = FVM_LIST; settings.fFlags = FWF_AUTOARRANGE;
    hr = IExplorerBrowser_SetFolderSettings(browser, &settings);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    settings.ViewMode = 0; settings.fFlags = FWF_SNAPTOGRID;
    hr = IExplorerBrowser_SetFolderSettings(browser, &settings);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(settings.ViewMode == 0 && settings.fFlags == FWF_SNAPTOGRID,
            "Got view mode %d, flags %#x.\n", settings.ViewMode, settings.fFlags);
    settings.ViewMode = FVM_LIST;
    CHECK_SETTINGS(browser, settings);

    settings.ViewMode = FVM_LAST + 1; settings.fFlags = FWF_AUTOARRANGE;
    hr = IExplorerBrowser_SetFolderSettings(browser, &settings);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    settings.ViewMode = FVM_LIST;
    CHECK_SETTINGS(browser, settings);

    settings.ViewMode = FVM_ICON; settings.fFlags = FWF_NONE;
    hr = IExplorerBrowser_SetFolderSettings(browser, &settings);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(settings.ViewMode == FVM_ICON && settings.fFlags == FWF_NONE,
            "Got view mode %d, flags %#x.\n", settings.ViewMode, settings.fFlags);
    CHECK_SETTINGS(browser, settings);

    IExplorerBrowser_Release(browser);
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
    test_SetSite();
    test_InputObject();
    test_folder_settings();

    DestroyWindow(hwnd);
    OleUninitialize();
}
