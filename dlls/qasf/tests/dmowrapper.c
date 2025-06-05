/*
 * DMO wrapper filter unit tests
 *
 * Copyright (C) 2019-2020 Elizabeth Figura
 * Copyright (C) 2021,2025 Elizabeth Figura for CodeWeavers
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
#include "dmo.h"
#include "dmodshow.h"
#include "mferror.h"
#include "wine/strmbase.h"
#include "wine/test.h"

static const GUID testdmo_clsid = {0x1234};
static const GUID test_iid = {0x33333333};

static int mt1_format = 0xdeadbeef;
static const AM_MEDIA_TYPE mt1 =
{
    .majortype = {0x123},
    .subtype = {0x456},
    .lSampleSize = 789,
    .formattype = {0xabc},
    .cbFormat = sizeof(mt1_format),
    .pbFormat = (BYTE *)&mt1_format,
};

static int mt2_format = 0xdeadf00d;
static const AM_MEDIA_TYPE mt2 =
{
    .majortype = {0x987},
    .subtype = {0x654},
    .lSampleSize = 321,
    .formattype = {0xcba},
    .cbFormat = sizeof(mt2_format),
    .pbFormat = (BYTE *)&mt2_format,
};

static inline BOOL compare_media_types(const AM_MEDIA_TYPE *a, const AM_MEDIA_TYPE *b)
{
    return !memcmp(a, b, offsetof(AM_MEDIA_TYPE, pbFormat))
        && !memcmp(a->pbFormat, b->pbFormat, a->cbFormat);
}

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

static const IMediaObjectVtbl dmo_vtbl;

static IMediaObject testdmo = {&dmo_vtbl};
static IUnknown *testdmo_outer_unk;
static LONG testdmo_refcount = 1;
static AM_MEDIA_TYPE testdmo_input_mt, testdmo_output_mt;
static BOOL testdmo_input_mt_set, testdmo_output_mt_set;

static HRESULT testdmo_GetInputSizeInfo_hr = E_NOTIMPL;
static HRESULT testdmo_GetOutputSizeInfo_hr = S_OK;
static DWORD testdmo_output_size = 123;
static DWORD testdmo_output_alignment = 1;

static unsigned int got_Flush, got_Discontinuity, got_ProcessInput, got_ProcessOutput, got_Receive;
static unsigned int got_AllocateStreamingResources, got_FreeStreamingResources;

static IMediaBuffer *testdmo_buffer;

static int testmode;
static HRESULT process_output_hr = S_OK;
static HRESULT sink_receive_hr = S_OK;

static HRESULT WINAPI dmo_inner_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    if (winetest_debug > 1) trace("QueryInterface(%s)\n", wine_dbgstr_guid(iid));

    if (IsEqualGUID(iid, &IID_IUnknown))
        *out = iface;
    else if (IsEqualGUID(iid, &IID_IMediaObject) || IsEqualGUID(iid, &test_iid))
        *out = &testdmo;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI dmo_inner_AddRef(IUnknown *iface)
{
    return InterlockedIncrement(&testdmo_refcount);
}

static ULONG WINAPI dmo_inner_Release(IUnknown *iface)
{
    return InterlockedDecrement(&testdmo_refcount);
}

static const IUnknownVtbl dmo_inner_vtbl =
{
    dmo_inner_QueryInterface,
    dmo_inner_AddRef,
    dmo_inner_Release,
};

static IUnknown testdmo_inner = {&dmo_inner_vtbl};

static HRESULT WINAPI dmo_QueryInterface(IMediaObject *iface, REFIID iid, void **out)
{
    return IUnknown_QueryInterface(testdmo_outer_unk, iid, out);
}

static ULONG WINAPI dmo_AddRef(IMediaObject *iface)
{
    return IUnknown_AddRef(testdmo_outer_unk);
}

static ULONG WINAPI dmo_Release(IMediaObject *iface)
{
    return IUnknown_Release(testdmo_outer_unk);
}

static HRESULT WINAPI dmo_GetStreamCount(IMediaObject *iface, DWORD *input, DWORD *output)
{
    if (winetest_debug > 1) trace("GetStreamCount()\n");
    *input = 1;
    *output = 2;
    return S_OK;
}

static HRESULT WINAPI dmo_GetInputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    if (winetest_debug > 1) trace("GetInputStreamInfo(%lu)\n", index);
    *flags = 0;
    return S_OK;
}

static HRESULT WINAPI dmo_GetOutputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    if (winetest_debug > 1) trace("GetOutputStreamInfo(%lu)\n", index);
    *flags = 0;
    return S_OK;
}

static HRESULT WINAPI dmo_GetInputType(IMediaObject *iface, DWORD index, DWORD type_index, DMO_MEDIA_TYPE *type)
{
    if (winetest_debug > 1) trace("GetInputType(index %lu, type_index %lu)\n", index, type_index);
    if (!type_index)
    {
        memset(type, 0, sizeof(*type)); /* cover up the holes */
        MoCopyMediaType(type, (const DMO_MEDIA_TYPE *)&mt1);
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

static HRESULT WINAPI dmo_GetOutputType(IMediaObject *iface, DWORD index, DWORD type_index, DMO_MEDIA_TYPE *type)
{
    if (winetest_debug > 1) trace("GetOutputType(index %lu, type_index %lu)\n", index, type_index);
    if (!type_index)
    {
        memset(type, 0, sizeof(*type)); /* cover up the holes */
        MoCopyMediaType(type, (const DMO_MEDIA_TYPE *)&mt2);
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

static HRESULT WINAPI dmo_SetInputType(IMediaObject *iface, DWORD index, const DMO_MEDIA_TYPE *type, DWORD flags)
{
    if (winetest_debug > 1) trace("SetInputType(index %lu, flags %#lx)\n", index, flags);
    strmbase_dump_media_type((AM_MEDIA_TYPE *)type);
    if (flags & DMO_SET_TYPEF_TEST_ONLY)
        return type->lSampleSize == 123 ? S_OK : S_FALSE;
    if (flags & DMO_SET_TYPEF_CLEAR)
    {
        testdmo_input_mt_set = FALSE;
        return S_OK;
    }
    MoCopyMediaType((DMO_MEDIA_TYPE *)&testdmo_input_mt, type);
    testdmo_input_mt_set = TRUE;
    return S_OK;
}

static HRESULT WINAPI dmo_SetOutputType(IMediaObject *iface, DWORD index, const DMO_MEDIA_TYPE *type, DWORD flags)
{
    if (winetest_debug > 1) trace("SetOutputType(index %lu, flags %#lx)\n", index, flags);
    strmbase_dump_media_type((AM_MEDIA_TYPE *)type);
    if (flags & DMO_SET_TYPEF_TEST_ONLY)
        return type->lSampleSize == 321 ? S_OK : S_FALSE;
    if (flags & DMO_SET_TYPEF_CLEAR)
    {
        testdmo_output_mt_set = FALSE;
        return S_OK;
    }
    MoCopyMediaType((DMO_MEDIA_TYPE *)&testdmo_output_mt, type);
    testdmo_output_mt_set = TRUE;
    return S_OK;
}

static HRESULT WINAPI dmo_GetInputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *type)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI dmo_GetOutputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *type)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI dmo_GetInputSizeInfo(IMediaObject *iface, DWORD index,
        DWORD *size, DWORD *lookahead, DWORD *alignment)
{
    if (winetest_debug > 1) trace("GetInputSizeInfo(%lu)\n", index);
    *size = 321;
    *alignment = 64;
    *lookahead = 0;
    return testdmo_GetInputSizeInfo_hr;
}

static HRESULT WINAPI dmo_GetOutputSizeInfo(IMediaObject *iface, DWORD index, DWORD *size, DWORD *alignment)
{
    if (winetest_debug > 1) trace("GetOutputSizeInfo(%lu)\n", index);
    *size = testdmo_output_size;
    *alignment = testdmo_output_alignment;
    return testdmo_GetOutputSizeInfo_hr;
}

static HRESULT WINAPI dmo_GetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME *latency)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI dmo_SetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME latency)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI dmo_Flush(IMediaObject *iface)
{
    if (winetest_debug > 1) trace("Flush()\n");
    ++got_Flush;
    return S_OK;
}

static HRESULT WINAPI dmo_Discontinuity(IMediaObject *iface, DWORD index)
{
    if (winetest_debug > 1) trace("Discontinuity(index %lu)\n", index);
    ++got_Discontinuity;
    return S_OK;
}

static HRESULT WINAPI dmo_AllocateStreamingResources(IMediaObject *iface)
{
    if (winetest_debug > 1) trace("AllocateStreamingResources()\n");
    ++got_AllocateStreamingResources;
    return S_OK;
}

static HRESULT WINAPI dmo_FreeStreamingResources(IMediaObject *iface)
{
    if (winetest_debug > 1) trace("FreeStreamingResources()\n");
    ++got_FreeStreamingResources;
    return S_OK;
}

static HRESULT WINAPI dmo_GetInputStatus(IMediaObject *iface, DWORD index, DWORD *flags)
{
    if (winetest_debug > 1) trace("GetInputStatus(index %lu)\n", index);
    *flags = DMO_INPUT_STATUSF_ACCEPT_DATA;
    return S_OK;
}

