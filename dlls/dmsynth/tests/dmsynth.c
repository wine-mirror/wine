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

static void test_dmsynth(void)
{
    IDirectMusicSynth *dmsynth = NULL;
    IDirectMusicSynthSink *dmsynth_sink = NULL;
    IReferenceClock* clock_synth = NULL;
    IReferenceClock* clock_sink = NULL;
    IKsControl* control_synth = NULL;
    IKsControl* control_sink = NULL;
    HRESULT hr;
    KSPROPERTY property;
    ULONG value;
    ULONG bytes;

    hr = CoCreateInstance(&CLSID_DirectMusicSynth, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicSynth, (LPVOID*)&dmsynth);
    if (hr != S_OK)
    {
        skip("Cannot create DirectMusicSync object (%x)\n", hr);
        return;
    }

    hr = CoCreateInstance(&CLSID_DirectMusicSynthSink, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicSynthSink, (LPVOID*)&dmsynth_sink);
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

    /* Synth has no default clock */
    hr = IDirectMusicSynth_GetLatencyClock(dmsynth, &clock_synth);
    ok(hr == DMUS_E_NOSYNTHSINK, "IDirectMusicSynth_GetLatencyClock returned: %x\n", hr);

    /* SynthSink has a default clock */
    hr = IDirectMusicSynthSink_GetLatencyClock(dmsynth_sink, &clock_sink);
    ok(hr == S_OK, "IDirectMusicSynth_GetLatencyClock returned: %x\n", hr);
    ok(clock_sink != NULL, "No clock returned\n");

    /* This will set clock to Synth */
    hr = IDirectMusicSynth_SetSynthSink(dmsynth, dmsynth_sink);
    ok(hr == S_OK, "IDirectMusicSynth_SetSynthSink returned: %x\n", hr);

    /* Check clocks are the same */
    hr = IDirectMusicSynth_GetLatencyClock(dmsynth, &clock_synth);
    ok(hr == S_OK, "IDirectMusicSynth_GetLatencyClock returned: %x\n", hr);
    ok(clock_synth != NULL, "No clock returned\n");
    ok(clock_synth == clock_sink, "Synth and SynthSink clocks are not the same\n");

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
    IDirectMusicSynth_Release(dmsynth);
}

START_TEST(dmsynth)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    test_dmsynth();

    CoUninitialize();
}
