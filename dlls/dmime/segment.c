/* IDirectMusicSegment8 Implementation
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
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

#include "dmime_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmime);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

/*****************************************************************************
 * IDirectMusicSegmentImpl implementation
 */
/* IDirectMusicSegment IUnknown part: */
HRESULT WINAPI IDirectMusicSegment8Impl_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, UnknownVtbl, iface);
	
	if (IsEqualIID (riid, &IID_IUnknown)) {
		*ppobj = (LPVOID)&This->UnknownVtbl;
		IDirectMusicSegment8Impl_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
		return S_OK;	
	} else if (IsEqualIID (riid, &IID_IDirectMusicSegment)
	|| IsEqualIID (riid, &IID_IDirectMusicSegment2)
	|| IsEqualIID (riid, &IID_IDirectMusicSegment8)) {
		*ppobj = (LPVOID)&This->SegmentVtbl;
		IDirectMusicSegment8Impl_IDirectMusicSegment8_AddRef ((LPDIRECTMUSICSEGMENT8)&This->SegmentVtbl);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IDirectMusicObject)) {
		*ppobj = (LPVOID)&This->ObjectVtbl;
		IDirectMusicSegment8Impl_IDirectMusicObject_AddRef ((LPDIRECTMUSICOBJECT)&This->ObjectVtbl);		
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IPersistStream)) {
		*ppobj = (LPVOID)&This->PersistStreamVtbl;
		IDirectMusicSegment8Impl_IPersistStream_AddRef ((LPPERSISTSTREAM)&This->PersistStreamVtbl);		
		return S_OK;
	}
	
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicSegment8Impl_IUnknown_AddRef (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, UnknownVtbl, iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicSegment8Impl_IUnknown_Release (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, UnknownVtbl, iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

ICOM_VTABLE(IUnknown) DirectMusicSegment8_Unknown_Vtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicSegment8Impl_IUnknown_QueryInterface,
	IDirectMusicSegment8Impl_IUnknown_AddRef,
	IDirectMusicSegment8Impl_IUnknown_Release
};

/* IDirectMusicSegmentImpl IDirectMusicSegment part: */
HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_QueryInterface (LPDIRECTMUSICSEGMENT8 iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	return IDirectMusicSegment8Impl_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

ULONG WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_AddRef (LPDIRECTMUSICSEGMENT8 iface) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	return IDirectMusicSegment8Impl_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

ULONG WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_Release (LPDIRECTMUSICSEGMENT8 iface) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	return IDirectMusicSegment8Impl_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_GetLength (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME* pmtLength) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	FIXME("(%p, %p): stub\n", This, pmtLength);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_SetLength (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME mtLength) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	FIXME("(%p, %ld): stub\n", This, mtLength);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_GetRepeats (LPDIRECTMUSICSEGMENT8 iface, DWORD* pdwRepeats) { 
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	FIXME("(%p, %p): stub\n", This, pdwRepeats);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_SetRepeats (LPDIRECTMUSICSEGMENT8 iface, DWORD dwRepeats) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	FIXME("(%p, %ld): stub\n", This, dwRepeats);	
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_GetDefaultResolution (LPDIRECTMUSICSEGMENT8 iface, DWORD* pdwResolution) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	FIXME("(%p, %p): stub\n", This, pdwResolution);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_SetDefaultResolution (LPDIRECTMUSICSEGMENT8 iface, DWORD dwResolution) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	FIXME("(%p, %ld): stub\n", This, dwResolution);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_GetTrack (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, IDirectMusicTrack** ppTrack) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	FIXME("(%p, %s, %ld, %ld, %p): stub\n", This, debugstr_guid(rguidType), dwGroupBits, dwIndex, ppTrack);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_GetTrackGroup (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicTrack* pTrack, DWORD* pdwGroupBits) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	FIXME("(%p, %p, %p): stub\n", This, pTrack, pdwGroupBits);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_InsertTrack (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicTrack* pTrack, DWORD dwGroupBits) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	FIXME("(%p, %p, %ld): stub\n", This, pTrack, dwGroupBits);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_RemoveTrack (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicTrack* pTrack) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	FIXME("(%p, %p): stub\n", This, pTrack);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_InitPlay (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicSegmentState** ppSegState, IDirectMusicPerformance* pPerformance, DWORD dwFlags) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	FIXME("(%p, %p, %p, %ld): stub\n", This, ppSegState, pPerformance, dwFlags);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_GetGraph (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicGraph** ppGraph) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	FIXME("(%p, %p): stub\n", This, ppGraph);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_SetGraph (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicGraph* pGraph) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	FIXME("(%p, %p): stub\n", This, pGraph);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_AddNotificationType (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidNotificationType) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_RemoveNotificationType (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidNotificationType) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_GetParam (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	FIXME("(%p, %s, %ld, %ld, %ld, %p, %p): stub\n", This, debugstr_guid(rguidType), dwGroupBits, dwIndex, mtTime, pmtNext, pParam);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_SetParam (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, void* pParam) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	FIXME("(%p, %s, %ld, %ld, %ld, %p): stub\n", This, debugstr_guid(rguidType), dwGroupBits, dwIndex, mtTime, pParam);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_Clone (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicSegment** ppSegment) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppSegment);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_SetStartPoint (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME mtStart) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	FIXME("(%p, %ld): stub\n", This, mtStart);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_GetStartPoint (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME* pmtStart) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	FIXME("(%p, %p): stub\n", This, pmtStart);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_SetLoopPoints (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	FIXME("(%p, %ld, %ld): stub\n", This, mtStart, mtEnd);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_GetLoopPoints (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME* pmtStart, MUSIC_TIME* pmtEnd) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	FIXME("(%p, %p, %p): stub\n", This, pmtStart, pmtEnd);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_SetPChannelsUsed (LPDIRECTMUSICSEGMENT8 iface, DWORD dwNumPChannels, DWORD* paPChannels) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	FIXME("(%p, %ld, %p): stub\n", This, dwNumPChannels, paPChannels);	
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_SetTrackConfig (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidTrackClassID, DWORD dwGroupBits, DWORD dwIndex, DWORD dwFlagsOn, DWORD dwFlagsOff) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	FIXME("(%p, %s, %ld, %ld, %ld, %ld): stub\n", This, debugstr_guid(rguidTrackClassID), dwGroupBits, dwIndex, dwFlagsOn, dwFlagsOff);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_GetAudioPathConfig (LPDIRECTMUSICSEGMENT8 iface, IUnknown** ppAudioPathConfig){
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	FIXME("(%p, %p): stub\n", This, ppAudioPathConfig);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_Compose (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME mtTime, IDirectMusicSegment* pFromSegment, IDirectMusicSegment* pToSegment, IDirectMusicSegment** ppComposedSegment) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	FIXME("(%p, %ld, %p, %p, %p): stub\n", This, mtTime, pFromSegment, pToSegment, ppComposedSegment);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_Download (LPDIRECTMUSICSEGMENT8 iface, IUnknown *pAudioPath) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	FIXME("(%p, %p): stub\n", This, pAudioPath);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicSegment8_Unload (LPDIRECTMUSICSEGMENT8 iface, IUnknown *pAudioPath) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, SegmentVtbl, iface);
	FIXME("(%p, %p): stub\n", This, pAudioPath);
	return S_OK;
}

ICOM_VTABLE(IDirectMusicSegment8) DirectMusicSegment8_Segment_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicSegment8Impl_IDirectMusicSegment8_QueryInterface,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_AddRef,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_Release,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_GetLength,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_SetLength,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_GetRepeats,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_SetRepeats,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_GetDefaultResolution,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_SetDefaultResolution,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_GetTrack,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_GetTrackGroup,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_InsertTrack,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_RemoveTrack,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_InitPlay,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_GetGraph,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_SetGraph,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_AddNotificationType,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_RemoveNotificationType,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_GetParam,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_SetParam,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_Clone,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_SetStartPoint,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_GetStartPoint,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_SetLoopPoints,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_GetLoopPoints,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_SetPChannelsUsed,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_SetTrackConfig,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_GetAudioPathConfig,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_Compose,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_Download,
	IDirectMusicSegment8Impl_IDirectMusicSegment8_Unload
};

/* IDirectMusicSegment8Impl IDirectMusicObject part: */
HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, ObjectVtbl, iface);
	return IDirectMusicSegment8Impl_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

ULONG WINAPI IDirectMusicSegment8Impl_IDirectMusicObject_AddRef (LPDIRECTMUSICOBJECT iface) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, ObjectVtbl, iface);
	return IDirectMusicSegment8Impl_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

