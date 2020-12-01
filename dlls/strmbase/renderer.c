/*
 * Generic Implementation of strmbase Base Renderer classes
 *
 * Copyright 2012 Aric Stewart, CodeWeavers
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

#include "strmbase_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(strmbase);

static inline struct strmbase_renderer *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct strmbase_renderer, filter);
}

static inline struct strmbase_renderer *impl_from_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, struct strmbase_renderer, sink.pin.IPin_iface);
}

static struct strmbase_pin *renderer_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    struct strmbase_renderer *filter = impl_from_strmbase_filter(iface);

    if (index == 0)
        return &filter->sink.pin;
    return NULL;
}

static void renderer_destroy(struct strmbase_filter *iface)
{
    struct strmbase_renderer *filter = impl_from_strmbase_filter(iface);
    filter->pFuncsTable->renderer_destroy(filter);
}

static HRESULT renderer_query_interface(struct strmbase_filter *iface, REFIID iid, void **out)
{
    struct strmbase_renderer *filter = impl_from_strmbase_filter(iface);
    HRESULT hr;

    if (filter->pFuncsTable->renderer_query_interface
            && SUCCEEDED(hr = filter->pFuncsTable->renderer_query_interface(filter, iid, out)))
    {
        return hr;
    }

    if (IsEqualGUID(iid, &IID_IMediaPosition))
        *out = &filter->passthrough.IMediaPosition_iface;
    else if (IsEqualGUID(iid, &IID_IMediaSeeking))
        *out = &filter->passthrough.IMediaSeeking_iface;
    else if (IsEqualGUID(iid, &IID_IQualityControl))
        *out = &filter->qc.IQualityControl_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT renderer_init_stream(struct strmbase_filter *iface)
{
    struct strmbase_renderer *filter = impl_from_strmbase_filter(iface);

    if (filter->sink.pin.peer)
        ResetEvent(filter->state_event);
    filter->eos = FALSE;
    ResetEvent(filter->flush_event);
    if (filter->pFuncsTable->renderer_init_stream)
        filter->pFuncsTable->renderer_init_stream(filter);

    return filter->sink.pin.peer ? S_FALSE : S_OK;
}

static HRESULT renderer_start_stream(struct strmbase_filter *iface, REFERENCE_TIME start)
{
    struct strmbase_renderer *filter = impl_from_strmbase_filter(iface);

    filter->stream_start = start;
    SetEvent(filter->state_event);
    if (filter->sink.pin.peer)
        filter->eos = FALSE;
    QualityControlRender_Start(&filter->qc, filter->stream_start);
    if (filter->sink.pin.peer && filter->pFuncsTable->renderer_start_stream)
        filter->pFuncsTable->renderer_start_stream(filter);

    return S_OK;
}

static HRESULT renderer_stop_stream(struct strmbase_filter *iface)
{
    struct strmbase_renderer *filter = impl_from_strmbase_filter(iface);

    if (filter->sink.pin.peer && filter->pFuncsTable->renderer_stop_stream)
        filter->pFuncsTable->renderer_stop_stream(filter);

    return S_OK;
}

static HRESULT renderer_cleanup_stream(struct strmbase_filter *iface)
{
    struct strmbase_renderer *filter = impl_from_strmbase_filter(iface);

    strmbase_passthrough_invalidate_time(&filter->passthrough);
    SetEvent(filter->state_event);
    SetEvent(filter->flush_event);

    return S_OK;
}

static HRESULT renderer_wait_state(struct strmbase_filter *iface, DWORD timeout)
{
    struct strmbase_renderer *filter = impl_from_strmbase_filter(iface);

    if (WaitForSingleObject(filter->state_event, timeout) == WAIT_TIMEOUT)
        return VFW_S_STATE_INTERMEDIATE;
    return S_OK;
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = renderer_get_pin,
    .filter_destroy = renderer_destroy,
    .filter_query_interface = renderer_query_interface,
    .filter_init_stream = renderer_init_stream,
    .filter_start_stream = renderer_start_stream,
    .filter_stop_stream = renderer_stop_stream,
    .filter_cleanup_stream = renderer_cleanup_stream,
    .filter_wait_state = renderer_wait_state,
};

static HRESULT sink_query_accept(struct strmbase_pin *pin, const AM_MEDIA_TYPE *mt)
{
    struct strmbase_renderer *filter = impl_from_IPin(&pin->IPin_iface);
    return filter->pFuncsTable->pfnCheckMediaType(filter, mt);
}

static HRESULT sink_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct strmbase_renderer *filter = impl_from_IPin(&iface->IPin_iface);
    HRESULT hr;

    if (filter->pFuncsTable->renderer_pin_query_interface
            && SUCCEEDED(hr = filter->pFuncsTable->renderer_pin_query_interface(filter, iid, out)))
        return hr;

    if (IsEqualGUID(iid, &IID_IMemInputPin))
        *out = &filter->sink.IMemInputPin_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT WINAPI BaseRenderer_Receive(struct strmbase_sink *pin, IMediaSample *sample)
{
    struct strmbase_renderer *filter = impl_from_IPin(&pin->pin.IPin_iface);
    REFERENCE_TIME start, stop;
    BOOL need_wait = FALSE;
    FILTER_STATE state;
    HRESULT hr = S_OK;
    AM_MEDIA_TYPE *mt;

    if (filter->eos || filter->sink.flushing)
        return S_FALSE;

    state = filter->filter.state;
    if (state == State_Stopped)
        return VFW_E_WRONG_STATE;

    if (IMediaSample_GetMediaType(sample, &mt) == S_OK)
    {
        TRACE("Format change.\n");
        strmbase_dump_media_type(mt);

        if (FAILED(filter->pFuncsTable->pfnCheckMediaType(filter, mt)))
            return VFW_E_TYPE_NOT_ACCEPTED;
        DeleteMediaType(mt);
    }

    EnterCriticalSection(&filter->csRenderLock);

    if (filter->filter.clock && SUCCEEDED(IMediaSample_GetTime(sample, &start, &stop)))
    {
        strmbase_passthrough_update_time(&filter->passthrough, start);
        need_wait = TRUE;
    }
    else
        start = stop = -1;

    if (state == State_Paused)
    {
        QualityControlRender_BeginRender(&filter->qc, start, stop);
        hr = filter->pFuncsTable->pfnDoRenderSample(filter, sample);
        QualityControlRender_EndRender(&filter->qc);
    }

    if (need_wait)
    {
        REFERENCE_TIME now;
        DWORD_PTR cookie;

        IReferenceClock_GetTime(filter->filter.clock, &now);

        if (now - filter->stream_start - start <= -10000)
        {
            HANDLE handles[2] = {filter->advise_event, filter->flush_event};
            DWORD ret;

            IReferenceClock_AdviseTime(filter->filter.clock, filter->stream_start,
                    start, (HEVENT)filter->advise_event, &cookie);

            ret = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
            IReferenceClock_Unadvise(filter->filter.clock, cookie);

            if (ret == 1)
            {
                LeaveCriticalSection(&filter->csRenderLock);
                TRACE("Flush signaled; discarding current sample.\n");
                return S_OK;
            }
        }
    }

    if (state == State_Running)
    {
        QualityControlRender_BeginRender(&filter->qc, start, stop);
        hr = filter->pFuncsTable->pfnDoRenderSample(filter, sample);
        QualityControlRender_EndRender(&filter->qc);
    }

    QualityControlRender_DoQOS(&filter->qc);

    LeaveCriticalSection(&filter->csRenderLock);

    return hr;
}

static HRESULT sink_connect(struct strmbase_sink *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    struct strmbase_renderer *filter = impl_from_IPin(&iface->pin.IPin_iface);

    if (filter->pFuncsTable->renderer_connect)
        return filter->pFuncsTable->renderer_connect(filter, mt);
    return S_OK;
}

static void sink_disconnect(struct strmbase_sink *iface)
{
    struct strmbase_renderer *filter = impl_from_IPin(&iface->pin.IPin_iface);

    if (filter->pFuncsTable->pfnBreakConnect)
        filter->pFuncsTable->pfnBreakConnect(filter);
}

static HRESULT sink_eos(struct strmbase_sink *iface)
{
    struct strmbase_renderer *filter = impl_from_IPin(&iface->pin.IPin_iface);
    IFilterGraph *graph = filter->filter.graph;
    IMediaEventSink *event_sink;

    EnterCriticalSection(&filter->csRenderLock);

    filter->eos = TRUE;

    if (graph && SUCCEEDED(IFilterGraph_QueryInterface(graph,
            &IID_IMediaEventSink, (void **)&event_sink)))
    {
        IMediaEventSink_Notify(event_sink, EC_COMPLETE, S_OK,
                (LONG_PTR)&filter->filter.IBaseFilter_iface);
        IMediaEventSink_Release(event_sink);
    }
    strmbase_passthrough_eos(&filter->passthrough);
    SetEvent(filter->state_event);

    LeaveCriticalSection(&filter->csRenderLock);
    return S_OK;
}

static HRESULT sink_begin_flush(struct strmbase_sink *iface)
{
    struct strmbase_renderer *filter = impl_from_IPin(&iface->pin.IPin_iface);

    SetEvent(filter->flush_event);

    return S_OK;
}

static HRESULT sink_end_flush(struct strmbase_sink *iface)
{
    struct strmbase_renderer *filter = impl_from_IPin(&iface->pin.IPin_iface);

    EnterCriticalSection(&filter->csRenderLock);

    filter->eos = FALSE;
    QualityControlRender_Start(&filter->qc, filter->stream_start);
    strmbase_passthrough_invalidate_time(&filter->passthrough);
    ResetEvent(filter->flush_event);

    LeaveCriticalSection(&filter->csRenderLock);
    return S_OK;
}

static const struct strmbase_sink_ops sink_ops =
{
    .base.pin_query_accept = sink_query_accept,
    .base.pin_query_interface = sink_query_interface,
    .pfnReceive = BaseRenderer_Receive,
    .sink_connect = sink_connect,
    .sink_disconnect = sink_disconnect,
    .sink_eos = sink_eos,
    .sink_begin_flush = sink_begin_flush,
    .sink_end_flush = sink_end_flush,
};

void strmbase_renderer_cleanup(struct strmbase_renderer *filter)
{
    if (filter->sink.pin.peer)
        IPin_Disconnect(filter->sink.pin.peer);
    IPin_Disconnect(&filter->sink.pin.IPin_iface);
    strmbase_sink_cleanup(&filter->sink);

    strmbase_passthrough_cleanup(&filter->passthrough);

    filter->csRenderLock.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&filter->csRenderLock);

    CloseHandle(filter->state_event);
    CloseHandle(filter->advise_event);
    CloseHandle(filter->flush_event);
    strmbase_filter_cleanup(&filter->filter);
}

void strmbase_renderer_init(struct strmbase_renderer *filter, IUnknown *outer,
        const CLSID *clsid, const WCHAR *sink_name, const struct strmbase_renderer_ops *ops)
{
    memset(filter, 0, sizeof(*filter));
    strmbase_filter_init(&filter->filter, outer, clsid, &filter_ops);
    strmbase_passthrough_init(&filter->passthrough, (IUnknown *)&filter->filter.IBaseFilter_iface);
    ISeekingPassThru_Init(&filter->passthrough.ISeekingPassThru_iface, TRUE, &filter->sink.pin.IPin_iface);
    strmbase_qc_init(&filter->qc, &filter->sink.pin);

    filter->pFuncsTable = ops;

    strmbase_sink_init(&filter->sink, &filter->filter, sink_name, &sink_ops, NULL);

    InitializeCriticalSection(&filter->csRenderLock);
    filter->csRenderLock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__": strmbase_renderer.csRenderLock");
    filter->state_event = CreateEventW(NULL, TRUE, TRUE, NULL);
    filter->advise_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    filter->flush_event = CreateEventW(NULL, TRUE, TRUE, NULL);
}
