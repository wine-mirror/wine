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

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
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

struct network
{
    INetwork             INetwork_iface;
    LONG                 refs;
    struct list          entry;
    GUID                 id;
    INetworkListManager *mgr;
    VARIANT_BOOL         connected_to_internet;
    VARIANT_BOOL         connected;
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

struct connection_point
{
    IConnectionPoint IConnectionPoint_iface;
    IConnectionPointContainer *container;
    IID iid;
    struct list sinks;
    DWORD cookie;
};

struct list_manager
{
    INetworkListManager INetworkListManager_iface;
    INetworkCostManager INetworkCostManager_iface;
    IConnectionPointContainer IConnectionPointContainer_iface;
    LONG                refs;
    struct list         networks;
    struct list         connections;
    struct connection_point list_mgr_cp;
    struct connection_point cost_mgr_cp;
    struct connection_point conn_mgr_cp;
    struct connection_point events_cp;
};

struct sink_entry
{
    struct list entry;
    DWORD cookie;
    IUnknown *unk;
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
    return IConnectionPointContainer_AddRef( cp->container );
}

static ULONG WINAPI connection_point_Release(
    IConnectionPoint *iface )
{
    struct connection_point *cp = impl_from_IConnectionPoint( iface );
    return IConnectionPointContainer_Release( cp->container );
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
    struct sink_entry *sink_entry;
    IUnknown *unk;
    HRESULT hr;

    FIXME( "%p, %p, %p - semi-stub\n", cp, sink, cookie );

    if (!sink || !cookie)
        return E_POINTER;

    hr = IUnknown_QueryInterface( sink, &cp->iid, (void**)&unk );
    if (FAILED(hr))
    {
        WARN( "iface %s not implemented by sink\n", debugstr_guid(&cp->iid) );
        return CO_E_FAILEDTOOPENTHREADTOKEN;
    }

    sink_entry = malloc( sizeof(*sink_entry) );
    if (!sink_entry)
    {
        IUnknown_Release( unk );
        return E_OUTOFMEMORY;
    }

    sink_entry->unk = unk;
    *cookie = sink_entry->cookie = ++cp->cookie;
    list_add_tail( &cp->sinks, &sink_entry->entry );
    return S_OK;
}

static void sink_entry_release( struct sink_entry *entry )
{
    list_remove( &entry->entry );
    IUnknown_Release( entry->unk );
    free( entry );
}

static HRESULT WINAPI connection_point_Unadvise(
    IConnectionPoint *iface,
    DWORD cookie )
{
    struct connection_point *cp = impl_from_IConnectionPoint( iface );
    struct sink_entry *iter;

    TRACE( "%p, %ld\n", cp, cookie );

    LIST_FOR_EACH_ENTRY( iter, &cp->sinks, struct sink_entry, entry )
    {
        if (iter->cookie != cookie) continue;
        sink_entry_release( iter );
        return S_OK;
    }

    WARN( "invalid cookie\n" );
    return OLE_E_NOCONNECTION;
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

static void connection_point_init(
    struct connection_point *cp,
    REFIID riid,
    IConnectionPointContainer *container )
{
    cp->IConnectionPoint_iface.lpVtbl = &connection_point_vtbl;
    cp->container = container;
    cp->cookie = 0;
    cp->iid = *riid;
    list_init( &cp->sinks );
}

static void connection_point_release( struct connection_point *cp )
{
    while (!list_empty( &cp->sinks ))
        sink_entry_release( LIST_ENTRY( list_head( &cp->sinks ), struct sink_entry, entry ) );
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
        INetworkListManager_Release( network->mgr );
        free( network );
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

static inline struct list_manager *impl_from_INetworkListManager(
    INetworkListManager *iface )
{
    return CONTAINING_RECORD( iface, struct list_manager, INetworkListManager_iface );
}

static HRESULT create_connections_enum(
    struct list_manager *, IEnumNetworkConnections** );

static HRESULT WINAPI network_GetNetworkConnections(
    INetwork *iface,
    IEnumNetworkConnections **ppEnum )
{
    struct network *network = impl_from_INetwork( iface );
    struct list_manager *mgr = impl_from_INetworkListManager( network->mgr );

    TRACE( "%p, %p\n", iface, ppEnum );
    return create_connections_enum( mgr, ppEnum );
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
    struct network *network = impl_from_INetwork( iface );

    FIXME( "%p, %p\n", iface, pConnectivity );

    *pConnectivity = NLM_CONNECTIVITY_DISCONNECTED;

    if (network->connected_to_internet)
        *pConnectivity |= NLM_CONNECTIVITY_IPV4_INTERNET;
    else if (network->connected)
        *pConnectivity |= NLM_CONNECTIVITY_IPV4_LOCALNETWORK;

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

    if (!(ret = calloc( 1, sizeof(*ret) ))) return NULL;

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

    if (addr6->sin6_family != AF_INET6) return FALSE;

    for (i = 0; i < 5; i++)
        if (addr6->sin6_addr.u.Word[i]) return FALSE;

    if (addr6->sin6_addr.u.Word[5] != 0xffff) return FALSE;

    addr4->sin_family = AF_INET;
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

    if (dst && ((dst->sa_family == AF_INET && (dst4 = (SOCKADDR_IN *)dst)) ||
               ((dst->sa_family == AF_INET6 && map_address_6to4( (const SOCKADDR_IN6 *)dst, &addr4 )
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

struct networks_enum
{
    IEnumNetworks        IEnumNetworks_iface;
    LONG                 refs;
    struct list_manager *mgr;
    struct list         *cursor;
    NLM_ENUM_NETWORK     flags;
};

static inline struct networks_enum *impl_from_IEnumNetworks(
    IEnumNetworks *iface )
{
    return CONTAINING_RECORD( iface, struct networks_enum, IEnumNetworks_iface );
}

static HRESULT WINAPI networks_enum_QueryInterface(
    IEnumNetworks *iface, REFIID riid, void **obj )
{
    struct networks_enum *iter = impl_from_IEnumNetworks( iface );

    TRACE( "%p, %s, %p\n", iter, debugstr_guid(riid), obj );

    if (IsEqualIID( riid, &IID_IEnumNetworks ) ||
        IsEqualIID( riid, &IID_IDispatch ) ||
        IsEqualIID( riid, &IID_IUnknown ))
    {
        *obj = iface;
        IEnumNetworks_AddRef( iface );
        return S_OK;
    }
    else
    {
        WARN( "interface not supported %s\n", debugstr_guid(riid) );
        *obj = NULL;
        return E_NOINTERFACE;
    }
}

static ULONG WINAPI networks_enum_AddRef(
    IEnumNetworks *iface )
{
    struct networks_enum *iter = impl_from_IEnumNetworks( iface );

    TRACE( "%p\n", iter );
    return InterlockedIncrement( &iter->refs );
}

static ULONG WINAPI networks_enum_Release(
    IEnumNetworks *iface )
{
    struct networks_enum *iter = impl_from_IEnumNetworks( iface );
    LONG refs;

    TRACE( "%p\n", iter );

    if (!(refs = InterlockedDecrement( &iter->refs )))
    {
        INetworkListManager_Release( &iter->mgr->INetworkListManager_iface );
        free( iter );
    }
    return refs;
}

static HRESULT WINAPI networks_enum_GetTypeInfoCount(
    IEnumNetworks *iface,
    UINT *count )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI networks_enum_GetTypeInfo(
    IEnumNetworks *iface,
    UINT index,
    LCID lcid,
    ITypeInfo **info )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI networks_enum_GetIDsOfNames(
    IEnumNetworks *iface,
    REFIID riid,
    LPOLESTR *names,
    UINT count,
    LCID lcid,
    DISPID *dispid )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI networks_enum_Invoke(
    IEnumNetworks *iface,
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

static HRESULT WINAPI networks_enum_get__NewEnum(
    IEnumNetworks *iface, IEnumVARIANT **ppEnumVar )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static BOOL match_enum_network_flags( NLM_ENUM_NETWORK flags, struct network *network )
{
    if (flags == NLM_ENUM_NETWORK_ALL) return TRUE;
    if (network->connected)
    {
        if (flags & NLM_ENUM_NETWORK_CONNECTED) return TRUE;
    }
    else if (flags & NLM_ENUM_NETWORK_DISCONNECTED) return TRUE;
    return FALSE;
}

static HRESULT WINAPI networks_enum_Next(
    IEnumNetworks *iface, ULONG count, INetwork **ret, ULONG *fetched )
{
    struct networks_enum *iter = impl_from_IEnumNetworks( iface );
    ULONG i = 0;

    TRACE( "%p, %lu %p %p\n", iter, count, ret, fetched );

    if (!ret) return E_POINTER;
    *ret = NULL;
    if (fetched) *fetched = 0;
    if (!count) return S_OK;

    while (iter->cursor && i < count)
    {
        struct network *network = LIST_ENTRY( iter->cursor, struct network, entry );
        if (match_enum_network_flags( iter->flags, network ))
        {
            ret[i] = &network->INetwork_iface;
            INetwork_AddRef( ret[i] );
            i++;
        }
        iter->cursor = list_next( &iter->mgr->networks, iter->cursor );
    }
    if (fetched) *fetched = i;

    return i < count ? S_FALSE : S_OK;
}

static HRESULT WINAPI networks_enum_Skip(
    IEnumNetworks *iface, ULONG count )
{
    struct networks_enum *iter = impl_from_IEnumNetworks( iface );

    TRACE( "%p, %lu\n", iter, count);

    if (!count) return S_OK;
    if (!iter->cursor) return S_FALSE;

    for (;;)
    {
        struct network *network;
        iter->cursor = list_next( &iter->mgr->networks, iter->cursor );
        if (!iter->cursor) break;
        network = LIST_ENTRY( iter->cursor, struct network, entry );
        if (match_enum_network_flags( iter->flags, network )) count--;
        if (!count) break;
    }

    return count ? S_FALSE : S_OK;
}

static HRESULT WINAPI networks_enum_Reset(
    IEnumNetworks *iface )
{
    struct networks_enum *iter = impl_from_IEnumNetworks( iface );

    TRACE( "%p\n", iter );

    iter->cursor = list_head( &iter->mgr->networks );
    return S_OK;
}

static HRESULT create_networks_enum(
    struct list_manager *, NLM_ENUM_NETWORK, IEnumNetworks ** );

static HRESULT WINAPI networks_enum_Clone(
    IEnumNetworks *iface, IEnumNetworks **ret )
{
    struct networks_enum *iter = impl_from_IEnumNetworks( iface );

    TRACE( "%p, %p\n", iter, ret );
    return create_networks_enum( iter->mgr, iter->flags, ret );
}

static const IEnumNetworksVtbl networks_enum_vtbl =
{
    networks_enum_QueryInterface,
    networks_enum_AddRef,
    networks_enum_Release,
    networks_enum_GetTypeInfoCount,
    networks_enum_GetTypeInfo,
    networks_enum_GetIDsOfNames,
    networks_enum_Invoke,
    networks_enum_get__NewEnum,
    networks_enum_Next,
    networks_enum_Skip,
    networks_enum_Reset,
    networks_enum_Clone
};

static HRESULT create_networks_enum(
    struct list_manager *mgr, NLM_ENUM_NETWORK flags, IEnumNetworks **ret )
{
    struct networks_enum *iter;

    *ret = NULL;
    if (!(iter = calloc( 1, sizeof(*iter) ))) return E_OUTOFMEMORY;

    iter->IEnumNetworks_iface.lpVtbl = &networks_enum_vtbl;
    iter->cursor = list_head( &mgr->networks );
    iter->mgr    = mgr;
    INetworkListManager_AddRef( &mgr->INetworkListManager_iface );
    iter->flags  = flags;
    iter->refs   = 1;

    *ret = &iter->IEnumNetworks_iface;
    return S_OK;
}

struct connections_enum
{
    IEnumNetworkConnections IEnumNetworkConnections_iface;
    LONG                    refs;
    struct list_manager    *mgr;
    struct list            *cursor;
};

static inline struct connections_enum *impl_from_IEnumNetworkConnections(
    IEnumNetworkConnections *iface )
{
    return CONTAINING_RECORD( iface, struct connections_enum, IEnumNetworkConnections_iface );
}

static HRESULT WINAPI connections_enum_QueryInterface(
    IEnumNetworkConnections *iface, REFIID riid, void **obj )
{
    struct connections_enum *iter = impl_from_IEnumNetworkConnections( iface );

    TRACE( "%p, %s, %p\n", iter, debugstr_guid(riid), obj );

    if (IsEqualIID( riid, &IID_IEnumNetworkConnections ) ||
        IsEqualIID( riid, &IID_IDispatch ) ||
        IsEqualIID( riid, &IID_IUnknown ))
    {
        *obj = iface;
        IEnumNetworkConnections_AddRef( iface );
        return S_OK;
    }
    else
    {
        WARN( "interface not supported %s\n", debugstr_guid(riid) );
        *obj = NULL;
        return E_NOINTERFACE;
    }
}

static ULONG WINAPI connections_enum_AddRef(
    IEnumNetworkConnections *iface )
{
    struct connections_enum *iter = impl_from_IEnumNetworkConnections( iface );

    TRACE( "%p\n", iter );
    return InterlockedIncrement( &iter->refs );
}

static ULONG WINAPI connections_enum_Release(
    IEnumNetworkConnections *iface )
{
    struct connections_enum *iter = impl_from_IEnumNetworkConnections( iface );
    LONG refs;

    TRACE( "%p\n", iter );

    if (!(refs = InterlockedDecrement( &iter->refs )))
    {
        INetworkListManager_Release( &iter->mgr->INetworkListManager_iface );
        free( iter );
    }
    return refs;
}

static HRESULT WINAPI connections_enum_GetTypeInfoCount(
    IEnumNetworkConnections *iface,
    UINT *count )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI connections_enum_GetTypeInfo(
    IEnumNetworkConnections *iface,
    UINT index,
    LCID lcid,
    ITypeInfo **info )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI connections_enum_GetIDsOfNames(
    IEnumNetworkConnections *iface,
    REFIID riid,
    LPOLESTR *names,
    UINT count,
    LCID lcid,
    DISPID *dispid )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI connections_enum_Invoke(
    IEnumNetworkConnections *iface,
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

static HRESULT WINAPI connections_enum_get__NewEnum(
    IEnumNetworkConnections *iface, IEnumVARIANT **ppEnumVar )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI connections_enum_Next(
    IEnumNetworkConnections *iface, ULONG count, INetworkConnection **ret, ULONG *fetched )
{
    struct connections_enum *iter = impl_from_IEnumNetworkConnections( iface );
    ULONG i = 0;

    TRACE( "%p, %lu %p %p\n", iter, count, ret, fetched );

    if (!ret) return E_POINTER;
    *ret = NULL;
    if (fetched) *fetched = 0;
    if (!count) return S_OK;

    while (iter->cursor && i < count)
    {
        struct connection *connection = LIST_ENTRY( iter->cursor, struct connection, entry );
        ret[i] = &connection->INetworkConnection_iface;
        INetworkConnection_AddRef( ret[i] );
        iter->cursor = list_next( &iter->mgr->connections, iter->cursor );
        i++;
    }
    if (fetched) *fetched = i;

    return i < count ? S_FALSE : S_OK;
}

static HRESULT WINAPI connections_enum_Skip(
    IEnumNetworkConnections *iface, ULONG count )
{
    struct connections_enum *iter = impl_from_IEnumNetworkConnections( iface );

    TRACE( "%p, %lu\n", iter, count);

    if (!count) return S_OK;
    if (!iter->cursor) return S_FALSE;

    while (count--)
    {
        iter->cursor = list_next( &iter->mgr->connections, iter->cursor );
        if (!iter->cursor) break;
    }

    return count ? S_FALSE : S_OK;
}

static HRESULT WINAPI connections_enum_Reset(
    IEnumNetworkConnections *iface )
{
    struct connections_enum *iter = impl_from_IEnumNetworkConnections( iface );

    TRACE( "%p\n", iter );

    iter->cursor = list_head( &iter->mgr->connections );
    return S_OK;
}

static HRESULT WINAPI connections_enum_Clone(
    IEnumNetworkConnections *iface, IEnumNetworkConnections **ret )
{
    struct connections_enum *iter = impl_from_IEnumNetworkConnections( iface );

    TRACE( "%p, %p\n", iter, ret );
    return create_connections_enum( iter->mgr, ret );
}

static const IEnumNetworkConnectionsVtbl connections_enum_vtbl =
{
    connections_enum_QueryInterface,
    connections_enum_AddRef,
    connections_enum_Release,
    connections_enum_GetTypeInfoCount,
    connections_enum_GetTypeInfo,
    connections_enum_GetIDsOfNames,
    connections_enum_Invoke,
    connections_enum_get__NewEnum,
    connections_enum_Next,
    connections_enum_Skip,
    connections_enum_Reset,
    connections_enum_Clone
};

static HRESULT create_connections_enum(
    struct list_manager *mgr, IEnumNetworkConnections **ret )
{
    struct connections_enum *iter;

    *ret = NULL;
    if (!(iter = calloc( 1, sizeof(*iter) ))) return E_OUTOFMEMORY;

    iter->IEnumNetworkConnections_iface.lpVtbl = &connections_enum_vtbl;
    iter->mgr         = mgr;
    INetworkListManager_AddRef( &mgr->INetworkListManager_iface );
    iter->cursor      = list_head( &iter->mgr->connections );
    iter->refs        = 1;

    *ret = &iter->IEnumNetworkConnections_iface;
    return S_OK;
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
        struct network *network, *next_network;
        struct connection *connection, *next_connection;

        TRACE( "destroying %p\n", mgr );

        connection_point_release( &mgr->events_cp );
        connection_point_release( &mgr->conn_mgr_cp );
        connection_point_release( &mgr->cost_mgr_cp );
        connection_point_release( &mgr->list_mgr_cp );
        LIST_FOR_EACH_ENTRY_SAFE( connection, next_connection, &mgr->connections, struct connection, entry )
        {
            INetworkConnection_Release( &connection->INetworkConnection_iface );
        }
        LIST_FOR_EACH_ENTRY_SAFE( network, next_network, &mgr->networks, struct network, entry )
        {
            INetwork_Release( &network->INetwork_iface );
        }
        free( mgr );
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
    struct list_manager *mgr = impl_from_INetworkListManager( iface );

    TRACE( "%p, %x, %p\n", iface, Flags, ppEnumNetwork );

    return create_networks_enum( mgr, Flags, ppEnumNetwork );
}

static HRESULT WINAPI list_manager_GetNetwork(
    INetworkListManager *iface,
    GUID gdNetworkId,
    INetwork **ppNetwork )
{
    struct list_manager *mgr = impl_from_INetworkListManager( iface );
    struct network *network;

    TRACE( "%p, %s, %p\n", iface, debugstr_guid(&gdNetworkId), ppNetwork );

    LIST_FOR_EACH_ENTRY( network, &mgr->networks, struct network, entry )
    {
        if (IsEqualGUID( &network->id, &gdNetworkId ))
        {
            *ppNetwork = &network->INetwork_iface;
            INetwork_AddRef( *ppNetwork );
            return S_OK;
        }
    }

    return S_FALSE;
}

static HRESULT WINAPI list_manager_GetNetworkConnections(
    INetworkListManager *iface,
    IEnumNetworkConnections **ppEnum )
{
    struct list_manager *mgr = impl_from_INetworkListManager( iface );

    TRACE( "%p, %p\n", iface, ppEnum );
    return create_connections_enum( mgr, ppEnum );
}

static HRESULT WINAPI list_manager_GetNetworkConnection(
    INetworkListManager *iface,
    GUID gdNetworkConnectionId,
    INetworkConnection **ppNetworkConnection )
{
    struct list_manager *mgr = impl_from_INetworkListManager( iface );
    struct connection *connection;

    TRACE( "%p, %s, %p\n", iface, debugstr_guid(&gdNetworkConnectionId),
            ppNetworkConnection );

    LIST_FOR_EACH_ENTRY( connection, &mgr->connections, struct connection, entry )
    {
        if (IsEqualGUID( &connection->id, &gdNetworkConnectionId ))
        {
            *ppNetworkConnection = &connection->INetworkConnection_iface;
            INetworkConnection_AddRef( *ppNetworkConnection );
            return S_OK;
        }
    }

    return S_FALSE;
}

static HRESULT WINAPI list_manager_IsConnectedToInternet(
    INetworkListManager *iface,
    VARIANT_BOOL *pbIsConnected )
{
    struct list_manager *mgr = impl_from_INetworkListManager( iface );
    struct network *network;

    TRACE( "%p, %p\n", iface, pbIsConnected );

    LIST_FOR_EACH_ENTRY( network, &mgr->networks, struct network, entry )
    {
        if (network->connected_to_internet)
        {
            *pbIsConnected = VARIANT_TRUE;
            return S_OK;
        }
    }

    *pbIsConnected = VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI list_manager_IsConnected(
    INetworkListManager *iface,
    VARIANT_BOOL *pbIsConnected )
{
    struct list_manager *mgr = impl_from_INetworkListManager( iface );
    struct network *network;

    TRACE( "%p, %p\n", iface, pbIsConnected );

    LIST_FOR_EACH_ENTRY( network, &mgr->networks, struct network, entry )
    {
        if (network->connected)
        {
            *pbIsConnected = VARIANT_TRUE;
            return S_OK;
        }
    }

    *pbIsConnected = VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI list_manager_GetConnectivity(
    INetworkListManager *iface,
    NLM_CONNECTIVITY *pConnectivity )
{
    struct list_manager *mgr = impl_from_INetworkListManager( iface );
    struct network *network;

    FIXME( "%p, %p\n", iface, pConnectivity );

    *pConnectivity = NLM_CONNECTIVITY_DISCONNECTED;

    LIST_FOR_EACH_ENTRY( network, &mgr->networks, struct network, entry )
    {
        if (network->connected_to_internet)
            *pConnectivity |= NLM_CONNECTIVITY_IPV4_INTERNET;
        else if (network->connected)
            *pConnectivity |= NLM_CONNECTIVITY_IPV4_LOCALNETWORK;
    }

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
    struct connection_point *ret;

    TRACE( "%p, %s, %p\n", This, debugstr_guid(riid), cp );

    if (!riid || !cp)
        return E_POINTER;

    if (IsEqualGUID( riid, &IID_INetworkListManagerEvents ))
        ret = &This->list_mgr_cp;
    else if (IsEqualGUID( riid, &IID_INetworkCostManagerEvents ))
        ret = &This->cost_mgr_cp;
    else if (IsEqualGUID( riid, &IID_INetworkConnectionEvents ))
        ret = &This->conn_mgr_cp;
    else if (IsEqualGUID( riid, &IID_INetworkEvents))
        ret = &This->events_cp;
    else
    {
        FIXME( "interface %s not implemented\n", debugstr_guid(riid) );
        *cp = NULL;
        return E_NOINTERFACE;
    }

    IConnectionPoint_AddRef( *cp = &ret->IConnectionPoint_iface );
    return S_OK;
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
        list_remove( &connection->entry );
        INetwork_Release( connection->network );
        free( connection );
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
    struct connection *connection = impl_from_INetworkConnection( iface );

    FIXME( "%p, %p\n", iface, pConnectivity );

    *pConnectivity = NLM_CONNECTIVITY_DISCONNECTED;

    if (connection->connected_to_internet)
        *pConnectivity |= NLM_CONNECTIVITY_IPV4_INTERNET;
    else if (connection->connected)
        *pConnectivity |= NLM_CONNECTIVITY_IPV4_LOCALNETWORK;

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

    if (!(ret = calloc( 1, sizeof(*ret) ))) return NULL;

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

static IP_ADAPTER_ADDRESSES *get_network_adapters(void)
{
    ULONG err, size = 4096, flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
                                    GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_INCLUDE_GATEWAYS;
    IP_ADAPTER_ADDRESSES *tmp, *ret;

    if (!(ret = malloc( size ))) return NULL;
    err = GetAdaptersAddresses( AF_UNSPEC, flags, NULL, ret, &size );
    while (err == ERROR_BUFFER_OVERFLOW)
    {
        if (!(tmp = realloc( ret, size ))) break;
        ret = tmp;
        err = GetAdaptersAddresses( AF_UNSPEC, flags, NULL, ret, &size );
    }
    if (err == ERROR_SUCCESS) return ret;
    free( ret );
    return NULL;
}

static void init_networks( struct list_manager *mgr )
{
    IP_ADAPTER_ADDRESSES *buf, *aa;
    GUID id;

    list_init( &mgr->networks );
    list_init( &mgr->connections );

    if (!(buf = get_network_adapters())) return;

    memset( &id, 0, sizeof(id) );
    for (aa = buf; aa; aa = aa->Next)
    {
        struct network *network;
        struct connection *connection;
        NET_LUID luid;

        ConvertInterfaceIndexToLuid(aa->IfIndex, &luid);
        ConvertInterfaceLuidToGuid(&luid, &id);

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

        network->mgr = &mgr->INetworkListManager_iface;
        INetworkListManager_AddRef( network->mgr );
        connection->network = &network->INetwork_iface;
        INetwork_AddRef( connection->network );

        list_add_tail( &mgr->networks, &network->entry );
        list_add_tail( &mgr->connections, &connection->entry );
    }

done:
    free( buf );
}

HRESULT list_manager_create( void **obj )
{
    struct list_manager *mgr;

    TRACE( "%p\n", obj );

    if (!(mgr = calloc( 1, sizeof(*mgr) ))) return E_OUTOFMEMORY;
    mgr->INetworkListManager_iface.lpVtbl = &list_manager_vtbl;
    mgr->INetworkCostManager_iface.lpVtbl = &cost_manager_vtbl;
    mgr->IConnectionPointContainer_iface.lpVtbl = &cpc_vtbl;
    init_networks( mgr );
    mgr->refs = 1;

    connection_point_init( &mgr->list_mgr_cp, &IID_INetworkListManagerEvents,
                           &mgr->IConnectionPointContainer_iface );
    connection_point_init( &mgr->cost_mgr_cp, &IID_INetworkCostManagerEvents,
                           &mgr->IConnectionPointContainer_iface);
    connection_point_init( &mgr->conn_mgr_cp, &IID_INetworkConnectionEvents,
                           &mgr->IConnectionPointContainer_iface );
    connection_point_init( &mgr->events_cp, &IID_INetworkEvents,
                           &mgr->IConnectionPointContainer_iface );

    *obj = &mgr->INetworkListManager_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}
