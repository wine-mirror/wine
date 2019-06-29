/*
 * MPEG-1 splitter filter unit tests
 *
 * Copyright 2015 Anton Baskanov
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
#include "mmreg.h"
#include "wine/test.h"

static const WCHAR mp3file[] = {'t','e','s','t','.','m','p','3',0};

static const WCHAR inputW[] = {'I','n','p','u','t',0};
static const WCHAR audioW[] = {'A','u','d','i','o',0};

static const GUID MEDIASUBTYPE_mp3 = {0x00000055,0x0000,0x0010,{0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71}};

static IBaseFilter *create_mpeg_splitter(void)
{
    IBaseFilter *filter = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_MPEG1Splitter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    return filter;
}

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

    IBaseFilter_FindPin(splitter, inputW, &sink);
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
    const WCHAR *filename = load_resource(mp3file);
    IBaseFilter *filter = create_mpeg_splitter();
    IFilterGraph2 *graph = connect_input(filter, filename);
    IPin *pin;

    check_interface(filter, &IID_IAMStreamSelect, TRUE);
    check_interface(filter, &IID_IBaseFilter, TRUE);
    check_interface(filter, &IID_IPersist, TRUE);
    check_interface(filter, &IID_IMediaFilter, TRUE);
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

    IBaseFilter_FindPin(filter, inputW, &pin);

    todo_wine check_interface(pin, &IID_IMemInputPin, TRUE);
    check_interface(pin, &IID_IPin, TRUE);
    todo_wine check_interface(pin, &IID_IQualityControl, TRUE);
    check_interface(pin, &IID_IUnknown, TRUE);

    check_interface(pin, &IID_IKsPropertySet, FALSE);
    check_interface(pin, &IID_IMediaPosition, FALSE);
    todo_wine check_interface(pin, &IID_IMediaSeeking, FALSE);

    IPin_Release(pin);

    IBaseFilter_FindPin(filter, audioW, &pin);

    check_interface(pin, &IID_IMediaSeeking, TRUE);
    check_interface(pin, &IID_IPin, TRUE);
    todo_wine check_interface(pin, &IID_IQualityControl, TRUE);
    check_interface(pin, &IID_IUnknown, TRUE);

    check_interface(pin, &IID_IAsyncReader, FALSE);
    check_interface(pin, &IID_IKsPropertySet, FALSE);
    check_interface(pin, &IID_IMediaPosition, FALSE);

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
    hr = CoCreateInstance(&CLSID_MPEG1Splitter, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == E_NOINTERFACE, "Got hr %#x.\n", hr);
    ok(!filter, "Got interface %p.\n", filter);

    hr = CoCreateInstance(&CLSID_MPEG1Splitter, &test_outer, CLSCTX_INPROC_SERVER,
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
    const WCHAR *filename = load_resource(mp3file);
    IBaseFilter *filter = create_mpeg_splitter();
    IEnumPins *enum1, *enum2;
    IFilterGraph2 *graph;
    ULONG count, ref;
    IPin *pins[3];
    HRESULT hr;
    BOOL ret;

    ref = get_refcount(filter);
    ok(ref == 1, "Got unexpected refcount %d.\n", ref);

    hr = IBaseFilter_EnumPins(filter, &enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ref = get_refcount(filter);
    ok(ref == 2, "Got unexpected refcount %d.\n", ref);
    ref = get_refcount(enum1);
    ok(ref == 1, "Got unexpected refcount %d.\n", ref);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ref = get_refcount(filter);
    ok(ref == 3, "Got unexpected refcount %d.\n", ref);
    ref = get_refcount(pins[0]);
    ok(ref == 3, "Got unexpected refcount %d.\n", ref);
    ref = get_refcount(enum1);
    ok(ref == 1, "Got unexpected refcount %d.\n", ref);
    IPin_Release(pins[0]);

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
    static const WCHAR input_pinW[] = {'i','n','p','u','t',' ','p','i','n',0};
    const WCHAR *filename = load_resource(mp3file);
    IBaseFilter *filter = create_mpeg_splitter();
    IFilterGraph2 *graph;
    IEnumPins *enum_pins;
    IPin *pin, *pin2;
    HRESULT hr;
    ULONG ref;
    BOOL ret;

    hr = IBaseFilter_FindPin(filter, input_pinW, &pin);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#x.\n", hr);

    hr = IBaseFilter_FindPin(filter, inputW, &pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    IPin_Release(pin);

    hr = IBaseFilter_FindPin(filter, audioW, &pin);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#x.\n", hr);

    graph = connect_input(filter, filename);

    hr = IBaseFilter_EnumPins(filter, &enum_pins);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum_pins, 1, &pin2, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IBaseFilter_FindPin(filter, inputW, &pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(pin == pin2, "Expected pin %p, got %p.\n", pin2, pin);
    IPin_Release(pin);
    IPin_Release(pin2);

    hr = IEnumPins_Next(enum_pins, 1, &pin2, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IBaseFilter_FindPin(filter, audioW, &pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
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
    const WCHAR *filename = load_resource(mp3file);
    IBaseFilter *filter = create_mpeg_splitter();
    ULONG ref, expect_ref;
    IFilterGraph2 *graph;
    PIN_DIRECTION dir;
    PIN_INFO info;
    HRESULT hr;
    WCHAR *id;
    IPin *pin;
    BOOL ret;

    graph = connect_input(filter, filename);

    hr = IBaseFilter_FindPin(filter, inputW, &pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    expect_ref = get_refcount(filter);
    ref = get_refcount(pin);
    ok(ref == expect_ref, "Got unexpected refcount %d.\n", ref);

    hr = IPin_QueryPinInfo(pin, &info);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(info.pFilter == filter, "Expected filter %p, got %p.\n", filter, info.pFilter);
    ok(info.dir == PINDIR_INPUT, "Got direction %d.\n", info.dir);
    ok(!lstrcmpW(info.achName, inputW), "Got name %s.\n", wine_dbgstr_w(info.achName));
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
    ok(!lstrcmpW(id, inputW), "Got id %s.\n", wine_dbgstr_w(id));
    CoTaskMemFree(id);

    IPin_Release(pin);

    hr = IBaseFilter_FindPin(filter, audioW, &pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_QueryPinInfo(pin, &info);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(info.pFilter == filter, "Expected filter %p, got %p.\n", filter, info.pFilter);
    ok(info.dir == PINDIR_OUTPUT, "Got direction %d.\n", info.dir);
    ok(!lstrcmpW(info.achName, audioW), "Got name %s.\n", wine_dbgstr_w(info.achName));
    IBaseFilter_Release(info.pFilter);

    hr = IPin_QueryDirection(pin, &dir);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(dir == PINDIR_OUTPUT, "Got direction %d.\n", dir);

    hr = IPin_QueryId(pin, &id);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!lstrcmpW(id, audioW), "Got id %s.\n", wine_dbgstr_w(id));
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
    MPEG1WAVEFORMAT expect_wfx =
    {
        {WAVE_FORMAT_MPEG, 1, 48000, 8000, 192, 0, sizeof(MPEG1WAVEFORMAT) - sizeof(WAVEFORMATEX)},
        ACM_MPEG_LAYER3, 64000, ACM_MPEG_SINGLECHANNEL, 0, 1, ACM_MPEG_PROTECTIONBIT | ACM_MPEG_ID_MPEG1, 0, 0
    };
    static const MPEGLAYER3WAVEFORMAT expect_mp3_wfx =
    {
        {WAVE_FORMAT_MPEGLAYER3, 1, 48000, 8000, 1, 0, sizeof(MPEGLAYER3WAVEFORMAT) - sizeof(WAVEFORMATEX)},
        MPEGLAYER3_ID_MPEG, 0, 192, 1, 0
    };

    const WCHAR *filename = load_resource(mp3file);
    AM_MEDIA_TYPE mt = {{0}}, *pmt, expect_mt = {{0}};
    IBaseFilter *filter = create_mpeg_splitter();
    IEnumMediaTypes *enummt;
    IFilterGraph2 *graph;
    HRESULT hr;
    ULONG ref;
    IPin *pin;
    BOOL ret;

    IBaseFilter_FindPin(filter, inputW, &pin);

    hr = IPin_EnumMediaTypes(pin, &enummt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    expect_mt.majortype = MEDIATYPE_Stream;
    expect_mt.bFixedSizeSamples = TRUE;
    expect_mt.bTemporalCompression = TRUE;
    expect_mt.lSampleSize = 1;

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    expect_mt.subtype = MEDIASUBTYPE_MPEG1System;
    if (hr == S_OK)
    {
        ok(!memcmp(pmt, &expect_mt, sizeof(AM_MEDIA_TYPE)), "Media types didn't match.\n");
        CoTaskMemFree(pmt);
    }

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    expect_mt.subtype = MEDIASUBTYPE_MPEG1VideoCD;
    if (hr == S_OK)
    {
        ok(!memcmp(pmt, &expect_mt, sizeof(AM_MEDIA_TYPE)), "Media types didn't match.\n");
        CoTaskMemFree(pmt);
    }

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    expect_mt.subtype = MEDIASUBTYPE_MPEG1Video;
    if (hr == S_OK)
    {
        ok(!memcmp(pmt, &expect_mt, sizeof(AM_MEDIA_TYPE)), "Media types didn't match.\n");
        CoTaskMemFree(pmt);
    }

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    expect_mt.subtype = MEDIASUBTYPE_MPEG1Audio;
    if (hr == S_OK)
    {
        ok(!memcmp(pmt, &expect_mt, sizeof(AM_MEDIA_TYPE)), "Media types didn't match.\n");
        CoTaskMemFree(pmt);
    }

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    IEnumMediaTypes_Release(enummt);

    mt.majortype = MEDIATYPE_Stream;
    mt.subtype = MEDIASUBTYPE_MPEG1Audio;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    mt.subtype = MEDIASUBTYPE_MPEG1Video;
    hr = IPin_QueryAccept(pin, &mt);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);

    mt.subtype = MEDIASUBTYPE_MPEG1VideoCD;
    hr = IPin_QueryAccept(pin, &mt);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);

    mt.subtype = MEDIASUBTYPE_MPEG1System;
    hr = IPin_QueryAccept(pin, &mt);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);

    mt.subtype = MEDIASUBTYPE_MPEG1AudioPayload;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    mt.subtype = MEDIASUBTYPE_MPEG1Payload;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    mt.subtype = MEDIASUBTYPE_MPEG1Packet;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    mt.subtype = GUID_NULL;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    mt.majortype = MEDIATYPE_Audio;
    mt.subtype = MEDIASUBTYPE_MPEG1Audio;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    mt.majortype = MEDIATYPE_Stream;
    mt.bFixedSizeSamples = TRUE;
    mt.bTemporalCompression = TRUE;
    mt.lSampleSize = 123;
    mt.formattype = FORMAT_WaveFormatEx;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    graph = connect_input(filter, filename);

    /* Connecting input doesn't change the reported media types. */
    hr = IPin_EnumMediaTypes(pin, &enummt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    expect_mt.subtype = MEDIASUBTYPE_MPEG1System;
    if (hr == S_OK)
    {
        ok(!memcmp(pmt, &expect_mt, sizeof(AM_MEDIA_TYPE)), "Media types didn't match.\n");
        CoTaskMemFree(pmt);
    }

    IEnumMediaTypes_Release(enummt);
    IPin_Release(pin);

    IBaseFilter_FindPin(filter, audioW, &pin);

    hr = IPin_EnumMediaTypes(pin, &enummt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(IsEqualGUID(&pmt->majortype, &MEDIATYPE_Audio), "Got major type %s.\n",
            wine_dbgstr_guid(&pmt->majortype));
    todo_wine ok(IsEqualGUID(&pmt->subtype, &MEDIASUBTYPE_MPEG1AudioPayload), "Got subtype %s.\n",
            wine_dbgstr_guid(&pmt->subtype));
    todo_wine ok(pmt->bFixedSizeSamples == TRUE, "Got fixed size %d.\n", pmt->bFixedSizeSamples);
    ok(!pmt->bTemporalCompression, "Got temporal compression %d.\n", pmt->bTemporalCompression);
    todo_wine ok(pmt->lSampleSize == 1, "Got sample size %u.\n", pmt->lSampleSize);
    ok(IsEqualGUID(&pmt->formattype, &FORMAT_WaveFormatEx), "Got format type %s.\n",
            wine_dbgstr_guid(&pmt->formattype));
    ok(!pmt->pUnk, "Got pUnk %p.\n", pmt->pUnk);
    todo_wine ok(pmt->cbFormat == sizeof(MPEG1WAVEFORMAT), "Got format size %u.\n", pmt->cbFormat);
    if (pmt->cbFormat == sizeof(MPEG1WAVEFORMAT))
    {
        /* Native will sometimes leave junk in the joint stereo flags. */
        expect_wfx.fwHeadModeExt = ((MPEG1WAVEFORMAT *)pmt->pbFormat)->fwHeadModeExt;
        ok(!memcmp(pmt->pbFormat, &expect_wfx, sizeof(MPEG1WAVEFORMAT)), "Format blocks didn't match.\n");
    }

    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    pmt->bFixedSizeSamples = FALSE;
    pmt->bTemporalCompression = TRUE;
    pmt->lSampleSize = 123;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    pmt->majortype = MEDIATYPE_Video;
    hr = IPin_QueryAccept(pin, pmt);
    todo_wine ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    pmt->majortype = MEDIATYPE_Audio;

    pmt->subtype = MEDIASUBTYPE_MPEG1Audio;
    hr = IPin_QueryAccept(pin, pmt);
    todo_wine ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    pmt->subtype = MEDIASUBTYPE_MPEG1Packet;
    hr = IPin_QueryAccept(pin, pmt);
    todo_wine ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    pmt->subtype = MEDIASUBTYPE_MPEG1Video;
    hr = IPin_QueryAccept(pin, pmt);
    todo_wine ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    pmt->subtype = MEDIASUBTYPE_MPEG1AudioPayload;

    pmt->formattype = FORMAT_None;
    hr = IPin_QueryAccept(pin, pmt);
    todo_wine ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    pmt->formattype = GUID_NULL;
    hr = IPin_QueryAccept(pin, pmt);
    todo_wine ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    pmt->formattype = FORMAT_WaveFormatEx;

    CoTaskMemFree(pmt->pbFormat);
    CoTaskMemFree(pmt);

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    if (hr != S_OK)
        goto done;
    ok(IsEqualGUID(&pmt->majortype, &MEDIATYPE_Audio), "Got major type %s.\n",
            wine_dbgstr_guid(&pmt->majortype));
    ok(IsEqualGUID(&pmt->subtype, &MEDIASUBTYPE_MPEG1Payload), "Got subtype %s.\n",
            wine_dbgstr_guid(&pmt->subtype));
    ok(pmt->bFixedSizeSamples == TRUE, "Got fixed size %d.\n", pmt->bFixedSizeSamples);
    ok(!pmt->bTemporalCompression, "Got temporal compression %d.\n", pmt->bTemporalCompression);
    ok(pmt->lSampleSize == 1, "Got sample size %u.\n", pmt->lSampleSize);
    ok(IsEqualGUID(&pmt->formattype, &FORMAT_WaveFormatEx), "Got format type %s.\n",
            wine_dbgstr_guid(&pmt->formattype));
    ok(!pmt->pUnk, "Got pUnk %p.\n", pmt->pUnk);
    ok(pmt->cbFormat == sizeof(MPEG1WAVEFORMAT), "Got format size %u.\n", pmt->cbFormat);
    /* Native will sometimes leave junk in the joint stereo flags. */
    expect_wfx.fwHeadModeExt = ((MPEG1WAVEFORMAT *)pmt->pbFormat)->fwHeadModeExt;
    ok(!memcmp(pmt->pbFormat, &expect_wfx, sizeof(MPEG1WAVEFORMAT)), "Format blocks didn't match.\n");

    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    CoTaskMemFree(pmt->pbFormat);
    CoTaskMemFree(pmt);

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(IsEqualGUID(&pmt->majortype, &MEDIATYPE_Audio), "Got major type %s.\n",
            wine_dbgstr_guid(&pmt->majortype));
    ok(IsEqualGUID(&pmt->subtype, &MEDIASUBTYPE_mp3), "Got subtype %s.\n",
            wine_dbgstr_guid(&pmt->subtype));
    ok(pmt->bFixedSizeSamples == TRUE, "Got fixed size %d.\n", pmt->bFixedSizeSamples);
    ok(!pmt->bTemporalCompression, "Got temporal compression %d.\n", pmt->bTemporalCompression);
    ok(pmt->lSampleSize == 1, "Got sample size %u.\n", pmt->lSampleSize);
    ok(IsEqualGUID(&pmt->formattype, &FORMAT_WaveFormatEx), "Got format type %s.\n",
            wine_dbgstr_guid(&pmt->formattype));
    ok(!pmt->pUnk, "Got pUnk %p.\n", pmt->pUnk);
    ok(pmt->cbFormat == sizeof(MPEGLAYER3WAVEFORMAT), "Got format size %u.\n", pmt->cbFormat);
    ok(!memcmp(pmt->pbFormat, &expect_mp3_wfx, sizeof(MPEGLAYER3WAVEFORMAT)),
            "Format blocks didn't match.\n");

    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    pmt->bFixedSizeSamples = FALSE;
    pmt->bTemporalCompression = TRUE;
    pmt->lSampleSize = 123;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    pmt->majortype = MEDIATYPE_Video;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    pmt->majortype = MEDIATYPE_Audio;

    pmt->subtype = MEDIASUBTYPE_MPEG1Audio;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    pmt->subtype = MEDIASUBTYPE_MPEG1Packet;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    pmt->subtype = MEDIASUBTYPE_MPEG1Video;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    pmt->subtype = MEDIASUBTYPE_MPEG1AudioPayload;

    pmt->formattype = FORMAT_None;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    pmt->formattype = GUID_NULL;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    pmt->formattype = FORMAT_WaveFormatEx;

    CoTaskMemFree(pmt->pbFormat);
    CoTaskMemFree(pmt);

done:
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
    const WCHAR *filename = load_resource(mp3file);
    IBaseFilter *filter = create_mpeg_splitter();
    IFilterGraph2 *graph = connect_input(filter, filename);
    IEnumMediaTypes *enum1, *enum2;
    AM_MEDIA_TYPE *mts[5];
    ULONG ref, count;
    unsigned int i;
    HRESULT hr;
    IPin *pin;
    BOOL ret;

    IBaseFilter_FindPin(filter, inputW, &pin);

    hr = IPin_EnumMediaTypes(pin, &enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    for (i = 0; i < 4; ++i)
    {
        hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
        todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
        if (hr == S_OK) CoTaskMemFree(mts[0]);
    }

    hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    for (i = 0; i < 4; ++i)
    {
        hr = IEnumMediaTypes_Next(enum1, 1, mts, &count);
        todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
        todo_wine ok(count == 1, "Got count %u.\n", count);
        if (hr == S_OK) CoTaskMemFree(mts[0]);
    }

    hr = IEnumMediaTypes_Next(enum1, 1, mts, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(!count, "Got count %u.\n", count);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 2, mts, &count);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(count == 2, "Got count %u.\n", count);
    if (count > 0) CoTaskMemFree(mts[0]);
    if (count > 1) CoTaskMemFree(mts[1]);

    hr = IEnumMediaTypes_Next(enum1, 3, mts, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    todo_wine ok(count == 2, "Got count %u.\n", count);
    if (count > 0) CoTaskMemFree(mts[0]);
    if (count > 1) CoTaskMemFree(mts[1]);

    hr = IEnumMediaTypes_Next(enum1, 2, mts, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(!count, "Got count %u.\n", count);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Clone(enum1, &enum2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 5);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 1);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 4);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 1);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enum2, 1, mts, NULL);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    if (hr == S_OK) CoTaskMemFree(mts[0]);

    IEnumMediaTypes_Release(enum1);
    IEnumMediaTypes_Release(enum2);
    IPin_Release(pin);

    IBaseFilter_FindPin(filter, audioW, &pin);

    hr = IPin_EnumMediaTypes(pin, &enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    for (i = 0; i < 3; ++i)
    {
        hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
        todo_wine_if(i) ok(hr == S_OK, "Got hr %#x.\n", hr);
        if (hr == S_OK) CoTaskMemFree(mts[0]->pbFormat);
        if (hr == S_OK) CoTaskMemFree(mts[0]);
    }

    hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    for (i = 0; i < 3; ++i)
    {
        hr = IEnumMediaTypes_Next(enum1, 1, mts, &count);
        todo_wine_if(i) ok(hr == S_OK, "Got hr %#x.\n", hr);
        todo_wine_if(i) ok(count == 1, "Got count %u.\n", count);
        if (hr == S_OK) CoTaskMemFree(mts[0]->pbFormat);
        if (hr == S_OK) CoTaskMemFree(mts[0]);
    }

    hr = IEnumMediaTypes_Next(enum1, 1, mts, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(!count, "Got count %u.\n", count);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 2, mts, &count);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(count == 2, "Got count %u.\n", count);
    CoTaskMemFree(mts[0]->pbFormat);
    CoTaskMemFree(mts[0]);
    if (count > 1) CoTaskMemFree(mts[1]->pbFormat);
    if (count > 1) CoTaskMemFree(mts[1]);

    hr = IEnumMediaTypes_Next(enum1, 2, mts, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    todo_wine ok(count == 1, "Got count %u.\n", count);
    if (count) CoTaskMemFree(mts[0]->pbFormat);
    if (count) CoTaskMemFree(mts[0]);

    hr = IEnumMediaTypes_Next(enum1, 2, mts, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(!count, "Got count %u.\n", count);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Clone(enum1, &enum2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 4);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 1);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 3);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);

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

START_TEST(mpegsplit)
{
    CoInitialize(NULL);

    test_interfaces();
    test_aggregation();
    test_enum_pins();
    test_find_pin();
    test_pin_info();
    test_media_types();
    test_enum_media_types();

    CoUninitialize();
}
