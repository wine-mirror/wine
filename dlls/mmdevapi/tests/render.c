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

#include <math.h>
#include <stdio.h>

#include "wine/test.h"

#define COBJMACROS

#ifdef STANDALONE
#include "initguid.h"
#endif

#include "unknwn.h"
#include "uuids.h"
#include "mmdeviceapi.h"
#include "audioclient.h"
#include "audiopolicy.h"

#define NULL_PTR_ERR MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, RPC_X_NULL_REF_POINTER)

static IMMDeviceEnumerator *mme = NULL;
static IMMDevice *dev = NULL;

static inline const char *dbgstr_guid( const GUID *id )
{
    static char ret[256];
    sprintf(ret, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                             id->Data1, id->Data2, id->Data3,
                             id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
                             id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7] );
    return ret;
}

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

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 0, 0, pwfx, NULL);
    ok(hr == S_OK, "Initialize with 0 buffer size returns %08x\n", hr);

    IAudioClient_Release(ac);

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);

    if(pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE){
        WAVEFORMATEXTENSIBLE *fmtex = (WAVEFORMATEXTENSIBLE*)pwfx;
        WAVEFORMATEX *fmt2 = NULL;

        ok(fmtex->dwChannelMask != 0, "Got empty dwChannelMask\n");

        fmtex->dwChannelMask = 0xffff;

        hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000, 0, pwfx, NULL);
        ok(hr == S_OK, "Initialize(dwChannelMask = 0xffff) returns %08x\n", hr);

        IAudioClient_Release(ac);

        hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
                NULL, (void**)&ac);
        ok(hr == S_OK, "Activation failed with %08x\n", hr);

        fmtex->dwChannelMask = 0;

        hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_SHARED, pwfx, &fmt2);
        ok(hr == S_OK, "IsFormatSupported(dwChannelMask = 0) call returns %08x\n", hr);
        ok(fmtex->dwChannelMask == 0, "Passed format was modified\n");

        CoTaskMemFree(fmt2);

        hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000, 0, pwfx, NULL);
        ok(hr == S_OK, "Initialize(dwChannelMask = 0) returns %08x\n", hr);

        IAudioClient_Release(ac);

        hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
                NULL, (void**)&ac);
        ok(hr == S_OK, "Activation failed with %08x\n", hr);

        CoTaskMemFree(pwfx);

        hr = IAudioClient_GetMixFormat(ac, &pwfx);
        ok(hr == S_OK, "Valid GetMixFormat returns %08x\n", hr);
    }else
        skip("Skipping dwChannelMask tests\n");

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
    IAudioStreamVolume *asv;
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

    /* IAudioStreamVolume */
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

    hr = IAudioClient_GetService(ac, &IID_IAudioStreamVolume, (void**)&asv);
    ok(hr == S_OK, "GetService failed: %08x\n", hr);

    IAudioStreamVolume_AddRef(asv);
    ref = IAudioStreamVolume_Release(asv);
    ok(ref != 0, "AudioStreamVolume_Release gave wrong refcount: %u\n", ref);

    ref = IAudioClient_Release(ac);
    ok(ref != 0, "Client_Release gave wrong refcount: %u\n", ref);

    ref = IAudioStreamVolume_Release(asv);
    ok(ref == 0, "AudioStreamVolume_Release gave wrong refcount: %u\n", ref);
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

    psize = (defp / 10000000.) * pwfx->nSamplesPerSec * 10;

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

    psize = (minp / 10000000.) * pwfx->nSamplesPerSec * 10;

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

    CoTaskMemFree(pwfx);

    IAudioClock_Release(acl);
    IAudioRenderClient_Release(arc);
    IAudioClient_Release(ac);
}

