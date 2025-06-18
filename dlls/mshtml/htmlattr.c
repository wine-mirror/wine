/*
 * Copyright 2011 Jacek Caban for CodeWeavers
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

#include "mshtml_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

static inline HTMLDOMAttribute *impl_from_IHTMLDOMAttribute(IHTMLDOMAttribute *iface)
{
    return CONTAINING_RECORD(iface, HTMLDOMAttribute, IHTMLDOMAttribute_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLDOMAttribute, IHTMLDOMAttribute, impl_from_IHTMLDOMAttribute(iface)->dispex)

static HRESULT WINAPI HTMLDOMAttribute_get_nodeName(IHTMLDOMAttribute *iface, BSTR *p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->elem) {
        if(!This->name) {
            FIXME("No name available\n");
            return E_FAIL;
        }

        *p = SysAllocString(This->name);
        return *p ? S_OK : E_OUTOFMEMORY;
    }

    return dispex_prop_name(&This->elem->node.event_target.dispex, This->dispid, p);
}

static HRESULT WINAPI HTMLDOMAttribute_put_nodeValue(IHTMLDOMAttribute *iface, VARIANT v)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute(iface);
    EXCEPINFO ei;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    if(!This->elem)
        return VariantCopy(&This->value, &v);

    memset(&ei, 0, sizeof(ei));
    return dispex_prop_put(&This->elem->node.event_target.dispex, This->dispid, LOCALE_SYSTEM_DEFAULT, &v, &ei, NULL);
}

static HRESULT WINAPI HTMLDOMAttribute_get_nodeValue(IHTMLDOMAttribute *iface, VARIANT *p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->elem)
        return VariantCopy(p, &This->value);

    return get_elem_attr_value_by_dispid(This->elem, This->dispid, p);
}

static HRESULT WINAPI HTMLDOMAttribute_get_specified(IHTMLDOMAttribute *iface, VARIANT_BOOL *p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute(iface);
    nsIDOMAttr *nsattr;
    nsAString nsname;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->elem || !This->elem->dom_element) {
        FIXME("NULL This->elem\n");
        return E_UNEXPECTED;
    }

    if(get_dispid_type(This->dispid) != DISPEXPROP_BUILTIN) {
        *p = VARIANT_TRUE;
        return S_OK;
    }

    /* FIXME: This is not exactly right, we have some attributes that don't map directly to Gecko attributes. */
    nsAString_InitDepend(&nsname, dispex_builtin_prop_name(&This->elem->node.event_target.dispex, This->dispid));
    nsres = nsIDOMElement_GetAttributeNode(This->elem->dom_element, &nsname, &nsattr);
    nsAString_Finish(&nsname);
    if(NS_FAILED(nsres))
        return E_FAIL;

    /* If the Gecko attribute node can be found, we know that the attribute is specified.
       There is no point in calling GetSpecified */
    if(nsattr) {
        nsIDOMAttr_Release(nsattr);
        *p = VARIANT_TRUE;
    }else {
        *p = VARIANT_FALSE;
    }
    return S_OK;
}

static const IHTMLDOMAttributeVtbl HTMLDOMAttributeVtbl = {
    HTMLDOMAttribute_QueryInterface,
    HTMLDOMAttribute_AddRef,
    HTMLDOMAttribute_Release,
    HTMLDOMAttribute_GetTypeInfoCount,
    HTMLDOMAttribute_GetTypeInfo,
    HTMLDOMAttribute_GetIDsOfNames,
    HTMLDOMAttribute_Invoke,
    HTMLDOMAttribute_get_nodeName,
    HTMLDOMAttribute_put_nodeValue,
    HTMLDOMAttribute_get_nodeValue,
    HTMLDOMAttribute_get_specified
};

