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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

struct HTMLStyleSheet {
    DispatchEx dispex;
    IHTMLStyleSheet IHTMLStyleSheet_iface;
    IHTMLStyleSheet4 IHTMLStyleSheet4_iface;

    LONG ref;

    nsIDOMCSSStyleSheet *nsstylesheet;
};

struct HTMLStyleSheetsCollection {
    DispatchEx dispex;
    IHTMLStyleSheetsCollection IHTMLStyleSheetsCollection_iface;

    LONG ref;

    nsIDOMStyleSheetList *nslist;
};

struct HTMLStyleSheetRulesCollection {
    DispatchEx dispex;
    IHTMLStyleSheetRulesCollection IHTMLStyleSheetRulesCollection_iface;

    LONG ref;

    nsIDOMCSSRuleList *nslist;
};

struct HTMLStyleSheetRule {
    DispatchEx dispex;
    IHTMLStyleSheetRule IHTMLStyleSheetRule_iface;

    LONG ref;

    nsIDOMCSSRule *nsstylesheetrule;
};

static inline HTMLStyleSheetRule *impl_from_IHTMLStyleSheetRule(IHTMLStyleSheetRule *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyleSheetRule, IHTMLStyleSheetRule_iface);
}

static HRESULT WINAPI HTMLStyleSheetRule_QueryInterface(IHTMLStyleSheetRule *iface,
        REFIID riid, void **ppv)
{
    HTMLStyleSheetRule *This = impl_from_IHTMLStyleSheetRule(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if (IsEqualGUID(&IID_IUnknown, riid))
        *ppv = &This->IHTMLStyleSheetRule_iface;
    else if (IsEqualGUID(&IID_IHTMLStyleSheetRule, riid))
        *ppv = &This->IHTMLStyleSheetRule_iface;
    else if (dispex_query_interface(&This->dispex, riid, ppv))
        return *ppv ? S_OK : E_NOINTERFACE;
    else
    {
        *ppv = NULL;
        FIXME("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLStyleSheetRule_AddRef(IHTMLStyleSheetRule *iface)
{
    HTMLStyleSheetRule *This = impl_from_IHTMLStyleSheetRule(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLStyleSheetRule_Release(IHTMLStyleSheetRule *iface)
{
    HTMLStyleSheetRule *This = impl_from_IHTMLStyleSheetRule(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        release_dispex(&This->dispex);
        if(This->nsstylesheetrule)
            nsIDOMCSSRule_Release(This->nsstylesheetrule);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLStyleSheetRule_GetTypeInfoCount(
        IHTMLStyleSheetRule *iface, UINT *pctinfo)
{
    HTMLStyleSheetRule *This = impl_from_IHTMLStyleSheetRule(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLStyleSheetRule_GetTypeInfo(IHTMLStyleSheetRule *iface,
        UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLStyleSheetRule *This = impl_from_IHTMLStyleSheetRule(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLStyleSheetRule_GetIDsOfNames(IHTMLStyleSheetRule *iface,
        REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLStyleSheetRule *This = impl_from_IHTMLStyleSheetRule(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLStyleSheetRule_Invoke(IHTMLStyleSheetRule *iface,
        DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLStyleSheetRule *This = impl_from_IHTMLStyleSheetRule(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLStyleSheetRule_put_selectorText(IHTMLStyleSheetRule *iface, BSTR v)
{
    HTMLStyleSheetRule *This = impl_from_IHTMLStyleSheetRule(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheetRule_get_selectorText(IHTMLStyleSheetRule *iface, BSTR *p)
{
    HTMLStyleSheetRule *This = impl_from_IHTMLStyleSheetRule(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheetRule_get_style(IHTMLStyleSheetRule *iface, IHTMLRuleStyle **p)
{
    HTMLStyleSheetRule *This = impl_from_IHTMLStyleSheetRule(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheetRule_get_readOnly(IHTMLStyleSheetRule *iface, VARIANT_BOOL *p)
{
    HTMLStyleSheetRule *This = impl_from_IHTMLStyleSheetRule(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLStyleSheetRuleVtbl HTMLStyleSheetRuleVtbl = {
    HTMLStyleSheetRule_QueryInterface,
    HTMLStyleSheetRule_AddRef,
    HTMLStyleSheetRule_Release,
    HTMLStyleSheetRule_GetTypeInfoCount,
    HTMLStyleSheetRule_GetTypeInfo,
    HTMLStyleSheetRule_GetIDsOfNames,
    HTMLStyleSheetRule_Invoke,
    HTMLStyleSheetRule_put_selectorText,
    HTMLStyleSheetRule_get_selectorText,
    HTMLStyleSheetRule_get_style,
    HTMLStyleSheetRule_get_readOnly
};

static const tid_t HTMLStyleSheetRule_iface_tids[] = {
    IHTMLStyleSheetRule_tid,
    0
};
static dispex_static_data_t HTMLStyleSheetRule_dispex = {
    NULL,
    DispHTMLStyleSheetRule_tid,
    HTMLStyleSheetRule_iface_tids
};

static HRESULT create_style_sheet_rule(nsIDOMCSSRule *nsstylesheetrule, compat_mode_t compat_mode,
                                       IHTMLStyleSheetRule **ret)
{
    HTMLStyleSheetRule *rule;
    nsresult nsres;

    if(!(rule = heap_alloc(sizeof(*rule))))
        return E_OUTOFMEMORY;

    rule->IHTMLStyleSheetRule_iface.lpVtbl = &HTMLStyleSheetRuleVtbl;
    rule->ref = 1;
    rule->nsstylesheetrule = NULL;

    init_dispatch(&rule->dispex, (IUnknown *)&rule->IHTMLStyleSheetRule_iface, &HTMLStyleSheetRule_dispex,
                  compat_mode);

    if (nsstylesheetrule)
    {
        nsres = nsIDOMCSSRule_QueryInterface(nsstylesheetrule, &IID_nsIDOMCSSRule,
                (void **)&rule->nsstylesheetrule);
        if (NS_FAILED(nsres))
            ERR("Could not get nsIDOMCSSRule interface: %08x\n", nsres);
    }

    *ret = &rule->IHTMLStyleSheetRule_iface;
    return S_OK;
}

static inline HTMLStyleSheetRulesCollection *impl_from_IHTMLStyleSheetRulesCollection(IHTMLStyleSheetRulesCollection *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyleSheetRulesCollection, IHTMLStyleSheetRulesCollection_iface);
}

static HRESULT WINAPI HTMLStyleSheetRulesCollection_QueryInterface(IHTMLStyleSheetRulesCollection *iface,
        REFIID riid, void **ppv)
{
    HTMLStyleSheetRulesCollection *This = impl_from_IHTMLStyleSheetRulesCollection(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLStyleSheetRulesCollection_iface;
    }else if(IsEqualGUID(&IID_IHTMLStyleSheetRulesCollection, riid)) {
        *ppv = &This->IHTMLStyleSheetRulesCollection_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        *ppv = NULL;
        FIXME("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLStyleSheetRulesCollection_AddRef(IHTMLStyleSheetRulesCollection *iface)
{
    HTMLStyleSheetRulesCollection *This = impl_from_IHTMLStyleSheetRulesCollection(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLStyleSheetRulesCollection_Release(IHTMLStyleSheetRulesCollection *iface)
{
    HTMLStyleSheetRulesCollection *This = impl_from_IHTMLStyleSheetRulesCollection(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        release_dispex(&This->dispex);
        if(This->nslist)
            nsIDOMCSSRuleList_Release(This->nslist);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLStyleSheetRulesCollection_GetTypeInfoCount(
        IHTMLStyleSheetRulesCollection *iface, UINT *pctinfo)
{
    HTMLStyleSheetRulesCollection *This = impl_from_IHTMLStyleSheetRulesCollection(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLStyleSheetRulesCollection_GetTypeInfo(IHTMLStyleSheetRulesCollection *iface,
        UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLStyleSheetRulesCollection *This = impl_from_IHTMLStyleSheetRulesCollection(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLStyleSheetRulesCollection_GetIDsOfNames(IHTMLStyleSheetRulesCollection *iface,
        REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLStyleSheetRulesCollection *This = impl_from_IHTMLStyleSheetRulesCollection(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLStyleSheetRulesCollection_Invoke(IHTMLStyleSheetRulesCollection *iface,
        DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLStyleSheetRulesCollection *This = impl_from_IHTMLStyleSheetRulesCollection(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLStyleSheetRulesCollection_get_length(IHTMLStyleSheetRulesCollection *iface,
        LONG *p)
{
    HTMLStyleSheetRulesCollection *This = impl_from_IHTMLStyleSheetRulesCollection(iface);
    UINT32 len = 0;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->nslist) {
        nsresult nsres;

        nsres = nsIDOMCSSRuleList_GetLength(This->nslist, &len);
        if(NS_FAILED(nsres))
            ERR("GetLength failed: %08x\n", nsres);
    }

    *p = len;
    return S_OK;
}

static HRESULT WINAPI HTMLStyleSheetRulesCollection_item(IHTMLStyleSheetRulesCollection *iface,
        LONG index, IHTMLStyleSheetRule **p)
{
    HTMLStyleSheetRulesCollection *This = impl_from_IHTMLStyleSheetRulesCollection(iface);
    nsIDOMCSSRule *nsstylesheetrule;
    nsresult nsres;

    TRACE("(%p)->(%d %p)\n", This, index, p);

    nsres = nsIDOMCSSRuleList_Item(This->nslist, index, &nsstylesheetrule);
    if(NS_FAILED(nsres))
        return map_nsresult(nsres);
    if(!nsstylesheetrule)
        return E_INVALIDARG;

    return create_style_sheet_rule(nsstylesheetrule, dispex_compat_mode(&This->dispex), p);
}

static const IHTMLStyleSheetRulesCollectionVtbl HTMLStyleSheetRulesCollectionVtbl = {
    HTMLStyleSheetRulesCollection_QueryInterface,
    HTMLStyleSheetRulesCollection_AddRef,
    HTMLStyleSheetRulesCollection_Release,
    HTMLStyleSheetRulesCollection_GetTypeInfoCount,
    HTMLStyleSheetRulesCollection_GetTypeInfo,
    HTMLStyleSheetRulesCollection_GetIDsOfNames,
    HTMLStyleSheetRulesCollection_Invoke,
    HTMLStyleSheetRulesCollection_get_length,
    HTMLStyleSheetRulesCollection_item
};

static const tid_t HTMLStyleSheetRulesCollection_iface_tids[] = {
    IHTMLStyleSheetRulesCollection_tid,
    0
};
static dispex_static_data_t HTMLStyleSheetRulesCollection_dispex = {
    NULL,
    DispHTMLStyleSheetRulesCollection_tid,
    HTMLStyleSheetRulesCollection_iface_tids
};

static HRESULT create_style_sheet_rules_collection(nsIDOMCSSRuleList *nslist, compat_mode_t compat_mode,
                                                   IHTMLStyleSheetRulesCollection **ret)
{
    HTMLStyleSheetRulesCollection *collection;

    if(!(collection = heap_alloc(sizeof(*collection))))
        return E_OUTOFMEMORY;

    collection->IHTMLStyleSheetRulesCollection_iface.lpVtbl = &HTMLStyleSheetRulesCollectionVtbl;
    collection->ref = 1;
    collection->nslist = nslist;

    init_dispatch(&collection->dispex, (IUnknown*)&collection->IHTMLStyleSheetRulesCollection_iface,
                  &HTMLStyleSheetRulesCollection_dispex, compat_mode);

    if(nslist)
        nsIDOMCSSRuleList_AddRef(nslist);

    *ret = &collection->IHTMLStyleSheetRulesCollection_iface;
    return S_OK;
}

static inline HTMLStyleSheetsCollection *impl_from_IHTMLStyleSheetsCollection(IHTMLStyleSheetsCollection *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyleSheetsCollection, IHTMLStyleSheetsCollection_iface);
}

static HRESULT WINAPI HTMLStyleSheetsCollection_QueryInterface(IHTMLStyleSheetsCollection *iface,
         REFIID riid, void **ppv)
{
    HTMLStyleSheetsCollection *This = impl_from_IHTMLStyleSheetsCollection(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLStyleSheetsCollection_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        *ppv = &This->IHTMLStyleSheetsCollection_iface;
    }else if(IsEqualGUID(&IID_IHTMLStyleSheetsCollection, riid)) {
        *ppv = &This->IHTMLStyleSheetsCollection_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        *ppv = NULL;
        WARN("unsupported %s\n", debugstr_mshtml_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLStyleSheetsCollection_AddRef(IHTMLStyleSheetsCollection *iface)
{
    HTMLStyleSheetsCollection *This = impl_from_IHTMLStyleSheetsCollection(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLStyleSheetsCollection_Release(IHTMLStyleSheetsCollection *iface)
{
    HTMLStyleSheetsCollection *This = impl_from_IHTMLStyleSheetsCollection(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        release_dispex(&This->dispex);
        if(This->nslist)
            nsIDOMStyleSheetList_Release(This->nslist);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLStyleSheetsCollection_GetTypeInfoCount(IHTMLStyleSheetsCollection *iface,
        UINT *pctinfo)
{
    HTMLStyleSheetsCollection *This = impl_from_IHTMLStyleSheetsCollection(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLStyleSheetsCollection_GetTypeInfo(IHTMLStyleSheetsCollection *iface,
        UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLStyleSheetsCollection *This = impl_from_IHTMLStyleSheetsCollection(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLStyleSheetsCollection_GetIDsOfNames(IHTMLStyleSheetsCollection *iface,
        REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLStyleSheetsCollection *This = impl_from_IHTMLStyleSheetsCollection(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLStyleSheetsCollection_Invoke(IHTMLStyleSheetsCollection *iface,
        DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLStyleSheetsCollection *This = impl_from_IHTMLStyleSheetsCollection(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLStyleSheetsCollection_get_length(IHTMLStyleSheetsCollection *iface,
        LONG *p)
{
    HTMLStyleSheetsCollection *This = impl_from_IHTMLStyleSheetsCollection(iface);
    UINT32 len = 0;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->nslist)
        nsIDOMStyleSheetList_GetLength(This->nslist, &len);

    *p = len;
    return S_OK;
}

static HRESULT WINAPI HTMLStyleSheetsCollection_get__newEnum(IHTMLStyleSheetsCollection *iface,
        IUnknown **p)
{
    HTMLStyleSheetsCollection *This = impl_from_IHTMLStyleSheetsCollection(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheetsCollection_item(IHTMLStyleSheetsCollection *iface,
        VARIANT *pvarIndex, VARIANT *pvarResult)
{
    HTMLStyleSheetsCollection *This = impl_from_IHTMLStyleSheetsCollection(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_variant(pvarIndex), pvarResult);

    switch(V_VT(pvarIndex)) {
    case VT_I4: {
        nsIDOMStyleSheet *nsstylesheet;
        IHTMLStyleSheet *stylesheet;
        nsresult nsres;
        HRESULT hres;

        TRACE("index=%d\n", V_I4(pvarIndex));

        nsres = nsIDOMStyleSheetList_Item(This->nslist, V_I4(pvarIndex), &nsstylesheet);
        if(NS_FAILED(nsres) || !nsstylesheet) {
            WARN("Item failed: %08x\n", nsres);
            V_VT(pvarResult) = VT_EMPTY;
            return E_INVALIDARG;
        }

        hres = create_style_sheet(nsstylesheet, dispex_compat_mode(&This->dispex), &stylesheet);
        if(FAILED(hres))
            return hres;

        V_VT(pvarResult) = VT_DISPATCH;
        V_DISPATCH(pvarResult) = (IDispatch*)stylesheet;
        return S_OK;
    }

    case VT_BSTR:
        FIXME("id=%s not implemented\n", debugstr_w(V_BSTR(pvarResult)));
        return E_NOTIMPL;

    default:
        WARN("Invalid index %s\n", debugstr_variant(pvarIndex));
    }

    return E_INVALIDARG;
}

static const IHTMLStyleSheetsCollectionVtbl HTMLStyleSheetsCollectionVtbl = {
    HTMLStyleSheetsCollection_QueryInterface,
    HTMLStyleSheetsCollection_AddRef,
    HTMLStyleSheetsCollection_Release,
    HTMLStyleSheetsCollection_GetTypeInfoCount,
    HTMLStyleSheetsCollection_GetTypeInfo,
    HTMLStyleSheetsCollection_GetIDsOfNames,
    HTMLStyleSheetsCollection_Invoke,
    HTMLStyleSheetsCollection_get_length,
    HTMLStyleSheetsCollection_get__newEnum,
    HTMLStyleSheetsCollection_item
};

static const tid_t HTMLStyleSheetsCollection_iface_tids[] = {
    IHTMLStyleSheetsCollection_tid,
    0
};
static dispex_static_data_t HTMLStyleSheetsCollection_dispex = {
    NULL,
    DispHTMLStyleSheetsCollection_tid,
    HTMLStyleSheetsCollection_iface_tids
};

HRESULT create_style_sheet_collection(nsIDOMStyleSheetList *nslist, compat_mode_t compat_mode,
                                      IHTMLStyleSheetsCollection **ret)
{
    HTMLStyleSheetsCollection *collection;

    if(!(collection = heap_alloc(sizeof(HTMLStyleSheetsCollection))))
        return E_OUTOFMEMORY;

    collection->IHTMLStyleSheetsCollection_iface.lpVtbl = &HTMLStyleSheetsCollectionVtbl;
    collection->ref = 1;

    if(nslist)
        nsIDOMStyleSheetList_AddRef(nslist);
    collection->nslist = nslist;

    init_dispatch(&collection->dispex, (IUnknown*)&collection->IHTMLStyleSheetsCollection_iface,
                  &HTMLStyleSheetsCollection_dispex, compat_mode);

    *ret = &collection->IHTMLStyleSheetsCollection_iface;
    return S_OK;
}

static inline HTMLStyleSheet *impl_from_IHTMLStyleSheet(IHTMLStyleSheet *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyleSheet, IHTMLStyleSheet_iface);
}

static HRESULT WINAPI HTMLStyleSheet_QueryInterface(IHTMLStyleSheet *iface, REFIID riid, void **ppv)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLStyleSheet_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        *ppv = &This->IHTMLStyleSheet_iface;
    }else if(IsEqualGUID(&IID_IHTMLStyleSheet, riid)) {
        *ppv = &This->IHTMLStyleSheet_iface;
    }else if(IsEqualGUID(&IID_IHTMLStyleSheet4, riid)) {
        *ppv = &This->IHTMLStyleSheet4_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        *ppv = NULL;
        WARN("unsupported %s\n", debugstr_mshtml_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLStyleSheet_AddRef(IHTMLStyleSheet *iface)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLStyleSheet_Release(IHTMLStyleSheet *iface)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        release_dispex(&This->dispex);
        if(This->nsstylesheet)
            nsIDOMCSSStyleSheet_Release(This->nsstylesheet);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLStyleSheet_GetTypeInfoCount(IHTMLStyleSheet *iface, UINT *pctinfo)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    TRACE("(%p)->(%p)\n", This, pctinfo);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLStyleSheet_GetTypeInfo(IHTMLStyleSheet *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLStyleSheet_GetIDsOfNames(IHTMLStyleSheet *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLStyleSheet_Invoke(IHTMLStyleSheet *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, wFlags, pDispParams,
            pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLStyleSheet_put_title(IHTMLStyleSheet *iface, BSTR v)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_title(IHTMLStyleSheet *iface, BSTR *p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_parentStyleSheet(IHTMLStyleSheet *iface,
                                                          IHTMLStyleSheet **p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_owningElement(IHTMLStyleSheet *iface, IHTMLElement **p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_put_disabled(IHTMLStyleSheet *iface, VARIANT_BOOL v)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_disabled(IHTMLStyleSheet *iface, VARIANT_BOOL *p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_readOnly(IHTMLStyleSheet *iface, VARIANT_BOOL *p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_imports(IHTMLStyleSheet *iface,
                                                 IHTMLStyleSheetsCollection **p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_put_href(IHTMLStyleSheet *iface, BSTR v)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_href(IHTMLStyleSheet *iface, BSTR *p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    nsAString href_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&href_str, NULL);
    nsres = nsIDOMCSSStyleSheet_GetHref(This->nsstylesheet, &href_str);
    return return_nsstr(nsres, &href_str, p);
}

static HRESULT WINAPI HTMLStyleSheet_get_type(IHTMLStyleSheet *iface, BSTR *p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_id(IHTMLStyleSheet *iface, BSTR *p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_addImport(IHTMLStyleSheet *iface, BSTR bstrURL,
                                               LONG lIndex, LONG *plIndex)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%s %d %p)\n", This, debugstr_w(bstrURL), lIndex, plIndex);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_addRule(IHTMLStyleSheet *iface, BSTR bstrSelector,
                                             BSTR bstrStyle, LONG lIndex, LONG *plIndex)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%s %s %d %p)\n", This, debugstr_w(bstrSelector), debugstr_w(bstrStyle),
          lIndex, plIndex);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_removeImport(IHTMLStyleSheet *iface, LONG lIndex)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%d)\n", This, lIndex);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_removeRule(IHTMLStyleSheet *iface, LONG lIndex)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%d)\n", This, lIndex);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_put_media(IHTMLStyleSheet *iface, BSTR v)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_media(IHTMLStyleSheet *iface, BSTR *p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_put_cssText(IHTMLStyleSheet *iface, BSTR v)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    do {
        nsres = nsIDOMCSSStyleSheet_DeleteRule(This->nsstylesheet, 0);
    }while(NS_SUCCEEDED(nsres));

    if(v && *v) {
        nsAString nsstr;
        UINT32 idx;

        /* FIXME: This won't work for multiple rules in the string. */
        nsAString_InitDepend(&nsstr, v);
        nsres = nsIDOMCSSStyleSheet_InsertRule(This->nsstylesheet, &nsstr, 0, &idx);
        nsAString_Finish(&nsstr);
        if(NS_FAILED(nsres)) {
            FIXME("InsertRule failed for string %s. Probably multiple rules passed.\n", debugstr_w(v));
            return E_FAIL;
        }
    }

    return S_OK;
}

static HRESULT WINAPI HTMLStyleSheet_get_cssText(IHTMLStyleSheet *iface, BSTR *p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    nsIDOMCSSRuleList *nslist = NULL;
    nsIDOMCSSRule *nsrule;
    nsAString nsstr;
    UINT32 len;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMCSSStyleSheet_GetCssRules(This->nsstylesheet, &nslist);
    if(NS_FAILED(nsres)) {
        ERR("GetCssRules failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMCSSRuleList_GetLength(nslist, &len);
    assert(nsres == NS_OK);

    if(len) {
        nsres = nsIDOMCSSRuleList_Item(nslist, 0, &nsrule);
        if(NS_FAILED(nsres))
            ERR("Item failed: %08x\n", nsres);
    }

    nsIDOMCSSRuleList_Release(nslist);
    if(NS_FAILED(nsres))
        return E_FAIL;

    if(!len) {
        *p = NULL;
        return S_OK;
    }

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMCSSRule_GetCssText(nsrule, &nsstr);
    nsIDOMCSSRule_Release(nsrule);
    return return_nsstr(nsres, &nsstr, p);
}

static HRESULT WINAPI HTMLStyleSheet_get_rules(IHTMLStyleSheet *iface,
                                               IHTMLStyleSheetRulesCollection **p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    nsIDOMCSSRuleList *nslist = NULL;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMCSSStyleSheet_GetCssRules(This->nsstylesheet, &nslist);
    if(NS_FAILED(nsres)) {
        ERR("GetCssRules failed: %08x\n", nsres);
        return E_FAIL;
    }

    hres = create_style_sheet_rules_collection(nslist, dispex_compat_mode(&This->dispex), p);
    nsIDOMCSSRuleList_Release(nslist);
    return hres;
}

static const IHTMLStyleSheetVtbl HTMLStyleSheetVtbl = {
    HTMLStyleSheet_QueryInterface,
    HTMLStyleSheet_AddRef,
    HTMLStyleSheet_Release,
    HTMLStyleSheet_GetTypeInfoCount,
    HTMLStyleSheet_GetTypeInfo,
    HTMLStyleSheet_GetIDsOfNames,
    HTMLStyleSheet_Invoke,
    HTMLStyleSheet_put_title,
    HTMLStyleSheet_get_title,
    HTMLStyleSheet_get_parentStyleSheet,
    HTMLStyleSheet_get_owningElement,
    HTMLStyleSheet_put_disabled,
    HTMLStyleSheet_get_disabled,
    HTMLStyleSheet_get_readOnly,
    HTMLStyleSheet_get_imports,
    HTMLStyleSheet_put_href,
    HTMLStyleSheet_get_href,
    HTMLStyleSheet_get_type,
    HTMLStyleSheet_get_id,
    HTMLStyleSheet_addImport,
    HTMLStyleSheet_addRule,
    HTMLStyleSheet_removeImport,
    HTMLStyleSheet_removeRule,
    HTMLStyleSheet_put_media,
    HTMLStyleSheet_get_media,
    HTMLStyleSheet_put_cssText,
    HTMLStyleSheet_get_cssText,
    HTMLStyleSheet_get_rules
};

static inline HTMLStyleSheet *impl_from_IHTMLStyleSheet4(IHTMLStyleSheet4 *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyleSheet, IHTMLStyleSheet4_iface);
}

static HRESULT WINAPI HTMLStyleSheet4_QueryInterface(IHTMLStyleSheet4 *iface, REFIID riid, void **ppv)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet4(iface);
    return IHTMLStyleSheet_QueryInterface(&This->IHTMLStyleSheet_iface, riid, ppv);
}

static ULONG WINAPI HTMLStyleSheet4_AddRef(IHTMLStyleSheet4 *iface)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet4(iface);
    return IHTMLStyleSheet_AddRef(&This->IHTMLStyleSheet_iface);
}

static ULONG WINAPI HTMLStyleSheet4_Release(IHTMLStyleSheet4 *iface)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet4(iface);
    return IHTMLStyleSheet_Release(&This->IHTMLStyleSheet_iface);
}

static HRESULT WINAPI HTMLStyleSheet4_GetTypeInfoCount(IHTMLStyleSheet4 *iface, UINT *pctinfo)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet4(iface);
    TRACE("(%p)->(%p)\n", This, pctinfo);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLStyleSheet4_GetTypeInfo(IHTMLStyleSheet4 *iface, UINT iTInfo,
                                                  LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet4(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLStyleSheet4_GetIDsOfNames(IHTMLStyleSheet4 *iface, REFIID riid,
                                                    LPOLESTR *rgszNames, UINT cNames,
                                                    LCID lcid, DISPID *rgDispId)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet4(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLStyleSheet4_Invoke(IHTMLStyleSheet4 *iface, DISPID dispIdMember,
                                             REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                                             VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet4(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, wFlags, pDispParams,
                              pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLStyleSheet4_get_type(IHTMLStyleSheet4 *iface, BSTR *p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet4(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return IHTMLStyleSheet_get_type(&This->IHTMLStyleSheet_iface, p);
}

static HRESULT WINAPI HTMLStyleSheet4_get_href(IHTMLStyleSheet4 *iface, VARIANT *p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet4(iface);
    nsAString href_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&href_str, NULL);
    nsres = nsIDOMCSSStyleSheet_GetHref(This->nsstylesheet, &href_str);
    return return_nsstr_variant(nsres, &href_str, 0, p);
}

static HRESULT WINAPI HTMLStyleSheet4_get_title(IHTMLStyleSheet4 *iface, BSTR *p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet4_get_ownerNode(IHTMLStyleSheet4 *iface, IHTMLElement **p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet4_get_ownerRule(IHTMLStyleSheet4 *iface, IHTMLCSSRule **p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet4_get_cssRules(IHTMLStyleSheet4 *iface, IHTMLStyleSheetRulesCollection **p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet4(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return IHTMLStyleSheet_get_rules(&This->IHTMLStyleSheet_iface, p);
}

static HRESULT WINAPI HTMLStyleSheet4_get_media(IHTMLStyleSheet4 *iface, VARIANT *p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet4_insertRule(IHTMLStyleSheet4 *iface, BSTR rule, LONG index, LONG *p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet4(iface);
    UINT32 new_index = 0;
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s %d %p)\n", This, debugstr_w(rule), index, p);

    nsAString_InitDepend(&nsstr, rule);
    nsres = nsIDOMCSSStyleSheet_InsertRule(This->nsstylesheet, &nsstr, index, &new_index);
    if(NS_FAILED(nsres)) WARN("failed: %08x\n", nsres);
    nsAString_Finish(&nsstr);
    *p = new_index;
    return map_nsresult(nsres);
}

static HRESULT WINAPI HTMLStyleSheet4_deleteRule(IHTMLStyleSheet4 *iface, LONG index)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet4(iface);
    FIXME("(%p)->(%d)\n", This, index);
    return E_NOTIMPL;
}

static const IHTMLStyleSheet4Vtbl HTMLStyleSheet4Vtbl = {
    HTMLStyleSheet4_QueryInterface,
    HTMLStyleSheet4_AddRef,
    HTMLStyleSheet4_Release,
    HTMLStyleSheet4_GetTypeInfoCount,
    HTMLStyleSheet4_GetTypeInfo,
    HTMLStyleSheet4_GetIDsOfNames,
    HTMLStyleSheet4_Invoke,
    HTMLStyleSheet4_get_type,
    HTMLStyleSheet4_get_href,
    HTMLStyleSheet4_get_title,
    HTMLStyleSheet4_get_ownerNode,
    HTMLStyleSheet4_get_ownerRule,
    HTMLStyleSheet4_get_cssRules,
    HTMLStyleSheet4_get_media,
    HTMLStyleSheet4_insertRule,
    HTMLStyleSheet4_deleteRule,
};

static void HTMLStyleSheet_init_dispex_info(dispex_data_t *info, compat_mode_t mode)
{
    if(mode >= COMPAT_MODE_IE9)
        dispex_info_add_interface(info, IHTMLStyleSheet4_tid, NULL);
}

static const tid_t HTMLStyleSheet_iface_tids[] = {
    IHTMLStyleSheet_tid,
    0
};
static dispex_static_data_t HTMLStyleSheet_dispex = {
    NULL,
    DispHTMLStyleSheet_tid,
    HTMLStyleSheet_iface_tids,
    HTMLStyleSheet_init_dispex_info
};

HRESULT create_style_sheet(nsIDOMStyleSheet *nsstylesheet, compat_mode_t compat_mode, IHTMLStyleSheet **ret)
{
    HTMLStyleSheet *style_sheet;
    nsresult nsres;

    if(!(style_sheet = heap_alloc(sizeof(HTMLStyleSheet))))
        return E_OUTOFMEMORY;

    style_sheet->IHTMLStyleSheet_iface.lpVtbl = &HTMLStyleSheetVtbl;
    style_sheet->IHTMLStyleSheet4_iface.lpVtbl = &HTMLStyleSheet4Vtbl;
    style_sheet->ref = 1;
    style_sheet->nsstylesheet = NULL;

    init_dispatch(&style_sheet->dispex, (IUnknown*)&style_sheet->IHTMLStyleSheet_iface,
                  &HTMLStyleSheet_dispex, compat_mode);

    if(nsstylesheet) {
        nsres = nsIDOMStyleSheet_QueryInterface(nsstylesheet, &IID_nsIDOMCSSStyleSheet,
                (void**)&style_sheet->nsstylesheet);
        if(NS_FAILED(nsres))
            ERR("Could not get nsICSSStyleSheet interface: %08x\n", nsres);
    }

    *ret = &style_sheet->IHTMLStyleSheet_iface;
    return S_OK;
}
