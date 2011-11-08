/*
 * Copyright 2010 Maarten Lankhorst for CodeWeavers
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

/* This test is for audio capture specific mechanisms
 * Tests:
 * - IAudioClient with eCapture and IAudioCaptureClient
 */

#include <math.h>

#include "wine/test.h"

#define COBJMACROS

#ifdef STANDALONE
#include "initguid.h"
#endif

#include "unknwn.h"
#include "uuids.h"
#include "mmdeviceapi.h"
#include "audioclient.h"

#define NULL_PTR_ERR MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, RPC_X_NULL_REF_POINTER)

static IMMDevice *dev = NULL;

static void test_uninitialized(IAudioClient *ac)
{
    HRESULT hr;
    UINT32 num;
    REFERENCE_TIME t1;

    HANDLE handle = CreateEventW(NULL, FALSE, FALSE, NULL);
    IUnknown *unk;

    hr = IAudioClient_GetBufferSize(ac, &num);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized GetBufferSize call returns %08x\n", hr);

    hr = IAudioClient_GetStreamLatency(ac, &t1);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized GetStreamLatency call returns %08x\n", hr);

    hr = IAudioClient_GetCurrentPadding(ac, &num);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized GetCurrentPadding call returns %08x\n", hr);

    hr = IAudioClient_Start(ac);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized Start call returns %08x\n", hr);

    hr = IAudioClient_Stop(ac);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized Stop call returns %08x\n", hr);

    hr = IAudioClient_Reset(ac);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized Reset call returns %08x\n", hr);

    hr = IAudioClient_SetEventHandle(ac, handle);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized SetEventHandle call returns %08x\n", hr);

    hr = IAudioClient_GetService(ac, &IID_IAudioStreamVolume, (void**)&unk);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized GetService call returns %08x\n", hr);

    CloseHandle(handle);
}

static void test_capture(IAudioClient *ac, HANDLE handle, WAVEFORMATEX *wfx)
{
    IAudioCaptureClient *acc;
    HRESULT hr;
    UINT32 frames = 0;
    BYTE *data = NULL;
    DWORD flags;
    UINT64 devpos, qpcpos;

    hr = IAudioClient_GetService(ac, &IID_IAudioCaptureClient, (void**)&acc);
    ok(hr == S_OK, "IAudioClient_GetService(IID_IAudioCaptureClient) returns %08x\n", hr);
    if (hr != S_OK)
        return;

    hr = IAudioCaptureClient_GetNextPacketSize(acc, NULL);
    ok(hr == E_POINTER, "IAudioCaptureClient_GetNextPacketSize(NULL) returns %08x\n", hr);

    ok(WaitForSingleObject(handle, 2000) == WAIT_OBJECT_0, "Waiting on event handle failed!\n");

    /* frames can be 0 if no data is available yet.. */
    hr = IAudioCaptureClient_GetNextPacketSize(acc, &frames);
    ok(hr == S_OK, "IAudioCaptureClient_GetNextPacketSize returns %08x\n", hr);

    data = (BYTE*)(DWORD_PTR)0xdeadbeef;
    hr = IAudioCaptureClient_GetBuffer(acc, &data, NULL, NULL, NULL, NULL);
    ok(hr == E_POINTER, "IAudioCaptureClient_GetBuffer(data, NULL, NULL) returns %08x\n", hr);
    ok((DWORD_PTR)data == 0xdeadbeef, "data is reset to %p\n", data);

    frames = 0xdeadbeef;
    hr = IAudioCaptureClient_GetBuffer(acc, NULL, &frames, NULL, NULL, NULL);
    ok(hr == E_POINTER, "IAudioCaptureClient_GetBuffer(NULL, &frames, NULL) returns %08x\n", hr);
    ok(frames == 0xdeadbeef, "frames is reset to %08x\n", frames);

    flags = 0xdeadbeef;
    hr = IAudioCaptureClient_GetBuffer(acc, NULL, NULL, &flags, NULL, NULL);
    ok(hr == E_POINTER, "IAudioCaptureClient_GetBuffer(NULL, NULL, &flags) returns %08x\n", hr);
    ok(flags == 0xdeadbeef, "flags is reset to %08x\n", flags);

    hr = IAudioCaptureClient_GetBuffer(acc, &data, &frames, NULL, NULL, NULL);
    ok(hr == E_POINTER, "IAudioCaptureClient_GetBuffer(&ata, &frames, NULL) returns %08x\n", hr);

    hr = IAudioCaptureClient_GetBuffer(acc, &data, &frames, &flags, &devpos, &qpcpos);
    ok(hr == S_OK || hr == AUDCLNT_S_BUFFER_EMPTY, "Valid IAudioCaptureClient_GetBuffer returns %08x\n", hr);
    if (hr == S_OK)
        ok(frames, "Amount of frames locked is 0!\n");
    else if (hr == AUDCLNT_S_BUFFER_EMPTY)
        ok(!frames, "Amount of frames locked with empty buffer is %u!\n", frames);
    else
        ok(0, "GetBuffer returned %08x\n", hr);
    trace("Device position is at %u, amount of frames locked: %u\n", (DWORD)devpos, frames);

    if (frames) {
        hr = IAudioCaptureClient_GetBuffer(acc, &data, &frames, &flags, &devpos, &qpcpos);
        ok(hr == AUDCLNT_E_OUT_OF_ORDER, "Out of order IAudioCaptureClient_GetBuffer returns %08x\n", hr);
    }

    hr = IAudioCaptureClient_ReleaseBuffer(acc, frames);
    ok(hr == S_OK, "Releasing buffer returns %08x\n", hr);

    if (frames) {
        hr = IAudioCaptureClient_ReleaseBuffer(acc, frames);
        ok(hr == AUDCLNT_E_OUT_OF_ORDER, "Releasing buffer twice returns %08x\n", hr);
    }

    IUnknown_Release(acc);
}

