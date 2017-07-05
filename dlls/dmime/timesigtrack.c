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
#include "dmobject.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmime);

/*****************************************************************************
 * IDirectMusicTimeSigTrack implementation
 */
typedef struct IDirectMusicTimeSigTrack {
    IDirectMusicTrack IDirectMusicTrack_iface;
    struct dmobject dmobj;  /* IPersistStream only */
    LONG ref;
} IDirectMusicTimeSigTrack;

/* IDirectMusicTimeSigTrack IDirectMusicTrack8 part: */
static inline IDirectMusicTimeSigTrack *impl_from_IDirectMusicTrack(IDirectMusicTrack *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicTimeSigTrack, IDirectMusicTrack_iface);
}

static HRESULT WINAPI IDirectMusicTrackImpl_QueryInterface(IDirectMusicTrack *iface, REFIID riid,
        void **ret_iface)
{
    IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ret_iface);

    *ret_iface = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDirectMusicTrack))
        *ret_iface = iface;
    else if (IsEqualIID(riid, &IID_IPersistStream))
        *ret_iface = &This->dmobj.IPersistStream_iface;
    else {
        WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ret_iface);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ret_iface);
    return S_OK;
}

static ULONG WINAPI IDirectMusicTrackImpl_AddRef(IDirectMusicTrack *iface)
{
    IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirectMusicTrackImpl_Release(IDirectMusicTrack *iface)
{
    IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if (!ref) {
        HeapFree(GetProcessHeap(), 0, This);
        DMIME_UnlockModule();
    }

    return ref;
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

static HRESULT WINAPI time_IPersistStream_Load(IPersistStream *iface, IStream *stream)
{
	FIXME(": Loading not implemented yet\n");
	return S_OK;
}

static const IPersistStreamVtbl persiststream_vtbl = {
    dmobj_IPersistStream_QueryInterface,
    dmobj_IPersistStream_AddRef,
    dmobj_IPersistStream_Release,
    dmobj_IPersistStream_GetClassID,
    unimpl_IPersistStream_IsDirty,
    time_IPersistStream_Load,
    unimpl_IPersistStream_Save,
    unimpl_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT WINAPI create_dmtimesigtrack(REFIID lpcGUID, void **ppobj)
{
    IDirectMusicTimeSigTrack *track;
    HRESULT hr;

    track = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*track));
    if (!track) {
        *ppobj = NULL;
        return E_OUTOFMEMORY;
    }
    track->IDirectMusicTrack_iface.lpVtbl = &dmtack_vtbl;
    track->ref = 1;
    dmobject_init(&track->dmobj, &CLSID_DirectMusicTimeSigTrack,
                  (IUnknown *)&track->IDirectMusicTrack_iface);
    track->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;

    DMIME_LockModule();
    hr = IDirectMusicTrack_QueryInterface(&track->IDirectMusicTrack_iface, lpcGUID, ppobj);
    IDirectMusicTrack_Release(&track->IDirectMusicTrack_iface);

    return hr;
}
