/*
 * Implementation of IWebBrowser interface for WebBrowser control
 *
 * Copyright 2001 John R. Sheets (for CodeWeavers)
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

#include "wine/debug.h"
#include "shdocvw.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

/**********************************************************************
 * Implement the IWebBrowser interface
 */

#define WEBBROWSER_THIS(iface) DEFINE_THIS(WebBrowser, WebBrowser, iface)

static HRESULT WINAPI WebBrowser_QueryInterface(IWebBrowser *iface, REFIID riid, LPVOID *ppv)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);

    if (ppv == NULL)
        return E_POINTER;
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = WEBBROWSER(This);
    }else if(IsEqualGUID (&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = WEBBROWSER(This);
    }else if(IsEqualGUID(&IID_IWebBrowser, riid)) {
        TRACE("(%p)->(IID_IWebBrowser %p)\n", This, ppv);
        *ppv = WEBBROWSER(This);
    }else if(IsEqualGUID(&IID_IOleObject, riid)) {
        TRACE("(%p)->(IID_IOleObject %p)\n", This, ppv);
        *ppv = OLEOBJ(This);
    }else if(IsEqualGUID(&IID_IOleWindow, riid)) {
        TRACE("(%p)->(IID_IOleWindow %p)\n", This, ppv);
        *ppv = INPLACEOBJ(This);
    }else if(IsEqualGUID (&IID_IOleInPlaceObject, riid)) {
        TRACE("(%p)->(IID_IOleInPlaceObject %p)\n", This, ppv);
        *ppv = INPLACEOBJ(This);
    }else if(IsEqualGUID(&IID_IOleControl, riid)) {
        TRACE("(%p)->(IID_IOleControl %p)\n", This, ppv);
        *ppv = CONTROL(This);
    }else if(IsEqualGUID(&IID_IPersist, riid)) {
        TRACE("(%p)->(IID_IPersist %p)\n", This, ppv);
        *ppv = PERSTORAGE(This);
    }else if(IsEqualGUID(&IID_IPersistStorage, riid)) {
        TRACE("(%p)->(IID_IPersistStorage %p)\n", This, ppv);
        *ppv = PERSTORAGE(This);
    }else if(IsEqualGUID (&IID_IPersistStreamInit, riid)) {
        TRACE("(%p)->(IID_IPersistStreamInit %p)\n", This, ppv);
        *ppv = PERSTRINIT(This);
    }else if(IsEqualGUID (&IID_IProvideClassInfo, riid)) {
        TRACE("(%p)->(IID_IProvideClassInfo %p)\n", This, ppv);
        *ppv = CLASSINFO(This);
    }else if(IsEqualGUID (&IID_IProvideClassInfo2, riid)) {
        TRACE("(%p)->(IID_IProvideClassInfo2 %p)\n", This, ppv);
        *ppv = CLASSINFO(This);
    }else if(IsEqualGUID (&IID_IQuickActivate, riid)) {
        FIXME("(%p)->(IID_IQuickActivate %p)\n", This, ppv);
        *ppv = &SHDOCVW_QuickActivate;
    }else if(IsEqualGUID (&IID_IConnectionPointContainer, riid)) {
        FIXME("(%p)->(IID_IConnectionPointContainer %p)\n", This, ppv);
        *ppv = &SHDOCVW_ConnectionPointContainer;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    FIXME("(%p)->(%s %p) interface not supported\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI WebBrowser_AddRef(IWebBrowser *iface)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%ld\n", This, ref);
    return ref;
}

static ULONG WINAPI WebBrowser_Release(IWebBrowser *iface)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        HeapFree(GetProcessHeap(), 0, This);
        SHDOCVW_UnlockModule();
    }

    return ref;
}

/* IDispatch methods */
static HRESULT WINAPI WebBrowser_GetTypeInfoCount(IWebBrowser *iface, UINT *pctinfo)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_GetTypeInfo(IWebBrowser *iface, UINT iTInfo, LCID lcid,
                                     LPTYPEINFO *ppTInfo)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%d %ld %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_GetIDsOfNames(IWebBrowser *iface, REFIID riid,
                                       LPOLESTR *rgszNames, UINT cNames,
                                       LCID lcid, DISPID *rgDispId)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%s %p %d %ld %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
            lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_Invoke(IWebBrowser *iface, DISPID dispIdMember,
                                REFIID riid, LCID lcid, WORD wFlags,
                                DISPPARAMS *pDispParams, VARIANT *pVarResult,
                                EXCEPINFO *pExepInfo, UINT *puArgErr)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%ld %s %ld %08x %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);
    return E_NOTIMPL;
}

