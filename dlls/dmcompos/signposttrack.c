/* IDirectMusicSignPostTrack Implementation
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

#include "dmcompos_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmcompos);

static ULONG WINAPI IDirectMusicSignPostTrack_IUnknown_AddRef (LPUNKNOWN iface);
static ULONG WINAPI IDirectMusicSignPostTrack_IPersistStream_AddRef (LPPERSISTSTREAM iface);

/*****************************************************************************
 * IDirectMusicSignPostTrack implementation
 */
typedef struct IDirectMusicSignPostTrack {
    const IUnknownVtbl *UnknownVtbl;
    IDirectMusicTrack8 IDirectMusicTrack8_iface;
    const IPersistStreamVtbl *PersistStreamVtbl;
    LONG ref;
    DMUS_OBJECTDESC *pDesc;
} IDirectMusicSignPostTrack;

/* IDirectMusicSignPostTrack IUnknown part: */
static HRESULT WINAPI IDirectMusicSignPostTrack_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicSignPostTrack, UnknownVtbl, iface);
	TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);

	if (IsEqualIID (riid, &IID_IUnknown)) {
		*ppobj = &This->UnknownVtbl;
		IDirectMusicSignPostTrack_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IDirectMusicTrack)
	  || IsEqualIID (riid, &IID_IDirectMusicTrack8)) {
		*ppobj = &This->IDirectMusicTrack8_iface;
		IDirectMusicTrack8_AddRef(&This->IDirectMusicTrack8_iface);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IPersistStream)) {
		*ppobj = &This->PersistStreamVtbl;
		IDirectMusicSignPostTrack_IPersistStream_AddRef ((LPPERSISTSTREAM)&This->PersistStreamVtbl);
		return S_OK;
	}
	
	WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicSignPostTrack_IUnknown_AddRef (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicSignPostTrack, UnknownVtbl, iface);
        ULONG ref = InterlockedIncrement(&This->ref);

	TRACE("(%p): AddRef from %d\n", This, ref - 1);
	
	DMCOMPOS_LockModule();
	
	return ref;
}

static ULONG WINAPI IDirectMusicSignPostTrack_IUnknown_Release (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicSignPostTrack, UnknownVtbl, iface);
	ULONG ref = InterlockedDecrement(&This->ref);

	TRACE("(%p): ReleaseRef to %d\n", This, ref);
	
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	
	DMCOMPOS_UnlockModule();
	
	return ref;
}

static const IUnknownVtbl DirectMusicSignPostTrack_Unknown_Vtbl = {
	IDirectMusicSignPostTrack_IUnknown_QueryInterface,
	IDirectMusicSignPostTrack_IUnknown_AddRef,
	IDirectMusicSignPostTrack_IUnknown_Release
};

/* IDirectMusicSignPostTrack IDirectMusicTrack8 part: */
static inline IDirectMusicSignPostTrack *impl_from_IDirectMusicTrack8(IDirectMusicTrack8 *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicSignPostTrack, IDirectMusicTrack8_iface);
}

static HRESULT WINAPI IDirectMusicTrack8Impl_QueryInterface(IDirectMusicTrack8 *iface, REFIID riid,
        void **ppobj)
{
    IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);
	return IDirectMusicSignPostTrack_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicTrack8Impl_AddRef(IDirectMusicTrack8 *iface)
{
    IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);
	return IDirectMusicSignPostTrack_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

static ULONG WINAPI IDirectMusicTrack8Impl_Release(IDirectMusicTrack8 *iface)
{
    IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);
	return IDirectMusicSignPostTrack_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

static HRESULT WINAPI IDirectMusicTrack8Impl_Init(IDirectMusicTrack8 *iface,
        IDirectMusicSegment *pSegment)
{
        IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p): stub\n", This, pSegment);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_InitPlay(IDirectMusicTrack8 *iface,
        IDirectMusicSegmentState *pSegmentState, IDirectMusicPerformance *pPerformance,
        void **ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
        IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %p, %p, %d, %d): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_EndPlay(IDirectMusicTrack8 *iface, void *pStateData)
{
        IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p): stub\n", This, pStateData);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_Play(IDirectMusicTrack8 *iface, void *pStateData,
        MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
        IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %d, %d, %d, %d, %p, %p, %d): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_GetParam(IDirectMusicTrack8 *iface, REFGUID type,
        MUSIC_TIME time, MUSIC_TIME *next, void *param)
{
    IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %d, %p, %p): method not implemented\n", This, debugstr_dmguid(type), time,
            next, param);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_SetParam(IDirectMusicTrack8 *iface, REFGUID type,
        MUSIC_TIME time, void *param)
{
    IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %d, %p): method not implemented\n", This, debugstr_dmguid(type), time, param);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_IsParamSupported(IDirectMusicTrack8 *iface,
        REFGUID type)
{
    IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): method not implemented\n", This, debugstr_dmguid(type));
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_AddNotificationType(IDirectMusicTrack8 *iface,
        REFGUID rguidNotificationType)
{
        IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_RemoveNotificationType(IDirectMusicTrack8 *iface,
        REFGUID rguidNotificationType)
{
        IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_Clone(IDirectMusicTrack8 *iface, MUSIC_TIME mtStart,
        MUSIC_TIME mtEnd, IDirectMusicTrack **ppTrack)
{
        IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %d, %d, %p): stub\n", This, mtStart, mtEnd, ppTrack);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_PlayEx(IDirectMusicTrack8 *iface, void *pStateData,
        REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
        IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, 0x%s, 0x%s, 0x%s, %d, %p, %p, %d): stub\n", This, pStateData, wine_dbgstr_longlong(rtStart),
	    wine_dbgstr_longlong(rtEnd), wine_dbgstr_longlong(rtOffset), dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_GetParamEx(IDirectMusicTrack8 *iface, REFGUID type,
        REFERENCE_TIME time, REFERENCE_TIME *next, void *param, void *state, DWORD flags)
{
    IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %s, %p, %p, %p, %x): method not implemented\n", This, debugstr_dmguid(type),
            wine_dbgstr_longlong(time), next, param, state, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_SetParamEx(IDirectMusicTrack8 *iface, REFGUID type,
        REFERENCE_TIME time, void *param, void *state, DWORD flags)
{
    IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %s, %p, %p, %x): method not implemented\n", This, debugstr_dmguid(type),
            wine_dbgstr_longlong(time), param, state, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_Compose(IDirectMusicTrack8 *iface, IUnknown *pContext,
        DWORD dwTrackGroup, IDirectMusicTrack **ppResultTrack)
{
        IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %d, %p): stub\n", This, pContext, dwTrackGroup, ppResultTrack);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_Join(IDirectMusicTrack8 *iface,
        IDirectMusicTrack *pNewTrack, MUSIC_TIME mtJoin, IUnknown *pContext, DWORD dwTrackGroup,
        IDirectMusicTrack **ppResultTrack)
{
        IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %d, %p, %d, %p): stub\n", This, pNewTrack, mtJoin, pContext, dwTrackGroup, ppResultTrack);
	return S_OK;
}