static inline HTMLDOMAttribute *impl_from_IHTMLDOMAttribute2(IHTMLDOMAttribute2 *iface)
{
    return CONTAINING_RECORD(iface, HTMLDOMAttribute, IHTMLDOMAttribute2_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLDOMAttribute2, IHTMLDOMAttribute2, impl_from_IHTMLDOMAttribute2(iface)->dispex)

static HRESULT WINAPI HTMLDOMAttribute2_get_name(IHTMLDOMAttribute2 *iface, BSTR *p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLDOMAttribute_get_nodeName(&This->IHTMLDOMAttribute_iface, p);
}

static HRESULT WINAPI HTMLDOMAttribute2_put_value(IHTMLDOMAttribute2 *iface, BSTR v)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);
    VARIANT var;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = v;
    return IHTMLDOMAttribute_put_nodeValue(&This->IHTMLDOMAttribute_iface, var);
}

static HRESULT WINAPI HTMLDOMAttribute2_get_value(IHTMLDOMAttribute2 *iface, BSTR *p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);
    VARIANT val;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    V_VT(&val) = VT_EMPTY;
    if(This->elem)
        hres = get_elem_attr_value_by_dispid(This->elem, This->dispid, &val);
    else
        hres = VariantCopy(&val, &This->value);
    if(SUCCEEDED(hres))
        hres = attr_value_to_string(&val);
    if(FAILED(hres))
        return hres;

    assert(V_VT(&val) == VT_BSTR);
    *p = V_BSTR(&val);
    if(!*p && !(*p = SysAllocStringLen(NULL, 0)))
        return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI HTMLDOMAttribute2_get_expando(IHTMLDOMAttribute2 *iface, VARIANT_BOOL *p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = variant_bool(This->elem && get_dispid_type(This->dispid) != DISPEXPROP_BUILTIN);
    return S_OK;
}

static HRESULT WINAPI HTMLDOMAttribute2_get_nodeType(IHTMLDOMAttribute2 *iface, LONG *p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = 2;
    return S_OK;
}

static HRESULT WINAPI HTMLDOMAttribute2_get_parentNode(IHTMLDOMAttribute2 *iface, IHTMLDOMNode **p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = NULL;
    return S_OK;
}

static HRESULT WINAPI HTMLDOMAttribute2_get_childNodes(IHTMLDOMAttribute2 *iface, IDispatch **p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = NULL;
    return S_OK;
}

static HRESULT WINAPI HTMLDOMAttribute2_get_firstChild(IHTMLDOMAttribute2 *iface, IHTMLDOMNode **p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = NULL;
    return S_OK;
}

static HRESULT WINAPI HTMLDOMAttribute2_get_lastChild(IHTMLDOMAttribute2 *iface, IHTMLDOMNode **p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = NULL;
    return S_OK;
}

static HRESULT WINAPI HTMLDOMAttribute2_get_previousSibling(IHTMLDOMAttribute2 *iface, IHTMLDOMNode **p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = NULL;
    return S_OK;
}

static HRESULT WINAPI HTMLDOMAttribute2_get_nextSibling(IHTMLDOMAttribute2 *iface, IHTMLDOMNode **p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = NULL;
    return S_OK;
}

static HRESULT WINAPI HTMLDOMAttribute2_get_attributes(IHTMLDOMAttribute2 *iface, IDispatch **p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = NULL;
    return S_OK;
}

static HRESULT WINAPI HTMLDOMAttribute2_get_ownerDocument(IHTMLDOMAttribute2 *iface, IDispatch **p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = (IDispatch*)&This->doc->IHTMLDocument2_iface;
    IDispatch_AddRef(*p);
    return S_OK;
}

static HRESULT WINAPI HTMLDOMAttribute2_insertBefore(IHTMLDOMAttribute2 *iface, IHTMLDOMNode *newChild,
        VARIANT refChild, IHTMLDOMNode **node)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);

    TRACE("(%p)->(%p %s %p)\n", This, newChild, debugstr_variant(&refChild), node);

    /* mostly a stub, doesn't really insert anything on native either */
    *node = NULL;
    return S_OK;
}

