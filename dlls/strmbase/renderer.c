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

/* The following quality-of-service code is based on GstBaseSink QoS code, which
 * is covered by the following copyright information:
 *
 * GStreamer
 * Copyright (C) 2005-2007 Wim Taymans <wim.taymans@gmail.com>
 *
 * gstbasesink.c: Base class for sink elements
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#define DO_RUNNING_AVG(avg,val,size) (((val) + ((size)-1) * (avg)) / (size))

/* Generic running average; this has a neutral window size. */
#define UPDATE_RUNNING_AVG(avg,val)   DO_RUNNING_AVG(avg,val,8)

/* The windows for these running averages are experimentally obtained.
 * Positive values get averaged more, while negative values use a small
 * window so we can react faster to badness. */
#define UPDATE_RUNNING_AVG_P(avg,val) DO_RUNNING_AVG(avg,val,16)
#define UPDATE_RUNNING_AVG_N(avg,val) DO_RUNNING_AVG(avg,val,4)

static void QualityControlRender_Start(struct strmbase_qc *This, REFERENCE_TIME tStart)
{
    This->avg_render = This->last_in_time = This->last_left = This->avg_duration = This->avg_pt = -1;
    This->clockstart = tStart;
    This->avg_rate = -1.0;
    This->rendered = This->dropped = 0;
    This->is_dropped = FALSE;
}

static BOOL QualityControlRender_IsLate(struct strmbase_qc *This, REFERENCE_TIME jitter,
                                        REFERENCE_TIME start, REFERENCE_TIME stop)
{
    REFERENCE_TIME max_lateness = 200000;

    TRACE("jitter %s, start %s, stop %s.\n", debugstr_time(jitter),
            debugstr_time(start), debugstr_time(stop));

    /* we can add a valid stop time */
    if (stop >= start)
        max_lateness += stop;
    else
        max_lateness += start;

    /* if the jitter bigger than duration and lateness we are too late */
    if (start + jitter > max_lateness) {
        WARN("buffer is too late %i > %i\n", (int)((start + jitter)/10000),  (int)(max_lateness/10000));
        /* !!emergency!!, if we did not receive anything valid for more than a
         * second, render it anyway so the user sees something */
        if (This->last_in_time < 0 ||
            start - This->last_in_time < 10000000)
            return TRUE;
        FIXME("A lot of buffers are being dropped.\n");
        FIXME("There may be a timestamping problem, or this computer is too slow.\n");
    }
    This->last_in_time = start;
    return FALSE;
}

