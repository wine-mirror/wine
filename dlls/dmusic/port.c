/*
 * IDirectMusicPort Implementation
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

static inline IDirectMusicPortImpl *impl_from_IDirectMusicPort(IDirectMusicPort *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicPortImpl, IDirectMusicPort_iface);
}

static inline IDirectMusicPortImpl *impl_from_IDirectMusicPortDownload(IDirectMusicPortDownload *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicPortImpl, IDirectMusicPortDownload_iface);
}

static inline IDirectMusicPortImpl *impl_from_IDirectMusicThru(IDirectMusicThru *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicPortImpl, IDirectMusicThru_iface);
}

/* IDirectMusicPortImpl IUnknown part: */
static HRESULT WINAPI IDirectMusicPortImpl_QueryInterface(LPDIRECTMUSICPORT iface, REFIID riid, LPVOID *ppobj)
{
	IDirectMusicPortImpl *This = impl_from_IDirectMusicPort(iface);

	TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);

	if (IsEqualIID (riid, &IID_IUnknown) ||
	    IsEqualGUID(riid, &IID_IDirectMusicPort) ||
	    IsEqualGUID(riid, &IID_IDirectMusicPort8)) {
		*ppobj = &This->IDirectMusicPort_iface;
		IDirectMusicPort_AddRef((LPDIRECTMUSICPORT)*ppobj);
		return S_OK;
	} else if (IsEqualGUID(riid, &IID_IDirectMusicPortDownload) ||
		   IsEqualGUID(riid, &IID_IDirectMusicPortDownload8)) {
		*ppobj = &This->IDirectMusicPortDownload_iface;
		IDirectMusicPortDownload_AddRef((LPDIRECTMUSICPORTDOWNLOAD)*ppobj);
		return S_OK;
	} else if (IsEqualGUID(riid, &IID_IDirectMusicThru) ||
		   IsEqualGUID(riid, &IID_IDirectMusicThru8)) {
		*ppobj = &This->IDirectMusicThru_iface;
		IDirectMusicThru_AddRef((LPDIRECTMUSICTHRU)*ppobj);
		return S_OK;
	}
	WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicPortImpl_AddRef(LPDIRECTMUSICPORT iface)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicPort(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", This, ref);

    DMUSIC_LockModule();

    return ref;
}

static ULONG WINAPI IDirectMusicPortImpl_Release(LPDIRECTMUSICPORT iface)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicPort(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", This, ref);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    DMUSIC_UnlockModule();

    return ref;
}

/* IDirectMusicPortImpl IDirectMusicPort part: */
static HRESULT WINAPI IDirectMusicPortImpl_PlayBuffer(LPDIRECTMUSICPORT iface, LPDIRECTMUSICBUFFER buffer)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicPort(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, buffer);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_SetReadNotificationHandle(LPDIRECTMUSICPORT iface, HANDLE event)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicPort(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, event);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_Read(LPDIRECTMUSICPORT iface, LPDIRECTMUSICBUFFER buffer)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicPort(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, buffer);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_DownloadInstrument(LPDIRECTMUSICPORT iface, IDirectMusicInstrument* pInstrument, IDirectMusicDownloadedInstrument** ppDownloadedInstrument, DMUS_NOTERANGE* pNoteRanges, DWORD dwNumNoteRanges)
{
	IDirectMusicPortImpl *This = impl_from_IDirectMusicPort(iface);

	FIXME("(%p, %p, %p, %p, %d): stub\n", This, pInstrument, ppDownloadedInstrument, pNoteRanges, dwNumNoteRanges);

	if (!pInstrument || !ppDownloadedInstrument || (dwNumNoteRanges && !pNoteRanges))
		return E_POINTER;

	return DMUSIC_CreateDirectMusicDownloadedInstrumentImpl(&IID_IDirectMusicDownloadedInstrument, (LPVOID*)ppDownloadedInstrument, NULL);
}

