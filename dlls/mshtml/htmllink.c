 /*
 * Copyright 2012 Jacek Caban for CodeWeavers
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct {
    HTMLElement element;
    IHTMLLinkElement IHTMLLinkElement_iface;
} HTMLLinkElement;

static inline HTMLLinkElement *impl_from_IHTMLLinkElement(IHTMLLinkElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLLinkElement, IHTMLLinkElement_iface);
}

static HRESULT WINAPI HTMLLinkElement_QueryInterface(IHTMLLinkElement *iface,
                                                         REFIID riid, void **ppv)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLLinkElement_AddRef(IHTMLLinkElement *iface)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLLinkElement_Release(IHTMLLinkElement *iface)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLLinkElement_GetTypeInfoCount(IHTMLLinkElement *iface, UINT *pctinfo)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);

    return IDispatchEx_GetTypeInfoCount(&This->element.node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLLinkElement_GetTypeInfo(IHTMLLinkElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);

    return IDispatchEx_GetTypeInfo(&This->element.node.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLLinkElement_GetIDsOfNames(IHTMLLinkElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);

    return IDispatchEx_GetIDsOfNames(&This->element.node.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLLinkElement_Invoke(IHTMLLinkElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);

    return IDispatchEx_Invoke(&This->element.node.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLLinkElement_put_href(IHTMLLinkElement *iface, BSTR v)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLinkElement_get_href(IHTMLLinkElement *iface, BSTR *p)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLinkElement_put_rel(IHTMLLinkElement *iface, BSTR v)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLinkElement_get_rel(IHTMLLinkElement *iface, BSTR *p)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLinkElement_put_rev(IHTMLLinkElement *iface, BSTR v)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLinkElement_get_rev(IHTMLLinkElement *iface, BSTR *p)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLinkElement_put_type(IHTMLLinkElement *iface, BSTR v)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLinkElement_get_type(IHTMLLinkElement *iface, BSTR *p)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLinkElement_get_readyState(IHTMLLinkElement *iface, BSTR *p)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLinkElement_put_onreadystatechange(IHTMLLinkElement *iface, VARIANT v)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLinkElement_get_onreadystatechange(IHTMLLinkElement *iface, VARIANT *p)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLinkElement_put_onload(IHTMLLinkElement *iface, VARIANT v)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLinkElement_get_onload(IHTMLLinkElement *iface, VARIANT *p)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLinkElement_put_onerror(IHTMLLinkElement *iface, VARIANT v)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLinkElement_get_onerror(IHTMLLinkElement *iface, VARIANT *p)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLinkElement_get_styleSheet(IHTMLLinkElement *iface, IHTMLStyleSheet **p)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLinkElement_put_disabled(IHTMLLinkElement *iface, VARIANT_BOOL v)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLinkElement_get_disabled(IHTMLLinkElement *iface, VARIANT_BOOL *p)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLinkElement_put_media(IHTMLLinkElement *iface, BSTR v)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLinkElement_get_media(IHTMLLinkElement *iface, BSTR *p)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLLinkElementVtbl HTMLLinkElementVtbl = {
    HTMLLinkElement_QueryInterface,
    HTMLLinkElement_AddRef,
    HTMLLinkElement_Release,
    HTMLLinkElement_GetTypeInfoCount,
    HTMLLinkElement_GetTypeInfo,
    HTMLLinkElement_GetIDsOfNames,
    HTMLLinkElement_Invoke,
    HTMLLinkElement_put_href,
    HTMLLinkElement_get_href,
    HTMLLinkElement_put_rel,
    HTMLLinkElement_get_rel,
    HTMLLinkElement_put_rev,
    HTMLLinkElement_get_rev,
    HTMLLinkElement_put_type,
    HTMLLinkElement_get_type,
    HTMLLinkElement_get_readyState,
    HTMLLinkElement_put_onreadystatechange,
    HTMLLinkElement_get_onreadystatechange,
    HTMLLinkElement_put_onload,
    HTMLLinkElement_get_onload,
    HTMLLinkElement_put_onerror,
    HTMLLinkElement_get_onerror,
    HTMLLinkElement_get_styleSheet,
    HTMLLinkElement_put_disabled,
    HTMLLinkElement_get_disabled,
    HTMLLinkElement_put_media,
    HTMLLinkElement_get_media
};

static inline HTMLLinkElement *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLLinkElement, element.node);
}

static HRESULT HTMLLinkElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLLinkElement *This = impl_from_HTMLDOMNode(iface);

    if(IsEqualGUID(&IID_IHTMLLinkElement, riid)) {
        TRACE("(%p)->(IID_IHTMLLinkElement %p)\n", This, ppv);
        *ppv = &This->IHTMLLinkElement_iface;
    }else {
        return HTMLElement_QI(&This->element.node, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static void HTMLLinkElement_destructor(HTMLDOMNode *iface)
{
    HTMLLinkElement *This = impl_from_HTMLDOMNode(iface);

    HTMLElement_destructor(&This->element.node);
}

static const NodeImplVtbl HTMLLinkElementImplVtbl = {
    HTMLLinkElement_QI,
    HTMLLinkElement_destructor,
    HTMLElement_clone,
    HTMLElement_handle_event,
    HTMLElement_get_attr_col
};

static const tid_t HTMLLinkElement_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLLinkElement_tid,
    0
};
static dispex_static_data_t HTMLLinkElement_dispex = {
    NULL,
    DispHTMLLinkElement_tid,
    NULL,
    HTMLLinkElement_iface_tids
};

HRESULT HTMLLinkElement_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem, HTMLElement **elem)
{
    HTMLLinkElement *ret;

    ret = heap_alloc_zero(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLLinkElement_iface.lpVtbl = &HTMLLinkElementVtbl;
    ret->element.node.vtbl = &HTMLLinkElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLLinkElement_dispex);

    *elem = &ret->element;
    return S_OK;
}
