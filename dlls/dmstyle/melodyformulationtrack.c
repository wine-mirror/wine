/* IDirectMusicMelodyFormulationTrack Implementation
 *
 * Copyright (C) 2003 Rok Mandeljc
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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "dmstyle_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmstyle);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

/*****************************************************************************
 * IDirectMusicMelodyFormulationTrack implementation
 */
/* IDirectMusicMelodyFormulationTrack IUnknown part: */
HRESULT WINAPI IDirectMusicMelodyFormulationTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicMelodyFormulationTrack,iface);

	if (IsEqualIID (riid, &IID_IUnknown) || 
	    IsEqualIID (riid, &IID_IDirectMusicTrack) ||
	    IsEqualIID (riid, &IID_IDirectMusicTrack8)) {
		IDirectMusicMelodyFormulationTrack_AddRef(iface);
		*ppobj = This;
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IPersistStream)) {
		IDirectMusicMelodyFormulationTrackStream_AddRef ((LPPERSISTSTREAM)This->pStream);
		*ppobj = This->pStream;
		return S_OK;
	}

	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicMelodyFormulationTrack_AddRef (LPDIRECTMUSICTRACK8 iface)
{
	ICOM_THIS(IDirectMusicMelodyFormulationTrack,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicMelodyFormulationTrack_Release (LPDIRECTMUSICTRACK8 iface)
{
	ICOM_THIS(IDirectMusicMelodyFormulationTrack,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicMelodyFormulationTrack IDirectMusicTrack part: */
HRESULT WINAPI IDirectMusicMelodyFormulationTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment)
{
	ICOM_THIS(IDirectMusicMelodyFormulationTrack,iface);

	FIXME("(%p, %p): stub\n", This, pSegment);

	return S_OK;
}

HRESULT WINAPI IDirectMusicMelodyFormulationTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
	ICOM_THIS(IDirectMusicMelodyFormulationTrack,iface);

	FIXME("(%p, %p, %p, %p, %ld, %ld): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicMelodyFormulationTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData)
{
	ICOM_THIS(IDirectMusicMelodyFormulationTrack,iface);

	FIXME("(%p, %p): stub\n", This, pStateData);

	return S_OK;
}

HRESULT WINAPI IDirectMusicMelodyFormulationTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID)
{
	ICOM_THIS(IDirectMusicMelodyFormulationTrack,iface);

	FIXME("(%p, %p, %ld, %ld, %ld, %ld, %p, %p, %ld): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);

	return S_OK;
}

HRESULT WINAPI IDirectMusicMelodyFormulationTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam)
{
	ICOM_THIS(IDirectMusicMelodyFormulationTrack,iface);

	FIXME("(%p, %s, %ld, %p, %p): stub\n", This, debugstr_guid(rguidType), mtTime, pmtNext, pParam);

	return S_OK;
}

HRESULT WINAPI IDirectMusicMelodyFormulationTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam)
{
	ICOM_THIS(IDirectMusicMelodyFormulationTrack,iface);

	FIXME("(%p, %s, %ld, %p): stub\n", This, debugstr_guid(rguidType), mtTime, pParam);

	return S_OK;
}

HRESULT WINAPI IDirectMusicMelodyFormulationTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType)
{
	ICOM_THIS(IDirectMusicMelodyFormulationTrack,iface);

	TRACE("(%p, %s): ", This, debugstr_guid(rguidType));
	if (IsEqualGUID (rguidType, &GUID_BandParam)
		|| IsEqualGUID (rguidType, &GUID_Clear_All_MelodyFragments)	
		|| IsEqualGUID (rguidType, &GUID_MelodyFragment)) {
		TRACE("param supported\n");
		return S_OK;
		}

	TRACE("param unsupported\n");
	return DMUS_E_TYPE_UNSUPPORTED;
}

HRESULT WINAPI IDirectMusicMelodyFormulationTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType)
{
	ICOM_THIS(IDirectMusicMelodyFormulationTrack,iface);

	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));

	return S_OK;
}

HRESULT WINAPI IDirectMusicMelodyFormulationTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType)
{
	ICOM_THIS(IDirectMusicMelodyFormulationTrack,iface);

	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));

	return S_OK;
}

HRESULT WINAPI IDirectMusicMelodyFormulationTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack)
{
	ICOM_THIS(IDirectMusicMelodyFormulationTrack,iface);

	FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);

	return S_OK;
}

/* IDirectMusicMelodyFormulationTrack IDirectMusicTrack8 part: */
HRESULT WINAPI IDirectMusicMelodyFormulationTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID)
{
	ICOM_THIS(IDirectMusicMelodyFormulationTrack,iface);

	FIXME("(%p, %p, %lli, %lli, %lli, %ld, %p, %p, %ld): stub\n", This, pStateData, rtStart, rtEnd, rtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);

	return S_OK;
}

