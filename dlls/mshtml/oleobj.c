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

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "shlguid.h"
#include "shdeprecated.h"
#include "mshtmdid.h"
#include "idispids.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define DOCHOST_DOCCANNAVIGATE  0

typedef struct {
    IEnumUnknown IEnumUnknown_iface;
    LONG ref;
} EnumUnknown;

static inline EnumUnknown *impl_from_IEnumUnknown(IEnumUnknown *iface)
{
    return CONTAINING_RECORD(iface, EnumUnknown, IEnumUnknown_iface);
}

static HRESULT WINAPI EnumUnknown_QueryInterface(IEnumUnknown *iface, REFIID riid, void **ppv)
{
    EnumUnknown *This = impl_from_IEnumUnknown(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IEnumUnknown_iface;
    }else if(IsEqualGUID(&IID_IEnumUnknown, riid)) {
        TRACE("(%p)->(IID_IEnumUnknown %p)\n", This, ppv);
        *ppv = &This->IEnumUnknown_iface;
    }else {
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI EnumUnknown_AddRef(IEnumUnknown *iface)
{
    EnumUnknown *This = impl_from_IEnumUnknown(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI EnumUnknown_Release(IEnumUnknown *iface)
{
    EnumUnknown *This = impl_from_IEnumUnknown(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI EnumUnknown_Next(IEnumUnknown *iface, ULONG celt, IUnknown **rgelt, ULONG *pceltFetched)
{
    EnumUnknown *This = impl_from_IEnumUnknown(iface);

    TRACE("(%p)->(%lu %p %p)\n", This, celt, rgelt, pceltFetched);

    /* FIXME: It's not clear if we should ever return something here */
    if(pceltFetched)
        *pceltFetched = 0;
    return S_FALSE;
}

static HRESULT WINAPI EnumUnknown_Skip(IEnumUnknown *iface, ULONG celt)
{
    EnumUnknown *This = impl_from_IEnumUnknown(iface);
    FIXME("(%p)->(%lu)\n", This, celt);
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumUnknown_Reset(IEnumUnknown *iface)
{
    EnumUnknown *This = impl_from_IEnumUnknown(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumUnknown_Clone(IEnumUnknown *iface, IEnumUnknown **ppenum)
{
    EnumUnknown *This = impl_from_IEnumUnknown(iface);
    FIXME("(%p)->(%p)\n", This, ppenum);
    return E_NOTIMPL;
}

static const IEnumUnknownVtbl EnumUnknownVtbl = {
    EnumUnknown_QueryInterface,
    EnumUnknown_AddRef,
    EnumUnknown_Release,
    EnumUnknown_Next,
    EnumUnknown_Skip,
    EnumUnknown_Reset,
    EnumUnknown_Clone
};

/**********************************************************
 * IOleObject implementation
 */

static inline HTMLDocumentNode *HTMLDocumentNode_from_IOleObject(IOleObject *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IOleObject_iface);
}

static HRESULT WINAPI DocNodeOleObject_QueryInterface(IOleObject *iface, REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    return htmldoc_query_interface(&This->basedoc, riid, ppv);
}

static ULONG WINAPI DocNodeOleObject_AddRef(IOleObject *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    return htmldoc_addref(&This->basedoc);
}

static ULONG WINAPI DocNodeOleObject_Release(IOleObject *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    return htmldoc_release(&This->basedoc);
}

static HRESULT WINAPI DocNodeOleObject_SetClientSite(IOleObject *iface, IOleClientSite *pClientSite)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    return IOleObject_SetClientSite(&This->basedoc.doc_obj->IOleObject_iface, pClientSite);
}

static HRESULT WINAPI DocNodeOleObject_GetClientSite(IOleObject *iface, IOleClientSite **ppClientSite)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    return IOleObject_GetClientSite(&This->basedoc.doc_obj->IOleObject_iface, ppClientSite);
}

static HRESULT WINAPI DocNodeOleObject_SetHostNames(IOleObject *iface, LPCOLESTR szContainerApp, LPCOLESTR szContainerObj)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_w(szContainerApp), debugstr_w(szContainerObj));
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleObject_Close(IOleObject *iface, DWORD dwSaveOption)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    return IOleObject_Close(&This->basedoc.doc_obj->IOleObject_iface, dwSaveOption);
}

static HRESULT WINAPI DocNodeOleObject_SetMoniker(IOleObject *iface, DWORD dwWhichMoniker, IMoniker *pmk)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    FIXME("(%p %ld %p)->()\n", This, dwWhichMoniker, pmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleObject_GetMoniker(IOleObject *iface, DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    FIXME("(%p)->(%ld %ld %p)\n", This, dwAssign, dwWhichMoniker, ppmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleObject_InitFromData(IOleObject *iface, IDataObject *pDataObject, BOOL fCreation,
                                                    DWORD dwReserved)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    FIXME("(%p)->(%p %x %ld)\n", This, pDataObject, fCreation, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleObject_GetClipboardData(IOleObject *iface, DWORD dwReserved, IDataObject **ppDataObject)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    FIXME("(%p)->(%ld %p)\n", This, dwReserved, ppDataObject);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleObject_DoVerb(IOleObject *iface, LONG iVerb, LPMSG lpmsg, IOleClientSite *pActiveSite,
                                              LONG lindex, HWND hwndParent, LPCRECT lprcPosRect)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    return IOleObject_DoVerb(&This->basedoc.doc_obj->IOleObject_iface, iVerb, lpmsg, pActiveSite, lindex, hwndParent, lprcPosRect);
}

static HRESULT WINAPI DocNodeOleObject_EnumVerbs(IOleObject *iface, IEnumOLEVERB **ppEnumOleVerb)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    FIXME("(%p)->(%p)\n", This, ppEnumOleVerb);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleObject_Update(IOleObject *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleObject_IsUpToDate(IOleObject *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleObject_GetUserClassID(IOleObject *iface, CLSID *pClsid)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    return IOleObject_GetUserClassID(&This->basedoc.doc_obj->IOleObject_iface, pClsid);
}

static HRESULT WINAPI DocNodeOleObject_GetUserType(IOleObject *iface, DWORD dwFormOfType, LPOLESTR *pszUserType)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    FIXME("(%p)->(%ld %p)\n", This, dwFormOfType, pszUserType);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleObject_SetExtent(IOleObject *iface, DWORD dwDrawAspect, SIZEL *psizel)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    return IOleObject_SetExtent(&This->basedoc.doc_obj->IOleObject_iface, dwDrawAspect, psizel);
}

static HRESULT WINAPI DocNodeOleObject_GetExtent(IOleObject *iface, DWORD dwDrawAspect, SIZEL *psizel)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    return IOleObject_GetExtent(&This->basedoc.doc_obj->IOleObject_iface, dwDrawAspect, psizel);
}

static HRESULT WINAPI DocNodeOleObject_Advise(IOleObject *iface, IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    return IOleObject_Advise(&This->basedoc.doc_obj->IOleObject_iface, pAdvSink, pdwConnection);
}

static HRESULT WINAPI DocNodeOleObject_Unadvise(IOleObject *iface, DWORD dwConnection)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    return IOleObject_Unadvise(&This->basedoc.doc_obj->IOleObject_iface, dwConnection);
}

static HRESULT WINAPI DocNodeOleObject_EnumAdvise(IOleObject *iface, IEnumSTATDATA **ppenumAdvise)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    return IOleObject_EnumAdvise(&This->basedoc.doc_obj->IOleObject_iface, ppenumAdvise);
}

static HRESULT WINAPI DocNodeOleObject_GetMiscStatus(IOleObject *iface, DWORD dwAspect, DWORD *pdwStatus)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    FIXME("(%p)->(%ld %p)\n", This, dwAspect, pdwStatus);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleObject_SetColorScheme(IOleObject *iface, LOGPALETTE *pLogpal)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    FIXME("(%p)->(%p)\n", This, pLogpal);
    return E_NOTIMPL;
}

static const IOleObjectVtbl DocNodeOleObjectVtbl = {
    DocNodeOleObject_QueryInterface,
    DocNodeOleObject_AddRef,
    DocNodeOleObject_Release,
    DocNodeOleObject_SetClientSite,
    DocNodeOleObject_GetClientSite,
    DocNodeOleObject_SetHostNames,
    DocNodeOleObject_Close,
    DocNodeOleObject_SetMoniker,
    DocNodeOleObject_GetMoniker,
    DocNodeOleObject_InitFromData,
    DocNodeOleObject_GetClipboardData,
    DocNodeOleObject_DoVerb,
    DocNodeOleObject_EnumVerbs,
    DocNodeOleObject_Update,
    DocNodeOleObject_IsUpToDate,
    DocNodeOleObject_GetUserClassID,
    DocNodeOleObject_GetUserType,
    DocNodeOleObject_SetExtent,
    DocNodeOleObject_GetExtent,
    DocNodeOleObject_Advise,
    DocNodeOleObject_Unadvise,
    DocNodeOleObject_EnumAdvise,
    DocNodeOleObject_GetMiscStatus,
    DocNodeOleObject_SetColorScheme
};

static inline HTMLDocumentObj *HTMLDocumentObj_from_IOleObject(IOleObject *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IOleObject_iface);
}

static HRESULT WINAPI DocObjOleObject_QueryInterface(IOleObject *iface, REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    return htmldoc_query_interface(&This->basedoc, riid, ppv);
}

static ULONG WINAPI DocObjOleObject_AddRef(IOleObject *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    return htmldoc_addref(&This->basedoc);
}

static ULONG WINAPI DocObjOleObject_Release(IOleObject *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    return htmldoc_release(&This->basedoc);
}

static void update_hostinfo(HTMLDocumentObj *This, DOCHOSTUIINFO *hostinfo)
{
    nsIScrollable *scrollable;
    nsresult nsres;

    if(!This->nscontainer)
        return;

    nsres = nsIWebBrowser_QueryInterface(This->nscontainer->webbrowser, &IID_nsIScrollable, (void**)&scrollable);
    if(NS_SUCCEEDED(nsres)) {
        nsres = nsIScrollable_SetDefaultScrollbarPreferences(scrollable, ScrollOrientation_Y,
                (hostinfo->dwFlags & DOCHOSTUIFLAG_SCROLL_NO) ? Scrollbar_Never : Scrollbar_Always);
        if(NS_FAILED(nsres))
            ERR("Could not set default Y scrollbar prefs: %08lx\n", nsres);

        nsres = nsIScrollable_SetDefaultScrollbarPreferences(scrollable, ScrollOrientation_X,
                hostinfo->dwFlags & DOCHOSTUIFLAG_SCROLL_NO ? Scrollbar_Never : Scrollbar_Auto);
        if(NS_FAILED(nsres))
            ERR("Could not set default X scrollbar prefs: %08lx\n", nsres);

        nsIScrollable_Release(scrollable);
    }else {
        ERR("Could not get nsIScrollable: %08lx\n", nsres);
    }
}

/* Calls undocumented 84 cmd of CGID_ShellDocView */
void call_docview_84(HTMLDocumentObj *doc)
{
    IOleCommandTarget *olecmd;
    VARIANT var;
    HRESULT hres;

    if(!doc->client)
        return;

    hres = IOleClientSite_QueryInterface(doc->client, &IID_IOleCommandTarget, (void**)&olecmd);
    if(FAILED(hres))
        return;

    VariantInit(&var);
    hres = IOleCommandTarget_Exec(olecmd, &CGID_ShellDocView, 84, 0, NULL, &var);
    IOleCommandTarget_Release(olecmd);
    if(SUCCEEDED(hres) && V_VT(&var) != VT_NULL)
        FIXME("handle result\n");
}

void set_document_navigation(HTMLDocumentObj *doc, BOOL doc_can_navigate)
{
    VARIANT var;

    if(!doc->client_cmdtrg)
        return;

    if(doc_can_navigate) {
        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = (IUnknown*)&doc->basedoc.window->base.IHTMLWindow2_iface;
    }

    IOleCommandTarget_Exec(doc->client_cmdtrg, &CGID_DocHostCmdPriv, DOCHOST_DOCCANNAVIGATE, 0,
            doc_can_navigate ? &var : NULL, NULL);
}

static void load_settings(HTMLDocumentObj *doc)
{
    HKEY settings_key;
    DWORD val, size;
    LONG res;

    res = RegOpenKeyW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Internet Explorer", &settings_key);
    if(res != ERROR_SUCCESS)
        return;

    size = sizeof(val);
    res = RegGetValueW(settings_key, L"Zoom", L"ZoomFactor", RRF_RT_REG_DWORD, NULL, &val, &size);
    RegCloseKey(settings_key);
    if(res == ERROR_SUCCESS)
        set_viewer_zoom(doc->nscontainer, (float)val/100000);
}

static HRESULT WINAPI DocObjOleObject_SetClientSite(IOleObject *iface, IOleClientSite *pClientSite)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    IBrowserService *browser_service;
    IOleCommandTarget *cmdtrg = NULL;
    IOleWindow *ole_window;
    BOOL hostui_setup;
    VARIANT silent;
    HWND hwnd;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, pClientSite);

    if(pClientSite == This->client)
        return S_OK;

    if(This->client) {
        IOleClientSite_Release(This->client);
        This->client = NULL;
        This->nscontainer->usermode = UNKNOWN_USERMODE;
    }

    if(This->client_cmdtrg) {
        IOleCommandTarget_Release(This->client_cmdtrg);
        This->client_cmdtrg = NULL;
    }

    if(This->hostui && !This->custom_hostui) {
        IDocHostUIHandler_Release(This->hostui);
        This->hostui = NULL;
    }

    if(This->doc_object_service) {
        IDocObjectService_Release(This->doc_object_service);
        This->doc_object_service = NULL;
    }

    if(This->webbrowser) {
        IUnknown_Release(This->webbrowser);
        This->webbrowser = NULL;
    }

    if(This->browser_service) {
        IUnknown_Release(This->browser_service);
        This->browser_service = NULL;
    }

    if(This->travel_log) {
        ITravelLog_Release(This->travel_log);
        This->travel_log = NULL;
    }

    memset(&This->hostinfo, 0, sizeof(DOCHOSTUIINFO));

    if(!pClientSite)
        return S_OK;

    IOleClientSite_AddRef(pClientSite);
    This->client = pClientSite;

    hostui_setup = This->hostui_setup;

    if(!This->hostui) {
        IDocHostUIHandler *uihandler;

        This->custom_hostui = FALSE;

        hres = IOleClientSite_QueryInterface(pClientSite, &IID_IDocHostUIHandler, (void**)&uihandler);
        if(SUCCEEDED(hres))
            This->hostui = uihandler;
    }

    if(This->hostui) {
        DOCHOSTUIINFO hostinfo;
        LPOLESTR key_path = NULL, override_key_path = NULL;
        IDocHostUIHandler2 *uihandler2;

        memset(&hostinfo, 0, sizeof(DOCHOSTUIINFO));
        hostinfo.cbSize = sizeof(DOCHOSTUIINFO);
        hres = IDocHostUIHandler_GetHostInfo(This->hostui, &hostinfo);
        if(SUCCEEDED(hres)) {
            TRACE("hostinfo = {%lu %08lx %08lx %s %s}\n",
                    hostinfo.cbSize, hostinfo.dwFlags, hostinfo.dwDoubleClick,
                    debugstr_w(hostinfo.pchHostCss), debugstr_w(hostinfo.pchHostNS));
            update_hostinfo(This, &hostinfo);
            This->hostinfo = hostinfo;
        }

        if(!hostui_setup) {
            hres = IDocHostUIHandler_GetOptionKeyPath(This->hostui, &key_path, 0);
            if(hres == S_OK && key_path) {
                if(key_path[0]) {
                    /* FIXME: use key_path */
                    FIXME("key_path = %s\n", debugstr_w(key_path));
                }
                CoTaskMemFree(key_path);
            }

            hres = IDocHostUIHandler_QueryInterface(This->hostui, &IID_IDocHostUIHandler2,
                    (void**)&uihandler2);
            if(SUCCEEDED(hres)) {
                hres = IDocHostUIHandler2_GetOverrideKeyPath(uihandler2, &override_key_path, 0);
                if(hres == S_OK && override_key_path) {
                    if(override_key_path[0]) {
                        /*FIXME: use override_key_path */
                        FIXME("override_key_path = %s\n", debugstr_w(override_key_path));
                    }
                    CoTaskMemFree(override_key_path);
                }
                IDocHostUIHandler2_Release(uihandler2);
            }

            This->hostui_setup = TRUE;
        }
    }

    load_settings(This);

    /* Native calls here GetWindow. What is it for?
     * We don't have anything to do with it here (yet). */
    hres = IOleClientSite_QueryInterface(pClientSite, &IID_IOleWindow, (void**)&ole_window);
    if(SUCCEEDED(hres)) {
        IOleWindow_GetWindow(ole_window, &hwnd);
        IOleWindow_Release(ole_window);
    }

    hres = do_query_service((IUnknown*)pClientSite, &IID_IShellBrowser,
            &IID_IBrowserService, (void**)&browser_service);
    if(SUCCEEDED(hres)) {
        ITravelLog *travel_log;

        This->browser_service = (IUnknown*)browser_service;

        hres = IBrowserService_GetTravelLog(browser_service, &travel_log);
        if(SUCCEEDED(hres))
            This->travel_log = travel_log;
    }else {
        browser_service = NULL;
    }

    hres = IOleClientSite_QueryInterface(pClientSite, &IID_IOleCommandTarget, (void**)&cmdtrg);
    if(SUCCEEDED(hres)) {
        VARIANT var;
        OLECMD cmd = {OLECMDID_SETPROGRESSTEXT, 0};

        This->client_cmdtrg = cmdtrg;

        if(!hostui_setup) {
            IDocObjectService *doc_object_service;
            IWebBrowser2 *wb;

            set_document_navigation(This, TRUE);

            if(browser_service) {
                hres = IBrowserService_QueryInterface(browser_service,
                        &IID_IDocObjectService, (void**)&doc_object_service);
                if(SUCCEEDED(hres)) {
                    This->doc_object_service = doc_object_service;

                    /*
                     * Some embedding routines, esp. in regards to use of IDocObjectService, differ if
                     * embedder supports IWebBrowserApp.
                     */
                    hres = do_query_service((IUnknown*)pClientSite, &IID_IWebBrowserApp, &IID_IWebBrowser2, (void**)&wb);
                    if(SUCCEEDED(hres))
                        This->webbrowser = (IUnknown*)wb;
                }
            }
        }

        call_docview_84(This);

        IOleCommandTarget_QueryStatus(cmdtrg, NULL, 1, &cmd, NULL);

        V_VT(&var) = VT_I4;
        V_I4(&var) = 0;
        IOleCommandTarget_Exec(cmdtrg, NULL, OLECMDID_SETPROGRESSMAX,
                OLECMDEXECOPT_DONTPROMPTUSER, &var, NULL);
        IOleCommandTarget_Exec(cmdtrg, NULL, OLECMDID_SETPROGRESSPOS, 
                OLECMDEXECOPT_DONTPROMPTUSER, &var, NULL);
    }

    if(This->nscontainer->usermode == UNKNOWN_USERMODE)
        IOleControl_OnAmbientPropertyChange(&This->IOleControl_iface, DISPID_AMBIENT_USERMODE);

    IOleControl_OnAmbientPropertyChange(&This->IOleControl_iface,
            DISPID_AMBIENT_OFFLINEIFNOTCONNECTED);

    hres = get_client_disp_property(This->client, DISPID_AMBIENT_SILENT, &silent);
    if(SUCCEEDED(hres)) {
        if(V_VT(&silent) != VT_BOOL)
            WARN("silent = %s\n", debugstr_variant(&silent));
        else if(V_BOOL(&silent))
            FIXME("silent == true\n");
    }

    IOleControl_OnAmbientPropertyChange(&This->IOleControl_iface, DISPID_AMBIENT_USERAGENT);
    IOleControl_OnAmbientPropertyChange(&This->IOleControl_iface, DISPID_AMBIENT_PALETTE);

    return S_OK;
}

