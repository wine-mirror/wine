/*
 * Copyright 2014 Michael Stefaniuc
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


static BOOL missing_dmcompos(void)
{
    IDirectMusicComposer *dmc;
    HRESULT hr = CoCreateInstance(&CLSID_DirectMusicComposer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicComposer, (void**)&dmc);

    if (hr == S_OK && dmc)
    {
        IDirectMusicComposer_Release(dmc);
        return FALSE;
    }
    return TRUE;
}

static void test_COM(void)
{
    IDirectMusicComposer *dmc = (IDirectMusicComposer*)0xdeadbeef;
    IUnknown *unk;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectMusicComposer, (IUnknown*)&dmc, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dmc);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicComposer create failed: %08x, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dmc, "dmc = %p\n", dmc);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicComposer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void**)&dmc);
    ok(hr == E_NOINTERFACE,
            "DirectMusicComposer create failed: %08x, expected E_NOINTERFACE\n", hr);

    /* Same refcount for all DirectMusicComposer interfaces */
    hr = CoCreateInstance(&CLSID_DirectMusicComposer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicComposer, (void**)&dmc);
    ok(hr == S_OK, "DirectMusicComposer create failed: %08x, expected S_OK\n", hr);
    refcount = IDirectMusicComposer_AddRef(dmc);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);

    hr = IDirectMusicComposer_QueryInterface(dmc, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %08x\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 4, "refcount == %u, expected 4\n", refcount);
    refcount = IUnknown_Release(unk);

    while (IDirectMusicComposer_Release(dmc));
}

static void test_COM_chordmap(void)
{
    IDirectMusicChordMap *dmcm = (IDirectMusicChordMap*)0xdeadbeef;
    IDirectMusicObject *dmo;
    IPersistStream *ps;
    IUnknown *unk;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectMusicChordMap, (IUnknown*)&dmcm, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dmcm);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicChordMap create failed: %08x, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dmcm, "dmcm = %p\n", dmcm);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicChordMap, NULL, CLSCTX_INPROC_SERVER,
            &IID_IClassFactory, (void**)&dmcm);
    ok(hr == E_NOINTERFACE,
            "DirectMusicChordMap create failed: %08x, expected E_NOINTERFACE\n", hr);

    /* Same refcount for all DirectMusicChordMap interfaces */
    hr = CoCreateInstance(&CLSID_DirectMusicChordMap, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicChordMap, (void**)&dmcm);
    ok(hr == S_OK, "DirectMusicChordMap create failed: %08x, expected S_OK\n", hr);
    refcount = IDirectMusicChordMap_AddRef(dmcm);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);

    hr = IDirectMusicChordMap_QueryInterface(dmcm, &IID_IDirectMusicObject, (void**)&dmo);
    ok(hr == S_OK, "QueryInterface for IID_IDirectMusicObject failed: %08x\n", hr);
    refcount = IDirectMusicObject_AddRef(dmo);
    ok(refcount == 4, "refcount == %u, expected 4\n", refcount);
    refcount = IDirectMusicObject_Release(dmo);

    hr = IDirectMusicChordMap_QueryInterface(dmcm, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %08x\n", hr);
    refcount = IPersistStream_AddRef(ps);
    ok(refcount == 5, "refcount == %u, expected 5\n", refcount);
    refcount = IPersistStream_Release(ps);

    hr = IDirectMusicChordMap_QueryInterface(dmcm, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %08x\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 6, "refcount == %u, expected 6\n", refcount);
    refcount = IUnknown_Release(unk);

    while (IDirectMusicChordMap_Release(dmcm));
}

START_TEST(dmcompos)
{
    CoInitialize(NULL);

    if (missing_dmcompos())
    {
        skip("dmcompos not available\n");
        CoUninitialize();
        return;
    }
    test_COM();
    test_COM_chordmap();

    CoUninitialize();
}
