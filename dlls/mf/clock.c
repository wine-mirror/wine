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

#define COBJMACROS

#include "wine/debug.h"
#include "wine/list.h"

#include "mf_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

struct clock_sink
{
    struct list entry;
    IMFClockStateSink *state_sink;
};

enum clock_command
{
    CLOCK_CMD_START = 0,
    CLOCK_CMD_STOP,
    CLOCK_CMD_PAUSE,
    CLOCK_CMD_SET_RATE,
    CLOCK_CMD_MAX,
};

enum clock_notification
{
    CLOCK_NOTIFY_START,
    CLOCK_NOTIFY_STOP,
    CLOCK_NOTIFY_PAUSE,
    CLOCK_NOTIFY_RESTART,
    CLOCK_NOTIFY_SET_RATE,
};

struct clock_state_change_param
{
    union
    {
        LONGLONG offset;
        float rate;
    } u;
};

struct sink_notification
{
    IUnknown IUnknown_iface;
    LONG refcount;
    MFTIME system_time;
    struct clock_state_change_param param;
    enum clock_notification notification;
    IMFClockStateSink *sink;
};

struct clock_timer
{
    IUnknown IUnknown_iface;
    LONG refcount;
    IMFAsyncResult *result;
    IMFAsyncCallback *callback;
    MFWORKITEM_KEY key;
    struct list entry;
};

struct presentation_clock
{
    IMFPresentationClock IMFPresentationClock_iface;
    IMFRateControl IMFRateControl_iface;
    IMFTimer IMFTimer_iface;
    IMFShutdown IMFShutdown_iface;
    IMFAsyncCallback sink_callback;
    IMFAsyncCallback timer_callback;
    LONG refcount;
    IMFPresentationTimeSource *time_source;
    IMFClockStateSink *time_source_sink;
    MFCLOCK_STATE state;
    LONGLONG start_offset;
    struct list sinks;
    struct list timers;
    float rate;
    LONGLONG frequency;
    CRITICAL_SECTION cs;
    BOOL is_shut_down;
};

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

static struct presentation_clock *impl_from_sink_callback_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct presentation_clock, sink_callback);
}

static struct presentation_clock *impl_from_timer_callback_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct presentation_clock, timer_callback);
}

static struct clock_timer *impl_clock_timer_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct clock_timer, IUnknown_iface);
}

static struct sink_notification *impl_sink_notification_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct sink_notification, IUnknown_iface);
}

static HRESULT WINAPI sink_notification_QueryInterface(IUnknown *iface, REFIID riid, void **out)
{
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI sink_notification_AddRef(IUnknown *iface)
{
    struct sink_notification *notification = impl_sink_notification_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&notification->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI sink_notification_Release(IUnknown *iface)
{
    struct sink_notification *notification = impl_sink_notification_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&notification->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        IMFClockStateSink_Release(notification->sink);
        free(notification);
    }

    return refcount;
}

static const IUnknownVtbl sinknotificationvtbl =
{
    sink_notification_QueryInterface,
    sink_notification_AddRef,
    sink_notification_Release,
};

static void clock_notify_async_sink(struct presentation_clock *clock, MFTIME system_time,
        struct clock_state_change_param param, enum clock_notification notification, IMFClockStateSink *sink)
{
    struct sink_notification *object;
    IMFAsyncResult *result;
    HRESULT hr;

    if (!(object = malloc(sizeof(*object))))
        return;

    object->IUnknown_iface.lpVtbl = &sinknotificationvtbl;
    object->refcount = 1;
    object->system_time = system_time;
    object->param = param;
    object->notification = notification;
    object->sink = sink;
    IMFClockStateSink_AddRef(object->sink);

    hr = MFCreateAsyncResult(&object->IUnknown_iface, &clock->sink_callback, NULL, &result);
    IUnknown_Release(&object->IUnknown_iface);
    if (SUCCEEDED(hr))
    {
        MFPutWorkItemEx(MFASYNC_CALLBACK_QUEUE_STANDARD, result);
        IMFAsyncResult_Release(result);
    }
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

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI present_clock_Release(IMFPresentationClock *iface)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    ULONG refcount = InterlockedDecrement(&clock->refcount);
    struct clock_timer *timer, *timer2;
    struct clock_sink *sink, *sink2;

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (clock->time_source)
            IMFPresentationTimeSource_Release(clock->time_source);
        if (clock->time_source_sink)
            IMFClockStateSink_Release(clock->time_source_sink);
        LIST_FOR_EACH_ENTRY_SAFE(sink, sink2, &clock->sinks, struct clock_sink, entry)
        {
            list_remove(&sink->entry);
            IMFClockStateSink_Release(sink->state_sink);
            free(sink);
        }
        LIST_FOR_EACH_ENTRY_SAFE(timer, timer2, &clock->timers, struct clock_timer, entry)
        {
            list_remove(&timer->entry);
            IUnknown_Release(&timer->IUnknown_iface);
        }
        DeleteCriticalSection(&clock->cs);
        free(clock);
    }

    return refcount;
}

