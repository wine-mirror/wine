/* IDirectMusicChordTrack Implementation
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
 * Copyright (C) 2003-2004 Raphael Junqueira
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

#include "dmstyle_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmstyle);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

static ULONG WINAPI IDirectMusicChordTrack_IUnknown_AddRef (LPUNKNOWN iface);
static ULONG WINAPI IDirectMusicChordTrack_IDirectMusicTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
static ULONG WINAPI IDirectMusicChordTrack_IPersistStream_AddRef (LPPERSISTSTREAM iface);

/*****************************************************************************
 * IDirectMusicChordTrack implementation
 */
/* IDirectMusicChordTrack IUnknown part: */
static HRESULT WINAPI IDirectMusicChordTrack_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicChordTrack, UnknownVtbl, iface);
	TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);

	if (IsEqualIID (riid, &IID_IUnknown)) {
		*ppobj = (LPUNKNOWN)&This->UnknownVtbl;
		IDirectMusicChordTrack_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IDirectMusicTrack)
	  || IsEqualIID (riid, &IID_IDirectMusicTrack8)) {
		*ppobj = (LPDIRECTMUSICTRACK8)&This->TrackVtbl;
		IDirectMusicChordTrack_IDirectMusicTrack_AddRef ((LPDIRECTMUSICTRACK8)&This->TrackVtbl);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IPersistStream)) {
		*ppobj = (LPPERSISTSTREAM)&This->PersistStreamVtbl;
		IDirectMusicChordTrack_IPersistStream_AddRef ((LPPERSISTSTREAM)&This->PersistStreamVtbl);
		return S_OK;
	}
	
	WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicChordTrack_IUnknown_AddRef (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicChordTrack, UnknownVtbl, iface);
        ULONG ref = InterlockedIncrement(&This->ref);

	TRACE("(%p): AddRef from %d\n", This, ref - 1);

	DMSTYLE_LockModule();

	return ref;
}

static ULONG WINAPI IDirectMusicChordTrack_IUnknown_Release (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicChordTrack, UnknownVtbl, iface);
	ULONG ref = InterlockedDecrement(&This->ref);

	TRACE("(%p): ReleaseRef to %d\n", This, ref);

	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}

	DMSTYLE_UnlockModule();

	return ref;
}

static const IUnknownVtbl DirectMusicChordTrack_Unknown_Vtbl = {
  IDirectMusicChordTrack_IUnknown_QueryInterface,
  IDirectMusicChordTrack_IUnknown_AddRef,
  IDirectMusicChordTrack_IUnknown_Release
};

