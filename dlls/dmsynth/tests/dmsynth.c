/*
 * Unit tests for dmsynth functions
 *
 * Copyright (C) 2012 Christian Costa
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define COBJMACROS

#include <stdio.h>

#include "wine/test.h"
#include "uuids.h"
#include "ole2.h"
#include "initguid.h"
#include "dmusics.h"
#include "dmusici.h"
#include "dmksctrl.h"

static BOOL missing_dmsynth(void)
{
    IDirectMusicSynth *dms;
    HRESULT hr = CoCreateInstance(&CLSID_DirectMusicSynth, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicSynth, (void**)&dms);

    if (hr == S_OK && dms)
    {
        IDirectMusicSynth_Release(dms);
        return FALSE;
    }
    return TRUE;
}

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported)
{
    ULONG expect_ref = get_refcount(iface_ptr);
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected;
    IUnknown *unk;

    expected = supported ? S_OK : E_NOINTERFACE;
    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected, "got hr %#lx, expected %#lx.\n", hr, expected);
    if (SUCCEEDED(hr))
    {
        LONG ref = get_refcount(unk);
        ok_(__FILE__, line)(ref == expect_ref + 1, "got %ld\n", ref);
        IUnknown_Release(unk);
        ref = get_refcount(iface_ptr);
        ok_(__FILE__, line)(ref == expect_ref, "got %ld\n", ref);
    }
}

struct test_synth
{
    IDirectMusicSynth8 IDirectMusicSynth8_iface;
    LONG refcount;
};

static HRESULT WINAPI test_synth_QueryInterface(IDirectMusicSynth8 *iface, REFIID riid, void **ret_iface)
{
    ok(0, "unexpected %s\n", __func__);
    return S_OK;
}

static ULONG WINAPI test_synth_AddRef(IDirectMusicSynth8 *iface)
{
    struct test_synth *sink = CONTAINING_RECORD(iface, struct test_synth, IDirectMusicSynth8_iface);
    return InterlockedIncrement(&sink->refcount);
}

static ULONG WINAPI test_synth_Release(IDirectMusicSynth8 *iface)
{
    struct test_synth *sink = CONTAINING_RECORD(iface, struct test_synth, IDirectMusicSynth8_iface);
    ULONG ref = InterlockedDecrement(&sink->refcount);
    if (!ref) free(sink);
    return ref;
}

static HRESULT WINAPI test_synth_Open(IDirectMusicSynth8 *iface, DMUS_PORTPARAMS *params)
{
    ok(0, "unexpected %s\n", __func__);
    return S_OK;
}

static HRESULT WINAPI test_synth_Close(IDirectMusicSynth8 *iface)
{
    ok(0, "unexpected %s\n", __func__);
    return S_OK;
}

static HRESULT WINAPI test_synth_SetNumChannelGroups(IDirectMusicSynth8 *iface, DWORD groups)
{
    ok(0, "unexpected %s\n", __func__);
    return S_OK;
}

static HRESULT WINAPI test_synth_Download(IDirectMusicSynth8 *iface, HANDLE *handle, void *data, BOOL *can_free)
{
    ok(0, "unexpected %s\n", __func__);
    return S_OK;
}

static HRESULT WINAPI test_synth_Unload(IDirectMusicSynth8 *iface, HANDLE handle,
        HRESULT (CALLBACK *free_callback)(HANDLE,HANDLE), HANDLE user_data)
{
    ok(0, "unexpected %s\n", __func__);
    return S_OK;
}

static HRESULT WINAPI test_synth_PlayBuffer(IDirectMusicSynth8 *iface, REFERENCE_TIME time, BYTE *buffer, DWORD size)
{
    ok(0, "unexpected %s\n", __func__);
    return S_OK;
}

static HRESULT WINAPI test_synth_GetRunningStats(IDirectMusicSynth8 *iface, DMUS_SYNTHSTATS *stats)
{
    ok(0, "unexpected %s\n", __func__);
    return S_OK;
}

static HRESULT WINAPI test_synth_GetPortCaps(IDirectMusicSynth8 *iface, DMUS_PORTCAPS *caps)
{
    ok(0, "unexpected %s\n", __func__);
    return S_OK;
}

static HRESULT WINAPI test_synth_SetMasterClock(IDirectMusicSynth8 *iface, IReferenceClock *clock)
{
    ok(0, "unexpected %s\n", __func__);
    return S_OK;
}

static HRESULT WINAPI test_synth_GetLatencyClock(IDirectMusicSynth8 *iface, IReferenceClock **clock)
{
    ok(0, "unexpected %s\n", __func__);
    return S_OK;
}

static HRESULT WINAPI test_synth_Activate(IDirectMusicSynth8 *iface, BOOL enable)
{
    ok(0, "unexpected %s\n", __func__);
    return S_OK;
}

static HRESULT WINAPI test_synth_SetSynthSink(IDirectMusicSynth8 *iface, IDirectMusicSynthSink *sink)
{
    ok(0, "unexpected %s\n", __func__);
    return S_OK;
}

static HRESULT WINAPI test_synth_Render(IDirectMusicSynth8 *iface, short *buffer, DWORD length, LONGLONG position)
{
    return S_OK;
}

static HRESULT WINAPI test_synth_SetChannelPriority(IDirectMusicSynth8 *iface, DWORD group, DWORD channel, DWORD priority)
{
    ok(0, "unexpected %s\n", __func__);
    return S_OK;
}

static HRESULT WINAPI test_synth_GetChannelPriority(IDirectMusicSynth8 *iface, DWORD group, DWORD channel, DWORD *priority)
{
    ok(0, "unexpected %s\n", __func__);
    return S_OK;
}

static HRESULT WINAPI test_synth_GetFormat(IDirectMusicSynth8 *iface, WAVEFORMATEX *format, DWORD *ext_size)
{
    *ext_size = 0;
    memset(format, 0, sizeof(*format));
    format->wFormatTag = WAVE_FORMAT_PCM;
    format->nChannels = 2;
    format->wBitsPerSample = 16;
    format->nSamplesPerSec = 44100;
    format->nBlockAlign = format->nChannels * format->wBitsPerSample / 8;
    format->nAvgBytesPerSec = format->nSamplesPerSec * format->nBlockAlign;
    return S_OK;
}

static HRESULT WINAPI test_synth_GetAppend(IDirectMusicSynth8 *iface, DWORD *append)
{
    ok(0, "unexpected %s\n", __func__);
    return S_OK;
}

static HRESULT WINAPI test_synth_PlayVoice(IDirectMusicSynth8 *iface, REFERENCE_TIME rt, DWORD voice_id,
        DWORD group, DWORD channel, DWORD dlid, LONG pitch, LONG volume, SAMPLE_TIME voice_start,
        SAMPLE_TIME loop_start, SAMPLE_TIME loop_end)
{
    ok(0, "unexpected %s\n", __func__);
    return S_OK;
}

static HRESULT WINAPI test_synth_StopVoice(IDirectMusicSynth8 *iface, REFERENCE_TIME rt, DWORD voice_id)
{
    ok(0, "unexpected %s\n", __func__);
    return S_OK;
}

static HRESULT WINAPI test_synth_GetVoiceState(IDirectMusicSynth8 *iface, DWORD voice_buf[], DWORD voice_len,
        DMUS_VOICE_STATE voice_state[])
{
    ok(0, "unexpected %s\n", __func__);
    return S_OK;
}

static HRESULT WINAPI test_synth_Refresh(IDirectMusicSynth8 *iface, DWORD dlid, DWORD flags)
{
    ok(0, "unexpected %s\n", __func__);
    return S_OK;
}

static HRESULT WINAPI test_synth_AssignChannelToBuses(IDirectMusicSynth8 *iface, DWORD group, DWORD channel,
        DWORD *buses, DWORD buses_count)
{
    ok(0, "unexpected %s\n", __func__);
    return S_OK;
}

static const IDirectMusicSynth8Vtbl test_synth_vtbl =
{
    test_synth_QueryInterface,
    test_synth_AddRef,
    test_synth_Release,
    test_synth_Open,
    test_synth_Close,
    test_synth_SetNumChannelGroups,
    test_synth_Download,
    test_synth_Unload,
    test_synth_PlayBuffer,
    test_synth_GetRunningStats,
    test_synth_GetPortCaps,
    test_synth_SetMasterClock,
    test_synth_GetLatencyClock,
    test_synth_Activate,
    test_synth_SetSynthSink,
    test_synth_Render,
    test_synth_SetChannelPriority,
    test_synth_GetChannelPriority,
    test_synth_GetFormat,
    test_synth_GetAppend,
    test_synth_PlayVoice,
    test_synth_StopVoice,
    test_synth_GetVoiceState,
    test_synth_Refresh,
    test_synth_AssignChannelToBuses,
};

static HRESULT test_synth_create(IDirectMusicSynth8 **out)
{
    struct test_synth *synth;

    *out = NULL;
    if (!(synth = calloc(1, sizeof(*synth)))) return E_OUTOFMEMORY;
    synth->IDirectMusicSynth8_iface.lpVtbl = &test_synth_vtbl;
    synth->refcount = 1;

    *out = &synth->IDirectMusicSynth8_iface;
    return S_OK;
}

static void test_synth_getformat(IDirectMusicSynth *synth, DMUS_PORTPARAMS *params, const char *context)
{
    WAVEFORMATEX format;
    DWORD size;
    HRESULT hr;

    winetest_push_context("%s", context);

    size = sizeof(format);
    hr = IDirectMusicSynth_GetFormat(synth, &format, &size);
    ok(hr == S_OK, "GetFormat failed: %#lx\n", hr);
    ok(format.wFormatTag == WAVE_FORMAT_PCM, "wFormatTag: %#x\n", format.wFormatTag);
    ok(format.nChannels == params->dwAudioChannels, "nChannels: %d\n", format.nChannels);
    ok(format.nSamplesPerSec == params->dwSampleRate, "nSamplesPerSec: %ld\n", format.nSamplesPerSec);
    ok(format.wBitsPerSample == 16, "wBitsPerSample: %d\n", format.wBitsPerSample);
    ok(format.nBlockAlign == params->dwAudioChannels * format.wBitsPerSample / 8, "nBlockAlign: %d\n",
            format.nBlockAlign);
    ok(format.nAvgBytesPerSec == params->dwSampleRate * format.nBlockAlign, "nAvgBytesPerSec: %ld\n",
            format.nAvgBytesPerSec);
    ok(format.cbSize == 0, "cbSize: %d\n", format.cbSize);

    winetest_pop_context();
}

static void test_dmsynth(void)
{
    IDirectMusicSynth *dmsynth = NULL;
    IDirectMusicSynthSink *dmsynth_sink = NULL, *dmsynth_sink2 = NULL;
    IReferenceClock* clock_synth = NULL;
    IReferenceClock* clock_sink = NULL;
    IKsControl* control_synth = NULL;
    IKsControl* control_sink = NULL;
    ULONG ref_clock_synth, ref_clock_sink;
    HRESULT hr;
    KSPROPERTY property;
    ULONG value;
    ULONG bytes;
    DMUS_PORTCAPS caps;
    DMUS_PORTPARAMS params;
    const DWORD all_params = DMUS_PORTPARAMS_VOICES|DMUS_PORTPARAMS_CHANNELGROUPS|DMUS_PORTPARAMS_AUDIOCHANNELS|
            DMUS_PORTPARAMS_SAMPLERATE|DMUS_PORTPARAMS_EFFECTS|DMUS_PORTPARAMS_SHARE|DMUS_PORTPARAMS_FEATURES;
    WAVEFORMATEX format;
    DWORD size;

    hr = CoCreateInstance(&CLSID_DirectMusicSynth, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicSynth, (LPVOID*)&dmsynth);
    ok(hr == S_OK, "CoCreateInstance returned: %#lx\n", hr);

    hr = CoCreateInstance(&CLSID_DirectMusicSynthSink, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicSynthSink,
            (void **)&dmsynth_sink);
    ok(hr == S_OK, "CoCreateInstance returned: %#lx\n", hr);
    hr = CoCreateInstance(&CLSID_DirectMusicSynthSink, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicSynthSink,
            (void **)&dmsynth_sink2);
    ok(hr == S_OK, "CoCreateInstance returned: %#lx\n", hr);

    hr = IDirectMusicSynth_QueryInterface(dmsynth, &IID_IKsControl, (LPVOID*)&control_synth);
    ok(hr == S_OK, "IDirectMusicSynth_QueryInterface returned: %#lx\n", hr);

    property.Id = 0;
    property.Flags = KSPROPERTY_TYPE_GET;

    property.Set = GUID_DMUS_PROP_INSTRUMENT2;
    hr = IKsControl_KsProperty(control_synth, &property, sizeof(property), &value, sizeof(value), &bytes);
    ok(hr == S_OK, "IKsControl_KsProperty returned: %#lx\n", hr);
    ok(bytes == sizeof(DWORD), "Returned bytes: %lu, should be 4\n", bytes);
    ok(value == TRUE, "Return value: %lu, should be 1\n", value);
    property.Set = GUID_DMUS_PROP_DLS2;
    hr = IKsControl_KsProperty(control_synth, &property, sizeof(property), &value, sizeof(value), &bytes);
    ok(hr == S_OK, "IKsControl_KsProperty returned: %#lx\n", hr);
    ok(bytes == sizeof(DWORD), "Returned bytes: %lu, should be 4\n", bytes);
    ok(value == TRUE, "Return value: %lu, should be 1\n", value);
    property.Set = GUID_DMUS_PROP_GM_Hardware;
    hr = IKsControl_KsProperty(control_synth, &property, sizeof(property), &value, sizeof(value), &bytes);
    ok(hr == S_OK, "IKsControl_KsProperty returned: %#lx\n", hr);
    ok(bytes == sizeof(DWORD), "Returned bytes: %lu, should be 4\n", bytes);
    ok(value == FALSE, "Return value: %lu, should be 0\n", value);
    property.Set = GUID_DMUS_PROP_GS_Hardware;
    hr = IKsControl_KsProperty(control_synth, &property, sizeof(property), &value, sizeof(value), &bytes);
    ok(hr == S_OK, "IKsControl_KsProperty returned: %#lx\n", hr);
    ok(bytes == sizeof(DWORD), "Returned bytes: %lu, should be 4\n", bytes);
    ok(value == FALSE, "Return value: %lu, should be 0\n", value);
    property.Set = GUID_DMUS_PROP_XG_Hardware;
    hr = IKsControl_KsProperty(control_synth, &property, sizeof(property), &value, sizeof(value), &bytes);
    ok(hr == S_OK, "IKsControl_KsProperty returned: %#lx\n", hr);
    ok(bytes == sizeof(DWORD), "Returned bytes: %lu, should be 4\n", bytes);
    ok(value == FALSE, "Return value: %lu, should be 0\n", value);

    hr = IDirectMusicSynthSink_QueryInterface(dmsynth_sink, &IID_IKsControl, (LPVOID*)&control_sink);
    ok(hr == S_OK, "IDirectMusicSynthSink_QueryInterface returned: %#lx\n", hr);

    property.Set = GUID_DMUS_PROP_SinkUsesDSound;
    hr = IKsControl_KsProperty(control_sink, &property, sizeof(property), &value, sizeof(value), &bytes);
    ok(hr == S_OK, "IKsControl_KsProperty returned: %#lx\n", hr);
    ok(bytes == sizeof(DWORD), "Returned bytes: %lu, should be 4\n", bytes);
    ok(value == TRUE, "Return value: %lu, should be 1\n", value);

    /* Synth isn't fully initialized yet */
    hr = IDirectMusicSynth_Activate(dmsynth, TRUE);
    ok(hr == DMUS_E_NOSYNTHSINK, "IDirectMusicSynth_Activate returned: %#lx\n", hr);
    hr = IDirectMusicSynthSink_GetDesiredBufferSize(dmsynth_sink, &size);
    ok(hr == DMUS_E_SYNTHNOTCONFIGURED, "IDirectMusicSynthSink_GetDesiredBufferSize returned: %#lx\n", hr);

    /* Open / Close */
    hr = IDirectMusicSynth_Open(dmsynth, NULL);
    ok(hr == S_OK, "Open failed: %#lx\n", hr);
    hr = IDirectMusicSynth_Open(dmsynth, NULL);
    ok(hr == DMUS_E_ALREADYOPEN, "Open failed: %#lx\n", hr);
    hr = IDirectMusicSynth_Close(dmsynth);
    ok(hr == S_OK, "Close failed: %#lx\n", hr);
    hr = IDirectMusicSynth_Close(dmsynth);
    ok(hr == DMUS_E_ALREADYCLOSED, "Close failed: %#lx\n", hr);
    memset(&params, 0, sizeof(params));
    hr = IDirectMusicSynth_Open(dmsynth, &params);
    ok(hr == E_INVALIDARG, "Open failed: %#lx\n", hr);
    params.dwSize = sizeof(DMUS_PORTPARAMS7);
    hr = IDirectMusicSynth_Open(dmsynth, &params);
    ok(hr == S_OK, "Open failed: %#lx\n", hr);
    IDirectMusicSynth_Close(dmsynth);
    /* All params supported and set to 0 */
    params.dwSize = sizeof(params);
    params.dwValidParams = all_params;
    hr = IDirectMusicSynth_Open(dmsynth, &params);
    ok(hr == S_OK, "Open failed: %#lx\n", hr);
    ok(params.dwSize == sizeof(params), "dwSize: %ld\n", params.dwSize);
    ok(params.dwValidParams == all_params, "dwValidParams: %#lx\n", params.dwValidParams);
    ok(params.dwVoices == 32, "dwVoices: %ld\n", params.dwVoices);
    ok(params.dwChannelGroups == 2, "dwChannelGroups: %ld\n", params.dwChannelGroups);
    ok(params.dwAudioChannels == 2, "dwAudioChannels: %ld\n", params.dwAudioChannels);
    ok(params.dwSampleRate == 22050, "dwSampleRate: %ld\n", params.dwSampleRate);
    ok(params.dwEffectFlags == DMUS_EFFECT_REVERB, "params.dwEffectFlags: %#lx\n", params.dwEffectFlags);
    ok(params.fShare == FALSE, "fShare: %d\n", params.fShare);
    ok(params.dwFeatures == 0, "dwFeatures: %#lx\n", params.dwFeatures);
    test_synth_getformat(dmsynth, &params, "defaults");
    IDirectMusicSynth_Close(dmsynth);
    /* Requesting more than supported */
    params.dwValidParams = DMUS_PORTPARAMS_SAMPLERATE;
    params.dwSampleRate = 100003;
    hr = IDirectMusicSynth_Open(dmsynth, &params);
    ok(hr == S_FALSE, "Open failed: %#lx\n", hr);
    ok(params.dwValidParams == all_params, "dwValidParams: %#lx\n", params.dwValidParams);
    ok(params.dwSampleRate == 96000, "dwSampleRate: %ld\n", params.dwSampleRate);
    test_synth_getformat(dmsynth, &params, "max");
    IDirectMusicSynth_Close(dmsynth);
    /* Minimums */
    params.dwValidParams = DMUS_PORTPARAMS_VOICES|DMUS_PORTPARAMS_CHANNELGROUPS|DMUS_PORTPARAMS_AUDIOCHANNELS|
            DMUS_PORTPARAMS_SAMPLERATE;
    params.dwVoices = 1;
    params.dwChannelGroups = 1;
    params.dwAudioChannels = 1;
    params.dwSampleRate = 1;
    hr = IDirectMusicSynth_Open(dmsynth, &params);
    ok(hr == S_FALSE, "Open failed: %#lx\n", hr);
    ok(params.dwValidParams == all_params, "dwValidParams: %#lx\n", params.dwValidParams);
    ok(params.dwVoices == 1, "dwVoices: %ld\n", params.dwVoices);
    ok(params.dwChannelGroups == 1, "dwChannelGroups: %ld\n", params.dwChannelGroups);
    todo_wine ok(params.dwAudioChannels == 1, "dwAudioChannels: %ld\n", params.dwAudioChannels);
    ok(params.dwSampleRate == 11025, "dwSampleRate: %ld\n", params.dwSampleRate);
    test_synth_getformat(dmsynth, &params, "min");
    IDirectMusicSynth_Close(dmsynth);
    /* Test share and features */
    params.dwValidParams = DMUS_PORTPARAMS_SHARE|DMUS_PORTPARAMS_FEATURES;
    params.fShare = TRUE;
    params.dwFeatures = DMUS_PORT_FEATURE_AUDIOPATH|DMUS_PORT_FEATURE_STREAMING;
    hr = IDirectMusicSynth_Open(dmsynth, &params);
    ok(hr == S_FALSE, "Open failed: %#lx\n", hr);
    ok(params.dwValidParams == all_params, "dwValidParams: %#lx\n", params.dwValidParams);
    ok(params.fShare == FALSE, "fShare: %d\n", params.fShare);
    ok(params.dwFeatures == (DMUS_PORT_FEATURE_AUDIOPATH|DMUS_PORT_FEATURE_STREAMING),
            "dwFeatures: %#lx\n", params.dwFeatures);
    test_synth_getformat(dmsynth, &params, "features");
    IDirectMusicSynth_Close(dmsynth);

    /* Synth has no default clock */
    hr = IDirectMusicSynth_GetLatencyClock(dmsynth, &clock_synth);
    ok(hr == DMUS_E_NOSYNTHSINK, "IDirectMusicSynth_GetLatencyClock returned: %#lx\n", hr);

    /* SynthSink has a default clock */
    hr = IDirectMusicSynthSink_GetLatencyClock(dmsynth_sink, &clock_sink);
    ok(hr == S_OK, "IDirectMusicSynth_GetLatencyClock returned: %#lx\n", hr);
    ok(clock_sink != NULL, "No clock returned\n");
    ref_clock_sink = get_refcount(clock_sink);

    /* This will Init() the SynthSink and finish initializing the Synth */
    hr = IDirectMusicSynthSink_Init(dmsynth_sink2, NULL);
    ok(hr == S_OK, "IDirectMusicSynthSink_Init returned: %#lx\n", hr);
    hr = IDirectMusicSynth_SetSynthSink(dmsynth, dmsynth_sink2);
    ok(hr == S_OK, "IDirectMusicSynth_SetSynthSink returned: %#lx\n", hr);
    hr = IDirectMusicSynth_SetSynthSink(dmsynth, dmsynth_sink);
    ok(hr == S_OK, "IDirectMusicSynth_SetSynthSink returned: %#lx\n", hr);

    /* Check clocks are the same */
    hr = IDirectMusicSynth_GetLatencyClock(dmsynth, &clock_synth);
    ok(hr == S_OK, "IDirectMusicSynth_GetLatencyClock returned: %#lx\n", hr);
    ok(clock_synth != NULL, "No clock returned\n");
    ok(clock_synth == clock_sink, "Synth and SynthSink clocks are not the same\n");
    ref_clock_synth = get_refcount(clock_synth);
    ok(ref_clock_synth > ref_clock_sink + 1, "Latency clock refcount didn't increase\n");

    /* GetPortCaps */
    hr = IDirectMusicSynth_GetPortCaps(dmsynth, NULL);
    ok(hr == E_INVALIDARG, "GetPortCaps failed: %#lx\n", hr);
    memset(&caps, 0, sizeof(caps));
    hr = IDirectMusicSynth_GetPortCaps(dmsynth, &caps);
    ok(hr == E_INVALIDARG, "GetPortCaps failed: %#lx\n", hr);
    caps.dwSize = sizeof(caps) + 1;
    hr = IDirectMusicSynth_GetPortCaps(dmsynth, &caps);
    ok(hr == S_OK, "GetPortCaps failed: %#lx\n", hr);

    /* GetFormat */
    hr = IDirectMusicSynth_GetFormat(dmsynth, NULL, NULL);
    ok(hr == E_POINTER, "GetFormat failed: %#lx\n", hr);
    hr = IDirectMusicSynth_GetFormat(dmsynth, NULL, &size);
    ok(hr == DMUS_E_SYNTHNOTCONFIGURED, "GetFormat failed: %#lx\n", hr);
    hr = IDirectMusicSynth_Open(dmsynth, NULL);
    ok(hr == S_OK, "Open failed: %#lx\n", hr);
    hr = IDirectMusicSynth_GetFormat(dmsynth, NULL, &size);
    ok(hr == S_OK, "GetFormat failed: %#lx\n", hr);
    ok(size == sizeof(format), "GetFormat size mismatch, got %ld\n", size);
    size = 1;
    hr = IDirectMusicSynth_GetFormat(dmsynth, &format, &size);
    ok(hr == S_OK, "GetFormat failed: %#lx\n", hr);
    ok(size == sizeof(format), "GetFormat size mismatch, got %ld\n", size);
    size = sizeof(format) + 1;
    hr = IDirectMusicSynth_GetFormat(dmsynth, &format, &size);
    ok(hr == S_OK, "GetFormat failed: %#lx\n", hr);
    ok(size == sizeof(format), "GetFormat size mismatch, got %ld\n", size);
    IDirectMusicSynth_Close(dmsynth);

    /* GetDesiredBufferSize */
    hr = IDirectMusicSynthSink_GetDesiredBufferSize(dmsynth_sink, NULL);
    ok(hr == E_POINTER, "IDirectMusicSynthSink_GetDesiredBufferSize returned: %#lx\n", hr);
    hr = IDirectMusicSynthSink_GetDesiredBufferSize(dmsynth_sink, &size);
    ok(hr == E_UNEXPECTED, "IDirectMusicSynthSink_GetDesiredBufferSize returned: %#lx\n", hr);
    params.dwValidParams = 0;
    hr = IDirectMusicSynth_Open(dmsynth, &params);
    ok(hr == S_OK, "Open failed: %#lx\n", hr);
    hr = IDirectMusicSynthSink_GetDesiredBufferSize(dmsynth_sink, &size);
    ok(hr == S_OK, "IDirectMusicSynthSink_GetDesiredBufferSize returned: %#lx\n", hr);
    ok(size == params.dwSampleRate * params.dwAudioChannels * 4, "size: %ld\n", size);
    IDirectMusicSynth_Close(dmsynth);
    params.dwValidParams = DMUS_PORTPARAMS_AUDIOCHANNELS;
    params.dwAudioChannels = 1;
    hr = IDirectMusicSynth_Open(dmsynth, &params);
    todo_wine_if(SUCCEEDED(hr)) ok(hr == S_OK, "Open failed: %#lx\n", hr);
    hr = IDirectMusicSynthSink_GetDesiredBufferSize(dmsynth_sink, &size);
    ok(hr == S_OK, "IDirectMusicSynthSink_GetDesiredBufferSize returned: %#lx\n", hr);
    ok(size == params.dwSampleRate * params.dwAudioChannels * 4, "size: %ld\n", size);
    IDirectMusicSynth_Close(dmsynth);

    if (control_synth)
        IDirectMusicSynth_Release(control_synth);
    if (control_sink)
        IDirectMusicSynth_Release(control_sink);
    if (clock_synth)
        IReferenceClock_Release(clock_synth);
    if (clock_sink)
        IReferenceClock_Release(clock_sink);
    if (dmsynth_sink)
        IDirectMusicSynthSink_Release(dmsynth_sink);
    if (dmsynth_sink2)
        IDirectMusicSynthSink_Release(dmsynth_sink2);
    IDirectMusicSynth_Release(dmsynth);
}

