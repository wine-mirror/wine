/* WMA Decoder Transform
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

DEFINE_MEDIATYPE_GUID(MFAudioFormat_XMAudio2, 0x0166);

static const GUID *wma_decoder_input_types[] =
{
    &MEDIASUBTYPE_MSAUDIO1,
    &MFAudioFormat_WMAudioV8,
    &MFAudioFormat_WMAudioV9,
    &MFAudioFormat_WMAudio_Lossless,
    &MFAudioFormat_XMAudio2,
};
static const GUID *wma_decoder_output_types[] =
{
    &MFAudioFormat_Float,
    &MFAudioFormat_PCM,
};

struct wma_decoder
{
    IUnknown IUnknown_inner;
    IMFTransform IMFTransform_iface;
    IMediaObject IMediaObject_iface;
    IUnknown *outer;
    LONG refcount;
    IMFMediaType *input_type;
    IMFMediaType *output_type;

    IMFSample *input_sample;
    struct wg_transform *wg_transform;
};

static inline struct wma_decoder *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct wma_decoder, IUnknown_inner);
}

static HRESULT WINAPI wma_unknown_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    struct wma_decoder *decoder = impl_from_IUnknown(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown))
        *out = &decoder->IUnknown_inner;
    else if (IsEqualGUID(iid, &IID_IMFTransform))
        *out = &decoder->IMFTransform_iface;
    else if (IsEqualGUID(iid, &IID_IMediaObject))
        *out = &decoder->IMediaObject_iface;
    else
    {
        *out = NULL;
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI wma_unknown_AddRef(IUnknown *iface)
{
    struct wma_decoder *decoder = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&decoder->refcount);

    TRACE("iface %p increasing refcount to %u.\n", decoder, refcount);

    return refcount;
}

static ULONG WINAPI wma_unknown_Release(IUnknown *iface)
{
    struct wma_decoder *decoder = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&decoder->refcount);

    TRACE("iface %p decreasing refcount to %u.\n", decoder, refcount);

    if (!refcount)
    {
        if (decoder->input_sample)
            IMFSample_Release(decoder->input_sample);
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

static const IUnknownVtbl wma_unknown_vtbl =
{
    wma_unknown_QueryInterface,
    wma_unknown_AddRef,
    wma_unknown_Release,
};

static struct wma_decoder *impl_from_IMFTransform(IMFTransform *iface)
{
    return CONTAINING_RECORD(iface, struct wma_decoder, IMFTransform_iface);
}

static HRESULT try_create_wg_transform(struct wma_decoder *decoder)
{
    struct wg_encoded_format input_format;
    struct wg_format output_format;

    if (decoder->wg_transform)
        wg_transform_destroy(decoder->wg_transform);
    decoder->wg_transform = NULL;

    mf_media_type_to_wg_encoded_format(decoder->input_type, &input_format);
    if (input_format.encoded_type == WG_ENCODED_TYPE_UNKNOWN)
        return MF_E_INVALIDMEDIATYPE;

    mf_media_type_to_wg_format(decoder->output_type, &output_format);
    if (output_format.major_type == WG_MAJOR_TYPE_UNKNOWN)
        return MF_E_INVALIDMEDIATYPE;

    decoder->wg_transform = wg_transform_create(&input_format, &output_format);
    if (decoder->wg_transform)
        return S_OK;

    WARN("Failed to create wg_transform.\n");
    return E_FAIL;
}

static HRESULT WINAPI wma_decoder_QueryInterface(IMFTransform *iface, REFIID iid, void **out)
{
    struct wma_decoder *decoder = impl_from_IMFTransform(iface);
    return IUnknown_QueryInterface(decoder->outer, iid, out);
}

static ULONG WINAPI wma_decoder_AddRef(IMFTransform *iface)
{
    struct wma_decoder *decoder = impl_from_IMFTransform(iface);
    return IUnknown_AddRef(decoder->outer);
}

static ULONG WINAPI wma_decoder_Release(IMFTransform *iface)
{
    struct wma_decoder *decoder = impl_from_IMFTransform(iface);
    return IUnknown_Release(decoder->outer);
}

static HRESULT WINAPI wma_decoder_GetStreamLimits(IMFTransform *iface, DWORD *input_minimum,
        DWORD *input_maximum, DWORD *output_minimum, DWORD *output_maximum)
{
    FIXME("iface %p, input_minimum %p, input_maximum %p, output_minimum %p, output_maximum %p stub!\n",
            iface, input_minimum, input_maximum, output_minimum, output_maximum);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_decoder_GetStreamCount(IMFTransform *iface, DWORD *inputs, DWORD *outputs)
{
    FIXME("iface %p, inputs %p, outputs %p stub!\n", iface, inputs, outputs);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_decoder_GetStreamIDs(IMFTransform *iface, DWORD input_size, DWORD *inputs,
        DWORD output_size, DWORD *outputs)
{
    FIXME("iface %p, input_size %u, inputs %p, output_size %u, outputs %p stub!\n", iface,
            input_size, inputs, output_size, outputs);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_decoder_GetInputStreamInfo(IMFTransform *iface, DWORD id, MFT_INPUT_STREAM_INFO *info)
{
    struct wma_decoder *decoder = impl_from_IMFTransform(iface);
    UINT32 block_alignment;
    HRESULT hr;

    TRACE("iface %p, id %u, info %p.\n", iface, id, info);

    if (!decoder->input_type || !decoder->output_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    if (FAILED(hr = IMFMediaType_GetUINT32(decoder->input_type, &MF_MT_AUDIO_BLOCK_ALIGNMENT, &block_alignment)))
        return hr;

    info->hnsMaxLatency = 0;
    info->dwFlags = 0;
    info->cbSize = block_alignment;
    info->cbMaxLookahead = 0;
    info->cbAlignment = 1;

    return S_OK;
}

static HRESULT WINAPI wma_decoder_GetOutputStreamInfo(IMFTransform *iface, DWORD id, MFT_OUTPUT_STREAM_INFO *info)
{
    struct wma_decoder *decoder = impl_from_IMFTransform(iface);
    UINT32 channel_count, block_alignment;
    HRESULT hr;

    TRACE("iface %p, id %u, info %p.\n", iface, id, info);

    if (!decoder->input_type || !decoder->output_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    if (FAILED(hr = IMFMediaType_GetUINT32(decoder->output_type, &MF_MT_AUDIO_NUM_CHANNELS, &channel_count)))
        return hr;
    if (FAILED(hr = IMFMediaType_GetUINT32(decoder->output_type, &MF_MT_AUDIO_BLOCK_ALIGNMENT, &block_alignment)))
        return hr;

    info->dwFlags = 0;
    info->cbSize = 1024 * block_alignment * channel_count;
    info->cbAlignment = 1;

    return S_OK;
}

static HRESULT WINAPI wma_decoder_GetAttributes(IMFTransform *iface, IMFAttributes **attributes)
{
    FIXME("iface %p, attributes %p stub!\n", iface, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_decoder_GetInputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    FIXME("iface %p, id %u, attributes %p stub!\n", iface, id, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_decoder_GetOutputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    FIXME("iface %p, id %u, attributes %p stub!\n", iface, id, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_decoder_DeleteInputStream(IMFTransform *iface, DWORD id)
{
    FIXME("iface %p, id %u stub!\n", iface, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_decoder_AddInputStreams(IMFTransform *iface, DWORD streams, DWORD *ids)
{
    FIXME("iface %p, streams %u, ids %p stub!\n", iface, streams, ids);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_decoder_GetInputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    FIXME("iface %p, id %u, index %u, type %p stub!\n", iface, id, index, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_decoder_GetOutputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    UINT32 channel_count, sample_size, sample_rate, block_alignment;
    struct wma_decoder *decoder = impl_from_IMFTransform(iface);
    IMFMediaType *media_type;
    const GUID *output_type;
    HRESULT hr;

    TRACE("iface %p, id %u, index %u, type %p.\n", iface, id, index, type);

    if (!decoder->input_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    *type = NULL;

    if (index >= ARRAY_SIZE(wma_decoder_output_types))
        return MF_E_NO_MORE_TYPES;
    output_type = wma_decoder_output_types[index];

    if (FAILED(hr = MFCreateMediaType(&media_type)))
        return hr;

    if (FAILED(hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio)))
        goto done;
    if (FAILED(hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, output_type)))
        goto done;

    if (IsEqualGUID(output_type, &MFAudioFormat_Float))
        sample_size = 32;
    else if (IsEqualGUID(output_type, &MFAudioFormat_PCM))
        sample_size = 16;
    else
    {
        FIXME("Subtype %s not implemented!\n", debugstr_guid(output_type));
        hr = E_NOTIMPL;
        goto done;
    }

    if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_BITS_PER_SAMPLE, sample_size)))
        goto done;

    if (FAILED(hr = IMFMediaType_GetUINT32(decoder->input_type, &MF_MT_AUDIO_NUM_CHANNELS, &channel_count)))
        goto done;
    if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_NUM_CHANNELS, channel_count)))
        goto done;

    if (FAILED(hr = IMFMediaType_GetUINT32(decoder->input_type, &MF_MT_AUDIO_SAMPLES_PER_SECOND, &sample_rate)))
        goto done;
    if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_SAMPLES_PER_SECOND, sample_rate)))
        goto done;

    block_alignment = sample_size * channel_count / 8;
    if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_BLOCK_ALIGNMENT, block_alignment)))
        goto done;
    if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_AVG_BYTES_PER_SECOND, sample_rate * block_alignment)))
        goto done;

    if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, 1)))
        goto done;
    if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_FIXED_SIZE_SAMPLES, 1)))
        goto done;
    if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_PREFER_WAVEFORMATEX, 1)))
        goto done;

done:
    if (SUCCEEDED(hr))
        IMFMediaType_AddRef((*type = media_type));

    IMFMediaType_Release(media_type);
    return hr;
}

static HRESULT WINAPI wma_decoder_SetInputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct wma_decoder *decoder = impl_from_IMFTransform(iface);
    MF_ATTRIBUTE_TYPE item_type;
    GUID major, subtype;
    HRESULT hr;
    ULONG i;

    TRACE("iface %p, id %u, type %p, flags %#x.\n", iface, id, type, flags);

    if (FAILED(hr = IMFMediaType_GetGUID(type, &MF_MT_MAJOR_TYPE, &major)) ||
        FAILED(hr = IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &subtype)))
        return hr;

    if (!IsEqualGUID(&major, &MFMediaType_Audio))
        return MF_E_INVALIDMEDIATYPE;

    for (i = 0; i < ARRAY_SIZE(wma_decoder_input_types); ++i)
        if (IsEqualGUID(&subtype, wma_decoder_input_types[i]))
            break;
    if (i == ARRAY_SIZE(wma_decoder_input_types))
        return MF_E_INVALIDMEDIATYPE;

    if (FAILED(IMFMediaType_GetItemType(type, &MF_MT_USER_DATA, &item_type)) ||
        item_type != MF_ATTRIBUTE_BLOB)
        return MF_E_INVALIDMEDIATYPE;
    if (FAILED(IMFMediaType_GetItemType(type, &MF_MT_AUDIO_BLOCK_ALIGNMENT, &item_type)) ||
        item_type != MF_ATTRIBUTE_UINT32)
        return MF_E_INVALIDMEDIATYPE;
    if (FAILED(IMFMediaType_GetItemType(type, &MF_MT_AUDIO_SAMPLES_PER_SECOND, &item_type)) ||
        item_type != MF_ATTRIBUTE_UINT32)
        return MF_E_INVALIDMEDIATYPE;
    if (FAILED(IMFMediaType_GetItemType(type, &MF_MT_AUDIO_NUM_CHANNELS, &item_type)) ||
        item_type != MF_ATTRIBUTE_UINT32)
        return MF_E_INVALIDMEDIATYPE;

    if (!decoder->input_type && FAILED(hr = MFCreateMediaType(&decoder->input_type)))
        return hr;

    if (decoder->output_type)
    {
        IMFMediaType_Release(decoder->output_type);
        decoder->output_type = NULL;
    }

    if (FAILED(hr = IMFMediaType_CopyAllItems(type, (IMFAttributes *)decoder->input_type)))
    {
        IMFMediaType_Release(decoder->input_type);
        decoder->input_type = NULL;
    }

    return hr;
}

static HRESULT WINAPI wma_decoder_SetOutputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct wma_decoder *decoder = impl_from_IMFTransform(iface);
    MF_ATTRIBUTE_TYPE item_type;
    ULONG i, sample_size;
    GUID major, subtype;
    HRESULT hr;

    TRACE("iface %p, id %u, type %p, flags %#x.\n", iface, id, type, flags);

    if (FAILED(hr = IMFMediaType_GetGUID(type, &MF_MT_MAJOR_TYPE, &major)) ||
        FAILED(hr = IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &subtype)))
        return hr;

    if (!IsEqualGUID(&major, &MFMediaType_Audio))
        return MF_E_INVALIDMEDIATYPE;

    for (i = 0; i < ARRAY_SIZE(wma_decoder_output_types); ++i)
        if (IsEqualGUID(&subtype, wma_decoder_output_types[i]))
            break;
    if (i == ARRAY_SIZE(wma_decoder_output_types))
        return MF_E_INVALIDMEDIATYPE;

    if (IsEqualGUID(&subtype, &MFAudioFormat_Float))
        sample_size = 32;
    else if (IsEqualGUID(&subtype, &MFAudioFormat_PCM))
        sample_size = 16;
    else
    {
        FIXME("Subtype %s not implemented!\n", debugstr_guid(&subtype));
        hr = E_NOTIMPL;
        return hr;
    }

    if (FAILED(IMFMediaType_SetUINT32(decoder->input_type, &MF_MT_AUDIO_BITS_PER_SAMPLE, sample_size)))
        return MF_E_INVALIDMEDIATYPE;

    if (FAILED(IMFMediaType_GetItemType(type, &MF_MT_AUDIO_AVG_BYTES_PER_SECOND, &item_type)) ||
        item_type != MF_ATTRIBUTE_UINT32)
        return MF_E_INVALIDMEDIATYPE;
    if (FAILED(IMFMediaType_GetItemType(type, &MF_MT_AUDIO_BITS_PER_SAMPLE, &item_type)) ||
        item_type != MF_ATTRIBUTE_UINT32)
        return MF_E_INVALIDMEDIATYPE;
    if (FAILED(IMFMediaType_GetItemType(type, &MF_MT_AUDIO_NUM_CHANNELS, &item_type)) ||
        item_type != MF_ATTRIBUTE_UINT32)
        return MF_E_INVALIDMEDIATYPE;
    if (FAILED(IMFMediaType_GetItemType(type, &MF_MT_AUDIO_SAMPLES_PER_SECOND, &item_type)) ||
        item_type != MF_ATTRIBUTE_UINT32)
        return MF_E_INVALIDMEDIATYPE;
    if (FAILED(IMFMediaType_GetItemType(type, &MF_MT_AUDIO_BLOCK_ALIGNMENT, &item_type)) ||
        item_type != MF_ATTRIBUTE_UINT32)
        return MF_E_INVALIDMEDIATYPE;

    if (!decoder->output_type && FAILED(hr = MFCreateMediaType(&decoder->output_type)))
        return hr;

    if (FAILED(hr = IMFMediaType_CopyAllItems(type, (IMFAttributes *)decoder->output_type)))
        goto failed;

    if (FAILED(hr = try_create_wg_transform(decoder)))
        goto failed;

    return S_OK;

failed:
    IMFMediaType_Release(decoder->output_type);
    decoder->output_type = NULL;
    return hr;
}

static HRESULT WINAPI wma_decoder_GetInputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    FIXME("iface %p, id %u, type %p stub!\n", iface, id, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_decoder_GetOutputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    FIXME("iface %p, id %u, type %p stub!\n", iface, id, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_decoder_GetInputStatus(IMFTransform *iface, DWORD id, DWORD *flags)
{
    FIXME("iface %p, id %u, flags %p stub!\n", iface, id, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_decoder_GetOutputStatus(IMFTransform *iface, DWORD *flags)
{
    FIXME("iface %p, flags %p stub!\n", iface, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_decoder_SetOutputBounds(IMFTransform *iface, LONGLONG lower, LONGLONG upper)
{
    FIXME("iface %p, lower %s, upper %s stub!\n", iface, wine_dbgstr_longlong(lower),
            wine_dbgstr_longlong(upper));
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_decoder_ProcessEvent(IMFTransform *iface, DWORD id, IMFMediaEvent *event)
{
    FIXME("iface %p, id %u, event %p stub!\n", iface, id, event);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_decoder_ProcessMessage(IMFTransform *iface, MFT_MESSAGE_TYPE message, ULONG_PTR param)
{
    FIXME("iface %p, message %#x, param %p stub!\n", iface, message, (void *)param);
    return S_OK;
}

static HRESULT WINAPI wma_decoder_ProcessInput(IMFTransform *iface, DWORD id, IMFSample *sample, DWORD flags)
{
    struct wma_decoder *decoder = impl_from_IMFTransform(iface);
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

    if (decoder->input_sample)
        return MF_E_NOTACCEPTING;

    if (FAILED(hr = IMFSample_ConvertToContiguousBuffer(sample, &media_buffer)))
        return hr;

    if (FAILED(hr = IMFMediaBuffer_GetCurrentLength(media_buffer, &buffer_size)))
        return hr;

    if (!(buffer_size = (buffer_size / info.cbSize) * info.cbSize))
        return S_OK;

    if (FAILED(hr = IMFMediaBuffer_Lock(media_buffer, &buffer, NULL, NULL)))
        goto done;

    if (SUCCEEDED(hr = wg_transform_push_data(decoder->wg_transform, buffer, buffer_size)))
        IMFSample_AddRef((decoder->input_sample = sample));

    IMFMediaBuffer_Unlock(media_buffer);

done:
    IMFMediaBuffer_Release(media_buffer);
    return hr;
}

static HRESULT WINAPI wma_decoder_ProcessOutput(IMFTransform *iface, DWORD flags, DWORD count,
        MFT_OUTPUT_DATA_BUFFER *samples, DWORD *status)
{
    struct wma_decoder *decoder = impl_from_IMFTransform(iface);
    struct wg_sample wg_sample = {0};
    IMFMediaBuffer *media_buffer;
    MFT_OUTPUT_STREAM_INFO info;
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

    if (wg_sample.size < info.cbSize)
        hr = MF_E_BUFFERTOOSMALL;
    else if (SUCCEEDED(hr = wg_transform_read_data(decoder->wg_transform, &wg_sample)))
    {
        if (wg_sample.flags & WG_SAMPLE_FLAG_INCOMPLETE)
            samples[0].dwStatus |= MFT_OUTPUT_DATA_BUFFER_INCOMPLETE;
    }
    else
    {
        if (decoder->input_sample)
            IMFSample_Release(decoder->input_sample);
        decoder->input_sample = NULL;
    }

    IMFMediaBuffer_Unlock(media_buffer);

done:
    IMFMediaBuffer_SetCurrentLength(media_buffer, wg_sample.size);
    IMFMediaBuffer_Release(media_buffer);
    return hr;
}

static const IMFTransformVtbl mf_transform_vtbl =
{
    wma_decoder_QueryInterface,
    wma_decoder_AddRef,
    wma_decoder_Release,
    wma_decoder_GetStreamLimits,
    wma_decoder_GetStreamCount,
    wma_decoder_GetStreamIDs,
    wma_decoder_GetInputStreamInfo,
    wma_decoder_GetOutputStreamInfo,
    wma_decoder_GetAttributes,
    wma_decoder_GetInputStreamAttributes,
    wma_decoder_GetOutputStreamAttributes,
    wma_decoder_DeleteInputStream,
    wma_decoder_AddInputStreams,
    wma_decoder_GetInputAvailableType,
    wma_decoder_GetOutputAvailableType,
    wma_decoder_SetInputType,
    wma_decoder_SetOutputType,
    wma_decoder_GetInputCurrentType,
    wma_decoder_GetOutputCurrentType,
    wma_decoder_GetInputStatus,
    wma_decoder_GetOutputStatus,
    wma_decoder_SetOutputBounds,
    wma_decoder_ProcessEvent,
    wma_decoder_ProcessMessage,
    wma_decoder_ProcessInput,
    wma_decoder_ProcessOutput,
};

static inline struct wma_decoder *impl_from_IMediaObject(IMediaObject *iface)
{
    return CONTAINING_RECORD(iface, struct wma_decoder, IMediaObject_iface);
}

static HRESULT WINAPI wma_media_object_QueryInterface(IMediaObject *iface, REFIID iid, void **obj)
{
    struct wma_decoder *decoder = impl_from_IMediaObject(iface);
    return IUnknown_QueryInterface(decoder->outer, iid, obj);
}

static ULONG WINAPI wma_media_object_AddRef(IMediaObject *iface)
{
    struct wma_decoder *decoder = impl_from_IMediaObject(iface);
    return IUnknown_AddRef(decoder->outer);
}

static ULONG WINAPI wma_media_object_Release(IMediaObject *iface)
{
    struct wma_decoder *decoder = impl_from_IMediaObject(iface);
    return IUnknown_Release(decoder->outer);
}

static HRESULT WINAPI wma_media_object_GetStreamCount(IMediaObject *iface, DWORD *input, DWORD *output)
{
    FIXME("iface %p, input %p, output %p stub!\n", iface, input, output);
    *input = *output = 1;
    return S_OK;
}

static HRESULT WINAPI wma_media_object_GetInputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    FIXME("iface %p, index %u, flags %p stub!\n", iface, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_media_object_GetOutputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    FIXME("iface %p, index %u, flags %p stub!\n", iface, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_media_object_GetInputType(IMediaObject *iface, DWORD index,
        DWORD type_index, DMO_MEDIA_TYPE *type)
{
    FIXME("iface %p, index %u, type_index %u, type %p stub!\n", iface, index, type_index, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_media_object_GetOutputType(IMediaObject *iface, DWORD index,
        DWORD type_index, DMO_MEDIA_TYPE *type)
{
    FIXME("iface %p, index %u, type_index %u, type %p stub!\n", iface, index, type_index, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_media_object_SetInputType(IMediaObject *iface, DWORD index,
        const DMO_MEDIA_TYPE *type, DWORD flags)
{
    FIXME("iface %p, index %u, type %p, flags %#x stub!\n", iface, index, type, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_media_object_SetOutputType(IMediaObject *iface, DWORD index,
        const DMO_MEDIA_TYPE *type, DWORD flags)
{
    FIXME("iface %p, index %u, type %p, flags %#x stub!\n", iface, index, type, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_media_object_GetInputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *type)
{
    FIXME("iface %p, index %u, type %p stub!\n", iface, index, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_media_object_GetOutputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *type)
{
    FIXME("iface %p, index %u, type %p stub!\n", iface, index, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_media_object_GetInputSizeInfo(IMediaObject *iface, DWORD index,
        DWORD *size, DWORD *lookahead, DWORD *alignment)
{
    FIXME("iface %p, index %u, size %p, lookahead %p, alignment %p stub!\n", iface, index, size,
            lookahead, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_media_object_GetOutputSizeInfo(IMediaObject *iface, DWORD index,
        DWORD *size, DWORD *alignment)
{
    FIXME("iface %p, index %u, size %p, alignment %p stub!\n", iface, index, size, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_media_object_GetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME *latency)
{
    FIXME("iface %p, index %u, latency %p stub!\n", iface, index, latency);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_media_object_SetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME latency)
{
    FIXME("iface %p, index %u, latency %s stub!\n", iface, index, wine_dbgstr_longlong(latency));
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_media_object_Flush(IMediaObject *iface)
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_media_object_Discontinuity(IMediaObject *iface, DWORD index)
{
    FIXME("iface %p, index %u stub!\n", iface, index);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_media_object_AllocateStreamingResources(IMediaObject *iface)
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_media_object_FreeStreamingResources(IMediaObject *iface)
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_media_object_GetInputStatus(IMediaObject *iface, DWORD index, DWORD *flags)
{
    FIXME("iface %p, index %u, flags %p stub!\n", iface, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_media_object_ProcessInput(IMediaObject *iface, DWORD index,
        IMediaBuffer *buffer, DWORD flags, REFERENCE_TIME timestamp, REFERENCE_TIME timelength)
{
    FIXME("iface %p, index %u, buffer %p, flags %#x, timestamp %s, timelength %s stub!\n", iface,
            index, buffer, flags, wine_dbgstr_longlong(timestamp), wine_dbgstr_longlong(timelength));
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_media_object_ProcessOutput(IMediaObject *iface, DWORD flags, DWORD count,
        DMO_OUTPUT_DATA_BUFFER *buffers, DWORD *status)
{
    FIXME("iface %p, flags %#x, count %u, buffers %p, status %p stub!\n", iface, flags, count, buffers, status);
    return E_NOTIMPL;
}

static HRESULT WINAPI wma_media_object_Lock(IMediaObject *iface, LONG lock)
{
    FIXME("iface %p, lock %d stub!\n", iface, lock);
    return E_NOTIMPL;
}

static const IMediaObjectVtbl wma_media_object_vtbl =
{
    wma_media_object_QueryInterface,
    wma_media_object_AddRef,
    wma_media_object_Release,
    wma_media_object_GetStreamCount,
    wma_media_object_GetInputStreamInfo,
    wma_media_object_GetOutputStreamInfo,
    wma_media_object_GetInputType,
    wma_media_object_GetOutputType,
    wma_media_object_SetInputType,
    wma_media_object_SetOutputType,
    wma_media_object_GetInputCurrentType,
    wma_media_object_GetOutputCurrentType,
    wma_media_object_GetInputSizeInfo,
    wma_media_object_GetOutputSizeInfo,
    wma_media_object_GetInputMaxLatency,
    wma_media_object_SetInputMaxLatency,
    wma_media_object_Flush,
    wma_media_object_Discontinuity,
    wma_media_object_AllocateStreamingResources,
    wma_media_object_FreeStreamingResources,
    wma_media_object_GetInputStatus,
    wma_media_object_ProcessInput,
    wma_media_object_ProcessOutput,
    wma_media_object_Lock,
};

HRESULT wma_decoder_create(IUnknown *outer, IUnknown **out)
{
    struct wma_decoder *decoder;

    TRACE("outer %p, out %p.\n", outer, out);

    if (!(decoder = calloc(1, sizeof(*decoder))))
        return E_OUTOFMEMORY;

    decoder->IUnknown_inner.lpVtbl = &wma_unknown_vtbl;
    decoder->IMFTransform_iface.lpVtbl = &mf_transform_vtbl;
    decoder->IMediaObject_iface.lpVtbl = &wma_media_object_vtbl;
    decoder->refcount = 1;
    decoder->outer = outer ? outer : &decoder->IUnknown_inner;

    *out = &decoder->IUnknown_inner;
    TRACE("Created decoder %p\n", *out);
    return S_OK;
}
