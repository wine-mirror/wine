/* IDirectMusicSegment8 Implementation
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
#include "dmobject.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmime);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

/*****************************************************************************
 * IDirectMusicSegmentImpl implementation
 */
typedef struct IDirectMusicSegment8Impl {
    IDirectMusicSegment8 IDirectMusicSegment8_iface;
    struct dmobject dmobj;
    LONG ref;
    DMUS_IO_SEGMENT_HEADER header;
    IDirectMusicGraph *pGraph;
    struct list Tracks;
} IDirectMusicSegment8Impl;

static inline IDirectMusicSegment8Impl *impl_from_IDirectMusicSegment8(IDirectMusicSegment8 *iface)
{
  return CONTAINING_RECORD(iface, IDirectMusicSegment8Impl, IDirectMusicSegment8_iface);
}

static HRESULT WINAPI IDirectMusicSegment8Impl_QueryInterface(IDirectMusicSegment8 *iface,
        REFIID riid, void **ret_iface)
{
    IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);

    *ret_iface = NULL;

    if (IsEqualIID (riid, &IID_IUnknown) || IsEqualIID (riid, &IID_IDirectMusicSegment) ||
            IsEqualIID(riid, &IID_IDirectMusicSegment2) ||
            IsEqualIID (riid, &IID_IDirectMusicSegment8))
        *ret_iface = iface;
    else if (IsEqualIID (riid, &IID_IDirectMusicObject))
        *ret_iface = &This->dmobj.IDirectMusicObject_iface;
    else if (IsEqualIID (riid, &IID_IPersistStream))
        *ret_iface = &This->dmobj.IPersistStream_iface;
    else {
        WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ret_iface);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ret_iface);
    return S_OK;
}

