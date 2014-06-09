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

WINE_DEFAULT_DEBUG_CHANNEL(dmstyle);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

/*****************************************************************************
 * IDirectMusicStyleImpl implementation
 */
typedef struct IDirectMusicStyle8Impl {
    IDirectMusicStyle8 IDirectMusicStyle8_iface;
    const IDirectMusicObjectVtbl *ObjectVtbl;
    const IPersistStreamVtbl *PersistStreamVtbl;
    LONG ref;
    DMUS_OBJECTDESC *pDesc;
    DMUS_IO_STYLE style;
    struct list Motifs;
    struct list Bands;
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
        *ret_iface = &This->ObjectVtbl;
    else if (IsEqualIID(riid, &IID_IPersistStream))
        *ret_iface = &This->PersistStreamVtbl;
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

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirectMusicStyle8Impl_Release(IDirectMusicStyle8 *iface)
{
    IDirectMusicStyle8Impl *This = impl_from_IDirectMusicStyle8(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if (!ref) {
        HeapFree(GetProcessHeap(), 0, This->pDesc);
        HeapFree(GetProcessHeap(), 0, This);
        DMSTYLE_UnlockModule();
    }

    return ref;
}

/* IDirectMusicStyle8Impl IDirectMusicStyle(8) part: */
static HRESULT WINAPI IDirectMusicStyle8Impl_GetBand(IDirectMusicStyle8 *iface, WCHAR *pwszName,
        IDirectMusicBand **ppBand)
{
        IDirectMusicStyle8Impl *This = impl_from_IDirectMusicStyle8(iface);
	FIXME("(%p, %p, %p): stub\n", This, pwszName, ppBand);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicStyle8Impl_EnumBand(IDirectMusicStyle8 *iface, DWORD dwIndex,
        WCHAR *pwszName)
{
        IDirectMusicStyle8Impl *This = impl_from_IDirectMusicStyle8(iface);
	FIXME("(%p, %d, %p): stub\n", This, dwIndex, pwszName);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicStyle8Impl_GetDefaultBand(IDirectMusicStyle8 *iface,
        IDirectMusicBand **ppBand)
{
        IDirectMusicStyle8Impl *This = impl_from_IDirectMusicStyle8(iface);
	FIXME("(%p, %p): stub\n", This, ppBand);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicStyle8Impl_EnumMotif(IDirectMusicStyle8 *iface, DWORD dwIndex,
        WCHAR *pwszName)
{
        IDirectMusicStyle8Impl *This = impl_from_IDirectMusicStyle8(iface);
	FIXME("(%p, %d, %p): stub\n", This, dwIndex, pwszName);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicStyle8Impl_GetMotif(IDirectMusicStyle8 *iface, WCHAR *pwszName,
        IDirectMusicSegment **ppSegment)
{
        IDirectMusicStyle8Impl *This = impl_from_IDirectMusicStyle8(iface);
	FIXME("(%p, %p, %p): stub\n", This, pwszName, ppSegment);
	return S_OK;
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
	FIXME("(%p, %d, %p): stub\n", This, dwIndex, pwszName);
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
	FIXME("(%p, %d, %d, %p, %p): stub\n", This, dwType, dwLevel, pdwMin, pdwMax);
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
	FIXME("(%p, %d, %d, %p): stub\n", This, dwIndex, dwPatternType, pwszName);
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
static HRESULT WINAPI IDirectMusicStyle8Impl_IDirectMusicObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicStyle8Impl, ObjectVtbl, iface);
        return IDirectMusicStyle8_QueryInterface(&This->IDirectMusicStyle8_iface, riid, ppobj);
}

static ULONG WINAPI IDirectMusicStyle8Impl_IDirectMusicObject_AddRef (LPDIRECTMUSICOBJECT iface) {
	ICOM_THIS_MULTI(IDirectMusicStyle8Impl, ObjectVtbl, iface);
        return IDirectMusicStyle8_AddRef(&This->IDirectMusicStyle8_iface);
}

static ULONG WINAPI IDirectMusicStyle8Impl_IDirectMusicObject_Release (LPDIRECTMUSICOBJECT iface) {
	ICOM_THIS_MULTI(IDirectMusicStyle8Impl, ObjectVtbl, iface);
        return IDirectMusicStyle8_Release(&This->IDirectMusicStyle8_iface);
}

static HRESULT WINAPI IDirectMusicStyle8Impl_IDirectMusicObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc) {
	ICOM_THIS_MULTI(IDirectMusicStyle8Impl, ObjectVtbl, iface);
	TRACE("(%p, %p)\n", This, pDesc);
	/* I think we shouldn't return pointer here since then values can be changed; it'd be a mess */
	memcpy (pDesc, This->pDesc, This->pDesc->dwSize);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicStyle8Impl_IDirectMusicObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc) {
	ICOM_THIS_MULTI(IDirectMusicStyle8Impl, ObjectVtbl, iface);
	TRACE("(%p, %p): setting descriptor:\n%s\n", This, pDesc, debugstr_DMUS_OBJECTDESC (pDesc));

	/* According to MSDN, we should copy only given values, not whole struct */	
	if (pDesc->dwValidData & DMUS_OBJ_OBJECT)
		This->pDesc->guidObject = pDesc->guidObject;
	if (pDesc->dwValidData & DMUS_OBJ_CLASS)
		This->pDesc->guidClass = pDesc->guidClass;
	if (pDesc->dwValidData & DMUS_OBJ_NAME)
		lstrcpynW (This->pDesc->wszName, pDesc->wszName, DMUS_MAX_NAME);
	if (pDesc->dwValidData & DMUS_OBJ_CATEGORY)
		lstrcpynW (This->pDesc->wszCategory, pDesc->wszCategory, DMUS_MAX_CATEGORY);
	if (pDesc->dwValidData & DMUS_OBJ_FILENAME)
		lstrcpynW (This->pDesc->wszFileName, pDesc->wszFileName, DMUS_MAX_FILENAME);
	if (pDesc->dwValidData & DMUS_OBJ_VERSION)
		This->pDesc->vVersion = pDesc->vVersion;
	if (pDesc->dwValidData & DMUS_OBJ_DATE)
		This->pDesc->ftDate = pDesc->ftDate;
	if (pDesc->dwValidData & DMUS_OBJ_MEMORY) {
		This->pDesc->llMemLength = pDesc->llMemLength;
		memcpy (This->pDesc->pbMemData, pDesc->pbMemData, pDesc->llMemLength);
	}
	if (pDesc->dwValidData & DMUS_OBJ_STREAM) {
		/* according to MSDN, we copy the stream */
		IStream_Clone (pDesc->pStream, &This->pDesc->pStream);	
	}
	
	/* add new flags */
	This->pDesc->dwValidData |= pDesc->dwValidData;

	return S_OK;
}

static HRESULT WINAPI IDirectMusicStyle8Impl_IDirectMusicObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc) {
	ICOM_THIS_MULTI(IDirectMusicStyle8Impl, ObjectVtbl, iface);
	DMUS_PRIVATE_CHUNK Chunk;
	DWORD StreamSize, StreamCount, ListSize[1], ListCount[1];
	LARGE_INTEGER liMove; /* used when skipping chunks */

	TRACE("(%p, %p, %p)\n", This, pStream, pDesc);

	/* FIXME: should this be determined from stream? */
	pDesc->dwValidData |= DMUS_OBJ_CLASS;
	pDesc->guidClass = CLSID_DirectMusicStyle;

	IStream_Read (pStream, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
	TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
	switch (Chunk.fccID) {	
		case FOURCC_RIFF: {
			IStream_Read (pStream, &Chunk.fccID, sizeof(FOURCC), NULL);				
			TRACE_(dmfile)(": RIFF chunk of type %s", debugstr_fourcc(Chunk.fccID));
			StreamSize = Chunk.dwSize - sizeof(FOURCC);
			StreamCount = 0;
			if (Chunk.fccID == DMUS_FOURCC_STYLE_FORM) {
				TRACE_(dmfile)(": style form\n");
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
	
	TRACE(": returning descriptor:\n%s\n", debugstr_DMUS_OBJECTDESC (pDesc));
	
	return S_OK;
}

static const IDirectMusicObjectVtbl DirectMusicStyle8_Object_Vtbl = {
  IDirectMusicStyle8Impl_IDirectMusicObject_QueryInterface,
  IDirectMusicStyle8Impl_IDirectMusicObject_AddRef,
  IDirectMusicStyle8Impl_IDirectMusicObject_Release,
  IDirectMusicStyle8Impl_IDirectMusicObject_GetDescriptor,
  IDirectMusicStyle8Impl_IDirectMusicObject_SetDescriptor,
  IDirectMusicStyle8Impl_IDirectMusicObject_ParseDescriptor
};

/* IDirectMusicStyle8Impl IPersistStream part: */
static HRESULT WINAPI IDirectMusicStyle8Impl_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj) {
  ICOM_THIS_MULTI(IDirectMusicStyle8Impl, PersistStreamVtbl, iface);
  return IDirectMusicStyle8_QueryInterface(&This->IDirectMusicStyle8_iface, riid, ppobj);
}

static ULONG WINAPI IDirectMusicStyle8Impl_IPersistStream_AddRef (LPPERSISTSTREAM iface) {
  ICOM_THIS_MULTI(IDirectMusicStyle8Impl, PersistStreamVtbl, iface);
  return IDirectMusicStyle8_AddRef(&This->IDirectMusicStyle8_iface);
}

static ULONG WINAPI IDirectMusicStyle8Impl_IPersistStream_Release (LPPERSISTSTREAM iface) {
  ICOM_THIS_MULTI(IDirectMusicStyle8Impl, PersistStreamVtbl, iface);
  return IDirectMusicStyle8_Release(&This->IDirectMusicStyle8_iface);
}

static HRESULT WINAPI IDirectMusicStyle8Impl_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID) {
  ICOM_THIS_MULTI(IDirectMusicStyle8Impl, PersistStreamVtbl, iface);
  TRACE("(%p, %p)\n", This, pClassID);
  *pClassID = CLSID_DirectMusicStyle;
  return S_OK;
}

static HRESULT WINAPI IDirectMusicStyle8Impl_IPersistStream_IsDirty (LPPERSISTSTREAM iface) {
  ICOM_THIS_MULTI(IDirectMusicStyle8Impl, PersistStreamVtbl, iface);
  FIXME("(%p): stub, always S_FALSE\n", This);
  return S_FALSE;
}

static HRESULT IDirectMusicStyle8Impl_IPersistStream_LoadBand (LPPERSISTSTREAM iface, IStream* pClonedStream, IDirectMusicBand** ppBand) {

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

static HRESULT IDirectMusicStyle8Impl_IPersistStream_ParsePartRefList (LPPERSISTSTREAM iface, DMUS_PRIVATE_CHUNK* pChunk, IStream* pStm, LPDMUS_PRIVATE_STYLE_MOTIF pNewMotif) {
  /*ICOM_THIS_MULTI(IDirectMusicStyle8Impl, PersistStreamVtbl, iface);*/
  HRESULT hr = E_FAIL;
  DMUS_PRIVATE_CHUNK Chunk;
  DWORD ListSize[3], ListCount[3];
  LARGE_INTEGER liMove; /* used when skipping chunks */

  LPDMUS_PRIVATE_STYLE_PARTREF_ITEM pNewItem = NULL;


  if (pChunk->fccID != DMUS_FOURCC_PARTREF_LIST) {
    ERR_(dmfile)(": %s chunk should be a PARTREF list\n", debugstr_fourcc (pChunk->fccID));
    return E_FAIL;
  }  

  ListSize[0] = pChunk->dwSize - sizeof(FOURCC);
  ListCount[0] = 0;

  do {
    IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
    ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
    TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
    switch (Chunk.fccID) {
    case DMUS_FOURCC_PARTREF_CHUNK: {
      TRACE_(dmfile)(": PartRef chunk\n");
      pNewItem = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(DMUS_PRIVATE_STYLE_PARTREF_ITEM));
      if (!pNewItem) {
	ERR(": no more memory\n");
	return E_OUTOFMEMORY;
      }
      hr = IStream_Read (pStm, &pNewItem->part_ref, sizeof(DMUS_IO_PARTREF), NULL);
      /*TRACE_(dmfile)(" - sizeof %lu\n",  sizeof(DMUS_IO_PARTREF));*/
      list_add_tail (&pNewMotif->Items, &pNewItem->entry);      
      DM_STRUCT_INIT(&pNewItem->desc);
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
          TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
	  
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
          TRACE_(dmfile)(": ListCount[1] = %d < ListSize[1] = %d\n", ListCount[1], ListSize[1]);
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
    TRACE_(dmfile)(": ListCount[0] = %d < ListSize[0] = %d\n", ListCount[0], ListSize[0]);
  } while (ListCount[0] < ListSize[0]);

  return S_OK;
}

static HRESULT IDirectMusicStyle8Impl_IPersistStream_ParsePartList (LPPERSISTSTREAM iface, DMUS_PRIVATE_CHUNK* pChunk, IStream* pStm) {

  /*ICOM_THIS_MULTI(IDirectMusicStyle8Impl, PersistStreamVtbl, iface);*/
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
    TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
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
      TRACE_(dmfile)(" - dwSize: %u\n", dwSize);
      TRACE_(dmfile)(" - cnt: %u (%u / %u)\n", cnt / dwSize, (DWORD)(Chunk.dwSize - sizeof(DWORD)), dwSize);
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
      TRACE_(dmfile)(" - dwSize: %u\n", dwSize);
      TRACE_(dmfile)(" - cnt: %u (%u / %u)\n", cnt / dwSize, (DWORD)(Chunk.dwSize - sizeof(DWORD)), dwSize);
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
          TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
	  
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

static HRESULT IDirectMusicStyle8Impl_IPersistStream_ParsePatternList (LPPERSISTSTREAM iface, DMUS_PRIVATE_CHUNK* pChunk, IStream* pStm) {

  ICOM_THIS_MULTI(IDirectMusicStyle8Impl, PersistStreamVtbl, iface);
  HRESULT hr = E_FAIL;
  DMUS_PRIVATE_CHUNK Chunk;
  DWORD ListSize[3], ListCount[3];
  LARGE_INTEGER liMove; /* used when skipping chunks */

  DMUS_OBJECTDESC desc;
  IDirectMusicBand* pBand = NULL;
  LPDMUS_PRIVATE_STYLE_MOTIF pNewMotif = NULL;

  DM_STRUCT_INIT(&desc);

  if (pChunk->fccID != DMUS_FOURCC_PATTERN_LIST) {
    ERR_(dmfile)(": %s chunk should be a PATTERN list\n", debugstr_fourcc (pChunk->fccID));
    return E_FAIL;
  }  

  ListSize[0] = pChunk->dwSize - sizeof(FOURCC);
  ListCount[0] = 0;

  do {
    IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
    ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
    TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
    switch (Chunk.fccID) {
    case DMUS_FOURCC_PATTERN_CHUNK: {
      TRACE_(dmfile)(": Pattern chunk\n");
      /** alloc new motif entry */
      pNewMotif = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(DMUS_PRIVATE_STYLE_MOTIF));
      if (NULL == pNewMotif) {
	ERR(": no more memory\n");
	return  E_OUTOFMEMORY;
      }
      list_add_tail (&This->Motifs, &pNewMotif->entry);

      IStream_Read (pStm, &pNewMotif->pattern, Chunk.dwSize, NULL);
      /** TODO trace pattern */

      /** reset all data, as a new pattern begin */
      DM_STRUCT_INIT(&pNewMotif->desc);
      list_init (&pNewMotif->Items);
      break;
    }
    case DMUS_FOURCC_RHYTHM_CHUNK: { 
      TRACE_(dmfile)(": Rhythm chunk\n");
      IStream_Read (pStm, &pNewMotif->dwRhythm, sizeof(DWORD), NULL);
      TRACE_(dmfile)(" - dwRhythm: %u\n", pNewMotif->dwRhythm);
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
	
	hr = IDirectMusicStyle8Impl_IPersistStream_LoadBand (iface, pClonedStream, &pBand);
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
          TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
	  
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
          TRACE_(dmfile)(": ListCount[1] = %d < ListSize[1] = %d\n", ListCount[1], ListSize[1]);
	} while (ListCount[1] < ListSize[1]);
	break;
      }
      case DMUS_FOURCC_PARTREF_LIST: {
	TRACE_(dmfile)(": PartRef list\n");
	hr = IDirectMusicStyle8Impl_IPersistStream_ParsePartRefList (iface, &Chunk, pStm, pNewMotif);
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
    TRACE_(dmfile)(": ListCount[0] = %d < ListSize[0] = %d\n", ListCount[0], ListSize[0]);
  } while (ListCount[0] < ListSize[0]);

  return S_OK;
}

static HRESULT IDirectMusicStyle8Impl_IPersistStream_ParseStyleForm (LPPERSISTSTREAM iface, DMUS_PRIVATE_CHUNK* pChunk, IStream* pStm) {
  ICOM_THIS_MULTI(IDirectMusicStyle8Impl, PersistStreamVtbl, iface);

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
    TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
    
    hr = IDirectMusicUtils_IPersistStream_ParseDescGeneric(&Chunk, pStm, This->pDesc);
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
	  LPSTREAM pClonedStream = NULL;
	  LPDMUS_PRIVATE_STYLE_BAND pNewBand;

	  TRACE_(dmfile)(": BAND RIFF\n");
	  
	  IStream_Clone (pStm, &pClonedStream);
	    
	  liMove.QuadPart = 0;
	  liMove.QuadPart -= sizeof(FOURCC) + (sizeof(FOURCC)+sizeof(DWORD));
	  IStream_Seek (pClonedStream, liMove, STREAM_SEEK_CUR, NULL);
	  
	  hr = IDirectMusicStyle8Impl_IPersistStream_LoadBand (iface, pClonedStream, &pBand);
	  if (FAILED(hr)) {
	    ERR(": could not load track\n");
	    return hr;
	  }
	  IStream_Release (pClonedStream);
	  
	  pNewBand = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(DMUS_PRIVATE_STYLE_BAND));
	  if (NULL == pNewBand) {
	    ERR(": no more memory\n");
	    return  E_OUTOFMEMORY;
	  }
	  pNewBand->pBand = pBand;
	  IDirectMusicBand_AddRef(pBand);
	  list_add_tail (&This->Bands, &pNewBand->entry);

	  IDirectMusicTrack_Release(pBand); pBand = NULL;  /* now we can release it as it's inserted */
	
	  /** now safely move the cursor */
	  liMove.QuadPart = ListSize[0];
	  IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
	  
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
	case DMUS_FOURCC_PART_LIST: {
	  TRACE_(dmfile)(": PART list\n");
	  hr = IDirectMusicStyle8Impl_IPersistStream_ParsePartList (iface, &Chunk, pStm);
	  if (FAILED(hr)) return hr;
	  break;
	}
	case  DMUS_FOURCC_PATTERN_LIST: {
	  TRACE_(dmfile)(": PATTERN list\n");
	  hr = IDirectMusicStyle8Impl_IPersistStream_ParsePatternList (iface, &Chunk, pStm);
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

static HRESULT WINAPI IDirectMusicStyle8Impl_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm) {
  ICOM_THIS_MULTI(IDirectMusicStyle8Impl, PersistStreamVtbl, iface);
  
  DMUS_PRIVATE_CHUNK Chunk;
  LARGE_INTEGER liMove; /* used when skipping chunks */
  HRESULT hr;

  FIXME("(%p, %p): Loading\n", This, pStm);

  IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
  TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
  switch (Chunk.fccID) {
  case FOURCC_RIFF: {
    IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);
    TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
    switch (Chunk.fccID) {
    case DMUS_FOURCC_STYLE_FORM: {
      TRACE_(dmfile)(": Style form\n");
      hr = IDirectMusicStyle8Impl_IPersistStream_ParseStyleForm (iface, &Chunk, pStm);
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

static HRESULT WINAPI IDirectMusicStyle8Impl_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty) {
  ICOM_THIS_MULTI(IDirectMusicStyle8Impl, PersistStreamVtbl, iface);
  FIXME("(%p): Saving not implemented yet\n", This);
  return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicStyle8Impl_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize) {
  ICOM_THIS_MULTI(IDirectMusicStyle8Impl, PersistStreamVtbl, iface);
  FIXME("(%p, %p): stub\n", This, pcbSize);
  return E_NOTIMPL;

}

static const IPersistStreamVtbl DirectMusicStyle8_PersistStream_Vtbl = {
  IDirectMusicStyle8Impl_IPersistStream_QueryInterface,
  IDirectMusicStyle8Impl_IPersistStream_AddRef,
  IDirectMusicStyle8Impl_IPersistStream_Release,
  IDirectMusicStyle8Impl_IPersistStream_GetClassID,
  IDirectMusicStyle8Impl_IPersistStream_IsDirty,
  IDirectMusicStyle8Impl_IPersistStream_Load,
  IDirectMusicStyle8Impl_IPersistStream_Save,
  IDirectMusicStyle8Impl_IPersistStream_GetSizeMax
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
  obj->ObjectVtbl = &DirectMusicStyle8_Object_Vtbl;
  obj->PersistStreamVtbl = &DirectMusicStyle8_PersistStream_Vtbl;
  obj->pDesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DMUS_OBJECTDESC));
  DM_STRUCT_INIT(obj->pDesc);
  obj->pDesc->dwValidData |= DMUS_OBJ_CLASS;
  obj->pDesc->guidClass = CLSID_DirectMusicStyle;
  obj->ref = 1;
  list_init (&obj->Bands);
  list_init (&obj->Motifs);

  DMSTYLE_LockModule();
  hr = IDirectMusicStyle8_QueryInterface(&obj->IDirectMusicStyle8_iface, lpcGUID, ppobj);
  IDirectMusicStyle8_Release(&obj->IDirectMusicStyle8_iface);

  return hr;
}
