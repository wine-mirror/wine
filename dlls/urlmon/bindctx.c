/*
 * Copyright 2007 Jacek Caban for CodeWeavers
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
#include <stdio.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "winuser.h"
#include "ole2.h"
#include "urlmon.h"
#include "urlmon_main.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

typedef struct {
    const IBindCtxVtbl *lpBindCtxVtbl;

    LONG ref;

    IBindCtx *bindctx;
} AsyncBindCtx;

#define BINDCTX(x)  ((IBindCtx*)  &(x)->lpBindCtxVtbl)

#define BINDCTX_THIS(iface) DEFINE_THIS(AsyncBindCtx, BindCtx, iface)

static HRESULT WINAPI AsyncBindCtx_QueryInterface(IBindCtx *iface, REFIID riid, void **ppv)
{
    AsyncBindCtx *This = BINDCTX_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = BINDCTX(This);
    }else if(IsEqualGUID(riid, &IID_IBindCtx)) {
        TRACE("(%p)->(IID_IBindCtx %p)\n", This, ppv);
        *ppv = BINDCTX(This);
    }else if(IsEqualGUID(riid, &IID_IAsyncBindCtx)) {
        TRACE("(%p)->(IID_IAsyncBindCtx %p)\n", This, ppv);
        *ppv = BINDCTX(This);
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI AsyncBindCtx_AddRef(IBindCtx *iface)
{
    AsyncBindCtx *This = BINDCTX_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI AsyncBindCtx_Release(IBindCtx *iface)
{
    AsyncBindCtx *This = BINDCTX_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        IBindCtx_Release(This->bindctx);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI AsyncBindCtx_RegisterObjectBound(IBindCtx *iface, IUnknown *punk)
{
    AsyncBindCtx *This = BINDCTX_THIS(iface);

    TRACE("(%p)->(%p)\n", This, punk);

    return IBindCtx_RegisterObjectBound(This->bindctx, punk);
}

static HRESULT WINAPI AsyncBindCtx_RevokeObjectBound(IBindCtx *iface, IUnknown *punk)
{
    AsyncBindCtx *This = BINDCTX_THIS(iface);

    TRACE("(%p %p)\n", This, punk);

    return IBindCtx_RevokeObjectBound(This->bindctx, punk);
}

static HRESULT WINAPI AsyncBindCtx_ReleaseBoundObjects(IBindCtx *iface)
{
    AsyncBindCtx *This = BINDCTX_THIS(iface);

    TRACE("(%p)\n", This);

    return IBindCtx_ReleaseBoundObjects(This->bindctx);
}

static HRESULT WINAPI AsyncBindCtx_SetBindOptions(IBindCtx *iface, BIND_OPTS *pbindopts)
{
    AsyncBindCtx *This = BINDCTX_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pbindopts);

    return IBindCtx_SetBindOptions(This->bindctx, pbindopts);
}

static HRESULT WINAPI AsyncBindCtx_GetBindOptions(IBindCtx *iface, BIND_OPTS *pbindopts)
{
    AsyncBindCtx *This = BINDCTX_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pbindopts);

    return IBindCtx_GetBindOptions(This->bindctx, pbindopts);
}

static HRESULT WINAPI AsyncBindCtx_GetRunningObjectTable(IBindCtx *iface, IRunningObjectTable **pprot)
{
    AsyncBindCtx *This = BINDCTX_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pprot);

    return IBindCtx_GetRunningObjectTable(This->bindctx, pprot);
}

static HRESULT WINAPI AsyncBindCtx_RegisterObjectParam(IBindCtx *iface, LPOLESTR pszkey, IUnknown *punk)
{
    AsyncBindCtx *This = BINDCTX_THIS(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(pszkey), punk);

    return IBindCtx_RegisterObjectParam(This->bindctx, pszkey, punk);
}

static HRESULT WINAPI AsyncBindCtx_GetObjectParam(IBindCtx* iface, LPOLESTR pszkey, IUnknown **punk)
{
    AsyncBindCtx *This = BINDCTX_THIS(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(pszkey), punk);

    return IBindCtx_GetObjectParam(This->bindctx, pszkey, punk);
}

static HRESULT WINAPI AsyncBindCtx_RevokeObjectParam(IBindCtx *iface, LPOLESTR ppenum)
{
    AsyncBindCtx *This = BINDCTX_THIS(iface);

    TRACE("(%p)->(%p)\n", This, ppenum);

    return IBindCtx_RevokeObjectParam(This->bindctx, ppenum);
}

static HRESULT WINAPI AsyncBindCtx_EnumObjectParam(IBindCtx *iface, IEnumString **pszkey)
{
    AsyncBindCtx *This = BINDCTX_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pszkey);

    return IBindCtx_EnumObjectParam(This->bindctx, pszkey);
}

#undef BINDCTX_THIS

static const IBindCtxVtbl AsyncBindCtxVtbl =
{
    AsyncBindCtx_QueryInterface,
    AsyncBindCtx_AddRef,
    AsyncBindCtx_Release,
    AsyncBindCtx_RegisterObjectBound,
    AsyncBindCtx_RevokeObjectBound,
    AsyncBindCtx_ReleaseBoundObjects,
    AsyncBindCtx_SetBindOptions,
    AsyncBindCtx_GetBindOptions,
    AsyncBindCtx_GetRunningObjectTable,
    AsyncBindCtx_RegisterObjectParam,
    AsyncBindCtx_GetObjectParam,
    AsyncBindCtx_EnumObjectParam,
    AsyncBindCtx_RevokeObjectParam
};

static HRESULT init_bindctx(IBindCtx *bindctx, DWORD options,
       IBindStatusCallback *callback, IEnumFORMATETC *format)
{
    BIND_OPTS bindopts;
    HRESULT hres;

    if(options)
        FIXME("not supported options %08x\n", options);
    if(format)
        FIXME("format is not supported\n");

    bindopts.cbStruct = sizeof(BIND_OPTS);
    bindopts.grfFlags = BIND_MAYBOTHERUSER;
    bindopts.grfMode = STGM_READWRITE | STGM_SHARE_EXCLUSIVE;
    bindopts.dwTickCountDeadline = 0;

    hres = IBindCtx_SetBindOptions(bindctx, &bindopts);
    if(FAILED(hres))
       return hres;

    if(callback) {
        hres = RegisterBindStatusCallback(bindctx, callback, NULL, 0);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

/***********************************************************************
 *           CreateAsyncBindCtx (urlmon.@)
 */
