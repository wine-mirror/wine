/* IDirectMusicAudioPath Implementation
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

#include "dmime_private.h"
#include "dmobject.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmime);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

struct IDirectMusicAudioPathImpl {
    IDirectMusicAudioPath IDirectMusicAudioPath_iface;
    struct dmobject dmobj;
    LONG ref;

    /* IDirectMusicAudioPathImpl fields */
    LPDMUS_OBJECTDESC pDesc;

    IDirectMusicPerformance8* pPerf;
    IDirectMusicGraph*        pToolGraph;
    IDirectSoundBuffer*       pDSBuffer;
    IDirectSoundBuffer*       pPrimary;

    BOOL fActive;
};

static inline struct IDirectMusicAudioPathImpl *impl_from_IDirectMusicAudioPath(IDirectMusicAudioPath *iface)
{
    return CONTAINING_RECORD(iface, struct IDirectMusicAudioPathImpl, IDirectMusicAudioPath_iface);
}

static inline struct IDirectMusicAudioPathImpl *impl_from_IDirectMusicObject(IDirectMusicObject *iface)
{
    return CONTAINING_RECORD(iface, struct IDirectMusicAudioPathImpl, dmobj.IDirectMusicObject_iface);
}

static inline struct IDirectMusicAudioPathImpl *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, struct IDirectMusicAudioPathImpl, dmobj.IPersistStream_iface);
}

void set_audiopath_perf_pointer(IDirectMusicAudioPath *iface, IDirectMusicPerformance8 *performance)
{
    struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);
    This->pPerf = performance;
}

void set_audiopath_dsound_buffer(IDirectMusicAudioPath *iface, IDirectSoundBuffer *buffer)
{
    struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);
    This->pDSBuffer = buffer;
}

void set_audiopath_primary_dsound_buffer(IDirectMusicAudioPath *iface, IDirectSoundBuffer *buffer)
{
    struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);
    This->pPrimary = buffer;
}

/*****************************************************************************
 * IDirectMusicAudioPathImpl implementation
 */
static HRESULT WINAPI IDirectMusicAudioPathImpl_QueryInterface (IDirectMusicAudioPath *iface, REFIID riid, void **ppobj)
{
    struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);

    *ppobj = NULL;

    if (IsEqualIID (riid, &IID_IDirectMusicAudioPath) || IsEqualIID (riid, &IID_IUnknown))
        *ppobj = &This->IDirectMusicAudioPath_iface;
    else if (IsEqualIID (riid, &IID_IDirectMusicObject))
        *ppobj = &This->dmobj.IDirectMusicObject_iface;
    else if (IsEqualIID (riid, &IID_IPersistStream))
        *ppobj = &This->dmobj.IPersistStream_iface;

    if (*ppobj) {
        IUnknown_AddRef((IUnknown*)*ppobj);
        return S_OK;
    }

    WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicAudioPathImpl_AddRef (IDirectMusicAudioPath *iface)
{
    struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): AddRef from %d\n", This, ref - 1);

    DMIME_LockModule();
    return ref;
}

static ULONG WINAPI IDirectMusicAudioPathImpl_Release (IDirectMusicAudioPath *iface)
{
    struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): ReleaseRef to %d\n", This, ref);

    if (ref == 0) {
        if (This->pDSBuffer)
            IDirectSoundBuffer8_Release(This->pDSBuffer);
        This->pPerf = NULL;
        HeapFree(GetProcessHeap(), 0, This);
    }

    DMIME_UnlockModule();
    return ref;
}

