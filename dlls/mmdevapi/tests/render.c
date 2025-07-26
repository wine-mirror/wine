/*
 * Copyright 2010 Maarten Lankhorst for CodeWeavers
 *      2011-2012 Jörg Höhle
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
#include "mmsystem.h"
#include "audioclient.h"
#include "audiopolicy.h"
#include "endpointvolume.h"

static const unsigned int sampling_rates[] = { 8000, 16000, 22050, 44100, 48000, 96000 };
static const unsigned int channel_counts[] = { 1, 2, 8 };
static const unsigned int sample_formats[][2] = { {WAVE_FORMAT_PCM, 8}, {WAVE_FORMAT_PCM, 16},
                                                  {WAVE_FORMAT_PCM, 32}, {WAVE_FORMAT_IEEE_FLOAT, 32} };

#define NULL_PTR_ERR MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, RPC_X_NULL_REF_POINTER)

/* undocumented error code */
#define D3D11_ERROR_4E MAKE_HRESULT(SEVERITY_ERROR, FACILITY_DIRECT3D11, 0x4e)

static IMMDeviceEnumerator *mme = NULL;
static IMMDevice *dev = NULL;
static HRESULT hexcl = S_OK; /* or AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED */
static BOOL win10 = FALSE;

static const LARGE_INTEGER ullZero;

#define PI 3.14159265358979323846L
static DWORD wave_generate_tone(PWAVEFORMATEX pwfx, BYTE* data, UINT32 frames)
{
    static double phase = 0.; /* normalized to unit, not 2*PI */
    PWAVEFORMATEXTENSIBLE wfxe = (PWAVEFORMATEXTENSIBLE)pwfx;
    DWORD cn, i;
    double delta, y;

    if(!winetest_interactive)
        return AUDCLNT_BUFFERFLAGS_SILENT;
    if(wfxe->Format.wBitsPerSample != ((wfxe->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
       IsEqualGUID(&wfxe->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) ? 8 * sizeof(float) : 16))
        return AUDCLNT_BUFFERFLAGS_SILENT;

    for(delta = phase, cn = 0; cn < wfxe->Format.nChannels;
        delta += .5/wfxe->Format.nChannels, cn++){
        for(i = 0; i < frames; i++){
            y = sin(2*PI*(440.* i / wfxe->Format.nSamplesPerSec + delta));
            /* assume alignment is granted */
            if(wfxe->Format.wBitsPerSample == 16)
                ((short*)data)[cn+i*wfxe->Format.nChannels] = y * 32767.9;
            else
                ((float*)data)[cn+i*wfxe->Format.nChannels] = y;
        }
    }
    phase += 440.* frames / wfxe->Format.nSamplesPerSec;
    phase -= floor(phase);
    return 0;
}

static void test_uninitialized(IAudioClient *ac)
{
    HRESULT hr;
    UINT32 num;
    REFERENCE_TIME t1;

    HANDLE handle = CreateEventW(NULL, FALSE, FALSE, NULL);
    IUnknown *unk;

    hr = IAudioClient_GetBufferSize(ac, &num);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized GetBufferSize call returns %08lx\n", hr);

    hr = IAudioClient_GetStreamLatency(ac, &t1);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized GetStreamLatency call returns %08lx\n", hr);

    hr = IAudioClient_GetCurrentPadding(ac, &num);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized GetCurrentPadding call returns %08lx\n", hr);

    hr = IAudioClient_Start(ac);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized Start call returns %08lx\n", hr);

    hr = IAudioClient_Stop(ac);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized Stop call returns %08lx\n", hr);

    hr = IAudioClient_Reset(ac);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized Reset call returns %08lx\n", hr);

    hr = IAudioClient_SetEventHandle(ac, handle);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized SetEventHandle call returns %08lx\n", hr);

    hr = IAudioClient_GetService(ac, &IID_IAudioStreamVolume, (void**)&unk);
    ok(hr == AUDCLNT_E_NOT_INITIALIZED, "Uninitialized GetService call returns %08lx\n", hr);

    CloseHandle(handle);
}

static void test_audioclient(void)
{
    IAudioClient *ac;
    IAudioClient2 *ac2;
    IAudioClient3 *ac3;
    IAudioClock *acl;
    IUnknown *unk;
    IAudioClockAdjustment *aca;
    HRESULT hr;
    ULONG ref;
    WAVEFORMATEX *pwfx, *pwfx2;
    WAVEFORMATEXTENSIBLE format_float_error;
    REFERENCE_TIME t1, t2;
    HANDLE handle;
    BOOL offload_capable;
    AudioClientProperties client_props;

    format_float_error.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE);
    format_float_error.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    format_float_error.Format.nAvgBytesPerSec = 384000;
    format_float_error.Format.nBlockAlign = 8;
    format_float_error.Format.nChannels = 2;
    format_float_error.Format.nSamplesPerSec = 48000;
    format_float_error.Format.wBitsPerSample = 32;
    format_float_error.Samples.wValidBitsPerSample = 4;
    format_float_error.dwChannelMask = 3;
    format_float_error.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

    hr = IMMDevice_Activate(dev, &IID_IAudioClient3, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac3);
    ok(hr == S_OK ||
            broken(hr == E_NOINTERFACE) /* win8 */,
            "IAudioClient3 Activation failed with %08lx\n", hr);
    if(hr == S_OK)
        IAudioClient3_Release(ac3);

    hr = IMMDevice_Activate(dev, &IID_IAudioClient2, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac2);
    ok(hr == S_OK ||
       broken(hr == E_NOINTERFACE) /* win7 */,
       "IAudioClient2 Activation failed with %08lx\n", hr);
    if(hr == S_OK)
        IAudioClient2_Release(ac2);

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08lx\n", hr);
    if(hr != S_OK)
        return;

    handle = CreateEventW(NULL, FALSE, FALSE, NULL);

    unk = (void*)(LONG_PTR)0x12345678;
    hr = IAudioClient_QueryInterface(ac, &IID_NULL, (void**)&unk);
    ok(hr == E_NOINTERFACE, "QueryInterface(IID_NULL) returned %08lx\n", hr);
    ok(!unk, "QueryInterface(IID_NULL) returned non-null pointer %p\n", unk);

    hr = IAudioClient_QueryInterface(ac, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface(IID_IUnknown) returned %08lx\n", hr);
    if (unk)
    {
        ref = IUnknown_Release(unk);
        ok(ref == 1, "Released count is %lu\n", ref);
    }

    hr = IAudioClient_QueryInterface(ac, &IID_IAudioClient, (void**)&unk);
    ok(hr == S_OK, "QueryInterface(IID_IAudioClient) returned %08lx\n", hr);
    if (unk)
    {
        ref = IUnknown_Release(unk);
        ok(ref == 1, "Released count is %lu\n", ref);
    }
    hr = IAudioClient_QueryInterface(ac, &IID_IAudioClock, (void**)&acl);
    ok(hr == E_NOINTERFACE, "QueryInterface(IID_IAudioClock) returned %08lx\n", hr);

    hr = IAudioClient_QueryInterface(ac, &IID_IAudioClockAdjustment, (void**)&aca);
    ok(hr == S_OK, "QueryInterface(IID_IAudioClockAdjustment) returned %08lx\n", hr);
    if (aca)
    {
        hr = IAudioClockAdjustment_QueryInterface(aca, &IID_IAudioClock, (void**)&acl);
        ok(hr == E_NOINTERFACE, "QueryInterface(IID_IAudioClock) returned %08lx\n", hr);
        ref = IAudioClockAdjustment_Release(aca);
        ok(ref == 1, "Released count is %lu\n", ref);
    }

    hr = IAudioClient_GetDevicePeriod(ac, NULL, NULL);
    ok(hr == E_POINTER, "Invalid GetDevicePeriod call returns %08lx\n", hr);

    hr = IAudioClient_GetDevicePeriod(ac, &t1, NULL);
    ok(hr == S_OK, "Valid GetDevicePeriod call returns %08lx\n", hr);

    hr = IAudioClient_GetDevicePeriod(ac, NULL, &t2);
    ok(hr == S_OK, "Valid GetDevicePeriod call returns %08lx\n", hr);

    hr = IAudioClient_GetDevicePeriod(ac, &t1, &t2);
    ok(hr == S_OK, "Valid GetDevicePeriod call returns %08lx\n", hr);
    trace("Returned periods: %u.%04u ms %u.%04u ms\n",
          (UINT)(t1/10000), (UINT)(t1 % 10000),
          (UINT)(t2/10000), (UINT)(t2 % 10000));

    hr = IAudioClient_GetMixFormat(ac, NULL);
    ok(hr == E_POINTER, "GetMixFormat returns %08lx\n", hr);

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "Valid GetMixFormat returns %08lx\n", hr);

    if (hr == S_OK)
    {
        trace("pwfx: %p\n", pwfx);
        trace("Tag: %04x\n", pwfx->wFormatTag);
        trace("bits: %u\n", pwfx->wBitsPerSample);
        trace("chan: %u\n", pwfx->nChannels);
        trace("rate: %lu\n", pwfx->nSamplesPerSec);
        trace("align: %u\n", pwfx->nBlockAlign);
        trace("extra: %u\n", pwfx->cbSize);
        ok(pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE, "wFormatTag is %x\n", pwfx->wFormatTag);
        if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        {
            WAVEFORMATEXTENSIBLE *pwfxe = (void*)pwfx;
            trace("Res: %u\n", pwfxe->Samples.wReserved);
            trace("Mask: %lx\n", pwfxe->dwChannelMask);
            trace("Alg: %s\n",
                  IsEqualGUID(&pwfxe->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM)?"PCM":
                  (IsEqualGUID(&pwfxe->SubFormat,
                               &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)?"FLOAT":"Other"));
            ok(IsEqualGUID(&pwfxe->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT),
               "got format %s\n", debugstr_guid(&pwfxe->SubFormat));

            pwfxe->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
            hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_SHARED, pwfx, &pwfx2);
            ok(hr == S_OK, "Valid IsFormatSupported(Shared) call returns %08lx\n", hr);
            ok(pwfx2 == NULL, "pwfx2 is non-null\n");
            CoTaskMemFree(pwfx2);
        }

        hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_SHARED, pwfx, &pwfx2);
        ok(hr == S_OK, "Valid IsFormatSupported(Shared) call returns %08lx\n", hr);
        ok(pwfx2 == NULL, "pwfx2 is non-null\n");
        CoTaskMemFree(pwfx2);

        hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_SHARED, NULL, NULL);
        ok(hr == E_POINTER, "IsFormatSupported(NULL) call returns %08lx\n", hr);

        hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_SHARED, pwfx, NULL);
        ok(hr == E_POINTER, "IsFormatSupported(Shared,NULL) call returns %08lx\n", hr);

        hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_EXCLUSIVE, pwfx, NULL);
        ok(hr == S_OK || hr == AUDCLNT_E_UNSUPPORTED_FORMAT || hr == AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED,
           "IsFormatSupported(Exclusive) call returns %08lx\n", hr);
        hexcl = hr;

        hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_EXCLUSIVE, &format_float_error.Format, NULL);
        ok(hr == S_OK || hr == AUDCLNT_E_UNSUPPORTED_FORMAT || hr == AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED,
                  "IsFormatSupported(Exclusive) call returns %08lx\n", hr);

        pwfx2 = (WAVEFORMATEX*)0xDEADF00D;
        hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_EXCLUSIVE, pwfx, &pwfx2);
        ok(hr == hexcl, "IsFormatSupported(Exclusive) call returns %08lx\n", hr);
        ok(pwfx2 == NULL, "pwfx2 non-null on exclusive IsFormatSupported\n");

        if (hexcl != AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED)
            hexcl = S_OK;

        hr = IAudioClient_IsFormatSupported(ac, 0xffffffff, pwfx, NULL);
        ok(hr == E_INVALIDARG/*w32*/ ||
           broken(hr == AUDCLNT_E_UNSUPPORTED_FORMAT/*w64 response from exclusive mode driver */),
           "IsFormatSupported(0xffffffff) call returns %08lx\n", hr);
    }

    hr = IAudioClient_QueryInterface(ac, &IID_IAudioClient2, (void**)&ac2);
    if (hr == S_OK)
    {
        hr = IAudioClient2_IsOffloadCapable(ac2, AudioCategory_BackgroundCapableMedia, NULL);
        ok(hr == E_INVALIDARG, "IsOffloadCapable gave wrong error: %08lx\n", hr);

        hr = IAudioClient2_IsOffloadCapable(ac2, AudioCategory_BackgroundCapableMedia, &offload_capable);
        ok(hr == S_OK, "IsOffloadCapable failed: %08lx\n", hr);

        hr = IAudioClient2_SetClientProperties(ac2, NULL);
        ok(hr == E_POINTER, "SetClientProperties with NULL props gave wrong error: %08lx\n", hr);

        /* invalid cbSize */
        client_props.cbSize = 0;
        client_props.bIsOffload = FALSE;
        client_props.eCategory = AudioCategory_BackgroundCapableMedia;
        client_props.Options = 0;

        hr = IAudioClient2_SetClientProperties(ac2, &client_props);
        ok(hr == E_INVALIDARG, "SetClientProperties with invalid cbSize gave wrong error: %08lx\n", hr);

        /* offload consistency */
        client_props.cbSize = sizeof(client_props) - sizeof(client_props.Options);
        client_props.bIsOffload = TRUE;

        hr = IAudioClient2_SetClientProperties(ac2, &client_props);
        if(!offload_capable)
            ok(hr == AUDCLNT_E_ENDPOINT_OFFLOAD_NOT_CAPABLE, "SetClientProperties(offload) gave wrong error: %08lx\n", hr);
        else
            ok(hr == S_OK, "SetClientProperties(offload) failed: %08lx\n", hr);

        /* disable offload */
        client_props.bIsOffload = FALSE;
        hr = IAudioClient2_SetClientProperties(ac2, &client_props);
        ok(hr == S_OK, "SetClientProperties failed: %08lx\n", hr);

        /* Options field added in Win 8.1 */
        client_props.cbSize = sizeof(client_props);
        hr = IAudioClient2_SetClientProperties(ac2, &client_props);
        ok(hr == S_OK ||
                broken(hr == E_INVALIDARG) /* <= win8 */,
                "SetClientProperties failed: %08lx\n", hr);

        IAudioClient2_Release(ac2);
    }
    else
        win_skip("IAudioClient2 is not present on Win <= 7\n");

    hr = IAudioClient_QueryInterface(ac, &IID_IAudioClient3, (void**)&ac3);
    ok(hr == S_OK ||
            broken(hr == E_NOINTERFACE) /* win8 */,
            "Failed to query IAudioClient3 interface: %08lx\n", hr);

    if(hr == S_OK){
        UINT32 default_period = 0, unit_period, min_period, max_period;

        hr = IAudioClient3_GetSharedModeEnginePeriod(
            ac3, pwfx, &default_period, &unit_period, &min_period, &max_period);
        ok(hr == S_OK, "GetSharedModeEnginePeriod returns %08lx\n", hr);

        hr = IAudioClient3_InitializeSharedAudioStream(
            ac3, AUDCLNT_SHAREMODE_SHARED, default_period, pwfx, NULL);
        ok(hr == S_OK, "InitializeSharedAudioStream returns %08lx\n", hr);

        IAudioClient3_Release(ac3);
        IAudioClient_Release(ac);

        hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
                NULL, (void**)&ac);
        ok(hr == S_OK, "Activation failed with %08lx\n", hr);
    }
    else
        win_skip("IAudioClient3 is not present\n");

    test_uninitialized(ac);

    hr = IAudioClient_Initialize(ac, 3, 0, 5000000, 0, pwfx, NULL);
    ok(broken(hr == AUDCLNT_E_NOT_INITIALIZED) || /* <= win8 */
            hr == E_INVALIDARG, "Initialize with invalid sharemode returns %08lx\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0xffffffff, 5000000, 0, pwfx, NULL);
    ok(hr == E_INVALIDARG ||
            hr == AUDCLNT_E_INVALID_STREAM_FLAG, "Initialize with invalid flags returns %08lx\n", hr);

    /* A period != 0 is ignored and the call succeeds.
     * Since we can only initialize successfully once, skip those tests.
     */
    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000, 0, NULL, NULL);
    ok(hr == E_POINTER, "Initialize with null format returns %08lx\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 0, 0, pwfx, NULL);
    ok(hr == S_OK, "Initialize with 0 buffer size returns %08lx\n", hr);
    if(hr == S_OK){
        UINT32 num;

        hr = IAudioClient_GetBufferSize(ac, &num);
        ok(hr == S_OK, "GetBufferSize from duration 0 returns %08lx\n", hr);
        if(hr == S_OK)
            trace("Initialize(duration=0) GetBufferSize is %u\n", num);
    }

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000, 0, pwfx, NULL);
    ok(hr == AUDCLNT_E_ALREADY_INITIALIZED, "Calling Initialize twice returns %08lx\n", hr);

    hr = IAudioClient_Start(ac);
    ok(hr == S_OK ||
       broken(hr == AUDCLNT_E_DEVICE_INVALIDATED), /* Win10 >= 1607 */
       "Start on a doubly initialized stream returns %08lx\n", hr);

    IAudioClient_Release(ac);

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08lx\n", hr);

    if(pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE){
        WAVEFORMATEXTENSIBLE *fmtex = (WAVEFORMATEXTENSIBLE*)pwfx;
        WAVEFORMATEX *fmt2 = NULL;

        ok(fmtex->dwChannelMask != 0, "Got empty dwChannelMask\n");

        fmtex->dwChannelMask = 0xffff;

        hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000, 0, pwfx, NULL);
        ok(hr == S_OK ||
                hr == AUDCLNT_E_UNSUPPORTED_FORMAT /* win10 */, "Initialize(dwChannelMask = 0xffff) returns %08lx\n", hr);

        IAudioClient_Release(ac);

        hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
                NULL, (void**)&ac);
        ok(hr == S_OK, "Activation failed with %08lx\n", hr);

        fmtex->dwChannelMask = 0;

        hr = IAudioClient_IsFormatSupported(ac, AUDCLNT_SHAREMODE_SHARED, pwfx, &fmt2);
        ok(hr == S_OK || broken(hr == S_FALSE /* w7 Realtek HDA */),
           "IsFormatSupported(dwChannelMask = 0) call returns %08lx\n", hr);
        ok(fmtex->dwChannelMask == 0, "Passed format was modified\n");

        CoTaskMemFree(fmt2);

        hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000, 0, pwfx, NULL);
        ok(hr == S_OK, "Initialize(dwChannelMask = 0) returns %08lx\n", hr);

        IAudioClient_Release(ac);

        hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
                NULL, (void**)&ac);
        ok(hr == S_OK, "Activation failed with %08lx\n", hr);

        CoTaskMemFree(pwfx);

        hr = IAudioClient_GetMixFormat(ac, &pwfx);
        ok(hr == S_OK, "Valid GetMixFormat returns %08lx\n", hr);
    }else
        skip("Skipping dwChannelMask tests\n");

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000, 0, pwfx, NULL);
    ok(hr == S_OK, "Valid Initialize returns %08lx\n", hr);
    if (hr != S_OK)
        goto cleanup;

    hr = IAudioClient_GetStreamLatency(ac, NULL);
    ok(hr == E_POINTER, "GetStreamLatency(NULL) call returns %08lx\n", hr);

    hr = IAudioClient_GetStreamLatency(ac, &t2);
    ok(hr == S_OK, "Valid GetStreamLatency call returns %08lx\n", hr);
    trace("Returned latency: %u.%04u ms\n",
          (UINT)(t2/10000), (UINT)(t2 % 10000));
    ok(t2 >= t1 || broken(t2 >= t1/2 && pwfx->nSamplesPerSec > 48000) ||
            broken(t2 == 0) /* (!) win10 */,
       "Latency < default period, delta %ldus (%s vs %s)\n",
       (LONG)((t2-t1)/10), wine_dbgstr_longlong(t2), wine_dbgstr_longlong(t1));
    /* Native appears to add the engine period to the HW latency in shared mode */
    if(t2 == 0)
        win10 = TRUE;

    hr = IAudioClient_SetEventHandle(ac, NULL);
    ok(hr == E_INVALIDARG, "SetEventHandle(NULL) returns %08lx\n", hr);

    hr = IAudioClient_SetEventHandle(ac, handle);
    ok(hr == AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED ||
       broken(hr == HRESULT_FROM_WIN32(ERROR_INVALID_NAME)) ||
       broken(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) /* Some 2k8 */ ||
       broken(hr == HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME)) /* Some Vista */
       , "SetEventHandle returns %08lx\n", hr);

    hr = IAudioClient_Reset(ac);
    ok(hr == S_OK, "Reset on an initialized stream returns %08lx\n", hr);

    hr = IAudioClient_Reset(ac);
    ok(hr == S_OK, "Reset on an already reset stream returns %08lx\n", hr);

    hr = IAudioClient_Stop(ac);
    ok(hr == S_FALSE, "Stop on a stopped stream returns %08lx\n", hr);

    hr = IAudioClient_Start(ac);
    ok(hr == S_OK, "Start on a stopped stream returns %08lx\n", hr);

    hr = IAudioClient_Start(ac);
    ok(hr == AUDCLNT_E_NOT_STOPPED, "Start twice returns %08lx\n", hr);

