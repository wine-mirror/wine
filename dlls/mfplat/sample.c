/*
 * Copyright 2021 Nikolay Sivov for CodeWeavers
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
#include "rtworkq.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

enum sample_prop_flags
{
    SAMPLE_PROP_HAS_DURATION  = 1 << 0,
    SAMPLE_PROP_HAS_TIMESTAMP = 1 << 1,
};

struct sample
{
    struct attributes attributes;
    IMFSample IMFSample_iface;
    IMFTrackedSample IMFTrackedSample_iface;

    IMFMediaBuffer **buffers;
    size_t buffer_count;
    size_t capacity;
    DWORD flags;
    DWORD prop_flags;
    LONGLONG duration;
    LONGLONG timestamp;

    /* Tracked sample functionality. */
    IRtwqAsyncResult *tracked_result;
    LONG tracked_refcount;
};

static struct sample *impl_from_IMFSample(IMFSample *iface)
{
    return CONTAINING_RECORD(iface, struct sample, IMFSample_iface);
}

static struct sample *impl_from_IMFTrackedSample(IMFTrackedSample *iface)
{
    return CONTAINING_RECORD(iface, struct sample, IMFTrackedSample_iface);
}

static HRESULT WINAPI sample_QueryInterface(IMFSample *iface, REFIID riid, void **out)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFSample) ||
            IsEqualIID(riid, &IID_IMFAttributes) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &sample->IMFSample_iface;
    }
    else if (sample->IMFTrackedSample_iface.lpVtbl && IsEqualIID(riid, &IID_IMFTrackedSample))
    {
        *out = &sample->IMFTrackedSample_iface;
    }
    else
    {
        WARN("Unsupported %s.\n", debugstr_guid(riid));
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI sample_AddRef(IMFSample *iface)
{
    struct sample *sample = impl_from_IMFSample(iface);
    ULONG refcount = InterlockedIncrement(&sample->attributes.ref);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static void release_sample_object(struct sample *sample)
{
    size_t i;

    for (i = 0; i < sample->buffer_count; ++i)
        IMFMediaBuffer_Release(sample->buffers[i]);
    clear_attributes_object(&sample->attributes);
    heap_free(sample->buffers);
    heap_free(sample);
}

static ULONG WINAPI sample_Release(IMFSample *iface)
{
    struct sample *sample = impl_from_IMFSample(iface);
    ULONG refcount = InterlockedDecrement(&sample->attributes.ref);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
        release_sample_object(sample);

    return refcount;
}

static ULONG WINAPI sample_tracked_Release(IMFSample *iface)
{
    struct sample *sample = impl_from_IMFSample(iface);
    ULONG refcount;
    HRESULT hr;

    EnterCriticalSection(&sample->attributes.cs);
    refcount = InterlockedDecrement(&sample->attributes.ref);
    if (sample->tracked_result && sample->tracked_refcount == refcount)
    {
        /* Call could fail if queue system is not initialized, it's not critical. */
        if (FAILED(hr = RtwqInvokeCallback(sample->tracked_result)))
            WARN("Failed to invoke tracking callback, hr %#x.\n", hr);
        IRtwqAsyncResult_Release(sample->tracked_result);
        sample->tracked_result = NULL;
        sample->tracked_refcount = 0;
    }
    LeaveCriticalSection(&sample->attributes.cs);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
        release_sample_object(sample);

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

    EnterCriticalSection(&sample->attributes.cs);
    *flags = sample->flags;
    LeaveCriticalSection(&sample->attributes.cs);

    return S_OK;
}

static HRESULT WINAPI sample_SetSampleFlags(IMFSample *iface, DWORD flags)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %#x.\n", iface, flags);

    EnterCriticalSection(&sample->attributes.cs);
    sample->flags = flags;
    LeaveCriticalSection(&sample->attributes.cs);

    return S_OK;
}

static HRESULT WINAPI sample_GetSampleTime(IMFSample *iface, LONGLONG *timestamp)
{
    struct sample *sample = impl_from_IMFSample(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, timestamp);

    EnterCriticalSection(&sample->attributes.cs);
    if (sample->prop_flags & SAMPLE_PROP_HAS_TIMESTAMP)
        *timestamp = sample->timestamp;
    else
        hr = MF_E_NO_SAMPLE_TIMESTAMP;
    LeaveCriticalSection(&sample->attributes.cs);

    return hr;
}

static HRESULT WINAPI sample_SetSampleTime(IMFSample *iface, LONGLONG timestamp)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s.\n", iface, debugstr_time(timestamp));

    EnterCriticalSection(&sample->attributes.cs);
    sample->timestamp = timestamp;
    sample->prop_flags |= SAMPLE_PROP_HAS_TIMESTAMP;
    LeaveCriticalSection(&sample->attributes.cs);

    return S_OK;
}

