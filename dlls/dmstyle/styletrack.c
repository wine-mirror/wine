/* IDirectMusicStyleTrack Implementation
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

#include "dmstyle_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmstyle);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

/*****************************************************************************
 * IDirectMusicStyleTrack implementation
 */
/* IDirectMusicStyleTrack IUnknown part: */
HRESULT WINAPI IDirectMusicStyleTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicStyleTrack,iface);

	if (IsEqualIID (riid, &IID_IUnknown) || 
	    IsEqualIID (riid, &IID_IDirectMusicTrack) ||
	    IsEqualIID (riid, &IID_IDirectMusicTrack8)) {
		IDirectMusicStyleTrack_AddRef(iface);
		*ppobj = This;
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IPersistStream)) {
		IDirectMusicStyleTrackStream_AddRef ((LPPERSISTSTREAM)This->pStream);
		*ppobj = This->pStream;
		return S_OK;
	}
	
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicStyleTrack_AddRef (LPDIRECTMUSICTRACK8 iface)
{
	ICOM_THIS(IDirectMusicStyleTrack,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicStyleTrack_Release (LPDIRECTMUSICTRACK8 iface)
{
	ICOM_THIS(IDirectMusicStyleTrack,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicStyleTrack IDirectMusicTrack part: */
HRESULT WINAPI IDirectMusicStyleTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment)
{
	ICOM_THIS(IDirectMusicStyleTrack,iface);

	FIXME("(%p, %p): stub\n", This, pSegment);

	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
	ICOM_THIS(IDirectMusicStyleTrack,iface);

	FIXME("(%p, %p, %p, %p, %ld, %ld): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData)
{
	ICOM_THIS(IDirectMusicStyleTrack,iface);

	FIXME("(%p, %p): stub\n", This, pStateData);

	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID)
{
	ICOM_THIS(IDirectMusicStyleTrack,iface);

	FIXME("(%p, %p, %ld, %ld, %ld, %ld, %p, %p, %ld): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);

	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam)
{
	ICOM_THIS(IDirectMusicStyleTrack,iface);

	FIXME("(%p, %s, %ld, %p, %p): stub\n", This, debugstr_guid(rguidType), mtTime, pmtNext, pParam);

	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam)
{
	ICOM_THIS(IDirectMusicStyleTrack,iface);

	FIXME("(%p, %s, %ld, %p): stub\n", This, debugstr_guid(rguidType), mtTime, pParam);

	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType)
{
	ICOM_THIS(IDirectMusicStyleTrack,iface);

	TRACE("(%p, %s): ", This, debugstr_guid(rguidType));
	if (IsEqualGUID (rguidType, &GUID_DisableTimeSig)
		|| IsEqualGUID (rguidType, &GUID_EnableTimeSig)
		|| IsEqualGUID (rguidType, &GUID_IDirectMusicStyle)
		|| IsEqualGUID (rguidType, &GUID_SeedVariations)
		|| IsEqualGUID (rguidType, &GUID_TimeSignature)) {
		TRACE("param supported\n");
		return S_OK;
		}

	TRACE("param unsupported\n");
	return DMUS_E_TYPE_UNSUPPORTED;
}

HRESULT WINAPI IDirectMusicStyleTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType)
{
	ICOM_THIS(IDirectMusicStyleTrack,iface);

	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));

	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType)
{
	ICOM_THIS(IDirectMusicStyleTrack,iface);

	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));

	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack)
{
	ICOM_THIS(IDirectMusicStyleTrack,iface);

	FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);

	return S_OK;
}