static void test_session(void)
{
    IAudioClient *ses1_ac1, *ses1_ac2, *cap_ac = NULL;
    IAudioSessionControl2 *ses1_ctl, *ses1_ctl2, *cap_ctl;
    IMMDevice *cap_dev;
    GUID ses1_guid;
    AudioSessionState state;
    WAVEFORMATEX *pwfx;
    ULONG ref;
    HRESULT hr;

    hr = CoCreateGuid(&ses1_guid);
    ok(hr == S_OK, "CoCreateGuid failed: %08x\n", hr);

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ses1_ac1);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if (FAILED(hr)) return;

    hr = IAudioClient_GetMixFormat(ses1_ac1, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);

    hr = IAudioClient_Initialize(ses1_ac1, AUDCLNT_SHAREMODE_SHARED,
            0, 5000000, 0, pwfx, &ses1_guid);
    ok(hr == S_OK, "Initialize failed: %08x\n", hr);

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ses1_ac2);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);
    if (FAILED(hr))
    {
        IAudioClient_Release(ses1_ac1);
        return;
    }

    hr = IAudioClient_Initialize(ses1_ac2, AUDCLNT_SHAREMODE_SHARED,
            0, 5000000, 0, pwfx, &ses1_guid);
    ok(hr == S_OK, "Initialize failed: %08x\n", hr);

    hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(mme, eCapture,
            eMultimedia, &cap_dev);
    if(hr == S_OK){
        WAVEFORMATEX *cap_pwfx;

        hr = IMMDevice_Activate(cap_dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
                NULL, (void**)&cap_ac);
        ok(hr == S_OK, "Activate failed: %08x\n", hr);

        hr = IAudioClient_GetMixFormat(cap_ac, &cap_pwfx);
        ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);

        hr = IAudioClient_Initialize(cap_ac, AUDCLNT_SHAREMODE_SHARED,
                0, 5000000, 0, cap_pwfx, &ses1_guid);
        ok(hr == S_OK, "Initialize failed: %08x\n", hr);

        hr = IAudioClient_GetService(cap_ac, &IID_IAudioSessionControl, (void**)&cap_ctl);
        ok(hr == S_OK, "GetService failed: %08x\n", hr);

        IMMDevice_Release(cap_dev);
        CoTaskMemFree(cap_pwfx);
    }else
        skip("No capture device available; skipping capture device in render session tests\n");

    hr = IAudioClient_GetService(ses1_ac1, &IID_IAudioSessionControl2, (void**)&ses1_ctl);
    ok(hr == E_NOINTERFACE, "GetService gave wrong error: %08x\n", hr);

    hr = IAudioClient_GetService(ses1_ac1, &IID_IAudioSessionControl, (void**)&ses1_ctl);
    ok(hr == S_OK, "GetService failed: %08x\n", hr);

    hr = IAudioClient_GetService(ses1_ac1, &IID_IAudioSessionControl, (void**)&ses1_ctl2);
    ok(hr == S_OK, "GetService failed: %08x\n", hr);
    ok(ses1_ctl == ses1_ctl2, "Got different controls: %p %p\n", ses1_ctl, ses1_ctl2);
    ref = IAudioSessionControl_Release(ses1_ctl2);
    ok(ref != 0, "AudioSessionControl was destroyed\n");

    hr = IAudioClient_GetService(ses1_ac2, &IID_IAudioSessionControl, (void**)&ses1_ctl2);
    ok(hr == S_OK, "GetService failed: %08x\n", hr);

    hr = IAudioSessionControl_GetState(ses1_ctl, NULL);
    ok(hr == NULL_PTR_ERR, "GetState gave wrong error: %08x\n", hr);

    hr = IAudioSessionControl_GetState(ses1_ctl, &state);
    ok(hr == S_OK, "GetState failed: %08x\n", hr);
    ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

    hr = IAudioSessionControl_GetState(ses1_ctl2, &state);
    ok(hr == S_OK, "GetState failed: %08x\n", hr);
    ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

    if(cap_ac){
        hr = IAudioSessionControl_GetState(cap_ctl, &state);
        ok(hr == S_OK, "GetState failed: %08x\n", hr);
        ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);
    }

    hr = IAudioClient_Start(ses1_ac1);
    ok(hr == S_OK, "Start failed: %08x\n", hr);

    hr = IAudioSessionControl_GetState(ses1_ctl, &state);
    ok(hr == S_OK, "GetState failed: %08x\n", hr);
    ok(state == AudioSessionStateActive, "Got wrong state: %d\n", state);

    hr = IAudioSessionControl_GetState(ses1_ctl2, &state);
    ok(hr == S_OK, "GetState failed: %08x\n", hr);
    ok(state == AudioSessionStateActive, "Got wrong state: %d\n", state);

    if(cap_ac){
        hr = IAudioSessionControl_GetState(cap_ctl, &state);
        ok(hr == S_OK, "GetState failed: %08x\n", hr);
        ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);
    }

    hr = IAudioClient_Stop(ses1_ac1);
    ok(hr == S_OK, "Start failed: %08x\n", hr);

    hr = IAudioSessionControl_GetState(ses1_ctl, &state);
    ok(hr == S_OK, "GetState failed: %08x\n", hr);
    ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

    hr = IAudioSessionControl_GetState(ses1_ctl2, &state);
    ok(hr == S_OK, "GetState failed: %08x\n", hr);
    ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

    if(cap_ac){
        hr = IAudioSessionControl_GetState(cap_ctl, &state);
        ok(hr == S_OK, "GetState failed: %08x\n", hr);
        ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

        hr = IAudioClient_Start(cap_ac);
        ok(hr == S_OK, "Start failed: %08x\n", hr);

        hr = IAudioSessionControl_GetState(ses1_ctl, &state);
        ok(hr == S_OK, "GetState failed: %08x\n", hr);
        ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

        hr = IAudioSessionControl_GetState(ses1_ctl2, &state);
        ok(hr == S_OK, "GetState failed: %08x\n", hr);
        ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

        hr = IAudioSessionControl_GetState(cap_ctl, &state);
        ok(hr == S_OK, "GetState failed: %08x\n", hr);
        ok(state == AudioSessionStateActive, "Got wrong state: %d\n", state);

        hr = IAudioClient_Stop(cap_ac);
        ok(hr == S_OK, "Stop failed: %08x\n", hr);

        hr = IAudioSessionControl_GetState(ses1_ctl, &state);
        ok(hr == S_OK, "GetState failed: %08x\n", hr);
        ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

        hr = IAudioSessionControl_GetState(ses1_ctl2, &state);
        ok(hr == S_OK, "GetState failed: %08x\n", hr);
        ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

        hr = IAudioSessionControl_GetState(cap_ctl, &state);
        ok(hr == S_OK, "GetState failed: %08x\n", hr);
        ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

        ref = IAudioSessionControl_Release(cap_ctl);
        ok(ref == 0, "AudioSessionControl wasn't released: %u\n", ref);

        ref = IAudioClient_Release(cap_ac);
        ok(ref == 0, "AudioClient wasn't released: %u\n", ref);
    }

    ref = IAudioSessionControl_Release(ses1_ctl);
    ok(ref == 0, "AudioSessionControl wasn't released: %u\n", ref);

    ref = IAudioClient_Release(ses1_ac1);
    ok(ref == 0, "AudioClient wasn't released: %u\n", ref);

    ref = IAudioClient_Release(ses1_ac2);
    ok(ref == 1, "AudioClient had wrong refcount: %u\n", ref);

    /* we've released all of our IAudioClient references, so check GetState */
    hr = IAudioSessionControl_GetState(ses1_ctl2, &state);
    ok(hr == S_OK, "GetState failed: %08x\n", hr);
    ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

    ref = IAudioSessionControl_Release(ses1_ctl2);
    ok(ref == 0, "AudioSessionControl wasn't released: %u\n", ref);

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

