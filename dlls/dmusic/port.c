/* IDirectMusicPort Implementation
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
		return S_OK;
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
	ICOM_THIS(IDirectMusicPortImpl,iface);

	FIXME("(%p, %p): stub\n", This, pBuffer);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_SetReadNotificationHandle (LPDIRECTMUSICPORT iface, HANDLE hEvent)
{
	ICOM_THIS(IDirectMusicPortImpl,iface);

	FIXME("(%p, %p): stub\n", This, hEvent);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_Read (LPDIRECTMUSICPORT iface, LPDIRECTMUSICBUFFER pBuffer)
{
	ICOM_THIS(IDirectMusicPortImpl,iface);

	FIXME("(%p, %p): stub\n", This, pBuffer);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_DownloadInstrument (LPDIRECTMUSICPORT iface, IDirectMusicInstrument* pInstrument, IDirectMusicDownloadedInstrument** ppDownloadedInstrument, DMUS_NOTERANGE* pNoteRanges, DWORD dwNumNoteRanges)
{
	ICOM_THIS(IDirectMusicPortImpl,iface);

	FIXME("(%p, %p, %p, %p, %ld): stub\n", This, pInstrument, ppDownloadedInstrument, pNoteRanges, dwNumNoteRanges);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_UnloadInstrument (LPDIRECTMUSICPORT iface, IDirectMusicDownloadedInstrument *pDownloadedInstrument)
{
	ICOM_THIS(IDirectMusicPortImpl,iface);

	FIXME("(%p, %p): stub\n", This, pDownloadedInstrument);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_GetLatencyClock (LPDIRECTMUSICPORT iface, IReferenceClock** ppClock)
{
	ICOM_THIS(IDirectMusicPortImpl,iface);

	TRACE("(%p, %p)\n", This, ppClock);
	*ppClock = This->pLatencyClock;
	IReferenceClock_AddRef (*ppClock);
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_GetRunningStats (LPDIRECTMUSICPORT iface, LPDMUS_SYNTHSTATS pStats)
{
	ICOM_THIS(IDirectMusicPortImpl,iface);

	FIXME("(%p, %p): stub\n", This, pStats);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_GetCaps (LPDIRECTMUSICPORT iface, LPDMUS_PORTCAPS pPortCaps)
{
	ICOM_THIS(IDirectMusicPortImpl,iface);
	
	TRACE("(%p, %p)\n", This, pPortCaps);
	pPortCaps = This->pCaps;
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_DeviceIoControl (LPDIRECTMUSICPORT iface, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped)
{
	ICOM_THIS(IDirectMusicPortImpl,iface);

	FIXME("(%p, %ld, %p, %ld, %p, %ld, %p, %p): stub\n", This, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_SetNumChannelGroups (LPDIRECTMUSICPORT iface, DWORD dwChannelGroups)
{
	ICOM_THIS(IDirectMusicPortImpl,iface);

	FIXME("(%p, %ld): semi-stub\n", This, dwChannelGroups);
	This->nrofgroups = dwChannelGroups;
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_GetNumChannelGroups (LPDIRECTMUSICPORT iface, LPDWORD pdwChannelGroups)
{
	ICOM_THIS(IDirectMusicPortImpl,iface);

	TRACE("(%p, %p)\n", This, pdwChannelGroups);
	*pdwChannelGroups = This->nrofgroups;
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_Activate (LPDIRECTMUSICPORT iface, BOOL fActive)
{
	ICOM_THIS(IDirectMusicPortImpl,iface);

	TRACE("(%p, %d)\n", This, fActive);
	This->fActive = fActive;
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_SetChannelPriority (LPDIRECTMUSICPORT iface, DWORD dwChannelGroup, DWORD dwChannel, DWORD dwPriority)
{
	ICOM_THIS(IDirectMusicPortImpl,iface);
	
	FIXME("(%p, %ld, %ld, %ld): semi-stub\n", This, dwChannelGroup, dwChannel, dwPriority);
	
	if (dwChannel > 16)
	{
		WARN("isn't there supposed to be 16 channels (no. %ld requested)?! (faking as it is ok)\n", dwChannel);
		/*return E_INVALIDARG;*/
	}	
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_GetChannelPriority (LPDIRECTMUSICPORT iface, DWORD dwChannelGroup, DWORD dwChannel, LPDWORD pdwPriority)
{
	ICOM_THIS(IDirectMusicPortImpl,iface);

	TRACE("(%p, %ld, %ld, %p)\n", This, dwChannelGroup, dwChannel, pdwPriority);
	*pdwPriority = This->group[dwChannelGroup-1].channel[dwChannel].priority;
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_SetDirectSound (LPDIRECTMUSICPORT iface, LPDIRECTSOUND pDirectSound, LPDIRECTSOUNDBUFFER pDirectSoundBuffer)
{
	ICOM_THIS(IDirectMusicPortImpl,iface);

	FIXME("(%p, %p, %p): stub\n", This, pDirectSound, pDirectSoundBuffer);

	return S_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_GetFormat (LPDIRECTMUSICPORT iface, LPWAVEFORMATEX pWaveFormatEx, LPDWORD pdwWaveFormatExSize, LPDWORD pdwBufferSize)
{
	ICOM_THIS(IDirectMusicPortImpl,iface);

	FIXME("(%p, %p, %p, %p): stub\n", This, pWaveFormatEx, pdwWaveFormatExSize, pdwBufferSize);

	return S_OK;
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

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicPort (LPCGUID lpcGUID, LPDIRECTMUSICPORT* ppDMPort, LPUNKNOWN pUnkOuter)
{
	if (IsEqualGUID (lpcGUID, &IID_IDirectMusicPort))
	{
		FIXME("Not yet\n");
		return E_NOINTERFACE;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;	
}
