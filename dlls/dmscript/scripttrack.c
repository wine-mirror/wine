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

typedef struct DirectMusicScriptTrack {
    IDirectMusicTrack8 IDirectMusicTrack8_iface;
    IPersistStream IPersistStream_iface;
    LONG ref;
    DMUS_OBJECTDESC desc;
} DirectMusicScriptTrack;

static inline DirectMusicScriptTrack *impl_from_IDirectMusicTrack8(IDirectMusicTrack8 *iface)
{
    return CONTAINING_RECORD(iface, DirectMusicScriptTrack, IDirectMusicTrack8_iface);
}

static inline DirectMusicScriptTrack *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, DirectMusicScriptTrack, IPersistStream_iface);
}

/* DirectMusicScriptTrack IDirectMusicTrack8 part: */
static HRESULT WINAPI IDirectMusicTrack8Impl_QueryInterface(IDirectMusicTrack8 *iface, REFIID riid,
        void **ret_iface)
{
    DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ret_iface);

    *ret_iface = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDirectMusicTrack) ||
            IsEqualIID(riid, &IID_IDirectMusicTrack8))
        *ret_iface = iface;
    else if (IsEqualIID(riid, &IID_IPersistStream))
        *ret_iface = &This->IPersistStream_iface;
    else {
        WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ret_iface);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ret_iface);
    return S_OK;
}

static ULONG WINAPI IDirectMusicTrack8Impl_AddRef(IDirectMusicTrack8 *iface)
{
    DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirectMusicTrack8Impl_Release(IDirectMusicTrack8 *iface)
{
    DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if (!ref) {
        HeapFree(GetProcessHeap(), 0, This);
        DMSCRIPT_UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_Init(IDirectMusicTrack8 *iface,
        IDirectMusicSegment *pSegment)
{
	DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p): stub\n", This, pSegment);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_InitPlay(IDirectMusicTrack8 *iface,
        IDirectMusicSegmentState *pSegmentState, IDirectMusicPerformance *pPerformance,
        void **ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
	DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %p, %p, %d, %d): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_EndPlay(IDirectMusicTrack8 *iface, void *pStateData)
{
	DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p): stub\n", This, pStateData);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_Play(IDirectMusicTrack8 *iface, void *pStateData,
        MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
	DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %d, %d, %d, %d, %p, %p, %d): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_GetParam(IDirectMusicTrack8 *iface, REFGUID rguidType,
        MUSIC_TIME mtTime, MUSIC_TIME *pmtNext, void *pParam)
{
	DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s, %d, %p, %p): stub\n", This, debugstr_dmguid(rguidType), mtTime, pmtNext, pParam);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_SetParam(IDirectMusicTrack8 *iface, REFGUID rguidType,
        MUSIC_TIME mtTime, void *pParam)
{
	DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s, %d, %p): stub\n", This, debugstr_dmguid(rguidType), mtTime, pParam);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_IsParamSupported(IDirectMusicTrack8 *iface,
        REFGUID rguidType)
{
	DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);

	TRACE("(%p, %s)\n", This, debugstr_dmguid(rguidType));
	/* didn't find any params */
	TRACE("param unsupported\n");
	return DMUS_E_TYPE_UNSUPPORTED;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_AddNotificationType(IDirectMusicTrack8 *iface,
        REFGUID rguidNotificationType)
{
	DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_RemoveNotificationType(IDirectMusicTrack8 *iface,
        REFGUID rguidNotificationType)
{
	DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_Clone(IDirectMusicTrack8 *iface, MUSIC_TIME mtStart,
        MUSIC_TIME mtEnd, IDirectMusicTrack **ppTrack)
{
	DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %d, %d, %p): stub\n", This, mtStart, mtEnd, ppTrack);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_PlayEx(IDirectMusicTrack8 *iface, void *pStateData,
        REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
	DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, 0x%s, 0x%s, 0x%s, %d, %p, %p, %d): stub\n", This, pStateData, wine_dbgstr_longlong(rtStart),
	    wine_dbgstr_longlong(rtEnd), wine_dbgstr_longlong(rtOffset), dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_GetParamEx(IDirectMusicTrack8 *iface,
        REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME *prtNext, void *pParam,
        void *pStateData, DWORD dwFlags)
{
	DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s, 0x%s, %p, %p, %p, %d): stub\n", This, debugstr_dmguid(rguidType),
	    wine_dbgstr_longlong(rtTime), prtNext, pParam, pStateData, dwFlags);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_SetParamEx(IDirectMusicTrack8 *iface,
        REFGUID rguidType, REFERENCE_TIME rtTime, void *pParam, void *pStateData, DWORD dwFlags)
{
	DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s, 0x%s, %p, %p, %d): stub\n", This, debugstr_dmguid(rguidType),
	    wine_dbgstr_longlong(rtTime), pParam, pStateData, dwFlags);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_Compose(IDirectMusicTrack8 *iface, IUnknown *pContext,
        DWORD dwTrackGroup, IDirectMusicTrack **ppResultTrack)
{
	DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %d, %p): stub\n", This, pContext, dwTrackGroup, ppResultTrack);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrack8Impl_Join(IDirectMusicTrack8 *iface,
        IDirectMusicTrack *pNewTrack, MUSIC_TIME mtJoin, IUnknown *pContext, DWORD dwTrackGroup,
        IDirectMusicTrack **ppResultTrack)
{
	DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);
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

