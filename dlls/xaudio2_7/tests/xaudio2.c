/*
 * Copyright (c) 2015 Andrew Eikum for CodeWeavers
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

#include <windows.h>
#include <math.h>
#include <stdbool.h>

#define COBJMACROS
#include "wine/test.h"
#include "initguid.h"
/* Don't include xaudio2.h directly; it's generated from an IDL with ifdefs and
 * hence is frozen at version 2.7. Instead include that IDL in a local IDL and
 * include the generated header.
 *
 * Because shared sources are compiled from the C file in the xaudio2_7
 * directory, we need to use angle brackets here to prevent the compiler from
 * picking up xaudio_classes.h from that directory for other versions. */
#include <xaudio_classes.h>
#include "xapo.h"
#include "xapofx.h"
#include "mmsystem.h"
#include "ks.h"
#include "ksmedia.h"

static const GUID IID_IXAudio27 =            {0x8bcf1f58, 0x9fe7, 0x4583, {0x8a, 0xc6, 0xe2, 0xad, 0xc4, 0x65, 0xc8, 0xbb}};
static const GUID IID_IXAudio28 =            {0x60d8dac8, 0x5aa1, 0x4e8e, {0xb5, 0x97, 0x2f, 0x5e, 0x28, 0x83, 0xd4, 0x84}};
static const GUID IID_IXAudio29 =            {0x2b02e3cf, 0x2e0b, 0x4ec3, {0xbe, 0x45, 0x1b, 0x2a, 0x3f, 0xe7, 0x21, 0x0d}};

static const GUID CLSID_AudioVolumeMeter20 = {0xc0c56f46, 0x29b1, 0x44e9, {0x99, 0x39, 0xa3, 0x2c, 0xe8, 0x68, 0x67, 0xe2}};
static const GUID CLSID_AudioVolumeMeter21 = {0xc1e3f122, 0xa2ea, 0x442c, {0x85, 0x4f, 0x20, 0xd9, 0x8f, 0x83, 0x57, 0xa1}};
static const GUID CLSID_AudioVolumeMeter22 = {0xf5ca7b34, 0x8055, 0x42c0, {0xb8, 0x36, 0x21, 0x61, 0x29, 0xeb, 0x7e, 0x30}};
static const GUID CLSID_AudioVolumeMeter23 = {0xe180344b, 0xac83, 0x4483, {0x95, 0x9e, 0x18, 0xa5, 0xc5, 0x6a, 0x5e, 0x19}};
static const GUID CLSID_AudioVolumeMeter24 = {0xc7338b95, 0x52b8, 0x4542, {0xaa, 0x79, 0x42, 0xeb, 0x01, 0x6c, 0x8c, 0x1c}};
static const GUID CLSID_AudioVolumeMeter25 = {0x2139e6da, 0xc341, 0x4774, {0x9a, 0xc3, 0xb4, 0xe0, 0x26, 0x34, 0x7f, 0x64}};
static const GUID CLSID_AudioVolumeMeter26 = {0xe48c5a3f, 0x93ef, 0x43bb, {0xa0, 0x92, 0x2c, 0x7c, 0xeb, 0x94, 0x6f, 0x27}};
static const GUID CLSID_AudioVolumeMeter27 = {0xcac1105f, 0x619b, 0x4d04, {0x83, 0x1a, 0x44, 0xe1, 0xcb, 0xf1, 0x2d, 0x57}};
static const GUID CLSID_AudioReverb20 =      {0x6f6ea3a9, 0x2cf5, 0x41cf, {0x91, 0xc1, 0x21, 0x70, 0xb1, 0x54, 0x00, 0x63}};
static const GUID CLSID_AudioReverb21 =      {0xf4769300, 0xb949, 0x4df9, {0xb3, 0x33, 0x00, 0xd3, 0x39, 0x32, 0xe9, 0xa6}};
static const GUID CLSID_AudioReverb22 =      {0x629cf0de, 0x3ecc, 0x41e7, {0x99, 0x26, 0xf7, 0xe4, 0x3e, 0xeb, 0xec, 0x51}};
static const GUID CLSID_AudioReverb23 =      {0x9cab402c, 0x1d37, 0x44b4, {0x88, 0x6d, 0xfa, 0x4f, 0x36, 0x17, 0x0a, 0x4c}};
static const GUID CLSID_AudioReverb24 =      {0x8bb7778b, 0x645b, 0x4475, {0x9a, 0x73, 0x1d, 0xe3, 0x17, 0x0b, 0xd3, 0xaf}};
static const GUID CLSID_AudioReverb25 =      {0xd06df0d0, 0x8518, 0x441e, {0x82, 0x2f, 0x54, 0x51, 0xd5, 0xc5, 0x95, 0xb8}};
static const GUID CLSID_AudioReverb26 =      {0xcecec95a, 0xd894, 0x491a, {0xbe, 0xe3, 0x5e, 0x10, 0x6f, 0xb5, 0x9f, 0x2d}};
static const GUID CLSID_AudioReverb27 =      {0x6a93130e, 0x1d53, 0x41d1, {0xa9, 0xcf, 0xe7, 0x58, 0x80, 0x0b, 0xb1, 0x79}};

static const bool xaudio27 = (XAUDIO2_VER <= 7);

static IXAudio2 *create_xaudio2(void)
{
    IXAudio2 *audio;
    HRESULT hr;

#if XAUDIO2_VER <= 7
    hr = CoCreateInstance(&CLSID_XAudio2, NULL, CLSCTX_INPROC_SERVER, &IID_IXAudio2, (void **)&audio);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("XAudio 2.7 is not available\n");
        return NULL;
    }
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IXAudio2_Initialize(audio, 0, XAUDIO2_ANY_PROCESSOR);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
#else
    hr = XAudio2Create(&audio, 0, XAUDIO2_DEFAULT_PROCESSOR);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
#endif

    return audio;
}

#define check_refcount(a, b) check_refcount_(__LINE__, a, b)
static void check_refcount_(int line, IUnknown *unk, LONG expected)
{
    LONG ref;

    IUnknown_AddRef(unk);
    ref = IUnknown_Release(unk);
    ok_(__FILE__, line)(ref == expected, "got refcount %ld, expected %ld.\n", ref, expected);
}

static void test_interfaces(IXAudio2 *audio)
{
    const GUID *td[] =
    {
        &IID_IXAudio27, /* DirectX SDK */
        &IID_IXAudio28, /* Windows 8 */
        &IID_IXAudio29, /* Windows 10 */
    };
    UINT match_count = 0;
    INT last_match = -1;
    IXAudio2 *audio2;
    HRESULT hr;
    UINT i;

    for (i = 0; i < ARRAY_SIZE(td); i++)
    {
        hr = IXAudio2_QueryInterface(audio, td[i], (void **)&audio2);
        ok(hr == S_OK || hr == E_NOINTERFACE, "%u: Got hr %#lx.\n", i, hr);
        if (hr == S_OK)
        {
            IXAudio2_Release(audio2);
            ++match_count;
            last_match = i;
        }
    }
    ok(match_count == 1, "Found %u matching interfaces.\n", match_count);
#if XAUDIO2_VER <= 7
    ok(last_match < 1, "Unexpectedly matched interface (%d).\n", last_match);
#else
    ok(last_match != 0, "Unexpectedly matched interface (%d).\n", last_match);
#endif
}

static HRESULT create_mastering_voice(IXAudio2 *audio, unsigned int channel_count, IXAudio2MasteringVoice **voice)
{
#if XAUDIO2_VER <= 7
    return IXAudio2_CreateMasteringVoice(audio, voice, channel_count, 44100, 0, 0, NULL);
#else
    return IXAudio2_CreateMasteringVoice(audio, voice, channel_count, 44100, 0, NULL, NULL, AudioCategory_GameEffects);
#endif
}

static void get_voice_state(IXAudio2SourceVoice *voice, XAUDIO2_VOICE_STATE *state)
{
#if XAUDIO2_VER <= 7
    IXAudio2SourceVoice_GetState(voice, state);
#else
    IXAudio2SourceVoice_GetState(voice, state, 0);
#endif
}

