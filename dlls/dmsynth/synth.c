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
#include "dls2.h"

#include <fluidsynth.h>

WINE_DEFAULT_DEBUG_CHANNEL(dmsynth);

#define CONN_SRC_CC2  0x0082
#define CONN_SRC_RPN0 0x0100

static const char *debugstr_conn_src(UINT src)
{
    switch (src)
    {
    case CONN_SRC_NONE: return "SRC_NONE";
    case CONN_SRC_LFO: return "SRC_LFO";
    case CONN_SRC_KEYONVELOCITY: return "SRC_KEYONVELOCITY";
    case CONN_SRC_KEYNUMBER: return "SRC_KEYNUMBER";
    case CONN_SRC_EG1: return "SRC_EG1";
    case CONN_SRC_EG2: return "SRC_EG2";
    case CONN_SRC_PITCHWHEEL: return "SRC_PITCHWHEEL";
    case CONN_SRC_CC1: return "SRC_CC1";
    case CONN_SRC_CC7: return "SRC_CC7";
    case CONN_SRC_CC10: return "SRC_CC10";
    case CONN_SRC_CC11: return "SRC_CC11";
    case CONN_SRC_POLYPRESSURE: return "SRC_POLYPRESSURE";
    case CONN_SRC_CHANNELPRESSURE: return "SRC_CHANNELPRESSURE";
    case CONN_SRC_VIBRATO: return "SRC_VIBRATO";
    case CONN_SRC_MONOPRESSURE: return "SRC_MONOPRESSURE";
    case CONN_SRC_CC91: return "SRC_CC91";
    case CONN_SRC_CC93: return "SRC_CC93";

    case CONN_SRC_CC2: return "SRC_CC2";
    case CONN_SRC_RPN0: return "SRC_RPN0";
    }

    return wine_dbg_sprintf("%#x", src);
}

static const char *debugstr_conn_dst(UINT dst)
{
    switch (dst)
    {
    case CONN_DST_NONE: return "DST_NONE";
    /* case CONN_DST_ATTENUATION: return "DST_ATTENUATION"; Same as CONN_DST_GAIN */
    case CONN_DST_PITCH: return "DST_PITCH";
    case CONN_DST_PAN: return "DST_PAN";
    case CONN_DST_LFO_FREQUENCY: return "DST_LFO_FREQUENCY";
    case CONN_DST_LFO_STARTDELAY: return "DST_LFO_STARTDELAY";
    case CONN_DST_EG1_ATTACKTIME: return "DST_EG1_ATTACKTIME";
    case CONN_DST_EG1_DECAYTIME: return "DST_EG1_DECAYTIME";
    case CONN_DST_EG1_RELEASETIME: return "DST_EG1_RELEASETIME";
    case CONN_DST_EG1_SUSTAINLEVEL: return "DST_EG1_SUSTAINLEVEL";
    case CONN_DST_EG2_ATTACKTIME: return "DST_EG2_ATTACKTIME";
    case CONN_DST_EG2_DECAYTIME: return "DST_EG2_DECAYTIME";
    case CONN_DST_EG2_RELEASETIME: return "DST_EG2_RELEASETIME";
    case CONN_DST_EG2_SUSTAINLEVEL: return "DST_EG2_SUSTAINLEVEL";
    case CONN_DST_GAIN: return "DST_GAIN";
    case CONN_DST_KEYNUMBER: return "DST_KEYNUMBER";
    case CONN_DST_LEFT: return "DST_LEFT";
    case CONN_DST_RIGHT: return "DST_RIGHT";
    case CONN_DST_CENTER: return "DST_CENTER";
    case CONN_DST_LEFTREAR: return "DST_LEFTREAR";
    case CONN_DST_RIGHTREAR: return "DST_RIGHTREAR";
    case CONN_DST_LFE_CHANNEL: return "DST_LFE_CHANNEL";
    case CONN_DST_CHORUS: return "DST_CHORUS";
    case CONN_DST_REVERB: return "DST_REVERB";
    case CONN_DST_VIB_FREQUENCY: return "DST_VIB_FREQUENCY";
    case CONN_DST_VIB_STARTDELAY: return "DST_VIB_STARTDELAY";
    case CONN_DST_EG1_DELAYTIME: return "DST_EG1_DELAYTIME";
    case CONN_DST_EG1_HOLDTIME: return "DST_EG1_HOLDTIME";
    case CONN_DST_EG1_SHUTDOWNTIME: return "DST_EG1_SHUTDOWNTIME";
    case CONN_DST_EG2_DELAYTIME: return "DST_EG2_DELAYTIME";
    case CONN_DST_EG2_HOLDTIME: return "DST_EG2_HOLDTIME";
    case CONN_DST_FILTER_CUTOFF: return "DST_FILTER_CUTOFF";
    case CONN_DST_FILTER_Q: return "DST_FILTER_Q";
    }

    return wine_dbg_sprintf("%#x", dst);
}