static HRESULT WINAPI DocObjOleObject_GetClientSite(IOleObject *iface, IOleClientSite **ppClientSite)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);

    TRACE("(%p)->(%p)\n", This, ppClientSite);

    if(!ppClientSite)
        return E_INVALIDARG;

    if(This->client)
        IOleClientSite_AddRef(This->client);
    *ppClientSite = This->client;

    return S_OK;
}

static HRESULT WINAPI DocObjOleObject_SetHostNames(IOleObject *iface, LPCOLESTR szContainerApp, LPCOLESTR szContainerObj)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_w(szContainerApp), debugstr_w(szContainerObj));
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleObject_Close(IOleObject *iface, DWORD dwSaveOption)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);

    TRACE("(%p)->(%08lx)\n", This, dwSaveOption);

    if(dwSaveOption == OLECLOSE_PROMPTSAVE)
        FIXME("OLECLOSE_PROMPTSAVE not implemented\n");

    if(This->in_place_active)
        IOleInPlaceObjectWindowless_InPlaceDeactivate(&This->IOleInPlaceObjectWindowless_iface);

    HTMLDocument_LockContainer(This, FALSE);

    if(This->advise_holder)
        IOleAdviseHolder_SendOnClose(This->advise_holder);

    return S_OK;
}

static HRESULT WINAPI DocObjOleObject_SetMoniker(IOleObject *iface, DWORD dwWhichMoniker, IMoniker *pmk)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    FIXME("(%p %ld %p)->()\n", This, dwWhichMoniker, pmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleObject_GetMoniker(IOleObject *iface, DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    FIXME("(%p)->(%ld %ld %p)\n", This, dwAssign, dwWhichMoniker, ppmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleObject_InitFromData(IOleObject *iface, IDataObject *pDataObject, BOOL fCreation,
                                                   DWORD dwReserved)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    FIXME("(%p)->(%p %x %ld)\n", This, pDataObject, fCreation, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleObject_GetClipboardData(IOleObject *iface, DWORD dwReserved, IDataObject **ppDataObject)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    FIXME("(%p)->(%ld %p)\n", This, dwReserved, ppDataObject);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleObject_DoVerb(IOleObject *iface, LONG iVerb, LPMSG lpmsg, IOleClientSite *pActiveSite,
                                             LONG lindex, HWND hwndParent, LPCRECT lprcPosRect)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
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
        hres = IOleDocumentSite_ActivateMe(pDocSite, &This->IOleDocumentView_iface);
        IOleDocumentSite_Release(pDocSite);
    }else {
        hres = IOleDocumentView_UIActivate(&This->IOleDocumentView_iface, TRUE);
        if(SUCCEEDED(hres)) {
            if(lprcPosRect) {
                RECT rect; /* We need to pass rect as not const pointer */
                rect = *lprcPosRect;
                IOleDocumentView_SetRect(&This->IOleDocumentView_iface, &rect);
            }
            IOleDocumentView_Show(&This->IOleDocumentView_iface, TRUE);
        }
    }

    return hres;
}

static HRESULT WINAPI DocObjOleObject_EnumVerbs(IOleObject *iface, IEnumOLEVERB **ppEnumOleVerb)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    FIXME("(%p)->(%p)\n", This, ppEnumOleVerb);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleObject_Update(IOleObject *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleObject_IsUpToDate(IOleObject *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleObject_GetUserClassID(IOleObject *iface, CLSID *pClsid)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);

    TRACE("(%p)->(%p)\n", This, pClsid);

    if(!pClsid)
        return E_INVALIDARG;

    *pClsid = CLSID_HTMLDocument;
    return S_OK;
}

static HRESULT WINAPI DocObjOleObject_GetUserType(IOleObject *iface, DWORD dwFormOfType, LPOLESTR *pszUserType)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    FIXME("(%p)->(%ld %p)\n", This, dwFormOfType, pszUserType);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleObject_SetExtent(IOleObject *iface, DWORD dwDrawAspect, SIZEL *psizel)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);

    TRACE("(%p)->(%ld %p)\n", This, dwDrawAspect, psizel);

    if (dwDrawAspect != DVASPECT_CONTENT)
        return E_INVALIDARG;

    This->extent = *psizel;
    return S_OK;
}

