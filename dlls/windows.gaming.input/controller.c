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

static CRITICAL_SECTION controller_cs;
static CRITICAL_SECTION_DEBUG controller_cs_debug =
{
    0, 0, &controller_cs,
    { &controller_cs_debug.ProcessLocksList, &controller_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": controller_cs") }
};
static CRITICAL_SECTION controller_cs = { &controller_cs_debug, -1, 0, 0, 0, 0 };

static IVector_RawGameController *controllers;
static struct list controller_added_handlers = LIST_INIT( controller_added_handlers );
static struct list controller_removed_handlers = LIST_INIT( controller_removed_handlers );

static HRESULT init_controllers(void)
{
    static const struct vector_iids iids =
    {
        .vector = &IID_IVector_RawGameController,
        .view = &IID_IVectorView_RawGameController,
        .iterable = &IID_IIterable_RawGameController,
        .iterator = &IID_IIterator_RawGameController,
    };
    HRESULT hr;

    EnterCriticalSection( &controller_cs );
    if (controllers) hr = S_OK;
    else hr = vector_create( &iids, (void **)&controllers );
    LeaveCriticalSection( &controller_cs );

    return hr;
}

struct controller
{
    IGameControllerImpl IGameControllerImpl_iface;
    IGameControllerInputSink IGameControllerInputSink_iface;
    IRawGameController IRawGameController_iface;
    IRawGameController2 IRawGameController2_iface;
    IGameController *IGameController_outer;
    LONG ref;

    IGameControllerProvider *provider;
    IWineGameControllerProvider *wine_provider;
};

static inline struct controller *impl_from_IGameControllerImpl( IGameControllerImpl *iface )
{
    return CONTAINING_RECORD( iface, struct controller, IGameControllerImpl_iface );
}

