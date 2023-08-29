/*
 * IDirectMusicSynth8 Implementation
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

#define COBJMACROS
#include "objbase.h"
#include "initguid.h"
#include "dmksctrl.h"

#include "dmsynth_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmsynth);

struct synth
{
    IDirectMusicSynth8 IDirectMusicSynth8_iface;
    IKsControl IKsControl_iface;
    LONG ref;

    DMUS_PORTCAPS caps;
    DMUS_PORTPARAMS params;
    BOOL active;
    BOOL open;
    IDirectMusicSynthSink *sink;
};

static inline struct synth *impl_from_IDirectMusicSynth8(IDirectMusicSynth8 *iface)
{
    return CONTAINING_RECORD(iface, struct synth, IDirectMusicSynth8_iface);
}

static HRESULT WINAPI synth_QueryInterface(IDirectMusicSynth8 *iface, REFIID riid,
        void **ret_iface)
{
    struct synth *This = impl_from_IDirectMusicSynth8(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_dmguid(riid), ret_iface);

    if (IsEqualIID (riid, &IID_IUnknown) ||
        IsEqualIID (riid, &IID_IDirectMusicSynth) ||
        IsEqualIID (riid, &IID_IDirectMusicSynth8))
    {
        IUnknown_AddRef(iface);
        *ret_iface = iface;
        return S_OK;
    }
    else if (IsEqualIID(riid, &IID_IKsControl))
    {
        IUnknown_AddRef(iface);
        *ret_iface = &This->IKsControl_iface;
        return S_OK;
    }

    *ret_iface = NULL;

    WARN("(%p)->(%s, %p): not found\n", iface, debugstr_dmguid(riid), ret_iface);

    return E_NOINTERFACE;
}

static ULONG WINAPI synth_AddRef(IDirectMusicSynth8 *iface)
{
    struct synth *This = impl_from_IDirectMusicSynth8(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): new ref = %lu\n", This, ref);

    return ref;
}

static ULONG WINAPI synth_Release(IDirectMusicSynth8 *iface)
{
    struct synth *This = impl_from_IDirectMusicSynth8(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): new ref = %lu\n", This, ref);

    if (!ref) free(This);

    return ref;
}

static HRESULT WINAPI synth_Open(IDirectMusicSynth8 *iface, DMUS_PORTPARAMS *params)
{
    struct synth *This = impl_from_IDirectMusicSynth8(iface);
    BOOL modified = FALSE;
    const DMUS_PORTPARAMS def = {
        .dwValidParams = DMUS_PORTPARAMS_VOICES|DMUS_PORTPARAMS_CHANNELGROUPS|
                DMUS_PORTPARAMS_AUDIOCHANNELS|DMUS_PORTPARAMS_SAMPLERATE|DMUS_PORTPARAMS_EFFECTS|
                DMUS_PORTPARAMS_SHARE|DMUS_PORTPARAMS_FEATURES,
        .dwSize = sizeof(def), .dwVoices = 32, .dwChannelGroups = 2, .dwAudioChannels = 2,
        .dwSampleRate = 22050, .dwEffectFlags = DMUS_EFFECT_REVERB
    };

    TRACE("(%p, %p)\n", This, params);

    if (This->open)
        return DMUS_E_ALREADYOPEN;
    if (params && params->dwSize < sizeof(DMUS_PORTPARAMS7))
        return E_INVALIDARG;

    This->open = TRUE;

    if (!params) {
        memcpy(&This->params, &def, sizeof(This->params));
        return S_OK;
    }

    if (params->dwValidParams & DMUS_PORTPARAMS_VOICES && params->dwVoices) {
        if (params->dwVoices > This->caps.dwMaxVoices) {
            modified = TRUE;
            params->dwVoices = This->caps.dwMaxVoices;
        }
    } else
        params->dwVoices = def.dwVoices;

    if (params->dwValidParams & DMUS_PORTPARAMS_CHANNELGROUPS && params->dwChannelGroups) {
        if (params->dwChannelGroups > This->caps.dwMaxChannelGroups) {
            modified = TRUE;
            params->dwChannelGroups = This->caps.dwMaxChannelGroups;
        }
    } else
        params->dwChannelGroups = def.dwChannelGroups;

    if (params->dwValidParams & DMUS_PORTPARAMS_AUDIOCHANNELS && params->dwAudioChannels) {
        if (params->dwAudioChannels > This->caps.dwMaxAudioChannels) {
            modified = TRUE;
            params->dwAudioChannels = This->caps.dwMaxAudioChannels;
        }
    } else
        params->dwAudioChannels = def.dwAudioChannels;

    if (params->dwValidParams & DMUS_PORTPARAMS_SAMPLERATE && params->dwSampleRate) {
        if (params->dwSampleRate > 96000) {
            modified = TRUE;
            params->dwSampleRate = 96000;
        } else if (params->dwSampleRate < 11025) {
            modified = TRUE;
            params->dwSampleRate = 11025;
        }
    } else
        params->dwSampleRate = def.dwSampleRate;

    if (params->dwValidParams & DMUS_PORTPARAMS_EFFECTS && params->dwEffectFlags != def.dwEffectFlags)
        modified = TRUE;
    params->dwEffectFlags = def.dwEffectFlags;

    if (params->dwValidParams & DMUS_PORTPARAMS_SHARE && params->fShare)
        modified = TRUE;
    params->fShare = FALSE;

    if (params->dwSize >= sizeof(*params)) {
        if (params->dwValidParams & DMUS_PORTPARAMS_FEATURES && params->dwFeatures) {
            if (params->dwFeatures & ~(DMUS_PORT_FEATURE_AUDIOPATH|DMUS_PORT_FEATURE_STREAMING)) {
                modified = TRUE;
                params->dwFeatures &= DMUS_PORT_FEATURE_AUDIOPATH|DMUS_PORT_FEATURE_STREAMING;
            }
        } else
            params->dwFeatures = def.dwFeatures;
        params->dwValidParams = def.dwValidParams;
    } else
        params->dwValidParams = def.dwValidParams & ~DMUS_PORTPARAMS_FEATURES;

    memcpy(&This->params, params, min(params->dwSize, sizeof(This->params)));

    return modified ? S_FALSE : S_OK;
}

static HRESULT WINAPI synth_Close(IDirectMusicSynth8 *iface)
{
    struct synth *This = impl_from_IDirectMusicSynth8(iface);

    TRACE("(%p)\n", This);

    if (!This->open)
        return DMUS_E_ALREADYCLOSED;

    This->open = FALSE;

    return S_OK;
}

static HRESULT WINAPI synth_SetNumChannelGroups(IDirectMusicSynth8 *iface,
        DWORD groups)
{
    struct synth *This = impl_from_IDirectMusicSynth8(iface);

    FIXME("(%p, %ld): stub\n", This, groups);

    return S_OK;
}

static HRESULT WINAPI synth_Download(IDirectMusicSynth8 *iface, HANDLE *hDownload,
        void *data, BOOL *free)
{
    struct synth *This = impl_from_IDirectMusicSynth8(iface);
    LPBYTE buffer = data;
    DMUS_DOWNLOADINFO *info = (DMUS_DOWNLOADINFO*)buffer;
    ULONG *offsets = ((DMUS_OFFSETTABLE*)(buffer + sizeof(DMUS_DOWNLOADINFO)))->ulOffsetTable;
    LPBYTE object = buffer + sizeof(DMUS_DOWNLOADINFO) + info->dwNumOffsetTableEntries * sizeof(ULONG);

    FIXME("(%p)->(%p, %p, %p): stub\n", This, hDownload, data, free);

    /* FIXME: Currently we only dump data which is very useful to known how native dmusic behave and debug builtin dmusic */

    if (!hDownload || !free)
        return E_POINTER;

    if (TRACE_ON(dmsynth))
    {
        TRACE("Dump DMUS_DOWNLOADINFO struct:\n");
        TRACE(" - dwDLType                = %lu\n", info->dwDLType);
        TRACE(" - dwDLId                  = %lu\n", info->dwDLId);
        TRACE(" - dwNumOffsetTableEntries = %lu\n", info->dwNumOffsetTableEntries);
        TRACE(" - cbSize                  = %lu\n", info->cbSize);
    }

    /* The struct should have at least one offset corresponding to the download object itself */
    if (!info->dwNumOffsetTableEntries)
    {
        FIXME("Offset table is empty\n");
        return DMUS_E_BADOFFSETTABLE;
    }

    /* First offset should point to the download object */
    if ((buffer + offsets[0]) != object)
    {
        FIXME("Object is not at the beginning\n");
        return DMUS_E_BADOFFSETTABLE;
    }

    if (info->dwDLType == DMUS_DOWNLOADINFO_INSTRUMENT)
    {
        FIXME("Download type DMUS_DOWNLOADINFO_INSTRUMENT not yet supported\n");
    }
    else if (info->dwDLType == DMUS_DOWNLOADINFO_WAVE)
    {
        DMUS_WAVE *wave = (DMUS_WAVE*)object;
        DMUS_WAVEDATA *wave_data;

        TRACE("Processing download type DMUS_DOWNLOADINFO_WAVE\n");

        if (TRACE_ON(dmsynth))
        {
            TRACE("Dump DMUS_WAVE struct\n");
            TRACE(" - ulFirstExtCkIdx   = %lu\n", wave->ulFirstExtCkIdx);
            TRACE(" - ulCopyrightIdx    = %lu\n", wave->ulCopyrightIdx);
            TRACE(" - ulWaveDataIdx     = %lu\n", wave->ulWaveDataIdx);
            TRACE(" - WaveformatEx:\n");
            TRACE("   - wFormatTag      = %u\n", wave->WaveformatEx.wFormatTag);
            TRACE("   - nChannels       = %u\n", wave->WaveformatEx.nChannels);
            TRACE("   - nSamplesPerSec  = %lu\n", wave->WaveformatEx.nSamplesPerSec);
            TRACE("   - nAvgBytesPerSec = %lu\n", wave->WaveformatEx.nAvgBytesPerSec);
            TRACE("   - nBlockAlign     = %u\n", wave->WaveformatEx.nBlockAlign);
            TRACE("   - wBitsPerSample  = %u\n", wave->WaveformatEx.wBitsPerSample);
            TRACE("   - cbSize          = %u\n", wave->WaveformatEx.cbSize);

            if (wave->ulCopyrightIdx)
            {
                DMUS_COPYRIGHT *copyright = (DMUS_COPYRIGHT*)(buffer + offsets[wave->ulCopyrightIdx]);
                TRACE("Copyright = '%s'\n",  (char*)copyright->byCopyright);
            }

            wave_data = (DMUS_WAVEDATA*)(buffer + offsets[wave->ulWaveDataIdx]);
            TRACE("Found %lu bytes of wave data\n", wave_data->cbSize);
        }
    }
    else if (info->dwDLType == DMUS_DOWNLOADINFO_INSTRUMENT2)
    {
        DMUS_INSTRUMENT *instrument = (DMUS_INSTRUMENT*)object;
        ULONG nb_regions = 0;

        TRACE("Processing download type DMUS_DOWNLOADINFO_INSTRUMENT2\n");

        if (TRACE_ON(dmsynth))
        {
            TRACE("Dump DMUS_INSTRUMENT struct\n");
            TRACE(" - ulPatch          = %lu\n", instrument->ulPatch);
            TRACE(" - ulFirstRegionIdx = %lu\n", instrument->ulFirstRegionIdx);
            TRACE(" - ulGlobalArtIdx   = %lu\n", instrument->ulGlobalArtIdx);
            TRACE(" - ulFirstExtCkIdx  = %lu\n", instrument->ulFirstExtCkIdx);
            TRACE(" - ulCopyrightIdx   = %lu\n", instrument->ulCopyrightIdx);
            TRACE(" - ulFlags          = %lu\n", instrument->ulFlags);

            if (instrument->ulCopyrightIdx)
            {
                DMUS_COPYRIGHT *copyright = (DMUS_COPYRIGHT*)(buffer + offsets[instrument->ulCopyrightIdx]);
                TRACE("Copyright = '%s'\n",  (char*)copyright->byCopyright);
            }
        }

        if (instrument->ulFirstRegionIdx)
        {
            ULONG region_idx = instrument->ulFirstRegionIdx;

            while (region_idx)
            {
                DMUS_REGION *region = (DMUS_REGION*)(buffer + offsets[region_idx]);

                region_idx = region->ulNextRegionIdx;
                nb_regions++;
            }
        }

        TRACE("Number of regions = %lu\n", nb_regions);
    }
    else if (info->dwDLType == DMUS_DOWNLOADINFO_WAVEARTICULATION)
    {
        FIXME("Download type DMUS_DOWNLOADINFO_WAVEARTICULATION not yet supported\n");
    }
    else if (info->dwDLType == DMUS_DOWNLOADINFO_STREAMINGWAVE)
    {
        FIXME("Download type DMUS_DOWNLOADINFO_STREAMINGWAVE not yet supported\n");
    }
    else if (info->dwDLType == DMUS_DOWNLOADINFO_ONESHOTWAVE)
    {
        FIXME("Download type DMUS_DOWNLOADINFO_ONESHOTWAVE not yet supported\n");
    }
    else
    {
        WARN("Unknown download type %lu\n", info->dwDLType);
        return DMUS_E_UNKNOWNDOWNLOAD;
    }

    return S_OK;
}