static HRESULT WINAPI DocObjOleObject_GetExtent(IOleObject *iface, DWORD dwDrawAspect, SIZEL *psizel)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);

    TRACE("(%p)->(%ld %p)\n", This, dwDrawAspect, psizel);

    if (dwDrawAspect != DVASPECT_CONTENT)
        return E_INVALIDARG;

    *psizel = This->extent;
    return S_OK;
}

static HRESULT WINAPI DocObjOleObject_Advise(IOleObject *iface, IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    TRACE("(%p)->(%p %p)\n", This, pAdvSink, pdwConnection);

    if(!pdwConnection)
        return E_INVALIDARG;

    if(!pAdvSink) {
        *pdwConnection = 0;
        return E_INVALIDARG;
    }

    if(!This->advise_holder) {
        CreateOleAdviseHolder(&This->advise_holder);
        if(!This->advise_holder)
            return E_OUTOFMEMORY;
    }

    return IOleAdviseHolder_Advise(This->advise_holder, pAdvSink, pdwConnection);
}

static HRESULT WINAPI DocObjOleObject_Unadvise(IOleObject *iface, DWORD dwConnection)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    TRACE("(%p)->(%ld)\n", This, dwConnection);

    if(!This->advise_holder)
        return OLE_E_NOCONNECTION;

    return IOleAdviseHolder_Unadvise(This->advise_holder, dwConnection);
}

static HRESULT WINAPI DocObjOleObject_EnumAdvise(IOleObject *iface, IEnumSTATDATA **ppenumAdvise)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);

    if(!This->advise_holder) {
        *ppenumAdvise = NULL;
        return S_OK;
    }

    return IOleAdviseHolder_EnumAdvise(This->advise_holder, ppenumAdvise);
}

static HRESULT WINAPI DocObjOleObject_GetMiscStatus(IOleObject *iface, DWORD dwAspect, DWORD *pdwStatus)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    FIXME("(%p)->(%ld %p)\n", This, dwAspect, pdwStatus);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleObject_SetColorScheme(IOleObject *iface, LOGPALETTE *pLogpal)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    FIXME("(%p)->(%p)\n", This, pLogpal);
    return E_NOTIMPL;
}

