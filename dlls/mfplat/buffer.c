/*
 * Copyright 2018 Alistair Leslie-Hughes
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

#include "mfplat_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

struct memory_buffer
{
    IMFMediaBuffer IMFMediaBuffer_iface;
    LONG refcount;

    BYTE *data;
    DWORD max_length;
    DWORD current_length;
};

enum sample_prop_flags
{
    SAMPLE_PROP_HAS_DURATION  = 1 << 0,
    SAMPLE_PROP_HAS_TIMESTAMP = 1 << 1,
};

struct sample
{
    struct attributes attributes;
    IMFSample IMFSample_iface;

    IMFMediaBuffer **buffers;
    size_t buffer_count;
    size_t capacity;
    DWORD flags;
    CRITICAL_SECTION cs;
    DWORD prop_flags;
    LONGLONG duration;
    LONGLONG timestamp;
};

static inline struct memory_buffer *impl_from_IMFMediaBuffer(IMFMediaBuffer *iface)
{
    return CONTAINING_RECORD(iface, struct memory_buffer, IMFMediaBuffer_iface);
}

static inline struct sample *impl_from_IMFSample(IMFSample *iface)
{
    return CONTAINING_RECORD(iface, struct sample, IMFSample_iface);
}

static HRESULT WINAPI memory_buffer_QueryInterface(IMFMediaBuffer *iface, REFIID riid, void **out)
{
    struct memory_buffer *buffer = impl_from_IMFMediaBuffer(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFMediaBuffer) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &buffer->IMFMediaBuffer_iface;
    }
    else
    {
        FIXME("(%s, %p)\n", debugstr_guid(riid), out);
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*out);
    return S_OK;
}

static ULONG WINAPI memory_buffer_AddRef(IMFMediaBuffer *iface)
{
    struct memory_buffer *buffer = impl_from_IMFMediaBuffer(iface);
    ULONG refcount = InterlockedIncrement(&buffer->refcount);

    TRACE("%p, refcount %u.\n", buffer, refcount);

    return refcount;
}

static ULONG WINAPI memory_buffer_Release(IMFMediaBuffer *iface)
{
    struct memory_buffer *buffer = impl_from_IMFMediaBuffer(iface);
    ULONG refcount = InterlockedDecrement(&buffer->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        heap_free(buffer->data);
        heap_free(buffer);
    }

    return refcount;
}

static HRESULT WINAPI memory_buffer_Lock(IMFMediaBuffer *iface, BYTE **data, DWORD *max_length, DWORD *current_length)
{
    struct memory_buffer *buffer = impl_from_IMFMediaBuffer(iface);

    TRACE("%p, %p %p, %p.\n", iface, data, max_length, current_length);

    if (!data)
        return E_INVALIDARG;

    *data = buffer->data;
    if (max_length)
        *max_length = buffer->max_length;
    if (current_length)
        *current_length = buffer->current_length;

    return S_OK;
}

static HRESULT WINAPI memory_buffer_Unlock(IMFMediaBuffer *iface)
{
    TRACE("%p.\n", iface);

    return S_OK;
}

static HRESULT WINAPI memory_buffer_GetCurrentLength(IMFMediaBuffer *iface, DWORD *current_length)
{
    struct memory_buffer *buffer = impl_from_IMFMediaBuffer(iface);

    TRACE("%p.\n", iface);

    if (!current_length)
        return E_INVALIDARG;

    *current_length = buffer->current_length;

    return S_OK;
}

static HRESULT WINAPI memory_buffer_SetCurrentLength(IMFMediaBuffer *iface, DWORD current_length)
{
    struct memory_buffer *buffer = impl_from_IMFMediaBuffer(iface);

    TRACE("%p, %u.\n", iface, current_length);

    if (current_length > buffer->max_length)
        return E_INVALIDARG;

    buffer->current_length = current_length;

    return S_OK;
}

static HRESULT WINAPI memory_buffer_GetMaxLength(IMFMediaBuffer *iface, DWORD *max_length)
{
    struct memory_buffer *buffer = impl_from_IMFMediaBuffer(iface);

    TRACE("%p, %p.\n", iface, max_length);

    if (!max_length)
        return E_INVALIDARG;

    *max_length = buffer->max_length;

    return S_OK;
}

static const IMFMediaBufferVtbl memorybuffervtbl =
{
    memory_buffer_QueryInterface,
    memory_buffer_AddRef,
    memory_buffer_Release,
    memory_buffer_Lock,
    memory_buffer_Unlock,
    memory_buffer_GetCurrentLength,
    memory_buffer_SetCurrentLength,
    memory_buffer_GetMaxLength,
};

static HRESULT create_memory_buffer(DWORD max_length, DWORD alignment, IMFMediaBuffer **buffer)
{
    struct memory_buffer *object;

    if (!buffer)
        return E_INVALIDARG;

    object = heap_alloc(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->data = heap_alloc((max_length + alignment) & ~alignment);
    if (!object->data)
    {
        heap_free(object);
        return E_OUTOFMEMORY;
    }

    object->IMFMediaBuffer_iface.lpVtbl = &memorybuffervtbl;
    object->refcount = 1;
    object->max_length = max_length;
    object->current_length = 0;

    *buffer = &object->IMFMediaBuffer_iface;

    return S_OK;
}

/***********************************************************************
 *      MFCreateMemoryBuffer (mfplat.@)
 */
