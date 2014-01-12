/* IDirectMusicScriptTrack Implementation
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

#include "dmscript_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmscript);

/*****************************************************************************
 * IDirectMusicScriptTrack implementation
 */

/* IDirectMusicScriptTrack IUnknown part: */
static HRESULT WINAPI IDirectMusicScriptTrack_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicScriptTrack, UnknownVtbl, iface);

	TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);
	if (IsEqualIID (riid, &IID_IUnknown)) {
		*ppobj = &This->UnknownVtbl;
		IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IDirectMusicTrack)
	  || IsEqualIID (riid, &IID_IDirectMusicTrack8)) {
		*ppobj = (LPDIRECTMUSICTRACK8)&This->TrackVtbl;
		IDirectMusicTrack_AddRef ((LPDIRECTMUSICTRACK8)&This->TrackVtbl);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IPersistStream)) {
		*ppobj = &This->PersistStreamVtbl;
		IPersistStream_AddRef ((LPPERSISTSTREAM)&This->PersistStreamVtbl);
		return S_OK;
	}
	
	WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicScriptTrack_IUnknown_AddRef (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicScriptTrack, UnknownVtbl, iface);
        ULONG ref = InterlockedIncrement(&This->ref);

	TRACE("(%p): AddRef from %d\n", This, ref - 1);

	DMSCRIPT_LockModule();

	return ref;
}

static ULONG WINAPI IDirectMusicScriptTrack_IUnknown_Release (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicScriptTrack, UnknownVtbl, iface);
	ULONG ref = InterlockedDecrement(&This->ref);

	TRACE("(%p): ReleaseRef to %d\n", This, ref);

	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}

	DMSCRIPT_UnlockModule();

	return ref;
}

static const IUnknownVtbl DirectMusicScriptTrack_Unknown_Vtbl = {
	IDirectMusicScriptTrack_IUnknown_QueryInterface,
	IDirectMusicScriptTrack_IUnknown_AddRef,
	IDirectMusicScriptTrack_IUnknown_Release
};

