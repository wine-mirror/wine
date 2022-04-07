/* IDirectMusicStyle8 Implementation
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
#include "dmobject.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmstyle);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

struct style_band {
    struct list entry;
    IDirectMusicBand *pBand;
};

struct style_partref_item {
    struct list entry;
    DMUS_OBJECTDESC desc;
    DMUS_IO_PARTREF part_ref;
};

struct style_motif {
    struct list entry;
    DWORD dwRhythm;
    DMUS_IO_PATTERN pattern;
    DMUS_OBJECTDESC desc;
    /** optional for motifs */
    DMUS_IO_MOTIFSETTINGS settings;
    IDirectMusicBand *pBand;
    struct list Items;
};

/*****************************************************************************
 * IDirectMusicStyleImpl implementation
 */
typedef struct IDirectMusicStyle8Impl {
    IDirectMusicStyle8 IDirectMusicStyle8_iface;
    struct dmobject dmobj;
    LONG ref;
    DMUS_IO_STYLE style;
    struct list motifs;
    struct list bands;
} IDirectMusicStyle8Impl;

static inline IDirectMusicStyle8Impl *impl_from_IDirectMusicStyle8(IDirectMusicStyle8 *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicStyle8Impl, IDirectMusicStyle8_iface);
}

/* DirectMusicStyle8Impl IDirectMusicStyle8 part: */
static HRESULT WINAPI IDirectMusicStyle8Impl_QueryInterface(IDirectMusicStyle8 *iface, REFIID riid,
        void **ret_iface)
{
    IDirectMusicStyle8Impl *This = impl_from_IDirectMusicStyle8(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ret_iface);

    *ret_iface = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDirectMusicStyle) ||
            IsEqualIID(riid, &IID_IDirectMusicStyle8))
        *ret_iface = iface;
    else if (IsEqualIID(riid, &IID_IDirectMusicObject))
        *ret_iface = &This->dmobj.IDirectMusicObject_iface;
    else if (IsEqualIID(riid, &IID_IPersistStream))
        *ret_iface = &This->dmobj.IPersistStream_iface;
    else {
        WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ret_iface);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ret_iface);
    return S_OK;
}