static HRESULT WINAPI synth_Unload(IDirectMusicSynth8 *iface, HANDLE hDownload,
        HRESULT (CALLBACK *lpFreeHandle)(HANDLE,HANDLE), HANDLE hUserData)
{
    struct synth *This = impl_from_IDirectMusicSynth8(iface);

    FIXME("(%p)->(%p, %p, %p): stub\n", This, hDownload, lpFreeHandle, hUserData);

    return S_OK;
}

static HRESULT WINAPI synth_PlayBuffer(IDirectMusicSynth8 *iface,
        REFERENCE_TIME rt, BYTE *buffer, DWORD size)
{
    struct synth *This = impl_from_IDirectMusicSynth8(iface);

    FIXME("(%p, 0x%s, %p, %lu): stub\n", This, wine_dbgstr_longlong(rt), buffer, size);

    return S_OK;
}

static HRESULT WINAPI synth_GetRunningStats(IDirectMusicSynth8 *iface,
        DMUS_SYNTHSTATS *stats)
{
    struct synth *This = impl_from_IDirectMusicSynth8(iface);

    FIXME("(%p)->(%p): stub\n", This, stats);

    return S_OK;
}

static HRESULT WINAPI synth_GetPortCaps(IDirectMusicSynth8 *iface,
        DMUS_PORTCAPS *caps)
{
    struct synth *This = impl_from_IDirectMusicSynth8(iface);

    TRACE("(%p)->(%p)\n", This, caps);

    if (!caps || caps->dwSize < sizeof(*caps))
        return E_INVALIDARG;

    *caps = This->caps;

    return S_OK;
}

