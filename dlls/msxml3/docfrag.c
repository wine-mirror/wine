/*
 *    DOM Document Fragment implementation
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

#include "config.h"

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml6.h"

#include "msxml_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

#ifdef HAVE_LIBXML2

typedef struct _domfrag
{
    xmlnode node;
    const struct IXMLDOMDocumentFragmentVtbl *lpVtbl;
    LONG ref;
} domfrag;

static inline domfrag *impl_from_IXMLDOMDocumentFragment( IXMLDOMDocumentFragment *iface )
{
    return (domfrag *)((char*)iface - FIELD_OFFSET(domfrag, lpVtbl));
}

static HRESULT WINAPI domfrag_QueryInterface(
    IXMLDOMDocumentFragment *iface,
    REFIID riid,
    void** ppvObject )
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IXMLDOMDocumentFragment ) ||
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
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IXMLDOMText_AddRef((IUnknown*)*ppvObject);
    return S_OK;
}

static ULONG WINAPI domfrag_AddRef(
    IXMLDOMDocumentFragment *iface )
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    return InterlockedIncrement( &This->ref );
}

static ULONG WINAPI domfrag_Release(
    IXMLDOMDocumentFragment *iface )
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    ULONG ref;

    ref = InterlockedDecrement( &This->ref );
    if ( ref == 0 )
    {
        destroy_xmlnode(&This->node);
        heap_free( This );
    }

    return ref;
}

static HRESULT WINAPI domfrag_GetTypeInfoCount(
    IXMLDOMDocumentFragment *iface,
    UINT* pctinfo )
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI domfrag_GetTypeInfo(
    IXMLDOMDocumentFragment *iface,
    UINT iTInfo, LCID lcid,
    ITypeInfo** ppTInfo )
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    HRESULT hr;

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(IXMLDOMDocumentFragment_tid, ppTInfo);

    return hr;
}

static HRESULT WINAPI domfrag_GetIDsOfNames(
    IXMLDOMDocumentFragment *iface,
    REFIID riid, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgDispId )
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IXMLDOMDocumentFragment_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI domfrag_Invoke(
    IXMLDOMDocumentFragment *iface,
    DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo, UINT* puArgErr )
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IXMLDOMDocumentFragment_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &(This->lpVtbl), dispIdMember, wFlags, pDispParams,
                pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI domfrag_get_nodeName(
    IXMLDOMDocumentFragment *iface,
    BSTR* p )
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );

    static const WCHAR document_fragmentW[] =
        {'#','d','o','c','u','m','e','n','t','-','f','r','a','g','m','e','n','t',0};

    TRACE("(%p)->(%p)\n", This, p);

    return return_bstr(document_fragmentW, p);
}

static HRESULT WINAPI domfrag_get_nodeValue(
    IXMLDOMDocumentFragment *iface,
    VARIANT* value)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );

    FIXME("(%p)->(%p)\n", This, value);

    if(!value)
        return E_INVALIDARG;

    V_VT(value) = VT_NULL;
    return S_FALSE;
}

static HRESULT WINAPI domfrag_put_nodeValue(
    IXMLDOMDocumentFragment *iface,
    VARIANT value)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    TRACE("(%p)->(v%d)\n", This, V_VT(&value));
    return E_FAIL;
}

static HRESULT WINAPI domfrag_get_nodeType(
    IXMLDOMDocumentFragment *iface,
    DOMNodeType* domNodeType )
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );

    TRACE("(%p)->(%p)\n", This, domNodeType);

    *domNodeType = NODE_DOCUMENT_FRAGMENT;
    return S_OK;
}

static HRESULT WINAPI domfrag_get_parentNode(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMNode** parent )
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );

    TRACE("(%p)->(%p)\n", This, parent);

    return node_get_parent(&This->node, parent);
}

static HRESULT WINAPI domfrag_get_childNodes(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMNodeList** outList)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );

    TRACE("(%p)->(%p)\n", This, outList);

    return node_get_child_nodes(&This->node, outList);
}

static HRESULT WINAPI domfrag_get_firstChild(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMNode** domNode)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_first_child(&This->node, domNode);
}

static HRESULT WINAPI domfrag_get_lastChild(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMNode** domNode)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_last_child(&This->node, domNode);
}

static HRESULT WINAPI domfrag_get_previousSibling(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMNode** domNode)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return return_null_node(domNode);
}

static HRESULT WINAPI domfrag_get_nextSibling(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMNode** domNode)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return return_null_node(domNode);
}

static HRESULT WINAPI domfrag_get_attributes(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMNamedNodeMap** attributeMap)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );

    TRACE("(%p)->(%p)\n", This, attributeMap);

    return return_null_ptr((void**)attributeMap);
}

static HRESULT WINAPI domfrag_insertBefore(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMNode* newNode, VARIANT refChild,
    IXMLDOMNode** outOldNode)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );

    TRACE("(%p)->(%p x%d %p)\n", This, newNode, V_VT(&refChild), outOldNode);

    /* TODO: test */
    return node_insert_before(&This->node, newNode, &refChild, outOldNode);
}

