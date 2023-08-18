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

typedef struct IDirectMusicSynthSinkImpl IDirectMusicSynthSinkImpl;

struct IDirectMusicSynthSinkImpl
{
    IDirectMusicSynthSink IDirectMusicSynthSink_iface;
    IKsControl IKsControl_iface;
    LONG ref;

    IReferenceClock *latency_clock;
    IReferenceClock *master_clock;
    IDirectMusicSynth *synth;   /* No reference hold! */
    BOOL active;
};

static inline IDirectMusicSynthSinkImpl *impl_from_IDirectMusicSynthSink(IDirectMusicSynthSink *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicSynthSinkImpl, IDirectMusicSynthSink_iface);
}

static HRESULT WINAPI synth_sink_QueryInterface(IDirectMusicSynthSink *iface,
        REFIID riid, void **ret_iface)
{
    IDirectMusicSynthSinkImpl *This = impl_from_IDirectMusicSynthSink(iface);

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
    IDirectMusicSynthSinkImpl *This = impl_from_IDirectMusicSynthSink(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): new ref = %lu\n", This, ref);

    return ref;
}

static ULONG WINAPI synth_sink_Release(IDirectMusicSynthSink *iface)
{
    IDirectMusicSynthSinkImpl *This = impl_from_IDirectMusicSynthSink(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): new ref = %lu\n", This, ref);

    if (!ref) {
        if (This->latency_clock)
            IReferenceClock_Release(This->latency_clock);
        if (This->master_clock)
            IReferenceClock_Release(This->master_clock);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI synth_sink_Init(IDirectMusicSynthSink *iface,
        IDirectMusicSynth *synth)
{
    IDirectMusicSynthSinkImpl *This = impl_from_IDirectMusicSynthSink(iface);

    TRACE("(%p)->(%p)\n", This, synth);

    /* Not holding a reference to avoid circular dependencies.
       The synth will release the sink during the synth's destruction. */
    This->synth = synth;

    return S_OK;
}

static HRESULT WINAPI synth_sink_SetMasterClock(IDirectMusicSynthSink *iface,
        IReferenceClock *clock)
{
    IDirectMusicSynthSinkImpl *This = impl_from_IDirectMusicSynthSink(iface);

    TRACE("(%p)->(%p)\n", This, clock);

    if (!clock)
        return E_POINTER;
    if (This->active)
        return E_FAIL;

    IReferenceClock_AddRef(clock);
    This->master_clock = clock;

    return S_OK;
}

static HRESULT WINAPI synth_sink_GetLatencyClock(IDirectMusicSynthSink *iface,
        IReferenceClock **clock)
{
    IDirectMusicSynthSinkImpl *This = impl_from_IDirectMusicSynthSink(iface);

    TRACE("(%p)->(%p)\n", iface, clock);

    if (!clock)
        return E_POINTER;

    *clock = This->latency_clock;
    IReferenceClock_AddRef(This->latency_clock);

    return S_OK;
}

static HRESULT WINAPI synth_sink_Activate(IDirectMusicSynthSink *iface,
        BOOL enable)
{
    IDirectMusicSynthSinkImpl *This = impl_from_IDirectMusicSynthSink(iface);

    FIXME("(%p)->(%d): stub\n", This, enable);

    return S_OK;
}

static HRESULT WINAPI synth_sink_SampleToRefTime(IDirectMusicSynthSink *iface,
        LONGLONG sample_time, REFERENCE_TIME *ref_time)
{
    IDirectMusicSynthSinkImpl *This = impl_from_IDirectMusicSynthSink(iface);

    FIXME("(%p)->(0x%s, %p): stub\n", This, wine_dbgstr_longlong(sample_time), ref_time);

    return S_OK;
}

static HRESULT WINAPI synth_sink_RefTimeToSample(IDirectMusicSynthSink *iface,
        REFERENCE_TIME ref_time, LONGLONG *sample_time)
{
    IDirectMusicSynthSinkImpl *This = impl_from_IDirectMusicSynthSink(iface);

    FIXME("(%p)->(0x%s, %p): stub\n", This, wine_dbgstr_longlong(ref_time), sample_time);

    return S_OK;
}

static HRESULT WINAPI synth_sink_SetDirectSound(IDirectMusicSynthSink *iface,
        IDirectSound *dsound, IDirectSoundBuffer *dsound_buffer)
{
    IDirectMusicSynthSinkImpl *This = impl_from_IDirectMusicSynthSink(iface);

    FIXME("(%p)->(%p, %p): stub\n", This, dsound, dsound_buffer);

    return S_OK;
}

static HRESULT WINAPI synth_sink_GetDesiredBufferSize(IDirectMusicSynthSink *iface,
        DWORD *size)
{
    IDirectMusicSynthSinkImpl *This = impl_from_IDirectMusicSynthSink(iface);
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

static inline IDirectMusicSynthSinkImpl *impl_from_IKsControl(IKsControl *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicSynthSinkImpl, IKsControl_iface);
}

static HRESULT WINAPI synth_sink_control_QueryInterface(IKsControl* iface, REFIID riid, LPVOID *ppobj)
{
    IDirectMusicSynthSinkImpl *This = impl_from_IKsControl(iface);

    return synth_sink_QueryInterface(&This->IDirectMusicSynthSink_iface, riid, ppobj);
}

static ULONG WINAPI synth_sink_control_AddRef(IKsControl* iface)
{
    IDirectMusicSynthSinkImpl *This = impl_from_IKsControl(iface);

    return synth_sink_AddRef(&This->IDirectMusicSynthSink_iface);
}

static ULONG WINAPI synth_sink_control_Release(IKsControl* iface)
{
    IDirectMusicSynthSinkImpl *This = impl_from_IKsControl(iface);

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

HRESULT DMUSIC_CreateDirectMusicSynthSinkImpl(REFIID riid, void **ret_iface)
{
    IDirectMusicSynthSinkImpl *obj;
    HRESULT hr;

    TRACE("(%s, %p)\n", debugstr_guid(riid), ret_iface);

    *ret_iface = NULL;

    obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicSynthSinkImpl));
    if (!obj)
        return E_OUTOFMEMORY;

    obj->IDirectMusicSynthSink_iface.lpVtbl = &synth_sink_vtbl;
    obj->IKsControl_iface.lpVtbl = &synth_sink_control;
    obj->ref = 1;

    hr = CoCreateInstance(&CLSID_SystemClock, NULL, CLSCTX_INPROC_SERVER, &IID_IReferenceClock, (LPVOID*)&obj->latency_clock);
    if (FAILED(hr))
    {
        HeapFree(GetProcessHeap(), 0, obj);
        return hr;
    }

    hr = IDirectMusicSynthSink_QueryInterface(&obj->IDirectMusicSynthSink_iface, riid, ret_iface);
    IDirectMusicSynthSink_Release(&obj->IDirectMusicSynthSink_iface);

    return hr;
}