static HRESULT WINAPI IDirectMusicPortImpl_UnloadInstrument(LPDIRECTMUSICPORT iface, IDirectMusicDownloadedInstrument *downloaded_instrument)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicPort(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, downloaded_instrument);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_GetLatencyClock(LPDIRECTMUSICPORT iface, IReferenceClock** clock)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicPort(iface);

    TRACE("(%p/%p)->(%p)\n", iface, This, clock);

    *clock = This->pLatencyClock;
    IReferenceClock_AddRef(*clock);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_GetRunningStats(LPDIRECTMUSICPORT iface, LPDMUS_SYNTHSTATS stats)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicPort(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, stats);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_Compact(LPDIRECTMUSICPORT iface)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicPort(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_GetCaps(LPDIRECTMUSICPORT iface, LPDMUS_PORTCAPS port_caps)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicPort(iface);

    TRACE("(%p/%p)->(%p)\n", iface, This, port_caps);

    *port_caps = This->caps;

    return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_DeviceIoControl(LPDIRECTMUSICPORT iface, DWORD io_control_code, LPVOID in_buffer, DWORD in_buffer_size,
                                                           LPVOID out_buffer, DWORD out_buffer_size, LPDWORD bytes_returned, LPOVERLAPPED overlapped)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicPort(iface);

    FIXME("(%p/%p)->(%d, %p, %d, %p, %d, %p, %p): stub\n", iface, This, io_control_code, in_buffer, in_buffer_size, out_buffer, out_buffer_size, bytes_returned, overlapped);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_SetNumChannelGroups(LPDIRECTMUSICPORT iface, DWORD channel_groups)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicPort(iface);

    FIXME("(%p/%p)->(%d): semi-stub\n", iface, This, channel_groups);

    This->nrofgroups = channel_groups;

    return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_GetNumChannelGroups(LPDIRECTMUSICPORT iface, LPDWORD channel_groups)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicPort(iface);

    TRACE("(%p/%p)->(%p)\n", iface, This, channel_groups);

    *channel_groups = This->nrofgroups;

    return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_Activate(LPDIRECTMUSICPORT iface, BOOL active)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicPort(iface);

    TRACE("(%p/%p)->(%d)\n", iface, This, active);

    This->fActive = active;

    return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_SetChannelPriority(LPDIRECTMUSICPORT iface, DWORD channel_group, DWORD channel, DWORD priority)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicPort(iface);

    FIXME("(%p/%p)->(%d, %d, %d): semi-stub\n", iface, This, channel_group, channel, priority);

    if (channel > 16)
    {
        WARN("isn't there supposed to be 16 channels (no. %d requested)?! (faking as it is ok)\n", channel);
        /*return E_INVALIDARG;*/
    }

    return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_GetChannelPriority(LPDIRECTMUSICPORT iface, DWORD channel_group, DWORD channel, LPDWORD priority)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicPort(iface);

    TRACE("(%p/%p)->(%u, %u, %p)\n", iface, This, channel_group, channel, priority);

    *priority = This->group[channel_group - 1].channel[channel].priority;

    return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_SetDirectSound(LPDIRECTMUSICPORT iface, LPDIRECTSOUND direct_sound, LPDIRECTSOUNDBUFFER direct_sound_buffer)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicPort(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, direct_sound, direct_sound_buffer);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_GetFormat(LPDIRECTMUSICPORT iface, LPWAVEFORMATEX pWaveFormatEx, LPDWORD pdwWaveFormatExSize, LPDWORD pdwBufferSize)
{
	IDirectMusicPortImpl *This = impl_from_IDirectMusicPort(iface);
	WAVEFORMATEX format;
	FIXME("(%p, %p, %p, %p): stub\n", This, pWaveFormatEx, pdwWaveFormatExSize, pdwBufferSize);

	if (pWaveFormatEx == NULL)
	{
		if (pdwWaveFormatExSize)
			*pdwWaveFormatExSize = sizeof(format);
		else
			return E_POINTER;
	}
	else
	{
		if (pdwWaveFormatExSize == NULL)
			return E_POINTER;

		/* Just fill this in with something that will not crash Direct Sound for now. */
		/* It won't be used anyway until Performances are completed */
		format.wFormatTag = WAVE_FORMAT_PCM;
		format.nChannels = 2; /* This->params.dwAudioChannels; */
		format.nSamplesPerSec = 44100; /* This->params.dwSampleRate; */
		format.wBitsPerSample = 16;	/* FIXME: check this */
		format.nBlockAlign = (format.wBitsPerSample * format.nChannels) / 8;
		format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
		format.cbSize = 0;

		if (*pdwWaveFormatExSize >= sizeof(format))
		{
			CopyMemory(pWaveFormatEx, &format, min(sizeof(format), *pdwWaveFormatExSize));
			*pdwWaveFormatExSize = sizeof(format);	/* FIXME check if this is set */
		}
		else
			return E_POINTER;	/* FIXME find right error */
	}

	if (pdwBufferSize)
		*pdwBufferSize = 44100 * 2 * 2;
	else
		return E_POINTER;

	return S_OK;
}

static const IDirectMusicPortVtbl DirectMusicPort_Vtbl = {
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
	IDirectMusicPortImpl_Compact,
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
static HRESULT WINAPI IDirectMusicPortDownloadImpl_QueryInterface(LPDIRECTMUSICPORTDOWNLOAD iface, REFIID riid, LPVOID *ret_iface)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicPortDownload(iface);

    TRACE("(%p/%p)->(%s, %p)\n", iface, This, debugstr_dmguid(riid), ret_iface);

    return IDirectMusicPort_QueryInterface(&This->IDirectMusicPort_iface, riid, ret_iface);
}

static ULONG WINAPI IDirectMusicPortDownloadImpl_AddRef (LPDIRECTMUSICPORTDOWNLOAD iface)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicPortDownload(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return IDirectMusicPort_AddRef(&This->IDirectMusicPort_iface);
}

static ULONG WINAPI IDirectMusicPortDownloadImpl_Release(LPDIRECTMUSICPORTDOWNLOAD iface)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicPortDownload(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return IDirectMusicPort_Release(&This->IDirectMusicPort_iface);
}

/* IDirectMusicPortDownload Interface follow: */
static HRESULT WINAPI IDirectMusicPortDownloadImpl_GetBuffer(LPDIRECTMUSICPORTDOWNLOAD iface, DWORD DLId, IDirectMusicDownload** IDMDownload)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicPortDownload(iface);

    FIXME("(%p/%p)->(%u, %p): stub\n", iface, This, DLId, IDMDownload);

    if (!IDMDownload)
        return E_POINTER;

    return DMUSIC_CreateDirectMusicDownloadImpl(&IID_IDirectMusicDownload, (LPVOID*)IDMDownload, NULL);
}

