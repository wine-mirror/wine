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

static void test_dmsynth(void)
{
    IDirectMusicSynth *dmsynth = NULL;
    IDirectMusicSynthSink *dmsynth_sink = NULL;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectMusicSynth, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicSynth, (LPVOID*)&dmsynth);
    if (hr != S_OK)
    {
        skip("Cannot create DirectMusicSync object (%x)\n", hr);
        return;
    }

    hr = CoCreateInstance(&CLSID_DirectMusicSynthSink, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicSynthSink, (LPVOID*)&dmsynth_sink);
    ok(hr == S_OK, "CoCreateInstance returned: %x\n", hr);

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