static void test_COM(void)
{
    IDirectMusicSynth8 *dms8 = (IDirectMusicSynth8*)0xdeadbeef;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectMusicSynth, (IUnknown *)0xdeadbeef, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dms8);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicSynth create failed: %#lx, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dms8, "dms8 = %p\n", dms8);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicSynth, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void**)&dms8);
    ok(hr == E_NOINTERFACE, "DirectMusicSynth create failed: %#lx, expected E_NOINTERFACE\n", hr);

    hr = CoCreateInstance(&CLSID_DirectMusicSynth, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicSynth8, (void**)&dms8);
    ok(hr == S_OK, "DirectMusicSynth create failed: %#lx, expected S_OK\n", hr);

    check_interface(dms8, &IID_IUnknown, TRUE);
    check_interface(dms8, &IID_IKsControl, TRUE);

    /* Unsupported interfaces */
    check_interface(dms8, &IID_IDirectMusicSynthSink, FALSE);
    check_interface(dms8, &IID_IReferenceClock, FALSE);

    IDirectMusicSynth8_Release(dms8);
}

static void test_COM_synthsink(void)
{
    IDirectMusicSynthSink *dmss = (IDirectMusicSynthSink*)0xdeadbeef;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectMusicSynthSink, (IUnknown *)0xdeadbeef, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dmss);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicSynthSink create failed: %#lx, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dmss, "dmss = %p\n", dmss);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicSynthSink, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void**)&dmss);
    ok(hr == E_NOINTERFACE, "DirectMusicSynthSink create failed: %#lx, expected E_NOINTERFACE\n", hr);

    /* Same refcount for all DirectMusicSynthSink interfaces */
    hr = CoCreateInstance(&CLSID_DirectMusicSynthSink, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicSynthSink, (void**)&dmss);
    ok(hr == S_OK, "DirectMusicSynthSink create failed: %#lx, expected S_OK\n", hr);

    check_interface(dmss, &IID_IUnknown, TRUE);
    check_interface(dmss, &IID_IKsControl, TRUE);

    /* Unsupported interfaces */
    check_interface(dmss, &IID_IReferenceClock, FALSE);

    IDirectMusicSynthSink_Release(dmss);
}

