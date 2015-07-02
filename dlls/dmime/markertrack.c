/* IDirectMusicMarkerTrack Implementation
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

WINE_DEFAULT_DEBUG_CHANNEL(dmime);

/*****************************************************************************
 * IDirectMusicMarkerTrack implementation
 */
typedef struct IDirectMusicMarkerTrack {
    const IUnknownVtbl *UnknownVtbl;
    IDirectMusicTrack IDirectMusicTrack_iface;
    const IPersistStreamVtbl *PersistStreamVtbl;
    LONG ref;
    DMUS_OBJECTDESC *pDesc;
} IDirectMusicMarkerTrack;

/* IDirectMusicMarkerTrack IUnknown part: */
static HRESULT WINAPI IDirectMusicMarkerTrack_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicMarkerTrack, UnknownVtbl, iface);
	TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);

	if (IsEqualIID (riid, &IID_IUnknown)) {
		*ppobj = &This->UnknownVtbl;
		IUnknown_AddRef (iface);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IDirectMusicTrack)) {
		*ppobj = &This->IDirectMusicTrack_iface;
		IUnknown_AddRef (iface);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IPersistStream)) {
		*ppobj = &This->PersistStreamVtbl;
		IUnknown_AddRef (iface);
		return S_OK;
	}
	
	WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicMarkerTrack_IUnknown_AddRef (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicMarkerTrack, UnknownVtbl, iface);
        ULONG ref = InterlockedIncrement(&This->ref);

	TRACE("(%p): AddRef from %d\n", This, ref - 1);

	DMIME_LockModule();

	return ref;
}

static ULONG WINAPI IDirectMusicMarkerTrack_IUnknown_Release (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicMarkerTrack, UnknownVtbl, iface);
	ULONG ref = InterlockedDecrement(&This->ref);
	TRACE("(%p): ReleaseRef to %d\n", This, ref);
	
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}

	DMIME_UnlockModule();
	
	return ref;
}

static const IUnknownVtbl DirectMusicMarkerTrack_Unknown_Vtbl = {
	IDirectMusicMarkerTrack_IUnknown_QueryInterface,
	IDirectMusicMarkerTrack_IUnknown_AddRef,
	IDirectMusicMarkerTrack_IUnknown_Release
};

/* IDirectMusicMarkerTrack IDirectMusicTrack8 part: */
static inline IDirectMusicMarkerTrack *impl_from_IDirectMusicTrack(IDirectMusicTrack *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicMarkerTrack, IDirectMusicTrack_iface);
}

static HRESULT WINAPI IDirectMusicTrackImpl_QueryInterface(IDirectMusicTrack *iface, REFIID riid,
        void **ppobj)
{
    IDirectMusicMarkerTrack *This = impl_from_IDirectMusicTrack(iface);
	return IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicTrackImpl_AddRef(IDirectMusicTrack *iface)
{
    IDirectMusicMarkerTrack *This = impl_from_IDirectMusicTrack(iface);
	return IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

static ULONG WINAPI IDirectMusicTrackImpl_Release(IDirectMusicTrack *iface)
{
    IDirectMusicMarkerTrack *This = impl_from_IDirectMusicTrack(iface);
	return IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

static HRESULT WINAPI IDirectMusicTrackImpl_Init(IDirectMusicTrack *iface,
        IDirectMusicSegment *pSegment)
{
        IDirectMusicMarkerTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %p): stub\n", This, pSegment);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrackImpl_InitPlay(IDirectMusicTrack *iface,
        IDirectMusicSegmentState *pSegmentState, IDirectMusicPerformance *pPerformance,
        void **ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
        IDirectMusicMarkerTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %p, %p, %p, %d, %d): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrackImpl_EndPlay(IDirectMusicTrack *iface, void *pStateData)
{
        IDirectMusicMarkerTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %p): stub\n", This, pStateData);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrackImpl_Play(IDirectMusicTrack *iface, void *pStateData,
        MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
        IDirectMusicMarkerTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %p, %d, %d, %d, %d, %p, %p, %d): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrackImpl_GetParam(IDirectMusicTrack *iface, REFGUID rguidType,
        MUSIC_TIME mtTime, MUSIC_TIME *pmtNext, void *pParam)
{
        IDirectMusicMarkerTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %s, %d, %p, %p): stub\n", This, debugstr_dmguid(rguidType), mtTime, pmtNext, pParam);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrackImpl_SetParam(IDirectMusicTrack *iface, REFGUID rguidType,
        MUSIC_TIME mtTime, void *pParam)
{
        IDirectMusicMarkerTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %s, %d, %p): stub\n", This, debugstr_dmguid(rguidType), mtTime, pParam);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrackImpl_IsParamSupported(IDirectMusicTrack *iface,
        REFGUID rguidType)
{
        IDirectMusicMarkerTrack *This = impl_from_IDirectMusicTrack(iface);

	TRACE("(%p, %s)\n", This, debugstr_dmguid(rguidType));
	if (IsEqualGUID (rguidType, &GUID_Play_Marker)
		|| IsEqualGUID (rguidType, &GUID_Valid_Start_Time)) {
		TRACE("param supported\n");
		return S_OK;
	}
	TRACE("param unsupported\n");
	return DMUS_E_TYPE_UNSUPPORTED;
}

