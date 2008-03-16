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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

typedef struct {
    const IActiveScriptVtbl  *lpIActiveScriptVtbl;

    LONG ref;
} JScript;

#define ACTSCRIPT(x)  ((IActiveScript*)      &(x)->lpIActiveScriptVtbl)

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

HRESULT WINAPI JScriptFactory_CreateInstance(IClassFactory *iface, IUnknown *pUnkOuter,
                                             REFIID riid, void **ppv)
{
    JScript *ret;
    HRESULT hres;

    TRACE("(%p %s %p)\n", pUnkOuter, debugstr_guid(riid), ppv);

    ret = heap_alloc(sizeof(*ret));

    ret->lpIActiveScriptVtbl = &JScriptVtbl;
    ret->ref = 1;

    hres = IActiveScript_QueryInterface(ACTSCRIPT(ret), riid, ppv);
    IActiveScript_Release(ACTSCRIPT(ret));
    return hres;
}