struct test_sink
{
    IDirectMusicSynthSink IDirectMusicSynthSink_iface;
    IReferenceClock IReferenceClock_iface;
    LONG refcount;

    IReferenceClock *clock;
    IDirectMusicSynth *synth;
    REFERENCE_TIME activate_time;
    REFERENCE_TIME latency_time;
    DWORD written;
};

static HRESULT WINAPI test_sink_QueryInterface(IDirectMusicSynthSink *iface, REFIID riid, void **ret_iface)
{
    ok(0, "unexpected %s\n", __func__);
    return S_OK;
}

static ULONG WINAPI test_sink_AddRef(IDirectMusicSynthSink *iface)
{
    struct test_sink *sink = CONTAINING_RECORD(iface, struct test_sink, IDirectMusicSynthSink_iface);
    return InterlockedIncrement(&sink->refcount);
}

static ULONG WINAPI test_sink_Release(IDirectMusicSynthSink *iface)
{
    struct test_sink *sink = CONTAINING_RECORD(iface, struct test_sink, IDirectMusicSynthSink_iface);
    ULONG ref = InterlockedDecrement(&sink->refcount);
    if (!ref) free(sink);
    return ref;
}

static HRESULT WINAPI test_sink_Init(IDirectMusicSynthSink *iface, IDirectMusicSynth *synth)
{
    struct test_sink *sink = CONTAINING_RECORD(iface, struct test_sink, IDirectMusicSynthSink_iface);
    sink->synth = synth;
    return S_OK;
}

