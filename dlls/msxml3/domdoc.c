/*
 *    DOM Document implementation
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

#define COBJMACROS

#include "config.h"

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "ole2.h"
#include "ocidl.h"
#include "msxml.h"
#include "xmldom.h"

#include "wine/debug.h"

#include "msxml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

#ifdef HAVE_LIBXML2

typedef struct _domdoc
{
    const struct IXMLDOMDocumentVtbl *lpVtbl;
    LONG ref;
    VARIANT_BOOL async;
    IXMLDOMNode *node;
} domdoc;

static inline domdoc *impl_from_IXMLDOMDocument( IXMLDOMDocument *iface )
{
    return (domdoc *)((char*)iface - FIELD_OFFSET(domdoc, lpVtbl));
}

static inline xmlDocPtr get_doc( domdoc *This )
{
    return (xmlDocPtr) xmlNodePtr_from_domnode( This->node, XML_DOCUMENT_NODE );
}

static HRESULT WINAPI domdoc_QueryInterface( IXMLDOMDocument *iface, REFIID riid, void** ppvObject )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_IXMLDOMDocument ) ||
         IsEqualGUID( riid, &IID_IXMLDOMNode ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
        return E_NOINTERFACE;

    IXMLDOMDocument_AddRef( iface );

    return S_OK;
}


static ULONG WINAPI domdoc_AddRef(
     IXMLDOMDocument *iface )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    TRACE("%p\n", This );
    return InterlockedIncrement( &This->ref );
}


static ULONG WINAPI domdoc_Release(
     IXMLDOMDocument *iface )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    LONG ref;

    TRACE("%p\n", This );

    ref = InterlockedDecrement( &This->ref );
    if ( ref == 0 )
    {
        if ( This->node )
            IXMLDOMElement_Release( This->node );
        HeapFree( GetProcessHeap(), 0, This );
    }

    return ref;
}

static HRESULT WINAPI domdoc_GetTypeInfoCount( IXMLDOMDocument *iface, UINT* pctinfo )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoc_GetTypeInfo(
    IXMLDOMDocument *iface,
    UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoc_GetIDsOfNames(
    IXMLDOMDocument *iface,
    REFIID riid,
    LPOLESTR* rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID* rgDispId)
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_Invoke(
    IXMLDOMDocument *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS* pDispParams,
    VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo,
    UINT* puArgErr)
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_nodeName(
    IXMLDOMDocument *iface,
    BSTR* name )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_get_nodeName( This->node, name );
}


static HRESULT WINAPI domdoc_get_nodeValue(
    IXMLDOMDocument *iface,
    VARIANT* value )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_get_nodeValue( This->node, value );
}


static HRESULT WINAPI domdoc_put_nodeValue(
    IXMLDOMDocument *iface,
    VARIANT value)
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_put_nodeValue( This->node, value );
}


static HRESULT WINAPI domdoc_get_nodeType(
    IXMLDOMDocument *iface,
    DOMNodeType* type )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_get_nodeType( This->node, type );
}


static HRESULT WINAPI domdoc_get_parentNode(
    IXMLDOMDocument *iface,
    IXMLDOMNode** parent )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_get_parentNode( This->node, parent );
}


static HRESULT WINAPI domdoc_get_childNodes(
    IXMLDOMDocument *iface,
    IXMLDOMNodeList** childList )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_get_childNodes( This->node, childList );
}


static HRESULT WINAPI domdoc_get_firstChild(
    IXMLDOMDocument *iface,
    IXMLDOMNode** firstChild )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_get_firstChild( This->node, firstChild );
}


static HRESULT WINAPI domdoc_get_lastChild(
    IXMLDOMDocument *iface,
    IXMLDOMNode** lastChild )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_get_lastChild( This->node, lastChild );
}


static HRESULT WINAPI domdoc_get_previousSibling(
    IXMLDOMDocument *iface,
    IXMLDOMNode** previousSibling )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_get_previousSibling( This->node, previousSibling );
}


static HRESULT WINAPI domdoc_get_nextSibling(
    IXMLDOMDocument *iface,
    IXMLDOMNode** nextSibling )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_get_nextSibling( This->node, nextSibling );
}


static HRESULT WINAPI domdoc_get_attributes(
    IXMLDOMDocument *iface,
    IXMLDOMNamedNodeMap** attributeMap )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_get_attributes( This->node, attributeMap );
}


static HRESULT WINAPI domdoc_insertBefore(
    IXMLDOMDocument *iface,
    IXMLDOMNode* newChild,
    VARIANT refChild,
    IXMLDOMNode** outNewChild )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_insertBefore( This->node, newChild, refChild, outNewChild );
}


static HRESULT WINAPI domdoc_replaceChild(
    IXMLDOMDocument *iface,
    IXMLDOMNode* newChild,
    IXMLDOMNode* oldChild,
    IXMLDOMNode** outOldChild)
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_replaceChild( This->node, newChild, oldChild, outOldChild );
}


static HRESULT WINAPI domdoc_removeChild(
    IXMLDOMDocument *iface,
    IXMLDOMNode* childNode,
    IXMLDOMNode** oldChild)
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_removeChild( This->node, childNode, oldChild );
}


static HRESULT WINAPI domdoc_appendChild(
    IXMLDOMDocument *iface,
    IXMLDOMNode* newChild,
    IXMLDOMNode** outNewChild)
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_appendChild( This->node, newChild, outNewChild );
}


static HRESULT WINAPI domdoc_hasChildNodes(
    IXMLDOMDocument *iface,
    VARIANT_BOOL* hasChild)
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_hasChildNodes( This->node, hasChild );
}


static HRESULT WINAPI domdoc_get_ownerDocument(
    IXMLDOMDocument *iface,
    IXMLDOMDocument** DOMDocument)
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_get_ownerDocument( This->node, DOMDocument );
}


static HRESULT WINAPI domdoc_cloneNode(
    IXMLDOMDocument *iface,
    VARIANT_BOOL deep,
    IXMLDOMNode** cloneRoot)
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_cloneNode( This->node, deep, cloneRoot );
}


static HRESULT WINAPI domdoc_get_nodeTypeString(
    IXMLDOMDocument *iface,
    BSTR* nodeType )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_get_nodeTypeString( This->node, nodeType );
}


static HRESULT WINAPI domdoc_get_text(
    IXMLDOMDocument *iface,
    BSTR* text )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_get_text( This->node, text );
}


static HRESULT WINAPI domdoc_put_text(
    IXMLDOMDocument *iface,
    BSTR text )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_put_text( This->node, text );
}


static HRESULT WINAPI domdoc_get_specified(
    IXMLDOMDocument *iface,
    VARIANT_BOOL* isSpecified )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_get_specified( This->node, isSpecified );
}


static HRESULT WINAPI domdoc_get_definition(
    IXMLDOMDocument *iface,
    IXMLDOMNode** definitionNode )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_get_definition( This->node, definitionNode );
}


static HRESULT WINAPI domdoc_get_nodeTypedValue(
    IXMLDOMDocument *iface,
    VARIANT* typedValue )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_get_nodeTypedValue( This->node, typedValue );
}

static HRESULT WINAPI domdoc_put_nodeTypedValue(
    IXMLDOMDocument *iface,
    VARIANT typedValue )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_put_nodeTypedValue( This->node, typedValue );
}


static HRESULT WINAPI domdoc_get_dataType(
    IXMLDOMDocument *iface,
    VARIANT* dataTypeName )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_get_dataType( This->node, dataTypeName );
}


static HRESULT WINAPI domdoc_put_dataType(
    IXMLDOMDocument *iface,
    BSTR dataTypeName )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_put_dataType( This->node, dataTypeName );
}


static HRESULT WINAPI domdoc_get_xml(
    IXMLDOMDocument *iface,
    BSTR* xmlString )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_get_xml( This->node, xmlString );
}


static HRESULT WINAPI domdoc_transformNode(
    IXMLDOMDocument *iface,
    IXMLDOMNode* styleSheet,
    BSTR* xmlString )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_transformNode( This->node, styleSheet, xmlString );
}


static HRESULT WINAPI domdoc_selectNodes(
    IXMLDOMDocument *iface,
    BSTR queryString,
    IXMLDOMNodeList** resultList )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_selectNodes( This->node, queryString, resultList );
}


static HRESULT WINAPI domdoc_selectSingleNode(
    IXMLDOMDocument *iface,
    BSTR queryString,
    IXMLDOMNode** resultNode )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_selectSingleNode( This->node, queryString, resultNode );
}


static HRESULT WINAPI domdoc_get_parsed(
    IXMLDOMDocument *iface,
    VARIANT_BOOL* isParsed )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_get_parsed( This->node, isParsed );
}


static HRESULT WINAPI domdoc_get_namespaceURI(
    IXMLDOMDocument *iface,
    BSTR* namespaceURI )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_get_namespaceURI( This->node, namespaceURI );
}


static HRESULT WINAPI domdoc_get_prefix(
    IXMLDOMDocument *iface,
    BSTR* prefixString )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_get_prefix( This->node, prefixString );
}


static HRESULT WINAPI domdoc_get_baseName(
    IXMLDOMDocument *iface,
    BSTR* nameString )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_get_baseName( This->node, nameString );
}


static HRESULT WINAPI domdoc_transformNodeToObject(
    IXMLDOMDocument *iface,
    IXMLDOMNode* stylesheet,
    VARIANT outputObject)
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    return IXMLDOMNode_transformNodeToObject( This->node, stylesheet, outputObject );
}


static HRESULT WINAPI domdoc_get_doctype(
    IXMLDOMDocument *iface,
    IXMLDOMDocument** documentType )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_implementation(
    IXMLDOMDocument *iface,
    IXMLDOMImplementation** impl )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoc_get_documentElement(
    IXMLDOMDocument *iface,
    IXMLDOMElement** DOMElement )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    xmlDocPtr xmldoc = NULL;
    xmlNodePtr root = NULL;

    TRACE("%p\n", This);

    *DOMElement = NULL;

    if ( !This->node )
        return S_FALSE;

    xmldoc = get_doc( This );
    if ( !xmldoc )
        return S_FALSE;

    root = xmlDocGetRootElement( xmldoc );
    if ( !root )
        return S_FALSE;

    *DOMElement = create_element( root );
 
    return S_OK;
}


static HRESULT WINAPI domdoc_documentElement(
    IXMLDOMDocument *iface,
    IXMLDOMElement* DOMElement )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_createElement(
    IXMLDOMDocument *iface,
    BSTR tagname,
    IXMLDOMElement** element )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_createDocumentFragment(
    IXMLDOMDocument *iface,
    IXMLDOMDocumentFragment** docFrag )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_createTextNode(
    IXMLDOMDocument *iface,
    BSTR data,
    IXMLDOMText** text )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_createComment(
    IXMLDOMDocument *iface,
    BSTR data,
    IXMLDOMComment** comment )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_createCDATASection(
    IXMLDOMDocument *iface,
    BSTR data,
    IXMLDOMCDATASection** cdata )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_createProcessingInstruction(
    IXMLDOMDocument *iface,
    BSTR target,
    BSTR data,
    IXMLDOMProcessingInstruction** pi )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_createAttribute(
    IXMLDOMDocument *iface,
    BSTR name,
    IXMLDOMAttribute** attribute )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_createEntityReference(
    IXMLDOMDocument *iface,
    BSTR name,
    IXMLDOMEntityReference** entityRef )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_getElementsByTagName(
    IXMLDOMDocument *iface,
    BSTR tagName,
    IXMLDOMNodeList** resultList )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_createNode(
    IXMLDOMDocument *iface,
    VARIANT Type,
    BSTR name,
    BSTR namespaceURI,
    IXMLDOMNode** node )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_nodeFromID(
    IXMLDOMDocument *iface,
    BSTR idString,
    IXMLDOMNode** node )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static xmlDocPtr doparse( char *ptr, int len )
{
#ifdef HAVE_XMLREADMEMORY
    /*
     * use xmlReadMemory if possible so we can suppress
     * writing errors to stderr
     */
    return xmlReadMemory( ptr, len, NULL, NULL,
                          XML_PARSE_NOERROR | XML_PARSE_NOWARNING );
