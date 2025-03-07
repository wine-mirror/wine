/* WMA Decoder DMO / MF Transform
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
#include "mediaerr.h"
#include "dmort.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wmadec);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

static const GUID *const wma_decoder_input_types[] =
{
    &MEDIASUBTYPE_MSAUDIO1,
    &MFAudioFormat_WMAudioV8,
    &MFAudioFormat_WMAudioV9,
    &MFAudioFormat_WMAudio_Lossless,
};
static const GUID *const wma_decoder_output_types[] =
{
    &MFAudioFormat_Float,
    &MFAudioFormat_PCM,
};

struct wma_decoder
{
    IUnknown IUnknown_inner;
    IMFTransform IMFTransform_iface;
    IMediaObject IMediaObject_iface;
    IPropertyBag IPropertyBag_iface;
    IUnknown *outer;
    LONG refcount;

    DMO_MEDIA_TYPE input_type;
    DMO_MEDIA_TYPE output_type;

    DWORD input_buf_size;
    DWORD output_buf_size;

    wg_transform_t wg_transform;
    struct wg_sample_queue *wg_sample_queue;
};

static inline struct wma_decoder *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct wma_decoder, IUnknown_inner);
}

static HRESULT try_create_wg_transform(struct wma_decoder *decoder)
{
    struct wg_transform_attrs attrs = {0};

    if (decoder->wg_transform)
    {
        wg_transform_destroy(decoder->wg_transform);
        decoder->wg_transform = 0;
    }

    return wg_transform_create_quartz(&decoder->input_type, &decoder->output_type,
            &attrs, &decoder->wg_transform);
}

static HRESULT WINAPI unknown_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    struct wma_decoder *decoder = impl_from_IUnknown(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown))
        *out = &decoder->IUnknown_inner;
    else if (IsEqualGUID(iid, &IID_IMFTransform))
        *out = &decoder->IMFTransform_iface;
    else if (IsEqualGUID(iid, &IID_IMediaObject))
        *out = &decoder->IMediaObject_iface;
    else if (IsEqualIID(iid, &IID_IPropertyBag))
        *out = &decoder->IPropertyBag_iface;
    else
    {
        *out = NULL;
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI unknown_AddRef(IUnknown *iface)
{
    struct wma_decoder *decoder = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&decoder->refcount);

    TRACE("iface %p increasing refcount to %lu.\n", decoder, refcount);

    return refcount;
}

static ULONG WINAPI unknown_Release(IUnknown *iface)
{
    struct wma_decoder *decoder = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&decoder->refcount);

    TRACE("iface %p decreasing refcount to %lu.\n", decoder, refcount);

    if (!refcount)
    {
        if (decoder->wg_transform)
            wg_transform_destroy(decoder->wg_transform);

        wg_sample_queue_destroy(decoder->wg_sample_queue);

        MoFreeMediaType(&decoder->input_type);
        MoFreeMediaType(&decoder->output_type);
        free(decoder);
    }

    return refcount;
}

static const IUnknownVtbl unknown_vtbl =
{
    unknown_QueryInterface,
    unknown_AddRef,
    unknown_Release,
};

static struct wma_decoder *impl_from_IMFTransform(IMFTransform *iface)
{
    return CONTAINING_RECORD(iface, struct wma_decoder, IMFTransform_iface);
}

static HRESULT WINAPI transform_QueryInterface(IMFTransform *iface, REFIID iid, void **out)
{
    struct wma_decoder *decoder = impl_from_IMFTransform(iface);
    return IUnknown_QueryInterface(decoder->outer, iid, out);
}

static ULONG WINAPI transform_AddRef(IMFTransform *iface)
{
    struct wma_decoder *decoder = impl_from_IMFTransform(iface);
    return IUnknown_AddRef(decoder->outer);
}

static ULONG WINAPI transform_Release(IMFTransform *iface)
{
    struct wma_decoder *decoder = impl_from_IMFTransform(iface);
    return IUnknown_Release(decoder->outer);
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
    struct wma_decoder *decoder = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %lu, info %p.\n", iface, id, info);

    if (IsEqualGUID(&decoder->input_type.majortype, &GUID_NULL)
            || IsEqualGUID(&decoder->output_type.majortype, &GUID_NULL))
    {
        memset(info, 0, sizeof(*info));
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    }

    info->hnsMaxLatency = 0;
    info->dwFlags = 0;
    info->cbSize = decoder->input_buf_size;
    info->cbMaxLookahead = 0;
    info->cbAlignment = 1;
    return S_OK;
}

static HRESULT WINAPI transform_GetOutputStreamInfo(IMFTransform *iface, DWORD id, MFT_OUTPUT_STREAM_INFO *info)
{
    struct wma_decoder *decoder = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %lu, info %p.\n", iface, id, info);

    if (IsEqualGUID(&decoder->input_type.majortype, &GUID_NULL)
            || IsEqualGUID(&decoder->output_type.majortype, &GUID_NULL))
    {
        memset(info, 0, sizeof(*info));
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    }

    info->dwFlags = 0;
    info->cbSize = decoder->output_buf_size;
    info->cbAlignment = 1;
    return S_OK;
}

static HRESULT WINAPI transform_GetAttributes(IMFTransform *iface, IMFAttributes **attributes)
{
    TRACE("iface %p, attributes %p.\n", iface, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetInputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    TRACE("iface %p, id %#lx, attributes %p.\n", iface, id, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetOutputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    TRACE("iface %p, id %#lx, attributes %p.\n", iface, id, attributes);
    return E_NOTIMPL;
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
    FIXME("iface %p, id %lu, index %lu, type %p stub!\n", iface, id, index, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetOutputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    UINT32 sample_size, block_alignment;
    struct wma_decoder *decoder = impl_from_IMFTransform(iface);
    IMFMediaType *media_type;
    const GUID *output_type;
    WAVEFORMATEX *wfx;
    HRESULT hr;

    TRACE("iface %p, id %lu, index %lu, type %p.\n", iface, id, index, type);

    if (IsEqualGUID(&decoder->input_type.majortype, &GUID_NULL))
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

    if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_BITS_PER_SAMPLE,
            sample_size)))
        goto done;

    wfx = (WAVEFORMATEX *)decoder->input_type.pbFormat;
    if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_NUM_CHANNELS, wfx->nChannels)))
        goto done;
    if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_SAMPLES_PER_SECOND, wfx->nSamplesPerSec)))
        goto done;

    block_alignment = sample_size * wfx->nChannels / 8;
    if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_BLOCK_ALIGNMENT, block_alignment)))
        goto done;
    if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_AVG_BYTES_PER_SECOND, wfx->nSamplesPerSec * block_alignment)))
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

static HRESULT WINAPI transform_SetInputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct wma_decoder *decoder = impl_from_IMFTransform(iface);
    MF_ATTRIBUTE_TYPE item_type;
    UINT32 block_alignment;
    GUID major, subtype;
    HRESULT hr;
    ULONG i;

    TRACE("iface %p, id %lu, type %p, flags %#lx.\n", iface, id, type, flags);

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
    if (FAILED(IMFMediaType_GetUINT32(type, &MF_MT_AUDIO_BLOCK_ALIGNMENT, &block_alignment)))
        return MF_E_INVALIDMEDIATYPE;
    if (FAILED(IMFMediaType_GetItemType(type, &MF_MT_AUDIO_SAMPLES_PER_SECOND, &item_type)) ||
        item_type != MF_ATTRIBUTE_UINT32)
        return MF_E_INVALIDMEDIATYPE;
    if (FAILED(IMFMediaType_GetItemType(type, &MF_MT_AUDIO_NUM_CHANNELS, &item_type)) ||
        item_type != MF_ATTRIBUTE_UINT32)
        return MF_E_INVALIDMEDIATYPE;
    if (flags & MFT_SET_TYPE_TEST_ONLY)
        return S_OK;

    MoFreeMediaType(&decoder->output_type);
    memset(&decoder->output_type, 0, sizeof(decoder->output_type));
    MoFreeMediaType(&decoder->input_type);
    memset(&decoder->input_type, 0, sizeof(decoder->input_type));

    if (SUCCEEDED(hr = MFInitAMMediaTypeFromMFMediaType(type, GUID_NULL, &decoder->input_type)))
        decoder->input_buf_size = block_alignment;

    return hr;
}

static HRESULT WINAPI transform_SetOutputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct wma_decoder *decoder = impl_from_IMFTransform(iface);
    UINT32 channel_count, block_alignment;
    MF_ATTRIBUTE_TYPE item_type;
    ULONG i, sample_size;
    GUID major, subtype;
    HRESULT hr;

    TRACE("iface %p, id %lu, type %p, flags %#lx.\n", iface, id, type, flags);

    if (IsEqualGUID(&decoder->input_type.majortype, &GUID_NULL))
        return MF_E_TRANSFORM_TYPE_NOT_SET;

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

    if (FAILED(IMFMediaType_GetItemType(type, &MF_MT_AUDIO_AVG_BYTES_PER_SECOND, &item_type)) ||
        item_type != MF_ATTRIBUTE_UINT32)
        return MF_E_INVALIDMEDIATYPE;
    if (FAILED(IMFMediaType_GetItemType(type, &MF_MT_AUDIO_BITS_PER_SAMPLE, &item_type)) ||
        item_type != MF_ATTRIBUTE_UINT32)
        return MF_E_INVALIDMEDIATYPE;
    if (FAILED(IMFMediaType_GetUINT32(type, &MF_MT_AUDIO_NUM_CHANNELS, &channel_count)))
        return MF_E_INVALIDMEDIATYPE;
    if (FAILED(IMFMediaType_GetItemType(type, &MF_MT_AUDIO_SAMPLES_PER_SECOND, &item_type)) ||
        item_type != MF_ATTRIBUTE_UINT32)
        return MF_E_INVALIDMEDIATYPE;
    if (FAILED(IMFMediaType_GetUINT32(type, &MF_MT_AUDIO_BLOCK_ALIGNMENT, &block_alignment)))
        return MF_E_INVALIDMEDIATYPE;
    if (flags & MFT_SET_TYPE_TEST_ONLY)
        return S_OK;

    MoFreeMediaType(&decoder->output_type);
    memset(&decoder->output_type, 0, sizeof(decoder->output_type));

    if (SUCCEEDED(hr = MFInitAMMediaTypeFromMFMediaType(type, GUID_NULL, &decoder->output_type)))
    {
        WAVEFORMATEX *wfx = (WAVEFORMATEX *)decoder->input_type.pbFormat;
        wfx->wBitsPerSample = sample_size;
        decoder->output_buf_size = 1024 * block_alignment * channel_count;
    }

    if (FAILED(hr = try_create_wg_transform(decoder)))
        goto failed;

    return S_OK;

failed:
    MoFreeMediaType(&decoder->output_type);
    memset(&decoder->output_type, 0, sizeof(decoder->output_type));
    return hr;
}

static HRESULT WINAPI transform_GetInputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    FIXME("iface %p, id %lu, type %p stub!\n", iface, id, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetOutputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    FIXME("iface %p, id %lu, type %p stub!\n", iface, id, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetInputStatus(IMFTransform *iface, DWORD id, DWORD *flags)
{
    FIXME("iface %p, id %lu, flags %p stub!\n", iface, id, flags);
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
    FIXME("iface %p, id %lu, event %p stub!\n", iface, id, event);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_ProcessMessage(IMFTransform *iface, MFT_MESSAGE_TYPE message, ULONG_PTR param)
{
    FIXME("iface %p, message %#x, param %p stub!\n", iface, message, (void *)param);
    return S_OK;
}

static HRESULT WINAPI transform_ProcessInput(IMFTransform *iface, DWORD id, IMFSample *sample, DWORD flags)
{
    struct wma_decoder *decoder = impl_from_IMFTransform(iface);
    MFT_INPUT_STREAM_INFO info;
    DWORD total_length;
    HRESULT hr;

    TRACE("iface %p, id %lu, sample %p, flags %#lx.\n", iface, id, sample, flags);

    if (!decoder->wg_transform)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    if (FAILED(hr = IMFTransform_GetInputStreamInfo(iface, 0, &info))
            || FAILED(hr = IMFSample_GetTotalLength(sample, &total_length)))
        return hr;

    /* WMA transform uses fixed size input samples and ignores samples with invalid sizes */
    if (total_length % info.cbSize)
        return S_OK;

    return wg_transform_push_mf(decoder->wg_transform, sample, decoder->wg_sample_queue);
}

