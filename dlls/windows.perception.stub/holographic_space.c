/* WinRT Windows.Perception.Stub HolographicSpace Implementation
 *
 * Copyright (C) 2023 Mohamad Al-Jaf
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

WINE_DEFAULT_DEBUG_CHANNEL(perception);

static EventRegistrationToken dummy_cookie = {.value = 0xdeadbeef};

struct holographic_space_statics
{
    IActivationFactory IActivationFactory_iface;
    IHolographicSpaceStatics2 IHolographicSpaceStatics2_iface;
    IHolographicSpaceStatics3 IHolographicSpaceStatics3_iface;
    IHolographicSpaceInterop IHolographicSpaceInterop_iface;
    LONG ref;
};

static inline struct holographic_space_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct holographic_space_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct holographic_space_statics *impl = impl_from_IActivationFactory( iface );

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

    if (IsEqualGUID( iid, &IID_IHolographicSpaceStatics2 ))
    {
        *out = &impl->IHolographicSpaceStatics2_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IHolographicSpaceStatics3 ))
    {
        *out = &impl->IHolographicSpaceStatics3_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IHolographicSpaceInterop ))
    {
        *out = &impl->IHolographicSpaceInterop_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct holographic_space_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct holographic_space_statics *impl = impl_from_IActivationFactory( iface );
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

DEFINE_IINSPECTABLE( holographic_space_statics2, IHolographicSpaceStatics2, struct holographic_space_statics, IActivationFactory_iface )

static HRESULT WINAPI holographic_space_statics2_get_IsSupported( IHolographicSpaceStatics2 *iface, boolean *value )
{
    TRACE( "iface %p, value %p\n", iface, value );

    *value = FALSE;
    return S_OK;
}

static HRESULT WINAPI holographic_space_statics2_get_IsAvailable( IHolographicSpaceStatics2 *iface, boolean *value )
{
    TRACE( "iface %p, value %p\n", iface, value );

    *value = FALSE;
    return S_OK;
}

static HRESULT WINAPI holographic_space_statics2_add_IsAvailableChanged( IHolographicSpaceStatics2 *iface, IEventHandler_IInspectable *handler,
                                                                         EventRegistrationToken *token )
{
    FIXME( "iface %p, handler %p, token %p stub!\n", iface, handler, token );
    return E_NOTIMPL;
}

static HRESULT WINAPI holographic_space_statics2_remove_IsAvailableChanged( IHolographicSpaceStatics2 *iface, EventRegistrationToken token )
{
    FIXME( "iface %p, token %#I64x stub!\n", iface, token.value );
    return E_NOTIMPL;
}

static const struct IHolographicSpaceStatics2Vtbl holographic_space_statics2_vtbl =
{
    holographic_space_statics2_QueryInterface,
    holographic_space_statics2_AddRef,
    holographic_space_statics2_Release,
    /* IInspectable methods */
    holographic_space_statics2_GetIids,
    holographic_space_statics2_GetRuntimeClassName,
    holographic_space_statics2_GetTrustLevel,
    /* IHolographicSpaceStatics2 methods */
    holographic_space_statics2_get_IsSupported,
    holographic_space_statics2_get_IsAvailable,
    holographic_space_statics2_add_IsAvailableChanged,
    holographic_space_statics2_remove_IsAvailableChanged,
};

DEFINE_IINSPECTABLE( holographic_space_statics3, IHolographicSpaceStatics3, struct holographic_space_statics, IActivationFactory_iface )

static HRESULT WINAPI holographic_space_statics3_get_IsConfigured( IHolographicSpaceStatics3 *iface, boolean *value )
{
    TRACE( "iface %p, value %p\n", iface, value );

    *value = FALSE;
    return S_OK;
}

static const struct IHolographicSpaceStatics3Vtbl holographic_space_statics3_vtbl =
{
    holographic_space_statics3_QueryInterface,
    holographic_space_statics3_AddRef,
    holographic_space_statics3_Release,
    /* IInspectable methods */
    holographic_space_statics3_GetIids,
    holographic_space_statics3_GetRuntimeClassName,
    holographic_space_statics3_GetTrustLevel,
    /* IHolographicSpaceStatics3 methods */
    holographic_space_statics3_get_IsConfigured,
};

struct holographic_space
{
    IHolographicSpace IHolographicSpace_iface;
    LONG ref;
};

static inline struct holographic_space *impl_from_IHolographicSpace( IHolographicSpace *iface )
{
    return CONTAINING_RECORD( iface, struct holographic_space, IHolographicSpace_iface );
}

