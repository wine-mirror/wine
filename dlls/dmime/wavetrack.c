/* IDirectMusicWaveTrack Implementation
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
 * IDirectMusicWaveTrack implementation
 */
/*****************************************************************************
 * IDirectMusicChordMapTrack implementation
 */
typedef struct IDirectMusicWaveTrack {
    IDirectMusicTrack8 IDirectMusicTrack8_iface;
    struct dmobject dmobj;  /* IPersistStream only */
    LONG ref;
} IDirectMusicWaveTrack;

/* IDirectMusicWaveTrack IDirectMusicTrack8 part: */
static inline IDirectMusicWaveTrack *impl_from_IDirectMusicTrack8(IDirectMusicTrack8 *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicWaveTrack, IDirectMusicTrack8_iface);
}

static HRESULT WINAPI wave_track_QueryInterface(IDirectMusicTrack8 *iface, REFIID riid,
        void **ret_iface)
{
    IDirectMusicWaveTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ret_iface);

    *ret_iface = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDirectMusicTrack) ||
            IsEqualIID(riid, &IID_IDirectMusicTrack8))
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

static ULONG WINAPI wave_track_AddRef(IDirectMusicTrack8 *iface)
{
    IDirectMusicWaveTrack *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI wave_track_Release(IDirectMusicTrack8 *iface)
{
    IDirectMusicWaveTrack *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if (!ref) {
        HeapFree(GetProcessHeap(), 0, This);
        DMIME_UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI wave_track_Init(IDirectMusicTrack8 *iface, IDirectMusicSegment *pSegment)
{
        IDirectMusicWaveTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p): stub\n", This, pSegment);
	return S_OK;
}

static HRESULT WINAPI wave_track_InitPlay(IDirectMusicTrack8 *iface,
        IDirectMusicSegmentState *pSegmentState, IDirectMusicPerformance *pPerformance,
        void **ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
        IDirectMusicWaveTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %p, %p, %d, %d): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);
	return S_OK;
}

static HRESULT WINAPI wave_track_EndPlay(IDirectMusicTrack8 *iface, void *pStateData)
{
        IDirectMusicWaveTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p): stub\n", This, pStateData);
	return S_OK;
}

static HRESULT WINAPI wave_track_Play(IDirectMusicTrack8 *iface, void *pStateData,
        MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
        IDirectMusicWaveTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %d, %d, %d, %d, %p, %p, %d): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI wave_track_GetParam(IDirectMusicTrack8 *iface, REFGUID type, MUSIC_TIME time,
        MUSIC_TIME *next, void *param)
{
    IDirectMusicWaveTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %d, %p, %p): not supported\n", This, debugstr_dmguid(type), time, next, param);
    return DMUS_E_GET_UNSUPPORTED;
}

static HRESULT WINAPI wave_track_SetParam(IDirectMusicTrack8 *iface, REFGUID type, MUSIC_TIME time,
        void *param)
{
    IDirectMusicWaveTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %d, %p)\n", This, debugstr_dmguid(type), time, param);

    if (IsEqualGUID(type, &GUID_Disable_Auto_Download)) {
        FIXME("GUID_Disable_Auto_Download not handled yet\n");
        return S_OK;
    }
    if (IsEqualGUID(type, &GUID_Download)) {
        FIXME("GUID_Download not handled yet\n");
        return S_OK;
    }
    if (IsEqualGUID(type, &GUID_DownloadToAudioPath)) {
        FIXME("GUID_DownloadToAudioPath not handled yet\n");
        return S_OK;
    }
    if (IsEqualGUID(type, &GUID_Enable_Auto_Download)) {
        FIXME("GUID_Enable_Auto_Download not handled yet\n");
        return S_OK;
    }
    if (IsEqualGUID(type, &GUID_Unload)) {
        FIXME("GUID_Unload not handled yet\n");
        return S_OK;
    }
    if (IsEqualGUID(type, &GUID_UnloadFromAudioPath)) {
        FIXME("GUID_UnloadFromAudioPath not handled yet\n");
        return S_OK;
    }

    return DMUS_E_TYPE_UNSUPPORTED;
}

static HRESULT WINAPI wave_track_IsParamSupported(IDirectMusicTrack8 *iface, REFGUID type)
{
    IDirectMusicWaveTrack *This = impl_from_IDirectMusicTrack8(iface);
    static const GUID *valid[] = {
        &GUID_Disable_Auto_Download,
        &GUID_Download,
        &GUID_DownloadToAudioPath,
        &GUID_Enable_Auto_Download,
        &GUID_Unload,
        &GUID_UnloadFromAudioPath
    };
    unsigned int i;

    TRACE("(%p, %s)\n", This, debugstr_dmguid(type));

    for (i = 0; i < ARRAY_SIZE(valid); i++)
        if (IsEqualGUID(type, valid[i]))
            return S_OK;

    TRACE("param unsupported\n");
    return DMUS_E_TYPE_UNSUPPORTED;
}