static HRESULT WINAPI sample_GetSampleDuration(IMFSample *iface, LONGLONG *duration)
{
    struct sample *sample = impl_from_IMFSample(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, duration);

    EnterCriticalSection(&sample->attributes.cs);
    if (sample->prop_flags & SAMPLE_PROP_HAS_DURATION)
        *duration = sample->duration;
    else
        hr = MF_E_NO_SAMPLE_DURATION;
    LeaveCriticalSection(&sample->attributes.cs);

    return hr;
}

static HRESULT WINAPI sample_SetSampleDuration(IMFSample *iface, LONGLONG duration)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %s.\n", iface, debugstr_time(duration));

    EnterCriticalSection(&sample->attributes.cs);
    sample->duration = duration;
    sample->prop_flags |= SAMPLE_PROP_HAS_DURATION;
    LeaveCriticalSection(&sample->attributes.cs);

    return S_OK;
}

static HRESULT WINAPI sample_GetBufferCount(IMFSample *iface, DWORD *count)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %p.\n", iface, count);

    if (!count)
        return E_INVALIDARG;

    EnterCriticalSection(&sample->attributes.cs);
    *count = sample->buffer_count;
    LeaveCriticalSection(&sample->attributes.cs);

    return S_OK;
}

static HRESULT WINAPI sample_GetBufferByIndex(IMFSample *iface, DWORD index, IMFMediaBuffer **buffer)
{
    struct sample *sample = impl_from_IMFSample(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u, %p.\n", iface, index, buffer);

    EnterCriticalSection(&sample->attributes.cs);
    if (index < sample->buffer_count)
    {
        *buffer = sample->buffers[index];
        IMFMediaBuffer_AddRef(*buffer);
    }
    else
        hr = E_INVALIDARG;
    LeaveCriticalSection(&sample->attributes.cs);

    return hr;
}

static unsigned int sample_get_total_length(struct sample *sample)
{
    DWORD total_length = 0, length;
    size_t i;

    for (i = 0; i < sample->buffer_count; ++i)
    {
        length = 0;
        if (SUCCEEDED(IMFMediaBuffer_GetCurrentLength(sample->buffers[i], &length)))
            total_length += length;
    }

    return total_length;
}

static HRESULT sample_copy_to_buffer(struct sample *sample, IMFMediaBuffer *buffer)
{
    DWORD total_length, dst_length, dst_current_length, src_max_length, current_length;
    BYTE *src_ptr, *dst_ptr;
    BOOL locked;
    HRESULT hr;
    size_t i;

    total_length = sample_get_total_length(sample);
    dst_current_length = 0;

    dst_ptr = NULL;
    dst_length = current_length = 0;
    locked = SUCCEEDED(hr = IMFMediaBuffer_Lock(buffer, &dst_ptr, &dst_length, &current_length));
    if (locked)
    {
        if (dst_length < total_length)
            hr = MF_E_BUFFERTOOSMALL;
        else if (dst_ptr)
        {
            for (i = 0; i < sample->buffer_count && SUCCEEDED(hr); ++i)
            {
                src_ptr = NULL;
                src_max_length = current_length = 0;
                if (SUCCEEDED(hr = IMFMediaBuffer_Lock(sample->buffers[i], &src_ptr, &src_max_length, &current_length)))
                {
                    if (src_ptr)
                    {
                        if (current_length > dst_length)
                            hr = MF_E_BUFFERTOOSMALL;
                        else if (current_length)
                        {
                            memcpy(dst_ptr, src_ptr, current_length);
                            dst_length -= current_length;
                            dst_current_length += current_length;
                            dst_ptr += current_length;
                        }
                    }
                    IMFMediaBuffer_Unlock(sample->buffers[i]);
                }
            }
        }
    }

    if (FAILED(IMFMediaBuffer_SetCurrentLength(buffer, dst_current_length)))
        WARN("Failed to set buffer length.\n");

    if (locked)
        IMFMediaBuffer_Unlock(buffer);

    return hr;
}

static HRESULT WINAPI sample_ConvertToContiguousBuffer(IMFSample *iface, IMFMediaBuffer **buffer)
{
    struct sample *sample = impl_from_IMFSample(iface);
    unsigned int total_length, i;
    IMFMediaBuffer *dest_buffer;
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, buffer);

    EnterCriticalSection(&sample->attributes.cs);

    if (sample->buffer_count == 0)
        hr = E_UNEXPECTED;
    else if (sample->buffer_count > 1)
    {
        total_length = sample_get_total_length(sample);
        if (SUCCEEDED(hr = MFCreateMemoryBuffer(total_length, &dest_buffer)))
        {
            if (SUCCEEDED(hr = sample_copy_to_buffer(sample, dest_buffer)))
            {
                for (i = 0; i < sample->buffer_count; ++i)
                    IMFMediaBuffer_Release(sample->buffers[i]);

                sample->buffers[0] = dest_buffer;
                IMFMediaBuffer_AddRef(sample->buffers[0]);

                sample->buffer_count = 1;
            }
            IMFMediaBuffer_Release(dest_buffer);
        }
    }

    if (SUCCEEDED(hr) && buffer)
    {
        *buffer = sample->buffers[0];
        IMFMediaBuffer_AddRef(*buffer);
    }

    LeaveCriticalSection(&sample->attributes.cs);

    return hr;
}

