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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "winnls.h"
#include "ole2.h"
#include "shlwapi.h"

#include "wine/debug.h"
#include "wine/unicode.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

static HRESULT HTMLElementCollection_Create(IUnknown*,HTMLElement**,DWORD,IDispatch**);

typedef struct {
    HTMLElement **buf;
    DWORD len;
    DWORD size;
} elem_vector;

static void elem_vector_add(elem_vector *buf, HTMLElement *elem)
{
    if(buf->len == buf->size) {
        buf->size <<= 1;
        buf->buf = mshtml_realloc(buf->buf, buf->size*sizeof(HTMLElement**));
    }

    buf->buf[buf->len++] = elem;
}

#define HTMLELEM_THIS(iface) DEFINE_THIS(HTMLElement, HTMLElement, iface)

#define HTMLELEM_NODE_THIS(node)  ((HTMLElement *) node)

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
    return IHTMLDocument2_AddRef(HTMLDOC(This->node.doc));
}

static ULONG WINAPI HTMLElement_Release(IHTMLElement *iface)
{
    HTMLElement *This = HTMLELEM_THIS(iface);

    if(This->impl)
        return IUnknown_Release(This->impl);

    TRACE("(%p)\n", This);
    return IHTMLDocument2_Release(HTMLDOC(This->node.doc));
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
    FIXME("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_GetIDsOfNames(IHTMLElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
                                        lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_Invoke(IHTMLElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_setAttribute(IHTMLElement *iface, BSTR strAttributeName,
                                               VARIANT AttributeValue, LONG lFlags)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    nsAString attr_str;
    nsAString value_str;
    nsresult nsres;
    HRESULT hres;
    VARIANT AttributeValueChanged;

    WARN("(%p)->(%s . %08x)\n", This, debugstr_w(strAttributeName), lFlags);

    VariantInit(&AttributeValueChanged);

    hres = VariantChangeType(&AttributeValueChanged, &AttributeValue, 0, VT_BSTR);
    if (FAILED(hres)) {
        WARN("couldn't convert input attribute value %d to VT_BSTR\n", V_VT(&AttributeValue));
        return hres;
    }

    nsAString_Init(&attr_str, strAttributeName);
    nsAString_Init(&value_str, V_BSTR(&AttributeValueChanged));

    TRACE("setting %s to %s\n", debugstr_w(strAttributeName),
        debugstr_w(V_BSTR(&AttributeValueChanged)));

    nsres = nsIDOMHTMLElement_SetAttribute(This->nselem, &attr_str, &value_str);
    nsAString_Finish(&attr_str);
    nsAString_Finish(&value_str);

    if(NS_SUCCEEDED(nsres)) {
        hres = S_OK;
    }else {
        ERR("SetAttribute failed: %08x\n", nsres);
        hres = E_FAIL;
    }

    return hres;
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

    WARN("(%p)->(%s %08x %p)\n", This, debugstr_w(strAttributeName), lFlags, AttributeValue);

    VariantInit(AttributeValue);

    nsAString_Init(&attr_str, strAttributeName);
    nsAString_Init(&value_str, NULL);

    nsres = nsIDOMHTMLElement_GetAttribute(This->nselem, &attr_str, &value_str);
    nsAString_Finish(&attr_str);

    if(NS_SUCCEEDED(nsres)) {
        static const WCHAR wszSRC[] = {'s','r','c',0};
        nsAString_GetData(&value_str, &value, NULL);
        if(!strcmpiW(strAttributeName, wszSRC))
        {
            WCHAR buffer[256];
            DWORD len;
            BSTR bstrBaseUrl;
            hres = IHTMLDocument2_get_URL(HTMLDOC(This->node.doc), &bstrBaseUrl);
            if(SUCCEEDED(hres)) {
                hres = CoInternetCombineUrl(bstrBaseUrl, value,
                                            URL_ESCAPE_SPACES_ONLY|URL_DONT_ESCAPE_EXTRA_INFO,
                                            buffer, sizeof(buffer)/sizeof(WCHAR), &len, 0);
                SysFreeString(bstrBaseUrl);
                if(SUCCEEDED(hres)) {
                    V_VT(AttributeValue) = VT_BSTR;
                    V_BSTR(AttributeValue) = SysAllocString(buffer);
                    TRACE("attr_value=%s\n", debugstr_w(V_BSTR(AttributeValue)));
                }
            }
        }else {
            V_VT(AttributeValue) = VT_BSTR;
            V_BSTR(AttributeValue) = SysAllocString(value);
            TRACE("attr_value=%s\n", debugstr_w(V_BSTR(AttributeValue)));
        }
    }else {
        ERR("GetAttribute failed: %08x\n", nsres);
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
    nsAString class_str;
    nsresult nsres;
    HRESULT hres = S_OK;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&class_str, NULL);
    nsres = nsIDOMHTMLElement_GetClassName(This->nselem, &class_str);

    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *class;
        nsAString_GetData(&class_str, &class, NULL);
        *p = SysAllocString(class);
    }else {
        ERR("GetClassName failed: %08x\n", nsres);
        hres = E_FAIL;
    }

    nsAString_Finish(&class_str);

    TRACE("className=%s\n", debugstr_w(*p));
    return hres;
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
    nsIDOMElementCSSInlineStyle *nselemstyle;
    nsIDOMCSSStyleDeclaration *nsstyle;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLElement_QueryInterface(This->nselem, &IID_nsIDOMElementCSSInlineStyle,
                                             (void**)&nselemstyle);
    if(NS_FAILED(nsres)) {
        ERR("Coud not get nsIDOMCSSStyleDeclaration interface: %08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMElementCSSInlineStyle_GetStyle(nselemstyle, &nsstyle);
    nsIDOMElementCSSInlineStyle_Release(nselemstyle);
    if(NS_FAILED(nsres)) {
        ERR("GetStyle failed: %08x\n", nsres);
        return E_FAIL;
    }

    /* FIXME: Store style instead of creating a new instance in each call */
    *p = HTMLStyle_Create(nsstyle);

    nsIDOMCSSStyleDeclaration_Release(nsstyle);
    return S_OK;
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
    nsIDOMNSHTMLElement *nselem;
    nsAString html_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsres = nsIDOMHTMLElement_QueryInterface(This->nselem, &IID_nsIDOMNSHTMLElement, (void**)&nselem);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMNSHTMLElement: %08x\n", nsres);
        return E_FAIL;
    }

    nsAString_Init(&html_str, v);
    nsres = nsIDOMNSHTMLElement_SetInnerHTML(nselem, &html_str);
    nsAString_Finish(&html_str);

    if(NS_FAILED(nsres)) {
        FIXME("SetInnerHtml failed %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
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

static HRESULT HTMLElement_InsertAdjacentNode(HTMLElement *This, BSTR where, nsIDOMNode *nsnode)
{
    static const WCHAR wszBeforeBegin[] = {'b','e','f','o','r','e','B','e','g','i','n',0};
    static const WCHAR wszAfterBegin[] = {'a','f','t','e','r','B','e','g','i','n',0};
    static const WCHAR wszBeforeEnd[] = {'b','e','f','o','r','e','E','n','d',0};
    static const WCHAR wszAfterEnd[] = {'a','f','t','e','r','E','n','d',0};
    nsresult nsres;

    if (!strcmpiW(where, wszBeforeBegin))
    {
        nsIDOMNode *unused;
        nsIDOMNode *parent;
        nsres = nsIDOMNode_GetParentNode(This->nselem, &parent);
        if (!parent) return E_INVALIDARG;
        nsres = nsIDOMNode_InsertBefore(parent, nsnode,
                                        (nsIDOMNode *)This->nselem, &unused);
        if (unused) nsIDOMNode_Release(unused);
        nsIDOMNode_Release(parent);
    }
    else if (!strcmpiW(where, wszAfterBegin))
    {
        nsIDOMNode *unused;
        nsIDOMNode *first_child;
        nsIDOMNode_GetFirstChild(This->nselem, &first_child);
        nsres = nsIDOMNode_InsertBefore(This->nselem, nsnode, first_child, &unused);
        if (unused) nsIDOMNode_Release(unused);
        if (first_child) nsIDOMNode_Release(first_child);
    }
    else if (!strcmpiW(where, wszBeforeEnd))
    {
        nsIDOMNode *unused;
        nsres = nsIDOMNode_AppendChild(This->nselem, nsnode, &unused);
        if (unused) nsIDOMNode_Release(unused);
    }
    else if (!strcmpiW(where, wszAfterEnd))
    {
        nsIDOMNode *unused;
        nsIDOMNode *next_sibling;
        nsIDOMNode *parent;
        nsIDOMNode_GetParentNode(This->nselem, &parent);
        if (!parent) return E_INVALIDARG;

        nsIDOMNode_GetNextSibling(This->nselem, &next_sibling);
        if (next_sibling)
        {
            nsres = nsIDOMNode_InsertBefore(parent, nsnode, next_sibling, &unused);
            nsIDOMNode_Release(next_sibling);
        }
        else
            nsres = nsIDOMNode_AppendChild(parent, nsnode, &unused);
        nsIDOMNode_Release(parent);
        if (unused) nsIDOMNode_Release(unused);
    }
    else
    {
        ERR("invalid where: %s\n", debugstr_w(where));
        return E_INVALIDARG;
    }

    if (NS_FAILED(nsres))
        return E_FAIL;
    else
        return S_OK;
}

static HRESULT WINAPI HTMLElement_insertAdjacentHTML(IHTMLElement *iface, BSTR where,
                                                     BSTR html)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    nsresult nsres;
    nsIDOMDocument *nsdoc;
    nsIDOMDocumentRange *nsdocrange;
    nsIDOMRange *range;
    nsIDOMNSRange *nsrange;
    nsIDOMNode *nsnode;
    nsAString ns_html;
    HRESULT hr;

    TRACE("(%p)->(%s %s)\n", This, debugstr_w(where), debugstr_w(html));

    nsres = nsIWebNavigation_GetDocument(This->node.doc->nscontainer->navigation, &nsdoc);
    if(NS_FAILED(nsres))
    {
        ERR("GetDocument failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMDocument_QueryInterface(nsdoc, &IID_nsIDOMDocumentRange, (void **)&nsdocrange);
    nsIDOMDocument_Release(nsdoc);
    if(NS_FAILED(nsres))
    {
        ERR("getting nsIDOMDocumentRange failed: %08x\n", nsres);
        return E_FAIL;
    }
    nsres = nsIDOMDocumentRange_CreateRange(nsdocrange, &range);
    nsIDOMDocumentRange_Release(nsdocrange);
    if(NS_FAILED(nsres))
    {
        ERR("CreateRange failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsIDOMRange_SetStartBefore(range, (nsIDOMNode *)This->nselem);

    nsIDOMRange_QueryInterface(range, &IID_nsIDOMNSRange, (void **)&nsrange);
    nsIDOMRange_Release(range);
    if(NS_FAILED(nsres))
    {
        ERR("getting nsIDOMNSRange failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsAString_Init(&ns_html, html);

    nsres = nsIDOMNSRange_CreateContextualFragment(nsrange, &ns_html, (nsIDOMDocumentFragment **)&nsnode);
    nsIDOMNSRange_Release(nsrange);
    nsAString_Finish(&ns_html);

    if(NS_FAILED(nsres) || !nsnode)
    {
        ERR("CreateTextNode failed: %08x\n", nsres);
        return E_FAIL;
    }

    hr = HTMLElement_InsertAdjacentNode(This, where, nsnode);
    nsIDOMNode_Release(nsnode);

    return hr;
}

static HRESULT WINAPI HTMLElement_insertAdjacentText(IHTMLElement *iface, BSTR where,
                                                     BSTR text)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    nsresult nsres;
    nsIDOMDocument *nsdoc;
    nsIDOMNode *nsnode;
    nsAString ns_text;
    HRESULT hr;

    TRACE("(%p)->(%s %s)\n", This, debugstr_w(where), debugstr_w(text));

    nsres = nsIWebNavigation_GetDocument(This->node.doc->nscontainer->navigation, &nsdoc);
    if(NS_FAILED(nsres) || !nsdoc)
    {
        ERR("GetDocument failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsAString_Init(&ns_text, text);

    nsres = nsIDOMDocument_CreateTextNode(nsdoc, &ns_text, (nsIDOMText **)&nsnode);
    nsIDOMDocument_Release(nsdoc);
    nsAString_Finish(&ns_text);

    if(NS_FAILED(nsres) || !nsnode)
    {
        ERR("CreateTextNode failed: %08x\n", nsres);
        return E_FAIL;
    }

    hr = HTMLElement_InsertAdjacentNode(This, where, nsnode);
    nsIDOMNode_Release(nsnode);

    return hr;
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

static void create_child_list(HTMLDocument *doc, HTMLElement *elem, elem_vector *buf)
{
    nsIDOMNodeList *nsnode_list;
    nsIDOMNode *iter;
    PRUint32 list_len = 0, i;
    HTMLDOMNode *node;
    nsresult nsres;

    nsres = nsIDOMNode_GetChildNodes(elem->node.nsnode, &nsnode_list);
    if(NS_FAILED(nsres)) {
        ERR("GetChildNodes failed: %08x\n", nsres);
        return;
    }

    nsIDOMNodeList_GetLength(nsnode_list, &list_len);
    if(!list_len)
        return;

    buf->size = list_len;
    buf->buf = mshtml_alloc(buf->size*sizeof(HTMLElement**));

    for(i=0; i<list_len; i++) {
        nsres = nsIDOMNodeList_Item(nsnode_list, i, &iter);
        if(NS_FAILED(nsres)) {
            ERR("Item failed: %08x\n", nsres);
            continue;
        }

        node = get_node(doc, iter);
        if(node->node_type != NT_HTMLELEM)
            continue;

        elem_vector_add(buf, HTMLELEM_NODE_THIS(node));
    }
}

static HRESULT WINAPI HTMLElement_get_children(IHTMLElement *iface, IDispatch **p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    elem_vector buf = {NULL, 0, 0};

    TRACE("(%p)->(%p)\n", This, p);

    create_child_list(This->node.doc, This, &buf);

    return HTMLElementCollection_Create((IUnknown*)HTMLELEM(This), buf.buf, buf.len, p);
}

static void create_all_list(HTMLDocument *doc, HTMLElement *elem, elem_vector *buf)
{
    nsIDOMNodeList *nsnode_list;
    nsIDOMNode *iter;
    PRUint32 list_len = 0, i;
    HTMLDOMNode *node;
    nsresult nsres;

    nsres = nsIDOMNode_GetChildNodes(elem->node.nsnode, &nsnode_list);
    if(NS_FAILED(nsres)) {
        ERR("GetChildNodes failed: %08x\n", nsres);
        return;
    }

    nsIDOMNodeList_GetLength(nsnode_list, &list_len);
    if(!list_len)
        return;

    for(i=0; i<list_len; i++) {
        nsres = nsIDOMNodeList_Item(nsnode_list, i, &iter);
        if(NS_FAILED(nsres)) {
            ERR("Item failed: %08x\n", nsres);
            continue;
        }

        node = get_node(doc, iter);
        if(node->node_type != NT_HTMLELEM)
            continue;

        elem_vector_add(buf, HTMLELEM_NODE_THIS(node));
        create_all_list(doc, HTMLELEM_NODE_THIS(node), buf);
    }
}

static HRESULT WINAPI HTMLElement_get_all(IHTMLElement *iface, IDispatch **p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    elem_vector buf = {NULL, 0, 8};

    TRACE("(%p)->(%p)\n", This, p);

    buf.buf = mshtml_alloc(buf.size*sizeof(HTMLElement**));

    create_all_list(This->node.doc, This, &buf);

    TRACE("ret\n");

    if(!buf.len) {
        mshtml_free(buf.buf);
        buf.buf = NULL;
    }else if(buf.size > buf.len) {
        buf.buf = mshtml_realloc(buf.buf, buf.len*sizeof(HTMLElement**));
    }

    return HTMLElementCollection_Create((IUnknown*)HTMLELEM(This), buf.buf, buf.len, p);
}

static void HTMLElement_destructor(IUnknown *iface)
{
    HTMLElement *This = HTMLELEM_THIS(iface);

    if(This->destructor)
        This->destructor(This->impl);

    if(This->nselem)
        nsIDOMHTMLElement_Release(This->nselem);

    mshtml_free(This);
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
    }else if(IsEqualGUID(&IID_IHTMLElement2, riid)) {
        TRACE("(%p)->(IID_IHTMLElement2 %p)\n", This, ppv);
        *ppv = HTMLELEM2(This);
    }

    if(*ppv) {
        IHTMLElement_AddRef(HTMLELEM(This));
        return S_OK;
    }

    return HTMLDOMNode_QI(&This->node, riid, ppv);
}

HTMLElement *HTMLElement_Create(nsIDOMNode *nsnode)
{
    nsIDOMHTMLElement *nselem;
    HTMLElement *ret = NULL;
    nsAString class_name_str;
    const PRUnichar *class_name;
    nsresult nsres;

    static const WCHAR wszA[]        = {'A',0};
    static const WCHAR wszBODY[]     = {'B','O','D','Y',0};
    static const WCHAR wszINPUT[]    = {'I','N','P','U','T',0};
    static const WCHAR wszSELECT[]   = {'S','E','L','E','C','T',0};
    static const WCHAR wszTEXTAREA[] = {'T','E','X','T','A','R','E','A',0};

    nsres = nsIDOMNode_QueryInterface(nsnode, &IID_nsIDOMHTMLElement, (void**)&nselem);
    if(NS_FAILED(nsres))
        return NULL;

    nsAString_Init(&class_name_str, NULL);
    nsIDOMHTMLElement_GetTagName(nselem, &class_name_str);

    nsAString_GetData(&class_name_str, &class_name, NULL);

    if(!strcmpW(class_name, wszA))
        ret = HTMLAnchorElement_Create(nselem);
    else if(!strcmpW(class_name, wszBODY))
        ret = HTMLBodyElement_Create(nselem);
    else if(!strcmpW(class_name, wszINPUT))
        ret = HTMLInputElement_Create(nselem);
    if(!strcmpW(class_name, wszSELECT))
        ret = HTMLSelectElement_Create(nselem);
    else if(!strcmpW(class_name, wszTEXTAREA))
        ret = HTMLTextAreaElement_Create(nselem);

    if(!ret) {
        ret = mshtml_alloc(sizeof(HTMLElement));

        ret->impl = NULL;
        ret->destructor = NULL;
    }

    nsAString_Finish(&class_name_str);

    ret->lpHTMLElementVtbl = &HTMLElementVtbl;
    ret->nselem = nselem;

    HTMLElement2_Init(ret);

    ret->node.node_type = NT_HTMLELEM;
    ret->node.impl.elem = HTMLELEM(ret);
    ret->node.destructor = HTMLElement_destructor;

    return ret;
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

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLElementCollection_Release(IHTMLElementCollection *iface)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        IUnknown_Release(This->ref_unk);
        mshtml_free(This->elems);
        mshtml_free(This);
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
    FIXME("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElementCollection_GetIDsOfNames(IHTMLElementCollection *iface,
        REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    FIXME("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
                                        lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElementCollection_Invoke(IHTMLElementCollection *iface,
        DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    FIXME("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
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

    TRACE("(%p)->(v(%d) v(%d) %p)\n", This, V_VT(&name), V_VT(&index), pdisp);

    if(V_VT(&name) == VT_I4) {
        TRACE("name is VT_I4: %d\n", V_I4(&name));
        if(V_I4(&name) < 0 || V_I4(&name) >= This->len) {
            ERR("Invalid name! name=%d\n", V_I4(&name));
            return E_INVALIDARG;
        }

        *pdisp = (IDispatch*)This->elems[V_I4(&name)];
        IDispatch_AddRef(*pdisp);
        TRACE("Returning pdisp=%p\n", pdisp);
        return S_OK;
    }

    if(V_VT(&name) == VT_BSTR) {
        DWORD i;
        nsAString tag_str;
        const PRUnichar *tag;
        elem_vector buf = {NULL, 0, 8};

        TRACE("name is VT_BSTR: %s\n", debugstr_w(V_BSTR(&name)));

        nsAString_Init(&tag_str, NULL);
        buf.buf = mshtml_alloc(buf.size*sizeof(HTMLElement*));

        for(i=0; i<This->len; i++) {
            if(!This->elems[i]->nselem) continue;

            nsIDOMHTMLElement_GetId(This->elems[i]->nselem, &tag_str);
            nsAString_GetData(&tag_str, &tag, NULL);

            if(CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, tag, -1,
                                V_BSTR(&name), -1) == CSTR_EQUAL) {
                TRACE("Found name. elem=%d\n", i);
                if (V_VT(&index) == VT_I4)
                    if (buf.len == V_I4(&index)) {
                        nsAString_Finish(&tag_str);
                        mshtml_free(buf.buf);
                        buf.buf = NULL;
                        *pdisp = (IDispatch*)This->elems[i];
                        TRACE("Returning element %d pdisp=%p\n", i, pdisp);
                        IDispatch_AddRef(*pdisp);
                        return S_OK;
                    }
                elem_vector_add(&buf, This->elems[i]);
            }
        }
        nsAString_Finish(&tag_str);
        if (V_VT(&index) == VT_I4) {
            mshtml_free(buf.buf);
            buf.buf = NULL;
            ERR("Invalid index. index=%d >= buf.len=%d\n",V_I4(&index), buf.len);
            return E_INVALIDARG;
        }
        if(!buf.len) {
            mshtml_free(buf.buf);
            buf.buf = NULL;
        } else if(buf.size > buf.len) {
            buf.buf = mshtml_realloc(buf.buf, buf.len*sizeof(HTMLElement*));
        }
        TRACE("Returning %d element(s).\n", buf.len);
        return HTMLElementCollection_Create(This->ref_unk, buf.buf, buf.len, pdisp);
    }

    FIXME("unsupported arguments\n");
    return E_INVALIDARG;
}

static HRESULT WINAPI HTMLElementCollection_tags(IHTMLElementCollection *iface,
                                                 VARIANT tagName, IDispatch **pdisp)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    DWORD i;
    nsAString tag_str;
    const PRUnichar *tag;
    elem_vector buf = {NULL, 0, 8};

    if(V_VT(&tagName) != VT_BSTR) {
        WARN("Invalid arg\n");
        return DISP_E_MEMBERNOTFOUND;
    }

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(V_BSTR(&tagName)), pdisp);

    buf.buf = mshtml_alloc(buf.size*sizeof(HTMLElement*));

    nsAString_Init(&tag_str, NULL);

    for(i=0; i<This->len; i++) {
        if(!This->elems[i]->nselem)
            continue;

        nsIDOMElement_GetTagName(This->elems[i]->nselem, &tag_str);
        nsAString_GetData(&tag_str, &tag, NULL);

        if(CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, tag, -1,
                          V_BSTR(&tagName), -1) == CSTR_EQUAL)
            elem_vector_add(&buf, This->elems[i]);
    }

    nsAString_Finish(&tag_str);

    TRACE("fount %d tags\n", buf.len);

    if(!buf.len) {
        mshtml_free(buf.buf);
        buf.buf = NULL;
    }else if(buf.size > buf.len) {
        buf.buf = mshtml_realloc(buf.buf, buf.len*sizeof(HTMLElement*));
    }

    return HTMLElementCollection_Create(This->ref_unk, buf.buf, buf.len, pdisp);
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
    HTMLElementCollection *ret = mshtml_alloc(sizeof(HTMLElementCollection));

    ret->lpHTMLElementCollectionVtbl = &HTMLElementCollectionVtbl;
    ret->ref = 1;
    ret->elems = elems;
    ret->len = len;

    IUnknown_AddRef(ref_unk);
    ret->ref_unk = ref_unk;

    TRACE("ret=%p len=%d\n", ret, len);

    *p = (IDispatch*)ret;
    return S_OK;
}