static void fill_buf(float *buf, WAVEFORMATEX *fmt, DWORD hz, DWORD len_frames)
{
    if(winetest_interactive){
        DWORD offs, c;
        for(offs = 0; offs < len_frames; ++offs)
            for(c = 0; c < fmt->nChannels; ++c)
                buf[offs * fmt->nChannels + c] = sinf(offs * hz * 2 * M_PI / fmt->nSamplesPerSec);
    }else
        memset(buf, 0, sizeof(float) * len_frames * fmt->nChannels);
}

static struct _cb_state {
    BOOL start_called, end_called;
} ecb_state, src1_state, src2_state;

static int pass_state = 0;
static BOOL buffers_called = FALSE;

static void WINAPI ECB_OnProcessingPassStart(IXAudio2EngineCallback *This)
{
    ok(!xaudio27 || pass_state == 0, "Callbacks called out of order: %u\n", pass_state);
    ++pass_state;
}

static void WINAPI ECB_OnProcessingPassEnd(IXAudio2EngineCallback *This)
{
    flaky
    ok(!xaudio27 || pass_state == (buffers_called ? 7 : 5), "Callbacks called out of order: %u\n", pass_state);
    pass_state = 0;
    buffers_called = FALSE;
}

static void WINAPI ECB_OnCriticalError(IXAudio2EngineCallback *This, HRESULT Error)
{
    ok(0, "Unexpected OnCriticalError\n");
}

static const IXAudio2EngineCallbackVtbl ecb_vtbl = {
    ECB_OnProcessingPassStart,
    ECB_OnProcessingPassEnd,
    ECB_OnCriticalError
};

static IXAudio2EngineCallback ecb = { &ecb_vtbl };

static IXAudio2VoiceCallback vcb1;
static IXAudio2VoiceCallback vcb2;

static void WINAPI VCB_OnVoiceProcessingPassStart(IXAudio2VoiceCallback *This,
        UINT32 BytesRequired)
{
    if(This == &vcb1){
        ok(!xaudio27 || pass_state == (buffers_called ? 4 : 3), "Callbacks called out of order: %u\n", pass_state);
        ++pass_state;
    }else{
        ok(!xaudio27 || pass_state == 1, "Callbacks called out of order: %u\n", pass_state);
        ++pass_state;
    }
}

static void WINAPI VCB_OnVoiceProcessingPassEnd(IXAudio2VoiceCallback *This)
{
    if(This == &vcb1){
        ok(!xaudio27 || pass_state == (buffers_called ? 6 : 4), "Callbacks called out of order: %u\n", pass_state);
        ++pass_state;
    }else{
        ok(!xaudio27 || pass_state == (buffers_called ? 3 : 2), "Callbacks called out of order: %u\n", pass_state);
        ++pass_state;
    }
}

static void WINAPI VCB_OnStreamEnd(IXAudio2VoiceCallback *This)
{
    ok(0, "Unexpected OnStreamEnd\n");
}

static void WINAPI VCB_OnBufferStart(IXAudio2VoiceCallback *This,
        void *pBufferContext)
{
    if(This == &vcb1){
        ok(!xaudio27 || pass_state == 5, "Callbacks called out of order: %u\n", pass_state);
        ++pass_state;
    }else{
        ok(!xaudio27 || pass_state == 2, "Callbacks called out of order: %u\n", pass_state);
        ++pass_state;
        buffers_called = TRUE;
    }
}

static void WINAPI VCB_OnBufferEnd(IXAudio2VoiceCallback *This,
        void *pBufferContext)
{
    if(This == &vcb1){
        ok(!xaudio27 || pass_state == 5, "Callbacks called out of order: %u\n", pass_state);
        ++pass_state;
    }else{
        ok(!xaudio27 || pass_state == 2, "Callbacks called out of order: %u\n", pass_state);
        ++pass_state;
        buffers_called = TRUE;
    }
}

static void WINAPI VCB_OnLoopEnd(IXAudio2VoiceCallback *This,
        void *pBufferContext)
{
    ok(0, "Unexpected OnLoopEnd\n");
}

static void WINAPI VCB_OnVoiceError(IXAudio2VoiceCallback *This,
        void *pBuffercontext, HRESULT Error)
{
    ok(0, "Unexpected OnVoiceError\n");
}

static const IXAudio2VoiceCallbackVtbl vcb_vtbl = {
    VCB_OnVoiceProcessingPassStart,
    VCB_OnVoiceProcessingPassEnd,
    VCB_OnStreamEnd,
    VCB_OnBufferStart,
    VCB_OnBufferEnd,
    VCB_OnLoopEnd,
    VCB_OnVoiceError
};

static IXAudio2VoiceCallback vcb1 = { &vcb_vtbl };
static IXAudio2VoiceCallback vcb2 = { &vcb_vtbl };

static void test_simple_streaming(IXAudio2 *xa)
{
    HRESULT hr;
    IXAudio2MasteringVoice *master;
    IXAudio2SourceVoice *src, *src2;
    IUnknown *vumeter;
    WAVEFORMATEX fmt;
    XAUDIO2_BUFFER buf, buf2;
    XAUDIO2_VOICE_DETAILS details;
    XAUDIO2_VOICE_STATE state;
    XAUDIO2_EFFECT_DESCRIPTOR effect;
    XAUDIO2_EFFECT_CHAIN chain;
    IXAPO *xapo;

    memset(&ecb_state, 0, sizeof(ecb_state));
    memset(&src1_state, 0, sizeof(src1_state));
    memset(&src2_state, 0, sizeof(src2_state));

    IXAudio2_StopEngine(xa);

    /* Tests show in native XA2.8, ECB is called from a mixer thread, but VCBs
     * may be called from other threads in any order. So we can't rely on any
     * sequencing between VCB calls.
     *
     * XA2.7 does all mixing from a single thread, so call sequence can be
     * tested. */
    hr = IXAudio2_RegisterForCallbacks(xa, &ecb);
    ok(hr == S_OK, "RegisterForCallbacks failed: %08lx\n", hr);

    hr = create_mastering_voice(xa, 2, &master);
    ok(hr == S_OK, "CreateMasteringVoice failed: %08lx\n", hr);

#if XAUDIO2_VER >= 8
    {
        DWORD chmask = 0xdeadbeef;

        IXAudio2MasteringVoice_GetChannelMask(master, &chmask);
        ok(chmask == (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT), "Got unexpected channel mask: 0x%lx\n", chmask);
    }
#endif

    /* create first source voice */
    fmt.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    fmt.nChannels = 2;
    fmt.nSamplesPerSec = 44100;
    fmt.wBitsPerSample = 32;
    fmt.nBlockAlign = fmt.nChannels * fmt.wBitsPerSample / 8;
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
    fmt.cbSize = 0;

    hr = IXAudio2_CreateSourceVoice(xa, &src, &fmt, 0, 1.f, &vcb1, NULL, NULL);
    ok(hr == S_OK, "CreateSourceVoice failed: %08lx\n", hr);

    IXAudio2SourceVoice_GetVoiceDetails(src, &details);
    ok(details.CreationFlags == 0, "Got wrong creation flags: 0x%x\n", details.CreationFlags);
#if XAUDIO2_VER >= 8
    ok(details.ActiveFlags == 0, "Got wrong active flags: 0x%x\n", details.ActiveFlags);
#endif
    ok(details.InputChannels == 2, "Got wrong channel count: 0x%x\n", details.InputChannels);
    ok(details.InputSampleRate == 44100, "Got wrong sample rate: 0x%x\n", details.InputSampleRate);

    memset(&buf, 0, sizeof(buf));
    buf.AudioBytes = 22050 * fmt.nBlockAlign;
    buf.pAudioData = HeapAlloc(GetProcessHeap(), 0, buf.AudioBytes);
    fill_buf((float*)buf.pAudioData, &fmt, 440, 22050);

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08lx\n", hr);

    hr = IXAudio2SourceVoice_Start(src, 0, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "Start failed: %08lx\n", hr);

    /* create second source voice */
    hr = IXAudio2_CreateSourceVoice(xa, &src2, &fmt, 0, 1.f, &vcb2, NULL, NULL);
    ok(hr == S_OK, "CreateSourceVoice failed: %08lx\n", hr);

    IXAudio2SourceVoice_GetVoiceDetails(src2, &details);
    ok(details.CreationFlags == 0, "Got wrong creation flags: 0x%x\n", details.CreationFlags);
#if XAUDIO2_VER >= 8
    ok(details.ActiveFlags == 0, "Got wrong active flags: 0x%x\n", details.ActiveFlags);
#endif
    ok(details.InputChannels == 2, "Got wrong channel count: 0x%x\n", details.InputChannels);
    ok(details.InputSampleRate == 44100, "Got wrong sample rate: 0x%x\n", details.InputSampleRate);

    memset(&buf2, 0, sizeof(buf2));
    buf2.AudioBytes = 22050 * fmt.nBlockAlign;
    buf2.pAudioData = HeapAlloc(GetProcessHeap(), 0, buf2.AudioBytes);
    fill_buf((float*)buf2.pAudioData, &fmt, 220, 22050);

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src2, &buf2, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08lx\n", hr);

    hr = IXAudio2SourceVoice_Start(src2, 0, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "Start failed: %08lx\n", hr);

    hr = IXAudio2_StartEngine(xa);
    ok(hr == S_OK, "StartEngine failed: %08lx\n", hr);

    /* hook up volume meter */
#if XAUDIO2_VER <= 7
    hr = CoCreateInstance(&CLSID_AudioVolumeMeter, NULL,
            CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&vumeter);
#else
    hr = CreateAudioVolumeMeter(&vumeter);
#endif
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IUnknown_QueryInterface(vumeter, xaudio27 ? &IID_IXAPO27 : &IID_IXAPO, (void **)&xapo);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IXAPO_Release(xapo);

    effect.InitialState = TRUE;
    effect.OutputChannels = 2;
    effect.pEffect = vumeter;

    chain.EffectCount = 1;
    chain.pEffectDescriptors = &effect;

    hr = IXAudio2MasteringVoice_SetEffectChain(master, &chain);
    ok(hr == S_OK, "SetEffectchain failed: %08lx\n", hr);

    IUnknown_Release(vumeter);

    while(1){
        get_voice_state(src, &state);
        if(state.SamplesPlayed >= 22050)
            break;
        Sleep(100);
    }

    ok(state.SamplesPlayed == 22050, "Got wrong samples played\n");

    HeapFree(GetProcessHeap(), 0, (void*)buf.pAudioData);
    HeapFree(GetProcessHeap(), 0, (void*)buf2.pAudioData);

    IXAudio2SourceVoice_DestroyVoice(src);
    IXAudio2SourceVoice_DestroyVoice(src2);
    IXAudio2MasteringVoice_DestroyVoice(master);

    IXAudio2_UnregisterForCallbacks(xa, &ecb);
}

