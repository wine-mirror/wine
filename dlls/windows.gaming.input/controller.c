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

struct controller_statics
{
    IActivationFactory IActivationFactory_iface;
    IRawGameControllerStatics IRawGameControllerStatics_iface;
    LONG ref;
};

static inline struct controller_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct controller_statics, IActivationFactory_iface );
}

static inline struct controller_statics *impl_from_IRawGameControllerStatics( IRawGameControllerStatics *iface )
{
    return CONTAINING_RECORD( iface, struct controller_statics, IRawGameControllerStatics_iface );
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
        IUnknown_AddRef( iface );
        *out = iface;
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IRawGameControllerStatics ))
    {
        IUnknown_AddRef( iface );
        *out = &impl->IRawGameControllerStatics_iface;
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

static HRESULT WINAPI statics_QueryInterface( IRawGameControllerStatics *iface, REFIID iid, void **out )
{
    struct controller_statics *impl = impl_from_IRawGameControllerStatics( iface );
    return IActivationFactory_QueryInterface( &impl->IActivationFactory_iface, iid, out );
}

static ULONG WINAPI statics_AddRef( IRawGameControllerStatics *iface )
{
    struct controller_statics *impl = impl_from_IRawGameControllerStatics( iface );
    return IActivationFactory_AddRef( &impl->IActivationFactory_iface );
}

static ULONG WINAPI statics_Release( IRawGameControllerStatics *iface )
{
    struct controller_statics *impl = impl_from_IRawGameControllerStatics( iface );
    return IActivationFactory_Release( &impl->IActivationFactory_iface );
}

static HRESULT WINAPI statics_GetIids( IRawGameControllerStatics *iface, ULONG *iid_count, IID **iids )
{
    struct controller_statics *impl = impl_from_IRawGameControllerStatics( iface );
    return IActivationFactory_Release( &impl->IActivationFactory_iface );
}

static HRESULT WINAPI statics_GetRuntimeClassName( IRawGameControllerStatics *iface, HSTRING *class_name )
{
    struct controller_statics *impl = impl_from_IRawGameControllerStatics( iface );
    return IActivationFactory_Release( &impl->IActivationFactory_iface );
}

static HRESULT WINAPI statics_GetTrustLevel( IRawGameControllerStatics *iface, TrustLevel *trust_level )
{
    struct controller_statics *impl = impl_from_IRawGameControllerStatics( iface );
    return IActivationFactory_Release( &impl->IActivationFactory_iface );
}

static HRESULT WINAPI statics_add_RawGameControllerAdded( IRawGameControllerStatics *iface, IEventHandler_RawGameController *value,
                                                          EventRegistrationToken *token )
{
    FIXME( "iface %p, value %p, token %p stub!\n", iface, value, token );
    if (!value) return E_INVALIDARG;
    token->value = 0;
    return S_OK;
}

static HRESULT WINAPI statics_remove_RawGameControllerAdded( IRawGameControllerStatics *iface, EventRegistrationToken token )
{
    FIXME( "iface %p, token %#I64x stub!\n", iface, token.value );
    return S_OK;
}

static HRESULT WINAPI statics_add_RawGameControllerRemoved( IRawGameControllerStatics *iface, IEventHandler_RawGameController *value,
                                                            EventRegistrationToken *token )
{
    FIXME( "iface %p, value %p, token %p stub!\n", iface, value, token );
    if (!value) return E_INVALIDARG;
    token->value = 0;
    return S_OK;
}

static HRESULT WINAPI statics_remove_RawGameControllerRemoved( IRawGameControllerStatics *iface, EventRegistrationToken token )
{
    FIXME( "iface %p, token %#I64x stub!\n", iface, token.value );
    return S_OK;
}

static HRESULT WINAPI statics_get_RawGameControllers( IRawGameControllerStatics *iface, IVectorView_RawGameController **value )
{
    static const GUID *view_iid = &IID_IVectorView_RawGameController;
    static const GUID *iid = &IID_IVector_RawGameController;
    IVector_RawGameController *controllers;
    HRESULT hr;

    TRACE( "iface %p, value %p.\n", iface, value );

    if (SUCCEEDED(hr = vector_create( iid, view_iid, (void **)&controllers )))
    {
        hr = IVector_RawGameController_GetView( controllers, value );
        IVector_RawGameController_Release( controllers );
    }

    return hr;
}

static HRESULT WINAPI statics_FromGameController( IRawGameControllerStatics *iface, IGameController *game_controller,
                                                  IRawGameController **value )
{
    FIXME( "iface %p, game_controller %p, value %p stub!\n", iface, game_controller, value );
    return E_NOTIMPL;
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

static struct controller_statics controller_statics =
{
    {&factory_vtbl},
    {&statics_vtbl},
    1,
};

IActivationFactory *controller_factory = &controller_statics.IActivationFactory_iface;
