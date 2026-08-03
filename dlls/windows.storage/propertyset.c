/* WinRT Windows.Storage.ApplicationData IPropertySet Implementation
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
#include "wine/debug.h"
#include "pathcch.h"

WINE_DEFAULT_DEBUG_CHANNEL(data);

struct propertyset_impl
{
    IPropertySet IPropertySet_iface;
    IObservableMap_HSTRING_IInspectable IObservableMap_iface;
    IMap_HSTRING_IInspectable IMap_iface;
    IIterable_IKeyValuePair_HSTRING_IInspectable IIterable_iface;
    LONG ref;
};

static inline struct propertyset_impl *impl_from_IPropertySet( IPropertySet *iface )
{
    return CONTAINING_RECORD( iface, struct propertyset_impl, IPropertySet_iface );
}

static HRESULT WINAPI propertyset_QueryInterface( IPropertySet *iface, REFIID iid, void **out )
{
    struct propertyset_impl *impl = impl_from_IPropertySet( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IPropertySet ))
    {
        *out = &impl->IPropertySet_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if ( IsEqualGUID( iid, &IID_IObservableMap_HSTRING_IInspectable) ) {
        *out = &impl->IObservableMap_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if ( IsEqualGUID( iid, &IID_IMap_HSTRING_IInspectable) ) {
        *out = &impl->IMap_iface;
        IInspectable_AddRef( *out );    
        return S_OK;
    }

    if ( IsEqualGUID( iid, &IID_IIterable_IKeyValuePair_HSTRING_IInspectable) ) {
        *out = &impl->IIterable_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI propertyset_AddRef( IPropertySet *iface )
{
    struct propertyset_impl *impl = impl_from_IPropertySet( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI propertyset_Release( IPropertySet *iface )
{
    struct propertyset_impl *impl = impl_from_IPropertySet( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI propertyset_GetIids( IPropertySet *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI propertyset_GetRuntimeClassName( IPropertySet *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI propertyset_GetTrustLevel( IPropertySet *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static const struct IPropertySetVtbl propertyset_vtbl =
{
    propertyset_QueryInterface,
    propertyset_AddRef,
    propertyset_Release,
    /* IInspectable methods */
    propertyset_GetIids,
    propertyset_GetRuntimeClassName,
    propertyset_GetTrustLevel,
};

static inline struct propertyset_impl *impl_from_IObservableMap( IObservableMap_HSTRING_IInspectable *iface )
{
    return CONTAINING_RECORD( iface, struct propertyset_impl, IObservableMap_iface );
}

