/* IDirectMusicCommandTrack Implementation
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

#include "dmstyle_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmstyle);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

static ULONG WINAPI IDirectMusicCommandTrack_IUnknown_AddRef (LPUNKNOWN iface);
static ULONG WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
static ULONG WINAPI IDirectMusicCommandTrack_IPersistStream_AddRef (LPPERSISTSTREAM iface);

/*****************************************************************************
 * IDirectMusicCommandTrack implementation
 */
/* IDirectMusicCommandTrack IUnknown part: */
static HRESULT WINAPI IDirectMusicCommandTrack_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicCommandTrack, UnknownVtbl, iface);
	TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);

	if (IsEqualIID (riid, &IID_IUnknown)) {
		*ppobj = (LPUNKNOWN)&This->UnknownVtbl;
		IDirectMusicCommandTrack_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IDirectMusicTrack)
	  || IsEqualIID (riid, &IID_IDirectMusicTrack8)) {
		*ppobj = (LPDIRECTMUSICTRACK8)&This->TrackVtbl;
		IDirectMusicCommandTrack_IDirectMusicTrack_AddRef ((LPDIRECTMUSICTRACK8)&This->TrackVtbl);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IPersistStream)) {
		*ppobj = (LPPERSISTSTREAM)&This->PersistStreamVtbl;
		IDirectMusicCommandTrack_IPersistStream_AddRef ((LPPERSISTSTREAM)&This->PersistStreamVtbl);
		return S_OK;
	}
	
	WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicCommandTrack_IUnknown_AddRef (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicCommandTrack, UnknownVtbl, iface);
        ULONG ref = InterlockedIncrement(&This->ref);

	TRACE("(%p): AddRef from %d\n", This, ref - 1);

	DMSTYLE_LockModule();

	return ref;
}

static ULONG WINAPI IDirectMusicCommandTrack_IUnknown_Release (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicCommandTrack, UnknownVtbl, iface);
	ULONG ref = InterlockedDecrement(&This->ref);

	TRACE("(%p): ReleaseRef to %d\n", This, ref);

	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	
	DMSTYLE_UnlockModule();
	
	return ref;
}

static const IUnknownVtbl DirectMusicCommandTrack_Unknown_Vtbl = {
	IDirectMusicCommandTrack_IUnknown_QueryInterface,
	IDirectMusicCommandTrack_IUnknown_AddRef,
	IDirectMusicCommandTrack_IUnknown_Release
};

