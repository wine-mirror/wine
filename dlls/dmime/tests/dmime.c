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
#include <math.h>
#include <windef.h>
#include <wine/test.h>
#include <initguid.h>
#include <ole2.h>
#include <dmusici.h>
#include <dmusicf.h>
#include <audioclient.h>
#include <guiddef.h>

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);
DEFINE_GUID(GUID_Bunk,0xFFFFFFFF,0xFFFF,0xFFFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF);

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c, FALSE)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported, BOOL check_refs)
{
    ULONG expect_ref = get_refcount(iface_ptr);
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected;
    IUnknown *unk;

    expected = supported ? S_OK : E_NOINTERFACE;
    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected, "got hr %#lx, expected %#lx.\n", hr, expected);
    if (SUCCEEDED(hr))
    {
        LONG ref = get_refcount(unk);
        if (check_refs) ok_(__FILE__, line)(ref == expect_ref + 1, "got %ld\n", ref);
        IUnknown_Release(unk);
        ref = get_refcount(iface_ptr);
        if (check_refs) ok_(__FILE__, line)(ref == expect_ref, "got %ld\n", ref);
    }
}

static double scale_music_time(MUSIC_TIME time, double tempo)
{
    return (600000000.0 * time) / (tempo * DMUS_PPQ);
}

static MUSIC_TIME music_time_from_reference(REFERENCE_TIME time, double tempo)
{
    return (time * tempo * DMUS_PPQ) / 600000000;
}

#define check_dmus_note_pmsg(a, b, c, d, e, f, g) check_dmus_note_pmsg_(__LINE__, a, b, c, d, e, f, g)
static void check_dmus_note_pmsg_(int line, DMUS_NOTE_PMSG *msg, MUSIC_TIME time, UINT chan,
        UINT duration, UINT key, UINT vel, UINT flags)
{
    ok_(__FILE__, line)(msg->dwSize == sizeof(*msg), "got dwSize %lu\n", msg->dwSize);
    ok_(__FILE__, line)(!!msg->rtTime, "got rtTime %I64u\n", msg->rtTime);
    ok_(__FILE__, line)(abs(msg->mtTime - time) < 10, "got mtTime %lu\n", msg->mtTime);
    ok_(__FILE__, line)(msg->dwPChannel == chan, "got dwPChannel %lu\n", msg->dwPChannel);
    ok_(__FILE__, line)(!!msg->dwVirtualTrackID, "got dwVirtualTrackID %lu\n", msg->dwVirtualTrackID);
    ok_(__FILE__, line)(msg->dwType == DMUS_PMSGT_NOTE, "got %#lx\n", msg->dwType);
    ok_(__FILE__, line)(!msg->dwVoiceID, "got dwVoiceID %lu\n", msg->dwVoiceID);
    ok_(__FILE__, line)(msg->dwGroupID == 1, "got dwGroupID %lu\n", msg->dwGroupID);
    ok_(__FILE__, line)(!msg->punkUser, "got punkUser %p\n", msg->punkUser);
    ok_(__FILE__, line)(msg->mtDuration == duration, "got mtDuration %lu\n", msg->mtDuration);
    ok_(__FILE__, line)(msg->wMusicValue == key, "got wMusicValue %u\n", msg->wMusicValue);
    ok_(__FILE__, line)(!msg->wMeasure, "got wMeasure %u\n", msg->wMeasure);
    /* FIXME: ok_(__FILE__, line)(!msg->nOffset, "got nOffset %u\n", msg->nOffset); */
    /* FIXME: ok_(__FILE__, line)(!msg->bBeat, "got bBeat %u\n", msg->bBeat); */
    /* FIXME: ok_(__FILE__, line)(!msg->bGrid, "got bGrid %u\n", msg->bGrid); */
    ok_(__FILE__, line)(msg->bVelocity == vel, "got bVelocity %u\n", msg->bVelocity);
    ok_(__FILE__, line)(msg->bFlags == flags, "got bFlags %#x\n", msg->bFlags);
    ok_(__FILE__, line)(!msg->bTimeRange, "got bTimeRange %u\n", msg->bTimeRange);
    ok_(__FILE__, line)(!msg->bDurRange, "got bDurRange %u\n", msg->bDurRange);
    ok_(__FILE__, line)(!msg->bVelRange, "got bVelRange %u\n", msg->bVelRange);
    ok_(__FILE__, line)(!msg->bPlayModeFlags, "got bPlayModeFlags %#x\n", msg->bPlayModeFlags);
    ok_(__FILE__, line)(!msg->bSubChordLevel, "got bSubChordLevel %u\n", msg->bSubChordLevel);
    ok_(__FILE__, line)(msg->bMidiValue == key, "got bMidiValue %u\n", msg->bMidiValue);
    ok_(__FILE__, line)(!msg->cTranspose, "got cTranspose %u\n", msg->cTranspose);
}

static void load_resource(const WCHAR *name, WCHAR *filename)
{
    static WCHAR path[MAX_PATH];
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    GetTempPathW(ARRAY_SIZE(path), path);
    GetTempFileNameW(path, name, 0, filename);

    file = CreateFileW(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "failed to create %s, error %lu\n", debugstr_w(filename), GetLastError());

    res = FindResourceW(NULL, name, (const WCHAR *)RT_RCDATA);
    ok(res != 0, "couldn't find resource\n");
    ptr = LockResource(LoadResource(GetModuleHandleW(NULL ), res ));
    WriteFile(file, ptr, SizeofResource(GetModuleHandleW(NULL ), res ), &written, NULL);
    ok(written == SizeofResource(GetModuleHandleW(NULL ), res ), "couldn't write resource\n");
    CloseHandle(file);
}

