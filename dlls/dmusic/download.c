/* IDirectMusicDownload Implementation
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

#include "dmusic_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

/* IDirectMusicDownloadImpl IUnknown part: */
HRESULT WINAPI IDirectMusicDownloadImpl_QueryInterface (LPDIRECTMUSICDOWNLOAD iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS(IDirectMusicDownloadImpl,iface);

	if (IsEqualIID (riid, &IID_IUnknown) 
		|| IsEqualIID (riid, &IID_IDirectMusicDownload)) {
		IDirectMusicDownloadImpl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicDownloadImpl_AddRef (LPDIRECTMUSICDOWNLOAD iface) {
	ICOM_THIS(IDirectMusicDownloadImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicDownloadImpl_Release (LPDIRECTMUSICDOWNLOAD iface) {
	ICOM_THIS(IDirectMusicDownloadImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicDownloadImpl IDirectMusicDownload part: */
HRESULT WINAPI IDirectMusicDownloadImpl_GetBuffer (LPDIRECTMUSICDOWNLOAD iface, void** ppvBuffer, DWORD* pdwSize) {
	ICOM_THIS(IDirectMusicDownloadImpl,iface);
	FIXME("(%p,%p, %p): stub\n", This, ppvBuffer, pdwSize);
	return S_OK;
}

ICOM_VTABLE(IDirectMusicDownload) DirectMusicDownload_Vtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicDownloadImpl_QueryInterface,
	IDirectMusicDownloadImpl_AddRef,
	IDirectMusicDownloadImpl_Release,
	IDirectMusicDownloadImpl_GetBuffer
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicDownloadImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter) {
	IDirectMusicDownloadImpl* dmdl;
	
	dmdl = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicDownloadImpl));
	if (NULL == dmdl) {
		*ppobj = (LPVOID) NULL;
		return E_OUTOFMEMORY;
	}
	dmdl->lpVtbl = &DirectMusicDownload_Vtbl;
	dmdl->ref = 0; /* will be inited by QueryInterface */
	
	return IDirectMusicDownloadImpl_QueryInterface ((LPDIRECTMUSICDOWNLOAD)dmdl, lpcGUID, ppobj);
}
