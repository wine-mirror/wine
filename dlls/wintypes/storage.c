/*
 * Copyright 2022-2024 Zhiyi Zhang for CodeWeavers
 * Copyright 2025 Jactry Zeng for CodeWeavers
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

#include "private.h"
#include "robuffer.h"

WINE_DEFAULT_DEBUG_CHANNEL(wintypes);

/* Synchronous wrapper for async operations returned by IOutputStream async methods.
 * The IAsyncInfo object is queried from the inner operation. */

#define DEFINE_IASYNCOPERATION(iface_type, cs_type_str, impl_type, result_type,                                   \
        handler_iface_type, inner_op_iface_type, outer_handler_iface_type)                                        \
struct impl_type                                                                                                  \
{                                                                                                                 \
    iface_type iface_type##_iface;                                                                                \
    handler_iface_type handler_iface_type##_iface;                                                                \
    IAsyncInfo *async_inner;                                                                                      \
    LONG refcount;                                                                                                \
    struct data_writer *data_writer;                                                                              \
    outer_handler_iface_type *outer_handler;                                                                      \
    AsyncStatus status;                                                                                           \
    result_type result;                                                                                           \
};                                                                                                                \
                                                                                                                  \
static inline struct impl_type *impl_from_##handler_iface_type(handler_iface_type *iface)                         \
{                                                                                                                 \
    return CONTAINING_RECORD(iface, struct impl_type, handler_iface_type##_iface);                                \
}                                                                                                                 \
                                                                                                                  \
static HRESULT WINAPI impl_type##_handler_QueryInterface(handler_iface_type *iface, REFIID iid, void **out)       \
{                                                                                                                 \
    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IAgileObject) ||                                 \
        IsEqualGUID(iid, &IID_##handler_iface_type))                                                              \
    {                                                                                                             \
        IUnknown_AddRef(iface);                                                                                   \
        *out = iface;                                                                                             \
        return S_OK;                                                                                              \
    }                                                                                                             \
                                                                                                                  \
    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));                                   \
    *out = NULL;                                                                                                  \
    return E_NOINTERFACE;                                                                                         \
}                                                                                                                 \
                                                                                                                  \
static ULONG WINAPI impl_type##_handler_AddRef(handler_iface_type *iface)                                         \
{                                                                                                                 \
    struct impl_type *impl = impl_from_##handler_iface_type(iface);                                               \
    return iface_type##_AddRef(&impl->iface_type##_iface);                                                        \
}                                                                                                                 \
                                                                                                                  \
static ULONG WINAPI impl_type##_handler_Release(handler_iface_type *iface)                                        \
{                                                                                                                 \
    struct impl_type *impl = impl_from_##handler_iface_type(iface);                                               \
    return iface_type##_Release(&impl->iface_type##_iface);                                                       \
}                                                                                                                 \
                                                                                                                  \
