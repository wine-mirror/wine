 /*
 * Copyright 2011,2012 Jacek Caban for CodeWeavers
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
#include "mshtmdid.h"

#include "wine/debug.h"

#include "mshtml_private.h"
#include "htmlevent.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

struct HTMLTitleElement {
    HTMLElement element;

    IHTMLTitleElement IHTMLTitleElement_iface;
};

static inline HTMLTitleElement *impl_from_IHTMLTitleElement(IHTMLTitleElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLTitleElement, IHTMLTitleElement_iface);
}

static HRESULT WINAPI HTMLTitleElement_QueryInterface(IHTMLTitleElement *iface,
                                                         REFIID riid, void **ppv)
{
    HTMLTitleElement *This = impl_from_IHTMLTitleElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLTitleElement_AddRef(IHTMLTitleElement *iface)
{
    HTMLTitleElement *This = impl_from_IHTMLTitleElement(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLTitleElement_Release(IHTMLTitleElement *iface)
{
    HTMLTitleElement *This = impl_from_IHTMLTitleElement(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLTitleElement_GetTypeInfoCount(IHTMLTitleElement *iface, UINT *pctinfo)
{
    HTMLTitleElement *This = impl_from_IHTMLTitleElement(iface);

    return IDispatchEx_GetTypeInfoCount(&This->element.node.event_target.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLTitleElement_GetTypeInfo(IHTMLTitleElement *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLTitleElement *This = impl_from_IHTMLTitleElement(iface);

    return IDispatchEx_GetTypeInfo(&This->element.node.event_target.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLTitleElement_GetIDsOfNames(IHTMLTitleElement *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLTitleElement *This = impl_from_IHTMLTitleElement(iface);

    return IDispatchEx_GetIDsOfNames(&This->element.node.event_target.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLTitleElement_Invoke(IHTMLTitleElement *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLTitleElement *This = impl_from_IHTMLTitleElement(iface);

    return IDispatchEx_Invoke(&This->element.node.event_target.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLTitleElement_put_text(IHTMLTitleElement *iface, BSTR v)
{
    HTMLTitleElement *This = impl_from_IHTMLTitleElement(iface);
    nsAString text;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&text, v);
    nsres = nsIDOMNode_SetTextContent(This->element.node.nsnode, &text);
    nsAString_Finish(&text);

    return map_nsresult(nsres);
}

static HRESULT WINAPI HTMLTitleElement_get_text(IHTMLTitleElement *iface, BSTR *p)
{
    HTMLTitleElement *This = impl_from_IHTMLTitleElement(iface);
    nsAString text;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_InitDepend(&text, NULL);
    nsres = nsIDOMNode_GetTextContent(This->element.node.nsnode, &text);
    return return_nsstr(nsres, &text, p);
}

static const IHTMLTitleElementVtbl HTMLTitleElementVtbl = {
    HTMLTitleElement_QueryInterface,
    HTMLTitleElement_AddRef,
    HTMLTitleElement_Release,
    HTMLTitleElement_GetTypeInfoCount,
    HTMLTitleElement_GetTypeInfo,
    HTMLTitleElement_GetIDsOfNames,
    HTMLTitleElement_Invoke,
    HTMLTitleElement_put_text,
    HTMLTitleElement_get_text
};

static inline HTMLTitleElement *HTMLTitleElement_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLTitleElement, element.node.event_target.dispex);
}

static void *HTMLTitleElement_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLTitleElement *This = HTMLTitleElement_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLTitleElement, riid))
        return &This->IHTMLTitleElement_iface;

    return HTMLElement_query_interface(&This->element.node.event_target.dispex, riid);
}

static const NodeImplVtbl HTMLTitleElementImplVtbl = {
    .clsid                 = &CLSID_HTMLTitleElement,
    .cpc_entries           = HTMLElement_cpc,
    .clone                 = HTMLElement_clone,
    .get_attr_col          = HTMLElement_get_attr_col
};

static const event_target_vtbl_t HTMLTitleElement_event_target_vtbl = {
    {
        HTMLELEMENT_DISPEX_VTBL_ENTRIES,
        .query_interface= HTMLTitleElement_query_interface,
        .destructor     = HTMLElement_destructor,
        .traverse       = HTMLElement_traverse,
        .unlink         = HTMLElement_unlink
    },
    HTMLELEMENT_EVENT_TARGET_VTBL_ENTRIES,
    .handle_event       = HTMLElement_handle_event
};

static const tid_t HTMLTitleElement_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLTitleElement_tid,
    0
};
static dispex_static_data_t HTMLTitleElement_dispex = {
    "HTMLTitleElement",
    &HTMLTitleElement_event_target_vtbl.dispex_vtbl,
    DispHTMLTitleElement_tid,
    HTMLTitleElement_iface_tids,
    HTMLElement_init_dispex_info
};

HRESULT HTMLTitleElement_Create(HTMLDocumentNode *doc, nsIDOMElement *nselem, HTMLElement **elem)
{
    HTMLTitleElement *ret;

    ret = calloc(1, sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLTitleElement_iface.lpVtbl = &HTMLTitleElementVtbl;
    ret->element.node.vtbl = &HTMLTitleElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLTitleElement_dispex);

    *elem = &ret->element;
    return S_OK;
}

struct HTMLHtmlElement {
    HTMLElement element;

    IHTMLHtmlElement IHTMLHtmlElement_iface;
};

static inline HTMLHtmlElement *impl_from_IHTMLHtmlElement(IHTMLHtmlElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLHtmlElement, IHTMLHtmlElement_iface);
}

static HRESULT WINAPI HTMLHtmlElement_QueryInterface(IHTMLHtmlElement *iface,
                                                         REFIID riid, void **ppv)
{
    HTMLHtmlElement *This = impl_from_IHTMLHtmlElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLHtmlElement_AddRef(IHTMLHtmlElement *iface)
{
    HTMLHtmlElement *This = impl_from_IHTMLHtmlElement(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLHtmlElement_Release(IHTMLHtmlElement *iface)
{
    HTMLHtmlElement *This = impl_from_IHTMLHtmlElement(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLHtmlElement_GetTypeInfoCount(IHTMLHtmlElement *iface, UINT *pctinfo)
{
    HTMLHtmlElement *This = impl_from_IHTMLHtmlElement(iface);

    return IDispatchEx_GetTypeInfoCount(&This->element.node.event_target.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLHtmlElement_GetTypeInfo(IHTMLHtmlElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLHtmlElement *This = impl_from_IHTMLHtmlElement(iface);

    return IDispatchEx_GetTypeInfo(&This->element.node.event_target.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLHtmlElement_GetIDsOfNames(IHTMLHtmlElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLHtmlElement *This = impl_from_IHTMLHtmlElement(iface);

    return IDispatchEx_GetIDsOfNames(&This->element.node.event_target.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLHtmlElement_Invoke(IHTMLHtmlElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLHtmlElement *This = impl_from_IHTMLHtmlElement(iface);

    return IDispatchEx_Invoke(&This->element.node.event_target.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLHtmlElement_put_version(IHTMLHtmlElement *iface, BSTR v)
{
    HTMLHtmlElement *This = impl_from_IHTMLHtmlElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLHtmlElement_get_version(IHTMLHtmlElement *iface, BSTR *p)
{
    HTMLHtmlElement *This = impl_from_IHTMLHtmlElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLHtmlElementVtbl HTMLHtmlElementVtbl = {
    HTMLHtmlElement_QueryInterface,
    HTMLHtmlElement_AddRef,
    HTMLHtmlElement_Release,
    HTMLHtmlElement_GetTypeInfoCount,
    HTMLHtmlElement_GetTypeInfo,
    HTMLHtmlElement_GetIDsOfNames,
    HTMLHtmlElement_Invoke,
    HTMLHtmlElement_put_version,
    HTMLHtmlElement_get_version
};

static inline HTMLHtmlElement *HTMLHtmlElement_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLHtmlElement, element.node.event_target.dispex);
}

static BOOL HTMLHtmlElement_is_settable(HTMLDOMNode *iface, DISPID dispid)
{
    switch(dispid) {
    case DISPID_IHTMLELEMENT_OUTERTEXT:
        return FALSE;
    default:
        return TRUE;
    }
}

static void *HTMLHtmlElement_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLHtmlElement *This = HTMLHtmlElement_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLHtmlElement, riid))
        return &This->IHTMLHtmlElement_iface;

    return HTMLElement_query_interface(&This->element.node.event_target.dispex, riid);
}

static const NodeImplVtbl HTMLHtmlElementImplVtbl = {
    .clsid                 = &CLSID_HTMLHtmlElement,
    .cpc_entries           = HTMLElement_cpc,
    .clone                 = HTMLElement_clone,
    .get_attr_col          = HTMLElement_get_attr_col,
    .is_settable           = HTMLHtmlElement_is_settable
};

static const event_target_vtbl_t HTMLHtmlElement_event_target_vtbl = {
    {
        HTMLELEMENT_DISPEX_VTBL_ENTRIES,
        .query_interface= HTMLHtmlElement_query_interface,
        .destructor     = HTMLElement_destructor,
        .traverse       = HTMLElement_traverse,
        .unlink         = HTMLElement_unlink
    },
    HTMLELEMENT_EVENT_TARGET_VTBL_ENTRIES,
    .handle_event       = HTMLElement_handle_event
};

static const tid_t HTMLHtmlElement_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLHtmlElement_tid,
    0
};
static dispex_static_data_t HTMLHtmlElement_dispex = {
    "HTMLHtmlElement",
    &HTMLHtmlElement_event_target_vtbl.dispex_vtbl,
    DispHTMLHtmlElement_tid,
    HTMLHtmlElement_iface_tids,
    HTMLElement_init_dispex_info
};

HRESULT HTMLHtmlElement_Create(HTMLDocumentNode *doc, nsIDOMElement *nselem, HTMLElement **elem)
{
    HTMLHtmlElement *ret;

    ret = calloc(1, sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLHtmlElement_iface.lpVtbl = &HTMLHtmlElementVtbl;
    ret->element.node.vtbl = &HTMLHtmlElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLHtmlElement_dispex);

    *elem = &ret->element;
    return S_OK;
}

struct HTMLMetaElement {
    HTMLElement element;

    IHTMLMetaElement IHTMLMetaElement_iface;
};

static inline HTMLMetaElement *impl_from_IHTMLMetaElement(IHTMLMetaElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLMetaElement, IHTMLMetaElement_iface);
}

static HRESULT WINAPI HTMLMetaElement_QueryInterface(IHTMLMetaElement *iface, REFIID riid, void **ppv)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLMetaElement_AddRef(IHTMLMetaElement *iface)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLMetaElement_Release(IHTMLMetaElement *iface)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLMetaElement_GetTypeInfoCount(IHTMLMetaElement *iface, UINT *pctinfo)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);
    return IDispatchEx_GetTypeInfoCount(&This->element.node.event_target.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLMetaElement_GetTypeInfo(IHTMLMetaElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);
    return IDispatchEx_GetTypeInfo(&This->element.node.event_target.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLMetaElement_GetIDsOfNames(IHTMLMetaElement *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);
    return IDispatchEx_GetIDsOfNames(&This->element.node.event_target.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLMetaElement_Invoke(IHTMLMetaElement *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
        UINT *puArgErr)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);
    return IDispatchEx_Invoke(&This->element.node.event_target.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLMetaElement_put_httpEquiv(IHTMLMetaElement *iface, BSTR v)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLMetaElement_get_httpEquiv(IHTMLMetaElement *iface, BSTR *p)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return elem_string_attr_getter(&This->element, L"http-equiv", TRUE, p);
}

static HRESULT WINAPI HTMLMetaElement_put_content(IHTMLMetaElement *iface, BSTR v)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLMetaElement_get_content(IHTMLMetaElement *iface, BSTR *p)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return elem_string_attr_getter(&This->element, L"content", TRUE, p);
}

static HRESULT WINAPI HTMLMetaElement_put_name(IHTMLMetaElement *iface, BSTR v)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLMetaElement_get_name(IHTMLMetaElement *iface, BSTR *p)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return elem_string_attr_getter(&This->element, L"name", TRUE, p);
}

static HRESULT WINAPI HTMLMetaElement_put_url(IHTMLMetaElement *iface, BSTR v)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLMetaElement_get_url(IHTMLMetaElement *iface, BSTR *p)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLMetaElement_put_charset(IHTMLMetaElement *iface, BSTR v)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return elem_string_attr_setter(&This->element, L"charset", v);
}

static HRESULT WINAPI HTMLMetaElement_get_charset(IHTMLMetaElement *iface, BSTR *p)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return elem_string_attr_getter(&This->element, L"charset", TRUE, p);
}

static const IHTMLMetaElementVtbl HTMLMetaElementVtbl = {
    HTMLMetaElement_QueryInterface,
    HTMLMetaElement_AddRef,
    HTMLMetaElement_Release,
    HTMLMetaElement_GetTypeInfoCount,
    HTMLMetaElement_GetTypeInfo,
    HTMLMetaElement_GetIDsOfNames,
    HTMLMetaElement_Invoke,
    HTMLMetaElement_put_httpEquiv,
    HTMLMetaElement_get_httpEquiv,
    HTMLMetaElement_put_content,
    HTMLMetaElement_get_content,
    HTMLMetaElement_put_name,
    HTMLMetaElement_get_name,
    HTMLMetaElement_put_url,
    HTMLMetaElement_get_url,
    HTMLMetaElement_put_charset,
    HTMLMetaElement_get_charset
};

static inline HTMLMetaElement *HTMLMetaElement_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLMetaElement, element.node.event_target.dispex);
}

static void *HTMLMetaElement_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLMetaElement *This = HTMLMetaElement_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLMetaElement, riid))
        return &This->IHTMLMetaElement_iface;

    return HTMLElement_query_interface(&This->element.node.event_target.dispex, riid);
}

static const NodeImplVtbl HTMLMetaElementImplVtbl = {
    .clsid                 = &CLSID_HTMLMetaElement,
    .cpc_entries           = HTMLElement_cpc,
    .clone                 = HTMLElement_clone,
    .get_attr_col          = HTMLElement_get_attr_col
};

static const event_target_vtbl_t HTMLMetaElement_event_target_vtbl = {
    {
        HTMLELEMENT_DISPEX_VTBL_ENTRIES,
        .query_interface= HTMLMetaElement_query_interface,
        .destructor     = HTMLElement_destructor,
        .traverse       = HTMLElement_traverse,
        .unlink         = HTMLElement_unlink
    },
    HTMLELEMENT_EVENT_TARGET_VTBL_ENTRIES,
    .handle_event       = HTMLElement_handle_event
};

static const tid_t HTMLMetaElement_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLMetaElement_tid,
    0
};

static dispex_static_data_t HTMLMetaElement_dispex = {
    "HTMLMetaElement",
    &HTMLMetaElement_event_target_vtbl.dispex_vtbl,
    DispHTMLMetaElement_tid,
    HTMLMetaElement_iface_tids,
    HTMLElement_init_dispex_info
};

HRESULT HTMLMetaElement_Create(HTMLDocumentNode *doc, nsIDOMElement *nselem, HTMLElement **elem)
{
    HTMLMetaElement *ret;

    ret = calloc(1, sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLMetaElement_iface.lpVtbl = &HTMLMetaElementVtbl;
    ret->element.node.vtbl = &HTMLMetaElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLMetaElement_dispex);

    *elem = &ret->element;
    return S_OK;
}

struct HTMLHeadElement {
    HTMLElement element;

    IHTMLHeadElement IHTMLHeadElement_iface;
};

static inline HTMLHeadElement *impl_from_IHTMLHeadElement(IHTMLHeadElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLHeadElement, IHTMLHeadElement_iface);
}

static HRESULT WINAPI HTMLHeadElement_QueryInterface(IHTMLHeadElement *iface,
                                                         REFIID riid, void **ppv)
{
    HTMLHeadElement *This = impl_from_IHTMLHeadElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLHeadElement_AddRef(IHTMLHeadElement *iface)
{
    HTMLHeadElement *This = impl_from_IHTMLHeadElement(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLHeadElement_Release(IHTMLHeadElement *iface)
{
    HTMLHeadElement *This = impl_from_IHTMLHeadElement(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLHeadElement_GetTypeInfoCount(IHTMLHeadElement *iface, UINT *pctinfo)
{
    HTMLHeadElement *This = impl_from_IHTMLHeadElement(iface);

    return IDispatchEx_GetTypeInfoCount(&This->element.node.event_target.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLHeadElement_GetTypeInfo(IHTMLHeadElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLHeadElement *This = impl_from_IHTMLHeadElement(iface);

    return IDispatchEx_GetTypeInfo(&This->element.node.event_target.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLHeadElement_GetIDsOfNames(IHTMLHeadElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLHeadElement *This = impl_from_IHTMLHeadElement(iface);

    return IDispatchEx_GetIDsOfNames(&This->element.node.event_target.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLHeadElement_Invoke(IHTMLHeadElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLHeadElement *This = impl_from_IHTMLHeadElement(iface);

    return IDispatchEx_Invoke(&This->element.node.event_target.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLHeadElement_put_profile(IHTMLHeadElement *iface, BSTR v)
{
    HTMLHeadElement *This = impl_from_IHTMLHeadElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLHeadElement_get_profile(IHTMLHeadElement *iface, BSTR *p)
{
    HTMLHeadElement *This = impl_from_IHTMLHeadElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLHeadElementVtbl HTMLHeadElementVtbl = {
    HTMLHeadElement_QueryInterface,
    HTMLHeadElement_AddRef,
    HTMLHeadElement_Release,
    HTMLHeadElement_GetTypeInfoCount,
    HTMLHeadElement_GetTypeInfo,
    HTMLHeadElement_GetIDsOfNames,
    HTMLHeadElement_Invoke,
    HTMLHeadElement_put_profile,
    HTMLHeadElement_get_profile
};

static inline HTMLHeadElement *impl_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLHeadElement, element.node.event_target.dispex);
}

static void *HTMLHeadElement_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLHeadElement *This = impl_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLHeadElement, riid))
        return &This->IHTMLHeadElement_iface;
    if(IsEqualGUID(&DIID_DispHTMLHeadElement, riid))
        return &This->IHTMLHeadElement_iface;

    return HTMLElement_query_interface(&This->element.node.event_target.dispex, riid);
}

static const NodeImplVtbl HTMLHeadElementImplVtbl = {
    .clsid                 = &CLSID_HTMLHeadElement,
    .cpc_entries           = HTMLElement_cpc,
    .clone                 = HTMLElement_clone,
    .get_attr_col          = HTMLElement_get_attr_col
};

static const event_target_vtbl_t HTMLHeadElement_event_target_vtbl = {
    {
        HTMLELEMENT_DISPEX_VTBL_ENTRIES,
        .query_interface= HTMLHeadElement_query_interface,
        .destructor     = HTMLElement_destructor,
        .traverse       = HTMLElement_traverse,
        .unlink         = HTMLElement_unlink
    },
    HTMLELEMENT_EVENT_TARGET_VTBL_ENTRIES,
    .handle_event       = HTMLElement_handle_event
};

static const tid_t HTMLHeadElement_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLHeadElement_tid,
    0
};
static dispex_static_data_t HTMLHeadElement_dispex = {
    "HTMLHeadElement",
    &HTMLHeadElement_event_target_vtbl.dispex_vtbl,
    DispHTMLHeadElement_tid,
    HTMLHeadElement_iface_tids,
    HTMLElement_init_dispex_info
};

HRESULT HTMLHeadElement_Create(HTMLDocumentNode *doc, nsIDOMElement *nselem, HTMLElement **elem)
{
    HTMLHeadElement *ret;

    ret = calloc(1, sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLHeadElement_iface.lpVtbl = &HTMLHeadElementVtbl;
    ret->element.node.vtbl = &HTMLHeadElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLHeadElement_dispex);

    *elem = &ret->element;
    return S_OK;
}
