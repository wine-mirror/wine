/*
 * Copyright 2006,2007 Jacek Caban for CodeWeavers
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

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

struct HTMLOptionElement {
    HTMLElement element;

    IHTMLOptionElement IHTMLOptionElement_iface;

    nsIDOMHTMLOptionElement *nsoption;
};

static inline HTMLOptionElement *impl_from_IHTMLOptionElement(IHTMLOptionElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLOptionElement, IHTMLOptionElement_iface);
}

static HRESULT WINAPI HTMLOptionElement_QueryInterface(IHTMLOptionElement *iface,
        REFIID riid, void **ppv)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLOptionElement_AddRef(IHTMLOptionElement *iface)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLOptionElement_Release(IHTMLOptionElement *iface)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLOptionElement_GetTypeInfoCount(IHTMLOptionElement *iface, UINT *pctinfo)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);
    return IDispatchEx_GetTypeInfoCount(&This->element.node.event_target.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLOptionElement_GetTypeInfo(IHTMLOptionElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);
    return IDispatchEx_GetTypeInfo(&This->element.node.event_target.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLOptionElement_GetIDsOfNames(IHTMLOptionElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);
    return IDispatchEx_GetIDsOfNames(&This->element.node.event_target.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLOptionElement_Invoke(IHTMLOptionElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);
    return IDispatchEx_Invoke(&This->element.node.event_target.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLOptionElement_put_selected(IHTMLOptionElement *iface, VARIANT_BOOL v)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%x)\n", This, v);

    nsres = nsIDOMHTMLOptionElement_SetSelected(This->nsoption, v != VARIANT_FALSE);
    if(NS_FAILED(nsres)) {
        ERR("SetSelected failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLOptionElement_get_selected(IHTMLOptionElement *iface, VARIANT_BOOL *p)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);
    cpp_bool selected;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLOptionElement_GetSelected(This->nsoption, &selected);
    if(NS_FAILED(nsres)) {
        ERR("GetSelected failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = variant_bool(selected);
    return S_OK;
}

static HRESULT WINAPI HTMLOptionElement_put_value(IHTMLOptionElement *iface, BSTR v)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);
    nsAString value_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&value_str, v);
    nsres = nsIDOMHTMLOptionElement_SetValue(This->nsoption, &value_str);
    nsAString_Finish(&value_str);
    if(NS_FAILED(nsres))
        ERR("SetValue failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLOptionElement_get_value(IHTMLOptionElement *iface, BSTR *p)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);
    nsAString value_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&value_str, NULL);
    nsres = nsIDOMHTMLOptionElement_GetValue(This->nsoption, &value_str);
    return return_nsstr(nsres, &value_str, p);
}

static HRESULT WINAPI HTMLOptionElement_put_defaultSelected(IHTMLOptionElement *iface, VARIANT_BOOL v)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);
    cpp_bool val, selected;
    nsresult nsres;

    TRACE("(%p)->(%x)\n", This, v);

    val = (v == VARIANT_TRUE);

    nsres = nsIDOMHTMLOptionElement_GetSelected(This->nsoption, &selected);
    if(NS_FAILED(nsres)) {
        ERR("GetSelected failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMHTMLOptionElement_SetDefaultSelected(This->nsoption, val);
    if(NS_FAILED(nsres)) {
        ERR("SetDefaultSelected failed: %08x\n", nsres);
        return E_FAIL;
    }

    if(val != selected) {
        nsres = nsIDOMHTMLOptionElement_SetSelected(This->nsoption, selected); /* WinAPI will reserve selected property */
        if(NS_FAILED(nsres)) {
            ERR("SetSelected failed: %08x\n", nsres);
            return E_FAIL;
        }
    }

    return S_OK;
}

