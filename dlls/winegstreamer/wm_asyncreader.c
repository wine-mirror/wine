/*
 * Copyright 2012 Austin English
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

#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(wmvcore);

union async_op_data
{
    struct
    {
        QWORD start;
        QWORD duration;
        void *context;
    } start;
};

struct async_op
{
    enum async_op_type
    {
        ASYNC_OP_START,
        ASYNC_OP_STOP,
        ASYNC_OP_CLOSE,
    } type;
    union async_op_data u;
    struct list entry;
};

struct sample
{
    INSSBuffer *buffer;
    QWORD pts, duration;
    DWORD flags;
    WORD stream;
};

struct async_reader
{
    IWMReader IWMReader_iface;
    IWMReaderAdvanced6 IWMReaderAdvanced6_iface;
    IWMReaderAccelerator IWMReaderAccelerator_iface;
    IWMReaderNetworkConfig2 IWMReaderNetworkConfig2_iface;
    IWMReaderStreamClock IWMReaderStreamClock_iface;
    IWMReaderTypeNegotiation IWMReaderTypeNegotiation_iface;
    IReferenceClock IReferenceClock_iface;
    IUnknown *reader_inner;
    LONG refcount;

    IWMSyncReader2 *reader;
    struct wm_reader *wm_reader;

    CRITICAL_SECTION cs;

    IWMReaderCallbackAdvanced *callback_advanced;
    IWMReaderCallback *callback;
    void *context;

    REFERENCE_TIME clock_start;
    LARGE_INTEGER clock_frequency;

    HANDLE callback_thread;
    CRITICAL_SECTION callback_cs;
    CONDITION_VARIABLE callback_cv;

    bool running;
    struct list async_ops;

    bool user_clock;
    QWORD user_time;
};

static REFERENCE_TIME get_current_time(const struct async_reader *reader)
{
    LARGE_INTEGER time;

    QueryPerformanceCounter(&time);
    return (time.QuadPart * 1000) / reader->clock_frequency.QuadPart * 10000;
}

static DWORD async_reader_get_wait_timeout(struct async_reader *reader, QWORD pts)
{
    REFERENCE_TIME current_time = reader->user_time;
    DWORD timeout = INFINITE;

    if (!reader->user_clock)
    {
        current_time = get_current_time(reader) - reader->clock_start;
        timeout = (pts - current_time) / 10000;
    }

    return pts > current_time ? timeout : 0;
}

static bool async_reader_wait_pts(struct async_reader *reader, QWORD pts)
{
    IWMReaderCallbackAdvanced *callback_advanced = reader->callback_advanced;
    DWORD timeout;

    TRACE("reader %p, pts %I64d.\n", reader, pts);

    if (reader->user_clock && pts > reader->user_time && callback_advanced)
    {
        QWORD user_time = reader->user_time;
        LeaveCriticalSection(&reader->callback_cs);
        IWMReaderCallbackAdvanced_OnTime(callback_advanced, user_time, reader->context);
        EnterCriticalSection(&reader->callback_cs);
    }

    while (reader->running && list_empty(&reader->async_ops))
    {
        if (!(timeout = async_reader_get_wait_timeout(reader, pts)))
            return true;
        SleepConditionVariableCS(&reader->callback_cv, &reader->callback_cs, timeout);
    }

    return false;
}

static void async_reader_deliver_sample(struct async_reader *reader, struct sample *sample)
{
    IWMReaderCallbackAdvanced *callback_advanced = reader->callback_advanced;
    IWMReaderCallback *callback = reader->callback;
    BOOL read_compressed;
    HRESULT hr;

    TRACE("reader %p, stream %u, pts %s, duration %s, flags %#lx, buffer %p.\n",
            reader, sample->stream, debugstr_time(sample->pts), debugstr_time(sample->duration),
            sample->flags, sample->buffer);

    if (FAILED(hr = IWMSyncReader2_GetReadStreamSamples(reader->reader, sample->stream,
            &read_compressed)))
        read_compressed = FALSE;

    LeaveCriticalSection(&reader->callback_cs);
    if (read_compressed)
        hr = IWMReaderCallbackAdvanced_OnStreamSample(callback_advanced, sample->stream,
                sample->pts, sample->duration, sample->flags, sample->buffer, reader->context);
    else
        hr = IWMReaderCallback_OnSample(callback, sample->stream - 1, sample->pts, sample->duration,
                sample->flags, sample->buffer, reader->context);
    EnterCriticalSection(&reader->callback_cs);

    TRACE("Callback returned %#lx.\n", hr);

    INSSBuffer_Release(sample->buffer);
}

static void callback_thread_run(struct async_reader *reader)
{
    IWMReaderCallbackAdvanced *callback_advanced = reader->callback_advanced;
    IWMReaderCallback *callback = reader->callback;
    static const DWORD zero;
    HRESULT hr = S_OK;

    while (reader->running && list_empty(&reader->async_ops))
    {
        struct sample sample;

        LeaveCriticalSection(&reader->callback_cs);
        hr = wm_reader_get_stream_sample(reader->wm_reader, callback_advanced, 0, &sample.buffer,
                &sample.pts, &sample.duration, &sample.flags, &sample.stream);
        EnterCriticalSection(&reader->callback_cs);
        if (hr != S_OK)
            break;

        if (async_reader_wait_pts(reader, sample.pts))
            async_reader_deliver_sample(reader, &sample);
        else
            INSSBuffer_Release(sample.buffer);
    }

    if (hr == NS_E_NO_MORE_SAMPLES)
    {
        BOOL user_clock = reader->user_clock;
        QWORD user_time = reader->user_time;

        LeaveCriticalSection(&reader->callback_cs);

        IWMReaderCallback_OnStatus(callback, WMT_END_OF_STREAMING, S_OK,
                WMT_TYPE_DWORD, (BYTE *)&zero, reader->context);
        IWMReaderCallback_OnStatus(callback, WMT_EOF, S_OK,
                WMT_TYPE_DWORD, (BYTE *)&zero, reader->context);

        if (user_clock && callback_advanced)
        {
            /* We can only get here if user_time is greater than the PTS
             * of all samples, in which case we cannot have sent this
             * notification already. */
            IWMReaderCallbackAdvanced_OnTime(callback_advanced,
                    user_time, reader->context);
        }

        EnterCriticalSection(&reader->callback_cs);

        TRACE("Reached end of stream; exiting.\n");
    }
    else if (hr != S_OK)
    {
        ERR("Failed to get sample, hr %#lx.\n", hr);
    }
}