static void test_audioclient(void)
{
    IAudioClient *ac;
    IUnknown *unk;
    HRESULT hr;
    ULONG ref;
    WAVEFORMATEX *pwfx, *pwfx2;
    REFERENCE_TIME t1, t2;
    HANDLE handle;

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if(hr != S_OK)
        return;

    handle = CreateEventW(NULL, FALSE, FALSE, NULL);

    hr = IAudioClient_QueryInterface(ac, &IID_IUnknown, NULL);
    ok(hr == E_POINTER, "QueryInterface(NULL) returned %08x\n", hr);

    unk = (void*)(LONG_PTR)0x12345678;
    hr = IAudioClient_QueryInterface(ac, &IID_NULL, (void**)&unk);
    ok(hr == E_NOINTERFACE, "QueryInterface(IID_NULL) returned %08x\n", hr);
    ok(!unk, "QueryInterface(IID_NULL) returned non-null pointer %p\n", unk);

    hr = IAudioClient_QueryInterface(ac, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface(IID_IUnknown) returned %08x\n", hr);
    if (unk)
    {
        ref = IUnknown_Release(unk);
        ok(ref == 1, "Released count is %u\n", ref);
    }

    hr = IAudioClient_QueryInterface(ac, &IID_IAudioClient, (void**)&unk);
    ok(hr == S_OK, "QueryInterface(IID_IAudioClient) returned %08x\n", hr);
    if (unk)
    {
        ref = IUnknown_Release(unk);
        ok(ref == 1, "Released count is %u\n", ref);
    }

    hr = IAudioClient_GetDevicePeriod(ac, NULL, NULL);
    ok(hr == E_POINTER, "Invalid GetDevicePeriod call returns %08x\n", hr);

    hr = IAudioClient_GetDevicePeriod(ac, &t1, NULL);
    ok(hr == S_OK, "Valid GetDevicePeriod call returns %08x\n", hr);

    hr = IAudioClient_GetDevicePeriod(ac, NULL, &t2);
    ok(hr == S_OK, "Valid GetDevicePeriod call returns %08x\n", hr);

    hr = IAudioClient_GetDevicePeriod(ac, &t1, &t2);
    ok(hr == S_OK, "Valid GetDevicePeriod call returns %08x\n", hr);
    trace("Returned periods: %u.%04u ms %u.%04u ms\n",
          (UINT)(t1/10000), (UINT)(t1 % 10000),
          (UINT)(t2/10000), (UINT)(t2 % 10000));

    hr = IAudioClient_GetMixFormat(ac, NULL);
    ok(hr == E_POINTER, "GetMixFormat returns %08x\n", hr);

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "Valid GetMixFormat returns %08x\n", hr);

    if (hr == S_OK)
    {
        trace("pwfx: %p\n", pwfx);
        trace("Tag: %04x\n", pwfx->wFormatTag);
        trace("bits: %u\n", pwfx->wBitsPerSample);
        trace("chan: %u\n", pwfx->nChannels);
        trace("rate: %u\n", pwfx->nSamplesPerSec);
        trace("align: %u\n", pwfx->nBlockAlign);
        trace("extra: %u\n", pwfx->cbSize);
        ok(pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE, "wFormatTag is %x\n", pwfx->wFormatTag);
        if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        {
            WAVEFORMATEXTENSIBLE *pwfxe = (void*)pwfx;
            trace("Res: %u\n", pwfxe->Samples.wReserved);
            trace("Mask: %x\n", pwfxe->dwChannelMask);
            trace("Alg: %s\n",
                  IsEqualGUID(&pwfxe->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM)?"PCM":
                  (IsEqualGUID(&pwfxe->SubFormat,
                               &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)?"FLOAT":"Other"));
        }

        hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_SHARED, pwfx, &pwfx2);
        ok(hr == S_OK, "Valid IsFormatSupported(Shared) call returns %08x\n", hr);
        ok(pwfx2 == NULL, "pwfx2 is non-null\n");
        CoTaskMemFree(pwfx2);

        hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_SHARED, NULL, NULL);
        ok(hr == E_POINTER, "IsFormatSupported(NULL) call returns %08x\n", hr);

        hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_SHARED, pwfx, NULL);
        ok(hr == E_POINTER, "IsFormatSupported(Shared,NULL) call returns %08x\n", hr);

        hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_EXCLUSIVE, pwfx, NULL);
        ok(hr == S_OK || hr == AUDCLNT_E_UNSUPPORTED_FORMAT, "IsFormatSupported(Exclusive) call returns %08x\n", hr);

        hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_EXCLUSIVE, pwfx, &pwfx2);
        ok(hr == S_OK || hr == AUDCLNT_E_UNSUPPORTED_FORMAT, "IsFormatSupported(Exclusive) call returns %08x\n", hr);
        ok(pwfx2 == NULL, "pwfx2 non-null on exclusive IsFormatSupported\n");

        hr = IAudioClient_IsFormatSupported(ac, 0xffffffff, pwfx, NULL);
        ok(hr == E_INVALIDARG, "IsFormatSupported(0xffffffff) call returns %08x\n", hr);
    }

    test_uninitialized(ac);

    hr = IAudioClient_Initialize(ac, 3, 0, 5000000, 0, pwfx, NULL);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Initialize with invalid sharemode returns %08x\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0xffffffff, 5000000, 0, pwfx, NULL);
    ok(hr == E_INVALIDARG, "Initialize with invalid flags returns %08x\n", hr);

    /* It seems that if length > 2s or periodicity != 0 the length is ignored and call succeeds
     * Since we can only initialize successfully once skip those tests
     */
    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000, 0, NULL, NULL);
    ok(hr == E_POINTER, "Initialize with null format returns %08x\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 5000000, 0, pwfx, NULL);
    ok(hr == S_OK, "Valid Initialize returns %08x\n", hr);

    if (hr != S_OK)
    {
        skip("Cannot initialize %08x, remainder of tests is useless\n", hr);
        CoTaskMemFree(pwfx);
        return;
    }

    hr = IAudioClient_GetStreamLatency(ac, NULL);
    ok(hr == E_POINTER, "GetStreamLatency(NULL) call returns %08x\n", hr);

    hr = IAudioClient_GetStreamLatency(ac, &t1);
    ok(hr == S_OK, "Valid GetStreamLatency call returns %08x\n", hr);
    trace("Returned latency: %u.%04u ms\n",
          (UINT)(t1/10000), (UINT)(t1 % 10000));

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000, 0, pwfx, NULL);
    ok(hr == AUDCLNT_E_ALREADY_INITIALIZED, "Calling Initialize twice returns %08x\n", hr);

    hr = IAudioClient_SetEventHandle(ac, NULL);
    ok(hr == E_INVALIDARG, "SetEventHandle(NULL) returns %08x\n", hr);

    hr = IAudioClient_Start(ac);
    ok(hr == AUDCLNT_E_EVENTHANDLE_NOT_SET, "Start before SetEventHandle returns %08x\n", hr);

    hr = IAudioClient_SetEventHandle(ac, handle);
    ok(hr == S_OK, "SetEventHandle returns %08x\n", hr);

    hr = IAudioClient_Reset(ac);
    ok(hr == S_OK, "Reset on a resetted stream returns %08x\n", hr);

    hr = IAudioClient_Stop(ac);
    ok(hr == S_FALSE, "Stop on a stopped stream returns %08x\n", hr);

    hr = IAudioClient_Start(ac);
    ok(hr == S_OK, "Start on a stopped stream returns %08x\n", hr);

    test_capture(ac, handle, pwfx);

    IAudioClient_Release(ac);
    CloseHandle(handle);
    CoTaskMemFree(pwfx);
}