static HRESULT WINAPI present_clock_GetClockCharacteristics(IMFPresentationClock *iface, DWORD *flags)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    HRESULT hr = MF_E_CLOCK_NO_TIME_SOURCE;

    TRACE("%p, %p.\n", iface, flags);

    EnterCriticalSection(&clock->cs);
    if (clock->time_source)
        hr = IMFPresentationTimeSource_GetClockCharacteristics(clock->time_source, flags);
    LeaveCriticalSection(&clock->cs);

    return hr;
}

static HRESULT WINAPI present_clock_GetCorrelatedTime(IMFPresentationClock *iface, DWORD reserved,
        LONGLONG *clock_time, MFTIME *system_time)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    HRESULT hr = MF_E_CLOCK_NO_TIME_SOURCE;

    TRACE("%p, %#lx, %p, %p.\n", iface, reserved, clock_time, system_time);

    EnterCriticalSection(&clock->cs);
    if (clock->time_source)
        hr = IMFPresentationTimeSource_GetCorrelatedTime(clock->time_source, reserved, clock_time, system_time);
    LeaveCriticalSection(&clock->cs);

    return hr;
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

    TRACE("%p, %#lx, %p.\n", iface, reserved, state);

    EnterCriticalSection(&clock->cs);
    *state = clock->state;
    LeaveCriticalSection(&clock->cs);

    return S_OK;
}

static HRESULT WINAPI present_clock_GetProperties(IMFPresentationClock *iface, MFCLOCK_PROPERTIES *props)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    HRESULT hr = MF_E_CLOCK_NO_TIME_SOURCE;

    TRACE("%p, %p.\n", iface, props);

    EnterCriticalSection(&clock->cs);
    if (clock->time_source)
        hr = IMFPresentationTimeSource_GetProperties(clock->time_source, props);
    LeaveCriticalSection(&clock->cs);

    return hr;
}

static HRESULT WINAPI present_clock_SetTimeSource(IMFPresentationClock *iface,
        IMFPresentationTimeSource *time_source)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    MFCLOCK_PROPERTIES props;
    IMFClock *source_clock;
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

    if (SUCCEEDED(IMFPresentationTimeSource_GetUnderlyingClock(time_source, &source_clock)))
    {
        if (SUCCEEDED(IMFClock_GetProperties(source_clock, &props)))
            clock->frequency = props.qwClockFrequency;
        IMFClock_Release(source_clock);
    }

    if (!clock->frequency)
        clock->frequency = MFCLOCK_FREQUENCY_HNS;

    LeaveCriticalSection(&clock->cs);

    return hr;
}

static HRESULT WINAPI present_clock_GetTimeSource(IMFPresentationClock *iface,
        IMFPresentationTimeSource **time_source)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, time_source);

    if (!time_source)
        return E_INVALIDARG;

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
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    HRESULT hr = MF_E_CLOCK_NO_TIME_SOURCE;
    MFTIME systime;

    TRACE("%p, %p.\n", iface, time);

    if (!time)
        return E_POINTER;

    EnterCriticalSection(&clock->cs);
    if (clock->time_source)
        hr = IMFPresentationTimeSource_GetCorrelatedTime(clock->time_source, 0, time, &systime);
    LeaveCriticalSection(&clock->cs);

    return hr;
}

