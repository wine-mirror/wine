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

#include "wine/test.h"

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

START_TEST(mmaudio)
{
    CoInitialize(NULL);
    test_interfaces();
    test_device_id();
    CoUninitialize();
}