static HRESULT WINAPI observable_QueryInterface( IObservableMap_HSTRING_IInspectable *iface, REFIID iid, void **out )
{
    struct propertyset_impl *impl = impl_from_IObservableMap( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IObservableMap_HSTRING_IInspectable ))
    {
        *out = &impl->IObservableMap_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI observable_AddRef( IObservableMap_HSTRING_IInspectable *iface )
{
    struct propertyset_impl *impl = impl_from_IObservableMap( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI observable_Release( IObservableMap_HSTRING_IInspectable *iface )
{
    struct propertyset_impl *impl = impl_from_IObservableMap( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI observable_GetIids( IObservableMap_HSTRING_IInspectable *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI observable_GetRuntimeClassName( IObservableMap_HSTRING_IInspectable *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI observable_GetTrustLevel( IObservableMap_HSTRING_IInspectable *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI observable_add_MapChanged( IObservableMap_HSTRING_IInspectable *iface, IMapChangedEventHandler_HSTRING_IInspectable *handler, EventRegistrationToken *token )
{
    FIXME( "iface %p, handler %p token %p stub!\n", iface, handler, token);
    return S_OK;
}

static HRESULT WINAPI observable_remove_MapChanged( IObservableMap_HSTRING_IInspectable *iface, EventRegistrationToken token )
{
    FIXME( "iface %p, token %#I64x stub!\n", iface, token.value);
    return S_OK;
}

static const struct IObservableMap_HSTRING_IInspectableVtbl observable_vtbl =
{
    observable_QueryInterface,
    observable_AddRef,
    observable_Release,
    /* IInspectable methods */
    observable_GetIids,
    observable_GetRuntimeClassName,
    observable_GetTrustLevel,
    /* IObservableMap methods */
    observable_add_MapChanged,
    observable_remove_MapChanged,
};

static inline struct propertyset_impl *impl_from_IMap( IMap_HSTRING_IInspectable *iface )
{
    return CONTAINING_RECORD( iface, struct propertyset_impl, IObservableMap_iface );
}

static HRESULT WINAPI map_QueryInterface( IMap_HSTRING_IInspectable *iface, REFIID iid, void **out )
{
    struct propertyset_impl *impl = impl_from_IMap( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IMap_HSTRING_IInspectable ))
    {
        *out = &impl->IMap_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IIterable_IKeyValuePair_HSTRING_IInspectable ))
    {
        *out = &impl->IIterable_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI map_AddRef( IMap_HSTRING_IInspectable *iface )
{
    struct propertyset_impl *impl = impl_from_IMap( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI map_Release( IMap_HSTRING_IInspectable *iface )
{
    struct propertyset_impl *impl = impl_from_IMap( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI map_GetIids( IMap_HSTRING_IInspectable *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI map_GetRuntimeClassName( IMap_HSTRING_IInspectable *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI map_GetTrustLevel( IMap_HSTRING_IInspectable *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI map_Lookup( IMap_HSTRING_IInspectable *iface, HSTRING key, IInspectable **value )
{
    FIXME( "iface %p, key %s, value %p stub!\n", iface, debugstr_hstring(key), value);
    return E_NOTIMPL;
}

static HRESULT WINAPI map_get_Size( IMap_HSTRING_IInspectable *iface, unsigned int *size )
{
    FIXME( "iface %p, size %p stub!\n", iface, size);
    *size = 0;
    return S_OK;
}

static HRESULT WINAPI map_HasKey( IMap_HSTRING_IInspectable *iface, HSTRING key, boolean *found )
{
    FIXME( "iface %p, key %s, found %p stub!\n", iface, debugstr_hstring(key), found);
    *found = FALSE;
    return S_OK;
}

static HRESULT WINAPI map_GetView( IMap_HSTRING_IInspectable *iface, IMapView_HSTRING_IInspectable **view )
{
    FIXME( "iface %p, view %p stub!\n", iface, view);
    return E_NOTIMPL;
}

static HRESULT WINAPI map_Insert( IMap_HSTRING_IInspectable *iface, HSTRING key, IInspectable *value, boolean *replaced )
{
    FIXME( "iface %p, key %s, value %p, replaced %p stub!\n", iface, debugstr_hstring(key), value, replaced);
    *replaced = FALSE;
    return S_OK;
}

static HRESULT WINAPI map_Remove( IMap_HSTRING_IInspectable *iface, HSTRING key )
{
    FIXME( "iface %p, key %s stub!\n", iface, debugstr_hstring(key));
    return S_OK;
}

static HRESULT WINAPI map_Clear( IMap_HSTRING_IInspectable *iface )
{
    FIXME( "iface %p, stub!\n", iface);
    return S_OK;
}


static const struct IMap_HSTRING_IInspectableVtbl map_vtbl =
{
    map_QueryInterface,
    map_AddRef,
    map_Release,
    /* IInspectable methods */
    map_GetIids,
    map_GetRuntimeClassName,
    map_GetTrustLevel,
    /* IMap methods */
    map_Lookup,
    map_get_Size,
    map_HasKey,
    map_GetView,
    map_Insert,
    map_Remove,
    map_Clear,
};

static inline struct propertyset_impl *impl_from_IIterable( IIterable_IKeyValuePair_HSTRING_IInspectable *iface )
{
    return CONTAINING_RECORD( iface, struct propertyset_impl, IIterable_iface );
}

static HRESULT WINAPI iterable_QueryInterface( IIterable_IKeyValuePair_HSTRING_IInspectable *iface, REFIID iid, void **out )
{
    struct propertyset_impl *impl = impl_from_IIterable( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IIterable_IKeyValuePair_HSTRING_IInspectable ))
    {
        *out = &impl->IIterable_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI iterable_AddRef( IIterable_IKeyValuePair_HSTRING_IInspectable *iface )
{
    struct propertyset_impl *impl = impl_from_IIterable( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI iterable_Release( IIterable_IKeyValuePair_HSTRING_IInspectable *iface )
{
    struct propertyset_impl *impl = impl_from_IIterable( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI iterable_GetIids( IIterable_IKeyValuePair_HSTRING_IInspectable *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI iterable_GetRuntimeClassName( IIterable_IKeyValuePair_HSTRING_IInspectable *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI iterable_GetTrustLevel( IIterable_IKeyValuePair_HSTRING_IInspectable *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI iterable_First( IIterable_IKeyValuePair_HSTRING_IInspectable *iface, IIterator_IKeyValuePair_HSTRING_IInspectable **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}


static const struct IIterable_IKeyValuePair_HSTRING_IInspectableVtbl iterable_vtbl =
{
    iterable_QueryInterface,
    iterable_AddRef,
    iterable_Release,
    /* IInspectable methods */
    iterable_GetIids,
    iterable_GetRuntimeClassName,
    iterable_GetTrustLevel,
    /* IIterable methods */
    iterable_First,
};

IPropertySet *create_propertyset(void) {
    struct propertyset_impl *object;

    if (!(object = calloc(1, sizeof(*object))))
        return NULL;

    object->IPropertySet_iface.lpVtbl = &propertyset_vtbl;
    object->IObservableMap_iface.lpVtbl = &observable_vtbl;
    object->IMap_iface.lpVtbl = &map_vtbl;
    object->IIterable_iface.lpVtbl = &iterable_vtbl;
    object->ref = 1;

    return &object->IPropertySet_iface;
}
