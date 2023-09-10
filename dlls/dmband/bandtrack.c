/*
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
#include "dmobject.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmband);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

struct band_entry
{
    struct list entry;
    DMUS_IO_BAND_ITEM_HEADER2 head;
    IDirectMusicBand *band;
};

static void band_entry_destroy(struct band_entry *entry)
{
    IDirectMusicTrack_Release(entry->band);
    free(entry);
}

struct band_track
{
    IDirectMusicTrack8 IDirectMusicTrack8_iface;
    struct dmobject dmobj; /* IPersistStream only */
    LONG ref;
    DMUS_IO_BAND_TRACK_HEADER header;
    struct list bands;
};

static inline struct band_track *impl_from_IDirectMusicTrack8(IDirectMusicTrack8 *iface)
{
    return CONTAINING_RECORD(iface, struct band_track, IDirectMusicTrack8_iface);
}

static HRESULT WINAPI band_track_QueryInterface(IDirectMusicTrack8 *iface, REFIID riid,
        void **ret_iface)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);

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

static ULONG WINAPI band_track_AddRef(IDirectMusicTrack8 *iface)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI band_track_Release(IDirectMusicTrack8 *iface)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref)
    {
        struct band_entry *entry, *next;

        LIST_FOR_EACH_ENTRY_SAFE(entry, next, &This->bands, struct band_entry, entry)
        {
            list_remove(&entry->entry);
            band_entry_destroy(entry);
        }

        free(This);
    }

    return ref;
}

static HRESULT WINAPI band_track_Init(IDirectMusicTrack8 *iface, IDirectMusicSegment *pSegment)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);
    FIXME("(%p, %p): stub\n", This, pSegment);
    return S_OK;
}

static HRESULT WINAPI band_track_InitPlay(IDirectMusicTrack8 *iface,
        IDirectMusicSegmentState *segment_state, IDirectMusicPerformance *performance,
        void **state_data, DWORD virtual_track8id, DWORD flags)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);

    FIXME("(%p, %p, %p, %p, %ld, %lx): stub\n", This, segment_state, performance, state_data, virtual_track8id, flags);

    return S_OK;
}

static HRESULT WINAPI band_track_EndPlay(IDirectMusicTrack8 *iface, void *pStateData)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);
    FIXME("(%p, %p): stub\n", This, pStateData);
    return S_OK;
}

static HRESULT WINAPI band_track_Play(IDirectMusicTrack8 *iface, void *state_data,
        MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD flags,
        IDirectMusicPerformance *performance, IDirectMusicSegmentState *segment_state,
        DWORD virtual_id)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);

    FIXME("(%p, %p, %ld, %ld, %ld, %lx, %p, %p, %ld): semi-stub\n", This, state_data, mtStart, mtEnd, mtOffset, flags, performance, segment_state, virtual_id);

    /* Sends following pMSG:
       - DMUS_PATCH_PMSG
       - DMUS_TRANSPOSE_PMSG
       - DMUS_CHANNEL_PRIORITY_PMSG
       - DMUS_MIDI_PMSG
    */

    return S_OK;
}

static HRESULT WINAPI band_track_GetParam(IDirectMusicTrack8 *iface, REFGUID type, MUSIC_TIME time,
        MUSIC_TIME *next, void *param)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %ld, %p, %p)\n", This, debugstr_dmguid(type), time, next, param);

    if (!type)
        return E_POINTER;
    if (!IsEqualGUID(type, &GUID_BandParam))
        return DMUS_E_GET_UNSUPPORTED;

    FIXME("GUID_BandParam not handled yet\n");

    return S_OK;
}