static ULONG WINAPI IDirectMusicStyle8Impl_AddRef(IDirectMusicStyle8 *iface)
{
    IDirectMusicStyle8Impl *This = impl_from_IDirectMusicStyle8(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirectMusicStyle8Impl_Release(IDirectMusicStyle8 *iface)
{
    IDirectMusicStyle8Impl *This = impl_from_IDirectMusicStyle8(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref) {
        struct style_band *band, *band2;
        struct style_motif *motif, *motif2;
        struct style_partref_item *item, *item2;

        LIST_FOR_EACH_ENTRY_SAFE(band, band2, &This->bands, struct style_band, entry) {
            list_remove(&band->entry);
            if (band->pBand)
                IDirectMusicBand_Release(band->pBand);
            heap_free(band);
        }
        LIST_FOR_EACH_ENTRY_SAFE(motif, motif2, &This->motifs, struct style_motif, entry) {
            list_remove(&motif->entry);
            LIST_FOR_EACH_ENTRY_SAFE(item, item2, &motif->Items, struct style_partref_item, entry) {
                list_remove(&item->entry);
                heap_free(item);
            }
            heap_free(motif);
        }
        heap_free(This);
        DMSTYLE_UnlockModule();
    }

    return ref;
}

/* IDirectMusicStyle8Impl IDirectMusicStyle(8) part: */
static HRESULT WINAPI IDirectMusicStyle8Impl_GetBand(IDirectMusicStyle8 *iface, WCHAR *name,
        IDirectMusicBand **band)
{
    IDirectMusicStyle8Impl *This = impl_from_IDirectMusicStyle8(iface);
    struct style_band *sband;
    HRESULT hr;

    TRACE("(%p, %s, %p)\n", This, debugstr_w(name), band);

    if (!name)
        return E_POINTER;

    LIST_FOR_EACH_ENTRY(sband, &This->bands, struct style_band, entry) {
        IDirectMusicObject *obj;

        hr = IDirectMusicBand_QueryInterface(sband->pBand, &IID_IDirectMusicObject, (void**)&obj);
        if (SUCCEEDED(hr)) {
            DMUS_OBJECTDESC desc;

            if (IDirectMusicObject_GetDescriptor(obj, &desc) == S_OK) {
                if (desc.dwValidData & DMUS_OBJ_NAME && !lstrcmpW(name, desc.wszName)) {
                    IDirectMusicObject_Release(obj);
                    IDirectMusicBand_AddRef(sband->pBand);
                    *band = sband->pBand;
                    return S_OK;
                }
            }

            IDirectMusicObject_Release(obj);
        }
    }

    return S_FALSE;
}

static HRESULT WINAPI IDirectMusicStyle8Impl_EnumBand(IDirectMusicStyle8 *iface, DWORD dwIndex,
        WCHAR *pwszName)
{
        IDirectMusicStyle8Impl *This = impl_from_IDirectMusicStyle8(iface);
	FIXME("(%p, %ld, %p): stub\n", This, dwIndex, pwszName);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicStyle8Impl_GetDefaultBand(IDirectMusicStyle8 *iface,
        IDirectMusicBand **band)
{
    IDirectMusicStyle8Impl *This = impl_from_IDirectMusicStyle8(iface);
    FIXME("(%p, %p): stub\n", This, band);

    if (!band)
        return E_POINTER;

    *band = NULL;

    return S_FALSE;
}

static HRESULT WINAPI IDirectMusicStyle8Impl_EnumMotif(IDirectMusicStyle8 *iface, DWORD index,
        WCHAR *name)
{
    IDirectMusicStyle8Impl *This = impl_from_IDirectMusicStyle8(iface);
    const struct style_motif *motif = NULL;
    const struct list *cursor;
    unsigned int i = 0;

    TRACE("(%p, %lu, %p)\n", This, index, name);

    if (!name)
        return E_POINTER;

    /* index is zero based */
    LIST_FOR_EACH(cursor, &This->motifs) {
        if (i == index) {
            motif = LIST_ENTRY(cursor, struct style_motif, entry);
            break;
        }
        i++;
    }
    if (!motif)
        return S_FALSE;

    if (motif->desc.dwValidData & DMUS_OBJ_NAME)
        lstrcpynW(name, motif->desc.wszName, DMUS_MAX_NAME);
    else
        name[0] = 0;

    TRACE("returning name: %s\n", debugstr_w(name));
    return S_OK;
}

static HRESULT WINAPI IDirectMusicStyle8Impl_GetMotif(IDirectMusicStyle8 *iface, WCHAR *pwszName,
        IDirectMusicSegment **ppSegment)
{
        IDirectMusicStyle8Impl *This = impl_from_IDirectMusicStyle8(iface);
        FIXME("(%p, %s, %p): stub\n", This, debugstr_w(pwszName), ppSegment);
        return S_FALSE;
}

static HRESULT WINAPI IDirectMusicStyle8Impl_GetDefaultChordMap(IDirectMusicStyle8 *iface,
        IDirectMusicChordMap **ppChordMap)
{
        IDirectMusicStyle8Impl *This = impl_from_IDirectMusicStyle8(iface);
	FIXME("(%p, %p): stub\n", This, ppChordMap);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicStyle8Impl_EnumChordMap(IDirectMusicStyle8 *iface, DWORD dwIndex,
        WCHAR *pwszName)
{
        IDirectMusicStyle8Impl *This = impl_from_IDirectMusicStyle8(iface);
	FIXME("(%p, %ld, %p): stub\n", This, dwIndex, pwszName);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicStyle8Impl_GetChordMap(IDirectMusicStyle8 *iface, WCHAR *pwszName,
        IDirectMusicChordMap **ppChordMap)
{
        IDirectMusicStyle8Impl *This = impl_from_IDirectMusicStyle8(iface);
	FIXME("(%p, %p, %p): stub\n", This, pwszName, ppChordMap);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicStyle8Impl_GetTimeSignature(IDirectMusicStyle8 *iface,
        DMUS_TIMESIGNATURE *pTimeSig)
{
        IDirectMusicStyle8Impl *This = impl_from_IDirectMusicStyle8(iface);
	FIXME("(%p, %p): stub\n", This, pTimeSig);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicStyle8Impl_GetEmbellishmentLength(IDirectMusicStyle8 *iface,
        DWORD dwType, DWORD dwLevel, DWORD *pdwMin, DWORD *pdwMax)
{
        IDirectMusicStyle8Impl *This = impl_from_IDirectMusicStyle8(iface);
	FIXME("(%p, %ld, %ld, %p, %p): stub\n", This, dwType, dwLevel, pdwMin, pdwMax);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicStyle8Impl_GetTempo(IDirectMusicStyle8 *iface, double *pTempo)
{
        IDirectMusicStyle8Impl *This = impl_from_IDirectMusicStyle8(iface);
	FIXME("(%p, %p): stub\n", This, pTempo);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicStyle8Impl_EnumPattern(IDirectMusicStyle8 *iface, DWORD dwIndex,
        DWORD dwPatternType, WCHAR *pwszName)
{
        IDirectMusicStyle8Impl *This = impl_from_IDirectMusicStyle8(iface);
	FIXME("(%p, %ld, %ld, %p): stub\n", This, dwIndex, dwPatternType, pwszName);
	return S_OK;
}

static const IDirectMusicStyle8Vtbl dmstyle8_vtbl = {
    IDirectMusicStyle8Impl_QueryInterface,
    IDirectMusicStyle8Impl_AddRef,
    IDirectMusicStyle8Impl_Release,
    IDirectMusicStyle8Impl_GetBand,
    IDirectMusicStyle8Impl_EnumBand,
    IDirectMusicStyle8Impl_GetDefaultBand,
    IDirectMusicStyle8Impl_EnumMotif,
    IDirectMusicStyle8Impl_GetMotif,
    IDirectMusicStyle8Impl_GetDefaultChordMap,
    IDirectMusicStyle8Impl_EnumChordMap,
    IDirectMusicStyle8Impl_GetChordMap,
    IDirectMusicStyle8Impl_GetTimeSignature,
    IDirectMusicStyle8Impl_GetEmbellishmentLength,
    IDirectMusicStyle8Impl_GetTempo,
    IDirectMusicStyle8Impl_EnumPattern
};

/* IDirectMusicStyle8Impl IDirectMusicObject part: */
static HRESULT WINAPI style_IDirectMusicObject_ParseDescriptor(IDirectMusicObject *iface,
        IStream *stream, DMUS_OBJECTDESC *desc)
{
    struct chunk_entry riff = {0};
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", iface, stream, desc);

    if (!stream || !desc)
        return E_POINTER;

    if ((hr = stream_get_chunk(stream, &riff)) != S_OK)
        return hr;
    if (riff.id != FOURCC_RIFF || riff.type != DMUS_FOURCC_STYLE_FORM) {
        TRACE("loading failed: unexpected %s\n", debugstr_chunk(&riff));
        stream_skip_chunk(stream, &riff);
        return DMUS_E_CHUNKNOTFOUND;
    }

    hr = dmobj_parsedescriptor(stream, &riff, desc,
            DMUS_OBJ_OBJECT|DMUS_OBJ_NAME|DMUS_OBJ_NAME_INAM|DMUS_OBJ_VERSION);
    if (FAILED(hr))
        return hr;

    desc->guidClass = CLSID_DirectMusicStyle;
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
    style_IDirectMusicObject_ParseDescriptor
};

/* IDirectMusicStyle8Impl IPersistStream part: */
static inline IDirectMusicStyle8Impl *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicStyle8Impl, dmobj.IPersistStream_iface);
}

static HRESULT load_band(IStream *pClonedStream, IDirectMusicBand **ppBand)
{
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

  return S_OK;
}

static HRESULT parse_part_ref_list(DMUS_PRIVATE_CHUNK *pChunk, IStream *pStm,
        struct style_motif *pNewMotif)
{
  HRESULT hr = E_FAIL;
  DMUS_PRIVATE_CHUNK Chunk;
  DWORD ListSize[3], ListCount[3];
  LARGE_INTEGER liMove; /* used when skipping chunks */
  struct style_partref_item *pNewItem = NULL;


  if (pChunk->fccID != DMUS_FOURCC_PARTREF_LIST) {
    ERR_(dmfile)(": %s chunk should be a PARTREF list\n", debugstr_fourcc (pChunk->fccID));
    return E_FAIL;
  }  

  ListSize[0] = pChunk->dwSize - sizeof(FOURCC);
  ListCount[0] = 0;

  do {
    IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
    ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
    TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
    switch (Chunk.fccID) {
    case DMUS_FOURCC_PARTREF_CHUNK: {
      TRACE_(dmfile)(": PartRef chunk\n");
      pNewItem = heap_alloc_zero(sizeof(*pNewItem));
      if (!pNewItem) {
	ERR(": no more memory\n");
	return E_OUTOFMEMORY;
      }
      hr = IStream_Read (pStm, &pNewItem->part_ref, sizeof(DMUS_IO_PARTREF), NULL);
      /*TRACE_(dmfile)(" - sizeof %lu\n",  sizeof(DMUS_IO_PARTREF));*/
      list_add_tail (&pNewMotif->Items, &pNewItem->entry);      
      pNewItem->desc.dwSize = sizeof(pNewItem->desc);
      break;
    }    
    case FOURCC_LIST: {
      IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);
      TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(Chunk.fccID));
      ListSize[1] = Chunk.dwSize - sizeof(FOURCC);
      ListCount[1] = 0;
      switch (Chunk.fccID) {  
      case DMUS_FOURCC_UNFO_LIST: { 
	TRACE_(dmfile)(": UNFO list\n");
	do {
	  IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
	  ListCount[1] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
          TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
	  
          if (!pNewItem) {
	    ERR(": pNewItem not yet allocated, chunk order bad?\n");
	    return E_OUTOFMEMORY;
          }
	  hr = IDirectMusicUtils_IPersistStream_ParseUNFOGeneric(&Chunk, pStm, &pNewItem->desc);
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
          TRACE_(dmfile)(": ListCount[1] = %ld < ListSize[1] = %ld\n", ListCount[1], ListSize[1]);
	} while (ListCount[1] < ListSize[1]);
	break;
      }
      default: {
	TRACE_(dmfile)(": unknown chunk (skipping)\n");
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
    TRACE_(dmfile)(": ListCount[0] = %ld < ListSize[0] = %ld\n", ListCount[0], ListSize[0]);
  } while (ListCount[0] < ListSize[0]);

  return S_OK;
}

static HRESULT parse_part_list(DMUS_PRIVATE_CHUNK *pChunk, IStream *pStm)
{
  HRESULT hr = E_FAIL;
  DMUS_PRIVATE_CHUNK Chunk;
  DWORD ListSize[3], ListCount[3];
  LARGE_INTEGER liMove; /* used when skipping chunks */

  DMUS_OBJECTDESC desc;
  DWORD dwSize = 0;
  DWORD cnt = 0;

  if (pChunk->fccID != DMUS_FOURCC_PART_LIST) {
    ERR_(dmfile)(": %s chunk should be a PART list\n", debugstr_fourcc (pChunk->fccID));
    return E_FAIL;
  }  

  ListSize[0] = pChunk->dwSize - sizeof(FOURCC);
  ListCount[0] = 0;

  do {
    IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
    ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
    TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
    switch (Chunk.fccID) {
    case DMUS_FOURCC_PART_CHUNK: {
      TRACE_(dmfile)(": Part chunk (skipping for now)\n" );
      liMove.QuadPart = Chunk.dwSize;
      IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
      break;
    }
    case DMUS_FOURCC_NOTE_CHUNK: { 
      TRACE_(dmfile)(": Note chunk (skipping for now)\n");
      IStream_Read (pStm, &dwSize, sizeof(DWORD), NULL);
      cnt = (Chunk.dwSize - sizeof(DWORD));
      TRACE_(dmfile)(" - dwSize: %lu\n", dwSize);
      TRACE_(dmfile)(" - cnt: %lu (%Iu / %lu)\n", cnt / dwSize, Chunk.dwSize - sizeof(DWORD), dwSize);
      if (cnt % dwSize != 0) {
	ERR("Invalid Array Size\n");
	return E_FAIL;
      }
      cnt /= dwSize;
      /** skip for now */
      liMove.QuadPart = cnt * dwSize;
      IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
      break;
    }
    case DMUS_FOURCC_CURVE_CHUNK: { 
      TRACE_(dmfile)(": Curve chunk (skipping for now)\n");
      IStream_Read (pStm, &dwSize, sizeof(DWORD), NULL);
      cnt = (Chunk.dwSize - sizeof(DWORD));
      TRACE_(dmfile)(" - dwSize: %lu\n", dwSize);
      TRACE_(dmfile)(" - cnt: %lu (%Iu / %lu)\n", cnt / dwSize, Chunk.dwSize - sizeof(DWORD), dwSize);
      if (cnt % dwSize != 0) {
	ERR("Invalid Array Size\n");
	return E_FAIL;
      }
      cnt /= dwSize;
      /** skip for now */
      liMove.QuadPart = cnt * dwSize;
      IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
      break;
    }
    case DMUS_FOURCC_MARKER_CHUNK: { 
      TRACE_(dmfile)(": Marker chunk (skipping for now)\n");
      liMove.QuadPart = Chunk.dwSize;
      IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
      break;
    }
    case DMUS_FOURCC_RESOLUTION_CHUNK: { 
      TRACE_(dmfile)(": Resolution chunk (skipping for now)\n");
      liMove.QuadPart = Chunk.dwSize;
      IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
      break;
    }
    case DMUS_FOURCC_ANTICIPATION_CHUNK: { 
      TRACE_(dmfile)(": Anticipation chunk (skipping for now)\n");
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
      case DMUS_FOURCC_UNFO_LIST: { 
	TRACE_(dmfile)(": UNFO list\n");
	do {
	  IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
	  ListCount[1] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
          TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
	  
	  hr = IDirectMusicUtils_IPersistStream_ParseUNFOGeneric(&Chunk, pStm, &desc);
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
          TRACE_(dmfile)(": ListCount[1] = %ld < ListSize[1] = %ld\n", ListCount[1], ListSize[1]);
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
    break;
    }
    default: {
      TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
      liMove.QuadPart = Chunk.dwSize;
      IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
      break;						
    }
    }
    TRACE_(dmfile)(": ListCount[0] = %ld < ListSize[0] = %ld\n", ListCount[0], ListSize[0]);
  } while (ListCount[0] < ListSize[0]);

  return S_OK;
}

static HRESULT parse_pattern_list(IDirectMusicStyle8Impl *This, DMUS_PRIVATE_CHUNK *pChunk,
        IStream *pStm)
{
  HRESULT hr = E_FAIL;
  DMUS_PRIVATE_CHUNK Chunk;
  DWORD ListSize[3], ListCount[3];
  LARGE_INTEGER liMove; /* used when skipping chunks */
  IDirectMusicBand* pBand = NULL;
  struct style_motif *pNewMotif = NULL;

  if (pChunk->fccID != DMUS_FOURCC_PATTERN_LIST) {
    ERR_(dmfile)(": %s chunk should be a PATTERN list\n", debugstr_fourcc (pChunk->fccID));
    return E_FAIL;
  }  

  ListSize[0] = pChunk->dwSize - sizeof(FOURCC);
  ListCount[0] = 0;

  do {
    IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
    ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
    TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
    switch (Chunk.fccID) {
    case DMUS_FOURCC_PATTERN_CHUNK: {
      TRACE_(dmfile)(": Pattern chunk\n");
      /** alloc new motif entry */
      pNewMotif = heap_alloc_zero(sizeof(*pNewMotif));
      if (NULL == pNewMotif) {
	ERR(": no more memory\n");
	return  E_OUTOFMEMORY;
      }
      list_add_tail(&This->motifs, &pNewMotif->entry);

      IStream_Read (pStm, &pNewMotif->pattern, Chunk.dwSize, NULL);
      /** TODO trace pattern */

      /** reset all data, as a new pattern begin */
      pNewMotif->desc.dwSize = sizeof(pNewMotif->desc);
      list_init (&pNewMotif->Items);
      break;
    }
    case DMUS_FOURCC_RHYTHM_CHUNK: { 
      TRACE_(dmfile)(": Rhythm chunk\n");
      IStream_Read (pStm, &pNewMotif->dwRhythm, sizeof(DWORD), NULL);
      TRACE_(dmfile)(" - dwRhythm: %lu\n", pNewMotif->dwRhythm);
      /** TODO understand why some Chunks have size > 4 */
      liMove.QuadPart = Chunk.dwSize - sizeof(DWORD);
      IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
      break;
    }
    case DMUS_FOURCC_MOTIFSETTINGS_CHUNK: {
      TRACE_(dmfile)(": MotifSettings chunk (skipping for now)\n");
      IStream_Read (pStm, &pNewMotif->settings, Chunk.dwSize, NULL);
      /** TODO trace settings */
      break;
    }
    case FOURCC_RIFF: {
      /**
       * should be embedded Bands into pattern
       */
      IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);
      TRACE_(dmfile)(": RIFF chunk of type %s", debugstr_fourcc(Chunk.fccID));
      ListSize[1] = Chunk.dwSize - sizeof(FOURCC);
      ListCount[1] = 0;
      switch (Chunk.fccID) {
      case DMUS_FOURCC_BAND_FORM: { 
	LPSTREAM pClonedStream = NULL;
	
	TRACE_(dmfile)(": BAND RIFF\n");
	
	IStream_Clone (pStm, &pClonedStream);
	
	liMove.QuadPart = 0;
	liMove.QuadPart -= sizeof(FOURCC) + (sizeof(FOURCC)+sizeof(DWORD));
	IStream_Seek (pClonedStream, liMove, STREAM_SEEK_CUR, NULL);

        hr = load_band(pClonedStream, &pBand);
	if (FAILED(hr)) {
	  ERR(": could not load track\n");
	  return hr;
	}
	IStream_Release (pClonedStream);
	
	pNewMotif->pBand = pBand;
	IDirectMusicBand_AddRef(pBand);

	IDirectMusicTrack_Release(pBand); pBand = NULL;  /* now we can release it as it's inserted */
	
	/** now safe move the cursor */
	liMove.QuadPart = ListSize[1];
	IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
	
	break;
      }
      default: {
	TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
	liMove.QuadPart = ListSize[1];
	IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
	break;
      }
      }
      break;
    }
    case FOURCC_LIST: {
      IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);
      TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(Chunk.fccID));
      ListSize[1] = Chunk.dwSize - sizeof(FOURCC);
      ListCount[1] = 0;
      switch (Chunk.fccID) {
      case DMUS_FOURCC_UNFO_LIST: { 
	TRACE_(dmfile)(": UNFO list\n");
	do {
	  IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
	  ListCount[1] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
          TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
	  
	  hr = IDirectMusicUtils_IPersistStream_ParseUNFOGeneric(&Chunk, pStm, &pNewMotif->desc);
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
          TRACE_(dmfile)(": ListCount[1] = %ld < ListSize[1] = %ld\n", ListCount[1], ListSize[1]);
	} while (ListCount[1] < ListSize[1]);
	break;
      }
      case DMUS_FOURCC_PARTREF_LIST: {
	TRACE_(dmfile)(": PartRef list\n");
        hr = parse_part_ref_list(&Chunk, pStm, pNewMotif);
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
    TRACE_(dmfile)(": ListCount[0] = %ld < ListSize[0] = %ld\n", ListCount[0], ListSize[0]);
  } while (ListCount[0] < ListSize[0]);

  return S_OK;
}

static HRESULT parse_style_form(IDirectMusicStyle8Impl *This, DMUS_PRIVATE_CHUNK *pChunk,
        IStream *pStm)
{
  HRESULT hr = E_FAIL;
  DMUS_PRIVATE_CHUNK Chunk;
  DWORD StreamSize, StreamCount, ListSize[3], ListCount[3];
  LARGE_INTEGER liMove; /* used when skipping chunks */

  IDirectMusicBand* pBand = NULL;

  if (pChunk->fccID != DMUS_FOURCC_STYLE_FORM) {
    ERR_(dmfile)(": %s chunk should be a STYLE form\n", debugstr_fourcc (pChunk->fccID));
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
      case DMUS_FOURCC_STYLE_CHUNK: {
	TRACE_(dmfile)(": Style chunk\n");
	IStream_Read (pStm, &This->style, sizeof(DMUS_IO_STYLE), NULL);
	/** TODO dump DMUS_IO_TIMESIG style.timeSig */
	TRACE_(dmfile)(" - dblTempo: %g\n", This->style.dblTempo);
	break;
      }   
      case FOURCC_RIFF: {
	/**
	 * should be embedded Bands into style
	 */
	IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);
	TRACE_(dmfile)(": RIFF chunk of type %s", debugstr_fourcc(Chunk.fccID));
	ListSize[0] = Chunk.dwSize - sizeof(FOURCC);
	ListCount[0] = 0;
	switch (Chunk.fccID) {
	case DMUS_FOURCC_BAND_FORM: { 
          ULARGE_INTEGER save;
          struct style_band *pNewBand;

	  TRACE_(dmfile)(": BAND RIFF\n");

          /* Can be application provided IStream without Clone method */
	  liMove.QuadPart = 0;
	  liMove.QuadPart -= sizeof(FOURCC) + (sizeof(FOURCC)+sizeof(DWORD));
          IStream_Seek(pStm, liMove, STREAM_SEEK_CUR, &save);

          hr = load_band(pStm, &pBand);
	  if (FAILED(hr)) {
	    ERR(": could not load track\n");
	    return hr;
	  }

          pNewBand = heap_alloc_zero(sizeof(*pNewBand));
	  if (NULL == pNewBand) {
	    ERR(": no more memory\n");
	    return  E_OUTOFMEMORY;
	  }
	  pNewBand->pBand = pBand;
	  IDirectMusicBand_AddRef(pBand);
	  list_add_tail(&This->bands, &pNewBand->entry);

	  IDirectMusicTrack_Release(pBand); pBand = NULL;  /* now we can release it as it's inserted */
	
	  /** now safely move the cursor */
          liMove.QuadPart = save.QuadPart - liMove.QuadPart + ListSize[0];
          IStream_Seek(pStm, liMove, STREAM_SEEK_SET, NULL);

	  break;
	}
	default: {
	  TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
	  liMove.QuadPart = ListSize[0];
	  IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
	  break;
	}
	}
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
	case DMUS_FOURCC_PART_LIST: {
	  TRACE_(dmfile)(": PART list\n");
          hr = parse_part_list(&Chunk, pStm);
	  if (FAILED(hr)) return hr;
	  break;
	}
	case  DMUS_FOURCC_PATTERN_LIST: {
	  TRACE_(dmfile)(": PATTERN list\n");
          hr = parse_pattern_list(This, &Chunk, pStm);
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

static HRESULT WINAPI IPersistStreamImpl_Load(IPersistStream *iface, IStream *pStm)
{
  IDirectMusicStyle8Impl *This = impl_from_IPersistStream(iface);
  DMUS_PRIVATE_CHUNK Chunk;
  LARGE_INTEGER liMove; /* used when skipping chunks */
  HRESULT hr;

  FIXME("(%p, %p): Loading\n", This, pStm);

  IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
  TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
  switch (Chunk.fccID) {
  case FOURCC_RIFF: {
    IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);
    TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
    switch (Chunk.fccID) {
    case DMUS_FOURCC_STYLE_FORM: {
      TRACE_(dmfile)(": Style form\n");
      hr = parse_style_form(This, &Chunk, pStm);
      if (FAILED(hr)) return hr;
      break;
    }
    default: {
      TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
      liMove.QuadPart = Chunk.dwSize;
      IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
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
HRESULT WINAPI create_dmstyle(REFIID lpcGUID, void **ppobj)
{
  IDirectMusicStyle8Impl* obj;
  HRESULT hr;

  obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicStyle8Impl));
  if (NULL == obj) {
    *ppobj = NULL;
    return E_OUTOFMEMORY;
  }
  obj->IDirectMusicStyle8_iface.lpVtbl = &dmstyle8_vtbl;
  obj->ref = 1;
  dmobject_init(&obj->dmobj, &CLSID_DirectMusicStyle, (IUnknown *)&obj->IDirectMusicStyle8_iface);
  obj->dmobj.IDirectMusicObject_iface.lpVtbl = &dmobject_vtbl;
  obj->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;
  list_init(&obj->bands);
  list_init(&obj->motifs);

  DMSTYLE_LockModule();
  hr = IDirectMusicStyle8_QueryInterface(&obj->IDirectMusicStyle8_iface, lpcGUID, ppobj);
  IDirectMusicStyle8_Release(&obj->IDirectMusicStyle8_iface);

  return hr;
}
