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

static BOOL missing_dmband(void)
{
    IDirectMusicBand *dmb;
    HRESULT hr = CoCreateInstance(&CLSID_DirectMusicBand, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicBand, (void**)&dmb);

    if (hr == S_OK && dmb)
    {
        IDirectMusicBand_Release(dmb);
        return FALSE;
    }
    return TRUE;
}

static void test_COM(void)
{
    IDirectMusicBand *dmb = (IDirectMusicBand*)0xdeadbeef;
    IDirectMusicObject *dmo;
    IPersistStream *ps;
    IUnknown *unk;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectMusicBand, (IUnknown*)&dmb, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dmb);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicBand create failed: %08x, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dmb, "dmb = %p\n", dmb);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicBand, NULL, CLSCTX_INPROC_SERVER, &IID_IClassFactory,
            (void**)&dmb);
    ok(hr == E_NOINTERFACE, "DirectMusicBand create failed: %08x, expected E_NOINTERFACE\n", hr);

    /* Same refcount for all DirectMusicBand interfaces */
    hr = CoCreateInstance(&CLSID_DirectMusicBand, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicBand,
            (void**)&dmb);
    ok(hr == S_OK, "DirectMusicBand create failed: %08x, expected S_OK\n", hr);
    refcount = IDirectMusicBand_AddRef(dmb);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);

    hr = IDirectMusicBand_QueryInterface(dmb, &IID_IDirectMusicObject, (void**)&dmo);
    ok(hr == S_OK, "QueryInterface for IID_IDirectMusicObject failed: %08x\n", hr);
    refcount = IDirectMusicObject_AddRef(dmo);
    ok(refcount == 4, "refcount == %u, expected 4\n", refcount);
    refcount = IDirectMusicObject_Release(dmo);

    hr = IDirectMusicBand_QueryInterface(dmb, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %08x\n", hr);
    refcount = IPersistStream_AddRef(ps);
    ok(refcount == 5, "refcount == %u, expected 5\n", refcount);
    refcount = IPersistStream_Release(ps);

    hr = IDirectMusicBand_QueryInterface(dmb, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %08x\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 6, "refcount == %u, expected 6\n", refcount);
    refcount = IUnknown_Release(unk);

    while (IDirectMusicBand_Release(dmb));
}

static void test_dmband(void)
{
    IUnknown *unknown = NULL;
    IDirectMusicTrack *track = NULL;
    IPersistStream *stream = NULL;
    IPersistStream *private = NULL;
    HRESULT hr;

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
}

START_TEST(dmband)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (missing_dmband())
    {
        skip("dmband not available\n");
        CoUninitialize();
        return;
    }

    test_COM();
    test_dmband();

    CoUninitialize();
}