static const IOleObjectVtbl DocObjOleObjectVtbl = {
    DocObjOleObject_QueryInterface,
    DocObjOleObject_AddRef,
    DocObjOleObject_Release,
    DocObjOleObject_SetClientSite,
    DocObjOleObject_GetClientSite,
    DocObjOleObject_SetHostNames,
    DocObjOleObject_Close,
    DocObjOleObject_SetMoniker,
    DocObjOleObject_GetMoniker,
    DocObjOleObject_InitFromData,
    DocObjOleObject_GetClipboardData,
    DocObjOleObject_DoVerb,
    DocObjOleObject_EnumVerbs,
    DocObjOleObject_Update,
    DocObjOleObject_IsUpToDate,
    DocObjOleObject_GetUserClassID,
    DocObjOleObject_GetUserType,
    DocObjOleObject_SetExtent,
    DocObjOleObject_GetExtent,
    DocObjOleObject_Advise,
    DocObjOleObject_Unadvise,
    DocObjOleObject_EnumAdvise,
    DocObjOleObject_GetMiscStatus,
    DocObjOleObject_SetColorScheme
};

/**********************************************************
 * IOleDocument implementation
 */

static inline HTMLDocumentNode *HTMLDocumentNode_from_IOleDocument(IOleDocument *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IOleDocument_iface);
}

static HRESULT WINAPI DocNodeOleDocument_QueryInterface(IOleDocument *iface, REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleDocument(iface);
    return htmldoc_query_interface(&This->basedoc, riid, ppv);
}

static ULONG WINAPI DocNodeOleDocument_AddRef(IOleDocument *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleDocument(iface);
    return htmldoc_addref(&This->basedoc);
}

static ULONG WINAPI DocNodeOleDocument_Release(IOleDocument *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleDocument(iface);
    return htmldoc_release(&This->basedoc);
}

static HRESULT WINAPI DocNodeOleDocument_CreateView(IOleDocument *iface, IOleInPlaceSite *pIPSite, IStream *pstm,
                                                    DWORD dwReserved, IOleDocumentView **ppView)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleDocument(iface);
    return IOleDocument_CreateView(&This->basedoc.doc_obj->IOleDocument_iface, pIPSite, pstm, dwReserved, ppView);
}

static HRESULT WINAPI DocNodeOleDocument_GetDocMiscStatus(IOleDocument *iface, DWORD *pdwStatus)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleDocument(iface);
    FIXME("(%p)->(%p)\n", This, pdwStatus);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleDocument_EnumViews(IOleDocument *iface, IEnumOleDocumentViews **ppEnum,
                                                   IOleDocumentView **ppView)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleDocument(iface);
    FIXME("(%p)->(%p %p)\n", This, ppEnum, ppView);
    return E_NOTIMPL;
}

static const IOleDocumentVtbl DocNodeOleDocumentVtbl = {
    DocNodeOleDocument_QueryInterface,
    DocNodeOleDocument_AddRef,
    DocNodeOleDocument_Release,
    DocNodeOleDocument_CreateView,
    DocNodeOleDocument_GetDocMiscStatus,
    DocNodeOleDocument_EnumViews
};

static inline HTMLDocumentObj *HTMLDocumentObj_from_IOleDocument(IOleDocument *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IOleDocument_iface);
}

static HRESULT WINAPI DocObjOleDocument_QueryInterface(IOleDocument *iface, REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleDocument(iface);
    return htmldoc_query_interface(&This->basedoc, riid, ppv);
}

static ULONG WINAPI DocObjOleDocument_AddRef(IOleDocument *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleDocument(iface);
    return htmldoc_addref(&This->basedoc);
}

static ULONG WINAPI DocObjOleDocument_Release(IOleDocument *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleDocument(iface);
    return htmldoc_release(&This->basedoc);
}

static HRESULT WINAPI DocObjOleDocument_CreateView(IOleDocument *iface, IOleInPlaceSite *pIPSite, IStream *pstm,
                                                   DWORD dwReserved, IOleDocumentView **ppView)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleDocument(iface);
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
        hres = IOleDocumentView_SetInPlaceSite(&This->IOleDocumentView_iface, pIPSite);
        if(FAILED(hres))
            return hres;
    }

    if(pstm)
        FIXME("pstm is not supported\n");

    IOleDocumentView_AddRef(&This->IOleDocumentView_iface);
    *ppView = &This->IOleDocumentView_iface;
    return S_OK;
}

static HRESULT WINAPI DocObjOleDocument_GetDocMiscStatus(IOleDocument *iface, DWORD *pdwStatus)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleDocument(iface);
    FIXME("(%p)->(%p)\n", This, pdwStatus);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleDocument_EnumViews(IOleDocument *iface, IEnumOleDocumentViews **ppEnum,
                                                  IOleDocumentView **ppView)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleDocument(iface);
    FIXME("(%p)->(%p %p)\n", This, ppEnum, ppView);
    return E_NOTIMPL;
}

static const IOleDocumentVtbl DocObjOleDocumentVtbl = {
    DocObjOleDocument_QueryInterface,
    DocObjOleDocument_AddRef,
    DocObjOleDocument_Release,
    DocObjOleDocument_CreateView,
    DocObjOleDocument_GetDocMiscStatus,
    DocObjOleDocument_EnumViews
};

/**********************************************************
 * IOleControl implementation
 */

static inline HTMLDocumentNode *HTMLDocumentNode_from_IOleControl(IOleControl *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IOleControl_iface);
}

static HRESULT WINAPI DocNodeOleControl_QueryInterface(IOleControl *iface, REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleControl(iface);
    return htmldoc_query_interface(&This->basedoc, riid, ppv);
}

static ULONG WINAPI DocNodeOleControl_AddRef(IOleControl *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleControl(iface);
    return htmldoc_addref(&This->basedoc);
}

static ULONG WINAPI DocNodeOleControl_Release(IOleControl *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleControl(iface);
    return htmldoc_release(&This->basedoc);
}

static HRESULT WINAPI DocNodeOleControl_GetControlInfo(IOleControl *iface, CONTROLINFO *pCI)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleControl(iface);
    FIXME("(%p)->(%p)\n", This, pCI);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleControl_OnMnemonic(IOleControl *iface, MSG *pMsg)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleControl(iface);
    FIXME("(%p)->(%p)\n", This, pMsg);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleControl_OnAmbientPropertyChange(IOleControl *iface, DISPID dispID)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleControl(iface);
    return IOleControl_OnAmbientPropertyChange(&This->basedoc.doc_obj->IOleControl_iface, dispID);
}

static HRESULT WINAPI DocNodeOleControl_FreezeEvents(IOleControl *iface, BOOL bFreeze)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleControl(iface);
    FIXME("(%p)->(%x)\n", This, bFreeze);
    return E_NOTIMPL;
}

static const IOleControlVtbl DocNodeOleControlVtbl = {
    DocNodeOleControl_QueryInterface,
    DocNodeOleControl_AddRef,
    DocNodeOleControl_Release,
    DocNodeOleControl_GetControlInfo,
    DocNodeOleControl_OnMnemonic,
    DocNodeOleControl_OnAmbientPropertyChange,
    DocNodeOleControl_FreezeEvents
};

static inline HTMLDocumentObj *HTMLDocumentObj_from_IOleControl(IOleControl *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IOleControl_iface);
}

static HRESULT WINAPI DocObjOleControl_QueryInterface(IOleControl *iface, REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleControl(iface);
    return htmldoc_query_interface(&This->basedoc, riid, ppv);
}

static ULONG WINAPI DocObjOleControl_AddRef(IOleControl *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleControl(iface);
    return htmldoc_addref(&This->basedoc);
}

static ULONG WINAPI DocObjOleControl_Release(IOleControl *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleControl(iface);
    return htmldoc_release(&This->basedoc);
}

static HRESULT WINAPI DocObjOleControl_GetControlInfo(IOleControl *iface, CONTROLINFO *pCI)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleControl(iface);
    FIXME("(%p)->(%p)\n", This, pCI);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleControl_OnMnemonic(IOleControl *iface, MSG *pMsg)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleControl(iface);
    FIXME("(%p)->(%p)\n", This, pMsg);
    return E_NOTIMPL;
}

HRESULT get_client_disp_property(IOleClientSite *client, DISPID dispid, VARIANT *res)
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

static HRESULT on_change_dlcontrol(HTMLDocumentObj *This)
{
    VARIANT res;
    HRESULT hres;

    hres = get_client_disp_property(This->client, DISPID_AMBIENT_DLCONTROL, &res);
    if(SUCCEEDED(hres))
        FIXME("unsupported dlcontrol %08lx\n", V_I4(&res));

    return S_OK;
}

