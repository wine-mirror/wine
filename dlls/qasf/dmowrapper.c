/*
 * DMO wrapper filter
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

#include "qasf_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(qasf);

struct dmo_wrapper_source
{
    struct strmbase_source pin;
};

struct dmo_wrapper
{
    struct strmbase_filter filter;
    IDMOWrapperFilter IDMOWrapperFilter_iface;

    IUnknown *dmo;

    DWORD sink_count, source_count;
    struct strmbase_sink *sinks;
    struct dmo_wrapper_source *sources;
};

static inline struct dmo_wrapper *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_wrapper, filter);
}

static inline struct strmbase_sink *impl_sink_from_strmbase_pin(struct strmbase_pin *iface)
{
    return CONTAINING_RECORD(iface, struct strmbase_sink, pin);
}

static HRESULT dmo_wrapper_sink_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct strmbase_sink *sink = impl_sink_from_strmbase_pin(iface);

    if (IsEqualGUID(iid, &IID_IMemInputPin))
        *out = &sink->IMemInputPin_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT dmo_wrapper_sink_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    struct dmo_wrapper *filter = impl_from_strmbase_filter(iface->filter);
    IMediaObject *dmo;
    HRESULT hr;

    IUnknown_QueryInterface(filter->dmo, &IID_IMediaObject, (void **)&dmo);

    hr = IMediaObject_SetInputType(dmo, impl_sink_from_strmbase_pin(iface) - filter->sinks,
            (const DMO_MEDIA_TYPE *)mt, DMO_SET_TYPEF_TEST_ONLY);

    IMediaObject_Release(dmo);

    return hr;
}

static HRESULT dmo_wrapper_sink_get_media_type(struct strmbase_pin *iface, unsigned int index, AM_MEDIA_TYPE *mt)
{
    struct dmo_wrapper *filter = impl_from_strmbase_filter(iface->filter);
    IMediaObject *dmo;
    HRESULT hr;

    IUnknown_QueryInterface(filter->dmo, &IID_IMediaObject, (void **)&dmo);

    hr = IMediaObject_GetInputType(dmo, impl_sink_from_strmbase_pin(iface) - filter->sinks,
            index, (DMO_MEDIA_TYPE *)mt);

    IMediaObject_Release(dmo);

    return hr == S_OK ? S_OK : VFW_S_NO_MORE_ITEMS;
}

static HRESULT dmo_wrapper_sink_connect(struct strmbase_sink *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    struct dmo_wrapper *filter = impl_from_strmbase_filter(iface->pin.filter);
    IMediaObject *dmo;
    HRESULT hr;

    IUnknown_QueryInterface(filter->dmo, &IID_IMediaObject, (void **)&dmo);

    hr = IMediaObject_SetInputType(dmo, iface - filter->sinks, (const DMO_MEDIA_TYPE *)mt, 0);

    IMediaObject_Release(dmo);
    return hr;
}

static void dmo_wrapper_sink_disconnect(struct strmbase_sink *iface)
{
    struct dmo_wrapper *filter = impl_from_strmbase_filter(iface->pin.filter);
    IMediaObject *dmo;

    IUnknown_QueryInterface(filter->dmo, &IID_IMediaObject, (void **)&dmo);

    IMediaObject_SetInputType(dmo, iface - filter->sinks, NULL, DMO_SET_TYPEF_CLEAR);

    IMediaObject_Release(dmo);
}

static const struct strmbase_sink_ops sink_ops =
{
    .base.pin_query_interface = dmo_wrapper_sink_query_interface,
    .base.pin_query_accept = dmo_wrapper_sink_query_accept,
    .base.pin_get_media_type = dmo_wrapper_sink_get_media_type,
    .sink_connect = dmo_wrapper_sink_connect,
    .sink_disconnect = dmo_wrapper_sink_disconnect,
};

static inline struct dmo_wrapper_source *impl_source_from_strmbase_pin(struct strmbase_pin *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_wrapper_source, pin.pin);
}

static HRESULT dmo_wrapper_source_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    struct dmo_wrapper *filter = impl_from_strmbase_filter(iface->filter);
    IMediaObject *dmo;
    HRESULT hr;

    IUnknown_QueryInterface(filter->dmo, &IID_IMediaObject, (void **)&dmo);

    hr = IMediaObject_SetOutputType(dmo, impl_source_from_strmbase_pin(iface) - filter->sources,
            (const DMO_MEDIA_TYPE *)mt, DMO_SET_TYPEF_TEST_ONLY);

    IMediaObject_Release(dmo);

    return hr;
}

static HRESULT dmo_wrapper_source_get_media_type(struct strmbase_pin *iface, unsigned int index, AM_MEDIA_TYPE *mt)
{
    struct dmo_wrapper *filter = impl_from_strmbase_filter(iface->filter);
    IMediaObject *dmo;
    HRESULT hr;

    IUnknown_QueryInterface(filter->dmo, &IID_IMediaObject, (void **)&dmo);

    hr = IMediaObject_GetOutputType(dmo, impl_source_from_strmbase_pin(iface) - filter->sources,
            index, (DMO_MEDIA_TYPE *)mt);

    IMediaObject_Release(dmo);

    return hr == S_OK ? S_OK : VFW_S_NO_MORE_ITEMS;
}

static HRESULT WINAPI dmo_wrapper_source_DecideBufferSize(struct strmbase_source *iface,
        IMemAllocator *allocator, ALLOCATOR_PROPERTIES *props)
{
    struct dmo_wrapper *filter = impl_from_strmbase_filter(iface->pin.filter);
    DWORD index = impl_source_from_strmbase_pin(&iface->pin) - filter->sources;
    ALLOCATOR_PROPERTIES ret_props;
    DWORD size = 0, alignment = 0;
    IMediaObject *dmo;
    HRESULT hr;

    IUnknown_QueryInterface(filter->dmo, &IID_IMediaObject, (void **)&dmo);

    if (SUCCEEDED(hr = IMediaObject_SetOutputType(dmo, index,
            (const DMO_MEDIA_TYPE *)&iface->pin.mt, 0)))
        hr = IMediaObject_GetOutputSizeInfo(dmo, index, &size, &alignment);

    if (SUCCEEDED(hr))
    {
        props->cBuffers = max(props->cBuffers, 1);
        props->cbBuffer = max(max(props->cbBuffer, size), 16384);
        props->cbAlign = max(props->cbAlign, alignment);
        hr = IMemAllocator_SetProperties(allocator, props, &ret_props);
    }

    IMediaObject_Release(dmo);

    return hr;
}

static void dmo_wrapper_source_disconnect(struct strmbase_source *iface)
{
    struct dmo_wrapper *filter = impl_from_strmbase_filter(iface->pin.filter);
    IMediaObject *dmo;

    IUnknown_QueryInterface(filter->dmo, &IID_IMediaObject, (void **)&dmo);

    IMediaObject_SetOutputType(dmo, impl_source_from_strmbase_pin(&iface->pin) - filter->sources,
            NULL, DMO_SET_TYPEF_CLEAR);

    IMediaObject_Release(dmo);
}

static const struct strmbase_source_ops source_ops =
{
    .base.pin_query_accept = dmo_wrapper_source_query_accept,
    .base.pin_get_media_type = dmo_wrapper_source_get_media_type,
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideAllocator = BaseOutputPinImpl_DecideAllocator,
    .pfnDecideBufferSize = dmo_wrapper_source_DecideBufferSize,
    .source_disconnect = dmo_wrapper_source_disconnect,
};

static inline struct dmo_wrapper *impl_from_IDMOWrapperFilter(IDMOWrapperFilter *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_wrapper, IDMOWrapperFilter_iface);
}

static HRESULT WINAPI dmo_wrapper_filter_QueryInterface(IDMOWrapperFilter *iface, REFIID iid, void **out)
{
    struct dmo_wrapper *filter = impl_from_IDMOWrapperFilter(iface);
    return IUnknown_QueryInterface(filter->filter.outer_unk, iid, out);
}

static ULONG WINAPI dmo_wrapper_filter_AddRef(IDMOWrapperFilter *iface)
{
    struct dmo_wrapper *filter = impl_from_IDMOWrapperFilter(iface);
    return IUnknown_AddRef(filter->filter.outer_unk);
}

static ULONG WINAPI dmo_wrapper_filter_Release(IDMOWrapperFilter *iface)
{
    struct dmo_wrapper *filter = impl_from_IDMOWrapperFilter(iface);
    return IUnknown_Release(filter->filter.outer_unk);
}

static HRESULT WINAPI dmo_wrapper_filter_Init(IDMOWrapperFilter *iface, REFCLSID clsid, REFCLSID category)
{
    struct dmo_wrapper *filter = impl_from_IDMOWrapperFilter(iface);
    struct dmo_wrapper_source *sources;
    DWORD input_count, output_count;
    struct strmbase_sink *sinks;
    IMediaObject *dmo;
    IUnknown *unk;
    WCHAR id[14];
    HRESULT hr;
    DWORD i;

    TRACE("filter %p, clsid %s, category %s.\n", filter, debugstr_guid(clsid), debugstr_guid(category));

    if (FAILED(hr = CoCreateInstance(clsid, &filter->filter.IUnknown_inner,
            CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&unk)))
        return hr;

    if (FAILED(hr = IUnknown_QueryInterface(unk, &IID_IMediaObject, (void **)&dmo)))
    {
        IUnknown_Release(unk);
        return hr;
    }

    if (FAILED(IMediaObject_GetStreamCount(dmo, &input_count, &output_count)))
        input_count = output_count = 0;

    sinks = calloc(sizeof(*sinks), input_count);
    sources = calloc(sizeof(*sources), output_count);
    if (!sinks || !sources)
    {
        free(sinks);
        free(sources);
        IMediaObject_Release(dmo);
        IUnknown_Release(unk);
        return hr;
    }

    for (i = 0; i < input_count; ++i)
    {
        swprintf(id, ARRAY_SIZE(id), L"in%u", i);
        strmbase_sink_init(&sinks[i], &filter->filter, id, &sink_ops, NULL);
    }

    for (i = 0; i < output_count; ++i)
    {
        swprintf(id, ARRAY_SIZE(id), L"out%u", i);
        strmbase_source_init(&sources[i].pin, &filter->filter, id, &source_ops);
    }

    EnterCriticalSection(&filter->filter.csFilter);

    filter->dmo = unk;
    filter->sink_count = input_count;
    filter->source_count = output_count;
    filter->sinks = sinks;
    filter->sources = sources;

    LeaveCriticalSection(&filter->filter.csFilter);

    IMediaObject_Release(dmo);

    return S_OK;
}

static const IDMOWrapperFilterVtbl dmo_wrapper_filter_vtbl =
{
    dmo_wrapper_filter_QueryInterface,
    dmo_wrapper_filter_AddRef,
    dmo_wrapper_filter_Release,
    dmo_wrapper_filter_Init,
};

static struct strmbase_pin *dmo_wrapper_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    struct dmo_wrapper *filter = impl_from_strmbase_filter(iface);

    if (index < filter->sink_count)
        return &filter->sinks[index].pin;
    else if (index < filter->sink_count + filter->source_count)
        return &filter->sources[index - filter->sink_count].pin.pin;
    return NULL;
}

static void dmo_wrapper_destroy(struct strmbase_filter *iface)
{
    struct dmo_wrapper *filter = impl_from_strmbase_filter(iface);
    DWORD i;

    if (filter->dmo)
        IUnknown_Release(filter->dmo);
    for (i = 0; i < filter->sink_count; ++i)
        strmbase_sink_cleanup(&filter->sinks[i]);
    for (i = 0; i < filter->source_count; ++i)
        strmbase_source_cleanup(&filter->sources[i].pin);
    free(filter->sinks);
    free(filter->sources);
    strmbase_filter_cleanup(&filter->filter);
    free(filter);
}

static HRESULT dmo_wrapper_query_interface(struct strmbase_filter *iface, REFIID iid, void **out)
{
    struct dmo_wrapper *filter = impl_from_strmbase_filter(iface);

    if (IsEqualGUID(iid, &IID_IDMOWrapperFilter))
    {
        *out = &filter->IDMOWrapperFilter_iface;
        IUnknown_AddRef((IUnknown *)*out);
        return S_OK;
    }

    if (filter->dmo && !IsEqualGUID(iid, &IID_IUnknown))
        return IUnknown_QueryInterface(filter->dmo, iid, out);
    return E_NOINTERFACE;
}

static HRESULT dmo_wrapper_cleanup_stream(struct strmbase_filter *iface)
{
    struct dmo_wrapper *filter = impl_from_strmbase_filter(iface);
    IMediaObject *dmo;

    IUnknown_QueryInterface(filter->dmo, &IID_IMediaObject, (void **)&dmo);

    IMediaObject_Flush(dmo);

    IMediaObject_Release(dmo);
    return S_OK;
}

static struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = dmo_wrapper_get_pin,
    .filter_destroy = dmo_wrapper_destroy,
    .filter_query_interface = dmo_wrapper_query_interface,
    .filter_cleanup_stream = dmo_wrapper_cleanup_stream,
};

HRESULT dmo_wrapper_create(IUnknown *outer, IUnknown **out)
{
    struct dmo_wrapper *object;

    if (!(object = calloc(sizeof(*object), 1)))
        return E_OUTOFMEMORY;

    /* Always pass NULL as the outer object; see test_aggregation(). */
    strmbase_filter_init(&object->filter, NULL, &CLSID_DMOWrapperFilter, &filter_ops);

    object->IDMOWrapperFilter_iface.lpVtbl = &dmo_wrapper_filter_vtbl;

    TRACE("Created DMO wrapper %p.\n", object);
    *out = &object->filter.IUnknown_inner;

    return S_OK;
}
