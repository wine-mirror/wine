/*
 * Copyright 2017 Nikolay Sivov
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
#include <string.h>
#include <float.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"

#include "uuids.h"
#include "wmcodecdsp.h"
#include "d3d9.h"
#include "evr9.h"
#include "d3d11_4.h"

#include "mferror.h"

#include "mf_test.h"

#include "wine/test.h"

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func  "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func);     \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

#define CHECK_NOT_CALLED(func) \
    do { \
        ok(!called_ ## func, "unexpected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

#define CLEAR_CALLED(func) \
    expect_ ## func = called_ ## func = FALSE

extern GUID DMOVideoFormat_RGB32;

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG expected_refcount, int line)
{
    ULONG refcount;
    IUnknown_AddRef(obj);
    refcount = IUnknown_Release(obj);
    ok_(__FILE__, line)(refcount == expected_refcount, "Unexpected refcount %ld, expected %ld.\n", refcount,
            expected_refcount);
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

#define check_subtype(a, b) check_subtype_(__LINE__, a, b)
void check_subtype_(unsigned int line, IMFMediaType *media_type, const GUID * const *expected)
{
    GUID subtype = {0};
    HRESULT hr;
    UINT i;

    hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &subtype);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    for (i = 0; expected[i]; ++i)
        if (IsEqualGUID(&subtype, expected[i]))
            return;

    ok(0, "Unexpected subtype %s.\n", debugstr_guid(&subtype));
}

static HWND create_window(void)
{
    RECT r = {0, 0, 640, 480};

    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW | WS_VISIBLE, FALSE);

    return CreateWindowA("static", "mf_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
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

static IMFSample *create_sample(const BYTE *data, ULONG size)
{
    IMFMediaBuffer *media_buffer;
    IMFSample *sample;
    DWORD length;
    BYTE *buffer;
    HRESULT hr;
    ULONG ret;

    hr = MFCreateSample(&sample);
    ok(hr == S_OK, "MFCreateSample returned %#lx\n", hr);
    hr = MFCreateMemoryBuffer(size, &media_buffer);
    ok(hr == S_OK, "MFCreateMemoryBuffer returned %#lx\n", hr);
    hr = IMFMediaBuffer_Lock(media_buffer, &buffer, NULL, &length);
    ok(hr == S_OK, "Lock returned %#lx\n", hr);
    ok(length == 0, "got length %lu\n", length);
    if (!data) memset(buffer, 0xcd, size);
    else memcpy(buffer, data, size);
    hr = IMFMediaBuffer_Unlock(media_buffer);
    ok(hr == S_OK, "Unlock returned %#lx\n", hr);
    hr = IMFMediaBuffer_SetCurrentLength(media_buffer, data ? size : 0);
    ok(hr == S_OK, "SetCurrentLength returned %#lx\n", hr);
    hr = IMFSample_AddBuffer(sample, media_buffer);
    ok(hr == S_OK, "AddBuffer returned %#lx\n", hr);
    ret = IMFMediaBuffer_Release(media_buffer);
    ok(ret == 1, "Release returned %lu\n", ret);

    return sample;
}

static void create_descriptors(UINT enum_types_count, IMFMediaType **enum_types, const media_type_desc *current_desc,
        IMFPresentationDescriptor **pd, IMFStreamDescriptor **sd)
{
    HRESULT hr;

    hr = MFCreateStreamDescriptor(0, enum_types_count, enum_types, sd);
    ok(hr == S_OK, "Failed to create stream descriptor, hr %#lx.\n", hr);

    hr = MFCreatePresentationDescriptor(1, sd, pd);
    ok(hr == S_OK, "Failed to create presentation descriptor, hr %#lx.\n", hr);

    if (current_desc)
    {
        IMFMediaTypeHandler *handler;
        IMFMediaType *type;

        hr = MFCreateMediaType(&type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        init_media_type(type, *current_desc, -1);

        hr = IMFStreamDescriptor_GetMediaTypeHandler(*sd, &handler);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        IMFMediaTypeHandler_Release(handler);
        IMFMediaType_Release(type);
    }
}

static void init_source_node(IMFMediaSource *source, MF_CONNECT_METHOD method, IMFTopologyNode *node,
        IMFPresentationDescriptor *pd, IMFStreamDescriptor *sd)
{
    HRESULT hr;

    hr = IMFTopologyNode_DeleteAllItems(node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetUnknown(node, &MF_TOPONODE_PRESENTATION_DESCRIPTOR, (IUnknown *)pd);
    ok(hr == S_OK, "Failed to set node pd, hr %#lx.\n", hr);
    hr = IMFTopologyNode_SetUnknown(node, &MF_TOPONODE_STREAM_DESCRIPTOR, (IUnknown *)sd);
    ok(hr == S_OK, "Failed to set node sd, hr %#lx.\n", hr);

    if (method != -1)
    {
        hr = IMFTopologyNode_SetUINT32(node, &MF_TOPONODE_CONNECT_METHOD, method);
        ok(hr == S_OK, "Failed to set connect method, hr %#lx.\n", hr);
    }

    if (source)
    {
        hr = IMFTopologyNode_SetUnknown(node, &MF_TOPONODE_SOURCE, (IUnknown *)source);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    }
}

static void init_sink_node(IMFStreamSink *stream_sink, MF_CONNECT_METHOD method, IMFTopologyNode *node)
{
    HRESULT hr;

    hr = IMFTopologyNode_DeleteAllItems(node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetObject(node, (IUnknown *)stream_sink);
    ok(hr == S_OK, "Failed to set object, hr %#lx.\n", hr);

    if (method != -1)
    {
        hr = IMFTopologyNode_SetUINT32(node, &MF_TOPONODE_CONNECT_METHOD, method);
        ok(hr == S_OK, "Failed to set connect method, hr %#lx.\n", hr);
    }
}

struct test_transform
{
    IMFTransform IMFTransform_iface;
    LONG refcount;

    IMFAttributes *attributes;
    const MFT_OUTPUT_STREAM_INFO *output_stream_info;

    UINT input_count;
    IMFMediaType **input_types;
    IMFMediaType *input_type;

    UINT output_count;
    IMFMediaType **output_types;
    IMFMediaType *output_type;
    BOOL input_enum_complete;
    BOOL output_enum_complete;
    BOOL input_type_set;
    BOOL output_type_set;
    BOOL set_input_nulls_output;

    IDirect3DDeviceManager9 *expect_d3d9_device_manager;
    BOOL got_d3d9_device_manager;
    IMFDXGIDeviceManager *expect_dxgi_device_manager;
    BOOL got_dxgi_device_manager;
};

static struct test_transform *test_transform_from_IMFTransform(IMFTransform *iface)
{
    return CONTAINING_RECORD(iface, struct test_transform, IMFTransform_iface);
}

static HRESULT WINAPI test_transform_QueryInterface(IMFTransform *iface, REFIID iid, void **out)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);

    if (IsEqualGUID(iid, &IID_IUnknown)
            || IsEqualGUID(iid, &IID_IMFTransform))
    {
        IMFTransform_AddRef(&transform->IMFTransform_iface);
        *out = &transform->IMFTransform_iface;
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_transform_AddRef(IMFTransform *iface)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);
    ULONG refcount = InterlockedIncrement(&transform->refcount);
    return refcount;
}

static ULONG WINAPI test_transform_Release(IMFTransform *iface)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);
    ULONG refcount = InterlockedDecrement(&transform->refcount);

    if (!refcount)
    {
        if (transform->input_type)
            IMFMediaType_Release(transform->input_type);
        if (transform->output_type)
            IMFMediaType_Release(transform->output_type);
        if (transform->attributes)
            IMFAttributes_Release(transform->attributes);
        free(transform);
    }

    return refcount;
}

static HRESULT WINAPI test_transform_GetStreamLimits(IMFTransform *iface, DWORD *input_minimum,
        DWORD *input_maximum, DWORD *output_minimum, DWORD *output_maximum)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_transform_GetStreamCount(IMFTransform *iface, DWORD *inputs, DWORD *outputs)
{
    *inputs = *outputs = 1;
    return S_OK;
}

static HRESULT WINAPI test_transform_GetStreamIDs(IMFTransform *iface, DWORD input_size, DWORD *inputs,
        DWORD output_size, DWORD *outputs)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_transform_GetInputStreamInfo(IMFTransform *iface, DWORD id, MFT_INPUT_STREAM_INFO *info)
{
    ok(0, "Unexpected %s call.\n", __func__);
    return E_NOTIMPL;
}

static void test_transform_set_output_stream_info(IMFTransform *iface, const MFT_OUTPUT_STREAM_INFO *info)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);
    transform->output_stream_info = info;
}

static HRESULT WINAPI test_transform_GetOutputStreamInfo(IMFTransform *iface, DWORD id, MFT_OUTPUT_STREAM_INFO *info)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);

    ok(!!transform->output_stream_info, "Unexpected %s iface %p call.\n", __func__, iface);
    if (!transform->output_stream_info)
        return E_NOTIMPL;

    *info = *transform->output_stream_info;
    return S_OK;
}

static HRESULT WINAPI test_transform_GetAttributes(IMFTransform *iface, IMFAttributes **attributes)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);
    if (!(*attributes = transform->attributes))
        return E_NOTIMPL;
    IMFAttributes_AddRef(*attributes);
    return S_OK;
}

static HRESULT WINAPI test_transform_GetInputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_transform_GetOutputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_transform_DeleteInputStream(IMFTransform *iface, DWORD id)
{
    ok(0, "Unexpected %s call.\n", __func__);
    return E_NOTIMPL;
}

static HRESULT WINAPI test_transform_AddInputStreams(IMFTransform *iface, DWORD streams, DWORD *ids)
{
    ok(0, "Unexpected %s call.\n", __func__);
    return E_NOTIMPL;
}

static HRESULT WINAPI test_transform_GetInputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);

    if (index >= transform->input_count)
    {
        *type = NULL;
        transform->input_enum_complete = TRUE;
        return MF_E_NO_MORE_TYPES;
    }

    *type = transform->input_types[index];
    IMFMediaType_AddRef(*type);
    return S_OK;
}

static HRESULT WINAPI test_transform_GetOutputAvailableType(IMFTransform *iface, DWORD id,
        DWORD index, IMFMediaType **type)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);

    if (index >= transform->output_count)
    {
        *type = NULL;
        transform->output_enum_complete = TRUE;
        return MF_E_NO_MORE_TYPES;
    }

    *type = transform->output_types[index];
    IMFMediaType_AddRef(*type);
    return S_OK;
}

static HRESULT WINAPI test_transform_SetInputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);
    BOOL result;
    UINT i;

    if (type)
    {
        for (i = 0; i < transform->input_count; ++i)
        {
            if (IMFMediaType_Compare(transform->input_types[i], (IMFAttributes *)type,
                    MF_ATTRIBUTES_MATCH_OUR_ITEMS, &result) == S_OK && result)
                break;
        }
        if (i == transform->input_count)
            return MF_E_INVALIDMEDIATYPE;
    }

    if (flags & MFT_SET_TYPE_TEST_ONLY)
        return S_OK;

    if (transform->input_type)
        IMFMediaType_Release(transform->input_type);

    if ((transform->input_type = type))
    {
        transform->input_type_set = TRUE;
        IMFMediaType_AddRef(transform->input_type);
    }

    if (transform->output_type && (!type || transform->set_input_nulls_output))
    {
        IMFMediaType_Release(transform->output_type);
        transform->output_type = NULL;
    }

    return S_OK;
}

static HRESULT WINAPI test_transform_SetOutputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);
    if (flags & MFT_SET_TYPE_TEST_ONLY)
        return S_OK;
    if (transform->output_type)
        IMFMediaType_Release(transform->output_type);
    if ((transform->output_type = type))
    {
        transform->output_type_set = TRUE;
        IMFMediaType_AddRef(transform->output_type);
    }
    return S_OK;
}

static HRESULT WINAPI test_transform_GetInputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);
    if (!(*type = transform->input_type))
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    IMFMediaType_AddRef(*type);
    return S_OK;
}

static HRESULT WINAPI test_transform_GetOutputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);
    if (!(*type = transform->output_type))
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    IMFMediaType_AddRef(*type);
    return S_OK;
}

static HRESULT WINAPI test_transform_GetInputStatus(IMFTransform *iface, DWORD id, DWORD *flags)
{
    ok(0, "Unexpected %s call.\n", __func__);
    return E_NOTIMPL;
}

static HRESULT WINAPI test_transform_GetOutputStatus(IMFTransform *iface, DWORD *flags)
{
    ok(0, "Unexpected %s call.\n", __func__);
    return E_NOTIMPL;
}

static HRESULT WINAPI test_transform_SetOutputBounds(IMFTransform *iface, LONGLONG lower, LONGLONG upper)
{
    ok(0, "Unexpected %s call.\n", __func__);
    return E_NOTIMPL;
}

static HRESULT WINAPI test_transform_ProcessEvent(IMFTransform *iface, DWORD id, IMFMediaEvent *event)
{
    ok(0, "Unexpected %s call.\n", __func__);
    return E_NOTIMPL;
}

static void test_transform_set_expect_d3d9_device_manager(IMFTransform *iface, IDirect3DDeviceManager9 *manager)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);
    transform->expect_d3d9_device_manager = manager;
}

static void test_transform_set_expect_dxgi_device_manager(IMFTransform *iface, IMFDXGIDeviceManager *manager)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);
    transform->expect_dxgi_device_manager = manager;
}

static HRESULT WINAPI test_transform_ProcessMessage(IMFTransform *iface, MFT_MESSAGE_TYPE message, ULONG_PTR param)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);
    switch (message)
    {
    case MFT_MESSAGE_SET_D3D_MANAGER:
        ok(!!param, "Unexpected param.\n");
        ok(transform->expect_d3d9_device_manager || transform->expect_dxgi_device_manager, "Unexpected %s call.\n", __func__);

        if (transform->expect_d3d9_device_manager)
        {
            IDirect3DDeviceManager9 *manager;
            HRESULT hr;

            check_interface((IUnknown *)param, &IID_IMFDXGIDeviceManager, FALSE);
            hr = IUnknown_QueryInterface((IUnknown *)param, &IID_IDirect3DDeviceManager9, (void **)&manager);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(manager == transform->expect_d3d9_device_manager, "got manager %p\n", manager);
            IDirect3DDeviceManager9_Release(manager);

            transform->got_d3d9_device_manager = TRUE;
        }
        if (transform->expect_dxgi_device_manager)
        {
            IMFDXGIDeviceManager *manager;
            HRESULT hr;

            check_interface((IUnknown *)param, &IID_IDirect3DDeviceManager9, FALSE);
            hr = IUnknown_QueryInterface((IUnknown *)param, &IID_IMFDXGIDeviceManager, (void **)&manager);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(manager == transform->expect_dxgi_device_manager, "got manager %p\n", manager);
            if (manager) IMFDXGIDeviceManager_Release(manager);

            transform->got_dxgi_device_manager = TRUE;
        }
        return S_OK;

    default:
        ok(0, "Unexpected %s call.\n", __func__);
        return E_NOTIMPL;
    }
}

static BOOL test_transform_got_d3d9_device_manager(IMFTransform *iface)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);
    BOOL ret = transform->got_d3d9_device_manager;
    transform->got_d3d9_device_manager = FALSE;
    return ret;
}

static BOOL test_transform_got_dxgi_device_manager(IMFTransform *iface)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);
    BOOL ret = transform->got_dxgi_device_manager;
    transform->got_dxgi_device_manager = FALSE;
    return ret;
}

static HRESULT WINAPI test_transform_ProcessInput(IMFTransform *iface, DWORD id, IMFSample *sample, DWORD flags)
{
    ok(0, "Unexpected %s call.\n", __func__);
    return E_NOTIMPL;
}

static HRESULT WINAPI test_transform_ProcessOutput(IMFTransform *iface, DWORD flags, DWORD count,
        MFT_OUTPUT_DATA_BUFFER *data, DWORD *status)
{
    ok(0, "Unexpected %s call.\n", __func__);
    return E_NOTIMPL;
}

static const IMFTransformVtbl test_transform_vtbl =
{
    test_transform_QueryInterface,
    test_transform_AddRef,
    test_transform_Release,
    test_transform_GetStreamLimits,
    test_transform_GetStreamCount,
    test_transform_GetStreamIDs,
    test_transform_GetInputStreamInfo,
    test_transform_GetOutputStreamInfo,
    test_transform_GetAttributes,
    test_transform_GetInputStreamAttributes,
    test_transform_GetOutputStreamAttributes,
    test_transform_DeleteInputStream,
    test_transform_AddInputStreams,
    test_transform_GetInputAvailableType,
    test_transform_GetOutputAvailableType,
    test_transform_SetInputType,
    test_transform_SetOutputType,
    test_transform_GetInputCurrentType,
    test_transform_GetOutputCurrentType,
    test_transform_GetInputStatus,
    test_transform_GetOutputStatus,
    test_transform_SetOutputBounds,
    test_transform_ProcessEvent,
    test_transform_ProcessMessage,
    test_transform_ProcessInput,
    test_transform_ProcessOutput,
};

static HRESULT WINAPI test_transform_create(UINT input_count, IMFMediaType **input_types,
        UINT output_count, IMFMediaType **output_types, BOOL d3d_aware, IMFTransform **out)
{
    struct test_transform *transform;
    HRESULT hr;
    GUID type;

    if (!(transform = calloc(1, sizeof(*transform))))
        return E_OUTOFMEMORY;
    transform->IMFTransform_iface.lpVtbl = &test_transform_vtbl;
    transform->refcount = 1;

    transform->input_count = input_count;
    transform->input_types = input_types;
    transform->input_type = input_types[0];
    IMFMediaType_AddRef(transform->input_type);
    transform->output_count = output_count;
    transform->output_types = output_types;
    transform->output_type = output_types[0];
    IMFMediaType_AddRef(transform->output_type);

    transform->set_input_nulls_output = SUCCEEDED(IMFMediaType_GetGUID(output_types[0], &MF_MT_MAJOR_TYPE, &type))
            && !IsEqualGUID(&type, &MFMediaType_Audio);

    hr = MFCreateAttributes(&transform->attributes, 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_SetUINT32(transform->attributes, &MF_SA_D3D_AWARE, d3d_aware);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_SetUINT32(transform->attributes, &MF_SA_D3D11_AWARE, d3d_aware);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    *out = &transform->IMFTransform_iface;
    return S_OK;
}

DEFINE_EXPECT(test_source_BeginGetEvent);
DEFINE_EXPECT(test_source_QueueEvent);
DEFINE_EXPECT(test_source_Start);

struct test_source
{
    IMFMediaSource IMFMediaSource_iface;
    LONG refcount;
    HRESULT begin_get_event_res;
    IMFPresentationDescriptor *pd;
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
        IMFPresentationDescriptor_Release(source->pd);
        free(source);
    }

    return refcount;
}

static HRESULT WINAPI test_source_GetEvent(IMFMediaSource *iface, DWORD flags, IMFMediaEvent **event)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_source_BeginGetEvent(IMFMediaSource *iface, IMFAsyncCallback *callback, IUnknown *state)
{
    struct test_source *source = impl_from_IMFMediaSource(iface);
    CHECK_EXPECT(test_source_BeginGetEvent);
    return source->begin_get_event_res;
}

static HRESULT WINAPI test_source_EndGetEvent(IMFMediaSource *iface, IMFAsyncResult *result, IMFMediaEvent **event)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_source_QueueEvent(IMFMediaSource *iface, MediaEventType event_type, REFGUID ext_type,
        HRESULT hr, const PROPVARIANT *value)
{
    CHECK_EXPECT(test_source_QueueEvent);
    return E_NOTIMPL;
}

static HRESULT WINAPI test_source_GetCharacteristics(IMFMediaSource *iface, DWORD *flags)
{
    *flags = 0;
    return S_OK;
}

static HRESULT WINAPI test_source_CreatePresentationDescriptor(IMFMediaSource *iface, IMFPresentationDescriptor **pd)
{
    struct test_source *source = impl_from_IMFMediaSource(iface);
    return IMFPresentationDescriptor_Clone(source->pd, pd);
}

static HRESULT WINAPI test_source_Start(IMFMediaSource *iface, IMFPresentationDescriptor *pd, const GUID *time_format,
        const PROPVARIANT *start_position)
{
    CHECK_EXPECT(test_source_Start);
    return E_NOTIMPL;
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
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
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

static IMFMediaSource *create_test_source(IMFPresentationDescriptor *pd)
{
    struct test_source *source;

    source = calloc(1, sizeof(*source));
    source->IMFMediaSource_iface.lpVtbl = &test_source_vtbl;
    source->refcount = 1;
    source->begin_get_event_res = E_NOTIMPL;
    IMFPresentationDescriptor_AddRef((source->pd = pd));

    return &source->IMFMediaSource_iface;
}

static HRESULT WINAPI test_unk_QueryInterface(IUnknown *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_unk_AddRef(IUnknown *iface)
{
    return 2;
}

static ULONG WINAPI test_unk_Release(IUnknown *iface)
{
    return 1;
}

static const IUnknownVtbl test_unk_vtbl =
{
    test_unk_QueryInterface,
    test_unk_AddRef,
    test_unk_Release,
};

static void test_topology(void)
{
    IMFMediaType *mediatype, *mediatype2, *mediatype3;
    IMFCollection *collection, *collection2;
    IUnknown test_unk2 = { &test_unk_vtbl };
    IUnknown test_unk = { &test_unk_vtbl };
    IMFTopologyNode *node, *node2, *node3;
    IMFTopology *topology, *topology2;
    DWORD size, io_count, index;
    MF_TOPOLOGY_TYPE node_type;
    IUnknown *object;
    WORD node_count;
    UINT32 count;
    HRESULT hr;
    TOPOID id;
    LONG ref;

    hr = MFCreateTopology(NULL);
    ok(hr == E_POINTER, "got %#lx\n", hr);

    hr = MFCreateTopology(&topology);
    ok(hr == S_OK, "Failed to create topology, hr %#lx.\n", hr);
    hr = IMFTopology_GetTopologyID(topology, &id);
    ok(hr == S_OK, "Failed to get id, hr %#lx.\n", hr);
    ok(id == 1, "Unexpected id.\n");

    hr = MFCreateTopology(&topology2);
    ok(hr == S_OK, "Failed to create topology, hr %#lx.\n", hr);
    hr = IMFTopology_GetTopologyID(topology2, &id);
    ok(hr == S_OK, "Failed to get id, hr %#lx.\n", hr);
    ok(id == 2, "Unexpected id.\n");

    ref = IMFTopology_Release(topology);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = MFCreateTopology(&topology);
    ok(hr == S_OK, "Failed to create topology, hr %#lx.\n", hr);
    hr = IMFTopology_GetTopologyID(topology, &id);
    ok(hr == S_OK, "Failed to get id, hr %#lx.\n", hr);
    ok(id == 3, "Unexpected id.\n");

    ref = IMFTopology_Release(topology2);
    ok(ref == 0, "Release returned %ld\n", ref);

    /* No attributes by default. */
    for (node_type = MF_TOPOLOGY_OUTPUT_NODE; node_type < MF_TOPOLOGY_TEE_NODE; ++node_type)
    {
        hr = MFCreateTopologyNode(node_type, &node);
        ok(hr == S_OK, "Failed to create a node for type %d, hr %#lx.\n", node_type, hr);
        hr = IMFTopologyNode_GetCount(node, &count);
        ok(hr == S_OK, "Failed to get attribute count, hr %#lx.\n", hr);
        ok(!count, "Unexpected attribute count %u.\n", count);
        ref = IMFTopologyNode_Release(node);
        ok(ref == 0, "Release returned %ld\n", ref);
    }

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &node);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_TEE_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetTopoNodeID(node, &id);
    ok(hr == S_OK, "Failed to get node id, hr %#lx.\n", hr);
    ok(((id >> 32) == GetCurrentProcessId()) && !!(id & 0xffff), "Unexpected node id %s.\n", wine_dbgstr_longlong(id));

    hr = IMFTopologyNode_SetTopoNodeID(node2, id);
    ok(hr == S_OK, "Failed to set node id, hr %#lx.\n", hr);

    hr = IMFTopology_GetNodeCount(topology, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopology_AddNode(topology, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    node_count = 1;
    hr = IMFTopology_GetNodeCount(topology, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#lx.\n", hr);
    ok(node_count == 0, "Unexpected node count %u.\n", node_count);

    /* Same id, different nodes. */
    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);

    node_count = 0;
    hr = IMFTopology_GetNodeCount(topology, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#lx.\n", hr);
    ok(node_count == 1, "Unexpected node count %u.\n", node_count);

    hr = IMFTopology_AddNode(topology, node2);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ref = IMFTopologyNode_Release(node2);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = IMFTopology_GetNodeByID(topology, id, &node2);
    ok(hr == S_OK, "Failed to get a node, hr %#lx.\n", hr);
    ok(node2 == node, "Unexpected node.\n");
    IMFTopologyNode_Release(node2);

    /* Change node id, add it again. */
    hr = IMFTopologyNode_SetTopoNodeID(node, ++id);
    ok(hr == S_OK, "Failed to set node id, hr %#lx.\n", hr);

    hr = IMFTopology_GetNodeByID(topology, id, &node2);
    ok(hr == S_OK, "Failed to get a node, hr %#lx.\n", hr);
    ok(node2 == node, "Unexpected node.\n");
    IMFTopologyNode_Release(node2);

    hr = IMFTopology_GetNodeByID(topology, id + 1, &node2);
    ok(hr == MF_E_NOT_FOUND, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopology_AddNode(topology, node);
    ok(hr == E_INVALIDARG, "Failed to add a node, hr %#lx.\n", hr);

    hr = IMFTopology_GetNode(topology, 0, &node2);
    ok(hr == S_OK, "Failed to get a node, hr %#lx.\n", hr);
    ok(node2 == node, "Unexpected node.\n");
    IMFTopologyNode_Release(node2);

    hr = IMFTopology_GetNode(topology, 1, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopology_GetNode(topology, 1, &node2);
    ok(hr == MF_E_INVALIDINDEX, "Failed to get a node, hr %#lx.\n", hr);

    hr = IMFTopology_GetNode(topology, -2, &node2);
    ok(hr == MF_E_INVALIDINDEX, "Failed to get a node, hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_TEE_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);
    hr = IMFTopology_AddNode(topology, node2);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);
    ref = IMFTopologyNode_Release(node2);
    ok(ref == 1, "Release returned %ld\n", ref);

    node_count = 0;
    hr = IMFTopology_GetNodeCount(topology, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#lx.\n", hr);
    ok(node_count == 2, "Unexpected node count %u.\n", node_count);

    /* Remove with detached node, existing id. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_TEE_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);
    hr = IMFTopologyNode_SetTopoNodeID(node2, id);
    ok(hr == S_OK, "Failed to set node id, hr %#lx.\n", hr);
    hr = IMFTopology_RemoveNode(topology, node2);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ref = IMFTopologyNode_Release(node2);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = IMFTopology_RemoveNode(topology, node);
    ok(hr == S_OK, "Failed to remove a node, hr %#lx.\n", hr);

    node_count = 0;
    hr = IMFTopology_GetNodeCount(topology, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#lx.\n", hr);
    ok(node_count == 1, "Unexpected node count %u.\n", node_count);

    hr = IMFTopology_Clear(topology);
    ok(hr == S_OK, "Failed to clear topology, hr %#lx.\n", hr);

    node_count = 1;
    hr = IMFTopology_GetNodeCount(topology, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#lx.\n", hr);
    ok(node_count == 0, "Unexpected node count %u.\n", node_count);

    hr = IMFTopology_Clear(topology);
    ok(hr == S_OK, "Failed to clear topology, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetTopoNodeID(node, 123);
    ok(hr == S_OK, "Failed to set node id, hr %#lx.\n", hr);

    ref = IMFTopologyNode_Release(node);
    ok(ref == 0, "Release returned %ld\n", ref);

    /* Change id for attached node. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &node);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_TEE_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);

    hr = IMFTopology_AddNode(topology, node2);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetTopoNodeID(node, &id);
    ok(hr == S_OK, "Failed to get node id, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetTopoNodeID(node2, id);
    ok(hr == S_OK, "Failed to get node id, hr %#lx.\n", hr);

    hr = IMFTopology_GetNodeByID(topology, id, &node3);
    ok(hr == S_OK, "Failed to get a node, hr %#lx.\n", hr);
    ok(node3 == node, "Unexpected node.\n");
    IMFTopologyNode_Release(node3);

    /* Source/output collections. */
    hr = IMFTopology_Clear(topology);
    ok(hr == S_OK, "Failed to clear topology, hr %#lx.\n", hr);

    ref = IMFTopologyNode_Release(node);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopologyNode_Release(node2);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = IMFTopology_GetSourceNodeCollection(topology, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopology_GetSourceNodeCollection(topology, &collection);
    ok(hr == S_OK, "Failed to get source node collection, hr %#lx.\n", hr);
    ok(!!collection, "Unexpected object pointer.\n");

    hr = IMFTopology_GetSourceNodeCollection(topology, &collection2);
    ok(hr == S_OK, "Failed to get source node collection, hr %#lx.\n", hr);
    ok(!!collection2, "Unexpected object pointer.\n");
    ok(collection2 != collection, "Expected cloned collection.\n");

    hr = IMFCollection_GetElementCount(collection, &size);
    ok(hr == S_OK, "Failed to get item count, hr %#lx.\n", hr);
    ok(!size, "Unexpected item count.\n");

    EXPECT_REF(collection, 1);
    hr = IMFCollection_AddElement(collection, (IUnknown *)collection);
    ok(hr == S_OK, "Failed to add element, hr %#lx.\n", hr);
    EXPECT_REF(collection, 2);

    hr = IMFCollection_GetElementCount(collection, &size);
    ok(hr == S_OK, "Failed to get item count, hr %#lx.\n", hr);
    ok(size == 1, "Unexpected item count.\n");

    /* Empty collection to stop referencing itself */
    hr = IMFCollection_RemoveAllElements(collection);
    ok(hr == S_OK, "Failed to get item count, hr %#lx.\n", hr);

    hr = IMFCollection_GetElementCount(collection2, &size);
    ok(hr == S_OK, "Failed to get item count, hr %#lx.\n", hr);
    ok(!size, "Unexpected item count.\n");

    ref = IMFCollection_Release(collection2);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFCollection_Release(collection);
    ok(ref == 0, "Release returned %ld\n", ref);

    /* Add some nodes. */
    hr = IMFTopology_GetSourceNodeCollection(topology, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopology_GetOutputNodeCollection(topology, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node);
    ok(hr == S_OK, "Failed to create a node, hr %#lx.\n", hr);
    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);
    IMFTopologyNode_Release(node);

    hr = IMFTopology_GetSourceNodeCollection(topology, &collection);
    ok(hr == S_OK, "Failed to get source node collection, hr %#lx.\n", hr);
    ok(!!collection, "Unexpected object pointer.\n");
    hr = IMFCollection_GetElementCount(collection, &size);
    ok(hr == S_OK, "Failed to get item count, hr %#lx.\n", hr);
    ok(size == 1, "Unexpected item count.\n");
    ref = IMFCollection_Release(collection);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_TEE_NODE, &node);
    ok(hr == S_OK, "Failed to create a node, hr %#lx.\n", hr);
    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);
    IMFTopologyNode_Release(node);

    hr = IMFTopology_GetSourceNodeCollection(topology, &collection);
    ok(hr == S_OK, "Failed to get source node collection, hr %#lx.\n", hr);
    ok(!!collection, "Unexpected object pointer.\n");
    hr = IMFCollection_GetElementCount(collection, &size);
    ok(hr == S_OK, "Failed to get item count, hr %#lx.\n", hr);
    ok(size == 1, "Unexpected item count.\n");
    ref = IMFCollection_Release(collection);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE, &node);
    ok(hr == S_OK, "Failed to create a node, hr %#lx.\n", hr);
    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);
    IMFTopologyNode_Release(node);

    hr = IMFTopology_GetSourceNodeCollection(topology, &collection);
    ok(hr == S_OK, "Failed to get source node collection, hr %#lx.\n", hr);
    ok(!!collection, "Unexpected object pointer.\n");
    hr = IMFCollection_GetElementCount(collection, &size);
    ok(hr == S_OK, "Failed to get item count, hr %#lx.\n", hr);
    ok(size == 1, "Unexpected item count.\n");
    ref = IMFCollection_Release(collection);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &node);
    ok(hr == S_OK, "Failed to create a node, hr %#lx.\n", hr);
    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);

    /* Associated object. */
    hr = IMFTopologyNode_SetObject(node, NULL);
    ok(hr == S_OK, "Failed to set object, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetObject(node, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    object = (void *)0xdeadbeef;
    hr = IMFTopologyNode_GetObject(node, &object);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(!object, "Unexpected object %p.\n", object);

    hr = IMFTopologyNode_SetObject(node, &test_unk);
    ok(hr == S_OK, "Failed to set object, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetObject(node, &object);
    ok(hr == S_OK, "Failed to get object, hr %#lx.\n", hr);
    ok(object == &test_unk, "Unexpected object %p.\n", object);
    IUnknown_Release(object);

    hr = IMFTopologyNode_SetObject(node, &test_unk2);
    ok(hr == S_OK, "Failed to set object, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetCount(node, &count);
    ok(hr == S_OK, "Failed to get attribute count, hr %#lx.\n", hr);
    ok(count == 0, "Unexpected attribute count %u.\n", count);

    hr = IMFTopologyNode_SetGUID(node, &MF_TOPONODE_TRANSFORM_OBJECTID, &MF_TOPONODE_TRANSFORM_OBJECTID);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetObject(node, NULL);
    ok(hr == S_OK, "Failed to set object, hr %#lx.\n", hr);

    object = (void *)0xdeadbeef;
    hr = IMFTopologyNode_GetObject(node, &object);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(!object, "Unexpected object %p.\n", object);

    hr = IMFTopologyNode_GetCount(node, &count);
    ok(hr == S_OK, "Failed to get attribute count, hr %#lx.\n", hr);
    ok(count == 1, "Unexpected attribute count %u.\n", count);

    /* Preferred stream types. */
    hr = IMFTopologyNode_GetInputCount(node, &io_count);
    ok(hr == S_OK, "Failed to get input count, hr %#lx.\n", hr);
    ok(io_count == 0, "Unexpected count %lu.\n", io_count);

    hr = IMFTopologyNode_GetInputPrefType(node, 0, &mediatype);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetInputPrefType(node, 0, mediatype);
    ok(hr == S_OK, "Failed to set preferred type, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetInputPrefType(node, 0, &mediatype2);
    ok(hr == S_OK, "Failed to get preferred type, hr %#lx.\n", hr);
    ok(mediatype2 == mediatype, "Unexpected mediatype instance.\n");
    IMFMediaType_Release(mediatype2);

    hr = IMFTopologyNode_SetInputPrefType(node, 0, NULL);
    ok(hr == S_OK, "Failed to set preferred type, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetInputPrefType(node, 0, &mediatype2);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(!mediatype2, "Unexpected mediatype instance.\n");

    hr = IMFTopologyNode_SetInputPrefType(node, 1, mediatype);
    ok(hr == S_OK, "Failed to set preferred type, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetInputPrefType(node, 1, mediatype);
    ok(hr == S_OK, "Failed to set preferred type, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetInputCount(node, &io_count);
    ok(hr == S_OK, "Failed to get input count, hr %#lx.\n", hr);
    ok(io_count == 2, "Unexpected count %lu.\n", io_count);

    hr = IMFTopologyNode_GetOutputCount(node, &io_count);
    ok(hr == S_OK, "Failed to get input count, hr %#lx.\n", hr);
    ok(io_count == 0, "Unexpected count %lu.\n", io_count);

    hr = IMFTopologyNode_SetOutputPrefType(node, 0, mediatype);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    ref = IMFTopologyNode_Release(node);
    ok(ref == 1, "Release returned %ld\n", ref);

    /* Source node. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node);
    ok(hr == S_OK, "Failed to create a node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetInputPrefType(node, 0, mediatype);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetOutputPrefType(node, 2, mediatype);
    ok(hr == S_OK, "Failed to set preferred type, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetOutputPrefType(node, 0, &mediatype2);
    ok(hr == E_FAIL, "Failed to get preferred type, hr %#lx.\n", hr);
    ok(!mediatype2, "Unexpected mediatype instance.\n");

    hr = IMFTopologyNode_GetOutputCount(node, &io_count);
    ok(hr == S_OK, "Failed to get output count, hr %#lx.\n", hr);
    ok(io_count == 3, "Unexpected count %lu.\n", io_count);

    ref = IMFTopologyNode_Release(node);
    ok(ref == 0, "Release returned %ld\n", ref);

    /* Tee node. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_TEE_NODE, &node);
    ok(hr == S_OK, "Failed to create a node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetInputPrefType(node, 0, mediatype);
    ok(hr == S_OK, "Failed to set preferred type, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetInputPrefType(node, 0, &mediatype2);
    ok(hr == S_OK, "Failed to get preferred type, hr %#lx.\n", hr);
    ok(mediatype2 == mediatype, "Unexpected mediatype instance.\n");
    IMFMediaType_Release(mediatype2);

    hr = IMFTopologyNode_GetOutputPrefType(node, 0, &mediatype2);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetInputCount(node, &io_count);
    ok(hr == S_OK, "Failed to get output count, hr %#lx.\n", hr);
    ok(io_count == 0, "Unexpected count %lu.\n", io_count);

    hr = IMFTopologyNode_SetInputPrefType(node, 1, mediatype);
    ok(hr == MF_E_INVALIDTYPE, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetInputPrefType(node, 3, mediatype);
    ok(hr == MF_E_INVALIDTYPE, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetOutputPrefType(node, 4, mediatype);
    ok(hr == S_OK, "Failed to set preferred type, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetOutputPrefType(node, 0, &mediatype2);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMediaType(&mediatype2);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    /* Changing output type does not change input type. */
    hr = IMFTopologyNode_SetOutputPrefType(node, 4, mediatype2);
    ok(hr == S_OK, "Failed to set preferred type, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetInputPrefType(node, 0, &mediatype3);
    ok(hr == S_OK, "Failed to get preferred type, hr %#lx.\n", hr);
    ok(mediatype3 == mediatype, "Unexpected mediatype instance.\n");
    IMFMediaType_Release(mediatype3);

    IMFMediaType_Release(mediatype2);

    hr = IMFTopologyNode_GetInputCount(node, &io_count);
    ok(hr == S_OK, "Failed to get output count, hr %#lx.\n", hr);
    ok(io_count == 0, "Unexpected count %lu.\n", io_count);

    hr = IMFTopologyNode_GetOutputCount(node, &io_count);
    ok(hr == S_OK, "Failed to get output count, hr %#lx.\n", hr);
    ok(io_count == 5, "Unexpected count %lu.\n", io_count);

    ref = IMFTopologyNode_Release(node);
    ok(ref == 0, "Release returned %ld\n", ref);

    /* Transform node. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE, &node);
    ok(hr == S_OK, "Failed to create a node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetInputPrefType(node, 3, mediatype);
    ok(hr == S_OK, "Failed to set preferred type, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetInputCount(node, &io_count);
    ok(hr == S_OK, "Failed to get input count, hr %#lx.\n", hr);
    ok(io_count == 4, "Unexpected count %lu.\n", io_count);

    hr = IMFTopologyNode_SetOutputPrefType(node, 4, mediatype);
    ok(hr == S_OK, "Failed to set preferred type, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetInputCount(node, &io_count);
    ok(hr == S_OK, "Failed to get output count, hr %#lx.\n", hr);
    ok(io_count == 4, "Unexpected count %lu.\n", io_count);

    hr = IMFTopologyNode_GetOutputCount(node, &io_count);
    ok(hr == S_OK, "Failed to get output count, hr %#lx.\n", hr);
    ok(io_count == 5, "Unexpected count %lu.\n", io_count);

    ref = IMFTopologyNode_Release(node);
    ok(ref == 0, "Release returned %ld\n", ref);

    IMFMediaType_Release(mediatype);

    hr = IMFTopology_GetOutputNodeCollection(topology, &collection);
    ok(hr == S_OK || broken(hr == E_FAIL) /* before Win8 */, "Failed to get output node collection, hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
    {
        ok(!!collection, "Unexpected object pointer.\n");
        hr = IMFCollection_GetElementCount(collection, &size);
        ok(hr == S_OK, "Failed to get item count, hr %#lx.\n", hr);
        ok(size == 1, "Unexpected item count.\n");
        ref = IMFCollection_Release(collection);
        ok(ref == 0, "Release returned %ld\n", ref);
    }

    ref = IMFTopology_Release(topology);
    ok(ref == 0, "Release returned %ld\n", ref);

    /* Connect nodes. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    EXPECT_REF(node, 1);
    EXPECT_REF(node2, 1);

    hr = IMFTopologyNode_ConnectOutput(node, 0, node2, 1);
    ok(hr == S_OK, "Failed to connect nodes, hr %#lx.\n", hr);

    EXPECT_REF(node, 2);
    EXPECT_REF(node2, 2);

    IMFTopologyNode_Release(node);

    EXPECT_REF(node, 1);
    EXPECT_REF(node2, 2);

    IMFTopologyNode_Release(node2);

    EXPECT_REF(node, 1);
    EXPECT_REF(node2, 1);

    hr = IMFTopologyNode_GetNodeType(node2, &node_type);
    ok(hr == S_OK, "Failed to get node type, hr %#lx.\n", hr);

    IMFTopologyNode_Release(node);

    /* Connect within topology. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = MFCreateTopology(&topology);
    ok(hr == S_OK, "Failed to create topology, hr %#lx.\n", hr);

    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);

    hr = IMFTopology_AddNode(topology, node2);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);

    EXPECT_REF(node, 2);
    EXPECT_REF(node2, 2);

    hr = IMFTopologyNode_ConnectOutput(node, 0, node2, 1);
    ok(hr == S_OK, "Failed to connect nodes, hr %#lx.\n", hr);

    EXPECT_REF(node, 3);
    EXPECT_REF(node2, 3);

    hr = IMFTopology_Clear(topology);
    ok(hr == S_OK, "Failed to clear topology, hr %#lx.\n", hr);

    EXPECT_REF(node, 1);
    EXPECT_REF(node2, 1);

    /* Removing connected node breaks connection. */
    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);

    hr = IMFTopology_AddNode(topology, node2);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_ConnectOutput(node, 0, node2, 1);
    ok(hr == S_OK, "Failed to connect nodes, hr %#lx.\n", hr);

    hr = IMFTopology_RemoveNode(topology, node);
    ok(hr == S_OK, "Failed to remove a node, hr %#lx.\n", hr);

    EXPECT_REF(node, 1);
    EXPECT_REF(node2, 2);

    hr = IMFTopologyNode_GetOutput(node, 0, &node3, &index);
    ok(hr == MF_E_NOT_FOUND, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_ConnectOutput(node, 0, node2, 1);
    ok(hr == S_OK, "Failed to connect nodes, hr %#lx.\n", hr);

    hr = IMFTopology_RemoveNode(topology, node2);
    ok(hr == S_OK, "Failed to remove a node, hr %#lx.\n", hr);

    EXPECT_REF(node, 2);
    EXPECT_REF(node2, 1);

    IMFTopologyNode_Release(node);
    IMFTopologyNode_Release(node2);

    /* Cloning nodes of different types. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_CloneFrom(node, node2);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    ref = IMFTopologyNode_Release(node2);
    ok(ref == 0, "Release returned %ld\n", ref);

    /* Cloning preferred types. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetOutputPrefType(node2, 0, mediatype);
    ok(hr == S_OK, "Failed to set preferred type, hr %#lx.\n", hr);

    /* Vista checks for additional attributes. */
    hr = IMFTopologyNode_CloneFrom(node, node2);
    ok(hr == S_OK || broken(hr == MF_E_ATTRIBUTENOTFOUND) /* Vista */, "Failed to clone a node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetOutputPrefType(node, 0, &mediatype2);
    ok(hr == S_OK, "Failed to get preferred type, hr %#lx.\n", hr);
    ok(mediatype == mediatype2, "Unexpected media type.\n");

    IMFMediaType_Release(mediatype2);

    ref = IMFTopologyNode_Release(node2);
    ok(ref == 0, "Release returned %ld\n", ref);

    IMFMediaType_Release(mediatype);

    /* Existing preferred types are not cleared. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetOutputCount(node, &io_count);
    ok(hr == S_OK, "Failed to get output count, hr %#lx.\n", hr);
    ok(io_count == 1, "Unexpected output count.\n");

    hr = IMFTopologyNode_CloneFrom(node, node2);
    ok(hr == S_OK || broken(hr == MF_E_ATTRIBUTENOTFOUND) /* Vista */, "Failed to clone a node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetOutputCount(node, &io_count);
    ok(hr == S_OK, "Failed to get output count, hr %#lx.\n", hr);
    ok(io_count == 1, "Unexpected output count.\n");

    hr = IMFTopologyNode_GetOutputPrefType(node, 0, &mediatype2);
    ok(hr == S_OK, "Failed to get preferred type, hr %#lx.\n", hr);
    ok(!!mediatype2, "Unexpected media type.\n");
    IMFMediaType_Release(mediatype2);

    hr = IMFTopologyNode_CloneFrom(node2, node);
    ok(hr == S_OK || broken(hr == MF_E_ATTRIBUTENOTFOUND) /* Vista */, "Failed to clone a node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetOutputCount(node2, &io_count);
    ok(hr == S_OK, "Failed to get output count, hr %#lx.\n", hr);
    ok(io_count == 1, "Unexpected output count.\n");

    ref = IMFTopologyNode_Release(node2);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopologyNode_Release(node);
    ok(ref == 0, "Release returned %ld\n", ref);

    /* Add one node, connect to another that hasn't been added. */
    hr = IMFTopology_Clear(topology);
    ok(hr == S_OK, "Failed to clear topology, hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);

    hr = IMFTopology_GetNodeCount(topology, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#lx.\n", hr);
    ok(node_count == 1, "Unexpected node count.\n");

    hr = IMFTopologyNode_ConnectOutput(node, 0, node2, 0);
    ok(hr == S_OK, "Failed to connect nodes, hr %#lx.\n", hr);

    hr = IMFTopology_GetNodeCount(topology, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#lx.\n", hr);
    ok(node_count == 1, "Unexpected node count.\n");

    /* Add same node to different topologies. */
    hr = IMFTopology_Clear(topology);
    ok(hr == S_OK, "Failed to clear topology, hr %#lx.\n", hr);

    hr = MFCreateTopology(&topology2);
    ok(hr == S_OK, "Failed to create topology, hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);
    EXPECT_REF(node, 2);

    hr = IMFTopology_GetNodeCount(topology, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#lx.\n", hr);
    ok(node_count == 1, "Unexpected node count.\n");

    hr = IMFTopology_GetNodeCount(topology2, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#lx.\n", hr);
    ok(node_count == 0, "Unexpected node count.\n");

    hr = IMFTopology_AddNode(topology2, node);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);
    EXPECT_REF(node, 3);

    hr = IMFTopology_GetNodeCount(topology, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#lx.\n", hr);
    ok(node_count == 1, "Unexpected node count.\n");

    hr = IMFTopology_GetNodeCount(topology2, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#lx.\n", hr);
    ok(node_count == 1, "Unexpected node count.\n");

    ref = IMFTopology_Release(topology2);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopology_Release(topology);
    ok(ref == 0, "Release returned %ld\n", ref);

    ref = IMFTopologyNode_Release(node);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopologyNode_Release(node2);
    ok(ref == 0, "Release returned %ld\n", ref);

    /* Try cloning a topology without all outputs connected */
    hr = MFCreateTopology(&topology);
    ok(hr == S_OK, "Failed to create topology, hr %#lx.\n", hr);

    hr = MFCreateTopology(&topology2);
    ok(hr == S_OK, "Failed to create topology, hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE, &node);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);
    EXPECT_REF(node, 2);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &node2);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = IMFTopology_AddNode(topology, node2);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);
    EXPECT_REF(node, 2);

    hr = IMFTopologyNode_ConnectOutput(node, 1, node2, 0);
    ok(hr == S_OK, "Failed to connect output, hr %#lx.\n", hr);

    hr = IMFTopology_CloneFrom(topology2, topology);
    ok(hr == S_OK, "Failed to clone from topology, hr %#lx.\n", hr);

    hr = IMFTopology_GetNodeCount(topology2, &node_count);
    ok(hr == S_OK, "Failed to get node count, hr %#lx.\n", hr);
    ok(node_count == 2, "Unexpected node count %u.\n", node_count);

    hr = IMFTopology_GetNode(topology2, 0, &node3);
    ok(hr == S_OK, "Failed to get node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetOutputCount(node3, &size);
    ok(hr == S_OK, "Failed to get output count, hr %#lx.\n", hr);
    ok(size == 2, "Unexpected output count %lu.\n", size);

    IMFTopologyNode_Release(node3);

    ref = IMFTopology_Release(topology2);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopology_Release(topology);
    ok(ref == 0, "Release returned %ld\n", ref);

    ref = IMFTopologyNode_Release(node2);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopologyNode_Release(node);
    ok(ref == 0, "Release returned %ld\n", ref);
}

static void test_topology_tee_node(void)
{
    IMFTopologyNode *src_node, *tee_node;
    IMFMediaType *mediatype, *mediatype2;
    IMFTopology *topology;
    DWORD count;
    HRESULT hr;
    LONG ref;

    hr = MFCreateTopology(&topology);
    ok(hr == S_OK, "Failed to create topology, hr %#lx.\n", hr);

    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_TEE_NODE, &tee_node);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &src_node);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetInputPrefType(tee_node, 0, mediatype);
    ok(hr == S_OK, "Failed to set type, hr %#lx.\n", hr);

    /* Even though tee node has only one input and source has only one output,
       it's possible to connect to higher inputs/outputs. */

    /* SRC(0) -> TEE(0) */
    hr = IMFTopologyNode_ConnectOutput(src_node, 0, tee_node, 0);
    ok(hr == S_OK, "Failed to connect nodes, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetInputCount(tee_node, &count);
    ok(hr == S_OK, "Failed to get count, hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %lu.\n", count);

    hr = IMFTopologyNode_GetInputPrefType(tee_node, 0, &mediatype2);
    ok(hr == S_OK, "Failed to get type, hr %#lx.\n", hr);
    ok(mediatype2 == mediatype, "Unexpected type.\n");
    IMFMediaType_Release(mediatype2);

    /* SRC(0) -> TEE(1) */
    hr = IMFTopologyNode_ConnectOutput(src_node, 0, tee_node, 1);
    ok(hr == S_OK, "Failed to connect nodes, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetInputCount(tee_node, &count);
    ok(hr == S_OK, "Failed to get count, hr %#lx.\n", hr);
    ok(count == 2, "Unexpected count %lu.\n", count);

    hr = IMFTopologyNode_SetInputPrefType(tee_node, 1, mediatype);
    ok(hr == MF_E_INVALIDTYPE, "Unexpected hr %#lx.\n", hr);

    /* SRC(1) -> TEE(1) */
    hr = IMFTopologyNode_ConnectOutput(src_node, 1, tee_node, 1);
    ok(hr == S_OK, "Failed to connect nodes, hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetOutputCount(src_node, &count);
    ok(hr == S_OK, "Failed to get count, hr %#lx.\n", hr);
    ok(count == 2, "Unexpected count %lu.\n", count);

    EXPECT_REF(src_node, 2);
    EXPECT_REF(tee_node, 2);
    hr = IMFTopologyNode_DisconnectOutput(src_node, 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ref = IMFTopologyNode_Release(src_node);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopologyNode_Release(tee_node);
    ok(ref == 0, "Release returned %ld\n", ref);

    ref = IMFMediaType_Release(mediatype);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopology_Release(topology);
    ok(ref == 0, "Release returned %ld\n", ref);
}

struct test_handler
{
    IMFMediaTypeHandler IMFMediaTypeHandler_iface;

    ULONG set_current_count;
    IMFMediaType *current_type;
    IMFMediaType *invalid_type;
    BOOL support_any;
    BOOL return_media_types;

    ULONG enum_count;
    ULONG media_types_count;
    IMFMediaType **media_types;
    BOOL enum_complete;
};

static struct test_handler *impl_from_IMFMediaTypeHandler(IMFMediaTypeHandler *iface)
{
    return CONTAINING_RECORD(iface, struct test_handler, IMFMediaTypeHandler_iface);
}

static HRESULT WINAPI test_handler_QueryInterface(IMFMediaTypeHandler *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFMediaTypeHandler)
            || IsEqualIID(riid, &IID_IUnknown))
    {
        IMFMediaTypeHandler_AddRef((*obj = iface));
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_handler_AddRef(IMFMediaTypeHandler *iface)
{
    return 2;
}

static ULONG WINAPI test_handler_Release(IMFMediaTypeHandler *iface)
{
    return 1;
}

static HRESULT WINAPI test_handler_IsMediaTypeSupported(IMFMediaTypeHandler *iface, IMFMediaType *in_type,
        IMFMediaType **out_type)
{
    struct test_handler *impl = impl_from_IMFMediaTypeHandler(iface);
    BOOL result;
    ULONG i;

    if (out_type)
        *out_type = NULL;

    if (impl->invalid_type && IMFMediaType_Compare(impl->invalid_type, (IMFAttributes *)in_type,
            MF_ATTRIBUTES_MATCH_OUR_ITEMS, &result) == S_OK && result)
        return MF_E_INVALIDMEDIATYPE;

    if (impl->support_any)
        return S_OK;

    if (impl->current_type && IMFMediaType_Compare(impl->current_type, (IMFAttributes *)in_type,
            MF_ATTRIBUTES_MATCH_OUR_ITEMS, &result) == S_OK && result)
        return S_OK;

    for (i = 0; i < impl->media_types_count; ++i)
        if (IMFMediaType_Compare(impl->media_types[i], (IMFAttributes *)in_type,
                MF_ATTRIBUTES_MATCH_OUR_ITEMS, &result) == S_OK && result)
            return S_OK;

    return MF_E_INVALIDMEDIATYPE;
}

static HRESULT WINAPI test_handler_GetMediaTypeCount(IMFMediaTypeHandler *iface, DWORD *count)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_handler_GetMediaTypeByIndex(IMFMediaTypeHandler *iface, DWORD index,
        IMFMediaType **type)
{
    struct test_handler *impl = impl_from_IMFMediaTypeHandler(iface);

    if (impl->return_media_types && impl->media_types)
    {
        impl->enum_count++;

        if (index >= impl->media_types_count)
        {
            impl->enum_complete = TRUE;
            return MF_E_NO_MORE_TYPES;
        }

        IMFMediaType_AddRef((*type = impl->media_types[index]));
        return S_OK;
    }

    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_handler_SetCurrentMediaType(IMFMediaTypeHandler *iface, IMFMediaType *media_type)
{
    struct test_handler *impl = impl_from_IMFMediaTypeHandler(iface);

    if (impl->current_type)
        IMFMediaType_Release(impl->current_type);
    IMFMediaType_AddRef((impl->current_type = media_type));
    impl->set_current_count++;

    return S_OK;
}

static HRESULT WINAPI test_handler_GetCurrentMediaType(IMFMediaTypeHandler *iface, IMFMediaType **media_type)
{
    struct test_handler *impl = impl_from_IMFMediaTypeHandler(iface);
    HRESULT hr;

    if (!impl->current_type)
    {
        if (!impl->return_media_types)
            return E_FAIL;
        if (!impl->media_types_count)
            return MF_E_TRANSFORM_TYPE_NOT_SET;
        return MF_E_NOT_INITIALIZED;
    }

    if (FAILED(hr = MFCreateMediaType(media_type)))
        return hr;

    hr = IMFMediaType_CopyAllItems(impl->current_type, (IMFAttributes *)*media_type);
    if (FAILED(hr))
        IMFMediaType_Release(*media_type);

    return hr;
}

static HRESULT WINAPI test_handler_GetMajorType(IMFMediaTypeHandler *iface, GUID *type)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IMFMediaTypeHandlerVtbl test_handler_vtbl =
{
    test_handler_QueryInterface,
    test_handler_AddRef,
    test_handler_Release,
    test_handler_IsMediaTypeSupported,
    test_handler_GetMediaTypeCount,
    test_handler_GetMediaTypeByIndex,
    test_handler_SetCurrentMediaType,
    test_handler_GetCurrentMediaType,
    test_handler_GetMajorType,
};

static const struct test_handler test_handler = {.IMFMediaTypeHandler_iface.lpVtbl = &test_handler_vtbl};

struct test_stream_sink
{
    IMFStreamSink IMFStreamSink_iface;
    IMFGetService IMFGetService_iface;
    IMFMediaTypeHandler *handler;
    IMFMediaSink *media_sink;

    IMFAttributes *attributes;
    IUnknown *device_manager;
};

static struct test_stream_sink *impl_from_IMFStreamSink(IMFStreamSink *iface)
{
    return CONTAINING_RECORD(iface, struct test_stream_sink, IMFStreamSink_iface);
}

static HRESULT WINAPI test_stream_sink_QueryInterface(IMFStreamSink *iface, REFIID riid, void **obj)
{
    struct test_stream_sink *impl = impl_from_IMFStreamSink(iface);

    if (IsEqualIID(riid, &IID_IMFStreamSink)
            || IsEqualIID(riid, &IID_IMFMediaEventGenerator)
            || IsEqualIID(riid, &IID_IUnknown))
    {
        IMFStreamSink_AddRef((*obj = iface));
        return S_OK;
    }

    if (IsEqualIID(riid, &IID_IMFAttributes) && impl->attributes)
    {
        IMFAttributes_AddRef((*obj = impl->attributes));
        return S_OK;
    }

    if (IsEqualIID(riid, &IID_IMFGetService))
    {
        *obj = &impl->IMFGetService_iface;
        IMFGetService_AddRef(&impl->IMFGetService_iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_stream_sink_AddRef(IMFStreamSink *iface)
{
    return 2;
}

static ULONG WINAPI test_stream_sink_Release(IMFStreamSink *iface)
{
    return 1;
}

static HRESULT WINAPI test_stream_sink_GetEvent(IMFStreamSink *iface, DWORD flags, IMFMediaEvent **event)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_stream_sink_BeginGetEvent(IMFStreamSink *iface, IMFAsyncCallback *callback, IUnknown *state)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_stream_sink_EndGetEvent(IMFStreamSink *iface, IMFAsyncResult *result,
        IMFMediaEvent **event)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_stream_sink_QueueEvent(IMFStreamSink *iface, MediaEventType event_type,
        REFGUID ext_type, HRESULT hr, const PROPVARIANT *value)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_stream_sink_GetMediaSink(IMFStreamSink *iface, IMFMediaSink **sink)
{
    struct test_stream_sink *impl = impl_from_IMFStreamSink(iface);

    if (impl->media_sink)
    {
        IMFMediaSink_AddRef((*sink = impl->media_sink));
        return S_OK;
    }

    todo_wine
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_stream_sink_GetIdentifier(IMFStreamSink *iface, DWORD *id)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_stream_sink_GetMediaTypeHandler(IMFStreamSink *iface, IMFMediaTypeHandler **handler)
{
    struct test_stream_sink *impl = impl_from_IMFStreamSink(iface);

    if (impl->handler)
    {
        IMFMediaTypeHandler_AddRef((*handler = impl->handler));
        return S_OK;
    }

    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_stream_sink_ProcessSample(IMFStreamSink *iface, IMFSample *sample)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_stream_sink_PlaceMarker(IMFStreamSink *iface, MFSTREAMSINK_MARKER_TYPE marker_type,
        const PROPVARIANT *marker_value, const PROPVARIANT *context)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_stream_sink_Flush(IMFStreamSink *iface)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IMFStreamSinkVtbl test_stream_sink_vtbl =
{
    test_stream_sink_QueryInterface,
    test_stream_sink_AddRef,
    test_stream_sink_Release,
    test_stream_sink_GetEvent,
    test_stream_sink_BeginGetEvent,
    test_stream_sink_EndGetEvent,
    test_stream_sink_QueueEvent,
    test_stream_sink_GetMediaSink,
    test_stream_sink_GetIdentifier,
    test_stream_sink_GetMediaTypeHandler,
    test_stream_sink_ProcessSample,
    test_stream_sink_PlaceMarker,
    test_stream_sink_Flush,
};

static struct test_stream_sink *impl_from_IMFGetService(IMFGetService *iface)
{
    return CONTAINING_RECORD(iface, struct test_stream_sink, IMFGetService_iface);
}

static HRESULT WINAPI test_stream_sink_get_service_QueryInterface(IMFGetService *iface, REFIID riid, void **obj)
{
    struct test_stream_sink *stream = impl_from_IMFGetService(iface);
    return IMFStreamSink_QueryInterface(&stream->IMFStreamSink_iface, riid, obj);
}

static ULONG WINAPI test_stream_sink_get_service_AddRef(IMFGetService *iface)
{
    struct test_stream_sink *stream = impl_from_IMFGetService(iface);
    return IMFStreamSink_AddRef(&stream->IMFStreamSink_iface);
}

static ULONG WINAPI test_stream_sink_get_service_Release(IMFGetService *iface)
{
    struct test_stream_sink *stream = impl_from_IMFGetService(iface);
    return IMFStreamSink_Release(&stream->IMFStreamSink_iface);
}

static HRESULT WINAPI test_stream_sink_get_service_GetService(IMFGetService *iface, REFGUID service, REFIID riid, void **obj)
{
    struct test_stream_sink *stream = impl_from_IMFGetService(iface);

    if (IsEqualGUID(service, &MR_VIDEO_ACCELERATION_SERVICE) && stream->device_manager)
        return IUnknown_QueryInterface(stream->device_manager, riid, obj);

    return E_NOINTERFACE;
}

static const IMFGetServiceVtbl test_stream_sink_get_service_vtbl =
{
    test_stream_sink_get_service_QueryInterface,
    test_stream_sink_get_service_AddRef,
    test_stream_sink_get_service_Release,
    test_stream_sink_get_service_GetService,
};

static const struct test_stream_sink test_stream_sink =
{
    .IMFStreamSink_iface.lpVtbl = &test_stream_sink_vtbl,
    .IMFGetService_iface.lpVtbl = &test_stream_sink_get_service_vtbl,
};

enum object_state
{
    SOURCE_START,
    SOURCE_PAUSE,
    SOURCE_STOP,
    SOURCE_SHUTDOWN,
    SINK_ON_CLOCK_START,
    SINK_ON_CLOCK_PAUSE,
    SINK_ON_CLOCK_STOP,
    SINK_ON_CLOCK_RESTART,
    SINK_ON_CLOCK_SETRATE,
};

struct test_grabber_callback
{
    IMFSampleGrabberSinkCallback IMFSampleGrabberSinkCallback_iface;
    LONG refcount;

    IMFCollection *samples;
    HANDLE ready_event;
    HANDLE done_event;
};

static struct test_grabber_callback *impl_from_IMFSampleGrabberSinkCallback(IMFSampleGrabberSinkCallback *iface)
{
    return CONTAINING_RECORD(iface, struct test_grabber_callback, IMFSampleGrabberSinkCallback_iface);
}

static HRESULT WINAPI test_grabber_callback_QueryInterface(IMFSampleGrabberSinkCallback *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFSampleGrabberSinkCallback) ||
            IsEqualIID(riid, &IID_IMFClockStateSink) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFSampleGrabberSinkCallback_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_grabber_callback_AddRef(IMFSampleGrabberSinkCallback *iface)
{
    struct test_grabber_callback *grabber = impl_from_IMFSampleGrabberSinkCallback(iface);
    return InterlockedIncrement(&grabber->refcount);
}

static ULONG WINAPI test_grabber_callback_Release(IMFSampleGrabberSinkCallback *iface)
{
    struct test_grabber_callback *grabber = impl_from_IMFSampleGrabberSinkCallback(iface);
    ULONG refcount = InterlockedDecrement(&grabber->refcount);

    if (!refcount)
    {
        IMFCollection_Release(grabber->samples);
        if (grabber->ready_event)
            CloseHandle(grabber->ready_event);
        if (grabber->done_event)
            CloseHandle(grabber->done_event);
        free(grabber);
    }

    return refcount;
}

static HRESULT WINAPI test_grabber_callback_OnClockStart(IMFSampleGrabberSinkCallback *iface, MFTIME time, LONGLONG offset)
{
    return S_OK;
}

static HRESULT WINAPI test_grabber_callback_OnClockStop(IMFSampleGrabberSinkCallback *iface, MFTIME time)
{
    return S_OK;
}

static HRESULT WINAPI test_grabber_callback_OnClockPause(IMFSampleGrabberSinkCallback *iface, MFTIME time)
{
    return S_OK;
}

static HRESULT WINAPI test_grabber_callback_OnClockRestart(IMFSampleGrabberSinkCallback *iface, MFTIME time)
{
    return S_OK;
}

static HRESULT WINAPI test_grabber_callback_OnClockSetRate(IMFSampleGrabberSinkCallback *iface, MFTIME time, float rate)
{
    return S_OK;
}

static HRESULT WINAPI test_grabber_callback_OnSetPresentationClock(IMFSampleGrabberSinkCallback *iface,
        IMFPresentationClock *clock)
{
    return S_OK;
}

static HRESULT WINAPI test_grabber_callback_OnProcessSample(IMFSampleGrabberSinkCallback *iface, REFGUID major_type,
        DWORD sample_flags, LONGLONG sample_time, LONGLONG sample_duration, const BYTE *buffer, DWORD sample_size)
{
    struct test_grabber_callback *grabber = CONTAINING_RECORD(iface, struct test_grabber_callback, IMFSampleGrabberSinkCallback_iface);
    IMFSample *sample;
    HRESULT hr;
    DWORD res;

    if (!grabber->ready_event)
        return E_NOTIMPL;

    sample = create_sample(buffer, sample_size);
    hr = IMFSample_SetSampleFlags(sample, sample_flags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    /* FIXME: sample time is inconsistent across windows versions, ignore it */
    hr = IMFSample_SetSampleTime(sample, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFSample_SetSampleDuration(sample, sample_duration);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFCollection_AddElement(grabber->samples, (IUnknown *)sample);
    IMFSample_Release(sample);

    SetEvent(grabber->ready_event);
    res = WaitForSingleObject(grabber->done_event, 1000);
    ok(!res, "WaitForSingleObject returned %#lx\n", res);

    return S_OK;
}

static HRESULT WINAPI test_grabber_callback_OnShutdown(IMFSampleGrabberSinkCallback *iface)
{
    return S_OK;
}

static const IMFSampleGrabberSinkCallbackVtbl test_grabber_callback_vtbl =
{
    test_grabber_callback_QueryInterface,
    test_grabber_callback_AddRef,
    test_grabber_callback_Release,
    test_grabber_callback_OnClockStart,
    test_grabber_callback_OnClockStop,
    test_grabber_callback_OnClockPause,
    test_grabber_callback_OnClockRestart,
    test_grabber_callback_OnClockSetRate,
    test_grabber_callback_OnSetPresentationClock,
    test_grabber_callback_OnProcessSample,
    test_grabber_callback_OnShutdown,
};

static IMFSampleGrabberSinkCallback *create_test_grabber_callback(void)
{
    struct test_grabber_callback *grabber;
    HRESULT hr;

    if (!(grabber = calloc(1, sizeof(*grabber))))
        return NULL;

    grabber->IMFSampleGrabberSinkCallback_iface.lpVtbl = &test_grabber_callback_vtbl;
    grabber->refcount = 1;
    hr = MFCreateCollection(&grabber->samples);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    return &grabber->IMFSampleGrabberSinkCallback_iface;
}

enum loader_test_flags
{
    LOADER_TODO = 0x1,
    LOADER_TODO_OUT_TYPE = 0x2,
    LOADER_SET_XVP_FOR_PLAYBACK = 0x4,
    LOADER_NEEDS_VIDEO_PROCESSOR = 0x8,
    LOADER_SET_ENUMERATE_SOURCE_TYPES = 0x10,
    LOADER_NO_CURRENT_OUTPUT = 0x20,
    LOADER_SET_INVALID_INPUT = 0x40,
    LOADER_SET_MEDIA_TYPES = 0x80,
    LOADER_ADD_RESAMPLER_MFT = 0x100,
    LOADER_SUPPORT_ANY = 0x200,
    LOADER_EXPECT_SINK_ENUMERATED = 0x400,
    LOADER_EXPECT_SINK_ENUMERATED_TODO = 0x800,
    LOADER_ADD_TEST_MFT = 0x1000,
    LOADER_TEST_MFT_EXPECT_CONVERTER = 0x2000,
    LOADER_EXPECT_MFT_OUTPUT_ENUMERATED = 0x4000,
    LOADER_EXPECT_MFT_INPUT_ENUMERATED = 0x8000,
    LOADER_EXPECT_MFT_INPUT_ENUMERATED_TODO = 0x10000,
    LOADER_TODO_MFT_IN_TYPE = 0x20000,
    LOADER_TODO_MFT_OUT_TYPE = 0x40000,
};

static void test_topology_loader(void)
{
    static const media_type_desc audio_float_44100 =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_Float),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 4),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 4 * 44100),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 4 * 8),
    };
    static const media_type_desc audio_pcm_44100 =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 44100),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 8),
    };
    static const media_type_desc audio_pcm_48000 =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 48000),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 8),
    };
    static const media_type_desc audio_pcm_48000_resampler =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 192000),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 4),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        ATTR_UINT32(MF_MT_AUDIO_PREFER_WAVEFORMATEX, 1),
    };
    static const media_type_desc audio_float_48000 =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_Float),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 8 * 48000),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 8),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32),
    };
    static const media_type_desc audio_mp3_44100 =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_MP3),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 16000),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1),
    };
    static const media_type_desc audio_pcm_44100_incomplete =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 8),
    };
    static const media_type_desc audio_float_minimal =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_Float),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
    };
    static const media_type_desc audio_pcm_minimal =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_PCM),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
    };
    static const media_type_desc audio_float_44100_stereo =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_Float),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 2 * 4),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 2 * 4 * 44100),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 4 * 8),
        ATTR_UINT32(MF_MT_AUDIO_CHANNEL_MASK, 3, .todo = TRUE),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
    };
    static const media_type_desc audio_float_44100_no_ch =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_Float),
        ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 4 * 8),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
    };
    static const media_type_desc audio_aac_44100 =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_AAC),
        ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 2),
        ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
        ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 16000),
        ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1),
    };
    static const media_type_desc video_i420_1280 =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_I420),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 1280, 720),
    };
    static const media_type_desc video_color_convert_1280_rgb32 =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, DMOVideoFormat_RGB32),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 1280, 720),
    };
    static const media_type_desc video_video_processor_1280_rgb32 =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 1280, 720),
    };
    static const media_type_desc video_video_processor_1280_rgb24 =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB24),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 1280, 720),
    };
    static const media_type_desc video_video_processor_1280_rgb555 =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB555),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 1280, 720),
    };
    static const media_type_desc video_video_processor_1024_rgb32 =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 1024, 576),
    };
    static const media_type_desc video_video_processor_rgb32 =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
    };
    static const media_type_desc video_h264_1280 =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_H264),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 1280, 720),
    };
    static const media_type_desc video_nv12_1280 =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_NV12),
        ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 1280, 720),
        ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
        ATTR_UINT32(MF_MT_SAMPLE_SIZE, 1280 * 720 * 3 / 2),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        ATTR_UINT32(MF_MT_DEFAULT_STRIDE, 1280),
        ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
        ATTR_UINT32(MF_MT_INTERLACE_MODE, 7),
    };
    static const media_type_desc video_generic_yuv_1280 =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_RATIO(MF_MT_FRAME_RATE, 30000, 1001),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 1280, 720),
        ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
        ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1),
        ATTR_UINT32(MF_MT_INTERLACE_MODE, 7),
    };
    static const media_type_desc video_dummy =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
    };

    const struct loader_test
    {
        const media_type_desc *input_types[2];
        const media_type_desc *output_types[2];
        const media_type_desc *mft_input_type;
        const media_type_desc *mft_output_types[5];
        const media_type_desc *current_input;
        const media_type_desc *mft_current_output;
        const media_type_desc *decoded_type;
        const GUID *decoded_subtypes[3];
        MF_CONNECT_METHOD source_method;
        MF_CONNECT_METHOD sink_method;
        HRESULT expected_result;
        unsigned int expected_output_index;
        unsigned int flags;
        GUID decoder_class;
        GUID converter_class;
    }
    loader_tests[] =
    {
        {
            /* PCM -> PCM, same enumerated type, no current type */
            .input_types = {&audio_pcm_44100}, .output_types = {&audio_pcm_44100}, .sink_method = MF_CONNECT_DIRECT, .source_method = -1,
            .expected_result = S_OK,
        },
        {
            /* PCM -> PCM, same enumerated type, incomplete current type */
            .input_types = {&audio_pcm_44100}, .output_types = {&audio_pcm_44100}, .sink_method = MF_CONNECT_DIRECT, .source_method = -1,
            .current_input = &audio_pcm_44100_incomplete,
            .expected_result = MF_E_INVALIDMEDIATYPE,
            .flags = LOADER_TODO,
        },
        {
            /* PCM -> PCM, same enumerated bps, different current bps */
            .input_types = {&audio_pcm_48000}, .output_types = {&audio_pcm_48000}, .sink_method = MF_CONNECT_DIRECT, .source_method = -1,
            .current_input = &audio_pcm_44100,
            .expected_result = MF_E_INVALIDMEDIATYPE,
        },
        {
            /* PCM -> PCM, same enumerated bps, different current bps, force enumerate */
            .input_types = {&audio_pcm_48000}, .output_types = {&audio_pcm_48000}, .sink_method = MF_CONNECT_DIRECT, .source_method = -1,
            .current_input = &audio_pcm_44100,
            .expected_result = S_OK,
            .flags = LOADER_SET_ENUMERATE_SOURCE_TYPES,
        },

        {
            /* PCM -> PCM, incomplete enumerated type, same current type */
            .input_types = {&audio_pcm_44100_incomplete}, .output_types = {&audio_pcm_44100}, .sink_method = MF_CONNECT_DIRECT, .source_method = -1,
            .current_input = &audio_pcm_44100,
            .expected_result = S_OK,
        },
        {
            /* PCM -> PCM, incomplete enumerated type, same current type, force enumerate */
            .input_types = {&audio_pcm_44100_incomplete}, .output_types = {&audio_pcm_44100}, .sink_method = MF_CONNECT_DIRECT, .source_method = -1,
            .current_input = &audio_pcm_44100,
            .expected_result = MF_E_NO_MORE_TYPES,
            .flags = LOADER_SET_ENUMERATE_SOURCE_TYPES | LOADER_TODO,
        },

        {
            /* PCM -> PCM, different enumerated bps, no current type */
            .input_types = {&audio_pcm_44100}, .output_types = {&audio_pcm_48000}, .sink_method = MF_CONNECT_DIRECT, .source_method = -1,
            .expected_result = MF_E_INVALIDMEDIATYPE,
        },
        {
            /* PCM -> PCM, different enumerated bps, same current bps */
            .input_types = {&audio_pcm_44100}, .output_types = {&audio_pcm_48000}, .sink_method = MF_CONNECT_DIRECT, .source_method = -1,
            .current_input = &audio_pcm_48000,
            .expected_result = S_OK,
        },
        {
            /* PCM -> PCM, different enumerated bps, same current bps, force enumerate */
            .input_types = {&audio_pcm_44100}, .output_types = {&audio_pcm_48000}, .sink_method = MF_CONNECT_DIRECT, .source_method = -1,
            .current_input = &audio_pcm_48000,
            .expected_result = MF_E_NO_MORE_TYPES,
            .flags = LOADER_SET_ENUMERATE_SOURCE_TYPES,
        },
        {
            /* PCM -> PCM, different enumerated bps, no current type, sink allow converter */
            .input_types = {&audio_pcm_44100}, .output_types = {&audio_pcm_48000}, .sink_method = MF_CONNECT_ALLOW_CONVERTER, .source_method = MF_CONNECT_DIRECT,
            .expected_result = S_OK, .converter_class = CLSID_CResamplerMediaObject,
        },
        {
            /* #10 PCM -> PCM, different enumerated bps, same current type, sink allow converter, force enumerate */
            .input_types = {&audio_pcm_44100}, .output_types = {&audio_pcm_48000}, .sink_method = MF_CONNECT_ALLOW_CONVERTER, .source_method = -1,
            .current_input = &audio_pcm_48000,
            .expected_result = S_OK, .converter_class = CLSID_CResamplerMediaObject,
            .flags = LOADER_SET_ENUMERATE_SOURCE_TYPES,
        },
        {
            /* PCM -> PCM, different enumerated bps, no current type, sink allow converter, no output types */
            .input_types = {&audio_pcm_44100}, .output_types = {&audio_pcm_48000}, .sink_method = MF_CONNECT_ALLOW_CONVERTER, .source_method = MF_CONNECT_DIRECT,
            .expected_result = MF_E_INVALIDMEDIATYPE, .converter_class = CLSID_CResamplerMediaObject,
            .flags = LOADER_NO_CURRENT_OUTPUT | LOADER_TODO,
        },
        {
            /* PCM -> PCM, different enumerated bps, no current type, sink allow converter, no current output type */
            .input_types = {&audio_pcm_44100}, .output_types = {&audio_pcm_48000}, .sink_method = MF_CONNECT_ALLOW_CONVERTER, .source_method = MF_CONNECT_DIRECT,
            .expected_result = S_OK, .converter_class = CLSID_CResamplerMediaObject,
            .flags = LOADER_NO_CURRENT_OUTPUT | LOADER_SET_MEDIA_TYPES | LOADER_EXPECT_SINK_ENUMERATED_TODO,
        },
        {
            /* PCM -> PCM, different enumerated bps, no current type, sink allow decoder */
            .input_types = {&audio_pcm_44100}, .output_types = {&audio_pcm_48000}, .sink_method = MF_CONNECT_ALLOW_DECODER, .source_method = MF_CONNECT_DIRECT,
            .expected_result = S_OK, .converter_class = CLSID_CResamplerMediaObject,
        },
        {
            /* PCM -> PCM, different enumerated bps, no current type, default methods */
            .input_types = {&audio_pcm_44100}, .output_types = {&audio_pcm_48000}, .sink_method = -1, .source_method = -1,
            .expected_result = S_OK, .converter_class = CLSID_CResamplerMediaObject,
        },
        {
            /* PCM -> PCM, different enumerated bps, no current type, source allow converter */
            .input_types = {&audio_pcm_44100}, .output_types = {&audio_pcm_48000}, .sink_method = MF_CONNECT_DIRECT, .source_method = MF_CONNECT_ALLOW_CONVERTER,
            .expected_result = MF_E_INVALIDMEDIATYPE,
        },

        {
            /* Float -> PCM, refuse input type, add converter */
            .input_types = {&audio_float_44100}, .output_types = {&audio_pcm_48000}, .sink_method = MF_CONNECT_DIRECT, .source_method = -1,
            .expected_result = MF_E_NO_MORE_TYPES, .converter_class = CLSID_CResamplerMediaObject,
            .flags = LOADER_SET_INVALID_INPUT | LOADER_ADD_RESAMPLER_MFT,
        },
        {
            /* Float -> PCM, refuse input type, add converter, allow resampler output type */
            .input_types = {&audio_float_44100}, .output_types = {&audio_pcm_48000_resampler}, .sink_method = MF_CONNECT_DIRECT, .source_method = -1,
            .expected_result = S_OK, .converter_class = CLSID_CResamplerMediaObject,
            .flags = LOADER_SET_INVALID_INPUT | LOADER_ADD_RESAMPLER_MFT,
        },

        {
            /* MP3 -> PCM */
            .input_types = {&audio_mp3_44100}, .output_types = {&audio_pcm_44100}, .sink_method = MF_CONNECT_DIRECT, .source_method = -1,
            .current_input = &audio_mp3_44100,
            .expected_result = MF_E_INVALIDMEDIATYPE,
        },
        {
            /* MP3 -> PCM, force enumerate */
            .input_types = {&audio_mp3_44100}, .output_types = {&audio_pcm_44100}, .sink_method = MF_CONNECT_DIRECT, .source_method = -1,
            .current_input = &audio_mp3_44100,
            .expected_result = MF_E_NO_MORE_TYPES,
            .flags = LOADER_SET_ENUMERATE_SOURCE_TYPES,
        },
        {
            /* #20 MP3 -> PCM */
            .input_types = {&audio_mp3_44100}, .output_types = {&audio_pcm_44100}, .sink_method = MF_CONNECT_ALLOW_CONVERTER, .source_method = -1,
            .current_input = &audio_mp3_44100,
            .expected_result = MF_E_TRANSFORM_NOT_POSSIBLE_FOR_CURRENT_MEDIATYPE_COMBINATION,
            .flags = LOADER_TODO,
        },
        {
            /* MP3 -> PCM */
            .input_types = {&audio_mp3_44100}, .output_types = {&audio_pcm_44100}, .sink_method = MF_CONNECT_ALLOW_DECODER, .source_method = -1,
            .current_input = &audio_mp3_44100,
            .expected_result = S_OK, .decoder_class = CLSID_CMP3DecMediaObject,
        },
        {
            /* MP3 -> float, need both decoder and converter */
            .input_types = {&audio_mp3_44100}, .output_types = {&audio_float_48000}, .sink_method = MF_CONNECT_ALLOW_DECODER, .source_method = -1,
            .current_input = &audio_mp3_44100, .decoded_type = &audio_float_44100_no_ch,
            .expected_result = S_OK, .decoder_class = CLSID_CMP3DecMediaObject, .converter_class = CLSID_CResamplerMediaObject,
        },
        {
            /* MP3 -> float, need both decoder and converter, no current output type */
            .input_types = {&audio_mp3_44100}, .output_types = {&audio_float_48000}, .sink_method = MF_CONNECT_ALLOW_DECODER, .source_method = -1,
            .current_input = &audio_mp3_44100, .decoded_type = &audio_float_44100_no_ch,
            .expected_result = S_OK, .decoder_class = CLSID_CMP3DecMediaObject, .converter_class = CLSID_CResamplerMediaObject,
            .flags = LOADER_NO_CURRENT_OUTPUT | LOADER_SET_MEDIA_TYPES | LOADER_EXPECT_SINK_ENUMERATED_TODO,
        },
        {
            /* MP3 -> {float, PCM}, need both decoder and converter, no current output type */
            .input_types = {&audio_mp3_44100}, .output_types = {&audio_float_48000, &audio_pcm_48000}, .sink_method = MF_CONNECT_ALLOW_DECODER, .source_method = -1,
            .current_input = &audio_mp3_44100, .decoded_type = &audio_float_44100_stereo,
            .expected_result = S_OK, .decoder_class = CLSID_CMP3DecMediaObject, .converter_class = CLSID_CResamplerMediaObject,
            .flags = LOADER_NO_CURRENT_OUTPUT | LOADER_SET_MEDIA_TYPES | LOADER_EXPECT_SINK_ENUMERATED_TODO,
        },
        {
            /* MP3 -> {PCM, float}, need both decoder and converter, no current output type */
            .input_types = {&audio_mp3_44100}, .output_types = {&audio_pcm_48000, &audio_float_48000}, .sink_method = MF_CONNECT_ALLOW_DECODER, .source_method = -1,
            .current_input = &audio_mp3_44100, .decoded_type = &audio_float_44100_stereo, .expected_output_index = 1,
            .expected_result = S_OK, .decoder_class = CLSID_CMP3DecMediaObject, .converter_class = CLSID_CResamplerMediaObject,
            .flags = LOADER_NO_CURRENT_OUTPUT | LOADER_SET_MEDIA_TYPES | LOADER_TODO_OUT_TYPE | LOADER_EXPECT_SINK_ENUMERATED_TODO,
        },

        {
            /* {MP3, PCM} -> PCM, force enumerate, sink allow decoder, no output types */
            .input_types = {&audio_mp3_44100, &audio_pcm_44100}, .output_types = {&audio_pcm_44100}, .sink_method = MF_CONNECT_ALLOW_DECODER, .source_method = -1,
            .expected_result = S_OK,
            .flags = LOADER_NO_CURRENT_OUTPUT | LOADER_SET_ENUMERATE_SOURCE_TYPES,
        },
        {
            /* {MP3, PCM} -> PCM, force enumerate, independent outtypes, sink allow decoder, no current output type */
            .input_types = {&audio_mp3_44100, &audio_pcm_44100}, .output_types = {&audio_pcm_44100}, .sink_method = MF_CONNECT_ALLOW_DECODER,
            .source_method = MF_CONNECT_RESOLVE_INDEPENDENT_OUTPUTTYPES,
            .expected_result = S_OK, .decoder_class = CLSID_CMP3DecMediaObject,
            .flags = LOADER_NO_CURRENT_OUTPUT | LOADER_SET_MEDIA_TYPES | LOADER_SET_ENUMERATE_SOURCE_TYPES | LOADER_EXPECT_SINK_ENUMERATED_TODO,
        },

        {
            /* PCM -> PCM, different enumerated bps, add test MFT, configure MFT incomplete type */
            .input_types = {&audio_pcm_44100}, .output_types = {&audio_pcm_48000}, .sink_method = MF_CONNECT_DIRECT, .source_method = MF_CONNECT_DIRECT,
            .mft_output_types = {&audio_float_minimal, &audio_pcm_minimal, &audio_float_48000, &audio_pcm_48000},
            .mft_current_output = &audio_pcm_minimal,
            .expected_result = MF_E_INVALIDMEDIATYPE,
            .flags = LOADER_ADD_TEST_MFT | LOADER_TODO_MFT_IN_TYPE | LOADER_TODO_MFT_OUT_TYPE | LOADER_TODO,
        },
        {
            /* PCM -> PCM, different enumerated bps, add test MFT */
            .input_types = {&audio_pcm_44100}, .output_types = {&audio_pcm_48000}, .sink_method = MF_CONNECT_DIRECT, .source_method = MF_CONNECT_DIRECT,
            .mft_output_types = {&audio_float_minimal, &audio_pcm_minimal, &audio_float_48000, &audio_pcm_48000_resampler},
            .expected_result = MF_E_NO_MORE_TYPES,
            .flags = LOADER_ADD_TEST_MFT | LOADER_EXPECT_MFT_OUTPUT_ENUMERATED,
        },
        {
            /* #30 PCM -> PCM, different enumerated bps, add test MFT, configure MFT */
            .input_types = {&audio_pcm_44100}, .output_types = {&audio_pcm_48000}, .sink_method = MF_CONNECT_DIRECT, .source_method = MF_CONNECT_DIRECT,
            .mft_output_types = {&audio_float_minimal, &audio_pcm_minimal, &audio_float_48000, &audio_pcm_48000_resampler},
            .mft_current_output = &audio_pcm_48000,
            .expected_result = S_OK,
            .flags = LOADER_ADD_TEST_MFT | LOADER_TODO_MFT_IN_TYPE | LOADER_TODO_MFT_OUT_TYPE,
        },
        {
            /* PCM -> PCM, different enumerated bps, add test MFT, configure MFT, no output types */
            .input_types = {&audio_pcm_44100}, .output_types = {&audio_pcm_48000}, .sink_method = MF_CONNECT_DIRECT, .source_method = MF_CONNECT_DIRECT,
            .mft_output_types = {&audio_float_minimal, &audio_pcm_minimal, &audio_float_48000, &audio_pcm_48000_resampler},
            .mft_current_output = &audio_pcm_48000,
            .expected_result = S_OK,
            .flags = LOADER_ADD_TEST_MFT | LOADER_NO_CURRENT_OUTPUT | LOADER_TODO_MFT_IN_TYPE | LOADER_TODO_MFT_OUT_TYPE,
        },
        {
            /* MP3 -> PCM, different enumerated bps, add test MFT, configure MFT */
            .input_types = {&audio_mp3_44100}, .output_types = {&audio_pcm_48000}, .sink_method = MF_CONNECT_DIRECT, .source_method = MF_CONNECT_DIRECT,
            .mft_input_type = &audio_pcm_44100, .mft_output_types = {&audio_pcm_48000},
            .decoded_type = &audio_pcm_44100,
            .expected_result = S_OK, .decoder_class = CLSID_CMP3DecMediaObject,
            .flags = LOADER_ADD_TEST_MFT | LOADER_EXPECT_MFT_INPUT_ENUMERATED_TODO | LOADER_TODO_MFT_OUT_TYPE,
        },
        {
            /* MP3 -> PCM, different enumerated bps, add incompatible test MFT, configure MFT */
            .input_types = {&audio_mp3_44100}, .output_types = {&audio_pcm_48000}, .sink_method = MF_CONNECT_DIRECT, .source_method = MF_CONNECT_DIRECT,
            .mft_input_type = &audio_aac_44100, .mft_output_types = {&audio_pcm_48000},
            .expected_result = MF_E_TOPO_CODEC_NOT_FOUND,
            .flags = LOADER_ADD_TEST_MFT | LOADER_EXPECT_MFT_INPUT_ENUMERATED_TODO | LOADER_TODO,
        },

        {
            /* PCM -> PCM, different enumerated bps, add converter, no output types */
            .input_types = {&audio_pcm_44100}, .output_types = {&audio_pcm_48000}, .sink_method = MF_CONNECT_DIRECT, .source_method = MF_CONNECT_DIRECT,
            .expected_result = MF_E_NO_MORE_TYPES, .converter_class = CLSID_CResamplerMediaObject,
            .flags = LOADER_ADD_RESAMPLER_MFT | LOADER_NO_CURRENT_OUTPUT,
        },
        {
            /* PCM -> PCM, different enumerated bps, add converter, configure converter, no output types */
            .input_types = {&audio_pcm_44100}, .output_types = {&audio_pcm_48000}, .sink_method = MF_CONNECT_DIRECT, .source_method = MF_CONNECT_DIRECT,
            .mft_current_output = &audio_pcm_48000,
            .expected_result = S_OK, .converter_class = CLSID_CResamplerMediaObject,
            .flags = LOADER_ADD_RESAMPLER_MFT | LOADER_NO_CURRENT_OUTPUT,
        },

        {
            /* I420 -> RGB32, Color Convert media type */
            .input_types = {&video_i420_1280}, .output_types = {&video_color_convert_1280_rgb32}, .sink_method = -1, .source_method = -1,
            .expected_result = MF_E_TOPO_CODEC_NOT_FOUND, .converter_class = CLSID_CColorConvertDMO,
            .flags = LOADER_NEEDS_VIDEO_PROCESSOR,
        },
        {
            /* I420 -> RGB32, Video Processor media type */
            .input_types = {&video_i420_1280}, .output_types = {&video_video_processor_1280_rgb32}, .sink_method = -1, .source_method = -1,
            .expected_result = S_OK, .converter_class = CLSID_CColorConvertDMO,
        },
        {
            /* I420 -> RGB32, Video Processor media type without frame size */
            .input_types = {&video_i420_1280}, .output_types = {&video_video_processor_rgb32}, .sink_method = -1, .source_method = -1,
            .expected_result = S_OK, .converter_class = CLSID_CColorConvertDMO,
        },
        {
            /* H264 -> RGB32, Video Processor media type */
            .input_types = {&video_h264_1280}, .output_types = {&video_video_processor_1280_rgb32}, .sink_method = -1, .source_method = -1,
            .decoded_type = &video_nv12_1280,
            .expected_result = S_OK, .decoder_class = CLSID_CMSH264DecoderMFT, .converter_class = CLSID_CColorConvertDMO,
        },
        {
            /* #40 RGB32 -> Any Video, no current output type */
            .input_types = {&video_i420_1280}, .output_types = {&video_dummy}, .sink_method = -1, .source_method = -1,
            .expected_result = S_OK,
            .flags = LOADER_NO_CURRENT_OUTPUT | LOADER_SUPPORT_ANY,
        },
        {
            /* RGB32 -> Any Video, no current output type, refuse input type */
            .input_types = {&video_i420_1280}, .output_types = {&video_dummy}, .sink_method = -1, .source_method = -1,
            .expected_result = S_OK, .converter_class = CLSID_CColorConvertDMO,
            .flags = LOADER_NO_CURRENT_OUTPUT | LOADER_SET_INVALID_INPUT | LOADER_SUPPORT_ANY,
        },
        {
            /* RGB32 -> Any Video, no current output type, refuse input type */
            .input_types = {&video_i420_1280}, .output_types = {&video_video_processor_rgb32}, .sink_method = -1, .source_method = -1,
            .expected_result = S_OK, .converter_class = CLSID_CColorConvertDMO,
            .flags = LOADER_NO_CURRENT_OUTPUT | LOADER_SET_INVALID_INPUT | LOADER_SET_MEDIA_TYPES | LOADER_EXPECT_SINK_ENUMERATED_TODO,
        },

        {
            /* H264 -> {DMO_RGB32, MF_RGB32}, no current output type */
            .input_types = {&video_h264_1280}, .output_types = {&video_color_convert_1280_rgb32, &video_video_processor_1280_rgb32}, .sink_method = -1, .source_method = -1,
            .decoded_type = &video_nv12_1280, .expected_output_index = 1,
            .expected_result = S_OK, .decoder_class = CLSID_CMSH264DecoderMFT, .converter_class = CLSID_CColorConvertDMO,
            .flags = LOADER_NO_CURRENT_OUTPUT | LOADER_SET_MEDIA_TYPES | LOADER_EXPECT_SINK_ENUMERATED_TODO | LOADER_TODO,
        },
        {
            /* H264 -> {DMO_RGB32, MF_RGB32} */
            .input_types = {&video_h264_1280}, .output_types = {&video_color_convert_1280_rgb32, &video_video_processor_1280_rgb32}, .sink_method = -1, .source_method = -1,
            .decoded_type = &video_nv12_1280, .expected_output_index = 1,
            .expected_result = MF_E_TOPO_CODEC_NOT_FOUND, .decoder_class = CLSID_CMSH264DecoderMFT, .converter_class = CLSID_CColorConvertDMO,
        },

        {
            /* H264 -> {RGB32, RGB24} */
            .input_types = {&video_h264_1280}, .output_types = {&video_video_processor_1280_rgb32, &video_video_processor_1280_rgb24}, .sink_method = -1, .source_method = -1,
            .decoded_type = &video_generic_yuv_1280, .expected_output_index = 1,
            .decoded_subtypes = {&MFVideoFormat_NV12, &MFVideoFormat_YUY2},
            .expected_result = S_OK, .decoder_class = CLSID_CMSH264DecoderMFT, .converter_class = CLSID_CColorConvertDMO,
            .flags = LOADER_NO_CURRENT_OUTPUT | LOADER_SET_MEDIA_TYPES | LOADER_TODO_OUT_TYPE | LOADER_EXPECT_SINK_ENUMERATED_TODO,
        },
        {
            /* H264 -> {RGB24, RGB32} */
            .input_types = {&video_h264_1280}, .output_types = {&video_video_processor_1280_rgb24, &video_video_processor_1280_rgb32}, .sink_method = -1, .source_method = -1,
            .decoded_type = &video_nv12_1280, .expected_output_index = 1,
            .expected_result = S_OK, .decoder_class = CLSID_CMSH264DecoderMFT, .converter_class = CLSID_CColorConvertDMO,
            .flags = LOADER_NO_CURRENT_OUTPUT | LOADER_SET_MEDIA_TYPES | LOADER_TODO_OUT_TYPE | LOADER_EXPECT_SINK_ENUMERATED_TODO,
        },
        {
            /* H264 -> {RGB24, RGB555} */
            .input_types = {&video_h264_1280}, .output_types = {&video_video_processor_1280_rgb24, &video_video_processor_1280_rgb555}, .sink_method = -1, .source_method = -1,
            .decoded_type = &video_nv12_1280, .expected_output_index = 1,
            .expected_result = S_OK, .decoder_class = CLSID_CMSH264DecoderMFT, .converter_class = CLSID_CColorConvertDMO,
            .flags = LOADER_NO_CURRENT_OUTPUT | LOADER_SET_MEDIA_TYPES | LOADER_TODO_OUT_TYPE | LOADER_EXPECT_SINK_ENUMERATED_TODO,
        },
        {
            /* H264 -> {RGB555, RGB24} */
            .input_types = {&video_h264_1280}, .output_types = {&video_video_processor_1280_rgb555, &video_video_processor_1280_rgb24}, .sink_method = -1, .source_method = -1,
            .decoded_type = &video_nv12_1280, .expected_output_index = 1,
            .expected_result = S_OK, .decoder_class = CLSID_CMSH264DecoderMFT, .converter_class = CLSID_CColorConvertDMO,
            .flags = LOADER_NO_CURRENT_OUTPUT | LOADER_SET_MEDIA_TYPES | LOADER_TODO_OUT_TYPE | LOADER_EXPECT_SINK_ENUMERATED_TODO,
        },

        {
            /* H264 -> RGB32, Color Convert media type, no current output type */
            .input_types = {&video_h264_1280}, .output_types = {&video_color_convert_1280_rgb32}, .sink_method = -1, .source_method = -1,
            .decoded_type = &video_nv12_1280,
            .expected_result = MF_E_TOPO_CODEC_NOT_FOUND, .decoder_class = CLSID_CMSH264DecoderMFT, .converter_class = CLSID_CColorConvertDMO,
            .flags = LOADER_NO_CURRENT_OUTPUT | LOADER_SET_MEDIA_TYPES | LOADER_EXPECT_SINK_ENUMERATED_TODO,
        },

        {
            /* #50 H264 -> {DMO_RGB32, MF_RGB32}, no current output type, set XVP */
            .input_types = {&video_h264_1280}, .output_types = {&video_color_convert_1280_rgb32, &video_video_processor_1280_rgb32}, .sink_method = -1, .source_method = -1,
            .decoded_type = &video_generic_yuv_1280, .expected_output_index = 1,
            .decoded_subtypes = {&MFVideoFormat_NV12, &MFVideoFormat_YUY2},
            .expected_result = S_OK, .decoder_class = CLSID_CMSH264DecoderMFT, .converter_class = CLSID_VideoProcessorMFT,
            .flags = LOADER_NO_CURRENT_OUTPUT | LOADER_SET_MEDIA_TYPES | LOADER_SET_XVP_FOR_PLAYBACK | LOADER_EXPECT_SINK_ENUMERATED_TODO | LOADER_TODO,
        },
        {
            /* H264 -> RGB32, resize, set XVP */
            .input_types = {&video_h264_1280}, .output_types = {&video_video_processor_1024_rgb32}, .sink_method = -1, .source_method = -1,
            .decoded_type = &video_nv12_1280,
            .expected_result = S_OK, .decoder_class = CLSID_CMSH264DecoderMFT, .converter_class = CLSID_VideoProcessorMFT,
            .flags = LOADER_SET_XVP_FOR_PLAYBACK,
        },

        {
            /* H264 -> RGB32, Color Convert media type, add test MFT */
            .input_types = {&video_h264_1280}, .output_types = {&video_color_convert_1280_rgb32}, .sink_method = -1, .source_method = -1,
            .mft_input_type = &video_color_convert_1280_rgb32, .mft_output_types = {&video_color_convert_1280_rgb32},
            .expected_result = MF_E_TOPO_CODEC_NOT_FOUND, .decoder_class = CLSID_CMSH264DecoderMFT,
            .flags = LOADER_ADD_TEST_MFT | LOADER_EXPECT_MFT_INPUT_ENUMERATED_TODO | LOADER_TODO,
        },
        {
            /* H264 -> RGB32, Video Processor media type, add test MFT */
            .input_types = {&video_h264_1280}, .output_types = {&video_video_processor_1280_rgb32}, .sink_method = -1, .source_method = -1,
            .mft_input_type = &video_video_processor_1280_rgb32, .mft_output_types = {&video_video_processor_1280_rgb32},
            .expected_result = S_OK, .decoder_class = CLSID_CMSH264DecoderMFT, .converter_class = CLSID_CColorConvertDMO,
            .flags = LOADER_ADD_TEST_MFT | LOADER_TODO_MFT_OUT_TYPE | LOADER_TEST_MFT_EXPECT_CONVERTER | LOADER_EXPECT_MFT_INPUT_ENUMERATED_TODO,
        },
        {
            /* H264 -> NV12, add test MFT */
            .input_types = {&video_h264_1280}, .output_types = {&video_nv12_1280}, .sink_method = -1, .source_method = -1,
            .mft_input_type = &video_nv12_1280, .mft_output_types = {&video_nv12_1280},
            .expected_result = S_OK, .decoder_class = CLSID_CMSH264DecoderMFT,
            .flags = LOADER_ADD_TEST_MFT | LOADER_TODO_MFT_OUT_TYPE | LOADER_EXPECT_MFT_INPUT_ENUMERATED_TODO,
        },
    };

    IMFTopologyNode *src_node, *sink_node, *src_node2, *sink_node2, *mft_node;
    IMFSampleGrabberSinkCallback *grabber_callback = create_test_grabber_callback();
    IMFMediaType *media_type, *input_types[2], *output_types[2];
    struct test_stream_sink stream_sink = test_stream_sink;
    IMFTopology *topology, *topology2, *full_topology;
    unsigned int i, j, count, expected_count, value;
    struct test_handler handler = test_handler;
    IMFPresentationDescriptor *pd;
    IMFActivate *sink_activate;
    MF_TOPOLOGY_TYPE node_type;
    IMFStreamDescriptor *sd;
    IMFTransform *transform;
    IMFMediaSource *source;
    IMFTopoLoader *loader;
    IUnknown *node_object;
    WORD node_count;
    TOPOID node_id, oldtopoid, newtopoid;
    DWORD index;
    HRESULT hr;
    BOOL ret;
    LONG ref;

    stream_sink.handler = &handler.IMFMediaTypeHandler_iface;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Startup failure, hr %#lx.\n", hr);

    hr = MFCreateTopoLoader(NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateTopoLoader(&loader);
    ok(hr == S_OK, "Failed to create topology loader, hr %#lx.\n", hr);

    hr = MFCreateTopology(&topology);
    ok(hr == S_OK, "Failed to create topology, hr %#lx.\n", hr);

    /* Empty topology */
    hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
    todo_wine_if(hr == S_OK)
    ok(hr == MF_E_TOPO_UNSUPPORTED, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK) IMFTopology_Release(full_topology);

    /* Add source node. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &src_node);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    /* When a decoder is involved, windows requires this attribute to be present */
    create_descriptors(1, &media_type, NULL, &pd, &sd);
    IMFMediaType_Release(media_type);

    source = create_test_source(pd);

    hr = IMFTopologyNode_SetUnknown(src_node, &MF_TOPONODE_SOURCE, (IUnknown *)source);
    ok(hr == S_OK, "Failed to set node source, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetUnknown(src_node, &MF_TOPONODE_STREAM_DESCRIPTOR, (IUnknown *)sd);
    ok(hr == S_OK, "Failed to set node sd, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetUnknown(src_node, &MF_TOPONODE_PRESENTATION_DESCRIPTOR, (IUnknown *)pd);
    ok(hr == S_OK, "Failed to set node pd, hr %#lx.\n", hr);

    hr = IMFTopology_AddNode(topology, src_node);
    ok(hr == S_OK, "Failed to add a node, hr %#lx.\n", hr);

    /* Source node only. */
    hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
    ok(hr == MF_E_TOPO_UNSUPPORTED, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &sink_node);
    ok(hr == S_OK, "Failed to create output node, hr %#lx.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = MFCreateSampleGrabberSinkActivate(media_type, grabber_callback, &sink_activate);
    ok(hr == S_OK, "Failed to create grabber sink, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetObject(sink_node, (IUnknown *)sink_activate);
    ok(hr == S_OK, "Failed to set object, hr %#lx.\n", hr);

    IMFMediaType_Release(media_type);

    hr = IMFTopology_AddNode(topology, sink_node);
    ok(hr == S_OK, "Failed to add sink node, hr %#lx.\n", hr);

    hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
    todo_wine_if(hr == MF_E_TOPO_SINK_ACTIVATES_UNSUPPORTED)
    ok(hr == MF_E_TOPO_UNSUPPORTED, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopologyNode_ConnectOutput(src_node, 0, sink_node, 0);
    ok(hr == S_OK, "Failed to connect nodes, hr %#lx.\n", hr);

    /* Sink was not resolved. */
    hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
    ok(hr == MF_E_TOPO_SINK_ACTIVATES_UNSUPPORTED, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetObject(sink_node, NULL);
    ok(hr == S_OK, "Failed to set object, hr %#lx.\n", hr);

    hr = IMFActivate_ShutdownObject(sink_activate);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ref = IMFActivate_Release(sink_activate);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = IMFTopologyNode_SetUnknown(src_node, &MF_TOPONODE_SOURCE, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTopologyNode_SetUnknown(src_node, &MF_TOPONODE_STREAM_DESCRIPTOR, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTopologyNode_SetUnknown(src_node, &MF_TOPONODE_PRESENTATION_DESCRIPTOR, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ref = IMFMediaSource_Release(source);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFPresentationDescriptor_Release(pd);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFStreamDescriptor_Release(sd);
    ok(ref == 0, "Release returned %ld\n", ref);


    hr = MFCreateMediaType(&input_types[0]);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);
    hr = MFCreateMediaType(&input_types[1]);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = MFCreateMediaType(&output_types[0]);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);
    hr = MFCreateMediaType(&output_types[1]);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(loader_tests); ++i)
    {
        const struct loader_test *test = &loader_tests[i];
        struct test_transform *test_transform = NULL;
        IMFMediaType *mft_output_types[4] = {0};
        IMFMediaType *mft_input_type = NULL;

        winetest_push_context("%u", i);

        init_media_type(input_types[0], *test->input_types[0], -1);
        init_media_type(output_types[0], *test->output_types[0], -1);

        handler.set_current_count = 0;
        if (test->flags & LOADER_NO_CURRENT_OUTPUT)
            handler.current_type = NULL;
        else
            IMFMediaType_AddRef((handler.current_type = output_types[0]));

        if (test->flags & LOADER_SET_INVALID_INPUT)
            handler.invalid_type = input_types[0];
        else
            handler.invalid_type = NULL;
        if (test->flags & LOADER_SUPPORT_ANY)
            handler.support_any = TRUE;
        else
            handler.support_any = FALSE;
        if (test->flags & LOADER_SET_MEDIA_TYPES)
            handler.return_media_types = TRUE;
        else
            handler.return_media_types = FALSE;

        handler.enum_count = 0;
        handler.media_types_count = 1;
        handler.media_types = output_types;
        handler.enum_complete = FALSE;

        if (test->input_types[1])
        {
            init_media_type(input_types[1], *test->input_types[1], -1);
        }

        if (test->output_types[1])
        {
            init_media_type(output_types[1], *test->output_types[1], -1);
            handler.media_types_count = 2;
        }

        if (test->flags & (LOADER_ADD_RESAMPLER_MFT | LOADER_ADD_TEST_MFT))
        {
            hr = IMFTopology_Clear(topology);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            hr = IMFTopology_AddNode(topology, src_node);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            hr = IMFTopology_AddNode(topology, sink_node);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            hr = MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE, &mft_node);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            if (test->flags & LOADER_ADD_TEST_MFT)
            {
                mft_input_type = input_types[0];

                if (test->mft_input_type)
                {
                    hr = MFCreateMediaType(&mft_input_type);
                    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);
                    init_media_type(mft_input_type, *test->mft_input_type, -1);
                }
                for (j = 0; test->mft_output_types[j]; ++j)
                {
                    hr = MFCreateMediaType(&mft_output_types[j]);
                    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);
                    init_media_type(mft_output_types[j], *test->mft_output_types[j], -1);
                }
                test_transform_create(1, &mft_input_type, j, mft_output_types, FALSE, &transform);
                test_transform = test_transform_from_IMFTransform(transform);
                IMFTransform_AddRef(transform);
            }
            else
            {
                hr = CoCreateInstance(&CLSID_CResamplerMediaObject, NULL, CLSCTX_INPROC_SERVER, &IID_IMFTransform, (void **)&transform);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
                hr = IMFTopologyNode_SetGUID(mft_node, &MF_TOPONODE_TRANSFORM_OBJECTID, &CLSID_CResamplerMediaObject);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            }
            hr = IMFTopologyNode_SetObject(mft_node, (IUnknown *)transform);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            if (test->mft_current_output)
            {
                hr = IMFTransform_SetInputType(transform, 0, input_types[0], 0);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
                hr = MFCreateMediaType(&media_type);
                ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);
                init_media_type(media_type, *test->mft_current_output, -1);
                hr = IMFTransform_SetOutputType(transform, 0, media_type, 0);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
                IMFMediaType_Release(media_type);
            }
            else
            {
                IMFTransform_SetInputType(transform, 0, NULL, 0);
                IMFTransform_SetOutputType(transform, 0, NULL, 0);
            }
            IMFTransform_Release(transform);

            if (test_transform)
            {
                test_transform->input_type_set = FALSE;
                test_transform->output_type_set = FALSE;
            }

            hr = IMFTopology_AddNode(topology, mft_node);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            hr = IMFTopologyNode_ConnectOutput(src_node, 0, mft_node, 0);
            ok(hr == S_OK, "Failed to connect nodes, hr %#lx.\n", hr);
            hr = IMFTopologyNode_ConnectOutput(mft_node, 0, sink_node, 0);
            ok(hr == S_OK, "Failed to connect nodes, hr %#lx.\n", hr);
            IMFTopologyNode_Release(mft_node);
        }
        else
        {
            hr = IMFTopology_Clear(topology);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            hr = IMFTopology_AddNode(topology, src_node);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            hr = IMFTopology_AddNode(topology, sink_node);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            hr = IMFTopologyNode_ConnectOutput(src_node, 0, sink_node, 0);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        }

        create_descriptors(1 + !!test->input_types[1], input_types, test->current_input, &pd, &sd);

        source = create_test_source(pd);

        init_source_node(source, test->source_method, src_node, pd, sd);
        init_sink_node(&stream_sink.IMFStreamSink_iface, test->sink_method, sink_node);

        hr = IMFTopology_GetCount(topology, &count);
        ok(hr == S_OK, "Failed to get attribute count, hr %#lx.\n", hr);
        ok(!count, "Unexpected count %u.\n", count);

        if (test->flags & LOADER_SET_ENUMERATE_SOURCE_TYPES)
            IMFTopology_SetUINT32(topology, &MF_TOPOLOGY_ENUMERATE_SOURCE_TYPES, 1);
        if (test->flags & LOADER_SET_XVP_FOR_PLAYBACK)
            IMFTopology_SetUINT32(topology, &MF_TOPOLOGY_ENABLE_XVP_FOR_PLAYBACK, 1);
        hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
        IMFTopology_DeleteItem(topology, &MF_TOPOLOGY_ENUMERATE_SOURCE_TYPES);
        IMFTopology_DeleteItem(topology, &MF_TOPOLOGY_ENABLE_XVP_FOR_PLAYBACK);

        if (test->flags & LOADER_NEEDS_VIDEO_PROCESSOR && !has_video_processor)
            ok(hr == MF_E_INVALIDMEDIATYPE || hr == MF_E_TOPO_CODEC_NOT_FOUND,
                    "Unexpected hr %#lx\n", hr);
        else
        {
            todo_wine_if(test->flags & LOADER_TODO)
            ok(hr == test->expected_result, "Unexpected hr %#lx\n", hr);
            ok(full_topology != topology, "Unexpected instance.\n");
        }

        if (test->expected_result != hr)
        {
            if (hr != S_OK) ref = 0;
            else ref = IMFTopology_Release(full_topology);
            ok(ref == 0, "Release returned %ld\n", ref);
        }
        else if (test->expected_result == S_OK)
        {
            IMFTopology_GetTopologyID(topology, &oldtopoid);
            IMFTopology_GetTopologyID(full_topology, &newtopoid);
            ok(oldtopoid == newtopoid, "Expected the same topology id. %I64u == %I64u\n", oldtopoid, newtopoid);
            ok(topology != full_topology, "Expected a different object for the resolved topology.\n");

            expected_count = 1 + !!(test->flags & LOADER_SET_ENUMERATE_SOURCE_TYPES)
                    + !!(test->flags & LOADER_SET_XVP_FOR_PLAYBACK);
            hr = IMFTopology_GetCount(full_topology, &count);
            ok(hr == S_OK, "Failed to get attribute count, hr %#lx.\n", hr);
            todo_wine
            ok(count == expected_count, "Unexpected count %u.\n", count);

            value = 0xdeadbeef;
            hr = IMFTopology_GetUINT32(full_topology, &MF_TOPOLOGY_RESOLUTION_STATUS, &value);
todo_wine {
            ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);
            ok(value == MF_TOPOLOGY_RESOLUTION_SUCCEEDED, "Unexpected value %#x.\n", value);
}
            count = 2 + !!test_transform;
            if (!IsEqualGUID(&test->decoder_class, &GUID_NULL))
                count++;
            if (!IsEqualGUID(&test->converter_class, &GUID_NULL))
                count++;

            hr = IMFTopology_GetNodeCount(full_topology, &node_count);
            ok(hr == S_OK, "Failed to get node count, hr %#lx.\n", hr);
            ok(node_count == count, "Unexpected node count %u.\n", node_count);

            hr = IMFTopologyNode_GetTopoNodeID(src_node, &node_id);
            ok(hr == S_OK, "Failed to get source node id, hr %#lx.\n", hr);

            hr = IMFTopology_GetNodeByID(full_topology, node_id, &src_node2);
            ok(hr == S_OK, "Failed to get source in resolved topology, hr %#lx.\n", hr);

            hr = IMFTopologyNode_GetOutputPrefType(src_node2, 0, &media_type);
            ok(hr == E_FAIL, "Got src pref type hr %#lx.\n", hr);

            hr = IMFTopologyNode_GetTopoNodeID(sink_node, &node_id);
            ok(hr == S_OK, "Failed to get sink node id, hr %#lx.\n", hr);

            hr = IMFTopology_GetNodeByID(full_topology, node_id, &sink_node2);
            ok(hr == S_OK, "Failed to get sink in resolved topology, hr %#lx.\n", hr);

            hr = IMFTopologyNode_GetInputPrefType(sink_node, 0, &media_type);
            ok(hr == E_FAIL, "Got sink pref type hr %#lx.\n", hr);
            hr = IMFTopologyNode_GetInputPrefType(sink_node2, 0, &media_type);
            todo_wine
            ok(hr == S_OK, "Failed to get sink pref type, hr %#lx.\n", hr);
            if (hr == S_OK)
                IMFMediaType_Release(media_type);

            if (!IsEqualGUID(&test->decoder_class, &GUID_NULL))
            {
                GUID class_id;

                hr = IMFTopologyNode_GetOutput(src_node2, 0, &mft_node, &index);
                ok(hr == S_OK, "Failed to get decoder in resolved topology, hr %#lx.\n", hr);
                ok(!index, "Unexpected stream index %lu.\n", index);

                hr = IMFTopologyNode_GetNodeType(mft_node, &node_type);
                ok(hr == S_OK, "Failed to get transform node type in resolved topology, hr %#lx.\n", hr);
                ok(node_type == MF_TOPOLOGY_TRANSFORM_NODE, "Unexpected node type %u.\n", node_type);

                value = 0;
                hr = IMFTopologyNode_GetUINT32(mft_node, &MF_TOPONODE_DECODER, &value);
                ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);
                ok(value == 1, "Unexpected value.\n");

                value = 0;
                hr = IMFTopologyNode_GetUINT32(mft_node, &MF_TOPONODE_MARKIN_HERE, &value);
                todo_wine
                ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);
                todo_wine
                ok(value == 1, "Unexpected value.\n");

                value = 0;
                hr = IMFTopologyNode_GetUINT32(mft_node, &MF_TOPONODE_MARKOUT_HERE, &value);
                todo_wine
                ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);
                todo_wine
                ok(value == 1, "Unexpected value.\n");

                class_id = GUID_NULL;
                hr = IMFTopologyNode_GetGUID(mft_node, &MF_TOPONODE_TRANSFORM_OBJECTID, &class_id);
                ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);
                ok(IsEqualGUID(&class_id, &test->decoder_class), "got MF_TOPONODE_TRANSFORM_OBJECTID %s.\n", debugstr_guid(&class_id));

                hr = IMFTopologyNode_GetObject(mft_node, &node_object);
                ok(hr == S_OK, "Failed to get object of transform node, hr %#lx.\n", hr);
                IMFTopologyNode_Release(mft_node);

                hr = IUnknown_QueryInterface(node_object, &IID_IMFTransform, (void **)&transform);
                ok(hr == S_OK, "Failed to get IMFTransform from transform node's object, hr %#lx.\n", hr);
                IUnknown_Release(node_object);

                hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type);
                ok(hr == S_OK, "Failed to get transform input type, hr %#lx.\n", hr);
                hr = IMFMediaType_Compare(input_types[0], (IMFAttributes *)media_type, MF_ATTRIBUTES_MATCH_OUR_ITEMS, &ret);
                ok(hr == S_OK, "Failed to compare media types, hr %#lx.\n", hr);
                ok(ret, "Input type of first transform doesn't match source node type.\n");
                IMFMediaType_Release(media_type);

                hr = IMFTransform_GetOutputCurrentType(transform, 0, &media_type);
                ok(hr == S_OK, "Failed to get transform input type, hr %#lx.\n", hr);
                if (IsEqualGUID(&test->converter_class, &GUID_NULL) && !test_transform)
                {
                    hr = IMFMediaType_Compare(output_types[test->expected_output_index], (IMFAttributes *)media_type, MF_ATTRIBUTES_MATCH_OUR_ITEMS, &ret);
                    ok(hr == S_OK, "Failed to compare media types, hr %#lx.\n", hr);
                    ok(ret, "Output type of first transform doesn't match sink node type.\n");
                }
                else if (test->decoded_type)
                {
                    check_media_type(media_type, *test->decoded_type, -1);
                    if (test->decoded_subtypes[0])
                        check_subtype(media_type, test->decoded_subtypes);
                }
                IMFMediaType_Release(media_type);

                IMFTransform_Release(transform);
            }

            if (!IsEqualGUID(&test->converter_class, &GUID_NULL))
            {
                GUID class_id;

                hr = IMFTopologyNode_GetInput(sink_node2, 0, &mft_node, &index);
                ok(hr == S_OK, "Failed to get converter in resolved topology, hr %#lx.\n", hr);
                ok(!index, "Unexpected stream index %lu.\n", index);

                if (test->flags & LOADER_TEST_MFT_EXPECT_CONVERTER)
                {
                    IMFTopologyNode *test_mft_node = mft_node;
                    hr = IMFTopologyNode_GetInput(test_mft_node, 0, &mft_node, &index);
                    ok(hr == S_OK, "Failed to get converter in resolved topology, hr %#lx.\n", hr);
                    ok(!index, "Unexpected stream index %lu.\n", index);
                    IMFTopologyNode_Release(test_mft_node);
                }

                hr = IMFTopologyNode_GetNodeType(mft_node, &node_type);
                ok(hr == S_OK, "Failed to get transform node type in resolved topology, hr %#lx.\n", hr);
                ok(node_type == MF_TOPOLOGY_TRANSFORM_NODE, "Unexpected node type %u.\n", node_type);

                class_id = GUID_NULL;
                hr = IMFTopologyNode_GetGUID(mft_node, &MF_TOPONODE_TRANSFORM_OBJECTID, &class_id);
                if (test->flags & LOADER_SET_XVP_FOR_PLAYBACK)
                {
                    todo_wine
                    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Got attribute hr %#lx.\n", hr);
                }
                else
                {
                    ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);
                    todo_wine_if(IsEqualGUID(&test->converter_class, &CLSID_CColorConvertDMO))
                    ok(IsEqualGUID(&class_id, &test->converter_class), "got MF_TOPONODE_TRANSFORM_OBJECTID %s.\n", debugstr_guid(&class_id));
                }

                hr = IMFTopologyNode_GetObject(mft_node, &node_object);
                ok(hr == S_OK, "Failed to get object of transform node, hr %#lx.\n", hr);
                IMFTopologyNode_Release(mft_node);

                hr = IUnknown_QueryInterface(node_object, &IID_IMFTransform, (void **)&transform);
                ok(hr == S_OK, "Failed to get IMFTransform from transform node's object, hr %#lx.\n", hr);
                IUnknown_Release(node_object);

                hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type);
                ok(hr == S_OK, "Failed to get transform input type, hr %#lx.\n", hr);
                if (IsEqualGUID(&test->decoder_class, &GUID_NULL))
                {
                    hr = IMFMediaType_Compare(input_types[0], (IMFAttributes *)media_type, MF_ATTRIBUTES_MATCH_OUR_ITEMS, &ret);
                    ok(hr == S_OK, "Failed to compare media types, hr %#lx.\n", hr);
                    ok(ret, "Input type of last transform doesn't match source node type.\n");
                }
                else if (test->decoded_type)
                {
                    check_media_type(media_type, *test->decoded_type, -1);
                    if (test->decoded_subtypes[0])
                        check_subtype(media_type, test->decoded_subtypes);
                }
                IMFMediaType_Release(media_type);

                hr = IMFTransform_GetOutputCurrentType(transform, 0, &media_type);
                ok(hr == S_OK, "Failed to get transform output type, hr %#lx.\n", hr);
                hr = IMFMediaType_Compare(output_types[test->expected_output_index], (IMFAttributes *)media_type, MF_ATTRIBUTES_MATCH_OUR_ITEMS, &ret);
                ok(hr == S_OK, "Failed to compare media types, hr %#lx.\n", hr);
                todo_wine_if(test->flags & LOADER_TODO_OUT_TYPE)
                ok(ret, "Output type of last transform doesn't match sink node type.\n");
                IMFMediaType_Release(media_type);

                IMFTransform_Release(transform);
            }

            IMFTopologyNode_Release(src_node2);
            IMFTopologyNode_Release(sink_node2);

            hr = IMFTopology_SetUINT32(full_topology, &IID_IMFTopology, 123);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            hr = IMFTopoLoader_Load(loader, full_topology, &topology2, NULL);
            ok(hr == S_OK, "Failed to resolve topology, hr %#lx.\n", hr);
            ok(full_topology != topology2, "Unexpected instance.\n");
            IMFTopology_GetTopologyID(topology2, &oldtopoid);
            IMFTopology_GetTopologyID(full_topology, &newtopoid);
            ok(oldtopoid == newtopoid, "Expected the same topology id. %I64u == %I64u\n", oldtopoid, newtopoid);
            hr = IMFTopology_GetUINT32(topology2, &IID_IMFTopology, &value);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            ref = IMFTopology_Release(topology2);
            ok(ref == 0, "Release returned %ld\n", ref);
            ref = IMFTopology_Release(full_topology);
            ok(ref == 0, "Release returned %ld\n", ref);
        }

        hr = IMFTopology_GetCount(topology, &count);
        ok(hr == S_OK, "Failed to get attribute count, hr %#lx.\n", hr);
        ok(!count, "Unexpected count %u.\n", count);

        if (test->flags & LOADER_SET_MEDIA_TYPES)
            ok(handler.enum_count, "got %lu GetMediaTypeByIndex\n", handler.enum_count);
        else
            ok(!handler.enum_count, "got %lu GetMediaTypeByIndex\n", handler.enum_count);
        ok(!handler.set_current_count, "got %lu SetCurrentMediaType\n", handler.set_current_count);
        todo_wine_if(test->flags & LOADER_EXPECT_SINK_ENUMERATED_TODO)
        ok(handler.enum_complete == !!(test->flags & (LOADER_EXPECT_SINK_ENUMERATED | LOADER_EXPECT_SINK_ENUMERATED_TODO)),
                "Got enum_complete %u.\n", handler.enum_complete);

        if (test_transform)
        {
            todo_wine_if(test->flags & LOADER_EXPECT_MFT_INPUT_ENUMERATED_TODO)
            ok(test_transform->input_enum_complete == !!(test->flags & (LOADER_EXPECT_MFT_INPUT_ENUMERATED | LOADER_EXPECT_MFT_INPUT_ENUMERATED_TODO)),
                    "got transform input_enum_complete %u\n", test_transform->input_enum_complete);
            todo_wine_if(test_transform->output_enum_complete && (test->flags & LOADER_TODO))
            ok(test_transform->output_enum_complete == !!(test->flags & LOADER_EXPECT_MFT_OUTPUT_ENUMERATED),
                    "got transform output_enum_complete %u\n", test_transform->output_enum_complete);
            todo_wine_if(test->flags & LOADER_TODO_MFT_IN_TYPE)
            ok(test_transform->input_type_set == (test->expected_result != MF_E_TOPO_CODEC_NOT_FOUND),
                    "Got transform input_type_set %u.\n", test_transform->input_type_set);
            todo_wine_if(test->flags & LOADER_TODO_MFT_OUT_TYPE)
            ok(test_transform->output_type_set == SUCCEEDED(test->expected_result),
                    "Got transform output_type_set %u.\n", test_transform->output_type_set);
            IMFTransform_Release(&test_transform->IMFTransform_iface);
        }

        if (handler.current_type)
            IMFMediaType_Release(handler.current_type);
        handler.current_type = NULL;

        hr = IMFTopologyNode_SetUnknown(src_node, &MF_TOPONODE_SOURCE, NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IMFTopologyNode_SetUnknown(src_node, &MF_TOPONODE_STREAM_DESCRIPTOR, NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IMFTopologyNode_SetUnknown(src_node, &MF_TOPONODE_PRESENTATION_DESCRIPTOR, NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ref = IMFMediaSource_Release(source);
        ok(ref == 0, "Release returned %ld\n", ref);
        ref = IMFPresentationDescriptor_Release(pd);
        ok(ref == 0, "Release returned %ld\n", ref);
        ref = IMFStreamDescriptor_Release(sd);
        ok(ref == 0, "Release returned %ld\n", ref);
        if (mft_input_type && test->mft_input_type)
            IMFMediaType_Release(mft_input_type);
        for (j = 0; j < ARRAY_SIZE(mft_output_types) && mft_output_types[j]; ++j)
            IMFMediaType_Release(mft_output_types[j]);

        winetest_pop_context();
    }

    ref = IMFTopology_Release(topology);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopoLoader_Release(loader);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopologyNode_Release(src_node);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopologyNode_Release(sink_node);
    ok(ref == 0, "Release returned %ld\n", ref);

    ref = IMFMediaType_Release(input_types[0]);
    todo_wine
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFMediaType_Release(input_types[1]);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFMediaType_Release(output_types[0]);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFMediaType_Release(output_types[1]);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = MFShutdown();
    ok(hr == S_OK, "Shutdown failure, hr %#lx.\n", hr);

    IMFSampleGrabberSinkCallback_Release(grabber_callback);
}

static void test_topology_loader_evr(void)
{
    static const media_type_desc media_type_desc =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 640, 480),
        ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE),
        {0},
    };
    IMFTopologyNode *node, *source_node, *evr_node;
    IMFTopology *topology, *full_topology;
    IMFPresentationDescriptor *pd;
    IMFMediaTypeHandler *handler;
    unsigned int i, count, value;
    IMFStreamSink *stream_sink;
    IMFMediaType *media_type;
    IMFStreamDescriptor *sd;
    IMFActivate *activate;
    IMFTopoLoader *loader;
    IMFMediaSink *sink;
    WORD node_count;
    UINT64 value64;
    HWND window;
    HRESULT hr;
    LONG ref;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    /* EVR sink node. */
    window = create_window();

    hr = MFCreateVideoRendererActivate(window, &activate);
    ok(hr == S_OK, "Failed to create activate object, hr %#lx.\n", hr);

    hr = IMFActivate_ActivateObject(activate, &IID_IMFMediaSink, (void **)&sink);
    if (FAILED(hr))
    {
        skip("Failed to create an EVR sink, skipping tests.\n");
        DestroyWindow(window);
        IMFActivate_Release(activate);
        CoUninitialize();
        return;
    }
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateTopoLoader(&loader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Source node. */
    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &source_node);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);
    init_media_type(media_type, media_type_desc, -1);

    create_descriptors(1, &media_type, &media_type_desc, &pd, &sd);
    init_source_node(NULL, -1, source_node, pd, sd);
    IMFPresentationDescriptor_Release(pd);
    IMFStreamDescriptor_Release(sd);


    hr = IMFMediaSink_GetStreamSinkById(sink, 0, &stream_sink);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &evr_node);
    ok(hr == S_OK, "Failed to create topology node, hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetObject(evr_node, (IUnknown *)stream_sink);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFStreamSink_GetMediaTypeHandler(stream_sink, &handler);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaTypeHandler_Release(handler);

    IMFStreamSink_Release(stream_sink);

    hr = MFCreateTopology(&topology);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopology_AddNode(topology, source_node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTopology_AddNode(topology, evr_node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFTopologyNode_ConnectOutput(source_node, 0, evr_node, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopologyNode_SetUINT32(evr_node, &MF_TOPONODE_CONNECT_METHOD, MF_CONNECT_DIRECT);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopologyNode_GetCount(evr_node, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected attribute count %u.\n", count);

    hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTopology_GetNodeCount(full_topology, &node_count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node_count == 3, "Unexpected node count %u.\n", node_count);

    for (i = 0; i < node_count; ++i)
    {
        MF_TOPOLOGY_TYPE node_type;

        hr = IMFTopology_GetNode(full_topology, i, &node);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFTopologyNode_GetNodeType(node, &node_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        switch (node_type)
        {
        case MF_TOPOLOGY_OUTPUT_NODE:
        {
            value = 1;
            hr = IMFTopologyNode_GetUINT32(node, &MF_TOPONODE_STREAMID, &value);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(!value, "Unexpected stream id %u.\n", value);
            break;
        }
        case MF_TOPOLOGY_TRANSFORM_NODE:
        {
            IMFAttributes *attrs;
            IMFTransform *copier;
            IUnknown *obj;

            hr = IMFTopologyNode_GetObject(node, (IUnknown **)&obj);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            hr = IUnknown_QueryInterface(obj, &IID_IMFTransform, (void **)&copier);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            hr = IMFTransform_GetAttributes(copier, &attrs);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            value = 0xdeadbeef;
            hr = IMFAttributes_GetUINT32(attrs, &MFT_SUPPORT_DYNAMIC_FORMAT_CHANGE, &value);
            ok(value == 1, "Unexpected dynamic state support state %u.\n", value);

            IMFAttributes_Release(attrs);
            IMFTransform_Release(copier);
            IUnknown_Release(obj);
            break;
        }
        case MF_TOPOLOGY_SOURCESTREAM_NODE:
        {
            value64 = 1;
            hr = IMFTopologyNode_GetUINT64(node, &MF_TOPONODE_MEDIASTART, &value64);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(!value64, "Unexpected value.\n");
            break;
        }
        default:
           ok(0, "Got unexpected node type %u.\n", node_type);
           break;
        }

        IMFTopologyNode_Release(node);
    }

    ref = IMFTopology_Release(full_topology);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopoLoader_Release(loader);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopology_Release(topology);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopologyNode_Release(source_node);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopologyNode_Release(evr_node);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = IMFActivate_ShutdownObject(activate);
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);
    ref = IMFActivate_Release(activate);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFMediaSink_Release(sink);
    ok(ref == 0, "Release returned %ld\n", ref);

    ref = IMFMediaType_Release(media_type);
    ok(ref == 0, "Release returned %ld\n", ref);

    DestroyWindow(window);

    CoUninitialize();
}

static void test_topology_loader_d3d_init(IMFTopology *topology, IMFMediaSource *source, IMFPresentationDescriptor *pd, IMFStreamDescriptor *sd,
            UINT transform_count, IMFTransform **transforms, IMFStreamSink *sink)
{
    IMFTopologyNode *prev_node, *node;
    HRESULT hr;
    UINT i;

    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    init_source_node(source, MF_CONNECT_DIRECT, node, pd, sd);
    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    prev_node = node;

    for (i = 0; i < transform_count; i++)
    {
        hr = MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE, &node);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IMFTopologyNode_SetObject(node, (IUnknown *)transforms[i]);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IMFTopology_AddNode(topology, node);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IMFTopologyNode_ConnectOutput(prev_node, 0, node, 0);
        ok(hr == S_OK, "Failed to connect nodes, hr %#lx.\n", hr);
        IMFTopologyNode_Release(prev_node);
        prev_node = node;
    }

    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &node);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    init_sink_node(sink, MF_CONNECT_DIRECT, node);
    hr = IMFTopology_AddNode(topology, node);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    hr = IMFTopologyNode_ConnectOutput(prev_node, 0, node, 0);
    ok(hr == S_OK, "Failed to connect nodes, hr %#lx.\n", hr);
    IMFTopologyNode_Release(prev_node);
    IMFTopologyNode_Release(node);
}

static void test_topology_loader_d3d(void)
{
    static const media_type_desc video_media_type_desc =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 1280, 720),
    };

    struct test_stream_sink stream_sink = test_stream_sink;
    struct test_handler handler = test_handler;
    IMFTopology *topology, *full_topology;
    IMFPresentationDescriptor *pd;
    IMFTransform *transforms[4];
    IMFMediaType *media_type;
    IMFStreamDescriptor *sd;
    IMFMediaSource *source;
    IMFTopoLoader *loader;
    HRESULT hr;
    WORD count;
    LONG ref;

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    init_media_type(media_type, video_media_type_desc, -1);

    stream_sink.handler = &handler.IMFMediaTypeHandler_iface;
    hr = MFCreateAttributes(&stream_sink.attributes, 1);
    ok(hr == S_OK, "got hr %#lx.\n", hr);

    handler.media_types_count = 1;
    handler.media_types = &media_type;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "got hr %#lx.\n", hr);

    hr = MFCreateTopoLoader(&loader);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    hr = MFCreateTopology(&topology);
    ok(hr == S_OK, "got hr %#lx.\n", hr);

    create_descriptors(1, &media_type, &video_media_type_desc, &pd, &sd);
    source = create_test_source(pd);

    test_topology_loader_d3d_init(topology, source, pd, sd, 0, NULL, &stream_sink.IMFStreamSink_iface);

    hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    ref = IMFTopology_GetNodeCount(full_topology, &count);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    ok(count == 2, "got count %u.\n", count);
    ref = IMFTopology_Release(full_topology);
    ok(ref == 0, "Release returned %ld\n", ref);


    /* sink D3D aware attribute doesn't change the resolved topology */

    hr = IMFAttributes_SetUINT32(stream_sink.attributes, &MF_SA_D3D11_AWARE, 1);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    hr = IMFAttributes_SetUINT32(stream_sink.attributes, &MF_SA_D3D_AWARE, 1);
    ok(hr == S_OK, "got hr %#lx.\n", hr);

    hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    ref = IMFTopology_GetNodeCount(full_topology, &count);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    ok(count == 2, "got count %u.\n", count);
    ref = IMFTopology_Release(full_topology);
    ok(ref == 0, "Release returned %ld\n", ref);


    /* source D3D aware attribute doesn't change the resolved topology */

    hr = IMFStreamDescriptor_SetUINT32(sd, &MF_SA_D3D11_AWARE, 1);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    hr = IMFStreamDescriptor_SetUINT32(sd, &MF_SA_D3D_AWARE, 1);
    ok(hr == S_OK, "got hr %#lx.\n", hr);

    hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    ref = IMFTopology_GetNodeCount(full_topology, &count);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    ok(count == 2, "got count %u.\n", count);
    ref = IMFTopology_Release(full_topology);
    ok(ref == 0, "Release returned %ld\n", ref);

    ref = IMFTopology_Release(topology);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopoLoader_Release(loader);
    ok(ref == 0, "Release returned %ld\n", ref);


    /* transform D3D aware attribute doesn't change the resolved topology */

    hr = MFCreateTopoLoader(&loader);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    hr = MFCreateTopology(&topology);
    ok(hr == S_OK, "got hr %#lx.\n", hr);

    test_transform_create(1, &media_type, 1, &media_type, FALSE, &transforms[0]);
    test_transform_create(1, &media_type, 1, &media_type, TRUE, &transforms[1]);
    test_transform_create(1, &media_type, 1, &media_type, FALSE, &transforms[2]);
    test_transform_create(1, &media_type, 1, &media_type, TRUE, &transforms[3]);

    test_topology_loader_d3d_init(topology, source, pd, sd, 4, transforms, &stream_sink.IMFStreamSink_iface);

    IMFTransform_Release(transforms[0]);
    IMFTransform_Release(transforms[1]);
    IMFTransform_Release(transforms[2]);
    IMFTransform_Release(transforms[3]);

    hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    ref = IMFTopology_GetNodeCount(full_topology, &count);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    ok(count == 6, "got count %u.\n", count);
    ref = IMFTopology_Release(full_topology);
    ok(ref == 0, "Release returned %ld\n", ref);

    ref = IMFTopology_Release(topology);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopoLoader_Release(loader);
    ok(ref == 0, "Release returned %ld\n", ref);


    ref = IMFMediaSource_Release(source);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFPresentationDescriptor_Release(pd);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFStreamDescriptor_Release(sd);
    ok(ref == 0, "Release returned %ld\n", ref);

    hr = MFShutdown();
    ok(hr == S_OK, "Shutdown failure, hr %#lx.\n", hr);


    IMFAttributes_Release(stream_sink.attributes);
    IMFMediaType_Release(media_type);

    winetest_pop_context();
}