static void stream_begin_chunk(IStream *stream, const char type[5], ULARGE_INTEGER *offset)
{
    static const LARGE_INTEGER zero = {0};
    HRESULT hr;
    hr = IStream_Write(stream, type, 4, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IStream_Seek(stream, zero, STREAM_SEEK_CUR, offset);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IStream_Write(stream, "\0\0\0\0", 4, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
}

static void stream_end_chunk(IStream *stream, ULARGE_INTEGER *offset)
{
    static const LARGE_INTEGER zero = {0};
    ULARGE_INTEGER position;
    HRESULT hr;
    UINT size;
    hr = IStream_Seek(stream, zero, STREAM_SEEK_CUR, &position);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IStream_Seek(stream, *(LARGE_INTEGER *)offset, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    size = position.QuadPart - offset->QuadPart - 4;
    hr = IStream_Write(stream, &size, 4, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IStream_Seek(stream, *(LARGE_INTEGER *)&position, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IStream_Write(stream, &zero, (position.QuadPart & 1), NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
}

#define CHUNK_BEGIN(stream, type)                                \
    do {                                                         \
        ULARGE_INTEGER __off;                                    \
        IStream *__stream = (stream);                            \
        stream_begin_chunk(stream, type, &__off);                \
        do

#define CHUNK_RIFF(stream, form)                                 \
    do {                                                         \
        ULARGE_INTEGER __off;                                    \
        IStream *__stream = (stream);                            \
        stream_begin_chunk(stream, "RIFF", &__off);              \
        IStream_Write(stream, form, 4, NULL);                    \
        do

#define CHUNK_LIST(stream, form)                                 \
    do {                                                         \
        ULARGE_INTEGER __off;                                    \
        IStream *__stream = (stream);                            \
        stream_begin_chunk(stream, "LIST", &__off);              \
        IStream_Write(stream, form, 4, NULL);                    \
        do

#define CHUNK_END                                                \
        while (0);                                               \
        stream_end_chunk(__stream, &__off);                      \
    } while (0)

#define CHUNK_DATA(stream, type, data)                           \
    CHUNK_BEGIN(stream, type)                                    \
    {                                                            \
        IStream_Write((stream), &(data), sizeof(data), NULL);    \
    }                                                            \
    CHUNK_END

#define CHUNK_ARRAY(stream, type, items)                         \
    CHUNK_BEGIN(stream, type)                                    \
    {                                                            \
        UINT __size = sizeof(*(items));                          \
        IStream_Write((stream), &__size, 4, NULL);               \
        IStream_Write((stream), &(items), sizeof(items), NULL);  \
    }                                                            \
    CHUNK_END

struct test_tool
{
    IDirectMusicTool IDirectMusicTool_iface;
    LONG ref;

    IDirectMusicGraph *graph;
    const DWORD *types;
    DWORD types_count;

    SRWLOCK lock;
    HANDLE message_event;
    DMUS_PMSG *messages[32];
    UINT message_count;
};

static DMUS_PMSG *test_tool_push_msg(struct test_tool *tool, DMUS_PMSG *msg)
{
    AcquireSRWLockExclusive(&tool->lock);
    ok(tool->message_count < ARRAY_SIZE(tool->messages),
            "got %u messages\n", tool->message_count + 1);
    if (tool->message_count < ARRAY_SIZE(tool->messages))
    {
        memmove(tool->messages + 1, tool->messages,
            tool->message_count * sizeof(*tool->messages));
        tool->messages[0] = msg;
        tool->message_count++;
        msg = NULL;
    }
    ReleaseSRWLockExclusive(&tool->lock);

    return msg;
}

static struct test_tool *impl_from_IDirectMusicTool(IDirectMusicTool *iface)
{
    return CONTAINING_RECORD(iface, struct test_tool, IDirectMusicTool_iface);
}

static HRESULT WINAPI test_tool_QueryInterface(IDirectMusicTool *iface, REFIID iid, void **out)
{
    if (IsEqualGUID(iid, &IID_IUnknown)
            || IsEqualGUID(iid, &IID_IDirectMusicTool))
    {
        IDirectMusicTool_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    ok(IsEqualGUID(iid, &IID_IDirectMusicTool8) || IsEqualGUID(iid, &IID_IPersistStream),
            "got iid %s\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_tool_AddRef(IDirectMusicTool *iface)
{
    struct test_tool *tool = impl_from_IDirectMusicTool(iface);
    return InterlockedIncrement(&tool->ref);
}

static ULONG WINAPI test_tool_Release(IDirectMusicTool *iface)
{
    struct test_tool *tool = impl_from_IDirectMusicTool(iface);
    ULONG ref = InterlockedDecrement(&tool->ref);

    if (!ref)
    {
        if (tool->graph) IDirectMusicGraph_Release(tool->graph);
        ok(!tool->message_count, "got %p\n", &tool->message_count);
        CloseHandle(tool->message_event);
        free(tool);
    }

    return ref;
}

static HRESULT WINAPI test_tool_Init(IDirectMusicTool *iface, IDirectMusicGraph *graph)
{
    struct test_tool *tool = impl_from_IDirectMusicTool(iface);
    if ((tool->graph = graph)) IDirectMusicGraph_AddRef(tool->graph);
    return S_OK;
}

static HRESULT WINAPI test_tool_GetMsgDeliveryType(IDirectMusicTool *iface, DWORD *type)
{
    *type = DMUS_PMSGF_TOOL_IMMEDIATE;
    return S_OK;
}

static HRESULT WINAPI test_tool_GetMediaTypeArraySize(IDirectMusicTool *iface, DWORD *size)
{
    struct test_tool *tool = impl_from_IDirectMusicTool(iface);
    *size = tool->types_count;
    return S_OK;
}

static HRESULT WINAPI test_tool_GetMediaTypes(IDirectMusicTool *iface, DWORD **types, DWORD size)
{
    struct test_tool *tool = impl_from_IDirectMusicTool(iface);
    UINT i;
    for (i = 0; i < tool->types_count; i++) (*types)[i] = tool->types[i];
    return S_OK;
}

static HRESULT WINAPI test_tool_ProcessPMsg(IDirectMusicTool *iface, IDirectMusicPerformance *performance, DMUS_PMSG *msg)
{
    struct test_tool *tool = impl_from_IDirectMusicTool(iface);
    DMUS_PMSG *clone;
    HRESULT hr;

    hr = IDirectMusicPerformance8_ClonePMsg((IDirectMusicPerformance8 *)performance, msg, &clone);
    ok(hr == S_OK, "got %#lx\n", hr);
    clone = test_tool_push_msg(tool, clone);
    ok(!clone, "got %p\n", clone);
    SetEvent(tool->message_event);

    hr = IDirectMusicGraph_StampPMsg(msg->pGraph, msg);
    ok(hr == S_OK, "got %#lx\n", hr);

    return DMUS_S_REQUEUE;
}

static HRESULT WINAPI test_tool_Flush(IDirectMusicTool *iface, IDirectMusicPerformance *performance,
        DMUS_PMSG *msg, REFERENCE_TIME time)
{
    ok(0, "unexpected %s\n", __func__);
    return S_OK;
}

static IDirectMusicToolVtbl test_tool_vtbl =
{
    test_tool_QueryInterface,
    test_tool_AddRef,
    test_tool_Release,
    test_tool_Init,
    test_tool_GetMsgDeliveryType,
    test_tool_GetMediaTypeArraySize,
    test_tool_GetMediaTypes,
    test_tool_ProcessPMsg,
    test_tool_Flush,
};

static HRESULT test_tool_create(const DWORD *types, DWORD types_count,
        IDirectMusicTool **ret_iface)
{
    struct test_tool *tool;

    *ret_iface = NULL;
    if (!(tool = calloc(1, sizeof(*tool)))) return E_OUTOFMEMORY;
    tool->IDirectMusicTool_iface.lpVtbl = &test_tool_vtbl;
    tool->ref = 1;

    tool->types = types;
    tool->types_count = types_count;
    tool->message_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(!!tool->message_event, "CreateEventW failed, error %lu\n", GetLastError());

    *ret_iface = &tool->IDirectMusicTool_iface;
    return S_OK;
}

static HRESULT test_tool_get_graph(IDirectMusicTool *iface, IDirectMusicGraph **graph)
{
    struct test_tool *tool = impl_from_IDirectMusicTool(iface);
    if ((*graph = tool->graph)) IDirectMusicGraph_AddRef(tool->graph);
    return tool->graph ? S_OK : DMUS_E_NOT_FOUND;
}

static DWORD test_tool_wait_message(IDirectMusicTool *iface, DWORD timeout, DMUS_PMSG **msg)
{
    struct test_tool *tool = impl_from_IDirectMusicTool(iface);
    DWORD ret = WAIT_FAILED;

    do
    {
        AcquireSRWLockExclusive(&tool->lock);
        if (!tool->message_count)
            *msg = NULL;
        else
        {
            UINT index = --tool->message_count;
            *msg = tool->messages[index];
            tool->messages[index] = NULL;
        }
        ReleaseSRWLockExclusive(&tool->lock);

        if (*msg) return 0;
    } while (!(ret = WaitForSingleObject(tool->message_event, timeout)));

    return ret;
}

struct test_loader_stream
{
    IStream IStream_iface;
    IDirectMusicGetLoader IDirectMusicGetLoader_iface;
    LONG ref;

    IStream *stream;
    IDirectMusicLoader *loader;
};

static struct test_loader_stream *impl_from_IStream(IStream *iface)
{
    return CONTAINING_RECORD(iface, struct test_loader_stream, IStream_iface);
}

static HRESULT WINAPI test_loader_stream_QueryInterface(IStream *iface, REFIID iid, void **out)
{
    struct test_loader_stream *impl = impl_from_IStream(iface);

    if (IsEqualGUID(iid, &IID_IUnknown)
            || IsEqualGUID(iid, &IID_IStream))
    {
        IStream_AddRef(&impl->IStream_iface);
        *out = &impl->IStream_iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_IDirectMusicGetLoader))
    {
        IDirectMusicGetLoader_AddRef(&impl->IDirectMusicGetLoader_iface);
        *out = &impl->IDirectMusicGetLoader_iface;
        return S_OK;
    }

    ok(IsEqualGUID(iid, &IID_IStream),
            "got iid %s\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_loader_stream_AddRef(IStream *iface)
{
    struct test_loader_stream *impl = impl_from_IStream(iface);
    return InterlockedIncrement(&impl->ref);
}

static ULONG WINAPI test_loader_stream_Release(IStream *iface)
{
    struct test_loader_stream *impl = impl_from_IStream(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);

    if (!ref)
    {
        IDirectMusicLoader_Release(impl->loader);
        IStream_Release(impl->stream);
        free(impl);
    }

    return ref;
}

static HRESULT WINAPI test_loader_stream_Read(IStream *iface, void *data, ULONG size, ULONG *ret_size)
{
    struct test_loader_stream *impl = impl_from_IStream(iface);
    return IStream_Read(impl->stream, data, size, ret_size);
}

static HRESULT WINAPI test_loader_stream_Write(IStream *iface, const void *data, ULONG size, ULONG *ret_size)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_loader_stream_Seek(IStream *iface, LARGE_INTEGER offset, DWORD method, ULARGE_INTEGER *ret_offset)
{
    struct test_loader_stream *impl = impl_from_IStream(iface);
    return IStream_Seek(impl->stream, offset, method, ret_offset);
}

static HRESULT WINAPI test_loader_stream_SetSize(IStream *iface, ULARGE_INTEGER size)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_loader_stream_CopyTo(IStream *iface, IStream *dest, ULARGE_INTEGER size,
        ULARGE_INTEGER *read_size, ULARGE_INTEGER *write_size)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_loader_stream_Commit(IStream *iface, DWORD flags)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_loader_stream_Revert(IStream *iface)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_loader_stream_LockRegion(IStream *iface, ULARGE_INTEGER offset, ULARGE_INTEGER size, DWORD type)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_loader_stream_UnlockRegion(IStream *iface, ULARGE_INTEGER offset, ULARGE_INTEGER size, DWORD type)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_loader_stream_Stat(IStream *iface, STATSTG *stat, DWORD flags)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_loader_stream_Clone(IStream *iface, IStream **out)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IStreamVtbl test_loader_stream_vtbl =
{
    test_loader_stream_QueryInterface,
    test_loader_stream_AddRef,
    test_loader_stream_Release,
    test_loader_stream_Read,
    test_loader_stream_Write,
    test_loader_stream_Seek,
    test_loader_stream_SetSize,
    test_loader_stream_CopyTo,
    test_loader_stream_Commit,
    test_loader_stream_Revert,
    test_loader_stream_LockRegion,
    test_loader_stream_UnlockRegion,
    test_loader_stream_Stat,
    test_loader_stream_Clone,
};

static struct test_loader_stream *impl_from_IDirectMusicGetLoader(IDirectMusicGetLoader *iface)
{
    return CONTAINING_RECORD(iface, struct test_loader_stream, IDirectMusicGetLoader_iface);
}

static HRESULT WINAPI test_loader_stream_getter_QueryInterface(IDirectMusicGetLoader *iface, REFIID iid, void **out)
{
    struct test_loader_stream *impl = impl_from_IDirectMusicGetLoader(iface);
    return IStream_QueryInterface(&impl->IStream_iface, iid, out);
}

static ULONG WINAPI test_loader_stream_getter_AddRef(IDirectMusicGetLoader *iface)
{
    struct test_loader_stream *impl = impl_from_IDirectMusicGetLoader(iface);
    return IStream_AddRef(&impl->IStream_iface);
}

static ULONG WINAPI test_loader_stream_getter_Release(IDirectMusicGetLoader *iface)
{
    struct test_loader_stream *impl = impl_from_IDirectMusicGetLoader(iface);
    return IStream_Release(&impl->IStream_iface);
}

static HRESULT WINAPI test_loader_stream_getter_GetLoader(IDirectMusicGetLoader *iface, IDirectMusicLoader **ret_loader)
{
    struct test_loader_stream *impl = impl_from_IDirectMusicGetLoader(iface);

    *ret_loader = impl->loader;
    IDirectMusicLoader_AddRef(impl->loader);

    return S_OK;
}

static const IDirectMusicGetLoaderVtbl test_loader_stream_getter_vtbl =
{
    test_loader_stream_getter_QueryInterface,
    test_loader_stream_getter_AddRef,
    test_loader_stream_getter_Release,
    test_loader_stream_getter_GetLoader,
};

static HRESULT test_loader_stream_create(IStream *stream, IDirectMusicLoader *loader,
        IStream **ret_iface)
{
    struct test_loader_stream *obj;

    *ret_iface = NULL;
    if (!(obj = calloc(1, sizeof(*obj)))) return E_OUTOFMEMORY;
    obj->IStream_iface.lpVtbl = &test_loader_stream_vtbl;
    obj->IDirectMusicGetLoader_iface.lpVtbl = &test_loader_stream_getter_vtbl;
    obj->ref = 1;

    obj->stream = stream;
    IStream_AddRef(stream);
    obj->loader = loader;
    IDirectMusicLoader_AddRef(loader);

    *ret_iface = &obj->IStream_iface;
    return S_OK;
}

struct test_track
{
    /* Implementing IDirectMusicTrack8 will cause native to call PlayEx */
    IDirectMusicTrack IDirectMusicTrack_iface;
    LONG ref;

    DWORD data;
    BOOL inserted;
    BOOL initialized;
    BOOL downloaded;
    BOOL playing;
    BOOL test_play;
    HANDLE playing_event;
};

#define check_track_state(track, state, value)                                                     \
    do                                                                                             \
    {                                                                                              \
        DWORD ret = impl_from_IDirectMusicTrack(track)->state;                                    \
        ok(ret == (value), "got %#lx\n", ret);                                                     \
    } while (0);

static inline struct test_track *impl_from_IDirectMusicTrack(IDirectMusicTrack *iface)
{
    return CONTAINING_RECORD(iface, struct test_track, IDirectMusicTrack_iface);
}

static HRESULT WINAPI test_track_QueryInterface(IDirectMusicTrack *iface, REFIID riid,
        void **ret_iface)
{
    struct test_track *This = impl_from_IDirectMusicTrack(iface);

    if (IsEqualIID(riid, &IID_IUnknown)
            || IsEqualIID(riid, &IID_IDirectMusicTrack))
    {
        *ret_iface = &This->IDirectMusicTrack_iface;
        IDirectMusicTrack_AddRef(&This->IDirectMusicTrack_iface);
        return S_OK;
    }

    ok(IsEqualGUID(riid, &IID_IDirectMusicTrack8) || IsEqualGUID(riid, &IID_IPersistStream),
            "unexpected %s %p %s\n", __func__, This, debugstr_guid(riid));
    *ret_iface = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_track_AddRef(IDirectMusicTrack *iface)
{
    struct test_track *This = impl_from_IDirectMusicTrack(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI test_track_Release(IDirectMusicTrack *iface)
{
    struct test_track *This = impl_from_IDirectMusicTrack(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    if (!ref)
    {
        CloseHandle(This->playing_event);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI test_track_Init(IDirectMusicTrack *iface, IDirectMusicSegment *segment)
{
    struct test_track *This = impl_from_IDirectMusicTrack(iface);
    This->inserted = TRUE;
    return S_OK;
}

static HRESULT WINAPI test_track_InitPlay(IDirectMusicTrack *iface, IDirectMusicSegmentState *segment_state,
        IDirectMusicPerformance *performance, void **state_data, DWORD track_id, DWORD segment_flags)
{
    struct test_track *This = impl_from_IDirectMusicTrack(iface);

    ok(!!segment_state, "got %p\n", segment_state);
    ok(!!performance, "got %p\n", performance);
    ok(!!state_data, "got %p\n", state_data);
    ok(!!track_id, "got %lu\n", track_id);
    ok(!segment_flags, "got %#lx\n", segment_flags);
    This->initialized = TRUE;

    *state_data = &This->data;
    return S_OK;
}

static HRESULT WINAPI test_track_EndPlay(IDirectMusicTrack *iface, void *state_data)
{
    struct test_track *This = impl_from_IDirectMusicTrack(iface);

    ok(state_data == &This->data, "got %p\n", state_data);
    This->playing = FALSE;

    return S_OK;
}

static HRESULT WINAPI test_track_Play(IDirectMusicTrack *iface, void *state_data,
        MUSIC_TIME start_time, MUSIC_TIME end_time, MUSIC_TIME time_offset, DWORD segment_flags,
        IDirectMusicPerformance *performance, IDirectMusicSegmentState *segment_state, DWORD track_id)
{
    struct test_track *This = impl_from_IDirectMusicTrack(iface);

    if (!This->test_play) return S_OK;

    ok(state_data == &This->data, "got %p\n", state_data);
    ok(start_time == 50, "got %lu\n", start_time);
    ok(end_time == 100, "got %lu\n", end_time);
    todo_wine ok(time_offset < 0, "got %lu\n", time_offset);
    ok(segment_flags == (DMUS_TRACKF_DIRTY|DMUS_TRACKF_START|DMUS_TRACKF_SEEK),
            "got %#lx\n", segment_flags);
    ok(!!performance, "got %p\n", performance);
    ok(!!segment_state, "got %p\n", segment_state);
    ok(!!track_id, "got %lu\n", track_id);
    This->playing = TRUE;
    SetEvent(This->playing_event);

    return S_OK;
}

static HRESULT WINAPI test_track_GetParam(IDirectMusicTrack *iface, REFGUID type, MUSIC_TIME time,
        MUSIC_TIME *next, void *param)
{
    struct test_track *This = impl_from_IDirectMusicTrack(iface);
    ok(0, "unexpected %s %p\n", __func__, This);
    return E_NOTIMPL;
}

static HRESULT WINAPI test_track_SetParam(IDirectMusicTrack *iface, REFGUID type, MUSIC_TIME time, void *param)
{
    struct test_track *This = impl_from_IDirectMusicTrack(iface);

    if (IsEqualGUID(type, &GUID_DownloadToAudioPath))
    {
        This->downloaded = TRUE;
        return S_OK;
    }

    if (IsEqualGUID(type, &GUID_UnloadFromAudioPath))
    {
        This->downloaded = FALSE;
        return S_OK;
    }

    ok(0, "unexpected %s %p %s %lu %p\n", __func__, This, debugstr_guid(type), time, param);
    return E_NOTIMPL;
}

static HRESULT WINAPI test_track_IsParamSupported(IDirectMusicTrack *iface, REFGUID type)
{
    struct test_track *This = impl_from_IDirectMusicTrack(iface);

    if (IsEqualGUID(type, &GUID_DownloadToAudioPath)) return S_OK;
    if (IsEqualGUID(type, &GUID_UnloadFromAudioPath)) return S_OK;
    if (IsEqualGUID(type, &GUID_TimeSignature)) return DMUS_E_TYPE_UNSUPPORTED;
    if (IsEqualGUID(type, &GUID_TempoParam)) return DMUS_E_TYPE_UNSUPPORTED;

    ok(broken(type->Data1 == 0xe8dbd832), /* native also checks some unknown parameter */
            "unexpected %s %p %s\n", __func__, This, debugstr_guid(type));
    return E_NOTIMPL;
}

static HRESULT WINAPI test_track_AddNotificationType(IDirectMusicTrack *iface, REFGUID type)
{
    ok(IsEqualGUID(type, &GUID_NOTIFICATION_SEGMENT) || IsEqualGUID(type, &GUID_NOTIFICATION_PERFORMANCE),
            "got %s\n", debugstr_guid(type));
    return E_NOTIMPL;
}

static HRESULT WINAPI test_track_RemoveNotificationType(IDirectMusicTrack *iface, REFGUID type)
{
    ok(IsEqualGUID(type, &GUID_NOTIFICATION_SEGMENT) || IsEqualGUID(type, &GUID_NOTIFICATION_PERFORMANCE),
            "got %s\n", debugstr_guid(type));
    return E_NOTIMPL;
}

static HRESULT WINAPI test_track_Clone(IDirectMusicTrack *iface, MUSIC_TIME start_time,
        MUSIC_TIME end_time, IDirectMusicTrack **ret_track)
{
    struct test_track *This = impl_from_IDirectMusicTrack(iface);
    ok(0, "unexpected %s %p\n", __func__, This);
    return E_NOTIMPL;
}

static const IDirectMusicTrackVtbl test_track_vtbl =
{
    test_track_QueryInterface,
    test_track_AddRef,
    test_track_Release,
    test_track_Init,
    test_track_InitPlay,
    test_track_EndPlay,
    test_track_Play,
    test_track_GetParam,
    test_track_SetParam,
    test_track_IsParamSupported,
    test_track_AddNotificationType,
    test_track_RemoveNotificationType,
    test_track_Clone,
};

static HRESULT test_track_create(IDirectMusicTrack **ret_iface, BOOL test_play)
{
    struct test_track *track;

    *ret_iface = NULL;
    if (!(track = calloc(1, sizeof(*track)))) return E_OUTOFMEMORY;
    track->IDirectMusicTrack_iface.lpVtbl = &test_track_vtbl;
    track->ref = 1;
    track->test_play = test_play;

    track->playing_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(!!track->playing_event, "CreateEventW failed, error %lu\n", GetLastError());

    *ret_iface = &track->IDirectMusicTrack_iface;
    return S_OK;
}

static DWORD test_track_wait_playing(IDirectMusicTrack *iface, DWORD timeout)
{
    struct test_track *This = impl_from_IDirectMusicTrack(iface);
    return WaitForSingleObject(This->playing_event, timeout);
}

static void create_performance(IDirectMusicPerformance8 **performance, IDirectMusic **dmusic,
        IDirectSound **dsound, BOOL set_cooplevel)
{
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicPerformance8, (void **)performance);
    ok(hr == S_OK, "DirectMusicPerformance create failed: %#lx\n", hr);
    if (dmusic) {
        hr = CoCreateInstance(&CLSID_DirectMusic, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusic8,
                (void **)dmusic);
        ok(hr == S_OK, "DirectMusic create failed: %#lx\n", hr);
    }
    if (dsound) {
        hr = DirectSoundCreate8(NULL, (IDirectSound8 **)dsound, NULL);
        ok(hr == S_OK, "DirectSoundCreate failed: %#lx\n", hr);
        if (set_cooplevel) {
            hr = IDirectSound_SetCooperativeLevel(*dsound, GetForegroundWindow(), DSSCL_PRIORITY);
            ok(hr == S_OK, "SetCooperativeLevel failed: %#lx\n", hr);
        }
    }
}

static void destroy_performance(IDirectMusicPerformance8 *performance, IDirectMusic *dmusic,
        IDirectSound *dsound)
{
    HRESULT hr;

    hr = IDirectMusicPerformance8_CloseDown(performance);
    ok(hr == S_OK, "CloseDown failed: %#lx\n", hr);
    IDirectMusicPerformance8_Release(performance);
    if (dmusic)
        IDirectMusic_Release(dmusic);
    if (dsound)
        IDirectSound_Release(dsound);
}

static BOOL missing_dmime(void)
{
    IDirectMusicSegment8 *dms;
    HRESULT hr = CoCreateInstance(&CLSID_DirectMusicSegment, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicSegment, (void**)&dms);

    if (hr == S_OK && dms)
    {
        IDirectMusicSegment8_Release(dms);
        return FALSE;
    }
    return TRUE;
}

static void test_COM_audiopath(void)
{
    IDirectMusicAudioPath *dmap;
    IUnknown *unk;
    IDirectMusicPerformance8 *performance;
    IDirectSoundBuffer *dsound;
    IDirectSoundBuffer8 *dsound8;
    IDirectSoundNotify *notify;
    IDirectSound3DBuffer *dsound3d;
    IKsPropertySet *propset;
    ULONG refcount;
    HRESULT hr;
    DWORD buffer = 0;

    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicPerformance8, (void**)&performance);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE), "DirectMusicPerformance create failed: %#lx\n", hr);
    if (!performance) {
        win_skip("IDirectMusicPerformance8 not available\n");
        return;
    }
    hr = IDirectMusicPerformance8_InitAudio(performance, NULL, NULL, NULL,
            DMUS_APATH_SHARED_STEREOPLUSREVERB, 64, DMUS_AUDIOF_ALL, NULL);
    ok(hr == S_OK || hr == DSERR_NODRIVER ||
       broken(hr == AUDCLNT_E_ENDPOINT_CREATE_FAILED), /* Win 10 testbot */
       "DirectMusicPerformance_InitAudio failed: %#lx\n", hr);
    if (FAILED(hr)) {
        skip("Audio failed to initialize\n");
        return;
    }
    hr = IDirectMusicPerformance8_GetDefaultAudioPath(performance, &dmap);
    ok(hr == S_OK, "DirectMusicPerformance_GetDefaultAudioPath failed: %#lx\n", hr);

    /* IDirectMusicObject and IPersistStream are not supported */
    hr = IDirectMusicAudioPath_QueryInterface(dmap, &IID_IDirectMusicObject, (void**)&unk);
    ok(FAILED(hr) && !unk, "Unexpected IDirectMusicObject interface: hr=%#lx, iface=%p\n", hr, unk);
    if (unk) IUnknown_Release(unk);
    hr = IDirectMusicAudioPath_QueryInterface(dmap, &IID_IPersistStream, (void**)&unk);
    ok(FAILED(hr) && !unk, "Unexpected IPersistStream interface: hr=%#lx, iface=%p\n", hr, unk);
    if (unk) IUnknown_Release(unk);

    /* Same refcount for all DirectMusicAudioPath interfaces */
    refcount = IDirectMusicAudioPath_AddRef(dmap);
    ok(refcount == 3, "refcount == %lu, expected 3\n", refcount);

    hr = IDirectMusicAudioPath_QueryInterface(dmap, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %#lx\n", hr);
    ok(unk == (IUnknown*)dmap, "got %p, %p\n", unk, dmap);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 5, "refcount == %lu, expected 5\n", refcount);
    refcount = IUnknown_Release(unk);

    hr = IDirectMusicAudioPath_GetObjectInPath(dmap, DMUS_PCHANNEL_ALL, DMUS_PATH_BUFFER, buffer, &GUID_NULL,
                0, &IID_IDirectSoundBuffer, (void**)&dsound);
    ok(hr == S_OK, "Failed: %#lx\n", hr);
    IDirectSoundBuffer_Release(dsound);

    hr = IDirectMusicAudioPath_GetObjectInPath(dmap, DMUS_PCHANNEL_ALL, DMUS_PATH_BUFFER, buffer, &GUID_NULL,
                0, &IID_IDirectSoundBuffer8, (void**)&dsound8);
    ok(hr == S_OK, "Failed: %#lx\n", hr);
    IDirectSoundBuffer8_Release(dsound8);

    hr = IDirectMusicAudioPath_GetObjectInPath(dmap, DMUS_PCHANNEL_ALL, DMUS_PATH_BUFFER, buffer, &GUID_NULL,
                0, &IID_IDirectSoundNotify, (void**)&notify);
    ok(hr == E_NOINTERFACE, "Failed: %#lx\n", hr);

    hr = IDirectMusicAudioPath_GetObjectInPath(dmap, DMUS_PCHANNEL_ALL, DMUS_PATH_BUFFER, buffer, &GUID_NULL,
                0, &IID_IDirectSound3DBuffer, (void**)&dsound3d);
    ok(hr == E_NOINTERFACE, "Failed: %#lx\n", hr);

    hr = IDirectMusicAudioPath_GetObjectInPath(dmap, DMUS_PCHANNEL_ALL, DMUS_PATH_BUFFER, buffer, &GUID_NULL,
                0, &IID_IKsPropertySet, (void**)&propset);
    todo_wine ok(hr == S_OK, "Failed: %#lx\n", hr);
    if (propset)
        IKsPropertySet_Release(propset);

    hr = IDirectMusicAudioPath_GetObjectInPath(dmap, DMUS_PCHANNEL_ALL, DMUS_PATH_BUFFER, buffer, &GUID_NULL,
                0, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "Failed: %#lx\n", hr);
    IUnknown_Release(unk);

    hr = IDirectMusicAudioPath_GetObjectInPath(dmap, DMUS_PCHANNEL_ALL, DMUS_PATH_BUFFER, buffer, &GUID_NULL,
                0, &GUID_NULL, (void**)&unk);
    ok(hr == E_NOINTERFACE, "Failed: %#lx\n", hr);

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
    hr = CoCreateInstance(&CLSID_DirectMusicAudioPathConfig, (IUnknown *)0xdeadbeef, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dmap);
    if (hr == REGDB_E_CLASSNOTREG) {
        win_skip("DirectMusicAudioPathConfig not registered\n");
        return;
    }
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicAudioPathConfig create failed: %#lx, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dmap, "dmap = %p\n", dmap);

    /* IDirectMusicAudioPath not supported */
    hr = CoCreateInstance(&CLSID_DirectMusicAudioPathConfig, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicAudioPath, (void**)&dmap);
    ok(FAILED(hr) && !dmap,
            "Unexpected IDirectMusicAudioPath interface: hr=%#lx, iface=%p\n", hr, dmap);

    /* IDirectMusicObject and IPersistStream supported */
    hr = CoCreateInstance(&CLSID_DirectMusicAudioPathConfig, NULL, CLSCTX_INPROC_SERVER,
            &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "DirectMusicObject create failed: %#lx, expected S_OK\n", hr);
    IPersistStream_Release(ps);
    hr = CoCreateInstance(&CLSID_DirectMusicAudioPathConfig, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void**)&dmo);
    ok(hr == S_OK, "DirectMusicObject create failed: %#lx, expected S_OK\n", hr);

    /* Same refcount for all DirectMusicObject interfaces */
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

    /* IDirectMusicAudioPath still not supported */
    hr = IDirectMusicObject_QueryInterface(dmo, &IID_IDirectMusicAudioPath, (void**)&dmap);
    ok(FAILED(hr) && !dmap,
        "Unexpected IDirectMusicAudioPath interface: hr=%#lx, iface=%p\n", hr, dmap);

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
    hr = CoCreateInstance(&CLSID_DirectMusicGraph, (IUnknown *)0xdeadbeef, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dmg);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicGraph create failed: %#lx, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dmg, "dmg = %p\n", dmg);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IClassFactory,
            (void**)&dmg);
    ok(hr == E_NOINTERFACE, "DirectMusicGraph create failed: %#lx, expected E_NOINTERFACE\n", hr);

    /* Same refcount for all DirectMusicGraph interfaces */
    hr = CoCreateInstance(&CLSID_DirectMusicGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicGraph, (void**)&dmg);
    ok(hr == S_OK, "DirectMusicGraph create failed: %#lx, expected S_OK\n", hr);
    refcount = IDirectMusicGraph_AddRef(dmg);
    ok(refcount == 2, "refcount == %lu, expected 2\n", refcount);

    hr = IDirectMusicGraph_QueryInterface(dmg, &IID_IDirectMusicObject, (void**)&dmo);
    if (hr == E_NOINTERFACE) {
        win_skip("DirectMusicGraph without IDirectMusicObject\n");
        return;
    }
    ok(hr == S_OK, "QueryInterface for IID_IDirectMusicObject failed: %#lx\n", hr);
    refcount = IDirectMusicObject_AddRef(dmo);
    ok(refcount == 4, "refcount == %lu, expected 4\n", refcount);
    refcount = IDirectMusicObject_Release(dmo);

    hr = IDirectMusicGraph_QueryInterface(dmg, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %#lx\n", hr);
    refcount = IPersistStream_AddRef(ps);
    ok(refcount == 5, "refcount == %lu, expected 5\n", refcount);
    refcount = IPersistStream_Release(ps);

    hr = IDirectMusicGraph_QueryInterface(dmg, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %#lx\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 6, "refcount == %lu, expected 6\n", refcount);
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
    hr = CoCreateInstance(&CLSID_DirectMusicSegment, (IUnknown *)0xdeadbeef, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dms);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicSegment create failed: %#lx, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dms, "dms = %p\n", dms);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicSegment, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectSound, (void**)&dms);
    ok(hr == E_NOINTERFACE, "DirectMusicSegment create failed: %#lx, expected E_NOINTERFACE\n", hr);

    /* Same refcount */
    hr = CoCreateInstance(&CLSID_DirectMusicSegment, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicSegment8, (void**)&dms);
    if (hr == E_NOINTERFACE) {
        win_skip("DirectMusicSegment without IDirectMusicSegment8\n");
        return;
    }
    ok(hr == S_OK, "DirectMusicSegment create failed: %#lx, expected S_OK\n", hr);
    refcount = IDirectMusicSegment8_AddRef(dms);
    ok (refcount == 2, "refcount == %lu, expected 2\n", refcount);
    hr = IDirectMusicSegment8_QueryInterface(dms, &IID_IDirectMusicObject, (void**)&dmo);
    ok(hr == S_OK, "QueryInterface for IID_IDirectMusicObject failed: %#lx\n", hr);
    IDirectMusicSegment8_AddRef(dms);
    refcount = IDirectMusicSegment8_Release(dms);
    ok (refcount == 3, "refcount == %lu, expected 3\n", refcount);
    hr = IDirectMusicSegment8_QueryInterface(dms, &IID_IPersistStream, (void**)&stream);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %#lx\n", hr);
    refcount = IDirectMusicSegment8_Release(dms);
    ok (refcount == 3, "refcount == %lu, expected 3\n", refcount);
    hr = IDirectMusicSegment8_QueryInterface(dms, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %#lx\n", hr);
    refcount = IUnknown_Release(unk);
    ok (refcount == 3, "refcount == %lu, expected 3\n", refcount);
    refcount = IDirectMusicObject_Release(dmo);
    ok (refcount == 2, "refcount == %lu, expected 2\n", refcount);
    refcount = IPersistStream_Release(stream);
    ok (refcount == 1, "refcount == %lu, expected 1\n", refcount);
    refcount = IDirectMusicSegment8_Release(dms);
    ok (refcount == 0, "refcount == %lu, expected 0\n", refcount);
}

static void test_COM_segmentstate(void)
{
    IDirectMusicSegmentState8 *dmss8 = (IDirectMusicSegmentState8*)0xdeadbeef;
    IUnknown *unk;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectMusicSegmentState, (IUnknown *)0xdeadbeef, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dmss8);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicSegmentState8 create failed: %#lx, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dmss8, "dmss8 = %p\n", dmss8);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicSegmentState, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void**)&dmss8);
    ok(hr == E_NOINTERFACE, "DirectMusicSegmentState8 create failed: %#lx, expected E_NOINTERFACE\n", hr);

    /* Same refcount for all DirectMusicSegmentState interfaces */
    hr = CoCreateInstance(&CLSID_DirectMusicSegmentState, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicSegmentState8, (void**)&dmss8);
    if (hr == E_NOINTERFACE) {
        win_skip("DirectMusicSegmentState without IDirectMusicSegmentState8\n");
        return;
    }
    ok(hr == S_OK, "DirectMusicSegmentState8 create failed: %#lx, expected S_OK\n", hr);
    refcount = IDirectMusicSegmentState8_AddRef(dmss8);
    ok(refcount == 2, "refcount == %lu, expected 2\n", refcount);

    hr = IDirectMusicSegmentState8_QueryInterface(dmss8, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %#lx\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 4, "refcount == %lu, expected 4\n", refcount);
    refcount = IUnknown_Release(unk);

    hr = IDirectMusicSegmentState8_QueryInterface(dmss8, &IID_IUnknown, NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);

    while (IDirectMusicSegmentState8_Release(dmss8));
}

static void test_COM_track(void)
{
    IDirectMusicTrack *dmt;
    IDirectMusicTrack8 *dmt8;
    IPersistStream *ps;
    IUnknown *unk;
    ULONG refcount;
    HRESULT hr;
#define X(class)        &CLSID_ ## class, #class
    const struct {
        REFCLSID clsid;
        const char *name;
        BOOL has_dmt8;
    } class[] = {
        { X(DirectMusicLyricsTrack), TRUE },
        { X(DirectMusicMarkerTrack), FALSE },
        { X(DirectMusicParamControlTrack), TRUE },
        { X(DirectMusicSegmentTriggerTrack), TRUE },
        { X(DirectMusicSeqTrack), TRUE },
        { X(DirectMusicSysExTrack), TRUE },
        { X(DirectMusicTempoTrack), TRUE },
        { X(DirectMusicTimeSigTrack), FALSE },
        { X(DirectMusicWaveTrack), TRUE }
    };
#undef X
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(class); i++) {
        trace("Testing %s\n", class[i].name);
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
        hr = CoCreateInstance(class[i].clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicTrack,
                (void**)&dmt);
        ok(hr == S_OK, "%s create failed: %#lx, expected S_OK\n", class[i].name, hr);
        refcount = IDirectMusicTrack_AddRef(dmt);
        ok(refcount == 2, "refcount == %lu, expected 2\n", refcount);

        hr = IDirectMusicTrack_QueryInterface(dmt, &IID_IPersistStream, (void**)&ps);
        ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %#lx\n", hr);
        refcount = IPersistStream_AddRef(ps);
        ok(refcount == 4, "refcount == %lu, expected 4\n", refcount);
        IPersistStream_Release(ps);

        hr = IDirectMusicTrack_QueryInterface(dmt, &IID_IUnknown, (void**)&unk);
        ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %#lx\n", hr);
        refcount = IUnknown_AddRef(unk);
        ok(refcount == 5, "refcount == %lu, expected 5\n", refcount);
        refcount = IUnknown_Release(unk);

        hr = IDirectMusicTrack_QueryInterface(dmt, &IID_IDirectMusicTrack8, (void**)&dmt8);
        if (class[i].has_dmt8) {
            ok(hr == S_OK, "QueryInterface for IID_IDirectMusicTrack8 failed: %#lx\n", hr);
            refcount = IDirectMusicTrack8_AddRef(dmt8);
            ok(refcount == 6, "refcount == %lu, expected 6\n", refcount);
            refcount = IDirectMusicTrack8_Release(dmt8);
        } else {
            ok(hr == E_NOINTERFACE, "QueryInterface for IID_IDirectMusicTrack8 failed: %#lx\n", hr);
            refcount = IDirectMusicTrack_AddRef(dmt);
            ok(refcount == 5, "refcount == %lu, expected 5\n", refcount);
        }

        while (IDirectMusicTrack_Release(dmt));
    }
}

static void test_COM_performance(void)
{
    IDirectMusicPerformance *dmp = (IDirectMusicPerformance*)0xdeadbeef;
    IDirectMusicPerformance *dmp2;
    IDirectMusicPerformance8 *dmp8;
    IDirectMusicAudioPath *dmap = NULL;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, (IUnknown *)0xdeadbeef, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dmp);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectMusicPerformance create failed: %#lx, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dmp, "dmp = %p\n", dmp);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void**)&dmp);
    ok(hr == E_NOINTERFACE, "DirectMusicPerformance create failed: %#lx, expected E_NOINTERFACE\n", hr);

    /* Same refcount */
    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicPerformance, (void**)&dmp);
    ok(hr == S_OK, "DirectMusicPerformance create failed: %#lx, expected S_OK\n", hr);
    refcount = IDirectMusicPerformance_AddRef(dmp);
    ok (refcount == 2, "refcount == %lu, expected 2\n", refcount);
    hr = IDirectMusicPerformance_QueryInterface(dmp, &IID_IDirectMusicPerformance2, (void**)&dmp2);
    ok(hr == S_OK, "QueryInterface for IID_IDirectMusicPerformance2 failed: %#lx\n", hr);
    IDirectMusicPerformance_AddRef(dmp);
    refcount = IDirectMusicPerformance_Release(dmp);
    ok (refcount == 3, "refcount == %lu, expected 3\n", refcount);
    hr = IDirectMusicPerformance_QueryInterface(dmp, &IID_IDirectMusicPerformance8, (void**)&dmp8);
    ok(hr == S_OK, "QueryInterface for IID_IDirectMusicPerformance8 failed: %#lx\n", hr);
    hr = IDirectMusicPerformance8_CreateAudioPath(dmp8, NULL, TRUE, &dmap);
    ok(hr == E_POINTER, "Unexpected result from CreateAudioPath: %#lx\n", hr);
    ok(dmap == NULL, "Unexpected dmap pointer\n");
    refcount = IDirectMusicPerformance_Release(dmp);
    ok (refcount == 3, "refcount == %lu, expected 3\n", refcount);
    refcount = IDirectMusicPerformance8_Release(dmp8);
    ok (refcount == 2, "refcount == %lu, expected 2\n", refcount);
    refcount = IDirectMusicPerformance_Release(dmp2);
    ok (refcount == 1, "refcount == %lu, expected 1\n", refcount);
    refcount = IDirectMusicPerformance_Release(dmp);
    ok (refcount == 0, "refcount == %lu, expected 0\n", refcount);
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
    if (hr == REGDB_E_CLASSNOTREG) {
        win_skip("DirectMusicAudioPathConfig not registered\n");
        return;
    }
    ok(hr == S_OK, "DirectMusicAudioPathConfig create failed: %#lx, expected S_OK\n", hr);

    /* IPersistStream */
    hr = IDirectMusicObject_QueryInterface(dmo, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %#lx\n", hr);
    hr = IPersistStream_GetClassID(ps, &class);
    ok(hr == S_OK, "IPersistStream_GetClassID failed: %#lx\n", hr);
    ok(IsEqualGUID(&class, &CLSID_DirectMusicAudioPathConfig),
            "Expected class CLSID_DirectMusicAudioPathConfig got %s\n", wine_dbgstr_guid(&class));

    /* Unimplemented IPersistStream methods */
    hr = IPersistStream_IsDirty(ps);
    ok(hr == S_FALSE, "IPersistStream_IsDirty failed: %#lx\n", hr);
    hr = IPersistStream_GetSizeMax(ps, &size);
    ok(hr == E_NOTIMPL, "IPersistStream_GetSizeMax failed: %#lx\n", hr);
    hr = IPersistStream_Save(ps, NULL, TRUE);
    ok(hr == E_NOTIMPL, "IPersistStream_Save failed: %#lx\n", hr);

    while (IDirectMusicObject_Release(dmo));
}

static void test_graph(void)
{
    IDirectMusicTool *tool1, *tool2, *tmp_tool;
    IDirectMusicGraph *graph, *tmp_graph;
    IPersistStream *ps;
    CLSID class = { 0 };
    ULARGE_INTEGER size;
    DMUS_PMSG msg;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectMusicGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicGraph, (void**)&graph);
    ok(hr == S_OK, "DirectMusicGraph create failed: %#lx, expected S_OK\n", hr);

    /* IPersistStream */
    hr = IDirectMusicGraph_QueryInterface(graph, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %#lx\n", hr);
    hr = IPersistStream_GetClassID(ps, &class);
    ok(hr == S_OK || broken(hr == E_NOTIMPL) /* win2k */, "IPersistStream_GetClassID failed: %#lx\n", hr);
    if (hr == S_OK)
        ok(IsEqualGUID(&class, &CLSID_DirectMusicGraph),
                "Expected class CLSID_DirectMusicGraph got %s\n", wine_dbgstr_guid(&class));

    /* Unimplemented IPersistStream methods */
    hr = IPersistStream_IsDirty(ps);
    ok(hr == S_FALSE, "IPersistStream_IsDirty failed: %#lx\n", hr);
    hr = IPersistStream_GetSizeMax(ps, &size);
    ok(hr == E_NOTIMPL, "IPersistStream_GetSizeMax failed: %#lx\n", hr);
    hr = IPersistStream_Save(ps, NULL, TRUE);
    ok(hr == E_NOTIMPL, "IPersistStream_Save failed: %#lx\n", hr);

    IDirectMusicGraph_Release(graph);


    hr = test_tool_create(NULL, 0, &tool1);
    ok(hr == S_OK, "got %#lx\n", hr);
    trace("created tool1 %p\n", tool1);
    hr = test_tool_create(NULL, 0, &tool2);
    ok(hr == S_OK, "got %#lx\n", hr);
    trace("created tool2 %p\n", tool2);

    hr = CoCreateInstance(&CLSID_DirectMusicGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicGraph, (void **)&graph);
    ok(hr == S_OK, "got %#lx\n", hr);


    hr = IDirectMusicGraph_InsertTool(graph, NULL, NULL, 0, -1);
    ok(hr == E_POINTER, "got %#lx\n", hr);

    /* InsertTool initializes the tool */
    hr = IDirectMusicGraph_InsertTool(graph, tool1, NULL, 0, -1);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = test_tool_get_graph(tool1, &tmp_graph);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(graph == tmp_graph, "got %#lx\n", hr);
    IDirectMusicGraph_Release(tmp_graph);

    hr = IDirectMusicGraph_InsertTool(graph, tool2, NULL, 0, 1);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicGraph_GetTool(graph, 0, NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);
    hr = IDirectMusicGraph_GetTool(graph, 0, &tmp_tool);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(tool1 == tmp_tool, "got %p\n", tmp_tool);
    if (hr == S_OK) IDirectMusicTool_Release(tmp_tool);
    hr = IDirectMusicGraph_GetTool(graph, 1, &tmp_tool);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(tool2 == tmp_tool, "got %p\n", tmp_tool);
    if (hr == S_OK) IDirectMusicTool_Release(tmp_tool);
    hr = IDirectMusicGraph_GetTool(graph, 2, &tmp_tool);
    ok(hr == DMUS_E_NOT_FOUND, "got %#lx\n", hr);

    /* cannot insert the tool twice */
    hr = IDirectMusicGraph_InsertTool(graph, tool1, NULL, 0, -1);
    ok(hr == DMUS_E_ALREADY_EXISTS, "got %#lx\n", hr);

    /* test removing the first tool */
    hr = IDirectMusicGraph_RemoveTool(graph, NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);
    hr = IDirectMusicGraph_RemoveTool(graph, tool1);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicGraph_RemoveTool(graph, tool1);
    ok(hr == DMUS_E_NOT_FOUND, "got %#lx\n", hr);

    hr = IDirectMusicGraph_GetTool(graph, 0, &tmp_tool);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(tool2 == tmp_tool, "got %p\n", tmp_tool);
    if (hr == S_OK) IDirectMusicTool_Release(tmp_tool);
    hr = IDirectMusicGraph_GetTool(graph, 1, &tmp_tool);
    ok(hr == DMUS_E_NOT_FOUND, "got %#lx\n", hr);

    hr = IDirectMusicGraph_InsertTool(graph, tool1, NULL, 0, -1);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicGraph_GetTool(graph, 0, &tmp_tool);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(tool1 == tmp_tool, "got %p\n", tmp_tool);
    if (hr == S_OK) IDirectMusicTool_Release(tmp_tool);


    /* Test basic IDirectMusicGraph_StampPMsg usage */
    hr = IDirectMusicGraph_StampPMsg(graph, NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);
    memset(&msg, 0, sizeof(msg));
    hr = IDirectMusicGraph_StampPMsg(graph, &msg);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(msg.pGraph == graph, "got %p\n", msg.pGraph);
    ok(msg.pTool == tool1, "got %p\n", msg.pTool);

    ok(!msg.dwSize, "got %ld\n", msg.dwSize);
    ok(!msg.rtTime, "got %I64d\n", msg.rtTime);
    ok(!msg.mtTime, "got %ld\n", msg.mtTime);
    ok(msg.dwFlags == DMUS_PMSGF_TOOL_IMMEDIATE, "got %#lx\n", msg.dwFlags);
    ok(!msg.dwPChannel, "got %ld\n", msg.dwPChannel);
    ok(!msg.dwVirtualTrackID, "got %ld\n", msg.dwVirtualTrackID);
    ok(!msg.dwType, "got %#lx\n", msg.dwType);
    ok(!msg.dwVoiceID, "got %ld\n", msg.dwVoiceID);
    ok(!msg.dwGroupID, "got %ld\n", msg.dwGroupID);
    ok(!msg.punkUser, "got %p\n", msg.punkUser);

    hr = IDirectMusicGraph_StampPMsg(graph, &msg);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(msg.pGraph == graph, "got %p\n", msg.pGraph);
    ok(msg.pTool == tool2, "got %p\n", msg.pTool);
    hr = IDirectMusicGraph_StampPMsg(graph, &msg);
    ok(hr == DMUS_S_LAST_TOOL, "got %#lx\n", hr);
    ok(msg.pGraph == graph, "got %p\n", msg.pGraph);
    ok(!msg.pTool, "got %p\n", msg.pTool);
    hr = IDirectMusicGraph_StampPMsg(graph, &msg);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(msg.pGraph == graph, "got %p\n", msg.pGraph);
    ok(msg.pTool == tool1, "got %p\n", msg.pTool);
    IDirectMusicGraph_Release(msg.pGraph);
    msg.pGraph = NULL;
    IDirectMusicGraph_Release(msg.pTool);
    msg.pTool = NULL;


    /* test StampPMsg with the wrong graph or innexistant tools */
    hr = CoCreateInstance(&CLSID_DirectMusicGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicGraph, (void **)&tmp_graph);
    ok(hr == S_OK, "got %#lx\n", hr);

    msg.pGraph = tmp_graph;
    IDirectMusicGraph_AddRef(msg.pGraph);
    msg.pTool = tool1;
    IDirectMusicTool_AddRef(msg.pTool);
    hr = IDirectMusicGraph_StampPMsg(graph, &msg);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(msg.pGraph == tmp_graph, "got %p\n", msg.pGraph);
    ok(msg.pTool == tool2, "got %p\n", msg.pTool);
    IDirectMusicGraph_Release(msg.pGraph);
    msg.pGraph = NULL;

    msg.pGraph = graph;
    IDirectMusicGraph_AddRef(msg.pGraph);
    hr = IDirectMusicGraph_StampPMsg(tmp_graph, &msg);
    ok(hr == DMUS_S_LAST_TOOL, "got %#lx\n", hr);
    ok(msg.pGraph == graph, "got %p\n", msg.pGraph);
    ok(msg.pTool == NULL, "got %p\n", msg.pTool);

    msg.pTool = tool2;
    IDirectMusicTool_AddRef(msg.pTool);
    hr = IDirectMusicGraph_InsertTool(tmp_graph, tool1, NULL, 0, 0);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicGraph_InsertTool(tmp_graph, tool2, NULL, 0, 0);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicGraph_StampPMsg(tmp_graph, &msg);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(msg.pGraph == graph, "got %p\n", msg.pGraph);
    ok(msg.pTool == tool1, "got %p\n", msg.pTool);
    IDirectMusicGraph_Release(msg.pGraph);
    msg.pGraph = NULL;

    hr = IDirectMusicGraph_RemoveTool(graph, tool1);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicGraph_StampPMsg(tmp_graph, &msg);
    ok(hr == DMUS_S_LAST_TOOL, "got %#lx\n", hr);
    ok(msg.pGraph == NULL, "got %p\n", msg.pGraph);
    ok(msg.pTool == NULL, "got %p\n", msg.pTool);

    IDirectMusicGraph_Release(tmp_graph);


    IDirectMusicGraph_Release(graph);
    IDirectMusicTool_Release(tool2);
    IDirectMusicTool_Release(tool1);
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
    ok(hr == S_OK, "DirectMusicSegment create failed: %#lx, expected S_OK\n", hr);

    /* IPersistStream */
    hr = IDirectMusicSegment_QueryInterface(dms, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %#lx\n", hr);
    hr = IPersistStream_GetClassID(ps, &class);
    ok(hr == S_OK || broken(hr == E_NOTIMPL) /* win2k */, "IPersistStream_GetClassID failed: %#lx\n", hr);
    if (hr == S_OK)
        ok(IsEqualGUID(&class, &CLSID_DirectMusicSegment),
                "Expected class CLSID_DirectMusicSegment got %s\n", wine_dbgstr_guid(&class));

    /* Unimplemented IPersistStream methods */
    hr = IPersistStream_IsDirty(ps);
    ok(hr == S_FALSE, "IPersistStream_IsDirty failed: %#lx\n", hr);
    hr = IPersistStream_GetSizeMax(ps, &size);
    ok(hr == E_NOTIMPL, "IPersistStream_GetSizeMax failed: %#lx\n", hr);
    hr = IPersistStream_Save(ps, NULL, TRUE);
    ok(hr == E_NOTIMPL, "IPersistStream_Save failed: %#lx\n", hr);

    while (IDirectMusicSegment_Release(dms));
}

static void _add_track(IDirectMusicSegment8 *seg, REFCLSID class, const char *name, DWORD group)
{
    IDirectMusicTrack *track;
    HRESULT hr;

    hr = CoCreateInstance(class, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicTrack,
            (void**)&track);
    ok(hr == S_OK, "%s create failed: %#lx, expected S_OK\n", name, hr);
    hr = IDirectMusicSegment8_InsertTrack(seg, track, group);
    if (group)
        ok(hr == S_OK, "Inserting %s failed: %#lx, expected S_OK\n", name, hr);
    else
        ok(hr == E_INVALIDARG, "Inserting %s failed: %#lx, expected E_INVALIDARG\n", name, hr);
    IDirectMusicTrack_Release(track);
}

#define add_track(seg, class, group) _add_track(seg, &CLSID_DirectMusic ## class, #class, group)

static void _expect_track(IDirectMusicSegment8 *seg, REFCLSID expect, const char *name, DWORD group,
        DWORD index, BOOL ignore_guid)
{
    IDirectMusicTrack *track;
    IPersistStream *ps;
    CLSID class;
    HRESULT hr;

    if (ignore_guid)
        hr = IDirectMusicSegment8_GetTrack(seg, &GUID_NULL, group, index, &track);
    else
        hr = IDirectMusicSegment8_GetTrack(seg, expect, group, index, &track);
    if (!expect) {
        ok(hr == DMUS_E_NOT_FOUND, "GetTrack failed: %#lx, expected DMUS_E_NOT_FOUND\n", hr);
        return;
    }

    ok(hr == S_OK, "GetTrack failed: %#lx, expected S_OK\n", hr);
    hr = IDirectMusicTrack_QueryInterface(track, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %#lx\n", hr);
    hr = IPersistStream_GetClassID(ps, &class);
    ok(hr == S_OK, "IPersistStream_GetClassID failed: %#lx\n", hr);
    ok(IsEqualGUID(&class, expect), "For group %#lx index %lu: Expected class %s got %s\n",
            group, index, name, wine_dbgstr_guid(&class));

    IPersistStream_Release(ps);
    IDirectMusicTrack_Release(track);
}

#define expect_track(seg, class, group, index) \
    _expect_track(seg, &CLSID_DirectMusic ## class, #class, group, index, TRUE)
#define expect_guid_track(seg, class, group, index) \
    _expect_track(seg, &CLSID_DirectMusic ## class, #class, group, index, FALSE)

static void test_gettrack(void)
{
    IDirectMusicSegment8 *seg;
    IDirectMusicTrack *track;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectMusicSegment, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicSegment8, (void**)&seg);
    ok(hr == S_OK, "DirectMusicSegment create failed: %#lx, expected S_OK\n", hr);

    add_track(seg, LyricsTrack, 0x0);         /* failure */
    add_track(seg, LyricsTrack, 0x1);         /* idx 0 group 1 */
    add_track(seg, ParamControlTrack, 0x3);   /* idx 1 group 1, idx 0 group 2 */
    add_track(seg, SegmentTriggerTrack, 0x2); /* idx 1 group 2 */
    add_track(seg, SeqTrack, 0x1);            /* idx 2 group 1 */
    add_track(seg, TempoTrack, 0x7);          /* idx 3 group 1, idx 2 group 2, idx 0 group 3 */
    add_track(seg, WaveTrack, 0xffffffff);    /* idx 4 group 1, idx 3 group 2, idx 1 group 3 */

    /* Ignore GUID in GetTrack */
    hr = IDirectMusicSegment8_GetTrack(seg, &GUID_NULL, 0, 0, &track);
    ok(hr == DMUS_E_NOT_FOUND, "GetTrack failed: %#lx, expected DMUS_E_NOT_FOUND\n", hr);

    expect_track(seg, LyricsTrack, 0x1, 0);
    expect_track(seg, ParamControlTrack, 0x1, 1);
    expect_track(seg, SeqTrack, 0x1, 2);
    expect_track(seg, TempoTrack, 0x1, 3);
    expect_track(seg, WaveTrack, 0x1, 4);
    _expect_track(seg, NULL, "", 0x1, 5, TRUE);
    _expect_track(seg, NULL, "", 0x1, DMUS_SEG_ANYTRACK, TRUE);
    expect_track(seg, ParamControlTrack, 0x2, 0);
    expect_track(seg, WaveTrack, 0x80000000, 0);
    expect_track(seg, SegmentTriggerTrack, 0x3, 2);  /* groups 1+2 combined index */
    expect_track(seg, SeqTrack, 0x3, 3);             /* groups 1+2 combined index */
    expect_track(seg, TempoTrack, 0x7, 4);           /* groups 1+2+3 combined index */
    expect_track(seg, TempoTrack, 0xffffffff, 4);    /* all groups combined index */
    _expect_track(seg, NULL, "", 0xffffffff, DMUS_SEG_ANYTRACK, TRUE);

    /* Use the GUID in GetTrack */
    hr = IDirectMusicSegment8_GetTrack(seg, &CLSID_DirectMusicLyricsTrack, 0, 0, &track);
    ok(hr == DMUS_E_NOT_FOUND, "GetTrack failed: %#lx, expected DMUS_E_NOT_FOUND\n", hr);

    expect_guid_track(seg, LyricsTrack, 0x1, 0);
    expect_guid_track(seg, ParamControlTrack, 0x1, 0);
    expect_guid_track(seg, SeqTrack, 0x1, 0);
    expect_guid_track(seg, TempoTrack, 0x1, 0);
    expect_guid_track(seg, ParamControlTrack, 0x2, 0);
    expect_guid_track(seg, SegmentTriggerTrack, 0x3, 0);
    expect_guid_track(seg, SeqTrack, 0x3, 0);
    expect_guid_track(seg, TempoTrack, 0x7, 0);
    expect_guid_track(seg, TempoTrack, 0xffffffff, 0);

    IDirectMusicSegment8_Release(seg);
}

static void test_segment_param(void)
{
    IDirectMusicSegment8 *seg;
    char buf[64];
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectMusicSegment, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicSegment8, (void **)&seg);
    ok(hr == S_OK, "DirectMusicSegment create failed: %#lx, expected S_OK\n", hr);

    add_track(seg, LyricsTrack, 0x1);         /* no params */
    add_track(seg, SegmentTriggerTrack, 0x1); /* all params "supported" */

    hr = IDirectMusicSegment8_GetParam(seg, NULL, 0x1, 0, 0, NULL, buf);
    ok(hr == E_POINTER, "GetParam failed: %#lx, expected E_POINTER\n", hr);
    hr = IDirectMusicSegment8_SetParam(seg, NULL, 0x1, 0, 0, buf);
    todo_wine ok(hr == E_POINTER, "SetParam failed: %#lx, expected E_POINTER\n", hr);

    hr = IDirectMusicSegment8_GetParam(seg, &GUID_Valid_Start_Time, 0x1, 0, 0, NULL, buf);
    ok(hr == DMUS_E_GET_UNSUPPORTED, "GetParam failed: %#lx, expected DMUS_E_GET_UNSUPPORTED\n", hr);
    hr = IDirectMusicSegment8_GetParam(seg, &GUID_Valid_Start_Time, 0x1, 1, 0, NULL, buf);
    ok(hr == DMUS_E_TRACK_NOT_FOUND, "GetParam failed: %#lx, expected DMUS_E_TRACK_NOT_FOUND\n", hr);
    hr = IDirectMusicSegment8_GetParam(seg, &GUID_Valid_Start_Time, 0x1, DMUS_SEG_ANYTRACK, 0,
            NULL, buf);
    ok(hr == DMUS_E_GET_UNSUPPORTED, "GetParam failed: %#lx, expected DMUS_E_GET_UNSUPPORTED\n", hr);

    hr = IDirectMusicSegment8_SetParam(seg, &GUID_Valid_Start_Time, 0x1, 0, 0, buf);
    ok(hr == S_OK, "SetParam failed: %#lx, expected S_OK\n", hr);
    hr = IDirectMusicSegment8_SetParam(seg, &GUID_Valid_Start_Time, 0x1, 1, 0, buf);
    todo_wine ok(hr == DMUS_E_TRACK_NOT_FOUND,
            "SetParam failed: %#lx, expected DMUS_E_TRACK_NOT_FOUND\n", hr);
    hr = IDirectMusicSegment8_SetParam(seg, &GUID_Valid_Start_Time, 0x1, DMUS_SEG_ALLTRACKS,
            0, buf);
    ok(hr == S_OK, "SetParam failed: %#lx, expected S_OK\n", hr);

    IDirectMusicSegment8_Release(seg);
}

static void expect_getparam(IDirectMusicTrack *track, REFGUID type, const char *name,
        HRESULT expect)
{
    HRESULT hr;
    char buf[64] = { 0 };

    hr = IDirectMusicTrack_GetParam(track, type, 0, NULL, buf);
    ok(hr == expect, "GetParam(%s) failed: %#lx, expected %#lx\n", name, hr, expect);
}

static void expect_setparam(IDirectMusicTrack *track, REFGUID type, const char *name,
        HRESULT expect)
{
    HRESULT hr;
    char buf[64] = { 0 };

    hr = IDirectMusicTrack_SetParam(track, type, 0, buf);
    ok(hr == expect, "SetParam(%s) failed: %#lx, expected %#lx\n", name, hr, expect);
}

static void test_track(void)
{
    IDirectMusicTrack *dmt;
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
        { X(GUID_Variations) },
        { X(GUID_NULL) }
    };
#undef X
#define X(class)        &CLSID_ ## class, #class
    const struct {
        REFCLSID clsid;
        const char *name;
        /* bitfield with supported param types */
        unsigned int has_params;
    } class[] = {
        { X(DirectMusicLyricsTrack), 0 },
        { X(DirectMusicMarkerTrack), 0x8080000 },
        { X(DirectMusicParamControlTrack), 0 },
        { X(DirectMusicSegmentTriggerTrack), 0x3fffffff },
        { X(DirectMusicSeqTrack), ~0 },         /* param methods not implemented */
        { X(DirectMusicSysExTrack), ~0 },       /* param methods not implemented */
        { X(DirectMusicTempoTrack), 0x802100 },
        { X(DirectMusicTimeSigTrack), 0x1004200 },
        { X(DirectMusicWaveTrack), 0x6001c80 }
    };
#undef X
    unsigned int i, j;

    for (i = 0; i < ARRAY_SIZE(class); i++) {
        trace("Testing %s\n", class[i].name);
        hr = CoCreateInstance(class[i].clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicTrack,
                (void**)&dmt);
        ok(hr == S_OK, "%s create failed: %#lx, expected S_OK\n", class[i].name, hr);

        /* IDirectMusicTrack */
        if (class[i].has_params != ~0) {
            for (j = 0; j < ARRAY_SIZE(param_types); j++) {
                hr = IDirectMusicTrack_IsParamSupported(dmt, param_types[j].type);
                if (class[i].has_params & (1 << j)) {
                    ok(hr == S_OK, "IsParamSupported(%s) failed: %#lx, expected S_OK\n",
                            param_types[j].name, hr);
                    if (class[i].clsid == &CLSID_DirectMusicSegmentTriggerTrack) {
                        expect_getparam(dmt, param_types[j].type, param_types[j].name,
                                DMUS_E_GET_UNSUPPORTED);
                        expect_setparam(dmt, param_types[j].type, param_types[j].name, S_OK);
                    } else if (class[i].clsid == &CLSID_DirectMusicMarkerTrack)
                        expect_setparam(dmt, param_types[j].type, param_types[j].name,
                                DMUS_E_SET_UNSUPPORTED);
                    else if (class[i].clsid == &CLSID_DirectMusicWaveTrack)
                        expect_getparam(dmt, param_types[j].type, param_types[j].name,
                                DMUS_E_GET_UNSUPPORTED);
                } else {
                    ok(hr == DMUS_E_TYPE_UNSUPPORTED,
                            "IsParamSupported(%s) failed: %#lx, expected DMUS_E_TYPE_UNSUPPORTED\n",
                            param_types[j].name, hr);
                    expect_getparam(dmt, param_types[j].type, param_types[j].name,
                            DMUS_E_GET_UNSUPPORTED);
                    if (class[i].clsid == &CLSID_DirectMusicWaveTrack)
                        expect_setparam(dmt, param_types[j].type, param_types[j].name,
                                DMUS_E_TYPE_UNSUPPORTED);
                    else
                        expect_setparam(dmt, param_types[j].type, param_types[j].name,
                                DMUS_E_SET_UNSUPPORTED);
                }

                /* GetParam / SetParam for IsParamSupported supported types */
                if (class[i].clsid == &CLSID_DirectMusicTimeSigTrack) {
                    expect_getparam(dmt, &GUID_DisableTimeSig, "GUID_DisableTimeSig",
                                DMUS_E_GET_UNSUPPORTED);
                    expect_getparam(dmt, &GUID_EnableTimeSig, "GUID_EnableTimeSig",
                                DMUS_E_GET_UNSUPPORTED);
                    expect_setparam(dmt, &GUID_TimeSignature, "GUID_TimeSignature",
                                DMUS_E_SET_UNSUPPORTED);
                } else if (class[i].clsid == &CLSID_DirectMusicTempoTrack) {
                    expect_getparam(dmt, &GUID_DisableTempo, "GUID_DisableTempo",
                                DMUS_E_GET_UNSUPPORTED);
                    expect_getparam(dmt, &GUID_EnableTempo, "GUID_EnableTempo",
                                DMUS_E_GET_UNSUPPORTED);
                }
            }
        } else {
            hr = IDirectMusicTrack_GetParam(dmt, NULL, 0, NULL, NULL);
            ok(hr == E_NOTIMPL, "IDirectMusicTrack_GetParam failed: %#lx\n", hr);
            hr = IDirectMusicTrack_SetParam(dmt, NULL, 0, NULL);
            ok(hr == E_NOTIMPL, "IDirectMusicTrack_SetParam failed: %#lx\n", hr);
            hr = IDirectMusicTrack_IsParamSupported(dmt, NULL);
            ok(hr == E_NOTIMPL, "IDirectMusicTrack_IsParamSupported failed: %#lx\n", hr);

            hr = IDirectMusicTrack_IsParamSupported(dmt, &GUID_IDirectMusicStyle);
            ok(hr == E_NOTIMPL, "got: %#lx\n", hr);
        }
        if (class[i].clsid != &CLSID_DirectMusicMarkerTrack &&
                class[i].clsid != &CLSID_DirectMusicTimeSigTrack) {
            hr = IDirectMusicTrack_AddNotificationType(dmt, NULL);
            ok(hr == E_NOTIMPL, "IDirectMusicTrack_AddNotificationType failed: %#lx\n", hr);
            hr = IDirectMusicTrack_RemoveNotificationType(dmt, NULL);
            ok(hr == E_NOTIMPL, "IDirectMusicTrack_RemoveNotificationType failed: %#lx\n", hr);
        }
        hr = IDirectMusicTrack_Clone(dmt, 0, 0, NULL);
        todo_wine ok(hr == E_POINTER, "IDirectMusicTrack_Clone failed: %#lx\n", hr);

        /* IDirectMusicTrack8 */
        hr = IDirectMusicTrack_QueryInterface(dmt, &IID_IDirectMusicTrack8, (void**)&dmt8);
        if (hr == S_OK) {
            hr = IDirectMusicTrack8_PlayEx(dmt8, NULL, 0, 0, 0, 0, NULL, NULL, 0);
            todo_wine ok(hr == E_POINTER, "IDirectMusicTrack8_PlayEx failed: %#lx\n", hr);
            if (class[i].has_params == ~0) {
                hr = IDirectMusicTrack8_GetParamEx(dmt8, NULL, 0, NULL, NULL, NULL, 0);
                ok(hr == E_NOTIMPL, "IDirectMusicTrack8_GetParamEx failed: %#lx\n", hr);
                hr = IDirectMusicTrack8_SetParamEx(dmt8, NULL, 0, NULL, NULL, 0);
                ok(hr == E_NOTIMPL, "IDirectMusicTrack8_SetParamEx failed: %#lx\n", hr);
            }
            hr = IDirectMusicTrack8_Compose(dmt8, NULL, 0, NULL);
            ok(hr == E_NOTIMPL, "IDirectMusicTrack8_Compose failed: %#lx\n", hr);
            hr = IDirectMusicTrack8_Join(dmt8, NULL, 0, NULL, 0, NULL);
            if (class[i].clsid == &CLSID_DirectMusicTempoTrack)
                todo_wine ok(hr == E_POINTER, "IDirectMusicTrack8_Join failed: %#lx\n", hr);
            else
                ok(hr == E_NOTIMPL, "IDirectMusicTrack8_Join failed: %#lx\n", hr);
            IDirectMusicTrack8_Release(dmt8);
        }

        /* IPersistStream */
        hr = IDirectMusicTrack_QueryInterface(dmt, &IID_IPersistStream, (void**)&ps);
        ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %#lx\n", hr);
        hr = IPersistStream_GetClassID(ps, &classid);
        ok(hr == S_OK, "IPersistStream_GetClassID failed: %#lx\n", hr);
        ok(IsEqualGUID(&classid, class[i].clsid),
                "Expected class %s got %s\n", class[i].name, wine_dbgstr_guid(&classid));
        hr = IPersistStream_IsDirty(ps);
        ok(hr == S_FALSE, "IPersistStream_IsDirty failed: %#lx\n", hr);

        /* Unimplemented IPersistStream methods */
        hr = IPersistStream_GetSizeMax(ps, &size);
        ok(hr == E_NOTIMPL, "IPersistStream_GetSizeMax failed: %#lx\n", hr);
        hr = IPersistStream_Save(ps, NULL, TRUE);
        ok(hr == E_NOTIMPL, "IPersistStream_Save failed: %#lx\n", hr);

        while (IDirectMusicTrack_Release(dmt));
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
    DMUS_OBJECTDESC desc;
    HRESULT hr;
    DWORD valid;
    unsigned int i;
    /* fourcc ~0 will be replaced later on */
    FOURCC alldesc[] =
    {
        FOURCC_RIFF, ~0, DMUS_FOURCC_CATEGORY_CHUNK, FOURCC_LIST, DMUS_FOURCC_UNFO_LIST,
        DMUS_FOURCC_UNAM_CHUNK, DMUS_FOURCC_UCOP_CHUNK, DMUS_FOURCC_UCMT_CHUNK,
        DMUS_FOURCC_USBJ_CHUNK, 0, DMUS_FOURCC_VERSION_CHUNK, DMUS_FOURCC_GUID_CHUNK, 0
    };
    FOURCC dupes[] =
    {
        FOURCC_RIFF, ~0, DMUS_FOURCC_CATEGORY_CHUNK, DMUS_FOURCC_CATEGORY_CHUNK,
        DMUS_FOURCC_VERSION_CHUNK, DMUS_FOURCC_VERSION_CHUNK, DMUS_FOURCC_GUID_CHUNK,
        DMUS_FOURCC_GUID_CHUNK, FOURCC_LIST, DMUS_FOURCC_UNFO_LIST, DMUS_FOURCC_UNAM_CHUNK, 0,
        FOURCC_LIST, DMUS_FOURCC_UNFO_LIST, mmioFOURCC('I','N','A','M'), 0, 0
    };
    FOURCC empty[] = {FOURCC_RIFF, ~0, 0};
    FOURCC inam[] = {FOURCC_RIFF, ~0, FOURCC_LIST, ~0, mmioFOURCC('I','N','A','M'), 0, 0};
    FOURCC noriff[] = {mmioFOURCC('J','U','N','K'), 0};
#define X(class)        &CLSID_ ## class, #class
#define Y(form)         form, #form
    const struct {
        REFCLSID clsid;
        const char *class;
        FOURCC form;
        const char *name;
        BOOL needs_size;
    } forms[] = {
        { X(DirectMusicSegment), Y(DMUS_FOURCC_SEGMENT_FORM), FALSE },
        { X(DirectMusicSegment), Y(mmioFOURCC('W','A','V','E')), FALSE },
        { X(DirectMusicAudioPathConfig), Y(DMUS_FOURCC_AUDIOPATH_FORM), TRUE },
        { X(DirectMusicGraph), Y(DMUS_FOURCC_TOOLGRAPH_FORM), TRUE },
    };
#undef X
#undef Y

    for (i = 0; i < ARRAY_SIZE(forms); i++) {
        trace("Testing %s / %s\n", forms[i].class, forms[i].name);
        hr = CoCreateInstance(forms[i].clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicObject,
                (void **)&dmo);
        if (hr != S_OK) {
            win_skip("Could not create %s object: %#lx\n", forms[i].class, hr);
            return;
        }

        /* Nothing loaded */
        memset(&desc, 0, sizeof(desc));
        hr = IDirectMusicObject_GetDescriptor(dmo, &desc);
        if (forms[i].needs_size) {
            todo_wine ok(hr == E_INVALIDARG, "GetDescriptor failed: %#lx, expected E_INVALIDARG\n", hr);
            desc.dwSize = sizeof(desc);
            hr = IDirectMusicObject_GetDescriptor(dmo, &desc);
        }
        ok(hr == S_OK, "GetDescriptor failed: %#lx, expected S_OK\n", hr);
        ok(desc.dwValidData == DMUS_OBJ_CLASS, "Got valid data %#lx, expected DMUS_OBJ_CLASS\n",
                desc.dwValidData);
        ok(IsEqualGUID(&desc.guidClass, forms[i].clsid), "Got class guid %s, expected CLSID_%s\n",
                wine_dbgstr_guid(&desc.guidClass), forms[i].class);

        /* Empty RIFF stream */
        empty[1] = forms[i].form;
        stream = gen_riff_stream(empty);
        memset(&desc, 0, sizeof(desc));
        hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
        if (forms[i].needs_size) {
            ok(hr == E_INVALIDARG, "ParseDescriptor failed: %#lx, expected E_INVALIDARG\n", hr);
            desc.dwSize = sizeof(desc);
            hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
        }
        ok(hr == S_OK, "ParseDescriptor failed: %#lx, expected S_OK\n", hr);
        ok(desc.dwValidData == DMUS_OBJ_CLASS, "Got valid data %#lx, expected DMUS_OBJ_CLASS\n",
                desc.dwValidData);
        ok(IsEqualGUID(&desc.guidClass, forms[i].clsid), "Got class guid %s, expected CLSID_%s\n",
                wine_dbgstr_guid(&desc.guidClass), forms[i].class);

        /* NULL pointers */
        memset(&desc, 0, sizeof(desc));
        desc.dwSize = sizeof(desc);
        hr = IDirectMusicObject_ParseDescriptor(dmo, NULL, &desc);
        ok(hr == E_POINTER, "ParseDescriptor failed: %#lx, expected E_POINTER\n", hr);
        hr = IDirectMusicObject_ParseDescriptor(dmo, stream, NULL);
        if (forms[i].needs_size)
            ok(hr == E_INVALIDARG, "ParseDescriptor failed: %#lx, expected E_INVALIDARG\n", hr);
        else
            ok(hr == E_POINTER, "ParseDescriptor failed: %#lx, expected E_POINTER\n", hr);
        hr = IDirectMusicObject_ParseDescriptor(dmo, NULL, NULL);
        ok(hr == E_POINTER, "ParseDescriptor failed: %#lx, expected E_POINTER\n", hr);
        IStream_Release(stream);

        /* Wrong form */
        empty[1] = DMUS_FOURCC_CONTAINER_FORM;
        stream = gen_riff_stream(empty);
        memset(&desc, 0, sizeof(desc));
        desc.dwSize = sizeof(desc);
        hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
        if (forms[i].needs_size)
            ok(hr == DMUS_E_CHUNKNOTFOUND,
                    "ParseDescriptor failed: %#lx, expected DMUS_E_CHUNKNOTFOUND\n", hr);
        else
            ok(hr == E_FAIL, "ParseDescriptor failed: %#lx, expected E_FAIL\n", hr);
        ok(!desc.dwValidData, "Got valid data %#lx, expected 0\n", desc.dwValidData);
        IStream_Release(stream);

        /* Not a RIFF stream */
        stream = gen_riff_stream(noriff);
        memset(&desc, 0, sizeof(desc));
        desc.dwSize = sizeof(desc);
        hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
        if (forms[i].needs_size)
            ok(hr == DMUS_E_CHUNKNOTFOUND,
                    "ParseDescriptor failed: %#lx, expected DMUS_E_CHUNKNOTFOUND\n", hr);
        else
            ok(hr == E_FAIL, "ParseDescriptor failed: %#lx, expected E_FAIL\n", hr);
        ok(!desc.dwValidData, "Got valid data %#lx, expected 0\n", desc.dwValidData);
        IStream_Release(stream);

        /* All desc chunks */
        alldesc[1] = forms[i].form;
        stream = gen_riff_stream(alldesc);
        memset(&desc, 0, sizeof(desc));
        desc.dwSize = sizeof(desc);
        hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
        ok(hr == S_OK, "ParseDescriptor failed: %#lx, expected S_OK\n", hr);
        valid = DMUS_OBJ_OBJECT | DMUS_OBJ_CLASS | DMUS_OBJ_VERSION;
        if (forms[i].form != mmioFOURCC('W','A','V','E'))
            valid |= DMUS_OBJ_NAME | DMUS_OBJ_CATEGORY;
        ok(desc.dwValidData == valid, "Got valid data %#lx, expected %#lx\n", desc.dwValidData, valid);
        ok(IsEqualGUID(&desc.guidClass, forms[i].clsid), "Got class guid %s, expected CLSID_%s\n",
                wine_dbgstr_guid(&desc.guidClass), forms[i].class);
        ok(IsEqualGUID(&desc.guidObject, &GUID_NULL), "Got object guid %s, expected GUID_NULL\n",
                wine_dbgstr_guid(&desc.guidClass));
        ok(desc.vVersion.dwVersionMS == 5 && desc.vVersion.dwVersionLS == 8,
            "Got version %lu.%lu, expected 5.8\n", desc.vVersion.dwVersionMS,
            desc.vVersion.dwVersionLS);
        if (forms[i].form != mmioFOURCC('W','A','V','E'))
            ok(!lstrcmpW(desc.wszName, L"UNAM"), "Got name '%s', expected 'UNAM'\n",
                    wine_dbgstr_w(desc.wszName));
        IStream_Release(stream);

        /* Duplicated chunks */
        dupes[1] = forms[i].form;
        stream = gen_riff_stream(dupes);
        memset(&desc, 0, sizeof(desc));
        desc.dwSize = sizeof(desc);
        hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
        ok(hr == S_OK, "ParseDescriptor failed: %#lx, expected S_OK\n", hr);
        ok(desc.dwValidData == valid, "Got valid data %#lx, expected %#lx\n", desc.dwValidData, valid);
        IStream_Release(stream);

        /* UNFO list with INAM */
        inam[1] = forms[i].form;
        inam[3] = DMUS_FOURCC_UNFO_LIST;
        stream = gen_riff_stream(inam);
        memset(&desc, 0, sizeof(desc));
        desc.dwSize = sizeof(desc);
        hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
        ok(hr == S_OK, "ParseDescriptor failed: %#lx, expected S_OK\n", hr);
        ok(desc.dwValidData == DMUS_OBJ_CLASS, "Got valid data %#lx, expected DMUS_OBJ_CLASS\n",
                desc.dwValidData);
        IStream_Release(stream);

        /* INFO list with INAM */
        inam[3] = DMUS_FOURCC_INFO_LIST;
        stream = gen_riff_stream(inam);
        memset(&desc, 0, sizeof(desc));
        desc.dwSize = sizeof(desc);
        hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
        ok(hr == S_OK, "ParseDescriptor failed: %#lx, expected S_OK\n", hr);
        valid = DMUS_OBJ_CLASS;
        if (forms[i].form == mmioFOURCC('W','A','V','E'))
            valid |= DMUS_OBJ_NAME;
        ok(desc.dwValidData == valid, "Got valid data %#lx, expected %#lx\n", desc.dwValidData, valid);
        if (forms[i].form == mmioFOURCC('W','A','V','E'))
            ok(!lstrcmpW(desc.wszName, L"I"), "Got name '%s', expected 'I'\n",
                    wine_dbgstr_w(desc.wszName));
        IStream_Release(stream);

        IDirectMusicObject_Release(dmo);
    }
}

static void test_performance_InitAudio(void)
{
    DMUS_PORTPARAMS params =
    {
        .dwSize = sizeof(params),
        .dwValidParams = DMUS_PORTPARAMS_EFFECTS,
        .dwEffectFlags = 1,
    };
    IDirectMusicPerformance8 *performance;
    IDirectMusic *dmusic;
    IDirectSound *dsound;
    IDirectMusicPort *port;
    IDirectMusicAudioPath *path;
    DWORD channel, group;
    HRESULT hr;
    ULONG ref;

    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicPerformance8, (void **)&performance);
    if (hr != S_OK) {
        win_skip("Cannot create DirectMusicPerformance object (%lx)\n", hr);
        CoUninitialize();
        return;
    }

    dsound = NULL;
    hr = IDirectMusicPerformance8_InitAudio(performance, NULL, &dsound, NULL,
            DMUS_APATH_SHARED_STEREOPLUSREVERB, 128, DMUS_AUDIOF_ALL, NULL);
    if (hr != S_OK) {
        IDirectMusicPerformance8_Release(performance);
        win_skip("InitAudio failed (%lx)\n", hr);
        return;
    }

    hr = IDirectMusicPerformance8_PChannelInfo(performance, 128, &port, NULL, NULL);
    ok(hr == E_INVALIDARG, "PChannelInfo failed, got %#lx\n", hr);
    hr = IDirectMusicPerformance8_PChannelInfo(performance, 127, &port, NULL, NULL);
    ok(hr == S_OK, "PChannelInfo failed, got %#lx\n", hr);
    IDirectMusicPort_Release(port);
    port = NULL;
    hr = IDirectMusicPerformance8_PChannelInfo(performance, 0, &port, NULL, NULL);
    ok(hr == S_OK, "PChannelInfo failed, got %#lx\n", hr);
    ok(port != NULL, "IDirectMusicPort not set\n");
    hr = IDirectMusicPerformance8_AssignPChannel(performance, 0, port, 0, 0);
    ok(hr == DMUS_E_AUDIOPATHS_IN_USE, "AssignPChannel failed (%#lx)\n", hr);
    hr = IDirectMusicPerformance8_AssignPChannelBlock(performance, 0, port, 0);
    ok(hr == DMUS_E_AUDIOPATHS_IN_USE, "AssignPChannelBlock failed (%#lx)\n", hr);
    IDirectMusicPort_Release(port);

    hr = IDirectMusicPerformance8_GetDefaultAudioPath(performance, &path);
    ok(hr == S_OK, "Failed to call GetDefaultAudioPath (%lx)\n", hr);
    if (hr == S_OK)
        IDirectMusicAudioPath_Release(path);

    hr = IDirectMusicPerformance8_CloseDown(performance);
    ok(hr == S_OK, "Failed to call CloseDown (%lx)\n", hr);

    IDirectMusicPerformance8_Release(performance);

    /* Auto generated dmusic and dsound */
    create_performance(&performance, NULL, NULL, FALSE);
    hr = IDirectMusicPerformance8_InitAudio(performance, NULL, NULL, NULL, 0, 64, 0, NULL);
    ok(hr == S_OK, "InitAudio failed: %#lx\n", hr);
    hr = IDirectMusicPerformance8_PChannelInfo(performance, 0, &port, NULL, NULL);
    ok(hr == E_INVALIDARG, "PChannelInfo failed, got %#lx\n", hr);
    destroy_performance(performance, NULL, NULL);

    /* Refcounts for auto generated dmusic and dsound */
    create_performance(&performance, NULL, NULL, FALSE);
    dmusic = NULL;
    dsound = NULL;
    hr = IDirectMusicPerformance8_InitAudio(performance, &dmusic, &dsound, NULL, 0, 64, 0, NULL);
    ok(hr == S_OK, "InitAudio failed: %#lx\n", hr);
    ref = get_refcount(dsound);
    ok(ref == 3, "dsound ref count got %ld expected 3\n", ref);
    ref = get_refcount(dmusic);
    ok(ref == 2, "dmusic ref count got %ld expected 2\n", ref);
    destroy_performance(performance, NULL, NULL);

    /* dsound without SetCooperativeLevel() */
    create_performance(&performance, NULL, &dsound, FALSE);
    hr = IDirectMusicPerformance8_InitAudio(performance, NULL, &dsound, NULL, 0, 0, 0, NULL);
    todo_wine ok(hr == DSERR_PRIOLEVELNEEDED, "InitAudio failed: %#lx\n", hr);
    destroy_performance(performance, NULL, dsound);

    /* Using the wrong CLSID_DirectSound */
    create_performance(&performance, NULL, NULL, FALSE);
    hr = DirectSoundCreate(NULL, &dsound, NULL);
    ok(hr == S_OK, "DirectSoundCreate failed: %#lx\n", hr);
    hr = IDirectMusicPerformance8_InitAudio(performance, NULL, &dsound, NULL, 0, 0, 0, NULL);
    todo_wine ok(hr == E_NOINTERFACE, "InitAudio failed: %#lx\n", hr);
    destroy_performance(performance, NULL, dsound);

    /* Init() works with just a CLSID_DirectSound */
    create_performance(&performance, NULL, NULL, FALSE);
    hr = DirectSoundCreate(NULL, &dsound, NULL);
    ok(hr == S_OK, "DirectSoundCreate failed: %#lx\n", hr);
    hr = IDirectSound_SetCooperativeLevel(dsound, GetForegroundWindow(), DSSCL_PRIORITY);
    ok(hr == S_OK, "SetCooperativeLevel failed: %#lx\n", hr);
    hr = IDirectMusicPerformance8_Init(performance, NULL, dsound, NULL);
    ok(hr == S_OK, "Init failed: %#lx\n", hr);
    destroy_performance(performance, NULL, dsound);

    /* Init() followed by InitAudio() */
    create_performance(&performance, NULL, &dsound, TRUE);
    hr = IDirectMusicPerformance8_Init(performance, NULL, dsound, NULL);
    ok(hr == S_OK, "Init failed: %#lx\n", hr);
    hr = IDirectMusicPerformance8_InitAudio(performance, NULL, &dsound, NULL, 0, 0, 0, NULL);
    ok(hr == DMUS_E_ALREADY_INITED, "InitAudio failed: %#lx\n", hr);
    destroy_performance(performance, NULL, dsound);

    /* Provided dmusic and dsound */
    create_performance(&performance, &dmusic, &dsound, TRUE);
    hr = IDirectMusicPerformance8_InitAudio(performance, &dmusic, &dsound, NULL, 0, 64, 0, NULL);
    ok(hr == S_OK, "InitAudio failed: %#lx\n", hr);
    ref = get_refcount(dsound);
    ok(ref == 2, "dsound ref count got %ld expected 2\n", ref);
    ref = get_refcount(dmusic);
    ok(ref == 2, "dmusic ref count got %ld expected 2\n", ref);
    destroy_performance(performance, dmusic, dsound);

    /* Provided dmusic initialized with SetDirectSound */
    create_performance(&performance, &dmusic, &dsound, TRUE);
    hr = IDirectMusic_SetDirectSound(dmusic, dsound, NULL);
    ok(hr == S_OK, "SetDirectSound failed: %#lx\n", hr);
    ref = get_refcount(dsound);
    ok(ref == 2, "dsound ref count got %ld expected 2\n", ref);
    hr = IDirectMusicPerformance8_InitAudio(performance, &dmusic, NULL, NULL, 0, 64, 0, NULL);
    ok(hr == S_OK, "InitAudio failed: %#lx\n", hr);
    ref = get_refcount(dsound);
    ok(ref == 2, "dsound ref count got %ld expected 2\n", ref);
    ref = get_refcount(dmusic);
    ok(ref == 2, "dmusic ref count got %ld expected 2\n", ref);
    destroy_performance(performance, dmusic, dsound);

    /* Provided dmusic and dsound, dmusic initialized with SetDirectSound */
    create_performance(&performance, &dmusic, &dsound, TRUE);
    hr = IDirectMusic_SetDirectSound(dmusic, dsound, NULL);
    ok(hr == S_OK, "SetDirectSound failed: %#lx\n", hr);
    ref = get_refcount(dsound);
    ok(ref == 2, "dsound ref count got %ld expected 2\n", ref);
    hr = IDirectMusicPerformance8_InitAudio(performance, &dmusic, &dsound, NULL, 0, 64, 0, NULL);
    ok(hr == S_OK, "InitAudio failed: %#lx\n", hr);
    ref = get_refcount(dsound);
    ok(ref == 3, "dsound ref count got %ld expected 3\n", ref);
    ref = get_refcount(dmusic);
    ok(ref == 2, "dmusic ref count got %ld expected 2\n", ref);
    destroy_performance(performance, dmusic, dsound);

    /* Provided dmusic and dsound, dmusic initialized with SetDirectSound, port created and activated */
    create_performance(&performance, &dmusic, &dsound, TRUE);
    hr = IDirectMusic_SetDirectSound(dmusic, dsound, NULL);
    ok(hr == S_OK, "SetDirectSound failed: %#lx\n", hr);
    hr = IDirectMusic_CreatePort(dmusic, &CLSID_DirectMusicSynth, &params, &port, NULL);
    ok(hr == S_OK, "CreatePort failed: %#lx\n", hr);
    hr = IDirectMusicPort_Activate(port, TRUE);
    ok(hr == S_OK, "Activate failed: %#lx\n", hr);
    hr = IDirectMusicPort_SetNumChannelGroups(port, 1);
    ok(hr == S_OK, "SetNumChannelGroups failed: %#lx\n", hr);
    hr = IDirectMusicPerformance8_Init(performance, &dmusic, dsound, 0);
    ok(hr == S_OK, "Init failed: %#lx\n", hr);
    destroy_performance(performance, dmusic, dsound);

    /* InitAudio with perf channel count not a multiple of 16 rounds up */
    create_performance(&performance, NULL, NULL, TRUE);
    hr = IDirectMusicPerformance8_InitAudio(performance, NULL, NULL, NULL,
            DMUS_APATH_SHARED_STEREOPLUSREVERB, 29, DMUS_AUDIOF_ALL, NULL);
    ok(hr == S_OK, "InitAudio failed: %#lx\n", hr);
    hr = IDirectMusicPerformance8_PChannelInfo(performance, 31, &port, &group, &channel);
    ok(hr == S_OK && group == 2 && channel == 15,
            "PChannelInfo failed, got %#lx, %lu, %lu\n", hr, group, channel);
    hr = IDirectMusicPerformance8_PChannelInfo(performance, 32, &port, NULL, NULL);
    ok(hr == E_INVALIDARG, "PChannelInfo failed, got %#lx\n", hr);
    destroy_performance(performance, NULL, NULL);
}

static void test_performance_createport(void)
{
    IDirectMusicPerformance8 *perf;
    IDirectMusic *music = NULL;
    IDirectMusicPort *port = NULL;
    DMUS_PORTCAPS portcaps;
    DMUS_PORTPARAMS portparams;
    DWORD i;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL,
            CLSCTX_INPROC_SERVER, &IID_IDirectMusicPerformance8, (void**)&perf);
    ok(hr == S_OK, "CoCreateInstance failed: %#lx\n", hr);

    hr = IDirectMusicPerformance8_Init(perf, &music, NULL, NULL);
    ok(hr == S_OK, "Init failed: %#lx\n", hr);
    ok(music != NULL, "Didn't get IDirectMusic pointer\n");

    i = 0;
    while(1){
        portcaps.dwSize = sizeof(portcaps);

        hr = IDirectMusic_EnumPort(music, i, &portcaps);
        ok(hr == S_OK || hr == S_FALSE || (i == 0 && hr == E_INVALIDARG), "EnumPort failed: %#lx\n", hr);
        if(hr != S_OK)
            break;

        ok(portcaps.dwSize == sizeof(portcaps), "Got unexpected portcaps struct size: %lu\n", portcaps.dwSize);
        trace("portcaps(%lu).dwFlags: %#lx\n", i, portcaps.dwFlags);
        trace("portcaps(%lu).guidPort: %s\n", i, wine_dbgstr_guid(&portcaps.guidPort));
        trace("portcaps(%lu).dwClass: %#lx\n", i, portcaps.dwClass);
        trace("portcaps(%lu).dwType: %#lx\n", i, portcaps.dwType);
        trace("portcaps(%lu).dwMemorySize: %#lx\n", i, portcaps.dwMemorySize);
        trace("portcaps(%lu).dwMaxChannelGroups: %lu\n", i, portcaps.dwMaxChannelGroups);
        trace("portcaps(%lu).dwMaxVoices: %lu\n", i, portcaps.dwMaxVoices);
        trace("portcaps(%lu).dwMaxAudioChannels: %lu\n", i, portcaps.dwMaxAudioChannels);
        trace("portcaps(%lu).dwEffectFlags: %#lx\n", i, portcaps.dwEffectFlags);
        trace("portcaps(%lu).wszDescription: %s\n", i, wine_dbgstr_w(portcaps.wszDescription));

        ++i;
    }

    if(i == 0){
        win_skip("No ports available, skipping tests\n");
        return;
    }

    portparams.dwSize = sizeof(portparams);

    /* dwValidParams == 0 -> S_OK, filled struct */
    portparams.dwValidParams = 0;
    hr = IDirectMusic_CreatePort(music, &CLSID_DirectMusicSynth, &portparams, &port, NULL);
    ok(hr == S_OK, "CreatePort failed: %#lx\n", hr);
    ok(port != NULL, "Didn't get IDirectMusicPort pointer\n");
    ok(portparams.dwValidParams, "portparams struct was not filled in\n");
    IDirectMusicPort_Release(port);
    port = NULL;

    /* dwValidParams != 0, invalid param -> S_FALSE, filled struct */
    portparams.dwValidParams = DMUS_PORTPARAMS_CHANNELGROUPS;
    portparams.dwChannelGroups = 0;
    hr = IDirectMusic_CreatePort(music, &CLSID_DirectMusicSynth, &portparams, &port, NULL);
    todo_wine ok(hr == S_FALSE, "CreatePort failed: %#lx\n", hr);
    ok(port != NULL, "Didn't get IDirectMusicPort pointer\n");
    ok(portparams.dwValidParams, "portparams struct was not filled in\n");
    IDirectMusicPort_Release(port);
    port = NULL;

    /* dwValidParams != 0, valid params -> S_OK */
    hr = IDirectMusic_CreatePort(music, &CLSID_DirectMusicSynth, &portparams, &port, NULL);
    ok(hr == S_OK, "CreatePort failed: %#lx\n", hr);
    ok(port != NULL, "Didn't get IDirectMusicPort pointer\n");
    IDirectMusicPort_Release(port);
    port = NULL;

    /* GUID_NULL succeeds */
    portparams.dwValidParams = 0;
    hr = IDirectMusic_CreatePort(music, &GUID_NULL, &portparams, &port, NULL);
    ok(hr == S_OK, "CreatePort failed: %#lx\n", hr);
    ok(port != NULL, "Didn't get IDirectMusicPort pointer\n");
    ok(portparams.dwValidParams, "portparams struct was not filled in\n");
    IDirectMusicPort_Release(port);
    port = NULL;

    /* null GUID fails */
    portparams.dwValidParams = 0;
    hr = IDirectMusic_CreatePort(music, NULL, &portparams, &port, NULL);
    ok(hr == E_POINTER, "CreatePort failed: %#lx\n", hr);
    ok(port == NULL, "Get IDirectMusicPort pointer? %p\n", port);
    ok(portparams.dwValidParams == 0, "portparams struct was filled in?\n");

    /* garbage GUID fails */
    portparams.dwValidParams = 0;
    hr = IDirectMusic_CreatePort(music, &GUID_Bunk, &portparams, &port, NULL);
    ok(hr == E_NOINTERFACE, "CreatePort failed: %#lx\n", hr);
    ok(port == NULL, "Get IDirectMusicPort pointer? %p\n", port);
    ok(portparams.dwValidParams == 0, "portparams struct was filled in?\n");

    hr = IDirectMusicPerformance8_CloseDown(perf);
    ok(hr == S_OK, "CloseDown failed: %#lx\n", hr);

    IDirectMusic_Release(music);
    IDirectMusicPerformance8_Release(perf);
}

static void test_performance_pchannel(void)
{
    IDirectMusicPerformance8 *perf;
    IDirectMusicPort *port = NULL, *port2;
    DWORD channel, group;
    unsigned int i;
    HRESULT hr;

    create_performance(&perf, NULL, NULL, TRUE);
    hr = IDirectMusicPerformance8_Init(perf, NULL, NULL, NULL);
    ok(hr == S_OK, "Init failed: %#lx\n", hr);
    hr = IDirectMusicPerformance8_PChannelInfo(perf, 0, &port, NULL, NULL);
    ok(hr == E_INVALIDARG && !port, "PChannelInfo failed, got %#lx, %p\n", hr, port);

    /* Add default port. Sets PChannels 0-15 to the corresponding channels in group 1 */
    hr = IDirectMusicPerformance8_AddPort(perf, NULL);
    ok(hr == S_OK, "AddPort of default port failed: %#lx\n", hr);
    hr = IDirectMusicPerformance8_PChannelInfo(perf, 0, NULL, NULL, NULL);
    ok(hr == S_OK, "PChannelInfo failed, got %#lx\n", hr);
    hr = IDirectMusicPerformance8_PChannelInfo(perf, 0, &port, NULL, NULL);
    ok(hr == S_OK && port, "PChannelInfo failed, got %#lx, %p\n", hr, port);
    for (i = 1; i < 16; i++) {
        hr = IDirectMusicPerformance8_PChannelInfo(perf, i, &port2, &group, &channel);
        ok(hr == S_OK && port == port2 && group == 1 && channel == i,
                "PChannelInfo failed, got %#lx, %p, %lu, %lu\n", hr, port2, group, channel);
        IDirectMusicPort_Release(port2);
    }

    /* Unset PChannels fail to retrieve */
    hr = IDirectMusicPerformance8_PChannelInfo(perf, 16, &port2, NULL, NULL);
    ok(hr == E_INVALIDARG, "PChannelInfo failed, got %#lx, %p\n", hr, port);
    hr = IDirectMusicPerformance8_PChannelInfo(perf, MAXDWORD - 16, &port2, NULL, NULL);
    ok(hr == E_INVALIDARG, "PChannelInfo failed, got %#lx, %p\n", hr, port);

    /* Channel group 0 can be set just fine */
    hr = IDirectMusicPerformance8_AssignPChannel(perf, 0, port, 0, 0);
    ok(hr == S_OK, "AssignPChannel failed, got %#lx\n", hr);
    hr = IDirectMusicPerformance8_AssignPChannelBlock(perf, 0, port, 0);
    ok(hr == S_OK, "AssignPChannelBlock failed, got %#lx\n", hr);
    for (i = 1; i < 16; i++) {
        hr = IDirectMusicPerformance8_PChannelInfo(perf, i, &port2, &group, &channel);
        ok(hr == S_OK && port == port2 && group == 0 && channel == i,
                "PChannelInfo failed, got %#lx, %p, %lu, %lu\n", hr, port2, group, channel);
        IDirectMusicPort_Release(port2);
    }

    /* Last PChannel Block can be set only individually but not read */
    hr = IDirectMusicPerformance8_AssignPChannel(perf, MAXDWORD, port, 0, 3);
    ok(hr == S_OK, "AssignPChannel failed, got %#lx\n", hr);
    port2 = (IDirectMusicPort *)0xdeadbeef;
    hr = IDirectMusicPerformance8_PChannelInfo(perf, MAXDWORD, &port2, NULL, NULL);
    todo_wine ok(hr == E_INVALIDARG && port2 == (IDirectMusicPort *)0xdeadbeef,
            "PChannelInfo failed, got %#lx, %p\n", hr, port2);
    hr = IDirectMusicPerformance8_AssignPChannelBlock(perf, MAXDWORD, port, 0);
    ok(hr == E_INVALIDARG, "AssignPChannelBlock failed, got %#lx\n", hr);
    hr = IDirectMusicPerformance8_AssignPChannelBlock(perf, MAXDWORD / 16, port, 1);
    todo_wine ok(hr == E_INVALIDARG, "AssignPChannelBlock failed, got %#lx\n", hr);
    for (i = MAXDWORD - 15; i < MAXDWORD; i++) {
        hr = IDirectMusicPerformance8_AssignPChannel(perf, i, port, 0, 0);
        ok(hr == S_OK, "AssignPChannel failed, got %#lx\n", hr);
        hr = IDirectMusicPerformance8_PChannelInfo(perf, i, &port2, NULL, NULL);
        todo_wine ok(hr == E_INVALIDARG && port2 == (IDirectMusicPort *)0xdeadbeef,
                "PChannelInfo failed, got %#lx, %p\n", hr, port2);
    }

    /* Second to last PChannel Block can be set only individually and read */
    hr = IDirectMusicPerformance8_AssignPChannelBlock(perf, MAXDWORD / 16 - 1, port, 1);
    todo_wine ok(hr == E_INVALIDARG, "AssignPChannelBlock failed, got %#lx\n", hr);
    for (i = MAXDWORD - 31; i < MAXDWORD - 15; i++) {
        hr = IDirectMusicPerformance8_AssignPChannel(perf, i, port, 1, 7);
        ok(hr == S_OK, "AssignPChannel failed, got %#lx\n", hr);
        hr = IDirectMusicPerformance8_PChannelInfo(perf, i, &port2, &group, &channel);
        ok(hr == S_OK && port2 == port && group == 1 && channel == 7,
                "PChannelInfo failed, got %#lx, %p, %lu, %lu\n", hr, port2, group, channel);
        IDirectMusicPort_Release(port2);
    }

    /* Third to last PChannel Block behaves normal */
    hr = IDirectMusicPerformance8_AssignPChannelBlock(perf, MAXDWORD / 16 - 2, port, 0);
    ok(hr == S_OK, "AssignPChannelBlock failed, got %#lx\n", hr);
    for (i = MAXDWORD - 47; i < MAXDWORD - 31; i++) {
        hr = IDirectMusicPerformance8_PChannelInfo(perf, i, &port2, &group, &channel);
        ok(hr == S_OK && port2 == port && group == 0 && channel == i % 16,
                "PChannelInfo failed, got %#lx, %p, %lu, %lu\n", hr, port2, group, channel);
        IDirectMusicPort_Release(port2);
    }

    /* One PChannel set in a Block, rest is initialized too */
    hr = IDirectMusicPerformance8_AssignPChannel(perf, 4711, port, 1, 13);
    ok(hr == S_OK, "AssignPChannel failed, got %#lx\n", hr);
    hr = IDirectMusicPerformance8_PChannelInfo(perf, 4711, &port2, &group, &channel);
    ok(hr == S_OK && port2 == port && group == 1 && channel == 13,
            "PChannelInfo failed, got %#lx, %p, %lu, %lu\n", hr, port2, group, channel);
    IDirectMusicPort_Release(port2);
    group = channel = 0xdeadbeef;
    hr = IDirectMusicPerformance8_PChannelInfo(perf, 4712, &port2, &group, &channel);
    ok(hr == S_OK && port2 == port && group == 0 && channel == 8,
            "PChannelInfo failed, got %#lx, %p, %lu, %lu\n", hr, port2, group, channel);
    IDirectMusicPort_Release(port2);
    group = channel = 0xdeadbeef;
    hr = IDirectMusicPerformance8_PChannelInfo(perf, 4719, &port2, &group, &channel);
    ok(hr == S_OK && port2 == port && group == 0 && channel == 15,
            "PChannelInfo failed, got %#lx, %p, %lu, %lu\n", hr, port2, group, channel);
    IDirectMusicPort_Release(port2);

    IDirectMusicPort_Release(port);
    destroy_performance(perf, NULL, NULL);
}

static void test_performance_tool(void)
{
    DMUS_NOTE_PMSG note60 =
    {
        .dwSize = sizeof(DMUS_NOTE_PMSG),
        .dwFlags = DMUS_PMSGF_REFTIME | DMUS_PMSGF_MUSICTIME | DMUS_PMSGF_TOOL_QUEUE,
        .dwVirtualTrackID = 1,
        .dwType = DMUS_PMSGT_NOTE,
        .dwGroupID = 1,
        .mtDuration = 1000,
        .wMusicValue = 60,
        .bVelocity = 127,
        .bFlags = DMUS_NOTEF_NOTEON,
        .bPlayModeFlags = DMUS_PLAYMODE_FIXED,
        .bMidiValue = 60,
    };
    IDirectMusicPerformance *performance;
    IDirectMusicGraph *graph;
    IDirectMusicTool *tool;
    DWORD value, types[1];
    DMUS_NOTE_PMSG *note;
    DMUS_PMSG msg = {0};
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicPerformance, (void **)&performance);
    ok(hr == S_OK, "got %#lx\n", hr);

    check_interface(performance, &IID_IDirectMusicTool8, FALSE);

    hr = IDirectMusicPerformance_QueryInterface(performance, &IID_IDirectMusicTool, (void **)&tool);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_QueryInterface(performance, &IID_IDirectMusicGraph, (void **)&graph);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicTool_Init(tool, graph);
    ok(hr == E_NOTIMPL, "got %#lx\n", hr);
    value = 0xdeadbeef;
    hr = IDirectMusicTool_GetMsgDeliveryType(tool, &value);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(value == DMUS_PMSGF_TOOL_IMMEDIATE, "got %#lx\n", value);
    value = 0xdeadbeef;
    hr = IDirectMusicTool_GetMediaTypeArraySize(tool, &value);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(value == 0, "got %#lx\n", value);
    hr = IDirectMusicTool_GetMediaTypes(tool, (DWORD **)&types, 64);
    ok(hr == E_NOTIMPL, "got %#lx\n", hr);
    hr = IDirectMusicTool_ProcessPMsg(tool, performance, &msg);
    ok(hr == DMUS_S_FREE, "got %#lx\n", hr);
    hr = IDirectMusicTool_Flush(tool, performance, &msg, 0);
    todo_wine ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicPerformance_Init(performance, NULL, NULL, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicPerformance_AddPort(performance, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicPerformance_AllocPMsg(performance, sizeof(DMUS_NOTE_PMSG), (DMUS_PMSG **)&note);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicPerformance_GetTime(performance, NULL, &note60.mtTime);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_MusicToReferenceTime(performance, note60.mtTime, &note60.rtTime);
    ok(hr == S_OK, "got %#lx\n", hr);

    *note = note60;
    note->bFlags = 0;
    hr = IDirectMusicTool_ProcessPMsg(tool, performance, (DMUS_PMSG *)note);
    ok(hr == DMUS_S_FREE, "got %#lx\n", hr);

    hr = IDirectMusicPerformance_GetTime(performance, NULL, &note60.mtTime);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_MusicToReferenceTime(performance, note60.mtTime, &note60.rtTime);
    ok(hr == S_OK, "got %#lx\n", hr);

    *note = note60;
    note->mtDuration = 0;
    hr = IDirectMusicTool_ProcessPMsg(tool, performance, (DMUS_PMSG *)note);
    ok(hr == DMUS_S_FREE, "got %#lx\n", hr);

    hr = IDirectMusicPerformance_GetTime(performance, NULL, &note60.mtTime);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_MusicToReferenceTime(performance, note60.mtTime, &note60.rtTime);
    ok(hr == S_OK, "got %#lx\n", hr);

    *note = note60;
    hr = IDirectMusicTool_ProcessPMsg(tool, performance, (DMUS_PMSG *)note);
    ok(hr == DMUS_S_REQUEUE, "got %#lx\n", hr);
    ok(fabs(note->rtTime - note60.rtTime - scale_music_time(999, 120.0)) < 10.0,
            "got %I64d\n", note->rtTime - note60.rtTime);
    ok(note->mtTime - note60.mtTime == 999, "got %ld\n", note->mtTime - note60.mtTime);
    check_dmus_note_pmsg(note, note60.mtTime + 999, 0, 1000, 60, 127, 0);

    hr = IDirectMusicPerformance_GetTime(performance, NULL, &note60.mtTime);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_MusicToReferenceTime(performance, note60.mtTime, &note60.rtTime);
    ok(hr == S_OK, "got %#lx\n", hr);

    *note = note60;
    note->nOffset = 1000;
    hr = IDirectMusicTool_ProcessPMsg(tool, performance, (DMUS_PMSG *)note);
    ok(hr == DMUS_S_REQUEUE, "got %#lx\n", hr);
    ok(fabs(note->rtTime - note60.rtTime - scale_music_time(999, 120.0)) < 10.0,
            "got %I64d\n", note->rtTime - note60.rtTime);
    ok(note->mtTime - note60.mtTime == 999, "got %ld\n", note->mtTime - note60.mtTime);
    check_dmus_note_pmsg(note, note60.mtTime + 999, 0, 1000, 60, 127, 0);

    hr = IDirectMusicPerformance_GetTime(performance, NULL, &note60.mtTime);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_MusicToReferenceTime(performance, note60.mtTime, &note60.rtTime);
    ok(hr == S_OK, "got %#lx\n", hr);

    *note = note60;
    note->mtDuration = 2;
    hr = IDirectMusicTool_ProcessPMsg(tool, performance, (DMUS_PMSG *)note);
    ok(hr == DMUS_S_REQUEUE, "got %#lx\n", hr);
    ok(fabs(note->rtTime - note60.rtTime - scale_music_time(1, 120.0)) < 10.0,
            "got %I64d\n", note->rtTime - note60.rtTime);
    ok(note->mtTime - note60.mtTime == 1, "got %ld\n", note->mtTime - note60.mtTime);
    check_dmus_note_pmsg(note, note60.mtTime + 1, 0, 2, 60, 127, 0);

    hr = IDirectMusicPerformance_GetTime(performance, NULL, &note60.mtTime);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_MusicToReferenceTime(performance, note60.mtTime, &note60.rtTime);
    ok(hr == S_OK, "got %#lx\n", hr);

    *note = note60;
    note->mtDuration = 1;
    hr = IDirectMusicTool_ProcessPMsg(tool, performance, (DMUS_PMSG *)note);
    ok(hr == DMUS_S_REQUEUE, "got %#lx\n", hr);
    ok(fabs(note->rtTime - note60.rtTime - scale_music_time(1, 120.0)) < 10.0,
            "got %I64d\n", note->rtTime - note60.rtTime);
    ok(note->mtTime - note60.mtTime == 1, "got %ld\n", note->mtTime - note60.mtTime);
    check_dmus_note_pmsg(note, note60.mtTime + 1, 0, 1, 60, 127, 0);

    hr = IDirectMusicPerformance_FreePMsg(performance, (DMUS_PMSG *)note);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicPerformance_CloseDown(performance);
    ok(hr == S_OK, "got %#lx\n", hr);

    IDirectMusicGraph_Release(graph);
    IDirectMusicTool_Release(tool);

    IDirectMusicPerformance_Release(performance);
}

static void test_performance_graph(void)
{
    IDirectMusicPerformance *performance;
    IDirectMusicGraph *graph, *tmp_graph;
    IDirectMusicTool *tool, *tmp_tool;
    DMUS_PMSG msg;
    HRESULT hr;

    hr = test_tool_create(NULL, 0, &tool);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicPerformance, (void **)&performance);
    ok(hr == S_OK, "got %#lx\n", hr);


    /* performance exposes a graph interface but it's not an actual toolgraph */
    hr = IDirectMusicPerformance_QueryInterface(performance, &IID_IDirectMusicGraph, (void **)&graph);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicGraph_InsertTool(graph, tool, NULL, 0, -1);
    ok(hr == E_NOTIMPL, "got %#lx\n", hr);
    hr = IDirectMusicGraph_GetTool(graph, 0, &tmp_tool);
    ok(hr == E_NOTIMPL, "got %#lx\n", hr);
    hr = IDirectMusicGraph_RemoveTool(graph, tool);
    ok(hr == E_NOTIMPL, "got %#lx\n", hr);

    /* test IDirectMusicGraph_StampPMsg usage */
    hr = IDirectMusicGraph_StampPMsg(graph, NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);
    memset(&msg, 0, sizeof(msg));
    hr = IDirectMusicGraph_StampPMsg(graph, &msg);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(msg.pGraph == NULL, "got %p\n", msg.pGraph);
    ok(msg.pTool != NULL, "got %p\n", msg.pTool);
    check_interface(msg.pTool, &IID_IDirectMusicPerformance, TRUE);

    ok(!msg.dwSize, "got %ld\n", msg.dwSize);
    ok(!msg.rtTime, "got %I64d\n", msg.rtTime);
    ok(!msg.mtTime, "got %ld\n", msg.mtTime);
    ok(msg.dwFlags == DMUS_PMSGF_TOOL_QUEUE, "got %#lx\n", msg.dwFlags);
    ok(!msg.dwPChannel, "got %ld\n", msg.dwPChannel);
    ok(!msg.dwVirtualTrackID, "got %ld\n", msg.dwVirtualTrackID);
    ok(!msg.dwType, "got %#lx\n", msg.dwType);
    ok(!msg.dwVoiceID, "got %ld\n", msg.dwVoiceID);
    ok(!msg.dwGroupID, "got %ld\n", msg.dwGroupID);
    ok(!msg.punkUser, "got %p\n", msg.punkUser);

    hr = IDirectMusicGraph_StampPMsg(graph, &msg);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(msg.pGraph == NULL, "got %p\n", msg.pGraph);
    ok(msg.pTool != NULL, "got %p\n", msg.pTool);
    check_interface(msg.pTool, &IID_IDirectMusicPerformance, TRUE);

    IDirectMusicTool_Release(msg.pTool);
    msg.pTool = NULL;

    IDirectMusicGraph_Release(graph);


    /* performance doesn't have a default embedded toolgraph */
    hr = IDirectMusicPerformance_GetGraph(performance, &graph);
    ok(hr == DMUS_E_NOT_FOUND, "got %#lx\n", hr);


    /* test adding a graph to the performance */
    hr = CoCreateInstance(&CLSID_DirectMusicGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicGraph, (void **)&graph);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_SetGraph(performance, graph);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicPerformance_GetGraph(performance, &tmp_graph);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(tmp_graph == graph, "got %p\n", graph);
    IDirectMusicGraph_Release(tmp_graph);

    hr = IDirectMusicGraph_InsertTool(graph, tool, NULL, 0, -1);
    ok(hr == S_OK, "got %#lx\n", hr);

    IDirectMusicGraph_Release(graph);


    /* test IDirectMusicGraph_StampPMsg usage */
    hr = IDirectMusicPerformance_QueryInterface(performance, &IID_IDirectMusicGraph, (void **)&graph);
    ok(hr == S_OK, "got %#lx\n", hr);

    memset(&msg, 0, sizeof(msg));
    hr = IDirectMusicGraph_StampPMsg(graph, &msg);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(msg.pGraph == graph, "got %p\n", msg.pGraph);
    ok(msg.pTool == tool, "got %p\n", msg.pTool);

    ok(!msg.dwSize, "got %ld\n", msg.dwSize);
    ok(!msg.rtTime, "got %I64d\n", msg.rtTime);
    ok(!msg.mtTime, "got %ld\n", msg.mtTime);
    ok(msg.dwFlags == DMUS_PMSGF_TOOL_IMMEDIATE, "got %#lx\n", msg.dwFlags);
    ok(!msg.dwPChannel, "got %ld\n", msg.dwPChannel);
    ok(!msg.dwVirtualTrackID, "got %ld\n", msg.dwVirtualTrackID);
    ok(!msg.dwType, "got %#lx\n", msg.dwType);
    ok(!msg.dwVoiceID, "got %ld\n", msg.dwVoiceID);
    ok(!msg.dwGroupID, "got %ld\n", msg.dwGroupID);
    ok(!msg.punkUser, "got %p\n", msg.punkUser);

    hr = IDirectMusicGraph_StampPMsg(graph, &msg);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(msg.pGraph == NULL, "got %p\n", msg.pGraph);
    ok(msg.pTool != NULL, "got %p\n", msg.pTool);
    check_interface(msg.pTool, &IID_IDirectMusicPerformance, TRUE);

    IDirectMusicTool_Release(msg.pTool);
    msg.pTool = NULL;

    IDirectMusicGraph_Release(graph);


    IDirectMusicPerformance_Release(performance);
    IDirectMusicTool_Release(tool);
}

#define check_reference_time(a, b) check_reference_time_(__LINE__, a, b)
static void check_reference_time_(int line, REFERENCE_TIME time, double expect)
{
    ok_(__FILE__, line)(llabs(time - (REFERENCE_TIME)expect) <= scale_music_time(1, 120) / 2.0,
                        "got %I64u, expected %f\n", time, expect);
}

static void test_performance_time(void)
{
    IDirectMusicPerformance *performance;
    REFERENCE_TIME init_time, time;
    IReferenceClock *clock;
    MUSIC_TIME music_time;
    IDirectMusic *dmusic;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicPerformance, (void **)&performance);
    ok(hr == S_OK, "got %#lx\n", hr);


    hr = IDirectMusicPerformance_MusicToReferenceTime(performance, 0, NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);
    time = 0xdeadbeef;
    hr = IDirectMusicPerformance_MusicToReferenceTime(performance, 0, &time);
    ok(hr == DMUS_E_NO_MASTER_CLOCK, "got %#lx\n", hr);
    ok(time == 0, "got %I64d\n", time);

    hr = IDirectMusicPerformance_ReferenceToMusicTime(performance, 0, NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);
    music_time = 0xdeadbeef;
    hr = IDirectMusicPerformance_ReferenceToMusicTime(performance, 0, &music_time);
    ok(hr == DMUS_E_NO_MASTER_CLOCK, "got %#lx\n", hr);
    ok(music_time == 0, "got %ld\n", music_time);


    dmusic = NULL;
    hr = IDirectMusicPerformance_Init(performance, &dmusic, NULL, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusic_GetMasterClock(dmusic, NULL, &clock);
    ok(hr == S_OK, "got %#lx\n", hr);
    IDirectMusic_Release(dmusic);
    hr = IReferenceClock_GetTime(clock, &init_time);
    ok(hr == S_OK, "got %#lx\n", hr);
    IReferenceClock_Release(clock);


    time = 0xdeadbeef;
    hr = IDirectMusicPerformance_MusicToReferenceTime(performance, 0, &time);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(time - init_time <= 100 * 10000, "got %I64d\n", time - init_time);
    init_time = time;

    time = 0xdeadbeef;
    hr = IDirectMusicPerformance_MusicToReferenceTime(performance, 1, &time);
    ok(hr == S_OK, "got %#lx\n", hr);
    check_reference_time(time - init_time, scale_music_time(1, 120));
    time = 0xdeadbeef;
    hr = IDirectMusicPerformance_MusicToReferenceTime(performance, 1000, &time);
    ok(hr == S_OK, "got %#lx\n", hr);
    check_reference_time(time - init_time, scale_music_time(1000, 120));
    time = 0xdeadbeef;
    hr = IDirectMusicPerformance_MusicToReferenceTime(performance, 2000, &time);
    ok(hr == S_OK, "got %#lx\n", hr);
    check_reference_time(time - init_time, scale_music_time(2000, 120));

    music_time = 0xdeadbeef;
    hr = IDirectMusicPerformance_ReferenceToMusicTime(performance, init_time, &music_time);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(music_time == 0, "got %ld\n", music_time);
    music_time = 0xdeadbeef;
    hr = IDirectMusicPerformance_ReferenceToMusicTime(performance, init_time + scale_music_time(1000, 120), &music_time);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(music_time == 1000, "got %ld\n", music_time);
    music_time = 0xdeadbeef;
    hr = IDirectMusicPerformance_ReferenceToMusicTime(performance, init_time + scale_music_time(2000, 120), &music_time);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(music_time == 2000, "got %ld\n", music_time);

    time = 0xdeadbeef;
    music_time = 0xdeadbeef;
    hr = IDirectMusicPerformance_GetTime(performance, &time, &music_time);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(time - init_time <= 200 * 10000, "got %I64d\n", time - init_time);
    ok(abs(music_time - music_time_from_reference(time - init_time, 120)) <= 1, "got %ld\n", music_time);


    hr = IDirectMusicPerformance_CloseDown(performance);
    ok(hr == S_OK, "got %#lx\n", hr);

    IDirectMusicPerformance_Release(performance);

}

static void test_performance_pmsg(void)
{
    static const DWORD delivery_flags[] = {DMUS_PMSGF_TOOL_IMMEDIATE, DMUS_PMSGF_TOOL_QUEUE, DMUS_PMSGF_TOOL_ATTIME};
    static const DWORD message_types[] = {DMUS_PMSGT_MIDI, DMUS_PMSGT_USER};
    IDirectMusicPerformance *performance;
    IDirectMusicGraph *graph, *performance_graph;
    IDirectMusicTool *tool;
    DMUS_PMSG *msg, *clone;
    MUSIC_TIME music_time;
    REFERENCE_TIME time;
    HRESULT hr;
    DWORD ret;
    UINT i;

    hr = test_tool_create(message_types, ARRAY_SIZE(message_types), &tool);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicPerformance, (void **)&performance);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_QueryInterface(performance, &IID_IDirectMusicGraph, (void **)&performance_graph);
    ok(hr == S_OK, "got %#lx\n", hr);


    hr = IDirectMusicPerformance_AllocPMsg(performance, 0, NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_AllocPMsg(performance, sizeof(DMUS_PMSG) - 1, &msg);
    ok(hr == E_INVALIDARG, "got %#lx\n", hr);

    hr = IDirectMusicPerformance_AllocPMsg(performance, sizeof(DMUS_PMSG), &msg);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(msg->dwSize == sizeof(DMUS_PMSG), "got %ld\n", msg->dwSize);
    ok(!msg->rtTime, "got %I64d\n", msg->rtTime);
    ok(!msg->mtTime, "got %ld\n", msg->mtTime);
    ok(!msg->dwFlags, "got %#lx\n", msg->dwFlags);
    ok(!msg->dwPChannel, "got %ld\n", msg->dwPChannel);
    ok(!msg->dwVirtualTrackID, "got %ld\n", msg->dwVirtualTrackID);
    ok(!msg->dwType, "got %#lx\n", msg->dwType);
    ok(!msg->dwVoiceID, "got %ld\n", msg->dwVoiceID);
    ok(!msg->dwGroupID, "got %ld\n", msg->dwGroupID);
    ok(!msg->punkUser, "got %p\n", msg->punkUser);

    hr = IDirectMusicPerformance_SendPMsg(performance, NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_SendPMsg(performance, msg);
    ok(hr == DMUS_E_NO_MASTER_CLOCK, "got %#lx\n", hr);

    hr = IDirectMusicPerformance_FreePMsg(performance, NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_FreePMsg(performance, msg);
    ok(hr == S_OK, "got %#lx\n", hr);


    hr = IDirectMusicPerformance_Init(performance, NULL, 0, 0);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = CoCreateInstance(&CLSID_DirectMusicGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicGraph, (void **)&graph);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_SetGraph(performance, graph);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicGraph_InsertTool(graph, tool, NULL, 0, -1);
    ok(hr == S_OK, "got %#lx\n", hr);
    IDirectMusicGraph_Release(graph);

    hr = IDirectMusicPerformance_AllocPMsg(performance, sizeof(DMUS_PMSG), &msg);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(msg->dwSize == sizeof(DMUS_PMSG), "got %ld\n", msg->dwSize);
    ok(!msg->rtTime, "got %I64d\n", msg->rtTime);
    ok(!msg->mtTime, "got %ld\n", msg->mtTime);
    ok(!msg->dwFlags, "got %#lx\n", msg->dwFlags);
    ok(!msg->dwPChannel, "got %ld\n", msg->dwPChannel);
    ok(!msg->dwVirtualTrackID, "got %ld\n", msg->dwVirtualTrackID);
    ok(!msg->pTool, "got %p\n", msg->pTool);
    ok(!msg->pGraph, "got %p\n", msg->pGraph);
    ok(!msg->dwType, "got %#lx\n", msg->dwType);
    ok(!msg->dwVoiceID, "got %ld\n", msg->dwVoiceID);
    ok(!msg->dwGroupID, "got %ld\n", msg->dwGroupID);
    ok(!msg->punkUser, "got %p\n", msg->punkUser);
    hr = IDirectMusicPerformance_SendPMsg(performance, msg);
    ok(hr == E_INVALIDARG, "got %#lx\n", hr);

    hr = IDirectMusicPerformance8_ClonePMsg((IDirectMusicPerformance8 *)performance, msg, NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);
    hr = IDirectMusicPerformance8_ClonePMsg((IDirectMusicPerformance8 *)performance, NULL, &clone);
    ok(hr == E_POINTER, "got %#lx\n", hr);
    clone = NULL;
    hr = IDirectMusicPerformance8_ClonePMsg((IDirectMusicPerformance8 *)performance, msg, &clone);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(clone != NULL, "got %p\n", clone);

    msg->mtTime = 500;
    msg->dwFlags = DMUS_PMSGF_MUSICTIME | DMUS_PMSGF_TOOL_QUEUE;
    hr = IDirectMusicPerformance_SendPMsg(performance, msg);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_SendPMsg(performance, msg);
    ok(hr == DMUS_E_ALREADY_SENT, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_FreePMsg(performance, msg);
    ok(hr == DMUS_E_CANNOT_FREE, "got %#lx\n", hr);

    hr = IDirectMusicPerformance_FreePMsg(performance, clone);
    ok(hr == S_OK, "got %#lx\n", hr);


    /* SendPMsg skips all the tools unless messages are stamped beforehand */

    hr = IDirectMusicPerformance_AllocPMsg(performance, sizeof(DMUS_PMSG), &msg);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(msg->dwSize == sizeof(DMUS_PMSG), "got %ld\n", msg->dwSize);
    msg->mtTime = 0;
    msg->dwFlags = DMUS_PMSGF_MUSICTIME;
    msg->dwType = DMUS_PMSGT_USER;
    hr = IDirectMusicPerformance_SendPMsg(performance, msg);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 10, &msg);
    ok(ret == WAIT_TIMEOUT, "got %#lx\n", ret);
    ok(!msg, "got %p\n", msg);


    /* SendPMsg converts music time to reference time if it is missing */

    hr = IDirectMusicPerformance_AllocPMsg(performance, sizeof(DMUS_PMSG), &msg);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(msg->dwSize == sizeof(DMUS_PMSG), "got %ld\n", msg->dwSize);
    msg->mtTime = 500;
    msg->dwFlags = DMUS_PMSGF_MUSICTIME;
    msg->dwType = DMUS_PMSGT_USER;
    hr = IDirectMusicGraph_StampPMsg(performance_graph, msg);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_SendPMsg(performance, msg);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 50, &msg);
    ok(!ret, "got %#lx\n", ret);
    ok(msg != NULL, "got %p\n", msg);

    time = 0xdeadbeef;
    hr = IDirectMusicPerformance_MusicToReferenceTime(performance, msg->mtTime, &time);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(msg->dwSize == sizeof(DMUS_PMSG), "got %ld\n", msg->dwSize);
    ok(msg->rtTime == time, "got %I64d\n", msg->rtTime);
    ok(msg->mtTime == 500, "got %ld\n", msg->mtTime);
    ok(msg->dwFlags & DMUS_PMSGF_REFTIME, "got %#lx\n", msg->dwFlags);
    ok(msg->dwFlags & DMUS_PMSGF_MUSICTIME, "got %#lx\n", msg->dwFlags);
    ok(msg->dwFlags & (DMUS_PMSGF_TOOL_QUEUE | DMUS_PMSGF_TOOL_IMMEDIATE), "got %#lx\n", msg->dwFlags);
    ok(!msg->dwPChannel, "got %ld\n", msg->dwPChannel);
    ok(!msg->dwVirtualTrackID, "got %ld\n", msg->dwVirtualTrackID);
    ok(msg->pTool == tool, "got %p\n", msg->pTool);
    ok(msg->pGraph == performance_graph, "got %p\n", msg->pGraph);
    ok(msg->dwType == DMUS_PMSGT_USER, "got %#lx\n", msg->dwType);
    ok(!msg->dwVoiceID, "got %ld\n", msg->dwVoiceID);
    ok(!msg->dwGroupID, "got %ld\n", msg->dwGroupID);
    ok(!msg->punkUser, "got %p\n", msg->punkUser);

    hr = IDirectMusicPerformance_FreePMsg(performance, msg);
    ok(hr == S_OK, "got %#lx\n", hr);


    /* SendPMsg converts reference time to music time if it is missing */

    hr = IDirectMusicPerformance_GetTime(performance, &time, &music_time);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicPerformance_AllocPMsg(performance, sizeof(DMUS_PMSG), &msg);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(msg->dwSize == sizeof(DMUS_PMSG), "got %ld\n", msg->dwSize);
    msg->rtTime = time;
    msg->dwFlags = DMUS_PMSGF_REFTIME;
    msg->dwType = DMUS_PMSGT_USER;
    hr = IDirectMusicGraph_StampPMsg(performance_graph, msg);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_SendPMsg(performance, msg);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 50, &msg);
    ok(!ret, "got %#lx\n", ret);
    ok(msg != NULL, "got %p\n", msg);

    music_time = 0xdeadbeef;
    hr = IDirectMusicPerformance_ReferenceToMusicTime(performance, msg->rtTime, &music_time);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(msg->dwSize == sizeof(DMUS_PMSG), "got %ld\n", msg->dwSize);
    ok(msg->rtTime == time, "got %I64d\n", msg->rtTime);
    ok(msg->mtTime == music_time, "got %ld\n", msg->mtTime);
    ok(msg->dwFlags & DMUS_PMSGF_REFTIME, "got %#lx\n", msg->dwFlags);
    ok(msg->dwFlags & DMUS_PMSGF_MUSICTIME, "got %#lx\n", msg->dwFlags);
    ok(msg->dwFlags & (DMUS_PMSGF_TOOL_QUEUE | DMUS_PMSGF_TOOL_IMMEDIATE), "got %#lx\n", msg->dwFlags);
    ok(!msg->dwPChannel, "got %ld\n", msg->dwPChannel);
    ok(!msg->dwVirtualTrackID, "got %ld\n", msg->dwVirtualTrackID);
    ok(msg->pTool == tool, "got %p\n", msg->pTool);
    ok(msg->pGraph == performance_graph, "got %p\n", msg->pGraph);
    ok(msg->dwType == DMUS_PMSGT_USER, "got %#lx\n", msg->dwType);
    ok(!msg->dwVoiceID, "got %ld\n", msg->dwVoiceID);
    ok(!msg->dwGroupID, "got %ld\n", msg->dwGroupID);
    ok(!msg->punkUser, "got %p\n", msg->punkUser);

    hr = IDirectMusicPerformance_FreePMsg(performance, msg);
    ok(hr == S_OK, "got %#lx\n", hr);


    for (i = 0; i < ARRAY_SIZE(delivery_flags); i++)
    {
        DWORD duration = 0;

        hr = IDirectMusicPerformance_GetTime(performance, &time, NULL);
        ok(hr == S_OK, "got %#lx\n", hr);
        hr = IDirectMusicPerformance_AllocPMsg(performance, sizeof(DMUS_PMSG), &msg);
        ok(hr == S_OK, "got %#lx\n", hr);
        ok(msg->dwSize == sizeof(DMUS_PMSG), "got %ld\n", msg->dwSize);
        msg->rtTime = time + 150 * 10000;
        msg->dwFlags = DMUS_PMSGF_REFTIME;
        msg->dwType = DMUS_PMSGT_USER;
        hr = IDirectMusicGraph_StampPMsg(performance_graph, msg);
        ok(hr == S_OK, "got %#lx\n", hr);

        msg->dwFlags &= ~(DMUS_PMSGF_TOOL_IMMEDIATE | DMUS_PMSGF_TOOL_QUEUE | DMUS_PMSGF_TOOL_ATTIME);
        msg->dwFlags |= delivery_flags[i];

        duration -= GetTickCount();
        hr = IDirectMusicPerformance_SendPMsg(performance, msg);
        ok(hr == S_OK, "got %#lx\n", hr);
        msg = NULL;
        ret = test_tool_wait_message(tool, 1000, &msg);
        ok(!ret, "got %#lx\n", ret);
        ok(msg != NULL, "got %p\n", msg);
        duration += GetTickCount();

        if (msg) hr = IDirectMusicPerformance_FreePMsg(performance, msg);
        ok(hr == S_OK, "got %#lx\n", hr);

        switch (delivery_flags[i])
        {
        case DMUS_PMSGF_TOOL_IMMEDIATE: ok(duration <= 50, "got %lu\n", duration); break;
        case DMUS_PMSGF_TOOL_QUEUE: ok(duration >= 50 && duration <= 125, "got %lu\n", duration); break;
        case DMUS_PMSGF_TOOL_ATTIME: ok(duration >= 125 && duration <= 500, "got %lu\n", duration); break;
        }
    }


    hr = IDirectMusicPerformance_CloseDown(performance);
    ok(hr == S_OK, "got %#lx\n", hr);


    IDirectMusicGraph_Release(performance_graph);

    IDirectMusicPerformance_Release(performance);
    IDirectMusicTool_Release(tool);
}

#define check_dmus_dirty_pmsg(a, b) check_dmus_dirty_pmsg_(__LINE__, a, b)
static void check_dmus_dirty_pmsg_(int line, DMUS_PMSG *msg, MUSIC_TIME music_time)
{
    ok_(__FILE__, line)(msg->dwSize == sizeof(*msg), "got dwSize %#lx\n", msg->dwSize);
    ok_(__FILE__, line)(abs(msg->mtTime - music_time) <= 100, "got mtTime %ld\n", msg->mtTime);
    ok_(__FILE__, line)(msg->dwFlags == (DMUS_PMSGF_MUSICTIME | DMUS_PMSGF_REFTIME | DMUS_PMSGF_TOOL_IMMEDIATE),
            "got dwFlags %#lx\n", msg->dwFlags);
    ok_(__FILE__, line)(msg->dwPChannel == 0, "got dwPChannel %#lx\n", msg->dwPChannel);
    ok_(__FILE__, line)(msg->dwVirtualTrackID == 0, "got dwVirtualTrackID %#lx\n", msg->dwVirtualTrackID);
    ok_(__FILE__, line)(msg->pTool != 0, "got pTool %p\n", msg->pTool);
    ok_(__FILE__, line)(msg->pGraph != 0, "got pGraph %p\n", msg->pGraph);
    ok_(__FILE__, line)(msg->dwType == DMUS_PMSGT_DIRTY, "got dwType %#lx\n", msg->dwType);
    ok_(__FILE__, line)(msg->dwVoiceID == 0, "got dwVoiceID %#lx\n", msg->dwVoiceID);
    todo_wine ok_(__FILE__, line)(msg->dwGroupID == -1, "got dwGroupID %#lx\n", msg->dwGroupID);
    ok_(__FILE__, line)(msg->punkUser == 0, "got punkUser %p\n", msg->punkUser);
}

#define check_dmus_notification_pmsg(a, b, c, d, e, f) check_dmus_notification_pmsg_(__LINE__, a, b, c, d, e, f)
static void check_dmus_notification_pmsg_(int line, DMUS_NOTIFICATION_PMSG *msg, MUSIC_TIME music_time, DWORD flags,
        const GUID *type, DWORD option, void *user)
{
    ok_(__FILE__, line)(msg->dwSize == sizeof(*msg), "got dwSize %#lx\n", msg->dwSize);
    ok_(__FILE__, line)(msg->rtTime > 0, "got rtTime %I64d\n", msg->rtTime);
    ok_(__FILE__, line)(abs(msg->mtTime - music_time) <= 100, "got mtTime %ld\n", msg->mtTime);
    todo_wine_if(flags == DMUS_PMSGF_TOOL_ATTIME)
    ok_(__FILE__, line)(msg->dwFlags == (DMUS_PMSGF_MUSICTIME | DMUS_PMSGF_REFTIME | flags),
            "got dwFlags %#lx\n", msg->dwFlags);
    ok_(__FILE__, line)(msg->dwPChannel == 0, "got dwPChannel %#lx\n", msg->dwPChannel);
    ok_(__FILE__, line)(msg->dwVirtualTrackID == 0, "got dwVirtualTrackID %#lx\n", msg->dwVirtualTrackID);
    if (flags == DMUS_PMSGF_TOOL_ATTIME)
    {
        todo_wine ok_(__FILE__, line)(msg->pTool == 0, "got pTool %p\n", msg->pTool);
        ok_(__FILE__, line)(msg->pGraph == 0, "got pGraph %p\n", msg->pGraph);
    }
    else
    {
        ok_(__FILE__, line)(msg->pTool != 0, "got pTool %p\n", msg->pTool);
        ok_(__FILE__, line)(msg->pGraph != 0, "got pGraph %p\n", msg->pGraph);
    }
    ok_(__FILE__, line)(msg->dwType == DMUS_PMSGT_NOTIFICATION, "got dwType %#lx\n", msg->dwType);
    ok_(__FILE__, line)(msg->dwVoiceID == 0, "got dwVoiceID %#lx\n", msg->dwVoiceID);
    todo_wine ok_(__FILE__, line)(msg->dwGroupID == -1, "got dwGroupID %#lx\n", msg->dwGroupID);
    ok_(__FILE__, line)(msg->punkUser == (IUnknown *)user, "got punkUser %p\n", msg->punkUser);

    ok_(__FILE__, line)(IsEqualGUID(&msg->guidNotificationType, type),
            "got guidNotificationType %s\n", debugstr_guid(&msg->guidNotificationType));
    ok_(__FILE__, line)(msg->dwNotificationOption == option,
            "got dwNotificationOption %#lx\n", msg->dwNotificationOption);
    ok_(__FILE__, line)(!msg->dwField1, "got dwField1 %lu\n", msg->dwField1);
    ok_(__FILE__, line)(!msg->dwField2, "got dwField2 %lu\n", msg->dwField2);

    if (!IsEqualGUID(&msg->guidNotificationType, &GUID_NOTIFICATION_SEGMENT))
        ok_(__FILE__, line)(!msg->punkUser, "got punkUser %p\n", msg->punkUser);
    else
    {
        check_interface_(line, msg->punkUser, &IID_IDirectMusicSegmentState, TRUE, FALSE);
        check_interface_(line, msg->punkUser, &IID_IDirectMusicSegmentState8, TRUE, FALSE);
    }
}

static void test_notification_pmsg(void)
{
    static const DWORD message_types[] =
    {
        DMUS_PMSGT_DIRTY,
        DMUS_PMSGT_NOTIFICATION,
        DMUS_PMSGT_WAVE,
    };
    IDirectMusicPerformance *performance;
    IDirectMusicSegmentState *state;
    DMUS_NOTIFICATION_PMSG *notif;
    MUSIC_TIME music_time, length;
    IDirectMusicSegment *segment;
    IDirectMusicTrack *track;
    IDirectMusicGraph *graph;
    IDirectMusicTool *tool;
    DMUS_PMSG *msg;
    HRESULT hr;
    DWORD ret;

    hr = test_tool_create(message_types, ARRAY_SIZE(message_types), &tool);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicPerformance, (void **)&performance);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = CoCreateInstance(&CLSID_DirectMusicGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicGraph, (void **)&graph);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_SetGraph(performance, graph);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicGraph_InsertTool(graph, tool, NULL, 0, -1);
    ok(hr == S_OK, "got %#lx\n", hr);
    IDirectMusicGraph_Release(graph);

    hr = IDirectMusicPerformance_Init(performance, NULL, 0, 0);
    ok(hr == S_OK, "got %#lx\n", hr);


    hr = CoCreateInstance(&CLSID_DirectMusicSegment, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicSegment, (void **)&segment);
    ok(hr == S_OK, "got %#lx\n", hr);


    /* native needs a track or the segment will end immediately */

    hr = test_track_create(&track, FALSE);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicSegment_InsertTrack(segment, track, 1);
    ok(hr == S_OK, "got %#lx\n", hr);
    IDirectMusicTrack_Release(track);

    /* native sends dirty / notification by batch of 1s, shorter length
     * will cause all messages to be received immediately */
    length = 10005870 / 6510;
    hr = IDirectMusicSegment_SetLength(segment, length);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicSegment8_Download((IDirectMusicSegment8 *)segment, (IUnknown *)performance);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicSegment8_Unload((IDirectMusicSegment8 *)segment, (IUnknown *)performance);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicPerformance_PlaySegment(performance, segment, 0, 0, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_GetTime(performance, NULL, &music_time);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 50, &msg);
    ok(!ret, "got %#lx\n", ret);
    check_dmus_dirty_pmsg(msg, music_time);
    hr = IDirectMusicPerformance_FreePMsg(performance, msg);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 500, &msg);
    ok(!ret, "got %#lx\n", ret);
    check_dmus_dirty_pmsg(msg, music_time + length);
    hr = IDirectMusicPerformance_FreePMsg(performance, msg);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 100, &msg);
    ok(ret == WAIT_TIMEOUT, "got %#lx\n", ret);
    ok(!msg, "got %p\n", msg);


    /* AddNotificationType is necessary to receive notification messages */

    hr = IDirectMusicPerformance_AddNotificationType(performance, &GUID_NOTIFICATION_PERFORMANCE);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_AddNotificationType(performance, &GUID_NOTIFICATION_SEGMENT);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_AddNotificationType(performance, &GUID_NOTIFICATION_SEGMENT);
    ok(hr == S_FALSE, "got %#lx\n", hr);

    /* avoid discarding older notifications */
    hr = IDirectMusicPerformance_SetNotificationHandle(performance, 0, 100000000);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicPerformance_PlaySegment(performance, segment, 0, 0, &state);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_GetTime(performance, NULL, &music_time);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 50, (DMUS_PMSG **)&notif);
    ok(!ret, "got %#lx\n", ret);
    check_dmus_notification_pmsg(notif, music_time, DMUS_PMSGF_TOOL_IMMEDIATE, &GUID_NOTIFICATION_PERFORMANCE,
            DMUS_NOTIFICATION_MUSICSTARTED, NULL);
    hr = IDirectMusicPerformance_FreePMsg(performance, (DMUS_PMSG *)notif);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 50, (DMUS_PMSG **)&notif);
    ok(!ret, "got %#lx\n", ret);
    check_dmus_notification_pmsg(notif, music_time, DMUS_PMSGF_TOOL_IMMEDIATE, &GUID_NOTIFICATION_SEGMENT,
            DMUS_NOTIFICATION_SEGSTART, state);
    hr = IDirectMusicPerformance_FreePMsg(performance, (DMUS_PMSG *)notif);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 50, &msg);
    ok(!ret, "got %#lx\n", ret);
    check_dmus_dirty_pmsg(msg, music_time);
    hr = IDirectMusicPerformance_FreePMsg(performance, msg);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 500, (DMUS_PMSG **)&notif);
    ok(!ret, "got %#lx\n", ret);
    check_dmus_notification_pmsg(notif, music_time + length - 1450, DMUS_PMSGF_TOOL_IMMEDIATE, &GUID_NOTIFICATION_SEGMENT,
            DMUS_NOTIFICATION_SEGALMOSTEND, state);
    hr = IDirectMusicPerformance_FreePMsg(performance, (DMUS_PMSG *)notif);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 50, (DMUS_PMSG **)&notif);
    ok(!ret, "got %#lx\n", ret);
    check_dmus_notification_pmsg(notif, music_time + length, DMUS_PMSGF_TOOL_IMMEDIATE, &GUID_NOTIFICATION_SEGMENT,
            DMUS_NOTIFICATION_SEGEND, state);
    hr = IDirectMusicPerformance_FreePMsg(performance, (DMUS_PMSG *)notif);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 50, &msg);
    ok(!ret, "got %#lx\n", ret);
    check_dmus_dirty_pmsg(msg, music_time + length);
    hr = IDirectMusicPerformance_FreePMsg(performance, msg);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 50, (DMUS_PMSG **)&notif);
    ok(!ret, "got %#lx\n", ret);
    check_dmus_notification_pmsg(notif, music_time + length, DMUS_PMSGF_TOOL_IMMEDIATE, &GUID_NOTIFICATION_PERFORMANCE,
            DMUS_NOTIFICATION_MUSICSTOPPED, NULL);
    hr = IDirectMusicPerformance_FreePMsg(performance, (DMUS_PMSG *)notif);
    ok(hr == S_OK, "got %#lx\n", hr);


    /* wait enough time for notifications to be delivered */

    ret = test_tool_wait_message(tool, 2000, &msg);
    ok(ret == WAIT_TIMEOUT, "got %#lx\n", ret);
    ok(!msg, "got %p\n", msg);


    /* notification messages are also queued for direct access */

    hr = IDirectMusicPerformance_GetNotificationPMsg(performance, &notif);
    ok(hr == S_OK, "got %#lx\n", hr);
    check_dmus_notification_pmsg(notif, music_time, DMUS_PMSGF_TOOL_ATTIME, &GUID_NOTIFICATION_PERFORMANCE,
            DMUS_NOTIFICATION_MUSICSTARTED, NULL);
    hr = IDirectMusicPerformance_FreePMsg(performance, (DMUS_PMSG *)notif);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicPerformance_GetNotificationPMsg(performance, &notif);
    ok(hr == S_OK, "got %#lx\n", hr);
    check_dmus_notification_pmsg(notif, music_time, DMUS_PMSGF_TOOL_ATTIME, &GUID_NOTIFICATION_SEGMENT,
            DMUS_NOTIFICATION_SEGSTART, state);
    hr = IDirectMusicPerformance_FreePMsg(performance, (DMUS_PMSG *)notif);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicPerformance_GetNotificationPMsg(performance, &notif);
    ok(hr == S_OK, "got %#lx\n", hr);
    check_dmus_notification_pmsg(notif, music_time + length - 1450, DMUS_PMSGF_TOOL_ATTIME, &GUID_NOTIFICATION_SEGMENT,
            DMUS_NOTIFICATION_SEGALMOSTEND, state);
    hr = IDirectMusicPerformance_FreePMsg(performance, (DMUS_PMSG *)notif);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicPerformance_GetNotificationPMsg(performance, &notif);
    ok(hr == S_OK, "got %#lx\n", hr);
    check_dmus_notification_pmsg(notif, music_time + length, DMUS_PMSGF_TOOL_ATTIME, &GUID_NOTIFICATION_SEGMENT,
            DMUS_NOTIFICATION_SEGEND, state);
    hr = IDirectMusicPerformance_FreePMsg(performance, (DMUS_PMSG *)notif);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicPerformance_GetNotificationPMsg(performance, &notif);
    ok(hr == S_OK, "got %#lx\n", hr);
    check_dmus_notification_pmsg(notif, music_time + length, DMUS_PMSGF_TOOL_ATTIME, &GUID_NOTIFICATION_PERFORMANCE,
            DMUS_NOTIFICATION_MUSICSTOPPED, NULL);
    hr = IDirectMusicPerformance_FreePMsg(performance, (DMUS_PMSG *)notif);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicPerformance_GetNotificationPMsg(performance, &notif);
    ok(hr == S_FALSE, "got %#lx\n", hr);

    IDirectMusicSegmentState_Release(state);

    hr = IDirectMusicPerformance_RemoveNotificationType(performance, &GUID_NOTIFICATION_PERFORMANCE);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_RemoveNotificationType(performance, &GUID_NOTIFICATION_SEGMENT);
    ok(hr == S_OK, "got %#lx\n", hr);


    /* RemoveNotificationType returns S_FALSE if already removed */

    hr = IDirectMusicPerformance_RemoveNotificationType(performance, &GUID_NOTIFICATION_PERFORMANCE);
    ok(hr == S_FALSE, "got %#lx\n", hr);


    /* Stop finishes segment immediately and skips notification messages */

    hr = IDirectMusicPerformance_AddNotificationType(performance, &GUID_NOTIFICATION_SEGMENT);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_PlaySegment(performance, segment, 0, 0, &state);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_GetTime(performance, NULL, &music_time);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 50, (DMUS_PMSG **)&notif);
    ok(!ret, "got %#lx\n", ret);
    check_dmus_notification_pmsg(notif, music_time, DMUS_PMSGF_TOOL_IMMEDIATE, &GUID_NOTIFICATION_SEGMENT,
            DMUS_NOTIFICATION_SEGSTART, state);
    hr = IDirectMusicPerformance_FreePMsg(performance, (DMUS_PMSG *)notif);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicPerformance_Stop(performance, NULL, NULL, 0, DMUS_SEGF_DEFAULT);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_RemoveNotificationType(performance, &GUID_NOTIFICATION_SEGMENT);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 50, &msg);
    ok(!ret, "got %#lx\n", ret);
    ok(msg->dwType == DMUS_PMSGT_DIRTY, "got %#lx\n", msg->dwType);
    hr = IDirectMusicPerformance_FreePMsg(performance, msg);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 50, (DMUS_PMSG **)&notif);
    ok(!ret, "got %#lx\n", ret);
    if (notif->dwNotificationOption == DMUS_NOTIFICATION_SEGALMOSTEND)
    {
        check_dmus_notification_pmsg(notif, music_time + length - 1450, DMUS_PMSGF_TOOL_IMMEDIATE,
                &GUID_NOTIFICATION_SEGMENT, DMUS_NOTIFICATION_SEGALMOSTEND, state);
        hr = IDirectMusicPerformance_FreePMsg(performance, (DMUS_PMSG *)notif);
        ok(hr == S_OK, "got %#lx\n", hr);

        ret = test_tool_wait_message(tool, 50, (DMUS_PMSG **)&notif);
        ok(!ret, "got %#lx\n", ret);
        check_dmus_notification_pmsg(notif, music_time + length, DMUS_PMSGF_TOOL_IMMEDIATE,
                &GUID_NOTIFICATION_SEGMENT, DMUS_NOTIFICATION_SEGEND, state);
        hr = IDirectMusicPerformance_FreePMsg(performance, (DMUS_PMSG *)notif);
        ok(hr == S_OK, "got %#lx\n", hr);

        ret = test_tool_wait_message(tool, 50, &msg);
        ok(!ret, "got %#lx\n", ret);
        check_dmus_dirty_pmsg(msg, music_time + length);
        hr = IDirectMusicPerformance_FreePMsg(performance, msg);
        ok(hr == S_OK, "got %#lx\n", hr);

        ret = test_tool_wait_message(tool, 50, (DMUS_PMSG **)&notif);
        ok(!ret, "got %#lx\n", ret);
    }
    check_dmus_notification_pmsg(notif, music_time, DMUS_PMSGF_TOOL_IMMEDIATE, &GUID_NOTIFICATION_SEGMENT,
            DMUS_NOTIFICATION_SEGABORT, state);
    hr = IDirectMusicPerformance_FreePMsg(performance, (DMUS_PMSG *)notif);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 500, &msg);
    ok(!ret, "got %#lx\n", ret);
    ok(msg->dwType == DMUS_PMSGT_DIRTY, "got dwType %#lx\n", msg->dwType);
    hr = IDirectMusicPerformance_FreePMsg(performance, msg);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 100, &msg);
    ok(ret == WAIT_TIMEOUT, "got %#lx\n", ret);

    IDirectMusicSegmentState_Release(state);


    /* CloseDown removes all notifications and notification messages */

    hr = IDirectMusicPerformance_AddNotificationType(performance, &GUID_NOTIFICATION_SEGMENT);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_PlaySegment(performance, segment, 0, 0, &state);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_GetTime(performance, NULL, &music_time);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 50, (DMUS_PMSG **)&notif);
    ok(!ret, "got %#lx\n", ret);
    check_dmus_notification_pmsg(notif, music_time, DMUS_PMSGF_TOOL_IMMEDIATE, &GUID_NOTIFICATION_SEGMENT,
            DMUS_NOTIFICATION_SEGSTART, state);
    hr = IDirectMusicPerformance_FreePMsg(performance, (DMUS_PMSG *)notif);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicPerformance_CloseDown(performance);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_GetNotificationPMsg(performance, &notif);
    ok(hr == S_FALSE, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_RemoveNotificationType(performance, &GUID_NOTIFICATION_SEGMENT);
    ok(hr == S_FALSE, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 50, &msg);
    ok(!ret, "got %#lx\n", ret);
    check_dmus_dirty_pmsg(msg, music_time);
    hr = IDirectMusicPerformance_FreePMsg(performance, msg);
    ok(hr == S_OK, "got %#lx\n", hr);

    IDirectMusicSegmentState_Release(state);
    IDirectMusicSegment_Release(segment);


    IDirectMusicPerformance_Release(performance);
    IDirectMusicTool_Release(tool);
}