#else
    return xmlParseMemory( ptr, len );
#endif
}

static xmlDocPtr doread( LPWSTR filename )
{
    HANDLE handle, mapping;
    DWORD len;
    xmlDocPtr xmldoc = NULL;
    char *ptr;

    TRACE("%s\n", debugstr_w( filename ));

    handle = CreateFileW( filename, GENERIC_READ, FILE_SHARE_READ,
                         NULL, OPEN_EXISTING, 0, NULL );
    if( handle == INVALID_HANDLE_VALUE )
        return xmldoc;

    len = GetFileSize( handle, NULL );
    if( len != INVALID_FILE_SIZE || GetLastError() == NO_ERROR )
    {
        mapping = CreateFileMappingW( handle, NULL, PAGE_READONLY, 0, 0, NULL );
        if ( mapping )
        {
            ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, len );
            if ( ptr )
            {
                xmldoc = doparse( ptr, len );
                UnmapViewOfFile( ptr );
            }
            CloseHandle( mapping );
        }
    }
    CloseHandle( handle );

    return xmldoc;
}


static HRESULT WINAPI domdoc_load(
    IXMLDOMDocument *iface,
    VARIANT xmlSource,
    VARIANT_BOOL* isSuccessful )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    LPWSTR filename = NULL;
    xmlDocPtr xmldoc;

    TRACE("type %d\n", V_VT(&xmlSource) );

    if ( This->node )
    {
        IXMLDOMNode_Release( This->node );
        This->node = NULL;
    }

    switch( V_VT(&xmlSource) )
    {
    case VT_BSTR:
        filename = V_BSTR(&xmlSource);
    }

    if ( !filename )
        return S_FALSE;

    xmldoc = doread( filename );
    if ( !xmldoc ) {
        *isSuccessful = VARIANT_FALSE;
        return S_FALSE;
    }

    This->node = create_node( (xmlNodePtr) xmldoc );
    if ( !This->node )
    {
        *isSuccessful = VARIANT_FALSE;
        return S_FALSE;
    }

    *isSuccessful = VARIANT_TRUE;
    return S_OK;
}