static void test_streamvolume(void)
{
    IAudioClient *ac;
    IAudioStreamVolume *asv;
    WAVEFORMATEX *fmt;
    UINT32 chans, i;
    HRESULT hr;
    float vol, *vols;

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &fmt);
    ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000,
            0, fmt, NULL);
    ok(hr == S_OK, "Initialize failed: %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetService(ac, &IID_IAudioStreamVolume, (void**)&asv);
    ok(hr == S_OK, "GetService failed: %08x\n", hr);

    hr = IAudioStreamVolume_GetChannelCount(asv, NULL);
    ok(hr == E_POINTER, "GetChannelCount gave wrong error: %08x\n", hr);

    hr = IAudioStreamVolume_GetChannelCount(asv, &chans);
    ok(hr == S_OK, "GetChannelCount failed: %08x\n", hr);
    ok(chans == fmt->nChannels, "GetChannelCount gave wrong number of channels: %d\n", chans);

    hr = IAudioStreamVolume_GetChannelVolume(asv, fmt->nChannels, NULL);
    ok(hr == E_POINTER, "GetChannelCount gave wrong error: %08x\n", hr);

    hr = IAudioStreamVolume_GetChannelVolume(asv, fmt->nChannels, &vol);
    ok(hr == E_INVALIDARG, "GetChannelCount gave wrong error: %08x\n", hr);

    hr = IAudioStreamVolume_GetChannelVolume(asv, 0, NULL);
    ok(hr == E_POINTER, "GetChannelCount gave wrong error: %08x\n", hr);

    hr = IAudioStreamVolume_GetChannelVolume(asv, 0, &vol);
    ok(hr == S_OK, "GetChannelCount failed: %08x\n", hr);
    ok(vol == 1.f, "Channel volume was not 1: %f\n", vol);

    hr = IAudioStreamVolume_SetChannelVolume(asv, fmt->nChannels, -1.f);
    ok(hr == E_INVALIDARG, "SetChannelVolume gave wrong error: %08x\n", hr);

    hr = IAudioStreamVolume_SetChannelVolume(asv, 0, -1.f);
    ok(hr == E_INVALIDARG, "SetChannelVolume gave wrong error: %08x\n", hr);

    hr = IAudioStreamVolume_SetChannelVolume(asv, 0, 2.f);
    ok(hr == E_INVALIDARG, "SetChannelVolume gave wrong error: %08x\n", hr);

    hr = IAudioStreamVolume_SetChannelVolume(asv, 0, 0.2f);
    ok(hr == S_OK, "SetChannelVolume failed: %08x\n", hr);

    hr = IAudioStreamVolume_GetChannelVolume(asv, 0, &vol);
    ok(hr == S_OK, "GetChannelCount failed: %08x\n", hr);
    ok(fabsf(vol - 0.2f) < 0.05f, "Channel volume wasn't 0.2: %f\n", vol);

    hr = IAudioStreamVolume_GetAllVolumes(asv, 0, NULL);
    ok(hr == E_POINTER, "GetAllVolumes gave wrong error: %08x\n", hr);

    hr = IAudioStreamVolume_GetAllVolumes(asv, fmt->nChannels, NULL);
    ok(hr == E_POINTER, "GetAllVolumes gave wrong error: %08x\n", hr);

    vols = HeapAlloc(GetProcessHeap(), 0, fmt->nChannels * sizeof(float));
    ok(vols != NULL, "HeapAlloc failed\n");

    hr = IAudioStreamVolume_GetAllVolumes(asv, fmt->nChannels - 1, vols);
    ok(hr == E_INVALIDARG, "GetAllVolumes gave wrong error: %08x\n", hr);

    hr = IAudioStreamVolume_GetAllVolumes(asv, fmt->nChannels, vols);
    ok(hr == S_OK, "GetAllVolumes failed: %08x\n", hr);
    ok(fabsf(vols[0] - 0.2f) < 0.05f, "Channel 0 volume wasn't 0.2: %f\n", vol);
    for(i = 1; i < fmt->nChannels; ++i)
        ok(vols[i] == 1.f, "Channel %d volume is not 1: %f\n", i, vols[i]);

    hr = IAudioStreamVolume_SetAllVolumes(asv, 0, NULL);
    ok(hr == E_POINTER, "SetAllVolumes gave wrong error: %08x\n", hr);

    hr = IAudioStreamVolume_SetAllVolumes(asv, fmt->nChannels, NULL);
    ok(hr == E_POINTER, "SetAllVolumes gave wrong error: %08x\n", hr);

    hr = IAudioStreamVolume_SetAllVolumes(asv, fmt->nChannels - 1, vols);
    ok(hr == E_INVALIDARG, "SetAllVolumes gave wrong error: %08x\n", hr);

    hr = IAudioStreamVolume_SetAllVolumes(asv, fmt->nChannels, vols);
    ok(hr == S_OK, "SetAllVolumes failed: %08x\n", hr);

    HeapFree(GetProcessHeap(), 0, vols);
    IAudioStreamVolume_Release(asv);
    IAudioClient_Release(ac);
    CoTaskMemFree(fmt);
}

