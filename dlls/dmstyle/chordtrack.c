/* IDirectMusicChordTrack Implementation
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
 * IDirectMusicChordTrack implementation
 */
/* IDirectMusicChordTrack IUnknown part: */
HRESULT WINAPI IDirectMusicChordTrack_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicChordTrack, UnknownVtbl, iface);

	if (IsEqualIID (riid, &IID_IUnknown)) {
		*ppobj = (LPUNKNOWN)&This->UnknownVtbl;
		IDirectMusicChordTrack_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IDirectMusicTrack)
	  || IsEqualIID (riid, &IID_IDirectMusicTrack8)) {
		*ppobj = (LPDIRECTMUSICTRACK8)&This->TrackVtbl;
		IDirectMusicChordTrack_IDirectMusicTrack_AddRef ((LPDIRECTMUSICTRACK8)&This->TrackVtbl);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IPersistStream)) {
		*ppobj = (LPPERSISTSTREAM)&This->PersistStreamVtbl;
		IDirectMusicChordTrack_IPersistStream_AddRef ((LPPERSISTSTREAM)&This->PersistStreamVtbl);
		return S_OK;
	}
	
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicChordTrack_IUnknown_AddRef (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicChordTrack, UnknownVtbl, iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicChordTrack_IUnknown_Release (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicChordTrack, UnknownVtbl, iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

ICOM_VTABLE(IUnknown) DirectMusicChordTrack_Unknown_Vtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicChordTrack_IUnknown_QueryInterface,
	IDirectMusicChordTrack_IUnknown_AddRef,
	IDirectMusicChordTrack_IUnknown_Release
};

/* IDirectMusicChordTrack IDirectMusicTrack8 part: */
HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
	return IDirectMusicChordTrack_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

ULONG WINAPI IDirectMusicChordTrack_IDirectMusicTrack_AddRef (LPDIRECTMUSICTRACK8 iface) {
	ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
	return IDirectMusicChordTrack_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

ULONG WINAPI IDirectMusicChordTrack_IDirectMusicTrack_Release (LPDIRECTMUSICTRACK8 iface) {
	ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
	return IDirectMusicChordTrack_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment)
{
	ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
	FIXME("(%p, %p): stub\n", This, pSegment);
	return S_OK;
}

HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
	ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
	FIXME("(%p, %p, %p, %p, %ld, %ld): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);
	return S_OK;
}

HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData)
{
	ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
	FIXME("(%p, %p): stub\n", This, pStateData);
	return S_OK;
}

HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID)
{
	ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
	FIXME("(%p, %p, %ld, %ld, %ld, %ld, %p, %p, %ld): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam) {
	ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
	FIXME("(%p, %s, %ld, %p, %p): stub\n", This, debugstr_guid(rguidType), mtTime, pmtNext, pParam);
	return S_OK;
}

HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam) {
	ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
	FIXME("(%p, %s, %ld, %p): stub\n", This, debugstr_guid(rguidType), mtTime, pParam);
	return S_OK;
}

HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType) {
	ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);

	TRACE("(%p, %s): ", This, debugstr_guid(rguidType));
	if (IsEqualGUID (rguidType, &GUID_BandParam)
		|| IsEqualGUID (rguidType, &GUID_ChordParam)
		|| IsEqualGUID (rguidType, &GUID_RhythmParam)) {
		TRACE("param supported\n");
		return S_OK;
	}
	TRACE("param unsupported\n");
	return DMUS_E_TYPE_UNSUPPORTED;
}

HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType) {
	ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));
	return S_OK;
}

HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType) {
	ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));
	return S_OK;
}

HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack) {
	ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
	FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);
	return S_OK;
}

HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID) {
	ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
	FIXME("(%p, %p, %lli, %lli, %lli, %ld, %p, %p, %ld): stub\n", This, pStateData, rtStart, rtEnd, rtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags) {
	ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
	FIXME("(%p, %s, %lli, %p, %p, %p, %ld): stub\n", This, debugstr_guid(rguidType), rtTime, prtNext, pParam, pStateData, dwFlags);
	return S_OK;
}

HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags) {
	ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
	FIXME("(%p, %s, %lli, %p, %p, %ld): stub\n", This, debugstr_guid(rguidType), rtTime, pParam, pStateData, dwFlags);
	return S_OK;
}

HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack) {
	ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
	FIXME("(%p, %p, %ld, %p): stub\n", This, pContext, dwTrackGroup, ppResultTrack);
	return S_OK;
}

HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack) {
	ICOM_THIS_MULTI(IDirectMusicChordTrack, TrackVtbl, iface);
	FIXME("(%p, %p, %ld, %p, %ld, %p): stub\n", This, pNewTrack, mtJoin, pContext, dwTrackGroup, ppResultTrack);
	return S_OK;
}

ICOM_VTABLE(IDirectMusicTrack8) DirectMusicChordTrack_Track_Vtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicChordTrack_IDirectMusicTrack_QueryInterface,
	IDirectMusicChordTrack_IDirectMusicTrack_AddRef,
	IDirectMusicChordTrack_IDirectMusicTrack_Release,
	IDirectMusicChordTrack_IDirectMusicTrack_Init,
	IDirectMusicChordTrack_IDirectMusicTrack_InitPlay,
	IDirectMusicChordTrack_IDirectMusicTrack_EndPlay,
	IDirectMusicChordTrack_IDirectMusicTrack_Play,
	IDirectMusicChordTrack_IDirectMusicTrack_GetParam,
	IDirectMusicChordTrack_IDirectMusicTrack_SetParam,
	IDirectMusicChordTrack_IDirectMusicTrack_IsParamSupported,
	IDirectMusicChordTrack_IDirectMusicTrack_AddNotificationType,
	IDirectMusicChordTrack_IDirectMusicTrack_RemoveNotificationType,
	IDirectMusicChordTrack_IDirectMusicTrack_Clone,
	IDirectMusicChordTrack_IDirectMusicTrack_PlayEx,
	IDirectMusicChordTrack_IDirectMusicTrack_GetParamEx,
	IDirectMusicChordTrack_IDirectMusicTrack_SetParamEx,
	IDirectMusicChordTrack_IDirectMusicTrack_Compose,
	IDirectMusicChordTrack_IDirectMusicTrack_Join
};

/* IDirectMusicChordTrack IPersistStream part: */
HRESULT WINAPI IDirectMusicChordTrack_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicChordTrack, PersistStreamVtbl, iface);
	return IDirectMusicChordTrack_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

ULONG WINAPI IDirectMusicChordTrack_IPersistStream_AddRef (LPPERSISTSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicChordTrack, PersistStreamVtbl, iface);
	return IDirectMusicChordTrack_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

ULONG WINAPI IDirectMusicChordTrack_IPersistStream_Release (LPPERSISTSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicChordTrack, PersistStreamVtbl, iface);
	return IDirectMusicChordTrack_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

HRESULT WINAPI IDirectMusicChordTrack_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID) {
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicChordTrack_IPersistStream_IsDirty (LPPERSISTSTREAM iface) {
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicChordTrack_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm) {
	FIXME(": Loading not implemented yet\n");
	return S_OK;
}

HRESULT WINAPI IDirectMusicChordTrack_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty) {
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicChordTrack_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize) {
	return E_NOTIMPL;
}

ICOM_VTABLE(IPersistStream) DirectMusicChordTrack_PersistStream_Vtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicChordTrack_IPersistStream_QueryInterface,
	IDirectMusicChordTrack_IPersistStream_AddRef,
	IDirectMusicChordTrack_IPersistStream_Release,
	IDirectMusicChordTrack_IPersistStream_GetClassID,
	IDirectMusicChordTrack_IPersistStream_IsDirty,
	IDirectMusicChordTrack_IPersistStream_Load,
	IDirectMusicChordTrack_IPersistStream_Save,
	IDirectMusicChordTrack_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicChordTrack (LPCGUID lpcGUID, LPVOID *ppobj, LPUNKNOWN pUnkOuter) {
	IDirectMusicChordTrack* track;
	
	track = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicChordTrack));
	if (NULL == track) {
		*ppobj = (LPVOID) NULL;
		return E_OUTOFMEMORY;
	}
	track->UnknownVtbl = &DirectMusicChordTrack_Unknown_Vtbl;
	track->TrackVtbl = &DirectMusicChordTrack_Track_Vtbl;
	track->PersistStreamVtbl = &DirectMusicChordTrack_PersistStream_Vtbl;
	track->pDesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DMUS_OBJECTDESC));
	DM_STRUCT_INIT(track->pDesc);
	track->pDesc->dwValidData |= DMUS_OBJ_CLASS;
	memcpy (&track->pDesc->guidClass, &CLSID_DirectMusicChordTrack, sizeof (CLSID));
	track->ref = 0; /* will be inited by QueryInterface */
	
	return IDirectMusicChordTrack_IUnknown_QueryInterface ((LPUNKNOWN)&track->UnknownVtbl, lpcGUID, ppobj);
}