static HRESULT WINAPI present_clock_AddClockStateSink(IMFPresentationClock *iface, IMFClockStateSink *state_sink)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    struct clock_sink *sink, *cur;
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, state_sink);

    if (!state_sink)
        return E_INVALIDARG;

    if (!(sink = malloc(sizeof(*sink))))
        return E_OUTOFMEMORY;

    sink->state_sink = state_sink;
    IMFClockStateSink_AddRef(sink->state_sink);

    EnterCriticalSection(&clock->cs);
    LIST_FOR_EACH_ENTRY(cur, &clock->sinks, struct clock_sink, entry)
    {
        if (cur->state_sink == state_sink)
        {
            hr = E_INVALIDARG;
            break;
        }
    }
    if (SUCCEEDED(hr))
    {
        static const enum clock_notification notifications[MFCLOCK_STATE_PAUSED + 1] =
        {
            /* MFCLOCK_STATE_INVALID */ 0, /* Does not apply */
            /* MFCLOCK_STATE_RUNNING */ CLOCK_NOTIFY_START,
            /* MFCLOCK_STATE_STOPPED */ CLOCK_NOTIFY_STOP,
            /* MFCLOCK_STATE_PAUSED  */ CLOCK_NOTIFY_PAUSE,
        };
        struct clock_state_change_param param;

        if (!clock->is_shut_down && clock->state != MFCLOCK_STATE_INVALID)
        {
            param.u.offset = clock->start_offset;
            clock_notify_async_sink(clock, MFGetSystemTime(), param, notifications[clock->state], sink->state_sink);
        }

        list_add_tail(&clock->sinks, &sink->entry);
    }
    LeaveCriticalSection(&clock->cs);

    if (FAILED(hr))
    {
        IMFClockStateSink_Release(sink->state_sink);
        free(sink);
    }

    return hr;
}

static HRESULT WINAPI present_clock_RemoveClockStateSink(IMFPresentationClock *iface,
        IMFClockStateSink *state_sink)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    struct clock_sink *sink;

    TRACE("%p, %p.\n", iface, state_sink);

    if (!state_sink)
        return E_INVALIDARG;

    EnterCriticalSection(&clock->cs);
    LIST_FOR_EACH_ENTRY(sink, &clock->sinks, struct clock_sink, entry)
    {
        if (sink->state_sink == state_sink)
        {
            IMFClockStateSink_Release(sink->state_sink);
            list_remove(&sink->entry);
            free(sink);
            break;
        }
    }
    LeaveCriticalSection(&clock->cs);

    return S_OK;
}

static HRESULT clock_call_state_change(MFTIME system_time, struct clock_state_change_param param,
        enum clock_notification notification, IMFClockStateSink *sink)
{
    HRESULT hr = S_OK;

    switch (notification)
    {
        case CLOCK_NOTIFY_START:
            hr = IMFClockStateSink_OnClockStart(sink, system_time, param.u.offset);
            break;
        case CLOCK_NOTIFY_STOP:
            hr = IMFClockStateSink_OnClockStop(sink, system_time);
            break;
        case CLOCK_NOTIFY_PAUSE:
            hr = IMFClockStateSink_OnClockPause(sink, system_time);
            break;
        case CLOCK_NOTIFY_RESTART:
            hr = IMFClockStateSink_OnClockRestart(sink, system_time);
            break;
        case CLOCK_NOTIFY_SET_RATE:
            /* System time source does not allow 0.0 rate, presentation clock allows it without raising errors. */
            IMFClockStateSink_OnClockSetRate(sink, system_time, param.u.rate);
            break;
        default:
            ;
    }

    return hr;
}

