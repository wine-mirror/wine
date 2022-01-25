/*
 * Unit tests for dmsynth functions
 *
 * Copyright (C) 2012 Christian Costa
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

#include <stdio.h>

#include "wine/test.h"
#include "uuids.h"
#include "ole2.h"
#include "initguid.h"
#include "dmusics.h"
#include "dmusici.h"
#include "dmksctrl.h"

static BOOL missing_dmsynth(void)
{
    IDirectMusicSynth *dms;
    HRESULT hr = CoCreateInstance(&CLSID_DirectMusicSynth, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicSynth, (void**)&dms);

    if (hr == S_OK && dms)
    {
        IDirectMusicSynth_Release(dms);
        return FALSE;
    }
    return TRUE;
}

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

static void test_dmsynth(void)
{
    IDirectMusicSynth *dmsynth = NULL;
    IDirectMusicSynthSink *dmsynth_sink = NULL, *dmsynth_sink2 = NULL;
    IReferenceClock* clock_synth = NULL;
    IReferenceClock* clock_sink = NULL;
    IKsControl* control_synth = NULL;
    IKsControl* control_sink = NULL;
    ULONG ref_clock_synth, ref_clock_sink;
    HRESULT hr;
    KSPROPERTY property;
    ULONG value;
    ULONG bytes;
    DMUS_PORTCAPS caps;

    hr = CoCreateInstance(&CLSID_DirectMusicSynth, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicSynth, (LPVOID*)&dmsynth);
    ok(hr == S_OK, "CoCreateInstance returned: %x\n", hr);

    hr = CoCreateInstance(&CLSID_DirectMusicSynthSink, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicSynthSink,
            (void **)&dmsynth_sink);
    ok(hr == S_OK, "CoCreateInstance returned: %x\n", hr);
    hr = CoCreateInstance(&CLSID_DirectMusicSynthSink, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicSynthSink,
            (void **)&dmsynth_sink2);
    ok(hr == S_OK, "CoCreateInstance returned: %x\n", hr);

    hr = IDirectMusicSynth_QueryInterface(dmsynth, &IID_IKsControl, (LPVOID*)&control_synth);
    ok(hr == S_OK, "IDirectMusicSynth_QueryInterface returned: %x\n", hr);

    S(U(property)).Id = 0;
    S(U(property)).Flags = KSPROPERTY_TYPE_GET;

    S(U(property)).Set = GUID_DMUS_PROP_INSTRUMENT2;
    hr = IKsControl_KsProperty(control_synth, &property, sizeof(property), &value, sizeof(value), &bytes);
    ok(hr == S_OK, "IKsControl_KsProperty returned: %x\n", hr);
    ok(bytes == sizeof(DWORD), "Returned bytes: %u, should be 4\n", bytes);
    ok(value == TRUE, "Return value: %u, should be 1\n", value);
    S(U(property)).Set = GUID_DMUS_PROP_DLS2;
    hr = IKsControl_KsProperty(control_synth, &property, sizeof(property), &value, sizeof(value), &bytes);
    ok(hr == S_OK, "IKsControl_KsProperty returned: %x\n", hr);
    ok(bytes == sizeof(DWORD), "Returned bytes: %u, should be 4\n", bytes);
    ok(value == TRUE, "Return value: %u, should be 1\n", value);
    S(U(property)).Set = GUID_DMUS_PROP_GM_Hardware;
    hr = IKsControl_KsProperty(control_synth, &property, sizeof(property), &value, sizeof(value), &bytes);
    ok(hr == S_OK, "IKsControl_KsProperty returned: %x\n", hr);
    ok(bytes == sizeof(DWORD), "Returned bytes: %u, should be 4\n", bytes);
    ok(value == FALSE, "Return value: %u, should be 0\n", value);
    S(U(property)).Set = GUID_DMUS_PROP_GS_Hardware;
    hr = IKsControl_KsProperty(control_synth, &property, sizeof(property), &value, sizeof(value), &bytes);
    ok(hr == S_OK, "IKsControl_KsProperty returned: %x\n", hr);
    ok(bytes == sizeof(DWORD), "Returned bytes: %u, should be 4\n", bytes);
    ok(value == FALSE, "Return value: %u, should be 0\n", value);
    S(U(property)).Set = GUID_DMUS_PROP_XG_Hardware;
    hr = IKsControl_KsProperty(control_synth, &property, sizeof(property), &value, sizeof(value), &bytes);
    ok(hr == S_OK, "IKsControl_KsProperty returned: %x\n", hr);
    ok(bytes == sizeof(DWORD), "Returned bytes: %u, should be 4\n", bytes);
    ok(value == FALSE, "Return value: %u, should be 0\n", value);

    hr = IDirectMusicSynthSink_QueryInterface(dmsynth_sink, &IID_IKsControl, (LPVOID*)&control_sink);
    ok(hr == S_OK, "IDirectMusicSynthSink_QueryInterface returned: %x\n", hr);

    S(U(property)).Set = GUID_DMUS_PROP_SinkUsesDSound;
    hr = IKsControl_KsProperty(control_sink, &property, sizeof(property), &value, sizeof(value), &bytes);
    ok(hr == S_OK, "IKsControl_KsProperty returned: %x\n", hr);
    ok(bytes == sizeof(DWORD), "Returned bytes: %u, should be 4\n", bytes);
    ok(value == TRUE, "Return value: %u, should be 1\n", value);

    /* Synth isn't fully initialized yet */
    hr = IDirectMusicSynth_Activate(dmsynth, TRUE);
    ok(hr == DMUS_E_NOSYNTHSINK, "IDirectMusicSynth_Activate returned: %x\n", hr);

    /* Synth has no default clock */
    hr = IDirectMusicSynth_GetLatencyClock(dmsynth, &clock_synth);
    ok(hr == DMUS_E_NOSYNTHSINK, "IDirectMusicSynth_GetLatencyClock returned: %x\n", hr);

    /* SynthSink has a default clock */
    hr = IDirectMusicSynthSink_GetLatencyClock(dmsynth_sink, &clock_sink);
    ok(hr == S_OK, "IDirectMusicSynth_GetLatencyClock returned: %x\n", hr);
    ok(clock_sink != NULL, "No clock returned\n");
    ref_clock_sink = get_refcount(clock_sink);

    /* This will Init() the SynthSink and finish initializing the Synth */
    hr = IDirectMusicSynthSink_Init(dmsynth_sink2, NULL);
    ok(hr == S_OK, "IDirectMusicSynthSink_Init returned: %x\n", hr);
    hr = IDirectMusicSynth_SetSynthSink(dmsynth, dmsynth_sink2);
    ok(hr == S_OK, "IDirectMusicSynth_SetSynthSink returned: %x\n", hr);
    hr = IDirectMusicSynth_SetSynthSink(dmsynth, dmsynth_sink);
    ok(hr == S_OK, "IDirectMusicSynth_SetSynthSink returned: %x\n", hr);

    /* Check clocks are the same */
    hr = IDirectMusicSynth_GetLatencyClock(dmsynth, &clock_synth);
    ok(hr == S_OK, "IDirectMusicSynth_GetLatencyClock returned: %x\n", hr);
    ok(clock_synth != NULL, "No clock returned\n");
    ok(clock_synth == clock_sink, "Synth and SynthSink clocks are not the same\n");
    ref_clock_synth = get_refcount(clock_synth);
    ok(ref_clock_synth > ref_clock_sink + 1, "Latency clock refcount didn't increase\n");

    /* GetPortCaps */
    hr = IDirectMusicSynth_GetPortCaps(dmsynth, NULL);
    ok(hr == E_INVALIDARG, "GetPortCaps failed: %#x\n", hr);
    memset(&caps, 0, sizeof(caps));
    hr = IDirectMusicSynth_GetPortCaps(dmsynth, &caps);
    ok(hr == E_INVALIDARG, "GetPortCaps failed: %#x\n", hr);
    caps.dwSize = sizeof(caps) + 1;
    hr = IDirectMusicSynth_GetPortCaps(dmsynth, &caps);
    ok(hr == S_OK, "GetPortCaps failed: %#x\n", hr);

    if (control_synth)
        IDirectMusicSynth_Release(control_synth);
    if (control_sink)
        IDirectMusicSynth_Release(control_sink);
    if (clock_synth)
        IReferenceClock_Release(clock_synth);
    if (clock_sink)
        IReferenceClock_Release(clock_sink);
    if (dmsynth_sink)
        IDirectMusicSynthSink_Release(dmsynth_sink);
    if (dmsynth_sink2)
        IDirectMusicSynthSink_Release(dmsynth_sink2);
    IDirectMusicSynth_Release(dmsynth);
}

