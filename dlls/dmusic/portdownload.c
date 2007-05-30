/* IDirectMusicPortDownloadImpl Implementation
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

#include "dmusic_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

/* IDirectMusicPortDownload IUnknown parts follow: */
static HRESULT WINAPI IDirectMusicPortDownloadImpl_QueryInterface (LPDIRECTMUSICPORTDOWNLOAD iface, REFIID riid, LPVOID *ppobj) {
	IDirectMusicPortDownloadImpl *This = (IDirectMusicPortDownloadImpl *)iface;
	TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);

	if (IsEqualIID (riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicPortDownload)) {
		IUnknown_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicPortDownloadImpl_AddRef (LPDIRECTMUSICPORTDOWNLOAD iface) {
	IDirectMusicPortDownloadImpl *This = (IDirectMusicPortDownloadImpl *)iface;
	ULONG refCount = InterlockedIncrement(&This->ref);

	TRACE("(%p)->(ref before=%u)\n", This, refCount - 1);

	DMUSIC_LockModule();

	return refCount;
}

static ULONG WINAPI IDirectMusicPortDownloadImpl_Release (LPDIRECTMUSICPORTDOWNLOAD iface) {
	IDirectMusicPortDownloadImpl *This = (IDirectMusicPortDownloadImpl *)iface;
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(ref before=%u)\n", This, refCount + 1);

	if (!refCount) {
		HeapFree(GetProcessHeap(), 0, This);
	}

	DMUSIC_UnlockModule();

	return refCount;
}

/* IDirectMusicPortDownload Interface follow: */
static HRESULT WINAPI IDirectMusicPortDownloadImpl_GetBuffer (LPDIRECTMUSICPORTDOWNLOAD iface, DWORD dwDLId, IDirectMusicDownload** ppIDMDownload) {
	IDirectMusicPortDownloadImpl *This = (IDirectMusicPortDownloadImpl *)iface;
	FIXME("(%p, %d, %p): stub\n", This, dwDLId, ppIDMDownload);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicPortDownloadImpl_AllocateBuffer (LPDIRECTMUSICPORTDOWNLOAD iface, DWORD dwSize, IDirectMusicDownload** ppIDMDownload) {
	IDirectMusicPortDownloadImpl *This = (IDirectMusicPortDownloadImpl *)iface;
	FIXME("(%p, %d, %p): stub\n", This, dwSize, ppIDMDownload);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicPortDownloadImpl_GetDLId (LPDIRECTMUSICPORTDOWNLOAD iface, DWORD* pdwStartDLId, DWORD dwCount) {
	IDirectMusicPortDownloadImpl *This = (IDirectMusicPortDownloadImpl *)iface;
	FIXME("(%p, %p, %d): stub\n", This, pdwStartDLId, dwCount);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicPortDownloadImpl_GetAppend (LPDIRECTMUSICPORTDOWNLOAD iface, DWORD* pdwAppend) {
	IDirectMusicPortDownloadImpl *This = (IDirectMusicPortDownloadImpl *)iface;
	FIXME("(%p, %p): stub\n", This, pdwAppend);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicPortDownloadImpl_Download (LPDIRECTMUSICPORTDOWNLOAD iface, IDirectMusicDownload* pIDMDownload) {
	IDirectMusicPortDownloadImpl *This = (IDirectMusicPortDownloadImpl *)iface;
	FIXME("(%p, %p): stub\n", This, pIDMDownload);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicPortDownloadImpl_Unload (LPDIRECTMUSICPORTDOWNLOAD iface, IDirectMusicDownload* pIDMDownload) {
	IDirectMusicPortDownloadImpl *This = (IDirectMusicPortDownloadImpl *)iface;
	FIXME("(%p, %p): stub\n", This, pIDMDownload);
	return S_OK;
}

static const IDirectMusicPortDownloadVtbl DirectMusicPortDownload_Vtbl = {
	IDirectMusicPortDownloadImpl_QueryInterface,
	IDirectMusicPortDownloadImpl_AddRef,
	IDirectMusicPortDownloadImpl_Release,
	IDirectMusicPortDownloadImpl_GetBuffer,
	IDirectMusicPortDownloadImpl_AllocateBuffer,
	IDirectMusicPortDownloadImpl_GetDLId,
	IDirectMusicPortDownloadImpl_GetAppend,
	IDirectMusicPortDownloadImpl_Download,
	IDirectMusicPortDownloadImpl_Unload
};