static HRESULT WINAPI IDirectMusicPortDownloadImpl_AllocateBuffer(LPDIRECTMUSICPORTDOWNLOAD iface, DWORD size, IDirectMusicDownload** IDMDownload)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicPortDownload(iface);

    FIXME("(%p/%p)->(%u, %p): stub\n", iface, This, size, IDMDownload);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicPortDownloadImpl_GetDLId(LPDIRECTMUSICPORTDOWNLOAD iface, DWORD* start_DLId, DWORD count)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicPortDownload(iface);

    FIXME("(%p/%p)->(%p, %u): stub\n", iface, This, start_DLId, count);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicPortDownloadImpl_GetAppend (LPDIRECTMUSICPORTDOWNLOAD iface, DWORD* append)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicPortDownload(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, append);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicPortDownloadImpl_Download(LPDIRECTMUSICPORTDOWNLOAD iface, IDirectMusicDownload* IDMDownload)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicPortDownload(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, IDMDownload);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicPortDownloadImpl_Unload(LPDIRECTMUSICPORTDOWNLOAD iface, IDirectMusicDownload* IDMDownload)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicPortDownload(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, IDMDownload);

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

/* IDirectMusicThru IUnknown parts follow: */
static HRESULT WINAPI IDirectMusicThruImpl_QueryInterface(LPDIRECTMUSICTHRU iface, REFIID riid, LPVOID *ret_iface)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicThru(iface);

    TRACE("(%p/%p)->(%s, %p)\n", iface, This, debugstr_dmguid(riid), ret_iface);

    return IDirectMusicPort_QueryInterface(&This->IDirectMusicPort_iface, riid, ret_iface);
}

static ULONG WINAPI IDirectMusicThruImpl_AddRef(LPDIRECTMUSICTHRU iface)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicThru(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return IDirectMusicPort_AddRef(&This->IDirectMusicPort_iface);
}

static ULONG WINAPI IDirectMusicThruImpl_Release(LPDIRECTMUSICTHRU iface)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicThru(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return IDirectMusicPort_Release(&This->IDirectMusicPort_iface);
}