static HRESULT WINAPI synth_SetMasterClock(IDirectMusicSynth8 *iface,
        IReferenceClock *clock)
{
    struct synth *This = impl_from_IDirectMusicSynth8(iface);

    TRACE("(%p)->(%p)\n", This, clock);

    if (!This->sink)
        return DMUS_E_NOSYNTHSINK;

    return IDirectMusicSynthSink_SetMasterClock(This->sink, clock);
}

static HRESULT WINAPI synth_GetLatencyClock(IDirectMusicSynth8 *iface,
        IReferenceClock **clock)
{
    struct synth *This = impl_from_IDirectMusicSynth8(iface);

    TRACE("(%p)->(%p)\n", iface, clock);

    if (!clock)
        return E_POINTER;
    if (!This->sink)
        return DMUS_E_NOSYNTHSINK;

    return IDirectMusicSynthSink_GetLatencyClock(This->sink, clock);
}

static HRESULT WINAPI synth_Activate(IDirectMusicSynth8 *iface, BOOL enable)
{
    struct synth *This = impl_from_IDirectMusicSynth8(iface);
    HRESULT hr;

    TRACE("(%p)->(%d)\n", This, enable);

    if (enable == This->active) return S_FALSE;

    if (!This->sink)
        return DMUS_E_NOSYNTHSINK;

    if ((hr = IDirectMusicSynthSink_Activate(This->sink, enable)) != S_OK) {
        if (hr == DMUS_E_SYNTHACTIVE || hr == S_FALSE)
            WARN("Synth and sink active state out of sync. Fixing.\n");
        else
            return hr;
    }

    This->active = enable;

    return S_OK;
}

