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
#include "d3d9.h"
#include "d3d11.h"
#include "initguid.h"
#include "dxva2api.h"

#include "wine/list.h"

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

struct sample_allocator
{
    IMFVideoSampleAllocatorEx IMFVideoSampleAllocatorEx_iface;
    IMFVideoSampleAllocatorCallback IMFVideoSampleAllocatorCallback_iface;
    IMFAsyncCallback tracking_callback;
    LONG refcount;

    IMFVideoSampleAllocatorNotify *callback;
    IDirect3DDeviceManager9 *d3d9_device_manager;
    IMFDXGIDeviceManager *dxgi_device_manager;

    struct
    {
        unsigned int width;
        unsigned int height;
        D3DFORMAT d3d9_format;
        DXGI_FORMAT dxgi_format;
        unsigned int usage;
        unsigned int bindflags;
        unsigned int miscflags;
        unsigned int buffer_count;
    } frame_desc;

    IMFAttributes *attributes;
    IMFMediaType *media_type;

    unsigned int free_sample_count;
    unsigned int cold_sample_count;
    struct list free_samples;
    struct list used_samples;
    CRITICAL_SECTION cs;
};

struct queued_sample
{
    struct list entry;
    IMFSample *sample;
};

static struct sample_allocator *impl_from_IMFVideoSampleAllocatorEx(IMFVideoSampleAllocatorEx *iface)
{
    return CONTAINING_RECORD(iface, struct sample_allocator, IMFVideoSampleAllocatorEx_iface);
}

static struct sample_allocator *impl_from_IMFVideoSampleAllocatorCallback(IMFVideoSampleAllocatorCallback *iface)
{
    return CONTAINING_RECORD(iface, struct sample_allocator, IMFVideoSampleAllocatorCallback_iface);
}

static struct sample_allocator *impl_from_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct sample_allocator, tracking_callback);
}

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

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static void release_sample_object(struct sample *sample)
{
    size_t i;

    for (i = 0; i < sample->buffer_count; ++i)
        IMFMediaBuffer_Release(sample->buffers[i]);
    clear_attributes_object(&sample->attributes);
    free(sample->buffers);
    free(sample);
}

static ULONG WINAPI sample_Release(IMFSample *iface)
{
    struct sample *sample = impl_from_IMFSample(iface);
    ULONG refcount = InterlockedDecrement(&sample->attributes.ref);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
        release_sample_object(sample);

    return refcount;
}

static ULONG WINAPI sample_tracked_Release(IMFSample *iface)
{
    struct sample *sample = impl_from_IMFSample(iface);
    ULONG refcount = InterlockedDecrement(&sample->attributes.ref);
    IRtwqAsyncResult *tracked_result = NULL;
    HRESULT hr;

    EnterCriticalSection(&sample->attributes.cs);
    if (sample->tracked_result && sample->tracked_refcount == refcount)
    {
        tracked_result = sample->tracked_result;
        sample->tracked_result = NULL;
        sample->tracked_refcount = 0;

        /* Call could fail if queue system is not initialized, it's not critical. */
        if (FAILED(hr = RtwqInvokeCallback(tracked_result)))
            WARN("Failed to invoke tracking callback, hr %#lx.\n", hr);
    }
    LeaveCriticalSection(&sample->attributes.cs);

    if (tracked_result)
        IRtwqAsyncResult_Release(tracked_result);

    TRACE("%p, refcount %lu.\n", iface, refcount);

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

    TRACE("%p, %#lx.\n", iface, flags);

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

    TRACE("%p, %lu, %p.\n", iface, index, buffer);

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

    TRACE("%p, %lu.\n", iface, index);

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
    IMF2DBuffer *buffer2d;
    BOOL locked = FALSE;
    HRESULT hr = E_FAIL;
    size_t i;

    TRACE("%p, %p.\n", iface, buffer);

    EnterCriticalSection(&sample->attributes.cs);

    total_length = sample_get_total_length(sample);
    dst_current_length = 0;

    if (sample->buffer_count == 1
            && SUCCEEDED(IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer, (void **)&buffer2d)))
    {
        if (SUCCEEDED(IMFMediaBuffer_GetCurrentLength(sample->buffers[0], &current_length))
                && SUCCEEDED(IMF2DBuffer_GetContiguousLength(buffer2d, &dst_length))
                && current_length == dst_length
                && SUCCEEDED(IMFMediaBuffer_Lock(sample->buffers[0], &src_ptr, &src_max_length, &current_length)))
        {
            hr = IMF2DBuffer_ContiguousCopyFrom(buffer2d, src_ptr, current_length);
            IMFMediaBuffer_Unlock(sample->buffers[0]);
        }
        IMF2DBuffer_Release(buffer2d);
        if (SUCCEEDED(hr))
        {
            dst_current_length = current_length;
            goto done;
        }
    }

    dst_ptr = NULL;
    dst_length = current_length = 0;
    locked = SUCCEEDED(hr = IMFMediaBuffer_Lock(buffer, &dst_ptr, &dst_length, &current_length));
    if (!locked)
        goto done;

    if (dst_length < total_length)
    {
        hr = MF_E_BUFFERTOOSMALL;
        goto done;
    }

    if (!dst_ptr)
        goto done;

    for (i = 0; i < sample->buffer_count && SUCCEEDED(hr); ++i)
    {
        src_ptr = NULL;
        src_max_length = current_length = 0;

        if (FAILED(hr = IMFMediaBuffer_Lock(sample->buffers[i], &src_ptr, &src_max_length, &current_length)))
            continue;

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

done:
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

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = init_attributes_object(&object->attributes, 0)))
    {
        free(object);
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

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = init_attributes_object(&object->attributes, 0)))
    {
        free(object);
        return hr;
    }

    object->IMFSample_iface.lpVtbl = &sample_tracked_vtbl;
    object->IMFTrackedSample_iface.lpVtbl = &tracked_sample_vtbl;

    *sample = &object->IMFTrackedSample_iface;

    return S_OK;
}

