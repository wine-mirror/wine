/* WinRT Windows.Gaming.Input implementation
 *
 * Copyright 2021 Rémi Bernon for CodeWeavers
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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(input);

static CRITICAL_SECTION gamepad_cs;
static CRITICAL_SECTION_DEBUG gamepad_cs_debug =
{
    0, 0, &gamepad_cs,
    { &gamepad_cs_debug.ProcessLocksList, &gamepad_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": gamepad_cs") }
};
static CRITICAL_SECTION gamepad_cs = { &gamepad_cs_debug, -1, 0, 0, 0, 0 };

static IVector_Gamepad *gamepads;
static struct list gamepad_added_handlers = LIST_INIT( gamepad_added_handlers );
static struct list gamepad_removed_handlers = LIST_INIT( gamepad_removed_handlers );

static HRESULT init_gamepads(void)
{
    static const struct vector_iids iids =
    {
        .vector = &IID_IVector_Gamepad,
        .view = &IID_IVectorView_Gamepad,
        .iterable = &IID_IIterable_Gamepad,
        .iterator = &IID_IIterator_Gamepad,
    };
    HRESULT hr;

    EnterCriticalSection( &gamepad_cs );
    if (gamepads) hr = S_OK;
    else hr = vector_create( &iids, (void **)&gamepads );
    LeaveCriticalSection( &gamepad_cs );

    return hr;
}

struct gamepad
{
    IGameControllerImpl IGameControllerImpl_iface;
    IGameControllerInputSink IGameControllerInputSink_iface;
    IGamepad IGamepad_iface;
    IGamepad2 IGamepad2_iface;
    IGameController *IGameController_outer;
    LONG ref;

    IGameControllerProvider *provider;
    IWineGameControllerProvider *wine_provider;

    struct WineGameControllerState initial_state;
    BOOL state_changed;
};

static inline struct gamepad *impl_from_IGameControllerImpl( IGameControllerImpl *iface )
{
    return CONTAINING_RECORD( iface, struct gamepad, IGameControllerImpl_iface );
}

static HRESULT WINAPI controller_QueryInterface( IGameControllerImpl *iface, REFIID iid, void **out )
{
    struct gamepad *impl = impl_from_IGameControllerImpl( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IGameControllerImpl ))
    {
        IInspectable_AddRef( (*out = &impl->IGameControllerImpl_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IGameControllerInputSink ))
    {
        IInspectable_AddRef( (*out = &impl->IGameControllerInputSink_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IGamepad ))
    {
        IInspectable_AddRef( (*out = &impl->IGamepad_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IGamepad2 ))
    {
        IInspectable_AddRef( (*out = &impl->IGamepad2_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI controller_AddRef( IGameControllerImpl *iface )
{
    struct gamepad *impl = impl_from_IGameControllerImpl( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI controller_Release( IGameControllerImpl *iface )
{
    struct gamepad *impl = impl_from_IGameControllerImpl( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref)
    {
        if (impl->wine_provider)
            IWineGameControllerProvider_Release( impl->wine_provider );
        IGameControllerProvider_Release( impl->provider );
        free( impl );
    }

    return ref;
}

static HRESULT WINAPI controller_GetIids( IGameControllerImpl *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI controller_GetRuntimeClassName( IGameControllerImpl *iface, HSTRING *class_name )
{
    return WindowsCreateString( RuntimeClass_Windows_Gaming_Input_Gamepad,
                                ARRAY_SIZE(RuntimeClass_Windows_Gaming_Input_Gamepad),
                                class_name );
}

static HRESULT WINAPI controller_GetTrustLevel( IGameControllerImpl *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI controller_Initialize( IGameControllerImpl *iface, IGameController *outer,
                                             IGameControllerProvider *provider )
{
    struct gamepad *impl = impl_from_IGameControllerImpl( iface );
    HRESULT hr;

    TRACE( "iface %p, outer %p, provider %p.\n", iface, outer, provider );

    impl->IGameController_outer = outer;
    IGameControllerProvider_AddRef( (impl->provider = provider) );

    hr = IGameControllerProvider_QueryInterface( provider, &IID_IWineGameControllerProvider,
                                                 (void **)&impl->wine_provider );

    if (SUCCEEDED(hr))
        hr = IWineGameControllerProvider_get_State( impl->wine_provider, &impl->initial_state );

    if (FAILED(hr)) return hr;

    EnterCriticalSection( &gamepad_cs );
    if (SUCCEEDED(hr = init_gamepads()))
        hr = IVector_Gamepad_Append( gamepads, &impl->IGamepad_iface );
    LeaveCriticalSection( &gamepad_cs );

    return hr;
}

static const struct IGameControllerImplVtbl controller_vtbl =
{
    controller_QueryInterface,
    controller_AddRef,
    controller_Release,
    /* IInspectable methods */
    controller_GetIids,
    controller_GetRuntimeClassName,
    controller_GetTrustLevel,
    /* IGameControllerImpl methods */
    controller_Initialize,
};

