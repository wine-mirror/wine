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
#include "dmobject.h"

#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmime);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

/*****************************************************************************
 * IDirectMusicTempoTrack implementation
 */
typedef struct IDirectMusicTempoTrack {
    IDirectMusicTrack8 IDirectMusicTrack8_iface;
    struct dmobject dmobj;  /* IPersistStream only */
    LONG ref;
    struct list Items;
} IDirectMusicTempoTrack;

/* IDirectMusicTempoTrack IDirectMusicTrack8 part: */
static inline IDirectMusicTempoTrack *impl_from_IDirectMusicTrack8(IDirectMusicTrack8 *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicTempoTrack, IDirectMusicTrack8_iface);
}

static HRESULT WINAPI tempo_track_QueryInterface(IDirectMusicTrack8 *iface, REFIID riid,
        void **ret_iface)
{
    IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ret_iface);

    *ret_iface = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDirectMusicTrack) ||
            IsEqualIID(riid, &IID_IDirectMusicTrack8))
        *ret_iface = iface;
    else if (IsEqualIID(riid, &IID_IPersistStream))
        *ret_iface = &This->dmobj.IPersistStream_iface;
    else {
        WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ret_iface);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ret_iface);
    return S_OK;
}

static ULONG WINAPI tempo_track_AddRef(IDirectMusicTrack8 *iface)
{
    IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI tempo_track_Release(IDirectMusicTrack8 *iface)
{
    IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if (!ref) {
        struct list *cursor, *cursor2;
        DMUS_PRIVATE_TEMPO_ITEM *item;

        LIST_FOR_EACH_SAFE(cursor, cursor2, &This->Items) {
            item = LIST_ENTRY(cursor, DMUS_PRIVATE_TEMPO_ITEM, entry);
            list_remove(cursor);

            heap_free(item);
        }

        heap_free(This);
        DMIME_UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI tempo_track_Init(IDirectMusicTrack8 *iface, IDirectMusicSegment *pSegment)
{
  IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);
  TRACE("(%p, %p): nothing to do here\n", This, pSegment);
  return S_OK;
}

static HRESULT WINAPI tempo_track_InitPlay(IDirectMusicTrack8 *iface,
        IDirectMusicSegmentState *pSegmentState, IDirectMusicPerformance *pPerformance,
        void **ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
  IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);

  LPDMUS_PRIVATE_TEMPO_PLAY_STATE pState = NULL;

  FIXME("(%p, %p, %p, %p, %d, %d): semi-stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);

  pState = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DMUS_PRIVATE_TEMPO_PLAY_STATE));
  if (NULL == pState)
    return E_OUTOFMEMORY;

  /** TODO real fill useful data */
  pState->dummy = 0;
  *ppStateData = pState;
  return S_OK;
}

static HRESULT WINAPI tempo_track_EndPlay(IDirectMusicTrack8 *iface, void *pStateData)
{
  IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);

  LPDMUS_PRIVATE_TEMPO_PLAY_STATE pState = pStateData;

  FIXME("(%p, %p): semi-stub\n", This, pStateData);

  if (NULL == pStateData) {
    return E_POINTER;
  }
  /** TODO real clean up */
  HeapFree(GetProcessHeap(), 0, pState);
  return S_OK;
}

static HRESULT WINAPI tempo_track_Play(IDirectMusicTrack8 *iface, void *pStateData,
        MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
  IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);
  FIXME("(%p, %p, %d, %d, %d, %d, %p, %p, %d): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
  /** should use IDirectMusicPerformance_SendPMsg here */
  return S_OK;
}

static HRESULT WINAPI tempo_track_GetParam(IDirectMusicTrack8 *iface, REFGUID type, MUSIC_TIME time,
        MUSIC_TIME *next, void *param)
{
    IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);
    DMUS_PRIVATE_TEMPO_ITEM *item = NULL;
    DMUS_TEMPO_PARAM *prm = param;

    TRACE("(%p, %s, %d, %p, %p)\n", This, debugstr_dmguid(type), time, next, param);

    if (!param)
        return E_POINTER;
    if (!IsEqualGUID(type, &GUID_TempoParam))
        return DMUS_E_GET_UNSUPPORTED;

    FIXME("Partial support for GUID_TempoParam\n");

    if (next)
        *next = 0;
    prm->mtTime = 0;
    prm->dblTempo = 0.123456;

    LIST_FOR_EACH_ENTRY(item, &This->Items, DMUS_PRIVATE_TEMPO_ITEM, entry) {
        if (item->item.lTime <= time) {
            MUSIC_TIME ofs = item->item.lTime - time;
            if (ofs > prm->mtTime) {
                prm->mtTime = ofs;
                prm->dblTempo = item->item.dblTempo;
            }
            if (next && item->item.lTime > time && item->item.lTime < *next)
                *next = item->item.lTime;
        }
    }

    if (0.123456 == prm->dblTempo)
        return DMUS_E_NOT_FOUND;

    return S_OK;
}

