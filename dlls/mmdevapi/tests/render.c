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

/* This test is for audio playback specific mechanisms
 * Tests:
 * - IAudioClient with eRender and IAudioRenderClient
 */

#include "wine/test.h"

#define COBJMACROS

#ifdef STANDALONE
#include "initguid.h"
#endif

#include "unknwn.h"
#include "uuids.h"
#include "mmdeviceapi.h"
#include "audioclient.h"

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
    trace("Returned periods: %u.%05u ms %u.%05u ms\n",
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
        ok(hr == E_INVALIDARG ||
           hr == AUDCLNT_E_UNSUPPORTED_FORMAT,
           "IsFormatSupported(0xffffffff) call returns %08x\n", hr);
    }

    test_uninitialized(ac);

    hr = IAudioClient_Initialize(ac, 3, 0, 5000000, 0, pwfx, NULL);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Initialize with invalid sharemode returns %08x\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0xffffffff, 5000000, 0, pwfx, NULL);
    ok(hr == E_INVALIDARG, "Initialize with invalid flags returns %08x\n", hr);

    /* It seems that if length > 2s or periodicity != 0 the length is ignored and call succeeds
     * Since we can only initialize successfully once, skip those tests.
     */
    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000, 0, NULL, NULL);
    ok(hr == E_POINTER, "Initialize with null format returns %08x\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000, 0, pwfx, NULL);
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
    trace("Returned latency: %u.%05u ms\n",
          (UINT)(t1/10000), (UINT)(t1 % 10000));

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000, 0, pwfx, NULL);
    ok(hr == AUDCLNT_E_ALREADY_INITIALIZED, "Calling Initialize twice returns %08x\n", hr);

    hr = IAudioClient_SetEventHandle(ac, NULL);
    ok(hr == E_INVALIDARG, "SetEventHandle(NULL) returns %08x\n", hr);

    hr = IAudioClient_SetEventHandle(ac, handle);
    ok(hr == AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED ||
       broken(hr == HRESULT_FROM_WIN32(ERROR_INVALID_NAME)) ||
       broken(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) /* Some 2k8 */ ||
       broken(hr == HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME)) /* Some Vista */
       , "SetEventHandle returns %08x\n", hr);

    hr = IAudioClient_Reset(ac);
    ok(hr == S_OK, "Reset on a resetted stream returns %08x\n", hr);

    hr = IAudioClient_Stop(ac);
    ok(hr == S_FALSE, "Stop on a stopped stream returns %08x\n", hr);

    hr = IAudioClient_Start(ac);
    ok(hr == S_OK, "Start on a stopped stream returns %08x\n", hr);

    IAudioClient_Release(ac);

    CloseHandle(handle);
    CoTaskMemFree(pwfx);
}