static void WINAPI vcb_buf_OnVoiceProcessingPassStart(IXAudio2VoiceCallback *This,
        UINT32 BytesRequired)
{
}

static void WINAPI vcb_buf_OnVoiceProcessingPassEnd(IXAudio2VoiceCallback *This)
{
}

static void WINAPI vcb_buf_OnStreamEnd(IXAudio2VoiceCallback *This)
{
    ok(0, "Unexpected OnStreamEnd\n");
}

struct vcb_buf_testdata {
    int idx;
    IXAudio2SourceVoice *src;
};

static int obs_calls = 0;
static int obe_calls = 0;

static void WINAPI vcb_buf_OnBufferStart(IXAudio2VoiceCallback *This,
        void *pBufferContext)
{
    struct vcb_buf_testdata *data = pBufferContext;
    XAUDIO2_VOICE_STATE state;

    ok(data->idx == obs_calls, "Buffer callback out of order: %u\n", data->idx);

    get_voice_state(data->src, &state);

    ok(state.BuffersQueued == 5 - obs_calls, "Got wrong number of buffers remaining: %u\n", state.BuffersQueued);
    ok(state.pCurrentBufferContext == pBufferContext, "Got wrong buffer from GetState\n");

    ++obs_calls;
}

static void WINAPI vcb_buf_OnBufferEnd(IXAudio2VoiceCallback *This,
        void *pBufferContext)
{
    struct vcb_buf_testdata *data = pBufferContext;
    XAUDIO2_VOICE_STATE state;

    ok(data->idx == obe_calls, "Buffer callback out of order: %u\n", data->idx);

    get_voice_state(data->src, &state);

    ok(state.BuffersQueued == 5 - obe_calls - 1, "Got wrong number of buffers remaining: %u\n", state.BuffersQueued);
    if(state.BuffersQueued == 0)
        ok(state.pCurrentBufferContext == NULL, "Got wrong buffer from GetState: %p\n", state.pCurrentBufferContext);
    else
        ok(state.pCurrentBufferContext == data + 1, "Got wrong buffer from GetState: %p\n", state.pCurrentBufferContext);

    ++obe_calls;
}

static void WINAPI vcb_buf_OnLoopEnd(IXAudio2VoiceCallback *This,
        void *pBufferContext)
{
    ok(0, "Unexpected OnLoopEnd\n");
}

static void WINAPI vcb_buf_OnVoiceError(IXAudio2VoiceCallback *This,
        void *pBuffercontext, HRESULT Error)
{
    ok(0, "Unexpected OnVoiceError\n");
}

static const IXAudio2VoiceCallbackVtbl vcb_buf_vtbl = {
    vcb_buf_OnVoiceProcessingPassStart,
    vcb_buf_OnVoiceProcessingPassEnd,
    vcb_buf_OnStreamEnd,
    vcb_buf_OnBufferStart,
    vcb_buf_OnBufferEnd,
    vcb_buf_OnLoopEnd,
    vcb_buf_OnVoiceError
};

static IXAudio2VoiceCallback vcb_buf = { &vcb_buf_vtbl };

static int nloopends = 0;
static int nstreamends = 0;

static void WINAPI loop_buf_OnStreamEnd(IXAudio2VoiceCallback *This)
{
    ++nstreamends;
}

static void WINAPI loop_buf_OnBufferStart(IXAudio2VoiceCallback *This,
        void *pBufferContext)
{
}

static void WINAPI loop_buf_OnBufferEnd(IXAudio2VoiceCallback *This,
        void *pBufferContext)
{
}

static void WINAPI loop_buf_OnLoopEnd(IXAudio2VoiceCallback *This,
        void *pBufferContext)
{
    ++nloopends;
}

static void WINAPI loop_buf_OnVoiceError(IXAudio2VoiceCallback *This,
        void *pBuffercontext, HRESULT Error)
{
}

static const IXAudio2VoiceCallbackVtbl loop_buf_vtbl = {
    vcb_buf_OnVoiceProcessingPassStart,
    vcb_buf_OnVoiceProcessingPassEnd,
    loop_buf_OnStreamEnd,
    loop_buf_OnBufferStart,
    loop_buf_OnBufferEnd,
    loop_buf_OnLoopEnd,
    loop_buf_OnVoiceError
};

static IXAudio2VoiceCallback loop_buf = { &loop_buf_vtbl };

