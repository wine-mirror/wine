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

    TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ret_iface);

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
static HRESULT WINAPI seg_IDirectMusicObject_ParseDescriptor(IDirectMusicObject *iface,
        IStream *stream, DMUS_OBJECTDESC *desc)
{
    struct chunk_entry riff = {0};
    DWORD supported = DMUS_OBJ_OBJECT | DMUS_OBJ_VERSION;
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", iface, stream, desc);

    if (!stream || !desc)
        return E_POINTER;

    if ((hr = stream_get_chunk(stream, &riff)) != S_OK)
        return hr;
    if (riff.id != FOURCC_RIFF || !(riff.type == DMUS_FOURCC_SEGMENT_FORM ||
                riff.type == mmioFOURCC('W','A','V','E'))) {
        TRACE("loading failed: unexpected %s\n", debugstr_chunk(&riff));
        stream_skip_chunk(stream, &riff);
        return E_FAIL;
    }

    if (riff.type == DMUS_FOURCC_SEGMENT_FORM)
        supported |= DMUS_OBJ_NAME | DMUS_OBJ_CATEGORY;
    else
        supported |= DMUS_OBJ_NAME_INFO;
    hr = dmobj_parsedescriptor(stream, &riff, desc, supported);
    if (FAILED(hr))
        return hr;

    desc->guidClass = CLSID_DirectMusicSegment;
    desc->dwValidData |= DMUS_OBJ_CLASS;

    TRACE("returning descriptor:\n%s\n", debugstr_DMUS_OBJECTDESC (desc));
    return S_OK;
}

static const IDirectMusicObjectVtbl dmobject_vtbl = {
    dmobj_IDirectMusicObject_QueryInterface,
    dmobj_IDirectMusicObject_AddRef,
    dmobj_IDirectMusicObject_Release,
    dmobj_IDirectMusicObject_GetDescriptor,
    dmobj_IDirectMusicObject_SetDescriptor,
    seg_IDirectMusicObject_ParseDescriptor
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

static HRESULT parse_track_list(IDirectMusicSegment8Impl *This, DWORD StreamSize, IStream *pStm)
{
  HRESULT hr = E_FAIL;
  DMUS_PRIVATE_CHUNK Chunk;
  DWORD ListSize[3], ListCount[3];
  LARGE_INTEGER liMove; /* used when skipping chunks */

  ListSize[0] = StreamSize - sizeof(FOURCC);
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

static inline void dump_segment_header(DMUS_IO_SEGMENT_HEADER *h, DWORD size)
{
    unsigned int dx = 9;

    if (size == offsetof(DMUS_IO_SEGMENT_HEADER, rtLength))
        dx = 7;
    else if (size == offsetof(DMUS_IO_SEGMENT_HEADER, rtLoopStart))
        dx = 8;
    TRACE("Found DirectX%d DMUS_IO_SEGMENT_HEADER\n", dx);
    TRACE("\tdwRepeats: %u\n", h->dwRepeats);
    TRACE("\tmtLength: %u\n", h->mtLength);
    TRACE("\tmtPlayStart: %u\n", h->mtPlayStart);
    TRACE("\tmtLoopStart: %u\n", h->mtLoopStart);
    TRACE("\tmtLoopEnd: %u\n", h->mtLoopEnd);
    TRACE("\tdwResolution: %u\n", h->dwResolution);
    if (dx >= 8) {
        TRACE("\trtLength: %s\n", wine_dbgstr_longlong(h->rtLength));
        TRACE("\tdwFlags: %u\n", h->dwFlags);
        TRACE("\tdwReserved: %u\n", h->dwReserved);
    }
    if (dx == 9) {
        TRACE("\trtLoopStart: %s\n", wine_dbgstr_longlong(h->rtLoopStart));
        TRACE("\trtLoopEnd: %s\n", wine_dbgstr_longlong(h->rtLoopEnd));
        TRACE("\trtPlayStart: %s\n", wine_dbgstr_longlong(h->rtPlayStart));
    }
}

static HRESULT parse_segment_form(IDirectMusicSegment8Impl *This, IStream *stream,
        const struct chunk_entry *riff)
{
    struct chunk_entry chunk = {.parent = riff};
    HRESULT hr;

    TRACE("Parsing segment form in %p: %s\n", stream, debugstr_chunk(riff));

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK) {
        switch (chunk.id) {
            case DMUS_FOURCC_SEGMENT_CHUNK:
                /* DX7, DX8 and DX9 structure sizes */
                if (chunk.size != offsetof(DMUS_IO_SEGMENT_HEADER, rtLength) &&
                        chunk.size != offsetof(DMUS_IO_SEGMENT_HEADER, rtLoopStart) &&
                        chunk.size != sizeof(DMUS_IO_SEGMENT_HEADER)) {
                    WARN("Invalid size of %s\n", debugstr_chunk(&chunk));
                    break;
                }
                if (FAILED(hr = stream_chunk_get_data(stream, &chunk, &This->header, chunk.size))) {
                    WARN("Failed to read data of %s\n", debugstr_chunk(&chunk));
                    return hr;
                }
                dump_segment_header(&This->header, chunk.size);
                break;
            case FOURCC_LIST:
                if (chunk.type == DMUS_FOURCC_TRACK_LIST)
                    if (FAILED(hr = parse_track_list(This, chunk.size, stream)))
                        return hr;
                break;
            case FOURCC_RIFF:
                FIXME("Loading of embedded RIFF form %s\n", debugstr_fourcc(chunk.type));
                break;
        }
    }

    return SUCCEEDED(hr) ? S_OK : hr;
}

static inline IDirectMusicSegment8Impl *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicSegment8Impl, dmobj.IPersistStream_iface);
}

static HRESULT WINAPI seg_IPersistStream_Load(IPersistStream *iface, IStream *stream)
{
    IDirectMusicSegment8Impl *This = impl_from_IPersistStream(iface);
    struct chunk_entry riff = {0};
    HRESULT hr;

    TRACE("(%p, %p): Loading\n", This, stream);

    if (!stream)
        return E_POINTER;

    if (stream_get_chunk(stream, &riff) != S_OK || riff.id != FOURCC_RIFF)
        return DMUS_E_UNSUPPORTED_STREAM;

    stream_reset_chunk_start(stream, &riff);
    hr = IDirectMusicObject_ParseDescriptor(&This->dmobj.IDirectMusicObject_iface, stream,
            &This->dmobj.desc);
    if (FAILED(hr))
        return hr;
    stream_reset_chunk_data(stream, &riff);

    if (riff.type == DMUS_FOURCC_SEGMENT_FORM)
        hr = parse_segment_form(This, stream, &riff);
    else {
        FIXME("WAVE form loading not implemented\n");
        hr = S_OK;
    }

    return hr;
}

static const IPersistStreamVtbl persiststream_vtbl = {
    dmobj_IPersistStream_QueryInterface,
    dmobj_IPersistStream_AddRef,
    dmobj_IPersistStream_Release,
    dmobj_IPersistStream_GetClassID,
    unimpl_IPersistStream_IsDirty,
    seg_IPersistStream_Load,
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
