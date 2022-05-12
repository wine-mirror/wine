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

#include "ddk/hidsdi.h"
#include "dinput.h"
#include "hidusage.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(input);

struct effect
{
    IWineForceFeedbackEffectImpl IWineForceFeedbackEffectImpl_iface;
    IForceFeedbackEffect IForceFeedbackEffect_iface;
    IInspectable *IInspectable_outer;
    LONG ref;

    CRITICAL_SECTION cs;
    IDirectInputEffect *effect;

    GUID type;
    DWORD axes[3];
    LONG directions[3];
    DICONSTANTFORCE constant_force;
    DIEFFECT params;
};

static inline struct effect *impl_from_IWineForceFeedbackEffectImpl( IWineForceFeedbackEffectImpl *iface )
{
    return CONTAINING_RECORD( iface, struct effect, IWineForceFeedbackEffectImpl_iface );
}

static HRESULT WINAPI effect_impl_QueryInterface( IWineForceFeedbackEffectImpl *iface, REFIID iid, void **out )
{
    struct effect *impl = impl_from_IWineForceFeedbackEffectImpl( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IWineForceFeedbackEffectImpl ))
    {
        IWineForceFeedbackEffectImpl_AddRef( (*out = &impl->IWineForceFeedbackEffectImpl_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IForceFeedbackEffect ))
    {
        IInspectable_AddRef( (*out = &impl->IForceFeedbackEffect_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI effect_impl_AddRef( IWineForceFeedbackEffectImpl *iface )
{
    struct effect *impl = impl_from_IWineForceFeedbackEffectImpl( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI effect_impl_Release( IWineForceFeedbackEffectImpl *iface )
{
    struct effect *impl = impl_from_IWineForceFeedbackEffectImpl( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref)
    {
        if (impl->effect) IDirectInputEffect_Release( impl->effect );
        impl->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection( &impl->cs );
        free( impl );
    }

    return ref;
}

static const struct IWineForceFeedbackEffectImplVtbl effect_impl_vtbl =
{
    effect_impl_QueryInterface,
    effect_impl_AddRef,
    effect_impl_Release,
    /* IWineForceFeedbackEffectImpl methods */
};

DEFINE_IINSPECTABLE_OUTER( effect, IForceFeedbackEffect, struct effect, IInspectable_outer )

static HRESULT WINAPI effect_get_Gain( IForceFeedbackEffect *iface, DOUBLE *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_put_Gain( IForceFeedbackEffect *iface, DOUBLE value )
{
    FIXME( "iface %p, value %f stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_get_State( IForceFeedbackEffect *iface, ForceFeedbackEffectState *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_Start( IForceFeedbackEffect *iface )
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_Stop( IForceFeedbackEffect *iface )
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static const struct IForceFeedbackEffectVtbl effect_vtbl =
{
    effect_QueryInterface,
    effect_AddRef,
    effect_Release,
    /* IInspectable methods */
    effect_GetIids,
    effect_GetRuntimeClassName,
    effect_GetTrustLevel,
    /* IForceFeedbackEffect methods */
    effect_get_Gain,
    effect_put_Gain,
    effect_get_State,
    effect_Start,
    effect_Stop,
};

HRESULT force_feedback_effect_create( enum WineForceFeedbackEffectType type, IInspectable *outer, IWineForceFeedbackEffectImpl **out )
{
    struct effect *impl;

    TRACE( "outer %p, out %p\n", outer, out );

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->IWineForceFeedbackEffectImpl_iface.lpVtbl = &effect_impl_vtbl;
    impl->IForceFeedbackEffect_iface.lpVtbl = &effect_vtbl;
    impl->IInspectable_outer = outer;
    impl->ref = 1;

    switch (type)
    {
    case WineForceFeedbackEffectType_Constant:
        impl->type = GUID_ConstantForce;
        impl->params.lpvTypeSpecificParams = &impl->constant_force;
        impl->params.cbTypeSpecificParams = sizeof(impl->constant_force);
        break;
    }

    impl->params.dwSize = sizeof(DIEFFECT);
    impl->params.rgdwAxes = impl->axes;
    impl->params.rglDirection = impl->directions;
    impl->params.dwTriggerButton = -1;
    impl->params.dwGain = 10000;
    impl->params.dwFlags = DIEFF_CARTESIAN|DIEFF_OBJECTOFFSETS;
    impl->params.cAxes = 0;
    impl->axes[0] = DIJOFS_X;
    impl->axes[1] = DIJOFS_Y;
    impl->axes[2] = DIJOFS_Z;

    InitializeCriticalSection( &impl->cs );
    impl->cs.DebugInfo->Spare[0] = (DWORD_PTR)( __FILE__ ": effect.cs" );

    *out = &impl->IWineForceFeedbackEffectImpl_iface;
    TRACE( "created ForceFeedbackEffect %p\n", *out );
    return S_OK;
}

struct motor
{
    IForceFeedbackMotor IForceFeedbackMotor_iface;
    LONG ref;

    IDirectInputDevice8W *device;
};

static inline struct motor *impl_from_IForceFeedbackMotor( IForceFeedbackMotor *iface )
{
    return CONTAINING_RECORD( iface, struct motor, IForceFeedbackMotor_iface );
}

static HRESULT WINAPI motor_QueryInterface( IForceFeedbackMotor *iface, REFIID iid, void **out )
{
    struct motor *impl = impl_from_IForceFeedbackMotor( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IForceFeedbackMotor ))
    {
        IInspectable_AddRef( (*out = &impl->IForceFeedbackMotor_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI motor_AddRef( IForceFeedbackMotor *iface )
{
    struct motor *impl = impl_from_IForceFeedbackMotor( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI motor_Release( IForceFeedbackMotor *iface )
{
    struct motor *impl = impl_from_IForceFeedbackMotor( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref)
    {
        IDirectInputDevice8_Release( impl->device );
        free( impl );
    }

    return ref;
}

static HRESULT WINAPI motor_GetIids( IForceFeedbackMotor *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI motor_GetRuntimeClassName( IForceFeedbackMotor *iface, HSTRING *class_name )
{
    return WindowsCreateString( RuntimeClass_Windows_Gaming_Input_ForceFeedback_ForceFeedbackMotor,
                                ARRAY_SIZE(RuntimeClass_Windows_Gaming_Input_ForceFeedback_ForceFeedbackMotor),
                                class_name );
}

static HRESULT WINAPI motor_GetTrustLevel( IForceFeedbackMotor *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI motor_get_AreEffectsPaused( IForceFeedbackMotor *iface, BOOLEAN *value )
{
    struct motor *impl = impl_from_IForceFeedbackMotor( iface );
    DWORD state;
    HRESULT hr;

    TRACE( "iface %p, value %p.\n", iface, value );

    if (FAILED(hr = IDirectInputDevice8_GetForceFeedbackState( impl->device, &state ))) *value = FALSE;
    else *value = (state & DIGFFS_PAUSED);

    return hr;
}

static HRESULT WINAPI motor_get_MasterGain( IForceFeedbackMotor *iface, double *value )
{
    struct motor *impl = impl_from_IForceFeedbackMotor( iface );
    DIPROPDWORD gain =
    {
        .diph =
        {
            .dwSize = sizeof(DIPROPDWORD),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_DEVICE,
        },
    };
    HRESULT hr;

    TRACE( "iface %p, value %p.\n", iface, value );

    if (FAILED(hr = IDirectInputDevice8_GetProperty( impl->device, DIPROP_FFGAIN, &gain.diph ))) *value = 1.;
    else *value = gain.dwData / 10000.;

    return hr;
}

static HRESULT WINAPI motor_put_MasterGain( IForceFeedbackMotor *iface, double value )
{
    struct motor *impl = impl_from_IForceFeedbackMotor( iface );
    DIPROPDWORD gain =
    {
        .diph =
        {
            .dwSize = sizeof(DIPROPDWORD),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_DEVICE,
        },
    };

    TRACE( "iface %p, value %f.\n", iface, value );

    gain.dwData = 10000 * value;
    return IDirectInputDevice8_SetProperty( impl->device, DIPROP_FFGAIN, &gain.diph );
}

static HRESULT WINAPI motor_get_IsEnabled( IForceFeedbackMotor *iface, BOOLEAN *value )
{
    struct motor *impl = impl_from_IForceFeedbackMotor( iface );
    DWORD state;
    HRESULT hr;

    TRACE( "iface %p, value %p.\n", iface, value );

    if (FAILED(hr = IDirectInputDevice8_GetForceFeedbackState( impl->device, &state ))) *value = FALSE;
    else *value = !(state & DIGFFS_ACTUATORSOFF);

    return hr;
}

static HRESULT WINAPI motor_get_SupportedAxes( IForceFeedbackMotor *iface, enum ForceFeedbackEffectAxes *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI motor_load_effect_async( IUnknown *invoker, IUnknown *param, PROPVARIANT *result )
{
    struct effect *effect = impl_from_IForceFeedbackEffect( (IForceFeedbackEffect *)param );
    struct motor *impl = impl_from_IForceFeedbackMotor( (IForceFeedbackMotor *)invoker );
    IDirectInputEffect *dinput_effect;
    HRESULT hr;

    if (SUCCEEDED(hr = IDirectInputDevice8_CreateEffect( impl->device, &effect->type, &effect->params, &dinput_effect, NULL )))
    {
        EnterCriticalSection( &effect->cs );
        if (!effect->effect)
        {
            effect->effect = dinput_effect;
            IDirectInputEffect_AddRef( effect->effect );
        }
        LeaveCriticalSection( &effect->cs );
        IDirectInputEffect_Release( dinput_effect );
    }

    result->vt = VT_UI4;
    if (SUCCEEDED(hr)) result->ulVal = ForceFeedbackLoadEffectResult_Succeeded;
    else if (hr == DIERR_DEVICEFULL) result->ulVal = ForceFeedbackLoadEffectResult_EffectStorageFull;
    else result->ulVal = ForceFeedbackLoadEffectResult_EffectNotSupported;

    return hr;
}

static HRESULT WINAPI motor_LoadEffectAsync( IForceFeedbackMotor *iface, IForceFeedbackEffect *effect,
                                             IAsyncOperation_ForceFeedbackLoadEffectResult **async_op )
{
    TRACE( "iface %p, effect %p, async_op %p.\n", iface, effect, async_op );
    return async_operation_effect_result_create( (IUnknown *)iface, (IUnknown *)effect, motor_load_effect_async, async_op );
}

static HRESULT WINAPI motor_PauseAllEffects( IForceFeedbackMotor *iface )
{
    struct motor *impl = impl_from_IForceFeedbackMotor( iface );

    TRACE( "iface %p.\n", iface );

    return IDirectInputDevice8_SendForceFeedbackCommand( impl->device, DISFFC_PAUSE );
}

static HRESULT WINAPI motor_ResumeAllEffects( IForceFeedbackMotor *iface )
{
    struct motor *impl = impl_from_IForceFeedbackMotor( iface );

    TRACE( "iface %p.\n", iface );

    return IDirectInputDevice8_SendForceFeedbackCommand( impl->device, DISFFC_CONTINUE );
}

static HRESULT WINAPI motor_StopAllEffects( IForceFeedbackMotor *iface )
{
    struct motor *impl = impl_from_IForceFeedbackMotor( iface );

    TRACE( "iface %p.\n", iface );

    return IDirectInputDevice8_SendForceFeedbackCommand( impl->device, DISFFC_STOPALL );
}

static HRESULT WINAPI motor_try_disable_async( IUnknown *invoker, IUnknown *param, PROPVARIANT *result )
{
    struct motor *impl = impl_from_IForceFeedbackMotor( (IForceFeedbackMotor *)invoker );
    HRESULT hr;

    hr = IDirectInputDevice8_SendForceFeedbackCommand( impl->device, DISFFC_SETACTUATORSOFF );
    result->vt = VT_BOOL;
    result->boolVal = SUCCEEDED(hr);

    return hr;
}

static HRESULT WINAPI motor_TryDisableAsync( IForceFeedbackMotor *iface, IAsyncOperation_boolean **async_op )
{
    TRACE( "iface %p, async_op %p.\n", iface, async_op );
    return async_operation_boolean_create( (IUnknown *)iface, NULL, motor_try_disable_async, async_op );
}

static HRESULT WINAPI motor_try_enable_async( IUnknown *invoker, IUnknown *param, PROPVARIANT *result )
{
    struct motor *impl = impl_from_IForceFeedbackMotor( (IForceFeedbackMotor *)invoker );
    HRESULT hr;

    hr = IDirectInputDevice8_SendForceFeedbackCommand( impl->device, DISFFC_SETACTUATORSON );
    result->vt = VT_BOOL;
    result->boolVal = SUCCEEDED(hr);

    return hr;
}

static HRESULT WINAPI motor_TryEnableAsync( IForceFeedbackMotor *iface, IAsyncOperation_boolean **async_op )
{
    TRACE( "iface %p, async_op %p.\n", iface, async_op );
    return async_operation_boolean_create( (IUnknown *)iface, NULL, motor_try_enable_async, async_op );
}

static HRESULT WINAPI motor_try_reset_async( IUnknown *invoker, IUnknown *param, PROPVARIANT *result )
{
    struct motor *impl = impl_from_IForceFeedbackMotor( (IForceFeedbackMotor *)invoker );
    HRESULT hr;

    hr = IDirectInputDevice8_SendForceFeedbackCommand( impl->device, DISFFC_RESET );
    result->vt = VT_BOOL;
    result->boolVal = SUCCEEDED(hr);

    return hr;
}

static HRESULT WINAPI motor_TryResetAsync( IForceFeedbackMotor *iface, IAsyncOperation_boolean **async_op )
{
    TRACE( "iface %p, async_op %p.\n", iface, async_op );
    return async_operation_boolean_create( (IUnknown *)iface, NULL, motor_try_reset_async, async_op );
}

static HRESULT WINAPI motor_unload_effect_async( IUnknown *iface, IUnknown *param, PROPVARIANT *result )
{
    struct effect *effect = impl_from_IForceFeedbackEffect( (IForceFeedbackEffect *)param );
    IDirectInputEffect *dinput_effect;
    HRESULT hr;

    EnterCriticalSection( &effect->cs );
    dinput_effect = effect->effect;
    effect->effect = NULL;
    LeaveCriticalSection( &effect->cs );

    if (!dinput_effect) hr = S_OK;
    else
    {
        hr = IDirectInputEffect_Unload( dinput_effect );
        IDirectInputEffect_Release( dinput_effect );
    }

    result->vt = VT_BOOL;
    result->boolVal = SUCCEEDED(hr);
    return hr;
}

static HRESULT WINAPI motor_TryUnloadEffectAsync( IForceFeedbackMotor *iface, IForceFeedbackEffect *effect,
                                                  IAsyncOperation_boolean **async_op )
{
    struct effect *impl = impl_from_IForceFeedbackEffect( (IForceFeedbackEffect *)effect );
    HRESULT hr = S_OK;

    TRACE( "iface %p, effect %p, async_op %p.\n", iface, effect, async_op );

    EnterCriticalSection( &impl->cs );
    if (!impl->effect) hr = E_FAIL;
    LeaveCriticalSection( &impl->cs );
    if (FAILED(hr)) return hr;

    return async_operation_boolean_create( (IUnknown *)iface, (IUnknown *)effect, motor_unload_effect_async, async_op );
}

static const struct IForceFeedbackMotorVtbl motor_vtbl =
{
    motor_QueryInterface,
    motor_AddRef,
    motor_Release,
    /* IInspectable methods */
    motor_GetIids,
    motor_GetRuntimeClassName,
    motor_GetTrustLevel,
    /* IForceFeedbackMotor methods */
    motor_get_AreEffectsPaused,
    motor_get_MasterGain,
    motor_put_MasterGain,
    motor_get_IsEnabled,
    motor_get_SupportedAxes,
    motor_LoadEffectAsync,
    motor_PauseAllEffects,
    motor_ResumeAllEffects,
    motor_StopAllEffects,
    motor_TryDisableAsync,
    motor_TryEnableAsync,
    motor_TryResetAsync,
    motor_TryUnloadEffectAsync,
};

HRESULT force_feedback_motor_create( IDirectInputDevice8W *device, IForceFeedbackMotor **out )
{
    struct motor *impl;
    HRESULT hr;

    TRACE( "device %p, out %p\n", device, out );

    if (FAILED(hr = IDirectInputDevice8_Unacquire( device ))) goto failed;
    if (FAILED(hr = IDirectInputDevice8_SetCooperativeLevel( device, GetDesktopWindow(), DISCL_BACKGROUND | DISCL_EXCLUSIVE ))) goto failed;
    if (FAILED(hr = IDirectInputDevice8_Acquire( device ))) goto failed;

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->IForceFeedbackMotor_iface.lpVtbl = &motor_vtbl;
    impl->ref = 1;

    IDirectInputDevice_AddRef( device );
    impl->device = device;

    *out = &impl->IForceFeedbackMotor_iface;
    TRACE( "created ForceFeedbackMotor %p\n", *out );
    return S_OK;

failed:
    IDirectInputDevice8_SetCooperativeLevel( device, 0, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE );
    IDirectInputDevice8_Acquire( device );
    WARN( "Failed to acquire device exclusively, hr %#lx\n", hr );
    return hr;
}