static HRESULT WINAPI sample_allocator_QueryInterface(IMFVideoSampleAllocatorEx *iface, REFIID riid, void **obj)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocatorEx(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFVideoSampleAllocatorEx) ||
            IsEqualIID(riid, &IID_IMFVideoSampleAllocator) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = &allocator->IMFVideoSampleAllocatorEx_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFVideoSampleAllocatorCallback))
    {
        *obj = &allocator->IMFVideoSampleAllocatorCallback_iface;
    }
    else
    {
        WARN("Unsupported interface %s.\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static ULONG WINAPI sample_allocator_AddRef(IMFVideoSampleAllocatorEx *iface)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocatorEx(iface);
    ULONG refcount = InterlockedIncrement(&allocator->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static void sample_allocator_release_samples(struct sample_allocator *allocator)
{
    struct queued_sample *iter, *iter2;

    LIST_FOR_EACH_ENTRY_SAFE(iter, iter2, &allocator->free_samples, struct queued_sample, entry)
    {
        list_remove(&iter->entry);
        IMFSample_Release(iter->sample);
        free(iter);
    }

    LIST_FOR_EACH_ENTRY_SAFE(iter, iter2, &allocator->used_samples, struct queued_sample, entry)
    {
        list_remove(&iter->entry);
        free(iter);
    }

    allocator->free_sample_count = 0;
    allocator->cold_sample_count = 0;
}

static void sample_allocator_set_media_type(struct sample_allocator *allocator, IMFMediaType *media_type)
{
    UINT64 frame_size;
    GUID subtype;

    if (!media_type)
    {
        if (allocator->media_type)
            IMFMediaType_Release(allocator->media_type);
        allocator->media_type = NULL;
        return;
    }

    /* Check if type is the same. */
    IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &frame_size);
    IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &subtype);

    if (frame_size == ((UINT64) allocator->frame_desc.width << 32 | allocator->frame_desc.height) &&
            subtype.Data1 == allocator->frame_desc.d3d9_format)
    {
        return;
    }

    if (allocator->media_type)
        IMFMediaType_Release(allocator->media_type);
    allocator->media_type = media_type;
    if (allocator->media_type)
        IMFMediaType_AddRef(allocator->media_type);
}

static void sample_allocator_set_attributes(struct sample_allocator *allocator, IMFAttributes *attributes)
{
    if (allocator->attributes)
        IMFAttributes_Release(allocator->attributes);
    allocator->attributes = attributes;
    if (allocator->attributes)
        IMFAttributes_AddRef(allocator->attributes);
}

static ULONG WINAPI sample_allocator_Release(IMFVideoSampleAllocatorEx *iface)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocatorEx(iface);
    ULONG refcount = InterlockedDecrement(&allocator->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (allocator->callback)
            IMFVideoSampleAllocatorNotify_Release(allocator->callback);
        if (allocator->d3d9_device_manager)
            IDirect3DDeviceManager9_Release(allocator->d3d9_device_manager);
        if (allocator->dxgi_device_manager)
            IMFDXGIDeviceManager_Release(allocator->dxgi_device_manager);
        sample_allocator_set_media_type(allocator, NULL);
        sample_allocator_set_attributes(allocator, NULL);
        sample_allocator_release_samples(allocator);
        DeleteCriticalSection(&allocator->cs);
        free(allocator);
    }

    return refcount;
}

