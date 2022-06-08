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

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

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
    sample->wg_sample.flags |= WG_SAMPLE_FLAG_HAS_REFCOUNT;

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

HRESULT wg_transform_push_mf(struct wg_transform *transform, struct wg_sample *wg_sample,
        struct wg_sample_queue *queue)
{
    struct sample *sample = unsafe_mf_from_wg_sample(wg_sample);
    LONGLONG time, duration;
    UINT32 value;
    HRESULT hr;

    TRACE_(mfplat)("transform %p, wg_sample %p, queue %p.\n", transform, wg_sample, queue);

    if (SUCCEEDED(IMFSample_GetSampleTime(sample->u.mf.sample, &time)))
    {
        sample->wg_sample.flags |= WG_SAMPLE_FLAG_HAS_PTS;
        sample->wg_sample.pts = time;
    }
    if (SUCCEEDED(IMFSample_GetSampleDuration(sample->u.mf.sample, &duration)))
    {
        sample->wg_sample.flags |= WG_SAMPLE_FLAG_HAS_DURATION;
        sample->wg_sample.duration = duration;
    }
    if (SUCCEEDED(IMFSample_GetUINT32(sample->u.mf.sample, &MFSampleExtension_CleanPoint, &value)) && value)
        sample->wg_sample.flags |= WG_SAMPLE_FLAG_SYNC_POINT;

    wg_sample_queue_begin_append(queue, wg_sample);
    hr = wg_transform_push_data(transform, wg_sample);
    wg_sample_queue_end_append(queue, wg_sample);

    return hr;
}

HRESULT wg_transform_read_mf(struct wg_transform *transform, struct wg_sample *wg_sample,
        struct wg_format *format)
{
    struct sample *sample = unsafe_mf_from_wg_sample(wg_sample);
    HRESULT hr;

    TRACE_(mfplat)("transform %p, wg_sample %p, format %p.\n", transform, wg_sample, format);

    if (FAILED(hr = wg_transform_read_data(transform, wg_sample, format)))
        return hr;

    if (FAILED(hr = IMFMediaBuffer_SetCurrentLength(sample->u.mf.buffer, wg_sample->size)))
        return hr;

    if (wg_sample->flags & WG_SAMPLE_FLAG_HAS_PTS)
        IMFSample_SetSampleTime(sample->u.mf.sample, wg_sample->pts);
    if (wg_sample->flags & WG_SAMPLE_FLAG_HAS_DURATION)
        IMFSample_SetSampleDuration(sample->u.mf.sample, wg_sample->duration);
    if (wg_sample->flags & WG_SAMPLE_FLAG_SYNC_POINT)
        IMFSample_SetUINT32(sample->u.mf.sample, &MFSampleExtension_CleanPoint, 1);

    return S_OK;
}