static void test_wave_pmsg(void)
{
    static const DWORD message_types[] =
    {
        DMUS_PMSGT_DIRTY,
        DMUS_PMSGT_WAVE,
    };
    IDirectMusicPerformance *performance;
    IDirectMusicSegment *segment;
    IDirectMusicLoader8 *loader;
    IDirectMusicGraph *graph;
    WCHAR test_wav[MAX_PATH];
    IDirectMusicTool *tool;
    DMUS_WAVE_PMSG *wave;
    MUSIC_TIME length;
    DMUS_PMSG *msg;
    HRESULT hr;
    DWORD ret;

    hr = test_tool_create(message_types, ARRAY_SIZE(message_types), &tool);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicPerformance, (void **)&performance);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = CoCreateInstance(&CLSID_DirectMusicGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicGraph, (void **)&graph);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_SetGraph(performance, graph);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicGraph_InsertTool(graph, tool, NULL, 0, -1);
    ok(hr == S_OK, "got %#lx\n", hr);
    IDirectMusicGraph_Release(graph);

    hr = IDirectMusicPerformance8_InitAudio((IDirectMusicPerformance8 *)performance, NULL, NULL, NULL,
            DMUS_APATH_SHARED_STEREOPLUSREVERB, 64, DMUS_AUDIOF_ALL, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);


    load_resource(L"test.wav", test_wav);

    hr = CoCreateInstance(&CLSID_DirectMusicLoader, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicLoader8, (void **)&loader);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicLoader8_LoadObjectFromFile(loader, &CLSID_DirectMusicSegment,
            &IID_IDirectMusicSegment, test_wav, (void **)&segment);
    ok(hr == S_OK, "got %#lx\n", hr);
    IDirectMusicLoader8_Release(loader);


    length = 0xdeadbeef;
    hr = IDirectMusicSegment_GetLength(segment, &length);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(length == 1, "got %lu\n", length);


    /* without Download, no DMUS_PMSGT_WAVE is sent */

    hr = IDirectMusicPerformance_PlaySegment(performance, segment, 0, 0, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 500, &msg);
    ok(!ret, "got %#lx\n", ret);
    ok(msg->dwType == DMUS_PMSGT_DIRTY, "got %p\n", msg);
    hr = IDirectMusicPerformance_FreePMsg(performance, msg);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 500, &msg);
    ok(!ret, "got %#lx\n", ret);
    ok(msg->dwType == DMUS_PMSGT_DIRTY, "got %p\n", msg);
    hr = IDirectMusicPerformance_FreePMsg(performance, msg);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 100, &msg);
    ok(ret == WAIT_TIMEOUT, "got %#lx\n", ret);
    ok(!msg, "got %p\n", msg);


    /* a single DMUS_PMSGT_WAVE message is sent with punkUser set */

    hr = IDirectMusicSegment8_Download((IDirectMusicSegment8 *)segment, (IUnknown *)performance);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicPerformance_PlaySegment(performance, segment, 0, 0, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 500, &msg);
    ok(!ret, "got %#lx\n", ret);
    ok(msg->dwType == DMUS_PMSGT_DIRTY, "got %p\n", msg);
    hr = IDirectMusicPerformance_FreePMsg(performance, msg);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 500, (DMUS_PMSG **)&wave);
    ok(!ret, "got %#lx\n", ret);
    ok(wave->dwType == DMUS_PMSGT_WAVE, "got %p\n", wave);
    ok(!!wave->punkUser, "got %p\n", wave->punkUser);
    ok(wave->rtStartOffset == 0, "got %I64d\n", wave->rtStartOffset);
    ok(wave->rtDuration == 1000000, "got %I64d\n", wave->rtDuration);
    ok(wave->lOffset == 0, "got %lu\n", wave->lOffset);
    ok(wave->lVolume == 0, "got %lu\n", wave->lVolume);
    ok(wave->lPitch == 0, "got %lu\n", wave->lPitch);
    ok(wave->bFlags == 0, "got %#x\n", wave->bFlags);
    hr = IDirectMusicPerformance_FreePMsg(performance, (DMUS_PMSG *)wave);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 500, &msg);
    ok(!ret, "got %#lx\n", ret);
    ok(msg->dwType == DMUS_PMSGT_DIRTY, "got %p\n", msg);
    hr = IDirectMusicPerformance_FreePMsg(performance, msg);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicSegment8_Unload((IDirectMusicSegment8 *)segment, (IUnknown *)performance);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 100, &msg);
    ok(ret == WAIT_TIMEOUT, "got %#lx\n", ret);
    ok(!msg, "got %p\n", msg);


    IDirectMusicSegment_Release(segment);


    hr = IDirectMusicPerformance_CloseDown(performance);
    ok(hr == S_OK, "got %#lx\n", hr);

    IDirectMusicPerformance_Release(performance);
    IDirectMusicTool_Release(tool);
}

