/*
 * Copyright 2021 Arkadiusz Hiler for CodeWeavers
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

#include <math.h>
#include <stdio.h>

#include "wine/test.h"

#define COBJMACROS

#ifdef STANDALONE
#include "initguid.h"
#endif

#include "mmdeviceapi.h"
#include "spatialaudioclient.h"
#include "mmsystem.h"

static IMMDeviceEnumerator *mme = NULL;
static IMMDevice *dev = NULL;
static ISpatialAudioClient *sac = NULL;
static UINT32 max_dyn_count;
static HANDLE event;
static WAVEFORMATEX format;

static void test_formats(void)
{
    HRESULT hr;
    IAudioFormatEnumerator *afe;
    UINT32 format_count = 0;
    WAVEFORMATEX *fmt = NULL;

    hr = ISpatialAudioClient_GetSupportedAudioObjectFormatEnumerator(sac, &afe);
    ok(hr == S_OK, "Getting format enumerator failed: 0x%08lx\n", hr);

    hr = IAudioFormatEnumerator_GetCount(afe, &format_count);
    ok(hr == S_OK, "Getting format count failed: 0x%08lx\n", hr);
    ok(format_count == 1, "Got wrong format count, expected 1 got %u\n", format_count);

    hr = IAudioFormatEnumerator_GetFormat(afe, 0, &fmt);
    ok(hr == S_OK, "Getting format failed: 0x%08lx\n", hr);
    ok(fmt != NULL, "Expected to get non-NULL format\n");

    ok(fmt->wFormatTag == WAVE_FORMAT_IEEE_FLOAT, "Wrong format, expected WAVE_FORMAT_IEEE_FLOAT got %hx\n", fmt->wFormatTag);
    ok(fmt->nChannels == 1, "Wrong number of channels, expected 1 got %hu\n", fmt->nChannels);
    ok(fmt->nSamplesPerSec == 48000, "Wrong sample ret, expected 48000 got %lu\n", fmt->nSamplesPerSec);
    ok(fmt->wBitsPerSample == 32, "Wrong bits per sample, expected 32 got %hu\n", fmt->wBitsPerSample);
    ok(fmt->nBlockAlign == 4, "Wrong block align, expected 4 got %hu\n", fmt->nBlockAlign);
    ok(fmt->nAvgBytesPerSec == 192000, "Wrong avg bytes per sec, expected 192000 got %lu\n", fmt->nAvgBytesPerSec);
    ok(fmt->cbSize == 0, "Wrong cbSize for simple format, expected 0, got %hu\n", fmt->cbSize);

    memcpy(&format, fmt, sizeof(format));

    IAudioFormatEnumerator_Release(afe);
}

static void fill_activation_params(SpatialAudioObjectRenderStreamActivationParams *activation_params)
{
    activation_params->StaticObjectTypeMask =  \
                AudioObjectType_FrontLeft     |
                AudioObjectType_FrontRight    |
                AudioObjectType_FrontCenter   |
                AudioObjectType_LowFrequency  |
                AudioObjectType_SideLeft      |
                AudioObjectType_SideRight     |
                AudioObjectType_BackLeft      |
                AudioObjectType_BackRight     |
                AudioObjectType_TopFrontLeft  |
                AudioObjectType_TopFrontRight |
                AudioObjectType_TopBackLeft   |
                AudioObjectType_TopBackRight;

    activation_params->MinDynamicObjectCount = 0;
    activation_params->MaxDynamicObjectCount = 0;
    activation_params->Category = AudioCategory_GameEffects;
    activation_params->EventHandle = event;
    activation_params->NotifyObject = NULL;

    activation_params->ObjectFormat = &format;
}

typedef struct NotifyObject
{
    ISpatialAudioObjectRenderStreamNotify ISpatialAudioObjectRenderStreamNotify_iface;
    LONG ref;
} NotifyObject;

static WINAPI HRESULT notifyobj_QueryInterface(
        ISpatialAudioObjectRenderStreamNotify *This,
        REFIID riid,
        void **ppvObject)
{
    return S_OK;
}

static WINAPI ULONG notifyobj_AddRef(
        ISpatialAudioObjectRenderStreamNotify *This)
{
    NotifyObject *obj = CONTAINING_RECORD(This, NotifyObject, ISpatialAudioObjectRenderStreamNotify_iface);
    ULONG ref = InterlockedIncrement(&obj->ref);
    return ref;
}

static WINAPI ULONG notifyobj_Release(
        ISpatialAudioObjectRenderStreamNotify *This)
{
    NotifyObject *obj = CONTAINING_RECORD(This, NotifyObject, ISpatialAudioObjectRenderStreamNotify_iface);
    ULONG ref = InterlockedDecrement(&obj->ref);
    return ref;
}

static WINAPI HRESULT notifyobj_OnAvailableDynamicObjectCountChange(
        ISpatialAudioObjectRenderStreamNotify *This,
        ISpatialAudioObjectRenderStreamBase *stream,
        LONGLONG deadline,
        UINT32 object_count)
{
    ok(FALSE, "Expected to never be notified of dynamic object count change\n");
    return S_OK;
}

static const ISpatialAudioObjectRenderStreamNotifyVtbl notifyobjvtbl =
{
    notifyobj_QueryInterface,
    notifyobj_AddRef,
    notifyobj_Release,
    notifyobj_OnAvailableDynamicObjectCountChange
};

static void test_stream_activation(void)
{
    HRESULT hr;
    WAVEFORMATEX wrong_format;
    ISpatialAudioObjectRenderStream *sas = NULL;

    SpatialAudioObjectRenderStreamActivationParams activation_params;
    PROPVARIANT activation_params_prop;
    NotifyObject notify_object;

    PropVariantInit(&activation_params_prop);
    activation_params_prop.vt = VT_BLOB;
    activation_params_prop.blob.cbSize = sizeof(activation_params);
    activation_params_prop.blob.pBlobData = (BYTE*) &activation_params;

    /* correct params */
    fill_activation_params(&activation_params);
    hr = ISpatialAudioClient_ActivateSpatialAudioStream(sac, &activation_params_prop, &IID_ISpatialAudioObjectRenderStream, (void**)&sas);
    ok(hr == S_OK, "Failed to activate spatial audio stream: 0x%08lx\n", hr);
    ok(ISpatialAudioObjectRenderStream_Release(sas) == 0, "Expected to release the last reference\n");

    /* event handle */
    fill_activation_params(&activation_params);
    activation_params.EventHandle = NULL;
    hr = ISpatialAudioClient_ActivateSpatialAudioStream(sac, &activation_params_prop, &IID_ISpatialAudioObjectRenderStream, (void**)&sas);
    ok(hr == E_INVALIDARG, "Expected lack of no EventHandle to be invalid: 0x%08lx\n", hr);
    ok(sas == NULL, "Expected spatial audio stream to be set to NULL upon failed activation\n");

    activation_params.EventHandle = INVALID_HANDLE_VALUE;
    hr = ISpatialAudioClient_ActivateSpatialAudioStream(sac, &activation_params_prop, &IID_ISpatialAudioObjectRenderStream, (void**)&sas);
    ok(hr == E_INVALIDARG, "Expected INVALID_HANDLE_VALUE to be invalid: 0x%08lx\n", hr);
    ok(sas == NULL, "Expected spatial audio stream to be set to NULL upon failed activation\n");

    /* must use only queried sample rate */
    fill_activation_params(&activation_params);
    memcpy(&wrong_format, &format, sizeof(format));
    activation_params.ObjectFormat = &wrong_format;
    wrong_format.nSamplesPerSec = 44100;
    wrong_format.nAvgBytesPerSec = wrong_format.nSamplesPerSec * wrong_format.nBlockAlign;
    hr = ISpatialAudioClient_ActivateSpatialAudioStream(sac, &activation_params_prop, &IID_ISpatialAudioObjectRenderStream, (void**)&sas);
    ok(hr == AUDCLNT_E_UNSUPPORTED_FORMAT, "Expected format to be unsupported: 0x%08lx\n", hr);
    ok(sas == NULL, "Expected spatial audio stream to be set to NULL upon failed activation\n");

    /* dynamic objects are not supported */
    if (max_dyn_count == 0)
    {
        fill_activation_params(&activation_params);
        activation_params.StaticObjectTypeMask |= AudioObjectType_Dynamic;
        hr = ISpatialAudioClient_ActivateSpatialAudioStream(sac, &activation_params_prop, &IID_ISpatialAudioObjectRenderStream, (void**)&sas);
        ok(hr == E_INVALIDARG, "Expected dynamic objects type be invalid: 0x%08lx\n", hr);
        ok(sas == NULL, "Expected spatial audio stream to be set to NULL upon failed activation\n");
    }

    activation_params.MinDynamicObjectCount = max_dyn_count + 1;
    activation_params.MaxDynamicObjectCount = max_dyn_count + 1;
    hr = ISpatialAudioClient_ActivateSpatialAudioStream(sac, &activation_params_prop, &IID_ISpatialAudioObjectRenderStream, (void**)&sas);
    if (max_dyn_count)
        ok(hr == AUDCLNT_E_UNSUPPORTED_FORMAT, "Expected dynamic object count exceeding max to be unsupported: 0x%08lx\n", hr);
    else
        ok(hr == E_INVALIDARG, "Expected setting dynamic object count to be invalid: 0x%08lx\n", hr);

    /* ISpatialAudioObjectRenderStreamNotify */
    fill_activation_params(&activation_params);
    notify_object.ISpatialAudioObjectRenderStreamNotify_iface.lpVtbl = &notifyobjvtbl;
    notify_object.ref = 0;
    activation_params.NotifyObject = &notify_object.ISpatialAudioObjectRenderStreamNotify_iface;
    hr = ISpatialAudioClient_ActivateSpatialAudioStream(sac, &activation_params_prop, &IID_ISpatialAudioObjectRenderStream, (void**)&sas);
    ok(hr == S_OK, "Failed to activate spatial audio stream: 0x%08lx\n", hr);
    ok(notify_object.ref == 1, "Expected to get increased NotifyObject's ref count\n");
    ok(ISpatialAudioObjectRenderStream_Release(sas) == 0, "Expected to release the last reference\n");
    ok(notify_object.ref == 0, "Expected to get lowered NotifyObject's ref count\n");
}

