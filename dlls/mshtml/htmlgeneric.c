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

#include "wine/debug.h"

#include "mshtml_private.h"
#include "htmlevent.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

struct HTMLGenericElement {
    HTMLElement element;

    IHTMLGenericElement IHTMLGenericElement_iface;
};

static inline HTMLGenericElement *impl_from_IHTMLGenericElement(IHTMLGenericElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLGenericElement, IHTMLGenericElement_iface);
}

static HRESULT WINAPI HTMLGenericElement_QueryInterface(IHTMLGenericElement *iface, REFIID riid, void **ppv)
{
    HTMLGenericElement *This = impl_from_IHTMLGenericElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLGenericElement_AddRef(IHTMLGenericElement *iface)
{
    HTMLGenericElement *This = impl_from_IHTMLGenericElement(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLGenericElement_Release(IHTMLGenericElement *iface)
{
    HTMLGenericElement *This = impl_from_IHTMLGenericElement(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLGenericElement_GetTypeInfoCount(IHTMLGenericElement *iface, UINT *pctinfo)
{
    HTMLGenericElement *This = impl_from_IHTMLGenericElement(iface);
    return IDispatchEx_GetTypeInfoCount(&This->element.node.event_target.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLGenericElement_GetTypeInfo(IHTMLGenericElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLGenericElement *This = impl_from_IHTMLGenericElement(iface);
    return IDispatchEx_GetTypeInfo(&This->element.node.event_target.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLGenericElement_GetIDsOfNames(IHTMLGenericElement *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLGenericElement *This = impl_from_IHTMLGenericElement(iface);
    return IDispatchEx_GetIDsOfNames(&This->element.node.event_target.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLGenericElement_Invoke(IHTMLGenericElement *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLGenericElement *This = impl_from_IHTMLGenericElement(iface);
    return IDispatchEx_Invoke(&This->element.node.event_target.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLGenericElement_get_recordset(IHTMLGenericElement *iface, IDispatch **p)
{
    HTMLGenericElement *This = impl_from_IHTMLGenericElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLGenericElement_namedRecordset(IHTMLGenericElement *iface,
        BSTR dataMember, VARIANT *hierarchy, IDispatch **ppRecordset)
{
    HTMLGenericElement *This = impl_from_IHTMLGenericElement(iface);
    FIXME("(%p)->(%s %p %p)\n", This, debugstr_w(dataMember), hierarchy, ppRecordset);
    return E_NOTIMPL;
}

static const IHTMLGenericElementVtbl HTMLGenericElementVtbl = {
    HTMLGenericElement_QueryInterface,
    HTMLGenericElement_AddRef,
    HTMLGenericElement_Release,
    HTMLGenericElement_GetTypeInfoCount,
    HTMLGenericElement_GetTypeInfo,
    HTMLGenericElement_GetIDsOfNames,
    HTMLGenericElement_Invoke,
    HTMLGenericElement_get_recordset,
    HTMLGenericElement_namedRecordset
};

static inline HTMLGenericElement *impl_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLGenericElement, element.node.event_target.dispex);
}

static void *HTMLGenericElement_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLGenericElement *This = impl_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLGenericElement, riid))
        return &This->IHTMLGenericElement_iface;

    return HTMLElement_query_interface(&This->element.node.event_target.dispex, riid);
}

static const NodeImplVtbl HTMLGenericElementImplVtbl = {
    .clsid                 = &CLSID_HTMLGenericElement,
    .cpc_entries           = HTMLElement_cpc,
    .clone                 = HTMLElement_clone,
    .get_attr_col          = HTMLElement_get_attr_col
};

static const event_target_vtbl_t HTMLGenericElement_event_target_vtbl = {
    {
        HTMLELEMENT_DISPEX_VTBL_ENTRIES,
        .query_interface= HTMLGenericElement_query_interface,
        .destructor     = HTMLElement_destructor,
        .traverse       = HTMLElement_traverse,
        .unlink         = HTMLElement_unlink
    },
    HTMLELEMENT_EVENT_TARGET_VTBL_ENTRIES,
    .handle_event       = HTMLElement_handle_event
};

static const tid_t HTMLGenericElement_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLGenericElement_tid,
    0
};

static dispex_static_data_t HTMLGenericElement_dispex = {
    "HTMLUnknownElement",
    &HTMLGenericElement_event_target_vtbl.dispex_vtbl,
    DispHTMLGenericElement_tid,
    HTMLGenericElement_iface_tids,
    HTMLElement_init_dispex_info
};

HRESULT HTMLGenericElement_Create(HTMLDocumentNode *doc, nsIDOMElement *nselem, HTMLElement **elem)
{
    HTMLGenericElement *ret;

    ret = calloc(1, sizeof(HTMLGenericElement));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLGenericElement_iface.lpVtbl = &HTMLGenericElementVtbl;
    ret->element.node.vtbl = &HTMLGenericElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLGenericElement_dispex);

    *elem = &ret->element;
    return S_OK;
}