/* IDirectMusicCommandTrack IDirectMusicTrack8 part: */
static HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicCommandTrack, TrackVtbl, iface);
	return IDirectMusicCommandTrack_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_AddRef (LPDIRECTMUSICTRACK8 iface) {
	ICOM_THIS_MULTI(IDirectMusicCommandTrack, TrackVtbl, iface);
	return IDirectMusicCommandTrack_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

static ULONG WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_Release (LPDIRECTMUSICTRACK8 iface) {
	ICOM_THIS_MULTI(IDirectMusicCommandTrack, TrackVtbl, iface);
	return IDirectMusicCommandTrack_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

static HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment)
{
	ICOM_THIS_MULTI(IDirectMusicCommandTrack, TrackVtbl, iface);
	FIXME("(%p, %p): stub\n", This, pSegment);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
	ICOM_THIS_MULTI(IDirectMusicCommandTrack, TrackVtbl, iface);
	FIXME("(%p, %p, %p, %p, %d, %d): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData)
{
	ICOM_THIS_MULTI(IDirectMusicCommandTrack, TrackVtbl, iface);
	FIXME("(%p, %p): stub\n", This, pStateData);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID)
{
	ICOM_THIS_MULTI(IDirectMusicCommandTrack, TrackVtbl, iface);
	FIXME("(%p, %p, %ld, %ld, %ld, %d, %p, %p, %d): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam) {
	ICOM_THIS_MULTI(IDirectMusicCommandTrack, TrackVtbl, iface);
	FIXME("(%p, %s, %ld, %p, %p): stub\n", This, debugstr_dmguid(rguidType), mtTime, pmtNext, pParam);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam) {
	ICOM_THIS_MULTI(IDirectMusicCommandTrack, TrackVtbl, iface);
	FIXME("(%p, %s, %ld, %p): stub\n", This, debugstr_dmguid(rguidType), mtTime, pParam);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType) {
	ICOM_THIS_MULTI(IDirectMusicCommandTrack, TrackVtbl, iface);

	TRACE("(%p, %s)\n", This, debugstr_dmguid(rguidType));
	if (IsEqualGUID (rguidType, &GUID_CommandParam)
		|| IsEqualGUID (rguidType, &GUID_CommandParam2)
		|| IsEqualGUID (rguidType, &GUID_CommandParamNext)) {
		TRACE("param supported\n");
		return S_OK;
	}
	
	TRACE("param unsupported\n");
	return DMUS_E_TYPE_UNSUPPORTED;
}

static HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType) {
	ICOM_THIS_MULTI(IDirectMusicCommandTrack, TrackVtbl, iface);
	FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
	return S_OK;
}

static HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType) {
	ICOM_THIS_MULTI(IDirectMusicCommandTrack, TrackVtbl, iface);
	FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
	return S_OK;
}

static HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack) {
	ICOM_THIS_MULTI(IDirectMusicCommandTrack, TrackVtbl, iface);
	FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID) {
	ICOM_THIS_MULTI(IDirectMusicCommandTrack, TrackVtbl, iface);
	FIXME("(%p, %p, 0x%s, 0x%s, 0x%s, %d, %p, %p, %d): stub\n", This, pStateData, wine_dbgstr_longlong(rtStart),
	    wine_dbgstr_longlong(rtEnd), wine_dbgstr_longlong(rtOffset), dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags) {
	ICOM_THIS_MULTI(IDirectMusicCommandTrack, TrackVtbl, iface);
	FIXME("(%p, %s, 0x%s, %p, %p, %p, %d): stub\n", This, debugstr_dmguid(rguidType),
	    wine_dbgstr_longlong(rtTime), prtNext, pParam, pStateData, dwFlags);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags) {
	ICOM_THIS_MULTI(IDirectMusicCommandTrack, TrackVtbl, iface);
	FIXME("(%p, %s, 0x%s, %p, %p, %d): stub\n", This, debugstr_dmguid(rguidType),
	    wine_dbgstr_longlong(rtTime), pParam, pStateData, dwFlags);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack) {
	ICOM_THIS_MULTI(IDirectMusicCommandTrack, TrackVtbl, iface);
	FIXME("(%p, %p, %d, %p): stub\n", This, pContext, dwTrackGroup, ppResultTrack);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack) {
	ICOM_THIS_MULTI(IDirectMusicCommandTrack, TrackVtbl, iface);
	FIXME("(%p, %p, %ld, %p, %d, %p): stub\n", This, pNewTrack, mtJoin, pContext, dwTrackGroup, ppResultTrack);
	return S_OK;
}

static const IDirectMusicTrack8Vtbl DirectMusicCommandTrack_Track_Vtbl = {
	IDirectMusicCommandTrack_IDirectMusicTrack_QueryInterface,
	IDirectMusicCommandTrack_IDirectMusicTrack_AddRef,
	IDirectMusicCommandTrack_IDirectMusicTrack_Release,
	IDirectMusicCommandTrack_IDirectMusicTrack_Init,
	IDirectMusicCommandTrack_IDirectMusicTrack_InitPlay,
	IDirectMusicCommandTrack_IDirectMusicTrack_EndPlay,
	IDirectMusicCommandTrack_IDirectMusicTrack_Play,
	IDirectMusicCommandTrack_IDirectMusicTrack_GetParam,
	IDirectMusicCommandTrack_IDirectMusicTrack_SetParam,
	IDirectMusicCommandTrack_IDirectMusicTrack_IsParamSupported,
	IDirectMusicCommandTrack_IDirectMusicTrack_AddNotificationType,
	IDirectMusicCommandTrack_IDirectMusicTrack_RemoveNotificationType,
	IDirectMusicCommandTrack_IDirectMusicTrack_Clone,
	IDirectMusicCommandTrack_IDirectMusicTrack_PlayEx,
	IDirectMusicCommandTrack_IDirectMusicTrack_GetParamEx,
	IDirectMusicCommandTrack_IDirectMusicTrack_SetParamEx,
	IDirectMusicCommandTrack_IDirectMusicTrack_Compose,
	IDirectMusicCommandTrack_IDirectMusicTrack_Join
};

/* IDirectMusicCommandTrack IPersistStream part: */
static HRESULT WINAPI IDirectMusicCommandTrack_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicCommandTrack, PersistStreamVtbl, iface);
	return IDirectMusicCommandTrack_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicCommandTrack_IPersistStream_AddRef (LPPERSISTSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicCommandTrack, PersistStreamVtbl, iface);
	return IDirectMusicCommandTrack_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

static ULONG WINAPI IDirectMusicCommandTrack_IPersistStream_Release (LPPERSISTSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicCommandTrack, PersistStreamVtbl, iface);
	return IDirectMusicCommandTrack_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

static HRESULT WINAPI IDirectMusicCommandTrack_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID) {
	return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicCommandTrack_IPersistStream_IsDirty (LPPERSISTSTREAM iface) {
	return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicCommandTrack_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm) {
	ICOM_THIS_MULTI(IDirectMusicCommandTrack, PersistStreamVtbl, iface);
	FOURCC chunkID;
	DWORD chunkSize, dwSizeOfStruct, nrCommands;
	LARGE_INTEGER liMove; /* used when skipping chunks */
	
	IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
	IStream_Read (pStm, &chunkSize, sizeof(DWORD), NULL);
	TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (chunkID), chunkSize);
	switch (chunkID) {
		case DMUS_FOURCC_COMMANDTRACK_CHUNK: {
			int count;
			TRACE_(dmfile)(": command track chunk\n");
			IStream_Read (pStm, &dwSizeOfStruct, sizeof(DWORD), NULL);
			if (dwSizeOfStruct != sizeof(DMUS_IO_COMMAND)) {
				TRACE_(dmfile)(": declared size of struct (=%d) != actual size; indicates older direct music version\n", dwSizeOfStruct);
			}
			chunkSize -= sizeof(DWORD); /* now chunk size is one DWORD shorter */
			nrCommands = chunkSize/dwSizeOfStruct; /* and this is the number of commands */
			/* load each command separately in new entry */
			for (count = 0; count < nrCommands; count++) {
				LPDMUS_PRIVATE_COMMAND pNewCommand = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(DMUS_PRIVATE_COMMAND));
				IStream_Read (pStm, &pNewCommand->pCommand, dwSizeOfStruct, NULL);
				list_add_tail (&This->Commands, &pNewCommand->entry);
			}
			TRACE_(dmfile)(": reading finished\n");
			This->pDesc->dwValidData |= DMUS_OBJ_LOADED;
			break;
		}	
		default: {
			TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
			liMove.QuadPart = chunkSize;
			IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
			return E_FAIL;
		}
	}

	/* DEBUG: dumps whole band track object tree: */
	if (TRACE_ON(dmstyle)) {
		int r = 0;
		DMUS_PRIVATE_COMMAND *tmpEntry;
		struct list *listEntry;
		TRACE("*** IDirectMusicCommandTrack (%p) ***\n", This->TrackVtbl);
		TRACE(" - Commands:\n");
		LIST_FOR_EACH (listEntry, &This->Commands) {
			tmpEntry = LIST_ENTRY (listEntry, DMUS_PRIVATE_COMMAND, entry);
			TRACE("    - Command[%i]:\n", r);
			TRACE("       - mtTime = %li\n", tmpEntry->pCommand.mtTime);
			TRACE("       - wMeasure = %d\n", tmpEntry->pCommand.wMeasure);
			TRACE("       - bBeat = %i\n", tmpEntry->pCommand.bBeat);
			TRACE("       - bCommand = %i\n", tmpEntry->pCommand.bCommand);
			TRACE("       - bGrooveLevel = %i\n", tmpEntry->pCommand.bGrooveLevel);
			TRACE("       - bGrooveRange = %i\n", tmpEntry->pCommand.bGrooveRange);
			TRACE("       - bRepeatMode = %i\n", tmpEntry->pCommand.bRepeatMode);			
			r++;
		}
	}

	return S_OK;
}

