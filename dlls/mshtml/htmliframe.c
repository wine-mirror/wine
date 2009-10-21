/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct {
    HTMLElement element;
    const IHTMLFrameBaseVtbl   *lpIHTMLFrameBaseVtbl;
    const IHTMLFrameBase2Vtbl  *lpIHTMLFrameBase2Vtbl;

    LONG ref;

    nsIDOMHTMLIFrameElement *nsiframe;
    HTMLWindow *content_window;
} HTMLIFrame;

#define HTMLFRAMEBASE(x)   (&(x)->lpIHTMLFrameBaseVtbl)
#define HTMLFRAMEBASE2(x)  (&(x)->lpIHTMLFrameBase2Vtbl)

#define HTMLFRAMEBASE_THIS(iface) DEFINE_THIS(HTMLIFrame, IHTMLFrameBase, iface)

static HRESULT WINAPI HTMLIFrameBase_QueryInterface(IHTMLFrameBase *iface, REFIID riid, void **ppv)
{
    HTMLIFrame *This = HTMLFRAMEBASE_THIS(iface);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(&This->element.node), riid, ppv);
}

static ULONG WINAPI HTMLIFrameBase_AddRef(IHTMLFrameBase *iface)
{
    HTMLIFrame *This = HTMLFRAMEBASE_THIS(iface);

    return IHTMLDOMNode_AddRef(HTMLDOMNODE(&This->element.node));
}

static ULONG WINAPI HTMLIFrameBase_Release(IHTMLFrameBase *iface)
{
    HTMLIFrame *This = HTMLFRAMEBASE_THIS(iface);

    return IHTMLDOMNode_Release(HTMLDOMNODE(&This->element.node));
}

static HRESULT WINAPI HTMLIFrameBase_GetTypeInfoCount(IHTMLFrameBase *iface, UINT *pctinfo)
{
    HTMLIFrame *This = HTMLFRAMEBASE_THIS(iface);

    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(&This->element.node.dispex), pctinfo);
}

