/* WinRT Windows.UI.StartScreen.JumpList Implementation
 *
 * Copyright (C) 2024 Onni Kukkonen
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
#include "initguid.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ui);

struct jumplist_statics
{
    IActivationFactory IActivationFactory_iface;
    IJumpListStatics IJumpListStatics_iface;
    LONG ref;
};

static inline struct jumplist_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct jumplist_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct jumplist_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        *out = &impl->IActivationFactory_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }
    else if (IsEqualGUID( iid, &IID_IJumpListStatics ))
    {
        *out = &impl->IJumpListStatics_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct jumplist_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct jumplist_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
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

struct jumplist
{
    IJumpList IJumpList_iface;
    LONG ref;
};

static inline struct jumplist *impl_from_IJumpList( IJumpList *iface )
{
    return CONTAINING_RECORD( iface, struct jumplist, IJumpList_iface );
}

static HRESULT WINAPI jumplist_QueryInterface( IJumpList *iface, REFIID iid, void **out )
{
    struct jumplist *impl = impl_from_IJumpList( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    *out = NULL;

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IJumpList ))
    {
        *out = &impl->IJumpList_iface;
    }

    if (!*out)
    {
        FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
        return E_NOINTERFACE;
    }

    IUnknown_AddRef( (IUnknown *)*out );
    return S_OK;
}

static ULONG WINAPI jumplist_AddRef( IJumpList *iface )
{
    struct jumplist *impl = impl_from_IJumpList( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI jumplist_Release( IJumpList *iface )
{
    struct jumplist *impl = impl_from_IJumpList( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p, ref %lu.\n", iface, ref );

    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI jumplist_GetIids( IJumpList *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI jumplist_GetRuntimeClassName( IJumpList *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI jumplist_GetTrustLevel( IJumpList *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI jumplist_get_Items( IJumpList *iface, __FIVector_1_Windows__CUI__CStartScreen__CJumpListItem **value)
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI jumplist_get_SystemGroupKind( IJumpList *iface, JumpListSystemGroupKind *value)
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    *value = JumpListSystemGroupKind_None;
    return S_OK;
}

static HRESULT WINAPI jumplist_put_SystemGroupKind( IJumpList *iface, JumpListSystemGroupKind value)
{
    FIXME( "iface %p, value %d stub!\n", iface, value );
    return S_OK;
}

static HRESULT WINAPI jumplist_SaveAsync( IJumpList *iface, IAsyncAction **result)
{
    FIXME( "iface %p, result %p stub!\n", iface, result );
    return E_NOTIMPL;
}

static const struct IJumpListVtbl jumplist_vtbl =
{
    jumplist_QueryInterface,
    jumplist_AddRef,
    jumplist_Release,

    /* IInspectable methods */
    jumplist_GetIids,
    jumplist_GetRuntimeClassName,
    jumplist_GetTrustLevel,

    /* IJumpList methods */
    jumplist_get_Items,
    jumplist_get_SystemGroupKind,
    jumplist_put_SystemGroupKind,
    jumplist_SaveAsync,
};

static HRESULT create_stub_jumplist_async( IInspectable *invoker, IInspectable **result )
{
    struct jumplist *impl;

    TRACE( "invoker %p, result %p.\n", invoker, result );

    if (!(impl = calloc( 1, sizeof(*impl) )))
    {
        *result = NULL;
        return E_OUTOFMEMORY;
    }

    impl->IJumpList_iface.lpVtbl = &jumplist_vtbl;
    impl->ref = 1;

    *result = (IInspectable *)&impl->IJumpList_iface;
    return S_OK;
}

DEFINE_IINSPECTABLE( jumplistsstatics, IJumpListStatics, struct jumplist_statics, IActivationFactory_iface );

static HRESULT WINAPI jumplistsstatics_LoadCurrentAsync( IJumpListStatics *iface, IAsyncOperation_JumpList **result )
{
    FIXME( "iface %p, result %p semi-stub!\n", iface, result );
    return async_operation_inspectable_create(&IID_IAsyncOperation_JumpList, NULL, create_stub_jumplist_async, (IAsyncOperation_IInspectable **)result);
}

static HRESULT WINAPI jumplistsstatics_IsSupported( IJumpListStatics *iface, boolean *result )
{
    TRACE( "iface %p, result %p\n", iface, result );
    *result = FALSE;
    return S_OK;
}

static const struct IJumpListStaticsVtbl jumplistsstatics_vtbl =
{
    jumplistsstatics_QueryInterface,
    jumplistsstatics_AddRef,
    jumplistsstatics_Release,

    /* IInspectable methods */
    jumplistsstatics_GetIids,
    jumplistsstatics_GetRuntimeClassName,
    jumplistsstatics_GetTrustLevel,

    /* IJumpListStatics methods */
    jumplistsstatics_LoadCurrentAsync,
    jumplistsstatics_IsSupported,
};

static struct jumplist_statics jumplist_statics =
{
    {&factory_vtbl},
    {&jumplistsstatics_vtbl},
    1,
};

IActivationFactory *jumplist_factory = &jumplist_statics.IActivationFactory_iface;
