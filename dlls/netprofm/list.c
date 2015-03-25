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
#include "initguid.h"
#include "objbase.h"
#include "ocidl.h"
#include "netlistmgr.h"
#include "olectl.h"

#include "wine/debug.h"
#include "netprofm_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(netprofm);

struct list_manager
{
    INetworkListManager INetworkListManager_iface;
    INetworkCostManager INetworkCostManager_iface;
    IConnectionPointContainer IConnectionPointContainer_iface;
    LONG                refs;
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
        HeapFree( GetProcessHeap(), 0, cp );
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

    if (!(cp = HeapAlloc( GetProcessHeap(), 0, sizeof(*cp) ))) return E_OUTOFMEMORY;
    cp->IConnectionPoint_iface.lpVtbl = &connection_point_vtbl;
    cp->container = container;
    cp->refs = 1;

    memcpy( &cp->iid, riid, sizeof(*riid) );
    IConnectionPointContainer_AddRef( container );

    *obj = &cp->IConnectionPoint_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
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

static HRESULT WINAPI cost_manager_GetDataPlanStatus(
    INetworkCostManager *iface, NLM_DATAPLAN_STATUS *pDataPlanStatus,
    NLM_SOCKADDR *pDestIPAddr)
{
    FIXME( "%p, %p, %p\n", iface, pDataPlanStatus, pDestIPAddr );
    return E_NOTIMPL;
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
        TRACE( "destroying %p\n", mgr );
        HeapFree( GetProcessHeap(), 0, mgr );
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

    if (IsEqualGUID( riid, &IID_INetworkListManagerEvents ))
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

HRESULT list_manager_create( void **obj )
{
    struct list_manager *mgr;

    TRACE( "%p\n", obj );

    if (!(mgr = HeapAlloc( GetProcessHeap(), 0, sizeof(*mgr) ))) return E_OUTOFMEMORY;
    mgr->INetworkListManager_iface.lpVtbl = &list_manager_vtbl;
    mgr->INetworkCostManager_iface.lpVtbl = &cost_manager_vtbl;
    mgr->IConnectionPointContainer_iface.lpVtbl = &cpc_vtbl;
    mgr->refs = 1;

    *obj = &mgr->INetworkListManager_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}