static HRESULT WINAPI band_track_SetParam(IDirectMusicTrack8 *iface, REFGUID type, MUSIC_TIME time,
        void *param)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %ld, %p)\n", This, debugstr_dmguid(type), time, param);

    if (!type)
        return E_POINTER;
    if (FAILED(IDirectMusicTrack8_IsParamSupported(iface, type)))
        return DMUS_E_TYPE_UNSUPPORTED;

    if (IsEqualGUID(type, &GUID_BandParam))
        FIXME("GUID_BandParam not handled yet\n");
    else if (IsEqualGUID(type, &GUID_Clear_All_Bands))
        FIXME("GUID_Clear_All_Bands not handled yet\n");
    else if (IsEqualGUID(type, &GUID_ConnectToDLSCollection))
        FIXME("GUID_ConnectToDLSCollection not handled yet\n");
    else if (IsEqualGUID(type, &GUID_Disable_Auto_Download))
        FIXME("GUID_Disable_Auto_Download not handled yet\n");
    else if (IsEqualGUID(type, &GUID_Download))
        FIXME("GUID_Download not handled yet\n");
    else if (IsEqualGUID(type, &GUID_DownloadToAudioPath))
        FIXME("GUID_DownloadToAudioPath not handled yet\n");
    else if (IsEqualGUID(type, &GUID_Enable_Auto_Download))
        FIXME("GUID_Enable_Auto_Download not handled yet\n");
    else if (IsEqualGUID(type, &GUID_IDirectMusicBand))
        FIXME("GUID_IDirectMusicBand not handled yet\n");
    else if (IsEqualGUID(type, &GUID_StandardMIDIFile))
        FIXME("GUID_StandardMIDIFile not handled yet\n");
    else if (IsEqualGUID(type, &GUID_UnloadFromAudioPath))
        FIXME("GUID_UnloadFromAudioPath not handled yet\n");

    return S_OK;
}

static HRESULT WINAPI band_track_IsParamSupported(IDirectMusicTrack8 *iface, REFGUID rguidType)
{
  struct band_track *This = impl_from_IDirectMusicTrack8(iface);

  TRACE("(%p, %s)\n", This, debugstr_dmguid(rguidType));

  if (!rguidType)
    return E_POINTER;

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

static HRESULT WINAPI band_track_AddNotificationType(IDirectMusicTrack8 *iface, REFGUID notiftype)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): method not implemented\n", This, debugstr_dmguid(notiftype));
    return E_NOTIMPL;
}

static HRESULT WINAPI band_track_RemoveNotificationType(IDirectMusicTrack8 *iface,
        REFGUID notiftype)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): method not implemented\n", This, debugstr_dmguid(notiftype));
    return E_NOTIMPL;
}

static HRESULT WINAPI band_track_Clone(IDirectMusicTrack8 *iface, MUSIC_TIME mtStart,
        MUSIC_TIME mtEnd, IDirectMusicTrack **ppTrack)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);
    FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);
    return S_OK;
}

static HRESULT WINAPI band_track_PlayEx(IDirectMusicTrack8 *iface, void *state_data,
        REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD flags,
        IDirectMusicPerformance *performance, IDirectMusicSegmentState *segment_state,
        DWORD virtual_id)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);

    FIXME("(%p, %p, 0x%s, 0x%s, 0x%s, %lx, %p, %p, %ld): stub\n", This, state_data, wine_dbgstr_longlong(rtStart),
        wine_dbgstr_longlong(rtEnd), wine_dbgstr_longlong(rtOffset), flags, performance, segment_state, virtual_id);

    return S_OK;
}

static HRESULT WINAPI band_track_GetParamEx(IDirectMusicTrack8 *iface,
        REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME *rtNext, void *param,
        void *state_data, DWORD flags)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);

    FIXME("(%p, %s, 0x%s, %p, %p, %p, %lx): stub\n", This, debugstr_dmguid(rguidType),
        wine_dbgstr_longlong(rtTime), rtNext, param, state_data, flags);

    return S_OK;
}

