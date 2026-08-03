/* WinRT Windows.Storage DataReader Implementation
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

WINE_DEFAULT_DEBUG_CHANNEL(datareader);

struct datareader
{
    IDataReader IDataReader_iface;
    IBuffer *buffer;
    IBufferByteAccess *buffer_access;
    UINT32 position;
    LONG ref;
};

static inline struct datareader *impl_from_IDataReader( IDataReader *iface )
{
    return CONTAINING_RECORD( iface, struct datareader, IDataReader_iface );
}

static HRESULT WINAPI datareader_QueryInterface( IDataReader *iface, REFIID iid, void **out )
{
    struct datareader *impl = impl_from_IDataReader( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IDataReader ))
    {
        *out = &impl->IDataReader_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI datareader_AddRef( IDataReader *iface )
{
    struct datareader *impl = impl_from_IDataReader( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI datareader_Release( IDataReader *iface )
{
    struct datareader *impl = impl_from_IDataReader( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI datareader_GetIids( IDataReader *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI datareader_GetRuntimeClassName( IDataReader *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI datareader_GetTrustLevel( IDataReader *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI datareader_get_UnconsumedBufferLength( IDataReader *iface, UINT32 *value )
{
    HRESULT ret;
    struct datareader *impl = impl_from_IDataReader( iface );

    TRACE( "iface %p, value %p\n", iface, value );
    ret = impl->buffer->lpVtbl->get_Length(impl->buffer, value);
    if (!SUCCEEDED(ret)) {
        return ret;
    }

    *value = *value - impl->position;
    return S_OK;
}

static HRESULT WINAPI datareader_get_UnicodeEncoding( IDataReader *iface, UnicodeEncoding *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    *value = UnicodeEncoding_Utf8;
    return S_OK;
}

static HRESULT WINAPI datareader_put_UnicodeEncoding( IDataReader *iface, UnicodeEncoding value )
{
    FIXME( "iface %p, value %d stub!\n", iface, value );
    return S_OK;
}

static HRESULT WINAPI datareader_get_ByteOrder( IDataReader *iface, ByteOrder *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    *value = ByteOrder_LittleEndian;
    return S_OK;
}

static HRESULT WINAPI datareader_put_ByteOrder( IDataReader *iface, ByteOrder value )
{
    FIXME( "iface %p, value %d stub!\n", iface, value );
    return S_OK;
}

static HRESULT WINAPI datareader_get_InputStreamOptions( IDataReader *iface, InputStreamOptions *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    *value = InputStreamOptions_None;
    return S_OK;
}

static HRESULT WINAPI datareader_put_InputStreamOptions( IDataReader *iface, InputStreamOptions value )
{
    FIXME( "iface %p, value %d stub!\n", iface, value );
    return S_OK;
}

static HRESULT WINAPI datareader_ReadBytes( IDataReader *iface, UINT32 __valueSize, BYTE *value )
{
    HRESULT ret;
    UINT32 remaining;
    byte *buf_ptr;
    struct datareader *impl = impl_from_IDataReader( iface );

    TRACE( "iface %p, __valueSize %d, value %p\n", iface, __valueSize, value );

    ret = datareader_get_UnconsumedBufferLength(iface, &remaining);
    if (!SUCCEEDED(ret)) {
        return ret;
    }

    if (__valueSize > remaining) {
        ERR("value size %u, we only have %u", __valueSize, remaining);
        return E_INVALIDARG;
    } 

    ret = impl->buffer_access->lpVtbl->get_Buffer(impl->buffer_access, &buf_ptr);
    if (!SUCCEEDED(ret)) {
        return ret;
    }

    impl->position = + __valueSize;
    memcpy(value, buf_ptr, __valueSize);

    return S_OK;
}

static HRESULT WINAPI datareader_ReadByte( IDataReader *iface, BYTE *value )
{
    return datareader_ReadBytes(iface, sizeof(BYTE), value);
}

static HRESULT WINAPI datareader_ReadBuffer( IDataReader *iface, UINT32 length, IBuffer** buffer )
{
    FIXME( "iface %p, length %d, buffer %p stub!\n", iface, length, buffer );
    return E_NOTIMPL;
}

static HRESULT WINAPI datareader_ReadBoolean( IDataReader *iface, boolean *value )
{
    return datareader_ReadBytes(iface, sizeof(boolean), (BYTE*)value);
}

static HRESULT WINAPI datareader_ReadGuid( IDataReader *iface, GUID *value )
{
    return datareader_ReadBytes(iface, sizeof(GUID), (BYTE*)value);
}

static HRESULT WINAPI datareader_ReadInt16( IDataReader *iface, INT16 *value )
{
    return datareader_ReadBytes(iface, sizeof(INT16), (BYTE*)value);
}

static HRESULT WINAPI datareader_ReadInt32( IDataReader *iface, INT32 *value )
{
    return datareader_ReadBytes(iface, sizeof(INT32), (BYTE*)value);
}

static HRESULT WINAPI datareader_ReadInt64( IDataReader *iface, INT64 *value )
{
    return datareader_ReadBytes(iface, sizeof(INT64), (BYTE*)value);
}

static HRESULT WINAPI datareader_ReadUInt16( IDataReader *iface, UINT16 *value )
{
    return datareader_ReadBytes(iface, sizeof(UINT16), (BYTE*)value);
}

static HRESULT WINAPI datareader_ReadUInt32( IDataReader *iface, UINT32 *value )
{
    return datareader_ReadBytes(iface, sizeof(UINT32), (BYTE*)value);
}

static HRESULT WINAPI datareader_ReadUInt64( IDataReader *iface, UINT64 *value )
{
    return datareader_ReadBytes(iface, sizeof(UINT64), (BYTE*)value);
}

static HRESULT WINAPI datareader_ReadSingle( IDataReader *iface, FLOAT *value )
{
    return datareader_ReadBytes(iface, sizeof(FLOAT), (BYTE*)value);
}

static HRESULT WINAPI datareader_ReadDouble( IDataReader *iface, DOUBLE *value )
{
    return datareader_ReadBytes(iface, sizeof(DOUBLE), (BYTE*)value);
}

static HRESULT WINAPI datareader_ReadString( IDataReader *iface, UINT32 codeUnitCount, HSTRING* value)
{
    FIXME( "iface %p, codeUnitCount %u, value %p stub!\n", iface, codeUnitCount, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI datareader_ReadDateTime( IDataReader *iface, DateTime *value )
{
    return datareader_ReadBytes(iface, sizeof(DateTime), (BYTE*)value);
}

static HRESULT WINAPI datareader_ReadTimeSpan( IDataReader *iface, TimeSpan *value )
{
    return datareader_ReadBytes(iface, sizeof(TimeSpan), (BYTE*)value);
}

static HRESULT WINAPI datareader_LoadAsync( IDataReader *iface, UINT32 count, IAsyncOperation_UINT32** operation )
{
    FIXME( "iface %p, count %u, operation %p stub!\n", iface, count, operation );
    return E_NOTIMPL;
}

static HRESULT WINAPI datareader_DetachBuffer( IDataReader *iface, IBuffer** buffer )
{
    FIXME( "iface %p, buffer %p stub!\n", iface, buffer );
    return E_NOTIMPL;
}

static HRESULT WINAPI datareader_DetachStream( IDataReader *iface, __x_ABI_CWindows_CStorage_CStreams_CIInputStream** stream )
{
    FIXME( "iface %p, stream %p stub!\n", iface, stream );
    return E_NOTIMPL;
}

static const struct IDataReaderVtbl datareader_vtbl =
{
    datareader_QueryInterface,
    datareader_AddRef,
    datareader_Release,
    /* IInspectable methods */
    datareader_GetIids,
    datareader_GetRuntimeClassName,
    datareader_GetTrustLevel,
    /* IDataReader methods */
    datareader_get_UnconsumedBufferLength,
    datareader_get_UnicodeEncoding,
    datareader_put_UnicodeEncoding,
    datareader_get_ByteOrder,
    datareader_put_ByteOrder,
    datareader_get_InputStreamOptions,
    datareader_put_InputStreamOptions,
    datareader_ReadByte,
    datareader_ReadBytes,
    datareader_ReadBuffer,
    datareader_ReadBoolean,
    datareader_ReadGuid,
    datareader_ReadInt16,
    datareader_ReadInt32,
    datareader_ReadInt64,
    datareader_ReadUInt16,
    datareader_ReadUInt32,
    datareader_ReadUInt64,
    datareader_ReadSingle,
    datareader_ReadDouble,
    datareader_ReadString,
    datareader_ReadDateTime,
    datareader_ReadTimeSpan,
    datareader_LoadAsync,
    datareader_DetachBuffer,
    datareader_DetachStream
};

