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
#include "mmsystem.h"

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
    ok(hr == S_OK, "RegisterForCallbacks failed: %08x\n", hr);

    if(xaudio27)
        hr = IXAudio27_CreateMasteringVoice((IXAudio27*)xa, &master, 2, 44100, 0, 0, NULL);
    else
        hr = IXAudio2_CreateMasteringVoice(xa, &master, 2, 44100, 0, NULL, NULL, AudioCategory_GameEffects);
    ok(hr == S_OK, "CreateMasteringVoice failed: %08x\n", hr);

    /* create first source voice */
    fmt.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    fmt.nChannels = 2;
    fmt.nSamplesPerSec = 44100;
    fmt.wBitsPerSample = 32;
    fmt.nBlockAlign = fmt.nChannels * fmt.wBitsPerSample / 8;
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
    fmt.cbSize = 0;

    XA2CALL(CreateSourceVoice, &src, &fmt, 0, 1.f, &vcb1, NULL, NULL);
    ok(hr == S_OK, "CreateSourceVoice failed: %08x\n", hr);

    memset(&buf, 0, sizeof(buf));
    buf.AudioBytes = 22050 * fmt.nBlockAlign;
    buf.pAudioData = HeapAlloc(GetProcessHeap(), 0, buf.AudioBytes);
    fill_buf((float*)buf.pAudioData, &fmt, 440, 22050);

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);

    hr = IXAudio2SourceVoice_Start(src, 0, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "Start failed: %08x\n", hr);

    /* create second source voice */
    XA2CALL(CreateSourceVoice, &src2, &fmt, 0, 1.f, &vcb2, NULL, NULL);
    ok(hr == S_OK, "CreateSourceVoice failed: %08x\n", hr);

    memset(&buf2, 0, sizeof(buf2));
    buf2.AudioBytes = 22050 * fmt.nBlockAlign;
    buf2.pAudioData = HeapAlloc(GetProcessHeap(), 0, buf2.AudioBytes);
    fill_buf((float*)buf2.pAudioData, &fmt, 220, 22050);

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src2, &buf2, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);

    hr = IXAudio2SourceVoice_Start(src2, 0, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "Start failed: %08x\n", hr);

    XA2CALL_0(StartEngine);
    ok(hr == S_OK, "StartEngine failed: %08x\n", hr);

    /* hook up volume meter */
    if(xaudio27){
        hr = CoCreateInstance(&CLSID_AudioVolumeMeter, NULL,
                CLSCTX_INPROC_SERVER, &IID_IUnknown, (void**)&vumeter);
        ok(hr == S_OK, "CoCreateInstance(AudioVolumeMeter) failed: %08x\n", hr);
    }else{
        hr = pCreateAudioVolumeMeter(&vumeter);
        ok(hr == S_OK, "CreateAudioVolumeMeter failed: %08x\n", hr);
    }

    effect.InitialState = TRUE;
    effect.OutputChannels = 2;
    effect.pEffect = vumeter;

    chain.EffectCount = 1;
    chain.pEffectDescriptors = &effect;

    hr = IXAudio2MasteringVoice_SetEffectChain(master, &chain);
    ok(hr == S_OK, "SetEffectchain failed: %08x\n", hr);

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
    ok(hr == S_OK, "CreateMasteringVoice failed: %08x\n", hr);

    /* test OnBufferStart/End callbacks */
    fmt.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    fmt.nChannels = 2;
    fmt.nSamplesPerSec = 44100;
    fmt.wBitsPerSample = 32;
    fmt.nBlockAlign = fmt.nChannels * fmt.wBitsPerSample / 8;
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
    fmt.cbSize = 0;

    XA2CALL(CreateSourceVoice, &src, &fmt, 0, 1.f, &vcb_buf, NULL, NULL);
    ok(hr == S_OK, "CreateSourceVoice failed: %08x\n", hr);

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
        ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);
    }

    hr = IXAudio2SourceVoice_Start(src, 0, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "Start failed: %08x\n", hr);


    XA2CALL_0(StartEngine);
    ok(hr == S_OK, "StartEngine failed: %08x\n", hr);

    if(xaudio27){
        hr = IXAudio27SourceVoice_SetSourceSampleRate((IXAudio27SourceVoice*)src, 48000);
        todo_wine ok(hr == S_OK, "SetSourceSampleRate failed: %08x\n", hr);
    }else{
        hr = IXAudio2SourceVoice_SetSourceSampleRate(src, 48000);
        ok(hr == XAUDIO2_E_INVALID_CALL, "SetSourceSampleRate should have failed: %08x\n", hr);
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
    ok(hr == S_OK, "CreateSourceVoice failed: %08x\n", hr);

    buf.Flags = XAUDIO2_END_OF_STREAM;

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);

    hr = IXAudio2SourceVoice_Start(src, 0, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "Start failed: %08x\n", hr);

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
    ok(hr == S_OK, "Start failed: %08x\n", hr);

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
    ok(hr == S_OK, "Start failed: %08x\n", hr);

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
    ok(hr == S_OK, "CreateMasteringVoice failed: %08x\n", hr);


    fmt.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    fmt.nChannels = 2;
    fmt.nSamplesPerSec = 44100;
    fmt.wBitsPerSample = 32;
    fmt.nBlockAlign = fmt.nChannels * fmt.wBitsPerSample / 8;
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
    fmt.cbSize = 0;

    XA2CALL(CreateSourceVoice, &src, &fmt, 0, 1.f, &loop_buf, NULL, NULL);
    ok(hr == S_OK, "CreateSourceVoice failed: %08x\n", hr);

    memset(&buf, 0, sizeof(buf));

    buf.AudioBytes = 44100 * fmt.nBlockAlign;
    buf.pAudioData = HeapAlloc(GetProcessHeap(), 0, buf.AudioBytes);
    fill_buf((float*)buf.pAudioData, &fmt, 440, 44100);

    XA2CALL_0(StartEngine);
    ok(hr == S_OK, "StartEngine failed: %08x\n", hr);

    /* play from middle to end */
    buf.PlayBegin = 22050;
    buf.PlayLength = 0;
    buf.LoopBegin = 0;
    buf.LoopLength = 0;
    buf.LoopCount = 0;

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);

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
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);

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
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);

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
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);

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
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);

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
        ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);

        played = play_to_completion(src, -1);
        ok(played - running_total == 4410 + (22050 + 4410 * 2), "Got wrong samples played: %u\n", played - running_total);
        running_total = played;
        ok(nloopends == 1, "Got wrong OnLoopEnd calls: %u\n", nloopends);
    }else
        ok(hr == XAUDIO2_E_INVALID_CALL, "SubmitSourceBuffer should have failed: %08x\n", hr);

    /* invalid: LoopEnd must be within play range
     * xaudio27: plays only play range */
    buf.PlayBegin = 22050;
    buf.PlayLength = 0; /* == until end of buffer */
    buf.LoopBegin = 0;
    buf.LoopLength = 22050;
    buf.LoopCount = 1;

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    if(xaudio27){
        ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);

        played = play_to_completion(src, -1);
        ok(played - running_total == 22050, "Got wrong samples played: %u\n", played - running_total);
        running_total = played;
        ok(nloopends == 0, "Got wrong OnLoopEnd calls: %u\n", nloopends);
    }else
        ok(hr == XAUDIO2_E_INVALID_CALL, "SubmitSourceBuffer should have failed: %08x\n", hr);

    /* invalid: LoopBegin must be before PlayEnd
     * xaudio27: crashes */
    if(!xaudio27){
        buf.PlayBegin = 0;
        buf.PlayLength = 4410;
        buf.LoopBegin = 22050;
        buf.LoopLength = 4410;
        buf.LoopCount = 1;

        hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
        ok(hr == XAUDIO2_E_INVALID_CALL, "SubmitSourceBuffer should have failed: %08x\n", hr);
    }

    /* infinite looping buffer */
    buf.PlayBegin = 22050;
    buf.PlayLength = 0;
    buf.LoopBegin = 0;
    buf.LoopLength = 0;
    buf.LoopCount = 255;

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);

    played = play_to_completion(src, running_total + 88200);
    ok(played - running_total == 22050 + 44100 * 2, "Got wrong samples played: %u\n", played - running_total);
    ok(nloopends == (played - running_total) / 88200 + 1, "Got wrong OnLoopEnd calls: %u\n", nloopends);
    running_total = played;
}