static HRESULT WINAPI holographic_space_QueryInterface( IHolographicSpace *iface, REFIID iid, void **out )
{
    struct holographic_space *impl = impl_from_IHolographicSpace( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IHolographicSpace ))
    {
        *out = &impl->IHolographicSpace_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI holographic_space_AddRef( IHolographicSpace *iface )
{
    struct holographic_space *impl = impl_from_IHolographicSpace( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI holographic_space_Release( IHolographicSpace *iface )
{
    struct holographic_space *impl = impl_from_IHolographicSpace( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI holographic_space_GetIids( IHolographicSpace *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI holographic_space_GetRuntimeClassName( IHolographicSpace *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI holographic_space_GetTrustLevel( IHolographicSpace *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI holographic_space_get_PrimaryAdapterId( IHolographicSpace *iface, HolographicAdapterId *value )
{
    TRACE( "iface %p, value %p\n", iface, value );

    value->LowPart = 0;
    value->HighPart = 0;

    return S_OK;
}

static HRESULT WINAPI holographic_space_SetDirect3D11Device( IHolographicSpace *iface, IDirect3DDevice *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );

    if (!value) return E_INVALIDARG;
    return S_OK;
}

static HRESULT WINAPI holographic_space_add_CameraAdded( IHolographicSpace *iface, ITypedEventHandler_HolographicSpace_HolographicSpaceCameraAddedEventArgs *handler,
                                                         EventRegistrationToken *cookie )
{
    FIXME( "iface %p, handler %p, cookie %p stub!\n", iface, handler, cookie );

    *cookie = dummy_cookie;
    return S_OK;
}

static HRESULT WINAPI holographic_space_remove_CameraAdded( IHolographicSpace *iface, EventRegistrationToken cookie )
{
    FIXME( "iface %p, cookie %#I64x stub!\n", iface, cookie.value );
    return S_OK;
}

static HRESULT WINAPI holographic_space_add_CameraRemoved( IHolographicSpace *iface, ITypedEventHandler_HolographicSpace_HolographicSpaceCameraRemovedEventArgs *handler,
                                                           EventRegistrationToken *cookie )
{
    FIXME( "iface %p, handler %p, cookie %p stub!\n", iface, handler, cookie );

    *cookie = dummy_cookie;
    return S_OK;
}

static HRESULT WINAPI holographic_space_remove_CameraRemoved( IHolographicSpace *iface, EventRegistrationToken cookie )
{
    FIXME( "iface %p, cookie %#I64x stub!\n", iface, cookie.value );
    return S_OK;
}

static HRESULT WINAPI holographic_space_CreateNextFrame( IHolographicSpace *iface, IHolographicFrame **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static const struct IHolographicSpaceVtbl holographic_space_vtbl =
{
    /* IUnknown methods */
    holographic_space_QueryInterface,
    holographic_space_AddRef,
    holographic_space_Release,
    /* IInspectable methods */
    holographic_space_GetIids,
    holographic_space_GetRuntimeClassName,
    holographic_space_GetTrustLevel,
    /* IHolographicSpace methods */
    holographic_space_get_PrimaryAdapterId,
    holographic_space_SetDirect3D11Device,
    holographic_space_add_CameraAdded,
    holographic_space_remove_CameraAdded,
    holographic_space_add_CameraRemoved,
    holographic_space_remove_CameraRemoved,
    holographic_space_CreateNextFrame,
};

DEFINE_IINSPECTABLE( holographic_space_interop, IHolographicSpaceInterop, struct holographic_space_statics, IActivationFactory_iface )

static HRESULT WINAPI holographic_space_interop_CreateForWindow( IHolographicSpaceInterop *iface,
                                                                 HWND window, REFIID iid, void **holographic_space )
{
    struct holographic_space *impl;
    HRESULT hr;

    TRACE( "iface %p, window %p, iid %s, holographic_space %p.\n", iface, window, debugstr_guid( iid ), holographic_space );

    if (!IsWindow( window ) || IsWindowVisible( window ))
    {
        *holographic_space = NULL;
        return E_INVALIDARG;
    }
    if (!(impl = calloc( 1, sizeof( *impl ) ))) return E_OUTOFMEMORY;

    impl->IHolographicSpace_iface.lpVtbl = &holographic_space_vtbl;
    impl->ref = 1;

    hr = IHolographicSpace_QueryInterface( &impl->IHolographicSpace_iface, iid, holographic_space );
    IHolographicSpace_Release( &impl->IHolographicSpace_iface );
    TRACE( "created IHolographicSpace %p.\n", *holographic_space );
    return hr;
}

static const struct IHolographicSpaceInteropVtbl holographic_space_interop_vtbl =
{
    holographic_space_interop_QueryInterface,
    holographic_space_interop_AddRef,
    holographic_space_interop_Release,
    /* IInspectable methods */
    holographic_space_interop_GetIids,
    holographic_space_interop_GetRuntimeClassName,
    holographic_space_interop_GetTrustLevel,
    /* IHolographicSpaceInterop methods */
    holographic_space_interop_CreateForWindow,
};

static struct holographic_space_statics holographic_space_statics =
{
    {&factory_vtbl},
    {&holographic_space_statics2_vtbl},
    {&holographic_space_statics3_vtbl},
    {&holographic_space_interop_vtbl},
    1,
};

IActivationFactory *holographic_space_factory = &holographic_space_statics.IActivationFactory_iface;
