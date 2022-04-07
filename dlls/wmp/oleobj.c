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

#include "wmp_private.h"
#include "olectl.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wmp);

static HWND get_container_hwnd(WindowsMediaPlayer *This)
{
    IOleWindow *ole_window;
    HWND hwnd = NULL;
    HRESULT hres;

    /* IOleInPlaceSite (which inherits from IOleWindow) is preferred. */
    hres = IOleClientSite_QueryInterface(This->client_site, &IID_IOleInPlaceSite, (void**)&ole_window);
    if(FAILED(hres)) {
        hres = IOleClientSite_QueryInterface(This->client_site, &IID_IOleWindow, (void**)&ole_window);
        if(FAILED(hres)) {
            IOleContainer *container = NULL;

            hres = IOleClientSite_GetContainer(This->client_site, &container);
            if(SUCCEEDED(hres)) {
                hres = IOleContainer_QueryInterface(container, &IID_IOleWindow, (void**)&ole_window);
                IOleContainer_Release(container);
            }
        }
    }

    if(FAILED(hres))
        return NULL;

    hres = IOleWindow_GetWindow(ole_window, &hwnd);
    IOleWindow_Release(ole_window);
    if(FAILED(hres))
        return NULL;

    TRACE("Got window %p\n", hwnd);
    return hwnd;
}

static LRESULT WINAPI wmp_wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HFONT font;
        RECT rect;
        HDC hdc;

        TRACE("WM_PAINT\n");

        GetClientRect(hwnd, &rect);
        hdc = BeginPaint(hwnd, &ps);

        SelectObject(hdc, GetStockObject(DC_BRUSH));
        SetDCBrushColor(hdc, RGB(255,0,0));
        SetBkColor(hdc, RGB(255,0,0));

        font = CreateFontA(25,0,0,0,400,0,0,0,ANSI_CHARSET,0,0,DEFAULT_QUALITY,DEFAULT_PITCH,NULL);
        SelectObject(hdc, font);

        Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
        DrawTextA(hdc, "FIXME: WMP", -1, &rect, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

        DeleteObject(font);
        EndPaint(hwnd, &ps);
        break;
    }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);

}

static ATOM wmp_class;

static BOOL WINAPI register_wmp_class(INIT_ONCE *once, void *param, void **context)
{
    static WNDCLASSEXW wndclass = {
        sizeof(wndclass), CS_DBLCLKS, wmp_wnd_proc, 0, 0,
        NULL, NULL, NULL, NULL, NULL,
        /* It seems that native uses ATL for this. We use a fake name to make tests happy. */
        L"ATL:WMP", NULL
    };

    wndclass.hInstance = wmp_instance;
    wmp_class = RegisterClassExW(&wndclass);
    return TRUE;
}

void unregister_wmp_class(void)
{
    if(wmp_class)
        UnregisterClassW(MAKEINTRESOURCEW(wmp_class), wmp_instance);
}

static HWND create_wmp_window(WindowsMediaPlayer *wmp, const RECT *posrect)
{
    static INIT_ONCE class_init_once = INIT_ONCE_STATIC_INIT;

    InitOnceExecuteOnce(&class_init_once, register_wmp_class, NULL, NULL);
    if(!wmp_class)
        return NULL;

    return CreateWindowExW(0, MAKEINTRESOURCEW(wmp_class), NULL, WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_VISIBLE|WS_CHILD,
            posrect->left, posrect->top, posrect->right-posrect->left, posrect->bottom-posrect->top,
            get_container_hwnd(wmp), NULL, wmp_instance, NULL);
}

