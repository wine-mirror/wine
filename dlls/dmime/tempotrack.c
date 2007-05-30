/* IDirectMusicTempoTrack Implementation
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
 * Copyright (C) 2004 Raphael Junqueira
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

#include "dmime_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmime);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

/*****************************************************************************
 * IDirectMusicTempoTrack implementation
 */
/* IDirectMusicTempoTrack IUnknown part: */
static HRESULT WINAPI IDirectMusicTempoTrack_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj) {
  ICOM_THIS_MULTI(IDirectMusicTempoTrack, UnknownVtbl, iface);
  TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);
  
  if (IsEqualIID (riid, &IID_IUnknown)) {
    *ppobj = (LPUNKNOWN)&This->UnknownVtbl;
    IUnknown_AddRef (iface);
    return S_OK;
  } else if (IsEqualIID (riid, &IID_IDirectMusicTrack)
	     || IsEqualIID (riid, &IID_IDirectMusicTrack8)) {
    *ppobj = (LPDIRECTMUSICTRACK8)&This->TrackVtbl;
    IUnknown_AddRef (iface);
    return S_OK;
  } else if (IsEqualIID (riid, &IID_IPersistStream)) {
    *ppobj = (LPPERSISTSTREAM)&This->PersistStreamVtbl;
    IUnknown_AddRef (iface);
    return S_OK;
  }
  
  WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
  return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicTempoTrack_IUnknown_AddRef (LPUNKNOWN iface) {
  ICOM_THIS_MULTI(IDirectMusicTempoTrack, UnknownVtbl, iface);
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("(%p): AddRef from %d\n", This, ref - 1);

  DMIME_LockModule();

  return ref;
}

static ULONG WINAPI IDirectMusicTempoTrack_IUnknown_Release (LPUNKNOWN iface) {
  ICOM_THIS_MULTI(IDirectMusicTempoTrack, UnknownVtbl, iface);
  ULONG ref = InterlockedDecrement(&This->ref);
  TRACE("(%p): ReleaseRef to %d\n", This, ref);
  
  if (ref == 0) {
    HeapFree(GetProcessHeap(), 0, This);
  }

  DMIME_UnlockModule();
  
  return ref;
}

static const IUnknownVtbl DirectMusicTempoTrack_Unknown_Vtbl = {
  IDirectMusicTempoTrack_IUnknown_QueryInterface,
  IDirectMusicTempoTrack_IUnknown_AddRef,
  IDirectMusicTempoTrack_IUnknown_Release
};