static DWORD WINAPI async_reader_callback_thread(void *arg)
{
    struct async_reader *reader = arg;
    static const DWORD zero;
    struct list *entry;
    HRESULT hr = S_OK;

    IWMReaderCallback_OnStatus(reader->callback, WMT_OPENED, S_OK,
            WMT_TYPE_DWORD, (BYTE *)&zero, reader->context);

    EnterCriticalSection(&reader->callback_cs);

    while (reader->running)
    {
        if ((entry = list_head(&reader->async_ops)))
        {
            struct async_op *op = LIST_ENTRY(entry, struct async_op, entry);
            list_remove(&op->entry);

            hr = list_empty(&reader->async_ops) ? S_OK : E_ABORT;
            switch (op->type)
            {
                case ASYNC_OP_START:
                {
                    reader->context = op->u.start.context;
                    if (SUCCEEDED(hr))
                        hr = IWMSyncReader2_SetRange(reader->reader, op->u.start.start, op->u.start.duration);
                    if (SUCCEEDED(hr))
                        reader->clock_start = get_current_time(reader);

                    LeaveCriticalSection(&reader->callback_cs);
                    IWMReaderCallback_OnStatus(reader->callback, WMT_STARTED, hr,
                            WMT_TYPE_DWORD, (BYTE *)&zero, reader->context);
                    EnterCriticalSection(&reader->callback_cs);

                    if (SUCCEEDED(hr))
                        callback_thread_run(reader);
                    break;
                }

                case ASYNC_OP_STOP:
                    LeaveCriticalSection(&reader->callback_cs);
                    IWMReaderCallback_OnStatus(reader->callback, WMT_STOPPED, hr,
                            WMT_TYPE_DWORD, (BYTE *)&zero, reader->context);
                    EnterCriticalSection(&reader->callback_cs);
                    break;

                case ASYNC_OP_CLOSE:
                    LeaveCriticalSection(&reader->callback_cs);
                    IWMReaderCallback_OnStatus(reader->callback, WMT_CLOSED, hr,
                            WMT_TYPE_DWORD, (BYTE *)&zero, reader->context);
                    EnterCriticalSection(&reader->callback_cs);

                    if (SUCCEEDED(hr))
                        reader->running = false;
                    break;
            }

            free(op);
        }

        if (reader->running && list_empty(&reader->async_ops))
            SleepConditionVariableCS(&reader->callback_cv, &reader->callback_cs, INFINITE);
    }

    LeaveCriticalSection(&reader->callback_cs);

    TRACE("Reader is stopping; exiting.\n");
    return 0;
}

static void async_reader_close(struct async_reader *reader)
{
    struct async_op *op, *next;

    if (reader->callback_thread)
    {
        WaitForSingleObject(reader->callback_thread, INFINITE);
        CloseHandle(reader->callback_thread);
        reader->callback_thread = NULL;
    }

    LIST_FOR_EACH_ENTRY_SAFE(op, next, &reader->async_ops, struct async_op, entry)
    {
        list_remove(&op->entry);
        free(op);
    }

    if (reader->callback_advanced)
        IWMReaderCallbackAdvanced_Release(reader->callback_advanced);
    reader->callback_advanced = NULL;

    if (reader->callback)
        IWMReaderCallback_Release(reader->callback);
    reader->callback = NULL;
    reader->context = NULL;
}

static HRESULT async_reader_open(struct async_reader *reader, IWMReaderCallback *callback, void *context)
{
    HRESULT hr = E_OUTOFMEMORY;

    IWMReaderCallback_AddRef((reader->callback = callback));
    reader->context = context;

    if (FAILED(hr = IWMReaderCallback_QueryInterface(callback, &IID_IWMReaderCallbackAdvanced,
            (void **)&reader->callback_advanced)))
    {
        WARN("Failed to retrieve IWMReaderCallbackAdvanced interface, hr %#lx\n", hr);
        reader->callback_advanced = NULL;
    }

    reader->running = true;
    if (!(reader->callback_thread = CreateThread(NULL, 0, async_reader_callback_thread, reader, 0, NULL)))
        goto error;

    return S_OK;

error:
    async_reader_close(reader);
    return hr;
}

static HRESULT async_reader_queue_op(struct async_reader *reader, enum async_op_type type, union async_op_data *data)
{
    struct async_op *op;

    if (!(op = calloc(1, sizeof(*op))))
        return E_OUTOFMEMORY;
    op->type = type;
    if (data)
        op->u = *data;

    EnterCriticalSection(&reader->callback_cs);
    list_add_tail(&reader->async_ops, &op->entry);
    LeaveCriticalSection(&reader->callback_cs);
    WakeConditionVariable(&reader->callback_cv);

    return S_OK;
}

static struct async_reader *impl_from_IWMReader(IWMReader *iface)
{
    return CONTAINING_RECORD(iface, struct async_reader, IWMReader_iface);
}

static HRESULT WINAPI WMReader_QueryInterface(IWMReader *iface, REFIID iid, void **out)
{
    struct async_reader *reader = impl_from_IWMReader(iface);

    TRACE("reader %p, iid %s, out %p.\n", reader, debugstr_guid(iid), out);

    if (IsEqualIID(iid, &IID_IUnknown)
            || IsEqualIID(iid, &IID_IWMReader))
        *out = &reader->IWMReader_iface;
    else if (IsEqualIID(iid, &IID_IWMReaderAccelerator))
        *out = &reader->IWMReaderAccelerator_iface;
    else if (IsEqualIID(iid, &IID_IWMReaderAdvanced)
            || IsEqualIID(iid, &IID_IWMReaderAdvanced2)
            || IsEqualIID(iid, &IID_IWMReaderAdvanced3)
            || IsEqualIID(iid, &IID_IWMReaderAdvanced4)
            || IsEqualIID(iid, &IID_IWMReaderAdvanced5)
            || IsEqualIID(iid, &IID_IWMReaderAdvanced6))
        *out = &reader->IWMReaderAdvanced6_iface;
    else if (IsEqualIID(iid, &IID_IWMReaderNetworkConfig)
            || IsEqualIID(iid, &IID_IWMReaderNetworkConfig2))
        *out = &reader->IWMReaderNetworkConfig2_iface;
    else if (IsEqualIID(iid, &IID_IWMReaderStreamClock))
        *out = &reader->IWMReaderStreamClock_iface;
    else if (IsEqualIID(iid, &IID_IWMReaderTypeNegotiation))
        *out = &reader->IWMReaderTypeNegotiation_iface;
    else if (IsEqualIID(iid, &IID_IWMHeaderInfo)
            || IsEqualIID(iid, &IID_IWMHeaderInfo2)
            || IsEqualIID(iid, &IID_IWMHeaderInfo3)
            || IsEqualIID(iid, &IID_IWMLanguageList)
            || IsEqualIID(iid, &IID_IWMPacketSize)
            || IsEqualIID(iid, &IID_IWMPacketSize2)
            || IsEqualIID(iid, &IID_IWMProfile)
            || IsEqualIID(iid, &IID_IWMProfile2)
            || IsEqualIID(iid, &IID_IWMProfile3)
            || IsEqualIID(iid, &IID_IWMReaderPlaylistBurn)
            || IsEqualIID(iid, &IID_IWMReaderTimecode))
        return IUnknown_QueryInterface(reader->reader_inner, iid, out);
    else if (IsEqualIID(iid, &IID_IReferenceClock))
        *out = &reader->IReferenceClock_iface;
    else
    {
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI WMReader_AddRef(IWMReader *iface)
{
    struct async_reader *reader = impl_from_IWMReader(iface);
    ULONG refcount = InterlockedIncrement(&reader->refcount);
    TRACE("%p increasing refcount to %lu.\n", reader, refcount);
    return refcount;
}

static ULONG WINAPI WMReader_Release(IWMReader *iface)
{
    struct async_reader *reader = impl_from_IWMReader(iface);
    ULONG refcount = InterlockedDecrement(&reader->refcount);

    TRACE("%p decreasing refcount to %lu.\n", reader, refcount);

    if (!refcount)
    {
        EnterCriticalSection(&reader->callback_cs);
        reader->running = false;
        LeaveCriticalSection(&reader->callback_cs);
        WakeConditionVariable(&reader->callback_cv);

        async_reader_close(reader);

        reader->callback_cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&reader->callback_cs);
        reader->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&reader->cs);

        IWMSyncReader2_Close(reader->reader);

        IUnknown_Release(reader->reader_inner);
        free(reader);
    }

    return refcount;
}