static HRESULT WINAPI sample_AddBuffer(IMFSample *iface, IMFMediaBuffer *buffer)
{
    struct sample *sample = impl_from_IMFSample(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, buffer);

    EnterCriticalSection(&sample->attributes.cs);
    if (!mf_array_reserve((void **)&sample->buffers, &sample->capacity, sample->buffer_count + 1,
            sizeof(*sample->buffers)))
        hr = E_OUTOFMEMORY;
    else
    {
        sample->buffers[sample->buffer_count++] = buffer;
        IMFMediaBuffer_AddRef(buffer);
    }
    LeaveCriticalSection(&sample->attributes.cs);

    return hr;
}

static HRESULT WINAPI sample_RemoveBufferByIndex(IMFSample *iface, DWORD index)
{
    struct sample *sample = impl_from_IMFSample(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u.\n", iface, index);

    EnterCriticalSection(&sample->attributes.cs);
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
    LeaveCriticalSection(&sample->attributes.cs);

    return hr;
}

static HRESULT WINAPI sample_RemoveAllBuffers(IMFSample *iface)
{
    struct sample *sample = impl_from_IMFSample(iface);
    size_t i;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&sample->attributes.cs);
    for (i = 0; i < sample->buffer_count; ++i)
         IMFMediaBuffer_Release(sample->buffers[i]);
    sample->buffer_count = 0;
    LeaveCriticalSection(&sample->attributes.cs);

    return S_OK;
}

static HRESULT WINAPI sample_GetTotalLength(IMFSample *iface, DWORD *total_length)
{
    struct sample *sample = impl_from_IMFSample(iface);

    TRACE("%p, %p.\n", iface, total_length);

    EnterCriticalSection(&sample->attributes.cs);
    *total_length = sample_get_total_length(sample);
    LeaveCriticalSection(&sample->attributes.cs);

    return S_OK;
}

