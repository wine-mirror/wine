/* IDirectMusicStyleTrack Implementation
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

#include "dmstyle_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmstyle);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

/*****************************************************************************
 * IDirectMusicStyleTrack implementation
 */
/* IDirectMusicStyleTrack IUnknown part: */
HRESULT WINAPI IDirectMusicStyleTrack_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicStyleTrack, UnknownVtbl, iface);

	if (IsEqualIID (riid, &IID_IUnknown)) {
		*ppobj = (LPUNKNOWN)&This->UnknownVtbl;
		IDirectMusicStyleTrack_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IDirectMusicTrack)
	  || IsEqualIID (riid, &IID_IDirectMusicTrack8)) {
		*ppobj = (LPDIRECTMUSICTRACK8)&This->TrackVtbl;
		IDirectMusicStyleTrack_IDirectMusicTrack_AddRef ((LPDIRECTMUSICTRACK8)&This->TrackVtbl);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IPersistStream)) {
		*ppobj = (LPPERSISTSTREAM)&This->PersistStreamVtbl;
		IDirectMusicStyleTrack_IPersistStream_AddRef ((LPPERSISTSTREAM)&This->PersistStreamVtbl);
		return S_OK;
	}
	
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicStyleTrack_IUnknown_AddRef (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicStyleTrack, UnknownVtbl, iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicStyleTrack_IUnknown_Release (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicStyleTrack, UnknownVtbl, iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

ICOM_VTABLE(IUnknown) DirectMusicStyleTrack_Unknown_Vtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicStyleTrack_IUnknown_QueryInterface,
	IDirectMusicStyleTrack_IUnknown_AddRef,
	IDirectMusicStyleTrack_IUnknown_Release
};

/* IDirectMusicStyleTrack IDirectMusicTrack8 part: */
HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicStyleTrack, TrackVtbl, iface);
	return IDirectMusicStyleTrack_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

ULONG WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_AddRef (LPDIRECTMUSICTRACK8 iface) {
	ICOM_THIS_MULTI(IDirectMusicStyleTrack, TrackVtbl, iface);
	return IDirectMusicStyleTrack_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

ULONG WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_Release (LPDIRECTMUSICTRACK8 iface) {
	ICOM_THIS_MULTI(IDirectMusicStyleTrack, TrackVtbl, iface);
	return IDirectMusicStyleTrack_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment)
{
	ICOM_THIS_MULTI(IDirectMusicStyleTrack, TrackVtbl, iface);
	FIXME("(%p, %p): stub\n", This, pSegment);
	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
	ICOM_THIS_MULTI(IDirectMusicStyleTrack, TrackVtbl, iface);
	FIXME("(%p, %p, %p, %p, %ld, %ld): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);
	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData)
{
	ICOM_THIS_MULTI(IDirectMusicStyleTrack, TrackVtbl, iface);
	FIXME("(%p, %p): stub\n", This, pStateData);
	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID)
{
	ICOM_THIS_MULTI(IDirectMusicStyleTrack, TrackVtbl, iface);
	FIXME("(%p, %p, %ld, %ld, %ld, %ld, %p, %p, %ld): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam) {
	ICOM_THIS_MULTI(IDirectMusicStyleTrack, TrackVtbl, iface);
	FIXME("(%p, %s, %ld, %p, %p): stub\n", This, debugstr_guid(rguidType), mtTime, pmtNext, pParam);
	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam) {
	ICOM_THIS_MULTI(IDirectMusicStyleTrack, TrackVtbl, iface);
	FIXME("(%p, %s, %ld, %p): stub\n", This, debugstr_guid(rguidType), mtTime, pParam);
	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType) {
	ICOM_THIS_MULTI(IDirectMusicStyleTrack, TrackVtbl, iface);

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

HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType) {
	ICOM_THIS_MULTI(IDirectMusicStyleTrack, TrackVtbl, iface);
	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));
	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType) {
	ICOM_THIS_MULTI(IDirectMusicStyleTrack, TrackVtbl, iface);
	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));
	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack) {
	ICOM_THIS_MULTI(IDirectMusicStyleTrack, TrackVtbl, iface);
	FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);
	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID) {
	ICOM_THIS_MULTI(IDirectMusicStyleTrack, TrackVtbl, iface);
	FIXME("(%p, %p, %lli, %lli, %lli, %ld, %p, %p, %ld): stub\n", This, pStateData, rtStart, rtEnd, rtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags) {
	ICOM_THIS_MULTI(IDirectMusicStyleTrack, TrackVtbl, iface);
	FIXME("(%p, %s, %lli, %p, %p, %p, %ld): stub\n", This, debugstr_guid(rguidType), rtTime, prtNext, pParam, pStateData, dwFlags);
	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags) {
	ICOM_THIS_MULTI(IDirectMusicStyleTrack, TrackVtbl, iface);
	FIXME("(%p, %s, %lli, %p, %p, %ld): stub\n", This, debugstr_guid(rguidType), rtTime, pParam, pStateData, dwFlags);
	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack) {
	ICOM_THIS_MULTI(IDirectMusicStyleTrack, TrackVtbl, iface);
	FIXME("(%p, %p, %ld, %p): stub\n", This, pContext, dwTrackGroup, ppResultTrack);
	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack) {
	ICOM_THIS_MULTI(IDirectMusicStyleTrack, TrackVtbl, iface);
	FIXME("(%p, %p, %ld, %p, %ld, %p): stub\n", This, pNewTrack, mtJoin, pContext, dwTrackGroup, ppResultTrack);
	return S_OK;
}

ICOM_VTABLE(IDirectMusicTrack8) DirectMusicStyleTrack_Track_Vtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicStyleTrack_IDirectMusicTrack_QueryInterface,
	IDirectMusicStyleTrack_IDirectMusicTrack_AddRef,
	IDirectMusicStyleTrack_IDirectMusicTrack_Release,
	IDirectMusicStyleTrack_IDirectMusicTrack_Init,
	IDirectMusicStyleTrack_IDirectMusicTrack_InitPlay,
	IDirectMusicStyleTrack_IDirectMusicTrack_EndPlay,
	IDirectMusicStyleTrack_IDirectMusicTrack_Play,
	IDirectMusicStyleTrack_IDirectMusicTrack_GetParam,
	IDirectMusicStyleTrack_IDirectMusicTrack_SetParam,
	IDirectMusicStyleTrack_IDirectMusicTrack_IsParamSupported,
	IDirectMusicStyleTrack_IDirectMusicTrack_AddNotificationType,
	IDirectMusicStyleTrack_IDirectMusicTrack_RemoveNotificationType,
	IDirectMusicStyleTrack_IDirectMusicTrack_Clone,
	IDirectMusicStyleTrack_IDirectMusicTrack_PlayEx,
	IDirectMusicStyleTrack_IDirectMusicTrack_GetParamEx,
	IDirectMusicStyleTrack_IDirectMusicTrack_SetParamEx,
	IDirectMusicStyleTrack_IDirectMusicTrack_Compose,
	IDirectMusicStyleTrack_IDirectMusicTrack_Join
};

