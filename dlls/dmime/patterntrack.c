/* IDirectMusicPatternTrack Implementation
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

/* IDirectMusicPatternTrack IUnknown parts follow: */
static HRESULT WINAPI IDirectMusicPatternTrackImpl_QueryInterface (LPDIRECTMUSICPATTERNTRACK iface, REFIID riid, LPVOID *ppobj) {
	IDirectMusicPatternTrackImpl *This = (IDirectMusicPatternTrackImpl *)iface;
	TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);

	if (IsEqualIID (riid, &IID_IUnknown) || 
	    IsEqualIID (riid, &IID_IDirectMusicPatternTrack)) {
		IUnknown_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	
	WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicPatternTrackImpl_AddRef (LPDIRECTMUSICPATTERNTRACK iface) {
	IDirectMusicPatternTrackImpl *This = (IDirectMusicPatternTrackImpl *)iface;
        ULONG ref = InterlockedIncrement(&This->ref);

	TRACE("(%p): AddRef from %d\n", This, ref - 1);

	DMIME_LockModule();

	return ref;
}

static ULONG WINAPI IDirectMusicPatternTrackImpl_Release (LPDIRECTMUSICPATTERNTRACK iface) {
	IDirectMusicPatternTrackImpl *This = (IDirectMusicPatternTrackImpl *)iface;
	ULONG ref = InterlockedDecrement(&This->ref);
	TRACE("(%p): ReleaseRef to %d\n", This, ref);
	
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}

	DMIME_UnlockModule();

	return ref;
}

/* IDirectMusicPatternTrack Interface follow: */
static HRESULT WINAPI IDirectMusicPatternTrackImpl_CreateSegment (LPDIRECTMUSICPATTERNTRACK iface, IDirectMusicStyle* pStyle, IDirectMusicSegment** ppSegment) {
	IDirectMusicPatternTrackImpl *This = (IDirectMusicPatternTrackImpl *)iface;
	FIXME("(%p, %p, %p): stub\n", This, pStyle, ppSegment);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicPatternTrackImpl_SetVariation (LPDIRECTMUSICPATTERNTRACK iface, IDirectMusicSegmentState* pSegState, DWORD dwVariationFlags, DWORD dwPart) {
	IDirectMusicPatternTrackImpl *This = (IDirectMusicPatternTrackImpl *)iface;
	FIXME("(%p, %p, %d, %d): stub\n", This, pSegState, dwVariationFlags, dwPart);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicPatternTrackImpl_SetPatternByName (LPDIRECTMUSICPATTERNTRACK iface, IDirectMusicSegmentState* pSegState, WCHAR* wszName, IDirectMusicStyle* pStyle, DWORD dwPatternType, DWORD* pdwLength) {
	IDirectMusicPatternTrackImpl *This = (IDirectMusicPatternTrackImpl *)iface;
	FIXME("(%p, %p, %p, %p, %d, %p): stub\n", This, pSegState, wszName, pStyle, dwPatternType, pdwLength);
	return S_OK;
}

static const IDirectMusicPatternTrackVtbl DirectMusicPatternTrack_Vtbl = {
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
		*ppobj = NULL;
		return E_OUTOFMEMORY;
	}
	track->lpVtbl = &DirectMusicPatternTrack_Vtbl;
	track->ref = 0; /* will be inited by QueryInterface */
	
	return IDirectMusicPatternTrackImpl_QueryInterface ((LPDIRECTMUSICPATTERNTRACK)track, lpcGUID, ppobj);
}