static void test_topology_loader_d3d9(MFTOPOLOGY_DXVA_MODE mode)
{
    static const media_type_desc video_media_type_desc =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 1280, 720),
    };

    struct test_stream_sink stream_sink = test_stream_sink;
    MFT_OUTPUT_STREAM_INFO output_stream_info = {0};
    struct test_handler handler = test_handler;
    IMFTopology *topology, *full_topology;
    IDirect3DDeviceManager9 *d3d9_manager;
    IMFPresentationDescriptor *pd;
    IDirect3DDevice9 *d3d9_device;
    IMFTransform *transforms[4];
    IMFMediaType *media_type;
    IMFStreamDescriptor *sd;
    IMFMediaSource *source;
    IMFTopoLoader *loader;
    IDirect3D9 *d3d9;
    UINT token;
    HRESULT hr;
    WORD count;
    HWND hwnd;
    BOOL ret;
    LONG ref;

    winetest_push_context("d3d9 mode %d", mode);

    hwnd = create_window();

    if (!(d3d9 = Direct3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create a D3D9 object, skipping tests.\n");
        goto skip_d3d9;
    }
    if (!(d3d9_device = create_d3d9_device(d3d9, hwnd)))
    {
        skip("Failed to create a D3D9 device, skipping tests.\n");
        IDirect3D9_Release(d3d9);
        goto skip_d3d9;
    }
    IDirect3D9_Release(d3d9);

    hr = DXVA2CreateDirect3DDeviceManager9(&token, &d3d9_manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDirect3DDeviceManager9_ResetDevice(d3d9_manager, d3d9_device, token);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IDirect3DDevice9_Release(d3d9_device);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    init_media_type(media_type, video_media_type_desc, -1);

    stream_sink.handler = &handler.IMFMediaTypeHandler_iface;
    hr = MFCreateAttributes(&stream_sink.attributes, 1);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    hr = IMFAttributes_SetUINT32(stream_sink.attributes, &MF_SA_D3D_AWARE, 1);
    ok(hr == S_OK, "got hr %#lx.\n", hr);

    stream_sink.device_manager = (IUnknown *)d3d9_manager;

    handler.media_types_count = 1;
    handler.media_types = &media_type;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "got hr %#lx.\n", hr);


    hr = MFCreateTopoLoader(&loader);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    hr = MFCreateTopology(&topology);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    hr = IMFTopology_SetUINT32(topology, &MF_TOPOLOGY_DXVA_MODE, mode);
    ok(hr == S_OK, "got hr %#lx.\n", hr);

    create_descriptors(1, &media_type, &video_media_type_desc, &pd, &sd);
    hr = IMFStreamDescriptor_SetUINT32(sd, &MF_SA_D3D_AWARE, 1);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    source = create_test_source(pd);


    /* D3D aware transforms connected to downstream non-D3D-aware transforms don't receive a device manager */

    test_transform_create(1, &media_type, 1, &media_type, FALSE, &transforms[0]);
    test_transform_create(1, &media_type, 1, &media_type, TRUE, &transforms[1]);
    test_transform_create(1, &media_type, 1, &media_type, FALSE, &transforms[2]);
    test_transform_create(1, &media_type, 1, &media_type, TRUE, &transforms[3]);

    test_topology_loader_d3d_init(topology, source, pd, sd, 4, transforms, &stream_sink.IMFStreamSink_iface);

    IMFMediaSource_Release(source);
    IMFPresentationDescriptor_Release(pd);
    IMFStreamDescriptor_Release(sd);

    test_transform_set_output_stream_info(transforms[3], &output_stream_info);

    if (mode == MFTOPOLOGY_DXVA_FULL)
    {
        test_transform_set_expect_d3d9_device_manager(transforms[1], d3d9_manager);
        test_transform_set_expect_d3d9_device_manager(transforms[3], d3d9_manager);
    }

    hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    ref = IMFTopology_GetNodeCount(full_topology, &count);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    ok(count == 6, "got count %u.\n", count);
    ref = IMFTopology_Release(full_topology);
    ok(ref == 0, "Release returned %ld\n", ref);

    if (mode == MFTOPOLOGY_DXVA_FULL)
    {
        ret = test_transform_got_d3d9_device_manager(transforms[0]);
        ok(!ret, "got d3d9 device manager\n");
        ret = test_transform_got_d3d9_device_manager(transforms[1]);
        ok(!ret, "got d3d9 device manager\n");
        ret = test_transform_got_d3d9_device_manager(transforms[2]);
        ok(!ret, "got d3d9 device manager\n");
        ret = test_transform_got_d3d9_device_manager(transforms[3]);
        ok(!!ret, "got d3d9 device manager\n");
    }

    /* copier is inserted before the sink if preceding node may allocate samples without a device manager */

    output_stream_info.dwFlags = MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES;
    hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    ref = IMFTopology_GetNodeCount(full_topology, &count);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    if (mode == MFTOPOLOGY_DXVA_NONE || mode == MFTOPOLOGY_DXVA_FULL)
        ok(count == 6, "got count %u.\n", count);
    else
        ok(count == 7, "got count %u.\n", count);
    ref = IMFTopology_Release(full_topology);
    ok(ref == 0, "Release returned %ld\n", ref);

    if (mode == MFTOPOLOGY_DXVA_FULL)
    {
        ret = test_transform_got_d3d9_device_manager(transforms[0]);
        ok(!ret, "got d3d9 device manager\n");
        ret = test_transform_got_d3d9_device_manager(transforms[1]);
        ok(!ret, "got d3d9 device manager\n");
        ret = test_transform_got_d3d9_device_manager(transforms[2]);
        ok(!ret, "got d3d9 device manager\n");
        ret = test_transform_got_d3d9_device_manager(transforms[3]);
        ok(!!ret, "got d3d9 device manager\n");
    }

    output_stream_info.dwFlags = MFT_OUTPUT_STREAM_PROVIDES_SAMPLES;
    hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    ref = IMFTopology_GetNodeCount(full_topology, &count);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    if (mode == MFTOPOLOGY_DXVA_NONE || mode == MFTOPOLOGY_DXVA_FULL)
        ok(count == 6, "got count %u.\n", count);
    else
        ok(count == 7, "got count %u.\n", count);
    ref = IMFTopology_Release(full_topology);
    ok(ref == 0, "Release returned %ld\n", ref);

    if (mode == MFTOPOLOGY_DXVA_FULL)
    {
        ret = test_transform_got_d3d9_device_manager(transforms[0]);
        ok(!ret, "got d3d9 device manager\n");
        ret = test_transform_got_d3d9_device_manager(transforms[1]);
        ok(!ret, "got d3d9 device manager\n");
        ret = test_transform_got_d3d9_device_manager(transforms[2]);
        ok(!ret, "got d3d9 device manager\n");
        ret = test_transform_got_d3d9_device_manager(transforms[3]);
        ok(!!ret, "got d3d9 device manager\n");
    }

    ref = IMFTopology_Release(topology);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopoLoader_Release(loader);
    ok(ref == 0, "Release returned %ld\n", ref);

    IMFTransform_Release(transforms[0]);
    IMFTransform_Release(transforms[1]);
    IMFTransform_Release(transforms[2]);
    IMFTransform_Release(transforms[3]);


    IMFAttributes_Release(stream_sink.attributes);
    IMFMediaType_Release(media_type);

    hr = MFShutdown();
    ok(hr == S_OK, "Shutdown failure, hr %#lx.\n", hr);

    IDirect3DDeviceManager9_Release(d3d9_manager);

skip_d3d9:
    DestroyWindow(hwnd);

    winetest_pop_context();
}