static HRESULT WINAPI tempo_track_SetParam(IDirectMusicTrack8 *iface, REFGUID type, MUSIC_TIME time,
        void *param)
{
    IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %d, %p)\n", This, debugstr_dmguid(type), time, param);

    if (IsEqualGUID(type, &GUID_DisableTempo)) {
        if (!param)
            return DMUS_E_TYPE_DISABLED;
        FIXME("GUID_DisableTempo not handled yet\n");
        return S_OK;
    }
    if (IsEqualGUID(type, &GUID_EnableTempo)) {
        if (!param)
            return DMUS_E_TYPE_DISABLED;
        FIXME("GUID_EnableTempo not handled yet\n");
        return S_OK;
    }
    if (IsEqualGUID(type, &GUID_TempoParam)) {
        if (!param)
            return E_POINTER;
        FIXME("GUID_TempoParam not handled yet\n");
        return S_OK;
    }

    return DMUS_E_SET_UNSUPPORTED;
}

static HRESULT WINAPI tempo_track_IsParamSupported(IDirectMusicTrack8 *iface, REFGUID rguidType)
{
  IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);

  TRACE("(%p, %s)\n", This, debugstr_dmguid(rguidType));
  if (IsEqualGUID (rguidType, &GUID_DisableTempo)
      || IsEqualGUID (rguidType, &GUID_EnableTempo)
      || IsEqualGUID (rguidType, &GUID_TempoParam)) {
    TRACE("param supported\n");
    return S_OK;
  }
  TRACE("param unsupported\n");
  return DMUS_E_TYPE_UNSUPPORTED;
}

static HRESULT WINAPI tempo_track_AddNotificationType(IDirectMusicTrack8 *iface, REFGUID notiftype)
{
    IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): method not implemented\n", This, debugstr_dmguid(notiftype));
    return E_NOTIMPL;
}

static HRESULT WINAPI tempo_track_RemoveNotificationType(IDirectMusicTrack8 *iface,
        REFGUID notiftype)
{
    IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): method not implemented\n", This, debugstr_dmguid(notiftype));
    return E_NOTIMPL;
}

static HRESULT WINAPI tempo_track_Clone(IDirectMusicTrack8 *iface, MUSIC_TIME mtStart,
        MUSIC_TIME mtEnd, IDirectMusicTrack **ppTrack)
{
  IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);
  FIXME("(%p, %d, %d, %p): stub\n", This, mtStart, mtEnd, ppTrack);
  return S_OK;
}

static HRESULT WINAPI tempo_track_PlayEx(IDirectMusicTrack8 *iface, void *pStateData,
        REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
  IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);
  FIXME("(%p, %p, 0x%s, 0x%s, 0x%s, %d, %p, %p, %d): stub\n", This, pStateData, wine_dbgstr_longlong(rtStart),
      wine_dbgstr_longlong(rtEnd), wine_dbgstr_longlong(rtOffset), dwFlags, pPerf, pSegSt, dwVirtualID);
  return S_OK;
}

static HRESULT WINAPI tempo_track_GetParamEx(IDirectMusicTrack8 *iface, REFGUID rguidType,
        REFERENCE_TIME rtTime, REFERENCE_TIME *prtNext, void *pParam, void *pStateData,
        DWORD dwFlags)
{
  IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);
  FIXME("(%p, %s, 0x%s, %p, %p, %p, %d): stub\n", This, debugstr_dmguid(rguidType),
      wine_dbgstr_longlong(rtTime), prtNext, pParam, pStateData, dwFlags);
  return S_OK;
}

static HRESULT WINAPI tempo_track_SetParamEx(IDirectMusicTrack8 *iface, REFGUID rguidType,
        REFERENCE_TIME rtTime, void *pParam, void *pStateData, DWORD dwFlags)
{
  IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);
  FIXME("(%p, %s, 0x%s, %p, %p, %d): stub\n", This, debugstr_dmguid(rguidType),
      wine_dbgstr_longlong(rtTime), pParam, pStateData, dwFlags);
  return S_OK;
}

static HRESULT WINAPI tempo_track_Compose(IDirectMusicTrack8 *iface, IUnknown *context,
        DWORD trackgroup, IDirectMusicTrack **track)
{
    IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %p, %d, %p): method not implemented\n", This, context, trackgroup, track);
    return E_NOTIMPL;
}

