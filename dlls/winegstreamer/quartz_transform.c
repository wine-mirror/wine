/*
 * DirectShow transform filters
 *
 * Copyright 2022 Anton Baskanov
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

#include "gst_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

struct transform
{
    struct strmbase_filter filter;

    struct strmbase_sink sink;
    struct strmbase_source source;

    const struct transform_ops *ops;
};

struct transform_ops
{
    HRESULT (*sink_query_accept)(struct transform *filter, const AM_MEDIA_TYPE *mt);
    HRESULT (*source_query_accept)(struct transform *filter, const AM_MEDIA_TYPE *mt);
};

static inline struct transform *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct transform, filter);
}

static struct strmbase_pin *transform_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    struct transform *filter = impl_from_strmbase_filter(iface);
    if (index == 0)
        return &filter->sink.pin;
    if (index == 1)
        return &filter->source.pin;
    return NULL;
}

static void transform_destroy(struct strmbase_filter *iface)
{
    struct transform *filter = impl_from_strmbase_filter(iface);

    strmbase_source_cleanup(&filter->source);
    strmbase_sink_cleanup(&filter->sink);
    strmbase_filter_cleanup(&filter->filter);

    free(filter);
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = transform_get_pin,
    .filter_destroy = transform_destroy,
};

static HRESULT transform_sink_query_accept(struct strmbase_pin *pin, const AM_MEDIA_TYPE *mt)
{
    struct transform *filter = impl_from_strmbase_filter(pin->filter);

    return filter->ops->sink_query_accept(filter, mt);
}

static HRESULT transform_sink_query_interface(struct strmbase_pin *pin, REFIID iid, void **out)
{
    struct transform *filter = impl_from_strmbase_filter(pin->filter);

    if (IsEqualGUID(iid, &IID_IMemInputPin))
        *out = &filter->sink.IMemInputPin_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static const struct strmbase_sink_ops sink_ops =
{
    .base.pin_query_accept = transform_sink_query_accept,
    .base.pin_query_interface = transform_sink_query_interface,
};

static HRESULT transform_source_query_accept(struct strmbase_pin *pin, const AM_MEDIA_TYPE *mt)
{
    struct transform *filter = impl_from_strmbase_filter(pin->filter);

    return filter->ops->source_query_accept(filter, mt);
}

static const struct strmbase_source_ops source_ops =
{
    .base.pin_query_accept = transform_source_query_accept,
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideAllocator = BaseOutputPinImpl_DecideAllocator,
};

static HRESULT transform_create(IUnknown *outer, const CLSID *clsid, const struct transform_ops *ops, struct transform **out)
{
    struct transform *object;

    object = calloc(1, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    strmbase_filter_init(&object->filter, outer, clsid, &filter_ops);
    strmbase_sink_init(&object->sink, &object->filter, L"In", &sink_ops, NULL);
    strmbase_source_init(&object->source, &object->filter, L"Out", &source_ops);

    object->ops = ops;

    *out = object;
    return S_OK;
}

static HRESULT mpeg_audio_codec_sink_query_accept(struct transform *filter, const AM_MEDIA_TYPE *mt)
{
    const MPEG1WAVEFORMAT *format;

    if (!IsEqualGUID(&mt->majortype, &MEDIATYPE_Audio))
        return S_FALSE;

    if (!IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_MPEG1Packet)
            && !IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_MPEG1Payload)
            && !IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_MPEG1AudioPayload)
            && !IsEqualGUID(&mt->subtype, &GUID_NULL))
        return S_FALSE;

    if (!IsEqualGUID(&mt->formattype, &FORMAT_WaveFormatEx)
            || mt->cbFormat < sizeof(MPEG1WAVEFORMAT))
        return S_FALSE;

    format = (const MPEG1WAVEFORMAT *)mt->pbFormat;

    if (format->wfx.wFormatTag != WAVE_FORMAT_MPEG
            || format->fwHeadLayer == ACM_MPEG_LAYER3)
        return S_FALSE;

    return S_OK;
}

static HRESULT mpeg_audio_codec_source_query_accept(struct transform *filter, const AM_MEDIA_TYPE *mt)
{
    const MPEG1WAVEFORMAT *input_format;
    const WAVEFORMATEX *output_format;
    DWORD expected_avg_bytes_per_sec;
    WORD expected_block_align;

    if (!filter->sink.pin.peer)
        return S_FALSE;

    if (!IsEqualGUID(&mt->majortype, &MEDIATYPE_Audio)
            || !IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_PCM)
            || !IsEqualGUID(&mt->formattype, &FORMAT_WaveFormatEx)
            || mt->cbFormat < sizeof(WAVEFORMATEX))
        return S_FALSE;

    input_format = (const MPEG1WAVEFORMAT *)filter->sink.pin.mt.pbFormat;
    output_format = (const WAVEFORMATEX *)mt->pbFormat;

    if (output_format->wFormatTag != WAVE_FORMAT_PCM
            || input_format->wfx.nSamplesPerSec != output_format->nSamplesPerSec
            || input_format->wfx.nChannels != output_format->nChannels
            || (output_format->wBitsPerSample != 8 && output_format->wBitsPerSample != 16))
        return S_FALSE;

    expected_block_align = output_format->nChannels * output_format->wBitsPerSample / 8;
    expected_avg_bytes_per_sec = expected_block_align * output_format->nSamplesPerSec;

    if (output_format->nBlockAlign != expected_block_align
            || output_format->nAvgBytesPerSec != expected_avg_bytes_per_sec)
        return S_FALSE;

    return S_OK;
}

static const struct transform_ops mpeg_audio_codec_transform_ops =
{
    mpeg_audio_codec_sink_query_accept,
    mpeg_audio_codec_source_query_accept,
};

HRESULT mpeg_audio_codec_create(IUnknown *outer, IUnknown **out)
{
    struct transform *object;
    HRESULT hr;

    hr = transform_create(outer, &CLSID_CMpegAudioCodec, &mpeg_audio_codec_transform_ops, &object);
    if (FAILED(hr))
        return hr;

    wcscpy(object->sink.pin.name, L"XForm In");
    wcscpy(object->source.pin.name, L"XForm Out");

    TRACE("Created MPEG audio decoder %p.\n", object);
    *out = &object->filter.IUnknown_inner;
    return hr;
}