static HRESULT WINAPI WMReader_Open(IWMReader *iface, const WCHAR *url,
        IWMReaderCallback *callback, void *context)
{
    struct async_reader *reader = impl_from_IWMReader(iface);
    HRESULT hr;

    TRACE("reader %p, url %s, callback %p, context %p.\n",
            reader, debugstr_w(url), callback, context);

    EnterCriticalSection(&reader->cs);

    if (SUCCEEDED(hr = IWMSyncReader2_Open(reader->reader, url))
            && FAILED(hr = async_reader_open(reader, callback, context)))
        IWMSyncReader2_Close(reader->reader);

    LeaveCriticalSection(&reader->cs);
    return hr;
}

static HRESULT WINAPI WMReader_Close(IWMReader *iface)
{
    struct async_reader *reader = impl_from_IWMReader(iface);
    HRESULT hr;

    TRACE("reader %p.\n", reader);

    EnterCriticalSection(&reader->cs);

    if (SUCCEEDED(hr = async_reader_queue_op(reader, ASYNC_OP_CLOSE, NULL)))
    {
        async_reader_close(reader);
        hr = IWMSyncReader2_Close(reader->reader);
    }

    LeaveCriticalSection(&reader->cs);

    return hr;
}

static HRESULT WINAPI WMReader_GetOutputCount(IWMReader *iface, DWORD *count)
{
    struct async_reader *reader = impl_from_IWMReader(iface);

    TRACE("reader %p, count %p.\n", reader, count);

    return IWMSyncReader2_GetOutputCount(reader->reader, count);
}

static HRESULT WINAPI WMReader_GetOutputProps(IWMReader *iface, DWORD output, IWMOutputMediaProps **props)
{
    struct async_reader *reader = impl_from_IWMReader(iface);

    TRACE("reader %p, output %lu, props %p.\n", reader, output, props);

    return IWMSyncReader2_GetOutputProps(reader->reader, output, props);
}

static HRESULT WINAPI WMReader_SetOutputProps(IWMReader *iface, DWORD output, IWMOutputMediaProps *props)
{
    struct async_reader *reader = impl_from_IWMReader(iface);

    TRACE("reader %p, output %lu, props %p.\n", reader, output, props);

    return IWMSyncReader2_SetOutputProps(reader->reader, output, props);
}

static HRESULT WINAPI WMReader_GetOutputFormatCount(IWMReader *iface, DWORD output, DWORD *count)
{
    struct async_reader *reader = impl_from_IWMReader(iface);

    TRACE("reader %p, output %lu, count %p.\n", reader, output, count);

    return IWMSyncReader2_GetOutputFormatCount(reader->reader, output, count);
}

static HRESULT WINAPI WMReader_GetOutputFormat(IWMReader *iface, DWORD output,
        DWORD index, IWMOutputMediaProps **props)
{
    struct async_reader *reader = impl_from_IWMReader(iface);

    TRACE("reader %p, output %lu, index %lu, props %p.\n", reader, output, index, props);

    return IWMSyncReader2_GetOutputFormat(reader->reader, output, index, props);
}

static HRESULT WINAPI WMReader_Start(IWMReader *iface,
        QWORD start, QWORD duration, float rate, void *context)
{
    union async_op_data data = {.start = {.start = start, .duration = duration, .context = context}};
    struct async_reader *reader = impl_from_IWMReader(iface);
    HRESULT hr;

    TRACE("reader %p, start %s, duration %s, rate %.8e, context %p.\n",
            reader, debugstr_time(start), debugstr_time(duration), rate, context);

    if (rate != 1.0f)
        FIXME("Ignoring rate %.8e.\n", rate);

    EnterCriticalSection(&reader->cs);

    if (!reader->callback_thread)
        hr = NS_E_INVALID_REQUEST;
    else
        hr = async_reader_queue_op(reader, ASYNC_OP_START, &data);

    LeaveCriticalSection(&reader->cs);

    return hr;
}

static HRESULT WINAPI WMReader_Stop(IWMReader *iface)
{
    struct async_reader *reader = impl_from_IWMReader(iface);
    HRESULT hr;

    TRACE("reader %p.\n", reader);

    EnterCriticalSection(&reader->cs);

    if (!reader->callback_thread)
        hr = E_UNEXPECTED;
    else
        hr = async_reader_queue_op(reader, ASYNC_OP_STOP, NULL);

    LeaveCriticalSection(&reader->cs);

    return hr;
}