/* IDirectMusicStyleTrack IPersistStream part: */
HRESULT WINAPI IDirectMusicStyleTrack_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicStyleTrack, PersistStreamVtbl, iface);
	return IDirectMusicStyleTrack_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

ULONG WINAPI IDirectMusicStyleTrack_IPersistStream_AddRef (LPPERSISTSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicStyleTrack, PersistStreamVtbl, iface);
	return IDirectMusicStyleTrack_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

ULONG WINAPI IDirectMusicStyleTrack_IPersistStream_Release (LPPERSISTSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicStyleTrack, PersistStreamVtbl, iface);
	return IDirectMusicStyleTrack_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

HRESULT WINAPI IDirectMusicStyleTrack_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID) {
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicStyleTrack_IPersistStream_IsDirty (LPPERSISTSTREAM iface) {
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicStyleTrack_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm) {
	FIXME(": Loading not implemented yet\n");
	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleTrack_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty) {
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicStyleTrack_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize) {
	return E_NOTIMPL;
}

ICOM_VTABLE(IPersistStream) DirectMusicStyleTrack_PersistStream_Vtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicStyleTrack_IPersistStream_QueryInterface,
	IDirectMusicStyleTrack_IPersistStream_AddRef,
	IDirectMusicStyleTrack_IPersistStream_Release,
	IDirectMusicStyleTrack_IPersistStream_GetClassID,
	IDirectMusicStyleTrack_IPersistStream_IsDirty,
	IDirectMusicStyleTrack_IPersistStream_Load,
	IDirectMusicStyleTrack_IPersistStream_Save,
	IDirectMusicStyleTrack_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicStyleTrack (LPCGUID lpcGUID, LPVOID *ppobj, LPUNKNOWN pUnkOuter) {
	IDirectMusicStyleTrack* track;
	
	track = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicStyleTrack));
	if (NULL == track) {
		*ppobj = (LPVOID) NULL;
		return E_OUTOFMEMORY;
	}
	track->UnknownVtbl = &DirectMusicStyleTrack_Unknown_Vtbl;
	track->TrackVtbl = &DirectMusicStyleTrack_Track_Vtbl;
	track->PersistStreamVtbl = &DirectMusicStyleTrack_PersistStream_Vtbl;
	track->pDesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DMUS_OBJECTDESC));
	DM_STRUCT_INIT(track->pDesc);
	track->pDesc->dwValidData |= DMUS_OBJ_CLASS;
	memcpy (&track->pDesc->guidClass, &CLSID_DirectMusicStyleTrack, sizeof (CLSID));
	track->ref = 0; /* will be inited by QueryInterface */
	
	return IDirectMusicStyleTrack_IUnknown_QueryInterface ((LPUNKNOWN)&track->UnknownVtbl, lpcGUID, ppobj);
}