static HRESULT WINAPI transform_ProcessOutput(IMFTransform *iface, DWORD flags, DWORD count,
        MFT_OUTPUT_DATA_BUFFER *samples, DWORD *status)
{
    struct wma_decoder *decoder = impl_from_IMFTransform(iface);
    MFT_OUTPUT_STREAM_INFO info;
    HRESULT hr;

    TRACE("iface %p, flags %#lx, count %lu, samples %p, status %p.\n", iface, flags, count, samples, status);

    if (count != 1)
        return E_INVALIDARG;

    if (!decoder->wg_transform)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    *status = samples->dwStatus = 0;
    if (!samples->pSample)
    {
        samples[0].dwStatus = MFT_OUTPUT_DATA_BUFFER_NO_SAMPLE;
        return MF_E_TRANSFORM_NEED_MORE_INPUT;
    }

    if (FAILED(hr = IMFTransform_GetOutputStreamInfo(iface, 0, &info)))
        return hr;

    if (SUCCEEDED(hr = wg_transform_read_mf(decoder->wg_transform, samples->pSample,
            info.cbSize, &samples->dwStatus, NULL)))
        wg_sample_queue_flush(decoder->wg_sample_queue, false);

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

static inline struct wma_decoder *impl_from_IMediaObject(IMediaObject *iface)
{
    return CONTAINING_RECORD(iface, struct wma_decoder, IMediaObject_iface);
}

static HRESULT WINAPI media_object_QueryInterface(IMediaObject *iface, REFIID iid, void **obj)
{
    struct wma_decoder *decoder = impl_from_IMediaObject(iface);
    return IUnknown_QueryInterface(decoder->outer, iid, obj);
}

static ULONG WINAPI media_object_AddRef(IMediaObject *iface)
{
    struct wma_decoder *decoder = impl_from_IMediaObject(iface);
    return IUnknown_AddRef(decoder->outer);
}

static ULONG WINAPI media_object_Release(IMediaObject *iface)
{
    struct wma_decoder *decoder = impl_from_IMediaObject(iface);
    return IUnknown_Release(decoder->outer);
}

static HRESULT WINAPI media_object_GetStreamCount(IMediaObject *iface, DWORD *input, DWORD *output)
{
    FIXME("iface %p, input %p, output %p semi-stub!\n", iface, input, output);
    *input = *output = 1;
    return S_OK;
}

static HRESULT WINAPI media_object_GetInputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    FIXME("iface %p, index %lu, flags %p stub!\n", iface, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_GetOutputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    FIXME("iface %p, index %lu, flags %p stub!\n", iface, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_GetInputType(IMediaObject *iface, DWORD index, DWORD type_index,
        DMO_MEDIA_TYPE *type)
{
    TRACE("iface %p, index %lu, type_index %lu, type %p.\n", iface, index, type_index, type);

    if (index > 0)
        return DMO_E_INVALIDSTREAMINDEX;
    if (type_index >= ARRAY_SIZE(wma_decoder_input_types))
        return DMO_E_NO_MORE_ITEMS;
    if (!type)
        return S_OK;

    memset(type, 0, sizeof(*type));
    type->majortype = MFMediaType_Audio;
    type->subtype = *wma_decoder_input_types[type_index];
    type->bFixedSizeSamples = FALSE;
    type->bTemporalCompression = TRUE;
    type->lSampleSize = 0;

    return S_OK;
}

static HRESULT WINAPI media_object_GetOutputType(IMediaObject *iface, DWORD index, DWORD type_index,
        DMO_MEDIA_TYPE *type)
{
    struct wma_decoder *decoder = impl_from_IMediaObject(iface);
    UINT32 depth, channels, rate;
    IMFMediaType *media_type;
    HRESULT hr;

    TRACE("iface %p, index %lu, type_index %lu, type %p\n", iface, index, type_index, type);

    if (index > 0)
        return DMO_E_INVALIDSTREAMINDEX;
    if (type_index >= 1)
        return DMO_E_NO_MORE_ITEMS;
    if (IsEqualGUID(&decoder->input_type.majortype, &GUID_NULL))
        return DMO_E_TYPE_NOT_SET;
    if (!type)
        return S_OK;

    if (FAILED(hr = MFCreateMediaTypeFromRepresentation(AM_MEDIA_TYPE_REPRESENTATION,
            &decoder->input_type, &media_type)))
        return hr;

    if (SUCCEEDED(IMFMediaType_GetUINT32(media_type, &MF_MT_AUDIO_BITS_PER_SAMPLE, &depth))
            && depth == 32)
        hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_Float);
    else
        hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);

    if (SUCCEEDED(hr))
        hr = IMFMediaType_GetUINT32(media_type, &MF_MT_AUDIO_NUM_CHANNELS, &channels);
    if (SUCCEEDED(hr))
        hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_BLOCK_ALIGNMENT, depth * channels / 8);

    if (SUCCEEDED(hr))
        hr = IMFMediaType_GetUINT32(media_type, &MF_MT_AUDIO_SAMPLES_PER_SECOND, &rate);
    if (SUCCEEDED(hr))
        hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_AVG_BYTES_PER_SECOND, depth * channels / 8 * rate);

    if (SUCCEEDED(hr))
        hr = IMFMediaType_DeleteItem(media_type, &MF_MT_USER_DATA);
    if (SUCCEEDED(hr))
        hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, type);

    IMFMediaType_Release(media_type);
    return hr;
}