static HRESULT WINAPI test_sink_SetMasterClock(IDirectMusicSynthSink *iface, IReferenceClock *clock)
{
    struct test_sink *sink = CONTAINING_RECORD(iface, struct test_sink, IDirectMusicSynthSink_iface);
    if (sink->clock)
        IReferenceClock_Release(sink->clock);
    if ((sink->clock = clock))
        IReferenceClock_AddRef(sink->clock);
    return S_OK;
}

static HRESULT WINAPI test_sink_GetLatencyClock(IDirectMusicSynthSink *iface, IReferenceClock **clock)
{
    struct test_sink *sink = CONTAINING_RECORD(iface, struct test_sink, IDirectMusicSynthSink_iface);
    *clock = &sink->IReferenceClock_iface;
    IReferenceClock_AddRef(*clock);
    return S_OK;
}

static HRESULT WINAPI test_sink_Activate(IDirectMusicSynthSink *iface, BOOL enable)
{
    struct test_sink *sink = CONTAINING_RECORD(iface, struct test_sink, IDirectMusicSynthSink_iface);

    if (!sink->clock)
        return DMUS_E_NO_MASTER_CLOCK;

    IReferenceClock_GetTime(sink->clock, &sink->activate_time);
    sink->latency_time = sink->activate_time;
    return S_OK;
}

