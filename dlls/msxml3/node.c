/*
 *    Node implementation
 *
 * Copyright 2005 Mike McCormack
 *
 * iface library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * iface library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#define COBJMACROS

#include <stdarg.h>
#include <assert.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "ole2.h"
#include "ocidl.h"
#include "msxml.h"
#include "xmldom.h"
#include "msxml.h"

#include "msxml_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

#ifdef HAVE_LIBXML2

typedef struct _xmlnode
{
    const struct IXMLDOMNodeVtbl *lpVtbl;
    LONG ref;
    xmlNodePtr node;
} xmlnode;

static inline xmlnode *impl_from_IXMLDOMNode( IXMLDOMNode *iface )
{
    return (xmlnode *)((char*)iface - FIELD_OFFSET(xmlnode, lpVtbl));
}

xmlNodePtr xmlNodePtr_from_domnode( IXMLDOMNode *iface, xmlElementType type )
{
    xmlnode *This;

    if ( !iface )
        return NULL;
    This = impl_from_IXMLDOMNode( iface );
    if ( !This->node )
        return NULL;
    if ( type && This->node->type != type )
        return NULL;
    return This->node;
}

static HRESULT WINAPI xmlnode_QueryInterface(
    IXMLDOMNode *iface,
    REFIID riid,
    void** ppvObject )
{
    TRACE("%p %p %p\n", iface, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IUnknown ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IXMLDOMNode ) )
    {
        *ppvObject = iface;
    }
    else
        return E_NOINTERFACE;

    IXMLDOMElement_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI xmlnode_AddRef(
    IXMLDOMNode *iface )
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    return InterlockedIncrement( &This->ref );
}

static ULONG WINAPI xmlnode_Release(
    IXMLDOMNode *iface )
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    ULONG ref;

    ref = InterlockedDecrement( &This->ref );
    if ( ref == 0 )
    {
        if( This->node->type == XML_DOCUMENT_NODE )
        {
            xmlFreeDoc( (xmlDocPtr) This->node );
        }
        else
        {
            IXMLDOMNode *root;
            assert( This->node->doc );
            root = This->node->doc->_private;
            assert( root );
            IXMLDOMNode_Release( root );
            This->node->_private = NULL;
        }
        HeapFree( GetProcessHeap(), 0, This );
    }

    return ref;
}

static HRESULT WINAPI xmlnode_GetTypeInfoCount(
    IXMLDOMNode *iface,
    UINT* pctinfo )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_GetTypeInfo(
    IXMLDOMNode *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo** ppTInfo )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_GetIDsOfNames(
    IXMLDOMNode *iface,
    REFIID riid,
    LPOLESTR* rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID* rgDispId )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_Invoke(
    IXMLDOMNode *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS* pDispParams,
    VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo,
    UINT* puArgErr )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_nodeName(
    IXMLDOMNode *iface,
    BSTR* name)
{
    FIXME("\n");
    return E_NOTIMPL;
}

BSTR bstr_from_xmlChar( const xmlChar *buf )
{
    DWORD len;
    LPWSTR str;
    BSTR bstr;

    if ( !buf )
        return NULL;

    len = MultiByteToWideChar( CP_UTF8, 0, (LPCSTR) buf, -1, NULL, 0 );
    str = (LPWSTR) HeapAlloc( GetProcessHeap(), 0, len * sizeof (WCHAR) );
    if ( !str )
        return NULL;
    MultiByteToWideChar( CP_UTF8, 0, (LPCSTR) buf, -1, str, len );
    bstr = SysAllocString( str );
    HeapFree( GetProcessHeap(), 0, str );
    return bstr;
}

static HRESULT WINAPI xmlnode_get_nodeValue(
    IXMLDOMNode *iface,
    VARIANT* value)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    HRESULT r = S_FALSE;

    TRACE("%p %p\n", This, value);

    V_BSTR(value) = NULL;
    V_VT(value) = VT_NULL;

    switch ( This->node->type )
    {
    case XML_ATTRIBUTE_NODE:
        V_VT(value) = VT_BSTR;
        V_BSTR(value) = bstr_from_xmlChar( This->node->name );
        r = S_OK;
        break;
    case XML_TEXT_NODE:
        V_VT(value) = VT_BSTR;
        V_BSTR(value) = bstr_from_xmlChar( This->node->content );
        r = S_OK;
        break;
    case XML_ELEMENT_NODE:
    case XML_DOCUMENT_NODE:
        /* these seem to return NULL */
        break;
    case XML_PI_NODE:
    default:
        FIXME("node %p type %d\n", This, This->node->type);
    }
 
    TRACE("%p returned %s\n", This, debugstr_w( V_BSTR(value) ) );

    return r;
}