static HRESULT WINAPI media_object_SetInputType(IMediaObject *iface, DWORD index,
        const DMO_MEDIA_TYPE *type, DWORD flags)
{
    struct wma_decoder *decoder = impl_from_IMediaObject(iface);
    unsigned int i;

    TRACE("iface %p, index %lu, type %p, flags %#lx.\n", iface, index, type, flags);

    if (index > 0)
        return DMO_E_INVALIDSTREAMINDEX;

    if (flags & DMO_SET_TYPEF_CLEAR)
    {
        if (flags != DMO_SET_TYPEF_CLEAR)
            return E_INVALIDARG;
        MoFreeMediaType(&decoder->input_type);
        memset(&decoder->input_type, 0, sizeof(decoder->input_type));
        if (decoder->wg_transform)
        {
            wg_transform_destroy(decoder->wg_transform);
            decoder->wg_transform = 0;
        }
        return S_OK;
    }
    if (!type)
        return E_POINTER;
    if (flags & ~DMO_SET_TYPEF_TEST_ONLY)
        return E_INVALIDARG;

    if (!IsEqualGUID(&type->majortype, &MEDIATYPE_Audio))
        return DMO_E_TYPE_NOT_ACCEPTED;

    for (i = 0; i < ARRAY_SIZE(wma_decoder_input_types); ++i)
        if (IsEqualGUID(&type->subtype, wma_decoder_input_types[i]))
            break;
    if (i == ARRAY_SIZE(wma_decoder_input_types))
        return DMO_E_TYPE_NOT_ACCEPTED;

    if (flags & DMO_SET_TYPEF_TEST_ONLY)
        return S_OK;

    MoFreeMediaType(&decoder->input_type);
    memset(&decoder->input_type, 0, sizeof(decoder->input_type));
    MoCopyMediaType(&decoder->input_type, type);

    if (decoder->wg_transform)
    {
        wg_transform_destroy(decoder->wg_transform);
        decoder->wg_transform = 0;
    }

    return S_OK;
}