static HRESULT WINAPI HTMLOptionElement_get_defaultSelected(IHTMLOptionElement *iface, VARIANT_BOOL *p)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);
    cpp_bool val;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);
    if(!p)
        return E_POINTER;
    nsres = nsIDOMHTMLOptionElement_GetDefaultSelected(This->nsoption, &val);
    if(NS_FAILED(nsres)) {
        ERR("GetDefaultSelected failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = variant_bool(val);
    return S_OK;
}

static HRESULT WINAPI HTMLOptionElement_put_index(IHTMLOptionElement *iface, LONG v)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLOptionElement_get_index(IHTMLOptionElement *iface, LONG *p)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);
    LONG val;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_INVALIDARG;

    nsres = nsIDOMHTMLOptionElement_GetIndex(This->nsoption, &val);
    if(NS_FAILED(nsres)) {
        ERR("GetIndex failed: %08x\n", nsres);
        return E_FAIL;
    }
    *p = val;
    return S_OK;
}

static HRESULT WINAPI HTMLOptionElement_put_text(IHTMLOptionElement *iface, BSTR v)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);
    nsIDOMText *text_node;
    nsAString text_str;
    nsIDOMNode *tmp;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->element.node.doc->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    while(1) {
        nsIDOMNode *child;

        nsres = nsIDOMElement_GetFirstChild(This->element.dom_element, &child);
        if(NS_FAILED(nsres) || !child)
            break;

        nsres = nsIDOMElement_RemoveChild(This->element.dom_element, child, &tmp);
        nsIDOMNode_Release(child);
        if(NS_SUCCEEDED(nsres)) {
            nsIDOMNode_Release(tmp);
        }else {
            ERR("RemoveChild failed: %08x\n", nsres);
            break;
        }
    }

    nsAString_InitDepend(&text_str, v);
    nsres = nsIDOMHTMLDocument_CreateTextNode(This->element.node.doc->nsdoc, &text_str, &text_node);
    nsAString_Finish(&text_str);
    if(NS_FAILED(nsres)) {
        ERR("CreateTextNode failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMElement_AppendChild(This->element.dom_element, (nsIDOMNode*)text_node, &tmp);
    if(NS_SUCCEEDED(nsres))
        nsIDOMNode_Release(tmp);
    else
        ERR("AppendChild failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLOptionElement_get_text(IHTMLOptionElement *iface, BSTR *p)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);
    nsAString text_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&text_str, NULL);
    nsres = nsIDOMHTMLOptionElement_GetText(This->nsoption, &text_str);
    return return_nsstr(nsres, &text_str, p);
}

static HRESULT WINAPI HTMLOptionElement_get_form(IHTMLOptionElement *iface, IHTMLFormElement **p)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);
    nsIDOMHTMLFormElement *nsform;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    nsres = nsIDOMHTMLOptionElement_GetForm(This->nsoption, &nsform);
    return return_nsform(nsres, nsform, p);
}

static const IHTMLOptionElementVtbl HTMLOptionElementVtbl = {
    HTMLOptionElement_QueryInterface,
    HTMLOptionElement_AddRef,
    HTMLOptionElement_Release,
    HTMLOptionElement_GetTypeInfoCount,
    HTMLOptionElement_GetTypeInfo,
    HTMLOptionElement_GetIDsOfNames,
    HTMLOptionElement_Invoke,
    HTMLOptionElement_put_selected,
    HTMLOptionElement_get_selected,
    HTMLOptionElement_put_value,
    HTMLOptionElement_get_value,
    HTMLOptionElement_put_defaultSelected,
    HTMLOptionElement_get_defaultSelected,
    HTMLOptionElement_put_index,
    HTMLOptionElement_get_index,
    HTMLOptionElement_put_text,
    HTMLOptionElement_get_text,
    HTMLOptionElement_get_form
};

static inline HTMLOptionElement *HTMLOptionElement_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLOptionElement, element.node);
}

static HRESULT HTMLOptionElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLOptionElement *This = HTMLOptionElement_from_HTMLDOMNode(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IHTMLOptionElement_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IHTMLOptionElement_iface;
    }else if(IsEqualGUID(&IID_IHTMLOptionElement, riid)) {
        TRACE("(%p)->(IID_IHTMLOptionElement %p)\n", This, ppv);
        *ppv = &This->IHTMLOptionElement_iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    return HTMLElement_QI(&This->element.node, riid, ppv);
}

