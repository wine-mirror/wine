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

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

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

static void test_COM_audiopath(void)
{
    IDirectMusicAudioPath *dmap;
    IUnknown *unk;
    IDirectMusicPerformance8 *performance;
    ULONG refcount;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicPerformance8, (void**)&performance);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE),
                "DirectMusicPerformance create failed: %08x\n", hr);
    if (!performance) {
        win_skip("IDirectMusicPerformance8 not available\n");
        return;
    }
    hr = IDirectMusicPerformance8_InitAudio(performance, NULL, NULL, NULL,
            DMUS_APATH_SHARED_STEREOPLUSREVERB, 64, DMUS_AUDIOF_ALL, NULL);
    if (hr == DSERR_NODRIVER) {
        skip("No audio driver\n");
        return;
    }
    ok(hr == S_OK, "DirectMusicPerformance_InitAudio failed: %08x\n", hr);
    hr = IDirectMusicPerformance8_GetDefaultAudioPath(performance, &dmap);
    ok(hr == S_OK, "DirectMusicPerformance_GetDefaultAudioPath failed: %08x\n", hr);

    /* IDirectMusicObject and IPersistStream are not supported */
    hr = IDirectMusicAudioPath_QueryInterface(dmap, &IID_IDirectMusicObject, (void**)&unk);
    todo_wine ok(FAILED(hr) && !unk, "Unexpected IDirectMusicObject interface: hr=%08x, iface=%p\n",
            hr, unk);
    if (unk) IUnknown_Release(unk);
    hr = IDirectMusicAudioPath_QueryInterface(dmap, &IID_IPersistStream, (void**)&unk);
    todo_wine ok(FAILED(hr) && !unk, "Unexpected IPersistStream interface: hr=%08x, iface=%p\n",
            hr, unk);
    if (unk) IUnknown_Release(unk);

    /* Same refcount for all DirectMusicAudioPath interfaces */
    refcount = IDirectMusicAudioPath_AddRef(dmap);
    ok(refcount == 3, "refcount == %u, expected 3\n", refcount);

    hr = IDirectMusicAudioPath_QueryInterface(dmap, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %08x\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 5, "refcount == %u, expected 5\n", refcount);
    refcount = IUnknown_Release(unk);

    while (IDirectMusicAudioPath_Release(dmap) > 1); /* performance has a reference too */
    IDirectMusicPerformance8_CloseDown(performance);
    IDirectMusicPerformance8_Release(performance);
}

static void test_COM_audiopathconfig(void)
{
    IDirectMusicAudioPath *dmap = (IDirectMusicAudioPath*)0xdeadbeef;
    IDirectMusicObject *dmo;
    IPersistStream *ps;
    IUnknown *unk;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectMusicAudioPathConfig, (IUnknown*)&dmap, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dmap);
    if (hr == REGDB_E_CLASSNOTREG) {
        win_skip("DirectMusicAudioPathConfig not registered\n");
        return;
    }
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicAudioPathConfig create failed: %08x, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dmap, "dmap = %p\n", dmap);

    /* IDirectMusicAudioPath not supported */
    hr = CoCreateInstance(&CLSID_DirectMusicAudioPathConfig, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicAudioPath, (void**)&dmap);
    todo_wine ok(FAILED(hr) && !dmap,
            "Unexpected IDirectMusicAudioPath interface: hr=%08x, iface=%p\n", hr, dmap);

    /* IDirectMusicObject and IPersistStream supported */
    hr = CoCreateInstance(&CLSID_DirectMusicAudioPathConfig, NULL, CLSCTX_INPROC_SERVER,
            &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "DirectMusicObject create failed: %08x, expected S_OK\n", hr);
    IPersistStream_Release(ps);
    hr = CoCreateInstance(&CLSID_DirectMusicAudioPathConfig, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void**)&dmo);
    ok(hr == S_OK, "DirectMusicObject create failed: %08x, expected S_OK\n", hr);

    /* Same refcount for all DirectMusicObject interfaces */
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

    /* IDirectMusicAudioPath still not supported */
    hr = IDirectMusicObject_QueryInterface(dmo, &IID_IDirectMusicAudioPath, (void**)&dmap);
    todo_wine ok(FAILED(hr) && !dmap,
            "Unexpected IDirectMusicAudioPath interface: hr=%08x, iface=%p\n", hr, dmap);

    while (IDirectMusicObject_Release(dmo));
}


