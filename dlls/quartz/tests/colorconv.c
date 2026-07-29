/*
 * Color Space Converter filter unit tests
 *
 * Copyright 2026 Brendan McGrath
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
#include "wine/fil_data.h"
#include "wine/strmbase.h"
#include "wine/test.h"

#include <control.h>
#include <uuids.h>

#define compare_media_types(got, expected) compare_media_types_(__LINE__, got, expected)

static void compare_media_types_(unsigned int line, const AM_MEDIA_TYPE *got, const AM_MEDIA_TYPE *expected)
{
    VIDEOINFO *got_video_info_ptr, *expected_video_info_ptr;

    ok_(__FILE__, line)(IsEqualGUID(&got->majortype, &expected->majortype), "Got major type %s.\n",
            wine_dbgstr_guid(&got->majortype));
    ok_(__FILE__, line)(
            IsEqualGUID(&got->subtype, &expected->subtype), "Got subtype %s.\n", wine_dbgstr_guid(&got->subtype));
    ok_(__FILE__, line)(IsEqualGUID(&got->formattype, &expected->formattype), "Got format type %s.\n",
            wine_dbgstr_guid(&got->formattype));

    ok_(__FILE__, line)(got->lSampleSize == expected->lSampleSize, "Got sample size %lu.\n", got->lSampleSize);
    ok_(__FILE__, line)(got->bFixedSizeSamples == expected->bFixedSizeSamples, "Got bFixedSizeSamples %d.\n",
            got->bFixedSizeSamples);
    ok_(__FILE__, line)(got->bTemporalCompression == expected->bTemporalCompression, "Got bTemporalCompression %d.\n",
            got->bTemporalCompression);
    ok_(__FILE__, line)(got->pUnk == expected->pUnk, "Got pUnk %p.\n", got->pUnk);

    ok_(__FILE__, line)(got->cbFormat == expected->cbFormat, "Got cbFormat %lu.\n", got->cbFormat);
    if (expected->pbFormat == NULL)
    {
        ok_(__FILE__, line)(got->pbFormat == NULL, "Expected pbFormat to be NULL.\n");
        return;
    }

    ok_(__FILE__, line)(got->pbFormat != NULL, "Expected a pbFormat value.\n");

    got_video_info_ptr = (VIDEOINFO *)got->pbFormat;
    expected_video_info_ptr = (VIDEOINFO *)expected->pbFormat;
    ok_(__FILE__, line)(got_video_info_ptr->bmiHeader.biSize == expected_video_info_ptr->bmiHeader.biSize,
            "Got size %lu.\n", got_video_info_ptr->bmiHeader.biSize);
    ok_(__FILE__, line)(got_video_info_ptr->bmiHeader.biWidth == expected_video_info_ptr->bmiHeader.biWidth,
            "Got width %ld.\n", got_video_info_ptr->bmiHeader.biWidth);
    ok_(__FILE__, line)(got_video_info_ptr->bmiHeader.biHeight == expected_video_info_ptr->bmiHeader.biHeight,
            "Got height %ld.\n", got_video_info_ptr->bmiHeader.biHeight);
    ok_(__FILE__, line)(got_video_info_ptr->bmiHeader.biPlanes == expected_video_info_ptr->bmiHeader.biPlanes,
            "Got planes %d.\n", got_video_info_ptr->bmiHeader.biPlanes);
    ok_(__FILE__, line)(got_video_info_ptr->bmiHeader.biBitCount == expected_video_info_ptr->bmiHeader.biBitCount,
            "Got bitcount %d.\n", got_video_info_ptr->bmiHeader.biBitCount);
    ok_(__FILE__, line)(got_video_info_ptr->bmiHeader.biCompression == expected_video_info_ptr->bmiHeader.biCompression,
            "Got compression %lx.\n", got_video_info_ptr->bmiHeader.biCompression);

    ok_(__FILE__, line)(got_video_info_ptr->bmiHeader.biSizeImage == expected_video_info_ptr->bmiHeader.biSizeImage,
            "Got image size %lu.\n", got_video_info_ptr->bmiHeader.biSizeImage);

    ok_(__FILE__, line)(got_video_info_ptr->rcSource.left == expected_video_info_ptr->rcSource.left,
            "Got source left %ld.\n", got_video_info_ptr->rcSource.left);
    ok_(__FILE__, line)(got_video_info_ptr->rcSource.top == expected_video_info_ptr->rcSource.top,
            "Got source top %ld.\n", got_video_info_ptr->rcSource.top);
    ok_(__FILE__, line)(got_video_info_ptr->rcSource.right == expected_video_info_ptr->rcSource.right,
            "Got source right %ld.\n", got_video_info_ptr->rcSource.right);
    ok_(__FILE__, line)(got_video_info_ptr->rcSource.bottom == expected_video_info_ptr->rcSource.bottom,
            "Got source bottom %ld.\n", got_video_info_ptr->rcSource.bottom);

    ok_(__FILE__, line)(got_video_info_ptr->rcTarget.left == expected_video_info_ptr->rcTarget.left,
            "Got target left %ld.\n", got_video_info_ptr->rcTarget.left);
    ok_(__FILE__, line)(got_video_info_ptr->rcTarget.top == expected_video_info_ptr->rcTarget.top,
            "Got target top %ld.\n", got_video_info_ptr->rcTarget.top);
    ok_(__FILE__, line)(got_video_info_ptr->rcTarget.right == expected_video_info_ptr->rcTarget.right,
            "Got target right %ld.\n", got_video_info_ptr->rcTarget.right);
    ok_(__FILE__, line)(got_video_info_ptr->rcTarget.bottom == expected_video_info_ptr->rcTarget.bottom,
            "Got target bottom %ld.\n", got_video_info_ptr->rcTarget.bottom);
}

struct image_data
{
    DWORD size;
    BYTE data[];
};

static struct image_data *create_image(BOOL is_32bit, BOOL is_flipped, DWORD width, DWORD height)
{
    static const struct
    {
        BYTE r;
        BYTE g;
        BYTE b;
    }
    color[8] =
    {
        { 0xFF, 0xFF, 0xFF }, /* white */
        { 0xFF, 0xFF, 0x00 }, /* yellow */
        { 0x00, 0xFF, 0xFF }, /* cyan */
        { 0x00, 0xFF, 0x00 }, /* green */
        { 0xFF, 0x00, 0xFF }, /* magenta */
        { 0xFF, 0x00, 0x00 }, /* red */
        { 0x00, 0x00, 0xFF }, /* blue */
        { 0x00, 0x00, 0x00 }, /* black */
    };

    struct image_data *image_data;
    BYTE pixel_size_in_bytes;
    BYTE *line, *ptr;
    LONG stride;
    DWORD size;
    int i, j;

    pixel_size_in_bytes = (is_32bit ? 4 : 3);
    stride = width * pixel_size_in_bytes;
    size = height * stride;

    image_data = calloc(1, sizeof(*image_data) + size);
    image_data->size = size;
    line = image_data->data;

    if (is_flipped)
    {
        line += (height - 1) * stride;
        stride = -stride;
    }

    for (i = 0; i < 240; i++)
    {
        ptr = line;
        for (j = 0; j < 240; j++)
        {
            *ptr++ = color[i / 30].b;
            *ptr++ = color[i / 30].g;
            *ptr++ = color[i / 30].r;
            if (is_32bit)
                ptr++;
        }
        line += stride;
    }

    return image_data;
}

struct mem_allocator
{
    IMemAllocator IMemAllocator_iface;
    LONG refcount;

    BOOL commited;
    BOOL second_decommit;
    BOOL expect_get_buffer;
    BOOL expect_get_media_type;
    BOOL expect_set_time;
    BOOL media_type_checked;
    BOOL send_media_type;

    IMediaSample IMediaSample_iface;
    LONG sample_refcount;
    BYTE data[640 * 480 * 4];
    LONG size;
    LONG length;
    BOOL sync_point;
    BOOL preroll;
    BOOL discontinuity;

    LONGLONG media_time_start, media_time_end;
    REFERENCE_TIME time_start, time_end;
    BOOL mts_set, mte_set, ts_set, te_set;
    BOOL test_qc;

    const struct strmbase_sink *sink;
};

static struct mem_allocator *mem_allocator_from_IMediaSample(IMediaSample *iface)
{
    return CONTAINING_RECORD(iface, struct mem_allocator, IMediaSample_iface);
}

static HRESULT WINAPI media_sample_QueryInterface(IMediaSample *iface, REFIID iid, void **out)
{
    struct mem_allocator *allocator = mem_allocator_from_IMediaSample(iface);
    return IMemAllocator_QueryInterface(&allocator->IMemAllocator_iface, iid, out);
}

static ULONG WINAPI media_sample_AddRef(IMediaSample *iface)
{
    struct mem_allocator *allocator = mem_allocator_from_IMediaSample(iface);
    InterlockedIncrement(&allocator->sample_refcount);
    return IMemAllocator_AddRef(&allocator->IMemAllocator_iface);
}

static ULONG WINAPI media_sample_Release(IMediaSample *iface)
{
    struct mem_allocator *allocator = mem_allocator_from_IMediaSample(iface);
    LONG refcount;

    refcount = InterlockedDecrement(&allocator->sample_refcount);

    if (!refcount)
    {
        allocator->mts_set = allocator->mte_set = FALSE;
        allocator->ts_set = allocator->te_set = FALSE;
        allocator->discontinuity = FALSE;
        allocator->preroll = FALSE;
        allocator->sync_point = FALSE;
        allocator->time_start = allocator->time_end = 0;
        allocator->media_time_start = allocator->media_time_end = 0;
        allocator->test_qc = FALSE;
        allocator->send_media_type = FALSE;
    }

    return IMemAllocator_Release(&allocator->IMemAllocator_iface);
}

static HRESULT WINAPI media_sample_GetPointer(IMediaSample *iface, BYTE **data)
{
    struct mem_allocator *allocator = mem_allocator_from_IMediaSample(iface);

    *data = allocator->data;

    return S_OK;
}

