/* WinRT Windows.Media.Speech implementation
 *
 * Copyright 2022 Bernhard Kölbl for CodeWeavers
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

WINE_DEFAULT_DEBUG_CHANNEL(speech);

/*
 *
 * IIterator<HSTRING>
 *
 */

struct iterator_hstring
{
   IIterator_HSTRING IIterator_HSTRING_iface;
   LONG ref;

   IVectorView_HSTRING *view;
   UINT32 index;
   UINT32 size;
};

static inline struct iterator_hstring *impl_from_IIterator_HSTRING( IIterator_HSTRING *iface )
{
   return CONTAINING_RECORD(iface, struct iterator_hstring, IIterator_HSTRING_iface);
}

static HRESULT WINAPI iterator_hstring_QueryInterface( IIterator_HSTRING *iface, REFIID iid, void **out )
{
    struct iterator_hstring *impl = impl_from_IIterator_HSTRING(iface);

   TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

   if (IsEqualGUID(iid, &IID_IUnknown) ||
       IsEqualGUID(iid, &IID_IInspectable) ||
       IsEqualGUID(iid, &IID_IAgileObject) ||
       IsEqualGUID(iid, &IID_IIterator_HSTRING))
    {
       IInspectable_AddRef((*out = &impl->IIterator_HSTRING_iface));
       return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI iterator_hstring_AddRef( IIterator_HSTRING *iface )
{
    struct iterator_hstring *impl = impl_from_IIterator_HSTRING(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p increasing refcount to %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI iterator_hstring_Release( IIterator_HSTRING *iface )
{
    struct iterator_hstring *impl = impl_from_IIterator_HSTRING(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);

    TRACE("iface %p decreasing refcount to %lu.\n", iface, ref);

    if (!ref)
    {
        IVectorView_HSTRING_Release(impl->view);
        free(impl);
    }

    return ref;
}

static HRESULT WINAPI iterator_hstring_GetIids( IIterator_HSTRING *iface, ULONG *iid_count, IID **iids )
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI iterator_hstring_GetRuntimeClassName( IIterator_HSTRING *iface, HSTRING *class_name )
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI iterator_hstring_GetTrustLevel( IIterator_HSTRING *iface, TrustLevel *trust_level )
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI iterator_hstring_get_Current( IIterator_HSTRING *iface, HSTRING *value )
{
    struct iterator_hstring *impl = impl_from_IIterator_HSTRING(iface);
    TRACE("iface %p, value %p.\n", iface, value);
    return IVectorView_HSTRING_GetAt(impl->view, impl->index, value);
}

static HRESULT WINAPI iterator_hstring_get_HasCurrent( IIterator_HSTRING *iface, BOOL *value )
{
    struct iterator_hstring *impl = impl_from_IIterator_HSTRING(iface);

    TRACE("iface %p, value %p.\n", iface, value);

    *value = impl->index < impl->size;
    return S_OK;
}

static HRESULT WINAPI iterator_hstring_MoveNext( IIterator_HSTRING *iface, BOOL *value )
{
    struct iterator_hstring *impl = impl_from_IIterator_HSTRING(iface);

    TRACE("iface %p, value %p.\n", iface, value);

    if (impl->index < impl->size) impl->index++;
    return IIterator_HSTRING_get_HasCurrent(iface, value);
}

static HRESULT WINAPI iterator_hstring_GetMany( IIterator_HSTRING *iface, UINT32 items_size,
                                                HSTRING *items, UINT *count )
{
    struct iterator_hstring *impl = impl_from_IIterator_HSTRING(iface);
    TRACE("iface %p, items_size %u, items %p, count %p.\n", iface, items_size, items, count);
    return IVectorView_HSTRING_GetMany(impl->view, impl->index, items_size, items, count);
}

static const IIterator_HSTRINGVtbl iterator_hstring_vtbl =
{
    /* IUnknown methods */
    iterator_hstring_QueryInterface,
    iterator_hstring_AddRef,
    iterator_hstring_Release,
    /* IInspectable methods */
    iterator_hstring_GetIids,
    iterator_hstring_GetRuntimeClassName,
    iterator_hstring_GetTrustLevel,
    /* IIterator<HSTRING> methods */
    iterator_hstring_get_Current,
    iterator_hstring_get_HasCurrent,
    iterator_hstring_MoveNext,
    iterator_hstring_GetMany,
};

/*
 *
 * IVectorView<HSTRING>
 *
 */

struct vector_view_hstring
{
    IVectorView_HSTRING IVectorView_HSTRING_iface;
    IIterable_HSTRING IIterable_HSTRING_iface;
    LONG ref;

    UINT32 size;
    HSTRING elements[];
};

static inline struct vector_view_hstring *impl_from_IVectorView_HSTRING( IVectorView_HSTRING *iface )
{
    return CONTAINING_RECORD(iface, struct vector_view_hstring, IVectorView_HSTRING_iface);
}

static HRESULT WINAPI vector_view_hstring_QueryInterface( IVectorView_HSTRING *iface, REFIID iid, void **out )
{
    struct vector_view_hstring *impl = impl_from_IVectorView_HSTRING(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_IVectorView_HSTRING))
    {
        IInspectable_AddRef((*out = &impl->IVectorView_HSTRING_iface));
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_IIterable_HSTRING))
    {
        IInspectable_AddRef((*out = &impl->IIterable_HSTRING_iface));
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI vector_view_hstring_AddRef( IVectorView_HSTRING *iface )
{
    struct vector_view_hstring *impl = impl_from_IVectorView_HSTRING(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p increasing refcount to %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI vector_view_hstring_Release( IVectorView_HSTRING *iface )
{
    struct vector_view_hstring *impl = impl_from_IVectorView_HSTRING(iface);
    ULONG i, ref = InterlockedDecrement(&impl->ref);

    TRACE("iface %p decreasing refcount to %lu.\n", iface, ref);

    if (!ref)
    {
        for (i = 0; i < impl->size; ++i) WindowsDeleteString(impl->elements[i]);
        free(impl);
    }

    return ref;
}

static HRESULT WINAPI vector_view_hstring_GetIids( IVectorView_HSTRING *iface, ULONG *iid_count, IID **iids )
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI vector_view_hstring_GetRuntimeClassName( IVectorView_HSTRING *iface, HSTRING *class_name )
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI vector_view_hstring_GetTrustLevel( IVectorView_HSTRING *iface, TrustLevel *trust_level )
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI vector_view_hstring_GetAt( IVectorView_HSTRING *iface, UINT32 index, HSTRING *value )
{
    struct vector_view_hstring *impl = impl_from_IVectorView_HSTRING(iface);

    TRACE("iface %p, index %u, value %p.\n", iface, index, value);

    *value = NULL;
    if (index >= impl->size) return E_BOUNDS;

    return WindowsDuplicateString(impl->elements[index], value);
}

static HRESULT WINAPI vector_view_hstring_get_Size( IVectorView_HSTRING *iface, UINT32 *value )
{
    struct vector_view_hstring *impl = impl_from_IVectorView_HSTRING(iface);

    TRACE("iface %p, value %p.\n", iface, value);

    *value = impl->size;
    return S_OK;
}

static HRESULT WINAPI vector_view_hstring_IndexOf( IVectorView_HSTRING *iface, HSTRING element,
                                                   UINT32 *index, BOOLEAN *found )
{
    struct vector_view_hstring *impl = impl_from_IVectorView_HSTRING(iface);
    ULONG i;

    TRACE("iface %p, element %p, index %p, found %p.\n", iface, element, index, found);

    for (i = 0; i < impl->size; ++i) if (impl->elements[i] == element) break;
    if ((*found = (i < impl->size))) *index = i;
    else *index = 0;

    return S_OK;
}

static HRESULT WINAPI vector_view_hstring_GetMany( IVectorView_HSTRING *iface, UINT32 start_index,
                                                   UINT32 items_size, HSTRING *items, UINT *count )
{
    struct vector_view_hstring *impl = impl_from_IVectorView_HSTRING(iface);
    HRESULT hr;
    UINT32 i;

    TRACE( "iface %p, start_index %u, items_size %u, items %p, count %p.\n",
           iface, start_index, items_size, items, count );

    if (start_index >= impl->size) return E_BOUNDS;

    for (i = start_index; i < impl->size; ++i)
    {
        if (i - start_index >= items_size) break;
        if (FAILED(hr = WindowsDuplicateString(impl->elements[i], &items[i - start_index]))) goto error;
    }
    *count = i - start_index;

    return S_OK;

error:
    *count = 0;
    while (i-- > start_index) WindowsDeleteString(items[i-start_index]);
    return hr;
}

static const struct IVectorView_HSTRINGVtbl vector_view_hstring_vtbl =
{
    /* IUnknown methods */
    vector_view_hstring_QueryInterface,
    vector_view_hstring_AddRef,
    vector_view_hstring_Release,
    /* IInspectable methods */
    vector_view_hstring_GetIids,
    vector_view_hstring_GetRuntimeClassName,
    vector_view_hstring_GetTrustLevel,
    /* IVectorView<HSTRING> methods */
    vector_view_hstring_GetAt,
    vector_view_hstring_get_Size,
    vector_view_hstring_IndexOf,
    vector_view_hstring_GetMany,
};

/*
 *
 * IIterable<HSTRING>
 *
 */

DEFINE_IINSPECTABLE_(iterable_view_hstring, IIterable_HSTRING, struct vector_view_hstring, view_impl_from_IIterable_HSTRING,
                     IIterable_HSTRING_iface, &impl->IVectorView_HSTRING_iface)

static HRESULT WINAPI iterable_view_hstring_First( IIterable_HSTRING *iface, IIterator_HSTRING **value )
{
    struct vector_view_hstring *impl = view_impl_from_IIterable_HSTRING(iface);
    struct iterator_hstring *iter;

    TRACE("iface %p, value %p.\n", iface, value);

    if (!(iter = calloc(1, sizeof(*iter)))) return E_OUTOFMEMORY;
    iter->IIterator_HSTRING_iface.lpVtbl = &iterator_hstring_vtbl;
    iter->ref = 1;

    IVectorView_HSTRING_AddRef((iter->view = &impl->IVectorView_HSTRING_iface));
    iter->size = impl->size;

    *value = &iter->IIterator_HSTRING_iface;
    return S_OK;
}

static const struct IIterable_HSTRINGVtbl iterable_view_hstring_vtbl =
{
    /* IUnknown methods */
    iterable_view_hstring_QueryInterface,
    iterable_view_hstring_AddRef,
    iterable_view_hstring_Release,
    /* IInspectable methods */
    iterable_view_hstring_GetIids,
    iterable_view_hstring_GetRuntimeClassName,
    iterable_view_hstring_GetTrustLevel,
    /* IIterable<HSTRING> methods */
    iterable_view_hstring_First,
};

/*
 *
 * IVector<HSTRING>
 *
 */

struct vector_hstring
{
    IVector_HSTRING IVector_HSTRING_iface;
    IIterable_HSTRING IIterable_HSTRING_iface;
    LONG ref;

    UINT32 size;
    UINT32 capacity;
    HSTRING *elements;
};

static inline struct vector_hstring *impl_from_IVector_HSTRING( IVector_HSTRING *iface )
{
    return CONTAINING_RECORD(iface, struct vector_hstring, IVector_HSTRING_iface);
}

static HRESULT WINAPI vector_hstring_QueryInterface( IVector_HSTRING *iface, REFIID iid, void **out )
{
    struct vector_hstring *impl = impl_from_IVector_HSTRING(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_IVector_HSTRING))
    {
        IInspectable_AddRef((*out = &impl->IVector_HSTRING_iface));
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_IIterable_HSTRING))
    {
        IInspectable_AddRef((*out = &impl->IIterable_HSTRING_iface));
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI vector_hstring_AddRef( IVector_HSTRING *iface )
{
    struct vector_hstring *impl = impl_from_IVector_HSTRING(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI vector_hstring_Release( IVector_HSTRING *iface )
{
    struct vector_hstring *impl = impl_from_IVector_HSTRING(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);

    TRACE("iface %p, ref %lu.\n", iface, ref);

    if (!ref)
    {
        IVector_HSTRING_Clear(iface);
        free(impl);
    }

    return ref;
}

static HRESULT WINAPI vector_hstring_GetIids( IVector_HSTRING *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI vector_hstring_GetRuntimeClassName( IVector_HSTRING *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI vector_hstring_GetTrustLevel( IVector_HSTRING *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI vector_hstring_GetAt( IVector_HSTRING *iface, UINT32 index, HSTRING *value )
{
    struct vector_hstring *impl = impl_from_IVector_HSTRING(iface);

    TRACE( "iface %p, index %u, value %p.\n", iface, index, value );

    *value = NULL;
    if (index >= impl->size) return E_BOUNDS;

    return WindowsDuplicateString(impl->elements[index], value);
}

static HRESULT WINAPI vector_hstring_get_Size( IVector_HSTRING *iface, UINT32 *value )
{
    struct vector_hstring *impl = impl_from_IVector_HSTRING(iface);
    TRACE( "iface %p, value %p.\n", iface, value );
    *value = impl->size;
    return S_OK;
}

static HRESULT WINAPI vector_hstring_GetView( IVector_HSTRING *iface, IVectorView_HSTRING **value )
{
    struct vector_hstring *impl = impl_from_IVector_HSTRING(iface);
    struct vector_view_hstring *view;
    HRESULT hr;
    ULONG i;

    TRACE("iface %p, value %p.\n", iface, value);

    if (!(view = calloc(1, offsetof(struct vector_view_hstring, elements[impl->size])))) return E_OUTOFMEMORY;
    view->IVectorView_HSTRING_iface.lpVtbl = &vector_view_hstring_vtbl;
    view->IIterable_HSTRING_iface.lpVtbl = &iterable_view_hstring_vtbl;
    view->ref = 1;

    for (i = 0; i < impl->size; ++i)
        if (FAILED(hr = WindowsDuplicateString(impl->elements[i], &view->elements[view->size++]))) goto error;

    *value = &view->IVectorView_HSTRING_iface;
    return S_OK;

error:
    while (i-- > 0) WindowsDeleteString(view->elements[i]);
    free(view);
    return hr;
}

static HRESULT WINAPI vector_hstring_IndexOf( IVector_HSTRING *iface, HSTRING element, UINT32 *index, BOOLEAN *found )
{
    struct vector_hstring *impl = impl_from_IVector_HSTRING(iface);
    ULONG i;

    TRACE("iface %p, element %p, index %p, found %p.\n", iface, element, index, found);

    for (i = 0; i < impl->size; ++i) if (impl->elements[i] == element) break;
    if ((*found = (i < impl->size))) *index = i;
    else *index = 0;

    return S_OK;
}

static HRESULT WINAPI vector_hstring_SetAt( IVector_HSTRING *iface, UINT32 index, HSTRING value )
{
    struct vector_hstring *impl = impl_from_IVector_HSTRING(iface);
    HSTRING tmp;
    HRESULT hr;

    TRACE( "iface %p, index %u, value %p.\n", iface, index, value );

    if (index >= impl->size) return E_BOUNDS;

    if (FAILED(hr = WindowsDuplicateString(value, &tmp))) return hr;

    WindowsDeleteString(impl->elements[index]);
    impl->elements[index] = tmp;
    return S_OK;
}

static HRESULT WINAPI vector_hstring_InsertAt( IVector_HSTRING *iface, UINT32 index, HSTRING value )
{
    struct vector_hstring *impl = impl_from_IVector_HSTRING(iface);
    HSTRING tmp, *tmp2 = impl->elements;
    HRESULT hr;

    TRACE( "iface %p, index %u, value %p.\n", iface, index, value );

    if (FAILED(hr = WindowsDuplicateString(value, &tmp))) return hr;

    if (impl->size == impl->capacity)
    {
        impl->capacity = max(32, impl->capacity * 3 / 2);
        if (!(impl->elements = realloc(impl->elements, impl->capacity * sizeof(*impl->elements))))
        {
            impl->elements = tmp2;
            return E_OUTOFMEMORY;
        }
    }

    memmove(impl->elements + index + 1, impl->elements + index, (impl->size++ - index) * sizeof(*impl->elements));
    impl->elements[index] = tmp;
    return S_OK;
}

static HRESULT WINAPI vector_hstring_RemoveAt( IVector_HSTRING *iface, UINT32 index )
{
    struct vector_hstring *impl = impl_from_IVector_HSTRING(iface);

    TRACE("iface %p, index %u.\n", iface, index);

    if (index >= impl->size) return E_BOUNDS;

    WindowsDeleteString(impl->elements[index]);
    memmove(impl->elements + index, impl->elements + index + 1, (--impl->size - index) * sizeof(*impl->elements));
    return S_OK;
}

static HRESULT WINAPI vector_hstring_Append( IVector_HSTRING *iface, HSTRING value )
{
    struct vector_hstring *impl = impl_from_IVector_HSTRING(iface);

    TRACE("iface %p, value %p.\n", iface, value);

    return IVector_HSTRING_InsertAt(iface, impl->size, value);
}

static HRESULT WINAPI vector_hstring_RemoveAtEnd( IVector_HSTRING *iface )
{
    struct vector_hstring *impl = impl_from_IVector_HSTRING(iface);

    TRACE("iface %p.\n", iface);

    if (impl->size) WindowsDeleteString(impl->elements[--impl->size]);
    return S_OK;
}

static HRESULT WINAPI vector_hstring_Clear( IVector_HSTRING *iface )
{
    struct vector_hstring *impl = impl_from_IVector_HSTRING(iface);

    TRACE("iface %p.\n", iface);

    while (impl->size) IVector_HSTRING_RemoveAtEnd(iface);
    free(impl->elements);
    impl->capacity = 0;
    impl->elements = NULL;

    return S_OK;
}

static HRESULT WINAPI vector_hstring_GetMany( IVector_HSTRING *iface, UINT32 start_index,
                                              UINT32 items_size, HSTRING *items, UINT32 *count )
{
    struct vector_hstring *impl = impl_from_IVector_HSTRING(iface);
    HRESULT hr;
    UINT32 i;

    TRACE("iface %p, start_index %u, items_size %u, items %p, count %p.\n", iface, start_index, items_size, items, count);

    if (start_index >= impl->size) return E_BOUNDS;

    for (i = start_index; i < impl->size; ++i)
    {
        if (i - start_index >= items_size) break;
        if (FAILED(hr = WindowsDuplicateString(impl->elements[i], &items[i-start_index]))) goto error;
    }
    *count = i - start_index;

    return S_OK;

error:
    *count = 0;
    while (i-- > start_index) WindowsDeleteString(items[i-start_index]);
    return hr;
}

static HRESULT WINAPI vector_hstring_ReplaceAll( IVector_HSTRING *iface, UINT32 count, HSTRING *items )
{
    HRESULT hr;
    ULONG i;

    TRACE("iface %p, count %u, items %p.\n", iface, count, items);

    hr = IVector_HSTRING_Clear(iface);
    for (i = 0; i < count && SUCCEEDED(hr); ++i) hr = IVector_HSTRING_Append(iface, items[i]);
    return hr;
}

static const struct IVector_HSTRINGVtbl vector_hstring_vtbl =
{
    /* IUnknown methods */
    vector_hstring_QueryInterface,
    vector_hstring_AddRef,
    vector_hstring_Release,
    /* IInspectable methods */
    vector_hstring_GetIids,
    vector_hstring_GetRuntimeClassName,
    vector_hstring_GetTrustLevel,
    /* IVector<HSTRING> methods */
    vector_hstring_GetAt,
    vector_hstring_get_Size,
    vector_hstring_GetView,
    vector_hstring_IndexOf,
    vector_hstring_SetAt,
    vector_hstring_InsertAt,
    vector_hstring_RemoveAt,
    vector_hstring_Append,
    vector_hstring_RemoveAtEnd,
    vector_hstring_Clear,
    vector_hstring_GetMany,
    vector_hstring_ReplaceAll,
};

/*
 *
 * IIterable<HSTRING>
 *
 */

DEFINE_IINSPECTABLE(iterable_hstring, IIterable_HSTRING, struct vector_hstring, IVector_HSTRING_iface)

static HRESULT WINAPI iterable_hstring_First( IIterable_HSTRING *iface, IIterator_HSTRING **value )
{
    struct vector_hstring *impl = impl_from_IIterable_HSTRING(iface);
    IIterable_HSTRING *iterable;
    IVectorView_HSTRING *view;
    HRESULT hr;

    TRACE("iface %p, value %p.\n", iface, value);

    if (FAILED(hr = IVector_HSTRING_GetView(&impl->IVector_HSTRING_iface, &view))) return hr;

    hr = IVectorView_HSTRING_QueryInterface(view, &IID_IIterable_HSTRING, (void **)&iterable);
    IVectorView_HSTRING_Release(view);
    if (FAILED(hr)) return hr;

    hr = IIterable_HSTRING_First(iterable, value);
    IIterable_HSTRING_Release(iterable);
    return hr;
}

static const struct IIterable_HSTRINGVtbl iterable_hstring_vtbl =
{
    /* IUnknown methods */
    iterable_hstring_QueryInterface,
    iterable_hstring_AddRef,
    iterable_hstring_Release,
    /* IInspectable methods */
    iterable_hstring_GetIids,
    iterable_hstring_GetRuntimeClassName,
    iterable_hstring_GetTrustLevel,
    /* IIterable<HSTRING> methods */
    iterable_hstring_First,
};

HRESULT vector_hstring_create( IVector_HSTRING **out )
{
    struct vector_hstring *impl;

    TRACE("out %p.\n", out);

    if (!(impl = calloc(1, sizeof(*impl)))) return E_OUTOFMEMORY;
    impl->IVector_HSTRING_iface.lpVtbl = &vector_hstring_vtbl;
    impl->IIterable_HSTRING_iface.lpVtbl = &iterable_hstring_vtbl;
    impl->ref = 1;

    *out = &impl->IVector_HSTRING_iface;
    TRACE("created %p\n", *out);
    return S_OK;
}

HRESULT vector_hstring_create_copy( IIterable_HSTRING *iterable, IVector_HSTRING **out )
{
    struct vector_hstring *impl;
    IIterator_HSTRING *iterator;
    UINT32 capacity = 0;
    BOOL available;
    HRESULT hr;

    TRACE("iterable %p, out %p.\n", iterable, out);

    if (FAILED(hr = vector_hstring_create(out))) return hr;
    if (FAILED(hr = IIterable_HSTRING_First(iterable, &iterator))) goto error;

    for (IIterator_HSTRING_get_HasCurrent(iterator, &available); available; IIterator_HSTRING_MoveNext(iterator, &available))
        capacity++;

    IIterator_HSTRING_Release(iterator);

    impl = impl_from_IVector_HSTRING(*out);
    impl->size = 0;
    impl->capacity = capacity;
    if (!(impl->elements = realloc(impl->elements, impl->capacity * sizeof(*impl->elements)))) goto error;

    if (FAILED(hr = IIterable_HSTRING_First(iterable, &iterator))) goto error;

    for (IIterator_HSTRING_get_HasCurrent(iterator, &available); available; IIterator_HSTRING_MoveNext(iterator, &available))
    {
        HSTRING str;
        if (FAILED(hr = IIterator_HSTRING_get_Current(iterator, &str))) goto error;
        if (FAILED(hr = WindowsDuplicateString(str, &impl->elements[impl->size]))) goto error;
        WindowsDeleteString(str);
        impl->size++;
    }

    IIterator_HSTRING_Release(iterator);

    TRACE("created %p\n", *out);
    return S_OK;

error:
    IVector_HSTRING_Release(*out);
    return hr;
}