static HRESULT activate_inplace(WindowsMediaPlayer *This)
{
    IOleInPlaceSiteWindowless *ipsite_windowless;
    IOleInPlaceSiteEx *ipsiteex = NULL;
    IOleInPlaceSite *ipsite;
    IOleInPlaceUIWindow *ip_window = NULL;
    IOleInPlaceFrame *ip_frame = NULL;
    RECT posrect = {0}, cliprect = {0};
    OLEINPLACEFRAMEINFO frameinfo = { sizeof(frameinfo) };
    HRESULT hres;

    if(This->hwnd) {
        FIXME("Already activated\n");
        return E_UNEXPECTED;
    }

    hres = IOleClientSite_QueryInterface(This->client_site, &IID_IOleInPlaceSiteWindowless, (void**)&ipsite_windowless);
    if(SUCCEEDED(hres)) {
        hres = IOleInPlaceSiteWindowless_CanWindowlessActivate(ipsite_windowless);
        IOleInPlaceSiteWindowless_Release(ipsite_windowless);
        if(hres == S_OK)
            FIXME("Windowless activation not supported\n");
        ipsiteex = (IOleInPlaceSiteEx*)ipsite_windowless;
    }else {
        IOleClientSite_QueryInterface(This->client_site, &IID_IOleInPlaceSiteEx, (void**)&ipsiteex);
    }

    if(ipsiteex) {
        BOOL redraw = FALSE; /* Not really used. */
        IOleInPlaceSiteEx_OnInPlaceActivateEx(ipsiteex, &redraw, 0);
        ipsite = (IOleInPlaceSite*)ipsiteex;
    }else {
        IOleClientSite_QueryInterface(This->client_site, &IID_IOleInPlaceSite, (void**)&ipsite);
        if(FAILED(hres)) {
            FIXME("No IOleInPlaceSite instance\n");
            return hres;
        }

        IOleInPlaceSite_OnInPlaceActivate(ipsite);
    }

    hres = IOleInPlaceSite_GetWindowContext(ipsite, &ip_frame, &ip_window, &posrect, &cliprect, &frameinfo);
    IOleInPlaceSite_Release(ipsite);
    if(FAILED(hres)) {
        FIXME("GetWindowContext failed: %08lx\n", hres);
        return hres;
    }

    This->hwnd = create_wmp_window(This, &posrect);
    if(!This->hwnd)
        return E_FAIL;

    IOleClientSite_ShowObject(This->client_site);
    return S_OK;
}

static void deactivate_window(WindowsMediaPlayer *This)
{
    IOleInPlaceSite *ip_site;
    HRESULT hres;

    hres = IOleClientSite_QueryInterface(This->client_site, &IID_IOleInPlaceSite, (void**)&ip_site);
    if(SUCCEEDED(hres)) {
        IOleInPlaceSite_OnInPlaceDeactivate(ip_site);
        IOleInPlaceSite_Release(ip_site);
    }

    DestroyWindow(This->hwnd);
    This->hwnd = NULL;
}

static void release_client_site(WindowsMediaPlayer *This)
{
    if(!This->client_site)
        return;

    if(This->hwnd)
        deactivate_window(This);

    IOleClientSite_Release(This->client_site);
    This->client_site = NULL;
}

static inline WindowsMediaPlayer *impl_from_IOleObject(IOleObject *iface)
{
    return CONTAINING_RECORD(iface, WindowsMediaPlayer, IOleObject_iface);
}

