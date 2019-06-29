/*
 * AVI splitter filter unit tests
 *
 * Copyright (C) 2007 Google (Lei Zhang)
 * Copyright (C) 2008 Google (Maarten Lankhorst)
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
#include "initguid.h"
#include "wmcodecdsp.h"
#include "wine/test.h"

static const WCHAR sink_name[] = {'i','n','p','u','t',' ','p','i','n',0};
static const WCHAR source0_name[] = {'S','t','r','e','a','m',' ','0','0',0};

static IBaseFilter *create_avi_splitter(void)
{
    IBaseFilter *filter = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_AviSplitter, NULL, CLSCTX_INPROC_SERVER,
        &IID_IBaseFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    return filter;
}

static const WCHAR avifile[] = {'t','e','s','t','.','a','v','i',0};

static WCHAR *load_resource(const WCHAR *name)
{
    static WCHAR pathW[MAX_PATH];
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    GetTempPathW(ARRAY_SIZE(pathW), pathW);
    lstrcatW(pathW, name);

    file = CreateFileW(pathW, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "Failed to create file %s, error %u.\n",
            wine_dbgstr_w(pathW), GetLastError());

    res = FindResourceW(NULL, name, (LPCWSTR)RT_RCDATA);
    ok(!!res, "Failed to load resource, error %u.\n", GetLastError());
    ptr = LockResource(LoadResource(GetModuleHandleA(NULL), res));
    WriteFile(file, ptr, SizeofResource( GetModuleHandleA(NULL), res), &written, NULL);
    ok(written == SizeofResource(GetModuleHandleA(NULL), res), "Failed to write resource.\n");
    CloseHandle(file);

    return pathW;
}

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

static IFilterGraph2 *connect_input(IBaseFilter *splitter, const WCHAR *filename)
{
    static const WCHAR outputW[] = {'O','u','t','p','u','t',0};
    IFileSourceFilter *filesource;
    IFilterGraph2 *graph;
    IBaseFilter *reader;
    IPin *source, *sink;
    HRESULT hr;

    CoCreateInstance(&CLSID_AsyncReader, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&reader);
    IBaseFilter_QueryInterface(reader, &IID_IFileSourceFilter, (void **)&filesource);
    IFileSourceFilter_Load(filesource, filename, NULL);

    CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterGraph2, (void **)&graph);
    IFilterGraph2_AddFilter(graph, reader, NULL);
    IFilterGraph2_AddFilter(graph, splitter, NULL);

    IBaseFilter_FindPin(splitter, sink_name, &sink);
    IBaseFilter_FindPin(reader, outputW, &source);

    hr = IFilterGraph2_ConnectDirect(graph, source, sink, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    IPin_Release(source);
    IPin_Release(sink);
    IBaseFilter_Release(reader);
    IFileSourceFilter_Release(filesource);
    return graph;
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
    const WCHAR *filename = load_resource(avifile);
    IBaseFilter *filter = create_avi_splitter();
    IFilterGraph2 *graph = connect_input(filter, filename);
    IPin *pin;

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
    check_interface(filter, &IID_IPersistPropertyBag, FALSE);
    check_interface(filter, &IID_IPin, FALSE);
    check_interface(filter, &IID_IQualityControl, FALSE);
    check_interface(filter, &IID_IQualProp, FALSE);
    check_interface(filter, &IID_IReferenceClock, FALSE);
    check_interface(filter, &IID_IVideoWindow, FALSE);

    IBaseFilter_FindPin(filter, sink_name, &pin);

    check_interface(pin, &IID_IPin, TRUE);
    check_interface(pin, &IID_IUnknown, TRUE);

    check_interface(pin, &IID_IKsPropertySet, FALSE);
    check_interface(pin, &IID_IMemInputPin, FALSE);
    check_interface(pin, &IID_IMediaPosition, FALSE);
    todo_wine check_interface(pin, &IID_IMediaSeeking, FALSE);

    IPin_Release(pin);

    IBaseFilter_FindPin(filter, source0_name, &pin);

    todo_wine check_interface(pin, &IID_IMediaPosition, TRUE);
    check_interface(pin, &IID_IMediaSeeking, TRUE);
    check_interface(pin, &IID_IPin, TRUE);
    check_interface(pin, &IID_IUnknown, TRUE);

    check_interface(pin, &IID_IAsyncReader, FALSE);
    check_interface(pin, &IID_IKsPropertySet, FALSE);

    IPin_Release(pin);

    IBaseFilter_Release(filter);
    IFilterGraph2_Release(graph);
    DeleteFileW(filename);
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
    hr = CoCreateInstance(&CLSID_AviSplitter, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == E_NOINTERFACE, "Got hr %#x.\n", hr);
    ok(!filter, "Got interface %p.\n", filter);

    hr = CoCreateInstance(&CLSID_AviSplitter, &test_outer, CLSCTX_INPROC_SERVER,
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
    const WCHAR *filename = load_resource(avifile);
    IBaseFilter *filter = create_avi_splitter();
    IEnumPins *enum1, *enum2;
    IFilterGraph2 *graph;
    ULONG count, ref;
    IPin *pins[3];
    HRESULT hr;
    BOOL ret;

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

    graph = connect_input(filter, filename);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
todo_wine
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    IPin_Release(pins[0]);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    IPin_Release(pins[0]);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum1, 2, pins, &count);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(count == 2, "Got count %u.\n", count);
    IPin_Release(pins[0]);
    IPin_Release(pins[1]);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum1, 3, pins, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(count == 2, "Got count %u.\n", count);
    IPin_Release(pins[0]);
    IPin_Release(pins[1]);

    IEnumPins_Release(enum1);
    IFilterGraph2_Release(graph);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete file, error %u.\n", GetLastError());
}

static void test_find_pin(void)
{
    static const WCHAR inputW[] = {'I','n','p','u','t',0};
    const WCHAR *filename = load_resource(avifile);
    IBaseFilter *filter = create_avi_splitter();
    IFilterGraph2 *graph;
    IEnumPins *enum_pins;
    IPin *pin, *pin2;
    HRESULT hr;
    ULONG ref;
    BOOL ret;

    hr = IBaseFilter_FindPin(filter, sink_name, &pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    IPin_Release(pin);

    hr = IBaseFilter_FindPin(filter, source0_name, &pin);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#x.\n", hr);

    hr = IBaseFilter_FindPin(filter, inputW, &pin);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#x.\n", hr);

    graph = connect_input(filter, filename);

    hr = IBaseFilter_EnumPins(filter, &enum_pins);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum_pins, 1, &pin2, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IBaseFilter_FindPin(filter, source0_name, &pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
todo_wine
    ok(pin == pin2, "Expected pin %p, got %p.\n", pin2, pin);
    IPin_Release(pin);
    IPin_Release(pin2);

    hr = IEnumPins_Next(enum_pins, 1, &pin2, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IBaseFilter_FindPin(filter, sink_name, &pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
todo_wine
    ok(pin == pin2, "Expected pin %p, got %p.\n", pin2, pin);
    IPin_Release(pin);
    IPin_Release(pin2);

    IEnumPins_Release(enum_pins);
    IFilterGraph2_Release(graph);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete file, error %u.\n", GetLastError());
}

static void test_pin_info(void)
{
    const WCHAR *filename = load_resource(avifile);
    IBaseFilter *filter = create_avi_splitter();
    ULONG ref, expect_ref;
    IFilterGraph2 *graph;
    PIN_DIRECTION dir;
    PIN_INFO info;
    HRESULT hr;
    WCHAR *id;
    IPin *pin;
    BOOL ret;

    graph = connect_input(filter, filename);

    hr = IBaseFilter_FindPin(filter, sink_name, &pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    expect_ref = get_refcount(filter);
    ref = get_refcount(pin);
    ok(ref == expect_ref, "Got unexpected refcount %d.\n", ref);

    hr = IPin_QueryPinInfo(pin, &info);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(info.pFilter == filter, "Expected filter %p, got %p.\n", filter, info.pFilter);
    ok(info.dir == PINDIR_INPUT, "Got direction %d.\n", info.dir);
    ok(!lstrcmpW(info.achName, sink_name), "Got name %s.\n", wine_dbgstr_w(info.achName));
    ref = get_refcount(filter);
    ok(ref == expect_ref + 1, "Got unexpected refcount %d.\n", ref);
    ref = get_refcount(pin);
    ok(ref == expect_ref + 1, "Got unexpected refcount %d.\n", ref);
    IBaseFilter_Release(info.pFilter);

    hr = IPin_QueryDirection(pin, &dir);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(dir == PINDIR_INPUT, "Got direction %d.\n", dir);

    hr = IPin_QueryId(pin, &id);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!lstrcmpW(id, sink_name), "Got id %s.\n", wine_dbgstr_w(id));
    CoTaskMemFree(id);

    IPin_Release(pin);

    hr = IBaseFilter_FindPin(filter, source0_name, &pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    check_interface(pin, &IID_IPin, TRUE);
    check_interface(pin, &IID_IMediaSeeking, TRUE);

    hr = IPin_QueryPinInfo(pin, &info);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(info.pFilter == filter, "Expected filter %p, got %p.\n", filter, info.pFilter);
    ok(info.dir == PINDIR_OUTPUT, "Got direction %d.\n", info.dir);
    ok(!lstrcmpW(info.achName, source0_name), "Got name %s.\n", wine_dbgstr_w(info.achName));
    IBaseFilter_Release(info.pFilter);

    hr = IPin_QueryDirection(pin, &dir);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(dir == PINDIR_OUTPUT, "Got direction %d.\n", dir);

    hr = IPin_QueryId(pin, &id);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!lstrcmpW(id, source0_name), "Got id %s.\n", wine_dbgstr_w(id));
    CoTaskMemFree(id);

    IPin_Release(pin);

    IFilterGraph2_Release(graph);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete file, error %u.\n", GetLastError());
}

static void test_media_types(void)
{
    static const VIDEOINFOHEADER expect_vih =
    {
        {0}, {0}, 0, 0, 1000 * 10000,
        {sizeof(BITMAPINFOHEADER), 32, 24, 1, 12, mmioFOURCC('I','4','2','0'), 32*24*12/8}
    };

    const WCHAR *filename = load_resource(avifile);
    IBaseFilter *filter = create_avi_splitter();
    AM_MEDIA_TYPE mt = {{0}}, *pmt;
    IEnumMediaTypes *enummt;
    IFilterGraph2 *graph;
    HRESULT hr;
    ULONG ref;
    IPin *pin;
    BOOL ret;

    IBaseFilter_FindPin(filter, sink_name, &pin);

    hr = IPin_EnumMediaTypes(pin, &enummt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    IEnumMediaTypes_Release(enummt);

    mt.majortype = MEDIATYPE_Stream;
    mt.subtype = MEDIASUBTYPE_Avi;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    mt.bFixedSizeSamples = TRUE;
    mt.bTemporalCompression = TRUE;
    mt.lSampleSize = 123;
    mt.formattype = FORMAT_WaveFormatEx;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    mt.majortype = GUID_NULL;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    mt.majortype = MEDIATYPE_Stream;

    mt.subtype = GUID_NULL;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    mt.subtype = MEDIASUBTYPE_Avi;

    graph = connect_input(filter, filename);

    hr = IPin_EnumMediaTypes(pin, &enummt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    IEnumMediaTypes_Release(enummt);
    IPin_Release(pin);

    IBaseFilter_FindPin(filter, source0_name, &pin);

    hr = IPin_EnumMediaTypes(pin, &enummt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(IsEqualGUID(&pmt->majortype, &MEDIATYPE_Video), "Got major type %s\n",
            wine_dbgstr_guid(&pmt->majortype));
    ok(IsEqualGUID(&pmt->subtype, &MEDIASUBTYPE_I420), "Got subtype %s\n",
            wine_dbgstr_guid(&pmt->subtype));
    ok(!pmt->bFixedSizeSamples, "Got fixed size %d.\n", pmt->bFixedSizeSamples);
    todo_wine ok(!pmt->bTemporalCompression, "Got temporal compression %d.\n", pmt->bTemporalCompression);
    ok(pmt->lSampleSize == 1, "Got sample size %u.\n", pmt->lSampleSize);
    ok(IsEqualGUID(&pmt->formattype, &FORMAT_VideoInfo), "Got format type %s.\n",
            wine_dbgstr_guid(&pmt->formattype));
    ok(!pmt->pUnk, "Got pUnk %p.\n", pmt->pUnk);
    ok(pmt->cbFormat == sizeof(VIDEOINFOHEADER), "Got format size %u.\n", pmt->cbFormat);
    ok(!memcmp(pmt->pbFormat, &expect_vih, sizeof(VIDEOINFOHEADER)), "Format blocks didn't match.\n");

    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    pmt->bFixedSizeSamples = TRUE;
    pmt->bTemporalCompression = TRUE;
    pmt->lSampleSize = 123;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    pmt->majortype = GUID_NULL;
    hr = IPin_QueryAccept(pin, pmt);
    todo_wine ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    pmt->majortype = MEDIATYPE_Video;

    pmt->subtype = GUID_NULL;
    hr = IPin_QueryAccept(pin, pmt);
    todo_wine ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    pmt->subtype = MEDIASUBTYPE_I420;

    pmt->formattype = GUID_NULL;
    hr = IPin_QueryAccept(pin, pmt);
    todo_wine ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    pmt->formattype = FORMAT_None;
    hr = IPin_QueryAccept(pin, pmt);
    todo_wine ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    pmt->formattype = FORMAT_VideoInfo;

    CoTaskMemFree(pmt->pbFormat);
    CoTaskMemFree(pmt);

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    IEnumMediaTypes_Release(enummt);
    IPin_Release(pin);

    IFilterGraph2_Release(graph);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete file, error %u.\n", GetLastError());
}

static void test_enum_media_types(void)
{
    const WCHAR *filename = load_resource(avifile);
    IBaseFilter *filter = create_avi_splitter();
    IFilterGraph2 *graph = connect_input(filter, filename);
    IEnumMediaTypes *enum1, *enum2;
    AM_MEDIA_TYPE *mts[2];
    ULONG ref, count;
    HRESULT hr;
    IPin *pin;
    BOOL ret;

    IBaseFilter_FindPin(filter, sink_name, &pin);

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

    IBaseFilter_FindPin(filter, source0_name, &pin);

    hr = IPin_EnumMediaTypes(pin, &enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    CoTaskMemFree(mts[0]->pbFormat);
    CoTaskMemFree(mts[0]);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, &count);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(count == 1, "Got count %u.\n", count);
    CoTaskMemFree(mts[0]->pbFormat);
    CoTaskMemFree(mts[0]);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(!count, "Got count %u.\n", count);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 2, mts, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(count == 1, "Got count %u.\n", count);
    CoTaskMemFree(mts[0]->pbFormat);
    CoTaskMemFree(mts[0]);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Clone(enum1, &enum2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 2);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 1);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 1);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enum2, 1, mts, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    CoTaskMemFree(mts[0]->pbFormat);
    CoTaskMemFree(mts[0]);

    IEnumMediaTypes_Release(enum1);
    IEnumMediaTypes_Release(enum2);
    IPin_Release(pin);

    IFilterGraph2_Release(graph);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete file, error %u.\n", GetLastError());
}

static void test_filter_graph(void)
{
    IFileSourceFilter *pfile = NULL;
    IBaseFilter *preader = NULL, *pavi = create_avi_splitter();
    IEnumPins *enumpins = NULL;
    IPin *filepin = NULL, *avipin = NULL;
    HRESULT hr;
    HANDLE file = NULL;
    PIN_DIRECTION dir = PINDIR_OUTPUT;
    char buffer[13];
    DWORD readbytes;
    FILTER_STATE state;

    WCHAR *filename = load_resource(avifile);

    file = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        skip("Could not read test file \"%s\", skipping test\n", wine_dbgstr_w(filename));
        DeleteFileW(filename);
        return;
    }

    memset(buffer, 0, 13);
    readbytes = 12;
    ReadFile(file, buffer, readbytes, &readbytes, NULL);
    CloseHandle(file);
    if (strncmp(buffer, "RIFF", 4) || strcmp(buffer + 8, "AVI "))
    {
        skip("%s is not an avi riff file, not doing the avi splitter test\n",
            wine_dbgstr_w(filename));
        DeleteFileW(filename);
        return;
    }

    hr = IUnknown_QueryInterface(pavi, &IID_IFileSourceFilter,
        (void **)&pfile);
    ok(hr == E_NOINTERFACE,
        "Avi splitter returns unexpected error: %08x\n", hr);
    if (pfile)
        IFileSourceFilter_Release(pfile);
    pfile = NULL;

    hr = CoCreateInstance(&CLSID_AsyncReader, NULL, CLSCTX_INPROC_SERVER,
        &IID_IBaseFilter, (LPVOID*)&preader);
    ok(hr == S_OK, "Could not create asynchronous reader: %08x\n", hr);
    if (hr != S_OK)
        goto fail;

    hr = IBaseFilter_QueryInterface(preader, &IID_IFileSourceFilter,
        (void**)&pfile);
    ok(hr == S_OK, "Could not get IFileSourceFilter: %08x\n", hr);
    if (hr != S_OK)
        goto fail;

    hr = IFileSourceFilter_Load(pfile, filename, NULL);
    if (hr != S_OK)
    {
        trace("Could not load file: %08x\n", hr);
        goto fail;
    }

    hr = IBaseFilter_EnumPins(preader, &enumpins);
    ok(hr == S_OK, "No enumpins: %08x\n", hr);
    if (hr != S_OK)
        goto fail;

    hr = IEnumPins_Next(enumpins, 1, &filepin, NULL);
    ok(hr == S_OK, "No pin: %08x\n", hr);
    if (hr != S_OK)
        goto fail;

    IEnumPins_Release(enumpins);
    enumpins = NULL;

    hr = IBaseFilter_EnumPins(pavi, &enumpins);
    ok(hr == S_OK, "No enumpins: %08x\n", hr);
    if (hr != S_OK)
        goto fail;

    hr = IEnumPins_Next(enumpins, 1, &avipin, NULL);
    ok(hr == S_OK, "No pin: %08x\n", hr);
    if (hr != S_OK)
        goto fail;

    hr = IPin_Connect(filepin, avipin, NULL);
    ok(hr == S_OK, "Could not connect: %08x\n", hr);
    if (hr != S_OK)
        goto fail;

    IPin_Release(avipin);
    avipin = NULL;

    IEnumPins_Reset(enumpins);

    /* Windows puts the pins in the order: Outputpins - Inputpin,
     * wine does the reverse, just don't test it for now
     * Hate to admit it, but windows way makes more sense
     */
    while (IEnumPins_Next(enumpins, 1, &avipin, NULL) == S_OK)
    {
        IPin_QueryDirection(avipin, &dir);
        if (dir == PINDIR_OUTPUT)
        {
            /* Well, connect it to a null renderer! */
            IBaseFilter *pnull = NULL;
            IEnumPins *nullenum = NULL;
            IPin *nullpin = NULL;

            hr = CoCreateInstance(&CLSID_NullRenderer, NULL,
                CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (LPVOID*)&pnull);
            if (hr == REGDB_E_CLASSNOTREG)
            {
                win_skip("Null renderer not registered, skipping\n");
                break;
            }
            ok(hr == S_OK, "Could not create null renderer: %08x\n", hr);

            hr = IBaseFilter_EnumPins(pnull, &nullenum);
            ok(hr == S_OK, "Failed to enum pins, hr %#x.\n", hr);
            hr = IEnumPins_Next(nullenum, 1, &nullpin, NULL);
            ok(hr == S_OK, "Failed to get next pin, hr %#x.\n", hr);
            IEnumPins_Release(nullenum);
            IPin_QueryDirection(nullpin, &dir);

            hr = IPin_Connect(avipin, nullpin, NULL);
            ok(hr == S_OK, "Failed to connect output pin: %08x\n", hr);
            IPin_Release(nullpin);
            if (hr != S_OK)
            {
                IBaseFilter_Release(pnull);
                break;
            }
            IBaseFilter_Run(pnull, 0);
        }

        IPin_Release(avipin);
        avipin = NULL;
    }

    if (avipin)
        IPin_Release(avipin);
    avipin = NULL;

    if (hr != S_OK)
        goto fail2;
    /* At this point there is a minimalistic connected avi splitter that can
     * be used for all sorts of source filter tests. However that still needs
     * to be written at a later time.
     *
     * Interesting tests:
     * - Can you disconnect an output pin while running?
     *   Expecting: Yes
     * - Can you disconnect the pullpin while running?
     *   Expecting: No
     * - Is the reference count incremented during playback or when connected?
     *   Does this happen once for every output pin? Or is there something else
     *   going on.
     *   Expecting: You tell me
     */

    IBaseFilter_Run(preader, 0);
    IBaseFilter_Run(pavi, 0);
    IBaseFilter_GetState(pavi, INFINITE, &state);

    IBaseFilter_Pause(pavi);
    IBaseFilter_Pause(preader);
    IBaseFilter_Stop(pavi);
    IBaseFilter_Stop(preader);
    IBaseFilter_GetState(pavi, INFINITE, &state);
    IBaseFilter_GetState(preader, INFINITE, &state);

