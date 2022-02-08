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

static void create_performance(IDirectMusicPerformance8 **performance, IDirectMusic **dmusic,
        IDirectSound **dsound, BOOL set_cooplevel)
{
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicPerformance8, (void **)performance);
    ok(hr == S_OK, "DirectMusicPerformance create failed: %#lx\n", hr);
    if (dmusic) {
        hr = CoCreateInstance(&CLSID_DirectMusic, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusic8,
                (void **)dmusic);
        ok(hr == S_OK, "DirectMusic create failed: %#lx\n", hr);
    }
    if (dsound) {
        hr = DirectSoundCreate8(NULL, (IDirectSound8 **)dsound, NULL);
        ok(hr == S_OK, "DirectSoundCreate failed: %#lx\n", hr);
        if (set_cooplevel) {
            hr = IDirectSound_SetCooperativeLevel(*dsound, GetForegroundWindow(), DSSCL_PRIORITY);
            ok(hr == S_OK, "SetCooperativeLevel failed: %#lx\n", hr);
        }
    }
}

static void destroy_performance(IDirectMusicPerformance8 *performance, IDirectMusic *dmusic,
        IDirectSound *dsound)
{
    HRESULT hr;

    hr = IDirectMusicPerformance8_CloseDown(performance);
    ok(hr == S_OK, "CloseDown failed: %#lx\n", hr);
    IDirectMusicPerformance8_Release(performance);
    if (dmusic)
        IDirectMusic_Release(dmusic);
    if (dsound)
        IDirectSound_Release(dsound);
}

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

