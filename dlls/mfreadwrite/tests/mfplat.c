/*
 * Copyright 2018 Alistair Leslie-Hughes
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

#include <stdarg.h>
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"

#include "initguid.h"
#include "ole2.h"

DEFINE_GUID(GUID_NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

#undef INITGUID
#include "guiddef.h"
#include "mfapi.h"
#include "mfidl.h"
#include "mferror.h"
#include "mfreadwrite.h"
#include "d3d9.h"
#include "dxva2api.h"

#include "wine/test.h"

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

static HWND create_window(void)
{
    RECT r = {0, 0, 640, 480};

    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW | WS_VISIBLE, FALSE);

    return CreateWindowA("static", "mfreadwrite_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, r.right - r.left, r.bottom - r.top, NULL, NULL, NULL, NULL);
}

static IDirect3DDevice9 *create_d3d9_device(IDirect3D9 *d3d9, HWND focus_window)
{
    D3DPRESENT_PARAMETERS present_parameters = {0};
    IDirect3DDevice9 *device = NULL;

    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.BackBufferFormat = D3DFMT_X8R8G8B8;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.hDeviceWindow = focus_window;
    present_parameters.Windowed = TRUE;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;
    present_parameters.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

    IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, focus_window,
            D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, &device);

    return device;
}

static HRESULT (WINAPI *pMFCreateMFByteStreamOnStream)(IStream *stream, IMFByteStream **bytestream);

static void init_functions(void)
{
    HMODULE mod = GetModuleHandleA("mfplat.dll");

#define X(f) if (!(p##f = (void*)GetProcAddress(mod, #f))) return;
    X(MFCreateMFByteStreamOnStream);
#undef X
}

enum source_state
{
    SOURCE_STOPPED = 0,
    SOURCE_RUNNING,
};

struct test_media_stream
{
    IMFMediaStream IMFMediaStream_iface;
    LONG refcount;
    IMFMediaSource *source;
    IMFStreamDescriptor *sd;
    IMFMediaEventQueue *event_queue;
    BOOL is_new;
};

static struct test_media_stream *impl_from_IMFMediaStream(IMFMediaStream *iface)
{
    return CONTAINING_RECORD(iface, struct test_media_stream, IMFMediaStream_iface);
}

static HRESULT WINAPI test_media_stream_QueryInterface(IMFMediaStream *iface, REFIID riid, void **out)
{
    if (IsEqualIID(riid, &IID_IMFMediaStream)
            || IsEqualIID(riid, &IID_IMFMediaEventGenerator)
            || IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
    }
    else
    {
        *out = NULL;
        return E_NOINTERFACE;
    }

    IMFMediaStream_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI test_media_stream_AddRef(IMFMediaStream *iface)
{
    struct test_media_stream *stream = impl_from_IMFMediaStream(iface);
    return InterlockedIncrement(&stream->refcount);
}

static ULONG WINAPI test_media_stream_Release(IMFMediaStream *iface)
{
    struct test_media_stream *stream = impl_from_IMFMediaStream(iface);
    ULONG refcount = InterlockedDecrement(&stream->refcount);

    if (!refcount)
    {
        IMFMediaEventQueue_Release(stream->event_queue);
        free(stream);
    }

    return refcount;
}

static HRESULT WINAPI test_media_stream_GetEvent(IMFMediaStream *iface, DWORD flags, IMFMediaEvent **event)
{
    struct test_media_stream *stream = impl_from_IMFMediaStream(iface);
    return IMFMediaEventQueue_GetEvent(stream->event_queue, flags, event);
}

static HRESULT WINAPI test_media_stream_BeginGetEvent(IMFMediaStream *iface, IMFAsyncCallback *callback, IUnknown *state)
{
    struct test_media_stream *stream = impl_from_IMFMediaStream(iface);
    ok(callback != NULL && state == (IUnknown *)iface, "Unexpected arguments.\n");
    return IMFMediaEventQueue_BeginGetEvent(stream->event_queue, callback, state);
}

static HRESULT WINAPI test_media_stream_EndGetEvent(IMFMediaStream *iface, IMFAsyncResult *result, IMFMediaEvent **event)
{
    struct test_media_stream *stream = impl_from_IMFMediaStream(iface);
    ok(!!result && !!event, "Unexpected arguments.\n");
    return IMFMediaEventQueue_EndGetEvent(stream->event_queue, result, event);
}

static HRESULT WINAPI test_media_stream_QueueEvent(IMFMediaStream *iface, MediaEventType event_type, REFGUID ext_type,
        HRESULT hr, const PROPVARIANT *value)
{
    struct test_media_stream *stream = impl_from_IMFMediaStream(iface);
    return IMFMediaEventQueue_QueueEventParamVar(stream->event_queue, event_type, ext_type, hr, value);
}

static HRESULT WINAPI test_media_stream_GetMediaSource(IMFMediaStream *iface, IMFMediaSource **source)
{
    struct test_media_stream *stream = impl_from_IMFMediaStream(iface);

    *source = stream->source;
    IMFMediaSource_AddRef(*source);

    return S_OK;
}

static HRESULT WINAPI test_media_stream_GetStreamDescriptor(IMFMediaStream *iface, IMFStreamDescriptor **sd)
{
    struct test_media_stream *stream = impl_from_IMFMediaStream(iface);

    *sd = stream->sd;
    IMFStreamDescriptor_AddRef(*sd);

    return S_OK;
}

static BOOL fail_request_sample;

static HRESULT WINAPI test_media_stream_RequestSample(IMFMediaStream *iface, IUnknown *token)
{
    struct test_media_stream *stream = impl_from_IMFMediaStream(iface);
    IMFMediaBuffer *buffer;
    IMFSample *sample;
    HRESULT hr;

    if (fail_request_sample)
        return E_NOTIMPL;

    hr = MFCreateSample(&sample);
    ok(hr == S_OK, "Failed to create a sample, hr %#x.\n", hr);
    hr = IMFSample_SetSampleTime(sample, 123);
    ok(hr == S_OK, "Failed to set sample time, hr %#x.\n", hr);
    if (token)
        IMFSample_SetUnknown(sample, &MFSampleExtension_Token, token);

    /* Reader expects buffers, empty samples are considered an error. */
    hr = MFCreateMemoryBuffer(8, &buffer);
    ok(hr == S_OK, "Failed to create a buffer, hr %#x.\n", hr);
    hr = IMFSample_AddBuffer(sample, buffer);
    ok(hr == S_OK, "Failed to add a buffer, hr %#x.\n", hr);
    IMFMediaBuffer_Release(buffer);

    hr = IMFMediaEventQueue_QueueEventParamUnk(stream->event_queue, MEMediaSample, &GUID_NULL, S_OK,
            (IUnknown *)sample);
    ok(hr == S_OK, "Failed to submit event, hr %#x.\n", hr);
    IMFSample_Release(sample);

    return S_OK;
}

