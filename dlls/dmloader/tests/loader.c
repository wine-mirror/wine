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
#include <ole2.h>
#include "dmusici.h"
#include <dmusicf.h>
#include "wine/test.h"

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);
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

static void test_directory(void)
{
    IDirectMusicLoader8 *loader = NULL;
    HRESULT hr;
    WCHAR con[] = L"con";
    WCHAR empty[] = L"";
    WCHAR invalid_path[] = L"/invalid path";
    WCHAR path[MAX_PATH];

    hr = CoCreateInstance(&CLSID_DirectMusicLoader, NULL, CLSCTX_INPROC, &IID_IDirectMusicLoader8,
            (void**)&loader);
    ok(hr == S_OK, "Couldn't create Loader %#lx\n", hr);

    /* ScanDirectory without a previous SetSearchDirectory isn't failing */
    hr = IDirectMusicLoader_ScanDirectory(loader, &CLSID_DirectMusicContainer, con, NULL);
    ok(hr == S_FALSE, "ScanDirectory for \"con\" files failed with %#lx\n", hr);

    /* SetSearchDirectory with invalid path */
    hr = IDirectMusicLoader_SetSearchDirectory(loader, &GUID_DirectMusicAllTypes, NULL, 0);
    ok(hr == E_POINTER, "SetSearchDirectory failed with %#lx\n", hr);
    hr = IDirectMusicLoader_SetSearchDirectory(loader, &GUID_DirectMusicAllTypes, invalid_path, 0);
    ok(hr == DMUS_E_LOADER_BADPATH, "SetSearchDirectory failed with %#lx\n", hr);

    /* SetSearchDirectory with the current directory */
    GetCurrentDirectoryW(ARRAY_SIZE(path), path);
    hr = IDirectMusicLoader_SetSearchDirectory(loader, &GUID_DirectMusicAllTypes, path, 0);
    ok(hr == S_OK, "SetSearchDirectory failed with %#lx\n", hr);

    /* Two consecutive SetSearchDirectory with the same path */
    GetTempPathW(ARRAY_SIZE(path), path);
    hr = IDirectMusicLoader_SetSearchDirectory(loader, &GUID_DirectMusicAllTypes, path, 0);
    ok(hr == S_OK, "SetSearchDirectory failed with %#lx\n", hr);
    hr = IDirectMusicLoader_SetSearchDirectory(loader, &GUID_DirectMusicAllTypes, path, 0);
    ok(hr == S_FALSE, "Second SetSearchDirectory failed with %#lx\n", hr);
    hr = IDirectMusicLoader_SetSearchDirectory(loader, &CLSID_DirectSoundWave, path, 0);
    ok(hr == S_OK, "SetSearchDirectory failed with %#lx\n", hr);
    hr = IDirectMusicLoader_SetSearchDirectory(loader, &CLSID_DirectSoundWave, path, 0);
    ok(hr == S_FALSE, "Second SetSearchDirectory failed with %#lx\n", hr);

    /* Invalid GUIDs */
    if (0)
        IDirectMusicLoader_SetSearchDirectory(loader, NULL, path, 0); /* Crashes on Windows */
    hr = IDirectMusicLoader_SetSearchDirectory(loader, &IID_IDirectMusicLoader8, path, 0);
    ok(hr == S_OK, "SetSearchDirectory failed with %#lx\n", hr);
    hr = IDirectMusicLoader_ScanDirectory(loader, &GUID_DirectMusicAllTypes, con, NULL);
    ok(hr == REGDB_E_CLASSNOTREG, "ScanDirectory failed, received %#lx\n", hr);

    /* NULL extension is not an error */
    hr = IDirectMusicLoader_ScanDirectory(loader, &CLSID_DirectSoundWave, NULL, NULL);
    ok(hr == S_FALSE, "ScanDirectory for \"wav\" files failed, received %#lx\n", hr);

    IDirectMusicLoader_Release(loader);

    /* An empty path is a valid path */
    hr = CoCreateInstance(&CLSID_DirectMusicLoader, NULL, CLSCTX_INPROC, &IID_IDirectMusicLoader8,
            (void**)&loader);
    ok(hr == S_OK, "Couldn't create Loader %#lx\n", hr);
    hr = IDirectMusicLoader_SetSearchDirectory(loader, &GUID_DirectMusicAllTypes, empty, 0);
    ok(hr == S_OK, "SetSearchDirectory failed with %#lx\n", hr);
    hr = IDirectMusicLoader_SetSearchDirectory(loader, &GUID_DirectMusicAllTypes, empty, 0);
    ok(hr == S_FALSE, "SetSearchDirectory failed with %#lx\n", hr);
    hr = IDirectMusicLoader_ScanDirectory(loader, &CLSID_DirectMusicContainer, con, NULL);
    ok(hr == S_FALSE, "ScanDirectory for \"con\" files failed with %#lx\n", hr);
    IDirectMusicLoader_Release(loader);
}

