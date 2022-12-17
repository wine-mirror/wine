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

IDirectMusicSegment8Impl *create_segment(void);

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

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirectMusicSegment8Impl_Release(IDirectMusicSegment8 *iface)
{
    IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

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

  TRACE("(%p, %ld)\n", This, mtLength);
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

  TRACE("(%p, %ld)\n", This, dwRepeats);
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

  TRACE("(%p, %ld)\n", This, dwResolution);
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

  TRACE("(%p, %s, %#lx, %#lx, %p)\n", This, debugstr_dmguid(rguidType), dwGroupBits, dwIndex, ppTrack);

  if (NULL == ppTrack) {
    return E_POINTER;
  }

  LIST_FOR_EACH (pEntry, &This->Tracks) {
    pIt = LIST_ENTRY(pEntry, DMUS_PRIVATE_SEGMENT_TRACK, entry);
    TRACE(" - %p -> %#lx,%p\n", pIt, pIt->dwGroupBits, pIt->pTrack);
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
    TRACE(" - %p -> %#lx, %p\n", pIt, pIt->dwGroupBits, pIt->pTrack);
    if (NULL != pIt && pIt->pTrack == pTrack) {
      *pdwGroupBits = pIt->dwGroupBits;
      return S_OK;
    }
  }

  return DMUS_E_NOT_FOUND;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_InsertTrack(IDirectMusicSegment8 *iface,
        IDirectMusicTrack *pTrack, DWORD group)
{
  IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);
  DWORD i = 0;
  struct list* pEntry = NULL;
  LPDMUS_PRIVATE_SEGMENT_TRACK pIt = NULL;
  LPDMUS_PRIVATE_SEGMENT_TRACK pNewSegTrack = NULL;

  TRACE("(%p, %p, %#lx)\n", This, pTrack, group);

  if (!group)
    return E_INVALIDARG;

  LIST_FOR_EACH (pEntry, &This->Tracks) {
    i++;
    pIt = LIST_ENTRY(pEntry, DMUS_PRIVATE_SEGMENT_TRACK, entry);
    TRACE(" - #%lu: %p -> %#lx, %p\n", i, pIt, pIt->dwGroupBits, pIt->pTrack);
    if (NULL != pIt && pIt->pTrack == pTrack) {
      ERR("(%p, %p): track is already in list\n", This, pTrack);
      return E_FAIL;
    }
  }

  pNewSegTrack = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(DMUS_PRIVATE_SEGMENT_TRACK));
  if (NULL == pNewSegTrack)
    return  E_OUTOFMEMORY;

  pNewSegTrack->dwGroupBits = group;
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

  FIXME("(%p, %p, %p, %ld): semi-stub\n", This, ppSegState, pPerformance, dwFlags);
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

