/* IDirectMusicBandTrack Implementation
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
 * IDirectMusicBandTrack implementation
 */
/* IDirectMusicBandTrack IUnknown part: */
HRESULT WINAPI IDirectMusicBandTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicBandTrack,iface);

	if (IsEqualIID (riid, &IID_IUnknown) || 
	    IsEqualIID (riid, &IID_IDirectMusicTrack) ||
	    IsEqualIID (riid, &IID_IDirectMusicTrack8)) {
		IDirectMusicBandTrack_AddRef(iface);
		*ppobj = This;
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IPersistStream)) {
		IDirectMusicBandTrackStream_AddRef ((LPPERSISTSTREAM)This->pStream);
		*ppobj = This->pStream;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicBandTrack_AddRef (LPDIRECTMUSICTRACK8 iface)
{
	ICOM_THIS(IDirectMusicBandTrack,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicBandTrack_Release (LPDIRECTMUSICTRACK8 iface)
{
	ICOM_THIS(IDirectMusicBandTrack,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicBandTrack IDirectMusicTrack part: */
HRESULT WINAPI IDirectMusicBandTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment)
{
	ICOM_THIS(IDirectMusicBandTrack,iface);

	FIXME("(%p, %p): stub\n", This, pSegment);

	return S_OK;
}

HRESULT WINAPI IDirectMusicBandTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
	ICOM_THIS(IDirectMusicBandTrack,iface);

	FIXME("(%p, %p, %p, %p, %ld, %ld): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicBandTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData)
{
	ICOM_THIS(IDirectMusicBandTrack,iface);

	FIXME("(%p, %p): stub\n", This, pStateData);

	return S_OK;
}

HRESULT WINAPI IDirectMusicBandTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID)
{
	ICOM_THIS(IDirectMusicBandTrack,iface);

	FIXME("(%p, %p, %ld, %ld, %ld, %ld, %p, %p, %ld): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);

	return S_OK;
}

HRESULT WINAPI IDirectMusicBandTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam)
{
	ICOM_THIS(IDirectMusicBandTrack,iface);

	FIXME("(%p, %s, %ld, %p, %p): stub\n", This, debugstr_guid(rguidType), mtTime, pmtNext, pParam);

	return S_OK;
}

HRESULT WINAPI IDirectMusicBandTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam)
{
	ICOM_THIS(IDirectMusicBandTrack,iface);

	FIXME("(%p, %s, %ld, %p): stub\n", This, debugstr_guid(rguidType), mtTime, pParam);

	return S_OK;
}

HRESULT WINAPI IDirectMusicBandTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType)
{
	ICOM_THIS(IDirectMusicBandTrack,iface);

	TRACE("(%p, %s): ", This, debugstr_guid(rguidType));
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

HRESULT WINAPI IDirectMusicBandTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType)
{
	ICOM_THIS(IDirectMusicBandTrack,iface);

	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));

	return S_OK;
}

HRESULT WINAPI IDirectMusicBandTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType)
{
	ICOM_THIS(IDirectMusicBandTrack,iface);

	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));

	return S_OK;
}

HRESULT WINAPI IDirectMusicBandTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack)
{
	ICOM_THIS(IDirectMusicBandTrack,iface);

	FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);

	return S_OK;
}

/* IDirectMusicBandTrack IDirectMusicTrack8 part: */
HRESULT WINAPI IDirectMusicBandTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID)
{
	ICOM_THIS(IDirectMusicBandTrack,iface);

	FIXME("(%p, %p, %lli, %lli, %lli, %ld, %p, %p, %ld): stub\n", This, pStateData, rtStart, rtEnd, rtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);

	return S_OK;
}

