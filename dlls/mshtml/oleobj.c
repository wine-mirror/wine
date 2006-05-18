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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "shlguid.h"
#include "mshtmdid.h"
#include "idispids.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

/**********************************************************
 * IOleObject implementation
 */

#define OLEOBJ_THIS(iface) DEFINE_THIS(HTMLDocument, OleObject, iface)

static HRESULT WINAPI OleObject_QueryInterface(IOleObject *iface, REFIID riid, void **ppvObject)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    return IHTMLDocument2_QueryInterface(HTMLDOC(This), riid, ppvObject);
}

static ULONG WINAPI OleObject_AddRef(IOleObject *iface)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    return IHTMLDocument2_AddRef(HTMLDOC(This));
}

static ULONG WINAPI OleObject_Release(IOleObject *iface)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    return IHTMLDocument2_Release(HTMLDOC(This));
}

static HRESULT WINAPI OleObject_SetClientSite(IOleObject *iface, IOleClientSite *pClientSite)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    IDocHostUIHandler *pDocHostUIHandler = NULL;
    IOleCommandTarget *cmdtrg = NULL;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, pClientSite);

    if(pClientSite == This->client)
        return S_OK;

    if(This->client) {
        IOleClientSite_Release(This->client);
        This->client = NULL;
    }

    if(This->hostui) {
        IDocHostUIHandler_Release(This->hostui);
        This->hostui = NULL;
    }

    if(!pClientSite)
        return S_OK;

    hres = IOleObject_QueryInterface(pClientSite, &IID_IDocHostUIHandler, (void**)&pDocHostUIHandler);
    if(SUCCEEDED(hres)) {
        DOCHOSTUIINFO hostinfo;
        LPOLESTR key_path = NULL, override_key_path = NULL;
        IDocHostUIHandler2 *pDocHostUIHandler2;

        memset(&hostinfo, 0, sizeof(DOCHOSTUIINFO));
        hostinfo.cbSize = sizeof(DOCHOSTUIINFO);
        hres = IDocHostUIHandler_GetHostInfo(pDocHostUIHandler, &hostinfo);
        if(SUCCEEDED(hres))
            /* FIXME: use hostinfo */
            TRACE("hostinfo = {%lu %08lx %08lx %s %s}\n",
                    hostinfo.cbSize, hostinfo.dwFlags, hostinfo.dwDoubleClick,
                    debugstr_w(hostinfo.pchHostCss), debugstr_w(hostinfo.pchHostNS));

        if(!This->has_key_path) {
            hres = IDocHostUIHandler_GetOptionKeyPath(pDocHostUIHandler, &key_path, 0);
            if(hres == S_OK && key_path) {
                if(key_path[0]) {
                    /* FIXME: use key_path */
                    TRACE("key_path = %s\n", debugstr_w(key_path));
                }
                CoTaskMemFree(key_path);
            }

            hres = IDocHostUIHandler_QueryInterface(pDocHostUIHandler, &IID_IDocHostUIHandler2,
                    (void**)&pDocHostUIHandler2);
            if(SUCCEEDED(hres)) {
                hres = IDocHostUIHandler2_GetOverrideKeyPath(pDocHostUIHandler2, &override_key_path, 0);
                if(hres == S_OK && override_key_path && override_key_path[0]) {
                    if(override_key_path[0]) {
                        /*FIXME: use override_key_path */
                        TRACE("override_key_path = %s\n", debugstr_w(override_key_path));
                    }
                    CoTaskMemFree(override_key_path);
                }
                IDocHostUIHandler2_Release(pDocHostUIHandler2);
            }

            This->has_key_path = TRUE;
        }
    }

    /* Native calls here GetWindow. What is it for?
     * We don't have anything to do with it here (yet). */
    if(pClientSite) {
        IOleWindow *pOleWindow = NULL;
        HWND hwnd;

        hres = IOleClientSite_QueryInterface(pClientSite, &IID_IOleWindow, (void**)&pOleWindow);
        if(SUCCEEDED(hres)) {
            IOleWindow_GetWindow(pOleWindow, &hwnd);
            IOleWindow_Release(pOleWindow);
        }
    }

    hres = IOleClientSite_QueryInterface(pClientSite, &IID_IOleCommandTarget, (void**)&cmdtrg);
    if(SUCCEEDED(hres)) {
        VARIANT var;
        OLECMD cmd = {OLECMDID_SETPROGRESSTEXT, 0};

        IOleCommandTarget_QueryStatus(cmdtrg, NULL, 1, &cmd, NULL);

        V_VT(&var) = VT_I4;
        V_I4(&var) = 0;
        IOleCommandTarget_Exec(cmdtrg, NULL, OLECMDID_SETPROGRESSMAX, 0, &var, NULL);
        IOleCommandTarget_Exec(cmdtrg, NULL, OLECMDID_SETPROGRESSPOS, 0, &var, NULL);

        IOleCommandTarget_Release(cmdtrg);
    }

    IOleClientSite_AddRef(pClientSite);
    This->client = pClientSite;
    This->hostui = pDocHostUIHandler;

    IOleControl_OnAmbientPropertyChange(CONTROL(This), DISPID_AMBIENT_USERMODE);
    IOleControl_OnAmbientPropertyChange(CONTROL(This), DISPID_AMBIENT_OFFLINEIFNOTCONNECTED); 
    IOleControl_OnAmbientPropertyChange(CONTROL(This), DISPID_AMBIENT_SILENT);
    IOleControl_OnAmbientPropertyChange(CONTROL(This), DISPID_AMBIENT_USERAGENT);
    IOleControl_OnAmbientPropertyChange(CONTROL(This), DISPID_AMBIENT_PALETTE);

    return S_OK;
}

