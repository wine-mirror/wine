/* IDirectMusicTimeSigTrack Implementation
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
 * IDirectMusicTimeSigTrack implementation
 */
typedef struct IDirectMusicTimeSigTrack {
    const IUnknownVtbl *UnknownVtbl;
    IDirectMusicTrack IDirectMusicTrack_iface;
    const IPersistStreamVtbl *PersistStreamVtbl;
    LONG ref;
    DMUS_OBJECTDESC *pDesc;
} IDirectMusicTimeSigTrack;

/* IDirectMusicTimeSigTrack IUnknown part: */
static HRESULT WINAPI IDirectMusicTimeSigTrack_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicTimeSigTrack, UnknownVtbl, iface);
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

static ULONG WINAPI IDirectMusicTimeSigTrack_IUnknown_AddRef (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicTimeSigTrack, UnknownVtbl, iface);
        ULONG ref = InterlockedIncrement(&This->ref);

	TRACE("(%p): AddRef from %d\n", This, ref - 1);

	DMIME_LockModule();

	return ref;
}

static ULONG WINAPI IDirectMusicTimeSigTrack_IUnknown_Release (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicTimeSigTrack, UnknownVtbl, iface);
	ULONG ref = InterlockedDecrement(&This->ref);
	TRACE("(%p): ReleaseRef to %d\n", This, ref);
	
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}

	DMIME_UnlockModule();
	
	return ref;
}

static const IUnknownVtbl DirectMusicTimeSigTrack_Unknown_Vtbl = {
	IDirectMusicTimeSigTrack_IUnknown_QueryInterface,
	IDirectMusicTimeSigTrack_IUnknown_AddRef,
	IDirectMusicTimeSigTrack_IUnknown_Release
};

/* IDirectMusicTimeSigTrack IDirectMusicTrack8 part: */
static inline IDirectMusicTimeSigTrack *impl_from_IDirectMusicTrack(IDirectMusicTrack *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicTimeSigTrack, IDirectMusicTrack_iface);
}

static HRESULT WINAPI IDirectMusicTrackImpl_QueryInterface(IDirectMusicTrack *iface, REFIID riid,
        void **ppobj)
{
    IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);
	return IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicTrackImpl_AddRef(IDirectMusicTrack *iface)
{
    IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);
	return IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

static ULONG WINAPI IDirectMusicTrackImpl_Release(IDirectMusicTrack *iface)
{
    IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);
	return IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

static HRESULT WINAPI IDirectMusicTrackImpl_Init(IDirectMusicTrack *iface,
        IDirectMusicSegment *pSegment)
{
        IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %p): stub\n", This, pSegment);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrackImpl_InitPlay(IDirectMusicTrack *iface,
        IDirectMusicSegmentState *pSegmentState, IDirectMusicPerformance *pPerformance,
        void **ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
        IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %p, %p, %p, %d, %d): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrackImpl_EndPlay(IDirectMusicTrack *iface, void *pStateData)
{
        IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %p): stub\n", This, pStateData);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrackImpl_Play(IDirectMusicTrack *iface, void *pStateData,
        MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
        IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %p, %d, %d, %d, %d, %p, %p, %d): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrackImpl_GetParam(IDirectMusicTrack *iface, REFGUID rguidType,
        MUSIC_TIME mtTime, MUSIC_TIME *pmtNext, void *pParam)
{
        IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %s, %d, %p, %p): stub\n", This, debugstr_dmguid(rguidType), mtTime, pmtNext, pParam);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrackImpl_SetParam(IDirectMusicTrack *iface, REFGUID rguidType,
        MUSIC_TIME mtTime, void *pParam)
{
        IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %s, %d, %p): stub\n", This, debugstr_dmguid(rguidType), mtTime, pParam);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrackImpl_IsParamSupported(IDirectMusicTrack *iface,
        REFGUID rguidType)
{
        IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);

	TRACE("(%p, %s)\n", This, debugstr_dmguid(rguidType));
	if (IsEqualGUID (rguidType, &GUID_DisableTimeSig)
		|| IsEqualGUID (rguidType, &GUID_EnableTimeSig)
		|| IsEqualGUID (rguidType, &GUID_TimeSignature)) {
		TRACE("param supported\n");
		return S_OK;
	}
	TRACE("param unsupported\n");
	return DMUS_E_TYPE_UNSUPPORTED;
}