static HRESULT WINAPI DocObjOleControl_OnAmbientPropertyChange(IOleControl *iface, DISPID dispID)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleControl(iface);
    IOleClientSite *client;
    VARIANT res;
    HRESULT hres;

    client = This->client;
    if(!client) {
        TRACE("client = NULL\n");
        return S_OK;
    }

    switch(dispID) {
    case DISPID_AMBIENT_USERMODE:
        TRACE("(%p)->(DISPID_AMBIENT_USERMODE)\n", This);
        hres = get_client_disp_property(client, DISPID_AMBIENT_USERMODE, &res);
        if(FAILED(hres))
            return S_OK;

        if(V_VT(&res) == VT_BOOL) {
            if(V_BOOL(&res)) {
                This->nscontainer->usermode = BROWSEMODE;
            }else {
                FIXME("edit mode is not supported\n");
                This->nscontainer->usermode = EDITMODE;
            }
        }else {
            FIXME("usermode=%s\n", debugstr_variant(&res));
        }
        return S_OK;
    case DISPID_AMBIENT_DLCONTROL:
        TRACE("(%p)->(DISPID_AMBIENT_DLCONTROL)\n", This);
        return on_change_dlcontrol(This);
    case DISPID_AMBIENT_OFFLINEIFNOTCONNECTED:
        TRACE("(%p)->(DISPID_AMBIENT_OFFLINEIFNOTCONNECTED)\n", This);
        on_change_dlcontrol(This);
        hres = get_client_disp_property(client, DISPID_AMBIENT_OFFLINEIFNOTCONNECTED, &res);
        if(FAILED(hres))
            return S_OK;

        if(V_VT(&res) == VT_BOOL) {
            if(V_BOOL(&res)) {
                FIXME("offline connection is not supported\n");
                hres = E_FAIL;
            }
        }else {
            FIXME("offlineconnected=%s\n", debugstr_variant(&res));
        }
        return S_OK;
    case DISPID_AMBIENT_SILENT:
        TRACE("(%p)->(DISPID_AMBIENT_SILENT)\n", This);
        on_change_dlcontrol(This);
        hres = get_client_disp_property(client, DISPID_AMBIENT_SILENT, &res);
        if(FAILED(hres))
            return S_OK;

        if(V_VT(&res) == VT_BOOL) {
            if(V_BOOL(&res)) {
                FIXME("silent mode is not supported\n");
            }
        }else {
            FIXME("silent=%s\n", debugstr_variant(&res));
        }
        return S_OK;
    case DISPID_AMBIENT_USERAGENT:
        TRACE("(%p)->(DISPID_AMBIENT_USERAGENT)\n", This);
        hres = get_client_disp_property(client, DISPID_AMBIENT_USERAGENT, &res);
        if(FAILED(hres))
            return S_OK;

        FIXME("not supported AMBIENT_USERAGENT\n");
        return S_OK;
    case DISPID_AMBIENT_PALETTE:
        TRACE("(%p)->(DISPID_AMBIENT_PALETTE)\n", This);
        hres = get_client_disp_property(client, DISPID_AMBIENT_PALETTE, &res);
        if(FAILED(hres))
            return S_OK;

        FIXME("not supported AMBIENT_PALETTE\n");
        return S_OK;
    }

    FIXME("(%p) unsupported dispID=%ld\n", This, dispID);
    return E_FAIL;
}

static HRESULT WINAPI DocObjOleControl_FreezeEvents(IOleControl *iface, BOOL bFreeze)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleControl(iface);
    FIXME("(%p)->(%x)\n", This, bFreeze);
    return E_NOTIMPL;
}

static const IOleControlVtbl DocObjOleControlVtbl = {
    DocObjOleControl_QueryInterface,
    DocObjOleControl_AddRef,
    DocObjOleControl_Release,
    DocObjOleControl_GetControlInfo,
    DocObjOleControl_OnMnemonic,
    DocObjOleControl_OnAmbientPropertyChange,
    DocObjOleControl_FreezeEvents
};

/**********************************************************
 * IOleInPlaceActiveObject implementation
 */

static inline HTMLDocumentNode *HTMLDocumentNode_from_IOleInPlaceActiveObject(IOleInPlaceActiveObject *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IOleInPlaceActiveObject_iface);
}

static HRESULT WINAPI DocNodeOleInPlaceActiveObject_QueryInterface(IOleInPlaceActiveObject *iface, REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceActiveObject(iface);
    return htmldoc_query_interface(&This->basedoc, riid, ppv);
}

static ULONG WINAPI DocNodeOleInPlaceActiveObject_AddRef(IOleInPlaceActiveObject *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceActiveObject(iface);
    return htmldoc_addref(&This->basedoc);
}

static ULONG WINAPI DocNodeOleInPlaceActiveObject_Release(IOleInPlaceActiveObject *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceActiveObject(iface);
    return htmldoc_release(&This->basedoc);
}

static HRESULT WINAPI DocNodeOleInPlaceActiveObject_GetWindow(IOleInPlaceActiveObject *iface, HWND *phwnd)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceActiveObject(iface);
    return IOleInPlaceActiveObject_GetWindow(&This->basedoc.doc_obj->IOleInPlaceActiveObject_iface, phwnd);
}

static HRESULT WINAPI DocNodeOleInPlaceActiveObject_ContextSensitiveHelp(IOleInPlaceActiveObject *iface, BOOL fEnterMode)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%x)\n", This, fEnterMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleInPlaceActiveObject_TranslateAccelerator(IOleInPlaceActiveObject *iface, LPMSG lpmsg)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%p)\n", This, lpmsg);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleInPlaceActiveObject_OnFrameWindowActivate(IOleInPlaceActiveObject *iface,
        BOOL fActivate)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceActiveObject(iface);
    return IOleInPlaceActiveObject_OnFrameWindowActivate(&This->basedoc.doc_obj->IOleInPlaceActiveObject_iface, fActivate);
}

static HRESULT WINAPI DocNodeOleInPlaceActiveObject_OnDocWindowActivate(IOleInPlaceActiveObject *iface, BOOL fActivate)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%x)\n", This, fActivate);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleInPlaceActiveObject_ResizeBorder(IOleInPlaceActiveObject *iface, LPCRECT prcBorder,
                                                                 IOleInPlaceUIWindow *pUIWindow, BOOL fFrameWindow)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%p %p %x)\n", This, prcBorder, pUIWindow, fFrameWindow);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleInPlaceActiveObject_EnableModeless(IOleInPlaceActiveObject *iface, BOOL fEnable)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%x)\n", This, fEnable);
    return E_NOTIMPL;
}

static const IOleInPlaceActiveObjectVtbl DocNodeOleInPlaceActiveObjectVtbl = {
    DocNodeOleInPlaceActiveObject_QueryInterface,
    DocNodeOleInPlaceActiveObject_AddRef,
    DocNodeOleInPlaceActiveObject_Release,
    DocNodeOleInPlaceActiveObject_GetWindow,
    DocNodeOleInPlaceActiveObject_ContextSensitiveHelp,
    DocNodeOleInPlaceActiveObject_TranslateAccelerator,
    DocNodeOleInPlaceActiveObject_OnFrameWindowActivate,
    DocNodeOleInPlaceActiveObject_OnDocWindowActivate,
    DocNodeOleInPlaceActiveObject_ResizeBorder,
    DocNodeOleInPlaceActiveObject_EnableModeless
};

static inline HTMLDocumentObj *HTMLDocumentObj_from_IOleInPlaceActiveObject(IOleInPlaceActiveObject *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IOleInPlaceActiveObject_iface);
}

static HRESULT WINAPI DocObjOleInPlaceActiveObject_QueryInterface(IOleInPlaceActiveObject *iface, REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceActiveObject(iface);
    return htmldoc_query_interface(&This->basedoc, riid, ppv);
}

static ULONG WINAPI DocObjOleInPlaceActiveObject_AddRef(IOleInPlaceActiveObject *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceActiveObject(iface);
    return htmldoc_addref(&This->basedoc);
}

static ULONG WINAPI DocObjOleInPlaceActiveObject_Release(IOleInPlaceActiveObject *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceActiveObject(iface);
    return htmldoc_release(&This->basedoc);
}

static HRESULT WINAPI DocObjOleInPlaceActiveObject_GetWindow(IOleInPlaceActiveObject *iface, HWND *phwnd)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceActiveObject(iface);

    TRACE("(%p)->(%p)\n", This, phwnd);

    if(!phwnd)
        return E_INVALIDARG;

    if(!This->in_place_active) {
        *phwnd = NULL;
        return E_FAIL;
    }

    *phwnd = This->hwnd;
    return S_OK;
}

