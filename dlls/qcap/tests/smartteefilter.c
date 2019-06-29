/*
 * Smart tee filter unit tests
 *
 * Copyright 2015 Damjan Jovanovic
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

static const WCHAR sink_id[] = {'I','n','p','u','t',0};
static const WCHAR capture_id[] = {'C','a','p','t','u','r','e',0};
static const WCHAR preview_id[] = {'P','r','e','v','i','e','w',0};

static HANDLE event;

static IBaseFilter *create_smart_tee(void)
{
    IBaseFilter *filter = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_SmartTee, NULL, CLSCTX_INPROC_SERVER,
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
    IBaseFilter *filter = create_smart_tee();
    ULONG ref;
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

    IBaseFilter_FindPin(filter, sink_id, &pin);

    check_interface(pin, &IID_IMemInputPin, TRUE);
    check_interface(pin, &IID_IPin, TRUE);
    todo_wine check_interface(pin, &IID_IQualityControl, TRUE);
    check_interface(pin, &IID_IUnknown, TRUE);

    check_interface(pin, &IID_IAMStreamConfig, FALSE);
    check_interface(pin, &IID_IAMStreamControl, FALSE);
    check_interface(pin, &IID_IKsPropertySet, FALSE);
    check_interface(pin, &IID_IMediaPosition, FALSE);
    check_interface(pin, &IID_IMediaSeeking, FALSE);
    check_interface(pin, &IID_IPropertyBag, FALSE);

    IPin_Release(pin);

    IBaseFilter_FindPin(filter, capture_id, &pin);

    todo_wine check_interface(pin, &IID_IAMStreamControl, TRUE);
    check_interface(pin, &IID_IPin, TRUE);
    todo_wine check_interface(pin, &IID_IQualityControl, TRUE);
    check_interface(pin, &IID_IUnknown, TRUE);

    check_interface(pin, &IID_IAMStreamConfig, FALSE);
    check_interface(pin, &IID_IAsyncReader, FALSE);
    check_interface(pin, &IID_IKsPropertySet, FALSE);
    check_interface(pin, &IID_IMediaPosition, FALSE);
    check_interface(pin, &IID_IMediaSeeking, FALSE);
    check_interface(pin, &IID_IPropertyBag, FALSE);

    IPin_Release(pin);

    IBaseFilter_FindPin(filter, preview_id, &pin);

    todo_wine check_interface(pin, &IID_IAMStreamControl, TRUE);
    check_interface(pin, &IID_IPin, TRUE);
    todo_wine check_interface(pin, &IID_IQualityControl, TRUE);
    check_interface(pin, &IID_IUnknown, TRUE);

    check_interface(pin, &IID_IAMStreamConfig, FALSE);
    check_interface(pin, &IID_IAsyncReader, FALSE);
    check_interface(pin, &IID_IKsPropertySet, FALSE);
    check_interface(pin, &IID_IMediaPosition, FALSE);
    check_interface(pin, &IID_IMediaSeeking, FALSE);
    check_interface(pin, &IID_IPropertyBag, FALSE);

    IPin_Release(pin);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got unexpected refcount %d.\n", ref);
}

static void test_enum_pins(void)
{
    IBaseFilter *filter = create_smart_tee();
    IEnumPins *enum1, *enum2;
    ULONG count, ref;
    IPin *pins[4];
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
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(count == 1, "Got count %u.\n", count);
    IPin_Release(pins[0]);

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
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(count == 2, "Got count %u.\n", count);
    IPin_Release(pins[0]);
    IPin_Release(pins[1]);

    hr = IEnumPins_Next(enum1, 2, pins, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(count == 1, "Got count %u.\n", count);
    IPin_Release(pins[0]);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum1, 4, pins, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(count == 3, "Got count %u.\n", count);
    IPin_Release(pins[0]);
    IPin_Release(pins[1]);
    IPin_Release(pins[2]);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Clone(enum1, &enum2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Skip(enum1, 4);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumPins_Skip(enum1, 3);
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
    IBaseFilter *filter = create_smart_tee();
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

    hr = IBaseFilter_FindPin(filter, capture_id, &pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IEnumPins_Next(enum_pins, 1, &pin2, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(pin2 == pin, "Expected pin %p, got %p.\n", pin, pin2);
    IPin_Release(pin2);
    IPin_Release(pin);

    hr = IBaseFilter_FindPin(filter, preview_id, &pin);
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
    IBaseFilter *filter = create_smart_tee();
    PIN_DIRECTION dir;
    PIN_INFO info;
    ULONG count;
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

    hr = IPin_QueryInternalConnections(pin, NULL, &count);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    IPin_Release(pin);

    hr = IBaseFilter_FindPin(filter, capture_id, &pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_QueryPinInfo(pin, &info);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(info.pFilter == filter, "Expected filter %p, got %p.\n", filter, info.pFilter);
    ok(info.dir == PINDIR_OUTPUT, "Got direction %d.\n", info.dir);
    ok(!lstrcmpW(info.achName, capture_id), "Got name %s.\n", wine_dbgstr_w(info.achName));
    ref = get_refcount(filter);
    ok(ref == 3, "Got unexpected refcount %d.\n", ref);
    ref = get_refcount(pin);
    ok(ref == 3, "Got unexpected refcount %d.\n", ref);
    IBaseFilter_Release(info.pFilter);

    hr = IPin_QueryDirection(pin, &dir);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(dir == PINDIR_OUTPUT, "Got direction %d.\n", dir);

    hr = IPin_QueryId(pin, &id);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!lstrcmpW(id, capture_id), "Got id %s.\n", wine_dbgstr_w(id));
    CoTaskMemFree(id);

    hr = IPin_QueryInternalConnections(pin, NULL, &count);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    IPin_Release(pin);

    hr = IBaseFilter_FindPin(filter, preview_id, &pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_QueryPinInfo(pin, &info);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(info.pFilter == filter, "Expected filter %p, got %p.\n", filter, info.pFilter);
    ok(info.dir == PINDIR_OUTPUT, "Got direction %d.\n", info.dir);
    ok(!lstrcmpW(info.achName, preview_id), "Got name %s.\n", wine_dbgstr_w(info.achName));
    ref = get_refcount(filter);
    ok(ref == 3, "Got unexpected refcount %d.\n", ref);
    ref = get_refcount(pin);
    ok(ref == 3, "Got unexpected refcount %d.\n", ref);
    IBaseFilter_Release(info.pFilter);

    hr = IPin_QueryDirection(pin, &dir);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(dir == PINDIR_OUTPUT, "Got direction %d.\n", dir);

    hr = IPin_QueryId(pin, &id);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!lstrcmpW(id, preview_id), "Got id %s.\n", wine_dbgstr_w(id));
    CoTaskMemFree(id);

    hr = IPin_QueryInternalConnections(pin, NULL, &count);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    IPin_Release(pin);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_enum_media_types(void)
{
    IBaseFilter *filter = create_smart_tee();
    IEnumMediaTypes *enum1, *enum2;
    AM_MEDIA_TYPE *mts[2];
    ULONG ref, count;
    HRESULT hr;
    IPin *pin;

    IBaseFilter_FindPin(filter, sink_id, &pin);

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

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

typedef struct {
    IBaseFilter IBaseFilter_iface;
    LONG ref;
    BOOL isCapture;
    DWORD receiveThreadId;
    IPin IPin_iface;
    IMemInputPin IMemInputPin_iface;
    IMemAllocator *allocator;
    IBaseFilter *nullRenderer;
    IPin *nullRendererPin;
    IMemInputPin *nullRendererMemInputPin;
} SinkFilter;

typedef struct {
    IEnumPins IEnumPins_iface;
    LONG ref;
    ULONG index;
    SinkFilter *filter;
} SinkEnumPins;

static SinkEnumPins* create_SinkEnumPins(SinkFilter *filter);

static inline SinkFilter* impl_from_SinkFilter_IBaseFilter(IBaseFilter *iface)
{
    return CONTAINING_RECORD(iface, SinkFilter, IBaseFilter_iface);
}

static inline SinkFilter* impl_from_SinkFilter_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, SinkFilter, IPin_iface);
}

static inline SinkFilter* impl_from_SinkFilter_IMemInputPin(IMemInputPin *iface)
{
    return CONTAINING_RECORD(iface, SinkFilter, IMemInputPin_iface);
}

static inline SinkEnumPins* impl_from_SinkFilter_IEnumPins(IEnumPins *iface)
{
    return CONTAINING_RECORD(iface, SinkEnumPins, IEnumPins_iface);
}

static HRESULT WINAPI SinkFilter_QueryInterface(IBaseFilter *iface, REFIID riid, void **ppv)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    if(IsEqualIID(riid, &IID_IUnknown)) {
        *ppv = &This->IBaseFilter_iface;
    } else if(IsEqualIID(riid, &IID_IPersist)) {
        *ppv = &This->IBaseFilter_iface;
    } else if(IsEqualIID(riid, &IID_IMediaFilter)) {
        *ppv = &This->IBaseFilter_iface;
    } else if(IsEqualIID(riid, &IID_IBaseFilter)) {
        *ppv = &This->IBaseFilter_iface;
    } else {
        trace("no interface for %s\n", wine_dbgstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI SinkFilter_AddRef(IBaseFilter *iface)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI SinkFilter_Release(IBaseFilter *iface)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    if(!ref) {
        if (This->allocator)
            IMemAllocator_Release(This->allocator);
        IMemInputPin_Release(This->nullRendererMemInputPin);
        IPin_Release(This->nullRendererPin);
        IBaseFilter_Release(This->nullRenderer);
        CoTaskMemFree(This);
    }
    return ref;
}

static HRESULT WINAPI SinkFilter_GetClassID(IBaseFilter *iface, CLSID *pClassID)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    return IBaseFilter_GetClassID(This->nullRenderer, pClassID);
}

static HRESULT WINAPI SinkFilter_Stop(IBaseFilter *iface)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    return IBaseFilter_Stop(This->nullRenderer);
}

static HRESULT WINAPI SinkFilter_Pause(IBaseFilter *iface)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    return IBaseFilter_Pause(This->nullRenderer);
}

static HRESULT WINAPI SinkFilter_Run(IBaseFilter *iface, REFERENCE_TIME tStart)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    return IBaseFilter_Run(This->nullRenderer, tStart);
}

static HRESULT WINAPI SinkFilter_GetState(IBaseFilter *iface, DWORD dwMilliSecsTimeout, FILTER_STATE *state)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    return IBaseFilter_GetState(This->nullRenderer, dwMilliSecsTimeout, state);
}

static HRESULT WINAPI SinkFilter_SetSyncSource(IBaseFilter *iface, IReferenceClock *pClock)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    return IBaseFilter_SetSyncSource(This->nullRenderer, pClock);
}

static HRESULT WINAPI SinkFilter_GetSyncSource(IBaseFilter *iface, IReferenceClock **ppClock)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    return IBaseFilter_GetSyncSource(This->nullRenderer, ppClock);
}

static HRESULT WINAPI SinkFilter_EnumPins(IBaseFilter *iface, IEnumPins **ppEnum)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    SinkEnumPins *sinkEnumPins = create_SinkEnumPins(This);
    if (sinkEnumPins) {
        *ppEnum = &sinkEnumPins->IEnumPins_iface;
        return S_OK;
    }
    else
        return E_OUTOFMEMORY;
}

static HRESULT WINAPI SinkFilter_FindPin(IBaseFilter *iface, LPCWSTR id, IPin **ppPin)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    HRESULT hr = IBaseFilter_FindPin(This->nullRenderer, id, ppPin);
    if (SUCCEEDED(hr)) {
        IPin_Release(*ppPin);
        *ppPin = &This->IPin_iface;
        IPin_AddRef(&This->IPin_iface);
    }
    return hr;
}

static HRESULT WINAPI SinkFilter_QueryFilterInfo(IBaseFilter *iface, FILTER_INFO *pInfo)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    return IBaseFilter_QueryFilterInfo(This->nullRenderer, pInfo);
}

static HRESULT WINAPI SinkFilter_JoinFilterGraph(IBaseFilter *iface, IFilterGraph *pGraph, LPCWSTR pName)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    return IBaseFilter_JoinFilterGraph(This->nullRenderer, pGraph, pName);
}

static HRESULT WINAPI SinkFilter_QueryVendorInfo(IBaseFilter *iface, LPWSTR *pVendorInfo)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    return IBaseFilter_QueryVendorInfo(This->nullRenderer, pVendorInfo);
}

static const IBaseFilterVtbl SinkFilterVtbl = {
    SinkFilter_QueryInterface,
    SinkFilter_AddRef,
    SinkFilter_Release,
    SinkFilter_GetClassID,
    SinkFilter_Stop,
    SinkFilter_Pause,
    SinkFilter_Run,
    SinkFilter_GetState,
    SinkFilter_SetSyncSource,
    SinkFilter_GetSyncSource,
    SinkFilter_EnumPins,
    SinkFilter_FindPin,
    SinkFilter_QueryFilterInfo,
    SinkFilter_JoinFilterGraph,
    SinkFilter_QueryVendorInfo
};

static HRESULT WINAPI SinkEnumPins_QueryInterface(IEnumPins *iface, REFIID riid, void **ppv)
{
    SinkEnumPins *This = impl_from_SinkFilter_IEnumPins(iface);
    if(IsEqualIID(riid, &IID_IUnknown)) {
        *ppv = &This->IEnumPins_iface;
    } else if(IsEqualIID(riid, &IID_IEnumPins)) {
        *ppv = &This->IEnumPins_iface;
    } else {
        trace("no interface for %s\n", wine_dbgstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI SinkEnumPins_AddRef(IEnumPins *iface)
{
    SinkEnumPins *This = impl_from_SinkFilter_IEnumPins(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI SinkEnumPins_Release(IEnumPins *iface)
{
    SinkEnumPins *This = impl_from_SinkFilter_IEnumPins(iface);
    ULONG ref;
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
    {
        IBaseFilter_Release(&This->filter->IBaseFilter_iface);
        CoTaskMemFree(This);
    }
    return ref;
}

static HRESULT WINAPI SinkEnumPins_Next(IEnumPins *iface, ULONG cPins, IPin **ppPins, ULONG *pcFetched)
{
    SinkEnumPins *This = impl_from_SinkFilter_IEnumPins(iface);
    if (!ppPins)
        return E_POINTER;
    if (cPins > 1 && !pcFetched)
        return E_INVALIDARG;
    if (pcFetched)
        *pcFetched = 0;
    if (cPins == 0)
        return S_OK;
    if (This->index == 0) {
        ppPins[0] = &This->filter->IPin_iface;
        IPin_AddRef(&This->filter->IPin_iface);
        ++This->index;
        if (pcFetched)
            *pcFetched = 1;
        return S_OK;
    }
    return S_FALSE;
}

static HRESULT WINAPI SinkEnumPins_Skip(IEnumPins *iface, ULONG cPins)
{
    SinkEnumPins *This = impl_from_SinkFilter_IEnumPins(iface);
    if (This->index + cPins >= 1)
        return S_FALSE;
    This->index += cPins;
    return S_OK;
}

static HRESULT WINAPI SinkEnumPins_Reset(IEnumPins *iface)
{
    SinkEnumPins *This = impl_from_SinkFilter_IEnumPins(iface);
    This->index = 0;
    return S_OK;
}

static HRESULT WINAPI SinkEnumPins_Clone(IEnumPins *iface, IEnumPins **ppEnum)
{
    SinkEnumPins *This = impl_from_SinkFilter_IEnumPins(iface);
    SinkEnumPins *clone = create_SinkEnumPins(This->filter);
    if (clone == NULL)
        return E_OUTOFMEMORY;
    clone->index = This->index;
    *ppEnum = &clone->IEnumPins_iface;
    return S_OK;
}

static const IEnumPinsVtbl SinkEnumPinsVtbl = {
    SinkEnumPins_QueryInterface,
    SinkEnumPins_AddRef,
    SinkEnumPins_Release,
    SinkEnumPins_Next,
    SinkEnumPins_Skip,
    SinkEnumPins_Reset,
    SinkEnumPins_Clone
};

static SinkEnumPins* create_SinkEnumPins(SinkFilter *filter)
{
    SinkEnumPins *This;
    This = CoTaskMemAlloc(sizeof(*This));
    if (This == NULL) {
        return NULL;
    }
    This->IEnumPins_iface.lpVtbl = &SinkEnumPinsVtbl;
    This->ref = 1;
    This->index = 0;
    This->filter = filter;
    IBaseFilter_AddRef(&filter->IBaseFilter_iface);
    return This;
}

static HRESULT WINAPI SinkPin_QueryInterface(IPin *iface, REFIID riid, void **ppv)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    if(IsEqualIID(riid, &IID_IUnknown)) {
        *ppv = &This->IPin_iface;
    } else if(IsEqualIID(riid, &IID_IPin)) {
        *ppv = &This->IPin_iface;
    } else if(IsEqualIID(riid, &IID_IMemInputPin)) {
        *ppv = &This->IMemInputPin_iface;
    } else {
        trace("no interface for %s\n", wine_dbgstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI SinkPin_AddRef(IPin *iface)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IBaseFilter_AddRef(&This->IBaseFilter_iface);
}

static ULONG WINAPI SinkPin_Release(IPin *iface)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IBaseFilter_Release(&This->IBaseFilter_iface);
}

static HRESULT WINAPI SinkPin_Connect(IPin *iface, IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IPin_Connect(This->nullRendererPin, pReceivePin, pmt);
}

static HRESULT WINAPI SinkPin_ReceiveConnection(IPin *iface, IPin *connector, const AM_MEDIA_TYPE *pmt)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IPin_ReceiveConnection(This->nullRendererPin, connector, pmt);
}

static HRESULT WINAPI SinkPin_Disconnect(IPin *iface)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IPin_Disconnect(This->nullRendererPin);
}

static HRESULT WINAPI SinkPin_ConnectedTo(IPin *iface, IPin **pPin)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IPin_ConnectedTo(This->nullRendererPin, pPin);
}

static HRESULT WINAPI SinkPin_ConnectionMediaType(IPin *iface, AM_MEDIA_TYPE *pmt)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IPin_ConnectionMediaType(This->nullRendererPin, pmt);
}

static HRESULT WINAPI SinkPin_QueryPinInfo(IPin *iface, PIN_INFO *pInfo)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    HRESULT hr = IPin_QueryPinInfo(This->nullRendererPin, pInfo);
    if (SUCCEEDED(hr)) {
        IBaseFilter_Release(pInfo->pFilter);
        pInfo->pFilter = &This->IBaseFilter_iface;
        IBaseFilter_AddRef(&This->IBaseFilter_iface);
    }
    return hr;
}

static HRESULT WINAPI SinkPin_QueryDirection(IPin *iface, PIN_DIRECTION *pPinDir)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IPin_QueryDirection(This->nullRendererPin, pPinDir);
}

static HRESULT WINAPI SinkPin_QueryId(IPin *iface, LPWSTR *id)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IPin_QueryId(This->nullRendererPin, id);
}

static HRESULT WINAPI SinkPin_QueryAccept(IPin *iface, const AM_MEDIA_TYPE *pmt)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IPin_QueryAccept(This->nullRendererPin, pmt);
}

static HRESULT WINAPI SinkPin_EnumMediaTypes(IPin *iface, IEnumMediaTypes **ppEnum)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IPin_EnumMediaTypes(This->nullRendererPin, ppEnum);
}

static HRESULT WINAPI SinkPin_QueryInternalConnections(IPin *iface, IPin **apPin, ULONG *nPin)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IPin_QueryInternalConnections(This->nullRendererPin, apPin, nPin);
}

static HRESULT WINAPI SinkPin_EndOfStream(IPin *iface)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IPin_EndOfStream(This->nullRendererPin);
}

static HRESULT WINAPI SinkPin_BeginFlush(IPin *iface)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IPin_BeginFlush(This->nullRendererPin);
}

static HRESULT WINAPI SinkPin_EndFlush(IPin *iface)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IPin_EndFlush(This->nullRendererPin);
}

static HRESULT WINAPI SinkPin_NewSegment(IPin *iface, REFERENCE_TIME tStart,
        REFERENCE_TIME tStop, double dRate)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IPin_NewSegment(This->nullRendererPin, tStart, tStop, dRate);
}

static const IPinVtbl SinkPinVtbl = {
    SinkPin_QueryInterface,
    SinkPin_AddRef,
    SinkPin_Release,
    SinkPin_Connect,
    SinkPin_ReceiveConnection,
    SinkPin_Disconnect,
    SinkPin_ConnectedTo,
    SinkPin_ConnectionMediaType,
    SinkPin_QueryPinInfo,
    SinkPin_QueryDirection,
    SinkPin_QueryId,
    SinkPin_QueryAccept,
    SinkPin_EnumMediaTypes,
    SinkPin_QueryInternalConnections,
    SinkPin_EndOfStream,
    SinkPin_BeginFlush,
    SinkPin_EndFlush,
    SinkPin_NewSegment
};

static HRESULT WINAPI SinkMemInputPin_QueryInterface(IMemInputPin *iface, REFIID riid, void **ppv)
{
    SinkFilter *This = impl_from_SinkFilter_IMemInputPin(iface);
    return IPin_QueryInterface(&This->IPin_iface, riid, ppv);
}

static ULONG WINAPI SinkMemInputPin_AddRef(IMemInputPin *iface)
{
    SinkFilter *This = impl_from_SinkFilter_IMemInputPin(iface);
    return IBaseFilter_AddRef(&This->IBaseFilter_iface);
}

static ULONG WINAPI SinkMemInputPin_Release(IMemInputPin *iface)
{
    SinkFilter *This = impl_from_SinkFilter_IMemInputPin(iface);
    return IBaseFilter_Release(&This->IBaseFilter_iface);
}

static HRESULT WINAPI SinkMemInputPin_GetAllocator(IMemInputPin *iface, IMemAllocator **ppAllocator)
{
    SinkFilter *This = impl_from_SinkFilter_IMemInputPin(iface);
    ok(0, "SmartTeeFilter never calls IMemInputPin_GetAllocator()\n");
    return IMemInputPin_GetAllocator(This->nullRendererMemInputPin, ppAllocator);
}

static HRESULT WINAPI SinkMemInputPin_NotifyAllocator(IMemInputPin *iface, IMemAllocator *pAllocator,
        BOOL bReadOnly)
{
    SinkFilter *This = impl_from_SinkFilter_IMemInputPin(iface);
    This->allocator = pAllocator;
    IMemAllocator_AddRef(This->allocator);
    ok(bReadOnly, "bReadOnly isn't supposed to be FALSE\n");
    return IMemInputPin_NotifyAllocator(This->nullRendererMemInputPin, pAllocator, bReadOnly);
}

static HRESULT WINAPI SinkMemInputPin_GetAllocatorRequirements(IMemInputPin *iface,
        ALLOCATOR_PROPERTIES *pProps)
{
    SinkFilter *This = impl_from_SinkFilter_IMemInputPin(iface);
    ok(0, "SmartTeeFilter never calls IMemInputPin_GetAllocatorRequirements()\n");
    return IMemInputPin_GetAllocatorRequirements(This->nullRendererMemInputPin, pProps);
}

static HRESULT WINAPI SinkMemInputPin_Receive(IMemInputPin *iface, IMediaSample *pSample)
{
    LONG samplesProcessed;
    todo_wine ok(0, "SmartTeeFilter never calls IMemInputPin_Receive(), only IMemInputPin_ReceiveMultiple()\n");
    return IMemInputPin_ReceiveMultiple(iface, &pSample, 1, &samplesProcessed);
}

static HRESULT WINAPI SinkMemInputPin_ReceiveMultiple(IMemInputPin *iface, IMediaSample **pSamples,
        LONG nSamples, LONG *nSamplesProcessed)
{
    SinkFilter *This = impl_from_SinkFilter_IMemInputPin(iface);
    IMediaSample *pSample;
    REFERENCE_TIME startTime, endTime;
    HRESULT hr;
    ok(nSamples == 1, "expected 1 sample, got %d\n", nSamples);
    pSample = pSamples[0];
    hr = IMediaSample_GetTime(pSample, &startTime, &endTime);
    if (This->isCapture)
        ok(SUCCEEDED(hr), "IMediaSample_GetTime() from Capture pin failed, hr=0x%08x\n", hr);
    else
        ok(hr == VFW_E_SAMPLE_TIME_NOT_SET, "IMediaSample_GetTime() from Preview pin returned hr=0x%08x\n", hr);
    This->receiveThreadId = GetCurrentThreadId();
    SetEvent(event);
    return IMemInputPin_ReceiveMultiple(This->nullRendererMemInputPin, pSamples,
            nSamples, nSamplesProcessed);
}

static HRESULT WINAPI SinkMemInputPin_ReceiveCanBlock(IMemInputPin *iface)
{
    SinkFilter *This = impl_from_SinkFilter_IMemInputPin(iface);
    return IMemInputPin_ReceiveCanBlock(This->nullRendererMemInputPin);
}

static const IMemInputPinVtbl SinkMemInputPinVtbl = {
    SinkMemInputPin_QueryInterface,
    SinkMemInputPin_AddRef,
    SinkMemInputPin_Release,
    SinkMemInputPin_GetAllocator,
    SinkMemInputPin_NotifyAllocator,
    SinkMemInputPin_GetAllocatorRequirements,
    SinkMemInputPin_Receive,
    SinkMemInputPin_ReceiveMultiple,
    SinkMemInputPin_ReceiveCanBlock
};

static SinkFilter* create_SinkFilter(BOOL isCapture)
{
    SinkFilter *This = NULL;
    HRESULT hr;
    This = CoTaskMemAlloc(sizeof(*This));
    if (This) {
        memset(This, 0, sizeof(*This));
        This->IBaseFilter_iface.lpVtbl = &SinkFilterVtbl;
        This->ref = 1;
        This->isCapture = isCapture;
        This->IPin_iface.lpVtbl = &SinkPinVtbl;
        This->IMemInputPin_iface.lpVtbl = &SinkMemInputPinVtbl;
        hr = CoCreateInstance(&CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER,
                &IID_IBaseFilter, (LPVOID*)&This->nullRenderer);
        if (SUCCEEDED(hr)) {
            IEnumPins *enumPins = NULL;
            hr = IBaseFilter_EnumPins(This->nullRenderer, &enumPins);
            if (SUCCEEDED(hr)) {
                hr = IEnumPins_Next(enumPins, 1, &This->nullRendererPin, NULL);
                IEnumPins_Release(enumPins);
                if (SUCCEEDED(hr)) {
                    hr = IPin_QueryInterface(This->nullRendererPin, &IID_IMemInputPin,
                            (LPVOID*)&This->nullRendererMemInputPin);
                    if (SUCCEEDED(hr))
                        return This;
                    IPin_Release(This->nullRendererPin);
                }
            }
            IBaseFilter_Release(This->nullRenderer);
        }
        CoTaskMemFree(This);
    }
    return NULL;
}

typedef struct {
    IBaseFilter IBaseFilter_iface;
    LONG ref;
    IPin IPin_iface;
    IKsPropertySet IKsPropertySet_iface;
    CRITICAL_SECTION cs;
    FILTER_STATE state;
    IReferenceClock *referenceClock;
    FILTER_INFO filterInfo;
    AM_MEDIA_TYPE mediaType;
    VIDEOINFOHEADER videoInfo;
    WAVEFORMATEX audioInfo;
    IPin *connectedTo;
    IMemInputPin *memInputPin;
    IMemAllocator *allocator;
    DWORD mediaThreadId;
} SourceFilter;

typedef struct {
    IEnumPins IEnumPins_iface;
    LONG ref;
    ULONG index;
    SourceFilter *filter;
} SourceEnumPins;

typedef struct {
    IEnumMediaTypes IEnumMediaTypes_iface;
    LONG ref;
    ULONG index;
    SourceFilter *filter;
} SourceEnumMediaTypes;

static const WCHAR sourcePinName[] = {'C','a','p','t','u','r','e',0};

static SourceEnumPins* create_SourceEnumPins(SourceFilter *filter);
static SourceEnumMediaTypes* create_SourceEnumMediaTypes(SourceFilter *filter);

static inline SourceFilter* impl_from_SourceFilter_IBaseFilter(IBaseFilter *iface)
{
    return CONTAINING_RECORD(iface, SourceFilter, IBaseFilter_iface);
}

static inline SourceFilter* impl_from_SourceFilter_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, SourceFilter, IPin_iface);
}

static inline SourceFilter* impl_from_SourceFilter_IKsPropertySet(IKsPropertySet *iface)
{
    return CONTAINING_RECORD(iface, SourceFilter, IKsPropertySet_iface);
}

static inline SourceEnumPins* impl_from_SourceFilter_IEnumPins(IEnumPins *iface)
{
    return CONTAINING_RECORD(iface, SourceEnumPins, IEnumPins_iface);
}

static inline SourceEnumMediaTypes* impl_from_SourceFilter_IEnumMediaTypes(IEnumMediaTypes *iface)
{
    return CONTAINING_RECORD(iface, SourceEnumMediaTypes, IEnumMediaTypes_iface);
}

static HRESULT WINAPI SourceFilter_QueryInterface(IBaseFilter *iface, REFIID riid, void **ppv)
{
    SourceFilter *This = impl_from_SourceFilter_IBaseFilter(iface);
    if(IsEqualIID(riid, &IID_IUnknown)) {
        *ppv = &This->IBaseFilter_iface;
    } else if(IsEqualIID(riid, &IID_IPersist)) {
        *ppv = &This->IBaseFilter_iface;
    } else if(IsEqualIID(riid, &IID_IMediaFilter)) {
        *ppv = &This->IBaseFilter_iface;
    } else if(IsEqualIID(riid, &IID_IBaseFilter)) {
        *ppv = &This->IBaseFilter_iface;
    } else {
        trace("no interface for %s\n", wine_dbgstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI SourceFilter_AddRef(IBaseFilter *iface)
{
    SourceFilter *This = impl_from_SourceFilter_IBaseFilter(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI SourceFilter_Release(IBaseFilter *iface)
{
    SourceFilter *This = impl_from_SourceFilter_IBaseFilter(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    if(!ref) {
        if (This->referenceClock)
            IReferenceClock_Release(This->referenceClock);
        if (This->connectedTo)
            IPin_Disconnect(&This->IPin_iface);
        DeleteCriticalSection(&This->cs);
        CoTaskMemFree(This);
    }
    return ref;
}

static HRESULT WINAPI SourceFilter_GetClassID(IBaseFilter *iface, CLSID *pClassID)
{
    *pClassID = CLSID_VfwCapture;
    return S_OK;
}

static HRESULT WINAPI SourceFilter_Stop(IBaseFilter *iface)
{
    SourceFilter *This = impl_from_SourceFilter_IBaseFilter(iface);
    EnterCriticalSection(&This->cs);
    IMemAllocator_Decommit(This->allocator);
    This->state = State_Stopped;
    LeaveCriticalSection(&This->cs);
    return S_OK;
}

static HRESULT WINAPI SourceFilter_Pause(IBaseFilter *iface)
{
    SourceFilter *This = impl_from_SourceFilter_IBaseFilter(iface);
    EnterCriticalSection(&This->cs);
    This->state = State_Paused;
    LeaveCriticalSection(&This->cs);
    return S_OK;
}

static DWORD WINAPI media_thread(LPVOID param)
{
    SourceFilter *This = (SourceFilter*) param;
    HRESULT hr;
    IMediaSample *sample = NULL;
    REFERENCE_TIME startTime;
    REFERENCE_TIME endTime;
    BYTE *buffer;

    hr = IMemAllocator_GetBuffer(This->allocator, &sample, NULL, NULL, 0);
    ok(SUCCEEDED(hr), "IMemAllocator_GetBuffer() failed, hr=0x%08x\n", hr);
    if (SUCCEEDED(hr)) {
        startTime = 10;
        endTime = 20;
        hr = IMediaSample_SetTime(sample, &startTime, &endTime);
        ok(SUCCEEDED(hr), "IMediaSample_SetTime() failed, hr=0x%08x\n", hr);
        hr = IMediaSample_SetMediaType(sample, &This->mediaType);
        ok(SUCCEEDED(hr), "IMediaSample_SetMediaType() failed, hr=0x%08x\n", hr);

        hr = IMediaSample_GetPointer(sample, &buffer);
        ok(SUCCEEDED(hr), "IMediaSample_GetPointer() failed, hr=0x%08x\n", hr);
        if (SUCCEEDED(hr)) {
            /* 10 by 10 pixel 32 RGB */
            int i;
            for (i = 0; i < 100; i++)
                buffer[4*i] = i;
        }

        hr = IMemInputPin_Receive(This->memInputPin, sample);
        ok(SUCCEEDED(hr), "delivering sample to SmartTeeFilter's Input pin failed, hr=0x%08x\n", hr);

        IMediaSample_Release(sample);
    }
    return 0;
}

