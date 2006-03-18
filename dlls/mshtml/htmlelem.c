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

static HRESULT HTMLElementCollection_Create(IUnknown*,HTMLElement**,DWORD,IDispatch**);

#define HTMLELEM_THIS(iface) DEFINE_THIS(HTMLElement, HTMLElement, iface)

static HRESULT WINAPI HTMLElement_QueryInterface(IHTMLElement *iface,
                                                 REFIID riid, void **ppv)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    HRESULT hres;

    if(This->impl)
        return IUnknown_QueryInterface(This->impl, riid, ppv);

    hres = HTMLElement_QI(This, riid, ppv);
    if(FAILED(hres))
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    return hres;
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
    nsAString attr_str;
    nsAString value_str;
    const PRUnichar *value;
    nsresult nsres;
    HRESULT hres = S_OK;

    WARN("(%p)->(%s %08lx %p)\n", This, debugstr_w(strAttributeName), lFlags, AttributeValue);

    nsAString_Init(&attr_str, strAttributeName);
    nsAString_Init(&value_str, NULL);

    nsres = nsIDOMHTMLElement_GetAttribute(This->nselem, &attr_str, &value_str);
    nsAString_Finish(&attr_str);

    if(NS_SUCCEEDED(nsres)) {
        nsAString_GetData(&value_str, &value, NULL);
        V_VT(AttributeValue) = VT_BSTR;
        V_BSTR(AttributeValue) = SysAllocString(value);;
        TRACE("attr_value=%s\n", debugstr_w(V_BSTR(AttributeValue)));
    }else {
        ERR("GetAttribute failed: %08lx\n", nsres);
        hres = E_FAIL;
    }

    nsAString_Finish(&value_str);

    return hres;
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

static void create_all_list(HTMLDocument *doc, HTMLElement *elem, HTMLElement ***list, DWORD *size,
                        DWORD *len)
{
    nsIDOMNodeList *nsnode_list;
    nsIDOMNode *iter;
    PRUint32 list_len = 0, i;
    HTMLDOMNode *node;
    nsresult nsres;

    nsres = nsIDOMNode_GetChildNodes(elem->node->nsnode, &nsnode_list);
    if(NS_FAILED(nsres)) {
        ERR("GetChildNodes failed: %08lx\n", nsres);
        return;
    }

    nsIDOMNodeList_GetLength(nsnode_list, &list_len);
    if(!list_len)
        return;

    for(i=0; i<list_len; i++) {
        nsres = nsIDOMNodeList_Item(nsnode_list, i, &iter);
        if(NS_FAILED(nsres)) {
            ERR("Item failed: %08lx\n", nsres);
            continue;
        }

        node = get_node(doc, iter);
        if(node->node_type != NT_HTMLELEM)
            continue;

        if(*len == *size) {
            *size <<= 1;
            *list = HeapReAlloc(GetProcessHeap(), 0, *list, *size * sizeof(HTMLElement**));
        }

        (*list)[(*len)++] = (HTMLElement*)node->impl.elem;

        create_all_list(doc, (HTMLElement*)node->impl.elem, list, size, len);
    }
}