cleanup:
    IAudioClient_Release(ac);
    CloseHandle(handle);
    CoTaskMemFree(pwfx);
}

static void test_formats(AUDCLNT_SHAREMODE mode)
{
    IAudioClient *ac;
    HRESULT hr, hrs;
    WAVEFORMATEX fmt, *pwfx, *pwfx2;
    int i, j, k;

    fmt.cbSize = 0;

    for (i = 0; i < ARRAY_SIZE(sampling_rates); i++) {
        for (j = 0; j < ARRAY_SIZE(channel_counts); j++) {
            for (k = 0; k < ARRAY_SIZE(sample_formats); k++) {
                char format_chr;

                hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
                        NULL, (void**)&ac);
                ok(hr == S_OK, "Activation failed with %08lx\n", hr);
                if(hr != S_OK)
                    continue;

                hr = IAudioClient_GetMixFormat(ac, &pwfx);
                ok(hr == S_OK, "GetMixFormat failed: %08lx\n", hr);

                fmt.wFormatTag     = sample_formats[k][0];
                fmt.nSamplesPerSec = sampling_rates[i];
                fmt.wBitsPerSample = sample_formats[k][1];
                fmt.nChannels      = channel_counts[j];
                fmt.nBlockAlign    = fmt.nChannels * fmt.wBitsPerSample / 8;
                fmt.nAvgBytesPerSec= fmt.nBlockAlign * fmt.nSamplesPerSec;

                format_chr = fmt.wFormatTag == WAVE_FORMAT_PCM ? 'P' : 'F';

                pwfx2 = (WAVEFORMATEX*)0xDEADF00D;
                hr = IAudioClient_IsFormatSupported(ac, mode, &fmt, &pwfx2);
                hrs = hr;
                if (hr == S_OK)
                    trace("IsSupported(%s, %c%lux%2ux%u)\n",
                          mode == AUDCLNT_SHAREMODE_SHARED ? "shared " : "exclus.",
                          format_chr, fmt.nSamplesPerSec, fmt.wBitsPerSample, fmt.nChannels);

                /* In shared mode you can only change bit width, not sampling rate or channel count. */
                if (mode == AUDCLNT_SHAREMODE_SHARED) {
                    BOOL compatible = fmt.nSamplesPerSec == pwfx->nSamplesPerSec && fmt.nChannels == pwfx->nChannels;
                    HRESULT expected = compatible ? S_OK : S_FALSE;
                    if (fmt.nChannels > 2)
                        expected = AUDCLNT_E_UNSUPPORTED_FORMAT;
                    todo_wine_if(hr != expected)
                    ok(hr == expected, "IsFormatSupported(shared, %c%lux%2ux%u) returns %08lx, expected %08lx\n",
                            format_chr, fmt.nSamplesPerSec, fmt.wBitsPerSample, fmt.nChannels, hr, expected);
                } else {
                    ok(hr == S_OK || hr == AUDCLNT_E_UNSUPPORTED_FORMAT || hr == hexcl,
                            "IsFormatSupported(exclusive, %c%lux%2ux%u) returns %08lx\n",
                            format_chr, fmt.nSamplesPerSec, fmt.wBitsPerSample, fmt.nChannels, hr);
                }

                /* Only shared mode suggests something ... GetMixFormat! */
                ok((hr == S_FALSE)^(pwfx2 == NULL), "hr %lx<->suggest %p\n", hr, pwfx2);
                if (pwfx2) {
                    ok(pwfx2->wFormatTag     == pwfx->wFormatTag &&
                       pwfx2->nSamplesPerSec == pwfx->nSamplesPerSec &&
                       pwfx2->nChannels      == pwfx->nChannels &&
                       pwfx2->wBitsPerSample == pwfx->wBitsPerSample,
                       "Suggestion %c%lux%2ux%u differs from GetMixFormat\n",
                       format_chr, pwfx2->nSamplesPerSec, pwfx2->wBitsPerSample, pwfx2->nChannels);
                }

                /* Vista returns E_INVALIDARG upon AUDCLNT_STREAMFLAGS_RATEADJUST */
                hr = IAudioClient_Initialize(ac, mode, 0, 5000000, 0, &fmt, NULL);
                if ((hrs == S_OK) ^ (hr == S_OK))
                    trace("Initialize (%s, %c%lux%2ux%u) returns %08lx unlike IsFormatSupported\n",
                          mode == AUDCLNT_SHAREMODE_SHARED ? "shared " : "exclus.",
                          format_chr, fmt.nSamplesPerSec, fmt.wBitsPerSample, fmt.nChannels, hr);
                if (mode == AUDCLNT_SHAREMODE_SHARED) {
                    HRESULT expected = hrs == S_OK ? S_OK : AUDCLNT_E_UNSUPPORTED_FORMAT;
                    if (fmt.nChannels > 2)
                        expected = E_INVALIDARG;
                    todo_wine_if(fmt.nChannels > 2)
                    ok(hr == expected, "Initialize(shared,  %c%lux%2ux%u) returns %08lx\n",
                       format_chr, fmt.nSamplesPerSec, fmt.wBitsPerSample, fmt.nChannels, hr);
                } else if (hrs == AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED)
                    /* Unsupported format implies "create failed" and shadows "not allowed" */
                    ok(hrs == hexcl && (hr == AUDCLNT_E_ENDPOINT_CREATE_FAILED || hr == hrs),
                       "Initialize(noexcl., %c%lux%2ux%u) returns %08lx(%08lx)\n",
                       format_chr, fmt.nSamplesPerSec, fmt.wBitsPerSample, fmt.nChannels, hr, hrs);
                else
                    /* On testbot 48000x16x1 claims support, but does not Initialize.
                     * Some cards Initialize 44100|48000x16x1 yet claim no support;
                     * F. Gouget's w7 bots do that for 12000|96000x8|16x1|2 */
                    todo_wine_if(fmt.nChannels > 2 && hr == AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED)
                    ok(hrs == S_OK ? hr == S_OK || broken(hr == AUDCLNT_E_ENDPOINT_CREATE_FAILED)
                       : hr == AUDCLNT_E_ENDPOINT_CREATE_FAILED || hr == AUDCLNT_E_UNSUPPORTED_FORMAT ||
                         broken(hr == S_OK &&
                             ((fmt.nChannels == 1 && fmt.wBitsPerSample == 16) ||
                              (fmt.nSamplesPerSec == 12000 || fmt.nSamplesPerSec == 96000)))
                              || (hr == E_INVALIDARG && fmt.nChannels > 2),
                       "Initialize(exclus., %c%lux%2ux%u) returns %08lx\n",
                       format_chr, fmt.nSamplesPerSec, fmt.wBitsPerSample, fmt.nChannels, hr);

                /* Bug in native (Vista/w2k8/w7): after Initialize failed, better
                 * Release this ac and Activate a new one.
                 * A second call (with a known working format) would yield
                 * ALREADY_INITIALIZED in shared mode yet be unusable, and in exclusive
                 * mode some entity keeps a lock on the device, causing DEVICE_IN_USE to
                 * all subsequent calls until the audio engine service is restarted. */

                CoTaskMemFree(pwfx2);
                CoTaskMemFree(pwfx);
                IAudioClient_Release(ac);
            }
        }
    }
}

