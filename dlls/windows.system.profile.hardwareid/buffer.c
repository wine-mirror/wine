/*
 * Copyright 2024 Onni Kukkonen
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

#include <assert.h>

#include "wine/debug.h"
#include "objbase.h"

#define WIDL_using_Windows_Storage
#include "windows.storage.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypto);

static inline struct buffer_impl *impl_from_IBufferByteAccess( IBufferByteAccess *iface )
{
    return CONTAINING_RECORD( iface, struct buffer_impl, IBufferByteAccess_iface );
}

static HRESULT WINAPI bufferaccess_impl_QueryInterface( IBufferByteAccess *iface, REFIID iid, void **out )
{
    struct buffer_impl *impl = impl_from_IBufferByteAccess( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IBufferByteAccess ))
    {
        *out = &impl->IBufferByteAccess_iface;
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI bufferaccess_impl_AddRef( IBufferByteAccess *iface )
{
    struct buffer_impl *impl = impl_from_IBufferByteAccess( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI bufferaccess_impl_Release( IBufferByteAccess *iface )
{
    struct buffer_impl *impl = impl_from_IBufferByteAccess( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI bufferaccess_impl_get_Buffer( IBufferByteAccess *iface, BYTE **value )
{
    struct buffer_impl *impl = impl_from_IBufferByteAccess( iface );
    TRACE( "iface %p value %p \n", iface, value );
    *value = impl->dataptr;
    return S_OK;
}

static const struct IBufferByteAccessVtbl bufferaccess_vtbl =
{
    bufferaccess_impl_QueryInterface,
    bufferaccess_impl_AddRef,
    bufferaccess_impl_Release,
    /* IBufferByteAccess methods */
    bufferaccess_impl_get_Buffer
};

static inline struct buffer_impl *impl_from_IBuffer( IBuffer *iface )
{
    return CONTAINING_RECORD( iface, struct buffer_impl, IBuffer_iface );
}

static HRESULT WINAPI buffer_impl_QueryInterface( IBuffer *iface, REFIID iid, void **out )
{
    struct buffer_impl *impl = impl_from_IBuffer( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IBuffer ))
    {
        *out = &impl->IBuffer_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IBufferByteAccess ))
    {
        *out = &impl->IBufferByteAccess_iface;
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI buffer_impl_AddRef( IBuffer *iface )
{
    struct buffer_impl *impl = impl_from_IBuffer( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI buffer_impl_Release( IBuffer *iface )
{
    struct buffer_impl *impl = impl_from_IBuffer( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI buffer_impl_GetIids( IBuffer *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI buffer_impl_GetRuntimeClassName( IBuffer *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI buffer_impl_GetTrustLevel( IBuffer *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI buffer_impl_get_Capacity( IBuffer *iface, UINT32 *value)
{
    struct buffer_impl *impl = impl_from_IBuffer( iface );
    FIXME( "iface %p, value %p semi-stub!\n", iface, value);
   
    *value = impl->length;
    return S_OK;
}

static HRESULT WINAPI buffer_impl_get_Length( IBuffer *iface, UINT32 *value)
{
    struct buffer_impl *impl = impl_from_IBuffer( iface );
    *value = impl->length;
    return S_OK;
}

static HRESULT WINAPI buffer_impl_set_Length( IBuffer *iface, UINT32 value)
{
    FIXME( "iface %p, value %u stub!\n", iface, value);
    return E_NOTIMPL;
}

static const struct IBufferVtbl buffer_vtbl =
{
    buffer_impl_QueryInterface,
    buffer_impl_AddRef,
    buffer_impl_Release,
    /* IInspectable methods */
    buffer_impl_GetIids,
    buffer_impl_GetRuntimeClassName,
    buffer_impl_GetTrustLevel,
    /* IBuffer methods */
    buffer_impl_get_Capacity,
    buffer_impl_get_Length,
    buffer_impl_set_Length,
};

struct buffer_impl* alloc_buffer(UINT32 length) {
    struct buffer_impl *impl;

    if (length <= 0) return NULL;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return NULL;

    impl->IBuffer_iface.lpVtbl = &buffer_vtbl;
    impl->IBufferByteAccess_iface.lpVtbl = &bufferaccess_vtbl;
    impl->ref = 1;
    impl->dataptr = calloc(1, length);
    impl->length = length;

    TRACE( "created IBuffer %p.\n", impl);
    return impl;
}