static HRESULT test_InitAudio(void)
{
    IDirectMusicPerformance8 *performance;
    IDirectMusic *dmusic;
    IDirectSound *dsound;
    IDirectMusicPort *port;
    IDirectMusicAudioPath *path;
    DWORD channel, group;
    HRESULT hr;
    ULONG ref;

    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicPerformance8, (void **)&performance);
    if (hr != S_OK) {
        skip("Cannot create DirectMusicPerformance object (%lx)\n", hr);
        CoUninitialize();
        return hr;
    }

    dsound = NULL;
    hr = IDirectMusicPerformance8_InitAudio(performance, NULL, &dsound, NULL,
            DMUS_APATH_SHARED_STEREOPLUSREVERB, 128, DMUS_AUDIOF_ALL, NULL);
    if (hr != S_OK) {
        IDirectMusicPerformance8_Release(performance);
        return hr;
    }

    hr = IDirectMusicPerformance8_PChannelInfo(performance, 128, &port, NULL, NULL);
    ok(hr == E_INVALIDARG, "PChannelInfo failed, got %#lx\n", hr);
    hr = IDirectMusicPerformance8_PChannelInfo(performance, 127, &port, NULL, NULL);
    ok(hr == S_OK, "PChannelInfo failed, got %#lx\n", hr);
    IDirectMusicPort_Release(port);
    port = NULL;
    hr = IDirectMusicPerformance8_PChannelInfo(performance, 0, &port, NULL, NULL);
    ok(hr == S_OK, "PChannelInfo failed, got %#lx\n", hr);
    ok(port != NULL, "IDirectMusicPort not set\n");
    hr = IDirectMusicPerformance8_AssignPChannel(performance, 0, port, 0, 0);
    todo_wine ok(hr == DMUS_E_AUDIOPATHS_IN_USE, "AssignPChannel failed (%#lx)\n", hr);
    hr = IDirectMusicPerformance8_AssignPChannelBlock(performance, 0, port, 0);
    todo_wine ok(hr == DMUS_E_AUDIOPATHS_IN_USE, "AssignPChannelBlock failed (%#lx)\n", hr);
    IDirectMusicPort_Release(port);

    hr = IDirectMusicPerformance8_GetDefaultAudioPath(performance, &path);
    ok(hr == S_OK, "Failed to call GetDefaultAudioPath (%lx)\n", hr);
    if (hr == S_OK)
        IDirectMusicAudioPath_Release(path);

    hr = IDirectMusicPerformance8_CloseDown(performance);
    ok(hr == S_OK, "Failed to call CloseDown (%lx)\n", hr);

    IDirectMusicPerformance8_Release(performance);

    /* Auto generated dmusic and dsound */
    create_performance(&performance, NULL, NULL, FALSE);
    hr = IDirectMusicPerformance8_InitAudio(performance, NULL, NULL, NULL, 0, 64, 0, NULL);
    ok(hr == S_OK, "InitAudio failed: %#lx\n", hr);
    hr = IDirectMusicPerformance8_PChannelInfo(performance, 0, &port, NULL, NULL);
    ok(hr == E_INVALIDARG, "PChannelInfo failed, got %#lx\n", hr);
    destroy_performance(performance, NULL, NULL);

    /* Refcounts for auto generated dmusic and dsound */
    create_performance(&performance, NULL, NULL, FALSE);
    dmusic = NULL;
    dsound = NULL;
    hr = IDirectMusicPerformance8_InitAudio(performance, &dmusic, &dsound, NULL, 0, 64, 0, NULL);
    ok(hr == S_OK, "InitAudio failed: %#lx\n", hr);
    ref = get_refcount(dsound);
    ok(ref == 3, "dsound ref count got %ld expected 3\n", ref);
    ref = get_refcount(dmusic);
    ok(ref == 2, "dmusic ref count got %ld expected 2\n", ref);
    destroy_performance(performance, NULL, NULL);

    /* dsound without SetCooperativeLevel() */
    create_performance(&performance, NULL, &dsound, FALSE);
    hr = IDirectMusicPerformance8_InitAudio(performance, NULL, &dsound, NULL, 0, 0, 0, NULL);
    todo_wine ok(hr == DSERR_PRIOLEVELNEEDED, "InitAudio failed: %#lx\n", hr);
    destroy_performance(performance, NULL, dsound);

    /* Using the wrong CLSID_DirectSound */
    create_performance(&performance, NULL, NULL, FALSE);
    hr = DirectSoundCreate(NULL, &dsound, NULL);
    ok(hr == S_OK, "DirectSoundCreate failed: %#lx\n", hr);
    hr = IDirectMusicPerformance8_InitAudio(performance, NULL, &dsound, NULL, 0, 0, 0, NULL);
    todo_wine ok(hr == E_NOINTERFACE, "InitAudio failed: %#lx\n", hr);
    destroy_performance(performance, NULL, dsound);

    /* Init() works with just a CLSID_DirectSound */
    create_performance(&performance, NULL, NULL, FALSE);
    hr = DirectSoundCreate(NULL, &dsound, NULL);
    ok(hr == S_OK, "DirectSoundCreate failed: %#lx\n", hr);
    hr = IDirectSound_SetCooperativeLevel(dsound, GetForegroundWindow(), DSSCL_PRIORITY);
    ok(hr == S_OK, "SetCooperativeLevel failed: %#lx\n", hr);
    hr = IDirectMusicPerformance8_Init(performance, NULL, dsound, NULL);
    ok(hr == S_OK, "Init failed: %#lx\n", hr);
    destroy_performance(performance, NULL, dsound);

    /* Init() followed by InitAudio() */
    create_performance(&performance, NULL, &dsound, TRUE);
    hr = IDirectMusicPerformance8_Init(performance, NULL, dsound, NULL);
    ok(hr == S_OK, "Init failed: %#lx\n", hr);
    hr = IDirectMusicPerformance8_InitAudio(performance, NULL, &dsound, NULL, 0, 0, 0, NULL);
    ok(hr == DMUS_E_ALREADY_INITED, "InitAudio failed: %#lx\n", hr);
    destroy_performance(performance, NULL, dsound);

    /* Provided dmusic and dsound */
    create_performance(&performance, &dmusic, &dsound, TRUE);
    hr = IDirectMusicPerformance8_InitAudio(performance, &dmusic, &dsound, NULL, 0, 64, 0, NULL);
    ok(hr == S_OK, "InitAudio failed: %#lx\n", hr);
    ref = get_refcount(dsound);
    todo_wine ok(ref == 2, "dsound ref count got %ld expected 2\n", ref);
    ref = get_refcount(dmusic);
    ok(ref == 2, "dmusic ref count got %ld expected 2\n", ref);
    destroy_performance(performance, dmusic, dsound);

    /* Provided dmusic initialized with SetDirectSound */
    create_performance(&performance, &dmusic, &dsound, TRUE);
    hr = IDirectMusic_SetDirectSound(dmusic, dsound, NULL);
    ok(hr == S_OK, "SetDirectSound failed: %#lx\n", hr);
    ref = get_refcount(dsound);
    ok(ref == 2, "dsound ref count got %ld expected 2\n", ref);
    hr = IDirectMusicPerformance8_InitAudio(performance, &dmusic, NULL, NULL, 0, 64, 0, NULL);
    ok(hr == S_OK, "InitAudio failed: %#lx\n", hr);
    ref = get_refcount(dsound);
    todo_wine ok(ref == 2, "dsound ref count got %ld expected 2\n", ref);
    ref = get_refcount(dmusic);
    ok(ref == 2, "dmusic ref count got %ld expected 2\n", ref);
    destroy_performance(performance, dmusic, dsound);

    /* Provided dmusic and dsound, dmusic initialized with SetDirectSound */
    create_performance(&performance, &dmusic, &dsound, TRUE);
    hr = IDirectMusic_SetDirectSound(dmusic, dsound, NULL);
    ok(hr == S_OK, "SetDirectSound failed: %#lx\n", hr);
    ref = get_refcount(dsound);
    ok(ref == 2, "dsound ref count got %ld expected 2\n", ref);
    hr = IDirectMusicPerformance8_InitAudio(performance, &dmusic, &dsound, NULL, 0, 64, 0, NULL);
    ok(hr == S_OK, "InitAudio failed: %#lx\n", hr);
    ref = get_refcount(dsound);
    ok(ref == 3, "dsound ref count got %ld expected 3\n", ref);
    ref = get_refcount(dmusic);
    ok(ref == 2, "dmusic ref count got %ld expected 2\n", ref);
    destroy_performance(performance, dmusic, dsound);

    /* InitAudio with perf channel count not a multiple of 16 rounds up */
    create_performance(&performance, NULL, NULL, TRUE);
    hr = IDirectMusicPerformance8_InitAudio(performance, NULL, NULL, NULL,
            DMUS_APATH_SHARED_STEREOPLUSREVERB, 29, DMUS_AUDIOF_ALL, NULL);
    ok(hr == S_OK, "InitAudio failed: %#lx\n", hr);
    hr = IDirectMusicPerformance8_PChannelInfo(performance, 31, &port, &group, &channel);
    ok(hr == S_OK && group == 2 && channel == 15,
            "PChannelInfo failed, got %#lx, %lu, %lu\n", hr, group, channel);
    hr = IDirectMusicPerformance8_PChannelInfo(performance, 32, &port, NULL, NULL);
    ok(hr == E_INVALIDARG, "PChannelInfo failed, got %#lx\n", hr);
    destroy_performance(performance, NULL, NULL);

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
    ok(hr == S_OK, "CoCreateInstance failed: %#lx\n", hr);

    hr = IDirectMusicPerformance8_Init(perf, &music, NULL, NULL);
    ok(hr == S_OK, "Init failed: %#lx\n", hr);
    ok(music != NULL, "Didn't get IDirectMusic pointer\n");

    i = 0;
    while(1){
        portcaps.dwSize = sizeof(portcaps);

        hr = IDirectMusic_EnumPort(music, i, &portcaps);
        ok(hr == S_OK || hr == S_FALSE || (i == 0 && hr == E_INVALIDARG), "EnumPort failed: %#lx\n", hr);
        if(hr != S_OK)
            break;

        ok(portcaps.dwSize == sizeof(portcaps), "Got unexpected portcaps struct size: %lu\n", portcaps.dwSize);
        trace("portcaps(%lu).dwFlags: %#lx\n", i, portcaps.dwFlags);
        trace("portcaps(%lu).guidPort: %s\n", i, wine_dbgstr_guid(&portcaps.guidPort));
        trace("portcaps(%lu).dwClass: %#lx\n", i, portcaps.dwClass);
        trace("portcaps(%lu).dwType: %#lx\n", i, portcaps.dwType);
        trace("portcaps(%lu).dwMemorySize: %#lx\n", i, portcaps.dwMemorySize);
        trace("portcaps(%lu).dwMaxChannelGroups: %lu\n", i, portcaps.dwMaxChannelGroups);
        trace("portcaps(%lu).dwMaxVoices: %lu\n", i, portcaps.dwMaxVoices);
        trace("portcaps(%lu).dwMaxAudioChannels: %lu\n", i, portcaps.dwMaxAudioChannels);
        trace("portcaps(%lu).dwEffectFlags: %#lx\n", i, portcaps.dwEffectFlags);
        trace("portcaps(%lu).wszDescription: %s\n", i, wine_dbgstr_w(portcaps.wszDescription));

        ++i;
    }

    if(i == 0){
        win_skip("No ports available, skipping tests\n");
        return;
    }

    portparams.dwSize = sizeof(portparams);

    /* dwValidParams == 0 -> S_OK, filled struct */
    portparams.dwValidParams = 0;
    hr = IDirectMusic_CreatePort(music, &CLSID_DirectMusicSynth, &portparams, &port, NULL);
    ok(hr == S_OK, "CreatePort failed: %#lx\n", hr);
    ok(port != NULL, "Didn't get IDirectMusicPort pointer\n");
    ok(portparams.dwValidParams, "portparams struct was not filled in\n");
    IDirectMusicPort_Release(port);
    port = NULL;

    /* dwValidParams != 0, invalid param -> S_FALSE, filled struct */
    portparams.dwValidParams = DMUS_PORTPARAMS_CHANNELGROUPS;
    portparams.dwChannelGroups = 0;
    hr = IDirectMusic_CreatePort(music, &CLSID_DirectMusicSynth, &portparams, &port, NULL);
    todo_wine ok(hr == S_FALSE, "CreatePort failed: %#lx\n", hr);
    ok(port != NULL, "Didn't get IDirectMusicPort pointer\n");
    ok(portparams.dwValidParams, "portparams struct was not filled in\n");
    IDirectMusicPort_Release(port);
    port = NULL;

    /* dwValidParams != 0, valid params -> S_OK */
    hr = IDirectMusic_CreatePort(music, &CLSID_DirectMusicSynth, &portparams, &port, NULL);
    ok(hr == S_OK, "CreatePort failed: %#lx\n", hr);
    ok(port != NULL, "Didn't get IDirectMusicPort pointer\n");
    IDirectMusicPort_Release(port);
    port = NULL;

    /* GUID_NULL succeeds */
    portparams.dwValidParams = 0;
    hr = IDirectMusic_CreatePort(music, &GUID_NULL, &portparams, &port, NULL);
    ok(hr == S_OK, "CreatePort failed: %#lx\n", hr);
    ok(port != NULL, "Didn't get IDirectMusicPort pointer\n");
    ok(portparams.dwValidParams, "portparams struct was not filled in\n");
    IDirectMusicPort_Release(port);
    port = NULL;

    /* null GUID fails */
    portparams.dwValidParams = 0;
    hr = IDirectMusic_CreatePort(music, NULL, &portparams, &port, NULL);
    ok(hr == E_POINTER, "CreatePort failed: %#lx\n", hr);
    ok(port == NULL, "Get IDirectMusicPort pointer? %p\n", port);
    ok(portparams.dwValidParams == 0, "portparams struct was filled in?\n");

    /* garbage GUID fails */
    portparams.dwValidParams = 0;
    hr = IDirectMusic_CreatePort(music, &GUID_Bunk, &portparams, &port, NULL);
    ok(hr == E_NOINTERFACE, "CreatePort failed: %#lx\n", hr);
    ok(port == NULL, "Get IDirectMusicPort pointer? %p\n", port);
    ok(portparams.dwValidParams == 0, "portparams struct was filled in?\n");

    hr = IDirectMusicPerformance8_CloseDown(perf);
    ok(hr == S_OK, "CloseDown failed: %#lx\n", hr);

    IDirectMusic_Release(music);
    IDirectMusicPerformance_Release(perf);
}