static HRESULT WINAPI test_sink_SampleToRefTime(IDirectMusicSynthSink *iface, LONGLONG sample, REFERENCE_TIME *time)
{
    struct test_sink *sink = CONTAINING_RECORD(iface, struct test_sink, IDirectMusicSynthSink_iface);
    WAVEFORMATEX format;
    DWORD format_size = sizeof(format);
    HRESULT hr;

    hr = IDirectMusicSynth_GetFormat(sink->synth, &format, &format_size);
    ok(hr == S_OK, "got %#lx\n", hr);

    *time = sink->activate_time + ((sample * 10000) / format.nSamplesPerSec) * 1000;
    return S_OK;
}

static HRESULT WINAPI test_sink_RefTimeToSample(IDirectMusicSynthSink *iface, REFERENCE_TIME time, LONGLONG *sample)
{
    struct test_sink *sink = CONTAINING_RECORD(iface, struct test_sink, IDirectMusicSynthSink_iface);
    WAVEFORMATEX format;
    DWORD format_size = sizeof(format);
    HRESULT hr;

    hr = IDirectMusicSynth_GetFormat(sink->synth, &format, &format_size);
    ok(hr == S_OK, "got %#lx\n", hr);

    *sample = (((time - sink->activate_time) / 1000) * format.nSamplesPerSec) / 10000;
    return S_OK;
}

static HRESULT WINAPI test_sink_SetDirectSound(IDirectMusicSynthSink *iface, IDirectSound *dsound,
        IDirectSoundBuffer *dsound_buffer)
{
    ok(0, "unexpected %s\n", __func__);
    return S_OK;
}

static HRESULT WINAPI test_sink_GetDesiredBufferSize(IDirectMusicSynthSink *iface, DWORD *size)
{
    ok(0, "unexpected %s\n", __func__);
    return S_OK;
}

static const IDirectMusicSynthSinkVtbl test_sink_vtbl =
{
    test_sink_QueryInterface,
    test_sink_AddRef,
    test_sink_Release,
    test_sink_Init,
    test_sink_SetMasterClock,
    test_sink_GetLatencyClock,
    test_sink_Activate,
    test_sink_SampleToRefTime,
    test_sink_RefTimeToSample,
    test_sink_SetDirectSound,
    test_sink_GetDesiredBufferSize,
};

static HRESULT WINAPI test_sink_latency_clock_QueryInterface(IReferenceClock *iface, REFIID iid, void **out)
{
    ok(0, "unexpected %s\n", __func__);
    return E_NOINTERFACE;
}

static ULONG WINAPI test_sink_latency_clock_AddRef(IReferenceClock *iface)
{
    struct test_sink *sink = CONTAINING_RECORD(iface, struct test_sink, IReferenceClock_iface);
    return IDirectMusicSynthSink_AddRef(&sink->IDirectMusicSynthSink_iface);
}

static ULONG WINAPI test_sink_latency_clock_Release(IReferenceClock *iface)
{
    struct test_sink *sink = CONTAINING_RECORD(iface, struct test_sink, IReferenceClock_iface);
    return IDirectMusicSynthSink_Release(&sink->IDirectMusicSynthSink_iface);
}

static HRESULT WINAPI test_sink_latency_clock_GetTime(IReferenceClock *iface, REFERENCE_TIME *time)
{
    struct test_sink *sink = CONTAINING_RECORD(iface, struct test_sink, IReferenceClock_iface);
    *time = sink->latency_time;
    return S_OK;
}

static HRESULT WINAPI test_sink_latency_clock_AdviseTime(IReferenceClock *iface,
        REFERENCE_TIME base, REFERENCE_TIME offset, HEVENT event, DWORD_PTR *cookie)
{
    ok(0, "unexpected %s\n", __func__);
    return E_NOTIMPL;
}

static HRESULT WINAPI test_sink_latency_clock_AdvisePeriodic(IReferenceClock *iface,
        REFERENCE_TIME start, REFERENCE_TIME period, HSEMAPHORE semaphore, DWORD_PTR *cookie)
{
    ok(0, "unexpected %s\n", __func__);
    return E_NOTIMPL;
}

static HRESULT WINAPI test_sink_latency_clock_Unadvise(IReferenceClock *iface, DWORD_PTR cookie)
{
    ok(0, "unexpected %s\n", __func__);
    return E_NOTIMPL;
}

static const IReferenceClockVtbl test_sink_latency_clock_vtbl =
{
    test_sink_latency_clock_QueryInterface,
    test_sink_latency_clock_AddRef,
    test_sink_latency_clock_Release,
    test_sink_latency_clock_GetTime,
    test_sink_latency_clock_AdviseTime,
    test_sink_latency_clock_AdvisePeriodic,
    test_sink_latency_clock_Unadvise,
};

static HRESULT test_sink_create(IDirectMusicSynthSink **out)
{
    struct test_sink *sink;

    *out = NULL;
    if (!(sink = calloc(1, sizeof(*sink)))) return E_OUTOFMEMORY;
    sink->IDirectMusicSynthSink_iface.lpVtbl = &test_sink_vtbl;
    sink->IReferenceClock_iface.lpVtbl = &test_sink_latency_clock_vtbl;
    sink->refcount = 1;

    *out = &sink->IDirectMusicSynthSink_iface;
    return S_OK;
}