static HRESULT WINAPI tempo_track_Join(IDirectMusicTrack8 *iface, IDirectMusicTrack *pNewTrack,
        MUSIC_TIME mtJoin, IUnknown *pContext, DWORD dwTrackGroup,
        IDirectMusicTrack **ppResultTrack)
{
  IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);
  FIXME("(%p, %p, %d, %p, %d, %p): stub\n", This, pNewTrack, mtJoin, pContext, dwTrackGroup, ppResultTrack);
  return S_OK;
}

static const IDirectMusicTrack8Vtbl dmtrack8_vtbl = {
    tempo_track_QueryInterface,
    tempo_track_AddRef,
    tempo_track_Release,
    tempo_track_Init,
    tempo_track_InitPlay,
    tempo_track_EndPlay,
    tempo_track_Play,
    tempo_track_GetParam,
    tempo_track_SetParam,
    tempo_track_IsParamSupported,
    tempo_track_AddNotificationType,
    tempo_track_RemoveNotificationType,
    tempo_track_Clone,
    tempo_track_PlayEx,
    tempo_track_GetParamEx,
    tempo_track_SetParamEx,
    tempo_track_Compose,
    tempo_track_Join
};

static inline IDirectMusicTempoTrack *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicTempoTrack, dmobj.IPersistStream_iface);
}

static HRESULT WINAPI tempo_IPersistStream_Load(IPersistStream *iface, IStream *pStm)
{
  IDirectMusicTempoTrack *This = impl_from_IPersistStream(iface);
  DMUS_PRIVATE_CHUNK Chunk;
  DWORD StreamSize, StreamCount;
  LARGE_INTEGER liMove;
  DMUS_IO_TEMPO_ITEM item;
  LPDMUS_PRIVATE_TEMPO_ITEM pNewItem = NULL;
  DWORD nItem = 0;
  FIXME("(%p, %p): Loading not fully implemented yet\n", This, pStm);

  IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
  TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
  switch (Chunk.fccID) {	
  case DMUS_FOURCC_TEMPO_TRACK: {
    TRACE_(dmfile)(": Tempo track\n");
    IStream_Read (pStm, &StreamSize, sizeof(DWORD), NULL);
    StreamSize -= sizeof(DWORD);
    StreamCount = 0;
    TRACE_(dmfile)(" - sizeof(DMUS_IO_TEMPO_ITEM): %u (chunkSize = %u)\n", StreamSize, Chunk.dwSize);
    do {
      IStream_Read (pStm, &item, sizeof(item), NULL);
      ++nItem;
      TRACE_(dmfile)("DMUS_IO_TEMPO_ITEM #%d\n", nItem);
      TRACE_(dmfile)(" - lTime = %u\n", item.lTime);
      TRACE_(dmfile)(" - dblTempo = %g\n", item.dblTempo);
      pNewItem = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(DMUS_PRIVATE_TEMPO_ITEM));
      if (NULL == pNewItem)
        return  E_OUTOFMEMORY;

      pNewItem->item = item;
      list_add_tail (&This->Items, &pNewItem->entry);
      pNewItem = NULL;
      StreamCount += sizeof(item);
      TRACE_(dmfile)(": StreamCount[0] = %d < StreamSize[0] = %d\n", StreamCount, StreamSize);
    } while (StreamCount < StreamSize); 
    break;
  }
  default: {
    TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
    liMove.QuadPart = Chunk.dwSize;
    IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
    return E_FAIL;
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
    tempo_IPersistStream_Load,
    unimpl_IPersistStream_Save,
    unimpl_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT WINAPI create_dmtempotrack(REFIID lpcGUID, void **ppobj)
{
    IDirectMusicTempoTrack *track;
    HRESULT hr;

    track = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*track));
    if (!track) {
        *ppobj = NULL;
        return E_OUTOFMEMORY;
    }
    track->IDirectMusicTrack8_iface.lpVtbl = &dmtrack8_vtbl;
    track->ref = 1;
    dmobject_init(&track->dmobj, &CLSID_DirectMusicTempoTrack,
                  (IUnknown *)&track->IDirectMusicTrack8_iface);
    track->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;
    list_init(&track->Items);

    DMIME_LockModule();
    hr = IDirectMusicTrack8_QueryInterface(&track->IDirectMusicTrack8_iface, lpcGUID, ppobj);
    IDirectMusicTrack8_Release(&track->IDirectMusicTrack8_iface);

    return hr;
}
