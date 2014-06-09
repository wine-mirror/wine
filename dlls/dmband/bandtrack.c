/* IDirectMusicBandTrack Implementation
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
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

#include "dmband_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmband);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

/*****************************************************************************
 * IDirectMusicBandTrack implementation
 */
typedef struct IDirectMusicBandTrack {
    const IUnknownVtbl *UnknownVtbl;
    const IDirectMusicTrack8Vtbl *TrackVtbl;
    const IPersistStreamVtbl *PersistStreamVtbl;
    LONG ref;
    DMUS_OBJECTDESC *pDesc;
    DMUS_IO_BAND_TRACK_HEADER header;
    struct list Bands;
} IDirectMusicBandTrack;

static HRESULT WINAPI IDirectMusicBandTrack_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicBandTrack, UnknownVtbl, iface);
	TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);

	if (IsEqualIID (riid, &IID_IUnknown)) {
		*ppobj = &This->UnknownVtbl;
		IUnknown_AddRef (iface);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IDirectMusicTrack)
	  || IsEqualIID (riid, &IID_IDirectMusicTrack8)) {
		*ppobj = (LPDIRECTMUSICTRACK8)&This->TrackVtbl;
		IUnknown_AddRef (iface);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IPersistStream)) {
		*ppobj = &This->PersistStreamVtbl;
		IUnknown_AddRef (iface);
		return S_OK;
	}
	
	WARN("(%p, %s,%p): not found\n", This, debugstr_dmguid(riid), ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicBandTrack_IUnknown_AddRef (LPUNKNOWN iface) {
  ICOM_THIS_MULTI(IDirectMusicBandTrack, UnknownVtbl, iface);
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("(%p) : AddRef from %d\n", This, ref - 1);

  DMBAND_LockModule();

  return ref;
}

static ULONG WINAPI IDirectMusicBandTrack_IUnknown_Release (LPUNKNOWN iface) {
  ICOM_THIS_MULTI(IDirectMusicBandTrack, UnknownVtbl, iface);
  ULONG ref = InterlockedDecrement(&This->ref);

  TRACE("(%p) : ReleaseRef to %d\n", This, ref);

  if (ref == 0) {
    HeapFree(GetProcessHeap(), 0, This);
  }

  DMBAND_UnlockModule();
  
  return ref;
}

static const IUnknownVtbl DirectMusicBandTrack_Unknown_Vtbl = {
  IDirectMusicBandTrack_IUnknown_QueryInterface,
  IDirectMusicBandTrack_IUnknown_AddRef,
  IDirectMusicBandTrack_IUnknown_Release
};

/* IDirectMusicBandTrack IDirectMusicTrack8 part: */
static HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj) {
  ICOM_THIS_MULTI(IDirectMusicBandTrack, TrackVtbl, iface);
  return IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicBandTrack_IDirectMusicTrack_AddRef (LPDIRECTMUSICTRACK8 iface) {
  ICOM_THIS_MULTI(IDirectMusicBandTrack, TrackVtbl, iface);
  return IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

static ULONG WINAPI IDirectMusicBandTrack_IDirectMusicTrack_Release (LPDIRECTMUSICTRACK8 iface) {
  ICOM_THIS_MULTI(IDirectMusicBandTrack, TrackVtbl, iface);
  return IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

static HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment) {
  ICOM_THIS_MULTI(IDirectMusicBandTrack, TrackVtbl, iface);
  FIXME("(%p, %p): stub\n", This, pSegment);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_InitPlay(LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* segment_state, IDirectMusicPerformance* performance, void** state_data, DWORD virtual_track8id, DWORD flags)
{
    ICOM_THIS_MULTI(IDirectMusicBandTrack, TrackVtbl, iface);

    FIXME("(%p, %p, %p, %p, %d, %x): stub\n", This, segment_state, performance, state_data, virtual_track8id, flags);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData) {
  ICOM_THIS_MULTI(IDirectMusicBandTrack, TrackVtbl, iface);
  FIXME("(%p, %p): stub\n", This, pStateData);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_Play(LPDIRECTMUSICTRACK8 iface, void* state_data, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD flags, IDirectMusicPerformance* performance, IDirectMusicSegmentState* segment_state, DWORD virtual_id)
{
    ICOM_THIS_MULTI(IDirectMusicBandTrack, TrackVtbl, iface);

    FIXME("(%p, %p, %d, %d, %d, %x, %p, %p, %d): semi-stub\n", This, state_data, mtStart, mtEnd, mtOffset, flags, performance, segment_state, virtual_id);

    /* Sends following pMSG:
       - DMUS_PATCH_PMSG
       - DMUS_TRANSPOSE_PMSG
       - DMUS_CHANNEL_PRIORITY_PMSG
       - DMUS_MIDI_PMSG
    */

    return S_OK;
}

static HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam) {
  ICOM_THIS_MULTI(IDirectMusicBandTrack, TrackVtbl, iface);
  FIXME("(%p, %s, %d, %p, %p): stub\n", This, debugstr_dmguid(rguidType), mtTime, pmtNext, pParam);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam) {
  ICOM_THIS_MULTI(IDirectMusicBandTrack, TrackVtbl, iface);
  FIXME("(%p, %s, %d, %p): stub\n", This, debugstr_dmguid(rguidType), mtTime, pParam);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType) {
  ICOM_THIS_MULTI(IDirectMusicBandTrack, TrackVtbl, iface);
  
  TRACE("(%p, %s)\n", This, debugstr_dmguid(rguidType));
  if (IsEqualGUID (rguidType, &GUID_BandParam)
      || IsEqualGUID (rguidType, &GUID_Clear_All_Bands)
      || IsEqualGUID (rguidType, &GUID_ConnectToDLSCollection)
      || IsEqualGUID (rguidType, &GUID_Disable_Auto_Download)
      || IsEqualGUID (rguidType, &GUID_Download)
      || IsEqualGUID (rguidType, &GUID_DownloadToAudioPath)
      || IsEqualGUID (rguidType, &GUID_Enable_Auto_Download)
      || IsEqualGUID (rguidType, &GUID_IDirectMusicBand)
      || IsEqualGUID (rguidType, &GUID_StandardMIDIFile)
      || IsEqualGUID (rguidType, &GUID_Unload)
      || IsEqualGUID (rguidType, &GUID_UnloadFromAudioPath)) {
    TRACE("param supported\n");
    return S_OK;
  }
  
  TRACE("param unsupported\n");
  return DMUS_E_TYPE_UNSUPPORTED;
}

static HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType) {
  ICOM_THIS_MULTI(IDirectMusicBandTrack, TrackVtbl, iface);
  FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
  return S_OK;
}

static HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType) {
  ICOM_THIS_MULTI(IDirectMusicBandTrack, TrackVtbl, iface);
  FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
  return S_OK;
}

static HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack) {
  ICOM_THIS_MULTI(IDirectMusicBandTrack, TrackVtbl, iface);
  FIXME("(%p, %d, %d, %p): stub\n", This, mtStart, mtEnd, ppTrack);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_PlayEx(LPDIRECTMUSICTRACK8 iface, void* state_data, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD flags, IDirectMusicPerformance* performance, IDirectMusicSegmentState* segment_state, DWORD virtual_id)
{
    ICOM_THIS_MULTI(IDirectMusicBandTrack, TrackVtbl, iface);

    FIXME("(%p, %p, 0x%s, 0x%s, 0x%s, %x, %p, %p, %d): stub\n", This, state_data, wine_dbgstr_longlong(rtStart),
        wine_dbgstr_longlong(rtEnd), wine_dbgstr_longlong(rtOffset), flags, performance, segment_state, virtual_id);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_GetParamEx(LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* rtNext, void* param, void* state_data, DWORD flags)
{
    ICOM_THIS_MULTI(IDirectMusicBandTrack, TrackVtbl, iface);

    FIXME("(%p, %s, 0x%s, %p, %p, %p, %x): stub\n", This, debugstr_dmguid(rguidType),
        wine_dbgstr_longlong(rtTime), rtNext, param, state_data, flags);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_SetParamEx(LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* param, void* state_data, DWORD flags)
{
    ICOM_THIS_MULTI(IDirectMusicBandTrack, TrackVtbl, iface);

    FIXME("(%p, %s, 0x%s, %p, %p, %x): stub\n", This, debugstr_dmguid(rguidType),
        wine_dbgstr_longlong(rtTime), param, state_data, flags);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack) {
  ICOM_THIS_MULTI(IDirectMusicBandTrack, TrackVtbl, iface);
  FIXME("(%p, %p, %d, %p): stub\n", This, pContext, dwTrackGroup, ppResultTrack);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack) {
  ICOM_THIS_MULTI(IDirectMusicBandTrack, TrackVtbl, iface);
  FIXME("(%p, %p, %d, %p, %d, %p): stub\n", This, pNewTrack, mtJoin, pContext, dwTrackGroup, ppResultTrack);
  return S_OK;
}

static const IDirectMusicTrack8Vtbl DirectMusicBandTrack_DirectMusicTrack_Vtbl = {
  IDirectMusicBandTrack_IDirectMusicTrack_QueryInterface,
  IDirectMusicBandTrack_IDirectMusicTrack_AddRef,
  IDirectMusicBandTrack_IDirectMusicTrack_Release,
  IDirectMusicBandTrack_IDirectMusicTrack_Init,
  IDirectMusicBandTrack_IDirectMusicTrack_InitPlay,
  IDirectMusicBandTrack_IDirectMusicTrack_EndPlay,
  IDirectMusicBandTrack_IDirectMusicTrack_Play,
  IDirectMusicBandTrack_IDirectMusicTrack_GetParam,
  IDirectMusicBandTrack_IDirectMusicTrack_SetParam,
  IDirectMusicBandTrack_IDirectMusicTrack_IsParamSupported,
  IDirectMusicBandTrack_IDirectMusicTrack_AddNotificationType,
  IDirectMusicBandTrack_IDirectMusicTrack_RemoveNotificationType,
  IDirectMusicBandTrack_IDirectMusicTrack_Clone,
  IDirectMusicBandTrack_IDirectMusicTrack_PlayEx,
  IDirectMusicBandTrack_IDirectMusicTrack_GetParamEx,
  IDirectMusicBandTrack_IDirectMusicTrack_SetParamEx,
  IDirectMusicBandTrack_IDirectMusicTrack_Compose,
  IDirectMusicBandTrack_IDirectMusicTrack_Join
};

/* IDirectMusicBandTrack IPersistStream part: */
static HRESULT WINAPI IDirectMusicBandTrack_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj) {
  ICOM_THIS_MULTI(IDirectMusicBandTrack, PersistStreamVtbl, iface);
  return IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicBandTrack_IPersistStream_AddRef (LPPERSISTSTREAM iface) {
  ICOM_THIS_MULTI(IDirectMusicBandTrack, PersistStreamVtbl, iface);
  return IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

static ULONG WINAPI IDirectMusicBandTrack_IPersistStream_Release (LPPERSISTSTREAM iface) {
  ICOM_THIS_MULTI(IDirectMusicBandTrack, PersistStreamVtbl, iface);
  return IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

static HRESULT WINAPI IDirectMusicBandTrack_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID) {
  ICOM_THIS_MULTI(IDirectMusicBandTrack, PersistStreamVtbl, iface);
  TRACE("(%p, %p)\n", This, pClassID);
  *pClassID = CLSID_DirectMusicBandTrack;
  return S_OK;
}

static HRESULT WINAPI IDirectMusicBandTrack_IPersistStream_IsDirty (LPPERSISTSTREAM iface) {
  ICOM_THIS_MULTI(IDirectMusicBandTrack, PersistStreamVtbl, iface);
  FIXME("(%p): stub, always S_FALSE\n", This);
  return S_FALSE;
}

static HRESULT IDirectMusicBandTrack_IPersistStream_LoadBand (LPPERSISTSTREAM iface, IStream* pClonedStream, IDirectMusicBand** ppBand,
							      DMUS_PRIVATE_BAND_ITEM_HEADER* pHeader) {

  ICOM_THIS_MULTI(IDirectMusicBandTrack, PersistStreamVtbl, iface);
  HRESULT hr = E_FAIL;
  IPersistStream* pPersistStream = NULL;
  
  hr = CoCreateInstance (&CLSID_DirectMusicBand, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicBand, (LPVOID*) ppBand);
  if (FAILED(hr)) {
    ERR(": could not create object\n");
    return hr;
  }
  /* acquire PersistStream interface */
  hr = IDirectMusicBand_QueryInterface (*ppBand, &IID_IPersistStream, (LPVOID*) &pPersistStream);
  if (FAILED(hr)) {
    ERR(": could not acquire IPersistStream\n");
    return hr;
  }
  /* load */
  hr = IPersistStream_Load (pPersistStream, pClonedStream);
  if (FAILED(hr)) {
    ERR(": failed to load object\n");
    return hr;
  }
  
  /* release all loading-related stuff */
  IPersistStream_Release (pPersistStream);

  /*
   * @TODO insert pBand into This
   */
  if (SUCCEEDED(hr)) {
    LPDMUS_PRIVATE_BAND pNewBand = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(DMUS_PRIVATE_BAND));
    if (NULL == pNewBand) {
      ERR(": no more memory\n");
      return  E_OUTOFMEMORY;
    }
    pNewBand->BandHeader = *pHeader;
    pNewBand->band = *ppBand;
    IDirectMusicBand_AddRef(*ppBand);
    list_add_tail (&This->Bands, &pNewBand->entry);
  }

  return S_OK;
}

static HRESULT IDirectMusicBandTrack_IPersistStream_ParseBandsList (LPPERSISTSTREAM iface, DMUS_PRIVATE_CHUNK* pChunk, IStream* pStm) {

  /*ICOM_THIS_MULTI(IDirectMusicBandTrack, PersistStreamVtbl, iface);*/
  HRESULT hr = E_FAIL;
  DMUS_PRIVATE_CHUNK Chunk;
  DWORD StreamSize, ListSize[3], ListCount[3];
  LARGE_INTEGER liMove; /* used when skipping chunks */

  IDirectMusicBand* pBand = NULL;
  DMUS_PRIVATE_BAND_ITEM_HEADER header;

  memset(&header, 0, sizeof header);

  if (pChunk->fccID != DMUS_FOURCC_BANDS_LIST) {
    ERR_(dmfile)(": %s chunk should be a BANDS list\n", debugstr_fourcc (pChunk->fccID));
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
      do {
	IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
	ListCount[1] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
	TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
	switch (Chunk.fccID) { 
	case DMUS_FOURCC_BANDITEM_CHUNK: {
	  DMUS_IO_BAND_ITEM_HEADER tmp_header;
	  TRACE_(dmfile)(": Band Item chunk v1\n");
	  
	  IStream_Read (pStm, &tmp_header, sizeof(DMUS_IO_BAND_ITEM_HEADER), NULL);
          TRACE_(dmfile)(" - lBandTime: %u\n", tmp_header.lBandTime);

	  header.dwVersion = 1;
	  header.lBandTime = tmp_header.lBandTime;
	  break;
	}
	case DMUS_FOURCC_BANDITEM_CHUNK2: { 
	  DMUS_IO_BAND_ITEM_HEADER2 tmp_header2;
	  TRACE_(dmfile)(": Band Item chunk v2\n");
	  
	  IStream_Read (pStm, &tmp_header2, sizeof(DMUS_IO_BAND_ITEM_HEADER2), NULL);
          TRACE_(dmfile)(" - lBandTimeLogical: %u\n", tmp_header2.lBandTimeLogical);
          TRACE_(dmfile)(" - lBandTimePhysical: %u\n", tmp_header2.lBandTimePhysical);

	  header.dwVersion = 2;
	  header.lBandTimeLogical = tmp_header2.lBandTimeLogical;
	  header.lBandTimePhysical = tmp_header2.lBandTimePhysical;
	  break;
	}
	case FOURCC_RIFF: { 
	  IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);
	  TRACE_(dmfile)(": RIFF chunk of type %s\n", debugstr_fourcc(Chunk.fccID));
	  StreamSize = Chunk.dwSize - sizeof(FOURCC);
	  switch (Chunk.fccID) {
	  case DMUS_FOURCC_BAND_FORM: {
	    LPSTREAM pClonedStream = NULL;
	    TRACE_(dmfile)(": BAND RIFF\n");
	    
	    IStream_Clone (pStm, &pClonedStream);
	    
	    liMove.QuadPart = 0;
	    liMove.QuadPart -= sizeof(FOURCC) + (sizeof(FOURCC)+sizeof(DWORD));
	    IStream_Seek (pClonedStream, liMove, STREAM_SEEK_CUR, NULL);
	    
	    hr = IDirectMusicBandTrack_IPersistStream_LoadBand (iface, pClonedStream, &pBand, &header);
	    if (FAILED(hr)) {
	      ERR(": could not load track\n");
	      return hr;
	    }
	    IStream_Release (pClonedStream);
	    
	    IDirectMusicTrack_Release(pBand); pBand = NULL; /* now we can release at as it inserted */
	    
	    /** now safe move the cursor */
	    liMove.QuadPart = StreamSize;
	    IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
	    break;
	  }
	  default: {
	    TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
	    liMove.QuadPart = StreamSize;
	    IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
	    break;						
	  }
	  }
	  break;
	}
	default: {
	  TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
	  liMove.QuadPart = Chunk.dwSize;
	  IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
	  break;						
	 }
	}
	TRACE_(dmfile)(": ListCount[1] = %d < ListSize[1] = %d\n", ListCount[1], ListSize[1]);
      } while (ListCount[1] < ListSize[1]);
      break;
    }
    default: {
      TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
      liMove.QuadPart = Chunk.dwSize;
      IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
      break;						
    }
    }
    TRACE_(dmfile)(": ListCount[0] = %d < ListSize[0] = %d\n", ListCount[0], ListSize[0]);
  } while (ListCount[0] < ListSize[0]);

  return S_OK;
}