static HRESULT WINAPI DocObjOleInPlaceActiveObject_ContextSensitiveHelp(IOleInPlaceActiveObject *iface, BOOL fEnterMode)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%x)\n", This, fEnterMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleInPlaceActiveObject_TranslateAccelerator(IOleInPlaceActiveObject *iface, LPMSG lpmsg)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%p)\n", This, lpmsg);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleInPlaceActiveObject_OnFrameWindowActivate(IOleInPlaceActiveObject *iface,
        BOOL fActivate)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceActiveObject(iface);

    TRACE("(%p)->(%x)\n", This, fActivate);

    if(This->hostui)
        IDocHostUIHandler_OnFrameWindowActivate(This->hostui, fActivate);

    return S_OK;
}

static HRESULT WINAPI DocObjOleInPlaceActiveObject_OnDocWindowActivate(IOleInPlaceActiveObject *iface, BOOL fActivate)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%x)\n", This, fActivate);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleInPlaceActiveObject_ResizeBorder(IOleInPlaceActiveObject *iface, LPCRECT prcBorder,
                                                                IOleInPlaceUIWindow *pUIWindow, BOOL fFrameWindow)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%p %p %x)\n", This, prcBorder, pUIWindow, fFrameWindow);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleInPlaceActiveObject_EnableModeless(IOleInPlaceActiveObject *iface, BOOL fEnable)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%x)\n", This, fEnable);
    return E_NOTIMPL;
}

static const IOleInPlaceActiveObjectVtbl DocObjOleInPlaceActiveObjectVtbl = {
    DocObjOleInPlaceActiveObject_QueryInterface,
    DocObjOleInPlaceActiveObject_AddRef,
    DocObjOleInPlaceActiveObject_Release,
    DocObjOleInPlaceActiveObject_GetWindow,
    DocObjOleInPlaceActiveObject_ContextSensitiveHelp,
    DocObjOleInPlaceActiveObject_TranslateAccelerator,
    DocObjOleInPlaceActiveObject_OnFrameWindowActivate,
    DocObjOleInPlaceActiveObject_OnDocWindowActivate,
    DocObjOleInPlaceActiveObject_ResizeBorder,
    DocObjOleInPlaceActiveObject_EnableModeless
};

/**********************************************************
 * IOleInPlaceObjectWindowless implementation
 */

static inline HTMLDocumentNode *HTMLDocumentNode_from_IOleInPlaceObjectWindowless(IOleInPlaceObjectWindowless *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IOleInPlaceObjectWindowless_iface);
}

static HRESULT WINAPI DocNodeOleInPlaceObjectWindowless_QueryInterface(IOleInPlaceObjectWindowless *iface,
        REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceObjectWindowless(iface);
    return htmldoc_query_interface(&This->basedoc, riid, ppv);
}

static ULONG WINAPI DocNodeOleInPlaceObjectWindowless_AddRef(IOleInPlaceObjectWindowless *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceObjectWindowless(iface);
    return htmldoc_addref(&This->basedoc);
}

static ULONG WINAPI DocNodeOleInPlaceObjectWindowless_Release(IOleInPlaceObjectWindowless *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceObjectWindowless(iface);
    return htmldoc_release(&This->basedoc);
}

static HRESULT WINAPI DocNodeOleInPlaceObjectWindowless_GetWindow(IOleInPlaceObjectWindowless *iface,
        HWND *phwnd)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceObjectWindowless(iface);
    return IOleInPlaceActiveObject_GetWindow(&This->IOleInPlaceActiveObject_iface, phwnd);
}

static HRESULT WINAPI DocNodeOleInPlaceObjectWindowless_ContextSensitiveHelp(IOleInPlaceObjectWindowless *iface,
        BOOL fEnterMode)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceObjectWindowless(iface);
    return IOleInPlaceActiveObject_ContextSensitiveHelp(&This->IOleInPlaceActiveObject_iface, fEnterMode);
}

static HRESULT WINAPI DocNodeOleInPlaceObjectWindowless_InPlaceDeactivate(IOleInPlaceObjectWindowless *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceObjectWindowless(iface);
    return IOleInPlaceObjectWindowless_InPlaceDeactivate(&This->basedoc.doc_obj->IOleInPlaceObjectWindowless_iface);
}

static HRESULT WINAPI DocNodeOleInPlaceObjectWindowless_UIDeactivate(IOleInPlaceObjectWindowless *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleInPlaceObjectWindowless_SetObjectRects(IOleInPlaceObjectWindowless *iface,
        const RECT *pos, const RECT *clip)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceObjectWindowless(iface);
    return IOleInPlaceObjectWindowless_SetObjectRects(&This->basedoc.doc_obj->IOleInPlaceObjectWindowless_iface, pos, clip);
}

static HRESULT WINAPI DocNodeOleInPlaceObjectWindowless_ReactivateAndUndo(IOleInPlaceObjectWindowless *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleInPlaceObjectWindowless_OnWindowMessage(IOleInPlaceObjectWindowless *iface,
        UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *lpResult)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)->(%u %Iu %Iu %p)\n", This, msg, wParam, lParam, lpResult);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleInPlaceObjectWindowless_GetDropTarget(IOleInPlaceObjectWindowless *iface,
        IDropTarget **ppDropTarget)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)->(%p)\n", This, ppDropTarget);
    return E_NOTIMPL;
}

static const IOleInPlaceObjectWindowlessVtbl DocNodeOleInPlaceObjectWindowlessVtbl = {
    DocNodeOleInPlaceObjectWindowless_QueryInterface,
    DocNodeOleInPlaceObjectWindowless_AddRef,
    DocNodeOleInPlaceObjectWindowless_Release,
    DocNodeOleInPlaceObjectWindowless_GetWindow,
    DocNodeOleInPlaceObjectWindowless_ContextSensitiveHelp,
    DocNodeOleInPlaceObjectWindowless_InPlaceDeactivate,
    DocNodeOleInPlaceObjectWindowless_UIDeactivate,
    DocNodeOleInPlaceObjectWindowless_SetObjectRects,
    DocNodeOleInPlaceObjectWindowless_ReactivateAndUndo,
    DocNodeOleInPlaceObjectWindowless_OnWindowMessage,
    DocNodeOleInPlaceObjectWindowless_GetDropTarget
};

static inline HTMLDocumentObj *HTMLDocumentObj_from_IOleInPlaceObjectWindowless(IOleInPlaceObjectWindowless *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IOleInPlaceObjectWindowless_iface);
}

static HRESULT WINAPI DocObjOleInPlaceObjectWindowless_QueryInterface(IOleInPlaceObjectWindowless *iface,
        REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceObjectWindowless(iface);
    return htmldoc_query_interface(&This->basedoc, riid, ppv);
}

static ULONG WINAPI DocObjOleInPlaceObjectWindowless_AddRef(IOleInPlaceObjectWindowless *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceObjectWindowless(iface);
    return htmldoc_addref(&This->basedoc);
}

static ULONG WINAPI DocObjOleInPlaceObjectWindowless_Release(IOleInPlaceObjectWindowless *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceObjectWindowless(iface);
    return htmldoc_release(&This->basedoc);
}

static HRESULT WINAPI DocObjOleInPlaceObjectWindowless_GetWindow(IOleInPlaceObjectWindowless *iface,
        HWND *phwnd)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceObjectWindowless(iface);
    return IOleInPlaceActiveObject_GetWindow(&This->IOleInPlaceActiveObject_iface, phwnd);
}

static HRESULT WINAPI DocObjOleInPlaceObjectWindowless_ContextSensitiveHelp(IOleInPlaceObjectWindowless *iface,
        BOOL fEnterMode)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceObjectWindowless(iface);
    return IOleInPlaceActiveObject_ContextSensitiveHelp(&This->IOleInPlaceActiveObject_iface, fEnterMode);
}