ULONG WINAPI IDirectMusicSegment8Impl_IDirectMusicObject_Release (LPDIRECTMUSICOBJECT iface) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, ObjectVtbl, iface);
	return IDirectMusicSegment8Impl_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, ObjectVtbl, iface);
	TRACE("(%p, %p)\n", This, pDesc);
	/* I think we shouldn't return pointer here since then values can be changed; it'd be a mess */
	memcpy (pDesc, This->pDesc, This->pDesc->dwSize);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, ObjectVtbl, iface);
	TRACE("(%p, %p): setting descriptor:\n", This, pDesc);
	if (TRACE_ON(dmime)) {
		DMUSIC_dump_DMUS_OBJECTDESC (pDesc);
	}
	
	/* According to MSDN, we should copy only given values, not whole struct */	
	if (pDesc->dwValidData & DMUS_OBJ_OBJECT)
		memcpy (&This->pDesc->guidObject, &pDesc->guidObject, sizeof (pDesc->guidObject));
	if (pDesc->dwValidData & DMUS_OBJ_CLASS)
		memcpy (&This->pDesc->guidClass, &pDesc->guidClass, sizeof (pDesc->guidClass));		
	if (pDesc->dwValidData & DMUS_OBJ_NAME)
		strncpyW (This->pDesc->wszName, pDesc->wszName, DMUS_MAX_NAME);
	if (pDesc->dwValidData & DMUS_OBJ_CATEGORY)
		strncpyW (This->pDesc->wszCategory, pDesc->wszCategory, DMUS_MAX_CATEGORY);		
	if (pDesc->dwValidData & DMUS_OBJ_FILENAME)
		strncpyW (This->pDesc->wszFileName, pDesc->wszFileName, DMUS_MAX_FILENAME);		
	if (pDesc->dwValidData & DMUS_OBJ_VERSION)
		memcpy (&This->pDesc->vVersion, &pDesc->vVersion, sizeof (pDesc->vVersion));				
	if (pDesc->dwValidData & DMUS_OBJ_DATE)
		memcpy (&This->pDesc->ftDate, &pDesc->ftDate, sizeof (pDesc->ftDate));				
	if (pDesc->dwValidData & DMUS_OBJ_MEMORY) {
		memcpy (&This->pDesc->llMemLength, &pDesc->llMemLength, sizeof (pDesc->llMemLength));				
		memcpy (This->pDesc->pbMemData, pDesc->pbMemData, sizeof (pDesc->pbMemData));
	}
	if (pDesc->dwValidData & DMUS_OBJ_STREAM) {
		/* according to MSDN, we copy the stream */
		IStream_Clone (pDesc->pStream, &This->pDesc->pStream);	
	}
	
	/* add new flags */
	This->pDesc->dwValidData |= pDesc->dwValidData;

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IDirectMusicObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, ObjectVtbl, iface);
	DMUS_PRIVATE_CHUNK Chunk;
	DWORD StreamSize, StreamCount, ListSize[1], ListCount[1];
	LARGE_INTEGER liMove; /* used when skipping chunks */

	TRACE("(%p,%p, %p)\n", This, pStream, pDesc);
	
	/* FIXME: should this be determined from stream? */
	pDesc->dwValidData |= DMUS_OBJ_CLASS;
	memcpy (&pDesc->guidClass, &CLSID_DirectMusicSegment, sizeof(CLSID));
	
	IStream_Read (pStream, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
	TRACE_(dmfile)(": %s chunk (size = 0x%04lx)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
	switch (Chunk.fccID) {	
		case FOURCC_RIFF: {
			IStream_Read (pStream, &Chunk.fccID, sizeof(FOURCC), NULL);				
			TRACE_(dmfile)(": RIFF chunk of type %s", debugstr_fourcc(Chunk.fccID));
			StreamSize = Chunk.dwSize - sizeof(FOURCC);
			StreamCount = 0;
			if (Chunk.fccID == DMUS_FOURCC_SEGMENT_FORM) {
				TRACE_(dmfile)(": segment form\n");
				do {
					IStream_Read (pStream, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
					StreamCount += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
					TRACE_(dmfile)(": %s chunk (size = 0x%04lx)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
					switch (Chunk.fccID) {
						case DMUS_FOURCC_GUID_CHUNK: {
							TRACE_(dmfile)(": GUID chunk\n");
							pDesc->dwValidData |= DMUS_OBJ_OBJECT;
							IStream_Read (pStream, &pDesc->guidObject, Chunk.dwSize, NULL);
							break;
						}
						case DMUS_FOURCC_VERSION_CHUNK: {
							TRACE_(dmfile)(": version chunk\n");
							pDesc->dwValidData |= DMUS_OBJ_VERSION;
							IStream_Read (pStream, &pDesc->vVersion, Chunk.dwSize, NULL);
							break;
						}
						case DMUS_FOURCC_CATEGORY_CHUNK: {
							TRACE_(dmfile)(": category chunk\n");
							pDesc->dwValidData |= DMUS_OBJ_CATEGORY;
							IStream_Read (pStream, pDesc->wszCategory, Chunk.dwSize, NULL);
							break;
						}
						case FOURCC_LIST: {
							IStream_Read (pStream, &Chunk.fccID, sizeof(FOURCC), NULL);				
							TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(Chunk.fccID));
							ListSize[0] = Chunk.dwSize - sizeof(FOURCC);
							ListCount[0] = 0;
							switch (Chunk.fccID) {
								/* evil M$ UNFO list, which can (!?) contain INFO elements */
								case DMUS_FOURCC_UNFO_LIST: {
									TRACE_(dmfile)(": UNFO list\n");
									do {
										IStream_Read (pStream, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
										ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
										TRACE_(dmfile)(": %s chunk (size = 0x%04lx)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
										switch (Chunk.fccID) {
											/* don't ask me why, but M$ puts INFO elements in UNFO list sometimes
                                             (though strings seem to be valid unicode) */
											case mmioFOURCC('I','N','A','M'):
											case DMUS_FOURCC_UNAM_CHUNK: {
												TRACE_(dmfile)(": name chunk\n");
												pDesc->dwValidData |= DMUS_OBJ_NAME;
												IStream_Read (pStream, pDesc->wszName, Chunk.dwSize, NULL);
												break;
											}
											case mmioFOURCC('I','A','R','T'):
											case DMUS_FOURCC_UART_CHUNK: {
												TRACE_(dmfile)(": artist chunk (ignored)\n");
												liMove.QuadPart = Chunk.dwSize;
												IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL);
												break;
											}
											case mmioFOURCC('I','C','O','P'):
											case DMUS_FOURCC_UCOP_CHUNK: {
												TRACE_(dmfile)(": copyright chunk (ignored)\n");
												liMove.QuadPart = Chunk.dwSize;
												IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL);
												break;
											}
											case mmioFOURCC('I','S','B','J'):
											case DMUS_FOURCC_USBJ_CHUNK: {
												TRACE_(dmfile)(": subject chunk (ignored)\n");
												liMove.QuadPart = Chunk.dwSize;
												IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL);
												break;
											}
											case mmioFOURCC('I','C','M','T'):
											case DMUS_FOURCC_UCMT_CHUNK: {
												TRACE_(dmfile)(": comment chunk (ignored)\n");
												liMove.QuadPart = Chunk.dwSize;
												IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL);
												break;
											}
											default: {
												TRACE_(dmfile)(": unknown chunk (irrevelant & skipping)\n");
												liMove.QuadPart = Chunk.dwSize;
												IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL);
												break;						
											}
										}
										TRACE_(dmfile)(": ListCount[0] = %ld < ListSize[0] = %ld\n", ListCount[0], ListSize[0]);
									} while (ListCount[0] < ListSize[0]);
									break;
								}
								default: {
									TRACE_(dmfile)(": unknown (skipping)\n");
									liMove.QuadPart = Chunk.dwSize - sizeof(FOURCC);
									IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL);
									break;						
								}
							}
							break;
						}	
						default: {
							TRACE_(dmfile)(": unknown chunk (irrevelant & skipping)\n");
							liMove.QuadPart = Chunk.dwSize;
							IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL);
							break;						
						}
					}
					TRACE_(dmfile)(": StreamCount[0] = %ld < StreamSize[0] = %ld\n", StreamCount, StreamSize);
				} while (StreamCount < StreamSize);
				break;
			} else {
				TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
				liMove.QuadPart = StreamSize;
				IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
				return E_FAIL;
			}
		
			TRACE_(dmfile)(": reading finished\n");
			break;
		}
		default: {
			TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
			liMove.QuadPart = Chunk.dwSize;
			IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
			return DMUS_E_INVALIDFILE;
		}
	}	
	
	TRACE(": returning descriptor:\n");
	if (TRACE_ON(dmime)) {
		DMUSIC_dump_DMUS_OBJECTDESC (pDesc);
	}
	
	return S_OK;
}

