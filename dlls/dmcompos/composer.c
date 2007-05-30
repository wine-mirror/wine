/* IDirectMusicComposer
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

#include "dmcompos_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmcompos);

/* IDirectMusicComposerImpl IUnknown part: */
static HRESULT WINAPI IDirectMusicComposerImpl_QueryInterface (LPDIRECTMUSICCOMPOSER iface, REFIID riid, LPVOID *ppobj) {
	IDirectMusicComposerImpl *This = (IDirectMusicComposerImpl *)iface;
	TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);

	if (IsEqualIID (riid, &IID_IUnknown) || 
	    IsEqualIID (riid, &IID_IDirectMusicComposer)) {
		IUnknown_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicComposerImpl_AddRef (LPDIRECTMUSICCOMPOSER iface) {
	IDirectMusicComposerImpl *This = (IDirectMusicComposerImpl *)iface;
        ULONG ref = InterlockedIncrement(&This->ref);

	TRACE("(%p): AddRef from %d\n", This, ref - 1);
	
	DMCOMPOS_LockModule();
	
	return ref;
}

static ULONG WINAPI IDirectMusicComposerImpl_Release (LPDIRECTMUSICCOMPOSER iface) {
	IDirectMusicComposerImpl *This = (IDirectMusicComposerImpl *)iface;
	ULONG ref = InterlockedDecrement(&This->ref);

	TRACE("(%p): ReleaseRef to %d\n", This, ref);
	
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	
	DMCOMPOS_UnlockModule();
	
	return ref;
}

/* IDirectMusicComposerImpl IDirectMusicComposer part: */
static HRESULT WINAPI IDirectMusicComposerImpl_ComposeSegmentFromTemplate (LPDIRECTMUSICCOMPOSER iface, IDirectMusicStyle* pStyle, IDirectMusicSegment* pTemplate, WORD wActivity, IDirectMusicChordMap* pChordMap, IDirectMusicSegment** ppSegment) {
	IDirectMusicComposerImpl *This = (IDirectMusicComposerImpl *)iface;
	FIXME("(%p, %p, %p, %d, %p, %p): stub\n", This, pStyle, pTemplate, wActivity, pChordMap, ppSegment);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicComposerImpl_ComposeSegmentFromShape (LPDIRECTMUSICCOMPOSER iface, IDirectMusicStyle* pStyle, WORD wNumMeasures, WORD wShape, WORD wActivity, BOOL fIntro, BOOL fEnd, IDirectMusicChordMap* pChordMap, IDirectMusicSegment** ppSegment) {
	IDirectMusicComposerImpl *This = (IDirectMusicComposerImpl *)iface;
	FIXME("(%p, %p, %d, %d, %d, %d, %d, %p, %p): stub\n", This, pStyle, wNumMeasures, wShape, wActivity, fIntro, fEnd, pChordMap, ppSegment);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicComposerImpl_ComposeTransition (LPDIRECTMUSICCOMPOSER iface, IDirectMusicSegment* pFromSeg, IDirectMusicSegment* pToSeg, MUSIC_TIME mtTime, WORD wCommand, DWORD dwFlags, IDirectMusicChordMap* pChordMap, IDirectMusicSegment** ppTransSeg) {
	IDirectMusicComposerImpl *This = (IDirectMusicComposerImpl *)iface;
	FIXME("(%p, %p, %p, %ld, %d, %d, %p, %p): stub\n", This, pFromSeg, pToSeg, mtTime, wCommand, dwFlags, pChordMap, ppTransSeg);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicComposerImpl_AutoTransition (LPDIRECTMUSICCOMPOSER iface, IDirectMusicPerformance* pPerformance, IDirectMusicSegment* pToSeg, WORD wCommand, DWORD dwFlags, IDirectMusicChordMap* pChordMap, IDirectMusicSegment** ppTransSeg, IDirectMusicSegmentState** ppToSegState, IDirectMusicSegmentState** ppTransSegState) {
	IDirectMusicComposerImpl *This = (IDirectMusicComposerImpl *)iface;
	FIXME("(%p, %p, %d, %d, %p, %p, %p, %p): stub\n", This, pPerformance, wCommand, dwFlags, pChordMap, ppTransSeg, ppToSegState, ppTransSegState);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicComposerImpl_ComposeTemplateFromShape (LPDIRECTMUSICCOMPOSER iface, WORD wNumMeasures, WORD wShape, BOOL fIntro, BOOL fEnd, WORD wEndLength, IDirectMusicSegment** ppTemplate) {
	IDirectMusicComposerImpl *This = (IDirectMusicComposerImpl *)iface;
	FIXME("(%p, %d, %d, %d, %d, %d, %p): stub\n", This, wNumMeasures, wShape, fIntro, fEnd, wEndLength, ppTemplate);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicComposerImpl_ChangeChordMap (LPDIRECTMUSICCOMPOSER iface, IDirectMusicSegment* pSegment, BOOL fTrackScale, IDirectMusicChordMap* pChordMap) {
	IDirectMusicComposerImpl *This = (IDirectMusicComposerImpl *)iface;
	FIXME("(%p, %p, %d, %p): stub\n", This, pSegment, fTrackScale, pChordMap);
	return S_OK;
}

static const IDirectMusicComposerVtbl DirectMusicComposer_Vtbl = {
	IDirectMusicComposerImpl_QueryInterface,
	IDirectMusicComposerImpl_AddRef,
	IDirectMusicComposerImpl_Release,
	IDirectMusicComposerImpl_ComposeSegmentFromTemplate,
	IDirectMusicComposerImpl_ComposeSegmentFromShape,
	IDirectMusicComposerImpl_ComposeTransition,
	IDirectMusicComposerImpl_AutoTransition,
	IDirectMusicComposerImpl_ComposeTemplateFromShape,
	IDirectMusicComposerImpl_ChangeChordMap
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicComposerImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter) {
	IDirectMusicComposerImpl* obj;
	
	obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicComposerImpl));
	if (NULL == obj) {
		*ppobj = (LPDIRECTMUSICCOMPOSER) NULL;
		return E_OUTOFMEMORY;
	}
	obj->lpVtbl = &DirectMusicComposer_Vtbl;
	obj->ref = 0; /* will be inited by QueryInterface */
	
	return IDirectMusicComposerImpl_QueryInterface ((LPDIRECTMUSICCOMPOSER)obj, lpcGUID, ppobj);	
}