static void test_pchannel(void)
{
    IDirectMusicPerformance8 *perf;
    IDirectMusicPort *port = NULL, *port2;
    DWORD channel, group;
    unsigned int i;
    HRESULT hr;

    create_performance(&perf, NULL, NULL, TRUE);
    hr = IDirectMusicPerformance8_Init(perf, NULL, NULL, NULL);
    ok(hr == S_OK, "Init failed: %#lx\n", hr);
    hr = IDirectMusicPerformance8_PChannelInfo(perf, 0, &port, NULL, NULL);
    ok(hr == E_INVALIDARG && !port, "PChannelInfo failed, got %#lx, %p\n", hr, port);

    /* Add default port. Sets PChannels 0-15 to the corresponding channels in group 1 */
    hr = IDirectMusicPerformance8_AddPort(perf, NULL);
    ok(hr == S_OK, "AddPort of default port failed: %#lx\n", hr);
    hr = IDirectMusicPerformance8_PChannelInfo(perf, 0, NULL, NULL, NULL);
    ok(hr == S_OK, "PChannelInfo failed, got %#lx\n", hr);
    hr = IDirectMusicPerformance8_PChannelInfo(perf, 0, &port, NULL, NULL);
    ok(hr == S_OK && port, "PChannelInfo failed, got %#lx, %p\n", hr, port);
    for (i = 1; i < 16; i++) {
        hr = IDirectMusicPerformance8_PChannelInfo(perf, i, &port2, &group, &channel);
        ok(hr == S_OK && port == port2 && group == 1 && channel == i,
                "PChannelInfo failed, got %#lx, %p, %lu, %lu\n", hr, port2, group, channel);
        IDirectMusicPort_Release(port2);
    }

    /* Unset PChannels fail to retrieve */
    hr = IDirectMusicPerformance8_PChannelInfo(perf, 16, &port2, NULL, NULL);
    ok(hr == E_INVALIDARG, "PChannelInfo failed, got %#lx, %p\n", hr, port);
    hr = IDirectMusicPerformance8_PChannelInfo(perf, MAXDWORD - 16, &port2, NULL, NULL);
    ok(hr == E_INVALIDARG, "PChannelInfo failed, got %#lx, %p\n", hr, port);

    /* Channel group 0 can be set just fine */
    hr = IDirectMusicPerformance8_AssignPChannel(perf, 0, port, 0, 0);
    ok(hr == S_OK, "AssignPChannel failed, got %#lx\n", hr);
    hr = IDirectMusicPerformance8_AssignPChannelBlock(perf, 0, port, 0);
    ok(hr == S_OK, "AssignPChannelBlock failed, got %#lx\n", hr);
    for (i = 1; i < 16; i++) {
        hr = IDirectMusicPerformance8_PChannelInfo(perf, i, &port2, &group, &channel);
        ok(hr == S_OK && port == port2 && group == 0 && channel == i,
                "PChannelInfo failed, got %#lx, %p, %lu, %lu\n", hr, port2, group, channel);
        IDirectMusicPort_Release(port2);
    }

    /* Last PChannel Block can be set only individually but not read */
    hr = IDirectMusicPerformance8_AssignPChannel(perf, MAXDWORD, port, 0, 3);
    ok(hr == S_OK, "AssignPChannel failed, got %#lx\n", hr);
    port2 = (IDirectMusicPort *)0xdeadbeef;
    hr = IDirectMusicPerformance8_PChannelInfo(perf, MAXDWORD, &port2, NULL, NULL);
    todo_wine ok(hr == E_INVALIDARG && port2 == (IDirectMusicPort *)0xdeadbeef,
            "PChannelInfo failed, got %#lx, %p\n", hr, port2);
    hr = IDirectMusicPerformance8_AssignPChannelBlock(perf, MAXDWORD, port, 0);
    ok(hr == E_INVALIDARG, "AssignPChannelBlock failed, got %#lx\n", hr);
    hr = IDirectMusicPerformance8_AssignPChannelBlock(perf, MAXDWORD / 16, port, 1);
    todo_wine ok(hr == E_INVALIDARG, "AssignPChannelBlock failed, got %#lx\n", hr);
    for (i = MAXDWORD - 15; i < MAXDWORD; i++) {
        hr = IDirectMusicPerformance8_AssignPChannel(perf, i, port, 0, 0);
        ok(hr == S_OK, "AssignPChannel failed, got %#lx\n", hr);
        hr = IDirectMusicPerformance8_PChannelInfo(perf, i, &port2, NULL, NULL);
        todo_wine ok(hr == E_INVALIDARG && port2 == (IDirectMusicPort *)0xdeadbeef,
                "PChannelInfo failed, got %#lx, %p\n", hr, port2);
    }

    /* Second to last PChannel Block can be set only individually and read */
    hr = IDirectMusicPerformance8_AssignPChannelBlock(perf, MAXDWORD / 16 - 1, port, 1);
    todo_wine ok(hr == E_INVALIDARG, "AssignPChannelBlock failed, got %#lx\n", hr);
    for (i = MAXDWORD - 31; i < MAXDWORD - 15; i++) {
        hr = IDirectMusicPerformance8_AssignPChannel(perf, i, port, 1, 7);
        ok(hr == S_OK, "AssignPChannel failed, got %#lx\n", hr);
        hr = IDirectMusicPerformance8_PChannelInfo(perf, i, &port2, &group, &channel);
        ok(hr == S_OK && port2 == port && group == 1 && channel == 7,
                "PChannelInfo failed, got %#lx, %p, %lu, %lu\n", hr, port2, group, channel);
        IDirectMusicPort_Release(port2);
    }

    /* Third to last PChannel Block behaves normal */
    hr = IDirectMusicPerformance8_AssignPChannelBlock(perf, MAXDWORD / 16 - 2, port, 0);
    ok(hr == S_OK, "AssignPChannelBlock failed, got %#lx\n", hr);
    for (i = MAXDWORD - 47; i < MAXDWORD - 31; i++) {
        hr = IDirectMusicPerformance8_PChannelInfo(perf, i, &port2, &group, &channel);
        ok(hr == S_OK && port2 == port && group == 0 && channel == i % 16,
                "PChannelInfo failed, got %#lx, %p, %lu, %lu\n", hr, port2, group, channel);
        IDirectMusicPort_Release(port2);
    }

    /* One PChannel set in a Block, rest is initialized too */
    hr = IDirectMusicPerformance8_AssignPChannel(perf, 4711, port, 1, 13);
    ok(hr == S_OK, "AssignPChannel failed, got %#lx\n", hr);
    hr = IDirectMusicPerformance8_PChannelInfo(perf, 4711, &port2, &group, &channel);
    ok(hr == S_OK && port2 == port && group == 1 && channel == 13,
            "PChannelInfo failed, got %#lx, %p, %lu, %lu\n", hr, port2, group, channel);
    IDirectMusicPort_Release(port2);
    group = channel = 0xdeadbeef;
    hr = IDirectMusicPerformance8_PChannelInfo(perf, 4712, &port2, &group, &channel);
    ok(hr == S_OK && port2 == port && group == 0 && channel == 8,
            "PChannelInfo failed, got %#lx, %p, %lu, %lu\n", hr, port2, group, channel);
    IDirectMusicPort_Release(port2);
    group = channel = 0xdeadbeef;
    hr = IDirectMusicPerformance8_PChannelInfo(perf, 4719, &port2, &group, &channel);
    ok(hr == S_OK && port2 == port && group == 0 && channel == 15,
            "PChannelInfo failed, got %#lx, %p, %lu, %lu\n", hr, port2, group, channel);
    IDirectMusicPort_Release(port2);

    IDirectMusicPort_Release(port);
    destroy_performance(perf, NULL, NULL);
}

