/*
 * Copyright 2022 Nikolay Sivov for CodeWeavers
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

#include "mfapi.h"
#include "mfidl.h"
#include "mferror.h"
#include "wine/mfinternal.h"

#include "wine/test.h"

static const char *debugstr_time(LONGLONG time)
{
    ULONGLONG abstime = time >= 0 ? time : -time;
    unsigned int i = 0, j = 0;
    char buffer[23], rev[23];

    while (abstime || i <= 8)
    {
        buffer[i++] = '0' + (abstime % 10);
        abstime /= 10;
        if (i == 7) buffer[i++] = '.';
    }
    if (time < 0) buffer[i++] = '-';

    while (i--) rev[j++] = buffer[i];
    while (rev[j-1] == '0' && rev[j-2] != '.') --j;
    rev[j] = 0;

    return wine_dbg_sprintf("%s", rev);
}

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

static void test_wave_sink(void)
{
    IMFMediaType *media_type, *media_type2;
    IMFMediaTypeHandler *type_handler;
    IMFPresentationClock *clock;
    IMFStreamSink *stream_sink;
    IMFMediaSink *sink, *sink2;
    IMFByteStream *bytestream;
    DWORD id, count, flags;
    HRESULT hr;
    GUID guid;

    hr = MFCreateWAVEMediaSink(NULL, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateWAVEMediaSink(NULL, NULL, &sink);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_BLOCK_ALIGNMENT, 2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_NUM_CHANNELS, 2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_BITS_PER_SAMPLE, 8);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateTempFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_DELETE_IF_EXIST, 0, &bytestream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateWAVEMediaSink(bytestream, NULL, &sink);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateWAVEMediaSink(bytestream, media_type, &sink);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Sink tests */
    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(flags == (MEDIASINK_FIXED_STREAMS | MEDIASINK_RATELESS), "Unexpected flags %#lx.\n", flags);

    hr = IMFMediaSink_GetStreamSinkCount(sink, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_GetStreamSinkCount(sink, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %lu.\n", count);

    hr = IMFMediaSink_AddStreamSink(sink, 123, media_type, &stream_sink);
    ok(hr == MF_E_STREAMSINKS_FIXED, "Unexpected hr %#lx.\n", hr);

    check_interface(sink, &IID_IMFMediaEventGenerator, TRUE);
    check_interface(sink, &IID_IMFFinalizableMediaSink, TRUE);
    check_interface(sink, &IID_IMFClockStateSink, TRUE);

    /* Clock */
    hr = MFCreatePresentationClock(&clock);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_SetPresentationClock(sink, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_SetPresentationClock(sink, clock);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFPresentationClock_Release(clock);

    /* Stream tests */
    hr = IMFMediaSink_GetStreamSinkByIndex(sink, 0, &stream_sink);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFStreamSink_GetIdentifier(stream_sink, &id);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(id == 1, "Unexpected id %#lx.\n", id);
    IMFStreamSink_Release(stream_sink);

    hr = IMFMediaSink_GetStreamSinkById(sink, 0, &stream_sink);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaSink_GetStreamSinkById(sink, id, &stream_sink);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFStreamSink_GetMediaSink(stream_sink, &sink2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaSink_Release(sink2);

    check_interface(stream_sink, &IID_IMFMediaEventGenerator, TRUE);
    check_interface(stream_sink, &IID_IMFMediaTypeHandler, TRUE);

    hr = IMFStreamSink_GetMediaTypeHandler(stream_sink, &type_handler);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetMajorType(type_handler, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaTypeHandler_GetMajorType(type_handler, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Audio), "Unexpected major type.\n");

    hr = IMFMediaTypeHandler_GetMediaTypeCount(type_handler, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %lu.\n", count);

    hr = IMFMediaTypeHandler_GetCurrentMediaType(type_handler, &media_type2);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = IMFMediaType_SetUINT32(media_type2, &MF_MT_AUDIO_NUM_CHANNELS, 1);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFMediaTypeHandler_SetCurrentMediaType(type_handler, NULL);
        ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
        hr = IMFMediaTypeHandler_SetCurrentMediaType(type_handler, media_type2);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        IMFMediaType_Release(media_type2);
    }

    /* Shutdown state */
    hr = IMFMediaSink_Shutdown(sink);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaSink_Shutdown(sink);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFStreamSink_GetMediaSink(stream_sink, &sink2);
    ok(hr == MF_E_STREAMSINK_REMOVED, "Unexpected hr %#lx.\n", hr);

    hr = IMFStreamSink_GetIdentifier(stream_sink, &id);
    ok(hr == MF_E_STREAMSINK_REMOVED, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetMajorType(type_handler, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaTypeHandler_GetMajorType(type_handler, &guid);
    ok(hr == MF_E_STREAMSINK_REMOVED, "Unexpected hr %#lx.\n", hr);

    IMFStreamSink_Release(stream_sink);

    hr = IMFMediaSink_AddStreamSink(sink, 123, media_type, &stream_sink);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaSink_GetStreamSinkByIndex(sink, 0, &stream_sink);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaSink_GetStreamSinkById(sink, 0, &stream_sink);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaSink_GetCharacteristics(sink, &flags);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    IMFMediaSink_Release(sink);

    IMFMediaTypeHandler_Release(type_handler);
    IMFMediaType_Release(media_type);
    IMFByteStream_Release(bytestream);
}

struct source_create_callback
{
    IMFAsyncCallback iface;
    LONG refcount;

    IMFByteStreamHandler *handler;
    HRESULT hr;
    MF_OBJECT_TYPE type;
    IUnknown *object;
    HANDLE event;
};

struct source_create_callback *source_create_callback_from_iface(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct source_create_callback, iface);
}

static HRESULT WINAPI source_create_callback_QueryInterface(IMFAsyncCallback *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFAsyncCallback) || IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFAsyncCallback_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI source_create_callback_AddRef(IMFAsyncCallback *iface)
{
    struct source_create_callback *callback = source_create_callback_from_iface(iface);
    return InterlockedIncrement(&callback->refcount);
}

static ULONG WINAPI source_create_callback_Release(IMFAsyncCallback *iface)
{
    struct source_create_callback *callback = source_create_callback_from_iface(iface);
    ULONG refcount = InterlockedDecrement(&callback->refcount);
    if (refcount == 0)
    {
        if (callback->object)
            IUnknown_Release(callback->object);
        IMFByteStreamHandler_Release(callback->handler);
        CloseHandle(callback->event);
        free(callback);
    }
    return refcount;
}

static HRESULT WINAPI source_create_callback_GetParameters(IMFAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI source_create_callback_stream_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct source_create_callback *callback = source_create_callback_from_iface(iface);
    callback->hr = IMFByteStreamHandler_EndCreateObject(callback->handler, result, &callback->type,
            &callback->object);
    SetEvent(callback->event);
    return callback->hr;
}

static const IMFAsyncCallbackVtbl source_create_callback_vtbl =
{
    &source_create_callback_QueryInterface,
    &source_create_callback_AddRef,
    &source_create_callback_Release,
    &source_create_callback_GetParameters,
    &source_create_callback_stream_Invoke,
};

static HRESULT create_source(const GUID *guid_handler, IMFByteStream *stream, IMFMediaSource **source)
{
    HRESULT hr;
    IMFByteStreamHandler *handler;
    struct source_create_callback *callback;

    if (!(callback = calloc(1, sizeof *callback)))
        return E_OUTOFMEMORY;
    hr = CoCreateInstance(guid_handler, NULL, CLSCTX_INPROC_SERVER, &IID_IMFByteStreamHandler, (void **)&handler);
    if (FAILED(hr))
    {
        free(callback);
        return hr;
    }
    callback->iface.lpVtbl = &source_create_callback_vtbl;
    callback->refcount = 1;
    callback->handler = handler;
    callback->object = NULL;
    callback->type = MF_OBJECT_INVALID;
    callback->hr = E_PENDING;
    callback->event = CreateEventW(NULL, FALSE, FALSE, NULL);

    hr = IMFByteStreamHandler_BeginCreateObject(callback->handler, stream, NULL,
            MF_RESOLUTION_MEDIASOURCE, NULL, NULL, &callback->iface, NULL);
    if (FAILED(hr))
        goto done;

    WaitForSingleObject(callback->event, INFINITE);
    if (FAILED(hr = callback->hr))
        goto done;
    if (callback->type != MF_OBJECT_MEDIASOURCE)
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    hr = S_OK;
    *source = (IMFMediaSource *)callback->object;
    callback->object = NULL;

done:
    IMFAsyncCallback_Release(&callback->iface);
    return hr;
}

static IMFByteStream *create_byte_stream(const BYTE *data, ULONG data_len)
{
    IMFByteStream *stream;
    HRESULT hr;

    hr = MFCreateTempFile(MF_ACCESSMODE_READWRITE, MF_OPENMODE_DELETE_IF_EXIST, MF_FILEFLAGS_NONE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFByteStream_Write(stream, data, data_len, &data_len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFByteStream_SetCurrentPosition(stream, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    return stream;
}

static IMFByteStream *create_resource_byte_stream(const WCHAR *name)
{
    const BYTE *resource_data;
    ULONG resource_len;
    HRSRC resource;

    resource = FindResourceW(NULL, name, (const WCHAR *)RT_RCDATA);
    ok(resource != 0, "FindResourceW %s failed, error %lu\n", debugstr_w(name), GetLastError());
    resource_data = LockResource(LoadResource(GetModuleHandleW(NULL), resource));
    resource_len = SizeofResource(GetModuleHandleW(NULL), resource);

    return create_byte_stream(resource_data, resource_len);
}

struct test_callback
{
    IMFAsyncCallback IMFAsyncCallback_iface;
    LONG refcount;

    HANDLE event;
    IMFMediaEvent *media_event;
    BOOL check_media_event;
};

static struct test_callback *impl_from_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct test_callback, IMFAsyncCallback_iface);
}

static HRESULT WINAPI testcallback_QueryInterface(IMFAsyncCallback *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFAsyncCallback_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI testcallback_AddRef(IMFAsyncCallback *iface)
{
    struct test_callback *callback = impl_from_IMFAsyncCallback(iface);
    return InterlockedIncrement(&callback->refcount);
}

static ULONG WINAPI testcallback_Release(IMFAsyncCallback *iface)
{
    struct test_callback *callback = impl_from_IMFAsyncCallback(iface);
    ULONG refcount = InterlockedDecrement(&callback->refcount);

    if (!refcount)
    {
        if (callback->media_event)
            IMFMediaEvent_Release(callback->media_event);
        CloseHandle(callback->event);
        free(callback);
    }

    return refcount;
}

static HRESULT WINAPI testcallback_GetParameters(IMFAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    ok(flags != NULL && queue != NULL, "Unexpected arguments.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testcallback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct test_callback *callback = CONTAINING_RECORD(iface, struct test_callback, IMFAsyncCallback_iface);
    IUnknown *object;
    HRESULT hr;

    ok(result != NULL, "Unexpected result object.\n");
    ok(!callback->media_event, "Event already present.\n");

    if (callback->check_media_event)
    {
        hr = IMFAsyncResult_GetObject(result, &object);
        ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

        hr = IMFAsyncResult_GetState(result, &object);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        callback->media_event = (void *)0xdeadbeef;
        hr = IMFMediaEventGenerator_EndGetEvent((IMFMediaEventGenerator *)object,
                result, &callback->media_event);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        IUnknown_Release(object);
    }

    SetEvent(callback->event);

    return S_OK;
}

static const IMFAsyncCallbackVtbl testcallbackvtbl =
{
    testcallback_QueryInterface,
    testcallback_AddRef,
    testcallback_Release,
    testcallback_GetParameters,
    testcallback_Invoke,
};

static IMFAsyncCallback *create_test_callback(BOOL check_media_event)
{
    struct test_callback *callback;

    if (!(callback = calloc(1, sizeof(*callback))))
        return NULL;

    callback->refcount = 1;
    callback->check_media_event = check_media_event;
    callback->IMFAsyncCallback_iface.lpVtbl = &testcallbackvtbl;
    callback->event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(!!callback->event, "CreateEventW failed, error %lu\n", GetLastError());

    return &callback->IMFAsyncCallback_iface;
}

#define next_media_event(a, b, c, d) next_media_event_(__LINE__, (IMFMediaEventGenerator *)a, b, c, d)
static DWORD next_media_event_(int line, IMFMediaEventGenerator *source, IMFAsyncCallback *callback, DWORD timeout,
        IMFMediaEvent **event)
{
    struct test_callback *impl = impl_from_IMFAsyncCallback(callback);
    HRESULT hr;
    DWORD ret;

    hr = IMFMediaEventGenerator_BeginGetEvent(source, &impl->IMFAsyncCallback_iface, (IUnknown *)source);
    ok_(__FILE__, line)(hr == S_OK || hr == MF_S_MULTIPLE_BEGIN || hr == MF_E_MULTIPLE_SUBSCRIBERS, "Unexpected hr %#lx.\n", hr);
    ret = WaitForSingleObject(impl->event, timeout);
    *event = impl->media_event;
    impl->media_event = NULL;

    return ret;
}

#define wait_media_event(a, b, c, d, e) wait_media_event_(__LINE__, (IMFMediaEventGenerator *)a, b, c, d, e)
static HRESULT wait_media_event_(int line, IMFMediaEventGenerator *source, IMFAsyncCallback *callback,
        MediaEventType expect_type, DWORD timeout, PROPVARIANT *value)
{
    IMFMediaEvent *event = NULL;
    MediaEventType type;
    HRESULT hr, status;
    DWORD ret;
    GUID guid;

    do
    {
        if (event) IMFMediaEvent_Release(event);
        ret = next_media_event_(line, source, callback, timeout, &event);
        if (ret) return MF_E_NO_EVENTS_AVAILABLE;
        hr = IMFMediaEvent_GetType(event, &type);
        ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok_(__FILE__, line)(type == expect_type, "got %#lx.\n", type);
    } while (type != expect_type);

    hr = IMFMediaEvent_GetExtendedType(event, &guid);
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(__FILE__, line)(IsEqualGUID(&guid, &GUID_NULL), "got extended type %s\n", debugstr_guid(&guid));

    hr = IMFMediaEvent_GetValue(event, value);
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEvent_GetStatus(event, &status);
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFMediaEvent_Release(event);
    return status;
}

static void test_sample_times_at_rate(IMFMediaSource *source, FLOAT rate, BOOL thin)
{
    static LONGLONG expect_times[] =
    {
              0, 333666,
        1668333, 333666,
        2002000, 333666,
        2335666, 333666,
        2669333, 333666,
    };
    static LONGLONG expect_times_thin[ARRAY_SIZE(expect_times)] =
    {
              0, 333666,
        1668333, 333666,
        3336666, 333666,
        5005000, 333666,
        6673333, 333666,
    };
    IMFAsyncCallback *callback, *source_callback;
    IMFRateControl *rate_control;
    IMFPresentationDescriptor *pd;
    IMFMediaStream *stream;
    IMFMediaEvent *event;
    PROPVARIANT value;
    LONGLONG time;
    HRESULT hr;
    DWORD ret;

    winetest_push_context("%f/%u", rate, thin);

    hr = IMFMediaSource_CreatePresentationDescriptor(source, &pd);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value.vt = VT_EMPTY;
    hr = IMFMediaSource_Start(source, pd, &GUID_NULL, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFPresentationDescriptor_Release(pd);

    callback = create_test_callback(TRUE);
    source_callback = create_test_callback(TRUE);
    if (!winetest_platform_is_wine)
    {
        hr = wait_media_event(source, source_callback, thin ? MENewStream : MEUpdatedStream, 100, &value);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    }
    else
    {
        ret = next_media_event(source, source_callback, 100, &event);
        ok(ret == 0, "Unexpected ret %#lx.\n", ret);
        hr = IMFMediaEvent_GetType(event, &ret);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        todo_wine_if(!thin)
        ok(ret == (thin ? MENewStream : MEUpdatedStream), "Unexpected type %#lx.\n", ret);
        hr = IMFMediaEvent_GetValue(event, &value);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        IMFMediaEvent_Release(event);
    }
    ok(value.vt == VT_UNKNOWN, "got vt %u\n", value.vt);
    stream = (IMFMediaStream *)value.punkVal;
    IMFMediaStream_AddRef(stream);
    PropVariantClear(&value);

    hr = wait_media_event(stream, callback, MEStreamStarted, 100, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_I8, "got vt %u\n", value.vt);
    hr = wait_media_event(source, source_callback, MESourceStarted, 100, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_I8, "got vt %u\n", value.vt);

    winetest_push_context("sample 0");

    hr = IMFMediaStream_RequestSample(stream, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = wait_media_event(stream, callback, MEMediaSample, 100, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_UNKNOWN, "got vt %u\n", value.vt);
    hr = IMFSample_GetSampleTime((IMFSample *)value.punkVal, &time);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(time == expect_times[0], "Unexpected time %s.\n", debugstr_time(time));
    hr = IMFSample_GetSampleDuration((IMFSample *)value.punkVal, &time);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(time == expect_times[1], "Unexpected time %s.\n", debugstr_time(time));
    hr = IMFSample_GetSampleFlags((IMFSample *)value.punkVal, &ret);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(ret == 0, "Unexpected flags %#lx.\n", ret);
    PropVariantClear(&value);

    winetest_pop_context();

    hr = MFGetService((IUnknown *)source, &MF_RATE_CONTROL_SERVICE, &IID_IMFRateControl, (void **)&rate_control);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFRateControl_SetRate(rate_control, thin, rate);
    todo_wine_if(thin && hr == MF_E_THINNING_UNSUPPORTED)
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = wait_media_event(source, source_callback, MESourceRateChanged, 100, &value);
    todo_wine_if(thin)
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(value.vt == VT_R4, "got vt %u\n", value.vt);
    ret = next_media_event(source, source_callback, 100, &event);
    ok(ret == WAIT_TIMEOUT, "Unexpected ret %#lx.\n", ret);

    hr = IMFMediaStream_RequestSample(stream, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (!winetest_platform_is_wine)
    {
    hr = wait_media_event(stream, callback, MEStreamThinMode, 100, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_INT, "got vt %u\n", value.vt);
    ok(value.iVal == thin, "Unexpected thin %d\n", value.iVal);
    }

    winetest_push_context("sample 1");

    hr = wait_media_event(stream, callback, MEMediaSample, 100, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_UNKNOWN, "got vt %u\n", value.vt);
    hr = IMFSample_GetSampleTime((IMFSample *)value.punkVal, &time);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(time == expect_times[2], "Unexpected time %s.\n", debugstr_time(time));
    hr = IMFSample_GetSampleDuration((IMFSample *)value.punkVal, &time);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(time == expect_times[3], "Unexpected time %s.\n", debugstr_time(time));
    hr = IMFSample_GetSampleFlags((IMFSample *)value.punkVal, &ret);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(ret == 0, "Unexpected flags %#lx.\n", ret);
    PropVariantClear(&value);

    winetest_pop_context();

    for (int i = 2; i < ARRAY_SIZE(expect_times) / 2; i++)
    {
        winetest_push_context("sample %u", i);

        hr = IMFMediaStream_RequestSample(stream, NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = wait_media_event(stream, callback, MEMediaSample, 100, &value);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(value.vt == VT_UNKNOWN, "got vt %u\n", value.vt);
        hr = IMFSample_GetSampleTime((IMFSample *)value.punkVal, &time);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        todo_wine
        ok(time == (thin ? expect_times_thin[2 * i] : expect_times[2 * i]), "Unexpected time %s.\n", debugstr_time(time));
        hr = IMFSample_GetSampleDuration((IMFSample *)value.punkVal, &time);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(time == (thin ? expect_times_thin[2 * i + 1] : expect_times[2 * i + 1]), "Unexpected time %s.\n", debugstr_time(time));
        hr = IMFSample_GetSampleFlags((IMFSample *)value.punkVal, &ret);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(ret == 0, "Unexpected flags %#lx.\n", ret);
        PropVariantClear(&value);

        winetest_pop_context();
    }

    /* change rate after sample requests */
    hr = IMFMediaStream_RequestSample(stream, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaStream_RequestSample(stream, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaStream_RequestSample(stream, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaStream_RequestSample(stream, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaStream_RequestSample(stream, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaStream_RequestSample(stream, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFRateControl_SetRate(rate_control, !thin, rate+0.5);
    todo_wine_if(!thin && hr == MF_E_THINNING_UNSUPPORTED)
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* rate change event is available immediately */
    hr = wait_media_event(source, source_callback, MESourceRateChanged, 100, &value);
    todo_wine_if(!thin && hr == MF_E_NO_EVENTS_AVAILABLE)
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = wait_media_event(stream, callback, MEMediaSample, 100, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    PropVariantClear(&value);
    hr = wait_media_event(stream, callback, MEMediaSample, 100, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    PropVariantClear(&value);
    hr = wait_media_event(stream, callback, MEMediaSample, 100, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    PropVariantClear(&value);
    hr = wait_media_event(stream, callback, MEMediaSample, 100, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    PropVariantClear(&value);
    hr = wait_media_event(stream, callback, MEMediaSample, 100, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    PropVariantClear(&value);
    hr = wait_media_event(stream, callback, MEMediaSample, 100, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    PropVariantClear(&value);

    /* thin mode only changes when a new sample is requested */
    hr = wait_media_event(stream, callback, MEStreamThinMode, 1000, &value);
    ok(MF_E_NO_EVENTS_AVAILABLE == hr, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaStream_RequestSample(stream, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (!thin)
    {
        /* next sample is already a keyframe. thin mode only changes after some samples get skipped */
        hr = wait_media_event(stream, callback, MEMediaSample, 100, &value);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        PropVariantClear(&value);
        hr = IMFMediaStream_RequestSample(stream, NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    }
    if (!winetest_platform_is_wine)
    {
    hr = wait_media_event(stream, callback, MEStreamThinMode, 1000, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    }
    hr = wait_media_event(stream, callback, MEMediaSample, 100, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    PropVariantClear(&value);

    /* switch back to previous mode */
    hr = IMFRateControl_SetRate(rate_control, thin, rate+0.5);
    todo_wine_if(thin && hr == MF_E_THINNING_UNSUPPORTED)
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = wait_media_event(source, source_callback, MESourceRateChanged, 100, &value);
    todo_wine_if(thin && hr == MF_E_NO_EVENTS_AVAILABLE)
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaStream_RequestSample(stream, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (!winetest_platform_is_wine)
    {
    hr = wait_media_event(stream, callback, MEStreamThinMode, 1000, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    }
    if (thin && !winetest_platform_is_wine)
    {
        hr = wait_media_event(stream, callback, MEEndOfStream, 100, &value);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = wait_media_event(source, source_callback, MEEndOfPresentation, 100, &value);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    }
    else
    {
        hr = wait_media_event(stream, callback, MEMediaSample, 100, &value);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        PropVariantClear(&value);
    }

    hr = IMFMediaSource_Stop(source);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = wait_media_event(source, source_callback, MESourceStopped, 100, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_EMPTY, "got vt %u\n", value.vt);
    hr = wait_media_event(stream, callback, MEStreamStopped, 100, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_EMPTY, "got vt %u\n", value.vt);

    IMFMediaStream_Release(stream);
    IMFAsyncCallback_Release(callback);
    IMFAsyncCallback_Release(source_callback);
    IMFRateControl_Release(rate_control);

    winetest_pop_context();
}

static void test_thinning(void)
{
    IMFMediaSource *source;
    IMFByteStream *stream;
    HRESULT hr;

    stream = create_resource_byte_stream(L"test_thinning.avi");
    hr = create_source(&CLSID_AVIByteStreamPlugin, stream, &source);
    IMFByteStream_Release(stream);

    if (FAILED(hr))
    {
        win_skip("Failed to create MPEG4 source: %#lx.\n", hr);
        return;
    }

    test_sample_times_at_rate(source, 2.0, TRUE);
    test_sample_times_at_rate(source, 3.0, FALSE);

    hr = IMFMediaSource_Shutdown(source);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaSource_Release(source);
}

START_TEST(mfsrcsnk)
{
    HRESULT hr;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    test_wave_sink();
    test_thinning();

    hr = MFShutdown();
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
}
