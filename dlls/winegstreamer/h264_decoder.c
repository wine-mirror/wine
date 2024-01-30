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

#include "initguid.h"

#include "codecapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

#define ALIGN_SIZE(size, alignment) (((size) + (alignment)) & ~((alignment)))

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

    IMFAttributes *attributes;
    IMFAttributes *output_attributes;

    UINT64 sample_time;
    IMFMediaType *input_type;
    MFT_INPUT_STREAM_INFO input_info;
    IMFMediaType *output_type;
    MFT_OUTPUT_STREAM_INFO output_info;
    IMFMediaType *stream_type;

    wg_transform_t wg_transform;
    struct wg_sample_queue *wg_sample_queue;

    IMFVideoSampleAllocatorEx *allocator;
    BOOL allocator_initialized;
    IMFTransform *copier;
    IMFMediaBuffer *temp_buffer;
};

static struct h264_decoder *impl_from_IMFTransform(IMFTransform *iface)
{
    return CONTAINING_RECORD(iface, struct h264_decoder, IMFTransform_iface);
}

static HRESULT try_create_wg_transform(struct h264_decoder *decoder)
{
    /* Call of Duty: Black Ops 3 doesn't care about the ProcessInput/ProcessOutput
     * return values, it calls them in a specific order and expects the decoder
     * transform to be able to queue its input buffers. We need to use a buffer list
     * to match its expectations.
     */
    struct wg_transform_attrs attrs =
    {
        .output_plane_align = 15,
        .input_queue_length = 15,
        .allow_size_change = TRUE,
    };
    struct wg_format input_format;
    struct wg_format output_format;
    UINT32 low_latency;

    if (decoder->wg_transform)
        wg_transform_destroy(decoder->wg_transform);
    decoder->wg_transform = 0;

    mf_media_type_to_wg_format(decoder->input_type, &input_format);
    if (input_format.major_type == WG_MAJOR_TYPE_UNKNOWN)
        return MF_E_INVALIDMEDIATYPE;

    mf_media_type_to_wg_format(decoder->output_type, &output_format);
    if (output_format.major_type == WG_MAJOR_TYPE_UNKNOWN)
        return MF_E_INVALIDMEDIATYPE;

    if (SUCCEEDED(IMFAttributes_GetUINT32(decoder->attributes, &MF_LOW_LATENCY, &low_latency)))
        attrs.low_latency = !!low_latency;

    if (!(decoder->wg_transform = wg_transform_create(&input_format, &output_format, &attrs)))
        return E_FAIL;

    return S_OK;
}

static HRESULT fill_output_media_type(struct h264_decoder *decoder, IMFMediaType *media_type)
{
    IMFMediaType *default_type = decoder->output_type;
    UINT32 value, width, height;
    MFVideoArea aperture;
    UINT64 ratio;
    GUID subtype;
    HRESULT hr;

    if (FAILED(hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &subtype)))
        return hr;

    if (FAILED(hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &ratio)))
    {
        if (FAILED(IMFMediaType_GetUINT64(decoder->stream_type, &MF_MT_FRAME_SIZE, &ratio)))
            ratio = (UINT64)1920 << 32 | 1080;
        if (FAILED(hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, ratio)))
            return hr;
    }
    width = ratio >> 32;
    height = ratio;

    if (FAILED(hr = IMFMediaType_GetItem(media_type, &MF_MT_FRAME_RATE, NULL)))
    {
        if (FAILED(IMFMediaType_GetUINT64(decoder->stream_type, &MF_MT_FRAME_RATE, &ratio)))
            ratio = (UINT64)30000 << 32 | 1001;
        if (FAILED(hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_RATE, ratio)))
            return hr;
    }

    if (FAILED(hr = IMFMediaType_GetItem(media_type, &MF_MT_PIXEL_ASPECT_RATIO, NULL)))
    {
        if (FAILED(IMFMediaType_GetUINT64(decoder->stream_type, &MF_MT_PIXEL_ASPECT_RATIO, &ratio)))
            ratio = (UINT64)1 << 32 | 1;
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
            && SUCCEEDED(hr = IMFMediaType_GetBlob(decoder->stream_type, &MF_MT_MINIMUM_DISPLAY_APERTURE,
            (BYTE *)&aperture, sizeof(aperture), &value)))
    {
        if (FAILED(hr = IMFMediaType_SetBlob(media_type, &MF_MT_MINIMUM_DISPLAY_APERTURE,
                (BYTE *)&aperture, sizeof(aperture))))
            return hr;
    }

    return S_OK;
}

