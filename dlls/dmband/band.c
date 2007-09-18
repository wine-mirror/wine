/* IDirectMusicBand Implementation
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
 * IDirectMusicBandImpl implementation
 */
/* IDirectMusicBandImpl IUnknown part: */
static HRESULT WINAPI IDirectMusicBandImpl_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicBandImpl, UnknownVtbl, iface);
	
	TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);
	if (IsEqualIID (riid, &IID_IUnknown)) {
		*ppobj = (LPVOID)&This->UnknownVtbl;
		IUnknown_AddRef (iface);
		return S_OK;	
	} else if (IsEqualIID (riid, &IID_IDirectMusicBand)) {
		*ppobj = (LPVOID)&This->BandVtbl;
		IUnknown_AddRef (iface);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IDirectMusicObject)) {
		*ppobj = (LPVOID)&This->ObjectVtbl;
		IUnknown_AddRef (iface);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IPersistStream)) {
		*ppobj = (LPVOID)&This->PersistStreamVtbl;
		IUnknown_AddRef (iface);
		return S_OK;
	}
	
	WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicBandImpl_IUnknown_AddRef (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicBandImpl, UnknownVtbl, iface);
        ULONG ref = InterlockedIncrement(&This->ref);

	TRACE("(%p) : AddRef from %d\n", This, ref - 1);

	DMBAND_LockModule();

	return ref;
}

static ULONG WINAPI IDirectMusicBandImpl_IUnknown_Release (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicBandImpl, UnknownVtbl, iface);
	ULONG ref = InterlockedDecrement(&This->ref);

	TRACE("(%p) : ReleaseRef to %d\n", This, ref);

	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}

	DMBAND_UnlockModule();
	
	return ref;
}

static const IUnknownVtbl DirectMusicBand_Unknown_Vtbl = {
	IDirectMusicBandImpl_IUnknown_QueryInterface,
	IDirectMusicBandImpl_IUnknown_AddRef,
	IDirectMusicBandImpl_IUnknown_Release
};