static void test_references(void)
{
    IAudioClient *ac;
    IAudioRenderClient *rc;
    ISimpleAudioVolume *sav;
    IAudioClock *acl;
    WAVEFORMATEX *pwfx;
    HRESULT hr;
    ULONG ref;

    /* IAudioRenderClient */
    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000,
            0, pwfx, NULL);
    ok(hr == S_OK, "Initialize failed: %08x\n", hr);

    CoTaskMemFree(pwfx);

    hr = IAudioClient_GetService(ac, &IID_IAudioRenderClient, (void**)&rc);
    ok(hr == S_OK, "GetService failed: %08x\n", hr);

    IAudioRenderClient_AddRef(rc);
    ref = IAudioRenderClient_Release(rc);
    ok(ref != 0, "RenderClient_Release gave wrong refcount: %u\n", ref);

    ref = IAudioClient_Release(ac);
    ok(ref != 0, "Client_Release gave wrong refcount: %u\n", ref);

    ref = IAudioRenderClient_Release(rc);
    ok(ref == 0, "RenderClient_Release gave wrong refcount: %u\n", ref);

    /* ISimpleAudioVolume */
    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000,
            0, pwfx, NULL);
    ok(hr == S_OK, "Initialize failed: %08x\n", hr);

    CoTaskMemFree(pwfx);

    hr = IAudioClient_GetService(ac, &IID_ISimpleAudioVolume, (void**)&sav);
    ok(hr == S_OK, "GetService failed: %08x\n", hr);

    ISimpleAudioVolume_AddRef(sav);
    ref = ISimpleAudioVolume_Release(sav);
    ok(ref != 0, "SimpleAudioVolume_Release gave wrong refcount: %u\n", ref);

    ref = IAudioClient_Release(ac);
    ok(ref != 0, "Client_Release gave wrong refcount: %u\n", ref);

    ref = ISimpleAudioVolume_Release(sav);
    ok(ref == 0, "SimpleAudioVolume_Release gave wrong refcount: %u\n", ref);

    /* IAudioClock */
    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000,
            0, pwfx, NULL);
    ok(hr == S_OK, "Initialize failed: %08x\n", hr);

    CoTaskMemFree(pwfx);

    hr = IAudioClient_GetService(ac, &IID_IAudioClock, (void**)&acl);
    ok(hr == S_OK, "GetService failed: %08x\n", hr);

    IAudioClock_AddRef(acl);
    ref = IAudioClock_Release(acl);
    ok(ref != 0, "AudioClock_Release gave wrong refcount: %u\n", ref);

    ref = IAudioClient_Release(ac);
    ok(ref != 0, "Client_Release gave wrong refcount: %u\n", ref);

    ref = IAudioClock_Release(acl);
    ok(ref == 0, "AudioClock_Release gave wrong refcount: %u\n", ref);
}

static void test_event(void)
{
    HANDLE event;
    HRESULT hr;
    IAudioClient *ac;
    WAVEFORMATEX *pwfx;

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 5000000,
            0, pwfx, NULL);
    ok(hr == S_OK, "Initialize failed: %08x\n", hr);

    CoTaskMemFree(pwfx);

    event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(event != NULL, "CreateEvent failed\n");

    hr = IAudioClient_Start(ac);
    ok(hr == AUDCLNT_E_EVENTHANDLE_NOT_SET, "Start failed: %08x\n", hr);

    hr = IAudioClient_SetEventHandle(ac, event);
    ok(hr == S_OK, "SetEventHandle failed: %08x\n", hr);

    hr = IAudioClient_Start(ac);
    ok(hr == S_OK, "Start failed: %08x\n", hr);

    hr = IAudioClient_Stop(ac);
    ok(hr == S_OK, "Start failed: %08x\n", hr);

    /* test releasing a playing stream */
    hr = IAudioClient_Start(ac);
    ok(hr == S_OK, "Start failed: %08x\n", hr);
    IAudioClient_Release(ac);

    CloseHandle(event);
}