static HRESULT WINAPI SourceFilter_Run(IBaseFilter *iface, REFERENCE_TIME tStart)
{
    SourceFilter *This = impl_from_SourceFilter_IBaseFilter(iface);
    HRESULT hr;
    EnterCriticalSection(&This->cs);
    hr = IMemAllocator_Commit(This->allocator);
    if (SUCCEEDED(hr)) {
        HANDLE thread = CreateThread(NULL, 0, media_thread, This, 0, &This->mediaThreadId);
        ok(thread != NULL, "couldn't create media thread, GetLastError()=%u\n", GetLastError());
        if (thread != NULL) {
            CloseHandle(thread);
            This->state = State_Running;
        } else {
            IMemAllocator_Decommit(This->allocator);
            hr = E_FAIL;
        }
    }
    LeaveCriticalSection(&This->cs);
    return hr;
}

static HRESULT WINAPI SourceFilter_GetState(IBaseFilter *iface, DWORD dwMilliSecsTimeout, FILTER_STATE *state)
{
    SourceFilter *This = impl_from_SourceFilter_IBaseFilter(iface);
    EnterCriticalSection(&This->cs);
    *state = This->state;
    LeaveCriticalSection(&This->cs);
    return S_OK;
}

static HRESULT WINAPI SourceFilter_SetSyncSource(IBaseFilter *iface, IReferenceClock *pClock)
{
    SourceFilter *This = impl_from_SourceFilter_IBaseFilter(iface);
    EnterCriticalSection(&This->cs);
    if (This->referenceClock)
        IReferenceClock_Release(This->referenceClock);
    This->referenceClock = pClock;
    if (This->referenceClock)
        IReferenceClock_AddRef(This->referenceClock);
    LeaveCriticalSection(&This->cs);
    return S_OK;
}