static void test_COM(void)
{
    IDirectMusicPerformance *dmp = (IDirectMusicPerformance*)0xdeadbeef;
    IDirectMusicPerformance *dmp2;
    IDirectMusicPerformance8 *dmp8;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, (IUnknown *)0xdeadbeef, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dmp);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicPerformance create failed: %#lx, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dmp, "dmp = %p\n", dmp);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void**)&dmp);
    ok(hr == E_NOINTERFACE, "DirectMusicPerformance create failed: %#lx, expected E_NOINTERFACE\n", hr);

    /* Same refcount */
    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicPerformance, (void**)&dmp);
    ok(hr == S_OK, "DirectMusicPerformance create failed: %#lx, expected S_OK\n", hr);
    refcount = IDirectMusicPerformance_AddRef(dmp);
    ok (refcount == 2, "refcount == %lu, expected 2\n", refcount);
    hr = IDirectMusicPerformance_QueryInterface(dmp, &IID_IDirectMusicPerformance2, (void**)&dmp2);
    ok(hr == S_OK, "QueryInterface for IID_IDirectMusicPerformance2 failed: %#lx\n", hr);
    IDirectMusicPerformance_AddRef(dmp);
    refcount = IDirectMusicPerformance_Release(dmp);
    ok (refcount == 3, "refcount == %lu, expected 3\n", refcount);
    hr = IDirectMusicPerformance_QueryInterface(dmp, &IID_IDirectMusicPerformance8, (void**)&dmp8);
    ok(hr == S_OK, "QueryInterface for IID_IDirectMusicPerformance8 failed: %#lx\n", hr);
    refcount = IDirectMusicPerformance_Release(dmp);
    ok (refcount == 3, "refcount == %lu, expected 3\n", refcount);
    refcount = IDirectMusicPerformance8_Release(dmp8);
    ok (refcount == 2, "refcount == %lu, expected 2\n", refcount);
    refcount = IDirectMusicPerformance_Release(dmp2);
    ok (refcount == 1, "refcount == %lu, expected 1\n", refcount);
    refcount = IDirectMusicPerformance_Release(dmp);
    ok (refcount == 0, "refcount == %lu, expected 0\n", refcount);
}