/* IDirectMusicThru Interface follow: */
static HRESULT WINAPI IDirectMusicThruImpl_ThruChannel(LPDIRECTMUSICTHRU iface, DWORD source_channel_group, DWORD source_channel, DWORD destination_channel_group,
                                                       DWORD destination_channel, LPDIRECTMUSICPORT destination_port)
{
    IDirectMusicPortImpl *This = impl_from_IDirectMusicThru(iface);

    FIXME("(%p/%p)->(%d, %d, %d, %d, %p): stub\n", iface, This, source_channel_group, source_channel, destination_channel_group, destination_channel, destination_port);

    return S_OK;
}

static const IDirectMusicThruVtbl DirectMusicThru_Vtbl = {
	IDirectMusicThruImpl_QueryInterface,
	IDirectMusicThruImpl_AddRef,
	IDirectMusicThruImpl_Release,
	IDirectMusicThruImpl_ThruChannel
};

HRESULT DMUSIC_CreateSynthPortImpl(LPCGUID lpcGUID, LPVOID *ppobj, LPUNKNOWN pUnkOuter, LPDMUS_PORTPARAMS pPortParams, LPDMUS_PORTCAPS pPortCaps, DWORD device)
{
	IDirectMusicPortImpl *obj;
	HRESULT hr = E_FAIL;
	UINT j;

	TRACE("(%p,%p,%p,%d)\n", lpcGUID, ppobj, pUnkOuter, device);

	obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicPortImpl));
	if (NULL == obj) {
		*ppobj = NULL;
		return E_OUTOFMEMORY;
	}
	obj->IDirectMusicPort_iface.lpVtbl = &DirectMusicPort_Vtbl;
	obj->IDirectMusicPortDownload_iface.lpVtbl = &DirectMusicPortDownload_Vtbl;
	obj->IDirectMusicThru_iface.lpVtbl = &DirectMusicThru_Vtbl;
	obj->ref = 0;  /* will be inited by QueryInterface */
	obj->fActive = FALSE;
	obj->params = *pPortParams;
	obj->caps = *pPortCaps;
	obj->pDirectSound = NULL;
	obj->pLatencyClock = NULL;
	hr = DMUSIC_CreateReferenceClockImpl(&IID_IReferenceClock, (LPVOID*)&obj->pLatencyClock, NULL);
	if(hr != S_OK)
	{
		HeapFree(GetProcessHeap(), 0, obj);
		return hr;
	}

if(0)
{
	if (pPortParams->dwValidParams & DMUS_PORTPARAMS_CHANNELGROUPS) {
	  obj->nrofgroups = pPortParams->dwChannelGroups;
	  /* setting default priorities */			
	  for (j = 0; j < obj->nrofgroups; j++) {
	    TRACE ("Setting default channel priorities on channel group %i\n", j + 1);
	    obj->group[j].channel[0].priority = DAUD_CHAN1_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[1].priority = DAUD_CHAN2_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[2].priority = DAUD_CHAN3_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[3].priority = DAUD_CHAN4_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[4].priority = DAUD_CHAN5_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[5].priority = DAUD_CHAN6_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[6].priority = DAUD_CHAN7_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[7].priority = DAUD_CHAN8_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[8].priority = DAUD_CHAN9_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[9].priority = DAUD_CHAN10_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[10].priority = DAUD_CHAN11_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[11].priority = DAUD_CHAN12_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[12].priority = DAUD_CHAN13_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[13].priority = DAUD_CHAN14_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[14].priority = DAUD_CHAN15_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[15].priority = DAUD_CHAN16_DEF_VOICE_PRIORITY;
	  }
	}
}

	return IDirectMusicPortImpl_QueryInterface ((LPDIRECTMUSICPORT)obj, lpcGUID, ppobj);
}

HRESULT DMUSIC_CreateMidiOutPortImpl(LPCGUID guid, LPVOID *object, LPUNKNOWN unkouter, LPDMUS_PORTPARAMS port_params, LPDMUS_PORTCAPS port_caps, DWORD device)
{
    TRACE("(%p,%p,%p,%p,%p,%d): stub\n", guid, object, unkouter, port_params, port_caps, device);

    return E_NOTIMPL;
}

HRESULT DMUSIC_CreateMidiInPortImpl(LPCGUID guid, LPVOID *object, LPUNKNOWN unkouter, LPDMUS_PORTPARAMS port_params, LPDMUS_PORTCAPS port_caps, DWORD device)
{
    TRACE("(%p,%p,%p,%p,%p,%d): stub\n", guid, object, unkouter, port_params, port_caps, device);

    return E_NOTIMPL;
}