DEFINE_IINSPECTABLE_OUTER( input_sink, IGameControllerInputSink, struct gamepad, IGameController_outer )

static HRESULT WINAPI input_sink_OnInputResumed( IGameControllerInputSink *iface, UINT64 timestamp )
{
    FIXME( "iface %p, timestamp %I64u stub!\n", iface, timestamp );
    return E_NOTIMPL;
}

static HRESULT WINAPI input_sink_OnInputSuspended( IGameControllerInputSink *iface, UINT64 timestamp )
{
    FIXME( "iface %p, timestamp %I64u stub!\n", iface, timestamp );
    return E_NOTIMPL;
}

static const struct IGameControllerInputSinkVtbl input_sink_vtbl =
{
    input_sink_QueryInterface,
    input_sink_AddRef,
    input_sink_Release,
    /* IInspectable methods */
    input_sink_GetIids,
    input_sink_GetRuntimeClassName,
    input_sink_GetTrustLevel,
    /* IGameControllerInputSink methods */
    input_sink_OnInputResumed,
    input_sink_OnInputSuspended,
};

DEFINE_IINSPECTABLE_OUTER( gamepad, IGamepad, struct gamepad, IGameController_outer )

static HRESULT WINAPI gamepad_get_Vibration( IGamepad *iface, struct GamepadVibration *value )
{
    struct gamepad *impl = impl_from_IGamepad( iface );
    struct WineGameControllerVibration vibration;
    HRESULT hr;

    TRACE( "iface %p, value %p.\n", iface, value );

    if (FAILED(hr = IWineGameControllerProvider_get_Vibration( impl->wine_provider, &vibration ))) return hr;

    value->LeftMotor = vibration.rumble / 65535.;
    value->RightMotor = vibration.buzz / 65535.;
    value->LeftTrigger = vibration.left / 65535.;
    value->RightTrigger = vibration.right / 65535.;

    return S_OK;
}

static HRESULT WINAPI gamepad_put_Vibration( IGamepad *iface, struct GamepadVibration value )
{
    struct gamepad *impl = impl_from_IGamepad( iface );
    struct WineGameControllerVibration vibration =
    {
        .rumble = value.LeftMotor * 65535.,
        .buzz = value.RightMotor * 65535.,
        .left = value.LeftTrigger * 65535.,
        .right = value.RightTrigger * 65535.,
    };

    TRACE( "iface %p, value %p.\n", iface, &value );

    return IWineGameControllerProvider_put_Vibration( impl->wine_provider, vibration );
}

