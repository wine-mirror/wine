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
    xmlDocPtr xmldoc;
} domdoc;

static inline domdoc *impl_from_IXMLDOMDocument( IXMLDOMDocument *iface )
{
    return (domdoc *)((char*)iface - FIELD_OFFSET(domdoc, lpVtbl));
}

static HRESULT WINAPI domdoc_QueryInterface( IXMLDOMDocument *iface, REFIID riid, void** ppvObject )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );

    TRACE("%p %p %p\n", This, debugstr_guid( riid ), ppvObject );

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
        xmlFreeDoc( This->xmldoc );
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
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_nodeValue(
    IXMLDOMDocument *iface,
    VARIANT* value )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_put_nodeValue(
    IXMLDOMDocument *iface,
    VARIANT value)
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_nodeType(
    IXMLDOMDocument *iface,
    DOMNodeType* type )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_parentNode(
    IXMLDOMDocument *iface,
    IXMLDOMNode** parent )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_childNodes(
    IXMLDOMDocument *iface,
    IXMLDOMNodeList** childList )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_firstChild(
    IXMLDOMDocument *iface,
    IXMLDOMNode** firstChild )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_lastChild(
    IXMLDOMDocument *iface,
    IXMLDOMNode** lastChild )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_previousSibling(
    IXMLDOMDocument *iface,
    IXMLDOMNode** previousSibling )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_nextSibling(
    IXMLDOMDocument *iface,
    IXMLDOMNode** nextSibling )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_attributes(
    IXMLDOMDocument *iface,
    IXMLDOMNamedNodeMap** attributeMap )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_insertBefore(
    IXMLDOMDocument *iface,
    IXMLDOMNode* newChild,
    VARIANT refChild,
    IXMLDOMNode** outNewChild )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_replaceChild(
    IXMLDOMDocument *iface,
    IXMLDOMNode* newChild,
    IXMLDOMNode* oldChild,
    IXMLDOMNode** outOldChild)
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_removeChild(
    IXMLDOMDocument *iface,
    IXMLDOMNode* childNode,
    IXMLDOMNode** oldChild)
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_appendChild(
    IXMLDOMDocument *iface,
    IXMLDOMNode* newChild,
    IXMLDOMNode** outNewChild)
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_hasChildNodes(
    IXMLDOMDocument *iface,
    VARIANT_BOOL* hasChild)
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_ownerDocument(
    IXMLDOMDocument *iface,
    IXMLDOMDocument** DOMDocument)
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_cloneNode(
    IXMLDOMDocument *iface,
    VARIANT_BOOL deep,
    IXMLDOMNode** cloneRoot)
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_nodeTypeString(
    IXMLDOMDocument *iface,
    BSTR* nodeType )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_text(
    IXMLDOMDocument *iface,
    BSTR* text )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_put_text(
    IXMLDOMDocument *iface,
    BSTR text )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_specified(
    IXMLDOMDocument *iface,
    VARIANT_BOOL* isSpecified )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_definition(
    IXMLDOMDocument *iface,
    IXMLDOMNode** definitionNode )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_nodeTypedValue(
    IXMLDOMDocument *iface,
    VARIANT* typedValue )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoc_put_nodeTypedValue(
    IXMLDOMDocument *iface,
    VARIANT typedValue )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_dataType(
    IXMLDOMDocument *iface,
    VARIANT* dataTypeName )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_put_dataType(
    IXMLDOMDocument *iface,
    BSTR dataTypeName )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_xml(
    IXMLDOMDocument *iface,
    BSTR* xmlString )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_transformNode(
    IXMLDOMDocument *iface,
    IXMLDOMNode* styleSheet,
    BSTR* xmlString )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_selectNodes(
    IXMLDOMDocument *iface,
    BSTR queryString,
    IXMLDOMNodeList** resultList )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_selectSingleNode(
    IXMLDOMDocument *iface,
    BSTR queryString,
    IXMLDOMNode** resultNode )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_parsed(
    IXMLDOMDocument *iface,
    VARIANT_BOOL* isParsed )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_namespaceURI(
    IXMLDOMDocument *iface,
    BSTR* namespaceURI )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_prefix(
    IXMLDOMDocument *iface,
    BSTR* prefixString )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_baseName(
    IXMLDOMDocument *iface,
    BSTR* nameString )
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_transformNodeToObject(
    IXMLDOMDocument *iface,
    IXMLDOMNode* stylesheet,
    VARIANT outputObject)
{
    FIXME("\n");
    return E_NOTIMPL;
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

    FIXME("%p\n", This);

    return DOMElement_create(DOMElement, This->xmldoc);
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

static int read_input_callback( void *context, char *buffer, int len )
{
    HANDLE handle = context;
    DWORD count = 0;
    BOOL r;

    r = ReadFile( handle, buffer, len, &count, NULL );
    if ( !r )
        return -1;

    return count;
}

static int input_close_callback( void *context )
{
    HANDLE handle = context;
    return CloseHandle( handle ) ? 0 : -1;
}

static xmlDocPtr doread( LPWSTR filename )
{
    HANDLE handle;

    TRACE("%s\n", debugstr_w( filename ));

    handle = CreateFileW( filename, GENERIC_READ, FILE_SHARE_READ,
                         NULL, OPEN_EXISTING, 0, NULL );
    if( handle == INVALID_HANDLE_VALUE )
        return NULL;

    return xmlReadIO( read_input_callback, input_close_callback,
                      handle, NULL, NULL, 0 );
}


static HRESULT WINAPI domdoc_load(
    IXMLDOMDocument *iface,
    VARIANT xmlSource,
    VARIANT_BOOL* isSuccessful )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    LPWSTR filename = NULL;
    xmlDocPtr xd;

    TRACE("%p\n", This);

    switch( V_VT(&xmlSource) )
    {
    case VT_BSTR:
        filename = V_BSTR(&xmlSource);
    }

    if ( !filename )
        return E_FAIL;

    xd = doread( filename );
    if ( !xd )
        return E_FAIL;

    /* free the old document before overwriting it */
    if ( This->xmldoc )
        xmlFreeDoc( This->xmldoc );
    This->xmldoc = xd;

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


static HRESULT WINAPI domdoc_loadXML(
    IXMLDOMDocument *iface,
    BSTR bstrXML,
    VARIANT_BOOL* isSuccessful )
{
    FIXME("\n");
    return E_NOTIMPL;
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
    doc->xmldoc = NULL;

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