HRESULT WINAPI MFCreateMemoryBuffer(DWORD max_length, IMFMediaBuffer **buffer)
{
    TRACE("%u, %p.\n", max_length, buffer);

    return create_memory_buffer(max_length, MF_1_BYTE_ALIGNMENT, buffer);
}

/***********************************************************************
 *      MFCreateAlignedMemoryBuffer (mfplat.@)
 */
HRESULT WINAPI MFCreateAlignedMemoryBuffer(DWORD max_length, DWORD alignment, IMFMediaBuffer **buffer)
{
    TRACE("%u, %u, %p.\n", max_length, alignment, buffer);

    return create_memory_buffer(max_length, alignment, buffer);
}

static HRESULT WINAPI sample_QueryInterface(IMFSample *iface, REFIID riid, void **out)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFSample) ||
            IsEqualIID(riid, &IID_IMFAttributes) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
        IMFSample_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI sample_AddRef(IMFSample *iface)
{
    struct sample *sample = impl_from_IMFSample(iface);
    ULONG refcount = InterlockedIncrement(&sample->attributes.ref);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI sample_Release(IMFSample *iface)
{
    struct sample *sample = impl_from_IMFSample(iface);
    ULONG refcount = InterlockedDecrement(&sample->attributes.ref);
    size_t i;

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        for (i = 0; i < sample->buffer_count; ++i)
            IMFMediaBuffer_Release(sample->buffers[i]);
        clear_attributes_object(&sample->attributes);
        DeleteCriticalSection(&sample->cs);
        heap_free(sample->buffers);
        heap_free(sample);
    }

    return refcount;
}

static HRESULT WINAPI sample_GetItem(IMFSample *iface, REFGUID key, PROPVARIANT *value)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetItem(&sample->attributes, key, value);
}

static HRESULT WINAPI sample_GetItemType(IMFSample *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), type);

    return attributes_GetItemType(&sample->attributes, key, type);
}

static HRESULT WINAPI sample_CompareItem(IMFSample *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_attr(key), debugstr_propvar(value), result);

    return attributes_CompareItem(&sample->attributes, key, value, result);
}

static HRESULT WINAPI sample_Compare(IMFSample *iface, IMFAttributes *theirs, MF_ATTRIBUTES_MATCH_TYPE type,
        BOOL *result)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %p, %d, %p.\n", iface, theirs, type, result);

    return attributes_Compare(&sample->attributes, theirs, type, result);
}

static HRESULT WINAPI sample_GetUINT32(IMFSample *iface, REFGUID key, UINT32 *value)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetUINT32(&sample->attributes, key, value);
}

static HRESULT WINAPI sample_GetUINT64(IMFSample *iface, REFGUID key, UINT64 *value)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetUINT64(&sample->attributes, key, value);
}

static HRESULT WINAPI sample_GetDouble(IMFSample *iface, REFGUID key, double *value)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetDouble(&sample->attributes, key, value);
}

static HRESULT WINAPI sample_GetGUID(IMFSample *iface, REFGUID key, GUID *value)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), value);

    return attributes_GetGUID(&sample->attributes, key, value);
}

static HRESULT WINAPI sample_GetStringLength(IMFSample *iface, REFGUID key, UINT32 *length)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), length);

    return attributes_GetStringLength(&sample->attributes, key, length);
}