static const IMFMediaStreamVtbl test_media_stream_vtbl =
{
    test_media_stream_QueryInterface,
    test_media_stream_AddRef,
    test_media_stream_Release,
    test_media_stream_GetEvent,
    test_media_stream_BeginGetEvent,
    test_media_stream_EndGetEvent,
    test_media_stream_QueueEvent,
    test_media_stream_GetMediaSource,
    test_media_stream_GetStreamDescriptor,
    test_media_stream_RequestSample,
};

#define TEST_SOURCE_NUM_STREAMS 3

struct test_source
{
    IMFMediaSource IMFMediaSource_iface;
    LONG refcount;
    IMFMediaEventQueue *event_queue;
    IMFPresentationDescriptor *pd;
    struct test_media_stream *streams[TEST_SOURCE_NUM_STREAMS];
    unsigned stream_count;
    enum source_state state;
    CRITICAL_SECTION cs;
};

static struct test_source *impl_from_IMFMediaSource(IMFMediaSource *iface)
{
    return CONTAINING_RECORD(iface, struct test_source, IMFMediaSource_iface);
}

static HRESULT WINAPI test_source_QueryInterface(IMFMediaSource *iface, REFIID riid, void **out)
{
    if (IsEqualIID(riid, &IID_IMFMediaSource)
            || IsEqualIID(riid, &IID_IMFMediaEventGenerator)
            || IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
    }
    else
    {
        *out = NULL;
        return E_NOINTERFACE;
    }

    IMFMediaSource_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI test_source_AddRef(IMFMediaSource *iface)
{
    struct test_source *source = impl_from_IMFMediaSource(iface);
    return InterlockedIncrement(&source->refcount);
}

static ULONG WINAPI test_source_Release(IMFMediaSource *iface)
{
    struct test_source *source = impl_from_IMFMediaSource(iface);
    ULONG refcount = InterlockedDecrement(&source->refcount);

    if (!refcount)
    {
        IMFMediaEventQueue_Release(source->event_queue);
        free(source);
    }

    return refcount;
}

static HRESULT WINAPI test_source_GetEvent(IMFMediaSource *iface, DWORD flags, IMFMediaEvent **event)
{
    struct test_source *source = impl_from_IMFMediaSource(iface);
    return IMFMediaEventQueue_GetEvent(source->event_queue, flags, event);
}

static HRESULT WINAPI test_source_BeginGetEvent(IMFMediaSource *iface, IMFAsyncCallback *callback, IUnknown *state)
{
    struct test_source *source = impl_from_IMFMediaSource(iface);
    ok(callback != NULL && state == (IUnknown *)iface, "Unexpected arguments source %p, %p, state %p.\n", iface, callback, state);
    return IMFMediaEventQueue_BeginGetEvent(source->event_queue, callback, state);
}

static HRESULT WINAPI test_source_EndGetEvent(IMFMediaSource *iface, IMFAsyncResult *result, IMFMediaEvent **event)
{
    struct test_source *source = impl_from_IMFMediaSource(iface);
    return IMFMediaEventQueue_EndGetEvent(source->event_queue, result, event);
}

static HRESULT WINAPI test_source_QueueEvent(IMFMediaSource *iface, MediaEventType event_type, REFGUID ext_type,
        HRESULT hr, const PROPVARIANT *value)
{
    struct test_source *source = impl_from_IMFMediaSource(iface);
    return IMFMediaEventQueue_QueueEventParamVar(source->event_queue, event_type, ext_type, hr, value);
}

static HRESULT WINAPI test_source_GetCharacteristics(IMFMediaSource *iface, DWORD *flags)
{
    *flags = MFMEDIASOURCE_CAN_SEEK;

    return S_OK;
}

static HRESULT WINAPI test_source_CreatePresentationDescriptor(IMFMediaSource *iface, IMFPresentationDescriptor **pd)
{
    struct test_source *source = impl_from_IMFMediaSource(iface);
    IMFStreamDescriptor *sds[ARRAY_SIZE(source->streams)];
    IMFMediaType *media_type;
    HRESULT hr = S_OK;
    int i;

    EnterCriticalSection(&source->cs);

    if (source->pd)
    {
        *pd = source->pd;
        IMFPresentationDescriptor_AddRef(*pd);
    }
    else
    {
        for (i = 0; i < source->stream_count; ++i)
        {
            hr = MFCreateMediaType(&media_type);
            ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

            hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
            ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);
            hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
            ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);
            hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_BITS_PER_SAMPLE, 32);
            ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

            hr = MFCreateStreamDescriptor(i, 1, &media_type, &sds[i]);
            ok(hr == S_OK, "Failed to create stream descriptor, hr %#x.\n", hr);

            IMFMediaType_Release(media_type);
        }

        hr = MFCreatePresentationDescriptor(source->stream_count, sds, &source->pd);
        ok(hr == S_OK, "Failed to create presentation descriptor, hr %#x.\n", hr);
        for (i = 0; i < source->stream_count; ++i)
            IMFStreamDescriptor_Release(sds[i]);

        *pd = source->pd;
        IMFPresentationDescriptor_AddRef(*pd);
    }

    LeaveCriticalSection(&source->cs);

    return hr;
}

