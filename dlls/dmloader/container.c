/* IDirectMusicContainer
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "dmloader_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmloader);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

/*****************************************************************************
 * IDirectMusicContainerImpl implementation
 */
/* IDirectMusicContainerImpl IUnknown part: */
HRESULT WINAPI IDirectMusicContainerImpl_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicContainerImpl, UnknownVtbl, iface);
	
	TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppobj);
	if (IsEqualIID (riid, &IID_IUnknown)) {
		*ppobj = (LPVOID)&This->UnknownVtbl;
		IDirectMusicContainerImpl_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
		return S_OK;	
	} else if (IsEqualIID (riid, &IID_IDirectMusicContainer)) {
		*ppobj = (LPVOID)&This->ContainerVtbl;
		IDirectMusicContainerImpl_IDirectMusicContainer_AddRef ((LPDIRECTMUSICCONTAINER)&This->ContainerVtbl);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IDirectMusicObject)) {
		*ppobj = (LPVOID)&This->ObjectVtbl;
		IDirectMusicContainerImpl_IDirectMusicObject_AddRef ((LPDIRECTMUSICOBJECT)&This->ObjectVtbl);		
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IPersistStream)) {
		*ppobj = (LPVOID)&This->PersistStreamVtbl;
		IDirectMusicContainerImpl_IPersistStream_AddRef ((LPPERSISTSTREAM)&This->PersistStreamVtbl);		
		return S_OK;
	}
	
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicContainerImpl_IUnknown_AddRef (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicContainerImpl, UnknownVtbl, iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicContainerImpl_IUnknown_Release (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicContainerImpl, UnknownVtbl, iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

ICOM_VTABLE(IUnknown) DirectMusicContainer_Unknown_Vtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicContainerImpl_IUnknown_QueryInterface,
	IDirectMusicContainerImpl_IUnknown_AddRef,
	IDirectMusicContainerImpl_IUnknown_Release
};

/* IDirectMusicContainer Interface follow: */
HRESULT WINAPI IDirectMusicContainerImpl_IDirectMusicContainer_QueryInterface (LPDIRECTMUSICCONTAINER iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicContainerImpl, ContainerVtbl, iface);
	return IDirectMusicContainerImpl_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

ULONG WINAPI IDirectMusicContainerImpl_IDirectMusicContainer_AddRef (LPDIRECTMUSICCONTAINER iface) {
	ICOM_THIS_MULTI(IDirectMusicContainerImpl, ContainerVtbl, iface);
	return IDirectMusicContainerImpl_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

ULONG WINAPI IDirectMusicContainerImpl_IDirectMusicContainer_Release (LPDIRECTMUSICCONTAINER iface) {
	ICOM_THIS_MULTI(IDirectMusicContainerImpl, ContainerVtbl, iface);
	return IDirectMusicContainerImpl_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

HRESULT WINAPI IDirectMusicContainerImpl_IDirectMusicContainer_EnumObject (LPDIRECTMUSICCONTAINER iface, REFGUID rguidClass, DWORD dwIndex, LPDMUS_OBJECTDESC pDesc, WCHAR* pwszAlias) {
	ICOM_THIS_MULTI(IDirectMusicContainerImpl, ContainerVtbl, iface);
	DWORD i = -1; /* index ;) ... must be -1 since dwIndex can be 0 */
	struct list *listEntry;
	LPDMUS_PRIVATE_CONTAINED_OBJECT_ENTRY objectEntry;

	TRACE("(%p, %s, %ld, %p, %p)\n", This, debugstr_guid(rguidClass), dwIndex, pDesc, pwszAlias);
	LIST_FOR_EACH (listEntry, &This->ObjectsList) {
		objectEntry = LIST_ENTRY(listEntry, DMUS_PRIVATE_CONTAINED_OBJECT_ENTRY, entry);	
		if (IsEqualGUID(rguidClass, &GUID_DirectMusicAllTypes)) i++;
		else if (IsEqualGUID(rguidClass, &objectEntry->pDesc->guidClass)) i++;

		if (i == dwIndex) {
			if (pDesc)
				memcpy (pDesc, objectEntry->pDesc, sizeof(DMUS_OBJECTDESC));
			if (pwszAlias && objectEntry->wszAlias) {
				strncpyW (pwszAlias, objectEntry->wszAlias, DMUS_MAX_NAME);
				if (strlenW (objectEntry->wszAlias) > DMUS_MAX_NAME)
					return DMUS_S_STRING_TRUNCATED;
			}
			
			return S_OK;
		}
	}
	
	return S_FALSE;
}

ICOM_VTABLE(IDirectMusicContainer) DirectMusicContainer_Container_Vtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicContainerImpl_IDirectMusicContainer_QueryInterface,
	IDirectMusicContainerImpl_IDirectMusicContainer_AddRef,
	IDirectMusicContainerImpl_IDirectMusicContainer_Release,
	IDirectMusicContainerImpl_IDirectMusicContainer_EnumObject
};

/* IDirectMusicContainerImpl IDirectMusicObject part: */
HRESULT WINAPI IDirectMusicContainerImpl_IDirectMusicObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicContainerImpl, ObjectVtbl, iface);
	return IDirectMusicContainerImpl_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

ULONG WINAPI IDirectMusicContainerImpl_IDirectMusicObject_AddRef (LPDIRECTMUSICOBJECT iface) {
	ICOM_THIS_MULTI(IDirectMusicContainerImpl, ObjectVtbl, iface);
	return IDirectMusicContainerImpl_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

ULONG WINAPI IDirectMusicContainerImpl_IDirectMusicObject_Release (LPDIRECTMUSICOBJECT iface) {
	ICOM_THIS_MULTI(IDirectMusicContainerImpl, ObjectVtbl, iface);
	return IDirectMusicContainerImpl_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

HRESULT WINAPI IDirectMusicContainerImpl_IDirectMusicObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc) {
	ICOM_THIS_MULTI(IDirectMusicContainerImpl, ObjectVtbl, iface);
	TRACE("(%p, %p)\n", This, pDesc);
	/* I think we shouldn't return pointer here since then values can be changed; it'd be a mess */
	memcpy (pDesc, This->pDesc, This->pDesc->dwSize);
	return S_OK;
}

HRESULT WINAPI IDirectMusicContainerImpl_IDirectMusicObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc) {
	ICOM_THIS_MULTI(IDirectMusicContainerImpl, ObjectVtbl, iface);
	TRACE("(%p, %p): setting descriptor:\n", This, pDesc);
	if (TRACE_ON(dmloader)) {
		DMUSIC_dump_DMUS_OBJECTDESC (pDesc);
	}
	
	/* According to MSDN, we should copy only given values, not whole struct */	
	if (pDesc->dwValidData & DMUS_OBJ_OBJECT)
		memcpy (&This->pDesc->guidObject, &pDesc->guidObject, sizeof (pDesc->guidObject));
	if (pDesc->dwValidData & DMUS_OBJ_CLASS)
		memcpy (&This->pDesc->guidClass, &pDesc->guidClass, sizeof (pDesc->guidClass));		
	if (pDesc->dwValidData & DMUS_OBJ_NAME)
		strncpyW (This->pDesc->wszName, pDesc->wszName, DMUS_MAX_NAME);
	if (pDesc->dwValidData & DMUS_OBJ_CATEGORY)
		strncpyW (This->pDesc->wszCategory, pDesc->wszCategory, DMUS_MAX_CATEGORY);		
	if (pDesc->dwValidData & DMUS_OBJ_FILENAME)
		strncpyW (This->pDesc->wszFileName, pDesc->wszFileName, DMUS_MAX_FILENAME);		
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

HRESULT WINAPI IDirectMusicContainerImpl_IDirectMusicObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc) {
	ICOM_THIS_MULTI(IDirectMusicContainerImpl, ObjectVtbl, iface);
	DMUS_PRIVATE_CHUNK Chunk;
	DWORD StreamSize, StreamCount, ListSize[1], ListCount[1];
	LARGE_INTEGER liMove; /* used when skipping chunks */

	TRACE("(%p, %p, %p)\n", This, pStream, pDesc);
	
	/* FIXME: should this be determined from stream? */
	pDesc->dwValidData |= DMUS_OBJ_CLASS;
	memcpy (&pDesc->guidClass, &CLSID_DirectMusicContainer, sizeof(CLSID));
	
	IStream_Read (pStream, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
	TRACE_(dmfile)(": %s chunk (size = 0x%04lx)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
	switch (Chunk.fccID) {	
		case FOURCC_RIFF: {
			IStream_Read (pStream, &Chunk.fccID, sizeof(FOURCC), NULL);				
			TRACE_(dmfile)(": RIFF chunk of type %s", debugstr_fourcc(Chunk.fccID));
			StreamSize = Chunk.dwSize - sizeof(FOURCC);
			StreamCount = 0;
			if (Chunk.fccID == DMUS_FOURCC_CONTAINER_FORM) {
				TRACE_(dmfile)(": container form\n");
				do {
					IStream_Read (pStream, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
					StreamCount += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
					TRACE_(dmfile)(": %s chunk (size = 0x%04lx)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
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
										TRACE_(dmfile)(": %s chunk (size = 0x%04lx)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
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
										TRACE_(dmfile)(": ListCount[0] = %ld < ListSize[0] = %ld\n", ListCount[0], ListSize[0]);
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
					TRACE_(dmfile)(": StreamCount[0] = %ld < StreamSize[0] = %ld\n", StreamCount, StreamSize);
				} while (StreamCount < StreamSize);
				break;
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
	
	TRACE(": returning descriptor:\n");
	if (TRACE_ON(dmloader)) {
		DMUSIC_dump_DMUS_OBJECTDESC (pDesc);
	}
	
	return S_OK;	
}

ICOM_VTABLE(IDirectMusicObject) DirectMusicContainer_Object_Vtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicContainerImpl_IDirectMusicObject_QueryInterface,
	IDirectMusicContainerImpl_IDirectMusicObject_AddRef,
	IDirectMusicContainerImpl_IDirectMusicObject_Release,
	IDirectMusicContainerImpl_IDirectMusicObject_GetDescriptor,
	IDirectMusicContainerImpl_IDirectMusicObject_SetDescriptor,
	IDirectMusicContainerImpl_IDirectMusicObject_ParseDescriptor
};

/* IDirectMusicContainerImpl IPersistStream part: */
HRESULT WINAPI IDirectMusicContainerImpl_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicContainerImpl, PersistStreamVtbl, iface);
	return IDirectMusicContainerImpl_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

ULONG WINAPI IDirectMusicContainerImpl_IPersistStream_AddRef (LPPERSISTSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicContainerImpl, PersistStreamVtbl, iface);
	return IDirectMusicContainerImpl_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

ULONG WINAPI IDirectMusicContainerImpl_IPersistStream_Release (LPPERSISTSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicContainerImpl, PersistStreamVtbl, iface);
	return IDirectMusicContainerImpl_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

HRESULT WINAPI IDirectMusicContainerImpl_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID) {
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicContainerImpl_IPersistStream_IsDirty (LPPERSISTSTREAM iface) {
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicContainerImpl_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm) {
	ICOM_THIS_MULTI(IDirectMusicContainerImpl, PersistStreamVtbl, iface);

	DMUS_PRIVATE_CHUNK Chunk;
	DWORD StreamSize, StreamCount, ListSize[3], ListCount[3];
	LARGE_INTEGER liMove; /* used when skipping chunks */
	ULARGE_INTEGER uliPos; /* needed when dealing with RIFF chunks */
	LPDIRECTMUSICGETLOADER pGetLoader;
	LPDIRECTMUSICLOADER pLoader;
	
	/* get loader since it will be needed later */
	IStream_QueryInterface (pStm, &IID_IDirectMusicGetLoader, (LPVOID*)&pGetLoader);
	IDirectMusicGetLoader_GetLoader (pGetLoader, &pLoader);
	IDirectMusicGetLoader_Release (pGetLoader);
	
	IStream_AddRef (pStm); /* add count for later references */

	IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
	TRACE_(dmfile)(": %s chunk (size = 0x%04lx)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
	switch (Chunk.fccID) {	
		case FOURCC_RIFF: {
			IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);				
			TRACE_(dmfile)(": RIFF chunk of type %s", debugstr_fourcc(Chunk.fccID));
			StreamSize = Chunk.dwSize - sizeof(FOURCC);
			StreamCount = 0;
			switch (Chunk.fccID) {
				case DMUS_FOURCC_CONTAINER_FORM: {
					TRACE_(dmfile)(": container form\n");
					do {
						IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
						StreamCount += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
						TRACE_(dmfile)(": %s chunk (size = 0x%04lx)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
						switch (Chunk.fccID) {
							case DMUS_FOURCC_CONTAINER_CHUNK: {
								TRACE_(dmfile)(": container header chunk\n");
								This->pHeader = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, Chunk.dwSize);
								IStream_Read (pStm, This->pHeader, Chunk.dwSize, NULL);
								break;	
							}
							case DMUS_FOURCC_GUID_CHUNK: {
								TRACE_(dmfile)(": GUID chunk\n");
								This->pDesc->dwValidData |= DMUS_OBJ_OBJECT;
								IStream_Read (pStm, &This->pDesc->guidObject, Chunk.dwSize, NULL);
								break;
							}
							case DMUS_FOURCC_VERSION_CHUNK: {
								TRACE_(dmfile)(": version chunk\n");
								This->pDesc->dwValidData |= DMUS_OBJ_VERSION;
								IStream_Read (pStm, &This->pDesc->vVersion, Chunk.dwSize, NULL);
								break;
							}
							case DMUS_FOURCC_CATEGORY_CHUNK: {
								TRACE_(dmfile)(": category chunk\n");
								This->pDesc->dwValidData |= DMUS_OBJ_CATEGORY;
								IStream_Read (pStm, This->pDesc->wszCategory, Chunk.dwSize, NULL);
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
											switch (Chunk.fccID) {
												/* don't ask me why, but M$ puts INFO elements in UNFO list sometimes
                                              (though strings seem to be valid unicode) */
												case mmioFOURCC('I','N','A','M'):
												case DMUS_FOURCC_UNAM_CHUNK: {
													TRACE_(dmfile)(": name chunk\n");
													This->pDesc->dwValidData |= DMUS_OBJ_NAME;
													IStream_Read (pStm, This->pDesc->wszName, Chunk.dwSize, NULL);
													break;
												}
												case mmioFOURCC('I','A','R','T'):
												case DMUS_FOURCC_UART_CHUNK: {
													TRACE_(dmfile)(": artist chunk (ignored)\n");
													liMove.QuadPart = Chunk.dwSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												case mmioFOURCC('I','C','O','P'):
												case DMUS_FOURCC_UCOP_CHUNK: {
													TRACE_(dmfile)(": copyright chunk (ignored)\n");
													liMove.QuadPart = Chunk.dwSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												case mmioFOURCC('I','S','B','J'):
												case DMUS_FOURCC_USBJ_CHUNK: {
													TRACE_(dmfile)(": subject chunk (ignored)\n");
													liMove.QuadPart = Chunk.dwSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												case mmioFOURCC('I','C','M','T'):
												case DMUS_FOURCC_UCMT_CHUNK: {
													TRACE_(dmfile)(": comment chunk (ignored)\n");
													liMove.QuadPart = Chunk.dwSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												default: {
													TRACE_(dmfile)(": unknown chunk (irrevelant & skipping)\n");
													liMove.QuadPart = Chunk.dwSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;						
												}
											}
											TRACE_(dmfile)(": ListCount[0] = %ld < ListSize[0] = %ld\n", ListCount[0], ListSize[0]);
										} while (ListCount[0] < ListSize[0]);
										break;
									}
									case DMUS_FOURCC_CONTAINED_OBJECTS_LIST: {
										TRACE_(dmfile)(": contained objects list\n");
										do {
											IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
											ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
											TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
											switch (Chunk.fccID) {
												case FOURCC_LIST: {
													IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);				
													TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(Chunk.fccID));
													ListSize[1] = Chunk.dwSize - sizeof(FOURCC);
													ListCount[1] = 0;
													switch (Chunk.fccID) {
														case DMUS_FOURCC_CONTAINED_OBJECT_LIST: {
															DMUS_IO_CONTAINED_OBJECT_HEADER tmpObjectHeader; /* temporary structure */
															LPDMUS_PRIVATE_CONTAINED_OBJECT_ENTRY newEntry;
															TRACE_(dmfile)(": contained object list\n");
															memset (&tmpObjectHeader, 0, sizeof(DMUS_IO_CONTAINED_OBJECT_HEADER));
															newEntry = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(DMUS_PRIVATE_CONTAINED_OBJECT_ENTRY));
															newEntry->pDesc = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(DMUS_OBJECTDESC));
															DM_STRUCT_INIT(newEntry->pDesc);
															do {
																IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
																ListCount[1] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
																TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
																switch (Chunk.fccID) {
																	case DMUS_FOURCC_CONTAINED_ALIAS_CHUNK: {
																		TRACE_(dmfile)(": alias chunk\n");
																		newEntry->wszAlias = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, Chunk.dwSize);
																		IStream_Read (pStm, newEntry->wszAlias, Chunk.dwSize, NULL);
																		break;
																	}
																	case DMUS_FOURCC_CONTAINED_OBJECT_CHUNK: {
																		TRACE_(dmfile)(": contained object header chunk\n");
																		IStream_Read (pStm, &tmpObjectHeader, Chunk.dwSize, NULL);
																		/* copy guidClass */
																		newEntry->pDesc->dwValidData |= DMUS_OBJ_CLASS;
																		memcpy (&newEntry->pDesc->guidClass, &tmpObjectHeader.guidClassID, sizeof(GUID));
																		break;
																	}
																	/* now read data... it may be safe to read everything after object header chunk, 
																		but I'm not comfortable with MSDN's "the header is *normally* followed by ..." */
																	case FOURCC_LIST: {
																		IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);				
																		TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(Chunk.fccID));
																		ListSize[2] = Chunk.dwSize - sizeof(FOURCC);
																		ListCount[2] = 0;
																		switch (Chunk.fccID) {
																			case DMUS_FOURCC_REF_LIST: {
																				DMUS_IO_REFERENCE tmpReferenceHeader; /* temporary structure */
																				TRACE_(dmfile)(": reference list\n");
																				memset (&tmpReferenceHeader, 0, sizeof(DMUS_IO_REFERENCE));
																				do {
																					IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
																					ListCount[2] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
																					TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
																					switch (Chunk.fccID) {
																						case DMUS_FOURCC_REF_CHUNK: {
																							TRACE_(dmfile)(": reference header chunk\n");
																							IStream_Read (pStm, &tmpReferenceHeader, Chunk.dwSize, NULL);
																							/* copy retrieved data to DMUS_OBJECTDESC */
																							if (!IsEqualCLSID (&newEntry->pDesc->guidClass, &tmpReferenceHeader.guidClassID)) ERR(": object header declares different CLSID than reference header\n");
																							/* no need since it's already there */
																							/*memcpy (&newEntry->pDesc->guidClass, &tempReferenceHeader.guidClassID, sizeof(GUID)); */
																							newEntry->pDesc->dwValidData = tmpReferenceHeader.dwValidData;
																							break;																	
																						}
																						case DMUS_FOURCC_GUID_CHUNK: {
																							TRACE_(dmfile)(": guid chunk\n");
																							/* no need to set flags since they were copied from reference header */
																							IStream_Read (pStm, &newEntry->pDesc->guidObject, Chunk.dwSize, NULL);
																							break;
																						}
																						case DMUS_FOURCC_DATE_CHUNK: {
																							TRACE_(dmfile)(": file date chunk\n");
																							/* no need to set flags since they were copied from reference header */
																							IStream_Read (pStm, &newEntry->pDesc->ftDate, Chunk.dwSize, NULL);
																							break;
																						}
																						case DMUS_FOURCC_NAME_CHUNK: {
																							TRACE_(dmfile)(": name chunk\n");
																							/* no need to set flags since they were copied from reference header */
																							IStream_Read (pStm, newEntry->pDesc->wszName, Chunk.dwSize, NULL);
																							break;
																						}
																						case DMUS_FOURCC_FILE_CHUNK: {
																							TRACE_(dmfile)(": file name chunk\n");
																							/* no need to set flags since they were copied from reference header */
																							IStream_Read (pStm, newEntry->pDesc->wszFileName, Chunk.dwSize, NULL);
																							break;
																						}
																						case DMUS_FOURCC_CATEGORY_CHUNK: {
																							TRACE_(dmfile)(": category chunk\n");
																							/* no need to set flags since they were copied from reference header */
																							IStream_Read (pStm, newEntry->pDesc->wszCategory, Chunk.dwSize, NULL);
																							break;
																						}
																						case DMUS_FOURCC_VERSION_CHUNK: {
																							TRACE_(dmfile)(": version chunk\n");
																							/* no need to set flags since they were copied from reference header */
																							IStream_Read (pStm, &newEntry->pDesc->vVersion, Chunk.dwSize, NULL);
																							break;
																						}
																						default: {
																							TRACE_(dmfile)(": unknown chunk (skipping)\n");
																							liMove.QuadPart = Chunk.dwSize;
																							IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL); /* skip this chunk */
																							break;
																						}
																					}
																					TRACE_(dmfile)(": ListCount[2] = %ld < ListSize[2] = %ld\n", ListCount[2], ListSize[2]);
																				} while (ListCount[2] < ListSize[2]);
																				break;
																			}
																			default: {
																				TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
																				return E_FAIL;
																			}
																		}
																		break;
																	}
																	case FOURCC_RIFF: {
																		IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);
																		TRACE_(dmfile)(": RIFF chunk of type %s", debugstr_fourcc(Chunk.fccID));
																		if (IS_VALID_DMFORM (Chunk.fccID)) {
																			TRACE_(dmfile)(": valid DMUSIC form\n");
																			/* we'll have to skip whole RIFF chunk after SetObject call */
																			#define RIFF_LOADING /* effective hack ;) */
																			liMove.QuadPart = 0;
																			IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, &uliPos);
																			uliPos.QuadPart += (Chunk.dwSize - sizeof(FOURCC)); /* set uliPos at the end of RIFF chunk */																			
																			/* move at the beginning of RIFF chunk */
																			liMove.QuadPart = 0;
																			liMove.QuadPart -= (sizeof(FOURCC)+sizeof(DWORD)+sizeof(FOURCC));
																			IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
																			/* put pointer to stream in descriptor */
																			newEntry->pDesc->dwValidData |= DMUS_OBJ_STREAM;
																			/* this is not how M$ does it (according to my tests), but 
																				who says their way is better? */
                                                                                                                                                        /* *newEntry->pDesc->pStream = pStm; */
                                                                                                                                                        /* *IStream_AddRef (pStm); */ /* reference increased */
																			IStream_Clone (pStm, &newEntry->pDesc->pStream);
																			/* wait till we get on the end of object list */
																		} else {
																			TRACE_(dmfile)(": invalid DMUSIC form (skipping)\n");
																			liMove.QuadPart = Chunk.dwSize - sizeof(FOURCC);
																			IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
																			/* FIXME: should we return E_FAIL? */
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
																TRACE_(dmfile)(": ListCount[1] = %ld < ListSize[1] = %ld\n", ListCount[1], ListSize[1]);
															} while (ListCount[1] < ListSize[1]);
															/* SetObject: this will fill descriptor with additional info
																and add alias in loader's cache */
															IDirectMusicLoader_SetObject (pLoader, newEntry->pDesc);
															/* my tests show tha we shouldn't return any info on stream when calling EnumObject... sigh... which
																means we have to clear these fields to be M$ compliant; but funny thing is, we return filename
																when loading from reference... M$ sux */
															/* FIXME: test what happens when we load with DMUS_OBJ_MEMORY */
															/* if we have loaded through RIFF chunk, skip it and clear stream flag */
															#ifdef RIFF_LOADING
																liMove.QuadPart = uliPos.QuadPart;
																IStream_Seek (pStm, liMove, STREAM_SEEK_SET, NULL);
																newEntry->pDesc->dwValidData &= ~DMUS_OBJ_STREAM; /* clear flag */
																newEntry->pDesc->pStream = NULL;
																#undef RIFF_LOADING
															#endif
															/* add entry to list of objects */
															list_add_tail (&This->ObjectsList, &newEntry->entry);
															
															/* now, if DMUS_CONTAINER_NOLOADS is not set, we are supposed to load contained objects;
																so when we call GetObject later, they'll already be in cache */
															if (!(This->pHeader->dwFlags & DMUS_CONTAINER_NOLOADS)) {
																IDirectMusicObject* pObject;
																TRACE_(dmfile)(": DMUS_CONTAINER_NOLOADS not set\n");
																/* native container and builtin loader show that we use IDirectMusicObject here */
																if (SUCCEEDED(IDirectMusicLoader_GetObject (pLoader, newEntry->pDesc, &IID_IDirectMusicObject, (LPVOID*)&pObject)))
																	IDirectMusicObject_Release (pObject);
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
											TRACE_(dmfile)(": ListCount[0] = %ld < ListSize[0] = %ld\n", ListCount[0], ListSize[0]);
										} while (ListCount[0] < ListSize[0]);
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
						TRACE_(dmfile)(": StreamCount[0] = %ld < StreamSize[0] = %ld\n", StreamCount, StreamSize);
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
			liMove.QuadPart = Chunk.dwSize;
			IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
			return E_FAIL;
		}
	}

	IDirectMusicLoader_Release (pLoader); /* release loader */
	
#if 0	
	/* DEBUG: dumps whole container object tree: */
	if (TRACE_ON(dmloader)) {
		int r = 0;
		DMUS_PRIVATE_CONTAINED_OBJECT_ENTRY *tmpEntry;
		struct list *listEntry;

		TRACE("*** IDirectMusicContainer (%p) ***\n", This->ContainerVtbl);
		TRACE(" - Object descriptor:\n");
		DMUSIC_dump_DMUS_OBJECTDESC (This->pDesc);
		TRACE(" - Header:\n");
		TRACE("    - dwFlags: ");
		DMUSIC_dump_DMUS_CONTAINER_FLAGS (This->pHeader->dwFlags);

		TRACE(" - Objects:\n");
		
		LIST_FOR_EACH (listEntry, &This->ObjectsList) {
			tmpEntry = LIST_ENTRY( listEntry, DMUS_PRIVATE_CONTAINED_OBJECT_ENTRY, entry );
			TRACE("    - Object[%i]:\n", r);
			TRACE("       - wszAlias: %s\n", debugstr_w(tmpEntry->wszAlias));
			TRACE("       - Object descriptor:\n");
			DMUSIC_dump_DMUS_OBJECTDESC(tmpEntry->pDesc);
			r++;
		}
	}
#endif
	
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicContainerImpl_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty) {
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicContainerImpl_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize) {
	return E_NOTIMPL;
}

ICOM_VTABLE(IPersistStream) DirectMusicContainer_PersistStream_Vtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicContainerImpl_IPersistStream_QueryInterface,
	IDirectMusicContainerImpl_IPersistStream_AddRef,
	IDirectMusicContainerImpl_IPersistStream_Release,
	IDirectMusicContainerImpl_IPersistStream_GetClassID,
	IDirectMusicContainerImpl_IPersistStream_IsDirty,
	IDirectMusicContainerImpl_IPersistStream_Load,
	IDirectMusicContainerImpl_IPersistStream_Save,
	IDirectMusicContainerImpl_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicContainerImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter) {
	IDirectMusicContainerImpl* obj;
	
	obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicContainerImpl));
	if (NULL == obj) {
		*ppobj = (LPVOID) NULL;
		return E_OUTOFMEMORY;
	}
	obj->UnknownVtbl = &DirectMusicContainer_Unknown_Vtbl;
	obj->ContainerVtbl = &DirectMusicContainer_Container_Vtbl;
	obj->ObjectVtbl = &DirectMusicContainer_Object_Vtbl;
	obj->PersistStreamVtbl = &DirectMusicContainer_PersistStream_Vtbl;
	obj->pDesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DMUS_OBJECTDESC));
	DM_STRUCT_INIT(obj->pDesc);
	obj->pDesc->dwValidData |= DMUS_OBJ_CLASS;
	memcpy (&obj->pDesc->guidClass, &CLSID_DirectMusicContainer, sizeof (CLSID));
	obj->ref = 0; /* will be inited by QueryInterface */
	list_init (&obj->ObjectsList);
	
	return IDirectMusicContainerImpl_IUnknown_QueryInterface ((LPUNKNOWN)&obj->UnknownVtbl, lpcGUID, ppobj);
}
