/* IDirectMusicChordTrack Implementation
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
#include "wine/unicode.h"

#include "dmstyle_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmstyle);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

/*****************************************************************************
 * IDirectMusicChordTrack implementation
 */
/* IDirectMusicChordTrack IUnknown part: */
HRESULT WINAPI IDirectMusicChordTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicChordTrack,iface);

	if (IsEqualIID (riid, &IID_IUnknown) || 
	    IsEqualIID (riid, &IID_IDirectMusicTrack) ||
	    IsEqualIID (riid, &IID_IDirectMusicTrack8)) {
		IDirectMusicChordTrack_AddRef(iface);
		*ppobj = This;
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IPersistStream)) {
		IDirectMusicChordTrackStream_AddRef ((LPPERSISTSTREAM)This->pStream);
		*ppobj = This->pStream;
		return S_OK;
	}

	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicChordTrack_AddRef (LPDIRECTMUSICTRACK8 iface)
{
	ICOM_THIS(IDirectMusicChordTrack,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicChordTrack_Release (LPDIRECTMUSICTRACK8 iface)
{
	ICOM_THIS(IDirectMusicChordTrack,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicChordTrack IDirectMusicTrack part: */
HRESULT WINAPI IDirectMusicChordTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment)
{
	ICOM_THIS(IDirectMusicChordTrack,iface);

	FIXME("(%p, %p): stub\n", This, pSegment);

	return S_OK;
}

HRESULT WINAPI IDirectMusicChordTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
	ICOM_THIS(IDirectMusicChordTrack,iface);

	FIXME("(%p, %p, %p, %p, %ld, %ld): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicChordTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData)
{
	ICOM_THIS(IDirectMusicChordTrack,iface);

	FIXME("(%p, %p): stub\n", This, pStateData);

	return S_OK;
}

HRESULT WINAPI IDirectMusicChordTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID)
{
	ICOM_THIS(IDirectMusicChordTrack,iface);

	FIXME("(%p, %p, %ld, %ld, %ld, %ld, %p, %p, %ld): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);

	return S_OK;
}

HRESULT WINAPI IDirectMusicChordTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam)
{
	ICOM_THIS(IDirectMusicChordTrack,iface);

	FIXME("(%p, %s, %ld, %p, %p): stub\n", This, debugstr_guid(rguidType), mtTime, pmtNext, pParam);

	return S_OK;
}

HRESULT WINAPI IDirectMusicChordTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam)
{
	ICOM_THIS(IDirectMusicChordTrack,iface);

	FIXME("(%p, %s, %ld, %p): stub\n", This, debugstr_guid(rguidType), mtTime, pParam);

	return S_OK;
}

HRESULT WINAPI IDirectMusicChordTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType)
{
	ICOM_THIS(IDirectMusicChordTrack,iface);

	TRACE("(%p, %s): ", This, debugstr_guid(rguidType));
	if (IsEqualGUID (rguidType, &GUID_BandParam)
		|| IsEqualGUID (rguidType, &GUID_ChordParam)
		|| IsEqualGUID (rguidType, &GUID_RhythmParam)) {
		TRACE("param supported\n");
		return S_OK;
		}

	TRACE("param unsupported\n");
	return DMUS_E_TYPE_UNSUPPORTED;
}

HRESULT WINAPI IDirectMusicChordTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType)
{
	ICOM_THIS(IDirectMusicChordTrack,iface);

	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));

	return S_OK;
}

HRESULT WINAPI IDirectMusicChordTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType)
{
	ICOM_THIS(IDirectMusicChordTrack,iface);

	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));

	return S_OK;
}

HRESULT WINAPI IDirectMusicChordTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack)
{
	ICOM_THIS(IDirectMusicChordTrack,iface);

	FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);

	return S_OK;
}

/* IDirectMusicChordTrack IDirectMusicTrack8 part: */
HRESULT WINAPI IDirectMusicChordTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID)
{
	ICOM_THIS(IDirectMusicChordTrack,iface);

	FIXME("(%p, %p, %lli, %lli, %lli, %ld, %p, %p, %ld): stub\n", This, pStateData, rtStart, rtEnd, rtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);

	return S_OK;
}