static HRESULT WINAPI OleObject_QueryInterface(IOleObject *iface, REFIID riid, void **ppv)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);

    *ppv = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IOleObject_iface;
    }else if(IsEqualGUID(riid, &IID_IOleObject)) {
        TRACE("(%p)->(IID_IOleObject %p)\n", This, ppv);
        *ppv = &This->IOleObject_iface;
    }else if(IsEqualGUID(riid, &IID_IProvideClassInfo)) {
        TRACE("(%p)->(IID_IProvideClassInfo %p)\n", This, ppv);
        *ppv = &This->IProvideClassInfo2_iface;
    }else if(IsEqualGUID(riid, &IID_IProvideClassInfo2)) {
        TRACE("(%p)->(IID_IProvideClassInfo2 %p)\n", This, ppv);
        *ppv = &This->IProvideClassInfo2_iface;
    }else if(IsEqualGUID(riid, &IID_IPersist)) {
        TRACE("(%p)->(IID_IPersist %p)\n", This, ppv);
        *ppv = &This->IPersistStreamInit_iface;
    }else if(IsEqualGUID(riid, &IID_IPersistStreamInit)) {
        TRACE("(%p)->(IID_IPersistStreamInit %p)\n", This, ppv);
        *ppv = &This->IPersistStreamInit_iface;
    }else if(IsEqualGUID(riid, &IID_IOleWindow)) {
        TRACE("(%p)->(IID_IOleWindow %p)\n", This, ppv);
        *ppv = &This->IOleInPlaceObjectWindowless_iface;
    }else if(IsEqualGUID(riid, &IID_IOleInPlaceObject)) {
        TRACE("(%p)->(IID_IOleInPlaceObject %p)\n", This, ppv);
        *ppv = &This->IOleInPlaceObjectWindowless_iface;
    }else if(IsEqualGUID(riid, &IID_IOleInPlaceObjectWindowless)) {
        TRACE("(%p)->(IID_IOleInPlaceObjectWindowless %p)\n", This, ppv);
        *ppv = &This->IOleInPlaceObjectWindowless_iface;
    }else if(IsEqualGUID(riid, &IID_IConnectionPointContainer)) {
        TRACE("(%p)->(IID_IConnectionPointContainer %p)\n", This, ppv);
        *ppv = &This->IConnectionPointContainer_iface;
    }else if(IsEqualGUID(riid, &IID_IWMPCore)) {
        TRACE("(%p)->(IID_IWMPCore %p)\n", This, ppv);
        *ppv = &This->IWMPPlayer4_iface;
    }else if(IsEqualGUID(riid, &IID_IWMPCore2)) {
        TRACE("(%p)->(IID_IWMPCore2 %p)\n", This, ppv);
        *ppv = &This->IWMPPlayer4_iface;
    }else if(IsEqualGUID(riid, &IID_IWMPCore3)) {
        TRACE("(%p)->(IID_IWMPCore3 %p)\n", This, ppv);
        *ppv = &This->IWMPPlayer4_iface;
    }else if(IsEqualGUID(riid, &IID_IWMPPlayer4)) {
        TRACE("(%p)->(IID_IWMPPlayer4 %p)\n", This, ppv);
        *ppv = &This->IWMPPlayer4_iface;
    }else if(IsEqualGUID(riid, &IID_IWMPPlayer)) {
        TRACE("(%p)->(IID_IWMPPlayer %p)\n", This, ppv);
        *ppv = &This->IWMPPlayer_iface;
    }else if(IsEqualGUID(riid, &IID_IWMPSettings)) {
        TRACE("(%p)->(IID_IWMPSettings %p)\n", This, ppv);
        *ppv = &This->IWMPSettings_iface;
    }else if(IsEqualGUID(riid, &IID_IOleControl)) {
        TRACE("(%p)->(IID_IOleControl %p)\n", This, ppv);
        *ppv = &This->IOleControl_iface;
    }else if(IsEqualGUID(riid, &IID_IMarshal)) {
        TRACE("(%p)->(IID_IMarshal %p)\n", This, ppv);
        return E_NOINTERFACE;
    }else if(IsEqualGUID(riid, &IID_IQuickActivate)) {
        TRACE("(%p)->(IID_IQuickActivate %p)\n", This, ppv);
        return E_NOINTERFACE;
    }else {
        FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI OleObject_AddRef(IOleObject *iface)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI OleObject_Release(IOleObject *iface)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        release_client_site(This);
        destroy_player(This);
        ConnectionPointContainer_Destroy(This);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI OleObject_SetClientSite(IOleObject *iface, IOleClientSite *pClientSite)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    IOleControlSite *control_site;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, pClientSite);

    release_client_site(This);
    if(!pClientSite)
        return S_OK;

    IOleClientSite_AddRef(pClientSite);
    This->client_site = pClientSite;

    hres = IOleClientSite_QueryInterface(pClientSite, &IID_IOleControlSite, (void**)&control_site);
    if(SUCCEEDED(hres)) {
        IDispatch *disp;

        hres = IOleControlSite_GetExtendedControl(control_site, &disp);
        if(SUCCEEDED(hres) && disp) {
            FIXME("Use extended control\n");
            IDispatch_Release(disp);
        }

        IOleControlSite_Release(control_site);
    }

    return S_OK;
}

