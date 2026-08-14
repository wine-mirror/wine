/* WinRT Windows.Xbox.Input.Gamepad implementation
 *
 * Backed by the real Windows.Gaming.Input.Gamepad implementation in
 * windows.gaming.input.dll (see wgi_backend.c), reusing its live HID /
 * XInput controller access rather than reimplementing it.
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

WINE_DEFAULT_DEBUG_CHANNEL(xbox);

/* Windows.Gaming.Input.GamepadButtons (include/windows.gaming.input.idl)
 * and Windows.Xbox.Input.GamepadButtons (dlls/windows.xbox/input.idl, taken
 * from the reverse engineered Xbox One ERA metadata) do not share bit
 * values, so buttons are remapped bit-for-bit here instead of assumed
 * equal. The WGI side is given as a literal hex mask (rather than its
 * symbolic GamepadButtons_* name) because windows.gaming.input.h cannot be
 * included from this file -- see wgi_backend.h for why. */
static GamepadButtons map_wgi_buttons( UINT32 wgi_buttons )
{
    GamepadButtons value = GamepadButtons_None;

    if (wgi_buttons & 0x0001) value |= GamepadButtons_Menu;            /* WGI Menu */
    if (wgi_buttons & 0x0002) value |= GamepadButtons_View;            /* WGI View */
    if (wgi_buttons & 0x0004) value |= GamepadButtons_A;               /* WGI A */
    if (wgi_buttons & 0x0008) value |= GamepadButtons_B;               /* WGI B */
    if (wgi_buttons & 0x0010) value |= GamepadButtons_X;               /* WGI X */
    if (wgi_buttons & 0x0020) value |= GamepadButtons_Y;               /* WGI Y */
    if (wgi_buttons & 0x0040) value |= GamepadButtons_DPadUp;          /* WGI DPadUp */
    if (wgi_buttons & 0x0080) value |= GamepadButtons_DPadDown;        /* WGI DPadDown */
    if (wgi_buttons & 0x0100) value |= GamepadButtons_DPadLeft;        /* WGI DPadLeft */
    if (wgi_buttons & 0x0200) value |= GamepadButtons_DPadRight;       /* WGI DPadRight */
    if (wgi_buttons & 0x0400) value |= GamepadButtons_LeftShoulder;    /* WGI LeftShoulder */
    if (wgi_buttons & 0x0800) value |= GamepadButtons_RightShoulder;   /* WGI RightShoulder */
    if (wgi_buttons & 0x1000) value |= GamepadButtons_LeftThumbstick;  /* WGI LeftThumbstick */
    if (wgi_buttons & 0x2000) value |= GamepadButtons_RightThumbstick; /* WGI RightThumbstick */
    /* WGI Paddle1-4 (0x4000-0x20000) have no Windows.Xbox.Input.GamepadButtons equivalent. */

    return value;
}

/*
 * IGamepadReading - a snapshot of a raw reading, exposing the same fields
 * as RawGamepadReading plus the IsXxxPressed booleans and a wall-clock
 * Windows.Foundation.DateTime timestamp (the raw UINT64 in RawGamepadReading
 * is the opaque device/XInput packet counter, not a wall-clock time, so it
 * would not be a meaningful DateTime).
 */
struct gamepad_reading
{
    IGamepadReading IGamepadReading_iface;
    LONG ref;

    RawGamepadReading raw;
    DateTime timestamp;
};

static inline struct gamepad_reading *impl_from_IGamepadReading( IGamepadReading *iface )
{
    return CONTAINING_RECORD( iface, struct gamepad_reading, IGamepadReading_iface );
}