static HRESULT WINAPI SourceFilter_GetSyncSource(IBaseFilter *iface, IReferenceClock **ppClock)
{
    SourceFilter *This = impl_from_SourceFilter_IBaseFilter(iface);
    EnterCriticalSection(&This->cs);
    *ppClock = This->referenceClock;
    if (This->referenceClock)
        IReferenceClock_AddRef(This->referenceClock);
    LeaveCriticalSection(&This->cs);
    return S_OK;
}

static HRESULT WINAPI SourceFilter_EnumPins(IBaseFilter *iface, IEnumPins **ppEnum)
{
    SourceFilter *This = impl_from_SourceFilter_IBaseFilter(iface);
    SourceEnumPins *sourceEnumPins = create_SourceEnumPins(This);
    if (sourceEnumPins) {
        *ppEnum = &sourceEnumPins->IEnumPins_iface;
        return S_OK;
    }
    else
        return E_OUTOFMEMORY;
}

static HRESULT WINAPI SourceFilter_FindPin(IBaseFilter *iface, LPCWSTR id, IPin **ppPin)
{
    SourceFilter *This = impl_from_SourceFilter_IBaseFilter(iface);
    if (ppPin == NULL)
        return E_POINTER;
    *ppPin = NULL;
    if (lstrcmpW(id, sourcePinName) == 0) {
        *ppPin = &This->IPin_iface;
        IPin_AddRef(&This->IPin_iface);
        return S_OK;
    }
    return VFW_E_NOT_FOUND;
}

