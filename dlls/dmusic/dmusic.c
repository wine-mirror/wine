/* IDirectMusic Implementation
 * IDirectMusic8 Implementation
 * IDirectMusicDownload Implementation
 * IDirectMusicBuffer Implementation
 * IDirectMusicObject Implementation
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

/* IDirectMusic8 IUnknown parts follow: */
HRESULT WINAPI IDirectMusic8Impl_QueryInterface (LPDIRECTMUSIC8 iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusic8Impl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusic8))
	{
		IDirectMusic8Impl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusic8Impl_AddRef (LPDIRECTMUSIC8 iface)
{
	ICOM_THIS(IDirectMusic8Impl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusic8Impl_Release (LPDIRECTMUSIC8 iface)
{
	ICOM_THIS(IDirectMusic8Impl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

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


/* IDirectMusic8 Interface follow: */
HRESULT WINAPI IDirectMusic8Impl_EnumPort (LPDIRECTMUSIC8 iface, DWORD dwIndex, LPDMUS_PORTCAPS pPortCaps)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusic8Impl_CreateMusicBuffer (LPDIRECTMUSIC8 iface, LPDMUS_BUFFERDESC pBufferDesc, LPDIRECTMUSICBUFFER** ppBuffer, LPUNKNOWN pUnkOuter)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusic8Impl_CreatePort (LPDIRECTMUSIC8 iface, REFCLSID rclsidPort, LPDMUS_PORTPARAMS pPortParams, LPDIRECTMUSICPORT* ppPort, LPUNKNOWN pUnkOuter)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusic8Impl_EnumMasterClock (LPDIRECTMUSIC8 iface, DWORD dwIndex, LPDMUS_CLOCKINFO lpClockInfo)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusic8Impl_GetMasterClock (LPDIRECTMUSIC8 iface, LPGUID pguidClock, IReferenceClock** ppReferenceClock)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusic8Impl_SetMasterClock (LPDIRECTMUSIC8 iface, REFGUID rguidClock)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusic8Impl_Activate (LPDIRECTMUSIC8 iface, BOOL fEnable)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusic8Impl_GetDefaultPort (LPDIRECTMUSIC8 iface, LPGUID pguidPort)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusic8Impl_SetDirectSound (LPDIRECTMUSIC8 iface, LPDIRECTSOUND pDirectSound, HWND hWnd)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusic8Impl_SetExternalMasterClock (LPDIRECTMUSIC8 iface, IReferenceClock* pClock)
{
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectMusic8) DirectMusic8_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusic8Impl_QueryInterface,
	IDirectMusic8Impl_AddRef,
	IDirectMusic8Impl_Release,
	IDirectMusic8Impl_EnumPort,
	IDirectMusic8Impl_CreateMusicBuffer,
	IDirectMusic8Impl_CreatePort,
	IDirectMusic8Impl_EnumMasterClock,
	IDirectMusic8Impl_GetMasterClock,
	IDirectMusic8Impl_SetMasterClock,
	IDirectMusic8Impl_Activate,
	IDirectMusic8Impl_GetDefaultPort,
	IDirectMusic8Impl_SetDirectSound,
	IDirectMusic8Impl_SetExternalMasterClock
};


/* IDirectMusicDownload IUnknown parts follow: */
HRESULT WINAPI IDirectMusicDownloadImpl_QueryInterface (LPDIRECTMUSICDOWNLOAD iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicDownloadImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicDownload))
	{
		IDirectMusicDownloadImpl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicDownloadImpl_AddRef (LPDIRECTMUSICDOWNLOAD iface)
{
	ICOM_THIS(IDirectMusicDownloadImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicDownloadImpl_Release (LPDIRECTMUSICDOWNLOAD iface)
{
	ICOM_THIS(IDirectMusicDownloadImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicDownload Interface follow: */
HRESULT WINAPI IDirectMusicDownloadImpl_GetBuffer (LPDIRECTMUSICDOWNLOAD iface, void** ppvBuffer, DWORD* pdwSize)
{
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectMusicDownload) DirectMusicDownload_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicDownloadImpl_QueryInterface,
	IDirectMusicDownloadImpl_AddRef,
	IDirectMusicDownloadImpl_Release,
	IDirectMusicDownloadImpl_GetBuffer
};


/* IDirectMusicBuffer IUnknown parts follow: */
HRESULT WINAPI IDirectMusicBufferImpl_QueryInterface (LPDIRECTMUSICBUFFER iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicBufferImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicBuffer))
	{
		IDirectMusicBufferImpl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicBufferImpl_AddRef (LPDIRECTMUSICBUFFER iface)
{
	ICOM_THIS(IDirectMusicBufferImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicBufferImpl_Release (LPDIRECTMUSICBUFFER iface)
{
	ICOM_THIS(IDirectMusicBufferImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicBuffer Interface follow: */
HRESULT WINAPI IDirectMusicBufferImpl_Flush (LPDIRECTMUSICBUFFER iface)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicBufferImpl_TotalTime (LPDIRECTMUSICBUFFER iface, LPREFERENCE_TIME prtTime)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicBufferImpl_PackStructured (LPDIRECTMUSICBUFFER iface, REFERENCE_TIME rt, DWORD dwChannelGroup, DWORD dwChannelMessage)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicBufferImpl_PackUnstructured (LPDIRECTMUSICBUFFER iface, REFERENCE_TIME rt, DWORD dwChannelGroup, DWORD cb, LPBYTE lpb)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicBufferImpl_ResetReadPtr (LPDIRECTMUSICBUFFER iface)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicBufferImpl_GetNextEvent (LPDIRECTMUSICBUFFER iface, LPREFERENCE_TIME prt, LPDWORD pdwChannelGroup, LPDWORD pdwLength, LPBYTE* ppData)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicBufferImpl_GetRawBufferPtr (LPDIRECTMUSICBUFFER iface, LPBYTE* ppData)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicBufferImpl_GetStartTime (LPDIRECTMUSICBUFFER iface, LPREFERENCE_TIME prt)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicBufferImpl_GetUsedBytes (LPDIRECTMUSICBUFFER iface, LPDWORD pcb)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicBufferImpl_GetMaxBytes (LPDIRECTMUSICBUFFER iface, LPDWORD pcb)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicBufferImpl_GetBufferFormat (LPDIRECTMUSICBUFFER iface, LPGUID pGuidFormat)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicBufferImpl_SetStartTime (LPDIRECTMUSICBUFFER iface, REFERENCE_TIME rt)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicBufferImpl_SetUsedBytes (LPDIRECTMUSICBUFFER iface, DWORD cb)
{
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectMusicBuffer) DirectMusicBuffer_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicBufferImpl_QueryInterface,
	IDirectMusicBufferImpl_AddRef,
	IDirectMusicBufferImpl_Release,
	IDirectMusicBufferImpl_Flush,
	IDirectMusicBufferImpl_TotalTime,
	IDirectMusicBufferImpl_PackStructured,
	IDirectMusicBufferImpl_PackUnstructured,
	IDirectMusicBufferImpl_ResetReadPtr,
	IDirectMusicBufferImpl_GetNextEvent,
	IDirectMusicBufferImpl_GetRawBufferPtr,
	IDirectMusicBufferImpl_GetStartTime,
	IDirectMusicBufferImpl_GetUsedBytes,
	IDirectMusicBufferImpl_GetMaxBytes,
	IDirectMusicBufferImpl_GetBufferFormat,
	IDirectMusicBufferImpl_SetStartTime,
	IDirectMusicBufferImpl_SetUsedBytes
};


/* IDirectMusicObject IUnknown parts follow: */
HRESULT WINAPI IDirectMusicObjectImpl_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicObjectImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicObject))
	{
		IDirectMusicObjectImpl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicObjectImpl_AddRef (LPDIRECTMUSICOBJECT iface)
{
	ICOM_THIS(IDirectMusicObjectImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicObjectImpl_Release (LPDIRECTMUSICOBJECT iface)
{
	ICOM_THIS(IDirectMusicObjectImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicObject Interface follow: */
HRESULT WINAPI IDirectMusicObjectImpl_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicObjectImpl_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicObjectImpl_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc)
{
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectMusicObject) DirectMusicObject_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicObjectImpl_QueryInterface,
	IDirectMusicObjectImpl_AddRef,
	IDirectMusicObjectImpl_Release,
	IDirectMusicObjectImpl_GetDescriptor,
	IDirectMusicObjectImpl_SetDescriptor,
	IDirectMusicObjectImpl_ParseDescriptor
};