static HRESULT IDirectMusicBandTrack_IPersistStream_ParseBandTrackForm (LPPERSISTSTREAM iface, DMUS_PRIVATE_CHUNK* pChunk, IStream* pStm) {

  ICOM_THIS_MULTI(IDirectMusicBandTrack, PersistStreamVtbl, iface);
  HRESULT hr = E_FAIL;
  DMUS_PRIVATE_CHUNK Chunk;
  DWORD StreamSize, StreamCount, ListSize[3], ListCount[3];
  LARGE_INTEGER liMove; /* used when skipping chunks */

  if (pChunk->fccID != DMUS_FOURCC_BANDTRACK_FORM) {
    ERR_(dmfile)(": %s chunk should be a BANDTRACK form\n", debugstr_fourcc (pChunk->fccID));
    return E_FAIL;
  }  

  StreamSize = pChunk->dwSize - sizeof(FOURCC);
  StreamCount = 0;

  do {
    IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
    StreamCount += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
    TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
    
    hr = IDirectMusicUtils_IPersistStream_ParseDescGeneric(&Chunk, pStm, This->pDesc);
    if (FAILED(hr)) return hr;

    if (hr == S_FALSE) {
      switch (Chunk.fccID) {
      case DMUS_FOURCC_BANDTRACK_CHUNK: {
	TRACE_(dmfile)(": BandTrack chunk\n");
	IStream_Read (pStm, &This->header, sizeof(DMUS_IO_BAND_TRACK_HEADER), NULL);
	TRACE_(dmfile)(" - bAutoDownload: %u\n", This->header.bAutoDownload);
	break;
      }   
      case FOURCC_LIST: {
	IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);
	TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(Chunk.fccID));
	ListSize[0] = Chunk.dwSize - sizeof(FOURCC);
	ListCount[0] = 0;
	switch (Chunk.fccID) {
	case DMUS_FOURCC_UNFO_LIST: { 
	  TRACE_(dmfile)(": UNFO list\n");
	  do {
	    IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
	    ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
            TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
	    
	    hr = IDirectMusicUtils_IPersistStream_ParseUNFOGeneric(&Chunk, pStm, This->pDesc);
	    if (FAILED(hr)) return hr;
	    
	    if (hr == S_FALSE) {
	      switch (Chunk.fccID) {
	      default: {
		TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
		liMove.QuadPart = Chunk.dwSize;
		IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
		break;				
	      }
	      }
	    }  
            TRACE_(dmfile)(": ListCount[0] = %d < ListSize[0] = %d\n", ListCount[0], ListSize[0]);
	  } while (ListCount[0] < ListSize[0]);
	  break;
	}
	case DMUS_FOURCC_BANDS_LIST: {
	  TRACE_(dmfile)(": TRACK list\n");
	  hr = IDirectMusicBandTrack_IPersistStream_ParseBandsList (iface, &Chunk, pStm);
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
	TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
	liMove.QuadPart = Chunk.dwSize;
	IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
	break;						
      }
      }
    }
    TRACE_(dmfile)(": StreamCount[0] = %d < StreamSize[0] = %d\n", StreamCount, StreamSize);
  } while (StreamCount < StreamSize);  

  return S_OK;
}