static void test_channelvolume(void)
{
    IAudioClient *ac;
    IChannelAudioVolume *acv;
    WAVEFORMATEX *fmt;
    UINT32 chans, i;
    HRESULT hr;
    float vol, *vols;

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &fmt);
    ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_NOPERSIST, 5000000, 0, fmt, NULL);
    ok(hr == S_OK, "Initialize failed: %08x\n", hr);

    hr = IAudioClient_GetService(ac, &IID_IChannelAudioVolume, (void**)&acv);
    ok(hr == S_OK, "GetService failed: %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IChannelAudioVolume_GetChannelCount(acv, NULL);
    ok(hr == NULL_PTR_ERR, "GetChannelCount gave wrong error: %08x\n", hr);

    hr = IChannelAudioVolume_GetChannelCount(acv, &chans);
    ok(hr == S_OK, "GetChannelCount failed: %08x\n", hr);
    ok(chans == fmt->nChannels, "GetChannelCount gave wrong number of channels: %d\n", chans);

    hr = IChannelAudioVolume_GetChannelVolume(acv, fmt->nChannels, NULL);
    ok(hr == NULL_PTR_ERR, "GetChannelCount gave wrong error: %08x\n", hr);

    hr = IChannelAudioVolume_GetChannelVolume(acv, fmt->nChannels, &vol);
    ok(hr == E_INVALIDARG, "GetChannelCount gave wrong error: %08x\n", hr);

    hr = IChannelAudioVolume_GetChannelVolume(acv, 0, NULL);
    ok(hr == NULL_PTR_ERR, "GetChannelCount gave wrong error: %08x\n", hr);

    hr = IChannelAudioVolume_GetChannelVolume(acv, 0, &vol);
    ok(hr == S_OK, "GetChannelCount failed: %08x\n", hr);
    ok(vol == 1.f, "Channel volume was not 1: %f\n", vol);

    hr = IChannelAudioVolume_SetChannelVolume(acv, fmt->nChannels, -1.f, NULL);
    ok(hr == E_INVALIDARG, "SetChannelVolume gave wrong error: %08x\n", hr);

    hr = IChannelAudioVolume_SetChannelVolume(acv, 0, -1.f, NULL);
    ok(hr == E_INVALIDARG, "SetChannelVolume gave wrong error: %08x\n", hr);

    hr = IChannelAudioVolume_SetChannelVolume(acv, 0, 2.f, NULL);
    ok(hr == E_INVALIDARG, "SetChannelVolume gave wrong error: %08x\n", hr);

    hr = IChannelAudioVolume_SetChannelVolume(acv, 0, 0.2f, NULL);
    ok(hr == S_OK, "SetChannelVolume failed: %08x\n", hr);

    hr = IChannelAudioVolume_GetChannelVolume(acv, 0, &vol);
    ok(hr == S_OK, "GetChannelCount failed: %08x\n", hr);
    ok(fabsf(vol - 0.2f) < 0.05f, "Channel volume wasn't 0.2: %f\n", vol);

    hr = IChannelAudioVolume_GetAllVolumes(acv, 0, NULL);
    ok(hr == NULL_PTR_ERR, "GetAllVolumes gave wrong error: %08x\n", hr);

    hr = IChannelAudioVolume_GetAllVolumes(acv, fmt->nChannels, NULL);
    ok(hr == NULL_PTR_ERR, "GetAllVolumes gave wrong error: %08x\n", hr);

    vols = HeapAlloc(GetProcessHeap(), 0, fmt->nChannels * sizeof(float));
    ok(vols != NULL, "HeapAlloc failed\n");

    hr = IChannelAudioVolume_GetAllVolumes(acv, fmt->nChannels - 1, vols);
    ok(hr == E_INVALIDARG, "GetAllVolumes gave wrong error: %08x\n", hr);

    hr = IChannelAudioVolume_GetAllVolumes(acv, fmt->nChannels, vols);
    ok(hr == S_OK, "GetAllVolumes failed: %08x\n", hr);
    ok(fabsf(vols[0] - 0.2f) < 0.05f, "Channel 0 volume wasn't 0.2: %f\n", vol);
    for(i = 1; i < fmt->nChannels; ++i)
        ok(vols[i] == 1.f, "Channel %d volume is not 1: %f\n", i, vols[i]);

    hr = IChannelAudioVolume_SetAllVolumes(acv, 0, NULL, NULL);
    ok(hr == NULL_PTR_ERR, "SetAllVolumes gave wrong error: %08x\n", hr);

    hr = IChannelAudioVolume_SetAllVolumes(acv, fmt->nChannels, NULL, NULL);
    ok(hr == NULL_PTR_ERR, "SetAllVolumes gave wrong error: %08x\n", hr);

    hr = IChannelAudioVolume_SetAllVolumes(acv, fmt->nChannels - 1, vols, NULL);
    ok(hr == E_INVALIDARG, "SetAllVolumes gave wrong error: %08x\n", hr);

    hr = IChannelAudioVolume_SetAllVolumes(acv, fmt->nChannels, vols, NULL);
    ok(hr == S_OK, "SetAllVolumes failed: %08x\n", hr);

    hr = IChannelAudioVolume_SetChannelVolume(acv, 0, 1.0f, NULL);
    ok(hr == S_OK, "SetChannelVolume failed: %08x\n", hr);

    HeapFree(GetProcessHeap(), 0, vols);
    IChannelAudioVolume_Release(acv);
    IAudioClient_Release(ac);
    CoTaskMemFree(fmt);
}

