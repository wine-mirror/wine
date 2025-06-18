/*
 * Copyright 2022 Nikolay Sivov for CodeWeavers
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "mf_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

enum quality_manager_state
{
    QUALITY_MANAGER_READY = 0,
    QUALITY_MANAGER_SHUT_DOWN,
};

struct quality_manager
{
    IMFQualityManager IMFQualityManager_iface;
    IMFClockStateSink IMFClockStateSink_iface;
    LONG refcount;

    IMFTopology *topology;
    IMFPresentationClock *clock;
    unsigned int state;
    CRITICAL_SECTION cs;
};

static struct quality_manager *impl_from_IMFQualityManager(IMFQualityManager *iface)
{
    return CONTAINING_RECORD(iface, struct quality_manager, IMFQualityManager_iface);
}

static struct quality_manager *impl_from_IMFClockStateSink(IMFClockStateSink *iface)
{
    return CONTAINING_RECORD(iface, struct quality_manager, IMFClockStateSink_iface);
}

static HRESULT WINAPI standard_quality_manager_QueryInterface(IMFQualityManager *iface, REFIID riid, void **out)
{
    struct quality_manager *manager = impl_from_IMFQualityManager(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFQualityManager) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
    }
    else if (IsEqualIID(riid, &IID_IMFClockStateSink))
    {
        *out = &manager->IMFClockStateSink_iface;
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

static ULONG WINAPI standard_quality_manager_AddRef(IMFQualityManager *iface)
{
    struct quality_manager *manager = impl_from_IMFQualityManager(iface);
    ULONG refcount = InterlockedIncrement(&manager->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI standard_quality_manager_Release(IMFQualityManager *iface)
{
    struct quality_manager *manager = impl_from_IMFQualityManager(iface);
    ULONG refcount = InterlockedDecrement(&manager->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (manager->clock)
            IMFPresentationClock_Release(manager->clock);
        if (manager->topology)
            IMFTopology_Release(manager->topology);
        DeleteCriticalSection(&manager->cs);
        free(manager);
    }

    return refcount;
}

static void standard_quality_manager_set_topology(struct quality_manager *manager, IMFTopology *topology)
{
    if (manager->topology)
        IMFTopology_Release(manager->topology);
    manager->topology = topology;
    if (manager->topology)
        IMFTopology_AddRef(manager->topology);
}

static HRESULT WINAPI standard_quality_manager_NotifyTopology(IMFQualityManager *iface, IMFTopology *topology)
{
    struct quality_manager *manager = impl_from_IMFQualityManager(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, topology);

    EnterCriticalSection(&manager->cs);
    if (manager->state == QUALITY_MANAGER_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
    {
        standard_quality_manager_set_topology(manager, topology);
    }
    LeaveCriticalSection(&manager->cs);

    return hr;
}

static void standard_quality_manager_release_clock(struct quality_manager *manager)
{
    if (manager->clock)
    {
        IMFPresentationClock_RemoveClockStateSink(manager->clock, &manager->IMFClockStateSink_iface);
        IMFPresentationClock_Release(manager->clock);
    }
    manager->clock = NULL;
}

static HRESULT WINAPI standard_quality_manager_NotifyPresentationClock(IMFQualityManager *iface,
        IMFPresentationClock *clock)
{
    struct quality_manager *manager = impl_from_IMFQualityManager(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, clock);

    EnterCriticalSection(&manager->cs);
    if (manager->state == QUALITY_MANAGER_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (!clock)
        hr = E_POINTER;
    else
    {
        standard_quality_manager_release_clock(manager);
        manager->clock = clock;
        IMFPresentationClock_AddRef(manager->clock);
        if (FAILED(IMFPresentationClock_AddClockStateSink(manager->clock, &manager->IMFClockStateSink_iface)))
            WARN("Failed to set state sink.\n");
    }
    LeaveCriticalSection(&manager->cs);

    return hr;
}

static HRESULT WINAPI standard_quality_manager_NotifyProcessInput(IMFQualityManager *iface, IMFTopologyNode *node,
        LONG input_index, IMFSample *sample)
{
    TRACE("%p, %p, %ld, %p stub.\n", iface, node, input_index, sample);

    return E_NOTIMPL;
}

static HRESULT WINAPI standard_quality_manager_NotifyProcessOutput(IMFQualityManager *iface, IMFTopologyNode *node,
        LONG output_index, IMFSample *sample)
{
    TRACE("%p, %p, %ld, %p stub.\n", iface, node, output_index, sample);

    return E_NOTIMPL;
}

static HRESULT WINAPI standard_quality_manager_NotifyQualityEvent(IMFQualityManager *iface, IUnknown *object,
        IMFMediaEvent *event)
{
    FIXME("%p, %p, %p stub.\n", iface, object, event);

    return E_NOTIMPL;
}

static HRESULT WINAPI standard_quality_manager_Shutdown(IMFQualityManager *iface)
{
    struct quality_manager *manager = impl_from_IMFQualityManager(iface);

    TRACE("%p.\n", iface);

    EnterCriticalSection(&manager->cs);
    if (manager->state != QUALITY_MANAGER_SHUT_DOWN)
    {
        standard_quality_manager_release_clock(manager);
        standard_quality_manager_set_topology(manager, NULL);
        manager->state = QUALITY_MANAGER_SHUT_DOWN;
    }
    LeaveCriticalSection(&manager->cs);

    return S_OK;
}

static const IMFQualityManagerVtbl standard_quality_manager_vtbl =
{
    standard_quality_manager_QueryInterface,
    standard_quality_manager_AddRef,
    standard_quality_manager_Release,
    standard_quality_manager_NotifyTopology,
    standard_quality_manager_NotifyPresentationClock,
    standard_quality_manager_NotifyProcessInput,
    standard_quality_manager_NotifyProcessOutput,
    standard_quality_manager_NotifyQualityEvent,
    standard_quality_manager_Shutdown,
};

static HRESULT WINAPI standard_quality_manager_sink_QueryInterface(IMFClockStateSink *iface,
        REFIID riid, void **obj)
{
    struct quality_manager *manager = impl_from_IMFClockStateSink(iface);
    return IMFQualityManager_QueryInterface(&manager->IMFQualityManager_iface, riid, obj);
}

static ULONG WINAPI standard_quality_manager_sink_AddRef(IMFClockStateSink *iface)
{
    struct quality_manager *manager = impl_from_IMFClockStateSink(iface);
    return IMFQualityManager_AddRef(&manager->IMFQualityManager_iface);
}

static ULONG WINAPI standard_quality_manager_sink_Release(IMFClockStateSink *iface)
{
    struct quality_manager *manager = impl_from_IMFClockStateSink(iface);
    return IMFQualityManager_Release(&manager->IMFQualityManager_iface);
}

static HRESULT WINAPI standard_quality_manager_sink_OnClockStart(IMFClockStateSink *iface,
        MFTIME systime, LONGLONG offset)
{
    return S_OK;
}

static HRESULT WINAPI standard_quality_manager_sink_OnClockStop(IMFClockStateSink *iface,
        MFTIME systime)
{
    return S_OK;
}

static HRESULT WINAPI standard_quality_manager_sink_OnClockPause(IMFClockStateSink *iface,
        MFTIME systime)
{
    return S_OK;
}

static HRESULT WINAPI standard_quality_manager_sink_OnClockRestart(IMFClockStateSink *iface,
        MFTIME systime)
{
    return S_OK;
}

static HRESULT WINAPI standard_quality_manager_sink_OnClockSetRate(IMFClockStateSink *iface,
        MFTIME systime, float rate)
{
    return S_OK;
}

static const IMFClockStateSinkVtbl standard_quality_manager_sink_vtbl =
{
    standard_quality_manager_sink_QueryInterface,
    standard_quality_manager_sink_AddRef,
    standard_quality_manager_sink_Release,
    standard_quality_manager_sink_OnClockStart,
    standard_quality_manager_sink_OnClockStop,
    standard_quality_manager_sink_OnClockPause,
    standard_quality_manager_sink_OnClockRestart,
    standard_quality_manager_sink_OnClockSetRate,
};

HRESULT WINAPI MFCreateStandardQualityManager(IMFQualityManager **manager)
{
    struct quality_manager *object;

    TRACE("%p.\n", manager);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFQualityManager_iface.lpVtbl = &standard_quality_manager_vtbl;
    object->IMFClockStateSink_iface.lpVtbl = &standard_quality_manager_sink_vtbl;
    object->refcount = 1;
    InitializeCriticalSection(&object->cs);

    *manager = &object->IMFQualityManager_iface;

    return S_OK;
}