static HRESULT init_allocator(struct h264_decoder *decoder)
{
    HRESULT hr;

    if (decoder->allocator_initialized)
        return S_OK;

    if (FAILED(hr = IMFTransform_SetInputType(decoder->copier, 0, decoder->output_type, 0)))
        return hr;
    if (FAILED(hr = IMFTransform_SetOutputType(decoder->copier, 0, decoder->output_type, 0)))
        return hr;

    if (FAILED(hr = IMFVideoSampleAllocatorEx_InitializeSampleAllocatorEx(decoder->allocator, 10, 10,
            decoder->attributes, decoder->output_type)))
        return hr;
    decoder->allocator_initialized = TRUE;
    return S_OK;
}

static void uninit_allocator(struct h264_decoder *decoder)
{
    IMFVideoSampleAllocatorEx_UninitializeSampleAllocator(decoder->allocator);
    decoder->allocator_initialized = FALSE;
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
        IMFTransform_Release(decoder->copier);
        IMFVideoSampleAllocatorEx_Release(decoder->allocator);
        if (decoder->temp_buffer)
            IMFMediaBuffer_Release(decoder->temp_buffer);
        if (decoder->wg_transform)
            wg_transform_destroy(decoder->wg_transform);
        if (decoder->input_type)
            IMFMediaType_Release(decoder->input_type);
        if (decoder->output_type)
            IMFMediaType_Release(decoder->output_type);
        if (decoder->output_attributes)
            IMFAttributes_Release(decoder->output_attributes);
        if (decoder->attributes)
            IMFAttributes_Release(decoder->attributes);
        wg_sample_queue_destroy(decoder->wg_sample_queue);
        free(decoder);
    }

    return refcount;
}

static HRESULT WINAPI transform_GetStreamLimits(IMFTransform *iface, DWORD *input_minimum,
        DWORD *input_maximum, DWORD *output_minimum, DWORD *output_maximum)
{
    TRACE("iface %p, input_minimum %p, input_maximum %p, output_minimum %p, output_maximum %p.\n",
            iface, input_minimum, input_maximum, output_minimum, output_maximum);
    *input_minimum = *input_maximum = *output_minimum = *output_maximum = 1;
    return S_OK;
}

static HRESULT WINAPI transform_GetStreamCount(IMFTransform *iface, DWORD *inputs, DWORD *outputs)
{
    TRACE("iface %p, inputs %p, outputs %p.\n", iface, inputs, outputs);
    *inputs = *outputs = 1;
    return S_OK;
}

static HRESULT WINAPI transform_GetStreamIDs(IMFTransform *iface, DWORD input_size, DWORD *inputs,
        DWORD output_size, DWORD *outputs)
{
    TRACE("iface %p, input_size %lu, inputs %p, output_size %lu, outputs %p.\n", iface,
            input_size, inputs, output_size, outputs);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetInputStreamInfo(IMFTransform *iface, DWORD id, MFT_INPUT_STREAM_INFO *info)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, info %p.\n", iface, id, info);

    *info = decoder->input_info;
    return S_OK;
}

static HRESULT WINAPI transform_GetOutputStreamInfo(IMFTransform *iface, DWORD id, MFT_OUTPUT_STREAM_INFO *info)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, info %p.\n", iface, id, info);

    *info = decoder->output_info;
    return S_OK;
}