static void test_sequence_track(void)
{
    static const DWORD message_types[] =
    {
        DMUS_PMSGT_MIDI,
        DMUS_PMSGT_NOTE,
        DMUS_PMSGT_CURVE,
        DMUS_PMSGT_DIRTY,
    };
    static const LARGE_INTEGER zero = {0};
    IDirectMusicPerformance *performance;
    IDirectMusicSegment *segment;
    IDirectMusicGraph *graph;
    IDirectMusicTrack *track;
    IPersistStream *persist;
    IDirectMusicTool *tool;
    DMUS_NOTE_PMSG *note;
    IStream *stream;
    DMUS_PMSG *msg;
    HRESULT hr;
    DWORD ret;

    hr = test_tool_create(message_types, ARRAY_SIZE(message_types), &tool);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicPerformance, (void **)&performance);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = CoCreateInstance(&CLSID_DirectMusicGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicGraph, (void **)&graph);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicGraph_InsertTool(graph, (IDirectMusicTool *)tool, NULL, 0, -1);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_SetGraph(performance, graph);
    ok(hr == S_OK, "got %#lx\n", hr);
    IDirectMusicGraph_Release(graph);


    /* create a segment and load a simple RIFF stream */

    hr = CoCreateInstance(&CLSID_DirectMusicSegment, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicSegment, (void **)&segment);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicSegment_QueryInterface(segment, &IID_IPersistStream, (void **)&persist);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = CreateStreamOnHGlobal(0, TRUE, &stream);
    ok(hr == S_OK, "got %#lx\n", hr);

    CHUNK_RIFF(stream, "DMSG")
    {
        /* set a non-zero segment length, or nothing will be played */
        DMUS_IO_SEGMENT_HEADER head = {.mtLength = 2000};
        CHUNK_DATA(stream, "segh", head);
        CHUNK_DATA(stream, "guid", CLSID_DirectMusicSegment);
    }
    CHUNK_END;

    hr = IStream_Seek(stream, zero, 0, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IPersistStream_Load(persist, stream);
    ok(hr == S_OK, "got %#lx\n", hr);
    IPersistStream_Release(persist);
    IStream_Release(stream);


    /* add a sequence track to our segment */

    hr = CoCreateInstance(&CLSID_DirectMusicSeqTrack, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicTrack, (void **)&track);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicSegment_QueryInterface(track, &IID_IPersistStream, (void **)&persist);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = CreateStreamOnHGlobal(0, TRUE, &stream);
    ok(hr == S_OK, "got %#lx\n", hr);

    CHUNK_BEGIN(stream, "seqt")
    {
        DMUS_IO_SEQ_ITEM items[] =
        {
            {.mtTime = 0, .mtDuration = 500, .dwPChannel = 0, .bStatus = 0x90, .bByte1 = 60, .bByte2 = 120},
            {.mtTime = 1000, .mtDuration = 200, .dwPChannel = 1, .bStatus = 0x90, .bByte1 = 50, .bByte2 = 100},
        };
        CHUNK_ARRAY(stream, "evtl", items);
    }
    CHUNK_END;

    hr = IStream_Seek(stream, zero, 0, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IPersistStream_Load(persist, stream);
    ok(hr == S_OK, "got %#lx\n", hr);
    IPersistStream_Release(persist);
    IStream_Release(stream);

    hr = IDirectMusicSegment_InsertTrack(segment, (IDirectMusicTrack *)track, 1);
    ok(hr == S_OK, "got %#lx\n", hr);
    IDirectMusicTrack_Release(track);


    /* now play the segment, and check produced messages */

    hr = IDirectMusicPerformance_Init(performance, NULL, 0, 0);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicPerformance_PlaySegment(performance, segment, 0x800, 0, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 500, &msg);
    ok(!ret, "got %#lx\n", ret);
    ok(msg->dwType == DMUS_PMSGT_DIRTY, "got %#lx\n", msg->dwType);
    hr = IDirectMusicPerformance_FreePMsg(performance, msg);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 500, (DMUS_PMSG **)&note);
    ok(!ret, "got %#lx\n", ret);
    check_dmus_note_pmsg(note, 0, 0, 500, 60, 120, DMUS_NOTEF_NOTEON);
    hr = IDirectMusicPerformance_FreePMsg(performance, (DMUS_PMSG *)note);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 500, (DMUS_PMSG **)&note);
    ok(!ret, "got %#lx\n", ret);
    check_dmus_note_pmsg(note, 1000, 1, 200, 50, 100, DMUS_NOTEF_NOTEON);
    hr = IDirectMusicPerformance_FreePMsg(performance, (DMUS_PMSG *)note);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 500, &msg);
    ok(!ret, "got %#lx\n", ret);
    ok(msg->dwType == DMUS_PMSGT_DIRTY, "got %#lx\n", msg->dwType);
    hr = IDirectMusicPerformance_FreePMsg(performance, msg);
    ok(hr == S_OK, "got %#lx\n", hr);

    IDirectMusicSegment_Release(segment);


    hr = IDirectMusicPerformance_CloseDown(performance);
    ok(hr == S_OK, "got %#lx\n", hr);

    IDirectMusicPerformance_Release(performance);
    IDirectMusicTool_Release(tool);
}

