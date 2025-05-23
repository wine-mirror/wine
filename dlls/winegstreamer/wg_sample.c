/*
 * Copyright 2022 Rémi Bernon for CodeWeavers
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

#include "gst_private.h"

#include "wmcodecdsp.h"
#include "mfapi.h"
#include "mferror.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);
WINE_DECLARE_DEBUG_CHANNEL(quartz);

struct wg_sample_queue
{
    CRITICAL_SECTION cs;
    struct list samples;
};

struct wg_sample_ops
{
    void (*destroy)(struct wg_sample *sample);
};

struct sample
{
    struct wg_sample wg_sample;

    const struct wg_sample_ops *ops;
    struct list entry;

    union
    {
        struct
        {
            IMFSample *sample;
            IMFMediaBuffer *buffer;
        } mf;
        struct
        {
            IMediaSample *sample;
        } quartz;
        struct
        {
            IMediaBuffer *buffer;
        } dmo;
    } u;
};

static const struct wg_sample_ops mf_sample_ops;

static inline struct sample *unsafe_mf_from_wg_sample(struct wg_sample *wg_sample)
{
    struct sample *sample = CONTAINING_RECORD(wg_sample, struct sample, wg_sample);
    if (sample->ops != &mf_sample_ops) return NULL;
    return sample;
}

static void mf_sample_destroy(struct wg_sample *wg_sample)
{
    struct sample *sample = unsafe_mf_from_wg_sample(wg_sample);

    TRACE_(mfplat)("wg_sample %p.\n", wg_sample);

    IMFMediaBuffer_Unlock(sample->u.mf.buffer);
    IMFMediaBuffer_Release(sample->u.mf.buffer);
    IMFSample_Release(sample->u.mf.sample);
}

static const struct wg_sample_ops mf_sample_ops =
{
    mf_sample_destroy,
};

HRESULT wg_sample_create_mf(IMFSample *mf_sample, struct wg_sample **out)
{
    DWORD current_length, max_length;
    struct sample *sample;
    BYTE *buffer;
    HRESULT hr;

    if (!(sample = calloc(1, sizeof(*sample))))
        return E_OUTOFMEMORY;
    if (FAILED(hr = IMFSample_ConvertToContiguousBuffer(mf_sample, &sample->u.mf.buffer)))
        goto fail;
    if (FAILED(hr = IMFMediaBuffer_Lock(sample->u.mf.buffer, &buffer, &max_length, &current_length)))
        goto fail;

    IMFSample_AddRef((sample->u.mf.sample = mf_sample));
    sample->wg_sample.data = (UINT_PTR)buffer;
    sample->wg_sample.size = current_length;
    sample->wg_sample.max_size = max_length;
    sample->ops = &mf_sample_ops;

    *out = &sample->wg_sample;
    TRACE_(mfplat)("Created wg_sample %p for IMFSample %p.\n", *out, mf_sample);
    return S_OK;

fail:
    if (sample->u.mf.buffer)
        IMFMediaBuffer_Release(sample->u.mf.buffer);
    free(sample);
    return hr;
}

static const struct wg_sample_ops quartz_sample_ops;

static inline struct sample *unsafe_quartz_from_wg_sample(struct wg_sample *wg_sample)
{
    struct sample *sample = CONTAINING_RECORD(wg_sample, struct sample, wg_sample);
    if (sample->ops != &quartz_sample_ops) return NULL;
    return sample;
}

static void quartz_sample_destroy(struct wg_sample *wg_sample)
{
    struct sample *sample = unsafe_quartz_from_wg_sample(wg_sample);

    TRACE_(quartz)("wg_sample %p.\n", wg_sample);

    IMediaSample_Release(sample->u.quartz.sample);
}

static const struct wg_sample_ops quartz_sample_ops =
{
    quartz_sample_destroy,
};

HRESULT wg_sample_create_quartz(IMediaSample *media_sample, struct wg_sample **out)
{
    DWORD current_length, max_length;
    struct sample *sample;
    BYTE *buffer;
    HRESULT hr;

    if (FAILED(hr = IMediaSample_GetPointer(media_sample, &buffer)))
        return hr;
    current_length = IMediaSample_GetActualDataLength(media_sample);
    max_length = IMediaSample_GetSize(media_sample);

    if (!(sample = calloc(1, sizeof(*sample))))
        return E_OUTOFMEMORY;

    IMediaSample_AddRef((sample->u.quartz.sample = media_sample));
    sample->wg_sample.data = (UINT_PTR)buffer;
    sample->wg_sample.size = current_length;
    sample->wg_sample.max_size = max_length;
    sample->ops = &quartz_sample_ops;

    TRACE_(quartz)("Created wg_sample %p for IMediaSample %p.\n", &sample->wg_sample, media_sample);
    *out = &sample->wg_sample;
    return S_OK;
}

static const struct wg_sample_ops dmo_sample_ops;

static inline struct sample *unsafe_dmo_from_wg_sample(struct wg_sample *wg_sample)
{
    struct sample *sample = CONTAINING_RECORD(wg_sample, struct sample, wg_sample);
    if (sample->ops != &dmo_sample_ops) return NULL;
    return sample;
}

static void dmo_sample_destroy(struct wg_sample *wg_sample)
{
    struct sample *sample = unsafe_dmo_from_wg_sample(wg_sample);

    TRACE_(mfplat)("wg_sample %p.\n", wg_sample);

    IMediaBuffer_Release(sample->u.dmo.buffer);
}

static const struct wg_sample_ops dmo_sample_ops =
{
    dmo_sample_destroy,
};

HRESULT wg_sample_create_dmo(IMediaBuffer *media_buffer, struct wg_sample **out)
{
    DWORD length, max_length;
    struct sample *sample;
    BYTE *buffer;
    HRESULT hr;

    if (!(sample = calloc(1, sizeof(*sample))))
        return E_OUTOFMEMORY;
    if (FAILED(hr = IMediaBuffer_GetBufferAndLength(media_buffer, &buffer, &length)))
        goto fail;
    if (FAILED(hr = IMediaBuffer_GetMaxLength(media_buffer, &max_length)))
        goto fail;

    IMediaBuffer_AddRef((sample->u.dmo.buffer = media_buffer));
    sample->wg_sample.data = (UINT_PTR)buffer;
    sample->wg_sample.size = length;
    sample->wg_sample.max_size = max_length;
    sample->ops = &dmo_sample_ops;

    *out = &sample->wg_sample;
    TRACE_(mfplat)("Created wg_sample %p for IMediaBuffer %p.\n", *out, media_buffer);
    return S_OK;

fail:
    if (sample->u.dmo.buffer)
        IMediaBuffer_Release(sample->u.dmo.buffer);
    free(sample);
    return hr;
}

void wg_sample_release(struct wg_sample *wg_sample)
{
    struct sample *sample = CONTAINING_RECORD(wg_sample, struct sample, wg_sample);

    if (InterlockedOr(&wg_sample->refcount, 0))
    {
        ERR("wg_sample %p is still in use, trouble ahead!\n", wg_sample);
        return;
    }

    sample->ops->destroy(wg_sample);

    free(sample);
}

static void wg_sample_queue_begin_append(struct wg_sample_queue *queue, struct wg_sample *wg_sample)
{
    struct sample *sample = CONTAINING_RECORD(wg_sample, struct sample, wg_sample);

    /* make sure a concurrent wg_sample_queue_flush call won't release the sample until we're done */
    InterlockedIncrement(&wg_sample->refcount);

    EnterCriticalSection(&queue->cs);
    list_add_tail(&queue->samples, &sample->entry);
    LeaveCriticalSection(&queue->cs);
}

