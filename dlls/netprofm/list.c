/*
 * Copyright 2014 Hans Leidekker for CodeWeavers
 * Copyright 2015 Michael Müller
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

#include "config.h"
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#define USE_WS_PREFIX
#include "winsock2.h"
#include "ws2ipdef.h"
#include "iphlpapi.h"
#include "ifdef.h"
#include "netioapi.h"
#include "initguid.h"
#include "objbase.h"
#include "ocidl.h"
#include "netlistmgr.h"
#include "olectl.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "netprofm_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(netprofm);

static inline void *heap_alloc( SIZE_T size )
{
    return HeapAlloc( GetProcessHeap(), 0, size );
}

static inline BOOL heap_free( void *mem )
{
    return HeapFree( GetProcessHeap(), 0, mem );
}

struct network
{
    INetwork     INetwork_iface;
    LONG         refs;
    struct list  entry;
    GUID         id;
    VARIANT_BOOL connected_to_internet;
    VARIANT_BOOL connected;
};

struct connection
{
    INetworkConnection     INetworkConnection_iface;
    INetworkConnectionCost INetworkConnectionCost_iface;
    LONG                   refs;
    struct list            entry;
    GUID                   id;
    INetwork              *network;
    VARIANT_BOOL           connected_to_internet;
    VARIANT_BOOL           connected;
};

struct list_manager
{
    INetworkListManager INetworkListManager_iface;
    INetworkCostManager INetworkCostManager_iface;
    IConnectionPointContainer IConnectionPointContainer_iface;
    LONG                refs;
    struct list         networks;
    struct list         connections;
};

struct connection_point
{
    IConnectionPoint IConnectionPoint_iface;
    IConnectionPointContainer *container;
    LONG refs;
    IID iid;
};

static inline struct list_manager *impl_from_IConnectionPointContainer(IConnectionPointContainer *iface)
{
    return CONTAINING_RECORD(iface, struct list_manager, IConnectionPointContainer_iface);
}

static inline struct list_manager *impl_from_INetworkCostManager(
    INetworkCostManager *iface )
{
    return CONTAINING_RECORD( iface, struct list_manager, INetworkCostManager_iface );
}

static inline struct connection_point *impl_from_IConnectionPoint(
    IConnectionPoint *iface )
{
    return CONTAINING_RECORD( iface, struct connection_point, IConnectionPoint_iface );
}

static HRESULT WINAPI connection_point_QueryInterface(
    IConnectionPoint *iface,
    REFIID riid,
    void **obj )
{
    struct connection_point *cp = impl_from_IConnectionPoint( iface );
    TRACE( "%p, %s, %p\n", cp, debugstr_guid(riid), obj );

    if (IsEqualGUID( riid, &IID_IConnectionPoint ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *obj = iface;
    }
    else
    {
        FIXME( "interface %s not implemented\n", debugstr_guid(riid) );
        *obj = NULL;
        return E_NOINTERFACE;
    }
    IConnectionPoint_AddRef( iface );
    return S_OK;
}

static ULONG WINAPI connection_point_AddRef(
    IConnectionPoint *iface )
{
    struct connection_point *cp = impl_from_IConnectionPoint( iface );
    return InterlockedIncrement( &cp->refs );
}

static ULONG WINAPI connection_point_Release(
    IConnectionPoint *iface )
{
    struct connection_point *cp = impl_from_IConnectionPoint( iface );
    LONG refs = InterlockedDecrement( &cp->refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", cp );
        IConnectionPointContainer_Release( cp->container );
        heap_free( cp );
    }
    return refs;
}

static HRESULT WINAPI connection_point_GetConnectionInterface(
    IConnectionPoint *iface,
    IID *iid )
{
    struct connection_point *cp = impl_from_IConnectionPoint( iface );
    TRACE( "%p, %p\n", cp, iid );

    if (!iid)
        return E_POINTER;

    memcpy( iid, &cp->iid, sizeof(*iid) );
    return S_OK;
}

static HRESULT WINAPI connection_point_GetConnectionPointContainer(
    IConnectionPoint *iface,
    IConnectionPointContainer **container )
{
    struct connection_point *cp = impl_from_IConnectionPoint( iface );
    TRACE( "%p, %p\n", cp, container );

    if (!container)
        return E_POINTER;

    IConnectionPointContainer_AddRef( cp->container );
    *container = cp->container;
    return S_OK;
}

static HRESULT WINAPI connection_point_Advise(
    IConnectionPoint *iface,
    IUnknown *sink,
    DWORD *cookie )
{
    struct connection_point *cp = impl_from_IConnectionPoint( iface );
    FIXME( "%p, %p, %p - stub\n", cp, sink, cookie );

    if (!sink || !cookie)
        return E_POINTER;

    return CONNECT_E_CANNOTCONNECT;
}

static HRESULT WINAPI connection_point_Unadvise(
    IConnectionPoint *iface,
    DWORD cookie )
{
    struct connection_point *cp = impl_from_IConnectionPoint( iface );
    FIXME( "%p, %d - stub\n", cp, cookie );

    return E_POINTER;
}

static HRESULT WINAPI connection_point_EnumConnections(
    IConnectionPoint *iface,
    IEnumConnections **connections )
{
    struct connection_point *cp = impl_from_IConnectionPoint( iface );
    FIXME( "%p, %p - stub\n", cp, connections );

    return E_NOTIMPL;
}

static const IConnectionPointVtbl connection_point_vtbl =
{
    connection_point_QueryInterface,
    connection_point_AddRef,
    connection_point_Release,
    connection_point_GetConnectionInterface,
    connection_point_GetConnectionPointContainer,
    connection_point_Advise,
    connection_point_Unadvise,
    connection_point_EnumConnections
};

static HRESULT connection_point_create(
    IConnectionPoint **obj,
    REFIID riid,
    IConnectionPointContainer *container )
{
    struct connection_point *cp;
    TRACE( "%p, %s, %p\n", obj, debugstr_guid(riid), container );

    if (!(cp = heap_alloc( sizeof(*cp) ))) return E_OUTOFMEMORY;
    cp->IConnectionPoint_iface.lpVtbl = &connection_point_vtbl;
    cp->container = container;
    cp->refs = 1;

    memcpy( &cp->iid, riid, sizeof(*riid) );
    IConnectionPointContainer_AddRef( container );

    *obj = &cp->IConnectionPoint_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}

static inline struct network *impl_from_INetwork(
    INetwork *iface )
{
    return CONTAINING_RECORD( iface, struct network, INetwork_iface );
}

static HRESULT WINAPI network_QueryInterface(
    INetwork *iface, REFIID riid, void **obj )
{
    struct network *network = impl_from_INetwork( iface );

    TRACE( "%p, %s, %p\n", network, debugstr_guid(riid), obj );

    if (IsEqualIID( riid, &IID_INetwork ) ||
        IsEqualIID( riid, &IID_IDispatch ) ||
        IsEqualIID( riid, &IID_IUnknown ))
    {
        *obj = iface;
        INetwork_AddRef( iface );
        return S_OK;
    }
    else
    {
        WARN( "interface not supported %s\n", debugstr_guid(riid) );
        *obj = NULL;
        return E_NOINTERFACE;
    }
}

static ULONG WINAPI network_AddRef(
    INetwork *iface )
{
    struct network *network = impl_from_INetwork( iface );

    TRACE( "%p\n", network );
    return InterlockedIncrement( &network->refs );
}

static ULONG WINAPI network_Release(
    INetwork *iface )
{
    struct network *network = impl_from_INetwork( iface );
    LONG refs;

    TRACE( "%p\n", network );

    if (!(refs = InterlockedDecrement( &network->refs )))
    {
        list_remove( &network->entry );
        heap_free( network );
    }
    return refs;
}

static HRESULT WINAPI network_GetTypeInfoCount(
    INetwork *iface,
    UINT *count )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI network_GetTypeInfo(
    INetwork *iface,
    UINT index,
    LCID lcid,
    ITypeInfo **info )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI network_GetIDsOfNames(
    INetwork *iface,
    REFIID riid,
    LPOLESTR *names,
    UINT count,
    LCID lcid,
    DISPID *dispid )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI network_Invoke(
    INetwork *iface,
    DISPID member,
    REFIID riid,
    LCID lcid,
    WORD flags,
    DISPPARAMS *params,
    VARIANT *result,
    EXCEPINFO *excep_info,
    UINT *arg_err )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI network_GetName(
    INetwork *iface,
    BSTR *pszNetworkName )
{
    FIXME( "%p, %p\n", iface, pszNetworkName );
    return E_NOTIMPL;
}

static HRESULT WINAPI network_SetName(
    INetwork *iface,
    BSTR szNetworkNewName )
{
    FIXME( "%p, %s\n", iface, debugstr_w(szNetworkNewName) );
    return E_NOTIMPL;
}

static HRESULT WINAPI network_GetDescription(
    INetwork *iface,
    BSTR *pszDescription )
{
    FIXME( "%p, %p\n", iface, pszDescription );
    return E_NOTIMPL;
}

static HRESULT WINAPI network_SetDescription(
    INetwork *iface,
    BSTR szDescription )
{
    FIXME( "%p, %s\n", iface, debugstr_w(szDescription) );
    return E_NOTIMPL;
}

static HRESULT WINAPI network_GetNetworkId(
    INetwork *iface,
    GUID *pgdGuidNetworkId )
{
    struct network *network = impl_from_INetwork( iface );

    TRACE( "%p, %p\n", iface, pgdGuidNetworkId );

    *pgdGuidNetworkId = network->id;
    return S_OK;
}

static HRESULT WINAPI network_GetDomainType(
    INetwork *iface,
    NLM_DOMAIN_TYPE *pDomainType )
{
    FIXME( "%p, %p\n", iface, pDomainType );

    *pDomainType = NLM_DOMAIN_TYPE_NON_DOMAIN_NETWORK;
    return S_OK;
}

static HRESULT WINAPI network_GetNetworkConnections(
    INetwork *iface,
    IEnumNetworkConnections **ppEnumNetworkConnection )
{
    FIXME( "%p, %p\n", iface, ppEnumNetworkConnection );
    return E_NOTIMPL;
}

static HRESULT WINAPI network_GetTimeCreatedAndConnected(
    INetwork *iface,
    DWORD *pdwLowDateTimeCreated,
    DWORD *pdwHighDateTimeCreated,
    DWORD *pdwLowDateTimeConnected,
    DWORD *pdwHighDateTimeConnected )
{
    FIXME( "%p, %p, %p, %p, %p\n", iface, pdwLowDateTimeCreated, pdwHighDateTimeCreated,
        pdwLowDateTimeConnected, pdwHighDateTimeConnected );
    return E_NOTIMPL;
}

static HRESULT WINAPI network_get_IsConnectedToInternet(
    INetwork *iface,
    VARIANT_BOOL *pbIsConnected )
{
    struct network *network = impl_from_INetwork( iface );

    TRACE( "%p, %p\n", iface, pbIsConnected );

    *pbIsConnected = network->connected_to_internet;
    return S_OK;
}

static HRESULT WINAPI network_get_IsConnected(
    INetwork *iface,
    VARIANT_BOOL *pbIsConnected )
{
    struct network *network = impl_from_INetwork( iface );

    TRACE( "%p, %p\n", iface, pbIsConnected );

    *pbIsConnected = network->connected;
    return S_OK;
}

static HRESULT WINAPI network_GetConnectivity(
    INetwork *iface,
    NLM_CONNECTIVITY *pConnectivity )
{
    FIXME( "%p, %p\n", iface, pConnectivity );

    *pConnectivity = NLM_CONNECTIVITY_IPV4_INTERNET;
    return S_OK;
}

static HRESULT WINAPI network_GetCategory(
    INetwork *iface,
    NLM_NETWORK_CATEGORY *pCategory )
{
    FIXME( "%p, %p\n", iface, pCategory );

    *pCategory = NLM_NETWORK_CATEGORY_PUBLIC;
    return S_OK;
}

static HRESULT WINAPI network_SetCategory(
    INetwork *iface,
    NLM_NETWORK_CATEGORY NewCategory )
{
    FIXME( "%p, %u\n", iface, NewCategory );
    return E_NOTIMPL;
}

static const struct INetworkVtbl network_vtbl =
{
    network_QueryInterface,
    network_AddRef,
    network_Release,
    network_GetTypeInfoCount,
    network_GetTypeInfo,
    network_GetIDsOfNames,
    network_Invoke,
    network_GetName,
    network_SetName,
    network_GetDescription,
    network_SetDescription,
    network_GetNetworkId,
    network_GetDomainType,
    network_GetNetworkConnections,
    network_GetTimeCreatedAndConnected,
    network_get_IsConnectedToInternet,
    network_get_IsConnected,
    network_GetConnectivity,
    network_GetCategory,
    network_SetCategory
};

static struct network *create_network( const GUID *id )
{
    struct network *ret;

    if (!(ret = heap_alloc( sizeof(*ret) ))) return NULL;

    ret->INetwork_iface.lpVtbl = &network_vtbl;
    ret->refs                  = 1;
    ret->id                    = *id;
    ret->connected             = VARIANT_FALSE;
    ret->connected_to_internet = VARIANT_FALSE;
    list_init( &ret->entry );

    return ret;
}

static HRESULT WINAPI cost_manager_QueryInterface(
    INetworkCostManager *iface,
    REFIID riid,
    void **obj )
{
    struct list_manager *mgr = impl_from_INetworkCostManager( iface );
    return INetworkListManager_QueryInterface( &mgr->INetworkListManager_iface, riid, obj );
}

static ULONG WINAPI cost_manager_AddRef(
    INetworkCostManager *iface )
{
    struct list_manager *mgr = impl_from_INetworkCostManager( iface );
    return INetworkListManager_AddRef( &mgr->INetworkListManager_iface );
}

static ULONG WINAPI cost_manager_Release(
    INetworkCostManager *iface )
{
    struct list_manager *mgr = impl_from_INetworkCostManager( iface );
    return INetworkListManager_Release( &mgr->INetworkListManager_iface );
}

static HRESULT WINAPI cost_manager_GetCost(
    INetworkCostManager *iface, DWORD *pCost, NLM_SOCKADDR *pDestIPAddr)
{
    FIXME( "%p, %p, %p\n", iface, pCost, pDestIPAddr );

    if (!pCost) return E_POINTER;

    *pCost = NLM_CONNECTION_COST_UNRESTRICTED;
    return S_OK;
}

static BOOL map_address_6to4( const SOCKADDR_IN6 *addr6, SOCKADDR_IN *addr4 )
{
    ULONG i;

    if (addr6->sin6_family != WS_AF_INET6) return FALSE;

    for (i = 0; i < 5; i++)
        if (addr6->sin6_addr.u.Word[i]) return FALSE;

    if (addr6->sin6_addr.u.Word[5] != 0xffff) return FALSE;

    addr4->sin_family = WS_AF_INET;
    addr4->sin_port   = addr6->sin6_port;
    addr4->sin_addr.S_un.S_addr = addr6->sin6_addr.u.Word[6] << 16 | addr6->sin6_addr.u.Word[7];
    memset( &addr4->sin_zero, 0, sizeof(addr4->sin_zero) );

    return TRUE;
}

static HRESULT WINAPI cost_manager_GetDataPlanStatus(
    INetworkCostManager *iface, NLM_DATAPLAN_STATUS *pDataPlanStatus,
    NLM_SOCKADDR *pDestIPAddr)
{
    DWORD ret, index;
    NET_LUID luid;
    SOCKADDR *dst = (SOCKADDR *)pDestIPAddr;
    SOCKADDR_IN addr4, *dst4;

    FIXME( "%p, %p, %p\n", iface, pDataPlanStatus, pDestIPAddr );

    if (!pDataPlanStatus) return E_POINTER;

    if (dst && ((dst->sa_family == WS_AF_INET && (dst4 = (SOCKADDR_IN *)dst)) ||
               ((dst->sa_family == WS_AF_INET6 && map_address_6to4( (const SOCKADDR_IN6 *)dst, &addr4 )
                && (dst4 = &addr4)))))
    {
        if ((ret = GetBestInterface( dst4->sin_addr.S_un.S_addr, &index )))
            return HRESULT_FROM_WIN32( ret );

        if ((ret = ConvertInterfaceIndexToLuid( index, &luid )))
            return HRESULT_FROM_WIN32( ret );

        if ((ret = ConvertInterfaceLuidToGuid( &luid, &pDataPlanStatus->InterfaceGuid )))
            return HRESULT_FROM_WIN32( ret );
    }
    else
    {
        FIXME( "interface guid not found\n" );
        memset( &pDataPlanStatus->InterfaceGuid, 0, sizeof(pDataPlanStatus->InterfaceGuid) );
    }

    pDataPlanStatus->UsageData.UsageInMegabytes = NLM_UNKNOWN_DATAPLAN_STATUS;
    memset( &pDataPlanStatus->UsageData.LastSyncTime, 0, sizeof(pDataPlanStatus->UsageData.LastSyncTime) );
    pDataPlanStatus->DataLimitInMegabytes       = NLM_UNKNOWN_DATAPLAN_STATUS;
    pDataPlanStatus->InboundBandwidthInKbps     = NLM_UNKNOWN_DATAPLAN_STATUS;
    pDataPlanStatus->OutboundBandwidthInKbps    = NLM_UNKNOWN_DATAPLAN_STATUS;
    memset( &pDataPlanStatus->NextBillingCycle, 0, sizeof(pDataPlanStatus->NextBillingCycle) );
    pDataPlanStatus->MaxTransferSizeInMegabytes = NLM_UNKNOWN_DATAPLAN_STATUS;
    pDataPlanStatus->Reserved                   = 0;

    return S_OK;
}

static HRESULT WINAPI cost_manager_SetDestinationAddresses(
    INetworkCostManager *iface, UINT32 length, NLM_SOCKADDR *pDestIPAddrList,
    VARIANT_BOOL bAppend)
{
    FIXME( "%p, %u, %p, %x\n", iface, length, pDestIPAddrList, bAppend );
    return E_NOTIMPL;
}

static const INetworkCostManagerVtbl cost_manager_vtbl =
{
    cost_manager_QueryInterface,
    cost_manager_AddRef,
    cost_manager_Release,
    cost_manager_GetCost,
    cost_manager_GetDataPlanStatus,
    cost_manager_SetDestinationAddresses
};

static inline struct list_manager *impl_from_INetworkListManager(
    INetworkListManager *iface )
{
    return CONTAINING_RECORD( iface, struct list_manager, INetworkListManager_iface );
}

static ULONG WINAPI list_manager_AddRef(
    INetworkListManager *iface )
{
    struct list_manager *mgr = impl_from_INetworkListManager( iface );
    return InterlockedIncrement( &mgr->refs );
}

static ULONG WINAPI list_manager_Release(
    INetworkListManager *iface )
{
    struct list_manager *mgr = impl_from_INetworkListManager( iface );
    LONG refs = InterlockedDecrement( &mgr->refs );
    if (!refs)
    {
        struct list *ptr;

        TRACE( "destroying %p\n", mgr );

        while ((ptr = list_head( &mgr->networks )))
        {
            struct network *network = LIST_ENTRY( ptr, struct network, entry );
            list_remove( &network->entry );
            INetwork_Release( &network->INetwork_iface );
        }
        while ((ptr = list_head( &mgr->connections )))
        {
            struct connection *connection = LIST_ENTRY( ptr, struct connection, entry );
            list_remove( &connection->entry );
            INetworkConnection_Release( &connection->INetworkConnection_iface );
        }
        heap_free( mgr );
    }
    return refs;
}

static HRESULT WINAPI list_manager_QueryInterface(
    INetworkListManager *iface,
    REFIID riid,
    void **obj )
{
    struct list_manager *mgr = impl_from_INetworkListManager( iface );

    TRACE( "%p, %s, %p\n", mgr, debugstr_guid(riid), obj );

    if (IsEqualGUID( riid, &IID_INetworkListManager ) ||
        IsEqualGUID( riid, &IID_IDispatch ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *obj = iface;
    }
    else if (IsEqualGUID( riid, &IID_INetworkCostManager ))
    {
        *obj = &mgr->INetworkCostManager_iface;
    }
    else if (IsEqualGUID( riid, &IID_IConnectionPointContainer ))
    {
        *obj = &mgr->IConnectionPointContainer_iface;
    }
    else
    {
        FIXME( "interface %s not implemented\n", debugstr_guid(riid) );
        *obj = NULL;
        return E_NOINTERFACE;
    }
    INetworkListManager_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI list_manager_GetTypeInfoCount(
    INetworkListManager *iface,
    UINT *count )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI list_manager_GetTypeInfo(
    INetworkListManager *iface,
    UINT index,
    LCID lcid,
    ITypeInfo **info )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI list_manager_GetIDsOfNames(
    INetworkListManager *iface,
    REFIID riid,
    LPOLESTR *names,
    UINT count,
    LCID lcid,
    DISPID *dispid )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI list_manager_Invoke(
    INetworkListManager *iface,
    DISPID member,
    REFIID riid,
    LCID lcid,
    WORD flags,
    DISPPARAMS *params,
    VARIANT *result,
    EXCEPINFO *excep_info,
    UINT *arg_err )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI list_manager_GetNetworks(
    INetworkListManager *iface,
    NLM_ENUM_NETWORK Flags,
    IEnumNetworks **ppEnumNetwork )
{
    FIXME( "%p, %x, %p\n", iface, Flags, ppEnumNetwork );
    return E_NOTIMPL;
}

static HRESULT WINAPI list_manager_GetNetwork(
    INetworkListManager *iface,
    GUID gdNetworkId,
    INetwork **ppNetwork )
{
    FIXME( "%p, %s, %p\n", iface, debugstr_guid(&gdNetworkId), ppNetwork );
    return E_NOTIMPL;
}

static HRESULT WINAPI list_manager_GetNetworkConnections(
    INetworkListManager *iface,
    IEnumNetworkConnections **ppEnum )
{
    FIXME( "%p, %p\n", iface, ppEnum );
    return E_NOTIMPL;
}

static HRESULT WINAPI list_manager_GetNetworkConnection(
    INetworkListManager *iface,
    GUID gdNetworkConnectionId,
    INetworkConnection **ppNetworkConnection )
{
    FIXME( "%p, %s, %p\n", iface, debugstr_guid(&gdNetworkConnectionId),
            ppNetworkConnection );
    return E_NOTIMPL;
}

static HRESULT WINAPI list_manager_IsConnectedToInternet(
    INetworkListManager *iface,
    VARIANT_BOOL *pbIsConnected )
{
    FIXME( "%p, %p\n", iface, pbIsConnected );

    *pbIsConnected = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI list_manager_IsConnected(
    INetworkListManager *iface,
    VARIANT_BOOL *pbIsConnected )
{
    FIXME( "%p, %p\n", iface, pbIsConnected );

    *pbIsConnected = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI list_manager_GetConnectivity(
    INetworkListManager *iface,
    NLM_CONNECTIVITY *pConnectivity )
{
    FIXME( "%p, %p\n", iface, pConnectivity );

    *pConnectivity = NLM_CONNECTIVITY_IPV4_INTERNET;
    return S_OK;
}

static const INetworkListManagerVtbl list_manager_vtbl =
{
    list_manager_QueryInterface,
    list_manager_AddRef,
    list_manager_Release,
    list_manager_GetTypeInfoCount,
    list_manager_GetTypeInfo,
    list_manager_GetIDsOfNames,
    list_manager_Invoke,
    list_manager_GetNetworks,
    list_manager_GetNetwork,
    list_manager_GetNetworkConnections,
    list_manager_GetNetworkConnection,
    list_manager_IsConnectedToInternet,
    list_manager_IsConnected,
    list_manager_GetConnectivity
};

static HRESULT WINAPI ConnectionPointContainer_QueryInterface(IConnectionPointContainer *iface,
                                                              REFIID riid, void **ppv)
{
    struct list_manager *This = impl_from_IConnectionPointContainer( iface );
    return INetworkListManager_QueryInterface(&This->INetworkListManager_iface, riid, ppv);
}

static ULONG WINAPI ConnectionPointContainer_AddRef(IConnectionPointContainer *iface)
{
    struct list_manager *This = impl_from_IConnectionPointContainer( iface );
    return INetworkListManager_AddRef(&This->INetworkListManager_iface);
}

static ULONG WINAPI ConnectionPointContainer_Release(IConnectionPointContainer *iface)
{
    struct list_manager *This = impl_from_IConnectionPointContainer( iface );
    return INetworkListManager_Release(&This->INetworkListManager_iface);
}

static HRESULT WINAPI ConnectionPointContainer_EnumConnectionPoints(IConnectionPointContainer *iface,
        IEnumConnectionPoints **ppEnum)
{
    struct list_manager *This = impl_from_IConnectionPointContainer( iface );
    FIXME("(%p)->(%p): stub\n", This, ppEnum);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPointContainer_FindConnectionPoint(IConnectionPointContainer *iface,
        REFIID riid, IConnectionPoint **cp)
{
    struct list_manager *This = impl_from_IConnectionPointContainer( iface );

    TRACE( "%p, %s, %p\n", This, debugstr_guid(riid), cp );

    if (!riid || !cp)
        return E_POINTER;

    if (IsEqualGUID( riid, &IID_INetworkListManagerEvents ) ||
        IsEqualGUID( riid, &IID_INetworkCostManagerEvents ) ||
        IsEqualGUID( riid, &IID_INetworkConnectionEvents ))
        return connection_point_create( cp, riid, iface );

    FIXME( "interface %s not implemented\n", debugstr_guid(riid) );

    *cp = NULL;
    return E_NOINTERFACE;
}

static const struct IConnectionPointContainerVtbl cpc_vtbl =
{
    ConnectionPointContainer_QueryInterface,
    ConnectionPointContainer_AddRef,
    ConnectionPointContainer_Release,
    ConnectionPointContainer_EnumConnectionPoints,
    ConnectionPointContainer_FindConnectionPoint
};

static inline struct connection *impl_from_INetworkConnection(
    INetworkConnection *iface )
{
    return CONTAINING_RECORD( iface, struct connection, INetworkConnection_iface );
}

static HRESULT WINAPI connection_QueryInterface(
    INetworkConnection *iface, REFIID riid, void **obj )
{
    struct connection *connection = impl_from_INetworkConnection( iface );

    TRACE( "%p, %s, %p\n", connection, debugstr_guid(riid), obj );

    if (IsEqualIID( riid, &IID_INetworkConnection ) ||
        IsEqualIID( riid, &IID_IDispatch ) ||
        IsEqualIID( riid, &IID_IUnknown ))
    {
        *obj = iface;
    }
    else if (IsEqualIID( riid, &IID_INetworkConnectionCost ))
    {
        *obj = &connection->INetworkConnectionCost_iface;
    }
    else
    {
        WARN( "interface not supported %s\n", debugstr_guid(riid) );
        *obj = NULL;
        return E_NOINTERFACE;
    }
    INetworkConnection_AddRef( iface );
    return S_OK;
}

static ULONG WINAPI connection_AddRef(
    INetworkConnection  *iface )
{
    struct connection *connection = impl_from_INetworkConnection( iface );

    TRACE( "%p\n", connection );
    return InterlockedIncrement( &connection->refs );
}

static ULONG WINAPI connection_Release(
    INetworkConnection  *iface )
{
    struct connection *connection = impl_from_INetworkConnection( iface );
    LONG refs;

    TRACE( "%p\n", connection );

    if (!(refs = InterlockedDecrement( &connection->refs )))
    {
        INetwork_Release( connection->network );
        list_remove( &connection->entry );
        heap_free( connection );
    }
    return refs;
}

static HRESULT WINAPI connection_GetTypeInfoCount(
    INetworkConnection *iface,
    UINT *count )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_GetTypeInfo(
    INetworkConnection *iface,
    UINT index,
    LCID lcid,
    ITypeInfo **info )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_GetIDsOfNames(
    INetworkConnection *iface,
    REFIID riid,
    LPOLESTR *names,
    UINT count,
    LCID lcid,
    DISPID *dispid )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_Invoke(
    INetworkConnection *iface,
    DISPID member,
    REFIID riid,
    LCID lcid,
    WORD flags,
    DISPPARAMS *params,
    VARIANT *result,
    EXCEPINFO *excep_info,
    UINT *arg_err )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_GetNetwork(
    INetworkConnection *iface,
    INetwork **ppNetwork )
{
    struct connection *connection = impl_from_INetworkConnection( iface );

    TRACE( "%p, %p\n", iface, ppNetwork );

    *ppNetwork = connection->network;
    INetwork_AddRef( *ppNetwork );
    return S_OK;
}

static HRESULT WINAPI connection_get_IsConnectedToInternet(
    INetworkConnection *iface,
    VARIANT_BOOL *pbIsConnected )
{
    struct connection *connection = impl_from_INetworkConnection( iface );

    TRACE( "%p, %p\n", iface, pbIsConnected );

    *pbIsConnected = connection->connected_to_internet;
    return S_OK;
}

static HRESULT WINAPI connection_get_IsConnected(
    INetworkConnection *iface,
    VARIANT_BOOL *pbIsConnected )
{
    struct connection *connection = impl_from_INetworkConnection( iface );

    TRACE( "%p, %p\n", iface, pbIsConnected );

    *pbIsConnected = connection->connected;
    return S_OK;
}

static HRESULT WINAPI connection_GetConnectivity(
    INetworkConnection *iface,
    NLM_CONNECTIVITY *pConnectivity )
{
    FIXME( "%p, %p\n", iface, pConnectivity );

    *pConnectivity = NLM_CONNECTIVITY_IPV4_INTERNET;
    return S_OK;
}

static HRESULT WINAPI connection_GetConnectionId(
    INetworkConnection *iface,
    GUID *pgdConnectionId )
{
    struct connection *connection = impl_from_INetworkConnection( iface );

    TRACE( "%p, %p\n", iface, pgdConnectionId );

    *pgdConnectionId = connection->id;
    return S_OK;
}

static HRESULT WINAPI connection_GetAdapterId(
    INetworkConnection *iface,
    GUID *pgdAdapterId )
{
    struct connection *connection = impl_from_INetworkConnection( iface );

    FIXME( "%p, %p\n", iface, pgdAdapterId );

    *pgdAdapterId = connection->id;
    return S_OK;
}

static HRESULT WINAPI connection_GetDomainType(
    INetworkConnection *iface,
    NLM_DOMAIN_TYPE *pDomainType )
{
    FIXME( "%p, %p\n", iface, pDomainType );

    *pDomainType = NLM_DOMAIN_TYPE_NON_DOMAIN_NETWORK;
    return S_OK;
}

static const struct INetworkConnectionVtbl connection_vtbl =
{
    connection_QueryInterface,
    connection_AddRef,
    connection_Release,
    connection_GetTypeInfoCount,
    connection_GetTypeInfo,
    connection_GetIDsOfNames,
    connection_Invoke,
    connection_GetNetwork,
    connection_get_IsConnectedToInternet,
    connection_get_IsConnected,
    connection_GetConnectivity,
    connection_GetConnectionId,
    connection_GetAdapterId,
    connection_GetDomainType
};

static inline struct connection *impl_from_INetworkConnectionCost(
    INetworkConnectionCost *iface )
{
    return CONTAINING_RECORD( iface, struct connection, INetworkConnectionCost_iface );
}

static HRESULT WINAPI connection_cost_QueryInterface(
    INetworkConnectionCost *iface,
    REFIID riid,
    void **obj )
{
    struct connection *conn = impl_from_INetworkConnectionCost( iface );
    return INetworkConnection_QueryInterface( &conn->INetworkConnection_iface, riid, obj );
}

static ULONG WINAPI connection_cost_AddRef(
    INetworkConnectionCost *iface )
{
    struct connection *conn = impl_from_INetworkConnectionCost( iface );
    return INetworkConnection_AddRef( &conn->INetworkConnection_iface );
}

static ULONG WINAPI connection_cost_Release(
    INetworkConnectionCost *iface )
{
    struct connection *conn = impl_from_INetworkConnectionCost( iface );
    return INetworkConnection_Release( &conn->INetworkConnection_iface );
}

static HRESULT WINAPI connection_cost_GetCost(
    INetworkConnectionCost *iface, DWORD *pCost )
{
    FIXME( "%p, %p\n", iface, pCost );

    if (!pCost) return E_POINTER;

    *pCost = NLM_CONNECTION_COST_UNRESTRICTED;
    return S_OK;
}

static HRESULT WINAPI connection_cost_GetDataPlanStatus(
    INetworkConnectionCost *iface, NLM_DATAPLAN_STATUS *pDataPlanStatus )
{
    struct connection *conn = impl_from_INetworkConnectionCost( iface );

    FIXME( "%p, %p\n", iface, pDataPlanStatus );

    if (!pDataPlanStatus) return E_POINTER;

    memcpy( &pDataPlanStatus->InterfaceGuid, &conn->id, sizeof(conn->id) );
    pDataPlanStatus->UsageData.UsageInMegabytes = NLM_UNKNOWN_DATAPLAN_STATUS;
    memset( &pDataPlanStatus->UsageData.LastSyncTime, 0, sizeof(pDataPlanStatus->UsageData.LastSyncTime) );
    pDataPlanStatus->DataLimitInMegabytes       = NLM_UNKNOWN_DATAPLAN_STATUS;
    pDataPlanStatus->InboundBandwidthInKbps     = NLM_UNKNOWN_DATAPLAN_STATUS;
    pDataPlanStatus->OutboundBandwidthInKbps    = NLM_UNKNOWN_DATAPLAN_STATUS;
    memset( &pDataPlanStatus->NextBillingCycle, 0, sizeof(pDataPlanStatus->NextBillingCycle) );
    pDataPlanStatus->MaxTransferSizeInMegabytes = NLM_UNKNOWN_DATAPLAN_STATUS;
    pDataPlanStatus->Reserved                   = 0;

    return S_OK;
}

static const INetworkConnectionCostVtbl connection_cost_vtbl =
{
    connection_cost_QueryInterface,
    connection_cost_AddRef,
    connection_cost_Release,
    connection_cost_GetCost,
    connection_cost_GetDataPlanStatus
};

static struct connection *create_connection( const GUID *id )
{
    struct connection *ret;

    if (!(ret = heap_alloc( sizeof(*ret) ))) return NULL;

    ret->INetworkConnection_iface.lpVtbl     = &connection_vtbl;
    ret->INetworkConnectionCost_iface.lpVtbl = &connection_cost_vtbl;
    ret->refs                  = 1;
    ret->id                    = *id;
    ret->network               = NULL;
    ret->connected             = VARIANT_FALSE;
    ret->connected_to_internet = VARIANT_FALSE;
    list_init( &ret->entry );

    return ret;
}

static void init_networks( struct list_manager *mgr )
{
    DWORD size = 0;
    IP_ADAPTER_ADDRESSES *buf, *aa;
    GUID id;
    ULONG ret, flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
                       GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_INCLUDE_ALL_GATEWAYS;

    list_init( &mgr->networks );
    list_init( &mgr->connections );

    ret = GetAdaptersAddresses( WS_AF_UNSPEC, flags, NULL, NULL, &size );
    if (ret != ERROR_BUFFER_OVERFLOW) return;

    if (!(buf = heap_alloc( size ))) return;
    if (GetAdaptersAddresses( WS_AF_UNSPEC, flags, NULL, buf, &size )) return;

    memset( &id, 0, sizeof(id) );
    for (aa = buf; aa; aa = aa->Next)
    {
        struct network *network;
        struct connection *connection;

        id.Data1 = aa->IfIndex;

        /* assume a one-to-one mapping between networks and connections */
        if (!(network = create_network( &id ))) goto done;
        if (!(connection = create_connection( &id )))
        {
            INetwork_Release( &network->INetwork_iface );
            goto done;
        }

        if (aa->FirstUnicastAddress)
        {
            network->connected = VARIANT_TRUE;
            connection->connected = VARIANT_TRUE;
        }
        if (aa->FirstGatewayAddress)
        {
            network->connected_to_internet = VARIANT_TRUE;
            connection->connected_to_internet = VARIANT_TRUE;
        }

        connection->network = &network->INetwork_iface;
        INetwork_AddRef( connection->network );

        list_add_tail( &mgr->networks, &network->entry );
        list_add_tail( &mgr->connections, &connection->entry );
    }

done:
    heap_free( buf );
}

HRESULT list_manager_create( void **obj )
{
    struct list_manager *mgr;

    TRACE( "%p\n", obj );

    if (!(mgr = heap_alloc( sizeof(*mgr) ))) return E_OUTOFMEMORY;
    mgr->INetworkListManager_iface.lpVtbl = &list_manager_vtbl;
    mgr->INetworkCostManager_iface.lpVtbl = &cost_manager_vtbl;
    mgr->IConnectionPointContainer_iface.lpVtbl = &cpc_vtbl;
    init_networks( mgr );
    mgr->refs = 1;

    *obj = &mgr->INetworkListManager_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}
