/* IDirectMusic Implementation
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

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusic (LPCGUID lpcGUID, LPDIRECTMUSIC *ppDM, LPUNKNOWN pUnkOuter)
{
	IDirectMusicImpl *dmusic;

	TRACE("(%p,%p,%p)\n",lpcGUID, ppDM, pUnkOuter);


	dmusic = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicImpl));
	if (NULL == dmusic)
	{
		*ppDM = (LPDIRECTMUSIC)NULL;
		return E_OUTOFMEMORY;
	}
	
	dmusic->lpVtbl = &DirectMusic_Vtbl;
	dmusic->ref = 1;
	
	*ppDM = (LPDIRECTMUSIC)dmusic;
	return S_OK;
	
	WARN("No interface found\n");
	return E_NOINTERFACE;
}

/* IDirectMusic IUnknown parts follow: */
HRESULT WINAPI IDirectMusicImpl_QueryInterface (LPDIRECTMUSIC iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusic))
	{
		IDirectMusicImpl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}

	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicImpl_AddRef (LPDIRECTMUSIC iface)
{
	ICOM_THIS(IDirectMusicImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI   IDirectMusicImpl_Release (LPDIRECTMUSIC iface)
{
	ICOM_THIS(IDirectMusicImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusic Interface follow: */
HRESULT WINAPI IDirectMusicImpl_EnumPort (LPDIRECTMUSIC iface, DWORD dwIndex, LPDMUS_PORTCAPS pPortCaps)
{
	/* FIXME: this is only for making DXCapsViewer display something */
	static const WCHAR name_WINE_SYNTHESIZER[] = {'W','i','n','e',' ','S','y','n','t','h','e','s','i','z','e','r',0};

	if (dwIndex == 0)
	{
		pPortCaps->dwSize = sizeof(DMUS_PORTCAPS);
		pPortCaps->dwFlags = DMUS_PC_DLS | DMUS_PC_SOFTWARESYNTH | DMUS_PC_DIRECTSOUND | DMUS_PC_DLS2 | DMUS_PC_AUDIOPATH | DMUS_PC_WAVE;
		/*pPortCaps->guidPort;*/ /* FIXME */
		pPortCaps->dwClass = DMUS_PC_OUTPUTCLASS;
		pPortCaps->dwType = DMUS_PORT_WINMM_DRIVER;
		pPortCaps->dwMemorySize = DMUS_PC_SYSTEMMEMORY;
		pPortCaps->dwMaxChannelGroups = 1000;
		pPortCaps->dwMaxVoices = 1000;
		pPortCaps->dwMaxAudioChannels = -1;
		pPortCaps->dwEffectFlags = DMUS_EFFECT_REVERB | DMUS_EFFECT_CHORUS | DMUS_EFFECT_DELAY;
		wsprintfW(pPortCaps->wszDescription, name_WINE_SYNTHESIZER);
		return S_OK;
	}

	FIXME("partial-stub (only first port supported)\n");
	return S_FALSE;
}

HRESULT WINAPI IDirectMusicImpl_CreateMusicBuffer (LPDIRECTMUSIC iface, LPDMUS_BUFFERDESC pBufferDesc, LPDIRECTMUSICBUFFER** ppBuffer, LPUNKNOWN pUnkOuter)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicImpl_CreatePort (LPDIRECTMUSIC iface, REFCLSID rclsidPort, LPDMUS_PORTPARAMS pPortParams, LPDIRECTMUSICPORT* ppPort, LPUNKNOWN pUnkOuter)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicImpl_EnumMasterClock (LPDIRECTMUSIC iface, DWORD dwIndex, LPDMUS_CLOCKINFO lpClockInfo)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicImpl_GetMasterClock (LPDIRECTMUSIC iface, LPGUID pguidClock, IReferenceClock** ppReferenceClock)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicImpl_SetMasterClock (LPDIRECTMUSIC iface, REFGUID rguidClock)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicImpl_Activate (LPDIRECTMUSIC iface, BOOL fEnable)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicImpl_GetDefaultPort (LPDIRECTMUSIC iface, LPGUID pguidPort)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicImpl_SetDirectSound (LPDIRECTMUSIC iface, LPDIRECTSOUND pDirectSound, HWND hWnd)
{
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectMusic) DirectMusic_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicImpl_QueryInterface,
	IDirectMusicImpl_AddRef,
	IDirectMusicImpl_Release,
	IDirectMusicImpl_EnumPort,
	IDirectMusicImpl_CreateMusicBuffer,
	IDirectMusicImpl_CreatePort,
	IDirectMusicImpl_EnumMasterClock,
	IDirectMusicImpl_GetMasterClock,
	IDirectMusicImpl_SetMasterClock,
	IDirectMusicImpl_Activate,
	IDirectMusicImpl_GetDefaultPort,
	IDirectMusicImpl_SetDirectSound
};
