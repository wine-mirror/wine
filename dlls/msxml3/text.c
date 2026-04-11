/*
 *    DOM text node implementation
 *
 * Copyright 2006 Huw Davies
 * Copyright 2007-2008 Alistair Leslie-Hughes
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

typedef struct _domtext
{
    DispatchEx dispex;
    IXMLDOMText IXMLDOMText_iface;
    LONG refcount;
    struct domnode *node;
} domtext;

static inline domtext *impl_from_IXMLDOMText( IXMLDOMText *iface )
{
    return CONTAINING_RECORD(iface, domtext, IXMLDOMText_iface);
}

static HRESULT WINAPI domtext_QueryInterface(IXMLDOMText *iface, REFIID riid, void **obj)
{
    domtext *text = impl_from_IXMLDOMText(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IXMLDOMText) ||
        IsEqualGUID(riid, &IID_IXMLDOMCharacterData) ||
        IsEqualGUID(riid, &IID_IXMLDOMNode) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IUnknown) )
    {
        *obj = iface;
    }
    else if (dispex_query_interface(&text->dispex, riid, obj))
    {
        return *obj ? S_OK : E_NOINTERFACE;
    }
    else if (node_query_interface(text->node, riid, obj))
    {
        return *obj ? S_OK : E_NOINTERFACE;
    }
    else
    {
        TRACE("Unsupported interface %s\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IXMLDOMText_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI domtext_AddRef(IXMLDOMText *iface)
{
    domtext *text = impl_from_IXMLDOMText(iface);
    ULONG refcount = InterlockedIncrement(&text->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI domtext_Release(IXMLDOMText *iface)
{
    domtext *text = impl_from_IXMLDOMText(iface);
    ULONG refcount = InterlockedDecrement(&text->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        domnode_release(text->node);
        free(text);
    }

    return refcount;
}

static HRESULT WINAPI domtext_GetTypeInfoCount(IXMLDOMText *iface, UINT *count)
{
    domtext *text = impl_from_IXMLDOMText(iface);
    return IDispatchEx_GetTypeInfoCount(&text->dispex.IDispatchEx_iface, count);
}

static HRESULT WINAPI domtext_GetTypeInfo(IXMLDOMText *iface, UINT index, LCID lcid, ITypeInfo **ti)
{
    domtext *text = impl_from_IXMLDOMText(iface);
    return IDispatchEx_GetTypeInfo(&text->dispex.IDispatchEx_iface, index, lcid, ti);
}

static HRESULT WINAPI domtext_GetIDsOfNames(IXMLDOMText *iface, REFIID riid, LPOLESTR *names,
        UINT name_count, LCID lcid, DISPID *dispid)
{
    domtext *text = impl_from_IXMLDOMText(iface);
    return IDispatchEx_GetIDsOfNames(&text->dispex.IDispatchEx_iface, riid, names, name_count, lcid, dispid);
}

static HRESULT WINAPI domtext_Invoke(
    IXMLDOMText *iface,
    DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo, UINT* puArgErr )
{
    domtext *text = impl_from_IXMLDOMText(iface);
    return IDispatchEx_Invoke(&text->dispex.IDispatchEx_iface,
        dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI domtext_get_nodeName(IXMLDOMText *iface, BSTR *p)
{
    TRACE("%p, %p.\n", iface, p);

    return return_bstr(L"#text", p);
}

static HRESULT WINAPI domtext_get_nodeValue(IXMLDOMText *iface, VARIANT *value)
{
    domtext *text = impl_from_IXMLDOMText(iface);

    TRACE("%p, %p.\n", iface, value);

    return node_get_value(text->node, value);
}

static HRESULT WINAPI domtext_put_nodeValue(IXMLDOMText *iface, VARIANT value)
{
    domtext *text = impl_from_IXMLDOMText(iface);

    TRACE("%p, %s.\n", iface, debugstr_variant(&value));

    return node_put_value(text->node, &value);
}

static HRESULT WINAPI domtext_get_nodeType(IXMLDOMText *iface, DOMNodeType *type)
{
    TRACE("%p, %p.\n", iface, type);

    *type = NODE_TEXT;
    return S_OK;
}

static HRESULT WINAPI domtext_get_parentNode(IXMLDOMText *iface, IXMLDOMNode **parent)
{
    domtext *text = impl_from_IXMLDOMText(iface);

    TRACE("%p, %p.\n", iface, parent);

    return node_get_parent(text->node, parent);
}

static HRESULT WINAPI domtext_get_childNodes(IXMLDOMText *iface, IXMLDOMNodeList **list)
{
    domtext *text = impl_from_IXMLDOMText(iface);

    TRACE("%p, %p.\n", iface, list);

    return node_get_child_nodes(text->node, list);
}

static HRESULT WINAPI domtext_get_firstChild(IXMLDOMText *iface, IXMLDOMNode **node)
{
    TRACE("%p, %p.\n", iface, node);

    return return_null_node(node);
}

static HRESULT WINAPI domtext_get_lastChild(IXMLDOMText *iface, IXMLDOMNode **node)
{
    TRACE("%p, %p.\n", iface, node);

    return return_null_node(node);
}

static HRESULT WINAPI domtext_get_previousSibling(IXMLDOMText *iface, IXMLDOMNode **node)
{
    domtext *text = impl_from_IXMLDOMText(iface);

    TRACE("%p, %p.\n", iface, node);

    return node_get_previous_sibling(text->node, node);
}

static HRESULT WINAPI domtext_get_nextSibling(IXMLDOMText *iface, IXMLDOMNode **node)
{
    domtext *text = impl_from_IXMLDOMText(iface);

    TRACE("%p, %p.\n", iface, node);

    return node_get_next_sibling(text->node, node);
}

static HRESULT WINAPI domtext_get_attributes(IXMLDOMText *iface, IXMLDOMNamedNodeMap **map)
{
    TRACE("%p, %p.\n", iface, map);

    return return_null_ptr((void **)map);
}

static HRESULT WINAPI domtext_insertBefore(IXMLDOMText *iface, IXMLDOMNode* newNode, VARIANT refChild,
    IXMLDOMNode** outOldNode)
{
    domtext *text = impl_from_IXMLDOMText(iface);

    FIXME("%p, %p, %s, %p needs test\n", iface, newNode, debugstr_variant(&refChild), outOldNode);

    return node_insert_before(text->node, newNode, &refChild, outOldNode);
}

static HRESULT WINAPI domtext_replaceChild(IXMLDOMText *iface, IXMLDOMNode *newNode,
        IXMLDOMNode *oldNode, IXMLDOMNode **outOldNode)
{
    domtext *text = impl_from_IXMLDOMText(iface);

    FIXME("%p, %p, %p, %p needs test\n", iface, newNode, oldNode, outOldNode);

    return node_replace_child(text->node, newNode, oldNode, outOldNode);
}

static HRESULT WINAPI domtext_removeChild(IXMLDOMText *iface, IXMLDOMNode *child, IXMLDOMNode **oldChild)
{
    domtext *text = impl_from_IXMLDOMText(iface);

    TRACE("%p, %p, %p.\n", iface, child, oldChild);

    return node_remove_child(text->node, child, oldChild);
}

static HRESULT WINAPI domtext_appendChild(IXMLDOMText *iface, IXMLDOMNode *child, IXMLDOMNode **outChild)
{
    domtext *text = impl_from_IXMLDOMText(iface);

    TRACE("%p, %p, %p.\n", iface, child, outChild);

    return node_append_child(text->node, child, outChild);
}

static HRESULT WINAPI domtext_hasChildNodes(IXMLDOMText *iface, VARIANT_BOOL *v)
{
    TRACE("%p, %p.\n", iface, v);

    return return_var_false(v);
}

static HRESULT WINAPI domtext_get_ownerDocument(IXMLDOMText *iface, IXMLDOMDocument **doc)
{
    domtext *text = impl_from_IXMLDOMText(iface);

    TRACE("%p, %p.\n", iface, doc);

    return node_get_owner_document(text->node, doc);
}

static HRESULT WINAPI domtext_cloneNode(IXMLDOMText *iface, VARIANT_BOOL deep, IXMLDOMNode **node)
{
    domtext *text = impl_from_IXMLDOMText(iface);

    TRACE("%p, %d, %p.\n", iface, deep, node);

    return node_clone(text->node, deep, node);
}

static HRESULT WINAPI domtext_get_nodeTypeString(IXMLDOMText *iface, BSTR *p)
{
    TRACE("%p, %p.\n", iface, p);

    return return_bstr(L"text", p);
}

static HRESULT WINAPI domtext_get_text(IXMLDOMText *iface, BSTR *p)
{
    domtext *text = impl_from_IXMLDOMText(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_text(text->node, p);
}

static HRESULT WINAPI domtext_put_text(IXMLDOMText *iface, BSTR p)
{
    domtext *text = impl_from_IXMLDOMText(iface);

    TRACE("%p, %s.\n", iface, debugstr_w(p));

    return node_put_data(text->node, p);
}

static HRESULT WINAPI domtext_get_specified(IXMLDOMText *iface, VARIANT_BOOL *v)
{
    FIXME("%p, %p stub!\n", iface, v);

    *v = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI domtext_get_definition(IXMLDOMText *iface, IXMLDOMNode **node)
{
    FIXME("%p, %p.\n", iface, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domtext_get_nodeTypedValue(IXMLDOMText *iface, VARIANT *var1)
{
    IXMLDOMNode* parent = NULL;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, var1);

    if (!var1)
        return E_INVALIDARG;

    hr = IXMLDOMText_get_parentNode(iface, &parent);

    if (hr == S_OK)
    {
        hr = IXMLDOMNode_get_nodeTypedValue(parent, var1);
        IXMLDOMNode_Release(parent);
    }
    else
    {
        V_VT(var1) = VT_NULL;
        V_BSTR(var1) = NULL;
        hr = S_FALSE;
    }

    return hr;
}

static HRESULT WINAPI domtext_put_nodeTypedValue(
    IXMLDOMText *iface,
    VARIANT value)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    IXMLDOMNode* parent = NULL;
    HRESULT hr;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&value));

    hr = IXMLDOMText_get_parentNode(iface, &parent);

    if (hr == S_OK)
    {
        hr = IXMLDOMNode_put_nodeTypedValue(parent, value);
        IXMLDOMNode_Release(parent);
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}

static HRESULT WINAPI domtext_get_dataType(
    IXMLDOMText *iface,
    VARIANT* dtName)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    IXMLDOMNode* parent = NULL;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, dtName);

    if (!dtName)
        return E_INVALIDARG;

    hr = IXMLDOMText_get_parentNode(iface, &parent);

    if (hr == S_OK)
    {
        hr = IXMLDOMNode_get_dataType(parent, dtName);
        IXMLDOMNode_Release(parent);
    }
    else
    {
        V_VT(dtName) = VT_NULL;
        V_BSTR(dtName) = NULL;
        hr = S_FALSE;
    }

    return hr;
}

static HRESULT WINAPI domtext_put_dataType(IXMLDOMText *iface, BSTR dtName)
{
    IXMLDOMNode* parent = NULL;
    HRESULT hr;

    TRACE("%p, %s.\n", iface, debugstr_w(dtName));

    if (!dtName)
        return E_INVALIDARG;

    hr = IXMLDOMText_get_parentNode(iface, &parent);

    if (hr == S_OK)
    {
        hr = IXMLDOMNode_put_dataType(parent, dtName);
        IXMLDOMNode_Release(parent);
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}

static HRESULT WINAPI domtext_get_xml(IXMLDOMText *iface, BSTR *p)
{
    domtext *text = impl_from_IXMLDOMText(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_xml(text->node, p);
}

static HRESULT WINAPI domtext_transformNode(IXMLDOMText *iface, IXMLDOMNode *node, BSTR *p)
{
    domtext *text = impl_from_IXMLDOMText(iface);

    TRACE("%p, %p, %p.\n", iface, node, p);

    return node_transform_node(text->node, node, p);
}

static HRESULT WINAPI domtext_selectNodes(IXMLDOMText *iface, BSTR p, IXMLDOMNodeList **list)
{
    domtext *text = impl_from_IXMLDOMText(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_w(p), list);

    return node_select_nodes(text->node, p, list);
}

static HRESULT WINAPI domtext_selectSingleNode(IXMLDOMText *iface, BSTR p, IXMLDOMNode **node)
{
    domtext *text = impl_from_IXMLDOMText(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_w(p), node);

    return node_select_singlenode(text->node, p, node);
}

static HRESULT WINAPI domtext_get_parsed(IXMLDOMText *iface, VARIANT_BOOL *v)
{
    FIXME("%p, %p stub!\n", iface, v);

    *v = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI domtext_get_namespaceURI(IXMLDOMText *iface, BSTR *p)
{
    domtext *text = impl_from_IXMLDOMText(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_namespaceURI(text->node, p);
}

static HRESULT WINAPI domtext_get_prefix(IXMLDOMText *iface, BSTR *p)
{
    TRACE("%p, %p.\n", iface, p);

    return return_null_bstr(p);
}

static HRESULT WINAPI domtext_get_baseName(IXMLDOMText *iface, BSTR *name)
{
    TRACE("%p, %p.\n", iface, name);

    return return_null_bstr(name);
}

static HRESULT WINAPI domtext_transformNodeToObject(IXMLDOMText *iface, IXMLDOMNode *node, VARIANT v)
{
    FIXME("%p, %p, %s.\n", iface, node, debugstr_variant(&v));

    return E_NOTIMPL;
}

static HRESULT WINAPI domtext_get_data(IXMLDOMText *iface, BSTR *p)
{
    domtext *text = impl_from_IXMLDOMText( iface );

    TRACE("%p, %p.\n", iface, p);

    return node_get_data(text->node, p);
}

static HRESULT WINAPI domtext_put_data(IXMLDOMText *iface, BSTR data)
{
    domtext *text = impl_from_IXMLDOMText(iface);

    TRACE("%p, %s.\n", iface, debugstr_w(data));

    return node_put_data(text->node, data);
}

static HRESULT WINAPI domtext_get_length(IXMLDOMText *iface, LONG *length)
{
    domtext *text = impl_from_IXMLDOMText(iface);

    TRACE("%p, %p.\n", iface, length);

    return node_get_data_length(text->node, length);
}

static HRESULT WINAPI domtext_substringData(IXMLDOMText *iface, LONG offset, LONG count, BSTR *p)
{
    domtext *text = impl_from_IXMLDOMText(iface);

    TRACE("%p, %ld, %ld, %p.\n", iface, offset, count, p);

    return node_substring_data(text->node, offset, count, p);
}

static HRESULT WINAPI domtext_appendData(IXMLDOMText *iface, BSTR p)
{
    domtext *text = impl_from_IXMLDOMText(iface);

    TRACE("%p, %s.\n", iface, debugstr_w(p));

    return node_append_data(text->node, p);
}

static HRESULT WINAPI domtext_insertData(
    IXMLDOMText *iface,
    LONG offset, BSTR p)
{
    HRESULT hr;
    BSTR data;
    LONG p_len;

    TRACE("%p, %ld, %s.\n", iface, offset, debugstr_w(p));

    /* If have a NULL or empty string, don't do anything. */
    if((p_len = SysStringLen(p)) == 0)
        return S_OK;

    if(offset < 0)
    {
        return E_INVALIDARG;
    }

    hr = IXMLDOMText_get_data(iface, &data);
    if(hr == S_OK)
    {
        LONG len = SysStringLen(data);
        BSTR str;

        if(len < offset)
        {
            SysFreeString(data);
            return E_INVALIDARG;
        }

        str = SysAllocStringLen(NULL, len + p_len);
        /* start part, supplied string and end part */
        memcpy(str, data, offset*sizeof(WCHAR));
        memcpy(&str[offset], p, p_len*sizeof(WCHAR));
        memcpy(&str[offset+p_len], &data[offset], (len-offset)*sizeof(WCHAR));
        str[len+p_len] = 0;

        hr = IXMLDOMText_put_data(iface, str);

        SysFreeString(str);
        SysFreeString(data);
    }

    return hr;
}

