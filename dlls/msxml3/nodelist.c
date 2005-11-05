/*
 *    Node list implementation
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
#include "msxml.h"

#include "msxml_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

#ifdef HAVE_LIBXML2

#ifdef HAVE_LIBXSLT

#ifdef HAVE_LIBXSLT_PATTERN_H
#include <libxslt/pattern.h>
#endif
#ifdef HAVE_LIBXSLT_TRANSFORM_H
#include <libxslt/transform.h>
#endif

struct xslt_info {
    xsltTransformContextPtr ctxt;
    xsltCompMatchPtr pattern;
    xsltStylesheetPtr sheet;
};

static void xlst_info_init( struct xslt_info *info )
{
    info->ctxt = NULL;
    info->pattern = NULL;
    info->sheet = NULL;
}

static int create_xslt_parser( struct xslt_info *info, xmlNodePtr node, const xmlChar *str )
{
    info->sheet = xsltNewStylesheet();
    if (!info->sheet)
        return 0;

    info->ctxt = xsltNewTransformContext( info->sheet, node->doc );
    if (!info->ctxt)
        return 0;

    info->pattern = xsltCompilePattern( str, node->doc,
                                        node, info->sheet, info->ctxt );
    if (!info->pattern)
        return 0;
    return 1;
}
 
void free_xslt_info( struct xslt_info *info )
{
    if (info->pattern)
        xsltFreeCompMatchList( info->pattern );
    if (info->sheet)
        xsltFreeStylesheet( info->sheet );
    if (info->ctxt)
        xsltFreeTransformContext( info->ctxt );
}

static HRESULT xslt_next_match( struct xslt_info *info, xmlNodePtr *node )
{
    if (!info->ctxt)
        return S_FALSE;
 
    /* make sure that the current element matches the pattern */
    while ( *node )
    {
        int r;

        r = xsltTestCompMatchList( info->ctxt, *node, info->pattern );
        if ( 1 == r )
        {
            TRACE("Matched %p (%s)\n", *node, (*node)->name );
            return S_OK;
        }
        if (r != 0)
        {
            ERR("Pattern match failed\n");
            return E_FAIL;
        }
        *node = (*node)->next;
    }
    return S_OK;
}

#else

struct xslt_info {
    /* empty */
};

static void xlst_info_init( struct xslt_info *info )
{
}

void free_xslt_info( struct xslt_info *info )
{
}

static int create_xslt_parser( struct xslt_info *info, xmlNodePtr node, const xmlChar *str )
{
    MESSAGE("libxslt was missing at compile time\n");
    return 0;
}

static HRESULT xslt_next_match( struct xslt_info *info, xmlNodePtr *node )
{
    return S_FALSE;
}

#endif

typedef struct _xmlnodelist
{
    const struct IXMLDOMNodeListVtbl *lpVtbl;
    LONG ref;
    xmlNodePtr node;
    xmlNodePtr current;
    struct xslt_info xinfo;
} xmlnodelist;

static inline xmlnodelist *impl_from_IXMLDOMNodeList( IXMLDOMNodeList *iface )
{
    return (xmlnodelist *)((char*)iface - FIELD_OFFSET(xmlnodelist, lpVtbl));
}

static HRESULT WINAPI xmlnodelist_QueryInterface(
    IXMLDOMNodeList *iface,
    REFIID riid,
    void** ppvObject )
{
    TRACE("%p %s %p\n", iface, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IUnknown ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IXMLDOMNodeList ) )
    {
        *ppvObject = iface;
    }
    else
        return E_NOINTERFACE;

    IXMLDOMNodeList_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI xmlnodelist_AddRef(
    IXMLDOMNodeList *iface )
{
    xmlnodelist *This = impl_from_IXMLDOMNodeList( iface );
    return InterlockedIncrement( &This->ref );
}

static ULONG WINAPI xmlnodelist_Release(
    IXMLDOMNodeList *iface )
{
    xmlnodelist *This = impl_from_IXMLDOMNodeList( iface );
    ULONG ref;

    ref = InterlockedDecrement( &This->ref );
    if ( ref == 0 )
    {
        free_xslt_info( &This->xinfo );
        HeapFree( GetProcessHeap(), 0, This );
    }

    return ref;
}

