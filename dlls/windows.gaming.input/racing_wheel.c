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

static CRITICAL_SECTION racing_wheel_cs;
static CRITICAL_SECTION_DEBUG racing_wheel_cs_debug =
{
    0, 0, &racing_wheel_cs,
    { &racing_wheel_cs_debug.ProcessLocksList, &racing_wheel_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": racing_wheel_cs") }
};
static CRITICAL_SECTION racing_wheel_cs = { &racing_wheel_cs_debug, -1, 0, 0, 0, 0 };

static IVector_RacingWheel *racing_wheels;
static struct list racing_wheel_added_handlers = LIST_INIT( racing_wheel_added_handlers );
static struct list racing_wheel_removed_handlers = LIST_INIT( racing_wheel_removed_handlers );

static HRESULT init_racing_wheels(void)
{
    static const struct vector_iids iids =
    {
        .vector = &IID_IVector_RacingWheel,
        .view = &IID_IVectorView_RacingWheel,
        .iterable = &IID_IIterable_RacingWheel,
        .iterator = &IID_IIterator_RacingWheel,
    };
    HRESULT hr;

    EnterCriticalSection( &racing_wheel_cs );
    if (racing_wheels) hr = S_OK;
    else hr = vector_create( &iids, (void **)&racing_wheels );
    LeaveCriticalSection( &racing_wheel_cs );

    return hr;
}

struct racing_wheel_statics
{
    IActivationFactory IActivationFactory_iface;
    IRacingWheelStatics IRacingWheelStatics_iface;
    IRacingWheelStatics2 IRacingWheelStatics2_iface;
    ICustomGameControllerFactory ICustomGameControllerFactory_iface;
    LONG ref;
};