static HRESULT WINAPI dmo_ProcessInput(IMediaObject *iface, DWORD index,
    IMediaBuffer *buffer, DWORD flags, REFERENCE_TIME timestamp, REFERENCE_TIME timelength)
{
    BYTE *data, expect[200];
    DWORD len, i;
    HRESULT hr;

    if (winetest_debug > 1) trace("ProcessInput(index %lu, flags %#lx, timestamp %I64d, timelength %I64d)\n",
            index, flags, timestamp, timelength);

    ++got_ProcessInput;

    len = 0xdeadbeef;
    hr = IMediaBuffer_GetBufferAndLength(buffer, NULL, &len);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(len == 200, "Got length %lu.\n", len);

    len = 0xdeadbeef;
    hr = IMediaBuffer_GetBufferAndLength(buffer, &data, &len);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(len == 200, "Got length %lu.\n", len);
    for (i = 0; i < 200; ++i)
        expect[i] = i;
    ok(!memcmp(data, expect, 200), "Data didn't match.\n");

    hr = IMediaBuffer_GetMaxLength(buffer, &len);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(len == 256, "Got length %lu.\n", len);

    if (testmode == 0 || testmode == 12 || testmode == 100)
    {
        ok(!flags, "Got flags %#lx.\n", flags);
        ok(!timestamp, "Got timestamp %s.\n", wine_dbgstr_longlong(timestamp));
        ok(!timelength, "Got length %s.\n", wine_dbgstr_longlong(timelength));
    }
    else if (testmode == 1)
    {
        ok(flags == (DMO_INPUT_DATA_BUFFERF_TIME | DMO_INPUT_DATA_BUFFERF_TIMELENGTH), "Got flags %#lx.\n", flags);
        ok(timestamp == 20000, "Got timestamp %s.\n", wine_dbgstr_longlong(timestamp));
        ok(timelength == 1, "Got length %s.\n", wine_dbgstr_longlong(timelength));
    }
    else if (testmode == 6)
    {
        ok(flags == (DMO_INPUT_DATA_BUFFERF_TIME | DMO_INPUT_DATA_BUFFERF_TIMELENGTH
                | DMO_INPUT_DATA_BUFFERF_SYNCPOINT), "Got flags %#lx.\n", flags);
        ok(timestamp == 20000, "Got timestamp %s.\n", wine_dbgstr_longlong(timestamp));
        ok(timelength == 10000, "Got length %s.\n", wine_dbgstr_longlong(timelength));
    }
    else
    {
        ok(flags == (DMO_INPUT_DATA_BUFFERF_TIME | DMO_INPUT_DATA_BUFFERF_TIMELENGTH), "Got flags %#lx.\n", flags);
        ok(timestamp == 20000, "Got timestamp %s.\n", wine_dbgstr_longlong(timestamp));
        ok(timelength == 10000, "Got length %s.\n", wine_dbgstr_longlong(timelength));
    }

    testdmo_buffer = buffer;
    IMediaBuffer_AddRef(buffer);

    return S_OK;
}

static HRESULT WINAPI dmo_ProcessOutput(IMediaObject *iface, DWORD flags,
        DWORD count, DMO_OUTPUT_DATA_BUFFER *buffers, DWORD *status)
{
    DWORD len, i;
    HRESULT hr;
    BYTE *data;

    if (winetest_debug > 1) trace("ProcessOutput(flags %#lx, count %lu)\n", flags, count);

    ++got_ProcessOutput;

    *status = 0;

    ok(flags == DMO_PROCESS_OUTPUT_DISCARD_WHEN_NO_BUFFER, "Got flags %#lx.\n", flags);
    ok(count == 2, "Got count %lu.\n", count);

    ok(!!buffers[0].pBuffer, "Expected a buffer.\n");
    if (testmode == 12)
        ok(!!buffers[1].pBuffer, "Expected a buffer.\n");
    else
        ok(!buffers[1].pBuffer, "Got unexpected buffer %p.\n", buffers[1].pBuffer);

    buffers[1].dwStatus = DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE;

    hr = IMediaBuffer_GetBufferAndLength(buffers[0].pBuffer, &data, &len);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!len, "Got length %lu.\n", len);

    hr = IMediaBuffer_GetMaxLength(buffers[0].pBuffer, &len);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (testmode == 100)
        ok(len == 555, "Got length %lu.\n", len);
    else
        ok(len == 16384, "Got length %lu.\n", len);

    buffers[0].dwStatus = DMO_OUTPUT_DATA_BUFFERF_TIME | DMO_OUTPUT_DATA_BUFFERF_TIMELENGTH;
    buffers[0].rtTimelength = 1000;
    buffers[0].rtTimestamp = 5000;

    if (buffers[1].pBuffer)
    {
        buffers[1].dwStatus = buffers[0].dwStatus;
        buffers[1].rtTimelength = buffers[0].rtTimelength;
        buffers[1].rtTimestamp = buffers[0].rtTimestamp;
        hr = IMediaBuffer_SetLength(buffers[1].pBuffer, 300);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
    }

    if (testmode == 3)
    {
        hr = IMediaBuffer_SetLength(buffers[0].pBuffer, 16200);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        buffers[0].dwStatus |= DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE;
        return S_OK;
    }
    if (testmode == 5)
    {
        hr = IMediaBuffer_SetLength(buffers[0].pBuffer, 0);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        IMediaBuffer_Release(testdmo_buffer);
        return S_FALSE;
    }

    if (testmode == 7)
        buffers[0].dwStatus |= DMO_OUTPUT_DATA_BUFFERF_SYNCPOINT;
    else if (testmode == 8)
        buffers[0].dwStatus = DMO_OUTPUT_DATA_BUFFERF_TIME;
    else if (testmode == 9)
        buffers[0].dwStatus = 0;

    for (i = 0; i < 300; ++i)
        data[i] = 111 - i;
    hr = IMediaBuffer_SetLength(buffers[0].pBuffer, 2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaBuffer_SetLength(buffers[0].pBuffer, 300);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (testdmo_buffer)
        IMediaBuffer_Release(testdmo_buffer);
    testdmo_buffer = NULL;

    return process_output_hr;
}

static HRESULT WINAPI dmo_Lock(IMediaObject *iface, LONG lock)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IMediaObjectVtbl dmo_vtbl =
{
    dmo_QueryInterface,
    dmo_AddRef,
    dmo_Release,
    dmo_GetStreamCount,
    dmo_GetInputStreamInfo,
    dmo_GetOutputStreamInfo,
    dmo_GetInputType,
    dmo_GetOutputType,
    dmo_SetInputType,
    dmo_SetOutputType,
    dmo_GetInputCurrentType,
    dmo_GetOutputCurrentType,
    dmo_GetInputSizeInfo,
    dmo_GetOutputSizeInfo,
    dmo_GetInputMaxLatency,
    dmo_SetInputMaxLatency,
    dmo_Flush,
    dmo_Discontinuity,
    dmo_AllocateStreamingResources,
    dmo_FreeStreamingResources,
    dmo_GetInputStatus,
    dmo_ProcessInput,
    dmo_ProcessOutput,
    dmo_Lock,
};

static HRESULT WINAPI dmo_cf_QueryInterface(IClassFactory *iface, REFIID iid, void **out)
{
    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IClassFactory))
    {
        *out = iface;
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI dmo_cf_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI dmo_cf_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI dmo_cf_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID iid, void **out)
{
    ok(!!outer, "Expected to be created aggregated.\n");
    ok(IsEqualGUID(iid, &IID_IUnknown), "Got unexpected iid %s.\n", wine_dbgstr_guid(iid));

    *out = &testdmo_inner;
    IUnknown_AddRef(&testdmo_inner);
    testdmo_outer_unk = outer;
    return S_OK;
}

static HRESULT WINAPI dmo_cf_LockServer(IClassFactory *iface, BOOL lock)
{
    ok(0, "Unexpected call.\n");
    return S_OK;
}

static const IClassFactoryVtbl dmo_cf_vtbl =
{
    dmo_cf_QueryInterface,
    dmo_cf_AddRef,
    dmo_cf_Release,
    dmo_cf_CreateInstance,
    dmo_cf_LockServer,
};

static IClassFactory testdmo_cf = {&dmo_cf_vtbl};