static HRESULT WINAPI sample_GetString(IMFSample *iface, REFGUID key, WCHAR *value, UINT32 size, UINT32 *length)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p, %u, %p.\n", iface, debugstr_attr(key), value, size, length);

    return attributes_GetString(&sample->attributes, key, value, size, length);
}

static HRESULT WINAPI sample_GetAllocatedString(IMFSample *iface, REFGUID key, WCHAR **value, UINT32 *length)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_attr(key), value, length);

    return attributes_GetAllocatedString(&sample->attributes, key, value, length);
}

static HRESULT WINAPI sample_GetBlobSize(IMFSample *iface, REFGUID key, UINT32 *size)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), size);

    return attributes_GetBlobSize(&sample->attributes, key, size);
}

static HRESULT WINAPI sample_GetBlob(IMFSample *iface, REFGUID key, UINT8 *buf, UINT32 bufsize, UINT32 *blobsize)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p, %u, %p.\n", iface, debugstr_attr(key), buf, bufsize, blobsize);

    return attributes_GetBlob(&sample->attributes, key, buf, bufsize, blobsize);
}

static HRESULT WINAPI sample_GetAllocatedBlob(IMFSample *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_attr(key), buf, size);

    return attributes_GetAllocatedBlob(&sample->attributes, key, buf, size);
}

static HRESULT WINAPI sample_GetUnknown(IMFSample *iface, REFGUID key, REFIID riid, void **out)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_attr(key), debugstr_guid(riid), out);

    return attributes_GetUnknown(&sample->attributes, key, riid, out);
}

static HRESULT WINAPI sample_SetItem(IMFSample *iface, REFGUID key, REFPROPVARIANT value)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_propvar(value));

    return attributes_SetItem(&sample->attributes, key, value);
}

static HRESULT WINAPI sample_DeleteItem(IMFSample *iface, REFGUID key)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s.\n", iface, debugstr_attr(key));

    return attributes_DeleteItem(&sample->attributes, key);
}

static HRESULT WINAPI sample_DeleteAllItems(IMFSample *iface)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p.\n", iface);

    return attributes_DeleteAllItems(&sample->attributes);
}

static HRESULT WINAPI sample_SetUINT32(IMFSample *iface, REFGUID key, UINT32 value)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %u.\n", iface, debugstr_attr(key), value);

    return attributes_SetUINT32(&sample->attributes, key, value);
}

static HRESULT WINAPI sample_SetUINT64(IMFSample *iface, REFGUID key, UINT64 value)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), wine_dbgstr_longlong(value));

    return attributes_SetUINT64(&sample->attributes, key, value);
}

static HRESULT WINAPI sample_SetDouble(IMFSample *iface, REFGUID key, double value)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %f.\n", iface, debugstr_attr(key), value);

    return attributes_SetDouble(&sample->attributes, key, value);
}

static HRESULT WINAPI sample_SetGUID(IMFSample *iface, REFGUID key, REFGUID value)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_mf_guid(value));

    return attributes_SetGUID(&sample->attributes, key, value);
}

static HRESULT WINAPI sample_SetString(IMFSample *iface, REFGUID key, const WCHAR *value)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_attr(key), debugstr_w(value));

    return attributes_SetString(&sample->attributes, key, value);
}

static HRESULT WINAPI sample_SetBlob(IMFSample *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p, %u.\n", iface, debugstr_attr(key), buf, size);

    return attributes_SetBlob(&sample->attributes, key, buf, size);
}

static HRESULT WINAPI sample_SetUnknown(IMFSample *iface, REFGUID key, IUnknown *unknown)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_attr(key), unknown);

    return attributes_SetUnknown(&sample->attributes, key, unknown);
}

static HRESULT WINAPI sample_LockStore(IMFSample *iface)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p.\n", iface);

    return attributes_LockStore(&sample->attributes);
}

static HRESULT WINAPI sample_UnlockStore(IMFSample *iface)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p.\n", iface);

    return attributes_UnlockStore(&sample->attributes);
}

static HRESULT WINAPI sample_GetCount(IMFSample *iface, UINT32 *count)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %p.\n", iface, count);

    return attributes_GetCount(&sample->attributes, count);
}

static HRESULT WINAPI sample_GetItemByIndex(IMFSample *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %u, %p, %p.\n", iface, index, key, value);

    return attributes_GetItemByIndex(&sample->attributes, index, key, value);
}

static HRESULT WINAPI sample_CopyAllItems(IMFSample *iface, IMFAttributes *dest)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %p.\n", iface, dest);

    return attributes_CopyAllItems(&sample->attributes, dest);
}