static const char *debugstr_connection(const CONNECTION *conn)
{
    return wine_dbg_sprintf("%s (%#x) x %s (%#x) -> %s (%#x): %ld", debugstr_conn_src(conn->usSource),
            (conn->usTransform >> 10) & 0x3f, debugstr_conn_src(conn->usControl), (conn->usTransform >> 4) & 0x3f,
            debugstr_conn_dst(conn->usDestination), (conn->usTransform & 0xf), conn->lScale);
}

static void dump_dmus_instrument(DMUS_INSTRUMENT *instrument)
{
    TRACE("DMUS_INSTRUMENT:\n");
    TRACE(" - ulPatch          = %lu\n", instrument->ulPatch);
    TRACE(" - ulFirstRegionIdx = %lu\n", instrument->ulFirstRegionIdx);
    TRACE(" - ulGlobalArtIdx   = %lu\n", instrument->ulGlobalArtIdx);
    TRACE(" - ulFirstExtCkIdx  = %lu\n", instrument->ulFirstExtCkIdx);
    TRACE(" - ulCopyrightIdx   = %lu\n", instrument->ulCopyrightIdx);
    TRACE(" - ulFlags          = %lu\n", instrument->ulFlags);
}

static void dump_dmus_region(DMUS_REGION *region)
{
    UINT i;

    TRACE("DMUS_REGION:\n");
    TRACE(" - RangeKey        = %u - %u\n", region->RangeKey.usLow, region->RangeKey.usHigh);
    TRACE(" - RangeVelocity   = %u - %u\n", region->RangeVelocity.usLow, region->RangeVelocity.usHigh);
    TRACE(" - fusOptions      = %#x\n", region->fusOptions);
    TRACE(" - usKeyGroup      = %u\n", region->usKeyGroup);
    TRACE(" - ulRegionArtIdx  = %lu\n", region->ulRegionArtIdx);
    TRACE(" - ulNextRegionIdx = %lu\n", region->ulNextRegionIdx);
    TRACE(" - ulFirstExtCkIdx = %lu\n", region->ulFirstExtCkIdx);
    TRACE(" - WaveLink:\n");
    TRACE("   - fusOptions    = %#x\n", region->WaveLink.fusOptions);
    TRACE("   - usPhaseGroup  = %u\n", region->WaveLink.usPhaseGroup);
    TRACE("   - ulChannel     = %lu\n", region->WaveLink.ulChannel);
    TRACE("   - ulTableIndex  = %lu\n", region->WaveLink.ulTableIndex);
    TRACE(" - WSMP:\n");
    TRACE("   - cbSize        = %lu\n", region->WSMP.cbSize);
    TRACE("   - usUnityNote   = %u\n", region->WSMP.usUnityNote);
    TRACE("   - sFineTune     = %u\n", region->WSMP.sFineTune);
    TRACE("   - lAttenuation  = %lu\n", region->WSMP.lAttenuation);
    TRACE("   - fulOptions    = %#lx\n", region->WSMP.fulOptions);
    TRACE("   - cSampleLoops  = %lu\n", region->WSMP.cSampleLoops);
    for (i = 0; i < region->WSMP.cSampleLoops; i++)
    {
        TRACE(" - WLOOP[%u]:\n", i);
        TRACE("   - cbSize        = %lu\n", region->WLOOP[i].cbSize);
        TRACE("   - ulType        = %#lx\n", region->WLOOP[i].ulType);
        TRACE("   - ulStart       = %lu\n", region->WLOOP[i].ulStart);
        TRACE("   - ulLength      = %lu\n", region->WLOOP[i].ulLength);
    }
}