static ULONG WINAPI IDirectMusicSegment8Impl_AddRef(IDirectMusicSegment8 *iface)
{
    IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirectMusicSegment8Impl_Release(IDirectMusicSegment8 *iface)
{
    IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if (!ref) {
        HeapFree(GetProcessHeap(), 0, This);
        DMIME_UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_GetLength(IDirectMusicSegment8 *iface,
        MUSIC_TIME *pmtLength)
{
  IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);

  TRACE("(%p, %p)\n", This, pmtLength);
  if (NULL == pmtLength) {
    return E_POINTER;
  }
  *pmtLength = This->header.mtLength;
  return S_OK;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_SetLength(IDirectMusicSegment8 *iface,
        MUSIC_TIME mtLength)
{
  IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);

  TRACE("(%p, %d)\n", This, mtLength);
  This->header.mtLength = mtLength;
  return S_OK;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_GetRepeats(IDirectMusicSegment8 *iface,
        DWORD *pdwRepeats)
{
  IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);

  TRACE("(%p, %p)\n", This, pdwRepeats);
  if (NULL == pdwRepeats) {
    return E_POINTER;
  }
  *pdwRepeats = This->header.dwRepeats;
  return S_OK;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_SetRepeats(IDirectMusicSegment8 *iface,
        DWORD dwRepeats)
{
  IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);

  TRACE("(%p, %d)\n", This, dwRepeats);
  This->header.dwRepeats = dwRepeats;
  return S_OK;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_GetDefaultResolution(IDirectMusicSegment8 *iface,
        DWORD *pdwResolution)
{
  IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);

  TRACE("(%p, %p)\n", This, pdwResolution);
  if (NULL == pdwResolution) {
    return E_POINTER;
  }
  *pdwResolution = This->header.dwResolution;
  return S_OK;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_SetDefaultResolution(IDirectMusicSegment8 *iface,
        DWORD dwResolution)
{
  IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);

  TRACE("(%p, %d)\n", This, dwResolution);
  This->header.dwResolution = dwResolution;
  return S_OK;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_GetTrack(IDirectMusicSegment8 *iface,
        REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, IDirectMusicTrack **ppTrack)
{
  IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);
  CLSID pIt_clsid;
  struct list* pEntry = NULL;
  LPDMUS_PRIVATE_SEGMENT_TRACK pIt = NULL;
  IPersistStream* pCLSIDStream = NULL;
  HRESULT hr = S_OK;

  TRACE("(%p, %s, %d, 0x%x, %p)\n", This, debugstr_dmguid(rguidType), dwGroupBits, dwIndex, ppTrack);

  if (NULL == ppTrack) {
    return E_POINTER;
  }

  LIST_FOR_EACH (pEntry, &This->Tracks) {
    pIt = LIST_ENTRY(pEntry, DMUS_PRIVATE_SEGMENT_TRACK, entry);
    TRACE(" - %p -> 0x%x,%p\n", pIt, pIt->dwGroupBits, pIt->pTrack);
    if (0xFFFFFFFF != dwGroupBits && 0 == (pIt->dwGroupBits & dwGroupBits)) continue ;
    if (FALSE == IsEqualGUID(&GUID_NULL, rguidType)) {
      /**
       * it rguidType is not null we must check if CLSIDs are equal
       * and the unique way to get it is using IPersistStream Interface
       */
      hr = IDirectMusicTrack_QueryInterface(pIt->pTrack, &IID_IPersistStream, (void**) &pCLSIDStream);
      if (FAILED(hr)) {
	ERR("(%p): object %p don't implement IPersistStream Interface. Expect a crash (critical problem)\n", This, pIt->pTrack);
	continue ;
      }
      hr = IPersistStream_GetClassID(pCLSIDStream, &pIt_clsid);
      IPersistStream_Release(pCLSIDStream); pCLSIDStream = NULL;
      if (FAILED(hr)) {
	ERR("(%p): non-implemented GetClassID for object %p\n", This, pIt->pTrack);
	continue ;
      }
      TRACE(" - %p -> %s\n", pIt, debugstr_dmguid(&pIt_clsid));
      if (FALSE == IsEqualGUID(&pIt_clsid, rguidType)) continue ;
    }
    if (0 == dwIndex) {
      *ppTrack = pIt->pTrack;
      IDirectMusicTrack_AddRef(*ppTrack);
      return S_OK;
    } 
    --dwIndex;
  }  
  return DMUS_E_NOT_FOUND;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_GetTrackGroup(IDirectMusicSegment8 *iface,
        IDirectMusicTrack *pTrack, DWORD *pdwGroupBits)
{
  IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);
  struct list* pEntry = NULL;
  LPDMUS_PRIVATE_SEGMENT_TRACK pIt = NULL;

  TRACE("(%p, %p, %p)\n", This, pTrack, pdwGroupBits);

  if (NULL == pdwGroupBits) {
    return E_POINTER;
  }

  LIST_FOR_EACH (pEntry, &This->Tracks) {
    pIt = LIST_ENTRY(pEntry, DMUS_PRIVATE_SEGMENT_TRACK, entry);
    TRACE(" - %p -> %d,%p\n", pIt, pIt->dwGroupBits, pIt->pTrack);
    if (NULL != pIt && pIt->pTrack == pTrack) {
      *pdwGroupBits = pIt->dwGroupBits;
      return S_OK;
    }
  }

  return DMUS_E_NOT_FOUND;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_InsertTrack(IDirectMusicSegment8 *iface,
        IDirectMusicTrack *pTrack, DWORD dwGroupBits)
{
  IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);
  DWORD i = 0;
  struct list* pEntry = NULL;
  LPDMUS_PRIVATE_SEGMENT_TRACK pIt = NULL;
  LPDMUS_PRIVATE_SEGMENT_TRACK pNewSegTrack = NULL;

  TRACE("(%p, %p, %d)\n", This, pTrack, dwGroupBits);

  LIST_FOR_EACH (pEntry, &This->Tracks) {
    i++;
    pIt = LIST_ENTRY(pEntry, DMUS_PRIVATE_SEGMENT_TRACK, entry);
    TRACE(" - #%u: %p -> %d,%p\n", i, pIt, pIt->dwGroupBits, pIt->pTrack);
    if (NULL != pIt && pIt->pTrack == pTrack) {
      ERR("(%p, %p): track is already in list\n", This, pTrack);
      return E_FAIL;
    }
  }

  pNewSegTrack = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(DMUS_PRIVATE_SEGMENT_TRACK));
  if (NULL == pNewSegTrack)
    return  E_OUTOFMEMORY;

  pNewSegTrack->dwGroupBits = dwGroupBits;
  pNewSegTrack->pTrack = pTrack;
  IDirectMusicTrack_Init(pTrack, (IDirectMusicSegment *)iface);
  IDirectMusicTrack_AddRef(pTrack);
  list_add_tail (&This->Tracks, &pNewSegTrack->entry);

  return S_OK;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_RemoveTrack(IDirectMusicSegment8 *iface,
        IDirectMusicTrack *pTrack)
{
  IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);
  struct list* pEntry = NULL;
  LPDMUS_PRIVATE_SEGMENT_TRACK pIt = NULL;

  TRACE("(%p, %p)\n", This, pTrack);

  LIST_FOR_EACH (pEntry, &This->Tracks) {
    pIt = LIST_ENTRY(pEntry, DMUS_PRIVATE_SEGMENT_TRACK, entry);
    if (pIt->pTrack == pTrack) {
      TRACE("(%p, %p): track in list\n", This, pTrack);
      
      list_remove(&pIt->entry);
      IDirectMusicTrack_Init(pIt->pTrack, NULL);
      IDirectMusicTrack_Release(pIt->pTrack);
      HeapFree(GetProcessHeap(), 0, pIt);   

      return S_OK;
    }
  }
  
  return S_FALSE;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_InitPlay(IDirectMusicSegment8 *iface,
        IDirectMusicSegmentState **ppSegState, IDirectMusicPerformance *pPerformance, DWORD dwFlags)
{
  IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);
  HRESULT hr;

  FIXME("(%p, %p, %p, %d): semi-stub\n", This, ppSegState, pPerformance, dwFlags);
  if (NULL == ppSegState) {
    return E_POINTER;
  }
  hr = create_dmsegmentstate(&IID_IDirectMusicSegmentState,(void**) ppSegState);
  if (FAILED(hr)) {
    return hr;
  }
  /* TODO: DMUS_SEGF_FLAGS */
  return S_OK;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_GetGraph(IDirectMusicSegment8 *iface,
        IDirectMusicGraph **ppGraph)
{
  IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);

  FIXME("(%p, %p): semi-stub\n", This, ppGraph);
  if (NULL == ppGraph) {
    return E_POINTER;
  }
  if (NULL == This->pGraph) {
    return DMUS_E_NOT_FOUND;
  }
  /** 
   * should return This, as seen in msdn 
   * "...The segment object implements IDirectMusicGraph directly..."
   */
  *ppGraph = This->pGraph;
  IDirectMusicGraph_AddRef(This->pGraph);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_SetGraph(IDirectMusicSegment8 *iface,
        IDirectMusicGraph *pGraph)
{
  IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);

  FIXME("(%p, %p): to complete\n", This, pGraph);
  if (NULL != This->pGraph) {
    IDirectMusicGraph_Release(This->pGraph);
  }
  This->pGraph = pGraph;
  if (NULL != This->pGraph) {
    IDirectMusicGraph_AddRef(This->pGraph);
  }
  return S_OK;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_AddNotificationType(IDirectMusicSegment8 *iface,
        REFGUID rguidNotificationType)
{
  IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);
  FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
  return S_OK;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_RemoveNotificationType(IDirectMusicSegment8 *iface,
        REFGUID rguidNotificationType)
{
  IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);
  FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
  return S_OK;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_GetParam(IDirectMusicSegment8 *iface,
        REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, MUSIC_TIME *pmtNext,
        void *pParam)
{
  IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);
  CLSID pIt_clsid;
  struct list* pEntry = NULL;
  IDirectMusicTrack* pTrack = NULL;
  IPersistStream* pCLSIDStream = NULL;
  LPDMUS_PRIVATE_SEGMENT_TRACK pIt = NULL;
  HRESULT hr = S_OK;

  FIXME("(%p, %s, 0x%x, %d, %d, %p, %p)\n", This, debugstr_dmguid(rguidType), dwGroupBits, dwIndex, mtTime, pmtNext, pParam);
  
  if (DMUS_SEG_ANYTRACK == dwIndex) {
    
    LIST_FOR_EACH (pEntry, &This->Tracks) {
      pIt = LIST_ENTRY(pEntry, DMUS_PRIVATE_SEGMENT_TRACK, entry);

      hr = IDirectMusicTrack_QueryInterface(pIt->pTrack, &IID_IPersistStream, (void**) &pCLSIDStream);
      if (FAILED(hr)) {
	ERR("(%p): object %p don't implement IPersistStream Interface. Expect a crash (critical problem)\n", This, pIt->pTrack);
	continue ;
      }

      TRACE(" - %p -> 0x%x,%p\n", pIt, pIt->dwGroupBits, pIt->pTrack);

      if (0xFFFFFFFF != dwGroupBits && 0 == (pIt->dwGroupBits & dwGroupBits)) continue ;
      hr = IPersistStream_GetClassID(pCLSIDStream, &pIt_clsid);
      IPersistStream_Release(pCLSIDStream); pCLSIDStream = NULL;
      if (FAILED(hr)) {
	ERR("(%p): non-implemented GetClassID for object %p\n", This, pIt->pTrack);
	continue ;
      }
      if (FALSE == IsEqualGUID(&pIt_clsid, rguidType)) continue ;
      if (FAILED(IDirectMusicTrack_IsParamSupported(pIt->pTrack, rguidType))) continue ;
      hr = IDirectMusicTrack_GetParam(pIt->pTrack, rguidType, mtTime, pmtNext, pParam);
      if (SUCCEEDED(hr)) return hr;
    }
    ERR("(%p): not found\n", This);
    return DMUS_E_TRACK_NOT_FOUND;
  } 

  hr = IDirectMusicSegment8Impl_GetTrack(iface, &GUID_NULL, dwGroupBits, dwIndex, &pTrack);
  if (FAILED(hr)) {
    ERR("(%p): not found\n", This);
    return DMUS_E_TRACK_NOT_FOUND;
  }

  hr = IDirectMusicTrack_GetParam(pTrack, rguidType, mtTime, pmtNext, pParam);
  IDirectMusicTrack_Release(pTrack); pTrack = NULL;

  return hr;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_SetParam(IDirectMusicSegment8 *iface,
        REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, void *pParam)
{
  IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);
  FIXME("(%p, %s, %d, %d, %d, %p): stub\n", This, debugstr_dmguid(rguidType), dwGroupBits, dwIndex, mtTime, pParam);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_Clone(IDirectMusicSegment8 *iface,
        MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicSegment **ppSegment)
{
  IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);
  FIXME("(%p, %d, %d, %p): stub\n", This, mtStart, mtEnd, ppSegment);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_SetStartPoint(IDirectMusicSegment8 *iface,
        MUSIC_TIME mtStart)
{
  IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);

  TRACE("(%p, %d)\n", This, mtStart);
  if (mtStart >= This->header.mtLength) {
    return DMUS_E_OUT_OF_RANGE;
  }
  This->header.mtPlayStart = mtStart;
  return S_OK;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_GetStartPoint(IDirectMusicSegment8 *iface,
        MUSIC_TIME *pmtStart)
{
  IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);

  TRACE("(%p, %p)\n", This, pmtStart);
  if (NULL == pmtStart) {
    return E_POINTER;
  }
  *pmtStart = This->header.mtPlayStart;
  return S_OK;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_SetLoopPoints(IDirectMusicSegment8 *iface,
        MUSIC_TIME mtStart, MUSIC_TIME mtEnd)
{
  IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);

  TRACE("(%p, %d, %d)\n", This, mtStart, mtEnd);
  if (mtStart >= This->header.mtLength || mtEnd > This->header.mtLength || mtStart > mtEnd) {
    return DMUS_E_OUT_OF_RANGE;
  }
  This->header.mtLoopStart = mtStart;
  This->header.mtLoopEnd   = mtEnd;
  return S_OK;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_GetLoopPoints(IDirectMusicSegment8 *iface,
        MUSIC_TIME *pmtStart, MUSIC_TIME *pmtEnd)
{
  IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);

  TRACE("(%p, %p, %p)\n", This, pmtStart, pmtEnd);
  if (NULL == pmtStart || NULL == pmtEnd) {
    return E_POINTER;
  }
  *pmtStart = This->header.mtLoopStart;
  *pmtEnd   = This->header.mtLoopEnd;
  return S_OK;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_SetPChannelsUsed(IDirectMusicSegment8 *iface,
        DWORD dwNumPChannels, DWORD *paPChannels)
{
        IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);
	FIXME("(%p, %d, %p): stub\n", This, dwNumPChannels, paPChannels);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_SetTrackConfig(IDirectMusicSegment8 *iface,
        REFGUID rguidTrackClassID, DWORD dwGroupBits, DWORD dwIndex, DWORD dwFlagsOn,
        DWORD dwFlagsOff)
{
        IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);
	FIXME("(%p, %s, %d, %d, %d, %d): stub\n", This, debugstr_dmguid(rguidTrackClassID), dwGroupBits, dwIndex, dwFlagsOn, dwFlagsOff);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_GetAudioPathConfig(IDirectMusicSegment8 *iface,
        IUnknown **ppAudioPathConfig)
{
        IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);
	FIXME("(%p, %p): stub\n", This, ppAudioPathConfig);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_Compose(IDirectMusicSegment8 *iface,
        MUSIC_TIME mtTime, IDirectMusicSegment *pFromSegment, IDirectMusicSegment *pToSegment,
        IDirectMusicSegment **ppComposedSegment)
{
        IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);
	FIXME("(%p, %d, %p, %p, %p): stub\n", This, mtTime, pFromSegment, pToSegment, ppComposedSegment);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_Download(IDirectMusicSegment8 *iface,
        IUnknown *pAudioPath)
{
        IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);
	FIXME("(%p, %p): stub\n", This, pAudioPath);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_Unload(IDirectMusicSegment8 *iface,
        IUnknown *pAudioPath)
{
        IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);
	FIXME("(%p, %p): stub\n", This, pAudioPath);
	return S_OK;
}

static const IDirectMusicSegment8Vtbl dmsegment8_vtbl = {
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

/* IDirectMusicSegment8Impl IDirectMusicObject part: */
static HRESULT WINAPI IDirectMusicObjectImpl_ParseDescriptor(IDirectMusicObject *iface,
        IStream *pStream, DMUS_OBJECTDESC *pDesc)
{
	DMUS_PRIVATE_CHUNK Chunk;
	DWORD StreamSize, StreamCount, ListSize[1], ListCount[1];
	LARGE_INTEGER liMove; /* used when skipping chunks */

        TRACE("(%p,%p, %p)\n", iface, pStream, pDesc);

	/* FIXME: should this be determined from stream? */
	pDesc->dwValidData |= DMUS_OBJ_CLASS;
	pDesc->guidClass = CLSID_DirectMusicSegment;

	IStream_Read (pStream, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
	TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
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
					TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
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
										TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
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
												TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
												liMove.QuadPart = Chunk.dwSize;
												IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL);
												break;						
											}
										}
										TRACE_(dmfile)(": ListCount[0] = %d < ListSize[0] = %d\n", ListCount[0], ListSize[0]);
									} while (ListCount[0] < ListSize[0]);
									break;
								}
								case DMUS_FOURCC_TRACK_LIST: {
								  TRACE_(dmfile)(": TRACK list\n");
								  do {
								    IStream_Read (pStream, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
								    ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
                                                                    TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
								    switch (Chunk.fccID) {
								    default: {
								      TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
								      liMove.QuadPart = Chunk.dwSize;
								      IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL);
								      break;						
								    }
								    }
                                                                    TRACE_(dmfile)(": ListCount[0] = %d < ListSize[0] = %d\n", ListCount[0], ListSize[0]);
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
							TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
							liMove.QuadPart = Chunk.dwSize;
							IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL);
							break;						
						}
					}
					TRACE_(dmfile)(": StreamCount[0] = %d < StreamSize[0] = %d\n", StreamCount, StreamSize);
				} while (StreamCount < StreamSize);
				break;
			} else if (Chunk.fccID == mmioFOURCC('W','A','V','E')) {
				TRACE_(dmfile)(": wave form (loading not yet implemented)\n");
				liMove.QuadPart = StreamSize;
				IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
			} else {
				TRACE_(dmfile)(": unexpected chunk (loading failed)\n");
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
	
	TRACE(": returning descriptor:\n%s\n", debugstr_DMUS_OBJECTDESC (pDesc));
	
	return S_OK;
}