static HRESULT WINAPI OleObject_GetClientSite(IOleObject *iface, IOleClientSite **ppClientSite)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);

    TRACE("(%p)->(%p)\n", This, ppClientSite);

    if(!ppClientSite)
        return E_INVALIDARG;

    if(This->client)
        IOleClientSite_AddRef(This->client);
    *ppClientSite = This->client;

    return S_OK;
}

static HRESULT WINAPI OleObject_SetHostNames(IOleObject *iface, LPCOLESTR szContainerApp, LPCOLESTR szContainerObj)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_w(szContainerApp), debugstr_w(szContainerObj));
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Close(IOleObject *iface, DWORD dwSaveOption)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);

    TRACE("(%p)->(%08lx)\n", This, dwSaveOption);

    if(dwSaveOption == OLECLOSE_PROMPTSAVE)
        FIXME("OLECLOSE_PROMPTSAVE not implemented\n");

    if(This->in_place_active)
        IOleInPlaceObjectWindowless_InPlaceDeactivate(INPLACEWIN(This));

    HTMLDocument_LockContainer(This, FALSE);
    
    return S_OK;
}

static HRESULT WINAPI OleObject_SetMoniker(IOleObject *iface, DWORD dwWhichMoniker, IMoniker *pmk)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p %ld %p)->()\n", This, dwWhichMoniker, pmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetMoniker(IOleObject *iface, DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%ld %ld %p)\n", This, dwAssign, dwWhichMoniker, ppmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_InitFromData(IOleObject *iface, IDataObject *pDataObject, BOOL fCreation,
                                        DWORD dwReserved)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%p %x %ld)\n", This, pDataObject, fCreation, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetClipboardData(IOleObject *iface, DWORD dwReserved, IDataObject **ppDataObject)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%ld %p)\n", This, dwReserved, ppDataObject);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_DoVerb(IOleObject *iface, LONG iVerb, LPMSG lpmsg, IOleClientSite *pActiveSite,
                                        LONG lindex, HWND hwndParent, LPCRECT lprcPosRect)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    IOleDocumentSite *pDocSite;
    HRESULT hres;

    TRACE("(%p)->(%ld %p %p %ld %p %p)\n", This, iVerb, lpmsg, pActiveSite, lindex, hwndParent, lprcPosRect);

    if(iVerb != OLEIVERB_SHOW && iVerb != OLEIVERB_UIACTIVATE && iVerb != OLEIVERB_INPLACEACTIVATE) { 
        FIXME("iVerb = %ld not supported\n", iVerb);
        return E_NOTIMPL;
    }

    if(!pActiveSite)
        pActiveSite = This->client;

    hres = IOleClientSite_QueryInterface(pActiveSite, &IID_IOleDocumentSite, (void**)&pDocSite);
    if(SUCCEEDED(hres)) {
        HTMLDocument_LockContainer(This, TRUE);

        /* FIXME: Create new IOleDocumentView. See CreateView for more info. */
        hres = IOleDocumentSite_ActivateMe(pDocSite, DOCVIEW(This));
        IOleDocumentSite_Release(pDocSite);
    }else {
        hres = IOleDocumentView_UIActivate(DOCVIEW(This), TRUE);
        if(SUCCEEDED(hres)) {
            if(lprcPosRect) {
                RECT rect; /* We need to pass rect as not const pointer */
                memcpy(&rect, lprcPosRect, sizeof(RECT));
                IOleDocumentView_SetRect(DOCVIEW(This), &rect);
            }
            IOleDocumentView_Show(DOCVIEW(This), TRUE);
        }
    }

    return hres;
}