static HRESULT WINAPI sample_CopyToBuffer(IMFSample *iface, IMFMediaBuffer *buffer)
{
    struct sample *sample = impl_from_IMFSample(iface);
    DWORD total_length, dst_length, dst_current_length, src_max_length, current_length;
    BYTE *src_ptr, *dst_ptr;
    BOOL locked;
    HRESULT hr;
    size_t i;

    TRACE("%p, %p.\n", iface, buffer);

    EnterCriticalSection(&sample->attributes.cs);

    total_length = sample_get_total_length(sample);
    dst_current_length = 0;

    dst_ptr = NULL;
    dst_length = current_length = 0;
    locked = SUCCEEDED(hr = IMFMediaBuffer_Lock(buffer, &dst_ptr, &dst_length, &current_length));
    if (locked)
    {
        if (dst_length < total_length)
            hr = MF_E_BUFFERTOOSMALL;
        else if (dst_ptr)
        {
            for (i = 0; i < sample->buffer_count && SUCCEEDED(hr); ++i)
            {
                src_ptr = NULL;
                src_max_length = current_length = 0;
                if (SUCCEEDED(hr = IMFMediaBuffer_Lock(sample->buffers[i], &src_ptr, &src_max_length, &current_length)))
                {
                    if (src_ptr)
                    {
                        if (current_length > dst_length)
                            hr = MF_E_BUFFERTOOSMALL;
                        else if (current_length)
                        {
                            memcpy(dst_ptr, src_ptr, current_length);
                            dst_length -= current_length;
                            dst_current_length += current_length;
                            dst_ptr += current_length;
                        }
                    }
                    IMFMediaBuffer_Unlock(sample->buffers[i]);
                }
            }
        }
    }

    IMFMediaBuffer_SetCurrentLength(buffer, dst_current_length);

    if (locked)
        IMFMediaBuffer_Unlock(buffer);

    LeaveCriticalSection(&sample->attributes.cs);

    return hr;
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

static HRESULT WINAPI tracked_sample_QueryInterface(IMFTrackedSample *iface, REFIID riid, void **obj)
{
    struct sample *sample = impl_from_IMFTrackedSample(iface);
    return IMFSample_QueryInterface(&sample->IMFSample_iface, riid, obj);
}

static ULONG WINAPI tracked_sample_AddRef(IMFTrackedSample *iface)
{
    struct sample *sample = impl_from_IMFTrackedSample(iface);
    return IMFSample_AddRef(&sample->IMFSample_iface);
}

static ULONG WINAPI tracked_sample_Release(IMFTrackedSample *iface)
{
    struct sample *sample = impl_from_IMFTrackedSample(iface);
    return IMFSample_Release(&sample->IMFSample_iface);
}

static HRESULT WINAPI tracked_sample_SetAllocator(IMFTrackedSample *iface,
        IMFAsyncCallback *sample_allocator, IUnknown *state)
{
    struct sample *sample = impl_from_IMFTrackedSample(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p, %p.\n", iface, sample_allocator, state);

    EnterCriticalSection(&sample->attributes.cs);

    if (sample->tracked_result)
        hr = MF_E_NOTACCEPTING;
    else
    {
        if (SUCCEEDED(hr = RtwqCreateAsyncResult((IUnknown *)iface, (IRtwqAsyncCallback *)sample_allocator,
                state, &sample->tracked_result)))
        {
            /* Account for additional refcount brought by 'state' object. This threshold is used
               on Release() to invoke tracker callback.  */
            sample->tracked_refcount = 1;
            if (state == (IUnknown *)&sample->IMFTrackedSample_iface ||
                    state == (IUnknown *)&sample->IMFSample_iface)
            {
                ++sample->tracked_refcount;
            }
        }
    }

    LeaveCriticalSection(&sample->attributes.cs);

    return hr;
}

static const IMFTrackedSampleVtbl tracked_sample_vtbl =
{
    tracked_sample_QueryInterface,
    tracked_sample_AddRef,
    tracked_sample_Release,
    tracked_sample_SetAllocator,
};

static const IMFSampleVtbl sample_tracked_vtbl =
{
    sample_QueryInterface,
    sample_AddRef,
    sample_tracked_Release,
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

    *sample = &object->IMFSample_iface;

    TRACE("Created sample %p.\n", *sample);

    return S_OK;
}

/***********************************************************************
 *      MFCreateTrackedSample (mfplat.@)
 */
HRESULT WINAPI MFCreateTrackedSample(IMFTrackedSample **sample)
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

    object->IMFSample_iface.lpVtbl = &sample_tracked_vtbl;
    object->IMFTrackedSample_iface.lpVtbl = &tracked_sample_vtbl;

    *sample = &object->IMFTrackedSample_iface;

    return S_OK;
}