/* IDirectMusicTempoTrack IDirectMusicTrack8 part: */
static HRESULT WINAPI IDirectMusicTempoTrack_IDirectMusicTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj) {
  ICOM_THIS_MULTI(IDirectMusicTempoTrack, TrackVtbl, iface);
  return IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicTempoTrack_IDirectMusicTrack_AddRef (LPDIRECTMUSICTRACK8 iface) {
  ICOM_THIS_MULTI(IDirectMusicTempoTrack, TrackVtbl, iface);
  return IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

static ULONG WINAPI IDirectMusicTempoTrack_IDirectMusicTrack_Release (LPDIRECTMUSICTRACK8 iface) {
  ICOM_THIS_MULTI(IDirectMusicTempoTrack, TrackVtbl, iface);
  return IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

static HRESULT WINAPI IDirectMusicTempoTrack_IDirectMusicTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment)
{
  ICOM_THIS_MULTI(IDirectMusicTempoTrack, TrackVtbl, iface);
  TRACE("(%p, %p): nothing to do here\n", This, pSegment);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicTempoTrack_IDirectMusicTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
  ICOM_THIS_MULTI(IDirectMusicTempoTrack, TrackVtbl, iface);

  LPDMUS_PRIVATE_TEMPO_PLAY_STATE pState = NULL;

  FIXME("(%p, %p, %p, %p, %d, %d): semi-stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);

  pState = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DMUS_PRIVATE_TEMPO_PLAY_STATE));
  if (NULL == pState) {
    ERR(": no more memory\n");
    return E_OUTOFMEMORY;
  }
  /** TODO real fill useful datas */
  pState->dummy = 0;
  *ppStateData = pState;
  return S_OK;
}

static HRESULT WINAPI IDirectMusicTempoTrack_IDirectMusicTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData)
{
  ICOM_THIS_MULTI(IDirectMusicTempoTrack, TrackVtbl, iface);

  LPDMUS_PRIVATE_TEMPO_PLAY_STATE pState = pStateData;

  FIXME("(%p, %p): semi-stub\n", This, pStateData);

  if (NULL == pStateData) {
    return E_POINTER;
  }
  /** TODO real clean up */
  HeapFree(GetProcessHeap(), 0, pState);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicTempoTrack_IDirectMusicTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID)
{
  ICOM_THIS_MULTI(IDirectMusicTempoTrack, TrackVtbl, iface);
  FIXME("(%p, %p, %ld, %ld, %ld, %d, %p, %p, %d): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
  /** should use IDirectMusicPerformance_SendPMsg here */
  return S_OK;
}

static HRESULT WINAPI IDirectMusicTempoTrack_IDirectMusicTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam) {
  ICOM_THIS_MULTI(IDirectMusicTempoTrack, TrackVtbl, iface);

  HRESULT hr = S_OK;
  struct list* pEntry = NULL;
  LPDMUS_PRIVATE_TEMPO_ITEM pIt = NULL;
  DMUS_TEMPO_PARAM* prm = pParam;

  FIXME("(%p, %s, %ld, %p, %p): almost stub\n", This, debugstr_dmguid(rguidType), mtTime, pmtNext, pParam);

  if (NULL == pParam) {
    return E_POINTER;
  }

  hr = IDirectMusicTrack_IsParamSupported (iface, rguidType);
  if (FAILED(hr)) {
    return hr;
  }
  if (FALSE == This->enabled) {
    return DMUS_E_TYPE_DISABLED;
  }

  if (NULL != pmtNext) *pmtNext = 0;
  prm->mtTime = 0;
  prm->dblTempo = 0.123456;

  LIST_FOR_EACH (pEntry, &This->Items) {
    pIt = LIST_ENTRY(pEntry, DMUS_PRIVATE_TEMPO_ITEM, entry);
    /*TRACE(" - %p -> 0x%lx,%p\n", pIt, pIt->item.lTime, pIt->item.dblTempo);*/
    if (pIt->item.lTime <= mtTime) {
      MUSIC_TIME ofs = pIt->item.lTime - mtTime;
      if (ofs > prm->mtTime) {
	prm->mtTime = ofs;
	prm->dblTempo = pIt->item.dblTempo;
      }	
      if (NULL != pmtNext && pIt->item.lTime > mtTime) {
	if (pIt->item.lTime < *pmtNext) {
	  *pmtNext = pIt->item.lTime;
	}
      }
    }
  }
  
  if (0.123456 == prm->dblTempo) {
    return DMUS_E_NOT_FOUND;
  }
  return S_OK;
}

static HRESULT WINAPI IDirectMusicTempoTrack_IDirectMusicTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam) {
	ICOM_THIS_MULTI(IDirectMusicTempoTrack, TrackVtbl, iface);
	FIXME("(%p, %s, %ld, %p): stub\n", This, debugstr_dmguid(rguidType), mtTime, pParam);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTempoTrack_IDirectMusicTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType) {
  ICOM_THIS_MULTI(IDirectMusicTempoTrack, TrackVtbl, iface);

  TRACE("(%p, %s)\n", This, debugstr_dmguid(rguidType));
  if (IsEqualGUID (rguidType, &GUID_DisableTempo)
      || IsEqualGUID (rguidType, &GUID_EnableTempo)
      || IsEqualGUID (rguidType, &GUID_TempoParam)) {
    TRACE("param supported\n");
    return S_OK;
  }
  if (FALSE == This->enabled) {
    return DMUS_E_TYPE_DISABLED;
  }
  TRACE("param unsupported\n");
  return DMUS_E_TYPE_UNSUPPORTED;
}

static HRESULT WINAPI IDirectMusicTempoTrack_IDirectMusicTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType) {
  ICOM_THIS_MULTI(IDirectMusicTempoTrack, TrackVtbl, iface);
  FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
  return S_OK;
}

static HRESULT WINAPI IDirectMusicTempoTrack_IDirectMusicTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType) {
  ICOM_THIS_MULTI(IDirectMusicTempoTrack, TrackVtbl, iface);
  FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
  return S_OK;
}