fail2:
    IEnumPins_Reset(enumpins);
    while (IEnumPins_Next(enumpins, 1, &avipin, NULL) == S_OK)
    {
        IPin *to = NULL;

        IPin_QueryDirection(avipin, &dir);
        IPin_ConnectedTo(avipin, &to);
        if (to)
        {
            IPin_Release(to);

            if (dir == PINDIR_OUTPUT)
            {
                PIN_INFO info;

                hr = IPin_QueryPinInfo(to, &info);
                ok(hr == S_OK, "Failed to query pin info, hr %#x.\n", hr);

                /* Release twice: Once normal, second from the
                 * previous while loop
                 */
                IBaseFilter_Stop(info.pFilter);
                IPin_Disconnect(to);
                IPin_Disconnect(avipin);
                IBaseFilter_Release(info.pFilter);
                IBaseFilter_Release(info.pFilter);
            }
            else
            {
                IPin_Disconnect(to);
                IPin_Disconnect(avipin);
            }
        }
        IPin_Release(avipin);
        avipin = NULL;
    }

fail:
    if (hr != S_OK)
        skip("Prerequisites not matched, skipping remainder of test\n");
    if (enumpins)
        IEnumPins_Release(enumpins);

    if (avipin)
        IPin_Release(avipin);
    if (filepin)
    {
        IPin *to = NULL;

        IPin_ConnectedTo(filepin, &to);
        if (to)
        {
            IPin_Disconnect(filepin);
            IPin_Disconnect(to);
        }
        IPin_Release(filepin);
    }

    if (preader)
        IBaseFilter_Release(preader);
    if (pavi)
        IBaseFilter_Release(pavi);
    if (pfile)
        IFileSourceFilter_Release(pfile);

    DeleteFileW(filename);
}

START_TEST(avisplit)
{
    CoInitialize(NULL);

    test_interfaces();
    test_aggregation();
    test_enum_pins();
    test_find_pin();
    test_pin_info();
    test_media_types();
    test_enum_media_types();
    test_filter_graph();

    CoUninitialize();
}