static void HTMLOptionElement_traverse(HTMLDOMNode *iface, nsCycleCollectionTraversalCallback *cb)
{
    HTMLOptionElement *This = HTMLOptionElement_from_HTMLDOMNode(iface);

    if(This->nsoption)
        note_cc_edge((nsISupports*)This->nsoption, "This->nsoption", cb);
}

static void HTMLOptionElement_unlink(HTMLDOMNode *iface)
{
    HTMLOptionElement *This = HTMLOptionElement_from_HTMLDOMNode(iface);

    if(This->nsoption) {
        nsIDOMHTMLOptionElement *nsoption = This->nsoption;

        This->nsoption = NULL;
        nsIDOMHTMLOptionElement_Release(nsoption);
    }
}

static const NodeImplVtbl HTMLOptionElementImplVtbl = {
    &CLSID_HTMLOptionElement,
    HTMLOptionElement_QI,
    HTMLElement_destructor,
    HTMLElement_cpc,
    HTMLElement_clone,
    HTMLElement_handle_event,
    HTMLElement_get_attr_col,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    HTMLOptionElement_traverse,
    HTMLOptionElement_unlink
};

static const tid_t HTMLOptionElement_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLOptionElement_tid,
    0
};
static dispex_static_data_t HTMLOptionElement_dispex = {
    NULL,
    DispHTMLOptionElement_tid,
    HTMLOptionElement_iface_tids,
    HTMLElement_init_dispex_info
};

HRESULT HTMLOptionElement_Create(HTMLDocumentNode *doc, nsIDOMElement *nselem, HTMLElement **elem)
{
    HTMLOptionElement *ret;
    nsresult nsres;

    ret = heap_alloc_zero(sizeof(HTMLOptionElement));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLOptionElement_iface.lpVtbl = &HTMLOptionElementVtbl;
    ret->element.node.vtbl = &HTMLOptionElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLOptionElement_dispex);

    nsres = nsIDOMElement_QueryInterface(nselem, &IID_nsIDOMHTMLOptionElement, (void**)&ret->nsoption);
    assert(nsres == NS_OK);

    *elem = &ret->element;
    return S_OK;
}

static inline HTMLOptionElementFactory *impl_from_IHTMLOptionElementFactory(IHTMLOptionElementFactory *iface)
{
    return CONTAINING_RECORD(iface, HTMLOptionElementFactory, IHTMLOptionElementFactory_iface);
}

