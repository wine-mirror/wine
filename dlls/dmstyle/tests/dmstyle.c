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
    hr = CoCreateInstance(&CLSID_DirectMusicStyle, (IUnknown *)0xdeadbeef, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dms8);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicStyle8 create failed: %#lx, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dms8, "dms8 = %p\n", dms8);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicStyle, NULL, CLSCTX_INPROC_SERVER, &IID_IClassFactory,
            (void**)&dms8);
    ok(hr == E_NOINTERFACE, "DirectMusicStyle8 create failed: %#lx, expected E_NOINTERFACE\n", hr);

    /* Same refcount for all DirectMusicStyle8 interfaces */
    hr = CoCreateInstance(&CLSID_DirectMusicStyle, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicStyle8, (void**)&dms8);
    if (hr == E_NOINTERFACE) {
        win_skip("Old version without IDirectMusicStyle8\n");
        return;
    }
    ok(hr == S_OK, "DirectMusicStyle8 create failed: %#lx, expected S_OK\n", hr);
    refcount = IDirectMusicStyle8_AddRef(dms8);
    ok(refcount == 2, "refcount == %lu, expected 2\n", refcount);

    hr = IDirectMusicStyle8_QueryInterface(dms8, &IID_IDirectMusicObject, (void**)&dmo);
    ok(hr == S_OK, "QueryInterface for IID_IDirectMusicObject failed: %#lx\n", hr);
    refcount = IDirectMusicObject_AddRef(dmo);
    ok(refcount == 4, "refcount == %lu, expected 4\n", refcount);
    refcount = IDirectMusicObject_Release(dmo);

    hr = IDirectMusicStyle8_QueryInterface(dms8, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %#lx\n", hr);
    refcount = IPersistStream_AddRef(ps);
    ok(refcount == 5, "refcount == %lu, expected 5\n", refcount);
    refcount = IPersistStream_Release(ps);

    hr = IDirectMusicStyle8_QueryInterface(dms8, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %#lx\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 6, "refcount == %lu, expected 6\n", refcount);
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
    hr = CoCreateInstance(&CLSID_DirectMusicSection, (IUnknown *)0xdeadbeef, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dmo);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicSection create failed: %#lx, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dmo, "dmo = %p\n", dmo);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicSection, NULL, CLSCTX_INPROC_SERVER, &IID_IClassFactory,
            (void**)&dmo);
    todo_wine ok(hr == E_NOINTERFACE,
            "DirectMusicSection create failed: %#lx, expected E_NOINTERFACE\n", hr);

    /* Same refcount for all DirectMusicObject interfaces */
    hr = CoCreateInstance(&CLSID_DirectMusicSection, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void**)&dmo);
    todo_wine ok(hr == S_OK, "DirectMusicSection create failed: %#lx, expected S_OK\n", hr);
    if (hr != S_OK) {
        skip("DirectMusicSection not implemented\n");
        return;
    }
    refcount = IDirectMusicObject_AddRef(dmo);
    ok(refcount == 2, "refcount == %lu, expected 2\n", refcount);

    hr = IDirectMusicObject_QueryInterface(dmo, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %#lx\n", hr);
    refcount = IPersistStream_AddRef(ps);
    ok(refcount == 4, "refcount == %lu, expected 4\n", refcount);
    IPersistStream_Release(ps);

    hr = IDirectMusicObject_QueryInterface(dmo, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %#lx\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 5, "refcount == %lu, expected 5\n", refcount);
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
        ok(hr == E_NOINTERFACE, "%s create failed: %#lx, expected E_NOINTERFACE\n",
                class[i].name, hr);

        /* Same refcount for all DirectMusicTrack interfaces */
        hr = CoCreateInstance(class[i].clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicTrack8,
                (void**)&dmt8);
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

static void test_dmstyle(void)
{
    IDirectMusicStyle *dms;
    IPersistStream *ps;
    CLSID class = { 0 };
    ULARGE_INTEGER size;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectMusicStyle, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicStyle, (void**)&dms);
    ok(hr == S_OK, "DirectMusicStyle create failed: %#lx, expected S_OK\n", hr);

    /* IPersistStream */
    hr = IDirectMusicStyle_QueryInterface(dms, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %#lx\n", hr);
    hr = IPersistStream_GetClassID(ps, &class);
    ok(hr == S_OK, "IPersistStream_GetClassID failed: %#lx\n", hr);
    ok(IsEqualGUID(&class, &CLSID_DirectMusicStyle),
            "Expected class CLSID_DirectMusicStyle got %s\n", wine_dbgstr_guid(&class));

    /* Unimplemented IPersistStream methods*/
    hr = IPersistStream_IsDirty(ps);
    ok(hr == S_FALSE, "IPersistStream_IsDirty failed: %#lx\n", hr);
    hr = IPersistStream_GetSizeMax(ps, &size);
    ok(hr == E_NOTIMPL, "IPersistStream_GetSizeMax failed: %#lx\n", hr);
    hr = IPersistStream_Save(ps, NULL, TRUE);
    ok(hr == E_NOTIMPL, "IPersistStream_Save failed: %#lx\n", hr);

    while (IDirectMusicStyle_Release(dms));
}

static void expect_getparam(IDirectMusicTrack8 *track, REFGUID type, const char *name,
        HRESULT expect)
{
    HRESULT hr;
    char buf[64] = { 0 };

    hr = IDirectMusicTrack8_GetParam(track, type, 0, NULL, buf);
    ok(hr == expect, "GetParam(%s) failed: %#lx, expected %#lx\n", name, hr, expect);
}

static void expect_setparam(IDirectMusicTrack8 *track, REFGUID type, const char *name,
        HRESULT expect)
{
    HRESULT hr;

    hr = IDirectMusicTrack8_SetParam(track, type, 0, track);
    ok(hr == expect, "SetParam(%s) failed: %#lx, expected %#lx\n", name, hr, expect);
}

static void test_track(void)
{
    IDirectMusicTrack8 *dmt8;
    IPersistStream *ps;
    CLSID classid;
    ULARGE_INTEGER size;
    HRESULT hr;
#define X(guid)        &guid, #guid
    const struct {
        REFGUID type;
        const char *name;
    } param_types[] = {
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
        { X(GUID_IDirectMusicChordMap) },
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
        { X(GUID_Variations) }
    };
#undef X
#define X(class)        &CLSID_ ## class, #class
    const struct {
        REFCLSID clsid;
        const char *name;
        BOOL has_save;
        BOOL has_join;
        /* bitfield with supported param types */
        unsigned int has_params;
    } class[] = {
        { X(DirectMusicAuditionTrack), TRUE, FALSE, 0x18204200 },
        { X(DirectMusicChordTrack), TRUE, TRUE, 0x100002 },
        { X(DirectMusicCommandTrack), TRUE, TRUE, 0x38 },
        { X(DirectMusicMotifTrack), FALSE, FALSE, 0x8204200 },
        { X(DirectMusicMuteTrack), TRUE, FALSE, 0x40000 },
        { X(DirectMusicStyleTrack), FALSE, TRUE, 0x1224200 },
    };
#undef X
    unsigned int i, j;

    for (i = 0; i < ARRAY_SIZE(class); i++) {
        trace("Testing %s\n", class[i].name);
        hr = CoCreateInstance(class[i].clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicTrack8,
                (void**)&dmt8);
        ok(hr == S_OK, "%s create failed: %#lx, expected S_OK\n", class[i].name, hr);

        /* IDirectMusicTrack8 */
        hr = IDirectMusicTrack8_Init(dmt8, NULL);
        todo_wine ok(hr == E_POINTER, "IDirectMusicTrack8_Init failed: %#lx\n", hr);
        if (class[i].clsid != &CLSID_DirectMusicChordTrack &&
                class[i].clsid != &CLSID_DirectMusicCommandTrack) {
            /* Crashes on native */
            hr = IDirectMusicTrack8_InitPlay(dmt8, NULL, NULL, NULL, 0, 0);
            if (class[i].clsid == &CLSID_DirectMusicMuteTrack)
                ok(hr == S_OK, "IDirectMusicTrack8_InitPlay failed: %#lx\n", hr);
            else
                todo_wine ok(hr == E_POINTER, "IDirectMusicTrack8_InitPlay failed: %#lx\n", hr);
        }

        hr = IDirectMusicTrack8_GetParam(dmt8, NULL, 0, NULL, NULL);
        ok(hr == E_POINTER, "IDirectMusicTrack8_GetParam failed: %#lx\n", hr);
        hr = IDirectMusicTrack8_SetParam(dmt8, NULL, 0, NULL);
        ok(hr == E_POINTER, "IDirectMusicTrack8_SetParam failed: %#lx\n", hr);

        hr = IDirectMusicTrack8_IsParamSupported(dmt8, NULL);
        ok(hr == E_POINTER, "IDirectMusicTrack8_IsParamSupported failed: %#lx\n", hr);
        for (j = 0; j < ARRAY_SIZE(param_types); j++) {
            hr = IDirectMusicTrack8_IsParamSupported(dmt8, param_types[j].type);
            if (class[i].has_params & (1 << j))
                ok(hr == S_OK, "IsParamSupported(%s) failed: %#lx, expected S_OK\n",
                        param_types[j].name, hr);
            else {
                ok(hr == DMUS_E_TYPE_UNSUPPORTED,
                        "IsParamSupported(%s) failed: %#lx, expected DMUS_E_TYPE_UNSUPPORTED\n",
                        param_types[j].name, hr);
                if (class[i].clsid == &CLSID_DirectMusicChordTrack) {
                    expect_setparam(dmt8, param_types[j].type, param_types[j].name,
                            DMUS_E_SET_UNSUPPORTED);
                } else if (class[i].clsid == &CLSID_DirectMusicMuteTrack) {
                    expect_getparam(dmt8, param_types[j].type, param_types[j].name,
                            DMUS_E_TYPE_UNSUPPORTED);
                    expect_setparam(dmt8, param_types[j].type, param_types[j].name,
                            DMUS_E_TYPE_UNSUPPORTED);
                } else {
                    expect_getparam(dmt8, param_types[j].type, param_types[j].name,
                            DMUS_E_GET_UNSUPPORTED);
                    expect_setparam(dmt8, param_types[j].type, param_types[j].name,
                            DMUS_E_SET_UNSUPPORTED);
                }
            }
        }

        /* GetParam / SetParam for IsParamSupported supported types */
        if (class[i].clsid == &CLSID_DirectMusicAuditionTrack) {
            expect_getparam(dmt8, &GUID_DisableTimeSig, "GUID_DisableTimeSig",
                    DMUS_E_GET_UNSUPPORTED);
            expect_getparam(dmt8, &GUID_EnableTimeSig, "GUID_EnableTimeSig",
                    DMUS_E_GET_UNSUPPORTED);
            expect_getparam(dmt8, &GUID_SeedVariations, "GUID_SeedVariations",
                    DMUS_E_GET_UNSUPPORTED);
            expect_setparam(dmt8, &GUID_Valid_Start_Time, "GUID_Valid_Start_Time",
                    DMUS_E_SET_UNSUPPORTED);
            expect_setparam(dmt8, &GUID_Variations, "GUID_Variations",
                    DMUS_E_SET_UNSUPPORTED);
        } else if (class[i].clsid == &CLSID_DirectMusicChordTrack) {
            expect_setparam(dmt8, &GUID_RhythmParam, "GUID_RhythmParam",
                    DMUS_E_SET_UNSUPPORTED);
        } else if (class[i].clsid == &CLSID_DirectMusicCommandTrack) {
            expect_setparam(dmt8, &GUID_CommandParam2, "GUID_CommandParam2",
                    DMUS_E_SET_UNSUPPORTED);
        } else if (class[i].clsid == &CLSID_DirectMusicMotifTrack) {
            expect_getparam(dmt8, &GUID_DisableTimeSig, "GUID_DisableTimeSig",
                    DMUS_E_GET_UNSUPPORTED);
            expect_getparam(dmt8, &GUID_EnableTimeSig, "GUID_EnableTimeSig",
                    DMUS_E_GET_UNSUPPORTED);
            expect_getparam(dmt8, &GUID_SeedVariations, "GUID_SeedVariations",
                    DMUS_E_GET_UNSUPPORTED);
            expect_setparam(dmt8, &GUID_Valid_Start_Time, "GUID_Valid_Start_Time",
                    DMUS_E_SET_UNSUPPORTED);
        } else if (class[i].clsid == &CLSID_DirectMusicStyleTrack) {
            expect_getparam(dmt8, &GUID_DisableTimeSig, "GUID_DisableTimeSig",
                    DMUS_E_GET_UNSUPPORTED);
            expect_getparam(dmt8, &GUID_EnableTimeSig, "GUID_EnableTimeSig",
                    DMUS_E_GET_UNSUPPORTED);
            expect_getparam(dmt8, &GUID_SeedVariations, "GUID_SeedVariations",
                    DMUS_E_GET_UNSUPPORTED);
            expect_setparam(dmt8, &GUID_TimeSignature, "GUID_TimeSignature",
                    DMUS_E_SET_UNSUPPORTED);
        }

        if (class[i].clsid == &CLSID_DirectMusicMuteTrack) {
            hr = IDirectMusicTrack8_AddNotificationType(dmt8, NULL);
            ok(hr == E_NOTIMPL, "IDirectMusicTrack8_AddNotificationType failed: %#lx\n", hr);
            hr = IDirectMusicTrack8_RemoveNotificationType(dmt8, NULL);
            ok(hr == E_NOTIMPL, "IDirectMusicTrack8_RemoveNotificationType failed: %#lx\n", hr);
        } else todo_wine {
            hr = IDirectMusicTrack8_AddNotificationType(dmt8, NULL);
            ok(hr == E_POINTER, "IDirectMusicTrack8_AddNotificationType failed: %#lx\n", hr);
            hr = IDirectMusicTrack8_RemoveNotificationType(dmt8, NULL);
            ok(hr == E_POINTER, "IDirectMusicTrack8_RemoveNotificationType failed: %#lx\n", hr);
        }
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
        if (class[i].has_join) {
            hr = IDirectMusicTrack8_Join(dmt8, NULL, 0, NULL, 0, NULL);
            todo_wine ok(hr == E_POINTER, "IDirectMusicTrack8_Join failed: %#lx\n", hr);
        } else {
            hr = IDirectMusicTrack8_Join(dmt8, NULL, 0, NULL, 0, NULL);
            ok(hr == E_NOTIMPL, "IDirectMusicTrack8_Join failed: %#lx\n", hr);
        }

        /* IPersistStream */
        hr = IDirectMusicTrack8_QueryInterface(dmt8, &IID_IPersistStream, (void**)&ps);
        ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %#lx\n", hr);
        hr = IPersistStream_GetClassID(ps, &classid);
        ok(hr == S_OK, "IPersistStream_GetClassID failed: %#lx\n", hr);
        ok(IsEqualGUID(&classid, class[i].clsid),
                "Expected class %s got %s\n", class[i].name, wine_dbgstr_guid(&classid));
        hr = IPersistStream_IsDirty(ps);
        ok(hr == S_FALSE, "IPersistStream_IsDirty failed: %#lx\n", hr);

        hr = IPersistStream_GetSizeMax(ps, &size);
        ok(hr == E_NOTIMPL, "IPersistStream_GetSizeMax failed: %#lx\n", hr);

        hr = IPersistStream_Save(ps, NULL, TRUE);
        if (class[i].has_save)
            ok(hr == E_POINTER, "IPersistStream_Save failed: %#lx\n", hr);
        else
            ok(hr == E_NOTIMPL, "IPersistStream_Save failed: %#lx\n", hr);

        while (IDirectMusicTrack8_Release(dmt8));
    }
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
    DWORD valid;
    const FOURCC alldesc[] =
    {
        FOURCC_RIFF, DMUS_FOURCC_STYLE_FORM, DMUS_FOURCC_CATEGORY_CHUNK, FOURCC_LIST,
        DMUS_FOURCC_UNFO_LIST, DMUS_FOURCC_UNAM_CHUNK, DMUS_FOURCC_UCOP_CHUNK,
        DMUS_FOURCC_UCMT_CHUNK, DMUS_FOURCC_USBJ_CHUNK, 0, DMUS_FOURCC_VERSION_CHUNK,
        DMUS_FOURCC_GUID_CHUNK, 0
    };
    const FOURCC dupes[] =
    {
        FOURCC_RIFF, DMUS_FOURCC_STYLE_FORM, DMUS_FOURCC_CATEGORY_CHUNK, DMUS_FOURCC_CATEGORY_CHUNK,
        DMUS_FOURCC_VERSION_CHUNK, DMUS_FOURCC_VERSION_CHUNK, DMUS_FOURCC_GUID_CHUNK,
        DMUS_FOURCC_GUID_CHUNK, FOURCC_LIST, DMUS_FOURCC_UNFO_LIST, DMUS_FOURCC_UNAM_CHUNK, 0,
        FOURCC_LIST, DMUS_FOURCC_UNFO_LIST, mmioFOURCC('I','N','A','M'), 0, 0
    };
    FOURCC empty[] = {FOURCC_RIFF, DMUS_FOURCC_STYLE_FORM, 0};
    FOURCC inam[] =
    {
        FOURCC_RIFF, DMUS_FOURCC_STYLE_FORM, FOURCC_LIST, DMUS_FOURCC_UNFO_LIST,
        mmioFOURCC('I','N','A','M'), 0, 0
    };

    hr = CoCreateInstance(&CLSID_DirectMusicStyle, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void **)&dmo);
    ok(hr == S_OK, "DirectMusicStyle create failed: %#lx, expected S_OK\n", hr);

    /* Nothing loaded */
    hr = IDirectMusicObject_GetDescriptor(dmo, &desc);
    ok(hr == S_OK, "GetDescriptor failed: %#lx, expected S_OK\n", hr);
    ok(desc.dwValidData & DMUS_OBJ_CLASS, "Got valid data %#lx, expected DMUS_OBJ_CLASS\n",
            desc.dwValidData);
    ok(IsEqualGUID(&desc.guidClass, &CLSID_DirectMusicStyle),
            "Got class guid %s, expected CLSID_DirectMusicStyle\n",
            wine_dbgstr_guid(&desc.guidClass));

    /* Empty RIFF stream */
    stream = gen_riff_stream(empty);
    memset(&desc, 0, sizeof(desc));
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
    ok(hr == S_OK, "ParseDescriptor failed: %#lx, expected S_OK\n", hr);
    ok(desc.dwValidData == DMUS_OBJ_CLASS, "Got valid data %#lx, expected DMUS_OBJ_CLASS\n",
            desc.dwValidData);
    ok(IsEqualGUID(&desc.guidClass, &CLSID_DirectMusicStyle),
            "Got class guid %s, expected CLSID_DirectMusicStyle\n",
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

    /* All desc chunks, DMUS_OBJ_CATEGORY not supported */
    stream = gen_riff_stream(alldesc);
    memset(&desc, 0, sizeof(desc));
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
    ok(hr == S_OK, "ParseDescriptor failed: %#lx, expected S_OK\n", hr);
    valid = DMUS_OBJ_OBJECT|DMUS_OBJ_CLASS|DMUS_OBJ_NAME|DMUS_OBJ_VERSION;
    ok(desc.dwValidData == valid, "Got valid data %#lx, expected %#lx\n", desc.dwValidData, valid);
    ok(IsEqualGUID(&desc.guidClass, &CLSID_DirectMusicStyle),
            "Got class guid %s, expected CLSID_DirectMusicStyle\n",
            wine_dbgstr_guid(&desc.guidClass));
    ok(IsEqualGUID(&desc.guidObject, &GUID_NULL), "Got object guid %s, expected GUID_NULL\n",
            wine_dbgstr_guid(&desc.guidClass));
    ok(!lstrcmpW(desc.wszName, L"UNAM"), "Got name '%s', expected 'UNAM'\n",
            wine_dbgstr_w(desc.wszName));
    IStream_Release(stream);

    /* UNFO list with INAM */
    inam[3] = DMUS_FOURCC_UNFO_LIST;
    stream = gen_riff_stream(inam);
    memset(&desc, 0, sizeof(desc));
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
    ok(hr == S_OK, "ParseDescriptor failed: %#lx, expected S_OK\n", hr);
    ok(desc.dwValidData == (DMUS_OBJ_CLASS | DMUS_OBJ_NAME),
            "Got valid data %#lx, expected DMUS_OBJ_CLASS | DMUS_OBJ_NAME\n", desc.dwValidData);
    ok(!lstrcmpW(desc.wszName, L"INAM"), "Got name '%s', expected 'INAM'\n",
            wine_dbgstr_w(desc.wszName));
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
    valid = DMUS_OBJ_OBJECT|DMUS_OBJ_CLASS|DMUS_OBJ_NAME|DMUS_OBJ_VERSION;
    ok(desc.dwValidData == valid, "Got valid data %#lx, expected %#lx\n", desc.dwValidData, valid);
    ok(!lstrcmpW(desc.wszName, L"INAM"), "Got name '%s', expected 'INAM'\n",
            wine_dbgstr_w(desc.wszName));
    IStream_Release(stream);

    IDirectMusicObject_Release(dmo);
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
    test_track();
    test_parsedescriptor();

    CoUninitialize();
}
