/*
 *    DOM Attribute implementation
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

typedef struct _domattr
{
    DispatchEx dispex;
    IXMLDOMAttribute IXMLDOMAttribute_iface;
    LONG refcount;
    struct domnode *node;
} domattr;

static const tid_t domattr_se_tids[] =
{
    IXMLDOMNode_tid,
    IXMLDOMAttribute_tid,
    NULL_tid
};

static inline domattr *impl_from_IXMLDOMAttribute( IXMLDOMAttribute *iface )
{
    return CONTAINING_RECORD(iface, domattr, IXMLDOMAttribute_iface);
}

static HRESULT WINAPI domattr_QueryInterface(IXMLDOMAttribute *iface, REFIID riid, void **obj)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IXMLDOMAttribute) ||
        IsEqualGUID(riid, &IID_IXMLDOMNode) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        *obj = iface;
    }
    else if (dispex_query_interface(&attr->dispex, riid, obj))
    {
        return *obj ? S_OK : E_NOINTERFACE;
    }
    else if (node_query_interface(attr->node, riid, obj))
    {
        return *obj ? S_OK : E_NOINTERFACE;
    }
    else if (IsEqualGUID(riid, &IID_ISupportErrorInfo))
    {
        return node_create_supporterrorinfo(domattr_se_tids, obj);
    }
    else
    {
        TRACE("Unsupported interface %s\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IXMLDOMAttribute_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI domattr_AddRef(IXMLDOMAttribute *iface)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);
    ULONG refcount = InterlockedIncrement(&attr->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI domattr_Release(IXMLDOMAttribute *iface)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);
    ULONG refcount = InterlockedDecrement(&attr->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        domnode_release(attr->node);
        free(attr);
    }

    return refcount;
}

static HRESULT WINAPI domattr_GetTypeInfoCount(IXMLDOMAttribute *iface, UINT *count)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);
    return IDispatchEx_GetTypeInfoCount(&attr->dispex.IDispatchEx_iface, count);
}

static HRESULT WINAPI domattr_GetTypeInfo(IXMLDOMAttribute *iface, UINT index, LCID lcid, ITypeInfo **ti)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);
    return IDispatchEx_GetTypeInfo(&attr->dispex.IDispatchEx_iface, index, lcid, ti);
}

static HRESULT WINAPI domattr_GetIDsOfNames(IXMLDOMAttribute *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);
    return IDispatchEx_GetIDsOfNames(&attr->dispex.IDispatchEx_iface,
        riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI domattr_Invoke(IXMLDOMAttribute *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD flags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT *puArgErr)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);
    return IDispatchEx_Invoke(&attr->dispex.IDispatchEx_iface,
        dispIdMember, riid, lcid, flags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI domattr_get_nodeName(IXMLDOMAttribute *iface, BSTR *p)
{
    domattr *attr = impl_from_IXMLDOMAttribute( iface );

    TRACE("%p, %p.\n", iface, p);

    return node_get_name(attr->node, p);
}

static HRESULT WINAPI domattr_get_nodeValue(IXMLDOMAttribute *iface, VARIANT *value)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);

    TRACE("%p, %p.\n", iface, value);

    return node_get_value(attr->node, value);
}

static HRESULT WINAPI domattr_put_nodeValue(IXMLDOMAttribute *iface, VARIANT value)
{
    domattr *attr = impl_from_IXMLDOMAttribute( iface );

    TRACE("%p, %s.\n", iface, debugstr_variant(&value));

    return node_put_value(attr->node, &value);
}

static HRESULT WINAPI domattr_get_nodeType(IXMLDOMAttribute *iface, DOMNodeType *type)
{
    TRACE("%p, %p.\n", iface, type);

    *type = NODE_ATTRIBUTE;
    return S_OK;
}

static HRESULT WINAPI domattr_get_parentNode(IXMLDOMAttribute *iface, IXMLDOMNode **parent)
{
    TRACE("%p, %p\n", iface, parent);

    return return_null_node(parent);
}

static HRESULT WINAPI domattr_get_childNodes(IXMLDOMAttribute *iface, IXMLDOMNodeList **list)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);

    TRACE("%p, %p.\n", iface, list);

    return node_get_child_nodes(attr->node, list);
}

static HRESULT WINAPI domattr_get_firstChild(IXMLDOMAttribute *iface, IXMLDOMNode **node)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);

    TRACE("%p, %p.\n", iface, node);

    return node_get_first_child(attr->node, node);
}

static HRESULT WINAPI domattr_get_lastChild(IXMLDOMAttribute *iface, IXMLDOMNode **node)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);

    TRACE("%p, %p.\n", iface, node);

    return node_get_last_child(attr->node, node);
}

static HRESULT WINAPI domattr_get_previousSibling(IXMLDOMAttribute *iface, IXMLDOMNode **node)
{
    TRACE("%p, %p.\n", iface, node);

    return return_null_node(node);
}

static HRESULT WINAPI domattr_get_nextSibling(IXMLDOMAttribute *iface, IXMLDOMNode **node)
{
    TRACE("%p, %p.\n", iface, node);

    return return_null_node(node);
}

static HRESULT WINAPI domattr_get_attributes(IXMLDOMAttribute *iface, IXMLDOMNamedNodeMap **map)
{
    TRACE("%p, %p.\n", iface, map);

    return return_null_ptr((void **)map);
}

static HRESULT WINAPI domattr_insertBefore(IXMLDOMAttribute *iface, IXMLDOMNode* newNode, VARIANT refChild,
        IXMLDOMNode** old_node)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);

    FIXME("%p, %p, %s, %p needs test\n", iface, newNode, debugstr_variant(&refChild), old_node);

    return node_insert_before(attr->node, newNode, &refChild, old_node);
}

static HRESULT WINAPI domattr_replaceChild(IXMLDOMAttribute *iface, IXMLDOMNode *newNode,
        IXMLDOMNode *oldNode, IXMLDOMNode **outOldNode)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);

    FIXME("%p, %p, %p, %p needs tests\n", iface, newNode, oldNode, outOldNode);

    return node_replace_child(attr->node, newNode, oldNode, outOldNode);
}

static HRESULT WINAPI domattr_removeChild(IXMLDOMAttribute *iface,
        IXMLDOMNode *child, IXMLDOMNode **oldChild)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);

    TRACE("%p, %p, %p.\n", iface, child, oldChild);

    return node_remove_child(attr->node, child, oldChild);
}

static HRESULT WINAPI domattr_appendChild(IXMLDOMAttribute *iface, IXMLDOMNode *child, IXMLDOMNode **outChild)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);

    TRACE("%p, %p, %p.\n", iface, child, outChild);

    return node_append_child(attr->node, child, outChild);
}

static HRESULT WINAPI domattr_hasChildNodes(IXMLDOMAttribute *iface, VARIANT_BOOL *ret)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);

    TRACE("%p, %p.\n", iface, ret);

    return node_has_childnodes(attr->node, ret);
}

static HRESULT WINAPI domattr_get_ownerDocument(IXMLDOMAttribute *iface, IXMLDOMDocument **doc)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);

    TRACE("%p, %p.\n", iface, doc);

    return node_get_owner_document(attr->node, doc);
}

static HRESULT WINAPI domattr_cloneNode(IXMLDOMAttribute *iface, VARIANT_BOOL deep, IXMLDOMNode **node)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);

    TRACE("%p, %d, %p.\n", iface, deep, node);

    return node_clone(attr->node, deep, node);
}

static HRESULT WINAPI domattr_get_nodeTypeString(IXMLDOMAttribute *iface, BSTR *p)
{
    TRACE("%p, %p.\n", iface, p);

    return return_bstr(L"attribute", p);
}

static HRESULT WINAPI domattr_get_text(IXMLDOMAttribute *iface, BSTR *p)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_text(attr->node, p);
}

static HRESULT WINAPI domattr_put_text(IXMLDOMAttribute *iface, BSTR p)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);

    TRACE("%p, %s.\n", iface, debugstr_w(p));

    return node_put_data(attr->node, p);
}

static HRESULT WINAPI domattr_get_specified(IXMLDOMAttribute *iface, VARIANT_BOOL *v)
{
    FIXME("%p, %p stub!\n", iface, v);

    *v = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI domattr_get_definition(IXMLDOMAttribute *iface, IXMLDOMNode **node)
{
    FIXME("%p, %p\n", iface, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domattr_get_nodeTypedValue(IXMLDOMAttribute *iface, VARIANT *value)
{
    IXMLDOMDocument *doc;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, value);

    hr = IXMLDOMAttribute_get_ownerDocument(iface, &doc);
    if (hr == S_OK)
    {
        IXMLDOMDocument3 *doc3;

        hr = IXMLDOMDocument_QueryInterface(doc, &IID_IXMLDOMDocument3, (void**)&doc3);
        IXMLDOMDocument_Release(doc);

        if (hr == S_OK)
        {
            VARIANT schemas;

            hr = IXMLDOMDocument3_get_schemas(doc3, &schemas);
            IXMLDOMDocument3_Release(doc3);

            if (hr != S_OK)
                return IXMLDOMAttribute_get_value(iface, value);
            else
            {
                FIXME("need to query schema for attribute type\n");
                VariantClear(&schemas);
            }
        }
    }

    return return_null_var(value);
}

static HRESULT WINAPI domattr_put_nodeTypedValue(IXMLDOMAttribute *iface, VARIANT v)
{
    FIXME("%p, %s\n", iface, debugstr_variant(&v));

    return E_NOTIMPL;
}

static HRESULT WINAPI domattr_get_dataType(IXMLDOMAttribute *iface, VARIANT *v)
{
    TRACE("%p, %p.\n", iface, v);

    return return_null_var(v);
}

static HRESULT WINAPI domattr_put_dataType(IXMLDOMAttribute *iface, BSTR p)
{
    FIXME("%p, %s.\n", iface, debugstr_w(p));

    if(!p)
        return E_INVALIDARG;

    return E_FAIL;
}

static HRESULT WINAPI domattr_get_xml(IXMLDOMAttribute *iface, BSTR *p)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_xml(attr->node, p);
}

static HRESULT WINAPI domattr_transformNode(IXMLDOMAttribute *iface, IXMLDOMNode *node, BSTR *p)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);

    TRACE("%p, %p, %p.\n", iface, node, p);

    return node_transform_node(attr->node, node, p);
}

static HRESULT WINAPI domattr_selectNodes(IXMLDOMAttribute *iface, BSTR p, IXMLDOMNodeList **list)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_w(p), list);

    return node_select_nodes(attr->node, p, list);
}

static HRESULT WINAPI domattr_selectSingleNode(IXMLDOMAttribute *iface, BSTR p, IXMLDOMNode **node)
{
    domattr *attr = impl_from_IXMLDOMAttribute( iface );

    TRACE("%p, %s, %p.\n", iface, debugstr_w(p), node);

    return node_select_singlenode(attr->node, p, node);
}

static HRESULT WINAPI domattr_get_parsed(IXMLDOMAttribute *iface, VARIANT_BOOL *v)
{
    FIXME("%p, %p stub!\n", iface, v);

    *v = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI domattr_get_namespaceURI(IXMLDOMAttribute *iface, BSTR *p)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_attribute_get_namespace_uri(attr->node, p);
}

static HRESULT WINAPI domattr_get_prefix(IXMLDOMAttribute *iface, BSTR *prefix)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);
    struct domnode *node = attr->node;

    TRACE("%p, %p.\n", iface, prefix);

    if (domdoc_version(node->owner) != MSXML6 && !wcscmp(node->name, L"xmlns"))
        return return_bstr(L"xmlns", prefix);

    return node_get_prefix(node, prefix);
}

static HRESULT WINAPI domattr_get_baseName(IXMLDOMAttribute *iface, BSTR *name)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);
    struct domnode *node = attr->node;

    TRACE("%p, %p.\n", iface, name);

    if (domdoc_version(node->owner) != MSXML6 && !wcscmp(node->name, L"xmlns"))
        return return_bstr(L"", name);

    return node_get_base_name(attr->node, name);
}

static HRESULT WINAPI domattr_transformNodeToObject(IXMLDOMAttribute *iface, IXMLDOMNode *node, VARIANT v)
{
    FIXME("%p, %p, %s.\n", iface, node, debugstr_variant(&v));

    return E_NOTIMPL;
}

static HRESULT WINAPI domattr_get_name(IXMLDOMAttribute *iface, BSTR *p)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_name(attr->node, p);
}

static HRESULT WINAPI domattr_get_value(IXMLDOMAttribute *iface, VARIANT *value)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);

    TRACE("%p, %p\n", iface, value);

    return node_get_value(attr->node, value);
}

static HRESULT WINAPI domattr_put_value(IXMLDOMAttribute *iface, VARIANT value)
{
    domattr *attr = impl_from_IXMLDOMAttribute(iface);

    TRACE("%p, %s.\n", iface, debugstr_variant(&value));

    return node_put_value(attr->node, &value);
}

static const struct IXMLDOMAttributeVtbl domattr_vtbl =
{
    domattr_QueryInterface,
    domattr_AddRef,
    domattr_Release,
    domattr_GetTypeInfoCount,
    domattr_GetTypeInfo,
    domattr_GetIDsOfNames,
    domattr_Invoke,
    domattr_get_nodeName,
    domattr_get_nodeValue,
    domattr_put_nodeValue,
    domattr_get_nodeType,
    domattr_get_parentNode,
    domattr_get_childNodes,
    domattr_get_firstChild,
    domattr_get_lastChild,
    domattr_get_previousSibling,
    domattr_get_nextSibling,
    domattr_get_attributes,
    domattr_insertBefore,
    domattr_replaceChild,
    domattr_removeChild,
    domattr_appendChild,
    domattr_hasChildNodes,
    domattr_get_ownerDocument,
    domattr_cloneNode,
    domattr_get_nodeTypeString,
    domattr_get_text,
    domattr_put_text,
    domattr_get_specified,
    domattr_get_definition,
    domattr_get_nodeTypedValue,
    domattr_put_nodeTypedValue,
    domattr_get_dataType,
    domattr_put_dataType,
    domattr_get_xml,
    domattr_transformNode,
    domattr_selectNodes,
    domattr_selectSingleNode,
    domattr_get_parsed,
    domattr_get_namespaceURI,
    domattr_get_prefix,
    domattr_get_baseName,
    domattr_transformNodeToObject,
    domattr_get_name,
    domattr_get_value,
    domattr_put_value
};

static const tid_t domattr_iface_tids[] =
{
    IXMLDOMAttribute_tid,
    0
};

static dispex_static_data_t domattr_dispex =
{
    NULL,
    IXMLDOMAttribute_tid,
    NULL,
    domattr_iface_tids
};

HRESULT create_attribute(struct domnode *node, IUnknown **obj)
{
    domattr *object;

    *obj = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IXMLDOMAttribute_iface.lpVtbl = &domattr_vtbl;
    object->refcount = 1;
    object->node = domnode_addref(node);

    init_dispex(&object->dispex, (IUnknown *)&object->IXMLDOMAttribute_iface, &domattr_dispex);

    *obj = (IUnknown *)&object->IXMLDOMAttribute_iface;

    return S_OK;
}
