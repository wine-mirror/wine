/*
 *    DOM Entity Reference implementation
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

typedef struct _entityref
{
    DispatchEx dispex;
    IXMLDOMEntityReference IXMLDOMEntityReference_iface;
    LONG refcount;
    struct domnode *node;
} entityref;

static const tid_t domentityref_se_tids[] =
{
    IXMLDOMNode_tid,
    IXMLDOMEntityReference_tid,
    NULL_tid
};

static inline entityref *impl_from_IXMLDOMEntityReference( IXMLDOMEntityReference *iface )
{
    return CONTAINING_RECORD(iface, entityref, IXMLDOMEntityReference_iface);
}

static HRESULT WINAPI entityref_QueryInterface(IXMLDOMEntityReference *iface, REFIID riid, void **obj)
{
    entityref *ref = impl_from_IXMLDOMEntityReference( iface );

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IXMLDOMEntityReference) ||
        IsEqualGUID(riid, &IID_IXMLDOMNode) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IUnknown) )
    {
        *obj = iface;
    }
    else if (dispex_query_interface(&ref->dispex, riid, obj))
    {
        return *obj ? S_OK : E_NOINTERFACE;
    }
    else if (node_query_interface(ref->node, riid, obj))
    {
        return *obj ? S_OK : E_NOINTERFACE;
    }
    else if (IsEqualGUID(riid, &IID_ISupportErrorInfo))
    {
        return node_create_supporterrorinfo(domentityref_se_tids, obj);
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

static ULONG WINAPI entityref_AddRef(IXMLDOMEntityReference *iface)
{
    entityref *ref = impl_from_IXMLDOMEntityReference(iface);
    ULONG refcount = InterlockedIncrement(&ref->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI entityref_Release(IXMLDOMEntityReference *iface)
{
    entityref *ref = impl_from_IXMLDOMEntityReference(iface);
    ULONG refcount = InterlockedDecrement(&ref->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        domnode_release(ref->node);
        free(ref);
    }

    return refcount;
}

static HRESULT WINAPI entityref_GetTypeInfoCount(IXMLDOMEntityReference *iface, UINT *count)
{
    entityref *ref = impl_from_IXMLDOMEntityReference(iface);
    return IDispatchEx_GetTypeInfoCount(&ref->dispex.IDispatchEx_iface, count);
}

static HRESULT WINAPI entityref_GetTypeInfo(IXMLDOMEntityReference *iface, UINT index, LCID lcid, ITypeInfo **ti)
{
    entityref *ref = impl_from_IXMLDOMEntityReference(iface);
    return IDispatchEx_GetTypeInfo(&ref->dispex.IDispatchEx_iface, index, lcid, ti);
}

static HRESULT WINAPI entityref_GetIDsOfNames(IXMLDOMEntityReference *iface, REFIID riid, LPOLESTR* rgszNames,
        UINT cNames, LCID lcid, DISPID* rgDispId )
{
    entityref *ref = impl_from_IXMLDOMEntityReference(iface);
    return IDispatchEx_GetIDsOfNames(&ref->dispex.IDispatchEx_iface, riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI entityref_Invoke(IXMLDOMEntityReference *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *ei, UINT *puArgErr)
{
    entityref *ref = impl_from_IXMLDOMEntityReference(iface);
    return IDispatchEx_Invoke(&ref->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, flags, params, result, ei, puArgErr);
}

static HRESULT WINAPI entityref_get_nodeName(IXMLDOMEntityReference *iface, BSTR *p)
{
    entityref *ref = impl_from_IXMLDOMEntityReference(iface);

    FIXME("%p, %p.\n", iface, p);

    return node_get_name(ref->node, p);
}

static HRESULT WINAPI entityref_get_nodeValue(IXMLDOMEntityReference *iface, VARIANT *value)
{
    TRACE("%p, %p.\n", iface, value);

    return return_null_var(value);
}

static HRESULT WINAPI entityref_put_nodeValue(IXMLDOMEntityReference *iface, VARIANT value)
{
    TRACE("%p, %s.\n", iface, debugstr_variant(&value));

    return E_FAIL;
}

static HRESULT WINAPI entityref_get_nodeType(IXMLDOMEntityReference *iface, DOMNodeType *type)
{
    TRACE("%p, %p.\n", iface, type);

    *type = NODE_ENTITY_REFERENCE;
    return S_OK;
}

static HRESULT WINAPI entityref_get_parentNode(IXMLDOMEntityReference *iface, IXMLDOMNode **parent)
{
    entityref *ref = impl_from_IXMLDOMEntityReference(iface);

    TRACE("%p, %p.\n", iface, parent);

    return node_get_parent(ref->node, parent);
}

static HRESULT WINAPI entityref_get_childNodes(IXMLDOMEntityReference *iface, IXMLDOMNodeList **list)
{
    entityref *ref = impl_from_IXMLDOMEntityReference(iface);

    TRACE("%p, %p.\n", iface, list);

    return node_get_child_nodes(ref->node, list);
}

static HRESULT WINAPI entityref_get_firstChild(IXMLDOMEntityReference *iface, IXMLDOMNode **node)
{
    entityref *ref = impl_from_IXMLDOMEntityReference(iface);

    TRACE("%p, %p.\n", iface, node);

    return node_get_first_child(ref->node, node);
}

static HRESULT WINAPI entityref_get_lastChild(IXMLDOMEntityReference *iface, IXMLDOMNode **node)
{
    entityref *ref = impl_from_IXMLDOMEntityReference(iface);

    TRACE("%p, %p.\n", iface, node);

    return node_get_last_child(ref->node, node);
}

static HRESULT WINAPI entityref_get_previousSibling(IXMLDOMEntityReference *iface, IXMLDOMNode **node)
{
    entityref *ref = impl_from_IXMLDOMEntityReference(iface);

    TRACE("%p, %p.\n", iface, node);

    return node_get_previous_sibling(ref->node, node);
}

static HRESULT WINAPI entityref_get_nextSibling(IXMLDOMEntityReference *iface, IXMLDOMNode **node)
{
    entityref *ref = impl_from_IXMLDOMEntityReference(iface);

    TRACE("%p, %p.\n", iface, node);

    return node_get_next_sibling(ref->node, node);
}

static HRESULT WINAPI entityref_get_attributes(IXMLDOMEntityReference *iface, IXMLDOMNamedNodeMap **map)
{
    TRACE("%p, %p.\n", iface, map);

    return return_null_ptr((void **)map);
}

static HRESULT WINAPI entityref_insertBefore(IXMLDOMEntityReference *iface,
    IXMLDOMNode* newNode, VARIANT refChild, IXMLDOMNode** outOldNode)
{
    entityref *ref = impl_from_IXMLDOMEntityReference(iface);

    FIXME("%p, %p, %s, %p needs test\n", iface, newNode, debugstr_variant(&refChild), outOldNode);

    return node_insert_before(ref->node, newNode, &refChild, outOldNode);
}

static HRESULT WINAPI entityref_replaceChild(IXMLDOMEntityReference *iface,
    IXMLDOMNode* newNode, IXMLDOMNode* oldNode, IXMLDOMNode** outOldNode)
{
    entityref *ref = impl_from_IXMLDOMEntityReference(iface);

    FIXME("%p, %p, %p, %p needs test\n", iface, newNode, oldNode, outOldNode);

    return node_replace_child(ref->node, newNode, oldNode, outOldNode);
}

static HRESULT WINAPI entityref_removeChild(IXMLDOMEntityReference *iface,
        IXMLDOMNode *child, IXMLDOMNode **oldChild)
{
    entityref *ref = impl_from_IXMLDOMEntityReference(iface);

    TRACE("%p, %p, %p.\n", iface, child, oldChild);

    return node_remove_child(ref->node, child, oldChild);
}

static HRESULT WINAPI entityref_appendChild(IXMLDOMEntityReference *iface,
        IXMLDOMNode *child, IXMLDOMNode **outChild)
{
    entityref *ref = impl_from_IXMLDOMEntityReference(iface);

    TRACE("%p, %p, %p.\n", iface, child, outChild);

    return node_append_child(ref->node, child, outChild);
}

static HRESULT WINAPI entityref_hasChildNodes(IXMLDOMEntityReference *iface, VARIANT_BOOL *ret)
{
    entityref *ref = impl_from_IXMLDOMEntityReference(iface);

    TRACE("%p, %p.\n", iface, ret);

    return node_has_childnodes(ref->node, ret);
}

static HRESULT WINAPI entityref_get_ownerDocument(IXMLDOMEntityReference *iface, IXMLDOMDocument **doc)
{
    entityref *ref = impl_from_IXMLDOMEntityReference(iface);

    TRACE("%p, %p.\n", iface, doc);

    return node_get_owner_document(ref->node, doc);
}

static HRESULT WINAPI entityref_cloneNode(IXMLDOMEntityReference *iface, VARIANT_BOOL deep, IXMLDOMNode **node)
{
    entityref *ref = impl_from_IXMLDOMEntityReference(iface);

    TRACE("%p, %d, %p.\n", iface, deep, node);

    return node_clone(ref->node, deep, node);
}

static HRESULT WINAPI entityref_get_nodeTypeString(IXMLDOMEntityReference *iface, BSTR *p)
{
    TRACE("%p, %p.\n", iface, p);

    return return_bstr(L"entityreference", p);
}

static HRESULT WINAPI entityref_get_text(IXMLDOMEntityReference *iface, BSTR *p)
{
    entityref *ref = impl_from_IXMLDOMEntityReference(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_text(ref->node, p);
}

static HRESULT WINAPI entityref_put_text(IXMLDOMEntityReference *iface, BSTR p)
{
    entityref *ref = impl_from_IXMLDOMEntityReference(iface);

    TRACE("%p, %s.\n", iface, debugstr_w(p));

    return node_put_data(ref->node, p);
}

static HRESULT WINAPI entityref_get_specified(IXMLDOMEntityReference *iface, VARIANT_BOOL *v)
{
    FIXME("%p, %p stub!\n", iface, v);

    *v = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI entityref_get_definition(IXMLDOMEntityReference *iface, IXMLDOMNode **node)
{
    FIXME("%p, %p.\n", iface, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI entityref_get_nodeTypedValue(IXMLDOMEntityReference *iface, VARIANT *v)
{
    FIXME("%p, %p.\n", iface, v);

    return return_null_var(v);
}

static HRESULT WINAPI entityref_put_nodeTypedValue(IXMLDOMEntityReference *iface, VARIANT v)
{
    FIXME("%p, %s.\n", iface, debugstr_variant(&v));

    return E_NOTIMPL;
}

static HRESULT WINAPI entityref_get_dataType(IXMLDOMEntityReference *iface, VARIANT *v)
{
    FIXME("%p, %p: should return a valid value\n", iface, v);

    return return_null_var(v);
}

static HRESULT WINAPI entityref_put_dataType(IXMLDOMEntityReference *iface, BSTR p)
{
    TRACE("%p, %s.\n", iface, debugstr_w(p));

    if (!p)
        return E_INVALIDARG;

    return E_FAIL;
}

static HRESULT WINAPI entityref_get_xml(IXMLDOMEntityReference *iface, BSTR *p)
{
    entityref *ref = impl_from_IXMLDOMEntityReference(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_xml(ref->node, p);
}

static HRESULT WINAPI entityref_transformNode(IXMLDOMEntityReference *iface, IXMLDOMNode *node, BSTR *p)
{
    entityref *ref = impl_from_IXMLDOMEntityReference(iface);

    TRACE("%p, %p, %p.\n", iface, node, p);

    return node_transform_node(ref->node, node, p);
}

static HRESULT WINAPI entityref_selectNodes(IXMLDOMEntityReference *iface, BSTR p, IXMLDOMNodeList **list)
{
    entityref *ref = impl_from_IXMLDOMEntityReference(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_w(p), list);

    return node_select_nodes(ref->node, p, list);
}

static HRESULT WINAPI entityref_selectSingleNode(IXMLDOMEntityReference *iface, BSTR p, IXMLDOMNode **node)
{
    entityref *ref = impl_from_IXMLDOMEntityReference(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_w(p), node);

    return node_select_singlenode(ref->node, p, node);
}

static HRESULT WINAPI entityref_get_parsed(IXMLDOMEntityReference *iface, VARIANT_BOOL *v)
{
    FIXME("%p, %p: stub!\n", iface, v);

    *v = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI entityref_get_namespaceURI(IXMLDOMEntityReference *iface, BSTR *p)
{
    entityref *ref = impl_from_IXMLDOMEntityReference(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_namespaceURI(ref->node, p);
}

static HRESULT WINAPI entityref_get_prefix(IXMLDOMEntityReference *iface, BSTR *prefix)
{
    FIXME("%p, %p: stub\n", iface, prefix);

    return return_null_bstr(prefix);
}

static HRESULT WINAPI entityref_get_baseName(IXMLDOMEntityReference *iface, BSTR *name)
{
    FIXME("%p, %p: needs test\n", iface, name);

    return return_null_bstr(name);
}

static HRESULT WINAPI entityref_transformNodeToObject(IXMLDOMEntityReference *iface, IXMLDOMNode *node, VARIANT var1)
{
    FIXME("%p, %p, %s.\n", iface, node, debugstr_variant(&var1));

    return E_NOTIMPL;
}

static const struct IXMLDOMEntityReferenceVtbl entityref_vtbl =
{
    entityref_QueryInterface,
    entityref_AddRef,
    entityref_Release,
    entityref_GetTypeInfoCount,
    entityref_GetTypeInfo,
    entityref_GetIDsOfNames,
    entityref_Invoke,
    entityref_get_nodeName,
    entityref_get_nodeValue,
    entityref_put_nodeValue,
    entityref_get_nodeType,
    entityref_get_parentNode,
    entityref_get_childNodes,
    entityref_get_firstChild,
    entityref_get_lastChild,
    entityref_get_previousSibling,
    entityref_get_nextSibling,
    entityref_get_attributes,
    entityref_insertBefore,
    entityref_replaceChild,
    entityref_removeChild,
    entityref_appendChild,
    entityref_hasChildNodes,
    entityref_get_ownerDocument,
    entityref_cloneNode,
    entityref_get_nodeTypeString,
    entityref_get_text,
    entityref_put_text,
    entityref_get_specified,
    entityref_get_definition,
    entityref_get_nodeTypedValue,
    entityref_put_nodeTypedValue,
    entityref_get_dataType,
    entityref_put_dataType,
    entityref_get_xml,
    entityref_transformNode,
    entityref_selectNodes,
    entityref_selectSingleNode,
    entityref_get_parsed,
    entityref_get_namespaceURI,
    entityref_get_prefix,
    entityref_get_baseName,
    entityref_transformNodeToObject,
};

static const tid_t domentityref_iface_tids[] =
{
    IXMLDOMEntityReference_tid,
    0
};

static dispex_static_data_t domentityref_dispex =
{
    NULL,
    IXMLDOMEntityReference_tid,
    NULL,
    domentityref_iface_tids
};

HRESULT create_entity_ref(struct domnode *node, IUnknown **obj)
{
    entityref *object;

    *obj = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IXMLDOMEntityReference_iface.lpVtbl = &entityref_vtbl;
    object->refcount = 1;
    object->node = domnode_addref(node);

    init_dispex(&object->dispex, (IUnknown *)&object->IXMLDOMEntityReference_iface, &domentityref_dispex);

    *obj = (IUnknown *)&object->IXMLDOMEntityReference_iface;

    return S_OK;
}