static IBaseFilter *create_dmo_wrapper(void)
{
    IDMOWrapperFilter *wrapper;
    IBaseFilter *filter = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_DMOWrapperFilter, NULL,
            CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_QueryInterface(filter, &IID_IDMOWrapperFilter, (void **)&wrapper);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IDMOWrapperFilter_Init(wrapper, &testdmo_clsid, &DMOCATEGORY_AUDIO_DECODER);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IDMOWrapperFilter_Release(wrapper);

    return filter;
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

static void test_interfaces(void)
{
    IBaseFilter *filter = create_dmo_wrapper();
    IPin *pin;

    check_interface(filter, &IID_IBaseFilter, TRUE);
    check_interface(filter, &IID_IDMOWrapperFilter, TRUE);
    check_interface(filter, &IID_IMediaFilter, TRUE);
    check_interface(filter, &IID_IPersist, TRUE);
    todo_wine check_interface(filter, &IID_IPersistStream, TRUE);
    check_interface(filter, &IID_IUnknown, TRUE);

    check_interface(filter, &IID_IAMFilterMiscFlags, FALSE);
    check_interface(filter, &IID_IBasicAudio, FALSE);
    check_interface(filter, &IID_IBasicVideo, FALSE);
    check_interface(filter, &IID_IKsPropertySet, FALSE);
    check_interface(filter, &IID_IMediaPosition, FALSE);
    check_interface(filter, &IID_IMediaSeeking, FALSE);
    check_interface(filter, &IID_IPersistPropertyBag, FALSE);
    check_interface(filter, &IID_IPin, FALSE);
    check_interface(filter, &IID_IQualityControl, FALSE);
    check_interface(filter, &IID_IQualProp, FALSE);
    check_interface(filter, &IID_IReferenceClock, FALSE);
    check_interface(filter, &IID_IVideoWindow, FALSE);

    IBaseFilter_FindPin(filter, L"in0", &pin);

    check_interface(pin, &IID_IMemInputPin, TRUE);
    check_interface(pin, &IID_IPin, TRUE);
    todo_wine check_interface(pin, &IID_IQualityControl, TRUE);
    check_interface(pin, &IID_IUnknown, TRUE);

    check_interface(pin, &IID_IKsPropertySet, FALSE);
    check_interface(pin, &IID_IMediaPosition, FALSE);
    check_interface(pin, &IID_IMediaSeeking, FALSE);

    IPin_Release(pin);

    IBaseFilter_FindPin(filter, L"out0", &pin);

    check_interface(pin, &IID_IMediaPosition, TRUE);
    check_interface(pin, &IID_IMediaSeeking, TRUE);
    check_interface(pin, &IID_IPin, TRUE);
    todo_wine check_interface(pin, &IID_IQualityControl, TRUE);
    check_interface(pin, &IID_IUnknown, TRUE);

    check_interface(pin, &IID_IAsyncReader, FALSE);
    check_interface(pin, &IID_IKsPropertySet, FALSE);

    IPin_Release(pin);

    IBaseFilter_Release(filter);
}

static HRESULT WINAPI outer_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    ok(0, "Unexpected call.\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI outer_AddRef(IUnknown *iface)
{
    ok(0, "Unexpected call.\n");
    return 2;
}

static ULONG WINAPI outer_Release(IUnknown *iface)
{
    ok(0, "Unexpected call.\n");
    return 1;
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

    /* The DMO wrapper filter pretends to support aggregation, but doesn't
     * actually aggregate anything. */

    filter = (IBaseFilter *)0xdeadbeef;
    hr = CoCreateInstance(&CLSID_DMOWrapperFilter, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!filter, "Got interface %p.\n", filter);

    hr = CoCreateInstance(&CLSID_DMOWrapperFilter, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(unk != &test_outer, "Returned IUnknown should not be outer IUnknown.\n");
    ref = get_refcount(unk);
    ok(ref == 1, "Got unexpected refcount %ld.\n", ref);

    ref = IUnknown_AddRef(unk);
    ok(ref == 2, "Got unexpected refcount %ld.\n", ref);

    ref = IUnknown_Release(unk);
    ok(ref == 1, "Got unexpected refcount %ld.\n", ref);

    hr = IUnknown_QueryInterface(unk, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(unk2 == unk, "Got unexpected IUnknown %p.\n", unk2);
    IUnknown_Release(unk2);

    hr = IUnknown_QueryInterface(unk, &IID_IBaseFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_QueryInterface(filter, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(unk2 == unk, "Got unexpected IUnknown %p.\n", unk2);
    IUnknown_Release(unk2);

    hr = IBaseFilter_QueryInterface(filter, &IID_IBaseFilter, (void **)&filter2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(filter2 == filter, "Got unexpected IBaseFilter %p.\n", filter2);
    IBaseFilter_Release(filter2);

    hr = IUnknown_QueryInterface(unk, &test_iid, (void **)&unk2);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!unk2, "Got unexpected IUnknown %p.\n", unk2);

    hr = IBaseFilter_QueryInterface(filter, &test_iid, (void **)&unk2);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!unk2, "Got unexpected IUnknown %p.\n", unk2);

    IBaseFilter_Release(filter);
    ref = IUnknown_Release(unk);
    ok(!ref, "Got unexpected refcount %ld.\n", ref);

    /* Test also aggregation of the inner media object. */

    filter = create_dmo_wrapper();

    hr = IBaseFilter_QueryInterface(filter, &IID_IMediaObject, (void **)&unk);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(unk == (IUnknown *)&testdmo, "Got unexpected object %p.\n", unk);
    IUnknown_Release(unk);

    hr = IBaseFilter_QueryInterface(filter, &test_iid, (void **)&unk);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(unk == (IUnknown *)&testdmo, "Got unexpected object %p.\n", unk);
    IUnknown_Release(unk);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got unexpected refcount %ld.\n", ref);
}

static void test_enum_pins(void)
{
    IBaseFilter *filter = create_dmo_wrapper();
    IEnumPins *enum1, *enum2;
    ULONG count, ref;
    IPin *pins[4];
    HRESULT hr;

    ref = get_refcount(filter);
    ok(ref == 1, "Got unexpected refcount %ld.\n", ref);

    hr = IBaseFilter_EnumPins(filter, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_EnumPins(filter, &enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ref = get_refcount(filter);
    ok(ref == 2, "Got unexpected refcount %ld.\n", ref);
    ref = get_refcount(enum1);
    ok(ref == 1, "Got unexpected refcount %ld.\n", ref);

    hr = IEnumPins_Next(enum1, 1, NULL, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ref = get_refcount(filter);
    ok(ref == 3, "Got unexpected refcount %ld.\n", ref);
    ref = get_refcount(pins[0]);
    ok(ref == 3, "Got unexpected refcount %ld.\n", ref);
    ref = get_refcount(enum1);
    ok(ref == 1, "Got unexpected refcount %ld.\n", ref);
    IPin_Release(pins[0]);
    ref = get_refcount(filter);
    ok(ref == 2, "Got unexpected refcount %ld.\n", ref);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ref = get_refcount(filter);
    ok(ref == 3, "Got unexpected refcount %ld.\n", ref);
    ref = get_refcount(pins[0]);
    ok(ref == 3, "Got unexpected refcount %ld.\n", ref);
    ref = get_refcount(enum1);
    ok(ref == 1, "Got unexpected refcount %ld.\n", ref);
    IPin_Release(pins[0]);
    ref = get_refcount(filter);
    ok(ref == 2, "Got unexpected refcount %ld.\n", ref);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ref = get_refcount(filter);
    ok(ref == 3, "Got unexpected refcount %ld.\n", ref);
    ref = get_refcount(pins[0]);
    ok(ref == 3, "Got unexpected refcount %ld.\n", ref);
    ref = get_refcount(enum1);
    ok(ref == 1, "Got unexpected refcount %ld.\n", ref);
    IPin_Release(pins[0]);
    ref = get_refcount(filter);
    ok(ref == 2, "Got unexpected refcount %ld.\n", ref);

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

    hr = IEnumPins_Next(enum1, 2, pins, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 2, "Got count %lu.\n", count);
    IPin_Release(pins[0]);
    IPin_Release(pins[1]);

    hr = IEnumPins_Next(enum1, 2, pins, &count);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(count == 1, "Got count %lu.\n", count);
    IPin_Release(pins[0]);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(enum1, 4, pins, &count);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(count == 3, "Got count %lu.\n", count);
    IPin_Release(pins[0]);
    IPin_Release(pins[1]);
    IPin_Release(pins[2]);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Clone(enum1, &enum2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Skip(enum1, 4);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Skip(enum1, 3);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Skip(enum1, 1);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(enum2, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IPin_Release(pins[0]);

    IEnumPins_Release(enum2);
    IEnumPins_Release(enum1);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_find_pin(void)
{
    IBaseFilter *filter = create_dmo_wrapper();
    IEnumPins *enum_pins;
    IPin *pin, *pin2;
    HRESULT hr;
    ULONG ref;

    hr = IBaseFilter_EnumPins(filter, &enum_pins);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_FindPin(filter, L"in0", &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IEnumPins_Next(enum_pins, 1, &pin2, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pin2 == pin, "Expected pin %p, got %p.\n", pin, pin2);
    IPin_Release(pin2);
    IPin_Release(pin);

    hr = IBaseFilter_FindPin(filter, L"out0", &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IEnumPins_Next(enum_pins, 1, &pin2, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pin2 == pin, "Expected pin %p, got %p.\n", pin, pin2);
    IPin_Release(pin2);
    IPin_Release(pin);

    hr = IBaseFilter_FindPin(filter, L"out1", &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IEnumPins_Next(enum_pins, 1, &pin2, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pin2 == pin, "Expected pin %p, got %p.\n", pin, pin2);
    IPin_Release(pin2);
    IPin_Release(pin);

    IEnumPins_Release(enum_pins);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_pin_info(void)
{
    IBaseFilter *filter = create_dmo_wrapper();
    PIN_DIRECTION dir;
    PIN_INFO info;
    ULONG count;
    HRESULT hr;
    WCHAR *id;
    ULONG ref;
    IPin *pin;

    hr = IBaseFilter_FindPin(filter, L"in0", &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ref = get_refcount(filter);
    ok(ref == 2, "Got unexpected refcount %ld.\n", ref);
    ref = get_refcount(pin);
    ok(ref == 2, "Got unexpected refcount %ld.\n", ref);

    hr = IPin_QueryPinInfo(pin, &info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(info.pFilter == filter, "Expected filter %p, got %p.\n", filter, info.pFilter);
    ok(info.dir == PINDIR_INPUT, "Got direction %d.\n", info.dir);
    ok(!wcscmp(info.achName, L"in0"), "Got name %s.\n", wine_dbgstr_w(info.achName));
    ref = get_refcount(filter);
    ok(ref == 3, "Got unexpected refcount %ld.\n", ref);
    ref = get_refcount(pin);
    ok(ref == 3, "Got unexpected refcount %ld.\n", ref);
    IBaseFilter_Release(info.pFilter);

    hr = IPin_QueryDirection(pin, &dir);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(dir == PINDIR_INPUT, "Got direction %d.\n", dir);

    hr = IPin_QueryId(pin, &id);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(id, L"in0"), "Got id %s.\n", wine_dbgstr_w(id));
    CoTaskMemFree(id);

    hr = IPin_QueryInternalConnections(pin, NULL, &count);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    IPin_Release(pin);

    hr = IBaseFilter_FindPin(filter, L"out0", &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IPin_QueryPinInfo(pin, &info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(info.pFilter == filter, "Expected filter %p, got %p.\n", filter, info.pFilter);
    ok(info.dir == PINDIR_OUTPUT, "Got direction %d.\n", info.dir);
    ok(!wcscmp(info.achName, L"out0"), "Got name %s.\n", wine_dbgstr_w(info.achName));
    ref = get_refcount(filter);
    ok(ref == 3, "Got unexpected refcount %ld.\n", ref);
    ref = get_refcount(pin);
    ok(ref == 3, "Got unexpected refcount %ld.\n", ref);
    IBaseFilter_Release(info.pFilter);

    hr = IPin_QueryDirection(pin, &dir);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(dir == PINDIR_OUTPUT, "Got direction %d.\n", dir);

    hr = IPin_QueryId(pin, &id);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(id, L"out0"), "Got id %s.\n", wine_dbgstr_w(id));
    CoTaskMemFree(id);

    hr = IPin_QueryInternalConnections(pin, NULL, &count);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    IPin_Release(pin);

    hr = IBaseFilter_FindPin(filter, L"out1", &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IPin_QueryPinInfo(pin, &info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(info.pFilter == filter, "Expected filter %p, got %p.\n", filter, info.pFilter);
    ok(info.dir == PINDIR_OUTPUT, "Got direction %d.\n", info.dir);
    ok(!wcscmp(info.achName, L"out1"), "Got name %s.\n", wine_dbgstr_w(info.achName));
    ref = get_refcount(filter);
    ok(ref == 3, "Got unexpected refcount %ld.\n", ref);
    ref = get_refcount(pin);
    ok(ref == 3, "Got unexpected refcount %ld.\n", ref);
    IBaseFilter_Release(info.pFilter);

    hr = IPin_QueryDirection(pin, &dir);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(dir == PINDIR_OUTPUT, "Got direction %d.\n", dir);

    hr = IPin_QueryId(pin, &id);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(id, L"out1"), "Got id %s.\n", wine_dbgstr_w(id));
    CoTaskMemFree(id);

    hr = IPin_QueryInternalConnections(pin, NULL, &count);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    IPin_Release(pin);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_media_types(void)
{
    IBaseFilter *filter = create_dmo_wrapper();
    AM_MEDIA_TYPE *mt, req_mt = {};
    IEnumMediaTypes *enummt;
    HRESULT hr;
    ULONG ref;
    IPin *pin;

    IBaseFilter_FindPin(filter, L"in0", &pin);

    hr = IPin_EnumMediaTypes(pin, &enummt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enummt, 1, &mt, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(mt, &mt1), "Media types didn't match.\n");
    DeleteMediaType(mt);

    hr = IEnumMediaTypes_Next(enummt, 1, &mt, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    IEnumMediaTypes_Release(enummt);

    hr = IPin_QueryAccept(pin, &req_mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    req_mt.lSampleSize = 123;
    hr = IPin_QueryAccept(pin, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IPin_Release(pin);

    IBaseFilter_FindPin(filter, L"out0", &pin);

    hr = IPin_EnumMediaTypes(pin, &enummt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enummt, 1, &mt, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(mt, &mt2), "Media types didn't match.\n");
    DeleteMediaType(mt);

    hr = IEnumMediaTypes_Next(enummt, 1, &mt, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    IEnumMediaTypes_Release(enummt);

    hr = IPin_QueryAccept(pin, &req_mt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    req_mt.lSampleSize = 321;
    hr = IPin_QueryAccept(pin, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IPin_Release(pin);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_enum_media_types(void)
{
    IBaseFilter *filter = create_dmo_wrapper();
    IEnumMediaTypes *enum1, *enum2;
    AM_MEDIA_TYPE *mts[2];
    ULONG ref, count;
    HRESULT hr;
    IPin *pin;

    IBaseFilter_FindPin(filter, L"in0", &pin);

    hr = IPin_EnumMediaTypes(pin, &enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    CoTaskMemFree(mts[0]->pbFormat);
    CoTaskMemFree(mts[0]);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 1, "Got count %lu.\n", count);
    CoTaskMemFree(mts[0]->pbFormat);
    CoTaskMemFree(mts[0]);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, &count);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(!count, "Got count %lu.\n", count);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 2, mts, &count);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(count == 1, "Got count %lu.\n", count);
    CoTaskMemFree(mts[0]->pbFormat);
    CoTaskMemFree(mts[0]);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Clone(enum1, &enum2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 2);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 1);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 1);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumMediaTypes_Next(enum2, 1, mts, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    CoTaskMemFree(mts[0]->pbFormat);
    CoTaskMemFree(mts[0]);

    IEnumMediaTypes_Release(enum1);
    IEnumMediaTypes_Release(enum2);
    IPin_Release(pin);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

struct testfilter
{
    struct strmbase_filter filter;
    struct strmbase_source source;
    struct strmbase_sink sink;
    const AM_MEDIA_TYPE *sink_mt;
    unsigned int got_new_segment, got_eos, got_begin_flush, got_end_flush;
    HANDLE event;
    IMemAllocator IMemAllocator_iface;
    IMemAllocator *wrapped_allocator;

    IMediaSample IMediaSample_iface;
    IMediaSample *wrapped_sample;
};

static inline struct testfilter *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, filter);
}

static struct strmbase_pin *testfilter_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface);
    if (!index)
        return &filter->source.pin;
    else if (index == 1)
        return &filter->sink.pin;
    return NULL;
}

static void testfilter_destroy(struct strmbase_filter *iface)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface);
    strmbase_source_cleanup(&filter->source);
    strmbase_sink_cleanup(&filter->sink);
    strmbase_filter_cleanup(&filter->filter);
    CloseHandle(filter->event);
}

static const struct strmbase_filter_ops testfilter_ops =
{
    .filter_get_pin = testfilter_get_pin,
    .filter_destroy = testfilter_destroy,
};

static HRESULT WINAPI testsource_DecideAllocator(struct strmbase_source *iface,
        IMemInputPin *input, IMemAllocator **allocator)
{
    return S_OK;
}

static const struct strmbase_source_ops testsource_ops =
{
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideAllocator = testsource_DecideAllocator,
};

static HRESULT testsink_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface->filter);

    if (IsEqualGUID(iid, &IID_IMemInputPin))
        *out = &filter->sink.IMemInputPin_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT testsink_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface->filter);
    if (filter->sink_mt && !compare_media_types(mt, filter->sink_mt))
        return S_FALSE;
    return S_OK;
}

static HRESULT testsink_get_media_type(struct strmbase_pin *iface, unsigned int index, AM_MEDIA_TYPE *mt)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface->filter);
    if (!index && filter->sink_mt)
    {
        CopyMediaType(mt, filter->sink_mt);
        return S_OK;
    }
    return VFW_S_NO_MORE_ITEMS;
}

static HRESULT WINAPI testsink_Receive(struct strmbase_sink *iface, IMediaSample *sample)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface->pin.filter);
    REFERENCE_TIME start, stop;
    AM_MEDIA_TYPE *mt;
    LONG len, i;
    HRESULT hr;

    ++got_Receive;

    len = IMediaSample_GetSize(sample);
    if (testmode == 100)
        ok(len == 555, "Got size %lu.\n", len);
    else
        ok(len == 16384, "Got size %lu.\n", len);
    len = IMediaSample_GetActualDataLength(sample);
    if (testmode == 3)
        ok(len == 16200, "Got length %lu.\n", len);
    else
    {
        BYTE *data, expect[300];

        ok(len == 300, "Got length %lu.\n", len);

        if (testmode != 12)
        {
            hr = IMediaSample_GetPointer(sample, &data);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);

            for (i = 0; i < 300; ++i)
                expect[i] = 111 - i;
            ok(!memcmp(data, expect, 300), "Data didn't match.\n");
        }
    }

    hr = IMediaSample_GetTime(sample, &start, &stop);
    if (testmode == 8)
    {
        ok(hr == VFW_S_NO_STOP_TIME, "Got hr %#lx.\n", hr);
        ok(start == 5000, "Got start time %s.\n", wine_dbgstr_longlong(start));
    }
    else if (testmode == 9)
        ok(hr == VFW_E_SAMPLE_TIME_NOT_SET, "Got hr %#lx.\n", hr);
    else
    {
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(start == 5000, "Got start time %s.\n", wine_dbgstr_longlong(start));
        ok(stop == 6000, "Got stop time %s.\n", wine_dbgstr_longlong(stop));
    }

    hr = IMediaSample_GetMediaTime(sample, &start, &stop);
    ok(hr == VFW_E_MEDIA_TIME_NOT_SET, "Got hr %#lx.\n", hr);

    hr = IMediaSample_IsDiscontinuity(sample);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    hr = IMediaSample_IsPreroll(sample);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    hr = IMediaSample_IsSyncPoint(sample);
    ok(hr == (testmode == 7 ? S_OK : S_FALSE), "Got hr %#lx.\n", hr);

    hr = IMediaSample_GetMediaType(sample, &mt);
    if (testmode == 100)
    {
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(compare_media_types(mt, &testdmo_output_mt), "Media types didn't match.\n");
    }
    else
    {
        ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    }

    if (testmode == 3)
        testmode = 4;
    if (testmode == 10)
        testmode = 11;

    if (testmode == 13 && got_Receive > 1)
        WaitForSingleObject(filter->event, INFINITE);

    return sink_receive_hr;
}

static HRESULT testsink_new_segment(struct strmbase_sink *iface,
        REFERENCE_TIME start, REFERENCE_TIME stop, double rate)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface->pin.filter);
    ++filter->got_new_segment;
    ok(start == 10000, "Got start %s.\n", wine_dbgstr_longlong(start));
    ok(stop == 20000, "Got stop %s.\n", wine_dbgstr_longlong(stop));
    ok(rate == 1.0, "Got rate %.16e.\n", rate);
    return 0xdeadbeef;
}

static HRESULT testsink_eos(struct strmbase_sink *iface)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface->pin.filter);
    ++filter->got_eos;
    return 0xdeadbeef;
}

static HRESULT testsink_begin_flush(struct strmbase_sink *iface)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface->pin.filter);
    ++filter->got_begin_flush;
    return 0xdeadbeef;
}

static HRESULT testsink_end_flush(struct strmbase_sink *iface)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface->pin.filter);
    ++filter->got_end_flush;
    return 0xdeadbeef;
}

static const struct strmbase_sink_ops testsink_ops =
{
    .base.pin_query_interface = testsink_query_interface,
    .base.pin_query_accept = testsink_query_accept,
    .base.pin_get_media_type = testsink_get_media_type,
    .pfnReceive = testsink_Receive,
    .sink_new_segment = testsink_new_segment,
    .sink_eos = testsink_eos,
    .sink_begin_flush = testsink_begin_flush,
    .sink_end_flush = testsink_end_flush,
};

static struct testfilter *impl_from_IMediaSample(IMediaSample *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, IMediaSample_iface);
}

static HRESULT WINAPI sample_QueryInterface(IMediaSample *iface, REFIID iid, void **out)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static ULONG WINAPI sample_AddRef(IMediaSample *iface)
{
    return 2;
}

static ULONG WINAPI sample_Release(IMediaSample *iface)
{
    return 1;
}

static HRESULT WINAPI sample_GetPointer(IMediaSample *iface, BYTE **data)
{
    struct testfilter *filter = impl_from_IMediaSample(iface);

    return IMediaSample_GetPointer(filter->wrapped_sample, data);
}

static LONG WINAPI sample_GetSize(IMediaSample *iface)
{
    struct testfilter *filter = impl_from_IMediaSample(iface);

    return IMediaSample_GetSize(filter->wrapped_sample);
}

static HRESULT WINAPI sample_GetTime(IMediaSample *iface, REFERENCE_TIME *start, REFERENCE_TIME *end)
{
    struct testfilter *filter = impl_from_IMediaSample(iface);

    return IMediaSample_GetTime(filter->wrapped_sample, start, end);
}

static HRESULT WINAPI sample_SetTime(IMediaSample *iface, REFERENCE_TIME *start, REFERENCE_TIME *end)
{
    struct testfilter *filter = impl_from_IMediaSample(iface);

    return IMediaSample_SetTime(filter->wrapped_sample, start, end);
}

static HRESULT WINAPI sample_IsSyncPoint(IMediaSample *iface)
{
    struct testfilter *filter = impl_from_IMediaSample(iface);

    return IMediaSample_IsSyncPoint(filter->wrapped_sample);
}

static HRESULT WINAPI sample_SetSyncPoint(IMediaSample *iface, BOOL sync_point)
{
    struct testfilter *filter = impl_from_IMediaSample(iface);

    return IMediaSample_SetSyncPoint(filter->wrapped_sample, sync_point);
}

static HRESULT WINAPI sample_IsPreroll(IMediaSample *iface)
{
    struct testfilter *filter = impl_from_IMediaSample(iface);

    return IMediaSample_IsPreroll(filter->wrapped_sample);
}

static HRESULT WINAPI sample_SetPreroll(IMediaSample *iface, BOOL preroll)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static LONG WINAPI sample_GetActualDataLength(IMediaSample *iface)
{
    struct testfilter *filter = impl_from_IMediaSample(iface);

    return IMediaSample_GetActualDataLength(filter->wrapped_sample);
}

static HRESULT WINAPI sample_SetActualDataLength(IMediaSample *iface, LONG size)
{
    struct testfilter *filter = impl_from_IMediaSample(iface);

    if (winetest_debug > 1) trace("SetActualDataLength(%ld)\n", size);

    ok(size == 300, "Got size %ld.\n", size);

    IMediaSample_SetActualDataLength(filter->wrapped_sample, size);
    return E_FAIL;
}

static HRESULT WINAPI sample_GetMediaType(IMediaSample *iface, AM_MEDIA_TYPE **mt)
{
    struct testfilter *filter = impl_from_IMediaSample(iface);

    return IMediaSample_GetMediaType(filter->wrapped_sample, mt);
}

static HRESULT WINAPI sample_SetMediaType(IMediaSample *iface, AM_MEDIA_TYPE *mt)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI sample_IsDiscontinuity(IMediaSample *iface)
{
    struct testfilter *filter = impl_from_IMediaSample(iface);

    return IMediaSample_IsDiscontinuity(filter->wrapped_sample);
}

static HRESULT WINAPI sample_SetDiscontinuity(IMediaSample *iface, BOOL discontinuity)
{
    struct testfilter *filter = impl_from_IMediaSample(iface);

    return IMediaSample_SetDiscontinuity(filter->wrapped_sample, discontinuity);
}

static HRESULT WINAPI sample_GetMediaTime(IMediaSample *iface, LONGLONG *start, LONGLONG *end)
{
    struct testfilter *filter = impl_from_IMediaSample(iface);

    return IMediaSample_GetMediaTime(filter->wrapped_sample, start, end);
}

static HRESULT WINAPI sample_SetMediaTime(IMediaSample *iface, LONGLONG *start, LONGLONG *end)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IMediaSampleVtbl sample_vtbl =
{
    sample_QueryInterface,
    sample_AddRef,
    sample_Release,
    sample_GetPointer,
    sample_GetSize,
    sample_GetTime,
    sample_SetTime,
    sample_IsSyncPoint,
    sample_SetSyncPoint,
    sample_IsPreroll,
    sample_SetPreroll,
    sample_GetActualDataLength,
    sample_SetActualDataLength,
    sample_GetMediaType,
    sample_SetMediaType,
    sample_IsDiscontinuity,
    sample_SetDiscontinuity,
    sample_GetMediaTime,
    sample_SetMediaTime,
};

static struct testfilter *impl_from_IMemAllocator(IMemAllocator *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, IMemAllocator_iface);
}