static HRESULT WINAPI IDirectMusicBandTrack_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm) {
  ICOM_THIS_MULTI(IDirectMusicBandTrack, PersistStreamVtbl, iface);

  DMUS_PRIVATE_CHUNK Chunk;
  LARGE_INTEGER liMove;
  HRESULT hr;

  TRACE("(%p, %p): Loading\n", This, pStm);

  IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
  TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
  switch (Chunk.fccID) {
  case FOURCC_RIFF: {
    IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);
    TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
    switch (Chunk.fccID) {
    case DMUS_FOURCC_BANDTRACK_FORM: {
      TRACE_(dmfile)(": Band track form\n");
      hr = IDirectMusicBandTrack_IPersistStream_ParseBandTrackForm (iface, &Chunk, pStm);
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

static HRESULT WINAPI IDirectMusicBandTrack_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty) {
  ICOM_THIS_MULTI(IDirectMusicBandTrack, PersistStreamVtbl, iface);
  FIXME("(%p): Saving not implemented yet\n", This);
  return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicBandTrack_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize) {
  ICOM_THIS_MULTI(IDirectMusicBandTrack, PersistStreamVtbl, iface);
  FIXME("(%p, %p): stub\n", This, pcbSize);
  return E_NOTIMPL;
}

static const IPersistStreamVtbl DirectMusicBandTrack_PerststStream_Vtbl = {
  IDirectMusicBandTrack_IPersistStream_QueryInterface,
  IDirectMusicBandTrack_IPersistStream_AddRef,
  IDirectMusicBandTrack_IPersistStream_Release,
  IDirectMusicBandTrack_IPersistStream_GetClassID,
  IDirectMusicBandTrack_IPersistStream_IsDirty,
  IDirectMusicBandTrack_IPersistStream_Load,
  IDirectMusicBandTrack_IPersistStream_Save,
  IDirectMusicBandTrack_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT WINAPI create_dmbandtrack(REFIID lpcGUID, void **ppobj)
{
  IDirectMusicBandTrack* track;
	
  track = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicBandTrack));
  if (NULL == track) {
    *ppobj = NULL;
    return E_OUTOFMEMORY;
  }
  track->UnknownVtbl = &DirectMusicBandTrack_Unknown_Vtbl;
  track->TrackVtbl = &DirectMusicBandTrack_DirectMusicTrack_Vtbl;
  track->PersistStreamVtbl = &DirectMusicBandTrack_PerststStream_Vtbl;
  track->pDesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DMUS_OBJECTDESC));
  DM_STRUCT_INIT(track->pDesc);
  track->pDesc->dwValidData |= DMUS_OBJ_CLASS;
  track->pDesc->guidClass = CLSID_DirectMusicBandTrack;
  track->ref = 0; /* will be inited by QueryInterface */
  list_init (&track->Bands);

  return IDirectMusicBandTrack_IUnknown_QueryInterface ((LPUNKNOWN)&track->UnknownVtbl, lpcGUID, ppobj);
}