/* IDirectMusicScriptTrack IDirectMusicTrack8 part: */
static HRESULT WINAPI IDirectMusicTrack8Impl_QueryInterface(IDirectMusicTrack8 *iface, REFIID riid,
        void **ppobj)
{
	ICOM_THIS_MULTI(IDirectMusicScriptTrack, TrackVtbl, iface);
	return IDirectMusicScriptTrack_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicTrack8Impl_AddRef(IDirectMusicTrack8 *iface)
{
	ICOM_THIS_MULTI(IDirectMusicScriptTrack, TrackVtbl, iface);
	return IDirectMusicScriptTrack_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

static ULONG WINAPI IDirectMusicTrack8Impl_Release(IDirectMusicTrack8 *iface)
{
	ICOM_THIS_MULTI(IDirectMusicScriptTrack, TrackVtbl, iface);
	return IDirectMusicScriptTrack_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

static HRESULT WINAPI IDirectMusicTrack8Impl_Init(IDirectMusicTrack8 *iface,
        IDirectMusicSegment *pSegment)
{
	ICOM_THIS_MULTI(IDirectMusicScriptTrack, TrackVtbl, iface);
	FIXME("(%p, %p): stub\n", This, pSegment);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_InitPlay(IDirectMusicTrack8 *iface,
        IDirectMusicSegmentState *pSegmentState, IDirectMusicPerformance *pPerformance,
        void **ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
	ICOM_THIS_MULTI(IDirectMusicScriptTrack, TrackVtbl, iface);
	FIXME("(%p, %p, %p, %p, %d, %d): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_EndPlay(IDirectMusicTrack8 *iface, void *pStateData)
{
	ICOM_THIS_MULTI(IDirectMusicScriptTrack, TrackVtbl, iface);
	FIXME("(%p, %p): stub\n", This, pStateData);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_Play(IDirectMusicTrack8 *iface, void *pStateData,
        MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
	ICOM_THIS_MULTI(IDirectMusicScriptTrack, TrackVtbl, iface);
	FIXME("(%p, %p, %d, %d, %d, %d, %p, %p, %d): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_GetParam(IDirectMusicTrack8 *iface, REFGUID rguidType,
        MUSIC_TIME mtTime, MUSIC_TIME *pmtNext, void *pParam)
{
	ICOM_THIS_MULTI(IDirectMusicScriptTrack, TrackVtbl, iface);
	FIXME("(%p, %s, %d, %p, %p): stub\n", This, debugstr_dmguid(rguidType), mtTime, pmtNext, pParam);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_SetParam(IDirectMusicTrack8 *iface, REFGUID rguidType,
        MUSIC_TIME mtTime, void *pParam)
{
	ICOM_THIS_MULTI(IDirectMusicScriptTrack, TrackVtbl, iface);
	FIXME("(%p, %s, %d, %p): stub\n", This, debugstr_dmguid(rguidType), mtTime, pParam);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_IsParamSupported(IDirectMusicTrack8 *iface,
        REFGUID rguidType)
{
	ICOM_THIS_MULTI(IDirectMusicScriptTrack, TrackVtbl, iface);

	TRACE("(%p, %s)\n", This, debugstr_dmguid(rguidType));
	/* didn't find any params */
	TRACE("param unsupported\n");
	return DMUS_E_TYPE_UNSUPPORTED;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_AddNotificationType(IDirectMusicTrack8 *iface,
        REFGUID rguidNotificationType)
{
	ICOM_THIS_MULTI(IDirectMusicScriptTrack, TrackVtbl, iface);
	FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_RemoveNotificationType(IDirectMusicTrack8 *iface,
        REFGUID rguidNotificationType)
{
	ICOM_THIS_MULTI(IDirectMusicScriptTrack, TrackVtbl, iface);
	FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_Clone(IDirectMusicTrack8 *iface, MUSIC_TIME mtStart,
        MUSIC_TIME mtEnd, IDirectMusicTrack **ppTrack)
{
	ICOM_THIS_MULTI(IDirectMusicScriptTrack, TrackVtbl, iface);
	FIXME("(%p, %d, %d, %p): stub\n", This, mtStart, mtEnd, ppTrack);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_PlayEx(IDirectMusicTrack8 *iface, void *pStateData,
        REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
	ICOM_THIS_MULTI(IDirectMusicScriptTrack, TrackVtbl, iface);
	FIXME("(%p, %p, 0x%s, 0x%s, 0x%s, %d, %p, %p, %d): stub\n", This, pStateData, wine_dbgstr_longlong(rtStart),
	    wine_dbgstr_longlong(rtEnd), wine_dbgstr_longlong(rtOffset), dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_GetParamEx(IDirectMusicTrack8 *iface,
        REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME *prtNext, void *pParam,
        void *pStateData, DWORD dwFlags)
{
	ICOM_THIS_MULTI(IDirectMusicScriptTrack, TrackVtbl, iface);
	FIXME("(%p, %s, 0x%s, %p, %p, %p, %d): stub\n", This, debugstr_dmguid(rguidType),
	    wine_dbgstr_longlong(rtTime), prtNext, pParam, pStateData, dwFlags);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_SetParamEx(IDirectMusicTrack8 *iface,
        REFGUID rguidType, REFERENCE_TIME rtTime, void *pParam, void *pStateData, DWORD dwFlags)
{
	ICOM_THIS_MULTI(IDirectMusicScriptTrack, TrackVtbl, iface);
	FIXME("(%p, %s, 0x%s, %p, %p, %d): stub\n", This, debugstr_dmguid(rguidType),
	    wine_dbgstr_longlong(rtTime), pParam, pStateData, dwFlags);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_Compose(IDirectMusicTrack8 *iface, IUnknown *pContext,
        DWORD dwTrackGroup, IDirectMusicTrack **ppResultTrack)
{
	ICOM_THIS_MULTI(IDirectMusicScriptTrack, TrackVtbl, iface);
	FIXME("(%p, %p, %d, %p): stub\n", This, pContext, dwTrackGroup, ppResultTrack);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_Join(IDirectMusicTrack8 *iface,
        IDirectMusicTrack *pNewTrack, MUSIC_TIME mtJoin, IUnknown *pContext, DWORD dwTrackGroup,
        IDirectMusicTrack **ppResultTrack)
{
	ICOM_THIS_MULTI(IDirectMusicScriptTrack, TrackVtbl, iface);
	FIXME("(%p, %p, %d, %p, %d, %p): stub\n", This, pNewTrack, mtJoin, pContext, dwTrackGroup, ppResultTrack);
	return S_OK;
}

static const IDirectMusicTrack8Vtbl DirectMusicScriptTrack_Track_Vtbl = {
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

/* IDirectMusicScriptTrack IPersistStream part: */
static HRESULT WINAPI IPersistStreamImpl_QueryInterface(IPersistStream *iface, REFIID riid,
        void **ppobj)
{
	ICOM_THIS_MULTI(IDirectMusicScriptTrack, PersistStreamVtbl, iface);
	return IDirectMusicScriptTrack_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

static ULONG WINAPI IPersistStreamImpl_AddRef(IPersistStream *iface)
{
	ICOM_THIS_MULTI(IDirectMusicScriptTrack, PersistStreamVtbl, iface);
	return IDirectMusicScriptTrack_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

static ULONG WINAPI IPersistStreamImpl_Release(IPersistStream *iface)
{
	ICOM_THIS_MULTI(IDirectMusicScriptTrack, PersistStreamVtbl, iface);
	return IDirectMusicScriptTrack_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

static HRESULT WINAPI IPersistStreamImpl_GetClassID(IPersistStream *iface, CLSID *pClassID)
{
	return E_NOTIMPL;
}

static HRESULT WINAPI IPersistStreamImpl_IsDirty(IPersistStream *iface)
{
	return E_NOTIMPL;
}

static HRESULT WINAPI IPersistStreamImpl_Load(IPersistStream *iface, IStream *pStm)
{
	FIXME(": Loading not implemented yet\n");
	return S_OK;
}

static HRESULT WINAPI IPersistStreamImpl_Save(IPersistStream *iface, IStream *pStm,
        BOOL fClearDirty)
{
	return E_NOTIMPL;
}

static HRESULT WINAPI IPersistStreamImpl_GetSizeMax(IPersistStream *iface, ULARGE_INTEGER *pcbSize)
{
	return E_NOTIMPL;
}

static const IPersistStreamVtbl DirectMusicScriptTrack_PersistStream_Vtbl = {
    IPersistStreamImpl_QueryInterface,
    IPersistStreamImpl_AddRef,
    IPersistStreamImpl_Release,
    IPersistStreamImpl_GetClassID,
    IPersistStreamImpl_IsDirty,
    IPersistStreamImpl_Load,
    IPersistStreamImpl_Save,
    IPersistStreamImpl_GetSizeMax
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicScriptTrack (LPCGUID lpcGUID, LPVOID *ppobj, LPUNKNOWN pUnkOuter) {
	IDirectMusicScriptTrack* track;
	
	track = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicScriptTrack));
	if (NULL == track) {
		*ppobj = NULL;
		return E_OUTOFMEMORY;
	}
	track->UnknownVtbl = &DirectMusicScriptTrack_Unknown_Vtbl;
	track->TrackVtbl = &DirectMusicScriptTrack_Track_Vtbl;
	track->PersistStreamVtbl = &DirectMusicScriptTrack_PersistStream_Vtbl;
	track->pDesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DMUS_OBJECTDESC));
	DM_STRUCT_INIT(track->pDesc);
	track->pDesc->dwValidData |= DMUS_OBJ_CLASS;
	track->pDesc->guidClass = CLSID_DirectMusicScriptTrack;
	track->ref = 0; /* will be inited by QueryInterface */

	return IDirectMusicScriptTrack_IUnknown_QueryInterface ((LPUNKNOWN)&track->UnknownVtbl, lpcGUID, ppobj);
}
