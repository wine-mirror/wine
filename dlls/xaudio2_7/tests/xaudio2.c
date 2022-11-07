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

#define COBJMACROS
#include "wine/test.h"
#include "initguid.h"
#include "xaudio2.h"
#include "xaudio2fx.h"
#include "xapo.h"
#include "xapofx.h"
#include "mmsystem.h"
#include "ks.h"
#include "ksmedia.h"

static BOOL xaudio27;

static HRESULT (WINAPI *pXAudio2Create)(IXAudio2 **, UINT32, XAUDIO2_PROCESSOR) = NULL;
static HRESULT (WINAPI *pCreateAudioVolumeMeter)(IUnknown**) = NULL;

#define XA2CALL_0(method) if(xaudio27) hr = IXAudio27_##method((IXAudio27*)xa); else hr = IXAudio2_##method(xa);
#define XA2CALL_0V(method) if(xaudio27) IXAudio27_##method((IXAudio27*)xa); else IXAudio2_##method(xa);
#define XA2CALL_V(method, ...) if(xaudio27) IXAudio27_##method((IXAudio27*)xa, __VA_ARGS__); else IXAudio2_##method(xa, __VA_ARGS__);
#define XA2CALL(method, ...) if(xaudio27) hr = IXAudio27_##method((IXAudio27*)xa, __VA_ARGS__); else hr = IXAudio2_##method(xa, __VA_ARGS__);

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
    XAUDIO2_VOICE_STATE state;
    XAUDIO2_EFFECT_DESCRIPTOR effect;
    XAUDIO2_EFFECT_CHAIN chain;
    DWORD chmask;

    memset(&ecb_state, 0, sizeof(ecb_state));
    memset(&src1_state, 0, sizeof(src1_state));
    memset(&src2_state, 0, sizeof(src2_state));

    XA2CALL_0V(StopEngine);

    /* Tests show in native XA2.8, ECB is called from a mixer thread, but VCBs
     * may be called from other threads in any order. So we can't rely on any
     * sequencing between VCB calls.
     *
     * XA2.7 does all mixing from a single thread, so call sequence can be
     * tested. */
    XA2CALL(RegisterForCallbacks, &ecb);
    ok(hr == S_OK, "RegisterForCallbacks failed: %08lx\n", hr);

    if(xaudio27)
        hr = IXAudio27_CreateMasteringVoice((IXAudio27*)xa, &master, 2, 44100, 0, 0, NULL);
    else
        hr = IXAudio2_CreateMasteringVoice(xa, &master, 2, 44100, 0, NULL, NULL, AudioCategory_GameEffects);
    ok(hr == S_OK, "CreateMasteringVoice failed: %08lx\n", hr);

    if(!xaudio27){
        chmask = 0xdeadbeef;
        IXAudio2MasteringVoice_GetChannelMask(master, &chmask);
        ok(chmask == (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT), "Got unexpected channel mask: 0x%lx\n", chmask);
    }

    /* create first source voice */
    fmt.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    fmt.nChannels = 2;
    fmt.nSamplesPerSec = 44100;
    fmt.wBitsPerSample = 32;
    fmt.nBlockAlign = fmt.nChannels * fmt.wBitsPerSample / 8;
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
    fmt.cbSize = 0;

    XA2CALL(CreateSourceVoice, &src, &fmt, 0, 1.f, &vcb1, NULL, NULL);
    ok(hr == S_OK, "CreateSourceVoice failed: %08lx\n", hr);

    if(xaudio27){
        XAUDIO27_VOICE_DETAILS details;
        IXAudio27SourceVoice_GetVoiceDetails((IXAudio27SourceVoice*)src, &details);
        ok(details.CreationFlags == 0, "Got wrong flags: 0x%x\n", details.CreationFlags);
        ok(details.InputChannels == 2, "Got wrong channel count: 0x%x\n", details.InputChannels);
        ok(details.InputSampleRate == 44100, "Got wrong sample rate: 0x%x\n", details.InputSampleRate);
    }else{
        XAUDIO2_VOICE_DETAILS details;
        IXAudio2SourceVoice_GetVoiceDetails(src, &details);
        ok(details.CreationFlags == 0, "Got wrong creation flags: 0x%x\n", details.CreationFlags);
        ok(details.ActiveFlags == 0, "Got wrong active flags: 0x%x\n", details.CreationFlags);
        ok(details.InputChannels == 2, "Got wrong channel count: 0x%x\n", details.InputChannels);
        ok(details.InputSampleRate == 44100, "Got wrong sample rate: 0x%x\n", details.InputSampleRate);
    }

    memset(&buf, 0, sizeof(buf));
    buf.AudioBytes = 22050 * fmt.nBlockAlign;
    buf.pAudioData = HeapAlloc(GetProcessHeap(), 0, buf.AudioBytes);
    fill_buf((float*)buf.pAudioData, &fmt, 440, 22050);

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08lx\n", hr);

    hr = IXAudio2SourceVoice_Start(src, 0, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "Start failed: %08lx\n", hr);

    /* create second source voice */
    XA2CALL(CreateSourceVoice, &src2, &fmt, 0, 1.f, &vcb2, NULL, NULL);
    ok(hr == S_OK, "CreateSourceVoice failed: %08lx\n", hr);

    if(xaudio27){
        XAUDIO27_VOICE_DETAILS details;
        IXAudio27SourceVoice_GetVoiceDetails((IXAudio27SourceVoice*)src2, &details);
        ok(details.CreationFlags == 0, "Got wrong flags: 0x%x\n", details.CreationFlags);
        ok(details.InputChannels == 2, "Got wrong channel count: 0x%x\n", details.InputChannels);
        ok(details.InputSampleRate == 44100, "Got wrong sample rate: 0x%x\n", details.InputSampleRate);
    }else{
        XAUDIO2_VOICE_DETAILS details;
        IXAudio2SourceVoice_GetVoiceDetails(src2, &details);
        ok(details.CreationFlags == 0, "Got wrong creation flags: 0x%x\n", details.CreationFlags);
        ok(details.ActiveFlags == 0, "Got wrong active flags: 0x%x\n", details.CreationFlags);
        ok(details.InputChannels == 2, "Got wrong channel count: 0x%x\n", details.InputChannels);
        ok(details.InputSampleRate == 44100, "Got wrong sample rate: 0x%x\n", details.InputSampleRate);
    }

    memset(&buf2, 0, sizeof(buf2));
    buf2.AudioBytes = 22050 * fmt.nBlockAlign;
    buf2.pAudioData = HeapAlloc(GetProcessHeap(), 0, buf2.AudioBytes);
    fill_buf((float*)buf2.pAudioData, &fmt, 220, 22050);

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src2, &buf2, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08lx\n", hr);

    hr = IXAudio2SourceVoice_Start(src2, 0, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "Start failed: %08lx\n", hr);

    XA2CALL_0(StartEngine);
    ok(hr == S_OK, "StartEngine failed: %08lx\n", hr);

    /* hook up volume meter */
    if(xaudio27){
        IXAPO *xapo;

        hr = CoCreateInstance(&CLSID_AudioVolumeMeter27, NULL,
                CLSCTX_INPROC_SERVER, &IID_IUnknown, (void**)&vumeter);
        ok(hr == S_OK, "CoCreateInstance(AudioVolumeMeter) failed: %08lx\n", hr);

        hr = IUnknown_QueryInterface(vumeter, &IID_IXAPO27, (void**)&xapo);
        ok(hr == S_OK, "Couldn't get IXAPO27 interface: %08lx\n", hr);
        if(SUCCEEDED(hr))
            IXAPO_Release(xapo);
    }else{
        IXAPO *xapo;

        hr = pCreateAudioVolumeMeter(&vumeter);
        ok(hr == S_OK, "CreateAudioVolumeMeter failed: %08lx\n", hr);

        hr = IUnknown_QueryInterface(vumeter, &IID_IXAPO, (void**)&xapo);
        ok(hr == S_OK, "Couldn't get IXAPO interface: %08lx\n", hr);
        if(SUCCEEDED(hr))
            IXAPO_Release(xapo);
    }

    effect.InitialState = TRUE;
    effect.OutputChannels = 2;
    effect.pEffect = vumeter;

    chain.EffectCount = 1;
    chain.pEffectDescriptors = &effect;

    hr = IXAudio2MasteringVoice_SetEffectChain(master, &chain);
    ok(hr == S_OK, "SetEffectchain failed: %08lx\n", hr);

    IUnknown_Release(vumeter);

    while(1){
        if(xaudio27)
            IXAudio27SourceVoice_GetState((IXAudio27SourceVoice*)src, &state);
        else
            IXAudio2SourceVoice_GetState(src, &state, 0);
        if(state.SamplesPlayed >= 22050)
            break;
        Sleep(100);
    }

    ok(state.SamplesPlayed == 22050, "Got wrong samples played\n");

    HeapFree(GetProcessHeap(), 0, (void*)buf.pAudioData);
    HeapFree(GetProcessHeap(), 0, (void*)buf2.pAudioData);

    if(xaudio27){
        IXAudio27SourceVoice_DestroyVoice((IXAudio27SourceVoice*)src);
        IXAudio27SourceVoice_DestroyVoice((IXAudio27SourceVoice*)src2);
    }else{
        IXAudio2SourceVoice_DestroyVoice(src);
        IXAudio2SourceVoice_DestroyVoice(src2);
    }
    IXAudio2MasteringVoice_DestroyVoice(master);

    XA2CALL_V(UnregisterForCallbacks, &ecb);
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

    if(xaudio27)
        IXAudio27SourceVoice_GetState((IXAudio27SourceVoice*)data->src, &state);
    else
        IXAudio2SourceVoice_GetState(data->src, &state, 0);

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

    if(xaudio27)
        IXAudio27SourceVoice_GetState((IXAudio27SourceVoice*)data->src, &state);
    else
        IXAudio2SourceVoice_GetState(data->src, &state, 0);

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

    XA2CALL_0V(StopEngine);

    if(xaudio27)
        hr = IXAudio27_CreateMasteringVoice((IXAudio27*)xa, &master, 2, 44100, 0, 0, NULL);
    else
        hr = IXAudio2_CreateMasteringVoice(xa, &master, 2, 44100, 0, NULL, NULL, AudioCategory_GameEffects);
    ok(hr == S_OK, "CreateMasteringVoice failed: %08lx\n", hr);

    /* test OnBufferStart/End callbacks */
    fmt.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    fmt.nChannels = 2;
    fmt.nSamplesPerSec = 44100;
    fmt.wBitsPerSample = 32;
    fmt.nBlockAlign = fmt.nChannels * fmt.wBitsPerSample / 8;
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
    fmt.cbSize = 0;

    XA2CALL(CreateSourceVoice, &src, &fmt, 0, 1.f, &vcb_buf, NULL, NULL);
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


    XA2CALL_0(StartEngine);
    ok(hr == S_OK, "StartEngine failed: %08lx\n", hr);

    if(xaudio27){
        hr = IXAudio27SourceVoice_SetSourceSampleRate((IXAudio27SourceVoice*)src, 48000);
        ok(hr == S_OK, "SetSourceSampleRate failed: %08lx\n", hr);
    }else{
        hr = IXAudio2SourceVoice_SetSourceSampleRate(src, 48000);
        ok(hr == XAUDIO2_E_INVALID_CALL, "SetSourceSampleRate should have failed: %08lx\n", hr);
    }

    while(1){
        if(xaudio27)
            IXAudio27SourceVoice_GetState((IXAudio27SourceVoice*)src, &state);
        else
            IXAudio2SourceVoice_GetState(src, &state, 0);
        if(state.SamplesPlayed >= 4410 * 5)
            break;
        Sleep(100);
    }

    ok(state.SamplesPlayed == 4410 * 5, "Got wrong samples played\n");

    if(xaudio27)
        IXAudio27SourceVoice_DestroyVoice((IXAudio27SourceVoice*)src);
    else
        IXAudio2SourceVoice_DestroyVoice(src);


    /* test OnStreamEnd callback */
    XA2CALL(CreateSourceVoice, &src, &fmt, 0, 1.f, &loop_buf, NULL, NULL);
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
    if(xaudio27)
        IXAudio27SourceVoice_GetState((IXAudio27SourceVoice*)src, &state);
    else
        IXAudio2SourceVoice_GetState(src, &state, 0);
    ok(state.SamplesPlayed == 0, "Got wrong samples played\n");

    if(xaudio27)
        IXAudio27SourceVoice_DestroyVoice((IXAudio27SourceVoice*)src);
    else
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
        if(xaudio27)
            IXAudio27SourceVoice_GetState((IXAudio27SourceVoice*)src, &state);
        else
            IXAudio2SourceVoice_GetState(src, &state, 0);
        if(state.BuffersQueued == 0)
            break;
        if(state.SamplesPlayed >= max_samples){
            if(xaudio27)
                IXAudio27SourceVoice_ExitLoop((IXAudio27SourceVoice*)src, XAUDIO2_COMMIT_NOW);
            else
                IXAudio2SourceVoice_ExitLoop(src, XAUDIO2_COMMIT_NOW);
        }
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

    XA2CALL_0V(StopEngine);

    if(xaudio27)
        hr = IXAudio27_CreateMasteringVoice((IXAudio27*)xa, &master, 2, 44100, 0, 0, NULL);
    else
        hr = IXAudio2_CreateMasteringVoice(xa, &master, 2, 44100, 0, NULL, NULL, AudioCategory_GameEffects);
    ok(hr == S_OK, "CreateMasteringVoice failed: %08lx\n", hr);


    fmt.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    fmt.nChannels = 2;
    fmt.nSamplesPerSec = 44100;
    fmt.wBitsPerSample = 32;
    fmt.nBlockAlign = fmt.nChannels * fmt.wBitsPerSample / 8;
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
    fmt.cbSize = 0;

    XA2CALL(CreateSourceVoice, &src, &fmt, 0, 1.f, &loop_buf, NULL, NULL);
    ok(hr == S_OK, "CreateSourceVoice failed: %08lx\n", hr);

    memset(&buf, 0, sizeof(buf));

    buf.AudioBytes = 44100 * fmt.nBlockAlign;
    buf.pAudioData = HeapAlloc(GetProcessHeap(), 0, buf.AudioBytes);
    fill_buf((float*)buf.pAudioData, &fmt, 440, 44100);

    XA2CALL_0(StartEngine);
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

    if(xaudio27)
        IXAudio27SourceVoice_DestroyVoice((IXAudio27SourceVoice*)src);
    else
        IXAudio2SourceVoice_DestroyVoice(src);
    IXAudio2MasteringVoice_DestroyVoice(master);
    HeapFree(GetProcessHeap(), 0, (void*)buf.pAudioData);
}