static void QualityControlRender_DoQOS(struct strmbase_qc *priv)
{
    REFERENCE_TIME start, stop, jitter, pt, entered, left, duration;
    double rate;

    TRACE("%p\n", priv);

    if (!priv->pin->filter->clock || priv->current_rstart < 0)
        return;

    start = priv->current_rstart;
    stop = priv->current_rstop;
    jitter = priv->current_jitter;

    if (jitter < 0)
    {
        /* This is the time the buffer entered the sink. */
        if (start < -jitter)
            entered = 0;
        else
            entered = start + jitter;
        left = start;
    }
    else
    {
        /* This is the time the buffer entered the sink. */
        entered = start + jitter;
        /* This is the time the buffer left the sink. */
        left = start + jitter;
    }

    /* Calculate the duration of the buffer. */
    if (stop >= start)
        duration = stop - start;
    else
        duration = 0;

    /* If we have the time when the last buffer left us, calculate processing
     * time. */
    if (priv->last_left >= 0)
    {
        if (entered > priv->last_left)
            pt = entered - priv->last_left;
        else
            pt = 0;
    }
    else
    {
        pt = priv->avg_pt;
    }

    TRACE("start %s, entered %s, left %s, pt %s, duration %s, jitter %s.\n",
            debugstr_time(start), debugstr_time(entered), debugstr_time(left),
            debugstr_time(pt), debugstr_time(duration), debugstr_time(jitter));

    TRACE("average duration %s, average pt %s, average rate %.16e.\n",
            debugstr_time(priv->avg_duration), debugstr_time(priv->avg_pt), priv->avg_rate);

    /* Collect running averages. For first observations, we copy the values. */
    if (priv->avg_duration < 0)
        priv->avg_duration = duration;
    else
        priv->avg_duration = UPDATE_RUNNING_AVG(priv->avg_duration, duration);

    if (priv->avg_pt < 0)
        priv->avg_pt = pt;
    else
        priv->avg_pt = UPDATE_RUNNING_AVG(priv->avg_pt, pt);

    if (priv->avg_duration != 0)
        rate = (double)priv->avg_pt / (double)priv->avg_duration;
    else
        rate = 0.0;

    if (priv->last_left >= 0)
    {
        if (priv->is_dropped || priv->avg_rate < 0.0)
        {
            priv->avg_rate = rate;
        }
        else
        {
            if (rate > 1.0)
                priv->avg_rate = UPDATE_RUNNING_AVG_N(priv->avg_rate, rate);
            else
                priv->avg_rate = UPDATE_RUNNING_AVG_P(priv->avg_rate, rate);
        }
    }

    if (priv->avg_rate >= 0.0)
    {
        Quality q;

        /* If we have a valid rate, start sending QoS messages. */
        if (priv->current_jitter < 0)
        {
            /* Make sure we never go below 0 when adding the jitter to the
             * timestamp. */
            if (priv->current_rstart < -priv->current_jitter)
                priv->current_jitter = -priv->current_rstart;
        }
        else
        {
            priv->current_jitter += (priv->current_rstop - priv->current_rstart);
        }

        q.Type = (jitter > 0 ? Famine : Flood);
        q.Proportion = 1000.0 / priv->avg_rate;
        if (q.Proportion < 200)
            q.Proportion = 200;
        else if (q.Proportion > 5000)
            q.Proportion = 5000;
        q.Late = priv->current_jitter;
        q.TimeStamp = priv->current_rstart;
        IQualityControl_Notify(&priv->IQualityControl_iface, &priv->pin->filter->IBaseFilter_iface, q);
    }

    /* Record when this buffer will leave us. */
    priv->last_left = left;
}

static void QualityControlRender_BeginRender(struct strmbase_qc *This, REFERENCE_TIME start, REFERENCE_TIME stop)
{
    This->start = -1;

    This->current_rstart = start;
    This->current_rstop = max(stop, start);

    if (start >= 0)
    {
        REFERENCE_TIME now;
        IReferenceClock_GetTime(This->pin->filter->clock, &now);
        This->current_jitter = (now - This->clockstart) - start;
    }
    else
    {
        This->current_jitter = 0;
    }

    /* FIXME: This isn't correct; we don't drop samples, nor should. */
    This->is_dropped = QualityControlRender_IsLate(This, This->current_jitter, start, stop);
    TRACE("dropped %d, start %s, stop %s, jitter %s.\n", This->is_dropped,
            debugstr_time(start), debugstr_time(stop), debugstr_time(This->current_jitter));
    if (This->is_dropped)
        This->dropped++;
    else
        This->rendered++;

    if (!This->pin->filter->clock)
        return;

    IReferenceClock_GetTime(This->pin->filter->clock, &This->start);

    TRACE("Starting at %s.\n", debugstr_time(This->start));
}

static void QualityControlRender_EndRender(struct strmbase_qc *This)
{
    REFERENCE_TIME elapsed;

    TRACE("%p\n", This);

    if (!This->pin->filter->clock || This->start < 0
            || FAILED(IReferenceClock_GetTime(This->pin->filter->clock, &This->stop)))
        return;

    elapsed = This->start - This->stop;
    if (elapsed < 0)
        return;
    if (This->avg_render < 0)
        This->avg_render = elapsed;
    else
        This->avg_render = UPDATE_RUNNING_AVG(This->avg_render, elapsed);
}

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

    EnterCriticalSection(&filter->filter.stream_cs);

    filter->eos = FALSE;
    QualityControlRender_Start(&filter->qc, filter->stream_start);
    strmbase_passthrough_invalidate_time(&filter->passthrough);
    ResetEvent(filter->flush_event);

    LeaveCriticalSection(&filter->filter.stream_cs);
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

