/* IDirectMusic8 Implementation
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
#include "winreg.h"
#include "winuser.h"
#include "wingdi.h"
#include "winerror.h"
#include "mmsystem.h"
#include "winternl.h"
#include "mmddk.h"
#include "wine/windef16.h"
#include "wine/winbase16.h"
#include "wine/debug.h"
#include "dsound.h"

#include "dmusic_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

/* IDirectMusic8 IUnknown parts follow: */
HRESULT WINAPI IDirectMusic8Impl_QueryInterface (LPDIRECTMUSIC8 iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusic8Impl,iface);

	if (IsEqualIID (riid, &IID_IUnknown) || 
	    IsEqualIID (riid, &IID_IDirectMusic2) ||
	    IsEqualIID (riid, &IID_IDirectMusic8)) {
		IDirectMusic8Impl_AddRef(iface);
		*ppobj = This;
		return S_OK;
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
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusic8 Interface follow: */
HRESULT WINAPI IDirectMusic8Impl_EnumPort(LPDIRECTMUSIC8 iface, DWORD dwIndex, LPDMUS_PORTCAPS pPortCaps)
{
	ICOM_THIS(IDirectMusic8Impl,iface);
	
	TRACE("(%p, %ld, %p)\n", This, dwIndex, pPortCaps);
	/* i guess the first port shown is always software synthesizer */
	if (dwIndex == 0) 
	{
		IDirectMusicSynth8* synth;
		TRACE("enumerating 'Microsoft Software Synthesizer' port\n");
		CoCreateInstance (&CLSID_DirectMusicSynth, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicSynth8, (void**)&synth);
		IDirectMusicSynth8_GetPortCaps (synth, pPortCaps);
		IDirectMusicSynth8_Release (synth);
		return S_OK;
	}

/* it seems that the rest of devices are obtained thru dmusic32.EnumLegacyDevices...*sigh*...which is undocumented*/
#if 0
	int numMIDI = midiOutGetNumDevs();
	int numWAVE = waveOutGetNumDevs();
	int i;
	/* then return digital sound ports */
	for (i = 1; i <= numWAVE; i++)
	{
		TRACE("enumerating 'digital sound' ports\n");	
		if (i == dwIndex)
		{
			DirectSoundEnumerateA((LPDSENUMCALLBACKA) register_waveport, (VOID*) pPortCaps);
			return S_OK;	
		}
	}
	/* finally, list all *real* MIDI ports*/
	for (i = numWAVE + 1; i <= numWAVE + numMIDI; i++) 
	{
		TRACE("enumerating 'real MIDI' ports\n");		
		if (i == dwIndex)
			FIXME("Found MIDI port, but *real* MIDI ports not supported yet\n");
	}
#endif	
	return S_FALSE;
}

HRESULT WINAPI IDirectMusic8Impl_CreateMusicBuffer (LPDIRECTMUSIC8 iface, LPDMUS_BUFFERDESC pBufferDesc, LPDIRECTMUSICBUFFER** ppBuffer, LPUNKNOWN pUnkOuter)
{
	ICOM_THIS(IDirectMusic8Impl,iface);

	FIXME("(%p, %p, %p, %p): stub\n", This, pBufferDesc, ppBuffer, pUnkOuter);
	
	return S_OK;
}

HRESULT WINAPI IDirectMusic8Impl_CreatePort (LPDIRECTMUSIC8 iface, REFCLSID rclsidPort, LPDMUS_PORTPARAMS pPortParams, LPDIRECTMUSICPORT* ppPort, LPUNKNOWN pUnkOuter)
{
	ICOM_THIS(IDirectMusic8Impl,iface);
	int i/*, j*/;
	DMUS_PORTCAPS PortCaps;
	
	TRACE("(%p, %s, %p, %p, %p)\n", This, debugstr_guid(rclsidPort), pPortParams, ppPort, pUnkOuter);
	for (i = 0; S_FALSE != IDirectMusic8Impl_EnumPort(iface, i, &PortCaps); i++) {				
		if (IsEqualCLSID (rclsidPort, &PortCaps.guidPort)) {		
			This->ppPorts = HeapReAlloc(GetProcessHeap(), 0, This->ppPorts, sizeof(LPDIRECTMUSICPORT) * This->nrofports);
			if (NULL == This->ppPorts[This->nrofports]) {
				*ppPort = (LPDIRECTMUSICPORT)NULL;
				return E_OUTOFMEMORY;
			}
			This->ppPorts[This->nrofports]->lpVtbl = &DirectMusicPort_Vtbl;
			This->ppPorts[This->nrofports]->ref = 1;
			This->ppPorts[This->nrofports]->fActive = FALSE;
			This->ppPorts[This->nrofports]->pCaps = &PortCaps;
			This->ppPorts[This->nrofports]->pParams = pPortParams; /* this one is here just because there's a funct. which retrieves it back */
			This->ppPorts[This->nrofports]->pDirectSound = NULL;
			DMUSIC_CreateReferenceClock (&IID_IReferenceClock, (IReferenceClock**)&This->ppPorts[This->nrofports]->pLatencyClock, NULL);

#if 0
			if (pPortParams->dwValidParams & DMUS_PORTPARAMS_CHANNELGROUPS) {
				This->ports[This->nrofports]->nrofgroups = pPortParams->dwChannelGroups;
				/* setting default priorities */			
				for (j = 0; j < This->ports[This->nrofports]->nrofgroups; j++) {
					TRACE ("Setting default channel priorities on channel group %i\n", j + 1);
					This->ports[This->nrofports]->group[j].channel[0].priority = DAUD_CHAN1_DEF_VOICE_PRIORITY;
					This->ports[This->nrofports]->group[j].channel[1].priority = DAUD_CHAN2_DEF_VOICE_PRIORITY;
					This->ports[This->nrofports]->group[j].channel[2].priority = DAUD_CHAN3_DEF_VOICE_PRIORITY;
					This->ports[This->nrofports]->group[j].channel[3].priority = DAUD_CHAN4_DEF_VOICE_PRIORITY;
					This->ports[This->nrofports]->group[j].channel[4].priority = DAUD_CHAN5_DEF_VOICE_PRIORITY;
					This->ports[This->nrofports]->group[j].channel[5].priority = DAUD_CHAN6_DEF_VOICE_PRIORITY;
					This->ports[This->nrofports]->group[j].channel[6].priority = DAUD_CHAN7_DEF_VOICE_PRIORITY;
					This->ports[This->nrofports]->group[j].channel[7].priority = DAUD_CHAN8_DEF_VOICE_PRIORITY;
					This->ports[This->nrofports]->group[j].channel[8].priority = DAUD_CHAN9_DEF_VOICE_PRIORITY;
					This->ports[This->nrofports]->group[j].channel[9].priority = DAUD_CHAN10_DEF_VOICE_PRIORITY;
					This->ports[This->nrofports]->group[j].channel[10].priority = DAUD_CHAN11_DEF_VOICE_PRIORITY;
					This->ports[This->nrofports]->group[j].channel[11].priority = DAUD_CHAN12_DEF_VOICE_PRIORITY;
					This->ports[This->nrofports]->group[j].channel[12].priority = DAUD_CHAN13_DEF_VOICE_PRIORITY;
					This->ports[This->nrofports]->group[j].channel[13].priority = DAUD_CHAN14_DEF_VOICE_PRIORITY;
					This->ports[This->nrofports]->group[j].channel[14].priority = DAUD_CHAN15_DEF_VOICE_PRIORITY;
					This->ports[This->nrofports]->group[j].channel[15].priority = DAUD_CHAN16_DEF_VOICE_PRIORITY;
				}
			}
#endif
			*ppPort = (LPDIRECTMUSICPORT) This->ppPorts[This->nrofports];
			This->nrofports++;
			return S_OK;			
		}
	}
	/* FIXME: place correct error here */
	return E_NOINTERFACE;
}

HRESULT WINAPI IDirectMusic8Impl_EnumMasterClock (LPDIRECTMUSIC8 iface, DWORD dwIndex, LPDMUS_CLOCKINFO lpClockInfo)
{
	ICOM_THIS(IDirectMusic8Impl,iface);

	FIXME("(%p, %ld, %p): stub\n", This, dwIndex, lpClockInfo);

	return S_FALSE;
}

HRESULT WINAPI IDirectMusic8Impl_GetMasterClock (LPDIRECTMUSIC8 iface, LPGUID pguidClock, IReferenceClock** ppReferenceClock)
{
	ICOM_THIS(IDirectMusic8Impl,iface);

	TRACE("(%p, %p, %p)\n", This, pguidClock, ppReferenceClock);
	if (pguidClock)
		*pguidClock = This->pMasterClock->pClockInfo.guidClock;
	if(ppReferenceClock)
		*ppReferenceClock = (IReferenceClock *)This->pMasterClock;

	return S_OK;
}

HRESULT WINAPI IDirectMusic8Impl_SetMasterClock (LPDIRECTMUSIC8 iface, REFGUID rguidClock)
{
	ICOM_THIS(IDirectMusic8Impl,iface);
	
	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidClock));
	
	return S_OK;
}