static const IDirectMusicObjectVtbl dmobject_vtbl = {
    dmobj_IDirectMusicObject_QueryInterface,
    dmobj_IDirectMusicObject_AddRef,
    dmobj_IDirectMusicObject_Release,
    dmobj_IDirectMusicObject_GetDescriptor,
    dmobj_IDirectMusicObject_SetDescriptor,
    IDirectMusicObjectImpl_ParseDescriptor
};

/* IDirectMusicSegment8Impl IPersistStream part: */
static HRESULT load_track(IDirectMusicSegment8Impl *This, IStream *pClonedStream,
        IDirectMusicTrack **ppTrack, DMUS_IO_TRACK_HEADER *pTrack_hdr)
{
  HRESULT hr = E_FAIL;
  IPersistStream* pPersistStream = NULL;
  
  hr = CoCreateInstance (&pTrack_hdr->guidClassID, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicTrack, (LPVOID*) ppTrack);
  if (FAILED(hr)) {
    ERR(": could not create object\n");
    return hr;
  }
  /* acquire PersistStream interface */
  hr = IDirectMusicTrack_QueryInterface (*ppTrack, &IID_IPersistStream, (LPVOID*) &pPersistStream);
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

  hr = IDirectMusicSegment8_InsertTrack(&This->IDirectMusicSegment8_iface, *ppTrack,
                                        pTrack_hdr->dwGroup); /* at dsPosition */
  if (FAILED(hr)) {
    ERR(": could not insert track\n");
    return hr;
  }

  return S_OK;
}