IDataReader *create_data_reader(IBuffer *buffer) {
    HRESULT ret;
    struct datareader *object;

    if (!(object = calloc(1, sizeof(*object))))
        return NULL;

    object->IDataReader_iface.lpVtbl = &datareader_vtbl;
    object->buffer = buffer;

    ret = IBuffer_QueryInterface(buffer, &IID_IBufferByteAccess, (void**)&object->buffer_access);
    if (!SUCCEEDED(ret)) {
        free(object);
        return NULL;
    }

    object->ref = 1;

    return &object->IDataReader_iface;
}

struct datareader_statics
{
    IActivationFactory IActivationFactory_iface;
    IDataReaderStatics IDataReaderStatics_iface;
    LONG ref;
};

static inline struct datareader_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct datareader_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct datareader_statics *impl = impl_from_IActivationFactory( iface );

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

    if (IsEqualGUID( iid, &IID_IDataReaderStatics ))
    {
        *out = &impl->IDataReaderStatics_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct datareader_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct datareader_statics *impl = impl_from_IActivationFactory( iface );
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

static inline struct datareader_statics *impl_from_IDataReaderStatics( IDataReaderStatics *iface )
{
    return CONTAINING_RECORD( iface, struct datareader_statics, IDataReaderStatics_iface );
}

static HRESULT WINAPI datareaderstatics_QueryInterface( IDataReaderStatics *iface, REFIID iid, void **out )
{
    struct datareader_statics *impl = impl_from_IDataReaderStatics( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IDataReaderStatics ))
    {
        *out = &impl->IDataReaderStatics_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI datareaderstatics_AddRef( IDataReaderStatics *iface )
{
    struct datareader_statics *impl = impl_from_IDataReaderStatics( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI datareaderstatics_Release( IDataReaderStatics *iface )
{
    struct datareader_statics *impl = impl_from_IDataReaderStatics( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI datareaderstatics_GetIids( IDataReaderStatics *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI datareaderstatics_GetRuntimeClassName( IDataReaderStatics *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI datareaderstatics_GetTrustLevel( IDataReaderStatics *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI datareaderstatics_FromBuffer( IDataReaderStatics *iface, IBuffer *buffer, IDataReader **dataReader)
{
    TRACE( "iface %p, buffer %p, dataReader %p\n", iface, buffer, dataReader );
    *dataReader = create_data_reader(buffer);
    return S_OK;
}

static const struct IDataReaderStaticsVtbl datareaderstatics_vtbl =
{
    datareaderstatics_QueryInterface,
    datareaderstatics_AddRef,
    datareaderstatics_Release,
    /* IInspectable methods */
    datareaderstatics_GetIids,
    datareaderstatics_GetRuntimeClassName,
    datareaderstatics_GetTrustLevel,
    /* IDataReaderStatics methods */
    datareaderstatics_FromBuffer,
};

static struct datareader_statics datareader_statics =
{
    .IActivationFactory_iface.lpVtbl = &factory_vtbl,
    .IDataReaderStatics_iface.lpVtbl = &datareaderstatics_vtbl,
    .ref = 1,
};

IActivationFactory *datareader_factory = &datareader_statics.IActivationFactory_iface;
