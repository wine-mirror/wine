/* IDirectMusicSegment8 Implementation
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "dmime_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmime);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

/*****************************************************************************
 * IDirectMusicSegment8Impl implementation
 */
/* IDirectMusicSegment8 IUnknown part: */
HRESULT WINAPI IDirectMusicSegment8Impl_QueryInterface (LPDIRECTMUSICSEGMENT8 iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	if (IsEqualIID (riid, &IID_IUnknown) || 
	    IsEqualIID (riid, &IID_IDirectMusicSegment) ||
	    IsEqualIID (riid, &IID_IDirectMusicSegment8)) {
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
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicSegment8 IDirectMusicSegment part: */
HRESULT WINAPI IDirectMusicSegment8Impl_GetLength (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME* pmtLength)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	TRACE("(%p, %p)\n", This, pmtLength);
	*pmtLength = This->segHeader.mtLength;

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_SetLength (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME mtLength)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	TRACE("(%p, %ld)\n", This, mtLength);
	This->segHeader.mtLength = mtLength;

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_GetRepeats (LPDIRECTMUSICSEGMENT8 iface, DWORD* pdwRepeats)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	TRACE("(%p, %p)\n", This, pdwRepeats);
	*pdwRepeats = This->segHeader.dwRepeats;

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_SetRepeats (LPDIRECTMUSICSEGMENT8 iface, DWORD dwRepeats)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	TRACE("(%p, %ld)\n", This, dwRepeats);
	This->segHeader.dwRepeats = dwRepeats;
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_GetDefaultResolution (LPDIRECTMUSICSEGMENT8 iface, DWORD* pdwResolution)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	TRACE("(%p, %p)\n", This, pdwResolution);
	*pdwResolution = This->segHeader.dwResolution;
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_SetDefaultResolution (LPDIRECTMUSICSEGMENT8 iface, DWORD dwResolution)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	TRACE("(%p, %ld)\n", This, dwResolution);
	This->segHeader.dwResolution = dwResolution;
	
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

	TRACE("(%p, %ld): stub\n", This, mtStart);
	This->segHeader.mtPlayStart = mtStart;
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_GetStartPoint (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME* pmtStart)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	TRACE("(%p, %p): stub\n", This, pmtStart);
	*pmtStart = This->segHeader.mtPlayStart;

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_SetLoopPoints (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	TRACE("(%p, %ld, %ld): stub\n", This, mtStart, mtEnd);
	This->segHeader.mtLoopStart = mtStart;
	This->segHeader.mtLoopEnd = mtEnd;
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_GetLoopPoints (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME* pmtStart, MUSIC_TIME* pmtEnd)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	TRACE("(%p, %p, %p): stub\n", This, pmtStart, pmtEnd);
	*pmtStart = This->segHeader.mtLoopStart;
	*pmtEnd = This->segHeader.mtLoopEnd;
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegment8Impl_SetPChannelsUsed (LPDIRECTMUSICSEGMENT8 iface, DWORD dwNumPChannels, DWORD* paPChannels)
{
	ICOM_THIS(IDirectMusicSegment8Impl,iface);

	FIXME("(%p, %ld, %p): stub\n", This, dwNumPChannels, paPChannels);	

	return S_OK;
}

/* IDirectMusicSegment8 IDirectMusicSegment8 part: */
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

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicSegment (LPCGUID lpcGUID, LPDIRECTMUSICSEGMENT8 *ppDMSeg, LPUNKNOWN pUnkOuter)
{
	IDirectMusicSegment8Impl *segment;
	
	TRACE("(%p,%p,%p)\n", lpcGUID, ppDMSeg, pUnkOuter);
	if (IsEqualIID (lpcGUID, &IID_IDirectMusicSegment)
		|| IsEqualIID (lpcGUID, &IID_IDirectMusicSegment2)
		|| IsEqualIID (lpcGUID, &IID_IDirectMusicSegment8)) {
		segment = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicSegment8Impl));
		if (NULL == segment) {
			*ppDMSeg = (LPDIRECTMUSICSEGMENT8) NULL;
			return E_OUTOFMEMORY;
		}
		segment->lpVtbl = &DirectMusicSegment8_Vtbl;
		segment->ref = 1;
		*ppDMSeg = (LPDIRECTMUSICSEGMENT8) segment;
		return S_OK;
	}
	
	WARN("No interface found\n");
	return E_NOINTERFACE;
}


/*****************************************************************************
 * IDirectMusicSegmentObject implementation
 */
/* IDirectMusicSegmentObject IUnknown part: */
HRESULT WINAPI IDirectMusicSegmentObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicSegmentObject,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) 
		|| IsEqualGUID(riid, &IID_IDirectMusicObject)) {
		IDirectMusicSegmentObject_AddRef(iface);
		*ppobj = This;
		return S_OK;
	} else if (IsEqualGUID (riid, &IID_IPersistStream)) {
		IDirectMusicSegmentObjectStream_AddRef ((LPPERSISTSTREAM)This->pStream);
		*ppobj = This->pStream;
		return S_OK;
	} else if (IsEqualGUID (riid, &IID_IDirectMusicSegment) 
		|| IsEqualGUID (riid, &IID_IDirectMusicSegment8)) {
		IDirectMusicSegment8Impl_AddRef ((LPDIRECTMUSICSEGMENT8)This->pSegment);
		*ppobj = This->pSegment;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicSegmentObject_AddRef (LPDIRECTMUSICOBJECT iface)
{
	ICOM_THIS(IDirectMusicSegmentObject,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicSegmentObject_Release (LPDIRECTMUSICOBJECT iface)
{
	ICOM_THIS(IDirectMusicSegmentObject,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicSegmentObject IDirectMusicObject part: */
HRESULT WINAPI IDirectMusicSegmentObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc)
{
	ICOM_THIS(IDirectMusicSegmentObject,iface);

	TRACE("(%p, %p)\n", This, pDesc);
	pDesc = This->pDesc;
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegmentObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc)
{
	ICOM_THIS(IDirectMusicSegmentObject,iface);

	TRACE("(%p, %p)\n", This, pDesc);
	This->pDesc = pDesc;

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegmentObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc)
{
	ICOM_THIS(IDirectMusicSegmentObject,iface);

	FIXME("(%p, %p, %p): stub\n", This, pStream, pDesc);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicObject) DirectMusicSegmentObject_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicSegmentObject_QueryInterface,
	IDirectMusicSegmentObject_AddRef,
	IDirectMusicSegmentObject_Release,
	IDirectMusicSegmentObject_GetDescriptor,
	IDirectMusicSegmentObject_SetDescriptor,
	IDirectMusicSegmentObject_ParseDescriptor
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicSegmentObject (LPCGUID lpcGUID, LPDIRECTMUSICOBJECT* ppObject, LPUNKNOWN pUnkOuter)
{
	IDirectMusicSegmentObject *obj;
	
	TRACE("(%p,%p,%p)\n", lpcGUID, ppObject, pUnkOuter);
	if (IsEqualGUID (lpcGUID, &IID_IDirectMusicObject)) {
		obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicSegmentObject));
		if (NULL == obj) {
			*ppObject = (LPDIRECTMUSICOBJECT) NULL;
			return E_OUTOFMEMORY;
		}
		obj->lpVtbl = &DirectMusicSegmentObject_Vtbl;
		obj->ref = 1;
		/* prepare IPersistStream */
		obj->pStream = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicSegmentObjectStream));
		obj->pStream->lpVtbl = &DirectMusicSegmentObjectStream_Vtbl;
		obj->pStream->ref = 1;
		obj->pStream->pParentObject = obj;
		/* prepare IDirectMusicSegment8 */
		DMUSIC_CreateDirectMusicSegment (&IID_IDirectMusicSegment8, (LPDIRECTMUSICSEGMENT8*)&obj->pSegment, NULL);
		obj->pSegment->pObject = obj;
		*ppObject = (LPDIRECTMUSICOBJECT) obj;
		return S_OK;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;
}

/*****************************************************************************
 * IDirectMusicSegmentObjectStream implementation
 */
/* IDirectMusicSegmentObjectStream IUnknown part: */
HRESULT WINAPI IDirectMusicSegmentObjectStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicSegmentObjectStream,iface);

	if (IsEqualGUID(riid, &IID_IUnknown)
		|| IsEqualGUID(riid, &IID_IPersistStream)) {
		IDirectMusicSegmentObjectStream_AddRef (iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicSegmentObjectStream_AddRef (LPPERSISTSTREAM iface)
{
	ICOM_THIS(IDirectMusicSegmentObjectStream,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicSegmentObjectStream_Release (LPPERSISTSTREAM iface)
{
	ICOM_THIS(IDirectMusicSegmentObjectStream,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicSegmentObjectStream IPersist part: */
HRESULT WINAPI IDirectMusicSegmentObjectStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID)
{
	return E_NOTIMPL;
}

/* IDirectMusicSegmentObjectStream IPersistStream part: */
HRESULT WINAPI IDirectMusicSegmentObjectStream_IsDirty (LPPERSISTSTREAM iface)
{
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicSegmentObjectStream_Load (LPPERSISTSTREAM iface, IStream* pStm)
{
	ICOM_THIS(IDirectMusicSegmentObjectStream,iface);
	FOURCC chunkID;
	DWORD chunkSize, StreamSize, StreamCount, ListSize[10], ListCount[10];
	LARGE_INTEGER liMove; /* used when skipping chunks */
	IDirectMusicSegment8Impl* pSegment = This->pParentObject->pSegment; /* that's where we load data */
	DMUS_IO_TRACK_HEADER tempHeader;
	DMUS_IO_TRACK_EXTRAS_HEADER tempXHeader;
	
	IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
	IStream_Read (pStm, &chunkSize, sizeof(FOURCC), NULL);
	TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (chunkID), chunkSize);
	switch (chunkID)
	{
		case FOURCC_RIFF: {
			IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
			TRACE_(dmfile)(": RIFF chunk containing %s", debugstr_fourcc (chunkID));
			StreamSize = chunkSize - sizeof(FOURCC);
			StreamCount = 0;
			switch (chunkID)
			{				
				case DMUS_FOURCC_SEGMENT_FORM: {
					TRACE_(dmfile)(": segment form\n");
					do {
						IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
						IStream_Read (pStm, &chunkSize, sizeof(FOURCC), NULL);
						StreamCount += sizeof (FOURCC) + sizeof (DWORD) + chunkSize;
						TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (chunkID), chunkSize);
						switch (chunkID) {
							case DMUS_FOURCC_SEGMENT_CHUNK: {
								TRACE_(dmfile)(": segment header chunk\n");
								IStream_Read (pStm, &pSegment->segHeader, chunkSize, NULL);
								break;
							}
							case DMUS_FOURCC_GUID_CHUNK: {
								TRACE_(dmfile)(": GUID chunk\n");
								IStream_Read (pStm, &pSegment->vVersion, chunkSize, NULL);
								break;
							}
							case DMUS_FOURCC_VERSION_CHUNK: {
								TRACE_(dmfile)(": version chunk\n");
								IStream_Read (pStm, &pSegment->guidID, chunkSize, NULL);
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
												case DMUS_FOURCC_UNAM_CHUNK: {
													TRACE_(dmfile)(": name chunk\n");
													pSegment->wszName = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, chunkSize);
													IStream_Read (pStm, pSegment->wszName, chunkSize, NULL);
													break;
												}
												case DMUS_FOURCC_UART_CHUNK: {
													TRACE_(dmfile)(": artist chunk\n");
													pSegment->wszArtist = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, chunkSize);
													IStream_Read (pStm, pSegment->wszArtist, chunkSize, NULL);
													break;
												}
												case DMUS_FOURCC_UCOP_CHUNK: {
													TRACE_(dmfile)(": copyright chunk\n");
													pSegment->wszCopyright = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, chunkSize);
													IStream_Read (pStm, pSegment->wszCopyright, chunkSize, NULL);
													break;
												}
												case DMUS_FOURCC_USBJ_CHUNK: {
													TRACE_(dmfile)(": subject chunk\n");
													pSegment->wszSubject = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, chunkSize);
													IStream_Read (pStm, pSegment->wszSubject, chunkSize, NULL);
													break;
												}
												case DMUS_FOURCC_UCMT_CHUNK: {
													TRACE_(dmfile)(": comment chunk\n");
													pSegment->wszComment = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, chunkSize);
													IStream_Read (pStm, pSegment->wszComment, chunkSize, NULL);
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
									case DMUS_FOURCC_TRACK_LIST: {
										TRACE_(dmfile)(": track list\n");
										do {
											IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
											IStream_Read (pStm, &chunkSize, sizeof(FOURCC), NULL);
											ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + chunkSize;
											TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (chunkID), chunkSize);
											switch (chunkID)
											{
												case FOURCC_RIFF: {
													TRACE_(dmfile)(": RIFF chunk");
													IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
													switch (chunkID)
													{
														case DMUS_FOURCC_TRACK_FORM: {
															TRACE_(dmfile)(": containing %s: track form\n", debugstr_fourcc(chunkID));
															ListSize[1] = chunkSize - sizeof(FOURCC);
															ListCount[1] = 0;
															do {
																IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
																IStream_Read (pStm, &chunkSize, sizeof(FOURCC), NULL);
																ListCount[1] += sizeof(FOURCC) + sizeof(DWORD) + chunkSize;
																TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (chunkID), chunkSize);
																switch (chunkID) {
																	case DMUS_FOURCC_TRACK_CHUNK: {
																		TRACE_(dmfile)(": track header chunk\n");
																		IStream_Read (pStm, &tempHeader, chunkSize, NULL);
																		break;
																	} 
																	case DMUS_FOURCC_TRACK_EXTRAS_CHUNK: {
																		TRACE_(dmfile)(": track extra header chunk\n");
																		IStream_Read (pStm, &tempXHeader, chunkSize, NULL);
																		break;
																	}
																	/* add other stuff (look at note below) */
																	default: {
																		TRACE_(dmfile)(": unknown chunk (skipping)\n");
																		liMove.QuadPart = chunkSize;
																		IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
																		break;						
																	}
																}
																TRACE_(dmfile)(": ListCount[1] = %ld < ListSize[1] = %ld\n", ListCount[1], ListSize[1]);
															} while (ListCount[1] < ListSize[1]);
																FIXME(": loading tracks not supported yet\n");
															/* sigh... now comes track creation... currently I have some problems with implementing 
															   this one because my test are contradicting: 
															    - tracks are not loaded by loader (at least my dxdiag test with native dmime doesn't show it)
															       therefore i guess they're created with CoCreateInstance with CLSID specified in header and
															       IID_IDirectMusicTrack(8). Tracks are then probably passed to IDirectMusicSegment_Insert
															       (not quite sure, but behaviour complies with the one described in MSDN (about calling IDirectMusicTrack_Init)
															    - but on the other hand, track's stream implementation gets only <data> chunk (look in MSDN for more info)
															       (tested with native dmime and builtin dmband and dmstyle) => this means that all info about track (header, extra header
															        UNFO, GUID and version are read by segment's stream... now, how the hell is all this info set on track?!
															   => I believe successful approach would be to create structure like this:
															       _DMUSIC_PRIVATE_TRACK_ENTRY {
															        DMUS_IO_TRACK_HEADER trkHeader;
																	 DMUS_IO_TRACK_EXTRAS_HEADER trkXHeader;
															        WCHAR* name, ...;
															        GUID guidID;
															        DMUS_VERSION vVersion;
															        ...
															        IDirectMusicTrack* pTrack;
																   } DMUSIC_PRIVATE_TRACK_ENTRY;
																   and then load all stuff into it
															   => anyway, I'll try to implement it when I find some time again, but this note is here for anyone that wants to give it a try :)
															*/
															break;
														}
														default: {
															TRACE_(dmfile)(": unknown chunk (only DMTK expected; skipping)\n");
															liMove.QuadPart = chunkSize - sizeof(FOURCC);
															IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
															break;
														}
													}
													break;
												}
												default: {
													TRACE_(dmfile)("(unexpected) non-RIFF chunk (skipping, but expect errors)\n");		
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
								TRACE_(dmfile)(": unknown chunk (skipping)\n");
								liMove.QuadPart = chunkSize;
								IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
								break;						
							}					
						}
						TRACE_(dmfile)(": StreamCount = %ld < StreamSize = %ld\n", StreamCount, StreamSize);
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

HRESULT WINAPI IDirectMusicSegmentObjectStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty)
{
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicSegmentObjectStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize)
{
	return E_NOTIMPL;
}

ICOM_VTABLE(IPersistStream) DirectMusicSegmentObjectStream_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicSegmentObjectStream_QueryInterface,
	IDirectMusicSegmentObjectStream_AddRef,
	IDirectMusicSegmentObjectStream_Release,
	IDirectMusicSegmentObjectStream_GetClassID,
	IDirectMusicSegmentObjectStream_IsDirty,
	IDirectMusicSegmentObjectStream_Load,
	IDirectMusicSegmentObjectStream_Save,
	IDirectMusicSegmentObjectStream_GetSizeMax
};
