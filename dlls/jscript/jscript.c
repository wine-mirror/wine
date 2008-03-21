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

#include "jscript.h"
#include "activscp.h"
#include "objsafe.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

typedef struct {
    const IActiveScriptVtbl                 *lpIActiveScriptVtbl;
    const IActiveScriptParseVtbl            *lpIActiveScriptParseVtbl;
    const IActiveScriptParseProcedure2Vtbl  *lpIActiveScriptParseProcedure2Vtbl;
    const IActiveScriptPropertyVtbl         *lpIActiveScriptPropertyVtbl;
    const IObjectSafetyVtbl                 *lpIObjectSafetyVtbl;

    LONG ref;
} JScript;

#define ACTSCRIPT(x)    ((IActiveScript*)                 &(x)->lpIActiveScriptVtbl)
#define ASPARSE(x)      ((IActiveScriptParse*)            &(x)->lpIActiveScriptParseVtbl)
#define ASPARSEPROC(x)  ((IActiveScriptParseProcedure2*)  &(x)->lpIActiveScriptParseProcedure2Vtbl)
#define ACTSCPPROP(x)   ((IActiveScriptProperty*)         &(x)->lpIActiveScriptPropertyVtbl)
#define OBJSAFETY(x)    ((IObjectSafety*)                 &(x)->lpIObjectSafetyVtbl)

#define ACTSCRIPT_THIS(iface) DEFINE_THIS(JScript, IActiveScript, iface)