/* IWebBrowser methods */
static HRESULT WINAPI WebBrowser_GoBack(IWebBrowser *iface)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_GoForward(IWebBrowser *iface)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_GoHome(IWebBrowser *iface)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_GoSearch(IWebBrowser *iface)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_Navigate(IWebBrowser *iface, BSTR URL,
                                  VARIANT *Flags, VARIANT *TargetFrameName,
                                  VARIANT *PostData, VARIANT *Headers)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%s %p %p %p %p)\n", This, debugstr_w(URL), Flags, TargetFrameName,
          PostData, Headers);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_Refresh(IWebBrowser *iface)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_Refresh2(IWebBrowser *iface, VARIANT *Level)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, Level);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_Stop(IWebBrowser *iface)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Application(IWebBrowser *iface, IDispatch **ppDisp)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppDisp);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Parent(IWebBrowser *iface, IDispatch **ppDisp)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppDisp);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Container(IWebBrowser *iface, IDispatch **ppDisp)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppDisp);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Document(IWebBrowser *iface, IDispatch **ppDisp)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppDisp);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_TopLevelContainer(IWebBrowser *iface, VARIANT_BOOL *pBool)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pBool);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Type(IWebBrowser *iface, BSTR *Type)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, Type);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Left(IWebBrowser *iface, long *pl)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pl);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_Left(IWebBrowser *iface, long Left)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, Left);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Top(IWebBrowser *iface, long *pl)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pl);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_Top(IWebBrowser *iface, long Top)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, Top);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Width(IWebBrowser *iface, long *pl)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pl);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_Width(IWebBrowser *iface, long Width)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, Width);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Height(IWebBrowser *iface, long *pl)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pl);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_Height(IWebBrowser *iface, long Height)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, Height);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_LocationName(IWebBrowser *iface, BSTR *LocationName)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, LocationName);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_LocationURL(IWebBrowser *iface, BSTR *LocationURL)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, LocationURL);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Busy(IWebBrowser *iface, VARIANT_BOOL *pBool)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pBool);
    return E_NOTIMPL;
}

#undef WEBBROWSER_THIS

/**********************************************************************
 * IWebBrowser virtual function table for IWebBrowser interface
 */

static const IWebBrowserVtbl WebBrowserVtbl =
{
    WebBrowser_QueryInterface,
    WebBrowser_AddRef,
    WebBrowser_Release,
    WebBrowser_GetTypeInfoCount,
    WebBrowser_GetTypeInfo,
    WebBrowser_GetIDsOfNames,
    WebBrowser_Invoke,
    WebBrowser_GoBack,
    WebBrowser_GoForward,
    WebBrowser_GoHome,
    WebBrowser_GoSearch,
    WebBrowser_Navigate,
    WebBrowser_Refresh,
    WebBrowser_Refresh2,
    WebBrowser_Stop,
    WebBrowser_get_Application,
    WebBrowser_get_Parent,
    WebBrowser_get_Container,
    WebBrowser_get_Document,
    WebBrowser_get_TopLevelContainer,
    WebBrowser_get_Type,
    WebBrowser_get_Left,
    WebBrowser_put_Left,
    WebBrowser_get_Top,
    WebBrowser_put_Top,
    WebBrowser_get_Width,
    WebBrowser_put_Width,
    WebBrowser_get_Height,
    WebBrowser_put_Height,
    WebBrowser_get_LocationName,
    WebBrowser_get_LocationURL,
    WebBrowser_get_Busy
};

HRESULT WebBrowser_Create(IUnknown *pOuter, REFIID riid, void **ppv)
{
    WebBrowser *ret;
    HRESULT hres;

    TRACE("(%p %s %p)\n", pOuter, debugstr_guid(riid), ppv);

    ret = HeapAlloc(GetProcessHeap(), 0, sizeof(WebBrowser));

    ret->lpWebBrowserVtbl = &WebBrowserVtbl;
    ret->ref = 0;

    WebBrowser_OleObject_Init(ret);
    WebBrowser_Persist_Init(ret);
    WebBrowser_ClassInfo_Init(ret);

    hres = IWebBrowser_QueryInterface(WEBBROWSER(ret), riid, ppv);
    if(SUCCEEDED(hres)) {
        SHDOCVW_LockModule();
    }else {
        HeapFree(GetProcessHeap(), 0, ret);
        return hres;
    }

    return hres;
}