HRESULT WINAPI IDirectMusicChordTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags)
{
	ICOM_THIS(IDirectMusicChordTrack,iface);

	FIXME("(%p, %s, %lli, %p, %p, %p, %ld): stub\n", This, debugstr_guid(rguidType), rtTime, prtNext, pParam, pStateData, dwFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicChordTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags)
{
	ICOM_THIS(IDirectMusicChordTrack,iface);

	FIXME("(%p, %s, %lli, %p, %p, %ld): stub\n", This, debugstr_guid(rguidType), rtTime, pParam, pStateData, dwFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicChordTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack)
{
	ICOM_THIS(IDirectMusicChordTrack,iface);

	FIXME("(%p, %p, %ld, %p): stub\n", This, pContext, dwTrackGroup, ppResultTrack);

	return S_OK;
}

HRESULT WINAPI IDirectMusicChordTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack)
{
	ICOM_THIS(IDirectMusicChordTrack,iface);

	FIXME("(%p, %p, %ld, %p, %ld, %p): stub\n", This, pNewTrack, mtJoin, pContext, dwTrackGroup, ppResultTrack);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicTrack8) DirectMusicChordTrack_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicChordTrack_QueryInterface,
	IDirectMusicChordTrack_AddRef,
	IDirectMusicChordTrack_Release,
	IDirectMusicChordTrack_Init,
	IDirectMusicChordTrack_InitPlay,
	IDirectMusicChordTrack_EndPlay,
	IDirectMusicChordTrack_Play,
	IDirectMusicChordTrack_GetParam,
	IDirectMusicChordTrack_SetParam,
	IDirectMusicChordTrack_IsParamSupported,
	IDirectMusicChordTrack_AddNotificationType,
	IDirectMusicChordTrack_RemoveNotificationType,
	IDirectMusicChordTrack_Clone,
	IDirectMusicChordTrack_PlayEx,
	IDirectMusicChordTrack_GetParamEx,
	IDirectMusicChordTrack_SetParamEx,
	IDirectMusicChordTrack_Compose,
	IDirectMusicChordTrack_Join
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicChordTrack (LPCGUID lpcGUID, LPDIRECTMUSICTRACK8 *ppTrack, LPUNKNOWN pUnkOuter)
{
	IDirectMusicChordTrack* track;
	
	if (IsEqualIID (lpcGUID, &IID_IDirectMusicTrack)
		|| IsEqualIID (lpcGUID, &IID_IDirectMusicTrack8)) {
		track = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicChordTrack));
		if (NULL == track) {
			*ppTrack = (LPDIRECTMUSICTRACK8) NULL;
			return E_OUTOFMEMORY;
		}
		track->lpVtbl = &DirectMusicChordTrack_Vtbl;
		track->ref = 1;
		track->pStream = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(IDirectMusicChordTrackStream));
		track->pStream->lpVtbl = &DirectMusicChordTrackStream_Vtbl;
		track->pStream->ref = 1;	
		track->pStream->pParentTrack = track;
		*ppTrack = (LPDIRECTMUSICTRACK8) track;
		return S_OK;
	}
	
	WARN("No interface found\n");
	return E_NOINTERFACE;
}


/*****************************************************************************
 * IDirectMusicChordTrackStream implementation
 */
