/*
 * Unit test suite for IDirectMusicPerformance
 *
 * Copyright 2010 Austin Lund
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

#include <stdarg.h>
#include <windef.h>
#include <initguid.h>
#include <wine/test.h>
#include <dmusici.h>

#include <stdio.h>

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);
DEFINE_GUID(GUID_Bunk,0xFFFFFFFF,0xFFFF,0xFFFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF);

static HRESULT test_InitAudio(void)
{
    IDirectMusicPerformance8 *idmusicperformance;
    IDirectSound *pDirectSound;
    IDirectMusicPort *pDirectMusicPort;
    IDirectMusicAudioPath *pDirectMusicAudioPath;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL,
            CLSCTX_INPROC_SERVER, &IID_IDirectMusicPerformance8, (LPVOID *)&idmusicperformance);
    if (hr != S_OK) {
        skip("Cannot create DirectMusicPerformance object (%x)\n", hr);
        CoUninitialize();
        return hr;
    }

    pDirectSound = NULL;
    hr = IDirectMusicPerformance8_InitAudio(idmusicperformance ,NULL,
        &pDirectSound, NULL, DMUS_APATH_SHARED_STEREOPLUSREVERB, 128, DMUS_AUDIOF_ALL, NULL);
    if(hr != S_OK)
        return hr;

    pDirectMusicPort = NULL;
    hr = IDirectMusicPerformance8_PChannelInfo(idmusicperformance, 0, &pDirectMusicPort, NULL, NULL);
    ok(hr == S_OK, "Failed to call PChannelInfo (%x)\n", hr);
    ok(pDirectMusicPort != NULL, "IDirectMusicPort not set\n");
    if (hr == S_OK && pDirectMusicPort != NULL)
        IDirectMusicPort_Release(pDirectMusicPort);

    hr = IDirectMusicPerformance8_GetDefaultAudioPath(idmusicperformance, &pDirectMusicAudioPath);
    ok(hr == S_OK, "Failed to call GetDefaultAudioPath (%x)\n", hr);
    if (hr == S_OK)
        IDirectMusicAudioPath_Release(pDirectMusicAudioPath);

    hr = IDirectMusicPerformance8_CloseDown(idmusicperformance);
    ok(hr == S_OK, "Failed to call CloseDown (%x)\n", hr);

    IDirectMusicPerformance8_Release(idmusicperformance);

    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL,
            CLSCTX_INPROC_SERVER, &IID_IDirectMusicPerformance8,
            (void**)&idmusicperformance);
    ok(hr == S_OK, "CoCreateInstance failed: %08x\n", hr);

    hr = IDirectMusicPerformance8_InitAudio(idmusicperformance, NULL, NULL,
            NULL, 0, 64, 0, NULL);
    ok(hr == S_OK, "InitAudio failed: %08x\n", hr);

    hr = IDirectMusicPerformance8_CloseDown(idmusicperformance);
    ok(hr == S_OK, "CloseDown failed: %08x\n", hr);

    IDirectMusicPerformance8_Release(idmusicperformance);

    return S_OK;
}

static void test_createport(void)
{
    IDirectMusicPerformance8 *perf;
    IDirectMusic *music = NULL;
    IDirectMusicPort *port = NULL;
    DMUS_PORTCAPS portcaps;
    DMUS_PORTPARAMS portparams;
    DWORD i;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL,
            CLSCTX_INPROC_SERVER, &IID_IDirectMusicPerformance8, (void**)&perf);
    ok(hr == S_OK, "CoCreateInstance failed: %08x\n", hr);

    hr = IDirectMusicPerformance8_Init(perf, &music, NULL, NULL);
    ok(hr == S_OK, "Init failed: %08x\n", hr);
    ok(music != NULL, "Didn't get IDirectMusic pointer\n");

    i = 0;
    while(1){
        portcaps.dwSize = sizeof(portcaps);

        hr = IDirectMusic_EnumPort(music, i, &portcaps);
        ok(hr == S_OK || hr == S_FALSE || (i == 0 && hr == E_INVALIDARG), "EnumPort failed: %08x\n", hr);
        if(hr != S_OK)
            break;

        ok(portcaps.dwSize == sizeof(portcaps), "Got unexpected portcaps struct size: %08x\n", portcaps.dwSize);
        trace("portcaps(%u).dwFlags: %08x\n", i, portcaps.dwFlags);
        trace("portcaps(%u).guidPort: %s\n", i, wine_dbgstr_guid(&portcaps.guidPort));
        trace("portcaps(%u).dwClass: %08x\n", i, portcaps.dwClass);
        trace("portcaps(%u).dwType: %08x\n", i, portcaps.dwType);
        trace("portcaps(%u).dwMemorySize: %08x\n", i, portcaps.dwMemorySize);
        trace("portcaps(%u).dwMaxChannelGroups: %08x\n", i, portcaps.dwMaxChannelGroups);
        trace("portcaps(%u).dwMaxVoices: %08x\n", i, portcaps.dwMaxVoices);
        trace("portcaps(%u).dwMaxAudioChannels: %08x\n", i, portcaps.dwMaxAudioChannels);
        trace("portcaps(%u).dwEffectFlags: %08x\n", i, portcaps.dwEffectFlags);
        trace("portcaps(%u).wszDescription: %s\n", i, wine_dbgstr_w(portcaps.wszDescription));

        ++i;
    }

    if(i == 0){
        win_skip("No ports available, skipping tests\n");
        return;
    }

    portparams.dwSize = sizeof(portparams);

    /* dwValidParams == 0 -> S_OK, filled struct */
    portparams.dwValidParams = 0;
    hr = IDirectMusic_CreatePort(music, &CLSID_DirectMusicSynth,
            &portparams, &port, NULL);
    ok(hr == S_OK, "CreatePort failed: %08x\n", hr);
    ok(port != NULL, "Didn't get IDirectMusicPort pointer\n");

    IDirectMusicPort_Release(port);
    port = NULL;

    todo_wine ok(portparams.dwValidParams != 0, "portparams struct was not filled in\n");

    /* dwValidParams != 0, invalid param -> S_FALSE, filled struct */
    portparams.dwValidParams = DMUS_PORTPARAMS_CHANNELGROUPS;
    portparams.dwChannelGroups = 0;
    hr = IDirectMusic_CreatePort(music, &CLSID_DirectMusicSynth,
            &portparams, &port, NULL);
    todo_wine ok(hr == S_FALSE, "CreatePort failed: %08x\n", hr);
    ok(port != NULL, "Didn't get IDirectMusicPort pointer\n");

    IDirectMusicPort_Release(port);
    port = NULL;

    ok(portparams.dwValidParams != 0, "portparams struct was not filled in\n");

    /* dwValidParams != 0, valid params -> S_OK */
    hr = IDirectMusic_CreatePort(music, &CLSID_DirectMusicSynth,
            &portparams, &port, NULL);
    ok(hr == S_OK, "CreatePort failed: %08x\n", hr);
    ok(port != NULL, "Didn't get IDirectMusicPort pointer\n");

    IDirectMusicPort_Release(port);
    port = NULL;

    /* GUID_NULL succeeds */
    portparams.dwValidParams = 0;
    hr = IDirectMusic_CreatePort(music, &GUID_NULL, &portparams, &port, NULL);
    ok(hr == S_OK, "CreatePort failed: %08x\n", hr);
    ok(port != NULL, "Didn't get IDirectMusicPort pointer\n");

    IDirectMusicPort_Release(port);
    port = NULL;

    todo_wine ok(portparams.dwValidParams != 0, "portparams struct was not filled in\n");

    /* null GUID fails */
    portparams.dwValidParams = 0;
    hr = IDirectMusic_CreatePort(music, NULL, &portparams, &port, NULL);
    ok(hr == E_POINTER, "CreatePort failed: %08x\n", hr);
    ok(port == NULL, "Get IDirectMusicPort pointer? %p\n", port);
    ok(portparams.dwValidParams == 0, "portparams struct was filled in?\n");

    /* garbage GUID fails */
    portparams.dwValidParams = 0;
    hr = IDirectMusic_CreatePort(music, &GUID_Bunk, &portparams, &port, NULL);
    ok(hr == E_NOINTERFACE, "CreatePort failed: %08x\n", hr);
    ok(port == NULL, "Get IDirectMusicPort pointer? %p\n", port);
    ok(portparams.dwValidParams == 0, "portparams struct was filled in?\n");

    IDirectMusic_Release(music);
    IDirectMusicPerformance_Release(perf);
}

