/* WinRT Windows.Gaming.UI.GameBar implementation
 *
 * Copyright 2022 Paul Gofman for CodeWeavers
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

#include "initguid.h"
#include "private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gamebar);

static EventRegistrationToken dummy_token = {.value = 0xdeadbeef};

struct gamebar_statics
{
    IActivationFactory IActivationFactory_iface;
    IGameBarStatics IGameBarStatics_iface;
    LONG ref;
};

static inline struct gamebar_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct gamebar_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct gamebar_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        *out = &impl->IActivationFactory_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IGameBarStatics ))
    {
        *out = &impl->IGameBarStatics_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct gamebar_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct gamebar_statics *impl = impl_from_IActivationFactory( iface );
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

DEFINE_IINSPECTABLE( statics, IGameBarStatics, struct gamebar_statics, IActivationFactory_iface )

static HRESULT WINAPI statics_add_VisibilityChanged( IGameBarStatics *iface,
                                                     IEventHandler_IInspectable *handler,
                                                     EventRegistrationToken *token )
{
    FIXME( "iface %p, handler %p, token %p stub.\n", iface, handler, token );
    *token = dummy_token;
    return S_OK;
}

static HRESULT WINAPI statics_remove_VisibilityChanged( IGameBarStatics *iface, EventRegistrationToken token )
{
    FIXME( "iface %p, token %#I64x stub.\n", iface, token.value );
    return S_OK;
}

static HRESULT WINAPI statics_add_IsInputRedirectedChanged( IGameBarStatics *iface,
                                                            IEventHandler_IInspectable *handler,
                                                            EventRegistrationToken *token )
{
    FIXME( "iface %p, handler %p, token %p stub.\n", iface, handler, token );
    *token = dummy_token;
    return S_OK;
}

static HRESULT WINAPI statics_remove_IsInputRedirectedChanged( IGameBarStatics *iface, EventRegistrationToken token )
{
    FIXME( "iface %p, token %#I64x stub.\n", iface, token.value );
    return S_OK;
}

static HRESULT WINAPI statics_get_Visible( IGameBarStatics *iface, BOOLEAN *value)
{
    TRACE( "iface %p, value %p.\n", iface, value );

    if (!value) return E_POINTER;
    *value = FALSE;
    return S_OK;
}

static HRESULT WINAPI statics_get_IsInputRedirected( IGameBarStatics *iface, BOOLEAN *value )
{
    TRACE( "iface %p, value %p.\n", iface, value );

    if (!value) return E_POINTER;
    *value = FALSE;
    return S_OK;
}

static const struct IGameBarStaticsVtbl statics_vtbl =
{
    statics_QueryInterface,
    statics_AddRef,
    statics_Release,
    /* IInspectable methods */
    statics_GetIids,
    statics_GetRuntimeClassName,
    statics_GetTrustLevel,
    /* IGameBarStatics methods */
    statics_add_VisibilityChanged,
    statics_remove_VisibilityChanged,
    statics_add_IsInputRedirectedChanged,
    statics_remove_IsInputRedirectedChanged,
    statics_get_Visible,
    statics_get_IsInputRedirected,
};

static struct gamebar_statics gamebar_statics =
{
    {&factory_vtbl},
    {&statics_vtbl},
    1,
};

static IActivationFactory *gamebar_factory = &gamebar_statics.IActivationFactory_iface;

HRESULT WINAPI DllGetClassObject( REFCLSID clsid, REFIID riid, void **out )
{
    FIXME("clsid %s, riid %s, out %p stub!\n", debugstr_guid(clsid), debugstr_guid(riid), out);

    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllGetActivationFactory( HSTRING class_str, IActivationFactory **factory )
{
    const WCHAR *buffer = WindowsGetStringRawBuffer( class_str, NULL );

    TRACE( "class %s, factory %p.\n", debugstr_w(buffer), factory );

    *factory = NULL;

    if (!wcscmp( buffer, RuntimeClass_Windows_Gaming_UI_GameBar ))
        IActivationFactory_QueryInterface( gamebar_factory, &IID_IActivationFactory, (void **)factory );

    if (*factory) return S_OK;
    return CLASS_E_CLASSNOTAVAILABLE;
}

BOOL WINAPI DllMain( HINSTANCE instance, DWORD reason, void *reserved )
{
    TRACE( "instance %p, reason %lu, reserved %p.\n", instance, reason, reserved );

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( instance );
        break;
    }
    return TRUE;
}
