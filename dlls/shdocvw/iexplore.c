/*
 * SHDOCVW - Internet Explorer main frame window
 *
 * Copyright 2006 Mike McCormack (for CodeWeavers)
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

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "winnls.h"
#include "ole2.h"
#include "exdisp.h"
#include "oleidl.h"

#include "shdocvw.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

typedef struct tag_ieoc {
    IOleContainerVtbl *lpVtbl;
    LONG ref;
} ieoc;

static ieoc *ieoc_from_IOleContainer(IOleContainer *iface)
{
    return (ieoc*) iface;
}

static HRESULT WINAPI
ic_QueryInterface(IOleContainer *iface, REFIID riid, void **ppv)
{
    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IOleClientSite))
    {
        IUnknown_AddRef(iface);
        *ppv = iface;
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI ic_AddRef(IOleContainer *iface)
{
    ieoc *This = ieoc_from_IOleContainer(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ic_Release(IOleContainer *iface)
{
    ieoc *This = ieoc_from_IOleContainer(iface);
    LONG ref = InterlockedDecrement(&This->ref);
    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
        SHDOCVW_UnlockModule();
    }
    TRACE("refcount = %ld\n", ref);
    return ref;
}

static HRESULT WINAPI
ic_ParseDisplayName(IOleContainer *iface, IBindCtx *pbc, LPOLESTR pszDisplayName,
                    ULONG *pchEaten, IMoniker **ppmkOut)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI
ic_EnumObjects(IOleContainer *iface, DWORD grfFlags, IEnumUnknown **ppenum)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ic_LockContainer(IOleContainer *iface, BOOL fLock)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static IOleContainerVtbl ocVtbl =
{
    ic_QueryInterface,
    ic_AddRef,
    ic_Release,
    ic_ParseDisplayName,
    ic_EnumObjects,
    ic_LockContainer,
};

static IOleContainer * get_container(void)
{
    IOleContainer *This;
    ieoc *oc;

    oc = HeapAlloc(GetProcessHeap(), 0, sizeof *oc);
    oc->lpVtbl = &ocVtbl;
    oc->ref = 0;

    This = (IOleContainer*) oc;
    IOleContainer_AddRef(This);
    SHDOCVW_LockModule();

    return This;
}

/**********************/

typedef struct tag_iecs {
    IOleClientSiteVtbl *lpVtbl;
    IOleInPlaceSiteVtbl *lpInPlaceVtbl;
    LONG ref;
    IOleContainer *container;
    HWND hwnd;
    IOleObject *oo;
} iecs;

static iecs *iecs_from_IOleClientSite(IOleClientSite *iface)
{
    return (iecs*) iface;
}

static iecs *iecs_from_IOleInPlaceSite(IOleInPlaceSite *iface)
{
    return (iecs*) (((char*)iface) - FIELD_OFFSET(iecs,lpInPlaceVtbl));
}