static HRESULT WINAPI allocator_QueryInterface(IMemAllocator *iface, REFIID iid, void **out)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static ULONG WINAPI allocator_AddRef(IMemAllocator *iface)
{
    return 2;
}

static ULONG WINAPI allocator_Release(IMemAllocator *iface)
{
    return 1;
}

static HRESULT WINAPI allocator_SetProperties(IMemAllocator *iface,
        ALLOCATOR_PROPERTIES *req_props, ALLOCATOR_PROPERTIES *ret_props)
{
    struct testfilter *filter = impl_from_IMemAllocator(iface);

    return IMemAllocator_SetProperties(filter->wrapped_allocator, req_props, ret_props);
}

static HRESULT WINAPI allocator_GetProperties(IMemAllocator *iface, ALLOCATOR_PROPERTIES *props)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI allocator_Commit(IMemAllocator *iface)
{
    struct testfilter *filter = impl_from_IMemAllocator(iface);

    return IMemAllocator_Commit(filter->wrapped_allocator);
}

static HRESULT WINAPI allocator_Decommit(IMemAllocator *iface)
{
    struct testfilter *filter = impl_from_IMemAllocator(iface);

    return IMemAllocator_Decommit(filter->wrapped_allocator);
}

static HRESULT WINAPI allocator_GetBuffer(IMemAllocator *iface, IMediaSample **sample,
        REFERENCE_TIME *start_time, REFERENCE_TIME *end_time, DWORD flags)
{
    struct testfilter *filter = impl_from_IMemAllocator(iface);
    HRESULT hr;

    if (winetest_debug > 1) trace("GetBuffer()\n");

    ok(!start_time, "Got start time.\n");
    ok(!end_time, "Got end time.\n");
    ok(!flags, "Got flags %#lx.\n", flags);

    ok(!filter->wrapped_sample, "Should not have called GetBuffer() twice here.\n");

    hr = IMemAllocator_GetBuffer(filter->wrapped_allocator, &filter->wrapped_sample, start_time, end_time, flags);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaSample_SetMediaType(filter->wrapped_sample, (AM_MEDIA_TYPE *)&mt1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaSample_SetActualDataLength(filter->wrapped_sample, 1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    *sample = &filter->IMediaSample_iface;
    return S_OK;
}

static HRESULT WINAPI allocator_ReleaseBuffer(IMemAllocator *iface, IMediaSample *sample)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IMemAllocatorVtbl allocator_vtbl =
{
    allocator_QueryInterface,
    allocator_AddRef,
    allocator_Release,
    allocator_SetProperties,
    allocator_GetProperties,
    allocator_Commit,
    allocator_Decommit,
    allocator_GetBuffer,
    allocator_ReleaseBuffer,
};

static void testfilter_init(struct testfilter *filter)
{
    static const GUID clsid = {0xabacab};
    memset(filter, 0, sizeof(*filter));
    strmbase_filter_init(&filter->filter, NULL, &clsid, &testfilter_ops);
    strmbase_source_init(&filter->source, &filter->filter, L"source", &testsource_ops);
    strmbase_sink_init(&filter->sink, &filter->filter, L"sink", &testsink_ops, NULL);
    filter->event = CreateEventA(NULL, TRUE, FALSE, NULL);
    filter->IMemAllocator_iface.lpVtbl = &allocator_vtbl;
    filter->IMediaSample_iface.lpVtbl = &sample_vtbl;
}

static void test_sink_allocator(IMemInputPin *input)
{
    IMemAllocator *req_allocator, *ret_allocator;
    ALLOCATOR_PROPERTIES props, ret_props;
    HRESULT hr;

    hr = IMemInputPin_GetAllocatorRequirements(input, &props);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    memset(&props, 0xcc, sizeof(props));
    testdmo_GetInputSizeInfo_hr = S_OK;
    hr = IMemInputPin_GetAllocatorRequirements(input, &props);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        ok(props.cBuffers == 1, "Got %ld buffers.\n", props.cBuffers);
        ok(props.cbBuffer == 321, "Got size %ld.\n", props.cbBuffer);
        ok(props.cbAlign == 64, "Got alignment %ld.\n", props.cbAlign);
        ok(props.cbPrefix == 0xcccccccc, "Got prefix %ld.\n", props.cbPrefix);
    }

    hr = IMemInputPin_GetAllocator(input, &ret_allocator);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);

    if (hr == S_OK)
    {
        hr = IMemAllocator_GetProperties(ret_allocator, &props);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(!props.cBuffers, "Got %ld buffers.\n", props.cBuffers);
        ok(!props.cbBuffer, "Got size %ld.\n", props.cbBuffer);
        ok(!props.cbAlign, "Got alignment %ld.\n", props.cbAlign);
        ok(!props.cbPrefix, "Got prefix %ld.\n", props.cbPrefix);

        hr = IMemInputPin_NotifyAllocator(input, ret_allocator, TRUE);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        IMemAllocator_Release(ret_allocator);
    }

    hr = IMemInputPin_NotifyAllocator(input, NULL, TRUE);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMemAllocator, (void **)&req_allocator);

    props.cBuffers = 1;
    props.cbBuffer = 256;
    props.cbAlign = 1;
    props.cbPrefix = 0;
    hr = IMemAllocator_SetProperties(req_allocator, &props, &ret_props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemInputPin_NotifyAllocator(input, req_allocator, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemInputPin_GetAllocator(input, &ret_allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(ret_allocator == req_allocator, "Allocators didn't match.\n");

    IMemAllocator_Release(req_allocator);
    IMemAllocator_Release(ret_allocator);
}

static void test_source_allocator(IFilterGraph2 *graph, IMediaControl *control,
        IPin *sink, IPin *source, struct testfilter *testsink)
{
    ALLOCATOR_PROPERTIES props, req_props = {2, 30000, 32, 0};
    IMemAllocator *allocator, *sink_allocator;
    IMediaSample *sample;
    IMemInputPin *input;
    HRESULT hr;
    BYTE *data;
    LONG size;

    got_AllocateStreamingResources = got_FreeStreamingResources = 0;

    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink->sink.pin.IPin_iface, &mt2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(!!testsink->sink.pAllocator, "Expected an allocator.\n");
    hr = IMemAllocator_GetProperties(testsink->sink.pAllocator, &props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(props.cBuffers == 1, "Got %ld buffers.\n", props.cBuffers);
    ok(props.cbBuffer == 16384, "Got size %ld.\n", props.cbBuffer);
    ok(props.cbAlign == 1, "Got alignment %ld.\n", props.cbAlign);
    ok(!props.cbPrefix, "Got prefix %ld.\n", props.cbPrefix);

    hr = IMemAllocator_GetBuffer(testsink->sink.pAllocator, &sample, NULL, NULL, 0);
    ok(hr == VFW_E_NOT_COMMITTED, "Got hr %#lx.\n", hr);

    ok(!got_AllocateStreamingResources, "Got %u calls to AllocateStreamingResources().\n",
            got_AllocateStreamingResources);
    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(got_AllocateStreamingResources == 1, "Got %u calls to AllocateStreamingResources().\n",
            got_AllocateStreamingResources);

    hr = IMemAllocator_GetBuffer(testsink->sink.pAllocator, &sample, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IMediaSample_Release(sample);

    ok(!got_FreeStreamingResources, "Got %u calls to FreeStreamingResources().\n",
            got_FreeStreamingResources);
    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(got_FreeStreamingResources == 1, "Got %u calls to FreeStreamingResources().\n",
            got_FreeStreamingResources);

    hr = IMemAllocator_GetBuffer(testsink->sink.pAllocator, &sample, NULL, NULL, 0);
    ok(hr == VFW_E_NOT_COMMITTED, "Got hr %#lx.\n", hr);

    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink->sink.pin.IPin_iface);

    testdmo_output_alignment = 16;
    testdmo_output_size = 20000;

    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink->sink.pin.IPin_iface, &mt2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(!!testsink->sink.pAllocator, "Expected an allocator.\n");
    hr = IMemAllocator_GetProperties(testsink->sink.pAllocator, &props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(props.cBuffers == 1, "Got %ld buffers.\n", props.cBuffers);
    ok(props.cbBuffer == 20000, "Got size %ld.\n", props.cbBuffer);
    ok(props.cbAlign == 16, "Got alignment %ld.\n", props.cbAlign);
    ok(!props.cbPrefix, "Got prefix %ld.\n", props.cbPrefix);

    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink->sink.pin.IPin_iface);

    testdmo_GetOutputSizeInfo_hr = E_NOTIMPL;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink->sink.pin.IPin_iface, &mt2);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);
    testdmo_GetOutputSizeInfo_hr = S_OK;

    CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMemAllocator, (void **)&allocator);
    testsink->wrapped_allocator = allocator;
    testsink->sink.pAllocator = &testsink->IMemAllocator_iface;

    hr = IMemAllocator_SetProperties(allocator, &req_props, &props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink->sink.pin.IPin_iface, &mt2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(testsink->sink.pAllocator == &testsink->IMemAllocator_iface, "Expected our allocator to be used.\n");
    hr = IMemAllocator_GetProperties(allocator, &props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(props.cBuffers == 1, "Got %ld buffers.\n", props.cBuffers);
    ok(props.cbBuffer == 20000, "Got size %ld.\n", props.cbBuffer);
    ok(props.cbAlign == 16, "Got alignment %ld.\n", props.cbAlign);
    ok(!props.cbPrefix, "Got prefix %ld.\n", props.cbPrefix);

    /* Test dynamic format change. */

    IPin_QueryInterface(sink, &IID_IMemInputPin, (void **)&input);

    CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMemAllocator, (void **)&sink_allocator);

    req_props.cBuffers = 1;
    req_props.cbBuffer = 256;
    req_props.cbAlign = 1;
    req_props.cbPrefix = 0;
    hr = IMemAllocator_SetProperties(sink_allocator, &req_props, &props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemInputPin_NotifyAllocator(input, sink_allocator, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemAllocator_Commit(sink_allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    req_props.cbBuffer = 555;
    hr = IMemAllocator_SetProperties(allocator, &req_props, &props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemAllocator_GetProperties(allocator, &props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(props.cBuffers == 1, "Got %ld buffers.\n", props.cBuffers);
    ok(props.cbBuffer == 555, "Got size %ld.\n", props.cbBuffer);
    ok(props.cbAlign == 1, "Got alignment %ld.\n", props.cbAlign);
    ok(!props.cbPrefix, "Got prefix %ld.\n", props.cbPrefix);

    hr = IMemAllocator_GetBuffer(sink_allocator, &sample, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaSample_GetPointer(sample, &data);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    size = IMediaSample_GetSize(sample);
    ok(size == 256, "Got size %ld.\n", size);
    for (unsigned int i = 0; i < 200; ++i)
        data[i] = i;
    hr = IMediaSample_SetActualDataLength(sample, 200);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    testmode = 100;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(got_Receive == 1, "Got %u calls to Receive().\n", got_Receive);
    got_Receive = 0;

    ok(compare_media_types(&testdmo_output_mt, &mt1), "Media types didn't match.\n");

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IMemInputPin_Release(input);
    IMemAllocator_Release(sink_allocator);

    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink->sink.pin.IPin_iface);
}

static void test_filter_state(IMediaControl *control)
{
    OAFilterState state;
    HRESULT hr;

    got_Flush = 0;

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

    ok(!got_Flush, "Unexpected IMediaObject::Flush().\n");
    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(got_Flush, "Expected IMediaObject::Flush().\n");
    got_Flush = 0;

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %lu.\n", state);

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Running, "Got state %lu.\n", state);

    ok(!got_Flush, "Unexpected IMediaObject::Flush().\n");
    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(got_Flush, "Expected IMediaObject::Flush().\n");
    got_Flush = 0;

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %lu.\n", state);
}

struct receive_proc_arg
{
    IMemInputPin *input_pin;
    IMediaSample *sample;
};

static DWORD WINAPI receive_proc(void *arg)
{
    struct receive_proc_arg *proc_arg = arg;
    HRESULT hr;

    hr = IMemInputPin_Receive(proc_arg->input_pin, proc_arg->sample);
    ok(hr == S_OK, "Receive returned %#lx.\n", hr);

    return 0;
}

static DWORD WINAPI stop_filter_proc(void *arg)
{
    IBaseFilter *filter = arg;
    HRESULT hr;

    hr = IBaseFilter_Stop(filter);
    ok(hr == S_OK, "Stop returned %#lx.\n", hr);

    return 0;
}

static void test_sample_processing(IMediaControl *control, IMemInputPin *input,
        struct testfilter *testsink, IBaseFilter *dmo_filter)
{
    struct receive_proc_arg receive_proc_arg;
    HANDLE stop_thread, receive_thread;
    REFERENCE_TIME start, stop;
    IMemAllocator *allocator;
    IMediaSample *sample;
    LONG size, i;
    HRESULT hr;
    BYTE *data;
    DWORD ret;

    hr = IMemInputPin_ReceiveCanBlock(input);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemInputPin_GetAllocator(input, &allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemAllocator_GetBuffer(allocator, &sample, NULL, NULL, 0);
    ok(hr == VFW_E_NOT_COMMITTED, "Got hr %#lx.\n", hr);

    hr = IMemAllocator_Commit(allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMemAllocator_GetBuffer(allocator, &sample, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMediaSample_GetPointer(sample, &data);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    size = IMediaSample_GetSize(sample);
    ok(size == 256, "Got size %ld.\n", size);
    for (i = 0; i < 200; ++i)
        data[i] = i;
    hr = IMediaSample_SetActualDataLength(sample, 200);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    start = 10000;
    stop = 20000;
    hr = IMediaSample_SetMediaTime(sample, &start, &stop);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaSample_SetPreroll(sample, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    testmode = 0;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(got_ProcessInput == 1, "Got %u calls to ProcessInput().\n", got_ProcessInput);
    ok(got_ProcessOutput == 1, "Got %u calls to ProcessOutput().\n", got_ProcessOutput);
    ok(got_Receive == 1, "Got %u calls to Receive().\n", got_Receive);
    got_ProcessInput = got_ProcessOutput = got_Receive = 0;

    start = 20000;
    hr = IMediaSample_SetTime(sample, &start, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    testmode = 1;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(got_ProcessInput == 1, "Got %u calls to ProcessInput().\n", got_ProcessInput);
    ok(got_ProcessOutput == 1, "Got %u calls to ProcessOutput().\n", got_ProcessOutput);
    ok(got_Receive == 1, "Got %u calls to Receive().\n", got_Receive);
    got_ProcessInput = got_ProcessOutput = got_Receive = 0;

    stop = 30000;
    hr = IMediaSample_SetTime(sample, &start, &stop);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    testmode = 2;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(got_ProcessInput == 1, "Got %u calls to ProcessInput().\n", got_ProcessInput);
    ok(got_ProcessOutput == 1, "Got %u calls to ProcessOutput().\n", got_ProcessOutput);
    ok(got_Receive == 1, "Got %u calls to Receive().\n", got_Receive);
    got_ProcessInput = got_ProcessOutput = got_Receive = 0;

    testmode = 3;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(got_ProcessInput == 1, "Got %u calls to ProcessInput().\n", got_ProcessInput);
    ok(got_ProcessOutput == 2, "Got %u calls to ProcessOutput().\n", got_ProcessOutput);
    ok(got_Receive == 2, "Got %u calls to Receive().\n", got_Receive);
    got_ProcessInput = got_ProcessOutput = got_Receive = 0;

    testmode = 5;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(got_ProcessInput == 1, "Got %u calls to ProcessInput().\n", got_ProcessInput);
    ok(got_ProcessOutput == 1, "Got %u calls to ProcessOutput().\n", got_ProcessOutput);
    ok(!got_Receive, "Got %u calls to Receive().\n", got_Receive);
    got_ProcessInput = got_ProcessOutput = got_Receive = 0;

    hr = IMediaSample_SetSyncPoint(sample, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    testmode = 6;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(got_ProcessInput == 1, "Got %u calls to ProcessInput().\n", got_ProcessInput);
    ok(got_ProcessOutput == 1, "Got %u calls to ProcessOutput().\n", got_ProcessOutput);
    ok(got_Receive == 1, "Got %u calls to Receive().\n", got_Receive);
    got_ProcessInput = got_ProcessOutput = got_Receive = 0;

    hr = IMediaSample_SetSyncPoint(sample, FALSE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    testmode = 7;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(got_ProcessInput == 1, "Got %u calls to ProcessInput().\n", got_ProcessInput);
    ok(got_ProcessOutput == 1, "Got %u calls to ProcessOutput().\n", got_ProcessOutput);
    ok(got_Receive == 1, "Got %u calls to Receive().\n", got_Receive);
    got_ProcessInput = got_ProcessOutput = got_Receive = 0;

    testmode = 8;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(got_ProcessInput == 1, "Got %u calls to ProcessInput().\n", got_ProcessInput);
    ok(got_ProcessOutput == 1, "Got %u calls to ProcessOutput().\n", got_ProcessOutput);
    ok(got_Receive == 1, "Got %u calls to Receive().\n", got_Receive);
    got_ProcessInput = got_ProcessOutput = got_Receive = 0;

    testmode = 9;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(got_ProcessInput == 1, "Got %u calls to ProcessInput().\n", got_ProcessInput);
    ok(got_ProcessOutput == 1, "Got %u calls to ProcessOutput().\n", got_ProcessOutput);
    ok(got_Receive == 1, "Got %u calls to Receive().\n", got_Receive);
    got_ProcessInput = got_ProcessOutput = got_Receive = 0;

    hr = IMediaSample_SetDiscontinuity(sample, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(!got_Discontinuity, "Got %u calls to Discontinuity().\n", got_Discontinuity);
    testmode = 10;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(got_ProcessInput == 1, "Got %u calls to ProcessInput().\n", got_ProcessInput);
    ok(got_ProcessOutput == 2, "Got %u calls to ProcessOutput().\n", got_ProcessOutput);
    ok(got_Receive == 2, "Got %u calls to Receive().\n", got_Receive);
    ok(got_Discontinuity == 1, "Got %u calls to Discontinuity().\n", got_Discontinuity);
    got_ProcessInput = got_ProcessOutput = got_Receive = got_Discontinuity = 0;

    /* Test receive with downstream input allocator decommitted. */
    hr = IMemAllocator_Decommit(testsink->sink.pAllocator);
    ok(hr == S_OK, "Decommit returned %#lx.\n", hr);
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == VFW_E_NOT_COMMITTED, "Receive returned %#lx.\n", hr);
    ok(got_ProcessInput == 0, "Got %u calls to ProcessInput().\n", got_ProcessInput);
    ok(got_ProcessOutput == 0, "Got %u calls to ProcessOutput().\n", got_ProcessOutput);
    ok(got_Receive == 0, "Got %u calls to Receive().\n", got_Receive);
    ok(got_Discontinuity == 1, "Got %u calls to Discontinuity().\n", got_Discontinuity);
    hr = IMemAllocator_Commit(testsink->sink.pAllocator);
    ok(hr == S_OK, "Commit returned %#lx.\n", hr);
    got_ProcessInput = got_ProcessOutput = got_Receive = got_Discontinuity = 0;

    /* Test receive with filter stopped. */
    hr = IBaseFilter_Stop(dmo_filter);
    ok(hr == S_OK, "Stop returned %#lx.\n", hr);
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == VFW_E_WRONG_STATE, "Receive returned %#lx.\n", hr);
    ok(got_ProcessInput == 0, "Got %u calls to ProcessInput().\n", got_ProcessInput);
    ok(got_ProcessOutput == 0, "Got %u calls to ProcessOutput().\n", got_ProcessOutput);
    ok(got_Receive == 0, "Got %u calls to Receive().\n", got_Receive);
    ok(got_Discontinuity == 0, "Got %u calls to Discontinuity().\n", got_Discontinuity);
    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Run returned %#lx.\n", hr);
    got_ProcessInput = got_ProcessOutput = got_Receive = got_Discontinuity = 0;

    /* Test stop filter during receiving. */
    testmode = 13;
    /* Call Receive() in new thread, it will be blocked
     * by event waiting in testsink_Receive(). */
    receive_proc_arg.input_pin = input;
    receive_proc_arg.sample = sample;
    receive_thread = CreateThread(NULL, 0, receive_proc, &receive_proc_arg, 0, NULL);
    ok(!!receive_thread, "CreateThread returned NULL thread.\n");
    ret = WaitForSingleObject(receive_thread, 200);
    ok(ret == WAIT_TIMEOUT, "WaitForSingleObject returned %#lx.\n", ret);
    /* Call Stop() in new thread, it will be blocked
     * because Receive() is still processing. */
    stop_thread = CreateThread(NULL, 0, stop_filter_proc, dmo_filter, 0, NULL);
    ok(!!stop_thread, "CreateThread returned NULL thread.\n");
    ret = WaitForSingleObject(stop_thread, 200);
    ok(ret == WAIT_TIMEOUT, "WaitForSingleObject returned %#lx.\n", ret);
    /* Signal event to end Receive(). */
    SetEvent(testsink->event);
    ret = WaitForSingleObject(receive_thread, 200);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx.\n", ret);
    ret = WaitForSingleObject(stop_thread, 200);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx.\n", ret);
    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Pause returned %#lx.\n", hr);
    got_ProcessInput = got_ProcessOutput = got_Receive = got_Discontinuity = 0;

    testmode = 0xdeadbeef;

    /* Test Receive if downstream Receive fails. */
    sink_receive_hr = E_FAIL;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == E_FAIL, "Receive returned %#lx.\n", hr);
    ok(got_ProcessInput == 0, "Got %u calls to ProcessInput().\n", got_ProcessInput);
    ok(got_ProcessOutput == 1, "Got %u calls to ProcessOutput().\n", got_ProcessOutput);
    ok(got_Receive == 1, "Got %u calls to Receive().\n", got_Receive);
    ok(got_Discontinuity == 1, "Got %u calls to Discontinuity().\n", got_Discontinuity);
    got_ProcessInput = got_ProcessOutput = got_Receive = got_Discontinuity = 0;

    /* Test Receive if downstream Receive return S_FALSE. */
    sink_receive_hr = S_FALSE;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_FALSE, "Receive returned %#lx.\n", hr);
    ok(got_ProcessInput == 0, "Got %u calls to ProcessInput().\n", got_ProcessInput);
    ok(got_ProcessOutput == 1, "Got %u calls to ProcessOutput().\n", got_ProcessOutput);
    ok(got_Receive == 1, "Got %u calls to Receive().\n", got_Receive);
    ok(got_Discontinuity == 1, "Got %u calls to Discontinuity().\n", got_Discontinuity);
    got_ProcessInput = got_ProcessOutput = got_Receive = got_Discontinuity = 0;

    sink_receive_hr = S_OK;

    /* Test Receive if ProcessOutput return S_FALSE. */
    process_output_hr = S_FALSE;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Receive returned %#lx.\n", hr);
    ok(got_ProcessInput == 1, "Got %u calls to ProcessInput().\n", got_ProcessInput);
    ok(got_ProcessOutput == 2, "Got %u calls to ProcessOutput().\n", got_ProcessOutput);
    ok(got_Receive == 2, "Got %u calls to Receive().\n", got_Receive);
    ok(got_Discontinuity == 1, "Got %u calls to Discontinuity().\n", got_Discontinuity);
    got_ProcessInput = got_ProcessOutput = got_Receive = got_Discontinuity = 0;

    /* Test Receive if ProcessOutput fails. */
    process_output_hr = E_FAIL;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Receive returned %#lx.\n", hr);
    ok(got_ProcessInput == 1, "Got %u calls to ProcessInput().\n", got_ProcessInput);
    ok(got_ProcessOutput == 2, "Got %u calls to ProcessOutput().\n", got_ProcessOutput);
    ok(got_Receive == 0, "Got %u calls to Receive().\n", got_Receive);
    ok(got_Discontinuity == 1, "Got %u calls to Discontinuity().\n", got_Discontinuity);
    got_ProcessInput = got_ProcessOutput = got_Receive = got_Discontinuity = 0;

    /* Test Receive if ProcessOutput needs more inputs. */
    process_output_hr = MF_E_TRANSFORM_NEED_MORE_INPUT;
    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Receive returned %#lx.\n", hr);
    ok(got_ProcessInput == 1, "Got %u calls to ProcessInput().\n", got_ProcessInput);
    ok(got_ProcessOutput == 2, "Got %u calls to ProcessOutput().\n", got_ProcessOutput);
    ok(got_Receive == 0, "Got %u calls to Receive().\n", got_Receive);
    ok(got_Discontinuity == 1, "Got %u calls to Discontinuity().\n", got_Discontinuity);
    got_ProcessInput = got_ProcessOutput = got_Receive = got_Discontinuity = 0;

    process_output_hr = S_OK;

    CloseHandle(stop_thread);
    CloseHandle(receive_thread);
    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IMediaSample_Release(sample);
    IMemAllocator_Release(allocator);
}

static void test_streaming_events(IMediaControl *control, IPin *sink, IMemInputPin *input,
        struct testfilter *testsink, struct testfilter *testsink2)
{
    IMemAllocator *allocator;
    IMediaSample *sample;
    HRESULT hr;
    BYTE *data;
    LONG i;

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    got_Flush = 0;

    hr = IMemInputPin_GetAllocator(input, &allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMemAllocator_Commit(allocator);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMemAllocator_GetBuffer(allocator, &sample, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IMediaSample_GetPointer(sample, &data);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    for (i = 0; i < 200; ++i)
        data[i] = i;
    hr = IMediaSample_SetActualDataLength(sample, 200);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(!testsink->got_new_segment, "Got %u calls to IPin::NewSegment().\n", testsink->got_new_segment);
    ok(!testsink2->got_new_segment, "Got %u calls to IPin::NewSegment().\n", testsink2->got_new_segment);
    hr = IPin_NewSegment(sink, 10000, 20000, 1.0);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(testsink->got_new_segment == 1, "Got %u calls to IPin::NewSegment().\n", testsink->got_new_segment);
    ok(testsink2->got_new_segment == 1, "Got %u calls to IPin::NewSegment().\n", testsink2->got_new_segment);

    ok(!testsink->got_eos, "Got %u calls to IPin::EndOfStream().\n", testsink->got_eos);
    ok(!testsink2->got_eos, "Got %u calls to IPin::EndOfStream().\n", testsink2->got_eos);
    testmode = 12;
    hr = IPin_EndOfStream(sink);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!got_ProcessInput, "Got %u calls to ProcessInput().\n", got_ProcessInput);
    ok(got_Discontinuity == 1, "Got %u calls to Discontinuity().\n", got_Discontinuity);
    ok(got_ProcessOutput == 1, "Got %u calls to ProcessOutput().\n", got_ProcessOutput);
    ok(got_Receive == 2, "Got %u calls to Receive().\n", got_Receive);
    ok(got_Flush == 1, "Got %u calls to Flush().\n", got_Flush);
    ok(testsink->got_eos == 1, "Got %u calls to IPin::EndOfStream().\n", testsink->got_eos);
    ok(testsink2->got_eos == 1, "Got %u calls to IPin::EndOfStream().\n", testsink2->got_eos);
    got_ProcessInput = got_ProcessOutput = got_Receive = got_Discontinuity = 0;

    hr = IPin_EndOfStream(sink);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(testsink->got_eos == 1, "Got %u calls to IPin::EndOfStream().\n", testsink->got_eos);
    todo_wine ok(testsink2->got_eos == 1, "Got %u calls to IPin::EndOfStream().\n", testsink2->got_eos);

    hr = IMemInputPin_Receive(input, sample);
    todo_wine ok(hr == S_FALSE /* 2003 */ || hr == VFW_E_SAMPLE_REJECTED_EOS, "Got hr %#lx.\n", hr);

    got_Flush = 0;
    ok(!testsink->got_begin_flush, "Got %u calls to IPin::BeginFlush().\n", testsink->got_begin_flush);
    ok(!testsink2->got_begin_flush, "Got %u calls to IPin::BeginFlush().\n", testsink2->got_begin_flush);
    hr = IPin_BeginFlush(sink);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(testsink->got_begin_flush == 1, "Got %u calls to IPin::BeginFlush().\n", testsink->got_begin_flush);
    ok(testsink2->got_begin_flush == 1, "Got %u calls to IPin::BeginFlush().\n", testsink2->got_begin_flush);

    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IPin_EndOfStream(sink);
    todo_wine ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    ok(!testsink->got_end_flush, "Got %u calls to IPin::EndFlush().\n", testsink->got_end_flush);
    ok(!testsink2->got_end_flush, "Got %u calls to IPin::EndFlush().\n", testsink2->got_end_flush);
    hr = IPin_EndFlush(sink);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(testsink->got_end_flush == 1, "Got %u calls to IPin::EndFlush().\n", testsink->got_end_flush);
    ok(testsink2->got_end_flush == 1, "Got %u calls to IPin::EndFlush().\n", testsink2->got_end_flush);
    ok(got_Flush, "Expected IMediaObject::Flush().\n");

    hr = IMemInputPin_Receive(input, sample);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(got_ProcessInput == 1, "Got %u calls to ProcessInput().\n", got_ProcessInput);
    todo_wine ok(got_ProcessOutput == 1, "Got %u calls to ProcessOutput().\n", got_ProcessOutput);
    todo_wine ok(got_Receive == 2, "Got %u calls to Receive().\n", got_Receive);
    got_ProcessInput = got_ProcessOutput = got_Receive = 0;

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IMediaSample_Release(sample);
    IMemAllocator_Release(allocator);
}

static void test_connect_pin(void)
{
    AM_MEDIA_TYPE req_mt =
    {
        .majortype = MEDIATYPE_Stream,
        .subtype = MEDIASUBTYPE_Avi,
        .formattype = FORMAT_None,
    };
    struct testfilter testsource, testsink, testsink2;
    IBaseFilter *filter = create_dmo_wrapper();
    IPin *sink, *source, *source2, *peer;
    IMediaControl *control;
    IMemInputPin *meminput;
    IFilterGraph2 *graph;
    AM_MEDIA_TYPE mt;
    HRESULT hr;
    ULONG ref;

    testfilter_init(&testsource);
    testfilter_init(&testsink);
    testfilter_init(&testsink2);
    CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterGraph2, (void **)&graph);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);
    IFilterGraph2_AddFilter(graph, &testsource.filter.IBaseFilter_iface, L"source");
    IFilterGraph2_AddFilter(graph, &testsink.filter.IBaseFilter_iface, L"sink");
    IFilterGraph2_AddFilter(graph, &testsink2.filter.IBaseFilter_iface, L"sink2");
    IFilterGraph2_AddFilter(graph, filter, L"DMO wrapper");
    IBaseFilter_FindPin(filter, L"in0", &sink);
    IBaseFilter_FindPin(filter, L"out0", &source);
    IBaseFilter_FindPin(filter, L"out1", &source2);
    IPin_QueryInterface(sink, &IID_IMemInputPin, (void **)&meminput);

    /* Test sink connection. */
    peer = (IPin *)0xdeadbeef;
    hr = IPin_ConnectedTo(sink, &peer);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);
    ok(!peer, "Got peer %p.\n", peer);

    hr = IPin_ConnectionMediaType(sink, &mt);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, &testsource.source.pin.IPin_iface, sink, &req_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_ConnectDirect(graph, &testsource.source.pin.IPin_iface, sink, &req_mt);
    ok(hr == VFW_E_NOT_STOPPED, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(!testdmo_input_mt_set, "Input type should not be set.\n");

    req_mt.lSampleSize = 123;
    hr = IFilterGraph2_ConnectDirect(graph, &testsource.source.pin.IPin_iface, sink, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IPin_ConnectedTo(sink, &peer);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(peer == &testsource.source.pin.IPin_iface, "Got peer %p.\n", peer);
    IPin_Release(peer);

    hr = IPin_ConnectionMediaType(sink, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&mt, &req_mt), "Media types didn't match.\n");

    ok(testdmo_input_mt_set, "Input type should be set.\n");
    ok(compare_media_types(&testdmo_input_mt, &req_mt), "Media types didn't match.\n");

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_Disconnect(graph, sink);
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

    ok(!testdmo_output_mt_set, "Output type should not be set.\n");

    /* Exact connection. */

    req_mt = mt2;

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_NOT_STOPPED, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IPin_ConnectedTo(source, &peer);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(peer == &testsink.sink.pin.IPin_iface, "Got peer %p.\n", peer);
    IPin_Release(peer);

    hr = IPin_ConnectionMediaType(source, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&mt, &req_mt), "Media types didn't match.\n");

    ok(testdmo_output_mt_set, "Output type should be set.\n");
    ok(compare_media_types(&testdmo_output_mt, &req_mt), "Media types didn't match.\n");

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_Disconnect(graph, source);
    ok(hr == VFW_E_NOT_STOPPED, "Got hr %#lx.\n", hr);
    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    test_filter_state(control);
    test_sample_processing(control, meminput, &testsink, filter);

    /* Streaming event tests are more interesting if multiple source pins are
     * connected. */

    hr = IFilterGraph2_ConnectDirect(graph, source2, &testsink2.sink.pin.IPin_iface, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    test_streaming_events(control, sink, meminput, &testsink, &testsink2);

    IFilterGraph2_Disconnect(graph, source2);
    IFilterGraph2_Disconnect(graph, &testsink2.sink.pin.IPin_iface);

    hr = IFilterGraph2_Disconnect(graph, source);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_Disconnect(graph, source);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(testsink.sink.pin.peer == source, "Got peer %p.\n", testsink.sink.pin.peer);
    IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

    peer = (IPin *)0xdeadbeef;
    hr = IPin_ConnectedTo(source, &peer);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);
    ok(!peer, "Got peer %p.\n", peer);

    hr = IPin_ConnectionMediaType(source, &mt);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    ok(!testdmo_output_mt_set, "Output type should not be set.\n");

    req_mt.lSampleSize = 0;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#lx.\n", hr);

    /* Connection with wildcards. */

    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&testsink.sink.pin.mt, &mt2), "Media types didn't match.\n");
    ok(testdmo_output_mt_set, "Output type should be set.\n");
    ok(compare_media_types(&testdmo_output_mt, &mt2), "Media types didn't match.\n");
    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

    req_mt.majortype = GUID_NULL;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&testsink.sink.pin.mt, &mt2), "Media types didn't match.\n");
    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

    req_mt.subtype = MEDIASUBTYPE_RGB32;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_NO_ACCEPTABLE_TYPES, "Got hr %#lx.\n", hr);

    req_mt.subtype = GUID_NULL;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&testsink.sink.pin.mt, &mt2), "Media types didn't match.\n");
    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

    req_mt.formattype = FORMAT_WaveFormatEx;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_NO_ACCEPTABLE_TYPES, "Got hr %#lx.\n", hr);

    req_mt = mt2;
    req_mt.formattype = GUID_NULL;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&testsink.sink.pin.mt, &mt2), "Media types didn't match.\n");
    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

    req_mt.subtype = MEDIASUBTYPE_RGB32;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_NO_ACCEPTABLE_TYPES, "Got hr %#lx.\n", hr);

    req_mt.subtype = GUID_NULL;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&testsink.sink.pin.mt, &mt2), "Media types didn't match.\n");
    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

    req_mt.majortype = MEDIATYPE_Audio;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_NO_ACCEPTABLE_TYPES, "Got hr %#lx.\n", hr);

    mt = req_mt;
    testsink.sink_mt = &mt;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&testsink.sink.pin.mt, &mt), "Media types didn't match.\n");
    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

    mt.lSampleSize = 1;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_NO_ACCEPTABLE_TYPES, "Got hr %#lx.\n", hr);
    mt.lSampleSize = 321;

    mt.majortype = mt.subtype = mt.formattype = GUID_NULL;
    req_mt = mt;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(compare_media_types(&testsink.sink.pin.mt, &mt), "Media types didn't match.\n");
    IFilterGraph2_Disconnect(graph, source);
    IFilterGraph2_Disconnect(graph, &testsink.sink.pin.IPin_iface);

    req_mt.majortype = mt2.majortype;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_NO_ACCEPTABLE_TYPES, "Got hr %#lx.\n", hr);
    req_mt.majortype = GUID_NULL;

    req_mt.subtype = mt2.subtype;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_NO_ACCEPTABLE_TYPES, "Got hr %#lx.\n", hr);
    req_mt.subtype = GUID_NULL;

    req_mt.formattype = mt2.formattype;
    hr = IFilterGraph2_ConnectDirect(graph, source, &testsink.sink.pin.IPin_iface, &req_mt);
    ok(hr == VFW_E_NO_ACCEPTABLE_TYPES, "Got hr %#lx.\n", hr);
    req_mt.formattype = GUID_NULL;

    testsink.sink_mt = NULL;

    hr = IFilterGraph2_Disconnect(graph, sink);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_Disconnect(graph, sink);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(testsource.source.pin.peer == sink, "Got peer %p.\n", testsource.source.pin.peer);
    IFilterGraph2_Disconnect(graph, &testsource.source.pin.IPin_iface);

    peer = (IPin *)0xdeadbeef;
    hr = IPin_ConnectedTo(sink, &peer);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);
    ok(!peer, "Got peer %p.\n", peer);

    hr = IPin_ConnectionMediaType(sink, &mt);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#lx.\n", hr);

    ok(!testdmo_input_mt_set, "Input type should not be set.\n");

    test_source_allocator(graph, control, sink, source, &testsink);

    IPin_Release(sink);
    IPin_Release(source);
    IPin_Release(source2);
    IMemInputPin_Release(meminput);
    IMediaControl_Release(control);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(&testsource.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(&testsink.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(&testsink2.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_uninitialized(void)
{
    IBaseFilter *filter = NULL;
    HRESULT hr;
    ULONG ref;

    hr = CoCreateInstance(&CLSID_DMOWrapperFilter, NULL,
            CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_Pause(filter);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_Stop(filter);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

START_TEST(dmowrapper)
{
    DWORD cookie;
    HRESULT hr;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = DMORegister(L"Wine test DMO", &testdmo_clsid, &DMOCATEGORY_AUDIO_DECODER, 0, 0, NULL, 0, NULL);
    if (FAILED(hr))
    {
        skip("Failed to register DMO, hr %#lx.\n", hr);
        return;
    }
    ok(hr == S_OK, "Failed to register class, hr %#lx.\n", hr);

    hr = CoRegisterClassObject(&testdmo_clsid, (IUnknown *)&testdmo_cf,
            CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &cookie);
    ok(hr == S_OK, "Failed to register class, hr %#lx.\n", hr);

    test_interfaces();
    test_aggregation();
    test_enum_pins();
    test_find_pin();
    test_pin_info();
    test_media_types();
    test_enum_media_types();
    test_connect_pin();
    test_uninitialized();

    CoRevokeClassObject(cookie);
    DMOUnregister(&testdmo_clsid, &DMOCATEGORY_AUDIO_DECODER);

    CoUninitialize();
}
