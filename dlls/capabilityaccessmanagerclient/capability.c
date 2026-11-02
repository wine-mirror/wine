/* Copyright 2026 Paul Gofman for CodeWeavers
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

WINE_DEFAULT_DEBUG_CHANNEL(capability);

struct capability_statics
{
    IActivationFactory IActivationFactory_iface;
    IAppCapabilityStatics IAppCapabilityStatics_iface;
    LONG ref;
};

static struct capability_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct capability_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct capability_statics *impl = impl_from_IActivationFactory( iface );

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

    if (IsEqualGUID( iid, &IID_IAppCapabilityStatics ))
    {
        *out = &impl->IAppCapabilityStatics_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct capability_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct capability_statics *impl = impl_from_IActivationFactory( iface );
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

DEFINE_IINSPECTABLE( capability_statics, IAppCapabilityStatics, struct capability_statics, IActivationFactory_iface )

static HRESULT WINAPI capability_statics_RequestAccessForCapabilitiesAsync( IAppCapabilityStatics *iface,
        IIterable_HSTRING *cap_names,
        IAsyncOperation_IMapView_HSTRING_AppCapabilityAccessStatus **operations )
{
    FIXME( "iface %p, cap_names %p, operations %p stub.\n", iface, cap_names, operations );
    return E_NOTIMPL;
}

static HRESULT WINAPI capability_statics_RequestAccessForCapabilitiesForUserAsync( IAppCapabilityStatics *iface,
            __x_ABI_CWindows_CSystem_CIUser *user, IIterable_HSTRING *cap_names,
            IAsyncOperation_IMapView_HSTRING_AppCapabilityAccessStatus **operations )
{
    FIXME( "iface %p, user %p, cap_names %p, operations %p.\n", iface, user, cap_names, operations );
    return E_NOTIMPL;
}

static HRESULT WINAPI capability_statics_Create( IAppCapabilityStatics *iface, HSTRING cap_name, IAppCapability **result )
{
    FIXME( "iface %p, cap_name %s, result %p.\n", iface, debugstr_hstring(cap_name), result );
    return E_NOTIMPL;
}

static HRESULT WINAPI capability_statics_CreateWithProcessIdForUser( IAppCapabilityStatics *iface,
        __x_ABI_CWindows_CSystem_CIUser *user, HSTRING cap_name, UINT32 pid, IAppCapability **result )
{
    FIXME( "iface %p, user %p, cap_name %s, pid %#x, result %p.\n", iface, user, debugstr_hstring(cap_name), pid, result );
    return E_NOTIMPL;
}

static const struct IAppCapabilityStaticsVtbl capability_statics_vtbl =
{
    capability_statics_QueryInterface,
    capability_statics_AddRef,
    capability_statics_Release,
    /* IInspectable methods */
    capability_statics_GetIids,
    capability_statics_GetRuntimeClassName,
    capability_statics_GetTrustLevel,
    /* IAppCapabilityStatics methods */
    capability_statics_RequestAccessForCapabilitiesAsync,
    capability_statics_RequestAccessForCapabilitiesForUserAsync,
    capability_statics_Create,
    capability_statics_CreateWithProcessIdForUser,
};

static struct capability_statics capability_statics =
{
    {&factory_vtbl},
    {&capability_statics_vtbl},
    1,
};

IActivationFactory *cap_access_factory = &capability_statics.IActivationFactory_iface;
