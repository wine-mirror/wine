/* IDirectMusicPortDownloadImpl Implementation
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "dmusic_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

/* IDirectMusicPortDownload IUnknown parts follow: */
HRESULT WINAPI IDirectMusicPortDownloadImpl_QueryInterface (LPDIRECTMUSICPORTDOWNLOAD iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicPortDownloadImpl,iface);

	if (IsEqualIID (riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicPortDownload)) {
		IDirectMusicPortDownloadImpl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}

	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicPortDownloadImpl_AddRef (LPDIRECTMUSICPORTDOWNLOAD iface)
{
	ICOM_THIS(IDirectMusicPortDownloadImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicPortDownloadImpl_Release (LPDIRECTMUSICPORTDOWNLOAD iface)
{
	ICOM_THIS(IDirectMusicPortDownloadImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicPortDownload Interface follow: */
HRESULT WINAPI IDirectMusicPortDownloadImpl_GetBuffer (LPDIRECTMUSICPORTDOWNLOAD iface, DWORD dwDLId, IDirectMusicDownload** ppIDMDownload)
{
	ICOM_THIS(IDirectMusicPortDownloadImpl,iface);
	
	FIXME("(%p, %ld, %p): stub\n", This, dwDLId, ppIDMDownload);
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicPortDownloadImpl_AllocateBuffer (LPDIRECTMUSICPORTDOWNLOAD iface, DWORD dwSize, IDirectMusicDownload** ppIDMDownload)
{
	ICOM_THIS(IDirectMusicPortDownloadImpl,iface);
	
	FIXME("(%p, %ld, %p): stub\n", This, dwSize, ppIDMDownload);
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicPortDownloadImpl_GetDLId (LPDIRECTMUSICPORTDOWNLOAD iface, DWORD* pdwStartDLId, DWORD dwCount)
{
	ICOM_THIS(IDirectMusicPortDownloadImpl,iface);
	
	FIXME("(%p, %p, %ld): stub\n", This, pdwStartDLId, dwCount);
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicPortDownloadImpl_GetAppend (LPDIRECTMUSICPORTDOWNLOAD iface, DWORD* pdwAppend)
{
	ICOM_THIS(IDirectMusicPortDownloadImpl,iface);
	
	FIXME("(%p, %p): stub\n", This, pdwAppend);
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicPortDownloadImpl_Download (LPDIRECTMUSICPORTDOWNLOAD iface, IDirectMusicDownload* pIDMDownload)
{
	ICOM_THIS(IDirectMusicPortDownloadImpl,iface);
	
	FIXME("(%p, %p): stub\n", This, pIDMDownload);
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicPortDownloadImpl_Unload (LPDIRECTMUSICPORTDOWNLOAD iface, IDirectMusicDownload* pIDMDownload)
{
	ICOM_THIS(IDirectMusicPortDownloadImpl,iface);
	
	FIXME("(%p, %p): stub\n", This, pIDMDownload);
	
	return S_OK;
}

ICOM_VTABLE(IDirectMusicPortDownload) DirectMusicPortDownload_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicPortDownload (LPCGUID lpcGUID, LPDIRECTMUSICPORTDOWNLOAD* ppDMPortDL, LPUNKNOWN pUnkOuter)
{
	if (IsEqualIID (lpcGUID, &IID_IDirectMusicPortDownload))
	{
		FIXME("Not yet\n");
		return E_NOINTERFACE;
	}

	WARN("No interface found\n");	
	return E_NOINTERFACE;	
}
