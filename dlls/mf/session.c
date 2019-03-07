/*
 * Copyright 2017 Nikolay Sivov
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
#include "config.h"

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "mfidl.h"
#include "mfapi.h"
#include "mferror.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

struct media_session
{
    IMFMediaSession IMFMediaSession_iface;
    LONG refcount;
    IMFMediaEventQueue *event_queue;
};

struct presentation_clock
{
    IMFPresentationClock IMFPresentationClock_iface;
    IMFRateControl IMFRateControl_iface;
    IMFTimer IMFTimer_iface;
    IMFShutdown IMFShutdown_iface;
    LONG refcount;
    IMFPresentationTimeSource *time_source;
    IMFClockStateSink *time_source_sink;
    MFCLOCK_STATE state;
    CRITICAL_SECTION cs;
};

static inline struct media_session *impl_from_IMFMediaSession(IMFMediaSession *iface)
{
    return CONTAINING_RECORD(iface, struct media_session, IMFMediaSession_iface);
}

static struct presentation_clock *impl_from_IMFPresentationClock(IMFPresentationClock *iface)
{
    return CONTAINING_RECORD(iface, struct presentation_clock, IMFPresentationClock_iface);
}

static struct presentation_clock *impl_from_IMFRateControl(IMFRateControl *iface)
{
    return CONTAINING_RECORD(iface, struct presentation_clock, IMFRateControl_iface);
}

static struct presentation_clock *impl_from_IMFTimer(IMFTimer *iface)
{
    return CONTAINING_RECORD(iface, struct presentation_clock, IMFTimer_iface);
}

static struct presentation_clock *impl_from_IMFShutdown(IMFShutdown *iface)
{
    return CONTAINING_RECORD(iface, struct presentation_clock, IMFShutdown_iface);
}

static HRESULT WINAPI mfsession_QueryInterface(IMFMediaSession *iface, REFIID riid, void **out)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);

    TRACE("(%p)->(%s %p)\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFMediaSession) ||
            IsEqualIID(riid, &IID_IMFMediaEventGenerator) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &session->IMFMediaSession_iface;
        IMFMediaSession_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI mfsession_AddRef(IMFMediaSession *iface)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);
    ULONG refcount = InterlockedIncrement(&session->refcount);

    TRACE("(%p) refcount=%u\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI mfsession_Release(IMFMediaSession *iface)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);
    ULONG refcount = InterlockedDecrement(&session->refcount);

    TRACE("(%p) refcount=%u\n", iface, refcount);

    if (!refcount)
    {
        if (session->event_queue)
            IMFMediaEventQueue_Release(session->event_queue);
        heap_free(session);
    }

    return refcount;
}

static HRESULT WINAPI mfsession_GetEvent(IMFMediaSession *iface, DWORD flags, IMFMediaEvent **event)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);

    TRACE("(%p)->(%#x, %p)\n", iface, flags, event);

    return IMFMediaEventQueue_GetEvent(session->event_queue, flags, event);
}

static HRESULT WINAPI mfsession_BeginGetEvent(IMFMediaSession *iface, IMFAsyncCallback *callback, IUnknown *state)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);

    TRACE("(%p)->(%p, %p)\n", iface, callback, state);

    return IMFMediaEventQueue_BeginGetEvent(session->event_queue, callback, state);
}

static HRESULT WINAPI mfsession_EndGetEvent(IMFMediaSession *iface, IMFAsyncResult *result, IMFMediaEvent **event)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);

    TRACE("(%p)->(%p, %p)\n", iface, result, event);

    return IMFMediaEventQueue_EndGetEvent(session->event_queue, result, event);
}

static HRESULT WINAPI mfsession_QueueEvent(IMFMediaSession *iface, MediaEventType event_type, REFGUID ext_type,
        HRESULT hr, const PROPVARIANT *value)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);

    TRACE("(%p)->(%d, %s, %#x, %p)\n", iface, event_type, debugstr_guid(ext_type), hr, value);

    return IMFMediaEventQueue_QueueEventParamVar(session->event_queue, event_type, ext_type, hr, value);
}

static HRESULT WINAPI mfsession_SetTopology(IMFMediaSession *iface, DWORD flags, IMFTopology *topology)
{
    FIXME("(%p)->(%#x, %p)\n", iface, flags, topology);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsession_ClearTopologies(IMFMediaSession *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsession_Start(IMFMediaSession *iface, const GUID *format, const PROPVARIANT *start)
{
    FIXME("(%p)->(%s, %p)\n", iface, debugstr_guid(format), start);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsession_Pause(IMFMediaSession *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsession_Stop(IMFMediaSession *iface)
{
    FIXME("(%p)\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsession_Close(IMFMediaSession *iface)
{
    FIXME("(%p)\n", iface);

    return S_OK;
}

static HRESULT WINAPI mfsession_Shutdown(IMFMediaSession *iface)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);

    FIXME("(%p)\n", iface);

    IMFMediaEventQueue_Shutdown(session->event_queue);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsession_GetClock(IMFMediaSession *iface, IMFClock **clock)
{
    FIXME("(%p)->(%p)\n", iface, clock);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsession_GetSessionCapabilities(IMFMediaSession *iface, DWORD *caps)
{
    FIXME("(%p)->(%p)\n", iface, caps);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsession_GetFullTopology(IMFMediaSession *iface, DWORD flags, TOPOID id, IMFTopology **topology)
{
    FIXME("(%p)->(%#x, %s, %p)\n", iface, flags, wine_dbgstr_longlong(id), topology);

    return E_NOTIMPL;
}

static const IMFMediaSessionVtbl mfmediasessionvtbl =
{
    mfsession_QueryInterface,
    mfsession_AddRef,
    mfsession_Release,
    mfsession_GetEvent,
    mfsession_BeginGetEvent,
    mfsession_EndGetEvent,
    mfsession_QueueEvent,
    mfsession_SetTopology,
    mfsession_ClearTopologies,
    mfsession_Start,
    mfsession_Pause,
    mfsession_Stop,
    mfsession_Close,
    mfsession_Shutdown,
    mfsession_GetClock,
    mfsession_GetSessionCapabilities,
    mfsession_GetFullTopology,
};

/***********************************************************************
 *      MFCreateMediaSession (mf.@)
 */