static HRESULT WINAPI IDirectMusicTrackImpl_AddNotificationType(IDirectMusicTrack *iface,
        REFGUID rguidNotificationType)
{
        IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrackImpl_RemoveNotificationType(IDirectMusicTrack *iface,
        REFGUID rguidNotificationType)
{
        IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrackImpl_Clone(IDirectMusicTrack *iface, MUSIC_TIME mtStart,
        MUSIC_TIME mtEnd, IDirectMusicTrack **ppTrack)
{
        IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %d, %d, %p): stub\n", This, mtStart, mtEnd, ppTrack);
	return S_OK;
}

static const IDirectMusicTrackVtbl dmtack_vtbl = {
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

/* IDirectMusicTimeSigTrack IPersistStream part: */
static HRESULT WINAPI IDirectMusicTimeSigTrack_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicTimeSigTrack, PersistStreamVtbl, iface);
	return IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicTimeSigTrack_IPersistStream_AddRef (LPPERSISTSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicTimeSigTrack, PersistStreamVtbl, iface);
	return IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

static ULONG WINAPI IDirectMusicTimeSigTrack_IPersistStream_Release (LPPERSISTSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicTimeSigTrack, PersistStreamVtbl, iface);
	return IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

static HRESULT WINAPI IDirectMusicTimeSigTrack_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID) {
	return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicTimeSigTrack_IPersistStream_IsDirty (LPPERSISTSTREAM iface) {
	return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicTimeSigTrack_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm) {
	FIXME(": Loading not implemented yet\n");
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTimeSigTrack_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty) {
	return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicTimeSigTrack_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize) {
	return E_NOTIMPL;
}

static const IPersistStreamVtbl DirectMusicTimeSigTrack_PersistStream_Vtbl = {
	IDirectMusicTimeSigTrack_IPersistStream_QueryInterface,
	IDirectMusicTimeSigTrack_IPersistStream_AddRef,
	IDirectMusicTimeSigTrack_IPersistStream_Release,
	IDirectMusicTimeSigTrack_IPersistStream_GetClassID,
	IDirectMusicTimeSigTrack_IPersistStream_IsDirty,
	IDirectMusicTimeSigTrack_IPersistStream_Load,
	IDirectMusicTimeSigTrack_IPersistStream_Save,
	IDirectMusicTimeSigTrack_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT WINAPI create_dmtimesigtrack(REFIID lpcGUID, void **ppobj)
{
	IDirectMusicTimeSigTrack* track;
	
	track = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicTimeSigTrack));
	if (NULL == track) {
		*ppobj = NULL;
		return E_OUTOFMEMORY;
	}
	track->UnknownVtbl = &DirectMusicTimeSigTrack_Unknown_Vtbl;
    track->IDirectMusicTrack_iface.lpVtbl = &dmtack_vtbl;
	track->PersistStreamVtbl = &DirectMusicTimeSigTrack_PersistStream_Vtbl;
	track->pDesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DMUS_OBJECTDESC));
	DM_STRUCT_INIT(track->pDesc);
	track->pDesc->dwValidData |= DMUS_OBJ_CLASS;
	track->pDesc->guidClass = CLSID_DirectMusicTimeSigTrack;
	track->ref = 0; /* will be inited by QueryInterface */

	return IDirectMusicTimeSigTrack_IUnknown_QueryInterface ((LPUNKNOWN)&track->UnknownVtbl, lpcGUID, ppobj);
}