static HRESULT WINAPI OleObject_GetClientSite(IOleObject *iface, IOleClientSite **ppClientSite)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);

    TRACE("(%p)->(%p)\n", This, ppClientSite);

    *ppClientSite = This->client_site;
    return This->client_site ? S_OK : E_FAIL;
}

static HRESULT WINAPI OleObject_SetHostNames(IOleObject *iface, LPCOLESTR szContainerApp, LPCOLESTR szContainerObj)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_w(szContainerApp), debugstr_w(szContainerObj));
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Close(IOleObject *iface, DWORD dwSaveOption)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);

    TRACE("(%p)->(%08lx)\n", This, dwSaveOption);

    if(dwSaveOption)
        FIXME("Unsupported option %ld\n", dwSaveOption);

    if(This->hwnd) /* FIXME: Possibly hide window */
        deactivate_window(This);
    return S_OK;
}

static HRESULT WINAPI OleObject_SetMoniker(IOleObject *iface, DWORD dwWhichMoniker, IMoniker *pmk)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p %ld %p)->()\n", This, dwWhichMoniker, pmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetMoniker(IOleObject *iface, DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%ld %ld %p)\n", This, dwAssign, dwWhichMoniker, ppmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_InitFromData(IOleObject *iface, IDataObject *pDataObject, BOOL fCreation,
                                        DWORD dwReserved)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p %x %ld)\n", This, pDataObject, fCreation, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetClipboardData(IOleObject *iface, DWORD dwReserved, IDataObject **ppDataObject)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%ld %p)\n", This, dwReserved, ppDataObject);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_DoVerb(IOleObject *iface, LONG iVerb, LPMSG lpmsg, IOleClientSite *pActiveSite,
                                        LONG lindex, HWND hwndParent, LPCRECT lprcPosRect)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);

    switch(iVerb) {
    case OLEIVERB_INPLACEACTIVATE:
        TRACE("(%p)->(OLEIVERB_INPLACEACTIVATE)\n", This);
        return activate_inplace(This);

    case OLEIVERB_HIDE:
        if(!This->hwnd) {
            FIXME("No window to hide\n");
            return E_UNEXPECTED;
        }

        ShowWindow(This->hwnd, SW_HIDE);
        return S_OK;

    default:
        FIXME("Unsupported iVerb %ld\n", iVerb);
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_EnumVerbs(IOleObject *iface, IEnumOLEVERB **ppEnumOleVerb)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p)\n", This, ppEnumOleVerb);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Update(IOleObject *iface)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_IsUpToDate(IOleObject *iface)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetUserClassID(IOleObject *iface, CLSID *pClsid)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p)\n", This, pClsid);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetUserType(IOleObject *iface, DWORD dwFormOfType, LPOLESTR *pszUserType)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%ld %p)\n", This, dwFormOfType, pszUserType);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_SetExtent(IOleObject *iface, DWORD dwDrawAspect, SIZEL *psizel)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);

    TRACE("(%p)->(%ld %p)\n", This, dwDrawAspect, psizel);

    if(dwDrawAspect != DVASPECT_CONTENT)
        return DV_E_DVASPECT;

    This->extent = *psizel;
    return S_OK;
}

static HRESULT WINAPI OleObject_GetExtent(IOleObject *iface, DWORD dwDrawAspect, SIZEL *psizel)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);

    TRACE("(%p)->(%ld %p)\n", This, dwDrawAspect, psizel);

    if(dwDrawAspect != DVASPECT_CONTENT)
        return E_FAIL;

    *psizel = This->extent;
    return S_OK;
}

