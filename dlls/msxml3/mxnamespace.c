/*
 *    IMXNamespaceManager implementation
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
#define NONAMELESSUNION

#include "config.h"

#include <stdarg.h>
#ifdef HAVE_LIBXML2
# include <libxml/parser.h>
# include <libxml/xmlerror.h>
# include <libxml/encoding.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml6.h"

#include "msxml_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

typedef struct
{
    IMXNamespaceManager   IMXNamespaceManager_iface;
    IVBMXNamespaceManager IVBMXNamespaceManager_iface;
    LONG ref;
} namespacemanager;

static inline namespacemanager *impl_from_IMXNamespaceManager( IMXNamespaceManager *iface )
{
    return CONTAINING_RECORD(iface, namespacemanager, IMXNamespaceManager_iface);
}

static inline namespacemanager *impl_from_IVBMXNamespaceManager( IVBMXNamespaceManager *iface )
{
    return CONTAINING_RECORD(iface, namespacemanager, IVBMXNamespaceManager_iface);
}

static HRESULT WINAPI namespacemanager_QueryInterface(IMXNamespaceManager *iface, REFIID riid, void **ppvObject)
{
    namespacemanager *This = impl_from_IMXNamespaceManager( iface );
    return IVBMXNamespaceManager_QueryInterface(&This->IVBMXNamespaceManager_iface, riid, ppvObject);
}

static ULONG WINAPI namespacemanager_AddRef(IMXNamespaceManager *iface)
{
    namespacemanager *This = impl_from_IMXNamespaceManager( iface );
    return IVBMXNamespaceManager_AddRef(&This->IVBMXNamespaceManager_iface);
}

static ULONG WINAPI namespacemanager_Release(IMXNamespaceManager *iface)
{
    namespacemanager *This = impl_from_IMXNamespaceManager( iface );
    return IVBMXNamespaceManager_Release(&This->IVBMXNamespaceManager_iface);
}

static HRESULT WINAPI namespacemanager_putAllowOverride(IMXNamespaceManager *iface,
    VARIANT_BOOL override)
{
    namespacemanager *This = impl_from_IMXNamespaceManager( iface );
    return IVBMXNamespaceManager_put_allowOverride(&This->IVBMXNamespaceManager_iface, override);
}

static HRESULT WINAPI namespacemanager_getAllowOverride(IMXNamespaceManager *iface,
    VARIANT_BOOL *override)
{
    namespacemanager *This = impl_from_IMXNamespaceManager( iface );
    return IVBMXNamespaceManager_get_allowOverride(&This->IVBMXNamespaceManager_iface, override);
}

static HRESULT WINAPI namespacemanager_reset(IMXNamespaceManager *iface)
{
    namespacemanager *This = impl_from_IMXNamespaceManager( iface );
    return IVBMXNamespaceManager_reset(&This->IVBMXNamespaceManager_iface);
}

static HRESULT WINAPI namespacemanager_pushContext(IMXNamespaceManager *iface)
{
    namespacemanager *This = impl_from_IMXNamespaceManager( iface );
    return IVBMXNamespaceManager_pushContext(&This->IVBMXNamespaceManager_iface);
}

static HRESULT WINAPI namespacemanager_pushNodeContext(IMXNamespaceManager *iface,
    IXMLDOMNode *node, VARIANT_BOOL deep)
{
    namespacemanager *This = impl_from_IMXNamespaceManager( iface );
    return IVBMXNamespaceManager_pushNodeContext(&This->IVBMXNamespaceManager_iface, node, deep);
}

static HRESULT WINAPI namespacemanager_popContext(IMXNamespaceManager *iface)
{
    namespacemanager *This = impl_from_IMXNamespaceManager( iface );
    return IVBMXNamespaceManager_popContext(&This->IVBMXNamespaceManager_iface);
}

static HRESULT WINAPI namespacemanager_declarePrefix(IMXNamespaceManager *iface,
    const WCHAR *prefix, const WCHAR *namespaceURI)
{
    namespacemanager *This = impl_from_IMXNamespaceManager( iface );
    FIXME("(%p)->(%s %s): stub\n", This, debugstr_w(prefix), debugstr_w(namespaceURI));
    return E_NOTIMPL;
}

static HRESULT WINAPI namespacemanager_getDeclaredPrefix(IMXNamespaceManager *iface,
    LONG index, WCHAR *prefix, int *prefix_len)
{
    namespacemanager *This = impl_from_IMXNamespaceManager( iface );
    FIXME("(%p)->(%d %p %p): stub\n", This, index, prefix, prefix_len);
    return E_NOTIMPL;
}

static HRESULT WINAPI namespacemanager_getPrefix(IMXNamespaceManager *iface,
    const WCHAR *uri, LONG index, WCHAR *prefix, int *prefix_len)
{
    namespacemanager *This = impl_from_IMXNamespaceManager( iface );
    FIXME("(%p)->(%s %d %p %p): stub\n", This, debugstr_w(uri), index, prefix, prefix_len);
    return E_NOTIMPL;
}

static HRESULT WINAPI namespacemanager_getURI(IMXNamespaceManager *iface,
    const WCHAR *prefix, IXMLDOMNode *node, WCHAR *uri, int *uri_len)
{
    namespacemanager *This = impl_from_IMXNamespaceManager( iface );
    FIXME("(%p)->(%s %p %p %p): stub\n", This, debugstr_w(prefix), node, uri, uri_len);
    return E_NOTIMPL;
}

static const struct IMXNamespaceManagerVtbl MXNamespaceManagerVtbl =
{
    namespacemanager_QueryInterface,
    namespacemanager_AddRef,
    namespacemanager_Release,
    namespacemanager_putAllowOverride,
    namespacemanager_getAllowOverride,
    namespacemanager_reset,
    namespacemanager_pushContext,
    namespacemanager_pushNodeContext,
    namespacemanager_popContext,
    namespacemanager_declarePrefix,
    namespacemanager_getDeclaredPrefix,
    namespacemanager_getPrefix,
    namespacemanager_getURI
};

static HRESULT WINAPI vbnamespacemanager_QueryInterface(IVBMXNamespaceManager *iface, REFIID riid, void **ppvObject)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IMXNamespaceManager) ||
         IsEqualGUID( riid, &IID_IUnknown) )
    {
        *ppvObject = &This->IMXNamespaceManager_iface;
    }
    else if ( IsEqualGUID( riid, &IID_IVBMXNamespaceManager) ||
              IsEqualGUID( riid, &IID_IDispatch) )
    {
        *ppvObject = &This->IVBMXNamespaceManager_iface;
    }
    else
    {
        TRACE("Unsupported interface %s\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IVBMXNamespaceManager_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI vbnamespacemanager_AddRef(IVBMXNamespaceManager *iface)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    ULONG ref = InterlockedIncrement( &This->ref );
    TRACE("(%p)->(%u)\n", This, ref );
    return ref;
}

static ULONG WINAPI vbnamespacemanager_Release(IVBMXNamespaceManager *iface)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    ULONG ref = InterlockedDecrement( &This->ref );

    TRACE("(%p)->(%u)\n", This, ref );

    if ( ref == 0 )
        heap_free( This );

    return ref;
}

static HRESULT WINAPI vbnamespacemanager_GetTypeInfoCount(IVBMXNamespaceManager *iface, UINT *pctinfo)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );

    TRACE("(%p)->(%p)\n", This, pctinfo);
    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI vbnamespacemanager_GetTypeInfo(IVBMXNamespaceManager *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    return get_typeinfo(IVBMXNamespaceManager_tid, ppTInfo);
}

static HRESULT WINAPI vbnamespacemanager_GetIDsOfNames(IVBMXNamespaceManager *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IVBMXNamespaceManager_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI vbnamespacemanager_Invoke(IVBMXNamespaceManager *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IVBMXNamespaceManager_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &This->IVBMXNamespaceManager_iface, dispIdMember, wFlags,
                pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI vbnamespacemanager_put_allowOverride(IVBMXNamespaceManager *iface,
    VARIANT_BOOL override)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    FIXME("(%p)->(%d): stub\n", This, override);
    return E_NOTIMPL;
}

static HRESULT WINAPI vbnamespacemanager_get_allowOverride(IVBMXNamespaceManager *iface,
    VARIANT_BOOL *override)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    FIXME("(%p)->(%p): stub\n", This, override);
    return E_NOTIMPL;
}

static HRESULT WINAPI vbnamespacemanager_reset(IVBMXNamespaceManager *iface)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI vbnamespacemanager_pushContext(IVBMXNamespaceManager *iface)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI vbnamespacemanager_pushNodeContext(IVBMXNamespaceManager *iface,
    IXMLDOMNode *node, VARIANT_BOOL deep)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    FIXME("(%p)->(%p %d): stub\n", This, node, deep);
    return E_NOTIMPL;
}

static HRESULT WINAPI vbnamespacemanager_popContext(IVBMXNamespaceManager *iface)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI vbnamespacemanager_declarePrefix(IVBMXNamespaceManager *iface,
    BSTR prefix, BSTR namespaceURI)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    FIXME("(%p)->(%s %s): stub\n", This, debugstr_w(prefix), debugstr_w(namespaceURI));
    return E_NOTIMPL;
}

static HRESULT WINAPI vbnamespacemanager_getDeclaredPrefixes(IVBMXNamespaceManager *iface,
    IMXNamespacePrefixes** prefixes)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    FIXME("(%p)->(%p): stub\n", This, prefixes);
    return E_NOTIMPL;
}

static HRESULT WINAPI vbnamespacemanager_getPrefixes(IVBMXNamespaceManager *iface,
    BSTR namespaceURI, IMXNamespacePrefixes** prefixes)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    FIXME("(%p)->(%s %p): stub\n", This, debugstr_w(namespaceURI), prefixes);
    return E_NOTIMPL;
}

static HRESULT WINAPI vbnamespacemanager_getURI(IVBMXNamespaceManager *iface,
    BSTR prefix, VARIANT* uri)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    FIXME("(%p)->(%s %p): stub\n", This, debugstr_w(prefix), uri);
    return E_NOTIMPL;
}

static HRESULT WINAPI vbnamespacemanager_getURIFromNode(IVBMXNamespaceManager *iface,
    BSTR prefix, IXMLDOMNode *node, VARIANT *uri)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    FIXME("(%p)->(%s %p %p): stub\n", This, debugstr_w(prefix), node, uri);
    return E_NOTIMPL;
}

static const struct IVBMXNamespaceManagerVtbl VBMXNamespaceManagerVtbl =
{
    vbnamespacemanager_QueryInterface,
    vbnamespacemanager_AddRef,
    vbnamespacemanager_Release,
    vbnamespacemanager_GetTypeInfoCount,
    vbnamespacemanager_GetTypeInfo,
    vbnamespacemanager_GetIDsOfNames,
    vbnamespacemanager_Invoke,
    vbnamespacemanager_put_allowOverride,
    vbnamespacemanager_get_allowOverride,
    vbnamespacemanager_reset,
    vbnamespacemanager_pushContext,
    vbnamespacemanager_pushNodeContext,
    vbnamespacemanager_popContext,
    vbnamespacemanager_declarePrefix,
    vbnamespacemanager_getDeclaredPrefixes,
    vbnamespacemanager_getPrefixes,
    vbnamespacemanager_getURI,
    vbnamespacemanager_getURIFromNode
};

HRESULT MXNamespaceManager_create(IUnknown *outer, void **obj)
{
    namespacemanager *ns;

    TRACE("(%p, %p)\n", outer, obj);

    ns = heap_alloc( sizeof (*ns) );
    if( !ns )
        return E_OUTOFMEMORY;

    ns->IMXNamespaceManager_iface.lpVtbl = &MXNamespaceManagerVtbl;
    ns->IVBMXNamespaceManager_iface.lpVtbl = &VBMXNamespaceManagerVtbl;
    ns->ref = 1;

    *obj = &ns->IMXNamespaceManager_iface;

    TRACE("returning iface %p\n", *obj);

    return S_OK;
}