static void test_COM(void)
{
    IDirectMusicSynth8 *dms8 = (IDirectMusicSynth8*)0xdeadbeef;
    IKsControl *iksc;
    IUnknown *unk;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectMusicSynth, (IUnknown *)0xdeadbeef, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dms8);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicSynth create failed: %08x, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dms8, "dms8 = %p\n", dms8);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicSynth, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void**)&dms8);
    ok(hr == E_NOINTERFACE, "DirectMusicSynth create failed: %08x, expected E_NOINTERFACE\n", hr);

    /* Same refcount for all DirectMusicSynth interfaces */
    hr = CoCreateInstance(&CLSID_DirectMusicSynth, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicSynth8, (void**)&dms8);
    ok(hr == S_OK, "DirectMusicSynth create failed: %08x, expected S_OK\n", hr);
    refcount = IDirectMusicSynth8_AddRef(dms8);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);

    hr = IDirectMusicSynth8_QueryInterface(dms8, &IID_IKsControl, (void**)&iksc);
    ok(hr == S_OK, "QueryInterface for IID_IKsControl failed: %08x\n", hr);
    refcount = IKsControl_AddRef(iksc);
    ok(refcount == 4, "refcount == %u, expected 4\n", refcount);
    IKsControl_Release(iksc);

    hr = IDirectMusicSynth8_QueryInterface(dms8, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %08x\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 5, "refcount == %u, expected 5\n", refcount);
    IUnknown_Release(unk);

    /* Unsupported interfaces */
    hr = IDirectMusicSynth8_QueryInterface(dms8, &IID_IDirectMusicSynthSink, (void**)&unk);
    ok(hr == E_NOINTERFACE, "QueryInterface for IID_IDirectMusicSynthSink failed: %08x\n", hr);
    hr = IDirectMusicSynth8_QueryInterface(dms8, &IID_IReferenceClock, (void**)&unk);
    ok(hr == E_NOINTERFACE, "QueryInterface for IID_IReferenceClock failed: %08x\n", hr);

    while (IDirectMusicSynth8_Release(dms8));
}