static HRESULT WINAPI IDirectMusicAudioPathImpl_GetObjectInPath (IDirectMusicAudioPath *iface, DWORD dwPChannel, DWORD dwStage, DWORD dwBuffer,
    REFGUID guidObject, WORD dwIndex, REFGUID iidInterface, void** ppObject)
{
	struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);
	HRESULT hr;

	FIXME("(%p, %d, %d, %d, %s, %d, %s, %p): stub\n", This, dwPChannel, dwStage, dwBuffer, debugstr_dmguid(guidObject),
            dwIndex, debugstr_dmguid(iidInterface), ppObject);
	    
	switch (dwStage) {
        case DMUS_PATH_BUFFER:
          if (This->pDSBuffer)
          {
            if (IsEqualIID (iidInterface, &IID_IDirectSoundBuffer8)) {
              IDirectSoundBuffer8_QueryInterface (This->pDSBuffer, &IID_IDirectSoundBuffer8, ppObject);
              TRACE("returning %p\n",*ppObject);
              return S_OK;
            } else if (IsEqualIID (iidInterface, &IID_IDirectSound3DBuffer)) {
              IDirectSoundBuffer8_QueryInterface (This->pDSBuffer, &IID_IDirectSound3DBuffer, ppObject);
              TRACE("returning %p\n",*ppObject);
              return S_OK;
            } else {
              FIXME("Bad iid\n");
            }
          }
          break;

	case DMUS_PATH_PRIMARY_BUFFER: {
	  if (IsEqualIID (iidInterface, &IID_IDirectSound3DListener)) {
	    IDirectSoundBuffer8_QueryInterface (This->pPrimary, &IID_IDirectSound3DListener, ppObject);
	    return S_OK;
	  } else {
	    FIXME("bad iid...\n");
	  }
	}
	break;

	case DMUS_PATH_AUDIOPATH_GRAPH:
	  {
	    if (IsEqualIID (iidInterface, &IID_IDirectMusicGraph)) {
	      if (NULL == This->pToolGraph) {
		IDirectMusicGraphImpl* pGraph;
		hr = create_dmgraph(&IID_IDirectMusicGraph, (void**)&pGraph);
		if (FAILED(hr))
		  return hr;
		This->pToolGraph = (IDirectMusicGraph*) pGraph;
	      }
	      *ppObject = This->pToolGraph;
	      IDirectMusicGraph_AddRef((LPDIRECTMUSICGRAPH) *ppObject);
	      return S_OK;
	    }
	  }
	  break;

	case DMUS_PATH_AUDIOPATH_TOOL:
	  {
	    /* TODO */
	  }
	  break;

	case DMUS_PATH_PERFORMANCE:
	  {
	    /* TODO check wanted GUID */
	    *ppObject = This->pPerf;
	    IUnknown_AddRef((LPUNKNOWN) *ppObject);
	    return S_OK;
	  }
	  break;

	case DMUS_PATH_PERFORMANCE_GRAPH:
	  {
	    IDirectMusicGraph* pPerfoGraph = NULL; 
	    IDirectMusicPerformance8_GetGraph(This->pPerf, &pPerfoGraph);
	    if (NULL == pPerfoGraph) {
	      IDirectMusicGraphImpl* pGraph = NULL; 
	      hr = create_dmgraph(&IID_IDirectMusicGraph, (void**)&pGraph);
	      if (FAILED(hr))
		return hr;
	      IDirectMusicPerformance8_SetGraph(This->pPerf, (IDirectMusicGraph*) pGraph);
	      /* we need release as SetGraph do an AddRef */
	      IDirectMusicGraph_Release((LPDIRECTMUSICGRAPH) pGraph);
	      pPerfoGraph = (LPDIRECTMUSICGRAPH) pGraph;
	    }
	    *ppObject = pPerfoGraph;
	    return S_OK;
	  }
	  break;

	case DMUS_PATH_PERFORMANCE_TOOL:
	default:
	  break;
	}

	*ppObject = NULL;
	return E_INVALIDARG;
}

static HRESULT WINAPI IDirectMusicAudioPathImpl_Activate (IDirectMusicAudioPath *iface, BOOL fActivate)
{
  struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);
  FIXME("(%p, %d): stub\n", This, fActivate);
  if (!fActivate) {
    if (!This->fActive) return S_OK;
    This->fActive = FALSE;
  } else {
    if (This->fActive) return S_OK;
    This->fActive = TRUE;
    if (NULL != This->pDSBuffer) {
      IDirectSoundBuffer_Stop(This->pDSBuffer);
    }
  }
  return S_OK;
}

static HRESULT WINAPI IDirectMusicAudioPathImpl_SetVolume (IDirectMusicAudioPath *iface, LONG lVolume, DWORD dwDuration)
{
  struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);
  FIXME("(%p, %i, %d): stub\n", This, lVolume, dwDuration);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicAudioPathImpl_ConvertPChannel (IDirectMusicAudioPath *iface, DWORD dwPChannelIn, DWORD* pdwPChannelOut)
{
  struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);
  FIXME("(%p, %d, %p): stub\n", This, dwPChannelIn, pdwPChannelOut);
  return S_OK;
}

