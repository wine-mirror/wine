/*
 *    DOM comment node implementation
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

#include "msxml_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

typedef struct _domcomment
{
    DispatchEx dispex;
    IXMLDOMComment IXMLDOMComment_iface;
    LONG refcount;
    struct domnode *node;
} domcomment;

static const tid_t domcomment_se_tids[] =
{
    IXMLDOMNode_tid,
    IXMLDOMComment_tid,
    NULL_tid
};

static inline domcomment *impl_from_IXMLDOMComment(IXMLDOMComment *iface)
{
    return CONTAINING_RECORD(iface, domcomment, IXMLDOMComment_iface);
}

static HRESULT WINAPI domcomment_QueryInterface(IXMLDOMComment *iface, REFIID riid, void **obj)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IXMLDOMComment) ||
        IsEqualGUID(riid, &IID_IXMLDOMCharacterData) ||
        IsEqualGUID(riid, &IID_IXMLDOMNode) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        *obj = iface;
    }
    else if (dispex_query_interface(&comment->dispex, riid, obj))
    {
        return *obj ? S_OK : E_NOINTERFACE;
    }
    else if (node_query_interface(comment->node, riid, obj))
    {
        return *obj ? S_OK : E_NOINTERFACE;
    }
    else if (IsEqualGUID(riid, &IID_ISupportErrorInfo))
    {
        return node_create_supporterrorinfo(domcomment_se_tids, obj);
    }
    else
    {
        TRACE("Unsupported interface %s\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IXMLDOMComment_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI domcomment_AddRef(IXMLDOMComment *iface)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);
    ULONG refcount = InterlockedIncrement(&comment->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI domcomment_Release(IXMLDOMComment *iface)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);
    ULONG refcount = InterlockedDecrement(&comment->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        domnode_release(comment->node);
        free(comment);
    }

    return refcount;
}

static HRESULT WINAPI domcomment_GetTypeInfoCount(IXMLDOMComment *iface, UINT *count)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);
    return IDispatchEx_GetTypeInfoCount(&comment->dispex.IDispatchEx_iface, count);
}

static HRESULT WINAPI domcomment_GetTypeInfo(IXMLDOMComment *iface, UINT index, LCID lcid, ITypeInfo **ti)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);
    return IDispatchEx_GetTypeInfo(&comment->dispex.IDispatchEx_iface, index, lcid, ti);
}

static HRESULT WINAPI domcomment_GetIDsOfNames(IXMLDOMComment *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);
    return IDispatchEx_GetIDsOfNames(&comment->dispex.IDispatchEx_iface,
        riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI domcomment_Invoke(IXMLDOMComment *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *ei, UINT *puArgErr)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);
    return IDispatchEx_Invoke(&comment->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, flags,
            params, result, ei, puArgErr);
}

static HRESULT WINAPI domcomment_get_nodeName(IXMLDOMComment *iface, BSTR *p)
{
    TRACE("%p, %p.\n", iface, p);

    return return_bstr(L"#comment", p);
}

static HRESULT WINAPI domcomment_get_nodeValue(IXMLDOMComment *iface, VARIANT *value)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    TRACE("%p, %p.\n", iface, value);

    return node_get_value(comment->node, value);
}

static HRESULT WINAPI domcomment_put_nodeValue(IXMLDOMComment *iface, VARIANT value)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    TRACE("%p, %s.\n", iface, debugstr_variant(&value));

    return node_put_value(comment->node, &value);
}

static HRESULT WINAPI domcomment_get_nodeType(IXMLDOMComment *iface, DOMNodeType *type)
{
    TRACE("%p, %p.\n", iface, type);

    *type = NODE_COMMENT;
    return S_OK;
}

static HRESULT WINAPI domcomment_get_parentNode(IXMLDOMComment *iface, IXMLDOMNode **parent)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    TRACE("%p, %p.\n", iface, parent);

    return node_get_parent(comment->node, parent);
}

static HRESULT WINAPI domcomment_get_childNodes(IXMLDOMComment *iface, IXMLDOMNodeList **list)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    TRACE("%p, %p.\n", iface, list);

    return node_get_child_nodes(comment->node, list);
}

static HRESULT WINAPI domcomment_get_firstChild(IXMLDOMComment *iface, IXMLDOMNode **node)
{
    TRACE("%p, %p.\n", iface, node);

    return return_null_node(node);
}

static HRESULT WINAPI domcomment_get_lastChild(IXMLDOMComment *iface, IXMLDOMNode **node)
{
    TRACE("%p, %p.\n", iface, node);

    return return_null_node(node);
}

static HRESULT WINAPI domcomment_get_previousSibling(IXMLDOMComment *iface, IXMLDOMNode **node)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    TRACE("%p, %p.\n", iface, node);

    return node_get_previous_sibling(comment->node, node);
}

static HRESULT WINAPI domcomment_get_nextSibling(IXMLDOMComment *iface, IXMLDOMNode **node)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    TRACE("%p, %p.\n", iface, node);

    return node_get_next_sibling(comment->node, node);
}

static HRESULT WINAPI domcomment_get_attributes(IXMLDOMComment *iface, IXMLDOMNamedNodeMap **map)
{
    TRACE("%p, %p.\n", iface, map);

    return return_null_ptr((void **)map);
}

static HRESULT WINAPI domcomment_insertBefore(IXMLDOMComment *iface, IXMLDOMNode *newNode,
        VARIANT refChild, IXMLDOMNode **outOldNode)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    FIXME("%p, %p, %s, %p needs test\n", iface, newNode, debugstr_variant(&refChild), outOldNode);

    return node_insert_before(comment->node, newNode, &refChild, outOldNode);
}

static HRESULT WINAPI domcomment_replaceChild(IXMLDOMComment *iface, IXMLDOMNode *newNode,
        IXMLDOMNode *oldNode, IXMLDOMNode **outOldNode)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    FIXME("%p, %p, %p, %p needs tests\n", iface, newNode, oldNode, outOldNode);

    return node_replace_child(comment->node, newNode, oldNode, outOldNode);
}

static HRESULT WINAPI domcomment_removeChild(IXMLDOMComment *iface, IXMLDOMNode *child, IXMLDOMNode **oldChild)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    TRACE("%p, %p, %p.\n", iface, child, oldChild);

    return node_remove_child(comment->node, child, oldChild);
}

static HRESULT WINAPI domcomment_appendChild(IXMLDOMComment *iface, IXMLDOMNode *child, IXMLDOMNode **outChild)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    TRACE("%p, %p, %p.\n", iface, child, outChild);

    return node_append_child(comment->node, child, outChild);
}

static HRESULT WINAPI domcomment_hasChildNodes(IXMLDOMComment *iface, VARIANT_BOOL *ret)
{
    TRACE("%p %p.\n", iface, ret);

    return return_var_false(ret);
}

static HRESULT WINAPI domcomment_get_ownerDocument(IXMLDOMComment *iface, IXMLDOMDocument **doc)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    TRACE("%p, %p.\n", iface, doc);

    return node_get_owner_document(comment->node, doc);
}

static HRESULT WINAPI domcomment_cloneNode(IXMLDOMComment *iface, VARIANT_BOOL deep, IXMLDOMNode **node)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    TRACE("%p, %d, %p.\n", iface, deep, node);

    return node_clone(comment->node, deep, node);
}

static HRESULT WINAPI domcomment_get_nodeTypeString(IXMLDOMComment *iface, BSTR *p)
{
    TRACE("%p, %p.\n", iface, p);

    return return_bstr(L"comment", p);
}

static HRESULT WINAPI domcomment_get_text(IXMLDOMComment *iface, BSTR *p)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_text(comment->node, p);
}

static HRESULT WINAPI domcomment_put_text(IXMLDOMComment *iface, BSTR p)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    TRACE("%p, %s.\n", iface, debugstr_w(p));

    return node_put_data(comment->node, p);
}

static HRESULT WINAPI domcomment_get_specified(IXMLDOMComment *iface, VARIANT_BOOL *v)
{
    FIXME("%p, %p stub!\n", iface, v);

    *v = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI domcomment_get_definition(IXMLDOMComment *iface, IXMLDOMNode **node)
{
    TRACE("%p, %p.\n", iface, node);

    return return_null_node(node);
}

static HRESULT WINAPI domcomment_get_nodeTypedValue(IXMLDOMComment *iface, VARIANT *v)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    TRACE("%p, %p.\n", iface, v);

    return node_get_value(comment->node, v);
}

static HRESULT WINAPI domcomment_put_nodeTypedValue(IXMLDOMComment *iface, VARIANT v)
{
    FIXME("%p, %s.\n", iface, debugstr_variant(&v));

    return E_NOTIMPL;
}

static HRESULT WINAPI domcomment_get_dataType(IXMLDOMComment *iface, VARIANT *v)
{
    TRACE("%p, %p.\n", iface, v);

    return return_null_var(v);
}

static HRESULT WINAPI domcomment_put_dataType(IXMLDOMComment *iface, BSTR p)
{
    TRACE("%p, %s.\n", iface, debugstr_w(p));

    if (!p)
        return E_INVALIDARG;

    return E_FAIL;
}

static HRESULT WINAPI domcomment_get_xml(IXMLDOMComment *iface, BSTR *p)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_xml(comment->node, p);
}

static HRESULT WINAPI domcomment_transformNode(IXMLDOMComment *iface, IXMLDOMNode *node, BSTR *p)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    TRACE("%p, %p, %p.\n", iface, node, p);

    return node_transform_node(comment->node, node, p);
}

static HRESULT WINAPI domcomment_selectNodes(IXMLDOMComment *iface, BSTR p, IXMLDOMNodeList **list)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_w(p), list);

    return node_select_nodes(comment->node, p, list);
}

static HRESULT WINAPI domcomment_selectSingleNode(IXMLDOMComment *iface, BSTR p, IXMLDOMNode **node)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_w(p), node);

    return node_select_singlenode(comment->node, p, node);
}

static HRESULT WINAPI domcomment_get_parsed(IXMLDOMComment *iface, VARIANT_BOOL *v)
{
    FIXME("%p, %p stub!\n", iface, v);

    *v = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI domcomment_get_namespaceURI(IXMLDOMComment *iface, BSTR *p)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_namespaceURI(comment->node, p);
}

static HRESULT WINAPI domcomment_get_prefix(IXMLDOMComment *iface, BSTR *prefix)
{
    TRACE("%p, %p.\n", iface, prefix);

    return return_null_bstr(prefix);
}

static HRESULT WINAPI domcomment_get_baseName(IXMLDOMComment *iface, BSTR *name)
{
    TRACE("%p, %p.\n", iface, name);

    return return_null_bstr(name);
}

static HRESULT WINAPI domcomment_transformNodeToObject(IXMLDOMComment *iface, IXMLDOMNode *node, VARIANT v)
{
    FIXME("%p, %p, %s.\n", iface, node, debugstr_variant(&v));

    return E_NOTIMPL;
}

static HRESULT WINAPI domcomment_get_data(IXMLDOMComment *iface, BSTR *p)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_data(comment->node, p);
}

static HRESULT WINAPI domcomment_put_data(IXMLDOMComment *iface, BSTR data)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    TRACE("%p, %s.\n", iface, debugstr_w(data));

    return node_put_data(comment->node, data);
}

static HRESULT WINAPI domcomment_get_length(IXMLDOMComment *iface, LONG *length)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    TRACE("%p, %p.\n", iface, length);

    return node_get_data_length(comment->node, length);
}

static HRESULT WINAPI domcomment_substringData(IXMLDOMComment *iface, LONG offset, LONG count, BSTR *p)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    TRACE("%p, %ld, %ld, %p.\n", iface, offset, count, p);

    return node_substring_data(comment->node, offset, count, p);
}

static HRESULT WINAPI domcomment_appendData(IXMLDOMComment *iface, BSTR p)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    TRACE("%p, %s.\n", iface, debugstr_w(p));

    return node_append_data(comment->node, p);
}

static HRESULT WINAPI domcomment_insertData(IXMLDOMComment *iface, LONG offset, BSTR p)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    TRACE("%p, %ld, %s.\n", iface, offset, debugstr_w(p));

    return node_insert_data(comment->node, offset, p);
}

static HRESULT WINAPI domcomment_deleteData(IXMLDOMComment *iface, LONG offset, LONG count)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    TRACE("%p, %ld, %ld.\n", iface, offset, count);

    return node_delete_data(comment->node, offset, count);
}

static HRESULT WINAPI domcomment_replaceData(IXMLDOMComment *iface, LONG offset, LONG count, BSTR p)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);

    TRACE("%p, %ld, %ld, %s.\n", iface, offset, count, debugstr_w(p));

    return node_replace_data(comment->node, offset, count, p);
}

static const struct IXMLDOMCommentVtbl domcomment_vtbl =
{
    domcomment_QueryInterface,
    domcomment_AddRef,
    domcomment_Release,
    domcomment_GetTypeInfoCount,
    domcomment_GetTypeInfo,
    domcomment_GetIDsOfNames,
    domcomment_Invoke,
    domcomment_get_nodeName,
    domcomment_get_nodeValue,
    domcomment_put_nodeValue,
    domcomment_get_nodeType,
    domcomment_get_parentNode,
    domcomment_get_childNodes,
    domcomment_get_firstChild,
    domcomment_get_lastChild,
    domcomment_get_previousSibling,
    domcomment_get_nextSibling,
    domcomment_get_attributes,
    domcomment_insertBefore,
    domcomment_replaceChild,
    domcomment_removeChild,
    domcomment_appendChild,
    domcomment_hasChildNodes,
    domcomment_get_ownerDocument,
    domcomment_cloneNode,
    domcomment_get_nodeTypeString,
    domcomment_get_text,
    domcomment_put_text,
    domcomment_get_specified,
    domcomment_get_definition,
    domcomment_get_nodeTypedValue,
    domcomment_put_nodeTypedValue,
    domcomment_get_dataType,
    domcomment_put_dataType,
    domcomment_get_xml,
    domcomment_transformNode,
    domcomment_selectNodes,
    domcomment_selectSingleNode,
    domcomment_get_parsed,
    domcomment_get_namespaceURI,
    domcomment_get_prefix,
    domcomment_get_baseName,
    domcomment_transformNodeToObject,
    domcomment_get_data,
    domcomment_put_data,
    domcomment_get_length,
    domcomment_substringData,
    domcomment_appendData,
    domcomment_insertData,
    domcomment_deleteData,
    domcomment_replaceData
};

static const tid_t domcomment_iface_tids[] =
{
    IXMLDOMComment_tid,
    0
};

static dispex_static_data_t domcomment_dispex =
{
    NULL,
    IXMLDOMComment_tid,
    NULL,
    domcomment_iface_tids
};

HRESULT create_comment(struct domnode *node, IUnknown **obj)
{
    domcomment *object;

    *obj = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IXMLDOMComment_iface.lpVtbl = &domcomment_vtbl;
    object->refcount = 1;
    object->node = domnode_addref(node);

    init_dispex(&object->dispex, (IUnknown *)&object->IXMLDOMComment_iface, &domcomment_dispex);

    *obj = (IUnknown *)&object->IXMLDOMComment_iface;

    return S_OK;
}
