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

struct periodic_effect
{
    IPeriodicForceEffect IPeriodicForceEffect_iface;
    IWineForceFeedbackEffectImpl *IWineForceFeedbackEffectImpl_inner;
    LONG ref;

    PeriodicForceEffectKind kind;
};

static inline struct periodic_effect *impl_from_IPeriodicForceEffect( IPeriodicForceEffect *iface )
{
    return CONTAINING_RECORD( iface, struct periodic_effect, IPeriodicForceEffect_iface );
}

static HRESULT WINAPI effect_QueryInterface( IPeriodicForceEffect *iface, REFIID iid, void **out )
{
    struct periodic_effect *impl = impl_from_IPeriodicForceEffect( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IPeriodicForceEffect ))
    {
        IInspectable_AddRef( (*out = &impl->IPeriodicForceEffect_iface) );
        return S_OK;
    }

    return IWineForceFeedbackEffectImpl_QueryInterface( impl->IWineForceFeedbackEffectImpl_inner, iid, out );
}

static ULONG WINAPI effect_AddRef( IPeriodicForceEffect *iface )
{
    struct periodic_effect *impl = impl_from_IPeriodicForceEffect( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI effect_Release( IPeriodicForceEffect *iface )
{
    struct periodic_effect *impl = impl_from_IPeriodicForceEffect( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref)
    {
        /* guard against re-entry if inner releases an outer iface */
        InterlockedIncrement( &impl->ref );
        IWineForceFeedbackEffectImpl_Release( impl->IWineForceFeedbackEffectImpl_inner );
        free( impl );
    }

    return ref;
}

static HRESULT WINAPI effect_GetIids( IPeriodicForceEffect *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_GetRuntimeClassName( IPeriodicForceEffect *iface, HSTRING *class_name )
{
    return WindowsCreateString( RuntimeClass_Windows_Gaming_Input_ForceFeedback_PeriodicForceEffect,
                                ARRAY_SIZE(RuntimeClass_Windows_Gaming_Input_ForceFeedback_PeriodicForceEffect),
                                class_name );
}

static HRESULT WINAPI effect_GetTrustLevel( IPeriodicForceEffect *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_get_Kind( IPeriodicForceEffect *iface, PeriodicForceEffectKind *kind )
{
    struct periodic_effect *impl = impl_from_IPeriodicForceEffect( iface );
    TRACE( "iface %p, kind %p.\n", iface, kind );
    *kind = impl->kind;
    return S_OK;
}

static HRESULT WINAPI effect_SetParameters( IPeriodicForceEffect *iface, Vector3 direction, FLOAT frequency, FLOAT phase,
                                            FLOAT bias, TimeSpan duration )
{
    struct periodic_effect *impl = impl_from_IPeriodicForceEffect( iface );
    WineForceFeedbackEffectParameters params =
    {
        .periodic =
        {
            .type = WineForceFeedbackEffectType_Periodic_SquareWave + impl->kind,
            .direction = direction,
            .frequency = frequency,
            .phase = phase,
            .bias = bias,
            .duration = duration,
            .repeat_count = 1,
            .gain = 1.,
        },
    };

    TRACE( "iface %p, direction %s, frequency %f, phase %f, bias %f, duration %I64u.\n", iface,
           debugstr_vector3( &direction ), frequency, phase, bias, duration.Duration );

    return IWineForceFeedbackEffectImpl_put_Parameters( impl->IWineForceFeedbackEffectImpl_inner, params, NULL );
}

static HRESULT WINAPI effect_SetParametersWithEnvelope( IPeriodicForceEffect *iface, Vector3 direction, FLOAT frequency, FLOAT phase, FLOAT bias,
                                                        FLOAT attack_gain, FLOAT sustain_gain, FLOAT release_gain, TimeSpan start_delay,
                                                        TimeSpan attack_duration, TimeSpan sustain_duration,
                                                        TimeSpan release_duration, UINT32 repeat_count )
{
    struct periodic_effect *impl = impl_from_IPeriodicForceEffect( iface );
    WineForceFeedbackEffectParameters params =
    {
        .periodic =
        {
            .type = WineForceFeedbackEffectType_Periodic_SquareWave + impl->kind,
            .direction = direction,
            .frequency = frequency,
            .phase = phase,
            .bias = bias,
            .duration = {attack_duration.Duration + sustain_duration.Duration + release_duration.Duration},
            .start_delay = start_delay,
            .repeat_count = repeat_count,
            .gain = sustain_gain,
        },
    };
    WineForceFeedbackEffectEnvelope envelope =
    {
        .attack_gain = attack_gain,
        .release_gain = release_gain,
        .attack_duration = attack_duration,
        .release_duration = release_duration,
    };

    TRACE( "iface %p, direction %s, frequency %f, phase %f, bias %f, attack_gain %f, sustain_gain %f, release_gain %f, start_delay %I64u, "
           "attack_duration %I64u, sustain_duration %I64u, release_duration %I64u, repeat_count %u.\n", iface, debugstr_vector3( &direction ),
           frequency, phase, bias, attack_gain, sustain_gain, release_gain, start_delay.Duration, attack_duration.Duration, sustain_duration.Duration,
           release_duration.Duration, repeat_count );

    return IWineForceFeedbackEffectImpl_put_Parameters( impl->IWineForceFeedbackEffectImpl_inner, params, &envelope );
}

static const struct IPeriodicForceEffectVtbl effect_vtbl =
{
    effect_QueryInterface,
    effect_AddRef,
    effect_Release,
    /* IInspectable methods */
    effect_GetIids,
    effect_GetRuntimeClassName,
    effect_GetTrustLevel,
    /* IPeriodicForceEffect methods */
    effect_get_Kind,
    effect_SetParameters,
    effect_SetParametersWithEnvelope,
};

struct periodic_factory
{
    IActivationFactory IActivationFactory_iface;
    IPeriodicForceEffectFactory IPeriodicForceEffectFactory_iface;
    LONG ref;
};

static inline struct periodic_factory *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct periodic_factory, IActivationFactory_iface );
}

static HRESULT WINAPI activation_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct periodic_factory *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        IInspectable_AddRef( (*out = &impl->IActivationFactory_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IPeriodicForceEffectFactory ))
    {
        IInspectable_AddRef( (*out = &impl->IPeriodicForceEffectFactory_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI activation_AddRef( IActivationFactory *iface )
{
    struct periodic_factory *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI activation_Release( IActivationFactory *iface )
{
    struct periodic_factory *impl = impl_from_IActivationFactory( iface );
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
    FIXME( "iface %p, instance %p stub!\n", iface, instance );
    return E_NOTIMPL;
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

DEFINE_IINSPECTABLE( factory, IPeriodicForceEffectFactory, struct periodic_factory, IActivationFactory_iface )

static HRESULT WINAPI factory_CreateInstance( IPeriodicForceEffectFactory *iface, enum PeriodicForceEffectKind kind, IForceFeedbackEffect **out )
{
    enum WineForceFeedbackEffectType type = WineForceFeedbackEffectType_Periodic + kind;
    struct periodic_effect *impl;
    HRESULT hr;

    TRACE( "iface %p, kind %u, out %p.\n", iface, kind, out );

    if (!(impl = calloc( 1, sizeof(struct periodic_effect) ))) return E_OUTOFMEMORY;
    impl->IPeriodicForceEffect_iface.lpVtbl = &effect_vtbl;
    impl->ref = 1;
    impl->kind = kind;

    if (FAILED(hr = force_feedback_effect_create( type, (IInspectable *)&impl->IPeriodicForceEffect_iface, &impl->IWineForceFeedbackEffectImpl_inner )) ||
        FAILED(hr = IPeriodicForceEffect_QueryInterface( &impl->IPeriodicForceEffect_iface, &IID_IForceFeedbackEffect, (void **)out )))
    {
        if (impl->IWineForceFeedbackEffectImpl_inner) IWineForceFeedbackEffectImpl_Release( impl->IWineForceFeedbackEffectImpl_inner );
        free( impl );
        return hr;
    }

    IPeriodicForceEffect_Release( &impl->IPeriodicForceEffect_iface );
    TRACE( "created PeriodicForceEffect %p\n", *out );
    return S_OK;
}

static const struct IPeriodicForceEffectFactoryVtbl factory_vtbl =
{
    factory_QueryInterface,
    factory_AddRef,
    factory_Release,
    /* IInspectable methods */
    factory_GetIids,
    factory_GetRuntimeClassName,
    factory_GetTrustLevel,
    /* IPeriodicForceEffectFactory methods */
    factory_CreateInstance,
};

static struct periodic_factory periodic_statics =
{
    {&activation_vtbl},
    {&factory_vtbl},
    1,
};

IInspectable *periodic_effect_factory = (IInspectable *)&periodic_statics.IActivationFactory_iface;