static BOOL is_stream_selected(IMFPresentationDescriptor *pd, DWORD index)
{
    IMFStreamDescriptor *sd;
    BOOL selected = FALSE;

    if (SUCCEEDED(IMFPresentationDescriptor_GetStreamDescriptorByIndex(pd, index, &selected, &sd)))
        IMFStreamDescriptor_Release(sd);

    return selected;
}

static HRESULT WINAPI test_source_Start(IMFMediaSource *iface, IMFPresentationDescriptor *pd, const GUID *time_format,
        const PROPVARIANT *start_position)
{
    struct test_source *source = impl_from_IMFMediaSource(iface);
    MediaEventType event_type;
    PROPVARIANT var;
    HRESULT hr;
    int i;

    ok(time_format && IsEqualGUID(time_format, &GUID_NULL), "Unexpected time format %s.\n",
            wine_dbgstr_guid(time_format));
    ok(start_position && (start_position->vt == VT_I8 || start_position->vt == VT_EMPTY),
            "Unexpected position type.\n");

    EnterCriticalSection(&source->cs);

    event_type = source->state == SOURCE_RUNNING ? MESourceSeeked : MESourceStarted;
    hr = IMFMediaEventQueue_QueueEventParamVar(source->event_queue, event_type, &GUID_NULL, S_OK, NULL);
    ok(hr == S_OK, "Failed to queue event, hr %#x.\n", hr);

    for (i = 0; i < source->stream_count; ++i)
    {
        if (!is_stream_selected(pd, i))
            continue;

        var.vt = VT_UNKNOWN;
        var.punkVal = (IUnknown *)&source->streams[i]->IMFMediaStream_iface;
        event_type = source->streams[i]->is_new ? MENewStream : MEUpdatedStream;
        source->streams[i]->is_new = FALSE;
        hr = IMFMediaEventQueue_QueueEventParamVar(source->event_queue, event_type, &GUID_NULL, S_OK, &var);
        ok(hr == S_OK, "Failed to queue event, hr %#x.\n", hr);

        event_type = source->state == SOURCE_RUNNING ? MEStreamSeeked : MEStreamStarted;
        hr = IMFMediaEventQueue_QueueEventParamVar(source->streams[i]->event_queue, event_type, &GUID_NULL,
                S_OK, NULL);
        ok(hr == S_OK, "Failed to queue event, hr %#x.\n", hr);
    }

    source->state = SOURCE_RUNNING;

    LeaveCriticalSection(&source->cs);

    return S_OK;
}

static HRESULT WINAPI test_source_Stop(IMFMediaSource *iface)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_source_Pause(IMFMediaSource *iface)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_source_Shutdown(IMFMediaSource *iface)
{
    struct test_source *source = impl_from_IMFMediaSource(iface);
    HRESULT hr;

    hr = IMFMediaEventQueue_Shutdown(source->event_queue);
    ok(hr == S_OK, "Failed to shut down event queue, hr %#x.\n", hr);

    return S_OK;
}

static const IMFMediaSourceVtbl test_source_vtbl =
{
    test_source_QueryInterface,
    test_source_AddRef,
    test_source_Release,
    test_source_GetEvent,
    test_source_BeginGetEvent,
    test_source_EndGetEvent,
    test_source_QueueEvent,
    test_source_GetCharacteristics,
    test_source_CreatePresentationDescriptor,
    test_source_Start,
    test_source_Stop,
    test_source_Pause,
    test_source_Shutdown,
};

static struct test_media_stream *create_test_stream(DWORD stream_index, IMFMediaSource *source)
{
    struct test_media_stream *stream;
    IMFPresentationDescriptor *pd;
    BOOL selected;
    HRESULT hr;

    stream = calloc(1, sizeof(*stream));
    stream->IMFMediaStream_iface.lpVtbl = &test_media_stream_vtbl;
    stream->refcount = 1;
    hr = MFCreateEventQueue(&stream->event_queue);
    ok(hr == S_OK, "Failed to create event queue, hr %#x.\n", hr);
    stream->source = source;
    IMFMediaSource_AddRef(stream->source);
    stream->is_new = TRUE;

    IMFMediaSource_CreatePresentationDescriptor(source, &pd);
    IMFPresentationDescriptor_GetStreamDescriptorByIndex(pd, stream_index, &selected, &stream->sd);
    IMFPresentationDescriptor_Release(pd);

    return stream;
}

static IMFMediaSource *create_test_source(int stream_count)
{
    struct test_source *source;
    int i;

    source = calloc(1, sizeof(*source));
    source->IMFMediaSource_iface.lpVtbl = &test_source_vtbl;
    source->refcount = 1;
    source->stream_count = stream_count;
    MFCreateEventQueue(&source->event_queue);
    InitializeCriticalSection(&source->cs);
    for (i = 0; i < source->stream_count; ++i)
        source->streams[i] = create_test_stream(i, &source->IMFMediaSource_iface);

    return &source->IMFMediaSource_iface;
}