static HRESULT WINAPI HTMLDOMAttribute2_replaceChild(IHTMLDOMAttribute2 *iface, IHTMLDOMNode *newChild,
        IHTMLDOMNode *oldChild, IHTMLDOMNode **node)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);

    TRACE("(%p)->(%p %p %p)\n", This, newChild, oldChild, node);

    /* mostly a stub, doesn't really replace anything on native either */
    *node = NULL;
    return S_OK;
}

static HRESULT WINAPI HTMLDOMAttribute2_removeChild(IHTMLDOMAttribute2 *iface, IHTMLDOMNode *oldChild,
        IHTMLDOMNode **node)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);

    TRACE("(%p)->(%p %p)\n", This, oldChild, node);

    /* mostly a stub, doesn't really remove anything on native either */
    *node = NULL;
    return S_OK;
}

static HRESULT WINAPI HTMLDOMAttribute2_appendChild(IHTMLDOMAttribute2 *iface, IHTMLDOMNode *newChild,
        IHTMLDOMNode **node)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);

    TRACE("(%p)->(%p %p)\n", This, newChild, node);

    /* mostly a stub, doesn't really append anything on native either */
    *node = NULL;
    return S_OK;
}

static HRESULT WINAPI HTMLDOMAttribute2_hasChildNodes(IHTMLDOMAttribute2 *iface, VARIANT_BOOL *fChildren)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);

    TRACE("(%p)->(%p)\n", This, fChildren);

    *fChildren = VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI HTMLDOMAttribute2_cloneNode(IHTMLDOMAttribute2 *iface, VARIANT_BOOL fDeep,
        IHTMLDOMAttribute **clonedNode)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);
    HTMLDOMAttribute *new_attr;
    HRESULT hres;

    TRACE("(%p)->(%x %p)\n", This, fDeep, clonedNode);

    hres = HTMLDOMAttribute_Create(This->name, NULL, 0, This->doc, &new_attr);
    if(FAILED(hres))
        return hres;

    if(This->elem)
        hres = get_elem_attr_value_by_dispid(This->elem, This->dispid, &new_attr->value);
    else
        hres = VariantCopy(&new_attr->value, &This->value);
    if(FAILED(hres)) {
        IHTMLDOMAttribute_Release(&new_attr->IHTMLDOMAttribute_iface);
        return hres;
    }

    *clonedNode = &new_attr->IHTMLDOMAttribute_iface;
    return hres;
}

static const IHTMLDOMAttribute2Vtbl HTMLDOMAttribute2Vtbl = {
    HTMLDOMAttribute2_QueryInterface,
    HTMLDOMAttribute2_AddRef,
    HTMLDOMAttribute2_Release,
    HTMLDOMAttribute2_GetTypeInfoCount,
    HTMLDOMAttribute2_GetTypeInfo,
    HTMLDOMAttribute2_GetIDsOfNames,
    HTMLDOMAttribute2_Invoke,
    HTMLDOMAttribute2_get_name,
    HTMLDOMAttribute2_put_value,
    HTMLDOMAttribute2_get_value,
    HTMLDOMAttribute2_get_expando,
    HTMLDOMAttribute2_get_nodeType,
    HTMLDOMAttribute2_get_parentNode,
    HTMLDOMAttribute2_get_childNodes,
    HTMLDOMAttribute2_get_firstChild,
    HTMLDOMAttribute2_get_lastChild,
    HTMLDOMAttribute2_get_previousSibling,
    HTMLDOMAttribute2_get_nextSibling,
    HTMLDOMAttribute2_get_attributes,
    HTMLDOMAttribute2_get_ownerDocument,
    HTMLDOMAttribute2_insertBefore,
    HTMLDOMAttribute2_replaceChild,
    HTMLDOMAttribute2_removeChild,
    HTMLDOMAttribute2_appendChild,
    HTMLDOMAttribute2_hasChildNodes,
    HTMLDOMAttribute2_cloneNode
};

