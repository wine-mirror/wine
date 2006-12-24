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
#include "urlmon.h"
#include "itsstor.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(itss);

typedef struct {
    const IInternetProtocolVtbl  *lpInternetProtocolVtbl;
    LONG ref;
} ITSProtocol;

#define PROTOCOL_THIS(iface) DEFINE_THIS(ITSProtocol, InternetProtocol, iface)

#define PROTOCOL(x)  ((IInternetProtocol*)  &(x)->lpInternetProtocolVtbl)

static HRESULT WINAPI ITSProtocol_QueryInterface(IInternetProtocol *iface, REFIID riid, void **ppv)
{
    ITSProtocol *This = PROTOCOL_THIS(iface);

    *ppv = NULL;
    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = PROTOCOL(This);
    }else if(IsEqualGUID(&IID_IInternetProtocolRoot, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolRoot %p)\n", This, ppv);
        *ppv = PROTOCOL(This);
    }else if(IsEqualGUID(&IID_IInternetProtocol, riid)) {
        TRACE("(%p)->(IID_IInternetProtocol %p)\n", This, ppv);
        *ppv = PROTOCOL(This);
    }

    if(*ppv) {
        IInternetProtocol_AddRef(iface);
        return S_OK;
    }

    WARN("not supported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ITSProtocol_AddRef(IInternetProtocol *iface)
{
    ITSProtocol *This = PROTOCOL_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%d\n", This, ref);
    return ref;
}

static ULONG WINAPI ITSProtocol_Release(IInternetProtocol *iface)
{
    ITSProtocol *This = PROTOCOL_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        HeapFree(GetProcessHeap(), 0, This);
        ITSS_UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI ITSProtocol_Start(IInternetProtocol *iface, LPCWSTR szUrl,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, DWORD dwReserved)
{
    ITSProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%s %p %p %08x %d)\n", This, debugstr_w(szUrl), pOIProtSink,
            pOIBindInfo, grfPI, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITSProtocol_Continue(IInternetProtocol *iface, PROTOCOLDATA *pProtocolData)
{
    ITSProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pProtocolData);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITSProtocol_Abort(IInternetProtocol *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    ITSProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%08x %08x)\n", This, hrReason, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITSProtocol_Terminate(IInternetProtocol *iface, DWORD dwOptions)
{
    ITSProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%08x)\n", This, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITSProtocol_Suspend(IInternetProtocol *iface)
{
    ITSProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITSProtocol_Resume(IInternetProtocol *iface)
{
    ITSProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITSProtocol_Read(IInternetProtocol *iface, void *pv,
        ULONG cb, ULONG *pcbRead)
{
    ITSProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%p %u %p)\n", This, pv, cb, pcbRead);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITSProtocol_Seek(IInternetProtocol *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrgin, ULARGE_INTEGER *plibNewPosition)
{
    ITSProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%d %d %p)\n", This, dlibMove.u.LowPart, dwOrgin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITSProtocol_LockRequest(IInternetProtocol *iface, DWORD dwOptions)
{
    ITSProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%08x)\n", This, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITSProtocol_UnlockRequest(IInternetProtocol *iface)
{
    ITSProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

#undef PROTOCOL_THIS

static const IInternetProtocolVtbl ITSProtocolVtbl = {
    ITSProtocol_QueryInterface,
    ITSProtocol_AddRef,
    ITSProtocol_Release,
    ITSProtocol_Start,
    ITSProtocol_Continue,
    ITSProtocol_Abort,
    ITSProtocol_Terminate,
    ITSProtocol_Suspend,
    ITSProtocol_Resume,
    ITSProtocol_Read,
    ITSProtocol_Seek,
    ITSProtocol_LockRequest,
    ITSProtocol_UnlockRequest
};

HRESULT ITSProtocol_create(IUnknown *pUnkOuter, LPVOID *ppobj)
{
    ITSProtocol *ret;

    TRACE("(%p %p)\n", pUnkOuter, ppobj);

    ITSS_LockModule();

    ret = HeapAlloc(GetProcessHeap(), 0, sizeof(ITSProtocol));

    ret->lpInternetProtocolVtbl = &ITSProtocolVtbl;
    ret->ref = 1;

    *ppobj = PROTOCOL(ret);

    return S_OK;
}