static const IDirectMusicAudioPathVtbl DirectMusicAudioPathVtbl = {
  IDirectMusicAudioPathImpl_QueryInterface,
  IDirectMusicAudioPathImpl_AddRef,
  IDirectMusicAudioPathImpl_Release,
  IDirectMusicAudioPathImpl_GetObjectInPath,
  IDirectMusicAudioPathImpl_Activate,
  IDirectMusicAudioPathImpl_SetVolume,
  IDirectMusicAudioPathImpl_ConvertPChannel
};

/* IDirectMusicObject */
static HRESULT WINAPI DirectMusicObject_ParseDescriptor(IDirectMusicObject *iface, IStream *pStream, DMUS_OBJECTDESC *pDesc)
{
	struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicObject(iface);
	DMUS_PRIVATE_CHUNK Chunk;
	DWORD StreamSize, StreamCount, ListSize[1], ListCount[1];
	LARGE_INTEGER liMove; /* used when skipping chunks */

	TRACE("(%p, %p, %p)\n", This, pStream, pDesc);

	/* FIXME: should this be determined from stream? */
	pDesc->dwValidData |= DMUS_OBJ_CLASS;
	pDesc->guidClass = This->pDesc->guidClass;

	IStream_Read (pStream, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
	TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
	switch (Chunk.fccID) {	
		case FOURCC_RIFF: {
			IStream_Read (pStream, &Chunk.fccID, sizeof(FOURCC), NULL);				
			TRACE_(dmfile)(": RIFF chunk of type %s", debugstr_fourcc(Chunk.fccID));
			StreamSize = Chunk.dwSize - sizeof(FOURCC);
			StreamCount = 0;
			if (Chunk.fccID == DMUS_FOURCC_AUDIOPATH_FORM) {
				TRACE_(dmfile)(": audio path form\n");
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

static const IDirectMusicObjectVtbl dmobject_vtbl = {
    dmobj_IDirectMusicObject_QueryInterface,
    dmobj_IDirectMusicObject_AddRef,
    dmobj_IDirectMusicObject_Release,
    dmobj_IDirectMusicObject_GetDescriptor,
    dmobj_IDirectMusicObject_SetDescriptor,
    DirectMusicObject_ParseDescriptor
};

/* IPersistStream */
static HRESULT WINAPI PersistStream_Load(IPersistStream *iface, IStream *pStm)
{
	struct IDirectMusicAudioPathImpl *This = impl_from_IPersistStream(iface);

	FOURCC chunkID;
	DWORD chunkSize, StreamSize, StreamCount, ListSize[3], ListCount[3];
	LARGE_INTEGER liMove; /* used when skipping chunks */

	FIXME("(%p, %p): Loading not implemented yet\n", This, pStm);
	IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
	IStream_Read (pStm, &chunkSize, sizeof(DWORD), NULL);
	TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (chunkID), chunkSize);
	switch (chunkID) {	
		case FOURCC_RIFF: {
			IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);				
			TRACE_(dmfile)(": RIFF chunk of type %s", debugstr_fourcc(chunkID));
			StreamSize = chunkSize - sizeof(FOURCC);
			StreamCount = 0;
			switch (chunkID) {
				case DMUS_FOURCC_AUDIOPATH_FORM: {
					TRACE_(dmfile)(": audio path form\n");
					do {
						IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
						IStream_Read (pStm, &chunkSize, sizeof(FOURCC), NULL);
						StreamCount += sizeof(FOURCC) + sizeof(DWORD) + chunkSize;
						TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (chunkID), chunkSize);
						switch (chunkID) {
							case DMUS_FOURCC_GUID_CHUNK: {
								TRACE_(dmfile)(": GUID chunk\n");
								This->pDesc->dwValidData |= DMUS_OBJ_OBJECT;
								IStream_Read (pStm, &This->pDesc->guidObject, chunkSize, NULL);
								break;
							}
							case DMUS_FOURCC_VERSION_CHUNK: {
								TRACE_(dmfile)(": version chunk\n");
								This->pDesc->dwValidData |= DMUS_OBJ_VERSION;
								IStream_Read (pStm, &This->pDesc->vVersion, chunkSize, NULL);
								break;
							}
							case DMUS_FOURCC_CATEGORY_CHUNK: {
								TRACE_(dmfile)(": category chunk\n");
								This->pDesc->dwValidData |= DMUS_OBJ_CATEGORY;
								IStream_Read (pStm, This->pDesc->wszCategory, chunkSize, NULL);
								break;
							}
							case FOURCC_LIST: {
								IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);				
								TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(chunkID));
								ListSize[0] = chunkSize - sizeof(FOURCC);
								ListCount[0] = 0;
								switch (chunkID) {
									case DMUS_FOURCC_UNFO_LIST: {
										TRACE_(dmfile)(": UNFO list\n");
										do {
											IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
											IStream_Read (pStm, &chunkSize, sizeof(FOURCC), NULL);
											ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + chunkSize;
											TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (chunkID), chunkSize);
											switch (chunkID) {
												/* don't ask me why, but M$ puts INFO elements in UNFO list sometimes
                                              (though strings seem to be valid unicode) */
												case mmioFOURCC('I','N','A','M'):
												case DMUS_FOURCC_UNAM_CHUNK: {
													TRACE_(dmfile)(": name chunk\n");
													This->pDesc->dwValidData |= DMUS_OBJ_NAME;
													IStream_Read (pStm, This->pDesc->wszName, chunkSize, NULL);
													break;
												}
												case mmioFOURCC('I','A','R','T'):
												case DMUS_FOURCC_UART_CHUNK: {
													TRACE_(dmfile)(": artist chunk (ignored)\n");
													liMove.QuadPart = chunkSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												case mmioFOURCC('I','C','O','P'):
												case DMUS_FOURCC_UCOP_CHUNK: {
													TRACE_(dmfile)(": copyright chunk (ignored)\n");
													liMove.QuadPart = chunkSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												case mmioFOURCC('I','S','B','J'):
												case DMUS_FOURCC_USBJ_CHUNK: {
													TRACE_(dmfile)(": subject chunk (ignored)\n");
													liMove.QuadPart = chunkSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												case mmioFOURCC('I','C','M','T'):
												case DMUS_FOURCC_UCMT_CHUNK: {
													TRACE_(dmfile)(": comment chunk (ignored)\n");
													liMove.QuadPart = chunkSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												default: {
													TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
													liMove.QuadPart = chunkSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;						
												}
											}
											TRACE_(dmfile)(": ListCount[0] = %d < ListSize[0] = %d\n", ListCount[0], ListSize[0]);
										} while (ListCount[0] < ListSize[0]);
										break;
									}
									default: {
										TRACE_(dmfile)(": unknown (skipping)\n");
										liMove.QuadPart = chunkSize - sizeof(FOURCC);
										IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
										break;						
									}
								}
								break;
							}	
							default: {
								TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
								liMove.QuadPart = chunkSize;
								IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
								break;						
							}
						}
						TRACE_(dmfile)(": StreamCount[0] = %d < StreamSize[0] = %d\n", StreamCount, StreamSize);
					} while (StreamCount < StreamSize);
					break;
				}
				default: {
					TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
					liMove.QuadPart = StreamSize;
					IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
					return E_FAIL;
				}
			}
			TRACE_(dmfile)(": reading finished\n");
			break;
		}
		default: {
			TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
			liMove.QuadPart = chunkSize;
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
    PersistStream_Load,
    unimpl_IPersistStream_Save,
    unimpl_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT WINAPI create_dmaudiopath(REFIID riid, void **ppobj)
{
    IDirectMusicAudioPathImpl* obj;
    HRESULT hr;

    obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicAudioPathImpl));
    if (NULL == obj) {
        *ppobj = NULL;
	return E_OUTOFMEMORY;
    }
    obj->IDirectMusicAudioPath_iface.lpVtbl = &DirectMusicAudioPathVtbl;
    obj->ref = 1;
    dmobject_init(&obj->dmobj, &CLSID_DirectMusicAudioPathConfig,
            (IUnknown *)&obj->IDirectMusicAudioPath_iface);
    obj->dmobj.IDirectMusicObject_iface.lpVtbl = &dmobject_vtbl;
    obj->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;
    obj->pDesc = &obj->dmobj.desc;

    hr = IDirectMusicAudioPath_QueryInterface(&obj->IDirectMusicAudioPath_iface, riid, ppobj);
    IDirectMusicAudioPath_Release(&obj->IDirectMusicAudioPath_iface);
    return hr;
}