static HRESULT WINAPI sample_allocator_SetDirectXManager(IMFVideoSampleAllocatorEx *iface,
        IUnknown *manager)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocatorEx(iface);
    IDirect3DDeviceManager9 *d3d9_device_manager = NULL;
    IMFDXGIDeviceManager *dxgi_device_manager = NULL;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, manager);

    if (manager)
    {
        if (FAILED(hr = IUnknown_QueryInterface(manager, &IID_IMFDXGIDeviceManager, (void **)&dxgi_device_manager)))
        {
            hr = IUnknown_QueryInterface(manager, &IID_IDirect3DDeviceManager9, (void **)&d3d9_device_manager);
        }

        if (FAILED(hr))
            return hr;
    }

    EnterCriticalSection(&allocator->cs);

    if (allocator->d3d9_device_manager)
        IDirect3DDeviceManager9_Release(allocator->d3d9_device_manager);
    if (allocator->dxgi_device_manager)
        IMFDXGIDeviceManager_Release(allocator->dxgi_device_manager);
    allocator->d3d9_device_manager = NULL;
    allocator->dxgi_device_manager = NULL;

    if (dxgi_device_manager)
        allocator->dxgi_device_manager = dxgi_device_manager;
    else if (d3d9_device_manager)
        allocator->d3d9_device_manager = d3d9_device_manager;

    LeaveCriticalSection(&allocator->cs);

    return S_OK;
}

static HRESULT WINAPI sample_allocator_UninitializeSampleAllocator(IMFVideoSampleAllocatorEx *iface)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocatorEx(iface);

    TRACE("%p.\n", iface);

    EnterCriticalSection(&allocator->cs);

    sample_allocator_release_samples(allocator);
    sample_allocator_set_media_type(allocator, NULL);
    sample_allocator_set_attributes(allocator, NULL);
    memset(&allocator->frame_desc, 0, sizeof(allocator->frame_desc));

    LeaveCriticalSection(&allocator->cs);

    return S_OK;
}

struct surface_service
{
    IDirectXVideoProcessorService *dxva_service;
    ID3D11Device *d3d11_device;
    HANDLE hdevice;
};

static HRESULT sample_allocator_get_surface_service(struct sample_allocator *allocator, struct surface_service *service)
{
    HRESULT hr = S_OK;

    memset(service, 0, sizeof(*service));

    if (allocator->d3d9_device_manager)
    {
        if (SUCCEEDED(hr = IDirect3DDeviceManager9_OpenDeviceHandle(allocator->d3d9_device_manager, &service->hdevice)))
        {
            if (FAILED(hr = IDirect3DDeviceManager9_GetVideoService(allocator->d3d9_device_manager, service->hdevice,
                    &IID_IDirectXVideoProcessorService, (void **)&service->dxva_service)))
            {
                WARN("Failed to get DXVA processor service, hr %#lx.\n", hr);
                IDirect3DDeviceManager9_CloseDeviceHandle(allocator->d3d9_device_manager, service->hdevice);
            }
        }
    }
    else if (allocator->dxgi_device_manager)
    {
        if (SUCCEEDED(hr = IMFDXGIDeviceManager_OpenDeviceHandle(allocator->dxgi_device_manager, &service->hdevice)))
        {
            if (FAILED(hr = IMFDXGIDeviceManager_GetVideoService(allocator->dxgi_device_manager, service->hdevice,
                    &IID_ID3D11Device, (void **)&service->d3d11_device)))
            {
                WARN("Failed to get D3D11 device, hr %#lx.\n", hr);
                IMFDXGIDeviceManager_CloseDeviceHandle(allocator->dxgi_device_manager, service->hdevice);
            }
        }
    }

    if (FAILED(hr))
        memset(service, 0, sizeof(*service));

    return hr;
}