static IMFByteStream *get_resource_stream(const char *name)
{
    IMFByteStream *bytestream;
    IStream *stream;
    DWORD written;
    HRESULT hr;
    HRSRC res;
    void *ptr;

    res = FindResourceA(NULL, name, (const char *)RT_RCDATA);
    ok(res != 0, "Resource %s wasn't found.\n", name);

    ptr = LockResource(LoadResource(GetModuleHandleA(NULL), res));

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Failed to create memory stream, hr %#x.\n", hr);

    hr = IStream_Write(stream, ptr, SizeofResource(GetModuleHandleA(NULL), res), &written);
    ok(hr == S_OK, "Failed to write stream content, hr %#x.\n", hr);

    hr = pMFCreateMFByteStreamOnStream(stream, &bytestream);
    ok(hr == S_OK, "Failed to create bytestream, hr %#x.\n", hr);
    IStream_Release(stream);

    return bytestream;
}

static void test_factory(void)
{
    IMFReadWriteClassFactory *factory, *factory2;
    HRESULT hr;

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_MFReadWriteClassFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IMFReadWriteClassFactory,
            (void **)&factory);
    ok(hr == S_OK, "Failed to create class factory, hr %#x.\n", hr);

    hr = CoCreateInstance(&CLSID_MFReadWriteClassFactory, (IUnknown *)factory, CLSCTX_INPROC_SERVER, &IID_IMFReadWriteClassFactory,
            (void **)&factory2);
    ok(hr == CLASS_E_NOAGGREGATION, "Unexpected hr %#x.\n", hr);

    IMFReadWriteClassFactory_Release(factory);

    CoUninitialize();
}

struct async_callback
{
    IMFSourceReaderCallback IMFSourceReaderCallback_iface;
    LONG refcount;
    HANDLE event;
};

static struct async_callback *impl_from_IMFSourceReaderCallback(IMFSourceReaderCallback *iface)
{
    return CONTAINING_RECORD(iface, struct async_callback, IMFSourceReaderCallback_iface);
}

static HRESULT WINAPI async_callback_QueryInterface(IMFSourceReaderCallback *iface, REFIID riid, void **out)
{
    if (IsEqualIID(riid, &IID_IMFSourceReaderCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
        IMFSourceReaderCallback_AddRef(iface);
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI async_callback_AddRef(IMFSourceReaderCallback *iface)
{
    struct async_callback *callback = impl_from_IMFSourceReaderCallback(iface);
    return InterlockedIncrement(&callback->refcount);
}

static ULONG WINAPI async_callback_Release(IMFSourceReaderCallback *iface)
{
    struct async_callback *callback = impl_from_IMFSourceReaderCallback(iface);
    ULONG refcount = InterlockedDecrement(&callback->refcount);

    if (!refcount)
        free(callback);

    return refcount;
}

static HRESULT WINAPI async_callback_OnReadSample(IMFSourceReaderCallback *iface, HRESULT hr, DWORD stream_index,
        DWORD stream_flags, LONGLONG timestamp, IMFSample *sample)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI async_callback_OnFlush(IMFSourceReaderCallback *iface, DWORD stream_index)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI async_callback_OnEvent(IMFSourceReaderCallback *iface, DWORD stream_index, IMFMediaEvent *event)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IMFSourceReaderCallbackVtbl async_callback_vtbl =
{
    async_callback_QueryInterface,
    async_callback_AddRef,
    async_callback_Release,
    async_callback_OnReadSample,
    async_callback_OnFlush,
    async_callback_OnEvent,
};

static struct async_callback *create_async_callback(void)
{
    struct async_callback *callback;

    callback = calloc(1, sizeof(*callback));
    callback->IMFSourceReaderCallback_iface.lpVtbl = &async_callback_vtbl;
    callback->refcount = 1;

    return callback;
}

static void test_source_reader(void)
{
    IMFMediaType *mediatype, *mediatype2;
    DWORD stream_flags, actual_index;
    struct async_callback *callback;
    IMFAttributes *attributes;
    IMFSourceReader *reader;
    IMFMediaSource *source;
    IMFByteStream *stream;
    LONGLONG timestamp;
    IMFSample *sample;
    ULONG refcount;
    BOOL selected;
    HRESULT hr;

    if (!pMFCreateMFByteStreamOnStream)
    {
        win_skip("MFCreateMFByteStreamOnStream() not found\n");
        return;
    }

    stream = get_resource_stream("test.wav");

    hr = MFCreateSourceReaderFromByteStream(stream, NULL, &reader);
    if (FAILED(hr))
    {
        skip("MFCreateSourceReaderFromByteStream() failed, is G-Streamer missing?\n");
        IMFByteStream_Release(stream);
        return;
    }
    ok(hr == S_OK, "Failed to create source reader, hr %#x.\n", hr);

    /* Access underlying media source object. */
    hr = IMFSourceReader_GetServiceForStream(reader, MF_SOURCE_READER_MEDIASOURCE, &GUID_NULL, &IID_IMFMediaSource,
            (void **)&source);
    ok(hr == S_OK, "Failed to get media source interface, hr %#x.\n", hr);
    IMFMediaSource_Release(source);

    /* Stream selection. */
    hr = IMFSourceReader_GetStreamSelection(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, &selected);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_GetStreamSelection(reader, 100, &selected);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    selected = FALSE;
    hr = IMFSourceReader_GetStreamSelection(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, &selected);
    ok(hr == S_OK, "Failed to get stream selection, hr %#x.\n", hr);
    ok(selected, "Unexpected selection.\n");

    selected = FALSE;
    hr = IMFSourceReader_GetStreamSelection(reader, 0, &selected);
    ok(hr == S_OK, "Failed to get stream selection, hr %#x.\n", hr);
    ok(selected, "Unexpected selection.\n");

    hr = IMFSourceReader_SetStreamSelection(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_SetStreamSelection(reader, 100, TRUE);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_SetStreamSelection(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, FALSE);
    ok(hr == S_OK, "Failed to deselect a stream, hr %#x.\n", hr);

    selected = TRUE;
    hr = IMFSourceReader_GetStreamSelection(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, &selected);
    ok(hr == S_OK, "Failed to get stream selection, hr %#x.\n", hr);
    ok(!selected, "Unexpected selection.\n");

    hr = IMFSourceReader_SetStreamSelection(reader, MF_SOURCE_READER_ALL_STREAMS, TRUE);
    ok(hr == S_OK, "Failed to deselect a stream, hr %#x.\n", hr);

    selected = FALSE;
    hr = IMFSourceReader_GetStreamSelection(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, &selected);
    ok(hr == S_OK, "Failed to get stream selection, hr %#x.\n", hr);
    ok(selected, "Unexpected selection.\n");

    /* Native media type. */
    hr = IMFSourceReader_GetNativeMediaType(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &mediatype);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_GetNativeMediaType(reader, 100, 0, &mediatype);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_GetNativeMediaType(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &mediatype);
    ok(hr == S_OK, "Failed to get native mediatype, hr %#x.\n", hr);
    hr = IMFSourceReader_GetNativeMediaType(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &mediatype2);
    ok(hr == S_OK, "Failed to get native mediatype, hr %#x.\n", hr);
    ok(mediatype != mediatype2, "Unexpected media type instance.\n");
    IMFMediaType_Release(mediatype2);
    IMFMediaType_Release(mediatype);

    /* MF_SOURCE_READER_CURRENT_TYPE_INDEX is Win8+ */
    hr = IMFSourceReader_GetNativeMediaType(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM,
            MF_SOURCE_READER_CURRENT_TYPE_INDEX, &mediatype);
    ok(hr == S_OK || broken(hr == MF_E_NO_MORE_TYPES), "Failed to get native mediatype, hr %#x.\n", hr);
    if (SUCCEEDED(hr))
        IMFMediaType_Release(mediatype);

    /* Current media type. */
    hr = IMFSourceReader_GetCurrentMediaType(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, &mediatype);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_GetCurrentMediaType(reader, 100, &mediatype);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_GetCurrentMediaType(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, &mediatype);
    ok(hr == S_OK, "Failed to get current media type, hr %#x.\n", hr);
    IMFMediaType_Release(mediatype);

    hr = IMFSourceReader_GetCurrentMediaType(reader, 0, &mediatype);
    ok(hr == S_OK, "Failed to get current media type, hr %#x.\n", hr);
    IMFMediaType_Release(mediatype);

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &actual_index, &stream_flags,
            &timestamp, &sample);
    ok(hr == S_OK, "Failed to get a sample, hr %#x.\n", hr);
    if (hr != S_OK)
        goto skip_read_sample;
    ok(actual_index == 0, "Unexpected stream index %u\n", actual_index);
    ok(!stream_flags, "Unexpected stream flags %#x.\n", stream_flags);
    IMFSample_Release(sample);

    /* There is no video stream. */
    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &actual_index, &stream_flags,
            &timestamp, &sample);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);
    ok(actual_index == MF_SOURCE_READER_FIRST_VIDEO_STREAM, "Unexpected stream index %u\n", actual_index);
    ok(stream_flags == MF_SOURCE_READERF_ERROR, "Unexpected stream flags %#x.\n", stream_flags);

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, &stream_flags, &timestamp,
            &sample);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, NULL, &timestamp, &sample);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &actual_index, &stream_flags,
            &timestamp, &sample);
    ok(hr == S_OK, "Failed to get a sample, hr %#x.\n", hr);
    ok(actual_index == 0, "Unexpected stream index %u\n", actual_index);
    /* TODO: gstreamer outputs .wav sample in increments of 4096, instead of 4410 */
