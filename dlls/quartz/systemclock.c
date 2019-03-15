/*
 * Implementation of IReferenceClock
 *
 * Copyright 2004 Raphael Junqueira
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

#include "quartz_private.h"

#include "wine/debug.h"
#include "wine/unicode.h"
#include <assert.h>

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

struct advise_sink
{
    struct list entry;
    HANDLE hEvent;
    REFERENCE_TIME rtBaseTime, rtIntervalTime;
};

typedef struct SystemClockImpl {
    IReferenceClock IReferenceClock_iface;
    LONG ref;

    BOOL thread_created;
    HANDLE thread, notify_event, stop_event;
    REFERENCE_TIME last_time;
    CRITICAL_SECTION safe;

    /* These lists are ordered by expiration time (soonest first). */
    struct list single_sinks, periodic_sinks;
} SystemClockImpl;

static inline SystemClockImpl *impl_from_IReferenceClock(IReferenceClock *iface)
{
    return CONTAINING_RECORD(iface, SystemClockImpl, IReferenceClock_iface);
}

static void insert_advise_sink(struct advise_sink *sink, struct list *queue)
{
    REFERENCE_TIME due_time = sink->rtBaseTime + sink->rtIntervalTime;
    struct advise_sink *cursor;

    LIST_FOR_EACH_ENTRY(cursor, queue, struct advise_sink, entry)
    {
        if (cursor->rtBaseTime + cursor->rtIntervalTime > due_time)
        {
            list_add_before(&cursor->entry, &sink->entry);
            return;
        }
    }

    list_add_tail(queue, &sink->entry);
}

static DWORD WINAPI SystemClockAdviseThread(LPVOID lpParam) {
  SystemClockImpl* This = lpParam;
  struct advise_sink *sink, *cursor;
  struct list *entry;
  DWORD timeOut = INFINITE;
  REFERENCE_TIME curTime;
  HANDLE handles[2] = {This->stop_event, This->notify_event};

  TRACE("(%p): Main Loop\n", This);

  while (TRUE) {
    EnterCriticalSection(&This->safe);

    curTime = GetTickCount64() * 10000;

    /** First SingleShots Advice: sorted list */
    LIST_FOR_EACH_ENTRY_SAFE(sink, cursor, &This->single_sinks, struct advise_sink, entry)
    {
      if (sink->rtBaseTime + sink->rtIntervalTime > curTime)
        break;

      SetEvent(sink->hEvent);
      list_remove(&sink->entry);
      heap_free(sink);
    }

    if ((entry = list_head(&This->single_sinks)))
    {
      sink = LIST_ENTRY(entry, struct advise_sink, entry);
      timeOut = (sink->rtBaseTime + sink->rtIntervalTime - curTime) / 10000;
    }
    else timeOut = INFINITE;

    /** Now Periodics Advice: semi sorted list (sort cannot be used) */
    LIST_FOR_EACH_ENTRY(sink, &This->periodic_sinks, struct advise_sink, entry)
    {
      if (sink->rtBaseTime <= curTime)
      {
        DWORD periods = ((curTime - sink->rtBaseTime) / sink->rtIntervalTime) + 1;
        ReleaseSemaphore(sink->hEvent, periods, NULL);
        sink->rtBaseTime += periods * sink->rtIntervalTime;
      }
      timeOut = min(timeOut, (sink->rtBaseTime - curTime) / 10000);
    }

    LeaveCriticalSection(&This->safe);

    if (WaitForMultipleObjects(2, handles, FALSE, timeOut) == 0)
        return 0;
  }
}

static void notify_thread(SystemClockImpl *clock)
{
    if (!InterlockedCompareExchange(&clock->thread_created, TRUE, FALSE))
    {
        clock->thread = CreateThread(NULL, 0, SystemClockAdviseThread, clock, 0, NULL);
        clock->notify_event = CreateEventW(NULL, FALSE, FALSE, NULL);
        clock->stop_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    }
    SetEvent(clock->notify_event);
}

static ULONG WINAPI SystemClockImpl_AddRef(IReferenceClock* iface) {
  SystemClockImpl *This = impl_from_IReferenceClock(iface);
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("(%p): AddRef from %d\n", This, ref - 1);

  return ref;
}

static HRESULT WINAPI SystemClockImpl_QueryInterface(IReferenceClock* iface, REFIID riid, void** ppobj) {
  SystemClockImpl *This = impl_from_IReferenceClock(iface);
  TRACE("(%p, %s,%p)\n", This, debugstr_guid(riid), ppobj);
  
  if (IsEqualIID (riid, &IID_IUnknown) || 
      IsEqualIID (riid, &IID_IReferenceClock)) {
    SystemClockImpl_AddRef(iface);
    *ppobj = &This->IReferenceClock_iface;
    return S_OK;
  }
  
  *ppobj = NULL;
  WARN("(%p, %s,%p): not found\n", This, debugstr_guid(riid), ppobj);
  return E_NOINTERFACE;
}

static ULONG WINAPI SystemClockImpl_Release(IReferenceClock *iface)
{
    SystemClockImpl *clock = impl_from_IReferenceClock(iface);
    ULONG refcount = InterlockedDecrement(&clock->ref);

    TRACE("%p decreasing refcount to %u.\n", clock, refcount);

    if (!refcount)
    {
        if (clock->thread)
        {
            SetEvent(clock->stop_event);
            WaitForSingleObject(clock->thread, INFINITE);
            CloseHandle(clock->thread);
            CloseHandle(clock->notify_event);
            CloseHandle(clock->stop_event);
        }
        clock->safe.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&clock->safe);
        heap_free(clock);
    }
    return refcount;
}