static void test_caching(void)
{
    IDirectMusicLoader8 *loader = NULL;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectMusicLoader, NULL, CLSCTX_INPROC, &IID_IDirectMusicLoader8,
            (void**)&loader);
    ok(hr == S_OK, "Couldn't create Loader %#lx\n", hr);

    /* Invalid GUID */
    if (0) /* Crashes on Windows */
        IDirectMusicLoader_EnableCache(loader, NULL, TRUE);
    hr = IDirectMusicLoader_EnableCache(loader, &IID_IDirectMusicLoader8, TRUE);
    ok(hr == S_FALSE, "EnableCache failed with %#lx\n", hr);

    /* Caching is enabled by default */
    hr = IDirectMusicLoader_EnableCache(loader, &CLSID_DirectMusicContainer, TRUE);
    ok(hr == S_FALSE, "EnableCache failed with %#lx\n", hr);
    hr = IDirectMusicLoader_EnableCache(loader, &GUID_DirectMusicAllTypes, TRUE);
    ok(hr == S_FALSE, "EnableCache failed with %#lx\n", hr);

    /* Disabling/enabling the cache for all types */
    hr = IDirectMusicLoader_EnableCache(loader, &GUID_DirectMusicAllTypes, FALSE);
    ok(hr == S_OK, "EnableCache failed with %#lx\n", hr);
    hr = IDirectMusicLoader_EnableCache(loader, &CLSID_DirectMusicContainer, FALSE);
    ok(hr == S_FALSE, "EnableCache failed with %#lx\n", hr);
    hr = IDirectMusicLoader_EnableCache(loader, &GUID_DirectMusicAllTypes, TRUE);
    ok(hr == S_OK, "EnableCache failed with %#lx\n", hr);
    hr = IDirectMusicLoader_EnableCache(loader, &CLSID_DirectMusicContainer, FALSE);
    ok(hr == S_OK, "EnableCache failed with %#lx\n", hr);
    hr = IDirectMusicLoader_EnableCache(loader, &GUID_DirectMusicAllTypes, TRUE);
    ok(hr == S_FALSE, "EnableCache failed with %#lx\n", hr);
    hr = IDirectMusicLoader_EnableCache(loader, &CLSID_DirectMusicContainer, TRUE);
    ok(hr == S_FALSE, "EnableCache failed with %#lx\n", hr);

    IDirectMusicLoader_Release(loader);
}

