/* IDirectMusicSegTriggerTrack Implementation
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

#include "dmime_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmime);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

/*****************************************************************************
 * IDirectMusicSegTriggerTrack implementation
 */
/* IDirectMusicSegTriggerTrack IUnknown part: */
static HRESULT WINAPI IDirectMusicSegTriggerTrack_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, UnknownVtbl, iface);
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

static ULONG WINAPI IDirectMusicSegTriggerTrack_IUnknown_AddRef (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, UnknownVtbl, iface);
        ULONG ref = InterlockedIncrement(&This->ref);

	TRACE("(%p): AddRef from %d\n", This, ref - 1);

	DMIME_LockModule();

	return ref;
}

static ULONG WINAPI IDirectMusicSegTriggerTrack_IUnknown_Release (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, UnknownVtbl, iface);
	ULONG ref = InterlockedDecrement(&This->ref);
	TRACE("(%p): ReleaseRef to %d\n", This, ref);
	
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}

	DMIME_UnlockModule();
	
	return ref;
}

static const IUnknownVtbl DirectMusicSegTriggerTrack_Unknown_Vtbl = {
	IDirectMusicSegTriggerTrack_IUnknown_QueryInterface,
	IDirectMusicSegTriggerTrack_IUnknown_AddRef,
	IDirectMusicSegTriggerTrack_IUnknown_Release
};

/* IDirectMusicSegTriggerTrack IDirectMusicTrack8 part: */
static HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	return IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_AddRef (LPDIRECTMUSICTRACK8 iface) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	return IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

static ULONG WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_Release (LPDIRECTMUSICTRACK8 iface) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	return IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

static HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment)
{
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	FIXME("(%p, %p): stub\n", This, pSegment);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	FIXME("(%p, %p, %p, %p, %d, %d): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData)
{
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	FIXME("(%p, %p): stub\n", This, pStateData);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID)
{
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	FIXME("(%p, %p, %ld, %ld, %ld, %d, %p, %p, %d): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	FIXME("(%p, %s, %ld, %p, %p): stub\n", This, debugstr_dmguid(rguidType), mtTime, pmtNext, pParam);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	FIXME("(%p, %s, %ld, %p): stub\n", This, debugstr_dmguid(rguidType), mtTime, pParam);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);

	TRACE("(%p, %s)\n", This, debugstr_dmguid(rguidType));
	/* didn't find any params */
	TRACE("param unsupported\n");
	return DMUS_E_TYPE_UNSUPPORTED;
}

static HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	FIXME("(%p, %p, 0x%s, 0x%s, 0x%s, %d, %p, %p, %d): stub\n", This, pStateData, wine_dbgstr_longlong(rtStart),
	    wine_dbgstr_longlong(rtEnd), wine_dbgstr_longlong(rtOffset), dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	FIXME("(%p, %s, 0x%s, %p, %p, %p, %d): stub\n", This, debugstr_dmguid(rguidType),
	    wine_dbgstr_longlong(rtTime), prtNext, pParam, pStateData, dwFlags);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	FIXME("(%p, %s, 0x%s, %p, %p, %d): stub\n", This, debugstr_dmguid(rguidType),
	    wine_dbgstr_longlong(rtTime), pParam, pStateData, dwFlags);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	FIXME("(%p, %p, %d, %p): stub\n", This, pContext, dwTrackGroup, ppResultTrack);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	FIXME("(%p, %p, %ld, %p, %d, %p): stub\n", This, pNewTrack, mtJoin, pContext, dwTrackGroup, ppResultTrack);
	return S_OK;
}

static const IDirectMusicTrack8Vtbl DirectMusicSegTriggerTrack_Track_Vtbl = {
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_QueryInterface,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_AddRef,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_Release,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_Init,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_InitPlay,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_EndPlay,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_Play,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_GetParam,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_SetParam,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_IsParamSupported,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_AddNotificationType,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_RemoveNotificationType,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_Clone,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_PlayEx,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_GetParamEx,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_SetParamEx,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_Compose,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_Join
};

/* IDirectMusicSegTriggerTrack IPersistStream part: */
static HRESULT WINAPI IDirectMusicSegTriggerTrack_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj) {
  ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, PersistStreamVtbl, iface);
  return IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicSegTriggerTrack_IPersistStream_AddRef (LPPERSISTSTREAM iface) {
  ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, PersistStreamVtbl, iface);
  return IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

static ULONG WINAPI IDirectMusicSegTriggerTrack_IPersistStream_Release (LPPERSISTSTREAM iface) {
  ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, PersistStreamVtbl, iface);
  return IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

static HRESULT WINAPI IDirectMusicSegTriggerTrack_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID) {
  ICOM_THIS_MULTI(IDirectMusicSegment8Impl, PersistStreamVtbl, iface);
  TRACE("(%p, %p)\n", This, pClassID);
  memcpy(pClassID, &CLSID_DirectMusicSegTriggerTrack, sizeof(CLSID));
  return S_OK;
}

static HRESULT WINAPI IDirectMusicSegTriggerTrack_IPersistStream_IsDirty (LPPERSISTSTREAM iface) {
  ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, PersistStreamVtbl, iface);
  FIXME("(%p): stub, always S_FALSE\n", This);
  return S_FALSE;
}

