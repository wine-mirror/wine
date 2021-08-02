/*
 * Copyright 2008,2009 Jacek Caban for CodeWeavers
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct HTMLPluginsCollection HTMLPluginsCollection;
typedef struct HTMLMimeTypesCollection HTMLMimeTypesCollection;

typedef struct {
    DispatchEx dispex;
    IOmNavigator IOmNavigator_iface;

    LONG ref;

    HTMLPluginsCollection *plugins;
    HTMLMimeTypesCollection *mime_types;
} OmNavigator;

typedef struct {
    DispatchEx dispex;
    IHTMLDOMImplementation IHTMLDOMImplementation_iface;
    IHTMLDOMImplementation2 IHTMLDOMImplementation2_iface;

    LONG ref;

    nsIDOMDOMImplementation *implementation;
    GeckoBrowser *browser;
} HTMLDOMImplementation;

static inline HTMLDOMImplementation *impl_from_IHTMLDOMImplementation(IHTMLDOMImplementation *iface)
{
    return CONTAINING_RECORD(iface, HTMLDOMImplementation, IHTMLDOMImplementation_iface);
}

static HRESULT WINAPI HTMLDOMImplementation_QueryInterface(IHTMLDOMImplementation *iface, REFIID riid, void **ppv)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IHTMLDOMImplementation, riid)) {
        *ppv = &This->IHTMLDOMImplementation_iface;
    }else if(IsEqualGUID(&IID_IHTMLDOMImplementation2, riid)) {
        *ppv = &This->IHTMLDOMImplementation2_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        WARN("Unsupported interface %s\n", debugstr_mshtml_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLDOMImplementation_AddRef(IHTMLDOMImplementation *iface)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLDOMImplementation_Release(IHTMLDOMImplementation *iface)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        assert(!This->browser);
        if(This->implementation)
            nsIDOMDOMImplementation_Release(This->implementation);
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLDOMImplementation_GetTypeInfoCount(IHTMLDOMImplementation *iface, UINT *pctinfo)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation(iface);

    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLDOMImplementation_GetTypeInfo(IHTMLDOMImplementation *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation(iface);

    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLDOMImplementation_GetIDsOfNames(IHTMLDOMImplementation *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation(iface);

    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLDOMImplementation_Invoke(IHTMLDOMImplementation *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation(iface);

    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLDOMImplementation_hasFeature(IHTMLDOMImplementation *iface, BSTR feature,
        VARIANT version, VARIANT_BOOL *pfHasFeature)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation(iface);

    FIXME("(%p)->(%s %s %p) returning false\n", This, debugstr_w(feature), debugstr_variant(&version), pfHasFeature);

    *pfHasFeature = VARIANT_FALSE;
    return S_OK;
}

static const IHTMLDOMImplementationVtbl HTMLDOMImplementationVtbl = {
    HTMLDOMImplementation_QueryInterface,
    HTMLDOMImplementation_AddRef,
    HTMLDOMImplementation_Release,
    HTMLDOMImplementation_GetTypeInfoCount,
    HTMLDOMImplementation_GetTypeInfo,
    HTMLDOMImplementation_GetIDsOfNames,
    HTMLDOMImplementation_Invoke,
    HTMLDOMImplementation_hasFeature
};

static inline HTMLDOMImplementation *impl_from_IHTMLDOMImplementation2(IHTMLDOMImplementation2 *iface)
{
    return CONTAINING_RECORD(iface, HTMLDOMImplementation, IHTMLDOMImplementation2_iface);
}

static HRESULT WINAPI HTMLDOMImplementation2_QueryInterface(IHTMLDOMImplementation2 *iface, REFIID riid, void **ppv)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation2(iface);
    return IHTMLDOMImplementation_QueryInterface(&This->IHTMLDOMImplementation_iface, riid, ppv);
}

static ULONG WINAPI HTMLDOMImplementation2_AddRef(IHTMLDOMImplementation2 *iface)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation2(iface);
    return IHTMLDOMImplementation_AddRef(&This->IHTMLDOMImplementation_iface);
}

static ULONG WINAPI HTMLDOMImplementation2_Release(IHTMLDOMImplementation2 *iface)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation2(iface);
    return IHTMLDOMImplementation_Release(&This->IHTMLDOMImplementation_iface);
}

static HRESULT WINAPI HTMLDOMImplementation2_GetTypeInfoCount(IHTMLDOMImplementation2 *iface, UINT *pctinfo)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation2(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLDOMImplementation2_GetTypeInfo(IHTMLDOMImplementation2 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation2(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLDOMImplementation2_GetIDsOfNames(IHTMLDOMImplementation2 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation2(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLDOMImplementation2_Invoke(IHTMLDOMImplementation2 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation2(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLDOMImplementation2_createDocumentType(IHTMLDOMImplementation2 *iface, BSTR name,
        VARIANT *public_id, VARIANT *system_id, IDOMDocumentType **new_type)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation2(iface);
    FIXME("(%p)->(%s %s %s %p)\n", This, debugstr_w(name), debugstr_variant(public_id),
          debugstr_variant(system_id), new_type);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMImplementation2_createDocument(IHTMLDOMImplementation2 *iface, VARIANT *ns,
        VARIANT *tag_name, IDOMDocumentType *document_type, IHTMLDocument7 **new_document)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation2(iface);
    FIXME("(%p)->(%s %s %p %p)\n", This, debugstr_variant(ns), debugstr_variant(tag_name),
          document_type, new_document);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMImplementation2_createHTMLDocument(IHTMLDOMImplementation2 *iface, BSTR title,
        IHTMLDocument7 **new_document)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation2(iface);
    HTMLDocumentNode *new_document_node;
    nsIDOMHTMLDocument *html_doc;
    nsIDOMDocument *doc;
    nsAString title_str;
    nsresult nsres;
    HRESULT hres;

    FIXME("(%p)->(%s %p)\n", This, debugstr_w(title), new_document);

    if(!This->browser)
        return E_UNEXPECTED;

    nsAString_InitDepend(&title_str, title);
    nsres = nsIDOMDOMImplementation_CreateHTMLDocument(This->implementation, &title_str, &doc);
    nsAString_Finish(&title_str);
    if(NS_FAILED(nsres)) {
        ERR("CreateHTMLDocument failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMDocument_QueryInterface(doc, &IID_nsIDOMHTMLDocument, (void**)&html_doc);
    nsIDOMDocument_Release(doc);
    assert(nsres == NS_OK);

    hres = create_document_node(html_doc, This->browser, NULL, dispex_compat_mode(&This->dispex), &new_document_node);
    nsIDOMHTMLDocument_Release(html_doc);
    if(FAILED(hres))
        return hres;

    *new_document = &new_document_node->basedoc.IHTMLDocument7_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLDOMImplementation2_hasFeature(IHTMLDOMImplementation2 *iface, BSTR feature,
        VARIANT version, VARIANT_BOOL *pfHasFeature)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation2(iface);

    FIXME("(%p)->(%s %s %p) returning false\n", This, debugstr_w(feature), debugstr_variant(&version), pfHasFeature);

    *pfHasFeature = VARIANT_FALSE;
    return S_OK;
}

static const IHTMLDOMImplementation2Vtbl HTMLDOMImplementation2Vtbl = {
    HTMLDOMImplementation2_QueryInterface,
    HTMLDOMImplementation2_AddRef,
    HTMLDOMImplementation2_Release,
    HTMLDOMImplementation2_GetTypeInfoCount,
    HTMLDOMImplementation2_GetTypeInfo,
    HTMLDOMImplementation2_GetIDsOfNames,
    HTMLDOMImplementation2_Invoke,
    HTMLDOMImplementation2_createDocumentType,
    HTMLDOMImplementation2_createDocument,
    HTMLDOMImplementation2_createHTMLDocument,
    HTMLDOMImplementation2_hasFeature
};

static const tid_t HTMLDOMImplementation_iface_tids[] = {
    IHTMLDOMImplementation_tid,
    0
};
static dispex_static_data_t HTMLDOMImplementation_dispex = {
    NULL,
    DispHTMLDOMImplementation_tid,
    HTMLDOMImplementation_iface_tids
};

HRESULT create_dom_implementation(HTMLDocumentNode *doc_node, IHTMLDOMImplementation **ret)
{
    HTMLDOMImplementation *dom_implementation;
    nsresult nsres;

    if(!doc_node->browser)
        return E_UNEXPECTED;

    dom_implementation = heap_alloc_zero(sizeof(*dom_implementation));
    if(!dom_implementation)
        return E_OUTOFMEMORY;

    dom_implementation->IHTMLDOMImplementation_iface.lpVtbl = &HTMLDOMImplementationVtbl;
    dom_implementation->IHTMLDOMImplementation2_iface.lpVtbl = &HTMLDOMImplementation2Vtbl;
    dom_implementation->ref = 1;
    dom_implementation->browser = doc_node->browser;

    init_dispatch(&dom_implementation->dispex, (IUnknown*)&dom_implementation->IHTMLDOMImplementation_iface,
                  &HTMLDOMImplementation_dispex, doc_node->document_mode);

    nsres = nsIDOMHTMLDocument_GetImplementation(doc_node->nsdoc, &dom_implementation->implementation);
    if(NS_FAILED(nsres)) {
        ERR("GetDOMImplementation failed: %08x\n", nsres);
        IHTMLDOMImplementation_Release(&dom_implementation->IHTMLDOMImplementation_iface);
        return E_FAIL;
    }

    *ret = &dom_implementation->IHTMLDOMImplementation_iface;
    return S_OK;
}

void detach_dom_implementation(IHTMLDOMImplementation *iface)
{
    HTMLDOMImplementation *dom_implementation = impl_from_IHTMLDOMImplementation(iface);
    dom_implementation->browser = NULL;
}

typedef struct {
    DispatchEx dispex;
    IHTMLScreen IHTMLScreen_iface;

    LONG ref;
} HTMLScreen;

static inline HTMLScreen *impl_from_IHTMLScreen(IHTMLScreen *iface)
{
    return CONTAINING_RECORD(iface, HTMLScreen, IHTMLScreen_iface);
}

static HRESULT WINAPI HTMLScreen_QueryInterface(IHTMLScreen *iface, REFIID riid, void **ppv)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLScreen_iface;
    }else if(IsEqualGUID(&IID_IHTMLScreen, riid)) {
        *ppv = &This->IHTMLScreen_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        *ppv = NULL;
        WARN("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLScreen_AddRef(IHTMLScreen *iface)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLScreen_Release(IHTMLScreen *iface)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLScreen_GetTypeInfoCount(IHTMLScreen *iface, UINT *pctinfo)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLScreen_GetTypeInfo(IHTMLScreen *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLScreen_GetIDsOfNames(IHTMLScreen *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLScreen_Invoke(IHTMLScreen *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLScreen_get_colorDepth(IHTMLScreen *iface, LONG *p)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    HDC hdc = GetDC(0);

    TRACE("(%p)->(%p)\n", This, p);

    *p = GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(0, hdc);
    return S_OK;
}

static HRESULT WINAPI HTMLScreen_put_bufferDepth(IHTMLScreen *iface, LONG v)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScreen_get_bufferDepth(IHTMLScreen *iface, LONG *p)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScreen_get_width(IHTMLScreen *iface, LONG *p)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = GetSystemMetrics(SM_CXSCREEN);
    return S_OK;
}

static HRESULT WINAPI HTMLScreen_get_height(IHTMLScreen *iface, LONG *p)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = GetSystemMetrics(SM_CYSCREEN);
    return S_OK;
}

static HRESULT WINAPI HTMLScreen_put_updateInterval(IHTMLScreen *iface, LONG v)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScreen_get_updateInterval(IHTMLScreen *iface, LONG *p)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScreen_get_availHeight(IHTMLScreen *iface, LONG *p)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    RECT work_area;

    TRACE("(%p)->(%p)\n", This, p);

    if(!SystemParametersInfoW(SPI_GETWORKAREA, 0, &work_area, 0))
        return E_FAIL;

    *p = work_area.bottom-work_area.top;
    return S_OK;
}

static HRESULT WINAPI HTMLScreen_get_availWidth(IHTMLScreen *iface, LONG *p)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    RECT work_area;

    TRACE("(%p)->(%p)\n", This, p);

    if(!SystemParametersInfoW(SPI_GETWORKAREA, 0, &work_area, 0))
        return E_FAIL;

    *p = work_area.right-work_area.left;
    return S_OK;
}

static HRESULT WINAPI HTMLScreen_get_fontSmoothingEnabled(IHTMLScreen *iface, VARIANT_BOOL *p)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLScreenVtbl HTMLSreenVtbl = {
    HTMLScreen_QueryInterface,
    HTMLScreen_AddRef,
    HTMLScreen_Release,
    HTMLScreen_GetTypeInfoCount,
    HTMLScreen_GetTypeInfo,
    HTMLScreen_GetIDsOfNames,
    HTMLScreen_Invoke,
    HTMLScreen_get_colorDepth,
    HTMLScreen_put_bufferDepth,
    HTMLScreen_get_bufferDepth,
    HTMLScreen_get_width,
    HTMLScreen_get_height,
    HTMLScreen_put_updateInterval,
    HTMLScreen_get_updateInterval,
    HTMLScreen_get_availHeight,
    HTMLScreen_get_availWidth,
    HTMLScreen_get_fontSmoothingEnabled
};

static const tid_t HTMLScreen_iface_tids[] = {
    IHTMLScreen_tid,
    0
};
static dispex_static_data_t HTMLScreen_dispex = {
    NULL,
    DispHTMLScreen_tid,
    HTMLScreen_iface_tids
};

HRESULT create_html_screen(compat_mode_t compat_mode, IHTMLScreen **ret)
{
    HTMLScreen *screen;

    screen = heap_alloc_zero(sizeof(HTMLScreen));
    if(!screen)
        return E_OUTOFMEMORY;

    screen->IHTMLScreen_iface.lpVtbl = &HTMLSreenVtbl;
    screen->ref = 1;

    init_dispatch(&screen->dispex, (IUnknown*)&screen->IHTMLScreen_iface, &HTMLScreen_dispex, compat_mode);

    *ret = &screen->IHTMLScreen_iface;
    return S_OK;
}

static inline OmHistory *impl_from_IOmHistory(IOmHistory *iface)
{
    return CONTAINING_RECORD(iface, OmHistory, IOmHistory_iface);
}

static HRESULT WINAPI OmHistory_QueryInterface(IOmHistory *iface, REFIID riid, void **ppv)
{
    OmHistory *This = impl_from_IOmHistory(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IOmHistory_iface;
    }else if(IsEqualGUID(&IID_IOmHistory, riid)) {
        *ppv = &This->IOmHistory_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        WARN("Unsupported interface %s\n", debugstr_mshtml_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI OmHistory_AddRef(IOmHistory *iface)
{
    OmHistory *This = impl_from_IOmHistory(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI OmHistory_Release(IOmHistory *iface)
{
    OmHistory *This = impl_from_IOmHistory(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI OmHistory_GetTypeInfoCount(IOmHistory *iface, UINT *pctinfo)
{
    OmHistory *This = impl_from_IOmHistory(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmHistory_GetTypeInfo(IOmHistory *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    OmHistory *This = impl_from_IOmHistory(iface);

    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI OmHistory_GetIDsOfNames(IOmHistory *iface, REFIID riid, LPOLESTR *rgszNames, UINT cNames,
        LCID lcid, DISPID *rgDispId)
{
    OmHistory *This = impl_from_IOmHistory(iface);

    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI OmHistory_Invoke(IOmHistory *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    OmHistory *This = impl_from_IOmHistory(iface);

    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI OmHistory_get_length(IOmHistory *iface, short *p)
{
    OmHistory *This = impl_from_IOmHistory(iface);
    GeckoBrowser *browser = NULL;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->window && This->window->base.outer_window)
        browser = This->window->base.outer_window->browser;

    *p = browser->doc->travel_log
        ? ITravelLog_CountEntries(browser->doc->travel_log, browser->doc->browser_service)
        : 0;
    return S_OK;
}

static HRESULT WINAPI OmHistory_back(IOmHistory *iface, VARIANT *pvargdistance)
{
    OmHistory *This = impl_from_IOmHistory(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(pvargdistance));
    return E_NOTIMPL;
}

static HRESULT WINAPI OmHistory_forward(IOmHistory *iface, VARIANT *pvargdistance)
{
    OmHistory *This = impl_from_IOmHistory(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(pvargdistance));
    return E_NOTIMPL;
}

static HRESULT WINAPI OmHistory_go(IOmHistory *iface, VARIANT *pvargdistance)
{
    OmHistory *This = impl_from_IOmHistory(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(pvargdistance));
    return E_NOTIMPL;
}

static const IOmHistoryVtbl OmHistoryVtbl = {
    OmHistory_QueryInterface,
    OmHistory_AddRef,
    OmHistory_Release,
    OmHistory_GetTypeInfoCount,
    OmHistory_GetTypeInfo,
    OmHistory_GetIDsOfNames,
    OmHistory_Invoke,
    OmHistory_get_length,
    OmHistory_back,
    OmHistory_forward,
    OmHistory_go
};

static const tid_t OmHistory_iface_tids[] = {
    IOmHistory_tid,
    0
};
static dispex_static_data_t OmHistory_dispex = {
    NULL,
    DispHTMLHistory_tid,
    OmHistory_iface_tids
};


HRESULT create_history(HTMLInnerWindow *window, OmHistory **ret)
{
    OmHistory *history;

    history = heap_alloc_zero(sizeof(*history));
    if(!history)
        return E_OUTOFMEMORY;

    init_dispatch(&history->dispex, (IUnknown*)&history->IOmHistory_iface, &OmHistory_dispex,
                  dispex_compat_mode(&window->event_target.dispex));
    history->IOmHistory_iface.lpVtbl = &OmHistoryVtbl;
    history->ref = 1;

    history->window = window;

    *ret = history;
    return S_OK;
}

struct HTMLPluginsCollection {
    DispatchEx dispex;
    IHTMLPluginsCollection IHTMLPluginsCollection_iface;

    LONG ref;

    OmNavigator *navigator;
};

static inline HTMLPluginsCollection *impl_from_IHTMLPluginsCollection(IHTMLPluginsCollection *iface)
{
    return CONTAINING_RECORD(iface, HTMLPluginsCollection, IHTMLPluginsCollection_iface);
}

static HRESULT WINAPI HTMLPluginsCollection_QueryInterface(IHTMLPluginsCollection *iface, REFIID riid, void **ppv)
{
    HTMLPluginsCollection *This = impl_from_IHTMLPluginsCollection(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLPluginsCollection_iface;
    }else if(IsEqualGUID(&IID_IHTMLPluginsCollection, riid)) {
        *ppv = &This->IHTMLPluginsCollection_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        *ppv = NULL;
        WARN("Unsupported interface %s\n", debugstr_mshtml_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLPluginsCollection_AddRef(IHTMLPluginsCollection *iface)
{
    HTMLPluginsCollection *This = impl_from_IHTMLPluginsCollection(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLPluginsCollection_Release(IHTMLPluginsCollection *iface)
{
    HTMLPluginsCollection *This = impl_from_IHTMLPluginsCollection(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->navigator)
            This->navigator->plugins = NULL;
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLPluginsCollection_GetTypeInfoCount(IHTMLPluginsCollection *iface, UINT *pctinfo)
{
    HTMLPluginsCollection *This = impl_from_IHTMLPluginsCollection(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLPluginsCollection_GetTypeInfo(IHTMLPluginsCollection *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLPluginsCollection *This = impl_from_IHTMLPluginsCollection(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLPluginsCollection_GetIDsOfNames(IHTMLPluginsCollection *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLPluginsCollection *This = impl_from_IHTMLPluginsCollection(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLPluginsCollection_Invoke(IHTMLPluginsCollection *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLPluginsCollection *This = impl_from_IHTMLPluginsCollection(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLPluginsCollection_get_length(IHTMLPluginsCollection *iface, LONG *p)
{
    HTMLPluginsCollection *This = impl_from_IHTMLPluginsCollection(iface);

    TRACE("(%p)->(%p)\n", This, p);

    /* IE always returns 0 here */
    *p = 0;
    return S_OK;
}