#define check_dmus_patch_pmsg(a, b, c, d, e) check_dmus_patch_pmsg_(__LINE__, a, b, c, d, e)
static void check_dmus_patch_pmsg_(int line, DMUS_PATCH_PMSG *msg, MUSIC_TIME time, UINT chan,
        UINT bank, UINT patch)
{
    ok_(__FILE__, line)(msg->dwSize == sizeof(*msg), "got dwSize %lu\n", msg->dwSize);
    ok_(__FILE__, line)(msg->rtTime != 0, "got rtTime %I64u\n", msg->rtTime);
    ok_(__FILE__, line)(abs(msg->mtTime - time) < 10, "got mtTime %lu\n", msg->mtTime);
    ok_(__FILE__, line)(msg->dwPChannel == chan, "got dwPChannel %lu\n", msg->dwPChannel);
    ok_(__FILE__, line)(!!msg->dwVirtualTrackID, "got dwVirtualTrackID %lu\n", msg->dwVirtualTrackID);
    ok_(__FILE__, line)(msg->dwType == DMUS_PMSGT_PATCH, "got %#lx\n", msg->dwType);
    ok_(__FILE__, line)(!msg->dwVoiceID, "got dwVoiceID %lu\n", msg->dwVoiceID);
    ok_(__FILE__, line)(msg->dwGroupID == 1, "got dwGroupID %lu\n", msg->dwGroupID);
    ok_(__FILE__, line)(!msg->punkUser, "got punkUser %p\n", msg->punkUser);
    ok_(__FILE__, line)(msg->byInstrument == patch, "got byInstrument %u\n", msg->byInstrument);
    ok_(__FILE__, line)(msg->byMSB == bank >> 8, "got byMSB %u\n", msg->byMSB);
    ok_(__FILE__, line)(msg->byLSB == (bank & 0xff), "got byLSB %u\n", msg->byLSB);
}

