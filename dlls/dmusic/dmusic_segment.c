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


/* IDirectMusicSegment8 IUnknown parts follow: */
HRESULT WINAPI IDirectMusicSegment8Impl_QueryInterface (LPDIRECTMUSICSEGMENT8 iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || 
	    IsEqualGUID(riid, &IID_IDirectMusicSegment) ||
	    IsEqualGUID(riid, &IID_IDirectMusicSegment8))
	{
		IDirectMusicSegment8Impl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
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
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pmtLength);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_SetLength (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME mtLength)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %ld): stub\n", This, mtLength);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_GetRepeats (LPDIRECTMUSICSEGMENT8 iface, DWORD* pdwRepeats)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pdwRepeats);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_SetRepeats (LPDIRECTMUSICSEGMENT8 iface, DWORD dwRepeats)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %ld): stub\n", This, dwRepeats);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_GetDefaultResolution (LPDIRECTMUSICSEGMENT8 iface, DWORD* pdwResolution)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pdwResolution);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_SetDefaultResolution (LPDIRECTMUSICSEGMENT8 iface, DWORD dwResolution)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %ld): stub\n", This, dwResolution);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_GetTrack (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, IDirectMusicTrack** ppTrack)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %s, %ld, %ld, %p): stub\n", This, debugstr_guid(rguidType), dwGroupBits, dwIndex, ppTrack);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_GetTrackGroup (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicTrack* pTrack, DWORD* pdwGroupBits)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %p, %p): stub\n", This, pTrack, pdwGroupBits);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_InsertTrack (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicTrack* pTrack, DWORD dwGroupBits)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %p, %ld): stub\n", This, pTrack, dwGroupBits);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_RemoveTrack (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicTrack* pTrack)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pTrack);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_InitPlay (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicSegmentState** ppSegState, IDirectMusicPerformance* pPerformance, DWORD dwFlags)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %p, %p, %ld): stub\n", This, ppSegState, pPerformance, dwFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_GetGraph (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicGraph** ppGraph)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %p): stub\n", This, ppGraph);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_SetGraph (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicGraph* pGraph)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pGraph);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_AddNotificationType (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidNotificationType)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_RemoveNotificationType (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidNotificationType)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_GetParam (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %s, %ld, %ld, %ld, %p, %p): stub\n", This, debugstr_guid(rguidType), dwGroupBits, dwIndex, mtTime, pmtNext, pParam);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_SetParam (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, void* pParam)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %s, %ld, %ld, %ld, %p): stub\n", This, debugstr_guid(rguidType), dwGroupBits, dwIndex, mtTime, pParam);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_Clone (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicSegment** ppSegment)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppSegment);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_SetStartPoint (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME mtStart)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %ld): stub\n", This, mtStart);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_GetStartPoint (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME* pmtStart)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pmtStart);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_SetLoopPoints (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %ld, %ld): stub\n", This, mtStart, mtEnd);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_GetLoopPoints (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME* pmtStart, MUSIC_TIME* pmtEnd)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %p, %p): stub\n", This, pmtStart, pmtEnd);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_SetPChannelsUsed (LPDIRECTMUSICSEGMENT8 iface, DWORD dwNumPChannels, DWORD* paPChannels)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %ld, %p): stub\n", This, dwNumPChannels, paPChannels);	

	return S_OK;
}

/* IDirectMusicSegment Interface part follow: */
HRESULT WINAPI IDirectMusicSegment8Impl_SetTrackConfig (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidTrackClassID, DWORD dwGroupBits, DWORD dwIndex, DWORD dwFlagsOn, DWORD dwFlagsOff)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %s, %ld, %ld, %ld, %ld): stub\n", This, debugstr_guid(rguidTrackClassID), dwGroupBits, dwIndex, dwFlagsOn, dwFlagsOff);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_GetAudioPathConfig (LPDIRECTMUSICSEGMENT8 iface, IUnknown** ppAudioPathConfig)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %p): stub\n", This, ppAudioPathConfig);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_Compose (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME mtTime, IDirectMusicSegment* pFromSegment, IDirectMusicSegment* pToSegment, IDirectMusicSegment** ppComposedSegment)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %ld, %p, %p, %p): stub\n", This, mtTime, pFromSegment, pToSegment, ppComposedSegment);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_Download (LPDIRECTMUSICSEGMENT8 iface, IUnknown *pAudioPath)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pAudioPath);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_Unload (LPDIRECTMUSICSEGMENT8 iface, IUnknown *pAudioPath)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pAudioPath);

	return S_OK;
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


