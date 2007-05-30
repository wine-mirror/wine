/* IDirectMusicSynth8 Implementation
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

#include "dmsynth_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmsynth);

/* IDirectMusicSynth8Impl IUnknown part: */
static HRESULT WINAPI IDirectMusicSynth8Impl_QueryInterface (LPDIRECTMUSICSYNTH8 iface, REFIID riid, LPVOID *ppobj) {
	IDirectMusicSynth8Impl *This = (IDirectMusicSynth8Impl *)iface;
	TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);

	if (IsEqualIID (riid, &IID_IUnknown) || 
	    IsEqualIID (riid, &IID_IDirectMusicSynth) ||
	    IsEqualIID (riid, &IID_IDirectMusicSynth8)) {
		IUnknown_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicSynth8Impl_AddRef (LPDIRECTMUSICSYNTH8 iface) {
	IDirectMusicSynth8Impl *This = (IDirectMusicSynth8Impl *)iface;
	ULONG refCount = InterlockedIncrement(&This->ref);

	TRACE("(%p)->(ref before=%u)\n", This, refCount - 1);

	DMSYNTH_LockModule();

	return refCount;
}

static ULONG WINAPI IDirectMusicSynth8Impl_Release (LPDIRECTMUSICSYNTH8 iface) {
	IDirectMusicSynth8Impl *This = (IDirectMusicSynth8Impl *)iface;
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(ref before=%u)\n", This, refCount + 1);

	if (!refCount) {
		HeapFree(GetProcessHeap(), 0, This);
	}

	DMSYNTH_UnlockModule();
	
	return refCount;
}

/* IDirectMusicSynth8Impl IDirectMusicSynth part: */
static HRESULT WINAPI IDirectMusicSynth8Impl_Open (LPDIRECTMUSICSYNTH8 iface, LPDMUS_PORTPARAMS pPortParams) {
	IDirectMusicSynth8Impl *This = (IDirectMusicSynth8Impl *)iface;
	FIXME("(%p, %p): stub\n", This, pPortParams);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_Close (LPDIRECTMUSICSYNTH8 iface) {
	IDirectMusicSynth8Impl *This = (IDirectMusicSynth8Impl *)iface;
	FIXME("(%p): stub\n", This);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_SetNumChannelGroups (LPDIRECTMUSICSYNTH8 iface, DWORD dwGroups) {
	IDirectMusicSynth8Impl *This = (IDirectMusicSynth8Impl *)iface;
	FIXME("(%p, %d): stub\n", This, dwGroups);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_Download (LPDIRECTMUSICSYNTH8 iface, LPHANDLE phDownload, LPVOID pvData, LPBOOL pbFree) {
	IDirectMusicSynth8Impl *This = (IDirectMusicSynth8Impl *)iface;
	FIXME("(%p, %p, %p, %p): stub\n", This, phDownload, pvData, pbFree);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_Unload (LPDIRECTMUSICSYNTH8 iface, HANDLE hDownload, HRESULT (CALLBACK* lpFreeHandle)(HANDLE,HANDLE), HANDLE hUserData) {
	IDirectMusicSynth8Impl *This = (IDirectMusicSynth8Impl *)iface;
	FIXME("(%p, %p, %p): stub\n", This, hDownload, hUserData);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_PlayBuffer (LPDIRECTMUSICSYNTH8 iface, REFERENCE_TIME rt, LPBYTE pbBuffer, DWORD cbBuffer) {
	IDirectMusicSynth8Impl *This = (IDirectMusicSynth8Impl *)iface;
	FIXME("(%p, 0x%s, %p, %d): stub\n", This, wine_dbgstr_longlong(rt), pbBuffer, cbBuffer);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_GetRunningStats (LPDIRECTMUSICSYNTH8 iface, LPDMUS_SYNTHSTATS pStats) {
	IDirectMusicSynth8Impl *This = (IDirectMusicSynth8Impl *)iface;
	FIXME("(%p, %p): stub\n", This, pStats);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_GetPortCaps (LPDIRECTMUSICSYNTH8 iface, LPDMUS_PORTCAPS pCaps) {
	IDirectMusicSynth8Impl *This = (IDirectMusicSynth8Impl *)iface;
	TRACE("(%p, %p)\n", This, pCaps);
	*pCaps = This->pCaps;
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_SetMasterClock (LPDIRECTMUSICSYNTH8 iface, IReferenceClock* pClock) {
	IDirectMusicSynth8Impl *This = (IDirectMusicSynth8Impl *)iface;
	FIXME("(%p, %p): stub\n", This, pClock);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_GetLatencyClock (LPDIRECTMUSICSYNTH8 iface, IReferenceClock** ppClock) {
	IDirectMusicSynth8Impl *This = (IDirectMusicSynth8Impl *)iface;
	TRACE("(%p, %p)\n", This, ppClock);
	*ppClock = This->pLatencyClock;
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_Activate (LPDIRECTMUSICSYNTH8 iface, BOOL fEnable) {
	IDirectMusicSynth8Impl *This = (IDirectMusicSynth8Impl *)iface;
	TRACE("(%p, %d)\n", This, fEnable);
	This->fActive = fEnable;
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_SetSynthSink (LPDIRECTMUSICSYNTH8 iface, IDirectMusicSynthSink* pSynthSink) {
	IDirectMusicSynth8Impl *This = (IDirectMusicSynth8Impl *)iface;
	TRACE("(%p, %p)\n", This, pSynthSink);
	This->pSynthSink = (IDirectMusicSynthSinkImpl*)pSynthSink;
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_Render (LPDIRECTMUSICSYNTH8 iface, short* pBuffer, DWORD dwLength, LONGLONG llPosition) {
	IDirectMusicSynth8Impl *This = (IDirectMusicSynth8Impl *)iface;
	FIXME("(%p, %p, %d, 0x%s): stub\n", This, pBuffer, dwLength, wine_dbgstr_longlong(llPosition));
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_SetChannelPriority (LPDIRECTMUSICSYNTH8 iface, DWORD dwChannelGroup, DWORD dwChannel, DWORD dwPriority) {
	/*IDirectMusicSynth8Impl *This = (IDirectMusicSynth8Impl *)iface; */
	/* silenced because of too many messages - 1000 groups * 16 channels ;=) */
	/*FIXME("(%p, %ld, %ld, %ld): stub\n", This, dwChannelGroup, dwChannel, dwPriority); */
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_GetChannelPriority (LPDIRECTMUSICSYNTH8 iface, DWORD dwChannelGroup, DWORD dwChannel, LPDWORD pdwPriority) {
	IDirectMusicSynth8Impl *This = (IDirectMusicSynth8Impl *)iface;
	FIXME("(%p, %d, %d, %p): stub\n", This, dwChannelGroup, dwChannel, pdwPriority);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_GetFormat (LPDIRECTMUSICSYNTH8 iface, LPWAVEFORMATEX pWaveFormatEx, LPDWORD pdwWaveFormatExSiz) {
	IDirectMusicSynth8Impl *This = (IDirectMusicSynth8Impl *)iface;
	FIXME("(%p, %p, %p): stub\n", This, pWaveFormatEx, pdwWaveFormatExSiz);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_GetAppend (LPDIRECTMUSICSYNTH8 iface, DWORD* pdwAppend) {
	IDirectMusicSynth8Impl *This = (IDirectMusicSynth8Impl *)iface;
	FIXME("(%p, %p): stub\n", This, pdwAppend);
	return S_OK;
}

/* IDirectMusicSynth8Impl IDirectMusicSynth8 part: */
static HRESULT WINAPI IDirectMusicSynth8Impl_PlayVoice (LPDIRECTMUSICSYNTH8 iface, REFERENCE_TIME rt, DWORD dwVoiceId, DWORD dwChannelGroup, DWORD dwChannel, DWORD dwDLId, long prPitch, long vrVolume, SAMPLE_TIME stVoiceStart, SAMPLE_TIME stLoopStart, SAMPLE_TIME stLoopEnd) {
	IDirectMusicSynth8Impl *This = (IDirectMusicSynth8Impl *)iface;
	FIXME("(%p, 0x%s, %d, %d, %d, %d, %li, %li,0x%s, 0x%s, 0x%s): stub\n",
	    This, wine_dbgstr_longlong(rt), dwVoiceId, dwChannelGroup, dwChannel, dwDLId, prPitch, vrVolume,
	    wine_dbgstr_longlong(stVoiceStart), wine_dbgstr_longlong(stLoopStart), wine_dbgstr_longlong(stLoopEnd));
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_StopVoice (LPDIRECTMUSICSYNTH8 iface, REFERENCE_TIME rt, DWORD dwVoiceId) {
	IDirectMusicSynth8Impl *This = (IDirectMusicSynth8Impl *)iface;
	FIXME("(%p, 0x%s, %d): stub\n", This, wine_dbgstr_longlong(rt), dwVoiceId);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_GetVoiceState (LPDIRECTMUSICSYNTH8 iface, DWORD dwVoice[], DWORD cbVoice, DMUS_VOICE_STATE dwVoiceState[]) {
	IDirectMusicSynth8Impl *This = (IDirectMusicSynth8Impl *)iface;
	FIXME("(%p, %p, %d, %p): stub\n", This, dwVoice, cbVoice, dwVoiceState);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_Refresh (LPDIRECTMUSICSYNTH8 iface, DWORD dwDownloadID, DWORD dwFlags) {
	IDirectMusicSynth8Impl *This = (IDirectMusicSynth8Impl *)iface;
	FIXME("(%p, %d, %d): stub\n", This, dwDownloadID, dwFlags);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSynth8Impl_AssignChannelToBuses (LPDIRECTMUSICSYNTH8 iface, DWORD dwChannelGroup, DWORD dwChannel, LPDWORD pdwBuses, DWORD cBuses) {
	IDirectMusicSynth8Impl *This = (IDirectMusicSynth8Impl *)iface;
	FIXME("(%p, %d, %d, %p, %d): stub\n", This, dwChannelGroup, dwChannel, pdwBuses, cBuses);
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

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicSynthImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter) {
	IDirectMusicSynth8Impl *obj;
	
	TRACE("(%p,%p,%p)\n", lpcGUID, ppobj, pUnkOuter);
	obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicSynth8Impl));
	if (NULL == obj) {
		*ppobj = (LPDIRECTMUSICSYNTH8) NULL;
		return E_OUTOFMEMORY;
	}
	obj->lpVtbl = &DirectMusicSynth8_Vtbl;
	obj->ref = 0;
	/* fill in caps */
	obj->pCaps.dwSize = sizeof(DMUS_PORTCAPS);
	obj->pCaps.dwFlags = DMUS_PC_DLS | DMUS_PC_SOFTWARESYNTH | DMUS_PC_DIRECTSOUND | DMUS_PC_DLS2 | DMUS_PC_AUDIOPATH | DMUS_PC_WAVE;
	obj->pCaps.guidPort = CLSID_DirectMusicSynth;
	obj->pCaps.dwClass = DMUS_PC_OUTPUTCLASS;
	obj->pCaps.dwType = DMUS_PORT_WINMM_DRIVER;
	obj->pCaps.dwMemorySize = DMUS_PC_SYSTEMMEMORY;
	obj->pCaps.dwMaxChannelGroups = 1000;
	obj->pCaps.dwMaxVoices = 1000;
	obj->pCaps.dwMaxAudioChannels = -1;
	obj->pCaps.dwEffectFlags = DMUS_EFFECT_REVERB | DMUS_EFFECT_CHORUS | DMUS_EFFECT_DELAY;
	MultiByteToWideChar (CP_ACP, 0, "Microsotf Synthesizer", -1, obj->pCaps.wszDescription, sizeof(obj->pCaps.wszDescription)/sizeof(WCHAR));
	/* assign latency clock */
	/*DMUSIC_CreateReferenceClockImpl (&IID_IReferenceClock, (LPVOID*)&This->pLatencyClock, NULL); */

	return IDirectMusicSynth8Impl_QueryInterface ((LPDIRECTMUSICSYNTH8)obj, lpcGUID, ppobj);
}