static HRESULT clock_change_state(struct presentation_clock *clock, enum clock_command command,
        struct clock_state_change_param param)
{
    static const BYTE state_change_is_allowed[MFCLOCK_STATE_PAUSED+1][CLOCK_CMD_MAX] =
    {   /*              S  S* P, R  */
        /* INVALID */ { 1, 1, 1, 1 },
        /* RUNNING */ { 1, 1, 1, 1 },
        /* STOPPED */ { 1, 1, 0, 1 },
        /* PAUSED  */ { 1, 1, 0, 1 },
    };
    static const MFCLOCK_STATE states[CLOCK_CMD_MAX] =
    {
        /* CLOCK_CMD_START    */ MFCLOCK_STATE_RUNNING,
        /* CLOCK_CMD_STOP     */ MFCLOCK_STATE_STOPPED,
        /* CLOCK_CMD_PAUSE    */ MFCLOCK_STATE_PAUSED,
        /* CLOCK_CMD_SET_RATE */ 0, /* Unused */
    };
    static const enum clock_notification notifications[CLOCK_CMD_MAX] =
    {
        /* CLOCK_CMD_START    */ CLOCK_NOTIFY_START,
        /* CLOCK_CMD_STOP     */ CLOCK_NOTIFY_STOP,
        /* CLOCK_CMD_PAUSE    */ CLOCK_NOTIFY_PAUSE,
        /* CLOCK_CMD_SET_RATE */ CLOCK_NOTIFY_SET_RATE,
    };
    enum clock_notification notification;
    struct clock_sink *sink;
    MFCLOCK_STATE old_state;
    IMFAsyncResult *result;
    MFTIME system_time;
    HRESULT hr;

    if (!clock->time_source)
        return MF_E_CLOCK_NO_TIME_SOURCE;

    if (command != CLOCK_CMD_SET_RATE && clock->state == states[command] && clock->state != MFCLOCK_STATE_RUNNING)
        return MF_E_CLOCK_STATE_ALREADY_SET;

    if (!state_change_is_allowed[clock->state][command])
        return MF_E_INVALIDREQUEST;

    system_time = MFGetSystemTime();

    if (command == CLOCK_CMD_START && clock->state == MFCLOCK_STATE_PAUSED &&
            param.u.offset == PRESENTATION_CURRENT_POSITION)
    {
        notification = CLOCK_NOTIFY_RESTART;
    }
    else
        notification = notifications[command];

    if (FAILED(hr = clock_call_state_change(system_time, param, notification, clock->time_source_sink)))
        return hr;

    old_state = clock->state;
    if (command != CLOCK_CMD_SET_RATE)
        clock->state = states[command];

    /* Dump all pending timer requests immediately on start; otherwise try to cancel scheduled items when
       transitioning from running state. */
    if ((clock->state == MFCLOCK_STATE_RUNNING) ^ (old_state == MFCLOCK_STATE_RUNNING))
    {
        struct clock_timer *timer, *timer2;

        if (clock->state == MFCLOCK_STATE_RUNNING)
        {
            LIST_FOR_EACH_ENTRY_SAFE(timer, timer2, &clock->timers, struct clock_timer, entry)
            {
                list_remove(&timer->entry);
                hr = MFCreateAsyncResult(&timer->IUnknown_iface, &clock->timer_callback, NULL, &result);
                IUnknown_Release(&timer->IUnknown_iface);
                if (SUCCEEDED(hr))
                {
                    MFPutWorkItemEx(MFASYNC_CALLBACK_QUEUE_TIMER, result);
                    IMFAsyncResult_Release(result);
                }
            }
        }
        else
        {
            LIST_FOR_EACH_ENTRY(timer, &clock->timers, struct clock_timer, entry)
            {
                if (timer->key)
                {
                    MFCancelWorkItem(timer->key);
                    timer->key = 0;
                }
            }
        }
    }

    LIST_FOR_EACH_ENTRY(sink, &clock->sinks, struct clock_sink, entry)
    {
        clock_notify_async_sink(clock, system_time, param, notification, sink->state_sink);
    }

    return S_OK;
}