static void wg_sample_queue_end_append(struct wg_sample_queue *queue, struct wg_sample *wg_sample)
{
    /* release temporary ref taken in wg_sample_queue_begin_append */
    InterlockedDecrement(&wg_sample->refcount);

    wg_sample_queue_flush(queue, false);
}

void wg_sample_queue_flush(struct wg_sample_queue *queue, bool all)
{
    struct sample *sample, *next;

    EnterCriticalSection(&queue->cs);

    LIST_FOR_EACH_ENTRY_SAFE(sample, next, &queue->samples, struct sample, entry)
    {
        if (!InterlockedOr(&sample->wg_sample.refcount, 0) || all)
        {
            list_remove(&sample->entry);
            wg_sample_release(&sample->wg_sample);
        }
    }

    LeaveCriticalSection(&queue->cs);
}

HRESULT wg_sample_queue_create(struct wg_sample_queue **out)
{
    struct wg_sample_queue *queue;

    if (!(queue = calloc(1, sizeof(*queue))))
        return E_OUTOFMEMORY;

    InitializeCriticalSectionEx(&queue->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    queue->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": cs");
    list_init(&queue->samples);

    TRACE("Created wg_sample_queue %p.\n", queue);
    *out = queue;

    return S_OK;
}

void wg_sample_queue_destroy(struct wg_sample_queue *queue)
{
    wg_sample_queue_flush(queue, true);

    queue->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&queue->cs);

    free(queue);
}

HRESULT wg_transform_push_mf(wg_transform_t transform, IMFSample *sample,
        struct wg_sample_queue *queue)
{
    struct wg_sample *wg_sample;
    LONGLONG time, duration;
    UINT32 value;
    HRESULT hr;

    TRACE_(mfplat)("transform %#I64x, sample %p, queue %p.\n", transform, sample, queue);

    if (FAILED(hr = wg_sample_create_mf(sample, &wg_sample)))
        return hr;