/* IDirectMusicChordTrack IDirectMusicTrack8 part: */
static HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj) {
  ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
  return IDirectMusicChordTrack_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicChordTrack_IDirectMusicTrack_AddRef (LPDIRECTMUSICTRACK8 iface) {
  ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
  return IDirectMusicChordTrack_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

static ULONG WINAPI IDirectMusicChordTrack_IDirectMusicTrack_Release (LPDIRECTMUSICTRACK8 iface) {
  ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
  return IDirectMusicChordTrack_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

static HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment) {
  ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
  FIXME("(%p, %p): stub\n", This, pSegment);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
  ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
  FIXME("(%p, %p, %p, %p, %d, %d): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData)
{
  ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
  FIXME("(%p, %p): stub\n", This, pStateData);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID)
{
  ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
  FIXME("(%p, %p, %ld, %ld, %ld, %d, %p, %p, %d): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam) {
  ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
  FIXME("(%p, %s, %ld, %p, %p): stub\n", This, debugstr_dmguid(rguidType), mtTime, pmtNext, pParam);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam) {
  ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
  FIXME("(%p, %s, %ld, %p): stub\n", This, debugstr_dmguid(rguidType), mtTime, pParam);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType) {
  ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
  
  TRACE("(%p, %s)\n", This, debugstr_dmguid(rguidType));
  if (IsEqualGUID (rguidType, &GUID_BandParam)
      || IsEqualGUID (rguidType, &GUID_ChordParam)
      || IsEqualGUID (rguidType, &GUID_RhythmParam)) {
    TRACE("param supported\n");
    return S_OK;
  }
  TRACE("param unsupported\n");
  return DMUS_E_TYPE_UNSUPPORTED;
}

static HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType) {
  ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
  FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
  return S_OK;
}

static HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType) {
  ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
  FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
  return S_OK;
}

static HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack) {
  ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
  FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID) {
  ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
  FIXME("(%p, %p, 0x%s, 0x%s, 0x%s, %d, %p, %p, %d): stub\n", This, pStateData, wine_dbgstr_longlong(rtStart),
      wine_dbgstr_longlong(rtEnd), wine_dbgstr_longlong(rtOffset), dwFlags, pPerf, pSegSt, dwVirtualID);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags) {
  ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
  FIXME("(%p, %s, 0x%s, %p, %p, %p, %d): stub\n", This, debugstr_dmguid(rguidType),
      wine_dbgstr_longlong(rtTime), prtNext, pParam, pStateData, dwFlags);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags) {
  ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
  FIXME("(%p, %s, 0x%s, %p, %p, %d): stub\n", This, debugstr_dmguid(rguidType),
      wine_dbgstr_longlong(rtTime), pParam, pStateData, dwFlags);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack) {
  ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
  FIXME("(%p, %p, %d, %p): stub\n", This, pContext, dwTrackGroup, ppResultTrack);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack) {
  ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
  FIXME("(%p, %p, %ld, %p, %d, %p): stub\n", This, pNewTrack, mtJoin, pContext, dwTrackGroup, ppResultTrack);
  return S_OK;
}

static const IDirectMusicTrack8Vtbl DirectMusicChordTrack_Track_Vtbl = {
  IDirectMusicChordTrack_IDirectMusicTrack_QueryInterface,
  IDirectMusicChordTrack_IDirectMusicTrack_AddRef,
  IDirectMusicChordTrack_IDirectMusicTrack_Release,
  IDirectMusicChordTrack_IDirectMusicTrack_Init,
  IDirectMusicChordTrack_IDirectMusicTrack_InitPlay,
  IDirectMusicChordTrack_IDirectMusicTrack_EndPlay,
  IDirectMusicChordTrack_IDirectMusicTrack_Play,
  IDirectMusicChordTrack_IDirectMusicTrack_GetParam,
  IDirectMusicChordTrack_IDirectMusicTrack_SetParam,
  IDirectMusicChordTrack_IDirectMusicTrack_IsParamSupported,
  IDirectMusicChordTrack_IDirectMusicTrack_AddNotificationType,
  IDirectMusicChordTrack_IDirectMusicTrack_RemoveNotificationType,
  IDirectMusicChordTrack_IDirectMusicTrack_Clone,
  IDirectMusicChordTrack_IDirectMusicTrack_PlayEx,
  IDirectMusicChordTrack_IDirectMusicTrack_GetParamEx,
  IDirectMusicChordTrack_IDirectMusicTrack_SetParamEx,
  IDirectMusicChordTrack_IDirectMusicTrack_Compose,
  IDirectMusicChordTrack_IDirectMusicTrack_Join
};

/* IDirectMusicChordTrack IPersistStream part: */
static HRESULT WINAPI IDirectMusicChordTrack_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj) {
  ICOM_THIS_MULTI(IDirectMusicChordTrack, PersistStreamVtbl, iface);
  return IDirectMusicChordTrack_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicChordTrack_IPersistStream_AddRef (LPPERSISTSTREAM iface) {
  ICOM_THIS_MULTI(IDirectMusicChordTrack, PersistStreamVtbl, iface);
  return IDirectMusicChordTrack_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

static ULONG WINAPI IDirectMusicChordTrack_IPersistStream_Release (LPPERSISTSTREAM iface) {
  ICOM_THIS_MULTI(IDirectMusicChordTrack, PersistStreamVtbl, iface);
  return IDirectMusicChordTrack_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

static HRESULT WINAPI IDirectMusicChordTrack_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID) {
  ICOM_THIS_MULTI(IDirectMusicChordTrack, PersistStreamVtbl, iface);
  TRACE("(%p, %p)\n", This, pClassID);
  memcpy(pClassID, &CLSID_DirectMusicChordTrack, sizeof(CLSID));
  return S_OK;
}

static HRESULT WINAPI IDirectMusicChordTrack_IPersistStream_IsDirty (LPPERSISTSTREAM iface) {
  ICOM_THIS_MULTI(IDirectMusicChordTrack, PersistStreamVtbl, iface);
  FIXME("(%p): stub, always S_FALSE\n", This);
  return S_FALSE;
}

static HRESULT IDirectMusicChordTrack_IPersistStream_ParseChordTrackList (LPPERSISTSTREAM iface, DMUS_PRIVATE_CHUNK* pChunk, IStream* pStm) {

  ICOM_THIS_MULTI(IDirectMusicChordTrack, PersistStreamVtbl, iface);
  DMUS_PRIVATE_CHUNK Chunk;
  DWORD ListSize[3], ListCount[3];
  LARGE_INTEGER liMove; /* used when skipping chunks */

  if (pChunk->fccID != DMUS_FOURCC_CHORDTRACK_LIST) {
    ERR_(dmfile)(": %s chunk should be a CHORDTRACK list\n", debugstr_fourcc (pChunk->fccID));
    return E_FAIL;
  }  

  ListSize[0] = pChunk->dwSize - sizeof(FOURCC);
  ListCount[0] = 0;

  do {
    IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
    ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
    TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
    switch (Chunk.fccID) { 
    case DMUS_FOURCC_CHORDTRACKHEADER_CHUNK: {
      TRACE_(dmfile)(": Chord track header chunk\n");
      IStream_Read (pStm, &This->dwScale, sizeof(DWORD), NULL);
      TRACE_(dmfile)(" - dwScale: %d\n", This->dwScale);
      break;
    }
    case DMUS_FOURCC_CHORDTRACKBODY_CHUNK: {
      DWORD sz;
      DWORD it;
      DWORD num;
      DMUS_IO_CHORD body;
      DMUS_IO_SUBCHORD subchords;

      TRACE_(dmfile)(": Chord track body chunk\n");

      IStream_Read (pStm, &sz, sizeof(DWORD), NULL);
      TRACE_(dmfile)(" - sizeof(DMUS_IO_CHORD): %d\n", sz);
      if (sz != sizeof(DMUS_IO_CHORD)) return E_FAIL;
      IStream_Read (pStm, &body, sizeof(DMUS_IO_CHORD), NULL);
      TRACE_(dmfile)(" - wszName: %s\n", debugstr_w(body.wszName));
      TRACE_(dmfile)(" - mtTime: %lu\n", body.mtTime);
      TRACE_(dmfile)(" - wMeasure: %u\n", body.wMeasure);
      TRACE_(dmfile)(" - bBeat:  %u\n", body.bBeat);
      TRACE_(dmfile)(" - bFlags: 0x%02x\n", body.bFlags);
      
      IStream_Read (pStm, &num, sizeof(DWORD), NULL);
      TRACE_(dmfile)(" - # DMUS_IO_SUBCHORDS: %d\n", num);
      IStream_Read (pStm, &sz, sizeof(DWORD), NULL);
      TRACE_(dmfile)(" - sizeof(DMUS_IO_SUBCHORDS): %d\n", sz);
      if (sz != sizeof(DMUS_IO_SUBCHORD)) return E_FAIL;

      for (it = 0; it < num; ++it) {
	IStream_Read (pStm, &subchords, sizeof(DMUS_IO_SUBCHORD), NULL);
	TRACE_(dmfile)("DMUS_IO_SUBCHORD #%d\n", it+1);
	TRACE_(dmfile)(" - dwChordPattern: %u\n", subchords.dwChordPattern);
	TRACE_(dmfile)(" - dwScalePattern: %u\n", subchords.dwScalePattern);
	TRACE_(dmfile)(" - dwInversionPoints: %u\n", subchords.dwInversionPoints);
	TRACE_(dmfile)(" - dwLevels: %u\n", subchords.dwLevels);
	TRACE_(dmfile)(" - bChordRoot:  %u\n", subchords.bChordRoot);
	TRACE_(dmfile)(" - bScaleRoot: %u\n", subchords.bScaleRoot);
      }
      break;
    }
    default: {
      TRACE_(dmfile)(": unknown chunk (irrevelant & skipping)\n");
      liMove.QuadPart = Chunk.dwSize;
      IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
      break;		
    }
    }
    TRACE_(dmfile)(": ListCount[0] = %d < ListSize[0] = %d\n", ListCount[0], ListSize[0]);
  } while (ListCount[0] < ListSize[0]);

  return S_OK;
}

static HRESULT WINAPI IDirectMusicChordTrack_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm) {
  ICOM_THIS_MULTI(IDirectMusicChordTrack, PersistStreamVtbl, iface);
 
  DMUS_PRIVATE_CHUNK Chunk;
  LARGE_INTEGER liMove;
  HRESULT hr;
 
  TRACE("(%p, %p): Loading\n", This, pStm);

  IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
  TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
  switch (Chunk.fccID) {	
  case FOURCC_LIST: {
    IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);
    TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
    switch (Chunk.fccID) { 
    case DMUS_FOURCC_CHORDTRACK_LIST: {
      TRACE_(dmfile)(": Chord track list\n");
      hr = IDirectMusicChordTrack_IPersistStream_ParseChordTrackList (iface, &Chunk, pStm);
      if (FAILED(hr)) return hr;
      break;    
    }
    default: {
      TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
      liMove.QuadPart = Chunk.dwSize;
      IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
      return E_FAIL;
    }
    }
    TRACE_(dmfile)(": reading finished\n");
    break;
  }
  default: {
    TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
    liMove.QuadPart = Chunk.dwSize;
    IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
    return E_FAIL;
  }
  }

  return S_OK;
}

static HRESULT WINAPI IDirectMusicChordTrack_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty) {
  ICOM_THIS_MULTI(IDirectMusicChordTrack, PersistStreamVtbl, iface);
  FIXME("(%p): Saving not implemented yet\n", This);
  return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicChordTrack_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize) {
  ICOM_THIS_MULTI(IDirectMusicChordTrack, PersistStreamVtbl, iface);
  FIXME("(%p, %p): stub\n", This, pcbSize);
  return E_NOTIMPL;
}

