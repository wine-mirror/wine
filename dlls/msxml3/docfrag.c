/*
 *    DOM Document Fragment implementation
 *
 * Copyright 2007 Alistair Leslie-Hughes
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

#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml6.h"

#include "msxml_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

typedef struct _docfrag
{
    DispatchEx dispex;
    IXMLDOMDocumentFragment IXMLDOMDocumentFragment_iface;
    LONG refcount;
    struct domnode *node;
} docfrag;

static const tid_t docfrag_se_tids[] =
{
    IXMLDOMNode_tid,
    IXMLDOMDocumentFragment_tid,
    NULL_tid
};

static inline docfrag *impl_from_IXMLDOMDocumentFragment( IXMLDOMDocumentFragment *iface )
{
    return CONTAINING_RECORD(iface, docfrag, IXMLDOMDocumentFragment_iface);
}

static HRESULT WINAPI docfrag_QueryInterface(IXMLDOMDocumentFragment *iface, REFIID riid, void **obj)
{
    docfrag *docfrag = impl_from_IXMLDOMDocumentFragment(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID( riid, &IID_IXMLDOMDocumentFragment) ||
        IsEqualGUID( riid, &IID_IXMLDOMNode) ||
        IsEqualGUID( riid, &IID_IDispatch) ||
        IsEqualGUID( riid, &IID_IUnknown))
    {
        *obj = iface;
    }
    else if (dispex_query_interface(&docfrag->dispex, riid, obj))
    {
        return *obj ? S_OK : E_NOINTERFACE;
    }
    else if (node_query_interface(docfrag->node, riid, obj))
    {
        return *obj ? S_OK : E_NOINTERFACE;
    }
    else if (IsEqualGUID(riid, &IID_ISupportErrorInfo))
    {
        return node_create_supporterrorinfo(docfrag_se_tids, obj);
    }
    else
    {
        TRACE("Unsupported interface %s\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IXMLDOMDocumentFragment_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI docfrag_AddRef(IXMLDOMDocumentFragment *iface)
{
    docfrag *docfrag = impl_from_IXMLDOMDocumentFragment(iface);
    ULONG refcount = InterlockedIncrement(&docfrag->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI docfrag_Release(IXMLDOMDocumentFragment *iface)
{
    docfrag *docfrag = impl_from_IXMLDOMDocumentFragment(iface);
    ULONG refcount = InterlockedDecrement(&docfrag->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        domnode_release(docfrag->node);
        free(docfrag);
    }

    return refcount;
}

static HRESULT WINAPI docfrag_GetTypeInfoCount(IXMLDOMDocumentFragment *iface, UINT *count)
{
    docfrag *docfrag = impl_from_IXMLDOMDocumentFragment(iface);
    return IDispatchEx_GetTypeInfoCount(&docfrag->dispex.IDispatchEx_iface, count);
}

static HRESULT WINAPI docfrag_GetTypeInfo(IXMLDOMDocumentFragment *iface, UINT index, LCID lcid, ITypeInfo **ti)
{
    docfrag *docfrag = impl_from_IXMLDOMDocumentFragment(iface);
    return IDispatchEx_GetTypeInfo(&docfrag->dispex.IDispatchEx_iface, index, lcid, ti);
}

static HRESULT WINAPI docfrag_GetIDsOfNames(IXMLDOMDocumentFragment *iface, REFIID riid,
    LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    docfrag *docfrag = impl_from_IXMLDOMDocumentFragment(iface);
    return IDispatchEx_GetIDsOfNames(&docfrag->dispex.IDispatchEx_iface,
        riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI docfrag_Invoke(IXMLDOMDocumentFragment *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *ei, UINT *puArgErr)
{
    docfrag *docfrag = impl_from_IXMLDOMDocumentFragment(iface);
    return IDispatchEx_Invoke(&docfrag->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, flags,
            params, result, ei, puArgErr);
}

static HRESULT WINAPI docfrag_get_nodeName(IXMLDOMDocumentFragment *iface, BSTR *p)
{
    TRACE("%p, %p.\n", iface, p);

    return return_bstr(L"#document-fragment", p);
}

static HRESULT WINAPI docfrag_get_nodeValue(IXMLDOMDocumentFragment *iface, VARIANT *value)
{
    TRACE("%p, %p.\n", iface, value);

    return return_null_var(value);
}

static HRESULT WINAPI docfrag_put_nodeValue(IXMLDOMDocumentFragment *iface, VARIANT value)
{
    TRACE("%p, %s.\n", iface, debugstr_variant(&value));

    return E_FAIL;
}

static HRESULT WINAPI docfrag_get_nodeType(IXMLDOMDocumentFragment *iface, DOMNodeType *type)
{
    TRACE("%p, %p.\n", iface, type);

    *type = NODE_DOCUMENT_FRAGMENT;
    return S_OK;
}

static HRESULT WINAPI docfrag_get_parentNode(IXMLDOMDocumentFragment *iface, IXMLDOMNode **parent)
{
    docfrag *docfrag = impl_from_IXMLDOMDocumentFragment(iface);

    TRACE("%p, %p.\n", iface, parent);

    return node_get_parent(docfrag->node, parent);
}

static HRESULT WINAPI docfrag_get_childNodes(IXMLDOMDocumentFragment *iface, IXMLDOMNodeList **list)
{
    docfrag *docfrag = impl_from_IXMLDOMDocumentFragment(iface);

    TRACE("%p, %p.\n", iface, list);

    return node_get_child_nodes(docfrag->node, list);
}

static HRESULT WINAPI docfrag_get_firstChild(IXMLDOMDocumentFragment *iface, IXMLDOMNode **node)
{
    docfrag *docfrag = impl_from_IXMLDOMDocumentFragment(iface);

    TRACE("%p, %p.\n", iface, node);

    return node_get_first_child(docfrag->node, node);
}

static HRESULT WINAPI docfrag_get_lastChild(IXMLDOMDocumentFragment *iface, IXMLDOMNode **node)
{
    docfrag *docfrag = impl_from_IXMLDOMDocumentFragment(iface);

    TRACE("%p, %p.\n", iface, node);

    return node_get_last_child(docfrag->node, node);
}

static HRESULT WINAPI docfrag_get_previousSibling(IXMLDOMDocumentFragment *iface, IXMLDOMNode **node)
{
    TRACE("%p, %p.\n", iface, node);

    return return_null_node(node);
}

static HRESULT WINAPI docfrag_get_nextSibling(IXMLDOMDocumentFragment *iface, IXMLDOMNode **node)
{
    TRACE("%p, %p.\n", iface, node);

    return return_null_node(node);
}

static HRESULT WINAPI docfrag_get_attributes(IXMLDOMDocumentFragment *iface, IXMLDOMNamedNodeMap **map)
{
    TRACE("%p, %p.\n", iface, map);

    return return_null_ptr((void **)map);
}

static HRESULT WINAPI docfrag_insertBefore(IXMLDOMDocumentFragment *iface, IXMLDOMNode* newNode,
        VARIANT refChild, IXMLDOMNode **outOldNode)
{
    docfrag *docfrag = impl_from_IXMLDOMDocumentFragment(iface);

    TRACE("%p, %p, %s, %p.\n", iface, newNode, debugstr_variant(&refChild), outOldNode);

    /* TODO: test */
    return node_insert_before(docfrag->node, newNode, &refChild, outOldNode);
}

