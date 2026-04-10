/*
 *    DOM DTD node implementation
 *
 * Copyright 2010 Nikolay Sivov
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

typedef struct _domdoctype
{
    DispatchEx dispex;
    IXMLDOMDocumentType IXMLDOMDocumentType_iface;
    LONG refcount;
    struct domnode *node;
} domdoctype;

static const tid_t doctype_se_tids[] =
{
    IXMLDOMNode_tid,
    IXMLDOMDocumentType_tid,
    NULL_tid
};

static inline domdoctype *impl_from_IXMLDOMDocumentType( IXMLDOMDocumentType *iface )
{
    return CONTAINING_RECORD(iface, domdoctype, IXMLDOMDocumentType_iface);
}

static HRESULT WINAPI domdoctype_QueryInterface(IXMLDOMDocumentType *iface, REFIID riid, void **obj)
{
    domdoctype *doctype = impl_from_IXMLDOMDocumentType(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IXMLDOMDocumentType) ||
        IsEqualGUID(riid, &IID_IXMLDOMNode) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        *obj = &doctype->IXMLDOMDocumentType_iface;
    }
    else if (dispex_query_interface(&doctype->dispex, riid, obj))
    {
        return *obj ? S_OK : E_NOINTERFACE;
    }
    else if (node_query_interface(doctype->node, riid, obj))
    {
        return *obj ? S_OK : E_NOINTERFACE;
    }
    else if (IsEqualGUID(riid, &IID_ISupportErrorInfo))
    {
        return node_create_supporterrorinfo(doctype_se_tids, obj);
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

static ULONG WINAPI domdoctype_AddRef(IXMLDOMDocumentType *iface)
{
    domdoctype *doctype = impl_from_IXMLDOMDocumentType(iface);
    LONG refcount = InterlockedIncrement(&doctype->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI domdoctype_Release(IXMLDOMDocumentType *iface)
{
    domdoctype *doctype = impl_from_IXMLDOMDocumentType(iface);
    ULONG refcount = InterlockedDecrement(&doctype->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    if (!refcount)
    {
        domnode_release(doctype->node);
        free(doctype);
    }

    return refcount;
}

static HRESULT WINAPI domdoctype_GetTypeInfoCount(IXMLDOMDocumentType *iface, UINT *count)
{
    domdoctype *doctype = impl_from_IXMLDOMDocumentType(iface);
    return IDispatchEx_GetTypeInfoCount(&doctype->dispex.IDispatchEx_iface, count);
}

static HRESULT WINAPI domdoctype_GetTypeInfo(IXMLDOMDocumentType *iface, UINT index, LCID lcid, ITypeInfo **ti)
{
    domdoctype *doctype = impl_from_IXMLDOMDocumentType(iface);
    return IDispatchEx_GetTypeInfo(&doctype->dispex.IDispatchEx_iface, index, lcid, ti);
}

static HRESULT WINAPI domdoctype_GetIDsOfNames(IXMLDOMDocumentType *iface, REFIID riid, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID *rgDispId)
{
    domdoctype *doctype = impl_from_IXMLDOMDocumentType(iface);
    return IDispatchEx_GetIDsOfNames(&doctype->dispex.IDispatchEx_iface,
        riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI domdoctype_Invoke(IXMLDOMDocumentType *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *ei, UINT *puArgErr)
{
    domdoctype *doctype = impl_from_IXMLDOMDocumentType(iface);
    return IDispatchEx_Invoke(&doctype->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, flags, params,
            result, ei, puArgErr);
}

static HRESULT WINAPI domdoctype_get_nodeName(IXMLDOMDocumentType *iface, BSTR *p)
{
    domdoctype *doctype = impl_from_IXMLDOMDocumentType(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_name(doctype->node, p);
}

static HRESULT WINAPI domdoctype_get_nodeValue(IXMLDOMDocumentType *iface, VARIANT *v)
{
    TRACE("%p, %p.\n", iface, v);

    return return_null_var(v);
}

static HRESULT WINAPI domdoctype_put_nodeValue(IXMLDOMDocumentType *iface, VARIANT v)
{
    FIXME("%p, %s: stub\n", iface, debugstr_variant(&v));

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_nodeType(IXMLDOMDocumentType *iface, DOMNodeType *type)
{
    TRACE("%p, %p.\n", iface, type);

    *type = NODE_DOCUMENT_TYPE;
    return S_OK;
}

static HRESULT WINAPI domdoctype_get_parentNode(IXMLDOMDocumentType *iface, IXMLDOMNode **parent)
{
    domdoctype *doctype = impl_from_IXMLDOMDocumentType(iface);

    TRACE("%p, %p.\n", iface, parent);

    return node_get_parent(doctype->node, parent);
}

static HRESULT WINAPI domdoctype_get_childNodes(IXMLDOMDocumentType *iface, IXMLDOMNodeList **list)
{
    FIXME("%p, %p: stub\n", iface, list);

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_firstChild(IXMLDOMDocumentType *iface, IXMLDOMNode **node)
{
    FIXME("%p, %p: stub\n", iface, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_lastChild(IXMLDOMDocumentType *iface, IXMLDOMNode **node)
{
    FIXME("%p, %p: stub\n", iface, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_previousSibling(IXMLDOMDocumentType *iface, IXMLDOMNode **node)
{
    FIXME("%p, %p: stub\n", iface, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_nextSibling(IXMLDOMDocumentType *iface, IXMLDOMNode **node)
{
    FIXME("%p, %p: stub\n", iface, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_attributes(IXMLDOMDocumentType *iface, IXMLDOMNamedNodeMap **map)
{
    FIXME("%p, %p: stub\n", iface, map);

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_insertBefore(IXMLDOMDocumentType *iface, IXMLDOMNode *node,
        VARIANT refChild, IXMLDOMNode **out_node)
{
    FIXME("%p, %p, %s, %p: stub\n", iface, node, debugstr_variant(&refChild), out_node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_replaceChild(IXMLDOMDocumentType *iface, IXMLDOMNode *newNode,
        IXMLDOMNode *oldNode, IXMLDOMNode **outOldNode)
{
    FIXME("%p, %p, %p, %p: stub\n", iface, newNode, oldNode, outOldNode);

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_removeChild(IXMLDOMDocumentType *iface, IXMLDOMNode *node,
        IXMLDOMNode **oldNode)
{
    FIXME("%p, %p, %p: stub\n", iface, node, oldNode);

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_appendChild(IXMLDOMDocumentType *iface, IXMLDOMNode *newNode,
        IXMLDOMNode **outNewNode)
{
    FIXME("%p, %p, %p: stub\n", iface, newNode, outNewNode);

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_hasChildNodes(IXMLDOMDocumentType *iface, VARIANT_BOOL *v)
{
    FIXME("%p, %p: stub\n", iface, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_ownerDocument(IXMLDOMDocumentType *iface, IXMLDOMDocument **doc)
{
    domdoctype *doctype = impl_from_IXMLDOMDocumentType(iface);

    TRACE("%p, %p.\n", iface, doc);

    return node_get_owner_document(doctype->node, doc);
}

static HRESULT WINAPI domdoctype_cloneNode(IXMLDOMDocumentType *iface, VARIANT_BOOL deep, IXMLDOMNode **node)
{
    FIXME("%p, %d, %p: stub\n", iface, deep, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_nodeTypeString(IXMLDOMDocumentType *iface, BSTR *p)
{
    TRACE("%p, %p.\n", iface, p);

    return return_bstr(L"documenttype", p);
}

static HRESULT WINAPI domdoctype_get_text(IXMLDOMDocumentType *iface, BSTR *p)
{
    TRACE("%p, %p.\n", iface, p);

    return return_bstr(L"", p);
}

static HRESULT WINAPI domdoctype_put_text(IXMLDOMDocumentType *iface, BSTR p)
{
    FIXME("%p, %s: stub\n", iface, debugstr_w(p));

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_specified(IXMLDOMDocumentType *iface, VARIANT_BOOL *v)
{
    FIXME("%p, %p: stub\n", iface, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_definition(IXMLDOMDocumentType *iface, IXMLDOMNode **node)
{
    FIXME("%p, %p\n", iface, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_nodeTypedValue(IXMLDOMDocumentType *iface, VARIANT *v)
{
    TRACE("%p, %p.\n", iface, v);

    return return_null_var(v);
}

static HRESULT WINAPI domdoctype_put_nodeTypedValue(IXMLDOMDocumentType *iface, VARIANT value)
{
    FIXME("%p, %s: stub\n", iface, debugstr_variant(&value));

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_dataType(IXMLDOMDocumentType *iface, VARIANT *v)
{
    FIXME("%p, %p: stub\n", iface, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_put_dataType(IXMLDOMDocumentType *iface, BSTR p)
{
    FIXME("%p, %s: stub\n", iface, debugstr_w(p));

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_xml(IXMLDOMDocumentType *iface, BSTR *p)
{
    domdoctype *doctype = impl_from_IXMLDOMDocumentType(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_xml(doctype->node, p);
}

static HRESULT WINAPI domdoctype_transformNode(IXMLDOMDocumentType *iface, IXMLDOMNode *node, BSTR *p)
{
    FIXME("%p, %p, %p: stub\n", iface, node, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_selectNodes(IXMLDOMDocumentType *iface, BSTR p, IXMLDOMNodeList **list)
{
    FIXME("%p, %s, %p: stub\n", iface, debugstr_w(p), list);

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_selectSingleNode(IXMLDOMDocumentType *iface, BSTR p, IXMLDOMNode **node)
{
    FIXME("%p, %s, %p: stub\n", iface, debugstr_w(p), node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_parsed(IXMLDOMDocumentType *iface, VARIANT_BOOL *v)
{
    FIXME("%p, %p: stub\n", iface, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_namespaceURI(IXMLDOMDocumentType *iface, BSTR *p)
{
    FIXME("%p, %p: stub\n", iface, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_prefix(IXMLDOMDocumentType *iface, BSTR *prefix)
{
    FIXME("%p, %p: stub\n", iface, prefix);

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_baseName(IXMLDOMDocumentType *iface, BSTR *name)
{
    FIXME("%p, %p: stub\n", iface, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_transformNodeToObject(IXMLDOMDocumentType *iface, IXMLDOMNode *node, VARIANT var1)
{
    FIXME("%p, %p, %s: stub\n", iface, node, debugstr_variant(&var1));

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_name(IXMLDOMDocumentType *iface, BSTR *p)
{
    domdoctype *doctype = impl_from_IXMLDOMDocumentType(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_name(doctype->node, p);
}

static HRESULT WINAPI domdoctype_get_entities(IXMLDOMDocumentType *iface, IXMLDOMNamedNodeMap **entities)
{
    FIXME("%p, %p: stub\n", iface, entities);

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_notations(IXMLDOMDocumentType *iface, IXMLDOMNamedNodeMap **notations)
{
    FIXME("%p, %p: stub\n", iface, notations);

    return E_NOTIMPL;
}

static const struct IXMLDOMDocumentTypeVtbl domdoctype_vtbl =
{
    domdoctype_QueryInterface,
    domdoctype_AddRef,
    domdoctype_Release,
    domdoctype_GetTypeInfoCount,
    domdoctype_GetTypeInfo,
    domdoctype_GetIDsOfNames,
    domdoctype_Invoke,
    domdoctype_get_nodeName,
    domdoctype_get_nodeValue,
    domdoctype_put_nodeValue,
    domdoctype_get_nodeType,
    domdoctype_get_parentNode,
    domdoctype_get_childNodes,
    domdoctype_get_firstChild,
    domdoctype_get_lastChild,
    domdoctype_get_previousSibling,
    domdoctype_get_nextSibling,
    domdoctype_get_attributes,
    domdoctype_insertBefore,
    domdoctype_replaceChild,
    domdoctype_removeChild,
    domdoctype_appendChild,
    domdoctype_hasChildNodes,
    domdoctype_get_ownerDocument,
    domdoctype_cloneNode,
    domdoctype_get_nodeTypeString,
    domdoctype_get_text,
    domdoctype_put_text,
    domdoctype_get_specified,
    domdoctype_get_definition,
    domdoctype_get_nodeTypedValue,
    domdoctype_put_nodeTypedValue,
    domdoctype_get_dataType,
    domdoctype_put_dataType,
    domdoctype_get_xml,
    domdoctype_transformNode,
    domdoctype_selectNodes,
    domdoctype_selectSingleNode,
    domdoctype_get_parsed,
    domdoctype_get_namespaceURI,
    domdoctype_get_prefix,
    domdoctype_get_baseName,
    domdoctype_transformNodeToObject,
    domdoctype_get_name,
    domdoctype_get_entities,
    domdoctype_get_notations
};

static const tid_t domdoctype_iface_tids[] =
{
    IXMLDOMDocumentType_tid,
    0
};

static dispex_static_data_t domdoctype_dispex =
{
    NULL,
    IXMLDOMDocumentType_tid,
    NULL,
    domdoctype_iface_tids
};

HRESULT create_doc_type(struct domnode *node, IUnknown **obj)
{
    domdoctype *object;

    *obj = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IXMLDOMDocumentType_iface.lpVtbl = &domdoctype_vtbl;
    object->refcount = 1;
    object->node = domnode_addref(node);

    init_dispex(&object->dispex, (IUnknown *)&object->IXMLDOMDocumentType_iface, &domdoctype_dispex);

    *obj = (IUnknown *)&object->IXMLDOMDocumentType_iface;

    return S_OK;
}