static HRESULT WINAPI controller_QueryInterface( IGameControllerImpl *iface, REFIID iid, void **out )
{
    struct controller *impl = impl_from_IGameControllerImpl( iface );

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

    if (IsEqualGUID( iid, &IID_IRawGameController ))
    {
        IInspectable_AddRef( (*out = &impl->IRawGameController_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IRawGameController2 ))
    {
        IInspectable_AddRef( (*out = &impl->IRawGameController2_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI controller_AddRef( IGameControllerImpl *iface )
{
    struct controller *impl = impl_from_IGameControllerImpl( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI controller_Release( IGameControllerImpl *iface )
{
    struct controller *impl = impl_from_IGameControllerImpl( iface );
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
    return WindowsCreateString( RuntimeClass_Windows_Gaming_Input_RawGameController,
                                ARRAY_SIZE(RuntimeClass_Windows_Gaming_Input_RawGameController),
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
    struct controller *impl = impl_from_IGameControllerImpl( iface );
    HRESULT hr;

    TRACE( "iface %p, outer %p, provider %p.\n", iface, outer, provider );

    impl->IGameController_outer = outer;
    IGameControllerProvider_AddRef( (impl->provider = provider) );

    hr = IGameControllerProvider_QueryInterface( provider, &IID_IWineGameControllerProvider,
                                                 (void **)&impl->wine_provider );
    if (FAILED(hr)) return hr;

    EnterCriticalSection( &controller_cs );
    if (SUCCEEDED(hr = init_controllers()))
        hr = IVector_RawGameController_Append( controllers, &impl->IRawGameController_iface );
    LeaveCriticalSection( &controller_cs );

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

DEFINE_IINSPECTABLE_OUTER( input_sink, IGameControllerInputSink, struct controller, IGameController_outer )

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

DEFINE_IINSPECTABLE_OUTER( raw_controller, IRawGameController, struct controller, IGameController_outer )

static HRESULT WINAPI raw_controller_get_AxisCount( IRawGameController *iface, INT32 *value )
{
    struct controller *impl = impl_from_IRawGameController( iface );
    return IWineGameControllerProvider_get_AxisCount( impl->wine_provider, value );
}

static HRESULT WINAPI raw_controller_get_ButtonCount( IRawGameController *iface, INT32 *value )
{
    struct controller *impl = impl_from_IRawGameController( iface );
    return IWineGameControllerProvider_get_ButtonCount( impl->wine_provider, value );
}

static HRESULT WINAPI raw_controller_get_ForceFeedbackMotors( IRawGameController *iface, IVectorView_ForceFeedbackMotor **value )
{
    static const struct vector_iids iids =
    {
        .vector = &IID_IVector_ForceFeedbackMotor,
        .view = &IID_IVectorView_ForceFeedbackMotor,
        .iterable = &IID_IIterable_ForceFeedbackMotor,
        .iterator = &IID_IIterator_ForceFeedbackMotor,
    };
    struct controller *impl = impl_from_IRawGameController( iface );
    IVector_ForceFeedbackMotor *vector;
    IForceFeedbackMotor *motor;
    HRESULT hr;

    TRACE( "iface %p, value %p\n", iface, value );

    if (FAILED(hr = vector_create( &iids, (void **)&vector ))) return hr;

    if (SUCCEEDED(IWineGameControllerProvider_get_ForceFeedbackMotor( impl->wine_provider, &motor )) && motor)
    {
        hr = IVector_ForceFeedbackMotor_Append( vector, motor );
        IForceFeedbackMotor_Release( motor );
    }

    if (SUCCEEDED(hr)) hr = IVector_ForceFeedbackMotor_GetView( vector, value );
    IVector_ForceFeedbackMotor_Release( vector );

    return hr;
}

static HRESULT WINAPI raw_controller_get_HardwareProductId( IRawGameController *iface, UINT16 *value )
{
    struct controller *impl = impl_from_IRawGameController( iface );
    return IGameControllerProvider_get_HardwareProductId( impl->provider, value );
}

static HRESULT WINAPI raw_controller_get_HardwareVendorId( IRawGameController *iface, UINT16 *value )
{
    struct controller *impl = impl_from_IRawGameController( iface );
    return IGameControllerProvider_get_HardwareVendorId( impl->provider, value );
}

static HRESULT WINAPI raw_controller_get_SwitchCount( IRawGameController *iface, INT32 *value )
{
    struct controller *impl = impl_from_IRawGameController( iface );
    return IWineGameControllerProvider_get_SwitchCount( impl->wine_provider, value );
}

static HRESULT WINAPI raw_controller_GetButtonLabel( IRawGameController *iface, INT32 index,
                                                     enum GameControllerButtonLabel *value )
{
    FIXME( "iface %p, index %d, value %p stub!\n", iface, index, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI raw_controller_GetCurrentReading( IRawGameController *iface, UINT32 buttons_size, BOOLEAN *buttons,
                                                        UINT32 switches_size, enum GameControllerSwitchPosition *switches,
                                                        UINT32 axes_size, DOUBLE *axes, UINT64 *timestamp )
{
    struct controller *impl = impl_from_IRawGameController( iface );
    WineGameControllerState state;
    HRESULT hr;

    TRACE( "iface %p, buttons_size %u, buttons %p, switches_size %u, switches %p, axes_size %u, axes %p, timestamp %p.\n",
           iface, buttons_size, buttons, switches_size, switches, axes_size, axes, timestamp );

    if (FAILED(hr = IWineGameControllerProvider_get_State( impl->wine_provider, &state ))) return hr;

    memcpy( axes, state.axes, axes_size * sizeof(*axes) );
    memcpy( buttons, state.buttons, buttons_size * sizeof(*buttons) );
    memcpy( switches, state.switches, switches_size * sizeof(*switches) );
    *timestamp = state.timestamp;

    return hr;
}

static HRESULT WINAPI raw_controller_GetSwitchKind( IRawGameController *iface, INT32 index, enum GameControllerSwitchKind *value )
{
    FIXME( "iface %p, index %d, value %p stub!\n", iface, index, value );
    return E_NOTIMPL;
}

static const struct IRawGameControllerVtbl raw_controller_vtbl =
{
    raw_controller_QueryInterface,
    raw_controller_AddRef,
    raw_controller_Release,
    /* IInspectable methods */
    raw_controller_GetIids,
    raw_controller_GetRuntimeClassName,
    raw_controller_GetTrustLevel,
    /* IRawGameController methods */
    raw_controller_get_AxisCount,
    raw_controller_get_ButtonCount,
    raw_controller_get_ForceFeedbackMotors,
    raw_controller_get_HardwareProductId,
    raw_controller_get_HardwareVendorId,
    raw_controller_get_SwitchCount,
    raw_controller_GetButtonLabel,
    raw_controller_GetCurrentReading,
    raw_controller_GetSwitchKind,
};

DEFINE_IINSPECTABLE_OUTER( raw_controller_2, IRawGameController2, struct controller, IGameController_outer )

static HRESULT WINAPI raw_controller_2_get_SimpleHapticsControllers( IRawGameController2 *iface, IVectorView_SimpleHapticsController** value)
{
    static const struct vector_iids iids =
    {
        .vector = &IID_IVector_SimpleHapticsController,
        .view = &IID_IVectorView_SimpleHapticsController,
        .iterable = &IID_IIterable_SimpleHapticsController,
        .iterator = &IID_IIterator_SimpleHapticsController,
    };
    IVector_SimpleHapticsController *vector;
    HRESULT hr;

    FIXME( "iface %p, value %p stub!\n", iface, value );

    if (SUCCEEDED(hr = vector_create( &iids, (void **)&vector )))
    {
        hr = IVector_SimpleHapticsController_GetView( vector, value );
        IVector_SimpleHapticsController_Release( vector );
    }

    return hr;
}

static HRESULT WINAPI raw_controller_2_get_NonRoamableId( IRawGameController2 *iface, HSTRING* value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI raw_controller_2_get_DisplayName( IRawGameController2 *iface, HSTRING* value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static const struct IRawGameController2Vtbl raw_controller_2_vtbl =
{
    raw_controller_2_QueryInterface,
    raw_controller_2_AddRef,
    raw_controller_2_Release,
    /* IInspectable methods */
    raw_controller_2_GetIids,
    raw_controller_2_GetRuntimeClassName,
    raw_controller_2_GetTrustLevel,
    /* IRawGameController2 methods */
    raw_controller_2_get_SimpleHapticsControllers,
    raw_controller_2_get_NonRoamableId,
    raw_controller_2_get_DisplayName,
};

struct controller_statics
{
    IActivationFactory IActivationFactory_iface;
    IRawGameControllerStatics IRawGameControllerStatics_iface;
    ICustomGameControllerFactory ICustomGameControllerFactory_iface;
    LONG ref;
};

static inline struct controller_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct controller_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct controller_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        IInspectable_AddRef( (*out = &impl->IActivationFactory_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IRawGameControllerStatics ))
    {
        IInspectable_AddRef( (*out = &impl->IRawGameControllerStatics_iface) );
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
    struct controller_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct controller_statics *impl = impl_from_IActivationFactory( iface );
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

DEFINE_IINSPECTABLE( statics, IRawGameControllerStatics, struct controller_statics, IActivationFactory_iface )

static HRESULT WINAPI statics_add_RawGameControllerAdded( IRawGameControllerStatics *iface,
                                                          IEventHandler_RawGameController *handler,
                                                          EventRegistrationToken *token )
{
    TRACE( "iface %p, handler %p, token %p.\n", iface, handler, token );
    if (!handler) return E_INVALIDARG;
    return event_handlers_append( &controller_added_handlers, (IEventHandler_IInspectable *)handler, token );
}

static HRESULT WINAPI statics_remove_RawGameControllerAdded( IRawGameControllerStatics *iface, EventRegistrationToken token )
{
    TRACE( "iface %p, token %#I64x.\n", iface, token.value );
    return event_handlers_remove( &controller_added_handlers, &token );
}

static HRESULT WINAPI statics_add_RawGameControllerRemoved( IRawGameControllerStatics *iface,
                                                            IEventHandler_RawGameController *handler,
                                                            EventRegistrationToken *token )
{
    TRACE( "iface %p, handler %p, token %p.\n", iface, handler, token );
    if (!handler) return E_INVALIDARG;
    return event_handlers_append( &controller_removed_handlers, (IEventHandler_IInspectable *)handler, token );
}

static HRESULT WINAPI statics_remove_RawGameControllerRemoved( IRawGameControllerStatics *iface, EventRegistrationToken token )
{
    TRACE( "iface %p, token %#I64x.\n", iface, token.value );
    return event_handlers_remove( &controller_removed_handlers, &token );
}

static HRESULT WINAPI statics_get_RawGameControllers( IRawGameControllerStatics *iface, IVectorView_RawGameController **value )
{
    HRESULT hr;

    TRACE( "iface %p, value %p.\n", iface, value );

    EnterCriticalSection( &controller_cs );
    if (SUCCEEDED(hr = init_controllers()))
        hr = IVector_RawGameController_GetView( controllers, value );
    LeaveCriticalSection( &controller_cs );

    return hr;
}

static HRESULT WINAPI statics_FromGameController( IRawGameControllerStatics *iface, IGameController *game_controller,
                                                  IRawGameController **value )
{
    struct controller_statics *impl = impl_from_IRawGameControllerStatics( iface );
    IGameController *controller;
    HRESULT hr;

    TRACE( "iface %p, game_controller %p, value %p.\n", iface, game_controller, value );

    *value = NULL;
    hr = IGameControllerFactoryManagerStatics2_TryGetFactoryControllerFromGameController( manager_factory, &impl->ICustomGameControllerFactory_iface,
                                                                                          game_controller, &controller );
    if (FAILED(hr) || !controller) return hr;

    hr = IGameController_QueryInterface( controller, &IID_IRawGameController, (void **)value );
    IGameController_Release( controller );

    return hr;
}

static const struct IRawGameControllerStaticsVtbl statics_vtbl =
{
    statics_QueryInterface,
    statics_AddRef,
    statics_Release,
    /* IInspectable methods */
    statics_GetIids,
    statics_GetRuntimeClassName,
    statics_GetTrustLevel,
    /* IRawGameControllerStatics methods */
    statics_add_RawGameControllerAdded,
    statics_remove_RawGameControllerAdded,
    statics_add_RawGameControllerRemoved,
    statics_remove_RawGameControllerRemoved,
    statics_get_RawGameControllers,
    statics_FromGameController,
};

DEFINE_IINSPECTABLE( controller_factory, ICustomGameControllerFactory, struct controller_statics, IActivationFactory_iface )

static HRESULT WINAPI controller_factory_CreateGameController( ICustomGameControllerFactory *iface, IGameControllerProvider *provider,
                                                               IInspectable **value )
{
    struct controller *impl;

    TRACE( "iface %p, provider %p, value %p.\n", iface, provider, value );

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->IGameControllerImpl_iface.lpVtbl = &controller_vtbl;
    impl->IGameControllerInputSink_iface.lpVtbl = &input_sink_vtbl;
    impl->IRawGameController_iface.lpVtbl = &raw_controller_vtbl;
    impl->IRawGameController2_iface.lpVtbl = &raw_controller_2_vtbl;
    impl->ref = 1;

    TRACE( "created RawGameController %p\n", impl );

    *value = (IInspectable *)&impl->IGameControllerImpl_iface;
    return S_OK;
}

static HRESULT WINAPI controller_factory_OnGameControllerAdded( ICustomGameControllerFactory *iface, IGameController *value )
{
    IRawGameController *controller;
    HRESULT hr;

    TRACE( "iface %p, value %p.\n", iface, value );

    if (FAILED(hr = IGameController_QueryInterface( value, &IID_IRawGameController, (void **)&controller )))
        return hr;
    event_handlers_notify( &controller_added_handlers, (IInspectable *)controller );
    IRawGameController_Release( controller );

    return S_OK;
}

static HRESULT WINAPI controller_factory_OnGameControllerRemoved( ICustomGameControllerFactory *iface, IGameController *value )
{
    IRawGameController *controller;
    BOOLEAN found;
    UINT32 index;
    HRESULT hr;

    TRACE( "iface %p, value %p.\n", iface, value );

    if (FAILED(hr = IGameController_QueryInterface( value, &IID_IRawGameController, (void **)&controller )))
        return hr;

    EnterCriticalSection( &controller_cs );
    if (SUCCEEDED(hr = init_controllers()))
    {
        if (FAILED(hr = IVector_RawGameController_IndexOf( controllers, controller, &index, &found )) || !found)
            WARN( "Could not find controller %p, hr %#lx!\n", controller, hr );
        else
            hr = IVector_RawGameController_RemoveAt( controllers, index );
    }
    LeaveCriticalSection( &controller_cs );

    if (FAILED(hr))
        WARN( "Failed to remove controller %p, hr %#lx!\n", controller, hr );
    else if (found)
    {
        TRACE( "Removed controller %p.\n", controller );
        event_handlers_notify( &controller_removed_handlers, (IInspectable *)controller );
    }
    IRawGameController_Release( controller );

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

static struct controller_statics controller_statics =
{
    {&factory_vtbl},
    {&statics_vtbl},
    {&controller_factory_vtbl},
    1,
};

ICustomGameControllerFactory *controller_factory = &controller_statics.ICustomGameControllerFactory_iface;