static void test_band_track_play(void)
{
    static const DWORD message_types[] =
    {
        DMUS_PMSGT_MIDI,
        DMUS_PMSGT_NOTE,
        DMUS_PMSGT_SYSEX,
        DMUS_PMSGT_NOTIFICATION,
        DMUS_PMSGT_TEMPO,
        DMUS_PMSGT_CURVE,
        DMUS_PMSGT_TIMESIG,
        DMUS_PMSGT_PATCH,
        DMUS_PMSGT_TRANSPOSE,
        DMUS_PMSGT_CHANNEL_PRIORITY,
        DMUS_PMSGT_STOP,
        DMUS_PMSGT_DIRTY,
        DMUS_PMSGT_WAVE,
        DMUS_PMSGT_LYRIC,
        DMUS_PMSGT_SCRIPTLYRIC,
        DMUS_PMSGT_USER,
    };
    DMUS_OBJECTDESC desc =
    {
        .dwSize = sizeof(DMUS_OBJECTDESC),
        .dwValidData = DMUS_OBJ_OBJECT | DMUS_OBJ_CLASS | DMUS_OBJ_FILENAME | DMUS_OBJ_FULLPATH,
        .guidClass = CLSID_DirectMusicCollection,
        .guidObject = GUID_DefaultGMCollection,
        .wszFileName = L"C:\\windows\\system32\\drivers\\gm.dls",
    };
    static const LARGE_INTEGER zero = {0};
    IDirectMusicPerformance *performance;
    IStream *stream, *loader_stream;
    IDirectMusicSegment *segment;
    IDirectMusicLoader *loader;
    IDirectMusicGraph *graph;
    IDirectMusicTrack *track;
    IPersistStream *persist;
    IDirectMusicTool *tool;
    DMUS_PATCH_PMSG *patch;
    DMUS_PMSG *msg;
    HRESULT hr;
    DWORD ret;

    hr = test_tool_create(message_types, ARRAY_SIZE(message_types), &tool);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicPerformance, (void **)&performance);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = CoCreateInstance(&CLSID_DirectMusicGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicGraph, (void **)&graph);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicGraph_InsertTool(graph, (IDirectMusicTool *)tool, NULL, 0, -1);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_SetGraph(performance, graph);
    ok(hr == S_OK, "got %#lx\n", hr);
    IDirectMusicGraph_Release(graph);


    /* create a segment and load a simple RIFF stream */

    hr = CoCreateInstance(&CLSID_DirectMusicSegment, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicSegment, (void **)&segment);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicSegment_QueryInterface(segment, &IID_IPersistStream, (void **)&persist);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = CreateStreamOnHGlobal(0, TRUE, &stream);
    ok(hr == S_OK, "got %#lx\n", hr);

    CHUNK_RIFF(stream, "DMSG")
    {
        /* set a non-zero segment length, or nothing will be played */
        DMUS_IO_SEGMENT_HEADER head = {.mtLength = 2000};
        CHUNK_DATA(stream, "segh", head);
        CHUNK_DATA(stream, "guid", CLSID_DirectMusicSegment);
    }
    CHUNK_END;

    hr = IStream_Seek(stream, zero, 0, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IPersistStream_Load(persist, stream);
    ok(hr == S_OK, "got %#lx\n", hr);
    IPersistStream_Release(persist);
    IStream_Release(stream);


    /* add a sequence track to our segment */

    hr = CoCreateInstance(&CLSID_DirectMusicBandTrack, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicTrack, (void **)&track);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicSegment_QueryInterface(track, &IID_IPersistStream, (void **)&persist);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = CreateStreamOnHGlobal(0, TRUE, &stream);
    ok(hr == S_OK, "got %#lx\n", hr);

    CHUNK_RIFF(stream, "DMBT")
    {
        DMUS_IO_BAND_TRACK_HEADER head = {.bAutoDownload = TRUE};

        CHUNK_DATA(stream, "bdth", head);
        CHUNK_LIST(stream, "lbdl")
        {
            CHUNK_LIST(stream, "lbnd")
            {
                DMUS_IO_BAND_ITEM_HEADER head = {.lBandTime = 0};
                CHUNK_DATA(stream, "bdih", head);

                CHUNK_RIFF(stream, "DMBD")
                {
                    GUID guid = CLSID_DirectMusicBand;
                    CHUNK_DATA(stream, "guid", guid);

                    CHUNK_LIST(stream, "lbil")
                    {
                        CHUNK_LIST(stream, "lbin")
                        {
                            DMUS_IO_INSTRUMENT head = {.dwPatch = 1, .dwPChannel = 1, .dwFlags = DMUS_IO_INST_PATCH};
                            CHUNK_DATA(stream, "bins", head);
                        }
                        CHUNK_END;
                    }
                    CHUNK_END;
                }
                CHUNK_END;
            }
            CHUNK_END;

            CHUNK_LIST(stream, "lbnd")
            {
                DMUS_IO_BAND_ITEM_HEADER head = {.lBandTime = 1000};
                CHUNK_DATA(stream, "bdih", head);

                CHUNK_RIFF(stream, "DMBD")
                {
                    GUID guid = CLSID_DirectMusicBand;
                    CHUNK_DATA(stream, "guid", guid);

                    CHUNK_LIST(stream, "lbil")
                    {
                        CHUNK_LIST(stream, "lbin")
                        {
                            DMUS_IO_INSTRUMENT head = {.dwPatch = 2, .dwPChannel = 1, .dwFlags = DMUS_IO_INST_PATCH};
                            CHUNK_DATA(stream, "bins", head);
                        }
                        CHUNK_END;

                        CHUNK_LIST(stream, "lbin")
                        {
                            DMUS_IO_INSTRUMENT head = {.dwPatch = 3, .dwPChannel = 2, .dwFlags = DMUS_IO_INST_PATCH};
                            CHUNK_DATA(stream, "bins", head);
                        }
                        CHUNK_END;
                    }
                    CHUNK_END;
                }
                CHUNK_END;
            }
            CHUNK_END;
        }
        CHUNK_END;
    }
    CHUNK_END;

    hr = IStream_Seek(stream, zero, 0, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);

    /* band track requires the stream to implement IDirectMusicGetLoader */

    hr = CoCreateInstance(&CLSID_DirectMusicLoader, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicLoader8, (void **)&loader);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicLoader_SetObject(loader, &desc);
    if (hr == DMUS_E_LOADER_FAILEDOPEN)
        skip("Failed to open gm.dls, missing system SoundFont?\n");
    else
        ok(hr == S_OK, "got %#lx\n", hr);

    hr = test_loader_stream_create(stream, loader, &loader_stream);
    ok(hr == S_OK, "got %#lx\n", hr);
    IDirectMusicLoader8_Release(loader);

    hr = IPersistStream_Load(persist, loader_stream);
    ok(hr == S_OK, "got %#lx\n", hr);
    IStream_Release(loader_stream);

    IPersistStream_Release(persist);
    IStream_Release(stream);

    hr = IDirectMusicSegment_InsertTrack(segment, (IDirectMusicTrack *)track, 1);
    ok(hr == S_OK, "got %#lx\n", hr);
    IDirectMusicTrack_Release(track);


    /* now play the segment, and check produced messages */

    hr = IDirectMusicPerformance_Init(performance, NULL, 0, 0);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicPerformance_PlaySegment(performance, segment, 0x800, 0, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 500, &msg);
    ok(!ret, "got %#lx\n", ret);
    ok(msg->dwType == DMUS_PMSGT_DIRTY, "got %#lx\n", msg->dwType);
    hr = IDirectMusicPerformance_FreePMsg(performance, msg);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 500, (DMUS_PMSG **)&patch);
    ok(!ret, "got %#lx\n", ret);
    check_dmus_patch_pmsg(patch, 0, 1, 0, 1);
    hr = IDirectMusicPerformance_FreePMsg(performance, (DMUS_PMSG *)patch);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 500, (DMUS_PMSG **)&patch);
    ok(!ret, "got %#lx\n", ret);
    check_dmus_patch_pmsg(patch, 1000, 2, 0, 3);
    hr = IDirectMusicPerformance_FreePMsg(performance, (DMUS_PMSG *)patch);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 500, (DMUS_PMSG **)&patch);
    ok(!ret, "got %#lx\n", ret);
    check_dmus_patch_pmsg(patch, 1000, 1, 0, 2);
    hr = IDirectMusicPerformance_FreePMsg(performance, (DMUS_PMSG *)patch);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 500, &msg);
    ok(!ret, "got %#lx\n", ret);
    ok(msg->dwType == DMUS_PMSGT_DIRTY, "got %#lx\n", msg->dwType);
    hr = IDirectMusicPerformance_FreePMsg(performance, msg);
    ok(hr == S_OK, "got %#lx\n", hr);

    IDirectMusicSegment_Release(segment);


    hr = IDirectMusicPerformance_CloseDown(performance);
    ok(hr == S_OK, "got %#lx\n", hr);

    IDirectMusicPerformance_Release(performance);
    IDirectMusicTool_Release(tool);
}

