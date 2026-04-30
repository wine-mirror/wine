/*
 * Color converter
 *
 * Copyright 2026 Brendan McGrath
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

#include "quartz_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

struct color_converter
{
    struct strmbase_filter filter;

    struct strmbase_source source;
    struct strmbase_passthrough passthrough;

    struct strmbase_sink sink;
};

static struct color_converter *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct color_converter, filter);
}

static HRESULT color_sink_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    FIXME("stub\n");
    return S_FALSE;
}

static const struct strmbase_sink_ops sink_ops =
{
    .base.pin_query_accept = color_sink_query_accept,
};

static HRESULT WINAPI color_source_DecideBufferSize(
        struct strmbase_source *iface, IMemAllocator *alloc, ALLOCATOR_PROPERTIES *props)
{
    ALLOCATOR_PROPERTIES actual;

    if (!props->cbAlign)
        props->cbAlign = 1;

    if (!props->cBuffers)
        props->cBuffers = 1;

    return IMemAllocator_SetProperties(alloc, props, &actual);
}

static HRESULT color_source_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct color_converter *filter = impl_from_strmbase_filter(iface->filter);

    if (IsEqualGUID(iid, &IID_IMediaSeeking))
        *out = &filter->passthrough.IMediaSeeking_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static const struct strmbase_source_ops source_ops =
{
    .base.pin_query_interface = color_source_query_interface,
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideAllocator = BaseOutputPinImpl_DecideAllocator,
    .pfnDecideBufferSize = color_source_DecideBufferSize,
};

static struct strmbase_pin *color_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    struct color_converter *filter = impl_from_strmbase_filter(iface);

    if (index == 0)
        return &filter->sink.pin;
    else if (index == 1)
        return &filter->source.pin;
    return NULL;
}

static void color_destroy(struct strmbase_filter *iface)
{
    struct color_converter *filter = impl_from_strmbase_filter(iface);

    if (filter->sink.pin.peer)
        IPin_Disconnect(filter->sink.pin.peer);
    IPin_Disconnect(&filter->sink.pin.IPin_iface);

    if (filter->source.pin.peer)
        IPin_Disconnect(filter->source.pin.peer);
    IPin_Disconnect(&filter->source.pin.IPin_iface);

    strmbase_sink_cleanup(&filter->sink);
    strmbase_source_cleanup(&filter->source);
    strmbase_passthrough_cleanup(&filter->passthrough);
    strmbase_filter_cleanup(&filter->filter);

    free(filter);
}

static HRESULT color_init_stream(struct strmbase_filter *iface)
{
    struct color_converter *filter = impl_from_strmbase_filter(iface);
    HRESULT hr;

    if (!filter->source.pin.peer)
        return S_OK;

    if (FAILED(hr = IMemAllocator_Commit(filter->source.pAllocator)))
        ERR("Failed to commit allocator, hr %#lx.\n", hr);

    return S_OK;
}

static HRESULT color_cleanup_stream(struct strmbase_filter *iface)
{
    struct color_converter *filter = impl_from_strmbase_filter(iface);

    if (!filter->source.pin.peer)
        return S_OK;

    IMemAllocator_Decommit(filter->source.pAllocator);

    return S_OK;
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = color_get_pin,
    .filter_destroy = color_destroy,
    .filter_init_stream = color_init_stream,
    .filter_cleanup_stream = color_cleanup_stream,
};

HRESULT color_create(IUnknown *outer, IUnknown **out)
{
    struct color_converter *object;
    IMemAllocator *allocator;
    HRESULT hr;

    if (FAILED(hr = CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMemAllocator, (void **)&allocator)))
        return hr;

    if (!(object = calloc(1, sizeof(*object))))
    {
        IMemAllocator_Release(allocator);
        return E_OUTOFMEMORY;
    }

    strmbase_filter_init(&object->filter, outer, &CLSID_Colour, &filter_ops);

    strmbase_sink_init(&object->sink, &object->filter, L"In", &sink_ops, allocator);
    wcscpy(object->sink.pin.name, L"Input");

    strmbase_source_init(&object->source, &object->filter, L"Out", &source_ops);
    wcscpy(object->source.pin.name, L"XForm Out");

    strmbase_passthrough_init(&object->passthrough, (IUnknown *)&object->source.pin.IPin_iface);
    ISeekingPassThru_Init(&object->passthrough.ISeekingPassThru_iface, FALSE, &object->sink.pin.IPin_iface);

    TRACE("Created Color Converter %p.\n", object);
    *out = &object->filter.IUnknown_inner;

    return S_OK;
}