static void test_audio_object_activation(void)
{
    HRESULT hr;
    BOOL is_active;
    ISpatialAudioObjectRenderStream *sas = NULL;
    ISpatialAudioObject *sao1, *sao2;

    SpatialAudioObjectRenderStreamActivationParams activation_params;
    PROPVARIANT activation_params_prop;

    PropVariantInit(&activation_params_prop);
    activation_params_prop.vt = VT_BLOB;
    activation_params_prop.blob.cbSize = sizeof(activation_params);
    activation_params_prop.blob.pBlobData = (BYTE*) &activation_params;

    fill_activation_params(&activation_params);
    activation_params.StaticObjectTypeMask &= ~AudioObjectType_FrontRight;
    hr = ISpatialAudioClient_ActivateSpatialAudioStream(sac, &activation_params_prop, &IID_ISpatialAudioObjectRenderStream, (void**)&sas);
    ok(hr == S_OK, "Failed to activate spatial audio stream: 0x%08lx\n", hr);

    hr = ISpatialAudioObjectRenderStream_ActivateSpatialAudioObject(sas, AudioObjectType_FrontLeft, &sao1);
    ok(hr == S_OK, "Failed to activate spatial audio object: 0x%08lx\n", hr);
    hr = ISpatialAudioObject_IsActive(sao1, &is_active);
    todo_wine ok(hr == S_OK, "Failed to check if spatial audio object is active: 0x%08lx\n", hr);
    if (hr == S_OK)
        ok(is_active, "Expected spatial audio object to be active\n");

    hr = ISpatialAudioObjectRenderStream_ActivateSpatialAudioObject(sas, AudioObjectType_FrontLeft, &sao2);
    ok(hr == SPTLAUDCLNT_E_OBJECT_ALREADY_ACTIVE, "Expected audio object to be already active: 0x%08lx\n", hr);

    hr = ISpatialAudioObjectRenderStream_ActivateSpatialAudioObject(sas, AudioObjectType_FrontRight, &sao2);
    ok(hr == SPTLAUDCLNT_E_STATIC_OBJECT_NOT_AVAILABLE, "Expected static object to be not available: 0x%08lx\n", hr);

    hr = ISpatialAudioObjectRenderStream_ActivateSpatialAudioObject(sas, AudioObjectType_Dynamic, &sao2);
    ok(hr == SPTLAUDCLNT_E_NO_MORE_OBJECTS, "Expected to not have no more dynamic objects: 0x%08lx\n", hr);

    ISpatialAudioObject_Release(sao1);
    ISpatialAudioObjectRenderStream_Release(sas);
}

