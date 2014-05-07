/* 
 * DirectPlay8 Client
 * 
 * Copyright 2004 Raphael Junqueira
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
 *
 */

#include "config.h"

#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "objbase.h"
#include "wine/debug.h"

#include "dplay8.h"
#include "dpnet_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dpnet);

static inline IDirectPlay8ClientImpl *impl_from_IDirectPlay8Client(IDirectPlay8Client *iface)
{
    return CONTAINING_RECORD(iface, IDirectPlay8ClientImpl, IDirectPlay8Client_iface);
}

/* IDirectPlay8Client IUnknown parts follow: */
static HRESULT WINAPI IDirectPlay8ClientImpl_QueryInterface(IDirectPlay8Client *iface, REFIID riid,
        void **ppobj)
{
    IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirectPlay8Client)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return DPN_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirectPlay8ClientImpl_AddRef(IDirectPlay8Client *iface)
{
    IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirectPlay8ClientImpl_Release(IDirectPlay8Client *iface)
{
    IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref) {
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirectPlay8Client Interface follow: */

static HRESULT WINAPI IDirectPlay8ClientImpl_Initialize(IDirectPlay8Client *iface,
        void * const pvUserContext, const PFNDPNMESSAGEHANDLER pfn, const DWORD dwFlags)
{
    IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);

    TRACE("(%p):(%p,%p,%x)\n", This, pvUserContext, pfn, dwFlags);

    if(!pfn)
        return DPNERR_INVALIDPARAM;

    This->usercontext = pvUserContext;
    This->msghandler = pfn;
    This->flags = dwFlags;

    return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8ClientImpl_EnumServiceProviders(IDirectPlay8Client *iface,
        const GUID * const pguidServiceProvider, const GUID * const pguidApplication,
        DPN_SERVICE_PROVIDER_INFO * const pSPInfoBuffer, PDWORD const pcbEnumData,
        PDWORD const pcReturned, const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%x): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_EnumHosts(IDirectPlay8Client *iface,
        PDPN_APPLICATION_DESC const pApplicationDesc, IDirectPlay8Address * const pAddrHost,
        IDirectPlay8Address * const pDeviceInfo, void * const pUserEnumData,
        const DWORD dwUserEnumDataSize, const DWORD dwEnumCount, const DWORD dwRetryInterval,
        const DWORD dwTimeOut, void * const pvUserContext, DPNHANDLE * const pAsyncHandle,
        const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%p,%p,%x): Stub\n", This, pvUserContext, pAsyncHandle, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_CancelAsyncOperation(IDirectPlay8Client *iface,
        const DPNHANDLE hAsyncHandle, const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%u,%x): Stub\n", This, hAsyncHandle, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_Connect(IDirectPlay8Client *iface,
        const DPN_APPLICATION_DESC * const pdnAppDesc, IDirectPlay8Address * const pHostAddr,
        IDirectPlay8Address * const pDeviceInfo, const DPN_SECURITY_DESC * const pdnSecurity,
        const DPN_SECURITY_CREDENTIALS * const pdnCredentials, const void * const pvUserConnectData,
        const DWORD dwUserConnectDataSize, void * const pvAsyncContext,
        DPNHANDLE * const phAsyncHandle, const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%p,%p,%x): Stub\n", This, pvAsyncContext, phAsyncHandle, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_Send(IDirectPlay8Client *iface,
        const DPN_BUFFER_DESC * const prgBufferDesc, const DWORD cBufferDesc, const DWORD dwTimeOut,
        void * const pvAsyncContext, DPNHANDLE * const phAsyncHandle, const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%p,%p,%x): Stub\n", This, pvAsyncContext, phAsyncHandle, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_GetSendQueueInfo(IDirectPlay8Client *iface,
        DWORD * const pdwNumMsgs, DWORD * const pdwNumBytes, const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%x): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_GetApplicationDesc(IDirectPlay8Client *iface,
        DPN_APPLICATION_DESC * const pAppDescBuffer, DWORD * const pcbDataSize, const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%x): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_SetClientInfo(IDirectPlay8Client *iface,
        const DPN_PLAYER_INFO * const pdpnPlayerInfo, void * const pvAsyncContext,
        DPNHANDLE * const phAsyncHandle, const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%p,%p,%x): Stub\n", This, pvAsyncContext, phAsyncHandle, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_GetServerInfo(IDirectPlay8Client *iface,
        DPN_PLAYER_INFO * const pdpnPlayerInfo, DWORD * const pdwSize, const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%x): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_GetServerAddress(IDirectPlay8Client *iface,
        IDirectPlay8Address ** const pAddress, const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%x): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_Close(IDirectPlay8Client *iface, const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%x): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_ReturnBuffer(IDirectPlay8Client *iface,
        const DPNHANDLE hBufferHandle, const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%x): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_GetCaps(IDirectPlay8Client *iface,
        DPN_CAPS * const pdpCaps, const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%x): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_SetCaps(IDirectPlay8Client *iface,
        const DPN_CAPS * const pdpCaps, const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%x): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_SetSPCaps(IDirectPlay8Client *iface,
        const GUID * const pguidSP, const DPN_SP_CAPS * const pdpspCaps, const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%x): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_GetSPCaps(IDirectPlay8Client *iface,
        const GUID * const pguidSP, DPN_SP_CAPS * const pdpspCaps, const DWORD dwFlags)
{
    IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);

    TRACE("(%p)->(%p,%p,%x)\n", This, pguidSP, pdpspCaps, dwFlags);

    if(!This->msghandler)
        return DPNERR_UNINITIALIZED;

    if(pdpspCaps->dwSize != sizeof(DPN_SP_CAPS))
    {
        return DPNERR_INVALIDPARAM;
    }

    *pdpspCaps = This->spcaps;

    return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8ClientImpl_GetConnectionInfo(IDirectPlay8Client *iface,
        DPN_CONNECTION_INFO * const pdpConnectionInfo, const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%x): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_RegisterLobby(IDirectPlay8Client *iface,
        const DPNHANDLE dpnHandle,
        struct IDirectPlay8LobbiedApplication * const pIDP8LobbiedApplication, const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%x): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static const IDirectPlay8ClientVtbl DirectPlay8Client_Vtbl =
{
    IDirectPlay8ClientImpl_QueryInterface,
    IDirectPlay8ClientImpl_AddRef,
    IDirectPlay8ClientImpl_Release,
    IDirectPlay8ClientImpl_Initialize,
    IDirectPlay8ClientImpl_EnumServiceProviders,
    IDirectPlay8ClientImpl_EnumHosts,
    IDirectPlay8ClientImpl_CancelAsyncOperation,
    IDirectPlay8ClientImpl_Connect,
    IDirectPlay8ClientImpl_Send,
    IDirectPlay8ClientImpl_GetSendQueueInfo,
    IDirectPlay8ClientImpl_GetApplicationDesc,
    IDirectPlay8ClientImpl_SetClientInfo,
    IDirectPlay8ClientImpl_GetServerInfo,
    IDirectPlay8ClientImpl_GetServerAddress,
    IDirectPlay8ClientImpl_Close,
    IDirectPlay8ClientImpl_ReturnBuffer,
    IDirectPlay8ClientImpl_GetCaps,
    IDirectPlay8ClientImpl_SetCaps,
    IDirectPlay8ClientImpl_SetSPCaps,
    IDirectPlay8ClientImpl_GetSPCaps,
    IDirectPlay8ClientImpl_GetConnectionInfo,
    IDirectPlay8ClientImpl_RegisterLobby
};

HRESULT DPNET_CreateDirectPlay8Client(IClassFactory *iface, IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    IDirectPlay8ClientImpl* client;
    HRESULT hr;

    TRACE("(%p, %s, %p)\n", pUnkOuter, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if(pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    client = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectPlay8ClientImpl));
    if (!client)
        return E_OUTOFMEMORY;

    client->IDirectPlay8Client_iface.lpVtbl = &DirectPlay8Client_Vtbl;
    client->ref = 1;

    init_dpn_sp_caps(&client->spcaps);

    hr = IDirectPlay8ClientImpl_QueryInterface(&client->IDirectPlay8Client_iface, riid, ppv);
    IDirectPlay8ClientImpl_Release(&client->IDirectPlay8Client_iface);

    return hr;
}
