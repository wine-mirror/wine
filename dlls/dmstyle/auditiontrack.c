/* IDirectMusicAuditionTrack Implementation
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
 * IDirectMusicAuditionTrack implementation
 */
/* IDirectMusicAuditionTrack IUnknown part: */
HRESULT WINAPI IDirectMusicAuditionTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicAuditionTrack,iface);

	if (IsEqualIID (riid, &IID_IUnknown) || 
	    IsEqualIID (riid, &IID_IDirectMusicTrack) ||
	    IsEqualIID (riid, &IID_IDirectMusicTrack8)) {
		IDirectMusicAuditionTrack_AddRef(iface);
		*ppobj = This;
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IPersistStream)) {
		IDirectMusicAuditionTrackStream_AddRef ((LPPERSISTSTREAM)This->pStream);
		*ppobj = This->pStream;
		return S_OK;
	}

	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicAuditionTrack_AddRef (LPDIRECTMUSICTRACK8 iface)
{
	ICOM_THIS(IDirectMusicAuditionTrack,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicAuditionTrack_Release (LPDIRECTMUSICTRACK8 iface)
{
	ICOM_THIS(IDirectMusicAuditionTrack,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicAuditionTrack IDirectMusicTrack part: */
HRESULT WINAPI IDirectMusicAuditionTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment)
{
	ICOM_THIS(IDirectMusicAuditionTrack,iface);

	FIXME("(%p, %p): stub\n", This, pSegment);

	return S_OK;
}

HRESULT WINAPI IDirectMusicAuditionTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
	ICOM_THIS(IDirectMusicAuditionTrack,iface);

	FIXME("(%p, %p, %p, %p, %ld, %ld): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicAuditionTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData)
{
	ICOM_THIS(IDirectMusicAuditionTrack,iface);

	FIXME("(%p, %p): stub\n", This, pStateData);

	return S_OK;
}

HRESULT WINAPI IDirectMusicAuditionTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID)
{
	ICOM_THIS(IDirectMusicAuditionTrack,iface);

	FIXME("(%p, %p, %ld, %ld, %ld, %ld, %p, %p, %ld): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);

	return S_OK;
}

HRESULT WINAPI IDirectMusicAuditionTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam)
{
	ICOM_THIS(IDirectMusicAuditionTrack,iface);

	FIXME("(%p, %s, %ld, %p, %p): stub\n", This, debugstr_guid(rguidType), mtTime, pmtNext, pParam);

	return S_OK;
}

HRESULT WINAPI IDirectMusicAuditionTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam)
{
	ICOM_THIS(IDirectMusicAuditionTrack,iface);

	FIXME("(%p, %s, %ld, %p): stub\n", This, debugstr_guid(rguidType), mtTime, pParam);

	return S_OK;
}

HRESULT WINAPI IDirectMusicAuditionTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType)
{
	ICOM_THIS(IDirectMusicAuditionTrack,iface);

	TRACE("(%p, %s): ", This, debugstr_guid(rguidType));
	/* didn't find any params */

	TRACE("param unsupported\n");
	return DMUS_E_TYPE_UNSUPPORTED;
}

HRESULT WINAPI IDirectMusicAuditionTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType)
{
	ICOM_THIS(IDirectMusicAuditionTrack,iface);

	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));

	return S_OK;
}

HRESULT WINAPI IDirectMusicAuditionTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType)
{
	ICOM_THIS(IDirectMusicAuditionTrack,iface);

	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));

	return S_OK;
}

HRESULT WINAPI IDirectMusicAuditionTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack)
{
	ICOM_THIS(IDirectMusicAuditionTrack,iface);

	FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);

	return S_OK;
}

/* IDirectMusicAuditionTrack IDirectMusicTrack8 part: */
HRESULT WINAPI IDirectMusicAuditionTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID)
{
	ICOM_THIS(IDirectMusicAuditionTrack,iface);

	FIXME("(%p, %p, %lli, %lli, %lli, %ld, %p, %p, %ld): stub\n", This, pStateData, rtStart, rtEnd, rtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);

	return S_OK;
}