static void sample_allocator_release_surface_service(struct sample_allocator *allocator,
        struct surface_service *service)
{
    if (service->dxva_service)
        IDirectXVideoProcessorService_Release(service->dxva_service);
    if (service->d3d11_device)
        ID3D11Device_Release(service->d3d11_device);

    if (allocator->d3d9_device_manager)
        IDirect3DDeviceManager9_CloseDeviceHandle(allocator->d3d9_device_manager, service->hdevice);
    else if (allocator->dxgi_device_manager)
        IMFDXGIDeviceManager_CloseDeviceHandle(allocator->dxgi_device_manager, service->hdevice);
}

static HRESULT sample_allocator_allocate_sample(struct sample_allocator *allocator, const struct surface_service *service,
        struct queued_sample **queued_sample)
{
    IMFTrackedSample *tracked_sample;
    IMFMediaBuffer *buffer;
    IMFSample *sample;
    unsigned int i;
    HRESULT hr;

    if (FAILED(hr = MFCreateTrackedSample(&tracked_sample)))
        return hr;

    IMFTrackedSample_QueryInterface(tracked_sample, &IID_IMFSample, (void **)&sample);
    IMFTrackedSample_Release(tracked_sample);

    for (i = 0; i < allocator->frame_desc.buffer_count; ++i)
    {
        if (service->dxva_service)
        {
            IDirect3DSurface9 *surface;

            if (SUCCEEDED(hr = IDirectXVideoProcessorService_CreateSurface(service->dxva_service, allocator->frame_desc.width,
                    allocator->frame_desc.height, 0, allocator->frame_desc.d3d9_format, D3DPOOL_DEFAULT, 0,
                    DXVA2_VideoProcessorRenderTarget, &surface, NULL)))
            {
                hr = MFCreateDXSurfaceBuffer(&IID_IDirect3DSurface9, (IUnknown *)surface, FALSE, &buffer);
                IDirect3DSurface9_Release(surface);
            }
        }
        else if (service->d3d11_device)
        {
            D3D11_TEXTURE2D_DESC desc = { 0 };
            ID3D11Texture2D *texture;

            desc.Width = allocator->frame_desc.width;
            desc.Height = allocator->frame_desc.height;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = allocator->frame_desc.dxgi_format;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.Usage = allocator->frame_desc.usage;
            desc.BindFlags = allocator->frame_desc.bindflags;
            if (desc.Usage == D3D11_USAGE_DYNAMIC)
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            else if (desc.Usage == D3D11_USAGE_STAGING)
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
            desc.MiscFlags = allocator->frame_desc.miscflags;

            if (SUCCEEDED(hr = ID3D11Device_CreateTexture2D(service->d3d11_device, &desc, NULL, &texture)))
            {
                hr = MFCreateDXGISurfaceBuffer(&IID_ID3D11Texture2D, (IUnknown *)texture, 0, FALSE, &buffer);
                ID3D11Texture2D_Release(texture);
            }
        }
        else
        {
            hr = MFCreate2DMediaBuffer(allocator->frame_desc.width, allocator->frame_desc.height,
                    allocator->frame_desc.d3d9_format, FALSE, &buffer);
        }

        if (SUCCEEDED(hr))
        {
            hr = IMFSample_AddBuffer(sample, buffer);
            IMFMediaBuffer_Release(buffer);
        }
    }

    if (FAILED(hr))
    {
        IMFSample_Release(sample);
        return hr;
    }

    if (!(*queued_sample = malloc(sizeof(**queued_sample))))
    {
        IMFSample_Release(sample);
        return E_OUTOFMEMORY;
    }
    (*queued_sample)->sample = sample;

    return hr;
}