static HRESULT WINAPI OleObject_EnumVerbs(IOleObject *iface, IEnumOLEVERB **ppEnumOleVerb)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppEnumOleVerb);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Update(IOleObject *iface)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_IsUpToDate(IOleObject *iface)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetUserClassID(IOleObject *iface, CLSID *pClsid)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pClsid);

    if(!pClsid)
        return E_INVALIDARG;

    memcpy(pClsid, &CLSID_HTMLDocument, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI OleObject_GetUserType(IOleObject *iface, DWORD dwFormOfType, LPOLESTR *pszUserType)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%ld %p)\n", This, dwFormOfType, pszUserType);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_SetExtent(IOleObject *iface, DWORD dwDrawAspect, SIZEL *psizel)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%ld %p)\n", This, dwDrawAspect, psizel);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetExtent(IOleObject *iface, DWORD dwDrawAspect, SIZEL *psizel)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%ld %p)\n", This, dwDrawAspect, psizel);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Advise(IOleObject *iface, IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pAdvSink, pdwConnection);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Unadvise(IOleObject *iface, DWORD dwConnection)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, dwConnection);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_EnumAdvise(IOleObject *iface, IEnumSTATDATA **ppenumAdvise)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppenumAdvise);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetMiscStatus(IOleObject *iface, DWORD dwAspect, DWORD *pdwStatus)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%ld %p)\n", This, dwAspect, pdwStatus);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_SetColorScheme(IOleObject *iface, LOGPALETTE *pLogpal)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pLogpal);
    return E_NOTIMPL;
}

#undef OLEPBJ_THIS

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

/**********************************************************
 * IOleDocument implementation
 */

#define OLEDOC_THIS(iface) DEFINE_THIS(HTMLDocument, OleDocument, iface)

static HRESULT WINAPI OleDocument_QueryInterface(IOleDocument *iface, REFIID riid, void **ppvObject)
{
    HTMLDocument *This = OLEDOC_THIS(iface);
    return IHTMLDocument2_QueryInterface(HTMLDOC(This), riid, ppvObject);
}

static ULONG WINAPI OleDocument_AddRef(IOleDocument *iface)
{
    HTMLDocument *This = OLEDOC_THIS(iface);
    return IHTMLDocument2_AddRef(HTMLDOC(This));
}