static void test_COM_synthsink(void)
{
    IDirectMusicSynthSink *dmss = (IDirectMusicSynthSink*)0xdeadbeef;
    IKsControl *iksc;
    IUnknown *unk;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectMusicSynthSink, (IUnknown *)0xdeadbeef, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dmss);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicSynthSink create failed: %08x, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dmss, "dmss = %p\n", dmss);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicSynthSink, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void**)&dmss);
    ok(hr == E_NOINTERFACE, "DirectMusicSynthSink create failed: %08x, expected E_NOINTERFACE\n", hr);

    /* Same refcount for all DirectMusicSynthSink interfaces */
    hr = CoCreateInstance(&CLSID_DirectMusicSynthSink, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicSynthSink, (void**)&dmss);
    ok(hr == S_OK, "DirectMusicSynthSink create failed: %08x, expected S_OK\n", hr);
    refcount = IDirectMusicSynthSink_AddRef(dmss);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);

    hr = IDirectMusicSynthSink_QueryInterface(dmss, &IID_IKsControl, (void**)&iksc);
    ok(hr == S_OK, "QueryInterface for IID_IKsControl failed: %08x\n", hr);
    refcount = IKsControl_AddRef(iksc);
    ok(refcount == 4, "refcount == %u, expected 4\n", refcount);
    IKsControl_Release(iksc);

    hr = IDirectMusicSynthSink_QueryInterface(dmss, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %08x\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 5, "refcount == %u, expected 5\n", refcount);
    IUnknown_Release(unk);

    /* Unsupported interfaces */
    hr = IDirectMusicSynthSink_QueryInterface(dmss, &IID_IReferenceClock, (void**)&unk);
    ok(hr == E_NOINTERFACE, "QueryInterface for IID_IReferenceClock failed: %08x\n", hr);

    while (IDirectMusicSynthSink_Release(dmss));
}
START_TEST(dmsynth)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (missing_dmsynth())
    {
        skip("dmsynth not available\n");
        CoUninitialize();
        return;
    }
    test_dmsynth();
    test_COM();
    test_COM_synthsink();

    CoUninitialize();
}
