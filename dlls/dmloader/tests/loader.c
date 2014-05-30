/*
 * Copyright (C) 2010 David Adam
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

#include "initguid.h"
#include "dmusici.h"
#include "wine/test.h"

static unsigned char mp3file[] = "\xFF\xFB\x92\x04"; /* MP3 header */
static unsigned char rifffile[8+4+8+16+8+256] = "RIFF\x24\x01\x00\x00WAVE" /* header: 4 ("WAVE") + (8 + 16) (format segment) + (8 + 256) (data segment) = 0x124 */
        "fmt \x10\x00\x00\x00\x01\x00\x20\x00\xAC\x44\x00\x00\x10\xB1\x02\x00\x04\x00\x10\x00" /* format segment: PCM, 2 chan, 44100 Hz, 16 bits */
        "data\x00\x01\x00\x00"; /* 256 byte data segment (silence) */

static BOOL missing_dmloader(void)
{
    IDirectMusicLoader8 *dml;
    HRESULT hr = CoCreateInstance(&CLSID_DirectMusicLoader, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicLoader8, (void**)&dml);

    if (hr == S_OK && dml)
    {
        IDirectMusicLoader8_Release(dml);
        return FALSE;
    }
    return TRUE;
}

static void test_release_object(void)
{
    HRESULT hr;
    IDirectMusicLoader8* loader = NULL;

    hr = CoCreateInstance(&CLSID_DirectMusicLoader, NULL, CLSCTX_INPROC, &IID_IDirectMusicLoader8, (void**)&loader);

    hr = IDirectMusicLoader_ReleaseObject(loader, NULL);
    ok(hr == E_POINTER, "Expected E_POINTER, received %#x\n", hr);

    IDirectMusicLoader_Release(loader);
}

static void test_simple_playing(void)
{
    IDirectMusic *music = NULL;
    IDirectMusicLoader8 *loader;
    IDirectMusicPerformance8 *perf;
    IDirectMusicAudioPath8 *path = NULL;
    IDirectMusicSegment8 *segment = NULL;
    IDirectMusicSegmentState *state = NULL;
    IDirectSound *dsound = NULL;
    DMUS_OBJECTDESC desc = {0};
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL,
            CLSCTX_INPROC_SERVER, &IID_IDirectMusicPerformance8,
            (void**)&perf);
    if ( FAILED(hr) )
    {
        skip("CoCreateInstance failed.\n");
        return;
    }

    hr = CoCreateInstance(&CLSID_DirectMusicLoader, NULL,
            CLSCTX_INPROC_SERVER, &IID_IDirectMusicLoader8,
            (void**)&loader);
    ok(hr == S_OK, "Couldn't create Loader %08x\n", hr);

    hr = IDirectMusicPerformance8_InitAudio(perf, &music, &dsound, NULL,
            DMUS_APATH_DYNAMIC_STEREO, 64, DMUS_AUDIOF_ALL, NULL);
    if (hr == DSERR_NODRIVER) {
        skip("No audio driver.\n");
        return;
    }
    ok(hr == S_OK, "InitAudio failed: %08x\n", hr);
    ok(music != NULL, "Didn't get IDirectMusic pointer\n");
    ok(dsound != NULL, "Didn't get IDirectSound pointer\n");

    hr = IDirectMusicPerformance8_CreateStandardAudioPath(perf,
            DMUS_APATH_DYNAMIC_STEREO, 64, TRUE, &path);
    ok(hr == S_OK, "CreateStandardAudioPath failed: %08x\n", hr);
    ok(path != NULL, "Didn't get IDirectMusicAudioPath pointer\n");

    desc.dwSize = sizeof(DMUS_OBJECTDESC);
    desc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_MEMORY;
    desc.guidClass = CLSID_DirectMusicSegment;
    desc.pbMemData = mp3file;
    desc.llMemLength = sizeof(mp3file);
    hr = IDirectMusicLoader8_GetObject(loader, &desc, &IID_IDirectMusicSegment8,
            (void**)&segment);
    ok(hr == DMUS_E_UNSUPPORTED_STREAM, "GetObject gave wrong error: %08x\n", hr);
    ok(segment == NULL, "Didn't get NULL IDirectMusicSegment pointer\n");

    desc.pbMemData = rifffile;
    desc.llMemLength = sizeof(rifffile);
    hr = IDirectMusicLoader8_GetObject(loader, &desc, &IID_IDirectMusicSegment8,
            (void**)&segment);
    ok(hr == S_OK, "GetObject failed: %08x\n", hr);
    ok(segment != NULL, "Didn't get IDirectMusicSegment pointer\n");

    hr = IDirectMusicSegment8_Download(segment, (IUnknown*)path);
    ok(hr == S_OK, "Download failed: %08x\n", hr);

    hr = IDirectMusicSegment8_SetRepeats(segment, 1);
    ok(hr == S_OK, "SetRepeats failed: %08x\n", hr);

    hr = IDirectMusicPerformance8_PlaySegmentEx(perf, (IUnknown*)segment,
            NULL, NULL, DMUS_SEGF_SECONDARY, 0, &state, NULL, (IUnknown*)path);
    ok(hr == S_OK, "PlaySegmentEx failed: %08x\n", hr);
    ok(state != NULL, "Didn't get IDirectMusicSegmentState pointer\n");

    hr = IDirectMusicPerformance8_CloseDown(perf);
    ok(hr == S_OK, "CloseDown failed: %08x\n", hr);

    IDirectSound_Release(dsound);
    IDirectMusicSegmentState_Release(state);
    IDirectMusicSegment8_Release(segment);
    IDirectMusic_Release(music);
    IDirectMusicPerformance8_Release(perf);
    IDirectMusicLoader8_Release(loader);
}