/* IDirectMusicStyleTrack IDirectMusicTrack8 part: */
HRESULT WINAPI IDirectMusicStyleTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID)
{
	ICOM_THIS(IDirectMusicStyleTrack,iface);

	FIXME("(%p, %p, %lli, %lli, %lli, %ld, %p, %p, %ld): stub\n", This, pStateData, rtStart, rtEnd, rtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);

	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags)
{
	ICOM_THIS(IDirectMusicStyleTrack,iface);

	FIXME("(%p, %s, %lli, %p, %p, %p, %ld): stub\n", This, debugstr_guid(rguidType), rtTime, prtNext, pParam, pStateData, dwFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags)
{
	ICOM_THIS(IDirectMusicStyleTrack,iface);

	FIXME("(%p, %s, %lli, %p, %p, %ld): stub\n", This, debugstr_guid(rguidType), rtTime, pParam, pStateData, dwFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack)
{
	ICOM_THIS(IDirectMusicStyleTrack,iface);

	FIXME("(%p, %p, %ld, %p): stub\n", This, pContext, dwTrackGroup, ppResultTrack);

	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack)
{
	ICOM_THIS(IDirectMusicStyleTrack,iface);

	FIXME("(%p, %p, %ld, %p, %ld, %p): stub\n", This, pNewTrack, mtJoin, pContext, dwTrackGroup, ppResultTrack);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicTrack8) DirectMusicStyleTrack_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicStyleTrack_QueryInterface,
	IDirectMusicStyleTrack_AddRef,
	IDirectMusicStyleTrack_Release,
	IDirectMusicStyleTrack_Init,
	IDirectMusicStyleTrack_InitPlay,
	IDirectMusicStyleTrack_EndPlay,
	IDirectMusicStyleTrack_Play,
	IDirectMusicStyleTrack_GetParam,
	IDirectMusicStyleTrack_SetParam,
	IDirectMusicStyleTrack_IsParamSupported,
	IDirectMusicStyleTrack_AddNotificationType,
	IDirectMusicStyleTrack_RemoveNotificationType,
	IDirectMusicStyleTrack_Clone,
	IDirectMusicStyleTrack_PlayEx,
	IDirectMusicStyleTrack_GetParamEx,
	IDirectMusicStyleTrack_SetParamEx,
	IDirectMusicStyleTrack_Compose,
	IDirectMusicStyleTrack_Join
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicStyleTrack (LPCGUID lpcGUID, LPDIRECTMUSICTRACK8 *ppTrack, LPUNKNOWN pUnkOuter)
{
	IDirectMusicStyleTrack* track;
	
	if (IsEqualIID (lpcGUID, &IID_IDirectMusicTrack)
		|| IsEqualIID (lpcGUID, &IID_IDirectMusicTrack8)) {
		track = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicStyleTrack));
		if (NULL == track) {
			*ppTrack = (LPDIRECTMUSICTRACK8) NULL;
			return E_OUTOFMEMORY;
		}
		track->lpVtbl = &DirectMusicStyleTrack_Vtbl;
		track->ref = 1;
		track->pStream = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(IDirectMusicStyleTrackStream));
		track->pStream->lpVtbl = &DirectMusicStyleTrackStream_Vtbl;
		track->pStream->ref = 1;	
		track->pStream->pParentTrack = track;
		*ppTrack = (LPDIRECTMUSICTRACK8) track;
		return S_OK;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;
}


/*****************************************************************************
 * IDirectMusicStyleTrackStream implementation
 */
