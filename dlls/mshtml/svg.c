/*
 * Copyright 2019 Jacek Caban for CodeWeavers
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
#include <assert.h>
#include <math.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "ole2.h"
#include "mshtmdid.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

struct SVGElement {
    HTMLElement element;
    ISVGElement ISVGElement_iface;
};

static inline SVGElement *impl_from_ISVGElement(ISVGElement *iface)
{
    return CONTAINING_RECORD(iface, SVGElement, ISVGElement_iface);
}

static HRESULT WINAPI SVGElement_QueryInterface(ISVGElement *iface,
        REFIID riid, void **ppv)
{
    SVGElement *This = impl_from_ISVGElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI SVGElement_AddRef(ISVGElement *iface)
{
    SVGElement *This = impl_from_ISVGElement(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI SVGElement_Release(ISVGElement *iface)
{
    SVGElement *This = impl_from_ISVGElement(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI SVGElement_GetTypeInfoCount(ISVGElement *iface, UINT *pctinfo)
{
    SVGElement *This = impl_from_ISVGElement(iface);
    return IDispatchEx_GetTypeInfoCount(&This->element.node.event_target.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI SVGElement_GetTypeInfo(ISVGElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    SVGElement *This = impl_from_ISVGElement(iface);
    return IDispatchEx_GetTypeInfo(&This->element.node.event_target.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI SVGElement_GetIDsOfNames(ISVGElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    SVGElement *This = impl_from_ISVGElement(iface);
    return IDispatchEx_GetIDsOfNames(&This->element.node.event_target.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI SVGElement_Invoke(ISVGElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    SVGElement *This = impl_from_ISVGElement(iface);
    return IDispatchEx_Invoke(&This->element.node.event_target.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI SVGElement_put_xmlbase(ISVGElement *iface, BSTR v)
{
    SVGElement *This = impl_from_ISVGElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGElement_get_xmlbase(ISVGElement *iface, BSTR *p)
{
    SVGElement *This = impl_from_ISVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGElement_putref_ownerSVGElement(ISVGElement *iface, ISVGSVGElement *v)
{
    SVGElement *This = impl_from_ISVGElement(iface);
    FIXME("(%p)->(%p)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGElement_get_ownerSVGElement(ISVGElement *iface, ISVGSVGElement **p)
{
    SVGElement *This = impl_from_ISVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGElement_putref_viewportElement(ISVGElement *iface, ISVGElement *v)
{
    SVGElement *This = impl_from_ISVGElement(iface);
    FIXME("(%p)->(%p)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGElement_get_viewportElement(ISVGElement *iface, ISVGElement **p)
{
    SVGElement *This = impl_from_ISVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGElement_putref_focusable(ISVGElement *iface, ISVGAnimatedEnumeration *v)
{
    SVGElement *This = impl_from_ISVGElement(iface);
    FIXME("(%p)->(%p)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGElement_get_focusable(ISVGElement *iface, ISVGAnimatedEnumeration **p)
{
    SVGElement *This = impl_from_ISVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const ISVGElementVtbl SVGElementVtbl = {
    SVGElement_QueryInterface,
    SVGElement_AddRef,
    SVGElement_Release,
    SVGElement_GetTypeInfoCount,
    SVGElement_GetTypeInfo,
    SVGElement_GetIDsOfNames,
    SVGElement_Invoke,
    SVGElement_put_xmlbase,
    SVGElement_get_xmlbase,
    SVGElement_putref_ownerSVGElement,
    SVGElement_get_ownerSVGElement,
    SVGElement_putref_viewportElement,
    SVGElement_get_viewportElement,
    SVGElement_putref_focusable,
    SVGElement_get_focusable
};

static inline SVGElement *SVGElement_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, SVGElement, element.node);
}

static HRESULT SVGElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    SVGElement *This = SVGElement_from_HTMLDOMNode(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_ISVGElement, riid))
        *ppv = &This->ISVGElement_iface;
    else
        return HTMLElement_QI(&This->element.node, riid, ppv);

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static const NodeImplVtbl SVGElementImplVtbl = {
    &CLSID_SVGElement,
    SVGElement_QI,
    HTMLElement_destructor,
    HTMLElement_cpc,
    HTMLElement_clone,
    NULL,
    HTMLElement_get_attr_col,
};

static void init_svg_element(SVGElement *svg_element, HTMLDocumentNode *doc, nsIDOMSVGElement *nselem)
{
    if(!svg_element->element.node.vtbl)
        svg_element->element.node.vtbl = &SVGElementImplVtbl;
    svg_element->ISVGElement_iface.lpVtbl = &SVGElementVtbl;
    HTMLElement_Init(&svg_element->element, doc, (nsIDOMElement*)nselem, NULL);
}

HRESULT create_svg_element(HTMLDocumentNode *doc, nsIDOMSVGElement *dom_element, const WCHAR *tag_name, HTMLElement **elem)
{
    SVGElement *svg_element;

    svg_element = heap_alloc_zero(sizeof(*svg_element));
    if(!svg_element)
        return E_OUTOFMEMORY;

    init_svg_element(svg_element, doc, dom_element);
    *elem = &svg_element->element;
    return S_OK;
}