static HRESULT WINAPI WMReader_Pause(IWMReader *iface)
{
    struct async_reader *This = impl_from_IWMReader(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReader_Resume(IWMReader *iface)
{
    struct async_reader *This = impl_from_IWMReader(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static const IWMReaderVtbl WMReaderVtbl = {
    WMReader_QueryInterface,
    WMReader_AddRef,
    WMReader_Release,
    WMReader_Open,
    WMReader_Close,
    WMReader_GetOutputCount,
    WMReader_GetOutputProps,
    WMReader_SetOutputProps,
    WMReader_GetOutputFormatCount,
    WMReader_GetOutputFormat,
    WMReader_Start,
    WMReader_Stop,
    WMReader_Pause,
    WMReader_Resume
};

static struct async_reader *impl_from_IWMReaderAdvanced6(IWMReaderAdvanced6 *iface)
{
    return CONTAINING_RECORD(iface, struct async_reader, IWMReaderAdvanced6_iface);
}

static HRESULT WINAPI WMReaderAdvanced_QueryInterface(IWMReaderAdvanced6 *iface, REFIID iid, void **out)
{
    struct async_reader *reader = impl_from_IWMReaderAdvanced6(iface);
    return IWMReader_QueryInterface(&reader->IWMReader_iface, iid, out);
}

static ULONG WINAPI WMReaderAdvanced_AddRef(IWMReaderAdvanced6 *iface)
{
    struct async_reader *reader = impl_from_IWMReaderAdvanced6(iface);
    return IWMReader_AddRef(&reader->IWMReader_iface);
}

static ULONG WINAPI WMReaderAdvanced_Release(IWMReaderAdvanced6 *iface)
{
    struct async_reader *reader = impl_from_IWMReaderAdvanced6(iface);
    return IWMReader_Release(&reader->IWMReader_iface);
}

static HRESULT WINAPI WMReaderAdvanced_SetUserProvidedClock(IWMReaderAdvanced6 *iface, BOOL user_clock)
{
    struct async_reader *reader = impl_from_IWMReaderAdvanced6(iface);

    TRACE("reader %p, user_clock %d.\n", reader, user_clock);

    EnterCriticalSection(&reader->callback_cs);
    reader->user_clock = !!user_clock;
    LeaveCriticalSection(&reader->callback_cs);
    WakeConditionVariable(&reader->callback_cv);
    return S_OK;
}

static HRESULT WINAPI WMReaderAdvanced_GetUserProvidedClock(IWMReaderAdvanced6 *iface, BOOL *user_clock)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p)\n", This, user_clock);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_DeliverTime(IWMReaderAdvanced6 *iface, QWORD time)
{
    struct async_reader *reader = impl_from_IWMReaderAdvanced6(iface);

    TRACE("reader %p, time %s.\n", reader, debugstr_time(time));

    EnterCriticalSection(&reader->callback_cs);

    if (!reader->user_clock)
    {
        LeaveCriticalSection(&reader->callback_cs);
        WARN("Not using a user-provided clock; returning E_UNEXPECTED.\n");
        return E_UNEXPECTED;
    }

    reader->user_time = time;

    LeaveCriticalSection(&reader->callback_cs);
    WakeConditionVariable(&reader->callback_cv);
    return S_OK;
}

static HRESULT WINAPI WMReaderAdvanced_SetManualStreamSelection(IWMReaderAdvanced6 *iface, BOOL selection)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%x)\n", This, selection);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_GetManualStreamSelection(IWMReaderAdvanced6 *iface, BOOL *selection)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p)\n", This, selection);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_SetStreamsSelected(IWMReaderAdvanced6 *iface,
        WORD count, WORD *stream_numbers, WMT_STREAM_SELECTION *selections)
{
    struct async_reader *reader = impl_from_IWMReaderAdvanced6(iface);

    TRACE("reader %p, count %u, stream_numbers %p, selections %p.\n",
            reader, count, stream_numbers, selections);

    return IWMSyncReader2_SetStreamsSelected(reader->reader, count, stream_numbers, selections);
}

static HRESULT WINAPI WMReaderAdvanced_GetStreamSelected(IWMReaderAdvanced6 *iface,
        WORD stream_number, WMT_STREAM_SELECTION *selection)
{
    struct async_reader *reader = impl_from_IWMReaderAdvanced6(iface);

    TRACE("reader %p, stream_number %u, selection %p.\n", reader, stream_number, selection);

    return IWMSyncReader2_GetStreamSelected(reader->reader, stream_number, selection);
}

static HRESULT WINAPI WMReaderAdvanced_SetReceiveSelectionCallbacks(IWMReaderAdvanced6 *iface, BOOL get_callbacks)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%x)\n", This, get_callbacks);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_GetReceiveSelectionCallbacks(IWMReaderAdvanced6 *iface, BOOL *get_callbacks)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p)\n", This, get_callbacks);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_SetReceiveStreamSamples(IWMReaderAdvanced6 *iface,
        WORD stream_number, BOOL compressed)
{
    struct async_reader *reader = impl_from_IWMReaderAdvanced6(iface);

    TRACE("reader %p, stream_number %u, compressed %d.\n", reader, stream_number, compressed);

    return IWMSyncReader2_SetReadStreamSamples(reader->reader, stream_number, compressed);
}

static HRESULT WINAPI WMReaderAdvanced_GetReceiveStreamSamples(IWMReaderAdvanced6 *iface, WORD stream_num,
        BOOL *receive_stream_samples)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%d %p)\n", This, stream_num, receive_stream_samples);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_SetAllocateForOutput(IWMReaderAdvanced6 *iface,
        DWORD output, BOOL allocate)
{
    struct async_reader *reader = impl_from_IWMReaderAdvanced6(iface);

    TRACE("reader %p, output %lu, allocate %d.\n", reader, output, allocate);

    return wm_reader_set_allocate_for_output(reader->wm_reader, output, allocate);
}

static HRESULT WINAPI WMReaderAdvanced_GetAllocateForOutput(IWMReaderAdvanced6 *iface, DWORD output_num, BOOL *allocate)
{
    struct async_reader *reader = impl_from_IWMReaderAdvanced6(iface);

    FIXME("reader %p, output %lu, allocate %p, stub!\n", reader, output_num, allocate);

    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_SetAllocateForStream(IWMReaderAdvanced6 *iface,
        WORD stream_number, BOOL allocate)
{
    struct async_reader *reader = impl_from_IWMReaderAdvanced6(iface);

    TRACE("reader %p, stream_number %u, allocate %d.\n", reader, stream_number, allocate);

    return wm_reader_set_allocate_for_stream(reader->wm_reader, stream_number, allocate);
}

static HRESULT WINAPI WMReaderAdvanced_GetAllocateForStream(IWMReaderAdvanced6 *iface, WORD output_num, BOOL *allocate)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%d %p)\n", This, output_num, allocate);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_GetStatistics(IWMReaderAdvanced6 *iface, WM_READER_STATISTICS *statistics)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p)\n", This, statistics);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_SetClientInfo(IWMReaderAdvanced6 *iface, WM_READER_CLIENTINFO *client_info)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p)\n", This, client_info);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_GetMaxOutputSampleSize(IWMReaderAdvanced6 *iface, DWORD output, DWORD *max)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%lu %p)\n", This, output, max);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced_GetMaxStreamSampleSize(IWMReaderAdvanced6 *iface,
        WORD stream_number, DWORD *size)
{
    struct async_reader *reader = impl_from_IWMReaderAdvanced6(iface);

    TRACE("reader %p, stream_number %u, size %p.\n", reader, stream_number, size);

    return IWMSyncReader2_GetMaxStreamSampleSize(reader->reader, stream_number, size);
}

static HRESULT WINAPI WMReaderAdvanced_NotifyLateDelivery(IWMReaderAdvanced6 *iface, QWORD lateness)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%s)\n", This, wine_dbgstr_longlong(lateness));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_SetPlayMode(IWMReaderAdvanced6 *iface, WMT_PLAY_MODE mode)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%d)\n", This, mode);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_GetPlayMode(IWMReaderAdvanced6 *iface, WMT_PLAY_MODE *mode)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p)\n", This, mode);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_GetBufferProgress(IWMReaderAdvanced6 *iface, DWORD *percent, QWORD *buffering)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p %p)\n", This, percent, buffering);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_GetDownloadProgress(IWMReaderAdvanced6 *iface, DWORD *percent,
        QWORD *bytes_downloaded, QWORD *download)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p %p %p)\n", This, percent, bytes_downloaded, download);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_GetSaveAsProgress(IWMReaderAdvanced6 *iface, DWORD *percent)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p)\n", This, percent);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_SaveFileAs(IWMReaderAdvanced6 *iface, const WCHAR *filename)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(filename));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_GetProtocolName(IWMReaderAdvanced6 *iface, WCHAR *protocol, DWORD *protocol_len)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p %p)\n", This, protocol, protocol_len);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_StartAtMarker(IWMReaderAdvanced6 *iface, WORD marker_index,
        QWORD duration, float rate, void *context)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%d %s %f %p)\n", This, marker_index, wine_dbgstr_longlong(duration), rate, context);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_GetOutputSetting(IWMReaderAdvanced6 *iface, DWORD output_num,
        const WCHAR *name, WMT_ATTR_DATATYPE *type, BYTE *value, WORD *length)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%lu %s %p %p %p)\n", This, output_num, debugstr_w(name), type, value, length);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_SetOutputSetting(IWMReaderAdvanced6 *iface, DWORD output_num,
        const WCHAR *name, WMT_ATTR_DATATYPE type, const BYTE *value, WORD length)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%lu %s %#x %p %u)\n", This, output_num, debugstr_w(name), type, value, length);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_Preroll(IWMReaderAdvanced6 *iface, QWORD start, QWORD duration, float rate)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%s %s %f)\n", This, wine_dbgstr_longlong(start), wine_dbgstr_longlong(duration), rate);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_SetLogClientID(IWMReaderAdvanced6 *iface, BOOL log_client_id)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%x)\n", This, log_client_id);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_GetLogClientID(IWMReaderAdvanced6 *iface, BOOL *log_client_id)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p)\n", This, log_client_id);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_StopBuffering(IWMReaderAdvanced6 *iface)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced2_OpenStream(IWMReaderAdvanced6 *iface,
        IStream *stream, IWMReaderCallback *callback, void *context)
{
    struct async_reader *reader = impl_from_IWMReaderAdvanced6(iface);
    HRESULT hr;

    TRACE("reader %p, stream %p, callback %p, context %p.\n", reader, stream, callback, context);

    EnterCriticalSection(&reader->cs);

    if (SUCCEEDED(hr = IWMSyncReader2_OpenStream(reader->reader, stream))
            && FAILED(hr = async_reader_open(reader, callback, context)))
        IWMSyncReader2_Close(reader->reader);

    LeaveCriticalSection(&reader->cs);
    return hr;
}

