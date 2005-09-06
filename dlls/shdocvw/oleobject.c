/*
 * Implementation of IOleObject interfaces for WebBrowser control
 *
 * - IOleObject
 * - IOleInPlaceObject
 * - IOleControl
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

#include <string.h>
#include "wine/debug.h"
#include "shdocvw.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

/**********************************************************************
 * Implement the IOleObject interface for the WebBrowser control
 */

#define OLEOBJ_THIS(iface) DEFINE_THIS(WebBrowser, OleObject, iface)

static HRESULT WINAPI OleObject_QueryInterface(IOleObject *iface, REFIID riid, void **ppv)
{
    WebBrowser *This = OLEOBJ_THIS(iface);
    return IWebBrowser_QueryInterface(WEBBROWSER(This), riid, ppv);
}

static ULONG WINAPI OleObject_AddRef(IOleObject *iface)
{
    WebBrowser *This = OLEOBJ_THIS(iface);
    return IWebBrowser_AddRef(WEBBROWSER(This));
}

static ULONG WINAPI OleObject_Release(IOleObject *iface)
{
    WebBrowser *This = OLEOBJ_THIS(iface);
    return IWebBrowser_Release(WEBBROWSER(This));
}

static HRESULT WINAPI OleObject_SetClientSite(IOleObject *iface, LPOLECLIENTSITE pClientSite)
{
    WebBrowser *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pClientSite);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetClientSite(IOleObject *iface, LPOLECLIENTSITE *ppClientSite)
{
    WebBrowser *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppClientSite);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_SetHostNames(IOleObject *iface, LPCOLESTR szContainerApp,
        LPCOLESTR szContainerObj)
{
    WebBrowser *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%s, %s)\n", This, debugstr_w(szContainerApp), debugstr_w(szContainerObj));
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Close(IOleObject *iface, DWORD dwSaveOption)
{
    WebBrowser *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, dwSaveOption);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_SetMoniker(IOleObject *iface, DWORD dwWhichMoniker, IMoniker* pmk)
{
    WebBrowser *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%ld, %p)\n", This, dwWhichMoniker, pmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetMoniker(IOleObject *iface, DWORD dwAssign,
        DWORD dwWhichMoniker, LPMONIKER *ppmk)
{
    WebBrowser *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%ld, %ld, %p)\n", This, dwAssign, dwWhichMoniker, ppmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_InitFromData(IOleObject *iface, LPDATAOBJECT pDataObject,
        BOOL fCreation, DWORD dwReserved)
{
    WebBrowser *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%p, %d, %ld)\n", This, pDataObject, fCreation, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetClipboardData(IOleObject *iface, DWORD dwReserved,
        LPDATAOBJECT *ppDataObject)
{
    WebBrowser *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%ld, %p)\n", This, dwReserved, ppDataObject);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_DoVerb(IOleObject *iface, LONG iVerb, struct tagMSG* lpmsg,
        LPOLECLIENTSITE pActiveSite, LONG lindex, HWND hwndParent, LPCRECT lprcPosRect)
{
    WebBrowser *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%ld %p %p %ld %p %p)\n", This, iVerb, lpmsg, pActiveSite, lindex, hwndParent,
            lprcPosRect);
    switch (iVerb)
    {
    case OLEIVERB_INPLACEACTIVATE:
        FIXME ("stub for OLEIVERB_INPLACEACTIVATE\n");
        break;
    case OLEIVERB_HIDE:
        FIXME ("stub for OLEIVERB_HIDE\n");
        break;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_EnumVerbs(IOleObject *iface, IEnumOLEVERB **ppEnumOleVerb)
{
    WebBrowser *This = OLEOBJ_THIS(iface);
    TRACE("(%p)->(%p)\n", This, ppEnumOleVerb);
    return OleRegEnumVerbs(&CLSID_WebBrowser, ppEnumOleVerb);
}

static HRESULT WINAPI OleObject_Update(IOleObject *iface)
{
    WebBrowser *This = OLEOBJ_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_IsUpToDate(IOleObject *iface)
{
    WebBrowser *This = OLEOBJ_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetUserClassID(IOleObject *iface, CLSID* pClsid)
{
    WebBrowser *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pClsid);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetUserType(IOleObject *iface, DWORD dwFormOfType,
        LPOLESTR* pszUserType)
{
    WebBrowser *This = OLEOBJ_THIS(iface);
    TRACE("(%p, %ld, %p)\n", This, dwFormOfType, pszUserType);
    return OleRegGetUserType(&CLSID_WebBrowser, dwFormOfType, pszUserType);
}

static HRESULT WINAPI OleObject_SetExtent(IOleObject *iface, DWORD dwDrawAspect, SIZEL *psizel)
{
    WebBrowser *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%lx %p)\n", This, dwDrawAspect, psizel);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetExtent(IOleObject *iface, DWORD dwDrawAspect, SIZEL *psizel)
{
    WebBrowser *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%lx, %p)\n", This, dwDrawAspect, psizel);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Advise(IOleObject *iface, IAdviseSink *pAdvSink,
        DWORD* pdwConnection)
{
    WebBrowser *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%p, %p)\n", This, pAdvSink, pdwConnection);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Unadvise(IOleObject *iface, DWORD dwConnection)
{
    WebBrowser *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, dwConnection);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_EnumAdvise(IOleObject *iface, IEnumSTATDATA **ppenumAdvise)
{
    WebBrowser *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppenumAdvise);
    return S_OK;
}

static HRESULT WINAPI OleObject_GetMiscStatus(IOleObject *iface, DWORD dwAspect, DWORD *pdwStatus)
{
    WebBrowser *This = OLEOBJ_THIS(iface);
    HRESULT hres;

    TRACE("(%p)->(%lx, %p)\n", This, dwAspect, pdwStatus);

    hres = OleRegGetMiscStatus(&CLSID_WebBrowser, dwAspect, pdwStatus);

    if (FAILED(hres))
        *pdwStatus = 0;

    return S_OK;
}

static HRESULT WINAPI OleObject_SetColorScheme(IOleObject *iface, LOGPALETTE* pLogpal)
{
    WebBrowser *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pLogpal);
    return E_NOTIMPL;
}

