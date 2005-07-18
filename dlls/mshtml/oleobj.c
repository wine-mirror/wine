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

#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "docobj.h"

#include "mshtml.h"
#include "mshtmhst.h"

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
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, pClientSite);

    if(This->client)
        IOleClientSite_Release(This->client);

    if(This->hostui)
        IDocHostUIHandler_Release(This->hostui);

    if(!pClientSite) {
        This->client = NULL;
        return S_OK;
    }

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

    IOleClientSite_AddRef(pClientSite);
    This->client = pClientSite;
    This->hostui = pDocHostUIHandler;

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
    HRESULT hres;

    FIXME("(%p)->(%08lx)\n", This, dwSaveOption);

    if(This->client) {
        IOleContainer *container;
        hres = IOleClientSite_GetContainer(This->client, &container);
        if(SUCCEEDED(hres)) {
            IOleContainer_LockContainer(container, FALSE);
            IOleContainer_Release(container);
        }
    }
    
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

    if(iVerb != OLEIVERB_SHOW && iVerb != OLEIVERB_UIACTIVATE) {
        FIXME("iVerb = %ld not supported\n", iVerb);
        return E_NOTIMPL;
    }

    if(!pActiveSite)
        pActiveSite = This->client;

    hres = IOleClientSite_QueryInterface(pActiveSite, &IID_IOleDocumentSite, (void**)&pDocSite);
    if(SUCCEEDED(hres)) {
        IOleContainer *pContainer;
        hres = IOleClientSite_GetContainer(pActiveSite, &pContainer);
        if(SUCCEEDED(hres)) {
            IOleContainer_LockContainer(pContainer, TRUE);
            IOleContainer_Release(pContainer);
        }
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
    FIXME("(%p)->(%s %ld %p %p)\n", This, debugstr_guid(pguidCmdGroup), cCmds, prgCmds, pCmdText);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleCommandTarget_Exec(IOleCommandTarget *iface, const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    HTMLDocument *This = CMDTARGET_THIS(iface);
    FIXME("(%p)->(%s %ld %ld %p %p)\n", This, debugstr_guid(pguidCmdGroup), nCmdID, nCmdexecopt,
            pvaIn, pvaOut);
    return E_NOTIMPL;
}

static const IOleCommandTargetVtbl OleCommandTargetVtbl = {
    OleCommandTarget_QueryInterface,
    OleCommandTarget_AddRef,
    OleCommandTarget_Release,
    OleCommandTarget_QueryStatus,
    OleCommandTarget_Exec
};

void HTMLDocument_OleObj_Init(HTMLDocument *This)
{
    This->lpOleObjectVtbl = &OleObjectVtbl;
    This->lpOleDocumentVtbl = &OleDocumentVtbl;
    This->lpOleCommandTargetVtbl = &OleCommandTargetVtbl;

    This->client = NULL;
    This->hostui = NULL;

    This->has_key_path = FALSE;
}