static HRESULT WINAPI DocObjOleInPlaceObjectWindowless_InPlaceDeactivate(IOleInPlaceObjectWindowless *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceObjectWindowless(iface);

    TRACE("(%p)\n", This);

    if(This->ui_active)
        IOleDocumentView_UIActivate(&This->IOleDocumentView_iface, FALSE);
    This->window_active = FALSE;

    if(!This->in_place_active)
        return S_OK;

    if(This->frame) {
        IOleInPlaceFrame_Release(This->frame);
        This->frame = NULL;
    }

    if(This->hwnd) {
        ShowWindow(This->hwnd, SW_HIDE);
        SetWindowPos(This->hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
    }

    This->focus = FALSE;
    notif_focus(This);

    This->in_place_active = FALSE;
    if(This->ipsite) {
        IOleInPlaceSiteEx *ipsiteex;
        HRESULT hres;

        hres = IOleInPlaceSite_QueryInterface(This->ipsite, &IID_IOleInPlaceSiteEx, (void**)&ipsiteex);
        if(SUCCEEDED(hres)) {
            IOleInPlaceSiteEx_OnInPlaceDeactivateEx(ipsiteex, TRUE);
            IOleInPlaceSiteEx_Release(ipsiteex);
        }else {
            IOleInPlaceSite_OnInPlaceDeactivate(This->ipsite);
        }
    }

    return S_OK;
}

static HRESULT WINAPI DocObjOleInPlaceObjectWindowless_UIDeactivate(IOleInPlaceObjectWindowless *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleInPlaceObjectWindowless_SetObjectRects(IOleInPlaceObjectWindowless *iface,
        const RECT *pos, const RECT *clip)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceObjectWindowless(iface);
    RECT r;

    TRACE("(%p)->(%s %s)\n", This, wine_dbgstr_rect(pos), wine_dbgstr_rect(clip));

    if(clip && !EqualRect(clip, pos))
        FIXME("Ignoring clip rect %s\n", wine_dbgstr_rect(clip));

    r = *pos;
    return IOleDocumentView_SetRect(&This->IOleDocumentView_iface, &r);
}

static HRESULT WINAPI DocObjOleInPlaceObjectWindowless_ReactivateAndUndo(IOleInPlaceObjectWindowless *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleInPlaceObjectWindowless_OnWindowMessage(IOleInPlaceObjectWindowless *iface,
        UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *lpResult)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)->(%u %Iu %Iu %p)\n", This, msg, wParam, lParam, lpResult);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleInPlaceObjectWindowless_GetDropTarget(IOleInPlaceObjectWindowless *iface,
        IDropTarget **ppDropTarget)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)->(%p)\n", This, ppDropTarget);
    return E_NOTIMPL;
}

static const IOleInPlaceObjectWindowlessVtbl DocObjOleInPlaceObjectWindowlessVtbl = {
    DocObjOleInPlaceObjectWindowless_QueryInterface,
    DocObjOleInPlaceObjectWindowless_AddRef,
    DocObjOleInPlaceObjectWindowless_Release,
    DocObjOleInPlaceObjectWindowless_GetWindow,
    DocObjOleInPlaceObjectWindowless_ContextSensitiveHelp,
    DocObjOleInPlaceObjectWindowless_InPlaceDeactivate,
    DocObjOleInPlaceObjectWindowless_UIDeactivate,
    DocObjOleInPlaceObjectWindowless_SetObjectRects,
    DocObjOleInPlaceObjectWindowless_ReactivateAndUndo,
    DocObjOleInPlaceObjectWindowless_OnWindowMessage,
    DocObjOleInPlaceObjectWindowless_GetDropTarget
};

/**********************************************************
 * IObjectWithSite implementation
 */

static inline HTMLDocumentNode *HTMLDocumentNode_from_IObjectWithSite(IObjectWithSite *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IObjectWithSite_iface);
}

static HRESULT WINAPI DocNodeObjectWithSite_QueryInterface(IObjectWithSite *iface, REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IObjectWithSite(iface);
    return htmldoc_query_interface(&This->basedoc, riid, ppv);
}

static ULONG WINAPI DocNodeObjectWithSite_AddRef(IObjectWithSite *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IObjectWithSite(iface);
    return htmldoc_addref(&This->basedoc);
}

static ULONG WINAPI DocNodeObjectWithSite_Release(IObjectWithSite *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IObjectWithSite(iface);
    return htmldoc_release(&This->basedoc);
}

static HRESULT WINAPI DocNodeObjectWithSite_SetSite(IObjectWithSite *iface, IUnknown *pUnkSite)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IObjectWithSite(iface);
    FIXME("(%p)->(%p)\n", This, pUnkSite);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeObjectWithSite_GetSite(IObjectWithSite* iface, REFIID riid, PVOID *ppvSite)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IObjectWithSite(iface);
    FIXME("(%p)->(%p)\n", This, ppvSite);
    return E_NOTIMPL;
}

static const IObjectWithSiteVtbl DocNodeObjectWithSiteVtbl = {
    DocNodeObjectWithSite_QueryInterface,
    DocNodeObjectWithSite_AddRef,
    DocNodeObjectWithSite_Release,
    DocNodeObjectWithSite_SetSite,
    DocNodeObjectWithSite_GetSite
};

static inline HTMLDocumentObj *HTMLDocumentObj_from_IObjectWithSite(IObjectWithSite *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IObjectWithSite_iface);
}

static HRESULT WINAPI DocObjObjectWithSite_QueryInterface(IObjectWithSite *iface, REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IObjectWithSite(iface);
    return htmldoc_query_interface(&This->basedoc, riid, ppv);
}

static ULONG WINAPI DocObjObjectWithSite_AddRef(IObjectWithSite *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IObjectWithSite(iface);
    return htmldoc_addref(&This->basedoc);
}

static ULONG WINAPI DocObjObjectWithSite_Release(IObjectWithSite *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IObjectWithSite(iface);
    return htmldoc_release(&This->basedoc);
}

static HRESULT WINAPI DocObjObjectWithSite_SetSite(IObjectWithSite *iface, IUnknown *pUnkSite)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IObjectWithSite(iface);
    FIXME("(%p)->(%p)\n", This, pUnkSite);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjObjectWithSite_GetSite(IObjectWithSite* iface, REFIID riid, PVOID *ppvSite)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IObjectWithSite(iface);
    FIXME("(%p)->(%p)\n", This, ppvSite);
    return E_NOTIMPL;
}

static const IObjectWithSiteVtbl DocObjObjectWithSiteVtbl = {
    DocObjObjectWithSite_QueryInterface,
    DocObjObjectWithSite_AddRef,
    DocObjObjectWithSite_Release,
    DocObjObjectWithSite_SetSite,
    DocObjObjectWithSite_GetSite
};

/**********************************************************
 * IOleContainer implementation
 */

static inline HTMLDocumentNode *HTMLDocumentNode_from_IOleContainer(IOleContainer *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IOleContainer_iface);
}

static HRESULT WINAPI DocNodeOleContainer_QueryInterface(IOleContainer *iface, REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleContainer(iface);
    return htmldoc_query_interface(&This->basedoc, riid, ppv);
}

static ULONG WINAPI DocNodeOleContainer_AddRef(IOleContainer *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleContainer(iface);
    return htmldoc_addref(&This->basedoc);
}

static ULONG WINAPI DocNodeOleContainer_Release(IOleContainer *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleContainer(iface);
    return htmldoc_release(&This->basedoc);
}

static HRESULT WINAPI DocNodeOleContainer_ParseDisplayName(IOleContainer *iface, IBindCtx *pbc, LPOLESTR pszDisplayName,
        ULONG *pchEaten, IMoniker **ppmkOut)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleContainer(iface);
    FIXME("(%p)->(%p %s %p %p)\n", This, pbc, debugstr_w(pszDisplayName), pchEaten, ppmkOut);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleContainer_EnumObjects(IOleContainer *iface, DWORD grfFlags, IEnumUnknown **ppenum)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleContainer(iface);
    return IOleContainer_EnumObjects(&This->basedoc.doc_obj->IOleContainer_iface, grfFlags, ppenum);
}

static HRESULT WINAPI DocNodeOleContainer_LockContainer(IOleContainer *iface, BOOL fLock)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleContainer(iface);
    FIXME("(%p)->(%x)\n", This, fLock);
    return E_NOTIMPL;
}

static const IOleContainerVtbl DocNodeOleContainerVtbl = {
    DocNodeOleContainer_QueryInterface,
    DocNodeOleContainer_AddRef,
    DocNodeOleContainer_Release,
    DocNodeOleContainer_ParseDisplayName,
    DocNodeOleContainer_EnumObjects,
    DocNodeOleContainer_LockContainer
};

static inline HTMLDocumentObj *HTMLDocumentObj_from_IOleContainer(IOleContainer *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IOleContainer_iface);
}

static HRESULT WINAPI DocObjOleContainer_QueryInterface(IOleContainer *iface, REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleContainer(iface);
    return htmldoc_query_interface(&This->basedoc, riid, ppv);
}

static ULONG WINAPI DocObjOleContainer_AddRef(IOleContainer *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleContainer(iface);
    return htmldoc_addref(&This->basedoc);
}

static ULONG WINAPI DocObjOleContainer_Release(IOleContainer *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleContainer(iface);
    return htmldoc_release(&This->basedoc);
}

static HRESULT WINAPI DocObjOleContainer_ParseDisplayName(IOleContainer *iface, IBindCtx *pbc, LPOLESTR pszDisplayName,
        ULONG *pchEaten, IMoniker **ppmkOut)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleContainer(iface);
    FIXME("(%p)->(%p %s %p %p)\n", This, pbc, debugstr_w(pszDisplayName), pchEaten, ppmkOut);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleContainer_EnumObjects(IOleContainer *iface, DWORD grfFlags, IEnumUnknown **ppenum)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleContainer(iface);
    EnumUnknown *ret;

    TRACE("(%p)->(%lx %p)\n", This, grfFlags, ppenum);

    ret = heap_alloc(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IEnumUnknown_iface.lpVtbl = &EnumUnknownVtbl;
    ret->ref = 1;

    *ppenum = &ret->IEnumUnknown_iface;
    return S_OK;
}