static HRESULT WINAPI domdoc_get_readyState(
    IXMLDOMDocument *iface,
    long* value )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_parseError(
    IXMLDOMDocument *iface,
    IXMLDOMParseError** errorObj )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_url(
    IXMLDOMDocument *iface,
    BSTR* urlString )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_async(
    IXMLDOMDocument *iface,
    VARIANT_BOOL* isAsync )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );

    TRACE("%p <- %d\n", isAsync, This->async);
    *isAsync = This->async;
    return S_OK;
}


static HRESULT WINAPI domdoc_put_async(
    IXMLDOMDocument *iface,
    VARIANT_BOOL isAsync )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );

    TRACE("%d\n", isAsync);
    This->async = isAsync;
    return S_OK;
}


static HRESULT WINAPI domdoc_abort(
    IXMLDOMDocument *iface )
{
    FIXME("\n");
    return E_NOTIMPL;
}


BOOL bstr_to_utf8( BSTR bstr, char **pstr, int *plen )
{
    UINT len, blen = SysStringLen( bstr );
    LPSTR str;

    len = WideCharToMultiByte( CP_UTF8, 0, bstr, blen, NULL, 0, NULL, NULL );
    str = HeapAlloc( GetProcessHeap(), 0, len );
    if ( !str )
        return FALSE;
    WideCharToMultiByte( CP_UTF8, 0, bstr, blen, str, len, NULL, NULL );
    *plen = len;
    *pstr = str;
    return TRUE;
}

