/*
 *    DOM Document implementation
 *
 * Copyright 2005 Mike McCormack
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
#include <assert.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "ole2.h"
#include "msxml2.h"
#include "wininet.h"
#include "urlmon.h"
#include "winreg.h"
#include "shlwapi.h"

#include "wine/debug.h"

#include "msxml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

#ifdef HAVE_LIBXML2

typedef struct {
    const struct IBindStatusCallbackVtbl *lpVtbl;
} bsc;

static HRESULT WINAPI bsc_QueryInterface(
    IBindStatusCallback *iface,
    REFIID riid,
    LPVOID *ppobj )
{
    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IBindStatusCallback))
    {
        IBindStatusCallback_AddRef( iface );
        *ppobj = iface;
        return S_OK;
    }

    FIXME("interface %s not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI bsc_AddRef(
    IBindStatusCallback *iface )
{
    return 2;
}

static ULONG WINAPI bsc_Release(
    IBindStatusCallback *iface )
{
    return 1;
}

static HRESULT WINAPI bsc_OnStartBinding(
        IBindStatusCallback* iface,
        DWORD dwReserved,
        IBinding* pib)
{
    return S_OK;
}

static HRESULT WINAPI bsc_GetPriority(
        IBindStatusCallback* iface,
        LONG* pnPriority)
{
    return S_OK;
}

static HRESULT WINAPI bsc_OnLowResource(
        IBindStatusCallback* iface,
        DWORD reserved)
{
    return S_OK;
}

static HRESULT WINAPI bsc_OnProgress(
        IBindStatusCallback* iface,
        ULONG ulProgress,
        ULONG ulProgressMax,
        ULONG ulStatusCode,
        LPCWSTR szStatusText)
{
    return S_OK;
}

static HRESULT WINAPI bsc_OnStopBinding(
        IBindStatusCallback* iface,
        HRESULT hresult,
        LPCWSTR szError)
{
    return S_OK;
}

static HRESULT WINAPI bsc_GetBindInfo(
        IBindStatusCallback* iface,
        DWORD* grfBINDF,
        BINDINFO* pbindinfo)
{
    *grfBINDF = BINDF_RESYNCHRONIZE;
    
    return S_OK;
}

static HRESULT WINAPI bsc_OnDataAvailable(
        IBindStatusCallback* iface,
        DWORD grfBSCF,
        DWORD dwSize,
        FORMATETC* pformatetc,
        STGMEDIUM* pstgmed)
{
    return S_OK;
}

static HRESULT WINAPI bsc_OnObjectAvailable(
        IBindStatusCallback* iface,
        REFIID riid,
        IUnknown* punk)
{
    return S_OK;
}

const struct IBindStatusCallbackVtbl bsc_vtbl =
{
    bsc_QueryInterface,
    bsc_AddRef,
    bsc_Release,
    bsc_OnStartBinding,
    bsc_GetPriority,
    bsc_OnLowResource,
    bsc_OnProgress,
    bsc_OnStopBinding,
    bsc_GetBindInfo,
    bsc_OnDataAvailable,
    bsc_OnObjectAvailable
};

static bsc domdoc_bsc = { &bsc_vtbl };

typedef struct _domdoc
{
    const struct IXMLDOMDocumentVtbl *lpVtbl;
    LONG ref;
    VARIANT_BOOL async;
    IUnknown *node_unk;
    IXMLDOMNode *node;
    HRESULT error;
} domdoc;

LONG xmldoc_add_ref(xmlDocPtr doc)
{
    LONG ref = InterlockedIncrement((LONG*)&doc->_private);
    TRACE("%ld\n", ref);
    return ref;
}

LONG xmldoc_release(xmlDocPtr doc)
{
    LONG ref = InterlockedDecrement((LONG*)&doc->_private);
    TRACE("%ld\n", ref);
    if(ref == 0)
    {
        TRACE("freeing docptr %p\n", doc);
        xmlFreeDoc(doc);
    }

    return ref;
}

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
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else if ( IsEqualGUID( riid, &IID_IXMLDOMNode ) ||
              IsEqualGUID( riid, &IID_IDispatch ) )
    {
        return IUnknown_QueryInterface(This->node_unk, riid, ppvObject);
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

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
        IUnknown_Release( This->node_unk );
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
    IXMLDOMDocumentType** documentType )
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
    IXMLDOMNode *element_node;
    HRESULT hr;

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

    element_node = create_node( root );
    if(!element_node) return S_FALSE;

    hr = IXMLDOMNode_QueryInterface(element_node, &IID_IXMLDOMElement, (LPVOID*)DOMElement);
    IXMLDOMNode_Release(element_node);

    return hr;
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
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    xmlChar *name;
    TRACE("(%p)->(%s, %p)\n", This, debugstr_w(tagName), resultList);

    name = xmlChar_from_wchar((WCHAR*)tagName);
    *resultList = create_filtered_nodelist((xmlNodePtr)get_doc(This), name, TRUE);
    HeapFree(GetProcessHeap(), 0, name);

    if(!*resultList) return S_FALSE;
    return S_OK;
}

static DOMNodeType get_node_type(VARIANT Type)
{
    if(V_VT(&Type) == VT_I4)
        return V_I4(&Type);

    FIXME("Unsupported variant type %x\n", V_VT(&Type));
    return 0;
}

static HRESULT WINAPI domdoc_createNode(
    IXMLDOMDocument *iface,
    VARIANT Type,
    BSTR name,
    BSTR namespaceURI,
    IXMLDOMNode** node )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    DOMNodeType node_type;
    xmlNodePtr xmlnode = NULL;
    xmlChar *xml_name;
    xmlDocPtr xmldoc;

    TRACE("(%p)->(type,%s,%s,%p)\n", This, debugstr_w(name), debugstr_w(namespaceURI), node);

    node_type = get_node_type(Type);
    TRACE("node_type %d\n", node_type);

    xml_name = xmlChar_from_wchar((WCHAR*)name);

    if(!get_doc(This))
    {
        xmldoc = xmlNewDoc(NULL);
        xmldoc->_private = 0;
        attach_xmlnode(This->node, (xmlNodePtr) xmldoc);
    }

    switch(node_type)
    {
    case NODE_ELEMENT:
        xmlnode = xmlNewDocNode(get_doc(This), NULL, xml_name, NULL);
        *node = create_node(xmlnode);
        TRACE("created %p\n", xmlnode);
        break;

    default:
        FIXME("unhandled node type %d\n", node_type);
        break;
    }

    HeapFree(GetProcessHeap(), 0, xml_name);

    if(xmlnode && *node)
        return S_OK;

    return E_FAIL;
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
                          XML_PARSE_NOERROR | XML_PARSE_NOWARNING | XML_PARSE_NOBLANKS );
#else
    return xmlParseMemory( ptr, len );
#endif
}

static xmlDocPtr doread( LPWSTR filename )
{
    xmlDocPtr xmldoc = NULL;
    HRESULT hr;
    IBindCtx *pbc;
    IStream *stream, *memstream;
    WCHAR url[INTERNET_MAX_URL_LENGTH];
    BYTE buf[4096];
    DWORD read, written;

    TRACE("%s\n", debugstr_w( filename ));

    if(!PathIsURLW(filename))
    {
        WCHAR fullpath[MAX_PATH];
        DWORD needed = sizeof(url)/sizeof(WCHAR);

        if(!PathSearchAndQualifyW(filename, fullpath, sizeof(fullpath)/sizeof(WCHAR)))
        {
            WARN("can't find path\n");
            return NULL;
        }

        if(FAILED(UrlCreateFromPathW(fullpath, url, &needed, 0)))
        {
            ERR("can't create url from path\n");
            return NULL;
        }
        filename = url;
    }

    hr = CreateBindCtx(0, &pbc);
    if(SUCCEEDED(hr))
    {
        hr = RegisterBindStatusCallback(pbc, (IBindStatusCallback*)&domdoc_bsc.lpVtbl, NULL, 0);
        if(SUCCEEDED(hr))
        {
            IMoniker *moniker;
            hr = CreateURLMoniker(NULL, filename, &moniker);
            if(SUCCEEDED(hr))
            {
                hr = IMoniker_BindToStorage(moniker, pbc, NULL, &IID_IStream, (LPVOID*)&stream);
                IMoniker_Release(moniker);
            }
        }
        IBindCtx_Release(pbc);
    }
    if(FAILED(hr))
        return NULL;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &memstream);
    if(FAILED(hr))
    {
        IStream_Release(stream);
        return NULL;
    }

    do
    {
        IStream_Read(stream, buf, sizeof(buf), &read);
        hr = IStream_Write(memstream, buf, read, &written);
    } while(SUCCEEDED(hr) && written != 0 && read != 0);

    if(SUCCEEDED(hr))
    {
        HGLOBAL hglobal;
        hr = GetHGlobalFromStream(memstream, &hglobal);
        if(SUCCEEDED(hr))
        {
            DWORD len = GlobalSize(hglobal);
            char *ptr = GlobalLock(hglobal);
            if(len != 0)
                xmldoc = doparse( ptr, len );
            GlobalUnlock(hglobal);
        }
    }
    IStream_Release(memstream);
    IStream_Release(stream);
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

    *isSuccessful = VARIANT_FALSE;

    assert( This->node );

    attach_xmlnode(This->node, NULL);

    switch( V_VT(&xmlSource) )
    {
    case VT_BSTR:
        filename = V_BSTR(&xmlSource);
    }

    if ( !filename )
        return S_FALSE;

    xmldoc = doread( filename );
    if ( !xmldoc )
    {
        This->error = E_FAIL;
        return S_FALSE;
    }

    This->error = S_OK;
    xmldoc->_private = 0;
    attach_xmlnode(This->node, (xmlNodePtr) xmldoc);

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
    BSTR error_string = NULL;
    static const WCHAR err[] = {'e','r','r','o','r',0};
    domdoc *This = impl_from_IXMLDOMDocument( iface );

    FIXME("(%p)->(%p): creating a dummy parseError\n", iface, errorObj);

    if(This->error)
        error_string = SysAllocString(err);

    *errorObj = create_parseError(This->error, NULL, error_string, NULL, 0, 0, 0);
    if(!*errorObj) return E_OUTOFMEMORY;
    return S_OK;
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

    assert ( This->node );

    attach_xmlnode( This->node, NULL );

    if ( !isSuccessful )
        return S_FALSE;

    *isSuccessful = VARIANT_FALSE;

    if ( !bstrXML )
        return S_FALSE;

    if ( !bstr_to_utf8( bstrXML, &str, &len ) )
        return S_FALSE;

    xmldoc = doparse( str, len );
    HeapFree( GetProcessHeap(), 0, str );
    if ( !xmldoc )
    {
        This->error = E_FAIL;
        return S_FALSE;
    }

    This->error = S_OK;
    xmldoc->_private = 0;
    attach_xmlnode( This->node, (xmlNodePtr) xmldoc );

    *isSuccessful = VARIANT_TRUE;
    return S_OK;
}


static HRESULT WINAPI domdoc_save(
    IXMLDOMDocument *iface,
    VARIANT destination )
{
    domdoc *This = impl_from_IXMLDOMDocument( iface );
    HANDLE handle;
    xmlChar *mem;
    int size;
    HRESULT ret = S_OK;
    DWORD written;

    TRACE("(%p)->(var(vt %x, %s))\n", This, V_VT(&destination),
          V_VT(&destination) == VT_BSTR ? debugstr_w(V_BSTR(&destination)) : NULL);

    if(V_VT(&destination) != VT_BSTR)
    {
        FIXME("Unhandled vt %x\n", V_VT(&destination));
        return S_FALSE;
    }

    handle = CreateFileW( V_BSTR(&destination), GENERIC_WRITE, 0,
                          NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    if( handle == INVALID_HANDLE_VALUE )
    {
        WARN("failed to create file\n");
        return S_FALSE;
    }

    xmlDocDumpMemory(get_doc(This), &mem, &size);
    if(!WriteFile(handle, mem, (DWORD)size, &written, NULL) || written != (DWORD)size)
    {
        WARN("write error\n");
        ret = S_FALSE;
    }

    xmlFree(mem);
    CloseHandle(handle);
    return ret;
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
    HRESULT hr;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    doc = HeapAlloc( GetProcessHeap(), 0, sizeof (*doc) );
    if( !doc )
        return E_OUTOFMEMORY;

    doc->lpVtbl = &domdoc_vtbl;
    doc->ref = 1;
    doc->async = 0;
    doc->error = S_OK;

    doc->node_unk = create_basic_node( NULL, (IUnknown*)&doc->lpVtbl );
    if(!doc->node_unk)
    {
        HeapFree(GetProcessHeap(), 0, doc);
        return E_FAIL;
    }

    hr = IUnknown_QueryInterface(doc->node_unk, &IID_IXMLDOMNode, (LPVOID*)&doc->node);
    if(FAILED(hr))
    {
        IUnknown_Release(doc->node_unk);
        HeapFree( GetProcessHeap(), 0, doc );
        return E_FAIL;
    }
    /* The ref on doc->node is actually looped back into this object, so release it */
    IXMLDOMNode_Release(doc->node);

    *ppObj = &doc->lpVtbl;

    TRACE("returning iface %p\n", *ppObj);
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
