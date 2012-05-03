/*
 * IDirectMusicSynth8 Implementation
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
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

#define COBJMACROS

#include "objbase.h"
#include "initguid.h"
#include "dmksctrl.h"

#include "dmsynth_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmsynth);

static inline IDirectMusicSynth8Impl *impl_from_IDirectMusicSynth8(IDirectMusicSynth8 *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicSynth8Impl, IDirectMusicSynth8_iface);
}

/* IDirectMusicSynth8Impl IUnknown part: */
static HRESULT WINAPI IDirectMusicSynth8Impl_QueryInterface(LPDIRECTMUSICSYNTH8 iface, REFIID riid, LPVOID *ppobj)
{
	IDirectMusicSynth8Impl *This = impl_from_IDirectMusicSynth8(iface);

	TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);

	if (IsEqualIID (riid, &IID_IUnknown) || 
	    IsEqualIID (riid, &IID_IDirectMusicSynth) ||
	    IsEqualIID (riid, &IID_IDirectMusicSynth8)) {
		IUnknown_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
    else if (IsEqualIID(riid, &IID_IKsControl)) {
		IUnknown_AddRef(iface);
		*ppobj = &This->IKsControl_iface;
		return S_OK;
    }

	WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicSynth8Impl_AddRef(LPDIRECTMUSICSYNTH8 iface)
{
	IDirectMusicSynth8Impl *This = impl_from_IDirectMusicSynth8(iface);
	ULONG refCount = InterlockedIncrement(&This->ref);

	TRACE("(%p)->(ref before=%u)\n", This, refCount - 1);

	DMSYNTH_LockModule();

	return refCount;
}

static ULONG WINAPI IDirectMusicSynth8Impl_Release(LPDIRECTMUSICSYNTH8 iface)
{
	IDirectMusicSynth8Impl *This = impl_from_IDirectMusicSynth8(iface);
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(ref before=%u)\n", This, refCount + 1);

	if (!refCount) {
		if (This->pLatencyClock)
			IReferenceClock_Release(This->pLatencyClock);
		HeapFree(GetProcessHeap(), 0, This);
	}

	DMSYNTH_UnlockModule();
	
	return refCount;
}

/* IDirectMusicSynth8Impl IDirectMusicSynth part: */
static HRESULT WINAPI IDirectMusicSynth8Impl_Open(LPDIRECTMUSICSYNTH8 iface, LPDMUS_PORTPARAMS pPortParams)
{
    IDirectMusicSynth8Impl *This = impl_from_IDirectMusicSynth8(iface);

    FIXME("(%p)->(%p): stub\n", This, pPortParams);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_Close(LPDIRECTMUSICSYNTH8 iface)
{
    IDirectMusicSynth8Impl *This = impl_from_IDirectMusicSynth8(iface);

    FIXME("(%p)->(): stub\n", This);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_SetNumChannelGroups(LPDIRECTMUSICSYNTH8 iface, DWORD groups)
{
    IDirectMusicSynth8Impl *This = impl_from_IDirectMusicSynth8(iface);

    FIXME("(%p->(%d): stub\n", This, groups);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_Download(LPDIRECTMUSICSYNTH8 iface, LPHANDLE hDownload, LPVOID data, LPBOOL free)
{
    IDirectMusicSynth8Impl *This = impl_from_IDirectMusicSynth8(iface);

    FIXME("(%p)->(%p, %p, %p): stub\n", This, hDownload, data, free);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_Unload(LPDIRECTMUSICSYNTH8 iface, HANDLE hDownload, HRESULT (CALLBACK* lpFreeHandle)(HANDLE,HANDLE), HANDLE hUserData)
{
    IDirectMusicSynth8Impl *This = impl_from_IDirectMusicSynth8(iface);

    FIXME("(%p)->(%p, %p, %p): stub\n", This, hDownload, lpFreeHandle, hUserData);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_PlayBuffer(LPDIRECTMUSICSYNTH8 iface, REFERENCE_TIME rt, LPBYTE buffer, DWORD size)
{
    IDirectMusicSynth8Impl *This = impl_from_IDirectMusicSynth8(iface);

    FIXME("(%p)->(0x%s, %p, %u): stub\n", This, wine_dbgstr_longlong(rt), buffer, size);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_GetRunningStats(LPDIRECTMUSICSYNTH8 iface, LPDMUS_SYNTHSTATS stats)
{
    IDirectMusicSynth8Impl *This = impl_from_IDirectMusicSynth8(iface);

    FIXME("(%p)->(%p): stub\n", This, stats);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_GetPortCaps(LPDIRECTMUSICSYNTH8 iface, LPDMUS_PORTCAPS caps)
{
    IDirectMusicSynth8Impl *This = impl_from_IDirectMusicSynth8(iface);

    TRACE("(%p)->(%p)\n", This, caps);

    *caps = This->pCaps;

    return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_SetMasterClock(LPDIRECTMUSICSYNTH8 iface, IReferenceClock* clock)
{
    IDirectMusicSynth8Impl *This = impl_from_IDirectMusicSynth8(iface);

    FIXME("(%p)->(%p): stub\n", This, clock);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_GetLatencyClock(LPDIRECTMUSICSYNTH8 iface, IReferenceClock** clock)
{
    IDirectMusicSynth8Impl *This = impl_from_IDirectMusicSynth8(iface);

    TRACE("(%p)->(%p)\n", iface, clock);

    if (!clock)
        return E_POINTER;

    if (!This->pSynthSink)
        return DMUS_E_NOSYNTHSINK;

    *clock = This->pLatencyClock;
    IReferenceClock_AddRef(This->pLatencyClock);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_Activate(LPDIRECTMUSICSYNTH8 iface, BOOL enable)
{
    IDirectMusicSynth8Impl *This = impl_from_IDirectMusicSynth8(iface);

    TRACE("(%p)->(%d)\n", This, enable);

    This->fActive = enable;

    return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_SetSynthSink(LPDIRECTMUSICSYNTH8 iface, IDirectMusicSynthSink* synth_sink)
{
    IDirectMusicSynth8Impl *This = impl_from_IDirectMusicSynth8(iface);

    TRACE("(%p)->(%p)\n", iface, synth_sink);

    This->pSynthSink = (IDirectMusicSynthSinkImpl*)synth_sink;

    if (synth_sink)
        return IDirectMusicSynthSink_GetLatencyClock(synth_sink, &This->pLatencyClock);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_Render(LPDIRECTMUSICSYNTH8 iface, short* buffer, DWORD length, LONGLONG position)
{
    IDirectMusicSynth8Impl *This = impl_from_IDirectMusicSynth8(iface);

    FIXME("(%p)->(%p, %d, 0x%s): stub\n", This, buffer, length, wine_dbgstr_longlong(position));

    return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_SetChannelPriority(LPDIRECTMUSICSYNTH8 iface, DWORD channel_group, DWORD channel, DWORD priority)
{
    /* IDirectMusicSynth8Impl *This = impl_from_IDirectMusicSynth8(iface); */

    /* Silenced because of too many messages - 1000 groups * 16 channels ;=) */
    /* FIXME("(%p)->(%ld, %ld, %ld): stub\n", This, channel_group, channel, priority); */

    return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_GetChannelPriority(LPDIRECTMUSICSYNTH8 iface, DWORD channel_group, DWORD channel, LPDWORD priority)
{
    IDirectMusicSynth8Impl *This = impl_from_IDirectMusicSynth8(iface);

    FIXME("(%p)->(%d, %d, %p): stub\n", This, channel_group, channel, priority);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_GetFormat(LPDIRECTMUSICSYNTH8 iface, LPWAVEFORMATEX wave_format, LPDWORD wave_format_size)
{
    IDirectMusicSynth8Impl *This = impl_from_IDirectMusicSynth8(iface);

    FIXME("(%p)->(%p, %p): stub\n", This, wave_format, wave_format_size);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_GetAppend(LPDIRECTMUSICSYNTH8 iface, DWORD* append)
{
    IDirectMusicSynth8Impl *This = impl_from_IDirectMusicSynth8(iface);

    FIXME("(%p)->(%p): stub\n", This, append);

    return S_OK;
}

/* IDirectMusicSynth8Impl IDirectMusicSynth8 part: */
static HRESULT WINAPI IDirectMusicSynth8Impl_PlayVoice(LPDIRECTMUSICSYNTH8 iface, REFERENCE_TIME ref_time, DWORD voice_id, DWORD channel_group, DWORD channel,
                                                       DWORD dwDLId, LONG prPitch, LONG vrVolume, SAMPLE_TIME stVoiceStart, SAMPLE_TIME stLoopStart, SAMPLE_TIME stLoopEnd)
{
    IDirectMusicSynth8Impl *This = impl_from_IDirectMusicSynth8(iface);

    FIXME("(%p)->(0x%s, %d, %d, %d, %d, %i, %i,0x%s, 0x%s, 0x%s): stub\n",
          This, wine_dbgstr_longlong(ref_time), voice_id, channel_group, channel, dwDLId, prPitch, vrVolume,
          wine_dbgstr_longlong(stVoiceStart), wine_dbgstr_longlong(stLoopStart), wine_dbgstr_longlong(stLoopEnd));

    return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_StopVoice(LPDIRECTMUSICSYNTH8 iface, REFERENCE_TIME ref_time, DWORD voice_id)
{
    IDirectMusicSynth8Impl *This = impl_from_IDirectMusicSynth8(iface);

    FIXME("(%p)->(0x%s, %d): stub\n", This, wine_dbgstr_longlong(ref_time), voice_id);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_GetVoiceState(LPDIRECTMUSICSYNTH8 iface, DWORD dwVoice[], DWORD cbVoice, DMUS_VOICE_STATE dwVoiceState[])
{
    IDirectMusicSynth8Impl *This = impl_from_IDirectMusicSynth8(iface);

    FIXME("(%p)->(%p, %d, %p): stub\n", This, dwVoice, cbVoice, dwVoiceState);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_Refresh(LPDIRECTMUSICSYNTH8 iface, DWORD download_id, DWORD flags)
{
    IDirectMusicSynth8Impl *This = impl_from_IDirectMusicSynth8(iface);

    FIXME("(%p)->(%d, %d): stub\n", This, download_id, flags);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_AssignChannelToBuses(LPDIRECTMUSICSYNTH8 iface, DWORD channel_group, DWORD channel, LPDWORD pdwBuses, DWORD cBuses)
{
    IDirectMusicSynth8Impl *This = impl_from_IDirectMusicSynth8(iface);

    FIXME("(%p)->(%d, %d, %p, %d): stub\n", This, channel_group, channel, pdwBuses, cBuses);

    return S_OK;
}

static const IDirectMusicSynth8Vtbl DirectMusicSynth8_Vtbl = {
	IDirectMusicSynth8Impl_QueryInterface,
	IDirectMusicSynth8Impl_AddRef,
	IDirectMusicSynth8Impl_Release,
	IDirectMusicSynth8Impl_Open,
	IDirectMusicSynth8Impl_Close,
	IDirectMusicSynth8Impl_SetNumChannelGroups,
	IDirectMusicSynth8Impl_Download,
	IDirectMusicSynth8Impl_Unload,
	IDirectMusicSynth8Impl_PlayBuffer,
	IDirectMusicSynth8Impl_GetRunningStats,
	IDirectMusicSynth8Impl_GetPortCaps,
	IDirectMusicSynth8Impl_SetMasterClock,
	IDirectMusicSynth8Impl_GetLatencyClock,
	IDirectMusicSynth8Impl_Activate,
	IDirectMusicSynth8Impl_SetSynthSink,
	IDirectMusicSynth8Impl_Render,
	IDirectMusicSynth8Impl_SetChannelPriority,
	IDirectMusicSynth8Impl_GetChannelPriority,
	IDirectMusicSynth8Impl_GetFormat,
	IDirectMusicSynth8Impl_GetAppend,
	IDirectMusicSynth8Impl_PlayVoice,
	IDirectMusicSynth8Impl_StopVoice,
	IDirectMusicSynth8Impl_GetVoiceState,
	IDirectMusicSynth8Impl_Refresh,
	IDirectMusicSynth8Impl_AssignChannelToBuses
};

static inline IDirectMusicSynth8Impl *impl_from_IKsControl(IKsControl *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicSynth8Impl, IKsControl_iface);
}

static HRESULT WINAPI DMSynthImpl_IKsControl_QueryInterface(IKsControl* iface, REFIID riid, LPVOID *ppobj)
{
    IDirectMusicSynth8Impl *This = impl_from_IKsControl(iface);

    return IDirectMusicSynth8Impl_QueryInterface(&This->IDirectMusicSynth8_iface, riid, ppobj);
}

static ULONG WINAPI DMSynthImpl_IKsControl_AddRef(IKsControl* iface)
{
    IDirectMusicSynth8Impl *This = impl_from_IKsControl(iface);

    return IDirectMusicSynth8Impl_AddRef(&This->IDirectMusicSynth8_iface);
}

static ULONG WINAPI DMSynthImpl_IKsControl_Release(IKsControl* iface)
{
    IDirectMusicSynth8Impl *This = impl_from_IKsControl(iface);

    return IDirectMusicSynth8Impl_Release(&This->IDirectMusicSynth8_iface);
}

static HRESULT WINAPI DMSynthImpl_IKsControl_KsProperty(IKsControl* iface, PKSPROPERTY Property, ULONG PropertyLength, LPVOID PropertyData,
                                                        ULONG DataLength, ULONG* BytesReturned)
{
    FIXME("(%p)->(%p, %u, %p, %u, %p): stub\n", iface, Property, PropertyLength, PropertyData, DataLength, BytesReturned);

    return E_NOTIMPL;
}

static HRESULT WINAPI DMSynthImpl_IKsControl_KsMethod(IKsControl* iface, PKSMETHOD Method, ULONG MethodLength, LPVOID MethodData,
                                                      ULONG DataLength, ULONG* BytesReturned)
{
    FIXME("(%p)->(%p, %u, %p, %u, %p): stub\n", iface, Method, MethodLength, MethodData, DataLength, BytesReturned);

    return E_NOTIMPL;
}

static HRESULT WINAPI DMSynthImpl_IKsControl_KsEvent(IKsControl* iface, PKSEVENT Event, ULONG EventLength, LPVOID EventData,
                                                     ULONG DataLength, ULONG* BytesReturned)
{
    FIXME("(%p)->(%p, %u, %p, %u, %p): stub\n", iface, Event, EventLength, EventData, DataLength, BytesReturned);

    return E_NOTIMPL;
}


static const IKsControlVtbl DMSynthImpl_IKsControl_Vtbl = {
    DMSynthImpl_IKsControl_QueryInterface,
    DMSynthImpl_IKsControl_AddRef,
    DMSynthImpl_IKsControl_Release,
    DMSynthImpl_IKsControl_KsProperty,
    DMSynthImpl_IKsControl_KsMethod,
    DMSynthImpl_IKsControl_KsEvent
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicSynthImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter)
{
	IDirectMusicSynth8Impl *obj;
	
	TRACE("(%p,%p,%p)\n", lpcGUID, ppobj, pUnkOuter);
	obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicSynth8Impl));
	if (NULL == obj) {
		*ppobj = NULL;
		return E_OUTOFMEMORY;
	}
	obj->IDirectMusicSynth8_iface.lpVtbl = &DirectMusicSynth8_Vtbl;
	obj->IKsControl_iface.lpVtbl = &DMSynthImpl_IKsControl_Vtbl;
	obj->ref = 0;
	/* fill in caps */
	obj->pCaps.dwSize = sizeof(DMUS_PORTCAPS);
	obj->pCaps.dwFlags = DMUS_PC_DLS | DMUS_PC_SOFTWARESYNTH | DMUS_PC_DIRECTSOUND | DMUS_PC_DLS2 | DMUS_PC_AUDIOPATH | DMUS_PC_WAVE;
	obj->pCaps.guidPort = CLSID_DirectMusicSynth;
	obj->pCaps.dwClass = DMUS_PC_OUTPUTCLASS;
	obj->pCaps.dwType = DMUS_PORT_USER_MODE_SYNTH;
	obj->pCaps.dwMemorySize = DMUS_PC_SYSTEMMEMORY;
	obj->pCaps.dwMaxChannelGroups = 1000;
	obj->pCaps.dwMaxVoices = 1000;
	obj->pCaps.dwMaxAudioChannels = 2;
	obj->pCaps.dwEffectFlags = DMUS_EFFECT_REVERB;
	MultiByteToWideChar (CP_ACP, 0, "Microsoft Synthesizer", -1, obj->pCaps.wszDescription, sizeof(obj->pCaps.wszDescription)/sizeof(WCHAR));

	return IDirectMusicSynth8Impl_QueryInterface ((LPDIRECTMUSICSYNTH8)obj, lpcGUID, ppobj);
}
