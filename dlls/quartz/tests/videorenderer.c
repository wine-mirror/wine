/*
 * Unit tests for Video Renderer functions
 *
 * Copyright (C) 2007 Google (Lei Zhang)
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
#include "dshow.h"
#include "wine/heap.h"
#include "wine/strmbase.h"
#include "wine/test.h"

static IBaseFilter *create_video_renderer(void)
{
    IBaseFilter *filter = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_VideoRenderer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    return filter;
}

static inline BOOL compare_media_types(const AM_MEDIA_TYPE *a, const AM_MEDIA_TYPE *b)
{
    return !memcmp(a, b, offsetof(AM_MEDIA_TYPE, pbFormat))
        && !memcmp(a->pbFormat, b->pbFormat, a->cbFormat);
}

static IFilterGraph2 *create_graph(void)
{
    IFilterGraph2 *ret;
    HRESULT hr;
    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IFilterGraph2, (void **)&ret);
    ok(hr == S_OK, "Failed to create FilterGraph: %#x\n", hr);
    return ret;
}

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#x, expected %#x.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

static void test_interfaces(void)
{
    IBaseFilter *filter = create_video_renderer();
    IPin *pin;

    check_interface(filter, &IID_IBaseFilter, TRUE);
    check_interface(filter, &IID_IBasicVideo, TRUE);
    todo_wine check_interface(filter, &IID_IBasicVideo2, TRUE);
    todo_wine check_interface(filter, &IID_IDirectDrawVideo, TRUE);
    todo_wine check_interface(filter, &IID_IKsPropertySet, TRUE);
    check_interface(filter, &IID_IMediaFilter, TRUE);
    check_interface(filter, &IID_IMediaPosition, TRUE);
    check_interface(filter, &IID_IMediaSeeking, TRUE);
    check_interface(filter, &IID_IPersist, TRUE);
    check_interface(filter, &IID_IQualityControl, TRUE);
    todo_wine check_interface(filter, &IID_IQualProp, TRUE);
    check_interface(filter, &IID_IUnknown, TRUE);
    check_interface(filter, &IID_IVideoWindow, TRUE);

    check_interface(filter, &IID_IAMFilterMiscFlags, FALSE);
    check_interface(filter, &IID_IBasicAudio, FALSE);
    check_interface(filter, &IID_IDispatch, FALSE);
    check_interface(filter, &IID_IOverlay, FALSE);
    check_interface(filter, &IID_IPersistPropertyBag, FALSE);
    check_interface(filter, &IID_IPin, FALSE);
    check_interface(filter, &IID_IReferenceClock, FALSE);

    IBaseFilter_FindPin(filter, L"In", &pin);

    check_interface(pin, &IID_IMemInputPin, TRUE);
    check_interface(pin, &IID_IOverlay, TRUE);
    check_interface(pin, &IID_IPin, TRUE);
    todo_wine check_interface(pin, &IID_IPinConnection, TRUE);
    todo_wine check_interface(pin, &IID_IQualityControl, TRUE);
    check_interface(pin, &IID_IUnknown, TRUE);

    check_interface(pin, &IID_IAsyncReader, FALSE);
    check_interface(pin, &IID_IMediaPosition, FALSE);
    check_interface(pin, &IID_IMediaSeeking, FALSE);

    IPin_Release(pin);
    IBaseFilter_Release(filter);
}

static const GUID test_iid = {0x33333333};
static LONG outer_ref = 1;

static HRESULT WINAPI outer_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    if (IsEqualGUID(iid, &IID_IUnknown)
            || IsEqualGUID(iid, &IID_IBaseFilter)
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

static void test_aggregation(void)
{
    IBaseFilter *filter, *filter2;
    IUnknown *unk, *unk2;
    HRESULT hr;
    ULONG ref;

    filter = (IBaseFilter *)0xdeadbeef;
    hr = CoCreateInstance(&CLSID_VideoRenderer, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == E_NOINTERFACE, "Got hr %#x.\n", hr);
    ok(!filter, "Got interface %p.\n", filter);

    hr = CoCreateInstance(&CLSID_VideoRenderer, &test_outer, CLSCTX_INPROC_SERVER,
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

    hr = IUnknown_QueryInterface(unk, &IID_IBaseFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IBaseFilter_QueryInterface(filter, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(unk2 == (IUnknown *)0xdeadbeef, "Got unexpected IUnknown %p.\n", unk2);

    hr = IBaseFilter_QueryInterface(filter, &IID_IBaseFilter, (void **)&filter2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(filter2 == (IBaseFilter *)0xdeadbeef, "Got unexpected IBaseFilter %p.\n", filter2);

    hr = IUnknown_QueryInterface(unk, &test_iid, (void **)&unk2);
    ok(hr == E_NOINTERFACE, "Got hr %#x.\n", hr);
    ok(!unk2, "Got unexpected IUnknown %p.\n", unk2);

    hr = IBaseFilter_QueryInterface(filter, &test_iid, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(unk2 == (IUnknown *)0xdeadbeef, "Got unexpected IUnknown %p.\n", unk2);

    IBaseFilter_Release(filter);
    ref = IUnknown_Release(unk);
    ok(!ref, "Got unexpected refcount %d.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %d.\n", outer_ref);
}

static void test_enum_pins(void)
{
    IBaseFilter *filter = create_video_renderer();
    IEnumPins *enum1, *enum2;
    IPin *pins[2];
    ULONG count;
    HRESULT hr;
    ULONG ref;

    ref = get_refcount(filter);
    ok(ref == 1, "Got unexpected refcount %d.\n", ref);

    hr = IBaseFilter_EnumPins(filter, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);

    hr = IBaseFilter_EnumPins(filter, &enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ref = get_refcount(filter);
    ok(ref == 2, "Got unexpected refcount %d.\n", ref);
    ref = get_refcount(enum1);
    ok(ref == 1, "Got unexpected refcount %d.\n", ref);

    hr = IEnumPins_Next(enum1, 1, NULL, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ref = get_refcount(filter);
    ok(ref == 3, "Got unexpected refcount %d.\n", ref);
    ref = get_refcount(pins[0]);
    ok(ref == 3, "Got unexpected refcount %d.\n", ref);
    ref = get_refcount(enum1);
    ok(ref == 1, "Got unexpected refcount %d.\n", ref);
    IPin_Release(pins[0]);
    ref = get_refcount(filter);
    ok(ref == 2, "Got unexpected refcount %d.\n", ref);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum1, 1, pins, &count);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(count == 1, "Got count %u.\n", count);
    IPin_Release(pins[0]);

    hr = IEnumPins_Next(enum1, 1, pins, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(!count, "Got count %u.\n", count);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum1, 2, pins, NULL);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum1, 2, pins, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(count == 1, "Got count %u.\n", count);
    IPin_Release(pins[0]);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Clone(enum1, &enum2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Skip(enum1, 2);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumPins_Skip(enum1, 1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Skip(enum1, 1);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum2, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    IPin_Release(pins[0]);

    IEnumPins_Release(enum2);
    IEnumPins_Release(enum1);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_find_pin(void)
{
    IBaseFilter *filter = create_video_renderer();
    IEnumPins *enum_pins;
    IPin *pin, *pin2;
    HRESULT hr;
    ULONG ref;

    hr = IBaseFilter_FindPin(filter, L"input pin", &pin);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#x.\n", hr);

    hr = IBaseFilter_FindPin(filter, L"In", &pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IBaseFilter_EnumPins(filter, &enum_pins);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum_pins, 1, &pin2, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(pin == pin2, "Expected pin %p, got %p.\n", pin2, pin);
    IPin_Release(pin);
    IPin_Release(pin2);

    IEnumPins_Release(enum_pins);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_pin_info(void)
{
    IBaseFilter *filter = create_video_renderer();
    PIN_DIRECTION dir;
    PIN_INFO info;
    HRESULT hr;
    WCHAR *id;
    ULONG ref;
    IPin *pin;

    hr = IBaseFilter_FindPin(filter, L"In", &pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ref = get_refcount(filter);
    ok(ref == 2, "Got unexpected refcount %d.\n", ref);
    ref = get_refcount(pin);
    ok(ref == 2, "Got unexpected refcount %d.\n", ref);

    hr = IPin_QueryPinInfo(pin, &info);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(info.pFilter == filter, "Expected filter %p, got %p.\n", filter, info.pFilter);
    ok(info.dir == PINDIR_INPUT, "Got direction %d.\n", info.dir);
todo_wine
    ok(!wcscmp(info.achName, L"Input"), "Got name %s.\n", wine_dbgstr_w(info.achName));
    ref = get_refcount(filter);
    ok(ref == 3, "Got unexpected refcount %d.\n", ref);
    ref = get_refcount(pin);
    ok(ref == 3, "Got unexpected refcount %d.\n", ref);
    IBaseFilter_Release(info.pFilter);

    hr = IPin_QueryDirection(pin, &dir);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(dir == PINDIR_INPUT, "Got direction %d.\n", dir);

    hr = IPin_QueryId(pin, &id);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!wcscmp(id, L"In"), "Got id %s.\n", wine_dbgstr_w(id));
    CoTaskMemFree(id);

    hr = IPin_QueryInternalConnections(pin, NULL, NULL);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    IPin_Release(pin);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_media_types(void)
{
    IBaseFilter *filter = create_video_renderer();
    AM_MEDIA_TYPE *mt, req_mt = {{0}};
    VIDEOINFOHEADER vih =
    {
        {0}, {0}, 0, 0, 0,
        {sizeof(BITMAPINFOHEADER), 32, 24, 1, 0, BI_RGB}
    };
    IEnumMediaTypes *enummt;
    unsigned int i;
    HRESULT hr;
    ULONG ref;
    IPin *pin;

    static const GUID *subtype_tests[] =
    {
        &MEDIASUBTYPE_RGB8,
        &MEDIASUBTYPE_RGB565,
        &MEDIASUBTYPE_RGB24,
        &MEDIASUBTYPE_RGB32,
    };

    IBaseFilter_FindPin(filter, L"In", &pin);

    hr = IPin_EnumMediaTypes(pin, &enummt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enummt, 1, &mt, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    IEnumMediaTypes_Release(enummt);

    req_mt.majortype = MEDIATYPE_Video;
    req_mt.formattype = FORMAT_VideoInfo;
    req_mt.cbFormat = sizeof(VIDEOINFOHEADER);
    req_mt.pbFormat = (BYTE *)&vih;

    for (i = 0; i < ARRAY_SIZE(subtype_tests); ++i)
    {
        req_mt.subtype = *subtype_tests[i];
        hr = IPin_QueryAccept(pin, &req_mt);
        ok(hr == S_OK, "Got hr %#x for subtype %s.\n", hr, wine_dbgstr_guid(subtype_tests[i]));
    }

    req_mt.subtype = MEDIASUBTYPE_NULL;
    hr = IPin_QueryAccept(pin, &req_mt);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    req_mt.subtype = MEDIASUBTYPE_RGB24;

    req_mt.majortype = MEDIATYPE_NULL;
    hr = IPin_QueryAccept(pin, &req_mt);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    req_mt.majortype = MEDIATYPE_Video;

    req_mt.formattype = FORMAT_None;
    hr = IPin_QueryAccept(pin, &req_mt);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    req_mt.formattype = GUID_NULL;
    hr = IPin_QueryAccept(pin, &req_mt);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    IPin_Release(pin);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_enum_media_types(void)
{
    IBaseFilter *filter = create_video_renderer();
    IEnumMediaTypes *enum1, *enum2;
    AM_MEDIA_TYPE *mts[2];
    ULONG ref, count;
    HRESULT hr;
    IPin *pin;

    IBaseFilter_FindPin(filter, L"In", &pin);

    hr = IPin_EnumMediaTypes(pin, &enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(!count, "Got count %u.\n", count);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Clone(enum1, &enum2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 1);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enum2, 1, mts, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    IEnumMediaTypes_Release(enum1);
    IEnumMediaTypes_Release(enum2);
    IPin_Release(pin);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

struct testfilter
{
    struct strmbase_filter filter;
    struct strmbase_source source;
};

static inline struct testfilter *impl_from_BaseFilter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, filter);
}

static struct strmbase_pin *testfilter_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    struct testfilter *filter = impl_from_BaseFilter(iface);
    if (!index)
        return &filter->source.pin;
    return NULL;
}

static void testfilter_destroy(struct strmbase_filter *iface)
{
    struct testfilter *filter = impl_from_BaseFilter(iface);
    strmbase_source_cleanup(&filter->source);
    strmbase_filter_cleanup(&filter->filter);
}

static const struct strmbase_filter_ops testfilter_ops =
{
    .filter_get_pin = testfilter_get_pin,
    .filter_destroy = testfilter_destroy,
};

static HRESULT testsource_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    return S_OK;
}

static HRESULT WINAPI testsource_DecideAllocator(struct strmbase_source *iface,
        IMemInputPin *peer, IMemAllocator **allocator)
{
    return S_OK;
}

static const struct strmbase_source_ops testsource_ops =
{
    .base.pin_query_accept = testsource_query_accept,
    .base.pin_get_media_type = strmbase_pin_get_media_type,
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideAllocator = testsource_DecideAllocator,
};

static void testfilter_init(struct testfilter *filter)
{
    static const GUID clsid = {0xabacab};
    strmbase_filter_init(&filter->filter, NULL, &clsid, &testfilter_ops);
    strmbase_source_init(&filter->source, &filter->filter, L"", &testsource_ops);
}

static void test_allocator(IMemInputPin *input)
{
    IMemAllocator *req_allocator, *ret_allocator;
    ALLOCATOR_PROPERTIES props;
    HRESULT hr;

    hr = IMemInputPin_GetAllocatorRequirements(input, &props);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    hr = IMemInputPin_GetAllocator(input, &ret_allocator);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);

    if (hr == S_OK)
    {
        hr = IMemAllocator_GetProperties(ret_allocator, &props);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        ok(!props.cBuffers, "Got %d buffers.\n", props.cBuffers);
        ok(!props.cbBuffer, "Got size %d.\n", props.cbBuffer);
        ok(!props.cbAlign, "Got alignment %d.\n", props.cbAlign);
        ok(!props.cbPrefix, "Got prefix %d.\n", props.cbPrefix);

        hr = IMemInputPin_NotifyAllocator(input, ret_allocator, TRUE);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        IMemAllocator_Release(ret_allocator);
    }

    hr = IMemInputPin_NotifyAllocator(input, NULL, TRUE);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);

    CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMemAllocator, (void **)&req_allocator);

    hr = IMemInputPin_NotifyAllocator(input, req_allocator, TRUE);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMemInputPin_GetAllocator(input, &ret_allocator);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(ret_allocator == req_allocator, "Allocators didn't match.\n");

    IMemAllocator_Release(req_allocator);
    IMemAllocator_Release(ret_allocator);
}

struct frame_thread_params
{
    IMemInputPin *sink;
    IMediaSample *sample;
};

static DWORD WINAPI frame_thread(void *arg)
{
    struct frame_thread_params *params = arg;
    HRESULT hr;

    if (winetest_debug > 1) trace("%04x: Sending frame.\n", GetCurrentThreadId());
    hr = IMemInputPin_Receive(params->sink, params->sample);
    if (winetest_debug > 1) trace("%04x: Returned %#x.\n", GetCurrentThreadId(), hr);
    IMediaSample_Release(params->sample);
    heap_free(params);
    return hr;
}

static HANDLE send_frame_time(IMemInputPin *sink, REFERENCE_TIME start_time, unsigned char color)
{
    struct frame_thread_params *params = heap_alloc(sizeof(*params));
    IMemAllocator *allocator;
    REFERENCE_TIME end_time;
    IMediaSample *sample;
    HANDLE thread;
    HRESULT hr;
    BYTE *data;

    hr = IMemInputPin_GetAllocator(sink, &allocator);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMemAllocator_GetBuffer(allocator, &sample, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaSample_GetPointer(sample, &data);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    memset(data, color, 32 * 16 * 2);

    hr = IMediaSample_SetActualDataLength(sample, 32 * 16 * 2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    start_time *= 10000000;
    end_time = start_time + 10000000;
    hr = IMediaSample_SetTime(sample, &start_time, &end_time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    params->sink = sink;
    params->sample = sample;
    thread = CreateThread(NULL, 0, frame_thread, params, 0, NULL);

    IMemAllocator_Release(allocator);
    return thread;
}

static HANDLE send_frame(IMemInputPin *sink)
{
    return send_frame_time(sink, 0, 0x55); /* purple */
}