static HRESULT parse_track_form(IDirectMusicSegment8Impl *This, DMUS_PRIVATE_CHUNK *pChunk,
        IStream *pStm)
{
  HRESULT hr = E_FAIL;
  DMUS_PRIVATE_CHUNK Chunk;
  DWORD StreamSize, StreamCount, ListSize[3];
  LARGE_INTEGER liMove; /* used when skipping chunks */

  DMUS_IO_TRACK_HEADER        track_hdr;
  DMUS_IO_TRACK_EXTRAS_HEADER track_xhdr;
  IDirectMusicTrack*          pTrack = NULL;

  if (pChunk->fccID != DMUS_FOURCC_TRACK_FORM) {
    ERR_(dmfile)(": %s chunk should be a TRACK form\n", debugstr_fourcc (pChunk->fccID));
    return E_FAIL;
  }  

  StreamSize = pChunk->dwSize - sizeof(FOURCC);
  StreamCount = 0;

  do {
    IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
    StreamCount += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
    TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
    
    switch (Chunk.fccID) {
    case DMUS_FOURCC_TRACK_CHUNK: {
      TRACE_(dmfile)(": track chunk\n");
      IStream_Read (pStm, &track_hdr, sizeof(DMUS_IO_TRACK_HEADER), NULL);
      TRACE_(dmfile)(" - class: %s\n", debugstr_guid (&track_hdr.guidClassID));
      TRACE_(dmfile)(" - dwGroup: %d\n", track_hdr.dwGroup);
      TRACE_(dmfile)(" - ckid: %s\n", debugstr_fourcc (track_hdr.ckid));
      TRACE_(dmfile)(" - fccType: %s\n", debugstr_fourcc (track_hdr.fccType));
      break;
    }
    case DMUS_FOURCC_TRACK_EXTRAS_CHUNK: {
      TRACE_(dmfile)(": track extras chunk\n");
      IStream_Read (pStm, &track_xhdr, sizeof(DMUS_IO_TRACK_EXTRAS_HEADER), NULL);
      break;
    }

    case DMUS_FOURCC_COMMANDTRACK_CHUNK: {
      TRACE_(dmfile)(": COMMANDTRACK track\n");
      liMove.QuadPart = Chunk.dwSize;
      IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
      break;
    }

    case FOURCC_LIST: {
      IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);
      TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(Chunk.fccID));
      ListSize[0] = Chunk.dwSize - sizeof(FOURCC);
      if (Chunk.fccID == track_hdr.fccType && 0 == track_hdr.ckid) {
	LPSTREAM pClonedStream = NULL;

	TRACE_(dmfile)(": TRACK list\n");

	IStream_Clone (pStm, &pClonedStream);

	liMove.QuadPart = 0;
	liMove.QuadPart -= sizeof(FOURCC) + (sizeof(FOURCC)+sizeof(DWORD));
	IStream_Seek (pClonedStream, liMove, STREAM_SEEK_CUR, NULL);

        hr = load_track(This, pClonedStream, &pTrack, &track_hdr);
	if (FAILED(hr)) {
	  ERR(": could not load track\n");
	  return hr;
	}
	IStream_Release (pClonedStream);
	
	IDirectMusicTrack_Release(pTrack); pTrack = NULL; /* now we can release at as it inserted */

	liMove.QuadPart = ListSize[0];
	IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
	
      } else {
	TRACE_(dmfile)(": unknown (skipping)\n");
	liMove.QuadPart = Chunk.dwSize;
	IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
      }
      break;
    }

    case FOURCC_RIFF: {
      IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);
      TRACE_(dmfile)(": RIFF chunk of type %s\n", debugstr_fourcc(Chunk.fccID));

      ListSize[0] = Chunk.dwSize - sizeof(FOURCC);

      if (Chunk.fccID == track_hdr.fccType && 0 == track_hdr.ckid) {	
	LPSTREAM pClonedStream = NULL;

	TRACE_(dmfile)(": TRACK RIFF\n");

	IStream_Clone (pStm, &pClonedStream);
	
	liMove.QuadPart = 0;
	liMove.QuadPart -= sizeof(FOURCC) + (sizeof(FOURCC)+sizeof(DWORD));
	IStream_Seek (pClonedStream, liMove, STREAM_SEEK_CUR, NULL);

        hr = load_track(This, pClonedStream, &pTrack, &track_hdr);
	if (FAILED(hr)) {
	  ERR(": could not load track\n");
	  return hr;
	}
	IStream_Release (pClonedStream);
	
	IDirectMusicTrack_Release(pTrack); pTrack = NULL; /* now we can release at as it inserted */

	/** now safe move the cursor */
	liMove.QuadPart = ListSize[0];
	IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
	 
      } else {
	TRACE_(dmfile)(": unknown RIFF fmt (skipping)\n");
	liMove.QuadPart = ListSize[0];
	IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
      }
      break;
    }

    default: {
      if (0 == track_hdr.fccType && Chunk.fccID == track_hdr.ckid) {
	LPSTREAM pClonedStream = NULL;

	TRACE_(dmfile)(": TRACK solo\n");

	IStream_Clone (pStm, &pClonedStream);
	
	liMove.QuadPart = 0;
	liMove.QuadPart -= (sizeof(FOURCC) + sizeof(DWORD));
	IStream_Seek (pClonedStream, liMove, STREAM_SEEK_CUR, NULL);

        hr = load_track(This, pClonedStream, &pTrack, &track_hdr);
	if (FAILED(hr)) {
	  ERR(": could not load track\n");
	  return hr;
	}
	IStream_Release (pClonedStream);
	
	IDirectMusicTrack_Release(pTrack); pTrack = NULL; /* now we can release at as it inserted */

	liMove.QuadPart = Chunk.dwSize;
	IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);

	break;
      }

      TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
      liMove.QuadPart = Chunk.dwSize;
      IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
      break;						
    }
    }
    TRACE_(dmfile)(": StreamCount[0] = %d < StreamSize[0] = %d\n", StreamCount, StreamSize);
  } while (StreamCount < StreamSize);  

  return S_OK;
}

