/*
 * Copyright 2010 Jacek Caban for CodeWeavers
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
#include "winreg.h"
#include "ole2.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct {
    HTMLElement element;

    const IHTMLStyleElementVtbl *lpIHTMLStyleElementVtbl;
} HTMLStyleElement;

#define HTMLSTYLE(x)  (&(x)->lpIHTMLStyleElementVtbl)

#define HTMLSTYLE_THIS(iface) DEFINE_THIS(HTMLStyleElement, IHTMLStyleElement, iface)

static HRESULT WINAPI HTMLStyleElement_QueryInterface(IHTMLStyleElement *iface,
        REFIID riid, void **ppv)
{
    HTMLStyleElement *This = HTMLSTYLE_THIS(iface);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(&This->element.node), riid, ppv);
}

static ULONG WINAPI HTMLStyleElement_AddRef(IHTMLStyleElement *iface)
{
    HTMLStyleElement *This = HTMLSTYLE_THIS(iface);

    return IHTMLDOMNode_AddRef(HTMLDOMNODE(&This->element.node));
}

static ULONG WINAPI HTMLStyleElement_Release(IHTMLStyleElement *iface)
{
    HTMLStyleElement *This = HTMLSTYLE_THIS(iface);

    return IHTMLDOMNode_Release(HTMLDOMNODE(&This->element.node));
}

static HRESULT WINAPI HTMLStyleElement_GetTypeInfoCount(IHTMLStyleElement *iface, UINT *pctinfo)
{
    HTMLStyleElement *This = HTMLSTYLE_THIS(iface);
    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(&This->element.node.dispex), pctinfo);
}

static HRESULT WINAPI HTMLStyleElement_GetTypeInfo(IHTMLStyleElement *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLStyleElement *This = HTMLSTYLE_THIS(iface);
    return IDispatchEx_GetTypeInfo(DISPATCHEX(&This->element.node.dispex), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLStyleElement_GetIDsOfNames(IHTMLStyleElement *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLStyleElement *This = HTMLSTYLE_THIS(iface);
    return IDispatchEx_GetIDsOfNames(DISPATCHEX(&This->element.node.dispex), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLStyleElement_Invoke(IHTMLStyleElement *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLStyleElement *This = HTMLSTYLE_THIS(iface);
    return IDispatchEx_Invoke(DISPATCHEX(&This->element.node.dispex), dispIdMember, riid, lcid, wFlags, pDispParams,
            pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLStyleElement_put_type(IHTMLStyleElement *iface, BSTR v)
{
    HTMLStyleElement *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleElement_get_type(IHTMLStyleElement *iface, BSTR *p)
{
    HTMLStyleElement *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleElement_get_readyState(IHTMLStyleElement *iface, BSTR *p)
{
    HTMLStyleElement *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleElement_put_onreadystatechange(IHTMLStyleElement *iface, VARIANT v)
{
    HTMLStyleElement *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleElement_get_onreadystatechange(IHTMLStyleElement *iface, VARIANT *p)
{
    HTMLStyleElement *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleElement_put_onload(IHTMLStyleElement *iface, VARIANT v)
{
    HTMLStyleElement *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleElement_get_onload(IHTMLStyleElement *iface, VARIANT *p)
{
    HTMLStyleElement *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleElement_put_onerror(IHTMLStyleElement *iface, VARIANT v)
{
    HTMLStyleElement *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleElement_get_onerror(IHTMLStyleElement *iface, VARIANT *p)
{
    HTMLStyleElement *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleElement_get_styleSheet(IHTMLStyleElement *iface, IHTMLStyleSheet **p)
{
    HTMLStyleElement *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleElement_put_disabled(IHTMLStyleElement *iface, VARIANT_BOOL v)
{
    HTMLStyleElement *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleElement_get_disabled(IHTMLStyleElement *iface, VARIANT_BOOL *p)
{
    HTMLStyleElement *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleElement_put_media(IHTMLStyleElement *iface, BSTR v)
{
    HTMLStyleElement *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleElement_get_media(IHTMLStyleElement *iface, BSTR *p)
{
    HTMLStyleElement *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

#undef HTMLSTYLE_THIS

static const IHTMLStyleElementVtbl HTMLStyleElementVtbl = {
    HTMLStyleElement_QueryInterface,
    HTMLStyleElement_AddRef,
    HTMLStyleElement_Release,
    HTMLStyleElement_GetTypeInfoCount,
    HTMLStyleElement_GetTypeInfo,
    HTMLStyleElement_GetIDsOfNames,
    HTMLStyleElement_Invoke,
    HTMLStyleElement_put_type,
    HTMLStyleElement_get_type,
    HTMLStyleElement_get_readyState,
    HTMLStyleElement_put_onreadystatechange,
    HTMLStyleElement_get_onreadystatechange,
    HTMLStyleElement_put_onload,
    HTMLStyleElement_get_onload,
    HTMLStyleElement_put_onerror,
    HTMLStyleElement_get_onerror,
    HTMLStyleElement_get_styleSheet,
    HTMLStyleElement_put_disabled,
    HTMLStyleElement_get_disabled,
    HTMLStyleElement_put_media,
    HTMLStyleElement_get_media
};

#define HTMLSTYLE_NODE_THIS(iface) DEFINE_THIS2(HTMLStyleElement, element.node, iface)

static HRESULT HTMLStyleElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLStyleElement *This = HTMLSTYLE_NODE_THIS(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLSTYLE(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = HTMLSTYLE(This);
    }else if(IsEqualGUID(&IID_IHTMLStyleElement, riid)) {
        TRACE("(%p)->(IID_IHTMLStyleElement %p)\n", This, ppv);
        *ppv = HTMLSTYLE(This);
    }else {
        return HTMLElement_QI(&This->element.node, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static void HTMLStyleElement_destructor(HTMLDOMNode *iface)
{
    HTMLStyleElement *This = HTMLSTYLE_NODE_THIS(iface);

    HTMLElement_destructor(&This->element.node);
}

#undef HTMLSTYLE_NODE_THIS

static const NodeImplVtbl HTMLStyleElementImplVtbl = {
    HTMLStyleElement_QI,
    HTMLStyleElement_destructor,
    HTMLElement_clone
};

static const tid_t HTMLStyleElement_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLStyleElement_tid,
    0
};
static dispex_static_data_t HTMLStyleElement_dispex = {
    NULL,
    DispHTMLStyleElement_tid,
    NULL,
    HTMLStyleElement_iface_tids
};

HTMLElement *HTMLStyleElement_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem)
{
    HTMLStyleElement *ret = heap_alloc_zero(sizeof(*ret));

    ret->lpIHTMLStyleElementVtbl = &HTMLStyleElementVtbl;
    ret->element.node.vtbl = &HTMLStyleElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLStyleElement_dispex);
    return &ret->element;
}
