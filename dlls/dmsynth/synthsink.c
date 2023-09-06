/*
 * IDirectMusicSynthSink Implementation
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
 * Copyright (C) 2012 Christian Costa
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "dmsynth_private.h"
#include "initguid.h"
#include "uuids.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmsynth);

struct synth_sink
{
    IDirectMusicSynthSink IDirectMusicSynthSink_iface;
    IKsControl IKsControl_iface;
    IReferenceClock IReferenceClock_iface;
    LONG ref;

    IReferenceClock *master_clock;
    IDirectMusicSynth *synth;   /* No reference hold! */
    IDirectSound *dsound;

    BOOL active;
    REFERENCE_TIME activate_time;
    REFERENCE_TIME latency_time;
};

static inline struct synth_sink *impl_from_IDirectMusicSynthSink(IDirectMusicSynthSink *iface)
{
    return CONTAINING_RECORD(iface, struct synth_sink, IDirectMusicSynthSink_iface);
}

static void synth_sink_get_format(struct synth_sink *This, WAVEFORMATEX *format)
{
    DWORD size = sizeof(*format);
    HRESULT hr;

    format->wFormatTag = WAVE_FORMAT_PCM;
    format->nChannels = 2;
    format->wBitsPerSample = 16;
    format->nSamplesPerSec = 22050;
    format->nBlockAlign = format->nChannels * format->wBitsPerSample / 8;
    format->nAvgBytesPerSec = format->nSamplesPerSec * format->nBlockAlign;

    if (This->synth)
    {
        if (FAILED(hr = IDirectMusicSynth_GetFormat(This->synth, format, &size)))
            WARN("Failed to get synth buffer format, hr %#lx\n", hr);
    }
}

static HRESULT synth_sink_activate(struct synth_sink *This)
{
    HRESULT hr;

    if (!This->synth) return DMUS_E_SYNTHNOTCONFIGURED;
    if (!This->dsound) return DMUS_E_DSOUND_NOT_SET;
    if (!This->master_clock) return DMUS_E_NO_MASTER_CLOCK;
    if (This->active) return DMUS_E_SYNTHACTIVE;

    if (FAILED(hr = IReferenceClock_GetTime(This->master_clock, &This->activate_time))) return hr;
    This->latency_time = This->activate_time;

    This->active = TRUE;
    return S_OK;
}

static HRESULT synth_sink_deactivate(struct synth_sink *This)
{
    This->active = FALSE;
    return S_OK;
}

static HRESULT WINAPI synth_sink_QueryInterface(IDirectMusicSynthSink *iface,
        REFIID riid, void **ret_iface)
{
    struct synth_sink *This = impl_from_IDirectMusicSynthSink(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_dmguid(riid), ret_iface);

    if (IsEqualIID (riid, &IID_IUnknown) ||
        IsEqualIID (riid, &IID_IDirectMusicSynthSink))
    {
        IUnknown_AddRef(iface);
        *ret_iface = iface;
        return S_OK;
    }
    else if (IsEqualIID(riid, &IID_IKsControl))
    {
        IUnknown_AddRef(iface);
        *ret_iface = &This->IKsControl_iface;
        return S_OK;
    }

    *ret_iface = NULL;

    WARN("(%p)->(%s, %p): not found\n", iface, debugstr_dmguid(riid), ret_iface);

    return E_NOINTERFACE;
}

static ULONG WINAPI synth_sink_AddRef(IDirectMusicSynthSink *iface)
{
    struct synth_sink *This = impl_from_IDirectMusicSynthSink(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): new ref = %lu\n", This, ref);

    return ref;
}

