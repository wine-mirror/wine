/* Generic Video Encoder Transform
 *
 * Copyright 2024 Ziqing Hui for CodeWeavers
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
#include "mediaerr.h"
#include "wmcodecdsp.h"

#include "wine/debug.h"

#include "initguid.h"

#include "icodecapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

struct video_encoder
{
    IMFTransform IMFTransform_iface;
    ICodecAPI ICodecAPI_iface;
    LONG refcount;

    const GUID *const *input_types;
    UINT input_type_count;
    const GUID *const *output_types;
    UINT output_type_count;

    IMFMediaType *input_type;
    MFT_INPUT_STREAM_INFO input_info;
    IMFMediaType *output_type;
    MFT_OUTPUT_STREAM_INFO output_info;

    IMFAttributes *attributes;

    wg_transform_t wg_transform;
    struct wg_transform_attrs wg_transform_attrs;
    struct wg_sample_queue *wg_sample_queue;
};

static inline struct video_encoder *impl_from_IMFTransform(IMFTransform *iface)
{
    return CONTAINING_RECORD(iface, struct video_encoder, IMFTransform_iface);
}

static inline struct video_encoder *impl_from_ICodecAPI(ICodecAPI *iface)
{
    return CONTAINING_RECORD(iface, struct video_encoder, ICodecAPI_iface);
}

static HRESULT video_encoder_create_input_type(struct video_encoder *encoder,
        const GUID *subtype, IMFMediaType **out)
{
    IMFVideoMediaType *input_type;
    UINT64 ratio;
    UINT32 value;
    HRESULT hr;

    if (FAILED(hr = MFCreateVideoMediaTypeFromSubtype(subtype, &input_type)))
        return hr;

    if (FAILED(hr = IMFMediaType_GetUINT64(encoder->output_type, &MF_MT_FRAME_SIZE, &ratio))
            || FAILED(hr = IMFVideoMediaType_SetUINT64(input_type, &MF_MT_FRAME_SIZE, ratio)))
        goto done;

    if (FAILED(hr = IMFMediaType_GetUINT64(encoder->output_type, &MF_MT_FRAME_RATE, &ratio))
            || FAILED(hr = IMFVideoMediaType_SetUINT64(input_type, &MF_MT_FRAME_RATE, ratio)))
        goto done;

    if (FAILED(hr = IMFMediaType_GetUINT32(encoder->output_type, &MF_MT_INTERLACE_MODE, &value))
            || FAILED(hr = IMFVideoMediaType_SetUINT32(input_type, &MF_MT_INTERLACE_MODE, value)))
        goto done;

    if (FAILED(IMFMediaType_GetUINT32(encoder->output_type, &MF_MT_VIDEO_NOMINAL_RANGE, &value)))
        value = MFNominalRange_Wide;
    if (FAILED(hr = IMFVideoMediaType_SetUINT32(input_type, &MF_MT_VIDEO_NOMINAL_RANGE, value)))
        goto done;

    if (FAILED(IMFMediaType_GetUINT64(encoder->output_type, &MF_MT_PIXEL_ASPECT_RATIO, &ratio)))
        ratio = (UINT64)1 << 32 | 1;
    if (FAILED(hr = IMFVideoMediaType_SetUINT64(input_type, &MF_MT_PIXEL_ASPECT_RATIO, ratio)))
        goto done;

    IMFMediaType_AddRef((*out = (IMFMediaType *)input_type));

done:
    IMFVideoMediaType_Release(input_type);
    return hr;
}

static HRESULT video_encoder_try_create_wg_transform(struct video_encoder *encoder)
{
    if (encoder->wg_transform)
    {
        wg_transform_destroy(encoder->wg_transform);
        encoder->wg_transform = 0;
    }

    return wg_transform_create_mf(encoder->input_type, encoder->output_type,
            &encoder->wg_transform_attrs, &encoder->wg_transform);
}

static HRESULT WINAPI transform_QueryInterface(IMFTransform *iface, REFIID iid, void **out)
{
    struct video_encoder *encoder = impl_from_IMFTransform(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IMFTransform) || IsEqualGUID(iid, &IID_IUnknown))
        *out = &encoder->IMFTransform_iface;
    else if (IsEqualGUID(iid, &IID_ICodecAPI))
        *out = &encoder->ICodecAPI_iface;
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
    struct video_encoder *encoder = impl_from_IMFTransform(iface);
    ULONG refcount = InterlockedIncrement(&encoder->refcount);

    TRACE("iface %p increasing refcount to %lu.\n", encoder, refcount);

    return refcount;
}

static ULONG WINAPI transform_Release(IMFTransform *iface)
{
    struct video_encoder *encoder = impl_from_IMFTransform(iface);
    ULONG refcount = InterlockedDecrement(&encoder->refcount);

    TRACE("iface %p decreasing refcount to %lu.\n", encoder, refcount);

    if (!refcount)
    {
        if (encoder->input_type)
            IMFMediaType_Release(encoder->input_type);
        if (encoder->output_type)
            IMFMediaType_Release(encoder->output_type);
        IMFAttributes_Release(encoder->attributes);
        if (encoder->wg_transform)
            wg_transform_destroy(encoder->wg_transform);
        wg_sample_queue_destroy(encoder->wg_sample_queue);
        free(encoder);
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
    FIXME("iface %p, input_size %lu, inputs %p, output_size %lu, outputs %p.\n", iface,
            input_size, inputs, output_size, outputs);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetInputStreamInfo(IMFTransform *iface, DWORD id, MFT_INPUT_STREAM_INFO *info)
{
    struct video_encoder *encoder = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, info %p.\n", iface, id, info);

    *info = encoder->input_info;
    return S_OK;
}

static HRESULT WINAPI transform_GetOutputStreamInfo(IMFTransform *iface, DWORD id, MFT_OUTPUT_STREAM_INFO *info)
{
    struct video_encoder *encoder = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, info %p.\n", iface, id, info);

    *info = encoder->output_info;
    return S_OK;
}

static HRESULT WINAPI transform_GetAttributes(IMFTransform *iface, IMFAttributes **attributes)
{
    struct video_encoder *encoder = impl_from_IMFTransform(iface);

    TRACE("iface %p, attributes %p.\n", iface, attributes);

    if (!attributes)
        return E_POINTER;

    IMFAttributes_AddRef((*attributes = encoder->attributes));
    return S_OK;
}

static HRESULT WINAPI transform_GetInputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    FIXME("iface %p, id %#lx, attributes %p.\n", iface, id, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetOutputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    FIXME("iface %p, id %#lx, attributes %p.\n", iface, id, attributes);
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
    struct video_encoder *encoder = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, index %#lx, type %p.\n", iface, id, index, type);

    *type = NULL;

    if (!encoder->output_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    if (index >= encoder->input_type_count)
        return MF_E_NO_MORE_TYPES;

    return video_encoder_create_input_type(encoder, encoder->input_types[index], type);
}

static HRESULT WINAPI transform_GetOutputAvailableType(IMFTransform *iface, DWORD id,
        DWORD index, IMFMediaType **type)
{
    struct video_encoder *encoder = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, index %#lx, type %p.\n", iface, id, index, type);

    *type = NULL;
    if (index >= encoder->output_type_count)
        return MF_E_NO_MORE_TYPES;
    return MFCreateVideoMediaTypeFromSubtype(encoder->output_types[index], (IMFVideoMediaType **)type);
}

static HRESULT WINAPI transform_SetInputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct video_encoder *encoder = impl_from_IMFTransform(iface);
    IMFMediaType *good_input_type;
    GUID major, subtype;
    UINT64 ratio;
    BOOL result;
    HRESULT hr;
    ULONG i;

    TRACE("iface %p, id %#lx, type %p, flags %#lx.\n", iface, id, type, flags);

    if (!type)
    {
        if (encoder->input_type)
        {
            IMFMediaType_Release(encoder->input_type);
            encoder->input_type = NULL;
        }
        if (encoder->wg_transform)
        {
            wg_transform_destroy(encoder->wg_transform);
            encoder->wg_transform = 0;
        }
        return S_OK;
    }

    if (!encoder->output_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    if (FAILED(IMFMediaType_GetGUID(type, &MF_MT_MAJOR_TYPE, &major)) ||
            FAILED(IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &subtype)))
        return E_INVALIDARG;

    if (!IsEqualGUID(&major, &MFMediaType_Video))
        return MF_E_INVALIDMEDIATYPE;

    for (i = 0; i < encoder->input_type_count; ++i)
        if (IsEqualGUID(&subtype, encoder->input_types[i]))
            break;
    if (i == encoder->input_type_count)
        return MF_E_INVALIDMEDIATYPE;

    if (FAILED(IMFMediaType_GetUINT64(type, &MF_MT_FRAME_SIZE, &ratio))
            || FAILED(IMFMediaType_GetUINT64(type, &MF_MT_FRAME_RATE, &ratio)))
        return MF_E_INVALIDMEDIATYPE;

    if (FAILED(hr = video_encoder_create_input_type(encoder, &subtype, &good_input_type)))
        return hr;
    hr = IMFMediaType_Compare(good_input_type, (IMFAttributes *)type,
            MF_ATTRIBUTES_MATCH_INTERSECTION, &result);
    IMFMediaType_Release(good_input_type);
    if (FAILED(hr) || !result)
        return MF_E_INVALIDMEDIATYPE;

    if (flags & MFT_SET_TYPE_TEST_ONLY)
        return S_OK;

    if (encoder->input_type)
        IMFMediaType_Release(encoder->input_type);
    IMFMediaType_AddRef((encoder->input_type = type));

    if (FAILED(hr = video_encoder_try_create_wg_transform(encoder)))
    {
        IMFMediaType_Release(encoder->input_type);
        encoder->input_type = NULL;
    }

    return hr;
}

static HRESULT WINAPI transform_SetOutputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct video_encoder *encoder = impl_from_IMFTransform(iface);
    UINT32 uint32_value;
    UINT64 uint64_value;
    GUID major, subtype;
    ULONG i;

    TRACE("iface %p, id %#lx, type %p, flags %#lx.\n", iface, id, type, flags);

    if (!type)
    {
        if (encoder->input_type)
        {
            IMFMediaType_Release(encoder->input_type);
            encoder->input_type = NULL;
        }
        if (encoder->output_type)
        {
            IMFMediaType_Release(encoder->output_type);
            encoder->output_type = NULL;
            memset(&encoder->output_info, 0, sizeof(encoder->output_info));
        }
        if (encoder->wg_transform)
        {
            wg_transform_destroy(encoder->wg_transform);
            encoder->wg_transform = 0;
        }
        return S_OK;
    }

    if (FAILED(IMFMediaType_GetGUID(type, &MF_MT_MAJOR_TYPE, &major))
            || FAILED(IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &subtype)))
        return E_INVALIDARG;

    if (!IsEqualGUID(&major, &MFMediaType_Video))
        return MF_E_INVALIDMEDIATYPE;

    for (i = 0; i < encoder->output_type_count; ++i)
        if (IsEqualGUID(&subtype, encoder->output_types[i]))
            break;
    if (i == encoder->output_type_count)
        return MF_E_INVALIDMEDIATYPE;

    if (FAILED(IMFMediaType_GetUINT64(type, &MF_MT_FRAME_SIZE, &uint64_value)))
        return MF_E_INVALIDMEDIATYPE;

    if (flags & MFT_SET_TYPE_TEST_ONLY)
        return S_OK;

    if (FAILED(IMFMediaType_GetUINT64(type, &MF_MT_FRAME_RATE, &uint64_value))
            || FAILED(IMFMediaType_GetUINT32(type, &MF_MT_AVG_BITRATE, &uint32_value))
            || FAILED(IMFMediaType_GetUINT32(type, &MF_MT_INTERLACE_MODE, &uint32_value)))
        return MF_E_INVALIDMEDIATYPE;

    if (encoder->input_type)
    {
        IMFMediaType_Release(encoder->input_type);
        encoder->input_type = NULL;
    }

    if (encoder->output_type)
        IMFMediaType_Release(encoder->output_type);
    IMFMediaType_AddRef((encoder->output_type = type));

    /* FIXME: Add MF_MT_MPEG_SEQUENCE_HEADER attribute. */

    /* FIXME: Hardcode a size that native uses for 1920 * 1080.
     * And hope it's large enough to make things work for now.
     * The right way is to calculate it based on frame width and height. */
    encoder->output_info.cbSize = 0x3bc400;

    if (encoder->wg_transform)
    {
        wg_transform_destroy(encoder->wg_transform);
        encoder->wg_transform = 0;
    }

    return S_OK;
}