static HRESULT WINAPI sample_GetSampleFlags(IMFSample *iface, DWORD *flags)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %p.\n", iface, flags);

    EnterCriticalSection(&sample->cs);
    *flags = sample->flags;
    LeaveCriticalSection(&sample->cs);

    return S_OK;
}

static HRESULT WINAPI sample_SetSampleFlags(IMFSample *iface, DWORD flags)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %#x.\n", iface, flags);

    EnterCriticalSection(&sample->cs);
    sample->flags = flags;
    LeaveCriticalSection(&sample->cs);

    return S_OK;
}

static HRESULT WINAPI sample_GetSampleTime(IMFSample *iface, LONGLONG *timestamp)
{
    struct sample *sample = impl_from_IMFSample(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, timestamp);

    EnterCriticalSection(&sample->cs);
    if (sample->prop_flags & SAMPLE_PROP_HAS_TIMESTAMP)
        *timestamp = sample->timestamp;
    else
        hr = MF_E_NO_SAMPLE_TIMESTAMP;
    LeaveCriticalSection(&sample->cs);

    return hr;
}

static HRESULT WINAPI sample_SetSampleTime(IMFSample *iface, LONGLONG timestamp)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s.\n", iface, wine_dbgstr_longlong(timestamp));

    EnterCriticalSection(&sample->cs);
    sample->timestamp = timestamp;
    sample->prop_flags |= SAMPLE_PROP_HAS_TIMESTAMP;
    LeaveCriticalSection(&sample->cs);

    return S_OK;
}

static HRESULT WINAPI sample_GetSampleDuration(IMFSample *iface, LONGLONG *duration)
{
    struct sample *sample = impl_from_IMFSample(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, duration);

    EnterCriticalSection(&sample->cs);
    if (sample->prop_flags & SAMPLE_PROP_HAS_DURATION)
        *duration = sample->duration;
    else
        hr = MF_E_NO_SAMPLE_DURATION;
    LeaveCriticalSection(&sample->cs);

    return hr;
}

static HRESULT WINAPI sample_SetSampleDuration(IMFSample *iface, LONGLONG duration)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s.\n", iface, wine_dbgstr_longlong(duration));

    EnterCriticalSection(&sample->cs);
    sample->duration = duration;
    sample->prop_flags |= SAMPLE_PROP_HAS_DURATION;
    LeaveCriticalSection(&sample->cs);

    return S_OK;
}

static HRESULT WINAPI sample_GetBufferCount(IMFSample *iface, DWORD *count)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %p.\n", iface, count);

    if (!count)
        return E_INVALIDARG;

    EnterCriticalSection(&sample->cs);
    *count = sample->buffer_count;
    LeaveCriticalSection(&sample->cs);

    return S_OK;
}