/* IDirectMusicChordTrackStream IUnknown part follow: */
HRESULT WINAPI IDirectMusicChordTrackStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicChordTrackStream,iface);

	if (IsEqualGUID(riid, &IID_IUnknown)
		|| IsEqualGUID(riid, &IID_IPersistStream)) {
		IDirectMusicChordTrackStream_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}

	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicChordTrackStream_AddRef (LPPERSISTSTREAM iface)
{
	ICOM_THIS(IDirectMusicChordTrackStream,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicChordTrackStream_Release (LPPERSISTSTREAM iface)
{
	ICOM_THIS(IDirectMusicChordTrackStream,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicChordTrackStream IPersist part: */
HRESULT WINAPI IDirectMusicChordTrackStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID)
{
	return E_NOTIMPL;
}

/* IDirectMusicChordTrackStream IPersistStream part: */
HRESULT WINAPI IDirectMusicChordTrackStream_IsDirty (LPPERSISTSTREAM iface)
{
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicChordTrackStream_Load (LPPERSISTSTREAM iface, IStream* pStm)
{
	ICOM_THIS(IDirectMusicChordTrackStream,iface);
	FOURCC chunkID;
	DWORD chunkSize, dwSizeOfStruct, ListSize[1], ListCount[1];
	LARGE_INTEGER liMove; /* used when skipping chunks */
	IDirectMusicChordTrack* pTrack = This->pParentTrack; /* that's where we load data to */
	DMUS_IO_CHORD tempChord; /* temporary, used for reading data */	
	DWORD tempSubChords;
	
	IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
	IStream_Read (pStm, &chunkSize, sizeof(DWORD), NULL);
	TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (chunkID), chunkSize);
	switch (chunkID) {	
		case FOURCC_LIST: {
			IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);				
			TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(chunkID));
			ListSize[0] = chunkSize - sizeof(FOURCC);
			ListCount[0] = 0;
			switch (chunkID) {
				case DMUS_FOURCC_CHORDTRACK_LIST: {
					TRACE_(dmfile)(": chord track list\n");
					do {
						IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
						IStream_Read (pStm, &chunkSize, sizeof(FOURCC), NULL);
						ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + chunkSize;
						TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (chunkID), chunkSize);
						switch (chunkID) {
							case DMUS_FOURCC_CHORDTRACKHEADER_CHUNK: {
								TRACE_(dmfile)(": chord track header chunk\n");
								IStream_Read (pStm, &pTrack->dwHeader, chunkSize, NULL);
								TRACE_(dmfile)(": (READ): header: chord root = %i; chord scale = %i\n", (pTrack->dwHeader && 0xFF000000) >> 24, pTrack->dwHeader && 0x00FFFFFF);
								break;
							}
							case DMUS_FOURCC_CHORDTRACKBODY_CHUNK: {
								TRACE_(dmfile)(": chord track body chunk\n");
								/* make space for one more structure */
								/* pTrack->dwChordKeys++; */ /* moved at the end for correct counting */
								/* FIXME: scheme with HeapReAlloc doesn't work so.. */
								/* pTrack->pChordKeys = HeapReAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, pTrack->pChordKeys, sizeof(DMUS_CHORD_KEY) * pTrack->dwChordKeys); */
								/* pTrack->pChordKeysTime = HeapReAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, pTrack->pChordKeysTime, sizeof(MUSIC_TIME) *pTrack->dwChordKeys); */
								/* load size of DMUS_IO_CHORD */
								IStream_Read (pStm, &dwSizeOfStruct, sizeof(DWORD), NULL);
								if (dwSizeOfStruct != sizeof(DMUS_IO_CHORD)) {
									TRACE_(dmfile)(": declared size of struct (=%ld) != actual size (=%i); loading failed\n", dwSizeOfStruct, sizeof(DMUS_IO_CHORD));
									return E_FAIL;
								}
								/* reset temporary storage and fill it with data */
								ZeroMemory (&tempChord, sizeof(DMUS_IO_CHORD));
								IStream_Read (pStm, &tempChord, dwSizeOfStruct, NULL);
								/* copy data to final destination */
								strncpyW (pTrack->pChordKeys[pTrack->dwChordKeys].wszName, tempChord.wszName, 16);
								/*pTrack->pChordKeys[pTrack->dwChordKeys].wszName = tempChord.wszName; */
								pTrack->pChordKeys[pTrack->dwChordKeys].wMeasure = tempChord.wMeasure;
								pTrack->pChordKeys[pTrack->dwChordKeys].bBeat = tempChord.bBeat;
								pTrack->pChordKeys[pTrack->dwChordKeys].bFlags = tempChord.bFlags;
								/* this one is my invention */
								pTrack->pChordKeysTime[pTrack->dwChordKeys] = tempChord.mtTime;
								/* FIXME: are these two are derived from header? */
								pTrack->pChordKeys[pTrack->dwChordKeys].dwScale = pTrack->dwHeader && 0x00FFFFFF;
								pTrack->pChordKeys[pTrack->dwChordKeys].bKey = (pTrack->dwHeader && 0xFF000000) >> 24;
								/* now here comes number of subchords */
								IStream_Read (pStm, &tempSubChords, sizeof(DWORD), NULL);
								pTrack->pChordKeys[pTrack->dwChordKeys].bSubChordCount = tempSubChords;
								/* load size of DMUS_IO_SUBCHORD */								
								IStream_Read (pStm, &dwSizeOfStruct, sizeof(DWORD), NULL);
								if (dwSizeOfStruct != sizeof(DMUS_IO_SUBCHORD)) {
									TRACE_(dmfile)(": declared size of struct (=%ld) != actual size (=%i); loading failed\n", dwSizeOfStruct, sizeof(DMUS_IO_SUBCHORD));
									return E_FAIL;
								}								
								IStream_Read (pStm, pTrack->pChordKeys[pTrack->dwChordKeys].SubChordList, dwSizeOfStruct * tempSubChords, NULL); 
								/* well, this should be it :) */
								pTrack->dwChordKeys++;
								break;
							}
							default: {
								TRACE_(dmfile)(": unknown chunk (skipping)\n");
								liMove.QuadPart = chunkSize;
								IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL); /* skip this chunk */
								break;					
							}	
						}
						TRACE_(dmfile)(": ListCount[0] = %ld < ListSize[0] = %ld\n", ListCount[0], ListSize[0]);
					} while (ListCount[0] < ListSize[0]);
					break;
				}
				default: {
					TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
					liMove.QuadPart = ListSize[0];
					IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
					return E_FAIL;
				}
			}
			/* in the end, let's see what we got */
			TRACE_(dmfile)(": reading finished\n");
			if (TRACE_ON(dmfile)) {
				int i,j;
				TRACE_(dmfile)(": (READ): number of chord keys in track = %ld\n", pTrack->dwChordKeys);
				for (i = 0; i < pTrack->dwChordKeys; i++) {
					TRACE_(dmfile)(": (READ): chord key[%i]: associated mtTime = %li\n", i, pTrack->pChordKeysTime[i]);
					TRACE_(dmfile)(": (READ): chord key[%i]: wszName = %s; wMeasure = %d; bBeat = %i; dwScale = %ld; \
bKey = %i; bFlags = %i; bSubChordCount = %i\n", i, debugstr_w (pTrack->pChordKeys[i].wszName), \
					pTrack->pChordKeys[i].wMeasure, pTrack->pChordKeys[i].bBeat, pTrack->pChordKeys[i].dwScale, \
					pTrack->pChordKeys[i].bKey, pTrack->pChordKeys[i].bFlags, pTrack->pChordKeys[i].bSubChordCount);
					for (j = 0; j < pTrack->pChordKeys[i].bSubChordCount; j++) {
						TRACE_(dmfile)(": (READ): chord key[%i]: subchord[%i]: 	dwChordPattern = %ld; \
dwScalePattern = %ld; dwInversionPoints = %ld; dwLevels = %ld; bChordRoot = %i; \
bScaleRoot = %i\n", i, j, pTrack->pChordKeys[i].SubChordList[j].dwChordPattern, \
						pTrack->pChordKeys[i].SubChordList[j].dwScalePattern, pTrack->pChordKeys[i].SubChordList[j].dwInversionPoints, \
						pTrack->pChordKeys[i].SubChordList[j].dwLevels, pTrack->pChordKeys[i].SubChordList[j].bChordRoot, \
						pTrack->pChordKeys[i].SubChordList[j].bScaleRoot);
					}
				}
			}			
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

HRESULT WINAPI IDirectMusicChordTrackStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty)
{
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicChordTrackStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize)
{
	return E_NOTIMPL;
}

ICOM_VTABLE(IPersistStream) DirectMusicChordTrackStream_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicChordTrackStream_QueryInterface,
	IDirectMusicChordTrackStream_AddRef,
	IDirectMusicChordTrackStream_Release,
	IDirectMusicChordTrackStream_GetClassID,
	IDirectMusicChordTrackStream_IsDirty,
	IDirectMusicChordTrackStream_Load,
	IDirectMusicChordTrackStream_Save,
	IDirectMusicChordTrackStream_GetSizeMax
};