static HRESULT WINAPI present_clock_Start(IMFPresentationClock *iface, LONGLONG start_offset)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    struct clock_state_change_param param = {{0}};
    HRESULT hr;

    TRACE("%p, %s.\n", iface, debugstr_time(start_offset));

    EnterCriticalSection(&clock->cs);
    clock->start_offset = param.u.offset = start_offset;
    hr = clock_change_state(clock, CLOCK_CMD_START, param);
    LeaveCriticalSection(&clock->cs);

    return hr;
}

static HRESULT WINAPI present_clock_Stop(IMFPresentationClock *iface)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    struct clock_state_change_param param = {{0}};
    HRESULT hr;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&clock->cs);
    hr = clock_change_state(clock, CLOCK_CMD_STOP, param);
    LeaveCriticalSection(&clock->cs);

    return hr;
}

static HRESULT WINAPI present_clock_Pause(IMFPresentationClock *iface)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    struct clock_state_change_param param = {{0}};
    HRESULT hr;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&clock->cs);
    hr = clock_change_state(clock, CLOCK_CMD_PAUSE, param);
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
    struct presentation_clock *clock = impl_from_IMFRateControl(iface);
    struct clock_state_change_param param;
    HRESULT hr;

    TRACE("%p, %d, %f.\n", iface, thin, rate);

    if (thin)
        return MF_E_THINNING_UNSUPPORTED;

    EnterCriticalSection(&clock->cs);
    param.u.rate = rate;
    if (SUCCEEDED(hr = clock_change_state(clock, CLOCK_CMD_SET_RATE, param)))
        clock->rate = rate;
    LeaveCriticalSection(&clock->cs);

    return hr;
}

