/* WinRT weak reference helpers
 *
 * Copyright 2025 Zhiyi Zhang for CodeWeavers
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
#include "weakref.h"

/* The control block is contained in struct weak_reference so that we don't have to allocate a
 * separate struct for it */
struct weak_reference
{
    IWeakReference IWeakReference_iface;
    IUnknown *object;
    /* control block */
    LONG ref_strong;
    LONG ref_weak;
};

static inline struct weak_reference *impl_from_IWeakReference( IWeakReference *iface )
{
    return CONTAINING_RECORD(iface, struct weak_reference, IWeakReference_iface);
}

static HRESULT WINAPI weak_reference_QueryInterface( IWeakReference *iface, REFIID iid, void **out )
{
    struct weak_reference *impl = impl_from_IWeakReference( iface );

    if (IsEqualGUID( iid, &IID_IUnknown ) || IsEqualGUID( iid, &IID_IWeakReference ))
    {
        *out = &impl->IWeakReference_iface;
        IWeakReference_AddRef( *out );
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI weak_reference_AddRef( IWeakReference *iface )
{
    struct weak_reference *impl = impl_from_IWeakReference( iface );
    return InterlockedIncrement( &impl->ref_weak );
}

static ULONG WINAPI weak_reference_Release( IWeakReference *iface )
{
    struct weak_reference *impl = impl_from_IWeakReference( iface );
    ULONG ref = InterlockedDecrement( &impl->ref_weak );

    if (!ref)
        free( impl );
    return ref;
}

static HRESULT WINAPI weak_reference_Resolve( IWeakReference *iface, REFIID iid, IInspectable **out )
{
    struct weak_reference *impl = impl_from_IWeakReference( iface );
    HRESULT hr;
    LONG ref;

    *out = NULL;
    do
    {
        if (!(ref = ReadNoFence( &impl->ref_strong )))
            return S_OK;
    } while (ref != InterlockedCompareExchange( &impl->ref_strong, ref + 1, ref ));

    hr = IUnknown_QueryInterface( impl->object, iid, (void **)out );
    IUnknown_Release( impl->object );
    return hr;
}

static const struct IWeakReferenceVtbl weak_reference_vtbl =
{
    weak_reference_QueryInterface,
    weak_reference_AddRef,
    weak_reference_Release,
    weak_reference_Resolve,
};

static inline struct weak_reference_source *impl_from_IWeakReferenceSource( IWeakReferenceSource *iface )
{
    return CONTAINING_RECORD(iface, struct weak_reference_source, IWeakReferenceSource_iface);
}

static HRESULT WINAPI weak_reference_source_QueryInterface( IWeakReferenceSource *iface, REFIID iid, void **out )
{
    struct weak_reference_source *weak_reference_source = impl_from_IWeakReferenceSource( iface );
    struct weak_reference *weak_reference = impl_from_IWeakReference( weak_reference_source->weak_reference );
    return IUnknown_QueryInterface( weak_reference->object, iid, out );
}

static ULONG WINAPI weak_reference_source_AddRef( IWeakReferenceSource *iface )
{
    struct weak_reference_source *weak_reference_source = impl_from_IWeakReferenceSource( iface );
    struct weak_reference *weak_reference = impl_from_IWeakReference( weak_reference_source->weak_reference );
    return IUnknown_AddRef( weak_reference->object );
}

static ULONG WINAPI weak_reference_source_Release( IWeakReferenceSource *iface )
{
    struct weak_reference_source *weak_reference_source = impl_from_IWeakReferenceSource( iface );
    struct weak_reference *weak_reference = impl_from_IWeakReference( weak_reference_source->weak_reference );
    return IUnknown_Release( weak_reference->object );
}

static HRESULT WINAPI weak_reference_source_GetWeakReference( IWeakReferenceSource *iface,
                                                              IWeakReference **weak_reference )
{
    struct weak_reference_source *impl = impl_from_IWeakReferenceSource( iface );

    *weak_reference = impl->weak_reference;
    IWeakReference_AddRef( *weak_reference );
    return S_OK;
}

static const struct IWeakReferenceSourceVtbl weak_reference_source_vtbl =
{
    weak_reference_source_QueryInterface,
    weak_reference_source_AddRef,
    weak_reference_source_Release,
    weak_reference_source_GetWeakReference,
};

static HRESULT weak_reference_create( IUnknown *object, IWeakReference **out )
{
    struct weak_reference *weak_reference;

    if (!(weak_reference = calloc( 1, sizeof(*weak_reference) )))
        return E_OUTOFMEMORY;

    weak_reference->IWeakReference_iface.lpVtbl = &weak_reference_vtbl;
    weak_reference->ref_strong = 1;
    weak_reference->ref_weak = 1;
    weak_reference->object = object;
    *out = &weak_reference->IWeakReference_iface;
    return S_OK;
}

HRESULT weak_reference_source_init( struct weak_reference_source *source, IUnknown *object )
{
    source->IWeakReferenceSource_iface.lpVtbl = &weak_reference_source_vtbl;
    return weak_reference_create( object, &source->weak_reference );
}

ULONG weak_reference_strong_add_ref( struct weak_reference_source *source )
{
    struct weak_reference *impl = impl_from_IWeakReference( source->weak_reference );
    return InterlockedIncrement( &impl->ref_strong );
}

ULONG weak_reference_strong_release( struct weak_reference_source *source )
{
    struct weak_reference *impl = impl_from_IWeakReference( source->weak_reference );
    ULONG ref = InterlockedDecrement( &impl->ref_strong );

    if (!ref)
        IWeakReference_Release( source->weak_reference );
    return ref;
}
