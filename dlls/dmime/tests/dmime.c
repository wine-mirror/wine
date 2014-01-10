/*
 * Copyright 2012, 2014 Michael Stefaniuc
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
#include <wine/test.h>
#include <dmusici.h>

static BOOL missing_dmime(void)
{
    IDirectMusicSegment8 *dms;
    HRESULT hr = CoCreateInstance(&CLSID_DirectMusicSegment, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicSegment, (void**)&dms);

    if (hr == S_OK && dms)
    {
        IDirectMusicSegment_Release(dms);
        return FALSE;
    }
    return TRUE;
}

static void test_COM_segment(void)
{
    IDirectMusicSegment8 *dms = (IDirectMusicSegment8*)0xdeadbeef;
    IDirectMusicObject *dmo;
    IPersistStream *stream;
    IUnknown *unk;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectMusicSegment, (IUnknown*)&dms, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dms);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicSegment create failed: %08x, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dms, "dms = %p\n", dms);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicSegment, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectSound, (void**)&dms);
    ok(hr == E_NOINTERFACE,
            "DirectMusicSegment create failed: %08x, expected E_NOINTERFACE\n", hr);

    /* Same refcount */
    hr = CoCreateInstance(&CLSID_DirectMusicSegment, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicSegment8, (void**)&dms);
    if (hr == E_NOINTERFACE) {
        win_skip("DirectMusicSegment without IDirectMusicSegment8\n");
        return;
    }
    ok(hr == S_OK, "DirectMusicSegment create failed: %08x, expected S_OK\n", hr);
    refcount = IDirectMusicSegment8_AddRef(dms);
    ok (refcount == 2, "refcount == %u, expected 2\n", refcount);
    hr = IDirectMusicSegment8_QueryInterface(dms, &IID_IDirectMusicObject, (void**)&dmo);
    ok(hr == S_OK, "QueryInterface for IID_IDirectMusicObject failed: %08x\n", hr);
    IDirectMusicSegment8_AddRef(dms);
    refcount = IDirectMusicSegment8_Release(dms);
    ok (refcount == 3, "refcount == %u, expected 3\n", refcount);
    hr = IDirectMusicSegment8_QueryInterface(dms, &IID_IPersistStream, (void**)&stream);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %08x\n", hr);
    refcount = IDirectMusicSegment8_Release(dms);
    ok (refcount == 3, "refcount == %u, expected 3\n", refcount);
    hr = IDirectMusicSegment8_QueryInterface(dms, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %08x\n", hr);
    refcount = IUnknown_Release(unk);
    ok (refcount == 3, "refcount == %u, expected 3\n", refcount);
    refcount = IDirectMusicObject_Release(dmo);
    ok (refcount == 2, "refcount == %u, expected 2\n", refcount);
    refcount = IPersistStream_Release(stream);
    ok (refcount == 1, "refcount == %u, expected 1\n", refcount);
    refcount = IDirectMusicSegment8_Release(dms);
    ok (refcount == 0, "refcount == %u, expected 0\n", refcount);
}

START_TEST(dmime)
{
    CoInitialize(NULL);

    if (missing_dmime())
    {
        skip("dmime not available\n");
        CoUninitialize();
        return;
    }
    test_COM_segment();

    CoUninitialize();
}
