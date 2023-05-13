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

#include "math.h"

#include "ddk/hidsdi.h"
#include "dinput.h"
#include "hidusage.h"
#include "provider.h"

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
    ULONG repeat_count;
    DICONSTANTFORCE constant_force;
    DIRAMPFORCE ramp_force;
    DICONDITION condition;
    DIPERIODIC periodic;
    DIENVELOPE envelope;
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

static int effect_reorient_direction( const WineForceFeedbackEffectParameters *params, Vector3 *direction )
{
    int sign = +1;

    switch (params->type)
    {
    case WineForceFeedbackEffectType_Constant:
        *direction = params->constant.direction;
        sign = params->constant.direction.X < 0 ? -1 : +1;
        break;

    case WineForceFeedbackEffectType_Ramp:
        *direction = params->ramp.start_vector;
        sign = params->ramp.start_vector.X < 0 ? -1 : +1;
        break;

    case WineForceFeedbackEffectType_Periodic_SineWave:
    case WineForceFeedbackEffectType_Periodic_TriangleWave:
    case WineForceFeedbackEffectType_Periodic_SquareWave:
    case WineForceFeedbackEffectType_Periodic_SawtoothWaveDown:
    case WineForceFeedbackEffectType_Periodic_SawtoothWaveUp:
        *direction = params->periodic.direction;
        break;

    case WineForceFeedbackEffectType_Condition_Spring:
    case WineForceFeedbackEffectType_Condition_Damper:
    case WineForceFeedbackEffectType_Condition_Inertia:
    case WineForceFeedbackEffectType_Condition_Friction:
        *direction = params->condition.direction;
        sign = -1;
        break;
    }

    direction->X *= -sign;
    direction->Y *= -sign;
    direction->Z *= -sign;

    return sign;
}