static HRESULT sample_allocator_initialize(struct sample_allocator *allocator, unsigned int sample_count,
        unsigned int max_sample_count, IMFAttributes *attributes, IMFMediaType *media_type)
{
    struct surface_service service;
    struct queued_sample *sample;
    DXGI_FORMAT dxgi_format;
    unsigned int i, value;
    GUID major, subtype;
    UINT64 frame_size;
    UINT32 usage;
    HRESULT hr;

    if (FAILED(hr = IMFMediaType_GetMajorType(media_type, &major)))
        return hr;

    if (!IsEqualGUID(&major, &MFMediaType_Video))
        return MF_E_INVALIDMEDIATYPE;

    if (FAILED(hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &frame_size)))
        return hr;

    if (FAILED(hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &subtype)))
        return hr;

    if (sample_count > max_sample_count)
        return E_INVALIDARG;

    usage = D3D11_USAGE_DEFAULT;
    if (attributes)
    {
        IMFAttributes_GetUINT32(attributes, &MF_SA_BUFFERS_PER_SAMPLE, &allocator->frame_desc.buffer_count);
        IMFAttributes_GetUINT32(attributes, &MF_SA_D3D11_USAGE, &usage);
    }

    if (usage == D3D11_USAGE_IMMUTABLE || usage > D3D11_USAGE_STAGING)
        return E_INVALIDARG;

    dxgi_format = MFMapDX9FormatToDXGIFormat(subtype.Data1);

    allocator->frame_desc.bindflags = 0;
    allocator->frame_desc.miscflags = 0;
    allocator->frame_desc.usage = D3D11_USAGE_DEFAULT;

    if (dxgi_format == DXGI_FORMAT_B8G8R8A8_UNORM ||
            dxgi_format == DXGI_FORMAT_B8G8R8X8_UNORM)
    {
        allocator->frame_desc.usage = usage;
        if (allocator->frame_desc.usage == D3D11_USAGE_DEFAULT)
            allocator->frame_desc.bindflags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        else if (allocator->frame_desc.usage == D3D11_USAGE_DYNAMIC)
            allocator->frame_desc.bindflags = D3D11_BIND_SHADER_RESOURCE;
    }

    if (attributes)
    {
        IMFAttributes_GetUINT32(attributes, &MF_SA_D3D11_BINDFLAGS, &allocator->frame_desc.bindflags);
        if (SUCCEEDED(IMFAttributes_GetUINT32(attributes, &MF_SA_D3D11_SHARED, &value)) && value)
            allocator->frame_desc.miscflags |= D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
        if (SUCCEEDED(IMFAttributes_GetUINT32(attributes, &MF_SA_D3D11_SHARED_WITHOUT_MUTEX, &value)) && value)
            allocator->frame_desc.miscflags |= D3D11_RESOURCE_MISC_SHARED;
    }

    sample_allocator_set_media_type(allocator, media_type);
    sample_allocator_set_attributes(allocator, attributes);

    sample_count = max(1, sample_count);
    max_sample_count = max(1, max_sample_count);

    allocator->frame_desc.d3d9_format = subtype.Data1;
    allocator->frame_desc.dxgi_format = dxgi_format;
    allocator->frame_desc.width = frame_size >> 32;
    allocator->frame_desc.height = frame_size;
    allocator->frame_desc.buffer_count = max(1, allocator->frame_desc.buffer_count);

    if (FAILED(hr = sample_allocator_get_surface_service(allocator, &service)))
        return hr;

    sample_allocator_release_samples(allocator);

    for (i = 0; i < sample_count; ++i)
    {
        if (SUCCEEDED(hr = sample_allocator_allocate_sample(allocator, &service, &sample)))
        {
            list_add_tail(&allocator->free_samples, &sample->entry);
            allocator->free_sample_count++;
        }
    }
    allocator->cold_sample_count = max_sample_count - allocator->free_sample_count;

    sample_allocator_release_surface_service(allocator, &service);

    return hr;
}

static HRESULT WINAPI sample_allocator_InitializeSampleAllocator(IMFVideoSampleAllocatorEx *iface,
        DWORD sample_count, IMFMediaType *media_type)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocatorEx(iface);
    HRESULT hr;

    TRACE("%p, %lu, %p.\n", iface, sample_count, media_type);

    if (!sample_count)
        return E_INVALIDARG;

    EnterCriticalSection(&allocator->cs);

    hr = sample_allocator_initialize(allocator, sample_count, sample_count, NULL, media_type);

    LeaveCriticalSection(&allocator->cs);

    return hr;
}

static HRESULT sample_allocator_track_sample(struct sample_allocator *allocator, IMFSample *sample)
{
    IMFTrackedSample *tracked_sample;
    HRESULT hr;

    if (SUCCEEDED(hr = IMFSample_QueryInterface(sample, &IID_IMFTrackedSample, (void **)&tracked_sample)))
    {
        hr = IMFTrackedSample_SetAllocator(tracked_sample, &allocator->tracking_callback, NULL);
        IMFTrackedSample_Release(tracked_sample);
    }

    return hr;
}

