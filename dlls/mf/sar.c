/*
 * Copyright 2019 Nikolay Sivov for CodeWeavers
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

#include "mfapi.h"
#include "mfidl.h"
#include "mferror.h"
#include "mf_private.h"
#include "initguid.h"
#include "mmdeviceapi.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

struct audio_renderer
{
    IMFMediaSink IMFMediaSink_iface;
    IMFMediaSinkPreroll IMFMediaSinkPreroll_iface;
    IMFClockStateSink IMFClockStateSink_iface;
    IMFMediaEventGenerator IMFMediaEventGenerator_iface;
    LONG refcount;
    IMFMediaEventQueue *event_queue;
    IMFPresentationClock *clock;
    BOOL is_shut_down;
    CRITICAL_SECTION cs;
};

static struct audio_renderer *impl_from_IMFMediaSink(IMFMediaSink *iface)
{
    return CONTAINING_RECORD(iface, struct audio_renderer, IMFMediaSink_iface);
}

static struct audio_renderer *impl_from_IMFMediaSinkPreroll(IMFMediaSinkPreroll *iface)
{
    return CONTAINING_RECORD(iface, struct audio_renderer, IMFMediaSinkPreroll_iface);
}

static struct audio_renderer *impl_from_IMFClockStateSink(IMFClockStateSink *iface)
{
    return CONTAINING_RECORD(iface, struct audio_renderer, IMFClockStateSink_iface);
}

static struct audio_renderer *impl_from_IMFMediaEventGenerator(IMFMediaEventGenerator *iface)
{
    return CONTAINING_RECORD(iface, struct audio_renderer, IMFMediaEventGenerator_iface);
}

static HRESULT WINAPI audio_renderer_sink_QueryInterface(IMFMediaSink *iface, REFIID riid, void **obj)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFMediaSink) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
    }
    else if (IsEqualIID(riid, &IID_IMFMediaSinkPreroll))
    {
        *obj = &renderer->IMFMediaSinkPreroll_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFClockStateSink))
    {
        *obj = &renderer->IMFClockStateSink_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFMediaEventGenerator))
    {
        *obj = &renderer->IMFMediaEventGenerator_iface;
    }
    else
    {
        WARN("Unsupported %s.\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);

    return S_OK;
}

static ULONG WINAPI audio_renderer_sink_AddRef(IMFMediaSink *iface)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);
    ULONG refcount = InterlockedIncrement(&renderer->refcount);
    TRACE("%p, refcount %u.\n", iface, refcount);
    return refcount;
}

static ULONG WINAPI audio_renderer_sink_Release(IMFMediaSink *iface)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);
    ULONG refcount = InterlockedDecrement(&renderer->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        if (renderer->event_queue)
            IMFMediaEventQueue_Release(renderer->event_queue);
        if (renderer->clock)
            IMFPresentationClock_Release(renderer->clock);
        DeleteCriticalSection(&renderer->cs);
        heap_free(renderer);
    }

    return refcount;
}

static HRESULT WINAPI audio_renderer_sink_GetCharacteristics(IMFMediaSink *iface, DWORD *flags)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);

    TRACE("%p, %p.\n", iface, flags);

    if (renderer->is_shut_down)
        return MF_E_SHUTDOWN;

    *flags = MEDIASINK_FIXED_STREAMS | MEDIASINK_CAN_PREROLL;

    return S_OK;
}

static HRESULT WINAPI audio_renderer_sink_AddStreamSink(IMFMediaSink *iface, DWORD stream_sink_id,
    IMFMediaType *media_type, IMFStreamSink **stream_sink)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);

    TRACE("%p, %#x, %p, %p.\n", iface, stream_sink_id, media_type, stream_sink);

    return renderer->is_shut_down ? MF_E_SHUTDOWN : MF_E_STREAMSINKS_FIXED;
}

static HRESULT WINAPI audio_renderer_sink_RemoveStreamSink(IMFMediaSink *iface, DWORD stream_sink_id)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);

    TRACE("%p, %#x.\n", iface, stream_sink_id);

    return renderer->is_shut_down ? MF_E_SHUTDOWN : MF_E_STREAMSINKS_FIXED;
}

static HRESULT WINAPI audio_renderer_sink_GetStreamSinkCount(IMFMediaSink *iface, DWORD *count)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);

    TRACE("%p, %p.\n", iface, count);

    if (!count)
        return E_POINTER;

    if (renderer->is_shut_down)
        return MF_E_SHUTDOWN;

    *count = 1;

    return S_OK;
}

static HRESULT WINAPI audio_renderer_sink_GetStreamSinkByIndex(IMFMediaSink *iface, DWORD index,
        IMFStreamSink **stream)
{
    FIXME("%p, %u, %p.\n", iface, index, stream);

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_sink_GetStreamSinkById(IMFMediaSink *iface, DWORD stream_sink_id,
        IMFStreamSink **stream)
{
    FIXME("%p, %#x, %p.\n", iface, stream_sink_id, stream);

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_sink_SetPresentationClock(IMFMediaSink *iface, IMFPresentationClock *clock)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, clock);

    EnterCriticalSection(&renderer->cs);

    if (renderer->is_shut_down)
        hr = MF_E_SHUTDOWN;
    else
    {
        if (renderer->clock)
        {
            IMFPresentationClock_RemoveClockStateSink(renderer->clock, &renderer->IMFClockStateSink_iface);
            IMFPresentationClock_Release(renderer->clock);
        }
        renderer->clock = clock;
        if (renderer->clock)
        {
            IMFPresentationClock_AddRef(renderer->clock);
            IMFPresentationClock_AddClockStateSink(renderer->clock, &renderer->IMFClockStateSink_iface);
        }
    }

    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static HRESULT WINAPI audio_renderer_sink_GetPresentationClock(IMFMediaSink *iface, IMFPresentationClock **clock)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, clock);

    if (!clock)
        return E_POINTER;

    EnterCriticalSection(&renderer->cs);

    if (renderer->is_shut_down)
        hr = MF_E_SHUTDOWN;
    else if (renderer->clock)
    {
        *clock = renderer->clock;
        IMFPresentationClock_AddRef(*clock);
    }
    else
        hr = MF_E_NO_CLOCK;

    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static HRESULT WINAPI audio_renderer_sink_Shutdown(IMFMediaSink *iface)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);

    TRACE("%p.\n", iface);

    if (renderer->is_shut_down)
        return MF_E_SHUTDOWN;

    EnterCriticalSection(&renderer->cs);
    renderer->is_shut_down = TRUE;
    IMFMediaEventQueue_Shutdown(renderer->event_queue);
    LeaveCriticalSection(&renderer->cs);

    return S_OK;
}

static const IMFMediaSinkVtbl audio_renderer_sink_vtbl =
{
    audio_renderer_sink_QueryInterface,
    audio_renderer_sink_AddRef,
    audio_renderer_sink_Release,
    audio_renderer_sink_GetCharacteristics,
    audio_renderer_sink_AddStreamSink,
    audio_renderer_sink_RemoveStreamSink,
    audio_renderer_sink_GetStreamSinkCount,
    audio_renderer_sink_GetStreamSinkByIndex,
    audio_renderer_sink_GetStreamSinkById,
    audio_renderer_sink_SetPresentationClock,
    audio_renderer_sink_GetPresentationClock,
    audio_renderer_sink_Shutdown,
};

static HRESULT WINAPI audio_renderer_preroll_QueryInterface(IMFMediaSinkPreroll *iface, REFIID riid, void **obj)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSinkPreroll(iface);
    return IMFMediaSink_QueryInterface(&renderer->IMFMediaSink_iface, riid, obj);
}

static ULONG WINAPI audio_renderer_preroll_AddRef(IMFMediaSinkPreroll *iface)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSinkPreroll(iface);
    return IMFMediaSink_AddRef(&renderer->IMFMediaSink_iface);
}

static ULONG WINAPI audio_renderer_preroll_Release(IMFMediaSinkPreroll *iface)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSinkPreroll(iface);
    return IMFMediaSink_Release(&renderer->IMFMediaSink_iface);
}

static HRESULT WINAPI audio_renderer_preroll_NotifyPreroll(IMFMediaSinkPreroll *iface, MFTIME start_time)
{
    FIXME("%p, %s.\n", iface, debugstr_time(start_time));

    return E_NOTIMPL;
}

static const IMFMediaSinkPrerollVtbl audio_renderer_preroll_vtbl =
{
    audio_renderer_preroll_QueryInterface,
    audio_renderer_preroll_AddRef,
    audio_renderer_preroll_Release,
    audio_renderer_preroll_NotifyPreroll,
};

static HRESULT WINAPI audio_renderer_events_QueryInterface(IMFMediaEventGenerator *iface, REFIID riid, void **obj)
{
    struct audio_renderer *renderer = impl_from_IMFMediaEventGenerator(iface);
    return IMFMediaSink_QueryInterface(&renderer->IMFMediaSink_iface, riid, obj);
}

static ULONG WINAPI audio_renderer_events_AddRef(IMFMediaEventGenerator *iface)
{
    struct audio_renderer *renderer = impl_from_IMFMediaEventGenerator(iface);
    return IMFMediaSink_AddRef(&renderer->IMFMediaSink_iface);
}

static ULONG WINAPI audio_renderer_events_Release(IMFMediaEventGenerator *iface)
{
    struct audio_renderer *renderer = impl_from_IMFMediaEventGenerator(iface);
    return IMFMediaSink_Release(&renderer->IMFMediaSink_iface);
}

static HRESULT WINAPI audio_renderer_events_GetEvent(IMFMediaEventGenerator *iface, DWORD flags, IMFMediaEvent **event)
{
    struct audio_renderer *renderer = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %#x, %p.\n", iface, flags, event);

    return IMFMediaEventQueue_GetEvent(renderer->event_queue, flags, event);
}

static HRESULT WINAPI audio_renderer_events_BeginGetEvent(IMFMediaEventGenerator *iface, IMFAsyncCallback *callback,
        IUnknown *state)
{
    struct audio_renderer *renderer = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %p, %p.\n", iface, callback, state);

    return IMFMediaEventQueue_BeginGetEvent(renderer->event_queue, callback, state);
}

static HRESULT WINAPI audio_renderer_events_EndGetEvent(IMFMediaEventGenerator *iface, IMFAsyncResult *result,
        IMFMediaEvent **event)
{
    struct audio_renderer *renderer = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %p, %p.\n", iface, result, event);

    return IMFMediaEventQueue_EndGetEvent(renderer->event_queue, result, event);
}

static HRESULT WINAPI audio_renderer_events_QueueEvent(IMFMediaEventGenerator *iface, MediaEventType event_type,
        REFGUID ext_type, HRESULT hr, const PROPVARIANT *value)
{
    struct audio_renderer *renderer = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %u, %s, %#x, %p.\n", iface, event_type, debugstr_guid(ext_type), hr, value);

    return IMFMediaEventQueue_QueueEventParamVar(renderer->event_queue, event_type, ext_type, hr, value);
}

static const IMFMediaEventGeneratorVtbl audio_renderer_events_vtbl =
{
    audio_renderer_events_QueryInterface,
    audio_renderer_events_AddRef,
    audio_renderer_events_Release,
    audio_renderer_events_GetEvent,
    audio_renderer_events_BeginGetEvent,
    audio_renderer_events_EndGetEvent,
    audio_renderer_events_QueueEvent,
};

static HRESULT WINAPI audio_renderer_clock_sink_QueryInterface(IMFClockStateSink *iface, REFIID riid, void **obj)
{
    struct audio_renderer *renderer = impl_from_IMFClockStateSink(iface);
    return IMFMediaSink_QueryInterface(&renderer->IMFMediaSink_iface, riid, obj);
}

static ULONG WINAPI audio_renderer_clock_sink_AddRef(IMFClockStateSink *iface)
{
    struct audio_renderer *renderer = impl_from_IMFClockStateSink(iface);
    return IMFMediaSink_AddRef(&renderer->IMFMediaSink_iface);
}

static ULONG WINAPI audio_renderer_clock_sink_Release(IMFClockStateSink *iface)
{
    struct audio_renderer *renderer = impl_from_IMFClockStateSink(iface);
    return IMFMediaSink_Release(&renderer->IMFMediaSink_iface);
}

static HRESULT WINAPI audio_renderer_clock_sink_OnClockStart(IMFClockStateSink *iface, MFTIME systime, LONGLONG offset)
{
    FIXME("%p, %s, %s.\n", iface, debugstr_time(systime), debugstr_time(offset));

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_clock_sink_OnClockStop(IMFClockStateSink *iface, MFTIME systime)
{
    FIXME("%p, %s.\n", iface, debugstr_time(systime));

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_clock_sink_OnClockPause(IMFClockStateSink *iface, MFTIME systime)
{
    FIXME("%p, %s.\n", iface, debugstr_time(systime));

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_clock_sink_OnClockRestart(IMFClockStateSink *iface, MFTIME systime)
{
    FIXME("%p, %s.\n", iface, debugstr_time(systime));

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_clock_sink_OnClockSetRate(IMFClockStateSink *iface, MFTIME systime, float rate)
{
    FIXME("%p, %s, %f.\n", iface, debugstr_time(systime), rate);

    return E_NOTIMPL;
}

static const IMFClockStateSinkVtbl audio_renderer_clock_sink_vtbl =
{
    audio_renderer_clock_sink_QueryInterface,
    audio_renderer_clock_sink_AddRef,
    audio_renderer_clock_sink_Release,
    audio_renderer_clock_sink_OnClockStart,
    audio_renderer_clock_sink_OnClockStop,
    audio_renderer_clock_sink_OnClockPause,
    audio_renderer_clock_sink_OnClockRestart,
    audio_renderer_clock_sink_OnClockSetRate,
};

static HRESULT sar_create_mmdevice(IMFAttributes *attributes, IMMDevice **device)
{
    WCHAR *endpoint;
    unsigned int length, role = eMultimedia;
    IMMDeviceEnumerator *devenum;
    HRESULT hr;

    if (attributes)
    {
        /* Mutually exclusive attributes. */
        if (SUCCEEDED(IMFAttributes_GetItem(attributes, &MF_AUDIO_RENDERER_ATTRIBUTE_ENDPOINT_ROLE, NULL)) &&
                SUCCEEDED(IMFAttributes_GetItem(attributes, &MF_AUDIO_RENDERER_ATTRIBUTE_ENDPOINT_ID, NULL)))
        {
            return E_INVALIDARG;
        }
    }

    if (FAILED(hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator,
            (void **)&devenum)))
    {
        return hr;
    }

    role = eMultimedia;
    if (attributes && SUCCEEDED(IMFAttributes_GetUINT32(attributes, &MF_AUDIO_RENDERER_ATTRIBUTE_ENDPOINT_ROLE, &role)))
        TRACE("Specified role %d.\n", role);

    if (attributes && SUCCEEDED(IMFAttributes_GetAllocatedString(attributes, &MF_AUDIO_RENDERER_ATTRIBUTE_ENDPOINT_ID,
            &endpoint, &length)))
    {
        TRACE("Specified end point %s.\n", debugstr_w(endpoint));
        hr = IMMDeviceEnumerator_GetDevice(devenum, endpoint, device);
        CoTaskMemFree(endpoint);
    }
    else
        hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(devenum, eRender, role, device);

    if (FAILED(hr))
        hr = MF_E_NO_AUDIO_PLAYBACK_DEVICE;

    IMMDeviceEnumerator_Release(devenum);

    return hr;
}

