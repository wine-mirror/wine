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

struct subtype
{
    const GUID *guid;
    DWORD compression;
    WORD bitcount;
    ULONG cbFormat;
};

static const struct subtype subtypes[] =
{
    { &MEDIASUBTYPE_ARGB32, BI_RGB, 32, sizeof(VIDEOINFOHEADER) },
    { &MEDIASUBTYPE_RGB32, BI_RGB, 32, sizeof(VIDEOINFOHEADER) },
    { &MEDIASUBTYPE_RGB24, BI_RGB, 24, sizeof(VIDEOINFOHEADER) },
    { &MEDIASUBTYPE_RGB565, BI_BITFIELDS, 16, sizeof(VIDEOINFOHEADER) + sizeof(DWORD[3]) /* dwBitMasks */ },
    { &MEDIASUBTYPE_RGB555, BI_BITFIELDS, 16, sizeof(VIDEOINFOHEADER) + sizeof(DWORD[3]) /* dwBitMasks */ },
    { &MEDIASUBTYPE_RGB8, BI_RGB, 8, sizeof(VIDEOINFOHEADER) + sizeof(RGBQUAD[256]) /* bmiColors */ },
};

static const struct subtype *get_subtype(const AM_MEDIA_TYPE *mt)
{
    const struct subtype *subtype = NULL;
    VIDEOINFOHEADER *video_info;
    int i;

    if (!IsEqualGUID(&mt->majortype, &MEDIATYPE_Video) || !IsEqualGUID(&mt->formattype, &FORMAT_VideoInfo))
        return NULL;

    for (i = 0; i < ARRAY_SIZE(subtypes); i++)
    {
        if (IsEqualGUID(&mt->subtype, subtypes[i].guid))
        {
            subtype = subtypes + i;
            break;
        }
    }

    if (!subtype || !(video_info = (VIDEOINFOHEADER *)mt->pbFormat)
            || video_info->bmiHeader.biSize != sizeof(video_info->bmiHeader))
        return NULL;

    if (video_info->rcSource.left < 0 || video_info->rcSource.top < 0 || video_info->rcTarget.left < 0
            || video_info->rcTarget.top < 0 || video_info->rcSource.right - video_info->rcSource.left < 0
            || video_info->rcTarget.bottom - video_info->rcTarget.top < 0
            || video_info->rcSource.right - video_info->rcSource.left
                       != video_info->rcTarget.right - video_info->rcTarget.left
            || video_info->rcSource.bottom - video_info->rcSource.top
                       != video_info->rcTarget.bottom - video_info->rcTarget.top)
        return NULL;

    if (video_info->rcSource.left != 0 || video_info->rcSource.top != 0 || video_info->rcTarget.left != 0
            || video_info->rcTarget.top != 0)
        FIXME("nontrivial rcSource/rcTarget handling is not supported\n");

    return subtype;
}

static LONG calculate_stride(const BITMAPINFOHEADER *bmi_header)
{
    return (bmi_header->biWidth * (bmi_header->biBitCount / 8) + 3) & ~3;
}

static struct color_converter *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct color_converter, filter);
}

static HRESULT color_sink_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct color_converter *filter = impl_from_strmbase_filter(iface->filter);

    if (IsEqualGUID(iid, &IID_IMemInputPin))
        *out = &filter->sink.IMemInputPin_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT color_sink_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    if (get_subtype(mt))
        return S_OK;
    else
        return S_FALSE;
}

static HRESULT color_sink_connect(struct strmbase_sink *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    BITMAPINFOHEADER *header;

    header = &((VIDEOINFOHEADER *)mt->pbFormat)->bmiHeader;
    if (header->biCompression == BI_RGB
            && mt->cbFormat >= sizeof(VIDEOINFOHEADER) + sizeof(RGBQUAD) * header->biClrUsed)
        return S_OK;
    else
        return VFW_E_INVALIDMEDIATYPE;
}

static const struct strmbase_sink_ops sink_ops =
{
    .base.pin_query_interface = color_sink_query_interface,
    .base.pin_query_accept = color_sink_query_accept,
    .sink_connect = color_sink_connect,
};

static HRESULT WINAPI color_source_DecideBufferSize(
        struct strmbase_source *iface, IMemAllocator *alloc, ALLOCATOR_PROPERTIES *props)
{
    ALLOCATOR_PROPERTIES actual;
    BITMAPINFOHEADER *header;
    LONG min_image_size;

    if (!props->cbAlign)
        props->cbAlign = 1;

    if (!props->cBuffers)
        props->cBuffers = 1;

