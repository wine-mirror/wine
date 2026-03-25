/*
 *    DOM processing instruction node implementation
 *
 * Copyright 2006 Huw Davies
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

#include "xmlparser.h"
#include "msxml_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

typedef struct _dom_pi
{
    DispatchEx dispex;
    IXMLDOMProcessingInstruction IXMLDOMProcessingInstruction_iface;
    LONG refcount;
    struct domnode *node;
} dom_pi;

static const struct nodemap_funcs dom_pi_attr_map;

static const tid_t dompi_se_tids[] =
{
    IXMLDOMNode_tid,
    IXMLDOMProcessingInstruction_tid,
    NULL_tid
};

static inline dom_pi *impl_from_IXMLDOMProcessingInstruction( IXMLDOMProcessingInstruction *iface )
{
    return CONTAINING_RECORD(iface, dom_pi, IXMLDOMProcessingInstruction_iface);
}

static HRESULT WINAPI dom_pi_QueryInterface(IXMLDOMProcessingInstruction *iface, REFIID riid, void **obj)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IXMLDOMProcessingInstruction) ||
        IsEqualGUID(riid, &IID_IXMLDOMNode) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        *obj = iface;
    }
    else if (dispex_query_interface(&pi->dispex, riid, obj))
    {
        return *obj ? S_OK : E_NOINTERFACE;
    }
    else if (node_query_interface(pi->node, riid, obj))
    {
        return *obj ? S_OK : E_NOINTERFACE;
    }
    else if (IsEqualGUID(riid, &IID_ISupportErrorInfo))
    {
        return node_create_supporterrorinfo(dompi_se_tids, obj);
    }
    else
    {
        TRACE("Unsupported interface %s\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static ULONG WINAPI dom_pi_AddRef(IXMLDOMProcessingInstruction *iface)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);
    ULONG refcount = InterlockedIncrement(&pi->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI dom_pi_Release(IXMLDOMProcessingInstruction *iface)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);
    ULONG refcount = InterlockedDecrement(&pi->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        domnode_release(pi->node);
        free(pi);
    }

    return refcount;
}

static HRESULT WINAPI dom_pi_GetTypeInfoCount(IXMLDOMProcessingInstruction *iface, UINT *count)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);
    return IDispatchEx_GetTypeInfoCount(&pi->dispex.IDispatchEx_iface, count);
}

static HRESULT WINAPI dom_pi_GetTypeInfo(IXMLDOMProcessingInstruction *iface, UINT index, LCID lcid, ITypeInfo **ti)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);
    return IDispatchEx_GetTypeInfo(&pi->dispex.IDispatchEx_iface, index, lcid, ti);
}

static HRESULT WINAPI dom_pi_GetIDsOfNames(
    IXMLDOMProcessingInstruction *iface,
    REFIID riid, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgDispId )
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);
    return IDispatchEx_GetIDsOfNames(&pi->dispex.IDispatchEx_iface,
        riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI dom_pi_Invoke(
    IXMLDOMProcessingInstruction *iface,
    DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo, UINT* puArgErr )
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);
    return IDispatchEx_Invoke(&pi->dispex.IDispatchEx_iface,
        dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI dom_pi_get_nodeName(IXMLDOMProcessingInstruction *iface, BSTR *p)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_name(pi->node, p);
}

static HRESULT WINAPI dom_pi_get_nodeValue(IXMLDOMProcessingInstruction *iface, VARIANT *value)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);

    TRACE("%p, %p.\n", iface, value);

    return node_get_value(pi->node, value);
}

static HRESULT WINAPI dom_pi_put_nodeValue(IXMLDOMProcessingInstruction *iface, VARIANT value)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);

    TRACE("%p, %s.\n", iface, debugstr_variant(&value));

    return node_put_value(pi->node, &value);
}

static HRESULT WINAPI dom_pi_get_nodeType(IXMLDOMProcessingInstruction *iface, DOMNodeType *type)
{
    TRACE("%p, %p.\n", iface, type);

    *type = NODE_PROCESSING_INSTRUCTION;
    return S_OK;
}

static HRESULT WINAPI dom_pi_get_parentNode(IXMLDOMProcessingInstruction *iface, IXMLDOMNode **parent)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);

    TRACE("%p, %p.\n", iface, parent);

    return node_get_parent(pi->node, parent);
}

static HRESULT WINAPI dom_pi_get_childNodes(IXMLDOMProcessingInstruction *iface, IXMLDOMNodeList **list)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction( iface );

    TRACE("%p, %p.\n", iface, list);

    return node_get_child_nodes(pi->node, list);
}

static HRESULT WINAPI dom_pi_get_firstChild(IXMLDOMProcessingInstruction *iface, IXMLDOMNode **node)
{
    TRACE("%p, %p.\n", iface, node);

    return return_null_node(node);
}

static HRESULT WINAPI dom_pi_get_lastChild(IXMLDOMProcessingInstruction *iface, IXMLDOMNode **node)
{
    TRACE("%p, %p.\n", iface, node);

    return return_null_node(node);
}

static HRESULT WINAPI dom_pi_get_previousSibling(IXMLDOMProcessingInstruction *iface, IXMLDOMNode **node)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);

    TRACE("%p, %p.\n", iface, node);

    return node_get_previous_sibling(pi->node, node);
}

static HRESULT WINAPI dom_pi_get_nextSibling(IXMLDOMProcessingInstruction *iface, IXMLDOMNode **node)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);

    TRACE("%p, %p.\n", iface, node);

    return node_get_next_sibling(pi->node, node);
}

static HRESULT WINAPI dom_pi_get_attributes(IXMLDOMProcessingInstruction *iface, IXMLDOMNamedNodeMap **map)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);

    TRACE("%p, %p.\n", iface, map);

    if (!map) return E_INVALIDARG;

    if (!wcscmp(pi->node->name, L"xml"))
    {
        return create_nodemap(pi->node, &dom_pi_attr_map, map);
    }
    else
    {
        *map = NULL;
        return S_FALSE;
    }
}

static HRESULT WINAPI dom_pi_insertBefore(IXMLDOMProcessingInstruction *iface, IXMLDOMNode *newNode, VARIANT refChild,
        IXMLDOMNode** outOldNode)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);

    FIXME("%p, %p, %s, %p needs test\n", iface, newNode, debugstr_variant(&refChild), outOldNode);

    return node_insert_before(pi->node, newNode, &refChild, outOldNode);
}

static HRESULT WINAPI dom_pi_replaceChild(IXMLDOMProcessingInstruction *iface, IXMLDOMNode *newNode,
        IXMLDOMNode* oldNode, IXMLDOMNode** outOldNode)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);

    FIXME("%p, %p, %p, %p needs test\n", iface, newNode, oldNode, outOldNode);

    return node_replace_child(pi->node, newNode, oldNode, outOldNode);
}

static HRESULT WINAPI dom_pi_removeChild(IXMLDOMProcessingInstruction *iface, IXMLDOMNode *child, IXMLDOMNode **oldChild)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);

    TRACE("%p, %p, %p.\n", iface, child, oldChild);

    return node_remove_child(pi->node, child, oldChild);
}

static HRESULT WINAPI dom_pi_appendChild(IXMLDOMProcessingInstruction *iface, IXMLDOMNode *child, IXMLDOMNode **outChild)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);

    TRACE("%p, %p, %p.\n", iface, child, outChild);

    return node_append_child(pi->node, child, outChild);
}

static HRESULT WINAPI dom_pi_hasChildNodes(IXMLDOMProcessingInstruction *iface, VARIANT_BOOL *ret)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction( iface );

    TRACE("%p, %p.\n", iface, ret);

    return node_has_childnodes(pi->node, ret);
}

static HRESULT WINAPI dom_pi_get_ownerDocument(IXMLDOMProcessingInstruction *iface, IXMLDOMDocument **doc)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);

    TRACE("%p, %p.\n", iface, doc);

    return node_get_owner_document(pi->node, doc);
}

static HRESULT WINAPI dom_pi_cloneNode(IXMLDOMProcessingInstruction *iface, VARIANT_BOOL deep, IXMLDOMNode **node)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);

    TRACE("%p, %d, %p.\n", iface, deep, node);

    return node_clone(pi->node, deep, node);
}

static HRESULT WINAPI dom_pi_get_nodeTypeString(IXMLDOMProcessingInstruction *iface, BSTR *p)
{
    TRACE("%p, %p.\n", iface, p);

    return return_bstr(L"processinginstruction", p);
}

static HRESULT WINAPI dom_pi_get_text(IXMLDOMProcessingInstruction *iface, BSTR *p)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_text(pi->node, p);
}

static HRESULT WINAPI dom_pi_put_text(IXMLDOMProcessingInstruction *iface, BSTR p)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);

    TRACE("%p, %s.\n", iface, debugstr_w(p));

    return node_put_data(pi->node, p);
}

static HRESULT WINAPI dom_pi_get_specified(IXMLDOMProcessingInstruction *iface, VARIANT_BOOL *v)
{
    FIXME("%p, %p stub!\n", iface, v);

    *v = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI dom_pi_get_definition(IXMLDOMProcessingInstruction *iface, IXMLDOMNode **node)
{
    FIXME("%p, %p.\n", iface, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI dom_pi_get_nodeTypedValue(IXMLDOMProcessingInstruction *iface, VARIANT *v)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);

    TRACE("%p, %p.\n", iface, v);

    return node_get_value(pi->node, v);
}

static HRESULT WINAPI dom_pi_put_nodeTypedValue(IXMLDOMProcessingInstruction *iface, VARIANT v)
{
    FIXME("%p, %s.\n", iface, debugstr_variant(&v));

    return E_NOTIMPL;
}

static HRESULT WINAPI dom_pi_get_dataType(IXMLDOMProcessingInstruction *iface, VARIANT *v)
{
    TRACE("%p, %p.\n", iface, v);

    return return_null_var(v);
}

static HRESULT WINAPI dom_pi_put_dataType(IXMLDOMProcessingInstruction *iface, BSTR p)
{
    TRACE("%p, %s.\n", iface, debugstr_w(p));

    if (!p)
        return E_INVALIDARG;

    return E_FAIL;
}

static HRESULT WINAPI dom_pi_get_xml(IXMLDOMProcessingInstruction *iface, BSTR *p)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_xml(pi->node, p);
}

static HRESULT WINAPI dom_pi_transformNode(IXMLDOMProcessingInstruction *iface, IXMLDOMNode *node, BSTR *p)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);

    TRACE("%p, %p, %p.\n", iface, node, p);

    return node_transform_node(pi->node, node, p);
}

static HRESULT WINAPI dom_pi_selectNodes(IXMLDOMProcessingInstruction *iface, BSTR p, IXMLDOMNodeList **list)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction( iface );

    TRACE("%p, %s, %p.\n", iface, debugstr_w(p), list);

    return node_select_nodes(pi->node, p, list);
}

static HRESULT WINAPI dom_pi_selectSingleNode(IXMLDOMProcessingInstruction *iface, BSTR p, IXMLDOMNode **node)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_w(p), node);

    return node_select_singlenode(pi->node, p, node);
}

static HRESULT WINAPI dom_pi_get_parsed(IXMLDOMProcessingInstruction *iface, VARIANT_BOOL *v)
{
    FIXME("%p, %p stub!\n", iface, v);

    *v = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI dom_pi_get_namespaceURI(IXMLDOMProcessingInstruction *iface, BSTR *p)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_namespaceURI(pi->node, p);
}

static HRESULT WINAPI dom_pi_get_prefix(IXMLDOMProcessingInstruction *iface, BSTR *prefix)
{
    TRACE("%p, %p.\n", iface, prefix);

    return return_null_bstr(prefix);
}

static HRESULT WINAPI dom_pi_get_baseName(IXMLDOMProcessingInstruction *iface, BSTR *name)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);

    TRACE("%p, %p.\n", iface, name);

    return node_get_base_name(pi->node, name);
}

static HRESULT WINAPI dom_pi_transformNodeToObject(IXMLDOMProcessingInstruction *iface, IXMLDOMNode *node, VARIANT var1)
{
    FIXME("%p, %p, %s.\n", iface, node, debugstr_variant(&var1));

    return E_NOTIMPL;
}

static HRESULT WINAPI dom_pi_get_target(IXMLDOMProcessingInstruction *iface, BSTR *p)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_name(pi->node, p);
}

static HRESULT WINAPI dom_pi_get_data(IXMLDOMProcessingInstruction *iface, BSTR *p)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_data(pi->node, p);
}

static HRESULT WINAPI dom_pi_put_data(IXMLDOMProcessingInstruction *iface, BSTR data)
{
    dom_pi *pi = impl_from_IXMLDOMProcessingInstruction( iface );

    TRACE("%p, %s.\n", iface, debugstr_w(data));

    return node_put_data(pi->node, data);
}

static const struct IXMLDOMProcessingInstructionVtbl dom_pi_vtbl =
{
    dom_pi_QueryInterface,
    dom_pi_AddRef,
    dom_pi_Release,
    dom_pi_GetTypeInfoCount,
    dom_pi_GetTypeInfo,
    dom_pi_GetIDsOfNames,
    dom_pi_Invoke,
    dom_pi_get_nodeName,
    dom_pi_get_nodeValue,
    dom_pi_put_nodeValue,
    dom_pi_get_nodeType,
    dom_pi_get_parentNode,
    dom_pi_get_childNodes,
    dom_pi_get_firstChild,
    dom_pi_get_lastChild,
    dom_pi_get_previousSibling,
    dom_pi_get_nextSibling,
    dom_pi_get_attributes,
    dom_pi_insertBefore,
    dom_pi_replaceChild,
    dom_pi_removeChild,
    dom_pi_appendChild,
    dom_pi_hasChildNodes,
    dom_pi_get_ownerDocument,
    dom_pi_cloneNode,
    dom_pi_get_nodeTypeString,
    dom_pi_get_text,
    dom_pi_put_text,
    dom_pi_get_specified,
    dom_pi_get_definition,
    dom_pi_get_nodeTypedValue,
    dom_pi_put_nodeTypedValue,
    dom_pi_get_dataType,
    dom_pi_put_dataType,
    dom_pi_get_xml,
    dom_pi_transformNode,
    dom_pi_selectNodes,
    dom_pi_selectSingleNode,
    dom_pi_get_parsed,
    dom_pi_get_namespaceURI,
    dom_pi_get_prefix,
    dom_pi_get_baseName,
    dom_pi_transformNodeToObject,
    dom_pi_get_target,
    dom_pi_get_data,
    dom_pi_put_data,
};

static HRESULT dom_pi_get_qualified_item(const struct domnode *node, BSTR name, BSTR uri,
    IXMLDOMNode **item)
{
    FIXME("%p, %s, %s, %p: stub\n", node, debugstr_w(name), debugstr_w(uri), item);

    return E_NOTIMPL;
}

static HRESULT dom_pi_get_named_item(const struct domnode *node, BSTR name, IXMLDOMNode **item)
{
    TRACE("%p, %s, %p.\n", node, debugstr_w(name), item);

    if (!item)
        return E_POINTER;

    return node_get_attribute(node, name, (IXMLDOMAttribute **)item);
}

static HRESULT dom_pi_set_named_item(struct domnode *node, IXMLDOMNode *newItem, IXMLDOMNode **namedItem)
{
    TRACE("%p, %p, %p.\n", node, newItem, namedItem);

    return node_set_attribute(node, newItem, namedItem);
}

static HRESULT dom_pi_remove_qualified_item(struct domnode *node, BSTR name, BSTR uri, IXMLDOMNode **item)
{
    FIXME("%p, %s, %s, %p: stub\n", node, debugstr_w(name), debugstr_w(uri), item);

    return E_NOTIMPL;
}

static HRESULT dom_pi_remove_named_item(struct domnode *node, BSTR name, IXMLDOMNode **item)
{
    TRACE("%p, %s, %p.\n", node, debugstr_w(name), item);

    return node_remove_attribute(node, name, item);
}

static HRESULT dom_pi_get_item(struct domnode *node, LONG index, IXMLDOMNode **item)
{
    TRACE("%p, %ld, %p.\n", node, index, item);

    return node_get_attribute_by_index(node, index, item);
}

static HRESULT dom_pi_get_length(struct domnode *node, LONG *length)
{
    TRACE("%p, %p.\n", node, length);

    *length = list_count(&node->attributes);
    return S_OK;
}

static HRESULT dom_pi_next_node(const struct domnode *node, LONG *iter, IXMLDOMNode **nextNode)
{
    FIXME("%p, %ld, %p: stub\n", node, *iter, nextNode);
    return E_NOTIMPL;
}

static const struct nodemap_funcs dom_pi_attr_map =
{
    .get_named_item = dom_pi_get_named_item,
    .set_named_item = dom_pi_set_named_item,
    .remove_named_item = dom_pi_remove_named_item,
    .get_item = dom_pi_get_item,
    .get_length = dom_pi_get_length,
    .get_qualified_item = dom_pi_get_qualified_item,
    .remove_qualified_item = dom_pi_remove_qualified_item,
    .next_node = dom_pi_next_node,
};

static const tid_t dompi_iface_tids[] =
{
    IXMLDOMProcessingInstruction_tid,
    0
};

static dispex_static_data_t dompi_dispex =
{
    NULL,
    IXMLDOMProcessingInstruction_tid,
    NULL,
    dompi_iface_tids
};

HRESULT create_pi(struct domnode *node, IUnknown **obj)
{
    dom_pi *object;

    *obj = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IXMLDOMProcessingInstruction_iface.lpVtbl = &dom_pi_vtbl;
    object->refcount = 1;
    object->node = domnode_addref(node);

    init_dispex(&object->dispex, (IUnknown *)&object->IXMLDOMProcessingInstruction_iface, &dompi_dispex);

    *obj = (IUnknown *)&object->IXMLDOMProcessingInstruction_iface;

    return S_OK;
}