HRESULT WINAPI IDirectMusicMelodyFormulationTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags)
{
	ICOM_THIS(IDirectMusicMelodyFormulationTrack,iface);

	FIXME("(%p, %s, %lli, %p, %p, %p, %ld): stub\n", This, debugstr_guid(rguidType), rtTime, prtNext, pParam, pStateData, dwFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicMelodyFormulationTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags)
{
	ICOM_THIS(IDirectMusicMelodyFormulationTrack,iface);

	FIXME("(%p, %s, %lli, %p, %p, %ld): stub\n", This, debugstr_guid(rguidType), rtTime, pParam, pStateData, dwFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicMelodyFormulationTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack)
{
	ICOM_THIS(IDirectMusicMelodyFormulationTrack,iface);

	FIXME("(%p, %p, %ld, %p): stub\n", This, pContext, dwTrackGroup, ppResultTrack);

	return S_OK;
}

HRESULT WINAPI IDirectMusicMelodyFormulationTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack)
{
	ICOM_THIS(IDirectMusicMelodyFormulationTrack,iface);

	FIXME("(%p, %p, %ld, %p, %ld, %p): stub\n", This, pNewTrack, mtJoin, pContext, dwTrackGroup, ppResultTrack);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicTrack8) DirectMusicMelodyFormulationTrack_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicMelodyFormulationTrack_QueryInterface,
	IDirectMusicMelodyFormulationTrack_AddRef,
	IDirectMusicMelodyFormulationTrack_Release,
	IDirectMusicMelodyFormulationTrack_Init,
	IDirectMusicMelodyFormulationTrack_InitPlay,
	IDirectMusicMelodyFormulationTrack_EndPlay,
	IDirectMusicMelodyFormulationTrack_Play,
	IDirectMusicMelodyFormulationTrack_GetParam,
	IDirectMusicMelodyFormulationTrack_SetParam,
	IDirectMusicMelodyFormulationTrack_IsParamSupported,
	IDirectMusicMelodyFormulationTrack_AddNotificationType,
	IDirectMusicMelodyFormulationTrack_RemoveNotificationType,
	IDirectMusicMelodyFormulationTrack_Clone,
	IDirectMusicMelodyFormulationTrack_PlayEx,
	IDirectMusicMelodyFormulationTrack_GetParamEx,
	IDirectMusicMelodyFormulationTrack_SetParamEx,
	IDirectMusicMelodyFormulationTrack_Compose,
	IDirectMusicMelodyFormulationTrack_Join
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicMelodyFormulationTrack (LPCGUID lpcGUID, LPDIRECTMUSICTRACK8 *ppTrack, LPUNKNOWN pUnkOuter)
{
	IDirectMusicMelodyFormulationTrack* track;
	
	if (IsEqualIID (lpcGUID, &IID_IDirectMusicTrack)
		|| IsEqualIID (lpcGUID, &IID_IDirectMusicTrack8)) {
		track = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicMelodyFormulationTrack));
		if (NULL == track) {
			*ppTrack = (LPDIRECTMUSICTRACK8) NULL;
			return E_OUTOFMEMORY;
		}
		track->lpVtbl = &DirectMusicMelodyFormulationTrack_Vtbl;
		track->ref = 1;
		track->pStream = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(IDirectMusicMelodyFormulationTrackStream));
		track->pStream->lpVtbl = &DirectMusicMelodyFormulationTrackStream_Vtbl;
		track->pStream->ref = 1;	
		track->pStream->pParentTrack = track;
		*ppTrack = (LPDIRECTMUSICTRACK8) track;
		return S_OK;
	}
	
	WARN("No interface found\n");
	return E_NOINTERFACE;
}


/*****************************************************************************
 * IDirectMusicMelodyFormulationTrackStream implementation
 */
/* IDirectMusicMelodyFormulationTrackStream IUnknown part follow: */
HRESULT WINAPI IDirectMusicMelodyFormulationTrackStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicMelodyFormulationTrackStream,iface);

	if (IsEqualIID(riid, &IID_IUnknown)
		|| IsEqualIID(riid, &IID_IPersistStream)) {
		IDirectMusicMelodyFormulationTrackStream_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}

	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicMelodyFormulationTrackStream_AddRef (LPPERSISTSTREAM iface)
{
	ICOM_THIS(IDirectMusicMelodyFormulationTrackStream,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicMelodyFormulationTrackStream_Release (LPPERSISTSTREAM iface)
{
	ICOM_THIS(IDirectMusicMelodyFormulationTrackStream,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicMelodyFormulationTrackStream IPersist part: */
HRESULT WINAPI IDirectMusicMelodyFormulationTrackStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID)
{
	return E_NOTIMPL;
}

/* IDirectMusicMelodyFormulationTrackStream IPersistStream part: */
HRESULT WINAPI IDirectMusicMelodyFormulationTrackStream_IsDirty (LPPERSISTSTREAM iface)
{
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicMelodyFormulationTrackStream_Load (LPPERSISTSTREAM iface, IStream* pStm)
{
	FIXME(": Loading not implemented yet\n");
	return S_OK;
}

HRESULT WINAPI IDirectMusicMelodyFormulationTrackStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty)
{
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicMelodyFormulationTrackStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize)
{
	return E_NOTIMPL;
}

ICOM_VTABLE(IPersistStream) DirectMusicMelodyFormulationTrackStream_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicMelodyFormulationTrackStream_QueryInterface,
	IDirectMusicMelodyFormulationTrackStream_AddRef,
	IDirectMusicMelodyFormulationTrackStream_Release,
	IDirectMusicMelodyFormulationTrackStream_GetClassID,
	IDirectMusicMelodyFormulationTrackStream_IsDirty,
	IDirectMusicMelodyFormulationTrackStream_Load,
	IDirectMusicMelodyFormulationTrackStream_Save,
	IDirectMusicMelodyFormulationTrackStream_GetSizeMax
};