HRESULT WINAPI IDirectMusic8Impl_Activate (LPDIRECTMUSIC8 iface, BOOL fEnable)
{
	ICOM_THIS(IDirectMusic8Impl,iface);
	int i;
	
	FIXME("(%p, %d): stub\n", This, fEnable);
	for (i = 0; i < This->nrofports; i++)	
	{
		This->ppPorts[i]->fActive = fEnable;
	}
	
	return S_OK;
}

HRESULT WINAPI IDirectMusic8Impl_GetDefaultPort (LPDIRECTMUSIC8 iface, LPGUID pguidPort)
{
	ICOM_THIS(IDirectMusic8Impl,iface);
	HKEY hkGUID;
	DWORD returnTypeGUID, sizeOfReturnBuffer = 50;
	char returnBuffer[51];
	GUID defaultPortGUID;
	WCHAR buff[51];

	TRACE("(%p, %p)\n", This, pguidPort);
	if ((RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\DirectMusic\\Defaults" , 0, KEY_READ, &hkGUID) != ERROR_SUCCESS) || 
	    (RegQueryValueExA(hkGUID, "DefaultOutputPort", NULL, &returnTypeGUID, returnBuffer, &sizeOfReturnBuffer) != ERROR_SUCCESS))
	{
		WARN(": registry entry missing\n" );
		*pguidPort = CLSID_DirectMusicSynth;
		return S_OK;
	}
	/* FIXME: Check return types to ensure we're interpreting data right */
	MultiByteToWideChar(CP_ACP, 0, returnBuffer, -1, buff, sizeof(buff) / sizeof(WCHAR));
	CLSIDFromString((LPCOLESTR) buff, &defaultPortGUID);
	*pguidPort = defaultPortGUID;
	
	return S_OK;
}

HRESULT WINAPI IDirectMusic8Impl_SetDirectSound (LPDIRECTMUSIC8 iface, LPDIRECTSOUND pDirectSound, HWND hWnd)
{
	ICOM_THIS(IDirectMusic8Impl,iface);

	FIXME("(%p, %p, %p): stub\n", This, pDirectSound, hWnd);

	return S_OK;
}

HRESULT WINAPI IDirectMusic8Impl_SetExternalMasterClock (LPDIRECTMUSIC8 iface, IReferenceClock* pClock)
{
	ICOM_THIS(IDirectMusic8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pClock);

	return S_OK;
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

/* helper stuff */
void register_waveport (LPGUID lpGUID, LPCSTR lpszDesc, LPCSTR lpszDrvName, LPVOID lpContext)
{
	LPDMUS_PORTCAPS pPortCaps = (LPDMUS_PORTCAPS)lpContext;
	
	pPortCaps->dwSize = sizeof(DMUS_PORTCAPS);
	pPortCaps->dwFlags = DMUS_PC_DLS | DMUS_PC_SOFTWARESYNTH | DMUS_PC_DIRECTSOUND | DMUS_PC_DLS2 | DMUS_PC_AUDIOPATH | DMUS_PC_WAVE;
	pPortCaps->guidPort = *lpGUID;
	pPortCaps->dwClass = DMUS_PC_OUTPUTCLASS;
	pPortCaps->dwType = DMUS_PORT_WINMM_DRIVER;
	pPortCaps->dwMemorySize = DMUS_PC_SYSTEMMEMORY;
	pPortCaps->dwMaxChannelGroups = 2;
	pPortCaps->dwMaxVoices = -1;
	pPortCaps->dwMaxAudioChannels = -1;
	pPortCaps->dwEffectFlags = DMUS_EFFECT_REVERB | DMUS_EFFECT_CHORUS | DMUS_EFFECT_DELAY;
	MultiByteToWideChar (CP_ACP, 0, lpszDesc, -1, pPortCaps->wszDescription, sizeof(pPortCaps->wszDescription)/sizeof(WCHAR));
}

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusic (LPCGUID lpcGUID, LPDIRECTMUSIC8 *ppDM, LPUNKNOWN pUnkOuter)
{
	IDirectMusic8Impl *dmusic;

	TRACE("(%p,%p,%p)\n",lpcGUID, ppDM, pUnkOuter);
	if (IsEqualIID (lpcGUID, &IID_IDirectMusic) ||
	    IsEqualIID (lpcGUID, &IID_IDirectMusic2) ||
	    IsEqualIID (lpcGUID, &IID_IDirectMusic8)) {
		dmusic = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusic8Impl));
		if (NULL == dmusic) {
			*ppDM = (LPDIRECTMUSIC8) NULL;
			return E_OUTOFMEMORY;
		}
		dmusic->lpVtbl = &DirectMusic8_Vtbl;
		dmusic->ref = 1;
		dmusic->pMasterClock = NULL;
		dmusic->ppPorts = NULL;
		dmusic->nrofports = 0;
		DMUSIC_CreateReferenceClock (&IID_IReferenceClock, (IReferenceClock**)&dmusic->pMasterClock, NULL);
		
		*ppDM = (LPDIRECTMUSIC8) dmusic;
		return S_OK;
	}
	
	WARN("No interface found\n");
	return E_NOINTERFACE;
}