static HRESULT WINAPI IDirectMusicSegment8Impl_GetParam(IDirectMusicSegment8 *iface, REFGUID type,
        DWORD group, DWORD index, MUSIC_TIME time, MUSIC_TIME *next, void *param)
{
    IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);
    IDirectMusicTrack *track;
    unsigned int i, count;
    HRESULT hr = DMUS_E_TRACK_NOT_FOUND;

    TRACE("(%p, %s, %#lx, %lu, %ld, %p, %p)\n", This, debugstr_dmguid(type), group, index, time,
            next, param);

    if (!type)
        return E_POINTER;

    /* Index is relative to the search pattern: group bits and supported param type */
    for (i = 0, count = 0; i < DMUS_SEG_ANYTRACK && count <= index; i++) {
        if (FAILED(IDirectMusicSegment8Impl_GetTrack(iface, &GUID_NULL, group, i, &track)))
            break;
        if (FAILED(IDirectMusicTrack_IsParamSupported(track, type)))
            continue;
        if (index == count || index == DMUS_SEG_ANYTRACK)
            hr = IDirectMusicTrack_GetParam(track, type, time, next, param);
        IDirectMusicTrack_Release(track);

        if (SUCCEEDED(hr))
            return hr;
        count++;
    }

    TRACE("(%p): not found\n", This);

    return hr;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_SetParam(IDirectMusicSegment8 *iface,
        REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, void *pParam)
{
  IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);
  FIXME("(%p, %s, %#lx, %ld, %ld, %p): stub\n", This, debugstr_dmguid(rguidType), dwGroupBits, dwIndex, mtTime, pParam);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_Clone(IDirectMusicSegment8 *iface, MUSIC_TIME start, MUSIC_TIME end,
        IDirectMusicSegment **segment)
{
    IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);
    IDirectMusicSegment8Impl *clone;
    IDirectMusicTrack *track;
    DMUS_PRIVATE_SEGMENT_TRACK *track_item, *cloned_item;
    HRESULT hr;
    BOOL track_clone_fail = FALSE;

    TRACE("(%p, %ld, %ld, %p)\n", This, start, end, segment);

    if (!segment)
        return E_POINTER;

    if (!(clone = create_segment())) {
        *segment = NULL;
        return E_OUTOFMEMORY;
    }

    clone->header = This->header;
    clone->pGraph = This->pGraph;
    if (clone->pGraph)
        IDirectMusicGraph_AddRef(clone->pGraph);

    LIST_FOR_EACH_ENTRY(track_item, &This->Tracks, DMUS_PRIVATE_SEGMENT_TRACK, entry) {
        if (SUCCEEDED(hr = IDirectMusicTrack_Clone(track_item->pTrack, start, end, &track))) {
            if ((cloned_item = HeapAlloc(GetProcessHeap(), 0, sizeof(*cloned_item)))) {
                cloned_item->dwGroupBits = track_item->dwGroupBits;
                cloned_item->flags = track_item->flags;
                cloned_item->pTrack = track;
                list_add_tail(&clone->Tracks, &cloned_item->entry);
                continue;
            } else {
                IDirectMusicTrack_Release(track);
            }
        }
        WARN("Failed to clone track %p: %#lx\n", track_item->pTrack, hr);
        track_clone_fail = TRUE;
    }

    *segment = (IDirectMusicSegment *)&clone->IDirectMusicSegment8_iface;

    return track_clone_fail ? S_FALSE : S_OK;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_SetStartPoint(IDirectMusicSegment8 *iface,
        MUSIC_TIME mtStart)
{
  IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);

  TRACE("(%p, %ld)\n", This, mtStart);
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
        MUSIC_TIME start, MUSIC_TIME end)
{
    IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);

    TRACE("(%p, %ld, %ld)\n", This, start, end);

    if ((end || start) &&
            (start >= This->header.mtLength || end > This->header.mtLength || start > end))
        return DMUS_E_OUT_OF_RANGE;

    This->header.mtLoopStart = start;
    This->header.mtLoopEnd = end;

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
	FIXME("(%p, %ld, %p): stub\n", This, dwNumPChannels, paPChannels);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSegment8Impl_SetTrackConfig(IDirectMusicSegment8 *iface,
        REFGUID rguidTrackClassID, DWORD dwGroupBits, DWORD dwIndex, DWORD dwFlagsOn,
        DWORD dwFlagsOff)
{
        IDirectMusicSegment8Impl *This = impl_from_IDirectMusicSegment8(iface);
	FIXME("(%p, %s, %#lx, %ld, %ld, %ld): stub\n", This, debugstr_dmguid(rguidTrackClassID), dwGroupBits, dwIndex, dwFlagsOn, dwFlagsOff);
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
	FIXME("(%p, %ld, %p, %p, %p): stub\n", This, mtTime, pFromSegment, pToSegment, ppComposedSegment);
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

    dump_DMUS_OBJECTDESC(desc);
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
static HRESULT parse_track_form(IDirectMusicSegment8Impl *This, IStream *stream,
        const struct chunk_entry *riff)
{
    struct chunk_entry chunk = {.parent = riff};
    IDirectMusicTrack *track = NULL;
    IPersistStream *ps = NULL;
    IStream *clone;
    DMUS_IO_TRACK_HEADER thdr;
    DMUS_IO_TRACK_EXTRAS_HEADER txhdr = {0};
    HRESULT hr;
    DMUS_PRIVATE_SEGMENT_TRACK *item;

    TRACE("Parsing track form in %p: %s\n", stream, debugstr_chunk(riff));

    /* First chunk must be the track header */
    if (FAILED(hr = stream_get_chunk(stream, &chunk)))
        return hr;
    if (chunk.id != DMUS_FOURCC_TRACK_CHUNK)
        return DMUS_E_TRACK_HDR_NOT_FIRST_CK;
    if (FAILED(hr = stream_chunk_get_data(stream, &chunk, &thdr, sizeof(thdr))))
        return hr;
    TRACE("Found DMUS_IO_TRACK_HEADER\n");
    TRACE("\tclass: %s\n", debugstr_guid (&thdr.guidClassID));
    TRACE("\tdwGroup: %#lx\n", thdr.dwGroup);
    TRACE("\tckid: %s\n", debugstr_fourcc (thdr.ckid));
    TRACE("\tfccType: %s\n", debugstr_fourcc (thdr.fccType));

    if (!!thdr.ckid == !!thdr.fccType) {
        WARN("One and only one of the ckid (%s) and fccType (%s) need to be set\n",
                debugstr_fourcc(thdr.ckid), debugstr_fourcc(thdr.fccType));
        return DMUS_E_INVALID_TRACK_HDR;
    }

    /* Optional chunks */
    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK) {
        if ((thdr.ckid && chunk.id == thdr.ckid) ||
                (!thdr.ckid && (chunk.id == FOURCC_LIST || chunk.id == FOURCC_RIFF) &&
                 chunk.type == thdr.fccType))
            break;

        if (chunk.id == DMUS_FOURCC_TRACK_EXTRAS_CHUNK &&
                SUCCEEDED(stream_chunk_get_data(stream, &chunk, &txhdr, sizeof(txhdr)))) {
            FIXME("DMUS_IO_TRACK_EXTRAS_HEADER chunk not fully handled\n");
            TRACE("dwFlags: %#lx, dwPriority: %lu\n", txhdr.dwFlags, txhdr.dwPriority);
        }
    }
    if (hr != S_OK)
        return hr == S_FALSE ? DMUS_E_TRACK_NOT_FOUND : hr;

    /* Some DirectMusicTrack implementation expect the stream to start with their data chunk */
    if (FAILED(hr = IStream_Clone(stream, &clone)))
        return hr;
    stream_reset_chunk_start(clone, &chunk);

    /* Load the track */
    hr = CoCreateInstance(&thdr.guidClassID, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicTrack,
            (void **)&track);
    if (FAILED(hr))
        goto done;
    hr = IDirectMusicTrack_QueryInterface(track, &IID_IPersistStream, (void **)&ps);
    if (FAILED(hr))
        goto done;
    hr = IPersistStream_Load(ps, clone);
    if (FAILED(hr))
        goto done;

    hr = IDirectMusicSegment8_InsertTrack(&This->IDirectMusicSegment8_iface, track, thdr.dwGroup);
    if (FAILED(hr))
        goto done;

    item = LIST_ENTRY(list_tail(&This->Tracks), DMUS_PRIVATE_SEGMENT_TRACK, entry);
    item->flags = txhdr.dwFlags;

done:
    if (ps)
        IPersistStream_Release(ps);
    if (track)
        IDirectMusicTrack_Release(track);
    IStream_Release(clone);

    return hr;
}

