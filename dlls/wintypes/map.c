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

struct map_entry
{
    HSTRING key;
    IInspectable *value;
};

static HRESULT map_entry_init( HSTRING key, IInspectable *value, struct map_entry *entry )
{
    HRESULT hr;
    if (FAILED(hr = WindowsDuplicateString( key, &entry->key ))) return hr;
    IInspectable_AddRef( (entry->value = value) );
    return S_OK;
}

static void map_entry_copy( struct map_entry *dst, const struct map_entry *src )
{
    /* key has been duplicated already and further duplication cannot fail */
    WindowsDuplicateString( src->key, &dst->key );
    IInspectable_AddRef( (dst->value = src->value) );
}

static void map_entry_clear( struct map_entry *entry )
{
    WindowsDeleteString( entry->key );
    entry->key = NULL;
    IInspectable_AddRef( entry->value );
    entry->value = NULL;
}

/* returns the first entry which is not less than addr, or entries + size if there's none. */
static struct map_entry *map_entries_lower_bound( struct map_entry *entries, UINT32 size, HSTRING key, int *ret )
{
    struct map_entry *begin = entries, *end = begin + size, *mid;
    int order = -1;

    while (begin < end)
    {
        mid = begin + (end - begin) / 2;
        WindowsCompareStringOrdinal( mid->key, key, &order );
        if (order < 0) begin = mid + 1;
        else if (order > 0) end = mid;
        else begin = end = mid;
    }

    *ret = order;
    return begin;
}

struct pair
{
    IKeyValuePair_HSTRING_IInspectable IKeyValuePair_HSTRING_IInspectable_iface;
    const GUID *iid;
    LONG ref;

    struct map_entry entry;
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
        map_entry_clear( &impl->entry );
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
    struct pair *impl = impl_from_IKeyValuePair_HSTRING_IInspectable( iface );
    TRACE( "iface %p, value %p\n", iface, value );
    return WindowsDuplicateString( impl->entry.key, value );
}