static HRESULT WINAPI IDirectMusicCommandTrack_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty) {
	return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicCommandTrack_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize) {
	return E_NOTIMPL;
}

static const IPersistStreamVtbl DirectMusicCommandTrack_PersistStream_Vtbl = {
	IDirectMusicCommandTrack_IPersistStream_QueryInterface,
	IDirectMusicCommandTrack_IPersistStream_AddRef,
	IDirectMusicCommandTrack_IPersistStream_Release,
	IDirectMusicCommandTrack_IPersistStream_GetClassID,
	IDirectMusicCommandTrack_IPersistStream_IsDirty,
	IDirectMusicCommandTrack_IPersistStream_Load,
	IDirectMusicCommandTrack_IPersistStream_Save,
	IDirectMusicCommandTrack_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicCommandTrack (LPCGUID lpcGUID, LPVOID *ppobj, LPUNKNOWN pUnkOuter) {
	IDirectMusicCommandTrack* track;
	
	track = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicCommandTrack));
	if (NULL == track) {
		*ppobj = NULL;
		return E_OUTOFMEMORY;
	}
	track->UnknownVtbl = &DirectMusicCommandTrack_Unknown_Vtbl;
	track->TrackVtbl = &DirectMusicCommandTrack_Track_Vtbl;
	track->PersistStreamVtbl = &DirectMusicCommandTrack_PersistStream_Vtbl;
	track->pDesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DMUS_OBJECTDESC));
	DM_STRUCT_INIT(track->pDesc);
	track->pDesc->dwValidData |= DMUS_OBJ_CLASS;
	memcpy (&track->pDesc->guidClass, &CLSID_DirectMusicCommandTrack, sizeof (CLSID));
	track->ref = 0; /* will be inited by QueryInterface */
	list_init (&track->Commands);
	
	return IDirectMusicCommandTrack_IUnknown_QueryInterface ((LPUNKNOWN)&track->UnknownVtbl, lpcGUID, ppobj);
}