static void test_COM(void)
{
    IDirectMusicPerformance *dmp = (IDirectMusicPerformance*)0xdeadbeef;
    IDirectMusicPerformance *dmp2;
    IDirectMusicPerformance8 *dmp8;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, (IUnknown*)&dmp, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dmp);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicPerformance create failed: %08x, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dmp, "dmp = %p\n", dmp);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void**)&dmp);
    ok(hr == E_NOINTERFACE,
            "DirectMusicPerformance create failed: %08x, expected E_NOINTERFACE\n", hr);

    /* Same refcount */
    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicPerformance, (void**)&dmp);
    ok(hr == S_OK, "DirectMusicPerformance create failed: %08x, expected S_OK\n", hr);
    refcount = IDirectMusicPerformance_AddRef(dmp);
    ok (refcount == 2, "refcount == %u, expected 2\n", refcount);
    hr = IDirectMusicPerformance_QueryInterface(dmp, &IID_IDirectMusicPerformance2, (void**)&dmp2);
    ok(hr == S_OK, "QueryInterface for IID_IDirectMusicPerformance2 failed: %08x\n", hr);
    IDirectMusicPerformance_AddRef(dmp);
    refcount = IDirectMusicPerformance_Release(dmp);
    ok (refcount == 3, "refcount == %u, expected 3\n", refcount);
    hr = IDirectMusicPerformance_QueryInterface(dmp, &IID_IDirectMusicPerformance8, (void**)&dmp8);
    ok(hr == S_OK, "QueryInterface for IID_IDirectMusicPerformance8 failed: %08x\n", hr);
    refcount = IDirectMusicPerformance_Release(dmp);
    ok (refcount == 3, "refcount == %u, expected 3\n", refcount);
    refcount = IDirectMusicPerformance8_Release(dmp8);
    ok (refcount == 2, "refcount == %u, expected 2\n", refcount);
    refcount = IDirectMusicPerformance_Release(dmp2);
    ok (refcount == 1, "refcount == %u, expected 1\n", refcount);
    refcount = IDirectMusicPerformance_Release(dmp);
    ok (refcount == 0, "refcount == %u, expected 0\n", refcount);
}

START_TEST( performance )
{
    HRESULT hr;

    hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        skip("Cannot initialize COM (%x)\n", hr);
        return;
    }

    hr = test_InitAudio();
    if (hr != S_OK) {
        skip("InitAudio failed (%x)\n", hr);
        return;
    }

    test_COM();
    test_createport();

    CoUninitialize();
}