/* IDirectMusicBandImpl IDirectMusicBand part: */
static HRESULT WINAPI IDirectMusicBandImpl_IDirectMusicBand_QueryInterface (LPDIRECTMUSICBAND iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicBandImpl, BandVtbl, iface);
	return IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicBandImpl_IDirectMusicBand_AddRef (LPDIRECTMUSICBAND iface) {
	ICOM_THIS_MULTI(IDirectMusicBandImpl, BandVtbl, iface);
	return IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

static ULONG WINAPI IDirectMusicBandImpl_IDirectMusicBand_Release (LPDIRECTMUSICBAND iface) {
	ICOM_THIS_MULTI(IDirectMusicBandImpl, BandVtbl, iface);
	return IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

static HRESULT WINAPI IDirectMusicBandImpl_IDirectMusicBand_CreateSegment (LPDIRECTMUSICBAND iface, IDirectMusicSegment** ppSegment) {
	ICOM_THIS_MULTI(IDirectMusicBandImpl, BandVtbl, iface);
	FIXME("(%p, %p): stub\n", This, ppSegment);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicBandImpl_IDirectMusicBand_Download (LPDIRECTMUSICBAND iface, IDirectMusicPerformance* pPerformance) {
	ICOM_THIS_MULTI(IDirectMusicBandImpl, BandVtbl, iface);
	FIXME("(%p, %p): stub\n", This, pPerformance);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicBandImpl_IDirectMusicBand_Unload (LPDIRECTMUSICBAND iface, IDirectMusicPerformance* pPerformance) {
	ICOM_THIS_MULTI(IDirectMusicBandImpl, BandVtbl, iface);
	FIXME("(%p, %p): stub\n", This, pPerformance);
	return S_OK;
}

static const IDirectMusicBandVtbl DirectMusicBand_Band_Vtbl = {
	IDirectMusicBandImpl_IDirectMusicBand_QueryInterface,
	IDirectMusicBandImpl_IDirectMusicBand_AddRef,
	IDirectMusicBandImpl_IDirectMusicBand_Release,
	IDirectMusicBandImpl_IDirectMusicBand_CreateSegment,
	IDirectMusicBandImpl_IDirectMusicBand_Download,
	IDirectMusicBandImpl_IDirectMusicBand_Unload
};

/* IDirectMusicBandImpl IDirectMusicObject part: */
static HRESULT WINAPI IDirectMusicBandImpl_IDirectMusicObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicBandImpl, ObjectVtbl, iface);
	return IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicBandImpl_IDirectMusicObject_AddRef (LPDIRECTMUSICOBJECT iface) {
	ICOM_THIS_MULTI(IDirectMusicBandImpl, ObjectVtbl, iface);
	return IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

static ULONG WINAPI IDirectMusicBandImpl_IDirectMusicObject_Release (LPDIRECTMUSICOBJECT iface) {
	ICOM_THIS_MULTI(IDirectMusicBandImpl, ObjectVtbl, iface);
	return IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

static HRESULT WINAPI IDirectMusicBandImpl_IDirectMusicObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc) {
	ICOM_THIS_MULTI(IDirectMusicBandImpl, ObjectVtbl, iface);
	TRACE("(%p, %p)\n", This, pDesc);
	/* I think we shouldn't return pointer here since then values can be changed; it'd be a mess */
	memcpy (pDesc, This->pDesc, This->pDesc->dwSize);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicBandImpl_IDirectMusicObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc) {
	ICOM_THIS_MULTI(IDirectMusicBandImpl, ObjectVtbl, iface);
	TRACE("(%p, %p): setting descriptor:\n", This, pDesc); debug_DMUS_OBJECTDESC (pDesc);
	
	/* According to MSDN, we should copy only given values, not whole struct */	
	if (pDesc->dwValidData & DMUS_OBJ_OBJECT)
		memcpy (&This->pDesc->guidObject, &pDesc->guidObject, sizeof (pDesc->guidObject));
	if (pDesc->dwValidData & DMUS_OBJ_CLASS)
		memcpy (&This->pDesc->guidClass, &pDesc->guidClass, sizeof (pDesc->guidClass));		
	if (pDesc->dwValidData & DMUS_OBJ_NAME)
		lstrcpynW (This->pDesc->wszName, pDesc->wszName, DMUS_MAX_NAME);
	if (pDesc->dwValidData & DMUS_OBJ_CATEGORY)
		lstrcpynW (This->pDesc->wszCategory, pDesc->wszCategory, DMUS_MAX_CATEGORY);
	if (pDesc->dwValidData & DMUS_OBJ_FILENAME)
		lstrcpynW (This->pDesc->wszFileName, pDesc->wszFileName, DMUS_MAX_FILENAME);
	if (pDesc->dwValidData & DMUS_OBJ_VERSION)
		memcpy (&This->pDesc->vVersion, &pDesc->vVersion, sizeof (pDesc->vVersion));				
	if (pDesc->dwValidData & DMUS_OBJ_DATE)
		memcpy (&This->pDesc->ftDate, &pDesc->ftDate, sizeof (pDesc->ftDate));				
	if (pDesc->dwValidData & DMUS_OBJ_MEMORY) {
		memcpy (&This->pDesc->llMemLength, &pDesc->llMemLength, sizeof (pDesc->llMemLength));				
		memcpy (This->pDesc->pbMemData, pDesc->pbMemData, sizeof (pDesc->pbMemData));
	}
	if (pDesc->dwValidData & DMUS_OBJ_STREAM) {
		/* according to MSDN, we copy the stream */
		IStream_Clone (pDesc->pStream, &This->pDesc->pStream);	
	}
	
	/* add new flags */
	This->pDesc->dwValidData |= pDesc->dwValidData;

	return S_OK;
}

static HRESULT WINAPI IDirectMusicBandImpl_IDirectMusicObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc) {
	DMUS_PRIVATE_CHUNK Chunk;
	DWORD StreamSize, StreamCount, ListSize[1], ListCount[1];
	LARGE_INTEGER liMove; /* used when skipping chunks */

	TRACE("(%p, %p)\n", pStream, pDesc);
	
	/* FIXME: should this be determined from stream? */
	pDesc->dwValidData |= DMUS_OBJ_CLASS;
	memcpy (&pDesc->guidClass, &CLSID_DirectMusicBand, sizeof(CLSID));
	
	IStream_Read (pStream, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
	TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
	switch (Chunk.fccID) {	
		case FOURCC_RIFF: {
			IStream_Read (pStream, &Chunk.fccID, sizeof(FOURCC), NULL);				
			TRACE_(dmfile)(": RIFF chunk of type %s", debugstr_fourcc(Chunk.fccID));
			StreamSize = Chunk.dwSize - sizeof(FOURCC);
			StreamCount = 0;
			if (Chunk.fccID == DMUS_FOURCC_BAND_FORM) {
				TRACE_(dmfile)(": band form\n");
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
												TRACE_(dmfile)(": unknown chunk (irrevelant & skipping)\n");
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
							TRACE_(dmfile)(": unknown chunk (irrevelant & skipping)\n");
							liMove.QuadPart = Chunk.dwSize;
							IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL);
							break;						
						}
					}
					TRACE_(dmfile)(": StreamCount[0] = %d < StreamSize[0] = %d\n", StreamCount, StreamSize);
				} while (StreamCount < StreamSize);
			} else {
				TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
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
	
	TRACE(": returning descriptor:\n"); debug_DMUS_OBJECTDESC (pDesc);
	
	return S_OK;
}

static const IDirectMusicObjectVtbl DirectMusicBand_Object_Vtbl = {
  IDirectMusicBandImpl_IDirectMusicObject_QueryInterface,
  IDirectMusicBandImpl_IDirectMusicObject_AddRef,
  IDirectMusicBandImpl_IDirectMusicObject_Release,
  IDirectMusicBandImpl_IDirectMusicObject_GetDescriptor,
  IDirectMusicBandImpl_IDirectMusicObject_SetDescriptor,
  IDirectMusicBandImpl_IDirectMusicObject_ParseDescriptor
};

/* IDirectMusicBandImpl IPersistStream part: */
static HRESULT WINAPI IDirectMusicBandImpl_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj) {
  ICOM_THIS_MULTI(IDirectMusicBandImpl, PersistStreamVtbl, iface);
  return IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicBandImpl_IPersistStream_AddRef (LPPERSISTSTREAM iface) {
  ICOM_THIS_MULTI(IDirectMusicBandImpl, PersistStreamVtbl, iface);
  return IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

static ULONG WINAPI IDirectMusicBandImpl_IPersistStream_Release (LPPERSISTSTREAM iface) {
  ICOM_THIS_MULTI(IDirectMusicBandImpl, PersistStreamVtbl, iface);
  return IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);	
}

static HRESULT WINAPI IDirectMusicBandImpl_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID) {
  ICOM_THIS_MULTI(IDirectMusicBandImpl, PersistStreamVtbl, iface);
  TRACE("(%p, %p)\n", This, pClassID);
  memcpy(pClassID, &CLSID_DirectMusicBand, sizeof(CLSID));
  return S_OK;
}

static HRESULT WINAPI IDirectMusicBandImpl_IPersistStream_IsDirty (LPPERSISTSTREAM iface) {
  ICOM_THIS_MULTI(IDirectMusicBandImpl, PersistStreamVtbl, iface);
  FIXME("(%p): stub, always S_FALSE\n", This);
  return S_FALSE;
}

static HRESULT IDirectMusicBandImpl_IPersistStream_ParseInstrument (LPPERSISTSTREAM iface, DMUS_PRIVATE_CHUNK* pChunk, IStream* pStm) {
  ICOM_THIS_MULTI(IDirectMusicBandImpl, PersistStreamVtbl, iface);
  DMUS_PRIVATE_CHUNK Chunk;
  DWORD ListSize[3], ListCount[3];
  LARGE_INTEGER liMove; /* used when skipping chunks */
  HRESULT hr;

  DMUS_IO_INSTRUMENT inst;
  LPDMUS_PRIVATE_INSTRUMENT pNewInstrument;
  IDirectMusicObject* pObject = NULL;

  if (pChunk->fccID != DMUS_FOURCC_INSTRUMENT_LIST) {
    ERR_(dmfile)(": %s chunk should be an INSTRUMENT list\n", debugstr_fourcc (pChunk->fccID));
    return E_FAIL;
  }

  ListSize[0] = pChunk->dwSize - sizeof(FOURCC);
  ListCount[0] = 0;

  do {
    IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
    ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
    TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
    switch (Chunk.fccID) { 
    case DMUS_FOURCC_INSTRUMENT_CHUNK: {
      TRACE_(dmfile)(": Instrument chunk\n");
      if (Chunk.dwSize != sizeof(DMUS_IO_INSTRUMENT)) return E_FAIL;
      IStream_Read (pStm, &inst, sizeof(DMUS_IO_INSTRUMENT), NULL);
      TRACE_(dmfile)(" - dwPatch: %u\n", inst.dwPatch);
      TRACE_(dmfile)(" - dwAssignPatch: %u\n", inst.dwAssignPatch);
      TRACE_(dmfile)(" - dwNoteRanges[0]: %u\n", inst.dwNoteRanges[0]);
      TRACE_(dmfile)(" - dwNoteRanges[1]: %u\n", inst.dwNoteRanges[1]);
      TRACE_(dmfile)(" - dwNoteRanges[2]: %u\n", inst.dwNoteRanges[2]);
      TRACE_(dmfile)(" - dwNoteRanges[3]: %u\n", inst.dwNoteRanges[3]);
      TRACE_(dmfile)(" - dwPChannel: %u\n", inst.dwPChannel);
      TRACE_(dmfile)(" - dwFlags: %u\n", inst.dwFlags);
      TRACE_(dmfile)(" - bPan: %u\n", inst.bPan);
      TRACE_(dmfile)(" - bVolume: %u\n", inst.bVolume);
      TRACE_(dmfile)(" - nTranspose: %d\n", inst.nTranspose);
      TRACE_(dmfile)(" - dwChannelPriority: %u\n", inst.dwChannelPriority);
      TRACE_(dmfile)(" - nPitchBendRange: %d\n", inst.nPitchBendRange);
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
  
  /*
   * @TODO insert pNewInstrument into This
   */
  pNewInstrument = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(DMUS_PRIVATE_INSTRUMENT));
  if (NULL == pNewInstrument) {
    ERR(": no more memory\n");
    return  E_OUTOFMEMORY;
  }
  memcpy(&pNewInstrument->pInstrument, &inst, sizeof(DMUS_IO_INSTRUMENT));
  pNewInstrument->ppReferenceCollection = NULL;
  if (NULL != pObject) {
    IDirectMusicCollection* pCol = NULL;
    hr = IDirectMusicObject_QueryInterface (pObject, &IID_IDirectMusicCollection, (void**) &pCol);
    if (FAILED(hr)) {
      ERR(": failed to get IDirectMusicCollection Interface from DMObject\n");
      HeapFree(GetProcessHeap(), 0, pNewInstrument);

      return hr;
    }
    pNewInstrument->ppReferenceCollection = pCol;
    IDirectMusicObject_Release(pObject);
  }
  list_add_tail (&This->Instruments, &pNewInstrument->entry);

  return S_OK;
}

static HRESULT IDirectMusicBandImpl_IPersistStream_ParseInstrumentsList (LPPERSISTSTREAM iface, DMUS_PRIVATE_CHUNK* pChunk, IStream* pStm) {
  /*ICOM_THIS_MULTI(IDirectMusicBandImpl, PersistStreamVtbl, iface);*/
  HRESULT hr;
  DMUS_PRIVATE_CHUNK Chunk;
  DWORD ListSize[3], ListCount[3];
  LARGE_INTEGER liMove; /* used when skipping chunks */

  if (pChunk->fccID != DMUS_FOURCC_INSTRUMENTS_LIST) {
    ERR_(dmfile)(": %s chunk should be an INSTRUMENTS list\n", debugstr_fourcc (pChunk->fccID));
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
      case DMUS_FOURCC_INSTRUMENT_LIST: {
	TRACE_(dmfile)(": Instrument list\n");
	hr = IDirectMusicBandImpl_IPersistStream_ParseInstrument (iface, &Chunk, pStm);
	if (FAILED(hr)) return hr;
	break;
      }
      default: {
	TRACE_(dmfile)(": unknown chunk (irrevelant & skipping)\n");
	liMove.QuadPart = ListSize[1];
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

static HRESULT IDirectMusicBandImpl_IPersistStream_ParseBandForm (LPPERSISTSTREAM iface, DMUS_PRIVATE_CHUNK* pChunk, IStream* pStm) {

  ICOM_THIS_MULTI(IDirectMusicBandImpl, PersistStreamVtbl, iface);
  HRESULT hr = E_FAIL;
  DMUS_PRIVATE_CHUNK Chunk;
  DWORD StreamSize, StreamCount, ListSize[3], ListCount[3];
  LARGE_INTEGER liMove; /* used when skipping chunks */

  GUID tmp_guid;

  if (pChunk->fccID != DMUS_FOURCC_BAND_FORM) {
    ERR_(dmfile)(": %s chunk should be a BAND form\n", debugstr_fourcc (pChunk->fccID));
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
      case DMUS_FOURCC_GUID_CHUNK: {
	TRACE_(dmfile)(": GUID\n");
	IStream_Read (pStm, &tmp_guid, sizeof(GUID), NULL);
	TRACE_(dmfile)(" - guid: %s\n", debugstr_dmguid(&tmp_guid));
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
		TRACE_(dmfile)(": unknown chunk (irrevelant & skipping)\n");
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
	case DMUS_FOURCC_INSTRUMENTS_LIST: {
	  TRACE_(dmfile)(": INSTRUMENTS list\n");
	  hr = IDirectMusicBandImpl_IPersistStream_ParseInstrumentsList (iface, &Chunk, pStm);
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
    }
    TRACE_(dmfile)(": StreamCount[0] = %d < StreamSize[0] = %d\n", StreamCount, StreamSize);
  } while (StreamCount < StreamSize);  

  return S_OK;
}

static HRESULT WINAPI IDirectMusicBandImpl_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm) {
  ICOM_THIS_MULTI(IDirectMusicBandImpl, PersistStreamVtbl, iface);

  DMUS_PRIVATE_CHUNK Chunk;
  LARGE_INTEGER liMove;
  HRESULT hr;
  
  TRACE("(%p,%p): loading\n", This, pStm);
  
  IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
  TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
  switch (Chunk.fccID) {
  case FOURCC_RIFF: {
    IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);
    TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
    switch (Chunk.fccID) {
    case DMUS_FOURCC_BAND_FORM: {
      TRACE_(dmfile)(": Band form\n");
      hr = IDirectMusicBandImpl_IPersistStream_ParseBandForm (iface, &Chunk, pStm);
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

static HRESULT WINAPI IDirectMusicBandImpl_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty) {
  ICOM_THIS_MULTI(IDirectMusicBandImpl, PersistStreamVtbl, iface);
  FIXME("(%p): Saving not implemented yet\n", This);
  return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicBandImpl_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize) {
  return E_NOTIMPL;
}

static const IPersistStreamVtbl DirectMusicBand_PersistStream_Vtbl = {
  IDirectMusicBandImpl_IPersistStream_QueryInterface,
  IDirectMusicBandImpl_IPersistStream_AddRef,
  IDirectMusicBandImpl_IPersistStream_Release,
  IDirectMusicBandImpl_IPersistStream_GetClassID,
  IDirectMusicBandImpl_IPersistStream_IsDirty,
  IDirectMusicBandImpl_IPersistStream_Load,
  IDirectMusicBandImpl_IPersistStream_Save,
  IDirectMusicBandImpl_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicBandImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter) {
  IDirectMusicBandImpl* obj;
	
  obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicBandImpl));
  if (NULL == obj) {
    *ppobj = NULL;
    return E_OUTOFMEMORY;
  }
  obj->UnknownVtbl = &DirectMusicBand_Unknown_Vtbl;
  obj->BandVtbl = &DirectMusicBand_Band_Vtbl;
  obj->ObjectVtbl = &DirectMusicBand_Object_Vtbl;
  obj->PersistStreamVtbl = &DirectMusicBand_PersistStream_Vtbl;
  obj->pDesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DMUS_OBJECTDESC));
  DM_STRUCT_INIT(obj->pDesc);
  obj->pDesc->dwValidData |= DMUS_OBJ_CLASS;
  memcpy (&obj->pDesc->guidClass, &CLSID_DirectMusicBand, sizeof (CLSID));
  obj->ref = 0; /* will be inited by QueryInterface */
  list_init (&obj->Instruments);
  
  return IDirectMusicBandImpl_IUnknown_QueryInterface ((LPUNKNOWN)&obj->UnknownVtbl, lpcGUID, ppobj);
}
