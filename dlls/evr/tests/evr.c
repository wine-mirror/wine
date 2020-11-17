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

static IDirect3DDevice9 *create_device(IDirect3D9 *d3d9, HWND focus_window)
{
    D3DPRESENT_PARAMETERS present_parameters = {0};
    IDirect3DDevice9 *device = NULL;

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

    return device;
}

static IBaseFilter *create_evr(void)
{
    IBaseFilter *filter = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_EnhancedVideoRenderer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    return filter;
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
    ok(hr == E_NOINTERFACE, "Got hr %#x.\n", hr);
    ok(!filter, "Got interface %p.\n", filter);

    hr = CoCreateInstance(&CLSID_EnhancedVideoRenderer, &test_outer, CLSCTX_INPROC_SERVER,
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

    /* Default presenter. */
    presenter = (void *)0xdeadbeef;
    hr = CoCreateInstance(&CLSID_MFVideoPresenter9, &test_outer, CLSCTX_INPROC_SERVER, &IID_IMFVideoPresenter,
            (void **)&presenter);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#x.\n", hr);
    ok(!presenter, "Got interface %p.\n", presenter);

    hr = CoCreateInstance(&CLSID_MFVideoPresenter9, &test_outer, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK || broken(hr == E_FAIL) /* WinXP */, "Unexpected hr %#x.\n", hr);
    if (SUCCEEDED(hr))
    {
        ok(outer_ref == 1, "Got unexpected refcount %d.\n", outer_ref);
        ok(unk != &test_outer, "Returned IUnknown should not be outer IUnknown.\n");
        ref = get_refcount(unk);
        ok(ref == 1, "Got unexpected refcount %d.\n", ref);

        IUnknown_Release(unk);
    }

    /* Default mixer. */
    presenter = (void *)0xdeadbeef;
    hr = CoCreateInstance(&CLSID_MFVideoMixer9, &test_outer, CLSCTX_INPROC_SERVER, &IID_IMFTransform,
            (void **)&mixer);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#x.\n", hr);
    ok(!mixer, "Got interface %p.\n", mixer);

    hr = CoCreateInstance(&CLSID_MFVideoMixer9, &test_outer, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(outer_ref == 1, "Got unexpected refcount %d.\n", outer_ref);
    ok(unk != &test_outer, "Returned IUnknown should not be outer IUnknown.\n");
    ref = get_refcount(unk);
    ok(ref == 1, "Got unexpected refcount %d.\n", ref);

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
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#x, expected %#x.\n", hr, expected_hr);
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
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#x, expected %#x.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

static void test_interfaces(void)
{
    IBaseFilter *filter = create_evr();
    ULONG ref;

    todo_wine check_interface(filter, &IID_IAMFilterMiscFlags, TRUE);
    check_interface(filter, &IID_IBaseFilter, TRUE);
    check_interface(filter, &IID_IMediaFilter, TRUE);
    check_interface(filter, &IID_IMediaPosition, TRUE);
    check_interface(filter, &IID_IMediaSeeking, TRUE);
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

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got unexpected refcount %d.\n", ref);
}

static void test_enum_pins(void)
{
    IBaseFilter *filter = create_evr();
    IEnumPins *enum1, *enum2;
    ULONG count, ref;
    IPin *pins[2];
    HRESULT hr;

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
    IBaseFilter *filter = create_evr();
    IEnumPins *enum_pins;
    IPin *pin, *pin2;
    HRESULT hr;
    ULONG ref;

    hr = IBaseFilter_EnumPins(filter, &enum_pins);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IBaseFilter_FindPin(filter, sink_id, &pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IEnumPins_Next(enum_pins, 1, &pin2, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(pin2 == pin, "Expected pin %p, got %p.\n", pin, pin2);
    IPin_Release(pin2);
    IPin_Release(pin);

    IEnumPins_Release(enum_pins);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
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
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ref = get_refcount(filter);
    ok(ref == 2, "Got unexpected refcount %d.\n", ref);
    ref = get_refcount(pin);
    ok(ref == 2, "Got unexpected refcount %d.\n", ref);

    hr = IPin_QueryPinInfo(pin, &info);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(info.pFilter == filter, "Expected filter %p, got %p.\n", filter, info.pFilter);
    ok(info.dir == PINDIR_INPUT, "Got direction %d.\n", info.dir);
    ok(!lstrcmpW(info.achName, sink_id), "Got name %s.\n", wine_dbgstr_w(info.achName));
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
    ok(!lstrcmpW(id, sink_id), "Got id %s.\n", wine_dbgstr_w(id));
    CoTaskMemFree(id);

    hr = IPin_QueryInternalConnections(pin, NULL, NULL);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    IPin_Release(pin);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static IMFMediaType * create_video_type(const GUID *subtype)
{
    IMFMediaType *video_type;
    HRESULT hr;

    hr = MFCreateMediaType(&video_type);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(video_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(video_type, &MF_MT_SUBTYPE, subtype);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

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
    DWORD flags, value, count;
    COLORREF color;
    unsigned int i;
    DWORD ids[16];
    IUnknown *unk;
    GUID *guids;
    HRESULT hr;
    IID iid;

    hr = MFCreateVideoMixer(NULL, &IID_IDirect3DDevice9, &IID_IMFTransform, (void **)&transform);
    ok(hr == S_OK, "Failed to create default mixer, hr %#x.\n", hr);

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
    ok(hr == MF_E_UNSUPPORTED_SERVICE, "Unexpected hr %#x.\n", hr);

    if (SUCCEEDED(MFGetService((IUnknown *)transform, &MR_VIDEO_MIXER_SERVICE, &IID_IMFVideoMixerControl2, (void **)&mixer_control2)))
    {
        hr = IMFVideoMixerControl2_GetMixingPrefs(mixer_control2, NULL);
        ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

        hr = IMFVideoMixerControl2_GetMixingPrefs(mixer_control2, &flags);
        ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
        ok(!flags, "Unexpected flags %#x.\n", flags);

        IMFVideoMixerControl2_Release(mixer_control2);
    }

    hr = MFGetService((IUnknown *)transform, &MR_VIDEO_MIXER_SERVICE, &IID_IMFVideoProcessor, (void **)&processor);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoProcessor_GetBackgroundColor(processor, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    color = 1;
    hr = IMFVideoProcessor_GetBackgroundColor(processor, &color);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!color, "Unexpected color %#x.\n", color);

    hr = IMFVideoProcessor_SetBackgroundColor(processor, 0x00121212);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoProcessor_GetBackgroundColor(processor, &color);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(color == 0x121212, "Unexpected color %#x.\n", color);

    hr = IMFVideoProcessor_GetFilteringRange(processor, DXVA2_DetailFilterChromaLevel, &range);
todo_wine
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoProcessor_GetFilteringValue(processor, DXVA2_DetailFilterChromaLevel, &dxva_value);
todo_wine
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoProcessor_GetAvailableVideoProcessorModes(processor, &count, &guids);
todo_wine
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#x.\n", hr);

    IMFVideoProcessor_Release(processor);

    hr = IMFTransform_SetOutputBounds(transform, 100, 10);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_QueryInterface(transform, &IID_IMFVideoDeviceID, (void **)&deviceid);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetAttributes(transform, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetAttributes(transform, &attributes);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetAttributes(transform, &attributes2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(attributes == attributes2, "Unexpected attributes instance.\n");
    IMFAttributes_Release(attributes2);

    hr = IMFTransform_QueryInterface(transform, &IID_IMFAttributes, (void **)&attributes2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(attributes != attributes2, "Unexpected attributes instance.\n");

    hr = IMFAttributes_QueryInterface(attributes2, &IID_IMFTransform, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    IUnknown_Release(unk);

    hr = IMFAttributes_GetCount(attributes2, &count);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(count == 1, "Unexpected attribute count %u.\n", count);

    value = 0;
    hr = IMFAttributes_GetUINT32(attributes2, &MF_SA_D3D_AWARE, &value);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(value == 1, "Unexpected value %d.\n", value);

    IMFAttributes_Release(attributes2);

    hr = IMFAttributes_GetCount(attributes, &count);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(count == 1, "Unexpected attribute count %u.\n", count);

    memset(&rect, 0, sizeof(rect));
    hr = IMFAttributes_GetBlob(attributes, &VIDEO_ZOOM_RECT, (UINT8 *)&rect, sizeof(rect), NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(rect.left == 0.0f && rect.top == 0.0f && rect.right == 1.0f && rect.bottom == 1.0f,
            "Unexpected zoom rect (%f, %f) - (%f, %f).\n", rect.left, rect.top, rect.right, rect.bottom);

    IMFAttributes_Release(attributes);

    hr = IMFVideoDeviceID_GetDeviceID(deviceid, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoDeviceID_GetDeviceID(deviceid, &iid);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(IsEqualIID(&iid, &IID_IDirect3DDevice9), "Unexpected id %s.\n", wine_dbgstr_guid(&iid));

    IMFVideoDeviceID_Release(deviceid);

    /* Stream configuration. */
    input_count = output_count = 0;
    hr = IMFTransform_GetStreamCount(transform, &input_count, &output_count);
    ok(hr == S_OK, "Failed to get stream count, hr %#x.\n", hr);
    ok(input_count == 1 && output_count == 1, "Unexpected stream count %u/%u.\n", input_count, output_count);

    hr = IMFTransform_GetStreamLimits(transform, &input_min, &input_max, &output_min, &output_max);
    ok(hr == S_OK, "Failed to get stream limits, hr %#x.\n", hr);
    ok(input_min == 1 && input_max == 16 && output_min == 1 && output_max == 1, "Unexpected stream limits %u/%u, %u/%u.\n",
            input_min, input_max, output_min, output_max);

    hr = IMFTransform_GetInputStreamInfo(transform, 1, &input_info);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetOutputStreamInfo(transform, 1, &output_info);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    memset(&input_info, 0xcc, sizeof(input_info));
    hr = IMFTransform_GetInputStreamInfo(transform, 0, &input_info);
    ok(hr == S_OK, "Failed to get input info, hr %#x.\n", hr);

    memset(&output_info, 0xcc, sizeof(output_info));
    hr = IMFTransform_GetOutputStreamInfo(transform, 0, &output_info);
    ok(hr == S_OK, "Failed to get input info, hr %#x.\n", hr);
    ok(!(output_info.dwFlags & (MFT_OUTPUT_STREAM_PROVIDES_SAMPLES | MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES)),
            "Unexpected output flags %#x.\n", output_info.dwFlags);

    hr = IMFTransform_GetStreamIDs(transform, 1, &input_id, 1, &output_id);
    ok(hr == S_OK, "Failed to get input info, hr %#x.\n", hr);
    ok(input_id == 0 && output_id == 0, "Unexpected stream ids.\n");

    hr = IMFTransform_GetInputStreamAttributes(transform, 1, &attributes);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetOutputStreamAttributes(transform, 1, &attributes);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetOutputStreamAttributes(transform, 0, &attributes);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_AddInputStreams(transform, 16, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_AddInputStreams(transform, 16, ids);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    memset(ids, 0, sizeof(ids));
    hr = IMFTransform_AddInputStreams(transform, 15, ids);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(ids); ++i)
        ids[i] = i + 1;

    hr = IMFTransform_AddInputStreams(transform, 15, ids);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    input_count = output_count = 0;
    hr = IMFTransform_GetStreamCount(transform, &input_count, &output_count);
    ok(hr == S_OK, "Failed to get stream count, hr %#x.\n", hr);
    ok(input_count == 16 && output_count == 1, "Unexpected stream count %u/%u.\n", input_count, output_count);

    memset(&input_info, 0, sizeof(input_info));
    hr = IMFTransform_GetInputStreamInfo(transform, 1, &input_info);
    ok(hr == S_OK, "Failed to get input info, hr %#x.\n", hr);
    ok((input_info.dwFlags & (MFT_INPUT_STREAM_REMOVABLE | MFT_INPUT_STREAM_OPTIONAL)) ==
            (MFT_INPUT_STREAM_REMOVABLE | MFT_INPUT_STREAM_OPTIONAL), "Unexpected flags %#x.\n", input_info.dwFlags);

    attributes = NULL;
    hr = IMFTransform_GetInputStreamAttributes(transform, 0, &attributes);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    hr = IMFAttributes_GetCount(attributes, &count);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);
    hr = IMFAttributes_GetUINT32(attributes, &MF_SA_REQUIRED_SAMPLE_COUNT, &count);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);
    ok(!!attributes, "Unexpected attributes.\n");

    attributes2 = NULL;
    hr = IMFTransform_GetInputStreamAttributes(transform, 0, &attributes2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(attributes == attributes2, "Unexpected instance.\n");

    IMFAttributes_Release(attributes2);
    IMFAttributes_Release(attributes);

    attributes = NULL;
    hr = IMFTransform_GetInputStreamAttributes(transform, 1, &attributes);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!!attributes, "Unexpected attributes.\n");
    IMFAttributes_Release(attributes);

    hr = IMFTransform_DeleteInputStream(transform, 0);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_DeleteInputStream(transform, 1);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    input_count = output_count = 0;
    hr = IMFTransform_GetStreamCount(transform, &input_count, &output_count);
    ok(hr == S_OK, "Failed to get stream count, hr %#x.\n", hr);
    ok(input_count == 15 && output_count == 1, "Unexpected stream count %u/%u.\n", input_count, output_count);

    IMFTransform_Release(transform);

    hr = MFCreateVideoMixer(NULL, &IID_IMFTransform, &IID_IMFTransform, (void **)&transform);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = CoCreateInstance(&CLSID_MFVideoMixer9, NULL, CLSCTX_INPROC_SERVER, &IID_IMFTransform, (void **)&transform);
    ok(hr == S_OK, "Failed to create default mixer, hr %#x.\n", hr);
    IMFTransform_Release(transform);
}

static void test_surface_sample(void)
{
    IDirect3DSurface9 *backbuffer = NULL, *surface;
    IMFDesiredSample *desired_sample;
    IMFMediaBuffer *buffer, *buffer2;
    LONGLONG duration, time1, time2;
    IDirect3DSwapChain9 *swapchain;
    DWORD flags, count, length;
    IDirect3DDevice9 *device;
    IMFSample *sample;
    IDirect3D9 *d3d;
    IUnknown *unk;
    HWND window;
    HRESULT hr;
    BYTE *data;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetSwapChain(device, 0, &swapchain);
    ok(SUCCEEDED(hr), "Failed to get the implicit swapchain (%08x)\n", hr);

    hr = IDirect3DSwapChain9_GetBackBuffer(swapchain, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(SUCCEEDED(hr), "Failed to get the back buffer (%08x)\n", hr);
    ok(backbuffer != NULL, "The back buffer is NULL\n");

    IDirect3DSwapChain9_Release(swapchain);

    hr = MFCreateVideoSampleFromSurface(NULL, &sample);
    ok(hr == S_OK, "Failed to create surface sample, hr %#x.\n", hr);
    IMFSample_Release(sample);

    hr = MFCreateVideoSampleFromSurface((IUnknown *)backbuffer, &sample);
    ok(hr == S_OK, "Failed to create surface sample, hr %#x.\n", hr);

    hr = IMFSample_QueryInterface(sample, &IID_IMFTrackedSample, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    IUnknown_Release(unk);

    hr = IMFSample_QueryInterface(sample, &IID_IMFDesiredSample, (void **)&desired_sample);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_GetCount(sample, &count);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!count, "Unexpected attribute count %u.\n", count);

    hr = IMFDesiredSample_GetDesiredSampleTimeAndDuration(desired_sample, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFDesiredSample_GetDesiredSampleTimeAndDuration(desired_sample, NULL, &time2);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFDesiredSample_GetDesiredSampleTimeAndDuration(desired_sample, &time1, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFDesiredSample_GetDesiredSampleTimeAndDuration(desired_sample, &time1, &time2);
    ok(hr == MF_E_NOT_AVAILABLE, "Unexpected hr %#x.\n", hr);

    IMFDesiredSample_SetDesiredSampleTimeAndDuration(desired_sample, 123, 456);

    time1 = time2 = 0;
    hr = IMFDesiredSample_GetDesiredSampleTimeAndDuration(desired_sample, &time1, &time2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(time1 == 123 && time2 == 456, "Unexpected time values.\n");

    IMFDesiredSample_SetDesiredSampleTimeAndDuration(desired_sample, 0, 0);

    time1 = time2 = 1;
    hr = IMFDesiredSample_GetDesiredSampleTimeAndDuration(desired_sample, &time1, &time2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(time1 == 0 && time2 == 0, "Unexpected time values.\n");

    IMFDesiredSample_Clear(desired_sample);

    hr = IMFDesiredSample_GetDesiredSampleTimeAndDuration(desired_sample, &time1, &time2);
    ok(hr == MF_E_NOT_AVAILABLE, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_GetCount(sample, &count);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!count, "Unexpected attribute count %u.\n", count);

    /* Attributes are cleared. */
    hr = IMFSample_SetUnknown(sample, &MFSampleExtension_Token, NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_GetCount(sample, &count);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(count == 1, "Unexpected attribute count %u.\n", count);

    hr = IMFSample_SetSampleTime(sample, 0);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_GetSampleTime(sample, &time1);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_SetSampleDuration(sample, 0);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_GetSampleDuration(sample, &duration);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_SetSampleFlags(sample, 0x1);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    IMFDesiredSample_Clear(desired_sample);

    hr = IMFSample_GetCount(sample, &count);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!count, "Unexpected attribute count %u.\n", count);

    hr = IMFSample_GetSampleTime(sample, &time1);
    ok(hr == MF_E_NO_SAMPLE_TIMESTAMP, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_GetSampleDuration(sample, &duration);
    ok(hr == MF_E_NO_SAMPLE_DURATION, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_GetSampleFlags(sample, &flags);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(flags == 0, "Unexpected flags %#x.\n", flags);

    IMFDesiredSample_Release(desired_sample);

    hr = IMFSample_GetCount(sample, &count);
    ok(hr == S_OK, "Failed to get attribute count, hr %#x.\n", hr);
    ok(!count, "Unexpected attribute count.\n");

    count = 0;
    hr = IMFSample_GetBufferCount(sample, &count);
    ok(hr == S_OK, "Failed to get buffer count, hr %#x.\n", hr);
    ok(count == 1, "Unexpected attribute count.\n");

    hr = IMFSample_GetTotalLength(sample, &length);
    ok(hr == S_OK, "Failed to get length, hr %#x.\n", hr);
    ok(!length, "Unexpected length %u.\n", length);

    hr = IMFSample_GetSampleDuration(sample, &duration);
    ok(hr == MF_E_NO_SAMPLE_DURATION, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_GetSampleTime(sample, &duration);
    ok(hr == MF_E_NO_SAMPLE_TIMESTAMP, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_GetBufferByIndex(sample, 0, &buffer);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaBuffer_GetMaxLength(buffer, &length);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaBuffer_GetCurrentLength(buffer, &length);
    ok(hr == S_OK, "Failed to get length, hr %#x.\n", hr);
    ok(!length, "Unexpected length %u.\n", length);

    hr = IMFMediaBuffer_SetCurrentLength(buffer, 16);
    ok(hr == S_OK, "Failed to get length, hr %#x.\n", hr);

    hr = IMFMediaBuffer_GetCurrentLength(buffer, &length);
    ok(hr == S_OK, "Failed to get length, hr %#x.\n", hr);
    ok(length == 16, "Unexpected length %u.\n", length);

    hr = IMFMediaBuffer_Lock(buffer, &data, NULL, NULL);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer, (void **)&unk);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_AddBuffer(sample, buffer);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_GetBufferCount(sample, &count);
    ok(hr == S_OK, "Failed to get buffer count, hr %#x.\n", hr);
    ok(count == 2, "Unexpected attribute count.\n");

    hr = IMFSample_ConvertToContiguousBuffer(sample, &buffer2);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_CopyToBuffer(sample, buffer);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_RemoveAllBuffers(sample);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_GetBufferCount(sample, &count);
    ok(hr == S_OK, "Failed to get buffer count, hr %#x.\n", hr);
    ok(!count, "Unexpected attribute count.\n");

    hr = MFGetService((IUnknown *)buffer, &MR_BUFFER_SERVICE, &IID_IDirect3DSurface9, (void **)&surface);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(surface == backbuffer, "Unexpected instance.\n");
    IDirect3DSurface9_Release(surface);

    hr = MFGetService((IUnknown *)buffer, &MR_BUFFER_SERVICE, &IID_IUnknown, (void **)&surface);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(surface == backbuffer, "Unexpected instance.\n");
    IDirect3DSurface9_Release(surface);

    IMFMediaBuffer_Release(buffer);

    hr = IMFSample_GetSampleFlags(sample, &flags);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_SetSampleFlags(sample, 0x123);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    flags = 0;
    hr = IMFSample_GetSampleFlags(sample, &flags);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(flags == 0x123, "Unexpected flags %#x.\n", flags);

    IMFSample_Release(sample);

done:
    if (backbuffer)
        IDirect3DSurface9_Release(backbuffer);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_default_mixer_type_negotiation(void)
{
    IMFMediaType *media_type, *media_type2;
    IDirect3DDeviceManager9 *manager;
    DXVA2_VideoProcessorCaps caps;
    IMFVideoProcessor *processor;
    IDirect3DDevice9 *device;
    IMFMediaType *video_type;
    IMFTransform *transform;
    DWORD index, count;
    GUID guid, *guids;
    IDirect3D9 *d3d;
    IUnknown *unk;
    HWND window;
    HRESULT hr;
    UINT token;

    hr = MFCreateVideoMixer(NULL, &IID_IDirect3DDevice9, &IID_IMFTransform, (void **)&transform);
    ok(hr == S_OK, "Failed to create default mixer, hr %#x.\n", hr);

    hr = IMFTransform_GetInputAvailableType(transform, 0, 0, &media_type);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &media_type);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#x.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
    ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_SetInputType(transform, 0, media_type, MFT_SET_TYPE_TEST_ONLY);
    ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#x.\n", hr);

    /* Now try with device manager. */

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = DXVA2CreateDirect3DDeviceManager9(&token, &manager);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)manager);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    /* Now manager is not initialized. */
    hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
    ok(hr == DXVA2_E_NOT_INITIALIZED, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_SetInputType(transform, 0, media_type, MFT_SET_TYPE_TEST_ONLY);
    ok(hr == DXVA2_E_NOT_INITIALIZED, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_ResetDevice(manager, device, token);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    /* And now type description is incomplete. */
    hr = IMFTransform_SetInputType(transform, 0, media_type, 0);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#x.\n", hr);
    IMFMediaType_Release(media_type);

    video_type = create_video_type(&MFVideoFormat_RGB32);

    /* Partially initialized type. */
    hr = IMFTransform_SetInputType(transform, 0, video_type, 0);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#x.\n", hr);

    /* Only required data - frame size and uncompressed marker. */
    hr = IMFMediaType_SetUINT64(video_type, &MF_MT_FRAME_SIZE, (UINT64)640 << 32 | 480);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    hr = IMFMediaType_SetUINT32(video_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_SetInputType(transform, 0, video_type, MFT_SET_TYPE_TEST_ONLY);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_SetInputType(transform, 0, video_type, 0);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(media_type == video_type, "Unexpected media type instance.\n");

    hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(media_type == media_type2, "Unexpected media type instance.\n");
    IMFMediaType_Release(media_type);
    IMFMediaType_Release(media_type2);

    /* Check attributes on available output types. */
    index = 0;
    while (SUCCEEDED(IMFTransform_GetOutputAvailableType(transform, 0, index++, &media_type)))
    {
        UINT64 frame_size;
        GUID subtype;
        UINT32 value;

        hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &subtype);
        ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
        hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &frame_size);
        ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
        hr = IMFMediaType_GetUINT32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, &value);
        ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

        IMFMediaType_Release(media_type);
    }
    ok(index > 1, "Unexpected number of available types.\n");

    hr = IMFTransform_QueryInterface(transform, &IID_IMFVideoProcessor, (void **)&processor);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoProcessor_GetVideoProcessorMode(processor, &guid);
todo_wine
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoProcessor_GetVideoProcessorCaps(processor, (GUID *)&DXVA2_VideoProcSoftwareDevice, &caps);
todo_wine
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(media_type == video_type, "Unexpected pointer.\n");
    hr = IMFMediaType_QueryInterface(media_type, &IID_IMFVideoMediaType, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    IUnknown_Release(unk);
    IMFMediaType_Release(media_type);

    hr = IMFVideoProcessor_GetAvailableVideoProcessorModes(processor, &count, &guids);
todo_wine
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &media_type);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_SetOutputType(transform, 1, media_type, 0);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoProcessor_GetVideoProcessorMode(processor, &guid);
todo_wine
    ok(hr == S_FALSE, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoProcessor_GetAvailableVideoProcessorModes(processor, &count, &guids);
todo_wine
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    if (SUCCEEDED(hr))
        CoTaskMemFree(guids);

    hr = IMFTransform_GetOutputCurrentType(transform, 0, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(media_type == media_type2, "Unexpected media type instance.\n");
    IMFMediaType_Release(media_type2);
    IMFMediaType_Release(media_type);

    IMFVideoProcessor_Release(processor);

    IMFMediaType_Release(video_type);

    IDirect3DDeviceManager9_Release(manager);

    IDirect3DDevice9_Release(device);

done:
    IMFTransform_Release(transform);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_default_presenter(void)
{
    IMFVideoDisplayControl *display_control;
    IMFVideoPresenter *presenter;
    IMFRateSupport *rate_support;
    IDirect3DDeviceManager9 *dm;
    IMFVideoDeviceID *deviceid;
    HWND hwnd, hwnd2;
    DWORD flags;
    float rate;
    HRESULT hr;
    GUID iid;

    hr = MFCreateVideoPresenter(NULL, &IID_IMFVideoPresenter, &IID_IMFVideoPresenter, (void **)&presenter);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = MFCreateVideoPresenter(NULL, &IID_IDirect3DDevice9, &IID_IMFVideoPresenter, (void **)&presenter);
    ok(hr == S_OK || broken(hr == E_FAIL) /* WinXP */, "Failed to create default presenter, hr %#x.\n", hr);
    if (FAILED(hr))
        return;

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
    check_service_interface(presenter, &MR_VIDEO_ACCELERATION_SERVICE, &IID_IDirect3DDeviceManager9, TRUE);

    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFVideoDeviceID, (void **)&deviceid);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoDeviceID_GetDeviceID(deviceid, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoDeviceID_GetDeviceID(deviceid, &iid);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(IsEqualIID(&iid, &IID_IDirect3DDevice9), "Unexpected id %s.\n", wine_dbgstr_guid(&iid));

    IMFVideoDeviceID_Release(deviceid);

    hr = MFGetService((IUnknown *)presenter, &MR_VIDEO_RENDER_SERVICE, &IID_IMFVideoDisplayControl, (void **)&display_control);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoDisplayControl_GetRenderingPrefs(display_control, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    flags = 123;
    hr = IMFVideoDisplayControl_GetRenderingPrefs(display_control, &flags);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!flags, "Unexpected rendering flags %#x.\n", flags);

    IMFVideoDisplayControl_Release(display_control);

    hr = MFGetService((IUnknown *)presenter, &MR_VIDEO_ACCELERATION_SERVICE, &IID_IDirect3DDeviceManager9, (void **)&dm);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFVideoDisplayControl, (void **)&display_control);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    /* Video window */
    hwnd = create_window();
    ok(!!hwnd, "Failed to create a test window.\n");

    hr = IMFVideoDisplayControl_GetVideoWindow(display_control, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hwnd2 = hwnd;
    hr = IMFVideoDisplayControl_GetVideoWindow(display_control, &hwnd2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(hwnd2 == NULL, "Unexpected window %p.\n", hwnd2);

    hr = IMFVideoDisplayControl_SetVideoWindow(display_control, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoDisplayControl_SetVideoWindow(display_control, (HWND)0x1);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoDisplayControl_SetVideoWindow(display_control, hwnd);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hwnd2 = NULL;
    hr = IMFVideoDisplayControl_GetVideoWindow(display_control, &hwnd2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(hwnd2 == hwnd, "Unexpected window %p.\n", hwnd2);

    /* Rate support. */
    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFRateSupport, (void **)&rate_support);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    rate = 1.0f;
    hr = IMFRateSupport_GetSlowestRate(rate_support, MFRATE_FORWARD, FALSE, &rate);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(rate == 0.0f, "Unexpected rate %f.\n", rate);

    rate = 1.0f;
    hr = IMFRateSupport_GetSlowestRate(rate_support, MFRATE_FORWARD, TRUE, &rate);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(rate == 0.0f, "Unexpected rate %f.\n", rate);

    rate = 1.0f;
    hr = IMFRateSupport_GetSlowestRate(rate_support, MFRATE_REVERSE, FALSE, &rate);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(rate == 0.0f, "Unexpected rate %f.\n", rate);

    rate = 1.0f;
    hr = IMFRateSupport_GetSlowestRate(rate_support, MFRATE_REVERSE, TRUE, &rate);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(rate == 0.0f, "Unexpected rate %f.\n", rate);

    IMFRateSupport_Release(rate_support);

    IMFVideoPresenter_Release(presenter);

    DestroyWindow(hwnd);
}

static void test_MFCreateVideoMixerAndPresenter(void)
{
    IUnknown *mixer, *presenter;
    HRESULT hr;

    hr = MFCreateVideoMixerAndPresenter(NULL, NULL, &IID_IUnknown, (void **)&mixer, &IID_IUnknown, (void **)&presenter);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

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
    IDirect3D9 *d3d;
    IUnknown *unk;
    HWND window;
    HRESULT hr;
    BYTE *data;

    hr = MFCreateVideoSampleAllocator(&IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    IUnknown_Release(unk);

    hr = MFCreateVideoSampleAllocator(&IID_IMFVideoSampleAllocator, (void **)&allocator);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoSampleAllocator_QueryInterface(allocator, &IID_IMFVideoSampleAllocatorCallback, (void **)&allocator_cb);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoSampleAllocatorCallback_SetCallback(allocator_cb, NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoSampleAllocatorCallback_SetCallback(allocator_cb, &test_notify);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoSampleAllocatorCallback_SetCallback(allocator_cb, NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoSampleAllocatorCallback_GetFreeSampleCount(allocator_cb, NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    count = 10;
    hr = IMFVideoSampleAllocatorCallback_GetFreeSampleCount(allocator_cb, &count);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!count, "Unexpected count %d.\n", count);

    hr = IMFVideoSampleAllocator_UninitializeSampleAllocator(allocator);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoSampleAllocator_AllocateSample(allocator, &sample);
    ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoSampleAllocator_SetDirectXManager(allocator, NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    /* It expects IMFVideoMediaType. */
    hr = IMFVideoSampleAllocator_InitializeSampleAllocator(allocator, 2, media_type);
todo_wine
    ok(hr == E_NOINTERFACE, "Unexpected hr %#x.\n", hr);

    video_type = create_video_type(&MFVideoFormat_RGB32);

    hr = IMFVideoSampleAllocator_InitializeSampleAllocator(allocator, 2, video_type);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#x.\n", hr);

    /* Frame size is required. */
    hr = IMFMediaType_SetUINT64(video_type, &MF_MT_FRAME_SIZE, (UINT64) 320 << 32 | 240);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    hr = IMFVideoSampleAllocator_InitializeSampleAllocator(allocator, 0, video_type);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoSampleAllocatorCallback_GetFreeSampleCount(allocator_cb, &count);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(count == 1, "Unexpected count %d.\n", count);

    sample = NULL;
    hr = IMFVideoSampleAllocator_AllocateSample(allocator, &sample);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    refcount = get_refcount(sample);

    hr = IMFVideoSampleAllocator_AllocateSample(allocator, &sample2);
    ok(hr == MF_E_SAMPLEALLOCATOR_EMPTY, "Unexpected hr %#x.\n", hr);

    /* Reinitialize with active sample. */
    hr = IMFVideoSampleAllocator_InitializeSampleAllocator(allocator, 4, video_type);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(refcount == get_refcount(sample), "Unexpected refcount %u.\n", get_refcount(sample));

    hr = IMFVideoSampleAllocatorCallback_GetFreeSampleCount(allocator_cb, &count);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(count == 4, "Unexpected count %d.\n", count);

    hr = IMFSample_QueryInterface(sample, &IID_IMFDesiredSample, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    IUnknown_Release(unk);

    hr = IMFSample_QueryInterface(sample, &IID_IMFTrackedSample, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    IUnknown_Release(unk);

    hr = IMFSample_GetBufferByIndex(sample, 0, &buffer);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMFGetService, (void **)&gs);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* Win7 */, "Unexpected hr %#x.\n", hr);

    /* Device manager wasn't set, sample gets regular memory buffers. */
    if (SUCCEEDED(hr))
    {
        hr = IMFGetService_GetService(gs, &MR_BUFFER_SERVICE, &IID_IDirect3DSurface9, (void **)&surface);
        ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);
        IMFGetService_Release(gs);
    }

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    IUnknown_Release(unk);

    IMFMediaBuffer_Release(buffer);

    IMFSample_Release(sample);

    IMFVideoSampleAllocatorCallback_Release(allocator_cb);

    IMFVideoSampleAllocator_Release(allocator);

    hr = MFCreateVideoSampleAllocator(&IID_IMFVideoSampleAllocatorCallback, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    IUnknown_Release(unk);

    /* Using device manager */
    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = DXVA2CreateDirect3DDeviceManager9(&token, &manager);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    hr = IDirect3DDeviceManager9_ResetDevice(manager, device, token);
    ok(hr == S_OK, "Failed to set a device, hr %#x.\n", hr);

    hr = MFCreateVideoSampleAllocator(&IID_IMFVideoSampleAllocator, (void **)&allocator);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoSampleAllocator_SetDirectXManager(allocator, (IUnknown *)manager);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaType_SetUINT64(video_type, &MF_MT_FRAME_SIZE, (UINT64) 320 << 32 | 240);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    hr = IMFVideoSampleAllocator_InitializeSampleAllocator(allocator, 0, video_type);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoSampleAllocator_AllocateSample(allocator, &sample);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_GetBufferByIndex(sample, 0, &buffer);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    check_service_interface(buffer, &MR_BUFFER_SERVICE, &IID_IDirect3DSurface9, TRUE);

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    IUnknown_Release(unk);

    hr = IMFMediaBuffer_Lock(buffer, &data, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    IMFSample_Release(sample);

    IMFVideoSampleAllocator_Release(allocator);

    IMFMediaType_Release(media_type);
    IDirect3DDeviceManager9_Release(manager);
    IDirect3DDevice9_Release(device);
done:
    IDirect3D9_Release(d3d);
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

    ok(*num_objects == 1, "Unexpected number of requested objects %u\n", *num_objects);

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
    HRESULT hr;
    DWORD count;
    HWND hwnd;

    hr = MFCreateVideoMixer(NULL, &IID_IDirect3DDevice9, &IID_IMFTransform, (void **)&mixer);
    ok(hr == S_OK, "Failed to create a mixer, hr %#x.\n", hr);

    hr = MFCreateVideoPresenter(NULL, &IID_IDirect3DDevice9, &IID_IMFVideoPresenter, (void **)&presenter);
    ok(hr == S_OK, "Failed to create default presenter, hr %#x.\n", hr);

    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFTopologyServiceLookupClient, (void **)&lookup_client);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    init_test_host(&host, mixer, presenter);

    /* Clear default mixer attributes, then attach presenter. */
    hr = IMFTransform_GetAttributes(mixer, &mixer_attributes);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFAttributes_DeleteAllItems(mixer_attributes);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFTopologyServiceLookupClient_InitServicePointers(lookup_client, &host.IMFTopologyServiceLookup_iface);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFAttributes_GetCount(mixer_attributes, &count);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);

    memset(&src_rect, 0, sizeof(src_rect));
    hr = IMFAttributes_GetBlob(mixer_attributes, &VIDEO_ZOOM_RECT, (UINT8 *)&src_rect, sizeof(src_rect), NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(src_rect.left == 0.0f && src_rect.top == 0.0f && src_rect.right == 1.0f &&
            src_rect.bottom == 1.0f, "Unexpected source rectangle.\n");

    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFVideoDisplayControl, (void **)&display_control);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoDisplayControl_GetVideoPosition(display_control, NULL, &dst_rect);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoDisplayControl_GetVideoPosition(display_control, &src_rect, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    SetRect(&dst_rect, 1, 2, 3, 4);
    hr = IMFVideoDisplayControl_GetVideoPosition(display_control, &src_rect, &dst_rect);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(src_rect.left == 0.0f && src_rect.top == 0.0f && src_rect.right == 1.0f &&
            src_rect.bottom == 1.0f, "Unexpected source rectangle.\n");
    ok(dst_rect.left == 0 && dst_rect.right == 0 && dst_rect.top == 0 && dst_rect.bottom == 0,
            "Unexpected destination rectangle %s.\n", wine_dbgstr_rect(&dst_rect));

    hr = IMFVideoDisplayControl_SetVideoPosition(display_control, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    /* Setting position requires a window. */
    hwnd = create_window();
    ok(!!hwnd, "Failed to create a test window.\n");

    SetRect(&dst_rect, 0, 0, 10, 10);
    memset(&src_rect, 0, sizeof(src_rect));
    hr = IMFVideoDisplayControl_SetVideoPosition(display_control, &src_rect, &dst_rect);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoDisplayControl_SetVideoWindow(display_control, hwnd);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    SetRect(&dst_rect, 0, 0, 10, 10);
    hr = IMFVideoDisplayControl_SetVideoPosition(display_control, NULL, &dst_rect);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    SetRect(&dst_rect, 1, 2, 3, 4);
    hr = IMFVideoDisplayControl_GetVideoPosition(display_control, &src_rect, &dst_rect);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(dst_rect.left == 0 && dst_rect.right == 10 && dst_rect.top == 0 && dst_rect.bottom == 10,
            "Unexpected destination rectangle %s.\n", wine_dbgstr_rect(&dst_rect));

    set_rect(&src_rect, 0.0f, 0.0f, 2.0f, 1.0f);
    hr = IMFVideoDisplayControl_SetVideoPosition(display_control, &src_rect, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    set_rect(&src_rect, -0.1f, 0.0f, 0.9f, 1.0f);
    hr = IMFVideoDisplayControl_SetVideoPosition(display_control, &src_rect, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    /* Flipped source rectangle. */
    set_rect(&src_rect, 0.5f, 0.0f, 0.4f, 1.0f);
    hr = IMFVideoDisplayControl_SetVideoPosition(display_control, &src_rect, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    set_rect(&src_rect, 0.0f, 0.5f, 0.4f, 0.1f);
    hr = IMFVideoDisplayControl_SetVideoPosition(display_control, &src_rect, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    set_rect(&src_rect, 0.1f, 0.2f, 0.8f, 0.9f);
    hr = IMFVideoDisplayControl_SetVideoPosition(display_control, &src_rect, NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    /* Presenter updates mixer attribute. */
    memset(&src_rect, 0, sizeof(src_rect));
    hr = IMFAttributes_GetBlob(mixer_attributes, &VIDEO_ZOOM_RECT, (UINT8 *)&src_rect, sizeof(src_rect), NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(src_rect.left == 0.1f && src_rect.top == 0.2f && src_rect.right == 0.8f &&
            src_rect.bottom == 0.9f, "Unexpected source rectangle.\n");

    hr = IMFVideoDisplayControl_GetVideoPosition(display_control, &src_rect, &dst_rect);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(src_rect.left == 0.1f && src_rect.top == 0.2f && src_rect.right == 0.8f &&
            src_rect.bottom == 0.9f, "Unexpected source rectangle.\n");

    SetRect(&dst_rect, 1, 2, 999, 1000);
    hr = IMFVideoDisplayControl_SetVideoPosition(display_control, NULL, &dst_rect);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    SetRect(&dst_rect, 0, 1, 3, 4);
    hr = IMFVideoDisplayControl_GetVideoPosition(display_control, &src_rect, &dst_rect);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(dst_rect.left == 1 && dst_rect.right == 999 && dst_rect.top == 2 && dst_rect.bottom == 1000,
            "Unexpected destination rectangle %s.\n", wine_dbgstr_rect(&dst_rect));

    /* Flipped destination rectangle. */
    SetRect(&dst_rect, 100, 1, 50, 1000);
    hr = IMFVideoDisplayControl_SetVideoPosition(display_control, NULL, &dst_rect);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    SetRect(&dst_rect, 1, 100, 100, 50);
    hr = IMFVideoDisplayControl_SetVideoPosition(display_control, NULL, &dst_rect);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

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
    ok(hr == S_OK, "Failed to create a mixer, hr %#x.\n", hr);

    hr = MFCreateVideoPresenter(NULL, &IID_IDirect3DDevice9, &IID_IMFVideoPresenter, (void **)&presenter);
    ok(hr == S_OK, "Failed to create default presenter, hr %#x.\n", hr);

    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFTopologyServiceLookupClient, (void **)&lookup_client);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFVideoDisplayControl, (void **)&display_control);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoDisplayControl_GetNativeVideoSize(display_control, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    memset(&size, 0xcc, sizeof(size));
    hr = IMFVideoDisplayControl_GetNativeVideoSize(display_control, &size, NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(size.cx == 0 && size.cy == 0, "Unexpected size.\n");

    memset(&ratio, 0xcc, sizeof(ratio));
    hr = IMFVideoDisplayControl_GetNativeVideoSize(display_control, NULL, &ratio);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(ratio.cx == 0 && ratio.cy == 0, "Unexpected ratio.\n");

    hr = IMFVideoPresenter_QueryInterface(presenter, &IID_IMFTopologyServiceLookupClient, (void **)&lookup_client);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    /* Configure mixer primary stream. */
    hr = MFGetService((IUnknown *)presenter, &MR_VIDEO_ACCELERATION_SERVICE, &IID_IDirect3DDeviceManager9, (void **)&dm);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_ProcessMessage(mixer, MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)dm);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    IDirect3DDeviceManager9_Release(dm);

    video_type = create_video_type(&MFVideoFormat_RGB32);

    hr = IMFMediaType_SetUINT64(video_type, &MF_MT_FRAME_SIZE, (UINT64)640 << 32 | 480);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    hr = IMFMediaType_SetUINT32(video_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_SetInputType(mixer, 0, video_type, 0);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    /* Native video size is cached on initialization. */
    init_test_host(&host, mixer, presenter);

    hr = IMFTopologyServiceLookupClient_InitServicePointers(lookup_client, &host.IMFTopologyServiceLookup_iface);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoDisplayControl_GetNativeVideoSize(display_control, &size, &ratio);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(size.cx == 640 && size.cy == 480, "Unexpected size %u x %u.\n", size.cx, size.cy);
    ok((ratio.cx == 4 && ratio.cy == 3) || broken(!memcmp(&ratio, &size, sizeof(ratio))) /* < Win10 */,
            "Unexpected ratio %u x %u.\n", ratio.cx, ratio.cy);

    /* Update input type. */
    hr = IMFMediaType_SetUINT64(video_type, &MF_MT_FRAME_SIZE, (UINT64)320 << 32 | 240);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_SetInputType(mixer, 0, video_type, 0);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoDisplayControl_GetNativeVideoSize(display_control, &size, &ratio);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(size.cx == 640 && size.cy == 480, "Unexpected size %u x %u.\n", size.cx, size.cy);
    ok((ratio.cx == 4 && ratio.cy == 3) || broken(!memcmp(&ratio, &size, sizeof(ratio))) /* < Win10 */,
            "Unexpected ratio %u x %u.\n", ratio.cx, ratio.cy);

    /* Negotiating types updates native video size. */
    hr = IMFVideoPresenter_ProcessMessage(presenter, MFVP_MESSAGE_INVALIDATEMEDIATYPE, 0);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoDisplayControl_GetNativeVideoSize(display_control, &size, &ratio);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(size.cx == 320 && size.cy == 240, "Unexpected size %u x %u.\n", size.cx, size.cy);
    ok((ratio.cx == 4 && ratio.cy == 3) || broken(!memcmp(&ratio, &size, sizeof(ratio))) /* < Win10 */,
            "Unexpected ratio %u x %u.\n", ratio.cx, ratio.cy);

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
    ok(hr == S_OK, "Failed to create default presenter, hr %#x.\n", hr);

    hr = IMFVideoDisplayControl_GetAspectRatioMode(display_control, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    mode = 0;
    hr = IMFVideoDisplayControl_GetAspectRatioMode(display_control, &mode);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(mode == (MFVideoARMode_PreservePicture | MFVideoARMode_PreservePixel), "Unexpected mode %#x.\n", mode);

    hr = IMFVideoDisplayControl_SetAspectRatioMode(display_control, 0x100);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoDisplayControl_SetAspectRatioMode(display_control, MFVideoARMode_Mask);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    mode = 0;
    hr = IMFVideoDisplayControl_GetAspectRatioMode(display_control, &mode);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(mode == MFVideoARMode_Mask, "Unexpected mode %#x.\n", mode);

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
    ok(hr == S_OK, "Failed to create default presenter, hr %#x.\n", hr);

    hr = MFGetService((IUnknown *)display_control, &MR_VIDEO_ACCELERATION_SERVICE,
         &IID_IDirect3DDeviceManager9, (void **)&dm);

    hr = IDirect3DDeviceManager9_OpenDeviceHandle(dm, &hdevice);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_LockDevice(dm, hdevice, &d3d_device, FALSE);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetCreationParameters(d3d_device, &device_params);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(device_params.hFocusWindow == GetDesktopWindow(), "Unexpected window %p.\n", device_params.hFocusWindow);

    hr = IDirect3DDevice9_GetSwapChain(d3d_device, 0, &swapchain);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DSwapChain9_GetPresentParameters(swapchain, &present_params);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    ok(present_params.hDeviceWindow == GetDesktopWindow(), "Unexpected device window.\n");
    ok(present_params.Windowed, "Unexpected windowed mode.\n");
    ok(present_params.SwapEffect == D3DSWAPEFFECT_COPY, "Unexpected swap effect.\n");
    ok(present_params.Flags & D3DPRESENTFLAG_VIDEO, "Unexpected flags %#x.\n", present_params.Flags);
    ok(present_params.PresentationInterval == D3DPRESENT_INTERVAL_IMMEDIATE, "Unexpected present interval.\n");

    IDirect3DSwapChain9_Release(swapchain);
    IDirect3DDevice9_Release(d3d_device);

    hr = IDirect3DDeviceManager9_UnlockDevice(dm, hdevice, FALSE);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    /* Setting window. */
    hr = IMFVideoDisplayControl_GetVideoWindow(display_control, &window);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!window, "Unexpected window %p.\n", window);

    window = create_window();

    hr = IMFVideoDisplayControl_SetVideoWindow(display_control, window);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    /* Device is not recreated or reset on window change. */
    hr = IDirect3DDeviceManager9_LockDevice(dm, hdevice, &d3d_device, FALSE);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetSwapChain(d3d_device, 0, &swapchain);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DSwapChain9_GetPresentParameters(swapchain, &present_params);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    ok(present_params.hDeviceWindow == GetDesktopWindow(), "Unexpected device window.\n");
    ok(present_params.Windowed, "Unexpected windowed mode.\n");
    ok(present_params.SwapEffect == D3DSWAPEFFECT_COPY, "Unexpected swap effect.\n");
    ok(present_params.Flags & D3DPRESENTFLAG_VIDEO, "Unexpected flags %#x.\n", present_params.Flags);
    ok(present_params.PresentationInterval == D3DPRESENT_INTERVAL_IMMEDIATE, "Unexpected present interval.\n");

    IDirect3DSwapChain9_Release(swapchain);
    IDirect3DDevice9_Release(d3d_device);

    hr = IDirect3DDeviceManager9_UnlockDevice(dm, hdevice, FALSE);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(dm, hdevice);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    IMFVideoDisplayControl_Release(display_control);

    DestroyWindow(window);
}

static void test_mixer_output_rectangle(void)
{
    IMFVideoMixerControl *mixer_control;
    MFVideoNormalizedRect rect;
    IMFTransform *mixer;
    HRESULT hr;

    hr = MFCreateVideoMixer(NULL, &IID_IDirect3DDevice9, &IID_IMFTransform, (void **)&mixer);
    ok(hr == S_OK, "Failed to create a mixer, hr %#x.\n", hr);

    hr = IMFTransform_QueryInterface(mixer, &IID_IMFVideoMixerControl, (void **)&mixer_control);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoMixerControl_GetStreamOutputRect(mixer_control, 0, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = IMFVideoMixerControl_GetStreamOutputRect(mixer_control, 0, &rect);
    ok(hr == S_OK, "Failed to get output rect, hr %#x.\n", hr);
    ok(rect.left == 0.0f && rect.top == 0.0f && rect.right == 1.0f && rect.bottom == 1.0f,
            "Unexpected rectangle.\n");

    hr = IMFVideoMixerControl_GetStreamOutputRect(mixer_control, 1, &rect);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoMixerControl_GetStreamOutputRect(mixer_control, 1, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoMixerControl_SetStreamOutputRect(mixer_control, 1, &rect);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoMixerControl_SetStreamOutputRect(mixer_control, 1, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    /* Wrong bounds. */
    set_rect(&rect, 0.0f, 0.0f, 1.1f, 1.0f);
    hr = IMFVideoMixerControl_SetStreamOutputRect(mixer_control, 0, &rect);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    set_rect(&rect, -0.1f, 0.0f, 0.5f, 1.0f);
    hr = IMFVideoMixerControl_SetStreamOutputRect(mixer_control, 0, &rect);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    /* Flipped. */
    set_rect(&rect, 1.0f, 0.0f, 0.0f, 1.0f);
    hr = IMFVideoMixerControl_SetStreamOutputRect(mixer_control, 0, &rect);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    set_rect(&rect, 0.0f, 1.0f, 1.0f, 0.5f);
    hr = IMFVideoMixerControl_SetStreamOutputRect(mixer_control, 0, &rect);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoMixerControl_SetStreamOutputRect(mixer_control, 0, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

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
    ok(hr == S_OK, "Failed to create a mixer, hr %#x.\n", hr);

    hr = IMFTransform_QueryInterface(mixer, &IID_IMFVideoMixerControl, (void **)&mixer_control);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoMixerControl_GetStreamZOrder(mixer_control, 0, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoMixerControl_GetStreamZOrder(mixer_control, 1, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    value = 1;
    hr = IMFVideoMixerControl_GetStreamZOrder(mixer_control, 0, &value);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!value, "Unexpected value %u.\n", value);

    value = 1;
    hr = IMFVideoMixerControl_GetStreamZOrder(mixer_control, 1, &value);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoMixerControl_SetStreamZOrder(mixer_control, 0, 1);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoMixerControl_SetStreamZOrder(mixer_control, 1, 1);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    /* Exceeds maximum stream number. */
    hr = IMFVideoMixerControl_SetStreamZOrder(mixer_control, 0, 20);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    value = 1;
    hr = IMFTransform_AddInputStreams(mixer, 1, &value);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    value = 0;
    hr = IMFVideoMixerControl_GetStreamZOrder(mixer_control, 1, &value);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(value == 1, "Unexpected zorder %u.\n", value);

    hr = IMFVideoMixerControl_SetStreamZOrder(mixer_control, 1, 0);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoMixerControl_SetStreamZOrder(mixer_control, 1, 2);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoMixerControl_SetStreamZOrder(mixer_control, 0, 0);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    value = 2;
    hr = IMFTransform_AddInputStreams(mixer, 1, &value);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    value = 0;
    hr = IMFVideoMixerControl_GetStreamZOrder(mixer_control, 2, &value);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(value == 2, "Unexpected zorder %u.\n", value);

    hr = IMFVideoMixerControl_SetStreamZOrder(mixer_control, 2, 1);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    value = 3;
    hr = IMFTransform_AddInputStreams(mixer, 1, &value);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    value = 0;
    hr = IMFVideoMixerControl_GetStreamZOrder(mixer_control, 3, &value);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(value == 3, "Unexpected zorder %u.\n", value);

    hr = IMFTransform_DeleteInputStream(mixer, 1);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    hr = IMFTransform_DeleteInputStream(mixer, 2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    hr = IMFTransform_DeleteInputStream(mixer, 3);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    ids[0] = 2;
    ids[1] = 1;
    hr = IMFTransform_AddInputStreams(mixer, 2, ids);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    value = 0;
    hr = IMFVideoMixerControl_GetStreamZOrder(mixer_control, 1, &value);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(value == 2, "Unexpected zorder %u.\n", value);

    value = 0;
    hr = IMFVideoMixerControl_GetStreamZOrder(mixer_control, 2, &value);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(value == 1, "Unexpected zorder %u.\n", value);

    IMFVideoMixerControl_Release(mixer_control);
    IMFTransform_Release(mixer);
}

static IDirect3DSurface9 * create_surface(IDirect3DDeviceManager9 *manager, unsigned int width,
        unsigned int height)
{
    IDirectXVideoAccelerationService *service;
    IDirect3DSurface9 *surface = NULL;
    IDirect3DDevice9 *device;
    HANDLE handle;
    HRESULT hr;

    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_LockDevice(manager, handle, &device, TRUE);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_GetVideoService(manager, handle, &IID_IDirectXVideoProcessorService,
            (void **)&service);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirectXVideoAccelerationService_CreateSurface(service, width, height, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_DEFAULT, 0, DXVA2_VideoProcessorRenderTarget, &surface, NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    IDirectXVideoAccelerationService_Release(service);

    hr = IDirect3DDevice9_ColorFill(device, surface, NULL, D3DCOLOR_ARGB(0x10, 0xff, 0x00, 0x00));
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    IDirect3DDevice9_Release(device);

    hr = IDirect3DDeviceManager9_UnlockDevice(manager, handle, FALSE);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

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
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(x < desc.Width && y < desc.Height, "Invalid coordinate.\n");
    if (x >= desc.Width || y >= desc.Height) return 0;

    hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, 0);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    row = (DWORD *)((char *)locked_rect.pBits + y * locked_rect.Pitch);
    color = row[x];

    hr = IDirect3DSurface9_UnlockRect(surface);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

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
    DWORD count, flags, color, status;
    IMFTransform *mixer;
    IMFSample *sample, *sample2;
    IDirect3D9 *d3d;
    HWND window;
    UINT token;
    HRESULT hr;
    LONGLONG pts, duration;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = MFCreateVideoMixer(NULL, &IID_IDirect3DDevice9, &IID_IMFTransform, (void **)&mixer);
    ok(hr == S_OK, "Failed to create a mixer, hr %#x.\n", hr);

    hr = IMFTransform_QueryInterface(mixer, &IID_IMFVideoProcessor, (void **)&processor);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetInputStatus(mixer, 0, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetInputStatus(mixer, 1, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetInputStatus(mixer, 0, &status);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetInputStatus(mixer, 1, &status);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetOutputStatus(mixer, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetOutputStatus(mixer, &status);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#x.\n", hr);

    /* Configure device and media types. */
    hr = DXVA2CreateDirect3DDeviceManager9(&token, &manager);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DDeviceManager9_ResetDevice(manager, device, token);
    ok(hr == S_OK, "Failed to set a device, hr %#x.\n", hr);

    hr = IMFTransform_ProcessMessage(mixer, MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)manager);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    video_type = create_video_type(&MFVideoFormat_RGB32);

    hr = IMFMediaType_SetUINT64(video_type, &MF_MT_FRAME_SIZE, (UINT64)640 << 32 | 480);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    hr = IMFMediaType_SetUINT32(video_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_SetInputType(mixer, 0, video_type, 0);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_GetInputStatus(mixer, 0, &status);
    ok(hr == MF_E_TRANSFORM_TYPE_NOT_SET, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_SetOutputType(mixer, 0, video_type, 0);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    status = 0;
    hr = IMFTransform_GetInputStatus(mixer, 0, &status);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(status == MFT_INPUT_STATUS_ACCEPT_DATA, "Unexpected status %#x.\n", status);

    hr = IMFTransform_GetInputStatus(mixer, 1, &status);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    status = ~0u;
    hr = IMFTransform_GetOutputStatus(mixer, &status);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!status, "Unexpected status %#x.\n", status);

    IMFMediaType_Release(video_type);

    memset(buffers, 0, sizeof(buffers));
    hr = IMFTransform_ProcessOutput(mixer, 0, 1, buffers, &status);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    /* It needs a sample with a backing surface. */
    hr = MFCreateSample(&sample);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    buffers[0].pSample = sample;
    hr = IMFTransform_ProcessOutput(mixer, 0, 1, buffers, &status);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    IMFSample_Release(sample);

    surface = create_surface(manager, 64, 64);

    hr = MFCreateVideoSampleFromSurface((IUnknown *)surface, &sample);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_QueryInterface(sample, &IID_IMFDesiredSample, (void **)&desired);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    buffers[0].pSample = sample;
    hr = IMFTransform_ProcessOutput(mixer, 0, 1, buffers, &status);
    ok(hr == MF_E_TRANSFORM_NEED_MORE_INPUT, "Unexpected hr %#x.\n", hr);

    color = get_surface_color(surface, 0, 0);
    ok(color == D3DCOLOR_ARGB(0x10, 0xff, 0x00, 0x00), "Unexpected color %#x.\n", color);

    /* Streaming is not started yet. Output is colored black, but only if desired timestamps were set. */
    IMFDesiredSample_SetDesiredSampleTimeAndDuration(desired, 100, 0);

    hr = IMFTransform_ProcessOutput(mixer, 0, 1, buffers, &status);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    color = get_surface_color(surface, 0, 0);
    ok(!color, "Unexpected color %#x.\n", color);

    hr = IMFVideoProcessor_SetBackgroundColor(processor, RGB(0, 0, 255));
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_ProcessOutput(mixer, 0, 1, buffers, &status);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    color = get_surface_color(surface, 0, 0);
    ok(!color, "Unexpected color %#x.\n", color);

    hr = IMFTransform_ProcessOutput(mixer, 0, 2, buffers, &status);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    buffers[1].pSample = sample;
    hr = IMFTransform_ProcessOutput(mixer, 0, 2, buffers, &status);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    buffers[0].dwStreamID = 1;
    hr = IMFTransform_ProcessOutput(mixer, 0, 1, buffers, &status);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    IMFDesiredSample_Clear(desired);

    hr = IMFTransform_ProcessInput(mixer, 0, NULL, 0);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_ProcessInput(mixer, 5, NULL, 0);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    status = 0;
    hr = IMFTransform_GetInputStatus(mixer, 0, &status);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(status == MFT_INPUT_STATUS_ACCEPT_DATA, "Unexpected status %#x.\n", status);

    status = ~0u;
    hr = IMFTransform_GetOutputStatus(mixer, &status);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!status, "Unexpected status %#x.\n", status);

    hr = IMFTransform_ProcessInput(mixer, 0, sample, 0);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    status = ~0u;
    hr = IMFTransform_GetInputStatus(mixer, 0, &status);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!status, "Unexpected status %#x.\n", status);

    hr = IMFTransform_GetOutputStatus(mixer, &status);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(status == MFT_OUTPUT_STATUS_SAMPLE_READY, "Unexpected status %#x.\n", status);

    hr = IMFTransform_ProcessInput(mixer, 0, sample, 0);
    ok(hr == MF_E_NOTACCEPTING, "Unexpected hr %#x.\n", hr);

    hr = IMFTransform_ProcessInput(mixer, 5, sample, 0);
    ok(hr == MF_E_INVALIDSTREAMNUMBER, "Unexpected hr %#x.\n", hr);

    /* ProcessOutput() sets sample time and duration. */
    hr = MFCreateVideoSampleFromSurface((IUnknown *)surface, &sample2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_SetUINT32(sample2, &IID_IMFSample, 1);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_SetSampleFlags(sample2, 0x123);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_GetSampleTime(sample2, &pts);
    ok(hr == MF_E_NO_SAMPLE_TIMESTAMP, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_GetSampleDuration(sample2, &duration);
    ok(hr == MF_E_NO_SAMPLE_DURATION, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_SetSampleTime(sample, 0);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_SetSampleDuration(sample, 0);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    memset(buffers, 0, sizeof(buffers));
    buffers[0].pSample = sample2;
    hr = IMFTransform_ProcessOutput(mixer, 0, 1, buffers, &status);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_GetSampleTime(sample2, &pts);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!pts, "Unexpected sample time.\n");

    hr = IMFSample_GetSampleDuration(sample2, &duration);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!duration, "Unexpected duration\n");

    /* Flags are not copied. */
    hr = IMFSample_GetSampleFlags(sample2, &flags);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(flags == 0x123, "Unexpected flags %#x.\n", flags);

    /* Attributes are not removed. */
    hr = IMFSample_GetCount(sample2, &count);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(count == 1, "Unexpected attribute count %u.\n", count);

    hr = IMFSample_GetCount(sample, &count);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!count, "Unexpected attribute count %u.\n", count);

    IMFSample_Release(sample2);

    hr = IMFTransform_ProcessMessage(mixer, MFT_MESSAGE_COMMAND_DRAIN, 0);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    IMFSample_Release(sample);

    IDirect3DSurface9_Release(surface);

    IMFVideoProcessor_Release(processor);
    IMFTransform_Release(mixer);

    IDirect3DDevice9_Release(device);
    IDirect3DDeviceManager9_Release(manager);

done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

START_TEST(evr)
{
    CoInitialize(NULL);

    test_aggregation();
    test_interfaces();
    test_enum_pins();
    test_find_pin();
    test_pin_info();
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
    test_mixer_output_rectangle();
    test_mixer_zorder();
    test_mixer_samples();

    CoUninitialize();
}