static HRESULT WINAPI synth_SetSynthSink(IDirectMusicSynth8 *iface,
        IDirectMusicSynthSink *sink)
{
    struct synth *This = impl_from_IDirectMusicSynth8(iface);

    TRACE("(%p)->(%p)\n", iface, sink);

    if (sink == This->sink)
        return S_OK;

    if (!sink || This->sink) IDirectMusicSynthSink_Release(This->sink);

    This->sink = sink;
    if (!sink)
        return S_OK;

    IDirectMusicSynthSink_AddRef(This->sink);
    return IDirectMusicSynthSink_Init(sink, (IDirectMusicSynth *)iface);
}

static HRESULT WINAPI synth_Render(IDirectMusicSynth8 *iface, short *buffer,
        DWORD length, LONGLONG position)
{
    struct synth *This = impl_from_IDirectMusicSynth8(iface);

    FIXME("(%p, %p, %ld, 0x%s): stub\n", This, buffer, length, wine_dbgstr_longlong(position));

    return S_OK;
}

static HRESULT WINAPI synth_SetChannelPriority(IDirectMusicSynth8 *iface,
        DWORD channel_group, DWORD channel, DWORD priority)
{
    /* struct synth *This = impl_from_IDirectMusicSynth8(iface); */

    /* Silenced because of too many messages - 1000 groups * 16 channels ;=) */
    /* FIXME("(%p)->(%ld, %ld, %ld): stub\n", This, channel_group, channel, priority); */

    return S_OK;
}