#undef OLEOBJ_THIS

static const IOleObjectVtbl OleObjectVtbl =
{
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

/**********************************************************************
 * Implement the IOleInPlaceObject interface
 */

static HRESULT WINAPI WBOIPO_QueryInterface(LPOLEINPLACEOBJECT iface,
                                            REFIID riid, LPVOID *ppobj)
{
    FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));

    if (ppobj == NULL) return E_POINTER;
    
    return E_NOINTERFACE;
}

static ULONG WINAPI WBOIPO_AddRef(LPOLEINPLACEOBJECT iface)
{
    SHDOCVW_LockModule();

    return 2; /* non-heap based object */
}

static ULONG WINAPI WBOIPO_Release(LPOLEINPLACEOBJECT iface)
{
    SHDOCVW_UnlockModule();

    return 1; /* non-heap based object */
}

static HRESULT WINAPI WBOIPO_GetWindow(LPOLEINPLACEOBJECT iface, HWND* phwnd)
{
#if 0
    /* Create a fake window to fool MFC into believing that we actually
     * have an implemented browser control.  Avoids the assertion.
     */
    HWND hwnd;
    hwnd = CreateWindowA("BUTTON", "Web Control",
                        WS_HSCROLL | WS_VSCROLL | WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, CW_USEDEFAULT, 600,
                        400, NULL, NULL, NULL, NULL);

    *phwnd = hwnd;
    TRACE ("Returning hwnd = %d\n", hwnd);
#endif

    FIXME("stub HWND* = %p\n", phwnd);
    return S_OK;
}

