/* IDirectMusicSegment Implementation
 * IDirectMusicSegment8 Implementation
 * IDirectMusicSegmentState Implementation
 * IDirectMusicSegmentState8 Implementation
 * IDirectMusicPatternTrack Implementation
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

/* IDirectMusicSegment IUnknown parts follow: */
HRESULT WINAPI IDirectMusicSegmentImpl_QueryInterface (LPDIRECTMUSICSEGMENT iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicSegmentImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicSegment))
	{
		IDirectMusicSegmentImpl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicSegmentImpl_AddRef (LPDIRECTMUSICSEGMENT iface)
{
	ICOM_THIS(IDirectMusicSegmentImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicSegmentImpl_Release (LPDIRECTMUSICSEGMENT iface)
{
	ICOM_THIS(IDirectMusicSegmentImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicSegment Interface follow: */
HRESULT WINAPI IDirectMusicSegmentImpl_GetLength (LPDIRECTMUSICSEGMENT iface, MUSIC_TIME* pmtLength)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentImpl_SetLength (LPDIRECTMUSICSEGMENT iface, MUSIC_TIME mtLength)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentImpl_GetRepeats (LPDIRECTMUSICSEGMENT iface, DWORD* pdwRepeats)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentImpl_SetRepeats (LPDIRECTMUSICSEGMENT iface, DWORD dwRepeats)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentImpl_GetDefaultResolution (LPDIRECTMUSICSEGMENT iface, DWORD* pdwResolution)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentImpl_SetDefaultResolution (LPDIRECTMUSICSEGMENT iface, DWORD dwResolution)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentImpl_GetTrack (LPDIRECTMUSICSEGMENT iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, IDirectMusicTrack** ppTrack)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentImpl_GetTrackGroup (LPDIRECTMUSICSEGMENT iface, IDirectMusicTrack* pTrack, DWORD* pdwGroupBits)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentImpl_InsertTrack (LPDIRECTMUSICSEGMENT iface, IDirectMusicTrack* pTrack, DWORD dwGroupBits)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentImpl_RemoveTrack (LPDIRECTMUSICSEGMENT iface, IDirectMusicTrack* pTrack)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentImpl_InitPlay (LPDIRECTMUSICSEGMENT iface, IDirectMusicSegmentState** ppSegState, IDirectMusicPerformance* pPerformance, DWORD dwFlags)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentImpl_GetGraph (LPDIRECTMUSICSEGMENT iface, IDirectMusicGraph** ppGraph)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentImpl_SetGraph (LPDIRECTMUSICSEGMENT iface, IDirectMusicGraph* pGraph)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentImpl_AddNotificationType (LPDIRECTMUSICSEGMENT iface, REFGUID rguidNotificationType)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentImpl_RemoveNotificationType (LPDIRECTMUSICSEGMENT iface, REFGUID rguidNotificationType)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentImpl_GetParam (LPDIRECTMUSICSEGMENT iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentImpl_SetParam (LPDIRECTMUSICSEGMENT iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, void* pParam)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentImpl_Clone (LPDIRECTMUSICSEGMENT iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicSegment** ppSegment)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentImpl_SetStartPoint (LPDIRECTMUSICSEGMENT iface, MUSIC_TIME mtStart)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentImpl_GetStartPoint (LPDIRECTMUSICSEGMENT iface, MUSIC_TIME* pmtStart)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentImpl_SetLoopPoints (LPDIRECTMUSICSEGMENT iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentImpl_GetLoopPoints (LPDIRECTMUSICSEGMENT iface, MUSIC_TIME* pmtStart, MUSIC_TIME* pmtEnd)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentImpl_SetPChannelsUsed (LPDIRECTMUSICSEGMENT iface, DWORD dwNumPChannels, DWORD* paPChannels)
{
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectMusicSegment) DirectMusicSegment_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicSegmentImpl_QueryInterface,
	IDirectMusicSegmentImpl_AddRef,
	IDirectMusicSegmentImpl_Release,
	IDirectMusicSegmentImpl_GetLength,
	IDirectMusicSegmentImpl_SetLength,
	IDirectMusicSegmentImpl_GetRepeats,
	IDirectMusicSegmentImpl_SetRepeats,
	IDirectMusicSegmentImpl_GetDefaultResolution,
	IDirectMusicSegmentImpl_SetDefaultResolution,
	IDirectMusicSegmentImpl_GetTrack,
	IDirectMusicSegmentImpl_GetTrackGroup,
	IDirectMusicSegmentImpl_InsertTrack,
	IDirectMusicSegmentImpl_RemoveTrack,
	IDirectMusicSegmentImpl_InitPlay,
	IDirectMusicSegmentImpl_GetGraph,
	IDirectMusicSegmentImpl_SetGraph,
	IDirectMusicSegmentImpl_AddNotificationType,
	IDirectMusicSegmentImpl_RemoveNotificationType,
	IDirectMusicSegmentImpl_GetParam,
	IDirectMusicSegmentImpl_SetParam,
	IDirectMusicSegmentImpl_Clone,
	IDirectMusicSegmentImpl_SetStartPoint,
	IDirectMusicSegmentImpl_GetStartPoint,
	IDirectMusicSegmentImpl_SetLoopPoints,
	IDirectMusicSegmentImpl_GetLoopPoints,
	IDirectMusicSegmentImpl_SetPChannelsUsed
};


/* IDirectMusicSegment8 IUnknown parts follow: */
HRESULT WINAPI IDirectMusicSegment8Impl_QueryInterface (LPDIRECTMUSICSEGMENT8 iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicSegment8))
	{
		IDirectMusicSegment8Impl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicSegment8Impl_AddRef (LPDIRECTMUSICSEGMENT8 iface)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicSegment8Impl_Release (LPDIRECTMUSICSEGMENT8 iface)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicSegment Interface part follow: */
HRESULT WINAPI IDirectMusicSegment8Impl_GetLength (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME* pmtLength)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_SetLength (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME mtLength)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_GetRepeats (LPDIRECTMUSICSEGMENT8 iface, DWORD* pdwRepeats)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_SetRepeats (LPDIRECTMUSICSEGMENT8 iface, DWORD dwRepeats)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_GetDefaultResolution (LPDIRECTMUSICSEGMENT8 iface, DWORD* pdwResolution)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_SetDefaultResolution (LPDIRECTMUSICSEGMENT8 iface, DWORD dwResolution)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_GetTrack (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, IDirectMusicTrack** ppTrack)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_GetTrackGroup (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicTrack* pTrack, DWORD* pdwGroupBits)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_InsertTrack (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicTrack* pTrack, DWORD dwGroupBits)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_RemoveTrack (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicTrack* pTrack)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_InitPlay (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicSegmentState** ppSegState, IDirectMusicPerformance* pPerformance, DWORD dwFlags)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_GetGraph (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicGraph** ppGraph)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_SetGraph (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicGraph* pGraph)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_AddNotificationType (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidNotificationType)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_RemoveNotificationType (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidNotificationType)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_GetParam (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_SetParam (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, void* pParam)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_Clone (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicSegment** ppSegment)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_SetStartPoint (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME mtStart)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_GetStartPoint (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME* pmtStart)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_SetLoopPoints (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_GetLoopPoints (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME* pmtStart, MUSIC_TIME* pmtEnd)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_SetPChannelsUsed (LPDIRECTMUSICSEGMENT8 iface, DWORD dwNumPChannels, DWORD* paPChannels)
{
	FIXME("stub\n");
	return DS_OK;
}

/* IDirectMusicSegment Interface part follow: */
HRESULT WINAPI IDirectMusicSegment8Impl_SetTrackConfig (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidTrackClassID, DWORD dwGroupBits, DWORD dwIndex, DWORD dwFlagsOn, DWORD dwFlagsOff)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_GetAudioPathConfig (LPDIRECTMUSICSEGMENT8 iface, IUnknown** ppAudioPathConfig)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_Compose (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME mtTime, IDirectMusicSegment* pFromSegment, IDirectMusicSegment* pToSegment, IDirectMusicSegment** ppComposedSegment)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_Download (LPDIRECTMUSICSEGMENT8 iface, IUnknown *pAudioPath)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_Unload (LPDIRECTMUSICSEGMENT8 iface, IUnknown *pAudioPath)
{
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectMusicSegment8) DirectMusicSegment8_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicSegment8Impl_QueryInterface,
	IDirectMusicSegment8Impl_AddRef,
	IDirectMusicSegment8Impl_Release,
	IDirectMusicSegment8Impl_GetLength,
	IDirectMusicSegment8Impl_SetLength,
	IDirectMusicSegment8Impl_GetRepeats,
	IDirectMusicSegment8Impl_SetRepeats,
	IDirectMusicSegment8Impl_GetDefaultResolution,
	IDirectMusicSegment8Impl_SetDefaultResolution,
	IDirectMusicSegment8Impl_GetTrack,
	IDirectMusicSegment8Impl_GetTrackGroup,
	IDirectMusicSegment8Impl_InsertTrack,
	IDirectMusicSegment8Impl_RemoveTrack,
	IDirectMusicSegment8Impl_InitPlay,
	IDirectMusicSegment8Impl_GetGraph,
	IDirectMusicSegment8Impl_SetGraph,
	IDirectMusicSegment8Impl_AddNotificationType,
	IDirectMusicSegment8Impl_RemoveNotificationType,
	IDirectMusicSegment8Impl_GetParam,
	IDirectMusicSegment8Impl_SetParam,
	IDirectMusicSegment8Impl_Clone,
	IDirectMusicSegment8Impl_SetStartPoint,
	IDirectMusicSegment8Impl_GetStartPoint,
	IDirectMusicSegment8Impl_SetLoopPoints,
	IDirectMusicSegment8Impl_GetLoopPoints,
	IDirectMusicSegment8Impl_SetPChannelsUsed,
	IDirectMusicSegment8Impl_SetTrackConfig,
	IDirectMusicSegment8Impl_GetAudioPathConfig,
	IDirectMusicSegment8Impl_Compose,
	IDirectMusicSegment8Impl_Download,
	IDirectMusicSegment8Impl_Unload
};


/* IDirectMusicSegmentState IUnknown parts follow: */
HRESULT WINAPI IDirectMusicSegmentStateImpl_QueryInterface (LPDIRECTMUSICSEGMENTSTATE iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicSegmentStateImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicSegmentState))
	{
		IDirectMusicSegmentStateImpl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicSegmentStateImpl_AddRef (LPDIRECTMUSICSEGMENTSTATE iface)
{
	ICOM_THIS(IDirectMusicSegmentStateImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicSegmentStateImpl_Release (LPDIRECTMUSICSEGMENTSTATE iface)
{
	ICOM_THIS(IDirectMusicSegmentStateImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicSegmentState Interface follow: */
HRESULT WINAPI IDirectMusicSegmentStateImpl_GetRepeats (LPDIRECTMUSICSEGMENTSTATE iface,  DWORD* pdwRepeats)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentStateImpl_GetSegment (LPDIRECTMUSICSEGMENTSTATE iface, IDirectMusicSegment** ppSegment)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentStateImpl_GetStartTime (LPDIRECTMUSICSEGMENTSTATE iface, MUSIC_TIME* pmtStart)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentStateImpl_GetSeek (LPDIRECTMUSICSEGMENTSTATE iface, MUSIC_TIME* pmtSeek)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentStateImpl_GetStartPoint (LPDIRECTMUSICSEGMENTSTATE iface, MUSIC_TIME* pmtStart)
{
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectMusicSegmentState) DirectMusicSegmentState_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicSegmentStateImpl_QueryInterface,
	IDirectMusicSegmentStateImpl_AddRef,
	IDirectMusicSegmentStateImpl_Release,
	IDirectMusicSegmentStateImpl_GetRepeats,
	IDirectMusicSegmentStateImpl_GetSegment,
	IDirectMusicSegmentStateImpl_GetStartTime,
	IDirectMusicSegmentStateImpl_GetSeek,
	IDirectMusicSegmentStateImpl_GetStartPoint
};

/* IDirectMusicSegmentState8 IUnknown parts follow: */
HRESULT WINAPI IDirectMusicSegmentState8Impl_QueryInterface (LPDIRECTMUSICSEGMENTSTATE8 iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicSegmentState8Impl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicSegmentState8))
	{
		IDirectMusicSegmentState8Impl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicSegmentState8Impl_AddRef (LPDIRECTMUSICSEGMENTSTATE8 iface)
{
	ICOM_THIS(IDirectMusicSegmentState8Impl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicSegmentState8Impl_Release (LPDIRECTMUSICSEGMENTSTATE8 iface)
{
	ICOM_THIS(IDirectMusicSegmentState8Impl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicSegmentState Interface part follow: */
HRESULT WINAPI IDirectMusicSegmentState8Impl_GetRepeats (LPDIRECTMUSICSEGMENTSTATE8 iface,  DWORD* pdwRepeats)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentState8Impl_GetSegment (LPDIRECTMUSICSEGMENTSTATE8 iface, IDirectMusicSegment** ppSegment)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentState8Impl_GetStartTime (LPDIRECTMUSICSEGMENTSTATE8 iface, MUSIC_TIME* pmtStart)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentState8Impl_GetSeek (LPDIRECTMUSICSEGMENTSTATE8 iface, MUSIC_TIME* pmtSeek)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentState8Impl_GetStartPoint (LPDIRECTMUSICSEGMENTSTATE8 iface, MUSIC_TIME* pmtStart)
{
	FIXME("stub\n");
	return DS_OK;
}

/* IDirectMusicSegmentState8 Interface part follow: */
HRESULT WINAPI IDirectMusicSegmentState8Impl_SetTrackConfig (LPDIRECTMUSICSEGMENTSTATE8 iface, REFGUID rguidTrackClassID, DWORD dwGroupBits, DWORD dwIndex, DWORD dwFlagsOn, DWORD dwFlagsOff)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSegmentState8Impl_GetObjectInPath (LPDIRECTMUSICSEGMENTSTATE8 iface, DWORD dwPChannel, DWORD dwStage, DWORD dwBuffer, REFGUID guidObject, DWORD dwIndex, REFGUID iidInterface, void** ppObject)
{
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectMusicSegmentState8) DirectMusicSegmentState8_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicSegmentState8Impl_QueryInterface,
	IDirectMusicSegmentState8Impl_AddRef,
	IDirectMusicSegmentState8Impl_Release,
	IDirectMusicSegmentState8Impl_GetRepeats,
	IDirectMusicSegmentState8Impl_GetSegment,
	IDirectMusicSegmentState8Impl_GetStartTime,
	IDirectMusicSegmentState8Impl_GetSeek,
	IDirectMusicSegmentState8Impl_GetStartPoint,
	IDirectMusicSegmentState8Impl_SetTrackConfig,
	IDirectMusicSegmentState8Impl_GetObjectInPath
};


/* IDirectMusicPatternTrack IUnknown parts follow: */
HRESULT WINAPI IDirectMusicPatternTrackImpl_QueryInterface (LPDIRECTMUSICPATTERNTRACK iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicPatternTrackImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicPatternTrack))
	{
		IDirectMusicPatternTrackImpl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicPatternTrackImpl_AddRef (LPDIRECTMUSICPATTERNTRACK iface)
{
	ICOM_THIS(IDirectMusicPatternTrackImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicPatternTrackImpl_Release (LPDIRECTMUSICPATTERNTRACK iface)
{
	ICOM_THIS(IDirectMusicPatternTrackImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicPatternTrack Interface follow: */
HRESULT WINAPI IDirectMusicPatternTrackImpl_CreateSegment (LPDIRECTMUSICPATTERNTRACK iface, IDirectMusicStyle* pStyle, IDirectMusicSegment** ppSegment)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPatternTrackImpl_SetVariation (LPDIRECTMUSICPATTERNTRACK iface, IDirectMusicSegmentState* pSegState, DWORD dwVariationFlags, DWORD dwPart)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPatternTrackImpl_SetPatternByName (LPDIRECTMUSICPATTERNTRACK iface, IDirectMusicSegmentState* pSegState, WCHAR* wszName, IDirectMusicStyle* pStyle, DWORD dwPatternType, DWORD* pdwLength)
{
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectMusicPatternTrack) DirectMusicPatternTrack_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicPatternTrackImpl_QueryInterface,
	IDirectMusicPatternTrackImpl_AddRef,
	IDirectMusicPatternTrackImpl_Release,
	IDirectMusicPatternTrackImpl_CreateSegment,
	IDirectMusicPatternTrackImpl_SetVariation,
	IDirectMusicPatternTrackImpl_SetPatternByName
};
