/*
 * Copyright 2025 Rémi Bernon for CodeWeavers
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

#define COBJMACROS
#include <winstring.h>
#include <wine/debug.h>

#include "private.h"

WINE_DEFAULT_DEBUG_CHANNEL(combase);

struct pair
{
    IKeyValuePair_HSTRING_IInspectable IKeyValuePair_HSTRING_IInspectable_iface;
    const GUID *iid;
    LONG ref;
};

static struct pair *impl_from_IKeyValuePair_HSTRING_IInspectable( IKeyValuePair_HSTRING_IInspectable *iface )
{
    return CONTAINING_RECORD( iface, struct pair, IKeyValuePair_HSTRING_IInspectable_iface );
}

static HRESULT WINAPI pair_QueryInterface( IKeyValuePair_HSTRING_IInspectable *iface, REFIID iid, void **out )
{
    struct pair *impl = impl_from_IKeyValuePair_HSTRING_IInspectable( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, impl->iid ))
    {
        IInspectable_AddRef( (*out = &impl->IKeyValuePair_HSTRING_IInspectable_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI pair_AddRef( IKeyValuePair_HSTRING_IInspectable *iface )
{
    struct pair *impl = impl_from_IKeyValuePair_HSTRING_IInspectable( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI pair_Release( IKeyValuePair_HSTRING_IInspectable *iface )
{
    struct pair *impl = impl_from_IKeyValuePair_HSTRING_IInspectable( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref)
    {
        free( impl );
    }

    return ref;
}

static HRESULT WINAPI pair_GetIids( IKeyValuePair_HSTRING_IInspectable *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI pair_GetRuntimeClassName( IKeyValuePair_HSTRING_IInspectable *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI pair_GetTrustLevel( IKeyValuePair_HSTRING_IInspectable *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI pair_get_Key( IKeyValuePair_HSTRING_IInspectable *iface, HSTRING *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI pair_get_Value( IKeyValuePair_HSTRING_IInspectable *iface, IInspectable **value )
{
    TRACE( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static const IKeyValuePair_HSTRING_IInspectableVtbl pair_vtbl =
{
    pair_QueryInterface,
    pair_AddRef,
    pair_Release,
    /* IInspectable methods */
    pair_GetIids,
    pair_GetRuntimeClassName,
    pair_GetTrustLevel,
    /* IKeyValuePair<HSTRING,IInspectable*> methods */
    pair_get_Key,
    pair_get_Value,
};

static HRESULT pair_create( const GUID *iid, IKeyValuePair_HSTRING_IInspectable **out )
{
    struct pair *pair;

    if (!(pair = calloc(1, sizeof(*pair)))) return E_OUTOFMEMORY;
    pair->IKeyValuePair_HSTRING_IInspectable_iface.lpVtbl = &pair_vtbl;
    pair->iid = iid;
    pair->ref = 1;

    *out = &pair->IKeyValuePair_HSTRING_IInspectable_iface;
    return S_OK;
}

struct map
{
    IInspectable IInspectable_iface;
    IMap_HSTRING_IInspectable IMap_HSTRING_IInspectable_iface;
    IIterable_IKeyValuePair_HSTRING_IInspectable IIterable_IKeyValuePair_HSTRING_IInspectable_iface;
    IInspectable *IInspectable_outer;
    struct map_iids iids;
    LONG ref;
};

struct map_view
{
    IMapView_HSTRING_IInspectable IMapView_HSTRING_IInspectable_iface;
    IIterable_IKeyValuePair_HSTRING_IInspectable IIterable_IKeyValuePair_HSTRING_IInspectable_iface;
    LONG ref;

    struct map *map;
};

struct iterator
{
    IIterator_IKeyValuePair_HSTRING_IInspectable IIterator_IKeyValuePair_HSTRING_IInspectable_iface;
    LONG ref;

    struct map_view *view;
};

static struct iterator *impl_from_IIterator_IKeyValuePair_HSTRING_IInspectable( IIterator_IKeyValuePair_HSTRING_IInspectable *iface )
{
    return CONTAINING_RECORD( iface, struct iterator, IIterator_IKeyValuePair_HSTRING_IInspectable_iface );
}