todo_wine
{
    ok(stream_flags == MF_SOURCE_READERF_ENDOFSTREAM, "Unexpected stream flags %#x.\n", stream_flags);
    ok(!sample, "Unexpected sample object.\n");
}
    if(!stream_flags)
    {
        IMFSample_Release(sample);

        hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &actual_index, &stream_flags,
                &timestamp, &sample);
        ok(hr == S_OK, "Failed to get a sample, hr %#x.\n", hr);
        ok(actual_index == 0, "Unexpected stream index %u\n", actual_index);
        ok(stream_flags == MF_SOURCE_READERF_ENDOFSTREAM, "Unexpected stream flags %#x.\n", stream_flags);
        ok(!sample, "Unexpected sample object.\n");
    }

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, MF_SOURCE_READER_CONTROLF_DRAIN,
            &actual_index, &stream_flags, &timestamp, &sample);
    ok(hr == S_OK, "Failed to get a sample, hr %#x.\n", hr);
    ok(actual_index == 0, "Unexpected stream index %u\n", actual_index);
    ok(stream_flags == MF_SOURCE_READERF_ENDOFSTREAM, "Unexpected stream flags %#x.\n", stream_flags);
    ok(!sample, "Unexpected sample object.\n");

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, MF_SOURCE_READER_CONTROLF_DRAIN,
            &actual_index, &stream_flags, &timestamp, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, MF_SOURCE_READER_CONTROLF_DRAIN,
            &actual_index, NULL, &timestamp, &sample);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, MF_SOURCE_READER_CONTROLF_DRAIN,
            NULL, &stream_flags, &timestamp, &sample);
    ok(hr == S_OK, "Failed to get a sample, hr %#x.\n", hr);
    ok(stream_flags == MF_SOURCE_READERF_ENDOFSTREAM, "Unexpected stream flags %#x.\n", stream_flags);
    ok(!sample, "Unexpected sample object.\n");

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, MF_SOURCE_READER_CONTROLF_DRAIN,
            &actual_index, &stream_flags, NULL, &sample);
    ok(hr == S_OK, "Failed to get a sample, hr %#x.\n", hr);
    ok(actual_index == 0, "Unexpected stream index %u\n", actual_index);
    ok(stream_flags == MF_SOURCE_READERF_ENDOFSTREAM, "Unexpected stream flags %#x.\n", stream_flags);
    ok(!sample, "Unexpected sample object.\n");

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_ANY_STREAM, MF_SOURCE_READER_CONTROLF_DRAIN,
            &actual_index, &stream_flags, NULL, &sample);
    ok(hr == S_OK, "Failed to get a sample, hr %#x.\n", hr);
    ok(actual_index == 0, "Unexpected stream index %u\n", actual_index);
    ok(stream_flags == MF_SOURCE_READERF_ENDOFSTREAM, "Unexpected stream flags %#x.\n", stream_flags);
    ok(!sample, "Unexpected sample object.\n");