static HRESULT WINAPI IDirectMusicTempoTrack_IDirectMusicTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack) {
  ICOM_THIS_MULTI(IDirectMusicTempoTrack, TrackVtbl, iface);
  FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicTempoTrack_IDirectMusicTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID) {
  ICOM_THIS_MULTI(IDirectMusicTempoTrack, TrackVtbl, iface);
  FIXME("(%p, %p, 0x%s, 0x%s, 0x%s, %d, %p, %p, %d): stub\n", This, pStateData, wine_dbgstr_longlong(rtStart),
      wine_dbgstr_longlong(rtEnd), wine_dbgstr_longlong(rtOffset), dwFlags, pPerf, pSegSt, dwVirtualID);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicTempoTrack_IDirectMusicTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags) {
  ICOM_THIS_MULTI(IDirectMusicTempoTrack, TrackVtbl, iface);
  FIXME("(%p, %s, 0x%s, %p, %p, %p, %d): stub\n", This, debugstr_dmguid(rguidType),
      wine_dbgstr_longlong(rtTime), prtNext, pParam, pStateData, dwFlags);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicTempoTrack_IDirectMusicTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags) {
  ICOM_THIS_MULTI(IDirectMusicTempoTrack, TrackVtbl, iface);
  FIXME("(%p, %s, 0x%s, %p, %p, %d): stub\n", This, debugstr_dmguid(rguidType),
      wine_dbgstr_longlong(rtTime), pParam, pStateData, dwFlags);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicTempoTrack_IDirectMusicTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack) {
  ICOM_THIS_MULTI(IDirectMusicTempoTrack, TrackVtbl, iface);
  FIXME("(%p, %p, %d, %p): stub\n", This, pContext, dwTrackGroup, ppResultTrack);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicTempoTrack_IDirectMusicTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack) {
  ICOM_THIS_MULTI(IDirectMusicTempoTrack, TrackVtbl, iface);
  FIXME("(%p, %p, %ld, %p, %d, %p): stub\n", This, pNewTrack, mtJoin, pContext, dwTrackGroup, ppResultTrack);
  return S_OK;
}

static const IDirectMusicTrack8Vtbl DirectMusicTempoTrack_Track_Vtbl = {
  IDirectMusicTempoTrack_IDirectMusicTrack_QueryInterface,
  IDirectMusicTempoTrack_IDirectMusicTrack_AddRef,
  IDirectMusicTempoTrack_IDirectMusicTrack_Release,
  IDirectMusicTempoTrack_IDirectMusicTrack_Init,
  IDirectMusicTempoTrack_IDirectMusicTrack_InitPlay,
  IDirectMusicTempoTrack_IDirectMusicTrack_EndPlay,
  IDirectMusicTempoTrack_IDirectMusicTrack_Play,
  IDirectMusicTempoTrack_IDirectMusicTrack_GetParam,
  IDirectMusicTempoTrack_IDirectMusicTrack_SetParam,
  IDirectMusicTempoTrack_IDirectMusicTrack_IsParamSupported,
  IDirectMusicTempoTrack_IDirectMusicTrack_AddNotificationType,
  IDirectMusicTempoTrack_IDirectMusicTrack_RemoveNotificationType,
  IDirectMusicTempoTrack_IDirectMusicTrack_Clone,
  IDirectMusicTempoTrack_IDirectMusicTrack_PlayEx,
  IDirectMusicTempoTrack_IDirectMusicTrack_GetParamEx,
  IDirectMusicTempoTrack_IDirectMusicTrack_SetParamEx,
  IDirectMusicTempoTrack_IDirectMusicTrack_Compose,
  IDirectMusicTempoTrack_IDirectMusicTrack_Join
};