ICOM_VTABLE(IDirectMusicObject) DirectMusicSegment8_Object_Vtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicSegment8Impl_IDirectMusicObject_QueryInterface,
	IDirectMusicSegment8Impl_IDirectMusicObject_AddRef,
	IDirectMusicSegment8Impl_IDirectMusicObject_Release,
	IDirectMusicSegment8Impl_IDirectMusicObject_GetDescriptor,
	IDirectMusicSegment8Impl_IDirectMusicObject_SetDescriptor,
	IDirectMusicSegment8Impl_IDirectMusicObject_ParseDescriptor
};

/* IDirectMusicSegment8Impl IPersistStream part: */
HRESULT WINAPI IDirectMusicSegment8Impl_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, PersistStreamVtbl, iface);
	return IDirectMusicSegment8Impl_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

ULONG WINAPI IDirectMusicSegment8Impl_IPersistStream_AddRef (LPPERSISTSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, PersistStreamVtbl, iface);
	return IDirectMusicSegment8Impl_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

ULONG WINAPI IDirectMusicSegment8Impl_IPersistStream_Release (LPPERSISTSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, PersistStreamVtbl, iface);
	return IDirectMusicSegment8Impl_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

HRESULT WINAPI IDirectMusicSegment8Impl_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID) {
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IPersistStream_IsDirty (LPPERSISTSTREAM iface) {
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm) {
	ICOM_THIS_MULTI(IDirectMusicSegment8Impl, PersistStreamVtbl, iface);
	FOURCC chunkID;
	DWORD chunkSize, StreamSize, StreamCount, ListSize[3], ListCount[3];
	LARGE_INTEGER liMove; /* used when skipping chunks */

	FIXME("(%p, %p): Loading not implemented yet\n", This, pStm);
	IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
	IStream_Read (pStm, &chunkSize, sizeof(DWORD), NULL);
	TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (chunkID), chunkSize);
	switch (chunkID) {	
		case FOURCC_RIFF: {
			IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);				
			TRACE_(dmfile)(": RIFF chunk of type %s", debugstr_fourcc(chunkID));
			StreamSize = chunkSize - sizeof(FOURCC);
			StreamCount = 0;
			switch (chunkID) {
				case DMUS_FOURCC_SEGMENT_FORM: {
					TRACE_(dmfile)(": segment form\n");
					do {
						IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
						IStream_Read (pStm, &chunkSize, sizeof(FOURCC), NULL);
						StreamCount += sizeof(FOURCC) + sizeof(DWORD) + chunkSize;
						TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (chunkID), chunkSize);
						switch (chunkID) {
							case DMUS_FOURCC_GUID_CHUNK: {
								TRACE_(dmfile)(": GUID chunk\n");
								This->pDesc->dwValidData |= DMUS_OBJ_OBJECT;
								IStream_Read (pStm, &This->pDesc->guidObject, chunkSize, NULL);
								break;
							}
							case DMUS_FOURCC_VERSION_CHUNK: {
								TRACE_(dmfile)(": version chunk\n");
								This->pDesc->dwValidData |= DMUS_OBJ_VERSION;
								IStream_Read (pStm, &This->pDesc->vVersion, chunkSize, NULL);
								break;
							}
							case DMUS_FOURCC_CATEGORY_CHUNK: {
								TRACE_(dmfile)(": category chunk\n");
								This->pDesc->dwValidData |= DMUS_OBJ_CATEGORY;
								IStream_Read (pStm, This->pDesc->wszCategory, chunkSize, NULL);
								break;
							}
							case FOURCC_LIST: {
								IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);				
								TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(chunkID));
								ListSize[0] = chunkSize - sizeof(FOURCC);
								ListCount[0] = 0;
								switch (chunkID) {
									case DMUS_FOURCC_UNFO_LIST: {
										TRACE_(dmfile)(": UNFO list\n");
										do {
											IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
											IStream_Read (pStm, &chunkSize, sizeof(FOURCC), NULL);
											ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + chunkSize;
											TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (chunkID), chunkSize);
											switch (chunkID) {
												/* don't ask me why, but M$ puts INFO elements in UNFO list sometimes
                                              (though strings seem to be valid unicode) */
												case mmioFOURCC('I','N','A','M'):
												case DMUS_FOURCC_UNAM_CHUNK: {
													TRACE_(dmfile)(": name chunk\n");
													This->pDesc->dwValidData |= DMUS_OBJ_NAME;
													IStream_Read (pStm, This->pDesc->wszName, chunkSize, NULL);
													break;
												}
												case mmioFOURCC('I','A','R','T'):
												case DMUS_FOURCC_UART_CHUNK: {
													TRACE_(dmfile)(": artist chunk (ignored)\n");
													liMove.QuadPart = chunkSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												case mmioFOURCC('I','C','O','P'):
												case DMUS_FOURCC_UCOP_CHUNK: {
													TRACE_(dmfile)(": copyright chunk (ignored)\n");
													liMove.QuadPart = chunkSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												case mmioFOURCC('I','S','B','J'):
												case DMUS_FOURCC_USBJ_CHUNK: {
													TRACE_(dmfile)(": subject chunk (ignored)\n");
													liMove.QuadPart = chunkSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												case mmioFOURCC('I','C','M','T'):
												case DMUS_FOURCC_UCMT_CHUNK: {
													TRACE_(dmfile)(": comment chunk (ignored)\n");
													liMove.QuadPart = chunkSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												default: {
													TRACE_(dmfile)(": unknown chunk (irrevelant & skipping)\n");
													liMove.QuadPart = chunkSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;						
												}
											}
											TRACE_(dmfile)(": ListCount[0] = %ld < ListSize[0] = %ld\n", ListCount[0], ListSize[0]);
										} while (ListCount[0] < ListSize[0]);
										break;
									}
									default: {
										TRACE_(dmfile)(": unknown (skipping)\n");
										liMove.QuadPart = chunkSize - sizeof(FOURCC);
										IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
										break;						
									}
								}
								break;
							}	
							default: {
								TRACE_(dmfile)(": unknown chunk (irrevelant & skipping)\n");
								liMove.QuadPart = chunkSize;
								IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
								break;						
							}
						}
						TRACE_(dmfile)(": StreamCount[0] = %ld < StreamSize[0] = %ld\n", StreamCount, StreamSize);
					} while (StreamCount < StreamSize);
					break;
				}
				default: {
					TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
					liMove.QuadPart = StreamSize;
					IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
					return E_FAIL;
				}
			}
			TRACE_(dmfile)(": reading finished\n");
			break;
		}
		default: {
			TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
			liMove.QuadPart = chunkSize;
			IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
			return E_FAIL;
		}
	}

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty) {
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicSegment8Impl_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize) {
	return E_NOTIMPL;
}