static HRESULT WINAPI docfrag_replaceChild(IXMLDOMDocumentFragment *iface, IXMLDOMNode *newNode,
        IXMLDOMNode *oldNode, IXMLDOMNode **outOldNode)
{
    docfrag *docfrag = impl_from_IXMLDOMDocumentFragment(iface);

    TRACE("%p, %p, %p, %p.\n", iface, newNode, oldNode, outOldNode);

    /* TODO: test */
    return node_replace_child(docfrag->node, newNode, oldNode, outOldNode);
}

static HRESULT WINAPI docfrag_removeChild(IXMLDOMDocumentFragment *iface, IXMLDOMNode *child, IXMLDOMNode **oldChild)
{
    docfrag *docfrag = impl_from_IXMLDOMDocumentFragment(iface);

    TRACE("%p, %p, %p.\n", iface, child, oldChild);

    return node_remove_child(docfrag->node, child, oldChild);
}

static HRESULT WINAPI docfrag_appendChild(IXMLDOMDocumentFragment *iface, IXMLDOMNode *child, IXMLDOMNode **outChild)
{
    docfrag *docfrag = impl_from_IXMLDOMDocumentFragment(iface);

    TRACE("%p, %p, %p.\n", iface, child, outChild);

    return node_append_child(docfrag->node, child, outChild);
}