static HRESULT WINAPI HTMLIFrameBase_GetTypeInfo(IHTMLFrameBase *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLIFrame *This = HTMLFRAMEBASE_THIS(iface);

    return IDispatchEx_GetTypeInfo(DISPATCHEX(&This->element.node.dispex), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLIFrameBase_GetIDsOfNames(IHTMLFrameBase *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLIFrame *This = HTMLFRAMEBASE_THIS(iface);

    return IDispatchEx_GetIDsOfNames(DISPATCHEX(&This->element.node.dispex), riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLIFrameBase_Invoke(IHTMLFrameBase *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLIFrame *This = HTMLFRAMEBASE_THIS(iface);

    return IDispatchEx_Invoke(DISPATCHEX(&This->element.node.dispex), dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLIFrameBase_put_src(IHTMLFrameBase *iface, BSTR v)
{
    HTMLIFrame *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase_get_src(IHTMLFrameBase *iface, BSTR *p)
{
    HTMLIFrame *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase_put_name(IHTMLFrameBase *iface, BSTR v)
{
    HTMLIFrame *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase_get_name(IHTMLFrameBase *iface, BSTR *p)
{
    HTMLIFrame *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase_put_border(IHTMLFrameBase *iface, VARIANT v)
{
    HTMLIFrame *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase_get_border(IHTMLFrameBase *iface, VARIANT *p)
{
    HTMLIFrame *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase_put_frameBorder(IHTMLFrameBase *iface, BSTR v)
{
    HTMLIFrame *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase_get_frameBorder(IHTMLFrameBase *iface, BSTR *p)
{
    HTMLIFrame *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase_put_frameSpacing(IHTMLFrameBase *iface, VARIANT v)
{
    HTMLIFrame *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase_get_frameSpacing(IHTMLFrameBase *iface, VARIANT *p)
{
    HTMLIFrame *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase_put_marginWidth(IHTMLFrameBase *iface, VARIANT v)
{
    HTMLIFrame *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase_get_marginWidth(IHTMLFrameBase *iface, VARIANT *p)
{
    HTMLIFrame *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase_put_marginHeight(IHTMLFrameBase *iface, VARIANT v)
{
    HTMLIFrame *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase_get_marginHeight(IHTMLFrameBase *iface, VARIANT *p)
{
    HTMLIFrame *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase_put_noResize(IHTMLFrameBase *iface, VARIANT_BOOL v)
{
    HTMLIFrame *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase_get_noResize(IHTMLFrameBase *iface, VARIANT_BOOL *p)
{
    HTMLIFrame *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase_put_scrolling(IHTMLFrameBase *iface, BSTR v)
{
    HTMLIFrame *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase_get_scrolling(IHTMLFrameBase *iface, BSTR *p)
{
    HTMLIFrame *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLFrameBaseVtbl HTMLIFrameBaseVtbl = {
    HTMLIFrameBase_QueryInterface,
    HTMLIFrameBase_AddRef,
    HTMLIFrameBase_Release,
    HTMLIFrameBase_GetTypeInfoCount,
    HTMLIFrameBase_GetTypeInfo,
    HTMLIFrameBase_GetIDsOfNames,
    HTMLIFrameBase_Invoke,
    HTMLIFrameBase_put_src,
    HTMLIFrameBase_get_src,
    HTMLIFrameBase_put_name,
    HTMLIFrameBase_get_name,
    HTMLIFrameBase_put_border,
    HTMLIFrameBase_get_border,
    HTMLIFrameBase_put_frameBorder,
    HTMLIFrameBase_get_frameBorder,
    HTMLIFrameBase_put_frameSpacing,
    HTMLIFrameBase_get_frameSpacing,
    HTMLIFrameBase_put_marginWidth,
    HTMLIFrameBase_get_marginWidth,
    HTMLIFrameBase_put_marginHeight,
    HTMLIFrameBase_get_marginHeight,
    HTMLIFrameBase_put_noResize,
    HTMLIFrameBase_get_noResize,
    HTMLIFrameBase_put_scrolling,
    HTMLIFrameBase_get_scrolling
};

#define HTMLFRAMEBASE2_THIS(iface) DEFINE_THIS(HTMLIFrame, IHTMLFrameBase2, iface)

static HRESULT WINAPI HTMLIFrameBase2_QueryInterface(IHTMLFrameBase2 *iface, REFIID riid, void **ppv)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(&This->element.node), riid, ppv);
}

static ULONG WINAPI HTMLIFrameBase2_AddRef(IHTMLFrameBase2 *iface)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);

    return IHTMLDOMNode_AddRef(HTMLDOMNODE(&This->element.node));
}

static ULONG WINAPI HTMLIFrameBase2_Release(IHTMLFrameBase2 *iface)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);

    return IHTMLDOMNode_Release(HTMLDOMNODE(&This->element.node));
}

static HRESULT WINAPI HTMLIFrameBase2_GetTypeInfoCount(IHTMLFrameBase2 *iface, UINT *pctinfo)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase2_GetTypeInfo(IHTMLFrameBase2 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase2_GetIDsOfNames(IHTMLFrameBase2 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase2_Invoke(IHTMLFrameBase2 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase2_get_contentWindow(IHTMLFrameBase2 *iface, IHTMLWindow2 **p)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->content_window) {
        nsIDOMWindow *nswindow;
        nsIDOMDocument *nsdoc;
        nsresult nsres;

        nsres = nsIDOMHTMLIFrameElement_GetContentDocument(This->nsiframe, &nsdoc);
        if(NS_FAILED(nsres)) {
            ERR("GetContentDocument failed: %08x\n", nsres);
            return E_FAIL;
        }

        if(!nsdoc) {
            FIXME("NULL contentDocument\n");
            return E_FAIL;
        }

        nswindow = get_nsdoc_window(nsdoc);
        nsIDOMDocument_Release(nsdoc);
        if(!nswindow)
            return E_FAIL;

        This->content_window = nswindow_to_window(nswindow);
        nsIDOMWindow_Release(nswindow);
        if(!This->content_window) {
            ERR("Could not get window object\n");
            return E_FAIL;
        }

        IHTMLWindow2_AddRef(HTMLWINDOW2(This->content_window));
    }

    IHTMLWindow2_AddRef(HTMLWINDOW2(This->content_window));
    *p = HTMLWINDOW2(This->content_window);
    return S_OK;
}

static HRESULT WINAPI HTMLIFrameBase2_put_onload(IHTMLFrameBase2 *iface, VARIANT v)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase2_get_onload(IHTMLFrameBase2 *iface, VARIANT *p)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase2_put_onreadystatechange(IHTMLFrameBase2 *iface, VARIANT v)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase2_get_onreadystatechange(IHTMLFrameBase2 *iface, VARIANT *p)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase2_get_readyState(IHTMLFrameBase2 *iface, BSTR *p)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase2_put_allowTransparency(IHTMLFrameBase2 *iface, VARIANT_BOOL v)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase2_get_allowTransparency(IHTMLFrameBase2 *iface, VARIANT_BOOL *p)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

#undef HTMLFRAMEBASE2_THIS

static const IHTMLFrameBase2Vtbl HTMLIFrameBase2Vtbl = {
    HTMLIFrameBase2_QueryInterface,
    HTMLIFrameBase2_AddRef,
    HTMLIFrameBase2_Release,
    HTMLIFrameBase2_GetTypeInfoCount,
    HTMLIFrameBase2_GetTypeInfo,
    HTMLIFrameBase2_GetIDsOfNames,
    HTMLIFrameBase2_Invoke,
    HTMLIFrameBase2_get_contentWindow,
    HTMLIFrameBase2_put_onload,
    HTMLIFrameBase2_get_onload,
    HTMLIFrameBase2_put_onreadystatechange,
    HTMLIFrameBase2_get_onreadystatechange,
    HTMLIFrameBase2_get_readyState,
    HTMLIFrameBase2_put_allowTransparency,
    HTMLIFrameBase2_get_allowTransparency
};

#define HTMLIFRAME_NODE_THIS(iface) DEFINE_THIS2(HTMLIFrame, element.node, iface)

static HRESULT HTMLIFrame_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLIFrame *This = HTMLIFRAME_NODE_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IHTMLFrameBase, riid)) {
        TRACE("(%p)->(IID_IHTMLFrameBase %p)\n", This, ppv);
        *ppv = HTMLFRAMEBASE(This);
    }else if(IsEqualGUID(&IID_IHTMLFrameBase2, riid)) {
        TRACE("(%p)->(IID_IHTMLFrameBase2 %p)\n", This, ppv);
        *ppv = HTMLFRAMEBASE2(This);
    }else {
        return HTMLElement_QI(&This->element.node, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static void HTMLIFrame_destructor(HTMLDOMNode *iface)
{
    HTMLIFrame *This = HTMLIFRAME_NODE_THIS(iface);

    if(This->content_window)
        IHTMLWindow2_Release(HTMLWINDOW2(This->content_window));
    if(This->nsiframe)
        nsIDOMHTMLIFrameElement_Release(This->nsiframe);

    HTMLElement_destructor(&This->element.node);
}

#undef HTMLIFRAME_NODE_THIS

static const NodeImplVtbl HTMLIFrameImplVtbl = {
    HTMLIFrame_QI,
    HTMLIFrame_destructor
};

static const tid_t HTMLIFrame_iface_tids[] = {
    IHTMLDOMNode_tid,
    IHTMLDOMNode2_tid,
    IHTMLElement_tid,
    IHTMLElement2_tid,
    IHTMLElement3_tid,
    IHTMLFrameBase_tid,
    IHTMLFrameBase2_tid,
    0
};

static dispex_static_data_t HTMLIFrame_dispex = {
    NULL,
    DispHTMLIFrame_tid,
    NULL,
    HTMLIFrame_iface_tids
};

HTMLElement *HTMLIFrame_Create(nsIDOMHTMLElement *nselem)
{
    HTMLIFrame *ret;
    nsresult nsres;

    ret = heap_alloc_zero(sizeof(HTMLIFrame));

    ret->lpIHTMLFrameBaseVtbl = &HTMLIFrameBaseVtbl;
    ret->lpIHTMLFrameBase2Vtbl = &HTMLIFrameBase2Vtbl;
    ret->element.node.vtbl = &HTMLIFrameImplVtbl;

    HTMLElement_Init(&ret->element, &HTMLIFrame_dispex);

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLIFrameElement, (void**)&ret->nsiframe);
    if(NS_FAILED(nsres))
        ERR("Could not get nsIDOMHTMLIFrameElement iface: %08x\n", nsres);

    return &ret->element;
}
