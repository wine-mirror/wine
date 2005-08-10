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
#include "msxml.h"

#include "msxml_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

#ifdef HAVE_LIBXML2

typedef struct _domelem
{
    const struct IXMLDOMElementVtbl *lpVtbl;
    LONG ref;
    IXMLDOMNode *node;
} domelem;

static inline domelem *impl_from_IXMLDOMElement( IXMLDOMElement *iface )
{
    return (domelem *)((char*)iface - FIELD_OFFSET(domelem, lpVtbl));
}

static HRESULT WINAPI domelem_QueryInterface(
    IXMLDOMElement *iface,
    REFIID riid,
    void** ppvObject )
{
    TRACE("%p %p %p\n", iface, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IXMLDOMElement ) ||
         IsEqualGUID( riid, &IID_IUnknown ) ||
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

static ULONG WINAPI domelem_AddRef(
    IXMLDOMElement *iface )
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return InterlockedIncrement( &This->ref );
}

static ULONG WINAPI domelem_Release(
    IXMLDOMElement *iface )
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    ULONG ref;

    ref = InterlockedDecrement( &This->ref );
    if ( ref == 0 )
    {
        IXMLDOMNode_Release( This->node );
        HeapFree( GetProcessHeap(), 0, This );
    }

    return ref;
}

