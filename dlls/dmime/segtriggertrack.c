/* IDirectMusicSegTriggerTrack Implementation
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

#include "dmime_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmime);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

/*****************************************************************************
 * IDirectMusicSegTriggerTrack implementation
 */
/* IDirectMusicSegTriggerTrack IUnknown part: */
HRESULT WINAPI IDirectMusicSegTriggerTrack_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, UnknownVtbl, iface);

	if (IsEqualIID (riid, &IID_IUnknown)) {
		*ppobj = (LPUNKNOWN)&This->UnknownVtbl;
		IDirectMusicSegTriggerTrack_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IDirectMusicTrack)
	  || IsEqualIID (riid, &IID_IDirectMusicTrack8)) {
		*ppobj = (LPDIRECTMUSICTRACK8)&This->TrackVtbl;
		IDirectMusicSegTriggerTrack_IDirectMusicTrack_AddRef ((LPDIRECTMUSICTRACK8)&This->TrackVtbl);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IPersistStream)) {
		*ppobj = (LPPERSISTSTREAM)&This->PersistStreamVtbl;
		IDirectMusicSegTriggerTrack_IPersistStream_AddRef ((LPPERSISTSTREAM)&This->PersistStreamVtbl);
		return S_OK;
	}
	
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicSegTriggerTrack_IUnknown_AddRef (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, UnknownVtbl, iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicSegTriggerTrack_IUnknown_Release (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, UnknownVtbl, iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

ICOM_VTABLE(IUnknown) DirectMusicSegTriggerTrack_Unknown_Vtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicSegTriggerTrack_IUnknown_QueryInterface,
	IDirectMusicSegTriggerTrack_IUnknown_AddRef,
	IDirectMusicSegTriggerTrack_IUnknown_Release
};

/* IDirectMusicSegTriggerTrack IDirectMusicTrack8 part: */
HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	return IDirectMusicSegTriggerTrack_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

ULONG WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_AddRef (LPDIRECTMUSICTRACK8 iface) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	return IDirectMusicSegTriggerTrack_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

ULONG WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_Release (LPDIRECTMUSICTRACK8 iface) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	return IDirectMusicSegTriggerTrack_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment)
{
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	FIXME("(%p, %p): stub\n", This, pSegment);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	FIXME("(%p, %p, %p, %p, %ld, %ld): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData)
{
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	FIXME("(%p, %p): stub\n", This, pStateData);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID)
{
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	FIXME("(%p, %p, %ld, %ld, %ld, %ld, %p, %p, %ld): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	FIXME("(%p, %s, %ld, %p, %p): stub\n", This, debugstr_guid(rguidType), mtTime, pmtNext, pParam);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	FIXME("(%p, %s, %ld, %p): stub\n", This, debugstr_guid(rguidType), mtTime, pParam);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);

	TRACE("(%p, %s): ", This, debugstr_guid(rguidType));
	/* didn't find any params */
	TRACE("param unsupported\n");
	return DMUS_E_TYPE_UNSUPPORTED;
}

HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	FIXME("(%p, %p, %lli, %lli, %lli, %ld, %p, %p, %ld): stub\n", This, pStateData, rtStart, rtEnd, rtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	FIXME("(%p, %s, %lli, %p, %p, %p, %ld): stub\n", This, debugstr_guid(rguidType), rtTime, prtNext, pParam, pStateData, dwFlags);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	FIXME("(%p, %s, %lli, %p, %p, %ld): stub\n", This, debugstr_guid(rguidType), rtTime, pParam, pStateData, dwFlags);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	FIXME("(%p, %p, %ld, %p): stub\n", This, pContext, dwTrackGroup, ppResultTrack);
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegTriggerTrack_IDirectMusicTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, TrackVtbl, iface);
	FIXME("(%p, %p, %ld, %p, %ld, %p): stub\n", This, pNewTrack, mtJoin, pContext, dwTrackGroup, ppResultTrack);
	return S_OK;
}

ICOM_VTABLE(IDirectMusicTrack8) DirectMusicSegTriggerTrack_Track_Vtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_QueryInterface,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_AddRef,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_Release,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_Init,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_InitPlay,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_EndPlay,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_Play,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_GetParam,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_SetParam,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_IsParamSupported,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_AddNotificationType,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_RemoveNotificationType,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_Clone,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_PlayEx,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_GetParamEx,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_SetParamEx,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_Compose,
	IDirectMusicSegTriggerTrack_IDirectMusicTrack_Join
};

/* IDirectMusicSegTriggerTrack IPersistStream part: */
HRESULT WINAPI IDirectMusicSegTriggerTrack_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, PersistStreamVtbl, iface);
	return IDirectMusicSegTriggerTrack_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

ULONG WINAPI IDirectMusicSegTriggerTrack_IPersistStream_AddRef (LPPERSISTSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, PersistStreamVtbl, iface);
	return IDirectMusicSegTriggerTrack_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

ULONG WINAPI IDirectMusicSegTriggerTrack_IPersistStream_Release (LPPERSISTSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicSegTriggerTrack, PersistStreamVtbl, iface);
	return IDirectMusicSegTriggerTrack_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

HRESULT WINAPI IDirectMusicSegTriggerTrack_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID) {
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicSegTriggerTrack_IPersistStream_IsDirty (LPPERSISTSTREAM iface) {
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicSegTriggerTrack_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm) {
	FIXME(": Loading not implemented yet\n");
	return S_OK;
}

HRESULT WINAPI IDirectMusicSegTriggerTrack_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty) {
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicSegTriggerTrack_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize) {
	return E_NOTIMPL;
}

ICOM_VTABLE(IPersistStream) DirectMusicSegTriggerTrack_PersistStream_Vtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicSegTriggerTrack_IPersistStream_QueryInterface,
	IDirectMusicSegTriggerTrack_IPersistStream_AddRef,
	IDirectMusicSegTriggerTrack_IPersistStream_Release,
	IDirectMusicSegTriggerTrack_IPersistStream_GetClassID,
	IDirectMusicSegTriggerTrack_IPersistStream_IsDirty,
	IDirectMusicSegTriggerTrack_IPersistStream_Load,
	IDirectMusicSegTriggerTrack_IPersistStream_Save,
	IDirectMusicSegTriggerTrack_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicSegTriggerTrack (LPCGUID lpcGUID, LPVOID *ppobj, LPUNKNOWN pUnkOuter) {
	IDirectMusicSegTriggerTrack* track;
	
	track = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicSegTriggerTrack));
	if (NULL == track) {
		*ppobj = (LPVOID) NULL;
		return E_OUTOFMEMORY;
	}
	track->UnknownVtbl = &DirectMusicSegTriggerTrack_Unknown_Vtbl;
	track->TrackVtbl = &DirectMusicSegTriggerTrack_Track_Vtbl;
	track->PersistStreamVtbl = &DirectMusicSegTriggerTrack_PersistStream_Vtbl;
	track->pDesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DMUS_OBJECTDESC));
	DM_STRUCT_INIT(track->pDesc);
	track->pDesc->dwValidData |= DMUS_OBJ_CLASS;
	memcpy (&track->pDesc->guidClass, &CLSID_DirectMusicSegTriggerTrack, sizeof (CLSID));
	track->ref = 0; /* will be inited by QueryInterface */
	
	return IDirectMusicSegTriggerTrack_IUnknown_QueryInterface ((LPUNKNOWN)&track->UnknownVtbl, lpcGUID, ppobj);
}