static HRESULT WINAPI sample_GetBufferByIndex(IMFSample *iface, DWORD index, IMFMediaBuffer **buffer)
{
    struct sample *sample = impl_from_IMFSample(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u, %p.\n", iface, index, buffer);

    EnterCriticalSection(&sample->cs);
    if (index < sample->buffer_count)
    {
        *buffer = sample->buffers[index];
        IMFMediaBuffer_AddRef(*buffer);
    }
    else
        hr = E_INVALIDARG;
    LeaveCriticalSection(&sample->cs);

    return hr;
}

static HRESULT WINAPI sample_ConvertToContiguousBuffer(IMFSample *iface, IMFMediaBuffer **buffer)
{
    struct sample *sample = impl_from_IMFSample(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, buffer);

    EnterCriticalSection(&sample->cs);

    if (sample->buffer_count == 0)
        hr = E_UNEXPECTED;
    else if (sample->buffer_count == 1)
    {
        *buffer = sample->buffers[0];
        IMFMediaBuffer_AddRef(*buffer);
    }
    else
    {
        FIXME("Samples with multiple buffers are not supported.\n");
        hr = E_NOTIMPL;
    }

    LeaveCriticalSection(&sample->cs);

    return hr;
}

static HRESULT WINAPI sample_AddBuffer(IMFSample *iface, IMFMediaBuffer *buffer)
{
    struct sample *sample = impl_from_IMFSample(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, buffer);

    EnterCriticalSection(&sample->cs);
    if (!mf_array_reserve((void **)&sample->buffers, &sample->capacity, sample->buffer_count + 1,
            sizeof(*sample->buffers)))
        hr = E_OUTOFMEMORY;
    else
    {
        sample->buffers[sample->buffer_count++] = buffer;
        IMFMediaBuffer_AddRef(buffer);
    }
    LeaveCriticalSection(&sample->cs);

    return hr;
}

static HRESULT WINAPI sample_RemoveBufferByIndex(IMFSample *iface, DWORD index)
{
    struct sample *sample = impl_from_IMFSample(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u.\n", iface, index);

    EnterCriticalSection(&sample->cs);
    if (index < sample->buffer_count)
    {
        IMFMediaBuffer_Release(sample->buffers[index]);
        if (index < sample->buffer_count - 1)
        {
            memmove(&sample->buffers[index], &sample->buffers[index+1],
                    (sample->buffer_count - index - 1) * sizeof(*sample->buffers));
        }
        sample->buffer_count--;
    }
    else
        hr = E_INVALIDARG;
    LeaveCriticalSection(&sample->cs);

    return hr;
}

static HRESULT WINAPI sample_RemoveAllBuffers(IMFSample *iface)
{
    struct sample *sample = impl_from_IMFSample(iface);
    size_t i;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&sample->cs);
    for (i = 0; i < sample->buffer_count; ++i)
         IMFMediaBuffer_Release(sample->buffers[i]);
    sample->buffer_count = 0;
    LeaveCriticalSection(&sample->cs);

    return S_OK;
}

static HRESULT WINAPI sample_GetTotalLength(IMFSample *iface, DWORD *total_length)
{
    struct sample *sample = impl_from_IMFSample(iface);
    DWORD length;
    size_t i;

    TRACE("%p, %p.\n", iface, total_length);

    *total_length = 0;

    EnterCriticalSection(&sample->cs);
    for (i = 0; i < sample->buffer_count; ++i)
    {
        if (SUCCEEDED(IMFMediaBuffer_GetCurrentLength(sample->buffers[i], &length)))
            *total_length += length;
    }
    LeaveCriticalSection(&sample->cs);

    return S_OK;
}

static HRESULT WINAPI sample_CopyToBuffer(IMFSample *iface, IMFMediaBuffer *buffer)
{
    FIXME("%p, %p.\n", iface, buffer);

    return E_NOTIMPL;
}

static const IMFSampleVtbl samplevtbl =
{
    sample_QueryInterface,
    sample_AddRef,
    sample_Release,
    sample_GetItem,
    sample_GetItemType,
    sample_CompareItem,
    sample_Compare,
    sample_GetUINT32,
    sample_GetUINT64,
    sample_GetDouble,
    sample_GetGUID,
    sample_GetStringLength,
    sample_GetString,
    sample_GetAllocatedString,
    sample_GetBlobSize,
    sample_GetBlob,
    sample_GetAllocatedBlob,
    sample_GetUnknown,
    sample_SetItem,
    sample_DeleteItem,
    sample_DeleteAllItems,
    sample_SetUINT32,
    sample_SetUINT64,
    sample_SetDouble,
    sample_SetGUID,
    sample_SetString,
    sample_SetBlob,
    sample_SetUnknown,
    sample_LockStore,
    sample_UnlockStore,
    sample_GetCount,
    sample_GetItemByIndex,
    sample_CopyAllItems,
    sample_GetSampleFlags,
    sample_SetSampleFlags,
    sample_GetSampleTime,
    sample_SetSampleTime,
    sample_GetSampleDuration,
    sample_SetSampleDuration,
    sample_GetBufferCount,
    sample_GetBufferByIndex,
    sample_ConvertToContiguousBuffer,
    sample_AddBuffer,
    sample_RemoveBufferByIndex,
    sample_RemoveAllBuffers,
    sample_GetTotalLength,
    sample_CopyToBuffer,
};

/***********************************************************************
 *      MFCreateSample (mfplat.@)
 */
HRESULT WINAPI MFCreateSample(IMFSample **sample)
{
    struct sample *object;
    HRESULT hr;

    TRACE("%p.\n", sample);

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    if (FAILED(hr = init_attributes_object(&object->attributes, 0)))
    {
        heap_free(object);
        return hr;
    }

    object->IMFSample_iface.lpVtbl = &samplevtbl;
    InitializeCriticalSection(&object->cs);

    *sample = &object->IMFSample_iface;

    TRACE("Created sample %p.\n", *sample);

    return S_OK;
}
