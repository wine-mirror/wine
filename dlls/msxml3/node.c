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

typedef union {
   xmlDocPtr     doc;
   xmlNodePtr    node;
   xmlAttrPtr    attr;
   xmlElementPtr element;
} libxml_node;

typedef struct _xmlnode
{
    const struct IXMLDOMNodeVtbl *lpVtbl;
    LONG ref;
    BOOL free_me;
    xmlElementType type;
    libxml_node u;
} xmlnode;

static inline xmlnode *impl_from_IXMLDOMNode( IXMLDOMNode *iface )
{
    return (xmlnode *)((char*)iface - FIELD_OFFSET(xmlnode, lpVtbl));
}

xmlDocPtr xmldoc_from_xmlnode( IXMLDOMNode *iface )
{
    xmlnode *This;

    if ( !iface )
        return NULL;

    This = impl_from_IXMLDOMNode( iface );
    if (This->type != XML_DOCUMENT_NODE )
        return NULL;
    return This->u.doc;
}

xmlNodePtr xmlelement_from_xmlnode( IXMLDOMNode *iface )
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    if (This->type != XML_ELEMENT_NODE )
        return NULL;
    return This->u.node;
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
        if ( This->free_me )
        {
            switch( This->type )
            {
            case XML_DOCUMENT_NODE:
                xmlFreeDoc( This->u.doc );
                break;
            case XML_ATTRIBUTE_NODE:
            case XML_ELEMENT_NODE:
                /* don't free these */
                break;
            default:
                ERR("don't know how to free this element\n");
            }
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

static HRESULT WINAPI xmlnode_get_xmlnodeName(
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

static HRESULT WINAPI xmlnode_get_xmlnodeValue(
    IXMLDOMNode *iface,
    VARIANT* value)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );

    TRACE("%p %p\n", This, value);

    switch ( This->type )
    {
    case XML_COMMENT_NODE:
        FIXME("comment\n");
        return E_FAIL;
        break;
    case XML_ATTRIBUTE_NODE:
        V_VT(value) = VT_BSTR;
        V_BSTR(value) = bstr_from_xmlChar( This->u.attr->name );
        break;
    case XML_PI_NODE:
        FIXME("processing instruction\n");
        return E_FAIL;
        break;
    case XML_ELEMENT_NODE:
    case XML_DOCUMENT_NODE:
    default:
        return E_FAIL;
        break;
    }
 
    TRACE("%p returned %s\n", This, debugstr_w( V_BSTR(value) ) );

    return S_OK;
}

static HRESULT WINAPI xmlnode_put_xmlnodeValue(
    IXMLDOMNode *iface,
    VARIANT value)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_xmlnodeType(
    IXMLDOMNode *iface,
    DOMNodeType* type)
{
    FIXME("\n");
    return E_NOTIMPL;
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
    FIXME("%p\n", This);
    return E_NOTIMPL;
    /*return NodeList_create( childList, This );*/
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
    FIXME("%p\n", This);
    return E_NOTIMPL;
    /*return NodeMap_create( attributeMap, This, node ); */
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

static HRESULT WINAPI xmlnode_get_xmlnodeTypeString(
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
    FIXME("\n");
    return E_NOTIMPL;
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

static HRESULT WINAPI xmlnode_get_xmlnodeTypedValue(
    IXMLDOMNode *iface,
    VARIANT* typedValue)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_put_xmlnodeTypedValue(
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
    FIXME("\n");
    return E_NOTIMPL;
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
    xmlnode_get_xmlnodeName,
    xmlnode_get_xmlnodeValue,
    xmlnode_put_xmlnodeValue,
    xmlnode_get_xmlnodeType,
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
    xmlnode_get_xmlnodeTypeString,
    xmlnode_get_text,
    xmlnode_put_text,
    xmlnode_get_specified,
    xmlnode_get_definition,
    xmlnode_get_xmlnodeTypedValue,
    xmlnode_put_xmlnodeTypedValue,
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

static xmlnode *create_node( void )
{
    xmlnode *This;

    This = HeapAlloc( GetProcessHeap(), 0, sizeof *This );
    if ( !This )
        return NULL;

    This->lpVtbl = &xmlnode_vtbl;
    This->ref = 1;
    This->free_me = TRUE;
    This->u.doc = NULL;

    return This;
}

IXMLDOMNode *create_domdoc_node( xmlDocPtr node )
{
    xmlnode *This;

    if ( !node )
        return NULL;

    This = create_node();
    if ( !This )
        return NULL;

    This->type = XML_DOCUMENT_NODE;
    This->u.doc = node;
 
    return (IXMLDOMNode*) &This->lpVtbl;
}

IXMLDOMNode *create_attribute_node( xmlAttrPtr node )
{
    xmlnode *This;

    if ( !node )
        return NULL;

    This = create_node();
    if ( !This )
        return NULL;

    This->type = XML_ATTRIBUTE_NODE;
    This->u.attr = node;
 
    return (IXMLDOMNode*) &This->lpVtbl;
}

IXMLDOMNode *create_element_node( xmlNodePtr element )
{
    xmlnode *This;

    if ( !element )
        return NULL;

    This = create_node();
    if ( !This )
        return NULL;

    This->type = XML_ELEMENT_NODE;
    This->u.node = element;
 
    return (IXMLDOMNode*) &This->lpVtbl;
}

#endif
