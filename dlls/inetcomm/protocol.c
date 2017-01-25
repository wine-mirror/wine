/*
 * Copyright 2017 Jacek Caban for CodeWeavers
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

#define COBJMACROS

#include <assert.h>

#include "mimeole.h"
#include "inetcomm_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(inetcomm);

typedef struct {
    IInternetProtocol IInternetProtocol_iface;
    LONG ref;
} MimeHtmlProtocol;

static inline MimeHtmlProtocol *impl_from_IInternetProtocol(IInternetProtocol *iface)
{
    return CONTAINING_RECORD(iface, MimeHtmlProtocol, IInternetProtocol_iface);
}

static HRESULT WINAPI MimeHtmlProtocol_QueryInterface(IInternetProtocol *iface, REFIID riid, void **ppv)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", iface, ppv);
        *ppv = &This->IInternetProtocol_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocolRoot, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolRoot %p)\n", iface, ppv);
        *ppv = &This->IInternetProtocol_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocol, riid)) {
        TRACE("(%p)->(IID_IInternetProtocol %p)\n", iface, ppv);
        *ppv = &This->IInternetProtocol_iface;
    }else {
        FIXME("unknown interface %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI MimeHtmlProtocol_AddRef(IInternetProtocol *iface)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI MimeHtmlProtocol_Release(IInternetProtocol *iface)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%x\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI MimeHtmlProtocol_Start(IInternetProtocol *iface, const WCHAR *szUrl,
        IInternetProtocolSink* pOIProtSink, IInternetBindInfo* pOIBindInfo,
        DWORD grfPI, HANDLE_PTR dwReserved)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)->(%s %p %p %08x %lx)\n", This, debugstr_w(szUrl), pOIProtSink,
          pOIBindInfo, grfPI, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeHtmlProtocol_Continue(IInternetProtocol *iface, PROTOCOLDATA *pProtocolData)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)->(%p)\n", This, pProtocolData);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeHtmlProtocol_Abort(IInternetProtocol *iface, HRESULT hrReason, DWORD dwOptions)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)->(%08x %08x)\n", This, hrReason, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeHtmlProtocol_Terminate(IInternetProtocol *iface, DWORD dwOptions)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);
    TRACE("(%p)->(%08x)\n", This, dwOptions);
    return S_OK;
}

static HRESULT WINAPI MimeHtmlProtocol_Suspend(IInternetProtocol *iface)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeHtmlProtocol_Resume(IInternetProtocol *iface)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeHtmlProtocol_Read(IInternetProtocol *iface, void* pv, ULONG cb, ULONG* pcbRead)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)->(%p %u %p)\n", This, pv, cb, pcbRead);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeHtmlProtocol_Seek(IInternetProtocol *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)->(%d %d %p)\n", This, dlibMove.u.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeHtmlProtocol_LockRequest(IInternetProtocol *iface, DWORD dwOptions)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)->(%d)\n", This, dwOptions);
    return S_OK;
}

static HRESULT WINAPI MimeHtmlProtocol_UnlockRequest(IInternetProtocol *iface)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)\n", This);
    return S_OK;
}

static const IInternetProtocolVtbl MimeHtmlProtocolVtbl = {
    MimeHtmlProtocol_QueryInterface,
    MimeHtmlProtocol_AddRef,
    MimeHtmlProtocol_Release,
    MimeHtmlProtocol_Start,
    MimeHtmlProtocol_Continue,
    MimeHtmlProtocol_Abort,
    MimeHtmlProtocol_Terminate,
    MimeHtmlProtocol_Suspend,
    MimeHtmlProtocol_Resume,
    MimeHtmlProtocol_Read,
    MimeHtmlProtocol_Seek,
    MimeHtmlProtocol_LockRequest,
    MimeHtmlProtocol_UnlockRequest
};

HRESULT MimeHtmlProtocol_create(IUnknown *outer, void **obj)
{
    MimeHtmlProtocol *protocol;

    if(outer)
        FIXME("outer not supported\n");

    protocol = heap_alloc(sizeof(*protocol));
    if(!protocol)
        return E_OUTOFMEMORY;

    protocol->IInternetProtocol_iface.lpVtbl = &MimeHtmlProtocolVtbl;
    protocol->ref = 1;

    *obj = &protocol->IInternetProtocol_iface;
    return S_OK;
}
