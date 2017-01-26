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
    IUnknown IUnknown_inner;
    IInternetProtocol IInternetProtocol_iface;
    IInternetProtocolInfo IInternetProtocolInfo_iface;

    LONG ref;
    IUnknown *outer_unk;
} MimeHtmlProtocol;

static inline MimeHtmlProtocol *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, MimeHtmlProtocol, IUnknown_inner);
}

static HRESULT WINAPI MimeHtmlProtocol_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    MimeHtmlProtocol *This = impl_from_IUnknown(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IInternetProtocol_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocolRoot, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolRoot %p)\n", This, ppv);
        *ppv = &This->IInternetProtocol_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocol, riid)) {
        TRACE("(%p)->(IID_IInternetProtocol %p)\n", This, ppv);
        *ppv = &This->IInternetProtocol_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocolInfo, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolInfo %p)\n", This, ppv);
        *ppv = &This->IInternetProtocolInfo_iface;
    }else {
        FIXME("unknown interface %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI MimeHtmlProtocol_AddRef(IUnknown *iface)
{
    MimeHtmlProtocol *This = impl_from_IUnknown(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI MimeHtmlProtocol_Release(IUnknown *iface)
{
    MimeHtmlProtocol *This = impl_from_IUnknown(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%x\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static const IUnknownVtbl MimeHtmlProtocolInnerVtbl = {
    MimeHtmlProtocol_QueryInterface,
    MimeHtmlProtocol_AddRef,
    MimeHtmlProtocol_Release
};

static inline MimeHtmlProtocol *impl_from_IInternetProtocol(IInternetProtocol *iface)
{
    return CONTAINING_RECORD(iface, MimeHtmlProtocol, IInternetProtocol_iface);
}

static HRESULT WINAPI InternetProtocol_QueryInterface(IInternetProtocol *iface, REFIID riid, void **ppv)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI InternetProtocol_AddRef(IInternetProtocol *iface)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI InternetProtocol_Release(IInternetProtocol *iface)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);
    return IUnknown_Release(This->outer_unk);
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
    InternetProtocol_QueryInterface,
    InternetProtocol_AddRef,
    InternetProtocol_Release,
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

static inline MimeHtmlProtocol *impl_from_IInternetProtocolInfo(IInternetProtocolInfo *iface)
{
    return CONTAINING_RECORD(iface, MimeHtmlProtocol, IInternetProtocolInfo_iface);
}

static HRESULT WINAPI MimeHtmlProtocolInfo_QueryInterface(IInternetProtocolInfo *iface, REFIID riid, void **ppv)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocolInfo(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI MimeHtmlProtocolInfo_AddRef(IInternetProtocolInfo *iface)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocolInfo(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI MimeHtmlProtocolInfo_Release(IInternetProtocolInfo *iface)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocolInfo(iface);
    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI MimeHtmlProtocolInfo_ParseUrl(IInternetProtocolInfo *iface, LPCWSTR pwzUrl,
        PARSEACTION ParseAction, DWORD dwParseFlags, LPWSTR pwzResult, DWORD cchResult,
        DWORD* pcchResult, DWORD dwReserved)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocolInfo(iface);
    FIXME("(%p)->(%s %d %x %p %d %p %d)\n", This, debugstr_w(pwzUrl), ParseAction,
          dwParseFlags, pwzResult, cchResult, pcchResult, dwReserved);
    return INET_E_DEFAULT_ACTION;
}

static HRESULT WINAPI MimeHtmlProtocolInfo_CombineUrl(IInternetProtocolInfo *iface,
        LPCWSTR pwzBaseUrl, LPCWSTR pwzRelativeUrl, DWORD dwCombineFlags, LPWSTR pwzResult,
        DWORD cchResult, DWORD* pcchResult, DWORD dwReserved)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocolInfo(iface);
    FIXME("(%p)->(%s %s %08x %p %d %p %d)\n", This, debugstr_w(pwzBaseUrl),
          debugstr_w(pwzRelativeUrl), dwCombineFlags, pwzResult, cchResult,
          pcchResult, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeHtmlProtocolInfo_CompareUrl(IInternetProtocolInfo *iface, LPCWSTR pwzUrl1,
        LPCWSTR pwzUrl2, DWORD dwCompareFlags)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocolInfo(iface);
    FIXME("(%p)->(%s %s %08x)\n", This, debugstr_w(pwzUrl1), debugstr_w(pwzUrl2), dwCompareFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeHtmlProtocolInfo_QueryInfo(IInternetProtocolInfo *iface, LPCWSTR pwzUrl,
        QUERYOPTION QueryOption, DWORD dwQueryFlags, LPVOID pBuffer, DWORD cbBuffer, DWORD* pcbBuf,
        DWORD dwReserved)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocolInfo(iface);
    FIXME("(%p)->(%s %08x %08x %p %d %p %d)\n", This, debugstr_w(pwzUrl), QueryOption, dwQueryFlags, pBuffer,
          cbBuffer, pcbBuf, dwReserved);
    return INET_E_USE_DEFAULT_PROTOCOLHANDLER;
}

static const IInternetProtocolInfoVtbl MimeHtmlProtocolInfoVtbl = {
    MimeHtmlProtocolInfo_QueryInterface,
    MimeHtmlProtocolInfo_AddRef,
    MimeHtmlProtocolInfo_Release,
    MimeHtmlProtocolInfo_ParseUrl,
    MimeHtmlProtocolInfo_CombineUrl,
    MimeHtmlProtocolInfo_CompareUrl,
    MimeHtmlProtocolInfo_QueryInfo
};

HRESULT MimeHtmlProtocol_create(IUnknown *outer, void **obj)
{
    MimeHtmlProtocol *protocol;

    protocol = heap_alloc(sizeof(*protocol));
    if(!protocol)
        return E_OUTOFMEMORY;

    protocol->IUnknown_inner.lpVtbl = &MimeHtmlProtocolInnerVtbl;
    protocol->IInternetProtocol_iface.lpVtbl = &MimeHtmlProtocolVtbl;
    protocol->IInternetProtocolInfo_iface.lpVtbl = &MimeHtmlProtocolInfoVtbl;
    protocol->ref = 1;
    protocol->outer_unk = outer ? outer : &protocol->IUnknown_inner;

    *obj = &protocol->IUnknown_inner;
    return S_OK;
}