static HRESULT WINAPI HTMLPluginsCollection_refresh(IHTMLPluginsCollection *iface, VARIANT_BOOL reload)
{
    HTMLPluginsCollection *This = impl_from_IHTMLPluginsCollection(iface);

    TRACE("(%p)->(%x)\n", This, reload);

    /* Nothing to do here. */
    return S_OK;
}

static const IHTMLPluginsCollectionVtbl HTMLPluginsCollectionVtbl = {
    HTMLPluginsCollection_QueryInterface,
    HTMLPluginsCollection_AddRef,
    HTMLPluginsCollection_Release,
    HTMLPluginsCollection_GetTypeInfoCount,
    HTMLPluginsCollection_GetTypeInfo,
    HTMLPluginsCollection_GetIDsOfNames,
    HTMLPluginsCollection_Invoke,
    HTMLPluginsCollection_get_length,
    HTMLPluginsCollection_refresh
};

static const tid_t HTMLPluginsCollection_iface_tids[] = {
    IHTMLPluginsCollection_tid,
    0
};
static dispex_static_data_t HTMLPluginsCollection_dispex = {
    NULL,
    DispCPlugins_tid,
    HTMLPluginsCollection_iface_tids
};

static HRESULT create_plugins_collection(OmNavigator *navigator, HTMLPluginsCollection **ret)
{
    HTMLPluginsCollection *col;

    col = heap_alloc_zero(sizeof(*col));
    if(!col)
        return E_OUTOFMEMORY;

    col->IHTMLPluginsCollection_iface.lpVtbl = &HTMLPluginsCollectionVtbl;
    col->ref = 1;
    col->navigator = navigator;

    init_dispatch(&col->dispex, (IUnknown*)&col->IHTMLPluginsCollection_iface,
                  &HTMLPluginsCollection_dispex, dispex_compat_mode(&navigator->dispex));

    *ret = col;
    return S_OK;
}