static void test_COM(void)
{
    IDirectMusicLoader8 *dml8 = (IDirectMusicLoader8*)0xdeadbeef;
    IUnknown *unk;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectMusicLoader, (IUnknown*)&dml8, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dml8);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicLoader create failed: %08x, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dml8, "dml8 = %p\n", dml8);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicLoader, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void**)&dml8);
    ok(hr == E_NOINTERFACE, "DirectMusicLoader create failed: %08x, expected E_NOINTERFACE\n", hr);

    /* Same refcount for all DirectMusicLoader interfaces */
    hr = CoCreateInstance(&CLSID_DirectMusicLoader, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicLoader8, (void**)&dml8);
    ok(hr == S_OK, "DirectMusicLoader create failed: %08x, expected S_OK\n", hr);
    refcount = IDirectMusicLoader8_AddRef(dml8);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);

    hr = IDirectMusicLoader8_QueryInterface(dml8, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %08x\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 4, "refcount == %u, expected 4\n", refcount);
    refcount = IUnknown_Release(unk);

    while (IDirectMusicLoader8_Release(dml8));
}

static void test_COM_container(void)
{
    IDirectMusicContainer *dmc = (IDirectMusicContainer*)0xdeadbeef;
    IDirectMusicObject *dmo;
    IPersistStream *ps;
    IUnknown *unk;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectMusicContainer, (IUnknown*)&dmc, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dmc);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicContainer create failed: %08x, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dmc, "dmc = %p\n", dmc);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicContainer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IClassFactory, (void**)&dmc);
    ok(hr == E_NOINTERFACE, "DirectMusicContainer create failed: %08x, expected E_NOINTERFACE\n", hr);

    /* Same refcount for all DirectMusicContainer interfaces */
    hr = CoCreateInstance(&CLSID_DirectMusicContainer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicContainer, (void**)&dmc);
    ok(hr == S_OK, "DirectMusicContainer create failed: %08x, expected S_OK\n", hr);
    refcount = IDirectMusicContainer_AddRef(dmc);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);

    hr = IDirectMusicContainer_QueryInterface(dmc, &IID_IDirectMusicObject, (void**)&dmo);
    ok(hr == S_OK, "QueryInterface for IID_IDirectMusicObject failed: %08x\n", hr);
    refcount = IDirectMusicObject_AddRef(dmo);
    ok(refcount == 4, "refcount == %u, expected 4\n", refcount);
    refcount = IDirectMusicObject_Release(dmo);

    hr = IDirectMusicContainer_QueryInterface(dmc, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %08x\n", hr);
    refcount = IPersistStream_AddRef(ps);
    ok(refcount == 5, "refcount == %u, expected 5\n", refcount);
    refcount = IPersistStream_Release(ps);

    hr = IDirectMusicContainer_QueryInterface(dmc, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %08x\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 6, "refcount == %u, expected 6\n", refcount);
    refcount = IUnknown_Release(unk);

    while (IDirectMusicContainer_Release(dmc));
}

static void test_container(void)
{
    IDirectMusicContainer *dmc;
    IPersistStream *ps;
    CLSID class = { 0 };
    ULARGE_INTEGER size;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectMusicContainer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicContainer, (void**)&dmc);
    ok(hr == S_OK, "DirectMusicContainer create failed: %08x, expected S_OK\n", hr);

    /* IPersistStream */
    hr = IDirectMusicContainer_QueryInterface(dmc, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %08x\n", hr);
    hr = IPersistStream_GetClassID(ps, &class);
    ok(hr == S_OK, "IPersistStream_GetClassID failed: %08x\n", hr);
    ok(IsEqualGUID(&class, &CLSID_DirectMusicContainer),
            "Expected class CLSID_DirectMusicContainer got %s\n", wine_dbgstr_guid(&class));

    /* Unimplemented IPersistStream methods */
    hr = IPersistStream_IsDirty(ps);
    todo_wine ok(hr == S_FALSE, "IPersistStream_IsDirty failed: %08x\n", hr);
    hr = IPersistStream_GetSizeMax(ps, &size);
    ok(hr == E_NOTIMPL, "IPersistStream_GetSizeMax failed: %08x\n", hr);
    hr = IPersistStream_Save(ps, NULL, TRUE);
    ok(hr == E_NOTIMPL, "IPersistStream_Save failed: %08x\n", hr);

    while (IDirectMusicContainer_Release(dmc));
}

START_TEST(loader)
{
    CoInitialize(NULL);

    if (missing_dmloader())
    {
        skip("dmloader not available\n");
        CoUninitialize();
        return;
    }
    test_release_object();
    test_simple_playing();
    test_COM();
    test_COM_container();
    test_container();
    CoUninitialize();
}
