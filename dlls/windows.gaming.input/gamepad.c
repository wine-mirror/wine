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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(input);

struct gamepad_statics
{
    IActivationFactory IActivationFactory_iface;
    IGamepadStatics IGamepadStatics_iface;
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

static HRESULT WINAPI statics_add_GamepadAdded( IGamepadStatics *iface, IEventHandler_Gamepad *value,
                                                EventRegistrationToken *token )
{
    FIXME( "iface %p, value %p, token %p stub!\n", iface, value, token );
    if (!value) return E_INVALIDARG;
    token->value = 0;
    return S_OK;
}

static HRESULT WINAPI statics_remove_GamepadAdded( IGamepadStatics *iface, EventRegistrationToken token )
{
    FIXME( "iface %p, token %#I64x stub!\n", iface, token.value );
    return S_OK;
}

static HRESULT WINAPI statics_add_GamepadRemoved( IGamepadStatics *iface, IEventHandler_Gamepad *value,
                                                  EventRegistrationToken *token )
{
    FIXME( "iface %p, value %p, token %p stub!\n", iface, value, token );
    if (!value) return E_INVALIDARG;
    token->value = 0;
    return S_OK;
}

static HRESULT WINAPI statics_remove_GamepadRemoved( IGamepadStatics *iface, EventRegistrationToken token )
{
    FIXME( "iface %p, token %#I64x stub!\n", iface, token.value );
    return S_OK;
}

static HRESULT WINAPI statics_get_Gamepads( IGamepadStatics *iface, IVectorView_Gamepad **value )
{
    static const GUID *view_iid = &IID_IVectorView_Gamepad;
    static const GUID *iid = &IID_IVector_Gamepad;
    IVector_Gamepad *gamepads;
    HRESULT hr;

    TRACE( "iface %p, value %p.\n", iface, value );

    if (SUCCEEDED(hr = vector_create( iid, view_iid, (void **)&gamepads )))
    {
        hr = IVector_Gamepad_GetView( gamepads, value );
        IVector_Gamepad_Release( gamepads );
    }

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

DEFINE_IINSPECTABLE( controller_factory, ICustomGameControllerFactory, struct gamepad_statics, IActivationFactory_iface )

static HRESULT WINAPI controller_factory_CreateGameController( ICustomGameControllerFactory *iface, IGameControllerProvider *provider,
                                                               IInspectable **value )
{
    FIXME( "iface %p, provider %p, value %p stub!\n", iface, provider, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI controller_factory_OnGameControllerAdded( ICustomGameControllerFactory *iface, IGameController *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI controller_factory_OnGameControllerRemoved( ICustomGameControllerFactory *iface, IGameController *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
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
    {&controller_factory_vtbl},
    1,
};

ICustomGameControllerFactory *gamepad_factory = &gamepad_statics.ICustomGameControllerFactory_iface;