static void test_buffer_callbacks(IXAudio2 *xa)
{
    HRESULT hr;
    IXAudio2MasteringVoice *master;
    IXAudio2SourceVoice *src;
    WAVEFORMATEX fmt;
    XAUDIO2_BUFFER buf;
    XAUDIO2_VOICE_STATE state;
    struct vcb_buf_testdata testdata[5];
    int i, timeout;

    obs_calls = 0;
    obe_calls = 0;

    IXAudio2_StopEngine(xa);

    hr = create_mastering_voice(xa, 2, &master);
    ok(hr == S_OK, "CreateMasteringVoice failed: %08lx\n", hr);

    /* test OnBufferStart/End callbacks */
    fmt.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    fmt.nChannels = 2;
    fmt.nSamplesPerSec = 44100;
    fmt.wBitsPerSample = 32;
    fmt.nBlockAlign = fmt.nChannels * fmt.wBitsPerSample / 8;
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
    fmt.cbSize = 0;

    hr = IXAudio2_CreateSourceVoice(xa, &src, &fmt, 0, 1.f, &vcb_buf, NULL, NULL);
    ok(hr == S_OK, "CreateSourceVoice failed: %08lx\n", hr);

    memset(&buf, 0, sizeof(buf));
    buf.AudioBytes = 4410 * fmt.nBlockAlign;
    buf.pAudioData = HeapAlloc(GetProcessHeap(), 0, buf.AudioBytes);
    fill_buf((float*)buf.pAudioData, &fmt, 440, 4410);

    /* submit same buffer fragment 5 times */
    for(i = 0; i < 5; ++i){
        testdata[i].idx = i;
        testdata[i].src = src;
        buf.pContext = &testdata[i];

        hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
        ok(hr == S_OK, "SubmitSourceBuffer failed: %08lx\n", hr);
    }

    hr = IXAudio2SourceVoice_Start(src, 0, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "Start failed: %08lx\n", hr);


    hr = IXAudio2_StartEngine(xa);
    ok(hr == S_OK, "StartEngine failed: %08lx\n", hr);

    hr = IXAudio2SourceVoice_SetSourceSampleRate(src, 48000);
    if (xaudio27)
        ok(hr == S_OK, "SetSourceSampleRate failed: %08lx\n", hr);
    else
        ok(hr == XAUDIO2_E_INVALID_CALL, "SetSourceSampleRate should have failed: %08lx\n", hr);

    while(1){
        get_voice_state(src, &state);
        if(state.SamplesPlayed >= 4410 * 5)
            break;
        Sleep(100);
    }

    ok(state.SamplesPlayed == 4410 * 5, "Got wrong samples played\n");

    IXAudio2SourceVoice_DestroyVoice(src);

    /* test OnStreamEnd callback */
    hr = IXAudio2_CreateSourceVoice(xa, &src, &fmt, 0, 1.f, &loop_buf, NULL, NULL);
    ok(hr == S_OK, "CreateSourceVoice failed: %08lx\n", hr);

    buf.Flags = XAUDIO2_END_OF_STREAM;

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08lx\n", hr);

    hr = IXAudio2SourceVoice_Start(src, 0, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "Start failed: %08lx\n", hr);

    timeout = 0;
    while(nstreamends == 0 && timeout < 1000){
        Sleep(100);
        timeout += 100;
    }

    ok(nstreamends == 1, "Got wrong number of OnStreamEnd calls: %u\n", nstreamends);

    /* xaudio resets SamplesPlayed after processing an end-of-stream buffer */
    get_voice_state(src, &state);
    ok(state.SamplesPlayed == 0, "Got wrong samples played\n");

    IXAudio2SourceVoice_DestroyVoice(src);
    IXAudio2MasteringVoice_DestroyVoice(master);

    HeapFree(GetProcessHeap(), 0, (void*)buf.pAudioData);
}

static UINT32 play_to_completion(IXAudio2SourceVoice *src, UINT32 max_samples)
{
    XAUDIO2_VOICE_STATE state;
    HRESULT hr;

    nloopends = 0;

    hr = IXAudio2SourceVoice_Start(src, 0, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "Start failed: %08lx\n", hr);

    while(1){
        get_voice_state(src, &state);
        if(state.BuffersQueued == 0)
            break;
        if (state.SamplesPlayed >= max_samples)
            IXAudio2SourceVoice_ExitLoop(src, XAUDIO2_COMMIT_NOW);
        Sleep(100);
    }

    hr = IXAudio2SourceVoice_Stop(src, 0, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "Start failed: %08lx\n", hr);

    return state.SamplesPlayed;
}

static void test_looping(IXAudio2 *xa)
{
    HRESULT hr;
    IXAudio2MasteringVoice *master;
    IXAudio2SourceVoice *src;
    WAVEFORMATEX fmt;
    XAUDIO2_BUFFER buf;
    UINT32 played, running_total = 0;

    IXAudio2_StopEngine(xa);

    hr = create_mastering_voice(xa, 2, &master);
    ok(hr == S_OK, "CreateMasteringVoice failed: %08lx\n", hr);


    fmt.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    fmt.nChannels = 2;
    fmt.nSamplesPerSec = 44100;
    fmt.wBitsPerSample = 32;
    fmt.nBlockAlign = fmt.nChannels * fmt.wBitsPerSample / 8;
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
    fmt.cbSize = 0;

    hr = IXAudio2_CreateSourceVoice(xa, &src, &fmt, 0, 1.f, &loop_buf, NULL, NULL);
    ok(hr == S_OK, "CreateSourceVoice failed: %08lx\n", hr);

    memset(&buf, 0, sizeof(buf));

    buf.AudioBytes = 44100 * fmt.nBlockAlign;
    buf.pAudioData = HeapAlloc(GetProcessHeap(), 0, buf.AudioBytes);
    fill_buf((float*)buf.pAudioData, &fmt, 440, 44100);

    hr = IXAudio2_StartEngine(xa);
    ok(hr == S_OK, "StartEngine failed: %08lx\n", hr);

    /* play from middle to end */
    buf.PlayBegin = 22050;
    buf.PlayLength = 0;
    buf.LoopBegin = 0;
    buf.LoopLength = 0;
    buf.LoopCount = 0;

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08lx\n", hr);

    played = play_to_completion(src, -1);
    ok(played - running_total == 22050, "Got wrong samples played: %u\n", played - running_total);
    running_total = played;
    ok(nloopends == 0, "Got wrong OnLoopEnd calls: %u\n", nloopends);

    /* play 4410 samples from middle */
    buf.PlayBegin = 22050;
    buf.PlayLength = 4410;
    buf.LoopBegin = 0;
    buf.LoopLength = 0;
    buf.LoopCount = 0;

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08lx\n", hr);

    played = play_to_completion(src, -1);
    ok(played - running_total == 4410, "Got wrong samples played: %u\n", played - running_total);
    running_total = played;
    ok(nloopends == 0, "Got wrong OnLoopEnd calls: %u\n", nloopends);

    /* loop 4410 samples in middle */
    buf.PlayBegin = 0;
    buf.PlayLength = 0;
    buf.LoopBegin = 22050;
    buf.LoopLength = 4410;
    buf.LoopCount = 1;

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08lx\n", hr);

    played = play_to_completion(src, -1);
    ok(played - running_total == 44100 + 4410, "Got wrong samples played: %u\n", played - running_total);
    running_total = played;
    ok(nloopends == 1, "Got wrong OnLoopEnd calls: %u\n", nloopends);

    /* play last half, then loop the whole buffer */
    buf.PlayBegin = 22050;
    buf.PlayLength = 0;
    buf.LoopBegin = 0;
    buf.LoopLength = 0;
    buf.LoopCount = 1;

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08lx\n", hr);

    played = play_to_completion(src, -1);
    ok(played - running_total == 22050 + 44100, "Got wrong samples played: %u\n", played - running_total);
    running_total = played;
    ok(nloopends == 1, "Got wrong OnLoopEnd calls: %u\n", nloopends);

    /* play short segment from middle, loop to the beginning, and end at PlayEnd */
    buf.PlayBegin = 22050;
    buf.PlayLength = 4410;
    buf.LoopBegin = 0;
    buf.LoopLength = 0;
    buf.LoopCount = 1;

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08lx\n", hr);

    played = play_to_completion(src, -1);
    ok(played - running_total == 4410 + (22050 + 4410), "Got wrong samples played: %u\n", played - running_total);
    running_total = played;
    ok(nloopends == 1, "Got wrong OnLoopEnd calls: %u\n", nloopends);

    /* invalid: LoopEnd must be <= PlayEnd
     * xaudio27: play until LoopEnd, loop to beginning, play until PlayEnd */
    buf.PlayBegin = 22050;
    buf.PlayLength = 4410;
    buf.LoopBegin = 0;
    buf.LoopLength = 22050 + 4410 * 2;
    buf.LoopCount = 1;

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    if(xaudio27){
        ok(hr == S_OK, "SubmitSourceBuffer failed: %08lx\n", hr);

        played = play_to_completion(src, -1);
        ok(played - running_total == 4410 + (22050 + 4410 * 2), "Got wrong samples played: %u\n", played - running_total);
        running_total = played;
        ok(nloopends == 1, "Got wrong OnLoopEnd calls: %u\n", nloopends);
    }else
        ok(hr == XAUDIO2_E_INVALID_CALL, "SubmitSourceBuffer should have failed: %08lx\n", hr);

    /* invalid: LoopEnd must be within play range
     * xaudio27: plays only play range */
    buf.PlayBegin = 22050;
    buf.PlayLength = 0; /* == until end of buffer */
    buf.LoopBegin = 0;
    buf.LoopLength = 22050;
    buf.LoopCount = 1;

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    if(xaudio27){
        ok(hr == S_OK, "SubmitSourceBuffer failed: %08lx\n", hr);

        played = play_to_completion(src, -1);
        ok(played - running_total == 22050, "Got wrong samples played: %u\n", played - running_total);
        running_total = played;
        ok(nloopends == 0, "Got wrong OnLoopEnd calls: %u\n", nloopends);
    }else
        ok(hr == XAUDIO2_E_INVALID_CALL, "SubmitSourceBuffer should have failed: %08lx\n", hr);

    /* invalid: LoopBegin must be before PlayEnd
     * xaudio27: crashes */
    if(!xaudio27){
        buf.PlayBegin = 0;
        buf.PlayLength = 4410;
        buf.LoopBegin = 22050;
        buf.LoopLength = 4410;
        buf.LoopCount = 1;

        hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
        ok(hr == XAUDIO2_E_INVALID_CALL, "SubmitSourceBuffer should have failed: %08lx\n", hr);
    }

    /* infinite looping buffer */
    buf.PlayBegin = 22050;
    buf.PlayLength = 0;
    buf.LoopBegin = 0;
    buf.LoopLength = 0;
    buf.LoopCount = 255;

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08lx\n", hr);

    played = play_to_completion(src, running_total + 88200);
    ok(played - running_total == 22050 + 44100 * 2, "Got wrong samples played: %u\n", played - running_total);
    ok(nloopends == (played - running_total) / 88200 + 1, "Got wrong OnLoopEnd calls: %u\n", nloopends);
    running_total = played;

    IXAudio2SourceVoice_DestroyVoice(src);
    IXAudio2MasteringVoice_DestroyVoice(master);
    HeapFree(GetProcessHeap(), 0, (void*)buf.pAudioData);
}