static void test_simplevolume(void)
{
    IAudioClient *ac;
    ISimpleAudioVolume *sav;
    WAVEFORMATEX *fmt;
    HRESULT hr;
    float vol;
    BOOL mute;

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &fmt);
    ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_NOPERSIST, 5000000, 0, fmt, NULL);
    ok(hr == S_OK, "Initialize failed: %08x\n", hr);

    hr = IAudioClient_GetService(ac, &IID_ISimpleAudioVolume, (void**)&sav);
    ok(hr == S_OK, "GetService failed: %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = ISimpleAudioVolume_GetMasterVolume(sav, NULL);
    ok(hr == NULL_PTR_ERR, "GetMasterVolume gave wrong error: %08x\n", hr);

    hr = ISimpleAudioVolume_GetMasterVolume(sav, &vol);
    ok(hr == S_OK, "GetMasterVolume failed: %08x\n", hr);
    ok(vol == 1.f, "Master volume wasn't 1: %f\n", vol);

    hr = ISimpleAudioVolume_SetMasterVolume(sav, -1.f, NULL);
    ok(hr == E_INVALIDARG, "SetMasterVolume gave wrong error: %08x\n", hr);

    hr = ISimpleAudioVolume_SetMasterVolume(sav, 2.f, NULL);
    ok(hr == E_INVALIDARG, "SetMasterVolume gave wrong error: %08x\n", hr);

    hr = ISimpleAudioVolume_SetMasterVolume(sav, 0.2f, NULL);
    ok(hr == S_OK, "SetMasterVolume failed: %08x\n", hr);

    hr = ISimpleAudioVolume_GetMasterVolume(sav, &vol);
    ok(hr == S_OK, "GetMasterVolume failed: %08x\n", hr);
    ok(fabsf(vol - 0.2f) < 0.05f, "Master volume wasn't 0.2: %f\n", vol);

    hr = ISimpleAudioVolume_GetMute(sav, NULL);
    ok(hr == NULL_PTR_ERR, "GetMute gave wrong error: %08x\n", hr);

    mute = TRUE;
    hr = ISimpleAudioVolume_GetMute(sav, &mute);
    ok(hr == S_OK, "GetMute failed: %08x\n", hr);
    ok(mute == FALSE, "Session is already muted\n");

    hr = ISimpleAudioVolume_SetMute(sav, TRUE, NULL);
    ok(hr == S_OK, "SetMute failed: %08x\n", hr);

    mute = FALSE;
    hr = ISimpleAudioVolume_GetMute(sav, &mute);
    ok(hr == S_OK, "GetMute failed: %08x\n", hr);
    ok(mute == TRUE, "Session should have been muted\n");

    hr = ISimpleAudioVolume_GetMasterVolume(sav, &vol);
    ok(hr == S_OK, "GetMasterVolume failed: %08x\n", hr);
    ok(fabsf(vol - 0.2f) < 0.05f, "Master volume wasn't 0.2: %f\n", vol);

    hr = ISimpleAudioVolume_SetMasterVolume(sav, 1.f, NULL);
    ok(hr == S_OK, "SetMasterVolume failed: %08x\n", hr);

    mute = FALSE;
    hr = ISimpleAudioVolume_GetMute(sav, &mute);
    ok(hr == S_OK, "GetMute failed: %08x\n", hr);
    ok(mute == TRUE, "Session should have been muted\n");

    hr = ISimpleAudioVolume_SetMute(sav, FALSE, NULL);
    ok(hr == S_OK, "SetMute failed: %08x\n", hr);

    ISimpleAudioVolume_Release(sav);
    IAudioClient_Release(ac);
    CoTaskMemFree(fmt);
}