static HRESULT WINAPI effect_impl_put_Parameters( IWineForceFeedbackEffectImpl *iface, WineForceFeedbackEffectParameters params,
                                                  WineForceFeedbackEffectEnvelope *envelope )
{
    struct effect *impl = impl_from_IWineForceFeedbackEffectImpl( iface );
    Vector3 direction = {0};
    double magnitude = 0;
    DWORD count = 0;
    HRESULT hr;
    int sign;

    TRACE( "iface %p, params %p, envelope %p.\n", iface, &params, envelope );

    EnterCriticalSection( &impl->cs );

    sign = effect_reorient_direction( &params, &direction );
    /* Y and Z axes seems to be always ignored, is it really the case? */
    magnitude += direction.X * direction.X;

    switch (params.type)
    {
    case WineForceFeedbackEffectType_Constant:
        impl->repeat_count = params.constant.repeat_count;
        impl->constant_force.lMagnitude = sign * round( params.constant.gain * sqrt( magnitude ) * 10000 );
        impl->params.dwDuration = min( max( params.constant.duration.Duration / 10, 0 ), INFINITE );
        impl->params.dwStartDelay = min( max( params.constant.start_delay.Duration / 10, 0 ), INFINITE );
        break;

    case WineForceFeedbackEffectType_Ramp:
        impl->repeat_count = params.ramp.repeat_count;
        impl->ramp_force.lStart = sign * round( params.ramp.gain * sqrt( magnitude ) * 10000 );
        impl->ramp_force.lEnd = round( params.ramp.gain * params.ramp.end_vector.X * 10000 );
        impl->params.dwDuration = min( max( params.ramp.duration.Duration / 10, 0 ), INFINITE );
        impl->params.dwStartDelay = min( max( params.ramp.start_delay.Duration / 10, 0 ), INFINITE );
        break;

    case WineForceFeedbackEffectType_Periodic_SineWave:
    case WineForceFeedbackEffectType_Periodic_TriangleWave:
    case WineForceFeedbackEffectType_Periodic_SquareWave:
    case WineForceFeedbackEffectType_Periodic_SawtoothWaveDown:
    case WineForceFeedbackEffectType_Periodic_SawtoothWaveUp:
        impl->repeat_count = params.periodic.repeat_count;
        impl->periodic.dwMagnitude = round( params.periodic.gain * 10000 );
        impl->periodic.dwPeriod = 1000000 / params.periodic.frequency;
        impl->periodic.dwPhase = round( params.periodic.phase * 36000 );
        impl->periodic.lOffset = round( params.periodic.bias * 10000 );
        impl->params.dwDuration = min( max( params.periodic.duration.Duration / 10, 0 ), INFINITE );
        impl->params.dwStartDelay = min( max( params.periodic.start_delay.Duration / 10, 0 ), INFINITE );
        break;

    case WineForceFeedbackEffectType_Condition_Spring:
    case WineForceFeedbackEffectType_Condition_Damper:
    case WineForceFeedbackEffectType_Condition_Inertia:
    case WineForceFeedbackEffectType_Condition_Friction:
        impl->repeat_count = 1;
        impl->condition.lPositiveCoefficient = round( atan( params.condition.positive_coeff ) / M_PI_2 * 10000 );
        impl->condition.lNegativeCoefficient = round( atan( params.condition.negative_coeff ) / M_PI_2 * 10000 );
        impl->condition.dwPositiveSaturation = round( params.condition.max_positive_magnitude * 10000 );
        impl->condition.dwNegativeSaturation = round( params.condition.max_negative_magnitude * 10000 );
        impl->condition.lDeadBand = round( params.condition.deadzone * 10000 );
        impl->condition.lOffset = round( params.condition.bias * 10000 );
        impl->params.dwDuration = -1;
        impl->params.dwStartDelay = 0;
        break;
    }

    if (impl->axes[count] == DIJOFS_X) impl->directions[count++] = round( direction.X * 10000 );
    if (impl->axes[count] == DIJOFS_Y) impl->directions[count++] = round( direction.Y * 10000 );
    if (impl->axes[count] == DIJOFS_Z) impl->directions[count++] = round( direction.Z * 10000 );

    if (!envelope) impl->params.lpEnvelope = NULL;
    else
    {
        impl->envelope.dwAttackTime = min( max( envelope->attack_duration.Duration / 10, 0 ), INFINITE );
        impl->envelope.dwAttackLevel = round( envelope->attack_gain * 10000 );
        impl->envelope.dwFadeTime = impl->params.dwDuration - min( max( envelope->release_duration.Duration / 10, 0 ), INFINITE );
        impl->envelope.dwFadeLevel = round( envelope->release_gain * 10000 );
        impl->params.lpEnvelope = &impl->envelope;
    }

    if (!impl->effect) hr = S_OK;
    else hr = IDirectInputEffect_SetParameters( impl->effect, &impl->params, DIEP_ALLPARAMS & ~DIEP_AXES );
    LeaveCriticalSection( &impl->cs );

    return hr;
}

static const struct IWineForceFeedbackEffectImplVtbl effect_impl_vtbl =
{
    effect_impl_QueryInterface,
    effect_impl_AddRef,
    effect_impl_Release,
    /* IWineForceFeedbackEffectImpl methods */
    effect_impl_put_Parameters,
};

DEFINE_IINSPECTABLE_OUTER( effect, IForceFeedbackEffect, struct effect, IInspectable_outer )

static HRESULT WINAPI effect_get_Gain( IForceFeedbackEffect *iface, DOUBLE *value )
{
    struct effect *impl = impl_from_IForceFeedbackEffect( iface );

    TRACE( "iface %p, value %p.\n", iface, value );

    EnterCriticalSection( &impl->cs );
    *value = impl->params.dwGain / 10000.;
    LeaveCriticalSection( &impl->cs );

    return S_OK;
}

static HRESULT WINAPI effect_put_Gain( IForceFeedbackEffect *iface, DOUBLE value )
{
    struct effect *impl = impl_from_IForceFeedbackEffect( iface );
    HRESULT hr;

    TRACE( "iface %p, value %f.\n", iface, value );

    EnterCriticalSection( &impl->cs );
    impl->params.dwGain = round( value * 10000 );
    if (!impl->effect) hr = S_FALSE;
    else hr = IDirectInputEffect_SetParameters( impl->effect, &impl->params, DIEP_GAIN );
    LeaveCriticalSection( &impl->cs );

    return hr;
}