struct test_xapo
{
    IXAPO IXAPO_iface;
};

static struct test_xapo test_xapo;
static BOOL test_xapo_supports_ixapo;
static BOOL test_xapo_supports_ixapo27;

static HRESULT WINAPI test_xapo_QueryInterface(IXAPO *iface, REFIID riid, void **ppvObject)
{
    if (IsEqualGUID(riid, &IID_IUnknown))
        *ppvObject = &test_xapo.IXAPO_iface;
    else if (test_xapo_supports_ixapo && IsEqualGUID(riid, &IID_IXAPO))
        *ppvObject = &test_xapo.IXAPO_iface;
    else if (test_xapo_supports_ixapo27 && IsEqualGUID(riid, &IID_IXAPO27))
        *ppvObject = &test_xapo.IXAPO_iface;
    else
        *ppvObject = NULL;

    if(*ppvObject)
        return S_OK;

    return E_NOINTERFACE;
}

static ULONG WINAPI test_xapo_AddRef(IXAPO *iface)
{
    return 1;
}

static ULONG WINAPI test_xapo_Release(IXAPO *iface)
{
    return 1;
}

static HRESULT WINAPI test_xapo_GetRegistrationProperties(IXAPO *iface,
    XAPO_REGISTRATION_PROPERTIES **props)
{
    XAPO_REGISTRATION_PROPERTIES *p;

    *props = CoTaskMemAlloc(sizeof(**props));
    memset(*props, 0, sizeof(**props));
    p = *props;
    p->MinInputBufferCount = 1;
    p->MaxInputBufferCount = 1000;
    p->MinOutputBufferCount = 1;
    p->MaxOutputBufferCount = 1000;
    return S_OK;
}

static HRESULT WINAPI test_xapo_IsInputFormatSupported(IXAPO *iface,
        const WAVEFORMATEX *output_fmt, const WAVEFORMATEX *input_fmt,
        WAVEFORMATEX **supported_fmt)
{
    return S_OK;
}

static HRESULT WINAPI test_xapo_IsOutputFormatSupported(IXAPO *iface,
        const WAVEFORMATEX *input_fmt, const WAVEFORMATEX *output_fmt,
        WAVEFORMATEX **supported_fmt)
{
    return S_OK;
}

static HRESULT WINAPI test_xapo_Initialize(IXAPO *iface, const void *data,
        UINT32 data_len)
{
    return S_OK;
}

static void WINAPI test_xapo_Reset(IXAPO *iface)
{
}

static HRESULT WINAPI test_xapo_LockForProcess(IXAPO *iface, UINT32 in_params_count,
        const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS *in_params,
        UINT32 out_params_count,
        const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS *out_params)
{
    return S_OK;
}

static void WINAPI test_xapo_UnlockForProcess(IXAPO *iface)
{
}

static void WINAPI test_xapo_Process(IXAPO *iface, UINT32 in_params_count,
        const XAPO_PROCESS_BUFFER_PARAMETERS *in_params,
        UINT32 out_params_count,
        XAPO_PROCESS_BUFFER_PARAMETERS *out_params, BOOL enabled)
{
}

static UINT32 WINAPI test_xapo_CalcInputFrames(IXAPO *iface, UINT32 output_frames)
{
    return 0;
}

static UINT32 WINAPI test_xapo_CalcOutputFrames(IXAPO *iface, UINT32 input_frames)
{
    return 0;
}

static const IXAPOVtbl test_xapo_vtbl = {
    test_xapo_QueryInterface,
    test_xapo_AddRef,
    test_xapo_Release,
    test_xapo_GetRegistrationProperties,
    test_xapo_IsInputFormatSupported,
    test_xapo_IsOutputFormatSupported,
    test_xapo_Initialize,
    test_xapo_Reset,
    test_xapo_LockForProcess,
    test_xapo_UnlockForProcess,
    test_xapo_Process,
    test_xapo_CalcInputFrames,
    test_xapo_CalcOutputFrames,
};