    if (SUCCEEDED(IMFSample_GetSampleTime(sample, &time)))
    {
        wg_sample->flags |= WG_SAMPLE_FLAG_HAS_PTS;
        wg_sample->pts = time;
    }
    if (SUCCEEDED(IMFSample_GetSampleDuration(sample, &duration)))
    {
        wg_sample->flags |= WG_SAMPLE_FLAG_HAS_DURATION;
        wg_sample->duration = duration;
    }
    if (SUCCEEDED(IMFSample_GetUINT32(sample, &MFSampleExtension_CleanPoint, &value)) && value)
        wg_sample->flags |= WG_SAMPLE_FLAG_SYNC_POINT;
    if (SUCCEEDED(IMFSample_GetUINT32(sample, &MFSampleExtension_Discontinuity, &value)) && value)
        wg_sample->flags |= WG_SAMPLE_FLAG_DISCONTINUITY;

    wg_sample_queue_begin_append(queue, wg_sample);
    hr = wg_transform_push_data(transform, wg_sample);
    wg_sample_queue_end_append(queue, wg_sample);

    return hr;
}

HRESULT wg_transform_read_mf(wg_transform_t transform, IMFSample *sample, DWORD *flags, bool *preserve_timestamps)
{
    struct wg_sample *wg_sample;
    IMFMediaBuffer *buffer;
    HRESULT hr;

    TRACE_(mfplat)("transform %#I64x, sample %p, flags %p.\n", transform, sample, flags);

    if (FAILED(hr = wg_sample_create_mf(sample, &wg_sample)))
        return hr;

    wg_sample->size = 0;

    if (FAILED(hr = wg_transform_read_data(transform, wg_sample)))
    {
        wg_sample_release(wg_sample);
        return hr;
    }

    if (wg_sample->flags & WG_SAMPLE_FLAG_INCOMPLETE)
        *flags |= MFT_OUTPUT_DATA_BUFFER_INCOMPLETE;
    if (wg_sample->flags & WG_SAMPLE_FLAG_HAS_PTS)
        IMFSample_SetSampleTime(sample, wg_sample->pts);
    if (wg_sample->flags & WG_SAMPLE_FLAG_HAS_DURATION)
        IMFSample_SetSampleDuration(sample, wg_sample->duration);
    if (wg_sample->flags & WG_SAMPLE_FLAG_SYNC_POINT)
        IMFSample_SetUINT32(sample, &MFSampleExtension_CleanPoint, 1);
    if (wg_sample->flags & WG_SAMPLE_FLAG_DISCONTINUITY)
        IMFSample_SetUINT32(sample, &MFSampleExtension_Discontinuity, 1);
    if (preserve_timestamps)
        *preserve_timestamps = !!(wg_sample->flags & WG_SAMPLE_FLAG_PRESERVE_TIMESTAMPS);

    if (SUCCEEDED(hr = IMFSample_ConvertToContiguousBuffer(sample, &buffer)))
    {
        hr = IMFMediaBuffer_SetCurrentLength(buffer, wg_sample->size);
        IMFMediaBuffer_Release(buffer);
    }

    wg_sample_release(wg_sample);
    return hr;
}

HRESULT wg_transform_push_quartz(wg_transform_t transform, struct wg_sample *wg_sample,
        struct wg_sample_queue *queue)
{
    struct sample *sample = unsafe_quartz_from_wg_sample(wg_sample);
    REFERENCE_TIME start_time, end_time;
    HRESULT hr;

    TRACE_(quartz)("transform %#I64x, wg_sample %p, queue %p.\n", transform, wg_sample, queue);

    hr = IMediaSample_GetTime(sample->u.quartz.sample, &start_time, &end_time);
    if (SUCCEEDED(hr))
    {
        wg_sample->pts = start_time;
        wg_sample->flags |= WG_SAMPLE_FLAG_HAS_PTS;
    }
    if (hr == S_OK)
    {
        wg_sample->duration = end_time - start_time;
        wg_sample->flags |= WG_SAMPLE_FLAG_HAS_DURATION;
    }

    if (IMediaSample_IsSyncPoint(sample->u.quartz.sample) == S_OK)
        wg_sample->flags |= WG_SAMPLE_FLAG_SYNC_POINT;
    if (IMediaSample_IsDiscontinuity(sample->u.quartz.sample) == S_OK)
        wg_sample->flags |= WG_SAMPLE_FLAG_DISCONTINUITY;

    wg_sample_queue_begin_append(queue, wg_sample);
    hr = wg_transform_push_data(transform, wg_sample);
    wg_sample_queue_end_append(queue, wg_sample);

    return hr;
}

