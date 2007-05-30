/* IDirectMusicTool8 Implementation
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

/* IDirectMusicTool8Impl IUnknown part: */
static HRESULT WINAPI IDirectMusicTool8Impl_QueryInterface (LPDIRECTMUSICTOOL8 iface, REFIID riid, LPVOID *ppobj) {
	IDirectMusicTool8Impl *This = (IDirectMusicTool8Impl *)iface;
	TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);
	if (IsEqualIID (riid, &IID_IUnknown) || 
	    IsEqualIID (riid, &IID_IDirectMusicTool) ||
	    IsEqualIID (riid, &IID_IDirectMusicTool8)) {
		IUnknown_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicTool8Impl_AddRef (LPDIRECTMUSICTOOL8 iface) {
	IDirectMusicTool8Impl *This = (IDirectMusicTool8Impl *)iface;
        ULONG ref = InterlockedIncrement(&This->ref);

	TRACE("(%p) : AddRef from %d\n", This, ref - 1);

	DMIME_LockModule();

	return ref;
}

static ULONG WINAPI IDirectMusicTool8Impl_Release (LPDIRECTMUSICTOOL8 iface) {
	IDirectMusicTool8Impl *This = (IDirectMusicTool8Impl *)iface;
	ULONG ref = InterlockedDecrement(&This->ref);
	TRACE("(%p) : ReleaseRef to %d\n", This, ref);
	
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}

	DMIME_UnlockModule();
	
	return ref;
}

/* IDirectMusicTool8Impl IDirectMusicTool part: */
static HRESULT WINAPI IDirectMusicTool8Impl_Init (LPDIRECTMUSICTOOL8 iface, IDirectMusicGraph* pGraph) {
	IDirectMusicTool8Impl *This = (IDirectMusicTool8Impl *)iface;
	FIXME("(%p, %p): stub\n", This, pGraph);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTool8Impl_GetMsgDeliveryType (LPDIRECTMUSICTOOL8 iface, DWORD* pdwDeliveryType) {
	IDirectMusicTool8Impl *This = (IDirectMusicTool8Impl *)iface;
	FIXME("(%p, %p): stub\n", This, pdwDeliveryType);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTool8Impl_GetMediaTypeArraySize (LPDIRECTMUSICTOOL8 iface, DWORD* pdwNumElements) {
	IDirectMusicTool8Impl *This = (IDirectMusicTool8Impl *)iface;
	FIXME("(%p, %p): stub\n", This, pdwNumElements);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTool8Impl_GetMediaTypes (LPDIRECTMUSICTOOL8 iface, DWORD** padwMediaTypes, DWORD dwNumElements) {
	IDirectMusicTool8Impl *This = (IDirectMusicTool8Impl *)iface;
	FIXME("(%p, %p, %d): stub\n", This, padwMediaTypes, dwNumElements);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTool8Impl_ProcessPMsg (LPDIRECTMUSICTOOL8 iface, IDirectMusicPerformance* pPerf, DMUS_PMSG* pPMSG) {
	IDirectMusicTool8Impl *This = (IDirectMusicTool8Impl *)iface;
	FIXME("(%p, %p, %p): stub\n", This, pPerf, pPMSG);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTool8Impl_Flush (LPDIRECTMUSICTOOL8 iface, IDirectMusicPerformance* pPerf, DMUS_PMSG* pPMSG, REFERENCE_TIME rtTime) {
	IDirectMusicTool8Impl *This = (IDirectMusicTool8Impl *)iface;
	FIXME("(%p, %p, %p, 0x%s): stub\n", This, pPerf, pPMSG, wine_dbgstr_longlong(rtTime));
	return S_OK;
}

/* IDirectMusicTool8Impl IDirectMusicTool8 part: */
static HRESULT WINAPI IDirectMusicTool8Impl_Clone (LPDIRECTMUSICTOOL8 iface, IDirectMusicTool** ppTool) {
	IDirectMusicTool8Impl *This = (IDirectMusicTool8Impl *)iface;
	FIXME("(%p, %p): stub\n", This, ppTool);
	return S_OK;
}

static const IDirectMusicTool8Vtbl DirectMusicTool8_Vtbl = {
	IDirectMusicTool8Impl_QueryInterface,
	IDirectMusicTool8Impl_AddRef,
	IDirectMusicTool8Impl_Release,
	IDirectMusicTool8Impl_Init,
	IDirectMusicTool8Impl_GetMsgDeliveryType,
	IDirectMusicTool8Impl_GetMediaTypeArraySize,
	IDirectMusicTool8Impl_GetMediaTypes,
	IDirectMusicTool8Impl_ProcessPMsg,
	IDirectMusicTool8Impl_Flush,
	IDirectMusicTool8Impl_Clone
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicobjImpl (LPCGUID lpcGUID, LPVOID *ppobj, LPUNKNOWN pUnkOuter) {
	IDirectMusicTool8Impl* obj;
	
	obj = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicTool8Impl));
	if (NULL == obj) {
		*ppobj = NULL;
		return E_OUTOFMEMORY;
	}
	obj->lpVtbl = &DirectMusicTool8_Vtbl;
	obj->ref = 0; /* will be inited by QueryInterface */
	
	return IDirectMusicTool8Impl_QueryInterface ((LPDIRECTMUSICTOOL8)obj, lpcGUID, ppobj);	
}