struct HTMLMimeTypesCollection {
    DispatchEx dispex;
    IHTMLMimeTypesCollection IHTMLMimeTypesCollection_iface;

    LONG ref;

    OmNavigator *navigator;
};

static inline HTMLMimeTypesCollection *impl_from_IHTMLMimeTypesCollection(IHTMLMimeTypesCollection *iface)
{
    return CONTAINING_RECORD(iface, HTMLMimeTypesCollection, IHTMLMimeTypesCollection_iface);
}

static HRESULT WINAPI HTMLMimeTypesCollection_QueryInterface(IHTMLMimeTypesCollection *iface, REFIID riid, void **ppv)
{
    HTMLMimeTypesCollection *This = impl_from_IHTMLMimeTypesCollection(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLMimeTypesCollection_iface;
    }else if(IsEqualGUID(&IID_IHTMLMimeTypesCollection, riid)) {
        *ppv = &This->IHTMLMimeTypesCollection_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        WARN("Unsupported interface %s\n", debugstr_mshtml_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLMimeTypesCollection_AddRef(IHTMLMimeTypesCollection *iface)
{
    HTMLMimeTypesCollection *This = impl_from_IHTMLMimeTypesCollection(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLMimeTypesCollection_Release(IHTMLMimeTypesCollection *iface)
{
    HTMLMimeTypesCollection *This = impl_from_IHTMLMimeTypesCollection(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->navigator)
            This->navigator->mime_types = NULL;
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLMimeTypesCollection_GetTypeInfoCount(IHTMLMimeTypesCollection *iface, UINT *pctinfo)
{
    HTMLMimeTypesCollection *This = impl_from_IHTMLMimeTypesCollection(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLMimeTypesCollection_GetTypeInfo(IHTMLMimeTypesCollection *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLMimeTypesCollection *This = impl_from_IHTMLMimeTypesCollection(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLMimeTypesCollection_GetIDsOfNames(IHTMLMimeTypesCollection *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLMimeTypesCollection *This = impl_from_IHTMLMimeTypesCollection(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLMimeTypesCollection_Invoke(IHTMLMimeTypesCollection *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLMimeTypesCollection *This = impl_from_IHTMLMimeTypesCollection(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLMimeTypesCollection_get_length(IHTMLMimeTypesCollection *iface, LONG *p)
{
    HTMLMimeTypesCollection *This = impl_from_IHTMLMimeTypesCollection(iface);

    TRACE("(%p)->(%p)\n", This, p);

    /* This is just a stub for compatibility with other browser in IE */
    *p = 0;
    return S_OK;
}

static const IHTMLMimeTypesCollectionVtbl HTMLMimeTypesCollectionVtbl = {
    HTMLMimeTypesCollection_QueryInterface,
    HTMLMimeTypesCollection_AddRef,
    HTMLMimeTypesCollection_Release,
    HTMLMimeTypesCollection_GetTypeInfoCount,
    HTMLMimeTypesCollection_GetTypeInfo,
    HTMLMimeTypesCollection_GetIDsOfNames,
    HTMLMimeTypesCollection_Invoke,
    HTMLMimeTypesCollection_get_length
};

static const tid_t HTMLMimeTypesCollection_iface_tids[] = {
    IHTMLMimeTypesCollection_tid,
    0
};
static dispex_static_data_t HTMLMimeTypesCollection_dispex = {
    NULL,
    IHTMLMimeTypesCollection_tid,
    HTMLMimeTypesCollection_iface_tids
};

static HRESULT create_mime_types_collection(OmNavigator *navigator, HTMLMimeTypesCollection **ret)
{
    HTMLMimeTypesCollection *col;

    col = heap_alloc_zero(sizeof(*col));
    if(!col)
        return E_OUTOFMEMORY;

    col->IHTMLMimeTypesCollection_iface.lpVtbl = &HTMLMimeTypesCollectionVtbl;
    col->ref = 1;
    col->navigator = navigator;

    init_dispatch(&col->dispex, (IUnknown*)&col->IHTMLMimeTypesCollection_iface,
                  &HTMLMimeTypesCollection_dispex, dispex_compat_mode(&navigator->dispex));

    *ret = col;
    return S_OK;
}

static inline OmNavigator *impl_from_IOmNavigator(IOmNavigator *iface)
{
    return CONTAINING_RECORD(iface, OmNavigator, IOmNavigator_iface);
}

static HRESULT WINAPI OmNavigator_QueryInterface(IOmNavigator *iface, REFIID riid, void **ppv)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IOmNavigator_iface;
    }else if(IsEqualGUID(&IID_IOmNavigator, riid)) {
        *ppv = &This->IOmNavigator_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        WARN("Unsupported interface %s\n", debugstr_mshtml_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI OmNavigator_AddRef(IOmNavigator *iface)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI OmNavigator_Release(IOmNavigator *iface)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->plugins)
            This->plugins->navigator = NULL;
        if(This->mime_types)
            This->mime_types->navigator = NULL;
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI OmNavigator_GetTypeInfoCount(IOmNavigator *iface, UINT *pctinfo)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_GetTypeInfo(IOmNavigator *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI OmNavigator_GetIDsOfNames(IOmNavigator *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI OmNavigator_Invoke(IOmNavigator *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI OmNavigator_get_appCodeName(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = SysAllocString(L"Mozilla");
    return S_OK;
}

static HRESULT WINAPI OmNavigator_get_appName(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = SysAllocString(dispex_compat_mode(&This->dispex) == COMPAT_MODE_IE11
                        ? L"Netscape" : L"Microsoft Internet Explorer");
    if(!*p)
        return E_OUTOFMEMORY;

    return S_OK;
}

static unsigned int get_ua_version(OmNavigator *navigator)
{
    switch(dispex_compat_mode(&navigator->dispex)) {
    case COMPAT_MODE_QUIRKS:
    case COMPAT_MODE_IE5:
    case COMPAT_MODE_IE7:
        return 7;
    case COMPAT_MODE_IE8:
        return 8;
    case COMPAT_MODE_IE9:
        return 9;
    case COMPAT_MODE_IE10:
        return 10;
    case COMPAT_MODE_IE11:
        return 11;
    }
    assert(0);
    return 0;
}

static HRESULT WINAPI OmNavigator_get_appVersion(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    char user_agent[512];
    DWORD size;
    HRESULT hres;
    const unsigned skip_prefix = 8; /* strlen("Mozilla/") */

    TRACE("(%p)->(%p)\n", This, p);

    size = sizeof(user_agent);
    hres = ObtainUserAgentString(get_ua_version(This), user_agent, &size);
    if(FAILED(hres))
        return hres;

    if(size <= skip_prefix) {
        *p = NULL;
        return S_OK;
    }

    size = MultiByteToWideChar(CP_ACP, 0, user_agent + skip_prefix, -1, NULL, 0);
    *p = SysAllocStringLen(NULL, size-1);
    if(!*p)
        return E_OUTOFMEMORY;

    MultiByteToWideChar(CP_ACP, 0, user_agent + skip_prefix, -1, *p, size);
    return S_OK;
}

static HRESULT WINAPI OmNavigator_get_userAgent(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);
    char user_agent[512];
    DWORD size;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    size = sizeof(user_agent);
    hres = ObtainUserAgentString(get_ua_version(This), user_agent, &size);
    if(FAILED(hres))
        return hres;

    size = MultiByteToWideChar(CP_ACP, 0, user_agent, -1, NULL, 0);
    *p = SysAllocStringLen(NULL, size-1);
    if(!*p)
        return E_OUTOFMEMORY;

    MultiByteToWideChar(CP_ACP, 0, user_agent, -1, *p, size);
    return S_OK;
}

static HRESULT WINAPI OmNavigator_javaEnabled(IOmNavigator *iface, VARIANT_BOOL *enabled)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    FIXME("(%p)->(%p) semi-stub\n", This, enabled);

    *enabled = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI OmNavigator_taintEnabled(IOmNavigator *iface, VARIANT_BOOL *enabled)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);
    FIXME("(%p)->(%p)\n", This, enabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_get_mimeTypes(IOmNavigator *iface, IHTMLMimeTypesCollection **p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->mime_types) {
        HRESULT hres;

        hres = create_mime_types_collection(This, &This->mime_types);
        if(FAILED(hres))
            return hres;
    }else {
        IHTMLMimeTypesCollection_AddRef(&This->mime_types->IHTMLMimeTypesCollection_iface);
    }

    *p = &This->mime_types->IHTMLMimeTypesCollection_iface;
    return S_OK;
}

static HRESULT WINAPI OmNavigator_get_plugins(IOmNavigator *iface, IHTMLPluginsCollection **p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->plugins) {
        HRESULT hres;

        hres = create_plugins_collection(This, &This->plugins);
        if(FAILED(hres))
            return hres;
    }else {
        IHTMLPluginsCollection_AddRef(&This->plugins->IHTMLPluginsCollection_iface);
    }

    *p = &This->plugins->IHTMLPluginsCollection_iface;
    return S_OK;
}

static HRESULT WINAPI OmNavigator_get_cookieEnabled(IOmNavigator *iface, VARIANT_BOOL *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    WARN("(%p)->(%p) semi-stub\n", This, p);

    *p = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI OmNavigator_get_opsProfile(IOmNavigator *iface, IHTMLOpsProfile **p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_toString(IOmNavigator *iface, BSTR *String)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    TRACE("(%p)->(%p)\n", This, String);

    if(!String)
        return E_INVALIDARG;

    *String = SysAllocString(dispex_compat_mode(&This->dispex) < COMPAT_MODE_IE9
                             ? L"[object]" : L"[object Navigator]");
    return *String ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI OmNavigator_get_cpuClass(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    TRACE("(%p)->(%p)\n", This, p);

#ifdef _WIN64
    *p = SysAllocString(L"x64");
#else
    *p = SysAllocString(L"x86");
#endif
    return *p ? S_OK : E_OUTOFMEMORY;
}

static HRESULT get_language_string(LCID lcid, BSTR *p)
{
    BSTR ret;
    int len;

    len = LCIDToLocaleName(lcid, NULL, 0, 0);
    if(!len) {
        WARN("LCIDToLocaleName failed: %u\n", GetLastError());
        return E_FAIL;
    }

    ret = SysAllocStringLen(NULL, len-1);
    if(!ret)
        return E_OUTOFMEMORY;

    len = LCIDToLocaleName(lcid, ret, len, 0);
    if(!len) {
        WARN("LCIDToLocaleName failed: %u\n", GetLastError());
        SysFreeString(ret);
        return E_FAIL;
    }

    *p = ret;
    return S_OK;
}

static HRESULT WINAPI OmNavigator_get_systemLanguage(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_language_string(LOCALE_SYSTEM_DEFAULT, p);
}

static HRESULT WINAPI OmNavigator_get_browserLanguage(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_language_string(GetUserDefaultUILanguage(), p);
}

static HRESULT WINAPI OmNavigator_get_userLanguage(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_language_string(LOCALE_USER_DEFAULT, p);
}

static HRESULT WINAPI OmNavigator_get_platform(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    TRACE("(%p)->(%p)\n", This, p);

#ifdef _WIN64
    *p = SysAllocString(L"Win64");
#else
    *p = SysAllocString(L"Win32");
#endif
    return S_OK;
}

static HRESULT WINAPI OmNavigator_get_appMinorVersion(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    TRACE("(%p)->(%p)\n", This, p);

    /* NOTE: MSIE returns "0" or values like ";SP2;". Returning "0" should be enough. */
    *p = SysAllocString(L"0");
    return S_OK;
}

static HRESULT WINAPI OmNavigator_get_connectionSpeed(IOmNavigator *iface, LONG *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_get_onLine(IOmNavigator *iface, VARIANT_BOOL *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    WARN("(%p)->(%p) semi-stub, returning true\n", This, p);

    *p = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI OmNavigator_get_userProfile(IOmNavigator *iface, IHTMLOpsProfile **p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IOmNavigatorVtbl OmNavigatorVtbl = {
    OmNavigator_QueryInterface,
    OmNavigator_AddRef,
    OmNavigator_Release,
    OmNavigator_GetTypeInfoCount,
    OmNavigator_GetTypeInfo,
    OmNavigator_GetIDsOfNames,
    OmNavigator_Invoke,
    OmNavigator_get_appCodeName,
    OmNavigator_get_appName,
    OmNavigator_get_appVersion,
    OmNavigator_get_userAgent,
    OmNavigator_javaEnabled,
    OmNavigator_taintEnabled,
    OmNavigator_get_mimeTypes,
    OmNavigator_get_plugins,
    OmNavigator_get_cookieEnabled,
    OmNavigator_get_opsProfile,
    OmNavigator_toString,
    OmNavigator_get_cpuClass,
    OmNavigator_get_systemLanguage,
    OmNavigator_get_browserLanguage,
    OmNavigator_get_userLanguage,
    OmNavigator_get_platform,
    OmNavigator_get_appMinorVersion,
    OmNavigator_get_connectionSpeed,
    OmNavigator_get_onLine,
    OmNavigator_get_userProfile
};

static const tid_t OmNavigator_iface_tids[] = {
    IOmNavigator_tid,
    0
};
static dispex_static_data_t OmNavigator_dispex = {
    NULL,
    DispHTMLNavigator_tid,
    OmNavigator_iface_tids
};

HRESULT create_navigator(compat_mode_t compat_mode, IOmNavigator **navigator)
{
    OmNavigator *ret;

    ret = heap_alloc_zero(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IOmNavigator_iface.lpVtbl = &OmNavigatorVtbl;
    ret->ref = 1;

    init_dispatch(&ret->dispex, (IUnknown*)&ret->IOmNavigator_iface, &OmNavigator_dispex, compat_mode);

    *navigator = &ret->IOmNavigator_iface;
    return S_OK;
}

typedef struct {
    DispatchEx dispex;
    IHTMLPerformanceTiming IHTMLPerformanceTiming_iface;

    LONG ref;
} HTMLPerformanceTiming;

static inline HTMLPerformanceTiming *impl_from_IHTMLPerformanceTiming(IHTMLPerformanceTiming *iface)
{
    return CONTAINING_RECORD(iface, HTMLPerformanceTiming, IHTMLPerformanceTiming_iface);
}

static HRESULT WINAPI HTMLPerformanceTiming_QueryInterface(IHTMLPerformanceTiming *iface, REFIID riid, void **ppv)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLPerformanceTiming_iface;
    }else if(IsEqualGUID(&IID_IHTMLPerformanceTiming, riid)) {
        *ppv = &This->IHTMLPerformanceTiming_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        WARN("Unsupported interface %s\n", debugstr_mshtml_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLPerformanceTiming_AddRef(IHTMLPerformanceTiming *iface)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLPerformanceTiming_Release(IHTMLPerformanceTiming *iface)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLPerformanceTiming_GetTypeInfoCount(IHTMLPerformanceTiming *iface, UINT *pctinfo)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLPerformanceTiming_GetTypeInfo(IHTMLPerformanceTiming *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLPerformanceTiming_GetIDsOfNames(IHTMLPerformanceTiming *iface, REFIID riid,
                                                          LPOLESTR *rgszNames, UINT cNames,
                                                          LCID lcid, DISPID *rgDispId)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLPerformanceTiming_Invoke(IHTMLPerformanceTiming *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

#define TIMING_FAKE_TIMESTAMP 0xdeadbeef

static HRESULT WINAPI HTMLPerformanceTiming_get_navigationStart(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    FIXME("(%p)->(%p) returning fake value\n", This, p);

    *p = TIMING_FAKE_TIMESTAMP;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_unloadEventStart(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    FIXME("(%p)->(%p) returning fake value\n", This, p);

    *p = TIMING_FAKE_TIMESTAMP;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_unloadEventEnd(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    FIXME("(%p)->(%p) returning fake value\n", This, p);

    *p = TIMING_FAKE_TIMESTAMP;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_redirectStart(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    FIXME("(%p)->(%p) returning fake value\n", This, p);

    *p = TIMING_FAKE_TIMESTAMP;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_redirectEnd(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    FIXME("(%p)->(%p) returning fake value\n", This, p);

    *p = TIMING_FAKE_TIMESTAMP;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_fetchStart(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    FIXME("(%p)->(%p) returning fake value\n", This, p);

    *p = TIMING_FAKE_TIMESTAMP;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_domainLookupStart(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    FIXME("(%p)->(%p) returning fake value\n", This, p);

    *p = TIMING_FAKE_TIMESTAMP;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_domainLookupEnd(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    FIXME("(%p)->(%p) returning fake value\n", This, p);

    *p = TIMING_FAKE_TIMESTAMP;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_connectStart(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    FIXME("(%p)->(%p) returning fake value\n", This, p);

    *p = TIMING_FAKE_TIMESTAMP;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_connectEnd(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    FIXME("(%p)->(%p) returning fake value\n", This, p);

    *p = TIMING_FAKE_TIMESTAMP;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_requestStart(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    FIXME("(%p)->(%p) returning fake value\n", This, p);

    *p = TIMING_FAKE_TIMESTAMP;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_responseStart(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    FIXME("(%p)->(%p) returning fake value\n", This, p);

    *p = TIMING_FAKE_TIMESTAMP;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_responseEnd(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    FIXME("(%p)->(%p) returning fake value\n", This, p);

    *p = TIMING_FAKE_TIMESTAMP;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_domLoading(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    FIXME("(%p)->(%p) returning fake value\n", This, p);

    *p = TIMING_FAKE_TIMESTAMP;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_domInteractive(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    FIXME("(%p)->(%p) returning fake value\n", This, p);

    *p = TIMING_FAKE_TIMESTAMP;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_domContentLoadedEventStart(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    FIXME("(%p)->(%p) returning fake value\n", This, p);

    *p = TIMING_FAKE_TIMESTAMP;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_domContentLoadedEventEnd(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    FIXME("(%p)->(%p) returning fake value\n", This, p);

    *p = TIMING_FAKE_TIMESTAMP;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_domComplete(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    FIXME("(%p)->(%p) returning fake value\n", This, p);

    *p = TIMING_FAKE_TIMESTAMP;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_loadEventStart(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    FIXME("(%p)->(%p) returning fake value\n", This, p);

    *p = TIMING_FAKE_TIMESTAMP;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_loadEventEnd(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    FIXME("(%p)->(%p) returning fake value\n", This, p);

    *p = TIMING_FAKE_TIMESTAMP;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_msFirstPaint(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    FIXME("(%p)->(%p) returning fake value\n", This, p);

    *p = TIMING_FAKE_TIMESTAMP;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_toString(IHTMLPerformanceTiming *iface, BSTR *string)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);
    FIXME("(%p)->(%p)\n", This, string);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLPerformanceTiming_toJSON(IHTMLPerformanceTiming *iface, VARIANT *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLPerformanceTimingVtbl HTMLPerformanceTimingVtbl = {
    HTMLPerformanceTiming_QueryInterface,
    HTMLPerformanceTiming_AddRef,
    HTMLPerformanceTiming_Release,
    HTMLPerformanceTiming_GetTypeInfoCount,
    HTMLPerformanceTiming_GetTypeInfo,
    HTMLPerformanceTiming_GetIDsOfNames,
    HTMLPerformanceTiming_Invoke,
    HTMLPerformanceTiming_get_navigationStart,
    HTMLPerformanceTiming_get_unloadEventStart,
    HTMLPerformanceTiming_get_unloadEventEnd,
    HTMLPerformanceTiming_get_redirectStart,
    HTMLPerformanceTiming_get_redirectEnd,
    HTMLPerformanceTiming_get_fetchStart,
    HTMLPerformanceTiming_get_domainLookupStart,
    HTMLPerformanceTiming_get_domainLookupEnd,
    HTMLPerformanceTiming_get_connectStart,
    HTMLPerformanceTiming_get_connectEnd,
    HTMLPerformanceTiming_get_requestStart,
    HTMLPerformanceTiming_get_responseStart,
    HTMLPerformanceTiming_get_responseEnd,
    HTMLPerformanceTiming_get_domLoading,
    HTMLPerformanceTiming_get_domInteractive,
    HTMLPerformanceTiming_get_domContentLoadedEventStart,
    HTMLPerformanceTiming_get_domContentLoadedEventEnd,
    HTMLPerformanceTiming_get_domComplete,
    HTMLPerformanceTiming_get_loadEventStart,
    HTMLPerformanceTiming_get_loadEventEnd,
    HTMLPerformanceTiming_get_msFirstPaint,
    HTMLPerformanceTiming_toString,
    HTMLPerformanceTiming_toJSON
};

static const tid_t HTMLPerformanceTiming_iface_tids[] = {
    IHTMLPerformanceTiming_tid,
    0
};
static dispex_static_data_t HTMLPerformanceTiming_dispex = {
    NULL,
    IHTMLPerformanceTiming_tid,
    HTMLPerformanceTiming_iface_tids
};

typedef struct {
    DispatchEx dispex;
    IHTMLPerformanceNavigation IHTMLPerformanceNavigation_iface;

    LONG ref;
} HTMLPerformanceNavigation;

static inline HTMLPerformanceNavigation *impl_from_IHTMLPerformanceNavigation(IHTMLPerformanceNavigation *iface)
{
    return CONTAINING_RECORD(iface, HTMLPerformanceNavigation, IHTMLPerformanceNavigation_iface);
}

static HRESULT WINAPI HTMLPerformanceNavigation_QueryInterface(IHTMLPerformanceNavigation *iface, REFIID riid, void **ppv)
{
    HTMLPerformanceNavigation *This = impl_from_IHTMLPerformanceNavigation(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLPerformanceNavigation_iface;
    }else if(IsEqualGUID(&IID_IHTMLPerformanceNavigation, riid)) {
        *ppv = &This->IHTMLPerformanceNavigation_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        WARN("Unsupported interface %s\n", debugstr_mshtml_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLPerformanceNavigation_AddRef(IHTMLPerformanceNavigation *iface)
{
    HTMLPerformanceNavigation *This = impl_from_IHTMLPerformanceNavigation(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLPerformanceNavigation_Release(IHTMLPerformanceNavigation *iface)
{
    HTMLPerformanceNavigation *This = impl_from_IHTMLPerformanceNavigation(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLPerformanceNavigation_GetTypeInfoCount(IHTMLPerformanceNavigation *iface, UINT *pctinfo)
{
    HTMLPerformanceNavigation *This = impl_from_IHTMLPerformanceNavigation(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLPerformanceNavigation_GetTypeInfo(IHTMLPerformanceNavigation *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLPerformanceNavigation *This = impl_from_IHTMLPerformanceNavigation(iface);

    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLPerformanceNavigation_GetIDsOfNames(IHTMLPerformanceNavigation *iface, REFIID riid,
                                                          LPOLESTR *rgszNames, UINT cNames,
                                                          LCID lcid, DISPID *rgDispId)
{
    HTMLPerformanceNavigation *This = impl_from_IHTMLPerformanceNavigation(iface);

    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLPerformanceNavigation_Invoke(IHTMLPerformanceNavigation *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLPerformanceNavigation *This = impl_from_IHTMLPerformanceNavigation(iface);

    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLPerformanceNavigation_get_type(IHTMLPerformanceNavigation *iface, ULONG *p)
{
    HTMLPerformanceNavigation *This = impl_from_IHTMLPerformanceNavigation(iface);

    FIXME("(%p)->(%p) returning TYPE_NAVIGATE\n", This, p);

    *p = 0; /* TYPE_NAVIGATE */
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceNavigation_get_redirectCount(IHTMLPerformanceNavigation *iface, ULONG *p)
{
    HTMLPerformanceNavigation *This = impl_from_IHTMLPerformanceNavigation(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLPerformanceNavigation_toString(IHTMLPerformanceNavigation *iface, BSTR *string)
{
    HTMLPerformanceNavigation *This = impl_from_IHTMLPerformanceNavigation(iface);
    FIXME("(%p)->(%p)\n", This, string);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLPerformanceNavigation_toJSON(IHTMLPerformanceNavigation *iface, VARIANT *p)
{
    HTMLPerformanceNavigation *This = impl_from_IHTMLPerformanceNavigation(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLPerformanceNavigationVtbl HTMLPerformanceNavigationVtbl = {
    HTMLPerformanceNavigation_QueryInterface,
    HTMLPerformanceNavigation_AddRef,
    HTMLPerformanceNavigation_Release,
    HTMLPerformanceNavigation_GetTypeInfoCount,
    HTMLPerformanceNavigation_GetTypeInfo,
    HTMLPerformanceNavigation_GetIDsOfNames,
    HTMLPerformanceNavigation_Invoke,
    HTMLPerformanceNavigation_get_type,
    HTMLPerformanceNavigation_get_redirectCount,
    HTMLPerformanceNavigation_toString,
    HTMLPerformanceNavigation_toJSON
};

static const tid_t HTMLPerformanceNavigation_iface_tids[] = {
    IHTMLPerformanceNavigation_tid,
    0
};
static dispex_static_data_t HTMLPerformanceNavigation_dispex = {
    NULL,
    IHTMLPerformanceNavigation_tid,
    HTMLPerformanceNavigation_iface_tids
};

typedef struct {
    DispatchEx dispex;
    IHTMLPerformance IHTMLPerformance_iface;

    LONG ref;

    IHTMLPerformanceNavigation *navigation;
    IHTMLPerformanceTiming *timing;
} HTMLPerformance;

static inline HTMLPerformance *impl_from_IHTMLPerformance(IHTMLPerformance *iface)
{
    return CONTAINING_RECORD(iface, HTMLPerformance, IHTMLPerformance_iface);
}

static HRESULT WINAPI HTMLPerformance_QueryInterface(IHTMLPerformance *iface, REFIID riid, void **ppv)
{
    HTMLPerformance *This = impl_from_IHTMLPerformance(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLPerformance_iface;
    }else if(IsEqualGUID(&IID_IHTMLPerformance, riid)) {
        *ppv = &This->IHTMLPerformance_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        WARN("Unsupported interface %s\n", debugstr_mshtml_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLPerformance_AddRef(IHTMLPerformance *iface)
{
    HTMLPerformance *This = impl_from_IHTMLPerformance(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLPerformance_Release(IHTMLPerformance *iface)
{
    HTMLPerformance *This = impl_from_IHTMLPerformance(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->timing)
            IHTMLPerformanceTiming_Release(This->timing);
        if(This->navigation)
            IHTMLPerformanceNavigation_Release(This->navigation);
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLPerformance_GetTypeInfoCount(IHTMLPerformance *iface, UINT *pctinfo)
{
    HTMLPerformance *This = impl_from_IHTMLPerformance(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLPerformance_GetTypeInfo(IHTMLPerformance *iface, UINT iTInfo,
                                                  LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLPerformance *This = impl_from_IHTMLPerformance(iface);

    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLPerformance_GetIDsOfNames(IHTMLPerformance *iface, REFIID riid,
                                                    LPOLESTR *rgszNames, UINT cNames,
                                                    LCID lcid, DISPID *rgDispId)
{
    HTMLPerformance *This = impl_from_IHTMLPerformance(iface);

    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLPerformance_Invoke(IHTMLPerformance *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLPerformance *This = impl_from_IHTMLPerformance(iface);

    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLPerformance_get_navigation(IHTMLPerformance *iface,
                                                     IHTMLPerformanceNavigation **p)
{
    HTMLPerformance *This = impl_from_IHTMLPerformance(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->navigation) {
        HTMLPerformanceNavigation *navigation;

        navigation = heap_alloc_zero(sizeof(*navigation));
        if(!navigation)
            return E_OUTOFMEMORY;

        navigation->IHTMLPerformanceNavigation_iface.lpVtbl = &HTMLPerformanceNavigationVtbl;
        navigation->ref = 1;
        init_dispatch(&navigation->dispex, (IUnknown*)&navigation->IHTMLPerformanceNavigation_iface,
                      &HTMLPerformanceNavigation_dispex, dispex_compat_mode(&This->dispex));

        This->navigation = &navigation->IHTMLPerformanceNavigation_iface;
    }

    IHTMLPerformanceNavigation_AddRef(*p = This->navigation);
    return S_OK;
}

static HRESULT WINAPI HTMLPerformance_get_timing(IHTMLPerformance *iface, IHTMLPerformanceTiming **p)
{
    HTMLPerformance *This = impl_from_IHTMLPerformance(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->timing) {
        HTMLPerformanceTiming *timing;

        timing = heap_alloc_zero(sizeof(*timing));
        if(!timing)
            return E_OUTOFMEMORY;

        timing->IHTMLPerformanceTiming_iface.lpVtbl = &HTMLPerformanceTimingVtbl;
        timing->ref = 1;
        init_dispatch(&timing->dispex, (IUnknown*)&timing->IHTMLPerformanceTiming_iface,
                      &HTMLPerformanceTiming_dispex, dispex_compat_mode(&This->dispex));

        This->timing = &timing->IHTMLPerformanceTiming_iface;
    }

    IHTMLPerformanceTiming_AddRef(*p = This->timing);
    return S_OK;
}

static HRESULT WINAPI HTMLPerformance_toString(IHTMLPerformance *iface, BSTR *string)
{
    HTMLPerformance *This = impl_from_IHTMLPerformance(iface);
    FIXME("(%p)->(%p)\n", This, string);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLPerformance_toJSON(IHTMLPerformance *iface, VARIANT *var)
{
    HTMLPerformance *This = impl_from_IHTMLPerformance(iface);
    FIXME("(%p)->(%p)\n", This, var);
    return E_NOTIMPL;
}

static const IHTMLPerformanceVtbl HTMLPerformanceVtbl = {
    HTMLPerformance_QueryInterface,
    HTMLPerformance_AddRef,
    HTMLPerformance_Release,
    HTMLPerformance_GetTypeInfoCount,
    HTMLPerformance_GetTypeInfo,
    HTMLPerformance_GetIDsOfNames,
    HTMLPerformance_Invoke,
    HTMLPerformance_get_navigation,
    HTMLPerformance_get_timing,
    HTMLPerformance_toString,
    HTMLPerformance_toJSON
};

static const tid_t HTMLPerformance_iface_tids[] = {
    IHTMLPerformance_tid,
    0
};
static dispex_static_data_t HTMLPerformance_dispex = {
    NULL,
    IHTMLPerformance_tid,
    HTMLPerformance_iface_tids
};

HRESULT create_performance(compat_mode_t compat_mode, IHTMLPerformance **ret)
{
    HTMLPerformance *performance;

    performance = heap_alloc_zero(sizeof(*performance));
    if(!performance)
        return E_OUTOFMEMORY;

    performance->IHTMLPerformance_iface.lpVtbl = &HTMLPerformanceVtbl;
    performance->ref = 1;

    init_dispatch(&performance->dispex, (IUnknown*)&performance->IHTMLPerformance_iface,
                  &HTMLPerformance_dispex, compat_mode);

    *ret = &performance->IHTMLPerformance_iface;
    return S_OK;
}

typedef struct {
    DispatchEx dispex;
    IHTMLNamespaceCollection IHTMLNamespaceCollection_iface;

    LONG ref;
} HTMLNamespaceCollection;

static inline HTMLNamespaceCollection *impl_from_IHTMLNamespaceCollection(IHTMLNamespaceCollection *iface)
{
    return CONTAINING_RECORD(iface, HTMLNamespaceCollection, IHTMLNamespaceCollection_iface);
}

static HRESULT WINAPI HTMLNamespaceCollection_QueryInterface(IHTMLNamespaceCollection *iface, REFIID riid, void **ppv)
{
    HTMLNamespaceCollection *This = impl_from_IHTMLNamespaceCollection(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLNamespaceCollection_iface;
    }else if(IsEqualGUID(&IID_IHTMLNamespaceCollection, riid)) {
        *ppv = &This->IHTMLNamespaceCollection_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        WARN("Unsupported interface %s\n", debugstr_mshtml_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLNamespaceCollection_AddRef(IHTMLNamespaceCollection *iface)
{
    HTMLNamespaceCollection *This = impl_from_IHTMLNamespaceCollection(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLNamespaceCollection_Release(IHTMLNamespaceCollection *iface)
{
    HTMLNamespaceCollection *This = impl_from_IHTMLNamespaceCollection(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLNamespaceCollection_GetTypeInfoCount(IHTMLNamespaceCollection *iface, UINT *pctinfo)
{
    HTMLNamespaceCollection *This = impl_from_IHTMLNamespaceCollection(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLNamespaceCollection_GetTypeInfo(IHTMLNamespaceCollection *iface, UINT iTInfo,
                                                  LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLNamespaceCollection *This = impl_from_IHTMLNamespaceCollection(iface);

    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLNamespaceCollection_GetIDsOfNames(IHTMLNamespaceCollection *iface, REFIID riid,
                                                    LPOLESTR *rgszNames, UINT cNames,
                                                    LCID lcid, DISPID *rgDispId)
{
    HTMLNamespaceCollection *This = impl_from_IHTMLNamespaceCollection(iface);

    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLNamespaceCollection_Invoke(IHTMLNamespaceCollection *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLNamespaceCollection *This = impl_from_IHTMLNamespaceCollection(iface);

    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLNamespaceCollection_get_length(IHTMLNamespaceCollection *iface, LONG *p)
{
    HTMLNamespaceCollection *This = impl_from_IHTMLNamespaceCollection(iface);
    FIXME("(%p)->(%p) returning 0\n", This, p);
    *p = 0;
    return S_OK;
}

static HRESULT WINAPI HTMLNamespaceCollection_item(IHTMLNamespaceCollection *iface, VARIANT index, IDispatch **p)
{
    HTMLNamespaceCollection *This = impl_from_IHTMLNamespaceCollection(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_variant(&index), p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLNamespaceCollection_add(IHTMLNamespaceCollection *iface, BSTR namespace, BSTR urn,
                                                  VARIANT implementation_url, IDispatch **p)
{
    HTMLNamespaceCollection *This = impl_from_IHTMLNamespaceCollection(iface);
    FIXME("(%p)->(%s %s %s %p)\n", This, debugstr_w(namespace), debugstr_w(urn), debugstr_variant(&implementation_url), p);
    return E_NOTIMPL;
}

static const IHTMLNamespaceCollectionVtbl HTMLNamespaceCollectionVtbl = {
    HTMLNamespaceCollection_QueryInterface,
    HTMLNamespaceCollection_AddRef,
    HTMLNamespaceCollection_Release,
    HTMLNamespaceCollection_GetTypeInfoCount,
    HTMLNamespaceCollection_GetTypeInfo,
    HTMLNamespaceCollection_GetIDsOfNames,
    HTMLNamespaceCollection_Invoke,
    HTMLNamespaceCollection_get_length,
    HTMLNamespaceCollection_item,
    HTMLNamespaceCollection_add
};

static const tid_t HTMLNamespaceCollection_iface_tids[] = {
    IHTMLNamespaceCollection_tid,
    0
};
static dispex_static_data_t HTMLNamespaceCollection_dispex = {
    NULL,
    DispHTMLNamespaceCollection_tid,
    HTMLNamespaceCollection_iface_tids
};

HRESULT create_namespace_collection(compat_mode_t compat_mode, IHTMLNamespaceCollection **ret)
{
    HTMLNamespaceCollection *namespaces;

    if (!(namespaces = heap_alloc_zero(sizeof(*namespaces))))
        return E_OUTOFMEMORY;

    namespaces->IHTMLNamespaceCollection_iface.lpVtbl = &HTMLNamespaceCollectionVtbl;
    namespaces->ref = 1;
    init_dispatch(&namespaces->dispex, (IUnknown*)&namespaces->IHTMLNamespaceCollection_iface,
                  &HTMLNamespaceCollection_dispex, compat_mode);
    *ret = &namespaces->IHTMLNamespaceCollection_iface;
    return S_OK;
}

struct console {
    DispatchEx dispex;
    IWineMSHTMLConsole IWineMSHTMLConsole_iface;
    LONG ref;
};

static inline struct console *impl_from_IWineMSHTMLConsole(IWineMSHTMLConsole *iface)
{
    return CONTAINING_RECORD(iface, struct console, IWineMSHTMLConsole_iface);
}

static HRESULT WINAPI console_QueryInterface(IWineMSHTMLConsole *iface, REFIID riid, void **ppv)
{
    struct console *console = impl_from_IWineMSHTMLConsole(iface);

    TRACE("(%p)->(%s %p)\n", console, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &console->IWineMSHTMLConsole_iface;
    }else if(IsEqualGUID(&IID_IWineMSHTMLConsole, riid)) {
        *ppv = &console->IWineMSHTMLConsole_iface;
    }else if(dispex_query_interface(&console->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        WARN("(%p)->(%s %p)\n", console, debugstr_mshtml_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI console_AddRef(IWineMSHTMLConsole *iface)
{
    struct console *console = impl_from_IWineMSHTMLConsole(iface);
    LONG ref = InterlockedIncrement(&console->ref);

    TRACE("(%p) ref=%d\n", console, ref);

    return ref;
}

static ULONG WINAPI console_Release(IWineMSHTMLConsole *iface)
{
    struct console *console = impl_from_IWineMSHTMLConsole(iface);
    LONG ref = InterlockedDecrement(&console->ref);

    TRACE("(%p) ref=%d\n", console, ref);

    if(!ref) {
        release_dispex(&console->dispex);
        heap_free(console);
    }

    return ref;
}

static HRESULT WINAPI console_GetTypeInfoCount(IWineMSHTMLConsole *iface, UINT *pctinfo)
{
    struct console *console = impl_from_IWineMSHTMLConsole(iface);
    FIXME("(%p)->(%p)\n", console, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI console_GetTypeInfo(IWineMSHTMLConsole *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    struct console *console = impl_from_IWineMSHTMLConsole(iface);

    return IDispatchEx_GetTypeInfo(&console->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI console_GetIDsOfNames(IWineMSHTMLConsole *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    struct console *console = impl_from_IWineMSHTMLConsole(iface);

    return IDispatchEx_GetIDsOfNames(&console->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI console_Invoke(IWineMSHTMLConsole *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    struct console *console = impl_from_IWineMSHTMLConsole(iface);

    return IDispatchEx_Invoke(&console->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI console_assert(IWineMSHTMLConsole *iface, VARIANT_BOOL *assertion, VARIANT *vararg_start)
{
    FIXME("iface %p, assertion %p, vararg_start %p stub.\n", iface, assertion, vararg_start);

    return S_OK;
}

static HRESULT WINAPI console_clear(IWineMSHTMLConsole *iface)
{
    FIXME("iface %p stub.\n", iface);

    return S_OK;
}

static HRESULT WINAPI console_count(IWineMSHTMLConsole *iface, VARIANT *label)
{
    FIXME("iface %p, label %p stub.\n", iface, label);

    return S_OK;
}

static HRESULT WINAPI console_debug(IWineMSHTMLConsole *iface, VARIANT *vararg_start)
{
    FIXME("iface %p, vararg_start %p stub.\n", iface, vararg_start);

    return S_OK;
}

static HRESULT WINAPI console_dir(IWineMSHTMLConsole *iface, VARIANT *object)
{
    FIXME("iface %p, object %p stub.\n", iface, object);

    return S_OK;
}

static HRESULT WINAPI console_dirxml(IWineMSHTMLConsole *iface, VARIANT *object)
{
    FIXME("iface %p, object %p stub.\n", iface, object);

    return S_OK;
}

static HRESULT WINAPI console_error(IWineMSHTMLConsole *iface, VARIANT *vararg_start)
{
    FIXME("iface %p, vararg_start %p stub.\n", iface, vararg_start);

    return S_OK;
}

static HRESULT WINAPI console_group(IWineMSHTMLConsole *iface, VARIANT *label)
{
    FIXME("iface %p, label %p stub.\n", iface, label);

    return S_OK;
}

static HRESULT WINAPI console_group_collapsed(IWineMSHTMLConsole *iface, VARIANT *label)
{
    FIXME("iface %p, label %p stub.\n", iface, label);

    return S_OK;
}

static HRESULT WINAPI console_group_end(IWineMSHTMLConsole *iface)
{
    FIXME("iface %p, stub.\n", iface);

    return S_OK;
}

static HRESULT WINAPI console_info(IWineMSHTMLConsole *iface, VARIANT *vararg_start)
{
    FIXME("iface %p, vararg_start %p stub.\n", iface, vararg_start);

    return S_OK;
}

static HRESULT WINAPI console_log(IWineMSHTMLConsole *iface, VARIANT *vararg_start)
{
    FIXME("iface %p, vararg_start %p stub.\n", iface, vararg_start);

    return S_OK;
}

static HRESULT WINAPI console_time(IWineMSHTMLConsole *iface, VARIANT *label)
{
    FIXME("iface %p, label %p stub.\n", iface, label);

    return S_OK;
}

static HRESULT WINAPI console_time_end(IWineMSHTMLConsole *iface, VARIANT *label)
{
    FIXME("iface %p, label %p stub.\n", iface, label);

    return S_OK;
}

static HRESULT WINAPI console_trace(IWineMSHTMLConsole *iface, VARIANT *vararg_start)
{
    FIXME("iface %p, vararg_start %p stub.\n", iface, vararg_start);

    return S_OK;
}

static HRESULT WINAPI console_warn(IWineMSHTMLConsole *iface, VARIANT *vararg_start)
{
    FIXME("iface %p, vararg_start %p stub.\n", iface, vararg_start);

    return S_OK;
}

static const IWineMSHTMLConsoleVtbl WineMSHTMLConsoleVtbl = {
    console_QueryInterface,
    console_AddRef,
    console_Release,
    console_GetTypeInfoCount,
    console_GetTypeInfo,
    console_GetIDsOfNames,
    console_Invoke,
    console_assert,
    console_clear,
    console_count,
    console_debug,
    console_dir,
    console_dirxml,
    console_error,
    console_group,
    console_group_collapsed,
    console_group_end,
    console_info,
    console_log,
    console_time,
    console_time_end,
    console_trace,
    console_warn,
};

static const tid_t console_iface_tids[] = {
    IWineMSHTMLConsole_tid,
    0
};
static dispex_static_data_t console_dispex = {
    NULL,
    IWineMSHTMLConsole_tid,
    console_iface_tids
};

void create_console(compat_mode_t compat_mode, IWineMSHTMLConsole **ret)
{
    struct console *obj;

    obj = heap_alloc_zero(sizeof(*obj));
    if(!obj)
    {
        ERR("No memory.\n");
        return;
    }

    obj->IWineMSHTMLConsole_iface.lpVtbl = &WineMSHTMLConsoleVtbl;
    obj->ref = 1;
    init_dispatch(&obj->dispex, (IUnknown*)&obj->IWineMSHTMLConsole_iface, &console_dispex, compat_mode);

    *ret = &obj->IWineMSHTMLConsole_iface;
}