static HRESULT impl_type##_handle_completion(struct impl_type *impl)                                              \
{                                                                                                                 \
    if (impl->status && impl->outer_handler)                                                                      \
        return outer_handler_iface_type##_Invoke(impl->outer_handler, &impl->iface_type##_iface, impl->status);   \
    return S_OK;                                                                                                  \
}                                                                                                                 \
static HRESULT WINAPI impl_type##_handler_Invoke(handler_iface_type *iface,                                       \
        inner_op_iface_type *operation, AsyncStatus status)                                                       \
{                                                                                                                 \
    struct impl_type *impl = impl_from_##handler_iface_type(iface);                                               \
    HRESULT hr;                                                                                                   \
    if (!status) return S_OK;                                                                                     \
    if (status == Completed && FAILED(hr = inner_op_iface_type##_GetResults(operation, &impl->result)))           \
        status = Error;                                                                                           \
    impl->status = status;                                                                                        \
    data_writer_async_complete(impl->data_writer);                                                                \
    return impl_type##_handle_completion(impl);                                                                   \
}                                                                                                                 \
                                                                                                                  \
static handler_iface_type##Vtbl impl_type##_handler_vtbl =                                                        \
{                                                                                                                 \
    impl_type##_handler_QueryInterface,                                                                           \
    impl_type##_handler_AddRef,                                                                                   \
    impl_type##_handler_Release,                                                                                  \
    impl_type##_handler_Invoke,                                                                                   \
};                                                                                                                \
                                                                                                                  \
static inline struct impl_type *impl_from_##iface_type(iface_type *iface)                                         \
{                                                                                                                 \
    return CONTAINING_RECORD(iface, struct impl_type, iface_type##_iface);                                        \
}                                                                                                                 \
static HRESULT WINAPI impl_type##_QueryInterface(iface_type *iface, REFIID iid, void **out)                       \
{                                                                                                                 \
    struct impl_type *impl = impl_from_##iface_type(iface);                                                       \
                                                                                                                  \
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);                                         \
                                                                                                                  \
    if (IsEqualGUID(iid, &IID_IUnknown) ||                                                                        \
        IsEqualGUID(iid, &IID_IInspectable) ||                                                                    \
        IsEqualGUID(iid, &IID_IAgileObject) ||                                                                    \
        IsEqualGUID(iid, &IID_##iface_type))                                                                      \
    {                                                                                                             \
        iface_type##_AddRef((*out = &impl->iface_type##_iface));                                                  \
        return S_OK;                                                                                              \
    }                                                                                                             \
                                                                                                                  \
    if (IsEqualGUID(iid, &IID_IAsyncInfo))                                                                        \
    {                                                                                                             \
        IAsyncInfo_AddRef(*out = impl->async_inner);                                                              \
        return S_OK;                                                                                              \
    }                                                                                                             \
                                                                                                                  \
    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));                                   \
    *out = NULL;                                                                                                  \
    return E_NOINTERFACE;                                                                                         \
}                                                                                                                 \
static ULONG WINAPI impl_type##_AddRef(iface_type *iface)                                                         \
{                                                                                                                 \
    struct impl_type *impl = impl_from_##iface_type(iface);                                                       \
    ULONG ref = InterlockedIncrement(&impl->refcount);                                                            \
    TRACE("iface %p, ref %lu.\n", iface, ref);                                                                    \
    return ref;                                                                                                   \
}                                                                                                                 \
static ULONG WINAPI impl_type##_Release(iface_type *iface)                                                        \
{                                                                                                                 \
    struct impl_type *impl = impl_from_##iface_type(iface);                                                       \
    ULONG ref = InterlockedDecrement(&impl->refcount);                                                            \
    TRACE("iface %p, ref %lu.\n", iface, ref);                                                                    \
                                                                                                                  \
    if (!ref)                                                                                                     \
    {                                                                                                             \
        IDataWriter_Release(&impl->data_writer->IDataWriter_iface);                                               \
        IAsyncInfo_Release(impl->async_inner);                                                                    \
        if (impl->outer_handler)                                                                                  \
            outer_handler_iface_type##_Release(impl->outer_handler);                                              \
        free(impl);                                                                                               \
    }                                                                                                             \
                                                                                                                  \
    return ref;                                                                                                   \
}                                                                                                                 \
static HRESULT WINAPI impl_type##_GetIids(iface_type *iface, ULONG *iid_count, IID **iids)                        \
{                                                                                                                 \
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);                                     \
    return E_NOTIMPL;                                                                                             \
}                                                                                                                 \
static HRESULT WINAPI impl_type##_GetRuntimeClassName(iface_type *iface, HSTRING *class_name)                     \
{                                                                                                                 \
    return WindowsCreateString(L"Windows.Foundation.IAsyncOperation`1<"cs_type_str">",                            \
                                ARRAY_SIZE(L"Windows.Foundation.IAsyncOperation`1<"cs_type_str">"),               \
                                class_name);                                                                      \
}                                                                                                                 \
static HRESULT WINAPI impl_type##_GetTrustLevel(iface_type *iface, TrustLevel *trust_level)                       \
{                                                                                                                 \
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);                                                \
    return E_NOTIMPL;                                                                                             \
}                                                                                                                 \
static HRESULT WINAPI impl_type##_put_Completed(iface_type *iface, outer_handler_iface_type *handler)             \
{                                                                                                                 \
    struct impl_type *impl = impl_from_##iface_type(iface);                                                       \
    TRACE("iface %p, handler %p.\n", iface, handler);                                                             \
    impl->outer_handler = handler;                                                                                \
    outer_handler_iface_type##_AddRef(impl->outer_handler);                                                       \
    return impl_type##_handle_completion(impl);                                                                   \
}                                                                                                                 \
static HRESULT WINAPI impl_type##_get_Completed(iface_type *iface, outer_handler_iface_type **handler)            \
{                                                                                                                 \
    struct impl_type *impl = impl_from_##iface_type(iface);                                                       \
    TRACE("iface %p, handler %p.\n", iface, handler);                                                             \
    if ((*handler = impl->outer_handler))                                                                         \
        outer_handler_iface_type##_AddRef(*handler);                                                              \
    return S_OK;                                                                                                  \
}                                                                                                                 \
static HRESULT WINAPI impl_type##_GetResults(iface_type *iface, result_type *results)                             \
{                                                                                                                 \
    struct impl_type *impl = impl_from_##iface_type(iface);                                                       \
                                                                                                                  \
    TRACE("iface %p, results %p.\n", iface, results);                                                             \
                                                                                                                  \
    if (impl->status != Completed)                                                                                \
        return E_ILLEGAL_METHOD_CALL;                                                                             \
                                                                                                                  \
    *results = impl->result;                                                                                      \
    return S_OK;                                                                                                  \
}                                                                                                                 \
static const struct iface_type##Vtbl impl_type##_vtbl =                                                           \
{                                                                                                                 \
    /* IUnknown methods */                                                                                        \
    impl_type##_QueryInterface,                                                                                   \
    impl_type##_AddRef,                                                                                           \
    impl_type##_Release,                                                                                          \
    /* IInspectable methods */                                                                                    \
    impl_type##_GetIids,                                                                                          \
    impl_type##_GetRuntimeClassName,                                                                              \
    impl_type##_GetTrustLevel,                                                                                    \
    /* IAsyncOperation<iface_type> */                                                                             \
    impl_type##_put_Completed,                                                                                    \
    impl_type##_get_Completed,                                                                                    \
    impl_type##_GetResults,                                                                                       \
};                                                                                                                \
HRESULT async_operation_##result_type##_create(struct data_writer *data_writer,                                   \
        inner_op_iface_type *inner_op, iface_type **out)                                                          \
{                                                                                                                 \
    struct impl_type *impl;                                                                                       \
    HRESULT hr;                                                                                                   \
                                                                                                                  \
    *out = NULL;                                                                                                  \
    if (!(impl = calloc(1, sizeof(*impl)))) return E_OUTOFMEMORY;                                                 \
                                                                                                                  \
    impl->iface_type##_iface.lpVtbl = &impl_type##_vtbl;                                                          \
    impl->handler_iface_type##_iface.lpVtbl = &impl_type##_handler_vtbl;                                          \
    impl->data_writer = data_writer;                                                                              \
    impl->refcount = 1;                                                                                           \
                                                                                                                  \
    if (FAILED(hr = inner_op_iface_type##_QueryInterface(inner_op, &IID_IAsyncInfo, (void **)&impl->async_inner)) \
            || FAILED(hr = inner_op_iface_type##_put_Completed(inner_op, &impl->handler_iface_type##_iface)))     \
    {                                                                                                             \
        if (impl->async_inner)                                                                                    \
            IAsyncInfo_Release(impl->async_inner);                                                                \
        free(impl);                                                                                               \
        return hr;                                                                                                \
    }                                                                                                             \
                                                                                                                  \
    IDataWriter_AddRef(&data_writer->IDataWriter_iface);                                                          \
                                                                                                                  \
    *out = &impl->iface_type##_iface;                                                                             \
    TRACE("created IAsyncOperation %p\n", *out);                                                                  \
    return S_OK;                                                                                                  \
}                                                                                                                 \

static HRESULT buffer_create(UINT32 capacity, IBuffer **out)
{
    IBufferFactory *buffer_factory;
    HRESULT hr;

    IActivationFactory_QueryInterface(buffer_activation_factory, &IID_IBufferFactory, (void **)&buffer_factory);
    hr = IBufferFactory_Create(buffer_factory, capacity, out);
    IBufferFactory_Release(buffer_factory);
    return hr;
}

struct data_writer
{
    IDataWriter IDataWriter_iface;
    LONG ref;

    IOutputStream *stream;
    IBuffer *buffer;
    byte *data;
    BOOL storing;
};

static HRESULT data_writer_async_complete(struct data_writer *impl);

DEFINE_IASYNCOPERATION(IAsyncOperation_UINT32, "IAsyncOperation<UInt32>", async_uint32, UINT32, \
        IAsyncOperationWithProgressCompletedHandler_UINT32_UINT32, \
        IAsyncOperationWithProgress_UINT32_UINT32, \
        IAsyncOperationCompletedHandler_UINT32)

static HRESULT data_writer_init_buffer(struct data_writer *impl, UINT32 extra_capacity)
{
    /* Native capacity starts at 0x88 */
    UINT32 new_capacity, capacity = max(extra_capacity, 0x88), pos = 0;
    IBufferByteAccess *access;
    IBuffer *buffer;
    HRESULT hr;
    byte *data;

    if (impl->buffer)
    {
        IBuffer_get_Capacity(impl->buffer, &capacity);
        IBuffer_get_Length(impl->buffer, &pos);

        new_capacity = pos + extra_capacity;
        if (new_capacity < pos || new_capacity < extra_capacity)
            return E_OUTOFMEMORY;
        if (new_capacity <= capacity)
            return S_OK;

        /* If allocation size grows by 50%, the sixth allocation can fit in freed
         * memory of the first four if they were contiguous. This matches native. */
        capacity = max(new_capacity, capacity + capacity / 2u);
    }

    if (FAILED(hr = buffer_create(capacity, &buffer)))
        return hr;

    IBuffer_QueryInterface(buffer, &IID_IBufferByteAccess, (void **)&access);
    IBufferByteAccess_Buffer(access, &data);
    IBufferByteAccess_Release(access);

    if (impl->buffer)
    {
        memcpy(data, impl->data, pos);
        IBuffer_Release(impl->buffer);
    }

    impl->buffer = buffer;
    impl->data = data;

    return S_OK;
}

static struct data_writer *impl_from_IDataWriter(IDataWriter *iface)
{
    return CONTAINING_RECORD(iface, struct data_writer, IDataWriter_iface);
}

static HRESULT WINAPI data_writer_QueryInterface(IDataWriter *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown)
            || IsEqualGUID(iid, &IID_IInspectable)
            || IsEqualGUID(iid, &IID_IAgileObject)
            || IsEqualGUID(iid, &IID_IDataWriter))
    {
        *out = iface;
        IDataWriter_AddRef(iface);
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI data_writer_AddRef(IDataWriter *iface)
{
    struct data_writer *impl = impl_from_IDataWriter(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI data_writer_Release(IDataWriter *iface)
{
    struct data_writer *impl = impl_from_IDataWriter(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);

    TRACE("iface %p, ref %lu.\n", iface, ref);

    if (!ref)
    {
        if (impl->stream)
        {
            IClosable *closable;
            HRESULT hr;

            if (SUCCEEDED(hr = IOutputStream_QueryInterface(impl->stream, &IID_IClosable, (void **)&closable)))
            {
                hr = IClosable_Close(closable);
                IClosable_Release(closable);
            }

            if (FAILED(hr))
                WARN("Failed to close stream, hr %#lx.\n", hr);

            IOutputStream_Release(impl->stream);
        }
        if (impl->buffer)
            IBuffer_Release(impl->buffer);
        free(impl);
    }

    return ref;
}

static HRESULT WINAPI data_writer_GetIids(IDataWriter *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI data_writer_GetRuntimeClassName(IDataWriter *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI data_writer_GetTrustLevel(IDataWriter *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI data_writer_get_UnstoredBufferLength(IDataWriter *iface, UINT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI data_writer_get_UnicodeEncoding(IDataWriter *iface, UnicodeEncoding *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI data_writer_put_UnicodeEncoding(IDataWriter *iface, UnicodeEncoding value)
{
    FIXME("iface %p, value %u stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI data_writer_get_ByteOrder(IDataWriter *iface, ByteOrder *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI data_writer_put_ByteOrder(IDataWriter *iface, ByteOrder value)
{
    FIXME("iface %p, value %u stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI data_writer_WriteByte(IDataWriter *iface, BYTE value)
{
    FIXME("iface %p, value %u stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI data_writer_WriteBytes(IDataWriter *iface, UINT32 value_size, BYTE *value)
{
    struct data_writer *impl = impl_from_IDataWriter(iface);
    UINT32 pos = 0;
    HRESULT hr;

    TRACE("iface %p, value_size %u, value %p.\n", iface, value_size, value);

    if (FAILED(hr = data_writer_init_buffer(impl, value_size)))
        return hr;

    IBuffer_get_Length(impl->buffer, &pos);
    memcpy(&impl->data[pos], value, value_size);
    IBuffer_put_Length(impl->buffer, pos + value_size);

    return hr;
}

static HRESULT WINAPI data_writer_WriteBuffer(IDataWriter *iface, IBuffer *buffer)
{
    FIXME("iface %p, buffer %p stub!\n", iface, buffer);
    return E_NOTIMPL;
}

static HRESULT WINAPI data_writer_WriteBufferRange(IDataWriter *iface, IBuffer *buffer, UINT32 start, UINT32 count)
{
    FIXME("iface %p, buffer %p, start %u, count %u stub!\n", iface, buffer, start, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI data_writer_WriteBoolean(IDataWriter *iface, boolean value)
{
    FIXME("iface %p, value %u stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI data_writer_WriteGuid(IDataWriter *iface, GUID value)
{
    FIXME("iface %p, value %s stub!\n", iface, debugstr_guid(&value));
    return E_NOTIMPL;
}

static HRESULT WINAPI data_writer_WriteInt16(IDataWriter *iface, INT16 value)
{
    FIXME("iface %p, value %u stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI data_writer_WriteInt32(IDataWriter *iface, INT32 value)
{
    FIXME("iface %p, value %u stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI data_writer_WriteInt64(IDataWriter *iface, INT64 value)
{
    FIXME("iface %p, value %I64d stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI data_writer_WriteUInt16(IDataWriter *iface, UINT16 value)
{
    FIXME("iface %p, value %u stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI data_writer_WriteUInt32(IDataWriter *iface, UINT32 value)
{
    FIXME("iface %p, value %u stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI data_writer_WriteUInt64(IDataWriter *iface, UINT64 value)
{
    FIXME("iface %p, value %I64u stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI data_writer_WriteSingle(IDataWriter *iface, FLOAT value)
{
    FIXME("iface %p, value %.7f stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI data_writer_WriteDouble(IDataWriter *iface, DOUBLE value)
{
    FIXME("iface %p, value %.15f stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI data_writer_WriteDateTime(IDataWriter *iface, DateTime value)
{
    FIXME("iface %p, value %I64u stub!\n", iface, value.UniversalTime);
    return E_NOTIMPL;
}

static HRESULT WINAPI data_writer_WriteTimeSpan(IDataWriter *iface, TimeSpan value)
{
    FIXME("iface %p, value %I64u stub!\n", iface, value.Duration);
    return E_NOTIMPL;
}

static HRESULT WINAPI data_writer_WriteString(IDataWriter *iface, HSTRING value, UINT32 *code_unit_count)
{
    FIXME("iface %p, value %p, code_unit_count %p stub!\n", iface, value, code_unit_count);
    return E_NOTIMPL;
}

static HRESULT WINAPI data_writer_MeasureString(IDataWriter *iface, HSTRING value, UINT32 *code_unit_count)
{
    FIXME("iface %p, value %p, code_unit_count %p stub!\n", iface, value, code_unit_count);
    return E_NOTIMPL;
}

static HRESULT data_writer_async_complete(struct data_writer *impl)
{
    impl->storing = FALSE;
    return S_OK;
}

static HRESULT WINAPI data_writer_StoreAsync(IDataWriter *iface, IAsyncOperation_UINT32 **operation)
{
    IAsyncOperationWithProgress_UINT32_UINT32 *inner_operation;
    struct data_writer *impl = impl_from_IDataWriter(iface);
    HRESULT hr;

    TRACE("iface %p, operation %p.\n", iface, operation);

    *operation = NULL;

    if (!impl->stream)
        return HRESULT_FROM_WIN32(ERROR_INVALID_OPERATION);

    if (FAILED(hr = IOutputStream_WriteAsync(impl->stream, impl->buffer, &inner_operation)))
        return hr;
    IBuffer_Release(impl->buffer);
    impl->buffer = NULL;
    impl->storing = TRUE;

    hr = async_operation_UINT32_create(impl, inner_operation, operation);
    IAsyncOperationWithProgress_UINT32_UINT32_Release(inner_operation);

    if (SUCCEEDED(hr))
        hr = data_writer_init_buffer(impl, 0);

    return hr;
}

static HRESULT WINAPI data_writer_FlushAsync(IDataWriter *iface, IAsyncOperation_boolean **operation)
{
    struct data_writer *impl = impl_from_IDataWriter(iface);

    FIXME("iface %p, operation %p stub!\n", iface, operation);

    *operation = NULL;

    if (!impl->stream)
        return HRESULT_FROM_WIN32(ERROR_INVALID_OPERATION);

    return E_NOTIMPL;
}

static HRESULT WINAPI data_writer_DetachBuffer(IDataWriter *iface, IBuffer **buffer)
{
    struct data_writer *impl = impl_from_IDataWriter(iface);

    TRACE("iface %p, buffer %p.\n", iface, buffer);

    *buffer = impl->buffer;
    impl->buffer = NULL;
    return data_writer_init_buffer(impl, 0);
}

static HRESULT WINAPI data_writer_DetachStream(IDataWriter *iface, IOutputStream **output_stream)
{
    struct data_writer *impl = impl_from_IDataWriter(iface);

    FIXME("iface %p, output_stream %p stub!\n", iface, output_stream);

    *output_stream = NULL;

    if (impl->storing)
        return HRESULT_FROM_WIN32(ERROR_INVALID_OPERATION);

    return E_NOTIMPL;
}

static const struct IDataWriterVtbl data_writer_vtbl =
{
    /* IUnknown methods */
    data_writer_QueryInterface,
    data_writer_AddRef,
    data_writer_Release,
    /* IInspectable methods */
    data_writer_GetIids,
    data_writer_GetRuntimeClassName,
    data_writer_GetTrustLevel,
    /* IDataWriter */
    data_writer_get_UnstoredBufferLength,
    data_writer_get_UnicodeEncoding,
    data_writer_put_UnicodeEncoding,
    data_writer_get_ByteOrder,
    data_writer_put_ByteOrder,
    data_writer_WriteByte,
    data_writer_WriteBytes,
    data_writer_WriteBuffer,
    data_writer_WriteBufferRange,
    data_writer_WriteBoolean,
    data_writer_WriteGuid,
    data_writer_WriteInt16,
    data_writer_WriteInt32,
    data_writer_WriteInt64,
    data_writer_WriteUInt16,
    data_writer_WriteUInt32,
    data_writer_WriteUInt64,
    data_writer_WriteSingle,
    data_writer_WriteDouble,
    data_writer_WriteDateTime,
    data_writer_WriteTimeSpan,
    data_writer_WriteString,
    data_writer_MeasureString,
    data_writer_StoreAsync,
    data_writer_FlushAsync,
    data_writer_DetachBuffer,
    data_writer_DetachStream,
};

static HRESULT data_writer_create(IOutputStream *output_stream, IDataWriter **out)
{
    struct data_writer *impl;
    HRESULT hr;

    *out = NULL;
    if (!(impl = calloc(1, sizeof(*impl))))
        return E_OUTOFMEMORY;

    impl->IDataWriter_iface.lpVtbl = &data_writer_vtbl;
    impl->stream = output_stream;
    impl->ref = 1;

    if (FAILED(hr = data_writer_init_buffer(impl, 0)))
    {
        free(impl);
        return hr;
    }

    if (output_stream)
        IOutputStream_AddRef(output_stream);

    *out = &impl->IDataWriter_iface;
    return S_OK;
}

struct data_writer_factory
{
    IActivationFactory IActivationFactory_iface;
    IDataWriterFactory IDataWriterFactory_iface;
    LONG ref;
};

static inline struct data_writer_factory *impl_data_writer_factory_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct data_writer_factory, IActivationFactory_iface);
}

static inline struct data_writer_factory *impl_data_writer_factory_from_IDataWriterFactory(IDataWriterFactory *iface)
{
    return CONTAINING_RECORD(iface, struct data_writer_factory, IDataWriterFactory_iface);
}

static HRESULT STDMETHODCALLTYPE data_writer_activation_factory_QueryInterface(IActivationFactory *iface, REFIID iid,
        void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown)
            || IsEqualGUID(iid, &IID_IInspectable)
            || IsEqualGUID(iid, &IID_IAgileObject)
            || IsEqualGUID(iid, &IID_IActivationFactory))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_IDataWriterFactory))
    {
        struct data_writer_factory *impl = impl_data_writer_factory_from_IActivationFactory(iface);
        IDataWriterFactory_AddRef(&impl->IDataWriterFactory_iface);
        *out = &impl->IDataWriterFactory_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE data_writer_activation_factory_AddRef(IActivationFactory *iface)
{
    struct data_writer_factory *impl = impl_data_writer_factory_from_IActivationFactory(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE data_writer_activation_factory_Release(IActivationFactory *iface)
{
    struct data_writer_factory *impl = impl_data_writer_factory_from_IActivationFactory(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static HRESULT STDMETHODCALLTYPE data_writer_activation_factory_GetIids(IActivationFactory *iface, ULONG *iid_count,
        IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE data_writer_activation_factory_GetRuntimeClassName(IActivationFactory *iface,
        HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE data_writer_activation_factory_GetTrustLevel(IActivationFactory *iface,
        TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE data_writer_activation_factory_ActivateInstance(IActivationFactory *iface,
        IInspectable **instance)
{
    IDataWriter *data_writer;
    HRESULT hr;

    TRACE("iface %p, instance %p.\n", iface, instance);

    if (FAILED(hr = data_writer_create(NULL, &data_writer)))
        return hr;

    hr = IDataWriter_QueryInterface(data_writer, &IID_IInspectable, (void **)instance);
    IDataWriter_Release(data_writer);

    return hr;
}

static const struct IActivationFactoryVtbl data_writer_activation_factory_vtbl =
{
    data_writer_activation_factory_QueryInterface,
    data_writer_activation_factory_AddRef,
    data_writer_activation_factory_Release,
    /* IInspectable methods */
    data_writer_activation_factory_GetIids,
    data_writer_activation_factory_GetRuntimeClassName,
    data_writer_activation_factory_GetTrustLevel,
    /* IActivationFactory methods */
    data_writer_activation_factory_ActivateInstance,
};

static HRESULT STDMETHODCALLTYPE data_writer_factory_QueryInterface(IDataWriterFactory *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown)
            || IsEqualGUID(iid, &IID_IInspectable)
            || IsEqualGUID(iid, &IID_IAgileObject)
            || IsEqualGUID(iid, &IID_IDataWriterFactory))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_IActivationFactory))
    {
        struct data_writer_factory *impl = impl_data_writer_factory_from_IDataWriterFactory(iface);
        IActivationFactory_AddRef(&impl->IActivationFactory_iface);
        *out = &impl->IActivationFactory_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE data_writer_factory_AddRef(IDataWriterFactory *iface)
{
    struct data_writer_factory *impl = impl_data_writer_factory_from_IDataWriterFactory(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE data_writer_factory_Release(IDataWriterFactory *iface)
{
    struct data_writer_factory *impl = impl_data_writer_factory_from_IDataWriterFactory(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static HRESULT STDMETHODCALLTYPE data_writer_factory_GetIids(IDataWriterFactory *iface, ULONG *iid_count,
        IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE data_writer_factory_GetRuntimeClassName(IDataWriterFactory *iface,
        HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE data_writer_factory_GetTrustLevel(IDataWriterFactory *iface,
        TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE data_writer_factory_CreateDataWriter(IDataWriterFactory *iface,
        IOutputStream *output_stream, IDataWriter **data_writer)
{
    TRACE("iface %p, output_stream %p, data_writer %p.\n", iface, output_stream, data_writer);
    return data_writer_create(output_stream, data_writer);
}

static const struct IDataWriterFactoryVtbl data_writer_factory_vtbl =
{
    data_writer_factory_QueryInterface,
    data_writer_factory_AddRef,
    data_writer_factory_Release,
    /* IInspectable methods */
    data_writer_factory_GetIids,
    data_writer_factory_GetRuntimeClassName,
    data_writer_factory_GetTrustLevel,
    /* IDataWriterFactory methods */
    data_writer_factory_CreateDataWriter,
};

struct data_writer_factory data_writer_factory =
{
    {&data_writer_activation_factory_vtbl},
    {&data_writer_factory_vtbl},
    1
};

IActivationFactory *data_writer_activation_factory = &data_writer_factory.IActivationFactory_iface;