static HRESULT WINAPI transform_GetAttributes(IMFTransform *iface, IMFAttributes **attributes)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);

    FIXME("iface %p, attributes %p semi-stub!\n", iface, attributes);

    if (!attributes)
        return E_POINTER;

    IMFAttributes_AddRef((*attributes = decoder->attributes));
    return S_OK;
}

static HRESULT WINAPI transform_GetInputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    TRACE("iface %p, id %#lx, attributes %p.\n", iface, id, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetOutputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);

    FIXME("iface %p, id %#lx, attributes %p semi-stub!\n", iface, id, attributes);

    if (!attributes)
        return E_POINTER;
    if (id)
        return MF_E_INVALIDSTREAMNUMBER;

    IMFAttributes_AddRef((*attributes = decoder->output_attributes));
    return S_OK;
}

static HRESULT WINAPI transform_DeleteInputStream(IMFTransform *iface, DWORD id)
{
    TRACE("iface %p, id %#lx.\n", iface, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_AddInputStreams(IMFTransform *iface, DWORD streams, DWORD *ids)
{
    TRACE("iface %p, streams %lu, ids %p.\n", iface, streams, ids);
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
    UINT64 frame_size;
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
    if (flags & MFT_SET_TYPE_TEST_ONLY)
        return S_OK;

    if (decoder->output_type)
    {
        IMFMediaType_Release(decoder->output_type);
        decoder->output_type = NULL;
    }

    if (decoder->input_type)
        IMFMediaType_Release(decoder->input_type);
    IMFMediaType_AddRef((decoder->input_type = type));

    if (SUCCEEDED(IMFMediaType_GetUINT64(type, &MF_MT_FRAME_SIZE, &frame_size)))
    {
        if (FAILED(hr = IMFMediaType_SetUINT64(decoder->stream_type, &MF_MT_FRAME_SIZE, frame_size)))
            WARN("Failed to update stream type frame size, hr %#lx\n", hr);
        decoder->output_info.cbSize = (frame_size >> 32) * (UINT32)frame_size * 2;
    }

    return S_OK;
}

static HRESULT WINAPI transform_SetOutputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);
    UINT64 frame_size, stream_frame_size;
    GUID major, subtype;
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

    if (FAILED(hr = IMFMediaType_GetUINT64(type, &MF_MT_FRAME_SIZE, &frame_size)))
        return MF_E_INVALIDMEDIATYPE;
    if (SUCCEEDED(IMFMediaType_GetUINT64(decoder->stream_type, &MF_MT_FRAME_SIZE, &stream_frame_size))
            && frame_size != stream_frame_size)
        return MF_E_INVALIDMEDIATYPE;
    if (flags & MFT_SET_TYPE_TEST_ONLY)
        return S_OK;

    if (decoder->output_type)
        IMFMediaType_Release(decoder->output_type);
    IMFMediaType_AddRef((decoder->output_type = type));

    if (decoder->wg_transform)
    {
        struct wg_format output_format;
        mf_media_type_to_wg_format(decoder->output_type, &output_format);

        if (output_format.major_type == WG_MAJOR_TYPE_UNKNOWN
                || !wg_transform_set_output_format(decoder->wg_transform, &output_format))
        {
            IMFMediaType_Release(decoder->output_type);
            decoder->output_type = NULL;
            return MF_E_INVALIDMEDIATYPE;
        }
    }
    else if (FAILED(hr = try_create_wg_transform(decoder)))
    {
        IMFMediaType_Release(decoder->output_type);
        decoder->output_type = NULL;
    }

    return hr;
}

static HRESULT WINAPI transform_GetInputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);
    HRESULT hr;

    TRACE("iface %p, id %#lx, type %p\n", iface, id, type);

    if (!decoder->input_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    if (FAILED(hr = MFCreateMediaType(type)))
        return hr;

    return IMFMediaType_CopyAllItems(decoder->input_type, (IMFAttributes *)*type);
}