/* IDirectMusicScriptTrack IPersistStream part: */
static HRESULT WINAPI IPersistStreamImpl_QueryInterface(IPersistStream *iface, REFIID riid,
        void **ret_iface)
{
    DirectMusicScriptTrack *This = impl_from_IPersistStream(iface);
    return IDirectMusicTrack8_QueryInterface(&This->IDirectMusicTrack8_iface, riid, ret_iface);
}

static ULONG WINAPI IPersistStreamImpl_AddRef(IPersistStream *iface)
{
    DirectMusicScriptTrack *This = impl_from_IPersistStream(iface);
    return IDirectMusicTrack8_AddRef(&This->IDirectMusicTrack8_iface);
}

static ULONG WINAPI IPersistStreamImpl_Release(IPersistStream *iface)
{
    DirectMusicScriptTrack *This = impl_from_IPersistStream(iface);
    return IDirectMusicTrack8_Release(&This->IDirectMusicTrack8_iface);
}

static HRESULT WINAPI IPersistStreamImpl_GetClassID(IPersistStream *iface, CLSID *pClassID)
{
    DirectMusicScriptTrack *This = impl_from_IPersistStream(iface);

    TRACE("(%p, %p)\n", This, pClassID);

    if (!pClassID)
        return E_POINTER;

    *pClassID = This->desc.guidClass;
    return S_OK;
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

static const IPersistStreamVtbl persist_vtbl = {
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
HRESULT WINAPI DMUSIC_CreateDirectMusicScriptTrack(REFIID riid, void **ret_iface,
        IUnknown *pUnkOuter)
{
    DirectMusicScriptTrack *track;
    HRESULT hr;

    *ret_iface = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    track = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*track));
    if (!track)
        return E_OUTOFMEMORY;

    track->IDirectMusicTrack8_iface.lpVtbl = &dmtrack8_vtbl;
    track->IPersistStream_iface.lpVtbl = &persist_vtbl;
    track->desc.dwSize = sizeof(track->desc);
    track->desc.dwValidData = DMUS_OBJ_CLASS;
    track->desc.guidClass = CLSID_DirectMusicScriptTrack;
    track->ref = 1;

    DMSCRIPT_LockModule();
    hr = IDirectMusicTrack8_QueryInterface(&track->IDirectMusicTrack8_iface, riid, ret_iface);
    IDirectMusicTrack8_Release(&track->IDirectMusicTrack8_iface);

    return hr;
}