static HRESULT WINAPI band_track_SetParamEx(IDirectMusicTrack8 *iface, REFGUID rguidType,
        REFERENCE_TIME rtTime, void *param, void *state_data, DWORD flags)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);

    FIXME("(%p, %s, 0x%s, %p, %p, %lx): stub\n", This, debugstr_dmguid(rguidType),
        wine_dbgstr_longlong(rtTime), param, state_data, flags);

    return S_OK;
}

static HRESULT WINAPI band_track_Compose(IDirectMusicTrack8 *iface, IUnknown *context,
        DWORD trackgroup, IDirectMusicTrack **track)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %p, %ld, %p): method not implemented\n", This, context, trackgroup, track);
    return E_NOTIMPL;
}

static HRESULT WINAPI band_track_Join(IDirectMusicTrack8 *iface, IDirectMusicTrack *pNewTrack,
        MUSIC_TIME mtJoin, IUnknown *pContext, DWORD dwTrackGroup,
        IDirectMusicTrack **ppResultTrack)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);
    FIXME("(%p, %p, %ld, %p, %ld, %p): stub\n", This, pNewTrack, mtJoin, pContext, dwTrackGroup, ppResultTrack);
    return S_OK;
}

static const IDirectMusicTrack8Vtbl band_track_vtbl =
{
    band_track_QueryInterface,
    band_track_AddRef,
    band_track_Release,
    band_track_Init,
    band_track_InitPlay,
    band_track_EndPlay,
    band_track_Play,
    band_track_GetParam,
    band_track_SetParam,
    band_track_IsParamSupported,
    band_track_AddNotificationType,
    band_track_RemoveNotificationType,
    band_track_Clone,
    band_track_PlayEx,
    band_track_GetParamEx,
    band_track_SetParamEx,
    band_track_Compose,
    band_track_Join,
};

static HRESULT parse_lbnd_list(struct band_track *This, IStream *stream, struct chunk_entry *parent)
{
    struct chunk_entry chunk = {.parent = parent};
    DMUS_IO_BAND_ITEM_HEADER2 header2;
    struct band_entry *entry;
    IDirectMusicBand *band;
    HRESULT hr;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case DMUS_FOURCC_BANDITEM_CHUNK:
        {
            DMUS_IO_BAND_ITEM_HEADER header;

            if (SUCCEEDED(hr = stream_chunk_get_data(stream, &chunk, &header, sizeof(header))))
            {
                header2.lBandTimeLogical = header.lBandTime;
                header2.lBandTimePhysical = header.lBandTime;
            }

            break;
        }

        case DMUS_FOURCC_BANDITEM_CHUNK2:
            hr = stream_chunk_get_data(stream, &chunk, &header2, sizeof(header2));
            break;

        case MAKE_IDTYPE(FOURCC_RIFF, DMUS_FOURCC_BAND_FORM):
        {
            IPersistStream *persist;

            if (FAILED(hr = CoCreateInstance(&CLSID_DirectMusicBand, NULL, CLSCTX_INPROC_SERVER,
                    &IID_IDirectMusicBand, (void **)&band)))
                break;

            if (SUCCEEDED(hr = IDirectMusicBand_QueryInterface(band, &IID_IPersistStream, (void **)&persist)))
            {
                if (SUCCEEDED(hr = stream_reset_chunk_start(stream, &chunk)))
                    hr = IPersistStream_Load(persist, stream);
                IPersistStream_Release(persist);
            }

            break;
        }

        default:
            FIXME("Ignoring chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            break;
        }

        if (FAILED(hr)) break;
    }

    if (FAILED(hr)) return hr;

    if (!(entry = calloc(1, sizeof(*entry)))) return E_OUTOFMEMORY;
    entry->head = header2;
    entry->band = band;
    IDirectMusicBand_AddRef(band);
    list_add_tail(&This->bands, &entry->entry);

    return S_OK;
}

