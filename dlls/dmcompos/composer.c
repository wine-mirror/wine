/* IDirectMusicComposer
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

#include "dmcompos_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmcompos);

/* IDirectMusicComposer IUnknown parts follow: */
HRESULT WINAPI IDirectMusicComposerImpl_QueryInterface (LPDIRECTMUSICCOMPOSER iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicComposerImpl,iface);

	if (IsEqualIID (riid, &IID_IUnknown) || 
	    IsEqualIID (riid, &IID_IDirectMusicComposer)) {
		IDirectMusicComposerImpl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicComposerImpl_AddRef (LPDIRECTMUSICCOMPOSER iface)
{
	ICOM_THIS(IDirectMusicComposerImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicComposerImpl_Release (LPDIRECTMUSICCOMPOSER iface)
{
	ICOM_THIS(IDirectMusicComposerImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicComposer Interface follow: */
HRESULT WINAPI IDirectMusicComposerImpl_ComposeSegmentFromTemplate (LPDIRECTMUSICCOMPOSER iface, IDirectMusicStyle* pStyle, IDirectMusicSegment* pTemplate, WORD wActivity, IDirectMusicChordMap* pChordMap, IDirectMusicSegment** ppSegment)
{
	ICOM_THIS(IDirectMusicComposerImpl,iface);

	FIXME("(%p, %p, %p, %d, %p, %p): stub\n", This, pStyle, pTemplate, wActivity, pChordMap, ppSegment);

	return S_OK;
}

HRESULT WINAPI IDirectMusicComposerImpl_ComposeSegmentFromShape (LPDIRECTMUSICCOMPOSER iface, IDirectMusicStyle* pStyle, WORD wNumMeasures, WORD wShape, WORD wActivity, BOOL fIntro, BOOL fEnd, IDirectMusicChordMap* pChordMap, IDirectMusicSegment** ppSegment)
{
	ICOM_THIS(IDirectMusicComposerImpl,iface);

	FIXME("(%p, %p, %d, %d, %d, %d, %d, %p, %p): stub\n", This, pStyle, wNumMeasures, wShape, wActivity, fIntro, fEnd, pChordMap, ppSegment);

	return S_OK;
}

HRESULT WINAPI IDirectMusicComposerImpl_ComposeTransition (LPDIRECTMUSICCOMPOSER iface, IDirectMusicSegment* pFromSeg, IDirectMusicSegment* pToSeg, MUSIC_TIME mtTime, WORD wCommand, DWORD dwFlags, IDirectMusicChordMap* pChordMap, IDirectMusicSegment** ppTransSeg)
{
	ICOM_THIS(IDirectMusicComposerImpl,iface);

	FIXME("(%p, %p, %p, %ld, %d, %ld, %p, %p): stub\n", This, pFromSeg, pToSeg, mtTime, wCommand, dwFlags, pChordMap, ppTransSeg);

	return S_OK;
}

HRESULT WINAPI IDirectMusicComposerImpl_AutoTransition (LPDIRECTMUSICCOMPOSER iface, IDirectMusicPerformance* pPerformance, IDirectMusicSegment* pToSeg, WORD wCommand, DWORD dwFlags, IDirectMusicChordMap* pChordMap, IDirectMusicSegment** ppTransSeg, IDirectMusicSegmentState** ppToSegState, IDirectMusicSegmentState** ppTransSegState)
{
	ICOM_THIS(IDirectMusicComposerImpl,iface);

	FIXME("(%p, %p, %d, %ld, %p, %p, %p, %p): stub\n", This, pPerformance, wCommand, dwFlags, pChordMap, ppTransSeg, ppToSegState, ppTransSegState);

	return S_OK;
}

HRESULT WINAPI IDirectMusicComposerImpl_ComposeTemplateFromShape (LPDIRECTMUSICCOMPOSER iface, WORD wNumMeasures, WORD wShape, BOOL fIntro, BOOL fEnd, WORD wEndLength, IDirectMusicSegment** ppTemplate)
{
	ICOM_THIS(IDirectMusicComposerImpl,iface);

	FIXME("(%p, %d, %d, %d, %d, %d, %p): stub\n", This, wNumMeasures, wShape, fIntro, fEnd, wEndLength, ppTemplate);

	return S_OK;
}

HRESULT WINAPI IDirectMusicComposerImpl_ChangeChordMap (LPDIRECTMUSICCOMPOSER iface, IDirectMusicSegment* pSegment, BOOL fTrackScale, IDirectMusicChordMap* pChordMap)
{
	ICOM_THIS(IDirectMusicComposerImpl,iface);

	FIXME("(%p, %p, %d, %p): stub\n", This, pSegment, fTrackScale, pChordMap);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicComposer) DirectMusicComposer_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
HRESULT WINAPI DMUSIC_CreateDirectMusicComposer (LPCGUID lpcGUID, LPDIRECTMUSICCOMPOSER* ppDMCP, LPUNKNOWN pUnkOuter)
{
	IDirectMusicComposerImpl* dmcompos;
	
	if (IsEqualIID (lpcGUID, &IID_IDirectMusicComposer)) {
		dmcompos = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicComposerImpl));
		if (NULL == dmcompos) {
			*ppDMCP = (LPDIRECTMUSICCOMPOSER) NULL;
			return E_OUTOFMEMORY;
		}
		dmcompos->lpVtbl = &DirectMusicComposer_Vtbl;
		dmcompos->ref = 1;
		*ppDMCP = (LPDIRECTMUSICCOMPOSER) dmcompos;
		return S_OK;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;	
}
