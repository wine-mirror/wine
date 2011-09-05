/*
 * Copyright 2011 Jacek Caban for CodeWeavers
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


#include <assert.h>

#include "vbscript.h"
#include "objsafe.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vbscript);

#ifdef _WIN64

#define CTXARG_T DWORDLONG
#define IActiveScriptParseVtbl IActiveScriptParse64Vtbl

#else

#define CTXARG_T DWORD
#define IActiveScriptParseVtbl IActiveScriptParse32Vtbl

#endif

struct VBScript {
    IActiveScript IActiveScript_iface;
    IActiveScriptParse IActiveScriptParse_iface;
    IObjectSafety IObjectSafety_iface;

    LONG ref;

    DWORD safeopt;
    SCRIPTSTATE state;
    IActiveScriptSite *site;
    script_ctx_t *ctx;
    LONG thread_id;
    LCID lcid;
};

static void change_state(VBScript *This, SCRIPTSTATE state)
{
    if(This->state == state)
        return;

    This->state = state;
    if(This->site)
        IActiveScriptSite_OnStateChange(This->site, state);
}

static void exec_queued_code(VBScript *This)
{
    FIXME("\n");
}

static HRESULT set_ctx_site(VBScript *This)
{
    HRESULT hres;

    This->ctx->lcid = This->lcid;

    hres = init_global(This->ctx);
    if(FAILED(hres))
        return hres;

    IActiveScriptSite_AddRef(This->site);
    This->ctx->site = This->site;

    change_state(This, SCRIPTSTATE_INITIALIZED);
    return S_OK;
}

static void destroy_script(script_ctx_t *ctx)
{
    while(!list_empty(&ctx->named_items)) {
        named_item_t *iter = LIST_ENTRY(list_head(&ctx->named_items), named_item_t, entry);

        list_remove(&iter->entry);
        if(iter->disp)
            IDispatch_Release(iter->disp);
        heap_free(iter->name);
        heap_free(iter);
    }

    if(ctx->host_global)
        IDispatch_Release(ctx->host_global);
    if(ctx->site)
        IActiveScriptSite_Release(ctx->site);
    if(ctx->script_obj)
        IDispatchEx_Release(&ctx->script_obj->IDispatchEx_iface);
    heap_free(ctx);
}

static void decrease_state(VBScript *This, SCRIPTSTATE state)
{
    switch(This->state) {
    case SCRIPTSTATE_CONNECTED:
        change_state(This, SCRIPTSTATE_DISCONNECTED);
        if(state == SCRIPTSTATE_DISCONNECTED)
            return;
        /* FALLTHROUGH */
    case SCRIPTSTATE_STARTED:
    case SCRIPTSTATE_DISCONNECTED:
        if(This->state == SCRIPTSTATE_DISCONNECTED)
            change_state(This, SCRIPTSTATE_INITIALIZED);
        if(state == SCRIPTSTATE_INITIALIZED)
            break;
        /* FALLTHROUGH */
    case SCRIPTSTATE_INITIALIZED:
    case SCRIPTSTATE_UNINITIALIZED:
        change_state(This, state);

        if(This->site) {
            IActiveScriptSite_Release(This->site);
            This->site = NULL;
        }

        This->thread_id = 0;

        if(state == SCRIPTSTATE_CLOSED) {
            destroy_script(This->ctx);
            This->ctx = NULL;
        }

        break;
    default:
        assert(0);
    }
}

static inline VBScript *impl_from_IActiveScript(IActiveScript *iface)
{
    return CONTAINING_RECORD(iface, VBScript, IActiveScript_iface);
}

