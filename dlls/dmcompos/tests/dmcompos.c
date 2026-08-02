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
#include <ole2.h>
#include <dmusici.h>
#include <dmusicf.h>

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

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
    hr = CoCreateInstance(&CLSID_DirectMusicComposer, (IUnknown *)0xdeadbeef, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dmc);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicComposer create failed: %#lx, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dmc, "dmc = %p\n", dmc);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicComposer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void**)&dmc);
    ok(hr == E_NOINTERFACE, "DirectMusicComposer create failed: %#lx, expected E_NOINTERFACE\n", hr);

    /* Same refcount for all DirectMusicComposer interfaces */
    hr = CoCreateInstance(&CLSID_DirectMusicComposer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicComposer, (void**)&dmc);
    ok(hr == S_OK, "DirectMusicComposer create failed: %#lx, expected S_OK\n", hr);
    refcount = IDirectMusicComposer_AddRef(dmc);
    ok(refcount == 2, "refcount == %lu, expected 2\n", refcount);

    hr = IDirectMusicComposer_QueryInterface(dmc, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %#lx\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 4, "refcount == %lu, expected 4\n", refcount);
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
    hr = CoCreateInstance(&CLSID_DirectMusicChordMap, (IUnknown *)0xdeadbeef, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dmcm);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicChordMap create failed: %#lx, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dmcm, "dmcm = %p\n", dmcm);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicChordMap, NULL, CLSCTX_INPROC_SERVER,
            &IID_IClassFactory, (void**)&dmcm);
    ok(hr == E_NOINTERFACE, "DirectMusicChordMap create failed: %#lx, expected E_NOINTERFACE\n", hr);

    /* Same refcount for all DirectMusicChordMap interfaces */
    hr = CoCreateInstance(&CLSID_DirectMusicChordMap, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicChordMap, (void**)&dmcm);
    ok(hr == S_OK, "DirectMusicChordMap create failed: %#lx, expected S_OK\n", hr);
    refcount = IDirectMusicChordMap_AddRef(dmcm);
    ok(refcount == 2, "refcount == %lu, expected 2\n", refcount);

    hr = IDirectMusicChordMap_QueryInterface(dmcm, &IID_IDirectMusicObject, (void**)&dmo);
    ok(hr == S_OK, "QueryInterface for IID_IDirectMusicObject failed: %#lx\n", hr);
    refcount = IDirectMusicObject_AddRef(dmo);
    ok(refcount == 4, "refcount == %lu, expected 4\n", refcount);
    refcount = IDirectMusicObject_Release(dmo);

    hr = IDirectMusicChordMap_QueryInterface(dmcm, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %#lx\n", hr);
    refcount = IPersistStream_AddRef(ps);
    ok(refcount == 5, "refcount == %lu, expected 5\n", refcount);
    refcount = IPersistStream_Release(ps);

    hr = IDirectMusicChordMap_QueryInterface(dmcm, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %#lx\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 6, "refcount == %lu, expected 6\n", refcount);
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
    hr = CoCreateInstance(&CLSID_DirectMusicTemplate, (IUnknown *)0xdeadbeef, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&ps);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicTemplate create failed: %#lx, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!ps, "ps = %p\n", ps);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicTemplate, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void**)&ps);
    todo_wine ok(hr == E_NOINTERFACE,
            "DirectMusicTemplate create failed: %#lx, expected E_NOINTERFACE\n", hr);

    /* Same refcount for all DirectMusicTemplate interfaces */
    hr = CoCreateInstance(&CLSID_DirectMusicTemplate, NULL, CLSCTX_INPROC_SERVER,
            &IID_IPersistStream, (void**)&ps);
    todo_wine ok(hr == S_OK, "DirectMusicTemplate create failed: %#lx, expected S_OK\n", hr);
    if (hr != S_OK) {
        skip("DirectMusicTemplate not implemented\n");
        return;
    }
    refcount = IPersistStream_AddRef(ps);
    ok(refcount == 2, "refcount == %lu, expected 2\n", refcount);

    hr = IPersistStream_QueryInterface(ps, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %#lx\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 4, "refcount == %lu, expected 4\n", refcount);
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
        hr = CoCreateInstance(class[i].clsid, (IUnknown *)0xdeadbeef, CLSCTX_INPROC_SERVER, &IID_IUnknown,
                (void**)&dmt8);
        if (hr == REGDB_E_CLASSNOTREG) {
            win_skip("%s not registered\n", class[i].name);
            continue;
        }
        ok(hr == CLASS_E_NOAGGREGATION,
                "%s create failed: %#lx, expected CLASS_E_NOAGGREGATION\n", class[i].name, hr);
        ok(!dmt8, "dmt8 = %p\n", dmt8);

        /* Invalid RIID */
        hr = CoCreateInstance(class[i].clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicObject,
                (void**)&dmt8);
        ok(hr == E_NOINTERFACE, "%s create failed: %#lx, expected E_NOINTERFACE\n", class[i].name, hr);

        /* Same refcount for all DirectMusicTrack interfaces */
        hr = CoCreateInstance(class[i].clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicTrack8,
                (void**)&dmt8);
        if (hr == E_NOINTERFACE && !dmt8) {
            skip("%s not created with CoCreateInstance()\n", class[i].name);
            continue;
        }
        ok(hr == S_OK, "%s create failed: %#lx, expected S_OK\n", class[i].name, hr);
        refcount = IDirectMusicTrack8_AddRef(dmt8);
        ok(refcount == 2, "refcount == %lu, expected 2\n", refcount);

        hr = IDirectMusicTrack8_QueryInterface(dmt8, &IID_IPersistStream, (void**)&ps);
        ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %#lx\n", hr);
        refcount = IPersistStream_AddRef(ps);
        ok(refcount == 4, "refcount == %lu, expected 4\n", refcount);
        IPersistStream_Release(ps);

        hr = IDirectMusicTrack8_QueryInterface(dmt8, &IID_IUnknown, (void**)&unk);
        ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %#lx\n", hr);
        refcount = IUnknown_AddRef(unk);
        ok(refcount == 5, "refcount == %lu, expected 5\n", refcount);
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
    ok(hr == S_OK, "DirectMusicChordMap create failed: %#lx, expected S_OK\n", hr);

    /* IPersistStream */
    hr = IDirectMusicChordMap_QueryInterface(dmcm, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %#lx\n", hr);
    hr = IPersistStream_GetClassID(ps, &class);
    ok(hr == S_OK, "IPersistStream_GetClassID failed: %#lx\n", hr);
    ok(IsEqualGUID(&class, &CLSID_DirectMusicChordMap),
            "Expected class CLSID_DirectMusicChordMap got %s\n", wine_dbgstr_guid(&class));

    /* Unimplemented IPersistStream methods */
    hr = IPersistStream_IsDirty(ps);
    ok(hr == S_FALSE || broken(hr == S_OK), "IPersistStream_IsDirty failed: %#lx\n", hr);
    hr = IPersistStream_GetSizeMax(ps, &size);
    ok(hr == E_NOTIMPL, "IPersistStream_GetSizeMax failed: %#lx\n", hr);
    hr = IPersistStream_Save(ps, NULL, TRUE);
    ok(hr == E_NOTIMPL, "IPersistStream_Save failed: %#lx\n", hr);

    while (IDirectMusicChordMap_Release(dmcm));
}

static void test_chordmaptrack(void)
{
    IDirectMusicTrack8 *dmt8;
    IPersistStream *ps;
    IDirectMusicChordMap *chordmap;
    CLSID class;
    ULARGE_INTEGER size;
    HRESULT hr;
#define X(guid)        &guid, #guid
    const struct {
        REFGUID type;
        const char *name;
    } unsupported[] = {
        { X(GUID_BandParam) },
        { X(GUID_ChordParam) },
        { X(GUID_Clear_All_Bands) },
        { X(GUID_CommandParam) },
        { X(GUID_CommandParam2) },
        { X(GUID_CommandParamNext) },
        { X(GUID_ConnectToDLSCollection) },
        { X(GUID_Disable_Auto_Download) },
        { X(GUID_DisableTempo) },
        { X(GUID_DisableTimeSig) },
        { X(GUID_Download) },
        { X(GUID_DownloadToAudioPath) },
        { X(GUID_Enable_Auto_Download) },
        { X(GUID_EnableTempo) },
        { X(GUID_EnableTimeSig) },
        { X(GUID_IDirectMusicBand) },
        { X(GUID_IDirectMusicStyle) },
        { X(GUID_MuteParam) },
        { X(GUID_Play_Marker) },
        { X(GUID_RhythmParam) },
        { X(GUID_SeedVariations) },
        { X(GUID_StandardMIDIFile) },
        { X(GUID_TempoParam) },
        { X(GUID_TimeSignature) },
        { X(GUID_Unload) },
        { X(GUID_UnloadFromAudioPath) },
        { X(GUID_Valid_Start_Time) },
        { X(GUID_Variations) },
    };
#undef X
    unsigned int i;

    hr = CoCreateInstance(&CLSID_DirectMusicChordMapTrack, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicTrack8, (void**)&dmt8);
    ok(hr == S_OK, "DirectMusicChordMapTrack create failed: %#lx, expected S_OK\n", hr);

    /* IDirectMusicTrack8 */
    hr = IDirectMusicTrack8_Init(dmt8, NULL);
    ok(hr == S_OK, "IDirectMusicTrack8_Init failed: %#lx\n", hr);
    hr = IDirectMusicTrack8_InitPlay(dmt8, NULL, NULL, NULL, 0, 0);
    ok(hr == S_OK, "IDirectMusicTrack8_InitPlay failed: %#lx\n", hr);
    hr = IDirectMusicTrack8_EndPlay(dmt8, NULL);
    ok(hr == S_OK, "IDirectMusicTrack8_EndPlay failed: %#lx\n", hr);
    hr = IDirectMusicTrack8_Play(dmt8, NULL, 0, 0, 0, 0, NULL, NULL, 0);
    ok(hr == S_OK, "IDirectMusicTrack8_Play failed: %#lx\n", hr);
    hr = IDirectMusicTrack8_GetParam(dmt8, NULL, 0, NULL, NULL);
    ok(hr == E_POINTER, "IDirectMusicTrack8_GetParam failed: %#lx\n", hr);
    hr = IDirectMusicTrack8_SetParam(dmt8, NULL, 0, NULL);
    ok(hr == E_POINTER, "IDirectMusicTrack8_SetParam failed: %#lx\n", hr);

    hr = IDirectMusicTrack8_IsParamSupported(dmt8, NULL);
    ok(hr == E_POINTER, "IDirectMusicTrack8_IsParamSupported failed: %#lx\n", hr);
    hr = IDirectMusicTrack8_IsParamSupported(dmt8, &GUID_IDirectMusicChordMap);
    ok(hr == S_OK, "IsParamSupported(GUID_IDirectMusicChordMap) failed: %#lx, expected S_OK\n", hr);
    hr = IDirectMusicTrack8_GetParam(dmt8, &GUID_IDirectMusicChordMap, 0, NULL, &chordmap);
    todo_wine ok(hr == DMUS_E_NOT_FOUND,
            "GetParam(GUID_IDirectMusicChordMap) failed: %#lx, expected DMUS_E_NOT_FOUND\n", hr);
    hr = CoCreateInstance(&CLSID_DirectMusicChordMap, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicChordMap, (void **)&chordmap);
    ok(hr == S_OK, "DirectMusicChordMap create failed: %#lx, expected S_OK\n", hr);
    hr = IDirectMusicTrack8_SetParam(dmt8, &GUID_IDirectMusicChordMap, 0, chordmap);
    ok(hr == S_OK, "SetParam(GUID_IDirectMusicChordMap) failed: %#lx, expected S_OK\n", hr);
    IDirectMusicChordMap_Release(chordmap);

    for (i = 0; i < ARRAY_SIZE(unsupported); i++) {
        hr = IDirectMusicTrack8_IsParamSupported(dmt8, unsupported[i].type);
        ok(hr == DMUS_E_TYPE_UNSUPPORTED,
                "IsParamSupported(%s) failed: %#lx, expected DMUS_E_TYPE_UNSUPPORTED\n",
                    unsupported[i].name, hr);
        hr = IDirectMusicTrack8_GetParam(dmt8, unsupported[i].type, 0, NULL, &chordmap);
        ok(hr == DMUS_E_GET_UNSUPPORTED,
                "GetParam(%s) failed: %#lx, expected DMUS_E_GET_UNSUPPORTED\n",
                unsupported[i].name, hr);
        hr = IDirectMusicTrack8_SetParam(dmt8, unsupported[i].type, 0, chordmap);
        ok(hr == DMUS_E_SET_UNSUPPORTED,
                "SetParam(%s) failed: %#lx, expected DMUS_E_SET_UNSUPPORTED\n",
                unsupported[i].name, hr);
    }

    hr = IDirectMusicTrack8_AddNotificationType(dmt8, NULL);
    ok(hr == E_NOTIMPL, "IDirectMusicTrack8_AddNotificationType failed: %#lx\n", hr);
    hr = IDirectMusicTrack8_RemoveNotificationType(dmt8, NULL);
    ok(hr == E_NOTIMPL, "IDirectMusicTrack8_RemoveNotificationType failed: %#lx\n", hr);
    todo_wine {
    hr = IDirectMusicTrack8_Clone(dmt8, 0, 0, NULL);
    ok(hr == E_POINTER, "IDirectMusicTrack8_Clone failed: %#lx\n", hr);
    hr = IDirectMusicTrack8_PlayEx(dmt8, NULL, 0, 0, 0, 0, NULL, NULL, 0);
    ok(hr == E_POINTER, "IDirectMusicTrack8_PlayEx failed: %#lx\n", hr);
    hr = IDirectMusicTrack8_GetParamEx(dmt8, NULL, 0, NULL, NULL, NULL, 0);
    ok(hr == E_POINTER, "IDirectMusicTrack8_GetParamEx failed: %#lx\n", hr);
    hr = IDirectMusicTrack8_SetParamEx(dmt8, NULL, 0, NULL, NULL, 0);
    ok(hr == E_POINTER, "IDirectMusicTrack8_SetParamEx failed: %#lx\n", hr);
    }
    hr = IDirectMusicTrack8_Compose(dmt8, NULL, 0, NULL);
    ok(hr == E_NOTIMPL, "IDirectMusicTrack8_Compose failed: %#lx\n", hr);
    hr = IDirectMusicTrack8_Join(dmt8, NULL, 0, NULL, 0, NULL);
    todo_wine ok(hr == E_POINTER, "IDirectMusicTrack8_Join failed: %#lx\n", hr);

    /* IPersistStream */
    hr = IDirectMusicTrack8_QueryInterface(dmt8, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %#lx\n", hr);
    hr = IPersistStream_GetClassID(ps, &class);
    ok(hr == S_OK, "IPersistStream_GetClassID failed: %#lx\n", hr);
    ok(IsEqualGUID(&class, &CLSID_DirectMusicChordMapTrack),
            "Expected class CLSID_DirectMusicChordMapTrack got %s\n", wine_dbgstr_guid(&class));

    /* Unimplemented IPersistStream methods */
    hr = IPersistStream_IsDirty(ps);
    ok(hr == S_FALSE, "IPersistStream_IsDirty failed: %#lx\n", hr);
    hr = IPersistStream_GetSizeMax(ps, &size);
    ok(hr == E_NOTIMPL, "IPersistStream_GetSizeMax failed: %#lx\n", hr);
    hr = IPersistStream_Save(ps, NULL, TRUE);
    ok(hr == E_NOTIMPL, "IPersistStream_Save failed: %#lx\n", hr);

    while (IDirectMusicTrack8_Release(dmt8));
}

static void test_signposttrack(void)
{
    IDirectMusicTrack8 *dmt8;
    IPersistStream *ps;
    CLSID class;
    ULARGE_INTEGER size;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectMusicSignPostTrack, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicTrack8, (void**)&dmt8);
    ok(hr == S_OK, "DirectMusicSignPostTrack create failed: %#lx, expected S_OK\n", hr);

    /* IDirectMusicTrack8 */
    hr = IDirectMusicTrack8_Init(dmt8, NULL);
    ok(hr == S_OK, "IDirectMusicTrack8_Init failed: %#lx\n", hr);
    if (0) {
        /* Crashes on Windows */
        hr = IDirectMusicTrack8_InitPlay(dmt8, NULL, NULL, NULL, 0, 0);
        ok(hr == E_POINTER, "IDirectMusicTrack8_InitPlay failed: %#lx\n", hr);
    }
    hr = IDirectMusicTrack8_EndPlay(dmt8, NULL);
    ok(hr == S_OK, "IDirectMusicTrack8_EndPlay failed: %#lx\n", hr);
    hr = IDirectMusicTrack8_Play(dmt8, NULL, 0, 0, 0, 0, NULL, NULL, 0);
    ok(hr == S_OK, "IDirectMusicTrack8_Play failed: %#lx\n", hr);
    hr = IDirectMusicTrack8_GetParam(dmt8, NULL, 0, NULL, NULL);
    ok(hr == E_NOTIMPL, "IDirectMusicTrack8_GetParam failed: %#lx\n", hr);
    hr = IDirectMusicTrack8_SetParam(dmt8, NULL, 0, NULL);
    ok(hr == E_NOTIMPL, "IDirectMusicTrack8_SetParam failed: %#lx\n", hr);
    hr = IDirectMusicTrack8_IsParamSupported(dmt8, NULL);
    ok(hr == E_NOTIMPL, "IDirectMusicTrack8_IsParamSupported failed: %#lx\n", hr);
    todo_wine {
    hr = IDirectMusicTrack8_AddNotificationType(dmt8, NULL);
    ok(hr == E_POINTER, "IDirectMusicTrack8_AddNotificationType failed: %#lx\n", hr);
    hr = IDirectMusicTrack8_RemoveNotificationType(dmt8, NULL);
    ok(hr == E_POINTER, "IDirectMusicTrack8_RemoveNotificationType failed: %#lx\n", hr);
    hr = IDirectMusicTrack8_Clone(dmt8, 0, 0, NULL);
    ok(hr == E_POINTER, "IDirectMusicTrack8_Clone failed: %#lx\n", hr);
    hr = IDirectMusicTrack8_PlayEx(dmt8, NULL, 0, 0, 0, 0, NULL, NULL, 0);
    ok(hr == E_POINTER, "IDirectMusicTrack8_PlayEx failed: %#lx\n", hr);
    }
    hr = IDirectMusicTrack8_GetParamEx(dmt8, NULL, 0, NULL, NULL, NULL, 0);
    ok(hr == E_NOTIMPL, "IDirectMusicTrack8_GetParamEx failed: %#lx\n", hr);
    hr = IDirectMusicTrack8_SetParamEx(dmt8, NULL, 0, NULL, NULL, 0);
    ok(hr == E_NOTIMPL, "IDirectMusicTrack8_SetParamEx failed: %#lx\n", hr);
    todo_wine {
    hr = IDirectMusicTrack8_Compose(dmt8, NULL, 0, NULL);
    ok(hr == E_POINTER, "IDirectMusicTrack8_Compose failed: %#lx\n", hr);
    hr = IDirectMusicTrack8_Join(dmt8, NULL, 0, NULL, 0, NULL);
    ok(hr == E_POINTER, "IDirectMusicTrack8_Join failed: %#lx\n", hr);
    }

    /* IPersistStream */
    hr = IDirectMusicTrack8_QueryInterface(dmt8, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %#lx\n", hr);
    hr = IPersistStream_GetClassID(ps, &class);
    ok(hr == S_OK, "IPersistStream_GetClassID failed: %#lx\n", hr);
    ok(IsEqualGUID(&class, &CLSID_DirectMusicSignPostTrack),
            "Expected class CLSID_DirectMusicSignPostTrack got %s\n", wine_dbgstr_guid(&class));
    hr = IPersistStream_Save(ps, NULL, TRUE);
    ok(hr == E_POINTER, "IPersistStream_Save failed: %#lx\n", hr);

    /* Unimplemented IPersistStream methods */
    hr = IPersistStream_IsDirty(ps);
    ok(hr == S_FALSE, "IPersistStream_IsDirty failed: %#lx\n", hr);
    hr = IPersistStream_GetSizeMax(ps, &size);
    ok(hr == E_NOTIMPL, "IPersistStream_GetSizeMax failed: %#lx\n", hr);

    while (IDirectMusicTrack8_Release(dmt8));
}

struct chunk {
    FOURCC id;
    DWORD size;
    FOURCC type;
};

#define CHUNK_HDR_SIZE (sizeof(FOURCC) + sizeof(DWORD))

/* Generate a RIFF file format stream from an array of FOURCC ids.
   RIFF and LIST need to be followed by the form type respectively list type,
   followed by the chunks of the list and terminated with 0. */
static IStream *gen_riff_stream(const FOURCC *ids)
{
    static const LARGE_INTEGER zero;
    int level = -1;
    DWORD *sizes[4];    /* Stack for the sizes of RIFF and LIST chunks */
    char riff[1024];
    char *p = riff;
    struct chunk *ck;
    IStream *stream;

    do {
        ck = (struct chunk *)p;
        ck->id = *ids++;
        switch (ck->id) {
            case 0:
                *sizes[level] = p - (char *)sizes[level] - sizeof(DWORD);
                level--;
                break;
            case FOURCC_LIST:
            case FOURCC_RIFF:
                level++;
                sizes[level] = &ck->size;
                ck->type = *ids++;
                p += sizeof(*ck);
                break;
            case DMUS_FOURCC_GUID_CHUNK:
                ck->size = sizeof(GUID_NULL);
                p += CHUNK_HDR_SIZE;
                memcpy(p, &GUID_NULL, sizeof(GUID_NULL));
                p += ck->size;
                break;
            case DMUS_FOURCC_VERSION_CHUNK:
            {
                DMUS_VERSION ver = {5, 8};

                ck->size = sizeof(ver);
                p += CHUNK_HDR_SIZE;
                memcpy(p, &ver, sizeof(ver));
                p += ck->size;
                break;
            }
            default:
            {
                /* Just convert the FOURCC id to a WCHAR string */
                WCHAR *s;

                ck->size = 5 * sizeof(WCHAR);
                p += CHUNK_HDR_SIZE;
                s = (WCHAR *)p;
                s[0] = (char)(ck->id);
                s[1] = (char)(ck->id >> 8);
                s[2] = (char)(ck->id >> 16);
                s[3] = (char)(ck->id >> 24);
                s[4] = 0;
                p += ck->size;
            }
        }
    } while (level >= 0);

    ck = (struct chunk *)riff;
    CreateStreamOnHGlobal(NULL, TRUE, &stream);
    IStream_Write(stream, riff, ck->size + CHUNK_HDR_SIZE, NULL);
    IStream_Seek(stream, zero, STREAM_SEEK_SET, NULL);

    return stream;
}

static void test_parsedescriptor(void)
{
    IDirectMusicObject *dmo;
    IStream *stream;
    DMUS_OBJECTDESC desc = {0};
    HRESULT hr;
    const FOURCC alldesc[] =
    {
        FOURCC_RIFF, DMUS_FOURCC_CHORDMAP_FORM, DMUS_FOURCC_CATEGORY_CHUNK, FOURCC_LIST,
        DMUS_FOURCC_UNFO_LIST, DMUS_FOURCC_UNAM_CHUNK, DMUS_FOURCC_UCOP_CHUNK,
        DMUS_FOURCC_UCMT_CHUNK, DMUS_FOURCC_USBJ_CHUNK, 0, DMUS_FOURCC_VERSION_CHUNK,
        DMUS_FOURCC_GUID_CHUNK, 0
    };
    const FOURCC dupes[] =
    {
        FOURCC_RIFF, DMUS_FOURCC_CHORDMAP_FORM, DMUS_FOURCC_CATEGORY_CHUNK, DMUS_FOURCC_CATEGORY_CHUNK,
        DMUS_FOURCC_VERSION_CHUNK, DMUS_FOURCC_VERSION_CHUNK, DMUS_FOURCC_GUID_CHUNK,
        DMUS_FOURCC_GUID_CHUNK, FOURCC_LIST, DMUS_FOURCC_UNFO_LIST, DMUS_FOURCC_UNAM_CHUNK, 0,
        FOURCC_LIST, DMUS_FOURCC_UNFO_LIST, mmioFOURCC('I','N','A','M'), 0, 0
    };
    FOURCC empty[] = {FOURCC_RIFF, DMUS_FOURCC_CHORDMAP_FORM, 0};
    FOURCC inam[] =
    {
        FOURCC_RIFF, DMUS_FOURCC_CHORDMAP_FORM, FOURCC_LIST, DMUS_FOURCC_UNFO_LIST,
        mmioFOURCC('I','N','A','M'), 0, 0
    };

    hr = CoCreateInstance(&CLSID_DirectMusicChordMap, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void **)&dmo);
    ok(hr == S_OK, "DirectMusicChordMap create failed: %#lx, expected S_OK\n", hr);

    /* Nothing loaded */
    hr = IDirectMusicObject_GetDescriptor(dmo, &desc);
    ok(hr == S_OK, "GetDescriptor failed: %#lx, expected S_OK\n", hr);
    ok(desc.dwValidData == DMUS_OBJ_CLASS, "Got valid data %#lx, expected DMUS_OBJ_OBJECT\n",
            desc.dwValidData);
    ok(IsEqualGUID(&desc.guidClass, &CLSID_DirectMusicChordMap),
            "Got class guid %s, expected CLSID_DirectMusicChordMap\n",
            wine_dbgstr_guid(&desc.guidClass));

    /* Empty RIFF stream */
    stream = gen_riff_stream(empty);
    memset(&desc, 0, sizeof(desc));
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
    ok(hr == S_OK, "ParseDescriptor failed: %#lx, expected S_OK\n", hr);
    ok(desc.dwValidData == DMUS_OBJ_CLASS, "Got valid data %#lx, expected DMUS_OBJ_CLASS\n",
            desc.dwValidData);
    ok(IsEqualGUID(&desc.guidClass, &CLSID_DirectMusicChordMap),
            "Got class guid %s, expected CLSID_DirectMusicChordMap\n",
            wine_dbgstr_guid(&desc.guidClass));
    IStream_Release(stream);

    /* NULL pointers */
    memset(&desc, 0, sizeof(desc));
    hr = IDirectMusicObject_ParseDescriptor(dmo, NULL, &desc);
    ok(hr == E_POINTER, "ParseDescriptor failed: %#lx, expected E_POINTER\n", hr);
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, NULL);
    ok(hr == E_POINTER, "ParseDescriptor failed: %#lx, expected E_POINTER\n", hr);

    /* Wrong form */
    empty[1] = DMUS_FOURCC_CONTAINER_FORM;
    stream = gen_riff_stream(empty);
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
    ok(hr == DMUS_E_CHUNKNOTFOUND,
            "ParseDescriptor failed: %#lx, expected DMUS_E_CHUNKNOTFOUND\n", hr);
    IStream_Release(stream);

    /* All desc chunks, only DMUS_OBJ_OBJECT and DMUS_OBJ_CLASS supported */
    stream = gen_riff_stream(alldesc);
    memset(&desc, 0, sizeof(desc));
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
    ok(hr == S_OK, "ParseDescriptor failed: %#lx, expected S_OK\n", hr);
    ok(desc.dwValidData == (DMUS_OBJ_OBJECT | DMUS_OBJ_CLASS),
            "Got valid data %#lx, expected DMUS_OBJ_OBJECT | DMUS_OBJ_CLASS\n", desc.dwValidData);
    ok(IsEqualGUID(&desc.guidClass, &CLSID_DirectMusicChordMap),
            "Got class guid %s, expected CLSID_DirectMusicChordMap\n",
            wine_dbgstr_guid(&desc.guidClass));
    ok(IsEqualGUID(&desc.guidObject, &GUID_NULL), "Got object guid %s, expected GUID_NULL\n",
            wine_dbgstr_guid(&desc.guidClass));
    IStream_Release(stream);

    /* UNFO list with INAM */
    inam[3] = DMUS_FOURCC_UNFO_LIST;
    stream = gen_riff_stream(inam);
    memset(&desc, 0, sizeof(desc));
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
    ok(hr == S_OK, "ParseDescriptor failed: %#lx, expected S_OK\n", hr);
    ok(desc.dwValidData == DMUS_OBJ_CLASS, "Got valid data %#lx, expected DMUS_OBJ_CLASS\n",
            desc.dwValidData);
    IStream_Release(stream);

    /* INFO list with INAM */
    inam[3] = DMUS_FOURCC_INFO_LIST;
    stream = gen_riff_stream(inam);
    memset(&desc, 0, sizeof(desc));
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
    ok(hr == S_OK, "ParseDescriptor failed: %#lx, expected S_OK\n", hr);
    ok(desc.dwValidData == DMUS_OBJ_CLASS, "Got valid data %#lx, expected DMUS_OBJ_CLASS\n",
            desc.dwValidData);
    IStream_Release(stream);

    /* Duplicated chunks */
    stream = gen_riff_stream(dupes);
    memset(&desc, 0, sizeof(desc));
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
    ok(hr == S_OK, "ParseDescriptor failed: %#lx, expected S_OK\n", hr);
    ok(desc.dwValidData == (DMUS_OBJ_OBJECT | DMUS_OBJ_CLASS),
            "Got valid data %#lx, expected DMUS_OBJ_OBJECT | DMUS_OBJ_CLASS\n", desc.dwValidData);
    IStream_Release(stream);

    IDirectMusicObject_Release(dmo);
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
    test_chordmaptrack();
    test_signposttrack();
    test_parsedescriptor();

    CoUninitialize();
}
