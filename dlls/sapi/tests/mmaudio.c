/*
 * Speech API (SAPI) winmm audio tests.
 *
 * Copyright 2023 Shaun Ren for CodeWeavers
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

#include "sapiddk.h"
#include "sperror.h"
#include "initguid.h"

#include "wine/test.h"

DEFINE_GUID(SPDFID_Text, 0x7ceef9f9, 0x3d13, 0x11d2, 0x9e, 0xe7, 0x00, 0xc0, 0x4f, 0x79, 0x73, 0x96);
DEFINE_GUID(SPDFID_WaveFormatEx, 0xc31adbae, 0x527f, 0x4ff5, 0xa2, 0x30, 0xf6, 0x2b, 0xb6, 0x1f, 0xf7, 0x0c);

static void test_interfaces(void)
{
    ISpMMSysAudio *mmaudio;
    IUnknown *unk;
    ISpEventSource *source;
    ISpEventSink *sink;
    ISpObjectWithToken *obj_with_token;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_SpMMAudioOut, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISpMMSysAudio, (void **)&mmaudio);
    ok(hr == S_OK, "Failed to create ISpMMSysAudio interface: %#lx.\n", hr);
    ISpMMSysAudio_Release(mmaudio);

    hr = CoCreateInstance(&CLSID_SpMMAudioOut, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Failed to create IUnknown interface: %#lx.\n", hr);
    IUnknown_Release(unk);

    hr = CoCreateInstance(&CLSID_SpMMAudioOut, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISpEventSource, (void **)&source);
    ok(hr == S_OK, "Failed to create ISpEventSource interface: %#lx.\n", hr);
    ISpEventSource_Release(source);

    hr = CoCreateInstance(&CLSID_SpMMAudioOut, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISpEventSink, (void **)&sink);
    ok(hr == S_OK, "Failed to create ISpEventSink interface: %#lx.\n", hr);
    ISpEventSink_Release(sink);

    hr = CoCreateInstance(&CLSID_SpMMAudioOut, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISpObjectWithToken, (void **)&obj_with_token);
    ok(hr == S_OK, "Failed to create ISpObjectWithToken interface: %#lx.\n", hr);
    ISpObjectWithToken_Release(obj_with_token);
}

static void test_device_id(void)
{
    ISpMMSysAudio *mmaudio;
    UINT id, num_devs;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_SpMMAudioOut, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISpMMSysAudio, (void **)&mmaudio);
    ok(hr == S_OK, "failed to create SpMMAudioOut instance: %#lx.\n", hr);

    id = 0xdeadbeef;
    hr = ISpMMSysAudio_GetDeviceId(mmaudio, &id);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(id == WAVE_MAPPER, "got %#x.\n", id);

    hr = ISpMMSysAudio_SetDeviceId(mmaudio, WAVE_MAPPER);
    ok(hr == S_OK, "got %#lx.\n", hr);

    num_devs = waveOutGetNumDevs();
    if (num_devs == 0) {
        skip("no wave out devices.\n");
        ISpMMSysAudio_Release(mmaudio);
        return;
    }

    hr = ISpMMSysAudio_SetDeviceId(mmaudio, num_devs);
    ok(hr == E_INVALIDARG, "got %#lx.\n", hr);

    hr = ISpMMSysAudio_SetDeviceId(mmaudio, 0);
    ok(hr == S_OK || broken(hr == S_FALSE) /* Windows */, "got %#lx.\n", hr);

    id = 0xdeadbeef;
    hr = ISpMMSysAudio_GetDeviceId(mmaudio, &id);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(id == 0, "got %u.\n", id);

    ISpMMSysAudio_Release(mmaudio);
}