HRESULT WINAPI IDirectMusicBandTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags)
{
	ICOM_THIS(IDirectMusicBandTrack,iface);

	FIXME("(%p, %s, %lli, %p, %p, %p, %ld): stub\n", This, debugstr_guid(rguidType), rtTime, prtNext, pParam, pStateData, dwFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicBandTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags)
{
	ICOM_THIS(IDirectMusicBandTrack,iface);

	FIXME("(%p, %s, %lli, %p, %p, %ld): stub\n", This, debugstr_guid(rguidType), rtTime, pParam, pStateData, dwFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicBandTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack)
{
	ICOM_THIS(IDirectMusicBandTrack,iface);

	FIXME("(%p, %p, %ld, %p): stub\n", This, pContext, dwTrackGroup, ppResultTrack);

	return S_OK;
}

HRESULT WINAPI IDirectMusicBandTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack)
{
	ICOM_THIS(IDirectMusicBandTrack,iface);

	FIXME("(%p, %p, %ld, %p, %ld, %p): stub\n", This, pNewTrack, mtJoin, pContext, dwTrackGroup, ppResultTrack);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicTrack8) DirectMusicBandTrack_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicBandTrack_QueryInterface,
	IDirectMusicBandTrack_AddRef,
	IDirectMusicBandTrack_Release,
	IDirectMusicBandTrack_Init,
	IDirectMusicBandTrack_InitPlay,
	IDirectMusicBandTrack_EndPlay,
	IDirectMusicBandTrack_Play,
	IDirectMusicBandTrack_GetParam,
	IDirectMusicBandTrack_SetParam,
	IDirectMusicBandTrack_IsParamSupported,
	IDirectMusicBandTrack_AddNotificationType,
	IDirectMusicBandTrack_RemoveNotificationType,
	IDirectMusicBandTrack_Clone,
	IDirectMusicBandTrack_PlayEx,
	IDirectMusicBandTrack_GetParamEx,
	IDirectMusicBandTrack_SetParamEx,
	IDirectMusicBandTrack_Compose,
	IDirectMusicBandTrack_Join
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicBandTrack (LPCGUID lpcGUID, LPDIRECTMUSICTRACK8 *ppTrack, LPUNKNOWN pUnkOuter)
{
	IDirectMusicBandTrack* track;
	
	if (IsEqualIID (lpcGUID, &IID_IDirectMusicTrack)
		|| IsEqualIID (lpcGUID, &IID_IDirectMusicTrack8)) {
		track = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicBandTrack));
		if (NULL == track) {
			*ppTrack = (LPDIRECTMUSICTRACK8) NULL;
			return E_OUTOFMEMORY;
		}
		track->lpVtbl = &DirectMusicBandTrack_Vtbl;
		track->ref = 1;
		track->pStream = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(IDirectMusicBandTrackStream));
		track->pStream->lpVtbl = &DirectMusicBandTrackStream_Vtbl;
		track->pStream->ref = 1;	
		track->pStream->pParentTrack = track;
		*ppTrack = (LPDIRECTMUSICTRACK8) track;
		return S_OK;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;
}


/*****************************************************************************
 * IDirectMusicBandTrackStream implementation
 */