HRESULT WINAPI MFCreateMediaSession(IMFAttributes *config, IMFMediaSession **session)
{
    struct media_session *object;
    HRESULT hr;

    TRACE("(%p, %p)\n", config, session);

    if (config)
        FIXME("session configuration ignored\n");

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFMediaSession_iface.lpVtbl = &mfmediasessionvtbl;
    object->refcount = 1;
    if (FAILED(hr = MFCreateEventQueue(&object->event_queue)))
    {
        IMFMediaSession_Release(&object->IMFMediaSession_iface);
        return hr;
    }

    *session = &object->IMFMediaSession_iface;

    return S_OK;
}

static HRESULT WINAPI present_clock_QueryInterface(IMFPresentationClock *iface, REFIID riid, void **out)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFPresentationClock) ||
            IsEqualIID(riid, &IID_IMFClock) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &clock->IMFPresentationClock_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFRateControl))
    {
        *out = &clock->IMFRateControl_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFTimer))
    {
        *out = &clock->IMFTimer_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFShutdown))
    {
        *out = &clock->IMFShutdown_iface;
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

static ULONG WINAPI present_clock_AddRef(IMFPresentationClock *iface)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    ULONG refcount = InterlockedIncrement(&clock->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI present_clock_Release(IMFPresentationClock *iface)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    ULONG refcount = InterlockedDecrement(&clock->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        if (clock->time_source)
            IMFPresentationTimeSource_Release(clock->time_source);
        if (clock->time_source_sink)
            IMFClockStateSink_Release(clock->time_source_sink);
        DeleteCriticalSection(&clock->cs);
        heap_free(clock);
    }

    return refcount;
}

static HRESULT WINAPI present_clock_GetClockCharacteristics(IMFPresentationClock *iface, DWORD *flags)
{
    FIXME("%p, %p.\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI present_clock_GetCorrelatedTime(IMFPresentationClock *iface, DWORD reserved,
        LONGLONG *clock_time, MFTIME *system_time)
{
    FIXME("%p, %#x, %p, %p.\n", iface, reserved, clock_time, system_time);

    return E_NOTIMPL;
}

static HRESULT WINAPI present_clock_GetContinuityKey(IMFPresentationClock *iface, DWORD *key)
{
    TRACE("%p, %p.\n", iface, key);

    *key = 0;

    return S_OK;
}

static HRESULT WINAPI present_clock_GetState(IMFPresentationClock *iface, DWORD reserved, MFCLOCK_STATE *state)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);

    TRACE("%p, %#x, %p.\n", iface, reserved, state);

    EnterCriticalSection(&clock->cs);
    *state = clock->state;
    LeaveCriticalSection(&clock->cs);

    return S_OK;
}

