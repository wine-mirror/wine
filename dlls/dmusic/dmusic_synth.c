/* IDirectMusicSynth Implementation
 * IDirectMusicSynth8 Implementation
 * IDirectMusicSynthSink Implementation
 *
 * Copyright (C) 2003 Rok Mandeljc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "dmusic_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

/* IDirectMusicSynth IUnknown parts follow: */
HRESULT WINAPI IDirectMusicSynthImpl_QueryInterface (LPDIRECTMUSICSYNTH iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicSynthImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicSynth))
	{
		IDirectMusicSynthImpl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicSynthImpl_AddRef (LPDIRECTMUSICSYNTH iface)
{
	ICOM_THIS(IDirectMusicSynthImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicSynthImpl_Release (LPDIRECTMUSICSYNTH iface)
{
	ICOM_THIS(IDirectMusicSynthImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicSynth Interface follow: */
HRESULT WINAPI IDirectMusicSynthImpl_Open (LPDIRECTMUSICSYNTH iface, LPDMUS_PORTPARAMS pPortParams)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynthImpl_Close (LPDIRECTMUSICSYNTH iface)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynthImpl_SetNumChannelGroups (LPDIRECTMUSICSYNTH iface, DWORD dwGroups)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynthImpl_Download (LPDIRECTMUSICSYNTH iface, LPHANDLE phDownload, LPVOID pvData, LPBOOL pbFree)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynthImpl_Unload (LPDIRECTMUSICSYNTH iface, HANDLE hDownload, HRESULT (CALLBACK* lpFreeHandle)(HANDLE,HANDLE), HANDLE hUserData)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynthImpl_PlayBuffer (LPDIRECTMUSICSYNTH iface, REFERENCE_TIME rt, LPBYTE pbBuffer, DWORD cbBuffer)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynthImpl_GetRunningStats (LPDIRECTMUSICSYNTH iface, LPDMUS_SYNTHSTATS pStats)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynthImpl_GetPortCaps (LPDIRECTMUSICSYNTH iface, LPDMUS_PORTCAPS pCaps)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynthImpl_SetMasterClock (LPDIRECTMUSICSYNTH iface, IReferenceClock* pClock)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynthImpl_GetLatencyClock (LPDIRECTMUSICSYNTH iface, IReferenceClock** ppClock)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynthImpl_Activate (LPDIRECTMUSICSYNTH iface, BOOL fEnable)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynthImpl_SetSynthSink (LPDIRECTMUSICSYNTH iface, IDirectMusicSynthSink* pSynthSink)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynthImpl_Render (LPDIRECTMUSICSYNTH iface, short* pBuffer, DWORD dwLength, LONGLONG llPosition)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynthImpl_SetChannelPriority (LPDIRECTMUSICSYNTH iface, DWORD dwChannelGroup, DWORD dwChannel, DWORD dwPriority)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynthImpl_GetChannelPriority (LPDIRECTMUSICSYNTH iface, DWORD dwChannelGroup, DWORD dwChannel, LPDWORD pdwPriority)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynthImpl_GetFormat (LPDIRECTMUSICSYNTH iface, LPWAVEFORMATEX pWaveFormatEx, LPDWORD pdwWaveFormatExSiz)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynthImpl_GetAppend (LPDIRECTMUSICSYNTH iface, DWORD* pdwAppend)
{
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectMusicSynth) DirectMusicSynth_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicSynthImpl_QueryInterface,
	IDirectMusicSynthImpl_AddRef,
	IDirectMusicSynthImpl_Release,
	IDirectMusicSynthImpl_Open,
	IDirectMusicSynthImpl_Close,
	IDirectMusicSynthImpl_SetNumChannelGroups,
	IDirectMusicSynthImpl_Download,
	IDirectMusicSynthImpl_Unload,
	IDirectMusicSynthImpl_PlayBuffer,
	IDirectMusicSynthImpl_GetRunningStats,
	IDirectMusicSynthImpl_GetPortCaps,
	IDirectMusicSynthImpl_SetMasterClock,
	IDirectMusicSynthImpl_GetLatencyClock,
	IDirectMusicSynthImpl_Activate,
	IDirectMusicSynthImpl_SetSynthSink,
	IDirectMusicSynthImpl_Render,
	IDirectMusicSynthImpl_SetChannelPriority,
	IDirectMusicSynthImpl_GetChannelPriority,
	IDirectMusicSynthImpl_GetFormat,
	IDirectMusicSynthImpl_GetAppend
};


/* IDirectMusicSynth8 IUnknown parts follow: */
HRESULT WINAPI IDirectMusicSynth8Impl_QueryInterface (LPDIRECTMUSICSYNTH8 iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicSynth8Impl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicSynth8))
	{
		IDirectMusicSynth8Impl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicSynth8Impl_AddRef (LPDIRECTMUSICSYNTH8 iface)
{
	ICOM_THIS(IDirectMusicSynth8Impl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicSynth8Impl_Release (LPDIRECTMUSICSYNTH8 iface)
{
	ICOM_THIS(IDirectMusicSynth8Impl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicSynth Interface parts follow: */
HRESULT WINAPI IDirectMusicSynth8Impl_Open (LPDIRECTMUSICSYNTH8 iface, LPDMUS_PORTPARAMS pPortParams)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynth8Impl_Close (LPDIRECTMUSICSYNTH8 iface)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynth8Impl_SetNumChannelGroups (LPDIRECTMUSICSYNTH8 iface, DWORD dwGroups)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynth8Impl_Download (LPDIRECTMUSICSYNTH8 iface, LPHANDLE phDownload, LPVOID pvData, LPBOOL pbFree)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynth8Impl_Unload (LPDIRECTMUSICSYNTH8 iface, HANDLE hDownload, HRESULT (CALLBACK* lpFreeHandle)(HANDLE,HANDLE), HANDLE hUserData)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynth8Impl_PlayBuffer (LPDIRECTMUSICSYNTH8 iface, REFERENCE_TIME rt, LPBYTE pbBuffer, DWORD cbBuffer)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynth8Impl_GetRunningStats (LPDIRECTMUSICSYNTH8 iface, LPDMUS_SYNTHSTATS pStats)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynth8Impl_GetPortCaps (LPDIRECTMUSICSYNTH8 iface, LPDMUS_PORTCAPS pCaps)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynth8Impl_SetMasterClock (LPDIRECTMUSICSYNTH8 iface, IReferenceClock* pClock)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynth8Impl_GetLatencyClock (LPDIRECTMUSICSYNTH8 iface, IReferenceClock** ppClock)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynth8Impl_Activate (LPDIRECTMUSICSYNTH8 iface, BOOL fEnable)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynth8Impl_SetSynthSink (LPDIRECTMUSICSYNTH8 iface, IDirectMusicSynthSink* pSynthSink)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynth8Impl_Render (LPDIRECTMUSICSYNTH8 iface, short* pBuffer, DWORD dwLength, LONGLONG llPosition)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynth8Impl_SetChannelPriority (LPDIRECTMUSICSYNTH8 iface, DWORD dwChannelGroup, DWORD dwChannel, DWORD dwPriority)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynth8Impl_GetChannelPriority (LPDIRECTMUSICSYNTH8 iface, DWORD dwChannelGroup, DWORD dwChannel, LPDWORD pdwPriority)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynth8Impl_GetFormat (LPDIRECTMUSICSYNTH8 iface, LPWAVEFORMATEX pWaveFormatEx, LPDWORD pdwWaveFormatExSiz)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynth8Impl_GetAppend (LPDIRECTMUSICSYNTH8 iface, DWORD* pdwAppend)
{
	FIXME("stub\n");
	return DS_OK;
}

/* IDirectMusicSynth8 Interface parts follow: */
HRESULT WINAPI IDirectMusicSynth8Impl_PlayVoice (LPDIRECTMUSICSYNTH8 iface, REFERENCE_TIME rt, DWORD dwVoiceId, DWORD dwChannelGroup, DWORD dwChannel, DWORD dwDLId, long prPitch, long vrVolume, SAMPLE_TIME stVoiceStart, SAMPLE_TIME stLoopStart, SAMPLE_TIME stLoopEnd)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynth8Impl_StopVoice (LPDIRECTMUSICSYNTH8 iface, REFERENCE_TIME rt, DWORD dwVoiceId)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynth8Impl_GetVoiceState (LPDIRECTMUSICSYNTH8 iface, DWORD dwVoice[], DWORD cbVoice, DMUS_VOICE_STATE dwVoiceState[])
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynth8Impl_Refresh (LPDIRECTMUSICSYNTH8 iface, DWORD dwDownloadID, DWORD dwFlags)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSynth8Impl_AssignChannelToBuses (LPDIRECTMUSICSYNTH8 iface, DWORD dwChannelGroup, DWORD dwChannel, LPDWORD pdwBuses, DWORD cBuses)
{
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectMusicSynth8) DirectMusicSynth8_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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


/* IDirectMusicSynthSink IUnknown parts follow: */
HRESULT WINAPI IDirectMusicSynthSinkImpl_QueryInterface (LPDIRECTMUSICSYNTHSINK iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicSynthSinkImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicSynthSink))
	{
		IDirectMusicSynthSinkImpl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicSynthSinkImpl_AddRef (LPDIRECTMUSICSYNTHSINK iface)
{
	ICOM_THIS(IDirectMusicSynthSinkImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicSynthSinkImpl_Release (LPDIRECTMUSICSYNTHSINK iface)
{
	ICOM_THIS(IDirectMusicSynthSinkImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicSynth Interface follow: */
HRESULT WINAPI IDirectMusicSinkSynthImpl_Init (LPDIRECTMUSICSYNTHSINK iface, IDirectMusicSynth* pSynth)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSinkSynthImpl_SetMasterClock (LPDIRECTMUSICSYNTHSINK iface, IReferenceClock* pClock)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSinkSynthImpl_GetLatencyClock (LPDIRECTMUSICSYNTHSINK iface, IReferenceClock** ppClock)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSinkSynthImpl_Activate (LPDIRECTMUSICSYNTHSINK iface, BOOL fEnable)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSinkSynthImpl_SampleToRefTime (LPDIRECTMUSICSYNTHSINK iface, LONGLONG llSampleTime, REFERENCE_TIME* prfTime)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSinkSynthImpl_RefTimeToSample (LPDIRECTMUSICSYNTHSINK iface, REFERENCE_TIME rfTime, LONGLONG* pllSampleTime)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSinkSynthImpl_SetDirectSound (LPDIRECTMUSICSYNTHSINK iface, LPDIRECTSOUND pDirectSound, LPDIRECTSOUNDBUFFER pDirectSoundBuffer)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSinkSynthImpl_GetDesiredBufferSize (LPDIRECTMUSICSYNTHSINK iface, LPDWORD pdwBufferSizeInSamples)
{
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectMusicSynthSink) DirectMusicSynthSink_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicSynthSinkImpl_QueryInterface,
	IDirectMusicSynthSinkImpl_AddRef,
	IDirectMusicSynthSinkImpl_Release,
	IDirectMusicSinkSynthImpl_Init,
	IDirectMusicSinkSynthImpl_SetMasterClock,
	IDirectMusicSinkSynthImpl_GetLatencyClock,
	IDirectMusicSinkSynthImpl_Activate,
	IDirectMusicSinkSynthImpl_SampleToRefTime,
	IDirectMusicSinkSynthImpl_RefTimeToSample,
	IDirectMusicSinkSynthImpl_SetDirectSound,
	IDirectMusicSinkSynthImpl_GetDesiredBufferSize
};