static struct strmbase_qc *impl_from_IQualityControl(IQualityControl *iface)
{
    return CONTAINING_RECORD(iface, struct strmbase_qc, IQualityControl_iface);
}

static HRESULT WINAPI quality_control_QueryInterface(IQualityControl *iface, REFIID iid, void **out)
{
    struct strmbase_qc *qc = impl_from_IQualityControl(iface);
    return IBaseFilter_QueryInterface(&qc->pin->filter->IBaseFilter_iface, iid, out);
}

static ULONG WINAPI quality_control_AddRef(IQualityControl *iface)
{
    struct strmbase_qc *qc = impl_from_IQualityControl(iface);
    return IBaseFilter_AddRef(&qc->pin->filter->IBaseFilter_iface);
}

static ULONG WINAPI quality_control_Release(IQualityControl *iface)
{
    struct strmbase_qc *qc = impl_from_IQualityControl(iface);
    return IBaseFilter_Release(&qc->pin->filter->IBaseFilter_iface);
}

static HRESULT WINAPI quality_control_Notify(IQualityControl *iface, IBaseFilter *sender, Quality q)
{
    struct strmbase_qc *qc = impl_from_IQualityControl(iface);
    HRESULT hr = S_FALSE;

    TRACE("iface %p, sender %p, type %#x, proportion %u, late %s, timestamp %s.\n",
        iface, sender, q.Type, q.Proportion, debugstr_time(q.Late), debugstr_time(q.TimeStamp));

    if (qc->tonotify)
        return IQualityControl_Notify(qc->tonotify, &qc->pin->filter->IBaseFilter_iface, q);

    if (qc->pin->peer)
    {
        IQualityControl *peer_qc = NULL;
        IPin_QueryInterface(qc->pin->peer, &IID_IQualityControl, (void **)&peer_qc);
        if (peer_qc)
        {
            hr = IQualityControl_Notify(peer_qc, &qc->pin->filter->IBaseFilter_iface, q);
            IQualityControl_Release(peer_qc);
        }
    }

    return hr;
}

static HRESULT WINAPI quality_control_SetSink(IQualityControl *iface, IQualityControl *sink)
{
    struct strmbase_qc *qc = impl_from_IQualityControl(iface);

    TRACE("iface %p, sink %p.\n", iface, sink);

    qc->tonotify = sink;
    return S_OK;
}

static const IQualityControlVtbl quality_control_vtbl =
{
    quality_control_QueryInterface,
    quality_control_AddRef,
    quality_control_Release,
    quality_control_Notify,
    quality_control_SetSink,
};

void strmbase_renderer_cleanup(struct strmbase_renderer *filter)
{
    if (filter->sink.pin.peer)
        IPin_Disconnect(filter->sink.pin.peer);
    IPin_Disconnect(&filter->sink.pin.IPin_iface);
    strmbase_sink_cleanup(&filter->sink);

    strmbase_passthrough_cleanup(&filter->passthrough);

    CloseHandle(filter->state_event);
    CloseHandle(filter->advise_event);
    CloseHandle(filter->flush_event);
    strmbase_filter_cleanup(&filter->filter);
}

static void strmbase_qc_init(struct strmbase_qc *qc, struct strmbase_pin *pin)
{
    memset(qc, 0, sizeof(*qc));
    qc->pin = pin;
    qc->current_rstart = qc->current_rstop = -1;
    qc->IQualityControl_iface.lpVtbl = &quality_control_vtbl;
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

    filter->state_event = CreateEventW(NULL, TRUE, TRUE, NULL);
    filter->advise_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    filter->flush_event = CreateEventW(NULL, TRUE, TRUE, NULL);
}