static HRESULT WINAPI xmlnode_put_nodeValue(
    IXMLDOMNode *iface,
    VARIANT value)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_nodeType(
    IXMLDOMNode *iface,
    DOMNodeType* type)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );

    TRACE("%p %p\n", This, type);

    assert( NODE_ELEMENT == XML_ELEMENT_NODE );
    assert( NODE_NOTATION == XML_NOTATION_NODE );

    *type = This->node->type;

    return S_OK;
}

static HRESULT WINAPI xmlnode_get_parentNode(
    IXMLDOMNode *iface,
    IXMLDOMNode** parent)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_childNodes(
    IXMLDOMNode *iface,
    IXMLDOMNodeList** childList)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );

    TRACE("%p %p\n", This, childList );

    if ( !childList )
        return E_INVALIDARG;
#if 0
    *childList = create_nodelist( This->node->children );
    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

static HRESULT WINAPI xmlnode_get_firstChild(
    IXMLDOMNode *iface,
    IXMLDOMNode** firstChild)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_lastChild(
    IXMLDOMNode *iface,
    IXMLDOMNode** lastChild)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_previousSibling(
    IXMLDOMNode *iface,
    IXMLDOMNode** previousSibling)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_nextSibling(
    IXMLDOMNode *iface,
    IXMLDOMNode** nextSibling)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_attributes(
    IXMLDOMNode *iface,
    IXMLDOMNamedNodeMap** attributeMap)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    TRACE("%p\n", This);
    *attributeMap = create_nodemap( iface );
    return S_OK;
}

static HRESULT WINAPI xmlnode_insertBefore(
    IXMLDOMNode *iface,
    IXMLDOMNode* newChild,
    VARIANT refChild,
    IXMLDOMNode** outNewChild)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_replaceChild(
    IXMLDOMNode *iface,
    IXMLDOMNode* newChild,
    IXMLDOMNode* oldChild,
    IXMLDOMNode** outOldChild)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_removeChild(
    IXMLDOMNode *iface,
    IXMLDOMNode* childNode,
    IXMLDOMNode** oldChild)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_appendChild(
    IXMLDOMNode *iface,
    IXMLDOMNode* newChild,
    IXMLDOMNode** outNewChild)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_hasChildNodes(
    IXMLDOMNode *iface,
    VARIANT_BOOL* hasChild)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_ownerDocument(
    IXMLDOMNode *iface,
    IXMLDOMDocument** DOMDocument)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_cloneNode(
    IXMLDOMNode *iface,
    VARIANT_BOOL deep,
    IXMLDOMNode** cloneRoot)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_nodeTypeString(
    IXMLDOMNode *iface,
    BSTR* xmlnodeType)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_text(
    IXMLDOMNode *iface,
    BSTR* text)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    xmlNodePtr child;
    BSTR str = NULL;

    TRACE("%p\n", This);

    if ( !text )
        return E_INVALIDARG;

    child = This->node->children;
    if ( child && child->type == XML_TEXT_NODE )
        str = bstr_from_xmlChar( child->content );

    TRACE("%p %s\n", This, debugstr_w(str) );
    *text = str;
 
    return S_OK;
}

static HRESULT WINAPI xmlnode_put_text(
    IXMLDOMNode *iface,
    BSTR text)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_specified(
    IXMLDOMNode *iface,
    VARIANT_BOOL* isSpecified)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_definition(
    IXMLDOMNode *iface,
    IXMLDOMNode** definitionNode)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_nodeTypedValue(
    IXMLDOMNode *iface,
    VARIANT* typedValue)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_put_nodeTypedValue(
    IXMLDOMNode *iface,
    VARIANT typedValue)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_dataType(
    IXMLDOMNode *iface,
    VARIANT* dataTypeName)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_put_dataType(
    IXMLDOMNode *iface,
    BSTR dataTypeName)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_xml(
    IXMLDOMNode *iface,
    BSTR* xmlString)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_transformNode(
    IXMLDOMNode *iface,
    IXMLDOMNode* styleSheet,
    BSTR* xmlString)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_selectNodes(
    IXMLDOMNode *iface,
    BSTR queryString,
    IXMLDOMNodeList** resultList)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_selectSingleNode(
    IXMLDOMNode *iface,
    BSTR queryString,
    IXMLDOMNode** resultNode)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_parsed(
    IXMLDOMNode *iface,
    VARIANT_BOOL* isParsed)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_namespaceURI(
    IXMLDOMNode *iface,
    BSTR* namespaceURI)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_prefix(
    IXMLDOMNode *iface,
    BSTR* prefixString)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_baseName(
    IXMLDOMNode *iface,
    BSTR* nameString)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    BSTR str = NULL;
    HRESULT r = S_FALSE;

    TRACE("%p %p\n", This, nameString );

    if ( !nameString )
        return E_INVALIDARG;

    switch ( This->node->type )
    {
    case XML_ELEMENT_NODE:
    case XML_ATTRIBUTE_NODE:
        str = bstr_from_xmlChar( This->node->name );
        r = S_OK;
        break;
    case XML_TEXT_NODE:
        break;
    default:
        ERR("Unhandled type %d\n", This->node->type );
        break;
    }

    TRACE("returning %08lx str = %s\n", r, debugstr_w( str ) );

    *nameString = str;
    return r;
}