static void test_submix(IXAudio2 *xa)
{
    HRESULT hr;
    IXAudio2MasteringVoice *master;
    IXAudio2SubmixVoice *sub;

    XA2CALL_0V(StopEngine);

    if(xaudio27)
        hr = IXAudio27_CreateMasteringVoice((IXAudio27*)xa, &master, 2, 44100, 0, 0, NULL);
    else
        hr = IXAudio2_CreateMasteringVoice(xa, &master, 2, 44100, 0, NULL, NULL, AudioCategory_GameEffects);
    ok(hr == S_OK, "CreateMasteringVoice failed: %08lx\n", hr);

    XA2CALL(CreateSubmixVoice, &sub, 2, 44100, 0, 0, NULL, NULL);
    ok(hr == S_OK, "CreateSubmixVoice failed: %08lx\n", hr);

    if(xaudio27){
        XAUDIO27_VOICE_DETAILS details;
        IXAudio27SubmixVoice_GetVoiceDetails((IXAudio27SubmixVoice*)sub, &details);
        ok(details.CreationFlags == 0, "Got wrong flags: 0x%x\n", details.CreationFlags);
        ok(details.InputChannels == 2, "Got wrong channel count: 0x%x\n", details.InputChannels);
        ok(details.InputSampleRate == 44100, "Got wrong sample rate: 0x%x\n", details.InputSampleRate);
    }else{
        XAUDIO2_VOICE_DETAILS details;
        IXAudio2SubmixVoice_GetVoiceDetails(sub, &details);
        ok(details.CreationFlags == 0, "Got wrong creation flags: 0x%x\n", details.CreationFlags);
        ok(details.ActiveFlags == 0, "Got wrong active flags: 0x%x\n", details.CreationFlags);
        ok(details.InputChannels == 2, "Got wrong channel count: 0x%x\n", details.InputChannels);
        ok(details.InputSampleRate == 44100, "Got wrong sample rate: 0x%x\n", details.InputSampleRate);
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

    XA2CALL_0V(StopEngine);

    if(xaudio27)
        hr = IXAudio27_CreateMasteringVoice((IXAudio27*)xa, &master, 2, 44100, 0, 0, NULL);
    else
        hr = IXAudio2_CreateMasteringVoice(xa, &master, 2, 44100, 0, NULL, NULL, AudioCategory_GameEffects);
    ok(hr == S_OK, "CreateMasteringVoice failed: %08lx\n", hr);

    fmt.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    fmt.nChannels = 2;
    fmt.nSamplesPerSec = 44100;
    fmt.wBitsPerSample = 32;
    fmt.nBlockAlign = fmt.nChannels * fmt.wBitsPerSample / 8;
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
    fmt.cbSize = 0;

    XA2CALL(CreateSourceVoice, &src, &fmt, 0, 1.f, NULL, NULL, NULL);
    ok(hr == S_OK, "CreateSourceVoice failed: %08lx\n", hr);

    memset(&buf, 0, sizeof(buf));
    buf.AudioBytes = 22050 * fmt.nBlockAlign;
    buf.pAudioData = HeapAlloc(GetProcessHeap(), 0, buf.AudioBytes);
    fill_buf((float*)buf.pAudioData, &fmt, 440, 22050);

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08lx\n", hr);

    hr = IXAudio2SourceVoice_Start(src, 0, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "Start failed: %08lx\n", hr);

    XA2CALL_0(StartEngine);
    ok(hr == S_OK, "StartEngine failed: %08lx\n", hr);

    while(1){
        if(xaudio27)
            IXAudio27SourceVoice_GetState((IXAudio27SourceVoice*)src, &state);
        else
            IXAudio2SourceVoice_GetState(src, &state, 0);
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

    if(xaudio27){
        IXAudio27SourceVoice_DestroyVoice((IXAudio27SourceVoice*)src);
    }else{
        IXAudio2SourceVoice_DestroyVoice(src);
    }
    IXAudio2MasteringVoice_DestroyVoice(master);

    HeapFree(GetProcessHeap(), 0, (void*)buf.pAudioData);
}

static UINT32 test_DeviceDetails(IXAudio27 *xa)
{
    HRESULT hr;
    XAUDIO2_DEVICE_DETAILS dd;
    UINT32 count, i;

    hr = IXAudio27_GetDeviceCount(xa, &count);
    ok(hr == S_OK, "GetDeviceCount failed: %08lx\n", hr);

    if(count == 0)
        return 0;

    for(i = 0; i < count; ++i){
        hr = IXAudio27_GetDeviceDetails(xa, i, &dd);
        ok(hr == S_OK, "GetDeviceDetails failed: %08lx\n", hr);

        if(i == 0)
            ok(dd.Role == GlobalDefaultDevice, "Got wrong role for index 0: 0x%x\n", dd.Role);
        else
            ok(dd.Role == NotDefaultDevice, "Got wrong role for index %u: 0x%x\n", i, dd.Role);

        ok(IsEqualGUID(&dd.OutputFormat.SubFormat, &KSDATAFORMAT_SUBTYPE_PCM),
           "got format %s\n", debugstr_guid(&dd.OutputFormat.SubFormat));
    }

    return count;
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

    if(xaudio27)
        hr = IXAudio27_CreateMasteringVoice((IXAudio27*)xa, &master, 8, 44100, 0, 0, NULL);
    else
        hr = IXAudio2_CreateMasteringVoice(xa, &master, 8, 44100, 0, NULL, NULL, AudioCategory_GameEffects);
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

    XA2CALL(CreateSourceVoice, &src_2ch, &fmt_2ch, 0, 1.f, NULL, NULL, NULL);
    ok(hr == S_OK, "CreateSourceVoice failed: %08lx\n", hr);

    XA2CALL(CreateSourceVoice, &src_8ch, &fmt_8ch, 0, 1.f, NULL, NULL, NULL);
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

    if(xaudio27){
        IXAudio27SourceVoice_DestroyVoice((IXAudio27SourceVoice*)src_2ch);
        IXAudio27SourceVoice_DestroyVoice((IXAudio27SourceVoice*)src_8ch);
    }else{
        IXAudio2SourceVoice_DestroyVoice(src_2ch);
        IXAudio2SourceVoice_DestroyVoice(src_8ch);
    }

    IXAudio2MasteringVoice_DestroyVoice(master);
}

static UINT32 check_has_devices(IXAudio2 *xa)
{
    HRESULT hr;
    IXAudio2MasteringVoice *master;

    hr = IXAudio2_CreateMasteringVoice(xa, &master, 2, 44100, 0, NULL, NULL, AudioCategory_GameEffects);
    if(hr != S_OK)
        return 0;

    IXAudio2MasteringVoice_DestroyVoice(master);

    return 1;
}

START_TEST(xaudio2)
{
    HRESULT hr;
    IXAudio27 *xa27 = NULL;
    IXAudio2 *xa = NULL;
    HANDLE xa28dll;
    UINT32 has_devices;
    ULONG rc;

    CoInitialize(NULL);

    xa28dll = LoadLibraryA("xaudio2_8.dll");
    if(xa28dll){
        pXAudio2Create = (void*)GetProcAddress(xa28dll, "XAudio2Create");
        pCreateAudioVolumeMeter = (void*)GetProcAddress(xa28dll, "CreateAudioVolumeMeter");
    }

    test_xapo_creation();

    /* XAudio 2.7 (Jun 2010 DirectX) */
    hr = CoCreateInstance(&CLSID_XAudio27, NULL, CLSCTX_INPROC_SERVER,
            &IID_IXAudio27, (void**)&xa27);
    if(hr == S_OK){
        xaudio27 = TRUE;

        hr = IXAudio27_QueryInterface(xa27, &IID_IXAudio28, (void**) &xa);
        ok(hr != S_OK, "QueryInterface with IID_IXAudio28 on IXAudio27 object returned success. Expected to fail\n");

        hr = IXAudio27_Initialize(xa27, 0, XAUDIO2_ANY_PROCESSOR);
        ok(hr == S_OK, "Initialize failed: %08lx\n", hr);

        has_devices = test_DeviceDetails(xa27);
        if(has_devices){
            test_simple_streaming((IXAudio2*)xa27);
            test_buffer_callbacks((IXAudio2*)xa27);
            test_looping((IXAudio2*)xa27);
            test_submix((IXAudio2*)xa27);
            test_flush((IXAudio2*)xa27);
            test_setchannelvolumes((IXAudio2*)xa27);
        }else
            skip("No audio devices available\n");

        rc = IXAudio27_Release(xa27);
        ok(rc == 0, "IXAudio2.7 object should have been released, got refcount %lu\n", rc);
    }else
        win_skip("XAudio 2.7 not available\n");

    /* XAudio 2.8 (Win8+) */
    if(pXAudio2Create){
        xaudio27 = FALSE;

        hr = pXAudio2Create(&xa, 0, XAUDIO2_DEFAULT_PROCESSOR);
        ok(hr == S_OK, "XAudio2Create failed: %08lx\n", hr);

        hr = IXAudio2_QueryInterface(xa, &IID_IXAudio27, (void**)&xa27);
        ok(hr == E_NOINTERFACE, "XA28 object should support IXAudio27, gave: %08lx\n", hr);

        has_devices = check_has_devices(xa);
        if(has_devices){
            test_simple_streaming(xa);
            test_buffer_callbacks(xa);
            test_looping(xa);
            test_submix(xa);
            test_flush(xa);
            test_setchannelvolumes(xa);
        }else
            skip("No audio devices available\n");

        rc = IXAudio2_Release(xa);
        ok(rc == 0, "IXAudio2 object should have been released, got refcount %lu\n", rc);
    }else
        win_skip("XAudio 2.8 not available\n");

    CoUninitialize();
}