static HRESULT WINAPI synth_GetChannelPriority(IDirectMusicSynth8 *iface,
        DWORD channel_group, DWORD channel, DWORD *priority)
{
    struct synth *This = impl_from_IDirectMusicSynth8(iface);

    FIXME("(%p, %ld, %ld, %p): stub\n", This, channel_group, channel, priority);

    return S_OK;
}

static HRESULT WINAPI synth_GetFormat(IDirectMusicSynth8 *iface,
        WAVEFORMATEX *format, DWORD *size)
{
    struct synth *This = impl_from_IDirectMusicSynth8(iface);
    WAVEFORMATEX fmt;

    TRACE("(%p, %p, %p)\n", This, format, size);

    if (!size)
        return E_POINTER;
    if (!This->open)
        return DMUS_E_SYNTHNOTCONFIGURED;

    if (format) {
        fmt.wFormatTag = WAVE_FORMAT_PCM;
        fmt.nChannels = This->params.dwAudioChannels;
        fmt.nSamplesPerSec = This->params.dwSampleRate;
        fmt.wBitsPerSample = 16;
        fmt.nBlockAlign = This->params.dwAudioChannels * fmt.wBitsPerSample / 8;
        fmt.nAvgBytesPerSec = This->params.dwSampleRate * fmt.nBlockAlign;
        fmt.cbSize = 0;
        memcpy(format, &fmt, min(*size, sizeof(fmt)));
    }
    *size = sizeof(fmt);

    return S_OK;
}

static HRESULT WINAPI synth_GetAppend(IDirectMusicSynth8 *iface, DWORD *append)
{
    TRACE("(%p)->(%p)\n", iface, append);

    /* We don't need extra space at the end of buffers passed to us for now */
    *append = 0;

    return S_OK;
}

static HRESULT WINAPI synth_PlayVoice(IDirectMusicSynth8 *iface,
        REFERENCE_TIME ref_time, DWORD voice_id, DWORD channel_group, DWORD channel, DWORD dwDLId,
        LONG prPitch, LONG vrVolume, SAMPLE_TIME stVoiceStart, SAMPLE_TIME stLoopStart,
        SAMPLE_TIME stLoopEnd)
{
    struct synth *This = impl_from_IDirectMusicSynth8(iface);

    FIXME("(%p, 0x%s, %ld, %ld, %ld, %ld, %li, %li, 0x%s, 0x%s, 0x%s): stub\n",
          This, wine_dbgstr_longlong(ref_time), voice_id, channel_group, channel, dwDLId, prPitch, vrVolume,
          wine_dbgstr_longlong(stVoiceStart), wine_dbgstr_longlong(stLoopStart), wine_dbgstr_longlong(stLoopEnd));

    return S_OK;
}