static HRESULT WINAPI media_object_SetOutputType(IMediaObject *iface, DWORD index,
        const DMO_MEDIA_TYPE *type, DWORD flags)
{
    struct wma_decoder *decoder = impl_from_IMediaObject(iface);
    struct wg_transform_attrs attrs = {0};
    unsigned int i;
    HRESULT hr;

    TRACE("iface %p, index %lu, type %p, flags %#lx,\n", iface, index, type, flags);

    if (index > 0)
        return DMO_E_INVALIDSTREAMINDEX;

    if (flags & DMO_SET_TYPEF_CLEAR)
    {
        if (flags != DMO_SET_TYPEF_CLEAR)
            return E_INVALIDARG;
        MoFreeMediaType(&decoder->output_type);
        memset(&decoder->output_type, 0, sizeof(decoder->output_type));
        if (decoder->wg_transform)
        {
            wg_transform_destroy(decoder->wg_transform);
            decoder->wg_transform = 0;
        }
        return S_OK;
    }
    if (!type)
        return E_POINTER;
    if (flags & ~DMO_SET_TYPEF_TEST_ONLY)
        return E_INVALIDARG;

    if (!IsEqualGUID(&type->majortype, &MEDIATYPE_Audio))
        return DMO_E_TYPE_NOT_ACCEPTED;

    for (i = 0; i < ARRAY_SIZE(wma_decoder_output_types); ++i)
        if (IsEqualGUID(&type->subtype, wma_decoder_output_types[i]))
            break;
    if (i == ARRAY_SIZE(wma_decoder_output_types))
        return DMO_E_TYPE_NOT_ACCEPTED;

    if (IsEqualGUID(&decoder->input_type.majortype, &GUID_NULL))
        return DMO_E_TYPE_NOT_SET;
    if (flags & DMO_SET_TYPEF_TEST_ONLY)
        return S_OK;

    MoFreeMediaType(&decoder->output_type);
    memset(&decoder->output_type, 0, sizeof(decoder->output_type));
    MoCopyMediaType(&decoder->output_type, type);

    /* Set up wg_transform. */
    if (decoder->wg_transform)
    {
        wg_transform_destroy(decoder->wg_transform);
        decoder->wg_transform = 0;
    }
    if (FAILED(hr = wg_transform_create_quartz(&decoder->input_type, &decoder->output_type,
            &attrs, &decoder->wg_transform)))
        return hr;

    return S_OK;
}

