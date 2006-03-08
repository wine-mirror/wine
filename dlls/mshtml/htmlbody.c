/*
 * Copyright 2006 Jacek Caban for CodeWeavers
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "ole2.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct {
    const IHTMLBodyElementVtbl *lpHTMLBodyElementVtbl;

    HTMLElement *element;
    nsIDOMHTMLBodyElement *nsbody;
} HTMLBodyElement;

#define HTMLBODY(x)  ((IHTMLBodyElement*)  &(x)->lpHTMLBodyElementVtbl)

#define HTMLBODY_THIS(iface) DEFINE_THIS(HTMLBodyElement, HTMLBodyElement, iface)

static HRESULT WINAPI HTMLBodyElement_QueryInterface(IHTMLBodyElement *iface,
                                                     REFIID riid, void **ppv)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLBODY(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = HTMLBODY(This);
    }else if(IsEqualGUID(&IID_IHTMLBodyElement, riid)) {
        TRACE("(%p)->(IID_IHTMLBodyElement %p)\n", This, ppv);
        *ppv = HTMLBODY(This);
    }else if(IsEqualGUID(&IID_IHTMLElement, riid)) {
        TRACE("(%p)->(IID_IHTMLElement %p)\n", This, ppv);
        *ppv = HTMLELEM(This->element);
    }else if(IsEqualGUID(&IID_IHTMLDOMNode, riid)) {
        TRACE("(%p)->(IID_IHTMLDOMNode %p)\n", This, ppv);
        *ppv = HTMLDOMNODE(This->element->node);
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI HTMLBodyElement_AddRef(IHTMLBodyElement *iface)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);

    TRACE("(%p)\n", This);

    return IHTMLDocument2_AddRef(HTMLDOC(This->element->node->doc));
}

static ULONG WINAPI HTMLBodyElement_Release(IHTMLBodyElement *iface)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);

    TRACE("(%p)\n", This);

    return IHTMLDocument2_Release(HTMLDOC(This->element->node->doc));
}

static HRESULT WINAPI HTMLBodyElement_GetTypeInfoCount(IHTMLBodyElement *iface, UINT *pctinfo)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_GetTypeInfo(IHTMLBodyElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->(%u %lu %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_GetIDsOfNames(IHTMLBodyElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->(%s %p %u %lu %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
                                        lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_Invoke(IHTMLBodyElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->(%ld %s %ld %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_background(IHTMLBodyElement *iface, BSTR v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_background(IHTMLBodyElement *iface, BSTR *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_bgProperties(IHTMLBodyElement *iface, BSTR v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_bgProperties(IHTMLBodyElement *iface, BSTR *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_leftMargin(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_leftMargin(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_topMargin(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_topMargin(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_rightMargin(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_rightMargin(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_bottomMargin(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_bottomMargin(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_noWrap(IHTMLBodyElement *iface, VARIANT_BOOL v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_noWrap(IHTMLBodyElement *iface, VARIANT_BOOL *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_bgColor(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_bgColor(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_text(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_text(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_link(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_link(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_vLink(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_vLink(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_aLink(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_aLink(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_onload(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_onload(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_onunload(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_onunload(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_scroll(IHTMLBodyElement *iface, BSTR v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_scroll(IHTMLBodyElement *iface, BSTR *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_onselect(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_onselect(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_onbeforeunload(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_onbeforeunload(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_createTextRange(IHTMLBodyElement *iface, IHTMLTxtRange **range)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    TRACE("(%p)->(%p)\n", This, range);
    return E_NOTIMPL;
}

static void HTMLBodyElement_destructor(IUnknown *iface)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);

    nsIDOMHTMLBodyElement_Release(This->nsbody);
    HeapFree(GetProcessHeap(), 0, This);
}

static const IHTMLBodyElementVtbl HTMLBodyElementVtbl = {
    HTMLBodyElement_QueryInterface,
    HTMLBodyElement_AddRef,
    HTMLBodyElement_Release,
    HTMLBodyElement_GetTypeInfoCount,
    HTMLBodyElement_GetTypeInfo,
    HTMLBodyElement_GetIDsOfNames,
    HTMLBodyElement_Invoke,
    HTMLBodyElement_put_background,
    HTMLBodyElement_get_background,
    HTMLBodyElement_put_bgProperties,
    HTMLBodyElement_get_bgProperties,
    HTMLBodyElement_put_leftMargin,
    HTMLBodyElement_get_leftMargin,
    HTMLBodyElement_put_topMargin,
    HTMLBodyElement_get_topMargin,
    HTMLBodyElement_put_rightMargin,
    HTMLBodyElement_get_rightMargin,
    HTMLBodyElement_put_bottomMargin,
    HTMLBodyElement_get_bottomMargin,
    HTMLBodyElement_put_noWrap,
    HTMLBodyElement_get_noWrap,
    HTMLBodyElement_put_bgColor,
    HTMLBodyElement_get_bgColor,
    HTMLBodyElement_put_text,
    HTMLBodyElement_get_text,
    HTMLBodyElement_put_link,
    HTMLBodyElement_get_link,
    HTMLBodyElement_put_vLink,
    HTMLBodyElement_get_vLink,
    HTMLBodyElement_put_aLink,
    HTMLBodyElement_get_aLink,
    HTMLBodyElement_put_onload,
    HTMLBodyElement_get_onload,
    HTMLBodyElement_put_onunload,
    HTMLBodyElement_get_onunload,
    HTMLBodyElement_put_scroll,
    HTMLBodyElement_get_scroll,
    HTMLBodyElement_put_onselect,
    HTMLBodyElement_get_onselect,
    HTMLBodyElement_put_onbeforeunload,
    HTMLBodyElement_get_onbeforeunload,
    HTMLBodyElement_createTextRange
};

void HTMLBodyElement_Create(HTMLElement *element)
{
    HTMLBodyElement *ret = HeapAlloc(GetProcessHeap(), 0, sizeof(HTMLBodyElement));
    nsresult nsres;

    ret->lpHTMLBodyElementVtbl = &HTMLBodyElementVtbl;
    ret->element = element;

    nsres = nsIDOMHTMLElement_QueryInterface(element->nselem, &IID_nsIDOMHTMLBodyElement,
                                             (void**)&ret->nsbody);
    if(NS_FAILED(nsres))
        ERR("Could not get nsDOMHTMLBodyElement: %08lx\n", nsres);

    element->impl = (IUnknown*)HTMLBODY(ret);
    element->destructor = HTMLBodyElement_destructor;
}