static HRESULT WINAPI domfrag_replaceChild(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMNode* newNode,
    IXMLDOMNode* oldNode,
    IXMLDOMNode** outOldNode)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );

    TRACE("(%p)->(%p %p %p)\n", This, newNode, oldNode, outOldNode);

    /* TODO: test */
    return node_replace_child(&This->node, newNode, oldNode, outOldNode);
}

static HRESULT WINAPI domfrag_removeChild(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMNode* domNode, IXMLDOMNode** oldNode)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    return IXMLDOMNode_removeChild( IXMLDOMNode_from_impl(&This->node), domNode, oldNode );
}

static HRESULT WINAPI domfrag_appendChild(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMNode* newNode, IXMLDOMNode** outNewNode)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    return IXMLDOMNode_appendChild( IXMLDOMNode_from_impl(&This->node), newNode, outNewNode );
}

static HRESULT WINAPI domfrag_hasChildNodes(
    IXMLDOMDocumentFragment *iface,
    VARIANT_BOOL* pbool)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    return IXMLDOMNode_hasChildNodes( IXMLDOMNode_from_impl(&This->node), pbool );
}

static HRESULT WINAPI domfrag_get_ownerDocument(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMDocument** domDocument)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    return IXMLDOMNode_get_ownerDocument( IXMLDOMNode_from_impl(&This->node), domDocument );
}

static HRESULT WINAPI domfrag_cloneNode(
    IXMLDOMDocumentFragment *iface,
    VARIANT_BOOL deep, IXMLDOMNode** outNode)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    TRACE("(%p)->(%d %p)\n", This, deep, outNode);
    return node_clone( &This->node, deep, outNode );
}

static HRESULT WINAPI domfrag_get_nodeTypeString(
    IXMLDOMDocumentFragment *iface,
    BSTR* p)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    return IXMLDOMNode_get_nodeTypeString( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI domfrag_get_text(
    IXMLDOMDocumentFragment *iface,
    BSTR* p)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    return IXMLDOMNode_get_text( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI domfrag_put_text(
    IXMLDOMDocumentFragment *iface,
    BSTR p)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    TRACE("(%p)->(%s)\n", This, debugstr_w(p));
    return node_put_text( &This->node, p );
}

static HRESULT WINAPI domfrag_get_specified(
    IXMLDOMDocumentFragment *iface,
    VARIANT_BOOL* isSpecified)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    FIXME("(%p)->(%p) stub!\n", This, isSpecified);
    *isSpecified = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI domfrag_get_definition(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMNode** definitionNode)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    FIXME("(%p)->(%p)\n", This, definitionNode);
    return E_NOTIMPL;
}

static HRESULT WINAPI domfrag_get_nodeTypedValue(
    IXMLDOMDocumentFragment *iface,
    VARIANT* var1)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    return IXMLDOMNode_get_nodeTypedValue( IXMLDOMNode_from_impl(&This->node), var1 );
}

static HRESULT WINAPI domfrag_put_nodeTypedValue(
    IXMLDOMDocumentFragment *iface,
    VARIANT var1)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    return IXMLDOMNode_put_nodeTypedValue( IXMLDOMNode_from_impl(&This->node), var1 );
}

static HRESULT WINAPI domfrag_get_dataType(
    IXMLDOMDocumentFragment *iface,
    VARIANT* typename)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    TRACE("(%p)->(%p)\n", This, typename);
    return return_null_var( typename );
}

