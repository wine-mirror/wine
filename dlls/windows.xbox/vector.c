/* WinRT Windows.Xbox.Input implementation
 *
 * Minimal read-only IVectorView<T> helper, local to this DLL.
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

WINE_DEFAULT_DEBUG_CHANNEL(xbox);

struct iterator
{
    IIterator_IInspectable IIterator_IInspectable_iface;
    const GUID *iid;
    LONG ref;

    IVectorView_IInspectable *view;
    UINT32 index;
    UINT32 size;
};

static inline struct iterator *impl_from_IIterator_IInspectable( IIterator_IInspectable *iface )
{
    return CONTAINING_RECORD( iface, struct iterator, IIterator_IInspectable_iface );
}

static HRESULT WINAPI iterator_QueryInterface( IIterator_IInspectable *iface, REFIID iid, void **out )
{
    struct iterator *impl = impl_from_IIterator_IInspectable( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, impl->iid ))
    {
        IInspectable_AddRef( (*out = &impl->IIterator_IInspectable_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI iterator_AddRef( IIterator_IInspectable *iface )
{
    struct iterator *impl = impl_from_IIterator_IInspectable( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI iterator_Release( IIterator_IInspectable *iface )
{
    struct iterator *impl = impl_from_IIterator_IInspectable( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref)
    {
        IVectorView_IInspectable_Release( impl->view );
        free( impl );
    }

    return ref;
}

static HRESULT WINAPI iterator_GetIids( IIterator_IInspectable *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI iterator_GetRuntimeClassName( IIterator_IInspectable *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI iterator_GetTrustLevel( IIterator_IInspectable *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI iterator_get_Current( IIterator_IInspectable *iface, IInspectable **value )
{
    struct iterator *impl = impl_from_IIterator_IInspectable( iface );
    TRACE( "iface %p, value %p.\n", iface, value );
    return IVectorView_IInspectable_GetAt( impl->view, impl->index, value );
}

static HRESULT WINAPI iterator_get_HasCurrent( IIterator_IInspectable *iface, boolean *value )
{
    struct iterator *impl = impl_from_IIterator_IInspectable( iface );

    TRACE( "iface %p, value %p.\n", iface, value );

    *value = impl->index < impl->size;
    return S_OK;
}

static HRESULT WINAPI iterator_MoveNext( IIterator_IInspectable *iface, boolean *value )
{
    struct iterator *impl = impl_from_IIterator_IInspectable( iface );

    TRACE( "iface %p, value %p.\n", iface, value );

    if (impl->index < impl->size) impl->index++;
    return IIterator_IInspectable_get_HasCurrent( iface, value );
}

static HRESULT WINAPI iterator_GetMany( IIterator_IInspectable *iface, UINT32 items_size,
                                        IInspectable **items, UINT *count )
{
    struct iterator *impl = impl_from_IIterator_IInspectable( iface );
    TRACE( "iface %p, items_size %u, items %p, count %p.\n", iface, items_size, items, count );
    return IVectorView_IInspectable_GetMany( impl->view, impl->index, items_size, items, count );
}

static const IIterator_IInspectableVtbl iterator_vtbl =
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

struct vector_view
{
    IVectorView_IInspectable IVectorView_IInspectable_iface;
    IIterable_IInspectable IIterable_IInspectable_iface;
    struct vector_view_iids iids;
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
        IsEqualGUID( iid, impl->iids.view ))
    {
        IInspectable_AddRef( (*out = &impl->IVectorView_IInspectable_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, impl->iids.iterable ))
    {
        IInspectable_AddRef( (*out = &impl->IIterable_IInspectable_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
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

DEFINE_IINSPECTABLE_( iterable_view, IIterable_IInspectable, struct vector_view, view_impl_from_IIterable_IInspectable,
                      IIterable_IInspectable_iface, &impl->IVectorView_IInspectable_iface )

static HRESULT WINAPI iterable_view_First( IIterable_IInspectable *iface, IIterator_IInspectable **value )
{
    struct vector_view *impl = view_impl_from_IIterable_IInspectable( iface );
    struct iterator *iter;

    TRACE( "iface %p, value %p.\n", iface, value );

    if (!(iter = calloc( 1, sizeof(struct iterator) ))) return E_OUTOFMEMORY;
    iter->IIterator_IInspectable_iface.lpVtbl = &iterator_vtbl;
    iter->iid = impl->iids.iterator;
    iter->ref = 1;

    IVectorView_IInspectable_AddRef( (iter->view = &impl->IVectorView_IInspectable_iface) );
    iter->size = impl->size;

    *value = &iter->IIterator_IInspectable_iface;
    return S_OK;
}

static const struct IIterable_IInspectableVtbl iterable_view_vtbl =
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

HRESULT vector_view_create( const struct vector_view_iids *iids, IInspectable **elements, UINT32 count, void **out )
{
    struct vector_view *view;
    UINT32 i;

    TRACE( "iid %s, elements %p, count %u, out %p.\n", debugstr_guid( iids->view ), elements, count, out );

    if (!(view = calloc( 1, offsetof( struct vector_view, elements[count] ) ))) return E_OUTOFMEMORY;
    view->IVectorView_IInspectable_iface.lpVtbl = &vector_view_vtbl;
    view->IIterable_IInspectable_iface.lpVtbl = &iterable_view_vtbl;
    view->iids = *iids;
    view->ref = 1;

    for (i = 0; i < count; ++i) IInspectable_AddRef( (view->elements[view->size++] = elements[i]) );

    *out = &view->IVectorView_IInspectable_iface;
    return S_OK;
}
