/* IDirectMusicMarkerTrack Implementation
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

#include "dmime_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmime);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

/*****************************************************************************
 * IDirectMusicMarkerTrack implementation
 */
/* IDirectMusicMarkerTrack IUnknown part: */
HRESULT WINAPI IDirectMusicMarkerTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicMarkerTrack,iface);

	if (IsEqualIID (riid, &IID_IUnknown) || 
	    IsEqualIID (riid, &IID_IDirectMusicTrack) ||
	    IsEqualIID (riid, &IID_IDirectMusicTrack8)) {
		IDirectMusicMarkerTrack_AddRef(iface);
		*ppobj = This;
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IPersistStream)) {
		IDirectMusicMarkerTrackStream_AddRef ((LPPERSISTSTREAM)This->pStream);
		*ppobj = This->pStream;
		return S_OK;
	}
	
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicMarkerTrack_AddRef (LPDIRECTMUSICTRACK8 iface)
{
	ICOM_THIS(IDirectMusicMarkerTrack,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicMarkerTrack_Release (LPDIRECTMUSICTRACK8 iface)
{
	ICOM_THIS(IDirectMusicMarkerTrack,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicMarkerTrack IDirectMusicTrack part: */
HRESULT WINAPI IDirectMusicMarkerTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment)
{
	ICOM_THIS(IDirectMusicMarkerTrack,iface);

	FIXME("(%p, %p): stub\n", This, pSegment);

	return S_OK;
}

HRESULT WINAPI IDirectMusicMarkerTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
	ICOM_THIS(IDirectMusicMarkerTrack,iface);

	FIXME("(%p, %p, %p, %p, %ld, %ld): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicMarkerTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData)
{
	ICOM_THIS(IDirectMusicMarkerTrack,iface);

	FIXME("(%p, %p): stub\n", This, pStateData);

	return S_OK;
}

HRESULT WINAPI IDirectMusicMarkerTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID)
{
	ICOM_THIS(IDirectMusicMarkerTrack,iface);

	FIXME("(%p, %p, %ld, %ld, %ld, %ld, %p, %p, %ld): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);

	return S_OK;
}

HRESULT WINAPI IDirectMusicMarkerTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam)
{
	ICOM_THIS(IDirectMusicMarkerTrack,iface);

	FIXME("(%p, %s, %ld, %p, %p): stub\n", This, debugstr_guid(rguidType), mtTime, pmtNext, pParam);

	return S_OK;
}

HRESULT WINAPI IDirectMusicMarkerTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam)
{
	ICOM_THIS(IDirectMusicMarkerTrack,iface);

	FIXME("(%p, %s, %ld, %p): stub\n", This, debugstr_guid(rguidType), mtTime, pParam);

	return S_OK;
}

HRESULT WINAPI IDirectMusicMarkerTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType)
{
	ICOM_THIS(IDirectMusicMarkerTrack,iface);

	TRACE("(%p, %s): ", This, debugstr_guid(rguidType));
	if (IsEqualGUID (rguidType, &GUID_Play_Marker)
		|| IsEqualGUID (rguidType, &GUID_Valid_Start_Time)) {
		TRACE("param supported\n");
		return S_OK;
		}

	TRACE("param unsupported\n");
	return DMUS_E_TYPE_UNSUPPORTED;
}

HRESULT WINAPI IDirectMusicMarkerTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType)
{
	ICOM_THIS(IDirectMusicMarkerTrack,iface);

	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));

	return S_OK;
}

HRESULT WINAPI IDirectMusicMarkerTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType)
{
	ICOM_THIS(IDirectMusicMarkerTrack,iface);

	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));

	return S_OK;
}

HRESULT WINAPI IDirectMusicMarkerTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack)
{
	ICOM_THIS(IDirectMusicMarkerTrack,iface);

	FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);

	return S_OK;
}

/* IDirectMusicMarkerTrack IDirectMusicTrack8 part: */
HRESULT WINAPI IDirectMusicMarkerTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID)
{
	ICOM_THIS(IDirectMusicMarkerTrack,iface);

	FIXME("(%p, %p, %lli, %lli, %lli, %ld, %p, %p, %ld): stub\n", This, pStateData, rtStart, rtEnd, rtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);

	return S_OK;
}

