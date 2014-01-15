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

    CoUninitialize();
}