static void test_padding(void)
{
    HRESULT hr;
    IAudioClient *ac;
    IAudioRenderClient *arc;
    WAVEFORMATEX *pwfx;
    REFERENCE_TIME minp, defp;
    BYTE *buf;
    UINT32 psize, pad, written;

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED,
            0, 5000000, 0, pwfx, NULL);
    ok(hr == S_OK, "Initialize failed: %08x\n", hr);

    hr = IAudioClient_GetDevicePeriod(ac, &defp, &minp);
    ok(hr == S_OK, "GetDevicePeriod failed: %08x\n", hr);
    ok(defp != 0, "Default period is 0\n");
    ok(minp != 0, "Minimum period is 0\n");
    ok(minp <= defp, "Mininum period is greater than default period\n");

    hr = IAudioClient_GetService(ac, &IID_IAudioRenderClient, (void**)&arc);
    ok(hr == S_OK, "GetService failed: %08x\n", hr);

    psize = (defp / 10000000.) * pwfx->nSamplesPerSec * pwfx->nBlockAlign;

    written = 0;
    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding failed: %08x\n", hr);
    ok(pad == written, "GetCurrentPadding returned %u, should be %u\n", pad, written);

    hr = IAudioRenderClient_GetBuffer(arc, psize, &buf);
    ok(hr == S_OK, "GetBuffer failed: %08x\n", hr);
    ok(buf != NULL, "NULL buffer returned\n");

    hr = IAudioRenderClient_ReleaseBuffer(arc, psize,
            AUDCLNT_BUFFERFLAGS_SILENT);
    ok(hr == S_OK, "ReleaseBuffer failed: %08x\n", hr);
    written += psize;

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding failed: %08x\n", hr);
    ok(pad == written, "GetCurrentPadding returned %u, should be %u\n", pad, written);

    psize = (minp / 10000000.) * pwfx->nSamplesPerSec * pwfx->nBlockAlign;

    hr = IAudioRenderClient_GetBuffer(arc, psize, &buf);
    ok(hr == S_OK, "GetBuffer failed: %08x\n", hr);
    ok(buf != NULL, "NULL buffer returned\n");

    hr = IAudioRenderClient_ReleaseBuffer(arc, psize,
            AUDCLNT_BUFFERFLAGS_SILENT);
    ok(hr == S_OK, "ReleaseBuffer failed: %08x\n", hr);
    written += psize;

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding failed: %08x\n", hr);
    ok(pad == written, "GetCurrentPadding returned %u, should be %u\n", pad, written);

    /* overfull buffer. requested 1/2s buffer size, so try
     * to get a 1/2s buffer, which should fail */
    psize = pwfx->nSamplesPerSec / 2.;
    hr = IAudioRenderClient_GetBuffer(arc, psize, &buf);
    ok(hr == AUDCLNT_E_BUFFER_TOO_LARGE, "GetBuffer gave wrong error: %08x\n", hr);

    hr = IAudioRenderClient_ReleaseBuffer(arc, psize, 0);
    ok(hr == AUDCLNT_E_OUT_OF_ORDER, "ReleaseBuffer gave wrong error: %08x\n", hr);

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding failed: %08x\n", hr);
    ok(pad == written, "GetCurrentPadding returned %u, should be %u\n", pad, written);

    CoTaskMemFree(pwfx);

    IAudioRenderClient_Release(arc);
    IAudioClient_Release(ac);
}