static HRESULT WINAPI transform_GetInputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct video_encoder *encoder = impl_from_IMFTransform(iface);
    HRESULT hr;

    TRACE("iface %p, id %#lx, type %p\n", iface, id, type);

    if (!encoder->input_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    if (FAILED(hr = MFCreateMediaType(type)))
        return hr;

    return IMFMediaType_CopyAllItems(encoder->input_type, (IMFAttributes *)*type);

}

static HRESULT WINAPI transform_GetOutputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct video_encoder *encoder = impl_from_IMFTransform(iface);
    HRESULT hr;

    TRACE("iface %p, id %#lx, type %p\n", iface, id, type);

    if (!encoder->output_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    if (FAILED(hr = MFCreateMediaType(type)))
        return hr;

    return IMFMediaType_CopyAllItems(encoder->output_type, (IMFAttributes *)*type);
}

static HRESULT WINAPI transform_GetInputStatus(IMFTransform *iface, DWORD id, DWORD *flags)
{
    FIXME("iface %p, id %#lx, flags %p.\n", iface, id, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetOutputStatus(IMFTransform *iface, DWORD *flags)
{
    FIXME("iface %p, flags %p stub!\n", iface, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_SetOutputBounds(IMFTransform *iface, LONGLONG lower, LONGLONG upper)
{
    FIXME("iface %p, lower %I64d, upper %I64d.\n", iface, lower, upper);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_ProcessEvent(IMFTransform *iface, DWORD id, IMFMediaEvent *event)
{
    FIXME("iface %p, id %#lx, event %p stub!\n", iface, id, event);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_ProcessMessage(IMFTransform *iface, MFT_MESSAGE_TYPE message, ULONG_PTR param)
{
    struct video_encoder *encoder = impl_from_IMFTransform(iface);

    TRACE("iface %p, message %#x, param %Ix.\n", iface, message, param);

    switch (message)
    {
        case MFT_MESSAGE_COMMAND_DRAIN:
            return wg_transform_drain(encoder->wg_transform);

        case MFT_MESSAGE_COMMAND_FLUSH:
            return wg_transform_flush(encoder->wg_transform);

        default:
            FIXME("Ignoring message %#x.\n", message);
            return S_OK;
    }
}

static HRESULT WINAPI transform_ProcessInput(IMFTransform *iface, DWORD id, IMFSample *sample, DWORD flags)
{
    struct video_encoder *encoder = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, sample %p, flags %#lx.\n", iface, id, sample, flags);

    if (!encoder->wg_transform)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    return wg_transform_push_mf(encoder->wg_transform, sample, encoder->wg_sample_queue);
}

static HRESULT WINAPI transform_ProcessOutput(IMFTransform *iface, DWORD flags, DWORD count,
        MFT_OUTPUT_DATA_BUFFER *samples, DWORD *status)
{
    struct video_encoder *encoder = impl_from_IMFTransform(iface);
    HRESULT hr;

    TRACE("iface %p, flags %#lx, count %lu, samples %p, status %p.\n", iface, flags, count, samples, status);

    if (count != 1)
        return E_INVALIDARG;
    if (!encoder->wg_transform)
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    if (!samples->pSample)
        return E_INVALIDARG;

    if (SUCCEEDED(hr = wg_transform_read_mf(encoder->wg_transform, samples->pSample, &samples->dwStatus, NULL)))
        wg_sample_queue_flush(encoder->wg_sample_queue, false);

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

static HRESULT WINAPI codec_api_QueryInterface(ICodecAPI *iface, REFIID riid, void **out)
{
    struct video_encoder *encoder = impl_from_ICodecAPI(iface);
    return IMFTransform_QueryInterface(&encoder->IMFTransform_iface, riid, out);
}

static ULONG WINAPI codec_api_AddRef(ICodecAPI *iface)
{
    struct video_encoder *encoder = impl_from_ICodecAPI(iface);
    return IMFTransform_AddRef(&encoder->IMFTransform_iface);
}

static ULONG WINAPI codec_api_Release(ICodecAPI *iface)
{
    struct video_encoder *encoder = impl_from_ICodecAPI(iface);
    return IMFTransform_Release(&encoder->IMFTransform_iface);
}

static HRESULT WINAPI codec_api_IsSupported(ICodecAPI *iface, const GUID *api)
{
    FIXME("iface %p, api %s.\n", iface, debugstr_guid(api));
    return E_NOTIMPL;
}

static HRESULT WINAPI codec_api_IsModifiable(ICodecAPI *iface, const GUID *api)
{
    FIXME("iface %p, api %s.\n", iface, debugstr_guid(api));
    return E_NOTIMPL;
}

static HRESULT WINAPI codec_api_GetParameterRange(ICodecAPI *iface,
        const GUID *api, VARIANT *min, VARIANT *max, VARIANT *delta)
{
    FIXME("iface %p, api %s, min %p, max %p, delta %p.\n",
            iface, debugstr_guid(api), min, max, delta);
    return E_NOTIMPL;
}

static HRESULT WINAPI codec_api_GetParameterValues(ICodecAPI *iface, const GUID *api, VARIANT **values, ULONG *count)
{
    FIXME("iface %p, api %s, values %p, count %p.\n", iface, debugstr_guid(api), values, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI codec_api_GetDefaultValue(ICodecAPI *iface, const GUID *api, VARIANT *value)
{
    FIXME("iface %p, api %s, value %p.\n", iface, debugstr_guid(api), value);
    return E_NOTIMPL;
}

static HRESULT WINAPI codec_api_GetValue(ICodecAPI *iface, const GUID *api, VARIANT *value)
{
    FIXME("iface %p, api %s, value %p.\n", iface, debugstr_guid(api), value);
    return E_NOTIMPL;
}

static HRESULT WINAPI codec_api_SetValue(ICodecAPI *iface, const GUID *api, VARIANT *value)
{
    FIXME("iface %p, api %s, value %p.\n", iface, debugstr_guid(api), value);
    return E_NOTIMPL;
}

static HRESULT WINAPI codec_api_RegisterForEvent(ICodecAPI *iface, const GUID *api, LONG_PTR userData)
{
    FIXME("iface %p, api %s, value %p.\n", iface, debugstr_guid(api), LongToPtr(userData));
    return E_NOTIMPL;
}

static HRESULT WINAPI codec_api_UnregisterForEvent(ICodecAPI *iface, const GUID *api)
{
    FIXME("iface %p, api %s.\n", iface, debugstr_guid(api));
    return E_NOTIMPL;
}

static HRESULT WINAPI codec_api_SetAllDefaults(ICodecAPI *iface)
{
    FIXME("iface %p.\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI codec_api_SetValueWithNotify(ICodecAPI *iface,
        const GUID *api, VARIANT *value, GUID **param, ULONG *count)
{
    FIXME("iface %p, api %s, param %p, count %p.\n", iface, debugstr_guid(api), param, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI codec_api_SetAllDefaultsWithNotify(ICodecAPI *iface, GUID **param, ULONG *count)
{
    FIXME("iface %p, param %p, count %p.\n", iface, param, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI codec_api_GetAllSettings(ICodecAPI *iface, IStream *stream)
{
    FIXME("iface %p, stream %p.\n", iface, stream);
    return E_NOTIMPL;
}

static HRESULT WINAPI codec_api_SetAllSettings(ICodecAPI *iface, IStream *stream)
{
    FIXME("iface %p, stream %p.\n", iface, stream);
    return E_NOTIMPL;
}

static HRESULT WINAPI codec_api_SetAllSettingsWithNotify(ICodecAPI *iface, IStream *stream, GUID **param, ULONG *count)
{
    FIXME("iface %p, stream %p, param %p, count %p.\n", iface, stream, param, count);
    return E_NOTIMPL;
}

static const ICodecAPIVtbl codec_api_vtbl =
{
    codec_api_QueryInterface,
    codec_api_AddRef,
    codec_api_Release,
    codec_api_IsSupported,
    codec_api_IsModifiable,
    codec_api_GetParameterRange,
    codec_api_GetParameterValues,
    codec_api_GetDefaultValue,
    codec_api_GetValue,
    codec_api_SetValue,
    codec_api_RegisterForEvent,
    codec_api_UnregisterForEvent,
    codec_api_SetAllDefaults,
    codec_api_SetValueWithNotify,
    codec_api_SetAllDefaultsWithNotify,
    codec_api_GetAllSettings,
    codec_api_SetAllSettings,
    codec_api_SetAllSettingsWithNotify,
};

static HRESULT video_encoder_create(const GUID *const *input_types, UINT input_type_count,
        const GUID *const *output_types, UINT output_type_count, struct video_encoder **out)
{
    struct video_encoder *encoder;
    HRESULT hr;

    if (!(encoder = calloc(1, sizeof(*encoder))))
        return E_OUTOFMEMORY;

    encoder->IMFTransform_iface.lpVtbl = &transform_vtbl;
    encoder->ICodecAPI_iface.lpVtbl = &codec_api_vtbl;
    encoder->refcount = 1;

    encoder->input_types = input_types;
    encoder->input_type_count = input_type_count;
    encoder->output_types = output_types;
    encoder->output_type_count = output_type_count;

    encoder->wg_transform_attrs.input_queue_length = 15;

    if (FAILED(hr = MFCreateAttributes(&encoder->attributes, 16)))
        goto failed;
    if (FAILED(hr = IMFAttributes_SetUINT32(encoder->attributes, &MFT_ENCODER_SUPPORTS_CONFIG_EVENT, TRUE)))
        goto failed;

    if (FAILED(hr = wg_sample_queue_create(&encoder->wg_sample_queue)))
        goto failed;

    *out = encoder;
    TRACE("Created video encoder %p\n", encoder);
    return S_OK;

failed:
    if (encoder->attributes)
        IMFAttributes_Release(encoder->attributes);
    free(encoder);
    return hr;
}

static const GUID *const h264_encoder_input_types[] =
{
    &MFVideoFormat_IYUV,
    &MFVideoFormat_YV12,
    &MFVideoFormat_NV12,
    &MFVideoFormat_YUY2,
};

static const GUID *const h264_encoder_output_types[] =
{
    &MFVideoFormat_H264,
};

HRESULT h264_encoder_create(REFIID riid, void **out)
{
    const MFVIDEOFORMAT input_format =
    {
        .dwSize = sizeof(MFVIDEOFORMAT),
        .videoInfo = {.dwWidth = 1920, .dwHeight = 1080},
        .guidFormat = MFVideoFormat_NV12,
    };
    const MFVIDEOFORMAT output_format =
    {
        .dwSize = sizeof(MFVIDEOFORMAT),
        .videoInfo = {.dwWidth = 1920, .dwHeight = 1080},
        .guidFormat = MFVideoFormat_H264,
    };
    struct video_encoder *encoder;
    HRESULT hr;

    TRACE("riid %s, out %p.\n", debugstr_guid(riid), out);

    if (FAILED(hr = check_video_transform_support(&input_format, &output_format)))
    {
        ERR_(winediag)("GStreamer doesn't support H.264 encoding, please install appropriate plugins\n");
        return hr;
    }

    if (FAILED(hr = video_encoder_create(h264_encoder_input_types, ARRAY_SIZE(h264_encoder_input_types),
            h264_encoder_output_types, ARRAY_SIZE(h264_encoder_output_types), &encoder)))
        return hr;

    TRACE("Created h264 encoder transform %p.\n", &encoder->IMFTransform_iface);

    hr = IMFTransform_QueryInterface(&encoder->IMFTransform_iface, riid, out);
    IMFTransform_Release(&encoder->IMFTransform_iface);
    return hr;
}