static LONG WINAPI media_sample_GetSize(IMediaSample *iface)
{
    struct mem_allocator *allocator = mem_allocator_from_IMediaSample(iface);

    return allocator->size;
}

static HRESULT WINAPI media_sample_GetTime(IMediaSample *iface, REFERENCE_TIME *start, REFERENCE_TIME *end)
{
    ok(0, "Unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI media_sample_SetTime(IMediaSample *iface, REFERENCE_TIME *start, REFERENCE_TIME *end)
{
    struct mem_allocator *allocator = mem_allocator_from_IMediaSample(iface);

    todo_wine_if(!allocator->expect_set_time)
    ok(allocator->expect_set_time, "Unexpected call to IMediaSample::SetTime.\n");

    if ((allocator->ts_set = !!start))
        allocator->time_start = *start;

    if ((allocator->te_set = !!end))
        allocator->time_end = *end;

    return S_OK;
}

static HRESULT WINAPI media_sample_IsSyncPoint(IMediaSample *iface)
{
    ok(0, "Unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI media_sample_SetSyncPoint(IMediaSample *iface, BOOL sync_point)
{
    struct mem_allocator *allocator = mem_allocator_from_IMediaSample(iface);

    allocator->sync_point = sync_point;

    return S_OK;
}

static HRESULT WINAPI media_sample_IsPreroll(IMediaSample *iface)
{
    ok(0, "Unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI media_sample_SetPreroll(IMediaSample *iface, BOOL preroll)
{
    struct mem_allocator *allocator = mem_allocator_from_IMediaSample(iface);

    allocator->preroll = preroll;

    return S_OK;
}

static LONG WINAPI media_sample_GetActualDataLength(IMediaSample *iface)
{
    struct mem_allocator *allocator = mem_allocator_from_IMediaSample(iface);

    return allocator->length;
}

static HRESULT WINAPI media_sample_SetActualDataLength(IMediaSample *iface, LONG length)
{
    struct mem_allocator *allocator = mem_allocator_from_IMediaSample(iface);

    allocator->length = length;

    return S_OK;
}

static HRESULT WINAPI media_sample_GetMediaType(IMediaSample *iface, AM_MEDIA_TYPE **ret_mt)
{
    struct mem_allocator *allocator = mem_allocator_from_IMediaSample(iface);

    ok(allocator->expect_get_media_type, "Unexpected call to IMediaSample::GetMediaType.\n");
    allocator->media_type_checked = TRUE;
    if (allocator->send_media_type)
    {
        *ret_mt = CoTaskMemAlloc(sizeof(**ret_mt));
        memset(*ret_mt, 0, sizeof(**ret_mt));
        CopyMediaType(*ret_mt, &allocator->sink->pin.mt);
        return S_OK;
    }

    *ret_mt = NULL;
    return S_FALSE;
}

static HRESULT WINAPI media_sample_SetMediaType(IMediaSample *iface, AM_MEDIA_TYPE *mt)
{
    ok(0, "Unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI media_sample_IsDiscontinuity(IMediaSample *iface)
{
    ok(0, "Unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI media_sample_SetDiscontinuity(IMediaSample *iface, BOOL discontinuity)
{
    struct mem_allocator *allocator = mem_allocator_from_IMediaSample(iface);

    allocator->discontinuity = discontinuity;

    return S_OK;
}

static HRESULT WINAPI media_sample_GetMediaTime(IMediaSample *iface, LONGLONG *start, LONGLONG *end)
{
    ok(0, "Unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI media_sample_SetMediaTime(IMediaSample *iface, LONGLONG *start, LONGLONG *end)
{
    struct mem_allocator *allocator = mem_allocator_from_IMediaSample(iface);

    if ((allocator->mts_set = !!start))
        allocator->media_time_start = *start;

    if ((allocator->mte_set = !!end))
        allocator->media_time_end = *end;

    return S_OK;
}

static struct IMediaSampleVtbl media_sample_vtbl =
{
    media_sample_QueryInterface,
    media_sample_AddRef,
    media_sample_Release,
    media_sample_GetPointer,
    media_sample_GetSize,
    media_sample_GetTime,
    media_sample_SetTime,
    media_sample_IsSyncPoint,
    media_sample_SetSyncPoint,
    media_sample_IsPreroll,
    media_sample_SetPreroll,
    media_sample_GetActualDataLength,
    media_sample_SetActualDataLength,
    media_sample_GetMediaType,
    media_sample_SetMediaType,
    media_sample_IsDiscontinuity,
    media_sample_SetDiscontinuity,
    media_sample_GetMediaTime,
    media_sample_SetMediaTime,
};

static struct mem_allocator *mem_allocator_from_IMemAllocator(IMemAllocator *iface)
{
    return CONTAINING_RECORD(iface, struct mem_allocator, IMemAllocator_iface);
}

static HRESULT WINAPI mem_allocator_QueryInterface(IMemAllocator *iface, REFIID iid, void **out)
{
    if (IsEqualGUID(iid, &IID_IMemAllocator) || IsEqualGUID(iid, &IID_IUnknown))
    {
        *out = iface;
    }
    else
    {
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI mem_allocator_AddRef(IMemAllocator *iface)
{
    struct mem_allocator *allocator = mem_allocator_from_IMemAllocator(iface);
    ULONG refcount;

    refcount = InterlockedIncrement(&allocator->refcount);

    return refcount;
}

static ULONG WINAPI mem_allocator_Release(IMemAllocator *iface)
{
    struct mem_allocator *allocator = mem_allocator_from_IMemAllocator(iface);
    ULONG refcount;

    refcount = InterlockedDecrement(&allocator->refcount);

    if (!refcount)
        free(allocator);

    return refcount;
}

static HRESULT WINAPI mem_allocator_SetProperties(
        IMemAllocator *iface, ALLOCATOR_PROPERTIES *req_props, ALLOCATOR_PROPERTIES *ret_props)
{
    *ret_props = *req_props;

    return S_OK;
}

static HRESULT WINAPI mem_allocator_GetProperties(IMemAllocator *iface, ALLOCATOR_PROPERTIES *props)
{
    ok(0, "Unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI mem_allocator_Commit(IMemAllocator *iface)
{
    struct mem_allocator *allocator = mem_allocator_from_IMemAllocator(iface);

    allocator->commited = TRUE;

    return S_OK;
}

static HRESULT WINAPI mem_allocator_Decommit(IMemAllocator *iface)
{
    struct mem_allocator *allocator = mem_allocator_from_IMemAllocator(iface);

    if (!allocator->commited)
        allocator->second_decommit = TRUE;

    allocator->commited = FALSE;

    return S_OK;
}

static HRESULT WINAPI mem_allocator_GetBuffer(
        IMemAllocator *iface, IMediaSample **ret_sample, REFERENCE_TIME *start, REFERENCE_TIME *end, DWORD flags)
{
    struct mem_allocator *allocator = mem_allocator_from_IMemAllocator(iface);

    todo_wine_if(!allocator->expect_get_buffer)
    ok(allocator->expect_get_buffer, "Not expecting call to IMemAllocator::GetBuffer.\n");
    IMediaSample_AddRef(*ret_sample = &allocator->IMediaSample_iface);

    return S_OK;
}

static HRESULT WINAPI mem_allocator_ReleaseBuffer(IMemAllocator *iface, IMediaSample *sample)
{
    ok(0, "Unexpected call\n");
    return E_NOTIMPL;
}

static IMemAllocatorVtbl mem_allocator_vtbl =
{
    mem_allocator_QueryInterface,
    mem_allocator_AddRef,
    mem_allocator_Release,
    mem_allocator_SetProperties,
    mem_allocator_GetProperties,
    mem_allocator_Commit,
    mem_allocator_Decommit,
    mem_allocator_GetBuffer,
    mem_allocator_ReleaseBuffer,
};

static struct mem_allocator *create_mem_allocator(void)
{
    struct mem_allocator *allocator;

    allocator = calloc(1, sizeof(*allocator));
    allocator->refcount = 1;
    allocator->IMemAllocator_iface.lpVtbl = &mem_allocator_vtbl;
    allocator->IMediaSample_iface.lpVtbl = &media_sample_vtbl;

    return allocator;
}

struct testfilter
{
    struct strmbase_filter filter;
    struct strmbase_source source;
    struct strmbase_sink sink;

    const GUID *wanted_subtype;
    IMediaSample *sample;

    HRESULT can_block;
    unsigned int got_new_segment, got_eos, got_begin_flush, got_end_flush;
};

static struct testfilter *testfilter_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, filter);
}

static struct strmbase_pin *testfilter_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    struct testfilter *filter = testfilter_from_strmbase_filter(iface);
    if (!index)
        return &filter->source.pin;
    else if (index == 1)
        return &filter->sink.pin;
    return NULL;
}

static void testfilter_destroy(struct strmbase_filter *iface)
{
    struct testfilter *filter = testfilter_from_strmbase_filter(iface);
    strmbase_source_cleanup(&filter->source);
    strmbase_sink_cleanup(&filter->sink);
    strmbase_filter_cleanup(&filter->filter);
}

static const struct strmbase_filter_ops testfilter_ops =
{
    .filter_get_pin = testfilter_get_pin,
    .filter_destroy = testfilter_destroy,
};

static HRESULT WINAPI peer_source_DecideAllocator(
        struct strmbase_source *iface, IMemInputPin *peer, IMemAllocator **allocator)
{
    return S_OK;
}

static HRESULT peer_source_query_accept(struct strmbase_pin *pin, const AM_MEDIA_TYPE *mt)
{
    struct testfilter *filter = testfilter_from_strmbase_filter(pin->filter);

    return (!filter->wanted_subtype || IsEqualGUID(&mt->subtype, filter->wanted_subtype)) ? S_OK : S_FALSE;
}

static const struct strmbase_source_ops peer_source_ops =
{
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideAllocator = peer_source_DecideAllocator,
    .base.pin_query_accept = peer_source_query_accept,
};

static HRESULT peer_sink_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct testfilter *filter = testfilter_from_strmbase_filter(iface->filter);

    if (IsEqualGUID(iid, &IID_IMemInputPin))
        *out = &filter->sink.IMemInputPin_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static DWORD WINAPI call_qc_notify(void *ptr)
{
    Quality q = { Famine, 2000, -10000000, 10000000 };
    struct testfilter *filter = ptr;
    IQualityControl *qc;
    HRESULT hr;

    hr = IPin_QueryInterface(filter->sink.pin.peer, &IID_IQualityControl, (void **)&qc);
    todo_wine
    ok(hr == S_OK, "QualityControl not implemented.\n");
    if (hr == S_OK)
    {
        /* don't worry too much about what it returns, just check that it doesn't deadlock */
        IQualityControl_Notify(qc, &filter->filter.IBaseFilter_iface, q);
        IQualityControl_Release(qc);
    }

    return 0;
}

static HRESULT WINAPI peer_sink_Receive(struct strmbase_sink *iface, IMediaSample *sample)
{
    struct testfilter *filter = testfilter_from_strmbase_filter(iface->pin.filter);
    struct mem_allocator *mem_allocator = mem_allocator_from_IMediaSample(sample);

    ok(sample != NULL, "Get Receive with a NULL sample.\n");
    ok(filter->sample == NULL, "Got Receive when we already have a sample.\n");
    IMediaSample_AddRef(filter->sample = sample);

    if (mem_allocator->test_qc)
    {
        HANDLE h = CreateThread(NULL, 0, call_qc_notify, filter, 0, NULL);
        ok(WaitForSingleObject(h, 1000) == WAIT_OBJECT_0, "Didn't expect deadlock.\n");
        CloseHandle(h);
    }

    return S_OK;
}

static HRESULT testsink_new_segment(struct strmbase_sink *iface, REFERENCE_TIME start, REFERENCE_TIME stop, double rate)
{
    struct testfilter *filter = testfilter_from_strmbase_filter(iface->pin.filter);
    ++filter->got_new_segment;
    ok(start == 10000, "Got start %I64d.\n", start);
    ok(stop == 20000, "Got stop %I64d.\n", stop);
    ok(rate == 1.0, "Got rate %.16e.\n", rate);
    return S_OK;
}

static HRESULT testsink_eos(struct strmbase_sink *iface)
{
    struct testfilter *filter = testfilter_from_strmbase_filter(iface->pin.filter);
    ++filter->got_eos;
    return S_OK;
}

static HRESULT testsink_begin_flush(struct strmbase_sink *iface)
{
    struct testfilter *filter = testfilter_from_strmbase_filter(iface->pin.filter);
    ++filter->got_begin_flush;
    return S_OK;
}

static HRESULT testsink_end_flush(struct strmbase_sink *iface)
{
    struct testfilter *filter = testfilter_from_strmbase_filter(iface->pin.filter);
    ++filter->got_end_flush;
    return S_OK;
}

static HRESULT peer_sink_receive_can_block(struct strmbase_sink *iface)
{
    struct testfilter *filter = testfilter_from_strmbase_filter(iface->pin.filter);

    return filter->can_block;
}

static const struct strmbase_sink_ops peer_sink_ops =
{
    .base.pin_query_interface = peer_sink_query_interface,
    .pfnReceive = peer_sink_Receive,
    .sink_new_segment = testsink_new_segment,
    .sink_eos = testsink_eos,
    .sink_begin_flush = testsink_begin_flush,
    .sink_end_flush = testsink_end_flush,
    .sink_receive_can_block = peer_sink_receive_can_block,
};

static struct testfilter *create_testfilter(IMemAllocator *mem_allocator)
{
    static const GUID clsid = { 0xabacab };

    struct testfilter *testfilter;

    testfilter = calloc(1, sizeof(*testfilter));
    strmbase_filter_init(&testfilter->filter, NULL, &clsid, &testfilter_ops);

    strmbase_sink_init(&testfilter->sink, &testfilter->filter, L"In", &peer_sink_ops, mem_allocator);
    wcscpy(testfilter->sink.pin.name, L"Input");

    strmbase_source_init(&testfilter->source, &testfilter->filter, L"Out", &peer_source_ops);
    wcscpy(testfilter->source.pin.name, L"XForm Out");

    return testfilter;
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

static HRESULT create_filter_graph(IFilterGraph **graph)
{
    return CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IFilterGraph, (void **)graph);
}

static HRESULT create_color_conv(IBaseFilter **filter)
{
    return CoCreateInstance(&CLSID_Colour, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (void **)filter);
}

static HRESULT create_filter_mapper(IFilterMapper2 **mapper)
{
    return CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER, &IID_IFilterMapper2, (void **)mapper);
}

static HRESULT create_color_conv_property_bag(IPropertyBag **property_bag)
{
    IEnumMoniker *moniker_enum;
    ICreateDevEnum *dev_enum;
    IMoniker *moniker;
    VARIANT var;
    HRESULT hr;
    GUID clsid;

    hr = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, &IID_ICreateDevEnum, (void **)&dev_enum);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = ICreateDevEnum_CreateClassEnumerator(dev_enum, &CLSID_LegacyAmFilterCategory, &moniker_enum, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    VariantInit(&var);

    while ((hr = IEnumMoniker_Next(moniker_enum, 1, &moniker, NULL)) == S_OK)
    {
        hr = IMoniker_BindToStorage(moniker, NULL, NULL, &IID_IPropertyBag, (void **)property_bag);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        hr = IPropertyBag_Read(*property_bag, L"CLSID", &var, 0);

        if (hr == S_OK)
            IIDFromString(var.bstrVal, &clsid);

        VariantClear(&var);
        IMoniker_Release(moniker);

        if (hr == S_OK && IsEqualGUID(&clsid, &CLSID_Colour))
            break;

        IPropertyBag_Release(*property_bag);
    }

    if (hr != S_OK)
        *property_bag = NULL;

    return hr;
}

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

static const struct
{
    const GUID *guid;
    DWORD compression;
    WORD bitcount;
    ULONG cbFormat;
}
subtypes[] =
{
    { &MEDIASUBTYPE_ARGB32, BI_RGB, 32, sizeof(VIDEOINFOHEADER) },
    { &MEDIASUBTYPE_RGB32, BI_RGB, 32, sizeof(VIDEOINFOHEADER) },
    { &MEDIASUBTYPE_RGB24, BI_RGB, 24, sizeof(VIDEOINFOHEADER) },
    { &MEDIASUBTYPE_RGB565, BI_BITFIELDS, 16, sizeof(VIDEOINFOHEADER) + sizeof(DWORD[3]) /* dwBitMasks */ },
    { &MEDIASUBTYPE_RGB555, BI_BITFIELDS, 16, sizeof(VIDEOINFOHEADER) + sizeof(DWORD[3]) /* dwBitMasks */ },
    { &MEDIASUBTYPE_RGB8, BI_RGB, 8, sizeof(VIDEOINFOHEADER) + sizeof(RGBQUAD[256]) /* bmiColors */ },
};

static void test_registration(void)
{
    REGFILTER2 **ret, *reg_filter;
    const REGPINTYPES *pin_types;
    IAMFilterData *filter_data;
    IPropertyBag *property_bag;
    const REGFILTERPINS2 *pin;
    IFilterMapper2 *mapper;
    VARIANT var;
    HRESULT hr;
    BYTE *data;
    ULONG len;
    int i, j;

    hr = create_color_conv_property_bag(&property_bag);

    if (hr != S_OK)
    {
        skip("Skipping registration tests.\n");
        return;
    }

    VariantInit(&var);
    hr = IPropertyBag_Read(property_bag, L"FilterData", &var, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = create_filter_mapper(&mapper);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterMapper2_QueryInterface(mapper, &IID_IAMFilterData, (void **)&filter_data);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = SafeArrayAccessData(var.parray, (void **)&data);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    len = var.parray->rgsabound[0].cElements;
    ok(len == 368, "Got len %ld.\n", len);

    hr = IAMFilterData_ParseFilterData(filter_data, data, len, (BYTE **)&ret);
    reg_filter = *ret;
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(reg_filter->dwVersion == 2, "Got dwVersion %lu.\n", reg_filter->dwVersion);
    ok(reg_filter->dwMerit == MERIT_UNLIKELY + 1, "Got merit %#lx.\n", reg_filter->dwMerit);
    ok(reg_filter->cPins2 == 2, "Got cPins2 %lu.\n", reg_filter->cPins2);
    ok(reg_filter->rgPins2 != NULL, "Expected rgPins2 to be non-NULL.\n");

    for (i = 0; i < reg_filter->cPins2; i++)
    {
        winetest_push_context("pin %d", i);

        pin = reg_filter->rgPins2 + i;
        ok(pin->dwFlags == (i ? REG_PINFLAG_B_OUTPUT : 0), "Got dwFlags %#lx.\n", pin->dwFlags);
        ok(pin->cInstances == 0, "Got cInstances %u.\n", pin->cInstances);
        ok(pin->nMediaTypes == ARRAY_SIZE(subtypes), "Got nMediaTypes %u.\n", pin->nMediaTypes);
        ok(pin->lpMediaType != NULL, "Expected lpMediaType to be non-NULL.\n");
        ok(pin->nMediums == 0, "Got nMediums %u.\n", pin->nMediums);
        ok(pin->clsPinCategory == NULL, "Expected clsPinCategory to be NULL.\n");

        for (j = 0; j < ARRAY_SIZE(subtypes); j++)
        {
            winetest_push_context("media type %d", j);

            pin_types = pin->lpMediaType + j;
            ok(IsEqualGUID(pin_types->clsMajorType, &MEDIATYPE_Video), "Got major type %s.\n",
                    wine_dbgstr_guid(pin_types->clsMajorType));
            ok(IsEqualGUID(pin_types->clsMinorType, subtypes[ARRAY_SIZE(subtypes) - j - 1].guid),
                    "Got minor type %s.\n", wine_dbgstr_guid(pin_types->clsMinorType));

            winetest_pop_context();
        }

        winetest_pop_context();
    }

    CoTaskMemFree(reg_filter);

    hr = SafeArrayUnaccessData(var.parray);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = VariantClear(&var);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IAMFilterData_Release(filter_data);
    IFilterMapper2_Release(mapper);
    IPropertyBag_Release(property_bag);
}

static void test_interfaces(void)
{
    IBaseFilter *filter;
    ULONG refcount;
    HRESULT hr;
    IPin *pin;

    hr = create_color_conv(&filter);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    if (hr != S_OK)
        return;

    check_interface(filter, &IID_IBaseFilter, TRUE);
    check_interface(filter, &IID_IMediaFilter, TRUE);
    check_interface(filter, &IID_IPersist, TRUE);
    check_interface(filter, &IID_IUnknown, TRUE);

    check_interface(filter, &IID_IAMFilterMiscFlags, FALSE);
    check_interface(filter, &IID_IBasicAudio, FALSE);
    check_interface(filter, &IID_IBasicVideo, FALSE);
    check_interface(filter, &IID_IKsPropertySet, FALSE);
    check_interface(filter, &IID_IMediaPosition, FALSE);
    check_interface(filter, &IID_IMediaSeeking, FALSE);
    check_interface(filter, &IID_IPin, FALSE);
    check_interface(filter, &IID_IQualityControl, FALSE);
    check_interface(filter, &IID_IQualProp, FALSE);
    check_interface(filter, &IID_IReferenceClock, FALSE);
    check_interface(filter, &IID_IVideoWindow, FALSE);

    hr = IBaseFilter_FindPin(filter, L"In", &pin);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    if (hr == S_OK)
    {
        todo_wine
        check_interface(pin, &IID_IMemInputPin, TRUE);
        check_interface(pin, &IID_IPin, TRUE);
        todo_wine
        check_interface(pin, &IID_IQualityControl, TRUE);
        check_interface(pin, &IID_IUnknown, TRUE);

        check_interface(pin, &IID_IMediaPosition, FALSE);
        check_interface(pin, &IID_IMediaSeeking, FALSE);

        IPin_Release(pin);
    }

    hr = IBaseFilter_FindPin(filter, L"Out", &pin);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    if (hr == S_OK)
    {
        check_interface(pin, &IID_IPin, TRUE);
        todo_wine
        check_interface(pin, &IID_IMediaPosition, TRUE);
        todo_wine
        check_interface(pin, &IID_IMediaSeeking, TRUE);
        todo_wine
        check_interface(pin, &IID_IQualityControl, TRUE);
        check_interface(pin, &IID_IUnknown, TRUE);

        check_interface(pin, &IID_IAsyncReader, FALSE);

        IPin_Release(pin);
    }

    refcount = IBaseFilter_Release(filter);
    ok(refcount == 0, "Got refcount %lu.\n", refcount);
}

static const GUID test_iid = { 0x33333333 };
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

static IUnknown test_outer = { &outer_vtbl };

static void test_aggregation(void)
{
    IBaseFilter *filter, *filter2;
    IUnknown *unk, *unk2;
    HRESULT hr;
    ULONG ref;

    filter = (IBaseFilter *)0xdeadbeef;
    hr = CoCreateInstance(&CLSID_AVIDec, &test_outer, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (void **)&filter);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!filter, "Got interface %p.\n", filter);

    hr = CoCreateInstance(&CLSID_AVIDec, &test_outer, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(outer_ref == 1, "Got unexpected refcount %ld.\n", outer_ref);
    ok(unk != &test_outer, "Returned IUnknown should not be outer IUnknown.\n");
    ref = get_refcount(unk);
    ok(ref == 1, "Got unexpected refcount %ld.\n", ref);

    ref = IUnknown_AddRef(unk);
    ok(ref == 2, "Got unexpected refcount %ld.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %ld.\n", outer_ref);

    ref = IUnknown_Release(unk);
    ok(ref == 1, "Got unexpected refcount %ld.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %ld.\n", outer_ref);

    hr = IUnknown_QueryInterface(unk, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(unk2 == unk, "Got unexpected IUnknown %p.\n", unk2);
    IUnknown_Release(unk2);

    hr = IUnknown_QueryInterface(unk, &IID_IBaseFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_QueryInterface(filter, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(unk2 == (IUnknown *)0xdeadbeef, "Got unexpected IUnknown %p.\n", unk2);

    hr = IBaseFilter_QueryInterface(filter, &IID_IBaseFilter, (void **)&filter2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(filter2 == (IBaseFilter *)0xdeadbeef, "Got unexpected IBaseFilter %p.\n", filter2);

    hr = IUnknown_QueryInterface(unk, &test_iid, (void **)&unk2);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!unk2, "Got unexpected IUnknown %p.\n", unk2);

    hr = IBaseFilter_QueryInterface(filter, &test_iid, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(unk2 == (IUnknown *)0xdeadbeef, "Got unexpected IUnknown %p.\n", unk2);

    IBaseFilter_Release(filter);
    ref = IUnknown_Release(unk);
    ok(!ref, "Got unexpected refcount %ld.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %ld.\n", outer_ref);
}

static void test_enum_pins(void)
{
    IEnumPins *enum1, *enum2;
    ULONG count, refcount;
    IBaseFilter *filter;
    IPin *pins[4];
    HRESULT hr;

    hr = create_color_conv(&filter);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr != S_OK)
        return;

    refcount = get_refcount(filter);
    ok(refcount == 1, "Got refcount %ld.\n", refcount);

    hr = IBaseFilter_EnumPins(filter, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_EnumPins(filter, &enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    refcount = get_refcount(filter);
    ok(refcount == 2, "Got refcount %ld.\n", refcount);
    refcount = get_refcount(enum1);
    ok(refcount == 1, "Got refcount %ld.\n", refcount);

    hr = IEnumPins_Next(enum1, 1, NULL, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr != S_OK)
        goto skip_test;

    refcount = get_refcount(filter);
    ok(refcount == 3, "Got refcount %ld.\n", refcount);
    refcount = get_refcount(pins[0]);
    ok(refcount == 3, "Got refcount %ld.\n", refcount);
    refcount = get_refcount(enum1);
    ok(refcount == 1, "Got refcount %ld.\n", refcount);
    IPin_Release(pins[0]);
    refcount = get_refcount(filter);
    ok(refcount == 2, "Got refcount %ld.\n", refcount);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    refcount = get_refcount(filter);
    ok(refcount == 3, "Got refcount %ld.\n", refcount);
    refcount = get_refcount(pins[0]);
    ok(refcount == 3, "Got refcount %ld.\n", refcount);
    refcount = get_refcount(enum1);
    ok(refcount == 1, "Got refcount %ld.\n", refcount);
    IPin_Release(pins[0]);
    refcount = get_refcount(filter);
    ok(refcount == 2, "Got refcount %ld.\n", refcount);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(enum1, 1, pins, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 1, "Got count %lu.\n", count);
    IPin_Release(pins[0]);

    hr = IEnumPins_Next(enum1, 1, pins, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 1, "Got count %lu.\n", count);
    IPin_Release(pins[0]);

    hr = IEnumPins_Next(enum1, 1, pins, &count);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(!count, "Got count %lu.\n", count);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(enum1, 2, pins, NULL);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(enum1, 3, pins, &count);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(count == 2, "Got count %lu.\n", count);
    IPin_Release(pins[0]);
    IPin_Release(pins[1]);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(enum1, 4, pins, &count);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(count == 2, "Got count %lu.\n", count);
    IPin_Release(pins[0]);
    IPin_Release(pins[1]);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Clone(enum1, &enum2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Skip(enum1, 4);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Skip(enum1, 2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Skip(enum1, 1);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(enum2, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IPin_Release(pins[0]);

    IEnumPins_Release(enum2);

skip_test:
    IEnumPins_Release(enum1);
    refcount = IBaseFilter_Release(filter);
    ok(refcount == 0, "Got refcount %ld.\n", refcount);
}

static void test_find_pin(void)
{
    IEnumPins *enum_pins;
    IBaseFilter *filter;
    IPin *pin, *pin2;
    ULONG refcount;
    HRESULT hr;

    hr = create_color_conv(&filter);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr != S_OK)
        return;

    hr = IBaseFilter_EnumPins(filter, &enum_pins);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_FindPin(filter, L"In", &pin);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr != S_OK)
        goto skip_test;

    hr = IEnumPins_Next(enum_pins, 1, &pin2, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pin2 == pin, "Expected pin %p, got %p.\n", pin, pin2);
    IPin_Release(pin2);
    IPin_Release(pin);

    hr = IBaseFilter_FindPin(filter, L"Out", &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IEnumPins_Next(enum_pins, 1, &pin2, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pin2 == pin, "Expected pin %p, got %p.\n", pin, pin2);
    IPin_Release(pin2);
    IPin_Release(pin);

skip_test:
    IEnumPins_Release(enum_pins);
    refcount = IBaseFilter_Release(filter);
    ok(refcount == 0, "Got refcount %ld.\n", refcount);
}

static void test_pin_info(void)
{
    IBaseFilter *filter;
    PIN_DIRECTION dir;
    ULONG refcount;
    PIN_INFO info;
    ULONG count;
    HRESULT hr;
    WCHAR *id;
    IPin *pin;

    hr = create_color_conv(&filter);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr != S_OK)
        return;

    hr = IBaseFilter_FindPin(filter, L"In", &pin);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr != S_OK)
        goto skip_test;

    refcount = get_refcount(filter);
    ok(refcount == 2, "Got refcount %ld.\n", refcount);
    refcount = get_refcount(pin);
    ok(refcount == 2, "Got refcount %ld.\n", refcount);

    hr = IPin_QueryPinInfo(pin, &info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(info.pFilter == filter, "Expected filter %p, got %p.\n", filter, info.pFilter);
    ok(info.dir == PINDIR_INPUT, "Got direction %d.\n", info.dir);
    ok(!wcscmp(info.achName, L"Input"), "Got name %s.\n", wine_dbgstr_w(info.achName));
    refcount = get_refcount(filter);
    ok(refcount == 3, "Got refcount %ld.\n", refcount);
    refcount = get_refcount(pin);
    ok(refcount == 3, "Got refcount %ld.\n", refcount);
    IBaseFilter_Release(info.pFilter);

    hr = IPin_QueryDirection(pin, &dir);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(dir == PINDIR_INPUT, "Got direction %d.\n", dir);

    hr = IPin_QueryId(pin, &id);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(id, L"In"), "Got id %s.\n", wine_dbgstr_w(id));
    CoTaskMemFree(id);

    hr = IPin_QueryInternalConnections(pin, NULL, &count);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    IPin_Release(pin);

    hr = IBaseFilter_FindPin(filter, L"Out", &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IPin_QueryPinInfo(pin, &info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(info.pFilter == filter, "Expected filter %p, got %p.\n", filter, info.pFilter);
    ok(info.dir == PINDIR_OUTPUT, "Got direction %d.\n", info.dir);
    ok(!wcscmp(info.achName, L"XForm Out"), "Got name %s.\n", wine_dbgstr_w(info.achName));
    refcount = get_refcount(filter);
    ok(refcount == 3, "Got refcount %ld.\n", refcount);
    refcount = get_refcount(pin);
    ok(refcount == 3, "Got refcount %ld.\n", refcount);
    IBaseFilter_Release(info.pFilter);

    hr = IPin_QueryDirection(pin, &dir);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(dir == PINDIR_OUTPUT, "Got direction %d.\n", dir);

    hr = IPin_QueryId(pin, &id);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(id, L"Out"), "Got id %s.\n", wine_dbgstr_w(id));
    CoTaskMemFree(id);

    hr = IPin_QueryInternalConnections(pin, NULL, &count);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    IPin_Release(pin);

skip_test:
    refcount = IBaseFilter_Release(filter);
    ok(refcount == 0, "Got refcount %ld.\n", refcount);
}

static void test_media_types(void)
{
    AM_MEDIA_TYPE *media_types[ARRAY_SIZE(subtypes)], req_mt, mt;
    struct testfilter *peer = NULL;
    IEnumMediaTypes *enum_types;
    ULONG refcount, num_types;
    IPin *sink, *source;
    VIDEOINFO video_info;
    IFilterGraph *graph;
    IBaseFilter *filter;
    DWORD image_size;
    HRESULT hr;
    int i;

    hr = create_color_conv(&filter);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr != S_OK)
        return;

    hr = create_filter_graph(&graph);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph_AddFilter(graph, filter, L"Color Filter");
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_FindPin(filter, L"Out", &source);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr != S_OK)
        goto skip_test;

    hr = IBaseFilter_FindPin(filter, L"In", &sink);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    peer = create_testfilter(NULL);

    memset(&req_mt, 0, sizeof(req_mt));
    memset(&video_info, 0, sizeof(video_info));
    req_mt.majortype = MEDIATYPE_Video;
    req_mt.formattype = FORMAT_VideoInfo;
    req_mt.pbFormat = (BYTE *)&video_info;
    video_info.bmiHeader.biSize = sizeof(video_info.bmiHeader);
    video_info.bmiHeader.biWidth = 240;

    for (i = 0; i < ARRAY_SIZE(subtypes); i++)
    {
        winetest_push_context("subtype %d", i);

        req_mt.subtype = *subtypes[i].guid;
        req_mt.cbFormat = subtypes[i].cbFormat;
        video_info.bmiHeader.biHeight = 240;
        hr = IPin_QueryAccept(sink, &req_mt);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        hr = IPin_ReceiveConnection(sink, &peer->source.pin.IPin_iface, &req_mt);
        todo_wine
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        hr = IPin_Disconnect(sink);
        todo_wine
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        /* Test negative height */
        video_info.bmiHeader.biHeight = -240;

        hr = IPin_QueryAccept(sink, &req_mt);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        hr = IPin_ReceiveConnection(sink, &peer->source.pin.IPin_iface, &req_mt);
        todo_wine
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        hr = IPin_Disconnect(sink);
        todo_wine
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        winetest_pop_context();
    }

    req_mt.subtype = MEDIASUBTYPE_RGB32;
    req_mt.cbFormat = sizeof(VIDEOINFOHEADER);
    video_info.bmiHeader.biHeight = 240;
    /* Any value of biPlanes is accepted, and then echoed back in the enum */
    video_info.bmiHeader.biPlanes = 123;
    /* source and target must be the same size and be contained within the bounds of heigth and width */
    video_info.rcSource.left = 60;
    video_info.rcSource.top = 80;
    video_info.rcSource.right = 120;
    video_info.rcSource.bottom = 200;
    video_info.rcTarget.left = 140;
    video_info.rcTarget.top = 100;
    video_info.rcTarget.right = 200;
    video_info.rcTarget.bottom = 220;

    hr = IPin_ReceiveConnection(sink, &peer->source.pin.IPin_iface, &req_mt);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IPin_EnumMediaTypes(source, &enum_types);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enum_types, ARRAY_SIZE(subtypes), media_types, &num_types);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine
    ok(num_types == 6, "Got num_types %lu.\n", num_types);

    mt = req_mt;
    mt.bFixedSizeSamples = TRUE;

    for (i = 0; i < num_types; i++)
    {
        winetest_push_context("subtype %d", i);

        image_size = 240 * 240 * (subtypes[i].bitcount / 8);
        mt.subtype = *subtypes[i].guid;
        mt.lSampleSize = image_size;
        mt.cbFormat = subtypes[i].cbFormat;
        video_info.bmiHeader.biSizeImage = image_size;
        video_info.bmiHeader.biBitCount = subtypes[i].bitcount;
        video_info.bmiHeader.biCompression = subtypes[i].compression;

        compare_media_types(media_types[i], &mt);

        hr = IPin_QueryAccept(source, media_types[i]);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        /* Copy the RGB32 media type that was accepted for another QueryAccept test after disconnect */
        if (i == 1)
            CopyMediaType(&req_mt, media_types[i]);

        /* Test negative height */
        ((VIDEOINFO *)media_types[i]->pbFormat)->bmiHeader.biHeight = -240;
        hr = IPin_QueryAccept(source, media_types[i]);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        DeleteMediaType(media_types[i]);

        winetest_pop_context();
    }

    hr = IEnumMediaTypes_Next(enum_types, ARRAY_SIZE(subtypes), media_types, &num_types);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(num_types == 0, "Got num_types %lu.\n", num_types);

    IEnumMediaTypes_Release(enum_types);

    hr = IPin_Disconnect(sink);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* The previously accepted media type is no longer accepted after disconnect */
    hr = IPin_QueryAccept(source, &req_mt);
    todo_wine
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    FreeMediaType(&req_mt);

    IPin_Release(sink);
    IPin_Release(source);

    refcount = IBaseFilter_Release(&peer->filter.IBaseFilter_iface);
    ok(refcount == 0, "Got refcount %lu.\n", refcount);

skip_test:
    hr = IFilterGraph_RemoveFilter(graph, filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    refcount = IFilterGraph_Release(graph);
    ok(refcount == 0, "Got refcount %lu.\n", refcount);

    refcount = IBaseFilter_Release(filter);
    ok(refcount == 0, "Got refcount %lu.\n", refcount);
}

static void test_enum_media_types(void)
{
    IEnumMediaTypes *enum1, *enum2;
    AM_MEDIA_TYPE *mts[1];
    IBaseFilter *filter;
    ULONG ref, count;
    HRESULT hr;
    IPin *pin;

    hr = create_color_conv(&filter);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr != S_OK)
        return;

    hr = IBaseFilter_FindPin(filter, L"In", &pin);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr != S_OK)
        goto skip_test;

    hr = IPin_EnumMediaTypes(pin, &enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, &count);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(!count, "Got count %lu.\n", count);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Clone(enum1, &enum2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 1);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enum2, 1, mts, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    IEnumMediaTypes_Release(enum1);
    IEnumMediaTypes_Release(enum2);
    IPin_Release(pin);

    IBaseFilter_FindPin(filter, L"Out", &pin);

    hr = IPin_EnumMediaTypes(pin, &enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, &count);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(!count, "Got count %lu.\n", count);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Clone(enum1, &enum2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 1);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enum2, 1, mts, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    IEnumMediaTypes_Release(enum1);
    IEnumMediaTypes_Release(enum2);
    IPin_Release(pin);

skip_test:
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_unconnected_filter_state(void)
{
    IBaseFilter *filter;
    FILTER_STATE state;
    HRESULT hr;
    ULONG ref;

    hr = create_color_conv(&filter);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr != S_OK)
        return;

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %u.\n", state);

    hr = IBaseFilter_Pause(filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Paused, "Got state %u.\n", state);

    hr = IBaseFilter_Run(filter, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Running, "Got state %u.\n", state);

    hr = IBaseFilter_Pause(filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Paused, "Got state %u.\n", state);

    hr = IBaseFilter_Stop(filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %u.\n", state);

    hr = IBaseFilter_Run(filter, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Running, "Got state %u.\n", state);

    hr = IBaseFilter_Stop(filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %u.\n", state);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_sink_allocator(IMemInputPin *input)
{
    ALLOCATOR_PROPERTIES allocator_props_request, allocator_props_actual;
    IMemAllocator *allocator, *req_allocator, *ret_allocator;
    ALLOCATOR_PROPERTIES props, ret_props;
    LONG image_size;
    HRESULT hr;

    hr = IMemInputPin_GetAllocatorRequirements(input, &props);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    memset(&props, 0xcc, sizeof(props));
    hr = IMemInputPin_GetAllocatorRequirements(input, &props);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    hr = IMemInputPin_GetAllocator(input, &allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    if (hr == S_OK)
    {
        hr = IMemAllocator_GetProperties(allocator, &props);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(!props.cBuffers, "Got %ld buffers.\n", props.cBuffers);
        ok(!props.cbBuffer, "Got size %ld.\n", props.cbBuffer);
        ok(!props.cbAlign, "Got alignment %ld.\n", props.cbAlign);
        ok(!props.cbPrefix, "Got prefix %ld.\n", props.cbPrefix);

        image_size = 240 * 240 * 3;
        allocator_props_request.cBuffers = 1;
        allocator_props_request.cbBuffer = image_size;
        allocator_props_request.cbAlign = 1;
        allocator_props_request.cbPrefix = 0;
        hr = IMemAllocator_SetProperties(allocator, &allocator_props_request, &allocator_props_actual);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(allocator_props_actual.cBuffers == 1, "Got cBuffers %ld.\n", allocator_props_actual.cBuffers);
        ok(allocator_props_actual.cbBuffer == image_size, "Got cbBuffer %ld.\n", allocator_props_actual.cbBuffer);
        ok(allocator_props_actual.cbAlign == 1, "Got cbAlign %ld.\n", allocator_props_actual.cbAlign);
        ok(allocator_props_actual.cbPrefix == 0, "Got cbPrefix %ld.\n", allocator_props_actual.cbPrefix);

        hr = IMemInputPin_NotifyAllocator(input, allocator, TRUE);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
    }

    hr = IMemInputPin_NotifyAllocator(input, NULL, TRUE);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER, &IID_IMemAllocator, (void **)&req_allocator);

    props.cBuffers = 1;
    props.cbBuffer = 256;
    props.cbAlign = 1;
    props.cbPrefix = 0;
    hr = IMemAllocator_SetProperties(req_allocator, &props, &ret_props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemInputPin_NotifyAllocator(input, req_allocator, TRUE);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemInputPin_GetAllocator(input, &ret_allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine
    ok(ret_allocator == req_allocator, "Allocators didn't match.\n");

    IMemAllocator_Release(req_allocator);
    IMemAllocator_Release(ret_allocator);

    hr = IMemInputPin_NotifyAllocator(input, allocator, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IMemAllocator_Release(allocator);
}

static void test_filter_state(IMediaControl *control, IBaseFilter *filter)
{
    FILTER_STATE base_state;
    OAFilterState state;
    HRESULT hr;

    hr = IBaseFilter_Pause(filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IBaseFilter_GetState(filter, 0, &base_state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(base_state == State_Paused, "Got state %#x.\n", base_state);

    hr = IBaseFilter_Pause(filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IBaseFilter_GetState(filter, 0, &base_state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(base_state == State_Paused, "Got state %#x.\n", base_state);
    hr = IBaseFilter_Stop(filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %lu.\n", state);

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Paused, "Got state %lu.\n", state);

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Running, "Got state %lu.\n", state);

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Paused, "Got state %lu.\n", state);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %lu.\n", state);

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Running, "Got state %lu.\n", state);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %lu.\n", state);
}

#define SET_TIME_START    (1 << 0)
#define SET_TIME_END      (1 << 1)
#define SET_MEDIA_TIME    (1 << 2)
#define SET_SYNC_POINT    (1 << 3)
#define SET_PREROLL       (1 << 4)
#define SET_DISCONTINUITY (1 << 5)
#define SET_TEST_QC       (1 << 6)

#define SET_TIME (SET_TIME_START | SET_TIME_END)

#define EXP_TIME               (1 << 0)
#define EXP_MEDIA_TIME         (1 << 1)
#define EXP_SYNC_POINT         (1 << 2)
#define EXP_PREROLL            (1 << 3)
#define EXP_DISCONTINUITY      (1 << 4)
#define EXP_UNDEFINED_TIME_END (1 << 5)

#define TODO_TIME           (1 << 0)
#define TODO_MEDIA_TIME     (1 << 1)
#define TODO_SYNC_POINT     (1 << 2)
#define TODO_PREROLL        (1 << 3)

static void test_sample_processing(
        IMediaControl *control, IMemInputPin *input, struct testfilter *testsink, IBaseFilter *dmo_filter)
{
    static struct
    {
        BYTE flags, expected_flags;
        REFERENCE_TIME time_start, time_end;
        LONGLONG media_time_start, media_time_end;
        BOOL sync_point, preroll, discontinuity;
        BYTE todo_flags;
    }
    tests[] =
    {
        {
            .todo_flags = TODO_SYNC_POINT | TODO_TIME
        },
        {
            .flags = SET_TIME_START | SET_MEDIA_TIME | SET_SYNC_POINT,
            .time_start = 20000,
            .media_time_start = 10000,
            .media_time_end = 20000,
            .sync_point = TRUE,
            .expected_flags = EXP_TIME | EXP_UNDEFINED_TIME_END | EXP_MEDIA_TIME | EXP_SYNC_POINT,
            .todo_flags = TODO_MEDIA_TIME
        },
        {
            .flags = SET_TIME,
            .time_start = 20000,
            .time_end = 30000,
            .media_time_start = 10000,
            .media_time_end = 20000,
            .expected_flags = EXP_TIME | EXP_MEDIA_TIME | EXP_SYNC_POINT,
            .todo_flags = TODO_MEDIA_TIME
        },
        {
            .flags = SET_PREROLL,
            .time_start = 20000,
            .time_end = 30000,
            .media_time_start = 10000,
            .media_time_end = 20000,
            .preroll = TRUE,
            .expected_flags = EXP_TIME | EXP_MEDIA_TIME | EXP_SYNC_POINT,
            .todo_flags = TODO_PREROLL | TODO_MEDIA_TIME
        },
        {
            .flags = SET_PREROLL | SET_DISCONTINUITY,
            .time_end = 30000,
            .time_start = 20000,
            .media_time_start = 10000,
            .media_time_end = 20000,
            .preroll = FALSE,
            .discontinuity = TRUE,
            .expected_flags = EXP_TIME | EXP_MEDIA_TIME | EXP_SYNC_POINT | EXP_DISCONTINUITY,
            .todo_flags = TODO_MEDIA_TIME
        },
        {
            .flags = SET_SYNC_POINT,
            .time_start = 20000,
            .time_end = 30000,
            .media_time_start = 10000,
            .media_time_end = 20000,
            .sync_point = FALSE,
            .expected_flags = EXP_TIME | EXP_MEDIA_TIME | EXP_DISCONTINUITY,
            .todo_flags = TODO_SYNC_POINT | TODO_MEDIA_TIME
        },
        {
            .flags = SET_DISCONTINUITY,
            .time_start = 20000,
            .time_end = 30000,
            .media_time_start = 10000,
            .media_time_end = 20000,
            .discontinuity = FALSE,
            .expected_flags = EXP_TIME | EXP_MEDIA_TIME,
            .todo_flags = TODO_SYNC_POINT | TODO_MEDIA_TIME
        },
        {
            .flags = SET_SYNC_POINT | SET_TEST_QC,
            .time_start = 20000,
            .time_end = 30000,
            .media_time_start = 10000,
            .media_time_end = 20000,
            .sync_point = TRUE,
            .expected_flags = EXP_TIME | EXP_MEDIA_TIME | EXP_SYNC_POINT,
            .todo_flags = TODO_MEDIA_TIME
        },
    };

    struct image_data *rgb24_image, *rgb32_image;
    REFERENCE_TIME *time_start, *time_end;
    struct mem_allocator *sink_allocator;
    VIDEOINFO *video_info_ptr;
    IMemAllocator *allocator;
    IMediaSample *sample;
    LONG image_size;
    LONGLONG diff;
    HRESULT hr;
    BYTE *buff;
    int i;

    hr = IMemInputPin_ReceiveCanBlock(input);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    testsink->can_block = S_FALSE;

    hr = IMemInputPin_ReceiveCanBlock(input);
    todo_wine
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    sink_allocator = mem_allocator_from_IMemAllocator(testsink->sink.pAllocator);

    ok(!sink_allocator->commited, "Allocator should not be commited\n");
    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(sink_allocator->commited, "Allocator should now be commited\n");

    hr = IMemInputPin_GetAllocator(input, &allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemAllocator_Commit(allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    sink_allocator->size = 240 * 240 * 4;

    sink_allocator->expect_get_buffer = TRUE;
    sink_allocator->expect_get_media_type = TRUE;
    sink_allocator->media_type_checked = FALSE;
    hr = IMemAllocator_GetBuffer(allocator, &sample, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine
    ok(sink_allocator->sample_refcount == 1, "Got sample refcount %ld.\n", sink_allocator->sample_refcount);
    todo_wine
    ok(sink_allocator->media_type_checked, "Expected media type to have been checked.\n");
    sink_allocator->expect_get_buffer = FALSE;
    sink_allocator->expect_get_media_type = FALSE;

    for (i = 0; i < ARRAYSIZE(tests); i++)
    {
        winetest_push_context("test %d", i);

        if (tests[i].flags & SET_TIME)
        {
            if (tests[i].flags & SET_TIME_START)
                time_start = &tests[i].time_start;
            else
                time_start = NULL;

            if (tests[i].flags & SET_TIME_END)
                time_end = &tests[i].time_end;
            else
                time_end = NULL;

            hr = IMediaSample_SetTime(sample, time_start, time_end);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
        }

        if (tests[i].flags & SET_MEDIA_TIME)
        {
            time_start = &tests[i].media_time_start;
            time_end = &tests[i].media_time_end;

            hr = IMediaSample_SetMediaTime(sample, time_start, time_end);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
        }

        if (tests[i].flags & SET_SYNC_POINT)
        {
            hr = IMediaSample_SetSyncPoint(sample, tests[i].sync_point);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
        }

        if (tests[i].flags & SET_PREROLL)
        {
            hr = IMediaSample_SetPreroll(sample, tests[i].preroll);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
        }

        if (tests[i].flags & SET_DISCONTINUITY)
        {
            hr = IMediaSample_SetDiscontinuity(sample, tests[i].discontinuity);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
        }

        sink_allocator->test_qc = !!(tests[i].flags & SET_TEST_QC);

        sink_allocator->expect_set_time = TRUE;
        sink_allocator->expect_get_buffer = TRUE;
        sink_allocator->expect_get_media_type = TRUE;
        sink_allocator->media_type_checked = FALSE;
        hr = IMemInputPin_Receive(input, sample);
        todo_wine
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        if (hr != S_OK)
        {
            winetest_pop_context();
            continue;
        }

        ok(sink_allocator->sample_refcount == 1, "Got sample refcount %ld.\n", sink_allocator->sample_refcount);
        todo_wine
        ok(sink_allocator->media_type_checked, "Expected media type to have been checked.\n");
        ok(testsink->sample != NULL, "Expected out peer sample.\n");
        sink_allocator->expect_get_buffer = FALSE;
        sink_allocator->expect_set_time = FALSE;
        sink_allocator->expect_get_media_type = FALSE;

        todo_wine_if(tests[i].todo_flags & TODO_TIME)
        if (tests[i].expected_flags & EXP_TIME)
        {
            ok(sink_allocator->ts_set, "Time start should be set.\n");
            ok(sink_allocator->te_set, "Time end should be set.\n");
            ok(sink_allocator->time_start == tests[i].time_start, "Got start time %I64d.\n",
                    sink_allocator->time_start);
            if (!(tests[i].expected_flags & EXP_UNDEFINED_TIME_END))
                ok(sink_allocator->time_end == tests[i].time_end, "Got end time %I64d.\n", sink_allocator->time_end);
        }
        else
        {
            ok(!sink_allocator->ts_set, "Time start should not be set.\n");
            ok(!sink_allocator->te_set, "Time end should not be set.\n");
        }

        todo_wine_if(tests[i].todo_flags & TODO_MEDIA_TIME)
        if (tests[i].expected_flags & EXP_MEDIA_TIME)
        {
            ok(sink_allocator->mts_set, "Media time start should be set.\n");
            ok(sink_allocator->mte_set, "Media time end should be set.\n");
            ok(sink_allocator->media_time_start == tests[i].media_time_start, "Got media start time %I64d.\n",
                    sink_allocator->media_time_start);
            ok(sink_allocator->media_time_end == tests[i].media_time_end, "Got media end time %I64d.\n",
                    sink_allocator->media_time_end);
        }
        else
        {
            ok(!sink_allocator->mts_set, "Media time start should not be set.\n");
            ok(!sink_allocator->mte_set, "Media time end should not be set.\n");
        }

        if (tests[i].expected_flags & EXP_DISCONTINUITY)
            ok(sink_allocator->discontinuity, "Discontinuity should be set.\n");
        else
            ok(!sink_allocator->discontinuity, "Discontinuity should not be set.\n");

        todo_wine_if(tests[i].todo_flags & TODO_PREROLL)
        if (tests[i].expected_flags & EXP_PREROLL)
            ok(sink_allocator->preroll, "Preroll should be set.\n");
        else
            ok(!sink_allocator->preroll, "Preroll should not be set.\n");

        todo_wine_if(tests[i].todo_flags & TODO_SYNC_POINT)
        if (tests[i].expected_flags & EXP_SYNC_POINT)
            ok(sink_allocator->sync_point, "Sync point should be set.\n");
        else
            ok(!sink_allocator->sync_point, "Sync point should not be set.\n");


        if (testsink->sample)
            IMediaSample_Release(testsink->sample);
        testsink->sample = NULL;

        winetest_pop_context();
    }

    IMediaSample_Release(sample);
    ok(sink_allocator->sample_refcount == 0, "Got sample refcount %ld.\n", sink_allocator->sample_refcount);

    /* Test content of image */
    sink_allocator->expect_get_buffer = TRUE;
    sink_allocator->expect_get_media_type = TRUE;
    sink_allocator->media_type_checked = FALSE;
    hr = IMemAllocator_GetBuffer(allocator, &sample, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine
    ok(sink_allocator->sample_refcount == 1, "Got sample refcount %ld.\n", sink_allocator->sample_refcount);
    todo_wine
    ok(sink_allocator->media_type_checked, "Expected media type to have been checked.\n");
    sink_allocator->expect_get_buffer = FALSE;
    sink_allocator->expect_get_media_type = FALSE;

    rgb24_image = create_image(FALSE, TRUE, 240, 240);

    image_size = IMediaSample_GetSize(sample);
    ok(image_size == rgb24_image->size, "Got image_size %ld.\n", image_size);

    hr = IMediaSample_GetPointer(sample, &buff);
    ok(hr == S_OK, "Get hr %#lx.\n", hr);

    memcpy(buff, rgb24_image->data, image_size);
    hr = IMediaSample_SetActualDataLength(sample, image_size);
    ok(hr == S_OK, "Get hr %#lx.\n", hr);

    sink_allocator->expect_get_buffer = TRUE;
    sink_allocator->expect_get_media_type = TRUE;
    sink_allocator->media_type_checked = FALSE;
    hr = IMemInputPin_Receive(input, sample);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine
    ok(sink_allocator->media_type_checked, "Expected media type to have been checked.\n");
    sink_allocator->expect_get_buffer = FALSE;
    sink_allocator->expect_get_media_type = FALSE;

    IMediaSample_Release(sample);

    sample = testsink->sample;
    testsink->sample = NULL;

    todo_wine
    ok(sample != NULL, "Expected out peer sample.\n");
    if (sample == NULL)
        goto skip_test;

    hr = IMediaSample_GetPointer(sample, &buff);
    ok(hr == S_OK, "Get hr %#lx.\n", hr);

    rgb32_image = create_image(TRUE, TRUE, 240, 240);

    image_size = IMediaSample_GetActualDataLength(sample);
    ok(image_size == rgb32_image->size, "Got image_size %ld.\n", image_size);

    diff = 0;
    for (unsigned int i = 0; i < image_size; ++i)
        diff += abs((int)buff[i] - (int)rgb32_image->data[i]);
    diff = diff * 100 / 256 / image_size;
    ok(diff == 0, "Got %I64u%% difference.\n", diff);

    free(rgb32_image);
    IMediaSample_Release(sample);
    ok(sink_allocator->sample_refcount == 0, "Got sample refcount %ld.\n", sink_allocator->sample_refcount);

    /* Test dynamic format change (flip height) */
    sink_allocator->expect_get_buffer = TRUE;
    sink_allocator->expect_get_media_type = TRUE;
    sink_allocator->media_type_checked = FALSE;
    hr = IMemAllocator_GetBuffer(allocator, &sample, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine
    ok(sink_allocator->sample_refcount == 1, "Got sample refcount %ld.\n", sink_allocator->sample_refcount);
    todo_wine
    ok(sink_allocator->media_type_checked, "Expected media type to have been checked.\n");
    sink_allocator->expect_get_buffer = FALSE;
    sink_allocator->expect_get_media_type = FALSE;

    image_size = IMediaSample_GetSize(sample);
    ok(image_size == rgb24_image->size, "Got image_size %ld.\n", image_size);

    hr = IMediaSample_GetPointer(sample, &buff);
    ok(hr == S_OK, "Get hr %#lx.\n", hr);

    memcpy(buff, rgb24_image->data, image_size);
    free(rgb24_image);
    hr = IMediaSample_SetActualDataLength(sample, image_size);
    ok(hr == S_OK, "Get hr %#lx.\n", hr);

    memset(sink_allocator->data, 0, sizeof(sink_allocator->data));
    video_info_ptr = (VIDEOINFO *)testsink->sink.pin.mt.pbFormat;
    video_info_ptr->bmiHeader.biWidth = 640;
    video_info_ptr->bmiHeader.biHeight = -480;
    video_info_ptr->rcSource.right = 240;
    video_info_ptr->rcSource.bottom = 240;
    video_info_ptr->rcTarget.right = 240;
    video_info_ptr->rcTarget.bottom = 240;
    sink_allocator->size = 640 * 480 * 4;
    sink_allocator->send_media_type = TRUE;

    sink_allocator->expect_get_media_type = TRUE;
    sink_allocator->media_type_checked = FALSE;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine
    ok(sink_allocator->media_type_checked, "Expected media type to have been checked.\n");
    sink_allocator->expect_get_buffer = FALSE;
    sink_allocator->expect_get_media_type = FALSE;
    sink_allocator->send_media_type = FALSE;

    IMediaSample_Release(sample);

    sample = testsink->sample;
    testsink->sample = NULL;

    ok(sample != NULL, "Expected out peer sample.\n");

    hr = IMediaSample_GetPointer(sample, &buff);
    ok(hr == S_OK, "Get hr %#lx.\n", hr);

    rgb32_image = create_image(TRUE, FALSE, 640, 480);

    image_size = IMediaSample_GetSize(sample);
    ok(image_size == rgb32_image->size, "Got image_size %ld.\n", image_size);

    diff = 0;
    for (unsigned int i = 0; i < image_size; ++i)
        diff += abs((int)buff[i] - (int)rgb32_image->data[i]);
    diff = diff * 100 / 256 / image_size;
    todo_wine
    ok(diff == 0, "Got %I64u%% difference.\n", diff);
    free(rgb32_image);

    IMediaSample_Release(sample);
    ok(sink_allocator->sample_refcount == 0, "Got sample refcount %ld.\n", sink_allocator->sample_refcount);

skip_test:
    hr = IMemAllocator_Decommit(allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IMemAllocator_Release(allocator);
}

static void test_streaming_events(IMediaControl *control, IPin *sink, IMemInputPin *input, struct testfilter *testsink)
{
    struct mem_allocator *sink_allocator;
    IMemAllocator *allocator;
    IMediaSample *sample;
    HRESULT hr;
    BYTE *data;
    LONG i;

    sink_allocator = mem_allocator_from_IMemAllocator(testsink->sink.pAllocator);

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemInputPin_GetAllocator(input, &allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMemAllocator_Commit(allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    sink_allocator->expect_get_buffer = TRUE;
    sink_allocator->expect_get_media_type = TRUE;
    hr = IMemAllocator_GetBuffer(allocator, &sample, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    sink_allocator->expect_get_buffer = FALSE;
    sink_allocator->expect_get_media_type = FALSE;
    todo_wine
    ok(sink_allocator->media_type_checked, "Expected media type to have been checked.\n");
    hr = IMediaSample_GetPointer(sample, &data);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    for (i = 0; i < 200; ++i)
        data[i] = i;
    hr = IMediaSample_SetActualDataLength(sample, 200);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(!testsink->got_new_segment, "Got %u calls to IPin::NewSegment().\n", testsink->got_new_segment);
    hr = IPin_NewSegment(sink, 10000, 20000, 1.0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(testsink->got_new_segment == 1, "Got %u calls to IPin::NewSegment().\n", testsink->got_new_segment);

    ok(!testsink->got_eos, "Got %u calls to IPin::EndOfStream().\n", testsink->got_eos);
    hr = IPin_EndOfStream(sink);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!testsink->sample, "Got unexpected sample %p.\n", testsink->sample);
    ok(testsink->got_eos == 1, "Got %u calls to IPin::EndOfStream().\n", testsink->got_eos);
    testsink->got_eos = 0;

    hr = IPin_EndOfStream(sink);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(testsink->got_eos == 1, "Got %u calls to IPin::EndOfStream().\n", testsink->got_eos);

    sink_allocator->media_type_checked = FALSE;
    sink_allocator->expect_get_media_type = TRUE;
    hr = IMemInputPin_Receive(input, sample);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine
    ok(testsink->sample != NULL, "Expected to receive sample.\n");
    todo_wine
    ok(sink_allocator->media_type_checked, "Expected media type to have been checked.\n");
    sink_allocator->expect_get_media_type = FALSE;
    if (testsink->sample)
        IMediaSample_Release(testsink->sample);
    testsink->sample = NULL;

    ok(!testsink->got_begin_flush, "Got %u calls to IPin::BeginFlush().\n", testsink->got_begin_flush);
    hr = IPin_BeginFlush(sink);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(testsink->got_begin_flush == 1, "Got %u calls to IPin::BeginFlush().\n", testsink->got_begin_flush);

    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IPin_EndOfStream(sink);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    ok(!testsink->got_end_flush, "Got %u calls to IPin::EndFlush().\n", testsink->got_end_flush);
    hr = IPin_EndFlush(sink);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(testsink->got_end_flush == 1, "Got %u calls to IPin::EndFlush().\n", testsink->got_end_flush);

    sink_allocator->media_type_checked = FALSE;
    sink_allocator->expect_get_media_type = TRUE;
    sink_allocator->expect_get_buffer = TRUE;
    hr = IMemInputPin_Receive(input, sample);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine
    ok(testsink->sample != NULL, "Expected to receive sample.\n");
    todo_wine
    ok(sink_allocator->media_type_checked, "Expected media type to have been checked.\n");
    sink_allocator->expect_get_buffer = FALSE;
    sink_allocator->expect_get_media_type = FALSE;
    if (testsink->sample)
        IMediaSample_Release(testsink->sample);
    testsink->sample = NULL;

    IMediaSample_Release(sample);

    hr = IMemAllocator_Decommit(allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IMemAllocator_Release(allocator);
}

static void test_connect_pin(void)
{
    struct testfilter *testsource, *testsink = NULL;
    struct mem_allocator *sink_allocator;
    IPin *sink, *source, *peer;
    AM_MEDIA_TYPE mt, req_mt;
    IMemInputPin *meminput;
    IMediaControl *control;
    VIDEOINFO video_info;
    IFilterGraph *graph;
    IBaseFilter *filter;
    ULONG refcount;
    HRESULT hr;

    hr = create_color_conv(&filter);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr != S_OK)
        return;

    hr = create_filter_graph(&graph);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph_QueryInterface(graph, &IID_IMediaControl, (void **)&control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph_AddFilter(graph, filter, L"Color Filter");
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_FindPin(filter, L"In", &sink);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr != S_OK)
        goto skip_test;

    hr = IBaseFilter_FindPin(filter, L"Out", &source);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    meminput = NULL;
    hr = IPin_QueryInterface(sink, &IID_IMemInputPin, (void **)&meminput);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    sink_allocator = create_mem_allocator();
    testsource = create_testfilter(NULL);
    testsink = create_testfilter(&sink_allocator->IMemAllocator_iface);
    sink_allocator->sink = &testsink->sink;

    hr = IFilterGraph_AddFilter(graph, &testsource->filter.IBaseFilter_iface, L"In Peer");
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph_AddFilter(graph, &testsink->filter.IBaseFilter_iface, L"Out Peer");
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Test sink connection. */
    memset(&req_mt, 0, sizeof(req_mt));
    req_mt.majortype = MEDIATYPE_Video;
    req_mt.subtype = MEDIASUBTYPE_RGB24;
    req_mt.formattype = FORMAT_None;

    peer = (IPin *)0xdeadbeef;
    hr = IPin_ConnectedTo(sink, &peer);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);
    ok(!peer, "Got peer %p.\n", peer);

    hr = IPin_ConnectionMediaType(sink, &mt);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    /* Partial types are not accepted */
    hr = IFilterGraph_ConnectDirect(graph, &testsource->source.pin.IPin_iface, sink, &req_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);

    /* Set exact type */
    req_mt.formattype = FORMAT_VideoInfo;
    req_mt.cbFormat = sizeof(video_info);
    req_mt.pbFormat = (BYTE *)&video_info;
    memset(&video_info, 0, sizeof(video_info));
    video_info.bmiHeader.biSize = sizeof(video_info.bmiHeader);
    video_info.bmiHeader.biWidth = 240;
    video_info.bmiHeader.biHeight = 240;
    video_info.bmiHeader.biBitCount = 24;
    testsource->wanted_subtype = &MEDIASUBTYPE_RGB24;

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph_ConnectDirect(graph, &testsource->source.pin.IPin_iface, sink, &req_mt);
    ok(hr == VFW_E_NOT_STOPPED, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph_ConnectDirect(graph, &testsource->source.pin.IPin_iface, sink, &req_mt);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr != S_OK)
        goto skip_connection_test;

    hr = IPin_ConnectedTo(sink, &peer);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(peer == &testsource->source.pin.IPin_iface, "Got peer %p.\n", peer);
    IPin_Release(peer);

    hr = IPin_ConnectionMediaType(sink, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    compare_media_types(&mt, &req_mt);
    compare_media_types(&testsource->source.pin.mt, &req_mt);

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph_Disconnect(graph, sink);
    ok(hr == VFW_E_NOT_STOPPED, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    test_sink_allocator(meminput);

    /* Test source connection. */
    peer = (IPin *)0xdeadbeef;
    hr = IPin_ConnectedTo(source, &peer);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);
    ok(!peer, "Got peer %p.\n", peer);

    hr = IPin_ConnectionMediaType(source, &mt);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    req_mt.subtype = MEDIASUBTYPE_RGB32;
    req_mt.bFixedSizeSamples = TRUE;
    req_mt.lSampleSize = 240 * 240 * 4;
    video_info.bmiHeader.biBitCount = 32;
    hr = IFilterGraph_ConnectDirect(graph, source, &testsink->sink.pin.IPin_iface, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IPin_ConnectionMediaType(source, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    compare_media_types(&mt, &req_mt);
    compare_media_types(&testsink->sink.pin.mt, &req_mt);

    ok(testsource->source.pin.peer == sink, "Got in peer %p.\n", testsource->source.pin.peer);
    ok(testsink->sink.pin.peer == source, "Got out peer %p.\n", testsink->sink.pin.peer);

    test_filter_state(control, filter);
    test_sample_processing(control, meminput, testsink, filter);
    test_streaming_events(control, sink, meminput, testsink);

    ok(sink_allocator->commited, "Allocator should still be commited\n");
    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!sink_allocator->commited, "Allocator should no longer be commited\n");

    hr = IFilterGraph_Disconnect(graph, &testsink->sink.pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(!sink_allocator->second_decommit, "Allocator should not yet have been decommitted twice\n");
    hr = IFilterGraph_Disconnect(graph, source);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine
    ok(sink_allocator->second_decommit, "Expected allocator to have been decommitted twice\n");

    hr = IFilterGraph_Disconnect(graph, sink);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph_Disconnect(graph, &testsource->source.pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

skip_connection_test:
    if (meminput)
        IMemInputPin_Release(meminput);

    IPin_Release(sink);
    IPin_Release(source);

    hr = IFilterGraph_RemoveFilter(graph, &testsink->filter.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph_RemoveFilter(graph, &testsource->filter.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    refcount = IBaseFilter_Release(&testsource->filter.IBaseFilter_iface);
    ok(refcount == 0, "Got refcount %lu.\n", refcount);

    refcount = IBaseFilter_Release(&testsink->filter.IBaseFilter_iface);
    ok(refcount == 0, "Got refcount %lu.\n", refcount);

    refcount = IMemAllocator_Release(&sink_allocator->IMemAllocator_iface);
    ok(refcount == 0, "Got refcount %lu.\n", refcount);
skip_test:
    IMediaControl_Release(control);

    hr = IFilterGraph_RemoveFilter(graph, filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    refcount = IFilterGraph_Release(graph);
    ok(refcount == 0, "Got refcount %lu.\n", refcount);

    refcount = IBaseFilter_Release(filter);
    ok(refcount == 0, "Got refcount %lu.\n", refcount);
}

START_TEST(colorconv)
{
    CoInitialize(NULL);

    test_registration();
    test_interfaces();
    test_aggregation();
    test_enum_pins();
    test_find_pin();
    test_pin_info();
    test_media_types();
    test_enum_media_types();
    test_unconnected_filter_state();
    test_connect_pin();

    CoUninitialize();
}