static HRESULT WINAPI present_clock_GetProperties(IMFPresentationClock *iface, MFCLOCK_PROPERTIES *props)
{
    FIXME("%p, %p.\n", iface, props);

    return E_NOTIMPL;
}

static HRESULT WINAPI present_clock_SetTimeSource(IMFPresentationClock *iface,
        IMFPresentationTimeSource *time_source)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    HRESULT hr;

    TRACE("%p, %p.\n", iface, time_source);

    EnterCriticalSection(&clock->cs);
    if (clock->time_source)
        IMFPresentationTimeSource_Release(clock->time_source);
    if (clock->time_source_sink)
        IMFClockStateSink_Release(clock->time_source_sink);
    clock->time_source = NULL;
    clock->time_source_sink = NULL;

    hr = IMFPresentationTimeSource_QueryInterface(time_source, &IID_IMFClockStateSink, (void **)&clock->time_source_sink);
    if (SUCCEEDED(hr))
    {
        clock->time_source = time_source;
        IMFPresentationTimeSource_AddRef(clock->time_source);
    }

    LeaveCriticalSection(&clock->cs);

    return hr;
}

static HRESULT WINAPI present_clock_GetTimeSource(IMFPresentationClock *iface,
        IMFPresentationTimeSource **time_source)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, time_source);

    EnterCriticalSection(&clock->cs);
    if (clock->time_source)
    {
        *time_source = clock->time_source;
        IMFPresentationTimeSource_AddRef(*time_source);
    }
    else
        hr = MF_E_CLOCK_NO_TIME_SOURCE;
    LeaveCriticalSection(&clock->cs);

    return hr;
}

static HRESULT WINAPI present_clock_GetTime(IMFPresentationClock *iface, MFTIME *time)
{
    FIXME("%p, %p.\n", iface, time);

    return E_NOTIMPL;
}

static HRESULT WINAPI present_clock_AddClockStateSink(IMFPresentationClock *iface, IMFClockStateSink *state_sink)
{
    FIXME("%p, %p.\n", iface, state_sink);

    return E_NOTIMPL;
}

static HRESULT WINAPI present_clock_RemoveClockStateSink(IMFPresentationClock *iface,
        IMFClockStateSink *state_sink)
{
    FIXME("%p, %p.\n", iface, state_sink);

    return E_NOTIMPL;
}

enum clock_command
{
    CLOCK_CMD_START = 0,
    CLOCK_CMD_STOP,
    CLOCK_CMD_PAUSE,
    CLOCK_CMD_MAX,
};

