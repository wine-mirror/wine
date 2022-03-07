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

WINE_DEFAULT_DEBUG_CHANNEL(combase);

struct vector_view
{
    IVectorView_IInspectable IVectorView_IInspectable_iface;
    const GUID *iid;
    LONG ref;

    UINT32 size;
    IInspectable *elements[1];
};

static inline struct vector_view *impl_from_IVectorView_IInspectable( IVectorView_IInspectable *iface )
{
    return CONTAINING_RECORD( iface, struct vector_view, IVectorView_IInspectable_iface );
}

static HRESULT WINAPI vector_view_QueryInterface( IVectorView_IInspectable *iface, REFIID iid, void **out )
{
    struct vector_view *impl = impl_from_IVectorView_IInspectable( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, impl->iid ))
    {
        IUnknown_AddRef( iface );
        *out = iface;
        return S_OK;
    }

    WARN( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI vector_view_AddRef( IVectorView_IInspectable *iface )
{
    struct vector_view *impl = impl_from_IVectorView_IInspectable( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI vector_view_Release( IVectorView_IInspectable *iface )
{
    struct vector_view *impl = impl_from_IVectorView_IInspectable( iface );
    ULONG i, ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref)
    {
        for (i = 0; i < impl->size; ++i) IInspectable_Release( impl->elements[i] );
        free( impl );
    }

    return ref;
}

static HRESULT WINAPI vector_view_GetIids( IVectorView_IInspectable *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI vector_view_GetRuntimeClassName( IVectorView_IInspectable *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI vector_view_GetTrustLevel( IVectorView_IInspectable *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI vector_view_GetAt( IVectorView_IInspectable *iface, UINT32 index, IInspectable **value )
{
    struct vector_view *impl = impl_from_IVectorView_IInspectable( iface );

    TRACE( "iface %p, index %u, value %p.\n", iface, index, value );

    *value = NULL;
    if (index >= impl->size) return E_BOUNDS;

    IInspectable_AddRef( (*value = impl->elements[index]) );
    return S_OK;
}

static HRESULT WINAPI vector_view_get_Size( IVectorView_IInspectable *iface, UINT32 *value )
{
    struct vector_view *impl = impl_from_IVectorView_IInspectable( iface );

    TRACE( "iface %p, value %p.\n", iface, value );

    *value = impl->size;
    return S_OK;
}

static HRESULT WINAPI vector_view_IndexOf( IVectorView_IInspectable *iface, IInspectable *element,
                                           UINT32 *index, BOOLEAN *found )
{
    struct vector_view *impl = impl_from_IVectorView_IInspectable( iface );
    ULONG i;

    TRACE( "iface %p, element %p, index %p, found %p.\n", iface, element, index, found );

    for (i = 0; i < impl->size; ++i) if (impl->elements[i] == element) break;
    if ((*found = (i < impl->size))) *index = i;
    else *index = 0;

    return S_OK;
}

static HRESULT WINAPI vector_view_GetMany( IVectorView_IInspectable *iface, UINT32 start_index,
                                           UINT32 items_size, IInspectable **items, UINT *count )
{
    struct vector_view *impl = impl_from_IVectorView_IInspectable( iface );
    UINT32 i;

    TRACE( "iface %p, start_index %u, items_size %u, items %p, count %p.\n",
           iface, start_index, items_size, items, count );

    if (start_index >= impl->size) return E_BOUNDS;

    for (i = start_index; i < impl->size; ++i)
    {
        if (i - start_index >= items_size) break;
        IInspectable_AddRef( (items[i - start_index] = impl->elements[i]) );
    }
    *count = i - start_index;

    return S_OK;
}

static const struct IVectorView_IInspectableVtbl vector_view_vtbl =
{
    vector_view_QueryInterface,
    vector_view_AddRef,
    vector_view_Release,
    /* IInspectable methods */
    vector_view_GetIids,
    vector_view_GetRuntimeClassName,
    vector_view_GetTrustLevel,
    /* IVectorView<IInspectable*> methods */
    vector_view_GetAt,
    vector_view_get_Size,
    vector_view_IndexOf,
    vector_view_GetMany,
};

struct vector
{
    IVector_IInspectable IVector_IInspectable_iface;
    const GUID *iid;
    const GUID *view_iid;
    LONG ref;

    UINT32 size;
    UINT32 capacity;
    IInspectable **elements;
};

static inline struct vector *impl_from_IVector_IInspectable( IVector_IInspectable *iface )
{
    return CONTAINING_RECORD( iface, struct vector, IVector_IInspectable_iface );
}

static HRESULT WINAPI vector_QueryInterface( IVector_IInspectable *iface, REFIID iid, void **out )
{
    struct vector *impl = impl_from_IVector_IInspectable( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, impl->iid ))
    {
        IUnknown_AddRef( iface );
        *out = iface;
        return S_OK;
    }

    WARN( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI vector_AddRef( IVector_IInspectable *iface )
{
    struct vector *impl = impl_from_IVector_IInspectable( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI vector_Release( IVector_IInspectable *iface )
{
    struct vector *impl = impl_from_IVector_IInspectable( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref)
    {
        IVector_IInspectable_Clear( iface );
        free( impl );
    }

    return ref;
}

static HRESULT WINAPI vector_GetIids( IVector_IInspectable *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI vector_GetRuntimeClassName( IVector_IInspectable *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI vector_GetTrustLevel( IVector_IInspectable *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI vector_GetAt( IVector_IInspectable *iface, UINT32 index, IInspectable **value )
{
    struct vector *impl = impl_from_IVector_IInspectable( iface );

    TRACE( "iface %p, index %u, value %p.\n", iface, index, value );

    *value = NULL;
    if (index >= impl->size) return E_BOUNDS;

    IInspectable_AddRef( (*value = impl->elements[index]) );
    return S_OK;
}

static HRESULT WINAPI vector_get_Size( IVector_IInspectable *iface, UINT32 *value )
{
    struct vector *impl = impl_from_IVector_IInspectable( iface );
    TRACE( "iface %p, value %p.\n", iface, value );
    *value = impl->size;
    return S_OK;
}

static HRESULT WINAPI vector_GetView( IVector_IInspectable *iface, IVectorView_IInspectable **value )
{
    struct vector *impl = impl_from_IVector_IInspectable( iface );
    struct vector_view *view;
    ULONG i;

    TRACE( "iface %p, value %p.\n", iface, value );

    if (!(view = calloc( 1, offsetof( struct vector_view, elements[impl->size] ) ))) return E_OUTOFMEMORY;
    view->IVectorView_IInspectable_iface.lpVtbl = &vector_view_vtbl;
    view->iid = impl->view_iid;
    view->ref = 1;

    for (i = 0; i < impl->size; ++i) IInspectable_AddRef( (view->elements[view->size++] = impl->elements[i]) );

    *value = &view->IVectorView_IInspectable_iface;
    return S_OK;
}

static HRESULT WINAPI vector_IndexOf( IVector_IInspectable *iface, IInspectable *element, UINT32 *index, BOOLEAN *found )
{
    struct vector *impl = impl_from_IVector_IInspectable( iface );
    ULONG i;

    TRACE( "iface %p, element %p, index %p, found %p.\n", iface, element, index, found );

    for (i = 0; i < impl->size; ++i) if (impl->elements[i] == element) break;
    if ((*found = (i < impl->size))) *index = i;
    else *index = 0;

    return S_OK;
}

static HRESULT WINAPI vector_SetAt( IVector_IInspectable *iface, UINT32 index, IInspectable *value )
{
    struct vector *impl = impl_from_IVector_IInspectable( iface );

    TRACE( "iface %p, index %u, value %p.\n", iface, index, value );

    if (index >= impl->size) return E_BOUNDS;
    IInspectable_Release( impl->elements[index] );
    IInspectable_AddRef( (impl->elements[index] = value) );
    return S_OK;
}

static HRESULT WINAPI vector_InsertAt( IVector_IInspectable *iface, UINT32 index, IInspectable *value )
{
    struct vector *impl = impl_from_IVector_IInspectable( iface );
    IInspectable **tmp = impl->elements;

    TRACE( "iface %p, index %u, value %p.\n", iface, index, value );

    if (impl->size == impl->capacity)
    {
        impl->capacity = max( 32, impl->capacity * 3 / 2 );
        if (!(impl->elements = realloc( impl->elements, impl->capacity * sizeof(*impl->elements) )))
        {
            impl->elements = tmp;
            return E_OUTOFMEMORY;
        }
    }

    memmove( impl->elements + index + 1, impl->elements + index, impl->size++ * sizeof(*impl->elements) );
    IInspectable_AddRef( (impl->elements[index] = value) );
    return S_OK;
}

static HRESULT WINAPI vector_RemoveAt( IVector_IInspectable *iface, UINT32 index )
{
    struct vector *impl = impl_from_IVector_IInspectable( iface );

    TRACE( "iface %p, index %u.\n", iface, index );

    if (index >= impl->size) return E_BOUNDS;
    IInspectable_Release( impl->elements[index] );
    memmove( impl->elements + index, impl->elements + index + 1, --impl->size * sizeof(*impl->elements) );
    return S_OK;
}

static HRESULT WINAPI vector_Append( IVector_IInspectable *iface, IInspectable *value )
{
    struct vector *impl = impl_from_IVector_IInspectable( iface );

    TRACE( "iface %p, value %p.\n", iface, value );

    return IVector_IInspectable_InsertAt( iface, impl->size, value );
}

static HRESULT WINAPI vector_RemoveAtEnd( IVector_IInspectable *iface )
{
    struct vector *impl = impl_from_IVector_IInspectable( iface );

    TRACE( "iface %p.\n", iface );

    if (impl->size) IInspectable_Release( impl->elements[--impl->size] );
    return S_OK;
}

static HRESULT WINAPI vector_Clear( IVector_IInspectable *iface )
{
    struct vector *impl = impl_from_IVector_IInspectable( iface );

    TRACE( "iface %p.\n", iface );

    while (impl->size) IVector_IInspectable_RemoveAtEnd( iface );
    free( impl->elements );
    impl->capacity = 0;
    impl->elements = NULL;

    return S_OK;
}

static HRESULT WINAPI vector_GetMany( IVector_IInspectable *iface, UINT32 start_index,
                                      UINT32 items_size, IInspectable **items, UINT *count )
{
    struct vector *impl = impl_from_IVector_IInspectable( iface );
    UINT32 i;

    TRACE( "iface %p, start_index %u, items_size %u, items %p, count %p.\n",
           iface, start_index, items_size, items, count );

    if (start_index >= impl->size) return E_BOUNDS;

    for (i = start_index; i < impl->size; ++i)
    {
        if (i - start_index >= items_size) break;
        IInspectable_AddRef( (items[i - start_index] = impl->elements[i]) );
    }
    *count = i - start_index;

    return S_OK;
}

static HRESULT WINAPI vector_ReplaceAll( IVector_IInspectable *iface, UINT32 count, IInspectable **items )
{
    HRESULT hr;
    ULONG i;

    TRACE( "iface %p, count %u, items %p.\n", iface, count, items );

    hr = IVector_IInspectable_Clear( iface );
    for (i = 0; i < count && SUCCEEDED(hr); ++i) hr = IVector_IInspectable_Append( iface, items[i] );
    return hr;
}

static const struct IVector_IInspectableVtbl vector_vtbl =
{
    vector_QueryInterface,
    vector_AddRef,
    vector_Release,
    /* IInspectable methods */
    vector_GetIids,
    vector_GetRuntimeClassName,
    vector_GetTrustLevel,
    /* IVector<IInspectable*> methods */
    vector_GetAt,
    vector_get_Size,
    vector_GetView,
    vector_IndexOf,
    vector_SetAt,
    vector_InsertAt,
    vector_RemoveAt,
    vector_Append,
    vector_RemoveAtEnd,
    vector_Clear,
    vector_GetMany,
    vector_ReplaceAll,
};

HRESULT vector_create( REFIID iid, REFIID view_iid, void **out )
{
    struct vector *impl;

    TRACE( "iid %s, out %p.\n", debugstr_guid( iid ), out );

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->IVector_IInspectable_iface.lpVtbl = &vector_vtbl;
    impl->iid = iid;
    impl->view_iid = view_iid;
    impl->ref = 1;

    *out = &impl->IVector_IInspectable_iface;
    TRACE( "created %p\n", *out );
    return S_OK;
}