static const IDirectMusicTrack8Vtbl dmtrack8_vtbl = {
    IDirectMusicTrack8Impl_QueryInterface,
    IDirectMusicTrack8Impl_AddRef,
    IDirectMusicTrack8Impl_Release,
    IDirectMusicTrack8Impl_Init,
    IDirectMusicTrack8Impl_InitPlay,
    IDirectMusicTrack8Impl_EndPlay,
    IDirectMusicTrack8Impl_Play,
    IDirectMusicTrack8Impl_GetParam,
    IDirectMusicTrack8Impl_SetParam,
    IDirectMusicTrack8Impl_IsParamSupported,
    IDirectMusicTrack8Impl_AddNotificationType,
    IDirectMusicTrack8Impl_RemoveNotificationType,
    IDirectMusicTrack8Impl_Clone,
    IDirectMusicTrack8Impl_PlayEx,
    IDirectMusicTrack8Impl_GetParamEx,
    IDirectMusicTrack8Impl_SetParamEx,
    IDirectMusicTrack8Impl_Compose,
    IDirectMusicTrack8Impl_Join
};

/* IDirectMusicSignPostTrack IPersistStream part: */
static HRESULT WINAPI IDirectMusicSignPostTrack_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicSignPostTrack, PersistStreamVtbl, iface);
	return IDirectMusicSignPostTrack_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicSignPostTrack_IPersistStream_AddRef (LPPERSISTSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicSignPostTrack, PersistStreamVtbl, iface);
	return IDirectMusicSignPostTrack_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

static ULONG WINAPI IDirectMusicSignPostTrack_IPersistStream_Release (LPPERSISTSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicSignPostTrack, PersistStreamVtbl, iface);
	return IDirectMusicSignPostTrack_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

static HRESULT WINAPI IDirectMusicSignPostTrack_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID) {
	return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicSignPostTrack_IPersistStream_IsDirty (LPPERSISTSTREAM iface) {
	return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicSignPostTrack_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm) {
	ICOM_THIS_MULTI(IDirectMusicSignPostTrack, PersistStreamVtbl, iface);
	FIXME("(%p, %p): Loading not implemented yet\n", This, pStm);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSignPostTrack_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty) {
	return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicSignPostTrack_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize) {
	return E_NOTIMPL;
}

static const IPersistStreamVtbl DirectMusicSignPostTrack_PersistStream_Vtbl =
{
	IDirectMusicSignPostTrack_IPersistStream_QueryInterface,
	IDirectMusicSignPostTrack_IPersistStream_AddRef,
	IDirectMusicSignPostTrack_IPersistStream_Release,
	IDirectMusicSignPostTrack_IPersistStream_GetClassID,
	IDirectMusicSignPostTrack_IPersistStream_IsDirty,
	IDirectMusicSignPostTrack_IPersistStream_Load,
	IDirectMusicSignPostTrack_IPersistStream_Save,
	IDirectMusicSignPostTrack_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT WINAPI create_dmsignposttrack(REFIID lpcGUID, void **ppobj)
{
	IDirectMusicSignPostTrack* track;
	
	track = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicSignPostTrack));
	if (NULL == track) {
		*ppobj = NULL;
		return E_OUTOFMEMORY;
	}
	track->UnknownVtbl = &DirectMusicSignPostTrack_Unknown_Vtbl;
    track->IDirectMusicTrack8_iface.lpVtbl = &dmtrack8_vtbl;
	track->PersistStreamVtbl = &DirectMusicSignPostTrack_PersistStream_Vtbl;
	track->pDesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DMUS_OBJECTDESC));
	DM_STRUCT_INIT(track->pDesc);
	track->pDesc->dwValidData |= DMUS_OBJ_CLASS;
	track->pDesc->guidClass = CLSID_DirectMusicSignPostTrack;
	track->ref = 0; /* will be inited by QueryInterface */

	return IDirectMusicSignPostTrack_IUnknown_QueryInterface ((LPUNKNOWN)&track->UnknownVtbl, lpcGUID, ppobj);
}
