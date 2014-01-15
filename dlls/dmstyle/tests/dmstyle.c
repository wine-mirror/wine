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

    CoUninitialize();
}