static HRESULT join_thread_(int line, HANDLE thread)
{
    DWORD ret;
    ok_(__FILE__, line)(!WaitForSingleObject(thread, 1000), "Wait failed.\n");
    GetExitCodeThread(thread, &ret);
    CloseHandle(thread);
    return ret;
}
#define join_thread(a) join_thread_(__LINE__, a)

static void test_filter_state(IMemInputPin *input, IFilterGraph2 *graph)
{
    IMemAllocator *allocator;
    IMediaControl *control;
    IMediaSample *sample;
    OAFilterState state;
    HANDLE thread;
    HRESULT hr;

    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);

    thread = send_frame(input);
    hr = join_thread(thread);
    todo_wine ok(hr == E_FAIL, "Got hr %#x.\n", hr);

    /* The renderer is not fully paused until it receives a sample. The thread
     * sending the sample blocks in IMemInputPin_Receive() until the filter is
     * stopped or run. */

    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    todo_wine ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#x.\n", hr);

    thread = send_frame(input);

    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    ok(WaitForSingleObject(thread, 100) == WAIT_TIMEOUT, "Thread should block in Receive().\n");

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = join_thread(thread);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    /* The sink will decommit our allocator for us when stopping, and recommit
     * it when pausing. */
    hr = IMemInputPin_GetAllocator(input, &allocator);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMemAllocator_GetBuffer(allocator, &sample, NULL, NULL, 0);
    todo_wine ok(hr == VFW_E_NOT_COMMITTED, "Got hr %#x.\n", hr);
    if (hr == S_OK) IMediaSample_Release(sample);

    hr = IMemAllocator_Commit(allocator);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    thread = send_frame(input);
    hr = join_thread(thread);
    todo_wine ok(hr == E_FAIL, "Got hr %#x.\n", hr);

    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    todo_wine ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#x.\n", hr);

    thread = send_frame(input);

    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    ok(WaitForSingleObject(thread, 100) == WAIT_TIMEOUT, "Thread should block in Receive().\n");

    hr = IMediaControl_Run(control);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = join_thread(thread);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    thread = send_frame(input);
    hr = join_thread(thread);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    todo_wine ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#x.\n", hr);

    thread = send_frame(input);

    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    ok(WaitForSingleObject(thread, 100) == WAIT_TIMEOUT, "Thread should block in Receive().\n");

    hr = IMediaControl_Run(control);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = join_thread(thread);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    todo_wine ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#x.\n", hr);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    todo_wine ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#x.\n", hr);

    hr = IMediaControl_Run(control);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    todo_wine ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#x.\n", hr);

    thread = send_frame(input);
    hr = join_thread(thread);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    IMemAllocator_Release(allocator);
    IMediaControl_Release(control);
}