static void test_formats(void)
{
    ISpMMSysAudio *mmaudio;
    GUID fmtid;
    WAVEFORMATEX *wfx;
    WAVEFORMATEX wfx2;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_SpMMAudioOut, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISpMMSysAudio, (void **)&mmaudio);
    ok(hr == S_OK, "failed to create SpMMAudioOut instance: %#lx.\n", hr);

    wfx = NULL;
    hr = ISpMMSysAudio_GetFormat(mmaudio, &fmtid, &wfx);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(IsEqualGUID(&fmtid, &SPDFID_WaveFormatEx), "got %s.\n", wine_dbgstr_guid(&fmtid));
    ok(wfx != NULL, "wfx == NULL.\n");
    ok(wfx->wFormatTag == WAVE_FORMAT_PCM, "got %u.\n", wfx->wFormatTag);
    ok(wfx->nChannels == 1, "got %u.\n", wfx->nChannels);
    ok(wfx->nSamplesPerSec == 22050, "got %lu.\n", wfx->nSamplesPerSec);
    ok(wfx->nAvgBytesPerSec == 22050 * 2, "got %lu.\n", wfx->nAvgBytesPerSec);
    ok(wfx->nBlockAlign == 2, "got %u.\n", wfx->nBlockAlign);
    ok(wfx->wBitsPerSample == 16, "got %u.\n", wfx->wBitsPerSample);
    ok(wfx->cbSize == 0, "got %u.\n", wfx->cbSize);
    CoTaskMemFree(wfx);

    hr = ISpMMSysAudio_SetFormat(mmaudio, NULL, NULL);
    ok(hr == E_INVALIDARG, "got %#lx.\n", hr);

    hr = ISpMMSysAudio_SetFormat(mmaudio, &SPDFID_Text, NULL);
    ok(hr == E_INVALIDARG, "got %#lx.\n", hr);

    hr = ISpMMSysAudio_SetFormat(mmaudio, &SPDFID_WaveFormatEx, NULL);
    ok(hr == E_INVALIDARG, "got %#lx.\n", hr);

    if (waveOutGetNumDevs() == 0) {
        skip("no wave out devices.\n");
        ISpMMSysAudio_Release(mmaudio);
        return;
    }

    wfx2.wFormatTag = WAVE_FORMAT_PCM;
    wfx2.nChannels = 2;
    wfx2.nSamplesPerSec = 16000;
    wfx2.nAvgBytesPerSec = 16000 * 2 * 2;
    wfx2.nBlockAlign = 2 * 2;
    wfx2.wBitsPerSample = 16;
    wfx2.cbSize = 0;

    hr = ISpMMSysAudio_SetFormat(mmaudio, &SPDFID_WaveFormatEx, &wfx2);
    ok(hr == S_OK, "got %#lx.\n", hr);

    ISpMMSysAudio_Release(mmaudio);
}

