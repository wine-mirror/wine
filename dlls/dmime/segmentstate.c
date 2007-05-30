/* IDirectMusicSegmentState8 Implementation
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

/* IDirectMusicSegmentState8Impl IUnknown part: */
static HRESULT WINAPI IDirectMusicSegmentState8Impl_QueryInterface (LPDIRECTMUSICSEGMENTSTATE8 iface, REFIID riid, LPVOID *ppobj) {
	IDirectMusicSegmentState8Impl *This = (IDirectMusicSegmentState8Impl *)iface;
	TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);

	if (IsEqualIID(riid, &IID_IUnknown) || 
	    IsEqualIID(riid, &IID_IDirectMusicSegmentState) ||
	    IsEqualIID(riid, &IID_IDirectMusicSegmentState8)) {
		IUnknown_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicSegmentState8Impl_AddRef (LPDIRECTMUSICSEGMENTSTATE8 iface) {
	IDirectMusicSegmentState8Impl *This = (IDirectMusicSegmentState8Impl *)iface;
        ULONG ref = InterlockedIncrement(&This->ref);

	TRACE("(%p): AddRef from %d\n", This, ref - 1);

	DMIME_LockModule();

	return ref;
}

static ULONG WINAPI IDirectMusicSegmentState8Impl_Release (LPDIRECTMUSICSEGMENTSTATE8 iface) {
	IDirectMusicSegmentState8Impl *This = (IDirectMusicSegmentState8Impl *)iface;
	ULONG ref = InterlockedDecrement(&This->ref);
	TRACE("(%p): ReleaseRef to %d\n", This, ref);
	
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}

	DMIME_UnlockModule();

	return ref;
}

/* IDirectMusicSegmentState8Impl IDirectMusicSegmentState part: */
static HRESULT WINAPI IDirectMusicSegmentState8Impl_GetRepeats (LPDIRECTMUSICSEGMENTSTATE8 iface,  DWORD* pdwRepeats) {
	IDirectMusicSegmentState8Impl *This = (IDirectMusicSegmentState8Impl *)iface;
	FIXME("(%p, %p): stub\n", This, pdwRepeats);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSegmentState8Impl_GetSegment (LPDIRECTMUSICSEGMENTSTATE8 iface, IDirectMusicSegment** ppSegment) {
	IDirectMusicSegmentState8Impl *This = (IDirectMusicSegmentState8Impl *)iface;
	FIXME("(%p, %p): stub\n", This, ppSegment);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSegmentState8Impl_GetStartTime (LPDIRECTMUSICSEGMENTSTATE8 iface, MUSIC_TIME* pmtStart) {
	IDirectMusicSegmentState8Impl *This = (IDirectMusicSegmentState8Impl *)iface;
	FIXME("(%p, %p): stub\n", This, pmtStart);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSegmentState8Impl_GetSeek (LPDIRECTMUSICSEGMENTSTATE8 iface, MUSIC_TIME* pmtSeek) {
	IDirectMusicSegmentState8Impl *This = (IDirectMusicSegmentState8Impl *)iface;
	FIXME("(%p, %p): stub\n", This, pmtSeek);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSegmentState8Impl_GetStartPoint (LPDIRECTMUSICSEGMENTSTATE8 iface, MUSIC_TIME* pmtStart) {
	IDirectMusicSegmentState8Impl *This = (IDirectMusicSegmentState8Impl *)iface;
	FIXME("(%p, %p): stub\n", This, pmtStart);
	return S_OK;
}

/* IDirectMusicSegmentState8Impl IDirectMusicSegmentState8 part: */
static HRESULT WINAPI IDirectMusicSegmentState8Impl_SetTrackConfig (LPDIRECTMUSICSEGMENTSTATE8 iface, REFGUID rguidTrackClassID, DWORD dwGroupBits, DWORD dwIndex, DWORD dwFlagsOn, DWORD dwFlagsOff) {
	IDirectMusicSegmentState8Impl *This = (IDirectMusicSegmentState8Impl *)iface;
	FIXME("(%p, %s, %d, %d, %d, %d): stub\n", This, debugstr_dmguid(rguidTrackClassID), dwGroupBits, dwIndex, dwFlagsOn, dwFlagsOff);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicSegmentState8Impl_GetObjectInPath (LPDIRECTMUSICSEGMENTSTATE8 iface, DWORD dwPChannel, DWORD dwStage, DWORD dwBuffer, REFGUID guidObject, DWORD dwIndex, REFGUID iidInterface, void** ppObject) {
	IDirectMusicSegmentState8Impl *This = (IDirectMusicSegmentState8Impl *)iface;
	FIXME("(%p, %d, %d, %d, %s, %d, %s, %p): stub\n", This, dwPChannel, dwStage, dwBuffer, debugstr_dmguid(guidObject), dwIndex, debugstr_dmguid(iidInterface), ppObject);
	return S_OK;
}

static const IDirectMusicSegmentState8Vtbl DirectMusicSegmentState8_Vtbl = {
	IDirectMusicSegmentState8Impl_QueryInterface,
	IDirectMusicSegmentState8Impl_AddRef,
	IDirectMusicSegmentState8Impl_Release,
	IDirectMusicSegmentState8Impl_GetRepeats,
	IDirectMusicSegmentState8Impl_GetSegment,
	IDirectMusicSegmentState8Impl_GetStartTime,
	IDirectMusicSegmentState8Impl_GetSeek,
	IDirectMusicSegmentState8Impl_GetStartPoint,
	IDirectMusicSegmentState8Impl_SetTrackConfig,
	IDirectMusicSegmentState8Impl_GetObjectInPath
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicSegmentStateImpl (LPCGUID lpcGUID, LPVOID *ppobj, LPUNKNOWN pUnkOuter) {
	IDirectMusicSegmentState8Impl* obj;
	
	obj = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicSegmentState8Impl));
	if (NULL == obj) {
		*ppobj = NULL;
		return E_OUTOFMEMORY;
	}
	obj->lpVtbl = &DirectMusicSegmentState8_Vtbl;
	obj->ref = 0; /* will be inited by QueryInterface */
	
	return IDirectMusicSegmentState8Impl_QueryInterface ((LPDIRECTMUSICSEGMENTSTATE8)obj, lpcGUID, ppobj);
}