static HRESULT WINAPI transform_GetOutputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);
    HRESULT hr;

    TRACE("iface %p, id %#lx, type %p\n", iface, id, type);

    if (!decoder->output_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    if (FAILED(hr = MFCreateMediaType(type)))
        return hr;

    return IMFMediaType_CopyAllItems(decoder->output_type, (IMFAttributes *)*type);
}

static HRESULT WINAPI transform_GetInputStatus(IMFTransform *iface, DWORD id, DWORD *flags)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, flags %p.\n", iface, id, flags);

    if (!decoder->wg_transform)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    *flags = MFT_INPUT_STATUS_ACCEPT_DATA;
    return S_OK;
}

static HRESULT WINAPI transform_GetOutputStatus(IMFTransform *iface, DWORD *flags)
{
    FIXME("iface %p, flags %p stub!\n", iface, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_SetOutputBounds(IMFTransform *iface, LONGLONG lower, LONGLONG upper)
{
    TRACE("iface %p, lower %I64d, upper %I64d.\n", iface, lower, upper);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_ProcessEvent(IMFTransform *iface, DWORD id, IMFMediaEvent *event)
{
    FIXME("iface %p, id %#lx, event %p stub!\n", iface, id, event);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_ProcessMessage(IMFTransform *iface, MFT_MESSAGE_TYPE message, ULONG_PTR param)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);
    HRESULT hr;

    TRACE("iface %p, message %#x, param %Ix.\n", iface, message, param);

    switch (message)
    {
    case MFT_MESSAGE_SET_D3D_MANAGER:
        if (FAILED(hr = IMFVideoSampleAllocatorEx_SetDirectXManager(decoder->allocator, (IUnknown *)param)))
            return hr;

        uninit_allocator(decoder);
        if (param)
            decoder->output_info.dwFlags |= MFT_OUTPUT_STREAM_PROVIDES_SAMPLES;
        else
            decoder->output_info.dwFlags &= ~MFT_OUTPUT_STREAM_PROVIDES_SAMPLES;
        return S_OK;

    case MFT_MESSAGE_COMMAND_DRAIN:
        return wg_transform_drain(decoder->wg_transform);

    case MFT_MESSAGE_COMMAND_FLUSH:
        return wg_transform_flush(decoder->wg_transform);

    default:
        FIXME("Ignoring message %#x.\n", message);
        return S_OK;
    }
}

static HRESULT WINAPI transform_ProcessInput(IMFTransform *iface, DWORD id, IMFSample *sample, DWORD flags)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, sample %p, flags %#lx.\n", iface, id, sample, flags);

    if (!decoder->wg_transform)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    return wg_transform_push_mf(decoder->wg_transform, sample, decoder->wg_sample_queue);
}

static HRESULT output_sample(struct h264_decoder *decoder, IMFSample **out, IMFSample *src_sample)
{
    MFT_OUTPUT_DATA_BUFFER output[1];
    IMFSample *sample;
    DWORD status;
    HRESULT hr;

    if (FAILED(hr = init_allocator(decoder)))
    {
        ERR("Failed to initialize allocator, hr %#lx.\n", hr);
        return hr;
    }
    if (FAILED(hr = IMFVideoSampleAllocatorEx_AllocateSample(decoder->allocator, &sample)))
        return hr;

    if (FAILED(hr = IMFTransform_ProcessInput(decoder->copier, 0, src_sample, 0)))
    {
        IMFSample_Release(sample);
        return hr;
    }
    output[0].pSample = sample;
    if (FAILED(hr = IMFTransform_ProcessOutput(decoder->copier, 0, 1, output, &status)))
    {
        IMFSample_Release(sample);
        return hr;
    }
    *out = sample;
    return S_OK;
}

static HRESULT handle_stream_type_change(struct h264_decoder *decoder, const struct wg_format *format)
{
    UINT64 frame_size, frame_rate;
    HRESULT hr;

    if (decoder->stream_type)
        IMFMediaType_Release(decoder->stream_type);
    if (!(decoder->stream_type = mf_media_type_from_wg_format(format)))
        return E_OUTOFMEMORY;

    if (SUCCEEDED(IMFMediaType_GetUINT64(decoder->output_type, &MF_MT_FRAME_RATE, &frame_rate))
            && FAILED(hr = IMFMediaType_SetUINT64(decoder->stream_type, &MF_MT_FRAME_RATE, frame_rate)))
            WARN("Failed to update stream type frame size, hr %#lx\n", hr);

    if (FAILED(hr = IMFMediaType_GetUINT64(decoder->stream_type, &MF_MT_FRAME_SIZE, &frame_size)))
        return hr;
    decoder->output_info.cbSize = (frame_size >> 32) * (UINT32)frame_size * 2;
    uninit_allocator(decoder);

    return MF_E_TRANSFORM_STREAM_CHANGE;
}

static HRESULT WINAPI transform_ProcessOutput(IMFTransform *iface, DWORD flags, DWORD count,
        MFT_OUTPUT_DATA_BUFFER *samples, DWORD *status)
{
    struct h264_decoder *decoder = impl_from_IMFTransform(iface);
    struct wg_format wg_format;
    UINT32 sample_size;
    LONGLONG duration;
    IMFSample *sample;
    UINT64 frame_size, frame_rate;
    GUID subtype;
    DWORD size;
    HRESULT hr;

    TRACE("iface %p, flags %#lx, count %lu, samples %p, status %p.\n", iface, flags, count, samples, status);

    if (count != 1)
        return E_INVALIDARG;

    if (!decoder->wg_transform)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    *status = samples->dwStatus = 0;
    if (!(sample = samples->pSample) && !(decoder->output_info.dwFlags & MFT_OUTPUT_STREAM_PROVIDES_SAMPLES))
        return E_INVALIDARG;

    if (FAILED(hr = IMFMediaType_GetGUID(decoder->output_type, &MF_MT_SUBTYPE, &subtype)))
        return hr;
    if (FAILED(hr = IMFMediaType_GetUINT64(decoder->output_type, &MF_MT_FRAME_SIZE, &frame_size)))
        return hr;
    if (FAILED(hr = MFCalculateImageSize(&subtype, frame_size >> 32, (UINT32)frame_size, &sample_size)))
        return hr;

    if (decoder->output_info.dwFlags & MFT_OUTPUT_STREAM_PROVIDES_SAMPLES)
    {
        if (decoder->temp_buffer)
        {
            if (FAILED(IMFMediaBuffer_GetMaxLength(decoder->temp_buffer, &size)) || size < sample_size)
            {
                IMFMediaBuffer_Release(decoder->temp_buffer);
                decoder->temp_buffer = NULL;
            }
        }
        if (!decoder->temp_buffer && FAILED(hr = MFCreateMemoryBuffer(sample_size, &decoder->temp_buffer)))
            return hr;
        if (FAILED(hr = MFCreateSample(&sample)))
            return hr;
        if (FAILED(hr = IMFSample_AddBuffer(sample, decoder->temp_buffer)))
        {
            IMFSample_Release(sample);
            return hr;
        }
    }

    if (SUCCEEDED(hr = wg_transform_read_mf(decoder->wg_transform, sample,
            sample_size, &wg_format, &samples->dwStatus)))
    {
        wg_sample_queue_flush(decoder->wg_sample_queue, false);

        if (FAILED(IMFMediaType_GetUINT64(decoder->input_type, &MF_MT_FRAME_RATE, &frame_rate)))
            frame_rate = (UINT64)30000 << 32 | 1001;

        duration = (UINT64)10000000 * (UINT32)frame_rate / (frame_rate >> 32);
        if (FAILED(IMFSample_SetSampleTime(sample, decoder->sample_time)))
            WARN("Failed to set sample time\n");
        if (FAILED(IMFSample_SetSampleDuration(sample, duration)))
            WARN("Failed to set sample duration\n");
        decoder->sample_time += duration;
    }

    if (hr == MF_E_TRANSFORM_STREAM_CHANGE)
    {
        samples[0].dwStatus |= MFT_OUTPUT_DATA_BUFFER_FORMAT_CHANGE;
        *status |= MFT_OUTPUT_DATA_BUFFER_FORMAT_CHANGE;
        hr = handle_stream_type_change(decoder, &wg_format);
    }

    if (decoder->output_info.dwFlags & MFT_OUTPUT_STREAM_PROVIDES_SAMPLES)
    {
        if (hr == S_OK && FAILED(hr = output_sample(decoder, &samples->pSample, sample)))
            ERR("Failed to output sample, hr %#lx.\n", hr);
        IMFSample_Release(sample);
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
    static const struct wg_format input_format = {.major_type = WG_MAJOR_TYPE_VIDEO_H264};
    struct wg_transform_attrs attrs = {0};
    wg_transform_t transform;
    struct h264_decoder *decoder;
    HRESULT hr;

    TRACE("riid %s, ret %p.\n", debugstr_guid(riid), ret);

    if (!(transform = wg_transform_create(&input_format, &output_format, &attrs)))
    {
        ERR_(winediag)("GStreamer doesn't support H.264 decoding, please install appropriate plugins\n");
        return E_FAIL;
    }
    wg_transform_destroy(transform);

    if (!(decoder = calloc(1, sizeof(*decoder))))
        return E_OUTOFMEMORY;

    decoder->IMFTransform_iface.lpVtbl = &transform_vtbl;
    decoder->refcount = 1;

    decoder->input_info.dwFlags = MFT_INPUT_STREAM_WHOLE_SAMPLES | MFT_INPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER
            | MFT_INPUT_STREAM_FIXED_SAMPLE_SIZE;
    decoder->input_info.cbSize = 0x1000;
    decoder->output_info.dwFlags = MFT_OUTPUT_STREAM_WHOLE_SAMPLES | MFT_OUTPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER
            | MFT_OUTPUT_STREAM_FIXED_SAMPLE_SIZE;
    decoder->output_info.cbSize = 1920 * 1088 * 2;

    if (FAILED(hr = MFCreateMediaType(&decoder->stream_type)))
        goto failed;
    if (FAILED(hr = MFCreateAttributes(&decoder->attributes, 16)))
        goto failed;
    if (FAILED(hr = IMFAttributes_SetUINT32(decoder->attributes, &MF_LOW_LATENCY, 0)))
        goto failed;
    if (FAILED(hr = IMFAttributes_SetUINT32(decoder->attributes, &MF_SA_D3D11_AWARE, TRUE)))
        goto failed;
    if (FAILED(hr = IMFAttributes_SetUINT32(decoder->attributes, &AVDecVideoAcceleration_H264, TRUE)))
        goto failed;

    if (FAILED(hr = MFCreateAttributes(&decoder->output_attributes, 0)))
        goto failed;
    if (FAILED(hr = wg_sample_queue_create(&decoder->wg_sample_queue)))
        goto failed;
    if (FAILED(hr = MFCreateVideoSampleAllocatorEx(&IID_IMFVideoSampleAllocatorEx, (void **)&decoder->allocator)))
        goto failed;
    if (FAILED(hr = MFCreateSampleCopierMFT(&decoder->copier)))
        goto failed;

    *ret = &decoder->IMFTransform_iface;
    TRACE("Created decoder %p\n", *ret);
    return S_OK;

failed:
    if (decoder->allocator)
        IMFVideoSampleAllocatorEx_Release(decoder->allocator);
    if (decoder->wg_sample_queue)
        wg_sample_queue_destroy(decoder->wg_sample_queue);
    if (decoder->output_attributes)
        IMFAttributes_Release(decoder->output_attributes);
    if (decoder->attributes)
        IMFAttributes_Release(decoder->attributes);
    if (decoder->stream_type)
        IMFMediaType_Release(decoder->stream_type);
    free(decoder);
    return hr;
}