static void test_submix(IXAudio2 *xa)
{
    HRESULT hr;
    IXAudio2MasteringVoice *master;
    XAUDIO2_VOICE_DETAILS details;
    IXAudio2SubmixVoice *sub, *sub2;
    XAUDIO2_SEND_DESCRIPTOR send_desc = { 0 };
    XAUDIO2_VOICE_SENDS sends = { 1, &send_desc };
    XAUDIO2_EFFECT_DESCRIPTOR effect;
    XAUDIO2_EFFECT_CHAIN chain;

    check_refcount((IUnknown *)xa, 1);

    IXAudio2_StopEngine(xa);

    hr = create_mastering_voice(xa, 2, &master);
    ok(hr == S_OK, "CreateMasteringVoice failed: %08lx\n", hr);

    effect.InitialState = TRUE;
    effect.OutputChannels = 2;
    effect.pEffect = NULL;

    chain.EffectCount = 1;
    chain.pEffectDescriptors = &effect;

    if (XAUDIO2_VER >= 8)
    {
        /* These tests with invalid effect crash on earlier xaudio2 versions. */
        hr = IXAudio2MasteringVoice_SetEffectChain(master, NULL);
        ok(!hr, "got %#lx\n", hr);
        hr = IXAudio2MasteringVoice_SetEffectChain(master, &chain);
        ok(hr == XAUDIO2_E_INVALID_CALL, "got %#lx\n", hr);

        hr = IXAudio2_CreateSubmixVoice(xa, &sub, 2, 44100, 0, 0, NULL, &chain);
        ok(hr == XAUDIO2_E_INVALID_CALL, "got %#lx\n", hr);
        test_xapo.IXAPO_iface.lpVtbl = &test_xapo_vtbl;

        test_xapo_supports_ixapo = FALSE;
        test_xapo_supports_ixapo27 = FALSE;
        effect.pEffect = (IUnknown *)&test_xapo.IXAPO_iface;
        hr = IXAudio2_CreateSubmixVoice(xa, &sub, 2, 44100, 0, 0, NULL, &chain);
        ok(hr == XAUDIO2_E_INVALID_CALL, "got %#lx\n", hr);

        test_xapo_supports_ixapo27 = TRUE;
        hr = IXAudio2_CreateSubmixVoice(xa, &sub, 2, 44100, 0, 0, NULL, &chain);
        ok(hr == XAUDIO2_E_INVALID_CALL, "got %#lx\n", hr);
        hr = IXAudio2MasteringVoice_SetEffectChain(master, &chain);
        ok(hr == XAUDIO2_E_INVALID_CALL, "got %#lx\n", hr);

        test_xapo_supports_ixapo = TRUE;
        test_xapo_supports_ixapo27 = FALSE;
        hr = IXAudio2_CreateSubmixVoice(xa, &sub, 2, 44100, 0, 0, NULL, &chain);
        ok(!hr, "got %#lx.\n", hr);
        IXAudio2SubmixVoice_DestroyVoice(sub);
    }

    hr = IXAudio2_CreateSubmixVoice(xa, &sub, 2, 44100, 0, 0, NULL, NULL);
    ok(hr == S_OK, "CreateSubmixVoice failed: %08lx\n", hr);

    check_refcount((IUnknown *)xa, 1);

    IXAudio2SubmixVoice_GetVoiceDetails(sub, &details);
    ok(details.CreationFlags == 0, "Got wrong creation flags: 0x%x\n", details.CreationFlags);
#if XAUDIO2_VER >= 8
    ok(details.ActiveFlags == 0, "Got wrong active flags: 0x%x\n", details.ActiveFlags);
#endif
    ok(details.InputChannels == 2, "Got wrong channel count: 0x%x\n", details.InputChannels);
    ok(details.InputSampleRate == 44100, "Got wrong sample rate: 0x%x\n", details.InputSampleRate);

    hr = IXAudio2_CreateSubmixVoice(xa, &sub2, 2, 44100, 0, 0, NULL, NULL);
    ok(hr == S_OK, "CreateSubmixVoice failed: %08lx\n", hr);
    send_desc.pOutputVoice = (IXAudio2Voice *)sub2;
    hr = IXAudio2SubmixVoice_SetOutputVoices(sub, &sends);
    ok(hr == S_OK || (XAUDIO2_VER >= 8 && hr == XAUDIO2_E_INVALID_CALL), "CreateSubmixVoice failed: %08lx\n", hr);
    if (hr == XAUDIO2_E_INVALID_CALL)
    {
        IXAudio2SubmixVoice_DestroyVoice(sub2);
        hr = IXAudio2_CreateSubmixVoice(xa, &sub2, 2, 44100, 0, 1, NULL, NULL);
        ok(hr == S_OK, "CreateSubmixVoice failed: %08lx\n", hr);
        send_desc.pOutputVoice = (IXAudio2Voice *)sub2;
        hr = IXAudio2SubmixVoice_SetOutputVoices(sub, &sends);
        ok(hr == S_OK, "CreateSubmixVoice failed: %08lx\n", hr);
    }

    IXAudio2SubmixVoice_DestroyVoice(sub2);
    /* The voice is not destroyed. */
    memset(&details, 0xcc, sizeof(details));
    IXAudio2SubmixVoice_GetVoiceDetails(sub2, &details);
    ok(details.InputChannels == 2, "Got wrong channel count: 0x%x\n", details.InputChannels);
    ok(details.InputSampleRate == 44100, "Got wrong sample rate: 0x%x\n", details.InputSampleRate);

    IXAudio2MasteringVoice_DestroyVoice(master);
    memset(&details, 0xcc, sizeof(details));
    IXAudio2MasteringVoice_GetVoiceDetails(master, &details);
    ok(details.InputChannels == 2, "Got wrong channel count: 0x%x\n", details.InputChannels);
    ok(details.InputSampleRate == 44100, "Got wrong sample rate: 0x%x\n", details.InputSampleRate);

    check_refcount((IUnknown *)xa, 1);

    sends.SendCount = 0;
    hr = IXAudio2SubmixVoice_SetOutputVoices(sub, &sends);
    ok(hr == S_OK || (XAUDIO2_VER >= 8 && hr == XAUDIO2_E_INVALID_CALL), "SetOutputVoices failed: %08lx\n", hr);
    if (hr == XAUDIO2_E_INVALID_CALL)
    {
        sends.pSends = NULL;
        hr = IXAudio2SubmixVoice_SetOutputVoices(sub, &sends);
        ok(hr == S_OK, "SetOutputVoices failed: %08lx\n", hr);
    }

    IXAudio2SubmixVoice_DestroyVoice(sub2);
    if (0)
    {
        /* Crashes on Windows and thus suggests that now the voice is actually destroyed. */
        IXAudio2SubmixVoice_GetVoiceDetails(sub2, &details);
    }

    IXAudio2SubmixVoice_DestroyVoice(sub);
    IXAudio2MasteringVoice_DestroyVoice(master);
}

static void test_flush(IXAudio2 *xa)
{
    HRESULT hr;
    IXAudio2MasteringVoice *master;
    IXAudio2SourceVoice *src;
    WAVEFORMATEX fmt;
    XAUDIO2_BUFFER buf;
    XAUDIO2_VOICE_STATE state;

    IXAudio2_StopEngine(xa);

    hr = create_mastering_voice(xa, 2, &master);
    ok(hr == S_OK, "CreateMasteringVoice failed: %08lx\n", hr);

    fmt.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    fmt.nChannels = 2;
    fmt.nSamplesPerSec = 44100;
    fmt.wBitsPerSample = 32;
    fmt.nBlockAlign = fmt.nChannels * fmt.wBitsPerSample / 8;
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
    fmt.cbSize = 0;

    hr = IXAudio2_CreateSourceVoice(xa, &src, &fmt, 0, 1.f, NULL, NULL, NULL);
    ok(hr == S_OK, "CreateSourceVoice failed: %08lx\n", hr);

    memset(&buf, 0, sizeof(buf));
    buf.AudioBytes = 22050 * fmt.nBlockAlign;
    buf.pAudioData = HeapAlloc(GetProcessHeap(), 0, buf.AudioBytes);
    fill_buf((float*)buf.pAudioData, &fmt, 440, 22050);

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08lx\n", hr);

    hr = IXAudio2SourceVoice_Start(src, 0, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "Start failed: %08lx\n", hr);

    hr = IXAudio2_StartEngine(xa);
    ok(hr == S_OK, "StartEngine failed: %08lx\n", hr);

    while(1){
        get_voice_state(src, &state);
        if(state.SamplesPlayed >= 2205)
            break;
        Sleep(10);
    }

    hr = IXAudio2SourceVoice_Stop(src, 0, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "Stop failed: %08lx\n", hr);

    hr = IXAudio2SourceVoice_FlushSourceBuffers(src);
    ok(hr == S_OK, "FlushSourceBuffers failed: %08lx\n", hr);

    hr = IXAudio2SourceVoice_Start(src, 0, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "Start failed: %08lx\n", hr);

    Sleep(100);

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08lx\n", hr);

    IXAudio2SourceVoice_DestroyVoice(src);
    IXAudio2MasteringVoice_DestroyVoice(master);

    HeapFree(GetProcessHeap(), 0, (void*)buf.pAudioData);
}