static UINT32 test_DeviceDetails(IXAudio27 *xa)
{
    HRESULT hr;
    XAUDIO2_DEVICE_DETAILS dd;
    UINT32 count, i;

    hr = IXAudio27_GetDeviceCount(xa, &count);
    ok(hr == S_OK, "GetDeviceCount failed: %08x\n", hr);

    if(count == 0)
        return 0;

    for(i = 0; i < count; ++i){
        hr = IXAudio27_GetDeviceDetails(xa, i, &dd);
        ok(hr == S_OK, "GetDeviceDetails failed: %08x\n", hr);

        if(i == 0)
            ok(dd.Role == GlobalDefaultDevice, "Got wrong role for index 0: 0x%x\n", dd.Role);
        else
            ok(dd.Role == NotDefaultDevice, "Got wrong role for index %u: 0x%x\n", i, dd.Role);
    }

    return count;
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

    CoInitialize(NULL);

    xa28dll = LoadLibraryA("xaudio2_8.dll");
    if(xa28dll){
        pXAudio2Create = (void*)GetProcAddress(xa28dll, "XAudio2Create");
        pCreateAudioVolumeMeter = (void*)GetProcAddress(xa28dll, "CreateAudioVolumeMeter");
    }

    /* XAudio 2.7 (Jun 2010 DirectX) */
    hr = CoCreateInstance(&CLSID_XAudio2, NULL, CLSCTX_INPROC_SERVER,
            &IID_IXAudio27, (void**)&xa27);
    if(hr == S_OK){
        xaudio27 = TRUE;

        hr = IXAudio27_Initialize(xa27, 0, XAUDIO2_ANY_PROCESSOR);
        ok(hr == S_OK, "Initialize failed: %08x\n", hr);

        has_devices = test_DeviceDetails(xa27);
        if(has_devices){
            test_simple_streaming((IXAudio2*)xa27);
            test_buffer_callbacks((IXAudio2*)xa27);
            test_looping((IXAudio2*)xa27);
        }else
            skip("No audio devices available\n");

        IXAudio27_Release(xa27);
    }else
        win_skip("XAudio 2.7 not available\n");

    /* XAudio 2.8 (Win8+) */
    if(pXAudio2Create){
        xaudio27 = FALSE;

        hr = pXAudio2Create(&xa, 0, XAUDIO2_DEFAULT_PROCESSOR);
        ok(hr == S_OK, "XAudio2Create failed: %08x\n", hr);

        has_devices = check_has_devices(xa);
        if(has_devices){
            test_simple_streaming(xa);
            test_buffer_callbacks(xa);
            test_looping(xa);
        }else
            skip("No audio devices available\n");

        IXAudio2_Release(xa);
    }else
        win_skip("XAudio 2.8 not available\n");

    CoUninitialize();
}
