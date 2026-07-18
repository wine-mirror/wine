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

struct domnotation
{
    DispatchEx dispex;
    IXMLDOMNotation IXMLDOMNotation_iface;
    LONG refcount;
    struct domnode *node;
};

static const tid_t notation_se_tids[] =
{
    IXMLDOMNode_tid,
    IXMLDOMNotation_tid,
    NULL_tid
};

static inline struct domnotation *impl_from_IXMLDOMNotation(IXMLDOMNotation *iface)
{
    return CONTAINING_RECORD(iface, struct domnotation, IXMLDOMNotation_iface);
}

static HRESULT WINAPI domnotation_QueryInterface(IXMLDOMNotation *iface, REFIID riid, void **obj)
{
    struct domnotation *notation = impl_from_IXMLDOMNotation(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IXMLDOMNotation) ||
        IsEqualGUID(riid, &IID_IXMLDOMNode) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        *obj = &notation->IXMLDOMNotation_iface;
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

static ULONG WINAPI domnotation_AddRef(IXMLDOMNotation *iface)
{
    struct domnotation *notation = impl_from_IXMLDOMNotation(iface);
    LONG refcount = InterlockedIncrement(&notation->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI domnotation_Release(IXMLDOMNotation *iface)
{
    struct domnotation *notation = impl_from_IXMLDOMNotation(iface);
    ULONG refcount = InterlockedDecrement(&notation->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    if (!refcount)
    {
        domnode_release(notation->node);
        free(notation);
    }

    return refcount;
}

static HRESULT WINAPI domnotation_GetTypeInfoCount(IXMLDOMNotation *iface, UINT *count)
{
    struct domnotation *notation = impl_from_IXMLDOMNotation(iface);
    return IDispatchEx_GetTypeInfoCount(&notation->dispex.IDispatchEx_iface, count);
}

static HRESULT WINAPI domnotation_GetTypeInfo(IXMLDOMNotation *iface, UINT index, LCID lcid, ITypeInfo **ti)
{
    struct domnotation *notation = impl_from_IXMLDOMNotation(iface);
    return IDispatchEx_GetTypeInfo(&notation->dispex.IDispatchEx_iface, index, lcid, ti);
}

static HRESULT WINAPI domnotation_GetIDsOfNames(IXMLDOMNotation *iface, REFIID riid, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID *rgDispId)
{
    struct domnotation *notation = impl_from_IXMLDOMNotation(iface);
    return IDispatchEx_GetIDsOfNames(&notation->dispex.IDispatchEx_iface, riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI domnotation_Invoke(IXMLDOMNotation *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *ei, UINT *puArgErr)
{
    struct domnotation *notation = impl_from_IXMLDOMNotation(iface);
    return IDispatchEx_Invoke(&notation->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, flags, params,
            result, ei, puArgErr);
}

static HRESULT WINAPI domnotation_get_nodeName(IXMLDOMNotation *iface, BSTR *p)
{
    struct domnotation *notation = impl_from_IXMLDOMNotation(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_name(notation->node, p);
}

static HRESULT WINAPI domnotation_get_nodeValue(IXMLDOMNotation *iface, VARIANT *v)
{
    FIXME("%p, %p: stub\n", iface, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI domnotation_put_nodeValue(IXMLDOMNotation *iface, VARIANT v)
{
    FIXME("%p, %s: stub\n", iface, debugstr_variant(&v));

    return E_NOTIMPL;
}

static HRESULT WINAPI domnotation_get_nodeType(IXMLDOMNotation *iface, DOMNodeType *type)
{
    TRACE("%p, %p.\n", iface, type);

    *type = NODE_NOTATION;
    return S_OK;
}

static HRESULT WINAPI domnotation_get_parentNode(IXMLDOMNotation *iface, IXMLDOMNode **parent)
{
    struct domnotation *notation = impl_from_IXMLDOMNotation(iface);

    TRACE("%p, %p.\n", iface, parent);

    return node_get_parent(notation->node, parent);
}

static HRESULT WINAPI domnotation_get_childNodes(IXMLDOMNotation *iface, IXMLDOMNodeList **list)
{
    FIXME("%p, %p: stub\n", iface, list);

    return E_NOTIMPL;
}

static HRESULT WINAPI domnotation_get_firstChild(IXMLDOMNotation *iface, IXMLDOMNode **node)
{
    FIXME("%p, %p: stub\n", iface, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domnotation_get_lastChild(IXMLDOMNotation *iface, IXMLDOMNode **node)
{
    FIXME("%p, %p: stub\n", iface, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domnotation_get_previousSibling(IXMLDOMNotation *iface, IXMLDOMNode **node)
{
    struct domnotation *notation = impl_from_IXMLDOMNotation(iface);

    TRACE("%p, %p.\n", iface, node);

    return node_get_previous_sibling(notation->node, node);
}

static HRESULT WINAPI domnotation_get_nextSibling(IXMLDOMNotation *iface, IXMLDOMNode **node)
{
    struct domnotation *notation = impl_from_IXMLDOMNotation(iface);

    TRACE("%p, %p.\n", iface, node);

    return node_get_next_sibling(notation->node, node);
}

static HRESULT WINAPI domnotation_get_attributes(IXMLDOMNotation *iface, IXMLDOMNamedNodeMap **map)
{
    FIXME("%p, %p: stub\n", iface, map);

    return E_NOTIMPL;
}

static HRESULT WINAPI domnotation_insertBefore(IXMLDOMNotation *iface, IXMLDOMNode *node,
        VARIANT refChild, IXMLDOMNode **out_node)
{
    FIXME("%p, %p, %s, %p: stub\n", iface, node, debugstr_variant(&refChild), out_node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domnotation_replaceChild(IXMLDOMNotation *iface, IXMLDOMNode *newNode,
        IXMLDOMNode *oldNode, IXMLDOMNode **outOldNode)
{
    FIXME("%p, %p, %p, %p: stub\n", iface, newNode, oldNode, outOldNode);

    return E_NOTIMPL;
}

static HRESULT WINAPI domnotation_removeChild(IXMLDOMNotation *iface, IXMLDOMNode *node,
        IXMLDOMNode **oldNode)
{
    FIXME("%p, %p, %p: stub\n", iface, node, oldNode);

    return E_NOTIMPL;
}

static HRESULT WINAPI domnotation_appendChild(IXMLDOMNotation *iface, IXMLDOMNode *newNode,
        IXMLDOMNode **outNewNode)
{
    FIXME("%p, %p, %p: stub\n", iface, newNode, outNewNode);

    return E_NOTIMPL;
}

static HRESULT WINAPI domnotation_hasChildNodes(IXMLDOMNotation *iface, VARIANT_BOOL *v)
{
    FIXME("%p, %p: stub\n", iface, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI domnotation_get_ownerDocument(IXMLDOMNotation *iface, IXMLDOMDocument **doc)
{
    struct domnotation *notation = impl_from_IXMLDOMNotation(iface);

    TRACE("%p, %p.\n", iface, doc);

    return node_get_owner_document(notation->node, doc);
}

static HRESULT WINAPI domnotation_cloneNode(IXMLDOMNotation *iface, VARIANT_BOOL deep, IXMLDOMNode **node)
{
    FIXME("%p, %d, %p: stub\n", iface, deep, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domnotation_get_nodeTypeString(IXMLDOMNotation *iface, BSTR *p)
{
    TRACE("%p, %p.\n", iface, p);

    return return_bstr(L"notation", p);
}

static HRESULT WINAPI domnotation_get_text(IXMLDOMNotation *iface, BSTR *p)
{
    TRACE("%p, %p.\n", iface, p);

    return return_bstr(L"", p);
}

static HRESULT WINAPI domnotation_put_text(IXMLDOMNotation *iface, BSTR p)
{
    TRACE("%p, %s.\n", iface, debugstr_w(p));

    return E_FAIL;
}

static HRESULT WINAPI domnotation_get_specified(IXMLDOMNotation *iface, VARIANT_BOOL *v)
{
    FIXME("%p, %p: stub\n", iface, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI domnotation_get_definition(IXMLDOMNotation *iface, IXMLDOMNode **node)
{
    FIXME("%p, %p: stub\n", iface, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domnotation_get_nodeTypedValue(IXMLDOMNotation *iface, VARIANT *v)
{
    FIXME("%p, %p: stub\n", iface, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI domnotation_put_nodeTypedValue(IXMLDOMNotation *iface, VARIANT value)
{
    FIXME("%p, %s: stub\n", iface, debugstr_variant(&value));

    return E_NOTIMPL;
}

static HRESULT WINAPI domnotation_get_dataType(IXMLDOMNotation *iface, VARIANT *v)
{
    FIXME("%p, %p: stub\n", iface, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI domnotation_put_dataType(IXMLDOMNotation *iface, BSTR p)
{
    FIXME("%p, %s: stub\n", iface, debugstr_w(p));

    return E_NOTIMPL;
}

static HRESULT WINAPI domnotation_get_xml(IXMLDOMNotation *iface, BSTR *p)
{
    FIXME("%p, %p.\n", iface, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI domnotation_transformNode(IXMLDOMNotation *iface, IXMLDOMNode *node, BSTR *p)
{
    FIXME("%p, %p, %p: stub\n", iface, node, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI domnotation_selectNodes(IXMLDOMNotation *iface, BSTR p, IXMLDOMNodeList **list)
{
    FIXME("%p, %s, %p: stub\n", iface, debugstr_w(p), list);

    return E_NOTIMPL;
}

static HRESULT WINAPI domnotation_selectSingleNode(IXMLDOMNotation *iface, BSTR p, IXMLDOMNode **node)
{
    FIXME("%p, %s, %p: stub\n", iface, debugstr_w(p), node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domnotation_get_parsed(IXMLDOMNotation *iface, VARIANT_BOOL *v)
{
    FIXME("%p, %p: stub\n", iface, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI domnotation_get_namespaceURI(IXMLDOMNotation *iface, BSTR *p)
{
    FIXME("%p, %p: stub\n", iface, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI domnotation_get_prefix(IXMLDOMNotation *iface, BSTR *prefix)
{
    FIXME("%p, %p: stub\n", iface, prefix);

    return E_NOTIMPL;
}

static HRESULT WINAPI domnotation_get_baseName(IXMLDOMNotation *iface, BSTR *name)
{
    struct domnotation *notation = impl_from_IXMLDOMNotation(iface);

    TRACE("%p, %p.\n", iface, name);

    return node_get_name(notation->node, name);
}

static HRESULT WINAPI domnotation_transformNodeToObject(IXMLDOMNotation *iface, IXMLDOMNode *node, VARIANT var1)
{
    FIXME("%p, %p, %s: stub\n", iface, node, debugstr_variant(&var1));

    return E_NOTIMPL;
}

static HRESULT WINAPI domnotation_get_publicId(IXMLDOMNotation *iface, VARIANT *id)
{
    FIXME("%p, %p: stub\n", iface, id);

    return E_NOTIMPL;
}

static HRESULT WINAPI domnotation_get_systemId(IXMLDOMNotation *iface, VARIANT *id)
{
    FIXME("%p, %p: stub\n", iface, id);

    return E_NOTIMPL;
}

static const struct IXMLDOMNotationVtbl domnotation_vtbl =
{
    domnotation_QueryInterface,
    domnotation_AddRef,
    domnotation_Release,
    domnotation_GetTypeInfoCount,
    domnotation_GetTypeInfo,
    domnotation_GetIDsOfNames,
    domnotation_Invoke,
    domnotation_get_nodeName,
    domnotation_get_nodeValue,
    domnotation_put_nodeValue,
    domnotation_get_nodeType,
    domnotation_get_parentNode,
    domnotation_get_childNodes,
    domnotation_get_firstChild,
    domnotation_get_lastChild,
    domnotation_get_previousSibling,
    domnotation_get_nextSibling,
    domnotation_get_attributes,
    domnotation_insertBefore,
    domnotation_replaceChild,
    domnotation_removeChild,
    domnotation_appendChild,
    domnotation_hasChildNodes,
    domnotation_get_ownerDocument,
    domnotation_cloneNode,
    domnotation_get_nodeTypeString,
    domnotation_get_text,
    domnotation_put_text,
    domnotation_get_specified,
    domnotation_get_definition,
    domnotation_get_nodeTypedValue,
    domnotation_put_nodeTypedValue,
    domnotation_get_dataType,
    domnotation_put_dataType,
    domnotation_get_xml,
    domnotation_transformNode,
    domnotation_selectNodes,
    domnotation_selectSingleNode,
    domnotation_get_parsed,
    domnotation_get_namespaceURI,
    domnotation_get_prefix,
    domnotation_get_baseName,
    domnotation_transformNodeToObject,
    domnotation_get_publicId,
    domnotation_get_systemId,
};

static const tid_t domnotation_iface_tids[] =
{
    IXMLDOMNotation_tid,
    0
};

static dispex_static_data_t domnotation_dispex =
{
    NULL,
    IXMLDOMNotation_tid,
    NULL,
    domnotation_iface_tids
};

HRESULT create_notation(struct domnode *node, IUnknown **obj)
{
    struct domnotation *object;

    *obj = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IXMLDOMNotation_iface.lpVtbl = &domnotation_vtbl;
    object->refcount = 1;
    object->node = domnode_addref(node);

    init_dispex(&object->dispex, (IUnknown *)&object->IXMLDOMNotation_iface, &domnotation_dispex);

    *obj = (IUnknown *)&object->IXMLDOMNotation_iface;

    return S_OK;
}