static void test_references(void)
{
    IAudioClient *ac, *ac2;
    IAudioRenderClient *rc;
    IAudioClockAdjustment *aca;
    ISimpleAudioVolume *sav;
    IAudioStreamVolume *asv;
    IAudioClock *acl;
    WAVEFORMATEX *pwfx;
    HRESULT hr;
    ULONG ref;

    /* IAudioRenderClient */
    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08lx\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000,
            0, pwfx, NULL);
    ok(hr == S_OK, "Initialize failed: %08lx\n", hr);

    CoTaskMemFree(pwfx);

    hr = IAudioClient_GetService(ac, &IID_IAudioRenderClient, (void**)&rc);
    ok(hr == S_OK, "GetService failed: %08lx\n", hr);
    if(hr != S_OK) {
        IAudioClient_Release(ac);
        return;
    }

    IAudioRenderClient_AddRef(rc);
    ref = IAudioRenderClient_Release(rc);
    ok(ref != 0, "RenderClient_Release gave wrong refcount: %lu\n", ref);

    ref = IAudioClient_Release(ac);
    ok(ref != 0, "Client_Release gave wrong refcount: %lu\n", ref);

    ref = IAudioRenderClient_Release(rc);
    ok(ref == 0, "RenderClient_Release gave wrong refcount: %lu\n", ref);

    /* IAudioClockAdjustment */
    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08lx\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000,
            0, pwfx, NULL);
    ok(hr == S_OK, "Initialize failed: %08lx\n", hr);

    CoTaskMemFree(pwfx);

    hr = IAudioClient_GetService(ac, &IID_IAudioClockAdjustment, (void**)&aca);
    todo_wine ok(hr == E_INVALIDARG, "IAudioClient_GetService(IID_IAudioClockAdjustment) returned %08lx\n", hr);

    if (hr == S_OK) {
        ref = IAudioClockAdjustment_Release(aca);
        ok(ref == 1, "AudioClockAdjustment_Release gave wrong refcount: %lu\n", ref);
    }

    ref = IAudioClient_Release(ac);
    ok(ref == 0, "Client_Release gave wrong refcount: %lu\n", ref);

    /* IAudioClockAdjustment */
    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08lx\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_RATEADJUST, 5000000,
            0, pwfx, NULL);
    ok(hr == S_OK, "Initialize failed: %08lx\n", hr);

    CoTaskMemFree(pwfx);

    hr = IAudioClient_GetService(ac, &IID_IAudioClockAdjustment, (void**)&aca);
    ok(hr == S_OK, "IAudioClient_GetService(IID_IAudioClockAdjustment) returned %08lx\n", hr);
    ref = IAudioClockAdjustment_Release(aca);
    ok(ref == 1, "AudioClockAdjustment_Release gave wrong refcount: %lu\n", ref);

    ref = IAudioClient_Release(ac);
    ok(ref == 0, "Client_Release gave wrong refcount: %lu\n", ref);


    /* ISimpleAudioVolume */
    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08lx\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000,
            0, pwfx, NULL);
    ok(hr == S_OK, "Initialize failed: %08lx\n", hr);

    CoTaskMemFree(pwfx);

    hr = IAudioClient_GetService(ac, &IID_ISimpleAudioVolume, (void**)&sav);
    ok(hr == S_OK, "GetService failed: %08lx\n", hr);

    ISimpleAudioVolume_AddRef(sav);
    ref = ISimpleAudioVolume_Release(sav);
    ok(ref != 0, "SimpleAudioVolume_Release gave wrong refcount: %lu\n", ref);

    ref = IAudioClient_Release(ac);
    ok(ref != 0, "Client_Release gave wrong refcount: %lu\n", ref);

    ref = ISimpleAudioVolume_Release(sav);
    ok(ref == 0, "SimpleAudioVolume_Release gave wrong refcount: %lu\n", ref);

    /* IAudioClock */
    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08lx\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000,
            0, pwfx, NULL);
    ok(hr == S_OK, "Initialize failed: %08lx\n", hr);

    CoTaskMemFree(pwfx);

    hr = IAudioClient_GetService(ac, &IID_IAudioClock, (void**)&acl);
    ok(hr == S_OK, "GetService failed: %08lx\n", hr);

    IAudioClock_AddRef(acl);
    ref = IAudioClock_Release(acl);
    ok(ref != 0, "AudioClock_Release gave wrong refcount: %lu\n", ref);

    hr = IAudioClock_QueryInterface(acl, &IID_IAudioClient, (void**)&ac2);
    ok(hr == E_NOINTERFACE, "QueryInterface(IID_IAudioClient) returned %08lx\n", hr);

    ref = IAudioClient_Release(ac);
    ok(ref != 0, "Client_Release gave wrong refcount: %lu\n", ref);

    ref = IAudioClock_Release(acl);
    ok(ref == 0, "AudioClock_Release gave wrong refcount: %lu\n", ref);

    /* IAudioStreamVolume */
    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08lx\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000,
            0, pwfx, NULL);
    ok(hr == S_OK, "Initialize failed: %08lx\n", hr);

    CoTaskMemFree(pwfx);

    hr = IAudioClient_GetService(ac, &IID_IAudioStreamVolume, (void**)&asv);
    ok(hr == S_OK, "GetService failed: %08lx\n", hr);

    IAudioStreamVolume_AddRef(asv);
    ref = IAudioStreamVolume_Release(asv);
    ok(ref != 0, "AudioStreamVolume_Release gave wrong refcount: %lu\n", ref);

    ref = IAudioClient_Release(ac);
    ok(ref != 0, "Client_Release gave wrong refcount: %lu\n", ref);

    ref = IAudioStreamVolume_Release(asv);
    ok(ref == 0, "AudioStreamVolume_Release gave wrong refcount: %lu\n", ref);
}

static void test_event(void)
{
    HANDLE event;
    HRESULT hr;
    DWORD r;
    IAudioClient *ac;
    WAVEFORMATEX *pwfx;

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08lx\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 5000000,
            0, pwfx, NULL);
    ok(hr == S_OK, "Initialize failed: %08lx\n", hr);

    CoTaskMemFree(pwfx);

    event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(event != NULL, "CreateEvent failed\n");

    hr = IAudioClient_Start(ac);
    ok(hr == AUDCLNT_E_EVENTHANDLE_NOT_SET ||
            hr == D3D11_ERROR_4E /* win10 */, "Start failed: %08lx\n", hr);

    hr = IAudioClient_SetEventHandle(ac, event);
    ok(hr == S_OK, "SetEventHandle failed: %08lx\n", hr);

    hr = IAudioClient_SetEventHandle(ac, event);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_NAME) ||
            hr == E_UNEXPECTED /* win10 */, "SetEventHandle returns %08lx\n", hr);

    r = WaitForSingleObject(event, 40);
    ok(r == WAIT_TIMEOUT, "Wait(event) before Start gave %lx\n", r);

    hr = IAudioClient_Start(ac);
    ok(hr == S_OK, "Start failed: %08lx\n", hr);

    r = WaitForSingleObject(event, 20);
    ok(r == WAIT_OBJECT_0, "Wait(event) after Start gave %lx\n", r);

    hr = IAudioClient_Stop(ac);
    ok(hr == S_OK, "Stop failed: %08lx\n", hr);

    ok(ResetEvent(event), "ResetEvent\n");

    /* Still receiving events! */
    r = WaitForSingleObject(event, 20);
    ok(r == WAIT_OBJECT_0, "Wait(event) after Stop gave %lx\n", r);

    hr = IAudioClient_Reset(ac);
    ok(hr == S_OK, "Reset failed: %08lx\n", hr);

    ok(ResetEvent(event), "ResetEvent\n");

    r = WaitForSingleObject(event, 120);
    ok(r == WAIT_OBJECT_0, "Wait(event) after Reset gave %lx\n", r);

    hr = IAudioClient_SetEventHandle(ac, NULL);
    ok(hr == E_INVALIDARG, "SetEventHandle(NULL) returns %08lx\n", hr);

    r = WaitForSingleObject(event, 70);
    ok(r == WAIT_OBJECT_0, "Wait(NULL event) gave %lx\n", r);

    /* test releasing a playing stream */
    hr = IAudioClient_Start(ac);
    ok(hr == S_OK, "Start failed: %08lx\n", hr);
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
    BYTE *buf, silence;
    UINT32 psize, pad, written, i;

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED,
            0, 5000000, 0, pwfx, NULL);
    ok(hr == S_OK, "Initialize failed: %08lx\n", hr);
    if(hr != S_OK)
        return;

    if(pwfx->wBitsPerSample == 8)
        silence = 128;
    else
        silence = 0;

    /** GetDevicePeriod
     * Default (= shared) device period is 10ms (e.g. 441 frames at 44100),
     * except when the HW/OS forces a particular alignment,
     * e.g. 10.1587ms is 28 * 16 = 448 frames at 44100 with HDA.
     * 441 observed with Vista, 448 with w7 on the same HW! */
    hr = IAudioClient_GetDevicePeriod(ac, &defp, &minp);
    ok(hr == S_OK, "GetDevicePeriod failed: %08lx\n", hr);
    /* some wineXYZ.drv use 20ms, not seen on native */
    ok(defp == 100000 || broken(defp == 101587) || defp == 200000,
       "Expected 10ms default period: %lu\n", (ULONG)defp);
    ok(minp != 0, "Minimum period is 0\n");
    ok(minp <= defp, "Minimum period is greater than default period\n");

    hr = IAudioClient_GetService(ac, &IID_IAudioRenderClient, (void**)&arc);
    ok(hr == S_OK, "GetService failed: %08lx\n", hr);

    psize = MulDiv(defp, pwfx->nSamplesPerSec, 10000000) * 10;

    written = 0;
    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding failed: %08lx\n", hr);
    ok(pad == written, "GetCurrentPadding returned %u, should be %u\n", pad, written);

    hr = IAudioRenderClient_GetBuffer(arc, psize, &buf);
    ok(hr == S_OK, "GetBuffer failed: %08lx\n", hr);
    ok(buf != NULL, "NULL buffer returned\n");
    if(!win10){
        /* win10 appears not to clear the buffer */
        for(i = 0; i < psize * pwfx->nBlockAlign; ++i){
            if(buf[i] != silence){
                ok(0, "buffer has data in it already, i: %u, value: %f\n", i, *((float*)buf));
                break;
            }
        }
    }

    hr = IAudioRenderClient_GetBuffer(arc, 0, &buf);
    ok(hr == AUDCLNT_E_OUT_OF_ORDER, "GetBuffer 0 size failed: %08lx\n", hr);
    ok(buf == NULL, "GetBuffer 0 gave %p\n", buf);
    /* MSDN instead documents buf remains untouched */

    hr = IAudioClient_Reset(ac);
    ok(hr == AUDCLNT_E_BUFFER_OPERATION_PENDING, "Reset failed: %08lx\n", hr);

    hr = IAudioRenderClient_ReleaseBuffer(arc, psize,
            AUDCLNT_BUFFERFLAGS_SILENT);
    ok(hr == S_OK, "ReleaseBuffer failed: %08lx\n", hr);
    if(hr == S_OK) written += psize;

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding failed: %08lx\n", hr);
    ok(pad == written, "GetCurrentPadding returned %u, should be %u\n", pad, written);

    psize = MulDiv(minp, pwfx->nSamplesPerSec, 10000000) * 10;

    hr = IAudioRenderClient_GetBuffer(arc, psize, &buf);
    ok(hr == S_OK, "GetBuffer failed: %08lx\n", hr);
    ok(buf != NULL, "NULL buffer returned\n");

    hr = IAudioRenderClient_ReleaseBuffer(arc, psize,
            AUDCLNT_BUFFERFLAGS_SILENT);
    ok(hr == S_OK, "ReleaseBuffer failed: %08lx\n", hr);
    written += psize;

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding failed: %08lx\n", hr);
    ok(pad == written, "GetCurrentPadding returned %u, should be %u\n", pad, written);

    /* overfull buffer. requested 1/2s buffer size, so try
     * to get a 1/2s buffer, which should fail */
    psize = pwfx->nSamplesPerSec / 2;
    buf = (void*)0xDEADF00D;
    hr = IAudioRenderClient_GetBuffer(arc, psize, &buf);
    ok(hr == AUDCLNT_E_BUFFER_TOO_LARGE, "GetBuffer gave wrong error: %08lx\n", hr);
    ok(buf == NULL, "NULL expected %p\n", buf);

    hr = IAudioRenderClient_ReleaseBuffer(arc, psize, 0);
    ok(hr == AUDCLNT_E_OUT_OF_ORDER, "ReleaseBuffer gave wrong error: %08lx\n", hr);

    psize = MulDiv(minp, pwfx->nSamplesPerSec, 10000000) * 2;

    hr = IAudioRenderClient_GetBuffer(arc, psize, &buf);
    ok(hr == S_OK, "GetBuffer failed: %08lx\n", hr);
    ok(buf != NULL, "NULL buffer returned\n");

    hr = IAudioRenderClient_ReleaseBuffer(arc, 0, 0);
    ok(hr == S_OK, "ReleaseBuffer 0 gave wrong error: %08lx\n", hr);

    buf = (void*)0xDEADF00D;
    hr = IAudioRenderClient_GetBuffer(arc, 0, &buf);
    ok(hr == S_OK, "GetBuffer 0 size failed: %08lx\n", hr);
    ok(buf == NULL, "GetBuffer 0 gave %p\n", buf);
    /* MSDN instead documents buf remains untouched */

    buf = (void*)0xDEADF00D;
    hr = IAudioRenderClient_GetBuffer(arc, 0, &buf);
    ok(hr == S_OK, "GetBuffer 0 size #2 failed: %08lx\n", hr);
    ok(buf == NULL, "GetBuffer 0 #2 gave %p\n", buf);

    hr = IAudioRenderClient_ReleaseBuffer(arc, psize, 0);
    ok(hr == AUDCLNT_E_OUT_OF_ORDER, "ReleaseBuffer not size 0 gave %08lx\n", hr);

    hr = IAudioRenderClient_GetBuffer(arc, psize, &buf);
    ok(hr == S_OK, "GetBuffer failed: %08lx\n", hr);
    ok(buf != NULL, "NULL buffer returned\n");

    hr = IAudioRenderClient_ReleaseBuffer(arc, 0, 0);
    ok(hr == S_OK, "ReleaseBuffer 0 gave wrong error: %08lx\n", hr);

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding failed: %08lx\n", hr);
    ok(pad == written, "GetCurrentPadding returned %u, should be %u\n", pad, written);

    hr = IAudioRenderClient_GetBuffer(arc, psize, &buf);
    ok(hr == S_OK, "GetBuffer failed: %08lx\n", hr);
    ok(buf != NULL, "NULL buffer returned\n");

    hr = IAudioRenderClient_ReleaseBuffer(arc, psize+1, AUDCLNT_BUFFERFLAGS_SILENT);
    ok(hr == AUDCLNT_E_INVALID_SIZE, "ReleaseBuffer too large error: %08lx\n", hr);
    /* todo_wine means Wine may overwrite memory */
    if(hr == S_OK) written += psize+1;

    /* Buffer still hold */
    hr = IAudioRenderClient_ReleaseBuffer(arc, psize/2, AUDCLNT_BUFFERFLAGS_SILENT);
    ok(hr == S_OK, "ReleaseBuffer after error: %08lx\n", hr);
    if(hr == S_OK) written += psize/2;

    hr = IAudioRenderClient_ReleaseBuffer(arc, 0, 0);
    ok(hr == S_OK, "ReleaseBuffer 0 gave wrong error: %08lx\n", hr);

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding failed: %08lx\n", hr);
    ok(pad == written, "GetCurrentPadding returned %u, should be %u\n", pad, written);

    CoTaskMemFree(pwfx);

    IAudioRenderClient_Release(arc);
    IAudioClient_Release(ac);
}

