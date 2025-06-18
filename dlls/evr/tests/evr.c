/*
 * Enhanced Video Renderer filter unit tests
 *
 * Copyright 2018 Zebediah Figura
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
#include "wine/test.h"
#include "d3d9.h"
#include "evr.h"
#include "mferror.h"
#include "mfapi.h"
#include "initguid.h"
#include "evr9.h"

static const WCHAR sink_id[] = L"EVR Input0";

static void load_resource(const WCHAR *filename, const BYTE **data, DWORD *length)
{
    HRSRC resource = FindResourceW(NULL, filename, (const WCHAR *)RT_RCDATA);
    ok(resource != 0, "FindResourceW failed, error %lu\n", GetLastError());
    *data = LockResource(LoadResource(GetModuleHandleW(NULL), resource));
    *length = SizeofResource(GetModuleHandleW(NULL), resource);
}

static DWORD compare_rgb(const BYTE *data, DWORD *length, const SIZE *size, const RECT *rect, const BYTE *expect, UINT bits)
{
    DWORD x, y, step = bits / 8, data_size, diff = 0, width = size->cx, height = size->cy;

    /* skip BMP header from the dump */
    data_size = *(DWORD *)(expect + 2 + 2 * sizeof(DWORD));
    *length = *length + data_size;
    expect = expect + data_size;

    for (y = 0; y < height; y++, data += width * step, expect += width * step)
    {
        if (y < rect->top || y >= rect->bottom) continue;
        for (x = 0; x < width; x++)
        {
            if (x < rect->left || x >= rect->right) continue;
            diff += abs((int)expect[step * x + 0] - (int)data[step * x + 0]);
            diff += abs((int)expect[step * x + 1] - (int)data[step * x + 1]);
            if (step >= 3) diff += abs((int)expect[step * x + 2] - (int)data[step * x + 2]);
        }
    }

    data_size = (rect->right - rect->left) * (rect->bottom - rect->top) * min(step, 3);
    return diff * 100 / 256 / data_size;
}

static DWORD compare_rgb32(const BYTE *data, DWORD *length, const SIZE *size, const RECT *rect, const BYTE *expect)
{
    return compare_rgb(data, length, size, rect, expect, 32);
}

static void dump_rgb(const BYTE *data, DWORD length, const SIZE *size, HANDLE output, UINT bits)
{
    DWORD width = size->cx, height = size->cy;
    static const char magic[2] = "BM";
    struct
    {
        DWORD length;
        DWORD reserved;
        DWORD offset;
        BITMAPINFOHEADER biHeader;
    } header =
    {
        .length = length + sizeof(header) + 2, .offset = sizeof(header) + 2,
        .biHeader =
        {
            .biSize = sizeof(BITMAPINFOHEADER), .biWidth = width, .biHeight = height, .biPlanes = 1,
            .biBitCount = bits, .biCompression = BI_RGB, .biSizeImage = width * height * (bits / 8),
        },
    };
    DWORD written;
    BOOL ret;

    ret = WriteFile(output, magic, sizeof(magic), &written, NULL);
    ok(ret, "WriteFile failed, error %lu\n", GetLastError());
    ok(written == sizeof(magic), "written %lu bytes\n", written);
    ret = WriteFile(output, &header, sizeof(header), &written, NULL);
    ok(ret, "WriteFile failed, error %lu\n", GetLastError());
    ok(written == sizeof(header), "written %lu bytes\n", written);
    ret = WriteFile(output, data, length, &written, NULL);
    ok(ret, "WriteFile failed, error %lu\n", GetLastError());
    ok(written == length, "written %lu bytes\n", written);
}

static void dump_rgb32(const BYTE *data, DWORD length, const SIZE *size, HANDLE output)
{
    return dump_rgb(data, length, size, output, 32);
}

#define check_rgb32_data(a, b, c, d) check_rgb32_data_(__LINE__, a, b, c, d)
static DWORD check_rgb32_data_(int line, const WCHAR *filename, const BYTE *data, DWORD length, const RECT *rect)
{
    SIZE size = {rect->right, rect->bottom};
    WCHAR output_path[MAX_PATH];
    const BYTE *expect_data;
    HRSRC resource;
    HANDLE output;

    GetTempPathW(ARRAY_SIZE(output_path), output_path);
    lstrcatW(output_path, filename);
    output = CreateFileW(output_path, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(output != INVALID_HANDLE_VALUE, "CreateFileW failed, error %lu\n", GetLastError());
    dump_rgb32(data, length, &size, output);
    trace("created %s\n", debugstr_w(output_path));
    CloseHandle(output);

    resource = FindResourceW(NULL, filename, (const WCHAR *)RT_RCDATA);
    ok(resource != 0, "FindResourceW failed, error %lu\n", GetLastError());
    expect_data = LockResource(LoadResource(GetModuleHandleW(NULL), resource));

    return compare_rgb32(data, &length, &size, rect, expect_data);
}

static void set_rect(MFVideoNormalizedRect *rect, float left, float top, float right, float bottom)
{
    rect->left = left;
    rect->top = top;
    rect->right = right;
    rect->bottom = bottom;
}

static HWND create_window(void)
{
    RECT r = {0, 0, 640, 480};

    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW | WS_VISIBLE, FALSE);

    return CreateWindowA("static", "evr_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, r.right - r.left, r.bottom - r.top, NULL, NULL, NULL, NULL);
}

static IDirect3DDevice9 *create_device(HWND focus_window)
{
    D3DPRESENT_PARAMETERS present_parameters = {0};
    IDirect3DDevice9 *device = NULL;
    IDirect3D9 *d3d9;

    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");

    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.hDeviceWindow = focus_window;
    present_parameters.Windowed = TRUE;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;

    IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, focus_window,
            D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, &device);

    IDirect3D9_Release(d3d9);

    return device;
}

static IBaseFilter *create_evr(void)
{
    IBaseFilter *filter = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_EnhancedVideoRenderer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    return filter;
}