static HRESULT WINAPI SourceFilter_QueryFilterInfo(IBaseFilter *iface, FILTER_INFO *pInfo)
{
    SourceFilter *This = impl_from_SourceFilter_IBaseFilter(iface);
    if (!pInfo)
        return E_POINTER;
    EnterCriticalSection(&This->cs);
    *pInfo = This->filterInfo;
    if (This->filterInfo.pGraph)
        IFilterGraph_AddRef(This->filterInfo.pGraph);
    LeaveCriticalSection(&This->cs);
    return S_OK;
}

static HRESULT WINAPI SourceFilter_JoinFilterGraph(IBaseFilter *iface, IFilterGraph *pGraph, LPCWSTR pName)
{
    SourceFilter *This = impl_from_SourceFilter_IBaseFilter(iface);
    EnterCriticalSection(&This->cs);
    if (pName)
        lstrcpyW(This->filterInfo.achName, pName);
    else
        This->filterInfo.achName[0] = 0;
    This->filterInfo.pGraph = pGraph;
    LeaveCriticalSection(&This->cs);
    return S_OK;
}

static HRESULT WINAPI SourceFilter_QueryVendorInfo(IBaseFilter *iface, LPWSTR *pVendorInfo)
{
    return E_NOTIMPL;
}

static const IBaseFilterVtbl SourceFilterVtbl = {
    SourceFilter_QueryInterface,
    SourceFilter_AddRef,
    SourceFilter_Release,
    SourceFilter_GetClassID,
    SourceFilter_Stop,
    SourceFilter_Pause,
    SourceFilter_Run,
    SourceFilter_GetState,
    SourceFilter_SetSyncSource,
    SourceFilter_GetSyncSource,
    SourceFilter_EnumPins,
    SourceFilter_FindPin,
    SourceFilter_QueryFilterInfo,
    SourceFilter_JoinFilterGraph,
    SourceFilter_QueryVendorInfo
};