static HRESULT WINAPI OleObject_Advise(IOleObject *iface, IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p %p)\n", This, pAdvSink, pdwConnection);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Unadvise(IOleObject *iface, DWORD dwConnection)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%ld)\n", This, dwConnection);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_EnumAdvise(IOleObject *iface, IEnumSTATDATA **ppenumAdvise)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p)\n", This, ppenumAdvise);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetMiscStatus(IOleObject *iface, DWORD dwAspect, DWORD *pdwStatus)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);

    TRACE("(%p)->(%ld %p)\n", This, dwAspect, pdwStatus);

    switch(dwAspect) {
    case DVASPECT_CONTENT:
        *pdwStatus = OLEMISC_SETCLIENTSITEFIRST|OLEMISC_ACTIVATEWHENVISIBLE|OLEMISC_INSIDEOUT
            |OLEMISC_CANTLINKINSIDE|OLEMISC_RECOMPOSEONRESIZE;
        break;
    default:
        FIXME("Unhandled aspect %ld\n", dwAspect);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT WINAPI OleObject_SetColorScheme(IOleObject *iface, LOGPALETTE *pLogpal)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p)\n", This, pLogpal);
    return E_NOTIMPL;
}

static const IOleObjectVtbl OleObjectVtbl = {
    OleObject_QueryInterface,
    OleObject_AddRef,
    OleObject_Release,
    OleObject_SetClientSite,
    OleObject_GetClientSite,
    OleObject_SetHostNames,
    OleObject_Close,
    OleObject_SetMoniker,
    OleObject_GetMoniker,
    OleObject_InitFromData,
    OleObject_GetClipboardData,
    OleObject_DoVerb,
    OleObject_EnumVerbs,
    OleObject_Update,
    OleObject_IsUpToDate,
    OleObject_GetUserClassID,
    OleObject_GetUserType,
    OleObject_SetExtent,
    OleObject_GetExtent,
    OleObject_Advise,
    OleObject_Unadvise,
    OleObject_EnumAdvise,
    OleObject_GetMiscStatus,
    OleObject_SetColorScheme
};

static inline WindowsMediaPlayer *impl_from_IOleInPlaceObjectWindowless(IOleInPlaceObjectWindowless *iface)
{
    return CONTAINING_RECORD(iface, WindowsMediaPlayer, IOleInPlaceObjectWindowless_iface);
}

static HRESULT WINAPI OleInPlaceObjectWindowless_QueryInterface(IOleInPlaceObjectWindowless *iface,
        REFIID riid, void **ppv)
{
    WindowsMediaPlayer *This = impl_from_IOleInPlaceObjectWindowless(iface);
    return IOleObject_QueryInterface(&This->IOleObject_iface, riid, ppv);
}

static ULONG WINAPI OleInPlaceObjectWindowless_AddRef(IOleInPlaceObjectWindowless *iface)
{
    WindowsMediaPlayer *This = impl_from_IOleInPlaceObjectWindowless(iface);
    return IOleObject_AddRef(&This->IOleObject_iface);
}

static ULONG WINAPI OleInPlaceObjectWindowless_Release(IOleInPlaceObjectWindowless *iface)
{
    WindowsMediaPlayer *This = impl_from_IOleInPlaceObjectWindowless(iface);
    return IOleObject_Release(&This->IOleObject_iface);
}