static void test_release_object(void)
{
    HRESULT hr;
    IDirectMusicLoader8* loader = NULL;

    hr = CoCreateInstance(&CLSID_DirectMusicLoader, NULL, CLSCTX_INPROC, &IID_IDirectMusicLoader8, (void**)&loader);
    ok(hr == S_OK, "Couldn't create Loader %08lx\n", hr);

    hr = IDirectMusicLoader_ReleaseObject(loader, NULL);
    ok(hr == E_POINTER, "Expected E_POINTER, received %#lx\n", hr);

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
    ok(hr == S_OK, "Couldn't create Loader %08lx\n", hr);

    hr = IDirectMusicPerformance8_InitAudio(perf, &music, &dsound, NULL,
            DMUS_APATH_DYNAMIC_STEREO, 64, DMUS_AUDIOF_ALL, NULL);
    if (hr == DSERR_NODRIVER) {
        skip("No audio driver.\n");
        return;
    }
    ok(hr == S_OK, "InitAudio failed: %08lx\n", hr);
    ok(music != NULL, "Didn't get IDirectMusic pointer\n");
    ok(dsound != NULL, "Didn't get IDirectSound pointer\n");

    hr = IDirectMusicPerformance8_CreateStandardAudioPath(perf,
            DMUS_APATH_DYNAMIC_STEREO, 64, TRUE, &path);
    ok(hr == S_OK, "CreateStandardAudioPath failed: %08lx\n", hr);
    ok(path != NULL, "Didn't get IDirectMusicAudioPath pointer\n");

    desc.dwSize = sizeof(DMUS_OBJECTDESC);
    desc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_MEMORY;
    desc.guidClass = CLSID_DirectMusicSegment;
    desc.pbMemData = mp3file;
    desc.llMemLength = sizeof(mp3file);
    hr = IDirectMusicLoader8_GetObject(loader, &desc, &IID_IDirectMusicSegment8,
            (void**)&segment);
    ok(hr == DMUS_E_UNSUPPORTED_STREAM, "GetObject gave wrong error: %08lx\n", hr);
    ok(segment == NULL, "Didn't get NULL IDirectMusicSegment pointer\n");

    desc.pbMemData = rifffile;
    desc.llMemLength = sizeof(rifffile);
    hr = IDirectMusicLoader8_GetObject(loader, &desc, &IID_IDirectMusicSegment8,
            (void**)&segment);
    ok(hr == S_OK, "GetObject failed: %08lx\n", hr);
    ok(segment != NULL, "Didn't get IDirectMusicSegment pointer\n");

    hr = IDirectMusicSegment8_Download(segment, (IUnknown*)path);
    ok(hr == S_OK, "Download failed: %08lx\n", hr);

    hr = IDirectMusicSegment8_SetRepeats(segment, 1);
    ok(hr == S_OK, "SetRepeats failed: %08lx\n", hr);

    hr = IDirectMusicPerformance8_PlaySegmentEx(perf, (IUnknown*)segment,
            NULL, NULL, DMUS_SEGF_SECONDARY, 0, &state, NULL, (IUnknown*)path);
    ok(hr == S_OK, "PlaySegmentEx failed: %08lx\n", hr);
    ok(state != NULL, "Didn't get IDirectMusicSegmentState pointer\n");

    hr = IDirectMusicPerformance8_CloseDown(perf);
    ok(hr == S_OK, "CloseDown failed: %08lx\n", hr);

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
    hr = CoCreateInstance(&CLSID_DirectMusicLoader, (IUnknown *)0xdeadbeef, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dml8);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicLoader create failed: %08lx, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dml8, "dml8 = %p\n", dml8);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicLoader, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void**)&dml8);
    ok(hr == E_NOINTERFACE, "DirectMusicLoader create failed: %08lx, expected E_NOINTERFACE\n", hr);

    /* Same refcount for all DirectMusicLoader interfaces */
    hr = CoCreateInstance(&CLSID_DirectMusicLoader, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicLoader8, (void**)&dml8);
    ok(hr == S_OK, "DirectMusicLoader create failed: %08lx, expected S_OK\n", hr);
    refcount = IDirectMusicLoader8_AddRef(dml8);
    ok(refcount == 2, "refcount == %lu, expected 2\n", refcount);

    hr = IDirectMusicLoader8_QueryInterface(dml8, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %08lx\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 4, "refcount == %lu, expected 4\n", refcount);
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
    hr = CoCreateInstance(&CLSID_DirectMusicContainer, (IUnknown *)0xdeadbeef, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dmc);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicContainer create failed: %08lx, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dmc, "dmc = %p\n", dmc);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicContainer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IClassFactory, (void**)&dmc);
    ok(hr == E_NOINTERFACE, "DirectMusicContainer create failed: %08lx, expected E_NOINTERFACE\n", hr);

    /* Same refcount for all DirectMusicContainer interfaces */
    hr = CoCreateInstance(&CLSID_DirectMusicContainer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicContainer, (void**)&dmc);
    ok(hr == S_OK, "DirectMusicContainer create failed: %08lx, expected S_OK\n", hr);
    refcount = IDirectMusicContainer_AddRef(dmc);
    ok(refcount == 2, "refcount == %lu, expected 2\n", refcount);

    hr = IDirectMusicContainer_QueryInterface(dmc, &IID_IDirectMusicObject, (void**)&dmo);
    ok(hr == S_OK, "QueryInterface for IID_IDirectMusicObject failed: %08lx\n", hr);
    refcount = IDirectMusicObject_AddRef(dmo);
    ok(refcount == 4, "refcount == %lu, expected 4\n", refcount);
    refcount = IDirectMusicObject_Release(dmo);

    hr = IDirectMusicContainer_QueryInterface(dmc, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %08lx\n", hr);
    refcount = IPersistStream_AddRef(ps);
    ok(refcount == 5, "refcount == %lu, expected 5\n", refcount);
    refcount = IPersistStream_Release(ps);

    hr = IDirectMusicContainer_QueryInterface(dmc, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %08lx\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 6, "refcount == %lu, expected 6\n", refcount);
    refcount = IUnknown_Release(unk);

    while (IDirectMusicContainer_Release(dmc));
}

