/*
 *    XSLTemplate/XSLProcessor support
 *
 * Copyright 2011 Nikolay Sivov for CodeWeavers
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

typedef struct _xsltemplate
{
    IXSLTemplate IXSLTemplate_iface;
    LONG ref;

    IXMLDOMNode *node;
} xsltemplate;

static inline xsltemplate *impl_from_IXSLTemplate( IXSLTemplate *iface )
{
    return CONTAINING_RECORD(iface, xsltemplate, IXSLTemplate_iface);
}

static void xsltemplate_set_node( xsltemplate *This, IXMLDOMNode *node )
{
    if (This->node) IXMLDOMNode_Release(This->node);
    This->node = node;
    if (node) IXMLDOMNode_AddRef(node);
}

static HRESULT WINAPI xsltemplate_QueryInterface(
    IXSLTemplate *iface,
    REFIID riid,
    void** ppvObject )
{
    xsltemplate *This = impl_from_IXSLTemplate( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IXSLTemplate ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppvObject);
    return S_OK;
}

static ULONG WINAPI xsltemplate_AddRef( IXSLTemplate *iface )
{
    xsltemplate *This = impl_from_IXSLTemplate( iface );
    return InterlockedIncrement( &This->ref );
}

static ULONG WINAPI xsltemplate_Release( IXSLTemplate *iface )
{
    xsltemplate *This = impl_from_IXSLTemplate( iface );
    ULONG ref;

    ref = InterlockedDecrement( &This->ref );
    if ( ref == 0 )
    {
        if (This->node) IXMLDOMNode_Release( This->node );
        heap_free( This );
    }

    return ref;
}

static HRESULT WINAPI xsltemplate_GetTypeInfoCount( IXSLTemplate *iface, UINT* pctinfo )
{
    xsltemplate *This = impl_from_IXSLTemplate( iface );

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI xsltemplate_GetTypeInfo(
    IXSLTemplate *iface,
    UINT iTInfo, LCID lcid,
    ITypeInfo** ppTInfo )
{
    xsltemplate *This = impl_from_IXSLTemplate( iface );

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    return get_typeinfo(IXSLTemplate_tid, ppTInfo);
}

static HRESULT WINAPI xsltemplate_GetIDsOfNames(
    IXSLTemplate *iface,
    REFIID riid, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgDispId )
{
    xsltemplate *This = impl_from_IXSLTemplate( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IXSLTemplate_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI xsltemplate_Invoke(
    IXSLTemplate *iface,
    DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo, UINT* puArgErr )
{
    xsltemplate *This = impl_from_IXSLTemplate( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IXSLTemplate_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
       hr = ITypeInfo_Invoke(typeinfo, &This->IXSLTemplate_iface, dispIdMember,
                wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI xsltemplate_putref_stylesheet( IXSLTemplate *iface,
    IXMLDOMNode *node)
{
    xsltemplate *This = impl_from_IXSLTemplate( iface );

    TRACE("(%p)->(%p)\n", This, node);

    if (!node)
    {
        xsltemplate_set_node(This, NULL);
        return S_OK;
    }

    /* FIXME: test for document type */
    xsltemplate_set_node(This, node);

    return S_OK;
}

static HRESULT WINAPI xsltemplate_get_stylesheet( IXSLTemplate *iface,
    IXMLDOMNode **node)
{
    xsltemplate *This = impl_from_IXSLTemplate( iface );

    FIXME("(%p)->(%p): stub\n", This, node);
    return E_NOTIMPL;
}

static HRESULT WINAPI xsltemplate_createProcessor( IXSLTemplate *iface,
    IXSLProcessor **processor)
{
    xsltemplate *This = impl_from_IXSLTemplate( iface );

    FIXME("(%p)->(%p)\n", This, processor);
    return E_NOTIMPL;
}

static const struct IXSLTemplateVtbl xsltemplate_vtbl =
{
    xsltemplate_QueryInterface,
    xsltemplate_AddRef,
    xsltemplate_Release,
    xsltemplate_GetTypeInfoCount,
    xsltemplate_GetTypeInfo,
    xsltemplate_GetIDsOfNames,
    xsltemplate_Invoke,

    xsltemplate_putref_stylesheet,
    xsltemplate_get_stylesheet,
    xsltemplate_createProcessor
};

HRESULT XSLTemplate_create(IUnknown *pUnkOuter, void **ppObj)
{
    xsltemplate *This;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    if(pUnkOuter) FIXME("support aggregation, outer\n");

    This = heap_alloc( sizeof (*This) );
    if(!This)
        return E_OUTOFMEMORY;

    This->IXSLTemplate_iface.lpVtbl = &xsltemplate_vtbl;
    This->ref = 1;
    This->node = NULL;

    *ppObj = &This->IXSLTemplate_iface;

    TRACE("returning iface %p\n", *ppObj);

    return S_OK;
}

#else

HRESULT XSLTemplate_create(IUnknown *pUnkOuter, void **ppObj)
{
    MESSAGE("This program tried to use a XSLTemplate object, but\n"
            "libxml2 support was not present at compile time.\n");
    return E_NOTIMPL;
}

#endif