static void test_clock(int share)
{
    HRESULT hr;
    IAudioClient *ac;
    IAudioClock *acl;
    IAudioRenderClient *arc;
    UINT64 freq, pos, pcpos0, pcpos, last;
    UINT32 pad, gbsize, bufsize, fragment, parts, avail, slept = 0, sum = 0;
    BYTE *data;
    WAVEFORMATEX *pwfx;
    LARGE_INTEGER hpctime, hpctime0, hpcfreq;
    REFERENCE_TIME minp, defp, t1, t2;
    REFERENCE_TIME duration = 5000000, period = 150000;
    int i;

    ok(QueryPerformanceFrequency(&hpcfreq), "PerfFrequency failed\n");

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetDevicePeriod(ac, &defp, &minp);
    ok(hr == S_OK, "GetDevicePeriod failed: %08lx\n", hr);
    ok(minp <= period, "desired period %lu too small for %lu\n", (ULONG)period, (ULONG)minp);

    if (share) {
        trace("Testing shared mode\n");
        /* period is ignored */
        hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED,
                0, duration, period, pwfx, NULL);
        period = defp;
    } else {
        pwfx->wFormatTag = WAVE_FORMAT_PCM;
        pwfx->nChannels = 2;
        pwfx->cbSize = 0;
        pwfx->wBitsPerSample = 16; /* no floating point */
        pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
        pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;
        trace("Testing exclusive mode at %lu\n", pwfx->nSamplesPerSec);

        hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_EXCLUSIVE,
                0, duration, period, pwfx, NULL);
    }
    ok(share ? hr == S_OK : hr == hexcl || hr == AUDCLNT_E_DEVICE_IN_USE, "Initialize failed: %08lx\n", hr);
    if (hr != S_OK) {
        CoTaskMemFree(pwfx);
        IAudioClient_Release(ac);
        if(hr == AUDCLNT_E_DEVICE_IN_USE)
            skip("Device in use, no %s access\n", share ? "shared" : "exclusive");
        return;
    }

    /** GetStreamLatency
     * Shared mode: 1x period + a little, but some 192000 devices return 5.3334ms.
     * Exclusive mode: testbot returns 2x period + a little, but
     * some HDA drivers return 1x period, some + a little. */
    hr = IAudioClient_GetStreamLatency(ac, &t2);
    ok(hr == S_OK, "GetStreamLatency failed: %08lx\n", hr);
    trace("Latency: %u.%04u ms\n", (UINT)(t2/10000), (UINT)(t2 % 10000));
    ok(t2 >= period || broken(t2 >= period/2 && share && pwfx->nSamplesPerSec > 48000) ||
            broken(t2 == 0) /* win10 */,
       "Latency < default period, delta %ldus\n", (long)((t2-period)/10));

    /** GetBufferSize
     * BufferSize must be rounded up, maximum 2s says MSDN.
     * Both is wrong.  Rounding may lead to size a little smaller than duration;
     * duration > 2s is accepted in shared mode.
     * Shared mode: round solely w.r.t. mixer rate,
     *              duration is no multiple of period.
     * Exclusive mode: size appears as a multiple of some fragment that
     * is either the rounded period or a fixed constant like 1024,
     * whatever the driver implements. */
    hr = IAudioClient_GetBufferSize(ac, &gbsize);
    ok(hr == S_OK, "GetBufferSize failed: %08lx\n", hr);

    bufsize   =  MulDiv(duration, pwfx->nSamplesPerSec, 10000000);
    fragment  =  MulDiv(period,   pwfx->nSamplesPerSec, 10000000);
    parts     =  MulDiv(bufsize, 1, fragment); /* instead of (duration, 1, period) */
    trace("BufferSize %u estimated fragment %u x %u = %u\n", gbsize, fragment, parts, fragment * parts);
    /* fragment size (= period in frames) is rounded up.
     * BufferSize must be rounded up, maximum 2s says MSDN
     * but it is rounded down modulo fragment ! */
    if (share)
        ok(gbsize == bufsize,
           "BufferSize %u at rate %lu\n", gbsize, pwfx->nSamplesPerSec);
    else
        flaky
        ok(gbsize == parts * fragment || gbsize == MulDiv(bufsize, 1, 1024) * 1024,
           "BufferSize %u misfits fragment size %u at rate %lu\n", gbsize, fragment, pwfx->nSamplesPerSec);

    /* In shared mode, GetCurrentPadding decreases in multiples of
     * fragment size (i.e. updated only at period ticks), whereas
     * GetPosition appears to be reporting continuous positions.
     * In exclusive mode, testbot behaves likewise, but native's Intel
     * HDA driver shows no such deltas, GetCurrentPadding closely
     * matches GetPosition, as in
     * GetCurrentPadding = GetPosition - frames held in mmdevapi */

    hr = IAudioClient_GetService(ac, &IID_IAudioClock, (void**)&acl);
    ok(hr == S_OK, "GetService(IAudioClock) failed: %08lx\n", hr);

    hr = IAudioClock_GetFrequency(acl, &freq);
    ok(hr == S_OK, "GetFrequency failed: %08lx\n", hr);
    trace("Clock Frequency %u\n", (UINT)freq);

    /* MSDN says it's arbitrary units, but shared mode is unlikely to change */
    if (share)
        ok(freq == pwfx->nSamplesPerSec * pwfx->nBlockAlign,
           "Clock Frequency %u\n", (UINT)freq);
    else
        ok(freq == pwfx->nSamplesPerSec,
           "Clock Frequency %u\n", (UINT)freq);

    hr = IAudioClock_GetPosition(acl, NULL, NULL);
    ok(hr == E_POINTER, "GetPosition wrong error: %08lx\n", hr);

    pcpos0 = 0;
    hr = IAudioClock_GetPosition(acl, &pos, &pcpos0);
    ok(hr == S_OK, "GetPosition failed: %08lx\n", hr);
    ok(pos == 0, "GetPosition returned non-zero pos before being started\n");
    ok(pcpos0 != 0, "GetPosition returned zero pcpos\n");

    hr = IAudioClient_GetService(ac, &IID_IAudioRenderClient, (void**)&arc);
    ok(hr == S_OK, "GetService(IAudioRenderClient) failed: %08lx\n", hr);

    hr = IAudioRenderClient_GetBuffer(arc, gbsize+1, &data);
    ok(hr == AUDCLNT_E_BUFFER_TOO_LARGE, "GetBuffer too large failed: %08lx\n", hr);

    avail = gbsize;
    data = NULL;
    hr = IAudioRenderClient_GetBuffer(arc, avail, &data);
    ok(hr == S_OK, "GetBuffer failed: %08lx\n", hr);
    trace("data at %p\n", data);

    hr = IAudioRenderClient_ReleaseBuffer(arc, avail, winetest_debug>2 ?
        wave_generate_tone(pwfx, data, avail) : AUDCLNT_BUFFERFLAGS_SILENT);
    ok(hr == S_OK, "ReleaseBuffer failed: %08lx\n", hr);
    if(hr == S_OK) sum += avail;

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding failed: %08lx\n", hr);
    ok(pad == sum, "padding %u prior to start\n", pad);

    hr = IAudioClock_GetPosition(acl, &pos, NULL);
    ok(hr == S_OK, "GetPosition failed: %08lx\n", hr);
    ok(pos == 0, "GetPosition returned non-zero pos before being started\n");

    hr = IAudioClient_Start(ac); /* #1 */
    ok(hr == S_OK, "Start failed: %08lx\n", hr);

    Sleep(100);
    slept += 100;

    hr = IAudioClient_GetStreamLatency(ac, &t1);
    ok(hr == S_OK, "GetStreamLatency failed: %08lx\n", hr);
    ok(t1 == t2, "Latency not constant, delta %ld\n", (long)(t1-t2));

    hr = IAudioClock_GetPosition(acl, &pos, NULL);
    ok(hr == S_OK, "GetPosition failed: %08lx\n", hr);
    ok(pos > 0, "Position %u vs. last %u\n", (UINT)pos,0);
    /* in rare cases is slept*1.1 not enough with dmix */
    flaky
    ok(pos*1000/freq <= slept*1.4, "Position %u too far after playing %ums\n", (UINT)pos, slept);
    last = pos;

    hr = IAudioClient_Stop(ac);
    ok(hr == S_OK, "Stop failed: %08lx\n", hr);

    hr = IAudioClock_GetPosition(acl, &pos, NULL);
    ok(hr == S_OK, "GetPosition failed: %08lx\n", hr);
    ok(pos >= last, "Position %u vs. last %u\n", (UINT)pos,(UINT)last);
    last = pos;
    if(/*share &&*/ winetest_debug>1)
        ok(pos*1000/freq <= slept*1.1, "Position %u too far after stop %ums\n", (UINT)pos, slept);

    hr = IAudioClient_Start(ac); /* #2 */
    ok(hr == S_OK, "Start failed: %08lx\n", hr);

    Sleep(100);
    slept += 100;

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding failed: %08lx\n", hr);
    trace("padding %u past sleep #2\n", pad);

    /** IAudioClient_Stop
     * Exclusive mode: the audio engine appears to drop frames,
     * bumping GetPosition to a higher value than time allows, even
     * allowing GetPosition > sum Released - GetCurrentPadding (testbot)
     * Shared mode: no drop observed (or too small to be visible).
     * GetPosition = sum Released - GetCurrentPadding
     * Bugs: Some USB headset system drained the whole buffer, leaving
     *       padding 0 and bumping pos to sum minus 17 frames! */

    hr = IAudioClient_Stop(ac);
    ok(hr == S_OK, "Stop failed: %08lx\n", hr);

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding failed: %08lx\n", hr);

    hr = IAudioClock_GetPosition(acl, &pos, NULL);
    ok(hr == S_OK, "GetPosition failed: %08lx\n", hr);
    trace("padding %u position %u past stop #2\n", pad, (UINT)pos);
    ok(pos * pwfx->nSamplesPerSec <= sum * freq, "Position %u > written %u\n", (UINT)pos, sum);
    /* Prove that Stop must not drop frames (in shared mode). */
    ok(pad ? pos > last : pos >= last, "Position %u vs. last %u\n", (UINT)pos,(UINT)last);
    if (share && pad > 0 && winetest_debug>1)
        ok(pos*1000/freq <= slept*1.1, "Position %u too far after playing %ums\n", (UINT)pos, slept);
    /* in exclusive mode, testbot's w7 machines yield pos > sum-pad */
    if(/*share &&*/ winetest_debug>1)
        ok(pos * pwfx->nSamplesPerSec == (sum-pad) * freq,
           "Position %u after stop vs. %u padding\n", (UINT)pos, pad);
    last = pos;

    Sleep(100);

    hr = IAudioClock_GetPosition(acl, &pos, NULL);
    ok(hr == S_OK, "GetPosition failed: %08lx\n", hr);
    ok(pos == last, "Position %u should stop.\n", (UINT)pos);

    /* Restart from 0 */
    hr = IAudioClient_Reset(ac);
    ok(hr == S_OK, "Reset failed: %08lx\n", hr);
    slept = sum = 0;

    hr = IAudioClient_Reset(ac);
    ok(hr == S_OK, "Reset on an already reset stream returns %08lx\n", hr);

    hr = IAudioClock_GetPosition(acl, &pos, &pcpos);
    ok(hr == S_OK, "GetPosition failed: %08lx\n", hr);
    ok(pos == 0, "GetPosition returned non-zero pos after Reset\n");
    ok(pcpos > pcpos0, "pcpos should increase\n");

    avail = gbsize; /* implies GetCurrentPadding == 0 */
    hr = IAudioRenderClient_GetBuffer(arc, avail, &data);
    ok(hr == S_OK, "GetBuffer failed: %08lx\n", hr);
    trace("data at %p\n", data);

    hr = IAudioRenderClient_ReleaseBuffer(arc, avail, winetest_debug>2 ?
        wave_generate_tone(pwfx, data, avail) : AUDCLNT_BUFFERFLAGS_SILENT);
    ok(hr == S_OK, "ReleaseBuffer failed: %08lx\n", hr);
    if(hr == S_OK) sum += avail;

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding failed: %08lx\n", hr);
    ok(pad == sum, "padding %u prior to start\n", pad);

    hr = IAudioClock_GetPosition(acl, &pos, NULL);
    ok(hr == S_OK, "GetPosition failed: %08lx\n", hr);
    ok(pos == 0, "GetPosition returned non-zero pos after Reset\n");
    last = pos;

    hr = IAudioClient_Start(ac); /* #3 */
    ok(hr == S_OK, "Start failed: %08lx\n", hr);

    Sleep(100);
    slept += 100;

    hr = IAudioClock_GetPosition(acl, &pos, NULL);
    ok(hr == S_OK, "GetPosition failed: %08lx\n", hr);
    trace("position %u past %ums sleep #3\n", (UINT)pos, slept);
    ok(pos > last, "Position %u vs. last %u\n", (UINT)pos,(UINT)last);
    ok(pos * pwfx->nSamplesPerSec <= sum * freq, "Position %u > written %u\n", (UINT)pos, sum);
    if (winetest_debug>1)
        ok(pos*1000/freq <= slept*1.1, "Position %u too far after playing %ums\n", (UINT)pos, slept);
    else
        skip("Rerun with WINETEST_DEBUG=2 for GetPosition tests.\n");
    last = pos;

    hr = IAudioClient_Reset(ac);
    ok(hr == AUDCLNT_E_NOT_STOPPED, "Reset while playing: %08lx\n", hr);

    hr = IAudioClient_Stop(ac);
    ok(hr == S_OK, "Stop failed: %08lx\n", hr);

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding failed: %08lx\n", hr);

    hr = IAudioClock_GetPosition(acl, &pos, &pcpos);
    ok(hr == S_OK, "GetPosition failed: %08lx\n", hr);
    trace("padding %u position %u past stop #3\n", pad, (UINT)pos);
    ok(pos >= last, "Position %u vs. last %u\n", (UINT)pos,(UINT)last);
    ok(pcpos > pcpos0, "pcpos should increase\n");
    ok(pos * pwfx->nSamplesPerSec <= sum * freq, "Position %u > written %u\n", (UINT)pos, sum);
    if (pad > 0 && winetest_debug>1)
        ok(pos*1000/freq <= slept*1.1, "Position %u too far after stop %ums\n", (UINT)pos, slept);
    if(winetest_debug>1)
        ok(pos * pwfx->nSamplesPerSec == (sum-pad) * freq,
           "Position %u after stop vs. %u padding\n", (UINT)pos, pad);
    last = pos;

    /* Begin the big loop */
    hr = IAudioClient_Reset(ac);
    ok(hr == S_OK, "Reset failed: %08lx\n", hr);
    slept = last = sum = 0;
    pcpos0 = pcpos;

    ok(QueryPerformanceCounter(&hpctime0), "PerfCounter unavailable\n");

    hr = IAudioClient_Reset(ac);
    ok(hr == S_OK, "Reset on an already reset stream returns %08lx\n", hr);

    hr = IAudioClient_Start(ac);
    ok(hr == S_OK, "Start failed: %08lx\n", hr);

    avail = pwfx->nSamplesPerSec * 15 / 16 / 2;
    data = NULL;
    hr = IAudioRenderClient_GetBuffer(arc, avail, &data);
    ok(hr == S_OK, "GetBuffer failed: %08lx\n", hr);
    trace("data at %p for prefill %u\n", data, avail);

    if (winetest_debug>2) {
        hr = IAudioClient_Stop(ac);
        ok(hr == S_OK, "Stop failed: %08lx\n", hr);

        Sleep(20);
        slept += 20;

        hr = IAudioClient_Reset(ac);
        ok(hr == AUDCLNT_E_BUFFER_OPERATION_PENDING, "Reset failed: %08lx\n", hr);

        hr = IAudioClient_Start(ac);
        ok(hr == S_OK, "Start failed: %08lx\n", hr);
    }

    /* Despite passed time, data must still point to valid memory... */
    hr = IAudioRenderClient_ReleaseBuffer(arc, avail,
        wave_generate_tone(pwfx, data, avail));
    ok(hr == S_OK, "ReleaseBuffer after stop+start failed: %08lx\n", hr);
    if(hr == S_OK) sum += avail;

    /* GetCurrentPadding(GCP) == 0 does not mean an underrun happened, as the
     * mixer may still have a little data.  We believe an underrun will occur
     * when the mixer finds GCP smaller than a period size at the *end* of a
     * period cycle, i.e. shortly before calling SetEvent to signal the app
     * that it has ~10ms to supply data for the next cycle.  IOW, a zero GCP
     * with no data written for over a period causes an underrun. */

    Sleep(350);
    slept += 350;
    ok(QueryPerformanceCounter(&hpctime), "PerfCounter failed\n");
    trace("hpctime %lu after %ums\n",
        (ULONG)((hpctime.QuadPart-hpctime0.QuadPart)*1000/hpcfreq.QuadPart), slept);

    hr = IAudioClock_GetPosition(acl, &pos, &pcpos);
    ok(hr == S_OK, "GetPosition failed: %08lx\n", hr);
    ok(pos > last, "Position %u vs. last %u\n", (UINT)pos,(UINT)last);
    last = pos;

    for(i=0; i < 9; i++) {
        Sleep(100);
        slept += 100;

        hr = IAudioClock_GetPosition(acl, &pos, &pcpos);
        ok(hr == S_OK, "GetPosition failed: %08lx\n", hr);

        hr = IAudioClient_GetCurrentPadding(ac, &pad);
        ok(hr == S_OK, "GetCurrentPadding failed: %08lx\n", hr);

        ok(QueryPerformanceCounter(&hpctime), "PerfCounter failed\n");
        trace("hpctime %lu pcpos %lu\n",
              (ULONG)((hpctime.QuadPart-hpctime0.QuadPart)*1000/hpcfreq.QuadPart),
              (ULONG)((pcpos-pcpos0)/10000));

        /* Use sum-pad to see whether position is ahead padding or not. */
        trace("padding %u position %u/%u slept %ums iteration %d\n", pad, (UINT)pos, sum-pad, slept, i);
        ok(pad ? pos > last : pos >= last, "No position increase at iteration %d\n", i);
        ok(pos * pwfx->nSamplesPerSec <= sum * freq, "Position %u > written %u\n", (UINT)pos, sum);
        if (winetest_debug>1) {
            /* Padding does not lag behind by much */
            ok(pos * pwfx->nSamplesPerSec <= (sum-pad+fragment) * freq, "Position %u > written %u\n", (UINT)pos, sum);
            ok(pos*1000/freq <= slept*1.1, "Position %u too far after %ums\n", (UINT)pos, slept);
            if (pad) /* not in case of underrun */
                ok((pos-last)*1000/freq >= 90 && 110 >= (pos-last)*1000/freq,
                   "Position delta %ld not regular: %ld ms\n", (long)(pos-last), (long)((pos-last)*1000/freq));
        }
        last = pos;

        hr = IAudioClient_GetStreamLatency(ac, &t1);
        ok(hr == S_OK, "GetStreamLatency failed: %08lx\n", hr);
        ok(t1 == t2, "Latency not constant, delta %ld\n", (long)(t1-t2));

        avail = pwfx->nSamplesPerSec * 15 / 16 / 2;
        data = NULL;
        hr = IAudioRenderClient_GetBuffer(arc, avail, &data);
        /* ok(hr == AUDCLNT_E_BUFFER_TOO_LARGE || (hr == S_OK && i==0) without todo_wine */
        ok(hr == S_OK || hr == AUDCLNT_E_BUFFER_TOO_LARGE,
           "GetBuffer large (%u) failed: %08lx\n", avail, hr);
        /* In theory only the first iteration should allow that large a buffer
         * as prefill was drained during the first 350+100ms sleep.
         * Afterwards, only 100ms of data should find room per iteration.
         * However on some drivers a large buffer is allowed even at later
         * iterations, so we don't explicitly check this. */

        if(hr == S_OK) {
            trace("data at %p\n", data);
        } else {
            avail = gbsize - pad;
            hr = IAudioRenderClient_GetBuffer(arc, avail, &data);
            ok(hr == S_OK, "GetBuffer small %u failed: %08lx\n", avail, hr);
            trace("data at %p (small %u)\n", data, avail);
        }
        ok(data != NULL, "NULL buffer returned\n");
        if(i % 3 && !winetest_interactive) {
            memset(data, 0, avail * pwfx->nBlockAlign);
            hr = IAudioRenderClient_ReleaseBuffer(arc, avail, 0);
        } else {
            hr = IAudioRenderClient_ReleaseBuffer(arc, avail,
                wave_generate_tone(pwfx, data, avail));
        }
        ok(hr == S_OK, "ReleaseBuffer failed: %08lx\n", hr);
        if(hr == S_OK) sum += avail;
    }

    hr = IAudioClock_GetPosition(acl, &pos, NULL);
    ok(hr == S_OK, "GetPosition failed: %08lx\n", hr);
    trace("position %u\n", (UINT)pos);

    Sleep(1000); /* 500ms buffer underrun past full buffer */

    hr = IAudioClient_GetCurrentPadding(ac, &pad);
    ok(hr == S_OK, "GetCurrentPadding failed: %08lx\n", hr);

    hr = IAudioClock_GetPosition(acl, &pos, NULL);
    ok(hr == S_OK, "GetPosition failed: %08lx\n", hr);
    trace("position %u past underrun, %u padding left, %u frames written\n", (UINT)pos, pad, sum);

    if (share) {
        /* Following underrun, all samples were played */
        ok(pad == 0, "GetCurrentPadding returned %u, should be 0\n", pad);
        ok(pos * pwfx->nSamplesPerSec == sum * freq,
           "Position %u at end vs. %u submitted frames\n", (UINT)pos, sum);
    } else {
        /* Vista and w2k8 leave partial fragments behind */
        ok(pad == 0 /* w7, w2k8R2 */||
           pos * pwfx->nSamplesPerSec == (sum-pad) * freq, "GetCurrentPadding returned %u, should be 0\n", pad);
        /* expect at most 5 fragments (75ms) away */
        ok(pos * pwfx->nSamplesPerSec <= sum * freq &&
           pos * pwfx->nSamplesPerSec + 5 * fragment * freq >= sum * freq,
           "Position %u at end vs. %u submitted frames\n", (UINT)pos, sum);
    }

    hr = IAudioClient_GetStreamLatency(ac, &t1);
    ok(hr == S_OK, "GetStreamLatency failed: %08lx\n", hr);
    ok(t1 == t2, "Latency not constant, delta %ld\n", (long)(t1-t2));

    ok(QueryPerformanceCounter(&hpctime), "PerfCounter failed\n");
    trace("hpctime %lu after underrun\n", (ULONG)((hpctime.QuadPart-hpctime0.QuadPart)*1000/hpcfreq.QuadPart));

    hr = IAudioClient_Stop(ac);
    ok(hr == S_OK, "Stop failed: %08lx\n", hr);

    CoTaskMemFree(pwfx);

    IAudioClock_Release(acl);
    IAudioRenderClient_Release(arc);
    IAudioClient_Release(ac);
}