#define check_dmus_tempo_pmsg(a, b, c) check_dmus_tempo_pmsg_(__LINE__, a, b, c)
static void check_dmus_tempo_pmsg_(int line, DMUS_TEMPO_PMSG *msg, MUSIC_TIME time, double tempo)
{
    ok_(__FILE__, line)(msg->dwSize == sizeof(*msg), "got dwSize %lu\n", msg->dwSize);
    ok_(__FILE__, line)(msg->rtTime != 0, "got rtTime %I64u\n", msg->rtTime);
    ok_(__FILE__, line)(abs(msg->mtTime - time) < 10, "got mtTime %lu\n", msg->mtTime);
    ok_(__FILE__, line)(!msg->dwPChannel, "got dwPChannel %lu\n", msg->dwPChannel);
    ok_(__FILE__, line)(!!msg->dwVirtualTrackID, "got dwVirtualTrackID %lu\n", msg->dwVirtualTrackID);
    ok_(__FILE__, line)(msg->dwType == DMUS_PMSGT_TEMPO, "got dwType %#lx\n", msg->dwType);
    ok_(__FILE__, line)(!msg->dwVoiceID, "got dwVoiceID %lu\n", msg->dwVoiceID);
    ok_(__FILE__, line)(msg->dwGroupID == -1, "got dwGroupID %lu\n", msg->dwGroupID);
    ok_(__FILE__, line)(!msg->punkUser, "got punkUser %p\n", msg->punkUser);
    ok_(__FILE__, line)(msg->dblTempo == tempo, "got tempo %f\n", msg->dblTempo);
}