HRESULT WINAPI CreateAsyncBindCtx(DWORD reserved, IBindStatusCallback *callback,
        IEnumFORMATETC *format, IBindCtx **pbind)
{
    IBindCtx *bindctx;
    HRESULT hres;

    TRACE("(%08x %p %p %p)\n", reserved, callback, format, pbind);

    if(!pbind || !callback)
        return E_INVALIDARG;

    hres = CreateBindCtx(0, &bindctx);
    if(FAILED(hres))
        return hres;

    hres = init_bindctx(bindctx, 0, callback, format);
    if(FAILED(hres)) {
        IBindCtx_Release(bindctx);
        return hres;
    }

    *pbind = bindctx;
    return S_OK;
}

/***********************************************************************
 *           CreateAsyncBindCtxEx (urlmon.@)
 *
 * Create an asynchronous bind context.
 */
HRESULT WINAPI CreateAsyncBindCtxEx(IBindCtx *ibind, DWORD options,
        IBindStatusCallback *callback, IEnumFORMATETC *format, IBindCtx** pbind,
        DWORD reserved)
{
    AsyncBindCtx *ret;
    IBindCtx *bindctx;
    HRESULT hres;

    TRACE("(%p %08x %p %p %p %d)\n", ibind, options, callback, format, pbind, reserved);

    if(!pbind)
        return E_INVALIDARG;

    if(reserved)
        WARN("reserved=%d\n", reserved);

    hres = CreateBindCtx(0, &bindctx);
    if(FAILED(hres))
        return hres;

    ret = HeapAlloc(GetProcessHeap(), 0, sizeof(AsyncBindCtx));

    ret->lpBindCtxVtbl = &AsyncBindCtxVtbl;
    ret->ref = 1;
    ret->bindctx = bindctx;

    hres = init_bindctx(BINDCTX(ret), options, callback, format);
    if(FAILED(hres)) {
        IBindCtx_Release(BINDCTX(ret));
        return hres;
    }

    *pbind = BINDCTX(ret);
    return S_OK;
}
