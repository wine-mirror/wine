/*
 *    DOM CDATA node implementation
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

typedef struct
{
    DispatchEx dispex;
    IXMLDOMCDATASection IXMLDOMCDATASection_iface;
    LONG refcount;
    struct domnode *node;
} domcdata;

static const tid_t domcdata_se_tids[] = {
    IXMLDOMNode_tid,
    IXMLDOMCDATASection_tid,
    NULL_tid
};

static inline domcdata *impl_from_IXMLDOMCDATASection( IXMLDOMCDATASection *iface )
{
    return CONTAINING_RECORD(iface, domcdata, IXMLDOMCDATASection_iface);
}

static HRESULT WINAPI domcdata_QueryInterface(IXMLDOMCDATASection *iface, REFIID riid, void **obj)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if ( IsEqualGUID( riid, &IID_IXMLDOMCDATASection ) ||
         IsEqualGUID( riid, &IID_IXMLDOMCharacterData) ||
         IsEqualGUID( riid, &IID_IXMLDOMNode ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *obj = iface;
    }
    else if (dispex_query_interface(&cdata->dispex, riid, obj))
    {
        return *obj ? S_OK : E_NOINTERFACE;
    }
    else if (node_query_interface(cdata->node, riid, obj))
    {
        return *obj ? S_OK : E_NOINTERFACE;
    }
    else if (IsEqualGUID(riid, &IID_ISupportErrorInfo))
    {
        return node_create_supporterrorinfo(domcdata_se_tids, obj);
    }
    else
    {
        WARN("Unsupported interface %s\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IXMLDOMCDATASection_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI domcdata_AddRef(IXMLDOMCDATASection *iface)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);
    ULONG ref = InterlockedIncrement(&cdata->refcount);
    TRACE("%p, refcount %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI domcdata_Release(IXMLDOMCDATASection *iface)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);
    ULONG ref = InterlockedDecrement(&cdata->refcount);

    TRACE("%p, refcount %lu.\n", iface, ref);

    if (!ref)
    {
        domnode_release(cdata->node);
        free(cdata);
    }

    return ref;
}

static HRESULT WINAPI domcdata_GetTypeInfoCount(IXMLDOMCDATASection *iface, UINT *count)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);
    return IDispatchEx_GetTypeInfoCount(&cdata->dispex.IDispatchEx_iface, count);
}

static HRESULT WINAPI domcdata_GetTypeInfo(IXMLDOMCDATASection *iface, UINT index, LCID lcid, ITypeInfo **ti)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);
    return IDispatchEx_GetTypeInfo(&cdata->dispex.IDispatchEx_iface, index, lcid, ti);
}

static HRESULT WINAPI domcdata_GetIDsOfNames(IXMLDOMCDATASection *iface, REFIID riid, LPOLESTR* rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection( iface );
    return IDispatchEx_GetIDsOfNames(&cdata->dispex.IDispatchEx_iface, riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI domcdata_Invoke(IXMLDOMCDATASection *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *ei, UINT *puArgErr)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);
    return IDispatchEx_Invoke(&cdata->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, flags,
            params, result, ei, puArgErr);
}

static HRESULT WINAPI domcdata_get_nodeName(IXMLDOMCDATASection *iface, BSTR *p)
{
    TRACE("%p, %p.\n", iface, p);

    return return_bstr(L"#cdata-section", p);
}

static HRESULT WINAPI domcdata_get_nodeValue(IXMLDOMCDATASection *iface, VARIANT *value)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);

    TRACE("%p, %p.\n", iface, value);

    return node_get_value(cdata->node, value);
}

static HRESULT WINAPI domcdata_put_nodeValue(IXMLDOMCDATASection *iface, VARIANT value)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection( iface );

    TRACE("%p, %s.\n", iface, debugstr_variant(&value));

    return node_put_value(cdata->node, &value);
}

static HRESULT WINAPI domcdata_get_nodeType(IXMLDOMCDATASection *iface, DOMNodeType *type)
{
    TRACE("%p, %p.\n", iface, type);

    *type = NODE_CDATA_SECTION;
    return S_OK;
}

static HRESULT WINAPI domcdata_get_parentNode(IXMLDOMCDATASection *iface, IXMLDOMNode **parent)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);

    TRACE("%p, %p.\n", iface, parent);

    return node_get_parent(cdata->node, parent);
}

static HRESULT WINAPI domcdata_get_childNodes(IXMLDOMCDATASection *iface, IXMLDOMNodeList **list)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);

    TRACE("%p, %p.\n", iface, list);

    return node_get_child_nodes(cdata->node, list);
}

static HRESULT WINAPI domcdata_get_firstChild(IXMLDOMCDATASection *iface, IXMLDOMNode **node)
{
    TRACE("%p, %p.\n", iface, node);

    return return_null_node(node);
}

static HRESULT WINAPI domcdata_get_lastChild(IXMLDOMCDATASection *iface, IXMLDOMNode **node)
{
    TRACE("%p, %p.\n", iface, node);

    return return_null_node(node);
}

static HRESULT WINAPI domcdata_get_previousSibling(IXMLDOMCDATASection *iface, IXMLDOMNode **node)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);

    TRACE("%p, %p.\n", iface, node);

    return node_get_previous_sibling(cdata->node, node);
}

static HRESULT WINAPI domcdata_get_nextSibling(IXMLDOMCDATASection *iface, IXMLDOMNode **node)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);

    TRACE("%p, %p.\n", iface, node);

    return node_get_next_sibling(cdata->node, node);
}

static HRESULT WINAPI domcdata_get_attributes(IXMLDOMCDATASection *iface, IXMLDOMNamedNodeMap **map)
{
    TRACE("%p, %p.\n", iface, map);

    return return_null_ptr((void **)map);
}

static HRESULT WINAPI domcdata_insertBefore(IXMLDOMCDATASection *iface, IXMLDOMNode *node,
        VARIANT refChild, IXMLDOMNode **oldnode)
{
    TRACE("%p, %p, %s, %p.\n", iface, node, debugstr_variant(&refChild), oldnode);

    if (oldnode) *oldnode = NULL;
    return E_FAIL;
}

static HRESULT WINAPI domcdata_replaceChild(IXMLDOMCDATASection *iface, IXMLDOMNode *node,
        IXMLDOMNode *oldnode, IXMLDOMNode **out_oldnode)
{
    TRACE("%p, %p, %p, %p.\n", iface, node, oldnode, out_oldnode);

    if (out_oldnode) *out_oldnode = NULL;
    return E_FAIL;
}

static HRESULT WINAPI domcdata_removeChild(IXMLDOMCDATASection *iface, IXMLDOMNode *child, IXMLDOMNode **oldChild)
{
    TRACE("%p, %p, %p.\n", iface, child, oldChild);

    if (oldChild) *oldChild = NULL;
    return E_FAIL;
}

static HRESULT WINAPI domcdata_appendChild(IXMLDOMCDATASection *iface, IXMLDOMNode *child, IXMLDOMNode **outChild)
{
    TRACE("%p, %p, %p.\n", iface, child, outChild);

    if (outChild) *outChild = NULL;
    return E_FAIL;
}

static HRESULT WINAPI domcdata_hasChildNodes(IXMLDOMCDATASection *iface, VARIANT_BOOL *ret)
{
    TRACE("%p, %p.\n", iface, ret);

    return return_var_false(ret);
}

static HRESULT WINAPI domcdata_get_ownerDocument(IXMLDOMCDATASection *iface, IXMLDOMDocument **doc)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);

    TRACE("%p, %p.\n", iface, doc);

    return node_get_owner_document(cdata->node, doc);
}

static HRESULT WINAPI domcdata_cloneNode(IXMLDOMCDATASection *iface, VARIANT_BOOL deep, IXMLDOMNode **node)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);

    TRACE("%p, %d, %p.\n", iface, deep, node);

    return node_clone(cdata->node, deep, node);
}

static HRESULT WINAPI domcdata_get_nodeTypeString(IXMLDOMCDATASection *iface, BSTR *p)
{
    TRACE("%p, %p.\n", iface, p);

    return return_bstr(L"cdatasection", p);
}

static HRESULT WINAPI domcdata_get_text(IXMLDOMCDATASection *iface, BSTR *p)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_text(cdata->node, p);
}

static HRESULT WINAPI domcdata_put_text(IXMLDOMCDATASection *iface, BSTR p)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);

    TRACE("%p, %s.\n", iface, debugstr_w(p));

    return node_put_data(cdata->node, p);
}

static HRESULT WINAPI domcdata_get_specified(IXMLDOMCDATASection *iface, VARIANT_BOOL *v)
{
    FIXME("%p, %p stub!\n", iface, v);

    *v = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI domcdata_get_definition(IXMLDOMCDATASection *iface, IXMLDOMNode **node)
{
    FIXME("%p, %p.\n", iface, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domcdata_get_nodeTypedValue(IXMLDOMCDATASection *iface, VARIANT *v)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection( iface );

    TRACE("%p, %p.\n", iface, v);

    return node_get_value(cdata->node, v);
}

static HRESULT WINAPI domcdata_put_nodeTypedValue(IXMLDOMCDATASection *iface, VARIANT v)
{
    FIXME("%p, %s.\n", iface, debugstr_variant(&v));

    return E_NOTIMPL;
}

static HRESULT WINAPI domcdata_get_dataType(IXMLDOMCDATASection *iface, VARIANT *v)
{
    TRACE("%p, %p.\n", iface, v);

    return return_null_var(v);
}

static HRESULT WINAPI domcdata_put_dataType(IXMLDOMCDATASection *iface, BSTR p)
{
    TRACE("%p, %s.\n", iface, debugstr_w(p));

    if (!p)
        return E_INVALIDARG;

    return E_FAIL;
}

static HRESULT WINAPI domcdata_get_xml(IXMLDOMCDATASection *iface, BSTR *p)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_xml(cdata->node, p);
}

static HRESULT WINAPI domcdata_transformNode(IXMLDOMCDATASection *iface, IXMLDOMNode *node, BSTR *p)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);

    TRACE("%p, %p, %p.\n", iface, node, p);

    return node_transform_node(cdata->node, node, p);
}

static HRESULT WINAPI domcdata_selectNodes(IXMLDOMCDATASection *iface, BSTR p, IXMLDOMNodeList **list)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_w(p), list);

    return node_select_nodes(cdata->node, p, list);
}

static HRESULT WINAPI domcdata_selectSingleNode(IXMLDOMCDATASection *iface, BSTR p, IXMLDOMNode **node)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_w(p), node);

    return node_select_singlenode(cdata->node, p, node);
}

static HRESULT WINAPI domcdata_get_parsed(IXMLDOMCDATASection *iface, VARIANT_BOOL *v)
{
    FIXME("%p, %p stub!\n", iface, v);

    *v = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI domcdata_get_namespaceURI(IXMLDOMCDATASection *iface, BSTR *p)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_namespaceURI(cdata->node, p);
}

static HRESULT WINAPI domcdata_get_prefix(IXMLDOMCDATASection *iface, BSTR *prefix)
{
    TRACE("%p, %p.\n", iface, prefix);

    return return_null_bstr(prefix);
}

static HRESULT WINAPI domcdata_get_baseName(IXMLDOMCDATASection *iface, BSTR *name)
{
    FIXME("%p, %p: needs test\n", iface, name);

    return return_null_bstr(name);
}

static HRESULT WINAPI domcdata_transformNodeToObject(IXMLDOMCDATASection *iface, IXMLDOMNode *node, VARIANT var1)
{
    FIXME("%p, %p, %s.\n", iface, node, debugstr_variant(&var1));

    return E_NOTIMPL;
}

static HRESULT WINAPI domcdata_get_data(IXMLDOMCDATASection *iface, BSTR *p)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_data(cdata->node, p);
}

static HRESULT WINAPI domcdata_put_data(IXMLDOMCDATASection *iface, BSTR data)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);

    TRACE("%p, %s.\n", iface, debugstr_w(data));

    return node_put_data(cdata->node, data);
}

static HRESULT WINAPI domcdata_get_length(IXMLDOMCDATASection *iface, LONG *length)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);

    TRACE("%p, %p.\n", iface, length);

    return node_get_data_length(cdata->node, length);
}

static HRESULT WINAPI domcdata_substringData(IXMLDOMCDATASection *iface, LONG offset, LONG count, BSTR *p)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);

    TRACE("%p, %ld, %ld, %p.\n", iface, offset, count, p);

    return node_substring_data(cdata->node, offset, count, p);
}

static HRESULT WINAPI domcdata_appendData(IXMLDOMCDATASection *iface, BSTR p)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);

    TRACE("%p, %s.\n", iface, debugstr_w(p));

    return node_append_data(cdata->node, p);
}

static HRESULT WINAPI domcdata_insertData(IXMLDOMCDATASection *iface, LONG offset, BSTR p)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);

    TRACE("%p, %ld, %s.\n", iface, offset, debugstr_w(p));

    return node_insert_data(cdata->node, offset, p);
}

static HRESULT WINAPI domcdata_deleteData(IXMLDOMCDATASection *iface, LONG offset, LONG count)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);

    TRACE("%p, %ld, %ld.\n", iface, offset, count);

    return node_delete_data(cdata->node, offset, count);
}

static HRESULT WINAPI domcdata_replaceData(
    IXMLDOMCDATASection *iface,
    LONG offset, LONG count, BSTR p)
{
    HRESULT hr;

    TRACE("%p, %ld, %ld, %s.\n", iface, offset, count, debugstr_w(p));

    hr = IXMLDOMCDATASection_deleteData(iface, offset, count);

    if (hr == S_OK)
       hr = IXMLDOMCDATASection_insertData(iface, offset, p);

    return hr;
}

static HRESULT WINAPI domcdata_splitText(IXMLDOMCDATASection *iface, LONG offset, IXMLDOMText **node)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);

    TRACE("%p, %ld, %p.\n", iface, offset, node);

    return node_split_text(cdata->node, offset, node);
}

static const struct IXMLDOMCDATASectionVtbl domcdata_vtbl =
{
    domcdata_QueryInterface,
    domcdata_AddRef,
    domcdata_Release,
    domcdata_GetTypeInfoCount,
    domcdata_GetTypeInfo,
    domcdata_GetIDsOfNames,
    domcdata_Invoke,
    domcdata_get_nodeName,
    domcdata_get_nodeValue,
    domcdata_put_nodeValue,
    domcdata_get_nodeType,
    domcdata_get_parentNode,
    domcdata_get_childNodes,
    domcdata_get_firstChild,
    domcdata_get_lastChild,
    domcdata_get_previousSibling,
    domcdata_get_nextSibling,
    domcdata_get_attributes,
    domcdata_insertBefore,
    domcdata_replaceChild,
    domcdata_removeChild,
    domcdata_appendChild,
    domcdata_hasChildNodes,
    domcdata_get_ownerDocument,
    domcdata_cloneNode,
    domcdata_get_nodeTypeString,
    domcdata_get_text,
    domcdata_put_text,
    domcdata_get_specified,
    domcdata_get_definition,
    domcdata_get_nodeTypedValue,
    domcdata_put_nodeTypedValue,
    domcdata_get_dataType,
    domcdata_put_dataType,
    domcdata_get_xml,
    domcdata_transformNode,
    domcdata_selectNodes,
    domcdata_selectSingleNode,
    domcdata_get_parsed,
    domcdata_get_namespaceURI,
    domcdata_get_prefix,
    domcdata_get_baseName,
    domcdata_transformNodeToObject,
    domcdata_get_data,
    domcdata_put_data,
    domcdata_get_length,
    domcdata_substringData,
    domcdata_appendData,
    domcdata_insertData,
    domcdata_deleteData,
    domcdata_replaceData,
    domcdata_splitText
};

static const tid_t domcdata_iface_tids[] =
{
    IXMLDOMCDATASection_tid,
    0
};

static dispex_static_data_t domcdata_dispex =
{
    NULL,
    IXMLDOMCDATASection_tid,
    NULL,
    domcdata_iface_tids
};

HRESULT create_cdata(struct domnode *node, IUnknown **obj)
{
    domcdata *object;

    *obj = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IXMLDOMCDATASection_iface.lpVtbl = &domcdata_vtbl;
    object->refcount = 1;
    object->node = domnode_addref(node);

    init_dispex(&object->dispex, (IUnknown *)&object->IXMLDOMCDATASection_iface, &domcdata_dispex);

    *obj = (IUnknown *)&object->IXMLDOMCDATASection_iface;

    return S_OK;
}
