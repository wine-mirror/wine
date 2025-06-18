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
#include <libxml/parser.h>
#include <libxml/xmlerror.h>

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
    xmlnode node;
    IXMLDOMProcessingInstruction IXMLDOMProcessingInstruction_iface;
    LONG ref;
} dom_pi;

static const struct nodemap_funcs dom_pi_attr_map;

static const tid_t dompi_se_tids[] = {
    IXMLDOMNode_tid,
    IXMLDOMProcessingInstruction_tid,
    NULL_tid
};

static inline dom_pi *impl_from_IXMLDOMProcessingInstruction( IXMLDOMProcessingInstruction *iface )
{
    return CONTAINING_RECORD(iface, dom_pi, IXMLDOMProcessingInstruction_iface);
}

static HRESULT WINAPI dom_pi_QueryInterface(
    IXMLDOMProcessingInstruction *iface,
    REFIID riid,
    void** ppvObject )
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IXMLDOMProcessingInstruction ) ||
         IsEqualGUID( riid, &IID_IXMLDOMNode ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else if(node_query_interface(&This->node, riid, ppvObject))
    {
        return *ppvObject ? S_OK : E_NOINTERFACE;
    }
    else if(IsEqualGUID( riid, &IID_ISupportErrorInfo ))
    {
        return node_create_supporterrorinfo(dompi_se_tids, ppvObject);
    }
    else
    {
        TRACE("Unsupported interface %s\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppvObject);
    return S_OK;
}

static ULONG WINAPI dom_pi_AddRef(
    IXMLDOMProcessingInstruction *iface )
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    ULONG ref = InterlockedIncrement( &This->ref );
    TRACE("%p, refcount %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI dom_pi_Release(
    IXMLDOMProcessingInstruction *iface )
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    ULONG ref = InterlockedDecrement( &This->ref );

    TRACE("%p, refcount %lu.\n", iface, ref);
    if ( ref == 0 )
    {
        destroy_xmlnode(&This->node);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI dom_pi_GetTypeInfoCount(
    IXMLDOMProcessingInstruction *iface,
    UINT* pctinfo )
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IDispatchEx_GetTypeInfoCount(&This->node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI dom_pi_GetTypeInfo(
    IXMLDOMProcessingInstruction *iface,
    UINT iTInfo, LCID lcid,
    ITypeInfo** ppTInfo )
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IDispatchEx_GetTypeInfo(&This->node.dispex.IDispatchEx_iface,
        iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI dom_pi_GetIDsOfNames(
    IXMLDOMProcessingInstruction *iface,
    REFIID riid, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgDispId )
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IDispatchEx_GetIDsOfNames(&This->node.dispex.IDispatchEx_iface,
        riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI dom_pi_Invoke(
    IXMLDOMProcessingInstruction *iface,
    DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo, UINT* puArgErr )
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IDispatchEx_Invoke(&This->node.dispex.IDispatchEx_iface,
        dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI dom_pi_get_nodeName(
    IXMLDOMProcessingInstruction *iface,
    BSTR* p )
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );

    TRACE("(%p)->(%p)\n", This, p);

    return node_get_nodeName(&This->node, p);
}

static HRESULT WINAPI dom_pi_get_nodeValue(
    IXMLDOMProcessingInstruction *iface,
    VARIANT* value)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );

    TRACE("(%p)->(%p)\n", This, value);

    return node_get_content(&This->node, value);
}

static HRESULT WINAPI dom_pi_put_nodeValue(
    IXMLDOMProcessingInstruction *iface,
    VARIANT value)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    BSTR target;
    HRESULT hr;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&value));

    /* Cannot set data to a PI node whose target is 'xml' */
    hr = IXMLDOMProcessingInstruction_get_nodeName(iface, &target);
    if(hr == S_OK)
    {
        if(!wcscmp(target, L"xml"))
        {
            SysFreeString(target);
            return E_FAIL;
        }

        SysFreeString(target);
    }

    return node_put_value(&This->node, &value);
}

static HRESULT WINAPI dom_pi_get_nodeType(
    IXMLDOMProcessingInstruction *iface,
    DOMNodeType* domNodeType )
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );

    TRACE("(%p)->(%p)\n", This, domNodeType);

    *domNodeType = NODE_PROCESSING_INSTRUCTION;
    return S_OK;
}