static HRESULT WINAPI gamepad_reading_QueryInterface( IGamepadReading *iface, REFIID iid, void **out )
{
    struct gamepad_reading *impl = impl_from_IGamepadReading( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IGamepadReading ))
    {
        IInspectable_AddRef( (*out = &impl->IGamepadReading_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI gamepad_reading_AddRef( IGamepadReading *iface )
{
    struct gamepad_reading *impl = impl_from_IGamepadReading( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI gamepad_reading_Release( IGamepadReading *iface )
{
    struct gamepad_reading *impl = impl_from_IGamepadReading( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref) free( impl );

    return ref;
}

static HRESULT WINAPI gamepad_reading_GetIids( IGamepadReading *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI gamepad_reading_GetRuntimeClassName( IGamepadReading *iface, HSTRING *class_name )
{
    TRACE( "iface %p, class_name %p.\n", iface, class_name );
    return WindowsCreateString( RuntimeClass_Windows_Xbox_Input_GamepadReading,
                                ARRAY_SIZE(RuntimeClass_Windows_Xbox_Input_GamepadReading) - 1, class_name );
}

static HRESULT WINAPI gamepad_reading_GetTrustLevel( IGamepadReading *iface, TrustLevel *trust_level )
{
    TRACE( "iface %p, trust_level %p.\n", iface, trust_level );
    *trust_level = BaseTrust;
    return S_OK;
}

static HRESULT WINAPI gamepad_reading_get_Timestamp( IGamepadReading *iface, DateTime *value )
{
    struct gamepad_reading *impl = impl_from_IGamepadReading( iface );
    TRACE( "iface %p, value %p.\n", iface, value );
    *value = impl->timestamp;
    return S_OK;
}

static HRESULT WINAPI gamepad_reading_get_Buttons( IGamepadReading *iface, GamepadButtons *value )
{
    struct gamepad_reading *impl = impl_from_IGamepadReading( iface );
    TRACE( "iface %p, value %p.\n", iface, value );
    *value = impl->raw.Buttons;
    return S_OK;
}

#define DEFINE_GAMEPAD_READING_BUTTON_PROP( name, flag )                                       \
    static HRESULT WINAPI gamepad_reading_get_##name( IGamepadReading *iface, boolean *value ) \
    {                                                                                          \
        struct gamepad_reading *impl = impl_from_IGamepadReading( iface );                     \
        TRACE( "iface %p, value %p.\n", iface, value );                                        \
        *value = !!(impl->raw.Buttons & (flag));                                               \
        return S_OK;                                                                           \
    }

DEFINE_GAMEPAD_READING_BUTTON_PROP( IsDPadUpPressed, GamepadButtons_DPadUp )
DEFINE_GAMEPAD_READING_BUTTON_PROP( IsDPadDownPressed, GamepadButtons_DPadDown )
DEFINE_GAMEPAD_READING_BUTTON_PROP( IsDPadLeftPressed, GamepadButtons_DPadLeft )
DEFINE_GAMEPAD_READING_BUTTON_PROP( IsDPadRightPressed, GamepadButtons_DPadRight )
DEFINE_GAMEPAD_READING_BUTTON_PROP( IsMenuPressed, GamepadButtons_Menu )
DEFINE_GAMEPAD_READING_BUTTON_PROP( IsViewPressed, GamepadButtons_View )
DEFINE_GAMEPAD_READING_BUTTON_PROP( IsLeftThumbstickPressed, GamepadButtons_LeftThumbstick )
DEFINE_GAMEPAD_READING_BUTTON_PROP( IsRightThumbstickPressed, GamepadButtons_RightThumbstick )
DEFINE_GAMEPAD_READING_BUTTON_PROP( IsLeftShoulderPressed, GamepadButtons_LeftShoulder )
DEFINE_GAMEPAD_READING_BUTTON_PROP( IsRightShoulderPressed, GamepadButtons_RightShoulder )
DEFINE_GAMEPAD_READING_BUTTON_PROP( IsAPressed, GamepadButtons_A )
DEFINE_GAMEPAD_READING_BUTTON_PROP( IsBPressed, GamepadButtons_B )
DEFINE_GAMEPAD_READING_BUTTON_PROP( IsXPressed, GamepadButtons_X )
DEFINE_GAMEPAD_READING_BUTTON_PROP( IsYPressed, GamepadButtons_Y )

#undef DEFINE_GAMEPAD_READING_BUTTON_PROP

#define DEFINE_GAMEPAD_READING_FLOAT_PROP( name )                                            \
    static HRESULT WINAPI gamepad_reading_get_##name( IGamepadReading *iface, FLOAT *value ) \
    {                                                                                        \
        struct gamepad_reading *impl = impl_from_IGamepadReading( iface );                   \
        TRACE( "iface %p, value %p.\n", iface, value );                                      \
        *value = impl->raw.name;                                                             \
        return S_OK;                                                                         \
    }

DEFINE_GAMEPAD_READING_FLOAT_PROP( LeftTrigger )
DEFINE_GAMEPAD_READING_FLOAT_PROP( RightTrigger )
DEFINE_GAMEPAD_READING_FLOAT_PROP( LeftThumbstickX )
DEFINE_GAMEPAD_READING_FLOAT_PROP( LeftThumbstickY )
DEFINE_GAMEPAD_READING_FLOAT_PROP( RightThumbstickX )
DEFINE_GAMEPAD_READING_FLOAT_PROP( RightThumbstickY )

#undef DEFINE_GAMEPAD_READING_FLOAT_PROP

static const struct IGamepadReadingVtbl gamepad_reading_vtbl =
{
    gamepad_reading_QueryInterface,
    gamepad_reading_AddRef,
    gamepad_reading_Release,
    /* IInspectable methods */
    gamepad_reading_GetIids,
    gamepad_reading_GetRuntimeClassName,
    gamepad_reading_GetTrustLevel,
    /* IGamepadReading methods */
    gamepad_reading_get_Timestamp,
    gamepad_reading_get_Buttons,
    gamepad_reading_get_IsDPadUpPressed,
    gamepad_reading_get_IsDPadDownPressed,
    gamepad_reading_get_IsDPadLeftPressed,
    gamepad_reading_get_IsDPadRightPressed,
    gamepad_reading_get_IsMenuPressed,
    gamepad_reading_get_IsViewPressed,
    gamepad_reading_get_IsLeftThumbstickPressed,
    gamepad_reading_get_IsRightThumbstickPressed,
    gamepad_reading_get_IsLeftShoulderPressed,
    gamepad_reading_get_IsRightShoulderPressed,
    gamepad_reading_get_IsAPressed,
    gamepad_reading_get_IsBPressed,
    gamepad_reading_get_IsXPressed,
    gamepad_reading_get_IsYPressed,
    gamepad_reading_get_LeftTrigger,
    gamepad_reading_get_RightTrigger,
    gamepad_reading_get_LeftThumbstickX,
    gamepad_reading_get_LeftThumbstickY,
    gamepad_reading_get_RightThumbstickX,
    gamepad_reading_get_RightThumbstickY,
};

static HRESULT create_gamepad_reading( const RawGamepadReading *raw, IGamepadReading **out )
{
    struct gamepad_reading *impl;
    FILETIME now;

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->IGamepadReading_iface.lpVtbl = &gamepad_reading_vtbl;
    impl->ref = 1;
    impl->raw = *raw;

    GetSystemTimeAsFileTime( &now );
    impl->timestamp.UniversalTime = ((INT64)now.dwHighDateTime << 32) | now.dwLowDateTime;

    TRACE( "created reading %p\n", impl );

    *out = &impl->IGamepadReading_iface;
    return S_OK;
}

/*
 * IGamepad
 */
struct gamepad
{
    IGamepad IGamepad_iface;
    LONG ref;

    void *wgi_handle; /* AddRef'd Windows.Gaming.Input.IGamepad, via wgi_backend.c */
};

static inline struct gamepad *impl_from_IGamepad( IGamepad *iface )
{
    return CONTAINING_RECORD( iface, struct gamepad, IGamepad_iface );
}

static HRESULT WINAPI gamepad_QueryInterface( IGamepad *iface, REFIID iid, void **out )
{
    struct gamepad *impl = impl_from_IGamepad( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IGamepad ))
    {
        IInspectable_AddRef( (*out = &impl->IGamepad_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI gamepad_AddRef( IGamepad *iface )
{
    struct gamepad *impl = impl_from_IGamepad( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI gamepad_Release( IGamepad *iface )
{
    struct gamepad *impl = impl_from_IGamepad( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref)
    {
        wgi_backend_release( impl->wgi_handle );
        free( impl );
    }

    return ref;
}

static HRESULT WINAPI gamepad_GetIids( IGamepad *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI gamepad_GetRuntimeClassName( IGamepad *iface, HSTRING *class_name )
{
    TRACE( "iface %p, class_name %p.\n", iface, class_name );
    return WindowsCreateString( RuntimeClass_Windows_Xbox_Input_Gamepad,
                                ARRAY_SIZE(RuntimeClass_Windows_Xbox_Input_Gamepad) - 1, class_name );
}

static HRESULT WINAPI gamepad_GetTrustLevel( IGamepad *iface, TrustLevel *trust_level )
{
    TRACE( "iface %p, trust_level %p.\n", iface, trust_level );
    *trust_level = BaseTrust;
    return S_OK;
}

static HRESULT WINAPI gamepad_SetVibration( IGamepad *iface, GamepadVibration value )
{
    struct gamepad *impl = impl_from_IGamepad( iface );
    struct wgi_gamepad_vibration vibration =
    {
        .left_motor = value.LeftMotorLevel,
        .right_motor = value.RightMotorLevel,
        .left_trigger = value.LeftTriggerLevel,
        .right_trigger = value.RightTriggerLevel,
    };

    TRACE( "iface %p, value %p.\n", iface, &value );

    return wgi_backend_set_vibration( impl->wgi_handle, &vibration );
}

static HRESULT WINAPI gamepad_GetRawCurrentReading( IGamepad *iface, RawGamepadReading *value )
{
    struct gamepad *impl = impl_from_IGamepad( iface );
    struct wgi_gamepad_reading reading;
    HRESULT hr;

    TRACE( "iface %p, value %p.\n", iface, value );

    memset( value, 0, sizeof(*value) );
    if (FAILED(hr = wgi_backend_get_reading( impl->wgi_handle, &reading ))) return hr;

    value->Timestamp = reading.timestamp;
    value->Buttons = map_wgi_buttons( reading.buttons );
    value->LeftTrigger = reading.left_trigger;
    value->RightTrigger = reading.right_trigger;
    value->LeftThumbstickX = reading.left_thumbstick_x;
    value->LeftThumbstickY = reading.left_thumbstick_y;
    value->RightThumbstickX = reading.right_thumbstick_x;
    value->RightThumbstickY = reading.right_thumbstick_y;

    return S_OK;
}

static HRESULT WINAPI gamepad_GetCurrentReading( IGamepad *iface, IGamepadReading **value )
{
    RawGamepadReading raw;
    HRESULT hr;

    TRACE( "iface %p, value %p.\n", iface, value );

    *value = NULL;
    if (FAILED(hr = gamepad_GetRawCurrentReading( iface, &raw ))) return hr;
    return create_gamepad_reading( &raw, value );
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
    gamepad_SetVibration,
    gamepad_GetCurrentReading,
    gamepad_GetRawCurrentReading,
};

static HRESULT create_gamepad( void *wgi_handle, IGamepad **out )
{
    struct gamepad *impl;

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->IGamepad_iface.lpVtbl = &gamepad_vtbl;
    impl->ref = 1;

    wgi_backend_addref( wgi_handle );
    impl->wgi_handle = wgi_handle;

    TRACE( "created gamepad %p\n", impl );

    *out = &impl->IGamepad_iface;
    return S_OK;
}

/*
 * IGamepadStatics / activation factory
 */
struct gamepad_statics
{
    IActivationFactory IActivationFactory_iface;
    IGamepadStatics IGamepadStatics_iface;
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
    TRACE( "iface %p, class_name %p.\n", iface, class_name );
    return WindowsCreateString( RuntimeClass_Windows_Xbox_Input_Gamepad,
                                ARRAY_SIZE(RuntimeClass_Windows_Xbox_Input_Gamepad) - 1, class_name );
}

static HRESULT WINAPI factory_GetTrustLevel( IActivationFactory *iface, TrustLevel *trust_level )
{
    TRACE( "iface %p, trust_level %p.\n", iface, trust_level );
    *trust_level = BaseTrust;
    return S_OK;
}

static HRESULT WINAPI factory_ActivateInstance( IActivationFactory *iface, IInspectable **instance )
{
    /* Windows.Xbox.Input.Gamepad is statics-only, like its
     * Windows.Gaming.Input.Gamepad counterpart: it is never directly
     * activated, only enumerated via IGamepadStatics::get_Gamepads. */
    FIXME( "iface %p, instance %p, Gamepad is not activatable.\n", iface, instance );
    *instance = NULL;
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

static HRESULT WINAPI statics_get_Gamepads( IGamepadStatics *iface, IVectorView_IGamepad **value )
{
    static const struct vector_view_iids iids =
    {
        .view = &IID_IVectorView_IGamepad,
        .iterable = &IID_IIterable_IGamepad,
        .iterator = &IID_IIterator_IGamepad,
    };
    IInspectable **elements = NULL;
    void **handles;
    UINT32 count, i;
    HRESULT hr;

    TRACE( "iface %p, value %p.\n", iface, value );

    *value = NULL;

    if (FAILED(hr = wgi_backend_get_gamepads( &handles, &count ))) return hr;

    if (count && !(elements = calloc( count, sizeof(*elements) )))
    {
        for (i = 0; i < count; ++i) wgi_backend_release( handles[i] );
        free( handles );
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < count; ++i)
    {
        IGamepad *gamepad;
        if (FAILED(hr = create_gamepad( handles[i], &gamepad ))) break;
        elements[i] = (IInspectable *)gamepad;
    }

    for (i = 0; i < count; ++i) wgi_backend_release( handles[i] );
    free( handles );

    if (FAILED(hr))
    {
        while (i--) IInspectable_Release( elements[i] );
        free( elements );
        return hr;
    }

    hr = vector_view_create( &iids, elements, count, (void **)value );

    for (i = 0; i < count; ++i) IInspectable_Release( elements[i] );
    free( elements );

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
    statics_get_Gamepads,
};

static struct gamepad_statics gamepad_statics =
{
    {&factory_vtbl},
    {&statics_vtbl},
    1,
};

IActivationFactory *xbox_gamepad_factory = &gamepad_statics.IActivationFactory_iface;
