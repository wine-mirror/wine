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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

static const GUID *const h264_decoder_input_types[] =
{
    &MFVideoFormat_H264,
    &MFVideoFormat_H264_ES,
};
static const GUID *const h264_decoder_output_types[] =
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

    struct wg_format wg_format;
    struct wg_transform *wg_transform;
};

static struct h264_decoder *impl_from_IMFTransform(IMFTransform *iface)
{
    return CONTAINING_RECORD(iface, struct h264_decoder, IMFTransform_iface);
}

static HRESULT try_create_wg_transform(struct h264_decoder *decoder)
{
    struct wg_format input_format;
    struct wg_format output_format;

    if (decoder->wg_transform)
        wg_transform_destroy(decoder->wg_transform);
    decoder->wg_transform = NULL;

    mf_media_type_to_wg_format(decoder->input_type, &input_format);
    if (input_format.major_type == WG_MAJOR_TYPE_UNKNOWN)
        return MF_E_INVALIDMEDIATYPE;

    mf_media_type_to_wg_format(decoder->output_type, &output_format);
    if (output_format.major_type == WG_MAJOR_TYPE_UNKNOWN)
        return MF_E_INVALIDMEDIATYPE;

    /* Don't force any specific size, H264 streams already have the metadata for it
     * and will generate a MF_E_TRANSFORM_STREAM_CHANGE result later.
     */
    output_format.u.video.width = 0;
    output_format.u.video.height = 0;
    output_format.u.video.fps_d = 0;
    output_format.u.video.fps_n = 0;

    if (!(decoder->wg_transform = wg_transform_create(&input_format, &output_format)))
        return E_FAIL;

    return S_OK;
}

static HRESULT fill_output_media_type(struct h264_decoder *decoder, IMFMediaType *media_type)
{
    IMFMediaType *default_type = decoder->output_type;
    struct wg_format *wg_format = &decoder->wg_format;
    UINT32 value, width, height;
    UINT64 ratio;
    GUID subtype;
    HRESULT hr;

    if (FAILED(hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &subtype)))
        return hr;

    if (FAILED(hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &ratio)))
    {
        ratio = (UINT64)wg_format->u.video.width << 32 | wg_format->u.video.height;
        if (FAILED(hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, ratio)))
            return hr;
    }
    width = ratio >> 32;
    height = ratio;

    if (FAILED(hr = IMFMediaType_GetItem(media_type, &MF_MT_FRAME_RATE, NULL)))
    {
        ratio = (UINT64)wg_format->u.video.fps_n << 32 | wg_format->u.video.fps_d;
        if (FAILED(hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_RATE, ratio)))
            return hr;
    }

    if (FAILED(hr = IMFMediaType_GetItem(media_type, &MF_MT_PIXEL_ASPECT_RATIO, NULL)))
    {
        ratio = (UINT64)1 << 32 | 1; /* FIXME: read it from format */
        if (FAILED(hr = IMFMediaType_SetUINT64(media_type, &MF_MT_PIXEL_ASPECT_RATIO, ratio)))
            return hr;
    }

    if (FAILED(hr = IMFMediaType_GetItem(media_type, &MF_MT_SAMPLE_SIZE, NULL)))
    {
        if (FAILED(hr = MFCalculateImageSize(&subtype, width, height, &value)))
            return hr;
        if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_SAMPLE_SIZE, value)))
            return hr;
    }

    if (FAILED(hr = IMFMediaType_GetItem(media_type, &MF_MT_DEFAULT_STRIDE, NULL)))
    {
        if (FAILED(hr = MFGetStrideForBitmapInfoHeader(subtype.Data1, width, (LONG *)&value)))
            return hr;
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

    if (FAILED(hr = IMFMediaType_GetItem(media_type, &MF_MT_MINIMUM_DISPLAY_APERTURE, NULL))
            && !IsRectEmpty(&wg_format->u.video.padding))
    {
        MFVideoArea aperture =
        {
            .OffsetX = {.value = wg_format->u.video.padding.left},
            .OffsetY = {.value = wg_format->u.video.padding.top},
            .Area.cx = wg_format->u.video.width - wg_format->u.video.padding.right - wg_format->u.video.padding.left,
            .Area.cy = wg_format->u.video.height - wg_format->u.video.padding.bottom - wg_format->u.video.padding.top,
        };

        if (FAILED(hr = IMFMediaType_SetBlob(media_type, &MF_MT_MINIMUM_DISPLAY_APERTURE,
                (BYTE *)&aperture, sizeof(aperture))))
            return hr;
    }

    return S_OK;
}