static const IPersistStreamVtbl DirectMusicChordTrack_PersistStream_Vtbl = {
  IDirectMusicChordTrack_IPersistStream_QueryInterface,
  IDirectMusicChordTrack_IPersistStream_AddRef,
  IDirectMusicChordTrack_IPersistStream_Release,
  IDirectMusicChordTrack_IPersistStream_GetClassID,
  IDirectMusicChordTrack_IPersistStream_IsDirty,
  IDirectMusicChordTrack_IPersistStream_Load,
  IDirectMusicChordTrack_IPersistStream_Save,
  IDirectMusicChordTrack_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicChordTrack (LPCGUID lpcGUID, LPVOID *ppobj, LPUNKNOWN pUnkOuter) {
  IDirectMusicChordTrack* track;
  
  track = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicChordTrack));
  if (NULL == track) {
    *ppobj = NULL;
    return E_OUTOFMEMORY;
  }
  track->UnknownVtbl = &DirectMusicChordTrack_Unknown_Vtbl;
  track->TrackVtbl = &DirectMusicChordTrack_Track_Vtbl;
  track->PersistStreamVtbl = &DirectMusicChordTrack_PersistStream_Vtbl;
  track->pDesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DMUS_OBJECTDESC));
  DM_STRUCT_INIT(track->pDesc);
  track->pDesc->dwValidData |= DMUS_OBJ_CLASS;
  memcpy (&track->pDesc->guidClass, &CLSID_DirectMusicChordTrack, sizeof (CLSID));
  track->ref = 0; /* will be inited by QueryInterface */
  
  return IDirectMusicChordTrack_IUnknown_QueryInterface ((LPUNKNOWN)&track->UnknownVtbl, lpcGUID, ppobj);
}