static IFilterGraph2 *create_graph(void)
{
    IFilterGraph2 *ret;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IFilterGraph2, (void **)&ret);
    ok(hr == S_OK, "Failed to create FilterGraph: %#lx\n", hr);
    return ret;
}

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown *obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__,line)(rc == ref, "Unexpected refcount %ld, expected %ld.\n", rc, ref);
}

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
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
    IMFVideoPresenter *presenter;
    IUnknown *unk, *unk2;
    IMFTransform *mixer;
    HRESULT hr;
    ULONG ref;

    filter = (IBaseFilter *)0xdeadbeef;
    hr = CoCreateInstance(&CLSID_EnhancedVideoRenderer, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!filter, "Got interface %p.\n", filter);

    hr = CoCreateInstance(&CLSID_EnhancedVideoRenderer, &test_outer, CLSCTX_INPROC_SERVER,
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

    /* Default presenter. */
    presenter = (void *)0xdeadbeef;
    hr = CoCreateInstance(&CLSID_MFVideoPresenter9, &test_outer, CLSCTX_INPROC_SERVER, &IID_IMFVideoPresenter,
            (void **)&presenter);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);
    ok(!presenter, "Got interface %p.\n", presenter);

    hr = CoCreateInstance(&CLSID_MFVideoPresenter9, &test_outer, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK || broken(hr == E_FAIL) /* WinXP */, "Unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
    {
        ok(outer_ref == 1, "Got unexpected refcount %ld.\n", outer_ref);
        ok(unk != &test_outer, "Returned IUnknown should not be outer IUnknown.\n");
        ref = get_refcount(unk);
        ok(ref == 1, "Got unexpected refcount %ld.\n", ref);

        IUnknown_Release(unk);
    }

    /* Default mixer. */
    presenter = (void *)0xdeadbeef;
    hr = CoCreateInstance(&CLSID_MFVideoMixer9, &test_outer, CLSCTX_INPROC_SERVER, &IID_IMFTransform,
            (void **)&mixer);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);
    ok(!mixer, "Got interface %p.\n", mixer);

    hr = CoCreateInstance(&CLSID_MFVideoMixer9, &test_outer, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(outer_ref == 1, "Got unexpected refcount %ld.\n", outer_ref);
    ok(unk != &test_outer, "Returned IUnknown should not be outer IUnknown.\n");
    ref = get_refcount(unk);
    ok(ref == 1, "Got unexpected refcount %ld.\n", ref);

    IUnknown_Release(unk);
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

static void test_interfaces(void)
{
    IBaseFilter *filter = create_evr(), *filter2;
    IUnknown *unk;
    HRESULT hr;
    ULONG ref;

    check_interface(filter, &IID_IAMFilterMiscFlags, TRUE);
    check_interface(filter, &IID_IBaseFilter, TRUE);
    check_interface(filter, &IID_IEVRFilterConfig, TRUE);
    check_interface(filter, &IID_IMFGetService, TRUE);
    check_interface(filter, &IID_IMFVideoRenderer, TRUE);
    check_interface(filter, &IID_IMediaFilter, TRUE);
    check_interface(filter, &IID_IMediaPosition, TRUE);
    check_interface(filter, &IID_IMediaSeeking, TRUE);
    check_interface(filter, &IID_IMediaEventSink, TRUE);
    check_interface(filter, &IID_IPersist, TRUE);
    check_interface(filter, &IID_IUnknown, TRUE);

    check_interface(filter, &IID_IBasicAudio, FALSE);
    check_interface(filter, &IID_IBasicVideo, FALSE);
    check_interface(filter, &IID_IDirectXVideoMemoryConfiguration, FALSE);
    check_interface(filter, &IID_IMemInputPin, FALSE);
    check_interface(filter, &IID_IPersistPropertyBag, FALSE);
    check_interface(filter, &IID_IPin, FALSE);
    check_interface(filter, &IID_IReferenceClock, FALSE);
    check_interface(filter, &IID_IVideoWindow, FALSE);

    /* The scope of IMediaEventSink */
    hr = IBaseFilter_QueryInterface(filter, &IID_IMediaEventSink, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IUnknown_QueryInterface(unk, &IID_IBaseFilter, (void **)&filter2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(filter == filter2, "Unexpected pointer.\n");
    IBaseFilter_Release(filter2);
    IUnknown_Release(unk);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got unexpected refcount %ld.\n", ref);
}

static void test_enum_pins(void)
{
    IBaseFilter *filter = create_evr();
    IEnumPins *enum1, *enum2;
    ULONG count, ref;
    IPin *pins[2];
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
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

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
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(count == 1, "Got count %lu.\n", count);
    IPin_Release(pins[0]);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Clone(enum1, &enum2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Skip(enum1, 2);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumPins_Skip(enum1, 1);
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
    IBaseFilter *filter = create_evr();
    IEnumPins *enum_pins;
    IPin *pin, *pin2;
    HRESULT hr;
    ULONG ref;

    hr = IBaseFilter_EnumPins(filter, &enum_pins);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_FindPin(filter, sink_id, &pin);
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
    IBaseFilter *filter = create_evr();
    PIN_DIRECTION dir;
    PIN_INFO info;
    HRESULT hr;
    WCHAR *id;
    ULONG ref;
    IPin *pin;

    hr = IBaseFilter_FindPin(filter, sink_id, &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ref = get_refcount(filter);
    ok(ref == 2, "Got unexpected refcount %ld.\n", ref);
    ref = get_refcount(pin);
    ok(ref == 2, "Got unexpected refcount %ld.\n", ref);

    hr = IPin_QueryPinInfo(pin, &info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(info.pFilter == filter, "Expected filter %p, got %p.\n", filter, info.pFilter);
    ok(info.dir == PINDIR_INPUT, "Got direction %d.\n", info.dir);
    ok(!lstrcmpW(info.achName, sink_id), "Got name %s.\n", wine_dbgstr_w(info.achName));
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
    ok(!lstrcmpW(id, sink_id), "Got id %s.\n", wine_dbgstr_w(id));
    CoTaskMemFree(id);

    hr = IPin_QueryInternalConnections(pin, NULL, NULL);
    ok(hr == E_NOTIMPL, "Got hr %#lx.\n", hr);

    IPin_Release(pin);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static unsigned int check_event_code(IMediaEvent *eventsrc, DWORD timeout, LONG expected_code, LONG_PTR expected1, LONG_PTR expected2)
{
    LONG_PTR param1, param2;
    unsigned int ret = 0;
    HRESULT hr;
    LONG code;

    while ((hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, timeout)) == S_OK)
    {
        if (code == expected_code)
        {
            ok(param1 == expected1, "Got param1 %#Ix.\n", param1);
            ok(param2 == expected2, "Got param2 %#Ix.\n", param2);
            ret++;
        }
        IMediaEvent_FreeEventParams(eventsrc, code, param1, param2);
        timeout = 0;
    }
    ok(hr == E_ABORT, "Got hr %#lx.\n", hr);

    return ret;
}

static unsigned int check_ec_complete(IMediaEvent *eventsrc, DWORD timeout)
{
    return check_event_code(eventsrc, timeout, EC_COMPLETE, S_OK, 0);
}

static void test_unconnected_eos(void)
{
    IBaseFilter *filter = create_evr();
    IFilterGraph2 *graph = create_graph();
    IMediaControl *control;
    IMediaEvent *eventsrc;
    unsigned int ret;
    HRESULT hr;
    ULONG ref;

    hr = IFilterGraph2_AddFilter(graph, filter, L"renderer");
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterGraph2_QueryInterface(graph, &IID_IMediaEvent, (void **)&eventsrc);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ret = check_ec_complete(eventsrc, 0);
    ok(!ret, "Got %u EC_COMPLETE events.\n", ret);

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ret = check_ec_complete(eventsrc, 0);
    ok(!ret, "Got %u EC_COMPLETE events.\n", ret);

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ret = check_ec_complete(eventsrc, 0);
    ok(ret == 1, "Got %u EC_COMPLETE events.\n", ret);

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ret = check_ec_complete(eventsrc, 0);
    ok(!ret, "Got %u EC_COMPLETE events.\n", ret);

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ret = check_ec_complete(eventsrc, 0);
    ok(ret == 1, "Got %u EC_COMPLETE events.\n", ret);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ret = check_ec_complete(eventsrc, 0);
    ok(!ret, "Got %u EC_COMPLETE events.\n", ret);

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ret = check_ec_complete(eventsrc, 0);
    ok(ret == 1, "Got %u EC_COMPLETE events.\n", ret);

    IMediaControl_Release(control);
    IMediaEvent_Release(eventsrc);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_misc_flags(void)
{
    IBaseFilter *filter = create_evr();
    IAMFilterMiscFlags *misc_flags;
    ULONG ref, flags;
    HRESULT hr;

    hr = IBaseFilter_QueryInterface(filter, &IID_IAMFilterMiscFlags, (void **)&misc_flags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    flags = IAMFilterMiscFlags_GetMiscFlags(misc_flags);
    ok(flags == AM_FILTER_MISC_FLAGS_IS_RENDERER, "Unexpected flags %#lx.\n", flags);
    IAMFilterMiscFlags_Release(misc_flags);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_display_control(void)
{
    IBaseFilter *filter = create_evr();
    IMFVideoDisplayControl *display_control;
    HRESULT hr;
    ULONG ref;

    hr = MFGetService((IUnknown *)filter, &MR_VIDEO_RENDER_SERVICE,
            &IID_IMFVideoDisplayControl, (void **)&display_control);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoDisplayControl_SetVideoWindow(display_control, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    IMFVideoDisplayControl_Release(display_control);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_service_lookup(void)
{
    IBaseFilter *filter = create_evr();
    IMFTopologyServiceLookup *service_lookup;
    IUnknown *unk;
    DWORD count;
    HRESULT hr;
    ULONG ref;

    hr = IBaseFilter_QueryInterface(filter, &IID_IMFTopologyServiceLookup, (void **)&service_lookup);
    if (FAILED(hr))
    {
        win_skip("IMFTopologyServiceLookup is not exposed.\n");
        IBaseFilter_Release(filter);
        return;
    }
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopologyServiceLookup_QueryInterface(service_lookup, &IID_IBaseFilter, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(unk == (IUnknown *)filter, "Unexpected pointer.\n");
    IUnknown_Release(unk);

    count = 1;
    hr = IMFTopologyServiceLookup_LookupService(service_lookup, MF_SERVICE_LOOKUP_GLOBAL, 0,
            &MR_VIDEO_RENDER_SERVICE, &IID_IMediaEventSink, (void **)&unk, &count);
    ok(hr == MF_E_NOTACCEPTING, "Unexpected hr %#lx.\n", hr);

    IMFTopologyServiceLookup_Release(service_lookup);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_query_accept(void)
{
    IBaseFilter *filter = create_evr();
    AM_MEDIA_TYPE req_mt = {{0}};
    VIDEOINFOHEADER vih =
    {
        {0}, {0}, 0, 0, 0,
        {sizeof(BITMAPINFOHEADER), 32, 24, 1, 0, 0xdeadbeef}
    };
    unsigned int i;
    HRESULT hr;
    ULONG ref;
    IPin *pin;

    static const GUID *subtype_tests[] =
    {
        &MEDIASUBTYPE_RGB32,
        &MEDIASUBTYPE_YUY2,
    };

    static const GUID *unsupported_subtype_tests[] =
    {
        &MEDIASUBTYPE_RGB8,
    };

    IBaseFilter_FindPin(filter, L"EVR Input0", &pin);

    req_mt.majortype = MEDIATYPE_Video;
    req_mt.formattype = FORMAT_VideoInfo;
    req_mt.cbFormat = sizeof(VIDEOINFOHEADER);
    req_mt.pbFormat = (BYTE *)&vih;

    for (i = 0; i < ARRAY_SIZE(subtype_tests); ++i)
    {
        memcpy(&req_mt.subtype, subtype_tests[i], sizeof(GUID));
        hr = IPin_QueryAccept(pin, &req_mt);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
    }

    for (i = 0; i < ARRAY_SIZE(unsupported_subtype_tests); ++i)
    {
        memcpy(&req_mt.subtype, unsupported_subtype_tests[i], sizeof(GUID));
        hr = IPin_QueryAccept(pin, &req_mt);
        ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    }

    IPin_Release(pin);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static IMFMediaType * create_video_type(const GUID *subtype)
{
    IMFMediaType *video_type;
    HRESULT hr;

    hr = MFCreateMediaType(&video_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(video_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(video_type, &MF_MT_SUBTYPE, subtype);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    return video_type;
}

static void test_default_mixer(void)
{
    DWORD input_min, input_max, output_min, output_max;
    IMFAttributes *attributes, *attributes2;
    IMFVideoMixerControl2 *mixer_control2;
    MFT_OUTPUT_STREAM_INFO output_info;
    MFT_INPUT_STREAM_INFO input_info;
    DWORD input_count, output_count;
    IMFVideoProcessor *processor;
    IMFVideoDeviceID *deviceid;
    MFVideoNormalizedRect rect;
    DWORD input_id, output_id;
    IMFTransform *transform;
    DXVA2_ValueRange range;
    DXVA2_Fixed32 dxva_value;
    UINT32 count, value;
    COLORREF color;
    unsigned int i;
    DWORD ids[16];
    IUnknown *unk;
    DWORD flags;
    GUID *guids;
    HRESULT hr;
    IID iid;

    hr = MFCreateVideoMixer(NULL, &IID_IDirect3DDevice9, &IID_IMFTransform, (void **)&transform);
    ok(hr == S_OK, "Failed to create default mixer, hr %#lx.\n", hr);

    check_interface(transform, &IID_IMFQualityAdvise, TRUE);
    check_interface(transform, &IID_IMFClockStateSink, TRUE);
    check_interface(transform, &IID_IMFTopologyServiceLookupClient, TRUE);
    check_interface(transform, &IID_IMFGetService, TRUE);
    check_interface(transform, &IID_IMFAttributes, TRUE);
    check_interface(transform, &IID_IMFVideoMixerBitmap, TRUE);
    check_interface(transform, &IID_IMFVideoPositionMapper, TRUE);
    check_interface(transform, &IID_IMFVideoProcessor, TRUE);
    check_interface(transform, &IID_IMFVideoMixerControl, TRUE);
    check_interface(transform, &IID_IMFVideoDeviceID, TRUE);
    check_service_interface(transform, &MR_VIDEO_MIXER_SERVICE, &IID_IMFVideoMixerBitmap, TRUE);
    check_service_interface(transform, &MR_VIDEO_MIXER_SERVICE, &IID_IMFVideoProcessor, TRUE);
    check_service_interface(transform, &MR_VIDEO_MIXER_SERVICE, &IID_IMFVideoMixerControl, TRUE);
    check_service_interface(transform, &MR_VIDEO_MIXER_SERVICE, &IID_IMFVideoPositionMapper, TRUE);
    check_service_interface(transform, &MR_VIDEO_MIXER_SERVICE, &IID_IMFTransform, FALSE);

    hr = MFGetService((IUnknown *)transform, &MR_VIDEO_RENDER_SERVICE, &IID_IUnknown, (void **)&unk);
    ok(hr == MF_E_UNSUPPORTED_SERVICE, "Unexpected hr %#lx.\n", hr);

    if (SUCCEEDED(MFGetService((IUnknown *)transform, &MR_VIDEO_MIXER_SERVICE, &IID_IMFVideoMixerControl2, (void **)&mixer_control2)))
    {
        hr = IMFVideoMixerControl2_GetMixingPrefs(mixer_control2, NULL);
        ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

        hr = IMFVideoMixerControl2_GetMixingPrefs(mixer_control2, &flags);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!flags, "Unexpected flags %#lx.\n", flags);

        IMFVideoMixerControl2_Release(mixer_control2);
    }

    hr = MFGetService((IUnknown *)transform, &MR_VIDEO_MIXER_SERVICE, &IID_IMFVideoProcessor, (void **)&processor);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoProcessor_GetBackgroundColor(processor, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    color = 1;
    hr = IMFVideoProcessor_GetBackgroundColor(processor, &color);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!color, "Unexpected color %#lx.\n", color);

    hr = IMFVideoProcessor_SetBackgroundColor(processor, 0x00121212);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoProcessor_GetBackgroundColor(processor, &color);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(color == 0x121212, "Unexpected color %#lx.\n", color);

    hr = IMFVideoProcessor_GetFilteringRange(processor, DXVA2_DetailFilterChromaLevel, &range);
    todo_wine
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoProcessor_GetFilteringValue(processor, DXVA2_DetailFilterChromaLevel, &dxva_value);
    todo_wine
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoProcessor_GetAvailableVideoProcessorModes(processor, &count, &guids);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    IMFVideoProcessor_Release(processor);

    hr = IMFTransform_SetOutputBounds(transform, 100, 10);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_QueryInterface(transform, &IID_IMFVideoDeviceID, (void **)&deviceid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetAttributes(transform, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetAttributes(transform, &attributes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetAttributes(transform, &attributes2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(attributes == attributes2, "Unexpected attributes instance.\n");
    IMFAttributes_Release(attributes2);

    hr = IMFTransform_QueryInterface(transform, &IID_IMFAttributes, (void **)&attributes2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(attributes != attributes2, "Unexpected attributes instance.\n");

    hr = IMFAttributes_QueryInterface(attributes2, &IID_IMFTransform, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IUnknown_Release(unk);

    hr = IMFAttributes_GetCount(attributes2, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected attribute count %u.\n", count);

    value = 0;
    hr = IMFAttributes_GetUINT32(attributes2, &MF_SA_D3D_AWARE, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value == 1, "Unexpected value %d.\n", value);

    IMFAttributes_Release(attributes2);

    hr = IMFAttributes_GetCount(attributes, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected attribute count %u.\n", count);

    memset(&rect, 0, sizeof(rect));
    hr = IMFAttributes_GetBlob(attributes, &VIDEO_ZOOM_RECT, (UINT8 *)&rect, sizeof(rect), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(rect.left == 0.0f && rect.top == 0.0f && rect.right == 1.0f && rect.bottom == 1.0f,
            "Unexpected zoom rect (%f, %f) - (%f, %f).\n", rect.left, rect.top, rect.right, rect.bottom);

    IMFAttributes_Release(attributes);

    hr = IMFVideoDeviceID_GetDeviceID(deviceid, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoDeviceID_GetDeviceID(deviceid, &iid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualIID(&iid, &IID_IDirect3DDevice9), "Unexpected id %s.\n", wine_dbgstr_guid(&iid));

    IMFVideoDeviceID_Release(deviceid);

    /* Stream configuration. */
    input_count = output_count = 0;
    hr = IMFTransform_GetStreamCount(transform, &input_count, &output_count);
    ok(hr == S_OK, "Failed to get stream count, hr %#lx.\n", hr);
    ok(input_count == 1 && output_count == 1, "Unexpected stream count %lu/%lu.\n", input_count, output_count);

    hr = IMFTransform_GetStreamLimits(transform, &input_min, &input_max, &output_min, &output_max);
    ok(hr == S_OK, "Failed to get stream limits, hr %#lx.\n", hr);
    ok(input_min == 1 && input_max == 16 && output_min == 1 && output_max == 1, "Unexpected stream limits %lu/%lu, %lu/%lu.\n",
            input_min, input_max, output_min, output_max);

    hr = IMFTransform_GetInputStreamInfo(transform, 1, &input_info);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputStreamInfo(transform, 1, &output_info);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    memset(&input_info, 0xcc, sizeof(input_info));
    hr = IMFTransform_GetInputStreamInfo(transform, 0, &input_info);
    ok(hr == S_OK, "Failed to get input info, hr %#lx.\n", hr);

    memset(&output_info, 0xcc, sizeof(output_info));
    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &output_info);
    ok(hr == S_OK, "Failed to get input info, hr %#lx.\n", hr);
    ok(!(output_info.dwFlags & (MFT_OUTPUT_STREAM_PROVIDES_SAMPLES | MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES)),
            "Unexpected output flags %#lx.\n", output_info.dwFlags);

    hr = IMFTransform_GetStreamIDs(transform, 1, &input_id, 1, &output_id);
    ok(hr == S_OK, "Failed to get input info, hr %#lx.\n", hr);
    ok(input_id == 0 && output_id == 0, "Unexpected stream ids.\n");

    hr = IMFTransform_GetInputStreamAttributes(transform, 1, &attributes);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputStreamAttributes(transform, 1, &attributes);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputStreamAttributes(transform, 0, &attributes);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_AddInputStreams(transform, 16, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_AddInputStreams(transform, 16, ids);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    memset(ids, 0, sizeof(ids));
    hr = IMFTransform_AddInputStreams(transform, 15, ids);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(ids); ++i)
        ids[i] = i + 1;

    hr = IMFTransform_AddInputStreams(transform, 15, ids);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    input_count = output_count = 0;
    hr = IMFTransform_GetStreamCount(transform, &input_count, &output_count);
    ok(hr == S_OK, "Failed to get stream count, hr %#lx.\n", hr);
    ok(input_count == 16 && output_count == 1, "Unexpected stream count %lu/%lu.\n", input_count, output_count);

    memset(&input_info, 0, sizeof(input_info));
    hr = IMFTransform_GetInputStreamInfo(transform, 1, &input_info);
    ok(hr == S_OK, "Failed to get input info, hr %#lx.\n", hr);
    ok((input_info.dwFlags & (MFT_INPUT_STREAM_REMOVABLE | MFT_INPUT_STREAM_OPTIONAL)) ==
            (MFT_INPUT_STREAM_REMOVABLE | MFT_INPUT_STREAM_OPTIONAL), "Unexpected flags %#lx.\n", input_info.dwFlags);

    attributes = NULL;
    hr = IMFTransform_GetInputStreamAttributes(transform, 0, &attributes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_GetCount(attributes, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);
    hr = IMFAttributes_GetUINT32(attributes, &MF_SA_REQUIRED_SAMPLE_COUNT, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);
    ok(!!attributes, "Unexpected attributes.\n");

    attributes2 = NULL;
    hr = IMFTransform_GetInputStreamAttributes(transform, 0, &attributes2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(attributes == attributes2, "Unexpected instance.\n");

    IMFAttributes_Release(attributes2);
    IMFAttributes_Release(attributes);

    attributes = NULL;
    hr = IMFTransform_GetInputStreamAttributes(transform, 1, &attributes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!attributes, "Unexpected attributes.\n");
    IMFAttributes_Release(attributes);

    hr = IMFTransform_DeleteInputStream(transform, 0);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_DeleteInputStream(transform, 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    input_count = output_count = 0;
    hr = IMFTransform_GetStreamCount(transform, &input_count, &output_count);
    ok(hr == S_OK, "Failed to get stream count, hr %#lx.\n", hr);
    ok(input_count == 15 && output_count == 1, "Unexpected stream count %lu/%lu.\n", input_count, output_count);

    IMFTransform_Release(transform);

    hr = MFCreateVideoMixer(NULL, &IID_IMFTransform, &IID_IMFTransform, (void **)&transform);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = CoCreateInstance(&CLSID_MFVideoMixer9, NULL, CLSCTX_INPROC_SERVER, &IID_IMFTransform, (void **)&transform);
    ok(hr == S_OK, "Failed to create default mixer, hr %#lx.\n", hr);
    IMFTransform_Release(transform);
}

static void test_surface_sample(void)
{
    IDirect3DSurface9 *backbuffer = NULL, *surface;
    DWORD flags, buffer_count, length;
    IMFDesiredSample *desired_sample;
    IMFMediaBuffer *buffer, *buffer2;
    LONGLONG duration, time1, time2;
    IDirect3DSwapChain9 *swapchain;
    IDirect3DDevice9 *device;
    IMFSample *sample;
    IUnknown *unk;
    UINT32 count;
    HWND window;
    HRESULT hr;
    BYTE *data;

    window = create_window();
    if (!(device = create_device(window)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetSwapChain(device, 0, &swapchain);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DSwapChain9_GetBackBuffer(swapchain, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(backbuffer != NULL, "The back buffer is NULL\n");

    IDirect3DSwapChain9_Release(swapchain);

    hr = MFCreateVideoSampleFromSurface(NULL, &sample);
    ok(hr == S_OK, "Failed to create surface sample, hr %#lx.\n", hr);
    IMFSample_Release(sample);

    hr = MFCreateVideoSampleFromSurface((IUnknown *)backbuffer, &sample);
    ok(hr == S_OK, "Failed to create surface sample, hr %#lx.\n", hr);

    hr = IMFSample_QueryInterface(sample, &IID_IMFTrackedSample, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IUnknown_Release(unk);

    hr = IMFSample_QueryInterface(sample, &IID_IMFDesiredSample, (void **)&desired_sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_GetCount(sample, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!count, "Unexpected attribute count %u.\n", count);

    hr = IMFDesiredSample_GetDesiredSampleTimeAndDuration(desired_sample, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFDesiredSample_GetDesiredSampleTimeAndDuration(desired_sample, NULL, &time2);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFDesiredSample_GetDesiredSampleTimeAndDuration(desired_sample, &time1, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFDesiredSample_GetDesiredSampleTimeAndDuration(desired_sample, &time1, &time2);
    ok(hr == MF_E_NOT_AVAILABLE, "Unexpected hr %#lx.\n", hr);

    IMFDesiredSample_SetDesiredSampleTimeAndDuration(desired_sample, 123, 456);

    time1 = time2 = 0;
    hr = IMFDesiredSample_GetDesiredSampleTimeAndDuration(desired_sample, &time1, &time2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(time1 == 123 && time2 == 456, "Unexpected time values.\n");

    IMFDesiredSample_SetDesiredSampleTimeAndDuration(desired_sample, 0, 0);

    time1 = time2 = 1;
    hr = IMFDesiredSample_GetDesiredSampleTimeAndDuration(desired_sample, &time1, &time2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(time1 == 0 && time2 == 0, "Unexpected time values.\n");

    IMFDesiredSample_Clear(desired_sample);

    hr = IMFDesiredSample_GetDesiredSampleTimeAndDuration(desired_sample, &time1, &time2);
    ok(hr == MF_E_NOT_AVAILABLE, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_GetCount(sample, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!count, "Unexpected attribute count %u.\n", count);

    /* Attributes are cleared. */
    hr = IMFSample_SetUnknown(sample, &MFSampleExtension_Token, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_GetCount(sample, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected attribute count %u.\n", count);

    hr = IMFSample_SetSampleTime(sample, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_GetSampleTime(sample, &time1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_SetSampleDuration(sample, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_GetSampleDuration(sample, &duration);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_SetSampleFlags(sample, 0x1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFDesiredSample_Clear(desired_sample);

    hr = IMFSample_GetCount(sample, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!count, "Unexpected attribute count %u.\n", count);

    hr = IMFSample_GetSampleTime(sample, &time1);
    ok(hr == MF_E_NO_SAMPLE_TIMESTAMP, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_GetSampleDuration(sample, &duration);
    ok(hr == MF_E_NO_SAMPLE_DURATION, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_GetSampleFlags(sample, &flags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!flags, "Unexpected flags %#lx.\n", flags);

    IMFDesiredSample_Release(desired_sample);

    hr = IMFSample_GetCount(sample, &count);
    ok(hr == S_OK, "Failed to get attribute count, hr %#lx.\n", hr);
    ok(!count, "Unexpected attribute count.\n");

    buffer_count = 0;
    hr = IMFSample_GetBufferCount(sample, &buffer_count);
    ok(hr == S_OK, "Failed to get buffer count, hr %#lx.\n", hr);
    ok(buffer_count == 1, "Unexpected attribute count.\n");

    hr = IMFSample_GetTotalLength(sample, &length);
    ok(hr == S_OK, "Failed to get length, hr %#lx.\n", hr);
    ok(!length, "Unexpected length %lu.\n", length);

    hr = IMFSample_GetSampleDuration(sample, &duration);
    ok(hr == MF_E_NO_SAMPLE_DURATION, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_GetSampleTime(sample, &duration);
    ok(hr == MF_E_NO_SAMPLE_TIMESTAMP, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_GetBufferByIndex(sample, 0, &buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaBuffer_GetMaxLength(buffer, &length);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaBuffer_GetCurrentLength(buffer, &length);
    ok(hr == S_OK, "Failed to get length, hr %#lx.\n", hr);
    ok(!length, "Unexpected length %lu.\n", length);

    hr = IMFMediaBuffer_SetCurrentLength(buffer, 16);
    ok(hr == S_OK, "Failed to get length, hr %#lx.\n", hr);

    hr = IMFMediaBuffer_GetCurrentLength(buffer, &length);
    ok(hr == S_OK, "Failed to get length, hr %#lx.\n", hr);
    ok(length == 16, "Unexpected length %lu.\n", length);

    hr = IMFMediaBuffer_Lock(buffer, &data, NULL, NULL);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer, (void **)&unk);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_AddBuffer(sample, buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_GetBufferCount(sample, &buffer_count);
    ok(hr == S_OK, "Failed to get buffer count, hr %#lx.\n", hr);
    ok(buffer_count == 2, "Unexpected buffer count.\n");

    hr = IMFSample_ConvertToContiguousBuffer(sample, &buffer2);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_CopyToBuffer(sample, buffer);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_RemoveAllBuffers(sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_GetBufferCount(sample, &buffer_count);
    ok(hr == S_OK, "Failed to get buffer count, hr %#lx.\n", hr);
    ok(!buffer_count, "Unexpected buffer count.\n");

    hr = MFGetService((IUnknown *)buffer, &MR_BUFFER_SERVICE, &IID_IDirect3DSurface9, (void **)&surface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(surface == backbuffer, "Unexpected instance.\n");
    IDirect3DSurface9_Release(surface);

    hr = MFGetService((IUnknown *)buffer, &MR_BUFFER_SERVICE, &IID_IUnknown, (void **)&surface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(surface == backbuffer, "Unexpected instance.\n");
    IDirect3DSurface9_Release(surface);

    IMFMediaBuffer_Release(buffer);

    hr = IMFSample_GetSampleFlags(sample, &flags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_SetSampleFlags(sample, 0x123);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    flags = 0;
    hr = IMFSample_GetSampleFlags(sample, &flags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(flags == 0x123, "Unexpected flags %#lx.\n", flags);

    IMFSample_Release(sample);
    if (backbuffer)
        IDirect3DSurface9_Release(backbuffer);
    ok(!IDirect3DDevice9_Release(device), "Unexpected refcount.\n");

done:
    DestroyWindow(window);
}

static void test_default_mixer_type_negotiation(void)
{
    IMFMediaType *media_type, *media_type2;
    IDirect3DDeviceManager9 *manager;
    DXVA2_VideoProcessorCaps caps;
    IMFVideoProcessor *processor;
    GUID subtype, guid, *guids;
    IDirect3DDevice9 *device;
    IMFMediaType *video_type;
    IMFTransform *transform;
    MFVideoArea aperture;
    UINT count, token;
    IUnknown *unk;
    DWORD index;
    HWND window;
    HRESULT hr;

    hr = MFCreateVideoMixer(NULL, &IID_IDirect3DDevice9, &IID_IMFTransform, (void **)&transform);
    ok(hr == S_OK, "Failed to create default mixer, hr %#lx.\n", hr);

    hr = IMFTransform_QueryInterface(transform, &IID_IMFVideoProcessor, (void **)&processor);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetInputAvailableType(transform, 0, 0, &media_type);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &media_type);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
    ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_SetInputType(transform, 0, media_type, MFT_SET_TYPE_TEST_ONLY);
    ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoProcessor_GetAvailableVideoProcessorModes(processor, &count, &guids);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    /* Now try with device manager. */

    window = create_window();
    if (!(device = create_device(window)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IMFTransform_SetInputType(transform, 0, NULL, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTransform_SetInputType(transform, 0, NULL, MFT_SET_TYPE_TEST_ONLY);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = DXVA2CreateDirect3DDeviceManager9(&token, &manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Now manager is not initialized. */
    hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
    ok(hr == DXVA2_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_SetInputType(transform, 0, media_type, MFT_SET_TYPE_TEST_ONLY);
    ok(hr == DXVA2_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_ResetDevice(manager, device, token);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* And now type description is incomplete. */
    hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(media_type);

    video_type = create_video_type(&MFVideoFormat_RGB32);

    /* Partially initialized type. */
    hr = IMFTransform_SetInputType(transform, 0, video_type, 0);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);

    /* Only required data - frame size and uncompressed marker. */
    hr = IMFMediaType_SetUINT64(video_type, &MF_MT_FRAME_SIZE, (UINT64)640 << 32 | 480);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(video_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    memset(&aperture, 0, sizeof(aperture));
    aperture.Area.cx = 100; aperture.Area.cy = 200;
    hr = IMFMediaType_SetBlob(video_type, &MF_MT_GEOMETRIC_APERTURE, (UINT8 *)&aperture, sizeof(aperture));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(video_type, &MF_MT_FIXED_SIZE_SAMPLES, 2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_SetInputType(transform, 0, video_type, MFT_SET_TYPE_TEST_ONLY);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &media_type);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_SetInputType(transform, 0, video_type, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_type == video_type, "Unexpected media type instance.\n");

    hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_type == media_type2, "Unexpected media type instance.\n");
    IMFMediaType_Release(media_type);
    IMFMediaType_Release(media_type2);

    /* Modified after type was set. */
    hr = IMFMediaType_SetUINT64(video_type, &MF_MT_PIXEL_ASPECT_RATIO, (UINT64)56 << 32 | 55);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Check attributes on available output types. */
    index = 0;
    while (SUCCEEDED(IMFTransform_GetOutputAvailableType(transform, 0, index++, &media_type)))
    {
        UINT64 frame_size, ratio;
        MFVideoArea aperture;
        GUID subtype, major;
        UINT32 value;

        hr = IMFMediaType_GetGUID(media_type, &MF_MT_MAJOR_TYPE, &major);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(IsEqualGUID(&major, &MFMediaType_Video), "Unexpected major type.\n");
        hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &subtype);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &frame_size);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(frame_size == ((UINT64)100 << 32 | 200), "Unexpected frame size %s.\n", wine_dbgstr_longlong(frame_size));
        hr = IMFMediaType_GetUINT32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, &value);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IMFMediaType_GetUINT32(media_type, &MF_MT_INTERLACE_MODE, &value);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(value == MFVideoInterlace_Progressive, "Unexpected interlace mode.\n");
        /* Ratio from input type */
        hr = IMFMediaType_GetUINT64(media_type, &MF_MT_PIXEL_ASPECT_RATIO, &ratio);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(ratio == ((UINT64)1 << 32 | 1), "Unexpected PAR %s.\n", wine_dbgstr_longlong(ratio));
        hr = IMFMediaType_GetBlob(media_type, &MF_MT_GEOMETRIC_APERTURE, (UINT8 *)&aperture, sizeof(aperture), NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(aperture.Area.cx == 100 && aperture.Area.cy == 200, "Unexpected aperture area.\n");
        hr = IMFMediaType_GetBlob(media_type, &MF_MT_MINIMUM_DISPLAY_APERTURE, (UINT8 *)&aperture, sizeof(aperture), NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(aperture.Area.cx == 100 && aperture.Area.cy == 200, "Unexpected aperture area.\n");
        hr = IMFMediaType_GetUINT32(video_type, &MF_MT_FIXED_SIZE_SAMPLES, &value);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(value == 2, "Unexpected value %u.\n", value);

        IMFMediaType_Release(media_type);
    }
    ok(index > 1, "Unexpected number of available types.\n");

    hr = IMFMediaType_DeleteItem(video_type, &MF_MT_FIXED_SIZE_SAMPLES);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_SetInputType(transform, 0, video_type, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    index = 0;
    while (SUCCEEDED(IMFTransform_GetOutputAvailableType(transform, 0, index++, &media_type)))
    {
        UINT32 value;
        UINT64 ratio;

        hr = IMFMediaType_GetUINT64(media_type, &MF_MT_PIXEL_ASPECT_RATIO, &ratio);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(ratio == ((UINT64)56 << 32 | 55), "Unexpected PAR %s.\n", wine_dbgstr_longlong(ratio));

        hr = IMFMediaType_GetUINT32(media_type, &MF_MT_FIXED_SIZE_SAMPLES, &value);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(value == 1, "Unexpected value %u.\n", value);

        IMFMediaType_Release(media_type);
    }
    ok(index > 1, "Unexpected number of available types.\n");

    /* Cloned type is returned. */
    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_type != media_type2, "Unexpected media type instance.\n");
    IMFMediaType_Release(media_type);
    IMFMediaType_Release(media_type2);

    /* Minimal valid attribute set for output type. */
    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMediaType(&media_type2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &subtype);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type2, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type2, &MF_MT_SUBTYPE, &subtype);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_SetOutputType(transform, 1, NULL, MFT_SET_TYPE_TEST_ONLY);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_SetOutputType(transform, 0, NULL, MFT_SET_TYPE_TEST_ONLY);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_SetOutputType(transform, 0, media_type2, MFT_SET_TYPE_TEST_ONLY);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetUINT32(media_type2, &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_SetOutputType(transform, 0, media_type2, MFT_SET_TYPE_TEST_ONLY);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Candidate type have frame size set, mismatching size is accepted. */
    hr = IMFMediaType_SetUINT64(media_type2, &MF_MT_FRAME_SIZE, (UINT64)64 << 32 | 64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_SetOutputType(transform, 0, media_type2, MFT_SET_TYPE_TEST_ONLY);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFMediaType_Release(media_type2);
    IMFMediaType_Release(media_type);

    hr = IMFVideoProcessor_GetVideoProcessorMode(processor, &guid);
    todo_wine
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoProcessor_GetVideoProcessorCaps(processor, (GUID *)&DXVA2_VideoProcSoftwareDevice, &caps);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_type == video_type, "Unexpected pointer.\n");
    hr = IMFMediaType_QueryInterface(media_type, &IID_IMFVideoMediaType, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IUnknown_Release(unk);
    IMFMediaType_Release(media_type);

    hr = IMFVideoProcessor_GetAvailableVideoProcessorModes(processor, &count, &guids);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_SetOutputType(transform, 1, media_type, 0);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoProcessor_GetVideoProcessorMode(processor, &guid);
    todo_wine
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoProcessor_GetAvailableVideoProcessorModes(processor, &count, &guids);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count > 0 && !!guids, "Unexpected modes data.\n");
    CoTaskMemFree(guids);

    hr = IMFVideoProcessor_GetVideoProcessorCaps(processor, (GUID *)&DXVA2_VideoProcSoftwareDevice, &caps);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputCurrentType(transform, 0, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_type == media_type2, "Unexpected media type instance.\n");

    IMFMediaType_Release(media_type);

    /* Clear input types */
    hr = IMFTransform_SetInputType(transform, 0, NULL, MFT_SET_TYPE_TEST_ONLY);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_SetInputType(transform, 0, NULL, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);
    hr = IMFTransform_GetOutputCurrentType(transform, 0, &media_type);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    /* Restore types */
    hr = IMFTransform_SetOutputType(transform, 0, media_type2, 0);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);
    hr = IMFTransform_GetOutputCurrentType(transform, 0, &media_type);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_SetInputType(transform, 0, video_type, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_type == video_type, "Unexpected media type instance.\n");
    IMFMediaType_Release(media_type);

    hr = IMFTransform_SetOutputType(transform, 0, media_type2, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTransform_GetOutputCurrentType(transform, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(media_type2 == media_type, "Unexpected media type instance.\n");

    IMFMediaType_Release(media_type2);
    IMFMediaType_Release(media_type);

    /* Resetting type twice */
    hr = IMFTransform_SetInputType(transform, 0, NULL, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTransform_SetInputType(transform, 0, NULL, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFVideoProcessor_Release(processor);

    IMFMediaType_Release(video_type);

    IDirect3DDeviceManager9_Release(manager);

    IDirect3DDevice9_Release(device);

done:
    IMFTransform_Release(transform);
    DestroyWindow(window);
}

static void test_default_presenter(void)
{
    IMFVideoDisplayControl *display_control;
    IMFVideoPresenter *presenter;
    IMFRateSupport *rate_support;
    IDirect3DDeviceManager9 *dm;
    IMFVideoDeviceID *deviceid;
    IUnknown *unk, *unk2;
    HWND hwnd, hwnd2;
    DWORD flags;
    float rate;
    HRESULT hr;
    GUID iid;

    hr = MFCreateVideoPresenter(NULL, &IID_IMFVideoPresenter, &IID_IMFVideoPresenter, (void **)&presenter);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateVideoPresenter(NULL, &IID_IDirect3DDevice9, &IID_IMFVideoPresenter, (void **)&presenter);
    ok(hr == S_OK, "Failed to create default presenter, hr %#lx.\n", hr);

    check_interface(presenter, &IID_IQualProp, TRUE);
    check_interface(presenter, &IID_IMFVideoPositionMapper, TRUE);
    check_interface(presenter, &IID_IMFTopologyServiceLookupClient, TRUE);
    check_interface(presenter, &IID_IMFVideoDisplayControl, TRUE);
    check_interface(presenter, &IID_IMFRateSupport, TRUE);
    check_interface(presenter, &IID_IMFGetService, TRUE);
    check_interface(presenter, &IID_IMFClockStateSink, TRUE);
    check_interface(presenter, &IID_IMFVideoPresenter, TRUE);
    check_interface(presenter, &IID_IMFVideoDeviceID, TRUE);
    check_interface(presenter, &IID_IMFQualityAdvise, TRUE);
    check_interface(presenter, &IID_IDirect3DDeviceManager9, TRUE);
    check_interface(presenter, &IID_IMFQualityAdviseLimits, TRUE);
    check_service_interface(presenter, &MR_VIDEO_RENDER_SERVICE, &IID_IMFVideoPositionMapper, TRUE);
    check_service_interface(presenter, &MR_VIDEO_RENDER_SERVICE, &IID_IMFVideoDisplayControl, TRUE);
    check_service_interface(presenter, &MR_VIDEO_RENDER_SERVICE, &IID_IMFVideoPresenter, TRUE);
    check_service_interface(presenter, &MR_VIDEO_RENDER_SERVICE, &IID_IMFClockStateSink, TRUE);
    check_service_interface(presenter, &MR_VIDEO_RENDER_SERVICE, &IID_IMFTopologyServiceLookupClient, TRUE);
    check_service_interface(presenter, &MR_VIDEO_RENDER_SERVICE, &IID_IQualProp, TRUE);
    check_service_interface(presenter, &MR_VIDEO_RENDER_SERVICE, &IID_IMFRateSupport, TRUE);
    check_service_interface(presenter, &MR_VIDEO_RENDER_SERVICE, &IID_IMFGetService, TRUE);
    check_service_interface(presenter, &MR_VIDEO_RENDER_SERVICE, &IID_IMFVideoDeviceID, TRUE);
    check_service_interface(presenter, &MR_VIDEO_RENDER_SERVICE, &IID_IMFQualityAdvise, TRUE);
    check_service_interface(presenter, &MR_VIDEO_RENDER_SERVICE, &IID_IMFQualityAdviseLimits, TRUE);
    check_service_interface(presenter, &MR_VIDEO_RENDER_SERVICE, &IID_IMFTransform, FALSE);
    check_service_interface(presenter, &MR_VIDEO_RENDER_SERVICE, &IID_IDirect3DDeviceManager9, TRUE);
    check_service_interface(presenter, &MR_VIDEO_ACCELERATION_SERVICE, &IID_IDirect3DDeviceManager9, TRUE);

    /* Query arbitrary supported interface back from device manager wrapper. */
    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IDirect3DDeviceManager9, (void **)&dm);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDirect3DDeviceManager9_QueryInterface(dm, &IID_IQualProp, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IUnknown_Release(unk);
    hr = IDirect3DDeviceManager9_QueryInterface(dm, &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(unk == unk2, "Unexpected interface.\n");
    IUnknown_Release(unk2);
    IUnknown_Release(unk);
    IDirect3DDeviceManager9_Release(dm);

    hr = MFGetService((IUnknown *)presenter, &MR_VIDEO_MIXER_SERVICE, &IID_IUnknown, (void **)&unk);
    ok(hr == MF_E_UNSUPPORTED_SERVICE, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFVideoDeviceID, (void **)&deviceid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoDeviceID_GetDeviceID(deviceid, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoDeviceID_GetDeviceID(deviceid, &iid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualIID(&iid, &IID_IDirect3DDevice9), "Unexpected id %s.\n", wine_dbgstr_guid(&iid));

    IMFVideoDeviceID_Release(deviceid);

    hr = MFGetService((IUnknown *)presenter, &MR_VIDEO_RENDER_SERVICE, &IID_IMFVideoDisplayControl, (void **)&display_control);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoDisplayControl_GetRenderingPrefs(display_control, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    flags = 123;
    hr = IMFVideoDisplayControl_GetRenderingPrefs(display_control, &flags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!flags, "Unexpected rendering flags %#lx.\n", flags);

    IMFVideoDisplayControl_Release(display_control);

    hr = MFGetService((IUnknown *)presenter, &MR_VIDEO_ACCELERATION_SERVICE, &IID_IDirect3DDeviceManager9, (void **)&dm);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IDirect3DDeviceManager9_Release(dm);

    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFVideoDisplayControl, (void **)&display_control);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Video window */
    hwnd = create_window();
    ok(!!hwnd, "Failed to create a test window.\n");

    hr = IMFVideoDisplayControl_GetVideoWindow(display_control, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hwnd2 = hwnd;
    hr = IMFVideoDisplayControl_GetVideoWindow(display_control, &hwnd2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(hwnd2 == NULL, "Unexpected window %p.\n", hwnd2);

    hr = IMFVideoDisplayControl_SetVideoWindow(display_control, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoDisplayControl_SetVideoWindow(display_control, (HWND)0x1);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoDisplayControl_SetVideoWindow(display_control, hwnd);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hwnd2 = NULL;
    hr = IMFVideoDisplayControl_GetVideoWindow(display_control, &hwnd2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(hwnd2 == hwnd, "Unexpected window %p.\n", hwnd2);
    IMFVideoDisplayControl_Release(display_control);

    /* Rate support. */
    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFRateSupport, (void **)&rate_support);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    rate = 1.0f;
    hr = IMFRateSupport_GetSlowestRate(rate_support, MFRATE_FORWARD, FALSE, &rate);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(rate == 0.0f, "Unexpected rate %f.\n", rate);

    rate = 1.0f;
    hr = IMFRateSupport_GetSlowestRate(rate_support, MFRATE_FORWARD, TRUE, &rate);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(rate == 0.0f, "Unexpected rate %f.\n", rate);

    rate = 1.0f;
    hr = IMFRateSupport_GetSlowestRate(rate_support, MFRATE_REVERSE, FALSE, &rate);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(rate == 0.0f, "Unexpected rate %f.\n", rate);

    rate = 1.0f;
    hr = IMFRateSupport_GetSlowestRate(rate_support, MFRATE_REVERSE, TRUE, &rate);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(rate == 0.0f, "Unexpected rate %f.\n", rate);

    IMFRateSupport_Release(rate_support);

    ok(!IMFVideoPresenter_Release(presenter), "Unexpected refcount.\n");

    DestroyWindow(hwnd);
}

static void test_MFCreateVideoMixerAndPresenter(void)
{
    IUnknown *mixer, *presenter;
    HRESULT hr;

    hr = MFCreateVideoMixerAndPresenter(NULL, NULL, &IID_IUnknown, (void **)&mixer, &IID_IUnknown, (void **)&presenter);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IUnknown_Release(mixer);
    IUnknown_Release(presenter);
}

static HRESULT WINAPI test_notify_callback_QueryInterface(IMFVideoSampleAllocatorNotify *iface,
        REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFVideoSampleAllocatorNotify) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFVideoSampleAllocatorNotify_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_notify_callback_AddRef(IMFVideoSampleAllocatorNotify *iface)
{
    return 2;
}

static ULONG WINAPI test_notify_callback_Release(IMFVideoSampleAllocatorNotify *iface)
{
    return 1;
}

static HRESULT WINAPI test_notify_callback_NotifyRelease(IMFVideoSampleAllocatorNotify *iface)
{
    return E_NOTIMPL;
}

static const IMFVideoSampleAllocatorNotifyVtbl test_notify_callback_vtbl =
{
    test_notify_callback_QueryInterface,
    test_notify_callback_AddRef,
    test_notify_callback_Release,
    test_notify_callback_NotifyRelease,
};

static void test_MFCreateVideoSampleAllocator(void)
{
    IMFVideoSampleAllocatorNotify test_notify = { &test_notify_callback_vtbl };
    IMFVideoSampleAllocatorCallback *allocator_cb;
    IMFMediaType *media_type, *video_type;
    IMFVideoSampleAllocator *allocator;
    IDirect3DDeviceManager9 *manager;
    IMFSample *sample, *sample2;
    IDirect3DSurface9 *surface;
    IDirect3DDevice9 *device;
    IMFMediaBuffer *buffer;
    LONG refcount, count;
    unsigned int token;
    IMFGetService *gs;
    IUnknown *unk;
    HWND window;
    HRESULT hr;
    BYTE *data;

    hr = MFCreateVideoSampleAllocator(&IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IUnknown_Release(unk);

    hr = MFCreateVideoSampleAllocator(&IID_IMFVideoSampleAllocator, (void **)&allocator);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocator_QueryInterface(allocator, &IID_IMFVideoSampleAllocatorCallback, (void **)&allocator_cb);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocatorCallback_SetCallback(allocator_cb, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocatorCallback_SetCallback(allocator_cb, &test_notify);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocatorCallback_SetCallback(allocator_cb, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocatorCallback_GetFreeSampleCount(allocator_cb, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = 10;
    hr = IMFVideoSampleAllocatorCallback_GetFreeSampleCount(allocator_cb, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!count, "Unexpected count %ld.\n", count);

    hr = IMFVideoSampleAllocator_UninitializeSampleAllocator(allocator);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocator_AllocateSample(allocator, &sample);
    ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocator_SetDirectXManager(allocator, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* It expects IMFVideoMediaType aka video major type. Exact return code is E_NOINTERFACE,
       likely coming from querying for IMFVideoMediaType. Does not seem valuable to match it. */
    hr = IMFVideoSampleAllocator_InitializeSampleAllocator(allocator, 2, media_type);
    ok(FAILED(hr), "Unexpected hr %#lx.\n", hr);

    video_type = create_video_type(&MFVideoFormat_RGB32);

    hr = IMFVideoSampleAllocator_InitializeSampleAllocator(allocator, 2, video_type);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);

    /* Frame size is required. */
    hr = IMFMediaType_SetUINT64(video_type, &MF_MT_FRAME_SIZE, (UINT64) 320 << 32 | 240);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFVideoSampleAllocator_InitializeSampleAllocator(allocator, 0, video_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocatorCallback_GetFreeSampleCount(allocator_cb, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %ld.\n", count);

    sample = NULL;
    hr = IMFVideoSampleAllocator_AllocateSample(allocator, &sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    refcount = get_refcount(sample);

    hr = IMFVideoSampleAllocator_AllocateSample(allocator, &sample2);
    ok(hr == MF_E_SAMPLEALLOCATOR_EMPTY, "Unexpected hr %#lx.\n", hr);

    /* Reinitialize with active sample. */
    hr = IMFVideoSampleAllocator_InitializeSampleAllocator(allocator, 4, video_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(refcount == get_refcount(sample), "Unexpected refcount %lu.\n", get_refcount(sample));

    hr = IMFVideoSampleAllocatorCallback_GetFreeSampleCount(allocator_cb, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 4, "Unexpected count %ld.\n", count);

    check_interface(sample, &IID_IMFDesiredSample, TRUE);
    check_interface(sample, &IID_IMFTrackedSample, TRUE);

    hr = IMFSample_GetBufferByIndex(sample, 0, &buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(buffer, &IID_IMF2DBuffer, TRUE);

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMFGetService, (void **)&gs);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* Win7 */, "Unexpected hr %#lx.\n", hr);

    /* Device manager wasn't set, sample gets regular memory buffers. */
    if (SUCCEEDED(hr))
    {
        hr = IMFGetService_GetService(gs, &MR_BUFFER_SERVICE, &IID_IDirect3DSurface9, (void **)&surface);
        ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
        IMFGetService_Release(gs);
    }

    IMFMediaBuffer_Release(buffer);

    IMFSample_Release(sample);

    IMFVideoSampleAllocatorCallback_Release(allocator_cb);

    IMFVideoSampleAllocator_Release(allocator);

    hr = MFCreateVideoSampleAllocator(&IID_IMFVideoSampleAllocatorCallback, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IUnknown_Release(unk);

    /* Using device manager */
    window = create_window();
    if (!(device = create_device(window)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IMFMediaType_Release(video_type);
        IMFMediaType_Release(media_type);
        DestroyWindow(window);
        return;
    }

    hr = DXVA2CreateDirect3DDeviceManager9(&token, &manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDirect3DDeviceManager9_ResetDevice(manager, device, token);
    ok(hr == S_OK, "Failed to set a device, hr %#lx.\n", hr);

    hr = MFCreateVideoSampleAllocator(&IID_IMFVideoSampleAllocator, (void **)&allocator);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocator_SetDirectXManager(allocator, (IUnknown *)manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetUINT64(video_type, &MF_MT_FRAME_SIZE, (UINT64) 320 << 32 | 240);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFVideoSampleAllocator_InitializeSampleAllocator(allocator, 0, video_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocator_AllocateSample(allocator, &sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(sample, &IID_IMFTrackedSample, TRUE);
    check_interface(sample, &IID_IMFDesiredSample, TRUE);

    hr = IMFSample_GetBufferByIndex(sample, 0, &buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_service_interface(buffer, &MR_BUFFER_SERVICE, &IID_IDirect3DSurface9, TRUE);
    check_interface(buffer, &IID_IMF2DBuffer, TRUE);
    check_interface(buffer, &IID_IMF2DBuffer2, TRUE);

    hr = IMFMediaBuffer_Lock(buffer, &data, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFMediaBuffer_Release(buffer);
    IMFSample_Release(sample);
    IMFVideoSampleAllocator_Release(allocator);
    IMFMediaType_Release(video_type);
    IMFMediaType_Release(media_type);
    IDirect3DDeviceManager9_Release(manager);
    IDirect3DDevice9_Release(device);
    DestroyWindow(window);
}

struct test_host
{
    IMFTopologyServiceLookup IMFTopologyServiceLookup_iface;
    IMediaEventSink IMediaEventSink_iface;
    IMFTransform *mixer;
    IMFVideoPresenter *presenter;
};

static struct test_host *impl_from_test_host(IMFTopologyServiceLookup *iface)
{
    return CONTAINING_RECORD(iface, struct test_host, IMFTopologyServiceLookup_iface);
}

static struct test_host *impl_from_test_host_events(IMediaEventSink *iface)
{
    return CONTAINING_RECORD(iface, struct test_host, IMediaEventSink_iface);
}

static HRESULT WINAPI test_host_QueryInterface(IMFTopologyServiceLookup *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFTopologyServiceLookup) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFTopologyServiceLookup_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_host_AddRef(IMFTopologyServiceLookup *iface)
{
    return 2;
}

static ULONG WINAPI test_host_Release(IMFTopologyServiceLookup *iface)
{
    return 1;
}

static HRESULT WINAPI test_host_LookupService(IMFTopologyServiceLookup *iface,
        MF_SERVICE_LOOKUP_TYPE lookup_type, DWORD index, REFGUID service,
        REFIID riid, void **objects, DWORD *num_objects)
{
    struct test_host *host = impl_from_test_host(iface);

    ok(*num_objects == 1, "Unexpected number of requested objects %lu\n", *num_objects);

    memset(objects, 0, *num_objects * sizeof(*objects));

    if (IsEqualGUID(service, &MR_VIDEO_RENDER_SERVICE))
    {
        if (IsEqualIID(riid, &IID_IMFClock)) return E_FAIL;
        if (IsEqualIID(riid, &IID_IMediaEventSink))
        {
            *objects = &host->IMediaEventSink_iface;
            IMediaEventSink_AddRef(&host->IMediaEventSink_iface);
            return S_OK;
        }

        ok(0, "Unexpected interface %s.\n", wine_dbgstr_guid(riid));
        return E_UNEXPECTED;
    }

    if (IsEqualGUID(service, &MR_VIDEO_MIXER_SERVICE))
    {
        if (IsEqualIID(riid, &IID_IMFTransform))
        {
            *objects = host->mixer;
            IMFTransform_AddRef(host->mixer);
            return S_OK;
        }
        ok(0, "Unexpected interface %s.\n", wine_dbgstr_guid(riid));
        return E_UNEXPECTED;
    }

    return E_NOTIMPL;
}

static const IMFTopologyServiceLookupVtbl test_host_vtbl =
{
    test_host_QueryInterface,
    test_host_AddRef,
    test_host_Release,
    test_host_LookupService,
};

static HRESULT WINAPI test_host_events_QueryInterface(IMediaEventSink *iface, REFIID riid, void **obj)
{
    struct test_host *host = impl_from_test_host_events(iface);
    return IMFTopologyServiceLookup_QueryInterface(&host->IMFTopologyServiceLookup_iface, riid, obj);
}

static ULONG WINAPI test_host_events_AddRef(IMediaEventSink *iface)
{
    struct test_host *host = impl_from_test_host_events(iface);
    return IMFTopologyServiceLookup_AddRef(&host->IMFTopologyServiceLookup_iface);
}

static ULONG WINAPI test_host_events_Release(IMediaEventSink *iface)
{
    struct test_host *host = impl_from_test_host_events(iface);
    return IMFTopologyServiceLookup_Release(&host->IMFTopologyServiceLookup_iface);
}

static HRESULT WINAPI test_host_events_Notify(IMediaEventSink *iface, LONG code, LONG_PTR param1, LONG_PTR param2)
{
    return S_OK;
}

static const IMediaEventSinkVtbl test_host_events_vtbl =
{
    test_host_events_QueryInterface,
    test_host_events_AddRef,
    test_host_events_Release,
    test_host_events_Notify,
};

static void init_test_host(struct test_host *host, IMFTransform *mixer, IMFVideoPresenter *presenter)
{
    host->IMFTopologyServiceLookup_iface.lpVtbl = &test_host_vtbl;
    host->IMediaEventSink_iface.lpVtbl = &test_host_events_vtbl;
    /* No need to keep references. */
    host->mixer = mixer;
    host->presenter = presenter;
}

static void test_presenter_video_position(void)
{
    IMFTopologyServiceLookupClient *lookup_client;
    IMFVideoDisplayControl *display_control;
    IMFAttributes *mixer_attributes;
    MFVideoNormalizedRect src_rect;
    IMFVideoPresenter *presenter;
    struct test_host host;
    IMFTransform *mixer;
    RECT dst_rect;
    UINT32 count;
    HRESULT hr;
    HWND hwnd;

    hwnd = create_window();
    ok(!!hwnd, "Failed to create a test window.\n");

    /* Setting position without the mixer. */
    hr = MFCreateVideoPresenter(NULL, &IID_IDirect3DDevice9, &IID_IMFVideoPresenter, (void **)&presenter);
    ok(hr == S_OK, "Failed to create default presenter, hr %#lx.\n", hr);
    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFVideoDisplayControl, (void **)&display_control);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoPresenter_ProcessMessage(presenter, MFVP_MESSAGE_INVALIDATEMEDIATYPE, 0);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);
    SetRect(&dst_rect, 0, 0, 10, 10);
    hr = IMFVideoDisplayControl_SetVideoPosition(display_control, NULL, &dst_rect);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    hr = IMFVideoDisplayControl_SetVideoWindow(display_control, hwnd);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoPresenter_ProcessMessage(presenter, MFVP_MESSAGE_INVALIDATEMEDIATYPE, 0);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);
    hr = IMFVideoDisplayControl_SetVideoPosition(display_control, NULL, &dst_rect);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFVideoDisplayControl_Release(display_control);
    IMFVideoPresenter_Release(presenter);

    /* With the mixer. */
    hr = MFCreateVideoMixer(NULL, &IID_IDirect3DDevice9, &IID_IMFTransform, (void **)&mixer);
    ok(hr == S_OK, "Failed to create a mixer, hr %#lx.\n", hr);

    hr = MFCreateVideoPresenter(NULL, &IID_IDirect3DDevice9, &IID_IMFVideoPresenter, (void **)&presenter);
    ok(hr == S_OK, "Failed to create default presenter, hr %#lx.\n", hr);

    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFVideoDisplayControl, (void **)&display_control);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFTopologyServiceLookupClient, (void **)&lookup_client);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    init_test_host(&host, mixer, presenter);

    /* Clear default mixer attributes, then attach presenter. */
    hr = IMFTransform_GetAttributes(mixer, &mixer_attributes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFAttributes_DeleteAllItems(mixer_attributes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopologyServiceLookupClient_InitServicePointers(lookup_client, &host.IMFTopologyServiceLookup_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFAttributes_GetCount(mixer_attributes, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);

    memset(&src_rect, 0, sizeof(src_rect));
    hr = IMFAttributes_GetBlob(mixer_attributes, &VIDEO_ZOOM_RECT, (UINT8 *)&src_rect, sizeof(src_rect), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(src_rect.left == 0.0f && src_rect.top == 0.0f && src_rect.right == 1.0f &&
            src_rect.bottom == 1.0f, "Unexpected source rectangle.\n");

    hr = IMFVideoDisplayControl_GetVideoPosition(display_control, NULL, &dst_rect);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoDisplayControl_GetVideoPosition(display_control, &src_rect, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    SetRect(&dst_rect, 1, 2, 3, 4);
    hr = IMFVideoDisplayControl_GetVideoPosition(display_control, &src_rect, &dst_rect);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(src_rect.left == 0.0f && src_rect.top == 0.0f && src_rect.right == 1.0f &&
            src_rect.bottom == 1.0f, "Unexpected source rectangle.\n");
    ok(dst_rect.left == 0 && dst_rect.right == 0 && dst_rect.top == 0 && dst_rect.bottom == 0,
            "Unexpected destination rectangle %s.\n", wine_dbgstr_rect(&dst_rect));

    hr = IMFVideoDisplayControl_SetVideoPosition(display_control, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    /* Setting position requires a window. */
    SetRect(&dst_rect, 0, 0, 10, 10);
    memset(&src_rect, 0, sizeof(src_rect));
    hr = IMFVideoDisplayControl_SetVideoPosition(display_control, &src_rect, &dst_rect);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoDisplayControl_SetVideoWindow(display_control, hwnd);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    SetRect(&dst_rect, 0, 0, 10, 10);
    hr = IMFVideoDisplayControl_SetVideoPosition(display_control, NULL, &dst_rect);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    SetRect(&dst_rect, 1, 2, 3, 4);
    hr = IMFVideoDisplayControl_GetVideoPosition(display_control, &src_rect, &dst_rect);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(dst_rect.left == 0 && dst_rect.right == 10 && dst_rect.top == 0 && dst_rect.bottom == 10,
            "Unexpected destination rectangle %s.\n", wine_dbgstr_rect(&dst_rect));

    set_rect(&src_rect, 0.0f, 0.0f, 2.0f, 1.0f);
    hr = IMFVideoDisplayControl_SetVideoPosition(display_control, &src_rect, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    set_rect(&src_rect, -0.1f, 0.0f, 0.9f, 1.0f);
    hr = IMFVideoDisplayControl_SetVideoPosition(display_control, &src_rect, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* Flipped source rectangle. */
    set_rect(&src_rect, 0.5f, 0.0f, 0.4f, 1.0f);
    hr = IMFVideoDisplayControl_SetVideoPosition(display_control, &src_rect, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    set_rect(&src_rect, 0.0f, 0.5f, 0.4f, 0.1f);
    hr = IMFVideoDisplayControl_SetVideoPosition(display_control, &src_rect, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    set_rect(&src_rect, 0.1f, 0.2f, 0.8f, 0.9f);
    hr = IMFVideoDisplayControl_SetVideoPosition(display_control, &src_rect, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Presenter updates mixer attribute. */
    memset(&src_rect, 0, sizeof(src_rect));
    hr = IMFAttributes_GetBlob(mixer_attributes, &VIDEO_ZOOM_RECT, (UINT8 *)&src_rect, sizeof(src_rect), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(src_rect.left == 0.1f && src_rect.top == 0.2f && src_rect.right == 0.8f &&
            src_rect.bottom == 0.9f, "Unexpected source rectangle.\n");

    hr = IMFVideoDisplayControl_GetVideoPosition(display_control, &src_rect, &dst_rect);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(src_rect.left == 0.1f && src_rect.top == 0.2f && src_rect.right == 0.8f &&
            src_rect.bottom == 0.9f, "Unexpected source rectangle.\n");

    SetRect(&dst_rect, 1, 2, 999, 1000);
    hr = IMFVideoDisplayControl_SetVideoPosition(display_control, NULL, &dst_rect);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    SetRect(&dst_rect, 0, 1, 3, 4);
    hr = IMFVideoDisplayControl_GetVideoPosition(display_control, &src_rect, &dst_rect);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(dst_rect.left == 1 && dst_rect.right == 999 && dst_rect.top == 2 && dst_rect.bottom == 1000,
            "Unexpected destination rectangle %s.\n", wine_dbgstr_rect(&dst_rect));

    /* Flipped destination rectangle. */
    SetRect(&dst_rect, 100, 1, 50, 1000);
    hr = IMFVideoDisplayControl_SetVideoPosition(display_control, NULL, &dst_rect);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    SetRect(&dst_rect, 1, 100, 100, 50);
    hr = IMFVideoDisplayControl_SetVideoPosition(display_control, NULL, &dst_rect);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    IMFVideoDisplayControl_Release(display_control);

    IMFTopologyServiceLookupClient_Release(lookup_client);
    IMFVideoPresenter_Release(presenter);
    IMFAttributes_Release(mixer_attributes);
    IMFTransform_Release(mixer);

    DestroyWindow(hwnd);
}

static void test_presenter_native_video_size(void)
{
    IMFTopologyServiceLookupClient *lookup_client;
    IMFVideoDisplayControl *display_control;
    IMFVideoPresenter *presenter;
    struct test_host host;
    IMFTransform *mixer;
    SIZE size, ratio;
    HRESULT hr;
    IMFMediaType *video_type;
    IDirect3DDeviceManager9 *dm;

    hr = MFCreateVideoMixer(NULL, &IID_IDirect3DDevice9, &IID_IMFTransform, (void **)&mixer);
    ok(hr == S_OK, "Failed to create a mixer, hr %#lx.\n", hr);

    hr = MFCreateVideoPresenter(NULL, &IID_IDirect3DDevice9, &IID_IMFVideoPresenter, (void **)&presenter);
    ok(hr == S_OK, "Failed to create default presenter, hr %#lx.\n", hr);

    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFTopologyServiceLookupClient, (void **)&lookup_client);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFTopologyServiceLookupClient_Release(lookup_client);

    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFVideoDisplayControl, (void **)&display_control);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoDisplayControl_GetNativeVideoSize(display_control, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    memset(&size, 0xcc, sizeof(size));
    hr = IMFVideoDisplayControl_GetNativeVideoSize(display_control, &size, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(size.cx == 0 && size.cy == 0, "Unexpected size.\n");

    memset(&ratio, 0xcc, sizeof(ratio));
    hr = IMFVideoDisplayControl_GetNativeVideoSize(display_control, NULL, &ratio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(ratio.cx == 0 && ratio.cy == 0, "Unexpected ratio.\n");

    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFTopologyServiceLookupClient, (void **)&lookup_client);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Configure mixer primary stream. */
    hr = MFGetService((IUnknown *)presenter, &MR_VIDEO_ACCELERATION_SERVICE, &IID_IDirect3DDeviceManager9, (void **)&dm);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_ProcessMessage(mixer, MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)dm);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IDirect3DDeviceManager9_Release(dm);

    video_type = create_video_type(&MFVideoFormat_RGB32);

    hr = IMFMediaType_SetUINT64(video_type, &MF_MT_FRAME_SIZE, (UINT64)640 << 32 | 480);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(video_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_SetInputType(mixer, 0, video_type, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Native video size is cached on initialization. */
    init_test_host(&host, mixer, presenter);

    hr = IMFTopologyServiceLookupClient_InitServicePointers(lookup_client, &host.IMFTopologyServiceLookup_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoDisplayControl_GetNativeVideoSize(display_control, &size, &ratio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(size.cx == 640 && size.cy == 480, "Unexpected size %lu x %lu.\n", size.cx, size.cy);
    ok((ratio.cx == 4 && ratio.cy == 3) || broken(!memcmp(&ratio, &size, sizeof(ratio))) /* < Win10 */,
            "Unexpected ratio %lu x %lu.\n", ratio.cx, ratio.cy);

    /* Update input type. */
    hr = IMFMediaType_SetUINT64(video_type, &MF_MT_FRAME_SIZE, (UINT64)320 << 32 | 240);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_SetInputType(mixer, 0, video_type, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoDisplayControl_GetNativeVideoSize(display_control, &size, &ratio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(size.cx == 640 && size.cy == 480, "Unexpected size %lu x %lu.\n", size.cx, size.cy);
    ok((ratio.cx == 4 && ratio.cy == 3) || broken(!memcmp(&ratio, &size, sizeof(ratio))) /* < Win10 */,
            "Unexpected ratio %lu x %lu.\n", ratio.cx, ratio.cy);

    /* Negotiating types updates native video size. */
    hr = IMFVideoPresenter_ProcessMessage(presenter, MFVP_MESSAGE_INVALIDATEMEDIATYPE, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoDisplayControl_GetNativeVideoSize(display_control, &size, &ratio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(size.cx == 320 && size.cy == 240, "Unexpected size %lu x %lu.\n", size.cx, size.cy);
    ok((ratio.cx == 4 && ratio.cy == 3) || broken(!memcmp(&ratio, &size, sizeof(ratio))) /* < Win10 */,
            "Unexpected ratio %lu x %lu.\n", ratio.cx, ratio.cy);

    IMFTopologyServiceLookupClient_Release(lookup_client);
    IMFMediaType_Release(video_type);
    IMFVideoDisplayControl_Release(display_control);
    IMFVideoPresenter_Release(presenter);
    IMFTransform_Release(mixer);
}

static void test_presenter_ar_mode(void)
{
    IMFVideoDisplayControl *display_control;
    HRESULT hr;
    DWORD mode;

    hr = MFCreateVideoPresenter(NULL, &IID_IDirect3DDevice9, &IID_IMFVideoDisplayControl, (void **)&display_control);
    ok(hr == S_OK, "Failed to create default presenter, hr %#lx.\n", hr);

    hr = IMFVideoDisplayControl_GetAspectRatioMode(display_control, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    mode = 0;
    hr = IMFVideoDisplayControl_GetAspectRatioMode(display_control, &mode);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(mode == (MFVideoARMode_PreservePicture | MFVideoARMode_PreservePixel), "Unexpected mode %#lx.\n", mode);

    hr = IMFVideoDisplayControl_SetAspectRatioMode(display_control, 0x100);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoDisplayControl_SetAspectRatioMode(display_control, MFVideoARMode_Mask);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    mode = 0;
    hr = IMFVideoDisplayControl_GetAspectRatioMode(display_control, &mode);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(mode == MFVideoARMode_Mask, "Unexpected mode %#lx.\n", mode);

    IMFVideoDisplayControl_Release(display_control);
}

static void test_presenter_video_window(void)
{
    D3DDEVICE_CREATION_PARAMETERS device_params = { 0 };
    IMFVideoDisplayControl *display_control;
    IDirect3DDeviceManager9 *dm;
    IDirect3DDevice9 *d3d_device;
    HANDLE hdevice;
    HRESULT hr;
    IDirect3DSwapChain9 *swapchain;
    D3DPRESENT_PARAMETERS present_params = { 0 };
    HWND window;

    hr = MFCreateVideoPresenter(NULL, &IID_IDirect3DDevice9, &IID_IMFVideoDisplayControl, (void **)&display_control);
    ok(hr == S_OK, "Failed to create default presenter, hr %#lx.\n", hr);

    hr = MFGetService((IUnknown *)display_control, &MR_VIDEO_ACCELERATION_SERVICE,
         &IID_IDirect3DDeviceManager9, (void **)&dm);

    hr = IDirect3DDeviceManager9_OpenDeviceHandle(dm, &hdevice);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_LockDevice(dm, hdevice, &d3d_device, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDevice9_GetCreationParameters(d3d_device, &device_params);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(device_params.hFocusWindow == GetDesktopWindow(), "Unexpected window %p.\n", device_params.hFocusWindow);
    ok(device_params.BehaviorFlags & D3DCREATE_MULTITHREADED, "Unexpected flags %#lx.\n", device_params.BehaviorFlags);

    hr = IDirect3DDevice9_GetSwapChain(d3d_device, 0, &swapchain);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DSwapChain9_GetPresentParameters(swapchain, &present_params);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(present_params.hDeviceWindow == GetDesktopWindow(), "Unexpected device window.\n");
    ok(present_params.Windowed, "Unexpected windowed mode.\n");
    ok(present_params.SwapEffect == D3DSWAPEFFECT_COPY, "Unexpected swap effect.\n");
    ok(present_params.Flags & D3DPRESENTFLAG_VIDEO, "Unexpected flags %#lx.\n", present_params.Flags);
    ok(present_params.PresentationInterval == D3DPRESENT_INTERVAL_IMMEDIATE, "Unexpected present interval.\n");

    IDirect3DSwapChain9_Release(swapchain);
    IDirect3DDevice9_Release(d3d_device);

    hr = IDirect3DDeviceManager9_UnlockDevice(dm, hdevice, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Setting window. */
    hr = IMFVideoDisplayControl_GetVideoWindow(display_control, &window);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!window, "Unexpected window %p.\n", window);

    window = create_window();

    hr = IMFVideoDisplayControl_SetVideoWindow(display_control, window);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Device is not recreated or reset on window change. */
    hr = IDirect3DDeviceManager9_LockDevice(dm, hdevice, &d3d_device, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDevice9_GetSwapChain(d3d_device, 0, &swapchain);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DSwapChain9_GetPresentParameters(swapchain, &present_params);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(present_params.hDeviceWindow == GetDesktopWindow(), "Unexpected device window.\n");
    ok(present_params.Windowed, "Unexpected windowed mode.\n");
    ok(present_params.SwapEffect == D3DSWAPEFFECT_COPY, "Unexpected swap effect.\n");
    ok(present_params.Flags & D3DPRESENTFLAG_VIDEO, "Unexpected flags %#lx.\n", present_params.Flags);
    ok(present_params.PresentationInterval == D3DPRESENT_INTERVAL_IMMEDIATE, "Unexpected present interval.\n");

    IDirect3DSwapChain9_Release(swapchain);
    IDirect3DDevice9_Release(d3d_device);

    hr = IDirect3DDeviceManager9_UnlockDevice(dm, hdevice, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(dm, hdevice);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IDirect3DDeviceManager9_Release(dm);
    ok(!IMFVideoDisplayControl_Release(display_control), "Unexpected refcount.\n");

    DestroyWindow(window);
}

static void test_presenter_quality_control(void)
{
    IMFQualityAdviseLimits *qa_limits;
    IMFVideoPresenter *presenter;
    MF_QUALITY_DROP_MODE mode;
    IMFQualityAdvise *advise;
    MF_QUALITY_LEVEL level;
    IQualProp *qual_prop;
    int frame_count;
    HRESULT hr;

    hr = MFCreateVideoPresenter(NULL, &IID_IDirect3DDevice9, &IID_IMFVideoPresenter, (void **)&presenter);
    ok(hr == S_OK, "Failed to create default presenter, hr %#lx.\n", hr);

    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFQualityAdviseLimits, (void **)&qa_limits);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFQualityAdvise, (void **)&advise);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFQualityAdviseLimits_GetMaximumDropMode(qa_limits, NULL);
    todo_wine
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFQualityAdviseLimits_GetMaximumDropMode(qa_limits, &mode);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
        ok(mode == MF_DROP_MODE_NONE, "Unexpected mode %d.\n", mode);

    hr = IMFQualityAdviseLimits_GetMinimumQualityLevel(qa_limits, NULL);
    todo_wine
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFQualityAdviseLimits_GetMinimumQualityLevel(qa_limits, &level);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
        ok(level == MF_QUALITY_NORMAL, "Unexpected level %d.\n", level);

    IMFQualityAdviseLimits_Release(qa_limits);

todo_wine {
    mode = 1;
    hr = IMFQualityAdvise_GetDropMode(advise, &mode);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(mode == MF_DROP_MODE_NONE, "Unexpected mode %d.\n", mode);

    level = 1;
    hr = IMFQualityAdvise_GetQualityLevel(advise, &level);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(level == MF_QUALITY_NORMAL, "Unexpected mode %d.\n", level);

    hr = IMFQualityAdvise_SetDropMode(advise, MF_DROP_MODE_1);
    ok(hr == MF_E_NO_MORE_DROP_MODES, "Unexpected hr %#lx.\n", hr);

    hr = IMFQualityAdvise_SetQualityLevel(advise, MF_QUALITY_NORMAL_MINUS_1);
    ok(hr == MF_E_NO_MORE_QUALITY_LEVELS, "Unexpected hr %#lx.\n", hr);
}

    IMFQualityAdvise_Release(advise);

    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IQualProp, (void **)&qual_prop);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IQualProp_get_FramesDrawn(qual_prop, NULL);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IQualProp_get_FramesDrawn(qual_prop, &frame_count);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    IQualProp_Release(qual_prop);

    IMFVideoPresenter_Release(presenter);
}

static void get_output_aperture(IMFTransform *mixer, SIZE *frame_size, MFVideoArea *aperture)
{
    IMFMediaType *media_type;
    UINT64 size;
    HRESULT hr;

    memset(frame_size, 0xcc, sizeof(*frame_size));
    memset(aperture, 0xcc, sizeof(*aperture));

    hr = IMFTransform_GetOutputCurrentType(mixer, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &size);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    frame_size->cx = size >> 32;
    frame_size->cy = size;

    hr = IMFMediaType_GetBlob(media_type, &MF_MT_GEOMETRIC_APERTURE, (UINT8 *)aperture, sizeof(*aperture), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFMediaType_Release(media_type);
}

static void test_presenter_media_type(void)
{
    IMFTopologyServiceLookupClient *lookup_client;
    IMFVideoPresenter *presenter;
    struct test_host host;
    IMFMediaType *input_type;
    IDirect3DDeviceManager9 *manager;
    HRESULT hr;
    IMFTransform *mixer;
    IDirect3DDevice9 *device;
    unsigned int token;
    SIZE frame_size;
    HWND window;
    MFVideoArea aperture;
    IMFVideoDisplayControl *display_control;
    RECT dst;

    window = create_window();
    if (!(device = create_device(window)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = DXVA2CreateDirect3DDeviceManager9(&token, &manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_ResetDevice(manager, device, token);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateVideoPresenter(NULL, &IID_IDirect3DDevice9, &IID_IMFVideoPresenter, (void **)&presenter);
    ok(hr == S_OK, "Failed to create default presenter, hr %#lx.\n", hr);

    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFVideoDisplayControl, (void **)&display_control);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateVideoMixer(NULL, &IID_IDirect3DDevice9, &IID_IMFTransform, (void **)&mixer);
    ok(hr == S_OK, "Failed to create a mixer, hr %#lx.\n", hr);

    input_type = create_video_type(&MFVideoFormat_RGB32);

    hr = IMFMediaType_SetUINT64(input_type, &MF_MT_FRAME_SIZE, (UINT64)100 << 32 | 50);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(input_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_ProcessMessage(mixer, MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFTopologyServiceLookupClient, (void **)&lookup_client);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    init_test_host(&host, mixer, presenter);

    hr = IMFTopologyServiceLookupClient_InitServicePointers(lookup_client, &host.IMFTopologyServiceLookup_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFTopologyServiceLookupClient_Release(lookup_client);

    hr = IMFVideoDisplayControl_SetVideoWindow(display_control, window);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Set destination rectangle before mixer types are configured. */
    SetRect(&dst, 0, 0, 101, 51);
    hr = IMFVideoDisplayControl_SetVideoPosition(display_control, NULL, &dst);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_SetInputType(mixer, 0, input_type, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(input_type);

    hr = IMFVideoPresenter_ProcessMessage(presenter, MFVP_MESSAGE_INVALIDATEMEDIATYPE, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    get_output_aperture(mixer, &frame_size, &aperture);
    ok(frame_size.cx == 101 && frame_size.cy == 51, "Unexpected frame size %lu x %lu.\n", frame_size.cx, frame_size.cy);
    ok(aperture.Area.cx == 101 && aperture.Area.cy == 51, "Unexpected size %lu x %lu.\n", aperture.Area.cx, aperture.Area.cy);
    ok(!aperture.OffsetX.value && !aperture.OffsetX.fract && !aperture.OffsetY.value && !aperture.OffsetY.fract,
            "Unexpected offset %u x %u.\n", aperture.OffsetX.value, aperture.OffsetY.value);

    SetRect(&dst, 1, 2, 200, 300);
    hr = IMFVideoDisplayControl_SetVideoPosition(display_control, NULL, &dst);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    get_output_aperture(mixer, &frame_size, &aperture);
    ok(frame_size.cx == 199 && frame_size.cy == 298, "Unexpected frame size %lu x %lu.\n", frame_size.cx, frame_size.cy);
    ok(aperture.Area.cx == 199 && aperture.Area.cy == 298, "Unexpected size %lu x %lu.\n", aperture.Area.cx, aperture.Area.cy);
    ok(!aperture.OffsetX.value && !aperture.OffsetX.fract && !aperture.OffsetY.value && !aperture.OffsetY.fract,
            "Unexpected offset %u x %u.\n", aperture.OffsetX.value, aperture.OffsetY.value);

    hr = IMFVideoDisplayControl_SetAspectRatioMode(display_control, MFVideoARMode_None);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    get_output_aperture(mixer, &frame_size, &aperture);
    ok(frame_size.cx == 199 && frame_size.cy == 298, "Unexpected frame size %lu x %lu.\n", frame_size.cx, frame_size.cy);
    ok(aperture.Area.cx == 199 && aperture.Area.cy == 298, "Unexpected size %lu x %lu.\n", aperture.Area.cx, aperture.Area.cy);
    ok(!aperture.OffsetX.value && !aperture.OffsetX.fract && !aperture.OffsetY.value && !aperture.OffsetY.fract,
            "Unexpected offset %u x %u.\n", aperture.OffsetX.value, aperture.OffsetY.value);

    IMFVideoDisplayControl_Release(display_control);
    IMFVideoPresenter_Release(presenter);
    IMFTransform_Release(mixer);
    IDirect3DDeviceManager9_Release(manager);
    IDirect3DDevice9_Release(device);

done:
    DestroyWindow(window);
}

static void test_presenter_shutdown(void)
{
    IMFTopologyServiceLookupClient *lookup_client;
    IMFVideoDisplayControl *display_control;
    IMFVideoMediaType *media_type;
    IMFVideoPresenter *presenter;
    IMFVideoDeviceID *deviceid;
    HWND window, window2;
    IQualProp *qual_prop;
    int frame_count;
    HRESULT hr;
    DWORD mode;
    RECT rect;
    SIZE size;
    IID iid;

    window = create_window();
    ok(!!window, "Failed to create test window.\n");

    hr = MFCreateVideoPresenter(NULL, &IID_IDirect3DDevice9, &IID_IMFVideoPresenter, (void **)&presenter);
    ok(hr == S_OK, "Failed to create default presenter, hr %#lx.\n", hr);

    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFTopologyServiceLookupClient, (void **)&lookup_client);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFVideoDeviceID, (void **)&deviceid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFVideoDisplayControl, (void **)&display_control);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IQualProp, (void **)&qual_prop);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopologyServiceLookupClient_ReleaseServicePointers(lookup_client);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoPresenter_ProcessMessage(presenter, MFVP_MESSAGE_INVALIDATEMEDIATYPE, 0);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoPresenter_ProcessMessage(presenter, MFVP_MESSAGE_BEGINSTREAMING, 0);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoPresenter_ProcessMessage(presenter, MFVP_MESSAGE_ENDSTREAMING, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoPresenter_ProcessMessage(presenter, MFVP_MESSAGE_PROCESSINPUTNOTIFY, 0);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoPresenter_GetCurrentMediaType(presenter, &media_type);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoDeviceID_GetDeviceID(deviceid, &iid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoDisplayControl_GetNativeVideoSize(display_control, &size, &size);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoDisplayControl_GetIdealVideoSize(display_control, &size, &size);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    SetRect(&rect, 0, 0, 10, 10);
    hr = IMFVideoDisplayControl_SetVideoPosition(display_control, NULL, &rect);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoDisplayControl_GetVideoPosition(display_control, NULL, &rect);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoDisplayControl_SetAspectRatioMode(display_control, MFVideoARMode_None);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoDisplayControl_GetAspectRatioMode(display_control, &mode);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoDisplayControl_SetVideoWindow(display_control, window);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoDisplayControl_SetVideoPosition(display_control, NULL, &rect);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoDisplayControl_GetVideoWindow(display_control, &window2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoDisplayControl_RepaintVideo(display_control);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IQualProp_get_FramesDrawn(qual_prop, NULL);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IQualProp_get_FramesDrawn(qual_prop, &frame_count);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopologyServiceLookupClient_ReleaseServicePointers(lookup_client);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IQualProp_Release(qual_prop);
    IMFVideoDeviceID_Release(deviceid);
    IMFVideoDisplayControl_Release(display_control);
    IMFTopologyServiceLookupClient_Release(lookup_client);

    IMFVideoPresenter_Release(presenter);

    DestroyWindow(window);
}

static void test_mixer_output_rectangle(void)
{
    IMFVideoMixerControl *mixer_control;
    MFVideoNormalizedRect rect;
    IMFTransform *mixer;
    HRESULT hr;

    hr = MFCreateVideoMixer(NULL, &IID_IDirect3DDevice9, &IID_IMFTransform, (void **)&mixer);
    ok(hr == S_OK, "Failed to create a mixer, hr %#lx.\n", hr);

    hr = IMFTransform_QueryInterface(mixer, &IID_IMFVideoMixerControl, (void **)&mixer_control);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoMixerControl_GetStreamOutputRect(mixer_control, 0, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = IMFVideoMixerControl_GetStreamOutputRect(mixer_control, 0, &rect);
    ok(hr == S_OK, "Failed to get output rect, hr %#lx.\n", hr);
    ok(rect.left == 0.0f && rect.top == 0.0f && rect.right == 1.0f && rect.bottom == 1.0f,
            "Unexpected rectangle.\n");

    hr = IMFVideoMixerControl_GetStreamOutputRect(mixer_control, 1, &rect);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoMixerControl_GetStreamOutputRect(mixer_control, 1, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoMixerControl_SetStreamOutputRect(mixer_control, 1, &rect);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoMixerControl_SetStreamOutputRect(mixer_control, 1, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    /* Wrong bounds. */
    set_rect(&rect, 0.0f, 0.0f, 1.1f, 1.0f);
    hr = IMFVideoMixerControl_SetStreamOutputRect(mixer_control, 0, &rect);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    set_rect(&rect, -0.1f, 0.0f, 0.5f, 1.0f);
    hr = IMFVideoMixerControl_SetStreamOutputRect(mixer_control, 0, &rect);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* Flipped. */
    set_rect(&rect, 1.0f, 0.0f, 0.0f, 1.0f);
    hr = IMFVideoMixerControl_SetStreamOutputRect(mixer_control, 0, &rect);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    set_rect(&rect, 0.0f, 1.0f, 1.0f, 0.5f);
    hr = IMFVideoMixerControl_SetStreamOutputRect(mixer_control, 0, &rect);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoMixerControl_SetStreamOutputRect(mixer_control, 0, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    IMFVideoMixerControl_Release(mixer_control);
    IMFTransform_Release(mixer);
}

static void test_mixer_zorder(void)
{
    IMFVideoMixerControl *mixer_control;
    IMFTransform *mixer;
    DWORD ids[2];
    DWORD value;
    HRESULT hr;

    hr = MFCreateVideoMixer(NULL, &IID_IDirect3DDevice9, &IID_IMFTransform, (void **)&mixer);
    ok(hr == S_OK, "Failed to create a mixer, hr %#lx.\n", hr);

    hr = IMFTransform_QueryInterface(mixer, &IID_IMFVideoMixerControl, (void **)&mixer_control);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoMixerControl_GetStreamZOrder(mixer_control, 0, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoMixerControl_GetStreamZOrder(mixer_control, 1, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    value = 1;
    hr = IMFVideoMixerControl_GetStreamZOrder(mixer_control, 0, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!value, "Unexpected value %lu.\n", value);

    value = 1;
    hr = IMFVideoMixerControl_GetStreamZOrder(mixer_control, 1, &value);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoMixerControl_SetStreamZOrder(mixer_control, 0, 1);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoMixerControl_SetStreamZOrder(mixer_control, 1, 1);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* Exceeds maximum stream number. */
    hr = IMFVideoMixerControl_SetStreamZOrder(mixer_control, 0, 20);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    value = 1;
    hr = IMFTransform_AddInputStreams(mixer, 1, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    value = 0;
    hr = IMFVideoMixerControl_GetStreamZOrder(mixer_control, 1, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value == 1, "Unexpected zorder %lu.\n", value);

    hr = IMFVideoMixerControl_SetStreamZOrder(mixer_control, 1, 0);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoMixerControl_SetStreamZOrder(mixer_control, 1, 2);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoMixerControl_SetStreamZOrder(mixer_control, 0, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    value = 2;
    hr = IMFTransform_AddInputStreams(mixer, 1, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    value = 0;
    hr = IMFVideoMixerControl_GetStreamZOrder(mixer_control, 2, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value == 2, "Unexpected zorder %lu.\n", value);

    hr = IMFVideoMixerControl_SetStreamZOrder(mixer_control, 2, 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    value = 3;
    hr = IMFTransform_AddInputStreams(mixer, 1, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    value = 0;
    hr = IMFVideoMixerControl_GetStreamZOrder(mixer_control, 3, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value == 3, "Unexpected zorder %lu.\n", value);

    hr = IMFTransform_DeleteInputStream(mixer, 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTransform_DeleteInputStream(mixer, 2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTransform_DeleteInputStream(mixer, 3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ids[0] = 2;
    ids[1] = 1;
    hr = IMFTransform_AddInputStreams(mixer, 2, ids);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    value = 0;
    hr = IMFVideoMixerControl_GetStreamZOrder(mixer_control, 1, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value == 2, "Unexpected zorder %lu.\n", value);

    value = 0;
    hr = IMFVideoMixerControl_GetStreamZOrder(mixer_control, 2, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value == 1, "Unexpected zorder %lu.\n", value);

    IMFVideoMixerControl_Release(mixer_control);
    IMFTransform_Release(mixer);
}

static IDirect3DSurface9 * create_surface(IDirect3DDeviceManager9 *manager, UINT fourcc,
        unsigned int width, unsigned int height)
{
    IDirectXVideoAccelerationService *service;
    IDirect3DSurface9 *surface = NULL;
    IDirect3DDevice9 *device;
    HANDLE handle;
    HRESULT hr;

    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_LockDevice(manager, handle, &device, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_GetVideoService(manager, handle, &IID_IDirectXVideoProcessorService,
            (void **)&service);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXVideoAccelerationService_CreateSurface(service, width, height, 0, fourcc,
            D3DPOOL_DEFAULT, 0, DXVA2_VideoProcessorRenderTarget, &surface, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IDirectXVideoAccelerationService_Release(service);

    hr = IDirect3DDevice9_ColorFill(device, surface, NULL, D3DCOLOR_ARGB(0x10, 0xff, 0x00, 0x00));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IDirect3DDevice9_Release(device);

    hr = IDirect3DDeviceManager9_UnlockDevice(manager, handle, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    return surface;
}

/* Format is assumed as 32bpp */
static DWORD get_surface_color(IDirect3DSurface9 *surface, unsigned int x, unsigned int y)
{
    D3DLOCKED_RECT locked_rect = { 0 };
    D3DSURFACE_DESC desc;
    DWORD *row, color;
    HRESULT hr;

    hr = IDirect3DSurface9_GetDesc(surface, &desc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(x < desc.Width && y < desc.Height, "Invalid coordinate.\n");
    if (x >= desc.Width || y >= desc.Height) return 0;

    hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    row = (DWORD *)((char *)locked_rect.pBits + y * locked_rect.Pitch);
    color = row[x];

    hr = IDirect3DSurface9_UnlockRect(surface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    return color;
}

static void test_mixer_samples(void)
{
    IDirect3DDeviceManager9 *manager;
    MFT_OUTPUT_DATA_BUFFER buffers[2];
    IMFVideoProcessor *processor;
    IDirect3DSurface9 *surface;
    IMFDesiredSample *desired;
    IDirect3DDevice9 *device;
    IMFMediaType *video_type;
    DWORD flags, color, status;
    IMFTransform *mixer;
    IMFSample *sample, *sample2;
    HWND window;
    UINT token;
    HRESULT hr;
    LONGLONG pts, duration;
    UINT32 count;

    window = create_window();
    if (!(device = create_device(window)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = MFCreateVideoMixer(NULL, &IID_IDirect3DDevice9, &IID_IMFTransform, (void **)&mixer);
    ok(hr == S_OK, "Failed to create a mixer, hr %#lx.\n", hr);

    hr = IMFTransform_QueryInterface(mixer, &IID_IMFVideoProcessor, (void **)&processor);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetInputStatus(mixer, 0, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetInputStatus(mixer, 1, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetInputStatus(mixer, 0, &status);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetInputStatus(mixer, 1, &status);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputStatus(mixer, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputStatus(mixer, &status);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    /* Configure device and media types. */
    hr = DXVA2CreateDirect3DDeviceManager9(&token, &manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_ResetDevice(manager, device, token);
    ok(hr == S_OK, "Failed to set a device, hr %#lx.\n", hr);

    hr = IMFTransform_ProcessMessage(mixer, MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    video_type = create_video_type(&MFVideoFormat_RGB32);

    hr = IMFMediaType_SetUINT64(video_type, &MF_MT_FRAME_SIZE, (UINT64)640 << 32 | 480);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(video_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_SetInputType(mixer, 0, video_type, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetInputStatus(mixer, 0, &status);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_SetOutputType(mixer, 0, video_type, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    status = 0;
    hr = IMFTransform_GetInputStatus(mixer, 0, &status);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(status == MFT_INPUT_STATUS_ACCEPT_DATA, "Unexpected status %#lx.\n", status);

    hr = IMFTransform_GetInputStatus(mixer, 1, &status);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    status = ~0u;
    hr = IMFTransform_GetOutputStatus(mixer, &status);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!status, "Unexpected status %#lx.\n", status);

    IMFMediaType_Release(video_type);

    memset(buffers, 0, sizeof(buffers));
    hr = IMFTransform_ProcessOutput(mixer, 0, 1, buffers, &status);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* It needs a sample with a backing surface. */
    hr = MFCreateSample(&sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    buffers[0].pSample = sample;
    hr = IMFTransform_ProcessOutput(mixer, 0, 1, buffers, &status);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    IMFSample_Release(sample);

    surface = create_surface(manager, D3DFMT_A8R8G8B8, 64, 64);

    hr = MFCreateVideoSampleFromSurface((IUnknown *)surface, &sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_QueryInterface(sample, &IID_IMFDesiredSample, (void **)&desired);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    buffers[0].pSample = sample;
    hr = IMFTransform_ProcessOutput(mixer, 0, 1, buffers, &status);
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "Unexpected hr %#lx.\n", hr);

    color = get_surface_color(surface, 0, 0);
    ok(color == D3DCOLOR_ARGB(0x10, 0xff, 0x00, 0x00), "Unexpected color %#lx.\n", color);

    /* Streaming is not started yet. Output is colored black, but only if desired timestamps were set. */
    IMFDesiredSample_SetDesiredSampleTimeAndDuration(desired, 100, 0);

    hr = IMFTransform_ProcessOutput(mixer, 0, 1, buffers, &status);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    color = get_surface_color(surface, 0, 0);
    ok(!color, "Unexpected color %#lx.\n", color);

    hr = IMFVideoProcessor_SetBackgroundColor(processor, RGB(0, 0, 255));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_ProcessOutput(mixer, 0, 1, buffers, &status);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    color = get_surface_color(surface, 0, 0);
    ok(!color, "Unexpected color %#lx.\n", color);

    hr = IMFTransform_ProcessOutput(mixer, 0, 2, buffers, &status);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    buffers[1].pSample = sample;
    hr = IMFTransform_ProcessOutput(mixer, 0, 2, buffers, &status);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    buffers[0].dwStreamID = 1;
    hr = IMFTransform_ProcessOutput(mixer, 0, 1, buffers, &status);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    IMFDesiredSample_Clear(desired);
    IMFDesiredSample_Release(desired);

    hr = IMFTransform_ProcessInput(mixer, 0, NULL, 0);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_ProcessInput(mixer, 5, NULL, 0);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    status = 0;
    hr = IMFTransform_GetInputStatus(mixer, 0, &status);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(status == MFT_INPUT_STATUS_ACCEPT_DATA, "Unexpected status %#lx.\n", status);

    status = ~0u;
    hr = IMFTransform_GetOutputStatus(mixer, &status);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!status, "Unexpected status %#lx.\n", status);

    hr = IMFTransform_ProcessInput(mixer, 0, sample, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    status = ~0u;
    hr = IMFTransform_GetInputStatus(mixer, 0, &status);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!status, "Unexpected status %#lx.\n", status);

    hr = IMFTransform_GetOutputStatus(mixer, &status);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(status == MFT_OUTPUT_STATUS_SAMPLE_READY, "Unexpected status %#lx.\n", status);

    hr = IMFTransform_ProcessInput(mixer, 0, sample, 0);
    ok(hr == MF_E_NOTACCEPTING, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_ProcessInput(mixer, 5, sample, 0);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#lx.\n", hr);

    /* ProcessOutput() sets sample time and duration. */
    hr = MFCreateVideoSampleFromSurface((IUnknown *)surface, &sample2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_SetUINT32(sample2, &IID_IMFSample, 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_SetSampleFlags(sample2, 0x123);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_GetSampleTime(sample2, &pts);
    ok(hr == MF_E_NO_SAMPLE_TIMESTAMP, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_GetSampleDuration(sample2, &duration);
    ok(hr == MF_E_NO_SAMPLE_DURATION, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_SetSampleTime(sample, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_SetSampleDuration(sample, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    memset(buffers, 0, sizeof(buffers));
    buffers[0].pSample = sample2;
    hr = IMFTransform_ProcessOutput(mixer, 0, 1, buffers, &status);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_GetSampleTime(sample2, &pts);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!pts, "Unexpected sample time.\n");

    hr = IMFSample_GetSampleDuration(sample2, &duration);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!duration, "Unexpected duration\n");

    /* Flags are not copied. */
    hr = IMFSample_GetSampleFlags(sample2, &flags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(flags == 0x123, "Unexpected flags %#lx.\n", flags);

    /* Attributes are not removed. */
    hr = IMFSample_GetCount(sample2, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected attribute count %u.\n", count);

    hr = IMFSample_GetCount(sample, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!count, "Unexpected attribute count %u.\n", count);

    IMFSample_Release(sample2);

    hr = IMFTransform_ProcessMessage(mixer, MFT_MESSAGE_COMMAND_DRAIN, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFSample_Release(sample);

    IDirect3DSurface9_Release(surface);

    IMFVideoProcessor_Release(processor);
    IMFTransform_Release(mixer);

    IDirect3DDeviceManager9_Release(manager);
    ok(!IDirect3DDevice9_Release(device), "Unexpected refcount.\n");

done:
    DestroyWindow(window);
}

static void create_d3d_sample(IDirect3DDeviceManager9 *manager, const GUID *subtype, IMFSample **sample)
{
    static const BITMAPINFOHEADER expect_header =
    {
        .biSize = sizeof(BITMAPINFOHEADER),
        .biWidth = 96, .biHeight = 96,
        .biPlanes = 1, .biBitCount = 32,
        .biCompression = BI_RGB,
        .biSizeImage = 96 * 96 * 4,
    };
    DWORD data_size, frame_data_len;
    D3DLOCKED_RECT d3d_rect = {0};
    IDirect3DSurface9 *surface;
    const BYTE *frame_data;
    LONG stride;
    HRESULT hr;

    if (IsEqualGUID(subtype, &MFVideoFormat_NV12))
    {
        load_resource(L"nv12frame.bmp", &frame_data, &frame_data_len);
        /* skip BMP header and RGB data from the dump */
        data_size = *(DWORD *)(frame_data + 2);
        frame_data_len = frame_data_len - data_size;
        frame_data = frame_data + data_size;
        ok(frame_data_len == 13824, "got length %lu\n", frame_data_len);
    }
    else
    {
        load_resource(L"rgb32frame.bmp", &frame_data, &frame_data_len);
        /* skip BMP header from the dump */
        data_size = *(DWORD *)(frame_data + 2 + 2 * sizeof(DWORD));
        frame_data_len -= data_size;
        frame_data += data_size;
        ok(frame_data_len == 36864, "got length %lu\n", frame_data_len);
    }

    surface = create_surface(manager, subtype->Data1, expect_header.biWidth, expect_header.biHeight);
    ok(!!surface, "Failed to create input surface.\n");
    hr = IDirect3DSurface9_LockRect(surface, &d3d_rect, NULL, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (IsEqualGUID(subtype, &MFVideoFormat_RGB32))
        memcpy(d3d_rect.pBits, frame_data, frame_data_len);
    else if (IsEqualGUID(subtype, &MFVideoFormat_NV12))
    {
        hr = MFGetStrideForBitmapInfoHeader(subtype->Data1, expect_header.biWidth, &stride);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = MFCopyImage(d3d_rect.pBits, d3d_rect.Pitch, frame_data, stride, expect_header.biWidth, expect_header.biHeight);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        frame_data += stride * expect_header.biHeight;
        d3d_rect.pBits = (BYTE *)d3d_rect.pBits + d3d_rect.Pitch * expect_header.biHeight;
        hr = MFCopyImage(d3d_rect.pBits, d3d_rect.Pitch, frame_data, stride, expect_header.biWidth, expect_header.biHeight / 2);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    }
    hr = IDirect3DSurface9_UnlockRect(surface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFCreateVideoSampleFromSurface((IUnknown *)surface, sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IDirect3DSurface9_Release(surface);
}

#define check_presenter_output(a, b, c, d) check_presenter_output_(__LINE__, a, b, c, d, FALSE)
static DWORD check_presenter_output_(int line, IMFVideoPresenter *presenter, const BITMAPINFOHEADER *expect_header,
        const WCHAR *resource, const RECT *rect, BOOL todo)
{
    BITMAPINFOHEADER header = {.biSize = sizeof(BITMAPINFOHEADER)};
    IMFVideoDisplayControl *display_control;
    DWORD diff, data_size;
    LONGLONG timestamp;
    BYTE *data;
    HRESULT hr;

    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFVideoDisplayControl, (void **)&display_control);
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFVideoDisplayControl_GetCurrentImage(display_control, &header, &data, &data_size, &timestamp);
    if (hr == MF_E_INVALIDREQUEST)
    {
        Sleep(500);
        hr = IMFVideoDisplayControl_GetCurrentImage(display_control, &header, &data, &data_size, &timestamp);
    }
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFVideoDisplayControl_Release(display_control);

    ok_(__FILE__, line)(header.biSize == expect_header->biSize, "Unexpected biSize %#lx\n", header.biSize);
    todo_wine_if(todo)
    ok_(__FILE__, line)(header.biWidth == expect_header->biWidth, "Unexpected biWidth %#lx\n", header.biWidth);
    todo_wine_if(todo)
    ok_(__FILE__, line)(header.biHeight == expect_header->biHeight, "Unexpected biHeight %#lx\n", header.biHeight);
    ok_(__FILE__, line)(header.biPlanes == expect_header->biPlanes, "Unexpected biPlanes %#x\n", header.biPlanes);
    ok_(__FILE__, line)(header.biBitCount == expect_header->biBitCount, "Unexpected biBitCount %#x\n", header.biBitCount);
    ok_(__FILE__, line)(header.biCompression == expect_header->biCompression, "Unexpected biCompression %#lx\n", header.biCompression);
    todo_wine_if(todo)
    ok_(__FILE__, line)(header.biSizeImage == expect_header->biSizeImage, "Unexpected biSizeImage %#lx\n", header.biSizeImage);
    ok_(__FILE__, line)(header.biXPelsPerMeter == expect_header->biXPelsPerMeter, "Unexpected biXPelsPerMeter %#lx\n", header.biXPelsPerMeter);
    ok_(__FILE__, line)(header.biYPelsPerMeter == expect_header->biYPelsPerMeter, "Unexpected biYPelsPerMeter %#lx\n", header.biYPelsPerMeter);
    ok_(__FILE__, line)(header.biClrUsed == expect_header->biClrUsed, "Unexpected biClrUsed %#lx\n", header.biClrUsed);
    ok_(__FILE__, line)(header.biClrImportant == expect_header->biClrImportant, "Unexpected biClrImportant %#lx\n", header.biClrImportant);

    diff = check_rgb32_data(resource, data, header.biSizeImage, rect);
    CoTaskMemFree(data);

    return diff;
}

static void test_presenter_orientation(const GUID *subtype)
{
    IMFTopologyServiceLookupClient *lookup_client;
    static const BITMAPINFOHEADER expect_header =
    {
        .biSize = sizeof(BITMAPINFOHEADER),
        .biWidth = 96, .biHeight = 96,
        .biPlanes = 1, .biBitCount = 32,
        .biCompression = BI_RGB,
        .biSizeImage = 96 * 96 * 4,
    };
    IDirect3DDeviceManager9 *manager;
    IMFVideoPresenter *presenter;
    IMFMediaType *video_type;
    struct test_host host;
    IMFTransform *mixer;
    IMFSample *sample;
    HWND window;
    HRESULT hr;
    DWORD diff;
    RECT rect;

    window = create_window();

    hr = MFCreateVideoMixer(NULL, &IID_IDirect3DDevice9, &IID_IMFTransform, (void **)&mixer);
    ok(hr == S_OK, "Failed to create a mixer, hr %#lx.\n", hr);
    hr = MFCreateVideoPresenter(NULL, &IID_IDirect3DDevice9, &IID_IMFVideoPresenter, (void **)&presenter);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    init_test_host(&host, mixer, presenter);
    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFTopologyServiceLookupClient, (void **)&lookup_client);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTopologyServiceLookupClient_InitServicePointers(lookup_client, &host.IMFTopologyServiceLookup_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFTopologyServiceLookupClient_Release(lookup_client);

    /* Configure device and media types. */

    hr = MFGetService((IUnknown *)presenter, &MR_VIDEO_ACCELERATION_SERVICE, &IID_IDirect3DDeviceManager9, (void **)&manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTransform_ProcessMessage(mixer, MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IDirect3DDeviceManager9_Release(manager);

    video_type = create_video_type(subtype);
    hr = IMFMediaType_SetUINT64(video_type, &MF_MT_FRAME_SIZE, (UINT64)expect_header.biWidth << 32 | expect_header.biHeight);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(video_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_SetInputType(mixer, 0, video_type, 0);
    if (broken(IsEqualGUID(subtype, &MFVideoFormat_NV12) && hr == E_FAIL))
    {
        win_skip("Skipping unsupported NV12 format\n");
        IMFMediaType_Release(video_type);
        goto skip_tests;
    }
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTransform_SetOutputType(mixer, 0, video_type, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(video_type);

    hr = IMFVideoPresenter_ProcessMessage(presenter, MFVP_MESSAGE_INVALIDATEMEDIATYPE, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFVideoPresenter_ProcessMessage(presenter, MFVP_MESSAGE_BEGINSTREAMING, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    create_d3d_sample(manager, subtype, &sample);
    hr = IMFTransform_ProcessInput(mixer, 0, sample, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFVideoPresenter_ProcessMessage(presenter, MFVP_MESSAGE_PROCESSINPUTNOTIFY, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFSample_Release(sample);

    SetRect(&rect, 0, 0, expect_header.biWidth, expect_header.biHeight);
    diff = check_presenter_output(presenter, &expect_header, L"rgb32frame-flip.bmp", &rect);
    ok(diff <= 5, "Unexpected %lu%% diff\n", diff);

    hr = IMFVideoPresenter_ProcessMessage(presenter, MFVP_MESSAGE_ENDSTREAMING, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

skip_tests:
    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFTopologyServiceLookupClient, (void **)&lookup_client);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTopologyServiceLookupClient_ReleaseServicePointers(lookup_client);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFTopologyServiceLookupClient_Release(lookup_client);

    IMFTransform_Release(mixer);
    IMFVideoPresenter_Release(presenter);

    DestroyWindow(window);
}

static void test_mixer_video_aperture(void)
{
    IMFTopologyServiceLookupClient *lookup_client;
    static const BITMAPINFOHEADER expect_header_crop =
    {
        .biSize = sizeof(BITMAPINFOHEADER),
        .biWidth = 34, .biHeight = 56,
        .biPlanes = 1, .biBitCount = 32,
        .biCompression = BI_RGB,
        .biSizeImage = 34 * 56 * 4,
    };
    static const BITMAPINFOHEADER expect_header =
    {
        .biSize = sizeof(BITMAPINFOHEADER),
        .biWidth = 96, .biHeight = 96,
        .biPlanes = 1, .biBitCount = 32,
        .biCompression = BI_RGB,
        .biSizeImage = 96 * 96 * 4,
    };
    const MFVideoArea aperture = {.Area = {.cx = 34, .cy = 56}};
    IDirect3DDeviceManager9 *manager;
    IMFVideoPresenter *presenter;
    IMFMediaType *video_type;
    struct test_host host;
    IMFTransform *mixer;
    IMFSample *sample;
    HWND window;
    HRESULT hr;
    DWORD diff;
    RECT rect;

    window = create_window();

    hr = MFCreateVideoMixer(NULL, &IID_IDirect3DDevice9, &IID_IMFTransform, (void **)&mixer);
    ok(hr == S_OK, "Failed to create a mixer, hr %#lx.\n", hr);
    hr = MFCreateVideoPresenter(NULL, &IID_IDirect3DDevice9, &IID_IMFVideoPresenter, (void **)&presenter);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    init_test_host(&host, mixer, presenter);
    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFTopologyServiceLookupClient, (void **)&lookup_client);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTopologyServiceLookupClient_InitServicePointers(lookup_client, &host.IMFTopologyServiceLookup_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFTopologyServiceLookupClient_Release(lookup_client);

    /* Configure device and media types. */

    hr = MFGetService((IUnknown *)presenter, &MR_VIDEO_ACCELERATION_SERVICE, &IID_IDirect3DDeviceManager9, (void **)&manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTransform_ProcessMessage(mixer, MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IDirect3DDeviceManager9_Release(manager);


    /* MF_MT_MINIMUM_DISPLAY_APERTURE / MF_MT_PAN_SCAN_APERTURE have no effect */

    video_type = create_video_type(&MFVideoFormat_RGB32);
    hr = IMFMediaType_SetUINT64(video_type, &MF_MT_FRAME_SIZE, (UINT64)expect_header.biWidth << 32 | expect_header.biHeight);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(video_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetBlob(video_type, &MF_MT_MINIMUM_DISPLAY_APERTURE, (BYTE *)&aperture, sizeof(aperture));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetBlob(video_type, &MF_MT_PAN_SCAN_APERTURE, (BYTE *)&aperture, sizeof(aperture));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTransform_SetInputType(mixer, 0, video_type, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(video_type);

    video_type = create_video_type(&MFVideoFormat_RGB32);
    hr = IMFMediaType_SetUINT64(video_type, &MF_MT_FRAME_SIZE, (UINT64)expect_header.biWidth << 32 | expect_header.biHeight);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(video_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTransform_SetOutputType(mixer, 0, video_type, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(video_type);

    hr = IMFVideoPresenter_ProcessMessage(presenter, MFVP_MESSAGE_INVALIDATEMEDIATYPE, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFVideoPresenter_ProcessMessage(presenter, MFVP_MESSAGE_BEGINSTREAMING, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    create_d3d_sample(manager, &MFVideoFormat_RGB32, &sample);
    hr = IMFTransform_ProcessInput(mixer, 0, sample, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFVideoPresenter_ProcessMessage(presenter, MFVP_MESSAGE_PROCESSINPUTNOTIFY, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFSample_Release(sample);

    SetRect(&rect, 0, 0, expect_header.biWidth, expect_header.biHeight);
    diff = check_presenter_output(presenter, &expect_header, L"rgb32frame-flip.bmp", &rect);
    ok(diff <= 5, "Unexpected %lu%% diff\n", diff);

    hr = IMFVideoPresenter_ProcessMessage(presenter, MFVP_MESSAGE_ENDSTREAMING, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);


    /* MF_MT_PAN_SCAN_APERTURE has an effect only when enabled */

    video_type = create_video_type(&MFVideoFormat_RGB32);
    hr = IMFMediaType_SetUINT64(video_type, &MF_MT_FRAME_SIZE, (UINT64)expect_header.biWidth << 32 | expect_header.biHeight);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(video_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTransform_SetOutputType(mixer, 0, video_type, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(video_type);

    video_type = create_video_type(&MFVideoFormat_RGB32);
    hr = IMFMediaType_SetUINT64(video_type, &MF_MT_FRAME_SIZE, (UINT64)expect_header.biWidth << 32 | expect_header.biHeight);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(video_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetBlob(video_type, &MF_MT_PAN_SCAN_APERTURE, (BYTE *)&aperture, sizeof(aperture));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(video_type, &MF_MT_PAN_SCAN_ENABLED, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTransform_SetInputType(mixer, 0, video_type, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(video_type);

    hr = IMFVideoPresenter_ProcessMessage(presenter, MFVP_MESSAGE_INVALIDATEMEDIATYPE, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFVideoPresenter_ProcessMessage(presenter, MFVP_MESSAGE_BEGINSTREAMING, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    create_d3d_sample(manager, &MFVideoFormat_RGB32, &sample);
    hr = IMFTransform_ProcessInput(mixer, 0, sample, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFVideoPresenter_ProcessMessage(presenter, MFVP_MESSAGE_PROCESSINPUTNOTIFY, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFSample_Release(sample);

    SetRect(&rect, 0, 0, expect_header_crop.biWidth, expect_header_crop.biHeight);
    diff = check_presenter_output_(__LINE__, presenter, &expect_header_crop, L"rgb32frame-crop.bmp", &rect, TRUE);
    todo_wine ok(diff <= 5, "Unexpected %lu%% diff\n", diff);

    hr = IMFVideoPresenter_ProcessMessage(presenter, MFVP_MESSAGE_ENDSTREAMING, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);


    /* MF_MT_GEOMETRIC_APERTURE has an effect */

    video_type = create_video_type(&MFVideoFormat_RGB32);
    hr = IMFMediaType_SetUINT64(video_type, &MF_MT_FRAME_SIZE, (UINT64)expect_header.biWidth << 32 | expect_header.biHeight);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(video_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetBlob(video_type, &MF_MT_GEOMETRIC_APERTURE, (BYTE *)&aperture, sizeof(aperture));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTransform_SetInputType(mixer, 0, video_type, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(video_type);

    video_type = create_video_type(&MFVideoFormat_RGB32);
    hr = IMFMediaType_SetUINT64(video_type, &MF_MT_FRAME_SIZE, (UINT64)expect_header.biWidth << 32 | expect_header.biHeight);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(video_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTransform_SetOutputType(mixer, 0, video_type, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(video_type);

    hr = IMFVideoPresenter_ProcessMessage(presenter, MFVP_MESSAGE_INVALIDATEMEDIATYPE, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFVideoPresenter_ProcessMessage(presenter, MFVP_MESSAGE_BEGINSTREAMING, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    create_d3d_sample(manager, &MFVideoFormat_RGB32, &sample);
    hr = IMFTransform_ProcessInput(mixer, 0, sample, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFVideoPresenter_ProcessMessage(presenter, MFVP_MESSAGE_PROCESSINPUTNOTIFY, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFSample_Release(sample);

    SetRect(&rect, 0, 0, expect_header_crop.biWidth, expect_header_crop.biHeight);
    diff = check_presenter_output(presenter, &expect_header_crop, L"rgb32frame-crop.bmp", &rect);
    ok(diff <= 5, "Unexpected %lu%% diff\n", diff);

    hr = IMFVideoPresenter_ProcessMessage(presenter, MFVP_MESSAGE_ENDSTREAMING, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);


    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFTopologyServiceLookupClient, (void **)&lookup_client);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTopologyServiceLookupClient_ReleaseServicePointers(lookup_client);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFTopologyServiceLookupClient_Release(lookup_client);

    IMFTransform_Release(mixer);
    IMFVideoPresenter_Release(presenter);

    DestroyWindow(window);
}

static void test_MFIsFormatYUV(void)
{
    static const DWORD formats[] =
    {
        D3DFMT_UYVY,
        D3DFMT_YUY2,
        MAKEFOURCC('A','Y','U','V'),
        MAKEFOURCC('I','M','C','1'),
        MAKEFOURCC('I','M','C','2'),
        MAKEFOURCC('Y','V','1','2'),
        MAKEFOURCC('N','V','1','1'),
        MAKEFOURCC('N','V','1','2'),
        MAKEFOURCC('Y','2','1','0'),
        MAKEFOURCC('Y','2','1','6'),
    };
    static const DWORD unsupported_formats[] =
    {
        D3DFMT_A8R8G8B8,
        MAKEFOURCC('I','Y','U','V'),
        MAKEFOURCC('I','4','2','0'),
        MAKEFOURCC('Y','V','Y','U'),
        MAKEFOURCC('Y','V','U','9'),
        MAKEFOURCC('Y','4','1','0'),
        MAKEFOURCC('Y','4','1','6'),
    };
    unsigned int i;
    BOOL ret;

    for (i = 0; i < ARRAY_SIZE(formats); ++i)
    {
        ret = MFIsFormatYUV(formats[i]);
        ok(ret, "Unexpected ret %d, format %s.\n", ret, debugstr_an((char *)&formats[i], 4));
    }

    for (i = 0; i < ARRAY_SIZE(unsupported_formats); ++i)
    {
        ret = MFIsFormatYUV(unsupported_formats[i]);
        ok(!ret, "Unexpected ret %d, format %s.\n", ret, debugstr_an((char *)&unsupported_formats[i], 4));
    }
}

static void test_mixer_render(void)
{
    IMFMediaType *video_type, *output_type;
    IMFVideoMixerControl *mixer_control;
    IDirect3DDeviceManager9 *manager;
    IMFVideoProcessor *processor;
    IDirect3DSurface9 *surface;
    IDirect3DDevice9 *device;
    IMFTransform *mixer;
    IMFSample *sample;
    DWORD status;
    HWND window;
    UINT token;
    HRESULT hr;

    window = create_window();
    if (!(device = create_device(window)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = MFCreateVideoMixer(NULL, &IID_IDirect3DDevice9, &IID_IMFTransform, (void **)&mixer);
    ok(hr == S_OK, "Failed to create a mixer, hr %#lx.\n", hr);

    hr = IMFTransform_QueryInterface(mixer, &IID_IMFVideoProcessor, (void **)&processor);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFVideoProcessor_Release(processor);

    hr = IMFTransform_QueryInterface(mixer, &IID_IMFVideoMixerControl, (void **)&mixer_control);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFVideoMixerControl_Release(mixer_control);

    /* Configure device and media types. */
    hr = DXVA2CreateDirect3DDeviceManager9(&token, &manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_ResetDevice(manager, device, token);
    ok(hr == S_OK, "Failed to set a device, hr %#lx.\n", hr);

    hr = IMFTransform_ProcessMessage(mixer, MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    video_type = create_video_type(&MFVideoFormat_RGB32);

    hr = IMFMediaType_SetUINT64(video_type, &MF_MT_FRAME_SIZE, (UINT64)64 << 32 | 64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(video_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_SetInputType(mixer, 0, video_type, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_GetOutputAvailableType(mixer, 0, 0, &output_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTransform_SetOutputType(mixer, 0, output_type, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFMediaType_Release(output_type);
    IMFMediaType_Release(video_type);

    surface = create_surface(manager, D3DFMT_A8R8G8B8, 64, 64);
    ok(!!surface, "Failed to create input surface.\n");

    hr = MFCreateVideoSampleFromSurface((IUnknown *)surface, &sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    EXPECT_REF(sample, 1);
    hr = IMFTransform_ProcessInput(mixer, 0, sample, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(sample, 2);

    hr = IMFTransform_GetOutputStatus(mixer, &status);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(status == MFT_OUTPUT_STATUS_SAMPLE_READY, "Unexpected status %#lx.\n", status);

    /* FLUSH/END_STREAMING releases input */
    hr = IMFTransform_ProcessMessage(mixer, MFT_MESSAGE_NOTIFY_END_STREAMING, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(sample, 1);

    hr = IMFTransform_GetOutputStatus(mixer, &status);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!status, "Unexpected status %#lx.\n", status);

    hr = IMFTransform_ProcessInput(mixer, 0, sample, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(sample, 2);

    hr = IMFTransform_GetOutputStatus(mixer, &status);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(status == MFT_OUTPUT_STATUS_SAMPLE_READY, "Unexpected status %#lx.\n", status);

    hr = IMFTransform_ProcessMessage(mixer, MFT_MESSAGE_COMMAND_FLUSH, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(sample, 1);

    hr = IMFTransform_GetOutputStatus(mixer, &status);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!status, "Unexpected status %#lx.\n", status);

    IMFSample_Release(sample);
    IDirect3DSurface9_Release(surface);
    IMFTransform_Release(mixer);

    IDirect3DDeviceManager9_Release(manager);
    ok(!IDirect3DDevice9_Release(device), "Unexpected refcount.\n");

done:
    DestroyWindow(window);
}

START_TEST(evr)
{
    IMFVideoPresenter *presenter;
    IDirect3D9 *d3d9;
    HRESULT hr;

    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d9)
    {
        skip("Failed to initialize D3D9. Skipping EVR tests.\n");
        return;
    }
    IDirect3D9_Release(d3d9);

    CoInitialize(NULL);

    if (FAILED(hr = MFCreateVideoPresenter(NULL, &IID_IDirect3DDevice9, &IID_IMFVideoPresenter, (void **)&presenter)))
    {
        win_skip("Failed to create default presenter, hr %#lx. Skipping tests.\n", hr);
        CoUninitialize();
        return;
    }
    IMFVideoPresenter_Release(presenter);

    test_aggregation();
    test_interfaces();
    test_enum_pins();
    test_find_pin();
    test_pin_info();
    test_unconnected_eos();
    test_misc_flags();
    test_display_control();
    test_service_lookup();
    test_query_accept();

    test_default_mixer();
    test_default_mixer_type_negotiation();
    test_surface_sample();
    test_default_presenter();
    test_MFCreateVideoMixerAndPresenter();
    test_MFCreateVideoSampleAllocator();
    test_presenter_video_position();
    test_presenter_native_video_size();
    test_presenter_ar_mode();
    test_presenter_video_window();
    test_presenter_quality_control();
    test_presenter_media_type();
    test_presenter_orientation(&MFVideoFormat_NV12);
    test_presenter_orientation(&MFVideoFormat_RGB32);
    test_mixer_video_aperture();
    test_presenter_shutdown();
    test_mixer_output_rectangle();
    test_mixer_zorder();
    test_mixer_samples();
    test_mixer_render();
    test_MFIsFormatYUV();

    CoUninitialize();
}