static ULONG WINAPI OleDocument_Release(IOleDocument *iface)
{
    HTMLDocument *This = OLEDOC_THIS(iface);
    return IHTMLDocument2_Release(HTMLDOC(This));
}

static HRESULT WINAPI OleDocument_CreateView(IOleDocument *iface, IOleInPlaceSite *pIPSite, IStream *pstm,
                                   DWORD dwReserved, IOleDocumentView **ppView)
{
    HTMLDocument *This = OLEDOC_THIS(iface);
    HRESULT hres;

    TRACE("(%p)->(%p %p %ld %p)\n", This, pIPSite, pstm, dwReserved, ppView);

    if(!ppView)
        return E_INVALIDARG;

    /* FIXME:
     * Windows implementation creates new IOleDocumentView when function is called for the
     * first time and returns E_FAIL when it is called for the second time, but it doesn't matter
     * if the application uses returned interfaces, passed to ActivateMe or returned by
     * QueryInterface, so there is no reason to create new interface. This needs more testing.
     */

    if(pIPSite) {
        hres = IOleDocumentView_SetInPlaceSite(DOCVIEW(This), pIPSite);
        if(FAILED(hres))
            return hres;
    }

    if(pstm)
        FIXME("pstm is not supported\n");

    IOleDocumentView_AddRef(DOCVIEW(This));
    *ppView = DOCVIEW(This);
    return S_OK;
}

static HRESULT WINAPI OleDocument_GetDocMiscStatus(IOleDocument *iface, DWORD *pdwStatus)
{
    HTMLDocument *This = OLEDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pdwStatus);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleDocument_EnumViews(IOleDocument *iface, IEnumOleDocumentViews **ppEnum,
                                   IOleDocumentView **ppView)
{
    HTMLDocument *This = OLEDOC_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, ppEnum, ppView);
    return E_NOTIMPL;
}

#undef OLEDOC_THIS

static const IOleDocumentVtbl OleDocumentVtbl = {
    OleDocument_QueryInterface,
    OleDocument_AddRef,
    OleDocument_Release,
    OleDocument_CreateView,
    OleDocument_GetDocMiscStatus,
    OleDocument_EnumViews
};

/**********************************************************
 * IOleCommandTarget implementation
 */

#define CMDTARGET_THIS(iface) DEFINE_THIS(HTMLDocument, OleCommandTarget, iface)