static void test_COM_graph(void)
{
    IDirectMusicGraph *dmg = (IDirectMusicGraph*)0xdeadbeef;
    IDirectMusicObject *dmo;
    IPersistStream *ps;
    IUnknown *unk;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectMusicGraph, (IUnknown*)&dmg, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dmg);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicGraph create failed: %08x, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dmg, "dmg = %p\n", dmg);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IClassFactory,
            (void**)&dmg);
    ok(hr == E_NOINTERFACE, "DirectMusicGraph create failed: %08x, expected E_NOINTERFACE\n", hr);

    /* Same refcount for all DirectMusicGraph interfaces */
    hr = CoCreateInstance(&CLSID_DirectMusicGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicGraph, (void**)&dmg);
    ok(hr == S_OK, "DirectMusicGraph create failed: %08x, expected S_OK\n", hr);
    refcount = IDirectMusicGraph_AddRef(dmg);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);

    hr = IDirectMusicGraph_QueryInterface(dmg, &IID_IDirectMusicObject, (void**)&dmo);
    if (hr == E_NOINTERFACE) {
        win_skip("DirectMusicGraph without IDirectMusicObject\n");
        return;
    }
    ok(hr == S_OK, "QueryInterface for IID_IDirectMusicObject failed: %08x\n", hr);
    refcount = IDirectMusicObject_AddRef(dmo);
    ok(refcount == 4, "refcount == %u, expected 4\n", refcount);
    refcount = IDirectMusicObject_Release(dmo);

    hr = IDirectMusicGraph_QueryInterface(dmg, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %08x\n", hr);
    refcount = IPersistStream_AddRef(ps);
    ok(refcount == 5, "refcount == %u, expected 5\n", refcount);
    refcount = IPersistStream_Release(ps);

    hr = IDirectMusicGraph_QueryInterface(dmg, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %08x\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 6, "refcount == %u, expected 6\n", refcount);
    refcount = IUnknown_Release(unk);

    while (IDirectMusicGraph_Release(dmg));
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

static void test_COM_segmentstate(void)
{
    IDirectMusicSegmentState8 *dmss8 = (IDirectMusicSegmentState8*)0xdeadbeef;
    IUnknown *unk;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectMusicSegmentState, (IUnknown*)&dmss8, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dmss8);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicSegmentState8 create failed: %08x, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dmss8, "dmss8 = %p\n", dmss8);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicSegmentState, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void**)&dmss8);
    ok(hr == E_NOINTERFACE,
            "DirectMusicSegmentState8 create failed: %08x, expected E_NOINTERFACE\n", hr);

    /* Same refcount for all DirectMusicSegmentState interfaces */
    hr = CoCreateInstance(&CLSID_DirectMusicSegmentState, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicSegmentState8, (void**)&dmss8);
    if (hr == E_NOINTERFACE) {
        win_skip("DirectMusicSegmentState without IDirectMusicSegmentState8\n");
        return;
    }
    ok(hr == S_OK, "DirectMusicSegmentState8 create failed: %08x, expected S_OK\n", hr);
    refcount = IDirectMusicSegmentState8_AddRef(dmss8);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);

    hr = IDirectMusicSegmentState8_QueryInterface(dmss8, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %08x\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 4, "refcount == %u, expected 4\n", refcount);
    refcount = IUnknown_Release(unk);

    hr = IDirectMusicSegmentState8_QueryInterface(dmss8, &IID_IUnknown, NULL);
    ok(hr == E_POINTER, "got %08x\n", hr);

    while (IDirectMusicSegmentState8_Release(dmss8));
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
        { X(DirectMusicLyricsTrack) },
        { X(DirectMusicMarkerTrack) },
        { X(DirectMusicParamControlTrack) },
        { X(DirectMusicSegmentTriggerTrack) },
        { X(DirectMusicSeqTrack) },
        { X(DirectMusicSysExTrack) },
        { X(DirectMusicTempoTrack) },
        { X(DirectMusicTimeSigTrack) },
        { X(DirectMusicWaveTrack) }
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