    header = &((VIDEOINFOHEADER *)iface->pin.mt.pbFormat)->bmiHeader;
    min_image_size = calculate_stride(header) * labs(header->biHeight);

    if (props->cbBuffer < min_image_size)
        props->cbBuffer = min_image_size;

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

static HRESULT color_source_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    struct color_converter *filter = impl_from_strmbase_filter(iface->filter);

    if (!filter->sink.pin.peer)
        return S_FALSE;

    return get_subtype(mt) ? S_OK : S_FALSE;
}

static const RGBQUAD color_prefix[] =
{
    { 0, 0, 0 },
    { 0, 0, 128 },
    { 0, 128, 0 },
    { 0, 128, 128 },
    { 128, 0, 0 },
    { 128, 0, 128 },
    { 128, 128, 0 },
    { 192, 192, 192 },
    { 192, 220, 192 },
    { 240, 202, 166 },
};

static const BYTE color_pattern[] = { 1, 51, 102, 153, 204, 254 };

static HRESULT color_source_get_media_type(struct strmbase_pin *iface, unsigned int index, AM_MEDIA_TYPE *mt)
{
    struct color_converter *filter = impl_from_strmbase_filter(iface->filter);
    const VIDEOINFOHEADER *sink_format;
    const struct subtype *subtype;
    VIDEOINFO *format;
    RGBQUAD *ptr;
    int r, g, b;

    if (!filter->sink.pin.peer || index >= ARRAY_SIZE(subtypes))
        return VFW_S_NO_MORE_ITEMS;

    subtype = subtypes + index;
    sink_format = (VIDEOINFOHEADER *)filter->sink.pin.mt.pbFormat;

    memset(mt, 0, sizeof(AM_MEDIA_TYPE));

    mt->cbFormat = max(filter->sink.pin.mt.cbFormat, subtype->cbFormat);
    if (!(format = CoTaskMemAlloc(mt->cbFormat)))
        return E_OUTOFMEMORY;

    memset(format, 0, mt->cbFormat);

    format->rcSource = sink_format->rcSource;
    format->rcTarget = sink_format->rcTarget;
    format->dwBitRate = sink_format->dwBitRate;
    format->dwBitErrorRate = sink_format->dwBitErrorRate;
    format->AvgTimePerFrame = sink_format->AvgTimePerFrame;

    format->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    format->bmiHeader.biWidth = sink_format->bmiHeader.biWidth;
    format->bmiHeader.biHeight = sink_format->bmiHeader.biHeight;
    format->bmiHeader.biPlanes = sink_format->bmiHeader.biPlanes;
    format->bmiHeader.biBitCount = subtype->bitcount;
    format->bmiHeader.biCompression = subtype->compression;
    format->bmiHeader.biSizeImage = calculate_stride(&format->bmiHeader) * labs(format->bmiHeader.biHeight);

    if (IsEqualGUID(subtype->guid, &MEDIASUBTYPE_RGB565))
    {
        format->dwBitMasks[iRED] = 0xf800;
        format->dwBitMasks[iGREEN] = 0x07e0;
        format->dwBitMasks[iBLUE] = 0x001f;
    }
    else if (IsEqualGUID(subtype->guid, &MEDIASUBTYPE_RGB555))
    {
        format->dwBitMasks[iRED] = 0x7c00;
        format->dwBitMasks[iGREEN] = 0x03e0;
        format->dwBitMasks[iBLUE] = 0x001f;
    }
    else if (IsEqualGUID(subtype->guid, &MEDIASUBTYPE_RGB8))
    {
        format->bmiHeader.biClrUsed = 226;
        format->bmiHeader.biClrImportant = 226;
        memcpy(format->bmiColors, color_prefix, sizeof(color_prefix));
        ptr = format->bmiColors + ARRAY_SIZE(color_prefix);
        for (b = 0; b < ARRAY_SIZE(color_pattern); b++)
            for (g = 0; g < ARRAY_SIZE(color_pattern); g++)
                for (r = 0; r < ARRAY_SIZE(color_pattern); r++)
                    *ptr++ = (RGBQUAD){ color_pattern[b], color_pattern[g], color_pattern[r] };
    }

    mt->majortype = MEDIATYPE_Video;
    mt->subtype = *subtype->guid;
    mt->bFixedSizeSamples = TRUE;
    mt->lSampleSize = format->bmiHeader.biSizeImage;
    mt->formattype = FORMAT_VideoInfo;
    mt->pbFormat = (BYTE *)format;

    return S_OK;
}

static const struct strmbase_source_ops source_ops =
{
    .base.pin_query_interface = color_source_query_interface,
    .base.pin_query_accept = color_source_query_accept,
    .base.pin_get_media_type = color_source_get_media_type,
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
