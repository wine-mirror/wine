/*
 * DMO wrapper filter unit tests
 *
 * Copyright (C) 2019 Zebediah Figura
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
#include "wine/test.h"

static const GUID testdmo_clsid = {0x1234};
static const GUID test_iid = {0x33333333};

static const IMediaObjectVtbl dmo_vtbl;

static IMediaObject testdmo = {&dmo_vtbl};
static IUnknown *testdmo_outer_unk;
static LONG testdmo_refcount = 1;

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
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI dmo_GetOutputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    if (winetest_debug > 1) trace("GetOutputStreamInfo(%u)\n", index);
    *flags = 0;
    return S_OK;
}

static HRESULT WINAPI dmo_GetInputType(IMediaObject *iface, DWORD index, DWORD type_index, DMO_MEDIA_TYPE *type)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI dmo_GetOutputType(IMediaObject *iface, DWORD index, DWORD type_index, DMO_MEDIA_TYPE *type)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI dmo_SetInputType(IMediaObject *iface, DWORD index, const DMO_MEDIA_TYPE *type, DWORD flags)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI dmo_SetOutputType(IMediaObject *iface, DWORD index, const DMO_MEDIA_TYPE *type, DWORD flags)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
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
        DWORD *size, DWORD *lookahead, DWORD *align)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI dmo_GetOutputSizeInfo(IMediaObject *iface, DWORD index, DWORD *size, DWORD *alignment)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
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
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI dmo_Discontinuity(IMediaObject *iface, DWORD index)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI dmo_AllocateStreamingResources(IMediaObject *iface)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI dmo_FreeStreamingResources(IMediaObject *iface)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI dmo_GetInputStatus(IMediaObject *iface, DWORD index, DWORD *flags)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI dmo_ProcessInput(IMediaObject *iface, DWORD index,
    IMediaBuffer *buffer, DWORD flags, REFERENCE_TIME timestamp, REFERENCE_TIME timelength)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI dmo_ProcessOutput(IMediaObject *iface, DWORD flags,
        DWORD count, DMO_OUTPUT_DATA_BUFFER *buffers, DWORD *status)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
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
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IBaseFilter_QueryInterface(filter, &IID_IDMOWrapperFilter, (void **)&wrapper);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IDMOWrapperFilter_Init(wrapper, &testdmo_clsid, &DMOCATEGORY_AUDIO_DECODER);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    IDMOWrapperFilter_Release(wrapper);

    return filter;
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
    IBaseFilter *filter = create_dmo_wrapper();

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
    ok(hr == E_NOINTERFACE, "Got hr %#x.\n", hr);
    ok(!filter, "Got interface %p.\n", filter);

    hr = CoCreateInstance(&CLSID_DMOWrapperFilter, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(unk != &test_outer, "Returned IUnknown should not be outer IUnknown.\n");
    ref = get_refcount(unk);
    ok(ref == 1, "Got unexpected refcount %d.\n", ref);

    ref = IUnknown_AddRef(unk);
    ok(ref == 2, "Got unexpected refcount %d.\n", ref);

    ref = IUnknown_Release(unk);
    ok(ref == 1, "Got unexpected refcount %d.\n", ref);

    hr = IUnknown_QueryInterface(unk, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(unk2 == unk, "Got unexpected IUnknown %p.\n", unk2);
    IUnknown_Release(unk2);

    hr = IUnknown_QueryInterface(unk, &IID_IBaseFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IBaseFilter_QueryInterface(filter, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(unk2 == unk, "Got unexpected IUnknown %p.\n", unk2);
    IUnknown_Release(unk2);

    hr = IBaseFilter_QueryInterface(filter, &IID_IBaseFilter, (void **)&filter2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(filter2 == filter, "Got unexpected IBaseFilter %p.\n", filter2);
    IBaseFilter_Release(filter2);

    hr = IUnknown_QueryInterface(unk, &test_iid, (void **)&unk2);
    ok(hr == E_NOINTERFACE, "Got hr %#x.\n", hr);
    ok(!unk2, "Got unexpected IUnknown %p.\n", unk2);

    hr = IBaseFilter_QueryInterface(filter, &test_iid, (void **)&unk2);
    ok(hr == E_NOINTERFACE, "Got hr %#x.\n", hr);
    ok(!unk2, "Got unexpected IUnknown %p.\n", unk2);

    IBaseFilter_Release(filter);
    ref = IUnknown_Release(unk);
    ok(!ref, "Got unexpected refcount %d.\n", ref);

    /* Test also aggregation of the inner media object. */

    filter = create_dmo_wrapper();

    hr = IBaseFilter_QueryInterface(filter, &IID_IMediaObject, (void **)&unk);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(unk == (IUnknown *)&testdmo, "Got unexpected object %p.\n", unk);
    IUnknown_Release(unk);

    hr = IBaseFilter_QueryInterface(filter, &test_iid, (void **)&unk);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(unk == (IUnknown *)&testdmo, "Got unexpected object %p.\n", unk);
    IUnknown_Release(unk);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got unexpected refcount %d.\n", ref);
}

START_TEST(dmowrapper)
{
    DWORD cookie;
    HRESULT hr;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = DMORegister(L"Wine test DMO", &testdmo_clsid, &DMOCATEGORY_AUDIO_DECODER, 0, 0, NULL, 0, NULL);
    if (FAILED(hr))
    {
        skip("Failed to register DMO, hr %#x.\n", hr);
        return;
    }
    ok(hr == S_OK, "Failed to register class, hr %#x.\n", hr);

    hr = CoRegisterClassObject(&testdmo_clsid, (IUnknown *)&testdmo_cf,
            CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &cookie);
    ok(hr == S_OK, "Failed to register class, hr %#x.\n", hr);

    test_interfaces();
    test_aggregation();

    CoRevokeClassObject(cookie);
    DMOUnregister(&testdmo_clsid, &DMOCATEGORY_AUDIO_DECODER);

    CoUninitialize();
}
