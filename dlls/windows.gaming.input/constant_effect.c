/* WinRT Windows.Gaming.Input implementation
 *
 * Copyright 2022 Rémi Bernon for CodeWeavers
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
#include "provider.h"

WINE_DEFAULT_DEBUG_CHANNEL(input);

struct constant_effect
{
    IConstantForceEffect IConstantForceEffect_iface;
    LONG ref;
};

static inline struct constant_effect *impl_from_IConstantForceEffect( IConstantForceEffect *iface )
{
    return CONTAINING_RECORD( iface, struct constant_effect, IConstantForceEffect_iface );
}

static HRESULT WINAPI effect_QueryInterface( IConstantForceEffect *iface, REFIID iid, void **out )
{
    struct constant_effect *impl = impl_from_IConstantForceEffect( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IConstantForceEffect ))
    {
        IInspectable_AddRef( (*out = &impl->IConstantForceEffect_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI effect_AddRef( IConstantForceEffect *iface )
{
    struct constant_effect *impl = impl_from_IConstantForceEffect( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI effect_Release( IConstantForceEffect *iface )
{
    struct constant_effect *impl = impl_from_IConstantForceEffect( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref) free( impl );

    return ref;
}

static HRESULT WINAPI effect_GetIids( IConstantForceEffect *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_GetRuntimeClassName( IConstantForceEffect *iface, HSTRING *class_name )
{
    return WindowsCreateString( RuntimeClass_Windows_Gaming_Input_ForceFeedback_ConstantForceEffect,
                                ARRAY_SIZE(RuntimeClass_Windows_Gaming_Input_ForceFeedback_ConstantForceEffect),
                                class_name );
}

static HRESULT WINAPI effect_GetTrustLevel( IConstantForceEffect *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_SetParameters( IConstantForceEffect *iface, Vector3 direction, TimeSpan duration )
{
    FIXME( "iface %p, direction %s, duration %I64u stub!\n", iface, debugstr_vector3( &direction ), duration.Duration );
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_SetParametersWithEnvelope( IConstantForceEffect *iface, Vector3 direction, FLOAT attack_gain,
                                                        FLOAT sustain_gain, FLOAT release_gain, TimeSpan start_delay,
                                                        TimeSpan attack_duration, TimeSpan sustain_duration,
                                                        TimeSpan release_duration, UINT32 repeat_count )
{
    FIXME( "iface %p, direction %s, attack_gain %f, sustain_gain %f, release_gain %f, start_delay %I64u, attack_duration %I64u, "
           "sustain_duration %I64u, release_duration %I64u, repeat_count %u stub!\n", iface, debugstr_vector3( &direction ),
           attack_gain, sustain_gain, release_gain, start_delay.Duration, attack_duration.Duration, sustain_duration.Duration,
           release_duration.Duration, repeat_count );
    return E_NOTIMPL;
}

static const struct IConstantForceEffectVtbl effect_vtbl =
{
    effect_QueryInterface,
    effect_AddRef,
    effect_Release,
    /* IInspectable methods */
    effect_GetIids,
    effect_GetRuntimeClassName,
    effect_GetTrustLevel,
    /* IConstantForceEffect methods */
    effect_SetParameters,
    effect_SetParametersWithEnvelope,
};

struct constant_factory
{
    IActivationFactory IActivationFactory_iface;
    LONG ref;
};

static inline struct constant_factory *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct constant_factory, IActivationFactory_iface );
}

static HRESULT WINAPI activation_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct constant_factory *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        IInspectable_AddRef( (*out = &impl->IActivationFactory_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI activation_AddRef( IActivationFactory *iface )
{
    struct constant_factory *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI activation_Release( IActivationFactory *iface )
{
    struct constant_factory *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI activation_GetIids( IActivationFactory *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_GetRuntimeClassName( IActivationFactory *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_GetTrustLevel( IActivationFactory *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_ActivateInstance( IActivationFactory *iface, IInspectable **instance )
{
    struct constant_effect *impl;

    TRACE( "iface %p, instance %p.\n", iface, instance );

    if (!(impl = calloc( 1, sizeof(struct constant_effect) ))) return E_OUTOFMEMORY;
    impl->IConstantForceEffect_iface.lpVtbl = &effect_vtbl;
    impl->ref = 1;

    *instance = (IInspectable *)&impl->IConstantForceEffect_iface;
    TRACE( "created ConstantForceEffect %p\n", *instance );
    return S_OK;
}

static const struct IActivationFactoryVtbl activation_vtbl =
{
    activation_QueryInterface,
    activation_AddRef,
    activation_Release,
    /* IInspectable methods */
    activation_GetIids,
    activation_GetRuntimeClassName,
    activation_GetTrustLevel,
    /* IActivationFactory methods */
    activation_ActivateInstance,
};

static struct constant_factory constant_statics =
{
    {&activation_vtbl},
    1,
};

IInspectable *constant_effect_factory = (IInspectable *)&constant_statics.IActivationFactory_iface;