static HRESULT WINAPI present_clock_rate_GetRate(IMFRateControl *iface, BOOL *thin, float *rate)
{
    struct presentation_clock *clock = impl_from_IMFRateControl(iface);

    TRACE("%p, %p, %p.\n", iface, thin, rate);

    if (!rate)
        return E_INVALIDARG;

    if (thin)
        *thin = FALSE;

    EnterCriticalSection(&clock->cs);
    *rate = clock->rate;
    LeaveCriticalSection(&clock->cs);

    return S_OK;
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

static HRESULT present_clock_schedule_timer(struct presentation_clock *clock, DWORD flags, LONGLONG time,
        struct clock_timer *timer)
{
    IMFAsyncResult *result;
    MFTIME systime, clocktime;
    LONGLONG frequency;
    HRESULT hr;

    if (!(flags & MFTIMER_RELATIVE))
    {
        if (FAILED(hr = IMFPresentationTimeSource_GetCorrelatedTime(clock->time_source, 0, &clocktime, &systime)))
        {
            WARN("Failed to get clock time, hr %#lx.\n", hr);
            return hr;
        }
        if (time > clocktime)
            time -= clocktime;
        else
            time = 0;
    }

    frequency = clock->frequency / 1000;
    time /= frequency;

    /* Scheduled item is using clock instance callback, with timer instance as an object. Clock callback will
       call user callback and cleanup timer list. */

    if (FAILED(hr = MFCreateAsyncResult(&timer->IUnknown_iface, &clock->timer_callback, NULL, &result)))
        return hr;

    hr = MFScheduleWorkItemEx(result, -time, &timer->key);
    IMFAsyncResult_Release(result);

    return hr;
}

static HRESULT WINAPI clock_timer_QueryInterface(IUnknown *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI clock_timer_AddRef(IUnknown *iface)
{
    struct clock_timer *timer = impl_clock_timer_from_IUnknown(iface);
    return InterlockedIncrement(&timer->refcount);
}

static ULONG WINAPI clock_timer_Release(IUnknown *iface)
{
    struct clock_timer *timer = impl_clock_timer_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&timer->refcount);

    if (!refcount)
    {
        IMFAsyncResult_Release(timer->result);
        IMFAsyncCallback_Release(timer->callback);
        free(timer);
    }

    return refcount;
}

static const IUnknownVtbl clock_timer_vtbl =
{
    clock_timer_QueryInterface,
    clock_timer_AddRef,
    clock_timer_Release,
};

static HRESULT WINAPI present_clock_timer_SetTimer(IMFTimer *iface, DWORD flags, LONGLONG time,
        IMFAsyncCallback *callback, IUnknown *state, IUnknown **cancel_key)
{
    struct presentation_clock *clock = impl_from_IMFTimer(iface);
    struct clock_timer *clock_timer;
    HRESULT hr;

    TRACE("%p, %#lx, %s, %p, %p, %p.\n", iface, flags, debugstr_time(time), callback, state, cancel_key);

    if (!(clock_timer = calloc(1, sizeof(*clock_timer))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = MFCreateAsyncResult(NULL, NULL, state, &clock_timer->result)))
    {
        free(clock_timer);
        return hr;
    }

    clock_timer->IUnknown_iface.lpVtbl = &clock_timer_vtbl;
    clock_timer->refcount = 1;
    clock_timer->callback = callback;
    IMFAsyncCallback_AddRef(clock_timer->callback);

    EnterCriticalSection(&clock->cs);

    if (clock->state == MFCLOCK_STATE_RUNNING)
        hr = present_clock_schedule_timer(clock, flags, time, clock_timer);
    else if (clock->state == MFCLOCK_STATE_STOPPED)
        hr = MF_S_CLOCK_STOPPED;

    if (SUCCEEDED(hr))
    {
        list_add_tail(&clock->timers, &clock_timer->entry);
        if (cancel_key)
        {
            *cancel_key = &clock_timer->IUnknown_iface;
            IUnknown_AddRef(*cancel_key);
        }
    }

    LeaveCriticalSection(&clock->cs);

    if (FAILED(hr))
        IUnknown_Release(&clock_timer->IUnknown_iface);

    return hr;
}

static HRESULT WINAPI present_clock_timer_CancelTimer(IMFTimer *iface, IUnknown *cancel_key)
{
    struct presentation_clock *clock = impl_from_IMFTimer(iface);
    struct clock_timer *timer;

    TRACE("%p, %p.\n", iface, cancel_key);

    EnterCriticalSection(&clock->cs);

    LIST_FOR_EACH_ENTRY(timer, &clock->timers, struct clock_timer, entry)
    {
        if (&timer->IUnknown_iface == cancel_key)
        {
            list_remove(&timer->entry);
            if (timer->key)
            {
                MFCancelWorkItem(timer->key);
                timer->key = 0;
            }
            IUnknown_Release(&timer->IUnknown_iface);
            break;
        }
    }

    LeaveCriticalSection(&clock->cs);

    return S_OK;
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
    struct presentation_clock *clock = impl_from_IMFShutdown(iface);

    TRACE("%p.\n", iface);

    EnterCriticalSection(&clock->cs);
    clock->is_shut_down = TRUE;
    LeaveCriticalSection(&clock->cs);

    return S_OK;
}

static HRESULT WINAPI present_clock_shutdown_GetShutdownStatus(IMFShutdown *iface, MFSHUTDOWN_STATUS *status)
{
    struct presentation_clock *clock = impl_from_IMFShutdown(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, status);

    if (!status)
        return E_INVALIDARG;

    EnterCriticalSection(&clock->cs);
    if (clock->is_shut_down)
        *status = MFSHUTDOWN_COMPLETED;
    else
        hr = MF_E_INVALIDREQUEST;
    LeaveCriticalSection(&clock->cs);

    return hr;
}

static const IMFShutdownVtbl presentclockshutdownvtbl =
{
    present_clock_shutdown_QueryInterface,
    present_clock_shutdown_AddRef,
    present_clock_shutdown_Release,
    present_clock_shutdown_Shutdown,
    present_clock_shutdown_GetShutdownStatus,
};

static HRESULT WINAPI present_clock_callback_QueryInterface(IMFAsyncCallback *iface, REFIID riid, void **out)
{
    if (IsEqualIID(riid, &IID_IMFAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
        IMFAsyncCallback_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", wine_dbgstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI present_clock_sink_callback_AddRef(IMFAsyncCallback *iface)
{
    struct presentation_clock *clock = impl_from_sink_callback_IMFAsyncCallback(iface);
    return IMFPresentationClock_AddRef(&clock->IMFPresentationClock_iface);
}

static ULONG WINAPI present_clock_sink_callback_Release(IMFAsyncCallback *iface)
{
    struct presentation_clock *clock = impl_from_sink_callback_IMFAsyncCallback(iface);
    return IMFPresentationClock_Release(&clock->IMFPresentationClock_iface);
}

static HRESULT WINAPI present_clock_callback_GetParameters(IMFAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI present_clock_sink_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct sink_notification *data;
    IUnknown *object;
    HRESULT hr;

    if (FAILED(hr = IMFAsyncResult_GetObject(result, &object)))
        return hr;

    data = impl_sink_notification_from_IUnknown(object);

    clock_call_state_change(data->system_time, data->param, data->notification, data->sink);

    IUnknown_Release(object);

    return S_OK;
}

static const IMFAsyncCallbackVtbl presentclocksinkcallbackvtbl =
{
    present_clock_callback_QueryInterface,
    present_clock_sink_callback_AddRef,
    present_clock_sink_callback_Release,
    present_clock_callback_GetParameters,
    present_clock_sink_callback_Invoke,
};

static ULONG WINAPI present_clock_timer_callback_AddRef(IMFAsyncCallback *iface)
{
    struct presentation_clock *clock = impl_from_timer_callback_IMFAsyncCallback(iface);
    return IMFPresentationClock_AddRef(&clock->IMFPresentationClock_iface);
}

static ULONG WINAPI present_clock_timer_callback_Release(IMFAsyncCallback *iface)
{
    struct presentation_clock *clock = impl_from_timer_callback_IMFAsyncCallback(iface);
    return IMFPresentationClock_Release(&clock->IMFPresentationClock_iface);
}

static HRESULT WINAPI present_clock_timer_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct presentation_clock *clock = impl_from_timer_callback_IMFAsyncCallback(iface);
    struct clock_timer *timer;
    IUnknown *object;
    HRESULT hr;

    if (FAILED(hr = IMFAsyncResult_GetObject(result, &object)))
        return hr;

    EnterCriticalSection(&clock->cs);
    LIST_FOR_EACH_ENTRY(timer, &clock->timers, struct clock_timer, entry)
    {
        if (&timer->IUnknown_iface == object)
        {
            list_remove(&timer->entry);
            IUnknown_Release(&timer->IUnknown_iface);
            break;
        }
    }
    LeaveCriticalSection(&clock->cs);

    timer = impl_clock_timer_from_IUnknown(object);
    IMFAsyncCallback_Invoke(timer->callback, timer->result);

    IUnknown_Release(object);

    return S_OK;
}

static const IMFAsyncCallbackVtbl presentclocktimercallbackvtbl =
{
    present_clock_callback_QueryInterface,
    present_clock_timer_callback_AddRef,
    present_clock_timer_callback_Release,
    present_clock_callback_GetParameters,
    present_clock_timer_callback_Invoke,
};

/***********************************************************************
 *      MFCreatePresentationClock (mf.@)
 */
HRESULT WINAPI MFCreatePresentationClock(IMFPresentationClock **clock)
{
    struct presentation_clock *object;

    TRACE("%p.\n", clock);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFPresentationClock_iface.lpVtbl = &presentationclockvtbl;
    object->IMFRateControl_iface.lpVtbl = &presentclockratecontrolvtbl;
    object->IMFTimer_iface.lpVtbl = &presentclocktimervtbl;
    object->IMFShutdown_iface.lpVtbl = &presentclockshutdownvtbl;
    object->sink_callback.lpVtbl = &presentclocksinkcallbackvtbl;
    object->timer_callback.lpVtbl = &presentclocktimercallbackvtbl;
    object->refcount = 1;
    list_init(&object->sinks);
    list_init(&object->timers);
    object->rate = 1.0f;
    InitializeCriticalSection(&object->cs);

    *clock = &object->IMFPresentationClock_iface;

    return S_OK;
}