static HRESULT WINAPI SystemClockImpl_GetTime(IReferenceClock *iface, REFERENCE_TIME *time)
{
    SystemClockImpl *clock = impl_from_IReferenceClock(iface);
    REFERENCE_TIME ret;
    HRESULT hr;

    TRACE("clock %p, time %p.\n", clock, time);

    if (!time) {
        return E_POINTER;
    }

    ret = GetTickCount64() * 10000;

    EnterCriticalSection(&clock->safe);

    hr = (ret == clock->last_time) ? S_FALSE : S_OK;
    *time = clock->last_time = ret;

    LeaveCriticalSection(&clock->safe);

    return hr;
}

static HRESULT WINAPI SystemClockImpl_AdviseTime(IReferenceClock *iface,
        REFERENCE_TIME base, REFERENCE_TIME offset, HEVENT event, DWORD_PTR *cookie)
{
    SystemClockImpl *clock = impl_from_IReferenceClock(iface);
    struct advise_sink *sink;

    TRACE("clock %p, base %s, offset %s, event %#lx, cookie %p.\n",
            clock, wine_dbgstr_longlong(base), wine_dbgstr_longlong(offset), event, cookie);

    if (!event)
        return E_INVALIDARG;

    if (base + offset <= 0)
        return E_INVALIDARG;

    if (!cookie)
        return E_POINTER;

    if (!(sink = heap_alloc_zero(sizeof(*sink))))
        return E_OUTOFMEMORY;

    sink->hEvent = (HANDLE)event;
    sink->rtBaseTime = base + offset;
    sink->rtIntervalTime = 0;

    EnterCriticalSection(&clock->safe);
    insert_advise_sink(sink, &clock->single_sinks);
    LeaveCriticalSection(&clock->safe);

    notify_thread(clock);

    *cookie = (DWORD_PTR)sink;
    return S_OK;
}

static HRESULT WINAPI SystemClockImpl_AdvisePeriodic(IReferenceClock* iface,
        REFERENCE_TIME start, REFERENCE_TIME period, HSEMAPHORE semaphore, DWORD_PTR *cookie)
{
    SystemClockImpl *clock = impl_from_IReferenceClock(iface);
    struct advise_sink *sink;

    TRACE("clock %p, start %s, period %s, semaphore %#lx, cookie %p.\n",
            clock, wine_dbgstr_longlong(start), wine_dbgstr_longlong(period), semaphore, cookie);

    if (!semaphore)
        return E_INVALIDARG;

    if (start <= 0 || period <= 0)
        return E_INVALIDARG;

    if (!cookie)
        return E_POINTER;

    if (!(sink = heap_alloc_zero(sizeof(*sink))))
        return E_OUTOFMEMORY;

    sink->hEvent = (HANDLE)semaphore;
    sink->rtBaseTime = start;
    sink->rtIntervalTime = period;

    EnterCriticalSection(&clock->safe);
    insert_advise_sink(sink, &clock->periodic_sinks);
    LeaveCriticalSection(&clock->safe);

    notify_thread(clock);

    *cookie = (DWORD_PTR)sink;
    return S_OK;
}

static HRESULT WINAPI SystemClockImpl_Unadvise(IReferenceClock *iface, DWORD_PTR cookie)
{
    SystemClockImpl *clock = impl_from_IReferenceClock(iface);
    struct advise_sink *sink;

    TRACE("clock %p, cookie %#lx.\n", clock, cookie);

    EnterCriticalSection(&clock->safe);

    LIST_FOR_EACH_ENTRY(sink, &clock->single_sinks, struct advise_sink, entry)
    {
        if (sink == (struct advise_sink *)cookie)
        {
            list_remove(&sink->entry);
            heap_free(sink);
            LeaveCriticalSection(&clock->safe);
            return S_OK;
        }
    }

    LIST_FOR_EACH_ENTRY(sink, &clock->periodic_sinks, struct advise_sink, entry)
    {
        if (sink == (struct advise_sink *)cookie)
        {
            list_remove(&sink->entry);
            heap_free(sink);
            LeaveCriticalSection(&clock->safe);
            return S_OK;
        }
    }

    LeaveCriticalSection(&clock->safe);

    return S_FALSE;
}

static const IReferenceClockVtbl SystemClock_Vtbl = 
{
    SystemClockImpl_QueryInterface,
    SystemClockImpl_AddRef,
    SystemClockImpl_Release,
    SystemClockImpl_GetTime,
    SystemClockImpl_AdviseTime,
    SystemClockImpl_AdvisePeriodic,
    SystemClockImpl_Unadvise
};

HRESULT QUARTZ_CreateSystemClock(IUnknown *outer, void **out)
{
    SystemClockImpl *object;
  
    TRACE("outer %p, out %p.\n", outer, out);

    if (!(object = heap_alloc_zero(sizeof(*object))))
    {
        *out = NULL;
        return E_OUTOFMEMORY;
    }

    object->IReferenceClock_iface.lpVtbl = &SystemClock_Vtbl;
    list_init(&object->single_sinks);
    list_init(&object->periodic_sinks);
    InitializeCriticalSection(&object->safe);
    object->safe.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": SystemClockImpl.safe");

    return SystemClockImpl_QueryInterface(&object->IReferenceClock_iface, &IID_IReferenceClock, out);
}