static HRESULT WINAPI wave_track_AddNotificationType(IDirectMusicTrack8 *iface, REFGUID notiftype)
{
    IDirectMusicWaveTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): method not implemented\n", This, debugstr_dmguid(notiftype));
    return E_NOTIMPL;
}

static HRESULT WINAPI wave_track_RemoveNotificationType(IDirectMusicTrack8 *iface,
        REFGUID notiftype)
{
    IDirectMusicWaveTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): method not implemented\n", This, debugstr_dmguid(notiftype));
    return E_NOTIMPL;
}

static HRESULT WINAPI wave_track_Clone(IDirectMusicTrack8 *iface, MUSIC_TIME mtStart,
        MUSIC_TIME mtEnd, IDirectMusicTrack **ppTrack)
{
        IDirectMusicWaveTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %d, %d, %p): stub\n", This, mtStart, mtEnd, ppTrack);
	return S_OK;
}

static HRESULT WINAPI wave_track_PlayEx(IDirectMusicTrack8 *iface, void *pStateData,
        REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
        IDirectMusicWaveTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, 0x%s, 0x%s, 0x%s, %d, %p, %p, %d): stub\n", This, pStateData, wine_dbgstr_longlong(rtStart),
	    wine_dbgstr_longlong(rtEnd), wine_dbgstr_longlong(rtOffset), dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI wave_track_GetParamEx(IDirectMusicTrack8 *iface, REFGUID rguidType,
        REFERENCE_TIME rtTime, REFERENCE_TIME *prtNext, void *pParam, void *pStateData,
        DWORD dwFlags)
{
        IDirectMusicWaveTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s, 0x%s, %p, %p, %p, %d): stub\n", This, debugstr_dmguid(rguidType),
	    wine_dbgstr_longlong(rtTime), prtNext, pParam, pStateData, dwFlags);
	return S_OK;
}

static HRESULT WINAPI wave_track_SetParamEx(IDirectMusicTrack8 *iface, REFGUID rguidType,
        REFERENCE_TIME rtTime, void *pParam, void *pStateData, DWORD dwFlags)
{
        IDirectMusicWaveTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s, 0x%s, %p, %p, %d): stub\n", This, debugstr_dmguid(rguidType),
	    wine_dbgstr_longlong(rtTime), pParam, pStateData, dwFlags);
	return S_OK;
}

static HRESULT WINAPI wave_track_Compose(IDirectMusicTrack8 *iface, IUnknown *context,
        DWORD trackgroup, IDirectMusicTrack **track)
{
    IDirectMusicWaveTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %p, %d, %p): method not implemented\n", This, context, trackgroup, track);
    return E_NOTIMPL;
}

static HRESULT WINAPI wave_track_Join(IDirectMusicTrack8 *iface, IDirectMusicTrack *newtrack,
        MUSIC_TIME join, IUnknown *context, DWORD trackgroup, IDirectMusicTrack **resulttrack)
{
    IDirectMusicWaveTrack *This = impl_from_IDirectMusicTrack8(iface);
    TRACE("(%p, %p, %d, %p, %d, %p): method not implemented\n", This, newtrack, join, context,
            trackgroup, resulttrack);
    return E_NOTIMPL;
}

static const IDirectMusicTrack8Vtbl dmtrack8_vtbl = {
    wave_track_QueryInterface,
    wave_track_AddRef,
    wave_track_Release,
    wave_track_Init,
    wave_track_InitPlay,
    wave_track_EndPlay,
    wave_track_Play,
    wave_track_GetParam,
    wave_track_SetParam,
    wave_track_IsParamSupported,
    wave_track_AddNotificationType,
    wave_track_RemoveNotificationType,
    wave_track_Clone,
    wave_track_PlayEx,
    wave_track_GetParamEx,
    wave_track_SetParamEx,
    wave_track_Compose,
    wave_track_Join
};

static HRESULT WINAPI wave_IPersistStream_Load(IPersistStream *iface, IStream *stream)
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
    wave_IPersistStream_Load,
    unimpl_IPersistStream_Save,
    unimpl_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT WINAPI create_dmwavetrack(REFIID lpcGUID, void **ppobj)
{
    IDirectMusicWaveTrack *track;
    HRESULT hr;

    track = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*track));
    if (!track) {
        *ppobj = NULL;
        return E_OUTOFMEMORY;
    }
    track->IDirectMusicTrack8_iface.lpVtbl = &dmtrack8_vtbl;
    track->ref = 1;
    dmobject_init(&track->dmobj, &CLSID_DirectMusicWaveTrack,
                  (IUnknown *)&track->IDirectMusicTrack8_iface);
    track->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;

    DMIME_LockModule();
    hr = IDirectMusicTrack8_QueryInterface(&track->IDirectMusicTrack8_iface, lpcGUID, ppobj);
    IDirectMusicTrack8_Release(&track->IDirectMusicTrack8_iface);

    return hr;
}
