/* IDirectMusicPort Implementation
 * IDirectMusicPortDownloadImpl Implementation
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

#include "dmusic_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

/* IDirectMusicPort IUnknown parts follow: */
HRESULT WINAPI IDirectMusicPortImpl_QueryInterface (LPDIRECTMUSICPORT iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicPortImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicPort))
	{
		IDirectMusicPortImpl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicPortImpl_AddRef (LPDIRECTMUSICPORT iface)
{
	ICOM_THIS(IDirectMusicPortImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicPortImpl_Release (LPDIRECTMUSICPORT iface)
{
	ICOM_THIS(IDirectMusicPortImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicPort Interface follow: */
HRESULT WINAPI IDirectMusicPortImpl_PlayBuffer (LPDIRECTMUSICPORT iface, LPDIRECTMUSICBUFFER pBuffer)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_SetReadNotificationHandle (LPDIRECTMUSICPORT iface, HANDLE hEvent)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_Read (LPDIRECTMUSICPORT iface, LPDIRECTMUSICBUFFER pBuffer)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_DownloadInstrument (LPDIRECTMUSICPORT iface, IDirectMusicInstrument* pInstrument, IDirectMusicDownloadedInstrument** ppDownloadedInstrument, DMUS_NOTERANGE* pNoteRanges, DWORD dwNumNoteRanges)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_UnloadInstrument (LPDIRECTMUSICPORT iface, IDirectMusicDownloadedInstrument *pDownloadedInstrument)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_GetLatencyClock (LPDIRECTMUSICPORT iface, IReferenceClock** ppClock)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_GetRunningStats (LPDIRECTMUSICPORT iface, LPDMUS_SYNTHSTATS pStats)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_GetCaps (LPDIRECTMUSICPORT iface, LPDMUS_PORTCAPS pPortCaps)
{
	ICOM_THIS(IDirectMusicPortImpl,iface);
	
	TRACE("(%p, %p)\n", This, pPortCaps);
	pPortCaps = This->caps;
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_DeviceIoControl (LPDIRECTMUSICPORT iface, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_SetNumChannelGroups (LPDIRECTMUSICPORT iface, DWORD dwChannelGroups)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_GetNumChannelGroups (LPDIRECTMUSICPORT iface, LPDWORD pdwChannelGroups)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_Activate (LPDIRECTMUSICPORT iface, BOOL fActive)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_SetChannelPriority (LPDIRECTMUSICPORT iface, DWORD dwChannelGroup, DWORD dwChannel, DWORD dwPriority)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_GetChannelPriority (LPDIRECTMUSICPORT iface, DWORD dwChannelGroup, DWORD dwChannel, LPDWORD pdwPriority)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_SetDirectSound (LPDIRECTMUSICPORT iface, LPDIRECTSOUND pDirectSound, LPDIRECTSOUNDBUFFER pDirectSoundBuffer)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_GetFormat (LPDIRECTMUSICPORT iface, LPWAVEFORMATEX pWaveFormatEx, LPDWORD pdwWaveFormatExSize, LPDWORD pdwBufferSize)
{
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectMusicPort) DirectMusicPort_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicPortImpl_QueryInterface,
	IDirectMusicPortImpl_AddRef,
	IDirectMusicPortImpl_Release,
	IDirectMusicPortImpl_PlayBuffer,
	IDirectMusicPortImpl_SetReadNotificationHandle,
	IDirectMusicPortImpl_Read,
	IDirectMusicPortImpl_DownloadInstrument,
	IDirectMusicPortImpl_UnloadInstrument,
	IDirectMusicPortImpl_GetLatencyClock,
	IDirectMusicPortImpl_GetRunningStats,
	IDirectMusicPortImpl_GetCaps,
	IDirectMusicPortImpl_DeviceIoControl,
	IDirectMusicPortImpl_SetNumChannelGroups,
	IDirectMusicPortImpl_GetNumChannelGroups,
	IDirectMusicPortImpl_Activate,
	IDirectMusicPortImpl_SetChannelPriority,
	IDirectMusicPortImpl_GetChannelPriority,
	IDirectMusicPortImpl_SetDirectSound,
	IDirectMusicPortImpl_GetFormat
};


/* IDirectMusicPortDownload IUnknown parts follow: */
HRESULT WINAPI IDirectMusicPortDownloadImpl_QueryInterface (LPDIRECTMUSICPORTDOWNLOAD iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicPortDownloadImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicPortDownload))
	{
		IDirectMusicPortDownloadImpl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
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
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicPortDownload Interface follow: */
HRESULT WINAPI IDirectMusicPortDownloadImpl_GetBuffer (LPDIRECTMUSICPORTDOWNLOAD iface, DWORD dwDLId, IDirectMusicDownload** ppIDMDownload)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPortDownloadImpl_AllocateBuffer (LPDIRECTMUSICPORTDOWNLOAD iface, DWORD dwSize, IDirectMusicDownload** ppIDMDownload)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPortDownloadImpl_GetDLId (LPDIRECTMUSICPORTDOWNLOAD iface, DWORD* pdwStartDLId, DWORD dwCount)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPortDownloadImpl_GetAppend (LPDIRECTMUSICPORTDOWNLOAD iface, DWORD* pdwAppend)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPortDownloadImpl_Download (LPDIRECTMUSICPORTDOWNLOAD iface, IDirectMusicDownload* pIDMDownload)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicPortDownloadImpl_Unload (LPDIRECTMUSICPORTDOWNLOAD iface, IDirectMusicDownload* pIDMDownload)
{
	FIXME("stub\n");
	return DS_OK;
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
