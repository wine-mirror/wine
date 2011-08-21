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
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

struct ns
{
    BSTR prefix;
    BSTR uri;
};

struct nscontext
{
    struct list entry;

    struct ns *ns;
    int   count;
    int   max_alloc;
};

#define DEFAULT_PREFIX_ALLOC_COUNT 16

static const WCHAR xmlW[] = {'x','m','l',0};
static const WCHAR xmluriW[] = {'h','t','t','p',':','/','/','w','w','w','.','w','3','.','o','r','g',
    '/','X','M','L','/','1','9','9','8','/','n','a','m','e','s','p','a','c','e',0};

typedef struct
{
    IMXNamespaceManager   IMXNamespaceManager_iface;
    IVBMXNamespaceManager IVBMXNamespaceManager_iface;
    LONG ref;

    struct list ctxts;
} namespacemanager;

static inline namespacemanager *impl_from_IMXNamespaceManager( IMXNamespaceManager *iface )
{
    return CONTAINING_RECORD(iface, namespacemanager, IMXNamespaceManager_iface);
}

static inline namespacemanager *impl_from_IVBMXNamespaceManager( IVBMXNamespaceManager *iface )
{
    return CONTAINING_RECORD(iface, namespacemanager, IVBMXNamespaceManager_iface);
}

static HRESULT declare_prefix(struct nscontext *ctxt, const WCHAR *prefix, const WCHAR *uri)
{
    if (ctxt->count == ctxt->max_alloc)
    {
        ctxt->max_alloc *= 2;
        ctxt->ns = heap_realloc(ctxt->ns, ctxt->max_alloc*sizeof(*ctxt->ns));
    }

    ctxt->ns[ctxt->count].prefix = SysAllocString(prefix);
    ctxt->ns[ctxt->count].uri = SysAllocString(uri);
    ctxt->count++;

    return S_OK;
}

/* returned stored pointer, caller needs to copy it */
static HRESULT get_declared_prefix_idx(const struct nscontext *ctxt, LONG index, BSTR *prefix)
{
    if (index >= ctxt->count || index < 0) return E_FAIL;

    if (index > 0) index = ctxt->count - index;
    *prefix = ctxt->ns[index].prefix;

    return S_OK;
}

static struct nscontext* alloc_ns_context(void)
{
    struct nscontext *ctxt;

    ctxt = heap_alloc(sizeof(*ctxt));
    if (!ctxt) return NULL;

    ctxt->count = 0;
    ctxt->max_alloc = DEFAULT_PREFIX_ALLOC_COUNT;
    ctxt->ns = heap_alloc(ctxt->max_alloc*sizeof(*ctxt->ns));

    /* first allocated prefix is always 'xml' */
    ctxt->ns[0].prefix = SysAllocString(xmlW);
    ctxt->ns[0].uri = SysAllocString(xmluriW);
    ctxt->count++;

    return ctxt;
}

static void free_ns_context(struct nscontext *ctxt)
{
    int i;

    for (i = 0; i < ctxt->count; i++)
    {
        SysFreeString(ctxt->ns[i].prefix);
        SysFreeString(ctxt->ns[i].uri);
    }

    heap_free(ctxt->ns);
    heap_free(ctxt);
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
    static const WCHAR xmlnsW[] = {'x','m','l','n','s',0};

    namespacemanager *This = impl_from_IMXNamespaceManager( iface );
    struct nscontext *ctxt;

    TRACE("(%p)->(%s %s)\n", This, debugstr_w(prefix), debugstr_w(namespaceURI));

    if (!prefix) return E_FAIL;

    if (!strcmpW(prefix, xmlW) || !strcmpW(prefix, xmlnsW) || (prefix && !namespaceURI))
        return E_INVALIDARG;

    ctxt = LIST_ENTRY(list_head(&This->ctxts), struct nscontext, entry);
    return declare_prefix(ctxt, prefix, namespaceURI);
}

static HRESULT WINAPI namespacemanager_getDeclaredPrefix(IMXNamespaceManager *iface,
    LONG index, WCHAR *prefix, int *prefix_len)
{
    namespacemanager *This = impl_from_IMXNamespaceManager( iface );
    struct nscontext *ctxt;
    HRESULT hr;
    BSTR prfx;

    TRACE("(%p)->(%d %p %p)\n", This, index, prefix, prefix_len);

    if (!prefix_len) return E_POINTER;

    ctxt = LIST_ENTRY(list_head(&This->ctxts), struct nscontext, entry);
    hr = get_declared_prefix_idx(ctxt, index, &prfx);
    if (hr != S_OK) return hr;

    if (prefix)
    {
        if (*prefix_len < (INT)SysStringLen(prfx)) return E_XML_BUFFERTOOSMALL;
        strcpyW(prefix, prfx);
    }

    *prefix_len = SysStringLen(prfx);

    return S_OK;
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
    {
        struct nscontext *ctxt, *ctxt2;

        LIST_FOR_EACH_ENTRY_SAFE(ctxt, ctxt2, &This->ctxts, struct nscontext, entry)
        {
            list_remove(&ctxt->entry);
            free_ns_context(ctxt);
        }

        heap_free( This );
    }

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
    return IMXNamespaceManager_declarePrefix(&This->IMXNamespaceManager_iface, prefix, namespaceURI);
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
    struct nscontext *ctxt;

    TRACE("(%p, %p)\n", outer, obj);

    ns = heap_alloc( sizeof (*ns) );
    if( !ns )
        return E_OUTOFMEMORY;

    ns->IMXNamespaceManager_iface.lpVtbl = &MXNamespaceManagerVtbl;
    ns->IVBMXNamespaceManager_iface.lpVtbl = &VBMXNamespaceManagerVtbl;
    ns->ref = 1;

    list_init(&ns->ctxts);
    ctxt = alloc_ns_context();
    list_add_head(&ns->ctxts, &ctxt->entry);

    *obj = &ns->IMXNamespaceManager_iface;

    TRACE("returning iface %p\n", *obj);

    return S_OK;
}
