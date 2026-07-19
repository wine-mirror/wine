/*
 * Copyright 2026 Nikolay Sivov for CodeWeavers
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
#include "winnls.h"
#include "ole2.h"
#include "msxml6.h"

#include "msxml_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

struct domentity
{
    DispatchEx dispex;
    IXMLDOMEntity IXMLDOMEntity_iface;
    LONG refcount;
    struct domnode *node;
};

static const tid_t notation_se_tids[] =
{
    IXMLDOMNode_tid,
    IXMLDOMEntity_tid,
    NULL_tid
};

static inline struct domentity *impl_from_IXMLDOMEntity(IXMLDOMEntity *iface)
{
    return CONTAINING_RECORD(iface, struct domentity, IXMLDOMEntity_iface);
}

static HRESULT WINAPI domentity_QueryInterface(IXMLDOMEntity *iface, REFIID riid, void **obj)
{
    struct domentity *notation = impl_from_IXMLDOMEntity(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IXMLDOMEntity) ||
        IsEqualGUID(riid, &IID_IXMLDOMNode) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        *obj = &notation->IXMLDOMEntity_iface;
    }
    else if (dispex_query_interface(&notation->dispex, riid, obj))
    {
        return *obj ? S_OK : E_NOINTERFACE;
    }
    else if (node_query_interface(notation->node, riid, obj))
    {
        return *obj ? S_OK : E_NOINTERFACE;
    }
    else if (IsEqualGUID(riid, &IID_ISupportErrorInfo))
    {
        return node_create_supporterrorinfo(notation_se_tids, obj);
    }
    else
    {
        TRACE("interface %s not implemented\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef( (IUnknown*)*obj );
    return S_OK;
}

static ULONG WINAPI domentity_AddRef(IXMLDOMEntity *iface)
{
    struct domentity *notation = impl_from_IXMLDOMEntity(iface);
    LONG refcount = InterlockedIncrement(&notation->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI domentity_Release(IXMLDOMEntity *iface)
{
    struct domentity *notation = impl_from_IXMLDOMEntity(iface);
    ULONG refcount = InterlockedDecrement(&notation->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    if (!refcount)
    {
        domnode_release(notation->node);
        free(notation);
    }

    return refcount;
}

static HRESULT WINAPI domentity_GetTypeInfoCount(IXMLDOMEntity *iface, UINT *count)
{
    struct domentity *notation = impl_from_IXMLDOMEntity(iface);
    return IDispatchEx_GetTypeInfoCount(&notation->dispex.IDispatchEx_iface, count);
}

static HRESULT WINAPI domentity_GetTypeInfo(IXMLDOMEntity *iface, UINT index, LCID lcid, ITypeInfo **ti)
{
    struct domentity *notation = impl_from_IXMLDOMEntity(iface);
    return IDispatchEx_GetTypeInfo(&notation->dispex.IDispatchEx_iface, index, lcid, ti);
}

static HRESULT WINAPI domentity_GetIDsOfNames(IXMLDOMEntity *iface, REFIID riid, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID *rgDispId)
{
    struct domentity *notation = impl_from_IXMLDOMEntity(iface);
    return IDispatchEx_GetIDsOfNames(&notation->dispex.IDispatchEx_iface, riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI domentity_Invoke(IXMLDOMEntity *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *ei, UINT *puArgErr)
{
    struct domentity *notation = impl_from_IXMLDOMEntity(iface);
    return IDispatchEx_Invoke(&notation->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, flags, params,
            result, ei, puArgErr);
}

static HRESULT WINAPI domentity_get_nodeName(IXMLDOMEntity *iface, BSTR *p)
{
    struct domentity *entity = impl_from_IXMLDOMEntity(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_name(entity->node, p);
}

static HRESULT WINAPI domentity_get_nodeValue(IXMLDOMEntity *iface, VARIANT *v)
{
    FIXME("%p, %p: stub\n", iface, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_put_nodeValue(IXMLDOMEntity *iface, VARIANT v)
{
    FIXME("%p, %s: stub\n", iface, debugstr_variant(&v));

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_get_nodeType(IXMLDOMEntity *iface, DOMNodeType *type)
{
    TRACE("%p, %p.\n", iface, type);

    *type = NODE_ENTITY;
    return S_OK;
}

static HRESULT WINAPI domentity_get_parentNode(IXMLDOMEntity *iface, IXMLDOMNode **parent)
{
    struct domentity *notation = impl_from_IXMLDOMEntity(iface);

    TRACE("%p, %p.\n", iface, parent);

    return node_get_parent(notation->node, parent);
}

static HRESULT WINAPI domentity_get_childNodes(IXMLDOMEntity *iface, IXMLDOMNodeList **list)
{
    FIXME("%p, %p: stub\n", iface, list);

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_get_firstChild(IXMLDOMEntity *iface, IXMLDOMNode **node)
{
    FIXME("%p, %p: stub\n", iface, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_get_lastChild(IXMLDOMEntity *iface, IXMLDOMNode **node)
{
    FIXME("%p, %p: stub\n", iface, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_get_previousSibling(IXMLDOMEntity *iface, IXMLDOMNode **node)
{
    struct domentity *notation = impl_from_IXMLDOMEntity(iface);

    TRACE("%p, %p.\n", iface, node);

    return node_get_previous_sibling(notation->node, node);
}

static HRESULT WINAPI domentity_get_nextSibling(IXMLDOMEntity *iface, IXMLDOMNode **node)
{
    struct domentity *notation = impl_from_IXMLDOMEntity(iface);

    TRACE("%p, %p.\n", iface, node);

    return node_get_next_sibling(notation->node, node);
}

static HRESULT WINAPI domentity_get_attributes(IXMLDOMEntity *iface, IXMLDOMNamedNodeMap **map)
{
    FIXME("%p, %p: stub\n", iface, map);

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_insertBefore(IXMLDOMEntity *iface, IXMLDOMNode *node,
        VARIANT refChild, IXMLDOMNode **out_node)
{
    FIXME("%p, %p, %s, %p: stub\n", iface, node, debugstr_variant(&refChild), out_node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_replaceChild(IXMLDOMEntity *iface, IXMLDOMNode *newNode,
        IXMLDOMNode *oldNode, IXMLDOMNode **outOldNode)
{
    FIXME("%p, %p, %p, %p: stub\n", iface, newNode, oldNode, outOldNode);

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_removeChild(IXMLDOMEntity *iface, IXMLDOMNode *node,
        IXMLDOMNode **oldNode)
{
    FIXME("%p, %p, %p: stub\n", iface, node, oldNode);

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_appendChild(IXMLDOMEntity *iface, IXMLDOMNode *newNode,
        IXMLDOMNode **outNewNode)
{
    FIXME("%p, %p, %p: stub\n", iface, newNode, outNewNode);

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_hasChildNodes(IXMLDOMEntity *iface, VARIANT_BOOL *v)
{
    FIXME("%p, %p: stub\n", iface, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_get_ownerDocument(IXMLDOMEntity *iface, IXMLDOMDocument **doc)
{
    struct domentity *notation = impl_from_IXMLDOMEntity(iface);

    TRACE("%p, %p.\n", iface, doc);

    return node_get_owner_document(notation->node, doc);
}

static HRESULT WINAPI domentity_cloneNode(IXMLDOMEntity *iface, VARIANT_BOOL deep, IXMLDOMNode **node)
{
    FIXME("%p, %d, %p: stub\n", iface, deep, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_get_nodeTypeString(IXMLDOMEntity *iface, BSTR *p)
{
    TRACE("%p, %p.\n", iface, p);

    return return_bstr(L"entity", p);
}

static HRESULT WINAPI domentity_get_text(IXMLDOMEntity *iface, BSTR *p)
{
    FIXME("%p, %p: stub\n", iface, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_put_text(IXMLDOMEntity *iface, BSTR p)
{
    FIXME("%p, %s: stub\n", iface, debugstr_w(p));

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_get_specified(IXMLDOMEntity *iface, VARIANT_BOOL *v)
{
    FIXME("%p, %p: stub\n", iface, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_get_definition(IXMLDOMEntity *iface, IXMLDOMNode **node)
{
    FIXME("%p, %p: stub\n", iface, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_get_nodeTypedValue(IXMLDOMEntity *iface, VARIANT *v)
{
    FIXME("%p, %p: stub\n", iface, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_put_nodeTypedValue(IXMLDOMEntity *iface, VARIANT value)
{
    FIXME("%p, %s: stub\n", iface, debugstr_variant(&value));

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_get_dataType(IXMLDOMEntity *iface, VARIANT *v)
{
    FIXME("%p, %p: stub\n", iface, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_put_dataType(IXMLDOMEntity *iface, BSTR p)
{
    FIXME("%p, %s: stub\n", iface, debugstr_w(p));

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_get_xml(IXMLDOMEntity *iface, BSTR *p)
{
    FIXME("%p, %p.\n", iface, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_transformNode(IXMLDOMEntity *iface, IXMLDOMNode *node, BSTR *p)
{
    FIXME("%p, %p, %p: stub\n", iface, node, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_selectNodes(IXMLDOMEntity *iface, BSTR p, IXMLDOMNodeList **list)
{
    FIXME("%p, %s, %p: stub\n", iface, debugstr_w(p), list);

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_selectSingleNode(IXMLDOMEntity *iface, BSTR p, IXMLDOMNode **node)
{
    FIXME("%p, %s, %p: stub\n", iface, debugstr_w(p), node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_get_parsed(IXMLDOMEntity *iface, VARIANT_BOOL *v)
{
    FIXME("%p, %p: stub\n", iface, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_get_namespaceURI(IXMLDOMEntity *iface, BSTR *p)
{
    FIXME("%p, %p: stub\n", iface, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_get_prefix(IXMLDOMEntity *iface, BSTR *prefix)
{
    FIXME("%p, %p: stub\n", iface, prefix);

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_get_baseName(IXMLDOMEntity *iface, BSTR *p)
{
    struct domentity *entity = impl_from_IXMLDOMEntity(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_name(entity->node, p);
}

static HRESULT WINAPI domentity_transformNodeToObject(IXMLDOMEntity *iface, IXMLDOMNode *node, VARIANT var1)
{
    FIXME("%p, %p, %s: stub\n", iface, node, debugstr_variant(&var1));

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_get_publicId(IXMLDOMEntity *iface, VARIANT *id)
{
    FIXME("%p, %p: stub\n", iface, id);

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_get_systemId(IXMLDOMEntity *iface, VARIANT *id)
{
    FIXME("%p, %p: stub\n", iface, id);

    return E_NOTIMPL;
}

static HRESULT WINAPI domentity_get_notationName(IXMLDOMEntity *iface, BSTR *p)
{
    FIXME("%p, %p: stub\n", iface, p);

    return E_NOTIMPL;
}

static const struct IXMLDOMEntityVtbl domentity_vtbl =
{
    domentity_QueryInterface,
    domentity_AddRef,
    domentity_Release,
    domentity_GetTypeInfoCount,
    domentity_GetTypeInfo,
    domentity_GetIDsOfNames,
    domentity_Invoke,
    domentity_get_nodeName,
    domentity_get_nodeValue,
    domentity_put_nodeValue,
    domentity_get_nodeType,
    domentity_get_parentNode,
    domentity_get_childNodes,
    domentity_get_firstChild,
    domentity_get_lastChild,
    domentity_get_previousSibling,
    domentity_get_nextSibling,
    domentity_get_attributes,
    domentity_insertBefore,
    domentity_replaceChild,
    domentity_removeChild,
    domentity_appendChild,
    domentity_hasChildNodes,
    domentity_get_ownerDocument,
    domentity_cloneNode,
    domentity_get_nodeTypeString,
    domentity_get_text,
    domentity_put_text,
    domentity_get_specified,
    domentity_get_definition,
    domentity_get_nodeTypedValue,
    domentity_put_nodeTypedValue,
    domentity_get_dataType,
    domentity_put_dataType,
    domentity_get_xml,
    domentity_transformNode,
    domentity_selectNodes,
    domentity_selectSingleNode,
    domentity_get_parsed,
    domentity_get_namespaceURI,
    domentity_get_prefix,
    domentity_get_baseName,
    domentity_transformNodeToObject,
    domentity_get_publicId,
    domentity_get_systemId,
    domentity_get_notationName,
};

static const tid_t domentity_iface_tids[] =
{
    IXMLDOMEntity_tid,
    0
};

static dispex_static_data_t domentity_dispex =
{
    NULL,
    IXMLDOMEntity_tid,
    NULL,
    domentity_iface_tids
};

HRESULT create_entity(struct domnode *node, IUnknown **obj)
{
    struct domentity *object;

    *obj = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IXMLDOMEntity_iface.lpVtbl = &domentity_vtbl;
    object->refcount = 1;
    object->node = domnode_addref(node);

    init_dispex(&object->dispex, (IUnknown *)&object->IXMLDOMEntity_iface, &domentity_dispex);

    *obj = (IUnknown *)&object->IXMLDOMEntity_iface;

    return S_OK;
}
