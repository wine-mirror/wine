/* IDirectMusicPatternTrack Implementation
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
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

#include "dmime_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmime);

/* IDirectMusicPatternTrack IUnknown parts follow: */
HRESULT WINAPI IDirectMusicPatternTrackImpl_QueryInterface (LPDIRECTMUSICPATTERNTRACK iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS(IDirectMusicPatternTrackImpl,iface);

	if (IsEqualIID (riid, &IID_IUnknown) || 
	    IsEqualIID (riid, &IID_IDirectMusicPatternTrack)) {
		IDirectMusicPatternTrackImpl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicPatternTrackImpl_AddRef (LPDIRECTMUSICPATTERNTRACK iface) {
	ICOM_THIS(IDirectMusicPatternTrackImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicPatternTrackImpl_Release (LPDIRECTMUSICPATTERNTRACK iface) {
	ICOM_THIS(IDirectMusicPatternTrackImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicPatternTrack Interface follow: */
HRESULT WINAPI IDirectMusicPatternTrackImpl_CreateSegment (LPDIRECTMUSICPATTERNTRACK iface, IDirectMusicStyle* pStyle, IDirectMusicSegment** ppSegment) {
	ICOM_THIS(IDirectMusicPatternTrackImpl,iface);
	FIXME("(%p, %p, %p): stub\n", This, pStyle, ppSegment);
	return S_OK;
}

HRESULT WINAPI IDirectMusicPatternTrackImpl_SetVariation (LPDIRECTMUSICPATTERNTRACK iface, IDirectMusicSegmentState* pSegState, DWORD dwVariationFlags, DWORD dwPart) {
	ICOM_THIS(IDirectMusicPatternTrackImpl,iface);
	FIXME("(%p, %p, %ld, %ld): stub\n", This, pSegState, dwVariationFlags, dwPart);
	return S_OK;
}

HRESULT WINAPI IDirectMusicPatternTrackImpl_SetPatternByName (LPDIRECTMUSICPATTERNTRACK iface, IDirectMusicSegmentState* pSegState, WCHAR* wszName, IDirectMusicStyle* pStyle, DWORD dwPatternType, DWORD* pdwLength) {
	ICOM_THIS(IDirectMusicPatternTrackImpl,iface);
	FIXME("(%p, %p, %p, %p, %ld, %p): stub\n", This, pSegState, wszName, pStyle, dwPatternType, pdwLength);
	return S_OK;
}

ICOM_VTABLE(IDirectMusicPatternTrack) DirectMusicPatternTrack_Vtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicPatternTrackImpl_QueryInterface,
	IDirectMusicPatternTrackImpl_AddRef,
	IDirectMusicPatternTrackImpl_Release,
	IDirectMusicPatternTrackImpl_CreateSegment,
	IDirectMusicPatternTrackImpl_SetVariation,
	IDirectMusicPatternTrackImpl_SetPatternByName
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicPatternTrackImpl (LPCGUID lpcGUID, LPVOID *ppobj, LPUNKNOWN pUnkOuter) {
	IDirectMusicPatternTrackImpl* track;
	
	track = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicPatternTrackImpl));
	if (NULL == track) {
		*ppobj = (LPVOID) NULL;
		return E_OUTOFMEMORY;
	}
	track->lpVtbl = &DirectMusicPatternTrack_Vtbl;
	track->ref = 0; /* will be inited by QueryInterface */
	
	return IDirectMusicPatternTrackImpl_QueryInterface ((LPDIRECTMUSICPATTERNTRACK)track, lpcGUID, ppobj);
}