static void test_DeviceDetails(IXAudio2 *xa)
{
#if XAUDIO2_VER <= 7
    HRESULT hr;
    XAUDIO2_DEVICE_DETAILS dd;
    UINT32 count, i;

    hr = IXAudio2_GetDeviceCount(xa, &count);
    ok(hr == S_OK, "GetDeviceCount failed: %08lx\n", hr);
    ok(count > 0, "Got %u devices.\n", count);

    for(i = 0; i < count; ++i){
        hr = IXAudio2_GetDeviceDetails(xa, i, &dd);
        ok(hr == S_OK, "GetDeviceDetails failed: %08lx\n", hr);

        if(i == 0)
            ok(dd.Role == GlobalDefaultDevice, "Got wrong role for index 0: 0x%x\n", dd.Role);
        else
            ok(dd.Role == NotDefaultDevice, "Got wrong role for index %u: 0x%x\n", i, dd.Role);

        ok(IsEqualGUID(&dd.OutputFormat.SubFormat, &KSDATAFORMAT_SUBTYPE_PCM),
           "got format %s\n", debugstr_guid(&dd.OutputFormat.SubFormat));
    }
#endif
}

static void test_xapo_creation_legacy(const char *module, unsigned int version)
{
    HANDLE xapofxdll;
    HRESULT hr;
    IUnknown *fx_unk;
    unsigned int i;
    ULONG rc;

    HRESULT (CDECL *pCreateFX)(REFCLSID,IUnknown**) = NULL;

    /* CLSIDs are the same across all versions */
    static const GUID *const_clsids[] = {
        &CLSID_FXEQ27,
        &CLSID_FXMasteringLimiter27,
        &CLSID_FXReverb27,
        &CLSID_FXEcho27,
        /* older versions of xapofx actually have support for new clsids */
        &CLSID_FXEQ,
        &CLSID_FXMasteringLimiter,
        &CLSID_FXReverb,
        &CLSID_FXEcho
    };

    /* different CLSID for each version */
    static const GUID *avm_clsids[] = {
        &CLSID_AudioVolumeMeter20,
        &CLSID_AudioVolumeMeter21,
        &CLSID_AudioVolumeMeter22,
        &CLSID_AudioVolumeMeter23,
        &CLSID_AudioVolumeMeter24,
        &CLSID_AudioVolumeMeter25,
        &CLSID_AudioVolumeMeter26,
        &CLSID_AudioVolumeMeter27
    };

    static const GUID *ar_clsids[] = {
        &CLSID_AudioReverb20,
        &CLSID_AudioReverb21,
        &CLSID_AudioReverb22,
        &CLSID_AudioReverb23,
        &CLSID_AudioReverb24,
        &CLSID_AudioReverb25,
        &CLSID_AudioReverb26,
        &CLSID_AudioReverb27
    };

    xapofxdll = LoadLibraryA(module);
    if(xapofxdll){
        pCreateFX = (void*)GetProcAddress(xapofxdll, "CreateFX");
        ok(pCreateFX != NULL, "%s did not have CreateFX?\n", module);
        if(!pCreateFX){
            FreeLibrary(xapofxdll);
            return;
        }
    }else{
        win_skip("Couldn't load %s\n", module);
        return;
    }

    for(i = 0; i < ARRAY_SIZE(const_clsids); ++i){
        hr = pCreateFX(const_clsids[i], &fx_unk);
        ok(hr == S_OK, "%s: CreateFX(%s) failed: %08lx\n", module, wine_dbgstr_guid(const_clsids[i]), hr);
        if(SUCCEEDED(hr)){
            IXAPO *xapo;
            hr = IUnknown_QueryInterface(fx_unk, &IID_IXAPO27, (void**)&xapo);
            ok(hr == S_OK, "Couldn't get IXAPO27 interface: %08lx\n", hr);
            if(SUCCEEDED(hr))
                IXAPO_Release(xapo);
            rc = IUnknown_Release(fx_unk);
            ok(rc == 0, "XAPO via CreateFX should have been released, got refcount: %lu\n", rc);
        }

        hr = CoCreateInstance(const_clsids[i], NULL, CLSCTX_INPROC_SERVER,
                &IID_IUnknown, (void**)&fx_unk);
        ok(hr == REGDB_E_CLASSNOTREG, "CoCreateInstance should have failed: %08lx\n", hr);
        if(SUCCEEDED(hr))
            IUnknown_Release(fx_unk);
    }

    hr = pCreateFX(avm_clsids[version - 20], &fx_unk);
    ok(hr == S_OK, "%s: CreateFX(%s) failed: %08lx\n", module, wine_dbgstr_guid(avm_clsids[version - 20]), hr);
    if(SUCCEEDED(hr)){
        IXAPO *xapo;
        hr = IUnknown_QueryInterface(fx_unk, &IID_IXAPO27, (void**)&xapo);
        ok(hr == S_OK, "Couldn't get IXAPO27 interface: %08lx\n", hr);
        if(SUCCEEDED(hr))
            IXAPO_Release(xapo);
        rc = IUnknown_Release(fx_unk);
        ok(rc == 0, "AudioVolumeMeter via CreateFX should have been released, got refcount: %lu\n", rc);
    }

    hr = pCreateFX(ar_clsids[version - 20], &fx_unk);
    ok(hr == S_OK, "%s: CreateFX(%s) failed: %08lx\n", module, wine_dbgstr_guid(ar_clsids[version - 20]), hr);
    if(SUCCEEDED(hr)){
        IXAPO *xapo;
        hr = IUnknown_QueryInterface(fx_unk, &IID_IXAPO27, (void**)&xapo);
        ok(hr == S_OK, "Couldn't get IXAPO27 interface: %08lx\n", hr);
        if(SUCCEEDED(hr))
            IXAPO_Release(xapo);
        rc = IUnknown_Release(fx_unk);
        ok(rc == 0, "AudioReverb via CreateFX should have been released, got refcount: %lu\n", rc);
    }

    FreeLibrary(xapofxdll);
}

static void test_xapo_creation_modern(const char *module)
{
    HANDLE xaudio2dll;
    HRESULT hr;
    IUnknown *fx_unk;
    unsigned int i;
    ULONG rc;

    HRESULT (CDECL *pCreateFX)(REFCLSID,IUnknown**,void*,UINT32) = NULL;
    HRESULT (WINAPI *pCAVM)(IUnknown**) = NULL;
    HRESULT (WINAPI *pCAR)(IUnknown**) = NULL;

    /* CLSIDs are the same across all versions */
    static const GUID *const_clsids[] = {
        &CLSID_FXEQ,
        &CLSID_FXMasteringLimiter,
        &CLSID_FXReverb,
        &CLSID_FXEcho
    };


    xaudio2dll = LoadLibraryA(module);
    if(xaudio2dll){
        pCreateFX = (void*)GetProcAddress(xaudio2dll, "CreateFX");
        ok(pCreateFX != NULL, "%s did not have CreateFX?\n", module);
        if(!pCreateFX){
            FreeLibrary(xaudio2dll);
            return;
        }
    }else{
        win_skip("Couldn't load %s\n", module);
        return;
    }

    for(i = 0; i < ARRAY_SIZE(const_clsids); ++i){
        hr = pCreateFX(const_clsids[i], &fx_unk, NULL, 0);
        ok(hr == S_OK, "%s: CreateFX(%s) failed: %08lx\n", module, wine_dbgstr_guid(const_clsids[i]), hr);
        if(SUCCEEDED(hr)){
            IXAPO *xapo;
            hr = IUnknown_QueryInterface(fx_unk, &IID_IXAPO, (void**)&xapo);
            ok(hr == S_OK, "Couldn't get IXAPO interface: %08lx\n", hr);
            if(SUCCEEDED(hr))
                IXAPO_Release(xapo);
            rc = IUnknown_Release(fx_unk);
            ok(rc == 0, "XAPO via CreateFX should have been released, got refcount: %lu\n", rc);
        }

        hr = CoCreateInstance(const_clsids[i], NULL, CLSCTX_INPROC_SERVER,
                &IID_IUnknown, (void**)&fx_unk);
        ok(hr == REGDB_E_CLASSNOTREG, "CoCreateInstance should have failed: %08lx\n", hr);
        if(SUCCEEDED(hr))
            IUnknown_Release(fx_unk);
    }

    pCAVM = (void*)GetProcAddress(xaudio2dll, "CreateAudioVolumeMeter");
    ok(pCAVM != NULL, "%s did not have CreateAudioVolumeMeter?\n", module);

    hr = pCAVM(&fx_unk);
    ok(hr == S_OK, "CreateAudioVolumeMeter failed: %08lx\n", hr);
    if(SUCCEEDED(hr)){
        IXAPO *xapo;
        hr = IUnknown_QueryInterface(fx_unk, &IID_IXAPO, (void**)&xapo);
        ok(hr == S_OK, "Couldn't get IXAPO interface: %08lx\n", hr);
        if(SUCCEEDED(hr))
            IXAPO_Release(xapo);
        rc = IUnknown_Release(fx_unk);
        ok(rc == 0, "XAPO via CreateAudioVolumeMeter should have been released, got refcount: %lu\n", rc);
    }

    pCAR = (void*)GetProcAddress(xaudio2dll, "CreateAudioReverb");
    ok(pCAR != NULL, "%s did not have CreateAudioReverb?\n", module);

    hr = pCAR(&fx_unk);
    ok(hr == S_OK, "CreateAudioReverb failed: %08lx\n", hr);
    if(SUCCEEDED(hr)){
        IXAPO *xapo;
        hr = IUnknown_QueryInterface(fx_unk, &IID_IXAPO, (void**)&xapo);
        ok(hr == S_OK, "Couldn't get IXAPO interface: %08lx\n", hr);
        if(SUCCEEDED(hr))
            IXAPO_Release(xapo);
        rc = IUnknown_Release(fx_unk);
        ok(rc == 0, "XAPO via CreateAudioReverb should have been released, got refcount: %lu\n", rc);
    }

    FreeLibrary(xaudio2dll);
}

