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

struct condition_effect
{
    IConditionForceEffect IConditionForceEffect_iface;
    IWineForceFeedbackEffectImpl *IWineForceFeedbackEffectImpl_inner;
    LONG ref;

    ConditionForceEffectKind kind;
};

static inline struct condition_effect *impl_from_IConditionForceEffect( IConditionForceEffect *iface )
{
    return CONTAINING_RECORD( iface, struct condition_effect, IConditionForceEffect_iface );
}

static HRESULT WINAPI effect_QueryInterface( IConditionForceEffect *iface, REFIID iid, void **out )
{
    struct condition_effect *impl = impl_from_IConditionForceEffect( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IConditionForceEffect ))
    {
        IInspectable_AddRef( (*out = &impl->IConditionForceEffect_iface) );
        return S_OK;
    }

    return IWineForceFeedbackEffectImpl_QueryInterface( impl->IWineForceFeedbackEffectImpl_inner, iid, out );
}

static ULONG WINAPI effect_AddRef( IConditionForceEffect *iface )
{
    struct condition_effect *impl = impl_from_IConditionForceEffect( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI effect_Release( IConditionForceEffect *iface )
{
    struct condition_effect *impl = impl_from_IConditionForceEffect( iface );
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

static HRESULT WINAPI effect_GetIids( IConditionForceEffect *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_GetRuntimeClassName( IConditionForceEffect *iface, HSTRING *class_name )
{
    return WindowsCreateString( RuntimeClass_Windows_Gaming_Input_ForceFeedback_ConditionForceEffect,
                                ARRAY_SIZE(RuntimeClass_Windows_Gaming_Input_ForceFeedback_ConditionForceEffect),
                                class_name );
}

static HRESULT WINAPI effect_GetTrustLevel( IConditionForceEffect *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_get_Kind( IConditionForceEffect *iface, ConditionForceEffectKind *kind )
{
    struct condition_effect *impl = impl_from_IConditionForceEffect( iface );
    TRACE( "iface %p, kind %p.\n", iface, kind );
    *kind = impl->kind;
    return S_OK;
}

static HRESULT WINAPI effect_SetParameters( IConditionForceEffect *iface, Vector3 direction, FLOAT positive_coeff, FLOAT negative_coeff,
                                            FLOAT max_positive_magnitude, FLOAT max_negative_magnitude, FLOAT deadzone, FLOAT bias )
{
    struct condition_effect *impl = impl_from_IConditionForceEffect( iface );
    WineForceFeedbackEffectParameters params =
    {
        .condition =
        {
            .type = WineForceFeedbackEffectType_Condition + impl->kind,
            .direction = direction,
            .positive_coeff = positive_coeff,
            .negative_coeff = negative_coeff,
            .max_positive_magnitude = max_positive_magnitude,
            .max_negative_magnitude = max_negative_magnitude,
            .deadzone = deadzone,
            .bias = bias,
        },
    };

    TRACE( "iface %p, direction %s, positive_coeff %f, negative_coeff %f, max_positive_magnitude %f, max_negative_magnitude %f, deadzone %f, bias %f.\n",
           iface, debugstr_vector3( &direction ), positive_coeff, negative_coeff, max_positive_magnitude, max_negative_magnitude, deadzone, bias );

    return IWineForceFeedbackEffectImpl_SetParameters( impl->IWineForceFeedbackEffectImpl_inner, params, NULL );
}

static const struct IConditionForceEffectVtbl effect_vtbl =
{
    effect_QueryInterface,
    effect_AddRef,
    effect_Release,
    /* IInspectable methods */
    effect_GetIids,
    effect_GetRuntimeClassName,
    effect_GetTrustLevel,
    /* IConditionForceEffect methods */
    effect_get_Kind,
    effect_SetParameters,
};

struct condition_factory
{
    IActivationFactory IActivationFactory_iface;
    IConditionForceEffectFactory IConditionForceEffectFactory_iface;
    LONG ref;
};

static inline struct condition_factory *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct condition_factory, IActivationFactory_iface );
}

static HRESULT WINAPI activation_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct condition_factory *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        IInspectable_AddRef( (*out = &impl->IActivationFactory_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IConditionForceEffectFactory ))
    {
        IInspectable_AddRef( (*out = &impl->IConditionForceEffectFactory_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI activation_AddRef( IActivationFactory *iface )
{
    struct condition_factory *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI activation_Release( IActivationFactory *iface )
{
    struct condition_factory *impl = impl_from_IActivationFactory( iface );
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

DEFINE_IINSPECTABLE( factory, IConditionForceEffectFactory, struct condition_factory, IActivationFactory_iface )

static HRESULT WINAPI factory_CreateInstance( IConditionForceEffectFactory *iface, enum ConditionForceEffectKind kind, IForceFeedbackEffect **out )
{
    enum WineForceFeedbackEffectType type = WineForceFeedbackEffectType_Condition + kind;
    struct condition_effect *impl;
    HRESULT hr;

    TRACE( "iface %p, kind %u, out %p.\n", iface, kind, out );

    if (!(impl = calloc( 1, sizeof(struct condition_effect) ))) return E_OUTOFMEMORY;
    impl->IConditionForceEffect_iface.lpVtbl = &effect_vtbl;
    impl->ref = 1;
    impl->kind = kind;

    if (FAILED(hr = force_feedback_effect_create( type, (IInspectable *)&impl->IConditionForceEffect_iface, &impl->IWineForceFeedbackEffectImpl_inner )) ||
        FAILED(hr = IConditionForceEffect_QueryInterface( &impl->IConditionForceEffect_iface, &IID_IForceFeedbackEffect, (void **)out )))
    {
        if (impl->IWineForceFeedbackEffectImpl_inner) IWineForceFeedbackEffectImpl_Release( impl->IWineForceFeedbackEffectImpl_inner );
        free( impl );
        return hr;
    }

    IConditionForceEffect_Release( &impl->IConditionForceEffect_iface );
    TRACE( "created ConditionForceEffect %p\n", *out );
    return S_OK;
}

static const struct IConditionForceEffectFactoryVtbl factory_vtbl =
{
    factory_QueryInterface,
    factory_AddRef,
    factory_Release,
    /* IInspectable methods */
    factory_GetIids,
    factory_GetRuntimeClassName,
    factory_GetTrustLevel,
    /* IConditionForceEffectFactory methods */
    factory_CreateInstance,
};

static struct condition_factory condition_statics =
{
    {&activation_vtbl},
    {&factory_vtbl},
    1,
};

IInspectable *condition_effect_factory = (IInspectable *)&condition_statics.IActivationFactory_iface;
