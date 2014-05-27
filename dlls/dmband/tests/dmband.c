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

static void test_COM_bandtrack(void)
{
    IDirectMusicTrack *dmbt = (IDirectMusicTrack*)0xdeadbeef;
    IPersistStream *ps;
    IUnknown *private;
    IUnknown *unk;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectMusicBandTrack, (IUnknown*)&dmbt, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dmbt);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicBandTrack create failed: %08x, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dmbt, "dmbt = %p\n", dmbt);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicBandTrack, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void**)&dmbt);
    ok(hr == E_NOINTERFACE, "DirectMusicBandTrack create failed: %08x, expected E_NOINTERFACE\n", hr);

    /* Same refcount for all DirectMusicBandTrack interfaces */
    hr = CoCreateInstance(&CLSID_DirectMusicBandTrack, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicTrack, (void**)&dmbt);
    ok(hr == S_OK, "DirectMusicBandTrack create failed: %08x, expected S_OK\n", hr);
    refcount = IDirectMusicTrack_AddRef(dmbt);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);

    hr = IDirectMusicTrack_QueryInterface(dmbt, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %08x\n", hr);
    refcount = IPersistStream_AddRef(ps);
    ok(refcount == 4, "refcount == %u, expected 4\n", refcount);
    refcount = IPersistStream_Release(ps);

    hr = IDirectMusicTrack_QueryInterface(dmbt, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %08x\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 5, "refcount == %u, expected 5\n", refcount);
    refcount = IUnknown_Release(unk);

    hr = IDirectMusicTrack_QueryInterface(dmbt, &IID_IDirectMusicBandTrackPrivate,
            (void**)&private);
    todo_wine ok(hr == S_OK, "QueryInterface for IID_IDirectMusicBandTrackPrivate failed: %08x\n", hr);
    if (hr == S_OK) {
        refcount = IUnknown_AddRef(private);
        ok(refcount == 6, "refcount == %u, expected 6\n", refcount);
        refcount = IUnknown_Release(private);
    }

    while (IDirectMusicTrack_Release(dmbt));
}

static void test_dmband(void)
{
    IDirectMusicBand *dmb;
    IPersistStream *ps;
    CLSID class;
    ULARGE_INTEGER size;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectMusicBand, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicBand, (void**)&dmb);
    ok(hr == S_OK, "DirectMusicBand create failed: %08x, expected S_OK\n", hr);

    /* Unimplemented IPersistStream methods */
    hr = IDirectMusicBand_QueryInterface(dmb, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %08x\n", hr);
    hr = IPersistStream_GetClassID(ps, &class);
    todo_wine ok(hr == E_NOTIMPL, "IPersistStream_GetClassID failed: %08x\n", hr);
    hr = IPersistStream_IsDirty(ps);
    ok(hr == S_FALSE, "IPersistStream_IsDirty failed: %08x\n", hr);
    hr = IPersistStream_GetSizeMax(ps, &size);
    ok(hr == E_NOTIMPL, "IPersistStream_GetSizeMax failed: %08x\n", hr);
    hr = IPersistStream_Save(ps, NULL, TRUE);
    ok(hr == E_NOTIMPL, "IPersistStream_Save failed: %08x\n", hr);

    while (IDirectMusicBand_Release(dmb));
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
    test_COM_bandtrack();
    test_dmband();

    CoUninitialize();
}