HRESULT wg_transform_read_quartz(wg_transform_t transform, struct wg_sample *wg_sample)
{
    struct sample *sample = unsafe_quartz_from_wg_sample(wg_sample);
    REFERENCE_TIME start_time, end_time;
    HRESULT hr;
    BOOL value;

    TRACE_(mfplat)("transform %#I64x, wg_sample %p.\n", transform, wg_sample);

    if (FAILED(hr = wg_transform_read_data(transform, wg_sample)))
    {
        if (hr == MF_E_TRANSFORM_STREAM_CHANGE)
            FIXME("Unexpected stream format change!\n");
        return hr;
    }

    if (FAILED(hr = IMediaSample_SetActualDataLength(sample->u.quartz.sample, wg_sample->size)))
        return hr;

    if (wg_sample->flags & WG_SAMPLE_FLAG_HAS_PTS)
    {
        start_time = wg_sample->pts;
        if (wg_sample->flags & WG_SAMPLE_FLAG_HAS_DURATION)
        {
            end_time = start_time + wg_sample->duration;
            IMediaSample_SetTime(sample->u.quartz.sample, &start_time, &end_time);
        }
        else
        {
            IMediaSample_SetTime(sample->u.quartz.sample, &start_time, NULL);
        }
    }

    value = !!(wg_sample->flags & WG_SAMPLE_FLAG_SYNC_POINT);
    IMediaSample_SetSyncPoint(sample->u.quartz.sample, value);
    value = !!(wg_sample->flags & WG_SAMPLE_FLAG_DISCONTINUITY);
    IMediaSample_SetDiscontinuity(sample->u.quartz.sample, value);

    return S_OK;
}

HRESULT wg_transform_push_dmo(wg_transform_t transform, IMediaBuffer *media_buffer,
        DWORD flags, REFERENCE_TIME time_stamp, REFERENCE_TIME time_length, struct wg_sample_queue *queue)
{
    struct wg_sample *wg_sample;
    HRESULT hr;

    TRACE_(mfplat)("transform %#I64x, media_buffer %p, flags %#lx, time_stamp %s, time_length %s, queue %p.\n",
            transform, media_buffer, flags, wine_dbgstr_longlong(time_stamp), wine_dbgstr_longlong(time_length), queue);

    if (FAILED(hr = wg_sample_create_dmo(media_buffer, &wg_sample)))
        return hr;

    if (flags & DMO_INPUT_DATA_BUFFERF_SYNCPOINT)
        wg_sample->flags |= WG_SAMPLE_FLAG_SYNC_POINT;
    if (flags & DMO_INPUT_DATA_BUFFERF_TIME)
    {
        wg_sample->flags |= WG_SAMPLE_FLAG_HAS_PTS;
        wg_sample->pts = time_stamp;
    }
    if (flags & DMO_INPUT_DATA_BUFFERF_TIMELENGTH)
    {
        wg_sample->flags |= WG_SAMPLE_FLAG_HAS_DURATION;
        wg_sample->duration = time_length;
    }

    wg_sample_queue_begin_append(queue, wg_sample);
    hr = wg_transform_push_data(transform, wg_sample);
    wg_sample_queue_end_append(queue, wg_sample);

    return hr;
}

HRESULT wg_transform_read_dmo(wg_transform_t transform, DMO_OUTPUT_DATA_BUFFER *buffer)
{
    struct wg_sample *wg_sample;
    HRESULT hr;

    TRACE_(mfplat)("transform %#I64x, buffer %p.\n", transform, buffer);

    if (FAILED(hr = wg_sample_create_dmo(buffer->pBuffer, &wg_sample)))
        return hr;
    wg_sample->size = 0;

    if (FAILED(hr = wg_transform_read_data(transform, wg_sample)))
    {
        if (hr == MF_E_TRANSFORM_STREAM_CHANGE)
            TRACE_(mfplat)("Stream format changed.\n");
        wg_sample_release(wg_sample);
        return hr;
    }

    buffer->dwStatus = 0;
    if (wg_sample->flags & WG_SAMPLE_FLAG_INCOMPLETE)
        buffer->dwStatus |= DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE;
    if (wg_sample->flags & WG_SAMPLE_FLAG_HAS_PTS)
    {
        buffer->dwStatus |= DMO_OUTPUT_DATA_BUFFERF_TIME;
        buffer->rtTimestamp = wg_sample->pts;
    }
    if (wg_sample->flags & WG_SAMPLE_FLAG_HAS_DURATION)
    {
        buffer->dwStatus |= DMO_OUTPUT_DATA_BUFFERF_TIMELENGTH;
        buffer->rtTimelength = wg_sample->duration;
    }
    if (wg_sample->flags & WG_SAMPLE_FLAG_SYNC_POINT)
        buffer->dwStatus |= DMO_OUTPUT_DATA_BUFFERF_SYNCPOINT;

    IMediaBuffer_SetLength(buffer->pBuffer, wg_sample->size);

    wg_sample_release(wg_sample);
    return hr;
}
