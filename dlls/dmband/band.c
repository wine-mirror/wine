/* IDirectMusicBand Implementation
 *
 * Copyright (C) 2003 Rok Mandeljc
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "dmband_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmband);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

/*****************************************************************************
 * IDirectMusicBandImpl implementation
 */
/* IDirectMusicBand IUnknown part: */
HRESULT WINAPI IDirectMusicBandImpl_QueryInterface (LPDIRECTMUSICBAND iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicBandImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || 
	    IsEqualGUID(riid, &IID_IDirectMusicBand)) {
		IDirectMusicBandImpl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicBandImpl_AddRef (LPDIRECTMUSICBAND iface)
{
	ICOM_THIS(IDirectMusicBandImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicBandImpl_Release (LPDIRECTMUSICBAND iface)
{
	ICOM_THIS(IDirectMusicBandImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicBand IDirectMusicBand part: */
HRESULT WINAPI IDirectMusicBandImpl_CreateSegment (LPDIRECTMUSICBAND iface, IDirectMusicSegment** ppSegment)
{
	ICOM_THIS(IDirectMusicBandImpl,iface);

	FIXME("(%p, %p): stub\n", This, ppSegment);

	return S_OK;
}

HRESULT WINAPI IDirectMusicBandImpl_Download (LPDIRECTMUSICBAND iface, IDirectMusicPerformance* pPerformance)
{
	ICOM_THIS(IDirectMusicBandImpl,iface);

	FIXME("(%p, %p): stub\n", This, pPerformance);

	return S_OK;
}

HRESULT WINAPI IDirectMusicBandImpl_Unload (LPDIRECTMUSICBAND iface, IDirectMusicPerformance* pPerformance)
{
	ICOM_THIS(IDirectMusicBandImpl,iface);

	FIXME("(%p, %p): stub\n", This, pPerformance);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicBand) DirectMusicBand_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicBandImpl_QueryInterface,
	IDirectMusicBandImpl_AddRef,
	IDirectMusicBandImpl_Release,
	IDirectMusicBandImpl_CreateSegment,
	IDirectMusicBandImpl_Download,
	IDirectMusicBandImpl_Unload
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicBand (LPCGUID lpcGUID, LPDIRECTMUSICBAND* ppDMBand, LPUNKNOWN pUnkOuter)
{
	IDirectMusicBandImpl* dmband;
	
	if (IsEqualGUID (lpcGUID, &IID_IDirectMusicBand)) {
		dmband = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicBandImpl));
		if (NULL == dmband) {
			*ppDMBand = (LPDIRECTMUSICBAND) NULL;
			return E_OUTOFMEMORY;
		}
		dmband->lpVtbl = &DirectMusicBand_Vtbl;
		dmband->ref = 1;
		list_init (&dmband->Instruments);
		*ppDMBand = (LPDIRECTMUSICBAND) dmband;
		return S_OK;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;
}

/*****************************************************************************
 * IDirectMusicBandObject implementation
 */
/* IDirectMusicBandObject IUnknown part: */
HRESULT WINAPI IDirectMusicBandObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicBandObject,iface);

	if (IsEqualGUID (riid, &IID_IUnknown) 
		|| IsEqualGUID(riid, &IID_IDirectMusicObject)) {
		IDirectMusicBandObject_AddRef(iface);
		*ppobj = This;
		return S_OK;
	} else if (IsEqualGUID (riid, &IID_IPersistStream)) {
		IPersistStream_AddRef ((LPPERSISTSTREAM)This->pStream);
		*ppobj = (LPPERSISTSTREAM)This->pStream;
		return S_OK;
	} else if (IsEqualGUID (riid, &IID_IDirectMusicBand)) {
		IDirectMusicBand_AddRef ((LPDIRECTMUSICBAND)This->pBand);
		*ppobj = (LPDIRECTMUSICBAND)This->pBand;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicBandObject_AddRef (LPDIRECTMUSICOBJECT iface)
{
	ICOM_THIS(IDirectMusicBandObject,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicBandObject_Release (LPDIRECTMUSICOBJECT iface)
{
	ICOM_THIS(IDirectMusicBandObject,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicBandObject IDirectMusicObject part: */
HRESULT WINAPI IDirectMusicBandObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc)
{
	ICOM_THIS(IDirectMusicBandObject,iface);

	TRACE("(%p, %p)\n", This, pDesc);
	pDesc = This->pDesc;
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicBandObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc)
{
	ICOM_THIS(IDirectMusicBandObject,iface);

	TRACE("(%p, %p)\n", This, pDesc);
	This->pDesc = pDesc;

	return S_OK;
}

HRESULT WINAPI IDirectMusicBandObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc)
{
	ICOM_THIS(IDirectMusicBandObject,iface);

	FIXME("(%p, %p, %p): stub\n", This, pStream, pDesc);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicObject) DirectMusicBandObject_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicBandObject_QueryInterface,
	IDirectMusicBandObject_AddRef,
	IDirectMusicBandObject_Release,
	IDirectMusicBandObject_GetDescriptor,
	IDirectMusicBandObject_SetDescriptor,
	IDirectMusicBandObject_ParseDescriptor
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicBandObject (LPCGUID lpcGUID, LPDIRECTMUSICOBJECT* ppObject, LPUNKNOWN pUnkOuter)
{
	IDirectMusicBandObject *obj;
	
	TRACE("(%p,%p,%p)\n", lpcGUID, ppObject, pUnkOuter);
	if (IsEqualGUID (lpcGUID, &IID_IDirectMusicObject)) {
		obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicBandObject));
		if (NULL == obj) {
			*ppObject = (LPDIRECTMUSICOBJECT) NULL;
			return E_OUTOFMEMORY;
		}
		obj->lpVtbl = &DirectMusicBandObject_Vtbl;
		obj->ref = 1;
		/* prepare IPersistStream */
		obj->pStream = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(IDirectMusicBandObjectStream));
		obj->pStream->lpVtbl = &DirectMusicBandObjectStream_Vtbl;
		obj->pStream->ref = 1;	
		obj->pStream->pParentObject = obj;
		/* prepare IDirectMusicBand */
		DMUSIC_CreateDirectMusicBand (&IID_IDirectMusicBand, (LPDIRECTMUSICBAND*)&obj->pBand, NULL);
		obj->pBand->pObject = obj;
		*ppObject = (LPDIRECTMUSICOBJECT) obj;
		return S_OK;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;
}
	
/*****************************************************************************
 * IDirectMusicBandObjectStream implementation
 */
/* IDirectMusicBandObjectStream IUnknown part: */
HRESULT WINAPI IDirectMusicBandObjectStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicBandObjectStream,iface);

	if (IsEqualGUID(riid, &IID_IUnknown)
		|| IsEqualGUID(riid, &IID_IPersistStream)) {
		IDirectMusicBandObjectStream_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicBandObjectStream_AddRef (LPPERSISTSTREAM iface)
{
	ICOM_THIS(IDirectMusicBandObjectStream,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicBandObjectStream_Release (LPPERSISTSTREAM iface)
{
	ICOM_THIS(IDirectMusicBandObjectStream,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicBandObjectStream IPersist part: */
HRESULT WINAPI IDirectMusicBandObjectStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID)
{
	return E_NOTIMPL;
}

/* IDirectMusicBandObjectStream IPersistStream part: */
HRESULT WINAPI IDirectMusicBandObjectStream_IsDirty (LPPERSISTSTREAM iface)
{
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicBandObjectStream_Load (LPPERSISTSTREAM iface, IStream* pStm)
{
	ICOM_THIS(IDirectMusicBandObjectStream,iface);
	FOURCC chunkID;
	DWORD chunkSize, StreamSize, StreamCount, ListSize[3], ListCount[3];
	LARGE_INTEGER liMove; /* used when skipping chunks */
	DMUS_IO_REFERENCE tempReferenceHeader;
	DMUS_OBJECTDESC ObjDesc;
	IDirectMusicBandImpl* pBand = This->pParentObject->pBand; /* that's where we load data to */
	LPDIRECTMUSICLOADER pLoader;
	LPDIRECTMUSICGETLOADER pGetLoader;
	
	IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
	IStream_Read (pStm, &chunkSize, sizeof(DWORD), NULL);
	TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (chunkID), chunkSize);
	switch (chunkID) {	
		case FOURCC_RIFF: {
			IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);				
			TRACE_(dmfile)(": RIFF chunk of type %s", debugstr_fourcc(chunkID));
			StreamSize = chunkSize - sizeof(FOURCC);
			StreamCount = 0;
			switch (chunkID) {
				case DMUS_FOURCC_BAND_FORM: {
					TRACE_(dmfile)(": band  form\n");
					do {
						IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
						IStream_Read (pStm, &chunkSize, sizeof(FOURCC), NULL);
						StreamCount += sizeof(FOURCC) + sizeof(DWORD) + chunkSize;
						TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (chunkID), chunkSize);
						switch (chunkID) {
							case DMUS_FOURCC_GUID_CHUNK: {
								TRACE_(dmfile)(": GUID chunk\n");
								IStream_Read (pStm, pBand->guidID, chunkSize, NULL);
								break;
							}
							case DMUS_FOURCC_VERSION_CHUNK: {
								TRACE_(dmfile)(": version chunk\n");
								IStream_Read (pStm, pBand->vVersion, chunkSize, NULL);
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
											TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (chunkID), chunkSize);
											switch (chunkID) {
												/* don't ask me why, but M$ puts INFO elements in UNFO list sometimes
                                              (though strings seem to be valid unicode) */
												case mmioFOURCC('I','N','A','M'):
												case DMUS_FOURCC_UNAM_CHUNK: {
													TRACE_(dmfile)(": name chunk\n");
													pBand->wzName = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, chunkSize);
													IStream_Read (pStm, pBand->wzName, chunkSize, NULL);
													break;
												}
												case mmioFOURCC('I','A','R','T'):
												case DMUS_FOURCC_UART_CHUNK: {
													TRACE_(dmfile)(": artist chunk\n");
													pBand->wzArtist = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, chunkSize);
													IStream_Read (pStm, pBand->wzArtist, chunkSize, NULL);
													break;
												}
												case mmioFOURCC('I','C','O','P'):
												case DMUS_FOURCC_UCOP_CHUNK: {
													TRACE_(dmfile)(": copyright chunk\n");
													pBand->wzCopyright = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, chunkSize);
													IStream_Read (pStm, pBand->wzCopyright, chunkSize, NULL);
													break;
												}
												case mmioFOURCC('I','S','B','J'):
												case DMUS_FOURCC_USBJ_CHUNK: {
													TRACE_(dmfile)(": subject chunk\n");
													pBand->wzSubject = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, chunkSize);
													IStream_Read (pStm, pBand->wzSubject, chunkSize, NULL);
													break;
												}
												case mmioFOURCC('I','C','M','T'):
												case DMUS_FOURCC_UCMT_CHUNK: {
													TRACE_(dmfile)(": comment chunk\n");
													pBand->wzComment = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, chunkSize);
													IStream_Read (pStm, pBand->wzComment, chunkSize, NULL);
													break;
												}
												default: {
													TRACE_(dmfile)(": unknown chunk (irrevelant & skipping)\n");
													liMove.QuadPart = chunkSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;						
												}
											}
											TRACE_(dmfile)(": ListCount[0] = %ld < ListSize[0] = %ld\n", ListCount[0], ListSize[0]);
										} while (ListCount[0] < ListSize[0]);
										break;
									}
									case DMUS_FOURCC_INSTRUMENTS_LIST: {
										TRACE_(dmfile)(": instruments list\n");
										do {
											IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
											IStream_Read (pStm, &chunkSize, sizeof(FOURCC), NULL);
											ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + chunkSize;
											TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (chunkID), chunkSize);
											switch (chunkID) {
												case FOURCC_LIST: {
													IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);				
													TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(chunkID));
													ListSize[1] = chunkSize - sizeof(FOURCC);
													ListCount[1] = 0;
													switch (chunkID) {
														case DMUS_FOURCC_INSTRUMENT_LIST: {
															/* init new instrument list entry */
															LPDMUS_PRIVATE_INSTRUMENT pNewInstrument = (LPDMUS_PRIVATE_INSTRUMENT) HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(DMUS_PRIVATE_INSTRUMENT));
															TRACE_(dmfile)(": instrument list\n");
															do {
																IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
																IStream_Read (pStm, &chunkSize, sizeof(FOURCC), NULL);
																ListCount[1] += sizeof(FOURCC) + sizeof(DWORD) + chunkSize;
																TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (chunkID), chunkSize);
																switch (chunkID) {
																	case DMUS_FOURCC_INSTRUMENT_CHUNK: {
																		TRACE_(dmfile)(": band instrument header\n");
																		IStream_Read (pStm, &pNewInstrument->pInstrument, chunkSize, NULL);
																		break;
																	}
																	case FOURCC_LIST: {
																		IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);				
																		TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(chunkID));
																		ListSize[2] = chunkSize - sizeof(FOURCC);
																		ListCount[2] = 0;
																		switch (chunkID) {
																			case DMUS_FOURCC_REF_LIST: {
																				TRACE_(dmfile)(": reference list\n");
																				ZeroMemory ((LPVOID)&ObjDesc, sizeof(DMUS_OBJECTDESC));
																				do {
																					IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
																					IStream_Read (pStm, &chunkSize, sizeof(FOURCC), NULL);
																					ListCount[2] += sizeof(FOURCC) + sizeof(DWORD) + chunkSize;
																					TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (chunkID), chunkSize);
																					switch (chunkID) {
																						case DMUS_FOURCC_REF_CHUNK: {
																							TRACE_(dmfile)(": reference header chunk\n");
																							IStream_Read (pStm, &tempReferenceHeader, chunkSize, NULL);
																							/* copy retrieved data to DMUS_OBJECTDESC */
																							ObjDesc.dwSize = sizeof(DMUS_OBJECTDESC);
																							ObjDesc.guidClass = tempReferenceHeader.guidClassID;
																							ObjDesc.dwValidData = tempReferenceHeader.dwValidData;
																							break;																	
																						}
																						case DMUS_FOURCC_GUID_CHUNK: {
																							TRACE_(dmfile)(": guid chunk\n");
																							IStream_Read (pStm, &ObjDesc.guidObject, chunkSize, NULL);
																							break;
																						}
																						case DMUS_FOURCC_DATE_CHUNK: {
																							TRACE_(dmfile)(": file date chunk\n");
																							IStream_Read (pStm, &ObjDesc.ftDate, chunkSize, NULL);
																							break;
																						}
																						case DMUS_FOURCC_NAME_CHUNK: {
																							TRACE_(dmfile)(": name chunk\n");
																							IStream_Read (pStm, &ObjDesc.wszName, chunkSize, NULL);
																							break;
																						}
																						case DMUS_FOURCC_FILE_CHUNK: {
																							TRACE_(dmfile)(": file name chunk\n");
																							IStream_Read (pStm, &ObjDesc.wszFileName, chunkSize, NULL);
																							break;
																						}
																						case DMUS_FOURCC_CATEGORY_CHUNK: {
																							TRACE_(dmfile)(": category chunk\n");
																							IStream_Read (pStm, &ObjDesc.wszCategory, chunkSize, NULL);
																							break;
																						}
																						case DMUS_FOURCC_VERSION_CHUNK: {
																							TRACE_(dmfile)(": version chunk\n");
																							IStream_Read (pStm, &ObjDesc.vVersion, chunkSize, NULL);
																							break;
																						}
																						default: {
																							TRACE_(dmfile)(": unknown chunk (skipping)\n");
																							liMove.QuadPart = chunkSize;
																							IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL); /* skip this chunk */
																							break;
																						}
																					}
																					TRACE_(dmfile)(": ListCount[2] = %ld < ListSize[2] = %ld\n", ListCount[2], ListSize[2]);
																				} while (ListCount[2] < ListSize[2]);
																				/* let's see what we have */
																				TRACE_(dmfile)(": (READ): reference: dwSize = %ld; dwValidData = %ld; guidObject = %s; guidClass = %s; \
vVersion = %08lx,%08lx; wszName = %s; wszCategory = %s; wszFileName = %s\n", ObjDesc.dwSize, ObjDesc.dwValidData, debugstr_guid(&ObjDesc.guidObject), debugstr_guid(&ObjDesc.guidClass),
ObjDesc.vVersion.dwVersionMS, ObjDesc.vVersion.dwVersionLS, debugstr_w(ObjDesc.wszName), debugstr_w(ObjDesc.wszCategory), debugstr_w(ObjDesc.wszFileName));
																				/* now, let's convience loader to load reference */								
																				if (IStream_QueryInterface (pStm, &IID_IDirectMusicGetLoader, (LPVOID*)&pGetLoader) == S_OK) {
																					if (IDirectMusicGetLoader_GetLoader (pGetLoader, &pLoader) == S_OK) {
																						/* load referenced object */
																						IDirectMusicObject* pObject;
																						if(FAILED(IDirectMusicLoader_GetObject (pLoader, &ObjDesc, &IID_IDirectMusicObject, (LPVOID*)&pObject)))
																						/* acquire collection from loaded referenced object */
																						if(FAILED(IDirectMusicObject_QueryInterface (pObject, &IID_IDirectMusicCollection, (LPVOID*)&pNewInstrument->ppReferenceCollection)))
																							IDirectMusicLoader_Release (pLoader);
																					}
																					IDirectMusicGetLoader_Release (pGetLoader);											
																				} else {
																					ERR("Could not get IDirectMusicGetLoader... reference will not be loaded :(\n");
																					/* E_FAIL */
																				}
																			break;
																		}
																		default: {
																			TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
																			return E_FAIL;
																		}
																	}
																	break;
																}
																	default: {
																		TRACE_(dmfile)(": unknown chunk (skipping)\n");
																		liMove.QuadPart = chunkSize;
																		IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL); /* skip this chunk */
																		break;					
																	}	
																}
																TRACE_(dmfile)(": ListCount[1] = %ld < ListSize[1] = %ld\n", ListCount[1], ListSize[1]);
															} while (ListCount[1] < ListSize[1]);
															/* hmm... in dxdiag segment's band there aren't any references, but loader still desperatly
														       loads default collection... does that mean that if there is no reference, use default?
														    */
															if (!pNewInstrument->ppReferenceCollection) {
																TRACE_(dmfile)(": (READ): loading default collection (as no specific reference was made)\n");
																ZeroMemory ((LPVOID)&ObjDesc, sizeof(DMUS_OBJECTDESC));
																ObjDesc.dwSize = sizeof(DMUS_OBJECTDESC);
        														ObjDesc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_OBJECT;
																ObjDesc.guidObject = GUID_DefaultGMCollection;
																ObjDesc.guidClass = CLSID_DirectMusicCollection;
																if (SUCCEEDED(IStream_QueryInterface (pStm, &IID_IDirectMusicGetLoader, (LPVOID*)&pGetLoader))) {
																	if (SUCCEEDED(IDirectMusicGetLoader_GetLoader (pGetLoader, &pLoader))) {
																		IDirectMusicObject* pObject;
																		if (SUCCEEDED(IDirectMusicLoader_GetObject (pLoader, &ObjDesc, &IID_IDirectMusicObject, (LPVOID*)&pObject))) {
																			IDirectMusicObject_QueryInterface (pObject, &IID_IDirectMusicCollection, (LPVOID*)&pNewInstrument->ppReferenceCollection);
																			IDirectMusicLoader_Release (pLoader);
																		}
																	}
																	IDirectMusicGetLoader_Release (pGetLoader);											
																} else {
																	ERR("Could not get IDirectMusicGetLoader... reference will not be loaded :(\n");
																	/* E_FAIL */
																}
															}
															list_add_tail (&This->pParentObject->pBand->Instruments, &pNewInstrument->entry);
															break;
														}
														default: {
															TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
															return E_FAIL;
														}
													}
													break;
												}
												default: {
													TRACE_(dmfile)(": unknown chunk (irrevelant & skipping)\n");
													liMove.QuadPart = chunkSize;
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
										liMove.QuadPart = chunkSize - sizeof(FOURCC);
										IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
										break;						
									}
								}
								break;
							}	
							default: {
								TRACE_(dmfile)(": unknown chunk (irrevelant & skipping)\n");
								liMove.QuadPart = chunkSize;
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
			liMove.QuadPart = chunkSize;
			IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
			return E_FAIL;
		}
	}
	
	/* DEBUG: dumps whole band object tree: */
	if (TRACE_ON(dmband)) {
		int r = 0;
		DMUS_PRIVATE_INSTRUMENT *tmpEntry;
		struct list *listEntry;

		TRACE("*** IDirectMusicBand (%p) ***\n", pBand);
		if (pBand->guidID)
			TRACE(" - GUID = %s\n", debugstr_guid(pBand->guidID));
		if (pBand->vVersion)
			TRACE(" - Version = %i,%i,%i,%i\n", (pBand->vVersion->dwVersionMS >> 8) && 0x0000FFFF, pBand->vVersion->dwVersionMS && 0x0000FFFF, \
				(pBand->vVersion->dwVersionLS >> 8) && 0x0000FFFF, pBand->vVersion->dwVersionLS && 0x0000FFFF);
		if (pBand->wzName)
			TRACE(" - Name = %s\n", debugstr_w(pBand->wzName));
		if (pBand->wzArtist)		
			TRACE(" - Artist = %s\n", debugstr_w(pBand->wzArtist));
		if (pBand->wzCopyright)
			TRACE(" - Copyright = %s\n", debugstr_w(pBand->wzCopyright));
		if (pBand->wzSubject)
			TRACE(" - Subject = %s\n", debugstr_w(pBand->wzSubject));
		if (pBand->wzComment)
			TRACE(" - Comment = %s\n", debugstr_w(pBand->wzComment));
		
		TRACE(" - Instruments:\n");
		
		LIST_FOR_EACH (listEntry, &This->pParentObject->pBand->Instruments) {
			tmpEntry = LIST_ENTRY( listEntry, DMUS_PRIVATE_INSTRUMENT, entry );
			TRACE("    - Instrument[%i]:\n", r);
			TRACE("       - Instrument header:\n");
			TRACE("          - dwPatch = %ld\n", tmpEntry->pInstrument.dwPatch);
			TRACE("          - dwAssignPatch = %ld\n", tmpEntry->pInstrument.dwAssignPatch);
			TRACE("          - dwNoteRanges[4] = %ln\n", tmpEntry->pInstrument.dwNoteRanges);
			TRACE("          - dwPChannel = %ld\n", tmpEntry->pInstrument.dwPChannel);
			TRACE("          - dwFlags = %ld\n", tmpEntry->pInstrument.dwFlags);
			TRACE("          - bPan = %i\n", tmpEntry->pInstrument.bPan);
			TRACE("          - bVolume = %i\n", tmpEntry->pInstrument.bVolume);
			TRACE("          - nTranspose = %i\n", tmpEntry->pInstrument.nTranspose);
			TRACE("          - dwChannelPriority = %ld\n", tmpEntry->pInstrument.dwChannelPriority);
			TRACE("          - nPitchBendRange = %i\n", tmpEntry->pInstrument.nPitchBendRange);
			TRACE("       - Reference collection: %p\n", tmpEntry->ppReferenceCollection);
			r++;
		}
	}

	return S_OK;
}

HRESULT WINAPI IDirectMusicBandObjectStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty)
{
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicBandObjectStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize)
{
	return E_NOTIMPL;
}

ICOM_VTABLE(IPersistStream) DirectMusicBandObjectStream_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicBandObjectStream_QueryInterface,
	IDirectMusicBandObjectStream_AddRef,
	IDirectMusicBandObjectStream_Release,
	IDirectMusicBandObjectStream_GetClassID,
	IDirectMusicBandObjectStream_IsDirty,
	IDirectMusicBandObjectStream_Load,
	IDirectMusicBandObjectStream_Save,
	IDirectMusicBandObjectStream_GetSizeMax
};
