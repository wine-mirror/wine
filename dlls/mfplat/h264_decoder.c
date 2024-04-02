/* H264 Decoder Transform
 *
 * Copyright 2022 Rémi Bernon for CodeWeavers
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

#include "mfapi.h"
#include "mferror.h"
#include "mfobjects.h"
#include "mftransform.h"
#include "wmcodecdsp.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

static const GUID *h264_decoder_input_types[] =
{
    &MFVideoFormat_H264,
};
static const GUID *h264_decoder_output_types[] =
{
    &MFVideoFormat_NV12,
    &MFVideoFormat_YV12,
    &MFVideoFormat_IYUV,
    &MFVideoFormat_I420,
    &MFVideoFormat_YUY2,
};

struct h264_decoder
{
    IMFTransform IMFTransform_iface;
    LONG refcount;
    IMFMediaType *input_type;
    IMFMediaType *output_type;

    struct wg_transform *wg_transform;
    struct wg_format wg_format;
    ULONGLONG last_pts;
};

static struct h264_decoder *impl_from_IMFTransform(IMFTransform *iface)
{
    return CONTAINING_RECORD(iface, struct h264_decoder, IMFTransform_iface);
}

static HRESULT try_create_wg_transform(struct h264_decoder *decoder)
{
    struct wg_encoded_format input_format;
    struct wg_format output_format;

    if (decoder->wg_transform)
        wg_transform_destroy(decoder->wg_transform);

    mf_media_type_to_wg_encoded_format(decoder->input_type, &input_format);
    if (input_format.encoded_type == WG_ENCODED_TYPE_UNKNOWN)
        return MF_E_INVALIDMEDIATYPE;

    mf_media_type_to_wg_format(decoder->output_type, &output_format);
    if (output_format.major_type == WG_MAJOR_TYPE_UNKNOWN)
        return MF_E_INVALIDMEDIATYPE;

    decoder->last_pts = 0;
    decoder->wg_transform = wg_transform_create(&input_format, &output_format);
    if (decoder->wg_transform)
        return S_OK;

    WARN("Failed to create H264 wg_transform.\n");
    return E_FAIL;
}

static HRESULT fill_output_media_type(IMFMediaType *media_type, IMFMediaType *default_type)
{
    UINT32 value, width, height;
    MFVideoArea aperture = {0};
    UINT64 value64;
    GUID subtype;
    HRESULT hr;

    if (FAILED(hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &subtype)))
        return hr;

    if (FAILED(hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64)))
    {
        if (!default_type || FAILED(hr = IMFMediaType_GetUINT64(default_type, &MF_MT_FRAME_SIZE, &value64)))
            value64 = (UINT64)1920 << 32 | 1080;
        if (FAILED(hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, value64)))
            return hr;
    }
    width = value64 >> 32;
    height = value64;

    if (FAILED(hr = IMFMediaType_GetItem(media_type, &MF_MT_FRAME_RATE, NULL)))
    {
        if (!default_type || FAILED(hr = IMFMediaType_GetUINT64(default_type, &MF_MT_FRAME_RATE, &value64)))
            value64 = (UINT64)30000 << 32 | 1001;
        if (FAILED(hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_RATE, value64)))
            return hr;
    }

    if (FAILED(hr = IMFMediaType_GetItem(media_type, &MF_MT_PIXEL_ASPECT_RATIO, NULL)))
    {
        if (!default_type || FAILED(hr = IMFMediaType_GetUINT64(default_type, &MF_MT_PIXEL_ASPECT_RATIO, &value64)))
            value64 = (UINT64)1 << 32 | 1;
        if (FAILED(hr = IMFMediaType_SetUINT64(media_type, &MF_MT_PIXEL_ASPECT_RATIO, value64)))
            return hr;
    }

    if (FAILED(hr = IMFMediaType_GetItem(media_type, &MF_MT_SAMPLE_SIZE, NULL)))
    {
        if (!default_type || FAILED(hr = IMFMediaType_GetUINT32(default_type, &MF_MT_SAMPLE_SIZE, &value)))
        {
            if (IsEqualGUID(&subtype, &MFVideoFormat_YUY2))
                value = width * height * 2;
            else
                value = width * height * 3 / 2;
        }
        if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_SAMPLE_SIZE, value)))
            return hr;
    }

    if (FAILED(hr = IMFMediaType_GetItem(media_type, &MF_MT_DEFAULT_STRIDE, NULL)))
    {
        if (!default_type || FAILED(hr = IMFMediaType_GetUINT32(default_type, &MF_MT_DEFAULT_STRIDE, &value)))
        {
            if (IsEqualGUID(&subtype, &MFVideoFormat_YUY2))
                value = width * 2;
            else
                value = width;
        }
        if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, value)))
            return hr;
    }

    if (FAILED(hr = IMFMediaType_GetItem(media_type, &MF_MT_INTERLACE_MODE, NULL)))
    {
        if (!default_type || FAILED(hr = IMFMediaType_GetUINT32(default_type, &MF_MT_INTERLACE_MODE, &value)))
            value = MFVideoInterlace_MixedInterlaceOrProgressive;
        if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_INTERLACE_MODE, value)))
            return hr;
    }

    if (FAILED(hr = IMFMediaType_GetItem(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, NULL)))
    {
        if (!default_type || FAILED(hr = IMFMediaType_GetUINT32(default_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, &value)))
            value = 1;
        if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, value)))
            return hr;
    }

    if (FAILED(hr = IMFMediaType_GetItem(media_type, &MF_MT_VIDEO_ROTATION, NULL)))
    {
        if (!default_type || FAILED(hr = IMFMediaType_GetUINT32(default_type, &MF_MT_VIDEO_ROTATION, &value)))
            value = 0;
        if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_VIDEO_ROTATION, value)))
            return hr;
    }

    if (FAILED(hr = IMFMediaType_GetItem(media_type, &MF_MT_FIXED_SIZE_SAMPLES, NULL)))
    {
        if (!default_type || FAILED(hr = IMFMediaType_GetUINT32(default_type, &MF_MT_FIXED_SIZE_SAMPLES, &value)))
            value = 1;
        if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_FIXED_SIZE_SAMPLES, value)))
            return hr;
    }

    if (FAILED(hr = IMFMediaType_GetItem(media_type, &MF_MT_MINIMUM_DISPLAY_APERTURE, NULL)))
    {
        if (default_type && SUCCEEDED(hr = IMFMediaType_GetBlob(default_type, &MF_MT_MINIMUM_DISPLAY_APERTURE,
                (BYTE *)&aperture, sizeof(aperture), NULL)))
        {
            if (FAILED(hr = IMFMediaType_SetBlob(media_type, &MF_MT_MINIMUM_DISPLAY_APERTURE,
                    (BYTE *)&aperture, sizeof(aperture))))
                return hr;
        }
    }

    return S_OK;
}

static HRESULT WINAPI h264_decoder_QueryInterface(IMFTransform *iface, REFIID iid, void **out)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IMFTransform))
        *out = &decoder->IMFTransform_iface;
    else
    {
        *out = NULL;
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI h264_decoder_AddRef(IMFTransform *iface)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);
    ULONG refcount = InterlockedIncrement(&decoder->refcount);

    TRACE("iface %p increasing refcount to %u.\n", decoder, refcount);

    return refcount;
}

static ULONG WINAPI h264_decoder_Release(IMFTransform *iface)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);
    ULONG refcount = InterlockedDecrement(&decoder->refcount);

    TRACE("iface %p decreasing refcount to %u.\n", decoder, refcount);

    if (!refcount)
    {
        if (decoder->wg_transform)
            wg_transform_destroy(decoder->wg_transform);
        if (decoder->input_type)
            IMFMediaType_Release(decoder->input_type);
        if (decoder->output_type)
            IMFMediaType_Release(decoder->output_type);
        free(decoder);
    }

    return refcount;
}

static HRESULT WINAPI h264_decoder_GetStreamLimits(IMFTransform *iface, DWORD *input_minimum, DWORD *input_maximum,
        DWORD *output_minimum, DWORD *output_maximum)
{
    FIXME("iface %p, input_minimum %p, input_maximum %p, output_minimum %p, output_maximum %p stub!\n",
            iface, input_minimum, input_maximum, output_minimum, output_maximum);
    return E_NOTIMPL;
}

static HRESULT WINAPI h264_decoder_GetStreamCount(IMFTransform *iface, DWORD *inputs, DWORD *outputs)
{
    FIXME("iface %p, inputs %p, outputs %p stub!\n", iface, inputs, outputs);
    return E_NOTIMPL;
}

static HRESULT WINAPI h264_decoder_GetStreamIDs(IMFTransform *iface, DWORD input_size, DWORD *inputs,
        DWORD output_size, DWORD *outputs)
{
    FIXME("iface %p, input_size %u, inputs %p, output_size %u, outputs %p stub!\n",
            iface, input_size, inputs, output_size, outputs);
    return E_NOTIMPL;
}

static HRESULT WINAPI h264_decoder_GetInputStreamInfo(IMFTransform *iface, DWORD id, MFT_INPUT_STREAM_INFO *info)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %u, info %p.\n", iface, id, info);

    if (!decoder->input_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    info->hnsMaxLatency = 0;
    info->dwFlags = MFT_INPUT_STREAM_WHOLE_SAMPLES | MFT_INPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER | MFT_INPUT_STREAM_FIXED_SAMPLE_SIZE;
    info->cbSize = 0x1000;
    info->cbMaxLookahead = 0;
    info->cbAlignment = 0;

    return S_OK;
}

static HRESULT WINAPI h264_decoder_GetOutputStreamInfo(IMFTransform *iface, DWORD id, MFT_OUTPUT_STREAM_INFO *info)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);
    IMFMediaType *media_type;
    HRESULT hr;

    TRACE("iface %p, id %u, info %p.\n", iface, id, info);

    if (!decoder->input_type || !decoder->output_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    media_type = decoder->output_type;

    info->dwFlags = MFT_OUTPUT_STREAM_WHOLE_SAMPLES | MFT_OUTPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER | MFT_OUTPUT_STREAM_FIXED_SAMPLE_SIZE;
    if (FAILED(hr = IMFMediaType_GetUINT32(media_type, &MF_MT_SAMPLE_SIZE, &info->cbSize)))
        info->cbSize = 1920 * 1080 * 2;
    info->cbAlignment = 0;

    return S_OK;
}

static HRESULT WINAPI h264_decoder_GetAttributes(IMFTransform *iface, IMFAttributes **attributes)
{
    FIXME("iface %p, attributes %p stub!\n", iface, attributes);

    return MFCreateAttributes(attributes, 0);
}

static HRESULT WINAPI h264_decoder_GetInputStreamAttributes(IMFTransform *iface, DWORD id,
        IMFAttributes **attributes)
{
    FIXME("iface %p, id %u, attributes %p stub!\n", iface, id, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI h264_decoder_GetOutputStreamAttributes(IMFTransform *iface, DWORD id,
        IMFAttributes **attributes)
{
    FIXME("iface %p, id %u, attributes %p stub!\n", iface, id, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI h264_decoder_DeleteInputStream(IMFTransform *iface, DWORD id)
{
    FIXME("iface %p, id %u stub!\n", iface, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI h264_decoder_AddInputStreams(IMFTransform *iface, DWORD streams, DWORD *ids)
{
    FIXME("iface %p, streams %u, ids %p stub!\n", iface, streams, ids);
    return E_NOTIMPL;
}

static HRESULT WINAPI h264_decoder_GetInputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    IMFMediaType *media_type;
    const GUID *subtype;
    HRESULT hr;

    TRACE("iface %p, id %u, index %u, type %p.\n", iface, id, index, type);

    *type = NULL;

    if (index >= ARRAY_SIZE(h264_decoder_input_types))
        return MF_E_NO_MORE_TYPES;
    subtype = h264_decoder_input_types[index];

    if (FAILED(hr = MFCreateMediaType(&media_type)))
        return hr;

    if (SUCCEEDED(hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video)) &&
            SUCCEEDED(hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, subtype)))
        IMFMediaType_AddRef((*type = media_type));

    IMFMediaType_Release(media_type);
    return hr;
}

static HRESULT WINAPI h264_decoder_GetOutputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);
    IMFMediaType *media_type;
    const GUID *output_type;
    HRESULT hr;

    TRACE("iface %p, id %u, index %u, type %p.\n", iface, id, index, type);

    if (!decoder->input_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    *type = NULL;

    if (index >= ARRAY_SIZE(h264_decoder_output_types))
        return MF_E_NO_MORE_TYPES;
    output_type = h264_decoder_output_types[index];

    if (FAILED(hr = MFCreateMediaType(&media_type)))
        return hr;

    if (FAILED(hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video)))
        goto done;
    if (FAILED(hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, output_type)))
        goto done;

    hr = fill_output_media_type(media_type, decoder->output_type);

done:
    if (SUCCEEDED(hr))
        IMFMediaType_AddRef((*type = media_type));

    IMFMediaType_Release(media_type);
    return hr;
}

static HRESULT WINAPI h264_decoder_SetInputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);
    GUID major, subtype;
    HRESULT hr;
    ULONG i;

    TRACE("iface %p, id %u, type %p, flags %#x.\n", iface, id, type, flags);

    if (FAILED(hr = IMFMediaType_GetGUID(type, &MF_MT_MAJOR_TYPE, &major)) ||
            FAILED(hr = IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &subtype)))
        return E_INVALIDARG;

    if (!IsEqualGUID(&major, &MFMediaType_Video))
        return MF_E_INVALIDMEDIATYPE;

    for (i = 0; i < ARRAY_SIZE(h264_decoder_input_types); ++i)
        if (IsEqualGUID(&subtype, h264_decoder_input_types[i]))
            break;
    if (i == ARRAY_SIZE(h264_decoder_input_types))
        return MF_E_INVALIDMEDIATYPE;

    if (decoder->output_type)
    {
        IMFMediaType_Release(decoder->output_type);
        decoder->output_type = NULL;
    }

    if (decoder->input_type)
        IMFMediaType_Release(decoder->input_type);
    IMFMediaType_AddRef((decoder->input_type = type));

    return S_OK;
}

static HRESULT WINAPI h264_decoder_SetOutputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);
    GUID major, subtype;
    BOOL identical;
    HRESULT hr;
    ULONG i;

    TRACE("iface %p, id %u, type %p, flags %#x.\n", iface, id, type, flags);

    if (!decoder->input_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    if (FAILED(hr = IMFMediaType_GetGUID(type, &MF_MT_MAJOR_TYPE, &major)) ||
            FAILED(hr = IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &subtype)))
        return hr;

    if (!IsEqualGUID(&major, &MFMediaType_Video))
        return MF_E_INVALIDMEDIATYPE;

    for (i = 0; i < ARRAY_SIZE(h264_decoder_output_types); ++i)
        if (IsEqualGUID(&subtype, h264_decoder_output_types[i]))
            break;
    if (i == ARRAY_SIZE(h264_decoder_output_types))
        return MF_E_INVALIDMEDIATYPE;

    if (decoder->output_type)
    {
        if (SUCCEEDED(hr = IMFMediaType_Compare(decoder->output_type, (IMFAttributes *)type,
                MF_ATTRIBUTES_MATCH_THEIR_ITEMS, &identical)) && identical)
            return S_OK;
        IMFMediaType_Release(decoder->output_type);
    }

    IMFMediaType_AddRef((decoder->output_type = type));

    if (FAILED(hr = try_create_wg_transform(decoder)))
    {
        IMFMediaType_Release(decoder->output_type);
        decoder->output_type = NULL;
    }

    return hr;
}

static HRESULT WINAPI h264_decoder_GetInputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    FIXME("iface %p, id %u, type %p stub!\n", iface, id, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI h264_decoder_GetOutputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    FIXME("iface %p, id %u, type %p stub!\n", iface, id, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI h264_decoder_GetInputStatus(IMFTransform *iface, DWORD id, DWORD *flags)
{
    FIXME("iface %p, id %u, flags %p stub!\n", iface, id, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI h264_decoder_GetOutputStatus(IMFTransform *iface, DWORD *flags)
{
    FIXME("iface %p, flags %p stub!\n", iface, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI h264_decoder_SetOutputBounds(IMFTransform *iface, LONGLONG lower, LONGLONG upper)
{
    FIXME("iface %p, lower %s, upper %s stub!\n", iface,
            wine_dbgstr_longlong(lower), wine_dbgstr_longlong(upper));
    return E_NOTIMPL;
}

static HRESULT WINAPI h264_decoder_ProcessEvent(IMFTransform *iface, DWORD id, IMFMediaEvent *event)
{
    FIXME("iface %p, id %u, event %p stub!\n", iface, id, event);
    return E_NOTIMPL;
}

static HRESULT WINAPI h264_decoder_ProcessMessage(IMFTransform *iface, MFT_MESSAGE_TYPE message, ULONG_PTR param)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);

    FIXME("iface %p, message %#x, param %p stub!\n", iface, message, (void *)param);

    switch (message)
    {
    case MFT_MESSAGE_NOTIFY_BEGIN_STREAMING:
        memset(&decoder->wg_format, 0, sizeof(decoder->wg_format));
        break;
    default:
        break;
    }

    return S_OK;
}

static HRESULT WINAPI h264_decoder_ProcessInput(IMFTransform *iface, DWORD id, IMFSample *sample, DWORD flags)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);
    IMFMediaBuffer *media_buffer;
    MFT_INPUT_STREAM_INFO info;
    UINT32 buffer_size;
    BYTE *buffer;
    HRESULT hr;

    TRACE("iface %p, id %u, sample %p, flags %#x.\n", iface, id, sample, flags);

    if (FAILED(hr = IMFTransform_GetInputStreamInfo(iface, 0, &info)))
        return hr;

    if (!decoder->wg_transform)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    if (FAILED(hr = IMFSample_ConvertToContiguousBuffer(sample, &media_buffer)))
        return hr;

    if (FAILED(hr = IMFMediaBuffer_Lock(media_buffer, &buffer, NULL, &buffer_size)))
        goto done;

    hr = wg_transform_push_data(decoder->wg_transform, buffer, buffer_size);

    IMFMediaBuffer_Unlock(media_buffer);

done:
    IMFMediaBuffer_Release(media_buffer);
    return hr;
}

static HRESULT WINAPI h264_decoder_ProcessOutput(IMFTransform *iface, DWORD flags, DWORD count,
        MFT_OUTPUT_DATA_BUFFER *samples, DWORD *status)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);
    struct wg_sample wg_sample = {0};
    IMFMediaBuffer *media_buffer;
    MFT_OUTPUT_STREAM_INFO info;
    MFVideoArea aperture = {0};
    IMFMediaType *media_type;
    UINT32 align, offset;
    UINT64 framerate;
    HRESULT hr;

    TRACE("iface %p, flags %#x, count %u, samples %p, status %p.\n", iface, flags, count, samples, status);

    if (count > 1)
    {
        FIXME("Not implemented count %u\n", count);
        return E_NOTIMPL;
    }

    if (FAILED(hr = IMFTransform_GetOutputStreamInfo(iface, 0, &info)))
        return hr;

    if (!decoder->wg_transform)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    *status = 0;
    samples[0].dwStatus = 0;
    if (!samples[0].pSample)
    {
        samples[0].dwStatus = MFT_OUTPUT_DATA_BUFFER_NO_SAMPLE;
        return MF_E_TRANSFORM_NEED_MORE_INPUT;
    }

    if (FAILED(hr = IMFSample_ConvertToContiguousBuffer(samples[0].pSample, &media_buffer)))
        return hr;

    if (FAILED(hr = IMFMediaBuffer_Lock(media_buffer, &wg_sample.data, &wg_sample.size, NULL)))
        goto done;

    wg_sample.format = &decoder->wg_format;
    if (wg_sample.size < info.cbSize)
        hr = MF_E_BUFFERTOOSMALL;
    else if (SUCCEEDED(hr = wg_transform_read_data(decoder->wg_transform, &wg_sample)))
    {
        if (!(wg_sample.flags & (WG_SAMPLE_FLAG_HAS_PTS|WG_SAMPLE_FLAG_HAS_DURATION)))
        {
            IMFMediaType_GetUINT64(decoder->output_type, &MF_MT_FRAME_RATE, &framerate);
            wg_sample.pts = decoder->last_pts;
            wg_sample.duration = (UINT64)10000000 * (UINT32)framerate / (framerate >> 32);
            wg_sample.flags |= (WG_SAMPLE_FLAG_HAS_PTS|WG_SAMPLE_FLAG_HAS_DURATION);
            decoder->last_pts += wg_sample.duration;
        }

        if (wg_sample.flags & WG_SAMPLE_FLAG_HAS_PTS)
            IMFSample_SetSampleTime(samples[0].pSample, wg_sample.pts);
        if (wg_sample.flags & WG_SAMPLE_FLAG_HAS_DURATION)
            IMFSample_SetSampleDuration(samples[0].pSample, wg_sample.duration);

        if (decoder->wg_format.u.video.format == WG_VIDEO_FORMAT_NV12 &&
                (align = decoder->wg_format.u.video.height & 15))
        {
            offset = decoder->wg_format.u.video.width * decoder->wg_format.u.video.height;
            align = (16 - align) * decoder->wg_format.u.video.width;
            memmove(wg_sample.data + offset + align, wg_sample.data + offset,
                    wg_sample.size - offset);
            wg_sample.size += align;
        }

        hr = IMFMediaBuffer_SetCurrentLength(media_buffer, wg_sample.size);
    }
    else if (hr == MF_E_TRANSFORM_STREAM_CHANGE)
    {
        media_type = mf_media_type_from_wg_format(&decoder->wg_format);
        IMFMediaType_SetUINT32(media_type, &MF_MT_SAMPLE_SIZE, wg_sample.size);
        IMFMediaType_DeleteItem(media_type, &MF_MT_FRAME_RATE);
        IMFMediaType_DeleteItem(decoder->output_type, &MF_MT_DEFAULT_STRIDE);
        fill_output_media_type(media_type, decoder->output_type);

        if (decoder->wg_format.u.video.format == WG_VIDEO_FORMAT_NV12 &&
                (align = decoder->wg_format.u.video.height & 15))
        {
            aperture.Area.cx = decoder->wg_format.u.video.width;
            aperture.Area.cy = decoder->wg_format.u.video.height;
            IMFMediaType_SetBlob(media_type, &MF_MT_MINIMUM_DISPLAY_APERTURE,
                    (BYTE *)&aperture, sizeof(aperture));

            aperture.Area.cy += 16 - align;
            IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE,
                    (UINT64)aperture.Area.cx << 32 | aperture.Area.cy);
            IMFMediaType_SetUINT32(media_type, &MF_MT_SAMPLE_SIZE,
                    aperture.Area.cx * aperture.Area.cy * 3 / 2);
        }

        IMFMediaType_Release(decoder->output_type);
        decoder->output_type = media_type;

        samples[0].dwStatus |= MFT_OUTPUT_DATA_BUFFER_FORMAT_CHANGE;
        *status |= MFT_OUTPUT_DATA_BUFFER_FORMAT_CHANGE;
    }

    IMFMediaBuffer_Unlock(media_buffer);

done:
    if (FAILED(hr))
        IMFMediaBuffer_SetCurrentLength(media_buffer, 0);
    IMFMediaBuffer_Release(media_buffer);
    return hr;
}

static const IMFTransformVtbl h264_decoder_vtbl =
{
    h264_decoder_QueryInterface,
    h264_decoder_AddRef,
    h264_decoder_Release,
    h264_decoder_GetStreamLimits,
    h264_decoder_GetStreamCount,
    h264_decoder_GetStreamIDs,
    h264_decoder_GetInputStreamInfo,
    h264_decoder_GetOutputStreamInfo,
    h264_decoder_GetAttributes,
    h264_decoder_GetInputStreamAttributes,
    h264_decoder_GetOutputStreamAttributes,
    h264_decoder_DeleteInputStream,
    h264_decoder_AddInputStreams,
    h264_decoder_GetInputAvailableType,
    h264_decoder_GetOutputAvailableType,
    h264_decoder_SetInputType,
    h264_decoder_SetOutputType,
    h264_decoder_GetInputCurrentType,
    h264_decoder_GetOutputCurrentType,
    h264_decoder_GetInputStatus,
    h264_decoder_GetOutputStatus,
    h264_decoder_SetOutputBounds,
    h264_decoder_ProcessEvent,
    h264_decoder_ProcessMessage,
    h264_decoder_ProcessInput,
    h264_decoder_ProcessOutput,
};

HRESULT h264_decoder_create(REFIID riid, void **ret)
{
    struct h264_decoder *decoder;

    TRACE("riid %s, ret %p.\n", debugstr_guid(riid), ret);

    if (!(decoder = calloc(1, sizeof(*decoder))))
        return E_OUTOFMEMORY;

    decoder->IMFTransform_iface.lpVtbl = &h264_decoder_vtbl;
    decoder->refcount = 1;

    *ret = &decoder->IMFTransform_iface;
    TRACE("Created decoder %p\n", *ret);
    return S_OK;
}
