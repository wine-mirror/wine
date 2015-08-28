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
#include "mmsystem.h"

static BOOL xaudio27;

static HRESULT (WINAPI *pXAudio2Create)(IXAudio2 **, UINT32, XAUDIO2_PROCESSOR) = NULL;

#define XA2CALL_0(method) if(xaudio27) hr = IXAudio27_##method((IXAudio27*)xa); else hr = IXAudio2_##method(xa);
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

static void test_simple_streaming(IXAudio2 *xa)
{
    HRESULT hr;
    IXAudio2MasteringVoice *master;
    IXAudio2SourceVoice *src, *src2;
    WAVEFORMATEX fmt;
    XAUDIO2_BUFFER buf, buf2;
    XAUDIO2_VOICE_STATE state;

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

    XA2CALL(CreateSourceVoice, &src, &fmt, 0, 1.f, NULL, NULL, NULL);
    ok(hr == S_OK, "CreateSourceVoice failed: %08x\n", hr);

    memset(&buf, 0, sizeof(buf));
    buf.AudioBytes = 44100 * fmt.nBlockAlign;
    buf.pAudioData = HeapAlloc(GetProcessHeap(), 0, buf.AudioBytes);
    fill_buf((float*)buf.pAudioData, &fmt, 440, 44100);

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);

    hr = IXAudio2SourceVoice_Start(src, 0, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "Start failed: %08x\n", hr);


    XA2CALL(CreateSourceVoice, &src2, &fmt, 0, 1.f, NULL, NULL, NULL);
    ok(hr == S_OK, "CreateSourceVoice failed: %08x\n", hr);

    memset(&buf2, 0, sizeof(buf2));
    buf2.AudioBytes = 44100 * fmt.nBlockAlign;
    buf2.pAudioData = HeapAlloc(GetProcessHeap(), 0, buf2.AudioBytes);
    fill_buf((float*)buf2.pAudioData, &fmt, 220, 44100);

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src2, &buf2, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);

    hr = IXAudio2SourceVoice_Start(src2, 0, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "Start failed: %08x\n", hr);

    while(1){
        if(xaudio27)
            IXAudio27SourceVoice_GetState((IXAudio27SourceVoice*)src, &state);
        else
            IXAudio2SourceVoice_GetState(src, &state, 0);
        if(state.SamplesPlayed >= 44100)
            break;
        Sleep(100);
    }

    ok(state.SamplesPlayed == 44100, "Got wrong samples played\n");

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
    if(xa28dll)
        pXAudio2Create = (void*)GetProcAddress(xa28dll, "XAudio2Create");

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
        }else
            skip("No audio devices available\n");

        IXAudio2_Release(xa);
    }else
        win_skip("XAudio 2.8 not available\n");

    CoUninitialize();
}
