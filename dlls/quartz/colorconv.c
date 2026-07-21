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

#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

struct color_converter
{
    struct strmbase_filter filter;

    struct strmbase_source source;
    struct strmbase_passthrough passthrough;

    struct strmbase_sink sink;

    struct SwsContext *sws_ctx;
};

struct subtype
{
    const GUID *guid;
    DWORD compression;
    WORD bitcount;
    ULONG cbFormat;
    enum AVPixelFormat ff_fmt;
};

static const struct subtype subtypes[] =
{
    { &MEDIASUBTYPE_ARGB32, BI_RGB, 32, sizeof(VIDEOINFOHEADER), AV_PIX_FMT_BGR0 },
    { &MEDIASUBTYPE_RGB32, BI_RGB, 32, sizeof(VIDEOINFOHEADER), AV_PIX_FMT_BGR0 },
    { &MEDIASUBTYPE_RGB24, BI_RGB, 24, sizeof(VIDEOINFOHEADER), AV_PIX_FMT_BGR24 },
    { &MEDIASUBTYPE_RGB565, BI_BITFIELDS, 16, sizeof(VIDEOINFOHEADER) + sizeof(DWORD[3]) /* dwBitMasks */,
            AV_PIX_FMT_RGB565 },
    { &MEDIASUBTYPE_RGB555, BI_BITFIELDS, 16, sizeof(VIDEOINFOHEADER) + sizeof(DWORD[3]) /* dwBitMasks */,
            AV_PIX_FMT_RGB555 },
    { &MEDIASUBTYPE_RGB8, BI_RGB, 8, sizeof(VIDEOINFOHEADER) + sizeof(RGBQUAD[256]) /* bmiColors */, AV_PIX_FMT_RGB8 },
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

static HRESULT create_context(struct color_converter *filter)
{
    const struct subtype *sink_subtype, *source_subtype;
    BITMAPINFOHEADER *sink_header, *source_header;
    struct SwsContext *sws_ctx;
    LONG width, height;
    int ret;

    sink_subtype = get_subtype(&filter->sink.pin.mt);
    sink_header = &((VIDEOINFOHEADER *)filter->sink.pin.mt.pbFormat)->bmiHeader;
    source_subtype = get_subtype(&filter->source.pin.mt);
    source_header = &((VIDEOINFOHEADER *)filter->source.pin.mt.pbFormat)->bmiHeader;
    height = min(labs(sink_header->biHeight), labs(source_header->biHeight));
    width = min(sink_header->biWidth, source_header->biWidth);

    sws_ctx = filter->sws_ctx = sws_alloc_context();
    if (!sws_ctx)
        return E_OUTOFMEMORY;

    av_opt_set(sws_ctx, "sws_flags", "neighbor", 0);
    av_opt_set_int(sws_ctx, "threads", 0, 0);
    av_opt_set_int(sws_ctx, "srcw", width, 0);
    av_opt_set_int(sws_ctx, "srch", height, 0);
    av_opt_set_pixel_fmt(sws_ctx, "src_format", sink_subtype->ff_fmt, 0);
    av_opt_set_int(sws_ctx, "dstw", width, 0);
    av_opt_set_int(sws_ctx, "dsth", height, 0);
    av_opt_set_pixel_fmt(sws_ctx, "dst_format", source_subtype->ff_fmt, 0);

    ret = sws_init_context(sws_ctx, NULL, NULL);

    if (ret < 0)
    {
        ERR("sws_init_context %d\n", ret);
        return E_FAIL;
    }

    return S_OK;
}

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

static void media_sample_release(void *opaque, uint8_t *data)
{
    IMediaSample_Release((IMediaSample *)opaque);
}

static AVBufferRef *buffer_from_media_sample(IMediaSample *media_sample, int flags)
{
    BYTE *buff;
    LONG size;

    if (FAILED(IMediaSample_GetPointer(media_sample, (BYTE **)&buff)))
        return NULL;

    size = IMediaSample_GetSize(media_sample);
    IMediaSample_AddRef(media_sample);
    return av_buffer_create(buff, size, media_sample_release, media_sample, flags);
}

static HRESULT create_av_frame(AVFrame **ret_frame, IMediaSample *media_sample, int flags,
        const AM_MEDIA_TYPE *media_type, LONG height)
{
    const struct subtype *subtype;
    BITMAPINFOHEADER *header;
    AVBufferRef *buffer;
    AVFrame *frame;
    int ret;

    frame = av_frame_alloc();
    if (!frame)
        return E_OUTOFMEMORY;

    *ret_frame = frame;

    if (!(buffer = buffer_from_media_sample(media_sample, flags)))
    {
        av_frame_free(ret_frame);
        return E_OUTOFMEMORY;
    }

    header = &((VIDEOINFOHEADER *)media_type->pbFormat)->bmiHeader;
    subtype = get_subtype(media_type);

    frame->buf[0] = buffer;
    frame->format = subtype->ff_fmt;
    frame->width = header->biWidth;
    frame->height = height;

    if ((ret = av_image_fill_arrays((uint8_t **)frame->data, (int *)frame->linesize, frame->buf[0]->data, frame->format,
            frame->width, frame->height, 4)) < 0)
    {
        av_frame_free(ret_frame);
        ERR("av_image_fill_arrays returned %d.\n", ret);
        return E_FAIL;
    }

    /* Move to first scan line, as Windows always blits from/to the top left rect */
    if (header->biHeight > 0)
    {
        frame->data[0] += frame->linesize[0] * (header->biHeight - 1);
        frame->linesize[0] = -frame->linesize[0];
    }

    return S_OK;
}

static HRESULT WINAPI color_sink_Receive(struct strmbase_sink *iface, IMediaSample *src_sample)
{
    struct color_converter *filter = impl_from_strmbase_filter(iface->pin.filter);
    BITMAPINFOHEADER *sink_header, *source_header;
    LONG output_image_size, dst_stride, height;
    AVFrame *input_frame, *output_frame;
    IMediaSample *dst_sample;
    LONGLONG start, stop;
    AM_MEDIA_TYPE *mt;
    BYTE *dst_buff;
    LONG dst_size;
    HRESULT hr;
    int ret;

    input_frame = output_frame = NULL;

    /* We do not expect pin connection state to change while the filter is
     * running. This guarantee is necessary, since otherwise we would have to
     * take the filter lock, and we can't take the filter lock from a streaming
     * thread. */
    if (!filter->source.pMemInputPin)
    {
        WARN("Source is not connected, returning VFW_E_NOT_CONNECTED.\n");
        return VFW_E_NOT_CONNECTED;
    }

    if (filter->filter.state == State_Stopped)
        return VFW_E_WRONG_STATE;

    if (filter->sink.flushing)
        return S_FALSE;

    /* Handle dynamic format change of input sample. */
    if ((hr = IMediaSample_GetMediaType(src_sample, &mt)) == S_OK)
    {
        if (memcmp(mt, &filter->sink.pin.mt, offsetof(AM_MEDIA_TYPE, pbFormat))
                || memcmp(mt->pbFormat, filter->sink.pin.mt.pbFormat, mt->cbFormat))
        {
            VIDEOINFO *curr_video_info = (VIDEOINFO *)filter->sink.pin.mt.pbFormat;
            VIDEOINFO *new_video_info = (VIDEOINFO *)mt->pbFormat;

            if (mt->lSampleSize != filter->sink.pin.mt.lSampleSize
                    || curr_video_info->bmiHeader.biWidth != new_video_info->bmiHeader.biWidth
                    || labs(curr_video_info->bmiHeader.biHeight) != labs(new_video_info->bmiHeader.biHeight))
            {
                DeleteMediaType(mt);
                FIXME("Dynamic format change that requires buffer renegotiation is not supported\n");
                return VFW_E_INVALIDMEDIATYPE;
            }
            else
            {
                FreeMediaType(&filter->sink.pin.mt);
                filter->sink.pin.mt = *mt;
                CoTaskMemFree(mt);
            }
        }
        else
        {
            DeleteMediaType(mt);
        }
    }

    if (FAILED(hr = IMemAllocator_GetBuffer(filter->source.pAllocator, &dst_sample, NULL, NULL, 0)))
    {
        ERR("Failed to get sample, hr %#lx.\n", hr);
        return hr;
    }

    if (FAILED(hr = IMediaSample_GetPointer(dst_sample, (BYTE **)&dst_buff)))
    {
        ERR("Failed to get output buffer pointer, hr %#lx.\n", hr);
        goto out;
    }

    /* Handle dynamic format change of output sample. */
    if ((hr = IMediaSample_GetMediaType(dst_sample, &mt)) == S_OK)
    {
        if (memcmp(mt, &filter->source.pin.mt, offsetof(AM_MEDIA_TYPE, pbFormat))
                || memcmp(mt->pbFormat, filter->source.pin.mt.pbFormat, mt->cbFormat))
        {
            FreeMediaType(&filter->source.pin.mt);
            filter->source.pin.mt = *mt;
            CoTaskMemFree(mt);
            sws_freeContext(filter->sws_ctx);
            if (FAILED(hr = create_context(filter)))
            {
                ERR("Failed create context %#lx.\n", hr);
                goto out;
            }
        }
        else
        {
            DeleteMediaType(mt);
        }
    }
    else if (hr != S_FALSE)
    {
        ERR("Failed to get media type, hr %#lx.\n", hr);
    }

    sink_header = &((VIDEOINFOHEADER *)filter->sink.pin.mt.pbFormat)->bmiHeader;
    source_header = &((VIDEOINFOHEADER *)filter->source.pin.mt.pbFormat)->bmiHeader;
    height = min(labs(sink_header->biHeight), labs(source_header->biHeight));
    dst_stride = calculate_stride(source_header);
    output_image_size = dst_stride * labs(source_header->biHeight);
    dst_size = IMediaSample_GetSize(dst_sample);
    if (dst_size < output_image_size)
    {
        ERR("Sample size is too small (%ld < %lu).\n", dst_size, output_image_size);
        hr = E_FAIL;
        goto out;
    }

    hr = IMediaSample_GetTime(src_sample, &start, &stop);

    if (hr == S_OK)
        IMediaSample_SetTime(dst_sample, &start, &stop);
    else if (hr == VFW_S_NO_STOP_TIME)
        IMediaSample_SetTime(dst_sample, &start, NULL);
    else
        IMediaSample_SetTime(dst_sample, NULL, NULL);

    if (FAILED(hr = create_av_frame(&input_frame, src_sample, AV_BUFFER_FLAG_READONLY, &filter->sink.pin.mt, height)))
        goto out;

    if (FAILED(hr = create_av_frame(&output_frame, dst_sample, 0, &filter->source.pin.mt, height)))
        goto out;

    ret = sws_scale_frame(filter->sws_ctx, output_frame, input_frame);

    av_frame_free(&input_frame);
    av_frame_free(&output_frame);

    if (ret < 0)
    {
        ERR("sws_scale_frame returned %d.\n", ret);
        hr = E_FAIL;
        goto out;
    }

    IMediaSample_SetActualDataLength(dst_sample, output_image_size);

    IMediaSample_SetPreroll(dst_sample, (IMediaSample_IsPreroll(src_sample) == S_OK));
    IMediaSample_SetDiscontinuity(dst_sample, (IMediaSample_IsDiscontinuity(src_sample) == S_OK));
    IMediaSample_SetSyncPoint(dst_sample, TRUE);

    hr = IMemInputPin_Receive(filter->source.pMemInputPin, dst_sample);
    if (hr != S_OK && hr != VFW_E_NOT_CONNECTED)
        ERR("Failed to send sample, hr %#lx.\n", hr);

out:
    IMediaSample_Release(dst_sample);
    av_frame_free(&input_frame);
    av_frame_free(&output_frame);

    return hr;
}

static HRESULT color_sink_receive_can_block(struct strmbase_sink *iface)
{
    struct color_converter *filter = impl_from_strmbase_filter(iface->pin.filter);

    if (!filter->source.pMemInputPin)
        return VFW_E_NOT_CONNECTED;

    return IMemInputPin_ReceiveCanBlock(filter->source.pMemInputPin);
}

static const struct strmbase_sink_ops sink_ops =
{
    .base.pin_query_interface = color_sink_query_interface,
    .base.pin_query_accept = color_sink_query_accept,
    .sink_connect = color_sink_connect,
    .pfnReceive = color_sink_Receive,
    .sink_receive_can_block = color_sink_receive_can_block,
};

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

    if (FAILED(hr = create_context(filter)))
    {
        ERR("Failed create context %#lx.\n", hr);
        return hr;
    }

    if (FAILED(hr = IMemAllocator_Commit(filter->source.pAllocator)))
        ERR("Failed to commit allocator, hr %#lx.\n", hr);

    return S_OK;
}

static HRESULT color_cleanup_stream(struct strmbase_filter *iface)
{
    struct color_converter *filter = impl_from_strmbase_filter(iface);

    if (!filter->source.pin.peer)
        return S_OK;

    sws_freeContext(filter->sws_ctx);

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