/* IDirectMusicTempoTrack IPersistStream part: */
static HRESULT WINAPI IDirectMusicTempoTrack_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj) {
  ICOM_THIS_MULTI(IDirectMusicTempoTrack, PersistStreamVtbl, iface);
  return IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicTempoTrack_IPersistStream_AddRef (LPPERSISTSTREAM iface) {
  ICOM_THIS_MULTI(IDirectMusicTempoTrack, PersistStreamVtbl, iface);
  return IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

static ULONG WINAPI IDirectMusicTempoTrack_IPersistStream_Release (LPPERSISTSTREAM iface) {
  ICOM_THIS_MULTI(IDirectMusicTempoTrack, PersistStreamVtbl, iface);
  return IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

static HRESULT WINAPI IDirectMusicTempoTrack_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID) {
  ICOM_THIS_MULTI(IDirectMusicSegment8Impl, PersistStreamVtbl, iface);
  TRACE("(%p, %p)\n", This, pClassID);
  memcpy(pClassID, &CLSID_DirectMusicTempoTrack, sizeof(CLSID));
  return S_OK;
}

static HRESULT WINAPI IDirectMusicTempoTrack_IPersistStream_IsDirty (LPPERSISTSTREAM iface) {
  ICOM_THIS_MULTI(IDirectMusicTempoTrack, PersistStreamVtbl, iface);
  FIXME("(%p): stub, always S_FALSE\n", This);
  return S_FALSE;
}

static HRESULT WINAPI IDirectMusicTempoTrack_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm) {
  ICOM_THIS_MULTI(IDirectMusicTempoTrack, PersistStreamVtbl, iface);
  DMUS_PRIVATE_CHUNK Chunk;
  DWORD StreamSize, StreamCount;
  LARGE_INTEGER liMove;
  DMUS_IO_TEMPO_ITEM item;
  LPDMUS_PRIVATE_TEMPO_ITEM pNewItem = NULL;
  DWORD nItem = 0;
  FIXME("(%p, %p): Loading not fully implemented yet\n", This, pStm);
  
#if 1
  IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
  TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
  switch (Chunk.fccID) {	
  case DMUS_FOURCC_TEMPO_TRACK: {
    TRACE_(dmfile)(": Tempo track\n");
#if 1
    IStream_Read (pStm, &StreamSize, sizeof(DWORD), NULL);
    StreamSize -= sizeof(DWORD);
    StreamCount = 0;
    TRACE_(dmfile)(" - sizeof(DMUS_IO_TEMPO_ITEM): %u (chunkSize = %u)\n", StreamSize, Chunk.dwSize);
    do {
      IStream_Read (pStm, &item, sizeof(item), NULL);
      ++nItem;
      TRACE_(dmfile)("DMUS_IO_TEMPO_ITEM #%d\n", nItem);
      TRACE_(dmfile)(" - lTime = %lu\n", item.lTime);
      TRACE_(dmfile)(" - dblTempo = %g\n", item.dblTempo);
      pNewItem = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(DMUS_PRIVATE_TEMPO_ITEM));
      if (NULL == pNewItem) {
	ERR(": no more memory\n");
	return  E_OUTOFMEMORY;
      }
      memcpy(&pNewItem->item, &item, sizeof(DMUS_IO_TEMPO_ITEM));
      list_add_tail (&This->Items, &pNewItem->entry);
      pNewItem = NULL;
      StreamCount += sizeof(item);
      TRACE_(dmfile)(": StreamCount[0] = %d < StreamSize[0] = %d\n", StreamCount, StreamSize);
    } while (StreamCount < StreamSize); 
#else    
    liMove.QuadPart = Chunk.dwSize;
    IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
#endif
    break;
  }
  default: {
    TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
    liMove.QuadPart = Chunk.dwSize;
    IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
    return E_FAIL;
  }
  }
#endif

  return S_OK;
}

static HRESULT WINAPI IDirectMusicTempoTrack_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty) {
  ICOM_THIS_MULTI(IDirectMusicTempoTrack, PersistStreamVtbl, iface);
  FIXME("(%p): Saving not implemented yet\n", This);
  return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicTempoTrack_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize) {
  ICOM_THIS_MULTI(IDirectMusicTempoTrack, PersistStreamVtbl, iface);
  FIXME("(%p, %p): stub\n", This, pcbSize);
  return E_NOTIMPL;
}

static const IPersistStreamVtbl DirectMusicTempoTrack_PersistStream_Vtbl = {
  IDirectMusicTempoTrack_IPersistStream_QueryInterface,
  IDirectMusicTempoTrack_IPersistStream_AddRef,
  IDirectMusicTempoTrack_IPersistStream_Release,
  IDirectMusicTempoTrack_IPersistStream_GetClassID,
  IDirectMusicTempoTrack_IPersistStream_IsDirty,
  IDirectMusicTempoTrack_IPersistStream_Load,
  IDirectMusicTempoTrack_IPersistStream_Save,
  IDirectMusicTempoTrack_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicTempoTrack (LPCGUID lpcGUID, LPVOID *ppobj, LPUNKNOWN pUnkOuter) {
  IDirectMusicTempoTrack* track;
	
  track = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicTempoTrack));
  if (NULL == track) {
    *ppobj = NULL;
    return E_OUTOFMEMORY;
  }
  track->UnknownVtbl = &DirectMusicTempoTrack_Unknown_Vtbl;
  track->TrackVtbl = &DirectMusicTempoTrack_Track_Vtbl;
  track->PersistStreamVtbl = &DirectMusicTempoTrack_PersistStream_Vtbl;
  track->pDesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DMUS_OBJECTDESC));
  DM_STRUCT_INIT(track->pDesc);
  track->pDesc->dwValidData |= DMUS_OBJ_CLASS;
  memcpy (&track->pDesc->guidClass, &CLSID_DirectMusicTempoTrack, sizeof (CLSID));
  track->ref = 0; /* will be inited by QueryInterface */
  track->enabled = TRUE;
  list_init (&track->Items);

  return IDirectMusicTempoTrack_IUnknown_QueryInterface ((LPUNKNOWN)&track->UnknownVtbl, lpcGUID, ppobj);
}