static void test_notification_type(void)
{
    static unsigned char rifffile[8+4+8+16+8+256] = "RIFF\x24\x01\x00\x00WAVE" /* header: 4 ("WAVE") + (8 + 16) (format segment) + (8 + 256) (data segment) = 0x124 */
        "fmt \x10\x00\x00\x00\x01\x00\x20\x00\xAC\x44\x00\x00\x10\xB1\x02\x00\x04\x00\x10\x00" /* format segment: PCM, 2 chan, 44100 Hz, 16 bits */
        "data\x00\x01\x00\x00"; /* 256 byte data segment (silence) */

    IDirectMusicPerformance8 *perf;
    IDirectMusic *music = NULL;
    IDirectMusicSegment8 *prime_segment8;
    IDirectMusicSegment8 *segment8 = NULL;
    IDirectMusicLoader8 *loader;
    IDirectMusicAudioPath8 *path;
    IDirectMusicSegmentState *state;
    IDirectSound *dsound = NULL;
    HRESULT hr;
    DWORD result;
    HANDLE messages;
    DMUS_NOTIFICATION_PMSG *msg;
    BOOL found_end = FALSE;
    DMUS_OBJECTDESC desc = {0};

    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL,
            CLSCTX_INPROC_SERVER, &IID_IDirectMusicPerformance8, (void**)&perf);
    ok(hr == S_OK, "CoCreateInstance failed: %#lx\n", hr);

    hr = IDirectMusicPerformance8_InitAudio(perf, &music, &dsound, NULL, DMUS_APATH_DYNAMIC_STEREO, 64, DMUS_AUDIOF_ALL, NULL);
    ok(music != NULL, "Didn't get IDirectMusic pointer\n");
    ok(dsound != NULL, "Didn't get IDirectSound pointer\n");

    hr = CoCreateInstance(&CLSID_DirectMusicLoader, NULL, CLSCTX_INPROC_SERVER,  &IID_IDirectMusicLoader8, (void**)&loader);
    ok(hr == S_OK, "CoCreateInstance failed: %#lx\n", hr);

    messages = CreateEventA( NULL, FALSE, FALSE, NULL );

    hr = IDirectMusicPerformance8_AddNotificationType(perf, &GUID_NOTIFICATION_SEGMENT);
    ok(hr == S_OK, "Failed: %#lx\n", hr);

    hr = IDirectMusicPerformance8_SetNotificationHandle(perf, messages, 0);
    ok(hr == S_OK, "Failed: %#lx\n", hr);

    hr = IDirectMusicPerformance8_GetDefaultAudioPath(perf, &path);
    ok(hr == S_OK, "Failed: %#lx\n", hr);
    ok(path != NULL, "Didn't get IDirectMusicAudioPath pointer\n");

    desc.dwSize = sizeof(DMUS_OBJECTDESC);
    desc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_MEMORY;
    desc.guidClass = CLSID_DirectMusicSegment;
    desc.pbMemData = rifffile;
    desc.llMemLength = sizeof(rifffile);
    hr = IDirectMusicLoader8_GetObject(loader, &desc, &IID_IDirectMusicSegment8, (void**)&prime_segment8);
    ok(hr == S_OK, "Failed: %#lx\n", hr);
    ok(prime_segment8 != NULL, "Didn't get IDirectMusicSegment pointer\n");

    hr = IDirectMusicSegment8_Download(prime_segment8, (IUnknown*)path);
    ok(hr == S_OK, "Download failed: %#lx\n", hr);

    hr = IDirectMusicPerformance8_PlaySegmentEx(perf, (IUnknown*)prime_segment8,
            NULL, NULL, DMUS_SEGF_SECONDARY, 0, &state, NULL, (IUnknown*)path);
    ok(hr == S_OK, "PlaySegmentEx failed: %#lx\n", hr);
    ok(state != NULL, "Didn't get IDirectMusicSegmentState pointer\n");

    while (!found_end) {
        result = WaitForSingleObject(messages, 500);
        todo_wine ok(result == WAIT_OBJECT_0, "Failed: %ld\n", result);
        if (result != WAIT_OBJECT_0)
            break;

        msg = NULL;
        hr = IDirectMusicPerformance8_GetNotificationPMsg(perf, &msg);
        ok(hr == S_OK, "Failed: %#lx\n", hr);
        ok(msg != NULL, "Unexpected NULL pointer\n");
        if (FAILED(hr) || !msg)
            break;

        trace("Notification: %ld\n", msg->dwNotificationOption);

        if (msg->dwNotificationOption == DMUS_NOTIFICATION_SEGEND ||
            msg->dwNotificationOption == DMUS_NOTIFICATION_SEGALMOSTEND) {
            ok(msg->punkUser != NULL, "Unexpected NULL pointer\n");
            if (msg->punkUser) {
                IDirectMusicSegmentState8 *segmentstate;
                IDirectMusicSegment       *segment;

                hr = IUnknown_QueryInterface(msg->punkUser, &IID_IDirectMusicSegmentState8, (void**)&segmentstate);
                ok(hr == S_OK, "Failed: %#lx\n", hr);

                hr = IDirectMusicSegmentState8_GetSegment(segmentstate, &segment);
                ok(hr == S_OK, "Failed: %#lx\n", hr);
                if (FAILED(hr)) {
                    IDirectMusicSegmentState8_Release(segmentstate);
                    break;
                }

                hr = IDirectMusicSegment_QueryInterface(segment, &IID_IDirectMusicSegment8, (void**)&segment8);
                ok(hr == S_OK, "Failed: %#lx\n", hr);

                found_end = TRUE;

                IDirectMusicSegment_Release(segment);
                IDirectMusicSegmentState8_Release(segmentstate);
            }
        }

        IDirectMusicPerformance8_FreePMsg(perf, (DMUS_PMSG*)msg);
    }
    todo_wine ok(prime_segment8 == segment8, "Wrong end segment\n");
    todo_wine ok(found_end, "Didn't receive DMUS_NOTIFICATION_SEGEND message\n");

    CloseHandle(messages);

    if(segment8)
        IDirectMusicSegment8_Release(segment8);
    IDirectSound_Release(dsound);
    IDirectMusicSegmentState_Release(state);
    IDirectMusicAudioPath_Release(path);
    IDirectMusicLoader8_Release(loader);
    IDirectMusic_Release(music);
    IDirectMusicPerformance8_Release(perf);
}

