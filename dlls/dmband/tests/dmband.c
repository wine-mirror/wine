/*
 * Unit tests for dmband functions
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
#include "dmusici.h"
#include "dmplugin.h"

DEFINE_GUID(IID_IDirectMusicBandTrackPrivate, 0x53466056, 0x6dc4, 0x11d1, 0xbf, 0x7b, 0x00, 0xc0, 0x4f, 0xbf, 0x8f, 0xef);

static void test_dmband(void)
{
    IDirectMusicBand *band;
    IUnknown *unknown = NULL;
    IDirectMusicTrack *track = NULL;
    IPersistStream *stream = NULL;
    IPersistStream *private = NULL;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectMusicBand, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicBand, (LPVOID*)&band);
    if (hr != S_OK)
    {
        skip("Cannot create DirectMusicBand object (%x)\n", hr);
        return;
    }

    hr = CoCreateInstance(&CLSID_DirectMusicBandTrack, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (LPVOID*)&unknown);
    ok(hr == S_OK, "CoCreateInstance returned: %x\n", hr);
    hr = IUnknown_QueryInterface(unknown, &IID_IDirectMusicTrack, (LPVOID*)&track);
    ok(hr == S_OK, "QueryInterface returned: %x\n", hr);
    todo_wine ok((LPVOID)track == (LPVOID)unknown, "Interface are not the same %p != %p\n", stream, private);
    hr = IUnknown_QueryInterface(unknown, &IID_IPersistStream, (LPVOID*)&stream);
    ok(hr == S_OK, "QueryInterface returned: %x\n", hr);
    /* Query private interface */
    hr = IUnknown_QueryInterface(unknown, &IID_IDirectMusicBandTrackPrivate, (LPVOID*)&private);
    todo_wine ok(hr == S_OK, "QueryInterface returned: %x\n", hr);

    trace("Interfaces: unknown = %p, track = %p, stream = %p, private = %p\n", unknown, track, stream, private);

    if (private)
        IPersistStream_Release(private);
    if (stream)
        IPersistStream_Release(stream);
    if (track)
        IDirectMusicTrack_Release(track);
    if (unknown)
        IUnknown_Release(unknown);
    IDirectMusicBand_Release(band);
}

START_TEST(dmband)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    test_dmband();

    CoUninitialize();
}