static void test_session_creation(void)
{
    IMMDevice *cap_dev;
    IAudioClient *ac;
    IAudioSessionManager *sesm;
    ISimpleAudioVolume *sav;
    GUID session_guid;
    float vol;
    HRESULT hr;
    WAVEFORMATEX *fmt;

    CoCreateGuid(&session_guid);

    hr = IMMDevice_Activate(dev, &IID_IAudioSessionManager,
            CLSCTX_INPROC_SERVER, NULL, (void**)&sesm);
    ok(hr == S_OK, "Activate failed: %08x\n", hr);

    hr = IAudioSessionManager_GetSimpleAudioVolume(sesm, &session_guid,
            FALSE, &sav);
    ok(hr == S_OK, "GetSimpleAudioVolume failed: %08x\n", hr);

    hr = ISimpleAudioVolume_SetMasterVolume(sav, 0.6f, NULL);
    ok(hr == S_OK, "SetMasterVolume failed: %08x\n", hr);

    /* Release completely to show session persistence */
    ISimpleAudioVolume_Release(sav);
    IAudioSessionManager_Release(sesm);

    /* test if we can create a capture audioclient in the session we just
     * created from a SessionManager derived from a render device */
    hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(mme, eCapture,
            eMultimedia, &cap_dev);
    if(hr == S_OK){
        WAVEFORMATEX *cap_pwfx;
        IAudioClient *cap_ac;
        ISimpleAudioVolume *cap_sav;
        IAudioSessionManager *cap_sesm;

        hr = IMMDevice_Activate(cap_dev, &IID_IAudioSessionManager,
                CLSCTX_INPROC_SERVER, NULL, (void**)&cap_sesm);
        ok(hr == S_OK, "Activate failed: %08x\n", hr);

        hr = IAudioSessionManager_GetSimpleAudioVolume(cap_sesm, &session_guid,
                FALSE, &cap_sav);
        ok(hr == S_OK, "GetSimpleAudioVolume failed: %08x\n", hr);

        vol = 0.5f;
        hr = ISimpleAudioVolume_GetMasterVolume(cap_sav, &vol);
        ok(hr == S_OK, "GetMasterVolume failed: %08x\n", hr);
        ok(vol == 1.f, "Got wrong volume: %f\n", vol);

        ISimpleAudioVolume_Release(cap_sav);
        IAudioSessionManager_Release(cap_sesm);

        hr = IMMDevice_Activate(cap_dev, &IID_IAudioClient,
                CLSCTX_INPROC_SERVER, NULL, (void**)&cap_ac);
        ok(hr == S_OK, "Activate failed: %08x\n", hr);

        hr = IAudioClient_GetMixFormat(cap_ac, &cap_pwfx);
        ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);

        hr = IAudioClient_Initialize(cap_ac, AUDCLNT_SHAREMODE_SHARED,
                0, 5000000, 0, cap_pwfx, &session_guid);
        ok(hr == S_OK, "Initialize failed: %08x\n", hr);

        hr = IAudioClient_GetService(cap_ac, &IID_ISimpleAudioVolume,
                (void**)&cap_sav);
        ok(hr == S_OK, "GetService failed: %08x\n", hr);

        vol = 0.5f;
        hr = ISimpleAudioVolume_GetMasterVolume(cap_sav, &vol);
        ok(hr == S_OK, "GetMasterVolume failed: %08x\n", hr);
        ok(vol == 1.f, "Got wrong volume: %f\n", vol);

        CoTaskMemFree(cap_pwfx);
        ISimpleAudioVolume_Release(cap_sav);
        IAudioClient_Release(cap_ac);
        IMMDevice_Release(cap_dev);
    }

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08x\n", hr);

    hr = IAudioClient_GetMixFormat(ac, &fmt);
    ok(hr == S_OK, "GetMixFormat failed: %08x\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_NOPERSIST, 5000000, 0, fmt, &session_guid);
    ok(hr == S_OK, "Initialize failed: %08x\n", hr);

    hr = IAudioClient_GetService(ac, &IID_ISimpleAudioVolume, (void**)&sav);
    ok(hr == S_OK, "GetService failed: %08x\n", hr);

    vol = 0.5f;
    hr = ISimpleAudioVolume_GetMasterVolume(sav, &vol);
    ok(hr == S_OK, "GetMasterVolume failed: %08x\n", hr);
    ok(fabs(vol - 0.6f) < 0.05f, "Got wrong volume: %f\n", vol);

    CoTaskMemFree(fmt);
    ISimpleAudioVolume_Release(sav);
    IAudioClient_Release(ac);
}

START_TEST(render)
{
    HRESULT hr;

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
    test_session();
    test_streamvolume();
    test_channelvolume();
    test_simplevolume();
    test_volume_dependence();
    test_session_creation();

    IMMDevice_Release(dev);

cleanup:
    if (mme)
        IMMDeviceEnumerator_Release(mme);
    CoUninitialize();
}