static HRESULT WINAPI effect_get_State( IForceFeedbackEffect *iface, ForceFeedbackEffectState *value )
{
    struct effect *impl = impl_from_IForceFeedbackEffect( iface );
    DWORD status;
    HRESULT hr;

    TRACE( "iface %p, value %p.\n", iface, value );

    EnterCriticalSection( &impl->cs );
    if (!impl->effect)
        *value = ForceFeedbackEffectState_Stopped;
    else if (FAILED(hr = IDirectInputEffect_GetEffectStatus( impl->effect, &status )))
        *value = ForceFeedbackEffectState_Faulted;
    else
    {
        if (status == DIEGES_PLAYING) *value = ForceFeedbackEffectState_Running;
        else *value = ForceFeedbackEffectState_Stopped;
    }
    LeaveCriticalSection( &impl->cs );

    return S_OK;
}

static HRESULT WINAPI effect_Start( IForceFeedbackEffect *iface )
{
    struct effect *impl = impl_from_IForceFeedbackEffect( iface );
    HRESULT hr = E_UNEXPECTED;
    DWORD flags = 0;

    TRACE( "iface %p.\n", iface );

    EnterCriticalSection( &impl->cs );
    if (impl->effect) hr = IDirectInputEffect_Start( impl->effect, impl->repeat_count, flags );
    LeaveCriticalSection( &impl->cs );

    return hr;
}

static HRESULT WINAPI effect_Stop( IForceFeedbackEffect *iface )
{
    struct effect *impl = impl_from_IForceFeedbackEffect( iface );
    HRESULT hr = E_UNEXPECTED;

    TRACE( "iface %p.\n", iface );

    EnterCriticalSection( &impl->cs );
    if (impl->effect) hr = IDirectInputEffect_Stop( impl->effect );
    LeaveCriticalSection( &impl->cs );

    return hr;
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

    case WineForceFeedbackEffectType_Ramp:
        impl->type = GUID_RampForce;
        impl->params.lpvTypeSpecificParams = &impl->ramp_force;
        impl->params.cbTypeSpecificParams = sizeof(impl->ramp_force);
        break;

    case WineForceFeedbackEffectType_Periodic_SineWave:
        impl->type = GUID_Sine;
        goto WineForceFeedbackEffectType_Periodic;
    case WineForceFeedbackEffectType_Periodic_TriangleWave:
        impl->type = GUID_Triangle;
        goto WineForceFeedbackEffectType_Periodic;
    case WineForceFeedbackEffectType_Periodic_SquareWave:
        impl->type = GUID_Square;
        goto WineForceFeedbackEffectType_Periodic;
    case WineForceFeedbackEffectType_Periodic_SawtoothWaveDown:
        impl->type = GUID_SawtoothDown;
        goto WineForceFeedbackEffectType_Periodic;
    case WineForceFeedbackEffectType_Periodic_SawtoothWaveUp:
        impl->type = GUID_SawtoothUp;
        goto WineForceFeedbackEffectType_Periodic;
    WineForceFeedbackEffectType_Periodic:
        impl->params.lpvTypeSpecificParams = &impl->periodic;
        impl->params.cbTypeSpecificParams = sizeof(impl->periodic);
        break;

    case WineForceFeedbackEffectType_Condition_Spring:
        impl->type = GUID_Spring;
        goto WineForceFeedbackEffectType_Condition;
    case WineForceFeedbackEffectType_Condition_Damper:
        impl->type = GUID_Damper;
        goto WineForceFeedbackEffectType_Condition;
    case WineForceFeedbackEffectType_Condition_Inertia:
        impl->type = GUID_Inertia;
        goto WineForceFeedbackEffectType_Condition;
    case WineForceFeedbackEffectType_Condition_Friction:
        impl->type = GUID_Friction;
        goto WineForceFeedbackEffectType_Condition;
    WineForceFeedbackEffectType_Condition:
        impl->params.lpvTypeSpecificParams = &impl->condition;
        impl->params.cbTypeSpecificParams = sizeof(impl->condition);
        break;
    }

    impl->envelope.dwSize = sizeof(DIENVELOPE);
    impl->params.dwSize = sizeof(DIEFFECT);
    impl->params.rgdwAxes = impl->axes;
    impl->params.rglDirection = impl->directions;
    impl->params.dwTriggerButton = -1;
    impl->params.dwGain = 10000;
    impl->params.dwFlags = DIEFF_CARTESIAN|DIEFF_OBJECTOFFSETS;
    impl->params.cAxes = -1;
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