static HRESULT WINAPI domfrag_put_dataType(
    IXMLDOMDocumentFragment *iface,
    BSTR p)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    return IXMLDOMNode_put_dataType( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI domfrag_get_xml(
    IXMLDOMDocumentFragment *iface,
    BSTR* p)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );

    TRACE("(%p)->(%p)\n", This, p);

    return node_get_xml(&This->node, FALSE, FALSE, p);
}

static HRESULT WINAPI domfrag_transformNode(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMNode* domNode, BSTR* p)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    return IXMLDOMNode_transformNode( IXMLDOMNode_from_impl(&This->node), domNode, p );
}

static HRESULT WINAPI domfrag_selectNodes(
    IXMLDOMDocumentFragment *iface,
    BSTR p, IXMLDOMNodeList** outList)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    return IXMLDOMNode_selectNodes( IXMLDOMNode_from_impl(&This->node), p, outList );
}

static HRESULT WINAPI domfrag_selectSingleNode(
    IXMLDOMDocumentFragment *iface,
    BSTR p, IXMLDOMNode** outNode)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    return IXMLDOMNode_selectSingleNode( IXMLDOMNode_from_impl(&This->node), p, outNode );
}

static HRESULT WINAPI domfrag_get_parsed(
    IXMLDOMDocumentFragment *iface,
    VARIANT_BOOL* isParsed)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    FIXME("(%p)->(%p) stub!\n", This, isParsed);
    *isParsed = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI domfrag_get_namespaceURI(
    IXMLDOMDocumentFragment *iface,
    BSTR* p)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    return IXMLDOMNode_get_namespaceURI( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI domfrag_get_prefix(
    IXMLDOMDocumentFragment *iface,
    BSTR* prefix)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    TRACE("(%p)->(%p)\n", This, prefix);
    return return_null_bstr( prefix );
}

static HRESULT WINAPI domfrag_get_baseName(
    IXMLDOMDocumentFragment *iface,
    BSTR* name)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    FIXME("(%p)->(%p): needs test\n", This, name);
    return return_null_bstr( name );
}

static HRESULT WINAPI domfrag_transformNodeToObject(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMNode* domNode, VARIANT var1)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    return IXMLDOMNode_transformNodeToObject( IXMLDOMNode_from_impl(&This->node), domNode, var1 );
}

static const struct IXMLDOMDocumentFragmentVtbl domfrag_vtbl =
{
    domfrag_QueryInterface,
    domfrag_AddRef,
    domfrag_Release,
    domfrag_GetTypeInfoCount,
    domfrag_GetTypeInfo,
    domfrag_GetIDsOfNames,
    domfrag_Invoke,
    domfrag_get_nodeName,
    domfrag_get_nodeValue,
    domfrag_put_nodeValue,
    domfrag_get_nodeType,
    domfrag_get_parentNode,
    domfrag_get_childNodes,
    domfrag_get_firstChild,
    domfrag_get_lastChild,
    domfrag_get_previousSibling,
    domfrag_get_nextSibling,
    domfrag_get_attributes,
    domfrag_insertBefore,
    domfrag_replaceChild,
    domfrag_removeChild,
    domfrag_appendChild,
    domfrag_hasChildNodes,
    domfrag_get_ownerDocument,
    domfrag_cloneNode,
    domfrag_get_nodeTypeString,
    domfrag_get_text,
    domfrag_put_text,
    domfrag_get_specified,
    domfrag_get_definition,
    domfrag_get_nodeTypedValue,
    domfrag_put_nodeTypedValue,
    domfrag_get_dataType,
    domfrag_put_dataType,
    domfrag_get_xml,
    domfrag_transformNode,
    domfrag_selectNodes,
    domfrag_selectSingleNode,
    domfrag_get_parsed,
    domfrag_get_namespaceURI,
    domfrag_get_prefix,
    domfrag_get_baseName,
    domfrag_transformNodeToObject
};

IUnknown* create_doc_fragment( xmlNodePtr fragment )
{
    domfrag *This;

    This = heap_alloc( sizeof *This );
    if ( !This )
        return NULL;

    This->lpVtbl = &domfrag_vtbl;
    This->ref = 1;

    init_xmlnode(&This->node, fragment, (IXMLDOMNode*)&This->lpVtbl, NULL);

    return (IUnknown*) &This->lpVtbl;
}

#endif