static HRESULT clock_change_state(struct presentation_clock *clock, enum clock_command command)
{
    static const BYTE state_change_is_allowed[MFCLOCK_STATE_PAUSED+1][CLOCK_CMD_MAX] =
    {   /*              S  S* P  */
        /* INVALID */ { 1, 1, 1 },
        /* RUNNING */ { 1, 1, 1 },
        /* STOPPED */ { 1, 1, 0 },
        /* PAUSED  */ { 1, 1, 0 },
    };
    static const MFCLOCK_STATE states[CLOCK_CMD_MAX] =
    {
        /* CLOCK_CMD_START */ MFCLOCK_STATE_RUNNING,
        /* CLOCK_CMD_STOP  */ MFCLOCK_STATE_STOPPED,
        /* CLOCK_CMD_PAUSE */ MFCLOCK_STATE_PAUSED,
    };
    HRESULT hr;

    /* FIXME: use correct timestamps. */

    if (clock->state == states[command] && clock->state != MFCLOCK_STATE_RUNNING)
        return MF_E_CLOCK_STATE_ALREADY_SET;

    if (!state_change_is_allowed[clock->state][command])
        return MF_E_INVALIDREQUEST;

    switch (command)
    {
        case CLOCK_CMD_START:
            if (clock->state == MFCLOCK_STATE_PAUSED)
                hr = IMFClockStateSink_OnClockRestart(clock->time_source_sink, 0);
            else
                hr = IMFClockStateSink_OnClockStart(clock->time_source_sink, 0, 0);
            break;
        case CLOCK_CMD_STOP:
            hr = IMFClockStateSink_OnClockStop(clock->time_source_sink, 0);
            break;
        case CLOCK_CMD_PAUSE:
            hr = IMFClockStateSink_OnClockPause(clock->time_source_sink, 0);
            break;
        default:
            ;
    }

    if (FAILED(hr))
        return hr;

    clock->state = states[command];

    /* FIXME: notify registered sinks. */

    return S_OK;
}

static HRESULT WINAPI present_clock_Start(IMFPresentationClock *iface, LONGLONG start_offset)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    HRESULT hr;

    TRACE("%p, %s.\n", iface, wine_dbgstr_longlong(start_offset));

    EnterCriticalSection(&clock->cs);
    hr = clock_change_state(clock, CLOCK_CMD_START);
    LeaveCriticalSection(&clock->cs);

    return hr;
}

static HRESULT WINAPI present_clock_Stop(IMFPresentationClock *iface)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    HRESULT hr;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&clock->cs);
    hr = clock_change_state(clock, CLOCK_CMD_STOP);
    LeaveCriticalSection(&clock->cs);

    return hr;
}

static HRESULT WINAPI present_clock_Pause(IMFPresentationClock *iface)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    HRESULT hr;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&clock->cs);
    hr = clock_change_state(clock, CLOCK_CMD_PAUSE);
    LeaveCriticalSection(&clock->cs);

    return hr;
}

static const IMFPresentationClockVtbl presentationclockvtbl =
{
    present_clock_QueryInterface,
    present_clock_AddRef,
    present_clock_Release,
    present_clock_GetClockCharacteristics,
    present_clock_GetCorrelatedTime,
    present_clock_GetContinuityKey,
    present_clock_GetState,
    present_clock_GetProperties,
    present_clock_SetTimeSource,
    present_clock_GetTimeSource,
    present_clock_GetTime,
    present_clock_AddClockStateSink,
    present_clock_RemoveClockStateSink,
    present_clock_Start,
    present_clock_Stop,
    present_clock_Pause,
};

static HRESULT WINAPI present_clock_rate_control_QueryInterface(IMFRateControl *iface, REFIID riid, void **out)
{
    struct presentation_clock *clock = impl_from_IMFRateControl(iface);
    return IMFPresentationClock_QueryInterface(&clock->IMFPresentationClock_iface, riid, out);
}

static ULONG WINAPI present_clock_rate_control_AddRef(IMFRateControl *iface)
{
    struct presentation_clock *clock = impl_from_IMFRateControl(iface);
    return IMFPresentationClock_AddRef(&clock->IMFPresentationClock_iface);
}

static ULONG WINAPI present_clock_rate_control_Release(IMFRateControl *iface)
{
    struct presentation_clock *clock = impl_from_IMFRateControl(iface);
    return IMFPresentationClock_Release(&clock->IMFPresentationClock_iface);
}

static HRESULT WINAPI present_clock_rate_SetRate(IMFRateControl *iface, BOOL thin, float rate)
{
    FIXME("%p, %d, %f.\n", iface, thin, rate);

    return E_NOTIMPL;
}

static HRESULT WINAPI present_clock_rate_GetRate(IMFRateControl *iface, BOOL *thin, float *rate)
{
    FIXME("%p, %p, %p.\n", iface, thin, rate);

    return E_NOTIMPL;
}