static HRESULT WINAPI HTMLOptionElementFactory_QueryInterface(IHTMLOptionElementFactory *iface,
                                                              REFIID riid, void **ppv)
{
    HTMLOptionElementFactory *This = impl_from_IHTMLOptionElementFactory(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLOptionElementFactory_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        *ppv = &This->IHTMLOptionElementFactory_iface;
    }else if(IsEqualGUID(&IID_IHTMLOptionElementFactory, riid)) {
        *ppv = &This->IHTMLOptionElementFactory_iface;
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

static ULONG WINAPI HTMLOptionElementFactory_AddRef(IHTMLOptionElementFactory *iface)
{
    HTMLOptionElementFactory *This = impl_from_IHTMLOptionElementFactory(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLOptionElementFactory_Release(IHTMLOptionElementFactory *iface)
{
    HTMLOptionElementFactory *This = impl_from_IHTMLOptionElementFactory(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLOptionElementFactory_GetTypeInfoCount(IHTMLOptionElementFactory *iface, UINT *pctinfo)
{
    HTMLOptionElementFactory *This = impl_from_IHTMLOptionElementFactory(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLOptionElementFactory_GetTypeInfo(IHTMLOptionElementFactory *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLOptionElementFactory *This = impl_from_IHTMLOptionElementFactory(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLOptionElementFactory_GetIDsOfNames(IHTMLOptionElementFactory *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLOptionElementFactory *This = impl_from_IHTMLOptionElementFactory(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLOptionElementFactory_Invoke(IHTMLOptionElementFactory *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLOptionElementFactory *This = impl_from_IHTMLOptionElementFactory(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, wFlags, pDispParams,
            pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLOptionElementFactory_create(IHTMLOptionElementFactory *iface,
        VARIANT text, VARIANT value, VARIANT defaultselected, VARIANT selected,
        IHTMLOptionElement **optelem)
{
    HTMLOptionElementFactory *This = impl_from_IHTMLOptionElementFactory(iface);
    nsIDOMElement *nselem;
    HTMLDOMNode *node;
    HRESULT hres;

    TRACE("(%p)->(%s %s %s %s %p)\n", This, debugstr_variant(&text), debugstr_variant(&value),
          debugstr_variant(&defaultselected), debugstr_variant(&selected), optelem);

    if(!This->window || !This->window->doc) {
        WARN("NULL doc\n");
        return E_UNEXPECTED;
    }

    *optelem = NULL;

    hres = create_nselem(This->window->doc, L"OPTION", &nselem);
    if(FAILED(hres))
        return hres;

    hres = get_node((nsIDOMNode*)nselem, TRUE, &node);
    nsIDOMElement_Release(nselem);
    if(FAILED(hres))
        return hres;

    hres = IHTMLDOMNode_QueryInterface(&node->IHTMLDOMNode_iface,
            &IID_IHTMLOptionElement, (void**)optelem);
    node_release(node);

    if(V_VT(&text) == VT_BSTR)
        IHTMLOptionElement_put_text(*optelem, V_BSTR(&text));
    else if(V_VT(&text) != VT_EMPTY)
        FIXME("Unsupported text %s\n", debugstr_variant(&text));

    if(V_VT(&value) == VT_BSTR)
        IHTMLOptionElement_put_value(*optelem, V_BSTR(&value));
    else if(V_VT(&value) != VT_EMPTY)
        FIXME("Unsupported value %s\n", debugstr_variant(&value));

    if(V_VT(&defaultselected) != VT_EMPTY)
        FIXME("Unsupported defaultselected %s\n", debugstr_variant(&defaultselected));
    if(V_VT(&selected) != VT_EMPTY)
        FIXME("Unsupported selected %s\n", debugstr_variant(&selected));

    return S_OK;
}

static const IHTMLOptionElementFactoryVtbl HTMLOptionElementFactoryVtbl = {
    HTMLOptionElementFactory_QueryInterface,
    HTMLOptionElementFactory_AddRef,
    HTMLOptionElementFactory_Release,
    HTMLOptionElementFactory_GetTypeInfoCount,
    HTMLOptionElementFactory_GetTypeInfo,
    HTMLOptionElementFactory_GetIDsOfNames,
    HTMLOptionElementFactory_Invoke,
    HTMLOptionElementFactory_create
};

static const tid_t HTMLOptionElementFactory_iface_tids[] = {
    IHTMLOptionElementFactory_tid,
    0
};

static dispex_static_data_t HTMLOptionElementFactory_dispex = {
    NULL,
    IHTMLOptionElementFactory_tid,
    HTMLOptionElementFactory_iface_tids,
    HTMLElement_init_dispex_info
};

HRESULT HTMLOptionElementFactory_Create(HTMLInnerWindow *window, HTMLOptionElementFactory **ret_ptr)
{
    HTMLOptionElementFactory *ret;

    ret = heap_alloc(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLOptionElementFactory_iface.lpVtbl = &HTMLOptionElementFactoryVtbl;
    ret->ref = 1;
    ret->window = window;

    init_dispatch(&ret->dispex, (IUnknown*)&ret->IHTMLOptionElementFactory_iface,
                  &HTMLOptionElementFactory_dispex, dispex_compat_mode(&window->event_target.dispex));

    *ret_ptr = ret;
    return S_OK;
}

struct HTMLSelectElement {
    HTMLElement element;

    IHTMLSelectElement IHTMLSelectElement_iface;

    nsIDOMHTMLSelectElement *nsselect;
};

static inline HTMLSelectElement *impl_from_IHTMLSelectElement(IHTMLSelectElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLSelectElement, IHTMLSelectElement_iface);
}

static HRESULT htmlselect_item(HTMLSelectElement *This, int i, IDispatch **ret)
{
    nsIDOMHTMLOptionsCollection *nscol;
    nsIDOMNode *nsnode;
    nsresult nsres;
    HRESULT hres;

    nsres = nsIDOMHTMLSelectElement_GetOptions(This->nsselect, &nscol);
    if(NS_FAILED(nsres)) {
        ERR("GetOptions failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMHTMLOptionsCollection_Item(nscol, i, &nsnode);
    nsIDOMHTMLOptionsCollection_Release(nscol);
    if(NS_FAILED(nsres)) {
        ERR("Item failed: %08x\n", nsres);
        return E_FAIL;
    }

    if(nsnode) {
        HTMLDOMNode *node;

        hres = get_node(nsnode, TRUE, &node);
        nsIDOMNode_Release(nsnode);
        if(FAILED(hres))
            return hres;

        *ret = (IDispatch*)&node->IHTMLDOMNode_iface;
    }else {
        *ret = NULL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_QueryInterface(IHTMLSelectElement *iface,
                                                         REFIID riid, void **ppv)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLSelectElement_AddRef(IHTMLSelectElement *iface)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLSelectElement_Release(IHTMLSelectElement *iface)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLSelectElement_GetTypeInfoCount(IHTMLSelectElement *iface, UINT *pctinfo)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);

    return IDispatchEx_GetTypeInfoCount(&This->element.node.event_target.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLSelectElement_GetTypeInfo(IHTMLSelectElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);

    return IDispatchEx_GetTypeInfo(&This->element.node.event_target.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLSelectElement_GetIDsOfNames(IHTMLSelectElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);

    return IDispatchEx_GetIDsOfNames(&This->element.node.event_target.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLSelectElement_Invoke(IHTMLSelectElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);

    return IDispatchEx_Invoke(&This->element.node.event_target.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLSelectElement_put_size(IHTMLSelectElement *iface, LONG v)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%d)\n", This, v);
    if(v < 0)
        return CTL_E_INVALIDPROPERTYVALUE;

    nsres = nsIDOMHTMLSelectElement_SetSize(This->nsselect, v);
    if(NS_FAILED(nsres)) {
        ERR("SetSize failed: %08x\n", nsres);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_size(IHTMLSelectElement *iface, LONG *p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    DWORD val;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);
    if(!p)
        return E_INVALIDARG;

    nsres = nsIDOMHTMLSelectElement_GetSize(This->nsselect, &val);
    if(NS_FAILED(nsres)) {
        ERR("GetSize failed: %08x\n", nsres);
        return E_FAIL;
    }
    *p = val;
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_put_multiple(IHTMLSelectElement *iface, VARIANT_BOOL v)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%x)\n", This, v);

    nsres = nsIDOMHTMLSelectElement_SetMultiple(This->nsselect, !!v);
    assert(nsres == NS_OK);
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_multiple(IHTMLSelectElement *iface, VARIANT_BOOL *p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    cpp_bool val;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLSelectElement_GetMultiple(This->nsselect, &val);
    assert(nsres == NS_OK);

    *p = variant_bool(val);
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_put_name(IHTMLSelectElement *iface, BSTR v)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsAString str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    nsAString_InitDepend(&str, v);
    nsres = nsIDOMHTMLSelectElement_SetName(This->nsselect, &str);
    nsAString_Finish(&str);

    if(NS_FAILED(nsres)) {
        ERR("SetName failed: %08x\n", nsres);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_name(IHTMLSelectElement *iface, BSTR *p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsAString name_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&name_str, NULL);
    nsres = nsIDOMHTMLSelectElement_GetName(This->nsselect, &name_str);

    return return_nsstr(nsres, &name_str, p);
}

static HRESULT WINAPI HTMLSelectElement_get_options(IHTMLSelectElement *iface, IDispatch **p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = (IDispatch*)&This->IHTMLSelectElement_iface;
    IDispatch_AddRef(*p);
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_put_onchange(IHTMLSelectElement *iface, VARIANT v)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);

    TRACE("(%p)->()\n", This);

    return set_node_event(&This->element.node, EVENTID_CHANGE, &v);
}

static HRESULT WINAPI HTMLSelectElement_get_onchange(IHTMLSelectElement *iface, VARIANT *p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLSelectElement_put_selectedIndex(IHTMLSelectElement *iface, LONG v)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%d)\n", This, v);

    nsres = nsIDOMHTMLSelectElement_SetSelectedIndex(This->nsselect, v);
    if(NS_FAILED(nsres))
        ERR("SetSelectedIndex failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_selectedIndex(IHTMLSelectElement *iface, LONG *p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLSelectElement_GetSelectedIndex(This->nsselect, p);
    if(NS_FAILED(nsres)) {
        ERR("GetSelectedIndex failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_type(IHTMLSelectElement *iface, BSTR *p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsAString type_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&type_str, NULL);
    nsres = nsIDOMHTMLSelectElement_GetType(This->nsselect, &type_str);
    return return_nsstr(nsres, &type_str, p);
}

static HRESULT WINAPI HTMLSelectElement_put_value(IHTMLSelectElement *iface, BSTR v)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsAString value_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&value_str, v);
    nsres = nsIDOMHTMLSelectElement_SetValue(This->nsselect, &value_str);
    nsAString_Finish(&value_str);
    if(NS_FAILED(nsres))
        ERR("SetValue failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_value(IHTMLSelectElement *iface, BSTR *p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsAString value_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&value_str, NULL);
    nsres = nsIDOMHTMLSelectElement_GetValue(This->nsselect, &value_str);
    return return_nsstr(nsres, &value_str, p);
}

static HRESULT WINAPI HTMLSelectElement_put_disabled(IHTMLSelectElement *iface, VARIANT_BOOL v)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%x)\n", This, v);

    nsres = nsIDOMHTMLSelectElement_SetDisabled(This->nsselect, v != VARIANT_FALSE);
    if(NS_FAILED(nsres)) {
        ERR("SetDisabled failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_disabled(IHTMLSelectElement *iface, VARIANT_BOOL *p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    cpp_bool disabled = FALSE;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLSelectElement_GetDisabled(This->nsselect, &disabled);
    if(NS_FAILED(nsres)) {
        ERR("GetDisabled failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = variant_bool(disabled);
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_form(IHTMLSelectElement *iface, IHTMLFormElement **p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsIDOMHTMLFormElement *nsform;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    nsres = nsIDOMHTMLSelectElement_GetForm(This->nsselect, &nsform);
    return return_nsform(nsres, nsform, p);
}

static HRESULT WINAPI HTMLSelectElement_add(IHTMLSelectElement *iface, IHTMLElement *element,
                                            VARIANT before)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsIWritableVariant *nsvariant;
    HTMLElement *element_obj;
    nsresult nsres;

    TRACE("(%p)->(%p %s)\n", This, element, debugstr_variant(&before));

    element_obj = unsafe_impl_from_IHTMLElement(element);
    if(!element_obj) {
        FIXME("External IHTMLElement implementation?\n");
        return E_INVALIDARG;
    }

    if(!element_obj->html_element) {
        FIXME("Not HTML element\n");
        return E_NOTIMPL;
    }

    nsvariant = create_nsvariant();
    if(!nsvariant)
        return E_FAIL;

    switch(V_VT(&before)) {
    case VT_EMPTY:
    case VT_ERROR:
        nsres = nsIWritableVariant_SetAsEmpty(nsvariant);
        break;
    case VT_I2:
        nsres = nsIWritableVariant_SetAsInt16(nsvariant, V_I2(&before));
        break;
    default:
        FIXME("unhandled before %s\n", debugstr_variant(&before));
        nsIWritableVariant_Release(nsvariant);
        return E_NOTIMPL;
    }

    if(NS_SUCCEEDED(nsres))
        nsres = nsIDOMHTMLSelectElement_Add(This->nsselect, element_obj->html_element, (nsIVariant*)nsvariant);
    nsIWritableVariant_Release(nsvariant);
    if(NS_FAILED(nsres)) {
        ERR("Add failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_remove(IHTMLSelectElement *iface, LONG index)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsresult nsres;
    TRACE("(%p)->(%d)\n", This, index);
    if(index < 0)
        return E_INVALIDARG;

    nsres = nsIDOMHTMLSelectElement_select_Remove(This->nsselect, index);
    if(NS_FAILED(nsres)) {
        ERR("Remove failed: %08x\n", nsres);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_put_length(IHTMLSelectElement *iface, LONG v)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%d)\n", This, v);

    nsres = nsIDOMHTMLSelectElement_SetLength(This->nsselect, v);
    if(NS_FAILED(nsres))
        ERR("SetLength failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_length(IHTMLSelectElement *iface, LONG *p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    UINT32 length = 0;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLSelectElement_GetLength(This->nsselect, &length);
    if(NS_FAILED(nsres))
        ERR("GetLength failed: %08x\n", nsres);

    *p = length;

    TRACE("ret %d\n", *p);
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get__newEnum(IHTMLSelectElement *iface, IUnknown **p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLSelectElement_item(IHTMLSelectElement *iface, VARIANT name,
                                             VARIANT index, IDispatch **pdisp)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_variant(&name), debugstr_variant(&index), pdisp);

    if(!pdisp)
        return E_POINTER;
    *pdisp = NULL;

    if(V_VT(&name) == VT_I4) {
        if(V_I4(&name) < 0)
            return E_INVALIDARG;
        return htmlselect_item(This, V_I4(&name), pdisp);
    }

    FIXME("Unsupported args\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLSelectElement_tags(IHTMLSelectElement *iface, VARIANT tagName,
                                             IDispatch **pdisp)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    FIXME("(%p)->(v %p)\n", This, pdisp);
    return E_NOTIMPL;
}

static const IHTMLSelectElementVtbl HTMLSelectElementVtbl = {
    HTMLSelectElement_QueryInterface,
    HTMLSelectElement_AddRef,
    HTMLSelectElement_Release,
    HTMLSelectElement_GetTypeInfoCount,
    HTMLSelectElement_GetTypeInfo,
    HTMLSelectElement_GetIDsOfNames,
    HTMLSelectElement_Invoke,
    HTMLSelectElement_put_size,
    HTMLSelectElement_get_size,
    HTMLSelectElement_put_multiple,
    HTMLSelectElement_get_multiple,
    HTMLSelectElement_put_name,
    HTMLSelectElement_get_name,
    HTMLSelectElement_get_options,
    HTMLSelectElement_put_onchange,
    HTMLSelectElement_get_onchange,
    HTMLSelectElement_put_selectedIndex,
    HTMLSelectElement_get_selectedIndex,
    HTMLSelectElement_get_type,
    HTMLSelectElement_put_value,
    HTMLSelectElement_get_value,
    HTMLSelectElement_put_disabled,
    HTMLSelectElement_get_disabled,
    HTMLSelectElement_get_form,
    HTMLSelectElement_add,
    HTMLSelectElement_remove,
    HTMLSelectElement_put_length,
    HTMLSelectElement_get_length,
    HTMLSelectElement_get__newEnum,
    HTMLSelectElement_item,
    HTMLSelectElement_tags
};

static inline HTMLSelectElement *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLSelectElement, element.node);
}

static HRESULT HTMLSelectElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLSelectElement *This = impl_from_HTMLDOMNode(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IHTMLSelectElement_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IHTMLSelectElement_iface;
    }else if(IsEqualGUID(&IID_IHTMLSelectElement, riid)) {
        TRACE("(%p)->(IID_IHTMLSelectElement %p)\n", This, ppv);
        *ppv = &This->IHTMLSelectElement_iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    return HTMLElement_QI(&This->element.node, riid, ppv);
}

static HRESULT HTMLSelectElementImpl_put_disabled(HTMLDOMNode *iface, VARIANT_BOOL v)
{
    HTMLSelectElement *This = impl_from_HTMLDOMNode(iface);
    return IHTMLSelectElement_put_disabled(&This->IHTMLSelectElement_iface, v);
}

static HRESULT HTMLSelectElementImpl_get_disabled(HTMLDOMNode *iface, VARIANT_BOOL *p)
{
    HTMLSelectElement *This = impl_from_HTMLDOMNode(iface);
    return IHTMLSelectElement_get_disabled(&This->IHTMLSelectElement_iface, p);
}

#define DISPID_OPTIONCOL_0 MSHTML_DISPID_CUSTOM_MIN

static HRESULT HTMLSelectElement_get_dispid(HTMLDOMNode *iface, BSTR name, DWORD flags, DISPID *dispid)
{
    const WCHAR *ptr;
    DWORD idx = 0;

    for(ptr = name; *ptr && is_digit(*ptr); ptr++) {
        idx = idx*10 + (*ptr-'0');
        if(idx > MSHTML_CUSTOM_DISPID_CNT) {
            WARN("too big idx\n");
            return DISP_E_UNKNOWNNAME;
        }
    }
    if(*ptr)
        return DISP_E_UNKNOWNNAME;

    *dispid = DISPID_OPTIONCOL_0 + idx;
    return S_OK;
}

static HRESULT HTMLSelectElement_invoke(HTMLDOMNode *iface, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLSelectElement *This = impl_from_HTMLDOMNode(iface);

    TRACE("(%p)->(%x %x %x %p %p %p %p)\n", This, id, lcid, flags, params, res, ei, caller);

    switch(flags) {
    case DISPATCH_PROPERTYGET: {
        IDispatch *ret;
        HRESULT hres;

        hres = htmlselect_item(This, id-DISPID_OPTIONCOL_0, &ret);
        if(FAILED(hres))
            return hres;

        if(ret) {
            V_VT(res) = VT_DISPATCH;
            V_DISPATCH(res) = ret;
        }else {
            V_VT(res) = VT_NULL;
        }
        break;
    }

    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static void HTMLSelectElement_traverse(HTMLDOMNode *iface, nsCycleCollectionTraversalCallback *cb)
{
    HTMLSelectElement *This = impl_from_HTMLDOMNode(iface);

    if(This->nsselect)
        note_cc_edge((nsISupports*)This->nsselect, "This->nsselect", cb);
}

static void HTMLSelectElement_unlink(HTMLDOMNode *iface)
{
    HTMLSelectElement *This = impl_from_HTMLDOMNode(iface);

    if(This->nsselect) {
        nsIDOMHTMLSelectElement *nsselect = This->nsselect;

        This->nsselect = NULL;
        nsIDOMHTMLSelectElement_Release(nsselect);
    }
}

static const NodeImplVtbl HTMLSelectElementImplVtbl = {
    &CLSID_HTMLSelectElement,
    HTMLSelectElement_QI,
    HTMLElement_destructor,
    HTMLElement_cpc,
    HTMLElement_clone,
    HTMLElement_handle_event,
    HTMLElement_get_attr_col,
    NULL,
    HTMLSelectElementImpl_put_disabled,
    HTMLSelectElementImpl_get_disabled,
    NULL,
    NULL,
    HTMLSelectElement_get_dispid,
    HTMLSelectElement_invoke,
    NULL,
    HTMLSelectElement_traverse,
    HTMLSelectElement_unlink
};

static const tid_t HTMLSelectElement_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLSelectElement_tid,
    0
};

static dispex_static_data_t HTMLSelectElement_dispex = {
    NULL,
    DispHTMLSelectElement_tid,
    HTMLSelectElement_tids,
    HTMLElement_init_dispex_info
};

HRESULT HTMLSelectElement_Create(HTMLDocumentNode *doc, nsIDOMElement *nselem, HTMLElement **elem)
{
    HTMLSelectElement *ret;
    nsresult nsres;

    ret = heap_alloc_zero(sizeof(HTMLSelectElement));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLSelectElement_iface.lpVtbl = &HTMLSelectElementVtbl;
    ret->element.node.vtbl = &HTMLSelectElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLSelectElement_dispex);

    nsres = nsIDOMElement_QueryInterface(nselem, &IID_nsIDOMHTMLSelectElement, (void**)&ret->nsselect);
    assert(nsres == NS_OK);

    *elem = &ret->element;
    return S_OK;
}
