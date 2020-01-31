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

static const IQualityControlVtbl Renderer_QualityControl_Vtbl = {
    QualityControlImpl_QueryInterface,
    QualityControlImpl_AddRef,
    QualityControlImpl_Release,
    QualityControlImpl_Notify,
    QualityControlImpl_SetSink
};

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

    if (IsEqualIID(iid, &IID_IMediaSeeking) || IsEqualIID(iid, &IID_IMediaPosition))
        return IUnknown_QueryInterface(filter->pPosition, iid, out);
    else if (IsEqualIID(iid, &IID_IQualityControl))
    {
        *out = &filter->qcimpl->IQualityControl_iface;
        IUnknown_AddRef((IUnknown *)*out);
        return S_OK;
    }
    return E_NOINTERFACE;
}

static HRESULT renderer_init_stream(struct strmbase_filter *iface)
{
    struct strmbase_renderer *filter = impl_from_strmbase_filter(iface);

    if (filter->sink.pin.peer)
        ResetEvent(filter->state_event);
    filter->eos = FALSE;
    BaseRendererImpl_ClearPendingSample(filter);
    ResetEvent(filter->flush_event);
    if (filter->pFuncsTable->renderer_init_stream)
        filter->pFuncsTable->renderer_init_stream(filter);

    return S_OK;
}

static HRESULT renderer_start_stream(struct strmbase_filter *iface, REFERENCE_TIME start)
{
    struct strmbase_renderer *filter = impl_from_strmbase_filter(iface);

    filter->stream_start = start;
    SetEvent(filter->state_event);
    if (filter->sink.pin.peer)
        filter->eos = FALSE;
    QualityControlRender_Start(filter->qcimpl, filter->stream_start);
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

    RendererPosPassThru_ResetMediaTime(filter->pPosition);
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
    return BaseRendererImpl_Receive(filter, sample);
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
    HRESULT hr = S_OK;

    EnterCriticalSection(&filter->csRenderLock);

    filter->eos = TRUE;

    if (graph && SUCCEEDED(IFilterGraph_QueryInterface(graph,
            &IID_IMediaEventSink, (void **)&event_sink)))
    {
        IMediaEventSink_Notify(event_sink, EC_COMPLETE, S_OK,
                (LONG_PTR)&filter->filter.IBaseFilter_iface);
        IMediaEventSink_Release(event_sink);
    }
    RendererPosPassThru_EOS(filter->pPosition);
    SetEvent(filter->state_event);

    if (filter->pFuncsTable->pfnEndOfStream)
        hr = filter->pFuncsTable->pfnEndOfStream(filter);

    LeaveCriticalSection(&filter->csRenderLock);
    return hr;
}

static HRESULT sink_begin_flush(struct strmbase_sink *iface)
{
    struct strmbase_renderer *filter = impl_from_IPin(&iface->pin.IPin_iface);

    BaseRendererImpl_ClearPendingSample(filter);
    SetEvent(filter->flush_event);

    return S_OK;
}

static HRESULT sink_end_flush(struct strmbase_sink *iface)
{
    struct strmbase_renderer *filter = impl_from_IPin(&iface->pin.IPin_iface);
    HRESULT hr = S_OK;

    EnterCriticalSection(&filter->csRenderLock);

    filter->eos = FALSE;
    QualityControlRender_Start(filter->qcimpl, filter->stream_start);
    RendererPosPassThru_ResetMediaTime(filter->pPosition);
    ResetEvent(filter->flush_event);

    if (filter->pFuncsTable->pfnEndFlush)
        hr = filter->pFuncsTable->pfnEndFlush(filter);

    LeaveCriticalSection(&filter->csRenderLock);
    return hr;
}

static const struct strmbase_sink_ops sink_ops =
{
    .base.pin_query_accept = sink_query_accept,
    .base.pin_query_interface = sink_query_interface,
    .base.pin_get_media_type = strmbase_pin_get_media_type,
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

    if (filter->pPosition)
        IUnknown_Release(filter->pPosition);

    filter->csRenderLock.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&filter->csRenderLock);

    BaseRendererImpl_ClearPendingSample(filter);
    CloseHandle(filter->state_event);
    CloseHandle(filter->advise_event);
    CloseHandle(filter->flush_event);
    QualityControlImpl_Destroy(filter->qcimpl);
    strmbase_filter_cleanup(&filter->filter);
}

