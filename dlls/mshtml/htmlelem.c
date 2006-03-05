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
#include "wine/unicode.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define HTMLELEM_THIS(iface) DEFINE_THIS(HTMLElement, HTMLElement, iface)

static HRESULT WINAPI HTMLElement_QueryInterface(IHTMLElement *iface,
                                                 REFIID riid, void **ppv)
{
    HTMLElement *This = HTMLELEM_THIS(iface);

    if(This->impl)
        return IUnknown_QueryInterface(This->impl, riid, ppv);

    *ppv =  NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLELEM(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = HTMLELEM(This);
    }else if(IsEqualGUID(&IID_IHTMLDOMNode, riid)) {
        TRACE("(%p)->(IID_IHTMLDOMNode %p)\n", This, ppv);
        *ppv = HTMLDOMNODE(This->node);
    }else if(IsEqualGUID(&IID_IHTMLElement, riid)) {
        TRACE("(%p)->(IID_IHTMLElement %p)\n", This, ppv);
        *ppv = HTMLELEM(This);
    }

    if(*ppv) {
        IHTMLElement_AddRef(HTMLELEM(This));
        return S_OK;
    }

    FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI HTMLElement_AddRef(IHTMLElement *iface)
{
    HTMLElement *This = HTMLELEM_THIS(iface);

    if(This->impl)
        return IUnknown_AddRef(This->impl);

    TRACE("(%p)\n", This);
    return IHTMLDocument2_AddRef(HTMLDOC(This->node->doc));
}

static ULONG WINAPI HTMLElement_Release(IHTMLElement *iface)
{
    HTMLElement *This = HTMLELEM_THIS(iface);

    if(This->impl)
        return IUnknown_Release(This->impl);

    TRACE("(%p)\n", This);
    return IHTMLDocument2_Release(HTMLDOC(This->node->doc));
}

static HRESULT WINAPI HTMLElement_GetTypeInfoCount(IHTMLElement *iface, UINT *pctinfo)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_GetTypeInfo(IHTMLElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%u %lu %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_GetIDsOfNames(IHTMLElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%s %p %u %lu %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
                                        lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_Invoke(IHTMLElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%ld %s %ld %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_setAttribute(IHTMLElement *iface, BSTR strAttributeName,
                                               VARIANT AttributeValue, LONG lFlags)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%s . %08lx)\n", This, debugstr_w(strAttributeName), lFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_getAttribute(IHTMLElement *iface, BSTR strAttributeName,
                                               LONG lFlags, VARIANT *AttributeValue)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%s %08lx %p)\n", This, debugstr_w(strAttributeName), lFlags,
          AttributeValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_removeAttribute(IHTMLElement *iface, BSTR strAttributeName,
                                                  LONG lFlags, VARIANT_BOOL *pfSuccess)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_className(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_className(IHTMLElement *iface, BSTR *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_id(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_id(IHTMLElement *iface, BSTR *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_tagName(IHTMLElement *iface, BSTR *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_parentElement(IHTMLElement *iface, IHTMLElement **p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_style(IHTMLElement *iface, IHTMLStyle **p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onhelp(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onhelp(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onclick(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onclick(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_ondblclick(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_ondblclick(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onkeydown(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onkeydown(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onkeyup(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onkeyup(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onkeypress(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onkeypress(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onmouseout(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onmouseout(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onmouseover(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onmouseover(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onmousemove(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onmousemove(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onmousedown(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onmousedown(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onmouseup(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onmouseup(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_document(IHTMLElement *iface, IDispatch **p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_title(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_title(IHTMLElement *iface, BSTR *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_language(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_language(IHTMLElement *iface, BSTR *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onselectstart(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onselectstart(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_scrollIntoView(IHTMLElement *iface, VARIANT varargStart)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_contains(IHTMLElement *iface, IHTMLElement *pChild,
                                           VARIANT_BOOL *pfResult)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pChild, pfResult);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_sourceIndex(IHTMLElement *iface, long *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_recordNumber(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_lang(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_lang(IHTMLElement *iface, BSTR *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_offsetLeft(IHTMLElement *iface, long *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_offsetTop(IHTMLElement *iface, long *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_offsetWidth(IHTMLElement *iface, long *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_offsetHeight(IHTMLElement *iface, long *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_offsetParent(IHTMLElement *iface, IHTMLElement **p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_innerHTML(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_innerHTML(IHTMLElement *iface, BSTR *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_innerText(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_innerText(IHTMLElement *iface, BSTR *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_outerHTML(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_outerHTML(IHTMLElement *iface, BSTR *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_outerText(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_outerText(IHTMLElement *iface, BSTR *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_insertAdjacentHTML(IHTMLElement *iface, BSTR where,
                                                     BSTR html)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_w(where), debugstr_w(html));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_insertAdjacentText(IHTMLElement *iface, BSTR where,
                                                     BSTR text)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_w(where), debugstr_w(text));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_parentTextEdit(IHTMLElement *iface, IHTMLElement **p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_isTextEdit(IHTMLElement *iface, VARIANT_BOOL *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_click(IHTMLElement *iface)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_filters(IHTMLElement *iface,
                                              IHTMLFiltersCollection **p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_ondragstart(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_ondragstart(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_toString(IHTMLElement *iface, BSTR *String)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, String);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onbeforeupdate(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onbeforeupdate(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onafterupdate(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onafterupdate(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onerrorupdate(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onerrorupdate(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onrowexit(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onrowexit(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onrowenter(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onrowenter(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_ondatasetchanged(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_ondatasetchanged(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_ondataavailable(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_ondataavailable(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_ondatasetcomplete(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_ondatasetcomplete(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onfilterchange(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onfilterchange(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_children(IHTMLElement *iface, IDispatch **p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_all(IHTMLElement *iface, IDispatch **p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static void HTMLElement_destructor(IUnknown *iface)
{
    HTMLElement *This = HTMLELEM_THIS(iface);

    if(This->destructor)
        This->destructor(This->impl);

    if(This->nselem)
        nsIDOMHTMLElement_Release(This->nselem);

    HeapFree(GetProcessHeap(), 0, This);
}

#undef HTMLELEM_THIS

static const IHTMLElementVtbl HTMLElementVtbl = {
    HTMLElement_QueryInterface,
    HTMLElement_AddRef,
    HTMLElement_Release,
    HTMLElement_GetTypeInfoCount,
    HTMLElement_GetTypeInfo,
    HTMLElement_GetIDsOfNames,
    HTMLElement_Invoke,
    HTMLElement_setAttribute,
    HTMLElement_getAttribute,
    HTMLElement_removeAttribute,
    HTMLElement_put_className,
    HTMLElement_get_className,
    HTMLElement_put_id,
    HTMLElement_get_id,
    HTMLElement_get_tagName,
    HTMLElement_get_parentElement,
    HTMLElement_get_style,
    HTMLElement_put_onhelp,
    HTMLElement_get_onhelp,
    HTMLElement_put_onclick,
    HTMLElement_get_onclick,
    HTMLElement_put_ondblclick,
    HTMLElement_get_ondblclick,
    HTMLElement_put_onkeydown,
    HTMLElement_get_onkeydown,
    HTMLElement_put_onkeyup,
    HTMLElement_get_onkeyup,
    HTMLElement_put_onkeypress,
    HTMLElement_get_onkeypress,
    HTMLElement_put_onmouseout,
    HTMLElement_get_onmouseout,
    HTMLElement_put_onmouseover,
    HTMLElement_get_onmouseover,
    HTMLElement_put_onmousemove,
    HTMLElement_get_onmousemove,
    HTMLElement_put_onmousedown,
    HTMLElement_get_onmousedown,
    HTMLElement_put_onmouseup,
    HTMLElement_get_onmouseup,
    HTMLElement_get_document,
    HTMLElement_put_title,
    HTMLElement_get_title,
    HTMLElement_put_language,
    HTMLElement_get_language,
    HTMLElement_put_onselectstart,
    HTMLElement_get_onselectstart,
    HTMLElement_scrollIntoView,
    HTMLElement_contains,
    HTMLElement_get_sourceIndex,
    HTMLElement_get_recordNumber,
    HTMLElement_put_lang,
    HTMLElement_get_lang,
    HTMLElement_get_offsetLeft,
    HTMLElement_get_offsetTop,
    HTMLElement_get_offsetWidth,
    HTMLElement_get_offsetHeight,
    HTMLElement_get_offsetParent,
    HTMLElement_put_innerHTML,
    HTMLElement_get_innerHTML,
    HTMLElement_put_innerText,
    HTMLElement_get_innerText,
    HTMLElement_put_outerHTML,
    HTMLElement_get_outerHTML,
    HTMLElement_put_outerText,
    HTMLElement_get_outerText,
    HTMLElement_insertAdjacentHTML,
    HTMLElement_insertAdjacentText,
    HTMLElement_get_parentTextEdit,
    HTMLElement_get_isTextEdit,
    HTMLElement_click,
    HTMLElement_get_filters,
    HTMLElement_put_ondragstart,
    HTMLElement_get_ondragstart,
    HTMLElement_toString,
    HTMLElement_put_onbeforeupdate,
    HTMLElement_get_onbeforeupdate,
    HTMLElement_put_onafterupdate,
    HTMLElement_get_onafterupdate,
    HTMLElement_put_onerrorupdate,
    HTMLElement_get_onerrorupdate,
    HTMLElement_put_onrowexit,
    HTMLElement_get_onrowexit,
    HTMLElement_put_onrowenter,
    HTMLElement_get_onrowenter,
    HTMLElement_put_ondatasetchanged,
    HTMLElement_get_ondatasetchanged,
    HTMLElement_put_ondataavailable,
    HTMLElement_get_ondataavailable,
    HTMLElement_put_ondatasetcomplete,
    HTMLElement_get_ondatasetcomplete,
    HTMLElement_put_onfilterchange,
    HTMLElement_get_onfilterchange,
    HTMLElement_get_children,
    HTMLElement_get_all
};

void HTMLElement_Create(HTMLDOMNode *node)
{
    HTMLElement *ret;

    ret = HeapAlloc(GetProcessHeap(), 0, sizeof(HTMLElement));
    ret->lpHTMLElementVtbl = &HTMLElementVtbl;
    ret->node = node;
    ret->impl = NULL;
    ret->destructor = NULL;

    node->node_type = NT_HTMLELEM;
    node->impl.elem = HTMLELEM(ret);
    node->destructor = HTMLElement_destructor;

    nsIDOMNode_QueryInterface(node->nsnode, &IID_nsIDOMHTMLElement, (void**)&ret->nselem);
}
