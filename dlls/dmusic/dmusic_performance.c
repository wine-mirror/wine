/* IDirectMusicPerformance Implementation
 * IDirectMusicPerformance8 Implementation
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

/* IDirectMusicPerformance IUnknown parts follow: */
HRESULT WINAPI IDirectMusicPerformanceImpl_QueryInterface (LPDIRECTMUSICPERFORMANCE iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicPerformance))
	{
		IDirectMusicPerformanceImpl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	if (IsEqualGUID(riid, &IID_IDirectMusicPerformance8))
	{
		return DMUSIC_CreateDirectMusicPerformance8 (riid, (LPDIRECTMUSICPERFORMANCE8 *)ppobj, NULL);
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicPerformanceImpl_AddRef (LPDIRECTMUSICPERFORMANCE iface)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicPerformanceImpl_Release (LPDIRECTMUSICPERFORMANCE iface)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicPerformance Interface follow: */
HRESULT WINAPI IDirectMusicPerformanceImpl_Init (LPDIRECTMUSICPERFORMANCE iface, IDirectMusic** ppDirectMusic, LPDIRECTSOUND pDirectSound, HWND hWnd)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);
	
	FIXME("(iface = %p, dmusic = %p (*dmusic = %p), dsound = %p, hwnd = %p): forward to IDirectMusicPerformance8Impl::Init\n", This, ppDirectMusic, *ppDirectMusic, pDirectSound, hWnd);

        return IDirectMusicPerformanceImpl_Init((LPDIRECTMUSICPERFORMANCE) iface, ppDirectMusic, pDirectSound, hWnd);
}

HRESULT WINAPI IDirectMusicPerformanceImpl_PlaySegment (LPDIRECTMUSICPERFORMANCE iface, IDirectMusicSegment* pSegment, DWORD dwFlags, __int64 i64StartTime, IDirectMusicSegmentState** ppSegmentState)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %p, %ld, FIXME, %p): stub\n", This, pSegment, dwFlags/*, i64StartTime*/, ppSegmentState);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_Stop (LPDIRECTMUSICPERFORMANCE iface, IDirectMusicSegment* pSegment, IDirectMusicSegmentState* pSegmentState, MUSIC_TIME mtTime, DWORD dwFlags)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %p, %p, %ld, %ld): stub\n", This, pSegment, pSegmentState, mtTime, dwFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_GetSegmentState (LPDIRECTMUSICPERFORMANCE iface, IDirectMusicSegmentState** ppSegmentState, MUSIC_TIME mtTime)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p,%p, %ld): stub\n", This, ppSegmentState, mtTime);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_SetPrepareTime (LPDIRECTMUSICPERFORMANCE iface, DWORD dwMilliSeconds)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %ld): stub\n", This, dwMilliSeconds);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_GetPrepareTime (LPDIRECTMUSICPERFORMANCE iface, DWORD* pdwMilliSeconds)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %p): stub\n", This, pdwMilliSeconds);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_SetBumperLength (LPDIRECTMUSICPERFORMANCE iface, DWORD dwMilliSeconds)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %ld): stub\n", This, dwMilliSeconds);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_GetBumperLength (LPDIRECTMUSICPERFORMANCE iface, DWORD* pdwMilliSeconds)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %p): stub\n", This, pdwMilliSeconds);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_SendPMsg (LPDIRECTMUSICPERFORMANCE iface, DMUS_PMSG* pPMSG)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %p): stub\n", This, pPMSG);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_MusicToReferenceTime (LPDIRECTMUSICPERFORMANCE iface, MUSIC_TIME mtTime, REFERENCE_TIME* prtTime)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %ld, FIXME): stub\n", This, mtTime/*,prtTime*/);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_ReferenceToMusicTime (LPDIRECTMUSICPERFORMANCE iface, REFERENCE_TIME rtTime, MUSIC_TIME* pmtTime)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, FIXME, %p): stub\n", This/*, rtTime*/, pmtTime);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_IsPlaying (LPDIRECTMUSICPERFORMANCE iface, IDirectMusicSegment* pSegment, IDirectMusicSegmentState* pSegState)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %p, %p): stub\n", This, pSegment, pSegState);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_GetTime (LPDIRECTMUSICPERFORMANCE iface, REFERENCE_TIME* prtNow, MUSIC_TIME* pmtNow)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, FIXME, %p): stub\n", This/*, prtNow*/, pmtNow);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_AllocPMsg (LPDIRECTMUSICPERFORMANCE iface, ULONG cb, DMUS_PMSG** ppPMSG)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %ld, %p): stub\n", This, cb, ppPMSG);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_FreePMsg (LPDIRECTMUSICPERFORMANCE iface, DMUS_PMSG* pPMSG)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %p): stub\n", This, pPMSG);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_GetGraph (LPDIRECTMUSICPERFORMANCE iface, IDirectMusicGraph** ppGraph)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %p): stub\n", This, ppGraph);

	return IDirectMusicPerformance8Impl_GetGraph((LPDIRECTMUSICPERFORMANCE8) iface, ppGraph);
}

HRESULT WINAPI IDirectMusicPerformanceImpl_SetGraph (LPDIRECTMUSICPERFORMANCE iface, IDirectMusicGraph* pGraph)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %p): stub\n", This, pGraph);

	return IDirectMusicPerformance8Impl_SetGraph((LPDIRECTMUSICPERFORMANCE8) iface, pGraph);
}