HRESULT WINAPI IDirectMusicAuditionTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags)
{
	ICOM_THIS(IDirectMusicAuditionTrack,iface);

	FIXME("(%p, %s, %lli, %p, %p, %p, %ld): stub\n", This, debugstr_guid(rguidType), rtTime, prtNext, pParam, pStateData, dwFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicAuditionTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags)
{
	ICOM_THIS(IDirectMusicAuditionTrack,iface);

	FIXME("(%p, %s, %lli, %p, %p, %ld): stub\n", This, debugstr_guid(rguidType), rtTime, pParam, pStateData, dwFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicAuditionTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack)
{
	ICOM_THIS(IDirectMusicAuditionTrack,iface);

	FIXME("(%p, %p, %ld, %p): stub\n", This, pContext, dwTrackGroup, ppResultTrack);

	return S_OK;
}

HRESULT WINAPI IDirectMusicAuditionTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack)
{
	ICOM_THIS(IDirectMusicAuditionTrack,iface);

	FIXME("(%p, %p, %ld, %p, %ld, %p): stub\n", This, pNewTrack, mtJoin, pContext, dwTrackGroup, ppResultTrack);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicTrack8) DirectMusicAuditionTrack_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicAuditionTrack_QueryInterface,
	IDirectMusicAuditionTrack_AddRef,
	IDirectMusicAuditionTrack_Release,
	IDirectMusicAuditionTrack_Init,
	IDirectMusicAuditionTrack_InitPlay,
	IDirectMusicAuditionTrack_EndPlay,
	IDirectMusicAuditionTrack_Play,
	IDirectMusicAuditionTrack_GetParam,
	IDirectMusicAuditionTrack_SetParam,
	IDirectMusicAuditionTrack_IsParamSupported,
	IDirectMusicAuditionTrack_AddNotificationType,
	IDirectMusicAuditionTrack_RemoveNotificationType,
	IDirectMusicAuditionTrack_Clone,
	IDirectMusicAuditionTrack_PlayEx,
	IDirectMusicAuditionTrack_GetParamEx,
	IDirectMusicAuditionTrack_SetParamEx,
	IDirectMusicAuditionTrack_Compose,
	IDirectMusicAuditionTrack_Join
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicAuditionTrack (LPCGUID lpcGUID, LPDIRECTMUSICTRACK8 *ppTrack, LPUNKNOWN pUnkOuter)
{
	IDirectMusicAuditionTrack* track;
	
	if (IsEqualIID (lpcGUID, &IID_IDirectMusicTrack)
		|| IsEqualIID (lpcGUID, &IID_IDirectMusicTrack8)) {
		track = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicAuditionTrack));
		if (NULL == track) {
			*ppTrack = (LPDIRECTMUSICTRACK8) NULL;
			return E_OUTOFMEMORY;
		}
		track->lpVtbl = &DirectMusicAuditionTrack_Vtbl;
		track->ref = 1;
		track->pStream = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(IDirectMusicAuditionTrackStream));
		track->pStream->lpVtbl = &DirectMusicAuditionTrackStream_Vtbl;
		track->pStream->ref = 1;	
		track->pStream->pParentTrack = track;
		*ppTrack = (LPDIRECTMUSICTRACK8) track;
		return S_OK;
	}

	WARN("No interface found\n");	
	return E_NOINTERFACE;
}


/*****************************************************************************
 * IDirectMusicAuditionTrackStream implementation
 */
/* IDirectMusicAuditionTrackStream IUnknown part follow: */
HRESULT WINAPI IDirectMusicAuditionTrackStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicAuditionTrackStream,iface);

	if (IsEqualIID (riid, &IID_IUnknown)
		|| IsEqualIID (riid, &IID_IPersistStream)) {
		IDirectMusicAuditionTrackStream_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}

	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicAuditionTrackStream_AddRef (LPPERSISTSTREAM iface)
{
	ICOM_THIS(IDirectMusicAuditionTrackStream,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicAuditionTrackStream_Release (LPPERSISTSTREAM iface)
{
	ICOM_THIS(IDirectMusicAuditionTrackStream,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicAuditionTrackStream IPersist part: */
HRESULT WINAPI IDirectMusicAuditionTrackStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID)
{
	return E_NOTIMPL;
}

/* IDirectMusicAuditionTrackStream IPersistStream part: */
HRESULT WINAPI IDirectMusicAuditionTrackStream_IsDirty (LPPERSISTSTREAM iface)
{
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicAuditionTrackStream_Load (LPPERSISTSTREAM iface, IStream* pStm)
{
	FIXME(": Loading not implemented yet\n");
	return S_OK;
}

HRESULT WINAPI IDirectMusicAuditionTrackStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty)
{
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicAuditionTrackStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize)
{
	return E_NOTIMPL;
}

ICOM_VTABLE(IPersistStream) DirectMusicAuditionTrackStream_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicAuditionTrackStream_QueryInterface,
	IDirectMusicAuditionTrackStream_AddRef,
	IDirectMusicAuditionTrackStream_Release,
	IDirectMusicAuditionTrackStream_GetClassID,
	IDirectMusicAuditionTrackStream_IsDirty,
	IDirectMusicAuditionTrackStream_Load,
	IDirectMusicAuditionTrackStream_Save,
	IDirectMusicAuditionTrackStream_GetSizeMax
};