static HRESULT WINAPI dom_pi_get_parentNode(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMNode** parent )
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );

    TRACE("(%p)->(%p)\n", This, parent);

    return node_get_parent(&This->node, parent);
}

static HRESULT WINAPI dom_pi_get_childNodes(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMNodeList** outList)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );

    TRACE("(%p)->(%p)\n", This, outList);

    return node_get_child_nodes(&This->node, outList);
}

static HRESULT WINAPI dom_pi_get_firstChild(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMNode** domNode)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return return_null_node(domNode);
}

static HRESULT WINAPI dom_pi_get_lastChild(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMNode** domNode)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return return_null_node(domNode);
}

static HRESULT WINAPI dom_pi_get_previousSibling(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMNode** domNode)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_previous_sibling(&This->node, domNode);
}

static HRESULT WINAPI dom_pi_get_nextSibling(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMNode** domNode)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_next_sibling(&This->node, domNode);
}

static HRESULT xml_get_value(xmlChar **p, xmlChar **value)
{
    xmlChar *v, q;
    int len;

    while (isspace(**p)) *p += 1;
    if (**p != '=') return XML_E_MISSINGEQUALS;
    *p += 1;

    while (isspace(**p)) *p += 1;
    if (**p != '"' && **p != '\'') return XML_E_MISSINGQUOTE;
    q = **p;
    *p += 1;

    v = *p;
    while (**p && **p != q) *p += 1;
    if (!**p) return XML_E_BADCHARINSTRING;
    len = *p - v;
    if (!len) return XML_E_MISSINGNAME;
    *p += 1;

    *value = malloc(len + 1);
    if (!*value) return E_OUTOFMEMORY;
    memcpy(*value, v, len);
    *(*value + len) = 0;

    return S_OK;
}

static void set_prop(xmlNodePtr node, xmlAttrPtr attr)
{
    xmlAttrPtr prop;

    if (!node->properties)
    {
        node->properties = attr;
        return;
    }

    prop = node->properties;
    while (prop->next) prop = prop->next;

    prop->next = attr;
}

static HRESULT parse_xml_decl(xmlNodePtr node)
{
    xmlChar *version, *encoding, *standalone, *p;
    xmlAttrPtr attr;
    HRESULT hr = S_OK;

    if (!node->content) return S_OK;

    version = encoding = standalone = NULL;

    p = node->content;

    while (*p)
    {
        while (isspace(*p)) p++;
        if (!*p) break;

        if (!strncmp((const char *)p, "version", 7))
        {
            p += 7;
            if ((hr = xml_get_value(&p, &version)) != S_OK) goto fail;
        }
        else if (!strncmp((const char *)p, "encoding", 8))
        {
            p += 8;
            if ((hr = xml_get_value(&p, &encoding)) != S_OK) goto fail;
        }
        else if (!strncmp((const char *)p, "standalone", 10))
        {
            p += 10;
            if ((hr = xml_get_value(&p, &standalone)) != S_OK) goto fail;
        }
        else
        {
            FIXME("unexpected XML attribute %s\n", debugstr_a((const char *)p));
            hr = XML_E_UNEXPECTED_ATTRIBUTE;
            goto fail;
        }
    }

    /* xmlSetProp/xmlSetNsProp accept only nodes of type XML_ELEMENT_NODE,
     * so we have to create and assign attributes to a node by hand.
     */

    if (version)
    {
        attr = xmlSetNsProp(NULL, NULL, (const xmlChar *)"version", version);
        if (attr)
        {
            attr->doc = node->doc;
            set_prop(node, attr);
        }
        else hr = E_OUTOFMEMORY;
    }
    if (encoding)
    {
        attr = xmlSetNsProp(NULL, NULL, (const xmlChar *)"encoding", encoding);
        if (attr)
        {
            attr->doc = node->doc;
            set_prop(node, attr);
        }
        else hr = E_OUTOFMEMORY;
    }
    if (standalone)
    {
        attr = xmlSetNsProp(NULL, NULL, (const xmlChar *)"standalone", standalone);
        if (attr)
        {
            attr->doc = node->doc;
            set_prop(node, attr);
        }
        else hr = E_OUTOFMEMORY;
    }

fail:
    if (hr != S_OK)
    {
        xmlFreePropList(node->properties);
        node->properties = NULL;
    }

    free(version);
    free(encoding);
    free(standalone);
    return hr;
}

