/* WinRT Windows.Storage.Streams Implementation
 *
 * Copyright (C) 2025 Mohamad Al-Jaf
 * Copyright 2026 Conor McCarthy for CodeWeavers
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

#include <assert.h>
#include "private.h"

WINE_DEFAULT_DEBUG_CHANNEL(storage);

struct random_access_stream_reference_statics
{
    IActivationFactory IActivationFactory_iface;
    IRandomAccessStreamReferenceStatics IRandomAccessStreamReferenceStatics_iface;
    LONG ref;
};

static inline struct random_access_stream_reference_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct random_access_stream_reference_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct random_access_stream_reference_statics *impl = impl_from_IActivationFactory( iface );

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

    if (IsEqualGUID( iid, &IID_IRandomAccessStreamReferenceStatics ))
    {
        *out = &impl->IRandomAccessStreamReferenceStatics_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct random_access_stream_reference_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct random_access_stream_reference_statics *impl = impl_from_IActivationFactory( iface );
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
    return S_OK;
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

DEFINE_IINSPECTABLE( random_access_stream_reference_statics, IRandomAccessStreamReferenceStatics, struct random_access_stream_reference_statics, IActivationFactory_iface )

static HRESULT WINAPI random_access_stream_reference_statics_CreateFromFile( IRandomAccessStreamReferenceStatics *iface, IStorageFile *file,
                                                                             IRandomAccessStreamReference **stream_reference )
{
    FIXME( "iface %p, file %p, stream_reference %p stub!\n", iface, file, stream_reference );
    return E_NOTIMPL;
}

static HRESULT WINAPI random_access_stream_reference_statics_CreateFromUri( IRandomAccessStreamReferenceStatics *iface, IUriRuntimeClass *uri,
                                                                            IRandomAccessStreamReference **stream_reference )
{
    FIXME( "iface %p, uri %p, stream_reference %p stub!\n", iface, uri, stream_reference );
    return E_NOTIMPL;
}

static HRESULT WINAPI random_access_stream_reference_statics_CreateFromStream( IRandomAccessStreamReferenceStatics *iface, IRandomAccessStream *stream,
                                                                               IRandomAccessStreamReference **stream_reference )
{
    FIXME( "iface %p, stream %p, stream_reference %p stub!\n", iface, stream, stream_reference );

    *stream_reference = NULL;

    if (!stream) return E_POINTER;
    return E_NOTIMPL;
}

static const struct IRandomAccessStreamReferenceStaticsVtbl random_access_stream_reference_statics_vtbl =
{
    random_access_stream_reference_statics_QueryInterface,
    random_access_stream_reference_statics_AddRef,
    random_access_stream_reference_statics_Release,
    /* IInspectable methods */
    random_access_stream_reference_statics_GetIids,
    random_access_stream_reference_statics_GetRuntimeClassName,
    random_access_stream_reference_statics_GetTrustLevel,
    /* IRandomAccessStreamReferenceStatics methods */
    random_access_stream_reference_statics_CreateFromFile,
    random_access_stream_reference_statics_CreateFromUri,
    random_access_stream_reference_statics_CreateFromStream,
};

static struct random_access_stream_reference_statics random_access_stream_reference_statics =
{
    {&factory_vtbl},
    {&random_access_stream_reference_statics_vtbl},
    0,
};

IActivationFactory *random_access_stream_reference_factory = &random_access_stream_reference_statics.IActivationFactory_iface;

/*
 * InMemoryRandomAccessStream
 */

struct memory_stream
{
    IRandomAccessStream IRandomAccessStream_iface;
    IInputStream IInputStream_iface;
    IOutputStream IOutputStream_iface;
    IClosable IClosable_iface;
    LONG ref;

    BYTE *buffer;
    size_t capacity;
    size_t size;
    size_t pos;
    BOOL closed;
};

static HRESULT memory_stream_require_capacity( struct memory_stream *impl, size_t capacity )
{
    BYTE *new_buffer;

    if (capacity <= impl->capacity)
        return S_OK;

    capacity = max( capacity, impl->capacity + impl->capacity / 2u );
    capacity = max( capacity, 0x1000 );
    new_buffer = realloc( impl->buffer, capacity );

    if (!new_buffer)
        return HRESULT_FROM_WIN32( ERROR_DISK_FULL );

    /* Zero memory for security and to silence address sanitisers */
    memset( &new_buffer[impl->capacity], 0, capacity - impl->capacity );
    impl->capacity = capacity;
    impl->buffer = new_buffer;

    return S_OK;
}

