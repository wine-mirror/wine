/* IDirectMusicSong Implementation
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

#include "dmime_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

/* IDirectMusicSong IUnknown parts follow: */
HRESULT WINAPI IDirectMusicSongImpl_QueryInterface (LPDIRECTMUSICSONG iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicSongImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || 
	    IsEqualGUID(riid, &IID_IDirectMusicSong))
	{
		IDirectMusicSongImpl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicSongImpl_AddRef (LPDIRECTMUSICSONG iface)
{
	ICOM_THIS(IDirectMusicSongImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicSongImpl_Release (LPDIRECTMUSICSONG iface)
{
	ICOM_THIS(IDirectMusicSongImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicSong Interface follow: */
HRESULT WINAPI IDirectMusicSongImpl_Compose (LPDIRECTMUSICSONG iface)
{
	ICOM_THIS(IDirectMusicSongImpl,iface);

	FIXME("(%p): stub\n", This);
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicSongImpl_GetParam (LPDIRECTMUSICSONG iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam)
{
	ICOM_THIS(IDirectMusicSongImpl,iface);

	FIXME("(%p, %s, %ld, %ld, %ld, %p, %p): stub\n", This, debugstr_guid(rguidType), dwGroupBits, dwIndex, mtTime, pmtNext, pParam);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSongImpl_GetSegment (LPDIRECTMUSICSONG iface, WCHAR* pwzName, IDirectMusicSegment** ppSegment)
{
	ICOM_THIS(IDirectMusicSongImpl,iface);

	FIXME("(%p, %p, %p): stub\n", This, pwzName, ppSegment);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSongImpl_GetAudioPathConfig (LPDIRECTMUSICSONG iface, IUnknown** ppAudioPathConfig)
{
	ICOM_THIS(IDirectMusicSongImpl,iface);

	FIXME("(%p, %p): stub\n", This, ppAudioPathConfig);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSongImpl_Download (LPDIRECTMUSICSONG iface, IUnknown* pAudioPath)
{
	ICOM_THIS(IDirectMusicSongImpl,iface);

	FIXME("(%p, %p): stub\n", This, pAudioPath);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSongImpl_Unload (LPDIRECTMUSICSONG iface, IUnknown* pAudioPath)
{
	ICOM_THIS(IDirectMusicSongImpl,iface);

	FIXME("(%p, %p): stub\n", This, pAudioPath);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSongImpl_EnumSegment (LPDIRECTMUSICSONG iface, DWORD dwIndex, IDirectMusicSegment** ppSegment)
{
	ICOM_THIS(IDirectMusicSongImpl,iface);

	FIXME("(%p, %ld, %p): stub\n", This, dwIndex, ppSegment);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicSong) DirectMusicSong_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicSongImpl_QueryInterface,
	IDirectMusicSongImpl_AddRef,
	IDirectMusicSongImpl_Release,
	IDirectMusicSongImpl_Compose,
	IDirectMusicSongImpl_GetParam,
	IDirectMusicSongImpl_GetSegment,
	IDirectMusicSongImpl_GetAudioPathConfig,
	IDirectMusicSongImpl_Download,
	IDirectMusicSongImpl_Unload,
	IDirectMusicSongImpl_EnumSegment
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicSong (LPCGUID lpcGUID, LPDIRECTMUSICSONG *ppDMSng, LPUNKNOWN pUnkOuter)
{
	if (IsEqualGUID (lpcGUID, &IID_IDirectMusicComposer))
	{
		FIXME("Not yet\n");
		return E_NOINTERFACE;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;	
}