static HRESULT WINAPI JScript_QueryInterface(IActiveScript *iface, REFIID riid, void **ppv)
{
    JScript *This = ACTSCRIPT_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = ACTSCRIPT(This);
    }else if(IsEqualGUID(riid, &IID_IActiveScript)) {
        TRACE("(%p)->(IID_IActiveScript %p)\n", This, ppv);
        *ppv = ACTSCRIPT(This);
    }else if(IsEqualGUID(riid, &IID_IActiveScriptParse)) {
        TRACE("(%p)->(IID_IActiveScriptParse %p)\n", This, ppv);
        *ppv = ASPARSE(This);
    }else if(IsEqualGUID(riid, &IID_IActiveScriptParseProcedure)) {
        TRACE("(%p)->(IID_IActiveScriptParseProcedure %p)\n", This, ppv);
        *ppv = ASPARSEPROC(This);
    }else if(IsEqualGUID(riid, &IID_IActiveScriptParseProcedure2)) {
        TRACE("(%p)->(IID_IActiveScriptParseProcedure2 %p)\n", This, ppv);
        *ppv = ASPARSEPROC(This);
    }else if(IsEqualGUID(riid, &IID_IActiveScriptProperty)) {
        TRACE("(%p)->(IID_IActiveScriptProperty %p)\n", This, ppv);
        *ppv = ACTSCPPROP(This);
    }else if(IsEqualGUID(riid, &IID_IObjectSafety)) {
        TRACE("(%p)->(IID_IObjectSafety %p)\n", This, ppv);
        *ppv = OBJSAFETY(This);
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI JScript_AddRef(IActiveScript *iface)
{
    JScript *This = ACTSCRIPT_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI JScript_Release(IActiveScript *iface)
{
    JScript *This = ACTSCRIPT_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", iface, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI JScript_SetScriptSite(IActiveScript *iface,
                                            IActiveScriptSite *pass)
{
    JScript *This = ACTSCRIPT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pass);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScript_GetScriptSite(IActiveScript *iface, REFIID riid,
                                            void **ppvObject)
{
    JScript *This = ACTSCRIPT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScript_SetScriptState(IActiveScript *iface, SCRIPTSTATE ss)
{
    JScript *This = ACTSCRIPT_THIS(iface);
    FIXME("(%p)->(%d)\n", This, ss);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScript_GetScriptState(IActiveScript *iface, SCRIPTSTATE *pssState)
{
    JScript *This = ACTSCRIPT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pssState);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScript_Close(IActiveScript *iface)
{
    JScript *This = ACTSCRIPT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScript_AddNamedItem(IActiveScript *iface,
                                           LPCOLESTR pstrName, DWORD dwFlags)
{
    JScript *This = ACTSCRIPT_THIS(iface);
    FIXME("(%p)->(%s %x)\n", This, debugstr_w(pstrName), dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScript_AddTypeLib(IActiveScript *iface, REFGUID rguidTypeLib,
                                         DWORD dwMajor, DWORD dwMinor, DWORD dwFlags)
{
    JScript *This = ACTSCRIPT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScript_GetScriptDispatch(IActiveScript *iface, LPCOLESTR pstrItemName,
                                                IDispatch **ppdisp)
{
    JScript *This = ACTSCRIPT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScript_GetCurrentScriptThreadID(IActiveScript *iface,
                                                       SCRIPTTHREADID *pstridThread)
{
    JScript *This = ACTSCRIPT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScript_GetScriptThreadID(IActiveScript *iface,
                                                DWORD dwWin32ThreadId, SCRIPTTHREADID *pstidThread)
{
    JScript *This = ACTSCRIPT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScript_GetScriptThreadState(IActiveScript *iface,
        SCRIPTTHREADID stidThread, SCRIPTTHREADSTATE *pstsState)
{
    JScript *This = ACTSCRIPT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScript_InterruptScriptThread(IActiveScript *iface,
        SCRIPTTHREADID stidThread, const EXCEPINFO *pexcepinfo, DWORD dwFlags)
{
    JScript *This = ACTSCRIPT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScript_Clone(IActiveScript *iface, IActiveScript **ppscript)
{
    JScript *This = ACTSCRIPT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

#undef ACTSCRIPT_THIS

static const IActiveScriptVtbl JScriptVtbl = {
    JScript_QueryInterface,
    JScript_AddRef,
    JScript_Release,
    JScript_SetScriptSite,
    JScript_GetScriptSite,
    JScript_SetScriptState,
    JScript_GetScriptState,
    JScript_Close,
    JScript_AddNamedItem,
    JScript_AddTypeLib,
    JScript_GetScriptDispatch,
    JScript_GetCurrentScriptThreadID,
    JScript_GetScriptThreadID,
    JScript_GetScriptThreadState,
    JScript_InterruptScriptThread,
    JScript_Clone
};

#define ASPARSE_THIS(iface) DEFINE_THIS(JScript, IActiveScriptParse, iface)

static HRESULT WINAPI JScriptParse_QueryInterface(IActiveScriptParse *iface, REFIID riid, void **ppv)
{
    JScript *This = ASPARSE_THIS(iface);
    return IActiveScript_QueryInterface(ACTSCRIPT(This), riid, ppv);
}

static ULONG WINAPI JScriptParse_AddRef(IActiveScriptParse *iface)
{
    JScript *This = ASPARSE_THIS(iface);
    return IActiveScript_AddRef(ACTSCRIPT(This));
}

static ULONG WINAPI JScriptParse_Release(IActiveScriptParse *iface)
{
    JScript *This = ASPARSE_THIS(iface);
    return IActiveScript_Release(ACTSCRIPT(This));
}

static HRESULT WINAPI JScriptParse_InitNew(IActiveScriptParse *iface)
{
    JScript *This = ASPARSE_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScriptParse_AddScriptlet(IActiveScriptParse *iface,
        LPCOLESTR pstrDefaultName, LPCOLESTR pstrCode, LPCOLESTR pstrItemName,
        LPCOLESTR pstrSubItemName, LPCOLESTR pstrEventName, LPCOLESTR pstrDelimiter,
        DWORD dwSourceContextCookie, ULONG ulStartingLineNumber, DWORD dwFlags,
        BSTR *pbstrName, EXCEPINFO *pexcepinfo)
{
    JScript *This = ASPARSE_THIS(iface);
    FIXME("(%p)->(%s %s %s %s %s %s %x %u %x %p %p)\n", This, debugstr_w(pstrDefaultName),
          debugstr_w(pstrCode), debugstr_w(pstrItemName), debugstr_w(pstrSubItemName),
          debugstr_w(pstrEventName), debugstr_w(pstrDelimiter), dwSourceContextCookie,
          ulStartingLineNumber, dwFlags, pbstrName, pexcepinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScriptParse_ParseScriptText(IActiveScriptParse *iface,
        LPCOLESTR pstrCode, LPCOLESTR pstrItemName, IUnknown *punkContext,
        LPCOLESTR pstrDelimiter, DWORD dwSourceContextCookie, ULONG ulStartingLine,
        DWORD dwFlags, VARIANT *pvarResult, EXCEPINFO *pexcepinfo)
{
    JScript *This = ASPARSE_THIS(iface);
    FIXME("(%p)->(%s %s %p %s %x %u %x %p %p)\n", This, debugstr_w(pstrCode),
          debugstr_w(pstrItemName), punkContext, debugstr_w(pstrDelimiter),
          dwSourceContextCookie, ulStartingLine, dwFlags, pvarResult, pexcepinfo);
    return E_NOTIMPL;
}

#undef ASPARSE_THIS

static const IActiveScriptParseVtbl JScriptParseVtbl = {
    JScriptParse_QueryInterface,
    JScriptParse_AddRef,
    JScriptParse_Release,
    JScriptParse_InitNew,
    JScriptParse_AddScriptlet,
    JScriptParse_ParseScriptText
};

#define ASPARSEPROC_THIS(iface) DEFINE_THIS(JScript, IActiveScriptParse, iface)

static HRESULT WINAPI JScriptParseProcedure_QueryInterface(IActiveScriptParseProcedure2 *iface, REFIID riid, void **ppv)
{
    JScript *This = ASPARSEPROC_THIS(iface);
    return IActiveScript_QueryInterface(ACTSCRIPT(This), riid, ppv);
}

static ULONG WINAPI JScriptParseProcedure_AddRef(IActiveScriptParseProcedure2 *iface)
{
    JScript *This = ASPARSEPROC_THIS(iface);
    return IActiveScript_AddRef(ACTSCRIPT(This));
}

static ULONG WINAPI JScriptParseProcedure_Release(IActiveScriptParseProcedure2 *iface)
{
    JScript *This = ASPARSEPROC_THIS(iface);
    return IActiveScript_Release(ACTSCRIPT(This));
}

static HRESULT WINAPI JScriptParseProcedure_ParseProcedureText(IActiveScriptParseProcedure2 *iface,
        LPCOLESTR pstrCode, LPCOLESTR pstrFormalParams, LPCOLESTR pstrProcedureName,
        LPCOLESTR pstrItemName, IUnknown *punkContext, LPCOLESTR pstrDelimiter,
        DWORD dwSourceContextCookie, ULONG ulStartingLineNumber, DWORD dwFlags, IDispatch **ppdisp)
{
    JScript *This = ASPARSEPROC_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

#undef ASPARSEPROC_THIS

static const IActiveScriptParseProcedure2Vtbl JScriptParseProcedureVtbl = {
    JScriptParseProcedure_QueryInterface,
    JScriptParseProcedure_AddRef,
    JScriptParseProcedure_Release,
    JScriptParseProcedure_ParseProcedureText,
};

#define ACTSCPPROP_THIS(iface) DEFINE_THIS(JScript, IActiveScriptProperty, iface)

static HRESULT WINAPI JScriptProperty_QueryInterface(IActiveScriptProperty *iface, REFIID riid, void **ppv)
{
    JScript *This = ACTSCPPROP_THIS(iface);
    return IActiveScript_QueryInterface(ACTSCRIPT(This), riid, ppv);
}

static ULONG WINAPI JScriptProperty_AddRef(IActiveScriptProperty *iface)
{
    JScript *This = ACTSCPPROP_THIS(iface);
    return IActiveScript_AddRef(ACTSCRIPT(This));
}

static ULONG WINAPI JScriptProperty_Release(IActiveScriptProperty *iface)
{
    JScript *This = ACTSCPPROP_THIS(iface);
    return IActiveScript_Release(ACTSCRIPT(This));
}

static HRESULT WINAPI JScriptProperty_GetProperty(IActiveScriptProperty *iface, DWORD dwProperty,
        VARIANT *pvarIndex, VARIANT *pvarValue)
{
    JScript *This = ACTSCPPROP_THIS(iface);
    FIXME("(%p)->(%x %p %p)\n", This, dwProperty, pvarIndex, pvarValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScriptProperty_SetProperty(IActiveScriptProperty *iface, DWORD dwProperty,
        VARIANT *pvarIndex, VARIANT *pvarValue)
{
    JScript *This = ACTSCPPROP_THIS(iface);
    FIXME("(%p)->(%x %p %p)\n", This, dwProperty, pvarIndex, pvarValue);
    return E_NOTIMPL;
}

#undef ACTSCPPROP_THIS

static const IActiveScriptPropertyVtbl JScriptPropertyVtbl = {
    JScriptProperty_QueryInterface,
    JScriptProperty_AddRef,
    JScriptProperty_Release,
    JScriptProperty_GetProperty,
    JScriptProperty_SetProperty
};

#define OBJSAFETY_THIS(iface) DEFINE_THIS(JScript, IObjectSafety, iface)

static HRESULT WINAPI JScriptSafety_QueryInterface(IObjectSafety *iface, REFIID riid, void **ppv)
{
    JScript *This = OBJSAFETY_THIS(iface);
    return IActiveScript_QueryInterface(ACTSCRIPT(This), riid, ppv);
}

static ULONG WINAPI JScriptSafety_AddRef(IObjectSafety *iface)
{
    JScript *This = OBJSAFETY_THIS(iface);
    return IActiveScript_AddRef(ACTSCRIPT(This));
}

static ULONG WINAPI JScriptSafety_Release(IObjectSafety *iface)
{
    JScript *This = OBJSAFETY_THIS(iface);
    return IActiveScript_Release(ACTSCRIPT(This));
}

static HRESULT WINAPI JScriptSafety_GetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
{
    JScript *This = OBJSAFETY_THIS(iface);
    FIXME("(%p)->(%s %p %p)\n", This, debugstr_guid(riid), pdwSupportedOptions, pdwEnabledOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScriptSafety_SetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD dwOptionSetMask, DWORD dwEnabledOptions)
{
    JScript *This = OBJSAFETY_THIS(iface);
    FIXME("(%p)->(%s %x %x)\n", This, debugstr_guid(riid), dwOptionSetMask, dwEnabledOptions);
    return S_OK;
}

#undef OBJSAFETY_THIS

static const IObjectSafetyVtbl JScriptSafetyVtbl = {
    JScriptSafety_QueryInterface,
    JScriptSafety_AddRef,
    JScriptSafety_Release,
    JScriptSafety_GetInterfaceSafetyOptions,
    JScriptSafety_SetInterfaceSafetyOptions
};

HRESULT WINAPI JScriptFactory_CreateInstance(IClassFactory *iface, IUnknown *pUnkOuter,
                                             REFIID riid, void **ppv)
{
    JScript *ret;
    HRESULT hres;

    TRACE("(%p %s %p)\n", pUnkOuter, debugstr_guid(riid), ppv);

    ret = heap_alloc(sizeof(*ret));

    ret->lpIActiveScriptVtbl                 = &JScriptVtbl;
    ret->lpIActiveScriptParseVtbl            = &JScriptParseVtbl;
    ret->lpIActiveScriptParseProcedure2Vtbl  = &JScriptParseProcedureVtbl;
    ret->lpIActiveScriptPropertyVtbl         = &JScriptPropertyVtbl;
    ret->lpIObjectSafetyVtbl                 = &JScriptSafetyVtbl;
    ret->ref = 1;

    hres = IActiveScript_QueryInterface(ACTSCRIPT(ret), riid, ppv);
    IActiveScript_Release(ACTSCRIPT(ret));
    return hres;
}
