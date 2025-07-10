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

struct ramp_effect
{
    IRampForceEffect IRampForceEffect_iface;
    IWineForceFeedbackEffectImpl *IWineForceFeedbackEffectImpl_inner;
    LONG ref;
};

static inline struct ramp_effect *impl_from_IRampForceEffect( IRampForceEffect *iface )
{
    return CONTAINING_RECORD( iface, struct ramp_effect, IRampForceEffect_iface );
}

static HRESULT WINAPI effect_QueryInterface( IRampForceEffect *iface, REFIID iid, void **out )
{
    struct ramp_effect *impl = impl_from_IRampForceEffect( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IRampForceEffect ))
    {
        IInspectable_AddRef( (*out = &impl->IRampForceEffect_iface) );
        return S_OK;
    }

    return IWineForceFeedbackEffectImpl_QueryInterface( impl->IWineForceFeedbackEffectImpl_inner, iid, out );
}

static ULONG WINAPI effect_AddRef( IRampForceEffect *iface )
{
    struct ramp_effect *impl = impl_from_IRampForceEffect( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI effect_Release( IRampForceEffect *iface )
{
    struct ramp_effect *impl = impl_from_IRampForceEffect( iface );
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

static HRESULT WINAPI effect_GetIids( IRampForceEffect *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_GetRuntimeClassName( IRampForceEffect *iface, HSTRING *class_name )
{
    return WindowsCreateString( RuntimeClass_Windows_Gaming_Input_ForceFeedback_RampForceEffect,
                                ARRAY_SIZE(RuntimeClass_Windows_Gaming_Input_ForceFeedback_RampForceEffect),
                                class_name );
}

static HRESULT WINAPI effect_GetTrustLevel( IRampForceEffect *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_SetParameters( IRampForceEffect *iface, Vector3 start_vector, Vector3 end_vector, TimeSpan duration )
{
    WineForceFeedbackEffectParameters params =
    {
        .ramp =
        {
            .type = WineForceFeedbackEffectType_Ramp,
            .start_vector = start_vector,
            .end_vector = end_vector,
            .duration = duration,
            .repeat_count = 1,
            .gain = 1.,
        },
    };
    struct ramp_effect *impl = impl_from_IRampForceEffect( iface );

    TRACE( "iface %p, start_vector %s, end_vector %s, duration %I64u.\n", iface,
           debugstr_vector3( &start_vector ), debugstr_vector3( &end_vector ), duration.Duration );

    return IWineForceFeedbackEffectImpl_SetParameters( impl->IWineForceFeedbackEffectImpl_inner, params, NULL );
}

static HRESULT WINAPI effect_SetParametersWithEnvelope( IRampForceEffect *iface, Vector3 start_vector, Vector3 end_vector, FLOAT attack_gain,
                                                        FLOAT sustain_gain, FLOAT release_gain, TimeSpan start_delay,
                                                        TimeSpan attack_duration, TimeSpan sustain_duration,
                                                        TimeSpan release_duration, UINT32 repeat_count )
{
    WineForceFeedbackEffectParameters params =
    {
        .ramp =
        {
            .type = WineForceFeedbackEffectType_Ramp,
            .start_vector = start_vector,
            .end_vector = end_vector,
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
    struct ramp_effect *impl = impl_from_IRampForceEffect( iface );

    TRACE( "iface %p, start_vector %s, end_vector %s, attack_gain %f, sustain_gain %f, release_gain %f, start_delay %I64u, attack_duration %I64u, "
           "sustain_duration %I64u, release_duration %I64u, repeat_count %u.\n", iface, debugstr_vector3( &start_vector ), debugstr_vector3( &end_vector ),
           attack_gain, sustain_gain, release_gain, start_delay.Duration, attack_duration.Duration, sustain_duration.Duration,
           release_duration.Duration, repeat_count );

    return IWineForceFeedbackEffectImpl_SetParameters( impl->IWineForceFeedbackEffectImpl_inner, params, &envelope );
}

static const struct IRampForceEffectVtbl effect_vtbl =
{
    effect_QueryInterface,
    effect_AddRef,
    effect_Release,
    /* IInspectable methods */
    effect_GetIids,
    effect_GetRuntimeClassName,
    effect_GetTrustLevel,
    /* IRampForceEffect methods */
    effect_SetParameters,
    effect_SetParametersWithEnvelope,
};

struct ramp_factory
{
    IActivationFactory IActivationFactory_iface;
    LONG ref;
};

static inline struct ramp_factory *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct ramp_factory, IActivationFactory_iface );
}

static HRESULT WINAPI activation_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct ramp_factory *impl = impl_from_IActivationFactory( iface );

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
    struct ramp_factory *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI activation_Release( IActivationFactory *iface )
{
    struct ramp_factory *impl = impl_from_IActivationFactory( iface );
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
    struct ramp_effect *impl;
    HRESULT hr;

    TRACE( "iface %p, instance %p.\n", iface, instance );

    if (!(impl = calloc( 1, sizeof(struct ramp_effect) ))) return E_OUTOFMEMORY;
    impl->IRampForceEffect_iface.lpVtbl = &effect_vtbl;
    impl->ref = 1;

    if (FAILED(hr = force_feedback_effect_create( WineForceFeedbackEffectType_Ramp, (IInspectable *)&impl->IRampForceEffect_iface,
                                                  &impl->IWineForceFeedbackEffectImpl_inner )))
    {
        free( impl );
        return hr;
    }

    *instance = (IInspectable *)&impl->IRampForceEffect_iface;
    TRACE( "created RampForceEffect %p\n", *instance );
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

static struct ramp_factory ramp_statics =
{
    {&activation_vtbl},
    1,
};

IInspectable *ramp_effect_factory = (IInspectable *)&ramp_statics.IActivationFactory_iface;