static HRESULT WINAPI transform_QueryInterface(IMFTransform *iface, REFIID iid, void **out)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
            IsEqualGUID(iid, &IID_IMFTransform))
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

static ULONG WINAPI transform_AddRef(IMFTransform *iface)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);
    ULONG refcount = InterlockedIncrement(&decoder->refcount);

    TRACE("iface %p increasing refcount to %lu.\n", decoder, refcount);

    return refcount;
}

static ULONG WINAPI transform_Release(IMFTransform *iface)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);
    ULONG refcount = InterlockedDecrement(&decoder->refcount);

    TRACE("iface %p decreasing refcount to %lu.\n", decoder, refcount);

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

static HRESULT WINAPI transform_GetStreamLimits(IMFTransform *iface, DWORD *input_minimum,
        DWORD *input_maximum, DWORD *output_minimum, DWORD *output_maximum)
{
    FIXME("iface %p, input_minimum %p, input_maximum %p, output_minimum %p, output_maximum %p stub!\n",
            iface, input_minimum, input_maximum, output_minimum, output_maximum);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetStreamCount(IMFTransform *iface, DWORD *inputs, DWORD *outputs)
{
    FIXME("iface %p, inputs %p, outputs %p stub!\n", iface, inputs, outputs);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetStreamIDs(IMFTransform *iface, DWORD input_size,
        DWORD *inputs, DWORD output_size, DWORD *outputs)
{
    FIXME("iface %p, input_size %lu, inputs %p, output_size %lu, outputs %p stub!\n", iface,
            input_size, inputs, output_size, outputs);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetInputStreamInfo(IMFTransform *iface, DWORD id, MFT_INPUT_STREAM_INFO *info)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, info %p.\n", iface, id, info);

    if (!decoder->input_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    info->hnsMaxLatency = 0;
    info->dwFlags = MFT_INPUT_STREAM_WHOLE_SAMPLES | MFT_INPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER | MFT_INPUT_STREAM_FIXED_SAMPLE_SIZE;
    info->cbSize = 0x1000;
    info->cbMaxLookahead = 0;
    info->cbAlignment = 0;

    return S_OK;
}

static HRESULT WINAPI transform_GetOutputStreamInfo(IMFTransform *iface, DWORD id, MFT_OUTPUT_STREAM_INFO *info)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);
    UINT32 actual_width, actual_height;

    TRACE("iface %p, id %#lx, info %p.\n", iface, id, info);

    actual_width = (decoder->wg_format.u.video.width + 15) & ~15;
    actual_height = (decoder->wg_format.u.video.height + 15) & ~15;

    info->dwFlags = MFT_OUTPUT_STREAM_WHOLE_SAMPLES | MFT_OUTPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER | MFT_OUTPUT_STREAM_FIXED_SAMPLE_SIZE;
    info->cbSize = actual_width * actual_height * 2;
    info->cbAlignment = 0;

    return S_OK;
}

static HRESULT WINAPI transform_GetAttributes(IMFTransform *iface, IMFAttributes **attributes)
{
    FIXME("iface %p, attributes %p stub!\n", iface, attributes);
    return MFCreateAttributes(attributes, 0);
}

static HRESULT WINAPI transform_GetInputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    FIXME("iface %p, id %#lx, attributes %p stub!\n", iface, id, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetOutputStreamAttributes(IMFTransform *iface, DWORD id,
        IMFAttributes **attributes)
{
    FIXME("iface %p, id %#lx, attributes %p stub!\n", iface, id, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_DeleteInputStream(IMFTransform *iface, DWORD id)
{
    FIXME("iface %p, id %#lx stub!\n", iface, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_AddInputStreams(IMFTransform *iface, DWORD streams, DWORD *ids)
{
    FIXME("iface %p, streams %lu, ids %p stub!\n", iface, streams, ids);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetInputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    IMFMediaType *media_type;
    const GUID *subtype;
    HRESULT hr;

    TRACE("iface %p, id %#lx, index %#lx, type %p.\n", iface, id, index, type);

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

static HRESULT WINAPI transform_GetOutputAvailableType(IMFTransform *iface, DWORD id,
        DWORD index, IMFMediaType **type)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);
    IMFMediaType *media_type;
    const GUID *output_type;
    HRESULT hr;

    TRACE("iface %p, id %#lx, index %#lx, type %p.\n", iface, id, index, type);

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

    hr = fill_output_media_type(decoder, media_type);

done:
    if (SUCCEEDED(hr))
        IMFMediaType_AddRef((*type = media_type));

    IMFMediaType_Release(media_type);
    return hr;
}

static HRESULT WINAPI transform_SetInputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);
    GUID major, subtype;
    HRESULT hr;
    ULONG i;

    TRACE("iface %p, id %#lx, type %p, flags %#lx.\n", iface, id, type, flags);

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

static HRESULT WINAPI transform_SetOutputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);
    GUID major, subtype;
    UINT64 frame_size;
    HRESULT hr;
    ULONG i;

    TRACE("iface %p, id %#lx, type %p, flags %#lx.\n", iface, id, type, flags);

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

    if (FAILED(hr = IMFMediaType_GetUINT64(type, &MF_MT_FRAME_SIZE, &frame_size))
            || (frame_size >> 32) != decoder->wg_format.u.video.width
            || (UINT32)frame_size != decoder->wg_format.u.video.height)
        return MF_E_INVALIDMEDIATYPE;

    if (decoder->output_type)
        IMFMediaType_Release(decoder->output_type);
    IMFMediaType_AddRef((decoder->output_type = type));

    if (FAILED(hr = try_create_wg_transform(decoder)))
    {
        IMFMediaType_Release(decoder->output_type);
        decoder->output_type = NULL;
    }

    return hr;
}

static HRESULT WINAPI transform_GetInputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    FIXME("iface %p, id %#lx, type %p stub!\n", iface, id, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetOutputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);
    HRESULT hr;

    FIXME("iface %p, id %#lx, type %p stub!\n", iface, id, type);

    if (!decoder->output_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    if (FAILED(hr = MFCreateMediaType(type)))
        return hr;

    return IMFMediaType_CopyAllItems(decoder->output_type, (IMFAttributes *)*type);
}

static HRESULT WINAPI transform_GetInputStatus(IMFTransform *iface, DWORD id, DWORD *flags)
{
    FIXME("iface %p, id %#lx, flags %p stub!\n", iface, id, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetOutputStatus(IMFTransform *iface, DWORD *flags)
{
    FIXME("iface %p, flags %p stub!\n", iface, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_SetOutputBounds(IMFTransform *iface, LONGLONG lower, LONGLONG upper)
{
    FIXME("iface %p, lower %I64d, upper %I64d stub!\n", iface, lower, upper);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_ProcessEvent(IMFTransform *iface, DWORD id, IMFMediaEvent *event)
{
    FIXME("iface %p, id %#lx, event %p stub!\n", iface, id, event);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_ProcessMessage(IMFTransform *iface, MFT_MESSAGE_TYPE message, ULONG_PTR param)
{
    FIXME("iface %p, message %#x, param %Ix stub!\n", iface, message, param);
    return S_OK;
}

static HRESULT WINAPI transform_ProcessInput(IMFTransform *iface, DWORD id, IMFSample *sample, DWORD flags)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);
    struct wg_sample *wg_sample;
    MFT_INPUT_STREAM_INFO info;
    HRESULT hr;

    TRACE("iface %p, id %#lx, sample %p, flags %#lx.\n", iface, id, sample, flags);

    if (FAILED(hr = IMFTransform_GetInputStreamInfo(iface, 0, &info)))
        return hr;

    if (!decoder->wg_transform)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    if (FAILED(hr = mf_create_wg_sample(sample, &wg_sample)))
        return hr;

    hr = wg_transform_push_data(decoder->wg_transform, wg_sample);

    mf_destroy_wg_sample(wg_sample);
    return hr;
}

static HRESULT WINAPI transform_ProcessOutput(IMFTransform *iface, DWORD flags, DWORD count,
        MFT_OUTPUT_DATA_BUFFER *samples, DWORD *status)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);
    MFT_OUTPUT_STREAM_INFO info;
    struct wg_sample *wg_sample;
    struct wg_format wg_format;
    UINT64 frame_rate;
    HRESULT hr;

    TRACE("iface %p, flags %#lx, count %lu, samples %p, status %p.\n", iface, flags, count, samples, status);

    if (count != 1)
        return E_INVALIDARG;

    if (FAILED(hr = IMFTransform_GetOutputStreamInfo(iface, 0, &info)))
        return hr;

    if (!decoder->wg_transform)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    *status = 0;
    samples[0].dwStatus = 0;
    if (!samples[0].pSample) return E_INVALIDARG;

    if (FAILED(hr = mf_create_wg_sample(samples[0].pSample, &wg_sample)))
        return hr;

    if (wg_sample->max_size < info.cbSize)
    {
        mf_destroy_wg_sample(wg_sample);
        return MF_E_BUFFERTOOSMALL;
    }

    hr = wg_transform_read_data(decoder->wg_transform, wg_sample,
            &wg_format);
    mf_destroy_wg_sample(wg_sample);

    if (hr == MF_E_TRANSFORM_STREAM_CHANGE)
    {
        decoder->wg_format = wg_format;

        /* keep the frame rate that was requested, GStreamer doesn't provide any */
        if (SUCCEEDED(IMFMediaType_GetUINT64(decoder->output_type, &MF_MT_FRAME_RATE, &frame_rate)))
        {
            decoder->wg_format.u.video.fps_n = frame_rate >> 32;
            decoder->wg_format.u.video.fps_d = (UINT32)frame_rate;
        }

        samples[0].dwStatus |= MFT_OUTPUT_DATA_BUFFER_FORMAT_CHANGE;
        *status |= MFT_OUTPUT_DATA_BUFFER_FORMAT_CHANGE;
    }

    return hr;
}

static const IMFTransformVtbl transform_vtbl =
{
    transform_QueryInterface,
    transform_AddRef,
    transform_Release,
    transform_GetStreamLimits,
    transform_GetStreamCount,
    transform_GetStreamIDs,
    transform_GetInputStreamInfo,
    transform_GetOutputStreamInfo,
    transform_GetAttributes,
    transform_GetInputStreamAttributes,
    transform_GetOutputStreamAttributes,
    transform_DeleteInputStream,
    transform_AddInputStreams,
    transform_GetInputAvailableType,
    transform_GetOutputAvailableType,
    transform_SetInputType,
    transform_SetOutputType,
    transform_GetInputCurrentType,
    transform_GetOutputCurrentType,
    transform_GetInputStatus,
    transform_GetOutputStatus,
    transform_SetOutputBounds,
    transform_ProcessEvent,
    transform_ProcessMessage,
    transform_ProcessInput,
    transform_ProcessOutput,
};

HRESULT h264_decoder_create(REFIID riid, void **ret)
{
    static const struct wg_format output_format =
    {
        .major_type = WG_MAJOR_TYPE_VIDEO,
        .u.video =
        {
            .format = WG_VIDEO_FORMAT_I420,
            .width = 1920,
            .height = 1080,
        },
    };
    static const struct wg_format input_format = {.major_type = WG_MAJOR_TYPE_H264};
    struct wg_transform *transform;
    struct h264_decoder *decoder;

    TRACE("riid %s, ret %p.\n", debugstr_guid(riid), ret);

    if (!(transform = wg_transform_create(&input_format, &output_format)))
    {
        ERR_(winediag)("GStreamer doesn't support H.264 decoding, please install appropriate plugins\n");
        return E_FAIL;
    }
    wg_transform_destroy(transform);

    if (!(decoder = calloc(1, sizeof(*decoder))))
        return E_OUTOFMEMORY;

    decoder->IMFTransform_iface.lpVtbl = &transform_vtbl;
    decoder->refcount = 1;
    decoder->wg_format.u.video.format = WG_VIDEO_FORMAT_UNKNOWN;
    decoder->wg_format.u.video.width = 1920;
    decoder->wg_format.u.video.height = 1080;
    decoder->wg_format.u.video.fps_n = 30000;
    decoder->wg_format.u.video.fps_d = 1001;

    *ret = &decoder->IMFTransform_iface;
    TRACE("Created decoder %p\n", *ret);
    return S_OK;
}