static HRESULT WINAPI DocObjOleContainer_LockContainer(IOleContainer *iface, BOOL fLock)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleContainer(iface);
    FIXME("(%p)->(%x)\n", This, fLock);
    return E_NOTIMPL;
}

static const IOleContainerVtbl DocObjOleContainerVtbl = {
    DocObjOleContainer_QueryInterface,
    DocObjOleContainer_AddRef,
    DocObjOleContainer_Release,
    DocObjOleContainer_ParseDisplayName,
    DocObjOleContainer_EnumObjects,
    DocObjOleContainer_LockContainer
};

static inline HTMLDocumentObj *impl_from_ITargetContainer(ITargetContainer *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, ITargetContainer_iface);
}

static HRESULT WINAPI TargetContainer_QueryInterface(ITargetContainer *iface, REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = impl_from_ITargetContainer(iface);
    return ICustomDoc_QueryInterface(&This->ICustomDoc_iface, riid, ppv);
}

static ULONG WINAPI TargetContainer_AddRef(ITargetContainer *iface)
{
    HTMLDocumentObj *This = impl_from_ITargetContainer(iface);
    return ICustomDoc_AddRef(&This->ICustomDoc_iface);
}

static ULONG WINAPI TargetContainer_Release(ITargetContainer *iface)
{
    HTMLDocumentObj *This = impl_from_ITargetContainer(iface);
    return ICustomDoc_Release(&This->ICustomDoc_iface);
}

static HRESULT WINAPI TargetContainer_GetFrameUrl(ITargetContainer *iface, LPWSTR *ppszFrameSrc)
{
    HTMLDocumentObj *This = impl_from_ITargetContainer(iface);
    FIXME("(%p)->(%p)\n", This, ppszFrameSrc);
    return E_NOTIMPL;
}

static HRESULT WINAPI TargetContainer_GetFramesContainer(ITargetContainer *iface, IOleContainer **ppContainer)
{
    HTMLDocumentObj *This = impl_from_ITargetContainer(iface);

    TRACE("(%p)->(%p)\n", This, ppContainer);

    /* NOTE: we should return wrapped interface here */
    IOleContainer_AddRef(&This->IOleContainer_iface);
    *ppContainer = &This->IOleContainer_iface;
    return S_OK;
}

static const ITargetContainerVtbl TargetContainerVtbl = {
    TargetContainer_QueryInterface,
    TargetContainer_AddRef,
    TargetContainer_Release,
    TargetContainer_GetFrameUrl,
    TargetContainer_GetFramesContainer
};

void TargetContainer_Init(HTMLDocumentObj *This)
{
    This->ITargetContainer_iface.lpVtbl = &TargetContainerVtbl;
}

/**********************************************************
 * IObjectSafety implementation
 */

static inline HTMLDocumentNode *HTMLDocumentNode_from_IObjectSafety(IObjectSafety *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IObjectSafety_iface);
}

static HRESULT WINAPI DocNodeObjectSafety_QueryInterface(IObjectSafety *iface, REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IObjectSafety(iface);
    return htmldoc_query_interface(&This->basedoc, riid, ppv);
}

static ULONG WINAPI DocNodeObjectSafety_AddRef(IObjectSafety *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IObjectSafety(iface);
    return htmldoc_addref(&This->basedoc);
}

static ULONG WINAPI DocNodeObjectSafety_Release(IObjectSafety *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IObjectSafety(iface);
    return htmldoc_release(&This->basedoc);
}

static HRESULT WINAPI DocNodeObjectSafety_GetInterfaceSafetyOptions(IObjectSafety *iface,
        REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IObjectSafety(iface);
    FIXME("(%p)->(%s %p %p)\n", This, debugstr_guid(riid), pdwSupportedOptions, pdwEnabledOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeObjectSafety_SetInterfaceSafetyOptions(IObjectSafety *iface,
        REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IObjectSafety(iface);
    return IObjectSafety_SetInterfaceSafetyOptions(&This->basedoc.doc_obj->IObjectSafety_iface,
                                                   riid, dwOptionSetMask, dwEnabledOptions);
}

static const IObjectSafetyVtbl DocNodeObjectSafetyVtbl = {
    DocNodeObjectSafety_QueryInterface,
    DocNodeObjectSafety_AddRef,
    DocNodeObjectSafety_Release,
    DocNodeObjectSafety_GetInterfaceSafetyOptions,
    DocNodeObjectSafety_SetInterfaceSafetyOptions
};

static inline HTMLDocumentObj *HTMLDocumentObj_from_IObjectSafety(IObjectSafety *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IObjectSafety_iface);
}

static HRESULT WINAPI DocObjObjectSafety_QueryInterface(IObjectSafety *iface, REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IObjectSafety(iface);
    return htmldoc_query_interface(&This->basedoc, riid, ppv);
}

static ULONG WINAPI DocObjObjectSafety_AddRef(IObjectSafety *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IObjectSafety(iface);
    return htmldoc_addref(&This->basedoc);
}

static ULONG WINAPI DocObjObjectSafety_Release(IObjectSafety *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IObjectSafety(iface);
    return htmldoc_release(&This->basedoc);
}

static HRESULT WINAPI DocObjObjectSafety_GetInterfaceSafetyOptions(IObjectSafety *iface,
        REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IObjectSafety(iface);
    FIXME("(%p)->(%s %p %p)\n", This, debugstr_guid(riid), pdwSupportedOptions, pdwEnabledOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjObjectSafety_SetInterfaceSafetyOptions(IObjectSafety *iface,
        REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IObjectSafety(iface);
    FIXME("(%p)->(%s %lx %lx)\n", This, debugstr_guid(riid), dwOptionSetMask, dwEnabledOptions);

    if(IsEqualGUID(&IID_IPersistMoniker, riid) &&
            dwOptionSetMask==INTERFACESAFE_FOR_UNTRUSTED_DATA &&
            dwEnabledOptions==INTERFACESAFE_FOR_UNTRUSTED_DATA)
        return S_OK;

    return E_NOTIMPL;
}

static const IObjectSafetyVtbl DocObjObjectSafetyVtbl = {
    DocObjObjectSafety_QueryInterface,
    DocObjObjectSafety_AddRef,
    DocObjObjectSafety_Release,
    DocObjObjectSafety_GetInterfaceSafetyOptions,
    DocObjObjectSafety_SetInterfaceSafetyOptions
};

void HTMLDocument_LockContainer(HTMLDocumentObj *This, BOOL fLock)
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

void HTMLDocumentNode_OleObj_Init(HTMLDocumentNode *This)
{
    This->IOleObject_iface.lpVtbl = &DocNodeOleObjectVtbl;
    This->IOleDocument_iface.lpVtbl = &DocNodeOleDocumentVtbl;
    This->IOleControl_iface.lpVtbl = &DocNodeOleControlVtbl;
    This->IOleInPlaceActiveObject_iface.lpVtbl = &DocNodeOleInPlaceActiveObjectVtbl;
    This->IOleInPlaceObjectWindowless_iface.lpVtbl = &DocNodeOleInPlaceObjectWindowlessVtbl;
    This->IObjectWithSite_iface.lpVtbl = &DocNodeObjectWithSiteVtbl;
    This->IOleContainer_iface.lpVtbl = &DocNodeOleContainerVtbl;
    This->IObjectSafety_iface.lpVtbl = &DocNodeObjectSafetyVtbl;
    This->basedoc.doc_obj->extent.cx = 1;
    This->basedoc.doc_obj->extent.cy = 1;
}

void HTMLDocumentObj_OleObj_Init(HTMLDocumentObj *This)
{
    This->IOleObject_iface.lpVtbl = &DocObjOleObjectVtbl;
    This->IOleDocument_iface.lpVtbl = &DocObjOleDocumentVtbl;
    This->IOleControl_iface.lpVtbl = &DocObjOleControlVtbl;
    This->IOleInPlaceActiveObject_iface.lpVtbl = &DocObjOleInPlaceActiveObjectVtbl;
    This->IOleInPlaceObjectWindowless_iface.lpVtbl = &DocObjOleInPlaceObjectWindowlessVtbl;
    This->IObjectWithSite_iface.lpVtbl = &DocObjObjectWithSiteVtbl;
    This->IOleContainer_iface.lpVtbl = &DocObjOleContainerVtbl;
    This->IObjectSafety_iface.lpVtbl = &DocObjObjectSafetyVtbl;
    This->extent.cx = 1;
    This->extent.cy = 1;
}
