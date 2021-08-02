/*
 * Copyright 2007,2008,2012 Jacek Caban for CodeWeavers
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

struct HTMLTableCell {
    HTMLElement element;

    IHTMLTableCell IHTMLTableCell_iface;

    nsIDOMHTMLTableCellElement *nscell;
};

static inline HTMLTableCell *impl_from_IHTMLTableCell(IHTMLTableCell *iface)
{
    return CONTAINING_RECORD(iface, HTMLTableCell, IHTMLTableCell_iface);
}

static HRESULT WINAPI HTMLTableCell_QueryInterface(IHTMLTableCell *iface, REFIID riid, void **ppv)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLTableCell_AddRef(IHTMLTableCell *iface)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLTableCell_Release(IHTMLTableCell *iface)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLTableCell_GetTypeInfoCount(IHTMLTableCell *iface, UINT *pctinfo)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    return IDispatchEx_GetTypeInfoCount(&This->element.node.event_target.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLTableCell_GetTypeInfo(IHTMLTableCell *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    return IDispatchEx_GetTypeInfo(&This->element.node.event_target.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLTableCell_GetIDsOfNames(IHTMLTableCell *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    return IDispatchEx_GetIDsOfNames(&This->element.node.event_target.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLTableCell_Invoke(IHTMLTableCell *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
        UINT *puArgErr)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    return IDispatchEx_Invoke(&This->element.node.event_target.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLTableCell_put_rowSpan(IHTMLTableCell *iface, LONG v)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    nsresult nsres;

    TRACE("(%p)->(%d)\n", This, v);

    if(v <= 0)
        return E_INVALIDARG;

    nsres = nsIDOMHTMLTableCellElement_SetRowSpan(This->nscell, v);
    if(NS_FAILED(nsres)) {
        ERR("SetRowSpan failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLTableCell_get_rowSpan(IHTMLTableCell *iface, LONG *p)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLTableCellElement_GetRowSpan(This->nscell, p);
    if(NS_FAILED(nsres)) {
        ERR("GetRowSpan failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLTableCell_put_colSpan(IHTMLTableCell *iface, LONG v)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    nsresult nsres;

    TRACE("(%p)->(%d)\n", This, v);

    if(v <= 0)
        return E_INVALIDARG;

    nsres = nsIDOMHTMLTableCellElement_SetColSpan(This->nscell, v);
    if(NS_FAILED(nsres)) {
        ERR("SetColSpan failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLTableCell_get_colSpan(IHTMLTableCell *iface, LONG *p)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLTableCellElement_GetColSpan(This->nscell, p);
    if(NS_FAILED(nsres)) {
        ERR("GetColSpan failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLTableCell_put_align(IHTMLTableCell *iface, BSTR v)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    nsAString str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&str, v);
    nsres = nsIDOMHTMLTableCellElement_SetAlign(This->nscell, &str);
    nsAString_Finish(&str);
    if (NS_FAILED(nsres)) {
        ERR("Set Align failed: %08x\n", nsres);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLTableCell_get_align(IHTMLTableCell *iface, BSTR *p)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    nsAString str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&str, NULL);
    nsres = nsIDOMHTMLTableCellElement_GetAlign(This->nscell, &str);

    return return_nsstr(nsres, &str, p);
}

static HRESULT WINAPI HTMLTableCell_put_vAlign(IHTMLTableCell *iface, BSTR v)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_get_vAlign(IHTMLTableCell *iface, BSTR *p)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_put_bgColor(IHTMLTableCell *iface, VARIANT v)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    nsAString strColor;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    if(!variant_to_nscolor(&v, &strColor))
        return S_OK;

    nsres = nsIDOMHTMLTableCellElement_SetBgColor(This->nscell, &strColor);
    nsAString_Finish(&strColor);
    if(NS_FAILED(nsres)) {
        ERR("SetBgColor(%s) failed: %08x\n", debugstr_variant(&v), nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLTableCell_get_bgColor(IHTMLTableCell *iface, VARIANT *p)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    nsAString strColor;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&strColor, NULL);
    nsres = nsIDOMHTMLTableCellElement_GetBgColor(This->nscell, &strColor);

    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *color;
        nsAString_GetData(&strColor, &color);
        V_VT(p) = VT_BSTR;
        hres = nscolor_to_str(color, &V_BSTR(p));
    }else {
        ERR("GetBgColor failed: %08x\n", nsres);
        hres = E_FAIL;
    }
    nsAString_Finish(&strColor);
    return hres;
}

static HRESULT WINAPI HTMLTableCell_put_noWrap(IHTMLTableCell *iface, VARIANT_BOOL v)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_get_noWrap(IHTMLTableCell *iface, VARIANT_BOOL *p)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_put_background(IHTMLTableCell *iface, BSTR v)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_get_background(IHTMLTableCell *iface, BSTR *p)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_put_borderColor(IHTMLTableCell *iface, VARIANT v)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_get_borderColor(IHTMLTableCell *iface, VARIANT *p)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_put_borderColorLight(IHTMLTableCell *iface, VARIANT v)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_get_borderColorLight(IHTMLTableCell *iface, VARIANT *p)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_put_borderColorDark(IHTMLTableCell *iface, VARIANT v)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_get_borderColorDark(IHTMLTableCell *iface, VARIANT *p)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_put_width(IHTMLTableCell *iface, VARIANT v)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    nsAString nsstr;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    hres = variant_to_nsstr(&v, FALSE, &nsstr);
    if(FAILED(hres))
        return hres;

    nsres = nsIDOMHTMLTableCellElement_SetWidth(This->nscell, &nsstr);
    nsAString_Finish(&nsstr);
    return map_nsresult(nsres);
}

static HRESULT WINAPI HTMLTableCell_get_width(IHTMLTableCell *iface, VARIANT *p)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMHTMLTableCellElement_GetWidth(This->nscell, &nsstr);
    return return_nsstr_variant(nsres, &nsstr, NSSTR_IMPLICIT_PX, p);
}

static HRESULT WINAPI HTMLTableCell_put_height(IHTMLTableCell *iface, VARIANT v)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    nsAString nsstr;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    hres = variant_to_nsstr(&v, FALSE, &nsstr);
    if(FAILED(hres))
        return hres;

    nsres = nsIDOMHTMLTableCellElement_SetHeight(This->nscell, &nsstr);
    nsAString_Finish(&nsstr);
    return map_nsresult(nsres);
}

static HRESULT WINAPI HTMLTableCell_get_height(IHTMLTableCell *iface, VARIANT *p)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMHTMLTableCellElement_GetHeight(This->nscell, &nsstr);
    return return_nsstr_variant(nsres, &nsstr, NSSTR_IMPLICIT_PX, p);
}

static HRESULT WINAPI HTMLTableCell_get_cellIndex(IHTMLTableCell *iface, LONG *p)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);
    nsres = nsIDOMHTMLTableCellElement_GetCellIndex(This->nscell, p);
    if (NS_FAILED(nsres)) {
        ERR("Get CellIndex failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static const IHTMLTableCellVtbl HTMLTableCellVtbl = {
    HTMLTableCell_QueryInterface,
    HTMLTableCell_AddRef,
    HTMLTableCell_Release,
    HTMLTableCell_GetTypeInfoCount,
    HTMLTableCell_GetTypeInfo,
    HTMLTableCell_GetIDsOfNames,
    HTMLTableCell_Invoke,
    HTMLTableCell_put_rowSpan,
    HTMLTableCell_get_rowSpan,
    HTMLTableCell_put_colSpan,
    HTMLTableCell_get_colSpan,
    HTMLTableCell_put_align,
    HTMLTableCell_get_align,
    HTMLTableCell_put_vAlign,
    HTMLTableCell_get_vAlign,
    HTMLTableCell_put_bgColor,
    HTMLTableCell_get_bgColor,
    HTMLTableCell_put_noWrap,
    HTMLTableCell_get_noWrap,
    HTMLTableCell_put_background,
    HTMLTableCell_get_background,
    HTMLTableCell_put_borderColor,
    HTMLTableCell_get_borderColor,
    HTMLTableCell_put_borderColorLight,
    HTMLTableCell_get_borderColorLight,
    HTMLTableCell_put_borderColorDark,
    HTMLTableCell_get_borderColorDark,
    HTMLTableCell_put_width,
    HTMLTableCell_get_width,
    HTMLTableCell_put_height,
    HTMLTableCell_get_height,
    HTMLTableCell_get_cellIndex
};

static inline HTMLTableCell *HTMLTableCell_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLTableCell, element.node);
}

static HRESULT HTMLTableCell_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLTableCell *This = HTMLTableCell_from_HTMLDOMNode(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IHTMLTableCell_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IHTMLTableCell_iface;
    }else if(IsEqualGUID(&IID_IHTMLTableCell, riid)) {
        TRACE("(%p)->(IID_IHTMLTableCell %p)\n", This, ppv);
        *ppv = &This->IHTMLTableCell_iface;
    }else {
        return HTMLElement_QI(&This->element.node, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static void HTMLTableCell_destructor(HTMLDOMNode *iface)
{
    HTMLTableCell *This = HTMLTableCell_from_HTMLDOMNode(iface);

    HTMLElement_destructor(&This->element.node);
}

static void HTMLTableCell_traverse(HTMLDOMNode *iface, nsCycleCollectionTraversalCallback *cb)
{
    HTMLTableCell *This = HTMLTableCell_from_HTMLDOMNode(iface);

    if(This->nscell)
        note_cc_edge((nsISupports*)This->nscell, "This->nstablecell", cb);
}

static void HTMLTableCell_unlink(HTMLDOMNode *iface)
{
    HTMLTableCell *This = HTMLTableCell_from_HTMLDOMNode(iface);

    if(This->nscell) {
        nsIDOMHTMLTableCellElement *nscell = This->nscell;

        This->nscell = NULL;
        nsIDOMHTMLTableCellElement_Release(nscell);
    }
}

static const NodeImplVtbl HTMLTableCellImplVtbl = {
    &CLSID_HTMLTableCell,
    HTMLTableCell_QI,
    HTMLTableCell_destructor,
    HTMLElement_cpc,
    HTMLElement_clone,
    HTMLElement_handle_event,
    HTMLElement_get_attr_col,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    HTMLTableCell_traverse,
    HTMLTableCell_unlink
};

static const tid_t HTMLTableCell_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLTableCell_tid,
    0
};

static dispex_static_data_t HTMLTableCell_dispex = {
    NULL,
    DispHTMLTableCell_tid,
    HTMLTableCell_iface_tids,
    HTMLElement_init_dispex_info
};

HRESULT HTMLTableCell_Create(HTMLDocumentNode *doc, nsIDOMElement *nselem, HTMLElement **elem)
{
    HTMLTableCell *ret;
    nsresult nsres;

    ret = heap_alloc_zero(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLTableCell_iface.lpVtbl = &HTMLTableCellVtbl;
    ret->element.node.vtbl = &HTMLTableCellImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLTableCell_dispex);

    nsres = nsIDOMElement_QueryInterface(nselem, &IID_nsIDOMHTMLTableCellElement, (void**)&ret->nscell);
    assert(nsres == NS_OK);

    *elem = &ret->element;
    return S_OK;
}

struct HTMLTableRow {
    HTMLElement element;

    IHTMLTableRow IHTMLTableRow_iface;

    nsIDOMHTMLTableRowElement *nsrow;
};

static inline HTMLTableRow *impl_from_IHTMLTableRow(IHTMLTableRow *iface)
{
    return CONTAINING_RECORD(iface, HTMLTableRow, IHTMLTableRow_iface);
}

static HRESULT WINAPI HTMLTableRow_QueryInterface(IHTMLTableRow *iface,
        REFIID riid, void **ppv)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLTableRow_AddRef(IHTMLTableRow *iface)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLTableRow_Release(IHTMLTableRow *iface)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLTableRow_GetTypeInfoCount(IHTMLTableRow *iface, UINT *pctinfo)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    return IDispatchEx_GetTypeInfoCount(&This->element.node.event_target.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLTableRow_GetTypeInfo(IHTMLTableRow *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    return IDispatchEx_GetTypeInfo(&This->element.node.event_target.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLTableRow_GetIDsOfNames(IHTMLTableRow *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    return IDispatchEx_GetIDsOfNames(&This->element.node.event_target.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLTableRow_Invoke(IHTMLTableRow *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    return IDispatchEx_Invoke(&This->element.node.event_target.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLTableRow_put_align(IHTMLTableRow *iface, BSTR v)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    nsAString val;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&val, v);

    nsres = nsIDOMHTMLTableRowElement_SetAlign(This->nsrow, &val);
    nsAString_Finish(&val);
    if (NS_FAILED(nsres)){
        ERR("Set Align(%s) failed!\n", debugstr_w(v));
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLTableRow_get_align(IHTMLTableRow *iface, BSTR *p)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    nsAString val;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&val, NULL);
    nsres = nsIDOMHTMLTableRowElement_GetAlign(This->nsrow, &val);

    return return_nsstr(nsres, &val, p);
}

static HRESULT WINAPI HTMLTableRow_put_vAlign(IHTMLTableRow *iface, BSTR v)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    nsAString val;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&val, v);

    nsres = nsIDOMHTMLTableRowElement_SetVAlign(This->nsrow, &val);
    nsAString_Finish(&val);

    if (NS_FAILED(nsres)){
        ERR("Set VAlign(%s) failed!\n", debugstr_w(v));
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLTableRow_get_vAlign(IHTMLTableRow *iface, BSTR *p)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    nsAString val;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&val, NULL);
    nsres = nsIDOMHTMLTableRowElement_GetVAlign(This->nsrow, &val);

    return return_nsstr(nsres, &val, p);
}

static HRESULT WINAPI HTMLTableRow_put_bgColor(IHTMLTableRow *iface, VARIANT v)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    nsAString val;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    if (!variant_to_nscolor(&v, &val))
        return S_OK;

    nsres = nsIDOMHTMLTableRowElement_SetBgColor(This->nsrow, &val);
    nsAString_Finish(&val);

    if (NS_FAILED(nsres)){
        ERR("Set BgColor(%s) failed!\n", debugstr_variant(&v));
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLTableRow_get_bgColor(IHTMLTableRow *iface, VARIANT *p)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    nsAString strColor;
    nsresult nsres;
    HRESULT hres;
    const PRUnichar *color;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&strColor, NULL);
    nsres = nsIDOMHTMLTableRowElement_GetBgColor(This->nsrow, &strColor);

    if(NS_SUCCEEDED(nsres)) {
       nsAString_GetData(&strColor, &color);
       V_VT(p) = VT_BSTR;
       hres = nscolor_to_str(color, &V_BSTR(p));
    }else {
       ERR("SetBgColor failed: %08x\n", nsres);
       hres = E_FAIL;
    }

    nsAString_Finish(&strColor);
    return hres;
}

static HRESULT WINAPI HTMLTableRow_put_borderColor(IHTMLTableRow *iface, VARIANT v)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableRow_get_borderColor(IHTMLTableRow *iface, VARIANT *p)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableRow_put_borderColorLight(IHTMLTableRow *iface, VARIANT v)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableRow_get_borderColorLight(IHTMLTableRow *iface, VARIANT *p)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableRow_put_borderColorDark(IHTMLTableRow *iface, VARIANT v)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableRow_get_borderColorDark(IHTMLTableRow *iface, VARIANT *p)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableRow_get_rowIndex(IHTMLTableRow *iface, LONG *p)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);
    nsres = nsIDOMHTMLTableRowElement_GetRowIndex(This->nsrow, p);
    if(NS_FAILED(nsres)) {
        ERR("Get rowIndex failed: %08x\n", nsres);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLTableRow_get_sectionRowIndex(IHTMLTableRow *iface, LONG *p)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);
    nsres = nsIDOMHTMLTableRowElement_GetSectionRowIndex(This->nsrow, p);
    if(NS_FAILED(nsres)) {
        ERR("Get selectionRowIndex failed: %08x\n", nsres);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLTableRow_get_cells(IHTMLTableRow *iface, IHTMLElementCollection **p)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    nsIDOMHTMLCollection *nscol;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLTableRowElement_GetCells(This->nsrow, &nscol);
    if(NS_FAILED(nsres)) {
        ERR("GetCells failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = create_collection_from_htmlcol(nscol, dispex_compat_mode(&This->element.node.event_target.dispex));

    nsIDOMHTMLCollection_Release(nscol);
    return S_OK;
}

static HRESULT WINAPI HTMLTableRow_insertCell(IHTMLTableRow *iface, LONG index, IDispatch **row)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    nsIDOMHTMLElement *nselem;
    HTMLElement *elem;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%d %p)\n", This, index, row);
    nsres = nsIDOMHTMLTableRowElement_InsertCell(This->nsrow, index, &nselem);
    if(NS_FAILED(nsres)) {
        ERR("Insert Cell at %d failed: %08x\n", index, nsres);
        return E_FAIL;
    }

    hres = HTMLTableCell_Create(This->element.node.doc, (nsIDOMElement*)nselem, &elem);
    nsIDOMHTMLElement_Release(nselem);
    if (FAILED(hres)) {
        ERR("Create TableCell failed: %08x\n", hres);
        return hres;
    }

    *row = (IDispatch *)&elem->IHTMLElement_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLTableRow_deleteCell(IHTMLTableRow *iface, LONG index)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    nsresult nsres;

    TRACE("(%p)->(%d)\n", This, index);
    nsres = nsIDOMHTMLTableRowElement_DeleteCell(This->nsrow, index);
    if(NS_FAILED(nsres)) {
        ERR("Delete Cell failed: %08x\n", nsres);
        return E_FAIL;
    }
    return S_OK;
}

static const IHTMLTableRowVtbl HTMLTableRowVtbl = {
    HTMLTableRow_QueryInterface,
    HTMLTableRow_AddRef,
    HTMLTableRow_Release,
    HTMLTableRow_GetTypeInfoCount,
    HTMLTableRow_GetTypeInfo,
    HTMLTableRow_GetIDsOfNames,
    HTMLTableRow_Invoke,
    HTMLTableRow_put_align,
    HTMLTableRow_get_align,
    HTMLTableRow_put_vAlign,
    HTMLTableRow_get_vAlign,
    HTMLTableRow_put_bgColor,
    HTMLTableRow_get_bgColor,
    HTMLTableRow_put_borderColor,
    HTMLTableRow_get_borderColor,
    HTMLTableRow_put_borderColorLight,
    HTMLTableRow_get_borderColorLight,
    HTMLTableRow_put_borderColorDark,
    HTMLTableRow_get_borderColorDark,
    HTMLTableRow_get_rowIndex,
    HTMLTableRow_get_sectionRowIndex,
    HTMLTableRow_get_cells,
    HTMLTableRow_insertCell,
    HTMLTableRow_deleteCell
};

static inline HTMLTableRow *HTMLTableRow_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLTableRow, element.node);
}

static HRESULT HTMLTableRow_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLTableRow *This = HTMLTableRow_from_HTMLDOMNode(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IHTMLTableRow_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IHTMLTableRow_iface;
    }else if(IsEqualGUID(&IID_IHTMLTableRow, riid)) {
        TRACE("(%p)->(IID_IHTMLTableRow %p)\n", This, ppv);
        *ppv = &This->IHTMLTableRow_iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    return HTMLElement_QI(&This->element.node, riid, ppv);
}

static void HTMLTableRow_traverse(HTMLDOMNode *iface, nsCycleCollectionTraversalCallback *cb)
{
    HTMLTableRow *This = HTMLTableRow_from_HTMLDOMNode(iface);

    if(This->nsrow)
        note_cc_edge((nsISupports*)This->nsrow, "This->nstablerow", cb);
}

static void HTMLTableRow_unlink(HTMLDOMNode *iface)
{
    HTMLTableRow *This = HTMLTableRow_from_HTMLDOMNode(iface);

    if(This->nsrow) {
        nsIDOMHTMLTableRowElement *nsrow = This->nsrow;

        This->nsrow = NULL;
        nsIDOMHTMLTableRowElement_Release(nsrow);
    }
}

static const NodeImplVtbl HTMLTableRowImplVtbl = {
    &CLSID_HTMLTableRow,
    HTMLTableRow_QI,
    HTMLElement_destructor,
    HTMLElement_cpc,
    HTMLElement_clone,
    HTMLElement_handle_event,
    HTMLElement_get_attr_col,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    HTMLTableRow_traverse,
    HTMLTableRow_unlink
};

static const tid_t HTMLTableRow_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLTableRow_tid,
    0
};

static dispex_static_data_t HTMLTableRow_dispex = {
    NULL,
    DispHTMLTableRow_tid,
    HTMLTableRow_iface_tids,
    HTMLElement_init_dispex_info
};

HRESULT HTMLTableRow_Create(HTMLDocumentNode *doc, nsIDOMElement *nselem, HTMLElement **elem)
{
    HTMLTableRow *ret;
    nsresult nsres;

    ret = heap_alloc_zero(sizeof(HTMLTableRow));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLTableRow_iface.lpVtbl = &HTMLTableRowVtbl;
    ret->element.node.vtbl = &HTMLTableRowImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLTableRow_dispex);

    nsres = nsIDOMElement_QueryInterface(nselem, &IID_nsIDOMHTMLTableRowElement, (void**)&ret->nsrow);
    assert(nsres == NS_OK);

    *elem = &ret->element;
    return S_OK;
}

struct HTMLTable {
    HTMLElement element;

    IHTMLTable  IHTMLTable_iface;
    IHTMLTable2 IHTMLTable2_iface;
    IHTMLTable3 IHTMLTable3_iface;

    nsIDOMHTMLTableElement *nstable;
};

static inline HTMLTable *impl_from_IHTMLTable(IHTMLTable *iface)
{
    return CONTAINING_RECORD(iface, HTMLTable, IHTMLTable_iface);
}

static inline HTMLTable *impl_from_IHTMLTable2(IHTMLTable2 *iface)
{
    return CONTAINING_RECORD(iface, HTMLTable, IHTMLTable2_iface);
}

static inline HTMLTable *impl_from_IHTMLTable3(IHTMLTable3 *iface)
{
    return CONTAINING_RECORD(iface, HTMLTable, IHTMLTable3_iface);
}

static HRESULT var2str(const VARIANT *p, nsAString *nsstr)
{
    BSTR str;
    BOOL ret;
    HRESULT hres;

    switch(V_VT(p)) {
    case VT_BSTR:
        return nsAString_Init(nsstr, V_BSTR(p))?
            S_OK : E_OUTOFMEMORY;
    case VT_R8:
        hres = VarBstrFromR8(V_R8(p), MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT), 0, &str);
        break;
    case VT_R4:
        hres = VarBstrFromR4(V_R4(p), MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT), 0, &str);
        break;
    case VT_I4:
        hres = VarBstrFromI4(V_I4(p), MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT), 0, &str);
        break;
    default:
        FIXME("unsupported arg %s\n", debugstr_variant(p));
        return E_NOTIMPL;
    }
    if (FAILED(hres))
        return hres;

    ret = nsAString_Init(nsstr, str);
    SysFreeString(str);
    return ret ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI HTMLTable_QueryInterface(IHTMLTable *iface,
                                                         REFIID riid, void **ppv)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLTable_AddRef(IHTMLTable *iface)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLTable_Release(IHTMLTable *iface)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLTable_GetTypeInfoCount(IHTMLTable *iface, UINT *pctinfo)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    return IDispatchEx_GetTypeInfoCount(&This->element.node.event_target.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLTable_GetTypeInfo(IHTMLTable *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    return IDispatchEx_GetTypeInfo(&This->element.node.event_target.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLTable_GetIDsOfNames(IHTMLTable *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    return IDispatchEx_GetIDsOfNames(&This->element.node.event_target.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLTable_Invoke(IHTMLTable *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    return IDispatchEx_Invoke(&This->element.node.event_target.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLTable_put_cols(IHTMLTable *iface, LONG v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_cols(IHTMLTable *iface, LONG *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_border(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_border(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_frame(IHTMLTable *iface, BSTR v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsAString str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&str, v);
    nsres = nsIDOMHTMLTableElement_SetFrame(This->nstable, &str);
    nsAString_Finish(&str);

    if (NS_FAILED(nsres)) {
        ERR("SetFrame(%s) failed: %08x\n", debugstr_w(v), nsres);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLTable_get_frame(IHTMLTable *iface, BSTR *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsAString str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&str, NULL);
    nsres = nsIDOMHTMLTableElement_GetFrame(This->nstable, &str);

    return return_nsstr(nsres, &str, p);
}

static HRESULT WINAPI HTMLTable_put_rules(IHTMLTable *iface, BSTR v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_rules(IHTMLTable *iface, BSTR *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_cellSpacing(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsAString nsstr;
    WCHAR buf[64];
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    switch(V_VT(&v)) {
    case VT_BSTR:
        nsAString_InitDepend(&nsstr, V_BSTR(&v));
        break;
    case VT_I4: {
        swprintf(buf, ARRAY_SIZE(buf), L"%d", V_I4(&v));
        nsAString_InitDepend(&nsstr, buf);
        break;
    }
    default:
        FIXME("unsupported arg %s\n", debugstr_variant(&v));
        return E_NOTIMPL;
    }

    nsres = nsIDOMHTMLTableElement_SetCellSpacing(This->nstable, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres)) {
        ERR("SetCellSpacing failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLTable_get_cellSpacing(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMHTMLTableElement_GetCellSpacing(This->nstable, &nsstr);
    V_VT(p) = VT_BSTR;
    return return_nsstr(nsres, &nsstr, &V_BSTR(p));
}

static HRESULT WINAPI HTMLTable_put_cellPadding(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsAString val;
    HRESULT hres;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    hres = var2str(&v, &val);
    if(FAILED(hres))
        return hres;

    nsres = nsIDOMHTMLTableElement_SetCellPadding(This->nstable, &val);
    nsAString_Finish(&val);
    if(NS_FAILED(nsres)) {
        ERR("Set Width(%s) failed, err = %08x\n", debugstr_variant(&v), nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLTable_get_cellPadding(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsAString val;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&val, NULL);
    nsres = nsIDOMHTMLTableElement_GetCellPadding(This->nstable, &val);
    return return_nsstr_variant(nsres, &val, 0, p);
}

static HRESULT WINAPI HTMLTable_put_background(IHTMLTable *iface, BSTR v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_background(IHTMLTable *iface, BSTR *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_bgColor(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsAString val;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    if(!variant_to_nscolor(&v, &val))
        return S_OK;

    nsres = nsIDOMHTMLTableElement_SetBgColor(This->nstable, &val);
    nsAString_Finish(&val);
    if (NS_FAILED(nsres)){
        ERR("Set BgColor(%s) failed!\n", debugstr_variant(&v));
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLTable_get_bgColor(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsAString strColor;
    nsresult nsres;
    HRESULT hres;
    const PRUnichar *color;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&strColor, NULL);
    nsres = nsIDOMHTMLTableElement_GetBgColor(This->nstable, &strColor);

    if(NS_SUCCEEDED(nsres)) {
       nsAString_GetData(&strColor, &color);
       V_VT(p) = VT_BSTR;
       hres = nscolor_to_str(color, &V_BSTR(p));
    }else {
       ERR("SetBgColor failed: %08x\n", nsres);
       hres = E_FAIL;
    }

    nsAString_Finish(&strColor);
    return hres;
}

static HRESULT WINAPI HTMLTable_put_borderColor(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_borderColor(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_borderColorLight(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_borderColorLight(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_borderColorDark(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_borderColorDark(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_align(IHTMLTable *iface, BSTR v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsAString val;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&val, v);

    nsres = nsIDOMHTMLTableElement_SetAlign(This->nstable, &val);
    nsAString_Finish(&val);
    if (NS_FAILED(nsres)){
        ERR("Set Align(%s) failed!\n", debugstr_w(v));
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLTable_get_align(IHTMLTable *iface, BSTR *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsAString val;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&val, NULL);
    nsres = nsIDOMHTMLTableElement_GetAlign(This->nstable, &val);

    return return_nsstr(nsres, &val, p);
}

static HRESULT WINAPI HTMLTable_refresh(IHTMLTable *iface)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_rows(IHTMLTable *iface, IHTMLElementCollection **p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsIDOMHTMLCollection *nscol;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLTableElement_GetRows(This->nstable, &nscol);
    if(NS_FAILED(nsres)) {
        ERR("GetRows failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = create_collection_from_htmlcol(nscol, dispex_compat_mode(&This->element.node.event_target.dispex));

    nsIDOMHTMLCollection_Release(nscol);
    return S_OK;
}

static HRESULT WINAPI HTMLTable_put_width(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsAString val;
    HRESULT hres;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    hres = var2str(&v, &val);

    if (FAILED(hres)){
        ERR("Set Width(%s) failed when initializing a nsAString, err = %08x\n",
            debugstr_variant(&v), hres);
        return hres;
    }

    nsres = nsIDOMHTMLTableElement_SetWidth(This->nstable, &val);
    nsAString_Finish(&val);

    if (NS_FAILED(nsres)){
        ERR("Set Width(%s) failed, err = %08x\n", debugstr_variant(&v), nsres);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLTable_get_width(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsAString val;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&val, NULL);
    nsres = nsIDOMHTMLTableElement_GetWidth(This->nstable, &val);
    return return_nsstr_variant(nsres, &val, NSSTR_IMPLICIT_PX, p);
}

static HRESULT WINAPI HTMLTable_put_height(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_height(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_dataPageSize(IHTMLTable *iface, LONG v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_dataPageSize(IHTMLTable *iface, LONG *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_nextPage(IHTMLTable *iface)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_previousPage(IHTMLTable *iface)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_tHead(IHTMLTable *iface, IHTMLTableSection **p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_tFoot(IHTMLTable *iface, IHTMLTableSection **p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_tBodies(IHTMLTable *iface, IHTMLElementCollection **p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsIDOMHTMLCollection *nscol = NULL;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLTableElement_GetTBodies(This->nstable, &nscol);
    if(NS_FAILED(nsres)) {
        ERR("GetTBodies failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = create_collection_from_htmlcol(nscol, dispex_compat_mode(&This->element.node.event_target.dispex));

    nsIDOMHTMLCollection_Release(nscol);
    return S_OK;
}

static HRESULT WINAPI HTMLTable_get_caption(IHTMLTable *iface, IHTMLTableCaption **p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_createTHead(IHTMLTable *iface, IDispatch **head)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, head);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_deleteTHead(IHTMLTable *iface)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_createTFoot(IHTMLTable *iface, IDispatch **foot)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, foot);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_deleteTFoot(IHTMLTable *iface)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_createCaption(IHTMLTable *iface, IHTMLTableCaption **caption)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, caption);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_deleteCaption(IHTMLTable *iface)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_insertRow(IHTMLTable *iface, LONG index, IDispatch **row)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsIDOMHTMLElement *nselem;
    HTMLElement *elem;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%d %p)\n", This, index, row);
    nsres = nsIDOMHTMLTableElement_InsertRow(This->nstable, index, &nselem);
    if(NS_FAILED(nsres)) {
        ERR("Insert Row at %d failed: %08x\n", index, nsres);
        return E_FAIL;
    }

    hres = HTMLTableRow_Create(This->element.node.doc, (nsIDOMElement*)nselem, &elem);
    nsIDOMHTMLElement_Release(nselem);
    if (FAILED(hres)) {
        ERR("Create TableRow failed: %08x\n", hres);
        return hres;
    }

    *row = (IDispatch *)&elem->IHTMLElement_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLTable_deleteRow(IHTMLTable *iface, LONG index)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsresult nsres;

    TRACE("(%p)->(%d)\n", This, index);
    nsres = nsIDOMHTMLTableElement_DeleteRow(This->nstable, index);
    if(NS_FAILED(nsres)) {
        ERR("Delete Row failed: %08x\n", nsres);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLTable_get_readyState(IHTMLTable *iface, BSTR *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_onreadystatechange(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_onreadystatechange(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLTableVtbl HTMLTableVtbl = {
    HTMLTable_QueryInterface,
    HTMLTable_AddRef,
    HTMLTable_Release,
    HTMLTable_GetTypeInfoCount,
    HTMLTable_GetTypeInfo,
    HTMLTable_GetIDsOfNames,
    HTMLTable_Invoke,
    HTMLTable_put_cols,
    HTMLTable_get_cols,
    HTMLTable_put_border,
    HTMLTable_get_border,
    HTMLTable_put_frame,
    HTMLTable_get_frame,
    HTMLTable_put_rules,
    HTMLTable_get_rules,
    HTMLTable_put_cellSpacing,
    HTMLTable_get_cellSpacing,
    HTMLTable_put_cellPadding,
    HTMLTable_get_cellPadding,
    HTMLTable_put_background,
    HTMLTable_get_background,
    HTMLTable_put_bgColor,
    HTMLTable_get_bgColor,
    HTMLTable_put_borderColor,
    HTMLTable_get_borderColor,
    HTMLTable_put_borderColorLight,
    HTMLTable_get_borderColorLight,
    HTMLTable_put_borderColorDark,
    HTMLTable_get_borderColorDark,
    HTMLTable_put_align,
    HTMLTable_get_align,
    HTMLTable_refresh,
    HTMLTable_get_rows,
    HTMLTable_put_width,
    HTMLTable_get_width,
    HTMLTable_put_height,
    HTMLTable_get_height,
    HTMLTable_put_dataPageSize,
    HTMLTable_get_dataPageSize,
    HTMLTable_nextPage,
    HTMLTable_previousPage,
    HTMLTable_get_tHead,
    HTMLTable_get_tFoot,
    HTMLTable_get_tBodies,
    HTMLTable_get_caption,
    HTMLTable_createTHead,
    HTMLTable_deleteTHead,
    HTMLTable_createTFoot,
    HTMLTable_deleteTFoot,
    HTMLTable_createCaption,
    HTMLTable_deleteCaption,
    HTMLTable_insertRow,
    HTMLTable_deleteRow,
    HTMLTable_get_readyState,
    HTMLTable_put_onreadystatechange,
    HTMLTable_get_onreadystatechange
};

/* IHTMLTable2 */
static HRESULT WINAPI HTMLTable2_QueryInterface(IHTMLTable2 *iface,
                                                         REFIID riid, void **ppv)
{
    HTMLTable *This = impl_from_IHTMLTable2(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLTable2_AddRef(IHTMLTable2 *iface)
{
    HTMLTable *This = impl_from_IHTMLTable2(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLTable2_Release(IHTMLTable2 *iface)
{
    HTMLTable *This = impl_from_IHTMLTable2(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLTable2_GetTypeInfoCount(IHTMLTable2 *iface, UINT *pctinfo)
{
    HTMLTable *This = impl_from_IHTMLTable2(iface);
    return IDispatchEx_GetTypeInfoCount(&This->element.node.event_target.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLTable2_GetTypeInfo(IHTMLTable2 *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLTable *This = impl_from_IHTMLTable2(iface);
    return IDispatchEx_GetTypeInfo(&This->element.node.event_target.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLTable2_GetIDsOfNames(IHTMLTable2 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLTable *This = impl_from_IHTMLTable2(iface);
    return IDispatchEx_GetIDsOfNames(&This->element.node.event_target.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLTable2_Invoke(IHTMLTable2 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLTable *This = impl_from_IHTMLTable2(iface);
    return IDispatchEx_Invoke(&This->element.node.event_target.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLTable2_firstPage(IHTMLTable2 *iface)
{
    HTMLTable *This = impl_from_IHTMLTable2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable2_lastPage(IHTMLTable2 *iface)
{
    HTMLTable *This = impl_from_IHTMLTable2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable2_cells(IHTMLTable2 *iface, IHTMLElementCollection **p)
{
    HTMLTable *This = impl_from_IHTMLTable2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable2_moveRow(IHTMLTable2 *iface, LONG indexFrom, LONG indexTo, IDispatch **row)
{
    HTMLTable *This = impl_from_IHTMLTable2(iface);
    FIXME("(%p)->(%d %d %p)\n", This, indexFrom, indexTo, row);
    return E_NOTIMPL;
}


static const IHTMLTable2Vtbl HTMLTable2Vtbl = {
    HTMLTable2_QueryInterface,
    HTMLTable2_AddRef,
    HTMLTable2_Release,
    HTMLTable2_GetTypeInfoCount,
    HTMLTable2_GetTypeInfo,
    HTMLTable2_GetIDsOfNames,
    HTMLTable2_Invoke,
    HTMLTable2_firstPage,
    HTMLTable2_lastPage,
    HTMLTable2_cells,
    HTMLTable2_moveRow
};

/* IHTMLTable3 */
static HRESULT WINAPI HTMLTable3_QueryInterface(IHTMLTable3 *iface,
                                                         REFIID riid, void **ppv)
{
    HTMLTable *This = impl_from_IHTMLTable3(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLTable3_AddRef(IHTMLTable3 *iface)
{
    HTMLTable *This = impl_from_IHTMLTable3(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLTable3_Release(IHTMLTable3 *iface)
{
    HTMLTable *This = impl_from_IHTMLTable3(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLTable3_GetTypeInfoCount(IHTMLTable3 *iface, UINT *pctinfo)
{
    HTMLTable *This = impl_from_IHTMLTable3(iface);
    return IDispatchEx_GetTypeInfoCount(&This->element.node.event_target.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLTable3_GetTypeInfo(IHTMLTable3 *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLTable *This = impl_from_IHTMLTable3(iface);
    return IDispatchEx_GetTypeInfo(&This->element.node.event_target.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLTable3_GetIDsOfNames(IHTMLTable3 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLTable *This = impl_from_IHTMLTable3(iface);
    return IDispatchEx_GetIDsOfNames(&This->element.node.event_target.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLTable3_Invoke(IHTMLTable3 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLTable *This = impl_from_IHTMLTable3(iface);
    return IDispatchEx_Invoke(&This->element.node.event_target.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLTable3_put_summary(IHTMLTable3 *iface, BSTR v)
{
    HTMLTable *This = impl_from_IHTMLTable3(iface);
    nsAString str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&str, v);

    nsres = nsIDOMHTMLTableElement_SetSummary(This->nstable, &str);

    nsAString_Finish(&str);
    if (NS_FAILED(nsres)) {
        ERR("Set summary(%s) failed: %08x\n", debugstr_w(v), nsres);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLTable3_get_summary(IHTMLTable3 *iface, BSTR * p)
{
    HTMLTable *This = impl_from_IHTMLTable3(iface);
    nsAString str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&str, NULL);
    nsres = nsIDOMHTMLTableElement_GetSummary(This->nstable, &str);

    return return_nsstr(nsres, &str, p);
}

static const IHTMLTable3Vtbl HTMLTable3Vtbl = {
    HTMLTable3_QueryInterface,
    HTMLTable3_AddRef,
    HTMLTable3_Release,
    HTMLTable3_GetTypeInfoCount,
    HTMLTable3_GetTypeInfo,
    HTMLTable3_GetIDsOfNames,
    HTMLTable3_Invoke,
    HTMLTable3_put_summary,
    HTMLTable3_get_summary
};

static inline HTMLTable *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLTable, element.node);
}

static HRESULT HTMLTable_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLTable *This = impl_from_HTMLDOMNode(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IHTMLTable_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IHTMLTable_iface;
    }else if(IsEqualGUID(&IID_IHTMLTable, riid)) {
        TRACE("(%p)->(IID_IHTMLTable %p)\n", This, ppv);
        *ppv = &This->IHTMLTable_iface;
    }else if(IsEqualGUID(&IID_IHTMLTable2, riid)) {
        TRACE("(%p)->(IID_IHTMLTable2 %p)\n", This, ppv);
        *ppv = &This->IHTMLTable2_iface;
    }else if(IsEqualGUID(&IID_IHTMLTable3, riid)) {
        TRACE("(%p)->(IID_IHTMLTable3 %p)\n", This, ppv);
        *ppv = &This->IHTMLTable3_iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    return HTMLElement_QI(&This->element.node, riid, ppv);
}

static void HTMLTable_traverse(HTMLDOMNode *iface, nsCycleCollectionTraversalCallback *cb)
{
    HTMLTable *This = impl_from_HTMLDOMNode(iface);

    if(This->nstable)
        note_cc_edge((nsISupports*)This->nstable, "This->nstable", cb);
}

static void HTMLTable_unlink(HTMLDOMNode *iface)
{
    HTMLTable *This = impl_from_HTMLDOMNode(iface);

    if(This->nstable) {
        nsIDOMHTMLTableElement *nstable = This->nstable;

        This->nstable = NULL;
        nsIDOMHTMLTableElement_Release(nstable);
    }
}

static const cpc_entry_t HTMLTable_cpc[] = {
    {&DIID_HTMLTableEvents},
    HTMLELEMENT_CPC,
    {NULL}
};

static const NodeImplVtbl HTMLTableImplVtbl = {
    &CLSID_HTMLTable,
    HTMLTable_QI,
    HTMLElement_destructor,
    HTMLTable_cpc,
    HTMLElement_clone,
    HTMLElement_handle_event,
    HTMLElement_get_attr_col,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    HTMLTable_traverse,
    HTMLTable_unlink
};

static const tid_t HTMLTable_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLTable_tid,
    IHTMLTable2_tid,
    IHTMLTable3_tid,
    0
};

static dispex_static_data_t HTMLTable_dispex = {
    NULL,
    DispHTMLTable_tid,
    HTMLTable_iface_tids,
    HTMLElement_init_dispex_info
};

HRESULT HTMLTable_Create(HTMLDocumentNode *doc, nsIDOMElement *nselem, HTMLElement **elem)
{
    HTMLTable *ret;
    nsresult nsres;

    ret = heap_alloc_zero(sizeof(HTMLTable));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->element.node.vtbl = &HTMLTableImplVtbl;
    ret->IHTMLTable_iface.lpVtbl = &HTMLTableVtbl;
    ret->IHTMLTable2_iface.lpVtbl = &HTMLTable2Vtbl;
    ret->IHTMLTable3_iface.lpVtbl = &HTMLTable3Vtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLTable_dispex);

    nsres = nsIDOMElement_QueryInterface(nselem, &IID_nsIDOMHTMLTableElement, (void**)&ret->nstable);
    assert(nsres == NS_OK);

    *elem = &ret->element;
    return S_OK;
}