static BOOL CALLBACK check_ffb_axes( const DIDEVICEOBJECTINSTANCEW *obj, void *args )
{
    ForceFeedbackEffectAxes *value = args;

    if (obj->dwType & DIDFT_FFACTUATOR)
    {
        if (IsEqualIID( &obj->guidType, &GUID_XAxis )) *value |= ForceFeedbackEffectAxes_X;
        else if (IsEqualIID( &obj->guidType, &GUID_YAxis )) *value |= ForceFeedbackEffectAxes_Y;
        else if (IsEqualIID( &obj->guidType, &GUID_ZAxis )) *value |= ForceFeedbackEffectAxes_Z;
    }

    return DIENUM_CONTINUE;
}

static HRESULT WINAPI motor_get_SupportedAxes( IForceFeedbackMotor *iface, enum ForceFeedbackEffectAxes *value )
{
    struct motor *impl = impl_from_IForceFeedbackMotor( iface );
    HRESULT hr;

    TRACE( "iface %p, value %p.\n", iface, value );

    *value = ForceFeedbackEffectAxes_None;
    if (FAILED(hr = IDirectInputDevice8_EnumObjects( impl->device, check_ffb_axes, value, DIDFT_AXIS )))
        *value = ForceFeedbackEffectAxes_None;

    return hr;
}

static HRESULT WINAPI motor_load_effect_async( IUnknown *invoker, IUnknown *param, PROPVARIANT *result )
{
    struct effect *effect = impl_from_IForceFeedbackEffect( (IForceFeedbackEffect *)param );
    IForceFeedbackMotor *motor = (IForceFeedbackMotor *)invoker;
    struct motor *impl = impl_from_IForceFeedbackMotor( motor );
    ForceFeedbackEffectAxes supported_axes = 0;
    IDirectInputEffect *dinput_effect;
    HRESULT hr;

    EnterCriticalSection( &effect->cs );

    if (FAILED(hr = IForceFeedbackMotor_get_SupportedAxes( motor, &supported_axes )))
    {
        WARN( "get_SupportedAxes for motor %p returned %#lx\n", motor, hr );
        effect->params.cAxes = 0;
    }
    else if (effect->params.cAxes == -1)
    {
        DWORD count = 0;

        /* initialize axis mapping and re-map directions that were set with the initial mapping */
        if (supported_axes & ForceFeedbackEffectAxes_X)
        {
            effect->directions[count] = effect->directions[0];
            effect->axes[count++] = DIJOFS_X;
        }
        if (supported_axes & ForceFeedbackEffectAxes_Y)
        {
            effect->directions[count] = effect->directions[1];
            effect->axes[count++] = DIJOFS_Y;
        }
        if (supported_axes & ForceFeedbackEffectAxes_Z)
        {
            effect->directions[count] = effect->directions[2];
            effect->axes[count++] = DIJOFS_Z;
        }

        effect->params.cAxes = count;
    }

    if (SUCCEEDED(hr = IDirectInputDevice8_CreateEffect( impl->device, &effect->type, &effect->params,
                                                         &dinput_effect, NULL )))
    {
        if (effect->effect) IDirectInputEffect_Release( effect->effect );
        effect->effect = dinput_effect;
    }

    LeaveCriticalSection( &effect->cs );

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
