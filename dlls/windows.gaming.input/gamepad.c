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

struct gamepad_vector
{
    IVectorView_Gamepad IVectorView_Gamepad_iface;
    LONG ref;
};

static inline struct gamepad_vector *impl_from_IVectorView_Gamepad( IVectorView_Gamepad *iface )
{
    return CONTAINING_RECORD( iface, struct gamepad_vector, IVectorView_Gamepad_iface );
}

static HRESULT WINAPI gamepads_QueryInterface( IVectorView_Gamepad *iface, REFIID iid, void **out )
{
    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IVectorView_Gamepad ))
    {
        IUnknown_AddRef( iface );
        *out = iface;
        return S_OK;
    }

    WARN( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI gamepads_AddRef( IVectorView_Gamepad *iface )
{
    struct gamepad_vector *impl = impl_from_IVectorView_Gamepad( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI gamepads_Release( IVectorView_Gamepad *iface )
{
    struct gamepad_vector *impl = impl_from_IVectorView_Gamepad( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI gamepads_GetIids( IVectorView_Gamepad *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI gamepads_GetRuntimeClassName( IVectorView_Gamepad *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI gamepads_GetTrustLevel( IVectorView_Gamepad *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI gamepads_GetAt( IVectorView_Gamepad *iface, UINT32 index, IGamepad **value )
{
    FIXME( "iface %p, index %u, value %p stub!\n", iface, index, value );
    *value = NULL;
    return E_BOUNDS;
}

static HRESULT WINAPI gamepads_get_Size( IVectorView_Gamepad *iface, UINT32 *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    *value = 0;
    return S_OK;
}

static HRESULT WINAPI gamepads_IndexOf( IVectorView_Gamepad *iface, IGamepad *element, UINT32 *index, BOOLEAN *found )
{
    FIXME( "iface %p, element %p, index %p, found %p stub!\n", iface, element, index, found );
    *index = 0;
    *found = FALSE;
    return S_OK;
}

static HRESULT WINAPI gamepads_GetMany( IVectorView_Gamepad *iface, UINT32 start_index,
                                        UINT32 items_size, IGamepad **items, UINT *value )
{
    FIXME( "iface %p, start_index %u, items_size %u, items %p, value %p stub!\n", iface,
           start_index, items_size, items, value );
    *value = 0;
    return E_BOUNDS;
}

static const struct IVectorView_GamepadVtbl gamepads_vtbl =
{
    gamepads_QueryInterface,
    gamepads_AddRef,
    gamepads_Release,
    /* IInspectable methods */
    gamepads_GetIids,
    gamepads_GetRuntimeClassName,
    gamepads_GetTrustLevel,
    /* IVectorView<VoiceInformation> methods */
    gamepads_GetAt,
    gamepads_get_Size,
    gamepads_IndexOf,
    gamepads_GetMany,
};

static struct gamepad_vector gamepads =
{
    {&gamepads_vtbl},
    0,
};

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

static inline struct gamepad_statics *impl_from_IGamepadStatics( IGamepadStatics *iface )
{
    return CONTAINING_RECORD( iface, struct gamepad_statics, IGamepadStatics_iface );
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
        IUnknown_AddRef( iface );
        *out = iface;
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IGamepadStatics ))
    {
        IUnknown_AddRef( iface );
        *out = &impl->IGamepadStatics_iface;
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

static HRESULT WINAPI statics_QueryInterface( IGamepadStatics *iface, REFIID iid, void **out )
{
    struct gamepad_statics *impl = impl_from_IGamepadStatics( iface );
    return IActivationFactory_QueryInterface( &impl->IActivationFactory_iface, iid, out );
}

static ULONG WINAPI statics_AddRef( IGamepadStatics *iface )
{
    struct gamepad_statics *impl = impl_from_IGamepadStatics( iface );
    return IActivationFactory_AddRef( &impl->IActivationFactory_iface );
}

static ULONG WINAPI statics_Release( IGamepadStatics *iface )
{
    struct gamepad_statics *impl = impl_from_IGamepadStatics( iface );
    return IActivationFactory_Release( &impl->IActivationFactory_iface );
}

static HRESULT WINAPI statics_GetIids( IGamepadStatics *iface, ULONG *iid_count, IID **iids )
{
    struct gamepad_statics *impl = impl_from_IGamepadStatics( iface );
    return IActivationFactory_Release( &impl->IActivationFactory_iface );
}

static HRESULT WINAPI statics_GetRuntimeClassName( IGamepadStatics *iface, HSTRING *class_name )
{
    struct gamepad_statics *impl = impl_from_IGamepadStatics( iface );
    return IActivationFactory_Release( &impl->IActivationFactory_iface );
}

static HRESULT WINAPI statics_GetTrustLevel( IGamepadStatics *iface, TrustLevel *trust_level )
{
    struct gamepad_statics *impl = impl_from_IGamepadStatics( iface );
    return IActivationFactory_Release( &impl->IActivationFactory_iface );
}

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
    TRACE( "iface %p, value %p.\n", iface, value );
    *value = &gamepads.IVectorView_Gamepad_iface;
    IVectorView_Gamepad_AddRef( *value );
    return S_OK;
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

static struct gamepad_statics gamepad_statics =
{
    {&factory_vtbl},
    {&statics_vtbl},
    1,
};

IActivationFactory *gamepad_factory = &gamepad_statics.IActivationFactory_iface;