skip_read_sample:

    /* Flush. */
    hr = IMFSourceReader_Flush(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_Flush(reader, 100);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_Flush(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM);
    ok(hr == S_OK, "Failed to flush stream, hr %#x.\n", hr);

    hr = IMFSourceReader_Flush(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM);
    ok(hr == S_OK, "Failed to flush stream, hr %#x.\n", hr);

    hr = IMFSourceReader_Flush(reader, MF_SOURCE_READER_ALL_STREAMS);
    ok(hr == S_OK, "Failed to flush all streams, hr %#x.\n", hr);

    refcount = IMFSourceReader_Release(reader);
    ok(!refcount, "Unexpected refcount %u.\n", refcount);

    /* Async mode. */
    callback = create_async_callback();

    hr = MFCreateAttributes(&attributes, 1);
    ok(hr == S_OK, "Failed to create attributes object, hr %#x.\n", hr);

    hr = IMFAttributes_SetUnknown(attributes, &MF_SOURCE_READER_ASYNC_CALLBACK,
            (IUnknown *)&callback->IMFSourceReaderCallback_iface);
    ok(hr == S_OK, "Failed to set attribute value, hr %#x.\n", hr);
    IMFSourceReaderCallback_Release(&callback->IMFSourceReaderCallback_iface);

    refcount = get_refcount(attributes);
    hr = MFCreateSourceReaderFromByteStream(stream, attributes, &reader);
    ok(hr == S_OK, "Failed to create source reader, hr %#x.\n", hr);
    ok(get_refcount(attributes) > refcount, "Unexpected refcount.\n");
    IMFAttributes_Release(attributes);
    IMFSourceReader_Release(reader);

    IMFByteStream_Release(stream);
}

