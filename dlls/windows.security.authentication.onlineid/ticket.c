/* WinRT Windows.Security.Authentication.Onlineid Implementation
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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(onlineid);

struct ticket_request_factory_statics
{
    IActivationFactory IActivationFactory_iface;
    IOnlineIdServiceTicketRequestFactory IOnlineIdServiceTicketRequestFactory_iface;
    LONG ref;
};

static inline struct ticket_request_factory_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct ticket_request_factory_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct ticket_request_factory_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        *out = &impl->IActivationFactory_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IOnlineIdServiceTicketRequestFactory ))
    {
        *out = &impl->IOnlineIdServiceTicketRequestFactory_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct ticket_request_factory_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct ticket_request_factory_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
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

DEFINE_IINSPECTABLE( ticket_request_factory_statics, IOnlineIdServiceTicketRequestFactory, struct ticket_request_factory_statics, IActivationFactory_iface )

static HRESULT WINAPI ticket_request_factory_statics_CreateOnlineIdServiceTicketRequest( IOnlineIdServiceTicketRequestFactory *iface, HSTRING service,
                                                                                         HSTRING policy, IOnlineIdServiceTicketRequest **request )
{
    FIXME( "iface %p, service %s, policy %s, request %p stub!\n", iface, debugstr_hstring( service ), debugstr_hstring( policy ), request );
    return E_NOTIMPL;
}

static HRESULT WINAPI ticket_request_factory_statics_CreateOnlineIdServiceTicketRequestAdvanced( IOnlineIdServiceTicketRequestFactory *iface, HSTRING service,
                                                                                                 IOnlineIdServiceTicketRequest **request )
{
    FIXME( "iface %p, service %s, request %p stub!\n", iface, debugstr_hstring( service ), request );
    return E_NOTIMPL;
}

static const struct IOnlineIdServiceTicketRequestFactoryVtbl ticket_request_factory_statics_vtbl =
{
    ticket_request_factory_statics_QueryInterface,
    ticket_request_factory_statics_AddRef,
    ticket_request_factory_statics_Release,
    /* IInspectable methods */
    ticket_request_factory_statics_GetIids,
    ticket_request_factory_statics_GetRuntimeClassName,
    ticket_request_factory_statics_GetTrustLevel,
    /* IOnlineIdServiceTicketRequestFactory methods */
    ticket_request_factory_statics_CreateOnlineIdServiceTicketRequest,
    ticket_request_factory_statics_CreateOnlineIdServiceTicketRequestAdvanced,
};

static struct ticket_request_factory_statics ticket_request_factory_statics =
{
    {&factory_vtbl},
    {&ticket_request_factory_statics_vtbl},
    1,
};

IActivationFactory *ticket_factory = &ticket_request_factory_statics.IActivationFactory_iface;
