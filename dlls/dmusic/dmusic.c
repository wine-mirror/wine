/*
 * IDirectMusic8 Implementation
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
 * Copyright (C) 2012 Christian Costa
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

#include <stdio.h>

#include "dmusic_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

static inline IDirectMusic8Impl *impl_from_IDirectMusic8(IDirectMusic8 *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusic8Impl, IDirectMusic8_iface);
}

/* IDirectMusic8Impl IUnknown part: */
static HRESULT WINAPI IDirectMusic8Impl_QueryInterface(LPDIRECTMUSIC8 iface, REFIID riid, LPVOID *ret_iface)
{
    IDirectMusic8Impl *This = impl_from_IDirectMusic8(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_dmguid(riid), ret_iface);

    if (IsEqualIID (riid, &IID_IUnknown) ||
        IsEqualIID (riid, &IID_IDirectMusic) ||
        IsEqualIID (riid, &IID_IDirectMusic2) ||
        IsEqualIID (riid, &IID_IDirectMusic8))
    {
        IDirectMusic8_AddRef(iface);
        *ret_iface = iface;
        return S_OK;
    }

    *ret_iface = NULL;

    WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ret_iface);

    return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusic8Impl_AddRef(LPDIRECTMUSIC8 iface)
{
    IDirectMusic8Impl *This = impl_from_IDirectMusic8(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", This, ref);

    DMUSIC_LockModule();

    return ref;
}

static ULONG WINAPI IDirectMusic8Impl_Release(LPDIRECTMUSIC8 iface)
{
    IDirectMusic8Impl *This = impl_from_IDirectMusic8(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", This, ref);

    if (!ref) {
        HeapFree(GetProcessHeap(), 0, This->ppPorts);
        HeapFree(GetProcessHeap(), 0, This);
    }

    DMUSIC_UnlockModule();

    return ref;
}

/* IDirectMusic8Impl IDirectMusic part: */
static HRESULT WINAPI IDirectMusic8Impl_EnumPort(LPDIRECTMUSIC8 iface, DWORD index, LPDMUS_PORTCAPS port_caps)
{
    IDirectMusic8Impl *This = impl_from_IDirectMusic8(iface);
    ULONG nb_midi_out;
    ULONG nb_midi_in;
    const WCHAR emulated[] = {' ','[','E','m','u','l','a','t','e','d',']',0};

    TRACE("(%p, %d, %p)\n", This, index, port_caps);

    if (!port_caps)
        return E_POINTER;

    /* NOTE: It seems some native versions get the rest of devices through dmusic32.EnumLegacyDevices...*sigh*...which is undocumented */

    /* NOTE: Should we enum wave devices ? Native does not seem to */

    /* Fill common port caps for winmm ports */
    port_caps->dwType = DMUS_PORT_WINMM_DRIVER;
    port_caps->dwMemorySize = 0;
    port_caps->dwMaxChannelGroups = 1;
    port_caps->dwMaxVoices = 0;
    port_caps->dwMaxAudioChannels = 0;
    port_caps->dwEffectFlags = DMUS_EFFECT_NONE;
    /* Fake port GUID */
    port_caps->guidPort = IID_IUnknown;
    port_caps->guidPort.Data1 = index + 1;

    nb_midi_out = midiOutGetNumDevs();

    if (index == 0)
    {
        MIDIOUTCAPSW caps;
        midiOutGetDevCapsW(MIDI_MAPPER, &caps, sizeof(caps));
        strcpyW(port_caps->wszDescription, caps.szPname);
        strcatW(port_caps->wszDescription, emulated);
        port_caps->dwFlags = DMUS_PC_SHAREABLE;
        port_caps->dwClass = DMUS_PC_OUTPUTCLASS;
        TRACE("Enumerating port: %s\n", debugstr_w(port_caps->wszDescription));
        return S_OK;
    }

    if (index < (nb_midi_out + 1))
    {
        MIDIOUTCAPSW caps;
        midiOutGetDevCapsW(index - 1, &caps, sizeof(caps));
        strcpyW(port_caps->wszDescription, caps.szPname);
        strcatW(port_caps->wszDescription, emulated);
        port_caps->dwFlags = DMUS_PC_SHAREABLE | DMUS_PC_EXTERNAL;
        port_caps->dwClass = DMUS_PC_OUTPUTCLASS;
        TRACE("Enumerating port: %s\n", debugstr_w(port_caps->wszDescription));
        return S_OK;
    }

    nb_midi_in = midiInGetNumDevs();

    if (index < (nb_midi_in + nb_midi_out + 1))
    {
        MIDIINCAPSW caps;
        midiInGetDevCapsW(index - nb_midi_out - 1, &caps, sizeof(caps));
        strcpyW(port_caps->wszDescription, caps.szPname);
        strcatW(port_caps->wszDescription, emulated);
        port_caps->dwFlags = DMUS_PC_EXTERNAL;
        port_caps->dwClass = DMUS_PC_INPUTCLASS;
        TRACE("Enumerating port: %s\n", debugstr_w(port_caps->wszDescription));
        return S_OK;
    }

    if (index == (nb_midi_in + nb_midi_out + 1))
    {
        IDirectMusicSynth8* synth = NULL;
        HRESULT hr;
        hr = CoCreateInstance(&CLSID_DirectMusicSynth, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicSynth8, (void**)&synth);
        if (SUCCEEDED(hr))
            hr = IDirectMusicSynth8_GetPortCaps(synth, port_caps);
        if (SUCCEEDED(hr))
            TRACE("Enumerating port: %s\n", debugstr_w(port_caps->wszDescription));
        if (synth)
            IDirectMusicSynth8_Release(synth);
        return hr;
    }

    return S_FALSE;
}

static HRESULT WINAPI IDirectMusic8Impl_CreateMusicBuffer(LPDIRECTMUSIC8 iface, LPDMUS_BUFFERDESC buffer_desc, LPDIRECTMUSICBUFFER* buffer, LPUNKNOWN unkouter)
{
    IDirectMusic8Impl *This = impl_from_IDirectMusic8(iface);

    TRACE("(%p)->(%p, %p, %p)\n", This, buffer_desc, buffer, unkouter);

    if (unkouter)
        return CLASS_E_NOAGGREGATION;

    if (!buffer_desc || !buffer)
        return E_POINTER;

    return DMUSIC_CreateDirectMusicBufferImpl(buffer_desc, (LPVOID)buffer);
}

static HRESULT WINAPI IDirectMusic8Impl_CreatePort(LPDIRECTMUSIC8 iface, REFCLSID rclsidPort, LPDMUS_PORTPARAMS pPortParams, LPDIRECTMUSICPORT* ppPort, LPUNKNOWN pUnkOuter)
{
	IDirectMusic8Impl *This = impl_from_IDirectMusic8(iface);
	int i;
	DMUS_PORTCAPS PortCaps;
	IDirectMusicPort* pNewPort = NULL;
	HRESULT hr;
	GUID default_port;
	const GUID *request_port = rclsidPort;

	TRACE("(%p, %s, %p, %p, %p)\n", This, debugstr_dmguid(rclsidPort), pPortParams, ppPort, pUnkOuter);

	if (TRACE_ON(dmusic))
		dump_DMUS_PORTPARAMS(pPortParams);

	if (!rclsidPort)
		return E_POINTER;
	if (!pPortParams)
		return E_INVALIDARG;
	if (!ppPort)
		return E_POINTER;
	if (pUnkOuter)
		return CLASS_E_NOAGGREGATION;

	if (TRACE_ON(dmusic))
		dump_DMUS_PORTPARAMS(pPortParams);

	ZeroMemory(&PortCaps, sizeof(DMUS_PORTCAPS));
	PortCaps.dwSize = sizeof(DMUS_PORTCAPS);

	if(IsEqualGUID(request_port, &GUID_NULL)){
		hr = IDirectMusic8_GetDefaultPort(iface, &default_port);
		if(FAILED(hr))
			return hr;
		request_port = &default_port;
	}

	for (i = 0; S_FALSE != IDirectMusic8Impl_EnumPort(iface, i, &PortCaps); i++) {
		if (IsEqualCLSID (request_port, &PortCaps.guidPort)) {
			hr = DMUSIC_CreateDirectMusicPortImpl(&IID_IDirectMusicPort, (LPVOID*) &pNewPort, (LPUNKNOWN) This, pPortParams, &PortCaps);
			if (FAILED(hr)) {
			  *ppPort = NULL;
			  return hr;
			}
			This->nrofports++;
			if (!This->ppPorts) This->ppPorts = HeapAlloc(GetProcessHeap(), 0, sizeof(LPDIRECTMUSICPORT) * This->nrofports);
			else This->ppPorts = HeapReAlloc(GetProcessHeap(), 0, This->ppPorts, sizeof(LPDIRECTMUSICPORT) * This->nrofports);
			This->ppPorts[This->nrofports - 1] = pNewPort;
			*ppPort = pNewPort;
			return S_OK;
		}
	}
	return E_NOINTERFACE;
}

static HRESULT WINAPI IDirectMusic8Impl_EnumMasterClock(LPDIRECTMUSIC8 iface, DWORD index, LPDMUS_CLOCKINFO clock_info)
{
    TRACE("(%p)->(%d, %p)\n", iface, index, clock_info);

    if (!clock_info)
        return E_POINTER;

    if (index > 1)
        return S_FALSE;

    if (!index)
    {
        static const GUID guid_system_clock = { 0x58d58419, 0x71b4, 0x11d1, { 0xa7, 0x4c, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12 } };
        static const WCHAR name_system_clock[] = { 'S','y','s','t','e','m',' ','C','l','o','c','k',0 };

        clock_info->ctType = 0;
        clock_info->guidClock = guid_system_clock;
        strcpyW(clock_info->wszDescription, name_system_clock);
    }
    else
    {
        static const GUID guid_dsound_clock = { 0x58d58420, 0x71b4, 0x11d1, { 0xa7, 0x4c, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12 } };
        static const WCHAR name_dsound_clock[] = { 'D','i','r','e','c','t','S','o','u','n','d',' ','C','l','o','c','k',0 };

        clock_info->ctType = 0;
        clock_info->guidClock = guid_dsound_clock;
        strcpyW(clock_info->wszDescription, name_dsound_clock);
    }

    return S_OK;
}

static HRESULT WINAPI IDirectMusic8Impl_GetMasterClock(LPDIRECTMUSIC8 iface, LPGUID pguidClock, IReferenceClock** ppReferenceClock)
{
	IDirectMusic8Impl *This = impl_from_IDirectMusic8(iface);

	TRACE("(%p, %p, %p)\n", This, pguidClock, ppReferenceClock);
	if (pguidClock)
		*pguidClock = This->pMasterClock->pClockInfo.guidClock;
	if(ppReferenceClock)
		*ppReferenceClock = (IReferenceClock *)This->pMasterClock;

	return S_OK;
}

static HRESULT WINAPI IDirectMusic8Impl_SetMasterClock(LPDIRECTMUSIC8 iface, REFGUID rguidClock)
{
    IDirectMusic8Impl *This = impl_from_IDirectMusic8(iface);

    FIXME("(%p)->(%s): stub\n", This, debugstr_dmguid(rguidClock));

    return S_OK;
}

static HRESULT WINAPI IDirectMusic8Impl_Activate(LPDIRECTMUSIC8 iface, BOOL enable)
{
    IDirectMusic8Impl *This = impl_from_IDirectMusic8(iface);
    int i;
    HRESULT hr;

    TRACE("(%p)->(%u)\n", This, enable);

    for (i = 0; i < This->nrofports; i++)
    {
        hr = IDirectMusicPort_Activate(This->ppPorts[i], enable);
        if (FAILED(hr))
            return hr;
    }

    return S_OK;
}

static HRESULT WINAPI IDirectMusic8Impl_GetDefaultPort(LPDIRECTMUSIC8 iface, LPGUID pguidPort)
{
	IDirectMusic8Impl *This = impl_from_IDirectMusic8(iface);
	HKEY hkGUID;
	DWORD returnTypeGUID, sizeOfReturnBuffer = 50;
	char returnBuffer[51];
	GUID defaultPortGUID;
	WCHAR buff[51];

	TRACE("(%p, %p)\n", This, pguidPort);
	if ((RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\DirectMusic\\Defaults" , 0, KEY_READ, &hkGUID) != ERROR_SUCCESS) || 
	    (RegQueryValueExA(hkGUID, "DefaultOutputPort", NULL, &returnTypeGUID, (LPBYTE)returnBuffer, &sizeOfReturnBuffer) != ERROR_SUCCESS))
	{
		WARN(": registry entry missing\n" );
		*pguidPort = CLSID_DirectMusicSynth;
		return S_OK;
	}
	/* FIXME: Check return types to ensure we're interpreting data right */
	MultiByteToWideChar(CP_ACP, 0, returnBuffer, -1, buff, sizeof(buff) / sizeof(WCHAR));
	CLSIDFromString(buff, &defaultPortGUID);
	*pguidPort = defaultPortGUID;
	
	return S_OK;
}

static HRESULT WINAPI IDirectMusic8Impl_SetDirectSound(LPDIRECTMUSIC8 iface, LPDIRECTSOUND dsound, HWND wnd)
{
    IDirectMusic8Impl *This = impl_from_IDirectMusic8(iface);

    FIXME("(%p)->(%p, %p): stub\n", This, dsound, wnd);

    return S_OK;
}

static HRESULT WINAPI IDirectMusic8Impl_SetExternalMasterClock(LPDIRECTMUSIC8 iface, IReferenceClock* clock)
{
    IDirectMusic8Impl *This = impl_from_IDirectMusic8(iface);

    FIXME("(%p)->(%p): stub\n", This, clock);

    return S_OK;
}

static const IDirectMusic8Vtbl DirectMusic8_Vtbl = {
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

/* For ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicImpl(LPCGUID riid, LPVOID* ret_iface, LPUNKNOWN unkouter)
{
    IDirectMusic8Impl *dmusic;
    HRESULT ret;

    TRACE("(%p,%p,%p)\n", riid, ret_iface, unkouter);

    *ret_iface = NULL;

    dmusic = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusic8Impl));
    if (!dmusic)
        return E_OUTOFMEMORY;

    dmusic->IDirectMusic8_iface.lpVtbl = &DirectMusic8_Vtbl;
    dmusic->ref = 0; /* Will be inited by QueryInterface */
    dmusic->pMasterClock = NULL;
    dmusic->ppPorts = NULL;
    dmusic->nrofports = 0;
    ret = DMUSIC_CreateReferenceClockImpl(&IID_IReferenceClock, (LPVOID*)&dmusic->pMasterClock, NULL);
    if (FAILED(ret)) {
        HeapFree(GetProcessHeap(), 0, dmusic);
        return ret;
    }

    ret = IDirectMusic8Impl_QueryInterface(&dmusic->IDirectMusic8_iface, riid, ret_iface);
    if (FAILED(ret)) {
        IReferenceClock_Release(&dmusic->pMasterClock->IReferenceClock_iface);
        HeapFree(GetProcessHeap(), 0, dmusic);
        return ret;
    }

    return S_OK;
}