static HRESULT WINAPI dom_pi_get_attributes(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMNamedNodeMap** map)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    HRESULT hr;
    BSTR name;

    TRACE("(%p)->(%p)\n", This, map);

    if (!map) return E_INVALIDARG;

    *map = NULL;

    hr = node_get_nodeName(&This->node, &name);
    if (hr != S_OK) return hr;

    if (!wcscmp(name, L"xml"))
        *map = create_nodemap(This->node.node, &dom_pi_attr_map);

    SysFreeString(name);

    return *map ? S_OK : S_FALSE;
}

static HRESULT WINAPI dom_pi_insertBefore(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMNode* newNode, VARIANT refChild,
    IXMLDOMNode** outOldNode)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );

    FIXME("(%p)->(%p %s %p) needs test\n", This, newNode, debugstr_variant(&refChild), outOldNode);

    return node_insert_before(&This->node, newNode, &refChild, outOldNode);
}

static HRESULT WINAPI dom_pi_replaceChild(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMNode* newNode,
    IXMLDOMNode* oldNode,
    IXMLDOMNode** outOldNode)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );

    FIXME("(%p)->(%p %p %p) needs test\n", This, newNode, oldNode, outOldNode);

    return node_replace_child(&This->node, newNode, oldNode, outOldNode);
}

static HRESULT WINAPI dom_pi_removeChild(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMNode *child, IXMLDOMNode **oldChild)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    TRACE("(%p)->(%p %p)\n", This, child, oldChild);
    return node_remove_child(&This->node, child, oldChild);
}

static HRESULT WINAPI dom_pi_appendChild(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMNode *child, IXMLDOMNode **outChild)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    TRACE("(%p)->(%p %p)\n", This, child, outChild);
    return node_append_child(&This->node, child, outChild);
}

static HRESULT WINAPI dom_pi_hasChildNodes(
    IXMLDOMProcessingInstruction *iface,
    VARIANT_BOOL *ret)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    TRACE("(%p)->(%p)\n", This, ret);
    return node_has_childnodes(&This->node, ret);
}

static HRESULT WINAPI dom_pi_get_ownerDocument(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMDocument **doc)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    TRACE("(%p)->(%p)\n", This, doc);
    return node_get_owner_doc(&This->node, doc);
}

static HRESULT WINAPI dom_pi_cloneNode(
    IXMLDOMProcessingInstruction *iface,
    VARIANT_BOOL deep, IXMLDOMNode** outNode)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    TRACE("(%p)->(%d %p)\n", This, deep, outNode);
    return node_clone( &This->node, deep, outNode );
}

static HRESULT WINAPI dom_pi_get_nodeTypeString(
    IXMLDOMProcessingInstruction *iface,
    BSTR* p)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );

    TRACE("(%p)->(%p)\n", This, p);

    return return_bstr(L"processinginstruction", p);
}

static HRESULT WINAPI dom_pi_get_text(
    IXMLDOMProcessingInstruction *iface,
    BSTR* p)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    TRACE("(%p)->(%p)\n", This, p);
    return node_get_text(&This->node, p);
}

static HRESULT WINAPI dom_pi_put_text(
    IXMLDOMProcessingInstruction *iface,
    BSTR p)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    TRACE("(%p)->(%s)\n", This, debugstr_w(p));
    return node_put_text( &This->node, p );
}

static HRESULT WINAPI dom_pi_get_specified(
    IXMLDOMProcessingInstruction *iface,
    VARIANT_BOOL* isSpecified)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    FIXME("(%p)->(%p) stub!\n", This, isSpecified);
    *isSpecified = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI dom_pi_get_definition(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMNode** definitionNode)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    FIXME("(%p)->(%p)\n", This, definitionNode);
    return E_NOTIMPL;
}

static HRESULT WINAPI dom_pi_get_nodeTypedValue(
    IXMLDOMProcessingInstruction *iface,
    VARIANT* v)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    TRACE("(%p)->(%p)\n", This, v);
    return node_get_content(&This->node, v);
}

static HRESULT WINAPI dom_pi_put_nodeTypedValue(
    IXMLDOMProcessingInstruction *iface,
    VARIANT typedValue)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&typedValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI dom_pi_get_dataType(
    IXMLDOMProcessingInstruction *iface,
    VARIANT* typename)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    TRACE("(%p)->(%p)\n", This, typename);
    return return_null_var( typename );
}