static void test_container(void)
{
    IDirectMusicContainer *dmc;
    IDirectMusicObject *dmo;
    IPersistStream *ps;
    DMUS_OBJECTDESC desc;
    CLSID class = { 0 };
    ULARGE_INTEGER size;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectMusicContainer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicContainer, (void**)&dmc);
    ok(hr == S_OK, "DirectMusicContainer create failed: %08lx, expected S_OK\n", hr);

    /* IDirectMusicObject */
    hr = IDirectMusicContainer_QueryInterface(dmc, &IID_IDirectMusicObject, (void**)&dmo);
    ok(hr == S_OK, "QueryInterface for IID_IDirectMusicObject failed: %08lx\n", hr);
    hr = IDirectMusicObject_GetDescriptor(dmo, NULL);
    ok(hr == E_POINTER, "IDirectMusicObject_GetDescriptor: expected E_POINTER, got  %08lx\n", hr);
    hr = IDirectMusicObject_SetDescriptor(dmo, NULL);
    ok(hr == E_POINTER, "IDirectMusicObject_SetDescriptor: expected E_POINTER, got  %08lx\n", hr);
    ZeroMemory(&desc, sizeof(desc));
    hr = IDirectMusicObject_GetDescriptor(dmo, &desc);
    ok(hr == S_OK, "IDirectMusicObject_GetDescriptor failed: %08lx\n", hr);
    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize = 1;  /* size is ignored */
    hr = IDirectMusicObject_GetDescriptor(dmo, &desc);
    ok(hr == S_OK, "IDirectMusicObject_GetDescriptor failed: %08lx\n", hr);
    ok(desc.dwValidData == DMUS_OBJ_CLASS,
            "Fresh object has more valid data (%#lx) than DMUS_OBJ_CLASS\n", desc.dwValidData);
    /* DMUS_OBJ_CLASS is immutable */
    desc.dwValidData = DMUS_OBJ_CLASS;
    desc.dwSize = 1;  /* size is ignored */
    hr = IDirectMusicObject_SetDescriptor(dmo, &desc);
    ok(hr == S_FALSE , "IDirectMusicObject_SetDescriptor failed: %08lx\n", hr);
    ok(!desc.dwValidData, "dwValidData wasn't cleared:  %#lx\n", desc.dwValidData);
    desc.dwValidData = DMUS_OBJ_CLASS;
    desc.guidClass = CLSID_DirectMusicSegment;
    hr = IDirectMusicObject_SetDescriptor(dmo, &desc);
    ok(hr == S_FALSE && !desc.dwValidData, "IDirectMusicObject_SetDescriptor failed: %08lx\n", hr);
    hr = IDirectMusicObject_GetDescriptor(dmo, &desc);
    ok(hr == S_OK, "IDirectMusicObject_GetDescriptor failed: %08lx\n", hr);
    ok(IsEqualGUID(&desc.guidClass, &CLSID_DirectMusicContainer),
            "guidClass changed, should be CLSID_DirectMusicContainer\n");

    /* IPersistStream */
    hr = IDirectMusicContainer_QueryInterface(dmc, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %08lx\n", hr);
    hr = IPersistStream_GetClassID(ps, NULL);
    ok(hr == E_POINTER, "IPersistStream_GetClassID failed: %08lx\n", hr);
    hr = IPersistStream_GetClassID(ps, &class);
    ok(hr == S_OK, "IPersistStream_GetClassID failed: %08lx\n", hr);
    ok(IsEqualGUID(&class, &CLSID_DirectMusicContainer),
            "Expected class CLSID_DirectMusicContainer got %s\n", wine_dbgstr_guid(&class));

    /* Unimplemented IPersistStream methods */
    hr = IPersistStream_IsDirty(ps);
    ok(hr == S_FALSE, "IPersistStream_IsDirty failed: %08lx\n", hr);
    hr = IPersistStream_GetSizeMax(ps, &size);
    ok(hr == E_NOTIMPL, "IPersistStream_GetSizeMax failed: %08lx\n", hr);
    hr = IPersistStream_Save(ps, NULL, TRUE);
    ok(hr == E_NOTIMPL, "IPersistStream_Save failed: %08lx\n", hr);

    while (IDirectMusicContainer_Release(dmc));
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
    DMUS_OBJECTDESC desc;
    HRESULT hr;
    DWORD valid;
    const FOURCC alldesc[] =
    {
        FOURCC_RIFF, DMUS_FOURCC_CONTAINER_FORM, DMUS_FOURCC_CATEGORY_CHUNK, FOURCC_LIST,
        DMUS_FOURCC_UNFO_LIST, DMUS_FOURCC_UNAM_CHUNK, DMUS_FOURCC_UCOP_CHUNK,
        DMUS_FOURCC_UCMT_CHUNK, DMUS_FOURCC_USBJ_CHUNK, 0, DMUS_FOURCC_VERSION_CHUNK,
        DMUS_FOURCC_GUID_CHUNK, 0
    };
    const FOURCC dupes[] =
    {
        FOURCC_RIFF, DMUS_FOURCC_CONTAINER_FORM, DMUS_FOURCC_CATEGORY_CHUNK,
        DMUS_FOURCC_CATEGORY_CHUNK, DMUS_FOURCC_VERSION_CHUNK, DMUS_FOURCC_VERSION_CHUNK,
        DMUS_FOURCC_GUID_CHUNK, DMUS_FOURCC_GUID_CHUNK, FOURCC_LIST, DMUS_FOURCC_UNFO_LIST,
        DMUS_FOURCC_UNAM_CHUNK, 0, FOURCC_LIST, DMUS_FOURCC_UNFO_LIST, mmioFOURCC('I','N','A','M'),
        0, 0
    };
    FOURCC empty[] = {FOURCC_RIFF, DMUS_FOURCC_CONTAINER_FORM, 0};
    FOURCC inam[] =
    {
        FOURCC_RIFF, DMUS_FOURCC_CONTAINER_FORM, FOURCC_LIST, DMUS_FOURCC_UNFO_LIST,
        mmioFOURCC('I','N','A','M'), 0, 0
    };

    hr = CoCreateInstance(&CLSID_DirectMusicContainer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void **)&dmo);
    ok(hr == S_OK, "DirectMusicContainer create failed: %08lx, expected S_OK\n", hr);

    /* Nothing loaded */
    memset(&desc, 0, sizeof(desc));
    hr = IDirectMusicObject_GetDescriptor(dmo, &desc);
    ok(hr == S_OK, "GetDescriptor failed: %08lx, expected S_OK\n", hr);
    ok(desc.dwValidData == DMUS_OBJ_CLASS /* XP */ ||
            broken(desc.dwValidData == (DMUS_OBJ_OBJECT | DMUS_OBJ_CLASS)), /* Vista and above */
            "Got valid data %#lx, expected DMUS_OBJ_OBJECT\n", desc.dwValidData);
    ok(IsEqualGUID(&desc.guidClass, &CLSID_DirectMusicContainer),
            "Got class guid %s, expected CLSID_DirectMusicContainer\n",
            wine_dbgstr_guid(&desc.guidClass));

    /* Empty RIFF stream */
    stream = gen_riff_stream(empty);
    memset(&desc, 0, sizeof(desc));
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
    ok(hr == E_INVALIDARG, "ParseDescriptor failed: %08lx, expected E_INVALIDARG\n", hr);
    desc.dwSize = sizeof(desc);
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
    ok(hr == S_OK, "ParseDescriptor failed: %08lx, expected S_OK\n", hr);
    ok(desc.dwValidData == DMUS_OBJ_CLASS, "Got valid data %#lx, expected DMUS_OBJ_CLASS\n",
            desc.dwValidData);
    ok(IsEqualGUID(&desc.guidClass, &CLSID_DirectMusicContainer),
            "Got class guid %s, expected CLSID_DirectMusicContainer\n",
            wine_dbgstr_guid(&desc.guidClass));
    IStream_Release(stream);

    /* NULL pointers */
    memset(&desc, 0, sizeof(desc));
    hr = IDirectMusicObject_ParseDescriptor(dmo, NULL, &desc);
    ok(hr == E_POINTER, "ParseDescriptor failed: %08lx, expected E_POINTER\n", hr);
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, NULL);
    ok(hr == E_INVALIDARG, "ParseDescriptor failed: %08lx, expected E_INVALIDARG\n", hr);
    hr = IDirectMusicObject_ParseDescriptor(dmo, NULL, NULL);
    ok(hr == E_POINTER, "ParseDescriptor failed: %08lx, expected E_POINTER\n", hr);

    /* Wrong form */
    empty[1] = DMUS_FOURCC_SEGMENT_FORM;
    stream = gen_riff_stream(empty);
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
    ok(hr == DMUS_E_DESCEND_CHUNK_FAIL,
            "ParseDescriptor failed: %08lx, expected DMUS_E_DESCEND_CHUNK_FAIL\n", hr);
    IStream_Release(stream);

    /* All desc chunks */
    stream = gen_riff_stream(alldesc);
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
    ok(hr == S_OK, "ParseDescriptor failed: %08lx, expected S_OK\n", hr);
    valid = DMUS_OBJ_OBJECT|DMUS_OBJ_CLASS|DMUS_OBJ_NAME|DMUS_OBJ_CATEGORY|DMUS_OBJ_VERSION;
    ok(desc.dwValidData == valid, "Got valid data %#lx, expected %#lx\n", desc.dwValidData, valid);
    ok(IsEqualGUID(&desc.guidClass, &CLSID_DirectMusicContainer),
            "Got class guid %s, expected CLSID_DirectMusicContainer\n",
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
    desc.dwSize = sizeof(desc);
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
    ok(hr == S_OK, "ParseDescriptor failed: %08lx, expected S_OK\n", hr);
    ok(desc.dwValidData == DMUS_OBJ_CLASS, "Got valid data %#lx, expected DMUS_OBJ_CLASS\n",
            desc.dwValidData);
    IStream_Release(stream);

    /* INFO list with INAM */
    inam[3] = DMUS_FOURCC_INFO_LIST;
    stream = gen_riff_stream(inam);
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
    ok(hr == S_OK, "ParseDescriptor failed: %08lx, expected S_OK\n", hr);
    ok(desc.dwValidData == DMUS_OBJ_CLASS, "Got valid data %#lx, expected DMUS_OBJ_CLASS\n",
            desc.dwValidData);
    IStream_Release(stream);

    /* Duplicated chunks */
    stream = gen_riff_stream(dupes);
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
    ok(hr == S_OK, "ParseDescriptor failed: %08lx, expected S_OK\n", hr);
    valid = DMUS_OBJ_OBJECT|DMUS_OBJ_CLASS|DMUS_OBJ_NAME|DMUS_OBJ_CATEGORY|DMUS_OBJ_VERSION;
    ok(desc.dwValidData == valid, "Got valid data %#lx, expected %#lx\n", desc.dwValidData, valid);
    ok(!lstrcmpW(desc.wszName, L"UNAM"), "Got name '%s', expected 'UNAM'\n",
            wine_dbgstr_w(desc.wszName));
    IStream_Release(stream);

    IDirectMusicObject_Release(dmo);
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
    test_directory();
    test_caching();
    test_release_object();
    test_simple_playing();
    test_COM();
    test_COM_container();
    test_container();
    test_parsedescriptor();

    CoUninitialize();
}