static HRESULT WINAPI WMReaderAdvanced3_StopNetStreaming(IWMReaderAdvanced6 *iface)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced3_StartAtPosition(IWMReaderAdvanced6 *iface, WORD stream_num,
        void *offset_start, void *duration, WMT_OFFSET_FORMAT format, float rate, void *context)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%d %p %p %d %f %p)\n", This, stream_num, offset_start, duration, format, rate, context);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced4_GetLanguageCount(IWMReaderAdvanced6 *iface, DWORD output_num, WORD *language_count)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%lu %p)\n", This, output_num, language_count);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced4_GetLanguage(IWMReaderAdvanced6 *iface, DWORD output_num,
       WORD language, WCHAR *language_string, WORD *language_string_len)
{
    struct async_reader *reader = impl_from_IWMReaderAdvanced6(iface);

    FIXME("reader %p, output %lu, language %#x, language_string %p, language_string_len %p, stub!\n",
            reader, output_num, language, language_string, language_string_len);

    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced4_GetMaxSpeedFactor(IWMReaderAdvanced6 *iface, double *factor)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p)\n", This, factor);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced4_IsUsingFastCache(IWMReaderAdvanced6 *iface, BOOL *using_fast_cache)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p)\n", This, using_fast_cache);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced4_AddLogParam(IWMReaderAdvanced6 *iface, const WCHAR *namespace,
        const WCHAR *name, const WCHAR *value)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%s %s %s)\n", This, debugstr_w(namespace), debugstr_w(name), debugstr_w(value));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced4_SendLogParams(IWMReaderAdvanced6 *iface)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced4_CanSaveFileAs(IWMReaderAdvanced6 *iface, BOOL *can_save)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p)\n", This, can_save);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced4_CancelSaveFileAs(IWMReaderAdvanced6 *iface)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced4_GetURL(IWMReaderAdvanced6 *iface, WCHAR *url, DWORD *url_len)
{
    struct async_reader *This = impl_from_IWMReaderAdvanced6(iface);
    FIXME("(%p)->(%p %p)\n", This, url, url_len);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced5_SetPlayerHook(IWMReaderAdvanced6 *iface, DWORD output_num, IWMPlayerHook *hook)
{
    struct async_reader *reader = impl_from_IWMReaderAdvanced6(iface);

    FIXME("reader %p, output %lu, hook %p, stub!\n", reader, output_num, hook);

    return E_NOTIMPL;
}

static HRESULT WINAPI WMReaderAdvanced6_SetProtectStreamSamples(IWMReaderAdvanced6 *iface, BYTE *cert,
        DWORD cert_size, DWORD cert_type, DWORD flags, BYTE *initialization_vector, DWORD *initialization_vector_size)
{
    struct async_reader *reader = impl_from_IWMReaderAdvanced6(iface);

    FIXME("reader %p, cert %p, cert_size %lu, cert_type %#lx, flags %#lx, vector %p, vector_size %p, stub!\n",
            reader, cert, cert_size, cert_type, flags, initialization_vector, initialization_vector_size);

    return E_NOTIMPL;
}

static const IWMReaderAdvanced6Vtbl WMReaderAdvanced6Vtbl = {
    WMReaderAdvanced_QueryInterface,
    WMReaderAdvanced_AddRef,
    WMReaderAdvanced_Release,
    WMReaderAdvanced_SetUserProvidedClock,
    WMReaderAdvanced_GetUserProvidedClock,
    WMReaderAdvanced_DeliverTime,
    WMReaderAdvanced_SetManualStreamSelection,
    WMReaderAdvanced_GetManualStreamSelection,
    WMReaderAdvanced_SetStreamsSelected,
    WMReaderAdvanced_GetStreamSelected,
    WMReaderAdvanced_SetReceiveSelectionCallbacks,
    WMReaderAdvanced_GetReceiveSelectionCallbacks,
    WMReaderAdvanced_SetReceiveStreamSamples,
    WMReaderAdvanced_GetReceiveStreamSamples,
    WMReaderAdvanced_SetAllocateForOutput,
    WMReaderAdvanced_GetAllocateForOutput,
    WMReaderAdvanced_SetAllocateForStream,
    WMReaderAdvanced_GetAllocateForStream,
    WMReaderAdvanced_GetStatistics,
    WMReaderAdvanced_SetClientInfo,
    WMReaderAdvanced_GetMaxOutputSampleSize,
    WMReaderAdvanced_GetMaxStreamSampleSize,
    WMReaderAdvanced_NotifyLateDelivery,
    WMReaderAdvanced2_SetPlayMode,
    WMReaderAdvanced2_GetPlayMode,
    WMReaderAdvanced2_GetBufferProgress,
    WMReaderAdvanced2_GetDownloadProgress,
    WMReaderAdvanced2_GetSaveAsProgress,
    WMReaderAdvanced2_SaveFileAs,
    WMReaderAdvanced2_GetProtocolName,
    WMReaderAdvanced2_StartAtMarker,
    WMReaderAdvanced2_GetOutputSetting,
    WMReaderAdvanced2_SetOutputSetting,
    WMReaderAdvanced2_Preroll,
    WMReaderAdvanced2_SetLogClientID,
    WMReaderAdvanced2_GetLogClientID,
    WMReaderAdvanced2_StopBuffering,
    WMReaderAdvanced2_OpenStream,
    WMReaderAdvanced3_StopNetStreaming,
    WMReaderAdvanced3_StartAtPosition,
    WMReaderAdvanced4_GetLanguageCount,
    WMReaderAdvanced4_GetLanguage,
    WMReaderAdvanced4_GetMaxSpeedFactor,
    WMReaderAdvanced4_IsUsingFastCache,
    WMReaderAdvanced4_AddLogParam,
    WMReaderAdvanced4_SendLogParams,
    WMReaderAdvanced4_CanSaveFileAs,
    WMReaderAdvanced4_CancelSaveFileAs,
    WMReaderAdvanced4_GetURL,
    WMReaderAdvanced5_SetPlayerHook,
    WMReaderAdvanced6_SetProtectStreamSamples
};

static struct async_reader *impl_from_IWMReaderAccelerator(IWMReaderAccelerator *iface)
{
    return CONTAINING_RECORD(iface, struct async_reader, IWMReaderAccelerator_iface);
}

static HRESULT WINAPI reader_accl_QueryInterface(IWMReaderAccelerator *iface, REFIID iid, void **out)
{
    struct async_reader *reader = impl_from_IWMReaderAccelerator(iface);
    return IWMReader_QueryInterface(&reader->IWMReader_iface, iid, out);
}

static ULONG WINAPI reader_accl_AddRef(IWMReaderAccelerator *iface)
{
    struct async_reader *reader = impl_from_IWMReaderAccelerator(iface);
    return IWMReader_AddRef(&reader->IWMReader_iface);
}

static ULONG WINAPI reader_accl_Release(IWMReaderAccelerator *iface)
{
    struct async_reader *reader = impl_from_IWMReaderAccelerator(iface);
    return IWMReader_Release(&reader->IWMReader_iface);
}

static HRESULT WINAPI reader_accl_GetCodecInterface(IWMReaderAccelerator *iface, DWORD output, REFIID riid, void **codec)
{
    struct async_reader *reader = impl_from_IWMReaderAccelerator(iface);

    FIXME("reader %p, output %lu, iid %s, codec %p, stub!\n", reader, output, debugstr_guid(riid), codec);

    return E_NOTIMPL;
}

static HRESULT WINAPI reader_accl_Notify(IWMReaderAccelerator *iface, DWORD output, WM_MEDIA_TYPE *subtype)
{
    struct async_reader *reader = impl_from_IWMReaderAccelerator(iface);

    FIXME("reader %p, output %lu, subtype %p, stub!\n", reader, output, subtype);

    return E_NOTIMPL;
}

static const IWMReaderAcceleratorVtbl WMReaderAcceleratorVtbl = {
    reader_accl_QueryInterface,
    reader_accl_AddRef,
    reader_accl_Release,
    reader_accl_GetCodecInterface,
    reader_accl_Notify
};

static struct async_reader *impl_from_IWMReaderNetworkConfig2(IWMReaderNetworkConfig2 *iface)
{
    return CONTAINING_RECORD(iface, struct async_reader, IWMReaderNetworkConfig2_iface);
}

static HRESULT WINAPI networkconfig_QueryInterface(IWMReaderNetworkConfig2 *iface, REFIID iid, void **out)
{
    struct async_reader *reader = impl_from_IWMReaderNetworkConfig2(iface);
    return IWMReader_QueryInterface(&reader->IWMReader_iface, iid, out);
}

static ULONG WINAPI networkconfig_AddRef(IWMReaderNetworkConfig2 *iface)
{
    struct async_reader *reader = impl_from_IWMReaderNetworkConfig2(iface);
    return IWMReader_AddRef(&reader->IWMReader_iface);
}

static ULONG WINAPI networkconfig_Release(IWMReaderNetworkConfig2 *iface)
{
    struct async_reader *reader = impl_from_IWMReaderNetworkConfig2(iface);
    return IWMReader_Release(&reader->IWMReader_iface);
}

static HRESULT WINAPI networkconfig_GetBufferingTime(IWMReaderNetworkConfig2 *iface, QWORD *buffering_time)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, buffering_time);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetBufferingTime(IWMReaderNetworkConfig2 *iface, QWORD buffering_time)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %s\n", This, wine_dbgstr_longlong(buffering_time));
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetUDPPortRanges(IWMReaderNetworkConfig2 *iface, WM_PORT_NUMBER_RANGE *array,
        DWORD *ranges)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p, %p\n", This, array, ranges);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetUDPPortRanges(IWMReaderNetworkConfig2 *iface,
        WM_PORT_NUMBER_RANGE *ranges, DWORD count)
{
    struct async_reader *reader = impl_from_IWMReaderNetworkConfig2(iface);

    FIXME("reader %p, ranges %p, count %lu.\n", reader, ranges, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetProxySettings(IWMReaderNetworkConfig2 *iface, const WCHAR *protocol,
        WMT_PROXY_SETTINGS *proxy)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %s, %p\n", This, debugstr_w(protocol), proxy);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetProxySettings(IWMReaderNetworkConfig2 *iface, LPCWSTR protocol,
        WMT_PROXY_SETTINGS proxy)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %s, %d\n", This, debugstr_w(protocol), proxy);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetProxyHostName(IWMReaderNetworkConfig2 *iface, const WCHAR *protocol,
        WCHAR *hostname, DWORD *size)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %s, %p, %p\n", This, debugstr_w(protocol), hostname, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetProxyHostName(IWMReaderNetworkConfig2 *iface, const WCHAR *protocol,
        const WCHAR *hostname)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %s, %s\n", This, debugstr_w(protocol), debugstr_w(hostname));
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetProxyPort(IWMReaderNetworkConfig2 *iface, const WCHAR *protocol,
        DWORD *port)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %s, %p\n", This, debugstr_w(protocol), port);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetProxyPort(IWMReaderNetworkConfig2 *iface, const WCHAR *protocol,
        DWORD port)
{
    struct async_reader *reader = impl_from_IWMReaderNetworkConfig2(iface);

    FIXME("reader %p, protocol %s, port %lu, stub!\n", reader, debugstr_w(protocol), port);

    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetProxyExceptionList(IWMReaderNetworkConfig2 *iface, const WCHAR *protocol,
        WCHAR *exceptions, DWORD *count)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %s, %p, %p\n", This, debugstr_w(protocol), exceptions, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetProxyExceptionList(IWMReaderNetworkConfig2 *iface, const WCHAR *protocol,
        const WCHAR *exceptions)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %s, %s\n", This, debugstr_w(protocol), debugstr_w(exceptions));
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetProxyBypassForLocal(IWMReaderNetworkConfig2 *iface, const WCHAR *protocol,
        BOOL *bypass)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %s, %p\n", This, debugstr_w(protocol), bypass);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetProxyBypassForLocal(IWMReaderNetworkConfig2 *iface, const WCHAR *protocol,
        BOOL bypass)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %s, %d\n", This, debugstr_w(protocol), bypass);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetForceRerunAutoProxyDetection(IWMReaderNetworkConfig2 *iface,
        BOOL *detection)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, detection);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetForceRerunAutoProxyDetection(IWMReaderNetworkConfig2 *iface,
        BOOL detection)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %d\n", This, detection);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetEnableMulticast(IWMReaderNetworkConfig2 *iface, BOOL *multicast)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, multicast);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetEnableMulticast(IWMReaderNetworkConfig2 *iface, BOOL multicast)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %d\n", This, multicast);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetEnableHTTP(IWMReaderNetworkConfig2 *iface, BOOL *enable)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetEnableHTTP(IWMReaderNetworkConfig2 *iface, BOOL enable)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %d\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetEnableUDP(IWMReaderNetworkConfig2 *iface, BOOL *enable)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetEnableUDP(IWMReaderNetworkConfig2 *iface, BOOL enable)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %d\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetEnableTCP(IWMReaderNetworkConfig2 *iface, BOOL *enable)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetEnableTCP(IWMReaderNetworkConfig2 *iface, BOOL enable)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %d\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_ResetProtocolRollover(IWMReaderNetworkConfig2 *iface)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetConnectionBandwidth(IWMReaderNetworkConfig2 *iface, DWORD *bandwidth)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, bandwidth);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetConnectionBandwidth(IWMReaderNetworkConfig2 *iface, DWORD bandwidth)
{
    struct async_reader *reader = impl_from_IWMReaderNetworkConfig2(iface);

    FIXME("reader %p, bandwidth %lu, stub!\n", reader, bandwidth);

    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetNumProtocolsSupported(IWMReaderNetworkConfig2 *iface, DWORD *protocols)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, protocols);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetSupportedProtocolName(IWMReaderNetworkConfig2 *iface, DWORD protocol_num,
        WCHAR *protocol, DWORD *size)
{
    struct async_reader *reader = impl_from_IWMReaderNetworkConfig2(iface);

    FIXME("reader %p, index %lu, protocol %p, size %p, stub!\n", reader, protocol_num, protocol, size);

    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_AddLoggingUrl(IWMReaderNetworkConfig2 *iface, const WCHAR *url)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %s\n", This, debugstr_w(url));
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetLoggingUrl(IWMReaderNetworkConfig2 *iface, DWORD index, WCHAR *url,
        DWORD *size)
{
    struct async_reader *reader = impl_from_IWMReaderNetworkConfig2(iface);

    FIXME("reader %p, index %lu, url %p, size %p, stub!\n", reader, index, url, size);

    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetLoggingUrlCount(IWMReaderNetworkConfig2 *iface, DWORD *count)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_ResetLoggingUrlList(IWMReaderNetworkConfig2 *iface)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetEnableContentCaching(IWMReaderNetworkConfig2 *iface, BOOL *enable)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetEnableContentCaching(IWMReaderNetworkConfig2 *iface, BOOL enable)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %d\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetEnableFastCache(IWMReaderNetworkConfig2 *iface, BOOL *enable)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetEnableFastCache(IWMReaderNetworkConfig2 *iface, BOOL enable)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %d\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetAcceleratedStreamingDuration(IWMReaderNetworkConfig2 *iface,
        QWORD *duration)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, duration);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetAcceleratedStreamingDuration(IWMReaderNetworkConfig2 *iface,
        QWORD duration)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %s\n", This, wine_dbgstr_longlong(duration));
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetAutoReconnectLimit(IWMReaderNetworkConfig2 *iface, DWORD *limit)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, limit);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetAutoReconnectLimit(IWMReaderNetworkConfig2 *iface, DWORD limit)
{
    struct async_reader *reader = impl_from_IWMReaderNetworkConfig2(iface);

    FIXME("reader %p, limit %lu, stub!\n", reader, limit);

    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetEnableResends(IWMReaderNetworkConfig2 *iface, BOOL *enable)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetEnableResends(IWMReaderNetworkConfig2 *iface, BOOL enable)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %u\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetEnableThinning(IWMReaderNetworkConfig2 *iface, BOOL *enable)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_SetEnableThinning(IWMReaderNetworkConfig2 *iface, BOOL enable)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %u\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI networkconfig_GetMaxNetPacketSize(IWMReaderNetworkConfig2 *iface, DWORD *packet_size)
{
    struct async_reader *This = impl_from_IWMReaderNetworkConfig2(iface);
    FIXME("%p, %p\n", This, packet_size);
    return E_NOTIMPL;
}

static const IWMReaderNetworkConfig2Vtbl WMReaderNetworkConfig2Vtbl =
{
    networkconfig_QueryInterface,
    networkconfig_AddRef,
    networkconfig_Release,
    networkconfig_GetBufferingTime,
    networkconfig_SetBufferingTime,
    networkconfig_GetUDPPortRanges,
    networkconfig_SetUDPPortRanges,
    networkconfig_GetProxySettings,
    networkconfig_SetProxySettings,
    networkconfig_GetProxyHostName,
    networkconfig_SetProxyHostName,
    networkconfig_GetProxyPort,
    networkconfig_SetProxyPort,
    networkconfig_GetProxyExceptionList,
    networkconfig_SetProxyExceptionList,
    networkconfig_GetProxyBypassForLocal,
    networkconfig_SetProxyBypassForLocal,
    networkconfig_GetForceRerunAutoProxyDetection,
    networkconfig_SetForceRerunAutoProxyDetection,
    networkconfig_GetEnableMulticast,
    networkconfig_SetEnableMulticast,
    networkconfig_GetEnableHTTP,
    networkconfig_SetEnableHTTP,
    networkconfig_GetEnableUDP,
    networkconfig_SetEnableUDP,
    networkconfig_GetEnableTCP,
    networkconfig_SetEnableTCP,
    networkconfig_ResetProtocolRollover,
    networkconfig_GetConnectionBandwidth,
    networkconfig_SetConnectionBandwidth,
    networkconfig_GetNumProtocolsSupported,
    networkconfig_GetSupportedProtocolName,
    networkconfig_AddLoggingUrl,
    networkconfig_GetLoggingUrl,
    networkconfig_GetLoggingUrlCount,
    networkconfig_ResetLoggingUrlList,
    networkconfig_GetEnableContentCaching,
    networkconfig_SetEnableContentCaching,
    networkconfig_GetEnableFastCache,
    networkconfig_SetEnableFastCache,
    networkconfig_GetAcceleratedStreamingDuration,
    networkconfig_SetAcceleratedStreamingDuration,
    networkconfig_GetAutoReconnectLimit,
    networkconfig_SetAutoReconnectLimit,
    networkconfig_GetEnableResends,
    networkconfig_SetEnableResends,
    networkconfig_GetEnableThinning,
    networkconfig_SetEnableThinning,
    networkconfig_GetMaxNetPacketSize
};

static struct async_reader *impl_from_IWMReaderStreamClock(IWMReaderStreamClock *iface)
{
    return CONTAINING_RECORD(iface, struct async_reader, IWMReaderStreamClock_iface);
}

static HRESULT WINAPI readclock_QueryInterface(IWMReaderStreamClock *iface, REFIID iid, void **out)
{
    struct async_reader *reader = impl_from_IWMReaderStreamClock(iface);
    return IWMReader_QueryInterface(&reader->IWMReader_iface, iid, out);
}

static ULONG WINAPI readclock_AddRef(IWMReaderStreamClock *iface)
{
    struct async_reader *reader = impl_from_IWMReaderStreamClock(iface);
    return IWMReader_AddRef(&reader->IWMReader_iface);
}

static ULONG WINAPI readclock_Release(IWMReaderStreamClock *iface)
{
    struct async_reader *reader = impl_from_IWMReaderStreamClock(iface);
    return IWMReader_Release(&reader->IWMReader_iface);
}

static HRESULT WINAPI readclock_GetTime(IWMReaderStreamClock *iface, QWORD *now)
{
    struct async_reader *This = impl_from_IWMReaderStreamClock(iface);
    FIXME("%p, %p\n", This, now);
    return E_NOTIMPL;
}

static HRESULT WINAPI readclock_SetTimer(IWMReaderStreamClock *iface, QWORD when, void *param, DWORD *id)
{
    struct async_reader *This = impl_from_IWMReaderStreamClock(iface);
    FIXME("%p, %s, %p, %p\n", This, wine_dbgstr_longlong(when), param, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI readclock_KillTimer(IWMReaderStreamClock *iface, DWORD id)
{
    struct async_reader *reader = impl_from_IWMReaderStreamClock(iface);

    FIXME("reader %p, id %lu, stub!\n", reader, id);

    return E_NOTIMPL;
}

static const IWMReaderStreamClockVtbl WMReaderStreamClockVtbl =
{
    readclock_QueryInterface,
    readclock_AddRef,
    readclock_Release,
    readclock_GetTime,
    readclock_SetTimer,
    readclock_KillTimer
};

static struct async_reader *impl_from_IWMReaderTypeNegotiation(IWMReaderTypeNegotiation *iface)
{
    return CONTAINING_RECORD(iface, struct async_reader, IWMReaderTypeNegotiation_iface);
}

static HRESULT WINAPI negotiation_QueryInterface(IWMReaderTypeNegotiation *iface, REFIID iid, void **out)
{
    struct async_reader *reader = impl_from_IWMReaderTypeNegotiation(iface);
    return IWMReader_QueryInterface(&reader->IWMReader_iface, iid, out);
}

static ULONG WINAPI negotiation_AddRef(IWMReaderTypeNegotiation *iface)
{
    struct async_reader *reader = impl_from_IWMReaderTypeNegotiation(iface);
    return IWMReader_AddRef(&reader->IWMReader_iface);
}

static ULONG WINAPI negotiation_Release(IWMReaderTypeNegotiation *iface)
{
    struct async_reader *reader = impl_from_IWMReaderTypeNegotiation(iface);
    return IWMReader_Release(&reader->IWMReader_iface);
}

static HRESULT WINAPI negotiation_TryOutputProps(IWMReaderTypeNegotiation *iface, DWORD output, IWMOutputMediaProps *props)
{
    struct async_reader *reader = impl_from_IWMReaderTypeNegotiation(iface);

    FIXME("reader %p, output %lu, props %p, stub!\n", reader, output, props);

    return E_NOTIMPL;
}

static const IWMReaderTypeNegotiationVtbl WMReaderTypeNegotiationVtbl =
{
    negotiation_QueryInterface,
    negotiation_AddRef,
    negotiation_Release,
    negotiation_TryOutputProps
};

static struct async_reader *impl_from_IReferenceClock(IReferenceClock *iface)
{
    return CONTAINING_RECORD(iface, struct async_reader, IReferenceClock_iface);
}

static HRESULT WINAPI refclock_QueryInterface(IReferenceClock *iface, REFIID iid, void **out)
{
    struct async_reader *reader = impl_from_IReferenceClock(iface);
    return IWMReader_QueryInterface(&reader->IWMReader_iface, iid, out);
}

static ULONG WINAPI refclock_AddRef(IReferenceClock *iface)
{
    struct async_reader *reader = impl_from_IReferenceClock(iface);
    return IWMReader_AddRef(&reader->IWMReader_iface);
}

static ULONG WINAPI refclock_Release(IReferenceClock *iface)
{
    struct async_reader *reader = impl_from_IReferenceClock(iface);
    return IWMReader_Release(&reader->IWMReader_iface);
}

static HRESULT WINAPI refclock_GetTime(IReferenceClock *iface, REFERENCE_TIME *time)
{
    struct async_reader *This = impl_from_IReferenceClock(iface);
    FIXME("%p, %p\n", This, time);
    return E_NOTIMPL;
}

static HRESULT WINAPI refclock_AdviseTime(IReferenceClock *iface, REFERENCE_TIME basetime,
        REFERENCE_TIME streamtime, HEVENT event, DWORD_PTR *cookie)
{
    struct async_reader *reader = impl_from_IReferenceClock(iface);

    FIXME("reader %p, basetime %s, streamtime %s, event %#Ix, cookie %p, stub!\n",
            reader, debugstr_time(basetime), debugstr_time(streamtime), event, cookie);

    return E_NOTIMPL;
}

static HRESULT WINAPI refclock_AdvisePeriodic(IReferenceClock *iface, REFERENCE_TIME starttime,
        REFERENCE_TIME period, HSEMAPHORE semaphore, DWORD_PTR *cookie)
{
    struct async_reader *reader = impl_from_IReferenceClock(iface);

    FIXME("reader %p, starttime %s, period %s, semaphore %#Ix, cookie %p, stub!\n",
            reader, debugstr_time(starttime), debugstr_time(period), semaphore, cookie);

    return E_NOTIMPL;
}

static HRESULT WINAPI refclock_Unadvise(IReferenceClock *iface, DWORD_PTR cookie)
{
    struct async_reader *reader = impl_from_IReferenceClock(iface);

    FIXME("reader %p, cookie %Iu, stub!\n", reader, cookie);

    return E_NOTIMPL;
}

static const IReferenceClockVtbl ReferenceClockVtbl =
{
    refclock_QueryInterface,
    refclock_AddRef,
    refclock_Release,
    refclock_GetTime,
    refclock_AdviseTime,
    refclock_AdvisePeriodic,
    refclock_Unadvise
};

HRESULT WINAPI winegstreamer_create_wm_async_reader(IWMReader **reader)
{
    struct async_reader *object;
    HRESULT hr;

    TRACE("reader %p.\n", reader);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IReferenceClock_iface.lpVtbl = &ReferenceClockVtbl;
    object->IWMReader_iface.lpVtbl = &WMReaderVtbl;
    object->IWMReaderAdvanced6_iface.lpVtbl = &WMReaderAdvanced6Vtbl;
    object->IWMReaderAccelerator_iface.lpVtbl = &WMReaderAcceleratorVtbl;
    object->IWMReaderNetworkConfig2_iface.lpVtbl = &WMReaderNetworkConfig2Vtbl;
    object->IWMReaderStreamClock_iface.lpVtbl = &WMReaderStreamClockVtbl;
    object->IWMReaderTypeNegotiation_iface.lpVtbl = &WMReaderTypeNegotiationVtbl;
    object->refcount = 1;

    if (FAILED(hr = winegstreamer_create_wm_sync_reader((IUnknown *)&object->IWMReader_iface,
            (void **)&object->reader_inner)))
        goto failed;

    if (FAILED(hr = IUnknown_QueryInterface(object->reader_inner, &IID_IWMSyncReader2,
            (void **)&object->reader)))
        goto failed;
    IWMReader_Release(&object->IWMReader_iface);
    object->wm_reader = wm_reader_from_sync_reader_inner(object->reader_inner);

    InitializeCriticalSection(&object->cs);
    object->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": async_reader.cs");
    InitializeCriticalSection(&object->callback_cs);
    object->callback_cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": async_reader.callback_cs");

    QueryPerformanceFrequency(&object->clock_frequency);
    list_init(&object->async_ops);

    TRACE("Created async reader %p.\n", object);
    *reader = (IWMReader *)&object->IWMReader_iface;
    return S_OK;

failed:
    if (object->reader_inner)
        IUnknown_Release(object->reader_inner);
    free(object);
    return hr;
}
