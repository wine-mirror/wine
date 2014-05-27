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

static BOOL missing_dswave(void)
{
    IDirectMusicObject *dmo;
    HRESULT hr = CoCreateInstance(&CLSID_DirectSoundWave, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void**)&dmo);

    if (hr == S_OK && dmo)
    {
        IDirectMusicObject_Release(dmo);
        return FALSE;
    }
    return TRUE;
}

static void test_COM(void)
{
    IDirectMusicObject *dmo = (IDirectMusicObject*)0xdeadbeef;
    IPersistStream *ps;
    IUnknown *unk;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectSoundWave, (IUnknown*)&dmo, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dmo);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectSoundWave create failed: %08x, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dmo, "dmo = %p\n", dmo);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectSoundWave, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicSegment8, (void**)&dmo);
    todo_wine ok(hr == E_NOINTERFACE, "DirectSoundWave create failed: %08x, expected E_NOINTERFACE\n", hr);

    /* Same refcount for all DirectSoundWave interfaces */
    hr = CoCreateInstance(&CLSID_DirectSoundWave, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void**)&dmo);
    ok(hr == S_OK, "DirectSoundWave create failed: %08x, expected S_OK\n", hr);
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

    /* Interfaces that native does not support */
    hr = IDirectMusicObject_QueryInterface(dmo, &IID_IDirectMusicSegment, (void**)&unk);
    todo_wine ok(hr == E_NOINTERFACE, "QueryInterface for IID_IDirectMusicSegment failed: %08x\n", hr);
    hr = IDirectMusicObject_QueryInterface(dmo, &IID_IDirectMusicSegment8, (void**)&unk);
    todo_wine ok(hr == E_NOINTERFACE, "QueryInterface for IID_IDirectMusicSegment8 failed: %08x\n", hr);

    while (IDirectMusicObject_Release(dmo));
}

static void test_dswave(void)
{
    IDirectMusicObject *dmo;
    IPersistStream *ps;
    CLSID class = { 0 };
    ULARGE_INTEGER size;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectSoundWave, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void**)&dmo);
    ok(hr == S_OK, "DirectSoundWave create failed: %08x, expected S_OK\n", hr);

    /* IPersistStream */
    hr = IDirectMusicObject_QueryInterface(dmo, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %08x\n", hr);
    hr = IPersistStream_GetClassID(ps, &class);
    todo_wine ok(hr == S_OK, "IPersistStream_GetClassID failed: %08x\n", hr);
    todo_wine ok(IsEqualGUID(&class, &CLSID_DirectSoundWave),
            "Expected class CLSID_DirectSoundWave got %s\n", wine_dbgstr_guid(&class));

    /* Unimplemented IPersistStream methods */
    hr = IPersistStream_IsDirty(ps);
    todo_wine ok(hr == S_FALSE, "IPersistStream_IsDirty failed: %08x\n", hr);
    hr = IPersistStream_GetSizeMax(ps, &size);
    ok(hr == E_NOTIMPL, "IPersistStream_GetSizeMax failed: %08x\n", hr);
    hr = IPersistStream_Save(ps, NULL, TRUE);
    ok(hr == E_NOTIMPL, "IPersistStream_Save failed: %08x\n", hr);

    while (IDirectMusicObject_Release(dmo));
}

START_TEST(dswave)
{
    CoInitialize(NULL);

    if (missing_dswave())
    {
        skip("dswave not available\n");
        CoUninitialize();
        return;
    }
    test_COM();
    test_dswave();

    CoUninitialize();
}