static HRESULT WINAPI sample_allocator_AllocateSample(IMFVideoSampleAllocatorEx *iface, IMFSample **out)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocatorEx(iface);
    struct queued_sample *sample;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, out);

    EnterCriticalSection(&allocator->cs);

    if (list_empty(&allocator->free_samples) && list_empty(&allocator->used_samples))
        hr = MF_E_NOT_INITIALIZED;
    else if (list_empty(&allocator->free_samples) && !allocator->cold_sample_count)
        hr = MF_E_SAMPLEALLOCATOR_EMPTY;
    else if (!list_empty(&allocator->free_samples))
    {
        struct list *head = list_head(&allocator->free_samples);

        sample = LIST_ENTRY(head, struct queued_sample, entry);

        if (SUCCEEDED(hr = sample_allocator_track_sample(allocator, sample->sample)))
        {
            list_remove(head);
            list_add_tail(&allocator->used_samples, head);
            allocator->free_sample_count--;

            /* Reference counter is not increased when sample is returned, so next release could trigger
               tracking condition. This is balanced by incremented reference counter when sample is returned
               back to the free list. */
            *out = sample->sample;
        }
    }
    else /* allocator->cold_sample_count != 0 */
    {
        struct surface_service service;

        if (SUCCEEDED(hr = sample_allocator_get_surface_service(allocator, &service)))
        {
            if (SUCCEEDED(hr = sample_allocator_allocate_sample(allocator, &service, &sample)))
            {
                if (SUCCEEDED(hr = sample_allocator_track_sample(allocator, sample->sample)))
                {
                    list_add_tail(&allocator->used_samples, &sample->entry);
                    allocator->cold_sample_count--;

                    *out = sample->sample;
                }
                else
                {
                    IMFSample_Release(sample->sample);
                    free(sample);
                }
            }

            sample_allocator_release_surface_service(allocator, &service);
        }
    }

    LeaveCriticalSection(&allocator->cs);

    return hr;
}

static HRESULT WINAPI sample_allocator_InitializeSampleAllocatorEx(IMFVideoSampleAllocatorEx *iface, DWORD initial_sample_count,
        DWORD max_sample_count, IMFAttributes *attributes, IMFMediaType *media_type)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocatorEx(iface);
    HRESULT hr;

    TRACE("%p, %lu, %lu, %p, %p.\n", iface, initial_sample_count, max_sample_count, attributes, media_type);

    EnterCriticalSection(&allocator->cs);

    hr = sample_allocator_initialize(allocator, initial_sample_count, max_sample_count, attributes, media_type);

    LeaveCriticalSection(&allocator->cs);

    return hr;
}

static const IMFVideoSampleAllocatorExVtbl sample_allocator_vtbl =
{
    sample_allocator_QueryInterface,
    sample_allocator_AddRef,
    sample_allocator_Release,
    sample_allocator_SetDirectXManager,
    sample_allocator_UninitializeSampleAllocator,
    sample_allocator_InitializeSampleAllocator,
    sample_allocator_AllocateSample,
    sample_allocator_InitializeSampleAllocatorEx,
};

static HRESULT WINAPI sample_allocator_callback_QueryInterface(IMFVideoSampleAllocatorCallback *iface,
        REFIID riid, void **obj)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocatorCallback(iface);
    return IMFVideoSampleAllocatorEx_QueryInterface(&allocator->IMFVideoSampleAllocatorEx_iface, riid, obj);
}

static ULONG WINAPI sample_allocator_callback_AddRef(IMFVideoSampleAllocatorCallback *iface)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocatorCallback(iface);
    return IMFVideoSampleAllocatorEx_AddRef(&allocator->IMFVideoSampleAllocatorEx_iface);
}

static ULONG WINAPI sample_allocator_callback_Release(IMFVideoSampleAllocatorCallback *iface)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocatorCallback(iface);
    return IMFVideoSampleAllocatorEx_Release(&allocator->IMFVideoSampleAllocatorEx_iface);
}

static HRESULT WINAPI sample_allocator_callback_SetCallback(IMFVideoSampleAllocatorCallback *iface,
        IMFVideoSampleAllocatorNotify *callback)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocatorCallback(iface);

    TRACE("%p, %p.\n", iface, callback);

    EnterCriticalSection(&allocator->cs);
    if (allocator->callback)
        IMFVideoSampleAllocatorNotify_Release(allocator->callback);
    allocator->callback = callback;
    if (allocator->callback)
        IMFVideoSampleAllocatorNotify_AddRef(allocator->callback);
    LeaveCriticalSection(&allocator->cs);

    return S_OK;
}

