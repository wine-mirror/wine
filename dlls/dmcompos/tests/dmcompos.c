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

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

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

static void test_COM_template(void)
{
    IPersistStream *ps = (IPersistStream*)0xdeadbeef;
    IUnknown *unk;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectMusicTemplate, (IUnknown*)&ps, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&ps);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicTemplate create failed: %08x, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!ps, "ps = %p\n", ps);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicTemplate, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void**)&ps);
    todo_wine ok(hr == E_NOINTERFACE,
            "DirectMusicTemplate create failed: %08x, expected E_NOINTERFACE\n", hr);

    /* Same refcount for all DirectMusicTemplate interfaces */
    hr = CoCreateInstance(&CLSID_DirectMusicTemplate, NULL, CLSCTX_INPROC_SERVER,
            &IID_IPersistStream, (void**)&ps);
    todo_wine ok(hr == S_OK, "DirectMusicTemplate create failed: %08x, expected S_OK\n", hr);
    if (hr != S_OK) {
        skip("DirectMusicTemplate not implemented\n");
        return;
    }
    refcount = IPersistStream_AddRef(ps);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);

    hr = IPersistStream_QueryInterface(ps, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %08x\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 4, "refcount == %u, expected 4\n", refcount);
    refcount = IUnknown_Release(unk);

    while (IPersistStream_Release(ps));
}

static void test_COM_track(void)
{
    IDirectMusicTrack8 *dmt8;
    IPersistStream *ps;
    IUnknown *unk;
    ULONG refcount;
    HRESULT hr;
#define X(class)        &CLSID_ ## class, #class
    const struct {
        REFCLSID clsid;
        const char *name;
    } class[] = {
        { X(DirectMusicChordMapTrack) },
        { X(DirectMusicSignPostTrack) },
    };
#undef X
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(class); i++) {
        /* COM aggregation */
        dmt8 = (IDirectMusicTrack8*)0xdeadbeef;
        hr = CoCreateInstance(class[i].clsid, (IUnknown*)&dmt8, CLSCTX_INPROC_SERVER, &IID_IUnknown,
                (void**)&dmt8);
        if (hr == REGDB_E_CLASSNOTREG) {
            win_skip("%s not registered\n", class[i].name);
            continue;
        }
        ok(hr == CLASS_E_NOAGGREGATION,
                "%s create failed: %08x, expected CLASS_E_NOAGGREGATION\n", class[i].name, hr);
        ok(!dmt8, "dmt8 = %p\n", dmt8);

        /* Invalid RIID */
        hr = CoCreateInstance(class[i].clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicObject,
                (void**)&dmt8);
        ok(hr == E_NOINTERFACE, "%s create failed: %08x, expected E_NOINTERFACE\n",
                class[i].name, hr);

        /* Same refcount for all DirectMusicTrack interfaces */
        hr = CoCreateInstance(class[i].clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicTrack8,
                (void**)&dmt8);
        if (hr == E_NOINTERFACE && !dmt8) {
            skip("%s not created with CoCreateInstance()\n", class[i].name);
            continue;
        }
        ok(hr == S_OK, "%s create failed: %08x, expected S_OK\n", class[i].name, hr);
        refcount = IDirectMusicTrack8_AddRef(dmt8);
        ok(refcount == 2, "refcount == %u, expected 2\n", refcount);

        hr = IDirectMusicTrack8_QueryInterface(dmt8, &IID_IPersistStream, (void**)&ps);
        ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %08x\n", hr);
        refcount = IPersistStream_AddRef(ps);
        ok(refcount == 4, "refcount == %u, expected 4\n", refcount);
        refcount = IPersistStream_Release(ps);

        hr = IDirectMusicTrack8_QueryInterface(dmt8, &IID_IUnknown, (void**)&unk);
        ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %08x\n", hr);
        refcount = IUnknown_AddRef(unk);
        ok(refcount == 5, "refcount == %u, expected 5\n", refcount);
        refcount = IUnknown_Release(unk);

        while (IDirectMusicTrack8_Release(dmt8));
    }
}

static void test_chordmap(void)
{
    IDirectMusicChordMap *dmcm;
    IPersistStream *ps;
    CLSID class = { 0 };
    ULARGE_INTEGER size;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectMusicChordMap, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicChordMap, (void**)&dmcm);
    ok(hr == S_OK, "DirectMusicChordMap create failed: %08x, expected S_OK\n", hr);

    /* IPersistStream */
    hr = IDirectMusicChordMap_QueryInterface(dmcm, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %08x\n", hr);
    hr = IPersistStream_GetClassID(ps, &class);
    todo_wine ok(hr == S_OK, "IPersistStream_GetClassID failed: %08x\n", hr);
    todo_wine ok(IsEqualGUID(&class, &CLSID_DirectMusicChordMap),
            "Expected class CLSID_DirectMusicChordMap got %s\n", wine_dbgstr_guid(&class));

    /* Unimplemented IPersistStream methods */
    hr = IPersistStream_IsDirty(ps);
    todo_wine ok(hr == S_FALSE, "IPersistStream_IsDirty failed: %08x\n", hr);
    hr = IPersistStream_GetSizeMax(ps, &size);
    ok(hr == E_NOTIMPL, "IPersistStream_GetSizeMax failed: %08x\n", hr);
    hr = IPersistStream_Save(ps, NULL, TRUE);
    ok(hr == E_NOTIMPL, "IPersistStream_Save failed: %08x\n", hr);

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
    test_COM_template();
    test_COM_track();
    test_chordmap();

    CoUninitialize();
}