HRESULT WINAPI IDirectMusicPerformanceImpl_SetNotificationHandle (LPDIRECTMUSICPERFORMANCE iface, HANDLE hNotification, REFERENCE_TIME rtMinimum)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %p, FIXME): stub\n", This, hNotification/*, rtMinimum*/);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_GetNotificationPMsg (LPDIRECTMUSICPERFORMANCE iface, DMUS_NOTIFICATION_PMSG** ppNotificationPMsg)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %p): stub\n", This, ppNotificationPMsg);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_AddNotificationType (LPDIRECTMUSICPERFORMANCE iface, REFGUID rguidNotificationType)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_RemoveNotificationType (LPDIRECTMUSICPERFORMANCE iface, REFGUID rguidNotificationType)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_AddPort (LPDIRECTMUSICPERFORMANCE iface, IDirectMusicPort* pPort)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);
	
	FIXME("(%p, %p): semi-stub\n", This, pPort);
	/* just pretend that the port has been added */
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_RemovePort (LPDIRECTMUSICPERFORMANCE iface, IDirectMusicPort* pPort)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %p): stub\n", This, pPort);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_AssignPChannelBlock (LPDIRECTMUSICPERFORMANCE iface, DWORD dwBlockNum, IDirectMusicPort* pPort, DWORD dwGroup)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);	

	FIXME("(%p, %ld, %p, %ld): forward to IDirectMusicPerformance8Impl::AssignPChannelBlock\n", This, dwBlockNum, pPort, dwGroup-1);

        return IDirectMusicPerformance8Impl_AssignPChannelBlock((LPDIRECTMUSICPERFORMANCE8) iface, dwBlockNum, pPort, dwGroup);
}

HRESULT WINAPI IDirectMusicPerformanceImpl_AssignPChannel (LPDIRECTMUSICPERFORMANCE iface, DWORD dwPChannel, 
							   IDirectMusicPort* pPort, DWORD dwGroup, DWORD dwMChannel)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	TRACE("(%p, %ld, %p, %ld, %ld): forward to IDirectMusicPerformance8Impl::AssignPChannel\n", This, dwPChannel, pPort, dwGroup, dwMChannel);

	return IDirectMusicPerformance8Impl_AssignPChannel((LPDIRECTMUSICPERFORMANCE8) iface, dwPChannel, pPort, dwGroup, dwMChannel);
}

HRESULT WINAPI IDirectMusicPerformanceImpl_PChannelInfo (LPDIRECTMUSICPERFORMANCE iface, DWORD dwPChannel, IDirectMusicPort** ppPort, DWORD* pdwGroup, DWORD* pdwMChannel)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %ld, %p, %p, %p): stub\n", This, dwPChannel, ppPort, pdwGroup, pdwMChannel);
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_DownloadInstrument (LPDIRECTMUSICPERFORMANCE iface, IDirectMusicInstrument* pInst, DWORD dwPChannel, IDirectMusicDownloadedInstrument** ppDownInst, DMUS_NOTERANGE* pNoteRanges, DWORD dwNumNoteRanges, IDirectMusicPort** ppPort, DWORD* pdwGroup, DWORD* pdwMChannel)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %p, %ld, %p, %p, %ld, %p, %p, %p): stub\n", This, pInst, dwPChannel, ppDownInst, pNoteRanges, dwNumNoteRanges, ppPort, pdwGroup, pdwMChannel);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_Invalidate (LPDIRECTMUSICPERFORMANCE iface, MUSIC_TIME mtTime, DWORD dwFlags)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %ld, %ld): stub\n", This, mtTime, dwFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_GetParam (LPDIRECTMUSICPERFORMANCE iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %s, %ld, %ld, %ld, %p, %p): stub\n", This, debugstr_guid(rguidType), dwGroupBits, dwIndex, mtTime, pmtNext, pParam);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_SetParam (LPDIRECTMUSICPERFORMANCE iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, void* pParam)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %s, %ld, %ld, %ld, %p): stub\n", This, debugstr_guid(rguidType), dwGroupBits, dwIndex, mtTime, pParam);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_GetGlobalParam (LPDIRECTMUSICPERFORMANCE iface, REFGUID rguidType, 
							   void* pParam, DWORD dwSize)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	TRACE("(%p, %s, %p, %ld): forward to IDirectMusicPerformance8Impl::GetGlobalParam\n", This, debugstr_guid(rguidType), pParam, dwSize);
	
	return IDirectMusicPerformance8Impl_GetGlobalParam((LPDIRECTMUSICPERFORMANCE8) iface, rguidType, pParam, dwSize);
}

HRESULT WINAPI IDirectMusicPerformanceImpl_SetGlobalParam (LPDIRECTMUSICPERFORMANCE iface, REFGUID rguidType, 
							   void* pParam, DWORD dwSize)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	TRACE("(%p, %s, %p, %ld): forward to IDirectMusicPerformance8Impl::SetGlobalParam\n", This, debugstr_guid(rguidType), pParam, dwSize);
	
	return IDirectMusicPerformance8Impl_SetGlobalParam((LPDIRECTMUSICPERFORMANCE8) iface, rguidType, pParam, dwSize);
}

