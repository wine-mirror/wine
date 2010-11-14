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

    const IHTMLEmbedElementVtbl *lpIHTMLEmbedElementVtbl;
} HTMLEmbedElement;

#define HTMLEMBED(x)  (&(x)->lpIHTMLEmbedElementVtbl)

#define HTMLEMBED_THIS(iface) DEFINE_THIS(HTMLEmbedElement, IHTMLEmbedElement, iface)

static HRESULT WINAPI HTMLEmbedElement_QueryInterface(IHTMLEmbedElement *iface,
        REFIID riid, void **ppv)
{
    HTMLEmbedElement *This = HTMLEMBED_THIS(iface);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(&This->element.node), riid, ppv);
}

static ULONG WINAPI HTMLEmbedElement_AddRef(IHTMLEmbedElement *iface)
{
    HTMLEmbedElement *This = HTMLEMBED_THIS(iface);

    return IHTMLDOMNode_AddRef(HTMLDOMNODE(&This->element.node));
}

static ULONG WINAPI HTMLEmbedElement_Release(IHTMLEmbedElement *iface)
{
    HTMLEmbedElement *This = HTMLEMBED_THIS(iface);

    return IHTMLDOMNode_Release(HTMLDOMNODE(&This->element.node));
}

static HRESULT WINAPI HTMLEmbedElement_GetTypeInfoCount(IHTMLEmbedElement *iface, UINT *pctinfo)
{
    HTMLEmbedElement *This = HTMLEMBED_THIS(iface);
    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(&This->element.node.dispex), pctinfo);
}

static HRESULT WINAPI HTMLEmbedElement_GetTypeInfo(IHTMLEmbedElement *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLEmbedElement *This = HTMLEMBED_THIS(iface);
    return IDispatchEx_GetTypeInfo(DISPATCHEX(&This->element.node.dispex), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLEmbedElement_GetIDsOfNames(IHTMLEmbedElement *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLEmbedElement *This = HTMLEMBED_THIS(iface);
    return IDispatchEx_GetIDsOfNames(DISPATCHEX(&This->element.node.dispex), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLEmbedElement_Invoke(IHTMLEmbedElement *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLEmbedElement *This = HTMLEMBED_THIS(iface);
    return IDispatchEx_Invoke(DISPATCHEX(&This->element.node.dispex), dispIdMember, riid, lcid, wFlags, pDispParams,
            pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLEmbedElement_put_hidden(IHTMLEmbedElement *iface, BSTR v)
{
    HTMLEmbedElement *This = HTMLEMBED_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_get_hidden(IHTMLEmbedElement *iface, BSTR *p)
{
    HTMLEmbedElement *This = HTMLEMBED_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_get_palete(IHTMLEmbedElement *iface, BSTR *p)
{
    HTMLEmbedElement *This = HTMLEMBED_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_get_pluginspage(IHTMLEmbedElement *iface, BSTR *p)
{
    HTMLEmbedElement *This = HTMLEMBED_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_put_src(IHTMLEmbedElement *iface, BSTR v)
{
    HTMLEmbedElement *This = HTMLEMBED_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_get_src(IHTMLEmbedElement *iface, BSTR *p)
{
    HTMLEmbedElement *This = HTMLEMBED_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_put_units(IHTMLEmbedElement *iface, BSTR v)
{
    HTMLEmbedElement *This = HTMLEMBED_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_get_units(IHTMLEmbedElement *iface, BSTR *p)
{
    HTMLEmbedElement *This = HTMLEMBED_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_put_name(IHTMLEmbedElement *iface, BSTR v)
{
    HTMLEmbedElement *This = HTMLEMBED_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_get_name(IHTMLEmbedElement *iface, BSTR *p)
{
    HTMLEmbedElement *This = HTMLEMBED_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_put_width(IHTMLEmbedElement *iface, VARIANT v)
{
    HTMLEmbedElement *This = HTMLEMBED_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_get_width(IHTMLEmbedElement *iface, VARIANT *p)
{
    HTMLEmbedElement *This = HTMLEMBED_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_put_height(IHTMLEmbedElement *iface, VARIANT v)
{
    HTMLEmbedElement *This = HTMLEMBED_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_get_height(IHTMLEmbedElement *iface, VARIANT *p)
{
    HTMLEmbedElement *This = HTMLEMBED_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

#undef HTMLEMBED_THIS

static const IHTMLEmbedElementVtbl HTMLEmbedElementVtbl = {
    HTMLEmbedElement_QueryInterface,
    HTMLEmbedElement_AddRef,
    HTMLEmbedElement_Release,
    HTMLEmbedElement_GetTypeInfoCount,
    HTMLEmbedElement_GetTypeInfo,
    HTMLEmbedElement_GetIDsOfNames,
    HTMLEmbedElement_Invoke,
    HTMLEmbedElement_put_hidden,
    HTMLEmbedElement_get_hidden,
    HTMLEmbedElement_get_palete,
    HTMLEmbedElement_get_pluginspage,
    HTMLEmbedElement_put_src,
    HTMLEmbedElement_get_src,
    HTMLEmbedElement_put_units,
    HTMLEmbedElement_get_units,
    HTMLEmbedElement_put_name,
    HTMLEmbedElement_get_name,
    HTMLEmbedElement_put_width,
    HTMLEmbedElement_get_width,
    HTMLEmbedElement_put_height,
    HTMLEmbedElement_get_height
};

#define HTMLEMBED_NODE_THIS(iface) DEFINE_THIS2(HTMLEmbedElement, element.node, iface)

static HRESULT HTMLEmbedElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLEmbedElement *This = HTMLEMBED_NODE_THIS(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLEMBED(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = HTMLEMBED(This);
    }else if(IsEqualGUID(&IID_IHTMLEmbedElement, riid)) {
        TRACE("(%p)->(IID_IHTMLEmbedElement %p)\n", This, ppv);
        *ppv = HTMLEMBED(This);
    }else {
        return HTMLElement_QI(&This->element.node, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static void HTMLEmbedElement_destructor(HTMLDOMNode *iface)
{
    HTMLEmbedElement *This = HTMLEMBED_NODE_THIS(iface);

    HTMLElement_destructor(&This->element.node);
}

#undef HTMLEMBED_NODE_THIS

static const NodeImplVtbl HTMLEmbedElementImplVtbl = {
    HTMLEmbedElement_QI,
    HTMLEmbedElement_destructor,
    HTMLElement_clone
};

static const tid_t HTMLEmbedElement_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLEmbedElement_tid,
    0
};
static dispex_static_data_t HTMLEmbedElement_dispex = {
    NULL,
    DispHTMLEmbed_tid,
    NULL,
    HTMLEmbedElement_iface_tids
};

HTMLElement *HTMLEmbedElement_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem)
{
    HTMLEmbedElement *ret = heap_alloc_zero(sizeof(*ret));

    ret->lpIHTMLEmbedElementVtbl = &HTMLEmbedElementVtbl;
    ret->element.node.vtbl = &HTMLEmbedElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLEmbedElement_dispex);
    return &ret->element;
}