static HRESULT WINAPI synth_StopVoice(IDirectMusicSynth8 *iface,
        REFERENCE_TIME ref_time, DWORD voice_id)
{
    struct synth *This = impl_from_IDirectMusicSynth8(iface);

    FIXME("(%p, 0x%s, %ld): stub\n", This, wine_dbgstr_longlong(ref_time), voice_id);

    return S_OK;
}

static HRESULT WINAPI synth_GetVoiceState(IDirectMusicSynth8 *iface,
        DWORD dwVoice[], DWORD cbVoice, DMUS_VOICE_STATE dwVoiceState[])
{
    struct synth *This = impl_from_IDirectMusicSynth8(iface);

    FIXME("(%p, %p, %ld, %p): stub\n", This, dwVoice, cbVoice, dwVoiceState);

    return S_OK;
}

static HRESULT WINAPI synth_Refresh(IDirectMusicSynth8 *iface, DWORD download_id,
        DWORD flags)
{
    struct synth *This = impl_from_IDirectMusicSynth8(iface);

    FIXME("(%p, %ld, %ld): stub\n", This, download_id, flags);

    return S_OK;
}

static HRESULT WINAPI synth_AssignChannelToBuses(IDirectMusicSynth8 *iface,
        DWORD channel_group, DWORD channel, DWORD *pdwBuses, DWORD cBuses)
{
    struct synth *This = impl_from_IDirectMusicSynth8(iface);

    FIXME("(%p, %ld, %ld, %p, %ld): stub\n", This, channel_group, channel, pdwBuses, cBuses);

    return S_OK;
}

static const IDirectMusicSynth8Vtbl synth_vtbl =
{
	synth_QueryInterface,
	synth_AddRef,
	synth_Release,
	synth_Open,
	synth_Close,
	synth_SetNumChannelGroups,
	synth_Download,
	synth_Unload,
	synth_PlayBuffer,
	synth_GetRunningStats,
	synth_GetPortCaps,
	synth_SetMasterClock,
	synth_GetLatencyClock,
	synth_Activate,
	synth_SetSynthSink,
	synth_Render,
	synth_SetChannelPriority,
	synth_GetChannelPriority,
	synth_GetFormat,
	synth_GetAppend,
	synth_PlayVoice,
	synth_StopVoice,
	synth_GetVoiceState,
	synth_Refresh,
	synth_AssignChannelToBuses,
};

static inline struct synth *impl_from_IKsControl(IKsControl *iface)
{
    return CONTAINING_RECORD(iface, struct synth, IKsControl_iface);
}

static HRESULT WINAPI synth_control_QueryInterface(IKsControl* iface, REFIID riid, LPVOID *ppobj)
{
    struct synth *This = impl_from_IKsControl(iface);

    return synth_QueryInterface(&This->IDirectMusicSynth8_iface, riid, ppobj);
}

static ULONG WINAPI synth_control_AddRef(IKsControl* iface)
{
    struct synth *This = impl_from_IKsControl(iface);

    return synth_AddRef(&This->IDirectMusicSynth8_iface);
}

static ULONG WINAPI synth_control_Release(IKsControl* iface)
{
    struct synth *This = impl_from_IKsControl(iface);

    return synth_Release(&This->IDirectMusicSynth8_iface);
}