HRESULT WINAPI IDirectMusicPerformanceImpl_GetLatencyTime (LPDIRECTMUSICPERFORMANCE iface, REFERENCE_TIME* prtTime)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %p): stub\n", This, prtTime);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_GetQueueTime (LPDIRECTMUSICPERFORMANCE iface, REFERENCE_TIME* prtTime)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %p): stub\n", This, prtTime);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_AdjustTime (LPDIRECTMUSICPERFORMANCE iface, REFERENCE_TIME rtAmount)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);
	
	FIXME("(%p, FIXME): stub\n", This/*, rtAmount*/);
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_CloseDown (LPDIRECTMUSICPERFORMANCE iface)
{
 	ICOM_THIS(IDirectMusicPerformanceImpl,iface);
	
	FIXME("(%p): stub\n", This);
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_GetResolvedTime (LPDIRECTMUSICPERFORMANCE iface, REFERENCE_TIME rtTime, REFERENCE_TIME* prtResolved, DWORD dwTimeResolveFlags)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, FIXME, %p, %ld): stub\n", This/*, rtTime*/, prtResolved, dwTimeResolveFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_MIDIToMusic (LPDIRECTMUSICPERFORMANCE iface, BYTE bMIDIValue, DMUS_CHORD_KEY* pChord, BYTE bPlayMode, BYTE bChordLevel, WORD* pwMusicValue)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %d, %p, %d, %d, %p): stub\n", This, bMIDIValue, pChord, bPlayMode, bChordLevel, pwMusicValue);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_MusicToMIDI (LPDIRECTMUSICPERFORMANCE iface, WORD wMusicValue, DMUS_CHORD_KEY* pChord, BYTE bPlayMode, BYTE bChordLevel, BYTE* pbMIDIValue)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %d, %p, %d, %d, %p): stub\n", This, wMusicValue, pChord, bPlayMode, bChordLevel, pbMIDIValue);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_TimeToRhythm (LPDIRECTMUSICPERFORMANCE iface, MUSIC_TIME mtTime, DMUS_TIMESIGNATURE* pTimeSig, WORD* pwMeasure, BYTE* pbBeat, BYTE* pbGrid, short* pnOffset)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %ld, %p, %p, %p, %p, %p): stub\n", This, mtTime, pTimeSig, pwMeasure, pbBeat, pbGrid, pnOffset);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_RhythmToTime (LPDIRECTMUSICPERFORMANCE iface, WORD wMeasure, BYTE bBeat, BYTE bGrid, short nOffset, DMUS_TIMESIGNATURE* pTimeSig, MUSIC_TIME* pmtTime)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %d, %d, %d, %i, %p, %p): stub\n", This, wMeasure, bBeat, bGrid, nOffset, pTimeSig, pmtTime);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicPerformance) DirectMusicPerformance_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicPerformanceImpl_QueryInterface,
	IDirectMusicPerformanceImpl_AddRef,
	IDirectMusicPerformanceImpl_Release,
	IDirectMusicPerformanceImpl_Init,
	IDirectMusicPerformanceImpl_PlaySegment,
	IDirectMusicPerformanceImpl_Stop,
	IDirectMusicPerformanceImpl_GetSegmentState,
	IDirectMusicPerformanceImpl_SetPrepareTime,
	IDirectMusicPerformanceImpl_GetPrepareTime,
	IDirectMusicPerformanceImpl_SetBumperLength,
	IDirectMusicPerformanceImpl_GetBumperLength,
	IDirectMusicPerformanceImpl_SendPMsg,
	IDirectMusicPerformanceImpl_MusicToReferenceTime,
	IDirectMusicPerformanceImpl_ReferenceToMusicTime,
	IDirectMusicPerformanceImpl_IsPlaying,
	IDirectMusicPerformanceImpl_GetTime,
	IDirectMusicPerformanceImpl_AllocPMsg,
	IDirectMusicPerformanceImpl_FreePMsg,
	IDirectMusicPerformanceImpl_GetGraph,
	IDirectMusicPerformanceImpl_SetGraph,
	IDirectMusicPerformanceImpl_SetNotificationHandle,
	IDirectMusicPerformanceImpl_GetNotificationPMsg,
	IDirectMusicPerformanceImpl_AddNotificationType,
	IDirectMusicPerformanceImpl_RemoveNotificationType,
	IDirectMusicPerformanceImpl_AddPort,
	IDirectMusicPerformanceImpl_RemovePort,
	IDirectMusicPerformanceImpl_AssignPChannelBlock,
	IDirectMusicPerformanceImpl_AssignPChannel,
	IDirectMusicPerformanceImpl_PChannelInfo,
	IDirectMusicPerformanceImpl_DownloadInstrument,
	IDirectMusicPerformanceImpl_Invalidate,
	IDirectMusicPerformanceImpl_GetParam,
	IDirectMusicPerformanceImpl_SetParam,
	IDirectMusicPerformanceImpl_GetGlobalParam,
	IDirectMusicPerformanceImpl_SetGlobalParam,
	IDirectMusicPerformanceImpl_GetLatencyTime,
	IDirectMusicPerformanceImpl_GetQueueTime,
	IDirectMusicPerformanceImpl_AdjustTime,
	IDirectMusicPerformanceImpl_CloseDown,
	IDirectMusicPerformanceImpl_GetResolvedTime,
	IDirectMusicPerformanceImpl_MIDIToMusic,
	IDirectMusicPerformanceImpl_MusicToMIDI,
	IDirectMusicPerformanceImpl_TimeToRhythm,
	IDirectMusicPerformanceImpl_RhythmToTime
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicPerformance (LPCGUID lpcGUID, LPDIRECTMUSICPERFORMANCE *ppDMPerf, LPUNKNOWN pUnkOuter)
{
	IDirectMusicPerformanceImpl *pPerf;

	TRACE("(%p,%p,%p)\n",lpcGUID, ppDMPerf, pUnkOuter);
	if (IsEqualGUID(lpcGUID, &IID_IDirectMusicPerformance))
	{
		pPerf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicPerformanceImpl));
		if (NULL == pPerf)
		{
			*ppDMPerf = (LPDIRECTMUSICPERFORMANCE)NULL;
			return E_OUTOFMEMORY;
		}
		pPerf->lpVtbl = &DirectMusicPerformance_Vtbl;
		pPerf->ref = 1;
		*ppDMPerf = (LPDIRECTMUSICPERFORMANCE)pPerf;
		return S_OK;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;
}