static void test_xapo_creation(void)
{
    test_xapo_creation_legacy("xapofx1_1.dll", 22);
    test_xapo_creation_legacy("xapofx1_2.dll", 23);
    test_xapo_creation_legacy("xapofx1_3.dll", 24);
    test_xapo_creation_legacy("xapofx1_3.dll", 25);
    test_xapo_creation_legacy("xapofx1_4.dll", 26);
    test_xapo_creation_legacy("xapofx1_5.dll", 27);
    test_xapo_creation_modern("xaudio2_8.dll");
}

static void test_setchannelvolumes(IXAudio2 *xa)
{
    HRESULT hr;
    IXAudio2MasteringVoice *master;
    IXAudio2SourceVoice *src_2ch, *src_8ch;
    WAVEFORMATEX fmt_2ch, fmt_8ch;
    float volumes[] = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f};

    hr = create_mastering_voice(xa, 8, &master);
    ok(hr == S_OK, "CreateMasteringVoice failed: %08lx\n", hr);

    fmt_2ch.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    fmt_2ch.nChannels = 2;
    fmt_2ch.nSamplesPerSec = 44100;
    fmt_2ch.wBitsPerSample = 32;
    fmt_2ch.nBlockAlign = fmt_2ch.nChannels * fmt_2ch.wBitsPerSample / 8;
    fmt_2ch.nAvgBytesPerSec = fmt_2ch.nSamplesPerSec * fmt_2ch.nBlockAlign;
    fmt_2ch.cbSize = 0;

    fmt_8ch.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    fmt_8ch.nChannels = 8;
    fmt_8ch.nSamplesPerSec = 44100;
    fmt_8ch.wBitsPerSample = 32;
    fmt_8ch.nBlockAlign = fmt_8ch.nChannels * fmt_8ch.wBitsPerSample / 8;
    fmt_8ch.nAvgBytesPerSec = fmt_8ch.nSamplesPerSec * fmt_8ch.nBlockAlign;
    fmt_8ch.cbSize = 0;

    hr = IXAudio2_CreateSourceVoice(xa, &src_2ch, &fmt_2ch, 0, 1.f, NULL, NULL, NULL);
    ok(hr == S_OK, "CreateSourceVoice failed: %08lx\n", hr);

    hr = IXAudio2_CreateSourceVoice(xa, &src_8ch, &fmt_8ch, 0, 1.f, NULL, NULL, NULL);
    ok(hr == S_OK, "CreateSourceVoice failed: %08lx\n", hr);

    hr = IXAudio2SourceVoice_SetChannelVolumes(src_2ch, 2, volumes, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "SetChannelVolumes failed: %08lx\n", hr);

    hr = IXAudio2SourceVoice_SetChannelVolumes(src_8ch, 8, volumes, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "SetChannelVolumes failed: %08lx\n", hr);

    if(xaudio27){
        /* XAudio 2.7 doesn't check the number of channels */
        hr = IXAudio2SourceVoice_SetChannelVolumes(src_8ch, 2, volumes, XAUDIO2_COMMIT_NOW);
        ok(hr == S_OK, "SetChannelVolumes failed: %08lx\n", hr);
    }else{
        /* the number of channels must be the same as the number of channels on the source voice */
        hr = IXAudio2SourceVoice_SetChannelVolumes(src_8ch, 2, volumes, XAUDIO2_COMMIT_NOW);
        ok(hr == XAUDIO2_E_INVALID_CALL, "SetChannelVolumes should have failed: %08lx\n", hr);

        hr = IXAudio2SourceVoice_SetChannelVolumes(src_2ch, 8, volumes, XAUDIO2_COMMIT_NOW);
        ok(hr == XAUDIO2_E_INVALID_CALL, "SetChannelVolumes should have failed: %08lx\n", hr);

        /* volumes must not be NULL, XAudio 2.7 doesn't check this */
        hr = IXAudio2SourceVoice_SetChannelVolumes(src_2ch, 2, NULL, XAUDIO2_COMMIT_NOW);
        ok(hr == XAUDIO2_E_INVALID_CALL, "SetChannelVolumes should have failed: %08lx\n", hr);
    }

    IXAudio2SourceVoice_DestroyVoice(src_2ch);
    IXAudio2SourceVoice_DestroyVoice(src_8ch);
    IXAudio2MasteringVoice_DestroyVoice(master);
}

#if XAUDIO2_VER >= 8
static void test_XAudio2CreateWithVersionInfo(void)
{
    IXAudio2 *audio;
    HRESULT hr;

    hr = XAudio2CreateWithVersionInfo(&audio, 0, XAUDIO2_DEFAULT_PROCESSOR, 0);
    ok(hr == S_OK, "got %#lx.\n", hr);
    IXAudio2_Release(audio);

    hr = XAudio2CreateWithVersionInfo(&audio, 0, XAUDIO2_DEFAULT_PROCESSOR, ~0);
    ok(hr == S_OK, "got %#lx.\n", hr);
    IXAudio2_Release(audio);
}
#endif

static UINT32 check_has_devices(IXAudio2 *xa)
{
    HRESULT hr;
    IXAudio2MasteringVoice *master;

    hr = create_mastering_voice(xa, 2, &master);
    if(hr != S_OK)
        return 0;

    IXAudio2MasteringVoice_DestroyVoice(master);

    return 1;
}

START_TEST(xaudio2)
{
    IXAudio2 *audio;
    ULONG ref;

    CoInitialize(NULL);

    test_xapo_creation();

    if (!(audio = create_xaudio2()))
        return;

#if XAUDIO2_VER >= 8
    test_XAudio2CreateWithVersionInfo();
#endif

    test_interfaces(audio);

    if (check_has_devices(audio))
    {
        test_DeviceDetails(audio);
        test_simple_streaming(audio);
        test_buffer_callbacks(audio);
        test_looping(audio);
        test_submix(audio);
        test_flush(audio);
        test_setchannelvolumes(audio);
    }

    ref = IXAudio2_Release(audio);
    ok(!ref, "Got unexpected refcount %lu.\n", ref);

    CoUninitialize();
}