static HRESULT WINAPI domdoc_loadXML(
    IXMLDOMDocument *iface,
    BSTR bstrXML,
    VARIANT_BOOL* isSuccessful )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    xmlDocPtr xmldoc;
    char *str;
    int len;

    TRACE("%p %s %p\n", This, debugstr_w( bstrXML ), isSuccessful );

    if ( This->node )
    {
        IXMLDOMNode_Release( This->node );
        This->node = NULL;
    }

    if ( !isSuccessful )
        return S_FALSE;

    *isSuccessful = VARIANT_FALSE;

    if ( !bstrXML )
        return S_FALSE;

    if ( !bstr_to_utf8( bstrXML, &str, &len ) )
        return S_FALSE;

    xmldoc = doparse( str, len );
    HeapFree( GetProcessHeap(), 0, str );

    This->node = create_node( (xmlNodePtr) xmldoc );
    if( !This->node )
        return S_FALSE;

    *isSuccessful = VARIANT_TRUE;
    return S_OK;
}


static HRESULT WINAPI domdoc_save(
    IXMLDOMDocument *iface,
    VARIANT destination )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoc_get_validateOnParse(
    IXMLDOMDocument *iface,
    VARIANT_BOOL* isValidating )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_put_validateOnParse(
    IXMLDOMDocument *iface,
    VARIANT_BOOL isValidating )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_resolveExternals(
    IXMLDOMDocument *iface,
    VARIANT_BOOL* isResolving )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_put_resolveExternals(
    IXMLDOMDocument *iface,
    VARIANT_BOOL isValidating )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_preserveWhiteSpace(
    IXMLDOMDocument *iface,
    VARIANT_BOOL* isPreserving )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_put_preserveWhiteSpace(
    IXMLDOMDocument *iface,
    VARIANT_BOOL isPreserving )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_put_onReadyStateChange(
    IXMLDOMDocument *iface,
    VARIANT readyStateChangeSink )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_put_onDataAvailable(
    IXMLDOMDocument *iface,
    VARIANT onDataAvailableSink )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoc_put_onTransformNode(
    IXMLDOMDocument *iface,
    VARIANT onTransformNodeSink )
{
    FIXME("\n");
    return E_NOTIMPL;
}

