/* Generic Video Decoder Transform
 *
 * Copyright 2022 Rémi Bernon for CodeWeavers
 * Copyright 2023 Shaun Ren for CodeWeavers
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

DEFINE_MEDIATYPE_GUID(MFVideoFormat_IV50, MAKEFOURCC('I','V','5','0'));

static const GUID *const input_types[] =
{
    &MFVideoFormat_IV50,
};
static const GUID *const output_types[] =
{
    &MFVideoFormat_YV12,
    &MFVideoFormat_YUY2,
    &MFVideoFormat_NV11,
    &MFVideoFormat_NV12,
    &MFVideoFormat_RGB32,
    &MFVideoFormat_RGB24,
    &MFVideoFormat_RGB565,
    &MFVideoFormat_RGB555,
    &MFVideoFormat_RGB8,
};

struct video_decoder
{
    IMFTransform IMFTransform_iface;
    LONG refcount;

    IMFMediaType *input_type;
    IMFMediaType *output_type;

    struct wg_format wg_format;
    wg_transform_t wg_transform;
    struct wg_sample_queue *wg_sample_queue;
};

static struct video_decoder *impl_from_IMFTransform(IMFTransform *iface)
{
    return CONTAINING_RECORD(iface, struct video_decoder, IMFTransform_iface);
}

static HRESULT try_create_wg_transform(struct video_decoder *decoder)
{
    struct wg_transform_attrs attrs = {0};
    struct wg_format input_format;
    struct wg_format output_format;

    if (decoder->wg_transform)
        wg_transform_destroy(decoder->wg_transform);
    decoder->wg_transform = 0;

    mf_media_type_to_wg_format(decoder->input_type, &input_format);
    if (input_format.major_type == WG_MAJOR_TYPE_UNKNOWN)
        return MF_E_INVALIDMEDIATYPE;

    mf_media_type_to_wg_format(decoder->output_type, &output_format);
    if (output_format.major_type == WG_MAJOR_TYPE_UNKNOWN)
        return MF_E_INVALIDMEDIATYPE;

    if (!(decoder->wg_transform = wg_transform_create(&input_format, &output_format, &attrs)))
    {
        ERR("Failed to create transform with input major_type %u.\n", input_format.major_type);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI transform_QueryInterface(IMFTransform *iface, REFIID iid, void **out)
{
    struct video_decoder *decoder = impl_from_IMFTransform(iface);

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
    struct video_decoder *decoder = impl_from_IMFTransform(iface);
    ULONG refcount = InterlockedIncrement(&decoder->refcount);

    TRACE("iface %p increasing refcount to %lu.\n", decoder, refcount);

    return refcount;
}

static ULONG WINAPI transform_Release(IMFTransform *iface)
{
    struct video_decoder *decoder = impl_from_IMFTransform(iface);
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

        wg_sample_queue_destroy(decoder->wg_sample_queue);
        free(decoder);
    }

    return refcount;
}

static HRESULT WINAPI transform_GetStreamLimits(IMFTransform *iface, DWORD *input_minimum,
        DWORD *input_maximum, DWORD *output_minimum, DWORD *output_maximum)
{
    FIXME("iface %p, input_minimum %p, input_maximum %p, output_minimum %p, output_maximum %p.\n",
            iface, input_minimum, input_maximum, output_minimum, output_maximum);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetStreamCount(IMFTransform *iface, DWORD *inputs, DWORD *outputs)
{
    FIXME("iface %p, inputs %p, outputs %p.\n", iface, inputs, outputs);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetStreamIDs(IMFTransform *iface, DWORD input_size, DWORD *inputs,
        DWORD output_size, DWORD *outputs)
{
    FIXME("iface %p, input_size %lu, inputs %p, output_size %lu, outputs %p.\n",
            iface, input_size, inputs, output_size, outputs);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetInputStreamInfo(IMFTransform *iface, DWORD id, MFT_INPUT_STREAM_INFO *info)
{
    FIXME("iface %p, id %#lx, info %p.\n", iface, id, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetOutputStreamInfo(IMFTransform *iface, DWORD id, MFT_OUTPUT_STREAM_INFO *info)
{
    FIXME("iface %p, id %#lx, info %p.\n", iface, id, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetAttributes(IMFTransform *iface, IMFAttributes **attributes)
{
    FIXME("iface %p, attributes %p semi-stub!\n", iface, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetInputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    FIXME("iface %p, id %#lx, attributes %p.\n", iface, id, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetOutputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    FIXME("iface %p, id %#lx, attributes %p stub!\n", iface, id, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_DeleteInputStream(IMFTransform *iface, DWORD id)
{
    FIXME("iface %p, id %#lx.\n", iface, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_AddInputStreams(IMFTransform *iface, DWORD streams, DWORD *ids)
{
    FIXME("iface %p, streams %lu, ids %p.\n", iface, streams, ids);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetInputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    FIXME("iface %p, id %#lx, index %#lx, type %p.\n", iface, id, index, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetOutputAvailableType(IMFTransform *iface, DWORD id,
        DWORD index, IMFMediaType **type)
{
    FIXME("iface %p, id %#lx, index %#lx, type %p.\n", iface, id, index, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_SetInputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct video_decoder *decoder = impl_from_IMFTransform(iface);
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

    for (i = 0; i < ARRAY_SIZE(input_types); ++i)
        if (IsEqualGUID(&subtype, input_types[i]))
            break;
    if (i == ARRAY_SIZE(input_types))
        return MF_E_INVALIDMEDIATYPE;

    if (FAILED(hr = IMFMediaType_GetUINT64(type, &MF_MT_FRAME_SIZE, &frame_size)) ||
            (frame_size >> 32) == 0 || (UINT32)frame_size == 0)
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

    return S_OK;
}

static HRESULT WINAPI transform_SetOutputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct video_decoder *decoder = impl_from_IMFTransform(iface);
    GUID major, subtype;
    UINT64 frame_size;
    struct wg_format output_format;
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

    for (i = 0; i < ARRAY_SIZE(output_types); ++i)
        if (IsEqualGUID(&subtype, output_types[i]))
            break;
    if (i == ARRAY_SIZE(output_types))
        return MF_E_INVALIDMEDIATYPE;

    if (FAILED(hr = IMFMediaType_GetUINT64(type, &MF_MT_FRAME_SIZE, &frame_size)))
        return hr;

    if (flags & MFT_SET_TYPE_TEST_ONLY)
        return S_OK;

    if (decoder->output_type)
        IMFMediaType_Release(decoder->output_type);
    IMFMediaType_AddRef((decoder->output_type = type));

    if (decoder->wg_transform)
    {
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
        return hr;
    }

    decoder->wg_format.u.video.width = frame_size >> 32;
    decoder->wg_format.u.video.height = (UINT32)frame_size;

    return hr;
}

static HRESULT WINAPI transform_GetInputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    FIXME("iface %p, id %#lx, type %p stub!\n", iface, id, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetOutputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    FIXME("iface %p, id %#lx, type %p stub!\n", iface, id, type);
    return E_NOTIMPL;
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
    FIXME("iface %p, message %#x, param %Ix stub!\n", iface, message, param);
    return S_OK;
}

static HRESULT WINAPI transform_ProcessInput(IMFTransform *iface, DWORD id, IMFSample *sample, DWORD flags)
{
    struct video_decoder *decoder = impl_from_IMFTransform(iface);
    HRESULT hr;

    TRACE("iface %p, id %#lx, sample %p, flags %#lx.\n", iface, id, sample, flags);

    if (!decoder->wg_transform)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    hr = wg_transform_push_mf(decoder->wg_transform, sample, decoder->wg_sample_queue);

    return hr;
}

static HRESULT WINAPI transform_ProcessOutput(IMFTransform *iface, DWORD flags, DWORD count,
        MFT_OUTPUT_DATA_BUFFER *samples, DWORD *status)
{
    struct video_decoder *decoder = impl_from_IMFTransform(iface);
    struct wg_format wg_format;
    UINT32 sample_size;
    UINT64 frame_rate;
    GUID subtype;
    HRESULT hr;

    TRACE("iface %p, flags %#lx, count %lu, samples %p, status %p.\n", iface, flags, count, samples, status);

    if (count != 1)
        return E_INVALIDARG;

    if (!decoder->wg_transform)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    *status = samples->dwStatus = 0;
    if (!samples->pSample)
        return E_INVALIDARG;

    if (FAILED(hr = IMFMediaType_GetGUID(decoder->output_type, &MF_MT_SUBTYPE, &subtype)))
        return hr;
    if (FAILED(hr = MFCalculateImageSize(&subtype, decoder->wg_format.u.video.width,
            decoder->wg_format.u.video.height, &sample_size)))
        return hr;

    if (SUCCEEDED(hr = wg_transform_read_mf(decoder->wg_transform, samples->pSample,
            sample_size, &wg_format, &samples->dwStatus)))
        wg_sample_queue_flush(decoder->wg_sample_queue, false);

    if (hr == MF_E_TRANSFORM_STREAM_CHANGE)
    {
        decoder->wg_format = wg_format;

        if (FAILED(hr = MFCalculateImageSize(&subtype, decoder->wg_format.u.video.width,
            decoder->wg_format.u.video.height, &sample_size)))
            return hr;

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

HRESULT WINAPI winegstreamer_create_video_decoder(IMFTransform **out)
{
    struct video_decoder *decoder;
    HRESULT hr;

    TRACE("out %p.\n", out);

    if (!init_gstreamer())
        return E_FAIL;

    if (!(decoder = calloc(1, sizeof(*decoder))))
        return E_OUTOFMEMORY;

    decoder->IMFTransform_iface.lpVtbl = &transform_vtbl;
    decoder->refcount = 1;

    decoder->wg_format.u.video.fps_d = 1;
    decoder->wg_format.u.video.fps_n = 1;

    if (FAILED(hr = wg_sample_queue_create(&decoder->wg_sample_queue)))
        goto failed;

    *out = &decoder->IMFTransform_iface;
    TRACE("created decoder %p.\n", *out);
    return S_OK;

failed:
    free(decoder);
    return hr;
}
