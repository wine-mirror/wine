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

DEFINE_IINSPECTABLE( network_information_statics, INetworkInformationStatics, struct network_information_statics, IActivationFactory_iface )

static HRESULT WINAPI network_information_statics_GetConnectionProfiles( INetworkInformationStatics *iface, IVectorView_ConnectionProfile **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI network_information_statics_GetInternetConnectionProfile( INetworkInformationStatics *iface, IConnectionProfile **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
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