static HRESULT WINAPI OleInPlaceObjectWindowless_GetWindow(IOleInPlaceObjectWindowless *iface, HWND *phwnd)
{
    WindowsMediaPlayer *This = impl_from_IOleInPlaceObjectWindowless(iface);

    TRACE("(%p)->(%p)\n", This, phwnd);

    *phwnd = This->hwnd;
    return This->hwnd ? S_OK : E_UNEXPECTED;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_ContextSensitiveHelp(IOleInPlaceObjectWindowless *iface,
        BOOL fEnterMode)
{
    WindowsMediaPlayer *This = impl_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)->(%x)\n", This, fEnterMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_InPlaceDeactivate(IOleInPlaceObjectWindowless *iface)
{
    WindowsMediaPlayer *This = impl_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_UIDeactivate(IOleInPlaceObjectWindowless *iface)
{
    WindowsMediaPlayer *This = impl_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_SetObjectRects(IOleInPlaceObjectWindowless *iface,
        LPCRECT lprcPosRect, LPCRECT lprcClipRect)
{
    WindowsMediaPlayer *This = impl_from_IOleInPlaceObjectWindowless(iface);

    TRACE("(%p)->(%s %s)\n", This, wine_dbgstr_rect(lprcPosRect), wine_dbgstr_rect(lprcClipRect));

    if(This->hwnd) {
       SetWindowPos(This->hwnd, NULL, lprcPosRect->left, lprcPosRect->top,
               lprcPosRect->right-lprcPosRect->left, lprcPosRect->bottom-lprcPosRect->top,
               SWP_NOZORDER | SWP_NOACTIVATE);
    }

    return S_OK;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_ReactivateAndUndo(IOleInPlaceObjectWindowless *iface)
{
    WindowsMediaPlayer *This = impl_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_OnWindowMessage(IOleInPlaceObjectWindowless *iface,
        UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *lpResult)
{
    WindowsMediaPlayer *This = impl_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)->(%u %Iu %Iu %p)\n", This, msg, wParam, lParam, lpResult);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_GetDropTarget(IOleInPlaceObjectWindowless *iface,
        IDropTarget **ppDropTarget)
{
    WindowsMediaPlayer *This = impl_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)->(%p)\n", This, ppDropTarget);
    return E_NOTIMPL;
}

static const IOleInPlaceObjectWindowlessVtbl OleInPlaceObjectWindowlessVtbl = {
    OleInPlaceObjectWindowless_QueryInterface,
    OleInPlaceObjectWindowless_AddRef,
    OleInPlaceObjectWindowless_Release,
    OleInPlaceObjectWindowless_GetWindow,
    OleInPlaceObjectWindowless_ContextSensitiveHelp,
    OleInPlaceObjectWindowless_InPlaceDeactivate,
    OleInPlaceObjectWindowless_UIDeactivate,
    OleInPlaceObjectWindowless_SetObjectRects,
    OleInPlaceObjectWindowless_ReactivateAndUndo,
    OleInPlaceObjectWindowless_OnWindowMessage,
    OleInPlaceObjectWindowless_GetDropTarget
};

static inline WindowsMediaPlayer *impl_from_IOleControl(IOleControl *iface)
{
    return CONTAINING_RECORD(iface, WindowsMediaPlayer, IOleControl_iface);
}

static HRESULT WINAPI OleControl_QueryInterface(IOleControl *iface, REFIID riid, void **ppv)
{
    WindowsMediaPlayer *This = impl_from_IOleControl(iface);
    return IOleObject_QueryInterface(&This->IOleObject_iface, riid, ppv);
}

static ULONG WINAPI OleControl_AddRef(IOleControl *iface)
{
    WindowsMediaPlayer *This = impl_from_IOleControl(iface);
    return IOleObject_AddRef(&This->IOleObject_iface);
}

static ULONG WINAPI OleControl_Release(IOleControl *iface)
{
    WindowsMediaPlayer *This = impl_from_IOleControl(iface);
    return IOleObject_Release(&This->IOleObject_iface);
}

static HRESULT WINAPI OleControl_GetControlInfo(IOleControl *iface, CONTROLINFO *pCI)
{
    WindowsMediaPlayer *This = impl_from_IOleControl(iface);
    FIXME("(%p)->(%p)\n", This, pCI);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleControl_OnMnemonic(IOleControl *iface, MSG *msg)
{
    WindowsMediaPlayer *This = impl_from_IOleControl(iface);
    FIXME("(%p)->(%p)\n", This, msg);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleControl_OnAmbientPropertyChange(IOleControl *iface, DISPID dispID)
{
    WindowsMediaPlayer *This = impl_from_IOleControl(iface);
    FIXME("(%p)->(%ld)\n", This, dispID);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleControl_FreezeEvents(IOleControl *iface, BOOL freeze)
{
    WindowsMediaPlayer *This = impl_from_IOleControl(iface);
    FIXME("(%p)->(%x)\n", This, freeze);
    return E_NOTIMPL;
}

static const IOleControlVtbl OleControlVtbl = {
    OleControl_QueryInterface,
    OleControl_AddRef,
    OleControl_Release,
    OleControl_GetControlInfo,
    OleControl_OnMnemonic,
    OleControl_OnAmbientPropertyChange,
    OleControl_FreezeEvents
};

static inline WindowsMediaPlayer *impl_from_IProvideClassInfo2(IProvideClassInfo2 *iface)
{
    return CONTAINING_RECORD(iface, WindowsMediaPlayer, IProvideClassInfo2_iface);
}

static HRESULT WINAPI ProvideClassInfo2_QueryInterface(IProvideClassInfo2 *iface, REFIID riid, void **ppv)
{
    WindowsMediaPlayer *This = impl_from_IProvideClassInfo2(iface);
    return IOleObject_QueryInterface(&This->IOleObject_iface, riid, ppv);
}

static ULONG WINAPI ProvideClassInfo2_AddRef(IProvideClassInfo2 *iface)
{
    WindowsMediaPlayer *This = impl_from_IProvideClassInfo2(iface);
    return IOleObject_AddRef(&This->IOleObject_iface);
}

static ULONG WINAPI ProvideClassInfo2_Release(IProvideClassInfo2 *iface)
{
    WindowsMediaPlayer *This = impl_from_IProvideClassInfo2(iface);
    return IOleObject_Release(&This->IOleObject_iface);
}

static HRESULT WINAPI ProvideClassInfo2_GetClassInfo(IProvideClassInfo2 *iface, ITypeInfo **ti)
{
    WindowsMediaPlayer *This = impl_from_IProvideClassInfo2(iface);

    TRACE("(%p)->(%p)\n", This, ti);

    if (!ti)
        return E_POINTER;

    return get_typeinfo(WindowsMediaPlayer_tid, ti);
}

static HRESULT WINAPI ProvideClassInfo2_GetGUID(IProvideClassInfo2 *iface, DWORD dwGuidKind, GUID *pGUID)
{
    WindowsMediaPlayer *This = impl_from_IProvideClassInfo2(iface);

    TRACE("(%p)->(%ld %p)\n", This, dwGuidKind, pGUID);

    if(dwGuidKind != GUIDKIND_DEFAULT_SOURCE_DISP_IID) {
        FIXME("Unexpected dwGuidKind %ld\n", dwGuidKind);
        return E_INVALIDARG;
    }

    *pGUID = IID__WMPOCXEvents;
    return S_OK;
}

static const IProvideClassInfo2Vtbl ProvideClassInfo2Vtbl = {
    ProvideClassInfo2_QueryInterface,
    ProvideClassInfo2_AddRef,
    ProvideClassInfo2_Release,
    ProvideClassInfo2_GetClassInfo,
    ProvideClassInfo2_GetGUID
};

static inline WindowsMediaPlayer *impl_from_IPersistStreamInit(IPersistStreamInit *iface)
{
    return CONTAINING_RECORD(iface, WindowsMediaPlayer, IPersistStreamInit_iface);
}

static HRESULT WINAPI PersistStreamInit_QueryInterface(IPersistStreamInit *iface,
                                                       REFIID riid, void **ppv)
{
    WindowsMediaPlayer *This = impl_from_IPersistStreamInit(iface);
    return IOleObject_QueryInterface(&This->IOleObject_iface, riid, ppv);
}

static ULONG WINAPI PersistStreamInit_AddRef(IPersistStreamInit *iface)
{
    WindowsMediaPlayer *This = impl_from_IPersistStreamInit(iface);
    return IOleObject_AddRef(&This->IOleObject_iface);
}

static ULONG WINAPI PersistStreamInit_Release(IPersistStreamInit *iface)
{
    WindowsMediaPlayer *This = impl_from_IPersistStreamInit(iface);
    return IOleObject_Release(&This->IOleObject_iface);
}

static HRESULT WINAPI PersistStreamInit_GetClassID(IPersistStreamInit *iface, CLSID *pClassID)
{
    WindowsMediaPlayer *This = impl_from_IPersistStreamInit(iface);
    FIXME("(%p)->(%p)\n", This, pClassID);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStreamInit_IsDirty(IPersistStreamInit *iface)
{
    WindowsMediaPlayer *This = impl_from_IPersistStreamInit(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStreamInit_Load(IPersistStreamInit *iface, LPSTREAM pStm)
{
    WindowsMediaPlayer *This = impl_from_IPersistStreamInit(iface);
    FIXME("(%p)->(%p)\n", This, pStm);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStreamInit_Save(IPersistStreamInit *iface, LPSTREAM pStm,
                                             BOOL fClearDirty)
{
    WindowsMediaPlayer *This = impl_from_IPersistStreamInit(iface);
    FIXME("(%p)->(%p %x)\n", This, pStm, fClearDirty);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStreamInit_GetSizeMax(IPersistStreamInit *iface,
                                                   ULARGE_INTEGER *pcbSize)
{
    WindowsMediaPlayer *This = impl_from_IPersistStreamInit(iface);
    FIXME("(%p)->(%p)\n", This, pcbSize);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStreamInit_InitNew(IPersistStreamInit *iface)
{
    WindowsMediaPlayer *This = impl_from_IPersistStreamInit(iface);

    TRACE("(%p)\n", This);

    if(!This->client_site)
        return E_FAIL;

    /* Nothing to do, yet. */
    get_container_hwnd(This);
    return S_OK;
}

static const IPersistStreamInitVtbl PersistStreamInitVtbl = {
    PersistStreamInit_QueryInterface,
    PersistStreamInit_AddRef,
    PersistStreamInit_Release,
    PersistStreamInit_GetClassID,
    PersistStreamInit_IsDirty,
    PersistStreamInit_Load,
    PersistStreamInit_Save,
    PersistStreamInit_GetSizeMax,
    PersistStreamInit_InitNew
};

HRESULT WINAPI WMPFactory_CreateInstance(IClassFactory *iface, IUnknown *outer,
        REFIID riid, void **ppv)
{
    WindowsMediaPlayer *wmp;
    DWORD dpi_x, dpi_y;
    HDC hdc;
    HRESULT hres;

    TRACE("(%p %s %p)\n", outer, debugstr_guid(riid), ppv);

    wmp = calloc(1, sizeof(*wmp));
    if(!wmp)
        return E_OUTOFMEMORY;

    wmp->IOleObject_iface.lpVtbl = &OleObjectVtbl;
    wmp->IProvideClassInfo2_iface.lpVtbl = &ProvideClassInfo2Vtbl;
    wmp->IPersistStreamInit_iface.lpVtbl = &PersistStreamInitVtbl;
    wmp->IOleInPlaceObjectWindowless_iface.lpVtbl = &OleInPlaceObjectWindowlessVtbl;
    wmp->IOleControl_iface.lpVtbl = &OleControlVtbl;

    wmp->ref = 1;

    if (init_player(wmp)) {
        ConnectionPointContainer_Init(wmp);
        hdc = GetDC(0);
        dpi_x = GetDeviceCaps(hdc, LOGPIXELSX);
        dpi_y = GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(0, hdc);

        wmp->extent.cx = MulDiv(192, 2540, dpi_x);
        wmp->extent.cy = MulDiv(192, 2540, dpi_y);

        hres = IOleObject_QueryInterface(&wmp->IOleObject_iface, riid, ppv);
    } else {
        hres = E_FAIL;
    }
    IOleObject_Release(&wmp->IOleObject_iface);
    return hres;
}