static void dump_connectionlist(CONNECTIONLIST *list)
{
    CONNECTION *connections = (CONNECTION *)(list + 1);
    UINT i;

    TRACE("CONNECTIONLIST:\n");
    TRACE(" - cbSize        = %lu\n", list->cbSize);
    TRACE(" - cConnections  = %lu\n", list->cConnections);

    for (i = 0; i < list->cConnections; i++)
        TRACE("- CONNECTION[%u]: %s\n", i, debugstr_connection(connections + i));
}

static void dump_dmus_wave(DMUS_WAVE *wave)
{
    TRACE("DMUS_WAVE:\n");
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
}

struct wave
{
    struct list entry;
    LONG ref;
    UINT id;

    WAVEFORMATEX format;
    UINT sample_count;
    short samples[];
};

C_ASSERT(sizeof(struct wave) == offsetof(struct wave, samples[0]));

static void wave_addref(struct wave *wave)
{
    InterlockedIncrement(&wave->ref);
}

static void wave_release(struct wave *wave)
{
    ULONG ref = InterlockedDecrement(&wave->ref);
    if (!ref) free(wave);
}

struct articulation
{
    struct list entry;
    CONNECTIONLIST list;
    CONNECTION connections[];
};

C_ASSERT(sizeof(struct articulation) == offsetof(struct articulation, connections[0]));

struct region
{
    struct list entry;

    RGNRANGE key_range;
    RGNRANGE vel_range;
    UINT flags;
    UINT group;

    struct list articulations;

    struct wave *wave;
    WAVELINK wave_link;
    WSMPL    wave_sample;
    WLOOP    wave_loops[];
};

static void region_destroy(struct region *region)
{
    struct articulation *articulation;
    void *next;

    LIST_FOR_EACH_ENTRY_SAFE(articulation, next, &region->articulations, struct articulation, entry)
    {
        list_remove(&articulation->entry);
        free(articulation);
    }

    wave_release(region->wave);
    free(region);
}

struct instrument
{
    struct list entry;
    LONG ref;
    UINT id;

    UINT patch;
    UINT flags;
    struct list regions;
    struct list articulations;

    struct synth *synth;
};