static void test_tempo_track_play(void)
{
    static const DWORD message_types[] =
    {
        DMUS_PMSGT_MIDI,
        DMUS_PMSGT_NOTE,
        DMUS_PMSGT_SYSEX,
        DMUS_PMSGT_NOTIFICATION,
        DMUS_PMSGT_TEMPO,
        DMUS_PMSGT_CURVE,
        DMUS_PMSGT_TIMESIG,
        DMUS_PMSGT_PATCH,
        DMUS_PMSGT_TRANSPOSE,
        DMUS_PMSGT_CHANNEL_PRIORITY,
        DMUS_PMSGT_STOP,
        DMUS_PMSGT_DIRTY,
        DMUS_PMSGT_WAVE,
        DMUS_PMSGT_LYRIC,
        DMUS_PMSGT_SCRIPTLYRIC,
        DMUS_PMSGT_USER,
    };
    static const LARGE_INTEGER zero = {0};
    DMUS_IO_TEMPO_ITEM tempo_items[] =
    {
        {.lTime = 100, .dblTempo = 80},
        {.lTime = 300, .dblTempo = 60},
        {.lTime = 200, .dblTempo = 20},
        {.lTime = 4000, .dblTempo = 50},
    };
    IDirectMusicPerformance *performance;
    MUSIC_TIME music_time, next_time;
    REFERENCE_TIME init_time, time;
    IDirectMusicSegment *segment;
    IDirectMusicGraph *graph;
    IDirectMusicTrack *track;
    IPersistStream *persist;
    IDirectMusicTool *tool;
    DMUS_TEMPO_PMSG *tempo;
    DMUS_TEMPO_PARAM param;
    IStream *stream;
    DMUS_PMSG *msg;
    HRESULT hr;
    DWORD ret;

    hr = test_tool_create(message_types, ARRAY_SIZE(message_types), &tool);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicPerformance, (void **)&performance);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = CoCreateInstance(&CLSID_DirectMusicGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicGraph, (void **)&graph);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicGraph_InsertTool(graph, (IDirectMusicTool *)tool, NULL, 0, -1);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_SetGraph(performance, graph);
    ok(hr == S_OK, "got %#lx\n", hr);
    IDirectMusicGraph_Release(graph);


    /* create a segment and load a simple RIFF stream */

    hr = CoCreateInstance(&CLSID_DirectMusicSegment, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicSegment, (void **)&segment);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicSegment_QueryInterface(segment, &IID_IPersistStream, (void **)&persist);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = CreateStreamOnHGlobal(0, TRUE, &stream);
    ok(hr == S_OK, "got %#lx\n", hr);

    CHUNK_RIFF(stream, "DMSG")
    {
        /* set a non-zero segment length, or nothing will be played */
        DMUS_IO_SEGMENT_HEADER head = {.mtLength = 1000};
        CHUNK_DATA(stream, "segh", head);
        CHUNK_DATA(stream, "guid", CLSID_DirectMusicSegment);
    }
    CHUNK_END;

    hr = IStream_Seek(stream, zero, 0, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IPersistStream_Load(persist, stream);
    ok(hr == S_OK, "got %#lx\n", hr);
    IPersistStream_Release(persist);
    IStream_Release(stream);


    /* add a tempo track to our segment */

    hr = CoCreateInstance(&CLSID_DirectMusicTempoTrack, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicTrack, (void **)&track);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicSegment_QueryInterface(track, &IID_IPersistStream, (void **)&persist);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = CreateStreamOnHGlobal(0, TRUE, &stream);
    ok(hr == S_OK, "got %#lx\n", hr);
    CHUNK_ARRAY(stream, "tetr", tempo_items);
    hr = IStream_Seek(stream, zero, 0, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IPersistStream_Load(persist, stream);
    ok(hr == S_OK, "got %#lx\n", hr);
    IPersistStream_Release(persist);
    IStream_Release(stream);

    hr = IDirectMusicSegment_GetParam(segment, &GUID_TempoParam, -1, DMUS_SEG_ALLTRACKS, 0, NULL, &param);
    ok(hr == DMUS_E_TRACK_NOT_FOUND, "got %#lx\n", hr);

    hr = IDirectMusicSegment_InsertTrack(segment, (IDirectMusicTrack *)track, 1);
    ok(hr == S_OK, "got %#lx\n", hr);
    IDirectMusicTrack_Release(track);

    hr = IDirectMusicSegment_GetParam(segment, &GUID_TempoParam, -1, DMUS_SEG_ALLTRACKS, 0, NULL, NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);
    hr = IDirectMusicSegment_GetParam(segment, &GUID_TempoParam, -1, DMUS_SEG_ALLTRACKS, 0, NULL, &param);
    ok(hr == S_OK, "got %#lx\n", hr);

    memset(&param, 0xcd, sizeof(param));
    next_time = 0xdeadbeef;
    hr = IDirectMusicSegment_GetParam(segment, &GUID_TempoParam, -1, DMUS_SEG_ALLTRACKS, 0, &next_time, &param);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(next_time == 100, "got next_time %lu\n", next_time);
    ok(param.mtTime == 100, "got mtTime %ld\n", param.mtTime);
    ok(param.dblTempo == 80, "got dblTempo %f\n", param.dblTempo);
    hr = IDirectMusicSegment_GetParam(segment, &GUID_TempoParam, -1, DMUS_SEG_ALLTRACKS, 100, &next_time, &param);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(next_time == 200, "got next_time %lu\n", next_time);
    ok(param.mtTime == 0, "got mtTime %ld\n", param.mtTime);
    ok(param.dblTempo == 80, "got dblTempo %f\n", param.dblTempo);
    hr = IDirectMusicSegment_GetParam(segment, &GUID_TempoParam, -1, DMUS_SEG_ALLTRACKS, 199, &next_time, &param);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(next_time == 101, "got next_time %lu\n", next_time);
    ok(param.mtTime == -99, "got mtTime %ld\n", param.mtTime);
    ok(param.dblTempo == 80, "got dblTempo %f\n", param.dblTempo);
    hr = IDirectMusicSegment_GetParam(segment, &GUID_TempoParam, -1, DMUS_SEG_ALLTRACKS, 200, &next_time, &param);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(next_time == 100, "got next_time %lu\n", next_time);
    ok(param.mtTime == -100, "got mtTime %ld\n", param.mtTime);
    ok(param.dblTempo == 80, "got dblTempo %f\n", param.dblTempo);
    hr = IDirectMusicSegment_GetParam(segment, &GUID_TempoParam, -1, DMUS_SEG_ALLTRACKS, 299, &next_time, &param);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(next_time == 1, "got next_time %lu\n", next_time);
    ok(param.mtTime == -199, "got mtTime %ld\n", param.mtTime);
    ok(param.dblTempo == 80, "got dblTempo %f\n", param.dblTempo);
    hr = IDirectMusicSegment_GetParam(segment, &GUID_TempoParam, -1, DMUS_SEG_ALLTRACKS, 300, &next_time, &param);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(next_time == 3700, "got next_time %lu\n", next_time);
    ok(param.mtTime == -100, "got mtTime %ld\n", param.mtTime);
    ok(param.dblTempo == 20, "got dblTempo %f\n", param.dblTempo);
    hr = IDirectMusicSegment_GetParam(segment, &GUID_TempoParam, -1, DMUS_SEG_ALLTRACKS, 5000, &next_time, &param);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(next_time == 0, "got next_time %lu\n", next_time);
    ok(param.mtTime == -1000, "got mtTime %ld\n", param.mtTime);
    ok(param.dblTempo == 50, "got dblTempo %f\n", param.dblTempo);


    /* now play the segment, and check produced messages */

    hr = IDirectMusicPerformance_Init(performance, NULL, 0, 0);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicPerformance_PlaySegment(performance, segment, 0x800, 0, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);

    /* the tempo track only takes effect after DMUS_PMSGT_DIRTY */

    ret = test_tool_wait_message(tool, 500, &msg);
    ok(!ret, "got %#lx\n", ret);
    ok(msg->dwType == DMUS_PMSGT_DIRTY, "got %#lx\n", msg->dwType);
    hr = IDirectMusicPerformance_FreePMsg(performance, msg);
    ok(hr == S_OK, "got %#lx\n", hr);


    time = 0xdeadbeef;
    hr = IDirectMusicPerformance_MusicToReferenceTime(performance, 0, &time);
    ok(hr == S_OK, "got %#lx\n", hr);
    init_time = time;

    time = 0xdeadbeef;
    hr = IDirectMusicPerformance_MusicToReferenceTime(performance, 1, &time);
    ok(hr == S_OK, "got %#lx\n", hr);
    check_reference_time(time - init_time, scale_music_time(1, 120));
    time = 0xdeadbeef;
    hr = IDirectMusicPerformance_MusicToReferenceTime(performance, 100, &time);
    ok(hr == S_OK, "got %#lx\n", hr);
    check_reference_time(time - init_time, scale_music_time(100, 120));
    time = 0xdeadbeef;
    hr = IDirectMusicPerformance_MusicToReferenceTime(performance, 150, &time);
    ok(hr == S_OK, "got %#lx\n", hr);
    check_reference_time(time - init_time, scale_music_time(100, 120) + scale_music_time(50, 80));
    time = 0xdeadbeef;
    hr = IDirectMusicPerformance_MusicToReferenceTime(performance, 200, &time);
    ok(hr == S_OK, "got %#lx\n", hr);
    check_reference_time(time - init_time, scale_music_time(100, 120) + scale_music_time(100, 80));
    time = 0xdeadbeef;
    hr = IDirectMusicPerformance_MusicToReferenceTime(performance, 400, &time);
    ok(hr == S_OK, "got %#lx\n", hr);
    check_reference_time(time - init_time, scale_music_time(100, 120) + scale_music_time(200, 80) + scale_music_time(100, 20));

    music_time = 0xdeadbeef;
    hr = IDirectMusicPerformance_ReferenceToMusicTime(performance, init_time, &music_time);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(music_time == 0, "got %ld\n", music_time);
    music_time = 0xdeadbeef;
    time = scale_music_time(100, 120) + scale_music_time(50, 80);
    hr = IDirectMusicPerformance_ReferenceToMusicTime(performance, init_time + time, &music_time);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(music_time == 150, "got %ld\n", music_time);
    music_time = 0xdeadbeef;
    time = scale_music_time(100, 120) + scale_music_time(200, 80) + scale_music_time(100, 20);
    hr = IDirectMusicPerformance_ReferenceToMusicTime(performance, init_time + time, &music_time);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(music_time == 400, "got %ld\n", music_time);


    ret = test_tool_wait_message(tool, 2000, (DMUS_PMSG **)&tempo);
    ok(!ret, "got %#lx\n", ret);
    todo_wine ok(tempo->dwType == DMUS_PMSGT_TEMPO, "got %#lx\n", tempo->dwType);
    if (tempo->dwType != DMUS_PMSGT_TEMPO) goto skip_tests;
    check_dmus_tempo_pmsg(tempo, 100, 80);
    hr = IDirectMusicPerformance_FreePMsg(performance, (DMUS_PMSG *)tempo);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 500, (DMUS_PMSG **)&tempo);
    todo_wine ok(!ret, "got %#lx\n", ret);
    check_dmus_tempo_pmsg(tempo, 300, 60);
    hr = IDirectMusicPerformance_FreePMsg(performance, (DMUS_PMSG *)tempo);
    ok(hr == S_OK, "got %#lx\n", hr);

    ret = test_tool_wait_message(tool, 500, &msg);
    todo_wine ok(!ret, "got %#lx\n", ret);
    ok(msg->dwType == DMUS_PMSGT_DIRTY, "got %#lx\n", msg->dwType);
    hr = IDirectMusicPerformance_FreePMsg(performance, msg);
    ok(hr == S_OK, "got %#lx\n", hr);

skip_tests:
    IDirectMusicSegment_Release(segment);


    hr = IDirectMusicPerformance_CloseDown(performance);
    ok(hr == S_OK, "got %#lx\n", hr);

    IDirectMusicPerformance_Release(performance);
    IDirectMusicTool_Release(tool);
}

static void test_connect_to_collection(void)
{
    IDirectMusicCollection *collection;
    IDirectMusicSegment *segment;
    IDirectMusicTrack *track;
    void *param;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectMusicCollection, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicCollection, (void **)&collection);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = CoCreateInstance(&CLSID_DirectMusicSegment, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicSegment, (void **)&segment);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = CoCreateInstance(&CLSID_DirectMusicBandTrack, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicTrack, (void **)&track);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicSegment_InsertTrack(segment, track, 1);
    ok(hr == S_OK, "got %#lx\n", hr);

    /* it is possible to connect the band track to another collection, but not to disconnect it */
    hr = IDirectMusicTrack_IsParamSupported(track, &GUID_ConnectToDLSCollection);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicTrack_SetParam(track, &GUID_ConnectToDLSCollection, 0, NULL);
    todo_wine ok(hr == E_POINTER, "got %#lx\n", hr);
    hr = IDirectMusicTrack_SetParam(track, &GUID_ConnectToDLSCollection, 0, collection);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectMusicTrack_GetParam(track, &GUID_ConnectToDLSCollection, 0, NULL, NULL);
    todo_wine ok(hr == E_POINTER, "got %#lx\n", hr);
    hr = IDirectMusicTrack_GetParam(track, &GUID_ConnectToDLSCollection, 0, NULL, &param);
    ok(hr == DMUS_E_GET_UNSUPPORTED, "got %#lx\n", hr);

    hr = IDirectMusicSegment_SetParam(segment, &GUID_ConnectToDLSCollection, -1, DMUS_SEG_ALLTRACKS, 0, NULL);
    todo_wine ok(hr == E_POINTER, "got %#lx\n", hr);
    hr = IDirectMusicSegment_SetParam(segment, &GUID_ConnectToDLSCollection, -1, DMUS_SEG_ALLTRACKS, 0, collection);
    ok(hr == S_OK, "got %#lx\n", hr);

    IDirectMusicTrack_Release(track);
    IDirectMusicSegment_Release(segment);
    IDirectMusicCollection_Release(collection);
}

static void test_segment_state(void)
{
    static const DWORD message_types[] =
    {
        DMUS_PMSGT_DIRTY,
        DMUS_PMSGT_NOTIFICATION,
        DMUS_PMSGT_WAVE,
    };
    IDirectMusicSegmentState *state, *tmp_state;
    IDirectMusicSegment *segment, *tmp_segment;
    IDirectMusicPerformance *performance;
    IDirectMusicTrack *track;
    IDirectMusicGraph *graph;
    IDirectMusicTool *tool;
    DWORD value, ret;
    MUSIC_TIME time;
    HRESULT hr;

    hr = test_tool_create(message_types, ARRAY_SIZE(message_types), &tool);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = test_track_create(&track, TRUE);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = CoCreateInstance(&CLSID_DirectMusicPerformance, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicPerformance, (void **)&performance);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = CoCreateInstance(&CLSID_DirectMusicGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicGraph, (void **)&graph);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_SetGraph(performance, graph);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicGraph_InsertTool(graph, tool, NULL, 0, -1);
    ok(hr == S_OK, "got %#lx\n", hr);
    IDirectMusicGraph_Release(graph);


    hr = CoCreateInstance(&CLSID_DirectMusicSegment, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicSegment, (void **)&segment);
    ok(hr == S_OK, "got %#lx\n", hr);

    check_track_state(track, inserted, FALSE);
    hr = IDirectMusicSegment_InsertTrack(segment, track, 1);
    ok(hr == S_OK, "got %#lx\n", hr);
    check_track_state(track, inserted, TRUE);

    check_track_state(track, downloaded, FALSE);
    hr = IDirectMusicSegment8_Download((IDirectMusicSegment8 *)segment, (IUnknown *)performance);
    ok(hr == S_OK, "got %#lx\n", hr);
    check_track_state(track, downloaded, TRUE);
    hr = IDirectMusicSegment8_Unload((IDirectMusicSegment8 *)segment, (IUnknown *)performance);
    ok(hr == S_OK, "got %#lx\n", hr);
    check_track_state(track, downloaded, FALSE);


    /* by default the segment length is 1 */

    time = 0xdeadbeef;
    hr = IDirectMusicSegment_GetLength(segment, &time);
    ok(hr == S_OK, "got %#lx\n", hr);
    todo_wine ok(time == 1, "got %lu\n", time);
    hr = IDirectMusicSegment_SetStartPoint(segment, 50);
    ok(hr == DMUS_E_OUT_OF_RANGE, "got %#lx\n", hr);
    hr = IDirectMusicSegment_SetRepeats(segment, 10);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicSegment_SetLoopPoints(segment, 10, 70);
    ok(hr == DMUS_E_OUT_OF_RANGE, "got %#lx\n", hr);

    /* Setting a larger length will cause PlayEx to be called multiple times,
     * as native splits the segment into chunks and play each chunk separately */
    hr = IDirectMusicSegment_SetLength(segment, 100);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicSegment_SetStartPoint(segment, 50);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicSegment_SetRepeats(segment, 0);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicSegment_SetLoopPoints(segment, 0, 0);
    ok(hr == S_OK, "got %#lx\n", hr);


    /* InitPlay returns a dummy segment state */

    state = (void *)0xdeadbeef;
    hr = IDirectMusicSegment_InitPlay(segment, &state, performance, 0);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(state != NULL, "got %p\n", state);
    ok(state != (void *)0xdeadbeef, "got %p\n", state);
    check_track_state(track, initialized, FALSE);

    tmp_segment = (void *)0xdeadbeef;
    hr = IDirectMusicSegmentState_GetSegment(state, &tmp_segment);
    ok(hr == DMUS_E_NOT_FOUND, "got %#lx\n", hr);
    ok(tmp_segment == NULL, "got %p\n", tmp_segment);
    time = 0xdeadbeef;
    hr = IDirectMusicSegmentState_GetStartPoint(state, &time);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(time == 0, "got %#lx\n", time);
    time = 0xdeadbeef;
    hr = IDirectMusicSegmentState_GetRepeats(state, &value);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(time == 0xdeadbeef, "got %#lx\n", time);
    time = 0xdeadbeef;
    hr = IDirectMusicSegmentState_GetStartTime(state, &time);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(time == -1, "got %#lx\n", time);
    time = 0xdeadbeef;
    hr = IDirectMusicSegmentState_GetSeek(state, &time);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(time == 0, "got %#lx\n", time);


    /* PlaySegment returns a different, genuine segment state */

    hr = IDirectMusicPerformance_Init(performance, NULL, 0, 0);
    ok(hr == S_OK, "got %#lx\n", hr);

    check_track_state(track, downloaded, FALSE);
    check_track_state(track, initialized, FALSE);
    check_track_state(track, playing, FALSE);

    hr = IDirectMusicPerformance_GetSegmentState(performance, NULL, 0);
    ok(hr == E_POINTER, "got %#lx\n", hr);
    hr = IDirectMusicPerformance_GetSegmentState(performance, &tmp_state, 0);
    ok(hr == DMUS_E_NOT_FOUND, "got %#lx\n", hr);


    tmp_state = state;
    state = (void *)0xdeadbeef;
    hr = IDirectMusicPerformance_PlaySegment(performance, segment, 0, 20, &state);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(state != NULL, "got %p\n", state);
    ok(state != (void *)0xdeadbeef, "got %p\n", state);
    ok(state != tmp_state, "got %p\n", state);
    IDirectMusicSegmentState_Release(tmp_state);

    tmp_state = (void *)0xdeadbeef;
    hr = IDirectMusicPerformance_GetSegmentState(performance, &tmp_state, 0);
    ok(hr == S_OK || broken(hr == DMUS_E_NOT_FOUND) /* sometimes on Windows */, "got %#lx\n", hr);
    if (hr == S_OK)
    {
        ok(state == tmp_state, "got %p\n", tmp_state);
        IDirectMusicSegmentState_Release(tmp_state);
    }

    tmp_state = (void *)0xdeadbeef;
    hr = IDirectMusicPerformance_GetSegmentState(performance, &tmp_state, 69);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(state == tmp_state, "got %p\n", tmp_state);
    IDirectMusicSegmentState_Release(tmp_state);

    hr = IDirectMusicPerformance_GetSegmentState(performance, &tmp_state, 71);
    todo_wine ok(hr == DMUS_E_NOT_FOUND, "got %#lx\n", hr);


    check_track_state(track, downloaded, FALSE);
    check_track_state(track, initialized, TRUE);


    /* The track can be removed from the segment */
    hr = IDirectMusicSegment_RemoveTrack(segment, track);
    ok(hr == S_OK, "got %#lx\n", hr);


    ret = test_track_wait_playing(track, 50);
    ok(ret == 0, "got %#lx\n", ret);

    hr = IDirectMusicPerformance_GetSegmentState(performance, &tmp_state, 0);
    todo_wine ok(hr == DMUS_E_NOT_FOUND, "got %#lx\n", hr);


    tmp_segment = (void *)0xdeadbeef;
    hr = IDirectMusicSegmentState_GetSegment(state, &tmp_segment);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(tmp_segment == segment, "got %p\n", tmp_segment);
    IDirectMusicSegment_Release(tmp_segment);

    time = 0xdeadbeef;
    hr = IDirectMusicSegmentState_GetStartPoint(state, &time);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(time == 50, "got %lu\n", time);
    time = 0xdeadbeef;
    hr = IDirectMusicSegmentState_GetRepeats(state, &value);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(time == 0xdeadbeef, "got %#lx\n", time);
    time = 0xdeadbeef;
    hr = IDirectMusicSegmentState_GetStartTime(state, &time);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(time == 20, "got %#lx\n", time);
    time = 0xdeadbeef;

    /* The seek value is also dependent on whether the tracks are playing.
     * It is initially 0, then start_point right before playing, then length.
     */
    hr = IDirectMusicSegmentState_GetSeek(state, &time);
    ok(hr == S_OK, "got %#lx\n", hr);
    todo_wine ok(time == 100, "got %#lx\n", time);

    /* changing the segment values doesn't change the segment state */

    hr = IDirectMusicSegment_SetStartPoint(segment, 0);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicSegment_SetRepeats(segment, 10);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IDirectMusicSegment_SetLoopPoints(segment, 50, 70);
    ok(hr == S_OK, "got %#lx\n", hr);

    time = 0xdeadbeef;
    hr = IDirectMusicSegmentState_GetStartPoint(state, &time);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(time == 50, "got %#lx\n", time);
    time = 0xdeadbeef;
    hr = IDirectMusicSegmentState_GetRepeats(state, &value);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(time == 0xdeadbeef, "got %#lx\n", time);
    time = 0xdeadbeef;
    hr = IDirectMusicSegmentState_GetStartTime(state, &time);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(time == 20, "got %#lx\n", time);
    time = 0xdeadbeef;
    hr = IDirectMusicSegmentState_GetSeek(state, &time);
    ok(hr == S_OK, "got %#lx\n", hr);
    todo_wine ok(time == 100, "got %#lx\n", time);

    IDirectMusicSegment_Release(segment);


    check_track_state(track, downloaded, FALSE);
    check_track_state(track, initialized, TRUE);
    check_track_state(track, playing, TRUE);

    hr = IDirectMusicPerformance_CloseDown(performance);
    ok(hr == S_OK, "got %#lx\n", hr);

    check_track_state(track, downloaded, FALSE);
    check_track_state(track, initialized, TRUE);
    check_track_state(track, playing, FALSE);


    IDirectMusicPerformance_Release(performance);
    IDirectMusicTrack_Release(track);
    IDirectMusicTool_Release(tool);
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
    test_COM_performance();
    test_audiopathconfig();
    test_graph();
    test_segment();
    test_gettrack();
    test_segment_param();
    test_track();
    test_parsedescriptor();
    test_performance_InitAudio();
    test_performance_createport();
    test_performance_pchannel();
    test_performance_tool();
    test_performance_graph();
    test_performance_time();
    test_performance_pmsg();
    test_notification_pmsg();
    test_wave_pmsg();
    test_sequence_track();
    test_band_track_play();
    test_tempo_track_play();
    test_connect_to_collection();
    test_segment_state();

    CoUninitialize();
}