HRESULT WINAPI IDirectMusicMarkerTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags)
{
	ICOM_THIS(IDirectMusicMarkerTrack,iface);

	FIXME("(%p, %s, %lli, %p, %p, %p, %ld): stub\n", This, debugstr_guid(rguidType), rtTime, prtNext, pParam, pStateData, dwFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicMarkerTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags)
{
	ICOM_THIS(IDirectMusicMarkerTrack,iface);

	FIXME("(%p, %s, %lli, %p, %p, %ld): stub\n", This, debugstr_guid(rguidType), rtTime, pParam, pStateData, dwFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicMarkerTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack)
{
	ICOM_THIS(IDirectMusicMarkerTrack,iface);

	FIXME("(%p, %p, %ld, %p): stub\n", This, pContext, dwTrackGroup, ppResultTrack);

	return S_OK;
}

HRESULT WINAPI IDirectMusicMarkerTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack)
{
	ICOM_THIS(IDirectMusicMarkerTrack,iface);

	FIXME("(%p, %p, %ld, %p, %ld, %p): stub\n", This, pNewTrack, mtJoin, pContext, dwTrackGroup, ppResultTrack);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicTrack8) DirectMusicMarkerTrack_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicMarkerTrack_QueryInterface,
	IDirectMusicMarkerTrack_AddRef,
	IDirectMusicMarkerTrack_Release,
	IDirectMusicMarkerTrack_Init,
	IDirectMusicMarkerTrack_InitPlay,
	IDirectMusicMarkerTrack_EndPlay,
	IDirectMusicMarkerTrack_Play,
	IDirectMusicMarkerTrack_GetParam,
	IDirectMusicMarkerTrack_SetParam,
	IDirectMusicMarkerTrack_IsParamSupported,
	IDirectMusicMarkerTrack_AddNotificationType,
	IDirectMusicMarkerTrack_RemoveNotificationType,
	IDirectMusicMarkerTrack_Clone,
	IDirectMusicMarkerTrack_PlayEx,
	IDirectMusicMarkerTrack_GetParamEx,
	IDirectMusicMarkerTrack_SetParamEx,
	IDirectMusicMarkerTrack_Compose,
	IDirectMusicMarkerTrack_Join
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicMarkerTrack (LPCGUID lpcGUID, LPDIRECTMUSICTRACK8 *ppTrack, LPUNKNOWN pUnkOuter)
{
	IDirectMusicMarkerTrack* track;
	
	if (IsEqualIID (lpcGUID, &IID_IDirectMusicTrack)
		|| IsEqualIID (lpcGUID, &IID_IDirectMusicTrack8)) {
		track = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicMarkerTrack));
		if (NULL == track) {
			*ppTrack = (LPDIRECTMUSICTRACK8) NULL;
			return E_OUTOFMEMORY;
		}
		track->lpVtbl = &DirectMusicMarkerTrack_Vtbl;
		track->ref = 1;
		track->pStream = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(IDirectMusicMarkerTrackStream));
		track->pStream->lpVtbl = &DirectMusicMarkerTrackStream_Vtbl;
		track->pStream->ref = 1;	
		track->pStream->pParentTrack = track;
		*ppTrack = (LPDIRECTMUSICTRACK8) track;
		return S_OK;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;
}


/*****************************************************************************
 * IDirectMusicMarkerTrackStream implementation
 */
/* IDirectMusicMarkerTrackStream IUnknown part follow: */
HRESULT WINAPI IDirectMusicMarkerTrackStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicMarkerTrackStream,iface);

	if (IsEqualIID (riid, &IID_IUnknown)
		|| IsEqualIID (riid, &IID_IPersistStream)) {
		IDirectMusicMarkerTrackStream_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicMarkerTrackStream_AddRef (LPPERSISTSTREAM iface)
{
	ICOM_THIS(IDirectMusicMarkerTrackStream,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicMarkerTrackStream_Release (LPPERSISTSTREAM iface)
{
	ICOM_THIS(IDirectMusicMarkerTrackStream,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicMarkerTrackStream IPersist part: */
HRESULT WINAPI IDirectMusicMarkerTrackStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID)
{
	return E_NOTIMPL;
}

/* IDirectMusicMarkerTrackStream IPersistStream part: */
HRESULT WINAPI IDirectMusicMarkerTrackStream_IsDirty (LPPERSISTSTREAM iface)
{
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicMarkerTrackStream_Load (LPPERSISTSTREAM iface, IStream* pStm)
{
	FIXME(": Loading not implemented yet\n");
	return S_OK;
}

HRESULT WINAPI IDirectMusicMarkerTrackStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty)
{
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicMarkerTrackStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize)
{
	return E_NOTIMPL;
}

ICOM_VTABLE(IPersistStream) DirectMusicMarkerTrackStream_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicMarkerTrackStream_QueryInterface,
	IDirectMusicMarkerTrackStream_AddRef,
	IDirectMusicMarkerTrackStream_Release,
	IDirectMusicMarkerTrackStream_GetClassID,
	IDirectMusicMarkerTrackStream_IsDirty,
	IDirectMusicMarkerTrackStream_Load,
	IDirectMusicMarkerTrackStream_Save,
	IDirectMusicMarkerTrackStream_GetSizeMax
};
