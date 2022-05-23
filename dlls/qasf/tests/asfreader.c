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
static const GUID testguid = {0x22222222, 0x2222, 0x2222, {0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22}};

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
    todo_wine
    ok(!wcscmp(info.achName, expect_name), "Got name %s.\n", debugstr_w(info.achName));
    IBaseFilter_Release(info.pFilter);

    hr = IPin_QueryId(pin, &id);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine
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
    todo_wine
    ok(i == expect_mt_count, "Got %u types.\n", i);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    IEnumMediaTypes_Release(enum_mt);

    check_interface(pin, &IID_IPin, TRUE);
    check_interface(pin, &IID_IUnknown, TRUE);
    todo_wine
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
    IFilterGraph2 *graph;
    IEnumPins *enumpins;
    AM_MEDIA_TYPE type;
    LPOLESTR olepath;
    IPin *pins[4];
    HRESULT hr;
    ULONG ref;
    BYTE *ptr;

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
    memset(&type, 0x22, sizeof(type));
    hr = IFileSourceFilter_GetCurFile(filesource, &olepath, &type);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!olepath, "Got %s.\n", wine_dbgstr_w(olepath));
    ok(IsEqualGUID(&type.majortype, &MEDIATYPE_NULL), "Got majortype %s.\n",
            wine_dbgstr_guid(&type.majortype));
    ok(IsEqualGUID(&type.subtype, &MEDIASUBTYPE_NULL), "Got subtype %s.\n",
            wine_dbgstr_guid(&type.subtype));
    ok(type.bFixedSizeSamples == 0x22222222, "Got fixed size %d.\n", type.bFixedSizeSamples);
    ok(type.bTemporalCompression == 0x22222222, "Got temporal compression %d.\n", type.bTemporalCompression);
    ok(!type.lSampleSize, "Got sample size %lu.\n", type.lSampleSize);
    ok(IsEqualGUID(&type.formattype, &testguid), "Got format type %s.\n", wine_dbgstr_guid(&type.formattype));
    ok(!type.pUnk, "Got pUnk %p.\n", type.pUnk);
    ok(!type.cbFormat, "Got format size %lu.\n", type.cbFormat);
    memset(&ptr, 0x22, sizeof(ptr));
    ok(type.pbFormat == ptr, "Got format block %p.\n", type.pbFormat);

    hr = IFileSourceFilter_Load(filesource, L"nonexistent.wmv", NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IFileSourceFilter_GetCurFile(filesource, NULL, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IFileSourceFilter_Load(filesource, L"nonexistent2.wmv", NULL);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    olepath = (void *)0xdeadbeef;
    memset(&type, 0x22, sizeof(type));
    hr = IFileSourceFilter_GetCurFile(filesource, &olepath, &type);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(olepath, L"nonexistent.wmv"), "Expected path %s, got %s.\n",
            wine_dbgstr_w(L"nonexistent.wmv"), wine_dbgstr_w(olepath));
    ok(IsEqualGUID(&type.majortype, &MEDIATYPE_NULL), "Got majortype %s.\n",
            wine_dbgstr_guid(&type.majortype));
    ok(IsEqualGUID(&type.subtype, &MEDIASUBTYPE_NULL), "Got subtype %s.\n",
            wine_dbgstr_guid(&type.subtype));
    ok(type.bFixedSizeSamples == 0x22222222, "Got fixed size %d.\n", type.bFixedSizeSamples);
    ok(type.bTemporalCompression == 0x22222222, "Got temporal compression %d.\n", type.bTemporalCompression);
    ok(!type.lSampleSize, "Got sample size %lu.\n", type.lSampleSize);
    ok(IsEqualGUID(&type.formattype, &testguid), "Got format type %s.\n", wine_dbgstr_guid(&type.formattype));
    ok(!type.pUnk, "Got pUnk %p.\n", type.pUnk);
    ok(!type.cbFormat, "Got format size %lu.\n", type.cbFormat);
    ok(type.pbFormat == ptr, "Got format block %p.\n", type.pbFormat);
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

    olepath = (void *)0xdeadbeef;
    memset(&type, 0x22, sizeof(type));
    hr = IFileSourceFilter_GetCurFile(filesource, &olepath, &type);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(olepath, L"nonexistent.wmv"), "Expected path %s, got %s.\n",
            wine_dbgstr_w(L"nonexistent.wmv"), wine_dbgstr_w(olepath));
    ok(IsEqualGUID(&type.majortype, &MEDIATYPE_NULL), "Got majortype %s.\n",
            wine_dbgstr_guid(&type.majortype));
    ok(IsEqualGUID(&type.subtype, &MEDIASUBTYPE_NULL), "Got subtype %s.\n",
            wine_dbgstr_guid(&type.subtype));
    ok(type.bFixedSizeSamples == 0x22222222, "Got fixed size %d.\n", type.bFixedSizeSamples);
    ok(type.bTemporalCompression == 0x22222222, "Got temporal compression %d.\n", type.bTemporalCompression);
    ok(!type.lSampleSize, "Got sample size %lu.\n", type.lSampleSize);
    ok(IsEqualGUID(&type.formattype, &testguid), "Got format type %s.\n", wine_dbgstr_guid(&type.formattype));
    ok(!type.pUnk, "Got pUnk %p.\n", type.pUnk);
    ok(!type.cbFormat, "Got format size %lu.\n", type.cbFormat);
    ok(type.pbFormat == ptr, "Got format block %p.\n", type.pbFormat);
    CoTaskMemFree(olepath);

    hr = IFileSourceFilter_Load(filesource, filename, NULL);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    IBaseFilter_Release(filter);
    ref = IFileSourceFilter_Release(filesource);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);


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
    memset(&type, 0x22, sizeof(type));
    hr = IFileSourceFilter_GetCurFile(filesource, &olepath, &type);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(olepath, filename), "Got file %s.\n", wine_dbgstr_w(olepath));
    ok(IsEqualGUID(&type.majortype, &MEDIATYPE_NULL), "Got majortype %s.\n",
            wine_dbgstr_guid(&type.majortype));
    ok(IsEqualGUID(&type.subtype, &MEDIASUBTYPE_NULL), "Got subtype %s.\n",
            wine_dbgstr_guid(&type.subtype));
    ok(type.bFixedSizeSamples == 0x22222222, "Got fixed size %d.\n", type.bFixedSizeSamples);
    ok(type.bTemporalCompression == 0x22222222, "Got temporal compression %d.\n", type.bTemporalCompression);
    ok(!type.lSampleSize, "Got sample size %lu.\n", type.lSampleSize);
    ok(IsEqualGUID(&type.formattype, &testguid), "Got format type %s.\n", wine_dbgstr_guid(&type.formattype));
    ok(!type.pUnk, "Got pUnk %p.\n", type.pUnk);
    ok(!type.cbFormat, "Got format size %lu.\n", type.cbFormat);
    ok(type.pbFormat == ptr, "Got format block %p.\n", type.pbFormat);
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
}

START_TEST(asfreader)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    test_interfaces();
    test_aggregation();
    test_filesourcefilter();

    CoUninitialize();
}