/* IDirectMusicPerformance8 IUnknown parts follow: */
HRESULT WINAPI IDirectMusicPerformance8Impl_QueryInterface (LPDIRECTMUSICPERFORMANCE8 iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || 
	    IsEqualGUID(riid, &IID_IDirectMusicPerformance) ||
	    IsEqualGUID(riid, &IID_IDirectMusicPerformance8))
	{
		IDirectMusicPerformance8Impl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicPerformance8Impl_AddRef (LPDIRECTMUSICPERFORMANCE8 iface)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicPerformance8Impl_Release (LPDIRECTMUSICPERFORMANCE8 iface)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicPerformance Interface part follow: */
HRESULT WINAPI IDirectMusicPerformance8Impl_Init (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusic** ppDirectMusic, LPDIRECTSOUND pDirectSound, HWND hWnd)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(iface = %p, dmusic = %p, dsound = %p, hwnd = %p): forward to IDirectMusicPerformanceImpl::Init\n", This, ppDirectMusic, pDirectSound, hWnd);

        if (This->dmusic || This->dsound)
	  return DMUS_E_ALREADY_INITED;
	
	if (NULL != ppDirectMusic && NULL != *ppDirectMusic) {
	  /* app creates it's own dmusic object and gives it to performance */
	  This->dmusic = (IDirectMusic*) *ppDirectMusic;
	  IDirectMusicImpl_AddRef((LPDIRECTMUSIC) This->dmusic);
	} else {
	  /* app allows the performance to initialise itfself and needs a pointer to object*/
	  /* maybe IID_IDirectMusic8 must be used here */
	  DMUSIC_CreateDirectMusic(&IID_IDirectMusic, (LPDIRECTMUSIC*) &This->dmusic, NULL);
	  if (ppDirectMusic) {
	    *ppDirectMusic = (LPDIRECTMUSIC) This->dmusic;
	    IDirectMusic_AddRef(*ppDirectMusic);
	  }
	}

	if (NULL != pDirectSound) {
	  This->dsound = (IDirectSound*) pDirectSound;
	  IDirectSound_AddRef((LPDIRECTSOUND) This->dsound);
	} else {
	  DirectSoundCreate8(&IID_IDirectSound8, (LPDIRECTSOUND8*) &This->dsound, NULL);
	}
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_PlaySegment (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicSegment* pSegment, DWORD dwFlags, __int64 i64StartTime, IDirectMusicSegmentState** ppSegmentState)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %p, %ld, FIXME, %p): stub\n", This, pSegment, dwFlags/*, i64StartTime*/, ppSegmentState);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_Stop (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicSegment* pSegment, IDirectMusicSegmentState* pSegmentState, MUSIC_TIME mtTime, DWORD dwFlags)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %p, %p, %ld, %ld): stub\n", This, pSegment, pSegmentState, mtTime, dwFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_GetSegmentState (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicSegmentState** ppSegmentState, MUSIC_TIME mtTime)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p,%p, %ld): stub\n", This, ppSegmentState, mtTime);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_SetPrepareTime (LPDIRECTMUSICPERFORMANCE8 iface, DWORD dwMilliSeconds)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);
	
	FIXME("(%p, %ld): stub\n", This, dwMilliSeconds);
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_GetPrepareTime (LPDIRECTMUSICPERFORMANCE8 iface, DWORD* pdwMilliSeconds)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pdwMilliSeconds);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_SetBumperLength (LPDIRECTMUSICPERFORMANCE8 iface, DWORD dwMilliSeconds)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %ld): stub\n", This, dwMilliSeconds);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_GetBumperLength (LPDIRECTMUSICPERFORMANCE8 iface, DWORD* pdwMilliSeconds)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pdwMilliSeconds);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_SendPMsg (LPDIRECTMUSICPERFORMANCE8 iface, DMUS_PMSG* pPMSG)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pPMSG);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_MusicToReferenceTime (LPDIRECTMUSICPERFORMANCE8 iface, MUSIC_TIME mtTime, REFERENCE_TIME* prtTime)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %ld, FIXME): stub\n", This, mtTime/*,prtTime*/);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_ReferenceToMusicTime (LPDIRECTMUSICPERFORMANCE8 iface, REFERENCE_TIME rtTime, MUSIC_TIME* pmtTime)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, FIXME, %p): stub\n", This/*, rtTime*/, pmtTime);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_IsPlaying (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicSegment* pSegment, IDirectMusicSegmentState* pSegState)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %p, %p): stub\n", This, pSegment, pSegState);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_GetTime (LPDIRECTMUSICPERFORMANCE8 iface, REFERENCE_TIME* prtNow, MUSIC_TIME* pmtNow)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, FIXME, %p): stub\n", This/*, prtNow*/, pmtNow);	
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_AllocPMsg (LPDIRECTMUSICPERFORMANCE8 iface, ULONG cb, DMUS_PMSG** ppPMSG)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %ld, %p): stub\n", This, cb, ppPMSG);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_FreePMsg (LPDIRECTMUSICPERFORMANCE8 iface, DMUS_PMSG* pPMSG)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pPMSG);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_GetGraph (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicGraph** ppGraph)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %p): to check\n", This, ppGraph);

	if (NULL != This->toolGraph) {
	  *ppGraph = (LPDIRECTMUSICGRAPH) This->toolGraph; 
	  IDirectMusicGraphImpl_AddRef((LPDIRECTMUSICGRAPH) *ppGraph);
	}
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_SetGraph (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicGraph* pGraph)
{
	ICOM_THIS(IDirectMusicPerformanceImpl,iface);

	FIXME("(%p, %p): to check\n", This, pGraph);

	if (NULL != This->toolGraph) {
	  /* Todo clean buffers and tools before */
	  IDirectMusicGraphImpl_Release((LPDIRECTMUSICGRAPH) This->toolGraph);
	}
	This->toolGraph = pGraph;
	if (NULL != This->toolGraph) {
	  IDirectMusicGraphImpl_AddRef((LPDIRECTMUSICGRAPH) This->toolGraph);
	}
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_SetNotificationHandle (LPDIRECTMUSICPERFORMANCE8 iface, HANDLE hNotification, REFERENCE_TIME rtMinimum)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %p, FIXME): stub\n", This, hNotification/*, rtMinimum*/);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_GetNotificationPMsg (LPDIRECTMUSICPERFORMANCE8 iface, DMUS_NOTIFICATION_PMSG** ppNotificationPMsg)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %p): stub\n", This, ppNotificationPMsg);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_AddNotificationType (LPDIRECTMUSICPERFORMANCE8 iface, REFGUID rguidNotificationType)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_RemoveNotificationType (LPDIRECTMUSICPERFORMANCE8 iface, REFGUID rguidNotificationType)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_AddPort (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicPort* pPort)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pPort);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_RemovePort (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicPort* pPort)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pPort);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_AssignPChannelBlock (LPDIRECTMUSICPERFORMANCE8 iface, DWORD dwBlockNum, IDirectMusicPort* pPort, DWORD dwGroup)
{
        int i, j, range /* min value in range */;
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %ld, %p, %ld): semi-stub\n", This, dwBlockNum, pPort, dwGroup-1);
        
        range = 16 * dwBlockNum;
        j = 0;
                
        for (i = range; i < range+16; i++)
        {
                /*TRACE("Setting PChannel[%i] to port %p, group %ld, MIDI port %i\n", i, pPort, dwGroup-1, j); */
                This->PChannel[i].port = pPort; 
                This->PChannel[i].group = dwGroup - 1; /* first index is always zero */
                This->PChannel[i].channel = j; /* FIXME: should this be assigned? */
                j++;
        }
                
        return S_FALSE;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_AssignPChannel (LPDIRECTMUSICPERFORMANCE8 iface, DWORD dwPChannel, IDirectMusicPort* pPort, DWORD dwGroup, DWORD dwMChannel)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	TRACE("(%p, %ld, %p, %ld, %ld)\n", This, dwPChannel, pPort, dwGroup, dwMChannel);
	This->PChannel[dwPChannel].port = pPort; 
	This->PChannel[dwPChannel].group = dwGroup; 
	This->PChannel[dwPChannel].channel = dwMChannel;

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_PChannelInfo (LPDIRECTMUSICPERFORMANCE8 iface, DWORD dwPChannel, IDirectMusicPort** ppPort, DWORD* pdwGroup, DWORD* pdwMChannel)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %ld, %p, %p, %p): stub\n", This, dwPChannel, ppPort, pdwGroup, pdwMChannel);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_DownloadInstrument (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicInstrument* pInst, DWORD dwPChannel, IDirectMusicDownloadedInstrument** ppDownInst, DMUS_NOTERANGE* pNoteRanges, DWORD dwNumNoteRanges, IDirectMusicPort** ppPort, DWORD* pdwGroup, DWORD* pdwMChannel)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %p, %ld, %p, %p, %ld, %p, %p, %p): stub\n", This, pInst, dwPChannel, ppDownInst, pNoteRanges, dwNumNoteRanges, ppPort, pdwGroup, pdwMChannel);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_Invalidate (LPDIRECTMUSICPERFORMANCE8 iface, MUSIC_TIME mtTime, DWORD dwFlags)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %ld, %ld): stub\n", This, mtTime, dwFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_GetParam (LPDIRECTMUSICPERFORMANCE8 iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %s, %ld, %ld, %ld, %p, %p): stub\n", This, debugstr_guid(rguidType), dwGroupBits, dwIndex, mtTime, pmtNext, pParam);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_SetParam (LPDIRECTMUSICPERFORMANCE8 iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, void* pParam)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %s, %ld, %ld, %ld, %p): stub\n", This, debugstr_guid(rguidType), dwGroupBits, dwIndex, mtTime, pParam);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_GetGlobalParam (LPDIRECTMUSICPERFORMANCE8 iface, REFGUID rguidType, void* pParam, DWORD dwSize)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	TRACE("(%p, %s, %p, %ld): stub\n", This, debugstr_guid(rguidType), pParam, dwSize);
	
	if (IsEqualGUID(rguidType, &GUID_PerfAutoDownload))
		memcpy(pParam, &This->AutoDownload, sizeof(&This->AutoDownload));
	if (IsEqualGUID(rguidType, &GUID_PerfMasterGrooveLevel))
		memcpy(pParam, &This->MasterGrooveLevel, sizeof(&This->MasterGrooveLevel));
	if (IsEqualGUID(rguidType, &GUID_PerfMasterTempo))
		memcpy(pParam, &This->MasterTempo, sizeof(&This->MasterTempo));
	if (IsEqualGUID(rguidType, &GUID_PerfMasterVolume))
		memcpy(pParam, &This->MasterVolume, sizeof(&This->MasterVolume));

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_SetGlobalParam (LPDIRECTMUSICPERFORMANCE8 iface, REFGUID rguidType, void* pParam, DWORD dwSize)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	TRACE("(%p, %s, %p, %ld)\n", This, debugstr_guid(rguidType), pParam, dwSize);
	
	if (IsEqualGUID(rguidType, &GUID_PerfAutoDownload))
	{
		memcpy(&This->AutoDownload, pParam, dwSize);
		TRACE("=> AutoDownload set to %d\n", This->AutoDownload);
	}
	if (IsEqualGUID(rguidType, &GUID_PerfMasterGrooveLevel))
	{
		memcpy(&This->MasterGrooveLevel, pParam, dwSize);
		TRACE("=> MasterGrooveLevel set to %i\n", This->MasterGrooveLevel);
	}
	if (IsEqualGUID(rguidType, &GUID_PerfMasterTempo))
	{
		memcpy(&This->MasterTempo, pParam, dwSize);
		TRACE("=> MasterTempo set to %f\n", This->MasterTempo);
	}
	if (IsEqualGUID(rguidType, &GUID_PerfMasterVolume))
	{
		memcpy(&This->MasterVolume, pParam, dwSize);
		TRACE("=> MasterVolume set to %li\n", This->MasterVolume);
	}

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_GetLatencyTime (LPDIRECTMUSICPERFORMANCE8 iface, REFERENCE_TIME* prtTime)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %p): stub\n", This, prtTime);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_GetQueueTime (LPDIRECTMUSICPERFORMANCE8 iface, REFERENCE_TIME* prtTime)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %p): stub\n", This, prtTime);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_AdjustTime (LPDIRECTMUSICPERFORMANCE8 iface, REFERENCE_TIME rtAmount)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, FIXME): stub\n", This/*, rtAmount*/);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_CloseDown (LPDIRECTMUSICPERFORMANCE8 iface)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p): stub\n", This);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_GetResolvedTime (LPDIRECTMUSICPERFORMANCE8 iface, REFERENCE_TIME rtTime, REFERENCE_TIME* prtResolved, DWORD dwTimeResolveFlags)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, FIXME, %p, %ld): stub\n", This/*, rtTime*/, prtResolved, dwTimeResolveFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_MIDIToMusic (LPDIRECTMUSICPERFORMANCE8 iface, BYTE bMIDIValue, DMUS_CHORD_KEY* pChord, BYTE bPlayMode, BYTE bChordLevel, WORD* pwMusicValue)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %d, %p, %d, %d, %p): stub\n", This, bMIDIValue, pChord, bPlayMode, bChordLevel, pwMusicValue);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_MusicToMIDI (LPDIRECTMUSICPERFORMANCE8 iface, WORD wMusicValue, DMUS_CHORD_KEY* pChord, BYTE bPlayMode, BYTE bChordLevel, BYTE* pbMIDIValue)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %d, %p, %d, %d, %p): stub\n", This, wMusicValue, pChord, bPlayMode, bChordLevel, pbMIDIValue);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_TimeToRhythm (LPDIRECTMUSICPERFORMANCE8 iface, MUSIC_TIME mtTime, DMUS_TIMESIGNATURE* pTimeSig, WORD* pwMeasure, BYTE* pbBeat, BYTE* pbGrid, short* pnOffset)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %ld, %p, %p, %p, %p, %p): stub\n", This, mtTime, pTimeSig, pwMeasure, pbBeat, pbGrid, pnOffset);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_RhythmToTime (LPDIRECTMUSICPERFORMANCE8 iface, WORD wMeasure, BYTE bBeat, BYTE bGrid, short nOffset, DMUS_TIMESIGNATURE* pTimeSig, MUSIC_TIME* pmtTime)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %d, %d, %d, %i, %p, %p): stub\n", This, wMeasure, bBeat, bGrid, nOffset, pTimeSig, pmtTime);

	return S_OK;
}