static HRESULT WINAPI docfrag_hasChildNodes(IXMLDOMDocumentFragment *iface, VARIANT_BOOL *ret)
{
    docfrag *docfrag = impl_from_IXMLDOMDocumentFragment(iface);

    TRACE("%p, %p.\n", iface, ret);

    return node_has_childnodes(docfrag->node, ret);
}

static HRESULT WINAPI docfrag_get_ownerDocument(IXMLDOMDocumentFragment *iface, IXMLDOMDocument **doc)
{
    docfrag *docfrag = impl_from_IXMLDOMDocumentFragment(iface);

    TRACE("%p, %p.\n", iface, doc);

    return node_get_owner_document(docfrag->node, doc);
}

static HRESULT WINAPI docfrag_cloneNode(IXMLDOMDocumentFragment *iface, VARIANT_BOOL deep, IXMLDOMNode **node)
{
    docfrag *docfrag = impl_from_IXMLDOMDocumentFragment(iface);

    TRACE("%p, %d, %p.\n", iface, deep, node);

    return node_clone(docfrag->node, deep, node);
}

static HRESULT WINAPI docfrag_get_nodeTypeString(IXMLDOMDocumentFragment *iface, BSTR *p)
{
    TRACE("%p, %p.\n", iface, p);

    return return_bstr(L"documentfragment", p);
}