static HRESULT WINAPI domtext_deleteData(IXMLDOMText *iface, LONG offset, LONG count)
{
    domtext *text = impl_from_IXMLDOMText(iface);

    TRACE("%p, %ld, %ld.\n", iface, offset, count);

    return node_delete_data(text->node, offset, count);
}

static HRESULT WINAPI domtext_replaceData(
    IXMLDOMText *iface,
    LONG offset, LONG count, BSTR p)
{
    HRESULT hr;

    TRACE("%p, %ld, %ld, %s.\n", iface, offset, count, debugstr_w(p));

    hr = IXMLDOMText_deleteData(iface, offset, count);

    if (hr == S_OK)
       hr = IXMLDOMText_insertData(iface, offset, p);

    return hr;
}

static HRESULT WINAPI domtext_splitText(IXMLDOMText *iface, LONG offset, IXMLDOMText **node)
{
    domtext *text = impl_from_IXMLDOMText(iface);

    TRACE("%p, %ld, %p.\n", iface, offset, node);

    return node_split_text(text->node, offset, node);
}

static const struct IXMLDOMTextVtbl domtext_vtbl =
{
    domtext_QueryInterface,
    domtext_AddRef,
    domtext_Release,
    domtext_GetTypeInfoCount,
    domtext_GetTypeInfo,
    domtext_GetIDsOfNames,
    domtext_Invoke,
    domtext_get_nodeName,
    domtext_get_nodeValue,
    domtext_put_nodeValue,
    domtext_get_nodeType,
    domtext_get_parentNode,
    domtext_get_childNodes,
    domtext_get_firstChild,
    domtext_get_lastChild,
    domtext_get_previousSibling,
    domtext_get_nextSibling,
    domtext_get_attributes,
    domtext_insertBefore,
    domtext_replaceChild,
    domtext_removeChild,
    domtext_appendChild,
    domtext_hasChildNodes,
    domtext_get_ownerDocument,
    domtext_cloneNode,
    domtext_get_nodeTypeString,
    domtext_get_text,
    domtext_put_text,
    domtext_get_specified,
    domtext_get_definition,
    domtext_get_nodeTypedValue,
    domtext_put_nodeTypedValue,
    domtext_get_dataType,
    domtext_put_dataType,
    domtext_get_xml,
    domtext_transformNode,
    domtext_selectNodes,
    domtext_selectSingleNode,
    domtext_get_parsed,
    domtext_get_namespaceURI,
    domtext_get_prefix,
    domtext_get_baseName,
    domtext_transformNodeToObject,
    domtext_get_data,
    domtext_put_data,
    domtext_get_length,
    domtext_substringData,
    domtext_appendData,
    domtext_insertData,
    domtext_deleteData,
    domtext_replaceData,
    domtext_splitText
};

static const tid_t domtext_iface_tids[] =
{
    IXMLDOMText_tid,
    0
};

static dispex_static_data_t domtext_dispex =
{
    NULL,
    IXMLDOMText_tid,
    NULL,
    domtext_iface_tids
};

HRESULT create_text(struct domnode *node, IUnknown **obj)
{
    domtext *object;

    *obj = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IXMLDOMText_iface.lpVtbl = &domtext_vtbl;
    object->refcount = 1;
    object->node = domnode_addref(node);

    init_dispex(&object->dispex, (IUnknown *)&object->IXMLDOMText_iface, &domtext_dispex);

    *obj = (IUnknown *)&object->IXMLDOMText_iface;

    return S_OK;
}