static HRESULT WINAPI dom_pi_put_dataType(
    IXMLDOMProcessingInstruction *iface,
    BSTR p)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );

    TRACE("(%p)->(%s)\n", This, debugstr_w(p));

    if(!p)
        return E_INVALIDARG;

    return E_FAIL;
}

static HRESULT WINAPI dom_pi_get_xml(
    IXMLDOMProcessingInstruction *iface,
    BSTR* p)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );

    TRACE("(%p)->(%p)\n", This, p);

    return node_get_xml(&This->node, FALSE, p);
}

static HRESULT WINAPI dom_pi_transformNode(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMNode *node, BSTR *p)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    TRACE("(%p)->(%p %p)\n", This, node, p);
    return node_transform_node(&This->node, node, p);
}

static HRESULT WINAPI dom_pi_selectNodes(
    IXMLDOMProcessingInstruction *iface,
    BSTR p, IXMLDOMNodeList** outList)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_w(p), outList);
    return node_select_nodes(&This->node, p, outList);
}

static HRESULT WINAPI dom_pi_selectSingleNode(
    IXMLDOMProcessingInstruction *iface,
    BSTR p, IXMLDOMNode** outNode)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_w(p), outNode);
    return node_select_singlenode(&This->node, p, outNode);
}

static HRESULT WINAPI dom_pi_get_parsed(
    IXMLDOMProcessingInstruction *iface,
    VARIANT_BOOL* isParsed)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    FIXME("(%p)->(%p) stub!\n", This, isParsed);
    *isParsed = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI dom_pi_get_namespaceURI(
    IXMLDOMProcessingInstruction *iface,
    BSTR* p)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    TRACE("(%p)->(%p)\n", This, p);
    return node_get_namespaceURI(&This->node, p);
}

static HRESULT WINAPI dom_pi_get_prefix(
    IXMLDOMProcessingInstruction *iface,
    BSTR* prefix)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    TRACE("(%p)->(%p)\n", This, prefix);
    return return_null_bstr( prefix );
}

static HRESULT WINAPI dom_pi_get_baseName(
    IXMLDOMProcessingInstruction *iface,
    BSTR* name)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    TRACE("(%p)->(%p)\n", This, name);
    return node_get_base_name( &This->node, name );
}

static HRESULT WINAPI dom_pi_transformNodeToObject(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMNode* domNode, VARIANT var1)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    FIXME("(%p)->(%p %s)\n", This, domNode, debugstr_variant(&var1));
    return E_NOTIMPL;
}

static HRESULT WINAPI dom_pi_get_target(
    IXMLDOMProcessingInstruction *iface,
    BSTR *p)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );

    TRACE("(%p)->(%p)\n", This, p);

    /* target returns the same value as nodeName property */
    return node_get_nodeName(&This->node, p);
}

static HRESULT WINAPI dom_pi_get_data(
    IXMLDOMProcessingInstruction *iface,
    BSTR *p)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    HRESULT hr;
    VARIANT ret;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_INVALIDARG;

    hr = IXMLDOMProcessingInstruction_get_nodeValue( iface, &ret );
    if(hr == S_OK)
    {
        *p = V_BSTR(&ret);
    }

    return hr;
}

static HRESULT WINAPI dom_pi_put_data(
    IXMLDOMProcessingInstruction *iface,
    BSTR data)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    BSTR target;
    HRESULT hr;

    TRACE("(%p)->(%s)\n", This, debugstr_w(data) );

    /* cannot set data to a PI node whose target is 'xml' */
    hr = IXMLDOMProcessingInstruction_get_nodeName(iface, &target);
    if(hr == S_OK)
    {
        if(!wcscmp(target, L"xml"))
        {
            SysFreeString(target);
            return E_FAIL;
        }

        SysFreeString(target);
    }

    return node_set_content(&This->node, data);
}

HRESULT dom_pi_put_xml_decl(IXMLDOMNode *node, BSTR data)
{
    xmlnode *node_obj;
    HRESULT hr;
    BSTR name;

    if (!data)
        return XML_E_XMLDECLSYNTAX;

    node_obj = get_node_obj(node);
    hr = node_set_content(node_obj, data);
    if (FAILED(hr))
        return hr;

    hr = node_get_nodeName(node_obj, &name);
    if (FAILED(hr))
        return hr;

    if (!wcscmp(name, L"xml") && !node_obj->node->properties)
        hr = parse_xml_decl(node_obj->node);
    else
        hr = S_OK;

    SysFreeString(name);
    return hr;
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
    dom_pi_put_data
};

