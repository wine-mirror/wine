/* 
 * DirectPlay8 Peer
 * 
 * Copyright 2004 Raphael Junqueira
 * Copyright 2008 Alexander N. Sørnes <alex@thehandofagony.com>
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

/* IUnknown interface follows */

static HRESULT WINAPI IDirectPlay8PeerImpl_QueryInterface(PDIRECTPLAY8PEER iface, REFIID riid, LPVOID* ppobj)
{
    IDirectPlay8PeerImpl* This = (IDirectPlay8PeerImpl*)iface;

    if(IsEqualGUID(riid, &IID_IUnknown) ||
       IsEqualGUID(riid, &IID_IDirectPlay8Peer))
    {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return DPN_OK;
    }

    WARN("(%p)->(%s,%p): not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirectPlay8PeerImpl_AddRef(PDIRECTPLAY8PEER iface)
{
    IDirectPlay8PeerImpl* This = (IDirectPlay8PeerImpl*)iface;
    ULONG RefCount = InterlockedIncrement(&This->ref);

    return RefCount;
}

static ULONG WINAPI IDirectPlay8PeerImpl_Release(PDIRECTPLAY8PEER iface)
{
    IDirectPlay8PeerImpl* This = (IDirectPlay8PeerImpl*)iface;
    ULONG RefCount = InterlockedDecrement(&This->ref);

    if(!RefCount)
        HeapFree(GetProcessHeap(), 0, This);

    return RefCount;
}

/* IDirectPlay8Peer interface follows */

static HRESULT WINAPI IDirectPlay8PeerImpl_Initialize(PDIRECTPLAY8PEER iface, PVOID CONST pvUserContext, CONST PFNDPNMESSAGEHANDLER pfn, CONST DWORD dwFlags)
{
    FIXME("(%p)->(%p,%p,%x): stub\n", iface, pvUserContext, pfn, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_EnumServiceProviders(PDIRECTPLAY8PEER iface, CONST GUID *CONST pguidServiceProvider, CONST GUID *CONST pguidApplication, DPN_SERVICE_PROVIDER_INFO *CONST pSPInfoBuffer, DWORD *CONST pcbEnumData, DWORD *CONST pcReturned, CONST DWORD dwFlags)
{
    FIXME("(%p)->(%p,%p,%p,%p,%p,%x): stub\n", iface, pguidServiceProvider, pguidApplication, pSPInfoBuffer, pcbEnumData, pcReturned, dwFlags);
    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_CancelAsyncOperation(PDIRECTPLAY8PEER iface,
                                                                CONST DPNHANDLE hAsyncHandle,
                                                                CONST DWORD dwFlags)
{
    FIXME("(%p)->(%x,%x): stub\n", iface, hAsyncHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_Connect(PDIRECTPLAY8PEER iface,
                                                   CONST DPN_APPLICATION_DESC* CONST pdnAppDesc,
                                                   IDirectPlay8Address *CONST pHostAddr,
                                                   IDirectPlay8Address *CONST pDeviceInfo,
                                                   CONST DPN_SECURITY_DESC *CONST pdnSecurity,
                                                   CONST DPN_SECURITY_CREDENTIALS *CONST pdnCredentials,
                                                   CONST VOID *CONST pvUserConnectData,
                                                   CONST DWORD dwUserConnectDataSize,
                                                   VOID *CONST pvPlayerContext,
                                                   VOID *CONST pvAsyncContext,
                                                   DPNHANDLE *CONST phAsyncHandle,
                                                   CONST DWORD dwFlags)
{
    FIXME("(%p)->(%p,%p,%p,%p,%p,%p,%x,%p,%p,%p,%x): stub\n", iface, pdnAppDesc, pHostAddr, pDeviceInfo, pdnSecurity, pdnCredentials, pvUserConnectData, dwUserConnectDataSize, pvPlayerContext, pvAsyncContext, phAsyncHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_SendTo(PDIRECTPLAY8PEER iface,
                                                  CONST DPNID dpnId,
                                                  CONST DPN_BUFFER_DESC *pBufferDesc,
                                                  CONST DWORD cBufferDesc,
                                                  CONST DWORD dwTimeOut,
                                                  VOID *CONST pvAsyncContext,
                                                  DPNHANDLE *CONST phAsyncHandle,
                                                  CONST DWORD dwFlags)
{
    FIXME("(%p)->(%x,%p,%x,%x,%p,%p,%x): stub\n", iface, dpnId, pBufferDesc, cBufferDesc, dwTimeOut, pvAsyncContext, phAsyncHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_GetSendQueueInfo(PDIRECTPLAY8PEER iface,
                                                            CONST DPNID dpnid,
                                                            DWORD *CONST pdwNumMsgs,
                                                            DWORD *CONST pdwNumBytes,
                                                            CONST DWORD dwFlags)
{
    FIXME("(%p)->(%x,%p,%p,%x): stub\n", iface, dpnid, pdwNumMsgs, pdwNumBytes, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_Host(PDIRECTPLAY8PEER iface,
                                                CONST DPN_APPLICATION_DESC *CONST pdnAppDesc,
                                                IDirectPlay8Address **CONST prgpDeviceInfo,
                                                CONST DWORD cDeviceInfo,
                                                CONST DPN_SECURITY_DESC *CONST pdpSecurity,
                                                CONST DPN_SECURITY_CREDENTIALS *CONST pdpCredentials,
                                                VOID *CONST pvPlayerContext,
                                                CONST DWORD dwFlags)
{
    FIXME("(%p)->(%p,%p,%x,%p,%p,%p,%x): stub\n", iface, pdnAppDesc, prgpDeviceInfo, cDeviceInfo, pdpSecurity, pdpCredentials, pvPlayerContext, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_GetApplicationDesc(PDIRECTPLAY8PEER iface,
                                                              DPN_APPLICATION_DESC *CONST pAppDescBuffer,
                                                              DWORD *CONST pcbDataSize,
                                                              CONST DWORD dwFlags)
{
    FIXME("(%p)->(%p,%p,%x): stub\n", iface, pAppDescBuffer, pcbDataSize, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_SetApplicationDesc(PDIRECTPLAY8PEER iface,
                                                              CONST DPN_APPLICATION_DESC *CONST pad,
                                                              CONST DWORD dwFlags)
{
    FIXME("(%p)->(%p,%x): stub\n", iface, pad, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_CreateGroup(PDIRECTPLAY8PEER iface,
                                                       CONST DPN_GROUP_INFO *CONST pdpnGroupInfo,
                                                       VOID *CONST pvGroupContext,
                                                       VOID *CONST pvAsyncContext,
                                                       DPNHANDLE *CONST phAsyncHandle,
                                                       CONST DWORD dwFlags)
{
    FIXME("(%p)->(%p,%p,%p,%p,%x): stub\n", iface, pdpnGroupInfo, pvGroupContext, pvAsyncContext, phAsyncHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_DestroyGroup(PDIRECTPLAY8PEER iface,
                                                        CONST DPNID idGroup,
                                                        PVOID CONST pvAsyncContext,
                                                        DPNHANDLE *CONST phAsyncHandle,
                                                        CONST DWORD dwFlags)
{
    FIXME("(%p)->(%x,%p,%p,%x): stub\n", iface, idGroup, pvAsyncContext, phAsyncHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_AddPlayerToGroup(PDIRECTPLAY8PEER iface,
                                                            CONST DPNID idGroup,
                                                            CONST DPNID idClient,
                                                            PVOID CONST pvAsyncContext,
                                                            DPNHANDLE *CONST phAsyncHandle,
                                                            CONST DWORD dwFlags)
{
    FIXME("(%p)->(%x,%x,%p,%p,%x): stub\n", iface, idGroup, idClient, pvAsyncContext, phAsyncHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_RemovePlayerFromGroup(PDIRECTPLAY8PEER iface,
                                                                 CONST DPNID idGroup,
                                                                 CONST DPNID idClient,
                                                                 PVOID CONST pvAsyncContext,
                                                                 DPNHANDLE *CONST phAsyncHandle,
                                                                 CONST DWORD dwFlags)
{
    FIXME("(%p)->(%x,%x,%p,%p,%x): stub\n", iface, idGroup, idClient, pvAsyncContext, phAsyncHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_SetGroupInfo(PDIRECTPLAY8PEER iface,
                                                        CONST DPNID dpnid,
                                                        DPN_GROUP_INFO *CONST pdpnGroupInfo,
                                                        PVOID CONST pvAsyncContext,
                                                        DPNHANDLE *CONST phAsyncHandle,
                                                        CONST DWORD dwFlags)
{
    FIXME("(%p)->(%x,%p,%p,%p,%x): stub\n", iface, dpnid, pdpnGroupInfo, pvAsyncContext, phAsyncHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_GetGroupInfo(PDIRECTPLAY8PEER iface,
                                                        CONST DPNID dpnid,
                                                        DPN_GROUP_INFO *CONST pdpnGroupInfo,
                                                        DWORD *CONST pdwSize,
                                                        CONST DWORD dwFlags)
{
    FIXME("(%p)->(%x,%p,%p,%x): stub\n", iface, dpnid, pdpnGroupInfo, pdwSize, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_EnumPlayersAndGroups(PDIRECTPLAY8PEER iface,
                                                                DPNID *CONST prgdpnid,
                                                                DWORD *CONST pcdpnid,
                                                                CONST DWORD dwFlags)
{
    FIXME("(%p)->(%p,%p,%x): stub\n", iface, prgdpnid, pcdpnid, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_EnumGroupMembers(PDIRECTPLAY8PEER iface,
                                                            CONST DPNID dpnid,
                                                            DPNID *CONST prgdpnid,
                                                            DWORD *CONST pcdpnid,
                                                            CONST DWORD dwFlags)
{
    FIXME("(%p)->(%x,%p,%p,%x): stub\n", iface, dpnid, prgdpnid, pcdpnid, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_SetPeerInfo(PDIRECTPLAY8PEER iface,
                                                       CONST DPN_PLAYER_INFO *CONST pdpnPlayerInfo,
                                                       PVOID CONST pvAsyncContext,
                                                       DPNHANDLE *CONST phAsyncHandle,
                                                       CONST DWORD dwFlags)
{
    FIXME("(%p)->(%p,%p,%p,%x): stub\n", iface, pdpnPlayerInfo, pvAsyncContext, phAsyncHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_GetPeerInfo(PDIRECTPLAY8PEER iface,
                                                       CONST DPNID dpnid,
                                                       DPN_PLAYER_INFO *CONST pdpnPlayerInfo,
                                                       DWORD *CONST pdwSize,
                                                       CONST DWORD dwFlags)
{
    FIXME("(%p)->(%x,%p,%p,%x): stub\n", iface, dpnid, pdpnPlayerInfo, pdwSize, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_GetPeerAddress(PDIRECTPLAY8PEER iface,
                                                          CONST DPNID dpnid,
                                                          IDirectPlay8Address **CONST pAddress,
                                                          CONST DWORD dwFlags)
{
    FIXME("(%p)->(%x,%p,%x): stub\n", iface, dpnid, pAddress, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_GetLocalHostAddresses(PDIRECTPLAY8PEER iface,
                                                                 IDirectPlay8Address **CONST prgpAddress,
                                                                 DWORD *CONST pcAddress,
                                                                 CONST DWORD dwFlags)
{
    FIXME("(%p)->(%p,%p,%x): stub\n", iface, prgpAddress, pcAddress, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_Close(PDIRECTPLAY8PEER iface,
                                                 CONST DWORD dwFlags)
{
    FIXME("(%p)->(%x): stub\n", iface, dwFlags);

    return DPN_OK;
}

static const IDirectPlay8PeerVtbl DirectPlay8Peer_Vtbl =
{
    IDirectPlay8PeerImpl_QueryInterface,
    IDirectPlay8PeerImpl_AddRef,
    IDirectPlay8PeerImpl_Release,
    IDirectPlay8PeerImpl_Initialize,
    IDirectPlay8PeerImpl_EnumServiceProviders,
    IDirectPlay8PeerImpl_CancelAsyncOperation,
    IDirectPlay8PeerImpl_Connect,
    IDirectPlay8PeerImpl_SendTo,
    IDirectPlay8PeerImpl_GetSendQueueInfo,
    IDirectPlay8PeerImpl_Host,
    IDirectPlay8PeerImpl_GetApplicationDesc,
    IDirectPlay8PeerImpl_SetApplicationDesc,
    IDirectPlay8PeerImpl_CreateGroup,
    IDirectPlay8PeerImpl_DestroyGroup,
    IDirectPlay8PeerImpl_AddPlayerToGroup,
    IDirectPlay8PeerImpl_RemovePlayerFromGroup,
    IDirectPlay8PeerImpl_SetGroupInfo,
    IDirectPlay8PeerImpl_GetGroupInfo,
    IDirectPlay8PeerImpl_EnumPlayersAndGroups,
    IDirectPlay8PeerImpl_EnumGroupMembers,
    IDirectPlay8PeerImpl_SetPeerInfo,
    IDirectPlay8PeerImpl_GetPeerInfo,
    IDirectPlay8PeerImpl_GetPeerAddress,
    IDirectPlay8PeerImpl_GetLocalHostAddresses,
    IDirectPlay8PeerImpl_Close
};

HRESULT DPNET_CreateDirectPlay8Peer(LPCLASSFACTORY iface, LPUNKNOWN punkOuter, REFIID riid, LPVOID *ppobj) {
    IDirectPlay8PeerImpl* Client;
    HRESULT Ret;

    Client = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectPlay8PeerImpl));

    if(Client == NULL)
    {
        *ppobj = NULL;
        WARN("Not enough memory\n");
        return E_OUTOFMEMORY;
    }

    Client->lpVtbl = &DirectPlay8Peer_Vtbl;
    if((Ret = IDirectPlay8PeerImpl_QueryInterface((PDIRECTPLAY8PEER)Client, riid, ppobj)) != DPN_OK)
        HeapFree(GetProcessHeap(), 0, Client);

    return Ret;
}
