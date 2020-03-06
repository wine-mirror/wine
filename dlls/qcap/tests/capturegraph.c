/*
 * Capture graph builder unit tests
 *
 * Copyright 2019 Zebediah Figura
 * Copyright 2020 Zebediah Figura for CodeWeavers
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
#include "wine/strmbase.h"
#include "wine/test.h"

static ICaptureGraphBuilder2 *create_capture_graph(void)
{
    ICaptureGraphBuilder2 *ret;
    HRESULT hr = CoCreateInstance(&CLSID_CaptureGraphBuilder2, NULL,
            CLSCTX_INPROC_SERVER, &IID_ICaptureGraphBuilder2, (void **)&ret);
    ok(hr == S_OK, "Failed to create capture graph builder, hr %#x.\n", hr);
    return ret;
}

static const GUID testiid = {0x11111111}, testtype = {0x22222222};

struct testfilter
{
    struct strmbase_filter filter;
    struct strmbase_source source, source2;
    struct strmbase_sink sink, sink2;
    BOOL filter_has_iface, source_has_iface, source2_has_iface, sink_has_iface, sink2_has_iface;
    IKsPropertySet IKsPropertySet_iface;
};

static inline struct testfilter *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, filter);
}

static HRESULT testfilter_query_interface(struct strmbase_filter *iface, REFIID iid, void **out)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface);

    ok(!IsEqualGUID(iid, &IID_IKsPropertySet), "Unexpected query for IKsPropertySet.\n");

    if (filter->filter_has_iface && IsEqualGUID(iid, &testiid))
        *out = &iface->IBaseFilter_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static struct strmbase_pin *testfilter_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface);

    if (!index)
        return &filter->source.pin;
    else if (index == 1)
        return &filter->sink.pin;
    else if (index == 2)
        return &filter->source2.pin;
    else if (index == 3)
        return &filter->sink2.pin;
    return NULL;
}

static void testfilter_destroy(struct strmbase_filter *iface)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface);
    strmbase_source_cleanup(&filter->source);
    strmbase_sink_cleanup(&filter->sink);
    strmbase_filter_cleanup(&filter->filter);
}

static const struct strmbase_filter_ops testfilter_ops =
{
    .filter_query_interface = testfilter_query_interface,
    .filter_get_pin = testfilter_get_pin,
    .filter_destroy = testfilter_destroy,
};

static struct testfilter *impl_from_IKsPropertySet(IKsPropertySet *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, IKsPropertySet_iface);
}

static HRESULT WINAPI property_set_QueryInterface(IKsPropertySet *iface, REFIID iid, void **out)
{
    struct testfilter *filter = impl_from_IKsPropertySet(iface);
    return IPin_QueryInterface(&filter->source.pin.IPin_iface, iid, out);
}

static ULONG WINAPI property_set_AddRef(IKsPropertySet *iface)
{
    struct testfilter *filter = impl_from_IKsPropertySet(iface);
    return IPin_AddRef(&filter->source.pin.IPin_iface);
}

static ULONG WINAPI property_set_Release(IKsPropertySet *iface)
{
    struct testfilter *filter = impl_from_IKsPropertySet(iface);
    return IPin_Release(&filter->source.pin.IPin_iface);
}

static HRESULT WINAPI property_set_Set(IKsPropertySet *iface, REFGUID set, DWORD id,
        void *instance_data, DWORD instance_size, void *property_data, DWORD property_size)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI property_set_Get(IKsPropertySet *iface, REFGUID set, DWORD id,
        void *instance_data, DWORD instance_size, void *property_data, DWORD property_size, DWORD *ret_size)
{
    if (winetest_debug > 1) trace("Get()\n");

    ok(IsEqualGUID(set, &AMPROPSETID_Pin), "Got set %s.\n", debugstr_guid(set));
    ok(id == AMPROPERTY_PIN_CATEGORY, "Got id %#x.\n", id);
    ok(!instance_data, "Got instance data %p.\n", instance_data);
    ok(!instance_size, "Got instance size %u.\n", instance_size);
    ok(property_size == sizeof(GUID), "Got property size %u.\n", property_size);
    ok(!!ret_size, "Expected non-NULL return size.\n");
    memcpy(property_data, &PIN_CATEGORY_CAPTURE, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI property_set_QuerySupported(IKsPropertySet *iface, REFGUID set, DWORD id, DWORD *flags)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IKsPropertySetVtbl property_set_vtbl =
{
    property_set_QueryInterface,
    property_set_AddRef,
    property_set_Release,
    property_set_Set,
    property_set_Get,
    property_set_QuerySupported,
};

static HRESULT testsource_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface->filter);

    if (iface == &filter->source.pin && filter->source_has_iface && IsEqualGUID(iid, &testiid))
        *out = &iface->IPin_iface;
    else if (iface == &filter->source2.pin && filter->source2_has_iface && IsEqualGUID(iid, &testiid))
        *out = &iface->IPin_iface;
    else if (iface == &filter->source.pin && filter->IKsPropertySet_iface.lpVtbl
            && IsEqualGUID(iid, &IID_IKsPropertySet))
        *out = &filter->IKsPropertySet_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT testsource_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    return S_OK;
}

static HRESULT testsource_get_media_type(struct strmbase_pin *iface, unsigned int index, AM_MEDIA_TYPE *mt)
{
    if (!index)
    {
        memset(mt, 0, sizeof(*mt));
        mt->majortype = testtype;
        return S_OK;
    }
    return VFW_S_NO_MORE_ITEMS;
}

static HRESULT WINAPI testsource_DecideAllocator(struct strmbase_source *iface,
        IMemInputPin *input, IMemAllocator **allocator)
{
    return S_OK;
}

static const struct strmbase_source_ops testsource_ops =
{
    .base.pin_query_interface = testsource_query_interface,
    .base.pin_query_accept = testsource_query_accept,
    .base.pin_get_media_type = testsource_get_media_type,
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideAllocator = testsource_DecideAllocator,
};

static HRESULT testsink_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface->filter);

    ok(!IsEqualGUID(iid, &IID_IKsPropertySet), "Unexpected query for IKsPropertySet.\n");

    if (IsEqualGUID(iid, &IID_IMemInputPin))
        *out = &filter->sink.IMemInputPin_iface;
    else if (iface == &filter->sink.pin && filter->sink_has_iface && IsEqualGUID(iid, &testiid))
        *out = &iface->IPin_iface;
    else if (iface == &filter->sink2.pin && filter->sink2_has_iface && IsEqualGUID(iid, &testiid))
        *out = &iface->IPin_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT testsink_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    return S_OK;
}

static const struct strmbase_sink_ops testsink_ops =
{
    .base.pin_query_interface = testsink_query_interface,
    .base.pin_query_accept = testsink_query_accept,
    .base.pin_get_media_type = strmbase_pin_get_media_type,
};

static void reset_interfaces(struct testfilter *filter)
{
    filter->filter_has_iface = filter->sink_has_iface = filter->sink2_has_iface = TRUE;
    filter->source_has_iface = filter->source2_has_iface = TRUE;
}

static void testfilter_init(struct testfilter *filter)
{
    static const GUID clsid = {0xabacab};
    memset(filter, 0, sizeof(*filter));
    strmbase_filter_init(&filter->filter, NULL, &clsid, &testfilter_ops);
    strmbase_source_init(&filter->source, &filter->filter, L"source", &testsource_ops);
    strmbase_source_init(&filter->source2, &filter->filter, L"source2", &testsource_ops);
    strmbase_sink_init(&filter->sink, &filter->filter, L"sink", &testsink_ops, NULL);
    strmbase_sink_init(&filter->sink2, &filter->filter, L"sink2", &testsink_ops, NULL);
    reset_interfaces(filter);
}

static void test_find_interface(void)
{
    static const AM_MEDIA_TYPE mt1 =
    {
        .majortype = {0x111},
        .subtype = {0x222},
        .formattype = {0x333},
    };
    static const AM_MEDIA_TYPE mt2 =
    {
        .majortype = {0x444},
        .subtype = {0x555},
        .formattype = {0x666},
    };
    static const GUID bogus_majortype = {0x777};

    ICaptureGraphBuilder2 *capture_graph = create_capture_graph();
    struct testfilter filter1, filter2, filter3;
    IGraphBuilder *graph;
    unsigned int i;
    IUnknown *unk;
    HRESULT hr;
    ULONG ref;

    struct
    {
        BOOL *expose;
        const void *iface;
    }
    tests_from_filter2[] =
    {
        {&filter2.filter_has_iface,     &filter2.filter.IBaseFilter_iface},
        {&filter2.source_has_iface,     &filter2.source.pin.IPin_iface},
        {&filter3.sink_has_iface,       &filter3.sink.pin.IPin_iface},
        {&filter3.filter_has_iface,     &filter3.filter.IBaseFilter_iface},
        {&filter3.source_has_iface,     &filter3.source.pin.IPin_iface},
        {&filter3.source2_has_iface,    &filter3.source2.pin.IPin_iface},
        {&filter2.source2_has_iface,    &filter2.source2.pin.IPin_iface},
        {&filter2.sink_has_iface,       &filter2.sink.pin.IPin_iface},
        {&filter1.source_has_iface,     &filter1.source.pin.IPin_iface},
        {&filter1.filter_has_iface,     &filter1.filter.IBaseFilter_iface},
        {&filter1.sink_has_iface,       &filter1.sink.pin.IPin_iface},
        {&filter1.sink2_has_iface,      &filter1.sink2.pin.IPin_iface},
        {&filter2.sink2_has_iface,      &filter2.sink2.pin.IPin_iface},
    }, tests_from_filter1[] =
    {
        {&filter1.filter_has_iface,     &filter1.filter.IBaseFilter_iface},
        {&filter1.source_has_iface,     &filter1.source.pin.IPin_iface},
        {&filter2.sink_has_iface,       &filter2.sink.pin.IPin_iface},
        {&filter2.filter_has_iface,     &filter2.filter.IBaseFilter_iface},
        {&filter2.source_has_iface,     &filter2.source.pin.IPin_iface},
        {&filter3.sink_has_iface,       &filter3.sink.pin.IPin_iface},
        {&filter3.filter_has_iface,     &filter3.filter.IBaseFilter_iface},
        {&filter3.source_has_iface,     &filter3.source.pin.IPin_iface},
        {&filter3.source2_has_iface,    &filter3.source2.pin.IPin_iface},
        {&filter2.source2_has_iface,    &filter2.source2.pin.IPin_iface},
        {&filter1.source2_has_iface,    &filter1.source2.pin.IPin_iface},
        {&filter1.sink_has_iface,       &filter1.sink.pin.IPin_iface},
        {&filter1.sink2_has_iface,      &filter1.sink2.pin.IPin_iface},
    }, look_upstream_tests[] =
    {
        {&filter2.sink_has_iface,       &filter2.sink.pin.IPin_iface},
        {&filter1.source_has_iface,     &filter1.source.pin.IPin_iface},
        {&filter1.filter_has_iface,     &filter1.filter.IBaseFilter_iface},
        {&filter1.sink_has_iface,       &filter1.sink.pin.IPin_iface},
        {&filter1.sink2_has_iface,      &filter1.sink2.pin.IPin_iface},
        {&filter2.sink2_has_iface,      &filter2.sink2.pin.IPin_iface},
    }, look_downstream_tests[] =
    {
        {&filter2.source_has_iface,     &filter2.source.pin.IPin_iface},
        {&filter3.sink_has_iface,       &filter3.sink.pin.IPin_iface},
        {&filter3.filter_has_iface,     &filter3.filter.IBaseFilter_iface},
        {&filter3.source_has_iface,     &filter3.source.pin.IPin_iface},
        {&filter3.source2_has_iface,    &filter3.source2.pin.IPin_iface},
        {&filter2.source2_has_iface,    &filter2.source2.pin.IPin_iface},
    }, category_tests[] =
    {
        {&filter3.filter_has_iface,     &filter3.filter.IBaseFilter_iface},
        {&filter3.source_has_iface,     &filter3.source.pin.IPin_iface},
        {&filter3.source2_has_iface,    &filter3.source2.pin.IPin_iface},
        {&filter2.sink_has_iface,       &filter2.sink.pin.IPin_iface},
        {&filter1.source_has_iface,     &filter1.source.pin.IPin_iface},
        {&filter1.filter_has_iface,     &filter1.filter.IBaseFilter_iface},
        {&filter1.sink_has_iface,       &filter1.sink.pin.IPin_iface},
        {&filter1.sink2_has_iface,      &filter1.sink2.pin.IPin_iface},
        {&filter2.sink2_has_iface,      &filter2.sink2.pin.IPin_iface},
    };

    CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IGraphBuilder, (void **)&graph);
    hr = ICaptureGraphBuilder2_SetFiltergraph(capture_graph, graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    testfilter_init(&filter1);
    IGraphBuilder_AddFilter(graph, &filter1.filter.IBaseFilter_iface, L"filter1");
    testfilter_init(&filter2);
    IGraphBuilder_AddFilter(graph, &filter2.filter.IBaseFilter_iface, L"filter2");
    testfilter_init(&filter3);
    IGraphBuilder_AddFilter(graph, &filter3.filter.IBaseFilter_iface, L"filter3");

    hr = IGraphBuilder_ConnectDirect(graph, &filter1.source.pin.IPin_iface, &filter2.sink.pin.IPin_iface, &mt1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IGraphBuilder_ConnectDirect(graph, &filter2.source.pin.IPin_iface, &filter3.sink.pin.IPin_iface, &mt2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    /* Test search order without any restrictions applied. */

    for (i = 0; i < ARRAY_SIZE(tests_from_filter2); ++i)
    {
        hr = ICaptureGraphBuilder2_FindInterface(capture_graph, NULL, &bogus_majortype,
                &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
        ok(hr == S_OK, "Test %u: got hr %#x.\n", i, hr);
        ok(unk == tests_from_filter2[i].iface, "Test %u: got wrong interface %p.\n", i, unk);
        IUnknown_Release(unk);
        *tests_from_filter2[i].expose = FALSE;
    }

    hr = ICaptureGraphBuilder2_FindInterface(capture_graph, NULL, &bogus_majortype,
            &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
    ok(hr == E_FAIL, "Got hr %#x.\n", hr);

    reset_interfaces(&filter1);
    reset_interfaces(&filter2);
    reset_interfaces(&filter3);

    for (i = 0; i < ARRAY_SIZE(tests_from_filter1); ++i)
    {
        hr = ICaptureGraphBuilder2_FindInterface(capture_graph, NULL, &bogus_majortype,
                &filter1.filter.IBaseFilter_iface, &testiid, (void **)&unk);
        ok(hr == S_OK, "Test %u: got hr %#x.\n", i, hr);
        ok(unk == tests_from_filter1[i].iface, "Test %u: got wrong interface %p.\n", i, unk);
        IUnknown_Release(unk);
        *tests_from_filter1[i].expose = FALSE;
    }

    hr = ICaptureGraphBuilder2_FindInterface(capture_graph, NULL, &bogus_majortype,
            &filter1.filter.IBaseFilter_iface, &testiid, (void **)&unk);
    ok(hr == E_FAIL, "Got hr %#x.\n", hr);

    /* Test with upstream/downstream flags. */

    reset_interfaces(&filter1);
    reset_interfaces(&filter2);
    reset_interfaces(&filter3);

    for (i = 0; i < ARRAY_SIZE(look_upstream_tests); ++i)
    {
        hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &LOOK_UPSTREAM_ONLY, &bogus_majortype,
                &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
        ok(hr == S_OK, "Test %u: got hr %#x.\n", i, hr);
        ok(unk == look_upstream_tests[i].iface, "Test %u: got wrong interface %p.\n", i, unk);
        IUnknown_Release(unk);
        *look_upstream_tests[i].expose = FALSE;
    }

    hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &LOOK_UPSTREAM_ONLY, &bogus_majortype,
            &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
    ok(hr == E_FAIL, "Got hr %#x.\n", hr);

    reset_interfaces(&filter1);
    reset_interfaces(&filter2);
    reset_interfaces(&filter3);

    for (i = 0; i < ARRAY_SIZE(look_downstream_tests); ++i)
    {
        hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &LOOK_DOWNSTREAM_ONLY, &bogus_majortype,
                &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
        ok(hr == S_OK, "Test %u: got hr %#x.\n", i, hr);
        ok(unk == look_downstream_tests[i].iface, "Test %u: got wrong interface %p.\n", i, unk);
        IUnknown_Release(unk);
        *look_downstream_tests[i].expose = FALSE;
    }

    hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &LOOK_DOWNSTREAM_ONLY, &bogus_majortype,
            &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
    ok(hr == E_FAIL, "Got hr %#x.\n", hr);

    /* Test with a category flag. */

    reset_interfaces(&filter1);
    reset_interfaces(&filter2);
    reset_interfaces(&filter3);

    hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &PIN_CATEGORY_CAPTURE, NULL,
            &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
    ok(hr == S_OK, "Got hr %#x\n", hr);
    ok(unk == (IUnknown *)&filter2.filter.IBaseFilter_iface, "Got wrong interface %p.\n", unk);
    IUnknown_Release(unk);
    filter2.filter_has_iface = FALSE;

    hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &PIN_CATEGORY_CAPTURE, NULL,
            &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
    ok(hr == E_NOINTERFACE, "Got hr %#x.\n", hr);

    filter2.IKsPropertySet_iface.lpVtbl = &property_set_vtbl;
    hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &PIN_CATEGORY_CAPTURE, NULL,
            &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(unk == (IUnknown *)&filter2.source.pin.IPin_iface, "Got wrong interface %p.\n", unk);
    IUnknown_Release(unk);
    filter2.source_has_iface = FALSE;

    /* Native returns the filter3 sink next, but suffers from a bug wherein it
     * releases a reference to the wrong pin. */
    filter3.sink_has_iface = FALSE;

    for (i = 0; i < ARRAY_SIZE(category_tests); ++i)
    {
        hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &PIN_CATEGORY_CAPTURE, NULL,
                &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
        ok(hr == S_OK, "Test %u: got hr %#x.\n", i, hr);
        ok(unk == category_tests[i].iface, "Test %u: got wrong interface %p.\n", i, unk);
        IUnknown_Release(unk);
        *category_tests[i].expose = FALSE;
    }

    hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &PIN_CATEGORY_CAPTURE, NULL,
            &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
    ok(hr == E_FAIL, "Got hr %#x.\n", hr);

    /* Test with a media type. */

    reset_interfaces(&filter1);
    reset_interfaces(&filter2);
    reset_interfaces(&filter3);

    hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &PIN_CATEGORY_CAPTURE, &bogus_majortype,
            &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
    ok(hr == S_OK, "Got hr %#x\n", hr);
    ok(unk == (IUnknown *)&filter2.filter.IBaseFilter_iface, "Got wrong interface %p.\n", unk);
    IUnknown_Release(unk);
    filter2.filter_has_iface = FALSE;

    hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &PIN_CATEGORY_CAPTURE, &bogus_majortype,
            &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
    ok(hr == E_NOINTERFACE, "Got hr %#x.\n", hr);

    hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &PIN_CATEGORY_CAPTURE, &mt2.majortype,
            &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
    ok(hr == E_NOINTERFACE, "Got hr %#x.\n", hr);

    hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &PIN_CATEGORY_CAPTURE, &testtype,
            &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(unk == (IUnknown *)&filter2.source.pin.IPin_iface, "Got wrong interface %p.\n", unk);
    IUnknown_Release(unk);
    filter2.source_has_iface = FALSE;

    filter3.sink_has_iface = FALSE;

    for (i = 0; i < ARRAY_SIZE(category_tests); ++i)
    {
        hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &PIN_CATEGORY_CAPTURE, NULL,
                &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
        ok(hr == S_OK, "Test %u: got hr %#x.\n", i, hr);
        ok(unk == category_tests[i].iface, "Test %u: got wrong interface %p.\n", i, unk);
        IUnknown_Release(unk);
        *category_tests[i].expose = FALSE;
    }

    hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &PIN_CATEGORY_CAPTURE, NULL,
            &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
    ok(hr == E_FAIL, "Got hr %#x.\n", hr);

    ref = ICaptureGraphBuilder2_Release(capture_graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IGraphBuilder_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IBaseFilter_Release(&filter1.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IBaseFilter_Release(&filter2.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IBaseFilter_Release(&filter3.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

START_TEST(capturegraph)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    test_find_interface();

    CoUninitialize();
}
