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

static BOOL missing_dmstyle(void)
{
    IDirectMusicStyle *dms;
    HRESULT hr = CoCreateInstance(&CLSID_DirectMusicStyle, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicStyle, (void**)&dms);

    if (hr == S_OK && dms)
    {
        IDirectMusicStyle_Release(dms);
        return FALSE;
    }
    return TRUE;
}

static void test_COM(void)
{
    IDirectMusicStyle8 *dms8 = (IDirectMusicStyle8*)0xdeadbeef;
    IDirectMusicObject *dmo;
    IPersistStream *ps;
    IUnknown *unk;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectMusicStyle, (IUnknown*)&dms8, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dms8);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicStyle8 create failed: %08x, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dms8, "dms8 = %p\n", dms8);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicStyle, NULL, CLSCTX_INPROC_SERVER, &IID_IClassFactory,
            (void**)&dms8);
    ok(hr == E_NOINTERFACE, "DirectMusicStyle8 create failed: %08x, expected E_NOINTERFACE\n", hr);

    /* Same refcount for all DirectMusicStyle8 interfaces */
    hr = CoCreateInstance(&CLSID_DirectMusicStyle, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicStyle8, (void**)&dms8);
    if (hr == E_NOINTERFACE) {
        win_skip("Old version without IDirectMusicStyle8\n");
        return;
    }
    ok(hr == S_OK, "DirectMusicStyle8 create failed: %08x, expected S_OK\n", hr);
    refcount = IDirectMusicStyle8_AddRef(dms8);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);

    hr = IDirectMusicStyle8_QueryInterface(dms8, &IID_IDirectMusicObject, (void**)&dmo);
    ok(hr == S_OK, "QueryInterface for IID_IDirectMusicObject failed: %08x\n", hr);
    refcount = IDirectMusicObject_AddRef(dmo);
    ok(refcount == 4, "refcount == %u, expected 4\n", refcount);
    refcount = IDirectMusicObject_Release(dmo);

    hr = IDirectMusicStyle8_QueryInterface(dms8, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %08x\n", hr);
    refcount = IPersistStream_AddRef(ps);
    ok(refcount == 5, "refcount == %u, expected 5\n", refcount);
    refcount = IPersistStream_Release(ps);

    hr = IDirectMusicStyle8_QueryInterface(dms8, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %08x\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 6, "refcount == %u, expected 6\n", refcount);
    refcount = IUnknown_Release(unk);

    while (IDirectMusicStyle8_Release(dms8));
}

static void test_COM_section(void)
{
    IDirectMusicObject *dmo = (IDirectMusicObject*)0xdeadbeef;
    IPersistStream *ps;
    IUnknown *unk;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectMusicSection, (IUnknown*)&dmo, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dmo);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicSection create failed: %08x, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dmo, "dmo = %p\n", dmo);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicSection, NULL, CLSCTX_INPROC_SERVER, &IID_IClassFactory,
            (void**)&dmo);
    todo_wine ok(hr == E_NOINTERFACE,
            "DirectMusicSection create failed: %08x, expected E_NOINTERFACE\n", hr);

    /* Same refcount for all DirectMusicObject interfaces */
    hr = CoCreateInstance(&CLSID_DirectMusicSection, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void**)&dmo);
    todo_wine ok(hr == S_OK, "DirectMusicSection create failed: %08x, expected S_OK\n", hr);
    if (hr != S_OK) {
        skip("DirectMusicSection not implemented\n");
        return;
    }
    refcount = IDirectMusicObject_AddRef(dmo);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);

    hr = IDirectMusicObject_QueryInterface(dmo, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %08x\n", hr);
    refcount = IPersistStream_AddRef(ps);
    ok(refcount == 4, "refcount == %u, expected 4\n", refcount);
    refcount = IPersistStream_Release(ps);

    hr = IDirectMusicObject_QueryInterface(dmo, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %08x\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 5, "refcount == %u, expected 5\n", refcount);
    refcount = IUnknown_Release(unk);

    while (IDirectMusicObject_Release(dmo));
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
        { X(DirectMusicAuditionTrack) },
        { X(DirectMusicChordTrack) },
        { X(DirectMusicCommandTrack) },
        { X(DirectMusicMotifTrack) },
        { X(DirectMusicMuteTrack) },
        { X(DirectMusicStyleTrack) },
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

static void test_dmstyle(void)
{
    IDirectMusicStyle *dms;
    IPersistStream *ps;
    CLSID class = { 0 };
    ULARGE_INTEGER size;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectMusicStyle, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicStyle, (void**)&dms);
    ok(hr == S_OK, "DirectMusicStyle create failed: %08x, expected S_OK\n", hr);

    /* IPersistStream */
    hr = IDirectMusicStyle_QueryInterface(dms, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %08x\n", hr);
    hr = IPersistStream_GetClassID(ps, &class);
    ok(hr == S_OK, "IPersistStream_GetClassID failed: %08x\n", hr);
    ok(IsEqualGUID(&class, &CLSID_DirectMusicStyle),
            "Expected class CLSID_DirectMusicStyle got %s\n", wine_dbgstr_guid(&class));

    /* Unimplemented IPersistStream methods*/
    hr = IPersistStream_IsDirty(ps);
    ok(hr == S_FALSE, "IPersistStream_IsDirty failed: %08x\n", hr);
    hr = IPersistStream_GetSizeMax(ps, &size);
    ok(hr == E_NOTIMPL, "IPersistStream_GetSizeMax failed: %08x\n", hr);
    hr = IPersistStream_Save(ps, NULL, TRUE);
    ok(hr == E_NOTIMPL, "IPersistStream_Save failed: %08x\n", hr);

    while (IDirectMusicStyle_Release(dms));
}

START_TEST(dmstyle)
{
    CoInitialize(NULL);

    if (missing_dmstyle())
    {
        skip("dmstyle not available\n");
        CoUninitialize();
        return;
    }
    test_COM();
    test_COM_section();
    test_COM_track();
    test_dmstyle();

    CoUninitialize();
}
