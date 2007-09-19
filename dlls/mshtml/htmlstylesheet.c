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
#include "winnls.h"
#include "ole2.h"

#include "wine/debug.h"
#include "wine/unicode.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct {
    const IHTMLStyleSheetVtbl *lpHTMLStyleSheetVtbl;
    LONG ref;
} HTMLStyleSheet;

typedef struct {
    const IHTMLStyleSheetsCollectionVtbl *lpHTMLStyleSheetsCollectionVtbl;

    LONG ref;

    nsIDOMStyleSheetList *nslist;
} HTMLStyleSheetsCollection;

#define HTMLSTYLESHEET(x)      ((IHTMLStyleSheet*)             &(x)->lpHTMLStyleSheetVtbl);
#define HTMLSTYLESHEETSCOL(x)  ((IHTMLStyleSheetsCollection*)  &(x)->lpHTMLStyleSheetsCollectionVtbl);

#define HTMLSTYLESHEETSCOL_THIS(iface) \
    DEFINE_THIS(HTMLStyleSheetsCollection, HTMLStyleSheetsCollection, iface)

static HRESULT WINAPI HTMLStyleSheetsCollection_QueryInterface(IHTMLStyleSheetsCollection *iface,
         REFIID riid, void **ppv)
{
    HTMLStyleSheetsCollection *This = HTMLSTYLESHEETSCOL_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLSTYLESHEETSCOL(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = HTMLSTYLESHEETSCOL(This);
    }else if(IsEqualGUID(&IID_IHTMLStyleSheetsCollection, riid)) {
        TRACE("(%p)->(IID_IHTMLStyleSheetsCollection %p)\n", This, ppv);
        *ppv = HTMLSTYLESHEETSCOL(This);
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("unsupported %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI HTMLStyleSheetsCollection_AddRef(IHTMLStyleSheetsCollection *iface)
{
    HTMLStyleSheetsCollection *This = HTMLSTYLESHEETSCOL_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLStyleSheetsCollection_Release(IHTMLStyleSheetsCollection *iface)
{
    HTMLStyleSheetsCollection *This = HTMLSTYLESHEETSCOL_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        mshtml_free(This);

    return ref;
}

static HRESULT WINAPI HTMLStyleSheetsCollection_GetTypeInfoCount(IHTMLStyleSheetsCollection *iface,
        UINT *pctinfo)
{
    HTMLStyleSheetsCollection *This = HTMLSTYLESHEETSCOL_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheetsCollection_GetTypeInfo(IHTMLStyleSheetsCollection *iface,
        UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLStyleSheetsCollection *This = HTMLSTYLESHEETSCOL_THIS(iface);
    FIXME("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheetsCollection_GetIDsOfNames(IHTMLStyleSheetsCollection *iface,
        REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLStyleSheetsCollection *This = HTMLSTYLESHEETSCOL_THIS(iface);
    FIXME("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheetsCollection_Invoke(IHTMLStyleSheetsCollection *iface,
        DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLStyleSheetsCollection *This = HTMLSTYLESHEETSCOL_THIS(iface);
    FIXME("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheetsCollection_get_length(IHTMLStyleSheetsCollection *iface,
        long *p)
{
    HTMLStyleSheetsCollection *This = HTMLSTYLESHEETSCOL_THIS(iface);
    PRUint32 len = 0;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->nslist)
        nsIDOMStyleSheetList_GetLength(This->nslist, &len);

    *p = len;
    return S_OK;
}

static HRESULT WINAPI HTMLStyleSheetsCollection_get__newEnum(IHTMLStyleSheetsCollection *iface,
        IUnknown **p)
{
    HTMLStyleSheetsCollection *This = HTMLSTYLESHEETSCOL_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheetsCollection_item(IHTMLStyleSheetsCollection *iface,
        VARIANT *pvarIndex, VARIANT *pvarResult)
{
    HTMLStyleSheetsCollection *This = HTMLSTYLESHEETSCOL_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pvarIndex, pvarResult);
    return E_NOTIMPL;
}

#undef HTMLSTYLESHEETSCOL_THIS

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

IHTMLStyleSheetsCollection *HTMLStyleSheetsCollection_Create(nsIDOMStyleSheetList *nslist)
{
    HTMLStyleSheetsCollection *ret = mshtml_alloc(sizeof(HTMLStyleSheetsCollection));

    ret->lpHTMLStyleSheetsCollectionVtbl = &HTMLStyleSheetsCollectionVtbl;
    ret->ref = 1;

    if(nslist)
        nsIDOMStyleSheetList_AddRef(nslist);
    ret->nslist = nslist;

    return HTMLSTYLESHEETSCOL(ret);
}

#define HTMLSTYLESHEET_THIS(iface) DEFINE_THIS(HTMLStyleSheet, HTMLStyleSheet, iface)

static HRESULT WINAPI HTMLStyleSheet_QueryInterface(IHTMLStyleSheet *iface, REFIID riid, void **ppv)
{
    HTMLStyleSheet *This = HTMLSTYLESHEET_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLSTYLESHEET(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = HTMLSTYLESHEET(This);
    }else if(IsEqualGUID(&IID_IHTMLStyleSheet, riid)) {
        TRACE("(%p)->(IID_IHTMLStyleSheet %p)\n", This, ppv);
        *ppv = HTMLSTYLESHEET(This);
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("unsupported %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI HTMLStyleSheet_AddRef(IHTMLStyleSheet *iface)
{
    HTMLStyleSheet *This = HTMLSTYLESHEET_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLStyleSheet_Release(IHTMLStyleSheet *iface)
{
    HTMLStyleSheet *This = HTMLSTYLESHEET_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        mshtml_free(This);

    return ref;
}

static HRESULT WINAPI HTMLStyleSheet_GetTypeInfoCount(IHTMLStyleSheet *iface, UINT *pctinfo)
{
    HTMLStyleSheet *This = HTMLSTYLESHEET_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_GetTypeInfo(IHTMLStyleSheet *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLStyleSheet *This = HTMLSTYLESHEET_THIS(iface);
    FIXME("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_GetIDsOfNames(IHTMLStyleSheet *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLStyleSheet *This = HTMLSTYLESHEET_THIS(iface);
    FIXME("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_Invoke(IHTMLStyleSheet *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLStyleSheet *This = HTMLSTYLESHEET_THIS(iface);
    FIXME("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_put_title(IHTMLStyleSheet *iface, BSTR v)
{
    HTMLStyleSheet *This = HTMLSTYLESHEET_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_title(IHTMLStyleSheet *iface, BSTR *p)
{
    HTMLStyleSheet *This = HTMLSTYLESHEET_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_parentStyleSheet(IHTMLStyleSheet *iface,
                                                          IHTMLStyleSheet **p)
{
    HTMLStyleSheet *This = HTMLSTYLESHEET_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_owningElement(IHTMLStyleSheet *iface, IHTMLElement **p)
{
    HTMLStyleSheet *This = HTMLSTYLESHEET_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_put_disabled(IHTMLStyleSheet *iface, VARIANT_BOOL v)
{
    HTMLStyleSheet *This = HTMLSTYLESHEET_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_disabled(IHTMLStyleSheet *iface, VARIANT_BOOL *p)
{
    HTMLStyleSheet *This = HTMLSTYLESHEET_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_readOnly(IHTMLStyleSheet *iface, VARIANT_BOOL *p)
{
    HTMLStyleSheet *This = HTMLSTYLESHEET_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_imports(IHTMLStyleSheet *iface,
                                                 IHTMLStyleSheetsCollection **p)
{
    HTMLStyleSheet *This = HTMLSTYLESHEET_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_put_href(IHTMLStyleSheet *iface, BSTR v)
{
    HTMLStyleSheet *This = HTMLSTYLESHEET_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_href(IHTMLStyleSheet *iface, BSTR *p)
{
    HTMLStyleSheet *This = HTMLSTYLESHEET_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_type(IHTMLStyleSheet *iface, BSTR *p)
{
    HTMLStyleSheet *This = HTMLSTYLESHEET_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_id(IHTMLStyleSheet *iface, BSTR *p)
{
    HTMLStyleSheet *This = HTMLSTYLESHEET_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_addImport(IHTMLStyleSheet *iface, BSTR bstrURL,
                                               long lIndex, long *plIndex)
{
    HTMLStyleSheet *This = HTMLSTYLESHEET_THIS(iface);
    FIXME("(%p)->(%s %ld %p)\n", This, debugstr_w(bstrURL), lIndex, plIndex);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_addRule(IHTMLStyleSheet *iface, BSTR bstrSelector,
                                             BSTR bstrStyle, long lIndex, long *plIndex)
{
    HTMLStyleSheet *This = HTMLSTYLESHEET_THIS(iface);
    FIXME("(%p)->(%s %s %ld %p)\n", This, debugstr_w(bstrSelector), debugstr_w(bstrStyle),
          lIndex, plIndex);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_removeImport(IHTMLStyleSheet *iface, long lIndex)
{
    HTMLStyleSheet *This = HTMLSTYLESHEET_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, lIndex);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_removeRule(IHTMLStyleSheet *iface, long lIndex)
{
    HTMLStyleSheet *This = HTMLSTYLESHEET_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, lIndex);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_put_media(IHTMLStyleSheet *iface, BSTR v)
{
    HTMLStyleSheet *This = HTMLSTYLESHEET_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_media(IHTMLStyleSheet *iface, BSTR *p)
{
    HTMLStyleSheet *This = HTMLSTYLESHEET_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_put_cssText(IHTMLStyleSheet *iface, BSTR v)
{
    HTMLStyleSheet *This = HTMLSTYLESHEET_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_cssText(IHTMLStyleSheet *iface, BSTR *p)
{
    HTMLStyleSheet *This = HTMLSTYLESHEET_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_rules(IHTMLStyleSheet *iface,
                                               IHTMLStyleSheetRulesCollection **p)
{
    HTMLStyleSheet *This = HTMLSTYLESHEET_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
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

IHTMLStyleSheet *HTMLStyleSheet_Create(void)
{
    HTMLStyleSheet *ret = mshtml_alloc(sizeof(HTMLStyleSheet));

    ret->lpHTMLStyleSheetVtbl = &HTMLStyleSheetVtbl;
    ret->ref = 1;

    return HTMLSTYLESHEET(ret);
}