static HRESULT WINAPI xmlnode_transformNodeToObject(
    IXMLDOMNode *iface,
    IXMLDOMNode* stylesheet,
    VARIANT outputObject)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const struct IXMLDOMNodeVtbl xmlnode_vtbl =
{
    xmlnode_QueryInterface,
    xmlnode_AddRef,
    xmlnode_Release,
    xmlnode_GetTypeInfoCount,
    xmlnode_GetTypeInfo,
    xmlnode_GetIDsOfNames,
    xmlnode_Invoke,
    xmlnode_get_nodeName,
    xmlnode_get_nodeValue,
    xmlnode_put_nodeValue,
    xmlnode_get_nodeType,
    xmlnode_get_parentNode,
    xmlnode_get_childNodes,
    xmlnode_get_firstChild,
    xmlnode_get_lastChild,
    xmlnode_get_previousSibling,
    xmlnode_get_nextSibling,
    xmlnode_get_attributes,
    xmlnode_insertBefore,
    xmlnode_replaceChild,
    xmlnode_removeChild,
    xmlnode_appendChild,
    xmlnode_hasChildNodes,
    xmlnode_get_ownerDocument,
    xmlnode_cloneNode,
    xmlnode_get_nodeTypeString,
    xmlnode_get_text,
    xmlnode_put_text,
    xmlnode_get_specified,
    xmlnode_get_definition,
    xmlnode_get_nodeTypedValue,
    xmlnode_put_nodeTypedValue,
    xmlnode_get_dataType,
    xmlnode_put_dataType,
    xmlnode_get_xml,
    xmlnode_transformNode,
    xmlnode_selectNodes,
    xmlnode_selectSingleNode,
    xmlnode_get_parsed,
    xmlnode_get_namespaceURI,
    xmlnode_get_prefix,
    xmlnode_get_baseName,
    xmlnode_transformNodeToObject,
};

static IXMLDOMNode *create_node( xmlNodePtr node )
{
    xmlnode *This;

    if ( !node )
        return NULL;

    assert( node->doc );

    /* if an interface already exists for this node, return it */
    if ( node->_private )
    {
        IXMLDOMNode *n = node->_private;
        IXMLDOMNode_AddRef( n );
        return n;
    }

    /*
     * Try adding a reference to the IXMLDOMNode implementation
     * containing the document's root element.
     */
    if ( node->type != XML_DOCUMENT_NODE )
    {
        IXMLDOMNode *root = NULL;

        root = node->doc->_private;
        assert( root );
        IXMLDOMNode_AddRef( root );
    }
    else
        assert( node->doc == (xmlDocPtr) node );

    This = HeapAlloc( GetProcessHeap(), 0, sizeof *This );
    if ( !This )
        return NULL;

    This->lpVtbl = &xmlnode_vtbl;
    This->ref = 1;
    This->node = node;

    /* remember which interface we associated with this node */
    node->_private = This;

    return (IXMLDOMNode*) &This->lpVtbl;
}

IXMLDOMNode *create_domdoc_node( xmlDocPtr node )
{
    return create_node( (xmlNodePtr) node );
}

IXMLDOMNode *create_attribute_node( xmlAttrPtr node )
{
    return create_node( (xmlNodePtr) node );
}

IXMLDOMNode *create_element_node( xmlNodePtr element )
{
    return create_node( element );
}

IXMLDOMNode *create_generic_node( xmlNodePtr node )
{
    return create_node( node );
}

#endif