HRESULT WINAPI BaseRendererImpl_Receive(struct strmbase_renderer *This, IMediaSample *pSample)
{
    HRESULT hr = S_OK;
    REFERENCE_TIME start, stop;
    AM_MEDIA_TYPE *pmt;

    TRACE("(%p)->%p\n", This, pSample);

    if (This->eos || This->sink.flushing)
        return S_FALSE;

    if (This->filter.state == State_Stopped)
        return VFW_E_WRONG_STATE;

    if (IMediaSample_GetMediaType(pSample, &pmt) == S_OK)
    {
        TRACE("Format change.\n");
        strmbase_dump_media_type(pmt);

        if (FAILED(This->pFuncsTable->pfnCheckMediaType(This, pmt)))
        {
            return VFW_E_TYPE_NOT_ACCEPTED;
        }
        DeleteMediaType(pmt);
    }

    This->pMediaSample = pSample;
    IMediaSample_AddRef(pSample);

    if (This->pFuncsTable->pfnPrepareReceive)
        hr = This->pFuncsTable->pfnPrepareReceive(This, pSample);
    if (FAILED(hr))
    {
        if (hr == VFW_E_SAMPLE_REJECTED)
            return S_OK;
        else
            return hr;
    }

    EnterCriticalSection(&This->csRenderLock);
    if (This->filter.state == State_Paused)
        SetEvent(This->state_event);

    /* Wait for render Time */
    if (This->filter.clock && SUCCEEDED(IMediaSample_GetTime(pSample, &start, &stop)))
    {
        hr = S_FALSE;
        RendererPosPassThru_RegisterMediaTime(This->pPosition, start);
        if (This->pFuncsTable->pfnShouldDrawSampleNow)
            hr = This->pFuncsTable->pfnShouldDrawSampleNow(This, pSample, &start, &stop);

        if (hr == S_OK)
            ;/* Do not wait: drop through */
        else if (hr == S_FALSE)
        {
            REFERENCE_TIME now;
            DWORD_PTR cookie;

            IReferenceClock_GetTime(This->filter.clock, &now);

            if (now - This->stream_start - start <= -10000)
            {
                HANDLE handles[2] = {This->advise_event, This->flush_event};
                DWORD ret;

                IReferenceClock_AdviseTime(This->filter.clock, This->stream_start,
                        start, (HEVENT)This->advise_event, &cookie);

                LeaveCriticalSection(&This->csRenderLock);

                ret = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
                IReferenceClock_Unadvise(This->filter.clock, cookie);

                if (ret == 1)
                {
                    TRACE("Flush signaled, discarding current sample.\n");
                    return S_OK;
                }

                EnterCriticalSection(&This->csRenderLock);
            }
        }
        else
        {
            LeaveCriticalSection(&This->csRenderLock);
            /* Drop Sample */
            return S_OK;
        }
    }
    else
        start = stop = -1;

    if (SUCCEEDED(hr))
    {
        QualityControlRender_BeginRender(This->qcimpl, start, stop);
        hr = This->pFuncsTable->pfnDoRenderSample(This, pSample);
        QualityControlRender_EndRender(This->qcimpl);
    }

    QualityControlRender_DoQOS(This->qcimpl);

    BaseRendererImpl_ClearPendingSample(This);
    LeaveCriticalSection(&This->csRenderLock);

    return hr;
}

HRESULT WINAPI BaseRendererImpl_ClearPendingSample(struct strmbase_renderer *iface)
{
    if (iface->pMediaSample)
    {
        IMediaSample_Release(iface->pMediaSample);
        iface->pMediaSample = NULL;
    }
    return S_OK;
}

HRESULT WINAPI strmbase_renderer_init(struct strmbase_renderer *filter, IUnknown *outer,
        const CLSID *clsid, const WCHAR *sink_name, const struct strmbase_renderer_ops *ops)
{
    HRESULT hr;

    memset(filter, 0, sizeof(*filter));
    strmbase_filter_init(&filter->filter, outer, clsid, &filter_ops);

    filter->pFuncsTable = ops;

    strmbase_sink_init(&filter->sink, &filter->filter, sink_name, &sink_ops, NULL);

    hr = CreatePosPassThru(outer ? outer : (IUnknown *)&filter->filter.IBaseFilter_iface,
            TRUE, &filter->sink.pin.IPin_iface, &filter->pPosition);
    if (FAILED(hr))
    {
        strmbase_sink_cleanup(&filter->sink);
        strmbase_filter_cleanup(&filter->filter);
        return hr;
    }

    InitializeCriticalSection(&filter->csRenderLock);
    filter->csRenderLock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__": strmbase_renderer.csRenderLock");
    filter->state_event = CreateEventW(NULL, TRUE, TRUE, NULL);
    filter->advise_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    filter->flush_event = CreateEventW(NULL, TRUE, TRUE, NULL);

    QualityControlImpl_Create(&filter->sink.pin, &filter->qcimpl);
    filter->qcimpl->IQualityControl_iface.lpVtbl = &Renderer_QualityControl_Vtbl;

    return S_OK;
}