static HRESULT WINAPI gamepad_GetCurrentReading( IGamepad *iface, struct GamepadReading *value )
{
    struct gamepad *impl = impl_from_IGamepad( iface );
    struct WineGameControllerState state;
    HRESULT hr;

    TRACE( "iface %p, value %p.\n", iface, value );

    if (FAILED(hr = IWineGameControllerProvider_get_State( impl->wine_provider, &state ))) return hr;

    memset(value, 0, sizeof(*value));
    if (impl->state_changed ||
        memcmp( impl->initial_state.axes, state.axes, sizeof(state) - offsetof(struct WineGameControllerState, axes)) )
    {
        impl->state_changed = TRUE;
        if (state.buttons[0]) value->Buttons |= GamepadButtons_A;
        if (state.buttons[1]) value->Buttons |= GamepadButtons_B;
        if (state.buttons[2]) value->Buttons |= GamepadButtons_X;
        if (state.buttons[3]) value->Buttons |= GamepadButtons_Y;
        if (state.buttons[4]) value->Buttons |= GamepadButtons_LeftShoulder;
        if (state.buttons[5]) value->Buttons |= GamepadButtons_RightShoulder;
        if (state.buttons[6]) value->Buttons |= GamepadButtons_View;
        if (state.buttons[7]) value->Buttons |= GamepadButtons_Menu;
        if (state.buttons[8]) value->Buttons |= GamepadButtons_LeftThumbstick;
        if (state.buttons[9]) value->Buttons |= GamepadButtons_RightThumbstick;

        switch (state.switches[0])
        {
        case GameControllerSwitchPosition_Up:
        case GameControllerSwitchPosition_UpRight:
        case GameControllerSwitchPosition_UpLeft:
            value->Buttons |= GamepadButtons_DPadUp;
            break;
        case GameControllerSwitchPosition_Down:
        case GameControllerSwitchPosition_DownRight:
        case GameControllerSwitchPosition_DownLeft:
            value->Buttons |= GamepadButtons_DPadDown;
            break;
        default:
            break;
        }

        switch (state.switches[0])
        {
        case GameControllerSwitchPosition_Right:
        case GameControllerSwitchPosition_UpRight:
        case GameControllerSwitchPosition_DownRight:
            value->Buttons |= GamepadButtons_DPadRight;
            break;
        case GameControllerSwitchPosition_Left:
        case GameControllerSwitchPosition_UpLeft:
        case GameControllerSwitchPosition_DownLeft:
            value->Buttons |= GamepadButtons_DPadLeft;
            break;
        default:
            break;
        }

        value->LeftThumbstickX = 2. * state.axes[0] - 1.;
        value->LeftThumbstickY = 2. * state.axes[1] - 1.;
        value->LeftTrigger = state.axes[2];
        value->RightThumbstickX = 2. * state.axes[3] - 1.;
        value->RightThumbstickY = 2. * state.axes[4] - 1.;
        value->RightTrigger = state.axes[5];

        value->Timestamp = state.timestamp;
    }

    return hr;
}

static const struct IGamepadVtbl gamepad_vtbl =
{
    gamepad_QueryInterface,
    gamepad_AddRef,
    gamepad_Release,
    /* IInspectable methods */
    gamepad_GetIids,
    gamepad_GetRuntimeClassName,
    gamepad_GetTrustLevel,
    /* IGamepad methods */
    gamepad_get_Vibration,
    gamepad_put_Vibration,
    gamepad_GetCurrentReading,
};

DEFINE_IINSPECTABLE_OUTER( gamepad2, IGamepad2, struct gamepad, IGameController_outer )

static HRESULT WINAPI gamepad2_GetButtonLabel( IGamepad2 *iface, GamepadButtons button, GameControllerButtonLabel *value )
{
    FIXME( "iface %p, button %#x, value %p stub!\n", iface, button, value );
    *value = GameControllerButtonLabel_None;
    return S_OK;
}

static const struct IGamepad2Vtbl gamepad2_vtbl =
{
    gamepad2_QueryInterface,
    gamepad2_AddRef,
    gamepad2_Release,
    /* IInspectable methods */
    gamepad2_GetIids,
    gamepad2_GetRuntimeClassName,
    gamepad2_GetTrustLevel,
    /* IGamepad2 methods */
    gamepad2_GetButtonLabel,
};

struct gamepad_statics
{
    IActivationFactory IActivationFactory_iface;
    IGamepadStatics IGamepadStatics_iface;
    IGamepadStatics2 IGamepadStatics2_iface;
    ICustomGameControllerFactory ICustomGameControllerFactory_iface;
    LONG ref;
};