static void test_sink_render(IDirectMusicSynthSink *iface, void *buffer, DWORD buffer_size, HANDLE output)
{
    struct test_sink *sink = CONTAINING_RECORD(iface, struct test_sink, IDirectMusicSynthSink_iface);
    DWORD written, format_size;
    WAVEFORMATEX format;
    HRESULT hr;

    format_size = sizeof(format);
    hr = IDirectMusicSynth_GetFormat(sink->synth, &format, &format_size);
    ok(hr == S_OK, "got %#lx\n", hr);

    memset(buffer, 0, buffer_size);
    hr = IDirectMusicSynth_Render(sink->synth, buffer, buffer_size / format.nBlockAlign, sink->written / format.nBlockAlign);
    ok(hr == S_OK, "got %#lx\n", hr);
    sink->written += buffer_size;

    hr = IDirectMusicSynthSink_SampleToRefTime(iface, sink->written / format.nBlockAlign, &sink->latency_time);
    ok(hr == S_OK, "got %#lx\n", hr);

    if (output)
    {
        BOOL ret = WriteFile(output, buffer, buffer_size, &written, NULL);
        ok(!!ret, "WriteFile failed, error %lu.\n", GetLastError());
    }
}

static BOOL unload_called;

static HRESULT CALLBACK test_unload_callback(HANDLE handle, HANDLE user_data)
{
    ok(!!handle, "got %p\n", handle);
    ok(user_data == (HANDLE)0xdeadbeef, "got %p\n", user_data);
    unload_called = TRUE;
    return E_FAIL;
}

static HRESULT CALLBACK test_unload_no_callback(HANDLE handle, HANDLE user_data)
{
    ok(0, "unexpected %s\n", __func__);
    return E_FAIL;
}