static HRESULT WINAPI VBScript_QueryInterface(IActiveScript *iface, REFIID riid, void **ppv)
{
    VBScript *This = impl_from_IActiveScript(iface);

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IActiveScript_iface;
    }else if(IsEqualGUID(riid, &IID_IActiveScript)) {
        TRACE("(%p)->(IID_IActiveScript %p)\n", This, ppv);
        *ppv = &This->IActiveScript_iface;
    }else if(IsEqualGUID(riid, &IID_IActiveScriptParse)) {
        TRACE("(%p)->(IID_IActiveScriptParse %p)\n", This, ppv);
        *ppv = &This->IActiveScriptParse_iface;
    }else if(IsEqualGUID(riid, &IID_IObjectSafety)) {
        TRACE("(%p)->(IID_IObjectSafety %p)\n", This, ppv);
        *ppv = &This->IObjectSafety_iface;
    }else {
        FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI VBScript_AddRef(IActiveScript *iface)
{
    VBScript *This = impl_from_IActiveScript(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI VBScript_Release(IActiveScript *iface)
{
    VBScript *This = impl_from_IActiveScript(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", iface, ref);

    if(!ref) {
        if(This->site)
            IActiveScriptSite_Release(This->site);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI VBScript_SetScriptSite(IActiveScript *iface, IActiveScriptSite *pass)
{
    VBScript *This = impl_from_IActiveScript(iface);
    LCID lcid;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, pass);

    if(!pass)
        return E_POINTER;

    if(This->site)
        return E_UNEXPECTED;

    if(InterlockedCompareExchange(&This->thread_id, GetCurrentThreadId(), 0))
        return E_UNEXPECTED;

    This->site = pass;
    IActiveScriptSite_AddRef(This->site);

    hres = IActiveScriptSite_GetLCID(This->site, &lcid);
    if(hres == S_OK)
        This->lcid = lcid;

    return This->ctx ? set_ctx_site(This) : S_OK;
}

static HRESULT WINAPI VBScript_GetScriptSite(IActiveScript *iface, REFIID riid,
                                            void **ppvObject)
{
    VBScript *This = impl_from_IActiveScript(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI VBScript_SetScriptState(IActiveScript *iface, SCRIPTSTATE ss)
{
    VBScript *This = impl_from_IActiveScript(iface);

    TRACE("(%p)->(%d)\n", This, ss);

    if(This->thread_id && GetCurrentThreadId() != This->thread_id)
        return E_UNEXPECTED;

    if(ss == SCRIPTSTATE_UNINITIALIZED) {
        if(This->state == SCRIPTSTATE_CLOSED)
            return E_UNEXPECTED;

        decrease_state(This, SCRIPTSTATE_UNINITIALIZED);
        return S_OK;
    }

    if(!This->ctx)
        return E_UNEXPECTED;

    switch(ss) {
    case SCRIPTSTATE_STARTED:
    case SCRIPTSTATE_CONNECTED: /* FIXME */
        if(This->state == SCRIPTSTATE_CLOSED)
            return E_UNEXPECTED;

        exec_queued_code(This);
        break;
    case SCRIPTSTATE_INITIALIZED:
        FIXME("unimplemented SCRIPTSTATE_INITIALIZED\n");
        return S_OK;
    default:
        FIXME("unimplemented state %d\n", ss);
        return E_NOTIMPL;
    }

    change_state(This, ss);
    return S_OK;
}

static HRESULT WINAPI VBScript_GetScriptState(IActiveScript *iface, SCRIPTSTATE *pssState)
{
    VBScript *This = impl_from_IActiveScript(iface);

    TRACE("(%p)->(%p)\n", This, pssState);

    if(!pssState)
        return E_POINTER;

    if(This->thread_id && This->thread_id != GetCurrentThreadId())
        return E_UNEXPECTED;

    *pssState = This->state;
    return S_OK;
}

static HRESULT WINAPI VBScript_Close(IActiveScript *iface)
{
    VBScript *This = impl_from_IActiveScript(iface);

    TRACE("(%p)->()\n", This);

    if(This->thread_id && This->thread_id != GetCurrentThreadId())
        return E_UNEXPECTED;

    decrease_state(This, SCRIPTSTATE_CLOSED);
    return S_OK;
}

static HRESULT WINAPI VBScript_AddNamedItem(IActiveScript *iface, LPCOLESTR pstrName, DWORD dwFlags)
{
    VBScript *This = impl_from_IActiveScript(iface);
    named_item_t *item;
    IDispatch *disp = NULL;
    HRESULT hres;

    TRACE("(%p)->(%s %x)\n", This, debugstr_w(pstrName), dwFlags);

    if(This->thread_id != GetCurrentThreadId() || !This->ctx || This->state == SCRIPTSTATE_CLOSED)
        return E_UNEXPECTED;

    if(dwFlags & SCRIPTITEM_GLOBALMEMBERS) {
        IUnknown *unk;

        hres = IActiveScriptSite_GetItemInfo(This->site, pstrName, SCRIPTINFO_IUNKNOWN, &unk, NULL);
        if(FAILED(hres)) {
            WARN("GetItemInfo failed: %08x\n", hres);
            return hres;
        }

        hres = IUnknown_QueryInterface(unk, &IID_IDispatch, (void**)&disp);
        IUnknown_Release(unk);
        if(FAILED(hres)) {
            WARN("object does not implement IDispatch\n");
            return hres;
        }

        if(This->ctx->host_global)
            IDispatch_Release(This->ctx->host_global);
        IDispatch_AddRef(disp);
        This->ctx->host_global = disp;
    }

    item = heap_alloc(sizeof(*item));
    if(!item) {
        if(disp)
            IDispatch_Release(disp);
        return E_OUTOFMEMORY;
    }

    item->disp = disp;
    item->flags = dwFlags;
    item->name = heap_strdupW(pstrName);
    if(!item->name) {
        if(disp)
            IDispatch_Release(disp);
        heap_free(item);
        return E_OUTOFMEMORY;
    }

    list_add_tail(&This->ctx->named_items, &item->entry);
    return S_OK;
}

static HRESULT WINAPI VBScript_AddTypeLib(IActiveScript *iface, REFGUID rguidTypeLib,
        DWORD dwMajor, DWORD dwMinor, DWORD dwFlags)
{
    VBScript *This = impl_from_IActiveScript(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI VBScript_GetScriptDispatch(IActiveScript *iface, LPCOLESTR pstrItemName, IDispatch **ppdisp)
{
    VBScript *This = impl_from_IActiveScript(iface);

    TRACE("(%p)->(%p)\n", This, ppdisp);

    if(!ppdisp)
        return E_POINTER;

    if(This->thread_id != GetCurrentThreadId() || !This->ctx->script_obj) {
        *ppdisp = NULL;
        return E_UNEXPECTED;
    }

    *ppdisp = (IDispatch*)&This->ctx->script_obj->IDispatchEx_iface;
    IDispatch_AddRef(*ppdisp);
    return S_OK;
}

static HRESULT WINAPI VBScript_GetCurrentScriptThreadID(IActiveScript *iface,
                                                       SCRIPTTHREADID *pstridThread)
{
    VBScript *This = impl_from_IActiveScript(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI VBScript_GetScriptThreadID(IActiveScript *iface,
                                                DWORD dwWin32ThreadId, SCRIPTTHREADID *pstidThread)
{
    VBScript *This = impl_from_IActiveScript(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI VBScript_GetScriptThreadState(IActiveScript *iface,
        SCRIPTTHREADID stidThread, SCRIPTTHREADSTATE *pstsState)
{
    VBScript *This = impl_from_IActiveScript(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI VBScript_InterruptScriptThread(IActiveScript *iface,
        SCRIPTTHREADID stidThread, const EXCEPINFO *pexcepinfo, DWORD dwFlags)
{
    VBScript *This = impl_from_IActiveScript(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI VBScript_Clone(IActiveScript *iface, IActiveScript **ppscript)
{
    VBScript *This = impl_from_IActiveScript(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static const IActiveScriptVtbl VBScriptVtbl = {
    VBScript_QueryInterface,
    VBScript_AddRef,
    VBScript_Release,
    VBScript_SetScriptSite,
    VBScript_GetScriptSite,
    VBScript_SetScriptState,
    VBScript_GetScriptState,
    VBScript_Close,
    VBScript_AddNamedItem,
    VBScript_AddTypeLib,
    VBScript_GetScriptDispatch,
    VBScript_GetCurrentScriptThreadID,
    VBScript_GetScriptThreadID,
    VBScript_GetScriptThreadState,
    VBScript_InterruptScriptThread,
    VBScript_Clone
};

static inline VBScript *impl_from_IActiveScriptParse(IActiveScriptParse *iface)
{
    return CONTAINING_RECORD(iface, VBScript, IActiveScriptParse_iface);
}

static HRESULT WINAPI VBScriptParse_QueryInterface(IActiveScriptParse *iface, REFIID riid, void **ppv)
{
    VBScript *This = impl_from_IActiveScriptParse(iface);
    return IActiveScript_QueryInterface(&This->IActiveScript_iface, riid, ppv);
}

static ULONG WINAPI VBScriptParse_AddRef(IActiveScriptParse *iface)
{
    VBScript *This = impl_from_IActiveScriptParse(iface);
    return IActiveScript_AddRef(&This->IActiveScript_iface);
}

static ULONG WINAPI VBScriptParse_Release(IActiveScriptParse *iface)
{
    VBScript *This = impl_from_IActiveScriptParse(iface);
    return IActiveScript_Release(&This->IActiveScript_iface);
}

static HRESULT WINAPI VBScriptParse_InitNew(IActiveScriptParse *iface)
{
    VBScript *This = impl_from_IActiveScriptParse(iface);
    script_ctx_t *ctx, *old_ctx;

    TRACE("(%p)\n", This);

    if(This->ctx)
        return E_UNEXPECTED;

    ctx = heap_alloc_zero(sizeof(script_ctx_t));
    if(!ctx)
        return E_OUTOFMEMORY;

    list_init(&ctx->named_items);

    old_ctx = InterlockedCompareExchangePointer((void**)&This->ctx, ctx, NULL);
    if(old_ctx) {
        destroy_script(ctx);
        return E_UNEXPECTED;
    }

    return This->site ? set_ctx_site(This) : S_OK;
}

static HRESULT WINAPI VBScriptParse_AddScriptlet(IActiveScriptParse *iface,
        LPCOLESTR pstrDefaultName, LPCOLESTR pstrCode, LPCOLESTR pstrItemName,
        LPCOLESTR pstrSubItemName, LPCOLESTR pstrEventName, LPCOLESTR pstrDelimiter,
        CTXARG_T dwSourceContextCookie, ULONG ulStartingLineNumber, DWORD dwFlags,
        BSTR *pbstrName, EXCEPINFO *pexcepinfo)
{
    VBScript *This = impl_from_IActiveScriptParse(iface);
    FIXME("(%p)->(%s %s %s %s %s %s %s %u %x %p %p)\n", This, debugstr_w(pstrDefaultName),
          debugstr_w(pstrCode), debugstr_w(pstrItemName), debugstr_w(pstrSubItemName),
          debugstr_w(pstrEventName), debugstr_w(pstrDelimiter), wine_dbgstr_longlong(dwSourceContextCookie),
          ulStartingLineNumber, dwFlags, pbstrName, pexcepinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI VBScriptParse_ParseScriptText(IActiveScriptParse *iface,
        LPCOLESTR pstrCode, LPCOLESTR pstrItemName, IUnknown *punkContext,
        LPCOLESTR pstrDelimiter, CTXARG_T dwSourceContextCookie, ULONG ulStartingLine,
        DWORD dwFlags, VARIANT *pvarResult, EXCEPINFO *pexcepinfo)
{
    VBScript *This = impl_from_IActiveScriptParse(iface);
    FIXME("(%p)->(%s %s %p %s %s %u %x %p %p)\n", This, debugstr_w(pstrCode),
          debugstr_w(pstrItemName), punkContext, debugstr_w(pstrDelimiter),
          wine_dbgstr_longlong(dwSourceContextCookie), ulStartingLine, dwFlags, pvarResult, pexcepinfo);
    return E_NOTIMPL;
}

static const IActiveScriptParseVtbl VBScriptParseVtbl = {
    VBScriptParse_QueryInterface,
    VBScriptParse_AddRef,
    VBScriptParse_Release,
    VBScriptParse_InitNew,
    VBScriptParse_AddScriptlet,
    VBScriptParse_ParseScriptText
};

static inline VBScript *impl_from_IObjectSafety(IObjectSafety *iface)
{
    return CONTAINING_RECORD(iface, VBScript, IObjectSafety_iface);
}

static HRESULT WINAPI VBScriptSafety_QueryInterface(IObjectSafety *iface, REFIID riid, void **ppv)
{
    VBScript *This = impl_from_IObjectSafety(iface);
    return IActiveScript_QueryInterface(&This->IActiveScript_iface, riid, ppv);
}

static ULONG WINAPI VBScriptSafety_AddRef(IObjectSafety *iface)
{
    VBScript *This = impl_from_IObjectSafety(iface);
    return IActiveScript_AddRef(&This->IActiveScript_iface);
}

static ULONG WINAPI VBScriptSafety_Release(IObjectSafety *iface)
{
    VBScript *This = impl_from_IObjectSafety(iface);
    return IActiveScript_Release(&This->IActiveScript_iface);
}

#define SUPPORTED_OPTIONS (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER)

static HRESULT WINAPI VBScriptSafety_GetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
{
    VBScript *This = impl_from_IObjectSafety(iface);

    TRACE("(%p)->(%s %p %p)\n", This, debugstr_guid(riid), pdwSupportedOptions, pdwEnabledOptions);

    if(!pdwSupportedOptions || !pdwEnabledOptions)
        return E_POINTER;

    *pdwSupportedOptions = SUPPORTED_OPTIONS;
    *pdwEnabledOptions = This->safeopt;
    return S_OK;
}

static HRESULT WINAPI VBScriptSafety_SetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD dwOptionSetMask, DWORD dwEnabledOptions)
{
    VBScript *This = impl_from_IObjectSafety(iface);

    TRACE("(%p)->(%s %x %x)\n", This, debugstr_guid(riid), dwOptionSetMask, dwEnabledOptions);

    if(dwOptionSetMask & ~SUPPORTED_OPTIONS)
        return E_FAIL;

    This->safeopt = (dwEnabledOptions & dwOptionSetMask) | (This->safeopt & ~dwOptionSetMask) | INTERFACE_USES_DISPEX;
    return S_OK;
}

static const IObjectSafetyVtbl VBScriptSafetyVtbl = {
    VBScriptSafety_QueryInterface,
    VBScriptSafety_AddRef,
    VBScriptSafety_Release,
    VBScriptSafety_GetInterfaceSafetyOptions,
    VBScriptSafety_SetInterfaceSafetyOptions
};

HRESULT WINAPI VBScriptFactory_CreateInstance(IClassFactory *iface, IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    VBScript *ret;
    HRESULT hres;

    TRACE("(%p %s %p)\n", pUnkOuter, debugstr_guid(riid), ppv);

    ret = heap_alloc_zero(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IActiveScript_iface.lpVtbl = &VBScriptVtbl;
    ret->IActiveScriptParse_iface.lpVtbl = &VBScriptParseVtbl;
    ret->IObjectSafety_iface.lpVtbl = &VBScriptSafetyVtbl;

    ret->ref = 1;
    ret->state = SCRIPTSTATE_UNINITIALIZED;
    ret->safeopt = INTERFACE_USES_DISPEX;

    hres = IActiveScript_QueryInterface(&ret->IActiveScript_iface, riid, ppv);
    IActiveScript_Release(&ret->IActiveScript_iface);
    return hres;
}