static HRESULT WINAPI IDirectMusicTrackImpl_AddNotificationType(IDirectMusicTrack *iface,
        REFGUID rguidNotificationType)
{
        IDirectMusicMarkerTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrackImpl_RemoveNotificationType(IDirectMusicTrack *iface,
        REFGUID rguidNotificationType)
{
        IDirectMusicMarkerTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrackImpl_Clone(IDirectMusicTrack *iface, MUSIC_TIME mtStart,
        MUSIC_TIME mtEnd, IDirectMusicTrack **ppTrack)
{
        IDirectMusicMarkerTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %d, %d, %p): stub\n", This, mtStart, mtEnd, ppTrack);
	return S_OK;
}

static const IDirectMusicTrackVtbl dmtrack_vtbl = {
    IDirectMusicTrackImpl_QueryInterface,
    IDirectMusicTrackImpl_AddRef,
    IDirectMusicTrackImpl_Release,
    IDirectMusicTrackImpl_Init,
    IDirectMusicTrackImpl_InitPlay,
    IDirectMusicTrackImpl_EndPlay,
    IDirectMusicTrackImpl_Play,
    IDirectMusicTrackImpl_GetParam,
    IDirectMusicTrackImpl_SetParam,
    IDirectMusicTrackImpl_IsParamSupported,
    IDirectMusicTrackImpl_AddNotificationType,
    IDirectMusicTrackImpl_RemoveNotificationType,
    IDirectMusicTrackImpl_Clone
};

/* IDirectMusicMarkerTrack IPersistStream part: */
static HRESULT WINAPI IDirectMusicMarkerTrack_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicMarkerTrack, PersistStreamVtbl, iface);
	return IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicMarkerTrack_IPersistStream_AddRef (LPPERSISTSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicMarkerTrack, PersistStreamVtbl, iface);
	return IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

static ULONG WINAPI IDirectMusicMarkerTrack_IPersistStream_Release (LPPERSISTSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicMarkerTrack, PersistStreamVtbl, iface);
	return IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

static HRESULT WINAPI IDirectMusicMarkerTrack_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID) {
	return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicMarkerTrack_IPersistStream_IsDirty (LPPERSISTSTREAM iface) {
	return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicMarkerTrack_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm) {
	FIXME(": Loading not implemented yet\n");
	return S_OK;
}

static HRESULT WINAPI IDirectMusicMarkerTrack_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty) {
	return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicMarkerTrack_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize) {
	return E_NOTIMPL;
}

static const IPersistStreamVtbl DirectMusicMarkerTrack_PersistStream_Vtbl = {
	IDirectMusicMarkerTrack_IPersistStream_QueryInterface,
	IDirectMusicMarkerTrack_IPersistStream_AddRef,
	IDirectMusicMarkerTrack_IPersistStream_Release,
	IDirectMusicMarkerTrack_IPersistStream_GetClassID,
	IDirectMusicMarkerTrack_IPersistStream_IsDirty,
	IDirectMusicMarkerTrack_IPersistStream_Load,
	IDirectMusicMarkerTrack_IPersistStream_Save,
	IDirectMusicMarkerTrack_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT WINAPI create_dmmarkertrack(REFIID lpcGUID, void **ppobj)
{
	IDirectMusicMarkerTrack* track;
	
	track = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicMarkerTrack));
	if (NULL == track) {
		*ppobj = NULL;
		return E_OUTOFMEMORY;
	}
	track->UnknownVtbl = &DirectMusicMarkerTrack_Unknown_Vtbl;
    track->IDirectMusicTrack_iface.lpVtbl = &dmtrack_vtbl;
	track->PersistStreamVtbl = &DirectMusicMarkerTrack_PersistStream_Vtbl;
	track->pDesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DMUS_OBJECTDESC));
	DM_STRUCT_INIT(track->pDesc);
	track->pDesc->dwValidData |= DMUS_OBJ_CLASS;
	track->pDesc->guidClass = CLSID_DirectMusicMarkerTrack;
	track->ref = 0; /* will be inited by QueryInterface */
	
	return IDirectMusicMarkerTrack_IUnknown_QueryInterface ((LPUNKNOWN)&track->UnknownVtbl, lpcGUID, ppobj);
}