static HRESULT IDirectMusicSegTriggerTrack_IPersistStream_ParseSegment (LPPERSISTSTREAM iface, DMUS_PRIVATE_CHUNK* pChunk, IStream* pStm) {
  ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, PersistStreamVtbl, iface);
  DMUS_PRIVATE_CHUNK Chunk;
  DWORD ListSize[3], ListCount[3];
  LARGE_INTEGER liMove; /* used when skipping chunks */
  HRESULT hr;

  IDirectMusicObject* pObject = NULL;
  LPDMUS_PRIVATE_SEGMENT_ITEM pNewItem = NULL;

  if (pChunk->fccID != DMUS_FOURCC_SEGMENT_LIST) {
    ERR_(dmfile)(": %s chunk should be a SEGMENT list\n", debugstr_fourcc (pChunk->fccID));
    return E_FAIL;
  }

  ListSize[0] = pChunk->dwSize - sizeof(FOURCC);
  ListCount[0] = 0;

  do {
    IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
    ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
    TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
    switch (Chunk.fccID) { 
    case DMUS_FOURCC_SEGMENTITEM_CHUNK: {
      TRACE_(dmfile)(": segment item chunk\n"); 
      /** alloc new item entry */
      pNewItem = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(DMUS_PRIVATE_SEGMENT_ITEM));
      if (NULL == pNewItem) {
	ERR(": no more memory\n");
	return  E_OUTOFMEMORY;
      }
      IStream_Read (pStm, &pNewItem->header, sizeof(DMUS_IO_SEGMENT_ITEM_HEADER), NULL);
      TRACE_(dmfile)(" - lTimePhysical: %lu\n", pNewItem->header.lTimeLogical);
      TRACE_(dmfile)(" - lTimePhysical: %lu\n", pNewItem->header.lTimePhysical);
      TRACE_(dmfile)(" - dwPlayFlags: 0x%08x\n", pNewItem->header.dwPlayFlags);
      TRACE_(dmfile)(" - dwFlags: 0x%08x\n", pNewItem->header.dwFlags);
      list_add_tail (&This->Items, &pNewItem->entry);
      break;
    }
    case DMUS_FOURCC_SEGMENTITEMNAME_CHUNK: {
      TRACE_(dmfile)(": segment item name chunk\n");
      IStream_Read (pStm, pNewItem->wszName, Chunk.dwSize, NULL);
      TRACE_(dmfile)(" - name: %s\n", debugstr_w(pNewItem->wszName));
      break;
    }
    case FOURCC_LIST: {
      IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);
      TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(Chunk.fccID));
      ListSize[1] = Chunk.dwSize - sizeof(FOURCC);
      ListCount[1] = 0;
      switch (Chunk.fccID) { 
      case DMUS_FOURCC_REF_LIST: {
	FIXME_(dmfile)(": DMRF (DM References) list\n");
	hr = IDirectMusicUtils_IPersistStream_ParseReference (iface,  &Chunk, pStm, &pObject);
	if (FAILED(hr)) {
	  ERR(": could not load Reference\n");
	  return hr;
	}
	pNewItem->pObject = pObject;
	break;						
      }
      default: {
	TRACE_(dmfile)(": unknown (skipping)\n");
	liMove.QuadPart = Chunk.dwSize - sizeof(FOURCC);
	IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
	break;						
      }
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

static HRESULT IDirectMusicSegTriggerTrack_IPersistStream_ParseSegmentsList (LPPERSISTSTREAM iface, DMUS_PRIVATE_CHUNK* pChunk, IStream* pStm) {
  /*ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, PersistStreamVtbl, iface);*/
  HRESULT hr = E_FAIL;
  DMUS_PRIVATE_CHUNK Chunk;
  DWORD ListSize[3], ListCount[3];
  LARGE_INTEGER liMove; /* used when skipping chunks */

  if (pChunk->fccID != DMUS_FOURCC_SEGMENTS_LIST) {
    ERR_(dmfile)(": %s chunk should be a SEGMENTS list\n", debugstr_fourcc (pChunk->fccID));
    return E_FAIL;
  }

  ListSize[0] = pChunk->dwSize - sizeof(FOURCC);
  ListCount[0] = 0;

  do {
    IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
    ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
    TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
    switch (Chunk.fccID) { 
    case FOURCC_LIST: {
      IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);
      TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(Chunk.fccID));
      ListSize[1] = Chunk.dwSize - sizeof(FOURCC);
      ListCount[1] = 0;
      switch (Chunk.fccID) { 
      case DMUS_FOURCC_SEGMENT_LIST: {
	TRACE_(dmfile)(": SEGMENT list\n"); 
	hr = IDirectMusicSegTriggerTrack_IPersistStream_ParseSegment (iface, &Chunk, pStm);
	if (FAILED(hr)) return hr;	
	break;
      }
      default: {
	TRACE_(dmfile)(": unknown (skipping)\n");
	liMove.QuadPart = Chunk.dwSize - sizeof(FOURCC);
	IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
	break;						
      }
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

static HRESULT IDirectMusicSegTriggerTrack_IPersistStream_ParseSegTrackList (LPPERSISTSTREAM iface, DMUS_PRIVATE_CHUNK* pChunk, IStream* pStm) {
  /*ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, PersistStreamVtbl, iface);*/
  HRESULT hr = E_FAIL;
  DMUS_PRIVATE_CHUNK Chunk;
  DWORD ListSize[3], ListCount[3];
  LARGE_INTEGER liMove; /* used when skipping chunks */

  if (pChunk->fccID != DMUS_FOURCC_SEGTRACK_LIST) {
    ERR_(dmfile)(": %s chunk should be a SEGTRACK list\n", debugstr_fourcc (pChunk->fccID));
    return E_FAIL;
  }  

  ListSize[0] = pChunk->dwSize - sizeof(FOURCC);
  ListCount[0] = 0;

  do {
    IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
    ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
    TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
    switch (Chunk.fccID) { 
    case DMUS_FOURCC_SEGTRACK_CHUNK: {
      TRACE_(dmfile)(": segment trigger track chunk\n");
      liMove.QuadPart = Chunk.dwSize;
      IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
      break;
    }
    case FOURCC_LIST: {
      IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);
      TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(Chunk.fccID));
      ListSize[1] = Chunk.dwSize - sizeof(FOURCC);
      ListCount[1] = 0;
      switch (Chunk.fccID) { 
      case DMUS_FOURCC_SEGMENTS_LIST: {
	TRACE_(dmfile)(": SEGMENTS list\n"); 
	hr = IDirectMusicSegTriggerTrack_IPersistStream_ParseSegmentsList (iface, &Chunk, pStm);
	if (FAILED(hr)) return hr;
	break;
      }
      default: {
	TRACE_(dmfile)(": unknown (skipping)\n");
	liMove.QuadPart = Chunk.dwSize - sizeof(FOURCC);
	IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
	break;						
      }
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

static HRESULT WINAPI IDirectMusicSegTriggerTrack_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm) {
  ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, PersistStreamVtbl, iface);

  DMUS_PRIVATE_CHUNK Chunk;
  LARGE_INTEGER liMove;
  HRESULT hr;

  TRACE("(%p, %p): Loading\n", This, pStm);
  
#if 1
  IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
  TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
  switch (Chunk.fccID) {	
  case FOURCC_LIST: {
    IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);
    TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
    switch (Chunk.fccID) {
    case DMUS_FOURCC_SEGTRACK_LIST: {
      TRACE_(dmfile)(": segment trigger track list\n");
      hr = IDirectMusicSegTriggerTrack_IPersistStream_ParseSegTrackList (iface, &Chunk, pStm);
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
#endif

  return S_OK;
}

static HRESULT WINAPI IDirectMusicSegTriggerTrack_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty) {
  ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, PersistStreamVtbl, iface);
  FIXME("(%p): Saving not implemented yet\n", This);
  return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicSegTriggerTrack_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize) {
  ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, PersistStreamVtbl, iface);
  FIXME("(%p, %p): stub\n", This, pcbSize);
  return E_NOTIMPL;
}

static const IPersistStreamVtbl DirectMusicSegTriggerTrack_PersistStream_Vtbl = {
  IDirectMusicSegTriggerTrack_IPersistStream_QueryInterface,
  IDirectMusicSegTriggerTrack_IPersistStream_AddRef,
  IDirectMusicSegTriggerTrack_IPersistStream_Release,
  IDirectMusicSegTriggerTrack_IPersistStream_GetClassID,
  IDirectMusicSegTriggerTrack_IPersistStream_IsDirty,
  IDirectMusicSegTriggerTrack_IPersistStream_Load,
  IDirectMusicSegTriggerTrack_IPersistStream_Save,
  IDirectMusicSegTriggerTrack_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicSegTriggerTrack (LPCGUID lpcGUID, LPVOID *ppobj, LPUNKNOWN pUnkOuter) {
  IDirectMusicSegTriggerTrack* track;
	
  track = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicSegTriggerTrack));
  if (NULL == track) {
    *ppobj = NULL;
    return E_OUTOFMEMORY;
  }
  track->UnknownVtbl = &DirectMusicSegTriggerTrack_Unknown_Vtbl;
  track->TrackVtbl = &DirectMusicSegTriggerTrack_Track_Vtbl;
  track->PersistStreamVtbl = &DirectMusicSegTriggerTrack_PersistStream_Vtbl;
  track->pDesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DMUS_OBJECTDESC));
  DM_STRUCT_INIT(track->pDesc);
  track->pDesc->dwValidData |= DMUS_OBJ_CLASS;
  memcpy (&track->pDesc->guidClass, &CLSID_DirectMusicSegTriggerTrack, sizeof (CLSID));
  track->ref = 0; /* will be inited by QueryInterface */
  list_init (&track->Items);

  return IDirectMusicSegTriggerTrack_IUnknown_QueryInterface ((LPUNKNOWN)&track->UnknownVtbl, lpcGUID, ppobj);
}
