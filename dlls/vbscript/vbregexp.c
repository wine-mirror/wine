/*
 * Copyright 2013 Piotr Caban for CodeWeavers
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

#include "vbscript.h"
#include "vbsregexp55.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vbscript);

#define REGEXP_TID_LIST \
    XDIID(RegExp2) \
    XDIID(Match2) \
    XDIID(MatchCollection2) \
    XDIID(SubMatches)

typedef enum {
#define XDIID(iface) iface ## _tid,
REGEXP_TID_LIST
#undef XDIID
    REGEXP_LAST_tid
} regexp_tid_t;

static REFIID tid_ids[] = {
#define XDIID(iface) &IID_I ## iface,
REGEXP_TID_LIST
#undef XDIID
};

static ITypeLib *typelib;
static ITypeInfo *typeinfos[REGEXP_LAST_tid];

static HRESULT init_regexp_typeinfo(regexp_tid_t tid)
{
    HRESULT hres;

    if(!typelib) {
        static const WCHAR vbscript_dll3W[] = {'v','b','s','c','r','i','p','t','.','d','l','l','\\','3',0};
        ITypeLib *tl;

        hres = LoadTypeLib(vbscript_dll3W, &tl);
        if(FAILED(hres)) {
            ERR("LoadRegTypeLib failed: %08x\n", hres);
            return hres;
        }

        if(InterlockedCompareExchangePointer((void**)&typelib, tl, NULL))
            ITypeLib_Release(tl);
    }

    if(!typeinfos[tid]) {
        ITypeInfo *ti;

        hres = ITypeLib_GetTypeInfoOfGuid(typelib, tid_ids[tid], &ti);
        if(FAILED(hres)) {
            ERR("GetTypeInfoOfGuid(%s) failed: %08x\n", debugstr_guid(tid_ids[tid]), hres);
            return hres;
        }

        if(InterlockedCompareExchangePointer((void**)(typeinfos+tid), ti, NULL))
            ITypeInfo_Release(ti);
    }

    return S_OK;
}

typedef struct {
    IRegExp2 IRegExp2_iface;
    IRegExp IRegExp_iface;

    LONG ref;

    WCHAR *pattern;
} RegExp2;

static inline RegExp2 *impl_from_IRegExp2(IRegExp2 *iface)
{
    return CONTAINING_RECORD(iface, RegExp2, IRegExp2_iface);
}

static HRESULT WINAPI RegExp2_QueryInterface(IRegExp2 *iface, REFIID riid, void **ppv)
{
    RegExp2 *This = impl_from_IRegExp2(iface);

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IRegExp2_iface;
    }else if(IsEqualGUID(riid, &IID_IDispatch)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IRegExp2_iface;
    }else if(IsEqualGUID(riid, &IID_IRegExp2)) {
        TRACE("(%p)->(IID_IRegExp2 %p)\n", This, ppv);
        *ppv = &This->IRegExp2_iface;
    }else if(IsEqualGUID(riid, &IID_IRegExp)) {
        TRACE("(%p)->(IID_IRegExp %p)\n", This, ppv);
        *ppv = &This->IRegExp_iface;
    }else {
        FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI RegExp2_AddRef(IRegExp2 *iface)
{
    RegExp2 *This = impl_from_IRegExp2(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI RegExp2_Release(IRegExp2 *iface)
{
    RegExp2 *This = impl_from_IRegExp2(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        heap_free(This->pattern);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI RegExp2_GetTypeInfoCount(IRegExp2 *iface, UINT *pctinfo)
{
    RegExp2 *This = impl_from_IRegExp2(iface);

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI RegExp2_GetTypeInfo(IRegExp2 *iface,
        UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    RegExp2 *This = impl_from_IRegExp2(iface);
    FIXME("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegExp2_GetIDsOfNames(IRegExp2 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    RegExp2 *This = impl_from_IRegExp2(iface);

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid),
            rgszNames, cNames, lcid, rgDispId);

    return ITypeInfo_GetIDsOfNames(typeinfos[RegExp2_tid], rgszNames, cNames, rgDispId);
}

static HRESULT WINAPI RegExp2_Invoke(IRegExp2 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    RegExp2 *This = impl_from_IRegExp2(iface);

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    return ITypeInfo_Invoke(typeinfos[RegExp2_tid], iface, dispIdMember, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI RegExp2_get_Pattern(IRegExp2 *iface, BSTR *pPattern)
{
    RegExp2 *This = impl_from_IRegExp2(iface);

    TRACE("(%p)->(%p)\n", This, pPattern);

    if(!pPattern)
        return E_POINTER;

    if(!This->pattern) {
        *pPattern = NULL;
        return S_OK;
    }

    *pPattern = SysAllocString(This->pattern);
    return *pPattern ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI RegExp2_put_Pattern(IRegExp2 *iface, BSTR pattern)
{
    RegExp2 *This = impl_from_IRegExp2(iface);
    WCHAR *p;
    DWORD size;

    TRACE("(%p)->(%s)\n", This, wine_dbgstr_w(pattern));

    if(!pattern) {
        heap_free(This->pattern);
        This->pattern = NULL;
        return S_OK;
    }

    size = (SysStringLen(pattern)+1) * sizeof(WCHAR);
    p = heap_alloc(size);
    if(!p)
        return E_OUTOFMEMORY;

    heap_free(This->pattern);
    This->pattern = p;
    memcpy(p, pattern, size);
    return S_OK;
}

static HRESULT WINAPI RegExp2_get_IgnoreCase(IRegExp2 *iface, VARIANT_BOOL *pIgnoreCase)
{
    RegExp2 *This = impl_from_IRegExp2(iface);
    FIXME("(%p)->(%p)\n", This, pIgnoreCase);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegExp2_put_IgnoreCase(IRegExp2 *iface, VARIANT_BOOL ignoreCase)
{
    RegExp2 *This = impl_from_IRegExp2(iface);
    FIXME("(%p)->(%s)\n", This, ignoreCase ? "true" : "false");
    return E_NOTIMPL;
}

static HRESULT WINAPI RegExp2_get_Global(IRegExp2 *iface, VARIANT_BOOL *pGlobal)
{
    RegExp2 *This = impl_from_IRegExp2(iface);
    FIXME("(%p)->(%p)\n", This, pGlobal);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegExp2_put_Global(IRegExp2 *iface, VARIANT_BOOL global)
{
    RegExp2 *This = impl_from_IRegExp2(iface);
    FIXME("(%p)->(%s)\n", This, global ? "true" : "false");
    return E_NOTIMPL;
}

static HRESULT WINAPI RegExp2_get_Multiline(IRegExp2 *iface, VARIANT_BOOL *pMultiline)
{
    RegExp2 *This = impl_from_IRegExp2(iface);
    FIXME("(%p)->(%p)\n", This, pMultiline);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegExp2_put_Multiline(IRegExp2 *iface, VARIANT_BOOL multiline)
{
    RegExp2 *This = impl_from_IRegExp2(iface);
    FIXME("(%p)->(%s)\n", This, multiline ? "true" : "false");
    return E_NOTIMPL;
}

static HRESULT WINAPI RegExp2_Execute(IRegExp2 *iface,
        BSTR sourceString, IDispatch **ppMatches)
{
    RegExp2 *This = impl_from_IRegExp2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(sourceString), ppMatches);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegExp2_Test(IRegExp2 *iface, BSTR sourceString, VARIANT_BOOL *pMatch)
{
    RegExp2 *This = impl_from_IRegExp2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(sourceString), pMatch);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegExp2_Replace(IRegExp2 *iface, BSTR sourceString,
        VARIANT replaceVar, BSTR *pDestString)
{
    RegExp2 *This = impl_from_IRegExp2(iface);
    FIXME("(%p)->(%s %s %p)\n", This, debugstr_w(sourceString),
            debugstr_variant(&replaceVar), pDestString);
    return E_NOTIMPL;
}

static const IRegExp2Vtbl RegExp2Vtbl = {
    RegExp2_QueryInterface,
    RegExp2_AddRef,
    RegExp2_Release,
    RegExp2_GetTypeInfoCount,
    RegExp2_GetTypeInfo,
    RegExp2_GetIDsOfNames,
    RegExp2_Invoke,
    RegExp2_get_Pattern,
    RegExp2_put_Pattern,
    RegExp2_get_IgnoreCase,
    RegExp2_put_IgnoreCase,
    RegExp2_get_Global,
    RegExp2_put_Global,
    RegExp2_get_Multiline,
    RegExp2_put_Multiline,
    RegExp2_Execute,
    RegExp2_Test,
    RegExp2_Replace
};

static inline RegExp2 *impl_from_IRegExp(IRegExp *iface)
{
    return CONTAINING_RECORD(iface, RegExp2, IRegExp_iface);
}

static HRESULT WINAPI RegExp_QueryInterface(IRegExp *iface, REFIID riid, void **ppv)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_QueryInterface(&This->IRegExp2_iface, riid, ppv);
}

static ULONG WINAPI RegExp_AddRef(IRegExp *iface)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_AddRef(&This->IRegExp2_iface);
}

static ULONG WINAPI RegExp_Release(IRegExp *iface)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_Release(&This->IRegExp2_iface);
}

static HRESULT WINAPI RegExp_GetTypeInfoCount(IRegExp *iface, UINT *pctinfo)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_GetTypeInfoCount(&This->IRegExp2_iface, pctinfo);
}

static HRESULT WINAPI RegExp_GetTypeInfo(IRegExp *iface,
        UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_GetTypeInfo(&This->IRegExp2_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI RegExp_GetIDsOfNames(IRegExp *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_GetIDsOfNames(&This->IRegExp2_iface, riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI RegExp_Invoke(IRegExp *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_Invoke(&This->IRegExp2_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI RegExp_get_Pattern(IRegExp *iface, BSTR *pPattern)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_get_Pattern(&This->IRegExp2_iface, pPattern);
}

static HRESULT WINAPI RegExp_put_Pattern(IRegExp *iface, BSTR pPattern)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_put_Pattern(&This->IRegExp2_iface, pPattern);
}

static HRESULT WINAPI RegExp_get_IgnoreCase(IRegExp *iface, VARIANT_BOOL *pIgnoreCase)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_get_IgnoreCase(&This->IRegExp2_iface, pIgnoreCase);
}

static HRESULT WINAPI RegExp_put_IgnoreCase(IRegExp *iface, VARIANT_BOOL pIgnoreCase)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_put_IgnoreCase(&This->IRegExp2_iface, pIgnoreCase);
}

static HRESULT WINAPI RegExp_get_Global(IRegExp *iface, VARIANT_BOOL *pGlobal)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_get_Global(&This->IRegExp2_iface, pGlobal);
}

static HRESULT WINAPI RegExp_put_Global(IRegExp *iface, VARIANT_BOOL pGlobal)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_put_Global(&This->IRegExp2_iface, pGlobal);
}

static HRESULT WINAPI RegExp_Execute(IRegExp *iface,
        BSTR sourceString, IDispatch **ppMatches)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_Execute(&This->IRegExp2_iface, sourceString, ppMatches);
}

static HRESULT WINAPI RegExp_Test(IRegExp *iface, BSTR sourceString, VARIANT_BOOL *pMatch)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_Test(&This->IRegExp2_iface, sourceString, pMatch);
}

static HRESULT WINAPI RegExp_Replace(IRegExp *iface, BSTR sourceString,
        BSTR replaceString, BSTR *pDestString)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    VARIANT replace;

    V_VT(&replace) = VT_BSTR;
    V_BSTR(&replace) = replaceString;
    return IRegExp2_Replace(&This->IRegExp2_iface, sourceString, replace, pDestString);
}

static IRegExpVtbl RegExpVtbl = {
    RegExp_QueryInterface,
    RegExp_AddRef,
    RegExp_Release,
    RegExp_GetTypeInfoCount,
    RegExp_GetTypeInfo,
    RegExp_GetIDsOfNames,
    RegExp_Invoke,
    RegExp_get_Pattern,
    RegExp_put_Pattern,
    RegExp_get_IgnoreCase,
    RegExp_put_IgnoreCase,
    RegExp_get_Global,
    RegExp_put_Global,
    RegExp_Execute,
    RegExp_Test,
    RegExp_Replace
};

HRESULT WINAPI VBScriptRegExpFactory_CreateInstance(IClassFactory *iface, IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    RegExp2 *ret;
    HRESULT hres;

    TRACE("(%p %s %p)\n", pUnkOuter, debugstr_guid(riid), ppv);

    hres = init_regexp_typeinfo(RegExp2_tid);
    if(FAILED(hres))
        return hres;

    ret = heap_alloc_zero(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IRegExp2_iface.lpVtbl = &RegExp2Vtbl;
    ret->IRegExp_iface.lpVtbl = &RegExpVtbl;

    ret->ref = 1;

    hres = IRegExp2_QueryInterface(&ret->IRegExp2_iface, riid, ppv);
    IRegExp2_Release(&ret->IRegExp2_iface);
    return hres;
}

void release_regexp_typelib(void)
{
    DWORD i;

    for(i=0; i<REGEXP_LAST_tid; i++) {
        if(typeinfos[i])
            ITypeInfo_Release(typeinfos[i]);
    }
    if(typelib)
        ITypeLib_Release(typelib);
}