static inline HTMLDOMAttribute *impl_from_IHTMLDOMAttribute3(IHTMLDOMAttribute3 *iface)
{
    return CONTAINING_RECORD(iface, HTMLDOMAttribute, IHTMLDOMAttribute3_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLDOMAttribute3, IHTMLDOMAttribute3, impl_from_IHTMLDOMAttribute3(iface)->dispex)

static HRESULT WINAPI HTMLDOMAttribute3_put_nodeValue(IHTMLDOMAttribute3 *iface, VARIANT v)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute3(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return HTMLDOMAttribute_put_nodeValue(&This->IHTMLDOMAttribute_iface, v);
}

static HRESULT WINAPI HTMLDOMAttribute3_get_nodeValue(IHTMLDOMAttribute3 *iface, VARIANT *p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute3(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return HTMLDOMAttribute_get_nodeValue(&This->IHTMLDOMAttribute_iface, p);
}

static HRESULT WINAPI HTMLDOMAttribute3_put_value(IHTMLDOMAttribute3 *iface, BSTR v)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute3(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return HTMLDOMAttribute2_put_value(&This->IHTMLDOMAttribute2_iface, v);
}

static HRESULT WINAPI HTMLDOMAttribute3_get_value(IHTMLDOMAttribute3 *iface, BSTR *p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute3(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return HTMLDOMAttribute2_get_value(&This->IHTMLDOMAttribute2_iface, p);
}

static HRESULT WINAPI HTMLDOMAttribute3_get_specified(IHTMLDOMAttribute3 *iface, VARIANT_BOOL *p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute3(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return HTMLDOMAttribute_get_specified(&This->IHTMLDOMAttribute_iface, p);
}

static HRESULT WINAPI HTMLDOMAttribute3_get_ownerElement(IHTMLDOMAttribute3 *iface, IHTMLElement2 **p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute3(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->elem) {
        *p = NULL;
        return S_OK;
    }

    *p = &This->elem->IHTMLElement2_iface;
    IHTMLElement2_AddRef(*p);
    return S_OK;
}

static const IHTMLDOMAttribute3Vtbl HTMLDOMAttribute3Vtbl = {
    HTMLDOMAttribute3_QueryInterface,
    HTMLDOMAttribute3_AddRef,
    HTMLDOMAttribute3_Release,
    HTMLDOMAttribute3_GetTypeInfoCount,
    HTMLDOMAttribute3_GetTypeInfo,
    HTMLDOMAttribute3_GetIDsOfNames,
    HTMLDOMAttribute3_Invoke,
    HTMLDOMAttribute3_put_nodeValue,
    HTMLDOMAttribute3_get_nodeValue,
    HTMLDOMAttribute3_put_value,
    HTMLDOMAttribute3_get_value,
    HTMLDOMAttribute3_get_specified,
    HTMLDOMAttribute3_get_ownerElement
};

static inline HTMLDOMAttribute *impl_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLDOMAttribute, dispex);
}

static void *HTMLDOMAttribute_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLDOMAttribute *This = impl_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLDOMAttribute, riid))
        return &This->IHTMLDOMAttribute_iface;
    if(IsEqualGUID(&IID_IHTMLDOMAttribute2, riid))
        return &This->IHTMLDOMAttribute2_iface;
    if(IsEqualGUID(&IID_IHTMLDOMAttribute3, riid))
        return &This->IHTMLDOMAttribute3_iface;

    return NULL;
}

static void HTMLDOMAttribute_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLDOMAttribute *This = impl_from_DispatchEx(dispex);

    if(This->doc)
        note_cc_edge((nsISupports*)&This->doc->node.IHTMLDOMNode_iface, "doc", cb);
    if(This->elem)
        note_cc_edge((nsISupports*)&This->elem->node.IHTMLDOMNode_iface, "elem", cb);
    traverse_variant(&This->value, "value", cb);
}