/* IDirectMusicPerformance8 Interface part follow: */
HRESULT WINAPI IDirectMusicPerformance8ImplInitAudio (LPDIRECTMUSICPERFORMANCE8 iface, 
						      IDirectMusic** ppDirectMusic, 
						      IDirectSound** ppDirectSound, 
						      HWND hWnd, 
						      DWORD dwDefaultPathType, 
						      DWORD dwPChannelCount, 
						      DWORD dwFlags, 
						      DMUS_AUDIOPARAMS* pParams)
{
        ICOM_THIS(IDirectMusicPerformance8Impl,iface);
	FIXME("(%p, %p, %p, %p, %lx, %lu, %lx, %p): to check\n", This, ppDirectMusic, ppDirectSound, hWnd, dwDefaultPathType, dwPChannelCount, dwFlags, pParams);

        if (This->dmusic || This->dsound)
	  return DMUS_E_ALREADY_INITED;

	if (NULL != ppDirectSound && NULL != *ppDirectSound) {
	  This->dsound = *ppDirectSound;
	} else {
	  DirectSoundCreate8(&IID_IDirectSound8, (LPDIRECTSOUND8*) &This->dsound, NULL);
	  if (ppDirectSound)
	    *ppDirectSound = This->dsound;  
	}
	
	IDirectMusicPerformance8Impl_Init(iface, ppDirectMusic, This->dsound, hWnd);

	/* Init increases the ref count of the dsound object. Decremente it if the app don't want a pointer to the object. */
        if (!ppDirectSound)
	  IDirectSound_Release(*ppDirectSound);
	
	/* as seen in msdn we need params init before audio path creation */
	if (NULL != pParams) {
	  memcpy(&This->params, pParams, sizeof(DMUS_AUDIOPARAMS));
	} else {
	  /* TODO, how can i fill the struct */
	}
	IDirectMusicPerformance8ImplCreateStandardAudioPath(iface, dwDefaultPathType, dwPChannelCount, FALSE, (IDirectMusicAudioPath**) &This->default_path);

	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8ImplPlaySegmentEx (LPDIRECTMUSICPERFORMANCE8 iface, IUnknown* pSource, WCHAR* pwzSegmentName, IUnknown* pTransition, DWORD dwFlags, __int64 i64StartTime, IDirectMusicSegmentState** ppSegmentState, IUnknown* pFrom, IUnknown* pAudioPath)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %p, %p, %p, %ld, FIXME, %p, %p, %p): stub\n", This, pSource, pwzSegmentName, pTransition, dwFlags/*, i64StartTime*/, ppSegmentState, pFrom, pAudioPath);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8ImplStopEx (LPDIRECTMUSICPERFORMANCE8 iface, IUnknown* pObjectToStop, __int64 i64StopTime, DWORD dwFlags)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %p, FIXME, %ld): stub\n", This, pObjectToStop/*, i64StopTime*/, dwFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8ImplClonePMsg (LPDIRECTMUSICPERFORMANCE8 iface, DMUS_PMSG* pSourcePMSG, DMUS_PMSG** ppCopyPMSG)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %p, %p): stub\n", This, pSourcePMSG, ppCopyPMSG);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8ImplCreateAudioPath (LPDIRECTMUSICPERFORMANCE8 iface, IUnknown* pSourceConfig, BOOL fActivate, IDirectMusicAudioPath** ppNewPath)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %p, %d, %p): stub\n", This, pSourceConfig, fActivate, ppNewPath);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPerformance8ImplCreateStandardAudioPath (LPDIRECTMUSICPERFORMANCE8 iface, DWORD dwType, DWORD dwPChannelCount, BOOL fActivate, IDirectMusicAudioPath** ppNewPath)
{
	IDirectMusicAudioPathImpl *default_path;
	DSBUFFERDESC desc;
	WAVEFORMATEX format;
	LPDIRECTSOUNDBUFFER8 buffer;

	ICOM_THIS(IDirectMusicPerformance8Impl,iface);
	
	FIXME("(%p)->(%ld, %ld, %d, %p): semi-stub\n", This, dwType, dwPChannelCount, fActivate, ppNewPath);

	default_path = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicAudioPathImpl));
	if (NULL == default_path)
	{
		*ppNewPath = (LPDIRECTMUSICAUDIOPATH)NULL;
		return E_OUTOFMEMORY;
	}
	default_path->lpVtbl = &DirectMusicAudioPath_Vtbl;
	default_path->ref = 1;
	default_path->perfo = (IDirectMusicPerformance*) This;

	/* Secondary buffer description */
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = 1;
	format.nSamplesPerSec = 44000;
	format.nAvgBytesPerSec = 44000*2;
	format.nBlockAlign = 2;
	format.wBitsPerSample = 16;
	format.cbSize = 0;
	
	desc.dwSize = sizeof(desc);
	desc.dwFlags = 0;
	desc.dwBufferBytes = DSBSIZE_MIN;
	desc.dwReserved = 0;
	desc.lpwfxFormat = &format;
	desc.guid3DAlgorithm = GUID_NULL;
	
	switch(dwType) {
	case DMUS_APATH_DYNAMIC_3D:
		desc.dwFlags |= DSBCAPS_CTRL3D;
		break;
	case DMUS_APATH_DYNAMIC_MONO:
		break;
	case DMUS_APATH_SHARED_STEREOPLUSREVERB:	
	case DMUS_APATH_DYNAMIC_STEREO:
		format.nChannels = 2;
		format.nBlockAlign *= 2;
		format.nAvgBytesPerSec *=2;
		break;
	default:
		break;
	}

	/* FIXME: Should we create one secondary buffer for each PChannel? */
	IDirectSound8_CreateSoundBuffer((LPDIRECTSOUND8) This->dsound, &desc, &buffer, NULL);
	default_path->buffer = (IDirectSoundBuffer*) buffer;

	/* Update description for creating primary buffer */
	desc.dwFlags |= DSBCAPS_PRIMARYBUFFER;
	desc.dwBufferBytes = 0;
	desc.lpwfxFormat = NULL;

	IDirectSound8_CreateSoundBuffer((LPDIRECTSOUND8) This->dsound, &desc, &buffer, NULL);

	default_path->primary = (IDirectSoundBuffer*) buffer;

	*ppNewPath = (LPDIRECTMUSICAUDIOPATH) default_path;
	
	TRACE(" returning IDirectMusicPerformance interface at %p.\n", *ppNewPath);

	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8ImplSetDefaultAudioPath (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicAudioPath* pAudioPath)
{
       ICOM_THIS(IDirectMusicPerformance8Impl,iface);

       	FIXME("(%p, %p): semi-stub\n", This, pAudioPath);

       if (NULL != This->default_path) {
         IDirectMusicAudioPathImpl_Release((LPDIRECTMUSICAUDIOPATH) This->default_path);
	 ((IDirectMusicAudioPathImpl*) This->default_path)->perfo = NULL;
         This->default_path = NULL;
       }
       This->default_path = pAudioPath;
       if (NULL != This->default_path) {
         IDirectMusicAudioPathImpl_AddRef((LPDIRECTMUSICAUDIOPATH) This->default_path);
	 ((IDirectMusicAudioPathImpl*) This->default_path)->perfo = (IDirectMusicPerformance*) This;
       }
       return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8ImplGetDefaultAudioPath (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicAudioPath** ppAudioPath)
{
        ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %p): semi-stub\n", This, ppAudioPath);

	if (NULL != This->default_path) {
	  *ppAudioPath = (LPDIRECTMUSICAUDIOPATH) This->default_path;
          IDirectMusicAudioPathImpl_AddRef(*ppAudioPath);
        } else {
	  *ppAudioPath = NULL;
        }
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8ImplGetParamEx (LPDIRECTMUSICPERFORMANCE8 iface, REFGUID rguidType, DWORD dwTrackID, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	FIXME("(%p, %s, %ld, %ld, %ld, %ld, %p, %p): stub\n", This, debugstr_guid(rguidType), dwTrackID, dwGroupBits, dwIndex, mtTime, pmtNext, pParam);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicPerformance8) DirectMusicPerformance8_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicPerformance8Impl_QueryInterface,
	IDirectMusicPerformance8Impl_AddRef,
	IDirectMusicPerformance8Impl_Release,
	IDirectMusicPerformance8Impl_Init,
	IDirectMusicPerformance8Impl_PlaySegment,
	IDirectMusicPerformance8Impl_Stop,
	IDirectMusicPerformance8Impl_GetSegmentState,
	IDirectMusicPerformance8Impl_SetPrepareTime,
	IDirectMusicPerformance8Impl_GetPrepareTime,
	IDirectMusicPerformance8Impl_SetBumperLength,
	IDirectMusicPerformance8Impl_GetBumperLength,
	IDirectMusicPerformance8Impl_SendPMsg,
	IDirectMusicPerformance8Impl_MusicToReferenceTime,
	IDirectMusicPerformance8Impl_ReferenceToMusicTime,
	IDirectMusicPerformance8Impl_IsPlaying,
	IDirectMusicPerformance8Impl_GetTime,
	IDirectMusicPerformance8Impl_AllocPMsg,
	IDirectMusicPerformance8Impl_FreePMsg,
	IDirectMusicPerformance8Impl_GetGraph,
	IDirectMusicPerformance8Impl_SetGraph,
	IDirectMusicPerformance8Impl_SetNotificationHandle,
	IDirectMusicPerformance8Impl_GetNotificationPMsg,
	IDirectMusicPerformance8Impl_AddNotificationType,
	IDirectMusicPerformance8Impl_RemoveNotificationType,
	IDirectMusicPerformance8Impl_AddPort,
	IDirectMusicPerformance8Impl_RemovePort,
	IDirectMusicPerformance8Impl_AssignPChannelBlock,
	IDirectMusicPerformance8Impl_AssignPChannel,
	IDirectMusicPerformance8Impl_PChannelInfo,
	IDirectMusicPerformance8Impl_DownloadInstrument,
	IDirectMusicPerformance8Impl_Invalidate,
	IDirectMusicPerformance8Impl_GetParam,
	IDirectMusicPerformance8Impl_SetParam,
	IDirectMusicPerformance8Impl_GetGlobalParam,
	IDirectMusicPerformance8Impl_SetGlobalParam,
	IDirectMusicPerformance8Impl_GetLatencyTime,
	IDirectMusicPerformance8Impl_GetQueueTime,
	IDirectMusicPerformance8Impl_AdjustTime,
	IDirectMusicPerformance8Impl_CloseDown,
	IDirectMusicPerformance8Impl_GetResolvedTime,
	IDirectMusicPerformance8Impl_MIDIToMusic,
	IDirectMusicPerformance8Impl_MusicToMIDI,
	IDirectMusicPerformance8Impl_TimeToRhythm,
	IDirectMusicPerformance8Impl_RhythmToTime,
	IDirectMusicPerformance8ImplInitAudio,
	IDirectMusicPerformance8ImplPlaySegmentEx,
	IDirectMusicPerformance8ImplStopEx,
	IDirectMusicPerformance8ImplClonePMsg,
	IDirectMusicPerformance8ImplCreateAudioPath,
	IDirectMusicPerformance8ImplCreateStandardAudioPath,
	IDirectMusicPerformance8ImplSetDefaultAudioPath,
	IDirectMusicPerformance8ImplGetDefaultAudioPath,
	IDirectMusicPerformance8ImplGetParamEx
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicPerformance8 (LPCGUID lpcGUID, LPDIRECTMUSICPERFORMANCE8 *ppDMPerf8, LPUNKNOWN pUnkOuter)
{
	IDirectMusicPerformance8Impl *pPerf8;

	TRACE("(%p,%p,%p)\n",lpcGUID, ppDMPerf8, pUnkOuter);
	if (IsEqualGUID(lpcGUID, &IID_IDirectMusicPerformance8))
	{
		pPerf8 = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicPerformance8Impl));
		if (NULL == pPerf8)
		{
			*ppDMPerf8 = (LPDIRECTMUSICPERFORMANCE8)NULL;
			return E_OUTOFMEMORY;
		}
		pPerf8->lpVtbl = &DirectMusicPerformance8_Vtbl;
		pPerf8->ref = 1;
		pPerf8->dmusic = NULL;
		pPerf8->dsound = NULL;
		pPerf8->default_path = NULL;
		
		*ppDMPerf8 = (LPDIRECTMUSICPERFORMANCE8)pPerf8;
		return S_OK;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;
}