static HRESULT WINAPI iterator_QueryInterface( IIterator_IKeyValuePair_HSTRING_IInspectable *iface, REFIID iid, void **out )
{
    struct iterator *impl = impl_from_IIterator_IKeyValuePair_HSTRING_IInspectable( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, impl->view->map->iids.iterator ))
    {
        IInspectable_AddRef( (*out = &impl->IIterator_IKeyValuePair_HSTRING_IInspectable_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI iterator_AddRef( IIterator_IKeyValuePair_HSTRING_IInspectable *iface )
{
    struct iterator *impl = impl_from_IIterator_IKeyValuePair_HSTRING_IInspectable( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI iterator_Release( IIterator_IKeyValuePair_HSTRING_IInspectable *iface )
{
    struct iterator *impl = impl_from_IIterator_IKeyValuePair_HSTRING_IInspectable( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref)
    {
        IMapView_HSTRING_IInspectable_Release( &impl->view->IMapView_HSTRING_IInspectable_iface );
        free( impl );
    }

    return ref;
}

static HRESULT WINAPI iterator_GetIids( IIterator_IKeyValuePair_HSTRING_IInspectable *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI iterator_GetRuntimeClassName( IIterator_IKeyValuePair_HSTRING_IInspectable *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI iterator_GetTrustLevel( IIterator_IKeyValuePair_HSTRING_IInspectable *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI iterator_get_Current( IIterator_IKeyValuePair_HSTRING_IInspectable *iface, IKeyValuePair_HSTRING_IInspectable **value )
{
    struct iterator *impl = impl_from_IIterator_IKeyValuePair_HSTRING_IInspectable( iface );
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return pair_create( impl->view->map->iids.pair, value );
}

static HRESULT WINAPI iterator_get_HasCurrent( IIterator_IKeyValuePair_HSTRING_IInspectable *iface, boolean *value )
{
    TRACE( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI iterator_MoveNext( IIterator_IKeyValuePair_HSTRING_IInspectable *iface, boolean *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI iterator_GetMany( IIterator_IKeyValuePair_HSTRING_IInspectable *iface, UINT32 items_size,
                                        IKeyValuePair_HSTRING_IInspectable **items, UINT *count )
{
    FIXME( "iface %p, items_size %u, items %p, count %p stub!\n", iface, items_size, items, count );
    return E_NOTIMPL;
}

static const IIterator_IKeyValuePair_HSTRING_IInspectableVtbl iterator_vtbl =
{
    iterator_QueryInterface,
    iterator_AddRef,
    iterator_Release,
    /* IInspectable methods */
    iterator_GetIids,
    iterator_GetRuntimeClassName,
    iterator_GetTrustLevel,
    /* IIterator<IInspectable*> methods */
    iterator_get_Current,
    iterator_get_HasCurrent,
    iterator_MoveNext,
    iterator_GetMany,
};

static struct map_view *impl_from_IMapView_HSTRING_IInspectable( IMapView_HSTRING_IInspectable *iface )
{
    return CONTAINING_RECORD( iface, struct map_view, IMapView_HSTRING_IInspectable_iface );
}

static HRESULT WINAPI map_view_QueryInterface( IMapView_HSTRING_IInspectable *iface, REFIID iid, void **out )
{
    struct map_view *impl = impl_from_IMapView_HSTRING_IInspectable( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, impl->map->iids.view ))
    {
        IInspectable_AddRef( (*out = &impl->IMapView_HSTRING_IInspectable_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, impl->map->iids.iterable ))
    {
        IInspectable_AddRef( (*out = &impl->IIterable_IKeyValuePair_HSTRING_IInspectable_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI map_view_AddRef( IMapView_HSTRING_IInspectable *iface )
{
    struct map_view *impl = impl_from_IMapView_HSTRING_IInspectable( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI map_view_Release( IMapView_HSTRING_IInspectable *iface )
{
    struct map_view *impl = impl_from_IMapView_HSTRING_IInspectable( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref)
    {
        IMap_HSTRING_IInspectable_Release( &impl->map->IMap_HSTRING_IInspectable_iface );
        free( impl );
    }
    return ref;
}

static HRESULT WINAPI map_view_GetIids( IMapView_HSTRING_IInspectable *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI map_view_GetRuntimeClassName( IMapView_HSTRING_IInspectable *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI map_view_GetTrustLevel( IMapView_HSTRING_IInspectable *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI map_view_Lookup( IMapView_HSTRING_IInspectable *iface, HSTRING key, IInspectable **value )
{
    FIXME( "iface %p, key %s, value %p stub!\n", iface, debugstr_hstring( key ), value );
    return E_NOTIMPL;
}

static HRESULT WINAPI map_view_get_Size( IMapView_HSTRING_IInspectable *iface, UINT32 *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI map_view_HasKey( IMapView_HSTRING_IInspectable *iface, HSTRING key, boolean *found )
{
    FIXME( "iface %p, key %s, found %p stub!\n", iface, debugstr_hstring( key ), found );
    return E_NOTIMPL;
}

static HRESULT WINAPI map_view_Split( IMapView_HSTRING_IInspectable *iface, IMapView_HSTRING_IInspectable **first,
                                      IMapView_HSTRING_IInspectable **second )
{
    FIXME( "iface %p, first %p, second %p stub!\n", iface, first, second );
    return E_NOTIMPL;
}

static const struct IMapView_HSTRING_IInspectableVtbl map_view_vtbl =
{
    map_view_QueryInterface,
    map_view_AddRef,
    map_view_Release,
    /* IInspectable methods */
    map_view_GetIids,
    map_view_GetRuntimeClassName,
    map_view_GetTrustLevel,
    /* IMapView_HSTRING<IInspectable*> methods */
    map_view_Lookup,
    map_view_get_Size,
    map_view_HasKey,
    map_view_Split,
};

DEFINE_IINSPECTABLE_( iterable_view, IIterable_IKeyValuePair_HSTRING_IInspectable, struct map_view, view_impl_from_IIterable_IKeyValuePair_HSTRING_IInspectable,
                      IIterable_IKeyValuePair_HSTRING_IInspectable_iface, &impl->IMapView_HSTRING_IInspectable_iface )

static HRESULT WINAPI iterable_view_First( IIterable_IKeyValuePair_HSTRING_IInspectable *iface, IIterator_IKeyValuePair_HSTRING_IInspectable **value )
{
    struct map_view *impl = view_impl_from_IIterable_IKeyValuePair_HSTRING_IInspectable( iface );
    struct iterator *iter;

    TRACE( "iface %p, value %p.\n", iface, value );

    if (!(iter = calloc( 1, sizeof(struct iterator) ))) return E_OUTOFMEMORY;
    iter->IIterator_IKeyValuePair_HSTRING_IInspectable_iface.lpVtbl = &iterator_vtbl;
    iter->ref = 1;

    IIterable_IKeyValuePair_HSTRING_IInspectable_AddRef( &impl->IIterable_IKeyValuePair_HSTRING_IInspectable_iface );
    iter->view = impl;

    *value = &iter->IIterator_IKeyValuePair_HSTRING_IInspectable_iface;
    return S_OK;
}

static const struct IIterable_IKeyValuePair_HSTRING_IInspectableVtbl iterable_view_vtbl =
{
    iterable_view_QueryInterface,
    iterable_view_AddRef,
    iterable_view_Release,
    /* IInspectable methods */
    iterable_view_GetIids,
    iterable_view_GetRuntimeClassName,
    iterable_view_GetTrustLevel,
    /* IIterable<T> methods */
    iterable_view_First,
};

static HRESULT map_view_create( struct map *map, IMapView_HSTRING_IInspectable **out )
{
    struct map_view *view;

    if (!(view = calloc( 1, sizeof(*view) ))) return E_OUTOFMEMORY;
    view->IMapView_HSTRING_IInspectable_iface.lpVtbl = &map_view_vtbl;
    view->IIterable_IKeyValuePair_HSTRING_IInspectable_iface.lpVtbl = &iterable_view_vtbl;
    view->ref = 1;

    IMap_HSTRING_IInspectable_AddRef( &map->IMap_HSTRING_IInspectable_iface );
    view->map = map;

    *out = &view->IMapView_HSTRING_IInspectable_iface;
    return S_OK;
}

static struct map *impl_from_IInspectable( IInspectable *iface )
{
    return CONTAINING_RECORD( iface, struct map, IInspectable_iface );
}

static HRESULT WINAPI map_inner_QueryInterface( IInspectable *iface, REFIID iid, void **out )
{
    struct map *impl = impl_from_IInspectable( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ))
    {
        IInspectable_AddRef( (*out = &impl->IInspectable_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, impl->iids.map ))
    {
        IInspectable_AddRef( (*out = &impl->IMap_HSTRING_IInspectable_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, impl->iids.iterable ))
    {
        IInspectable_AddRef( (*out = &impl->IIterable_IKeyValuePair_HSTRING_IInspectable_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI map_inner_AddRef( IInspectable *iface )
{
    struct map *impl = impl_from_IInspectable( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI map_inner_Release( IInspectable *iface )
{
    struct map *impl = impl_from_IInspectable( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref)
    {
        IMap_HSTRING_IInspectable_Clear( &impl->IMap_HSTRING_IInspectable_iface );
        free( impl );
    }

    return ref;
}

static HRESULT WINAPI map_inner_GetIids( IInspectable *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI map_inner_GetRuntimeClassName( IInspectable *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI map_inner_GetTrustLevel( IInspectable *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static const IInspectableVtbl map_inner_vtbl =
{
    /* IUnknown */
    map_inner_QueryInterface,
    map_inner_AddRef,
    map_inner_Release,
    /* IInspectable */
    map_inner_GetIids,
    map_inner_GetRuntimeClassName,
    map_inner_GetTrustLevel,
};

DEFINE_IINSPECTABLE_OUTER( map, IMap_HSTRING_IInspectable, struct map, IInspectable_outer )

static HRESULT WINAPI map_Lookup( IMap_HSTRING_IInspectable *iface, HSTRING key, IInspectable **value )
{
    FIXME( "iface %p, key %s, value %p stub!\n", iface, debugstr_hstring( key ), value );
    return E_NOTIMPL;
}

static HRESULT WINAPI map_get_Size( IMap_HSTRING_IInspectable *iface, UINT32 *size )
{
    FIXME( "iface %p, size %p stub!\n", iface, size );
    return E_NOTIMPL;
}

static HRESULT WINAPI map_HasKey( IMap_HSTRING_IInspectable *iface, HSTRING key, boolean *exists )
{
    FIXME( "iface %p, key %s, exists %p stub!\n", iface, debugstr_hstring( key ), exists );
    return E_NOTIMPL;
}

static HRESULT WINAPI map_GetView( IMap_HSTRING_IInspectable *iface, IMapView_HSTRING_IInspectable **value )
{
    struct map *impl = impl_from_IMap_HSTRING_IInspectable( iface );

    TRACE( "iface %p, value %p.\n", iface, value );

    return map_view_create( impl, value );
}

static HRESULT WINAPI map_Insert( IMap_HSTRING_IInspectable *iface, HSTRING key, IInspectable *value, boolean *replaced )
{
    FIXME( "iface %p, key %p, value %p, replaced %p stub!\n", iface, key, value, replaced );
    return E_NOTIMPL;
}

static HRESULT WINAPI map_Remove( IMap_HSTRING_IInspectable *iface, HSTRING key )
{
    FIXME( "iface %p, key %s stub!\n", iface, debugstr_hstring( key ) );
    return E_NOTIMPL;
}

static HRESULT WINAPI map_Clear( IMap_HSTRING_IInspectable *iface )
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static const IMap_HSTRING_IInspectableVtbl map_vtbl =
{
    /* IUnknown */
    map_QueryInterface,
    map_AddRef,
    map_Release,
    /* IInspectable */
    map_GetIids,
    map_GetRuntimeClassName,
    map_GetTrustLevel,
    /* IMap<HSTRING, IInspectable *> */
    map_Lookup,
    map_get_Size,
    map_HasKey,
    map_GetView,
    map_Insert,
    map_Remove,
    map_Clear,
};

DEFINE_IINSPECTABLE_OUTER( iterable, IIterable_IKeyValuePair_HSTRING_IInspectable, struct map, IInspectable_outer )

static HRESULT WINAPI iterable_First( IIterable_IKeyValuePair_HSTRING_IInspectable *iface, IIterator_IKeyValuePair_HSTRING_IInspectable **value )
{
    struct map *impl = impl_from_IIterable_IKeyValuePair_HSTRING_IInspectable( iface );
    IIterable_IKeyValuePair_HSTRING_IInspectable *iterable;
    IMapView_HSTRING_IInspectable *view;
    HRESULT hr;

    TRACE( "iface %p, value %p.\n", iface, value );

    if (FAILED(hr = IMap_HSTRING_IInspectable_GetView( &impl->IMap_HSTRING_IInspectable_iface, &view ))) return hr;

    hr = IMapView_HSTRING_IInspectable_QueryInterface( view, impl->iids.iterable, (void **)&iterable );
    IMapView_HSTRING_IInspectable_Release( view );
    if (FAILED(hr)) return hr;

    hr = IIterable_IKeyValuePair_HSTRING_IInspectable_First( iterable, value );
    IIterable_IKeyValuePair_HSTRING_IInspectable_Release( iterable );
    return hr;
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
    /* IIterable<T> methods */
    iterable_First,
};

HRESULT map_create( const struct map_iids *iids, IInspectable *outer, IInspectable **out )
{
    struct map *impl;

    TRACE( "iid %s, out %p.\n", debugstr_guid( iids->map ), out );

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->IInspectable_iface.lpVtbl = &map_inner_vtbl;
    impl->IMap_HSTRING_IInspectable_iface.lpVtbl = &map_vtbl;
    impl->IIterable_IKeyValuePair_HSTRING_IInspectable_iface.lpVtbl = &iterable_vtbl;
    impl->IInspectable_outer = outer ? outer : (IInspectable *)&impl->IInspectable_iface;
    impl->iids = *iids;
    impl->ref = 1;

    *out = &impl->IInspectable_iface;
    TRACE( "created %p\n", *out );
    return S_OK;
}