static void test_topology_loader_d3d11(MFTOPOLOGY_DXVA_MODE mode)
{
    static const media_type_desc video_media_type_desc =
    {
        ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
        ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32),
        ATTR_RATIO(MF_MT_FRAME_SIZE, 1280, 720),
    };

    struct test_stream_sink stream_sink = test_stream_sink;
    MFT_OUTPUT_STREAM_INFO output_stream_info = {0};
    struct test_handler handler = test_handler;
    IMFTopology *topology, *full_topology;
    IMFDXGIDeviceManager *dxgi_manager;
    ID3D11Multithread *multithread;
    IMFPresentationDescriptor *pd;
    IMFTransform *transforms[4];
    IMFMediaType *media_type;
    IMFStreamDescriptor *sd;
    IMFMediaSource *source;
    IMFTopoLoader *loader;
    ID3D11Device *d3d11;
    UINT token;
    HRESULT hr;
    WORD count;
    BOOL ret;
    LONG ref;

    winetest_push_context("d3d11 mode %d", mode);

    hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_VIDEO_SUPPORT, NULL, 0,
            D3D11_SDK_VERSION, &d3d11, NULL, NULL);
    if (FAILED(hr))
    {
        skip("D3D11 device creation failed, skipping tests.\n");
        goto skip_d3d11;
    }

    hr = ID3D11Device_QueryInterface(d3d11, &IID_ID3D11Multithread, (void **)&multithread);
    ok(hr == S_OK, "got %#lx\n", hr);
    ID3D11Multithread_SetMultithreadProtected(multithread, TRUE);
    ID3D11Multithread_Release(multithread);

    hr = pMFCreateDXGIDeviceManager(&token, &dxgi_manager);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IMFDXGIDeviceManager_ResetDevice(dxgi_manager, (IUnknown *)d3d11, token);
    ok(hr == S_OK, "got %#lx\n", hr);
    ID3D11Device_Release(d3d11);


    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "got hr %#lx.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    init_media_type(media_type, video_media_type_desc, -1);

    stream_sink.handler = &handler.IMFMediaTypeHandler_iface;
    hr = MFCreateAttributes(&stream_sink.attributes, 1);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    hr = IMFAttributes_SetUINT32(stream_sink.attributes, &MF_SA_D3D11_AWARE, 1);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    stream_sink.device_manager = (IUnknown *)dxgi_manager;

    handler.media_types_count = 1;
    handler.media_types = &media_type;

    create_descriptors(1, &media_type, &video_media_type_desc, &pd, &sd);
    hr = IMFStreamDescriptor_SetUINT32(sd, &MF_SA_D3D11_AWARE, 1);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    source = create_test_source(pd);


    /* D3D aware transforms connected to downstream non-D3D-aware transforms don't receive a device manager */

    hr = MFCreateTopoLoader(&loader);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    hr = MFCreateTopology(&topology);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    hr = IMFTopology_SetUINT32(topology, &MF_TOPOLOGY_DXVA_MODE, mode);
    ok(hr == S_OK, "got hr %#lx.\n", hr);

    test_transform_create(1, &media_type, 1, &media_type, TRUE, &transforms[0]);
    test_transform_create(1, &media_type, 1, &media_type, TRUE, &transforms[1]);
    test_transform_create(1, &media_type, 1, &media_type, FALSE, &transforms[2]);
    test_transform_create(1, &media_type, 1, &media_type, FALSE, &transforms[3]);

    test_topology_loader_d3d_init(topology, source, pd, sd, 4, transforms, &stream_sink.IMFStreamSink_iface);

    test_transform_set_output_stream_info(transforms[3], &output_stream_info);

    if (mode == MFTOPOLOGY_DXVA_FULL)
    {
        test_transform_set_expect_dxgi_device_manager(transforms[2], dxgi_manager);
        test_transform_set_expect_dxgi_device_manager(transforms[3], dxgi_manager);
    }

    hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    ref = IMFTopology_GetNodeCount(full_topology, &count);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    ok(count == 6, "got count %u.\n", count);
    ref = IMFTopology_Release(full_topology);
    ok(ref == 0, "Release returned %ld\n", ref);

    if (mode == MFTOPOLOGY_DXVA_FULL)
    {
        ret = test_transform_got_dxgi_device_manager(transforms[0]);
        ok(!ret, "got dxgi device manager\n");
        ret = test_transform_got_dxgi_device_manager(transforms[1]);
        ok(!ret, "got dxgi device manager\n");
        ret = test_transform_got_dxgi_device_manager(transforms[2]);
        ok(!ret, "got dxgi device manager\n");
        ret = test_transform_got_dxgi_device_manager(transforms[3]);
        ok(!ret, "got dxgi device manager\n");
    }

    /* copier is never needed in MFTOPOLOGY_DXVA_NONE mode */

    output_stream_info.dwFlags = MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES;
    hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    ref = IMFTopology_GetNodeCount(full_topology, &count);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    if (mode == MFTOPOLOGY_DXVA_NONE)
        ok(count == 6, "got count %u.\n", count);
    else
        todo_wine_if(mode == MFTOPOLOGY_DXVA_FULL) ok(count == 7, "got count %u.\n", count);
    ref = IMFTopology_Release(full_topology);
    ok(ref == 0, "Release returned %ld\n", ref);

    if (mode == MFTOPOLOGY_DXVA_FULL)
    {
        ret = test_transform_got_dxgi_device_manager(transforms[0]);
        ok(!ret, "got dxgi device manager\n");
        ret = test_transform_got_dxgi_device_manager(transforms[1]);
        ok(!ret, "got dxgi device manager\n");
        ret = test_transform_got_dxgi_device_manager(transforms[2]);
        ok(!ret, "got dxgi device manager\n");
        ret = test_transform_got_dxgi_device_manager(transforms[3]);
        ok(!ret, "got dxgi device manager\n");
    }

    output_stream_info.dwFlags = 0;

    ref = IMFTopology_Release(topology);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopoLoader_Release(loader);
    ok(ref == 0, "Release returned %ld\n", ref);

    IMFTransform_Release(transforms[0]);
    IMFTransform_Release(transforms[1]);
    IMFTransform_Release(transforms[2]);
    IMFTransform_Release(transforms[3]);


    /* D3D aware transforms connected together to downstream D3D-aware sink receive a device manager */

    hr = MFCreateTopoLoader(&loader);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    hr = MFCreateTopology(&topology);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    hr = IMFTopology_SetUINT32(topology, &MF_TOPOLOGY_DXVA_MODE, mode);
    ok(hr == S_OK, "got hr %#lx.\n", hr);

    test_transform_create(1, &media_type, 1, &media_type, FALSE, &transforms[0]);
    test_transform_create(1, &media_type, 1, &media_type, FALSE, &transforms[1]);
    test_transform_create(1, &media_type, 1, &media_type, TRUE, &transforms[2]);
    test_transform_create(1, &media_type, 1, &media_type, TRUE, &transforms[3]);

    test_topology_loader_d3d_init(topology, source, pd, sd, 4, transforms, &stream_sink.IMFStreamSink_iface);

    test_transform_set_output_stream_info(transforms[3], &output_stream_info);

    if (mode == MFTOPOLOGY_DXVA_FULL)
    {
        test_transform_set_expect_dxgi_device_manager(transforms[2], dxgi_manager);
        test_transform_set_expect_dxgi_device_manager(transforms[3], dxgi_manager);
    }

    hr = IMFTopoLoader_Load(loader, topology, &full_topology, NULL);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    ref = IMFTopology_GetNodeCount(full_topology, &count);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    ok(count == 6, "got count %u.\n", count);
    ref = IMFTopology_Release(full_topology);
    ok(ref == 0, "Release returned %ld\n", ref);

    if (mode == MFTOPOLOGY_DXVA_FULL)
    {
        ret = test_transform_got_dxgi_device_manager(transforms[0]);
        ok(!ret, "got dxgi device manager\n");
        ret = test_transform_got_dxgi_device_manager(transforms[1]);
        ok(!ret, "got dxgi device manager\n");
        ret = test_transform_got_dxgi_device_manager(transforms[2]);
        ok(!!ret, "got dxgi device manager\n");
        ret = test_transform_got_dxgi_device_manager(transforms[3]);
        ok(!!ret, "got dxgi device manager\n");
    }

    ref = IMFTopology_Release(topology);
    ok(ref == 0, "Release returned %ld\n", ref);
    ref = IMFTopoLoader_Release(loader);
    ok(ref == 0, "Release returned %ld\n", ref);

    IMFTransform_Release(transforms[0]);
    IMFTransform_Release(transforms[1]);
    IMFTransform_Release(transforms[2]);
    IMFTransform_Release(transforms[3]);


    IMFMediaSource_Release(source);
    IMFPresentationDescriptor_Release(pd);
    IMFStreamDescriptor_Release(sd);

    IMFAttributes_Release(stream_sink.attributes);
    IMFMediaType_Release(media_type);

    hr = MFShutdown();
    ok(hr == S_OK, "Shutdown failure, hr %#lx.\n", hr);

    IMFDXGIDeviceManager_Release(dxgi_manager);

skip_d3d11:
    winetest_pop_context();
}

START_TEST(topology)
{
    init_functions();

    test_topology();
    test_topology_tee_node();
    test_topology_loader();
    test_topology_loader_evr();
    test_topology_loader_d3d();
    test_topology_loader_d3d9(MFTOPOLOGY_DXVA_DEFAULT);
    test_topology_loader_d3d9(MFTOPOLOGY_DXVA_NONE);
    test_topology_loader_d3d9(MFTOPOLOGY_DXVA_FULL);
    test_topology_loader_d3d11(MFTOPOLOGY_DXVA_DEFAULT);
    test_topology_loader_d3d11(MFTOPOLOGY_DXVA_NONE);
    test_topology_loader_d3d11(MFTOPOLOGY_DXVA_FULL);
}
