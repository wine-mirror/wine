/*
 * Copyright 2020 Zebediah Figura
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
#include "windef.h"
#include "wingdi.h"
#include "mmreg.h"
#include "mmsystem.h"
#include "dmo.h"
#include "initguid.h"
#include "dsound.h"
#include "uuids.h"
#include "wine/test.h"

static const GUID test_iid = {0x33333333};
static LONG outer_ref = 1;

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

static HRESULT WINAPI outer_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    if (IsEqualGUID(iid, &IID_IUnknown)
            || IsEqualGUID(iid, &IID_IMediaObject)
            || IsEqualGUID(iid, &test_iid))
    {
        *out = (IUnknown *)0xdeadbeef;
        return S_OK;
    }
    ok(0, "unexpected call %s\n", wine_dbgstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI outer_AddRef(IUnknown *iface)
{
    return InterlockedIncrement(&outer_ref);
}

static ULONG WINAPI outer_Release(IUnknown *iface)
{
    return InterlockedDecrement(&outer_ref);
}

static const IUnknownVtbl outer_vtbl =
{
    outer_QueryInterface,
    outer_AddRef,
    outer_Release,
};

static IUnknown test_outer = {&outer_vtbl};

static void test_aggregation(const GUID *clsid)
{
    IMediaObject *dmo, *dmo2;
    IUnknown *unk, *unk2;
    HRESULT hr;
    ULONG ref;

    dmo = (IMediaObject *)0xdeadbeef;
    hr = CoCreateInstance(clsid, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IMediaObject, (void **)&dmo);
    todo_wine ok(hr == E_NOINTERFACE, "Got hr %#x.\n", hr);
    if (hr != E_NOINTERFACE)
        return;
    ok(!dmo, "Got interface %p.\n", dmo);

    hr = CoCreateInstance(clsid, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(outer_ref == 1, "Got unexpected refcount %d.\n", outer_ref);
    ok(unk != &test_outer, "Returned IUnknown should not be outer IUnknown.\n");
    ref = get_refcount(unk);
    ok(ref == 1, "Got unexpected refcount %d.\n", ref);

    ref = IUnknown_AddRef(unk);
    ok(ref == 2, "Got unexpected refcount %d.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %d.\n", outer_ref);

    ref = IUnknown_Release(unk);
    ok(ref == 1, "Got unexpected refcount %d.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %d.\n", outer_ref);

    hr = IUnknown_QueryInterface(unk, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(unk2 == unk, "Got unexpected IUnknown %p.\n", unk2);
    IUnknown_Release(unk2);

    hr = IUnknown_QueryInterface(unk, &IID_IMediaObject, (void **)&dmo);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaObject_QueryInterface(dmo, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(unk2 == (IUnknown *)0xdeadbeef, "Got unexpected IUnknown %p.\n", unk2);

    hr = IMediaObject_QueryInterface(dmo, &IID_IMediaObject, (void **)&dmo2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(dmo2 == (IMediaObject *)0xdeadbeef, "Got unexpected IMediaObject %p.\n", dmo2);

    hr = IUnknown_QueryInterface(unk, &test_iid, (void **)&unk2);
    ok(hr == E_NOINTERFACE, "Got hr %#x.\n", hr);
    ok(!unk2, "Got unexpected IUnknown %p.\n", unk2);

    hr = IMediaObject_QueryInterface(dmo, &test_iid, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(unk2 == (IUnknown *)0xdeadbeef, "Got unexpected IUnknown %p.\n", unk2);

    IMediaObject_Release(dmo);
    ref = IUnknown_Release(unk);
    ok(!ref, "Got unexpected refcount %d.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %d.\n", outer_ref);
}

START_TEST(dsdmo)
{
    static const struct
    {
        const GUID *clsid;
        const GUID *iid;
    }
    tests[] =
    {
        {&GUID_DSFX_STANDARD_CHORUS,        &IID_IDirectSoundFXChorus},
        {&GUID_DSFX_STANDARD_COMPRESSOR,    &IID_IDirectSoundFXCompressor},
        {&GUID_DSFX_STANDARD_DISTORTION,    &IID_IDirectSoundFXDistortion},
        {&GUID_DSFX_STANDARD_ECHO,          &IID_IDirectSoundFXEcho},
        {&GUID_DSFX_STANDARD_FLANGER,       &IID_IDirectSoundFXFlanger},
        {&GUID_DSFX_STANDARD_GARGLE,        &IID_IDirectSoundFXGargle},
        {&GUID_DSFX_STANDARD_I3DL2REVERB,   &IID_IDirectSoundFXI3DL2Reverb},
        {&GUID_DSFX_STANDARD_PARAMEQ,       &IID_IDirectSoundFXParamEq},
        {&GUID_DSFX_WAVES_REVERB,           &IID_IDirectSoundFXWavesReverb},
    };
    unsigned int i;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        test_aggregation(tests[i].clsid);
    }

    CoUninitialize();
}