static const IMFRateControlVtbl presentclockratecontrolvtbl =
{
    present_clock_rate_control_QueryInterface,
    present_clock_rate_control_AddRef,
    present_clock_rate_control_Release,
    present_clock_rate_SetRate,
    present_clock_rate_GetRate,
};

static HRESULT WINAPI present_clock_timer_QueryInterface(IMFTimer *iface, REFIID riid, void **out)
{
    struct presentation_clock *clock = impl_from_IMFTimer(iface);
    return IMFPresentationClock_QueryInterface(&clock->IMFPresentationClock_iface, riid, out);
}

static ULONG WINAPI present_clock_timer_AddRef(IMFTimer *iface)
{
    struct presentation_clock *clock = impl_from_IMFTimer(iface);
    return IMFPresentationClock_AddRef(&clock->IMFPresentationClock_iface);
}

static ULONG WINAPI present_clock_timer_Release(IMFTimer *iface)
{
    struct presentation_clock *clock = impl_from_IMFTimer(iface);
    return IMFPresentationClock_Release(&clock->IMFPresentationClock_iface);
}

static HRESULT WINAPI present_clock_timer_SetTimer(IMFTimer *iface, DWORD flags, LONGLONG time,
        IMFAsyncCallback *callback, IUnknown *state, IUnknown **cancel_key)
{
    FIXME("%p, %#x, %s, %p, %p, %p.\n", iface, flags, wine_dbgstr_longlong(time), callback, state, cancel_key);

    return E_NOTIMPL;
}

static HRESULT WINAPI present_clock_timer_CancelTimer(IMFTimer *iface, IUnknown *cancel_key)
{
    FIXME("%p, %p.\n", iface, cancel_key);

    return E_NOTIMPL;
}

static const IMFTimerVtbl presentclocktimervtbl =
{
    present_clock_timer_QueryInterface,
    present_clock_timer_AddRef,
    present_clock_timer_Release,
    present_clock_timer_SetTimer,
    present_clock_timer_CancelTimer,
};

static HRESULT WINAPI present_clock_shutdown_QueryInterface(IMFShutdown *iface, REFIID riid, void **out)
{
    struct presentation_clock *clock = impl_from_IMFShutdown(iface);
    return IMFPresentationClock_QueryInterface(&clock->IMFPresentationClock_iface, riid, out);
}

static ULONG WINAPI present_clock_shutdown_AddRef(IMFShutdown *iface)
{
    struct presentation_clock *clock = impl_from_IMFShutdown(iface);
    return IMFPresentationClock_AddRef(&clock->IMFPresentationClock_iface);
}

static ULONG WINAPI present_clock_shutdown_Release(IMFShutdown *iface)
{
    struct presentation_clock *clock = impl_from_IMFShutdown(iface);
    return IMFPresentationClock_Release(&clock->IMFPresentationClock_iface);
}

static HRESULT WINAPI present_clock_shutdown_Shutdown(IMFShutdown *iface)
{
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI present_clock_shutdown_GetShutdownStatus(IMFShutdown *iface, MFSHUTDOWN_STATUS *status)
{
    FIXME("%p, %p.\n", iface, status);

    return E_NOTIMPL;
}

static const IMFShutdownVtbl presentclockshutdownvtbl =
{
    present_clock_shutdown_QueryInterface,
    present_clock_shutdown_AddRef,
    present_clock_shutdown_Release,
    present_clock_shutdown_Shutdown,
    present_clock_shutdown_GetShutdownStatus,
};

/***********************************************************************
 *      MFCreatePresentationClock (mf.@)
 */
HRESULT WINAPI MFCreatePresentationClock(IMFPresentationClock **clock)
{
    struct presentation_clock *object;

    TRACE("%p.\n", clock);

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFPresentationClock_iface.lpVtbl = &presentationclockvtbl;
    object->IMFRateControl_iface.lpVtbl = &presentclockratecontrolvtbl;
    object->IMFTimer_iface.lpVtbl = &presentclocktimervtbl;
    object->IMFShutdown_iface.lpVtbl = &presentclockshutdownvtbl;
    object->refcount = 1;
    InitializeCriticalSection(&object->cs);

    *clock = &object->IMFPresentationClock_iface;

    return S_OK;
}
