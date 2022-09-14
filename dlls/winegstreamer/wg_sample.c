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
    sample->wg_sample.data = buffer;
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
    sample->wg_sample.data = buffer;
    sample->wg_sample.size = current_length;
    sample->wg_sample.max_size = max_length;
    sample->ops = &quartz_sample_ops;

    TRACE_(quartz)("Created wg_sample %p for IMediaSample %p.\n", &sample->wg_sample, media_sample);
    *out = &sample->wg_sample;
    return S_OK;
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

    InitializeCriticalSection(&queue->cs);
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
    InitializeCriticalSection(&queue->cs);

    free(queue);
}

/* These unixlib entry points should not be used directly, they assume samples
 * to be queued and zero-copy support, use the helpers below instead.
 */
HRESULT wg_transform_push_data(struct wg_transform *transform, struct wg_sample *sample);
HRESULT wg_transform_read_data(struct wg_transform *transform, struct wg_sample *sample,
        struct wg_format *format);

HRESULT wg_transform_push_mf(struct wg_transform *transform, IMFSample *sample,
        struct wg_sample_queue *queue)
{
    struct wg_sample *wg_sample;
    LONGLONG time, duration;
    UINT32 value;
    HRESULT hr;

    TRACE_(mfplat)("transform %p, sample %p, queue %p.\n", transform, sample, queue);

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

    wg_sample_queue_begin_append(queue, wg_sample);
    hr = wg_transform_push_data(transform, wg_sample);
    wg_sample_queue_end_append(queue, wg_sample);

    return hr;
}

HRESULT wg_transform_read_mf(struct wg_transform *transform, IMFSample *sample,
        DWORD sample_size, struct wg_format *format, DWORD *flags)
{
    struct wg_sample *wg_sample;
    IMFMediaBuffer *buffer;
    HRESULT hr;

    TRACE_(mfplat)("transform %p, sample %p, format %p, flags %p.\n", transform, sample, format, flags);

    if (FAILED(hr = wg_sample_create_mf(sample, &wg_sample)))
        return hr;

    wg_sample->size = 0;
    if (wg_sample->max_size < sample_size)
    {
        wg_sample_release(wg_sample);
        return MF_E_BUFFERTOOSMALL;
    }

    if (FAILED(hr = wg_transform_read_data(transform, wg_sample, format)))
    {
        if (hr == MF_E_TRANSFORM_STREAM_CHANGE && !format)
            FIXME("Unexpected stream format change!\n");
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

    if (SUCCEEDED(hr = IMFSample_ConvertToContiguousBuffer(sample, &buffer)))
    {
        hr = IMFMediaBuffer_SetCurrentLength(buffer, wg_sample->size);
        IMFMediaBuffer_Release(buffer);
    }

    wg_sample_release(wg_sample);
    return hr;
}

HRESULT wg_transform_push_quartz(struct wg_transform *transform, struct wg_sample *wg_sample,
        struct wg_sample_queue *queue)
{
    struct sample *sample = unsafe_quartz_from_wg_sample(wg_sample);
    REFERENCE_TIME start_time, end_time;
    HRESULT hr;

    TRACE_(quartz)("transform %p, wg_sample %p, queue %p.\n", transform, wg_sample, queue);

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

    wg_sample_queue_begin_append(queue, wg_sample);
    hr = wg_transform_push_data(transform, wg_sample);
    wg_sample_queue_end_append(queue, wg_sample);

    return hr;
}

HRESULT wg_transform_read_quartz(struct wg_transform *transform, struct wg_sample *wg_sample)
{
    struct sample *sample = unsafe_quartz_from_wg_sample(wg_sample);
    REFERENCE_TIME start_time, end_time;
    HRESULT hr;
    BOOL value;

    TRACE_(mfplat)("transform %p, wg_sample %p.\n", transform, wg_sample);

    if (FAILED(hr = wg_transform_read_data(transform, wg_sample, NULL)))
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

    return S_OK;
}
