/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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
#include "htmlevent.h"
#include "htmlscript.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

static inline HTMLScriptElement *impl_from_IHTMLScriptElement(IHTMLScriptElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLScriptElement, IHTMLScriptElement_iface);
}

static HRESULT WINAPI HTMLScriptElement_QueryInterface(IHTMLScriptElement *iface,
        REFIID riid, void **ppv)
{
    HTMLScriptElement *This = impl_from_IHTMLScriptElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLScriptElement_AddRef(IHTMLScriptElement *iface)
{
    HTMLScriptElement *This = impl_from_IHTMLScriptElement(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLScriptElement_Release(IHTMLScriptElement *iface)
{
    HTMLScriptElement *This = impl_from_IHTMLScriptElement(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLScriptElement_GetTypeInfoCount(IHTMLScriptElement *iface, UINT *pctinfo)
{
    HTMLScriptElement *This = impl_from_IHTMLScriptElement(iface);
    return IDispatchEx_GetTypeInfoCount(&This->element.node.event_target.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLScriptElement_GetTypeInfo(IHTMLScriptElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLScriptElement *This = impl_from_IHTMLScriptElement(iface);
    return IDispatchEx_GetTypeInfo(&This->element.node.event_target.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLScriptElement_GetIDsOfNames(IHTMLScriptElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLScriptElement *This = impl_from_IHTMLScriptElement(iface);
    return IDispatchEx_GetIDsOfNames(&This->element.node.event_target.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLScriptElement_Invoke(IHTMLScriptElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLScriptElement *This = impl_from_IHTMLScriptElement(iface);
    return IDispatchEx_Invoke(&This->element.node.event_target.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLScriptElement_put_src(IHTMLScriptElement *iface, BSTR v)
{
    HTMLScriptElement *This = impl_from_IHTMLScriptElement(iface);
    nsAString src_str;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&src_str, v);
    nsres = nsIDOMHTMLScriptElement_SetSrc(This->nsscript, &src_str);
    nsAString_Finish(&src_str);
    if(NS_FAILED(nsres)) {
        ERR("SetSrc failed: %08x\n", nsres);
        return E_FAIL;
    }

    if(This->parsed) {
        WARN("already parsed\n");
        return S_OK;
    }

    if(This->binding) {
        FIXME("binding in progress\n");
        return E_FAIL;
    }

    nsAString_Init(&src_str, NULL);
    nsres = nsIDOMHTMLScriptElement_GetSrc(This->nsscript, &src_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *src;
        nsAString_GetData(&src_str, &src);
        hres = load_script(This, src, TRUE);
    }else {
        ERR("SetSrc failed: %08x\n", nsres);
        hres = E_FAIL;
    }
    nsAString_Finish(&src_str);
    return hres;
}

static HRESULT WINAPI HTMLScriptElement_get_src(IHTMLScriptElement *iface, BSTR *p)
{
    HTMLScriptElement *This = impl_from_IHTMLScriptElement(iface);
    nsAString src_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&src_str, NULL);
    nsres = nsIDOMHTMLScriptElement_GetSrc(This->nsscript, &src_str);
    return return_nsstr(nsres, &src_str, p);
}

static HRESULT WINAPI HTMLScriptElement_put_htmlFor(IHTMLScriptElement *iface, BSTR v)
{
    HTMLScriptElement *This = impl_from_IHTMLScriptElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScriptElement_get_htmlFor(IHTMLScriptElement *iface, BSTR *p)
{
    HTMLScriptElement *This = impl_from_IHTMLScriptElement(iface);
    nsAString html_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&html_str, NULL);
    nsres = nsIDOMHTMLScriptElement_GetHtmlFor(This->nsscript, &html_str);
    return return_nsstr(nsres, &html_str, p);
}

static HRESULT WINAPI HTMLScriptElement_put_event(IHTMLScriptElement *iface, BSTR v)
{
    HTMLScriptElement *This = impl_from_IHTMLScriptElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScriptElement_get_event(IHTMLScriptElement *iface, BSTR *p)
{
    HTMLScriptElement *This = impl_from_IHTMLScriptElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScriptElement_put_text(IHTMLScriptElement *iface, BSTR v)
{
    HTMLScriptElement *This = impl_from_IHTMLScriptElement(iface);
    HTMLInnerWindow *window;
    nsIDOMNode *parent;
    nsAString text_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->element.node.doc || !This->element.node.doc->window) {
        WARN("no window\n");
        return E_UNEXPECTED;
    }

    window = This->element.node.doc->window;

    nsAString_InitDepend(&text_str, v);
    nsres = nsIDOMHTMLScriptElement_SetText(This->nsscript, &text_str);
    nsAString_Finish(&text_str);
    if(NS_FAILED(nsres)) {
        ERR("SetSrc failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMElement_GetParentNode(This->element.dom_element, &parent);
    if(NS_FAILED(nsres) || !parent) {
        TRACE("No parent, not executing\n");
        This->parse_on_bind = TRUE;
        return S_OK;
    }

    nsIDOMNode_Release(parent);
    doc_insert_script(window, This, FALSE);
    return S_OK;
}

static HRESULT WINAPI HTMLScriptElement_get_text(IHTMLScriptElement *iface, BSTR *p)
{
    HTMLScriptElement *This = impl_from_IHTMLScriptElement(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMHTMLScriptElement_GetText(This->nsscript, &nsstr);
    return return_nsstr(nsres, &nsstr, p);
}

static HRESULT WINAPI HTMLScriptElement_put_defer(IHTMLScriptElement *iface, VARIANT_BOOL v)
{
    HTMLScriptElement *This = impl_from_IHTMLScriptElement(iface);
    HRESULT hr = S_OK;
    nsresult nsres;

    TRACE("(%p)->(%x)\n", This, v);

    nsres = nsIDOMHTMLScriptElement_SetDefer(This->nsscript, v != VARIANT_FALSE);
    if(NS_FAILED(nsres))
    {
        hr = E_FAIL;
    }

    return hr;
}

static HRESULT WINAPI HTMLScriptElement_get_defer(IHTMLScriptElement *iface, VARIANT_BOOL *p)
{
    HTMLScriptElement *This = impl_from_IHTMLScriptElement(iface);
    cpp_bool defer = FALSE;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_INVALIDARG;

    nsres = nsIDOMHTMLScriptElement_GetDefer(This->nsscript, &defer);
    if(NS_FAILED(nsres)) {
        ERR("GetSrc failed: %08x\n", nsres);
    }

    *p = variant_bool(defer);
    return S_OK;
}

static HRESULT WINAPI HTMLScriptElement_get_readyState(IHTMLScriptElement *iface, BSTR *p)
{
    HTMLScriptElement *This = impl_from_IHTMLScriptElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_readystate_string(This->readystate, p);
}

static HRESULT WINAPI HTMLScriptElement_put_onerror(IHTMLScriptElement *iface, VARIANT v)
{
    HTMLScriptElement *This = impl_from_IHTMLScriptElement(iface);

    FIXME("(%p)->(%s) semi-stub\n", This, debugstr_variant(&v));

    return set_node_event(&This->element.node, EVENTID_ERROR, &v);
}

static HRESULT WINAPI HTMLScriptElement_get_onerror(IHTMLScriptElement *iface, VARIANT *p)
{
    HTMLScriptElement *This = impl_from_IHTMLScriptElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->element.node, EVENTID_ERROR, p);
}

static HRESULT WINAPI HTMLScriptElement_put_type(IHTMLScriptElement *iface, BSTR v)
{
    HTMLScriptElement *This = impl_from_IHTMLScriptElement(iface);
    nsAString nstype_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_Init(&nstype_str, v);
    nsres = nsIDOMHTMLScriptElement_SetType(This->nsscript, &nstype_str);
    if (NS_FAILED(nsres))
        ERR("SetType failed: %08x\n", nsres);
    nsAString_Finish (&nstype_str);

    return S_OK;
}

static HRESULT WINAPI HTMLScriptElement_get_type(IHTMLScriptElement *iface, BSTR *p)
{
    HTMLScriptElement *This = impl_from_IHTMLScriptElement(iface);
    nsAString nstype_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nstype_str, NULL);
    nsres = nsIDOMHTMLScriptElement_GetType(This->nsscript, &nstype_str);
    return return_nsstr(nsres, &nstype_str, p);
}

static const IHTMLScriptElementVtbl HTMLScriptElementVtbl = {
    HTMLScriptElement_QueryInterface,
    HTMLScriptElement_AddRef,
    HTMLScriptElement_Release,
    HTMLScriptElement_GetTypeInfoCount,
    HTMLScriptElement_GetTypeInfo,
    HTMLScriptElement_GetIDsOfNames,
    HTMLScriptElement_Invoke,
    HTMLScriptElement_put_src,
    HTMLScriptElement_get_src,
    HTMLScriptElement_put_htmlFor,
    HTMLScriptElement_get_htmlFor,
    HTMLScriptElement_put_event,
    HTMLScriptElement_get_event,
    HTMLScriptElement_put_text,
    HTMLScriptElement_get_text,
    HTMLScriptElement_put_defer,
    HTMLScriptElement_get_defer,
    HTMLScriptElement_get_readyState,
    HTMLScriptElement_put_onerror,
    HTMLScriptElement_get_onerror,
    HTMLScriptElement_put_type,
    HTMLScriptElement_get_type
};

static inline HTMLScriptElement *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLScriptElement, element.node);
}

static HRESULT HTMLScriptElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLScriptElement *This = impl_from_HTMLDOMNode(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IHTMLScriptElement_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IHTMLScriptElement_iface;
    }else if(IsEqualGUID(&IID_IHTMLScriptElement, riid)) {
        TRACE("(%p)->(IID_IHTMLScriptElement %p)\n", This, ppv);
        *ppv = &This->IHTMLScriptElement_iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    return HTMLElement_QI(&This->element.node, riid, ppv);
}

static void HTMLScriptElement_destructor(HTMLDOMNode *iface)
{
    HTMLScriptElement *This = impl_from_HTMLDOMNode(iface);
    heap_free(This->src_text);
    HTMLElement_destructor(&This->element.node);
}

static HRESULT HTMLScriptElement_get_readystate(HTMLDOMNode *iface, BSTR *p)
{
    HTMLScriptElement *This = impl_from_HTMLDOMNode(iface);

    return IHTMLScriptElement_get_readyState(&This->IHTMLScriptElement_iface, p);
}

static HRESULT HTMLScriptElement_bind_to_tree(HTMLDOMNode *iface)
{
    HTMLScriptElement *This = impl_from_HTMLDOMNode(iface);

    TRACE("(%p)\n", This);

    if(!This->parse_on_bind)
        return S_OK;

    if(!This->element.node.doc || !This->element.node.doc->window) {
        ERR("No window\n");
        return E_UNEXPECTED;
    }

    This->parse_on_bind = FALSE;
    doc_insert_script(This->element.node.doc->window, This, FALSE);
    return S_OK;
}

static void HTMLScriptElement_traverse(HTMLDOMNode *iface, nsCycleCollectionTraversalCallback *cb)
{
    HTMLScriptElement *This = impl_from_HTMLDOMNode(iface);

    if(This->nsscript)
        note_cc_edge((nsISupports*)This->nsscript, "This->nsscript", cb);
}

static void HTMLScriptElement_unlink(HTMLDOMNode *iface)
{
    HTMLScriptElement *This = impl_from_HTMLDOMNode(iface);

    if(This->nsscript) {
        nsIDOMHTMLScriptElement *nsscript = This->nsscript;

        This->nsscript = NULL;
        nsIDOMHTMLScriptElement_Release(nsscript);
    }
}

static const NodeImplVtbl HTMLScriptElementImplVtbl = {
    &CLSID_HTMLScriptElement,
    HTMLScriptElement_QI,
    HTMLScriptElement_destructor,
    HTMLElement_cpc,
    HTMLElement_clone,
    HTMLElement_handle_event,
    HTMLElement_get_attr_col,
    NULL,
    NULL,
    NULL,
    NULL,
    HTMLScriptElement_get_readystate,
    NULL,
    NULL,
    HTMLScriptElement_bind_to_tree,
    HTMLScriptElement_traverse,
    HTMLScriptElement_unlink
};

HRESULT script_elem_from_nsscript(nsIDOMHTMLScriptElement *nsscript, HTMLScriptElement **ret)
{
    nsIDOMNode *nsnode;
    HTMLDOMNode *node;
    nsresult nsres;
    HRESULT hres;

    nsres = nsIDOMHTMLScriptElement_QueryInterface(nsscript, &IID_nsIDOMNode, (void**)&nsnode);
    assert(nsres == NS_OK);

    hres = get_node(nsnode, TRUE, &node);
    nsIDOMNode_Release(nsnode);
    if(FAILED(hres))
        return hres;

    assert(node->vtbl == &HTMLScriptElementImplVtbl);
    *ret = impl_from_HTMLDOMNode(node);
    return S_OK;
}

static const tid_t HTMLScriptElement_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLScriptElement_tid,
    0
};

static dispex_static_data_t HTMLScriptElement_dispex = {
    NULL,
    DispHTMLScriptElement_tid,
    HTMLScriptElement_iface_tids,
    HTMLElement_init_dispex_info
};

HRESULT HTMLScriptElement_Create(HTMLDocumentNode *doc, nsIDOMElement *nselem, HTMLElement **elem)
{
    HTMLScriptElement *ret;
    nsresult nsres;

    ret = heap_alloc_zero(sizeof(HTMLScriptElement));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLScriptElement_iface.lpVtbl = &HTMLScriptElementVtbl;
    ret->element.node.vtbl = &HTMLScriptElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLScriptElement_dispex);

    nsres = nsIDOMElement_QueryInterface(nselem, &IID_nsIDOMHTMLScriptElement, (void**)&ret->nsscript);
    assert(nsres == NS_OK);

    *elem = &ret->element;
    return S_OK;
}
