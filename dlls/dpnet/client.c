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

/* IDirectPlay8Client IUnknown parts follow: */
static HRESULT WINAPI IDirectPlay8ClientImpl_QueryInterface(PDIRECTPLAY8CLIENT iface, REFIID riid, LPVOID *ppobj)
{
    IDirectPlay8ClientImpl *This = (IDirectPlay8ClientImpl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirectPlay8Client)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return DPN_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirectPlay8ClientImpl_AddRef(PDIRECTPLAY8CLIENT iface) {
    IDirectPlay8ClientImpl *This = (IDirectPlay8ClientImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IDirectPlay8ClientImpl_Release(PDIRECTPLAY8CLIENT iface) {
    IDirectPlay8ClientImpl *This = (IDirectPlay8ClientImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n", This, refCount + 1);

    if (!refCount) {
        HeapFree(GetProcessHeap(), 0, This);
    }
    return refCount;
}

/* IDirectPlay8Client Interface follow: */

static HRESULT WINAPI IDirectPlay8ClientImpl_Initialize(PDIRECTPLAY8CLIENT iface,  PVOID CONST pvUserContext, CONST PFNDPNMESSAGEHANDLER pfn, CONST DWORD dwFlags) { 
  IDirectPlay8ClientImpl *This = (IDirectPlay8ClientImpl *)iface;
  FIXME("(%p):(%p,%p,%x): Stub\n", This, pvUserContext, pfn, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_EnumServiceProviders(PDIRECTPLAY8CLIENT iface, 
							   CONST GUID * CONST pguidServiceProvider, 
							   CONST GUID * CONST pguidApplication, 
							   DPN_SERVICE_PROVIDER_INFO * CONST pSPInfoBuffer, 
							   PDWORD CONST pcbEnumData, 
							   PDWORD CONST pcReturned, 
							   CONST DWORD dwFlags) { 
  IDirectPlay8ClientImpl *This = (IDirectPlay8ClientImpl *)iface;
  FIXME("(%p):(%x): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_EnumHosts(PDIRECTPLAY8CLIENT iface, 
						PDPN_APPLICATION_DESC CONST pApplicationDesc, 
						IDirectPlay8Address * CONST pAddrHost, 
						IDirectPlay8Address * CONST pDeviceInfo, 
						PVOID CONST pUserEnumData, CONST DWORD dwUserEnumDataSize, CONST DWORD dwEnumCount, 
						CONST DWORD dwRetryInterval, 
						CONST DWORD dwTimeOut, 
						PVOID CONST pvUserContext, 
						DPNHANDLE * CONST pAsyncHandle, 
						CONST DWORD dwFlags) { 
  IDirectPlay8ClientImpl *This = (IDirectPlay8ClientImpl *)iface;
  /*FIXME("(%p):(%p,%p,%p,%p,%lu,%lu,%lu,%lu): Stub\n", This, pApplicationDesc, pAddrHost, pDeviceInfo, pUserEnumData, dwUserEnumDataSize, dwEnumCount, dwRetryInterval, dwTimeOut);*/
  FIXME("(%p):(%p,%p,%x): Stub\n", This, pvUserContext, pAsyncHandle, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_CancelAsyncOperation(PDIRECTPLAY8CLIENT iface, CONST DPNHANDLE hAsyncHandle, CONST DWORD dwFlags) { 
  IDirectPlay8ClientImpl *This = (IDirectPlay8ClientImpl *)iface;
  FIXME("(%p):(%u,%x): Stub\n", This, hAsyncHandle, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_Connect(PDIRECTPLAY8CLIENT iface, 
					      CONST DPN_APPLICATION_DESC * CONST pdnAppDesc,
					      IDirectPlay8Address * CONST pHostAddr,
					      IDirectPlay8Address * CONST pDeviceInfo, 
					      CONST DPN_SECURITY_DESC * CONST pdnSecurity, 
					      CONST DPN_SECURITY_CREDENTIALS * CONST pdnCredentials, 
					      CONST void * CONST pvUserConnectData, 
					      CONST DWORD dwUserConnectDataSize, 
					      void * CONST pvAsyncContext, 
					      DPNHANDLE * CONST phAsyncHandle, 
					      CONST DWORD dwFlags) { 
  IDirectPlay8ClientImpl *This = (IDirectPlay8ClientImpl *)iface;
  FIXME("(%p):(%p,%p,%x): Stub\n", This, pvAsyncContext, phAsyncHandle, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_Send(PDIRECTPLAY8CLIENT iface, 
					   CONST DPN_BUFFER_DESC * CONST prgBufferDesc, 
					   CONST DWORD cBufferDesc, 
					   CONST DWORD dwTimeOut, 
					   void * CONST pvAsyncContext, 
					   DPNHANDLE * CONST phAsyncHandle, 
					   CONST DWORD dwFlags) { 
  IDirectPlay8ClientImpl *This = (IDirectPlay8ClientImpl *)iface;
  FIXME("(%p):(%p,%p,%x): Stub\n", This, pvAsyncContext, phAsyncHandle, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_GetSendQueueInfo(PDIRECTPLAY8CLIENT iface, DWORD * CONST pdwNumMsgs, DWORD * CONST pdwNumBytes, CONST DWORD dwFlags) {
  IDirectPlay8ClientImpl *This = (IDirectPlay8ClientImpl *)iface;
  FIXME("(%p):(%x): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_GetApplicationDesc(PDIRECTPLAY8CLIENT iface, DPN_APPLICATION_DESC * CONST pAppDescBuffer, DWORD * CONST pcbDataSize, CONST DWORD dwFlags) { 
  IDirectPlay8ClientImpl *This = (IDirectPlay8ClientImpl *)iface;
  FIXME("(%p):(%x): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_SetClientInfo(PDIRECTPLAY8CLIENT iface, 
						    CONST DPN_PLAYER_INFO * CONST pdpnPlayerInfo, 
						    PVOID CONST pvAsyncContext, 
						    DPNHANDLE * CONST phAsyncHandle, 
						    CONST DWORD dwFlags) { 
  IDirectPlay8ClientImpl *This = (IDirectPlay8ClientImpl *)iface;
  FIXME("(%p):(%p,%p,%x): Stub\n", This, pvAsyncContext, phAsyncHandle, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_GetServerInfo(PDIRECTPLAY8CLIENT iface, DPN_PLAYER_INFO * CONST pdpnPlayerInfo, DWORD * CONST pdwSize, CONST DWORD dwFlags) { 
  IDirectPlay8ClientImpl *This = (IDirectPlay8ClientImpl *)iface;
  FIXME("(%p):(%x): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_GetServerAddress(PDIRECTPLAY8CLIENT iface, IDirectPlay8Address ** CONST pAddress, CONST DWORD dwFlags) { 
  IDirectPlay8ClientImpl *This = (IDirectPlay8ClientImpl *)iface;
  FIXME("(%p):(%x): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_Close(PDIRECTPLAY8CLIENT iface, CONST DWORD dwFlags) { 
  IDirectPlay8ClientImpl *This = (IDirectPlay8ClientImpl *)iface;
  FIXME("(%p):(%x): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_ReturnBuffer(PDIRECTPLAY8CLIENT iface, CONST DPNHANDLE hBufferHandle, CONST DWORD dwFlags) { 
  IDirectPlay8ClientImpl *This = (IDirectPlay8ClientImpl *)iface;
  FIXME("(%p):(%x): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_GetCaps(PDIRECTPLAY8CLIENT iface, DPN_CAPS * CONST pdpCaps, CONST DWORD dwFlags) { 
  IDirectPlay8ClientImpl *This = (IDirectPlay8ClientImpl *)iface;
  FIXME("(%p):(%x): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_SetCaps(PDIRECTPLAY8CLIENT iface, CONST DPN_CAPS * CONST pdpCaps, CONST DWORD dwFlags) { 
  IDirectPlay8ClientImpl *This = (IDirectPlay8ClientImpl *)iface;
  FIXME("(%p):(%x): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_SetSPCaps(PDIRECTPLAY8CLIENT iface, CONST GUID * CONST pguidSP, CONST DPN_SP_CAPS * CONST pdpspCaps, CONST DWORD dwFlags ) { 
  IDirectPlay8ClientImpl *This = (IDirectPlay8ClientImpl *)iface;
  FIXME("(%p):(%x): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_GetSPCaps(PDIRECTPLAY8CLIENT iface, CONST GUID * CONST pguidSP, DPN_SP_CAPS * CONST pdpspCaps, CONST DWORD dwFlags) { 
  IDirectPlay8ClientImpl *This = (IDirectPlay8ClientImpl *)iface;
  FIXME("(%p):(%x): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_GetConnectionInfo(PDIRECTPLAY8CLIENT iface, DPN_CONNECTION_INFO * CONST pdpConnectionInfo, CONST DWORD dwFlags) { 
  IDirectPlay8ClientImpl *This = (IDirectPlay8ClientImpl *)iface;
  FIXME("(%p):(%x): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_RegisterLobby(PDIRECTPLAY8CLIENT iface, CONST DPNHANDLE dpnHandle, struct IDirectPlay8LobbiedApplication * CONST pIDP8LobbiedApplication, CONST DWORD dwFlags) { 
  IDirectPlay8ClientImpl *This = (IDirectPlay8ClientImpl *)iface;
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

HRESULT DPNET_CreateDirectPlay8Client(LPCLASSFACTORY iface, LPUNKNOWN punkOuter, REFIID riid, LPVOID *ppobj) {
  IDirectPlay8ClientImpl* client;

  TRACE("(%p, %s, %p)\n", punkOuter, debugstr_guid(riid), ppobj);
  
  client = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectPlay8ClientImpl));
  if (NULL == client) {
    *ppobj = NULL;
    return E_OUTOFMEMORY;
  }
  client->lpVtbl = &DirectPlay8Client_Vtbl;
  client->ref = 0; /* will be inited with QueryInterface */
  return IDirectPlay8ClientImpl_QueryInterface ((PDIRECTPLAY8CLIENT)client, riid, ppobj);
}