/* IDirectMusicStyleTrackStream IUnknown part follow: */
HRESULT WINAPI IDirectMusicStyleTrackStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicStyleTrackStream,iface);

	if (IsEqualIID (riid, &IID_IUnknown)
		|| IsEqualIID (riid, &IID_IPersistStream)) {
		IDirectMusicStyleTrackStream_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}

	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicStyleTrackStream_AddRef (LPPERSISTSTREAM iface)
{
	ICOM_THIS(IDirectMusicStyleTrackStream,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicStyleTrackStream_Release (LPPERSISTSTREAM iface)
{
	ICOM_THIS(IDirectMusicStyleTrackStream,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicStyleTrackStream IPersist part: */
HRESULT WINAPI IDirectMusicStyleTrackStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID)
{
	return E_NOTIMPL;
}

/* IDirectMusicStyleTrackStream IPersistStream part: */
HRESULT WINAPI IDirectMusicStyleTrackStream_IsDirty (LPPERSISTSTREAM iface)
{
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicStyleTrackStream_Load (LPPERSISTSTREAM iface, IStream* pStm)
{
	ICOM_THIS(IDirectMusicStyleTrackStream,iface);
	FOURCC chunkID;
	DWORD chunkSize, ListSize[3], ListCount[3];
	LARGE_INTEGER liMove; /* used when skipping chunks */
	DMUS_IO_REFERENCE tempReferenceHeader;
	DMUS_OBJECTDESC ObjDesc;
	IDirectMusicStyleTrack* pTrack = This->pParentTrack; /* that's where we load data to */
	LPDIRECTMUSICLOADER pLoader;
	LPDIRECTMUSICGETLOADER pGetLoader;
	
	IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
	IStream_Read (pStm, &chunkSize, sizeof(DWORD), NULL);
	TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (chunkID), chunkSize);
	switch (chunkID) {	
		case FOURCC_LIST: {
			IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);				
			TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(chunkID));
			ListSize[0] = chunkSize - sizeof(FOURCC);
			ListCount[0] = 0;
			switch (chunkID) {
				case DMUS_FOURCC_STYLE_TRACK_LIST: {
					TRACE_(dmfile)(": style track list\n");
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
									case DMUS_FOURCC_STYLE_REF_LIST: {
										TRACE_(dmfile)(": style reference list\n");
										do {
											IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
											IStream_Read (pStm, &chunkSize, sizeof(FOURCC), NULL);
											ListCount[1] += sizeof(FOURCC) + sizeof(DWORD) + chunkSize;
											TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (chunkID), chunkSize);
											switch (chunkID) {
												case DMUS_FOURCC_TIME_STAMP_CHUNK: {
													TRACE_(dmfile)(": time stamp chunk\n");
													IStream_Read (pStm, &pTrack->pStampTimes[pTrack->dwStyles], sizeof(DWORD), NULL);
													TRACE_(dmfile)(": (READ): time stamp = %ld\n", pTrack->pStampTimes[pTrack->dwStyles]);
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
										/* let's see what we have */
										TRACE_(dmfile)(": (READ): reference: dwSize = %ld; dwValidData = %ld; guidObject = %s; guidClass = %s; \
vVersion = %08lx,%08lx; wszName = %s; wszCategory = %s; wszFileName = %s\n", ObjDesc.dwSize, ObjDesc.dwValidData, debugstr_guid(&ObjDesc.guidObject), debugstr_guid(&ObjDesc.guidClass), \
ObjDesc.vVersion.dwVersionMS, ObjDesc.vVersion.dwVersionLS, debugstr_w(ObjDesc.wszName), debugstr_w(ObjDesc.wszCategory), debugstr_w(ObjDesc.wszFileName));
										/* now, let's convience loader to load reference */								
										if (IStream_QueryInterface (pStm, &IID_IDirectMusicGetLoader, (LPVOID*)&pGetLoader) == S_OK) {
											if (IDirectMusicGetLoader_GetLoader (pGetLoader, &pLoader) == S_OK) {
												/* load referenced object */
												IDirectMusicObject* pObject;
												IDirectMusicLoader_GetObject (pLoader, &ObjDesc, &IID_IDirectMusicObject, (LPVOID*)&pObject);
												/* acquire style from loaded referenced object */
												IDirectMusicObject_QueryInterface (pObject, &IID_IDirectMusicStyle8, (LPVOID*)&pTrack->ppStyles[pTrack->dwStyles]);
												IDirectMusicLoader_Release (pLoader);
											}
											IDirectMusicGetLoader_Release (pGetLoader);											
										} else {
											ERR("Could not get IDirectMusicGetLoader... reference will not be loaded :(\n");
											/* E_FAIL */
										}
										pTrack->dwStyles++; /* add reference count */
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
						TRACE_(dmfile)(": ListCount[0] = %ld < ListSize[0] = %ld\n", ListCount[0], ListSize[0]);
					} while (ListCount[0] < ListSize[0]);
					break;
				}
				default: {
					TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
					liMove.QuadPart = ListSize[0];
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

HRESULT WINAPI IDirectMusicStyleTrackStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty)
{
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicStyleTrackStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize)
{
	return E_NOTIMPL;
}

ICOM_VTABLE(IPersistStream) DirectMusicStyleTrackStream_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicStyleTrackStream_QueryInterface,
	IDirectMusicStyleTrackStream_AddRef,
	IDirectMusicStyleTrackStream_Release,
	IDirectMusicStyleTrackStream_GetClassID,
	IDirectMusicStyleTrackStream_IsDirty,
	IDirectMusicStyleTrackStream_Load,
	IDirectMusicStyleTrackStream_Save,
	IDirectMusicStyleTrackStream_GetSizeMax
};