static void test_flushing(IPin *pin, IMemInputPin *input, IFilterGraph2 *graph)
{
    IMediaControl *control;
    OAFilterState state;
    HANDLE thread;
    HRESULT hr;

    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);

    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    thread = send_frame(input);
    ok(WaitForSingleObject(thread, 100) == WAIT_TIMEOUT, "Thread should block in Receive().\n");

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_BeginFlush(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = join_thread(thread);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    thread = send_frame(input);
    hr = join_thread(thread);
    todo_wine ok(hr == E_FAIL, "Got hr %#x.\n", hr);

    hr = IPin_EndFlush(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    /* We dropped the sample we were holding, so now we need a new one... */

    hr = IMediaControl_GetState(control, 0, &state);
    todo_wine ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#x.\n", hr);

    thread = send_frame(input);
    ok(WaitForSingleObject(thread, 100) == WAIT_TIMEOUT, "Thread should block in Receive().\n");

    hr = IMediaControl_Run(control);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = join_thread(thread);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_BeginFlush(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = join_thread(send_frame(input));
    todo_wine ok(hr == E_FAIL, "Got hr %#x.\n", hr);

    hr = IPin_EndFlush(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = join_thread(send_frame(input));
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    IMediaControl_Release(control);
}

static void test_sample_time(IPin *pin, IMemInputPin *input, IFilterGraph2 *graph)
{
    IMediaControl *control;
    OAFilterState state;
    HANDLE thread;
    HRESULT hr;

    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);

    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    todo_wine ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#x.\n", hr);

    thread = send_frame_time(input, 1, 0x11); /* dark blue */

    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    ok(WaitForSingleObject(thread, 100) == WAIT_TIMEOUT, "Thread should block in Receive().\n");

    hr = IMediaControl_Run(control);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);

    ok(WaitForSingleObject(thread, 500) == WAIT_TIMEOUT, "Thread should block in Receive().\n");

    hr = join_thread(thread);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    /* Sample time is relative to the time passed to Run(). Thus a sample
     * stamped at or earlier than 1s will now be displayed immediately, because
     * that time has already passed.
     * One may manually verify that all of the frames in this function are
     * rendered, including (by adding a Sleep() after sending the frame) the
     * dark and light green frames. Thus the video renderer does not attempt to
     * drop any frames that it considers late. This remains true if the frames
     * are marked as discontinuous. */

    hr = join_thread(send_frame_time(input, 1, 0x22)); /* dark green */
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = join_thread(send_frame_time(input, 0, 0x33)); /* light green */
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = join_thread(send_frame_time(input, -2, 0x44)); /* dark red */
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    thread = send_frame_time(input, 2, 0x66); /* orange */
    ok(WaitForSingleObject(thread, 800) == WAIT_TIMEOUT, "Thread should block in Receive().\n");
    hr = join_thread(thread);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    thread = send_frame_time(input, 1000000, 0xff); /* white */
    ok(WaitForSingleObject(thread, 100) == WAIT_TIMEOUT, "Thread should block in Receive().\n");

    hr = IPin_BeginFlush(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = join_thread(thread);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IPin_EndFlush(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    thread = send_frame_time(input, 1000000, 0xff);
    ok(WaitForSingleObject(thread, 100) == WAIT_TIMEOUT, "Thread should block in Receive().\n");

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = join_thread(thread);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    IMediaControl_Release(control);
}

static unsigned int check_ec_complete(IMediaEvent *eventsrc, DWORD timeout)
{
    LONG_PTR param1, param2;
    unsigned int ret = 0;
    HRESULT hr;
    LONG code;

    while ((hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, timeout)) == S_OK)
    {
        if (code == EC_COMPLETE)
        {
            ok(param1 == S_OK, "Got param1 %#lx.\n", param1);
            ok(!param2, "Got param2 %#lx.\n", param2);
            ret++;
        }
        IMediaEvent_FreeEventParams(eventsrc, code, param1, param2);
        timeout = 0;
    }
    ok(hr == E_ABORT, "Got hr %#x.\n", hr);

    return ret;
}

static void test_eos(IPin *pin, IMemInputPin *input, IFilterGraph2 *graph)
{
    IMediaControl *control;
    IMediaEvent *eventsrc;
    OAFilterState state;
    HRESULT hr;
    BOOL ret;

    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaEvent, (void **)&eventsrc);

    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ret = check_ec_complete(eventsrc, 0);
    ok(!ret, "Got unexpected EC_COMPLETE.\n");

    hr = IPin_EndOfStream(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ret = check_ec_complete(eventsrc, 0);
    todo_wine ok(!ret, "Got unexpected EC_COMPLETE.\n");

    hr = join_thread(send_frame(input));
    todo_wine ok(hr == E_UNEXPECTED, "Got hr %#x.\n", hr);

    hr = IMediaControl_Run(control);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    ret = check_ec_complete(eventsrc, 0);
    todo_wine ok(ret == 1, "Expected EC_COMPLETE.\n");

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ret = check_ec_complete(eventsrc, 0);
    ok(!ret, "Got unexpected EC_COMPLETE.\n");

    /* We do not receive an EC_COMPLETE notification until the last sample is
     * done rendering. */

    hr = IMediaControl_Run(control);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    hr = join_thread(send_frame(input));
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ret = check_ec_complete(eventsrc, 0);
    ok(!ret, "Got unexpected EC_COMPLETE.\n");

    hr = IPin_EndOfStream(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ret = check_ec_complete(eventsrc, 0);
    todo_wine ok(!ret, "Got unexpected EC_COMPLETE.\n");
    ret = check_ec_complete(eventsrc, 1600);
    todo_wine ok(ret == 1, "Expected EC_COMPLETE.\n");

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ret = check_ec_complete(eventsrc, 0);
    ok(!ret, "Got unexpected EC_COMPLETE.\n");

    /* Test sending EOS while flushing. */

    hr = IMediaControl_Run(control);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    hr = join_thread(send_frame(input));
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_BeginFlush(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IPin_EndOfStream(pin);
    todo_wine ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    hr = IPin_EndFlush(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ret = check_ec_complete(eventsrc, 0);
    todo_wine ok(!ret, "Got unexpected EC_COMPLETE.\n");

    /* Test sending EOS and then flushing or stopping. */

    hr = IMediaControl_Run(control);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    hr = join_thread(send_frame(input));
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_EndOfStream(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ret = check_ec_complete(eventsrc, 0);
    todo_wine ok(!ret, "Got unexpected EC_COMPLETE.\n");

    hr = IPin_BeginFlush(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IPin_EndFlush(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = join_thread(send_frame(input));
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IPin_EndOfStream(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ret = check_ec_complete(eventsrc, 0);
    ok(!ret, "Got unexpected EC_COMPLETE.\n");

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ret = check_ec_complete(eventsrc, 0);
    ok(!ret, "Got unexpected EC_COMPLETE.\n");

    IMediaEvent_Release(eventsrc);
    IMediaControl_Release(control);
}

static void test_connect_pin(void)
{
    VIDEOINFOHEADER vih =
    {
        .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
        .bmiHeader.biBitCount = 16,
        .bmiHeader.biWidth = 32,
        .bmiHeader.biHeight = 16,
        .bmiHeader.biPlanes = 1,
        .bmiHeader.biCompression = BI_RGB,
    };
    AM_MEDIA_TYPE req_mt =
    {
        .majortype = MEDIATYPE_Video,
        .formattype = FORMAT_VideoInfo,
        .cbFormat = sizeof(vih),
        .pbFormat = (BYTE *)&vih,
    };
    ALLOCATOR_PROPERTIES req_props = {1, 32 * 16 * 2, 1, 0}, ret_props;
    IBaseFilter *filter = create_video_renderer();
    IFilterGraph2 *graph = create_graph();
    struct testfilter source;
    IMemAllocator *allocator;
    IMemInputPin *input;
    AM_MEDIA_TYPE mt;
    IPin *pin, *peer;
    unsigned int i;
    HRESULT hr;
    ULONG ref;

    static const GUID *subtype_tests[] =
    {
        &MEDIASUBTYPE_RGB8,
        &MEDIASUBTYPE_RGB565,
        &MEDIASUBTYPE_RGB24,
        &MEDIASUBTYPE_RGB32,
    };

    testfilter_init(&source);

    IFilterGraph2_AddFilter(graph, &source.filter.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, filter, NULL);

    IBaseFilter_FindPin(filter, L"In", &pin);

    for (i = 0; i < ARRAY_SIZE(subtype_tests); ++i)
    {
        req_mt.subtype = *subtype_tests[i];
        hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &req_mt);
        ok(hr == S_OK, "Got hr %#x for subtype %s.\n", hr, wine_dbgstr_guid(subtype_tests[i]));

        hr = IFilterGraph2_Disconnect(graph, &source.source.pin.IPin_iface);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        hr = IFilterGraph2_Disconnect(graph, pin);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
    }

    req_mt.formattype = FORMAT_None;
    hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &req_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#x.\n", hr);
    req_mt.formattype = GUID_NULL;
    hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &req_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#x.\n", hr);
    req_mt.formattype = FORMAT_VideoInfo;

    peer = (IPin *)0xdeadbeef;
    hr = IPin_ConnectedTo(pin, &peer);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#x.\n", hr);
    ok(!peer, "Got peer %p.\n", peer);

    hr = IPin_ConnectionMediaType(pin, &mt);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#x.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &req_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_ConnectedTo(pin, &peer);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(peer == &source.source.pin.IPin_iface, "Got peer %p.\n", peer);
    IPin_Release(peer);

    hr = IPin_ConnectionMediaType(pin, &mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(compare_media_types(&mt, &req_mt), "Media types didn't match.\n");

    IPin_QueryInterface(pin, &IID_IMemInputPin, (void **)&input);

    test_allocator(input);

    hr = CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMemAllocator, (void **)&allocator);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMemAllocator_SetProperties(allocator, &req_props, &ret_props);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!memcmp(&ret_props, &req_props, sizeof(req_props)), "Properties did not match.\n");
    hr = IMemInputPin_NotifyAllocator(input, allocator, TRUE);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMemAllocator_Commit(allocator);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMemInputPin_ReceiveCanBlock(input);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    test_filter_state(input, graph);
    test_flushing(pin, input, graph);
    test_sample_time(pin, input, graph);
    test_eos(pin, input, graph);

    hr = IFilterGraph2_Disconnect(graph, pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IFilterGraph2_Disconnect(graph, pin);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(source.source.pin.peer == pin, "Got peer %p.\n", peer);
    IFilterGraph2_Disconnect(graph, &source.source.pin.IPin_iface);

    peer = (IPin *)0xdeadbeef;
    hr = IPin_ConnectedTo(pin, &peer);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#x.\n", hr);
    ok(!peer, "Got peer %p.\n", peer);

    hr = IPin_ConnectionMediaType(pin, &mt);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#x.\n", hr);

    ref = IMemAllocator_Release(allocator);
    todo_wine ok(!ref, "Got outstanding refcount %d.\n", ref);
    IMemInputPin_Release(input);
    IPin_Release(pin);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IBaseFilter_Release(&source.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_unconnected_filter_state(void)
{
    IBaseFilter *filter = create_video_renderer();
    FILTER_STATE state;
    HRESULT hr;
    ULONG ref;

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == State_Stopped, "Got state %u.\n", state);

    hr = IBaseFilter_Pause(filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == State_Paused, "Got state %u.\n", state);

    hr = IBaseFilter_Run(filter, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == State_Running, "Got state %u.\n", state);

    hr = IBaseFilter_Pause(filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == State_Paused, "Got state %u.\n", state);

    hr = IBaseFilter_Stop(filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == State_Stopped, "Got state %u.\n", state);

    hr = IBaseFilter_Run(filter, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == State_Running, "Got state %u.\n", state);

    hr = IBaseFilter_Stop(filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == State_Stopped, "Got state %u.\n", state);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_overlay(void)
{
    IBaseFilter *filter = create_video_renderer();
    IOverlay *overlay;
    HRESULT hr;
    ULONG ref;
    IPin *pin;
    HWND hwnd;

    IBaseFilter_FindPin(filter, L"In", &pin);

    hr = IPin_QueryInterface(pin, &IID_IOverlay, (void **)&overlay);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hwnd = (HWND)0xdeadbeef;
    hr = IOverlay_GetWindowHandle(overlay, &hwnd);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(hwnd && hwnd != (HWND)0xdeadbeef, "Got invalid window %p.\n", hwnd);

    IOverlay_Release(overlay);
    IPin_Release(pin);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

/* try to make sure pending X events have been processed before continuing */
static void flush_events(void)
{
    int diff = 200;
    DWORD time;
    MSG msg;

    time = GetTickCount() + diff;
    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects(0, NULL, FALSE, 100, QS_ALLINPUT) == WAIT_TIMEOUT)
            break;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
            DispatchMessageA(&msg);
        diff = time - GetTickCount();
    }
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (winetest_debug > 1)
        trace("hwnd %p, msg %#x, wparam %#lx, lparam %#lx.\n", hwnd, msg, wparam, lparam);

    if (wparam == 0xdeadbeef)
        return 0;

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static void test_video_window_caption(IVideoWindow *window, HWND hwnd)
{
    WCHAR text[50];
    BSTR caption;
    HRESULT hr;

    hr = IVideoWindow_get_Caption(window, &caption);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!wcscmp(caption, L"ActiveMovie Window"), "Got caption %s.\n", wine_dbgstr_w(caption));
    SysFreeString(caption);

    GetWindowTextW(hwnd, text, ARRAY_SIZE(text));
    ok(!wcscmp(text, L"ActiveMovie Window"), "Got caption %s.\n", wine_dbgstr_w(text));

    caption = SysAllocString(L"foo");
    hr = IVideoWindow_put_Caption(window, caption);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    SysFreeString(caption);

    hr = IVideoWindow_get_Caption(window, &caption);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!wcscmp(caption, L"foo"), "Got caption %s.\n", wine_dbgstr_w(caption));
    SysFreeString(caption);

    GetWindowTextW(hwnd, text, ARRAY_SIZE(text));
    ok(!wcscmp(text, L"foo"), "Got caption %s.\n", wine_dbgstr_w(text));
}

static void test_video_window_style(IVideoWindow *window, HWND hwnd, HWND our_hwnd)
{
    HRESULT hr;
    LONG style;

    hr = IVideoWindow_get_WindowStyle(window, &style);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(style == (WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW),
            "Got style %#x.\n", style);

    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(style == (WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW),
            "Got style %#x.\n", style);

    hr = IVideoWindow_put_WindowStyle(window, style | WS_DISABLED);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);
    hr = IVideoWindow_put_WindowStyle(window, style | WS_HSCROLL);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);
    hr = IVideoWindow_put_WindowStyle(window, style | WS_VSCROLL);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);
    hr = IVideoWindow_put_WindowStyle(window, style | WS_MAXIMIZE);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);
    hr = IVideoWindow_put_WindowStyle(window, style | WS_MINIMIZE);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    hr = IVideoWindow_put_WindowStyle(window, style & ~WS_CLIPCHILDREN);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVideoWindow_get_WindowStyle(window, &style);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(style == (WS_CLIPSIBLINGS | WS_OVERLAPPEDWINDOW), "Got style %#x.\n", style);

    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(style == (WS_CLIPSIBLINGS | WS_OVERLAPPEDWINDOW), "Got style %#x.\n", style);

    ok(GetActiveWindow() == our_hwnd, "Got active window %p.\n", GetActiveWindow());

    hr = IVideoWindow_get_WindowStyleEx(window, &style);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(style == WS_EX_WINDOWEDGE, "Got style %#x.\n", style);

    style = GetWindowLongA(hwnd, GWL_EXSTYLE);
    ok(style == WS_EX_WINDOWEDGE, "Got style %#x.\n", style);

    hr = IVideoWindow_put_WindowStyleEx(window, style | WS_EX_TRANSPARENT);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVideoWindow_get_WindowStyleEx(window, &style);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(style == (WS_EX_WINDOWEDGE | WS_EX_TRANSPARENT), "Got style %#x.\n", style);

    style = GetWindowLongA(hwnd, GWL_EXSTYLE);
    ok(style == (WS_EX_WINDOWEDGE | WS_EX_TRANSPARENT), "Got style %#x.\n", style);
}

static BOOL CALLBACK top_window_cb(HWND hwnd, LPARAM ctx)
{
    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == GetCurrentProcessId() && (GetWindowLongW(hwnd, GWL_STYLE) & WS_VISIBLE))
    {
        *(HWND *)ctx = hwnd;
        return FALSE;
    }
    return TRUE;
}

static HWND get_top_window(void)
{
    HWND hwnd;
    EnumWindows(top_window_cb, (LPARAM)&hwnd);
    return hwnd;
}

static void test_video_window_state(IVideoWindow *window, HWND hwnd, HWND our_hwnd)
{
    HRESULT hr;
    LONG state;
    HWND top;

    SetWindowPos(our_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    hr = IVideoWindow_get_WindowState(window, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == SW_HIDE, "Got state %d.\n", state);

    hr = IVideoWindow_get_Visible(window, &state);
    ok(state == OAFALSE, "Got state %d.\n", state);

    ok(!IsWindowVisible(hwnd), "Window should not be visible.\n");
    ok(!IsIconic(hwnd), "Window should not be minimized.\n");
    ok(!IsZoomed(hwnd), "Window should not be maximized.\n");

    hr = IVideoWindow_put_WindowState(window, SW_SHOWNA);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVideoWindow_get_WindowState(window, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == SW_SHOW, "Got state %d.\n", state);

    hr = IVideoWindow_get_Visible(window, &state);
    ok(state == OATRUE, "Got state %d.\n", state);

    ok(IsWindowVisible(hwnd), "Window should be visible.\n");
    ok(!IsIconic(hwnd), "Window should not be minimized.\n");
    ok(!IsZoomed(hwnd), "Window should not be maximized.\n");
    ok(GetActiveWindow() == our_hwnd, "Got active window %p.\n", GetActiveWindow());
    top = get_top_window();
    ok(top == hwnd, "Got top window %p.\n", top);

    hr = IVideoWindow_put_WindowState(window, SW_MINIMIZE);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVideoWindow_get_WindowState(window, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == SW_MINIMIZE, "Got state %d.\n", state);

    hr = IVideoWindow_get_Visible(window, &state);
    ok(state == OATRUE, "Got state %d.\n", state);

    ok(IsWindowVisible(hwnd), "Window should be visible.\n");
    ok(IsIconic(hwnd), "Window should be minimized.\n");
    ok(!IsZoomed(hwnd), "Window should not be maximized.\n");
    ok(GetActiveWindow() == our_hwnd, "Got active window %p.\n", GetActiveWindow());

    hr = IVideoWindow_put_WindowState(window, SW_RESTORE);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVideoWindow_get_WindowState(window, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == SW_SHOW, "Got state %d.\n", state);

    hr = IVideoWindow_get_Visible(window, &state);
    ok(state == OATRUE, "Got state %d.\n", state);

    ok(IsWindowVisible(hwnd), "Window should be visible.\n");
    ok(!IsIconic(hwnd), "Window should not be minimized.\n");
    ok(!IsZoomed(hwnd), "Window should not be maximized.\n");
    ok(GetActiveWindow() == hwnd, "Got active window %p.\n", GetActiveWindow());

    hr = IVideoWindow_put_WindowState(window, SW_MAXIMIZE);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVideoWindow_get_WindowState(window, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == SW_MAXIMIZE, "Got state %d.\n", state);

    hr = IVideoWindow_get_Visible(window, &state);
    ok(state == OATRUE, "Got state %d.\n", state);

    ok(IsWindowVisible(hwnd), "Window should be visible.\n");
    ok(!IsIconic(hwnd), "Window should be minimized.\n");
    ok(IsZoomed(hwnd), "Window should not be maximized.\n");
    ok(GetActiveWindow() == hwnd, "Got active window %p.\n", GetActiveWindow());

    hr = IVideoWindow_put_WindowState(window, SW_RESTORE);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVideoWindow_put_WindowState(window, SW_HIDE);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVideoWindow_get_WindowState(window, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == SW_HIDE, "Got state %d.\n", state);

    hr = IVideoWindow_get_Visible(window, &state);
    ok(state == OAFALSE, "Got state %d.\n", state);

    ok(!IsWindowVisible(hwnd), "Window should not be visible.\n");
    ok(!IsIconic(hwnd), "Window should not be minimized.\n");
    ok(!IsZoomed(hwnd), "Window should not be maximized.\n");
    ok(GetActiveWindow() == our_hwnd, "Got active window %p.\n", GetActiveWindow());

    hr = IVideoWindow_put_Visible(window, OATRUE);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVideoWindow_get_WindowState(window, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == SW_SHOW, "Got state %d.\n", state);

    hr = IVideoWindow_get_Visible(window, &state);
    ok(state == OATRUE, "Got state %d.\n", state);

    ok(IsWindowVisible(hwnd), "Window should be visible.\n");
    ok(!IsIconic(hwnd), "Window should not be minimized.\n");
    ok(!IsZoomed(hwnd), "Window should not be maximized.\n");
    ok(GetActiveWindow() == hwnd, "Got active window %p.\n", GetActiveWindow());

    hr = IVideoWindow_put_Visible(window, OAFALSE);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVideoWindow_get_WindowState(window, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == SW_HIDE, "Got state %d.\n", state);

    hr = IVideoWindow_get_Visible(window, &state);
    ok(state == OAFALSE, "Got state %d.\n", state);

    ok(!IsWindowVisible(hwnd), "Window should not be visible.\n");
    ok(!IsIconic(hwnd), "Window should not be minimized.\n");
    ok(!IsZoomed(hwnd), "Window should not be maximized.\n");
    ok(GetActiveWindow() == our_hwnd, "Got active window %p.\n", GetActiveWindow());

    hr = IVideoWindow_put_WindowState(window, SW_SHOWNA);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVideoWindow_SetWindowForeground(window, TRUE);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    SetWindowPos(our_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    hr = IVideoWindow_SetWindowForeground(window, OATRUE);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(GetActiveWindow() == hwnd, "Got active window %p.\n", GetActiveWindow());
    ok(GetFocus() == hwnd, "Got focus window %p.\n", GetFocus());
    ok(GetForegroundWindow() == hwnd, "Got foreground window %p.\n", GetForegroundWindow());
    top = get_top_window();
    ok(top == hwnd, "Got top window %p.\n", top);

    hr = IVideoWindow_SetWindowForeground(window, OAFALSE);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(GetActiveWindow() == hwnd, "Got active window %p.\n", GetActiveWindow());
    ok(GetFocus() == hwnd, "Got focus window %p.\n", GetFocus());
    ok(GetForegroundWindow() == hwnd, "Got foreground window %p.\n", GetForegroundWindow());
    top = get_top_window();
    ok(top == hwnd, "Got top window %p.\n", top);

    SetWindowPos(our_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    hr = IVideoWindow_SetWindowForeground(window, OAFALSE);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(GetActiveWindow() == our_hwnd, "Got active window %p.\n", GetActiveWindow());
    ok(GetFocus() == our_hwnd, "Got focus window %p.\n", GetFocus());
    ok(GetForegroundWindow() == our_hwnd, "Got foreground window %p.\n", GetForegroundWindow());
    top = get_top_window();
    ok(top == hwnd, "Got top window %p.\n", top);
}

static void test_video_window_position(IVideoWindow *window, HWND hwnd, HWND our_hwnd)
{
    LONG left, width, top, height, expect_width, expect_height;
    RECT rect = {0, 0, 640, 480};
    HWND top_hwnd;
    HRESULT hr;

    SetWindowPos(our_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    AdjustWindowRect(&rect, GetWindowLongA(hwnd, GWL_STYLE), FALSE);
    expect_width = rect.right - rect.left;
    expect_height = rect.bottom - rect.top;

    hr = IVideoWindow_put_Left(window, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IVideoWindow_put_Top(window, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVideoWindow_get_Left(window, &left);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(left == 0, "Got left %d.\n", left);
    hr = IVideoWindow_get_Top(window, &top);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(top == 0, "Got top %d.\n", top);
    hr = IVideoWindow_get_Width(window, &width);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(width == expect_width, "Got width %d.\n", width);
    hr = IVideoWindow_get_Height(window, &height);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(height == expect_height, "Got height %d.\n", height);
    hr = IVideoWindow_GetWindowPosition(window, &left, &top, &width, &height);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(left == 0, "Got left %d.\n", left);
    ok(top == 0, "Got top %d.\n", top);
    todo_wine ok(width == expect_width, "Got width %d.\n", width);
    todo_wine ok(height == expect_height, "Got height %d.\n", height);
    GetWindowRect(hwnd, &rect);
    ok(rect.left == 0, "Got window left %d.\n", rect.left);
    ok(rect.top == 0, "Got window top %d.\n", rect.top);
    todo_wine ok(rect.right == expect_width, "Got window right %d.\n", rect.right);
    todo_wine ok(rect.bottom == expect_height, "Got window bottom %d.\n", rect.bottom);

    hr = IVideoWindow_put_Left(window, 10);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVideoWindow_get_Left(window, &left);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(left == 10, "Got left %d.\n", left);
    hr = IVideoWindow_get_Top(window, &top);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(top == 0, "Got top %d.\n", top);
    hr = IVideoWindow_get_Width(window, &width);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(width == expect_width, "Got width %d.\n", width);
    hr = IVideoWindow_get_Height(window, &height);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(height == expect_height, "Got height %d.\n", height);
    hr = IVideoWindow_GetWindowPosition(window, &left, &top, &width, &height);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(left == 10, "Got left %d.\n", left);
    ok(top == 0, "Got top %d.\n", top);
    todo_wine ok(width == expect_width, "Got width %d.\n", width);
    todo_wine ok(height == expect_height, "Got height %d.\n", height);
    GetWindowRect(hwnd, &rect);
    ok(rect.left == 10, "Got window left %d.\n", rect.left);
    ok(rect.top == 0, "Got window top %d.\n", rect.top);
    todo_wine ok(rect.right == 10 + expect_width, "Got window right %d.\n", rect.right);
    todo_wine ok(rect.bottom == expect_height, "Got window bottom %d.\n", rect.bottom);

    hr = IVideoWindow_put_Height(window, 200);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVideoWindow_get_Left(window, &left);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(left == 10, "Got left %d.\n", left);
    hr = IVideoWindow_get_Top(window, &top);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(top == 0, "Got top %d.\n", top);
    hr = IVideoWindow_get_Width(window, &width);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(width == expect_width, "Got width %d.\n", width);
    hr = IVideoWindow_get_Height(window, &height);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(height == 200, "Got height %d.\n", height);
    hr = IVideoWindow_GetWindowPosition(window, &left, &top, &width, &height);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(left == 10, "Got left %d.\n", left);
    ok(top == 0, "Got top %d.\n", top);
    todo_wine ok(width == expect_width, "Got width %d.\n", width);
    ok(height == 200, "Got height %d.\n", height);
    GetWindowRect(hwnd, &rect);
    ok(rect.left == 10, "Got window left %d.\n", rect.left);
    ok(rect.top == 0, "Got window top %d.\n", rect.top);
    todo_wine ok(rect.right == 10 + expect_width, "Got window right %d.\n", rect.right);
    ok(rect.bottom == 200, "Got window bottom %d.\n", rect.bottom);

    hr = IVideoWindow_SetWindowPosition(window, 100, 200, 300, 400);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVideoWindow_get_Left(window, &left);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(left == 100, "Got left %d.\n", left);
    hr = IVideoWindow_get_Top(window, &top);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(top == 200, "Got top %d.\n", top);
    hr = IVideoWindow_get_Width(window, &width);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(width == 300, "Got width %d.\n", width);
    hr = IVideoWindow_get_Height(window, &height);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(height == 400, "Got height %d.\n", height);
    hr = IVideoWindow_GetWindowPosition(window, &left, &top, &width, &height);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(left == 100, "Got left %d.\n", left);
    ok(top == 200, "Got top %d.\n", top);
    ok(width == 300, "Got width %d.\n", width);
    ok(height == 400, "Got height %d.\n", height);
    GetWindowRect(hwnd, &rect);
    ok(rect.left == 100, "Got window left %d.\n", rect.left);
    ok(rect.top == 200, "Got window top %d.\n", rect.top);
    ok(rect.right == 400, "Got window right %d.\n", rect.right);
    ok(rect.bottom == 600, "Got window bottom %d.\n", rect.bottom);

    ok(GetActiveWindow() == our_hwnd, "Got active window %p.\n", GetActiveWindow());
    top_hwnd = get_top_window();
    ok(top_hwnd == our_hwnd, "Got top window %p.\n", top_hwnd);
}

static void test_video_window_owner(IVideoWindow *window, HWND hwnd, HWND our_hwnd)
{
    HWND parent, top_hwnd;
    LONG style, state;
    OAHWND oahwnd;
    HRESULT hr;

    SetWindowPos(our_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    hr = IVideoWindow_get_Owner(window, &oahwnd);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!oahwnd, "Got owner %#lx.\n", oahwnd);

    parent = GetAncestor(hwnd, GA_PARENT);
    ok(parent == GetDesktopWindow(), "Got parent %p.\n", parent);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_CHILD), "Got style %#x.\n", style);

    hr = IVideoWindow_put_Owner(window, (OAHWND)our_hwnd);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVideoWindow_get_Owner(window, &oahwnd);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(oahwnd == (OAHWND)our_hwnd, "Got owner %#lx.\n", oahwnd);

    parent = GetAncestor(hwnd, GA_PARENT);
    ok(parent == our_hwnd, "Got parent %p.\n", parent);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok((style & WS_CHILD), "Got style %#x.\n", style);

    ok(GetActiveWindow() == our_hwnd, "Got active window %p.\n", GetActiveWindow());
    top_hwnd = get_top_window();
    ok(top_hwnd == our_hwnd, "Got top window %p.\n", top_hwnd);

    ShowWindow(our_hwnd, SW_HIDE);

    hr = IVideoWindow_put_Visible(window, OATRUE);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVideoWindow_get_Visible(window, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == OAFALSE, "Got state %d.\n", state);

    hr = IVideoWindow_put_Owner(window, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVideoWindow_get_Owner(window, &oahwnd);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!oahwnd, "Got owner %#lx.\n", oahwnd);

    parent = GetAncestor(hwnd, GA_PARENT);
    ok(parent == GetDesktopWindow(), "Got parent %p.\n", parent);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_CHILD), "Got style %#x.\n", style);

    ok(GetActiveWindow() == hwnd, "Got active window %p.\n", GetActiveWindow());
    top_hwnd = get_top_window();
    ok(top_hwnd == hwnd, "Got top window %p.\n", top_hwnd);

    hr = IVideoWindow_get_Visible(window, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == OATRUE, "Got state %d.\n", state);
}

struct notify_message_params
{
    IVideoWindow *window;
    HWND hwnd;
    UINT message;
};

static DWORD CALLBACK notify_message_proc(void *arg)
{
    const struct notify_message_params *params = arg;
    HRESULT hr = IVideoWindow_NotifyOwnerMessage(params->window, (OAHWND)params->hwnd, params->message, 0, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    return 0;
}

static void test_video_window_messages(IVideoWindow *window, HWND hwnd, HWND our_hwnd)
{
    struct notify_message_params params;
    unsigned int i;
    OAHWND oahwnd;
    HANDLE thread;
    HRESULT hr;
    BOOL ret;
    MSG msg;

    static UINT drain_tests[] =
    {
        WM_MOUSEACTIVATE,
        WM_NCLBUTTONDOWN,
        WM_NCLBUTTONUP,
        WM_NCLBUTTONDBLCLK,
        WM_NCRBUTTONDOWN,
        WM_NCRBUTTONUP,
        WM_NCRBUTTONDBLCLK,
        WM_NCMBUTTONDOWN,
        WM_NCMBUTTONUP,
        WM_NCMBUTTONDBLCLK,
        WM_KEYDOWN,
        WM_KEYUP,
        WM_MOUSEMOVE,
        WM_LBUTTONDOWN,
        WM_LBUTTONUP,
        WM_LBUTTONDBLCLK,
        WM_RBUTTONDOWN,
        WM_RBUTTONUP,
        WM_RBUTTONDBLCLK,
        WM_MBUTTONDOWN,
        WM_MBUTTONUP,
        WM_MBUTTONDBLCLK,
    };

    flush_events();

    hr = IVideoWindow_get_MessageDrain(window, &oahwnd);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!oahwnd, "Got window %#lx.\n", oahwnd);

    hr = IVideoWindow_put_MessageDrain(window, (OAHWND)our_hwnd);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVideoWindow_get_MessageDrain(window, &oahwnd);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(oahwnd == (OAHWND)our_hwnd, "Got window %#lx.\n", oahwnd);

    for (i = 0; i < ARRAY_SIZE(drain_tests); ++i)
    {
        SendMessageA(hwnd, drain_tests[i], 0xdeadbeef, 0);
        ret = PeekMessageA(&msg, 0, drain_tests[i], drain_tests[i], PM_REMOVE);
        ok(ret, "Expected a message.\n");
        ok(msg.hwnd == our_hwnd, "Got hwnd %p.\n", msg.hwnd);
        ok(msg.message == drain_tests[i], "Got message %#x.\n", msg.message);
        ok(msg.wParam == 0xdeadbeef, "Got wparam %#lx.\n", msg.wParam);
        ok(!msg.lParam, "Got lparam %#lx.\n", msg.lParam);
        DispatchMessageA(&msg);

        ret = PeekMessageA(&msg, 0, drain_tests[i], drain_tests[i], PM_REMOVE);
        ok(!ret, "Got unexpected message %#x.\n", msg.message);
    }

    hr = IVideoWindow_put_MessageDrain(window, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVideoWindow_put_Owner(window, (OAHWND)our_hwnd);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    flush_events();

    hr = IVideoWindow_NotifyOwnerMessage(window, (OAHWND)our_hwnd, WM_SYSCOLORCHANGE, 0, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    ret = GetQueueStatus(QS_SENDMESSAGE | QS_POSTMESSAGE);
    ok(!ret, "Got unexpected status %#x.\n", ret);

    hr = IVideoWindow_NotifyOwnerMessage(window, (OAHWND)our_hwnd, WM_SETCURSOR,
            (WPARAM)hwnd, MAKELONG(HTCLIENT, WM_MOUSEMOVE));
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    ret = GetQueueStatus(QS_SENDMESSAGE | QS_POSTMESSAGE);
    ok(!ret, "Got unexpected status %#x.\n", ret);

    params.window = window;
    params.hwnd = our_hwnd;
    params.message = WM_SYSCOLORCHANGE;
    thread = CreateThread(NULL, 0, notify_message_proc, &params, 0, NULL);
    ok(WaitForSingleObject(thread, 100) == WAIT_TIMEOUT, "Thread should block.\n");
    ret = GetQueueStatus(QS_SENDMESSAGE | QS_POSTMESSAGE);
    ok(ret == ((QS_SENDMESSAGE << 16) | QS_SENDMESSAGE), "Got unexpected status %#x.\n", ret);

    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    ok(!WaitForSingleObject(thread, 1000), "Wait timed out.\n");
    CloseHandle(thread);

    params.message = WM_SETCURSOR;
    thread = CreateThread(NULL, 0, notify_message_proc, &params, 0, NULL);
    ok(!WaitForSingleObject(thread, 1000), "Thread should not block.\n");
    CloseHandle(thread);
    ret = GetQueueStatus(QS_SENDMESSAGE | QS_POSTMESSAGE);
    ok(!ret, "Got unexpected status %#x.\n", ret);

    hr = IVideoWindow_put_Owner(window, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
}

static void test_video_window_autoshow(IVideoWindow *window, IFilterGraph2 *graph, HWND hwnd)
{
    IMediaControl *control;
    HRESULT hr;
    LONG l;

    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);

    hr = IVideoWindow_get_AutoShow(window, &l);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(l == OATRUE, "Got %d.\n", l);

    hr = IVideoWindow_put_Visible(window, OAFALSE);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IVideoWindow_get_Visible(window, &l);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(l == OATRUE, "Got %d.\n", l);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVideoWindow_get_Visible(window, &l);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(l == OATRUE, "Got %d.\n", l);

    hr = IVideoWindow_put_AutoShow(window, OAFALSE);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVideoWindow_put_Visible(window, OAFALSE);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IVideoWindow_get_Visible(window, &l);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(l == OAFALSE, "Got %d.\n", l);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    IMediaControl_Release(control);
}

static void test_video_window(void)
{
    VIDEOINFOHEADER vih =
    {
        .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
        .bmiHeader.biBitCount = 24,
        .bmiHeader.biWidth = 640,
        .bmiHeader.biHeight = 480,
        .bmiHeader.biPlanes = 1,
        .bmiHeader.biCompression = BI_RGB,
    };
    AM_MEDIA_TYPE req_mt =
    {
        .majortype = MEDIATYPE_Video,
        .subtype = MEDIASUBTYPE_RGB24,
        .formattype = FORMAT_VideoInfo,
        .cbFormat = sizeof(vih),
        .pbFormat = (BYTE *)&vih,
    };
    IFilterGraph2 *graph = create_graph();
    WNDCLASSA window_class = {0};
    struct testfilter source;
    LONG width, height, l;
    IVideoWindow *window;
    IBaseFilter *filter;
    HWND hwnd, our_hwnd;
    IOverlay *overlay;
    BSTR caption;
    HRESULT hr;
    DWORD tid;
    ULONG ref;
    IPin *pin;
    RECT rect;

    window_class.lpszClassName = "wine_test_class";
    window_class.lpfnWndProc = window_proc;
    RegisterClassA(&window_class);
    our_hwnd = CreateWindowA("wine_test_class", "test window", WS_VISIBLE | WS_OVERLAPPEDWINDOW,
            100, 200, 300, 400, NULL, NULL, NULL, NULL);
    flush_events();

    filter = create_video_renderer();
    flush_events();

    ok(GetActiveWindow() == our_hwnd, "Got active window %p.\n", GetActiveWindow());

    IBaseFilter_FindPin(filter, L"In", &pin);

    hr = IPin_QueryInterface(pin, &IID_IOverlay, (void **)&overlay);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IOverlay_GetWindowHandle(overlay, &hwnd);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    if (winetest_debug > 1) trace("ours %p, theirs %p\n", our_hwnd, hwnd);
    GetWindowRect(hwnd, &rect);

    tid = GetWindowThreadProcessId(hwnd, NULL);
    ok(tid == GetCurrentThreadId(), "Expected tid %#x, got %#x.\n", GetCurrentThreadId(), tid);

    hr = IBaseFilter_QueryInterface(filter, &IID_IVideoWindow, (void **)&window);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVideoWindow_get_Caption(window, &caption);
    todo_wine ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#x.\n", hr);

    hr = IVideoWindow_get_WindowStyle(window, &l);
    todo_wine ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#x.\n", hr);

    hr = IVideoWindow_get_AutoShow(window, &l);
    todo_wine ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#x.\n", hr);

    testfilter_init(&source);
    IFilterGraph2_AddFilter(graph, &source.filter.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, filter, NULL);
    hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &req_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    ok(GetActiveWindow() == our_hwnd, "Got active window %p.\n", GetActiveWindow());

    test_video_window_caption(window, hwnd);
    test_video_window_style(window, hwnd, our_hwnd);
    test_video_window_state(window, hwnd, our_hwnd);
    test_video_window_position(window, hwnd, our_hwnd);
    test_video_window_autoshow(window, graph, hwnd);
    test_video_window_owner(window, hwnd, our_hwnd);
    test_video_window_messages(window, hwnd, our_hwnd);

    hr = IVideoWindow_put_FullScreenMode(window, OATRUE);
    todo_wine ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    hr = IVideoWindow_get_FullScreenMode(window, &l);
    todo_wine ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    hr = IVideoWindow_GetMinIdealImageSize(window, &width, &height);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(width == 640, "Got width %d.\n", width);
    ok(height == 480, "Got height %d.\n", height);

    hr = IVideoWindow_GetMaxIdealImageSize(window, &width, &height);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(width == 640, "Got width %d.\n", width);
    ok(height == 480, "Got height %d.\n", height);

    IFilterGraph2_Release(graph);
    IVideoWindow_Release(window);
    IOverlay_Release(overlay);
    IPin_Release(pin);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IBaseFilter_Release(&source.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    DestroyWindow(our_hwnd);
}

START_TEST(videorenderer)
{
    CoInitialize(NULL);

    test_interfaces();
    test_aggregation();
    test_enum_pins();
    test_find_pin();
    test_pin_info();
    test_media_types();
    test_enum_media_types();
    test_unconnected_filter_state();
    test_connect_pin();
    test_overlay();
    test_video_window();

    CoUninitialize();
}