static HRESULT exec_open(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_new(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_save(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_save_as(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_save_copy_as(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_print(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_print_preview(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_page_setup(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_spell(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_properties(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_cut(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_copy(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_paste(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_paste_special(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_undo(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_rendo(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_select_all(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_clear_selection(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_zoom(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_get_zoom_range(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_refresh(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_stop(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_stop_download(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_delete(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_enable_interaction(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_on_unload(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    TRACE("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);

    /* Tests show that we have nothing more to do here */

    if(pvaOut) {
        V_VT(pvaOut) = VT_BOOL;
        V_BOOL(pvaOut) = VARIANT_TRUE;
    }

    return S_OK;
}

static HRESULT exec_show_page_setup(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_show_print(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_close(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_set_print_template(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_get_print_template(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static const struct {
    OLECMDF cmdf;
    HRESULT (*func)(HTMLDocument*,DWORD,VARIANT*,VARIANT*);
} exec_table[OLECMDID_GETPRINTTEMPLATE+1] = {
    {0},
    { OLECMDF_SUPPORTED,                  exec_open                 }, /* OLECMDID_OPEN */
    { OLECMDF_SUPPORTED,                  exec_new                  }, /* OLECMDID_NEW */
    { OLECMDF_SUPPORTED,                  exec_save                 }, /* OLECMDID_SAVE */
    { OLECMDF_SUPPORTED|OLECMDF_ENABLED,  exec_save_as              }, /* OLECMDID_SAVEAS */
    { OLECMDF_SUPPORTED,                  exec_save_copy_as         }, /* OLECMDID_SAVECOPYAS */
    { OLECMDF_SUPPORTED|OLECMDF_ENABLED,  exec_print                }, /* OLECMDID_PRINT */
    { OLECMDF_SUPPORTED|OLECMDF_ENABLED,  exec_print_preview        }, /* OLECMDID_PRINTPREVIEW */
    { OLECMDF_SUPPORTED|OLECMDF_ENABLED,  exec_page_setup           }, /* OLECMDID_PAGESETUP */
    { OLECMDF_SUPPORTED,                  exec_spell                }, /* OLECMDID_SPELL */
    { OLECMDF_SUPPORTED|OLECMDF_ENABLED,  exec_properties           }, /* OLECMDID_PROPERTIES */
    { OLECMDF_SUPPORTED,                  exec_cut                  }, /* OLECMDID_CUT */
    { OLECMDF_SUPPORTED,                  exec_copy                 }, /* OLECMDID_COPY */
    { OLECMDF_SUPPORTED,                  exec_paste                }, /* OLECMDID_PASTE */
    { OLECMDF_SUPPORTED,                  exec_paste_special        }, /* OLECMDID_PASTESPECIAL */
    { OLECMDF_SUPPORTED,                  exec_undo                 }, /* OLECMDID_UNDO */
    { OLECMDF_SUPPORTED,                  exec_rendo                }, /* OLECMDID_REDO */
    { OLECMDF_SUPPORTED|OLECMDF_ENABLED,  exec_select_all           }, /* OLECMDID_SELECTALL */
    { OLECMDF_SUPPORTED,                  exec_clear_selection      }, /* OLECMDID_CLEARSELECTION */
    { OLECMDF_SUPPORTED,                  exec_zoom                 }, /* OLECMDID_ZOOM */
    { OLECMDF_SUPPORTED,                  exec_get_zoom_range       }, /* OLECMDID_GETZOOMRANGE */
    {0},
    { OLECMDF_SUPPORTED|OLECMDF_ENABLED,  exec_refresh              }, /* OLECMDID_REFRESH */
    { OLECMDF_SUPPORTED|OLECMDF_ENABLED,  exec_stop                 }, /* OLECMDID_STOP */
    {0},{0},{0},{0},{0},{0},
    { OLECMDF_SUPPORTED,                  exec_stop_download        }, /* OLECMDID_STOPDOWNLOAD */
    {0},{0},
    { OLECMDF_SUPPORTED,                  exec_delete               }, /* OLECMDID_DELETE */
    {0},{0},
    { OLECMDF_SUPPORTED,                  exec_enable_interaction   }, /* OLECMDID_ENABLE_INTERACTION */
    { OLECMDF_SUPPORTED,                  exec_on_unload            }, /* OLECMDID_ONUNLOAD */
    {0},{0},{0},{0},{0},
    { OLECMDF_SUPPORTED,                  exec_show_page_setup      }, /* OLECMDID_SHOWPAGESETUP */
    { OLECMDF_SUPPORTED,                  exec_show_print           }, /* OLECMDID_SHOWPRINT */
    {0},{0},
    { OLECMDF_SUPPORTED,                  exec_close                }, /* OLECMDID_CLOSE */
    {0},{0},{0},
    { OLECMDF_SUPPORTED,                  exec_set_print_template   }, /* OLECMDID_SETPRINTTEMPLATE */
    { OLECMDF_SUPPORTED,                  exec_get_print_template   }  /* OLECMDID_GETPRINTTEMPLATE */
};

static HRESULT WINAPI OleCommandTarget_QueryInterface(IOleCommandTarget *iface, REFIID riid, void **ppv)
{
    HTMLDocument *This = CMDTARGET_THIS(iface);
    return IHTMLDocument2_QueryInterface(HTMLDOC(This), riid, ppv);
}

static ULONG WINAPI OleCommandTarget_AddRef(IOleCommandTarget *iface)
{
    HTMLDocument *This = CMDTARGET_THIS(iface);
    return IHTMLDocument2_AddRef(HTMLDOC(This));
}

static ULONG WINAPI OleCommandTarget_Release(IOleCommandTarget *iface)
{
    HTMLDocument *This = CMDTARGET_THIS(iface);
    return IHTMLDocument_Release(HTMLDOC(This));
}

static HRESULT WINAPI OleCommandTarget_QueryStatus(IOleCommandTarget *iface, const GUID *pguidCmdGroup,
        ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    HTMLDocument *This = CMDTARGET_THIS(iface);
    HRESULT hres = S_OK, hr;

    TRACE("(%p)->(%s %ld %p %p)\n", This, debugstr_guid(pguidCmdGroup), cCmds, prgCmds, pCmdText);

    if(!pguidCmdGroup) {
        ULONG i;

        for(i=0; i<cCmds; i++) {
            if(prgCmds[i].cmdID<OLECMDID_OPEN || prgCmds[i].cmdID>OLECMDID_GETPRINTTEMPLATE) {
                WARN("Unsupported cmdID = %ld\n", prgCmds[i].cmdID);
                prgCmds[i].cmdf = 0;
                hres = OLECMDERR_E_NOTSUPPORTED;
            }else {
                if(prgCmds[i].cmdID == OLECMDID_OPEN || prgCmds[i].cmdID == OLECMDID_NEW) {
                    IOleCommandTarget *cmdtrg = NULL;
                    OLECMD olecmd;

                    prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    if(This->client) {
                        hr = IOleClientSite_QueryInterface(This->client, &IID_IOleCommandTarget,
                                (void**)&cmdtrg);
                        if(SUCCEEDED(hr)) {
                            olecmd.cmdID = prgCmds[i].cmdID;
                            olecmd.cmdf = 0;

                            hr = IOleCommandTarget_QueryStatus(cmdtrg, NULL, 1, &olecmd, NULL);
                            if(SUCCEEDED(hr) && olecmd.cmdf)
                                prgCmds[i].cmdf = olecmd.cmdf;
                        }
                    }else {
                        ERR("This->client == NULL, native would crash\n");
                    }
                }else {
                    prgCmds[i].cmdf = exec_table[prgCmds[i].cmdID].cmdf;
                    TRACE("cmdID = %ld  returning %lx\n", prgCmds[i].cmdID, prgCmds[i].cmdf);
                }
                hres = S_OK;
            }
        }

        if(pCmdText)
            FIXME("Set pCmdText\n");
    }else {
        FIXME("Unsupported pguidCmdGroup %s\n", debugstr_guid(pguidCmdGroup));
        hres = OLECMDERR_E_UNKNOWNGROUP;
    }

    return hres;
}

static HRESULT WINAPI OleCommandTarget_Exec(IOleCommandTarget *iface, const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    HTMLDocument *This = CMDTARGET_THIS(iface);

    if(!pguidCmdGroup) {
        if(nCmdID<OLECMDID_OPEN || nCmdID>OLECMDID_GETPRINTTEMPLATE || !exec_table[nCmdID].func) {
            WARN("Unsupported cmdID = %ld\n", nCmdID);
            return OLECMDERR_E_NOTSUPPORTED;
        }

        return exec_table[nCmdID].func(This, nCmdexecopt, pvaIn, pvaOut);
    }else if(IsEqualGUID(&CGID_Explorer, pguidCmdGroup)) {
        FIXME("unsupported nCmdID %ld of CGID_Explorer group\n", nCmdID);
        TRACE("%p %p\n", pvaIn, pvaOut);
        return OLECMDERR_E_NOTSUPPORTED;
    }else if(IsEqualGUID(&CGID_ShellDocView, pguidCmdGroup)) {
        FIXME("unsupported nCmdID %ld of CGID_ShellDocView group\n", nCmdID);
        return OLECMDERR_E_NOTSUPPORTED;
    }

    FIXME("Unsupported pguidCmdGroup %s\n", debugstr_guid(pguidCmdGroup));
    return OLECMDERR_E_UNKNOWNGROUP;
}

#undef CMDTARGET_THIS

static const IOleCommandTargetVtbl OleCommandTargetVtbl = {
    OleCommandTarget_QueryInterface,
    OleCommandTarget_AddRef,
    OleCommandTarget_Release,
    OleCommandTarget_QueryStatus,
    OleCommandTarget_Exec
};

/**********************************************************
 * IOleControl implementation
 */

#define CONTROL_THIS(iface) DEFINE_THIS(HTMLDocument, OleControl, iface)

static HRESULT WINAPI OleControl_QueryInterface(IOleControl *iface, REFIID riid, void **ppv)
{
    HTMLDocument *This = CONTROL_THIS(iface);
    return IHTMLDocument2_QueryInterface(HTMLDOC(This), riid, ppv);
}

static ULONG WINAPI OleControl_AddRef(IOleControl *iface)
{
    HTMLDocument *This = CONTROL_THIS(iface);
    return IHTMLDocument2_AddRef(HTMLDOC(This));
}

static ULONG WINAPI OleControl_Release(IOleControl *iface)
{
    HTMLDocument *This = CONTROL_THIS(iface);
    return IHTMLDocument_Release(HTMLDOC(This));
}

static HRESULT WINAPI OleControl_GetControlInfo(IOleControl *iface, CONTROLINFO *pCI)
{
    HTMLDocument *This = CONTROL_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pCI);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleControl_OnMnemonic(IOleControl *iface, MSG *pMsg)
{
    HTMLDocument *This = CONTROL_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pMsg);
    return E_NOTIMPL;
}

static HRESULT get_property(IOleClientSite *client, DISPID dispid, VARIANT *res)
{
    IDispatch *disp = NULL;
    DISPPARAMS dispparams = {NULL, 0};
    UINT err;
    HRESULT hres;

    hres = IOleClientSite_QueryInterface(client, &IID_IDispatch, (void**)&disp);
    if(FAILED(hres)) {
        TRACE("Could not get IDispatch\n");
        return hres;
    }

    VariantInit(res);

    hres = IDispatch_Invoke(disp, dispid, &IID_NULL, LOCALE_SYSTEM_DEFAULT,
            DISPATCH_PROPERTYGET, &dispparams, res, NULL, &err);

    IDispatch_Release(disp);

    return hres;
}

static HRESULT on_change_dlcontrol(HTMLDocument *This)
{
    VARIANT res;
    HRESULT hres;
    
    hres = get_property(This->client, DISPID_AMBIENT_DLCONTROL, &res);
    if(SUCCEEDED(hres))
        FIXME("unsupported dlcontrol %08lx\n", V_I4(&res));

    return S_OK;
}

static HRESULT WINAPI OleControl_OnAmbientPropertyChange(IOleControl *iface, DISPID dispID)
{
    HTMLDocument *This = CONTROL_THIS(iface);
    VARIANT res;
    HRESULT hres;

    if(!This->client) {
        TRACE("This->client = NULL\n");
        return S_OK;
    }

    switch(dispID) {
    case DISPID_AMBIENT_USERMODE:
        TRACE("(%p)->(DISPID_AMBIENT_USERMODE)\n", This);
        hres = get_property(This->client, DISPID_AMBIENT_USERMODE, &res);
        if(FAILED(hres))
            return S_OK;

        if(V_VT(&res) == VT_BOOL) {
            if(!V_BOOL(&res)) {
                FIXME("edit mode is not supported\n");
                hres = E_FAIL;
            }
        }else {
            FIXME("V_VT(res)=%d\n", V_VT(&res));
        }
        return S_OK;
    case DISPID_AMBIENT_DLCONTROL:
        TRACE("(%p)->(DISPID_AMBIENT_DLCONTROL)\n", This);
        return on_change_dlcontrol(This);
    case DISPID_AMBIENT_OFFLINEIFNOTCONNECTED:
        TRACE("(%p)->(DISPID_AMBIENT_OFFLINEIFNOTCONNECTED)\n", This);
        on_change_dlcontrol(This);
        hres = get_property(This->client, DISPID_AMBIENT_OFFLINEIFNOTCONNECTED, &res);
        if(FAILED(hres))
            return S_OK;

        if(V_VT(&res) == VT_BOOL) {
            if(V_BOOL(&res)) {
                FIXME("offline connection is not supported\n");
                hres = E_FAIL;
            }
        }else {
            FIXME("V_VT(res)=%d\n", V_VT(&res));
        }
        return S_OK;
    case DISPID_AMBIENT_SILENT:
        TRACE("(%p)->(DISPID_AMBIENT_SILENT)\n", This);
        on_change_dlcontrol(This);
        hres = get_property(This->client, DISPID_AMBIENT_SILENT, &res);
        if(FAILED(hres))
            return S_OK;

        if(V_VT(&res) == VT_BOOL) {
            if(V_BOOL(&res)) {
                FIXME("silent mode is not supported\n");
                hres = E_FAIL;
            }
        }else {
            FIXME("V_VT(res)=%d\n", V_VT(&res));
        }
        return S_OK;
    case DISPID_AMBIENT_USERAGENT:
        TRACE("(%p)->(DISPID_AMBIENT_USERAGENT)\n", This);
        hres = get_property(This->client, DISPID_AMBIENT_USERAGENT, &res);
        if(FAILED(hres))
            return S_OK;

        FIXME("not supported AMBIENT_USERAGENT\n");
        hres = E_FAIL;
        return S_OK;
    case DISPID_AMBIENT_PALETTE:
        TRACE("(%p)->(DISPID_AMBIENT_PALETTE)\n", This);
        hres = get_property(This->client, DISPID_AMBIENT_PALETTE, &res);
        if(FAILED(hres))
            return S_OK;

        FIXME("not supported AMBIENT_PALETTE\n");
        hres = E_FAIL;
        return S_OK;
    }

    FIXME("(%p) unsupported dispID=%ld\n", This, dispID);
    return E_FAIL;
}

static HRESULT WINAPI OleControl_FreezeEvents(IOleControl *iface, BOOL bFreeze)
{
    HTMLDocument *This = CONTROL_THIS(iface);
    FIXME("(%p)->(%x)\n", This, bFreeze);
    return E_NOTIMPL;
}

#undef CONTROL_THIS

static const IOleControlVtbl OleControlVtbl = {
    OleControl_QueryInterface,
    OleControl_AddRef,
    OleControl_Release,
    OleControl_GetControlInfo,
    OleControl_OnMnemonic,
    OleControl_OnAmbientPropertyChange,
    OleControl_FreezeEvents
};

void HTMLDocument_LockContainer(HTMLDocument *This, BOOL fLock)
{
    IOleContainer *container;
    HRESULT hres;

    if(!This->client || This->container_locked == fLock)
        return;

    hres = IOleClientSite_GetContainer(This->client, &container);
    if(SUCCEEDED(hres)) {
        IOleContainer_LockContainer(container, fLock);
        This->container_locked = fLock;
        IOleContainer_Release(container);
    }
}

void HTMLDocument_OleObj_Init(HTMLDocument *This)
{
    This->lpOleObjectVtbl = &OleObjectVtbl;
    This->lpOleDocumentVtbl = &OleDocumentVtbl;
    This->lpOleCommandTargetVtbl = &OleCommandTargetVtbl;
    This->lpOleControlVtbl = &OleControlVtbl;

    This->client = NULL;
    This->hostui = NULL;

    This->has_key_path = FALSE;
    This->container_locked = FALSE;
}
