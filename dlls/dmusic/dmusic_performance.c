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
		return DS_OK;
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
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_PlaySegment (LPDIRECTMUSICPERFORMANCE iface, IDirectMusicSegment* pSegment, DWORD dwFlags, __int64 i64StartTime, IDirectMusicSegmentState** ppSegmentState)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_Stop (LPDIRECTMUSICPERFORMANCE iface, IDirectMusicSegment* pSegment, IDirectMusicSegmentState* pSegmentState, MUSIC_TIME mtTime, DWORD dwFlags)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_GetSegmentState (LPDIRECTMUSICPERFORMANCE iface, IDirectMusicSegmentState** ppSegmentState, MUSIC_TIME mtTime)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_SetPrepareTime (LPDIRECTMUSICPERFORMANCE iface, DWORD dwMilliSeconds)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_GetPrepareTime (LPDIRECTMUSICPERFORMANCE iface, DWORD* pdwMilliSeconds)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_SetBumperLength (LPDIRECTMUSICPERFORMANCE iface, DWORD dwMilliSeconds)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_GetBumperLength (LPDIRECTMUSICPERFORMANCE iface, DWORD* pdwMilliSeconds)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_SendPMsg (LPDIRECTMUSICPERFORMANCE iface, DMUS_PMSG* pPMSG)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_MusicToReferenceTime (LPDIRECTMUSICPERFORMANCE iface, MUSIC_TIME mtTime, REFERENCE_TIME* prtTime)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_ReferenceToMusicTime (LPDIRECTMUSICPERFORMANCE iface, REFERENCE_TIME rtTime, MUSIC_TIME* pmtTime)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_IsPlaying (LPDIRECTMUSICPERFORMANCE iface, IDirectMusicSegment* pSegment, IDirectMusicSegmentState* pSegState)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_GetTime (LPDIRECTMUSICPERFORMANCE iface, REFERENCE_TIME* prtNow, MUSIC_TIME* pmtNow)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_AllocPMsg (LPDIRECTMUSICPERFORMANCE iface, ULONG cb, DMUS_PMSG** ppPMSG)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_FreePMsg (LPDIRECTMUSICPERFORMANCE iface, DMUS_PMSG* pPMSG)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_GetGraph (LPDIRECTMUSICPERFORMANCE iface, IDirectMusicGraph** ppGraph)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_SetGraph (LPDIRECTMUSICPERFORMANCE iface, IDirectMusicGraph* pGraph)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_SetNotificationHandle (LPDIRECTMUSICPERFORMANCE iface, HANDLE hNotification, REFERENCE_TIME rtMinimum)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_GetNotificationPMsg (LPDIRECTMUSICPERFORMANCE iface, DMUS_NOTIFICATION_PMSG** ppNotificationPMsg)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_AddNotificationType (LPDIRECTMUSICPERFORMANCE iface, REFGUID rguidNotificationType)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_RemoveNotificationType (LPDIRECTMUSICPERFORMANCE iface, REFGUID rguidNotificationType)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_AddPort (LPDIRECTMUSICPERFORMANCE iface, IDirectMusicPort* pPort)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_RemovePort (LPDIRECTMUSICPERFORMANCE iface, IDirectMusicPort* pPort)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_AssignPChannelBlock (LPDIRECTMUSICPERFORMANCE iface, DWORD dwBlockNum, IDirectMusicPort* pPort, DWORD dwGroup)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_AssignPChannel (LPDIRECTMUSICPERFORMANCE iface, DWORD dwPChannel, IDirectMusicPort* pPort, DWORD dwGroup, DWORD dwMChannel)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_PChannelInfo (LPDIRECTMUSICPERFORMANCE iface, DWORD dwPChannel, IDirectMusicPort** ppPort, DWORD* pdwGroup, DWORD* pdwMChannel)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_DownloadInstrument (LPDIRECTMUSICPERFORMANCE iface, IDirectMusicInstrument* pInst, DWORD dwPChannel, IDirectMusicDownloadedInstrument** ppDownInst, DMUS_NOTERANGE* pNoteRanges, DWORD dwNumNoteRanges, IDirectMusicPort** ppPort, DWORD* pdwGroup, DWORD* pdwMChannel)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_Invalidate (LPDIRECTMUSICPERFORMANCE iface, MUSIC_TIME mtTime, DWORD dwFlags)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_GetParam (LPDIRECTMUSICPERFORMANCE iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_SetParam (LPDIRECTMUSICPERFORMANCE iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, void* pParam)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_GetGlobalParam (LPDIRECTMUSICPERFORMANCE iface, REFGUID rguidType, void* pParam, DWORD dwSize)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_SetGlobalParam (LPDIRECTMUSICPERFORMANCE iface, REFGUID rguidType, void* pParam, DWORD dwSize)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_GetLatencyTime (LPDIRECTMUSICPERFORMANCE iface, REFERENCE_TIME* prtTime)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_GetQueueTime (LPDIRECTMUSICPERFORMANCE iface, REFERENCE_TIME* prtTime)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_AdjustTime (LPDIRECTMUSICPERFORMANCE iface, REFERENCE_TIME rtAmount)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_CloseDown (LPDIRECTMUSICPERFORMANCE iface)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_GetResolvedTime (LPDIRECTMUSICPERFORMANCE iface, REFERENCE_TIME rtTime, REFERENCE_TIME* prtResolved, DWORD dwTimeResolveFlags)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_MIDIToMusic (LPDIRECTMUSICPERFORMANCE iface, BYTE bMIDIValue, DMUS_CHORD_KEY* pChord, BYTE bPlayMode, BYTE bChordLevel, WORD* pwMusicValue)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_MusicToMIDI (LPDIRECTMUSICPERFORMANCE iface, WORD wMusicValue, DMUS_CHORD_KEY* pChord, BYTE bPlayMode, BYTE bChordLevel, BYTE* pbMIDIValue)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_TimeToRhythm (LPDIRECTMUSICPERFORMANCE iface, MUSIC_TIME mtTime, DMUS_TIMESIGNATURE* pTimeSig, WORD* pwMeasure, BYTE* pbBeat, BYTE* pbGrid, short* pnOffset)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformanceImpl_RhythmToTime (LPDIRECTMUSICPERFORMANCE iface, WORD wMeasure, BYTE bBeat, BYTE bGrid, short nOffset, DMUS_TIMESIGNATURE* pTimeSig, MUSIC_TIME* pmtTime)
{
	FIXME("stub\n");
	return DS_OK;
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


/* IDirectMusicPerformance8 IUnknown parts follow: */
HRESULT WINAPI IDirectMusicPerformance8Impl_QueryInterface (LPDIRECTMUSICPERFORMANCE8 iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicPerformance8Impl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicPerformance8))
	{
		IDirectMusicPerformance8Impl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
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
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_PlaySegment (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicSegment* pSegment, DWORD dwFlags, __int64 i64StartTime, IDirectMusicSegmentState** ppSegmentState)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_Stop (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicSegment* pSegment, IDirectMusicSegmentState* pSegmentState, MUSIC_TIME mtTime, DWORD dwFlags)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_GetSegmentState (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicSegmentState** ppSegmentState, MUSIC_TIME mtTime)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_SetPrepareTime (LPDIRECTMUSICPERFORMANCE8 iface, DWORD dwMilliSeconds)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_GetPrepareTime (LPDIRECTMUSICPERFORMANCE8 iface, DWORD* pdwMilliSeconds)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_SetBumperLength (LPDIRECTMUSICPERFORMANCE8 iface, DWORD dwMilliSeconds)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_GetBumperLength (LPDIRECTMUSICPERFORMANCE8 iface, DWORD* pdwMilliSeconds)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_SendPMsg (LPDIRECTMUSICPERFORMANCE8 iface, DMUS_PMSG* pPMSG)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_MusicToReferenceTime (LPDIRECTMUSICPERFORMANCE8 iface, MUSIC_TIME mtTime, REFERENCE_TIME* prtTime)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_ReferenceToMusicTime (LPDIRECTMUSICPERFORMANCE8 iface, REFERENCE_TIME rtTime, MUSIC_TIME* pmtTime)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_IsPlaying (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicSegment* pSegment, IDirectMusicSegmentState* pSegState)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_GetTime (LPDIRECTMUSICPERFORMANCE8 iface, REFERENCE_TIME* prtNow, MUSIC_TIME* pmtNow)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_AllocPMsg (LPDIRECTMUSICPERFORMANCE8 iface, ULONG cb, DMUS_PMSG** ppPMSG)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_FreePMsg (LPDIRECTMUSICPERFORMANCE8 iface, DMUS_PMSG* pPMSG)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_GetGraph (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicGraph** ppGraph)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_SetGraph (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicGraph* pGraph)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_SetNotificationHandle (LPDIRECTMUSICPERFORMANCE8 iface, HANDLE hNotification, REFERENCE_TIME rtMinimum)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_GetNotificationPMsg (LPDIRECTMUSICPERFORMANCE8 iface, DMUS_NOTIFICATION_PMSG** ppNotificationPMsg)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_AddNotificationType (LPDIRECTMUSICPERFORMANCE8 iface, REFGUID rguidNotificationType)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_RemoveNotificationType (LPDIRECTMUSICPERFORMANCE8 iface, REFGUID rguidNotificationType)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_AddPort (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicPort* pPort)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_RemovePort (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicPort* pPort)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_AssignPChannelBlock (LPDIRECTMUSICPERFORMANCE8 iface, DWORD dwBlockNum, IDirectMusicPort* pPort, DWORD dwGroup)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_AssignPChannel (LPDIRECTMUSICPERFORMANCE8 iface, DWORD dwPChannel, IDirectMusicPort* pPort, DWORD dwGroup, DWORD dwMChannel)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_PChannelInfo (LPDIRECTMUSICPERFORMANCE8 iface, DWORD dwPChannel, IDirectMusicPort** ppPort, DWORD* pdwGroup, DWORD* pdwMChannel)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_DownloadInstrument (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicInstrument* pInst, DWORD dwPChannel, IDirectMusicDownloadedInstrument** ppDownInst, DMUS_NOTERANGE* pNoteRanges, DWORD dwNumNoteRanges, IDirectMusicPort** ppPort, DWORD* pdwGroup, DWORD* pdwMChannel)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_Invalidate (LPDIRECTMUSICPERFORMANCE8 iface, MUSIC_TIME mtTime, DWORD dwFlags)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_GetParam (LPDIRECTMUSICPERFORMANCE8 iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_SetParam (LPDIRECTMUSICPERFORMANCE8 iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, void* pParam)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_GetGlobalParam (LPDIRECTMUSICPERFORMANCE8 iface, REFGUID rguidType, void* pParam, DWORD dwSize)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_SetGlobalParam (LPDIRECTMUSICPERFORMANCE8 iface, REFGUID rguidType, void* pParam, DWORD dwSize)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_GetLatencyTime (LPDIRECTMUSICPERFORMANCE8 iface, REFERENCE_TIME* prtTime)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_GetQueueTime (LPDIRECTMUSICPERFORMANCE8 iface, REFERENCE_TIME* prtTime)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_AdjustTime (LPDIRECTMUSICPERFORMANCE8 iface, REFERENCE_TIME rtAmount)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_CloseDown (LPDIRECTMUSICPERFORMANCE8 iface)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_GetResolvedTime (LPDIRECTMUSICPERFORMANCE8 iface, REFERENCE_TIME rtTime, REFERENCE_TIME* prtResolved, DWORD dwTimeResolveFlags)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_MIDIToMusic (LPDIRECTMUSICPERFORMANCE8 iface, BYTE bMIDIValue, DMUS_CHORD_KEY* pChord, BYTE bPlayMode, BYTE bChordLevel, WORD* pwMusicValue)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_MusicToMIDI (LPDIRECTMUSICPERFORMANCE8 iface, WORD wMusicValue, DMUS_CHORD_KEY* pChord, BYTE bPlayMode, BYTE bChordLevel, BYTE* pbMIDIValue)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_TimeToRhythm (LPDIRECTMUSICPERFORMANCE8 iface, MUSIC_TIME mtTime, DMUS_TIMESIGNATURE* pTimeSig, WORD* pwMeasure, BYTE* pbBeat, BYTE* pbGrid, short* pnOffset)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8Impl_RhythmToTime (LPDIRECTMUSICPERFORMANCE8 iface, WORD wMeasure, BYTE bBeat, BYTE bGrid, short nOffset, DMUS_TIMESIGNATURE* pTimeSig, MUSIC_TIME* pmtTime)
{
	FIXME("stub\n");
	return DS_OK;
}

/* IDirectMusicPerformance8 Interface part follow: */
HRESULT WINAPI IDirectMusicPerformance8ImplInitAudio (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusic** ppDirectMusic, IDirectSound** ppDirectSound, HWND hWnd, DWORD dwDefaultPathType, DWORD dwPChannelCount, DWORD dwFlags, DMUS_AUDIOPARAMS* pParams)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8ImplPlaySegmentEx (LPDIRECTMUSICPERFORMANCE8 iface, IUnknown* pSource, WCHAR* pwzSegmentName, IUnknown* pTransition, DWORD dwFlags, __int64 i64StartTime, IDirectMusicSegmentState** ppSegmentState, IUnknown* pFrom, IUnknown* pAudioPath)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8ImplStopEx (LPDIRECTMUSICPERFORMANCE8 iface, IUnknown* pObjectToStop, __int64 i64StopTime, DWORD dwFlags)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8ImplClonePMsg (LPDIRECTMUSICPERFORMANCE8 iface, DMUS_PMSG* pSourcePMSG, DMUS_PMSG** ppCopyPMSG)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8ImplCreateAudioPath (LPDIRECTMUSICPERFORMANCE8 iface, IUnknown* pSourceConfig, BOOL fActivate, IDirectMusicAudioPath** ppNewPath)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8ImplCreateStandardAudioPath (LPDIRECTMUSICPERFORMANCE8 iface, DWORD dwType, DWORD dwPChannelCount, BOOL fActivate, IDirectMusicAudioPath** ppNewPath)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8ImplSetDefaultAudioPath (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicAudioPath* pAudioPath)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8ImplGetDefaultAudioPath (LPDIRECTMUSICPERFORMANCE8 iface, IDirectMusicAudioPath** ppAudioPath)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPerformance8ImplGetParamEx (LPDIRECTMUSICPERFORMANCE8 iface, REFGUID rguidType, DWORD dwTrackID, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam)
{
	FIXME("stub\n");
	return DS_OK;
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
