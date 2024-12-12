/* WinRT Windows.Networking.Connectivity.NetworkInformation Implementation
 *
 * Copyright (C) 2024 Mohamad Al-Jaf
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

#include "private.h"
#include "initguid.h"
#include "netlistmgr.h"

WINE_DEFAULT_DEBUG_CHANNEL(connectivity);

static EventRegistrationToken dummy_cookie = {.value = 0xdeadbeef};

struct network_information_statics
{
    IActivationFactory IActivationFactory_iface;
    INetworkInformationStatics INetworkInformationStatics_iface;
    LONG ref;
};

static inline struct network_information_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct network_information_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct network_information_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        *out = &impl->IActivationFactory_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_INetworkInformationStatics ))
    {
        *out = &impl->INetworkInformationStatics_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct network_information_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct network_information_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI factory_GetIids( IActivationFactory *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetRuntimeClassName( IActivationFactory *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetTrustLevel( IActivationFactory *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_ActivateInstance( IActivationFactory *iface, IInspectable **instance )
{
    FIXME( "iface %p, instance %p stub!\n", iface, instance );
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl factory_vtbl =
{
    factory_QueryInterface,
    factory_AddRef,
    factory_Release,
    /* IInspectable methods */
    factory_GetIids,
    factory_GetRuntimeClassName,
    factory_GetTrustLevel,
    /* IActivationFactory methods */
    factory_ActivateInstance,
};

struct connection_profile
{
    IConnectionProfile IConnectionProfile_iface;
    IConnectionProfile2 IConnectionProfile2_iface;
    LONG ref;

    INetworkListManager *network_list_manager;
};

static inline struct connection_profile *impl_from_IConnectionProfile( IConnectionProfile *iface )
{
    return CONTAINING_RECORD( iface, struct connection_profile, IConnectionProfile_iface );
}