static void test_volume_dependence(void)
{
    IAudioClient *ac, *ac2;
    ISimpleAudioVolume *sav;
    IChannelAudioVolume *cav;
    IAudioStreamVolume *asv;
    WAVEFORMATEX *fmt;
    HRESULT hr;
    float vol;
    GUID session;
    UINT32 nch;

    hr = CoCreateGuid(&session);
    ok(hr == S_OK, "CoCreateGuid failed: %08x\n", hr);

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);

    hr = IAudioClient_GetMixFormat(ac, &fmt);
    ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_NOPERSIST, 5000000, 0, fmt, &session);
    ok(hr == S_OK, "Initialize failed: %08x\n", hr);

    hr = IAudioClient_GetService(ac, &IID_ISimpleAudioVolume, (void**)&sav);
    ok(hr == S_OK, "GetService (SimpleAudioVolume) failed: %08x\n", hr);

    hr = IAudioClient_GetService(ac, &IID_IChannelAudioVolume, (void**)&cav);
    ok(hr == S_OK, "GetService (ChannelAudioVolme) failed: %08x\n", hr);

    hr = IAudioClient_GetService(ac, &IID_IAudioStreamVolume, (void**)&asv);
    ok(hr == S_OK, "GetService (AudioStreamVolume) failed: %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioStreamVolume_SetChannelVolume(asv, 0, 0.2f);
    ok(hr == S_OK, "ASV_SetChannelVolume failed: %08x\n", hr);

    hr = IChannelAudioVolume_SetChannelVolume(cav, 0, 0.4f, NULL);
    ok(hr == S_OK, "CAV_SetChannelVolume failed: %08x\n", hr);

    hr = ISimpleAudioVolume_SetMasterVolume(sav, 0.6f, NULL);
    ok(hr == S_OK, "SAV_SetMasterVolume failed: %08x\n", hr);

    hr = IAudioStreamVolume_GetChannelVolume(asv, 0, &vol);
    ok(hr == S_OK, "ASV_GetChannelVolume failed: %08x\n", hr);
    ok(fabsf(vol - 0.2) < 0.05f, "ASV_GetChannelVolume gave wrong volume: %f\n", vol);

    hr = IChannelAudioVolume_GetChannelVolume(cav, 0, &vol);
    ok(hr == S_OK, "CAV_GetChannelVolume failed: %08x\n", hr);
    ok(fabsf(vol - 0.4) < 0.05f, "CAV_GetChannelVolume gave wrong volume: %f\n", vol);

    hr = ISimpleAudioVolume_GetMasterVolume(sav, &vol);
    ok(hr == S_OK, "SAV_GetMasterVolume failed: %08x\n", hr);
    ok(fabsf(vol - 0.6) < 0.05f, "SAV_GetMasterVolume gave wrong volume: %f\n", vol);

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac2);
    if(SUCCEEDED(hr)){
        IChannelAudioVolume *cav2;
        IAudioStreamVolume *asv2;

        hr = IAudioClient_Initialize(ac2, AUDCLNT_SHAREMODE_SHARED,
                AUDCLNT_STREAMFLAGS_NOPERSIST, 5000000, 0, fmt, &session);
        ok(hr == S_OK, "Initialize failed: %08x\n", hr);

        hr = IAudioClient_GetService(ac2, &IID_IChannelAudioVolume, (void**)&cav2);
        ok(hr == S_OK, "GetService failed: %08x\n", hr);

        hr = IAudioClient_GetService(ac2, &IID_IAudioStreamVolume, (void**)&asv2);
        ok(hr == S_OK, "GetService failed: %08x\n", hr);

        hr = IChannelAudioVolume_GetChannelVolume(cav2, 0, &vol);
        ok(hr == S_OK, "CAV_GetChannelVolume failed: %08x\n", hr);
        ok(fabsf(vol - 0.4) < 0.05f, "CAV_GetChannelVolume gave wrong volume: %f\n", vol);

        hr = IAudioStreamVolume_GetChannelVolume(asv2, 0, &vol);
        ok(hr == S_OK, "ASV_GetChannelVolume failed: %08x\n", hr);
        ok(vol == 1.f, "ASV_GetChannelVolume gave wrong volume: %f\n", vol);

        hr = IChannelAudioVolume_GetChannelCount(cav2, &nch);
        ok(hr == S_OK, "GetChannelCount failed: %08x\n", hr);
        ok(nch == fmt->nChannels, "Got wrong channel count, expected %u: %u\n", fmt->nChannels, nch);

        hr = IAudioStreamVolume_GetChannelCount(asv2, &nch);
        ok(hr == S_OK, "GetChannelCount failed: %08x\n", hr);
        ok(nch == fmt->nChannels, "Got wrong channel count, expected %u: %u\n", fmt->nChannels, nch);

        IAudioStreamVolume_Release(asv2);
        IChannelAudioVolume_Release(cav2);
        IAudioClient_Release(ac2);
    }else
        skip("Unable to open the same device twice. Skipping session volume control tests\n");

    hr = IChannelAudioVolume_SetChannelVolume(cav, 0, 1.f, NULL);
    ok(hr == S_OK, "CAV_SetChannelVolume failed: %08x\n", hr);

    hr = ISimpleAudioVolume_SetMasterVolume(sav, 1.f, NULL);
    ok(hr == S_OK, "SAV_SetMasterVolume failed: %08x\n", hr);

    CoTaskMemFree(fmt);
    ISimpleAudioVolume_Release(sav);
    IChannelAudioVolume_Release(cav);
    IAudioStreamVolume_Release(asv);
    IAudioClient_Release(ac);
}

START_TEST(capture)
{
    HRESULT hr;
    IMMDeviceEnumerator *mme = NULL;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, (void**)&mme);
    if (FAILED(hr))
    {
        skip("mmdevapi not available: 0x%08x\n", hr);
        goto cleanup;
    }

    hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(mme, eCapture, eMultimedia, &dev);
    ok(hr == S_OK || hr == E_NOTFOUND, "GetDefaultAudioEndpoint failed: 0x%08x\n", hr);
    if (hr != S_OK || !dev)
    {
        if (hr == E_NOTFOUND)
            skip("No sound card available\n");
        else
            skip("GetDefaultAudioEndpoint returns 0x%08x\n", hr);
        goto cleanup;
    }

    test_audioclient();
    test_streamvolume();
    test_channelvolume();
    test_simplevolume();
    test_volume_dependence();

    IMMDevice_Release(dev);

cleanup:
    if (mme)
        IMMDeviceEnumerator_Release(mme);
    CoUninitialize();
}