static ULONG WINAPI iecs_AddRef(iecs *This)
{
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI iecs_Release(iecs *This)
{
    LONG ref = InterlockedDecrement(&This->ref);
    if (!ref)
    {
        if (This->hwnd)
            DestroyWindow(This->hwnd);
        IOleContainer_Release(This->container);
        HeapFree(GetProcessHeap(), 0, This);
        SHDOCVW_UnlockModule();
    }
    return ref;
}

static HRESULT WINAPI
iecs_QueryInterface(iecs *This, REFIID riid, void **ppv)
{
    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IOleClientSite))
    {
        iecs_AddRef(This);
        *ppv = &This->lpVtbl;;
        return S_OK;
    }
    if (IsEqualGUID(riid, &IID_IOleInPlaceSite) ||
        IsEqualGUID(riid, &IID_IOleWindow))
    {
        iecs_AddRef(This);
        *ppv = &This->lpInPlaceVtbl;
        return S_OK;
    }
    FIXME("unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static HRESULT WINAPI
cs_QueryInterface(IOleClientSite *iface, REFIID riid, void **ppv)
{
    iecs *This = iecs_from_IOleClientSite(iface);
    return iecs_QueryInterface(This, riid, ppv);
}

static ULONG WINAPI cs_AddRef(IOleClientSite *iface)
{
    iecs *This = iecs_from_IOleClientSite(iface);
    return iecs_AddRef(This);
}

static ULONG WINAPI cs_Release(IOleClientSite *iface)
{
    iecs *This = iecs_from_IOleClientSite(iface);
    return iecs_Release(This);
}

static HRESULT WINAPI cs_SaveObject(IOleClientSite *iface)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI cs_GetMoniker(IOleClientSite *iface, DWORD dwAssign,
                                    DWORD dwWhichMoniker, IMoniker **ppmk)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI
cs_GetContainer(IOleClientSite *iface, IOleContainer **ppContainer)
{
    iecs *This = iecs_from_IOleClientSite(iface);

    TRACE("\n");

    IOleContainer_AddRef(This->container);
    *ppContainer = This->container;
    return S_OK;
}

static HRESULT WINAPI cs_ShowObject(IOleClientSite *iface)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI cs_OnShowWindow(IOleClientSite *iface, BOOL fShow)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI cs_RequestNewObjectLayout(IOleClientSite *iface)
{
    FIXME("\n");
    return E_NOTIMPL;
}

struct IOleClientSiteVtbl csVtbl =
{
    cs_QueryInterface,
    cs_AddRef,
    cs_Release,
    cs_SaveObject,
    cs_GetMoniker,
    cs_GetContainer,
    cs_ShowObject,
    cs_OnShowWindow,
    cs_RequestNewObjectLayout,
};

static HRESULT WINAPI
is_QueryInterface(IOleInPlaceSite *iface, REFIID riid, void **ppv)
{
    iecs *This = iecs_from_IOleInPlaceSite(iface);
    return iecs_QueryInterface(This, riid, ppv);
}

static ULONG WINAPI is_AddRef(IOleInPlaceSite *iface)
{
    iecs *This = iecs_from_IOleInPlaceSite(iface);
    return iecs_AddRef(This);
}

static ULONG WINAPI is_Release(IOleInPlaceSite *iface)
{
    iecs *This = iecs_from_IOleInPlaceSite(iface);
    return iecs_Release(This);
}

static HRESULT WINAPI is_getWindow(IOleInPlaceSite *iface, HWND *phwnd)
{
    iecs *This = iecs_from_IOleInPlaceSite(iface);

    TRACE("\n");

    *phwnd = This->hwnd;
    return S_OK;
}

static HRESULT WINAPI
is_ContextSensitiveHelp(IOleInPlaceSite *iface, BOOL fEnterMode)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI is_CanInPlaceActivate(IOleInPlaceSite *iface)
{
    FIXME("\n");
    return S_OK;
}

static HRESULT WINAPI is_OnInPlaceActivate(IOleInPlaceSite *iface)
{
    TRACE("\n");
    return S_OK;
}

static HRESULT WINAPI is_OnUIActivate(IOleInPlaceSite *iface)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI
is_GetWindowContext(IOleInPlaceSite *iface, IOleInPlaceFrame **ppFrame,
                    IOleInPlaceUIWindow ** ppDoc, LPRECT lprcPosRect,
                    LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    iecs *This = iecs_from_IOleInPlaceSite(iface);

    TRACE("%p\n", This);

    *ppFrame = NULL;
    *ppDoc = NULL;
    GetClientRect(This->hwnd, lprcPosRect);
    GetClientRect(This->hwnd, lprcClipRect);
    if (lpFrameInfo->cb != sizeof *lpFrameInfo)
        ERR("frame info wrong size\n");
    lpFrameInfo->cAccelEntries = 0;
    lpFrameInfo->fMDIApp = FALSE;
    lpFrameInfo->haccel = 0;
    lpFrameInfo->hwndFrame = This->hwnd;

    return S_OK;
}

static HRESULT WINAPI is_Scroll(IOleInPlaceSite *iface, SIZE scrollExtent)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI is_OnUIDeactivate(IOleInPlaceSite *iface, BOOL fUndoable)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI is_OnInPlaceDeactivate(IOleInPlaceSite *iface)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI is_DiscardUndoState(IOleInPlaceSite *iface)
{
    FIXME("\n");
    return E_NOTIMPL;
}

struct IOleInPlaceSiteVtbl isVtbl =
{
    is_QueryInterface,
    is_AddRef,
    is_Release,
    is_getWindow,
    is_ContextSensitiveHelp,
    is_CanInPlaceActivate,
    is_OnInPlaceActivate,
    is_OnUIActivate,
    is_GetWindowContext,
    is_Scroll,
    is_OnUIDeactivate,
    is_OnInPlaceDeactivate,
    is_DiscardUndoState,
};

static const WCHAR szIEWinFrame[] = { 'I','E','F','r','a','m','e',0 };

static LRESULT iewnd_OnCreate(HWND hwnd, LPCREATESTRUCTW lpcs)
{
    SetWindowLongPtrW(hwnd, 0, (LONG) lpcs->lpCreateParams);
    return 0;
}

static LRESULT iewnd_OnSize(iecs *This, INT width, INT height)
{
    SIZEL sz;

    TRACE("%p %d %d\n", This, width, height);

    sz.cx = width;
    sz.cy = height;
    IOleObject_SetExtent(This->oo, DVASPECT_CONTENT, &sz);
    return 0;
}

static LRESULT iewnd_OnDestroy(iecs *This)
{
    TRACE("%p\n", This);

    This->hwnd = NULL;
    IOleObject_Close(This->oo, OLECLOSE_NOSAVE);
    IOleObject_Release(This->oo);
    This->oo = NULL;
    PostQuitMessage(0);

    return 0;
}

static LRESULT CALLBACK
ie_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    iecs *This = (iecs*) GetWindowLongPtrW(hwnd, 0);

    switch (msg)
    {
    case WM_CREATE:
        return iewnd_OnCreate(hwnd, (LPCREATESTRUCTW)lparam);
    case WM_DESTROY:
        return iewnd_OnDestroy(This);
    case WM_SIZE:
        return iewnd_OnSize(This, LOWORD(lparam), HIWORD(lparam));
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

static IOleClientSite * get_client_site(IOleObject *oo, HWND *phwnd)
{
    static const WCHAR winname[] = {
        'I','n','t','e','r','n','e','t',' ','E','x','p','l','o','r','e','r',0};
    IOleClientSite *This;
    HWND hwnd;
    iecs *cs;

    cs = HeapAlloc(GetProcessHeap(), 0, sizeof *cs);
    if (!cs)
        return NULL;

    cs->ref = 0;
    cs->lpVtbl = &csVtbl;
    cs->lpInPlaceVtbl = &isVtbl;
    cs->container = get_container();
    cs->oo = oo;

    hwnd = CreateWindowW(szIEWinFrame, winname, WS_VISIBLE|WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, shdocvw_hinstance, cs);
    if (!hwnd)
    {
        HeapFree(GetProcessHeap(), 0, cs);
        return NULL;
    }

    IOleObject_AddRef(oo);
    cs->hwnd = hwnd;
    *phwnd = hwnd;

    This = (IOleClientSite*) cs;
    IOleClientSite_AddRef(This);
    SHDOCVW_LockModule();

    return This;
}

void register_iewindow_class(void)
{
    WNDCLASSW wc;

    memset(&wc, 0, sizeof wc);
    wc.style = 0;
    wc.lpfnWndProc = ie_window_proc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(iecs *);
    wc.hInstance = shdocvw_hinstance;
    wc.hIcon = 0;
    wc.hCursor = LoadCursorW(0, MAKEINTRESOURCEW(IDI_APPLICATION));
    wc.hbrBackground = 0;
    wc.lpszClassName = szIEWinFrame;
    wc.lpszMenuName = NULL;

    RegisterClassW(&wc);
}

void unregister_iewindow_class(void)
{
    UnregisterClassW(szIEWinFrame, shdocvw_hinstance);
}

BOOL create_ie_window(LPCWSTR url)
{
    IWebBrowser2 *wb = NULL;
    IOleObject *oo = NULL;
    IOleClientSite *cs = NULL;
    HRESULT r;
    MSG msg;
    RECT rcClient = { 0,0,100,100 };
    BSTR bstrUrl;
    HWND hwnd = NULL;

    /* create the web browser */
    r = CoCreateInstance(&CLSID_WebBrowser, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IWebBrowser2, (LPVOID)&wb);
    if (FAILED(r))
        goto error;

    /* get its IOleObject interface */
    r = IWebBrowser2_QueryInterface(wb, &IID_IOleObject, (void**) &oo);
    if (FAILED(r))
        goto error;

    /* create the window frame for the browser object */
    cs = get_client_site(oo, &hwnd);
    if (!cs)
        goto error;

    /* attach the browser to the window frame */
    r = IOleObject_SetClientSite(oo, cs);
    if (FAILED(r))
        goto error;

    /* activate the browser */
    r = IOleObject_DoVerb(oo, OLEIVERB_INPLACEACTIVATE, &msg, cs, 0, hwnd, &rcClient);
    if (FAILED(r))
        goto error;

    /* navigate to the first page */
    bstrUrl = SysAllocString(url);
    IWebBrowser2_Navigate(wb, bstrUrl, NULL, NULL, NULL, NULL);

    /* run the message loop for this thread */
    while (GetMessageW(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    r = S_OK;

    /* clean up */
error:
    if (cs)
        IOleClientSite_Release(cs);
    if (oo)
        IOleObject_Release(oo);
    if (wb)
        IOleObject_Release(wb);

    return r;
}
