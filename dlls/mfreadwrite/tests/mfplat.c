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
#include <stdbool.h>
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
#include "propvarutil.h"
#include "initguid.h"
#include "d3d9.h"
#include "dxva2api.h"
#include "d3d11_4.h"
#include "evr.h"

#include "wine/test.h"

DEFINE_MEDIATYPE_GUID(MFVideoFormat_TEST,MAKEFOURCC('T','E','S','T'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_ABGR32,D3DFMT_A8B8G8R8);

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

#define check_service_interface(a, b, c, d) check_service_interface_(__LINE__, a, b, c, d)
static void check_service_interface_(unsigned int line, void *iface_ptr, REFGUID service, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = MFGetService(iface, service, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

struct attribute_desc
{
    const GUID *key;
    const char *name;
    PROPVARIANT value;
    BOOL ratio;
    BOOL required;
    BOOL todo;
    BOOL todo_value;
    BOOL not_present;
};

#define ATTR_GUID(k, g, ...)      {.key = &k, .name = #k, {.vt = VT_CLSID, .puuid = (GUID *)&g}, __VA_ARGS__ }
#define ATTR_UINT32(k, v, ...)    {.key = &k, .name = #k, {.vt = VT_UI4, .ulVal = v}, __VA_ARGS__ }
#define ATTR_BLOB(k, p, n, ...)   {.key = &k, .name = #k, {.vt = VT_VECTOR | VT_UI1, .caub = {.pElems = (void *)p, .cElems = n}}, __VA_ARGS__ }
#define ATTR_RATIO(k, n, d, ...)  {.key = &k, .name = #k, {.vt = VT_UI8, .uhVal = {.HighPart = n, .LowPart = d}}, .ratio = TRUE, __VA_ARGS__ }
#define ATTR_UINT64(k, v, ...)    {.key = &k, .name = #k, {.vt = VT_UI8, .uhVal = {.QuadPart = v}}, __VA_ARGS__ }

#define check_media_type(a, b, c) check_attributes_(__FILE__, __LINE__, (IMFAttributes *)a, b, c)
#define check_attributes(a, b, c) check_attributes_(__FILE__, __LINE__, a, b, c)
void check_attributes_(const char *file, int line, IMFAttributes *attributes,
        const struct attribute_desc *desc, ULONG limit)
{
    char buffer[1024], *buf = buffer;
    PROPVARIANT value;
    int i, j, ret;
    HRESULT hr;

    for (i = 0; i < limit && desc[i].key; ++i)
    {
        hr = IMFAttributes_GetItem(attributes, desc[i].key, &value);
        todo_wine_if(desc[i].todo)
        if (desc[i].not_present)
            ok_(file, line)(hr == MF_E_ATTRIBUTENOTFOUND, "%s present, hr %#lx\n", debugstr_a(desc[i].name), hr);
        else
            ok_(file, line)(hr == S_OK, "%s missing, hr %#lx\n", debugstr_a(desc[i].name), hr);
        if (hr != S_OK) continue;

        switch (value.vt)
        {
        default: sprintf(buffer, "??"); break;
        case VT_CLSID: sprintf(buffer, "%s", debugstr_guid(value.puuid)); break;
        case VT_UI4: sprintf(buffer, "%lu", value.ulVal); break;
        case VT_UI8:
            if (desc[i].ratio)
                sprintf(buffer, "%lu:%lu", value.uhVal.HighPart, value.uhVal.LowPart);
            else
                sprintf(buffer, "%I64u", value.uhVal.QuadPart);
            break;
        case VT_VECTOR | VT_UI1:
            buf += sprintf(buf, "size %lu, data {", value.caub.cElems);
            for (j = 0; j < 128 && j < value.caub.cElems; ++j)
                buf += sprintf(buf, "0x%02x,", value.caub.pElems[j]);
            if (value.caub.cElems > 128)
                buf += sprintf(buf, "...}");
            else
                buf += sprintf(buf - (j ? 1 : 0), "}");
            break;
        }

        ret = PropVariantCompareEx(&value, &desc[i].value, 0, 0);
        todo_wine_if(desc[i].todo_value)
        ok_(file, line)(ret == 0, "%s mismatch, type %u, value %s\n",
                debugstr_a(desc[i].name), value.vt, buffer);
        PropVariantClear(&value);
    }
}

#define init_media_type(a, b, c) init_attributes_(__FILE__, __LINE__, (IMFAttributes *)a, b, c)
#define init_attributes(a, b, c) init_attributes_(__FILE__, __LINE__, a, b, c)
static void init_attributes_(const char *file, int line, IMFAttributes *attributes,
        const struct attribute_desc *desc, ULONG limit)
{
    HRESULT hr;
    ULONG i;

    hr = IMFAttributes_DeleteAllItems(attributes);
    ok_(file, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < limit && desc[i].key; ++i)
    {
        hr = IMFAttributes_SetItem(attributes, desc[i].key, &desc[i].value);
        ok_(file, line)(hr == S_OK, "SetItem %s returned %#lx\n", debugstr_a(desc[i].name), hr);
    }
}

static IMFSample *create_sample(const BYTE *data, DWORD size)
{
    IMFMediaBuffer *media_buffer;
    IMFSample *sample;
    BYTE *buffer;
    DWORD length;
    HRESULT hr;
    ULONG ret;

    hr = MFCreateSample(&sample);
    ok(hr == S_OK, "MFCreateSample returned %#lx\n", hr);
    hr = MFCreateMemoryBuffer(size, &media_buffer);
    ok(hr == S_OK, "MFCreateMemoryBuffer returned %#lx\n", hr);

    hr = IMFMediaBuffer_Lock(media_buffer, &buffer, NULL, &length);
    ok(hr == S_OK, "Lock returned %#lx\n", hr);
    ok(length == 0, "Unexpected length %lu\n", length);
    memcpy(buffer, data, size);
    hr = IMFMediaBuffer_Unlock(media_buffer);
    ok(hr == S_OK, "Unlock returned %#lx\n", hr);

    hr = IMFMediaBuffer_SetCurrentLength(media_buffer, size);
    ok(hr == S_OK, "SetCurrentLength returned %#lx\n", hr);
    hr = IMFSample_AddBuffer(sample, media_buffer);
    ok(hr == S_OK, "AddBuffer returned %#lx\n", hr);
    ret = IMFMediaBuffer_Release(media_buffer);
    ok(ret == 1, "Release returned %lu\n", ret);

    return sample;
}

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
static HRESULT (WINAPI *pMFCreateDXGIDeviceManager)(UINT *token, IMFDXGIDeviceManager **manager);

static void init_functions(void)
{
    HMODULE mod = GetModuleHandleA("mfplat.dll");

#define X(f) if (!(p##f = (void*)GetProcAddress(mod, #f))) return;
    X(MFCreateMFByteStreamOnStream);
    X(MFCreateDXGIDeviceManager);
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
    LONGLONG sample_duration, sample_time;
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
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (stream->sample_duration)
    {
        hr = IMFSample_SetSampleDuration(sample, stream->sample_duration);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFSample_SetSampleTime(sample, stream->sample_time);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        stream->sample_time += stream->sample_duration;
    }
    else
    {
        hr = IMFSample_SetSampleTime(sample, 123);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFSample_SetSampleDuration(sample, 1);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    }

    if (token)
        IMFSample_SetUnknown(sample, &MFSampleExtension_Token, token);

    /* Reader expects buffers, empty samples are considered an error. */
    hr = MFCreateMemoryBuffer(8, &buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFSample_AddBuffer(sample, buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaBuffer_Release(buffer);

    hr = IMFMediaEventQueue_QueueEventParamUnk(stream->event_queue, MEMediaSample, &GUID_NULL, S_OK,
            (IUnknown *)sample);
    ok(hr == S_OK || hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);
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
    HRESULT hr = S_OK;

    *pd = source->pd;
    IMFPresentationDescriptor_AddRef(*pd);

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
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < source->stream_count; ++i)
    {
        if (!is_stream_selected(pd, i))
            continue;

        var.vt = VT_UNKNOWN;
        var.punkVal = (IUnknown *)&source->streams[i]->IMFMediaStream_iface;
        event_type = source->streams[i]->is_new ? MENewStream : MEUpdatedStream;
        source->streams[i]->is_new = FALSE;
        hr = IMFMediaEventQueue_QueueEventParamVar(source->event_queue, event_type, &GUID_NULL, S_OK, &var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        event_type = source->state == SOURCE_RUNNING ? MEStreamSeeked : MEStreamStarted;
        hr = IMFMediaEventQueue_QueueEventParamVar(source->streams[i]->event_queue, event_type, &GUID_NULL,
                S_OK, NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
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
    UINT i;

    hr = IMFMediaEventQueue_Shutdown(source->event_queue);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    for (i = 0; i < source->stream_count; ++i)
    {
        hr = IMFMediaEventQueue_Shutdown(source->streams[i]->event_queue);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    }

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
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    stream->source = source;
    IMFMediaSource_AddRef(stream->source);
    stream->is_new = TRUE;

    IMFMediaSource_CreatePresentationDescriptor(source, &pd);
    IMFPresentationDescriptor_GetStreamDescriptorByIndex(pd, stream_index, &selected, &stream->sd);
    IMFPresentationDescriptor_Release(pd);

    return stream;
}

static IMFMediaSource *create_test_source(IMFStreamDescriptor **streams, UINT stream_count)
{
    struct test_source *source;
    HRESULT hr;
    int i;

    source = calloc(1, sizeof(*source));
    source->IMFMediaSource_iface.lpVtbl = &test_source_vtbl;
    source->refcount = 1;
    source->stream_count = stream_count;
    hr = MFCreatePresentationDescriptor(stream_count, streams, &source->pd);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFCreateEventQueue(&source->event_queue);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
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
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IStream_Write(stream, ptr, SizeofResource(GetModuleHandleA(NULL), res), &written);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pMFCreateMFByteStreamOnStream(stream, &bytestream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
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
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CoCreateInstance(&CLSID_MFReadWriteClassFactory, (IUnknown *)factory, CLSCTX_INPROC_SERVER, &IID_IMFReadWriteClassFactory,
            (void **)&factory2);
    ok(hr == CLASS_E_NOAGGREGATION, "Unexpected hr %#lx.\n", hr);

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

static void test_source_reader(const char *filename, bool video)
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

    winetest_push_context("%s", filename);

    stream = get_resource_stream(filename);

    /* Create the source reader with video processing enabled. This allows
     * outputting RGB formats. */
    MFCreateAttributes(&attributes, 1);
    hr = IMFAttributes_SetUINT32(attributes, &MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateSourceReaderFromByteStream(stream, attributes, &reader);
    if (FAILED(hr))
    {
        skip("MFCreateSourceReaderFromByteStream() failed, is G-Streamer missing?\n");
        IMFByteStream_Release(stream);
        IMFAttributes_Release(attributes);
        winetest_pop_context();
        return;
    }
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFAttributes_Release(attributes);

    /* Access underlying media source object. */
    hr = IMFSourceReader_GetServiceForStream(reader, MF_SOURCE_READER_MEDIASOURCE, &GUID_NULL, &IID_IMFMediaSource,
            (void **)&source);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaSource_Release(source);

    /* Stream selection. */
    selected = FALSE;
    hr = IMFSourceReader_GetStreamSelection(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, &selected);
    if (video)
    {
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(selected, "Unexpected selection.\n");
    }
    else
    {
        ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);
    }

    hr = IMFSourceReader_GetStreamSelection(reader, 100, &selected);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    selected = FALSE;
    hr = IMFSourceReader_GetStreamSelection(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, &selected);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(selected, "Unexpected selection.\n");

    selected = FALSE;
    hr = IMFSourceReader_GetStreamSelection(reader, 0, &selected);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(selected, "Unexpected selection.\n");

    hr = IMFSourceReader_SetStreamSelection(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);
    ok(hr == (video ? S_OK : MF_E_INVALIDSTREAMNUMBER), "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_SetStreamSelection(reader, 100, TRUE);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_SetStreamSelection(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    selected = TRUE;
    hr = IMFSourceReader_GetStreamSelection(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, &selected);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!selected, "Unexpected selection.\n");

    hr = IMFSourceReader_SetStreamSelection(reader, MF_SOURCE_READER_ALL_STREAMS, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    selected = FALSE;
    hr = IMFSourceReader_GetStreamSelection(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, &selected);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(selected, "Unexpected selection.\n");

    /* Native media type. */
    hr = IMFSourceReader_GetNativeMediaType(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &mediatype);
    if (video)
    {
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IMFSourceReader_GetNativeMediaType(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &mediatype2);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(mediatype != mediatype2, "Unexpected media type instance.\n");
        IMFMediaType_Release(mediatype2);
        IMFMediaType_Release(mediatype);
    }
    else
    {
        ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);
    }

    hr = IMFSourceReader_GetNativeMediaType(reader, 100, 0, &mediatype);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_GetNativeMediaType(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &mediatype);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFSourceReader_GetNativeMediaType(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &mediatype2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(mediatype != mediatype2, "Unexpected media type instance.\n");
    IMFMediaType_Release(mediatype2);
    IMFMediaType_Release(mediatype);

    /* MF_SOURCE_READER_CURRENT_TYPE_INDEX is Win8+ */
    hr = IMFSourceReader_GetNativeMediaType(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM,
            MF_SOURCE_READER_CURRENT_TYPE_INDEX, &mediatype);
    ok(hr == S_OK || broken(hr == MF_E_NO_MORE_TYPES), "Unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
        IMFMediaType_Release(mediatype);

    /* Current media type. */
    hr = IMFSourceReader_GetCurrentMediaType(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, &mediatype);
    if (video)
    {
        GUID subtype;
        UINT32 stride;
        UINT64 framesize;

        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFMediaType_GetGUID(mediatype, &MF_MT_SUBTYPE, &subtype);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        todo_wine ok(IsEqualGUID(&subtype, &MFVideoFormat_H264), "Got subtype %s.\n", debugstr_guid(&subtype));

        hr = IMFMediaType_GetUINT64(mediatype, &MF_MT_FRAME_SIZE, &framesize);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(framesize == ((UINT64)160 << 32 | 120), "Got frame size %ux%u.\n",
                (unsigned int)(framesize >> 32), (unsigned int)framesize);

        hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_DEFAULT_STRIDE, &stride);
        todo_wine ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

        IMFMediaType_Release(mediatype);

        /* Set the type to a YUV format. */

        hr = MFCreateMediaType(&mediatype);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IMFMediaType_SetGUID(mediatype, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IMFMediaType_SetGUID(mediatype, &MF_MT_SUBTYPE, &MFVideoFormat_NV12);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IMFSourceReader_SetCurrentMediaType(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, mediatype);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        IMFMediaType_Release(mediatype);

        hr = IMFSourceReader_GetCurrentMediaType(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, &mediatype);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFMediaType_GetGUID(mediatype, &MF_MT_SUBTYPE, &subtype);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(IsEqualGUID(&subtype, &MFVideoFormat_NV12), "Got subtype %s.\n", debugstr_guid(&subtype));

        hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_DEFAULT_STRIDE, &stride);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(stride == 160, "Got stride %u.\n", stride);

        IMFMediaType_Release(mediatype);

        /* Set the type to an RGB format. */

        hr = MFCreateMediaType(&mediatype);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IMFMediaType_SetGUID(mediatype, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IMFMediaType_SetGUID(mediatype, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IMFSourceReader_SetCurrentMediaType(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, mediatype);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        IMFMediaType_Release(mediatype);

        if (hr == S_OK)
        {
            hr = IMFSourceReader_GetCurrentMediaType(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, &mediatype);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            hr = IMFMediaType_GetGUID(mediatype, &MF_MT_SUBTYPE, &subtype);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(IsEqualGUID(&subtype, &MFVideoFormat_RGB32), "Got subtype %s.\n", debugstr_guid(&subtype));

            hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_DEFAULT_STRIDE, &stride);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            todo_wine ok(stride == 160 * 4, "Got stride %u.\n", stride);

            IMFMediaType_Release(mediatype);
        }
    }
    else
    {
        ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);
    }

    hr = IMFSourceReader_GetCurrentMediaType(reader, 100, &mediatype);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_GetCurrentMediaType(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, &mediatype);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(mediatype);

    hr = IMFSourceReader_GetCurrentMediaType(reader, 0, &mediatype);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(mediatype);

    for (;;)
    {
        hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &actual_index, &stream_flags,
                &timestamp, &sample);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        todo_wine_if (video) ok(!actual_index, "Unexpected stream index %lu.\n", actual_index);
        ok(!(stream_flags & ~MF_SOURCE_READERF_ENDOFSTREAM), "Unexpected stream flags %#lx.\n", stream_flags);

        if (stream_flags & MF_SOURCE_READERF_ENDOFSTREAM)
        {
            ok(!sample, "Unexpected sample object.\n");
            break;
        }

        IMFSample_Release(sample);
    }

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &actual_index, &stream_flags,
            &timestamp, &sample);
    if (video)
    {
        for (;;)
        {
            hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                    0, &actual_index, &stream_flags, &timestamp, &sample);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            todo_wine ok(actual_index == 1, "Unexpected stream index %lu.\n", actual_index);
            ok(!(stream_flags & ~MF_SOURCE_READERF_ENDOFSTREAM), "Unexpected stream flags %#lx.\n", stream_flags);

            if (stream_flags & MF_SOURCE_READERF_ENDOFSTREAM)
            {
                ok(!sample, "Unexpected sample object.\n");
                break;
            }

            IMFSample_Release(sample);
        }
    }
    else
    {
        hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                0, &actual_index, &stream_flags, &timestamp, &sample);
        ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);
        ok(actual_index == MF_SOURCE_READER_FIRST_VIDEO_STREAM, "Unexpected stream index %lu.\n", actual_index);
        ok(stream_flags == MF_SOURCE_READERF_ERROR, "Unexpected stream flags %#lx.\n", stream_flags);

        hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                0, NULL, &stream_flags, &timestamp, &sample);
        ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);
    }

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, NULL, &timestamp, &sample);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, MF_SOURCE_READER_CONTROLF_DRAIN,
            &actual_index, &stream_flags, &timestamp, &sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine_if (video) ok(actual_index == 0, "Unexpected stream index %lu.\n", actual_index);
    ok(stream_flags == MF_SOURCE_READERF_ENDOFSTREAM, "Unexpected stream flags %#lx.\n", stream_flags);
    ok(!sample, "Unexpected sample object.\n");

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, MF_SOURCE_READER_CONTROLF_DRAIN,
            &actual_index, &stream_flags, &timestamp, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, MF_SOURCE_READER_CONTROLF_DRAIN,
            &actual_index, NULL, &timestamp, &sample);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, MF_SOURCE_READER_CONTROLF_DRAIN,
            NULL, &stream_flags, &timestamp, &sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(stream_flags == MF_SOURCE_READERF_ENDOFSTREAM, "Unexpected stream flags %#lx.\n", stream_flags);
    ok(!sample, "Unexpected sample object.\n");

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, MF_SOURCE_READER_CONTROLF_DRAIN,
            &actual_index, &stream_flags, NULL, &sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine_if (video) ok(!actual_index, "Unexpected stream index %lu.\n", actual_index);
    ok(stream_flags == MF_SOURCE_READERF_ENDOFSTREAM, "Unexpected stream flags %#lx.\n", stream_flags);
    ok(!sample, "Unexpected sample object.\n");

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_ANY_STREAM, MF_SOURCE_READER_CONTROLF_DRAIN,
            &actual_index, &stream_flags, NULL, &sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(actual_index == 0, "Unexpected stream index %lu.\n", actual_index);
    ok(stream_flags == MF_SOURCE_READERF_ENDOFSTREAM, "Unexpected stream flags %#lx.\n", stream_flags);
    ok(!sample, "Unexpected sample object.\n");

    /* Flush. */
    hr = IMFSourceReader_Flush(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM);
    ok(hr == (video ? S_OK : MF_E_INVALIDSTREAMNUMBER), "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_Flush(reader, 100);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_Flush(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_Flush(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_Flush(reader, MF_SOURCE_READER_ALL_STREAMS);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    refcount = IMFSourceReader_Release(reader);
    ok(!refcount, "Unexpected refcount %lu.\n", refcount);

    /* Async mode. */
    callback = create_async_callback();

    hr = MFCreateAttributes(&attributes, 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFAttributes_SetUnknown(attributes, &MF_SOURCE_READER_ASYNC_CALLBACK,
            (IUnknown *)&callback->IMFSourceReaderCallback_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFSourceReaderCallback_Release(&callback->IMFSourceReaderCallback_iface);

    refcount = get_refcount(attributes);
    hr = MFCreateSourceReaderFromByteStream(stream, attributes, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(get_refcount(attributes) > refcount, "Unexpected refcount.\n");
    IMFAttributes_Release(attributes);
    IMFSourceReader_Release(reader);

    IMFByteStream_Release(stream);

    winetest_pop_context();
}

static void test_source_reader_from_media_source(void)
{
    static const DWORD expected_sample_order[10] = {0, 0, 1, 1, 0, 0, 0, 0, 1, 0};
    static const struct attribute_desc audio_stream_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32),
        {0},
    };

    IMFStreamDescriptor *audio_streams[3];
    struct async_callback *callback;
    IMFSourceReader *reader;
    IMFMediaSource *source;
    struct test_source *test_source;
    IMFMediaType *media_type;
    HRESULT hr;
    DWORD actual_index, stream_flags;
    IMFSample *sample;
    LONGLONG timestamp;
    IMFAttributes *attributes;
    ULONG refcount;
    int i;
    PROPVARIANT pos;

    for (i = 0; i < ARRAY_SIZE(audio_streams); i++)
    {
        hr = MFCreateMediaType(&media_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        init_media_type(media_type, audio_stream_type_desc, -1);

        hr = MFCreateStreamDescriptor(i, 1, &media_type, &audio_streams[i]);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        IMFMediaType_Release(media_type);
    }

    source = create_test_source(audio_streams, 3);
    ok(!!source, "Failed to create test source.\n");

    hr = MFCreateSourceReaderFromMediaSource(source, NULL, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* MF_SOURCE_READER_ANY_STREAM */
    hr = IMFSourceReader_SetStreamSelection(reader, 0, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_SetStreamSelection(reader, 1, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    pos.vt = VT_I8;
    pos.hVal.QuadPart = 0;
    hr = IMFSourceReader_SetCurrentPosition(reader, &GUID_NULL, &pos);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_ANY_STREAM, 0, &actual_index, &stream_flags,
            &timestamp, &sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(actual_index == 1, "Unexpected stream index %lu\n", actual_index);
    ok(!stream_flags, "Unexpected stream flags %#lx.\n", stream_flags);
    ok(timestamp == 123, "Unexpected timestamp.\n");
    ok(!!sample, "Expected sample object.\n");
    IMFSample_Release(sample);

    hr = IMFSourceReader_SetStreamSelection(reader, 0, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_SetStreamSelection(reader, 2, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < TEST_SOURCE_NUM_STREAMS + 1; ++i)
    {
        hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_ANY_STREAM, 0, &actual_index, &stream_flags,
                &timestamp, &sample);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(actual_index == i % TEST_SOURCE_NUM_STREAMS, "%d: Unexpected stream index %lu.\n", i, actual_index);
        ok(!stream_flags, "Unexpected stream flags %#lx.\n", stream_flags);
        ok(timestamp == 123, "Unexpected timestamp.\n");
        ok(!!sample, "Expected sample object.\n");
        IMFSample_Release(sample);
    }

    hr = IMFSourceReader_SetStreamSelection(reader, 0, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_SetStreamSelection(reader, 0, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < TEST_SOURCE_NUM_STREAMS + 1; ++i)
    {
        hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_ANY_STREAM, 0, &actual_index, &stream_flags,
                &timestamp, &sample);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(actual_index == i % TEST_SOURCE_NUM_STREAMS, "%d: Unexpected stream index %lu.\n", i, actual_index);
        ok(!stream_flags, "Unexpected stream flags %#lx.\n", stream_flags);
        ok(timestamp == 123, "Unexpected timestamp.\n");
        ok(!!sample, "Expected sample object.\n");
        IMFSample_Release(sample);
    }

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_ANY_STREAM, 0, &actual_index, &stream_flags,
            &timestamp, &sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!stream_flags, "Unexpected stream flags %#lx.\n", stream_flags);
    ok(timestamp == 123, "Unexpected timestamp.\n");
    ok(!!sample, "Expected sample object.\n");

    /* Once the last read sample of all streams has the same timestamp value, the reader will
       continue reading from the first stream until its timestamp increases. */
    ok(!actual_index, "%d: Unexpected stream index %lu.\n", TEST_SOURCE_NUM_STREAMS + 1, actual_index);

    IMFSample_Release(sample);

    IMFSourceReader_Release(reader);
    IMFMediaSource_Release(source);

    source = create_test_source(audio_streams, 1);
    ok(!!source, "Failed to create test source.\n");

    hr = MFCreateSourceReaderFromMediaSource(source, NULL, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* MF_SOURCE_READER_ANY_STREAM with a single stream */
    hr = IMFSourceReader_SetStreamSelection(reader, 0, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    pos.vt = VT_I8;
    pos.hVal.QuadPart = 0;
    hr = IMFSourceReader_SetCurrentPosition(reader, &GUID_NULL, &pos);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_ANY_STREAM, 0, &actual_index, &stream_flags,
            &timestamp, &sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!actual_index, "Unexpected stream index %lu.\n", actual_index);
    ok(!stream_flags, "Unexpected stream flags %#lx.\n", stream_flags);
    ok(timestamp == 123, "Unexpected timestamp.\n");
    ok(!!sample, "Expected sample object.\n");
    IMFSample_Release(sample);

    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_ANY_STREAM, 0, &actual_index, &stream_flags,
            &timestamp, &sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!actual_index, "Unexpected stream index %lu\n", actual_index);
    ok(!stream_flags, "Unexpected stream flags %#lx.\n", stream_flags);
    ok(timestamp == 123, "Unexpected timestamp.\n");
    ok(!!sample, "Expected sample object.\n");
    IMFSample_Release(sample);

    IMFSourceReader_Release(reader);
    IMFMediaSource_Release(source);

    /* Request from stream 0. */
    source = create_test_source(audio_streams, 3);
    ok(!!source, "Failed to create test source.\n");

    hr = MFCreateSourceReaderFromMediaSource(source, NULL, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_SetStreamSelection(reader, 0, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, 0, 0, &actual_index, &stream_flags, &timestamp, &sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!actual_index, "Unexpected stream index %lu.\n", actual_index);
    ok(!stream_flags, "Unexpected stream flags %#lx.\n", stream_flags);
    ok(timestamp == 123, "Unexpected timestamp.\n");
    ok(!!sample, "Expected sample object.\n");
    IMFSample_Release(sample);

    /* Request from deselected stream. */
    hr = IMFSourceReader_SetStreamSelection(reader, 1, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    actual_index = 0;
    stream_flags = 0;
    hr = IMFSourceReader_ReadSample(reader, 1, 0, &actual_index, &stream_flags, &timestamp, &sample);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);
    ok(actual_index == 1, "Unexpected stream index %lu.\n", actual_index);
    ok(stream_flags == MF_SOURCE_READERF_ERROR, "Unexpected stream flags %#lx.\n", stream_flags);
    ok(timestamp == 0, "Unexpected timestamp.\n");
    ok(!sample, "Expected sample object.\n");

    IMFSourceReader_Release(reader);
    IMFMediaSource_Release(source);

    /* Request a non-native bit depth. */
    source = create_test_source(audio_streams, 1);
    ok(!!source, "Failed to create test source.\n");

    hr = MFCreateSourceReaderFromMediaSource(source, NULL, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_SetCurrentMediaType(reader, 0, NULL, media_type);
    ok(hr == MF_E_TOPO_CODEC_NOT_FOUND, "Unexpected hr %#lx.\n", hr);

    IMFMediaType_Release(media_type);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_BITS_PER_SAMPLE, 32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_SetCurrentMediaType(reader, 0, NULL, media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_SetStreamSelection(reader, 0, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, 0, 0, &actual_index, &stream_flags, &timestamp, &sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!actual_index, "Unexpected stream index %lu\n", actual_index);
    ok(!stream_flags, "Unexpected stream flags %#lx.\n", stream_flags);
    ok(timestamp == 123, "Unexpected timestamp.\n");
    ok(!!sample, "Expected sample object.\n");
    IMFSample_Release(sample);

    IMFMediaType_Release(media_type);
    IMFSourceReader_Release(reader);
    IMFMediaSource_Release(source);

    /* Async mode. */
    source = create_test_source(audio_streams, 3);
    ok(!!source, "Failed to create test source.\n");

    callback = create_async_callback();

    hr = MFCreateAttributes(&attributes, 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFAttributes_SetUnknown(attributes, &MF_SOURCE_READER_ASYNC_CALLBACK,
            (IUnknown *)&callback->IMFSourceReaderCallback_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFSourceReaderCallback_Release(&callback->IMFSourceReaderCallback_iface);

    refcount = get_refcount(attributes);
    hr = MFCreateSourceReaderFromMediaSource(source, attributes, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(get_refcount(attributes) > refcount, "Unexpected refcount.\n");

    hr = IMFSourceReader_SetStreamSelection(reader, 0, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Return values are delivered to callback only. */
    hr = IMFSourceReader_ReadSample(reader, 0, 0, &actual_index, &stream_flags, &timestamp, &sample);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, 0, 0, NULL, &stream_flags, &timestamp, &sample);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, 0, 0, NULL, NULL, &timestamp, &sample);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, 0, 0, NULL, NULL, NULL, &sample);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* Flush() arguments validation. */
    hr = IMFSourceReader_Flush(reader, 123);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, 0, 0, NULL, NULL, NULL, NULL);
    ok(hr == MF_E_NOTACCEPTING, "Unexpected hr %#lx.\n", hr);

    IMFSourceReader_Release(reader);
    IMFMediaSource_Release(source);

    /* RequestSample failure. */
    source = create_test_source(audio_streams, 3);
    ok(!!source, "Failed to create test source.\n");

    fail_request_sample = TRUE;

    hr = MFCreateSourceReaderFromMediaSource(source, NULL, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_SetStreamSelection(reader, 0, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_ReadSample(reader, 0, 0, &actual_index, &stream_flags, &timestamp, &sample);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    actual_index = ~0u;
    stream_flags = 0;
    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &actual_index, &stream_flags,
            &timestamp, &sample);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
    ok(!actual_index, "Unexpected index %lu.\n", actual_index);
    ok(stream_flags == MF_SOURCE_READERF_ERROR, "Unexpected flags %#lx.\n", stream_flags);

    actual_index = ~0u;
    stream_flags = 0;
    hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &actual_index, &stream_flags,
            &timestamp, &sample);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
    ok(!actual_index, "Unexpected index %lu.\n", actual_index);
    ok(stream_flags == MF_SOURCE_READERF_ERROR, "Unexpected flags %#lx.\n", stream_flags);

    actual_index = ~0u;
    stream_flags = 0;
    hr = IMFSourceReader_ReadSample(reader, 0, 0, &actual_index, &stream_flags, &timestamp, &sample);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
    ok(!actual_index, "Unexpected index %lu.\n", actual_index);
    ok(stream_flags == MF_SOURCE_READERF_ERROR, "Unexpected flags %#lx.\n", stream_flags);

    IMFSourceReader_Release(reader);
    IMFMediaSource_Release(source);

    fail_request_sample = FALSE;

    /* MF_SOURCE_READER_ANY_STREAM with streams of different sample sizes */
    source = create_test_source(audio_streams, 2);
    ok(!!source, "Failed to create test source.\n");

    test_source = impl_from_IMFMediaSource(source);
    test_source->streams[0]->sample_duration = 100000;
    test_source->streams[1]->sample_duration = 400000;

    hr = MFCreateSourceReaderFromMediaSource(source, NULL, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_SetStreamSelection(reader, 0, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_SetStreamSelection(reader, 1, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* The source reader picks the stream whose last sample had the lower timestamp */
    for (i = 0; i < ARRAY_SIZE(expected_sample_order); i++)
    {
        hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_ANY_STREAM, 0, &actual_index, &stream_flags, &timestamp, &sample);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!stream_flags, "Unexpected stream flags %#lx.\n", stream_flags);
        ok(!!sample, "Expected sample object.\n");
        ok (actual_index == expected_sample_order[i], "Got sample %u from unexpected stream %lu, expected %lu.\n",
                i, actual_index, expected_sample_order[i]);
        IMFSample_Release(sample);
    }

    IMFSourceReader_Release(reader);
    IMFMediaSource_Release(source);

    for (i = 0; i < ARRAY_SIZE(audio_streams); i++)
        IMFStreamDescriptor_Release(audio_streams[i]);
}

static void test_reader_d3d9(void)
{
    static const struct attribute_desc audio_stream_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32),
        {0},
    };

    IMFStreamDescriptor *audio_streams[3];
    IDirect3DDeviceManager9 *d3d9_manager;
    IDirect3DDevice9 *d3d9_device;
    IMFAttributes *attributes;
    IMFMediaType *media_type;
    IMFSourceReader *reader;
    IMFMediaSource *source;
    IDirect3D9 *d3d9;
    HWND window;
    HRESULT hr;
    UINT i, token;
    ULONG refcount;

    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d9)
    {
        skip("Failed to create a D3D9 object, skipping tests.\n");
        return;
    }
    window = create_window();
    if (!(d3d9_device = create_d3d9_device(d3d9, window)))
    {
        skip("Failed to create a D3D9 device, skipping tests.\n");
        goto done;
    }

    for (i = 0; i < ARRAY_SIZE(audio_streams); i++)
    {
        hr = MFCreateMediaType(&media_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        init_media_type(media_type, audio_stream_type_desc, -1);

        hr = MFCreateStreamDescriptor(i, 1, &media_type, &audio_streams[i]);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        IMFMediaType_Release(media_type);
    }

    hr = DXVA2CreateDirect3DDeviceManager9(&token, &d3d9_manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_ResetDevice(d3d9_manager, d3d9_device, token);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    source = create_test_source(audio_streams, 3);
    ok(!!source, "Failed to create test source.\n");

    hr = MFCreateAttributes(&attributes, 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFAttributes_SetUnknown(attributes, &MF_SOURCE_READER_D3D_MANAGER, (IUnknown *)d3d9_manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateSourceReaderFromMediaSource(source, attributes, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFAttributes_Release(attributes);

    IMFSourceReader_Release(reader);

    for (i = 0; i < ARRAY_SIZE(audio_streams); i++)
        IMFStreamDescriptor_Release(audio_streams[i]);

    refcount = IDirect3DDeviceManager9_Release(d3d9_manager);
    ok(!refcount, "Unexpected refcount %lu.\n", refcount);

    IDirect3DDevice9_Release(d3d9_device);
done:
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_sink_writer_create(void)
{
    WCHAR temp_path[MAX_PATH], temp_file[MAX_PATH];
    IMFSinkWriter *writer;
    IMFByteStream *stream;
    IMFAttributes *attr;
    HRESULT hr;

    GetTempPathW(ARRAY_SIZE(temp_path), temp_path);
    GetTempFileNameW(temp_path, L"mf", 0, temp_file);
    wcscat(temp_file, L".mp4");

    hr = MFCreateAttributes(&attr, 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_SetGUID(attr, &MF_TRANSCODE_CONTAINERTYPE, &MFTranscodeContainerType_MPEG4);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateTempFile(MF_ACCESSMODE_READWRITE, MF_OPENMODE_DELETE_IF_EXIST, MF_FILEFLAGS_NONE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* MFCreateSinkWriterFromURL. */
    hr = MFCreateSinkWriterFromURL(NULL, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    writer = (void *)0xdeadbeef;
    hr = MFCreateSinkWriterFromURL(NULL, NULL, NULL, &writer);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(!writer, "Unexpected pointer %p.\n", writer);

    writer = (void *)0xdeadbeef;
    hr = MFCreateSinkWriterFromURL(NULL, NULL, attr, &writer);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(!writer, "Unexpected pointer %p.\n", writer);

    writer = (void *)0xdeadbeef;
    hr = MFCreateSinkWriterFromURL(NULL, stream, NULL, &writer);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(!writer, "Unexpected pointer %p.\n", writer);

    hr = MFCreateSinkWriterFromURL(NULL, stream, attr, &writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFSinkWriter_Release(writer);

    hr = MFCreateSinkWriterFromURL(temp_file, NULL, NULL, &writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFSinkWriter_Release(writer);

    hr = MFCreateSinkWriterFromURL(temp_file, NULL, attr, &writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFSinkWriter_Release(writer);

    hr = MFCreateSinkWriterFromURL(temp_file, stream, NULL, &writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFSinkWriter_Release(writer);

    hr = MFCreateSinkWriterFromURL(temp_file, stream, attr, &writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFSinkWriter_Release(writer);

    /* MFCreateSinkWriterFromMediaSink. */
    hr = MFCreateSinkWriterFromMediaSink(NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    writer = (void *)0xdeadbeef;
    hr = MFCreateSinkWriterFromMediaSink(NULL, NULL, &writer);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(!writer, "Unexpected pointer %p.\n", writer);

    IMFByteStream_Release(stream);
    IMFAttributes_Release(attr);
}

static void test_sink_writer_get_object(void)
{
    static const struct attribute_desc video_stream_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_H264),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001),
        ATTR_UINT32(MF_MT_AVG_BITRATE, 193540),
        ATTR_UINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive),
        {0},
    };
    static const struct attribute_desc video_input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001),
        {0},
    };
    static const struct attribute_desc video_input_type_nv12_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001),
        {0},
    };
    WCHAR temp_path[MAX_PATH], temp_file[MAX_PATH];
    IMFMediaType *stream_type, *input_type;
    IMFSinkWriterEx *writer_ex = NULL;
    IMFTransform *transform;
    IMFSinkWriter *writer;
    IMFMediaSink *sink;
    DWORD index;
    HRESULT hr;
    GUID guid;

    GetTempPathW(ARRAY_SIZE(temp_path), temp_path);
    GetTempFileNameW(temp_path, L"mf", 0, temp_file);
    wcscat(temp_file, L".mp4");
    hr = MFCreateSinkWriterFromURL(temp_file, NULL, NULL, &writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSinkWriter_QueryInterface(writer, &IID_IMFSinkWriterEx, (void **)&writer_ex);
    ok(hr == S_OK, "QueryInterface returned %#lx.\n", hr);

    hr = MFCreateMediaType(&stream_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_media_type(stream_type, video_stream_type_desc, -1);
    hr = IMFSinkWriter_AddStream(writer, stream_type, &index);
    ok(hr == S_OK, "AddStream returned %#lx.\n", hr);
    IMFMediaType_Release(stream_type);

    /* Get transform before SetInputMediaType. */
    transform = (void *)0xdeadbeef;
    hr = IMFSinkWriter_GetServiceForStream(writer, 0, &GUID_NULL, &IID_IMFTransform, (void **)&transform);
    todo_wine
    ok(hr == MF_E_UNSUPPORTED_SERVICE, "GetServiceForStream returned %#lx.\n", hr);
    ok(!transform, "Unexpected transform %p.\n", transform);

    transform = (void *)0xdeadbeef;
    hr = IMFSinkWriterEx_GetTransformForStream(writer_ex, 0, 0, &guid, &transform);
    ok(hr == MF_E_INVALIDINDEX, "GetTransformForStream returned %#lx.\n", hr);
    ok(!transform, "Unexpected transform %p.\n", transform);

    hr = MFCreateMediaType(&input_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_media_type(input_type, video_input_type_desc, -1);
    hr = IMFSinkWriter_SetInputMediaType(writer, 0, input_type, NULL);
    todo_wine
    ok(hr == S_OK, "SetInputMediaType returned %#lx.\n", hr);
    IMFMediaType_Release(input_type);

    /* Get transform after SetInputMediaType. */
    hr = IMFSinkWriter_GetServiceForStream(writer, 0, &GUID_NULL, &IID_IMFTransform, (void **)&transform);
    todo_wine
    ok(hr == S_OK, "GetServiceForStream returned %#lx.\n", hr);
    if (hr == S_OK)
    IMFTransform_Release(transform);

    hr = IMFSinkWriterEx_GetTransformForStream(writer_ex, 0, 0, &guid, &transform);
    todo_wine
    ok(hr == S_OK, "GetTransformForStream returned %#lx.\n", hr);
    todo_wine
    ok(IsEqualGUID(&guid, &MFT_CATEGORY_VIDEO_PROCESSOR), "Unexpected guid %s.\n", debugstr_guid(&guid));
    if (hr == S_OK)
    IMFTransform_Release(transform);

    hr = IMFSinkWriterEx_GetTransformForStream(writer_ex, 0, 1, &guid, &transform);
    todo_wine
    ok(hr == S_OK, "GetTransformForStream returned %#lx.\n", hr);
    todo_wine
    ok(IsEqualGUID(&guid, &MFT_CATEGORY_VIDEO_ENCODER), "Unexpected guid %s.\n", debugstr_guid(&guid));
    if (hr == S_OK)
    IMFTransform_Release(transform);

    transform = (void *)0xdeadbeef;
    hr = IMFSinkWriterEx_GetTransformForStream(writer_ex, 0, 2, &guid, &transform);
    ok(hr == MF_E_INVALIDINDEX, "GetTransformForStream returned %#lx.\n", hr);
    ok(!transform, "Unexpected transform %p.\n", transform);

    /* Get transform for writer without converter. */
    hr = MFCreateMediaType(&input_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_media_type(input_type, video_input_type_nv12_desc, -1);
    hr = IMFSinkWriter_SetInputMediaType(writer, 0, input_type, NULL);
    todo_wine
    ok(hr == S_OK, "SetInputMediaType returned %#lx.\n", hr);
    IMFMediaType_Release(input_type);

    hr = IMFSinkWriterEx_GetTransformForStream(writer_ex, 0, 0, &guid, &transform);
    todo_wine
    ok(hr == S_OK, "GetTransformForStream returned %#lx.\n", hr);
    todo_wine
    ok(IsEqualGUID(&guid, &MFT_CATEGORY_VIDEO_ENCODER), "Unexpected guid %s.\n", debugstr_guid(&guid));
    if (hr == S_OK)
        IMFTransform_Release(transform);

    hr = IMFSinkWriterEx_GetTransformForStream(writer_ex, 0, 1, &guid, &transform);
    ok(hr == MF_E_INVALIDINDEX, "GetTransformForStream returned %#lx.\n", hr);

    /* Get media sink before BeginWriting. */
    sink = (void *)0xdeadbeef;
    hr = IMFSinkWriter_GetServiceForStream(writer, MF_SINK_WRITER_MEDIASINK,
            &GUID_NULL, &IID_IMFMediaSink, (void **)&sink);
    todo_wine
    ok(hr == MF_E_UNSUPPORTED_SERVICE, "GetServiceForStream returned %#lx.\n", hr);
    todo_wine
    ok(!sink, "Unexpected sink %p.\n", sink);

    hr = IMFSinkWriter_BeginWriting(writer);
    todo_wine
    ok(hr == S_OK, "BeginWriting returned %#lx.\n", hr);

    /* Get media sink after BeginWriting. */
    hr = IMFSinkWriter_GetServiceForStream(writer, MF_SINK_WRITER_MEDIASINK,
            &GUID_NULL, &IID_IMFMediaSink, (void **)&sink);
    ok(hr == S_OK, "GetServiceForStream returned %#lx.\n", hr);
    IMFMediaSink_Release(sink);

    if (writer_ex)
    IMFSinkWriterEx_Release(writer_ex);
    IMFSinkWriter_Release(writer);
    DeleteFileW(temp_file);
}

static void test_sink_writer_add_stream(void)
{
    static const struct attribute_desc video_stream_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_H264),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001),
        ATTR_UINT32(MF_MT_AVG_BITRATE, 193540),
        ATTR_UINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive),
        {0},
    };
    static const struct attribute_desc video_input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001),
        {0},
    };
    WCHAR temp_path[MAX_PATH], temp_file[MAX_PATH];
    IMFMediaType *stream_type, *input_type;
    IMFSinkWriter *writer;
    DWORD index;
    HRESULT hr;

    GetTempPathW(ARRAY_SIZE(temp_path), temp_path);
    GetTempFileNameW(temp_path, L"mf", 0, temp_file);
    wcscat(temp_file, L".mp4");
    hr = MFCreateSinkWriterFromURL(temp_file, NULL, NULL, &writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Test AddStream. */
    hr = MFCreateMediaType(&stream_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_media_type(stream_type, video_stream_type_desc, -1);

    hr = IMFSinkWriter_AddStream(writer, NULL, NULL);
    ok(hr == E_INVALIDARG, "AddStream returned %#lx.\n", hr);

    hr = IMFSinkWriter_AddStream(writer, stream_type, NULL);
    ok(hr == E_POINTER, "AddStream returned %#lx.\n", hr);

    index = 0xdeadbeef;
    hr = IMFSinkWriter_AddStream(writer, NULL, &index);
    ok(hr == E_INVALIDARG, "AddStream returned %#lx.\n", hr);
    ok(index == 0xdeadbeef, "Unexpected index %lu.\n", index);

    hr = IMFSinkWriter_AddStream(writer, stream_type, &index);
    ok(hr == S_OK, "AddStream returned %#lx.\n", hr);
    ok(index == 0, "Unexpected index %lu.\n", index);

    IMFMediaType_Release(stream_type);

    /* Test SetInputMediaType. */
    hr = MFCreateMediaType(&input_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_media_type(input_type, video_input_type_desc, -1);

    hr = IMFSinkWriter_SetInputMediaType(writer, 0xdeadbeef, NULL, NULL);
    ok(hr == E_INVALIDARG, "SetInputMediaType returned %#lx.\n", hr);

    hr = IMFSinkWriter_SetInputMediaType(writer, 0, NULL, NULL);
    ok(hr == E_INVALIDARG, "SetInputMediaType returned %#lx.\n", hr);

    hr = IMFSinkWriter_SetInputMediaType(writer, 0xdeadbeef, input_type, NULL);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "SetInputMediaType returned %#lx.\n", hr);

    hr = IMFSinkWriter_SetInputMediaType(writer, 0, input_type, NULL);
    todo_wine
    ok(hr == S_OK, "SetInputMediaType returned %#lx.\n", hr);

    IMFMediaType_Release(input_type);

    IMFSinkWriter_Release(writer);
    DeleteFileW(temp_file);
}


static void test_sink_writer_sample_process(void)
{
    static const struct attribute_desc video_stream_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_H264),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001),
        ATTR_UINT32(MF_MT_AVG_BITRATE, 193540),
        ATTR_UINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive),
        {0},
    };
    static const struct attribute_desc video_input_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001),
        {0},
    };
    WCHAR temp_path[MAX_PATH], temp_file[MAX_PATH];
    IMFMediaType *stream_type, *input_type;
    DWORD rgb32_data[96 * 96], i, index;
    LARGE_INTEGER file_size;
    IMFSinkWriter *writer;
    HANDLE file;
    HRESULT hr;
    BOOL ret;

    GetTempPathW(ARRAY_SIZE(temp_path), temp_path);
    GetTempFileNameW(temp_path, L"mf", 0, temp_file);
    wcscat(temp_file, L".mp4");
    hr = MFCreateSinkWriterFromURL(temp_file, NULL, NULL, &writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* BeginWriting fails before adding stream. */
    hr = IMFSinkWriter_BeginWriting(writer);
    ok(hr == MF_E_INVALIDREQUEST, "BeginWriting returned %#lx.\n", hr);

    hr = MFCreateMediaType(&stream_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_media_type(stream_type, video_stream_type_desc, -1);
    hr = IMFSinkWriter_AddStream(writer, stream_type, &index);
    ok(hr == S_OK, "AddStream returned %#lx.\n", hr);
    IMFMediaType_Release(stream_type);

    hr = MFCreateMediaType(&input_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_media_type(input_type, video_input_type_desc, -1);
    hr = IMFSinkWriter_SetInputMediaType(writer, 0, input_type, NULL);
    todo_wine
    ok(hr == S_OK, "SetInputMediaType returned %#lx.\n", hr);
    IMFMediaType_Release(input_type);

    /* BeginWriting after adding stream. */
    hr = IMFSinkWriter_BeginWriting(writer);
    todo_wine
    ok(hr == S_OK, "BeginWriting returned %#lx.\n", hr);
    hr = IMFSinkWriter_BeginWriting(writer);
    todo_wine
    ok(hr == MF_E_INVALIDREQUEST, "BeginWriting returned %#lx.\n", hr);

    /* WriteSample. */
    for (i = 0; i < ARRAY_SIZE(rgb32_data); ++i)
        rgb32_data[i] = 0x0000ff00;
    for (i = 0; i < 30; ++i)
    {
        IMFSample *sample = create_sample((const BYTE *)rgb32_data, sizeof(rgb32_data));
        hr = IMFSample_SetSampleTime(sample, 333333 * i);
        ok(hr == S_OK, "SetSampleTime returned %#lx.\n", hr);
        hr = IMFSample_SetSampleDuration(sample, 333333);
        ok(hr == S_OK, "SetSampleDuration returned %#lx.\n", hr);
        hr = IMFSinkWriter_WriteSample(writer, 0, sample);
        todo_wine
        ok(hr == S_OK, "WriteSample returned %#lx.\n", hr);
        IMFSample_Release(sample);
    }

    /* Finalize. */
    hr = IMFSinkWriter_Finalize(writer);
    todo_wine
    ok(hr == S_OK, "Finalize returned %#lx.\n", hr);
    hr = IMFSinkWriter_Finalize(writer);
    todo_wine
    ok(hr == MF_E_INVALIDREQUEST, "Finalize returned %#lx.\n", hr);

    /* Check the output file. */
    file = CreateFileW(temp_file, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    todo_wine
    ok(file != INVALID_HANDLE_VALUE, "CreateFileW failed.\n");
    if (file != INVALID_HANDLE_VALUE)
    {
        ret = GetFileSizeEx(file, &file_size);
        ok(ret, "GetFileSizeEx failed.\n");
        ok(file_size.QuadPart > 0x400, "Unexpected file size %I64x.\n", file_size.QuadPart);
        CloseHandle(file);
    }
    IMFSinkWriter_Release(writer);
    DeleteFileW(temp_file);
}

static void test_interfaces(void)
{
    static const struct attribute_desc audio_stream_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32),
        {0},
    };

    IMFStreamDescriptor *audio_streams[1];
    IMFMediaType *media_type;
    IMFSourceReader *reader;
    IMFMediaSource *source;
    IUnknown *unk;
    HRESULT hr;
    UINT i;

    for (i = 0; i < ARRAY_SIZE(audio_streams); i++)
    {
        hr = MFCreateMediaType(&media_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        init_media_type(media_type, audio_stream_type_desc, -1);

        hr = MFCreateStreamDescriptor(i, 1, &media_type, &audio_streams[i]);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        IMFMediaType_Release(media_type);
    }

    source = create_test_source(audio_streams, 1);
    ok(!!source, "Failed to create test source.\n");

    hr = MFCreateSourceReaderFromMediaSource(source, NULL, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_QueryInterface(reader, &IID_IMFSourceReaderEx, (void **)&unk);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* Windows 7 and below.*/, "Unexpected hr %#lx.\n", hr);
    if (unk)
        IUnknown_Release(unk);

    IMFSourceReader_Release(reader);
    IMFMediaSource_Release(source);

    for (i = 0; i < ARRAY_SIZE(audio_streams); i++)
        IMFStreamDescriptor_Release(audio_streams[i]);
}

static void test_source_reader_transforms(BOOL enable_processing, BOOL enable_advanced)
{
    static const struct attribute_desc h264_stream_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_H264),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        {0},
    };
    static const struct attribute_desc nv12_stream_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        {0},
    };
    static const struct attribute_desc nv12_expect_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001),
        ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        ATTR_UINT32(MF_MT_AVG_BIT_ERROR_RATE, 0, .todo = TRUE),
        ATTR_UINT32(MF_MT_COMPRESSED, 0, .todo = TRUE),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, 96),
        ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
        ATTR_UINT32(MF_MT_INTERLACE_MODE, 7),
        ATTR_UINT32(MF_MT_SAMPLE_SIZE, 13824),
        ATTR_UINT32(MF_MT_VIDEO_ROTATION, 0),
        {0},
    };
    static const struct attribute_desc nv12_expect_advanced_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1, .todo = TRUE),
        ATTR_UINT32(MF_MT_COMPRESSED, 0, .todo = TRUE),
        ATTR_UINT32(MF_MT_INTERLACE_MODE, 2, .todo = TRUE),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, 96, .not_present = TRUE),
        {0},
    };
    static const struct attribute_desc yuy2_stream_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YUY2),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        {0},
    };
    static const struct attribute_desc yuy2_expect_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YUY2),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001),
        ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        ATTR_UINT32(MF_MT_AVG_BIT_ERROR_RATE, 0, .todo = TRUE),
        ATTR_UINT32(MF_MT_COMPRESSED, 0, .todo = TRUE),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, 192),
        ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
        ATTR_UINT32(MF_MT_INTERLACE_MODE, 7),
        ATTR_UINT32(MF_MT_SAMPLE_SIZE, 18432),
        ATTR_UINT32(MF_MT_VIDEO_ROTATION, 0),
        {0},
    };
    static const struct attribute_desc yuy2_expect_advanced_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YUY2),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1, .todo = TRUE),
        ATTR_UINT32(MF_MT_COMPRESSED, 0, .todo = TRUE),
        ATTR_UINT32(MF_MT_INTERLACE_MODE, 2, .todo = TRUE),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, 96, .not_present = TRUE),
        {0},
    };
    static const struct attribute_desc rgb32_stream_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        {0},
    };
    static const struct attribute_desc rgb32_expect_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1, .todo = TRUE),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, 384, .todo = TRUE),
        ATTR_UINT32(MF_MT_INTERLACE_MODE, 2, .todo = TRUE),
        ATTR_UINT32(MF_MT_SAMPLE_SIZE, 36864, .todo = TRUE),
        {0},
    };
    static const struct attribute_desc rgb32_expect_advanced_desc_todo1[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1, .todo = TRUE),
        ATTR_UINT32(MF_MT_COMPRESSED, 0, .todo = TRUE),
        ATTR_UINT32(MF_MT_INTERLACE_MODE, 2, .todo = TRUE),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, 96, .not_present = TRUE),
        {0},
    };
    static const struct attribute_desc rgb32_expect_advanced_desc_todo2[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        ATTR_UINT32(MF_MT_COMPRESSED, 0, .todo = TRUE),
        ATTR_UINT32(MF_MT_INTERLACE_MODE, 2, .todo_value = TRUE),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, 96, .not_present = TRUE),
        {0},
    };
    IMFStreamDescriptor *video_stream;
    IMFSourceReaderEx *reader_ex;
    IMFAttributes *attributes;
    IMFMediaType *media_type;
    IMFSourceReader *reader;
    IMFTransform *transform;
    IMFMediaSource *source;
    GUID category;
    HRESULT hr;

    winetest_push_context("vp %u adv %u", enable_processing, enable_advanced);

    hr = MFCreateAttributes(&attributes, 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_SetUINT32(attributes, &MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, enable_processing);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_SetUINT32(attributes, &MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, enable_advanced);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* test source reader with a RGB32 source */

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_media_type(media_type, rgb32_stream_type_desc, -1);
    hr = MFCreateStreamDescriptor(0, 1, &media_type, &video_stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(media_type);

    source = create_test_source(&video_stream, 1);
    ok(!!source, "Failed to create test source.\n");

    hr = MFCreateSourceReaderFromMediaSource(source, attributes, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFSourceReader_SetStreamSelection(reader, 0, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* skip tests on Win7 which misses IMFSourceReaderEx and has uninteresting media types differences */
    hr = IMFSourceReader_QueryInterface(reader, &IID_IMFSourceReaderEx, (void **)&reader_ex);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* Win7 */, "Unexpected hr %#lx.\n", hr);
    if (broken(hr == E_NOINTERFACE))
    {
        win_skip("missing IMFSourceReaderEx interface, skipping tests on Win7\n");
        goto skip_tests;
    }
    IMFSourceReaderEx_Release(reader_ex);

    hr = IMFSourceReader_GetNativeMediaType(reader, 0, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_media_type(media_type, rgb32_stream_type_desc, -1);
    IMFMediaType_Release(media_type);

    hr = IMFSourceReader_GetCurrentMediaType(reader, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_media_type(media_type, rgb32_stream_type_desc, -1);
    IMFMediaType_Release(media_type);

    /* only one stream type and only one stream */
    hr = IMFSourceReader_GetNativeMediaType(reader, 0, 1, &media_type);
    ok(hr == MF_E_NO_MORE_TYPES, "Unexpected hr %#lx.\n", hr);
    hr = IMFSourceReader_GetNativeMediaType(reader, 1, 0, &media_type);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);
    hr = IMFSourceReader_GetCurrentMediaType(reader, 1, &media_type);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    /* cannot request encoding to compressed media type */
    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_media_type(media_type, h264_stream_type_desc, -1);
    hr = IMFSourceReader_SetCurrentMediaType(reader, 0, NULL, media_type);
    ok(hr == MF_E_TOPO_CODEC_NOT_FOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(media_type);

    /* SetCurrentMediaType needs major type and subtype */
    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFSourceReader_SetCurrentMediaType(reader, 0, NULL, media_type);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    init_media_type(media_type, nv12_stream_type_desc, 1);
    hr = IMFSourceReader_SetCurrentMediaType(reader, 0, NULL, media_type);
    todo_wine ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* RGB32 -> NV12 conversion with MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING */
    init_media_type(media_type, nv12_stream_type_desc, 2); /* doesn't need the frame size */
    hr = IMFSourceReader_SetCurrentMediaType(reader, 0, NULL, media_type);
    if (enable_advanced)
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    else
    {
        todo_wine_if(enable_processing) /* Wine enables advanced video processing in all cases */
        ok(hr == MF_E_TOPO_CODEC_NOT_FOUND, "Unexpected hr %#lx.\n", hr);
    }
    IMFMediaType_Release(media_type);

    hr = IMFSourceReader_GetCurrentMediaType(reader, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (enable_advanced)
        check_media_type(media_type, nv12_expect_advanced_desc, -1);
    else if (!enable_processing)
        check_media_type(media_type, rgb32_stream_type_desc, -1);
    else if  (!winetest_platform_is_wine) /* Wine enables advanced video processing in all cases */
        check_media_type(media_type, rgb32_stream_type_desc, -1);
    IMFMediaType_Release(media_type);

    /* video processor is accessible with MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING */
    hr = IMFSourceReader_GetServiceForStream(reader, 0, &GUID_NULL, &IID_IMFTransform, (void **)&transform);
    if (!enable_advanced)
        ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);
    else
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        check_media_type(media_type, rgb32_stream_type_desc, -1);
        IMFMediaType_Release(media_type);

        hr = IMFTransform_GetOutputCurrentType(transform, 0, &media_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        check_media_type(media_type, nv12_stream_type_desc, -1);
        IMFMediaType_Release(media_type);

        IMFTransform_Release(transform);
    }

    IMFSourceReader_Release(reader);
    IMFMediaSource_Release(source);
    IMFStreamDescriptor_Release(video_stream);


    /* test source reader with a NV12 source */

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_media_type(media_type, nv12_stream_type_desc, -1);
    hr = MFCreateStreamDescriptor(0, 1, &media_type, &video_stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(media_type);

    source = create_test_source(&video_stream, 1);
    ok(!!source, "Failed to create test source.\n");

    hr = MFCreateSourceReaderFromMediaSource(source, attributes, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFSourceReader_SetStreamSelection(reader, 0, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_GetNativeMediaType(reader, 0, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_media_type(media_type, nv12_stream_type_desc, -1);
    IMFMediaType_Release(media_type);

    hr = IMFSourceReader_GetCurrentMediaType(reader, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_media_type(media_type, nv12_stream_type_desc, -1);
    IMFMediaType_Release(media_type);

    /* NV12 -> RGB32 conversion with MF_SOURCE_READER_ENABLE_(ADVANCED_)VIDEO_PROCESSING */
    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_media_type(media_type, rgb32_stream_type_desc, 2); /* doesn't need the frame size */
    hr = IMFSourceReader_SetCurrentMediaType(reader, 0, NULL, media_type);
    if (enable_processing || enable_advanced)
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    else
        ok(hr == MF_E_TOPO_CODEC_NOT_FOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(media_type);

    hr = IMFSourceReader_GetCurrentMediaType(reader, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (enable_advanced)
        check_media_type(media_type, rgb32_expect_advanced_desc_todo1, -1);
    else if (enable_processing)
        check_media_type(media_type, rgb32_expect_desc, -1);
    else
        check_media_type(media_type, nv12_stream_type_desc, -1);
    IMFMediaType_Release(media_type);

    /* convert transform is only exposed with MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING */
    hr = IMFSourceReader_GetServiceForStream(reader, 0, &GUID_NULL, &IID_IMFTransform, (void **)&transform);
    if (!enable_advanced)
        ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);
    else
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        check_media_type(media_type, nv12_stream_type_desc, -1);
        IMFMediaType_Release(media_type);

        hr = IMFTransform_GetOutputCurrentType(transform, 0, &media_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        check_media_type(media_type, rgb32_stream_type_desc, -1);
        IMFMediaType_Release(media_type);

        IMFTransform_Release(transform);
    }

    /* NV12 -> YUY2 conversion with MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING */
    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_media_type(media_type, yuy2_stream_type_desc, 2); /* doesn't need the frame size */
    hr = IMFSourceReader_SetCurrentMediaType(reader, 0, NULL, media_type);
    if (enable_advanced)
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    else
    {
        todo_wine_if(enable_processing) /* Wine enables advanced video processing in all cases */
        ok(hr == MF_E_TOPO_CODEC_NOT_FOUND, "Unexpected hr %#lx.\n", hr);
    }
    IMFMediaType_Release(media_type);

    hr = IMFSourceReader_GetCurrentMediaType(reader, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (enable_advanced)
        check_media_type(media_type, yuy2_expect_advanced_desc, -1);
    else if (!enable_processing)
        check_media_type(media_type, nv12_stream_type_desc, -1);
    else if  (!winetest_platform_is_wine) /* Wine enables advanced video processing in all cases */
        check_media_type(media_type, rgb32_expect_desc, -1);
    IMFMediaType_Release(media_type);

    /* convert transform is only exposed with MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING */
    hr = IMFSourceReader_GetServiceForStream(reader, 0, &GUID_NULL, &IID_IMFTransform, (void **)&transform);
    if (!enable_advanced)
        ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);
    else
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        check_media_type(media_type, nv12_stream_type_desc, -1);
        IMFMediaType_Release(media_type);

        hr = IMFTransform_GetOutputCurrentType(transform, 0, &media_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        check_media_type(media_type, yuy2_stream_type_desc, -1);
        IMFMediaType_Release(media_type);

        IMFTransform_Release(transform);
    }

    hr = IMFSourceReader_QueryInterface(reader, &IID_IMFSourceReaderEx, (void **)&reader_ex);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFSourceReaderEx_GetTransformForStream(reader_ex, 0, 0, &category, &transform);
    if (!enable_advanced)
        ok(hr == MF_E_INVALIDINDEX, "Unexpected hr %#lx.\n", hr);
    else
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        check_media_type(media_type, nv12_stream_type_desc, -1);
        IMFMediaType_Release(media_type);

        hr = IMFTransform_GetOutputCurrentType(transform, 0, &media_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        check_media_type(media_type, yuy2_stream_type_desc, -1);
        IMFMediaType_Release(media_type);

        IMFTransform_Release(transform);
    }

    hr = IMFSourceReaderEx_GetTransformForStream(reader_ex, 0, 1, &category, &transform);
    ok(hr == MF_E_INVALIDINDEX, "Unexpected hr %#lx.\n", hr);
    IMFSourceReaderEx_Release(reader_ex);

    IMFSourceReader_Release(reader);
    IMFMediaSource_Release(source);
    IMFStreamDescriptor_Release(video_stream);


    /* test source reader with a H264 source */

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_media_type(media_type, h264_stream_type_desc, -1);
    hr = MFCreateStreamDescriptor(0, 1, &media_type, &video_stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(media_type);

    source = create_test_source(&video_stream, 1);
    ok(!!source, "Failed to create test source.\n");

    hr = MFCreateSourceReaderFromMediaSource(source, attributes, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFSourceReader_SetStreamSelection(reader, 0, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_GetNativeMediaType(reader, 0, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_media_type(media_type, h264_stream_type_desc, -1);
    IMFMediaType_Release(media_type);

    hr = IMFSourceReader_GetCurrentMediaType(reader, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_media_type(media_type, h264_stream_type_desc, -1);
    IMFMediaType_Release(media_type);

    /* when H264 output is used, there's no decoder transform */
    hr = IMFSourceReader_GetServiceForStream(reader, 0, &GUID_NULL, &IID_IMFTransform, (void **)&transform);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);

    /* H264 -> RGB32 conversion with MF_SOURCE_READER_ENABLE_(ADVANCED_)VIDEO_PROCESSING  */
    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_media_type(media_type, rgb32_stream_type_desc, 2); /* doesn't need the frame size */
    hr = IMFSourceReader_SetCurrentMediaType(reader, 0, NULL, media_type);
    if (enable_processing || enable_advanced)
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    else
        todo_wine ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(media_type);

    hr = IMFSourceReader_GetCurrentMediaType(reader, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (enable_advanced)
        check_media_type(media_type, rgb32_expect_advanced_desc_todo2, -1);
    else if (!enable_processing)
        check_media_type(media_type, h264_stream_type_desc, -1);
    else if  (!winetest_platform_is_wine) /* Wine enables advanced video processing in all cases */
        check_media_type(media_type, rgb32_expect_desc, -1);
    IMFMediaType_Release(media_type);

    /* the exposed transform is the H264 decoder or the converter with MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING */
    hr = IMFSourceReader_GetServiceForStream(reader, 0, &GUID_NULL, &IID_IMFTransform, (void **)&transform);
    if (!enable_processing && !enable_advanced)
        ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);
    else
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if (enable_advanced)
            check_media_type(media_type, nv12_stream_type_desc, -1);
        else
            check_media_type(media_type, h264_stream_type_desc, -1);
        IMFMediaType_Release(media_type);

        /* with NV12 output */
        hr = IMFTransform_GetOutputCurrentType(transform, 0, &media_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if (enable_advanced)
            check_media_type(media_type, rgb32_stream_type_desc, -1);
        else
            check_media_type(media_type, nv12_stream_type_desc, -1);
        IMFMediaType_Release(media_type);

        IMFTransform_Release(transform);
    }

    /* H264 decoder transform is also available through the IMFSourceReaderEx interface */
    hr = IMFSourceReader_QueryInterface(reader, &IID_IMFSourceReaderEx, (void **)&reader_ex);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFSourceReaderEx_GetTransformForStream(reader_ex, 0, 0, &category, &transform);
    if (!enable_processing && !enable_advanced)
        ok(hr == MF_E_INVALIDINDEX, "Unexpected hr %#lx.\n", hr);
    else
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        check_media_type(media_type, h264_stream_type_desc, -1);
        IMFMediaType_Release(media_type);

        hr = IMFTransform_GetOutputCurrentType(transform, 0, &media_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        check_media_type(media_type, nv12_stream_type_desc, -1);
        IMFMediaType_Release(media_type);

        IMFTransform_Release(transform);
    }

    /* the video processor can be accessed at index 1 with MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING  */
    hr = IMFSourceReaderEx_GetTransformForStream(reader_ex, 0, 1, &category, &transform);
    if (!enable_advanced)
        ok(hr == MF_E_INVALIDINDEX, "Unexpected hr %#lx.\n", hr);
    else
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        check_media_type(media_type, nv12_stream_type_desc, -1);
        IMFMediaType_Release(media_type);

        hr = IMFTransform_GetOutputCurrentType(transform, 0, &media_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        check_media_type(media_type, rgb32_stream_type_desc, -1);
        IMFMediaType_Release(media_type);

        IMFTransform_Release(transform);
    }

    hr = IMFSourceReaderEx_GetTransformForStream(reader_ex, 0, 2, &category, &transform);
    ok(hr == MF_E_INVALIDINDEX, "Unexpected hr %#lx.\n", hr);
    IMFSourceReaderEx_Release(reader_ex);

    /* H264 -> NV12 conversion */
    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_media_type(media_type, nv12_stream_type_desc, 2); /* doesn't need the frame size */
    hr = IMFSourceReader_SetCurrentMediaType(reader, 0, NULL, media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(media_type);

    hr = IMFSourceReader_GetCurrentMediaType(reader, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_media_type(media_type, nv12_expect_desc, -1);
    IMFMediaType_Release(media_type);

    /* the H264 decoder transform can now be accessed */
    hr = IMFSourceReader_GetServiceForStream(reader, 0, &GUID_NULL, &IID_IMFTransform, (void **)&transform);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_media_type(media_type, h264_stream_type_desc, -1);
    IMFMediaType_Release(media_type);

    hr = IMFTransform_GetOutputCurrentType(transform, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_media_type(media_type, nv12_stream_type_desc, -1);
    IMFMediaType_Release(media_type);

    IMFTransform_Release(transform);

    /* YUY2 output works too */
    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_media_type(media_type, yuy2_stream_type_desc, 2); /* doesn't need the frame size */
    hr = IMFSourceReader_SetCurrentMediaType(reader, 0, NULL, media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(media_type);

    hr = IMFSourceReader_GetCurrentMediaType(reader, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_media_type(media_type, yuy2_expect_desc, -1);
    IMFMediaType_Release(media_type);

    /* H264 decoder transform is also available through the IMFSourceReaderEx interface */
    hr = IMFSourceReader_QueryInterface(reader, &IID_IMFSourceReaderEx, (void **)&reader_ex);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFSourceReaderEx_GetTransformForStream(reader_ex, 0, 0, &category, &transform);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        check_media_type(media_type, h264_stream_type_desc, -1);
        IMFMediaType_Release(media_type);

        hr = IMFTransform_GetOutputCurrentType(transform, 0, &media_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        check_media_type(media_type, yuy2_stream_type_desc, -1);
        IMFMediaType_Release(media_type);

        /* changing the transform media type doesn't change the reader output type */
        hr = MFCreateMediaType(&media_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        init_media_type(media_type, nv12_stream_type_desc, -1);
        hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        IMFMediaType_Release(media_type);

        hr = IMFSourceReader_GetCurrentMediaType(reader, 0, &media_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        check_media_type(media_type, yuy2_expect_desc, -1);
        IMFMediaType_Release(media_type);

        IMFTransform_Release(transform);
    }

    hr = IMFSourceReaderEx_GetTransformForStream(reader_ex, 0, 1, &category, &transform);
    ok(hr == MF_E_INVALIDINDEX, "Unexpected hr %#lx.\n", hr);
    IMFSourceReaderEx_Release(reader_ex);

skip_tests:
    IMFSourceReader_Release(reader);
    IMFMediaSource_Release(source);
    IMFStreamDescriptor_Release(video_stream);

    IMFAttributes_Release(attributes);

    winetest_pop_context();
}

static BOOL test_decoder_d3d_aware;
static BOOL test_decoder_d3d11_aware;
static BOOL test_decoder_got_d3d_manager;
static BOOL test_decoder_allocate_samples;
static IDirect3DDeviceManager9 *expect_d3d9_manager;
static IMFDXGIDeviceManager *expect_dxgi_manager;

struct test_decoder
{
    IMFTransform IMFTransform_iface;
    LONG refcount;

    IMFAttributes *attributes;
    IMFMediaType *input_type;
    IMFMediaType *output_type;

    MFVIDEOFORMAT output_format;
    HRESULT next_output;
};

static struct test_decoder *test_decoder_from_IMFTransform(IMFTransform *iface)
{
    return CONTAINING_RECORD(iface, struct test_decoder, IMFTransform_iface);
}

static HRESULT WINAPI test_decoder_QueryInterface(IMFTransform *iface, REFIID iid, void **out)
{
    struct test_decoder *decoder = test_decoder_from_IMFTransform(iface);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
            IsEqualGUID(iid, &IID_IMFTransform))
    {
        IMFTransform_AddRef(&decoder->IMFTransform_iface);
        *out = &decoder->IMFTransform_iface;
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_decoder_AddRef(IMFTransform *iface)
{
    struct test_decoder *decoder = test_decoder_from_IMFTransform(iface);
    ULONG refcount = InterlockedIncrement(&decoder->refcount);
    return refcount;
}

static ULONG WINAPI test_decoder_Release(IMFTransform *iface)
{
    struct test_decoder *decoder = test_decoder_from_IMFTransform(iface);
    ULONG refcount = InterlockedDecrement(&decoder->refcount);

    if (!refcount)
    {
        if (decoder->input_type)
            IMFMediaType_Release(decoder->input_type);
        if (decoder->output_type)
            IMFMediaType_Release(decoder->output_type);
        free(decoder);
    }

    return refcount;
}

static HRESULT WINAPI test_decoder_GetStreamLimits(IMFTransform *iface, DWORD *input_minimum,
        DWORD *input_maximum, DWORD *output_minimum, DWORD *output_maximum)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_decoder_GetStreamCount(IMFTransform *iface, DWORD *inputs, DWORD *outputs)
{
    *inputs = *outputs = 1;
    return S_OK;
}

static HRESULT WINAPI test_decoder_GetStreamIDs(IMFTransform *iface, DWORD input_size, DWORD *inputs,
        DWORD output_size, DWORD *outputs)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_decoder_GetInputStreamInfo(IMFTransform *iface, DWORD id, MFT_INPUT_STREAM_INFO *info)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_decoder_GetOutputStreamInfo(IMFTransform *iface, DWORD id, MFT_OUTPUT_STREAM_INFO *info)
{
    struct test_decoder *decoder = test_decoder_from_IMFTransform(iface);
    UINT64 frame_size;
    GUID subtype;

    if (!decoder->output_type || FAILED(IMFMediaType_GetUINT64(decoder->output_type, &MF_MT_FRAME_SIZE, &frame_size)))
        frame_size = (UINT64)96 << 32 | 96;
    if (!decoder->output_type || FAILED(IMFMediaType_GetGUID(decoder->output_type, &MF_MT_SUBTYPE, &subtype)))
        subtype = MFVideoFormat_YUY2;

    memset(info, 0, sizeof(*info));
    if (test_decoder_allocate_samples)
        info->dwFlags |= MFT_OUTPUT_STREAM_PROVIDES_SAMPLES;
    return MFCalculateImageSize(&MFVideoFormat_RGB32, (UINT32)frame_size, frame_size >> 32, (UINT32 *)&info->cbSize);
}

static HRESULT WINAPI test_decoder_GetAttributes(IMFTransform *iface, IMFAttributes **attributes)
{
    struct test_decoder *decoder = test_decoder_from_IMFTransform(iface);
    if (!(*attributes = decoder->attributes))
        return E_NOTIMPL;
    IMFAttributes_AddRef(*attributes);
    return S_OK;
}

static HRESULT WINAPI test_decoder_GetInputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_decoder_GetOutputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_decoder_DeleteInputStream(IMFTransform *iface, DWORD id)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_decoder_AddInputStreams(IMFTransform *iface, DWORD streams, DWORD *ids)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_decoder_GetInputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static void test_decoder_set_output_format(IMFTransform *iface, const MFVIDEOFORMAT *output_format)
{
    struct test_decoder *decoder = test_decoder_from_IMFTransform(iface);
    decoder->output_format = *output_format;
}

static HRESULT WINAPI test_decoder_GetOutputAvailableType(IMFTransform *iface, DWORD id,
        DWORD index, IMFMediaType **type)
{
    struct test_decoder *decoder = test_decoder_from_IMFTransform(iface);
    const GUID subtypes[] =
    {
        MFVideoFormat_NV12,
        MFVideoFormat_YUY2,
    };
    MFVIDEOFORMAT format =
    {
        .dwSize = sizeof(format),
        .videoInfo =
        {
            .dwWidth = 96,
            .dwHeight = 96,
        },
    };
    HRESULT hr;

    *type = NULL;
    if (index >= ARRAY_SIZE(subtypes))
        return MF_E_NO_MORE_TYPES;

    if (decoder->output_format.dwSize)
        format = decoder->output_format;
    format.guidFormat = subtypes[index];

    hr = MFCreateMediaType(type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitMediaTypeFromMFVideoFormat(*type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    return hr;
}

static HRESULT WINAPI test_decoder_SetInputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct test_decoder *decoder = test_decoder_from_IMFTransform(iface);
    if (flags & MFT_SET_TYPE_TEST_ONLY)
        return S_OK;
    if (decoder->input_type)
        IMFMediaType_Release(decoder->input_type);
    if ((decoder->input_type = type))
        IMFMediaType_AddRef(decoder->input_type);
    return S_OK;
}

static HRESULT WINAPI test_decoder_SetOutputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct test_decoder *decoder = test_decoder_from_IMFTransform(iface);
    GUID subtype;
    HRESULT hr;

    if (type && SUCCEEDED(hr = IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &subtype))
            && (IsEqualGUID(&subtype, &MFVideoFormat_RGB32)
            || IsEqualGUID(&subtype, &MFVideoFormat_ABGR32)))
        return MF_E_INVALIDMEDIATYPE;

    if (flags & MFT_SET_TYPE_TEST_ONLY)
        return S_OK;
    if (decoder->output_type)
        IMFMediaType_Release(decoder->output_type);
    if ((decoder->output_type = type))
        IMFMediaType_AddRef(decoder->output_type);
    return S_OK;
}

static HRESULT WINAPI test_decoder_GetInputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct test_decoder *decoder = test_decoder_from_IMFTransform(iface);
    if (!(*type = decoder->input_type))
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    IMFMediaType_AddRef(*type);
    return S_OK;
}

static HRESULT WINAPI test_decoder_GetOutputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct test_decoder *decoder = test_decoder_from_IMFTransform(iface);
    if (!(*type = decoder->output_type))
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    IMFMediaType_AddRef(*type);
    return S_OK;
}

static HRESULT WINAPI test_decoder_GetInputStatus(IMFTransform *iface, DWORD id, DWORD *flags)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_decoder_GetOutputStatus(IMFTransform *iface, DWORD *flags)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_decoder_SetOutputBounds(IMFTransform *iface, LONGLONG lower, LONGLONG upper)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_decoder_ProcessEvent(IMFTransform *iface, DWORD id, IMFMediaEvent *event)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_decoder_ProcessMessage(IMFTransform *iface, MFT_MESSAGE_TYPE message, ULONG_PTR param)
{
    switch (message)
    {
    case MFT_MESSAGE_COMMAND_FLUSH:
    case MFT_MESSAGE_NOTIFY_BEGIN_STREAMING:
    case MFT_MESSAGE_NOTIFY_END_STREAMING:
    case MFT_MESSAGE_NOTIFY_END_OF_STREAM:
    case MFT_MESSAGE_NOTIFY_START_OF_STREAM:
        return S_OK;

    case MFT_MESSAGE_SET_D3D_MANAGER:
        ok(test_decoder_d3d_aware || test_decoder_d3d11_aware, "Unexpected call.\n");
        if (!param)
            return S_OK;

        if (test_decoder_d3d_aware)
        {
            IDirect3DDeviceManager9 *manager;
            HRESULT hr;

            hr = IUnknown_QueryInterface((IUnknown *)param, &IID_IDirect3DDeviceManager9, (void **)&manager);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(manager == expect_d3d9_manager, "got manager %p\n", manager);
            IDirect3DDeviceManager9_Release(manager);

            test_decoder_got_d3d_manager = TRUE;
        }
        if (test_decoder_d3d11_aware)
        {
            IMFDXGIDeviceManager *manager;
            HRESULT hr;

            hr = IUnknown_QueryInterface((IUnknown *)param, &IID_IMFDXGIDeviceManager, (void **)&manager);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(manager == expect_dxgi_manager, "got manager %p\n", manager);
            IMFDXGIDeviceManager_Release(manager);

            test_decoder_got_d3d_manager = TRUE;
        }
        return S_OK;

    default:
        ok(0, "Unexpected call.\n");
        return E_NOTIMPL;
    }
}

static HRESULT WINAPI test_decoder_ProcessInput(IMFTransform *iface, DWORD id, IMFSample *sample, DWORD flags)
{
    return S_OK;
}

static void test_decoder_set_next_output(IMFTransform *iface, HRESULT hr)
{
    struct test_decoder *decoder = test_decoder_from_IMFTransform(iface);
    decoder->next_output = hr;
}

static HRESULT WINAPI test_decoder_ProcessOutput(IMFTransform *iface, DWORD flags, DWORD count,
        MFT_OUTPUT_DATA_BUFFER *data, DWORD *status)
{
    struct test_decoder *decoder = test_decoder_from_IMFTransform(iface);
    IMFMediaBuffer *buffer;
    IUnknown *unknown;
    HRESULT hr;

    if (test_decoder_allocate_samples)
    {
        ok(!data->pSample, "Unexpected sample\n");

        hr = MFCreateSample(&data->pSample);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = MFCreateMemoryBuffer(96 * 96 * 4, &buffer);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IMFSample_AddBuffer(data->pSample, buffer);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        IMFMediaBuffer_Release(buffer);
    }
    else
    {
        ok(!!data->pSample, "Missing sample\n");

        hr = IMFSample_GetBufferByIndex(data->pSample, 0, &buffer);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        todo_wine check_interface(buffer, &IID_IMF2DBuffer2, TRUE);
        todo_wine check_interface(buffer, &IID_IMFGetService, TRUE);
        check_interface(buffer, &IID_IMFDXGIBuffer, FALSE);
        hr = MFGetService((IUnknown *)buffer, &MR_BUFFER_SERVICE, &IID_IDirect3DSurface9, (void **)&unknown);
        todo_wine ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
        IMFMediaBuffer_Release(buffer);
    }

    if (decoder->next_output == MF_E_TRANSFORM_STREAM_CHANGE)
    {
        data[0].dwStatus = MFT_OUTPUT_DATA_BUFFER_FORMAT_CHANGE;
        decoder->next_output = S_OK;
        return MF_E_TRANSFORM_STREAM_CHANGE;
    }

    if (decoder->next_output == S_OK)
    {
        decoder->next_output = MF_E_TRANSFORM_NEED_MORE_INPUT;
        hr = IMFSample_SetSampleTime(data->pSample, 0);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        return S_OK;
    }

    return decoder->next_output;
}

static const IMFTransformVtbl test_decoder_vtbl =
{
    test_decoder_QueryInterface,
    test_decoder_AddRef,
    test_decoder_Release,
    test_decoder_GetStreamLimits,
    test_decoder_GetStreamCount,
    test_decoder_GetStreamIDs,
    test_decoder_GetInputStreamInfo,
    test_decoder_GetOutputStreamInfo,
    test_decoder_GetAttributes,
    test_decoder_GetInputStreamAttributes,
    test_decoder_GetOutputStreamAttributes,
    test_decoder_DeleteInputStream,
    test_decoder_AddInputStreams,
    test_decoder_GetInputAvailableType,
    test_decoder_GetOutputAvailableType,
    test_decoder_SetInputType,
    test_decoder_SetOutputType,
    test_decoder_GetInputCurrentType,
    test_decoder_GetOutputCurrentType,
    test_decoder_GetInputStatus,
    test_decoder_GetOutputStatus,
    test_decoder_SetOutputBounds,
    test_decoder_ProcessEvent,
    test_decoder_ProcessMessage,
    test_decoder_ProcessInput,
    test_decoder_ProcessOutput,
};

static HRESULT WINAPI test_mft_factory_QueryInterface(IClassFactory *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IClassFactory) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_mft_factory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI test_mft_factory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI test_mft_factory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **obj)
{
    struct test_decoder *decoder;
    HRESULT hr;

    if (!(decoder = calloc(1, sizeof(*decoder))))
        return E_OUTOFMEMORY;
    decoder->IMFTransform_iface.lpVtbl = &test_decoder_vtbl;
    decoder->refcount = 1;

    if (test_decoder_d3d_aware || test_decoder_d3d11_aware)
    {
        hr = MFCreateAttributes(&decoder->attributes, 1);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    }
    if (test_decoder_d3d_aware)
    {
        hr = IMFAttributes_SetUINT32(decoder->attributes, &MF_SA_D3D_AWARE, 1);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    }
    if (test_decoder_d3d11_aware)
    {
        hr = IMFAttributes_SetUINT32(decoder->attributes, &MF_SA_D3D11_AWARE, 1);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    }

    *obj = &decoder->IMFTransform_iface;
    return S_OK;
}

static HRESULT WINAPI test_mft_factory_LockServer(IClassFactory *iface, BOOL fLock)
{
    return S_OK;
}

static const IClassFactoryVtbl test_mft_factory_vtbl =
{
    test_mft_factory_QueryInterface,
    test_mft_factory_AddRef,
    test_mft_factory_Release,
    test_mft_factory_CreateInstance,
    test_mft_factory_LockServer,
};

static void test_source_reader_transform_stream_change(void)
{
    static const struct attribute_desc test_stream_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_TEST),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        {0},
    };
    static const struct attribute_desc yuy2_stream_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YUY2),
        {0},
    };
    static const struct attribute_desc yuy2_expect_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YUY2),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1, .todo = TRUE),
        ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1, .todo = TRUE),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, 96 * 2, .todo = TRUE),
        ATTR_UINT32(MF_MT_SAMPLE_SIZE, 96 * 96 * 2, .todo = TRUE),
        {0},
    };
    static const struct attribute_desc yuy2_expect_new_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_YUY2),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 128, 128),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, 128 * 2),
        ATTR_UINT32(MF_MT_SAMPLE_SIZE, 128 * 128 * 2),
        {0},
    };
    const MFT_REGISTER_TYPE_INFO output_info[] =
    {
        {MFMediaType_Video, MFVideoFormat_NV12},
        {MFMediaType_Video, MFVideoFormat_YUY2},
    };
    const MFT_REGISTER_TYPE_INFO input_info[] =
    {
        {MFMediaType_Video, MFVideoFormat_TEST},
    };
    MFVIDEOFORMAT output_format = {.dwSize = sizeof(output_format)};
    IClassFactory factory = {.lpVtbl = &test_mft_factory_vtbl};
    IMFStreamDescriptor *video_stream;
    IMFSourceReaderEx *reader_ex;
    IMFTransform *test_decoder;
    IMFMediaType *media_type;
    IMFSourceReader *reader;
    IMFMediaSource *source;
    LONGLONG timestamp;
    DWORD index, flags;
    IMFSample *sample;
    GUID category;
    HRESULT hr;


    hr = MFTRegisterLocal(&factory, &MFT_CATEGORY_VIDEO_DECODER, L"Test Decoder", 0,
            ARRAY_SIZE(input_info), input_info, ARRAY_SIZE(output_info), output_info);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* test source reader with a custom source */

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_media_type(media_type, test_stream_type_desc, -1);
    hr = MFCreateStreamDescriptor(0, 1, &media_type, &video_stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(media_type);

    source = create_test_source(&video_stream, 1);
    ok(!!source, "Failed to create test source.\n");
    IMFStreamDescriptor_Release(video_stream);

    hr = MFCreateSourceReaderFromMediaSource(source, NULL, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaSource_Release(source);

    /* skip tests on Win7 which misses IMFSourceReaderEx */
    hr = IMFSourceReader_QueryInterface(reader, &IID_IMFSourceReaderEx, (void **)&reader_ex);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* Win7 */, "Unexpected hr %#lx.\n", hr);
    if (broken(hr == E_NOINTERFACE))
    {
        win_skip("missing IMFSourceReaderEx interface, skipping tests on Win7\n");
        goto skip_tests;
    }
    IMFSourceReaderEx_Release(reader_ex);

    hr = IMFSourceReader_SetStreamSelection(reader, 0, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);


    hr = IMFSourceReader_GetNativeMediaType(reader, 0, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_media_type(media_type, test_stream_type_desc, -1);
    IMFMediaType_Release(media_type);

    hr = IMFSourceReader_GetCurrentMediaType(reader, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_media_type(media_type, test_stream_type_desc, -1);
    IMFMediaType_Release(media_type);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_media_type(media_type, yuy2_stream_type_desc, -1);
    hr = IMFSourceReader_SetCurrentMediaType(reader, 0, NULL, media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(media_type);

    hr = IMFSourceReader_GetCurrentMediaType(reader, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_media_type(media_type, yuy2_expect_desc, -1);
    IMFMediaType_Release(media_type);



    hr = IMFSourceReader_QueryInterface(reader, &IID_IMFSourceReaderEx, (void **)&reader_ex);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFSourceReaderEx_GetTransformForStream(reader_ex, 0, 0, &category, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    hr = IMFSourceReaderEx_GetTransformForStream(reader_ex, 0, 0, NULL, &test_decoder);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(test_decoder->lpVtbl == &test_decoder_vtbl, "got unexpected transform\n");
    IMFSourceReaderEx_Release(reader_ex);

    fail_request_sample = FALSE;

    test_decoder_set_next_output(test_decoder, MF_E_TRANSFORM_STREAM_CHANGE);

    output_format.videoInfo.dwHeight = 128;
    output_format.videoInfo.dwWidth = 128;
    test_decoder_set_output_format(test_decoder, &output_format);

    sample = (void *)0xdeadbeef;
    index = flags = timestamp = 0xdeadbeef;
    hr = IMFSourceReader_ReadSample(reader, 0, 0, &index, &flags, &timestamp, &sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(index == 0, "got %lu.\n", index);
    ok(flags == MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED, "got %lu.\n", flags);
    ok(timestamp == 0, "got %I64d.\n", timestamp);
    ok(sample != (void *)0xdeadbeef, "got %p.\n", sample);
    IMFSample_Release(sample);

    hr = IMFSourceReader_GetCurrentMediaType(reader, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_media_type(media_type, yuy2_expect_new_desc, -1);
    IMFMediaType_Release(media_type);

    fail_request_sample = TRUE;

    IMFTransform_Release(test_decoder);

skip_tests:
    IMFSourceReader_Release(reader);

    hr = MFTUnregisterLocal(&factory);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
}

static void test_source_reader_transforms_d3d9(void)
{
    static const struct attribute_desc test_stream_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_TEST),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        {0},
    };
    static const struct attribute_desc rgb32_stream_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
        {0},
    };
    static const struct attribute_desc rgb32_expect_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        ATTR_UINT32(MF_MT_COMPRESSED, 0, .todo = TRUE),
        ATTR_UINT32(MF_MT_INTERLACE_MODE, 2, .todo = TRUE),
        {0},
    };
    const MFT_REGISTER_TYPE_INFO output_info[] =
    {
        {MFMediaType_Video, MFVideoFormat_NV12},
        {MFMediaType_Video, MFVideoFormat_YUY2},
    };
    const MFT_REGISTER_TYPE_INFO input_info[] =
    {
        {MFMediaType_Video, MFVideoFormat_TEST},
    };
    IClassFactory factory = {.lpVtbl = &test_mft_factory_vtbl};
    IMFTransform *test_decoder, *video_processor;
    IDirect3DDeviceManager9 *d3d9_manager;
    IMFStreamDescriptor *video_stream;
    IDirect3DDevice9 *d3d9_device;
    IMFSourceReaderEx *reader_ex;
    IMFAttributes *attributes;
    IMFMediaType *media_type;
    IMFSourceReader *reader;
    IMFMediaBuffer *buffer;
    IMFMediaSource *source;
    LONGLONG timestamp;
    DWORD index, flags;
    IMFSample *sample;
    IDirect3D9 *d3d9;
    UINT32 value;
    HWND window;
    UINT token;
    HRESULT hr;

    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d9)
    {
        skip("Failed to create a D3D9 object, skipping tests.\n");
        return;
    }

    window = create_window();
    if (!(d3d9_device = create_d3d9_device(d3d9, window)))
    {
        skip("Failed to create a D3D9 device, skipping tests.\n");
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }
    IDirect3D9_Release(d3d9);

    test_decoder_d3d_aware = TRUE;

    hr = MFTRegisterLocal(&factory, &MFT_CATEGORY_VIDEO_DECODER, L"Test Decoder", 0,
            ARRAY_SIZE(input_info), input_info, ARRAY_SIZE(output_info), output_info);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateAttributes(&attributes, 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_SetUINT32(attributes, &MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = DXVA2CreateDirect3DDeviceManager9(&token, &d3d9_manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDirect3DDeviceManager9_ResetDevice(d3d9_manager, d3d9_device, token);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IDirect3DDevice9_Release(d3d9_device);

    hr = IMFAttributes_SetUnknown(attributes, &MF_SOURCE_READER_D3D_MANAGER, (IUnknown *)d3d9_manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    expect_d3d9_manager = d3d9_manager;


    /* test d3d aware decoder that doesn't allocate buffers */

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_media_type(media_type, test_stream_type_desc, -1);
    hr = MFCreateStreamDescriptor(0, 1, &media_type, &video_stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(media_type);

    source = create_test_source(&video_stream, 1);
    ok(!!source, "Failed to create test source.\n");
    IMFStreamDescriptor_Release(video_stream);

    hr = MFCreateSourceReaderFromMediaSource(source, attributes, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFAttributes_Release(attributes);
    IMFMediaSource_Release(source);

    /* skip tests on Win7 which misses IMFSourceReaderEx */
    hr = IMFSourceReader_QueryInterface(reader, &IID_IMFSourceReaderEx, (void **)&reader_ex);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* Win7 */, "Unexpected hr %#lx.\n", hr);
    if (broken(hr == E_NOINTERFACE))
    {
        win_skip("missing IMFSourceReaderEx interface, skipping tests on Win7\n");
        IMFSourceReader_Release(reader);
        goto skip_tests;
    }

    hr = IMFSourceReader_SetStreamSelection(reader, 0, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_GetNativeMediaType(reader, 0, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_media_type(media_type, test_stream_type_desc, -1);
    IMFMediaType_Release(media_type);

    hr = IMFSourceReader_GetCurrentMediaType(reader, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_media_type(media_type, test_stream_type_desc, -1);
    IMFMediaType_Release(media_type);
    ok(!test_decoder_got_d3d_manager, "d3d manager received\n");

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_media_type(media_type, rgb32_stream_type_desc, -1);
    hr = IMFSourceReader_SetCurrentMediaType(reader, 0, NULL, media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(media_type);
    ok(!!test_decoder_got_d3d_manager, "d3d manager not received\n");

    hr = IMFSourceReader_GetCurrentMediaType(reader, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_media_type(media_type, rgb32_expect_desc, -1);
    IMFMediaType_Release(media_type);


    /* video processor transform is not D3D9 aware on more recent Windows */

    hr = IMFSourceReaderEx_GetTransformForStream(reader_ex, 0, 1, NULL, &video_processor);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(video_processor->lpVtbl != &test_decoder_vtbl, "got unexpected transform\n");
    hr = IMFTransform_GetAttributes(video_processor, &attributes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_GetUINT32(attributes, &MF_SA_D3D_AWARE, &value);
    todo_wine /* Wine exposes MF_SA_D3D_AWARE on the video processor, as Win7 */
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_GetUINT32(attributes, &MF_SA_D3D11_AWARE, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value == 1, "got %u.\n", value);
    hr = IMFAttributes_GetUINT32(attributes, &MF_SA_MINIMUM_OUTPUT_SAMPLE_COUNT, &value);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFAttributes_Release(attributes);

    hr = IMFTransform_GetOutputStreamAttributes(video_processor, 0, &attributes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_GetCount(attributes, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value == 0, "got %u.\n", value);
    IMFAttributes_Release(attributes);

    hr = IMFTransform_GetOutputCurrentType(video_processor, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_media_type(media_type, rgb32_expect_desc, -1);
    IMFMediaType_Release(media_type);

    IMFTransform_Release(video_processor);

    hr = IMFSourceReaderEx_GetTransformForStream(reader_ex, 0, 0, NULL, &test_decoder);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(test_decoder->lpVtbl == &test_decoder_vtbl, "got unexpected transform\n");
    hr = IMFTransform_GetAttributes(test_decoder, &attributes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_GetUINT32(attributes, &MF_SA_D3D_AWARE, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value == 1, "got %u\n", value);
    IMFAttributes_Release(attributes);


    fail_request_sample = FALSE;
    test_decoder_set_next_output(test_decoder, S_OK);

    sample = (void *)0xdeadbeef;
    index = flags = timestamp = 0xdeadbeef;
    hr = IMFSourceReader_ReadSample(reader, 0, 0, &index, &flags, &timestamp, &sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(index == 0, "got %lu.\n", index);
    ok(flags == 0, "got %lu.\n", flags);
    ok(timestamp == 0, "got %I64d.\n", timestamp);
    ok(sample != (void *)0xdeadbeef, "got %p.\n", sample);

    hr = IMFSample_GetBufferByIndex(sample, 0, &buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_interface(buffer, &IID_IMF2DBuffer2, TRUE);
    check_interface(buffer, &IID_IMFGetService, TRUE);
    check_interface(buffer, &IID_IMFDXGIBuffer, FALSE);
    check_service_interface(buffer, &MR_BUFFER_SERVICE, &IID_IDirect3DSurface9, TRUE);
    IMFMediaBuffer_Release(buffer);

    IMFSample_Release(sample);

    fail_request_sample = TRUE;


    /* video processor output stream attributes are left empty in D3D9 mode */

    hr = IMFSourceReaderEx_GetTransformForStream(reader_ex, 0, 1, NULL, &video_processor);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(video_processor->lpVtbl != &test_decoder_vtbl, "got unexpected transform\n");

    hr = IMFTransform_GetAttributes(video_processor, &attributes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_GetUINT32(attributes, &MF_SA_MINIMUM_OUTPUT_SAMPLE_COUNT, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value == 6, "got %u.\n", value);
    IMFAttributes_Release(attributes);

    hr = IMFTransform_GetOutputStreamAttributes(video_processor, 0, &attributes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_GetCount(attributes, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value == 0, "got %u.\n", value);
    IMFAttributes_Release(attributes);

    IMFTransform_Release(video_processor);


    IMFTransform_Release(test_decoder);
    IMFSourceReaderEx_Release(reader_ex);
    IMFSourceReader_Release(reader);


    /* test d3d aware decoder that allocates buffers */

    test_decoder_allocate_samples = TRUE;

    hr = MFCreateAttributes(&attributes, 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_SetUINT32(attributes, &MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_SetUnknown(attributes, &MF_SOURCE_READER_D3D_MANAGER, (IUnknown *)d3d9_manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_media_type(media_type, test_stream_type_desc, -1);
    hr = MFCreateStreamDescriptor(0, 1, &media_type, &video_stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(media_type);

    source = create_test_source(&video_stream, 1);
    ok(!!source, "Failed to create test source.\n");
    IMFStreamDescriptor_Release(video_stream);

    hr = MFCreateSourceReaderFromMediaSource(source, attributes, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFAttributes_Release(attributes);
    IMFMediaSource_Release(source);

    hr = IMFSourceReader_SetStreamSelection(reader, 0, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_media_type(media_type, rgb32_stream_type_desc, -1);
    hr = IMFSourceReader_SetCurrentMediaType(reader, 0, NULL, media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(media_type);
    ok(!!test_decoder_got_d3d_manager, "d3d manager not received\n");


    hr = IMFSourceReader_QueryInterface(reader, &IID_IMFSourceReaderEx, (void **)&reader_ex);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFSourceReaderEx_GetTransformForStream(reader_ex, 0, 0, NULL, &test_decoder);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(test_decoder->lpVtbl == &test_decoder_vtbl, "got unexpected transform\n");
    IMFSourceReaderEx_Release(reader_ex);

    fail_request_sample = FALSE;
    test_decoder_set_next_output(test_decoder, S_OK);

    sample = (void *)0xdeadbeef;
    index = flags = timestamp = 0xdeadbeef;
    hr = IMFSourceReader_ReadSample(reader, 0, 0, &index, &flags, &timestamp, &sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(index == 0, "got %lu.\n", index);
    ok(flags == 0, "got %lu.\n", flags);
    ok(timestamp == 0, "got %I64d.\n", timestamp);
    ok(sample != (void *)0xdeadbeef, "got %p.\n", sample);

    /* the buffer we received is a D3D buffer nonetheless */
    hr = IMFSample_GetBufferByIndex(sample, 0, &buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_interface(buffer, &IID_IMF2DBuffer2, TRUE);
    check_interface(buffer, &IID_IMFGetService, TRUE);
    check_interface(buffer, &IID_IMFDXGIBuffer, FALSE);
    check_service_interface(buffer, &MR_BUFFER_SERVICE, &IID_IDirect3DSurface9, TRUE);
    IMFMediaBuffer_Release(buffer);

    IMFSample_Release(sample);

    fail_request_sample = TRUE;

    IMFTransform_Release(test_decoder);
    IMFSourceReader_Release(reader);

    test_decoder_allocate_samples = FALSE;


skip_tests:
    hr = MFTUnregisterLocal(&factory);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IDirect3DDeviceManager9_Release(d3d9_manager);
    DestroyWindow(window);

    test_decoder_got_d3d_manager = FALSE;
    test_decoder_d3d_aware = FALSE;
    expect_d3d9_manager = NULL;
}

static void test_source_reader_transforms_d3d11(void)
{
    static const struct attribute_desc test_stream_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_TEST),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        {0},
    };
    static const struct attribute_desc rgb32_stream_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
        {0},
    };
    static const struct attribute_desc rgb32_expect_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        ATTR_UINT32(MF_MT_COMPRESSED, 0, .todo = TRUE),
        ATTR_UINT32(MF_MT_INTERLACE_MODE, 2, .todo = TRUE),
        {0},
    };
    static const struct attribute_desc abgr32_stream_type_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_ABGR32),
        {0},
    };
    static const struct attribute_desc abgr32_expect_desc[] =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_ABGR32),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 96, 96),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        ATTR_UINT32(MF_MT_COMPRESSED, 0, .todo = TRUE),
        ATTR_UINT32(MF_MT_INTERLACE_MODE, 2, .todo = TRUE),
        {0},
    };
    const MFT_REGISTER_TYPE_INFO output_info[] =
    {
        {MFMediaType_Video, MFVideoFormat_NV12},
        {MFMediaType_Video, MFVideoFormat_YUY2},
    };
    const MFT_REGISTER_TYPE_INFO input_info[] =
    {
        {MFMediaType_Video, MFVideoFormat_TEST},
    };
    IClassFactory factory = {.lpVtbl = &test_mft_factory_vtbl};
    IMFTransform *test_decoder, *video_processor;
    IMFStreamDescriptor *video_stream;
    ID3D11Multithread *multithread;
    IMFDXGIDeviceManager *manager;
    IMFSourceReaderEx *reader_ex;
    IMFAttributes *attributes;
    IMFMediaType *media_type;
    IMFSourceReader *reader;
    IMFMediaBuffer *buffer;
    IMFMediaSource *source;
    UINT32 value, token;
    ID3D11Device *d3d11;
    LONGLONG timestamp;
    DWORD index, flags;
    IMFSample *sample;
    HRESULT hr;

    if (!pMFCreateDXGIDeviceManager)
    {
        win_skip("MFCreateDXGIDeviceManager() is not available, skipping tests.\n");
        return;
    }

    hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_VIDEO_SUPPORT, NULL, 0,
            D3D11_SDK_VERSION, &d3d11, NULL, NULL);
    if (FAILED(hr))
    {
        skip("D3D11 device creation failed, skipping tests.\n");
        return;
    }

    hr = ID3D11Device_QueryInterface(d3d11, &IID_ID3D11Multithread, (void **)&multithread);
    ok(hr == S_OK, "got %#lx\n", hr);
    ID3D11Multithread_SetMultithreadProtected(multithread, TRUE);
    ID3D11Multithread_Release(multithread);

    hr = pMFCreateDXGIDeviceManager(&token, &manager);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFDXGIDeviceManager_ResetDevice(manager, (IUnknown *)d3d11, token);
    ok(hr == S_OK, "got %#lx\n", hr);
    ID3D11Device_Release(d3d11);


    test_decoder_d3d11_aware = TRUE;

    hr = MFTRegisterLocal(&factory, &MFT_CATEGORY_VIDEO_DECODER, L"Test Decoder", 0,
            ARRAY_SIZE(input_info), input_info, ARRAY_SIZE(output_info), output_info);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateAttributes(&attributes, 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_SetUINT32(attributes, &MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_SetUnknown(attributes, &MF_SOURCE_READER_D3D_MANAGER, (IUnknown *)manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    expect_dxgi_manager = manager;


    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_media_type(media_type, test_stream_type_desc, -1);
    hr = MFCreateStreamDescriptor(0, 1, &media_type, &video_stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(media_type);

    source = create_test_source(&video_stream, 1);
    ok(!!source, "Failed to create test source.\n");
    IMFStreamDescriptor_Release(video_stream);

    hr = MFCreateSourceReaderFromMediaSource(source, attributes, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFAttributes_Release(attributes);
    IMFMediaSource_Release(source);

    /* skip tests on Win7 which misses IMFSourceReaderEx */
    hr = IMFSourceReader_QueryInterface(reader, &IID_IMFSourceReaderEx, (void **)&reader_ex);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* Win7 */, "Unexpected hr %#lx.\n", hr);
    if (broken(hr == E_NOINTERFACE))
    {
        win_skip("missing IMFSourceReaderEx interface, skipping tests on Win7\n");
        IMFSourceReader_Release(reader);
        goto skip_tests;
    }

    hr = IMFSourceReader_SetStreamSelection(reader, 0, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceReader_GetNativeMediaType(reader, 0, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_media_type(media_type, test_stream_type_desc, -1);
    IMFMediaType_Release(media_type);

    hr = IMFSourceReader_GetCurrentMediaType(reader, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_media_type(media_type, test_stream_type_desc, -1);
    IMFMediaType_Release(media_type);
    ok(!test_decoder_got_d3d_manager, "d3d manager received\n");

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_media_type(media_type, abgr32_stream_type_desc, -1);
    hr = IMFSourceReader_SetCurrentMediaType(reader, 0, NULL, media_type);
    ok(hr == S_OK || broken(hr == MF_E_INVALIDMEDIATYPE) /* needs a GPU */, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(media_type);

    if (hr == S_OK)
    {
        hr = IMFSourceReader_GetCurrentMediaType(reader, 0, &media_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        check_media_type(media_type, abgr32_expect_desc, -1);
        IMFMediaType_Release(media_type);
    }

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    init_media_type(media_type, rgb32_stream_type_desc, -1);
    hr = IMFSourceReader_SetCurrentMediaType(reader, 0, NULL, media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(media_type);

    hr = IMFSourceReader_GetCurrentMediaType(reader, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_media_type(media_type, rgb32_expect_desc, -1);
    IMFMediaType_Release(media_type);


    /* video processor output stream attributes are still empty */

    hr = IMFSourceReaderEx_GetTransformForStream(reader_ex, 0, 1, NULL, &video_processor);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(video_processor->lpVtbl != &test_decoder_vtbl, "got unexpected transform\n");

    hr = IMFTransform_GetAttributes(video_processor, &attributes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_GetUINT32(attributes, &MF_SA_MINIMUM_OUTPUT_SAMPLE_COUNT, &value);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFAttributes_Release(attributes);

    hr = IMFTransform_GetOutputStreamAttributes(video_processor, 0, &attributes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_GetCount(attributes, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value == 0, "got %u.\n", value);
    IMFAttributes_Release(attributes);

    IMFTransform_Release(video_processor);


    hr = IMFSourceReaderEx_GetTransformForStream(reader_ex, 0, 0, NULL, &test_decoder);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(test_decoder->lpVtbl == &test_decoder_vtbl, "got unexpected transform\n");

    fail_request_sample = FALSE;
    test_decoder_set_next_output(test_decoder, S_OK);

    sample = (void *)0xdeadbeef;
    index = flags = timestamp = 0xdeadbeef;
    hr = IMFSourceReader_ReadSample(reader, 0, 0, &index, &flags, &timestamp, &sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(index == 0, "got %lu.\n", index);
    ok(flags == 0, "got %lu.\n", flags);
    ok(timestamp == 0, "got %I64d.\n", timestamp);
    ok(sample != (void *)0xdeadbeef, "got %p.\n", sample);

    hr = IMFSample_GetBufferByIndex(sample, 0, &buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_interface(buffer, &IID_IMF2DBuffer2, TRUE);
    check_interface(buffer, &IID_IMFGetService, FALSE);
    check_interface(buffer, &IID_IMFDXGIBuffer, TRUE);
    IMFMediaBuffer_Release(buffer);

    IMFSample_Release(sample);

    fail_request_sample = TRUE;


    /* video processor output stream attributes are now set with some defaults */

    hr = IMFSourceReaderEx_GetTransformForStream(reader_ex, 0, 1, NULL, &video_processor);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(video_processor->lpVtbl != &test_decoder_vtbl, "got unexpected transform\n");

    hr = IMFTransform_GetAttributes(video_processor, &attributes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value = 0xdeadbeef;
    hr = IMFAttributes_GetUINT32(attributes, &MF_SA_MINIMUM_OUTPUT_SAMPLE_COUNT, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value == 6, "got %u.\n", value);
    IMFAttributes_Release(attributes);

    hr = IMFTransform_GetOutputStreamAttributes(video_processor, 0, &attributes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value = 0xdeadbeef;
    hr = IMFAttributes_GetUINT32(attributes, &MF_SA_D3D11_BINDFLAGS, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value == 1024, "got %u.\n", value);
    IMFAttributes_Release(attributes);

    IMFTransform_Release(video_processor);


    IMFSourceReaderEx_Release(reader_ex);
    IMFSourceReader_Release(reader);
    IMFTransform_Release(test_decoder);


skip_tests:
    hr = MFTUnregisterLocal(&factory);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFDXGIDeviceManager_Release(manager);

    test_decoder_got_d3d_manager = FALSE;
    test_decoder_d3d11_aware = FALSE;
    expect_dxgi_manager = NULL;
}

START_TEST(mfplat)
{
    HRESULT hr;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    init_functions();

    test_factory();
    test_interfaces();
    test_source_reader("test.wav", false);
    test_source_reader("test.mp4", true);
    test_source_reader_from_media_source();
    test_source_reader_transforms(FALSE, FALSE);
    test_source_reader_transforms(TRUE, FALSE);
    test_source_reader_transforms(FALSE, TRUE);
    test_source_reader_transform_stream_change();
    test_source_reader_transforms_d3d9();
    test_source_reader_transforms_d3d11();
    test_reader_d3d9();
    test_sink_writer_create();
    test_sink_writer_get_object();
    test_sink_writer_add_stream();
    test_sink_writer_sample_process();

    hr = MFShutdown();
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
}
