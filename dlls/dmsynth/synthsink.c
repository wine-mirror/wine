/* IDirectMusicSynthSink Implementation
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

#include "dmsynth_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

/* IDirectMusicSynthSink IUnknown parts follow: */
HRESULT WINAPI IDirectMusicSynthSinkImpl_QueryInterface (LPDIRECTMUSICSYNTHSINK iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicSynthSinkImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || 
	    IsEqualGUID(riid, &IID_IDirectMusicSynthSink))
	{
		IDirectMusicSynthSinkImpl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicSynthSinkImpl_AddRef (LPDIRECTMUSICSYNTHSINK iface)
{
	ICOM_THIS(IDirectMusicSynthSinkImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicSynthSinkImpl_Release (LPDIRECTMUSICSYNTHSINK iface)
{
	ICOM_THIS(IDirectMusicSynthSinkImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicSynth Interface follow: */
HRESULT WINAPI IDirectMusicSynthSinkImpl_Init (LPDIRECTMUSICSYNTHSINK iface, IDirectMusicSynth* pSynth)
{
	ICOM_THIS(IDirectMusicSynthSinkImpl,iface);

	FIXME("(%p, %p): stub\n", This, pSynth);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSynthSinkImpl_SetMasterClock (LPDIRECTMUSICSYNTHSINK iface, IReferenceClock* pClock)
{
	ICOM_THIS(IDirectMusicSynthSinkImpl,iface);

	FIXME("(%p, %p): stub\n", This, pClock);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSynthSinkImpl_GetLatencyClock (LPDIRECTMUSICSYNTHSINK iface, IReferenceClock** ppClock)
{
	ICOM_THIS(IDirectMusicSynthSinkImpl,iface);

	FIXME("(%p, %p): stub\n", This, ppClock);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSynthSinkImpl_Activate (LPDIRECTMUSICSYNTHSINK iface, BOOL fEnable)
{
	ICOM_THIS(IDirectMusicSynthSinkImpl,iface);

	FIXME("(%p, %d): stub\n", This, fEnable);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSynthSinkImpl_SampleToRefTime (LPDIRECTMUSICSYNTHSINK iface, LONGLONG llSampleTime, REFERENCE_TIME* prfTime)
{
	ICOM_THIS(IDirectMusicSynthSinkImpl,iface);

	FIXME("(%p, %lli, %p): stub\n", This, llSampleTime, prfTime);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSynthSinkImpl_RefTimeToSample (LPDIRECTMUSICSYNTHSINK iface, REFERENCE_TIME rfTime, LONGLONG* pllSampleTime)
{
	ICOM_THIS(IDirectMusicSynthSinkImpl,iface);

	FIXME("(%p, %lli, %p): stub\n", This, rfTime, pllSampleTime );

	return S_OK;
}

HRESULT WINAPI IDirectMusicSynthSinkImpl_SetDirectSound (LPDIRECTMUSICSYNTHSINK iface, LPDIRECTSOUND pDirectSound, LPDIRECTSOUNDBUFFER pDirectSoundBuffer)
{
	ICOM_THIS(IDirectMusicSynthSinkImpl,iface);

	FIXME("(%p, %p, %p): stub\n", This, pDirectSound, pDirectSoundBuffer);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSynthSinkImpl_GetDesiredBufferSize (LPDIRECTMUSICSYNTHSINK iface, LPDWORD pdwBufferSizeInSamples)
{
	ICOM_THIS(IDirectMusicSynthSinkImpl,iface);

	FIXME("(%p, %p): stub\n", This, pdwBufferSizeInSamples);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicSynthSink) DirectMusicSynthSink_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicSynthSinkImpl_QueryInterface,
	IDirectMusicSynthSinkImpl_AddRef,
	IDirectMusicSynthSinkImpl_Release,
	IDirectMusicSynthSinkImpl_Init,
	IDirectMusicSynthSinkImpl_SetMasterClock,
	IDirectMusicSynthSinkImpl_GetLatencyClock,
	IDirectMusicSynthSinkImpl_Activate,
	IDirectMusicSynthSinkImpl_SampleToRefTime,
	IDirectMusicSynthSinkImpl_RefTimeToSample,
	IDirectMusicSynthSinkImpl_SetDirectSound,
	IDirectMusicSynthSinkImpl_GetDesiredBufferSize
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicSynthSink (LPCGUID lpcGUID, LPDIRECTMUSICSYNTHSINK* ppDMSynthSink, LPUNKNOWN pUnkOuter)
{
	IDirectMusicSynthSinkImpl *dmsink;
	
	TRACE("(%p,%p,%p)\n", lpcGUID, ppDMSynthSink, pUnkOuter);
	if (IsEqualGUID (lpcGUID, &IID_IDirectMusicSynthSink)) {
		dmsink = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicSynthSinkImpl));
		if (NULL == dmsink) {
			*ppDMSynthSink = (LPDIRECTMUSICSYNTHSINK) NULL;
			return E_OUTOFMEMORY;
		}
		dmsink->lpVtbl = &DirectMusicSynthSink_Vtbl;
		dmsink->ref = 1;
		*ppDMSynthSink = (LPDIRECTMUSICSYNTHSINK) dmsink;
		return S_OK;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;
}
