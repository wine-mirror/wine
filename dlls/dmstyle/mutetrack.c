/* IDirectMusicMuteTrack Implementation
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

static ULONG WINAPI IDirectMusicMuteTrack_IUnknown_AddRef (LPUNKNOWN iface);
static ULONG WINAPI IDirectMusicMuteTrack_IPersistStream_AddRef (LPPERSISTSTREAM iface);

/*****************************************************************************
 * IDirectMusicMuteTrack implementation
 */
typedef struct IDirectMusicMuteTrack {
    const IUnknownVtbl *UnknownVtbl;
    IDirectMusicTrack8 IDirectMusicTrack8_iface;
    const IPersistStreamVtbl *PersistStreamVtbl;
    LONG ref;
    DMUS_OBJECTDESC *pDesc;
} IDirectMusicMuteTrack;

/* IDirectMusicMuteTrack IUnknown part: */
static HRESULT WINAPI IDirectMusicMuteTrack_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicMuteTrack, UnknownVtbl, iface);
	TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);

	if (IsEqualIID (riid, &IID_IUnknown)) {
		*ppobj = &This->UnknownVtbl;
		IDirectMusicMuteTrack_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IDirectMusicTrack)
	  || IsEqualIID (riid, &IID_IDirectMusicTrack8)) {
		*ppobj = &This->IDirectMusicTrack8_iface;
		IDirectMusicTrack8_AddRef (&This->IDirectMusicTrack8_iface);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IPersistStream)) {
		*ppobj = &This->PersistStreamVtbl;
		IDirectMusicMuteTrack_IPersistStream_AddRef ((LPPERSISTSTREAM)&This->PersistStreamVtbl);
		return S_OK;
	}
	
	WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicMuteTrack_IUnknown_AddRef (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicMuteTrack, UnknownVtbl, iface);
        ULONG ref = InterlockedIncrement(&This->ref);

	TRACE("(%p): AddRef from %d\n", This, ref - 1);

	DMSTYLE_LockModule();

	return ref;
}

static ULONG WINAPI IDirectMusicMuteTrack_IUnknown_Release (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicMuteTrack, UnknownVtbl, iface);
	ULONG ref = InterlockedDecrement(&This->ref);

	TRACE("(%p): ReleaseRef to %d\n", This, ref);

	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	
	DMSTYLE_UnlockModule();
	
	return ref;
}

static const IUnknownVtbl DirectMusicMuteTrack_Unknown_Vtbl = {
	IDirectMusicMuteTrack_IUnknown_QueryInterface,
	IDirectMusicMuteTrack_IUnknown_AddRef,
	IDirectMusicMuteTrack_IUnknown_Release
};

/* IDirectMusicMuteTrack IDirectMusicTrack8 part: */
static inline IDirectMusicMuteTrack *impl_from_IDirectMusicTrack8(IDirectMusicTrack8 *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicMuteTrack, IDirectMusicTrack8_iface);
}