static HRESULT WINAPI HTMLElement_get_all(IHTMLElement *iface, IDispatch **p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    HTMLElement **elem_list;
    DWORD list_size = 8, len = 0;

    TRACE("(%p)->(%p)\n", This, p);

    elem_list = HeapAlloc(GetProcessHeap(), 0, list_size*sizeof(HTMLElement**));

    create_all_list(This->node->doc, This, &elem_list, &list_size, &len);

    if(!len) {
        HeapFree(GetProcessHeap(), 0, elem_list);
        elem_list = NULL;
    }else if(list_size > len) {
        elem_list = HeapReAlloc(GetProcessHeap(), 0, elem_list, len*sizeof(HTMLElement**));
    }

    return HTMLElementCollection_Create((IUnknown*)HTMLELEM(This), elem_list, len, p);
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

HRESULT HTMLElement_QI(HTMLElement *This, REFIID riid, void **ppv)
{
    *ppv =  NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLELEM(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = HTMLELEM(This);
    }else if(IsEqualGUID(&IID_IHTMLElement, riid)) {
        TRACE("(%p)->(IID_IHTMLElement %p)\n", This, ppv);
        *ppv = HTMLELEM(This);
    }

    if(*ppv) {
        IHTMLElement_AddRef(HTMLELEM(This));
        return S_OK;
    }

    return HTMLDOMNode_QI(This->node, riid, ppv);
}

void HTMLElement_Create(HTMLDOMNode *node)
{
    HTMLElement *ret;
    nsAString class_name_str;
    const PRUnichar *class_name;
    nsresult nsres;

    static const WCHAR wszBODY[]     = {'B','O','D','Y',0};
    static const WCHAR wszINPUT[]    = {'I','N','P','U','T',0};
    static const WCHAR wszSELECT[]   = {'S','E','L','E','C','T',0};
    static const WCHAR wszTEXTAREA[] = {'T','E','X','T','A','R','E','A',0};

    ret = HeapAlloc(GetProcessHeap(), 0, sizeof(HTMLElement));
    ret->lpHTMLElementVtbl = &HTMLElementVtbl;
    ret->node = node;
    ret->impl = NULL;
    ret->destructor = NULL;

    node->node_type = NT_HTMLELEM;
    node->impl.elem = HTMLELEM(ret);
    node->destructor = HTMLElement_destructor;

    nsres = nsIDOMNode_QueryInterface(node->nsnode, &IID_nsIDOMHTMLElement, (void**)&ret->nselem);
    if(NS_FAILED(nsres))
        return;

    nsAString_Init(&class_name_str, NULL);
    nsIDOMHTMLElement_GetTagName(ret->nselem, &class_name_str);

    nsAString_GetData(&class_name_str, &class_name, NULL);

    if(!strcmpW(class_name, wszBODY))
        HTMLBodyElement_Create(ret);
    else if(!strcmpW(class_name, wszINPUT))
        HTMLInputElement_Create(ret);
    else if(!strcmpW(class_name, wszSELECT))
        HTMLSelectElement_Create(ret);
    else if(!strcmpW(class_name, wszTEXTAREA))
        HTMLTextAreaElement_Create(ret);

    nsAString_Finish(&class_name_str);
}

typedef struct {
    const IHTMLElementCollectionVtbl *lpHTMLElementCollectionVtbl;

    IUnknown *ref_unk;
    HTMLElement **elems;
    DWORD len;

    LONG ref;
} HTMLElementCollection;

#define HTMLELEMCOL(x)  ((IHTMLElementCollection*) &(x)->lpHTMLElementCollectionVtbl)

#define ELEMCOL_THIS(iface) DEFINE_THIS(HTMLElementCollection, HTMLElementCollection, iface)

static HRESULT WINAPI HTMLElementCollection_QueryInterface(IHTMLElementCollection *iface,
                                                           REFIID riid, void **ppv)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLELEMCOL(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = HTMLELEMCOL(This);
    }else if(IsEqualGUID(&IID_IHTMLElementCollection, riid)) {
        TRACE("(%p)->(IID_IHTMLElementCollection %p)\n", This, ppv);
        *ppv = HTMLELEMCOL(This);
    }

    if(*ppv) {
        IHTMLElementCollection_AddRef(HTMLELEMCOL(This));
        return S_OK;
    }

    FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI HTMLElementCollection_AddRef(IHTMLElementCollection *iface)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLElementCollection_Release(IHTMLElementCollection *iface)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        IUnknown_Release(This->ref_unk);
        HeapFree(GetProcessHeap(), 0, This->elems);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI HTMLElementCollection_GetTypeInfoCount(IHTMLElementCollection *iface,
                                                             UINT *pctinfo)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElementCollection_GetTypeInfo(IHTMLElementCollection *iface,
        UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    FIXME("(%p)->(%u %lu %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElementCollection_GetIDsOfNames(IHTMLElementCollection *iface,
        REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    FIXME("(%p)->(%s %p %u %lu %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
                                        lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElementCollection_Invoke(IHTMLElementCollection *iface,
        DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    FIXME("(%p)->(%ld %s %ld %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElementCollection_toString(IHTMLElementCollection *iface,
                                                     BSTR *String)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    FIXME("(%p)->(%p)\n", This, String);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElementCollection_put_length(IHTMLElementCollection *iface,
                                                       long v)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElementCollection_get_length(IHTMLElementCollection *iface,
                                                       long *p)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->len;
    return S_OK;
}

static HRESULT WINAPI HTMLElementCollection_get__newEnum(IHTMLElementCollection *iface,
                                                         IUnknown **p)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElementCollection_item(IHTMLElementCollection *iface,
        VARIANT name, VARIANT index, IDispatch **pdisp)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);

    if(V_VT(&index) != VT_I4) {
        WARN("Invalid index vt=%d\n", V_VT(&index));
        return E_INVALIDARG;
    }

    if(V_VT(&name) != VT_I4 || V_I4(&name) != V_I4(&index))
        FIXME("Unsupproted name vt=%d\n", V_VT(&name));

    TRACE("(%p)->(%ld %ld %p)\n", This, V_I4(&name), V_I4(&index), pdisp);

    if(V_I4(&index) < 0 || V_I4(&index) >= This->len)
        return E_INVALIDARG;

    *pdisp = (IDispatch*)This->elems[V_I4(&index)];
    IDispatch_AddRef(*pdisp);
    return S_OK;
}

static HRESULT WINAPI HTMLElementCollection_tags(IHTMLElementCollection *iface,
                                                 VARIANT tagName, IDispatch **pdisp)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    DWORD size = 8, len = 0, i;
    HTMLElement **elem_list;
    nsAString tag_str;
    const PRUnichar *tag;

    if(V_VT(&tagName) != VT_BSTR) {
        WARN("Invalid arg\n");
        return E_INVALIDARG;
    }

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(V_BSTR(&tagName)), pdisp);

    elem_list = HeapAlloc(GetProcessHeap(), 0, size*sizeof(HTMLElement*));

    nsAString_Init(&tag_str, NULL);

    for(i=0; i<This->len; i++) {
        if(!This->elems[i]->nselem)
            continue;

        nsIDOMElement_GetTagName(This->elems[i]->nselem, &tag_str);
        nsAString_GetData(&tag_str, &tag, NULL);

        if(CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, tag, -1,
                          V_BSTR(&tagName), -1) == CSTR_EQUAL) {
            if(len == size) {
                size <<= 2;
                elem_list = HeapReAlloc(GetProcessHeap(), 0, elem_list, size);
            }

            elem_list[len++] = This->elems[i];
        }
    }

    nsAString_Finish(&tag_str);

    TRACE("fount %ld tags\n", len);

    if(!len) {
        HeapFree(GetProcessHeap(), 0, elem_list);
        elem_list = NULL;
    }else if(size > len) {
        HeapReAlloc(GetProcessHeap(), 0, elem_list, len);
    }

    return HTMLElementCollection_Create(This->ref_unk, elem_list, len, pdisp);
}