static inline struct gamepad_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct gamepad_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct gamepad_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        IInspectable_AddRef( (*out = &impl->IActivationFactory_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IGamepadStatics ))
    {
        IInspectable_AddRef( (*out = &impl->IGamepadStatics_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IGamepadStatics2 ))
    {
        IInspectable_AddRef( (*out = &impl->IGamepadStatics2_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_ICustomGameControllerFactory ))
    {
        IInspectable_AddRef( (*out = &impl->ICustomGameControllerFactory_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct gamepad_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct gamepad_statics *impl = impl_from_IActivationFactory( iface );
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

DEFINE_IINSPECTABLE( statics, IGamepadStatics, struct gamepad_statics, IActivationFactory_iface )

static HRESULT WINAPI statics_add_GamepadAdded( IGamepadStatics *iface, IEventHandler_Gamepad *handler,
                                                EventRegistrationToken *token )
{
    TRACE( "iface %p, handler %p, token %p.\n", iface, handler, token );
    if (!handler) return E_INVALIDARG;
    return event_handlers_append( &gamepad_added_handlers, (IEventHandler_IInspectable *)handler, token );
}

static HRESULT WINAPI statics_remove_GamepadAdded( IGamepadStatics *iface, EventRegistrationToken token )
{
    TRACE( "iface %p, token %#I64x.\n", iface, token.value );
    return event_handlers_remove( &gamepad_added_handlers, &token );
}

static HRESULT WINAPI statics_add_GamepadRemoved( IGamepadStatics *iface, IEventHandler_Gamepad *handler,
                                                  EventRegistrationToken *token )
{
    TRACE( "iface %p, handler %p, token %p.\n", iface, handler, token );
    if (!handler) return E_INVALIDARG;
    return event_handlers_append( &gamepad_removed_handlers, (IEventHandler_IInspectable *)handler, token );
}

static HRESULT WINAPI statics_remove_GamepadRemoved( IGamepadStatics *iface, EventRegistrationToken token )
{
    TRACE( "iface %p, token %#I64x.\n", iface, token.value );
    return event_handlers_remove( &gamepad_removed_handlers, &token );
}

static HRESULT WINAPI statics_get_Gamepads( IGamepadStatics *iface, IVectorView_Gamepad **value )
{
    HRESULT hr;

    TRACE( "iface %p, value %p.\n", iface, value );

    EnterCriticalSection( &gamepad_cs );
    if (SUCCEEDED(hr = init_gamepads()))
        hr = IVector_Gamepad_GetView( gamepads, value );
    LeaveCriticalSection( &gamepad_cs );

    return hr;
}

static const struct IGamepadStaticsVtbl statics_vtbl =
{
    statics_QueryInterface,
    statics_AddRef,
    statics_Release,
    /* IInspectable methods */
    statics_GetIids,
    statics_GetRuntimeClassName,
    statics_GetTrustLevel,
    /* IGamepadStatics methods */
    statics_add_GamepadAdded,
    statics_remove_GamepadAdded,
    statics_add_GamepadRemoved,
    statics_remove_GamepadRemoved,
    statics_get_Gamepads,
};

DEFINE_IINSPECTABLE( statics2, IGamepadStatics2, struct gamepad_statics, IActivationFactory_iface )

static HRESULT WINAPI statics2_FromGameController( IGamepadStatics2 *iface, IGameController *game_controller, IGamepad **value )
{
    struct gamepad_statics *impl = impl_from_IGamepadStatics2( iface );
    IGameController *controller;
    HRESULT hr;

    TRACE( "iface %p, game_controller %p, value %p.\n", iface, game_controller, value );

    *value = NULL;
    hr = IGameControllerFactoryManagerStatics2_TryGetFactoryControllerFromGameController( manager_factory, &impl->ICustomGameControllerFactory_iface,
                                                                                          game_controller, &controller );
    if (FAILED(hr) || !controller) return hr;

    hr = IGameController_QueryInterface( controller, &IID_IGamepad, (void **)value );
    IGameController_Release( controller );
    return hr;
}

static const struct IGamepadStatics2Vtbl statics2_vtbl =
{
    statics2_QueryInterface,
    statics2_AddRef,
    statics2_Release,
    /* IInspectable methods */
    statics2_GetIids,
    statics2_GetRuntimeClassName,
    statics2_GetTrustLevel,
    /* IGamepadStatics2 methods */
    statics2_FromGameController,
};

DEFINE_IINSPECTABLE( controller_factory, ICustomGameControllerFactory, struct gamepad_statics, IActivationFactory_iface )

static HRESULT WINAPI controller_factory_CreateGameController( ICustomGameControllerFactory *iface, IGameControllerProvider *provider,
                                                               IInspectable **value )
{
    struct gamepad *impl;

    TRACE( "iface %p, provider %p, value %p.\n", iface, provider, value );

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->IGameControllerImpl_iface.lpVtbl = &controller_vtbl;
    impl->IGameControllerInputSink_iface.lpVtbl = &input_sink_vtbl;
    impl->IGamepad_iface.lpVtbl = &gamepad_vtbl;
    impl->IGamepad2_iface.lpVtbl = &gamepad2_vtbl;
    impl->ref = 1;

    TRACE( "created Gamepad %p\n", impl );

    *value = (IInspectable *)&impl->IGameControllerImpl_iface;
    return S_OK;
}

static HRESULT WINAPI controller_factory_OnGameControllerAdded( ICustomGameControllerFactory *iface, IGameController *value )
{
    IGamepad *gamepad;
    HRESULT hr;

    TRACE( "iface %p, value %p.\n", iface, value );

    if (FAILED(hr = IGameController_QueryInterface( value, &IID_IGamepad, (void **)&gamepad )))
        return hr;
    event_handlers_notify( &gamepad_added_handlers, (IInspectable *)gamepad );
    IGamepad_Release( gamepad );

    return S_OK;
}

static HRESULT WINAPI controller_factory_OnGameControllerRemoved( ICustomGameControllerFactory *iface, IGameController *value )
{
    IGamepad *gamepad;
    BOOLEAN found;
    UINT32 index;
    HRESULT hr;

    TRACE( "iface %p, value %p.\n", iface, value );

    if (FAILED(hr = IGameController_QueryInterface( value, &IID_IGamepad, (void **)&gamepad )))
        return hr;

    EnterCriticalSection( &gamepad_cs );
    if (SUCCEEDED(hr = init_gamepads()))
    {
        if (FAILED(hr = IVector_Gamepad_IndexOf( gamepads, gamepad, &index, &found )) || !found)
            WARN( "Could not find gamepad %p, hr %#lx!\n", gamepad, hr );
        else
            hr = IVector_Gamepad_RemoveAt( gamepads, index );
    }
    LeaveCriticalSection( &gamepad_cs );

    if (FAILED(hr))
        WARN( "Failed to remove gamepad %p, hr %#lx!\n", gamepad, hr );
    else if (found)
    {
        TRACE( "Removed gamepad %p.\n", gamepad );
        event_handlers_notify( &gamepad_removed_handlers, (IInspectable *)gamepad );
    }
    IGamepad_Release( gamepad );

    return S_OK;
}

static const struct ICustomGameControllerFactoryVtbl controller_factory_vtbl =
{
    controller_factory_QueryInterface,
    controller_factory_AddRef,
    controller_factory_Release,
    /* IInspectable methods */
    controller_factory_GetIids,
    controller_factory_GetRuntimeClassName,
    controller_factory_GetTrustLevel,
    /* ICustomGameControllerFactory methods */
    controller_factory_CreateGameController,
    controller_factory_OnGameControllerAdded,
    controller_factory_OnGameControllerRemoved,
};

static struct gamepad_statics gamepad_statics =
{
    {&factory_vtbl},
    {&statics_vtbl},
    {&statics2_vtbl},
    {&controller_factory_vtbl},
    1,
};

ICustomGameControllerFactory *gamepad_factory = &gamepad_statics.ICustomGameControllerFactory_iface;