const struct IXMLDOMDocumentVtbl domdoc_vtbl =
{
    domdoc_QueryInterface,
    domdoc_AddRef,
    domdoc_Release,
    domdoc_GetTypeInfoCount,
    domdoc_GetTypeInfo,
    domdoc_GetIDsOfNames,
    domdoc_Invoke,
    domdoc_get_nodeName,
    domdoc_get_nodeValue,
    domdoc_put_nodeValue,
    domdoc_get_nodeType,
    domdoc_get_parentNode,
    domdoc_get_childNodes,
    domdoc_get_firstChild,
    domdoc_get_lastChild,
    domdoc_get_previousSibling,
    domdoc_get_nextSibling,
    domdoc_get_attributes,
    domdoc_insertBefore,
    domdoc_replaceChild,
    domdoc_removeChild,
    domdoc_appendChild,
    domdoc_hasChildNodes,
    domdoc_get_ownerDocument,
    domdoc_cloneNode,
    domdoc_get_nodeTypeString,
    domdoc_get_text,
    domdoc_put_text,
    domdoc_get_specified,
    domdoc_get_definition,
    domdoc_get_nodeTypedValue,
    domdoc_put_nodeTypedValue,
    domdoc_get_dataType,
    domdoc_put_dataType,
    domdoc_get_xml,
    domdoc_transformNode,
    domdoc_selectNodes,
    domdoc_selectSingleNode,
    domdoc_get_parsed,
    domdoc_get_namespaceURI,
    domdoc_get_prefix,
    domdoc_get_baseName,
    domdoc_transformNodeToObject,
    domdoc_get_doctype,
    domdoc_get_implementation,
    domdoc_get_documentElement,
    domdoc_documentElement,
    domdoc_createElement,
    domdoc_createDocumentFragment,
    domdoc_createTextNode,
    domdoc_createComment,
    domdoc_createCDATASection,
    domdoc_createProcessingInstruction,
    domdoc_createAttribute,
    domdoc_createEntityReference,
    domdoc_getElementsByTagName,
    domdoc_createNode,
    domdoc_nodeFromID,
    domdoc_load,
    domdoc_get_readyState,
    domdoc_get_parseError,
    domdoc_get_url,
    domdoc_get_async,
    domdoc_put_async,
    domdoc_abort,
    domdoc_loadXML,
    domdoc_save,
    domdoc_get_validateOnParse,
    domdoc_put_validateOnParse,
    domdoc_get_resolveExternals,
    domdoc_put_resolveExternals,
    domdoc_get_preserveWhiteSpace,
    domdoc_put_preserveWhiteSpace,
    domdoc_put_onReadyStateChange,
    domdoc_put_onDataAvailable,
    domdoc_put_onTransformNode,
};

HRESULT DOMDocument_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    domdoc *doc;

    doc = HeapAlloc( GetProcessHeap(), 0, sizeof (*doc) );
    if( !doc )
        return E_OUTOFMEMORY;

    doc->lpVtbl = &domdoc_vtbl;
    doc->ref = 1;
    doc->async = 0;
    doc->node = NULL;

    *ppObj = &doc->lpVtbl;

    return S_OK;
}

#else

HRESULT DOMDocument_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    MESSAGE("This program tried to use a DOMDocument object, but\n"
            "libxml2 support was not present at compile time.\n");
    return E_NOTIMPL;
}

#endif