static HRESULT WINAPI WBOIPO_ContextSensitiveHelp(LPOLEINPLACEOBJECT iface,
                                                  BOOL fEnterMode)
{
    FIXME("stub fEnterMode = %d\n", fEnterMode);
    return S_OK;
}

static HRESULT WINAPI WBOIPO_InPlaceDeactivate(LPOLEINPLACEOBJECT iface)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WBOIPO_UIDeactivate(LPOLEINPLACEOBJECT iface)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WBOIPO_SetObjectRects(LPOLEINPLACEOBJECT iface,
                                            LPCRECT lprcPosRect, LPCRECT lprcClipRect)
{
    FIXME("stub PosRect = %p, ClipRect = %p\n", lprcPosRect, lprcClipRect);
    return S_OK;
}

static HRESULT WINAPI WBOIPO_ReactivateAndUndo(LPOLEINPLACEOBJECT iface)
{
    FIXME("stub \n");
    return S_OK;
}

/**********************************************************************
 * IOleInPlaceObject virtual function table for IE Web Browser component
 */

static const IOleInPlaceObjectVtbl WBOIPO_Vtbl =
{
    WBOIPO_QueryInterface,
    WBOIPO_AddRef,
    WBOIPO_Release,
    WBOIPO_GetWindow,
    WBOIPO_ContextSensitiveHelp,
    WBOIPO_InPlaceDeactivate,
    WBOIPO_UIDeactivate,
    WBOIPO_SetObjectRects,
    WBOIPO_ReactivateAndUndo
};

IOleInPlaceObjectImpl SHDOCVW_OleInPlaceObject = {&WBOIPO_Vtbl};


/**********************************************************************
 * Implement the IOleControl interface
 */

static HRESULT WINAPI WBOC_QueryInterface(LPOLECONTROL iface,
                                          REFIID riid, LPVOID *ppobj)
{
    FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));

    if (ppobj == NULL) return E_POINTER;
    
    return E_NOINTERFACE;
}

static ULONG WINAPI WBOC_AddRef(LPOLECONTROL iface)
{
    SHDOCVW_LockModule();

    return 2; /* non-heap based object */
}

static ULONG WINAPI WBOC_Release(LPOLECONTROL iface)
{
    SHDOCVW_UnlockModule();

    return 1; /* non-heap based object */
}

static HRESULT WINAPI WBOC_GetControlInfo(LPOLECONTROL iface, LPCONTROLINFO pCI)
{
    FIXME("stub: LPCONTROLINFO = %p\n", pCI);
    return S_OK;
}

static HRESULT WINAPI WBOC_OnMnemonic(LPOLECONTROL iface, struct tagMSG *pMsg)
{
    FIXME("stub: MSG* = %p\n", pMsg);
    return S_OK;
}

static HRESULT WINAPI WBOC_OnAmbientPropertyChange(LPOLECONTROL iface, DISPID dispID)
{
    FIXME("stub: DISPID = %ld\n", dispID);
    return S_OK;
}

static HRESULT WINAPI WBOC_FreezeEvents(LPOLECONTROL iface, BOOL bFreeze)
{
    FIXME("stub: bFreeze = %d\n", bFreeze);
    return S_OK;
}

/**********************************************************************
 * IOleControl virtual function table for IE Web Browser component
 */

static const IOleControlVtbl WBOC_Vtbl =
{
    WBOC_QueryInterface,
    WBOC_AddRef,
    WBOC_Release,
    WBOC_GetControlInfo,
    WBOC_OnMnemonic,
    WBOC_OnAmbientPropertyChange,
    WBOC_FreezeEvents
};

IOleControlImpl SHDOCVW_OleControl = {&WBOC_Vtbl};

void WebBrowser_OleObject_Init(WebBrowser *This)
{
    This->lpOleObjectVtbl = &OleObjectVtbl;
}