static HRESULT parse_lbdl_list(struct band_track *This, IStream *stream, struct chunk_entry *parent)
{
    struct chunk_entry chunk = {.parent = parent};
    HRESULT hr;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case MAKE_IDTYPE(FOURCC_LIST, DMUS_FOURCC_BAND_LIST):
            hr = parse_lbnd_list(This, stream, &chunk);
            break;

        default:
            FIXME("Ignoring chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            break;
        }

        if (FAILED(hr)) break;
    }

    return S_OK;
}

static HRESULT parse_bandtrack_form(struct band_track *This, DMUS_PRIVATE_CHUNK *pChunk,
        IStream *pStm)
{
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
    TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
    
    hr = IDirectMusicUtils_IPersistStream_ParseDescGeneric(&Chunk, pStm, &This->dmobj.desc);
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
            TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);

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
            TRACE_(dmfile)(": ListCount[0] = %ld < ListSize[0] = %ld\n", ListCount[0], ListSize[0]);
	  } while (ListCount[0] < ListSize[0]);
	  break;
	}
	case DMUS_FOURCC_BANDS_LIST: {
      static const LARGE_INTEGER zero = {0};
      struct chunk_entry chunk = {FOURCC_LIST, .size = Chunk.dwSize, .type = Chunk.fccID};
	  TRACE_(dmfile)(": TRACK list\n");
      IStream_Seek(pStm, zero, STREAM_SEEK_CUR, &chunk.offset);
      chunk.offset.QuadPart -= 12;
      hr = parse_lbdl_list(This, pStm, &chunk);
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
    TRACE_(dmfile)(": StreamCount[0] = %ld < StreamSize[0] = %ld\n", StreamCount, StreamSize);
  } while (StreamCount < StreamSize);  

  return S_OK;
}

static inline struct band_track *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, struct band_track, dmobj.IPersistStream_iface);
}

static HRESULT WINAPI band_track_persist_stream_Load(IPersistStream *iface, IStream *pStm)
{
  struct band_track *This = impl_from_IPersistStream(iface);
  DMUS_PRIVATE_CHUNK Chunk;
  LARGE_INTEGER liMove;
  HRESULT hr;

  TRACE("(%p, %p): Loading\n", This, pStm);

  IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
  TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
  switch (Chunk.fccID) {
  case FOURCC_RIFF: {
    IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);
    TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
    switch (Chunk.fccID) {
    case DMUS_FOURCC_BANDTRACK_FORM: {
      TRACE_(dmfile)(": Band track form\n");
      hr = parse_bandtrack_form(This, &Chunk, pStm);
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

static const IPersistStreamVtbl band_track_persist_stream_vtbl =
{
    dmobj_IPersistStream_QueryInterface,
    dmobj_IPersistStream_AddRef,
    dmobj_IPersistStream_Release,
    dmobj_IPersistStream_GetClassID,
    unimpl_IPersistStream_IsDirty,
    band_track_persist_stream_Load,
    unimpl_IPersistStream_Save,
    unimpl_IPersistStream_GetSizeMax,
};

/* for ClassFactory */
HRESULT create_dmbandtrack(REFIID lpcGUID, void **ppobj)
{
    struct band_track *track;
    HRESULT hr;

    *ppobj = NULL;
    if (!(track = calloc(1, sizeof(*track)))) return E_OUTOFMEMORY;
    track->IDirectMusicTrack8_iface.lpVtbl = &band_track_vtbl;
    track->ref = 1;
    dmobject_init(&track->dmobj, &CLSID_DirectMusicBandTrack, (IUnknown *)&track->IDirectMusicTrack8_iface);
    track->dmobj.IPersistStream_iface.lpVtbl = &band_track_persist_stream_vtbl;
    list_init(&track->bands);

    hr = IDirectMusicTrack8_QueryInterface(&track->IDirectMusicTrack8_iface, lpcGUID, ppobj);
    IDirectMusicTrack8_Release(&track->IDirectMusicTrack8_iface);

    return hr;
}