static HRESULT parse_track_list(IDirectMusicSegment8Impl *This, IStream *stream,
        const struct chunk_entry *trkl)
{
    struct chunk_entry chunk = {.parent = trkl};
    HRESULT hr;

    TRACE("Parsing track list in %p: %s\n", stream, debugstr_chunk(trkl));

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
        if (chunk.id == FOURCC_RIFF && chunk.type == DMUS_FOURCC_TRACK_FORM)
            hr = parse_track_form(This, stream, &chunk);

    return SUCCEEDED(hr) ? S_OK : hr;
}

static inline void dump_segment_header(DMUS_IO_SEGMENT_HEADER *h, DWORD size)
{
    unsigned int dx = 9;

    if (size == offsetof(DMUS_IO_SEGMENT_HEADER, rtLength))
        dx = 7;
    else if (size == offsetof(DMUS_IO_SEGMENT_HEADER, rtLoopStart))
        dx = 8;
    TRACE("Found DirectX%d DMUS_IO_SEGMENT_HEADER\n", dx);
    TRACE("\tdwRepeats: %lu\n", h->dwRepeats);
    TRACE("\tmtLength: %lu\n", h->mtLength);
    TRACE("\tmtPlayStart: %lu\n", h->mtPlayStart);
    TRACE("\tmtLoopStart: %lu\n", h->mtLoopStart);
    TRACE("\tmtLoopEnd: %lu\n", h->mtLoopEnd);
    TRACE("\tdwResolution: %lu\n", h->dwResolution);
    if (dx >= 8) {
        TRACE("\trtLength: %s\n", wine_dbgstr_longlong(h->rtLength));
        TRACE("\tdwFlags: %lu\n", h->dwFlags);
        TRACE("\tdwReserved: %lu\n", h->dwReserved);
    }
    if (dx == 9) {
        TRACE("\trtLoopStart: %s\n", wine_dbgstr_longlong(h->rtLoopStart));
        TRACE("\trtLoopEnd: %s\n", wine_dbgstr_longlong(h->rtLoopEnd));
        if (size > offsetof(DMUS_IO_SEGMENT_HEADER, rtPlayStart))
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
                /* DX9 without rtPlayStart field */
                if (chunk.size == offsetof(DMUS_IO_SEGMENT_HEADER, rtPlayStart))
                    WARN("Missing rtPlayStart field in %s\n", debugstr_chunk(&chunk));
                /* DX7, DX8 and DX9 structure sizes */
                else if (chunk.size != offsetof(DMUS_IO_SEGMENT_HEADER, rtLength) &&
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
                    if (FAILED(hr = parse_track_list(This, stream, &chunk)))
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

    if (stream_get_chunk(stream, &riff) != S_OK ||
            (riff.id != FOURCC_RIFF && riff.id != mmioFOURCC('M','T','h','d')))
        return DMUS_E_UNSUPPORTED_STREAM;
    stream_reset_chunk_start(stream, &riff);

    if (riff.id == mmioFOURCC('M','T','h','d')) {
        FIXME("MIDI file loading not supported\n");
        return S_OK;
    }

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

IDirectMusicSegment8Impl *create_segment(void)
{
    IDirectMusicSegment8Impl *obj;

    if (!(obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*obj))))
        return NULL;

    obj->IDirectMusicSegment8_iface.lpVtbl = &dmsegment8_vtbl;
    obj->ref = 1;
    dmobject_init(&obj->dmobj, &CLSID_DirectMusicSegment, (IUnknown *)&obj->IDirectMusicSegment8_iface);
    obj->dmobj.IDirectMusicObject_iface.lpVtbl = &dmobject_vtbl;
    obj->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;
    list_init (&obj->Tracks);

    DMIME_LockModule();

    return obj;
}

/* for ClassFactory */
HRESULT create_dmsegment(REFIID guid, void **ret_iface)
{
    IDirectMusicSegment8Impl *obj;
    HRESULT hr;

    if (!(obj = create_segment())) {
        *ret_iface = NULL;
        return E_OUTOFMEMORY;
    }

    hr = IDirectMusicSegment8_QueryInterface(&obj->IDirectMusicSegment8_iface, guid, ret_iface);
    IDirectMusicSegment8_Release(&obj->IDirectMusicSegment8_iface);

    return hr;
}
