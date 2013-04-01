/* 
 * DirectPlay8 Peer
 * 
 * Copyright 2004 Raphael Junqueira
 * Copyright 2008 Alexander N. Sørnes <alex@thehandofagony.com>
 * Copyright 2011 Louis Lenders
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

static inline IDirectPlay8PeerImpl *impl_from_IDirectPlay8Peer(IDirectPlay8Peer *iface)
{
    return CONTAINING_RECORD(iface, IDirectPlay8PeerImpl, IDirectPlay8Peer_iface);
}

/* IUnknown interface follows */
static HRESULT WINAPI IDirectPlay8PeerImpl_QueryInterface(IDirectPlay8Peer *iface, REFIID riid,
        void **ppobj)
{
    IDirectPlay8PeerImpl* This = impl_from_IDirectPlay8Peer(iface);

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

static ULONG WINAPI IDirectPlay8PeerImpl_AddRef(IDirectPlay8Peer *iface)
{
    IDirectPlay8PeerImpl* This = impl_from_IDirectPlay8Peer(iface);
    ULONG RefCount = InterlockedIncrement(&This->ref);

    return RefCount;
}

static ULONG WINAPI IDirectPlay8PeerImpl_Release(IDirectPlay8Peer *iface)
{
    IDirectPlay8PeerImpl* This = impl_from_IDirectPlay8Peer(iface);
    ULONG RefCount = InterlockedDecrement(&This->ref);

    if(!RefCount)
        HeapFree(GetProcessHeap(), 0, This);

    return RefCount;
}

/* IDirectPlay8Peer interface follows */

static HRESULT WINAPI IDirectPlay8PeerImpl_Initialize(IDirectPlay8Peer *iface,
        void * const pvUserContext, const PFNDPNMESSAGEHANDLER pfn, const DWORD dwFlags)
{
    TRACE("(%p)->(%p,%p,%x): stub\n", iface, pvUserContext, pfn, dwFlags);

    return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_EnumServiceProviders(IDirectPlay8Peer *iface,
        const GUID * const pguidServiceProvider, const GUID * const pguidApplication,
        DPN_SERVICE_PROVIDER_INFO * const pSPInfoBuffer, DWORD * const pcbEnumData,
        DWORD * const pcReturned, const DWORD dwFlags)
{
    static const WCHAR dp_providerW[] = {'D','i','r','e','c','t','P','l','a','y','8',' ','T','C','P','/','I','P',' ',
                                         'S','e','r','v','i','c','e',' ','P','r','o','v','i','d','e','r','\0'};
    static const WCHAR dp_adapterW[] = {'L','o','c','a','l',' ','A','r','e','a',' ','C','o','n','n','e','c','t','i','o','n',
                                        ' ','-',' ','I','P','v','4','\0'};

    static const GUID adapter_guid = {0x4ce725f6, 0xd3c0, 0xdade, {0xba, 0x6f, 0x11, 0xf9, 0x65, 0xbc, 0x42, 0x99}};
    DWORD req_size;

    TRACE("(%p)->(%p,%p,%p,%p,%p,%x): stub\n", iface, pguidServiceProvider, pguidApplication, pSPInfoBuffer,
                                               pcbEnumData, pcReturned, dwFlags);

    if(!pguidServiceProvider)
    {
        req_size = sizeof(DPN_SERVICE_PROVIDER_INFO) + sizeof(dp_providerW);
    }
    else if(IsEqualGUID(pguidServiceProvider, &CLSID_DP8SP_TCPIP))
    {
        req_size = sizeof(DPN_SERVICE_PROVIDER_INFO) + sizeof(dp_adapterW);
    }
    else
    {
        FIXME("Application requested a provider we don't handle (yet)\n");
        *pcReturned = 0;
        return DPNERR_DOESNOTEXIST;
    }

    if(*pcbEnumData < req_size)
    {
        *pcbEnumData = req_size;
        return DPNERR_BUFFERTOOSMALL;
    }

    pSPInfoBuffer->pwszName = (LPWSTR)(pSPInfoBuffer + 1);

    if(!pguidServiceProvider)
    {
        lstrcpyW(pSPInfoBuffer->pwszName, dp_providerW);
        pSPInfoBuffer->guid = CLSID_DP8SP_TCPIP;
    }
    else
    {
        lstrcpyW(pSPInfoBuffer->pwszName, dp_adapterW);
        pSPInfoBuffer->guid = adapter_guid;
    }

    *pcReturned = 1;
    return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_CancelAsyncOperation(IDirectPlay8Peer *iface,
        const DPNHANDLE hAsyncHandle, const DWORD dwFlags)
{
    FIXME("(%p)->(%x,%x): stub\n", iface, hAsyncHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_Connect(IDirectPlay8Peer *iface,
        const DPN_APPLICATION_DESC * const pdnAppDesc, IDirectPlay8Address * const pHostAddr,
        IDirectPlay8Address * const pDeviceInfo, const DPN_SECURITY_DESC * const pdnSecurity,
        const DPN_SECURITY_CREDENTIALS * const pdnCredentials, const void * const pvUserConnectData,
        const DWORD dwUserConnectDataSize, void * const pvPlayerContext,
        void * const pvAsyncContext, DPNHANDLE * const phAsyncHandle, const DWORD dwFlags)
{
    FIXME("(%p)->(%p,%p,%p,%p,%p,%p,%x,%p,%p,%p,%x): stub\n", iface, pdnAppDesc, pHostAddr, pDeviceInfo, pdnSecurity, pdnCredentials, pvUserConnectData, dwUserConnectDataSize, pvPlayerContext, pvAsyncContext, phAsyncHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_SendTo(IDirectPlay8Peer *iface, const DPNID dpnId,
        const DPN_BUFFER_DESC *pBufferDesc, const DWORD cBufferDesc, const DWORD dwTimeOut,
        void * const pvAsyncContext, DPNHANDLE * const phAsyncHandle, const DWORD dwFlags)
{
    FIXME("(%p)->(%x,%p,%x,%x,%p,%p,%x): stub\n", iface, dpnId, pBufferDesc, cBufferDesc, dwTimeOut, pvAsyncContext, phAsyncHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_GetSendQueueInfo(IDirectPlay8Peer *iface,
        const DPNID dpnid, DWORD * const pdwNumMsgs, DWORD * const pdwNumBytes, const DWORD dwFlags)
{
    FIXME("(%p)->(%x,%p,%p,%x): stub\n", iface, dpnid, pdwNumMsgs, pdwNumBytes, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_Host(IDirectPlay8Peer *iface,
        const DPN_APPLICATION_DESC * const pdnAppDesc, IDirectPlay8Address ** const prgpDeviceInfo,
        const DWORD cDeviceInfo, const DPN_SECURITY_DESC * const pdpSecurity,
        const DPN_SECURITY_CREDENTIALS * const pdpCredentials, void * const pvPlayerContext,
        const DWORD dwFlags)
{
    FIXME("(%p)->(%p,%p,%x,%p,%p,%p,%x): stub\n", iface, pdnAppDesc, prgpDeviceInfo, cDeviceInfo, pdpSecurity, pdpCredentials, pvPlayerContext, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_GetApplicationDesc(IDirectPlay8Peer *iface,
        DPN_APPLICATION_DESC * const pAppDescBuffer, DWORD * const pcbDataSize, const DWORD dwFlags)
{
    FIXME("(%p)->(%p,%p,%x): stub\n", iface, pAppDescBuffer, pcbDataSize, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_SetApplicationDesc(IDirectPlay8Peer *iface,
        const DPN_APPLICATION_DESC * const pad, const DWORD dwFlags)
{
    FIXME("(%p)->(%p,%x): stub\n", iface, pad, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_CreateGroup(IDirectPlay8Peer *iface,
        const DPN_GROUP_INFO * const pdpnGroupInfo, void * const pvGroupContext,
        void * const pvAsyncContext, DPNHANDLE * const phAsyncHandle, const DWORD dwFlags)
{
    FIXME("(%p)->(%p,%p,%p,%p,%x): stub\n", iface, pdpnGroupInfo, pvGroupContext, pvAsyncContext, phAsyncHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_DestroyGroup(IDirectPlay8Peer *iface,
        const DPNID idGroup, void * const pvAsyncContext, DPNHANDLE * const phAsyncHandle,
        const DWORD dwFlags)
{
    FIXME("(%p)->(%x,%p,%p,%x): stub\n", iface, idGroup, pvAsyncContext, phAsyncHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_AddPlayerToGroup(IDirectPlay8Peer *iface,
        const DPNID idGroup, const DPNID idClient, void * const pvAsyncContext,
        DPNHANDLE * const phAsyncHandle, const DWORD dwFlags)
{
    FIXME("(%p)->(%x,%x,%p,%p,%x): stub\n", iface, idGroup, idClient, pvAsyncContext, phAsyncHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_RemovePlayerFromGroup(IDirectPlay8Peer *iface,
        const DPNID idGroup, const DPNID idClient, void * const pvAsyncContext,
        DPNHANDLE * const phAsyncHandle, const DWORD dwFlags)
{
    FIXME("(%p)->(%x,%x,%p,%p,%x): stub\n", iface, idGroup, idClient, pvAsyncContext, phAsyncHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_SetGroupInfo(IDirectPlay8Peer *iface, const DPNID dpnid,
        DPN_GROUP_INFO * const pdpnGroupInfo, void * const pvAsyncContext,
        DPNHANDLE * const phAsyncHandle, const DWORD dwFlags)
{
    FIXME("(%p)->(%x,%p,%p,%p,%x): stub\n", iface, dpnid, pdpnGroupInfo, pvAsyncContext, phAsyncHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_GetGroupInfo(IDirectPlay8Peer *iface, const DPNID dpnid,
        DPN_GROUP_INFO * const pdpnGroupInfo, DWORD * const pdwSize, const DWORD dwFlags)
{
    FIXME("(%p)->(%x,%p,%p,%x): stub\n", iface, dpnid, pdpnGroupInfo, pdwSize, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_EnumPlayersAndGroups(IDirectPlay8Peer *iface,
        DPNID * const prgdpnid, DWORD * const pcdpnid, const DWORD dwFlags)
{
    FIXME("(%p)->(%p,%p,%x): stub\n", iface, prgdpnid, pcdpnid, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_EnumGroupMembers(IDirectPlay8Peer *iface,
        const DPNID dpnid, DPNID * const prgdpnid, DWORD * const pcdpnid, const DWORD dwFlags)
{
    FIXME("(%p)->(%x,%p,%p,%x): stub\n", iface, dpnid, prgdpnid, pcdpnid, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_SetPeerInfo(IDirectPlay8Peer *iface,
        const DPN_PLAYER_INFO * const pdpnPlayerInfo, void * const pvAsyncContext,
        DPNHANDLE * const phAsyncHandle, const DWORD dwFlags)
{
    FIXME("(%p)->(%p,%p,%p,%x): stub\n", iface, pdpnPlayerInfo, pvAsyncContext, phAsyncHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_GetPeerInfo(IDirectPlay8Peer *iface, const DPNID dpnid,
        DPN_PLAYER_INFO * const pdpnPlayerInfo, DWORD * const pdwSize, const DWORD dwFlags)
{
    FIXME("(%p)->(%x,%p,%p,%x): stub\n", iface, dpnid, pdpnPlayerInfo, pdwSize, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_GetPeerAddress(IDirectPlay8Peer *iface,
        const DPNID dpnid, IDirectPlay8Address ** const pAddress, const DWORD dwFlags)
{
    FIXME("(%p)->(%x,%p,%x): stub\n", iface, dpnid, pAddress, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_GetLocalHostAddresses(IDirectPlay8Peer *iface,
        IDirectPlay8Address ** const prgpAddress, DWORD * const pcAddress, const DWORD dwFlags)
{
    FIXME("(%p)->(%p,%p,%x): stub\n", iface, prgpAddress, pcAddress, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_Close(IDirectPlay8Peer *iface, const DWORD dwFlags)
{
    FIXME("(%p)->(%x): stub\n", iface, dwFlags);

    return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_EnumHosts(IDirectPlay8Peer *iface,
        PDPN_APPLICATION_DESC const pApplicationDesc, IDirectPlay8Address * const pAddrHost,
        IDirectPlay8Address * const pDeviceInfo, void * const pUserEnumData,
        const DWORD dwUserEnumDataSize, const DWORD dwEnumCount, const DWORD dwRetryInterval,
        const DWORD dwTimeOut, void * const pvUserContext, DPNHANDLE * const pAsyncHandle, const DWORD dwFlags)
{
    FIXME("(%p)->(%p,%p,%p,%p,%x,%x,%x,%x,%p,%p,%x): stub\n",
            iface, pApplicationDesc, pAddrHost, pDeviceInfo, pUserEnumData, dwUserEnumDataSize, dwEnumCount,
            dwRetryInterval, dwTimeOut, pvUserContext, pAsyncHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_DestroyPeer(IDirectPlay8Peer *iface, const DPNID dpnidClient,
        const void * const pvDestroyData, const DWORD dwDestroyDataSize, const DWORD dwFlags)
{
    FIXME("(%p)->(%x,%p,%x,%x): stub\n", iface, dpnidClient, pvDestroyData, dwDestroyDataSize, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_ReturnBuffer(IDirectPlay8Peer *iface, const DPNHANDLE hBufferHandle,
        const DWORD dwFlags)
{
    FIXME("(%p)->(%x,%x): stub\n", iface, hBufferHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_GetPlayerContext(IDirectPlay8Peer *iface, const DPNID dpnid,
        void ** const ppvPlayerContext, const DWORD dwFlags)
{
    FIXME("(%p)->(%x,%p,%x): stub\n", iface, dpnid, ppvPlayerContext, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_GetGroupContext(IDirectPlay8Peer *iface, const DPNID dpnid,
        void ** const ppvGroupContext, const DWORD dwFlags)
{
    FIXME("(%p)->(%x,%p,%x): stub\n", iface, dpnid, ppvGroupContext, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_GetCaps(IDirectPlay8Peer *iface, DPN_CAPS * const pdpCaps,
        const DWORD dwFlags)
{
    FIXME("(%p)->(%p,%x): stub\n", iface, pdpCaps, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_SetCaps(IDirectPlay8Peer *iface, const DPN_CAPS * const pdpCaps,
        const DWORD dwFlags)
{
    FIXME("(%p)->(%p,%x): stub\n", iface, pdpCaps, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_SetSPCaps(IDirectPlay8Peer *iface, const GUID * const pguidSP,
        const DPN_SP_CAPS * const pdpspCaps, const DWORD dwFlags )
{
    FIXME("(%p)->(%p,%p,%x): stub\n", iface, pguidSP, pdpspCaps, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_GetSPCaps(IDirectPlay8Peer *iface, const GUID * const pguidSP,
        DPN_SP_CAPS * const pdpspCaps, const DWORD dwFlags)
{
    TRACE("(%p)->(%p,%p,%x)\n", iface, pguidSP, pdpspCaps, dwFlags);

    if(pdpspCaps->dwSize != sizeof(DPN_SP_CAPS))
    {
        return DPNERR_INVALIDPARAM;
    }

    pdpspCaps->dwFlags = DPNSPCAPS_SUPPORTSDPNSRV | DPNSPCAPS_SUPPORTSBROADCAST |
                         DPNSPCAPS_SUPPORTSALLADAPTERS | DPNSPCAPS_SUPPORTSTHREADPOOL;
    pdpspCaps->dwNumThreads = 3;
    pdpspCaps->dwDefaultEnumCount = 5;
    pdpspCaps->dwDefaultEnumRetryInterval = 1500;
    pdpspCaps->dwDefaultEnumTimeout = 1500;
    pdpspCaps->dwMaxEnumPayloadSize = 983;
    pdpspCaps->dwBuffersPerThread = 1;
    pdpspCaps->dwSystemBufferSize = 0x10000;

    return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_GetConnectionInfo(IDirectPlay8Peer *iface, const DPNID dpnid,
        DPN_CONNECTION_INFO * const pdpConnectionInfo, const DWORD dwFlags)
{
    FIXME("(%p)->(%x,%p,%x): stub\n", iface, dpnid, pdpConnectionInfo, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_RegisterLobby(IDirectPlay8Peer *iface, const DPNHANDLE dpnHandle,
        struct IDirectPlay8LobbiedApplication * const pIDP8LobbiedApplication, const DWORD dwFlags)
{
    FIXME("(%p)->(%x,%p,%x): stub\n", iface, dpnHandle, pIDP8LobbiedApplication, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_TerminateSession(IDirectPlay8Peer *iface, void * const pvTerminateData,
        const DWORD dwTerminateDataSize, const DWORD dwFlags)
{
    FIXME("(%p)->(%p,%x,%x): stub\n", iface, pvTerminateData, dwTerminateDataSize, dwFlags);

    return DPNERR_GENERIC;
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
    IDirectPlay8PeerImpl_Close,
    IDirectPlay8PeerImpl_EnumHosts,
    IDirectPlay8PeerImpl_DestroyPeer,
    IDirectPlay8PeerImpl_ReturnBuffer,
    IDirectPlay8PeerImpl_GetPlayerContext,
    IDirectPlay8PeerImpl_GetGroupContext,
    IDirectPlay8PeerImpl_GetCaps,
    IDirectPlay8PeerImpl_SetCaps,
    IDirectPlay8PeerImpl_SetSPCaps,
    IDirectPlay8PeerImpl_GetSPCaps,
    IDirectPlay8PeerImpl_GetConnectionInfo,
    IDirectPlay8PeerImpl_RegisterLobby,
    IDirectPlay8PeerImpl_TerminateSession
};

HRESULT DPNET_CreateDirectPlay8Peer(LPCLASSFACTORY iface, LPUNKNOWN punkOuter, REFIID riid, LPVOID *ppobj) {
    IDirectPlay8PeerImpl* Client;
    HRESULT ret;

    Client = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectPlay8PeerImpl));

    if(Client == NULL)
    {
        *ppobj = NULL;
        WARN("Not enough memory\n");
        return E_OUTOFMEMORY;
    }

    Client->IDirectPlay8Peer_iface.lpVtbl = &DirectPlay8Peer_Vtbl;
    ret = IDirectPlay8PeerImpl_QueryInterface(&Client->IDirectPlay8Peer_iface, riid, ppobj);
    if(ret != DPN_OK)
        HeapFree(GetProcessHeap(), 0, Client);

    return ret;
}