static void test_source_reader_from_media_source(void)
{
    struct async_callback *callback;
    IMFSourceReader *reader;
    IMFMediaSource *source;
    IMFMediaType *media_type;
    HRESULT hr;
    DWORD actual_index, stream_flags;
    IMFSample *sample;
    LONGLONG timestamp;
    IMFAttributes *attributes;
    ULONG refcount;
    int i;
    PROPVARIANT pos;

    source = create_test_source(3);
    ok(!!source, "Failed to create test source.\n");

    hr = MFCreateSourceReaderFromMediaSource(source, NULL, &reader);
    ok(hr == S_OK, "Failed to create source reader, hr %#x.\n", hr);

    /* MF_SOURCE_READER_ANY_STREAM */
    hr = IMFSourceReader_SetStreamSelection(reader, 0, FALSE);
    ok(hr == S_OK, "Failed to select a stream, hr %#x.\n", hr);

    hr = IMFSourceReader_SetStreamSelection(reader, 1, TRUE);
    ok(hr == S_OK, "Failed to select a stream, hr %#x.\n", hr);

    pos.vt = VT_I8;
    pos.hVal.QuadPart = 0;
    hr = IMFSourceReader_SetCurrentPosition(reader, &GUID_NULL, &pos);
    ok(hr == S_OK, "Failed to seek to beginning of stream, hr %#x.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_ANY_STREAM, 0, &actual_index, &stream_flags,
            &timestamp, &sample);
    ok(hr == S_OK, "Failed to get a sample, hr %#x.\n", hr);
    ok(actual_index == 1, "Unexpected stream index %u\n", actual_index);
    ok(!stream_flags, "Unexpected stream flags %#x.\n", stream_flags);
    ok(timestamp == 123, "Unexpected timestamp.\n");
    ok(!!sample, "Expected sample object.\n");
    IMFSample_Release(sample);

    hr = IMFSourceReader_SetStreamSelection(reader, 0, TRUE);
    ok(hr == S_OK, "Failed to select a stream, hr %#x.\n", hr);

    hr = IMFSourceReader_SetStreamSelection(reader, 2, TRUE);
    ok(hr == S_OK, "Failed to select a stream, hr %#x.\n", hr);

    for (i = 0; i < TEST_SOURCE_NUM_STREAMS + 1; ++i)
    {
        hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_ANY_STREAM, 0, &actual_index, &stream_flags,
                &timestamp, &sample);
        ok(hr == S_OK, "Failed to get a sample, hr %#x.\n", hr);
        ok(actual_index == i % TEST_SOURCE_NUM_STREAMS, "%d: Unexpected stream index %u\n", i, actual_index);
        ok(!stream_flags, "Unexpected stream flags %#x.\n", stream_flags);
        ok(timestamp == 123, "Unexpected timestamp.\n");
        ok(!!sample, "Expected sample object.\n");
        IMFSample_Release(sample);
    }

    hr = IMFSourceReader_SetStreamSelection(reader, 0, FALSE);
    ok(hr == S_OK, "Failed to select a stream, hr %#x.\n", hr);

    hr = IMFSourceReader_SetStreamSelection(reader, 0, TRUE);
    ok(hr == S_OK, "Failed to select a stream, hr %#x.\n", hr);

    for (i = 0; i < TEST_SOURCE_NUM_STREAMS + 1; ++i)
    {
        hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_ANY_STREAM, 0, &actual_index, &stream_flags,
                &timestamp, &sample);
        ok(hr == S_OK, "Failed to get a sample, hr %#x.\n", hr);
        ok(actual_index == i % TEST_SOURCE_NUM_STREAMS, "%d: Unexpected stream index %u\n", i, actual_index);
        ok(!stream_flags, "Unexpected stream flags %#x.\n", stream_flags);
        ok(timestamp == 123, "Unexpected timestamp.\n");
        ok(!!sample, "Expected sample object.\n");
        IMFSample_Release(sample);
    }

    IMFSourceReader_Release(reader);
    IMFMediaSource_Release(source);

    source = create_test_source(1);
    ok(!!source, "Failed to create test source.\n");

    hr = MFCreateSourceReaderFromMediaSource(source, NULL, &reader);
    ok(hr == S_OK, "Failed to create source reader, hr %#x.\n", hr);

    /* MF_SOURCE_READER_ANY_STREAM with a single stream */
    hr = IMFSourceReader_SetStreamSelection(reader, 0, TRUE);
    ok(hr == S_OK, "Failed to select a stream, hr %#x.\n", hr);

    pos.vt = VT_I8;
    pos.hVal.QuadPart = 0;
    hr = IMFSourceReader_SetCurrentPosition(reader, &GUID_NULL, &pos);
    ok(hr == S_OK, "Failed to seek to beginning of stream, hr %#x.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_ANY_STREAM, 0, &actual_index, &stream_flags,
            &timestamp, &sample);
    ok(hr == S_OK, "Failed to get a sample, hr %#x.\n", hr);
    ok(actual_index == 0, "Unexpected stream index %u\n", actual_index);
    ok(!stream_flags, "Unexpected stream flags %#x.\n", stream_flags);
    ok(timestamp == 123, "Unexpected timestamp.\n");
    ok(!!sample, "Expected sample object.\n");
    IMFSample_Release(sample);

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_ANY_STREAM, 0, &actual_index, &stream_flags,
            &timestamp, &sample);
    ok(hr == S_OK, "Failed to get a sample, hr %#x.\n", hr);
    ok(actual_index == 0, "Unexpected stream index %u\n", actual_index);
    ok(!stream_flags, "Unexpected stream flags %#x.\n", stream_flags);
    ok(timestamp == 123, "Unexpected timestamp.\n");
    ok(!!sample, "Expected sample object.\n");
    IMFSample_Release(sample);

    IMFSourceReader_Release(reader);
    IMFMediaSource_Release(source);

    /* Request from stream 0. */
    source = create_test_source(3);
    ok(!!source, "Failed to create test source.\n");

    hr = MFCreateSourceReaderFromMediaSource(source, NULL, &reader);
    ok(hr == S_OK, "Failed to create source reader, hr %#x.\n", hr);

    hr = IMFSourceReader_SetStreamSelection(reader, 0, TRUE);
    ok(hr == S_OK, "Failed to select a stream, hr %#x.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, 0, 0, &actual_index, &stream_flags, &timestamp, &sample);
    ok(hr == S_OK, "Failed to get a sample, hr %#x.\n", hr);
    ok(actual_index == 0, "Unexpected stream index %u\n", actual_index);
    ok(!stream_flags, "Unexpected stream flags %#x.\n", stream_flags);
    ok(timestamp == 123, "Unexpected timestamp.\n");
    ok(!!sample, "Expected sample object.\n");
    IMFSample_Release(sample);

    /* Request from deselected stream. */
    hr = IMFSourceReader_SetStreamSelection(reader, 1, FALSE);
    ok(hr == S_OK, "Failed to select a stream, hr %#x.\n", hr);

    actual_index = 0;
    stream_flags = 0;
    hr = IMFSourceReader_ReadSample(reader, 1, 0, &actual_index, &stream_flags, &timestamp, &sample);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#x.\n", hr);
    ok(actual_index == 1, "Unexpected stream index %u\n", actual_index);
    ok(stream_flags == MF_SOURCE_READERF_ERROR, "Unexpected stream flags %#x.\n", stream_flags);
    ok(timestamp == 0, "Unexpected timestamp.\n");
    ok(!sample, "Expected sample object.\n");

    IMFSourceReader_Release(reader);
    IMFMediaSource_Release(source);

    /* Request a non-native bit depth. */
    source = create_test_source(1);
    ok(!!source, "Failed to create test source.\n");

    hr = MFCreateSourceReaderFromMediaSource(source, NULL, &reader);
    ok(hr == S_OK, "Failed to create source reader, hr %#x.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = IMFSourceReader_SetCurrentMediaType(reader, 0, NULL, media_type);
    ok(hr == MF_E_TOPO_CODEC_NOT_FOUND, "Unexpected success setting current media type, hr %#x.\n", hr);

    IMFMediaType_Release(media_type);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_BITS_PER_SAMPLE, 32);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = IMFSourceReader_SetCurrentMediaType(reader, 0, NULL, media_type);
    ok(hr == S_OK, "Failed to set current media type, hr %#x.\n", hr);

    hr = IMFSourceReader_SetStreamSelection(reader, 0, TRUE);
    ok(hr == S_OK, "Failed to select a stream, hr %#x.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, 0, 0, &actual_index, &stream_flags, &timestamp, &sample);
    ok(hr == S_OK, "Failed to get a sample, hr %#x.\n", hr);
    ok(actual_index == 0, "Unexpected stream index %u\n", actual_index);
    ok(!stream_flags, "Unexpected stream flags %#x.\n", stream_flags);
    ok(timestamp == 123, "Unexpected timestamp.\n");
    ok(!!sample, "Expected sample object.\n");
    IMFSample_Release(sample);

    IMFMediaType_Release(media_type);
    IMFSourceReader_Release(reader);
    IMFMediaSource_Release(source);

    /* Async mode. */
    source = create_test_source(3);
    ok(!!source, "Failed to create test source.\n");

    callback = create_async_callback();

    hr = MFCreateAttributes(&attributes, 1);
    ok(hr == S_OK, "Failed to create attributes object, hr %#x.\n", hr);

    hr = IMFAttributes_SetUnknown(attributes, &MF_SOURCE_READER_ASYNC_CALLBACK,
            (IUnknown *)&callback->IMFSourceReaderCallback_iface);
    ok(hr == S_OK, "Failed to set attribute value, hr %#x.\n", hr);
    IMFSourceReaderCallback_Release(&callback->IMFSourceReaderCallback_iface);

    refcount = get_refcount(attributes);
    hr = MFCreateSourceReaderFromMediaSource(source, attributes, &reader);
    ok(hr == S_OK, "Failed to create source reader, hr %#x.\n", hr);
    ok(get_refcount(attributes) > refcount, "Unexpected refcount.\n");

    hr = IMFSourceReader_SetStreamSelection(reader, 0, TRUE);
    ok(hr == S_OK, "Failed to select a stream, hr %#x.\n", hr);

    /* Return values are delivered to callback only. */
    hr = IMFSourceReader_ReadSample(reader, 0, 0, &actual_index, &stream_flags, &timestamp, &sample);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, 0, 0, NULL, &stream_flags, &timestamp, &sample);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, 0, 0, NULL, NULL, &timestamp, &sample);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, 0, 0, NULL, NULL, NULL, &sample);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    /* Flush() arguments validation. */
    hr = IMFSourceReader_Flush(reader, 123);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, 0, 0, NULL, NULL, NULL, NULL);
    ok(hr == MF_E_NOTACCEPTING, "Unexpected hr %#x.\n", hr);

    IMFSourceReader_Release(reader);
    IMFMediaSource_Release(source);

    /* RequestSample failure. */
    source = create_test_source(3);
    ok(!!source, "Failed to create test source.\n");

    fail_request_sample = TRUE;

    hr = MFCreateSourceReaderFromMediaSource(source, NULL, &reader);
    ok(hr == S_OK, "Failed to create source reader, hr %#x.\n", hr);

    hr = IMFSourceReader_SetStreamSelection(reader, 0, TRUE);
    ok(hr == S_OK, "Failed to select a stream, hr %#x.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, 0, 0, &actual_index, &stream_flags, &timestamp, &sample);
    ok(hr == E_NOTIMPL, "Unexpected ReadSample result, hr %#x.\n", hr);

    actual_index = ~0u;
    stream_flags = 0;
    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &actual_index, &stream_flags,
            &timestamp, &sample);
    ok(hr == E_NOTIMPL, "Unexpected ReadSample result, hr %#x.\n", hr);
    ok(actual_index == 0, "Unexpected index %u.\n", actual_index);
    ok(stream_flags == MF_SOURCE_READERF_ERROR, "Unexpected flags %#x.\n", stream_flags);

    actual_index = ~0u;
    stream_flags = 0;
    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &actual_index, &stream_flags,
            &timestamp, &sample);
    ok(hr == E_NOTIMPL, "Unexpected ReadSample result, hr %#x.\n", hr);
    ok(actual_index == 0, "Unexpected index %u.\n", actual_index);
    ok(stream_flags == MF_SOURCE_READERF_ERROR, "Unexpected flags %#x.\n", stream_flags);

    actual_index = ~0u;
    stream_flags = 0;
    hr = IMFSourceReader_ReadSample(reader, 0, 0, &actual_index, &stream_flags, &timestamp, &sample);
    ok(hr == E_NOTIMPL, "Unexpected ReadSample result, hr %#x.\n", hr);
    ok(actual_index == 0, "Unexpected index %u.\n", actual_index);
    ok(stream_flags == MF_SOURCE_READERF_ERROR, "Unexpected flags %#x.\n", stream_flags);

    IMFSourceReader_Release(reader);
    IMFMediaSource_Release(source);

    fail_request_sample = FALSE;
}