/* IDirectMusicSegmentState8 IUnknown part follow: */
HRESULT WINAPI IDirectMusicSegmentState8Impl_QueryInterface (LPDIRECTMUSICSEGMENTSTATE8 iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicSegmentState8Impl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || 
	    IsEqualGUID(riid, &IID_IDirectMusicSegmentState) ||
	    IsEqualGUID(riid, &IID_IDirectMusicSegmentState8))
	{
		IDirectMusicSegmentState8Impl_AddRef(iface);
		*ppobj = This;
		return S_OK;
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
	ICOM_THIS(IDirectMusicSegmentState8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pdwRepeats);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegmentState8Impl_GetSegment (LPDIRECTMUSICSEGMENTSTATE8 iface, IDirectMusicSegment** ppSegment)
{
	ICOM_THIS(IDirectMusicSegmentState8Impl,iface);

	FIXME("(%p, %p): stub\n", This, ppSegment);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegmentState8Impl_GetStartTime (LPDIRECTMUSICSEGMENTSTATE8 iface, MUSIC_TIME* pmtStart)
{
	ICOM_THIS(IDirectMusicSegmentState8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pmtStart);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegmentState8Impl_GetSeek (LPDIRECTMUSICSEGMENTSTATE8 iface, MUSIC_TIME* pmtSeek)
{
	ICOM_THIS(IDirectMusicSegmentState8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pmtSeek);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegmentState8Impl_GetStartPoint (LPDIRECTMUSICSEGMENTSTATE8 iface, MUSIC_TIME* pmtStart)
{
	ICOM_THIS(IDirectMusicSegmentState8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pmtStart);

	return S_OK;
}

/* IDirectMusicSegmentState8 Interface part follow: */
HRESULT WINAPI IDirectMusicSegmentState8Impl_SetTrackConfig (LPDIRECTMUSICSEGMENTSTATE8 iface, REFGUID rguidTrackClassID, DWORD dwGroupBits, DWORD dwIndex, DWORD dwFlagsOn, DWORD dwFlagsOff)
{
	ICOM_THIS(IDirectMusicSegmentState8Impl,iface);

	FIXME("(%p, %s, %ld, %ld, %ld, %ld): stub\n", This, debugstr_guid(rguidTrackClassID), dwGroupBits, dwIndex, dwFlagsOn, dwFlagsOff);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegmentState8Impl_GetObjectInPath (LPDIRECTMUSICSEGMENTSTATE8 iface, DWORD dwPChannel, DWORD dwStage, DWORD dwBuffer, REFGUID guidObject, DWORD dwIndex, REFGUID iidInterface, void** ppObject)
{
	ICOM_THIS(IDirectMusicSegmentState8Impl,iface);

	FIXME("(%p, %ld, %ld, %ld, %s, %ld, %s, %p): stub\n", This, dwPChannel, dwStage, dwBuffer, debugstr_guid(guidObject), dwIndex, debugstr_guid(iidInterface), ppObject);

	return S_OK;
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

	if (IsEqualGUID(riid, &IID_IUnknown) || 
	    IsEqualGUID(riid, &IID_IDirectMusicPatternTrack))
	{
		IDirectMusicPatternTrackImpl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
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
	ICOM_THIS(IDirectMusicPatternTrackImpl,iface);

	FIXME("(%p, %p, %p): stub\n", This, pStyle, ppSegment);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPatternTrackImpl_SetVariation (LPDIRECTMUSICPATTERNTRACK iface, IDirectMusicSegmentState* pSegState, DWORD dwVariationFlags, DWORD dwPart)
{
	ICOM_THIS(IDirectMusicPatternTrackImpl,iface);

	FIXME("(%p, %p, %ld, %ld): stub\n", This, pSegState, dwVariationFlags, dwPart);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPatternTrackImpl_SetPatternByName (LPDIRECTMUSICPATTERNTRACK iface, IDirectMusicSegmentState* pSegState, WCHAR* wszName, IDirectMusicStyle* pStyle, DWORD dwPatternType, DWORD* pdwLength)
{
	ICOM_THIS(IDirectMusicPatternTrackImpl,iface);

	FIXME("(%p, %p, %p, %p, %ld, %p): stub\n", This, pSegState, wszName, pStyle, dwPatternType, pdwLength);

	return S_OK;
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