static void test_clock(void)
{
    HRESULT hr;
    IAudioClient *ac;
    IAudioClock *acl;
    IAudioRenderClient *arc;
    UINT64 freq, pos, pcpos, last;
    BYTE *data;
    WAVEFORMATEX *pwfx;

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED,
            0, 5000000, 0, pwfx, NULL);
    ok(hr == S_OK, "Initialize failed: %08x\n", hr);

    hr = IAudioClient_GetService(ac, &IID_IAudioClock, (void**)&acl);
    ok(hr == S_OK, "GetService(IAudioClock) failed: %08x\n", hr);

    hr = IAudioClock_GetFrequency(acl, &freq);
    ok(hr == S_OK, "GetFrequency failed: %08x\n", hr);

    hr = IAudioClock_GetPosition(acl, NULL, NULL);
    ok(hr == E_POINTER, "GetPosition wrong error: %08x\n", hr);

    pcpos = 0;
    hr = IAudioClock_GetPosition(acl, &pos, &pcpos);
    ok(hr == S_OK, "GetPosition failed: %08x\n", hr);
    ok(pos == 0, "GetPosition returned non-zero pos before being started\n");
    ok(pcpos != 0, "GetPosition returned zero pcpos\n");
    last = pos;

    hr = IAudioClient_GetService(ac, &IID_IAudioRenderClient, (void**)&arc);
    ok(hr == S_OK, "GetService(IAudioRenderClient) failed: %08x\n", hr);

    hr = IAudioRenderClient_GetBuffer(arc, pwfx->nSamplesPerSec / 2., &data);
    ok(hr == S_OK, "GetBuffer failed: %08x\n", hr);

    hr = IAudioRenderClient_ReleaseBuffer(arc, pwfx->nSamplesPerSec / 2., AUDCLNT_BUFFERFLAGS_SILENT);
    ok(hr == S_OK, "ReleaseBuffer failed: %08x\n", hr);

    hr = IAudioClock_GetPosition(acl, &pos, NULL);
    ok(hr == S_OK, "GetPosition failed: %08x\n", hr);
    ok(pos == 0, "GetPosition returned non-zero pos before being started\n");

    hr = IAudioClient_Start(ac);
    ok(hr == S_OK, "Start failed: %08x\n", hr);

    Sleep(100);

    hr = IAudioClock_GetPosition(acl, &pos, NULL);
    ok(hr == S_OK, "GetPosition failed: %08x\n", hr);
    ok(pos > 0, "Position should have been further along...\n");
    last = pos;

    hr = IAudioClient_Stop(ac);
    ok(hr == S_OK, "Stop failed: %08x\n", hr);

    hr = IAudioClock_GetPosition(acl, &pos, NULL);
    ok(hr == S_OK, "GetPosition failed: %08x\n", hr);
    ok(pos >= last, "Position should have been further along...\n");
    last = pos;

    hr = IAudioClient_Start(ac);
    ok(hr == S_OK, "Start failed: %08x\n", hr);

    Sleep(100);

    hr = IAudioClient_Stop(ac);
    ok(hr == S_OK, "Stop failed: %08x\n", hr);

    hr = IAudioClock_GetPosition(acl, &pos, NULL);
    ok(hr == S_OK, "GetPosition failed: %08x\n", hr);
    ok(pos >= last, "Position should have been further along...\n");
    last = pos;

    hr = IAudioClock_GetPosition(acl, &pos, NULL);
    ok(hr == S_OK, "GetPosition failed: %08x\n", hr);
    ok(pos == last, "Position should have been further along...\n");

    hr = IAudioClient_Reset(ac);
    ok(hr == S_OK, "Reset failed: %08x\n", hr);

    hr = IAudioClock_GetPosition(acl, &pos, NULL);
    ok(hr == S_OK, "GetPosition failed: %08x\n", hr);
    ok(pos == 0, "GetPosition returned non-zero pos after Reset\n");
    last = pos;

    hr = IAudioRenderClient_GetBuffer(arc, pwfx->nSamplesPerSec / 2., &data);
    ok(hr == S_OK, "GetBuffer failed: %08x\n", hr);

    hr = IAudioRenderClient_ReleaseBuffer(arc, pwfx->nSamplesPerSec / 2., AUDCLNT_BUFFERFLAGS_SILENT);
    ok(hr == S_OK, "ReleaseBuffer failed: %08x\n", hr);

    hr = IAudioClock_GetPosition(acl, &pos, NULL);
    ok(hr == S_OK, "GetPosition failed: %08x\n", hr);
    ok(pos == 0, "GetPosition returned non-zero pos after Reset\n");
    last = pos;

    hr = IAudioClient_Start(ac);
    ok(hr == S_OK, "Start failed: %08x\n", hr);

    Sleep(100);

    hr = IAudioClock_GetPosition(acl, &pos, NULL);
    ok(hr == S_OK, "GetPosition failed: %08x\n", hr);
    ok(pos > last, "Position should have been further along...\n");

    hr = IAudioClient_Stop(ac);
    ok(hr == S_OK, "Stop failed: %08x\n", hr);

    hr = IAudioClock_GetPosition(acl, &pos, NULL);
    ok(hr == S_OK, "GetPosition failed: %08x\n", hr);
    ok(pos >= last, "Position should have been further along...\n");

    IAudioClock_Release(acl);
    IAudioClient_Release(ac);
}

START_TEST(render)
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

    hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(mme, eRender, eMultimedia, &dev);
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
    test_references();
    test_event();
    test_padding();
    test_clock();

    IMMDevice_Release(dev);

cleanup:
    if (mme)
        IMMDeviceEnumerator_Release(mme);
    CoUninitialize();
}