static HRESULT WINAPI domelem_GetTypeInfoCount(
    IXMLDOMElement *iface,
    UINT* pctinfo )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_GetTypeInfo(
    IXMLDOMElement *iface,
    UINT iTInfo, LCID lcid,
    ITypeInfo** ppTInfo )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_GetIDsOfNames(
    IXMLDOMElement *iface,
    REFIID riid, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgDispId )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_Invoke(
    IXMLDOMElement *iface,
    DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo, UINT* puArgErr )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_get_nodeName(
    IXMLDOMElement *iface,
    BSTR* p )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_get_nodeValue(
    IXMLDOMElement *iface,
    VARIANT* var1 )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_put_nodeValue(
    IXMLDOMElement *iface,
    VARIANT var1 )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_get_nodeType(
    IXMLDOMElement *iface,
    DOMNodeType* domNodeType )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_get_parentNode(
    IXMLDOMElement *iface,
    IXMLDOMNode** parent )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_get_childNodes(
    IXMLDOMElement *iface,
    IXMLDOMNodeList** outList)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_get_firstChild(
    IXMLDOMElement *iface,
    IXMLDOMNode** domNode)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_get_lastChild(
    IXMLDOMElement *iface,
    IXMLDOMNode** domNode)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_get_previousSibling(
    IXMLDOMElement *iface,
    IXMLDOMNode** domNode)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_get_nextSibling(
    IXMLDOMElement *iface,
    IXMLDOMNode** domNode)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_get_attributes(
    IXMLDOMElement *iface,
    IXMLDOMNamedNodeMap** attributeMap)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_insertBefore(
    IXMLDOMElement *iface,
    IXMLDOMNode* newNode, VARIANT var1,
    IXMLDOMNode** outOldNode)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_replaceChild(
    IXMLDOMElement *iface,
    IXMLDOMNode* newNode,
    IXMLDOMNode* oldNode,
    IXMLDOMNode** outOldNode)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_removeChild(
    IXMLDOMElement *iface,
    IXMLDOMNode* domNode, IXMLDOMNode** oldNode)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_appendChild(
    IXMLDOMElement *iface,
    IXMLDOMNode* newNode, IXMLDOMNode** outNewNode)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_hasChildNodes(
    IXMLDOMElement *iface,
    VARIANT_BOOL* pbool)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_get_ownerDocument(
    IXMLDOMElement *iface,
    IXMLDOMDocument** domDocument)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_cloneNode(
    IXMLDOMElement *iface,
    VARIANT_BOOL pbool, IXMLDOMNode** outNode)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_get_nodeTypeString(
    IXMLDOMElement *iface,
    BSTR* p)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_get_text(
    IXMLDOMElement *iface,
    BSTR* p)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_put_text(
    IXMLDOMElement *iface,
    BSTR p)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_get_specified(
    IXMLDOMElement *iface,
    VARIANT_BOOL* pbool)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_get_definition(
    IXMLDOMElement *iface,
    IXMLDOMNode** domNode)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_get_nodeTypedValue(
    IXMLDOMElement *iface,
    VARIANT* var1)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_put_nodeTypedValue(
    IXMLDOMElement *iface,
    VARIANT var1)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_get_dataType(
    IXMLDOMElement *iface,
    VARIANT* var1)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_put_dataType(
    IXMLDOMElement *iface,
    BSTR p)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_get_xml(
    IXMLDOMElement *iface,
    BSTR* p)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_transformNode(
    IXMLDOMElement *iface,
    IXMLDOMNode* domNode, BSTR* p)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_selectNodes(
    IXMLDOMElement *iface,
    BSTR p, IXMLDOMNodeList** outList)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_selectSingleNode(
    IXMLDOMElement *iface,
    BSTR p, IXMLDOMNode** outNode)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_get_parsed(
    IXMLDOMElement *iface,
    VARIANT_BOOL* pbool)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_get_namespaceURI(
    IXMLDOMElement *iface,
    BSTR* p)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_get_prefix(
    IXMLDOMElement *iface,
    BSTR* p)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_get_baseName(
    IXMLDOMElement *iface,
    BSTR* p)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_transformNodeToObject(
    IXMLDOMElement *iface,
    IXMLDOMNode* domNode, VARIANT var1)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_get_tagName(
    IXMLDOMElement *iface,
    BSTR* p)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_getAttribute(
    IXMLDOMElement *iface,
    BSTR p, VARIANT* var)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_setAttribute(
    IXMLDOMElement *iface,
    BSTR p, VARIANT var)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_removeAttribute(
    IXMLDOMElement *iface,
    BSTR p)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_getAttributeNode(
    IXMLDOMElement *iface,
    BSTR p, IXMLDOMAttribute** attributeNode )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_setAttributeNode(
    IXMLDOMElement *iface,
    IXMLDOMAttribute* domAttribute,
    IXMLDOMAttribute** attributeNode)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_removeAttributeNode(
    IXMLDOMElement *iface,
    IXMLDOMAttribute* domAttribute,
    IXMLDOMAttribute** attributeNode)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_getElementsByTagName(
    IXMLDOMElement *iface,
    BSTR p, IXMLDOMNodeList** resultList)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_normalize(
    IXMLDOMElement *iface )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const struct IXMLDOMElementVtbl domelem_vtbl =
{
    domelem_QueryInterface,
    domelem_AddRef,
    domelem_Release,
    domelem_GetTypeInfoCount,
    domelem_GetTypeInfo,
    domelem_GetIDsOfNames,
    domelem_Invoke,
    domelem_get_nodeName,
    domelem_get_nodeValue,
    domelem_put_nodeValue,
    domelem_get_nodeType,
    domelem_get_parentNode,
    domelem_get_childNodes,
    domelem_get_firstChild,
    domelem_get_lastChild,
    domelem_get_previousSibling,
    domelem_get_nextSibling,
    domelem_get_attributes,
    domelem_insertBefore,
    domelem_replaceChild,
    domelem_removeChild,
    domelem_appendChild,
    domelem_hasChildNodes,
    domelem_get_ownerDocument,
    domelem_cloneNode,
    domelem_get_nodeTypeString,
    domelem_get_text,
    domelem_put_text,
    domelem_get_specified,
    domelem_get_definition,
    domelem_get_nodeTypedValue,
    domelem_put_nodeTypedValue,
    domelem_get_dataType,
    domelem_put_dataType,
    domelem_get_xml,
    domelem_transformNode,
    domelem_selectNodes,
    domelem_selectSingleNode,
    domelem_get_parsed,
    domelem_get_namespaceURI,
    domelem_get_prefix,
    domelem_get_baseName,
    domelem_transformNodeToObject,
    domelem_get_tagName,
    domelem_getAttribute,
    domelem_setAttribute,
    domelem_removeAttribute,
    domelem_getAttributeNode,
    domelem_setAttributeNode,
    domelem_removeAttributeNode,
    domelem_getElementsByTagName,
    domelem_normalize,
};

IXMLDOMElement* create_element( xmlNodePtr element )
{
    domelem *This;

    This = HeapAlloc( GetProcessHeap(), 0, sizeof *This );
    if ( !This )
        return NULL;

    This->lpVtbl = &domelem_vtbl;
    This->node = create_element_node( element );
    This->ref = 1;

    if ( !This->node )
    {
        HeapFree( GetProcessHeap(), 0, This );
        return NULL;
    }

    return (IXMLDOMElement*) &This->lpVtbl;
}

#endif