#undef ELEMCOL_THIS

static const IHTMLElementCollectionVtbl HTMLElementCollectionVtbl = {
    HTMLElementCollection_QueryInterface,
    HTMLElementCollection_AddRef,
    HTMLElementCollection_Release,
    HTMLElementCollection_GetTypeInfoCount,
    HTMLElementCollection_GetTypeInfo,
    HTMLElementCollection_GetIDsOfNames,
    HTMLElementCollection_Invoke,
    HTMLElementCollection_toString,
    HTMLElementCollection_put_length,
    HTMLElementCollection_get_length,
    HTMLElementCollection_get__newEnum,
    HTMLElementCollection_item,
    HTMLElementCollection_tags
};

static HRESULT HTMLElementCollection_Create(IUnknown *ref_unk, HTMLElement **elems, DWORD len,
                                            IDispatch **p)
{
    HTMLElementCollection *ret = HeapAlloc(GetProcessHeap(), 0, sizeof(HTMLElementCollection));

    ret->lpHTMLElementCollectionVtbl = &HTMLElementCollectionVtbl;
    ret->ref = 1;
    ret->elems = elems;
    ret->len = len;

    IUnknown_AddRef(ref_unk);
    ret->ref_unk = ref_unk;

    TRACE("ret=%p len=%ld\n", ret, len);

    *p = (IDispatch*)ret;
    return S_OK;
}