static inline struct racing_wheel_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct racing_wheel_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct racing_wheel_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        IInspectable_AddRef( (*out = &impl->IActivationFactory_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IRacingWheelStatics ))
    {
        IInspectable_AddRef( (*out = &impl->IRacingWheelStatics_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IRacingWheelStatics2 ))
    {
        IInspectable_AddRef( (*out = &impl->IRacingWheelStatics2_iface) );
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
    struct racing_wheel_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct racing_wheel_statics *impl = impl_from_IActivationFactory( iface );
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

DEFINE_IINSPECTABLE( statics, IRacingWheelStatics, struct racing_wheel_statics, IActivationFactory_iface )

static HRESULT WINAPI statics_add_RacingWheelAdded( IRacingWheelStatics *iface, IEventHandler_RacingWheel *handler,
                                                    EventRegistrationToken *token )
{
    TRACE( "iface %p, handler %p, token %p.\n", iface, handler, token );
    if (!handler) return E_INVALIDARG;
    return event_handlers_append( &racing_wheel_added_handlers, (IEventHandler_IInspectable *)handler, token );
}

static HRESULT WINAPI statics_remove_RacingWheelAdded( IRacingWheelStatics *iface, EventRegistrationToken token )
{
    TRACE( "iface %p, token %#I64x.\n", iface, token.value );
    return event_handlers_remove( &racing_wheel_added_handlers, &token );
}

static HRESULT WINAPI statics_add_RacingWheelRemoved( IRacingWheelStatics *iface, IEventHandler_RacingWheel *handler,
                                                      EventRegistrationToken *token )
{
    TRACE( "iface %p, handler %p, token %p.\n", iface, handler, token );
    if (!handler) return E_INVALIDARG;
    return event_handlers_append( &racing_wheel_removed_handlers, (IEventHandler_IInspectable *)handler, token );
}

static HRESULT WINAPI statics_remove_RacingWheelRemoved( IRacingWheelStatics *iface, EventRegistrationToken token )
{
    TRACE( "iface %p, token %#I64x.\n", iface, token.value );
    return event_handlers_remove( &racing_wheel_removed_handlers, &token );
}

static HRESULT WINAPI statics_get_RacingWheels( IRacingWheelStatics *iface, IVectorView_RacingWheel **value )
{
    HRESULT hr;

    TRACE( "iface %p, value %p.\n", iface, value );

    EnterCriticalSection( &racing_wheel_cs );
    if (SUCCEEDED(hr = init_racing_wheels())) hr = IVector_RacingWheel_GetView( racing_wheels, value );
    LeaveCriticalSection( &racing_wheel_cs );

    return hr;
}

static const struct IRacingWheelStaticsVtbl statics_vtbl =
{
    statics_QueryInterface,
    statics_AddRef,
    statics_Release,
    /* IInspectable methods */
    statics_GetIids,
    statics_GetRuntimeClassName,
    statics_GetTrustLevel,
    /* IRacingWheelStatics methods */
    statics_add_RacingWheelAdded,
    statics_remove_RacingWheelAdded,
    statics_add_RacingWheelRemoved,
    statics_remove_RacingWheelRemoved,
    statics_get_RacingWheels,
};

DEFINE_IINSPECTABLE( statics2, IRacingWheelStatics2, struct racing_wheel_statics, IActivationFactory_iface )

static HRESULT WINAPI statics2_FromGameController( IRacingWheelStatics2 *iface, IGameController *game_controller, IRacingWheel **value )
{
    struct racing_wheel_statics *impl = impl_from_IRacingWheelStatics2( iface );
    IGameController *controller;
    HRESULT hr;

    TRACE( "iface %p, game_controller %p, value %p.\n", iface, game_controller, value );

    *value = NULL;
    hr = IGameControllerFactoryManagerStatics2_TryGetFactoryControllerFromGameController( manager_factory, &impl->ICustomGameControllerFactory_iface,
                                                                                          game_controller, &controller );
    if (FAILED(hr) || !controller) return hr;

    hr = IGameController_QueryInterface( controller, &IID_IRacingWheel, (void **)value );
    IGameController_Release( controller );
    return hr;
}

static const struct IRacingWheelStatics2Vtbl statics2_vtbl =
{
    statics2_QueryInterface,
    statics2_AddRef,
    statics2_Release,
    /* IInspectable methods */
    statics2_GetIids,
    statics2_GetRuntimeClassName,
    statics2_GetTrustLevel,
    /* IRacingWheelStatics2 methods */
    statics2_FromGameController,
};

DEFINE_IINSPECTABLE( controller_factory, ICustomGameControllerFactory, struct racing_wheel_statics, IActivationFactory_iface )

static HRESULT WINAPI controller_factory_CreateGameController( ICustomGameControllerFactory *iface, IGameControllerProvider *provider,
                                                               IInspectable **value )
{
    FIXME( "iface %p, provider %p, value %p stub!.\n", iface, provider, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI controller_factory_OnGameControllerAdded( ICustomGameControllerFactory *iface, IGameController *value )
{
    IRacingWheel *racing_wheel;
    HRESULT hr;

    TRACE( "iface %p, value %p.\n", iface, value );

    if (FAILED(hr = IGameController_QueryInterface( value, &IID_IRacingWheel, (void **)&racing_wheel )))
        return hr;
    event_handlers_notify( &racing_wheel_added_handlers, (IInspectable *)racing_wheel );
    IRacingWheel_Release( racing_wheel );

    return S_OK;
}

static HRESULT WINAPI controller_factory_OnGameControllerRemoved( ICustomGameControllerFactory *iface, IGameController *value )
{
    IRacingWheel *racing_wheel;
    BOOLEAN found;
    UINT32 index;
    HRESULT hr;

    TRACE( "iface %p, value %p.\n", iface, value );

    if (FAILED(hr = IGameController_QueryInterface( value, &IID_IRacingWheel, (void **)&racing_wheel )))
        return hr;

    EnterCriticalSection( &racing_wheel_cs );
    if (SUCCEEDED(hr = init_racing_wheels()))
    {
        if (FAILED(hr = IVector_RacingWheel_IndexOf( racing_wheels, racing_wheel, &index, &found )) || !found)
            WARN( "Could not find RacingWheel %p, hr %#lx!\n", racing_wheel, hr );
        else
            hr = IVector_RacingWheel_RemoveAt( racing_wheels, index );
    }
    LeaveCriticalSection( &racing_wheel_cs );

    if (FAILED(hr))
        WARN( "Failed to remove RacingWheel %p, hr %#lx!\n", racing_wheel, hr );
    else if (found)
    {
        TRACE( "Removed RacingWheel %p.\n", racing_wheel );
        event_handlers_notify( &racing_wheel_removed_handlers, (IInspectable *)racing_wheel );
    }
    IRacingWheel_Release( racing_wheel );

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

static struct racing_wheel_statics racing_wheel_statics =
{
    {&factory_vtbl},
    {&statics_vtbl},
    {&statics2_vtbl},
    {&controller_factory_vtbl},
    1,
};

ICustomGameControllerFactory *racing_wheel_factory = &racing_wheel_statics.ICustomGameControllerFactory_iface;