static void test_performance_graph(void)
{
    HRESULT hr;
    IDirectMusicPerformance8 *perf;
    IDirectMusicGraph *graph = NULL, *graph2;

    create_performance(&perf, NULL, NULL, FALSE);
    hr = IDirectMusicPerformance8_Init(perf, NULL, NULL, NULL);
    ok(hr == S_OK, "Init failed: %#lx\n", hr);

    hr = IDirectMusicPerformance8_GetGraph(perf, NULL);
    ok(hr == E_POINTER, "Failed: %#lx\n", hr);

    hr = IDirectMusicPerformance8_GetGraph(perf, &graph2);
    ok(hr == DMUS_E_NOT_FOUND, "Failed: %#lx\n", hr);
    ok(graph2 == NULL, "unexpected pointer.\n");

    hr = IDirectMusicPerformance8_QueryInterface(perf, &IID_IDirectMusicGraph, (void**)&graph);
    todo_wine ok(hr == S_OK, "Failed: %#lx\n", hr);

    if (graph)
        IDirectMusicGraph_Release(graph);
    destroy_performance(perf, NULL, NULL);
}

START_TEST( performance )
{
    HRESULT hr;

    hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        skip("Cannot initialize COM (%lx)\n", hr);
        return;
    }

    hr = test_InitAudio();
    if (hr != S_OK) {
        skip("InitAudio failed (%lx)\n", hr);
        return;
    }

    test_COM();
    test_createport();
    test_pchannel();
    test_notification_type();
    test_performance_graph();

    CoUninitialize();
}