static HRESULT WINAPI xmlnodelist_GetTypeInfoCount(
    IXMLDOMNodeList *iface,
    UINT* pctinfo )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnodelist_GetTypeInfo(
    IXMLDOMNodeList *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo** ppTInfo )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnodelist_GetIDsOfNames(
    IXMLDOMNodeList *iface,
    REFIID riid,
    LPOLESTR* rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID* rgDispId )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnodelist_Invoke(
    IXMLDOMNodeList *iface,
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

static HRESULT WINAPI xmlnodelist_get_item(
        IXMLDOMNodeList* iface,
        long index,
        IXMLDOMNode** listItem)
{
    xmlnodelist *This = impl_from_IXMLDOMNodeList( iface );
    xmlNodePtr curr;
    long nodeIndex = 0;
    HRESULT r;

    TRACE("%p %ld\n", This, index);
 
    *listItem = NULL;

    if (index < 0)
        return S_FALSE;

    curr = This->node;

    while(curr)
    {
        r = xslt_next_match( &This->xinfo, &curr );
        if(FAILED(r) || !curr) return S_FALSE;
        if(nodeIndex++ == index) break;
        curr = curr->next;
    }
    if(!curr) return S_FALSE;

    *listItem = create_node( curr );

    return S_OK;
}

static HRESULT WINAPI xmlnodelist_get_length(
        IXMLDOMNodeList* iface,
        long* listLength)
{

    xmlNodePtr curr;
    long nodeCount = 0;
    HRESULT r;

    xmlnodelist *This = impl_from_IXMLDOMNodeList( iface );

    TRACE("%p\n", This);

    if (This->node == NULL) {
        *listLength = 0;
	return S_OK;
    }
        
    for(curr = This->node; curr; curr = curr->next)
    {
        r = xslt_next_match( &This->xinfo, &curr );
        if(FAILED(r) || !curr) break;
        nodeCount++;
    }

    *listLength = nodeCount;
    return S_OK;
}

static HRESULT WINAPI xmlnodelist_nextNode(
        IXMLDOMNodeList* iface,
        IXMLDOMNode** nextItem)
{
    xmlnodelist *This = impl_from_IXMLDOMNodeList( iface );
    HRESULT r;

    TRACE("%p %p\n", This, nextItem );

    r = xslt_next_match( &This->xinfo, &This->current );
    if (FAILED(r) )
        return r;

    if (!This->current)
        return S_FALSE;

    *nextItem = create_node( This->current );
    This->current = This->current->next;
    return S_OK;
}

static HRESULT WINAPI xmlnodelist_reset(
        IXMLDOMNodeList* iface)
{
    xmlnodelist *This = impl_from_IXMLDOMNodeList( iface );

    TRACE("%p\n", This);
    This->current = This->node;
    return S_OK;
}

static HRESULT WINAPI xmlnodelist__newEnum(
        IXMLDOMNodeList* iface,
        IUnknown** ppUnk)
{
    FIXME("\n");
    return E_NOTIMPL;
}


static const struct IXMLDOMNodeListVtbl xmlnodelist_vtbl =
{
    xmlnodelist_QueryInterface,
    xmlnodelist_AddRef,
    xmlnodelist_Release,
    xmlnodelist_GetTypeInfoCount,
    xmlnodelist_GetTypeInfo,
    xmlnodelist_GetIDsOfNames,
    xmlnodelist_Invoke,
    xmlnodelist_get_item,
    xmlnodelist_get_length,
    xmlnodelist_nextNode,
    xmlnodelist_reset,
    xmlnodelist__newEnum,
};

static xmlnodelist *new_nodelist( xmlNodePtr node )
{
    xmlnodelist *nodelist;

    nodelist = HeapAlloc( GetProcessHeap(), 0, sizeof *nodelist );
    if ( !nodelist )
        return NULL;

    nodelist->lpVtbl = &xmlnodelist_vtbl;
    nodelist->ref = 1;
    nodelist->node = node;
    nodelist->current = node;
    xlst_info_init( &nodelist->xinfo );

    return nodelist;
}

IXMLDOMNodeList* create_nodelist( xmlNodePtr node )
{
    xmlnodelist *nodelist = new_nodelist( node );
    if (!node)
        return NULL;
    return (IXMLDOMNodeList*) &nodelist->lpVtbl;
}

IXMLDOMNodeList* create_filtered_nodelist( xmlNodePtr node, const xmlChar *str )
{
    xmlnodelist *This = new_nodelist( node );

    if (create_xslt_parser( &This->xinfo, node, str ))
        return (IXMLDOMNodeList*) &This->lpVtbl;

    IXMLDOMNodeList_Release( (IXMLDOMNodeList*) &This->lpVtbl );
    return NULL;
}

#endif