static HRESULT WINAPI connection_profile_QueryInterface( IConnectionProfile *iface, REFIID iid, void **out )
{
    struct connection_profile *impl = impl_from_IConnectionProfile( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IConnectionProfile ))
    {
        *out = &impl->IConnectionProfile_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IConnectionProfile2 ))
    {
        *out = &impl->IConnectionProfile2_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }
    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI connection_profile_AddRef( IConnectionProfile *iface )
{
    struct connection_profile *impl = impl_from_IConnectionProfile( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI connection_profile_Release( IConnectionProfile *iface )
{
    struct connection_profile *impl = impl_from_IConnectionProfile( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref)
    {
        INetworkListManager_Release( impl->network_list_manager );
        free( impl );
    }
    return ref;
}

static HRESULT WINAPI connection_profile_GetIids( IConnectionProfile *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_profile_GetRuntimeClassName( IConnectionProfile *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_profile_GetTrustLevel( IConnectionProfile *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_profile_get_ProfileName( IConnectionProfile *iface, HSTRING *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_profile_GetNetworkConnectivityLevel( IConnectionProfile *iface, NetworkConnectivityLevel *value )
{
    struct connection_profile *impl = impl_from_IConnectionProfile( iface );
    NetworkConnectivityLevel network_connectivity_level;
    NLM_CONNECTIVITY connectivity = 0xdeadbeef;
    HRESULT hr;

    TRACE( "iface %p, value %p\n", iface, value );

    if (!value) return E_POINTER;
    if (FAILED(hr = INetworkListManager_GetConnectivity( impl->network_list_manager, &connectivity ))) return hr;

    if (connectivity == NLM_CONNECTIVITY_DISCONNECTED)
        network_connectivity_level = NetworkConnectivityLevel_None;
    else if (connectivity & ( NLM_CONNECTIVITY_IPV4_INTERNET | NLM_CONNECTIVITY_IPV6_INTERNET ))
        network_connectivity_level = NetworkConnectivityLevel_InternetAccess;
    else if (connectivity & ( NLM_CONNECTIVITY_IPV4_LOCALNETWORK | NLM_CONNECTIVITY_IPV6_LOCALNETWORK | NLM_CONNECTIVITY_IPV4_NOTRAFFIC | NLM_CONNECTIVITY_IPV6_NOTRAFFIC ))
        network_connectivity_level = NetworkConnectivityLevel_LocalAccess;
    else
        network_connectivity_level = NetworkConnectivityLevel_ConstrainedInternetAccess;

    *value = network_connectivity_level;
    return S_OK;
}

static HRESULT WINAPI connection_profile_GetNetworkNames( IConnectionProfile *iface, IVectorView_HSTRING **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_profile_GetConnectionCost( IConnectionProfile *iface, IConnectionCost **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_profile_GetDataPlanStatus( IConnectionProfile *iface, IDataPlanStatus **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_profile_get_NetworkAdapter( IConnectionProfile *iface, INetworkAdapter **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_profile_GetLocalUsage( IConnectionProfile *iface, DateTime start, DateTime end, IDataUsage **value )
{
    FIXME( "iface %p, start %I64d, end %I64d, value %p stub!\n", iface, start.UniversalTime, end.UniversalTime, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_profile_GetLocalUsagePerRoamingStates( IConnectionProfile *iface, DateTime start, DateTime end, RoamingStates states, IDataUsage **value )
{
    FIXME( "iface %p, start %I64d, end %I64d, states %u, value %p stub!\n", iface, start.UniversalTime, end.UniversalTime, states, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_profile_get_NetworkSecuritySettings( IConnectionProfile *iface, INetworkSecuritySettings **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static const struct IConnectionProfileVtbl connection_profile_vtbl =
{
    connection_profile_QueryInterface,
    connection_profile_AddRef,
    connection_profile_Release,
    /* IInspectable methods */
    connection_profile_GetIids,
    connection_profile_GetRuntimeClassName,
    connection_profile_GetTrustLevel,
    /* IConnectionProfile methods */
    connection_profile_get_ProfileName,
    connection_profile_GetNetworkConnectivityLevel,
    connection_profile_GetNetworkNames,
    connection_profile_GetConnectionCost,
    connection_profile_GetDataPlanStatus,
    connection_profile_get_NetworkAdapter,
    connection_profile_GetLocalUsage,
    connection_profile_GetLocalUsagePerRoamingStates,
    connection_profile_get_NetworkSecuritySettings,
};

static inline struct connection_profile *impl_from_IConnectionProfile2( IConnectionProfile2 *iface )
{
    return CONTAINING_RECORD( iface, struct connection_profile, IConnectionProfile2_iface );
}

static HRESULT WINAPI connection_profile2_QueryInterface( IConnectionProfile2 *iface, REFIID iid, void **out )
{
    struct connection_profile *impl = impl_from_IConnectionProfile2( iface );
    return IConnectionProfile_QueryInterface(&impl->IConnectionProfile_iface, iid, out);
}

static ULONG WINAPI connection_profile2_AddRef( IConnectionProfile2 *iface )
{
    struct connection_profile *impl = impl_from_IConnectionProfile2( iface );
    return IConnectionProfile_AddRef(&impl->IConnectionProfile_iface);
}

static ULONG WINAPI connection_profile2_Release( IConnectionProfile2 *iface )
{
    struct connection_profile *impl = impl_from_IConnectionProfile2( iface );
    return IConnectionProfile_Release(&impl->IConnectionProfile_iface);
}

static HRESULT WINAPI connection_profile2_GetIids( IConnectionProfile2 *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_profile2_GetRuntimeClassName( IConnectionProfile2 *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_profile2_GetTrustLevel( IConnectionProfile2 *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

HRESULT WINAPI connection_profile2_get_IsWwanConnectionProfile( IConnectionProfile2 *iface, boolean *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    *value = 0;
    return S_OK;
}

HRESULT WINAPI connection_profile2_get_IsWlanConnectionProfile( IConnectionProfile2 *iface, boolean *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    *value = 0;
    return S_OK;
}

HRESULT WINAPI connection_profile2_get_WwanConnectionProfileDetails( IConnectionProfile2 *iface, IWwanConnectionProfileDetails **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

HRESULT WINAPI connection_profile2_get_WlanConnectionProfileDetails( IConnectionProfile2 *iface, IWlanConnectionProfileDetails **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

HRESULT WINAPI connection_profile2_get_ServiceProviderGuid( IConnectionProfile2 *iface, IReference_GUID **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

HRESULT WINAPI connection_profile2_GetSignalBars( IConnectionProfile2 *iface, IReference_BYTE **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

HRESULT WINAPI connection_profile2_GetDomainConnectivityLevel( IConnectionProfile2 *iface, DomainConnectivityLevel *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

HRESULT WINAPI connection_profile2_GetNetworkUsageAsync( IConnectionProfile2 *iface, DateTime time_start, DateTime time_end,
        DataUsageGranularity granularity, NetworkUsageStates states, IAsyncOperation_IVectorView_NetworkUsage **value )
{
    FIXME( "iface %p, time_start %I64d, time_end %I64d, granularity %d, states %d-%d, value %p stub!\n",
            iface, time_start.UniversalTime, time_end.UniversalTime, granularity, states.Roaming, states.Shared, value );
    return E_NOTIMPL;
}

HRESULT WINAPI connection_profile2_GetConnectivityIntervalsAsync( IConnectionProfile2 *iface, DateTime time_start, DateTime time_end,
        NetworkUsageStates states, IAsyncOperation_IVectorView_ConnectivityInterval **value )
{
    FIXME( "iface %p, time_start %I64d, time_end %I64d, states %d-%d, value %p stub!\n",
            iface, time_start.UniversalTime, time_end.UniversalTime, states.Roaming, states.Shared, value );
    return E_NOTIMPL;
}

static const struct IConnectionProfile2Vtbl connection_profile2_vtbl =
{
    connection_profile2_QueryInterface,
    connection_profile2_AddRef,
    connection_profile2_Release,
    /* IInspectable methods */
    connection_profile2_GetIids,
    connection_profile2_GetRuntimeClassName,
    connection_profile2_GetTrustLevel,
    /* IConnectionProfile2 methods */
    connection_profile2_get_IsWwanConnectionProfile,
    connection_profile2_get_IsWlanConnectionProfile,
    connection_profile2_get_WwanConnectionProfileDetails,
    connection_profile2_get_WlanConnectionProfileDetails,
    connection_profile2_get_ServiceProviderGuid,
    connection_profile2_GetSignalBars,
    connection_profile2_GetDomainConnectivityLevel,
    connection_profile2_GetNetworkUsageAsync,
    connection_profile2_GetConnectivityIntervalsAsync,
};

DEFINE_IINSPECTABLE( network_information_statics, INetworkInformationStatics, struct network_information_statics, IActivationFactory_iface )

static HRESULT WINAPI network_information_statics_GetConnectionProfiles( INetworkInformationStatics *iface, IVectorView_ConnectionProfile **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI network_information_statics_GetInternetConnectionProfile( INetworkInformationStatics *iface, IConnectionProfile **value )
{
    NetworkConnectivityLevel network_connectivity_level = 0xdeadbeef;
    INetworkListManager *network_list_manager = NULL;
    struct connection_profile *impl;
    HRESULT hr;

    TRACE( "iface %p, value %p\n", iface, value );

    *value = NULL;
    if (FAILED(hr = CoCreateInstance( &CLSID_NetworkListManager, NULL, CLSCTX_INPROC_SERVER, &IID_INetworkListManager, (void **)&network_list_manager ))) return hr;
    if (!(impl = calloc( 1, sizeof( *impl ) )))
    {
        INetworkListManager_Release( network_list_manager );
        return E_OUTOFMEMORY;
    }

    impl->IConnectionProfile_iface.lpVtbl = &connection_profile_vtbl;
    impl->IConnectionProfile2_iface.lpVtbl = &connection_profile2_vtbl;
    impl->ref = 1;
    impl->network_list_manager = network_list_manager;

    hr = IConnectionProfile_GetNetworkConnectivityLevel( &impl->IConnectionProfile_iface, &network_connectivity_level );
    if (FAILED(hr) || network_connectivity_level == NetworkConnectivityLevel_None)
    {
        free( impl );
        INetworkListManager_Release( network_list_manager );
        return hr;
    }

    *value = &impl->IConnectionProfile_iface;
    TRACE( "created IConnectionProfile %p.\n", *value );
    return hr;
}

static HRESULT WINAPI network_information_statics_GetLanIdentifiers( INetworkInformationStatics *iface, IVectorView_LanIdentifier **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI network_information_statics_GetHostNames( INetworkInformationStatics *iface, IVectorView_HostName **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI network_information_statics_GetProxyConfigurationAsync( INetworkInformationStatics *iface, IUriRuntimeClass *uri, IAsyncOperation_ProxyConfiguration **value )
{
    FIXME( "iface %p, uri %p, value %p stub!\n", iface, uri, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI network_information_statics_GetSortedEndpointPairs( INetworkInformationStatics *iface, IIterable_EndpointPair *endpoint,
                                                                          HostNameSortOptions options, IVectorView_EndpointPair **value )
{
    FIXME( "iface %p, endpoint %p, options %d, value %p stub!\n", iface, endpoint, options, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI network_information_statics_add_NetworkStatusChanged( INetworkInformationStatics *iface, INetworkStatusChangedEventHandler *handler,
                                                                            EventRegistrationToken *cookie )
{
    FIXME( "iface %p, handler %p, cookie %p stub!\n", iface, handler, cookie );
    *cookie = dummy_cookie;
    return S_OK;
}

static HRESULT WINAPI network_information_statics_remove_NetworkStatusChanged( INetworkInformationStatics *iface, EventRegistrationToken cookie )
{
    FIXME( "iface %p, cookie %#I64x stub!\n", iface, cookie.value );
    return S_OK;
}

static const struct INetworkInformationStaticsVtbl network_information_statics_vtbl =
{
    network_information_statics_QueryInterface,
    network_information_statics_AddRef,
    network_information_statics_Release,
    /* IInspectable methods */
    network_information_statics_GetIids,
    network_information_statics_GetRuntimeClassName,
    network_information_statics_GetTrustLevel,
    /* INetworkInformationStatics methods */
    network_information_statics_GetConnectionProfiles,
    network_information_statics_GetInternetConnectionProfile,
    network_information_statics_GetLanIdentifiers,
    network_information_statics_GetHostNames,
    network_information_statics_GetProxyConfigurationAsync,
    network_information_statics_GetSortedEndpointPairs,
    network_information_statics_add_NetworkStatusChanged,
    network_information_statics_remove_NetworkStatusChanged,
};

static struct network_information_statics network_information_statics =
{
    {&factory_vtbl},
    {&network_information_statics_vtbl},
    1,
};

IActivationFactory *network_information_factory = &network_information_statics.IActivationFactory_iface;