static void test_reader_d3d9(void)
{
    IDirect3DDeviceManager9 *d3d9_manager;
    IDirect3DDevice9 *d3d9_device;
    IMFAttributes *attributes;
    IMFSourceReader *reader;
    IMFMediaSource *source;
    IDirect3D9 *d3d9;
    HWND window;
    HRESULT hr;
    UINT token;

    window = create_window();
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D9 object.\n");
    if (!(d3d9_device = create_d3d9_device(d3d9, window)))
    {
        skip("Failed to create a D3D9 device, skipping tests.\n");
        goto done;
    }

    hr = DXVA2CreateDirect3DDeviceManager9(&token, &d3d9_manager);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_ResetDevice(d3d9_manager, d3d9_device, token);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    source = create_test_source(3);
    ok(!!source, "Failed to create test source.\n");

    hr = MFCreateAttributes(&attributes, 1);
    ok(hr == S_OK, "Failed to create attributes object, hr %#x.\n", hr);

    hr = IMFAttributes_SetUnknown(attributes, &MF_SOURCE_READER_D3D_MANAGER, (IUnknown *)d3d9_manager);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = MFCreateSourceReaderFromMediaSource(source, attributes, &reader);
    ok(hr == S_OK, "Failed to create source reader, hr %#x.\n", hr);

    IMFAttributes_Release(attributes);

    IMFSourceReader_Release(reader);

    IDirect3DDeviceManager9_Release(d3d9_manager);
    IDirect3DDevice9_Release(d3d9_device);

done:
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

START_TEST(mfplat)
{
    HRESULT hr;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    init_functions();

    test_factory();
    test_source_reader();
    test_source_reader_from_media_source();
    test_reader_d3d9();

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);
}