static xmlAttrPtr node_has_prop(const xmlNode *node, const xmlChar *name)
{
    xmlAttrPtr prop;

    /* xmlHasNsProp accepts only nodes of type XML_ELEMENT_NODE,
     * so we have to look for an attribute in the node by hand.
     */

    prop = node->properties;

    while (prop)
    {
        if (xmlStrEqual(prop->name, name))
            return prop;

        prop = prop->next;
    }

    return NULL;
}

static HRESULT dom_pi_get_qualified_item(const xmlNodePtr node, BSTR name, BSTR uri,
    IXMLDOMNode **item)
{
    FIXME("(%p)->(%s %s %p): stub\n", node, debugstr_w(name), debugstr_w(uri), item);
    return E_NOTIMPL;
}

static HRESULT dom_pi_get_named_item(const xmlNodePtr node, BSTR name, IXMLDOMNode **item)
{
    xmlChar *nameA;
    xmlAttrPtr attr;

    TRACE("(%p)->(%s %p)\n", node, debugstr_w(name), item);

    if (!item) return E_POINTER;

    nameA = xmlchar_from_wchar(name);
    if (!nameA) return E_OUTOFMEMORY;

    attr = node_has_prop(node, nameA);
    free(nameA);

    if (!attr)
    {
        *item = NULL;
        return S_FALSE;
    }

    *item = create_node((xmlNodePtr)attr);

    return S_OK;
}

static HRESULT dom_pi_set_named_item(xmlNodePtr node, IXMLDOMNode *newItem, IXMLDOMNode **namedItem)
{
    FIXME("(%p)->(%p %p): stub\n", node, newItem, namedItem );
    return E_NOTIMPL;
}

static HRESULT dom_pi_remove_qualified_item(xmlNodePtr node, BSTR name, BSTR uri, IXMLDOMNode **item)
{
    FIXME("(%p)->(%s %s %p): stub\n", node, debugstr_w(name), debugstr_w(uri), item);
    return E_NOTIMPL;
}

static HRESULT dom_pi_remove_named_item(xmlNodePtr node, BSTR name, IXMLDOMNode **item)
{
    FIXME("(%p)->(%s %p): stub\n", node, debugstr_w(name), item);
    return E_NOTIMPL;
}

static HRESULT dom_pi_get_item(const xmlNodePtr node, LONG index, IXMLDOMNode **item)
{
    FIXME("%p, %ld, %p: stub\n", node, index, item);
    return E_NOTIMPL;
}

static HRESULT dom_pi_get_length(const xmlNodePtr node, LONG *length)
{
    FIXME("(%p)->(%p): stub\n", node, length);

    *length = 0;
    return S_OK;
}

static HRESULT dom_pi_next_node(const xmlNodePtr node, LONG *iter, IXMLDOMNode **nextNode)
{
    FIXME("%p, %ld, %p: stub\n", node, *iter, nextNode);
    return E_NOTIMPL;
}

static const struct nodemap_funcs dom_pi_attr_map = {
    dom_pi_get_named_item,
    dom_pi_set_named_item,
    dom_pi_remove_named_item,
    dom_pi_get_item,
    dom_pi_get_length,
    dom_pi_get_qualified_item,
    dom_pi_remove_qualified_item,
    dom_pi_next_node
};

static const tid_t dompi_iface_tids[] = {
    IXMLDOMProcessingInstruction_tid,
    0
};

static dispex_static_data_t dompi_dispex = {
    NULL,
    IXMLDOMProcessingInstruction_tid,
    NULL,
    dompi_iface_tids
};

IUnknown* create_pi( xmlNodePtr pi )
{
    dom_pi *This;

    This = malloc(sizeof(*This));
    if ( !This )
        return NULL;

    This->IXMLDOMProcessingInstruction_iface.lpVtbl = &dom_pi_vtbl;
    This->ref = 1;

    init_xmlnode(&This->node, pi, (IXMLDOMNode*)&This->IXMLDOMProcessingInstruction_iface, &dompi_dispex);

    return (IUnknown*)&This->IXMLDOMProcessingInstruction_iface;
}