static void test_session(void)
{
    IAudioClient *ses1_ac1, *ses1_ac2, *cap_ac;
    IAudioSessionControl2 *ses1_ctl, *ses1_ctl2, *cap_ctl = NULL;
    IMMDevice *cap_dev;
    GUID ses1_guid;
    AudioSessionState state;
    WAVEFORMATEX *pwfx;
    ULONG ref;
    HRESULT hr;
    WCHAR *str;
    GUID guid1 = GUID_NULL, guid2 = GUID_NULL;

    hr = CoCreateGuid(&ses1_guid);
    ok(hr == S_OK, "CoCreateGuid failed: %08lx\n", hr);

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ses1_ac1);
    ok(hr == S_OK, "Activation failed with %08lx\n", hr);
    if (FAILED(hr)) return;

    hr = IAudioClient_GetMixFormat(ses1_ac1, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08lx\n", hr);

    hr = IAudioClient_Initialize(ses1_ac1, AUDCLNT_SHAREMODE_SHARED,
            0, 5000000, 0, pwfx, &ses1_guid);
    ok(hr == S_OK, "Initialize failed: %08lx\n", hr);

    if(hr == S_OK){
        hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
                NULL, (void**)&ses1_ac2);
        ok(hr == S_OK, "Activation failed with %08lx\n", hr);
    }
    if(hr != S_OK){
        skip("Unable to open the same device twice. Skipping session tests\n");

        ref = IAudioClient_Release(ses1_ac1);
        ok(ref == 0, "AudioClient wasn't released: %lu\n", ref);
        CoTaskMemFree(pwfx);
        return;
    }

    hr = IAudioClient_Initialize(ses1_ac2, AUDCLNT_SHAREMODE_SHARED,
            0, 5000000, 0, pwfx, &ses1_guid);
    ok(hr == S_OK, "Initialize failed: %08lx\n", hr);

    hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(mme, eCapture,
            eMultimedia, &cap_dev);
    if(hr == S_OK){
        hr = IMMDevice_Activate(cap_dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
                NULL, (void**)&cap_ac);
        ok((hr == S_OK)^(cap_ac == NULL), "Activate %08lx &out pointer\n", hr);
        ok(hr == S_OK, "Activate failed: %08lx\n", hr);
        IMMDevice_Release(cap_dev);
    }
    if(hr == S_OK){
        WAVEFORMATEX *cap_pwfx;

        hr = IAudioClient_GetMixFormat(cap_ac, &cap_pwfx);
        ok(hr == S_OK, "GetMixFormat failed: %08lx\n", hr);

        hr = IAudioClient_Initialize(cap_ac, AUDCLNT_SHAREMODE_SHARED,
                0, 5000000, 0, cap_pwfx, &ses1_guid);
        ok(hr == S_OK, "Initialize failed for capture in rendering session: %08lx\n", hr);
        CoTaskMemFree(cap_pwfx);
    }
    if(hr == S_OK){
        hr = IAudioClient_GetService(cap_ac, &IID_IAudioSessionControl, (void**)&cap_ctl);
        ok(hr == S_OK, "GetService failed: %08lx\n", hr);
        if(FAILED(hr))
            cap_ctl = NULL;
    }else
        skip("No capture session: %08lx; skipping capture device in render session tests\n", hr);

    hr = IAudioClient_GetService(ses1_ac1, &IID_IAudioSessionControl2, (void**)&ses1_ctl);
    ok(hr == E_NOINTERFACE, "GetService gave wrong error: %08lx\n", hr);

    hr = IAudioClient_GetService(ses1_ac1, &IID_IAudioSessionControl, (void**)&ses1_ctl);
    ok(hr == S_OK, "GetService failed: %08lx\n", hr);

    hr = IAudioClient_GetService(ses1_ac1, &IID_IAudioSessionControl, (void**)&ses1_ctl2);
    ok(hr == S_OK, "GetService failed: %08lx\n", hr);
    ok(ses1_ctl == ses1_ctl2, "Got different controls: %p %p\n", ses1_ctl, ses1_ctl2);
    ref = IAudioSessionControl2_Release(ses1_ctl2);
    ok(ref != 0, "AudioSessionControl was destroyed\n");

    hr = IAudioClient_GetService(ses1_ac2, &IID_IAudioSessionControl, (void**)&ses1_ctl2);
    ok(hr == S_OK, "GetService failed: %08lx\n", hr);

    hr = IAudioSessionControl2_GetState(ses1_ctl, NULL);
    ok(hr == NULL_PTR_ERR, "GetState gave wrong error: %08lx\n", hr);

    hr = IAudioSessionControl2_GetState(ses1_ctl, &state);
    ok(hr == S_OK, "GetState failed: %08lx\n", hr);
    ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

    hr = IAudioSessionControl2_GetState(ses1_ctl2, &state);
    ok(hr == S_OK, "GetState failed: %08lx\n", hr);
    ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

    if(cap_ctl){
        hr = IAudioSessionControl2_GetState(cap_ctl, &state);
        ok(hr == S_OK, "GetState failed: %08lx\n", hr);
        ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);
    }

    hr = IAudioClient_Start(ses1_ac1);
    ok(hr == S_OK, "Start failed: %08lx\n", hr);

    hr = IAudioSessionControl2_GetState(ses1_ctl, &state);
    ok(hr == S_OK, "GetState failed: %08lx\n", hr);
    ok(state == AudioSessionStateActive, "Got wrong state: %d\n", state);

    hr = IAudioSessionControl2_GetState(ses1_ctl2, &state);
    ok(hr == S_OK, "GetState failed: %08lx\n", hr);
    ok(state == AudioSessionStateActive, "Got wrong state: %d\n", state);

    if(cap_ctl){
        hr = IAudioSessionControl2_GetState(cap_ctl, &state);
        ok(hr == S_OK, "GetState failed: %08lx\n", hr);
        ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);
    }

    hr = IAudioClient_Stop(ses1_ac1);
    ok(hr == S_OK, "Stop failed: %08lx\n", hr);

    hr = IAudioSessionControl2_GetState(ses1_ctl, &state);
    ok(hr == S_OK, "GetState failed: %08lx\n", hr);
    ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

    hr = IAudioSessionControl2_GetState(ses1_ctl2, &state);
    ok(hr == S_OK, "GetState failed: %08lx\n", hr);
    ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

    /* Test GetDisplayName / SetDisplayName */

    hr = IAudioSessionControl2_GetDisplayName(ses1_ctl2, NULL);
    ok(hr == E_POINTER, "GetDisplayName failed: %08lx\n", hr);

    str = NULL;
    hr = IAudioSessionControl2_GetDisplayName(ses1_ctl2, &str);
    ok(hr == S_OK, "GetDisplayName failed: %08lx\n", hr);
    ok(str && !wcscmp(str, L""), "Got %s\n", wine_dbgstr_w(str));
    if (str)
        CoTaskMemFree(str);

    hr = IAudioSessionControl2_SetDisplayName(ses1_ctl2, NULL, NULL);
    ok(hr == HRESULT_FROM_WIN32(RPC_X_NULL_REF_POINTER), "SetDisplayName failed: %08lx\n", hr);

    hr = IAudioSessionControl2_SetDisplayName(ses1_ctl2, L"WineDisplayName", NULL);
    ok(hr == S_OK, "SetDisplayName failed: %08lx\n", hr);

    str = NULL;
    hr = IAudioSessionControl2_GetDisplayName(ses1_ctl2, &str);
    ok(hr == S_OK, "GetDisplayName failed: %08lx\n", hr);
    ok(str && !wcscmp(str, L"WineDisplayName"), "Got %s\n", wine_dbgstr_w(str));
    if (str)
        CoTaskMemFree(str);

    /* Test GetIconPath / SetIconPath */

    hr = IAudioSessionControl2_GetIconPath(ses1_ctl2, NULL);
    ok(hr == E_POINTER, "GetIconPath failed: %08lx\n", hr);

    str = NULL;
    hr = IAudioSessionControl2_GetIconPath(ses1_ctl2, &str);
    ok(hr == S_OK, "GetIconPath failed: %08lx\n", hr);
    ok(str && !wcscmp(str, L""), "Got %s\n", wine_dbgstr_w(str));
    if(str)
        CoTaskMemFree(str);

    hr = IAudioSessionControl2_SetIconPath(ses1_ctl2, NULL, NULL);
    ok(hr == HRESULT_FROM_WIN32(RPC_X_NULL_REF_POINTER), "SetIconPath failed: %08lx\n", hr);

    hr = IAudioSessionControl2_SetIconPath(ses1_ctl2, L"WineIconPath", NULL);
    ok(hr == S_OK, "SetIconPath failed: %08lx\n", hr);

    str = NULL;
    hr = IAudioSessionControl2_GetIconPath(ses1_ctl2, &str);
    ok(hr == S_OK, "GetIconPath failed: %08lx\n", hr);
    ok(str && !wcscmp(str, L"WineIconPath"), "Got %s\n", wine_dbgstr_w(str));
    if (str)
        CoTaskMemFree(str);

    /* Test GetGroupingParam / SetGroupingParam */

    hr = IAudioSessionControl2_GetGroupingParam(ses1_ctl2, NULL);
    ok(hr == HRESULT_FROM_WIN32(RPC_X_NULL_REF_POINTER), "GetGroupingParam failed: %08lx\n", hr);

    hr = IAudioSessionControl2_GetGroupingParam(ses1_ctl2, &guid1);
    ok(hr == S_OK, "GetGroupingParam failed: %08lx\n", hr);
    ok(!IsEqualGUID(&guid1, &guid2), "Expected non null GUID\n"); /* MSDN is wrong here, it is not GUID_NULL */

    hr = IAudioSessionControl2_SetGroupingParam(ses1_ctl2, NULL, NULL);
    ok(hr == HRESULT_FROM_WIN32(RPC_X_NULL_REF_POINTER), "SetGroupingParam failed: %08lx\n", hr);

    hr = CoCreateGuid(&guid2);
    ok(hr == S_OK, "CoCreateGuid failed: %08lx\n", hr);

    hr = IAudioSessionControl2_SetGroupingParam(ses1_ctl2, &guid2, NULL);
    ok(hr == S_OK, "SetGroupingParam failed: %08lx\n", hr);

    hr = IAudioSessionControl2_GetGroupingParam(ses1_ctl2, &guid1);
    ok(hr == S_OK, "GetGroupingParam failed: %08lx\n", hr);
    ok(IsEqualGUID(&guid1, &guid2), "Got %s\n", wine_dbgstr_guid(&guid1));

    /* Test capture */

    if(cap_ctl){
        hr = IAudioSessionControl2_GetState(cap_ctl, &state);
        ok(hr == S_OK, "GetState failed: %08lx\n", hr);
        ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

        hr = IAudioClient_Start(cap_ac);
        ok(hr == S_OK, "Start failed: %08lx\n", hr);

        hr = IAudioSessionControl2_GetState(ses1_ctl, &state);
        ok(hr == S_OK, "GetState failed: %08lx\n", hr);
        ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

        hr = IAudioSessionControl2_GetState(ses1_ctl2, &state);
        ok(hr == S_OK, "GetState failed: %08lx\n", hr);
        ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

        hr = IAudioSessionControl2_GetState(cap_ctl, &state);
        ok(hr == S_OK, "GetState failed: %08lx\n", hr);
        ok(state == AudioSessionStateActive, "Got wrong state: %d\n", state);

        hr = IAudioClient_Stop(cap_ac);
        ok(hr == S_OK, "Stop failed: %08lx\n", hr);

        hr = IAudioSessionControl2_GetState(ses1_ctl, &state);
        ok(hr == S_OK, "GetState failed: %08lx\n", hr);
        ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

        hr = IAudioSessionControl2_GetState(ses1_ctl2, &state);
        ok(hr == S_OK, "GetState failed: %08lx\n", hr);
        ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

        hr = IAudioSessionControl2_GetState(cap_ctl, &state);
        ok(hr == S_OK, "GetState failed: %08lx\n", hr);
        ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

        ref = IAudioSessionControl2_Release(cap_ctl);
        ok(ref == 0, "AudioSessionControl wasn't released: %lu\n", ref);

        ref = IAudioClient_Release(cap_ac);
        ok(ref == 0, "AudioClient wasn't released: %lu\n", ref);
    }

    ref = IAudioSessionControl2_Release(ses1_ctl);
    ok(ref == 0, "AudioSessionControl wasn't released: %lu\n", ref);

    ref = IAudioClient_Release(ses1_ac1);
    ok(ref == 0, "AudioClient wasn't released: %lu\n", ref);

    ref = IAudioClient_Release(ses1_ac2);
    ok(ref == 1, "AudioClient had wrong refcount: %lu\n", ref);

    /* we've released all of our IAudioClient references, so check GetState */
    hr = IAudioSessionControl2_GetState(ses1_ctl2, &state);
    ok(hr == S_OK, "GetState failed: %08lx\n", hr);
    ok(state == AudioSessionStateInactive, "Got wrong state: %d\n", state);

    ref = IAudioSessionControl2_Release(ses1_ctl2);
    ok(ref == 0, "AudioSessionControl wasn't released: %lu\n", ref);

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
    ok(hr == S_OK, "Activation failed with %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &fmt);
    ok(hr == S_OK, "GetMixFormat failed: %08lx\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000,
            0, fmt, NULL);
    ok(hr == S_OK, "Initialize failed: %08lx\n", hr);

    if(hr == S_OK){
        hr = IAudioClient_GetService(ac, &IID_IAudioStreamVolume, (void**)&asv);
        ok(hr == S_OK, "GetService failed: %08lx\n", hr);
    }
    if(hr != S_OK){
        IAudioClient_Release(ac);
        CoTaskMemFree(fmt);
        return;
    }

    hr = IAudioStreamVolume_GetChannelCount(asv, NULL);
    ok(hr == E_POINTER, "GetChannelCount gave wrong error: %08lx\n", hr);

    hr = IAudioStreamVolume_GetChannelCount(asv, &chans);
    ok(hr == S_OK, "GetChannelCount failed: %08lx\n", hr);
    ok(chans == fmt->nChannels, "GetChannelCount gave wrong number of channels: %d\n", chans);

    hr = IAudioStreamVolume_GetChannelVolume(asv, fmt->nChannels, NULL);
    ok(hr == E_POINTER, "GetChannelVolume gave wrong error: %08lx\n", hr);

    hr = IAudioStreamVolume_GetChannelVolume(asv, fmt->nChannels, &vol);
    ok(hr == E_INVALIDARG, "GetChannelVolume gave wrong error: %08lx\n", hr);

    hr = IAudioStreamVolume_GetChannelVolume(asv, 0, NULL);
    ok(hr == E_POINTER, "GetChannelVolume gave wrong error: %08lx\n", hr);

    hr = IAudioStreamVolume_GetChannelVolume(asv, 0, &vol);
    ok(hr == S_OK, "GetChannelVolume failed: %08lx\n", hr);
    ok(vol == 1.f, "Channel volume was not 1: %f\n", vol);

    hr = IAudioStreamVolume_SetChannelVolume(asv, fmt->nChannels, -1.f);
    ok(hr == E_INVALIDARG, "SetChannelVolume gave wrong error: %08lx\n", hr);

    hr = IAudioStreamVolume_SetChannelVolume(asv, 0, -1.f);
    ok(hr == E_INVALIDARG, "SetChannelVolume gave wrong error: %08lx\n", hr);

    hr = IAudioStreamVolume_SetChannelVolume(asv, 0, 2.f);
    ok(hr == E_INVALIDARG, "SetChannelVolume gave wrong error: %08lx\n", hr);

    hr = IAudioStreamVolume_SetChannelVolume(asv, 0, 0.2f);
    ok(hr == S_OK, "SetChannelVolume failed: %08lx\n", hr);

    hr = IAudioStreamVolume_GetChannelVolume(asv, 0, &vol);
    ok(hr == S_OK, "GetChannelVolume failed: %08lx\n", hr);
    ok(fabsf(vol - 0.2f) < 0.05f, "Channel volume wasn't 0.2: %f\n", vol);

    hr = IAudioStreamVolume_GetAllVolumes(asv, 0, NULL);
    ok(hr == E_POINTER, "GetAllVolumes gave wrong error: %08lx\n", hr);

    hr = IAudioStreamVolume_GetAllVolumes(asv, fmt->nChannels, NULL);
    ok(hr == E_POINTER, "GetAllVolumes gave wrong error: %08lx\n", hr);

    vols = HeapAlloc(GetProcessHeap(), 0, fmt->nChannels * sizeof(float));
    ok(vols != NULL, "HeapAlloc failed\n");

    hr = IAudioStreamVolume_GetAllVolumes(asv, fmt->nChannels - 1, vols);
    ok(hr == E_INVALIDARG, "GetAllVolumes gave wrong error: %08lx\n", hr);

    hr = IAudioStreamVolume_GetAllVolumes(asv, fmt->nChannels, vols);
    ok(hr == S_OK, "GetAllVolumes failed: %08lx\n", hr);
    ok(fabsf(vols[0] - 0.2f) < 0.05f, "Channel 0 volume wasn't 0.2: %f\n", vol);
    for(i = 1; i < fmt->nChannels; ++i)
        ok(vols[i] == 1.f, "Channel %d volume is not 1: %f\n", i, vols[i]);

    hr = IAudioStreamVolume_SetAllVolumes(asv, 0, NULL);
    ok(hr == E_POINTER, "SetAllVolumes gave wrong error: %08lx\n", hr);

    hr = IAudioStreamVolume_SetAllVolumes(asv, fmt->nChannels, NULL);
    ok(hr == E_POINTER, "SetAllVolumes gave wrong error: %08lx\n", hr);

    hr = IAudioStreamVolume_SetAllVolumes(asv, fmt->nChannels - 1, vols);
    ok(hr == E_INVALIDARG, "SetAllVolumes gave wrong error: %08lx\n", hr);

    hr = IAudioStreamVolume_SetAllVolumes(asv, fmt->nChannels, vols);
    ok(hr == S_OK, "SetAllVolumes failed: %08lx\n", hr);

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
    ok(hr == S_OK, "Activation failed with %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &fmt);
    ok(hr == S_OK, "GetMixFormat failed: %08lx\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_NOPERSIST, 5000000, 0, fmt, NULL);
    ok(hr == S_OK, "Initialize failed: %08lx\n", hr);

    if(hr == S_OK){
        hr = IAudioClient_GetService(ac, &IID_IChannelAudioVolume, (void**)&acv);
        ok(hr == S_OK, "GetService failed: %08lx\n", hr);
    }
    if(hr != S_OK){
        IAudioClient_Release(ac);
        CoTaskMemFree(fmt);
        return;
    }

    hr = IChannelAudioVolume_GetChannelCount(acv, NULL);
    ok(hr == NULL_PTR_ERR, "GetChannelCount gave wrong error: %08lx\n", hr);

    hr = IChannelAudioVolume_GetChannelCount(acv, &chans);
    ok(hr == S_OK, "GetChannelCount failed: %08lx\n", hr);
    ok(chans == fmt->nChannels, "GetChannelCount gave wrong number of channels: %d\n", chans);

    hr = IChannelAudioVolume_GetChannelVolume(acv, fmt->nChannels, NULL);
    ok(hr == NULL_PTR_ERR, "GetChannelVolume gave wrong error: %08lx\n", hr);

    hr = IChannelAudioVolume_GetChannelVolume(acv, fmt->nChannels, &vol);
    ok(hr == E_INVALIDARG, "GetChannelVolume gave wrong error: %08lx\n", hr);

    hr = IChannelAudioVolume_GetChannelVolume(acv, 0, NULL);
    ok(hr == NULL_PTR_ERR, "GetChannelVolume gave wrong error: %08lx\n", hr);

    hr = IChannelAudioVolume_GetChannelVolume(acv, 0, &vol);
    ok(hr == S_OK, "GetChannelVolume failed: %08lx\n", hr);
    ok(vol == 1.f, "Channel volume was not 1: %f\n", vol);

    hr = IChannelAudioVolume_SetChannelVolume(acv, fmt->nChannels, -1.f, NULL);
    ok(hr == E_INVALIDARG, "SetChannelVolume gave wrong error: %08lx\n", hr);

    hr = IChannelAudioVolume_SetChannelVolume(acv, 0, -1.f, NULL);
    ok(hr == E_INVALIDARG, "SetChannelVolume gave wrong error: %08lx\n", hr);

    hr = IChannelAudioVolume_SetChannelVolume(acv, 0, 2.f, NULL);
    ok(hr == E_INVALIDARG, "SetChannelVolume gave wrong error: %08lx\n", hr);

    hr = IChannelAudioVolume_SetChannelVolume(acv, 0, 0.2f, NULL);
    ok(hr == S_OK, "SetChannelVolume failed: %08lx\n", hr);

    hr = IChannelAudioVolume_GetChannelVolume(acv, 0, &vol);
    ok(hr == S_OK, "GetChannelVolume failed: %08lx\n", hr);
    ok(fabsf(vol - 0.2f) < 0.05f, "Channel volume wasn't 0.2: %f\n", vol);

    hr = IChannelAudioVolume_GetAllVolumes(acv, 0, NULL);
    ok(hr == NULL_PTR_ERR, "GetAllVolumes gave wrong error: %08lx\n", hr);

    hr = IChannelAudioVolume_GetAllVolumes(acv, fmt->nChannels, NULL);
    ok(hr == NULL_PTR_ERR, "GetAllVolumes gave wrong error: %08lx\n", hr);

    vols = HeapAlloc(GetProcessHeap(), 0, fmt->nChannels * sizeof(float));
    ok(vols != NULL, "HeapAlloc failed\n");

    hr = IChannelAudioVolume_GetAllVolumes(acv, fmt->nChannels - 1, vols);
    ok(hr == E_INVALIDARG, "GetAllVolumes gave wrong error: %08lx\n", hr);

    hr = IChannelAudioVolume_GetAllVolumes(acv, fmt->nChannels, vols);
    ok(hr == S_OK, "GetAllVolumes failed: %08lx\n", hr);
    ok(fabsf(vols[0] - 0.2f) < 0.05f, "Channel 0 volume wasn't 0.2: %f\n", vol);
    for(i = 1; i < fmt->nChannels; ++i)
        ok(vols[i] == 1.f, "Channel %d volume is not 1: %f\n", i, vols[i]);

    hr = IChannelAudioVolume_SetAllVolumes(acv, 0, NULL, NULL);
    ok(hr == NULL_PTR_ERR, "SetAllVolumes gave wrong error: %08lx\n", hr);

    hr = IChannelAudioVolume_SetAllVolumes(acv, fmt->nChannels, NULL, NULL);
    ok(hr == NULL_PTR_ERR, "SetAllVolumes gave wrong error: %08lx\n", hr);

    hr = IChannelAudioVolume_SetAllVolumes(acv, fmt->nChannels - 1, vols, NULL);
    ok(hr == E_INVALIDARG, "SetAllVolumes gave wrong error: %08lx\n", hr);

    hr = IChannelAudioVolume_SetAllVolumes(acv, fmt->nChannels, vols, NULL);
    ok(hr == S_OK, "SetAllVolumes failed: %08lx\n", hr);

    hr = IChannelAudioVolume_SetChannelVolume(acv, 0, 1.0f, NULL);
    ok(hr == S_OK, "SetChannelVolume failed: %08lx\n", hr);

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
    ok(hr == S_OK, "Activation failed with %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &fmt);
    ok(hr == S_OK, "GetMixFormat failed: %08lx\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_NOPERSIST, 5000000, 0, fmt, NULL);
    ok(hr == S_OK, "Initialize failed: %08lx\n", hr);

    if(hr == S_OK){
        hr = IAudioClient_GetService(ac, &IID_ISimpleAudioVolume, (void**)&sav);
        ok(hr == S_OK, "GetService failed: %08lx\n", hr);
    }
    if(hr != S_OK){
        IAudioClient_Release(ac);
        CoTaskMemFree(fmt);
        return;
    }

    hr = ISimpleAudioVolume_GetMasterVolume(sav, NULL);
    ok(hr == NULL_PTR_ERR, "GetMasterVolume gave wrong error: %08lx\n", hr);

    hr = ISimpleAudioVolume_GetMasterVolume(sav, &vol);
    ok(hr == S_OK, "GetMasterVolume failed: %08lx\n", hr);
    ok(vol == 1.f, "Master volume wasn't 1: %f\n", vol);

    hr = ISimpleAudioVolume_SetMasterVolume(sav, -1.f, NULL);
    ok(hr == E_INVALIDARG, "SetMasterVolume gave wrong error: %08lx\n", hr);

    hr = ISimpleAudioVolume_SetMasterVolume(sav, 2.f, NULL);
    ok(hr == E_INVALIDARG, "SetMasterVolume gave wrong error: %08lx\n", hr);

    hr = ISimpleAudioVolume_SetMasterVolume(sav, 0.2f, NULL);
    ok(hr == S_OK, "SetMasterVolume failed: %08lx\n", hr);

    hr = ISimpleAudioVolume_GetMasterVolume(sav, &vol);
    ok(hr == S_OK, "GetMasterVolume failed: %08lx\n", hr);
    ok(fabsf(vol - 0.2f) < 0.05f, "Master volume wasn't 0.2: %f\n", vol);

    hr = ISimpleAudioVolume_GetMute(sav, NULL);
    ok(hr == NULL_PTR_ERR, "GetMute gave wrong error: %08lx\n", hr);

    mute = TRUE;
    hr = ISimpleAudioVolume_GetMute(sav, &mute);
    ok(hr == S_OK, "GetMute failed: %08lx\n", hr);
    ok(mute == FALSE, "Session is already muted\n");

    hr = ISimpleAudioVolume_SetMute(sav, TRUE, NULL);
    ok(hr == S_OK, "SetMute failed: %08lx\n", hr);

    mute = FALSE;
    hr = ISimpleAudioVolume_GetMute(sav, &mute);
    ok(hr == S_OK, "GetMute failed: %08lx\n", hr);
    ok(mute == TRUE, "Session should have been muted\n");

    hr = ISimpleAudioVolume_GetMasterVolume(sav, &vol);
    ok(hr == S_OK, "GetMasterVolume failed: %08lx\n", hr);
    ok(fabsf(vol - 0.2f) < 0.05f, "Master volume wasn't 0.2: %f\n", vol);

    hr = ISimpleAudioVolume_SetMasterVolume(sav, 1.f, NULL);
    ok(hr == S_OK, "SetMasterVolume failed: %08lx\n", hr);

    mute = FALSE;
    hr = ISimpleAudioVolume_GetMute(sav, &mute);
    ok(hr == S_OK, "GetMute failed: %08lx\n", hr);
    ok(mute == TRUE, "Session should have been muted\n");

    hr = ISimpleAudioVolume_SetMute(sav, FALSE, NULL);
    ok(hr == S_OK, "SetMute failed: %08lx\n", hr);

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
    ok(hr == S_OK, "CoCreateGuid failed: %08lx\n", hr);

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &fmt);
    ok(hr == S_OK, "GetMixFormat failed: %08lx\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_NOPERSIST, 5000000, 0, fmt, &session);
    ok(hr == S_OK, "Initialize failed: %08lx\n", hr);

    if(hr == S_OK){
        hr = IAudioClient_GetService(ac, &IID_ISimpleAudioVolume, (void**)&sav);
        ok(hr == S_OK, "GetService (SimpleAudioVolume) failed: %08lx\n", hr);
    }
    if(hr != S_OK){
        IAudioClient_Release(ac);
        CoTaskMemFree(fmt);
        return;
    }

    hr = IAudioClient_GetService(ac, &IID_IChannelAudioVolume, (void**)&cav);
    ok(hr == S_OK, "GetService (ChannelAudioVolume) failed: %08lx\n", hr);

    hr = IAudioClient_GetService(ac, &IID_IAudioStreamVolume, (void**)&asv);
    ok(hr == S_OK, "GetService (AudioStreamVolume) failed: %08lx\n", hr);

    hr = IAudioStreamVolume_SetChannelVolume(asv, 0, 0.2f);
    ok(hr == S_OK, "ASV_SetChannelVolume failed: %08lx\n", hr);

    hr = IChannelAudioVolume_SetChannelVolume(cav, 0, 0.4f, NULL);
    ok(hr == S_OK, "CAV_SetChannelVolume failed: %08lx\n", hr);

    hr = ISimpleAudioVolume_SetMasterVolume(sav, 0.6f, NULL);
    ok(hr == S_OK, "SAV_SetMasterVolume failed: %08lx\n", hr);

    hr = IAudioStreamVolume_GetChannelVolume(asv, 0, &vol);
    ok(hr == S_OK, "ASV_GetChannelVolume failed: %08lx\n", hr);
    ok(fabsf(vol - 0.2f) < 0.05f, "ASV_GetChannelVolume gave wrong volume: %f\n", vol);

    hr = IChannelAudioVolume_GetChannelVolume(cav, 0, &vol);
    ok(hr == S_OK, "CAV_GetChannelVolume failed: %08lx\n", hr);
    ok(fabsf(vol - 0.4f) < 0.05f, "CAV_GetChannelVolume gave wrong volume: %f\n", vol);

    hr = ISimpleAudioVolume_GetMasterVolume(sav, &vol);
    ok(hr == S_OK, "SAV_GetMasterVolume failed: %08lx\n", hr);
    ok(fabsf(vol - 0.6f) < 0.05f, "SAV_GetMasterVolume gave wrong volume: %f\n", vol);

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac2);
    ok(hr == S_OK, "Activation failed with %08lx\n", hr);

    if(hr == S_OK){
        hr = IAudioClient_Initialize(ac2, AUDCLNT_SHAREMODE_SHARED,
                AUDCLNT_STREAMFLAGS_NOPERSIST, 5000000, 0, fmt, &session);
        ok(hr == S_OK, "Initialize failed: %08lx\n", hr);
        if(hr != S_OK)
            IAudioClient_Release(ac2);
    }

    if(hr == S_OK){
        IChannelAudioVolume *cav2;
        IAudioStreamVolume *asv2;

        hr = IAudioClient_GetService(ac2, &IID_IChannelAudioVolume, (void**)&cav2);
        ok(hr == S_OK, "GetService failed: %08lx\n", hr);

        hr = IAudioClient_GetService(ac2, &IID_IAudioStreamVolume, (void**)&asv2);
        ok(hr == S_OK, "GetService failed: %08lx\n", hr);

        hr = IChannelAudioVolume_GetChannelVolume(cav2, 0, &vol);
        ok(hr == S_OK, "CAV_GetChannelVolume failed: %08lx\n", hr);
        ok(fabsf(vol - 0.4f) < 0.05f, "CAV_GetChannelVolume gave wrong volume: %f\n", vol);

        hr = IAudioStreamVolume_GetChannelVolume(asv2, 0, &vol);
        ok(hr == S_OK, "ASV_GetChannelVolume failed: %08lx\n", hr);
        ok(vol == 1.f, "ASV_GetChannelVolume gave wrong volume: %f\n", vol);

        hr = IChannelAudioVolume_GetChannelCount(cav2, &nch);
        ok(hr == S_OK, "CAV_GetChannelCount failed: %08lx\n", hr);
        ok(nch == fmt->nChannels, "Got wrong channel count, expected %u: %u\n", fmt->nChannels, nch);

        hr = IAudioStreamVolume_GetChannelCount(asv2, &nch);
        ok(hr == S_OK, "ASV_GetChannelCount failed: %08lx\n", hr);
        ok(nch == fmt->nChannels, "Got wrong channel count, expected %u: %u\n", fmt->nChannels, nch);

        IAudioStreamVolume_Release(asv2);
        IChannelAudioVolume_Release(cav2);
        IAudioClient_Release(ac2);
    }else
        skip("Unable to open the same device twice. Skipping session volume control tests\n");

    hr = IChannelAudioVolume_SetChannelVolume(cav, 0, 1.f, NULL);
    ok(hr == S_OK, "CAV_SetChannelVolume failed: %08lx\n", hr);

    hr = ISimpleAudioVolume_SetMasterVolume(sav, 1.f, NULL);
    ok(hr == S_OK, "SAV_SetMasterVolume failed: %08lx\n", hr);

    CoTaskMemFree(fmt);
    ISimpleAudioVolume_Release(sav);
    IChannelAudioVolume_Release(cav);
    IAudioStreamVolume_Release(asv);
    IAudioClient_Release(ac);
}

#define check_session_ids(a, b, c) check_session_ids_(__LINE__, a, b, c)
static void check_session_ids_(unsigned int line, IMMDevice *dev, const GUID *session_guid, IAudioSessionControl *ctl)
{
    WCHAR exe_path[MAX_PATH], expected[MAX_PATH + 512], *dev_id, *str;
    IAudioSessionControl2 *ctl2;
    WCHAR guidstr[39];
    HRESULT hr;
    DWORD size;
    BOOL bret;
    int ret;

    if (FAILED(IAudioSessionControl_QueryInterface(ctl, &IID_IAudioSessionControl2, (void **)&ctl2)))
    {
        win_skip("IAudioSessionControl2 not available.\n");
        return;
    }

    ret = StringFromGUID2(session_guid, guidstr, ARRAY_SIZE(guidstr));
    ok(ret == 39, "got %d.\n", ret);

    size = ARRAY_SIZE(exe_path);
    bret = QueryFullProcessImageNameW(GetCurrentProcess(), PROCESS_NAME_NATIVE, exe_path, &size);
    ok(bret, "got error %ld.\n", GetLastError());

    hr = IMMDevice_GetId(dev, &dev_id);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = IAudioSessionControl2_GetSessionIdentifier(ctl2, &str);
    ok_(__FILE__, line)(hr == S_OK, "GetSessionIdentifier failed, hr %#lx.\n", hr);
    wsprintfW(expected, L"%s|%s%%b%s", dev_id, exe_path, guidstr);
    ok_(__FILE__, line)(!wcscmp(str, expected), "got %s, expected %s.\n", debugstr_w(str), debugstr_w(expected));
    CoTaskMemFree(str);

    hr = IAudioSessionControl2_GetSessionInstanceIdentifier(ctl2, &str);
    ok_(__FILE__, line)(hr == S_OK, "GetSessionInstanceIdentifier failed, hr %#lx.\n", hr);
    wsprintfW(expected, L"%s|%s%%b%s|1%%b%lu", dev_id, exe_path, guidstr, GetCurrentProcessId());
    ok_(__FILE__, line)(!wcscmp(str, expected), "got %s, expected %s.\n", debugstr_w(str), debugstr_w(expected));
    CoTaskMemFree(str);

    CoTaskMemFree(dev_id);
    IAudioSessionControl2_Release(ctl2);
}

static void test_session_creation(void)
{
    IMMDevice *cap_dev;
    IAudioClient *ac;
    IAudioSessionEnumerator *sess_enum, *sess_enum2;
    IAudioSessionManager2 *sesm2;
    IAudioSessionManager *sesm;
    ISimpleAudioVolume *sav;
    GUID session_guid, session_guid2;
    BOOL found_first, found_second;
    IAudioSessionControl *ctl;
    float vol;
    HRESULT hr;
    WAVEFORMATEX *fmt;
    int i, count;
    WCHAR *name;

    CoCreateGuid(&session_guid);

    hr = IMMDevice_Activate(dev, &IID_IAudioSessionManager,
            CLSCTX_INPROC_SERVER, NULL, (void**)&sesm);
    ok((hr == S_OK)^(sesm == NULL), "Activate %08lx &out pointer\n", hr);
    ok(hr == S_OK, "Activate failed: %08lx\n", hr);

    hr = IAudioSessionManager_GetSimpleAudioVolume(sesm, &session_guid,
            FALSE, &sav);
    ok(hr == S_OK, "GetSimpleAudioVolume failed: %08lx\n", hr);

    hr = ISimpleAudioVolume_SetMasterVolume(sav, 0.6f, NULL);
    ok(hr == S_OK, "SetMasterVolume failed: %08lx\n", hr);

    hr = IAudioSessionManager_GetAudioSessionControl(sesm, &session_guid, 0, &ctl);
    ok(hr == S_OK, "got %#lx.\n", hr);
    hr = IAudioSessionControl_SetDisplayName(ctl, L"test_session1", NULL);
    ok(hr == S_OK, "got %#lx.\n", hr);
    IAudioSessionControl_Release(ctl);

    hr = IAudioSessionManager_QueryInterface(sesm, &IID_IAudioSessionManager2, (void **)&sesm2);
    ok(hr == S_OK, "got %#lx.\n", hr);
    hr = IAudioSessionManager2_GetSessionEnumerator((void *)sesm2, &sess_enum);
    ok(hr == S_OK, "got %#lx.\n", hr);

    /* create another session after getting the first enumerarot. */
    CoCreateGuid(&session_guid2);
    hr = IAudioSessionManager_GetAudioSessionControl(sesm, &session_guid2, 0, &ctl);
    ok(hr == S_OK, "got %#lx.\n", hr);
    hr = IAudioSessionControl_SetDisplayName(ctl, L"test_session2", NULL);
    ok(hr == S_OK, "got %#lx.\n", hr);
    IAudioSessionControl_Release(ctl);

    hr = IAudioSessionManager2_GetSessionEnumerator(sesm2, &sess_enum2);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(sess_enum != sess_enum2, "got the same interface.\n");

    hr = IAudioSessionEnumerator_GetCount(sess_enum, &count);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(count, "got %d.\n", count);
    found_first = found_second = FALSE;
    for (i = 0; i < count; ++i)
    {
        hr = IAudioSessionEnumerator_GetSession(sess_enum, i, &ctl);
        ok(hr == S_OK, "got %#lx.\n", hr);
        hr = IAudioSessionControl_GetDisplayName(ctl, &name);
        ok(hr == S_OK, "got %#lx.\n", hr);
        if (!wcscmp(name, L"test_session1"))
            found_first = TRUE;
        if (!wcscmp(name, L"test_session2"))
            found_second = TRUE;
        CoTaskMemFree(name);
        IAudioSessionControl_Release(ctl);
    }
    ok(found_first && !found_second, "got %d, %d.\n", found_first, found_second);
    if (0)
    {
        /* random access violation on Win11. */
        IAudioSessionEnumerator_GetSession(sess_enum, count, &ctl);
    }

    hr = IAudioSessionEnumerator_GetCount(sess_enum2, &count);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(count, "got %d.\n", count);
    found_first = found_second = FALSE;
    for (i = 0; i < count; ++i)
    {
        hr = IAudioSessionEnumerator_GetSession(sess_enum2, i, &ctl);
        ok(hr == S_OK, "got %#lx.\n", hr);
        hr = IAudioSessionControl_GetDisplayName(ctl, &name);
        ok(hr == S_OK, "got %#lx.\n", hr);
        if (!wcscmp(name, L"test_session1"))
        {
            found_first = TRUE;
            check_session_ids(dev, &session_guid, ctl);
        }
        if (!wcscmp(name, L"test_session2"))
        {
            found_second = TRUE;
            check_session_ids(dev, &session_guid2, ctl);
        }
        CoTaskMemFree(name);
        IAudioSessionControl_Release(ctl);
    }
    ok(found_first && found_second, "got %d, %d.\n", found_first, found_second);
    IAudioSessionEnumerator_Release(sess_enum);
    IAudioSessionEnumerator_Release(sess_enum2);

    /* Release completely to show session persistence */
    ISimpleAudioVolume_Release(sav);
    IAudioSessionManager_Release(sesm);
    IAudioSessionManager2_Release(sesm2);

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
        ok((hr == S_OK)^(cap_sesm == NULL), "Activate %08lx &out pointer\n", hr);
        ok(hr == S_OK, "Activate failed: %08lx\n", hr);

        hr = IAudioSessionManager_GetSimpleAudioVolume(cap_sesm, &session_guid,
                FALSE, &cap_sav);
        ok(hr == S_OK, "GetSimpleAudioVolume failed: %08lx\n", hr);

        vol = 0.5f;
        hr = ISimpleAudioVolume_GetMasterVolume(cap_sav, &vol);
        ok(hr == S_OK, "GetMasterVolume failed: %08lx\n", hr);

        ISimpleAudioVolume_Release(cap_sav);
        IAudioSessionManager_Release(cap_sesm);

        hr = IMMDevice_Activate(cap_dev, &IID_IAudioClient,
                CLSCTX_INPROC_SERVER, NULL, (void**)&cap_ac);
        ok(hr == S_OK, "Activate failed: %08lx\n", hr);

        IMMDevice_Release(cap_dev);

        hr = IAudioClient_GetMixFormat(cap_ac, &cap_pwfx);
        ok(hr == S_OK, "GetMixFormat failed: %08lx\n", hr);

        hr = IAudioClient_Initialize(cap_ac, AUDCLNT_SHAREMODE_SHARED,
                0, 5000000, 0, cap_pwfx, &session_guid);
        ok(hr == S_OK, "Initialize failed: %08lx\n", hr);

        CoTaskMemFree(cap_pwfx);

        if(hr == S_OK){
            hr = IAudioClient_GetService(cap_ac, &IID_ISimpleAudioVolume,
                    (void**)&cap_sav);
            ok(hr == S_OK, "GetService failed: %08lx\n", hr);
        }
        if(hr == S_OK){
            vol = 0.5f;
            hr = ISimpleAudioVolume_GetMasterVolume(cap_sav, &vol);
            ok(hr == S_OK, "GetMasterVolume failed: %08lx\n", hr);

            ISimpleAudioVolume_Release(cap_sav);
        }

        IAudioClient_Release(cap_ac);
    }

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok((hr == S_OK)^(ac == NULL), "Activate %08lx &out pointer\n", hr);
    ok(hr == S_OK, "Activation failed with %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &fmt);
    ok(hr == S_OK, "GetMixFormat failed: %08lx\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_NOPERSIST, 5000000, 0, fmt, &session_guid);
    ok(hr == S_OK, "Initialize failed: %08lx\n", hr);

    hr = IAudioClient_GetService(ac, &IID_ISimpleAudioVolume, (void**)&sav);
    ok(hr == S_OK, "GetService failed: %08lx\n", hr);
    if(hr == S_OK){
        vol = 0.5f;
        hr = ISimpleAudioVolume_GetMasterVolume(sav, &vol);
        ok(hr == S_OK, "GetMasterVolume failed: %08lx\n", hr);
        ok(fabs(vol - 0.6f) < 0.05f, "Got wrong volume: %f\n", vol);

        ISimpleAudioVolume_Release(sav);
    }

    CoTaskMemFree(fmt);
    IAudioClient_Release(ac);
}

static void test_worst_case(void)
{
    HANDLE event;
    HRESULT hr;
    IAudioClient *ac;
    IAudioRenderClient *arc;
    IAudioClock *acl;
    WAVEFORMATEX *pwfx;
    REFERENCE_TIME defp;
    UINT64 freq, pos, pcpos0, pcpos;
    BYTE *data;
    DWORD r;
    UINT32 pad, fragment, sum;
    int i,j;

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08lx\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 500000, 0, pwfx, NULL);
    ok(hr == S_OK, "Initialize failed: %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetDevicePeriod(ac, &defp, NULL);
    ok(hr == S_OK, "GetDevicePeriod failed: %08lx\n", hr);

    fragment  =  MulDiv(defp,   pwfx->nSamplesPerSec, 10000000);

    event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(event != NULL, "CreateEvent failed\n");

    hr = IAudioClient_SetEventHandle(ac, event);
    ok(hr == S_OK, "SetEventHandle failed: %08lx\n", hr);

    hr = IAudioClient_GetService(ac, &IID_IAudioRenderClient, (void**)&arc);
    ok(hr == S_OK, "GetService(IAudioRenderClient) failed: %08lx\n", hr);

    hr = IAudioClient_GetService(ac, &IID_IAudioClock, (void**)&acl);
    ok(hr == S_OK, "GetService(IAudioClock) failed: %08lx\n", hr);

    hr = IAudioClock_GetFrequency(acl, &freq);
    ok(hr == S_OK, "GetFrequency failed: %08lx\n", hr);

    for(j = 0; j <= (winetest_interactive ? 9 : 2); j++){
        sum = 0;
        trace("Should play %lums continuous tone with fragment size %u.\n",
              (ULONG)(defp/100), fragment);

        hr = IAudioClock_GetPosition(acl, &pos, &pcpos0);
        ok(hr == S_OK, "GetPosition failed: %08lx\n", hr);

        /* XAudio2 prefills one period, play without it */
        if(winetest_debug>2){
            hr = IAudioRenderClient_GetBuffer(arc, fragment, &data);
            ok(hr == S_OK, "GetBuffer failed: %08lx\n", hr);

            hr = IAudioRenderClient_ReleaseBuffer(arc, fragment, AUDCLNT_BUFFERFLAGS_SILENT);
            ok(hr == S_OK, "ReleaseBuffer failed: %08lx\n", hr);
            if(hr == S_OK)
                sum += fragment;
        }

        hr = IAudioClient_Start(ac);
        ok(hr == S_OK, "Start failed: %08lx\n", hr);

        for(i = 0; i <= 99; i++){ /* 100 x 10ms = 1 second */
            r = WaitForSingleObject(event, 60 + defp / 10000);
            flaky_wine
            ok(r == WAIT_OBJECT_0, "Wait iteration %d gave %lx\n", i, r);

            /* the app has nearly one period time to feed data */
            Sleep((i % 10) * defp / 120000);

            hr = IAudioClient_GetCurrentPadding(ac, &pad);
            ok(hr == S_OK, "GetCurrentPadding failed: %08lx\n", hr);

            /* XAudio2 writes only when there's little data left */
            if(pad <= fragment){
                hr = IAudioRenderClient_GetBuffer(arc, fragment, &data);
                ok(hr == S_OK, "GetBuffer failed: %08lx\n", hr);

                hr = IAudioRenderClient_ReleaseBuffer(arc, fragment,
                       wave_generate_tone(pwfx, data, fragment));
                ok(hr == S_OK, "ReleaseBuffer failed: %08lx\n", hr);
                if(hr == S_OK)
                    sum += fragment;
            }
        }

        hr = IAudioClient_Stop(ac);
        ok(hr == S_OK, "Stop failed: %08lx\n", hr);

        hr = IAudioClient_GetCurrentPadding(ac, &pad);
        ok(hr == S_OK, "GetCurrentPadding failed: %08lx\n", hr);

        hr = IAudioClock_GetPosition(acl, &pos, &pcpos);
        ok(hr == S_OK, "GetPosition failed: %08lx\n", hr);

        Sleep(100);

        trace("Released %u=%ux%u -%u frames at %lu worth %ums in %lums\n",
              sum, sum/fragment, fragment, pad,
              pwfx->nSamplesPerSec, MulDiv(sum-pad, 1000, pwfx->nSamplesPerSec),
              (ULONG)((pcpos-pcpos0)/10000));

        ok(pos * pwfx->nSamplesPerSec == (sum-pad) * freq,
           "Position %u at end vs. %u-%u submitted frames\n", (UINT)pos, sum, pad);

        hr = IAudioClient_Reset(ac);
        ok(hr == S_OK, "Reset failed: %08lx\n", hr);

        Sleep(250);
    }

    CoTaskMemFree(pwfx);
    IAudioClient_Release(ac);
    IAudioClock_Release(acl);
    IAudioRenderClient_Release(arc);
}

static void test_marshal(void)
{
    IStream *pStream;
    IAudioClient *ac, *acDest;
    IAudioRenderClient *rc, *rcDest;
    WAVEFORMATEX *pwfx;
    HRESULT hr;

    /* IAudioRenderClient */
    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08lx\n", hr);

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED, 0, 5000000,
            0, pwfx, NULL);
    ok(hr == S_OK, "Initialize failed: %08lx\n", hr);

    CoTaskMemFree(pwfx);

    hr = IAudioClient_GetService(ac, &IID_IAudioRenderClient, (void**)&rc);
    ok(hr == S_OK, "GetService failed: %08lx\n", hr);
    if(hr != S_OK) {
        IAudioClient_Release(ac);
        return;
    }

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok(hr == S_OK, "CreateStreamOnHGlobal failed 0x%08lx\n", hr);

    /* marshal IAudioClient */

    hr = CoMarshalInterface(pStream, &IID_IAudioClient, (IUnknown*)ac, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok(hr == S_OK, "CoMarshalInterface IAudioClient failed 0x%08lx\n", hr);

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IAudioClient, (void **)&acDest);
    ok(hr == S_OK, "CoUnmarshalInterface IAudioClient failed 0x%08lx\n", hr);
    if (hr == S_OK)
        IAudioClient_Release(acDest);

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    /* marshal IAudioRenderClient */

    hr = CoMarshalInterface(pStream, &IID_IAudioRenderClient, (IUnknown*)rc, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok(hr == S_OK, "CoMarshalInterface IAudioRenderClient failed 0x%08lx\n", hr);

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IAudioRenderClient, (void **)&rcDest);
    ok(hr == S_OK, "CoUnmarshalInterface IAudioRenderClient failed 0x%08lx\n", hr);
    if (hr == S_OK)
        IAudioRenderClient_Release(rcDest);


    IStream_Release(pStream);

    IAudioClient_Release(ac);
    IAudioRenderClient_Release(rc);

}

static void test_endpointvolume(void)
{
    HRESULT hr;
    IAudioEndpointVolume *aev;
    float mindb, maxdb, increment, volume;
    BOOL mute;

    hr = IMMDevice_Activate(dev, &IID_IAudioEndpointVolume,
            CLSCTX_INPROC_SERVER, NULL, (void**)&aev);
    ok(hr == S_OK, "Activation failed with %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioEndpointVolume_GetVolumeRange(aev, &mindb, NULL, NULL);
    ok(hr == E_POINTER, "GetVolumeRange should have failed with E_POINTER: 0x%08lx\n", hr);

    hr = IAudioEndpointVolume_GetVolumeRange(aev, &mindb, &maxdb, &increment);
    ok(hr == S_OK, "GetVolumeRange failed: 0x%08lx\n", hr);
    trace("got range: [%f,%f]/%f\n", mindb, maxdb, increment);

    hr = IAudioEndpointVolume_SetMasterVolumeLevel(aev, mindb - increment, NULL);
    ok(hr == E_INVALIDARG, "SetMasterVolumeLevel failed: 0x%08lx\n", hr);

    hr = IAudioEndpointVolume_GetMasterVolumeLevel(aev, &volume);
    ok(hr == S_OK, "GetMasterVolumeLevel failed: 0x%08lx\n", hr);

    hr = IAudioEndpointVolume_SetMasterVolumeLevel(aev, volume, NULL);
    ok(hr == S_OK, "SetMasterVolumeLevel failed: 0x%08lx\n", hr);

    hr = IAudioEndpointVolume_GetMute(aev, &mute);
    ok(hr == S_OK, "GetMute failed: %08lx\n", hr);

    hr = IAudioEndpointVolume_SetMute(aev, mute, NULL);
    ok(hr == S_OK || hr == S_FALSE, "SetMute failed: %08lx\n", hr);

    IAudioEndpointVolume_Release(aev);
}

static void test_audio_clock_adjustment(void)
{
    HRESULT hr;
    IAudioClient *ac;
    IAudioClockAdjustment *aca;
    UINT bufsize, expected_bufsize;
    WAVEFORMATEX *pwfx;
    HANDLE event;

    const REFERENCE_TIME buffer_duration = 500000;

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&ac);
    ok(hr == S_OK, "Activation failed with %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetMixFormat(ac, &pwfx);
    ok(hr == S_OK, "GetMixFormat failed: %08lx\n", hr);

    expected_bufsize = (buffer_duration / 10000000.) * pwfx->nSamplesPerSec;

    hr = IAudioClient_Initialize(ac, AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_RATEADJUST, buffer_duration, 0, pwfx, NULL);
    ok(hr == S_OK, "Initialize failed: %08lx\n", hr);
    if(hr != S_OK)
        return;

    hr = IAudioClient_GetBufferSize(ac, &bufsize);
    ok(bufsize == expected_bufsize, "unexpected bufsize %d expected %d\n", bufsize, expected_bufsize);

    hr = IAudioClient_GetService(ac, &IID_IAudioClockAdjustment, (void**)&aca);
    ok(hr == S_OK, "IAudioClient_GetService(IID_IAudioClockAdjustment) returned %08lx\n", hr);

    event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(event != NULL, "CreateEvent failed\n");

    hr = IAudioClient_SetEventHandle(ac, event);
    ok(hr == S_OK, "SetEventHandle failed: %08lx\n", hr);

    hr = IAudioClient_Start(ac);
    ok(hr == S_OK, "Start failed: %08lx\n", hr);

    hr = IAudioClockAdjustment_SetSampleRate(aca, 48000.00f);
    ok(hr == S_OK, "SetSampleRate failed: %08lx\n", hr);

    /* Wait for frame processing */
    WaitForSingleObject(event, 1000);

    hr = IAudioClient_GetBufferSize(ac, &bufsize);
    ok(bufsize == expected_bufsize, "unexpected bufsize %d expected %d\n", bufsize, expected_bufsize);

    hr = IAudioClockAdjustment_SetSampleRate(aca, 44100.00f);
    ok(hr == S_OK, "SetSampleRate failed: %08lx\n", hr);

    /* Wait for frame processing */
    WaitForSingleObject(event, 1000);

    hr = IAudioClient_GetBufferSize(ac, &bufsize);
    ok(bufsize == expected_bufsize, "unexpected bufsize %d expected %d\n", bufsize, expected_bufsize);

    IAudioClockAdjustment_Release(aca);
    IAudioClient_Release(ac);
}

START_TEST(render)
{
    HRESULT hr;
    DWORD mode;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, (void**)&mme);
    if (FAILED(hr))
    {
        skip("mmdevapi not available: 0x%08lx\n", hr);
        goto cleanup;
    }

    hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(mme, eRender, eMultimedia, &dev);
    ok(hr == S_OK || hr == E_NOTFOUND, "GetDefaultAudioEndpoint failed: 0x%08lx\n", hr);
    if (hr != S_OK || !dev)
    {
        if (hr == E_NOTFOUND)
            skip("No sound card available\n");
        else
            skip("GetDefaultAudioEndpoint returns 0x%08lx\n", hr);
        goto cleanup;
    }

    test_audioclient();
    test_formats(AUDCLNT_SHAREMODE_EXCLUSIVE);
    test_formats(AUDCLNT_SHAREMODE_SHARED);
    test_references();
    test_marshal();
    if (GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &mode))
    {
        trace("Output to a MS-DOS console is particularly slow and disturbs timing.\n");
        trace("Please redirect output to a file.\n");
    }
    test_event();
    test_padding();
    test_clock(1);
    test_clock(0);
    test_session();
    test_streamvolume();
    test_channelvolume();
    test_simplevolume();
    test_volume_dependence();
    test_session_creation();
    test_worst_case();
    test_endpointvolume();
    test_audio_clock_adjustment();

    IMMDevice_Release(dev);

cleanup:
    if (mme)
        IMMDeviceEnumerator_Release(mme);
    CoUninitialize();
}