static void test_IDirectMusicSynth(void)
{
    static const UINT RENDER_ITERATIONS = 8;

    struct wave_download
    {
        DMUS_DOWNLOADINFO info;
        ULONG offsets[2];
        DMUS_WAVE wave;
        union
        {
            DMUS_WAVEDATA wave_data;
            struct
            {
                ULONG size;
                BYTE samples[256];
            };
        };
    } wave_download =
    {
        .info =
        {
            .dwDLType = DMUS_DOWNLOADINFO_WAVE,
            .dwDLId = 1,
            .dwNumOffsetTableEntries = 2,
            .cbSize = sizeof(struct wave_download),
        },
        .offsets =
        {
            offsetof(struct wave_download, wave),
            offsetof(struct wave_download, wave_data),
        },
        .wave =
        {
            .ulWaveDataIdx = 1,
            .WaveformatEx =
            {
                .wFormatTag = WAVE_FORMAT_PCM,
                .nChannels = 1,
                .wBitsPerSample = 8,
                .nSamplesPerSec = 44100,
                .nAvgBytesPerSec = 44100,
                .nBlockAlign = 1,
            },
        },
        .wave_data =
        {
            .cbSize = sizeof(wave_download.samples),
        },
    };
    struct instrument_download
    {
        DMUS_DOWNLOADINFO info;
        ULONG offsets[4];
        DMUS_INSTRUMENT instrument;
        DMUS_REGION region;
        DMUS_ARTICULATION articulation;
        DMUS_ARTICPARAMS artic_params;
    } instrument_download =
    {
        .info =
        {
            .dwDLType = DMUS_DOWNLOADINFO_INSTRUMENT,
            .dwDLId = 2,
            .dwNumOffsetTableEntries = 4,
            .cbSize = sizeof(struct instrument_download),
        },
        .offsets =
        {
            offsetof(struct instrument_download, instrument),
            offsetof(struct instrument_download, region),
            offsetof(struct instrument_download, articulation),
            offsetof(struct instrument_download, artic_params),
        },
        .instrument =
        {
            .ulPatch = 0,
            .ulFirstRegionIdx = 1,
            .ulGlobalArtIdx = 2,
        },
        .region =
        {
            .RangeKey = {.usLow = 0, .usHigh = 127},
            .RangeVelocity = {.usLow = 0, .usHigh = 127},
            .fusOptions = F_RGN_OPTION_SELFNONEXCLUSIVE,
            .WaveLink = {.ulChannel = 1, .ulTableIndex = 1},
            .WSMP = {.cbSize = sizeof(WSMPL), .usUnityNote = 60, .fulOptions = F_WSMP_NO_TRUNCATION},
            .WLOOP[0] = {.cbSize = sizeof(WLOOP), .ulType = WLOOP_TYPE_FORWARD},
        },
        .articulation = {.ulArt1Idx = 3},
        .artic_params =
        {
            .VolEG = {.tcAttack = 32768u << 16, .tcDecay = 32768u << 16, .ptSustain = 10000 << 16, .tcRelease = 32768u << 16},
        },
    };
    DMUS_BUFFERDESC buffer_desc =
    {
        .dwSize = sizeof(DMUS_BUFFERDESC),
        .cbBuffer = 4096,
    };
    DMUS_PORTPARAMS port_params =
    {
        .dwSize = sizeof(DMUS_PORTPARAMS),
        .dwValidParams = DMUS_PORTPARAMS_AUDIOCHANNELS | DMUS_PORTPARAMS_SAMPLERATE,
        .dwAudioChannels = 2,
        .dwSampleRate = 44100,
    };
    WCHAR temp_path[MAX_PATH], temp_file[MAX_PATH];
    IReferenceClock *latency_clock;
    IDirectMusicSynthSink *sink;
    IDirectMusicBuffer *buffer;
    DWORD format_size, written;
    IDirectMusicSynth *synth;
    HANDLE wave_file, wave_handle, instrument_handle;
    IReferenceClock *clock;
    BOOL can_free = FALSE;
    REFERENCE_TIME time;
    WAVEFORMATEX format;
    IDirectMusic *music;
    short samples[256];
    ULONG i, ref;
    HRESULT hr;
    DWORD len;
    BYTE *raw;
    BOOL ret;

    hr = CoCreateInstance(&CLSID_DirectMusic, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusic, (void **)&music);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusic_GetMasterClock(music, NULL, &clock);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = test_sink_create(&sink);
    ok(hr == S_OK, "got %#lx\n", hr);


    hr = CoCreateInstance(&CLSID_DirectMusicSynth, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicSynth, (void **)&synth);
    ok(hr == S_OK, "got %#lx\n", hr);

    /* SetNumChannelGroups needs Open */
    hr = IDirectMusicSynth_SetNumChannelGroups(synth, 1);
    todo_wine ok(hr == DMUS_E_SYNTHNOTCONFIGURED, "got %#lx\n", hr);
    /* GetFormat needs Open */
    hr = IDirectMusicSynth_GetFormat(synth, NULL, NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);
    hr = IDirectMusicSynth_GetFormat(synth, NULL, &format_size);
    ok(hr == DMUS_E_SYNTHNOTCONFIGURED, "got %#lx\n", hr);

    /* Open / Close don't need a sink */
    hr = IDirectMusicSynth_Open(synth, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicSynth_Open(synth, NULL);
    ok(hr == DMUS_E_ALREADYOPEN, "got %#lx\n", hr);
    hr = IDirectMusicSynth_SetNumChannelGroups(synth, 1);
    ok(hr == S_OK, "got %#lx\n", hr);
    format_size = sizeof(format);
    hr = IDirectMusicSynth_GetFormat(synth, NULL, &format_size);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(format_size == sizeof(format), "got %lu\n", format_size);
    hr = IDirectMusicSynth_Close(synth);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicSynth_Close(synth);
    ok(hr == DMUS_E_ALREADYCLOSED, "got %#lx\n", hr);

    /* GetLatencyClock needs a sink */
    hr = IDirectMusicSynth_GetLatencyClock(synth, NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);
    hr = IDirectMusicSynth_GetLatencyClock(synth, &latency_clock);
    ok(hr == DMUS_E_NOSYNTHSINK, "got %#lx\n", hr);

    /* Activate needs a sink, synth to be open, and a master clock on the sink */
    hr = IDirectMusicSynth_Open(synth, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicSynth_Activate(synth, TRUE);
    ok(hr == DMUS_E_NOSYNTHSINK, "got %#lx\n", hr);
    hr = IDirectMusicSynth_Activate(synth, FALSE);
    ok(hr == S_FALSE, "got %#lx\n", hr);

    hr = IDirectMusicSynth_SetSynthSink(synth, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicSynth_SetSynthSink(synth, sink);
    ok(hr == S_OK, "got %#lx\n", hr);
    ref = get_refcount(sink);
    ok(ref == 2, "got %lu\n", ref);
    hr = IDirectMusicSynth_Activate(synth, TRUE);
    ok(hr == DMUS_E_SYNTHNOTCONFIGURED, "got %#lx\n", hr);

    /* SetMasterClock does nothing */
    hr = IDirectMusicSynth_SetMasterClock(synth, NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);
    hr = IDirectMusicSynth_SetMasterClock(synth, clock);
    ok(hr == S_OK, "got %#lx\n", hr);
    ref = get_refcount(clock);
    todo_wine ok(ref == 1, "got %lu\n", ref);
    hr = IDirectMusicSynth_Activate(synth, TRUE);
    ok(hr == DMUS_E_SYNTHNOTCONFIGURED, "got %#lx\n", hr);

    /* SetMasterClock needs to be called on the sink */
    hr = IDirectMusicSynthSink_SetMasterClock(sink, clock);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicSynth_Activate(synth, TRUE);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicSynth_Activate(synth, TRUE);
    ok(hr == S_FALSE, "got %#lx\n", hr);

    /* Close is fine while active */
    hr = IDirectMusicSynth_Close(synth);
    ok(hr == S_OK, "got %#lx\n", hr);
    /* Removing the sink is fine while active */
    hr = IDirectMusicSynth_SetSynthSink(synth, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    ref = get_refcount(sink);
    ok(ref == 1, "got %lu\n", ref);

    /* but Activate might fail then */
    hr = IDirectMusicSynth_Activate(synth, FALSE);
    ok(hr == DMUS_E_SYNTHNOTCONFIGURED, "got %#lx\n", hr);
    hr = IDirectMusicSynth_Activate(synth, FALSE);
    ok(hr == S_FALSE, "got %#lx\n", hr);


    /* Test generating some samples */
    hr = IDirectMusicSynth_Open(synth, &port_params);
    ok(hr == S_OK, "got %#lx\n", hr);

    format_size = sizeof(format);
    hr = IDirectMusicSynth_GetFormat(synth, &format, &format_size);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicSynth_SetSynthSink(synth, sink);
    ok(hr == S_OK, "got %#lx\n", hr);
    ref = get_refcount(sink);
    ok(ref == 2, "got %lu\n", ref);
    hr = IDirectMusicSynth_Activate(synth, TRUE);
    ok(hr == S_OK, "got %#lx\n", hr);

    GetTempPathW(MAX_PATH, temp_path);
    GetTempFileNameW(temp_path, L"synth", 0, temp_file);
    wave_file = CreateFileW(temp_file, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(wave_file != INVALID_HANDLE_VALUE, "CreateFileW failed, error %lu.\n", GetLastError());
    ret = WriteFile(wave_file, "RIFF", 4, &written, NULL);
    ok(ret, "WriteFile failed, error %lu.\n", GetLastError());
    format_size = (RENDER_ITERATIONS + 1) * sizeof(samples) + sizeof(format) + 20;
    ret = WriteFile(wave_file, &format_size, 4, &written, NULL);
    ok(ret, "WriteFile failed, error %lu.\n", GetLastError());
    ret = WriteFile(wave_file, "WAVEfmt ", 8, &written, NULL);
    ok(ret, "WriteFile failed, error %lu.\n", GetLastError());
    format_size = sizeof(format);
    ret = WriteFile(wave_file, &format_size, 4, &written, NULL);
    ok(ret, "WriteFile failed, error %lu.\n", GetLastError());
    ret = WriteFile(wave_file, &format, format_size, &written, NULL);
    ok(ret, "WriteFile failed, error %lu.\n", GetLastError());
    ret = WriteFile(wave_file, "data", 4, &written, NULL);
    ok(ret, "WriteFile failed, error %lu.\n", GetLastError());
    format_size = (RENDER_ITERATIONS + 1) * sizeof(samples);
    ret = WriteFile(wave_file, &format_size, 4, &written, NULL);
    ok(ret, "WriteFile failed, error %lu.\n", GetLastError());

    /* native needs to render at least once before producing samples */
    test_sink_render(sink, samples, sizeof(samples), wave_file);

    for (i = 0; i < ARRAY_SIZE(wave_download.samples); i++)
        wave_download.samples[i] = i;

    can_free = 0xdeadbeef;
    wave_handle = NULL;
    hr = IDirectMusicSynth_Download(synth, &wave_handle, &wave_download, &can_free);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(!!wave_handle, "got %p\n", wave_handle);
    todo_wine ok(can_free == FALSE, "got %u\n", can_free);

    can_free = 0xdeadbeef;
    instrument_handle = NULL;
    hr = IDirectMusicSynth_Download(synth, &instrument_handle, &instrument_download, &can_free);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(!!instrument_handle, "got %p\n", instrument_handle);
    ok(can_free == TRUE, "got %u\n", can_free);

    /* add a MIDI note to a buffer and play it */
    hr = IDirectMusicSynth_GetLatencyClock(synth, &latency_clock);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusic_CreateMusicBuffer(music, &buffer_desc, &buffer, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    /* status = 0x90 (NOTEON / channel 0), key = 0x27 (39), vel = 0x78 (120) */
    hr = IDirectMusicBuffer_PackStructured(buffer, 0, 1, 0x782790);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IReferenceClock_GetTime(latency_clock, &time);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicBuffer_GetRawBufferPtr(buffer, &raw);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicBuffer_GetUsedBytes(buffer, &len);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicSynth_PlayBuffer(synth, time, raw, len);
    ok(hr == S_OK, "got %#lx\n", hr);
    IDirectMusicBuffer_Release(buffer);
    IReferenceClock_Release(latency_clock);

    for (i = 0; i < RENDER_ITERATIONS; i++)
        test_sink_render(sink, samples, sizeof(samples), wave_file);

    CloseHandle(wave_file);
    trace("Rendered samples to %s\n", debugstr_w(temp_file));


    hr = IDirectMusicSynth_Activate(synth, FALSE);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicSynth_SetSynthSink(synth, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    ref = get_refcount(sink);
    ok(ref == 1, "got %lu\n", ref);

    hr = IDirectMusicSynth_Unload(synth, 0, NULL, NULL);
    ok(hr == E_FAIL, "got %#lx\n", hr);
    hr = IDirectMusicSynth_Unload(synth, (HANDLE)0xdeadbeef, test_unload_no_callback, (HANDLE)0xdeadbeef);
    ok(hr == E_FAIL, "got %#lx\n", hr);
    hr = IDirectMusicSynth_Unload(synth, wave_handle, test_unload_callback, (HANDLE)0xdeadbeef);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(!unload_called, "callback called\n");
    hr = IDirectMusicSynth_Unload(synth, instrument_handle, NULL, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    todo_wine ok(unload_called, "callback not called\n");

    hr = IDirectMusicSynth_Close(synth);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicSynth_Unload(synth, 0, NULL, NULL);
    todo_wine ok(hr == DMUS_E_SYNTHNOTCONFIGURED, "got %#lx\n", hr);


    IDirectMusicSynth_Release(synth);


    IDirectMusicSynthSink_Release(sink);
    IReferenceClock_Release(clock);
    IDirectMusic_Release(music);
}

static void test_IDirectMusicSynthSink(void)
{
    IReferenceClock *latency_clock;
    REFERENCE_TIME time, tmp_time;
    IDirectMusicSynthSink *sink;
    IDirectMusicSynth8 *synth;
    IReferenceClock *clock;
    IDirectSound *dsound;
    IDirectMusic *music;
    LONGLONG sample;
    HRESULT hr;
    DWORD size;
    ULONG ref;

    hr = DirectSoundCreate(NULL, &dsound, NULL);
    ok(hr == S_OK || broken(hr == DSERR_NODRIVER), "got %#lx\n", hr);
    if (broken(hr == DSERR_NODRIVER))
    {
        win_skip("Failed to create IDirectSound, skipping tests\n");
        return;
    }

    hr = IDirectSound_SetCooperativeLevel(dsound, GetDesktopWindow(), DSSCL_PRIORITY);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = CoCreateInstance(&CLSID_DirectMusic, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusic, (void **)&music);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusic_GetMasterClock(music, NULL, &clock);
    ok(hr == S_OK, "got %#lx\n", hr);
    IDirectMusic_Release(music);

    hr = test_synth_create(&synth);
    ok(hr == S_OK, "got %#lx\n", hr);


    hr = CoCreateInstance(&CLSID_DirectMusicSynthSink, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicSynthSink, (void **)&sink);
    ok(hr == S_OK, "got %#lx\n", hr);

    /* sink is not configured */
    hr = IDirectMusicSynthSink_Activate(sink, TRUE);
    ok(hr == DMUS_E_SYNTHNOTCONFIGURED, "got %#lx\n", hr);
    hr = IDirectMusicSynthSink_Activate(sink, FALSE);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicSynthSink_SampleToRefTime(sink, 0, NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);
    time = 0xdeadbeef;
    hr = IDirectMusicSynthSink_SampleToRefTime(sink, 10, &time);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(time == 4000, "got %I64d\n", time);

    hr = IDirectMusicSynthSink_RefTimeToSample(sink, 0, NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);
    sample = 0xdeadbeef;
    hr = IDirectMusicSynthSink_RefTimeToSample(sink, 4000, &sample);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(sample == 8, "got %I64d\n", sample);

    hr = IDirectMusicSynthSink_GetDesiredBufferSize(sink, &size);
    ok(hr == DMUS_E_SYNTHNOTCONFIGURED, "got %#lx\n", hr);

    /* latency clock is available but not usable */
    hr = IDirectMusicSynthSink_GetLatencyClock(sink, NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);
    ref = get_refcount(sink);
    ok(ref == 1, "got %#lx\n", ref);
    hr = IDirectMusicSynthSink_GetLatencyClock(sink, &latency_clock);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(latency_clock != clock, "got same clock\n");
    ref = get_refcount(sink);
    ok(ref == 2, "got %#lx\n", ref);

    hr = IReferenceClock_GetTime(latency_clock, NULL);
    ok(hr == E_INVALIDARG, "got %#lx\n", hr);
    hr = IReferenceClock_GetTime(latency_clock, &time);
    ok(hr == E_FAIL, "got %#lx\n", hr);

    hr = IDirectMusicSynthSink_Init(sink, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicSynthSink_SetDirectSound(sink, NULL, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicSynthSink_SetDirectSound(sink, dsound, NULL);
    ok(hr == DMUS_E_SYNTHNOTCONFIGURED, "got %#lx\n", hr);

    /* Activate requires a synth, dsound and a clock */
    ref = get_refcount(synth);
    ok(ref == 1, "got %#lx\n", ref);
    hr = IDirectMusicSynthSink_Init(sink, (IDirectMusicSynth *)synth);
    ok(hr == S_OK, "got %#lx\n", hr);
    ref = get_refcount(synth);
    ok(ref == 1, "got %#lx\n", ref);
    hr = IDirectMusicSynthSink_GetDesiredBufferSize(sink, &size);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(size == 352800, "got %lu\n", size);
    hr = IDirectMusicSynthSink_Activate(sink, TRUE);
    ok(hr == DMUS_E_DSOUND_NOT_SET, "got %#lx\n", hr);
    hr = IDirectMusicSynthSink_SetDirectSound(sink, dsound, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicSynthSink_Activate(sink, TRUE);
    ok(hr == DMUS_E_NO_MASTER_CLOCK, "got %#lx\n", hr);
    hr = IDirectMusicSynthSink_SetMasterClock(sink, clock);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IReferenceClock_GetTime(latency_clock, &time);
    ok(hr == E_FAIL, "got %#lx\n", hr);
    hr = IDirectMusicSynthSink_Activate(sink, TRUE);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicSynthSink_Activate(sink, TRUE);
    ok(hr == DMUS_E_SYNTHACTIVE, "got %#lx\n", hr);
    ref = get_refcount(synth);
    todo_wine ok(ref == 1, "got %#lx\n", ref);

    hr = IDirectMusicSynthSink_GetDesiredBufferSize(sink, &size);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(size == 352800, "got %lu\n", size);

    /* conversion functions now use the activation time and master clock */
    hr = IReferenceClock_GetTime(clock, &time);
    ok(hr == S_OK, "got %#lx\n", hr);
    sample = 0xdeadbeef;
    hr = IDirectMusicSynthSink_RefTimeToSample(sink, time, &sample);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(sample <= 3000, "got %I64d\n", sample);
    tmp_time = time + 1;
    hr = IDirectMusicSynthSink_SampleToRefTime(sink, sample, &tmp_time);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(tmp_time <= time, "got %I64d\n", tmp_time - time);
    ok(time - tmp_time <= 5000, "got %I64d\n", tmp_time);

    /* latency clock now works fine */
    tmp_time = time;
    hr = IReferenceClock_GetTime(latency_clock, &tmp_time);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(tmp_time > time, "got %I64d\n", tmp_time - time);
    ok(tmp_time - time <= 2000000, "got %I64d\n", tmp_time - time);

    /* setting the clock while active is fine */
    hr = IDirectMusicSynthSink_SetMasterClock(sink, clock);
    ok(hr == S_OK, "got %#lx\n", hr);

    /* removing synth while active is fine */
    hr = IDirectMusicSynthSink_Init(sink, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    ref = get_refcount(synth);
    todo_wine ok(ref == 1, "got %#lx\n", ref);
    hr = IDirectMusicSynthSink_Activate(sink, TRUE);
    ok(hr == DMUS_E_SYNTHNOTCONFIGURED, "got %#lx\n", hr);

    /* changing dsound while active fails */
    hr = IDirectMusicSynthSink_SetDirectSound(sink, dsound, NULL);
    ok(hr == DMUS_E_SYNTHACTIVE, "got %#lx\n", hr);
    hr = IDirectMusicSynthSink_SetDirectSound(sink, NULL, NULL);
    ok(hr == DMUS_E_SYNTHACTIVE, "got %#lx\n", hr);
    hr = IDirectMusicSynthSink_Activate(sink, FALSE);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicSynthSink_SetDirectSound(sink, NULL, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);

    /* SetMasterClock doesn't need the sink to be configured */
    hr = IDirectMusicSynthSink_SetMasterClock(sink, NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);
    hr = IDirectMusicSynthSink_SetMasterClock(sink, clock);
    ok(hr == S_OK, "got %#lx\n", hr);

    IReferenceClock_Release(latency_clock);
    IDirectMusicSynthSink_Release(sink);


    IDirectMusicSynth8_Release(synth);
    IReferenceClock_Release(clock);

    IDirectSound_Release(dsound);
}

START_TEST(dmsynth)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (missing_dmsynth())
    {
        skip("dmsynth not available\n");
        CoUninitialize();
        return;
    }
    test_dmsynth();
    test_COM();
    test_COM_synthsink();
    test_IDirectMusicSynth();
    test_IDirectMusicSynthSink();

    CoUninitialize();
}