static void HTMLDOMAttribute_unlink(DispatchEx *dispex)
{
    HTMLDOMAttribute *This = impl_from_DispatchEx(dispex);

    if(This->doc) {
        HTMLDocumentNode *doc = This->doc;
        This->doc = NULL;
        IHTMLDOMNode_Release(&doc->node.IHTMLDOMNode_iface);
    }
    if(This->elem) {
        HTMLElement *elem = This->elem;
        This->elem = NULL;
        IHTMLDOMNode_Release(&elem->node.IHTMLDOMNode_iface);
    }
    unlink_variant(&This->value);
}

static void HTMLDOMAttribute_destructor(DispatchEx *dispex)
{
    HTMLDOMAttribute *This = impl_from_DispatchEx(dispex);
    VariantClear(&This->value);
    free(This->name);
    free(This);
}

static const dispex_static_data_vtbl_t HTMLDOMAttribute_dispex_vtbl = {
    .query_interface  = HTMLDOMAttribute_query_interface,
    .destructor       = HTMLDOMAttribute_destructor,
    .traverse         = HTMLDOMAttribute_traverse,
    .unlink           = HTMLDOMAttribute_unlink
};

static void HTMLDOMAttribute_init_dispex_info(dispex_data_t *info, compat_mode_t mode)
{
    if(mode >= COMPAT_MODE_IE8)
        dispex_info_add_interface(info, IHTMLDOMAttribute3_tid, NULL);
}

static const tid_t HTMLDOMAttribute_iface_tids[] = {
    IHTMLDOMAttribute_tid,
    IHTMLDOMAttribute2_tid,
    0
};
dispex_static_data_t Attr_dispex = {
    .id           = OBJID_Attr,
    .prototype_id = OBJID_Node,
    .vtbl         = &HTMLDOMAttribute_dispex_vtbl,
    .disp_tid     = DispHTMLDOMAttribute_tid,
    .iface_tids   = HTMLDOMAttribute_iface_tids,
    .init_info    = HTMLDOMAttribute_init_dispex_info,
};

HTMLDOMAttribute *unsafe_impl_from_IHTMLDOMAttribute(IHTMLDOMAttribute *iface)
{
    return iface->lpVtbl == &HTMLDOMAttributeVtbl ? impl_from_IHTMLDOMAttribute(iface) : NULL;
}

HRESULT HTMLDOMAttribute_Create(const WCHAR *name, HTMLElement *elem, DISPID dispid,
                                HTMLDocumentNode *doc, HTMLDOMAttribute **attr)
{
    HTMLAttributeCollection *col;
    HTMLDOMAttribute *ret;
    HRESULT hres;

    ret = calloc(1, sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLDOMAttribute_iface.lpVtbl = &HTMLDOMAttributeVtbl;
    ret->IHTMLDOMAttribute2_iface.lpVtbl = &HTMLDOMAttribute2Vtbl;
    ret->IHTMLDOMAttribute3_iface.lpVtbl = &HTMLDOMAttribute3Vtbl;
    ret->dispid = dispid;
    ret->elem = elem;

    init_dispatch(&ret->dispex, &Attr_dispex, doc->script_global,
                  dispex_compat_mode(&doc->script_global->event_target.dispex));

    /* For attributes attached to an element, (elem,dispid) pair should be valid used for its operation. */
    if(elem) {
        IHTMLDOMNode_AddRef(&elem->node.IHTMLDOMNode_iface);

        hres = HTMLElement_get_attr_col(&elem->node, &col);
        if(FAILED(hres)) {
            IHTMLDOMAttribute_Release(&ret->IHTMLDOMAttribute_iface);
            return hres;
        }
        IHTMLAttributeCollection_Release(&col->IHTMLAttributeCollection_iface);

        list_add_tail(&elem->attrs->attrs, &ret->entry);
    }

    /* For detached attributes we may still do most operations if we have its name available. */
    if(name) {
        ret->name = wcsdup(name);
        if(!ret->name) {
            IHTMLDOMAttribute_Release(&ret->IHTMLDOMAttribute_iface);
            return E_OUTOFMEMORY;
        }
    }

    ret->doc = doc;
    IHTMLDOMNode_AddRef(&doc->node.IHTMLDOMNode_iface);

    *attr = ret;
    return S_OK;
}