static HRESULT WINAPI SourceEnumPins_QueryInterface(IEnumPins *iface, REFIID riid, void **ppv)
{
    SourceEnumPins *This = impl_from_SourceFilter_IEnumPins(iface);
    if(IsEqualIID(riid, &IID_IUnknown)) {
        *ppv = &This->IEnumPins_iface;
    } else if(IsEqualIID(riid, &IID_IEnumPins)) {
        *ppv = &This->IEnumPins_iface;
    } else {
        trace("no interface for %s\n", wine_dbgstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI SourceEnumPins_AddRef(IEnumPins *iface)
{
    SourceEnumPins *This = impl_from_SourceFilter_IEnumPins(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI SourceEnumPins_Release(IEnumPins *iface)
{
    SourceEnumPins *This = impl_from_SourceFilter_IEnumPins(iface);
    ULONG ref;
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
    {
        IBaseFilter_Release(&This->filter->IBaseFilter_iface);
        CoTaskMemFree(This);
    }
    return ref;
}

static HRESULT WINAPI SourceEnumPins_Next(IEnumPins *iface, ULONG cPins, IPin **ppPins, ULONG *pcFetched)
{
    SourceEnumPins *This = impl_from_SourceFilter_IEnumPins(iface);
    if (!ppPins)
        return E_POINTER;
    if (cPins > 1 && !pcFetched)
        return E_INVALIDARG;
    if (pcFetched)
        *pcFetched = 0;
    if (cPins == 0)
        return S_OK;
    if (This->index == 0) {
        ppPins[0] = &This->filter->IPin_iface;
        IPin_AddRef(&This->filter->IPin_iface);
        ++This->index;
        if (pcFetched)
            *pcFetched = 1;
        return S_OK;
    }
    return S_FALSE;
}

static HRESULT WINAPI SourceEnumPins_Skip(IEnumPins *iface, ULONG cPins)
{
    SourceEnumPins *This = impl_from_SourceFilter_IEnumPins(iface);
    if (This->index + cPins >= 1)
        return S_FALSE;
    This->index += cPins;
    return S_OK;
}

static HRESULT WINAPI SourceEnumPins_Reset(IEnumPins *iface)
{
    SourceEnumPins *This = impl_from_SourceFilter_IEnumPins(iface);
    This->index = 0;
    return S_OK;
}

static HRESULT WINAPI SourceEnumPins_Clone(IEnumPins *iface, IEnumPins **ppEnum)
{
    SourceEnumPins *This = impl_from_SourceFilter_IEnumPins(iface);
    SourceEnumPins *clone = create_SourceEnumPins(This->filter);
    if (clone == NULL)
        return E_OUTOFMEMORY;
    clone->index = This->index;
    *ppEnum = &clone->IEnumPins_iface;
    return S_OK;
}

static const IEnumPinsVtbl SourceEnumPinsVtbl = {
    SourceEnumPins_QueryInterface,
    SourceEnumPins_AddRef,
    SourceEnumPins_Release,
    SourceEnumPins_Next,
    SourceEnumPins_Skip,
    SourceEnumPins_Reset,
    SourceEnumPins_Clone
};

static SourceEnumPins* create_SourceEnumPins(SourceFilter *filter)
{
    SourceEnumPins *This;
    This = CoTaskMemAlloc(sizeof(*This));
    if (This == NULL) {
        return NULL;
    }
    This->IEnumPins_iface.lpVtbl = &SourceEnumPinsVtbl;
    This->ref = 1;
    This->index = 0;
    This->filter = filter;
    IBaseFilter_AddRef(&filter->IBaseFilter_iface);
    return This;
}

static HRESULT WINAPI SourceEnumMediaTypes_QueryInterface(IEnumMediaTypes *iface, REFIID riid, void **ppv)
{
    SourceEnumMediaTypes *This = impl_from_SourceFilter_IEnumMediaTypes(iface);
    if(IsEqualIID(riid, &IID_IUnknown)) {
        *ppv = &This->IEnumMediaTypes_iface;
    } else if(IsEqualIID(riid, &IID_IEnumMediaTypes)) {
        *ppv = &This->IEnumMediaTypes_iface;
    } else {
        trace("no interface for %s\n", wine_dbgstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI SourceEnumMediaTypes_AddRef(IEnumMediaTypes *iface)
{
    SourceEnumMediaTypes *This = impl_from_SourceFilter_IEnumMediaTypes(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI SourceEnumMediaTypes_Release(IEnumMediaTypes *iface)
{
    SourceEnumMediaTypes *This = impl_from_SourceFilter_IEnumMediaTypes(iface);
    ULONG ref;
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
    {
        IBaseFilter_Release(&This->filter->IBaseFilter_iface);
        CoTaskMemFree(This);
    }
    return ref;
}

static HRESULT WINAPI SourceEnumMediaTypes_Next(IEnumMediaTypes *iface, ULONG cMediaTypes, AM_MEDIA_TYPE **ppMediaTypes, ULONG *pcFetched)
{
    SourceEnumMediaTypes *This = impl_from_SourceFilter_IEnumMediaTypes(iface);
    if (!ppMediaTypes)
        return E_POINTER;
    if (cMediaTypes > 1 && !pcFetched)
        return E_INVALIDARG;
    if (pcFetched)
        *pcFetched = 0;
    if (cMediaTypes == 0)
        return S_OK;
    if (This->index == 0) {
        ppMediaTypes[0] = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
        if (ppMediaTypes[0]) {
            *ppMediaTypes[0] = This->filter->mediaType;
            ppMediaTypes[0]->pbFormat = CoTaskMemAlloc(This->filter->mediaType.cbFormat);
            if (ppMediaTypes[0]->pbFormat) {
                memcpy(ppMediaTypes[0]->pbFormat, This->filter->mediaType.pbFormat, This->filter->mediaType.cbFormat);
                ++This->index;
                if (pcFetched)
                    *pcFetched = 1;
                return S_OK;
            }
            CoTaskMemFree(ppMediaTypes[0]);
        }
        return E_OUTOFMEMORY;
    }
    return S_FALSE;
}

static HRESULT WINAPI SourceEnumMediaTypes_Skip(IEnumMediaTypes *iface, ULONG cMediaTypes)
{
    SourceEnumMediaTypes *This = impl_from_SourceFilter_IEnumMediaTypes(iface);
    This->index += cMediaTypes;
    if (This->index >= 1)
        return S_FALSE;
    return S_OK;
}

static HRESULT WINAPI SourceEnumMediaTypes_Reset(IEnumMediaTypes *iface)
{
    SourceEnumMediaTypes *This = impl_from_SourceFilter_IEnumMediaTypes(iface);
    This->index = 0;
    return S_OK;
}

static HRESULT WINAPI SourceEnumMediaTypes_Clone(IEnumMediaTypes *iface, IEnumMediaTypes **ppEnum)
{
    SourceEnumMediaTypes *This = impl_from_SourceFilter_IEnumMediaTypes(iface);
    SourceEnumMediaTypes *clone = create_SourceEnumMediaTypes(This->filter);
    if (clone == NULL)
        return E_OUTOFMEMORY;
    clone->index = This->index;
    *ppEnum = &clone->IEnumMediaTypes_iface;
    return S_OK;
}

static const IEnumMediaTypesVtbl SourceEnumMediaTypesVtbl = {
    SourceEnumMediaTypes_QueryInterface,
    SourceEnumMediaTypes_AddRef,
    SourceEnumMediaTypes_Release,
    SourceEnumMediaTypes_Next,
    SourceEnumMediaTypes_Skip,
    SourceEnumMediaTypes_Reset,
    SourceEnumMediaTypes_Clone
};

static SourceEnumMediaTypes* create_SourceEnumMediaTypes(SourceFilter *filter)
{
    SourceEnumMediaTypes *This;
    This = CoTaskMemAlloc(sizeof(*This));
    if (This == NULL) {
        return NULL;
    }
    This->IEnumMediaTypes_iface.lpVtbl = &SourceEnumMediaTypesVtbl;
    This->ref = 1;
    This->index = 0;
    This->filter = filter;
    IBaseFilter_AddRef(&filter->IBaseFilter_iface);
    return This;
}

static HRESULT WINAPI SourcePin_QueryInterface(IPin *iface, REFIID riid, void **ppv)
{
    SourceFilter *This = impl_from_SourceFilter_IPin(iface);
    if(IsEqualIID(riid, &IID_IUnknown)) {
        *ppv = &This->IPin_iface;
    } else if(IsEqualIID(riid, &IID_IPin)) {
        *ppv = &This->IPin_iface;
    } else if(IsEqualIID(riid, &IID_IKsPropertySet)) {
        *ppv = &This->IKsPropertySet_iface;
    } else {
        trace("no interface for %s\n", wine_dbgstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI SourcePin_AddRef(IPin *iface)
{
    SourceFilter *This = impl_from_SourceFilter_IPin(iface);
    return IBaseFilter_AddRef(&This->IBaseFilter_iface);
}

static ULONG WINAPI SourcePin_Release(IPin *iface)
{
    SourceFilter *This = impl_from_SourceFilter_IPin(iface);
    return IBaseFilter_Release(&This->IBaseFilter_iface);
}

static HRESULT WINAPI SourcePin_Connect(IPin *iface, IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    SourceFilter *This = impl_from_SourceFilter_IPin(iface);
    HRESULT hr;

    if (pmt && !IsEqualGUID(&pmt->majortype, &GUID_NULL) && !IsEqualGUID(&pmt->majortype, &MEDIATYPE_Video))
        return VFW_E_TYPE_NOT_ACCEPTED;
    if (pmt && !IsEqualGUID(&pmt->subtype, &GUID_NULL) && !IsEqualGUID(&pmt->subtype, &MEDIASUBTYPE_RGB32))
        return VFW_E_TYPE_NOT_ACCEPTED;
    if (pmt && !IsEqualGUID(&pmt->formattype, &GUID_NULL))
        return VFW_E_TYPE_NOT_ACCEPTED;
    hr = IPin_ReceiveConnection(pReceivePin, &This->IPin_iface, &This->mediaType);
    ok(SUCCEEDED(hr), "SmartTeeFilter's Input pin's IPin_ReceiveConnection() failed with 0x%08x\n", hr);
    if (SUCCEEDED(hr)) {
        EnterCriticalSection(&This->cs);
        hr = IPin_QueryInterface(pReceivePin, &IID_IMemInputPin, (void**)&This->memInputPin);
        if (SUCCEEDED(hr)) {
            hr = IMemInputPin_GetAllocator(This->memInputPin, &This->allocator);
            ok(SUCCEEDED(hr), "couldn't get allocator from SmartTeeFilter, hr=0x%08x\n", hr);
            if (SUCCEEDED(hr)) {
                ALLOCATOR_PROPERTIES requested, actual;
                ZeroMemory(&requested, sizeof(ALLOCATOR_PROPERTIES));
                IMemInputPin_GetAllocatorRequirements(This->memInputPin, &requested);
                if (requested.cBuffers < 3) requested.cBuffers = 3;
                if (requested.cbBuffer < 4096) requested.cbBuffer = 4096;
                if (requested.cbAlign < 1) requested.cbAlign = 1;
                if (requested.cbPrefix < 0) requested.cbPrefix = 0;
                hr = IMemAllocator_SetProperties(This->allocator, &requested, &actual);
                if (SUCCEEDED(hr)) {
                    hr = IMemInputPin_NotifyAllocator(This->memInputPin, This->allocator, FALSE);
                    if (SUCCEEDED(hr)) {
                        This->connectedTo = pReceivePin;
                        IPin_AddRef(pReceivePin);
                    }
                }
                if (FAILED(hr)) {
                    IMemAllocator_Release(This->allocator);
                    This->allocator = NULL;
                }
            }
            if (FAILED(hr)) {
                IMemInputPin_Release(This->memInputPin);
                This->memInputPin = NULL;
            }
        }
        LeaveCriticalSection(&This->cs);

        if (FAILED(hr))
            IPin_Disconnect(pReceivePin);
    }
    return hr;
}

static HRESULT WINAPI SourcePin_ReceiveConnection(IPin *iface, IPin *connector, const AM_MEDIA_TYPE *pmt)
{
    return E_UNEXPECTED;
}

static HRESULT WINAPI SourcePin_Disconnect(IPin *iface)
{
    SourceFilter *This = impl_from_SourceFilter_IPin(iface);
    HRESULT hr;
    EnterCriticalSection(&This->cs);
    if (This->connectedTo) {
        if (This->state == State_Stopped) {
            IMemAllocator_Release(This->allocator);
            This->allocator = NULL;
            IMemInputPin_Release(This->memInputPin);
            This->memInputPin = NULL;
            IPin_Release(This->connectedTo);
            This->connectedTo = NULL;
            hr = S_OK;
        }
        else
            hr = VFW_E_NOT_STOPPED;
    } else
        hr = S_FALSE;
    LeaveCriticalSection(&This->cs);
    return hr;
}

static HRESULT WINAPI SourcePin_ConnectedTo(IPin *iface, IPin **pPin)
{
    SourceFilter *This = impl_from_SourceFilter_IPin(iface);
    HRESULT hr;
    if (!pPin)
        return E_POINTER;
    EnterCriticalSection(&This->cs);
    if (This->connectedTo) {
        *pPin = This->connectedTo;
        IPin_AddRef(This->connectedTo);
        hr = S_OK;
    } else
        hr = VFW_E_NOT_CONNECTED;
    LeaveCriticalSection(&This->cs);
    return hr;
}

static HRESULT WINAPI SourcePin_ConnectionMediaType(IPin *iface, AM_MEDIA_TYPE *pmt)
{
    SourceFilter *This = impl_from_SourceFilter_IPin(iface);
    HRESULT hr;
    if (!pmt)
        return E_POINTER;
    EnterCriticalSection(&This->cs);
    if (This->connectedTo) {
        *pmt = This->mediaType;
        pmt->pbFormat = CoTaskMemAlloc(sizeof(This->videoInfo));
        if (pmt->pbFormat) {
            memcpy(pmt->pbFormat, &This->videoInfo, sizeof(This->videoInfo));
            hr = S_OK;
        } else {
            memset(pmt, 0, sizeof(*pmt));
            hr = E_OUTOFMEMORY;
        }
    } else {
        memset(pmt, 0, sizeof(*pmt));
        hr = VFW_E_NOT_CONNECTED;
    }
    LeaveCriticalSection(&This->cs);
    return hr;
}

static HRESULT WINAPI SourcePin_QueryPinInfo(IPin *iface, PIN_INFO *pInfo)
{
    SourceFilter *This = impl_from_SourceFilter_IPin(iface);
    if (!pInfo)
        return E_POINTER;
    lstrcpyW(pInfo->achName, sourcePinName);
    pInfo->dir = PINDIR_OUTPUT;
    pInfo->pFilter = &This->IBaseFilter_iface;
    IBaseFilter_AddRef(&This->IBaseFilter_iface);
    return S_OK;
}

static HRESULT WINAPI SourcePin_QueryDirection(IPin *iface, PIN_DIRECTION *pPinDir)
{
    if (!pPinDir)
        return E_POINTER;
    *pPinDir = PINDIR_OUTPUT;
    return S_OK;
}

static HRESULT WINAPI SourcePin_QueryId(IPin *iface, LPWSTR *id)
{
    if (!id)
        return E_POINTER;
    *id = CoTaskMemAlloc((lstrlenW(sourcePinName) + 1)*sizeof(WCHAR));
    if (*id) {
        lstrcpyW(*id, sourcePinName);
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

static HRESULT WINAPI SourcePin_QueryAccept(IPin *iface, const AM_MEDIA_TYPE *pmt)
{
    return S_OK;
}

static HRESULT WINAPI SourcePin_EnumMediaTypes(IPin *iface, IEnumMediaTypes **ppEnum)
{
    SourceFilter *This = impl_from_SourceFilter_IPin(iface);
    SourceEnumMediaTypes *sourceEnumMediaTypes = create_SourceEnumMediaTypes(This);
    if (sourceEnumMediaTypes) {
        *ppEnum = &sourceEnumMediaTypes->IEnumMediaTypes_iface;
        return S_OK;
    }
    else
        return E_OUTOFMEMORY;
}

static HRESULT WINAPI SourcePin_QueryInternalConnections(IPin *iface, IPin **apPin, ULONG *nPin)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI SourcePin_EndOfStream(IPin *iface)
{
    return E_UNEXPECTED;
}

static HRESULT WINAPI SourcePin_BeginFlush(IPin *iface)
{
    return E_UNEXPECTED;
}

static HRESULT WINAPI SourcePin_EndFlush(IPin *iface)
{
    return E_UNEXPECTED;
}

static HRESULT WINAPI SourcePin_NewSegment(IPin *iface, REFERENCE_TIME tStart,
        REFERENCE_TIME tStop, double dRate)
{
    return S_OK;
}

static const IPinVtbl SourcePinVtbl = {
    SourcePin_QueryInterface,
    SourcePin_AddRef,
    SourcePin_Release,
    SourcePin_Connect,
    SourcePin_ReceiveConnection,
    SourcePin_Disconnect,
    SourcePin_ConnectedTo,
    SourcePin_ConnectionMediaType,
    SourcePin_QueryPinInfo,
    SourcePin_QueryDirection,
    SourcePin_QueryId,
    SourcePin_QueryAccept,
    SourcePin_EnumMediaTypes,
    SourcePin_QueryInternalConnections,
    SourcePin_EndOfStream,
    SourcePin_BeginFlush,
    SourcePin_EndFlush,
    SourcePin_NewSegment
};

static HRESULT WINAPI SourceKSP_QueryInterface(IKsPropertySet *iface, REFIID riid, LPVOID *ppv)
{
    SourceFilter *This = impl_from_SourceFilter_IKsPropertySet(iface);
    return IPin_QueryInterface(&This->IPin_iface, riid, ppv);
}

static ULONG WINAPI SourceKSP_AddRef(IKsPropertySet *iface)
{
    SourceFilter *This = impl_from_SourceFilter_IKsPropertySet(iface);
    return IBaseFilter_AddRef(&This->IBaseFilter_iface);
}

static ULONG WINAPI SourceKSP_Release(IKsPropertySet *iface)
{
    SourceFilter *This = impl_from_SourceFilter_IKsPropertySet(iface);
    return IBaseFilter_Release(&This->IBaseFilter_iface);
}

static HRESULT WINAPI SourceKSP_Set(IKsPropertySet *iface, REFGUID guidPropSet, DWORD dwPropID,
        LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData)
{
    SourceFilter *This = impl_from_SourceFilter_IKsPropertySet(iface);
    trace("(%p)->(%s, %u, %p, %u, %p, %u): stub\n", This, wine_dbgstr_guid(guidPropSet),
            dwPropID, pInstanceData, cbInstanceData, pPropData, cbPropData);
    return E_NOTIMPL;
}

static HRESULT WINAPI SourceKSP_Get(IKsPropertySet *iface, REFGUID guidPropSet, DWORD dwPropID,
        LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData,
        DWORD cbPropData, DWORD *pcbReturned)
{
    SourceFilter *This = impl_from_SourceFilter_IKsPropertySet(iface);
    trace("(%p)->(%s, %u, %p, %u, %p, %u, %p)\n", This, wine_dbgstr_guid(guidPropSet),
            dwPropID, pInstanceData, cbInstanceData, pPropData, cbPropData, pcbReturned);
    if (IsEqualIID(guidPropSet, &AMPROPSETID_Pin)) {
        if (pcbReturned)
            *pcbReturned = sizeof(GUID);
        if (pPropData) {
            LPGUID guid = pPropData;
            if (cbPropData >= sizeof(GUID))
                *guid = PIN_CATEGORY_CAPTURE;
        } else {
            if (!pcbReturned)
                return E_POINTER;
        }
        return S_OK;
    }
    return E_PROP_SET_UNSUPPORTED;
}

static HRESULT WINAPI SourceKSP_QuerySupported(IKsPropertySet *iface, REFGUID guidPropSet,
                    DWORD dwPropID, DWORD *pTypeSupport)
{
    SourceFilter *This = impl_from_SourceFilter_IKsPropertySet(iface);
    trace("(%p)->(%s, %u, %p): stub\n", This, wine_dbgstr_guid(guidPropSet),
            dwPropID, pTypeSupport);
    return E_NOTIMPL;
}

static const IKsPropertySetVtbl SourceKSPVtbl =
{
   SourceKSP_QueryInterface,
   SourceKSP_AddRef,
   SourceKSP_Release,
   SourceKSP_Set,
   SourceKSP_Get,
   SourceKSP_QuerySupported
};

static SourceFilter* create_SourceFilter(void)
{
    SourceFilter *This = NULL;
    This = CoTaskMemAlloc(sizeof(*This));
    if (This) {
        memset(This, 0, sizeof(*This));
        This->IBaseFilter_iface.lpVtbl = &SourceFilterVtbl;
        This->ref = 1;
        This->IPin_iface.lpVtbl = &SourcePinVtbl;
        This->IKsPropertySet_iface.lpVtbl = &SourceKSPVtbl;
        InitializeCriticalSection(&This->cs);
        return This;
    }
    return NULL;
}

static SourceFilter* create_video_SourceFilter(void)
{
    SourceFilter *This = create_SourceFilter();
    if (!This)
        return NULL;
    This->mediaType.majortype = MEDIATYPE_Video;
    This->mediaType.subtype = MEDIASUBTYPE_RGB32;
    This->mediaType.bFixedSizeSamples = FALSE;
    This->mediaType.bTemporalCompression = FALSE;
    This->mediaType.lSampleSize = 0;
    This->mediaType.formattype = FORMAT_VideoInfo;
    This->mediaType.pUnk = NULL;
    This->mediaType.cbFormat = sizeof(VIDEOINFOHEADER);
    This->mediaType.pbFormat = (BYTE*) &This->videoInfo;
    This->videoInfo.dwBitRate = 1000000;
    This->videoInfo.dwBitErrorRate = 0;
    This->videoInfo.AvgTimePerFrame = 400000;
    This->videoInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    This->videoInfo.bmiHeader.biWidth = 10;
    This->videoInfo.bmiHeader.biHeight = 10;
    This->videoInfo.bmiHeader.biPlanes = 1;
    This->videoInfo.bmiHeader.biBitCount = 32;
    This->videoInfo.bmiHeader.biCompression = BI_RGB;
    This->videoInfo.bmiHeader.biSizeImage = 0;
    This->videoInfo.bmiHeader.biXPelsPerMeter = 96;
    This->videoInfo.bmiHeader.biYPelsPerMeter = 96;
    This->videoInfo.bmiHeader.biClrUsed = 0;
    This->videoInfo.bmiHeader.biClrImportant = 0;
    return This;
}

static SourceFilter* create_audio_SourceFilter(void)
{
    SourceFilter *This = create_SourceFilter();
    if (!This)
        return NULL;
    This->mediaType.majortype = MEDIATYPE_Audio;
    This->mediaType.subtype = MEDIASUBTYPE_PCM;
    This->mediaType.bFixedSizeSamples = FALSE;
    This->mediaType.bTemporalCompression = FALSE;
    This->mediaType.lSampleSize = 0;
    This->mediaType.formattype = FORMAT_WaveFormatEx;
    This->mediaType.pUnk = NULL;
    This->mediaType.cbFormat = sizeof(WAVEFORMATEX);
    This->mediaType.pbFormat = (BYTE*) &This->audioInfo;
    This->audioInfo.wFormatTag = WAVE_FORMAT_PCM;
    This->audioInfo.nChannels = 1;
    This->audioInfo.nSamplesPerSec = 8000;
    This->audioInfo.nAvgBytesPerSec = 16000;
    This->audioInfo.nBlockAlign = 2;
    This->audioInfo.wBitsPerSample = 16;
    This->audioInfo.cbSize = 0;
    return This;
}

static void test_smart_tee_filter_in_graph(IBaseFilter *smartTeeFilter, IPin *inputPin,
        IPin *capturePin, IPin *previewPin)
{
    HRESULT hr;
    IGraphBuilder *graphBuilder = NULL;
    IMediaControl *mediaControl = NULL;
    SourceFilter *sourceFilter = NULL;
    SinkFilter *captureSinkFilter = NULL;
    SinkFilter *previewSinkFilter = NULL;
    DWORD endTime;

    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IGraphBuilder,
            (LPVOID*)&graphBuilder);
    ok(SUCCEEDED(hr), "couldn't create graph builder, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;

    hr = IGraphBuilder_AddFilter(graphBuilder, smartTeeFilter, NULL);
    ok(SUCCEEDED(hr), "couldn't add smart tee filter to graph, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;

    captureSinkFilter = create_SinkFilter(TRUE);
    if (captureSinkFilter == NULL) {
        skip("couldn't create capture sink filter\n");
        goto end;
    }
    hr = IGraphBuilder_AddFilter(graphBuilder, &captureSinkFilter->IBaseFilter_iface, NULL);
    if (FAILED(hr)) {
        skip("couldn't add capture sink filter to graph, hr=0x%08x\n", hr);
        goto end;
    }

    previewSinkFilter = create_SinkFilter(FALSE);
    if (previewSinkFilter == NULL) {
        skip("couldn't create preview sink filter\n");
        goto end;
    }
    hr = IGraphBuilder_AddFilter(graphBuilder, &previewSinkFilter->IBaseFilter_iface, NULL);
    if (FAILED(hr)) {
        skip("couldn't add preview sink filter to graph, hr=0x%08x\n", hr);
        goto end;
    }

    hr = IGraphBuilder_Connect(graphBuilder, capturePin, &captureSinkFilter->IPin_iface);
    ok(hr == VFW_E_NOT_CONNECTED, "connecting Capture pin without first connecting Input pin returned 0x%08x\n", hr);
    hr = IGraphBuilder_Connect(graphBuilder, previewPin, &previewSinkFilter->IPin_iface);
    ok(hr == VFW_E_NOT_CONNECTED, "connecting Preview pin without first connecting Input pin returned 0x%08x\n", hr);

    sourceFilter = create_video_SourceFilter();
    if (sourceFilter == NULL) {
        skip("couldn't create source filter\n");
        goto end;
    }
    hr = IGraphBuilder_AddFilter(graphBuilder, &sourceFilter->IBaseFilter_iface, NULL);
    ok(SUCCEEDED(hr), "couldn't add source filter to graph, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;

    hr = IGraphBuilder_Connect(graphBuilder, &sourceFilter->IPin_iface, inputPin);
    ok(SUCCEEDED(hr), "couldn't connect source filter to Input pin, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;
    hr = IGraphBuilder_Connect(graphBuilder, capturePin, &captureSinkFilter->IPin_iface);
    ok(SUCCEEDED(hr), "couldn't connect Capture pin to sink, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;
    hr = IGraphBuilder_Connect(graphBuilder, previewPin, &previewSinkFilter->IPin_iface);
    ok(SUCCEEDED(hr), "couldn't connect Preview pin to sink, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;

    ok(sourceFilter->allocator == captureSinkFilter->allocator, "input and capture allocators don't match\n");
    ok(sourceFilter->allocator == previewSinkFilter->allocator, "input and preview allocators don't match\n");

    hr = IGraphBuilder_QueryInterface(graphBuilder, &IID_IMediaControl, (void**)&mediaControl);
    ok(SUCCEEDED(hr), "couldn't get IMediaControl interface from IGraphBuilder, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;
    hr = IMediaControl_Run(mediaControl);
    ok(SUCCEEDED(hr), "IMediaControl_Run() failed, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;

    endTime = GetTickCount() + 5000;
    while (previewSinkFilter->receiveThreadId == 0 || captureSinkFilter->receiveThreadId == 0) {
        DWORD now = GetTickCount();
        if (now < endTime)
            WaitForSingleObject(event, endTime - now);
        else
            break;
    }
    if (previewSinkFilter->receiveThreadId != 0 && captureSinkFilter->receiveThreadId != 0) {
        todo_wine ok(sourceFilter->mediaThreadId != captureSinkFilter->receiveThreadId,
                "sending thread should != capture receiving thread\n");
        todo_wine ok(sourceFilter->mediaThreadId != previewSinkFilter->receiveThreadId,
                "sending thread should != preview receiving thread\n");
        todo_wine ok(captureSinkFilter->receiveThreadId != previewSinkFilter->receiveThreadId,
                "capture receiving thread should != preview receiving thread\n");
    } else {
        ok(0, "timeout: threads did not receive sample in time\n");
    }

    IMediaControl_Stop(mediaControl);

end:
    if (mediaControl)
        IMediaControl_Release(mediaControl);
    if (graphBuilder)
        IGraphBuilder_Release(graphBuilder);
    if (sourceFilter)
        IBaseFilter_Release(&sourceFilter->IBaseFilter_iface);
    if (captureSinkFilter)
        IBaseFilter_Release(&captureSinkFilter->IBaseFilter_iface);
    if (previewSinkFilter)
        IBaseFilter_Release(&previewSinkFilter->IBaseFilter_iface);
}

static void test_smart_tee_filter(void)
{
    HRESULT hr;
    IBaseFilter *smartTeeFilter = NULL;
    IEnumPins *enumPins = NULL;
    IPin *pin;
    IPin *inputPin = NULL;
    IPin *capturePin = NULL;
    IPin *previewPin = NULL;
    FILTER_INFO filterInfo;
    int pinNumber = 0;
    IMemInputPin *memInputPin = NULL;
    IEnumMediaTypes *enumMediaTypes = NULL;

    hr = CoCreateInstance(&CLSID_SmartTee, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void**)&smartTeeFilter);
    ok(SUCCEEDED(hr), "couldn't create smart tee filter, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;

    hr = IBaseFilter_QueryFilterInfo(smartTeeFilter, &filterInfo);
    ok(SUCCEEDED(hr), "QueryFilterInfo failed, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;

    ok(lstrlenW(filterInfo.achName) == 0,
            "filter's name is meant to be empty but it's %s\n", wine_dbgstr_w(filterInfo.achName));

    hr = IBaseFilter_EnumPins(smartTeeFilter, &enumPins);
    ok(SUCCEEDED(hr), "cannot enum filter pins, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;

    while (IEnumPins_Next(enumPins, 1, &pin, NULL) == S_OK)
    {
        if (pinNumber == 0)
        {
            inputPin = pin;
            IPin_AddRef(inputPin);
        }
        else if (pinNumber == 1)
        {
            capturePin = pin;
            IPin_AddRef(capturePin);
        }
        else if (pinNumber == 2)
        {
            previewPin = pin;
            IPin_AddRef(previewPin);
        }
        else
            ok(0, "pin %d isn't supposed to exist\n", pinNumber);

        IPin_Release(pin);
        pinNumber++;
    }

    ok(inputPin && capturePin && previewPin, "couldn't find all pins\n");
    if (!(inputPin && capturePin && previewPin))
        goto end;

    hr = IPin_QueryInterface(inputPin, &IID_IMemInputPin, (void**)&memInputPin);
    ok(SUCCEEDED(hr), "couldn't get mem input pin, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;
    hr = IMemInputPin_ReceiveCanBlock(memInputPin);
    ok(hr == S_OK, "unexpected IMemInputPin_ReceiveCanBlock() = 0x%08x\n", hr);

    hr = IPin_EnumMediaTypes(inputPin, &enumMediaTypes);
    ok(SUCCEEDED(hr), "IPin_EnumMediaTypes() failed, hr=0x%08x\n", hr);
    if (SUCCEEDED(hr)) {
        AM_MEDIA_TYPE *mediaType = NULL;
        hr = IEnumMediaTypes_Next(enumMediaTypes, 1, &mediaType, NULL);
        ok(hr == S_FALSE, "the media types are non-empty\n");
    }
    IEnumMediaTypes_Release(enumMediaTypes);
    enumMediaTypes = NULL;
    hr = IPin_EnumMediaTypes(capturePin, &enumMediaTypes);
    ok(hr == VFW_E_NOT_CONNECTED, "IPin_EnumMediaTypes() failed, hr=0x%08x\n", hr);
    hr = IPin_EnumMediaTypes(previewPin, &enumMediaTypes);
    ok(hr == VFW_E_NOT_CONNECTED, "IPin_EnumMediaTypes() failed, hr=0x%08x\n", hr);

    test_smart_tee_filter_in_graph(smartTeeFilter, inputPin, capturePin, previewPin);

end:
    if (inputPin)
        IPin_Release(inputPin);
    if (capturePin)
        IPin_Release(capturePin);
    if (previewPin)
        IPin_Release(previewPin);
    if (smartTeeFilter)
        IBaseFilter_Release(smartTeeFilter);
    if (enumPins)
        IEnumPins_Release(enumPins);
    if (memInputPin)
        IMemInputPin_Release(memInputPin);
    if (enumMediaTypes)
        IEnumMediaTypes_Release(enumMediaTypes);
}

static void test_smart_tee_filter_aggregation(void)
{
    SourceFilter *sourceFilter = create_video_SourceFilter();
    if (sourceFilter) {
        IUnknown *unknown = NULL;
        HRESULT hr = CoCreateInstance(&CLSID_SmartTee, (IUnknown*)&sourceFilter->IBaseFilter_iface,
                CLSCTX_INPROC_SERVER, &IID_IUnknown, (void**)&unknown);
        ok(SUCCEEDED(hr), "SmartTee filter doesn't support aggregation, hr=0x%08x\n", hr);
        if (unknown)
            IUnknown_Release(unknown);
        IBaseFilter_Release(&sourceFilter->IBaseFilter_iface);
    } else
        ok(0, "out of memory allocating SourceFilter for test\n");
}

static HRESULT get_connected_filter_classid(IPin *pin, GUID *guid)
{
    IPin *connectedPin = NULL;
    PIN_INFO connectedPinInfo;
    HRESULT hr = IPin_ConnectedTo(pin, &connectedPin);
    ok(SUCCEEDED(hr), "IPin_ConnectedTo() failed, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;
    hr = IPin_QueryPinInfo(connectedPin, &connectedPinInfo);
    ok(SUCCEEDED(hr), "IPin_QueryPinInfo() failed, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;
    if (connectedPinInfo.pFilter) {
        hr = IBaseFilter_GetClassID(connectedPinInfo.pFilter, guid);
        ok(SUCCEEDED(hr), "IBaseFilter_GetClassID() failed, hr=0x%08x\n", hr);
        IBaseFilter_Release(connectedPinInfo.pFilter);
    }
end:
    if (connectedPin)
        IPin_Release(connectedPin);
    return hr;
}

static void test_audio_preview(ICaptureGraphBuilder2 *captureGraphBuilder, IGraphBuilder *graphBuilder,
        SourceFilter *audioSource, IBaseFilter *nullRenderer)
{
    GUID clsid;
    HRESULT hr = ICaptureGraphBuilder2_RenderStream(captureGraphBuilder, &PIN_CATEGORY_PREVIEW, &MEDIATYPE_Audio,
                (IUnknown*)&audioSource->IBaseFilter_iface, NULL, nullRenderer);
    ok(hr == VFW_S_NOPREVIEWPIN, "ICaptureGraphBuilder2_RenderStream() returned hr=0x%08x\n", hr);
    hr = get_connected_filter_classid(&audioSource->IPin_iface, &clsid);
    if (FAILED(hr))
        return;
    ok(IsEqualIID(&clsid, &CLSID_SmartTee), "unexpected connected filter %s\n",
            wine_dbgstr_guid(&clsid));
}

static void test_audio_capture(ICaptureGraphBuilder2 *captureGraphBuilder, IGraphBuilder *graphBuilder,
        SourceFilter *audioSource, IBaseFilter *nullRenderer)
{
    GUID clsid;
    HRESULT hr = ICaptureGraphBuilder2_RenderStream(captureGraphBuilder, &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio,
                (IUnknown*)&audioSource->IBaseFilter_iface, NULL, nullRenderer);
    ok(hr == S_OK, "ICaptureGraphBuilder2_RenderStream() returned hr=0x%08x\n", hr);
    hr = get_connected_filter_classid(&audioSource->IPin_iface, &clsid);
    if (FAILED(hr))
        return;
    ok(IsEqualIID(&clsid, &CLSID_SmartTee), "unexpected connected filter %s\n",
            wine_dbgstr_guid(&clsid));
}

static void test_video_preview(ICaptureGraphBuilder2 *captureGraphBuilder, IGraphBuilder *graphBuilder,
        SourceFilter *videoSource, IBaseFilter *nullRenderer)
{
    GUID clsid;
    HRESULT hr = ICaptureGraphBuilder2_RenderStream(captureGraphBuilder, &PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video,
                (IUnknown*)&videoSource->IBaseFilter_iface, NULL, nullRenderer);
    ok(hr == VFW_S_NOPREVIEWPIN, "ICaptureGraphBuilder2_RenderStream() failed, hr=0x%08x\n", hr);
    hr = get_connected_filter_classid(&videoSource->IPin_iface, &clsid);
    if (FAILED(hr))
        return;
    ok(IsEqualIID(&clsid, &CLSID_SmartTee), "unexpected connected filter %s\n",
            wine_dbgstr_guid(&clsid));
}

static void test_video_capture(ICaptureGraphBuilder2 *captureGraphBuilder, IGraphBuilder *graphBuilder,
        SourceFilter *videoSource, IBaseFilter *nullRenderer)
{
    GUID clsid;
    HRESULT hr = ICaptureGraphBuilder2_RenderStream(captureGraphBuilder, &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
                (IUnknown*)&videoSource->IBaseFilter_iface, NULL, nullRenderer);
    ok(hr == S_OK, "ICaptureGraphBuilder2_RenderStream() failed, hr=0x%08x\n", hr);
    hr = get_connected_filter_classid(&videoSource->IPin_iface, &clsid);
    if (FAILED(hr))
        return;
    ok(IsEqualIID(&clsid, &CLSID_SmartTee), "unexpected connected filter %s\n",
            wine_dbgstr_guid(&clsid));
}

static void test_audio_smart_tee_filter_auto_insertion(
        void (*test_function)(ICaptureGraphBuilder2 *cgb, IGraphBuilder *gb,
                SourceFilter *audioSource, IBaseFilter *nullRenderer))
{
    HRESULT hr;
    ICaptureGraphBuilder2 *captureGraphBuilder = NULL;
    IGraphBuilder *graphBuilder = NULL;
    IBaseFilter *nullRenderer = NULL;
    SourceFilter *audioSource = NULL;

    hr = CoCreateInstance(&CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER,
            &IID_ICaptureGraphBuilder2, (void**)&captureGraphBuilder);
    ok(SUCCEEDED(hr), "couldn't create capture graph builder, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;

    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IGraphBuilder,
            (LPVOID*)&graphBuilder);
    ok(SUCCEEDED(hr), "couldn't create graph builder, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;

    hr = ICaptureGraphBuilder2_SetFiltergraph(captureGraphBuilder, graphBuilder);
    ok(SUCCEEDED(hr), "ICaptureGraphBuilder2_SetFilterGraph() failed, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;

    hr = CoCreateInstance(&CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (LPVOID*)&nullRenderer);
    ok(SUCCEEDED(hr) ||
            /* Windows 2008: http://stackoverflow.com/questions/29410348/initialize-nullrender-failed-with-error-regdb-e-classnotreg-on-win2008-r2 */
            broken(hr == REGDB_E_CLASSNOTREG), "couldn't create NullRenderer, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;
    hr = IGraphBuilder_AddFilter(graphBuilder, nullRenderer, NULL);
    ok(SUCCEEDED(hr), "IGraphBuilder_AddFilter() failed, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;

    audioSource = create_audio_SourceFilter();
    ok(audioSource != NULL, "couldn't create audio source\n");
    if (audioSource == NULL)
        goto end;
    hr = IGraphBuilder_AddFilter(graphBuilder, &audioSource->IBaseFilter_iface, NULL);
    ok(SUCCEEDED(hr), "IGraphBuilder_AddFilter() failed, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;

    test_function(captureGraphBuilder, graphBuilder, audioSource, nullRenderer);

end:
    if (nullRenderer)
        IBaseFilter_Release(nullRenderer);
    if (audioSource)
        IBaseFilter_Release(&audioSource->IBaseFilter_iface);
    if (captureGraphBuilder)
        ICaptureGraphBuilder2_Release(captureGraphBuilder);
    if (graphBuilder)
        IGraphBuilder_Release(graphBuilder);
}

static void test_video_smart_tee_filter_auto_insertion(
        void (*test_function)(ICaptureGraphBuilder2 *cgb, IGraphBuilder *gb,
                SourceFilter *videoSource, IBaseFilter *nullRenderer))
{
    HRESULT hr;
    ICaptureGraphBuilder2 *captureGraphBuilder = NULL;
    IGraphBuilder *graphBuilder = NULL;
    IBaseFilter *nullRenderer = NULL;
    SourceFilter *videoSource = NULL;

    hr = CoCreateInstance(&CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER,
            &IID_ICaptureGraphBuilder2, (void**)&captureGraphBuilder);
    ok(SUCCEEDED(hr), "couldn't create capture graph builder, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;

    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IGraphBuilder,
            (LPVOID*)&graphBuilder);
    ok(SUCCEEDED(hr), "couldn't create graph builder, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;

    hr = ICaptureGraphBuilder2_SetFiltergraph(captureGraphBuilder, graphBuilder);
    ok(SUCCEEDED(hr), "ICaptureGraphBuilder2_SetFilterGraph() failed, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;

    hr = CoCreateInstance(&CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (LPVOID*)&nullRenderer);
    ok(SUCCEEDED(hr) ||
            /* Windows 2008: http://stackoverflow.com/questions/29410348/initialize-nullrender-failed-with-error-regdb-e-classnotreg-on-win2008-r2 */
            broken(hr == REGDB_E_CLASSNOTREG), "couldn't create NullRenderer, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;
    hr = IGraphBuilder_AddFilter(graphBuilder, nullRenderer, NULL);
    ok(SUCCEEDED(hr), "IGraphBuilder_AddFilter() failed, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;

    videoSource = create_video_SourceFilter();
    ok(videoSource != NULL, "couldn't create audio source\n");
    if (videoSource == NULL)
        goto end;
    hr = IGraphBuilder_AddFilter(graphBuilder, &videoSource->IBaseFilter_iface, NULL);
    ok(SUCCEEDED(hr), "IGraphBuilder_AddFilter() failed, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;

    test_function(captureGraphBuilder, graphBuilder, videoSource, nullRenderer);

end:
    if (nullRenderer)
        IBaseFilter_Release(nullRenderer);
    if (videoSource)
        IBaseFilter_Release(&videoSource->IBaseFilter_iface);
    if (captureGraphBuilder)
        ICaptureGraphBuilder2_Release(captureGraphBuilder);
    if (graphBuilder)
        IGraphBuilder_Release(graphBuilder);
}

START_TEST(smartteefilter)
{
    CoInitialize(NULL);

    event = CreateEventW(NULL, FALSE, FALSE, NULL);

    test_interfaces();
    test_enum_pins();
    test_find_pin();
    test_pin_info();
    test_enum_media_types();

    test_smart_tee_filter_aggregation();
    test_smart_tee_filter();

    test_audio_smart_tee_filter_auto_insertion(test_audio_preview);
    test_audio_smart_tee_filter_auto_insertion(test_audio_capture);

    test_video_smart_tee_filter_auto_insertion(test_video_preview);
    test_video_smart_tee_filter_auto_insertion(test_video_capture);

    CloseHandle(event);
    CoUninitialize();
}