static inline struct memory_stream *impl_from_IRandomAccessStream( IRandomAccessStream *iface )
{
    return CONTAINING_RECORD( iface, struct memory_stream, IRandomAccessStream_iface );
}

static HRESULT WINAPI memory_stream_random_access_QueryInterface( IRandomAccessStream *iface, REFIID iid, void **out )
{
    struct memory_stream *impl = impl_from_IRandomAccessStream( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown )
            || IsEqualGUID( iid, &IID_IInspectable )
            || IsEqualGUID( iid, &IID_IRandomAccessStream ))
    {
        *out = iface;
    }
    else if (IsEqualGUID( iid, &IID_IInputStream ))
    {
        *out = &impl->IInputStream_iface;
    }
    else if (IsEqualGUID( iid, &IID_IOutputStream ))
    {
        *out = &impl->IOutputStream_iface;
    }
    else if (IsEqualGUID( iid, &IID_IClosable ))
    {
        *out = &impl->IClosable_iface;
    }
    else
    {
        WARN( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ));
        *out = NULL;
        return E_NOINTERFACE;
    }

    IRandomAccessStream_AddRef( iface );
    return S_OK;
}

static ULONG WINAPI memory_stream_random_access_AddRef( IRandomAccessStream *iface )
{
    struct memory_stream *impl = impl_from_IRandomAccessStream( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI memory_stream_random_access_Release( IRandomAccessStream *iface )
{
    struct memory_stream *impl = impl_from_IRandomAccessStream( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p, ref %lu.\n", iface, ref );

    if (!ref)
        free( impl->buffer );

    return ref;
}

static HRESULT WINAPI memory_stream_random_access_GetIids( IRandomAccessStream *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI memory_stream_random_access_GetRuntimeClassName( IRandomAccessStream *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI memory_stream_random_access_GetTrustLevel( IRandomAccessStream *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI memory_stream_random_access_get_Size( IRandomAccessStream *iface, UINT64 *value )
{
    struct memory_stream *impl = impl_from_IRandomAccessStream( iface );

    TRACE( "iface %p, value %p.\n", iface, value );

    if (impl->closed)
    {
        *value = 0;
        return RO_E_CLOSED;
    }

    *value = impl->size;
    return S_OK;
}

static HRESULT WINAPI memory_stream_random_access_put_Size( IRandomAccessStream *iface, UINT64 value )
{
    struct memory_stream *impl = impl_from_IRandomAccessStream( iface );
    HRESULT hr;

    TRACE( "iface %p, value %I64u.\n", iface, value );

    if (impl->closed)
        return RO_E_CLOSED;

    /* Native truncates the size to 32 bits, which is not replicated here if size_t is 64 bits. */
    if (FAILED(hr = memory_stream_require_capacity( impl, value )))
        return hr;

    impl->size = min( value, SIZE_MAX );
    return S_OK;
}

static HRESULT WINAPI memory_stream_random_access_GetInputStreamAt( IRandomAccessStream *iface, UINT64 position,
        IInputStream **stream )
{
    FIXME( "iface %p, position %I64u, stream %p stub!\n", iface, position, stream );

    *stream = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI memory_stream_random_access_GetOutputStreamAt( IRandomAccessStream *iface, UINT64 position,
        IOutputStream **stream )
{
    FIXME( "iface %p, position %I64u, stream %p stub!\n", iface, position, stream );

    *stream = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI memory_stream_random_access_get_Position( IRandomAccessStream *iface, UINT64 *value )
{
    struct memory_stream *impl = impl_from_IRandomAccessStream( iface );

    TRACE( "iface %p, value %p.\n", iface, value );

    *value = impl->pos;
    return impl->closed ? RO_E_CLOSED : S_OK;
}

static HRESULT WINAPI memory_stream_random_access_Seek( IRandomAccessStream *iface, UINT64 position )
{
    struct memory_stream *impl = impl_from_IRandomAccessStream( iface );

    FIXME( "iface %p, position %I64u stub!\n", iface, position );

    if (impl->closed)
        return RO_E_CLOSED;

    return E_NOTIMPL;
}

static HRESULT WINAPI memory_stream_random_access_CloneStream( IRandomAccessStream *iface, IRandomAccessStream **stream )
{
    FIXME( "iface %p, stream %p stub!\n", iface, stream );

    *stream = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI memory_stream_random_access_get_CanRead( IRandomAccessStream *iface, BOOLEAN *value )
{
    TRACE( "iface %p, value %p.\n", iface, value );

    *value = TRUE;
    return S_OK;
}

static HRESULT WINAPI memory_stream_random_access_get_CanWrite( IRandomAccessStream *iface, BOOLEAN *value )
{
    TRACE( "iface %p, value %p.\n", iface, value );

    *value = TRUE;
    return S_OK;
}

static const struct IRandomAccessStreamVtbl memory_stream_random_access_vtbl =
{
    /* IUnknown methods */
    memory_stream_random_access_QueryInterface,
    memory_stream_random_access_AddRef,
    memory_stream_random_access_Release,
    /* IInspectable methods */
    memory_stream_random_access_GetIids,
    memory_stream_random_access_GetRuntimeClassName,
    memory_stream_random_access_GetTrustLevel,
    /* IRandomAccessStream methods */
    memory_stream_random_access_get_Size,
    memory_stream_random_access_put_Size,
    memory_stream_random_access_GetInputStreamAt,
    memory_stream_random_access_GetOutputStreamAt,
    memory_stream_random_access_get_Position,
    memory_stream_random_access_Seek,
    memory_stream_random_access_CloneStream,
    memory_stream_random_access_get_CanRead,
    memory_stream_random_access_get_CanWrite,
};

DEFINE_IINSPECTABLE( memory_stream_closable, IClosable, struct memory_stream, IRandomAccessStream_iface )

static HRESULT WINAPI memory_stream_closable_Close( IClosable *iface )
{
    struct memory_stream *impl = CONTAINING_RECORD( iface, struct memory_stream, IClosable_iface );

    TRACE( "iface %p.\n", iface );

    impl->closed = TRUE;
    return S_OK;
}

static const struct IClosableVtbl memory_stream_closable_vtbl =
{
    /* IUnknown methods */
    memory_stream_closable_QueryInterface,
    memory_stream_closable_AddRef,
    memory_stream_closable_Release,
    /* IInspectable methods */
    memory_stream_closable_GetIids,
    memory_stream_closable_GetRuntimeClassName,
    memory_stream_closable_GetTrustLevel,
    /* IClosable methods */
    memory_stream_closable_Close,
};

static inline struct memory_stream *impl_from_IInputStream( IInputStream *iface )
{
    return CONTAINING_RECORD( iface, struct memory_stream, IInputStream_iface );
}

static HRESULT WINAPI memory_stream_input_QueryInterface( IInputStream *iface, REFIID iid, void **out )
{
    struct memory_stream *impl = impl_from_IInputStream( iface );
    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );
    return IRandomAccessStream_QueryInterface( &impl->IRandomAccessStream_iface, iid, out );
}

static ULONG WINAPI memory_stream_input_AddRef( IInputStream *iface )
{
    struct memory_stream *impl = impl_from_IInputStream( iface );
    TRACE( "iface %p.\n", iface );
    return IRandomAccessStream_AddRef( &impl->IRandomAccessStream_iface );
}

static ULONG WINAPI memory_stream_input_Release( IInputStream *iface )
{
    struct memory_stream *impl = impl_from_IInputStream( iface );
    TRACE( "iface %p.\n", iface );
    return IRandomAccessStream_Release( &impl->IRandomAccessStream_iface );
}

static HRESULT WINAPI memory_stream_input_GetIids( IInputStream *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI memory_stream_input_GetRuntimeClassName( IInputStream *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI memory_stream_input_GetTrustLevel( IInputStream *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI memory_stream_input_ReadAsync( IInputStream *iface, IBuffer *buffer, UINT32 count,
        InputStreamOptions options, IAsyncOperationWithProgress_IBuffer_UINT32 **operation )
{
    struct memory_stream *impl = impl_from_IInputStream( iface );

    FIXME( "iface %p, buffer %p, count %u, options %d, operation %p stub!\n", iface, buffer, count, options, operation );

    *operation = NULL;

    if (impl->closed)
        return RO_E_CLOSED;

    return E_NOTIMPL;
}

static const struct IInputStreamVtbl memory_stream_input_vtbl =
{
    /* IUnknown methods */
    memory_stream_input_QueryInterface,
    memory_stream_input_AddRef,
    memory_stream_input_Release,
    /* IInspectable methods */
    memory_stream_input_GetIids,
    memory_stream_input_GetRuntimeClassName,
    memory_stream_input_GetTrustLevel,
    /* IInputStream methods */
    memory_stream_input_ReadAsync
};

static inline struct memory_stream *impl_from_IOutputStream( IOutputStream *iface )
{
    return CONTAINING_RECORD( iface, struct memory_stream, IOutputStream_iface );
}

static HRESULT WINAPI memory_stream_output_QueryInterface( IOutputStream *iface, REFIID iid, void **out )
{
    struct memory_stream *impl = impl_from_IOutputStream( iface );
    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );
    return IRandomAccessStream_QueryInterface( &impl->IRandomAccessStream_iface, iid, out );
}

static ULONG WINAPI memory_stream_output_AddRef( IOutputStream *iface )
{
    struct memory_stream *impl = impl_from_IOutputStream( iface );
    TRACE( "iface %p.\n", iface );
    return IRandomAccessStream_AddRef( &impl->IRandomAccessStream_iface );
}

static ULONG WINAPI memory_stream_output_Release( IOutputStream *iface )
{
    struct memory_stream *impl = impl_from_IOutputStream( iface );
    TRACE( "iface %p.\n", iface );
    return IRandomAccessStream_Release( &impl->IRandomAccessStream_iface );
}

static HRESULT WINAPI memory_stream_output_GetIids( IOutputStream *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI memory_stream_output_GetRuntimeClassName( IOutputStream *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI memory_stream_output_GetTrustLevel( IOutputStream *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT memory_stream_output_async( IUnknown *invoker, IUnknown *param, PROPVARIANT *result, BOOL called_async )
{
    struct memory_stream *impl = impl_from_IOutputStream( (IOutputStream *)invoker );
    IBuffer *buffer = (IBuffer *)param;
    IBufferByteAccess *access;
    size_t capacity;
    UINT32 length;
    HRESULT hr;
    BYTE *data;

    assert( !called_async );

    IBuffer_get_Length( buffer, &length );

    if (!length)
        return S_OK;

    capacity = impl->pos + length;
    if (capacity < impl->pos || capacity < length)
        return HRESULT_FROM_WIN32( ERROR_DISK_FULL );

    if (FAILED(hr = memory_stream_require_capacity( impl, capacity )))
        return hr;

    IBuffer_QueryInterface( buffer, &IID_IBufferByteAccess, (void **)&access );
    IBufferByteAccess_Buffer( access, &data );
    IBufferByteAccess_Release( access );

    memcpy( &impl->buffer[impl->pos], data, length );
    impl->pos += length;
    impl->size = max( impl->size, impl->pos );

    result->vt = VT_UI4;
    result->ulVal = length;

    return S_OK;
}

static HRESULT WINAPI memory_stream_output_WriteAsync( IOutputStream *iface, IBuffer *buffer,
        IAsyncOperationWithProgress_UINT32_UINT32 **operation )
{
    struct memory_stream *impl = impl_from_IOutputStream( iface );

    TRACE( "iface %p, buffer %p, operation %p.\n", iface, buffer, operation );

    *operation = NULL;

    if (!buffer)
        return E_POINTER;

    if (impl->closed)
        return RO_E_CLOSED;

    return async_operation_uint32_uint32_create( (IUnknown *)iface, (IUnknown *)buffer, memory_stream_output_async, operation );
}

static HRESULT WINAPI memory_stream_output_FlushAsync( IOutputStream *iface, IAsyncOperation_boolean **operation )
{
    struct memory_stream *impl = impl_from_IOutputStream( iface );

    FIXME( "iface %p, operation %p stub!\n", iface, operation );

    *operation = NULL;

    if (impl->closed)
        return RO_E_CLOSED;

    return E_NOTIMPL;
}

static const struct IOutputStreamVtbl memory_stream_output_vtbl =
{
    /* IUnknown methods */
    memory_stream_output_QueryInterface,
    memory_stream_output_AddRef,
    memory_stream_output_Release,
    /* IInspectable methods */
    memory_stream_output_GetIids,
    memory_stream_output_GetRuntimeClassName,
    memory_stream_output_GetTrustLevel,
    /* IOutputStream methods */
    memory_stream_output_WriteAsync,
    memory_stream_output_FlushAsync,
};

static HRESULT memory_stream_create( IRandomAccessStream **out )
{
    struct memory_stream *impl;

    TRACE( "out %p.\n", out );

    if (!(impl = calloc( 1, sizeof(*impl) )))
    {
        *out = NULL;
        return E_OUTOFMEMORY;
    }

    impl->ref = 1;
    impl->IRandomAccessStream_iface.lpVtbl = &memory_stream_random_access_vtbl;
    impl->IInputStream_iface.lpVtbl = &memory_stream_input_vtbl;
    impl->IOutputStream_iface.lpVtbl = &memory_stream_output_vtbl;
    impl->IClosable_iface.lpVtbl = &memory_stream_closable_vtbl;

    *out = &impl->IRandomAccessStream_iface;
    return S_OK;
}

struct memory_stream_factory
{
    IActivationFactory IActivationFactory_iface;
    LONG ref;
};

static inline struct memory_stream_factory *impl_memory_stream_factory_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct memory_stream_factory, IActivationFactory_iface );
}

static HRESULT STDMETHODCALLTYPE memory_stream_activation_factory_QueryInterface( IActivationFactory *iface, REFIID iid,
        void **out )
{
    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown )
            || IsEqualGUID( iid, &IID_IInspectable )
            || IsEqualGUID( iid, &IID_IAgileObject )
            || IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        IUnknown_AddRef( iface );
        *out = iface;
        return S_OK;
    }

    WARN( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE memory_stream_activation_factory_AddRef( IActivationFactory *iface )
{
    struct memory_stream_factory *impl = impl_memory_stream_factory_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG STDMETHODCALLTYPE memory_stream_activation_factory_Release( IActivationFactory *iface )
{
    struct memory_stream_factory *impl = impl_memory_stream_factory_from_IActivationFactory( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static HRESULT STDMETHODCALLTYPE memory_stream_activation_factory_GetIids( IActivationFactory *iface, ULONG *iid_count,
        IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE memory_stream_activation_factory_GetRuntimeClassName( IActivationFactory *iface,
        HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE memory_stream_activation_factory_GetTrustLevel( IActivationFactory *iface,
        TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE memory_stream_activation_factory_ActivateInstance( IActivationFactory *iface,
        IInspectable **instance )
{
    IRandomAccessStream *out;
    HRESULT hr;

    TRACE( "iface %p, instance %p.\n", iface, instance );

    *instance = NULL;

    if (SUCCEEDED(hr = memory_stream_create( &out )))
    {
        hr = IRandomAccessStream_QueryInterface( out, &IID_IInspectable, (void **)instance );
        IRandomAccessStream_Release( out );
        if (SUCCEEDED(hr))
            TRACE( "created InMemoryRandomAccessStream %p.\n", *instance );
    }

    return hr;
}

static const struct IActivationFactoryVtbl memory_stream_activation_factory_vtbl =
{
    memory_stream_activation_factory_QueryInterface,
    memory_stream_activation_factory_AddRef,
    memory_stream_activation_factory_Release,
    /* IInspectable methods */
    memory_stream_activation_factory_GetIids,
    memory_stream_activation_factory_GetRuntimeClassName,
    memory_stream_activation_factory_GetTrustLevel,
    /* IActivationFactory methods */
    memory_stream_activation_factory_ActivateInstance,
};

struct memory_stream_factory memory_stream_factory =
{
    {&memory_stream_activation_factory_vtbl},
    1
};

IActivationFactory *memory_stream_activation_factory = &memory_stream_factory.IActivationFactory_iface;
