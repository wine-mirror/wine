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

#include "config.h"

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "ole2.h"

#include "vbscript.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vbscript);

#ifdef _WIN64

#define CTXARG_T DWORDLONG
#define IActiveScriptParseVtbl IActiveScriptParse64Vtbl

#else

#define CTXARG_T DWORD
#define IActiveScriptParseVtbl IActiveScriptParse32Vtbl

#endif

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

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI VBScript_SetScriptSite(IActiveScript *iface, IActiveScriptSite *pass)
{
    VBScript *This = impl_from_IActiveScript(iface);
    FIXME("(%p)->(%p)\n", This, pass);
    return S_OK;
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
    FIXME("(%p)->(%d)\n", This, ss);
    return S_OK;
}

static HRESULT WINAPI VBScript_GetScriptState(IActiveScript *iface, SCRIPTSTATE *pssState)
{
    VBScript *This = impl_from_IActiveScript(iface);
    FIXME("(%p)->(%p)\n", This, pssState);
    return E_NOTIMPL;
}

static HRESULT WINAPI VBScript_Close(IActiveScript *iface)
{
    VBScript *This = impl_from_IActiveScript(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI VBScript_AddNamedItem(IActiveScript *iface, LPCOLESTR pstrName, DWORD dwFlags)
{
    VBScript *This = impl_from_IActiveScript(iface);
    FIXME("(%p)->(%s %x)\n", This, debugstr_w(pstrName), dwFlags);
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
    FIXME("(%p)->(%p)\n", This, ppdisp);
    return E_NOTIMPL;
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

HRESULT WINAPI VBScriptFactory_CreateInstance(IClassFactory *iface, IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    VBScript *ret;
    HRESULT hres;

    TRACE("(%p %s %p)\n", pUnkOuter, debugstr_guid(riid), ppv);

    ret = heap_alloc_zero(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IActiveScript_iface.lpVtbl = &VBScriptVtbl;

    ret->ref = 1;

    hres = IActiveScript_QueryInterface(&ret->IActiveScript_iface, riid, ppv);
    IActiveScript_Release(&ret->IActiveScript_iface);
    return hres;
}
