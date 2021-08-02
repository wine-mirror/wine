/*
 * Copyright 2008,2010 Jacek Caban for CodeWeavers
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

#include "mshtml_private.h"
#include "binding.h"
#include "htmlevent.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

static HRESULT set_frame_doc(HTMLFrameBase *frame, nsIDOMDocument *nsdoc)
{
    mozIDOMWindowProxy *mozwindow;
    HTMLOuterWindow *window;
    nsresult nsres;
    HRESULT hres = S_OK;

    if(frame->content_window)
        return S_OK;

    nsres = nsIDOMDocument_GetDefaultView(nsdoc, &mozwindow);
    if(NS_FAILED(nsres) || !mozwindow)
        return E_FAIL;

    window = mozwindow_to_window(mozwindow);
    if(!window && frame->element.node.doc->browser)
        hres = create_outer_window(frame->element.node.doc->browser, mozwindow,
                frame->element.node.doc->basedoc.window, &window);
    mozIDOMWindowProxy_Release(mozwindow);
    if(FAILED(hres))
        return hres;

    frame->content_window = window;
    window->frame_element = frame;
    return S_OK;
}

static inline HTMLFrameBase *impl_from_IHTMLFrameBase(IHTMLFrameBase *iface)
{
    return CONTAINING_RECORD(iface, HTMLFrameBase, IHTMLFrameBase_iface);
}

static HRESULT WINAPI HTMLFrameBase_QueryInterface(IHTMLFrameBase *iface, REFIID riid, void **ppv)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLFrameBase_AddRef(IHTMLFrameBase *iface)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLFrameBase_Release(IHTMLFrameBase *iface)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLFrameBase_GetTypeInfoCount(IHTMLFrameBase *iface, UINT *pctinfo)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);

    return IDispatchEx_GetTypeInfoCount(&This->element.node.event_target.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLFrameBase_GetTypeInfo(IHTMLFrameBase *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);

    return IDispatchEx_GetTypeInfo(&This->element.node.event_target.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLFrameBase_GetIDsOfNames(IHTMLFrameBase *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);

    return IDispatchEx_GetIDsOfNames(&This->element.node.event_target.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLFrameBase_Invoke(IHTMLFrameBase *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);

    return IDispatchEx_Invoke(&This->element.node.event_target.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLFrameBase_put_src(IHTMLFrameBase *iface, BSTR v)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->content_window || !This->element.node.doc || !This->element.node.doc->basedoc.window) {
        nsAString nsstr;
        nsresult nsres;

        nsAString_InitDepend(&nsstr, v);
        if(This->nsframe)
            nsres = nsIDOMHTMLFrameElement_SetSrc(This->nsframe, &nsstr);
        else
            nsres = nsIDOMHTMLIFrameElement_SetSrc(This->nsiframe, &nsstr);
        nsAString_Finish(&nsstr);
        if(NS_FAILED(nsres)) {
            ERR("SetSrc failed: %08x\n", nsres);
            return E_FAIL;
        }

        return S_OK;
    }

    return navigate_url(This->content_window, v, This->element.node.doc->basedoc.window->uri, BINDING_NAVIGATED);
}

static HRESULT WINAPI HTMLFrameBase_get_src(IHTMLFrameBase *iface, BSTR *p)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->nsframe && !This->nsiframe) {
        ERR("No attached frame object\n");
        return E_UNEXPECTED;
    }

    nsAString_Init(&nsstr, NULL);
    if(This->nsframe)
        nsres = nsIDOMHTMLFrameElement_GetSrc(This->nsframe, &nsstr);
    else
        nsres = nsIDOMHTMLIFrameElement_GetSrc(This->nsiframe, &nsstr);
    return return_nsstr(nsres, &nsstr, p);
}

static HRESULT WINAPI HTMLFrameBase_put_name(IHTMLFrameBase *iface, BSTR v)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    nsAString name_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->nsframe && !This->nsiframe) {
        ERR("No attached ns frame object\n");
        return E_UNEXPECTED;
    }

    nsAString_InitDepend(&name_str, v);
    if(This->nsframe)
        nsres = nsIDOMHTMLFrameElement_SetName(This->nsframe, &name_str);
    else
        nsres = nsIDOMHTMLIFrameElement_SetName(This->nsiframe, &name_str);
    nsAString_Finish(&name_str);
    if(NS_FAILED(nsres)) {
        ERR("SetName failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLFrameBase_get_name(IHTMLFrameBase *iface, BSTR *p)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->nsframe && !This->nsiframe) {
        ERR("No attached ns frame object\n");
        return E_UNEXPECTED;
    }

    nsAString_Init(&nsstr, NULL);
    if(This->nsframe)
        nsres = nsIDOMHTMLFrameElement_GetName(This->nsframe, &nsstr);
    else
        nsres = nsIDOMHTMLIFrameElement_GetName(This->nsiframe, &nsstr);
    return return_nsstr(nsres, &nsstr, p);
}

static HRESULT WINAPI HTMLFrameBase_put_border(IHTMLFrameBase *iface, VARIANT v)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase_get_border(IHTMLFrameBase *iface, VARIANT *p)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase_put_frameBorder(IHTMLFrameBase *iface, BSTR v)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->nsframe && !This->nsiframe) {
        ERR("No attached ns frame object\n");
        return E_UNEXPECTED;
    }

    nsAString_InitDepend(&nsstr, v);
    if(This->nsframe)
        nsres = nsIDOMHTMLFrameElement_SetFrameBorder(This->nsframe, &nsstr);
    else
        nsres = nsIDOMHTMLIFrameElement_SetFrameBorder(This->nsiframe, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres)) {
        ERR("SetFrameBorder failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLFrameBase_get_frameBorder(IHTMLFrameBase *iface, BSTR *p)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->nsframe && !This->nsiframe) {
        ERR("No attached ns frame object\n");
        return E_UNEXPECTED;
    }

    nsAString_Init(&nsstr, NULL);
    if(This->nsframe)
        nsres = nsIDOMHTMLFrameElement_GetFrameBorder(This->nsframe, &nsstr);
    else
        nsres = nsIDOMHTMLIFrameElement_GetFrameBorder(This->nsiframe, &nsstr);
    return return_nsstr(nsres, &nsstr, p);
}

static HRESULT WINAPI HTMLFrameBase_put_frameSpacing(IHTMLFrameBase *iface, VARIANT v)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase_get_frameSpacing(IHTMLFrameBase *iface, VARIANT *p)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase_put_marginWidth(IHTMLFrameBase *iface, VARIANT v)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    if(V_VT(&v) != VT_BSTR) {
        FIXME("unsupported %s\n", debugstr_variant(&v));
        return E_NOTIMPL;
    }

    nsAString_InitDepend(&nsstr, V_BSTR(&v));
    if(This->nsframe)
        nsres = nsIDOMHTMLFrameElement_SetMarginWidth(This->nsframe, &nsstr);
    else
        nsres = nsIDOMHTMLIFrameElement_SetMarginWidth(This->nsiframe, &nsstr);
    nsAString_Finish(&nsstr);
    return NS_SUCCEEDED(nsres) ? S_OK : E_FAIL;
}

static HRESULT WINAPI HTMLFrameBase_get_marginWidth(IHTMLFrameBase *iface, VARIANT *p)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    nsAString nsstr;
    nsresult nsres;
    HRESULT hres = S_OK;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nsstr, NULL);
    if(This->nsframe)
        nsres = nsIDOMHTMLFrameElement_GetMarginWidth(This->nsframe, &nsstr);
    else
        nsres = nsIDOMHTMLIFrameElement_GetMarginWidth(This->nsiframe, &nsstr);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *str, *end;

        nsAString_GetData(&nsstr, &str);

        if(*str) {
            BSTR ret;

            end = wcsstr(str, L"px");
            if(!end)
                end = str+lstrlenW(str);
            ret = SysAllocStringLen(str, end-str);
            if(ret) {
                V_VT(p) = VT_BSTR;
                V_BSTR(p) = ret;
            }else {
                hres = E_OUTOFMEMORY;
            }
        }else {
            V_VT(p) = VT_BSTR;
            V_BSTR(p) = NULL;
        }
    }else {
        ERR("GetMarginWidth failed: %08x\n", nsres);
        hres = E_FAIL;
    }

    nsAString_Finish(&nsstr);
    return hres;
}

static HRESULT WINAPI HTMLFrameBase_put_marginHeight(IHTMLFrameBase *iface, VARIANT v)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    if(V_VT(&v) != VT_BSTR) {
        FIXME("unsupported %s\n", debugstr_variant(&v));
        return E_NOTIMPL;
    }

    nsAString_InitDepend(&nsstr, V_BSTR(&v));
    if(This->nsframe)
        nsres = nsIDOMHTMLFrameElement_SetMarginHeight(This->nsframe, &nsstr);
    else
        nsres = nsIDOMHTMLIFrameElement_SetMarginHeight(This->nsiframe, &nsstr);
    nsAString_Finish(&nsstr);
    return NS_SUCCEEDED(nsres) ? S_OK : E_FAIL;
}

static HRESULT WINAPI HTMLFrameBase_get_marginHeight(IHTMLFrameBase *iface, VARIANT *p)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    nsAString nsstr;
    nsresult nsres;
    HRESULT hres = S_OK;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nsstr, NULL);
    if(This->nsframe)
        nsres = nsIDOMHTMLFrameElement_GetMarginHeight(This->nsframe, &nsstr);
    else
        nsres = nsIDOMHTMLIFrameElement_GetMarginHeight(This->nsiframe, &nsstr);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *str, *end;

        nsAString_GetData(&nsstr, &str);

        if(*str) {
            BSTR ret;

            end = wcsstr(str, L"px");
            if(!end)
                end = str+lstrlenW(str);
            ret = SysAllocStringLen(str, end-str);
            if(ret) {
                V_VT(p) = VT_BSTR;
                V_BSTR(p) = ret;
            }else {
                hres = E_OUTOFMEMORY;
            }
        }else {
            V_VT(p) = VT_BSTR;
            V_BSTR(p) = NULL;
        }
    }else {
        ERR("SetMarginHeight failed: %08x\n", nsres);
        hres = E_FAIL;
    }

    nsAString_Finish(&nsstr);
    return hres;
}

static HRESULT WINAPI HTMLFrameBase_put_noResize(IHTMLFrameBase *iface, VARIANT_BOOL v)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase_get_noResize(IHTMLFrameBase *iface, VARIANT_BOOL *p)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase_put_scrolling(IHTMLFrameBase *iface, BSTR v)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!(!wcsicmp(v, L"yes") || !wcsicmp(v, L"no") || !wcsicmp(v, L"auto")))
        return E_INVALIDARG;

    if(This->nsframe) {
        nsAString_InitDepend(&nsstr, v);
        nsres = nsIDOMHTMLFrameElement_SetScrolling(This->nsframe, &nsstr);
    }else if(This->nsiframe) {
        nsAString_InitDepend(&nsstr, v);
        nsres = nsIDOMHTMLIFrameElement_SetScrolling(This->nsiframe, &nsstr);
    }else {
        ERR("No attached ns frame object\n");
        return E_UNEXPECTED;
    }
    nsAString_Finish(&nsstr);

    if(NS_FAILED(nsres)) {
        ERR("SetScrolling failed: 0x%08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLFrameBase_get_scrolling(IHTMLFrameBase *iface, BSTR *p)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    nsAString nsstr;
    const PRUnichar *strdata;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->nsframe) {
        nsAString_Init(&nsstr, NULL);
        nsres = nsIDOMHTMLFrameElement_GetScrolling(This->nsframe, &nsstr);
    }else if(This->nsiframe) {
        nsAString_Init(&nsstr, NULL);
        nsres = nsIDOMHTMLIFrameElement_GetScrolling(This->nsiframe, &nsstr);
    }else {
        ERR("No attached ns frame object\n");
        return E_UNEXPECTED;
    }

    if(NS_FAILED(nsres)) {
        ERR("GetScrolling failed: 0x%08x\n", nsres);
        nsAString_Finish(&nsstr);
        return E_FAIL;
    }

    nsAString_GetData(&nsstr, &strdata);

    if(*strdata)
        *p = SysAllocString(strdata);
    else
        *p = SysAllocString(L"auto");

    nsAString_Finish(&nsstr);

    return *p ? S_OK : E_OUTOFMEMORY;
}

static const IHTMLFrameBaseVtbl HTMLFrameBaseVtbl = {
    HTMLFrameBase_QueryInterface,
    HTMLFrameBase_AddRef,
    HTMLFrameBase_Release,
    HTMLFrameBase_GetTypeInfoCount,
    HTMLFrameBase_GetTypeInfo,
    HTMLFrameBase_GetIDsOfNames,
    HTMLFrameBase_Invoke,
    HTMLFrameBase_put_src,
    HTMLFrameBase_get_src,
    HTMLFrameBase_put_name,
    HTMLFrameBase_get_name,
    HTMLFrameBase_put_border,
    HTMLFrameBase_get_border,
    HTMLFrameBase_put_frameBorder,
    HTMLFrameBase_get_frameBorder,
    HTMLFrameBase_put_frameSpacing,
    HTMLFrameBase_get_frameSpacing,
    HTMLFrameBase_put_marginWidth,
    HTMLFrameBase_get_marginWidth,
    HTMLFrameBase_put_marginHeight,
    HTMLFrameBase_get_marginHeight,
    HTMLFrameBase_put_noResize,
    HTMLFrameBase_get_noResize,
    HTMLFrameBase_put_scrolling,
    HTMLFrameBase_get_scrolling
};

static inline HTMLFrameBase *impl_from_IHTMLFrameBase2(IHTMLFrameBase2 *iface)
{
    return CONTAINING_RECORD(iface, HTMLFrameBase, IHTMLFrameBase2_iface);
}

static HRESULT WINAPI HTMLFrameBase2_QueryInterface(IHTMLFrameBase2 *iface, REFIID riid, void **ppv)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLFrameBase2_AddRef(IHTMLFrameBase2 *iface)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLFrameBase2_Release(IHTMLFrameBase2 *iface)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLFrameBase2_GetTypeInfoCount(IHTMLFrameBase2 *iface, UINT *pctinfo)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase2_GetTypeInfo(IHTMLFrameBase2 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase2_GetIDsOfNames(IHTMLFrameBase2 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase2_Invoke(IHTMLFrameBase2 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase2_get_contentWindow(IHTMLFrameBase2 *iface, IHTMLWindow2 **p)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->content_window) {
        IHTMLWindow2_AddRef(&This->content_window->base.IHTMLWindow2_iface);
        *p = &This->content_window->base.IHTMLWindow2_iface;
    }else {
        WARN("NULL content window\n");
        *p = NULL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLFrameBase2_put_onload(IHTMLFrameBase2 *iface, VARIANT v)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->element.node, EVENTID_LOAD, &v);
}

static HRESULT WINAPI HTMLFrameBase2_get_onload(IHTMLFrameBase2 *iface, VARIANT *p)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->element.node, EVENTID_LOAD, p);
}

static HRESULT WINAPI HTMLFrameBase2_put_onreadystatechange(IHTMLFrameBase2 *iface, VARIANT v)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase2_get_onreadystatechange(IHTMLFrameBase2 *iface, VARIANT *p)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase2_get_readyState(IHTMLFrameBase2 *iface, BSTR *p)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->content_window || !This->content_window->base.inner_window->doc) {
        FIXME("no document associated\n");
        return E_FAIL;
    }

    return IHTMLDocument2_get_readyState(&This->content_window->base.inner_window->doc->basedoc.IHTMLDocument2_iface, p);
}

static HRESULT WINAPI HTMLFrameBase2_put_allowTransparency(IHTMLFrameBase2 *iface, VARIANT_BOOL v)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);

    FIXME("(%p)->(%x) semi-stub\n", This, v);

    return S_OK;
}

static HRESULT WINAPI HTMLFrameBase2_get_allowTransparency(IHTMLFrameBase2 *iface, VARIANT_BOOL *p)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);

    FIXME("(%p)->(%p) semi-stub\n", This, p);

    *p = VARIANT_TRUE;
    return S_OK;
}

static const IHTMLFrameBase2Vtbl HTMLFrameBase2Vtbl = {
    HTMLFrameBase2_QueryInterface,
    HTMLFrameBase2_AddRef,
    HTMLFrameBase2_Release,
    HTMLFrameBase2_GetTypeInfoCount,
    HTMLFrameBase2_GetTypeInfo,
    HTMLFrameBase2_GetIDsOfNames,
    HTMLFrameBase2_Invoke,
    HTMLFrameBase2_get_contentWindow,
    HTMLFrameBase2_put_onload,
    HTMLFrameBase2_get_onload,
    HTMLFrameBase2_put_onreadystatechange,
    HTMLFrameBase2_get_onreadystatechange,
    HTMLFrameBase2_get_readyState,
    HTMLFrameBase2_put_allowTransparency,
    HTMLFrameBase2_get_allowTransparency
};

static HRESULT HTMLFrameBase_QI(HTMLFrameBase *This, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IHTMLFrameBase, riid)) {
        TRACE("(%p)->(IID_IHTMLFrameBase %p)\n", This, ppv);
        *ppv = &This->IHTMLFrameBase_iface;
    }else if(IsEqualGUID(&IID_IHTMLFrameBase2, riid)) {
        TRACE("(%p)->(IID_IHTMLFrameBase2 %p)\n", This, ppv);
        *ppv = &This->IHTMLFrameBase2_iface;
    }else {
        return HTMLElement_QI(&This->element.node, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static void HTMLFrameBase_destructor(HTMLFrameBase *This)
{
    if(This->content_window)
        This->content_window->frame_element = NULL;

    HTMLElement_destructor(&This->element.node);
}

static void HTMLFrameBase_Init(HTMLFrameBase *This, HTMLDocumentNode *doc, nsIDOMElement *nselem,
        dispex_static_data_t *dispex_data)
{
    nsresult nsres;

    This->IHTMLFrameBase_iface.lpVtbl = &HTMLFrameBaseVtbl;
    This->IHTMLFrameBase2_iface.lpVtbl = &HTMLFrameBase2Vtbl;

    HTMLElement_Init(&This->element, doc, nselem, dispex_data);

    nsres = nsIDOMElement_QueryInterface(nselem, &IID_nsIDOMHTMLFrameElement, (void**)&This->nsframe);
    if(NS_FAILED(nsres)) {
        This->nsframe = NULL;
        nsres = nsIDOMElement_QueryInterface(nselem, &IID_nsIDOMHTMLIFrameElement, (void**)&This->nsiframe);
        assert(nsres == NS_OK);
    }else {
        This->nsiframe = NULL;
    }
}

struct HTMLFrameElement {
    HTMLFrameBase framebase;
    IHTMLFrameElement3 IHTMLFrameElement3_iface;
};

static inline HTMLFrameElement *impl_from_IHTMLFrameElement3(IHTMLFrameElement3 *iface)
{
    return CONTAINING_RECORD(iface, HTMLFrameElement, IHTMLFrameElement3_iface);
}

static HRESULT WINAPI HTMLFrameElement3_QueryInterface(IHTMLFrameElement3 *iface,
        REFIID riid, void **ppv)
{
    HTMLFrameElement *This = impl_from_IHTMLFrameElement3(iface);

    return IHTMLDOMNode_QueryInterface(&This->framebase.element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLFrameElement3_AddRef(IHTMLFrameElement3 *iface)
{
    HTMLFrameElement *This = impl_from_IHTMLFrameElement3(iface);

    return IHTMLDOMNode_AddRef(&This->framebase.element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLFrameElement3_Release(IHTMLFrameElement3 *iface)
{
    HTMLFrameElement *This = impl_from_IHTMLFrameElement3(iface);

    return IHTMLDOMNode_Release(&This->framebase.element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLFrameElement3_GetTypeInfoCount(IHTMLFrameElement3 *iface, UINT *pctinfo)
{
    HTMLFrameElement *This = impl_from_IHTMLFrameElement3(iface);
    return IDispatchEx_GetTypeInfoCount(&This->framebase.element.node.event_target.dispex.IDispatchEx_iface,
            pctinfo);
}

static HRESULT WINAPI HTMLFrameElement3_GetTypeInfo(IHTMLFrameElement3 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLFrameElement *This = impl_from_IHTMLFrameElement3(iface);
    return IDispatchEx_GetTypeInfo(&This->framebase.element.node.event_target.dispex.IDispatchEx_iface, iTInfo,
            lcid, ppTInfo);
}

static HRESULT WINAPI HTMLFrameElement3_GetIDsOfNames(IHTMLFrameElement3 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLFrameElement *This = impl_from_IHTMLFrameElement3(iface);
    return IDispatchEx_GetIDsOfNames(&This->framebase.element.node.event_target.dispex.IDispatchEx_iface, riid,
            rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLFrameElement3_Invoke(IHTMLFrameElement3 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLFrameElement *This = impl_from_IHTMLFrameElement3(iface);
    return IDispatchEx_Invoke(&This->framebase.element.node.event_target.dispex.IDispatchEx_iface, dispIdMember,
            riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLFrameElement3_get_contentDocument(IHTMLFrameElement3 *iface, IDispatch **p)
{
    HTMLFrameElement *This = impl_from_IHTMLFrameElement3(iface);
    IHTMLDocument2 *doc;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->framebase.content_window) {
        FIXME("NULL window\n");
        return E_FAIL;
    }

    hres = IHTMLWindow2_get_document(&This->framebase.content_window->base.IHTMLWindow2_iface, &doc);
    if(FAILED(hres))
        return hres;

    *p = doc ? (IDispatch*)doc : NULL;
    return S_OK;
}

static HRESULT WINAPI HTMLFrameElement3_put_src(IHTMLFrameElement3 *iface, BSTR v)
{
    HTMLFrameElement *This = impl_from_IHTMLFrameElement3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameElement3_get_src(IHTMLFrameElement3 *iface, BSTR *p)
{
    HTMLFrameElement *This = impl_from_IHTMLFrameElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameElement3_put_longDesc(IHTMLFrameElement3 *iface, BSTR v)
{
    HTMLFrameElement *This = impl_from_IHTMLFrameElement3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameElement3_get_longDesc(IHTMLFrameElement3 *iface, BSTR *p)
{
    HTMLFrameElement *This = impl_from_IHTMLFrameElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameElement3_put_frameBorder(IHTMLFrameElement3 *iface, BSTR v)
{
    HTMLFrameElement *This = impl_from_IHTMLFrameElement3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameElement3_get_frameBorder(IHTMLFrameElement3 *iface, BSTR *p)
{
    HTMLFrameElement *This = impl_from_IHTMLFrameElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLFrameElement3Vtbl HTMLFrameElement3Vtbl = {
    HTMLFrameElement3_QueryInterface,
    HTMLFrameElement3_AddRef,
    HTMLFrameElement3_Release,
    HTMLFrameElement3_GetTypeInfoCount,
    HTMLFrameElement3_GetTypeInfo,
    HTMLFrameElement3_GetIDsOfNames,
    HTMLFrameElement3_Invoke,
    HTMLFrameElement3_get_contentDocument,
    HTMLFrameElement3_put_src,
    HTMLFrameElement3_get_src,
    HTMLFrameElement3_put_longDesc,
    HTMLFrameElement3_get_longDesc,
    HTMLFrameElement3_put_frameBorder,
    HTMLFrameElement3_get_frameBorder
};

static inline HTMLFrameElement *frame_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLFrameElement, framebase.element.node);
}

static HRESULT HTMLFrameElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLFrameElement *This = frame_from_HTMLDOMNode(iface);

    if(IsEqualGUID(&IID_IHTMLFrameElement3, riid)) {
        TRACE("(%p)->(IID_IHTMLFrameElement3 %p)\n", This, ppv);
        *ppv = &This->IHTMLFrameElement3_iface;
    }else {
        return HTMLFrameBase_QI(&This->framebase, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static void HTMLFrameElement_destructor(HTMLDOMNode *iface)
{
    HTMLFrameElement *This = frame_from_HTMLDOMNode(iface);

    HTMLFrameBase_destructor(&This->framebase);
}

static HRESULT HTMLFrameElement_get_document(HTMLDOMNode *iface, IDispatch **p)
{
    HTMLFrameElement *This = frame_from_HTMLDOMNode(iface);

    if(!This->framebase.content_window || !This->framebase.content_window->base.inner_window->doc) {
        *p = NULL;
        return S_OK;
    }

    *p = (IDispatch*)&This->framebase.content_window->base.inner_window->doc->basedoc.IHTMLDocument2_iface;
    IDispatch_AddRef(*p);
    return S_OK;
}

static HRESULT HTMLFrameElement_get_readystate(HTMLDOMNode *iface, BSTR *p)
{
    HTMLFrameElement *This = frame_from_HTMLDOMNode(iface);

    return IHTMLFrameBase2_get_readyState(&This->framebase.IHTMLFrameBase2_iface, p);
}

static HRESULT HTMLFrameElement_get_dispid(HTMLDOMNode *iface, BSTR name,
        DWORD grfdex, DISPID *pid)
{
    HTMLFrameElement *This = frame_from_HTMLDOMNode(iface);

    if(!This->framebase.content_window)
        return DISP_E_UNKNOWNNAME;

    return search_window_props(This->framebase.content_window->base.inner_window, name, grfdex, pid);
}

static HRESULT HTMLFrameElement_invoke(HTMLDOMNode *iface, DISPID id, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLFrameElement *This = frame_from_HTMLDOMNode(iface);

    if(!This->framebase.content_window) {
        ERR("no content window to invoke on\n");
        return E_FAIL;
    }

    return IDispatchEx_InvokeEx(&This->framebase.content_window->base.IDispatchEx_iface, id, lcid,
            flags, params, res, ei, caller);
}

static HRESULT HTMLFrameElement_bind_to_tree(HTMLDOMNode *iface)
{
    HTMLFrameElement *This = frame_from_HTMLDOMNode(iface);
    nsIDOMDocument *nsdoc;
    nsresult nsres;
    HRESULT hres;

    nsres = nsIDOMHTMLFrameElement_GetContentDocument(This->framebase.nsframe, &nsdoc);
    if(NS_FAILED(nsres) || !nsdoc) {
        ERR("GetContentDocument failed: %08x\n", nsres);
        return E_FAIL;
    }

    hres = set_frame_doc(&This->framebase, nsdoc);
    nsIDOMDocument_Release(nsdoc);
    return hres;
}

static void HTMLFrameElement_traverse(HTMLDOMNode *iface, nsCycleCollectionTraversalCallback *cb)
{
    HTMLFrameElement *This = frame_from_HTMLDOMNode(iface);

    if(This->framebase.nsframe)
        note_cc_edge((nsISupports*)This->framebase.nsframe, "This->nsframe", cb);
}

static void HTMLFrameElement_unlink(HTMLDOMNode *iface)
{
    HTMLFrameElement *This = frame_from_HTMLDOMNode(iface);

    if(This->framebase.nsframe) {
        nsIDOMHTMLFrameElement *nsframe = This->framebase.nsframe;

        This->framebase.nsframe = NULL;
        nsIDOMHTMLFrameElement_Release(nsframe);
    }
}

static const NodeImplVtbl HTMLFrameElementImplVtbl = {
    &CLSID_HTMLFrameElement,
    HTMLFrameElement_QI,
    HTMLFrameElement_destructor,
    HTMLElement_cpc,
    HTMLElement_clone,
    HTMLElement_handle_event,
    HTMLElement_get_attr_col,
    NULL,
    NULL,
    NULL,
    HTMLFrameElement_get_document,
    HTMLFrameElement_get_readystate,
    HTMLFrameElement_get_dispid,
    HTMLFrameElement_invoke,
    HTMLFrameElement_bind_to_tree,
    HTMLFrameElement_traverse,
    HTMLFrameElement_unlink
};

static const tid_t HTMLFrameElement_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLFrameBase_tid,
    IHTMLFrameBase2_tid,
    IHTMLFrameElement3_tid,
    0
};

static dispex_static_data_t HTMLFrameElement_dispex = {
    NULL,
    DispHTMLFrameElement_tid,
    HTMLFrameElement_iface_tids,
    HTMLElement_init_dispex_info
};

HRESULT HTMLFrameElement_Create(HTMLDocumentNode *doc, nsIDOMElement *nselem, HTMLElement **elem)
{
    HTMLFrameElement *ret;

    ret = heap_alloc_zero(sizeof(HTMLFrameElement));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->framebase.element.node.vtbl = &HTMLFrameElementImplVtbl;
    ret->IHTMLFrameElement3_iface.lpVtbl = &HTMLFrameElement3Vtbl;

    HTMLFrameBase_Init(&ret->framebase, doc, nselem, &HTMLFrameElement_dispex);

    *elem = &ret->framebase.element;
    return S_OK;
}

struct HTMLIFrame {
    HTMLFrameBase framebase;
    IHTMLIFrameElement IHTMLIFrameElement_iface;
    IHTMLIFrameElement2 IHTMLIFrameElement2_iface;
    IHTMLIFrameElement3 IHTMLIFrameElement3_iface;
};

static inline HTMLIFrame *impl_from_IHTMLIFrameElement(IHTMLIFrameElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLIFrame, IHTMLIFrameElement_iface);
}

static HRESULT WINAPI HTMLIFrameElement_QueryInterface(IHTMLIFrameElement *iface,
        REFIID riid, void **ppv)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->framebase.element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLIFrameElement_AddRef(IHTMLIFrameElement *iface)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement(iface);

    return IHTMLDOMNode_AddRef(&This->framebase.element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLIFrameElement_Release(IHTMLIFrameElement *iface)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement(iface);

    return IHTMLDOMNode_Release(&This->framebase.element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLIFrameElement_GetTypeInfoCount(IHTMLIFrameElement *iface, UINT *pctinfo)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement(iface);
    return IDispatchEx_GetTypeInfoCount(&This->framebase.element.node.event_target.dispex.IDispatchEx_iface,
            pctinfo);
}

static HRESULT WINAPI HTMLIFrameElement_GetTypeInfo(IHTMLIFrameElement *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement(iface);
    return IDispatchEx_GetTypeInfo(&This->framebase.element.node.event_target.dispex.IDispatchEx_iface, iTInfo,
            lcid, ppTInfo);
}

static HRESULT WINAPI HTMLIFrameElement_GetIDsOfNames(IHTMLIFrameElement *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement(iface);
    return IDispatchEx_GetIDsOfNames(&This->framebase.element.node.event_target.dispex.IDispatchEx_iface, riid,
            rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLIFrameElement_Invoke(IHTMLIFrameElement *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement(iface);
    return IDispatchEx_Invoke(&This->framebase.element.node.event_target.dispex.IDispatchEx_iface, dispIdMember,
            riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLIFrameElement_put_vspace(IHTMLIFrameElement *iface, LONG v)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameElement_get_vspace(IHTMLIFrameElement *iface, LONG *p)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameElement_put_hspace(IHTMLIFrameElement *iface, LONG v)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameElement_get_hspace(IHTMLIFrameElement *iface, LONG *p)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameElement_put_align(IHTMLIFrameElement *iface, BSTR v)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameElement_get_align(IHTMLIFrameElement *iface, BSTR *p)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLIFrameElementVtbl HTMLIFrameElementVtbl = {
    HTMLIFrameElement_QueryInterface,
    HTMLIFrameElement_AddRef,
    HTMLIFrameElement_Release,
    HTMLIFrameElement_GetTypeInfoCount,
    HTMLIFrameElement_GetTypeInfo,
    HTMLIFrameElement_GetIDsOfNames,
    HTMLIFrameElement_Invoke,
    HTMLIFrameElement_put_vspace,
    HTMLIFrameElement_get_vspace,
    HTMLIFrameElement_put_hspace,
    HTMLIFrameElement_get_hspace,
    HTMLIFrameElement_put_align,
    HTMLIFrameElement_get_align
};

static inline HTMLIFrame *impl_from_IHTMLIFrameElement2(IHTMLIFrameElement2 *iface)
{
    return CONTAINING_RECORD(iface, HTMLIFrame, IHTMLIFrameElement2_iface);
}

static HRESULT WINAPI HTMLIFrameElement2_QueryInterface(IHTMLIFrameElement2 *iface,
        REFIID riid, void **ppv)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement2(iface);

    return IHTMLDOMNode_QueryInterface(&This->framebase.element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLIFrameElement2_AddRef(IHTMLIFrameElement2 *iface)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement2(iface);

    return IHTMLDOMNode_AddRef(&This->framebase.element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLIFrameElement2_Release(IHTMLIFrameElement2 *iface)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement2(iface);

    return IHTMLDOMNode_Release(&This->framebase.element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLIFrameElement2_GetTypeInfoCount(IHTMLIFrameElement2 *iface, UINT *pctinfo)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement2(iface);
    return IDispatchEx_GetTypeInfoCount(&This->framebase.element.node.event_target.dispex.IDispatchEx_iface,
            pctinfo);
}

static HRESULT WINAPI HTMLIFrameElement2_GetTypeInfo(IHTMLIFrameElement2 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement2(iface);
    return IDispatchEx_GetTypeInfo(&This->framebase.element.node.event_target.dispex.IDispatchEx_iface, iTInfo,
            lcid, ppTInfo);
}

static HRESULT WINAPI HTMLIFrameElement2_GetIDsOfNames(IHTMLIFrameElement2 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement2(iface);
    return IDispatchEx_GetIDsOfNames(&This->framebase.element.node.event_target.dispex.IDispatchEx_iface, riid,
            rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLIFrameElement2_Invoke(IHTMLIFrameElement2 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement2(iface);
    return IDispatchEx_Invoke(&This->framebase.element.node.event_target.dispex.IDispatchEx_iface, dispIdMember,
            riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLIFrameElement2_put_height(IHTMLIFrameElement2 *iface, VARIANT v)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement2(iface);
    nsAString nsstr;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    hres = variant_to_nsstr(&v, FALSE, &nsstr);
    if(FAILED(hres))
        return hres;

    nsres = nsIDOMHTMLIFrameElement_SetHeight(This->framebase.nsiframe, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres)) {
        ERR("SetHeight failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLIFrameElement2_get_height(IHTMLIFrameElement2 *iface, VARIANT *p)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement2(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMHTMLIFrameElement_GetHeight(This->framebase.nsiframe, &nsstr);

    V_VT(p) = VT_BSTR;
    return return_nsstr(nsres, &nsstr, &V_BSTR(p));
}

static HRESULT WINAPI HTMLIFrameElement2_put_width(IHTMLIFrameElement2 *iface, VARIANT v)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement2(iface);
    nsAString nsstr;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    hres = variant_to_nsstr(&v, FALSE, &nsstr);
    if(FAILED(hres))
        return hres;

    nsres = nsIDOMHTMLIFrameElement_SetWidth(This->framebase.nsiframe, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres)) {
        ERR("SetWidth failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLIFrameElement2_get_width(IHTMLIFrameElement2 *iface, VARIANT *p)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement2(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMHTMLIFrameElement_GetWidth(This->framebase.nsiframe, &nsstr);

    V_VT(p) = VT_BSTR;
    return return_nsstr(nsres, &nsstr, &V_BSTR(p));
}

static const IHTMLIFrameElement2Vtbl HTMLIFrameElement2Vtbl = {
    HTMLIFrameElement2_QueryInterface,
    HTMLIFrameElement2_AddRef,
    HTMLIFrameElement2_Release,
    HTMLIFrameElement2_GetTypeInfoCount,
    HTMLIFrameElement2_GetTypeInfo,
    HTMLIFrameElement2_GetIDsOfNames,
    HTMLIFrameElement2_Invoke,
    HTMLIFrameElement2_put_height,
    HTMLIFrameElement2_get_height,
    HTMLIFrameElement2_put_width,
    HTMLIFrameElement2_get_width
};

static inline HTMLIFrame *impl_from_IHTMLIFrameElement3(IHTMLIFrameElement3 *iface)
{
    return CONTAINING_RECORD(iface, HTMLIFrame, IHTMLIFrameElement3_iface);
}

static HRESULT WINAPI HTMLIFrameElement3_QueryInterface(IHTMLIFrameElement3 *iface,
        REFIID riid, void **ppv)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement3(iface);

    return IHTMLDOMNode_QueryInterface(&This->framebase.element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLIFrameElement3_AddRef(IHTMLIFrameElement3 *iface)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement3(iface);

    return IHTMLDOMNode_AddRef(&This->framebase.element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLIFrameElement3_Release(IHTMLIFrameElement3 *iface)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement3(iface);

    return IHTMLDOMNode_Release(&This->framebase.element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLIFrameElement3_GetTypeInfoCount(IHTMLIFrameElement3 *iface, UINT *pctinfo)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement3(iface);
    return IDispatchEx_GetTypeInfoCount(&This->framebase.element.node.event_target.dispex.IDispatchEx_iface,
            pctinfo);
}

static HRESULT WINAPI HTMLIFrameElement3_GetTypeInfo(IHTMLIFrameElement3 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement3(iface);
    return IDispatchEx_GetTypeInfo(&This->framebase.element.node.event_target.dispex.IDispatchEx_iface, iTInfo,
            lcid, ppTInfo);
}

static HRESULT WINAPI HTMLIFrameElement3_GetIDsOfNames(IHTMLIFrameElement3 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement3(iface);
    return IDispatchEx_GetIDsOfNames(&This->framebase.element.node.event_target.dispex.IDispatchEx_iface, riid,
            rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLIFrameElement3_Invoke(IHTMLIFrameElement3 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement3(iface);
    return IDispatchEx_Invoke(&This->framebase.element.node.event_target.dispex.IDispatchEx_iface, dispIdMember,
            riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLIFrameElement3_get_contentDocument(IHTMLIFrameElement3 *iface, IDispatch **p)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement3(iface);
    IHTMLDocument2 *doc;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->framebase.content_window) {
        *p = NULL;
        return S_OK;
    }

    hres = IHTMLWindow2_get_document(&This->framebase.content_window->base.IHTMLWindow2_iface, &doc);
    *p = (IDispatch*)doc;
    return hres;
}

static HRESULT WINAPI HTMLIFrameElement3_put_src(IHTMLIFrameElement3 *iface, BSTR v)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameElement3_get_src(IHTMLIFrameElement3 *iface, BSTR *p)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameElement3_put_longDesc(IHTMLIFrameElement3 *iface, BSTR v)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameElement3_get_longDesc(IHTMLIFrameElement3 *iface, BSTR *p)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameElement3_put_frameBorder(IHTMLIFrameElement3 *iface, BSTR v)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameElement3_get_frameBorder(IHTMLIFrameElement3 *iface, BSTR *p)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLIFrameElement3Vtbl HTMLIFrameElement3Vtbl = {
    HTMLIFrameElement3_QueryInterface,
    HTMLIFrameElement3_AddRef,
    HTMLIFrameElement3_Release,
    HTMLIFrameElement3_GetTypeInfoCount,
    HTMLIFrameElement3_GetTypeInfo,
    HTMLIFrameElement3_GetIDsOfNames,
    HTMLIFrameElement3_Invoke,
    HTMLIFrameElement3_get_contentDocument,
    HTMLIFrameElement3_put_src,
    HTMLIFrameElement3_get_src,
    HTMLIFrameElement3_put_longDesc,
    HTMLIFrameElement3_get_longDesc,
    HTMLIFrameElement3_put_frameBorder,
    HTMLIFrameElement3_get_frameBorder
};

static inline HTMLIFrame *iframe_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLIFrame, framebase.element.node);
}

static HRESULT HTMLIFrame_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLIFrame *This = iframe_from_HTMLDOMNode(iface);

    if(IsEqualGUID(&IID_IHTMLIFrameElement, riid)) {
        TRACE("(%p)->(IID_IHTMLIFrameElement %p)\n", This, ppv);
        *ppv = &This->IHTMLIFrameElement_iface;
    }else if(IsEqualGUID(&IID_IHTMLIFrameElement2, riid)) {
        TRACE("(%p)->(IID_IHTMLIFrameElement2 %p)\n", This, ppv);
        *ppv = &This->IHTMLIFrameElement2_iface;
    }else if(IsEqualGUID(&IID_IHTMLIFrameElement3, riid)) {
        TRACE("(%p)->(IID_IHTMLIFrameElement3 %p)\n", This, ppv);
        *ppv = &This->IHTMLIFrameElement3_iface;
    }else {
        return HTMLFrameBase_QI(&This->framebase, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static void HTMLIFrame_destructor(HTMLDOMNode *iface)
{
    HTMLIFrame *This = iframe_from_HTMLDOMNode(iface);

    HTMLFrameBase_destructor(&This->framebase);
}

static HRESULT HTMLIFrame_get_document(HTMLDOMNode *iface, IDispatch **p)
{
    HTMLIFrame *This = iframe_from_HTMLDOMNode(iface);

    if(!This->framebase.content_window || !This->framebase.content_window->base.inner_window->doc) {
        *p = NULL;
        return S_OK;
    }

    *p = (IDispatch*)&This->framebase.content_window->base.inner_window->doc->basedoc.IHTMLDocument2_iface;
    IDispatch_AddRef(*p);
    return S_OK;
}

static HRESULT HTMLIFrame_get_dispid(HTMLDOMNode *iface, BSTR name,
        DWORD grfdex, DISPID *pid)
{
    HTMLIFrame *This = iframe_from_HTMLDOMNode(iface);

    if(!This->framebase.content_window)
        return DISP_E_UNKNOWNNAME;

    return search_window_props(This->framebase.content_window->base.inner_window, name, grfdex, pid);
}

static HRESULT HTMLIFrame_invoke(HTMLDOMNode *iface, DISPID id, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLIFrame *This = iframe_from_HTMLDOMNode(iface);

    if(!This->framebase.content_window) {
        ERR("no content window to invoke on\n");
        return E_FAIL;
    }

    return IDispatchEx_InvokeEx(&This->framebase.content_window->base.IDispatchEx_iface, id, lcid,
            flags, params, res, ei, caller);
}

static HRESULT HTMLIFrame_get_readystate(HTMLDOMNode *iface, BSTR *p)
{
    HTMLIFrame *This = iframe_from_HTMLDOMNode(iface);

    return IHTMLFrameBase2_get_readyState(&This->framebase.IHTMLFrameBase2_iface, p);
}

static HRESULT HTMLIFrame_bind_to_tree(HTMLDOMNode *iface)
{
    HTMLIFrame *This = iframe_from_HTMLDOMNode(iface);
    nsIDOMDocument *nsdoc;
    nsresult nsres;
    HRESULT hres;

    nsres = nsIDOMHTMLIFrameElement_GetContentDocument(This->framebase.nsiframe, &nsdoc);
    if(NS_FAILED(nsres) || !nsdoc) {
        ERR("GetContentDocument failed: %08x\n", nsres);
        return E_FAIL;
    }

    hres = set_frame_doc(&This->framebase, nsdoc);
    nsIDOMDocument_Release(nsdoc);
    return hres;
}

static void HTMLIFrame_traverse(HTMLDOMNode *iface, nsCycleCollectionTraversalCallback *cb)
{
    HTMLIFrame *This = iframe_from_HTMLDOMNode(iface);

    if(This->framebase.nsiframe)
        note_cc_edge((nsISupports*)This->framebase.nsiframe, "This->nsiframe", cb);
}

static void HTMLIFrame_unlink(HTMLDOMNode *iface)
{
    HTMLIFrame *This = iframe_from_HTMLDOMNode(iface);

    if(This->framebase.nsiframe) {
        nsIDOMHTMLIFrameElement *nsiframe = This->framebase.nsiframe;

        This->framebase.nsiframe = NULL;
        nsIDOMHTMLIFrameElement_Release(nsiframe);
    }
}

static const NodeImplVtbl HTMLIFrameImplVtbl = {
    &CLSID_HTMLIFrame,
    HTMLIFrame_QI,
    HTMLIFrame_destructor,
    HTMLElement_cpc,
    HTMLElement_clone,
    HTMLElement_handle_event,
    HTMLElement_get_attr_col,
    NULL,
    NULL,
    NULL,
    HTMLIFrame_get_document,
    HTMLIFrame_get_readystate,
    HTMLIFrame_get_dispid,
    HTMLIFrame_invoke,
    HTMLIFrame_bind_to_tree,
    HTMLIFrame_traverse,
    HTMLIFrame_unlink
};

static const tid_t HTMLIFrame_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLFrameBase_tid,
    IHTMLFrameBase2_tid,
    IHTMLIFrameElement_tid,
    IHTMLIFrameElement2_tid,
    IHTMLIFrameElement3_tid,
    0
};

static dispex_static_data_t HTMLIFrame_dispex = {
    NULL,
    DispHTMLIFrame_tid,
    HTMLIFrame_iface_tids,
    HTMLElement_init_dispex_info
};

HRESULT HTMLIFrame_Create(HTMLDocumentNode *doc, nsIDOMElement *nselem, HTMLElement **elem)
{
    HTMLIFrame *ret;

    ret = heap_alloc_zero(sizeof(HTMLIFrame));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLIFrameElement_iface.lpVtbl = &HTMLIFrameElementVtbl;
    ret->IHTMLIFrameElement2_iface.lpVtbl = &HTMLIFrameElement2Vtbl;
    ret->IHTMLIFrameElement3_iface.lpVtbl = &HTMLIFrameElement3Vtbl;
    ret->framebase.element.node.vtbl = &HTMLIFrameImplVtbl;

    HTMLFrameBase_Init(&ret->framebase, doc, nselem, &HTMLIFrame_dispex);

    *elem = &ret->framebase.element;
    return S_OK;
}