static HRESULT WINAPI docfrag_get_text(IXMLDOMDocumentFragment *iface, BSTR *p)
{
    docfrag *docfrag = impl_from_IXMLDOMDocumentFragment(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_text(docfrag->node, p);
}

static HRESULT WINAPI docfrag_put_text(IXMLDOMDocumentFragment *iface, BSTR p)
{
    docfrag *docfrag = impl_from_IXMLDOMDocumentFragment(iface);

    TRACE("%p, %s.\n", iface, debugstr_w(p));

    return node_put_data(docfrag->node, p);
}

static HRESULT WINAPI docfrag_get_specified(IXMLDOMDocumentFragment *iface, VARIANT_BOOL *v)
{
    FIXME("%p, %p stub!\n", iface, v);

    *v = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI docfrag_get_definition(IXMLDOMDocumentFragment *iface, IXMLDOMNode **node)
{
    FIXME("%p, %p.\n", iface, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI docfrag_get_nodeTypedValue(IXMLDOMDocumentFragment *iface, VARIANT *v)
{
    TRACE("%p, %p.\n", iface, v);

    return return_null_var(v);
}

static HRESULT WINAPI docfrag_put_nodeTypedValue(IXMLDOMDocumentFragment *iface, VARIANT v)
{
    FIXME("%p, %s.\n", iface, debugstr_variant(&v));

    return E_NOTIMPL;
}

static HRESULT WINAPI docfrag_get_dataType(IXMLDOMDocumentFragment *iface, VARIANT *v)
{
    TRACE("%p, %p.\n", iface, v);

    return return_null_var(v);
}

static HRESULT WINAPI docfrag_put_dataType(IXMLDOMDocumentFragment *iface, BSTR p)
{
    TRACE("%p, %s.\n", iface, debugstr_w(p));

    if (!p)
        return E_INVALIDARG;

    return E_FAIL;
}

static HRESULT WINAPI docfrag_get_xml(IXMLDOMDocumentFragment *iface, BSTR *p)
{
    docfrag *docfrag = impl_from_IXMLDOMDocumentFragment(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_xml(docfrag->node, p);
}

static HRESULT WINAPI docfrag_transformNode(IXMLDOMDocumentFragment *iface, IXMLDOMNode *node, BSTR *p)
{
    docfrag *docfrag = impl_from_IXMLDOMDocumentFragment(iface);

    TRACE("%p, %p, %p.\n", iface, node, p);

    return node_transform_node(docfrag->node, node, p);
}

static HRESULT WINAPI docfrag_selectNodes(IXMLDOMDocumentFragment *iface, BSTR p, IXMLDOMNodeList **list)
{
    docfrag *docfrag = impl_from_IXMLDOMDocumentFragment(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_w(p), list);

    return node_select_nodes(docfrag->node, p, list);
}

static HRESULT WINAPI docfrag_selectSingleNode(IXMLDOMDocumentFragment *iface, BSTR p, IXMLDOMNode **node)
{
    docfrag *docfrag = impl_from_IXMLDOMDocumentFragment(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_w(p), node);

    return node_select_singlenode(docfrag->node, p, node);
}

static HRESULT WINAPI docfrag_get_parsed(IXMLDOMDocumentFragment *iface, VARIANT_BOOL *v)
{
    FIXME("%p, %p stub!\n", iface, v);

    *v = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI docfrag_get_namespaceURI(IXMLDOMDocumentFragment *iface, BSTR *p)
{
    docfrag *docfrag = impl_from_IXMLDOMDocumentFragment(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_namespaceURI(docfrag->node, p);
}

static HRESULT WINAPI docfrag_get_prefix(IXMLDOMDocumentFragment *iface, BSTR *prefix)
{
    TRACE("%p, %p.\n", iface, prefix);

    return return_null_bstr(prefix);
}

static HRESULT WINAPI docfrag_get_baseName(IXMLDOMDocumentFragment *iface, BSTR *name)
{
    TRACE("%p, %p.\n", iface, name);

    return return_null_bstr(name);
}

static HRESULT WINAPI docfrag_transformNodeToObject(IXMLDOMDocumentFragment *iface, IXMLDOMNode *node, VARIANT var)
{
    FIXME("%p, %p, %s.\n", iface, node, debugstr_variant(&var));

    return E_NOTIMPL;
}

static const struct IXMLDOMDocumentFragmentVtbl docfrag_vtbl =
{
    docfrag_QueryInterface,
    docfrag_AddRef,
    docfrag_Release,
    docfrag_GetTypeInfoCount,
    docfrag_GetTypeInfo,
    docfrag_GetIDsOfNames,
    docfrag_Invoke,
    docfrag_get_nodeName,
    docfrag_get_nodeValue,
    docfrag_put_nodeValue,
    docfrag_get_nodeType,
    docfrag_get_parentNode,
    docfrag_get_childNodes,
    docfrag_get_firstChild,
    docfrag_get_lastChild,
    docfrag_get_previousSibling,
    docfrag_get_nextSibling,
    docfrag_get_attributes,
    docfrag_insertBefore,
    docfrag_replaceChild,
    docfrag_removeChild,
    docfrag_appendChild,
    docfrag_hasChildNodes,
    docfrag_get_ownerDocument,
    docfrag_cloneNode,
    docfrag_get_nodeTypeString,
    docfrag_get_text,
    docfrag_put_text,
    docfrag_get_specified,
    docfrag_get_definition,
    docfrag_get_nodeTypedValue,
    docfrag_put_nodeTypedValue,
    docfrag_get_dataType,
    docfrag_put_dataType,
    docfrag_get_xml,
    docfrag_transformNode,
    docfrag_selectNodes,
    docfrag_selectSingleNode,
    docfrag_get_parsed,
    docfrag_get_namespaceURI,
    docfrag_get_prefix,
    docfrag_get_baseName,
    docfrag_transformNodeToObject
};

static const tid_t docfrag_iface_tids[] =
{
    IXMLDOMDocumentFragment_tid,
    0
};

static dispex_static_data_t docfrag_dispex =
{
    NULL,
    IXMLDOMDocumentFragment_tid,
    NULL,
    docfrag_iface_tids
};

HRESULT create_doc_fragment(struct domnode *node, IUnknown **obj)
{
    docfrag *object;

    *obj = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IXMLDOMDocumentFragment_iface.lpVtbl = &docfrag_vtbl;
    object->refcount = 1;
    object->node = domnode_addref(node);

    init_dispex(&object->dispex, (IUnknown *)&object->IXMLDOMDocumentFragment_iface, &docfrag_dispex);

    *obj = (IUnknown *)&object->IXMLDOMDocumentFragment_iface;

    return S_OK;
}