static ULONG WINAPI synth_sink_Release(IDirectMusicSynthSink *iface)
{
    struct synth_sink *This = impl_from_IDirectMusicSynthSink(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): new ref = %lu\n", This, ref);

    if (!ref) {
        if (This->master_clock)
            IReferenceClock_Release(This->master_clock);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI synth_sink_Init(IDirectMusicSynthSink *iface,
        IDirectMusicSynth *synth)
{
    struct synth_sink *This = impl_from_IDirectMusicSynthSink(iface);

    TRACE("(%p)->(%p)\n", This, synth);

    /* Not holding a reference to avoid circular dependencies.
       The synth will release the sink during the synth's destruction. */
    This->synth = synth;

    return S_OK;
}

static HRESULT WINAPI synth_sink_SetMasterClock(IDirectMusicSynthSink *iface,
        IReferenceClock *clock)
{
    struct synth_sink *This = impl_from_IDirectMusicSynthSink(iface);

    TRACE("(%p)->(%p)\n", This, clock);

    if (!clock)
        return E_POINTER;

    if (This->master_clock) IReferenceClock_Release(This->master_clock);
    IReferenceClock_AddRef(clock);
    This->master_clock = clock;

    return S_OK;
}

static HRESULT WINAPI synth_sink_GetLatencyClock(IDirectMusicSynthSink *iface,
        IReferenceClock **clock)
{
    struct synth_sink *This = impl_from_IDirectMusicSynthSink(iface);

    TRACE("(%p)->(%p)\n", iface, clock);

    if (!clock)
        return E_POINTER;

    *clock = &This->IReferenceClock_iface;
    IReferenceClock_AddRef(*clock);

    return S_OK;
}

static HRESULT WINAPI synth_sink_Activate(IDirectMusicSynthSink *iface,
        BOOL enable)
{
    struct synth_sink *This = impl_from_IDirectMusicSynthSink(iface);

    FIXME("(%p)->(%d): semi-stub\n", This, enable);

    return enable ? synth_sink_activate(This) : synth_sink_deactivate(This);
}

static HRESULT WINAPI synth_sink_SampleToRefTime(IDirectMusicSynthSink *iface,
        LONGLONG sample_time, REFERENCE_TIME *ref_time)
{
    struct synth_sink *This = impl_from_IDirectMusicSynthSink(iface);
    WAVEFORMATEX format;

    TRACE("(%p)->(%I64d, %p)\n", This, sample_time, ref_time);

    if (!ref_time) return E_POINTER;

    synth_sink_get_format(This, &format);
    *ref_time = This->activate_time + ((sample_time * 10000) / format.nSamplesPerSec) * 1000;

    return S_OK;
}

static HRESULT WINAPI synth_sink_RefTimeToSample(IDirectMusicSynthSink *iface,
        REFERENCE_TIME ref_time, LONGLONG *sample_time)
{
    struct synth_sink *This = impl_from_IDirectMusicSynthSink(iface);
    WAVEFORMATEX format;

    TRACE("(%p)->(%I64d, %p)\n", This, ref_time, sample_time);

    if (!sample_time) return E_POINTER;

    synth_sink_get_format(This, &format);
    ref_time -= This->activate_time;
    *sample_time = ((ref_time / 1000) * format.nSamplesPerSec) / 10000;

    return S_OK;
}

static HRESULT WINAPI synth_sink_SetDirectSound(IDirectMusicSynthSink *iface,
        IDirectSound *dsound, IDirectSoundBuffer *dsound_buffer)
{
    struct synth_sink *This = impl_from_IDirectMusicSynthSink(iface);

    TRACE("(%p)->(%p, %p)\n", This, dsound, dsound_buffer);

    if (dsound_buffer) FIXME("Ignoring IDirectSoundBuffer parameter.\n");
    if (This->active) return DMUS_E_SYNTHACTIVE;

    if (This->dsound) IDirectSound_Release(This->dsound);
    This->dsound = NULL;
    if (!dsound) return S_OK;

    if (!This->synth) return DMUS_E_SYNTHNOTCONFIGURED;
    if ((This->dsound = dsound)) IDirectSound_AddRef(This->dsound);

    return S_OK;
}

static HRESULT WINAPI synth_sink_GetDesiredBufferSize(IDirectMusicSynthSink *iface,
        DWORD *size)
{
    struct synth_sink *This = impl_from_IDirectMusicSynthSink(iface);
    WAVEFORMATEX format;
    DWORD fmtsize = sizeof(format);

    TRACE("(%p, %p)\n", This, size);

    if (!size)
        return E_POINTER;
    if (!This->synth)
        return DMUS_E_SYNTHNOTCONFIGURED;

    if (FAILED(IDirectMusicSynth_GetFormat(This->synth, &format, &fmtsize)))
        return E_UNEXPECTED;
    *size = format.nSamplesPerSec * format.nChannels * 4;

    return S_OK;
}

static const IDirectMusicSynthSinkVtbl synth_sink_vtbl =
{
	synth_sink_QueryInterface,
	synth_sink_AddRef,
	synth_sink_Release,
	synth_sink_Init,
	synth_sink_SetMasterClock,
	synth_sink_GetLatencyClock,
	synth_sink_Activate,
	synth_sink_SampleToRefTime,
	synth_sink_RefTimeToSample,
	synth_sink_SetDirectSound,
	synth_sink_GetDesiredBufferSize,
};

static inline struct synth_sink *impl_from_IKsControl(IKsControl *iface)
{
    return CONTAINING_RECORD(iface, struct synth_sink, IKsControl_iface);
}

static HRESULT WINAPI synth_sink_control_QueryInterface(IKsControl* iface, REFIID riid, LPVOID *ppobj)
{
    struct synth_sink *This = impl_from_IKsControl(iface);

    return synth_sink_QueryInterface(&This->IDirectMusicSynthSink_iface, riid, ppobj);
}

static ULONG WINAPI synth_sink_control_AddRef(IKsControl* iface)
{
    struct synth_sink *This = impl_from_IKsControl(iface);

    return synth_sink_AddRef(&This->IDirectMusicSynthSink_iface);
}

static ULONG WINAPI synth_sink_control_Release(IKsControl* iface)
{
    struct synth_sink *This = impl_from_IKsControl(iface);

    return synth_sink_Release(&This->IDirectMusicSynthSink_iface);
}

static HRESULT WINAPI synth_sink_control_KsProperty(IKsControl* iface, PKSPROPERTY Property,
        ULONG PropertyLength, LPVOID PropertyData, ULONG DataLength, ULONG* BytesReturned)
{
    TRACE("(%p, %p, %lu, %p, %lu, %p)\n", iface, Property, PropertyLength, PropertyData, DataLength, BytesReturned);

    TRACE("Property = %s - %lu - %lu\n", debugstr_guid(&Property->Set), Property->Id, Property->Flags);

    if (Property->Flags != KSPROPERTY_TYPE_GET)
    {
        FIXME("Property flags %lu not yet supported\n", Property->Flags);
        return S_FALSE;
    }

    if (DataLength <  sizeof(DWORD))
        return E_NOT_SUFFICIENT_BUFFER;

    if (IsEqualGUID(&Property->Set, &GUID_DMUS_PROP_SinkUsesDSound))
    {
        *(DWORD*)PropertyData = TRUE;
        *BytesReturned = sizeof(DWORD);
    }
    else
    {
        FIXME("Unknown property %s\n", debugstr_guid(&Property->Set));
        *(DWORD*)PropertyData = FALSE;
        *BytesReturned = sizeof(DWORD);
    }

    return S_OK;
}

static HRESULT WINAPI synth_sink_control_KsMethod(IKsControl* iface, PKSMETHOD Method,
        ULONG MethodLength, LPVOID MethodData, ULONG DataLength, ULONG* BytesReturned)
{
    FIXME("(%p, %p, %lu, %p, %lu, %p): stub\n", iface, Method, MethodLength, MethodData, DataLength, BytesReturned);

    return E_NOTIMPL;
}

static HRESULT WINAPI synth_sink_control_KsEvent(IKsControl* iface, PKSEVENT Event,
        ULONG EventLength, LPVOID EventData, ULONG DataLength, ULONG* BytesReturned)
{
    FIXME("(%p, %p, %lu, %p, %lu, %p): stub\n", iface, Event, EventLength, EventData, DataLength, BytesReturned);

    return E_NOTIMPL;
}


static const IKsControlVtbl synth_sink_control =
{
    synth_sink_control_QueryInterface,
    synth_sink_control_AddRef,
    synth_sink_control_Release,
    synth_sink_control_KsProperty,
    synth_sink_control_KsMethod,
    synth_sink_control_KsEvent,
};

static inline struct synth_sink *impl_from_IReferenceClock(IReferenceClock *iface)
{
    return CONTAINING_RECORD(iface, struct synth_sink, IReferenceClock_iface);
}

static HRESULT WINAPI latency_clock_QueryInterface(IReferenceClock *iface, REFIID iid, void **out)
{
    TRACE("(%p, %s, %p)\n", iface, debugstr_dmguid(iid), out);

    if (IsEqualIID(iid, &IID_IUnknown)
            || IsEqualIID(iid, &IID_IReferenceClock))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    FIXME("no interface for %s\n", debugstr_dmguid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI latency_clock_AddRef(IReferenceClock *iface)
{
    struct synth_sink *This = impl_from_IReferenceClock(iface);
    return IDirectMusicSynthSink_AddRef(&This->IDirectMusicSynthSink_iface);
}

static ULONG WINAPI latency_clock_Release(IReferenceClock *iface)
{
    struct synth_sink *This = impl_from_IReferenceClock(iface);
    return IDirectMusicSynthSink_Release(&This->IDirectMusicSynthSink_iface);
}

static HRESULT WINAPI latency_clock_GetTime(IReferenceClock *iface, REFERENCE_TIME *time)
{
    struct synth_sink *This = impl_from_IReferenceClock(iface);

    TRACE("(%p, %p)\n", iface, time);

    if (!time) return E_INVALIDARG;
    if (!This->active) return E_FAIL;
    *time = This->latency_time;

    return S_OK;
}

static HRESULT WINAPI latency_clock_AdviseTime(IReferenceClock *iface, REFERENCE_TIME base,
        REFERENCE_TIME offset, HEVENT event, DWORD_PTR *cookie)
{
    FIXME("(%p, %I64d, %I64d, %#Ix, %p): stub\n", iface, base, offset, event, cookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI latency_clock_AdvisePeriodic(IReferenceClock *iface, REFERENCE_TIME start,
        REFERENCE_TIME period, HSEMAPHORE semaphore, DWORD_PTR *cookie)
{
    FIXME("(%p, %I64d, %I64d, %#Ix, %p): stub\n", iface, start, period, semaphore, cookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI latency_clock_Unadvise(IReferenceClock *iface, DWORD_PTR cookie)
{
    FIXME("(%p, %#Ix): stub\n", iface, cookie);
    return E_NOTIMPL;
}

static const IReferenceClockVtbl latency_clock_vtbl =
{
    latency_clock_QueryInterface,
    latency_clock_AddRef,
    latency_clock_Release,
    latency_clock_GetTime,
    latency_clock_AdviseTime,
    latency_clock_AdvisePeriodic,
    latency_clock_Unadvise,
};

HRESULT synth_sink_create(IUnknown **ret_iface)
{
    struct synth_sink *obj;

    TRACE("(%p)\n", ret_iface);

    *ret_iface = NULL;
    if (!(obj = calloc(1, sizeof(*obj)))) return E_OUTOFMEMORY;
    obj->IDirectMusicSynthSink_iface.lpVtbl = &synth_sink_vtbl;
    obj->IKsControl_iface.lpVtbl = &synth_sink_control;
    obj->IReferenceClock_iface.lpVtbl = &latency_clock_vtbl;
    obj->ref = 1;

    TRACE("Created DirectMusicSynthSink %p\n", obj);
    *ret_iface = (IUnknown *)&obj->IDirectMusicSynthSink_iface;
    return S_OK;
}