static HRESULT sar_create_object(IMFAttributes *attributes, void *user_context, IUnknown **obj)
{
    struct audio_renderer *renderer;
    IMMDevice *device;
    HRESULT hr;

    TRACE("%p, %p, %p.\n", attributes, user_context, obj);

    if (!(renderer = heap_alloc_zero(sizeof(*renderer))))
        return E_OUTOFMEMORY;

    renderer->IMFMediaSink_iface.lpVtbl = &audio_renderer_sink_vtbl;
    renderer->IMFMediaSinkPreroll_iface.lpVtbl = &audio_renderer_preroll_vtbl;
    renderer->IMFClockStateSink_iface.lpVtbl = &audio_renderer_clock_sink_vtbl;
    renderer->IMFMediaEventGenerator_iface.lpVtbl = &audio_renderer_events_vtbl;
    renderer->refcount = 1;
    InitializeCriticalSection(&renderer->cs);

    if (FAILED(hr = MFCreateEventQueue(&renderer->event_queue)))
        goto failed;

    if (FAILED(hr = sar_create_mmdevice(attributes, &device)))
        goto failed;

    IMMDevice_Release(device);

    *obj = (IUnknown *)&renderer->IMFMediaSink_iface;

    return S_OK;

failed:

    IMFMediaSink_Release(&renderer->IMFMediaSink_iface);

    return hr;
}

static void sar_shutdown_object(void *user_context, IUnknown *obj)
{
    /* FIXME: shut down sink */
}

static void sar_free_private(void *user_context)
{
}

static const struct activate_funcs sar_activate_funcs =
{
    sar_create_object,
    sar_shutdown_object,
    sar_free_private,
};

/***********************************************************************
 *      MFCreateAudioRendererActivate (mf.@)
 */
HRESULT WINAPI MFCreateAudioRendererActivate(IMFActivate **activate)
{
    TRACE("%p.\n", activate);

    if (!activate)
        return E_POINTER;

    return create_activation_object(NULL, &sar_activate_funcs, activate);
}

/***********************************************************************
 *      MFCreateAudioRenderer (mf.@)
 */
HRESULT WINAPI MFCreateAudioRenderer(IMFAttributes *attributes, IMFMediaSink **sink)
{
    IUnknown *object;
    HRESULT hr;

    TRACE("%p, %p.\n", attributes, sink);

    if (SUCCEEDED(hr = sar_create_object(attributes, NULL, &object)))
    {
        hr = IUnknown_QueryInterface(object, &IID_IMFMediaSink, (void **)sink);
        IUnknown_Release(object);
    }

    return hr;
}