static HRESULT parse_track_list(IDirectMusicSegment8Impl *This, DMUS_PRIVATE_CHUNK *pChunk,
        IStream *pStm)
{
  HRESULT hr = E_FAIL;
  DMUS_PRIVATE_CHUNK Chunk;
  DWORD StreamSize, ListSize[3], ListCount[3];
  LARGE_INTEGER liMove; /* used when skipping chunks */

  if (pChunk->fccID != DMUS_FOURCC_TRACK_LIST) {
    ERR_(dmfile)(": %s chunk should be a TRACK list\n", debugstr_fourcc (pChunk->fccID));
    return E_FAIL;
  }  

  ListSize[0] = pChunk->dwSize - sizeof(FOURCC);
  ListCount[0] = 0;

  do {
    IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
    ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
    TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
    switch (Chunk.fccID) { 
    case FOURCC_RIFF: {
      IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);
      TRACE_(dmfile)(": RIFF chunk of type %s", debugstr_fourcc(Chunk.fccID));
      StreamSize = Chunk.dwSize - sizeof(FOURCC);
      switch (Chunk.fccID) {
      case  DMUS_FOURCC_TRACK_FORM: {
	TRACE_(dmfile)(": TRACK form\n");
        hr = parse_track_form(This, &Chunk, pStm);
	if (FAILED(hr)) return hr;	
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
    TRACE_(dmfile)(": ListCount[0] = %d < ListSize[0] = %d\n", ListCount[0], ListSize[0]);
  } while (ListCount[0] < ListSize[0]);

  return S_OK;
}

static HRESULT parse_segment_form(IDirectMusicSegment8Impl *This, DMUS_PRIVATE_CHUNK *pChunk,
        IStream *pStm)
{
  HRESULT hr = E_FAIL;
  DMUS_PRIVATE_CHUNK Chunk;
  DWORD StreamSize, StreamCount, ListSize[3], ListCount[3];
  LARGE_INTEGER liMove; /* used when skipping chunks */

  if (pChunk->fccID != DMUS_FOURCC_SEGMENT_FORM) {
    ERR_(dmfile)(": %s chunk should be a segment form\n", debugstr_fourcc (pChunk->fccID));
    return E_FAIL;
  }

  StreamSize = pChunk->dwSize - sizeof(FOURCC);
  StreamCount = 0;

  do {
    IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
    StreamCount += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
    TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);

    hr = IDirectMusicUtils_IPersistStream_ParseDescGeneric(&Chunk, pStm, &This->dmobj.desc);
    if (FAILED(hr)) return hr;
    
    if (hr == S_FALSE) {
      switch (Chunk.fccID) {
      case DMUS_FOURCC_SEGMENT_CHUNK: {
	DWORD checkSz = sizeof(FOURCC);
	TRACE_(dmfile)(": segment chunk\n");
	/** DX 7 */
	IStream_Read (pStm, &This->header.dwRepeats,    sizeof(This->header.dwRepeats), NULL);
	checkSz += sizeof(This->header.dwRepeats);
	IStream_Read (pStm, &This->header.mtLength,     sizeof(This->header.mtLength), NULL);
	checkSz += sizeof(This->header.mtLength);
	IStream_Read (pStm, &This->header.mtPlayStart,  sizeof(This->header.mtPlayStart), NULL);
	checkSz += sizeof(This->header.mtPlayStart);
	IStream_Read (pStm, &This->header.mtLoopStart,  sizeof(This->header.mtLoopStart), NULL);
	checkSz += sizeof(This->header.mtLoopStart);
	IStream_Read (pStm, &This->header.mtLoopEnd,    sizeof(This->header.mtLoopEnd), NULL);
	checkSz += sizeof(This->header.mtLoopEnd);
	IStream_Read (pStm, &This->header.dwResolution, sizeof(This->header.dwResolution), NULL);
	checkSz += sizeof(This->header.dwResolution);
	TRACE_(dmfile)("dwRepeats: %u\n", This->header.dwRepeats);
	TRACE_(dmfile)("mtLength: %u\n",  This->header.mtLength);
	TRACE_(dmfile)("mtPlayStart: %u\n",  This->header.mtPlayStart);
	TRACE_(dmfile)("mtLoopStart: %u\n",  This->header.mtLoopStart);
	TRACE_(dmfile)("mtLoopEnd: %u\n",  This->header.mtLoopEnd);
	TRACE_(dmfile)("dwResolution: %u\n", This->header.dwResolution);
	/** DX 8 */
	if (Chunk.dwSize > checkSz) {
	  IStream_Read (pStm, &This->header.rtLength,    sizeof(This->header.rtLength), NULL);
	  checkSz += sizeof(This->header.rtLength);
	  IStream_Read (pStm, &This->header.dwFlags,     sizeof(This->header.dwFlags), NULL);
	  checkSz += sizeof(This->header.dwFlags);
	}
	/** DX 9 */
	if (Chunk.dwSize > checkSz) {
	  IStream_Read (pStm, &This->header.rtLoopStart,  sizeof(This->header.rtLoopStart), NULL);
	  checkSz += sizeof(This->header.rtLoopStart);
	  IStream_Read (pStm, &This->header.rtLoopEnd,    sizeof(This->header.rtLoopEnd), NULL);
	  checkSz += sizeof(This->header.rtLoopEnd);
	  IStream_Read (pStm, &This->header.rtPlayStart,  sizeof(This->header.rtPlayStart), NULL);
	  checkSz += sizeof(This->header.rtPlayStart);
	}
	liMove.QuadPart = Chunk.dwSize - checkSz + sizeof(FOURCC);
	IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
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

            hr = IDirectMusicUtils_IPersistStream_ParseUNFOGeneric(&Chunk, pStm, &This->dmobj.desc);
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
	case DMUS_FOURCC_TRACK_LIST: {
	  TRACE_(dmfile)(": TRACK list\n");
          hr = parse_track_list(This, &Chunk, pStm);
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

static HRESULT load_wave(IStream *pClonedStream, IDirectMusicObject **ppWaveObject)
{
  HRESULT hr = E_FAIL;
  IPersistStream* pPersistStream = NULL;
  
  hr = CoCreateInstance (&CLSID_DirectSoundWave, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicObject, (LPVOID*) ppWaveObject);
  if (FAILED(hr)) {
    ERR(": could not create object\n");
    return hr;
  }
  /* acquire PersistStream interface */
  hr = IDirectMusicObject_QueryInterface (*ppWaveObject, &IID_IPersistStream, (LPVOID*) &pPersistStream);
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

  return S_OK;
}

static inline IDirectMusicSegment8Impl *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicSegment8Impl, dmobj.IPersistStream_iface);
}

static HRESULT WINAPI IPersistStreamImpl_Load(IPersistStream *iface, IStream *pStm)
{
  IDirectMusicSegment8Impl *This = impl_from_IPersistStream(iface);
  HRESULT hr;
  DMUS_PRIVATE_CHUNK Chunk;
  DWORD StreamSize;
  /*DWORD ListSize[3], ListCount[3];*/
  LARGE_INTEGER liMove; /* used when skipping chunks */

  TRACE("(%p, %p): Loading\n", This, pStm);
  hr = IStream_Read (pStm, &Chunk, sizeof(Chunk), NULL);
  if(hr != S_OK){
    WARN("IStream_Read failed: %08x\n", hr);
    return DMUS_E_UNSUPPORTED_STREAM;
  }
  TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
  switch (Chunk.fccID) {	
  case FOURCC_RIFF: {
    IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);				
    TRACE_(dmfile)(": RIFF chunk of type %s", debugstr_fourcc(Chunk.fccID));
    StreamSize = Chunk.dwSize - sizeof(FOURCC);
    switch (Chunk.fccID) {
    case DMUS_FOURCC_SEGMENT_FORM: {
      TRACE_(dmfile)(": segment form\n");
      hr = parse_segment_form(This, &Chunk, pStm);
      if (FAILED(hr)) return hr;
      break;
    }
    case mmioFOURCC('W','A','V','E'): {
      LPSTREAM pClonedStream = NULL;	
      IDirectMusicObject* pWave = NULL;

      FIXME_(dmfile)(": WAVE form (loading to be checked)\n");

      IStream_Clone (pStm, &pClonedStream);
	
      liMove.QuadPart = - (LONGLONG)(sizeof(FOURCC) * 2 + sizeof(DWORD));
      IStream_Seek (pClonedStream, liMove, STREAM_SEEK_CUR, NULL);

      hr = load_wave(pClonedStream, &pWave);
      if (FAILED(hr)) {
	ERR(": could not load track\n");
	return hr;
      }
      IStream_Release (pClonedStream);
      
      IDirectMusicTrack_Release(pWave); pWave = NULL; /* now we can release at as it inserted */

      liMove.QuadPart = StreamSize;
      IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
      break;      
    }
    default: {
      TRACE_(dmfile)(": unexpected chunk (loading failed)\n");
      liMove.QuadPart = StreamSize;
      IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
      return DMUS_E_UNSUPPORTED_STREAM;
    }
    }
    TRACE_(dmfile)(": reading finished\n");
    break;
  }
  default: {
    TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
    return DMUS_E_UNSUPPORTED_STREAM;
  }
  }
  
  return S_OK;
}

static const IPersistStreamVtbl persiststream_vtbl = {
    dmobj_IPersistStream_QueryInterface,
    dmobj_IPersistStream_AddRef,
    dmobj_IPersistStream_Release,
    dmobj_IPersistStream_GetClassID,
    unimpl_IPersistStream_IsDirty,
    IPersistStreamImpl_Load,
    unimpl_IPersistStream_Save,
    unimpl_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT WINAPI create_dmsegment(REFIID lpcGUID, void **ppobj)
{
  IDirectMusicSegment8Impl* obj;
  HRESULT hr;

  obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicSegment8Impl));
  if (NULL == obj) {
    *ppobj = NULL;
    return E_OUTOFMEMORY;
  }
  obj->IDirectMusicSegment8_iface.lpVtbl = &dmsegment8_vtbl;
  obj->ref = 1;
  dmobject_init(&obj->dmobj, &CLSID_DirectMusicSegment,
          (IUnknown *)&obj->IDirectMusicSegment8_iface);
  obj->dmobj.IDirectMusicObject_iface.lpVtbl = &dmobject_vtbl;
  obj->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;
  list_init (&obj->Tracks);

  DMIME_LockModule();
  hr = IDirectMusicSegment8_QueryInterface(&obj->IDirectMusicSegment8_iface, lpcGUID, ppobj);
  IDirectMusicSegment8_Release(&obj->IDirectMusicSegment8_iface);

  return hr;
}