/* IDirectMusicBandTrackStream IUnknown part: */
HRESULT WINAPI IDirectMusicBandTrackStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicBandTrackStream,iface);

	if (IsEqualIID (riid, &IID_IUnknown)
		|| IsEqualIID (riid, &IID_IPersistStream)) {
		IDirectMusicBandTrackStream_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicBandTrackStream_AddRef (LPPERSISTSTREAM iface)
{
	ICOM_THIS(IDirectMusicBandTrackStream,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicBandTrackStream_Release (LPPERSISTSTREAM iface)
{
	ICOM_THIS(IDirectMusicBandTrackStream,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicBandTrackStream IPersist part: */
HRESULT WINAPI IDirectMusicBandTrackStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID)
{
	return E_NOTIMPL;
}

/* IDirectMusicBandTrackStream IPersistStream part: */
HRESULT WINAPI IDirectMusicBandTrackStream_IsDirty (LPPERSISTSTREAM iface)
{
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicBandTrackStream_Load (LPPERSISTSTREAM iface, IStream* pStm)
{
	ICOM_THIS(IDirectMusicBandTrackStream,iface);
	FOURCC chunkID;
	DWORD chunkSize, StreamSize, StreamCount, ListSize[3], ListCount[3];
	LARGE_INTEGER liMove; /* used when skipping chunks */
	DMUS_OBJECTDESC ObjDesc;
	IDirectMusicBandTrack* pTrack = This->pParentTrack; /* that's where we load data to */
	DMUS_IO_BAND_ITEM_HEADER tempHeaderV1;
	DMUS_IO_BAND_ITEM_HEADER2 tempHeaderV2;
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
				case DMUS_FOURCC_BANDTRACK_FORM: {
					TRACE_(dmfile)(": band track form\n");
					do {
						IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
						IStream_Read (pStm, &chunkSize, sizeof(FOURCC), NULL);
						StreamCount += sizeof(FOURCC) + sizeof(DWORD) + chunkSize;
						TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (chunkID), chunkSize);
						switch (chunkID) {
							case DMUS_FOURCC_BANDTRACK_CHUNK: {
								TRACE_(dmfile)(": band track header chunk\n");
								IStream_Read (pStm, &pTrack->btkHeader, chunkSize, NULL);
								break;
							}
							case DMUS_FOURCC_GUID_CHUNK: {
								TRACE_(dmfile)(": GUID chunk\n");
								IStream_Read (pStm, &pTrack->vVersion, chunkSize, NULL);
								break;
							}
							case DMUS_FOURCC_VERSION_CHUNK: {
								TRACE_(dmfile)(": version chunk\n");
								IStream_Read (pStm, &pTrack->guidID, chunkSize, NULL);
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
												case DMUS_FOURCC_UNAM_CHUNK: {
													TRACE_(dmfile)(": name chunk\n");
													pTrack->wszName = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, chunkSize);
													IStream_Read (pStm, pTrack->wszName, chunkSize, NULL);
													break;
												}
												case DMUS_FOURCC_UART_CHUNK: {
													TRACE_(dmfile)(": artist chunk\n");
													pTrack->wszArtist = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, chunkSize);
													IStream_Read (pStm, pTrack->wszArtist, chunkSize, NULL);
													break;
												}
												case DMUS_FOURCC_UCOP_CHUNK: {
													TRACE_(dmfile)(": copyright chunk\n");
													pTrack->wszCopyright = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, chunkSize);
													IStream_Read (pStm, pTrack->wszCopyright, chunkSize, NULL);
													break;
												}
												case DMUS_FOURCC_USBJ_CHUNK: {
													TRACE_(dmfile)(": subject chunk\n");
													pTrack->wszSubject = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, chunkSize);
													IStream_Read (pStm, pTrack->wszSubject, chunkSize, NULL);
													break;
												}
												case DMUS_FOURCC_UCMT_CHUNK: {
													TRACE_(dmfile)(": comment chunk\n");
													pTrack->wszComment = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, chunkSize);
													IStream_Read (pStm, pTrack->wszComment, chunkSize, NULL);
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
									case DMUS_FOURCC_BANDS_LIST: {
										TRACE_(dmfile)(": bands list\n");
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
														case DMUS_FOURCC_BAND_LIST: {
															TRACE_(dmfile)(": band list\n");
															do {
																IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
																IStream_Read (pStm, &chunkSize, sizeof(FOURCC), NULL);
																ListCount[1] += sizeof(FOURCC) + sizeof(DWORD) + chunkSize;
																TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (chunkID), chunkSize);
																switch (chunkID) {
																	case DMUS_FOURCC_BANDITEM_CHUNK: {
																		TRACE_(dmfile)(": band item header (v.1) chunk\n");
																		IStream_Read (pStm, &tempHeaderV1, chunkSize, NULL);
																		/* now transfer data to universal header */
																		pTrack->pBandHeaders[pTrack->dwBands].dwVersion = 1;
																		pTrack->pBandHeaders[pTrack->dwBands].lBandTime = tempHeaderV1.lBandTime;
																		TRACE_(dmfile)(": (READ): header: lBandTime = %li\n", tempHeaderV1.lBandTime);
																		break;
																	}
																	case DMUS_FOURCC_BANDITEM_CHUNK2: {
																		TRACE_(dmfile)(": band item header (v.2) chunk\n");
																		IStream_Read (pStm, &tempHeaderV2, chunkSize, NULL);
																		/* now transfer data to universal header */
																		pTrack->pBandHeaders[pTrack->dwBands].dwVersion = 2;
																		pTrack->pBandHeaders[pTrack->dwBands].lBandTimeLogical = tempHeaderV2.lBandTimeLogical;
																		pTrack->pBandHeaders[pTrack->dwBands].lBandTimePhysical = tempHeaderV2.lBandTimePhysical;
																		break;
																	}
																	case FOURCC_RIFF: {
																		TRACE_(dmfile)(": RIFF chunk (probably band form)\n");																		
																		liMove.QuadPart = 0;
																		liMove.QuadPart -= (sizeof(FOURCC) + sizeof(DWORD)); /* goto the beginning of chunk */
																		IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
																		/* perform sacrificial ritual so that loader will load band */
																		ZeroMemory ((LPVOID)&ObjDesc, sizeof(DMUS_OBJECTDESC));
																		ObjDesc.dwSize = sizeof(DMUS_OBJECTDESC);
																		ObjDesc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_STREAM;
																		ObjDesc.guidClass = CLSID_DirectMusicBand;
																		ObjDesc.pStream = pStm;
																		/* now pray... */
																		if (SUCCEEDED(IStream_QueryInterface (pStm, &IID_IDirectMusicGetLoader, (LPVOID*)&pGetLoader))) {
																			if (SUCCEEDED(IDirectMusicGetLoader_GetLoader (pGetLoader, &pLoader))) {
																				IDirectMusicObject* pObject;
																				if (SUCCEEDED(IDirectMusicLoader_GetObject (pLoader, &ObjDesc, &IID_IDirectMusicObject, (LPVOID*)&pObject))) {
																					/* acquire band from loaded object */
																					IDirectMusicObject_QueryInterface (pObject, &IID_IDirectMusicBand, (LPVOID*)&pTrack->ppBands[pTrack->dwBands]);
																					/*IDirectMusicLoader_Release (pLoader);*/
																				} else FIXME(": couldn't get band\n");
																			}
																			IDirectMusicGetLoader_Release (pGetLoader);											
																		} else {
																			ERR("Could not get IDirectMusicGetLoader... reference will not be loaded :(\n");
																			/* E_FAIL */
																		}
																		/* MSDN states that loader returns stream pointer to it's before-reading position, 
																		   which means that we must skip whole (loaded) chunk */
																		liMove.QuadPart = sizeof(FOURCC) + sizeof(DWORD) + chunkID;
																		IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
																		break;
																	}
																	default: {
																		TRACE_(dmfile)(": unknown chunk (irrevelant & skipping)\n");
																		liMove.QuadPart = chunkSize;
																		IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
																		break;						
																	}
																}
																TRACE_(dmfile)(": ListCount[1] = %ld < ListSize[1] = %ld\n", ListCount[1], ListSize[1]);
															} while (ListCount[1] < ListSize[1]);
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
											TRACE_(dmfile)(": ListCount[0] = %ld < ListSize[0] = %ld\n", ListCount[0], ListSize[0]);
										} while (ListCount[0] < ListSize[0]);
										pTrack->dwBands++; /* add reference count */										
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
								TRACE_(dmfile)(": unknown chunk (skipping)\n");
								liMove.QuadPart = chunkSize;
								IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL); /* skip this chunk */
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
		
	return S_OK;
}

HRESULT WINAPI IDirectMusicBandTrackStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty)
{
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicBandTrackStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize)
{
	return E_NOTIMPL;
}

ICOM_VTABLE(IPersistStream) DirectMusicBandTrackStream_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicBandTrackStream_QueryInterface,
	IDirectMusicBandTrackStream_AddRef,
	IDirectMusicBandTrackStream_Release,
	IDirectMusicBandTrackStream_GetClassID,
	IDirectMusicBandTrackStream_IsDirty,
	IDirectMusicBandTrackStream_Load,
	IDirectMusicBandTrackStream_Save,
	IDirectMusicBandTrackStream_GetSizeMax
};