static HRESULT WINAPI pair_get_Value( IKeyValuePair_HSTRING_IInspectable *iface, IInspectable **value )
{
    struct pair *impl = impl_from_IKeyValuePair_HSTRING_IInspectable( iface );
    TRACE( "iface %p, value %p\n", iface, value );
    IInspectable_AddRef( (*value = impl->entry.value) );
    return S_OK;
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

static HRESULT pair_create( const GUID *iid, struct map_entry *entry, IKeyValuePair_HSTRING_IInspectable **out )
{
    struct pair *pair;

    if (!(pair = calloc(1, sizeof(*pair)))) return E_OUTOFMEMORY;
    pair->IKeyValuePair_HSTRING_IInspectable_iface.lpVtbl = &pair_vtbl;
    pair->iid = iid;
    pair->ref = 1;
    map_entry_copy( &pair->entry, entry );

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

    UINT32 size;
    UINT32 capacity;
    struct map_entry *entries;
};

struct map_view
{
    IMapView_HSTRING_IInspectable IMapView_HSTRING_IInspectable_iface;
    IIterable_IKeyValuePair_HSTRING_IInspectable IIterable_IKeyValuePair_HSTRING_IInspectable_iface;
    LONG ref;

    struct map *map;
    UINT32 size;
    struct map_entry entries[1];
};

struct iterator
{
    IIterator_IKeyValuePair_HSTRING_IInspectable IIterator_IKeyValuePair_HSTRING_IInspectable_iface;
    LONG ref;

    struct map_view *view;
    UINT32 index;
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

    TRACE( "iface %p, value %p\n", iface, value );

    if (impl->index >= impl->view->size) return E_BOUNDS;
    return pair_create( impl->view->map->iids.pair, impl->view->entries + impl->index, value );
}

static HRESULT WINAPI iterator_get_HasCurrent( IIterator_IKeyValuePair_HSTRING_IInspectable *iface, boolean *value )
{
    struct iterator *impl = impl_from_IIterator_IKeyValuePair_HSTRING_IInspectable( iface );
    TRACE( "iface %p, value %p\n", iface, value );
    *value = impl->index < impl->view->size;
    return S_OK;
}

static HRESULT WINAPI iterator_MoveNext( IIterator_IKeyValuePair_HSTRING_IInspectable *iface, boolean *value )
{
    struct iterator *impl = impl_from_IIterator_IKeyValuePair_HSTRING_IInspectable( iface );
    TRACE( "iface %p, value %p\n", iface, value );
    if (impl->index < impl->view->size) impl->index++;
    return IIterator_IKeyValuePair_HSTRING_IInspectable_get_HasCurrent( iface, value );
}

static HRESULT WINAPI iterator_GetMany( IIterator_IKeyValuePair_HSTRING_IInspectable *iface, UINT32 items_size,
                                        IKeyValuePair_HSTRING_IInspectable **items, UINT *count )
{
    struct iterator *impl = impl_from_IIterator_IKeyValuePair_HSTRING_IInspectable( iface );
    HRESULT hr;
    int len;

    TRACE( "iface %p, items_size %u, items %p, count %p\n", iface, items_size, items, count );

    if ((len = impl->view->size - impl->index) < 0) return E_BOUNDS;
    for (UINT32 i = 0; i < len; ++i)
    {
        if (i >= items_size) break;
        if (FAILED(hr = pair_create( impl->view->map->iids.pair, impl->view->entries + impl->index + i, items + i )))
        {
            while (i) IKeyValuePair_HSTRING_IInspectable_Release( items[--i] );
            return hr;
        }
    }
    *count = len;
    return S_OK;
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
    struct map_view *impl = impl_from_IMapView_HSTRING_IInspectable( iface );
    struct map_entry *entry;
    int order;

    TRACE( "iface %p, key %s, value %p\n", iface, debugstr_hstring( key ), value );

    if (!(entry = map_entries_lower_bound( impl->entries, impl->size, key, &order )) || order) return E_BOUNDS;
    IInspectable_AddRef( (*value = entry->value ));
    return S_OK;
}

static HRESULT WINAPI map_view_get_Size( IMapView_HSTRING_IInspectable *iface, UINT32 *value )
{
    struct map_view *impl = impl_from_IMapView_HSTRING_IInspectable( iface );
    TRACE( "iface %p, value %p\n", iface, value );
    *value = impl->size;
    return S_OK;
}

static HRESULT WINAPI map_view_HasKey( IMapView_HSTRING_IInspectable *iface, HSTRING key, boolean *found )
{
    struct map_view *impl = impl_from_IMapView_HSTRING_IInspectable( iface );
    int order;

    TRACE( "iface %p, key %s, found %p\n", iface, debugstr_hstring( key ), found );

    map_entries_lower_bound( impl->entries, impl->size, key, &order );
    *found = !order;
    return S_OK;
}

static HRESULT map_view_create( struct map *map, UINT first, UINT count, IMapView_HSTRING_IInspectable **out );

static HRESULT WINAPI map_view_Split( IMapView_HSTRING_IInspectable *iface, IMapView_HSTRING_IInspectable **first,
                                      IMapView_HSTRING_IInspectable **second )
{
    struct map_view *impl = impl_from_IMapView_HSTRING_IInspectable( iface );
    HRESULT hr;

    TRACE( "iface %p, first %p, second %p\n", iface, first, second );

    if (FAILED(hr = map_view_create( impl->map, 0, impl->size / 2, first ))) return hr;
    if (FAILED(hr = map_view_create( impl->map, impl->size / 2, impl->size - impl->size / 2, second )))
        IMapView_HSTRING_IInspectable_Release( *first );

    return hr;
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

static HRESULT map_view_create( struct map *map, UINT first, UINT count, IMapView_HSTRING_IInspectable **out )
{
    struct map_view *view;

    if (!(view = calloc( 1, offsetof( struct map_view, entries[count] ) ))) return E_OUTOFMEMORY;
    view->IMapView_HSTRING_IInspectable_iface.lpVtbl = &map_view_vtbl;
    view->IIterable_IKeyValuePair_HSTRING_IInspectable_iface.lpVtbl = &iterable_view_vtbl;
    view->ref = 1;

    IMap_HSTRING_IInspectable_AddRef( &map->IMap_HSTRING_IInspectable_iface );
    view->map = map;

    for (UINT32 i = 0; i < count; ++i) map_entry_copy( view->entries + view->size++, map->entries + i );

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
    struct map *impl = impl_from_IMap_HSTRING_IInspectable( iface );
    struct map_entry *entry;
    int order;

    TRACE( "iface %p, key %s, value %p\n", iface, debugstr_hstring( key ), value );

    if (!(entry = map_entries_lower_bound( impl->entries, impl->size, key, &order )) || order) return E_BOUNDS;
    IInspectable_AddRef( (*value = entry->value ));
    return S_OK;
}

static HRESULT WINAPI map_get_Size( IMap_HSTRING_IInspectable *iface, UINT32 *size )
{
    struct map *impl = impl_from_IMap_HSTRING_IInspectable( iface );
    TRACE( "iface %p, size %p\n", iface, size );
    *size = impl->size;
    return S_OK;
}

static HRESULT WINAPI map_HasKey( IMap_HSTRING_IInspectable *iface, HSTRING key, boolean *exists )
{
    struct map *impl = impl_from_IMap_HSTRING_IInspectable( iface );
    int order;

    TRACE( "iface %p, key %s, exists %p\n", iface, debugstr_hstring( key ), exists );

    map_entries_lower_bound( impl->entries, impl->size, key, &order );
    *exists = !order;
    return S_OK;
}

static HRESULT WINAPI map_GetView( IMap_HSTRING_IInspectable *iface, IMapView_HSTRING_IInspectable **value )
{
    struct map *impl = impl_from_IMap_HSTRING_IInspectable( iface );

    TRACE( "iface %p, value %p.\n", iface, value );

    return map_view_create( impl, 0, impl->size, value );
}

static HRESULT WINAPI map_Insert( IMap_HSTRING_IInspectable *iface, HSTRING key, IInspectable *value, boolean *replaced )
{
    struct map *impl = impl_from_IMap_HSTRING_IInspectable( iface );
    struct map_entry *tmp = impl->entries, *entry, copy;
    HRESULT hr;
    int order;

    TRACE( "iface %p, key %p, value %p, replaced %p\n", iface, key, value, replaced );

    if (impl->size == impl->capacity)
    {
        impl->capacity = max( 32, impl->capacity * 3 / 2 );
        if (!(impl->entries = realloc( impl->entries, impl->capacity * sizeof(*impl->entries) )))
        {
            impl->entries = tmp;
            return E_OUTOFMEMORY;
        }
    }

    if (!(entry = map_entries_lower_bound( impl->entries, impl->size, key, &order ))) return E_OUTOFMEMORY;
    if (FAILED(hr = map_entry_init( key, value, &copy ))) return hr;

    if ((*replaced = !order)) map_entry_clear( entry );
    else memmove( entry + 1, entry, (impl->entries + impl->size++ - entry) * sizeof(*impl->entries) );
    *entry = copy;

    return S_OK;
}

static HRESULT WINAPI map_Remove( IMap_HSTRING_IInspectable *iface, HSTRING key )
{
    struct map *impl = impl_from_IMap_HSTRING_IInspectable( iface );
    struct map_entry *entry;
    int order;

    TRACE( "iface %p, key %s\n", iface, debugstr_hstring( key ) );

    if (!(entry = map_entries_lower_bound( impl->entries, impl->size, key, &order )) || order) return E_BOUNDS;
    map_entry_clear( entry );
    memmove( entry, entry + 1, (impl->entries + --impl->size - entry) * sizeof(*impl->entries) );

    return S_OK;
}

static HRESULT WINAPI map_Clear( IMap_HSTRING_IInspectable *iface )
{
    struct map *impl = impl_from_IMap_HSTRING_IInspectable( iface );
    struct map_entry *entries = impl->entries;
    UINT32 size = impl->size;

    TRACE( "iface %p\n", iface );

    impl->entries = NULL;
    impl->capacity = 0;
    impl->size = 0;

    while (size) map_entry_clear( entries + --size );
    free( entries );

    return S_OK;
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