static void test_audiopathconfig(void)
{
    IDirectMusicObject *dmo;
    IPersistStream *ps;
    CLSID class = { 0 };
    ULARGE_INTEGER size;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectMusicAudioPathConfig, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void**)&dmo);
    ok(hr == S_OK, "DirectMusicAudioPathConfig create failed: %08x, expected S_OK\n", hr);

    /* IPersistStream */
    hr = IDirectMusicObject_QueryInterface(dmo, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %08x\n", hr);
    hr = IPersistStream_GetClassID(ps, &class);
    todo_wine ok(hr == S_OK, "IPersistStream_GetClassID failed: %08x\n", hr);
    todo_wine ok(IsEqualGUID(&class, &CLSID_DirectMusicAudioPathConfig),
            "Expected class CLSID_DirectMusicAudioPathConfig got %s\n", wine_dbgstr_guid(&class));

    /* Unimplemented IPersistStream methods */
    hr = IPersistStream_IsDirty(ps);
    todo_wine ok(hr == S_FALSE, "IPersistStream_IsDirty failed: %08x\n", hr);
    hr = IPersistStream_GetSizeMax(ps, &size);
    ok(hr == E_NOTIMPL, "IPersistStream_GetSizeMax failed: %08x\n", hr);
    hr = IPersistStream_Save(ps, NULL, TRUE);
    ok(hr == E_NOTIMPL, "IPersistStream_Save failed: %08x\n", hr);

    while (IDirectMusicObject_Release(dmo));
}

static void test_graph(void)
{
    IDirectMusicGraph *dmg;
    IPersistStream *ps;
    CLSID class = { 0 };
    ULARGE_INTEGER size;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectMusicGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicGraph, (void**)&dmg);
    ok(hr == S_OK, "DirectMusicGraph create failed: %08x, expected S_OK\n", hr);

    /* IPersistStream */
    hr = IDirectMusicGraph_QueryInterface(dmg, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %08x\n", hr);
    hr = IPersistStream_GetClassID(ps, &class);
    todo_wine ok(hr == S_OK, "IPersistStream_GetClassID failed: %08x\n", hr);
    todo_wine ok(IsEqualGUID(&class, &CLSID_DirectMusicGraph),
            "Expected class CLSID_DirectMusicGraph got %s\n", wine_dbgstr_guid(&class));

    /* Unimplemented IPersistStream methods */
    hr = IPersistStream_IsDirty(ps);
    todo_wine ok(hr == S_FALSE, "IPersistStream_IsDirty failed: %08x\n", hr);
    hr = IPersistStream_GetSizeMax(ps, &size);
    ok(hr == E_NOTIMPL, "IPersistStream_GetSizeMax failed: %08x\n", hr);
    hr = IPersistStream_Save(ps, NULL, TRUE);
    ok(hr == E_NOTIMPL, "IPersistStream_Save failed: %08x\n", hr);

    while (IDirectMusicGraph_Release(dmg));
}

static void test_segment(void)
{
    IDirectMusicSegment *dms;
    IPersistStream *ps;
    CLSID class = { 0 };
    ULARGE_INTEGER size;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectMusicSegment, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicSegment, (void**)&dms);
    ok(hr == S_OK, "DirectMusicSegment create failed: %08x, expected S_OK\n", hr);

    /* IPersistStream */
    hr = IDirectMusicSegment_QueryInterface(dms, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %08x\n", hr);
    hr = IPersistStream_GetClassID(ps, &class);
    ok(hr == S_OK, "IPersistStream_GetClassID failed: %08x\n", hr);
    ok(IsEqualGUID(&class, &CLSID_DirectMusicSegment),
            "Expected class CLSID_DirectMusicSegment got %s\n", wine_dbgstr_guid(&class));

    /* Unimplemented IPersistStream methods */
    hr = IPersistStream_IsDirty(ps);
    ok(hr == S_FALSE, "IPersistStream_IsDirty failed: %08x\n", hr);
    hr = IPersistStream_GetSizeMax(ps, &size);
    ok(hr == E_NOTIMPL, "IPersistStream_GetSizeMax failed: %08x\n", hr);
    hr = IPersistStream_Save(ps, NULL, TRUE);
    ok(hr == E_NOTIMPL, "IPersistStream_Save failed: %08x\n", hr);

    while (IDirectMusicSegment_Release(dms));
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
    test_COM_audiopath();
    test_COM_audiopathconfig();
    test_COM_graph();
    test_COM_segment();
    test_COM_segmentstate();
    test_COM_track();
    test_audiopathconfig();
    test_graph();
    test_segment();

    CoUninitialize();
}