static BOOL is_buffer_zeroed(const BYTE *buffer, UINT32 buffer_length)
{
    UINT32 i;

    for (i = 0; i < buffer_length; i++)
    {
        if (buffer[i] != 0)
            return FALSE;
    }

    return TRUE;
}

static void test_audio_object_buffers(void)
{
    UINT32 dyn_object_count, frame_count, max_frame_count, buffer_length;
    SpatialAudioObjectRenderStreamActivationParams activation_params;
    ISpatialAudioObjectRenderStream *sas = NULL;
    PROPVARIANT activation_params_prop;
    ISpatialAudioObject *sao[4];
    BYTE *buffer;
    INT i, j, k;
    HRESULT hr;

    PropVariantInit(&activation_params_prop);
    activation_params_prop.vt = VT_BLOB;
    activation_params_prop.blob.cbSize = sizeof(activation_params);
    activation_params_prop.blob.pBlobData = (BYTE*) &activation_params;

    fill_activation_params(&activation_params);
    hr = ISpatialAudioClient_ActivateSpatialAudioStream(sac, &activation_params_prop, &IID_ISpatialAudioObjectRenderStream, (void**)&sas);
    ok(hr == S_OK, "Failed to activate spatial audio stream: 0x%08lx\n", hr);

    hr = ISpatialAudioClient_GetMaxFrameCount(sac, &format, &max_frame_count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    frame_count = format.nSamplesPerSec / 100; /* 10ms */
    /* Most of the time the frame count matches the 10ms interval exactly.
     * However (seen on some Testbot machines) it might be a bit higher for some reason. */
    ok(max_frame_count <= frame_count + frame_count / 4, "Got unexpected frame count %u.\n", frame_count);

    /* The tests below which check frame count from _BeginUpdatingAudioObjects fail on some Testbot machines
     * with max_frame_count from _GetMaxFrameCount(). */
    max_frame_count = frame_count + frame_count / 4;

    hr = ISpatialAudioObjectRenderStream_ActivateSpatialAudioObject(sas, AudioObjectType_FrontLeft, &sao[0]);
    ok(hr == S_OK, "Failed to activate spatial audio object: 0x%08lx\n", hr);

    hr = ISpatialAudioObjectRenderStream_Start(sas);
    ok(hr == S_OK, "Failed to activate spatial audio render stream: 0x%08lx\n", hr);

    hr = ISpatialAudioObjectRenderStream_ActivateSpatialAudioObject(sas, AudioObjectType_FrontRight, &sao[1]);
    ok(hr == S_OK, "Failed to activate spatial audio object: 0x%08lx\n", hr);

    hr = WaitForSingleObject(event, 200);
    ok(hr == WAIT_OBJECT_0, "Expected event to be flagged: 0x%08lx\n", hr);

    hr = ISpatialAudioObjectRenderStream_ActivateSpatialAudioObject(sas, AudioObjectType_SideLeft, &sao[2]);
    ok(hr == S_OK, "Failed to activate spatial audio object: 0x%08lx\n", hr);

    hr = ISpatialAudioObjectRenderStream_BeginUpdatingAudioObjects(sas, &dyn_object_count, &frame_count);
    ok(hr == S_OK, "Failed to begin updating audio objects: 0x%08lx\n", hr);
    ok(dyn_object_count == 0, "Unexpected dynamic objects\n");
    ok(frame_count <= max_frame_count, "Got unexpected frame count %u.\n", frame_count);

    hr = ISpatialAudioObjectRenderStream_ActivateSpatialAudioObject(sas, AudioObjectType_SideRight, &sao[3]);
    ok(hr == S_OK, "Failed to activate spatial audio object: 0x%08lx\n", hr);

    for (i = 0; i < ARRAYSIZE(sao); i++)
    {
        hr = ISpatialAudioObject_GetBuffer(sao[i], &buffer, &buffer_length);
        ok(hr == S_OK, "Expected to be able to get buffers for audio object: 0x%08lx\n", hr);
        ok(buffer != NULL, "Expected to get a non-NULL buffer\n");
        ok(buffer_length == frame_count * format.wBitsPerSample / 8, "Expected buffer length to be sample_size * frame_count = %hu but got %u\n",
                frame_count * format.wBitsPerSample / 8, buffer_length);
        ok(is_buffer_zeroed(buffer, buffer_length), "Expected audio object's buffer to be zeroed\n");
    }

    hr = ISpatialAudioObjectRenderStream_EndUpdatingAudioObjects(sas);
    ok(hr == S_OK, "Failed to end updating audio objects: 0x%08lx\n", hr);

    /* Emulate underrun and test frame count approximate limit. */

    /* Force 1ms Sleep() timer resolution. */
    timeBeginPeriod(1);
    for (j = 0; j < 20; ++j)
    {
        hr = WaitForSingleObject(event, 200);
        ok(hr == WAIT_OBJECT_0, "Expected event to be flagged: 0x%08lx, j %u.\n", hr, j);

        hr = ISpatialAudioObjectRenderStream_BeginUpdatingAudioObjects(sas, &dyn_object_count, &frame_count);
        ok(hr == S_OK, "Failed to begin updating audio objects: 0x%08lx\n", hr);
        ok(dyn_object_count == 0, "Unexpected dynamic objects\n");
        ok(frame_count <= max_frame_count, "Got unexpected frame_count %u.\n", frame_count);

        /* Audio starts crackling with delays 10ms and above. However, setting such delay (that is, the delay
         * which skips the whole quantum) breaks SA on some Testbot machines: _BeginUpdatingAudioObjects fails
         * with SPTLAUDCLNT_E_INTERNAL starting from some iteration or WaitForSingleObject timeouts. That seems
         * to work on the real hardware though. */
        Sleep(5);

        for (i = 0; i < ARRAYSIZE(sao); i++)
        {
            hr = ISpatialAudioObject_GetBuffer(sao[i], &buffer, &buffer_length);
            ok(hr == S_OK, "Expected to be able to get buffers for audio object: 0x%08lx, i %d\n", hr, i);
            ok(buffer != NULL, "Expected to get a non-NULL buffer\n");
            ok(buffer_length == frame_count * format.wBitsPerSample / 8,
                    "Expected buffer length to be sample_size * frame_count = %hu but got %u\n",
                    frame_count * format.wBitsPerSample / 8, buffer_length);

            /* Enable to hear the test sound. */
            if (0)
            {
                if (format.wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
                {
                    for (k = 0; k < frame_count; ++k)
                    {
                        float time_sec = 10.0f / 1000.0f * (j + (float)k / frame_count);

                        /* 440Hz tone. */
                        ((float *)buffer)[k] = sinf(2.0f * M_PI * time_sec * 440.0f);
                    }
                }
            }
        }
        hr = ISpatialAudioObjectRenderStream_EndUpdatingAudioObjects(sas);
        ok(hr == S_OK, "Failed to end updating audio objects: 0x%08lx\n", hr);
    }
    timeEndPeriod(1);

    hr = WaitForSingleObject(event, 200);
    ok(hr == WAIT_OBJECT_0, "Expected event to be flagged: 0x%08lx\n", hr);

    hr = ISpatialAudioObjectRenderStream_BeginUpdatingAudioObjects(sas, &dyn_object_count, &frame_count);
    ok(hr == S_OK, "Failed to begin updating audio objects: 0x%08lx\n", hr);
    ok(dyn_object_count == 0, "Unexpected dynamic objects\n");

    /* one more iteration but not with every object */
    for (i = 0; i < ARRAYSIZE(sao) - 1; i++)
    {
        hr = ISpatialAudioObject_GetBuffer(sao[i], &buffer, &buffer_length);
        ok(hr == S_OK, "Expected to be able to get buffers for audio object: 0x%08lx\n", hr);
        ok(buffer != NULL, "Expected to get a non-NULL buffer\n");
        ok(buffer_length == frame_count * format.wBitsPerSample / 8, "Expected buffer length to be sample_size * frame_count = %hu but got %u\n",
                frame_count * format.wBitsPerSample / 8, buffer_length);
        ok(is_buffer_zeroed(buffer, buffer_length), "Expected audio object's buffer to be zeroed\n");
    }

    hr = ISpatialAudioObjectRenderStream_EndUpdatingAudioObjects(sas);
    ok(hr == S_OK, "Failed to end updating audio objects: 0x%08lx\n", hr);

    /* ending the stream */
    hr = ISpatialAudioObject_SetEndOfStream(sao[0], 0);
    todo_wine ok(hr == SPTLAUDCLNT_E_OUT_OF_ORDER, "Expected that ending the stream at this point won't be allowed: 0x%08lx\n", hr);

    hr = WaitForSingleObject(event, 200);
    ok(hr == WAIT_OBJECT_0, "Expected event to be flagged: 0x%08lx\n", hr);

    hr = ISpatialAudioObject_SetEndOfStream(sao[0], 0);
    todo_wine ok(hr == SPTLAUDCLNT_E_OUT_OF_ORDER, "Expected that ending the stream at this point won't be allowed: 0x%08lx\n", hr);

    hr = ISpatialAudioObjectRenderStream_BeginUpdatingAudioObjects(sas, &dyn_object_count, &frame_count);
    ok(hr == S_OK, "Failed to begin updating audio objects: 0x%08lx\n", hr);
    ok(dyn_object_count == 0, "Unexpected dynamic objects\n");

    /* expect the object that was not updated last cycle to be invalidated */
    hr = ISpatialAudioObject_GetBuffer(sao[ARRAYSIZE(sao) - 1], &buffer, &buffer_length);
    todo_wine ok(hr == SPTLAUDCLNT_E_RESOURCES_INVALIDATED, "Expected audio object to be invalidated: 0x%08lx\n", hr);

    for (i = 0; i < ARRAYSIZE(sao) - 1; i++)
    {
        hr = ISpatialAudioObject_GetBuffer(sao[i], &buffer, &buffer_length);
        ok(hr == S_OK, "Expected to be able to get buffers for audio object: 0x%08lx\n", hr);

        hr = ISpatialAudioObject_SetEndOfStream(sao[i], 0);
        todo_wine ok(hr == S_OK, "Failed to end the stream: 0x%08lx\n", hr);

        hr = ISpatialAudioObject_GetBuffer(sao[i], &buffer, &buffer_length);
        todo_wine ok(hr == SPTLAUDCLNT_E_RESOURCES_INVALIDATED, "Expected audio object to be invalidated: 0x%08lx\n", hr);
    }

    hr = ISpatialAudioObjectRenderStream_EndUpdatingAudioObjects(sas);
    ok(hr == S_OK, "Failed to end updating audio objects: 0x%08lx\n", hr);

    for (i = 0; i < ARRAYSIZE(sao); i++)
    {
        ISpatialAudioObject_Release(sao[i]);
    }

    ISpatialAudioObjectRenderStream_Release(sas);
}

START_TEST(spatialaudio)
{
    HRESULT hr;

    event = CreateEventA(NULL, FALSE, FALSE, "spatial-audio-test-prog-event");
    ok(event != NULL, "Failed to create event, last error: 0x%08lx\n", GetLastError());

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

    hr = IMMDevice_Activate(dev, &IID_ISpatialAudioClient, CLSCTX_INPROC_SERVER, NULL, (void**)&sac);
    ok(hr == S_OK || hr == E_NOINTERFACE, "ISpatialAudioClient Activation failed: 0x%08lx\n", hr);
    if (hr != S_OK || !dev)
    {
        if (hr == E_NOINTERFACE)
            skip("ISpatialAudioClient interface not found\n");
        else
            skip("ISpatialAudioClient Activation returns 0x%08lx\n", hr);
        goto cleanup;
    }

    hr = ISpatialAudioClient_GetMaxDynamicObjectCount(sac, &max_dyn_count);
    ok(hr == S_OK, "Failed to get max dynamic object count: 0x%08lx\n", hr);

    /* that's the default, after manually enabling Windows Sonic it's possible to have max_dyn_count > 0 */
    /* ok(max_dyn_count == 0, "expected max dynamic object count to be 0 got %u\n", max_dyn_count); */

    test_formats();
    test_stream_activation();
    test_audio_object_activation();
    test_audio_object_buffers();

    ISpatialAudioClient_Release(sac);

cleanup:
    if (dev)
        IMMDevice_Release(dev);
    if (mme)
        IMMDeviceEnumerator_Release(mme);
    CoUninitialize();
    CloseHandle(event);
}