static void instrument_release(struct instrument *instrument)
{
    ULONG ref = InterlockedDecrement(&instrument->ref);

    if (!ref)
    {
        struct articulation *articulation;
        struct region *region;
        void *next;

        LIST_FOR_EACH_ENTRY_SAFE(region, next, &instrument->regions, struct region, entry)
        {
            list_remove(&region->entry);
            region_destroy(region);
        }

        LIST_FOR_EACH_ENTRY_SAFE(articulation, next, &instrument->articulations, struct articulation, entry)
        {
            list_remove(&articulation->entry);
            free(articulation);
        }

        free(instrument);
    }
}

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

    struct list instruments;
    struct list waves;

    fluid_settings_t *fluid_settings;
    fluid_sfont_t *fluid_sfont;
    fluid_synth_t *fluid_synth;
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

    if (!ref)
    {
        struct instrument *instrument;
        struct wave *wave;
        void *next;

        LIST_FOR_EACH_ENTRY_SAFE(instrument, next, &This->instruments, struct instrument, entry)
        {
            list_remove(&instrument->entry);
            instrument_release(instrument);
        }

        LIST_FOR_EACH_ENTRY_SAFE(wave, next, &This->waves, struct wave, entry)
        {
            list_remove(&wave->entry);
            wave_release(wave);
        }

        fluid_sfont_set_data(This->fluid_sfont, NULL);
        delete_fluid_sfont(This->fluid_sfont);
        This->fluid_sfont = NULL;
        delete_fluid_settings(This->fluid_settings);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI synth_Open(IDirectMusicSynth8 *iface, DMUS_PORTPARAMS *params)
{
    struct synth *This = impl_from_IDirectMusicSynth8(iface);
    DMUS_PORTPARAMS actual =
    {
        .dwSize = sizeof(DMUS_PORTPARAMS),
        .dwValidParams = DMUS_PORTPARAMS_VOICES | DMUS_PORTPARAMS_CHANNELGROUPS
                | DMUS_PORTPARAMS_AUDIOCHANNELS | DMUS_PORTPARAMS_SAMPLERATE
                | DMUS_PORTPARAMS_EFFECTS | DMUS_PORTPARAMS_SHARE | DMUS_PORTPARAMS_FEATURES,
        .dwVoices = 32,
        .dwChannelGroups = 2,
        .dwAudioChannels = 2,
        .dwSampleRate = 22050,
        .dwEffectFlags = DMUS_EFFECT_REVERB,
    };
    UINT size = sizeof(DMUS_PORTPARAMS);
    BOOL modified = FALSE;
    UINT id;

    TRACE("(%p, %p)\n", This, params);

    if (This->open) return DMUS_E_ALREADYOPEN;

    if (params)
    {
        if (params->dwSize < sizeof(DMUS_PORTPARAMS7)) return E_INVALIDARG;
        if (size > params->dwSize) size = params->dwSize;

        if ((params->dwValidParams & DMUS_PORTPARAMS_VOICES) && params->dwVoices)
        {
            actual.dwVoices = min(params->dwVoices, This->caps.dwMaxVoices);
            modified |= actual.dwVoices != params->dwVoices;
        }

        if ((params->dwValidParams & DMUS_PORTPARAMS_CHANNELGROUPS) && params->dwChannelGroups)
        {
            actual.dwChannelGroups = min(params->dwChannelGroups, This->caps.dwMaxChannelGroups);
            modified |= actual.dwChannelGroups != params->dwChannelGroups;
        }

        if ((params->dwValidParams & DMUS_PORTPARAMS_AUDIOCHANNELS) && params->dwAudioChannels)
        {
            /* FluidSynth only works with stereo */
            actual.dwAudioChannels = 2;
            modified |= actual.dwAudioChannels != params->dwAudioChannels;
        }

        if ((params->dwValidParams & DMUS_PORTPARAMS_SAMPLERATE) && params->dwSampleRate)
        {
            actual.dwSampleRate = min(max(params->dwSampleRate, 11025), 96000);
            modified |= actual.dwSampleRate != params->dwSampleRate;
        }

        if (params->dwValidParams & DMUS_PORTPARAMS_EFFECTS)
        {
            actual.dwEffectFlags = DMUS_EFFECT_REVERB;
            modified |= actual.dwEffectFlags != params->dwEffectFlags;
        }

        if (params->dwValidParams & DMUS_PORTPARAMS_SHARE)
        {
            actual.fShare = FALSE;
            modified |= actual.fShare != params->fShare;
        }

        if (params->dwSize < sizeof(*params))
            actual.dwValidParams &= ~DMUS_PORTPARAMS_FEATURES;
        else if ((params->dwValidParams & DMUS_PORTPARAMS_FEATURES) && params->dwFeatures)
        {
            actual.dwFeatures = params->dwFeatures & (DMUS_PORT_FEATURE_AUDIOPATH | DMUS_PORT_FEATURE_STREAMING);
            modified |= actual.dwFeatures != params->dwFeatures;
        }

        memcpy(params, &actual, size);
    }

    fluid_settings_setnum(This->fluid_settings, "synth.sample-rate", actual.dwSampleRate);
    if (!(This->fluid_synth = new_fluid_synth(This->fluid_settings))) return E_OUTOFMEMORY;
    if ((id = fluid_synth_add_sfont(This->fluid_synth, This->fluid_sfont)) == FLUID_FAILED)
        WARN("Failed to add fluid_sfont to fluid_synth\n");

    This->params = actual;
    This->open = TRUE;

    return modified ? S_FALSE : S_OK;
}

static HRESULT WINAPI synth_Close(IDirectMusicSynth8 *iface)
{
    struct synth *This = impl_from_IDirectMusicSynth8(iface);

    TRACE("(%p)\n", This);

    if (!This->open)
        return DMUS_E_ALREADYCLOSED;

    fluid_synth_remove_sfont(This->fluid_synth, This->fluid_sfont);
    delete_fluid_synth(This->fluid_synth);
    This->fluid_synth = NULL;
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

static HRESULT synth_download_articulation2(struct synth *This, ULONG *offsets, BYTE *data,
        UINT index, struct list *articulations)
{
    DMUS_ARTICULATION2 *articulation_info;
    struct articulation *articulation;
    CONNECTION *connections;
    CONNECTIONLIST *list;
    UINT size;

    for (; index; index = articulation_info->ulNextArtIdx)
    {
        articulation_info = (DMUS_ARTICULATION2 *)(data + offsets[index]);
        list = (CONNECTIONLIST *)(data + offsets[articulation_info->ulArtIdx]);
        connections = (CONNECTION *)(list + 1);

        if (TRACE_ON(dmsynth)) dump_connectionlist(list);
        if (articulation_info->ulFirstExtCkIdx) FIXME("Articulation extensions not implemented\n");
        if (list->cbSize != sizeof(*list)) return DMUS_E_BADARTICULATION;

        size = offsetof(struct articulation, connections[list->cConnections]);
        if (!(articulation = calloc(1, size))) return E_OUTOFMEMORY;
        articulation->list = *list;
        memcpy(articulation->connections, connections, list->cConnections * sizeof(*connections));
        list_add_tail(articulations, &articulation->entry);
    }

    return S_OK;
}

static HRESULT synth_download_articulation(struct synth *This, DMUS_DOWNLOADINFO *info, ULONG *offsets, BYTE *data,
        UINT index, struct list *list)
{
    if (info->dwDLType == DMUS_DOWNLOADINFO_INSTRUMENT2)
        return synth_download_articulation2(This, offsets, data, index, list);
    else
    {
        FIXME("DMUS_ARTICPARAMS support not implemented\n");
        return S_OK;
    }
}

static struct wave *synth_find_wave_from_id(struct synth *This, DWORD id)
{
    struct wave *wave;

    LIST_FOR_EACH_ENTRY(wave, &This->waves, struct wave, entry)
    {
        if (wave->id == id)
        {
            wave_addref(wave);
            return wave;
        }
    }

    WARN("Failed to find wave with id %#lx\n", id);
    return NULL;
}

static HRESULT synth_download_instrument(struct synth *This, DMUS_DOWNLOADINFO *info, ULONG *offsets,
        BYTE *data, HANDLE *ret_handle)
{
    DMUS_INSTRUMENT *instrument_info = (DMUS_INSTRUMENT *)(data + offsets[0]);
    struct instrument *instrument;
    DMUS_REGION *region_info;
    struct region *region;
    ULONG index;

    if (TRACE_ON(dmsynth))
    {
        dump_dmus_instrument(instrument_info);

        if (instrument_info->ulCopyrightIdx)
        {
            DMUS_COPYRIGHT *copyright = (DMUS_COPYRIGHT *)(data + offsets[instrument_info->ulCopyrightIdx]);
            TRACE("Copyright = '%s'\n",  (char *)copyright->byCopyright);
        }
    }

    if (instrument_info->ulFirstExtCkIdx) FIXME("Instrument extensions not implemented\n");

    if (!(instrument = calloc(1, sizeof(*instrument)))) return E_OUTOFMEMORY;
    instrument->ref = 1;
    instrument->id = info->dwDLId;
    instrument->patch = instrument_info->ulPatch;
    instrument->flags = instrument_info->ulFlags;
    list_init(&instrument->regions);
    list_init(&instrument->articulations);
    instrument->synth = This;

    for (index = instrument_info->ulFirstRegionIdx; index; index = region_info->ulNextRegionIdx)
    {
        region_info = (DMUS_REGION *)(data + offsets[index]);
        if (TRACE_ON(dmsynth)) dump_dmus_region(region_info);
        if (region_info->ulFirstExtCkIdx) FIXME("Region extensions not implemented\n");

        if (!(region = calloc(1, offsetof(struct region, wave_loops[region_info->WSMP.cSampleLoops])))) goto error;
        region->key_range = region_info->RangeKey;
        region->vel_range = region_info->RangeVelocity;
        region->flags = region_info->fusOptions;
        region->group = region_info->usKeyGroup;
        region->wave_link = region_info->WaveLink;
        region->wave_sample = region_info->WSMP;
        memcpy(region->wave_loops, region_info->WLOOP, region_info->WSMP.cSampleLoops * sizeof(WLOOP));
        list_init(&region->articulations);

        if (!(region->wave = synth_find_wave_from_id(This, region->wave_link.ulTableIndex)))
        {
            free(region);
            instrument_release(instrument);
            return DMUS_E_BADWAVELINK;
        }

        list_add_tail(&instrument->regions, &region->entry);

        if (region_info->ulRegionArtIdx && FAILED(synth_download_articulation(This, info, offsets, data,
                region_info->ulRegionArtIdx, &region->articulations)))
            goto error;
    }

    if (FAILED(synth_download_articulation(This, info, offsets, data,
            instrument_info->ulGlobalArtIdx, &instrument->articulations)))
        goto error;

    list_add_tail(&This->instruments, &instrument->entry);
    *ret_handle = instrument;

    return S_OK;

error:
    instrument_release(instrument);
    return E_OUTOFMEMORY;
}

static HRESULT synth_download_wave(struct synth *This, DMUS_DOWNLOADINFO *info, ULONG *offsets,
        BYTE *data, HANDLE *ret_handle)
{
    DMUS_WAVE *wave_info = (DMUS_WAVE *)(data + offsets[0]);
    DMUS_WAVEDATA *wave_data = (DMUS_WAVEDATA *)(data + offsets[wave_info->ulWaveDataIdx]);
    struct wave *wave;
    UINT sample_count;

    if (TRACE_ON(dmsynth))
    {
        dump_dmus_wave(wave_info);

        if (wave_info->ulCopyrightIdx)
        {
            DMUS_COPYRIGHT *copyright = (DMUS_COPYRIGHT *)(data + offsets[wave_info->ulCopyrightIdx]);
            TRACE("Copyright = '%s'\n",  (char *)copyright->byCopyright);
        }

        TRACE("Found %lu bytes of wave data\n", wave_data->cbSize);
    }

    if (wave_info->ulFirstExtCkIdx) FIXME("Wave extensions not implemented\n");
    if (wave_info->WaveformatEx.wFormatTag != WAVE_FORMAT_PCM) return DMUS_E_NOTPCM;

    sample_count = wave_data->cbSize / wave_info->WaveformatEx.nBlockAlign;
    if (!(wave = calloc(1, offsetof(struct wave, samples[sample_count])))) return E_OUTOFMEMORY;
    wave->ref = 1;
    wave->id = info->dwDLId;
    wave->format = wave_info->WaveformatEx;
    wave->sample_count = sample_count;

    if (wave_info->WaveformatEx.nBlockAlign == 1)
    {
        while (sample_count--)
        {
            short sample = (wave_data->byData[sample_count] - 0x80) << 8;
            wave->samples[sample_count] = sample;
        }
    }
    else if (wave_info->WaveformatEx.nBlockAlign == 2)
    {
        while (sample_count--)
        {
            short sample = ((short *)wave_data->byData)[sample_count];
            wave->samples[sample_count] = sample;
        }
    }
    else if (wave_info->WaveformatEx.nBlockAlign == 4)
    {
        while (sample_count--)
        {
            short sample = ((UINT *)wave_data->byData)[sample_count] >> 16;
            wave->samples[sample_count] = sample;
        }
    }

    list_add_tail(&This->waves, &wave->entry);
    *ret_handle = wave;

    return S_OK;
}

static HRESULT WINAPI synth_Download(IDirectMusicSynth8 *iface, HANDLE *ret_handle, void *data, BOOL *ret_free)
{
    struct synth *This = impl_from_IDirectMusicSynth8(iface);
    DMUS_DOWNLOADINFO *info = data;
    ULONG *offsets = (ULONG *)(info + 1);

    TRACE("(%p)->(%p, %p, %p)\n", This, ret_handle, data, free);

    if (!ret_handle || !data || !ret_free) return E_POINTER;
    *ret_handle = 0;
    *ret_free = TRUE;

    if (TRACE_ON(dmsynth))
    {
        TRACE("Dump DMUS_DOWNLOADINFO struct:\n");
        TRACE(" - dwDLType                = %lu\n", info->dwDLType);
        TRACE(" - dwDLId                  = %lu\n", info->dwDLId);
        TRACE(" - dwNumOffsetTableEntries = %lu\n", info->dwNumOffsetTableEntries);
        TRACE(" - cbSize                  = %lu\n", info->cbSize);
    }

    if (!info->dwNumOffsetTableEntries) return DMUS_E_BADOFFSETTABLE;
    if (((BYTE *)data + offsets[0]) != (BYTE *)(offsets + info->dwNumOffsetTableEntries)) return DMUS_E_BADOFFSETTABLE;

    switch (info->dwDLType)
    {
    case DMUS_DOWNLOADINFO_INSTRUMENT:
    case DMUS_DOWNLOADINFO_INSTRUMENT2:
        return synth_download_instrument(This, info, offsets, data, ret_handle);
    case DMUS_DOWNLOADINFO_WAVE:
        return synth_download_wave(This, info, offsets, data, ret_handle);
    case DMUS_DOWNLOADINFO_WAVEARTICULATION:
        FIXME("Download type DMUS_DOWNLOADINFO_WAVEARTICULATION not yet supported\n");
        return E_NOTIMPL;
    case DMUS_DOWNLOADINFO_STREAMINGWAVE:
        FIXME("Download type DMUS_DOWNLOADINFO_STREAMINGWAVE not yet supported\n");
        return E_NOTIMPL;
    case DMUS_DOWNLOADINFO_ONESHOTWAVE:
        FIXME("Download type DMUS_DOWNLOADINFO_ONESHOTWAVE not yet supported\n");
        return E_NOTIMPL;
    default:
        WARN("Unknown download type %lu\n", info->dwDLType);
        return DMUS_E_UNKNOWNDOWNLOAD;
    }

    return S_OK;
}

static HRESULT WINAPI synth_Unload(IDirectMusicSynth8 *iface, HANDLE handle,
        HRESULT (CALLBACK *callback)(HANDLE, HANDLE), HANDLE user_data)
{
    struct synth *This = impl_from_IDirectMusicSynth8(iface);
    struct instrument *instrument;
    struct wave *wave;

    TRACE("(%p)->(%p, %p, %p)\n", This, handle, callback, user_data);
    if (callback) FIXME("Unload callbacks not implemented\n");

    LIST_FOR_EACH_ENTRY(instrument, &This->instruments, struct instrument, entry)
    {
        if (instrument == handle)
        {
            list_remove(&instrument->entry);
            instrument_release(instrument);
            return S_OK;
        }
    }

    LIST_FOR_EACH_ENTRY(wave, &This->waves, struct wave, entry)
    {
        if (wave == handle)
        {
            list_remove(&wave->entry);
            wave_release(wave);
            return S_OK;
        }
    }

    return E_FAIL;
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
    {
        This->active = FALSE;
        return enable ? DMUS_E_NOSYNTHSINK : DMUS_E_SYNTHNOTCONFIGURED;
    }

    if (FAILED(hr = IDirectMusicSynthSink_Activate(This->sink, enable))
            && hr != DMUS_E_SYNTHACTIVE)
    {
        This->active = FALSE;
        return DMUS_E_SYNTHNOTCONFIGURED;
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

static const char *synth_sfont_get_name(fluid_sfont_t *fluid_sfont)
{
    return "DirectMusicSynth";
}

static fluid_preset_t *synth_sfont_get_preset(fluid_sfont_t *fluid_sfont, int bank, int patch)
{
    struct synth *synth = fluid_sfont_get_data(fluid_sfont);
    struct instrument *instrument;

    TRACE("(%p, %d, %d)\n", fluid_sfont, bank, patch);

    if (!synth) return NULL;

    LIST_FOR_EACH_ENTRY(instrument, &synth->instruments, struct instrument, entry)
        if (instrument->patch == patch) break;
    if (&instrument->entry == &synth->instruments) return NULL;

    FIXME("Preset not implemented yet\n");
    return NULL;
}

static void synth_sfont_iter_start(fluid_sfont_t *fluid_sfont)
{
    FIXME("(%p): stub\n", fluid_sfont);
}

static fluid_preset_t *synth_sfont_iter_next(fluid_sfont_t *fluid_sfont)
{
    FIXME("(%p): stub\n", fluid_sfont);
    return NULL;
}

static int synth_sfont_free(fluid_sfont_t *fluid_sfont)
{
    return 0;
}

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

    list_init(&obj->instruments);
    list_init(&obj->waves);

    if (!(obj->fluid_settings = new_fluid_settings())) goto failed;
    if (!(obj->fluid_sfont = new_fluid_sfont(synth_sfont_get_name, synth_sfont_get_preset,
            synth_sfont_iter_start, synth_sfont_iter_next, synth_sfont_free)))
        goto failed;
    fluid_sfont_set_data(obj->fluid_sfont, obj);

    TRACE("Created DirectMusicSynth %p\n", obj);
    *ret_iface = (IUnknown *)&obj->IDirectMusicSynth8_iface;
    return S_OK;

failed:
    delete_fluid_settings(obj->fluid_settings);
    free(obj);
    return E_OUTOFMEMORY;
}