static HRESULT WINAPI IDirectMusicTrack8Impl_QueryInterface(IDirectMusicTrack8 *iface, REFIID riid,
        void **ppobj)
{
    IDirectMusicMuteTrack *This = impl_from_IDirectMusicTrack8(iface);
	return IDirectMusicMuteTrack_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicTrack8Impl_AddRef(IDirectMusicTrack8 *iface)
{
    IDirectMusicMuteTrack *This = impl_from_IDirectMusicTrack8(iface);
	return IDirectMusicMuteTrack_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

static ULONG WINAPI IDirectMusicTrack8Impl_Release(IDirectMusicTrack8 *iface)
{
    IDirectMusicMuteTrack *This = impl_from_IDirectMusicTrack8(iface);
	return IDirectMusicMuteTrack_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

static HRESULT WINAPI IDirectMusicTrack8Impl_Init(IDirectMusicTrack8 *iface,
        IDirectMusicSegment *pSegment)
{
        IDirectMusicMuteTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p): stub\n", This, pSegment);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_InitPlay(IDirectMusicTrack8 *iface,
        IDirectMusicSegmentState *pSegmentState, IDirectMusicPerformance *pPerformance,
        void **ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
        IDirectMusicMuteTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %p, %p, %d, %d): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_EndPlay(IDirectMusicTrack8 *iface, void *pStateData)
{
        IDirectMusicMuteTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p): stub\n", This, pStateData);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_Play(IDirectMusicTrack8 *iface, void *pStateData,
        MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
        IDirectMusicMuteTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %d, %d, %d, %d, %p, %p, %d): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_GetParam(IDirectMusicTrack8 *iface, REFGUID rguidType,
        MUSIC_TIME mtTime, MUSIC_TIME *pmtNext, void *pParam)
{
        IDirectMusicMuteTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s, %d, %p, %p): stub\n", This, debugstr_dmguid(rguidType), mtTime, pmtNext, pParam);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_SetParam(IDirectMusicTrack8 *iface, REFGUID rguidType,
        MUSIC_TIME mtTime, void *pParam)
{
        IDirectMusicMuteTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s, %d, %p): stub\n", This, debugstr_dmguid(rguidType), mtTime, pParam);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_IsParamSupported(IDirectMusicTrack8 *iface,
        REFGUID rguidType)
{
        IDirectMusicMuteTrack *This = impl_from_IDirectMusicTrack8(iface);

	TRACE("(%p, %s)\n", This, debugstr_dmguid(rguidType));

        if (!rguidType)
                return E_POINTER;

	if (IsEqualGUID (rguidType, &GUID_MuteParam)) {
		TRACE("param supported\n");
		return S_OK;
	}
	TRACE("param unsupported\n");
	return DMUS_E_TYPE_UNSUPPORTED;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_AddNotificationType(IDirectMusicTrack8 *iface,
        REFGUID notiftype)
{
    IDirectMusicMuteTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): method not implemented\n", This, debugstr_dmguid(notiftype));
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_RemoveNotificationType(IDirectMusicTrack8 *iface,
        REFGUID notiftype)
{
    IDirectMusicMuteTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): method not implemented\n", This, debugstr_dmguid(notiftype));
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_Clone(IDirectMusicTrack8 *iface, MUSIC_TIME mtStart,
        MUSIC_TIME mtEnd, IDirectMusicTrack **ppTrack)
{
        IDirectMusicMuteTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %d, %d, %p): stub\n", This, mtStart, mtEnd, ppTrack);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_PlayEx(IDirectMusicTrack8 *iface, void *pStateData,
        REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
        IDirectMusicMuteTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, 0x%s, 0x%s, 0x%s, %d, %p, %p, %d): stub\n", This, pStateData, wine_dbgstr_longlong(rtStart),
	    wine_dbgstr_longlong(rtEnd), wine_dbgstr_longlong(rtOffset), dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_GetParamEx(IDirectMusicTrack8 *iface,
        REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME *prtNext, void *pParam,
        void *pStateData, DWORD dwFlags)
{
        IDirectMusicMuteTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s, 0x%s, %p, %p, %p, %d): stub\n", This, debugstr_dmguid(rguidType),
	    wine_dbgstr_longlong(rtTime), prtNext, pParam, pStateData, dwFlags);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_SetParamEx(IDirectMusicTrack8 *iface,
        REFGUID rguidType, REFERENCE_TIME rtTime, void *pParam, void *pStateData, DWORD dwFlags)
{
        IDirectMusicMuteTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s, 0x%s, %p, %p, %d): stub\n", This, debugstr_dmguid(rguidType),
	    wine_dbgstr_longlong(rtTime), pParam, pStateData, dwFlags);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_Compose(IDirectMusicTrack8 *iface, IUnknown *context,
        DWORD trackgroup, IDirectMusicTrack **track)
{
    IDirectMusicMuteTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %p, %d, %p): method not implemented\n", This, context, trackgroup, track);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_Join(IDirectMusicTrack8 *iface,
        IDirectMusicTrack *newtrack, MUSIC_TIME join, IUnknown *context, DWORD trackgroup,
        IDirectMusicTrack **resulttrack)
{
    IDirectMusicMuteTrack *This = impl_from_IDirectMusicTrack8(iface);
    TRACE("(%p, %p, %d, %p, %d, %p): stub\n", This, newtrack, join, context, trackgroup,
            resulttrack);
    return E_NOTIMPL;
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

/* IDirectMusicMuteTrack IPersistStream part: */
static HRESULT WINAPI IDirectMusicMuteTrack_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicMuteTrack, PersistStreamVtbl, iface);
	return IDirectMusicMuteTrack_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicMuteTrack_IPersistStream_AddRef (LPPERSISTSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicMuteTrack, PersistStreamVtbl, iface);
	return IDirectMusicMuteTrack_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

static ULONG WINAPI IDirectMusicMuteTrack_IPersistStream_Release (LPPERSISTSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicMuteTrack, PersistStreamVtbl, iface);
	return IDirectMusicMuteTrack_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

static HRESULT WINAPI IDirectMusicMuteTrack_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID) {
	return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicMuteTrack_IPersistStream_IsDirty (LPPERSISTSTREAM iface) {
	return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicMuteTrack_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm) {
	FIXME(": Loading not implemented yet\n");
	return S_OK;
}

static HRESULT WINAPI IDirectMusicMuteTrack_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty) {
	return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicMuteTrack_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize) {
	return E_NOTIMPL;
}

static const IPersistStreamVtbl DirectMusicMuteTrack_PersistStream_Vtbl = {
	IDirectMusicMuteTrack_IPersistStream_QueryInterface,
	IDirectMusicMuteTrack_IPersistStream_AddRef,
	IDirectMusicMuteTrack_IPersistStream_Release,
	IDirectMusicMuteTrack_IPersistStream_GetClassID,
	IDirectMusicMuteTrack_IPersistStream_IsDirty,
	IDirectMusicMuteTrack_IPersistStream_Load,
	IDirectMusicMuteTrack_IPersistStream_Save,
	IDirectMusicMuteTrack_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT WINAPI create_dmmutetrack(REFIID lpcGUID, void **ppobj)
{
	IDirectMusicMuteTrack* track;
	
	track = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicMuteTrack));
	if (NULL == track) {
		*ppobj = NULL;
		return E_OUTOFMEMORY;
	}
	track->UnknownVtbl = &DirectMusicMuteTrack_Unknown_Vtbl;
    track->IDirectMusicTrack8_iface.lpVtbl = &dmtrack8_vtbl;
	track->PersistStreamVtbl = &DirectMusicMuteTrack_PersistStream_Vtbl;
	track->pDesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DMUS_OBJECTDESC));
	DM_STRUCT_INIT(track->pDesc);
	track->pDesc->dwValidData |= DMUS_OBJ_CLASS;
	track->pDesc->guidClass = CLSID_DirectMusicMuteTrack;
	track->ref = 0; /* will be inited by QueryInterface */

	return IDirectMusicMuteTrack_IUnknown_QueryInterface ((LPUNKNOWN)&track->UnknownVtbl, lpcGUID, ppobj);
}