static void test_audio_out(void)
{
    ISpMMSysAudio *mmaudio;
    GUID fmtid;
    WAVEFORMATEX *wfx = NULL;
    WAVEFORMATEX wfx2;
    UINT devid;
    char *buf = NULL;
    ULONG written;
    DWORD start, duration;
    HANDLE event = NULL;
    HRESULT hr;

    if (waveOutGetNumDevs() == 0) {
        skip("no wave out devices.\n");
        return;
    }

    hr = CoCreateInstance(&CLSID_SpMMAudioOut, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISpMMSysAudio, (void **)&mmaudio);
    ok(hr == S_OK, "failed to create SPMMAudioOut instance: %#lx.\n", hr);

    hr = ISpMMSysAudio_SetState(mmaudio, SPAS_CLOSED, 0);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = ISpMMSysAudio_GetFormat(mmaudio, &fmtid, &wfx);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(IsEqualGUID(&fmtid, &SPDFID_WaveFormatEx), "got %s.\n", wine_dbgstr_guid(&fmtid));
    ok(wfx != NULL, "wfx == NULL.\n");
    ok(wfx->wFormatTag == WAVE_FORMAT_PCM, "got %u.\n", wfx->wFormatTag);
    ok(wfx->cbSize == 0, "got %u.\n", wfx->cbSize);

    hr = ISpMMSysAudio_SetFormat(mmaudio, &fmtid, wfx);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = ISpMMSysAudio_SetDeviceId(mmaudio, WAVE_MAPPER);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = ISpMMSysAudio_SetState(mmaudio, SPAS_RUN, 0);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = ISpMMSysAudio_SetDeviceId(mmaudio, WAVE_MAPPER);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = ISpMMSysAudio_SetDeviceId(mmaudio, 0);
    ok(hr == SPERR_DEVICE_BUSY, "got %#lx.\n", hr);

    hr = ISpMMSysAudio_SetFormat(mmaudio, &fmtid, wfx);
    ok(hr == S_OK, "got %#lx.\n", hr);

    memcpy(&wfx2, wfx, sizeof(wfx2));
    wfx2.nChannels = wfx->nChannels == 1 ? 2 : 1;
    wfx2.nAvgBytesPerSec = wfx2.nSamplesPerSec * wfx2.nChannels * wfx2.wBitsPerSample / 8;
    wfx2.nBlockAlign = wfx2.nChannels * wfx2.wBitsPerSample / 8;

    hr = ISpMMSysAudio_SetFormat(mmaudio, &fmtid, &wfx2);
    ok(hr == SPERR_DEVICE_BUSY, "got %#lx.\n", hr);

    devid = 0xdeadbeef;
    hr = ISpMMSysAudio_GetDeviceId(mmaudio, &devid);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(devid == WAVE_MAPPER, "got %#x.\n", devid);

    hr = ISpMMSysAudio_SetState(mmaudio, SPAS_CLOSED, 0);
    ok(hr == S_OK, "got %#lx.\n", hr);

    buf = calloc(1, wfx->nAvgBytesPerSec);
    ok(buf != NULL, "failed to allocate buffer.\n");

    hr = ISpMMSysAudio_Write(mmaudio, buf, wfx->nAvgBytesPerSec, NULL);
    ok(hr == SP_AUDIO_STOPPED, "got %#lx.\n", hr);

    hr = ISpMMSysAudio_SetState(mmaudio, SPAS_STOP, 0);
    todo_wine ok(hr == S_OK, "got %#lx.\n", hr);
    if (hr == S_OK)
    {
        hr = ISpMMSysAudio_Write(mmaudio, buf, wfx->nAvgBytesPerSec, NULL);
        ok(hr == SP_AUDIO_STOPPED, "got %#lx.\n", hr);
    }

    hr = ISpMMSysAudio_SetState(mmaudio, SPAS_CLOSED, 0);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = ISpMMSysAudio_SetState(mmaudio, SPAS_RUN, 0);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = ISpMMSysAudio_Write(mmaudio, buf, wfx->nAvgBytesPerSec, NULL);
    ok(hr == S_OK, "got %#lx.\n", hr);

    Sleep(200);

    hr = ISpMMSysAudio_SetState(mmaudio, SPAS_CLOSED, 0);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = ISpMMSysAudio_SetState(mmaudio, SPAS_RUN, 0);
    ok(hr == S_OK, "got %#lx.\n", hr);

    written = 0xdeadbeef;
    start = GetTickCount();
    hr = ISpMMSysAudio_Write(mmaudio, buf, wfx->nAvgBytesPerSec * 200 / 1000, &written);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(written == wfx->nAvgBytesPerSec * 200 / 1000, "got %lu.\n", written);

    hr = ISpMMSysAudio_Write(mmaudio, buf, wfx->nAvgBytesPerSec * 200 / 1000, NULL);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = ISpMMSysAudio_Commit(mmaudio, STGC_DEFAULT);
    todo_wine ok(hr == S_OK, "got %#lx.\n", hr);

    event = ISpMMSysAudio_EventHandle(mmaudio);
    ok(event != NULL, "event == NULL.\n");

    hr = WaitForSingleObject(event, 1000);
    ok(hr == WAIT_OBJECT_0, "got %#lx.\n", hr);

    duration = GetTickCount() - start;
    ok(duration > 200 && duration < 800, "took %lu ms.\n", duration);

    hr = ISpMMSysAudio_SetState(mmaudio, SPAS_CLOSED, 0);
    ok(hr == S_OK, "got %#lx.\n", hr);

    CoTaskMemFree(wfx);
    free(buf);
    ISpMMSysAudio_Release(mmaudio);
}

START_TEST(mmaudio)
{
    CoInitialize(NULL);
    test_interfaces();
    test_device_id();
    test_formats();
    test_audio_out();
    CoUninitialize();
}