static HRESULT WINAPI media_object_GetInputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *type)
{
    struct wma_decoder *decoder = impl_from_IMediaObject(iface);

    TRACE("iface %p, index %lu, type %p\n", iface, index, type);

    if (index)
        return DMO_E_INVALIDSTREAMINDEX;
    if (IsEqualGUID(&decoder->input_type.majortype, &GUID_NULL))
        return DMO_E_TYPE_NOT_SET;
    if (!type)
        return E_POINTER;
    return MoCopyMediaType(type, &decoder->input_type);
}

static HRESULT WINAPI media_object_GetOutputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *type)
{
    struct wma_decoder *decoder = impl_from_IMediaObject(iface);

    TRACE("iface %p, index %lu, type %p\n", iface, index, type);

    if (index)
        return DMO_E_INVALIDSTREAMINDEX;
    if (IsEqualGUID(&decoder->output_type.majortype, &GUID_NULL))
        return DMO_E_TYPE_NOT_SET;
    if (!type)
        return E_POINTER;
    return MoCopyMediaType(type, &decoder->output_type);
}

static HRESULT WINAPI media_object_GetInputSizeInfo(IMediaObject *iface, DWORD index, DWORD *size,
        DWORD *lookahead, DWORD *alignment)
{
    FIXME("iface %p, index %lu, size %p, lookahead %p, alignment %p stub!\n", iface, index, size,
            lookahead, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_GetOutputSizeInfo(IMediaObject *iface, DWORD index, DWORD *size, DWORD *alignment)
{
    struct wma_decoder *decoder = impl_from_IMediaObject(iface);

    TRACE("iface %p, index %lu, size %p, alignment %p.\n", iface, index, size, alignment);

    if (!size || !alignment)
        return E_POINTER;
    if (index > 0)
        return DMO_E_INVALIDSTREAMINDEX;
    if (IsEqualGUID(&decoder->output_type.majortype, &GUID_NULL))
        return DMO_E_TYPE_NOT_SET;

    *size = 8192;
    *alignment = 1;

    return S_OK;
}

static HRESULT WINAPI media_object_GetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME *latency)
{
    FIXME("iface %p, index %lu, latency %p stub!\n", iface, index, latency);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_SetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME latency)
{
    FIXME("iface %p, index %lu, latency %s stub!\n", iface, index, wine_dbgstr_longlong(latency));
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_Flush(IMediaObject *iface)
{
    struct wma_decoder *decoder = impl_from_IMediaObject(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    if (!decoder->wg_transform)
        return DMO_E_TYPE_NOT_SET;

    if (FAILED(hr = wg_transform_flush(decoder->wg_transform)))
        return hr;

    wg_sample_queue_flush(decoder->wg_sample_queue, TRUE);

    return S_OK;
}

static HRESULT WINAPI media_object_Discontinuity(IMediaObject *iface, DWORD index)
{
    TRACE("iface %p, index %lu.\n", iface, index);

    if (index > 0)
        return DMO_E_INVALIDSTREAMINDEX;

    return S_OK;
}

static HRESULT WINAPI media_object_AllocateStreamingResources(IMediaObject *iface)
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_FreeStreamingResources(IMediaObject *iface)
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_GetInputStatus(IMediaObject *iface, DWORD index, DWORD *flags)
{
    FIXME("iface %p, index %lu, flags %p stub!\n", iface, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_ProcessInput(IMediaObject *iface, DWORD index,
        IMediaBuffer *buffer, DWORD flags, REFERENCE_TIME timestamp, REFERENCE_TIME timelength)
{
    struct wma_decoder *decoder = impl_from_IMediaObject(iface);

    TRACE("iface %p, index %lu, buffer %p, flags %#lx, timestamp %s, timelength %s.\n", iface,
             index, buffer, flags, wine_dbgstr_longlong(timestamp), wine_dbgstr_longlong(timelength));

    if (!decoder->wg_transform)
        return DMO_E_TYPE_NOT_SET;

    return wg_transform_push_dmo(decoder->wg_transform, buffer, flags, timestamp, timelength, decoder->wg_sample_queue);
}

static HRESULT WINAPI media_object_ProcessOutput(IMediaObject *iface, DWORD flags, DWORD count,
        DMO_OUTPUT_DATA_BUFFER *buffers, DWORD *status)
{
    struct wma_decoder *decoder = impl_from_IMediaObject(iface);
    HRESULT hr;

    TRACE("iface %p, flags %#lx, count %lu, buffers %p, status %p.\n", iface, flags, count, buffers, status);

    if (!decoder->wg_transform)
        return DMO_E_TYPE_NOT_SET;

    hr = wg_transform_read_dmo(decoder->wg_transform, buffers);

    if (SUCCEEDED(hr))
    {
        /* WMA Lossless emits anything from 0 to 12 packets of output for each packet of input */
        buffers[0].dwStatus |= DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE;
        wg_sample_queue_flush(decoder->wg_sample_queue, false);
    }

    return hr;
}

static HRESULT WINAPI media_object_Lock(IMediaObject *iface, LONG lock)
{
    FIXME("iface %p, lock %ld stub!\n", iface, lock);
    return E_NOTIMPL;
}

static const IMediaObjectVtbl media_object_vtbl =
{
    media_object_QueryInterface,
    media_object_AddRef,
    media_object_Release,
    media_object_GetStreamCount,
    media_object_GetInputStreamInfo,
    media_object_GetOutputStreamInfo,
    media_object_GetInputType,
    media_object_GetOutputType,
    media_object_SetInputType,
    media_object_SetOutputType,
    media_object_GetInputCurrentType,
    media_object_GetOutputCurrentType,
    media_object_GetInputSizeInfo,
    media_object_GetOutputSizeInfo,
    media_object_GetInputMaxLatency,
    media_object_SetInputMaxLatency,
    media_object_Flush,
    media_object_Discontinuity,
    media_object_AllocateStreamingResources,
    media_object_FreeStreamingResources,
    media_object_GetInputStatus,
    media_object_ProcessInput,
    media_object_ProcessOutput,
    media_object_Lock,
};

static inline struct wma_decoder *impl_from_IPropertyBag(IPropertyBag *iface)
{
    return CONTAINING_RECORD(iface, struct wma_decoder, IPropertyBag_iface);
}

static HRESULT WINAPI property_bag_QueryInterface(IPropertyBag *iface, REFIID iid, void **out)
{
    struct wma_decoder *filter = impl_from_IPropertyBag(iface);
    return IUnknown_QueryInterface(filter->outer, iid, out);
}

static ULONG WINAPI property_bag_AddRef(IPropertyBag *iface)
{
    struct wma_decoder *filter = impl_from_IPropertyBag(iface);
    return IUnknown_AddRef(filter->outer);
}

static ULONG WINAPI property_bag_Release(IPropertyBag *iface)
{
    struct wma_decoder *filter = impl_from_IPropertyBag(iface);
    return IUnknown_Release(filter->outer);
}

static HRESULT WINAPI property_bag_Read(IPropertyBag *iface, const WCHAR *prop_name, VARIANT *value,
        IErrorLog *error_log)
{
    FIXME("iface %p, prop_name %s, value %p, error_log %p stub!\n", iface, debugstr_w(prop_name), value, error_log);
    return E_NOTIMPL;
}

static HRESULT WINAPI property_bag_Write(IPropertyBag *iface, const WCHAR *prop_name, VARIANT *value)
{
    FIXME("iface %p, prop_name %s, value %p stub!\n", iface, debugstr_w(prop_name), value);
    return S_OK;
}

static const IPropertyBagVtbl property_bag_vtbl =
{
    property_bag_QueryInterface,
    property_bag_AddRef,
    property_bag_Release,
    property_bag_Read,
    property_bag_Write,
};

HRESULT wma_decoder_create(IUnknown *outer, IUnknown **out)
{
    static const WAVEFORMATEX output_format =
    {
        .wFormatTag = WAVE_FORMAT_IEEE_FLOAT, .wBitsPerSample = 32, .nSamplesPerSec = 44100, .nChannels = 1,
    };
    static const WMAUDIO2WAVEFORMAT input_format =
    {
        .wfx = {.wFormatTag = WAVE_FORMAT_WMAUDIO2, .wBitsPerSample = 16, .nSamplesPerSec = 44100, .nChannels = 1,
                .nAvgBytesPerSec = 3000, .nBlockAlign = 139, .cbSize = sizeof(input_format) - sizeof(WAVEFORMATEX)},
        .wEncodeOptions = 1,
    };
    struct wma_decoder *decoder;
    HRESULT hr;

    TRACE("outer %p, out %p.\n", outer, out);

    if (FAILED(hr = check_audio_transform_support(&input_format.wfx, &output_format)))
    {
        ERR_(winediag)("GStreamer doesn't support WMA decoding, please install appropriate plugins.\n");
        return hr;
    }

    if (!(decoder = calloc(1, sizeof(*decoder))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = wg_sample_queue_create(&decoder->wg_sample_queue)))
    {
        free(decoder);
        return hr;
    }

    decoder->IUnknown_inner.lpVtbl = &unknown_vtbl;
    decoder->IMFTransform_iface.lpVtbl = &transform_vtbl;
    decoder->IMediaObject_iface.lpVtbl = &media_object_vtbl;
    decoder->IPropertyBag_iface.lpVtbl = &property_bag_vtbl;
    decoder->refcount = 1;
    decoder->outer = outer ? outer : &decoder->IUnknown_inner;

    *out = &decoder->IUnknown_inner;
    TRACE("Created decoder %p\n", *out);
    return S_OK;
}