static HRESULT WINAPI sample_allocator_callback_GetFreeSampleCount(IMFVideoSampleAllocatorCallback *iface,
        LONG *count)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocatorCallback(iface);

    TRACE("%p, %p.\n", iface, count);

    if (!count)
        return E_POINTER;

    EnterCriticalSection(&allocator->cs);
    *count = allocator->free_sample_count;
    LeaveCriticalSection(&allocator->cs);

    return S_OK;
}

static const IMFVideoSampleAllocatorCallbackVtbl sample_allocator_callback_vtbl =
{
    sample_allocator_callback_QueryInterface,
    sample_allocator_callback_AddRef,
    sample_allocator_callback_Release,
    sample_allocator_callback_SetCallback,
    sample_allocator_callback_GetFreeSampleCount,
};

static HRESULT WINAPI sample_allocator_tracking_callback_QueryInterface(IMFAsyncCallback *iface,
        REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFAsyncCallback_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI sample_allocator_tracking_callback_AddRef(IMFAsyncCallback *iface)
{
    struct sample_allocator *allocator = impl_from_IMFAsyncCallback(iface);
    return IMFVideoSampleAllocatorEx_AddRef(&allocator->IMFVideoSampleAllocatorEx_iface);
}

static ULONG WINAPI sample_allocator_tracking_callback_Release(IMFAsyncCallback *iface)
{
    struct sample_allocator *allocator = impl_from_IMFAsyncCallback(iface);
    return IMFVideoSampleAllocatorEx_Release(&allocator->IMFVideoSampleAllocatorEx_iface);
}

static HRESULT WINAPI sample_allocator_tracking_callback_GetParameters(IMFAsyncCallback *iface,
        DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI sample_allocator_tracking_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct sample_allocator *allocator = impl_from_IMFAsyncCallback(iface);
    struct queued_sample *iter;
    IUnknown *object = NULL;
    IMFSample *sample = NULL;
    HRESULT hr;

    if (FAILED(IMFAsyncResult_GetObject(result, &object)))
        return E_UNEXPECTED;

    hr = IUnknown_QueryInterface(object, &IID_IMFSample, (void **)&sample);
    IUnknown_Release(object);
    if (FAILED(hr))
        return E_UNEXPECTED;

    EnterCriticalSection(&allocator->cs);

    LIST_FOR_EACH_ENTRY(iter, &allocator->used_samples, struct queued_sample, entry)
    {
        if (sample == iter->sample)
        {
            list_remove(&iter->entry);
            list_add_tail(&allocator->free_samples, &iter->entry);
            IMFSample_AddRef(iter->sample);
            allocator->free_sample_count++;
            break;
        }
    }

    IMFSample_Release(sample);

    if (allocator->callback)
        IMFVideoSampleAllocatorNotify_NotifyRelease(allocator->callback);

    LeaveCriticalSection(&allocator->cs);

    return S_OK;
}

static const IMFAsyncCallbackVtbl sample_allocator_tracking_callback_vtbl =
{
    sample_allocator_tracking_callback_QueryInterface,
    sample_allocator_tracking_callback_AddRef,
    sample_allocator_tracking_callback_Release,
    sample_allocator_tracking_callback_GetParameters,
    sample_allocator_tracking_callback_Invoke,
};

/***********************************************************************
 *      MFCreateVideoSampleAllocatorEx (mfplat.@)
 */
HRESULT WINAPI MFCreateVideoSampleAllocatorEx(REFIID riid, void **obj)
{
    struct sample_allocator *object;
    HRESULT hr;

    TRACE("%s, %p.\n", debugstr_guid(riid), obj);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFVideoSampleAllocatorEx_iface.lpVtbl = &sample_allocator_vtbl;
    object->IMFVideoSampleAllocatorCallback_iface.lpVtbl = &sample_allocator_callback_vtbl;
    object->tracking_callback.lpVtbl = &sample_allocator_tracking_callback_vtbl;
    object->refcount = 1;
    list_init(&object->used_samples);
    list_init(&object->free_samples);
    InitializeCriticalSection(&object->cs);

    hr = IMFVideoSampleAllocatorEx_QueryInterface(&object->IMFVideoSampleAllocatorEx_iface, riid, obj);
    IMFVideoSampleAllocatorEx_Release(&object->IMFVideoSampleAllocatorEx_iface);

    return hr;
}
