/*
 * WM ASF reader unit tests
 *
 * Copyright 2019 Zebediah Figura
 * Copyright 2020 Jactry Zeng for CodeWeavers
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

#include <stdbool.h>
#include "dshow.h"
#include "mediaobj.h"
#include "propsys.h"
#include "dvdmedia.h"
#include "wine/strmbase.h"
#include "wine/test.h"

#include "initguid.h"
#include "wmsdk.h"
#include "wmcodecdsp.h"

DEFINE_GUID(WMMEDIASUBTYPE_WMV1,0x31564d57,0x0000,0x0010,0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71);

static IBaseFilter *create_asf_reader(void)
{
    IBaseFilter *filter = NULL;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_WMAsfReader, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

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
    wcscat(pathW, name);

    file = CreateFileW(pathW, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "Failed to create file %s, error %lu.\n",
            wine_dbgstr_w(pathW), GetLastError());

    res = FindResourceW(NULL, name, (LPCWSTR)RT_RCDATA);
    ok(!!res, "Failed to load resource, error %lu.\n", GetLastError());
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
    IBaseFilter *filter = create_asf_reader();

    check_interface(filter, &IID_IBaseFilter, TRUE);
    check_interface(filter, &IID_IMediaFilter, TRUE);
    check_interface(filter, &IID_IFileSourceFilter, TRUE);
    check_interface(filter, &IID_IPersist, TRUE);
    check_interface(filter, &IID_IUnknown, TRUE);

    check_interface(filter, &IID_IAMFilterMiscFlags, FALSE);
    check_interface(filter, &IID_IMediaSeeking, FALSE);
    check_interface(filter, &IID_IReferenceClock, FALSE);
    check_interface(filter, &IID_IQualityControl, FALSE);
    todo_wine check_interface(filter, &IID_IServiceProvider, TRUE);
    todo_wine check_interface(filter, &IID_IWMHeaderInfo, TRUE);
    todo_wine check_interface(filter, &IID_IWMReaderAdvanced, TRUE);
    todo_wine check_interface(filter, &IID_IWMReaderAdvanced2, TRUE);

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
    ok(0, "Unexpected call %s.\n", wine_dbgstr_guid(iid));
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
    hr = CoCreateInstance(&CLSID_WMAsfReader, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!filter, "Got interface %p.\n", filter);

    hr = CoCreateInstance(&CLSID_WMAsfReader, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void **)&unk);
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

static bool compare_media_types(const AM_MEDIA_TYPE *a, const AM_MEDIA_TYPE *b)
{
    return !memcmp(a, b, offsetof(AM_MEDIA_TYPE, pbFormat))
            && !memcmp(a->pbFormat, b->pbFormat, a->cbFormat);
}

static void check_pin(IPin *pin, IBaseFilter *expect_filter, PIN_DIRECTION expect_dir,
        const WCHAR *expect_name, const WCHAR *expect_id, AM_MEDIA_TYPE *expect_mt, unsigned int expect_mt_count)
{
    IEnumMediaTypes *enum_mt;
    AM_MEDIA_TYPE *mt;
    unsigned int i;
    PIN_INFO info;
    HRESULT hr;
    WCHAR *id;

    hr = IPin_QueryPinInfo(pin, &info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(info.pFilter == expect_filter, "Got filter %p.\n", info.pFilter);
    ok(info.dir == expect_dir, "Got dir %#x.\n", info.dir);
    ok(!wcscmp(info.achName, expect_name), "Got name %s.\n", debugstr_w(info.achName));
    IBaseFilter_Release(info.pFilter);

    hr = IPin_QueryId(pin, &id);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(id, expect_id), "Got id %s.\n", debugstr_w(id));
    CoTaskMemFree(id);

    hr = IPin_EnumMediaTypes(pin, &enum_mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    for (i = 0; (hr = IEnumMediaTypes_Next(enum_mt, 1, &mt, NULL)) == S_OK; i++)
    {
        if (i < expect_mt_count)
        {
            todo_wine
            ok(compare_media_types(mt, expect_mt + i), "Media type %u didn't match.\n", i);
        }
        FreeMediaType(mt);
        CoTaskMemFree(mt);
    }
    todo_wine_if(IsEqualGUID(&expect_mt[0].majortype, &MEDIATYPE_Video))
    ok(i == expect_mt_count, "Got %u types.\n", i);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    IEnumMediaTypes_Release(enum_mt);

    check_interface(pin, &IID_IPin, TRUE);
    check_interface(pin, &IID_IUnknown, TRUE);
    check_interface(pin, &IID_IMediaSeeking, TRUE);
    todo_wine
    check_interface(pin, &IID_IQualityControl, TRUE);
    check_interface(pin, &IID_IServiceProvider, FALSE);
    check_interface(pin, &IID_IWMStreamConfig, FALSE);
}

static void test_filesourcefilter(void)
{
    static const MSAUDIO1WAVEFORMAT msaudio1_format =
    {
        .wfx.wFormatTag = WAVE_FORMAT_MSAUDIO1,
        .wfx.nChannels = 1,
        .wfx.nSamplesPerSec = 44100,
        .wfx.nBlockAlign = 743,
        .wfx.nAvgBytesPerSec = 16000,
        .wfx.wBitsPerSample = 16,
        .wfx.cbSize = sizeof(MSAUDIO1WAVEFORMAT) - sizeof(WAVEFORMATEX),
        .wEncodeOptions = 1,
    };
    static const VIDEOINFOHEADER2 wmv1_info2 =
    {
        .rcSource = {0, 0, 64, 48},
        .rcTarget = {0, 0, 64, 48},
        .dwBitRate = 189464,
        .dwPictAspectRatioX = 64,
        .dwPictAspectRatioY = 48,
        .bmiHeader =
        {
            .biSize = sizeof(BITMAPINFOHEADER),
            .biWidth = 64,
            .biHeight = 48,
            .biPlanes = 1,
            .biBitCount = 24,
            .biCompression = MAKEFOURCC('W','M','V','1'),
            .biSizeImage = 64 * 48 * 3,
        },
    };
    static const VIDEOINFOHEADER wmv1_info =
    {
        .rcSource = {0, 0, 64, 48},
        .rcTarget = {0, 0, 64, 48},
        .dwBitRate = 189464,
        .bmiHeader =
        {
            .biSize = sizeof(BITMAPINFOHEADER),
            .biWidth = 64,
            .biHeight = 48,
            .biPlanes = 1,
            .biBitCount = 24,
            .biCompression = MAKEFOURCC('W','M','V','1'),
            .biSizeImage = 64 * 48 * 3,
        },
    };
    AM_MEDIA_TYPE audio_mt[] =
    {
        {
            .majortype = MEDIATYPE_Audio,
            .subtype = MEDIASUBTYPE_MSAUDIO1,
            .bFixedSizeSamples = TRUE,
            .lSampleSize = 743,
            .formattype = FORMAT_WaveFormatEx,
            .cbFormat = sizeof(MSAUDIO1WAVEFORMAT),
            .pbFormat = (BYTE *)&msaudio1_format,
        },
    };
    AM_MEDIA_TYPE video_mt[] =
    {
        {
            .majortype = MEDIATYPE_Video,
            .subtype = WMMEDIASUBTYPE_WMV1,
            .formattype = FORMAT_VideoInfo2,
            .bTemporalCompression = TRUE,
            .cbFormat = sizeof(VIDEOINFOHEADER2),
            .pbFormat = (BYTE *)&wmv1_info2,
        },
        {
            .majortype = MEDIATYPE_Video,
            .subtype = WMMEDIASUBTYPE_WMV1,
            .formattype = FORMAT_VideoInfo,
            .bTemporalCompression = TRUE,
            .cbFormat = sizeof(VIDEOINFOHEADER),
            .pbFormat = (BYTE *)&wmv1_info,
        },
    };

    const WCHAR *filename = load_resource(L"test.wmv");
    IBaseFilter *filter = create_asf_reader();
    IFileSourceFilter *filesource;
    AM_MEDIA_TYPE mt, expect_mt;
    IFilterGraph2 *graph;
    IEnumPins *enumpins;
    LPOLESTR olepath;
    IPin *pins[4];
    HRESULT hr;
    ULONG ref;
    BOOL ret;

    ref = get_refcount(filter);
    ok(ref == 1, "Got unexpected refcount %ld.\n", ref);
    hr = IBaseFilter_QueryInterface(filter, &IID_IFileSourceFilter, (void **)&filesource);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ref = get_refcount(filesource);
    ok(ref == 2, "Got unexpected refcount %ld.\n", ref);
    ref = get_refcount(filter);
    ok(ref == 2, "Got unexpected refcount %ld.\n", ref);

    hr = IFileSourceFilter_Load(filesource, NULL, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    olepath = (void *)0xdeadbeef;
    memset(&mt, 0x22, sizeof(mt));
    hr = IFileSourceFilter_GetCurFile(filesource, &olepath, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!olepath, "Got %s.\n", wine_dbgstr_w(olepath));

    memset(&expect_mt, 0x22, sizeof(expect_mt));
    expect_mt.majortype = GUID_NULL;
    expect_mt.subtype = GUID_NULL;
    expect_mt.lSampleSize = 0;
    expect_mt.pUnk = NULL;
    expect_mt.cbFormat = 0;
    ok(compare_media_types(&mt, &expect_mt), "Media types didn't match.\n");

    hr = IFileSourceFilter_Load(filesource, L"nonexistent.wmv", NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFileSourceFilter_GetCurFile(filesource, NULL, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IFileSourceFilter_Load(filesource, L"nonexistent2.wmv", NULL);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    olepath = (void *)0xdeadbeef;
    memset(&mt, 0x22, sizeof(mt));
    hr = IFileSourceFilter_GetCurFile(filesource, &olepath, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(olepath, L"nonexistent.wmv"), "Expected path %s, got %s.\n",
            wine_dbgstr_w(L"nonexistent.wmv"), wine_dbgstr_w(olepath));
    ok(compare_media_types(&mt, &expect_mt), "Media types didn't match.\n");
    CoTaskMemFree(olepath);

    hr = IBaseFilter_EnumPins(filter, &enumpins);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IEnumPins_Next(enumpins, 1, pins, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    IEnumPins_Release(enumpins);

    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterGraph2, (void **)&graph);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_AddFilter(graph, filter, NULL);
    todo_wine ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)
            || broken(hr == HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND)) /* win2008 */,
            "Got hr %#lx.\n", hr);

    hr = IFileSourceFilter_Load(filesource, L"nonexistent2.wmv", NULL);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    hr = IFileSourceFilter_Load(filesource, filename, NULL);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    olepath = (void *)0xdeadbeef;
    memset(&mt, 0x22, sizeof(mt));
    hr = IFileSourceFilter_GetCurFile(filesource, &olepath, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(olepath, L"nonexistent.wmv"), "Expected path %s, got %s.\n",
            wine_dbgstr_w(L"nonexistent.wmv"), wine_dbgstr_w(olepath));
    ok(compare_media_types(&mt, &expect_mt), "Media types didn't match.\n");
    CoTaskMemFree(olepath);

    hr = IFileSourceFilter_Load(filesource, filename, NULL);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    IBaseFilter_Release(filter);
    ref = IFileSourceFilter_Release(filesource);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    /* Specify a bogus media type along with the file. */

    filter = create_asf_reader();
    hr = IBaseFilter_QueryInterface(filter, &IID_IFileSourceFilter, (void **)&filesource);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    memset(&mt, 0x11, sizeof(mt));
    hr = IFileSourceFilter_Load(filesource, L"nonexistent.wmv", &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    olepath = (void *)0xdeadbeef;
    memset(&mt, 0x22, sizeof(mt));
    hr = IFileSourceFilter_GetCurFile(filesource, &olepath, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(olepath, L"nonexistent.wmv"), "Got filename %s.\n", debugstr_w(olepath));
    ok(compare_media_types(&mt, &expect_mt), "Media types didn't match.\n");
    CoTaskMemFree(olepath);

    IBaseFilter_Release(filter);
    ref = IFileSourceFilter_Release(filesource);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    /* With a real file this time. */

    filter = create_asf_reader();
    hr = IBaseFilter_QueryInterface(filter, &IID_IFileSourceFilter, (void **)&filesource);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFileSourceFilter_Load(filesource, filename, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterGraph2, (void **)&graph);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_AddFilter(graph, filter, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    olepath = (void *)0xdeadbeef;
    memset(&mt, 0x22, sizeof(mt));
    hr = IFileSourceFilter_GetCurFile(filesource, &olepath, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(olepath, filename), "Got file %s.\n", wine_dbgstr_w(olepath));
    ok(compare_media_types(&mt, &expect_mt), "Media types didn't match.\n");
    CoTaskMemFree(olepath);

    hr = IBaseFilter_EnumPins(filter, &enumpins);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Next(enumpins, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_pin(pins[0], filter, PINDIR_OUTPUT, L"Raw Video 0", L"Raw Video 0", video_mt, ARRAY_SIZE(video_mt));
    IPin_Release(pins[0]);

    hr = IEnumPins_Next(enumpins, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_pin(pins[0], filter, PINDIR_OUTPUT, L"Raw Audio 1", L"Raw Audio 1", audio_mt, ARRAY_SIZE(audio_mt));
    IPin_Release(pins[0]);

    hr = IEnumPins_Next(enumpins, 1, pins, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    IEnumPins_Release(enumpins);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    IBaseFilter_Release(filter);
    ref = IFileSourceFilter_Release(filesource);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    /* Specify a bogus media type along with the file. */

    filter = create_asf_reader();
    hr = IBaseFilter_QueryInterface(filter, &IID_IFileSourceFilter, (void **)&filesource);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    memset(&mt, 0x11, sizeof(mt));
    hr = IFileSourceFilter_Load(filesource, filename, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    olepath = (void *)0xdeadbeef;
    memset(&mt, 0x22, sizeof(mt));
    hr = IFileSourceFilter_GetCurFile(filesource, &olepath, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(olepath, filename), "Expected filename %s, got %s.\n",
            debugstr_w(filename), debugstr_w(olepath));
    ok(compare_media_types(&mt, &expect_mt), "Media types didn't match.\n");
    CoTaskMemFree(olepath);

    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterGraph2, (void **)&graph);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph2_AddFilter(graph, filter, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    olepath = (void *)0xdeadbeef;
    memset(&mt, 0x22, sizeof(mt));
    hr = IFileSourceFilter_GetCurFile(filesource, &olepath, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(olepath, filename), "Expected filename %s, got %s.\n",
            debugstr_w(filename), debugstr_w(olepath));
    ok(compare_media_types(&mt, &expect_mt), "Media types didn't match.\n");
    CoTaskMemFree(olepath);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    IBaseFilter_Release(filter);
    ref = IFileSourceFilter_Release(filesource);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete %s, error %lu.\n", debugstr_w(filename), GetLastError());
}

static void test_filter_state(void)
{
    const WCHAR *filename = load_resource(L"test.wmv");
    IBaseFilter *filter = create_asf_reader();
    IFileSourceFilter *file_source;
    IReferenceClock *clock;
    IEnumPins *enum_pins;
    IFilterGraph *graph;
    FILTER_STATE state;
    IPin *pins[4];
    HRESULT hr;
    ULONG ref;

    hr = IBaseFilter_QueryInterface(filter, &IID_IFileSourceFilter, (void **)&file_source);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFileSourceFilter_Load(file_source, filename, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IFileSourceFilter_Release(file_source);

    hr = IBaseFilter_EnumPins(filter, &enum_pins);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IEnumPins_Next(enum_pins, 1, pins, NULL);
    todo_wine
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    if (hr == S_OK)
        IPin_Release(pins[0]);
    IEnumPins_Release(enum_pins);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %#x.\n", state);

    hr = IBaseFilter_Run(filter, GetTickCount() * 10000);
    todo_wine
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);
    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine
    ok(state == State_Stopped, "Got state %#x.\n", state);
    hr = IBaseFilter_Pause(filter);
    todo_wine
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);
    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine
    ok(state == State_Stopped, "Got state %#x.\n", state);
    hr = IBaseFilter_Stop(filter);
    todo_wine
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);
    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %#x.\n", state);

    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterGraph, (void **)&graph);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph_AddFilter(graph, filter, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ref = IFilterGraph_Release(graph);
    ok(!ref, "Got ref %ld.\n", ref);

    hr = IBaseFilter_EnumPins(filter, &enum_pins);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IEnumPins_Next(enum_pins, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IPin_Release(pins[0]);
    hr = IEnumPins_Next(enum_pins, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IPin_Release(pins[0]);
    hr = IEnumPins_Next(enum_pins, 1, pins, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    IEnumPins_Release(enum_pins);

    hr = IBaseFilter_GetSyncSource(filter, &clock);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!clock, "Got clock %p.\n", clock);

    hr = IBaseFilter_Run(filter, GetTickCount() * 10000);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Running, "Got state %#x.\n", state);

    hr = IBaseFilter_Stop(filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %#x.\n", state);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got ref %ld.\n", ref);
}

struct test_sink
{
    struct strmbase_sink sink;

    DWORD receive_tid;
    HANDLE receive_event;
    BOOL receive_can_block;
    IMemAllocator *allocator;
};

static inline struct test_sink *impl_from_IMemInputPin(IMemInputPin *iface)
{
    return CONTAINING_RECORD(iface, struct test_sink, sink.IMemInputPin_iface);
}

static HRESULT WINAPI test_mem_input_pin_QueryInterface(IMemInputPin *iface, REFIID iid, void **out)
{
    struct test_sink *pin = impl_from_IMemInputPin(iface);
    return IPin_QueryInterface(&pin->sink.pin.IPin_iface, iid, out);
}

static ULONG WINAPI test_mem_input_pin_AddRef(IMemInputPin *iface)
{
    struct test_sink *pin = impl_from_IMemInputPin(iface);
    return IPin_AddRef(&pin->sink.pin.IPin_iface);
}

static ULONG WINAPI test_mem_input_pin_Release(IMemInputPin *iface)
{
    struct test_sink *pin = impl_from_IMemInputPin(iface);
    return IPin_Release(&pin->sink.pin.IPin_iface);
}

static HRESULT WINAPI test_mem_input_pin_GetAllocator(IMemInputPin *iface,
        IMemAllocator **allocator)
{
    struct test_sink *pin = impl_from_IMemInputPin(iface);
    if (!pin->allocator)
        return VFW_E_NO_ALLOCATOR;
    IMemAllocator_AddRef((*allocator = pin->allocator));
    return S_OK;
}

static HRESULT WINAPI test_mem_input_pin_NotifyAllocator(IMemInputPin *iface,
        IMemAllocator *allocator, BOOL read_only)
{
    struct test_sink *pin = impl_from_IMemInputPin(iface);
    IMemAllocator_AddRef((pin->allocator = allocator));
    return S_OK;
}

static HRESULT WINAPI test_mem_input_pin_GetAllocatorRequirements(IMemInputPin *iface,
        ALLOCATOR_PROPERTIES *props)
{
    memset(props, 0, sizeof(*props));
    return S_OK;
}

static HRESULT WINAPI test_mem_input_pin_Receive(IMemInputPin *iface,
        IMediaSample *sample)
{
    struct test_sink *pin = impl_from_IMemInputPin(iface);

    pin->receive_tid = GetCurrentThreadId();
    SetEvent(pin->receive_event);

    todo_wine
    ok(0, "Unexpected call.\n");

    return S_OK;
}

static HRESULT WINAPI test_mem_input_pin_ReceiveMultiple(IMemInputPin *iface,
        IMediaSample **samples, LONG count, LONG *processed)
{
    struct test_sink *pin = impl_from_IMemInputPin(iface);

    pin->receive_tid = GetCurrentThreadId();
    SetEvent(pin->receive_event);

    *processed = count;
    return S_OK;
}

static HRESULT WINAPI test_mem_input_pin_ReceiveCanBlock(IMemInputPin *iface)
{
    struct test_sink *pin = impl_from_IMemInputPin(iface);
    return pin->receive_can_block ? S_OK : S_FALSE;
}

static const IMemInputPinVtbl test_mem_input_pin_vtbl =
{
    test_mem_input_pin_QueryInterface,
    test_mem_input_pin_AddRef,
    test_mem_input_pin_Release,
    test_mem_input_pin_GetAllocator,
    test_mem_input_pin_NotifyAllocator,
    test_mem_input_pin_GetAllocatorRequirements,
    test_mem_input_pin_Receive,
    test_mem_input_pin_ReceiveMultiple,
    test_mem_input_pin_ReceiveCanBlock,
};

struct test_filter
{
    struct strmbase_filter filter;
    struct test_sink video;
    struct test_sink audio;
};

static inline struct test_filter *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct test_filter, filter);
}

static inline struct strmbase_sink *impl_from_strmbase_pin(struct strmbase_pin *iface)
{
    return CONTAINING_RECORD(iface, struct strmbase_sink, pin);
}

static struct strmbase_pin *test_filter_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    struct test_filter *filter = impl_from_strmbase_filter(iface);
    if (index == 0)
        return &filter->video.sink.pin;
    if (index == 1)
        return &filter->audio.sink.pin;
    return NULL;
}

static void test_filter_destroy(struct strmbase_filter *iface)
{
    struct test_filter *filter = impl_from_strmbase_filter(iface);
    strmbase_sink_cleanup(&filter->audio.sink);
    strmbase_sink_cleanup(&filter->video.sink);
    strmbase_filter_cleanup(&filter->filter);
}

static const struct strmbase_filter_ops test_filter_ops =
{
    .filter_get_pin = test_filter_get_pin,
    .filter_destroy = test_filter_destroy,
};

static HRESULT test_sink_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct strmbase_sink *sink = impl_from_strmbase_pin(iface);

    if (IsEqualGUID(iid, &IID_IMemInputPin))
        *out = &sink->IMemInputPin_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static const struct strmbase_sink_ops test_sink_ops =
{
    .base.pin_query_interface = test_sink_query_interface,
};

static void test_filter_init(struct test_filter *filter)
{
    static const GUID clsid = {0xabacab};
    memset(filter, 0, sizeof(*filter));
    strmbase_filter_init(&filter->filter, NULL, &clsid, &test_filter_ops);
    strmbase_sink_init(&filter->video.sink, &filter->filter, L"Video", &test_sink_ops, NULL);
    filter->video.sink.IMemInputPin_iface.lpVtbl = &test_mem_input_pin_vtbl;
    strmbase_sink_init(&filter->audio.sink, &filter->filter, L"Audio", &test_sink_ops, NULL);
    filter->audio.sink.IMemInputPin_iface.lpVtbl = &test_mem_input_pin_vtbl;
}

static void test_threading(BOOL receive_can_block)
{
    static const WAVEFORMATEX pcm_format =
    {
        .wFormatTag = WAVE_FORMAT_MSAUDIO1,
        .nChannels = 1,
        .nSamplesPerSec = 44100,
        .nBlockAlign = 2,
        .nAvgBytesPerSec = 88200,
        .wBitsPerSample = 16,
        .cbSize = 0,
    };
    static const VIDEOINFOHEADER nv12_info =
    {
        .rcSource = {0, 0, 64, 48},
        .rcTarget = {0, 0, 64, 48},
        .bmiHeader =
        {
            .biSize = sizeof(BITMAPINFOHEADER),
            .biWidth = 64,
            .biHeight = 48,
            .biPlanes = 1,
            .biBitCount = 12,
            .biCompression = MAKEFOURCC('N','V','1','2'),
            .biSizeImage = 64 * 48 * 3 / 2,
        },
    };
    static const MSAUDIO1WAVEFORMAT msaudio1_format =
    {
        .wfx.wFormatTag = WAVE_FORMAT_MSAUDIO1,
        .wfx.nChannels = 1,
        .wfx.nSamplesPerSec = 44100,
        .wfx.nBlockAlign = 743,
        .wfx.nAvgBytesPerSec = 16000,
        .wfx.wBitsPerSample = 16,
        .wfx.cbSize = sizeof(MSAUDIO1WAVEFORMAT) - sizeof(WAVEFORMATEX),
        .wEncodeOptions = 1,
    };
    static const VIDEOINFOHEADER2 wmv1_info2 =
    {
        .rcSource = {0, 0, 64, 48},
        .rcTarget = {0, 0, 64, 48},
        .dwBitRate = 189464,
        .dwPictAspectRatioX = 64,
        .dwPictAspectRatioY = 48,
        .bmiHeader =
        {
            .biSize = sizeof(BITMAPINFOHEADER),
            .biWidth = 64,
            .biHeight = 48,
            .biPlanes = 1,
            .biBitCount = 24,
            .biCompression = MAKEFOURCC('W','M','V','1'),
            .biSizeImage = 64 * 48 * 3,
        },
    };
    AM_MEDIA_TYPE audio_mt =
    {
        .majortype = MEDIATYPE_Audio,
        .subtype = MEDIASUBTYPE_MSAUDIO1,
        .bFixedSizeSamples = TRUE,
        .lSampleSize = 743,
        .formattype = FORMAT_WaveFormatEx,
        .cbFormat = sizeof(MSAUDIO1WAVEFORMAT),
        .pbFormat = (BYTE *)&msaudio1_format,
    };
    AM_MEDIA_TYPE video_mt =
    {
        .majortype = MEDIATYPE_Video,
        .subtype = WMMEDIASUBTYPE_WMV1,
        .formattype = FORMAT_VideoInfo2,
        .bTemporalCompression = TRUE,
        .cbFormat = sizeof(VIDEOINFOHEADER2),
        .pbFormat = (BYTE *)&wmv1_info2,
    };
    AM_MEDIA_TYPE wine_audio_mt =
    {
        .majortype = MEDIATYPE_Audio,
        .subtype = MEDIASUBTYPE_PCM,
        .bFixedSizeSamples = TRUE,
        .formattype = FORMAT_WaveFormatEx,
        .cbFormat = sizeof(WAVEFORMATEX),
        .pbFormat = (BYTE *)&pcm_format,
    };
    AM_MEDIA_TYPE wine_video_mt =
    {
        .majortype = MEDIATYPE_Video,
        .subtype = MEDIASUBTYPE_NV12,
        .formattype = FORMAT_VideoInfo,
        .bTemporalCompression = TRUE,
        .cbFormat = sizeof(VIDEOINFOHEADER),
        .pbFormat = (BYTE *)&nv12_info,
    };

    const WCHAR *filename = load_resource(L"test.wmv");
    IBaseFilter *filter = create_asf_reader();
    IFileSourceFilter *file_source;
    struct test_filter test_sink;
    IFilterGraph *graph;
    FILTER_STATE state;
    HRESULT hr;
    IPin *pin;
    DWORD ret;
    ULONG ref;

    winetest_push_context("blocking %u", !!receive_can_block);

    test_filter_init(&test_sink);

    test_sink.video.receive_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(!!test_sink.video.receive_event, "CreateEventW failed, error %lu\n", GetLastError());
    test_sink.audio.receive_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(!!test_sink.audio.receive_event, "CreateEventW failed, error %lu\n", GetLastError());

    test_sink.video.receive_can_block = receive_can_block;
    test_sink.audio.receive_can_block = receive_can_block;

    hr = IBaseFilter_QueryInterface(filter, &IID_IFileSourceFilter, (void **)&file_source);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFileSourceFilter_Load(file_source, filename, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IFileSourceFilter_Release(file_source);

    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterGraph, (void **)&graph);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFilterGraph_AddFilter(graph, filter, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IFilterGraph_Release(graph);

    hr = IBaseFilter_FindPin(filter, L"Raw Video 0", &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IPin_Connect(pin, &test_sink.video.sink.pin.IPin_iface, &video_mt);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr != S_OK)
    {
        hr = IPin_Connect(pin, &test_sink.video.sink.pin.IPin_iface, &wine_video_mt);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
    }
    IPin_Release(pin);

    hr = IBaseFilter_FindPin(filter, L"Raw Audio 1", &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IPin_Connect(pin, &test_sink.audio.sink.pin.IPin_iface, &audio_mt);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr != S_OK)
    {
        hr = IPin_Connect(pin, &test_sink.audio.sink.pin.IPin_iface, &wine_audio_mt);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
    }
    IPin_Release(pin);

    hr = IBaseFilter_Run(filter, GetTickCount() * 10000);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Running, "Got state %#x.\n", state);

    ret = WaitForSingleObject(test_sink.video.receive_event, 500);
    ok(!ret, "Wait timed out.\n");
    ret = WaitForSingleObject(test_sink.audio.receive_event, 500);
    ok(!ret, "Wait timed out.\n");

    ok(test_sink.video.receive_tid != GetCurrentThreadId(), "got wrong thread\n");
    ok(test_sink.audio.receive_tid != GetCurrentThreadId(), "got wrong thread\n");
    if (receive_can_block)
    {
        todo_wine
        ok(test_sink.audio.receive_tid != test_sink.video.receive_tid, "got wrong thread\n");
    }
    else
        ok(test_sink.audio.receive_tid == test_sink.video.receive_tid, "got wrong thread\n");

    hr = IBaseFilter_Stop(filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %#x.\n", state);

    hr = IPin_Disconnect(&test_sink.video.sink.pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IPin_Disconnect(&test_sink.audio.sink.pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got ref %ld.\n", ref);

    if (test_sink.video.allocator)
        IMemAllocator_Release(test_sink.video.allocator);
    if (test_sink.audio.allocator)
        IMemAllocator_Release(test_sink.audio.allocator);

    CloseHandle(test_sink.video.receive_event);
    CloseHandle(test_sink.audio.receive_event);

    ref = IBaseFilter_Release(&test_sink.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);

    winetest_pop_context();
}

START_TEST(asfreader)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    test_interfaces();
    test_aggregation();
    test_filesourcefilter();
    test_filter_state();
    test_threading(FALSE);
    test_threading(TRUE);

    CoUninitialize();
}
