/* WinRT Windows.Perception.Stub HolographicSpace Implementation
 *
 * Copyright (C) 2023 Mohamad Al-Jaf
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

WINE_DEFAULT_DEBUG_CHANNEL(perception);

struct holographicspace
{
    IActivationFactory IActivationFactory_iface;
    IHolographicSpaceStatics2 IHolographicSpaceStatics2_iface;
    IHolographicSpaceStatics3 IHolographicSpaceStatics3_iface;
    LONG ref;
};

static inline struct holographicspace *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct holographicspace, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct holographicspace *impl = impl_from_IActivationFactory( iface );

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

    if (IsEqualGUID( iid, &IID_IHolographicSpaceStatics2 ))
    {
        *out = &impl->IHolographicSpaceStatics2_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IHolographicSpaceStatics3 ))
    {
        *out = &impl->IHolographicSpaceStatics3_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct holographicspace *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct holographicspace *impl = impl_from_IActivationFactory( iface );
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

DEFINE_IINSPECTABLE( holographicspace_statics2, IHolographicSpaceStatics2, struct holographicspace, IActivationFactory_iface )

static HRESULT WINAPI holographicspace_statics2_get_IsSupported( IHolographicSpaceStatics2 *iface, boolean *value )
{
    TRACE( "iface %p, value %p\n", iface, value );

    *value = FALSE;
    return S_OK;
}

static HRESULT WINAPI holographicspace_statics2_get_IsAvailable( IHolographicSpaceStatics2 *iface, boolean *value )
{
    TRACE( "iface %p, value %p\n", iface, value );

    *value = FALSE;
    return S_OK;
}

static HRESULT WINAPI holographicspace_statics2_add_IsAvailableChanged( IHolographicSpaceStatics2 *iface, IEventHandler_IInspectable *handler,
                                                                        EventRegistrationToken *token )
{
    FIXME( "iface %p, handler %p, token %p stub!\n", iface, handler, token );
    return E_NOTIMPL;
}

static HRESULT WINAPI holographicspace_statics2_remove_IsAvailableChanged( IHolographicSpaceStatics2 *iface, EventRegistrationToken token )
{
    FIXME( "iface %p, token %#I64x stub!\n", iface, token.value );
    return E_NOTIMPL;
}

static const struct IHolographicSpaceStatics2Vtbl holographicspace_statics2_vtbl =
{
    holographicspace_statics2_QueryInterface,
    holographicspace_statics2_AddRef,
    holographicspace_statics2_Release,
    /* IInspectable methods */
    holographicspace_statics2_GetIids,
    holographicspace_statics2_GetRuntimeClassName,
    holographicspace_statics2_GetTrustLevel,
    /* IHolographicSpaceStatics2 methods */
    holographicspace_statics2_get_IsSupported,
    holographicspace_statics2_get_IsAvailable,
    holographicspace_statics2_add_IsAvailableChanged,
    holographicspace_statics2_remove_IsAvailableChanged,
};

DEFINE_IINSPECTABLE( holographicspace_statics3, IHolographicSpaceStatics3, struct holographicspace, IActivationFactory_iface )

static HRESULT WINAPI holographicspace_statics3_get_IsConfigured( IHolographicSpaceStatics3 *iface, boolean *value )
{
    TRACE( "iface %p, value %p\n", iface, value );

    *value = FALSE;
    return S_OK;
}

static const struct IHolographicSpaceStatics3Vtbl holographicspace_statics3_vtbl =
{
    holographicspace_statics3_QueryInterface,
    holographicspace_statics3_AddRef,
    holographicspace_statics3_Release,
    /* IInspectable methods */
    holographicspace_statics3_GetIids,
    holographicspace_statics3_GetRuntimeClassName,
    holographicspace_statics3_GetTrustLevel,
    /* IHolographicSpaceStatics3 methods */
    holographicspace_statics3_get_IsConfigured,
};

static struct holographicspace holographicspace_statics =
{
    {&factory_vtbl},
    {&holographicspace_statics2_vtbl},
    {&holographicspace_statics3_vtbl},
    1,
};

IActivationFactory *holographicspace_factory = &holographicspace_statics.IActivationFactory_iface;