static HRESULT WINAPI synth_control_KsProperty(IKsControl* iface, PKSPROPERTY Property,
        ULONG PropertyLength, LPVOID PropertyData, ULONG DataLength, ULONG* BytesReturned)
{
    TRACE("(%p, %p, %lu, %p, %lu, %p)\n", iface, Property, PropertyLength, PropertyData, DataLength, BytesReturned);

    TRACE("Property = %s - %lu - %lu\n", debugstr_guid(&Property->Set), Property->Id, Property->Flags);

    if (Property->Flags != KSPROPERTY_TYPE_GET)
    {
        FIXME("Property flags %lu not yet supported\n", Property->Flags);
        return S_FALSE;
    }

    if (DataLength <  sizeof(DWORD))
        return E_NOT_SUFFICIENT_BUFFER;

    if (IsEqualGUID(&Property->Set, &GUID_DMUS_PROP_INSTRUMENT2))
    {
        *(DWORD*)PropertyData = TRUE;
        *BytesReturned = sizeof(DWORD);
    }
    else if (IsEqualGUID(&Property->Set, &GUID_DMUS_PROP_DLS2))
    {
        *(DWORD*)PropertyData = TRUE;
        *BytesReturned = sizeof(DWORD);
    }
    else if (IsEqualGUID(&Property->Set, &GUID_DMUS_PROP_GM_Hardware))
    {
        *(DWORD*)PropertyData = FALSE;
        *BytesReturned = sizeof(DWORD);
    }
    else if (IsEqualGUID(&Property->Set, &GUID_DMUS_PROP_GS_Hardware))
    {
        *(DWORD*)PropertyData = FALSE;
        *BytesReturned = sizeof(DWORD);
    }
    else if (IsEqualGUID(&Property->Set, &GUID_DMUS_PROP_XG_Hardware))
    {
        *(DWORD*)PropertyData = FALSE;
        *BytesReturned = sizeof(DWORD);
    }
    else
    {
        FIXME("Unknown property %s\n", debugstr_guid(&Property->Set));
        *(DWORD*)PropertyData = FALSE;
        *BytesReturned = sizeof(DWORD);
    }

    return S_OK;
}

static HRESULT WINAPI synth_control_KsMethod(IKsControl* iface, PKSMETHOD Method,
        ULONG MethodLength, LPVOID MethodData, ULONG DataLength, ULONG* BytesReturned)
{
    FIXME("(%p, %p, %lu, %p, %lu, %p): stub\n", iface, Method, MethodLength, MethodData, DataLength, BytesReturned);

    return E_NOTIMPL;
}

static HRESULT WINAPI synth_control_KsEvent(IKsControl* iface, PKSEVENT Event,
        ULONG EventLength, LPVOID EventData, ULONG DataLength, ULONG* BytesReturned)
{
    FIXME("(%p, %p, %lu, %p, %lu, %p): stub\n", iface, Event, EventLength, EventData, DataLength, BytesReturned);

    return E_NOTIMPL;
}


static const IKsControlVtbl synth_control_vtbl =
{
    synth_control_QueryInterface,
    synth_control_AddRef,
    synth_control_Release,
    synth_control_KsProperty,
    synth_control_KsMethod,
    synth_control_KsEvent,
};

HRESULT synth_create(IUnknown **ret_iface)
{
    struct synth *obj;

    TRACE("(%p)\n", ret_iface);

    *ret_iface = NULL;
    if (!(obj = calloc(1, sizeof(*obj)))) return E_OUTOFMEMORY;
    obj->IDirectMusicSynth8_iface.lpVtbl = &synth_vtbl;
    obj->IKsControl_iface.lpVtbl = &synth_control_vtbl;
    obj->ref = 1;

    obj->caps.dwSize = sizeof(DMUS_PORTCAPS);
    obj->caps.dwFlags = DMUS_PC_DLS | DMUS_PC_SOFTWARESYNTH | DMUS_PC_DIRECTSOUND | DMUS_PC_DLS2 | DMUS_PC_AUDIOPATH | DMUS_PC_WAVE;
    obj->caps.guidPort = CLSID_DirectMusicSynth;
    obj->caps.dwClass = DMUS_PC_OUTPUTCLASS;
    obj->caps.dwType = DMUS_PORT_USER_MODE_SYNTH;
    obj->caps.dwMemorySize = DMUS_PC_SYSTEMMEMORY;
    obj->caps.dwMaxChannelGroups = 1000;
    obj->caps.dwMaxVoices = 1000;
    obj->caps.dwMaxAudioChannels = 2;
    obj->caps.dwEffectFlags = DMUS_EFFECT_REVERB;
    lstrcpyW(obj->caps.wszDescription, L"Microsoft Synthesizer");

    TRACE("Created DirectMusicSynth %p\n", obj);
    *ret_iface = (IUnknown *)&obj->IDirectMusicSynth8_iface;
    return S_OK;
}