ICOM_VTABLE(IPersistStream) DirectMusicSegment8_PersistStream_Vtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicSegment8Impl_IPersistStream_QueryInterface,
	IDirectMusicSegment8Impl_IPersistStream_AddRef,
	IDirectMusicSegment8Impl_IPersistStream_Release,
	IDirectMusicSegment8Impl_IPersistStream_GetClassID,
	IDirectMusicSegment8Impl_IPersistStream_IsDirty,
	IDirectMusicSegment8Impl_IPersistStream_Load,
	IDirectMusicSegment8Impl_IPersistStream_Save,
	IDirectMusicSegment8Impl_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicSegmentImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter) {
	IDirectMusicSegment8Impl* obj;
	
	obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicSegment8Impl));
	if (NULL == obj) {
		*ppobj = (LPVOID) NULL;
		return E_OUTOFMEMORY;
	}
	obj->UnknownVtbl = &DirectMusicSegment8_Unknown_Vtbl;
	obj->SegmentVtbl = &DirectMusicSegment8_Segment_Vtbl;
	obj->ObjectVtbl = &DirectMusicSegment8_Object_Vtbl;
	obj->PersistStreamVtbl = &DirectMusicSegment8_PersistStream_Vtbl;
	obj->pDesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DMUS_OBJECTDESC));
	DM_STRUCT_INIT(obj->pDesc);
	obj->pDesc->dwValidData |= DMUS_OBJ_CLASS;
	memcpy (&obj->pDesc->guidClass, &CLSID_DirectMusicSegment, sizeof (CLSID));
	obj->ref = 0; /* will be inited by QueryInterface */
	
	return IDirectMusicSegment8Impl_IUnknown_QueryInterface ((LPUNKNOWN)&obj->UnknownVtbl, lpcGUID, ppobj);
}
