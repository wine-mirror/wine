/* AAC Decoder Transform
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
#include "ks.h"
#include "ksmedia.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

#define NEXT_WAVEFORMATEXTENSIBLE(format) (WAVEFORMATEXTENSIBLE *)((BYTE *)(&(format)->Format + 1) + (format)->Format.cbSize)

static WAVEFORMATEXTENSIBLE const aac_decoder_output_types[] =
{
    {.Format = {.wFormatTag = WAVE_FORMAT_IEEE_FLOAT, .wBitsPerSample = 32, .nSamplesPerSec = 48000, .nChannels = 2}},
    {.Format = {.wFormatTag = WAVE_FORMAT_PCM, .wBitsPerSample = 16, .nSamplesPerSec = 48000, .nChannels = 2}},
};

static const UINT32 default_channel_mask[7] =
{
    0,
    0,
    0,
    SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_CENTER,
    KSAUDIO_SPEAKER_QUAD,
    KSAUDIO_SPEAKER_QUAD | SPEAKER_FRONT_CENTER,
    KSAUDIO_SPEAKER_5POINT1,
};

struct aac_decoder
{
    IMFTransform IMFTransform_iface;
    LONG refcount;

    UINT input_type_count;
    WAVEFORMATEXTENSIBLE *input_types;

    IMFMediaType *input_type;
    IMFMediaType *output_type;

    wg_transform_t wg_transform;
    struct wg_sample_queue *wg_sample_queue;
};

static struct aac_decoder *impl_from_IMFTransform(IMFTransform *iface)
{
    return CONTAINING_RECORD(iface, struct aac_decoder, IMFTransform_iface);
}

static HRESULT try_create_wg_transform(struct aac_decoder *decoder)
{
    struct wg_format input_format, output_format;
    struct wg_transform_attrs attrs = {0};

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
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI transform_QueryInterface(IMFTransform *iface, REFIID iid, void **out)
{
    struct aac_decoder *decoder = impl_from_IMFTransform(iface);

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

static ULONG WINAPI transform_AddRef(IMFTransform *iface)
{
    struct aac_decoder *decoder = impl_from_IMFTransform(iface);
    ULONG refcount = InterlockedIncrement(&decoder->refcount);
    TRACE("iface %p increasing refcount to %lu.\n", decoder, refcount);
    return refcount;
}

static ULONG WINAPI transform_Release(IMFTransform *iface)
{
    struct aac_decoder *decoder = impl_from_IMFTransform(iface);
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
    TRACE("iface %p, id %#lx, info %p.\n", iface, id, info);

    if (id)
        return MF_E_INVALIDSTREAMNUMBER;

    memset(info, 0, sizeof(*info));
    info->dwFlags = MFT_INPUT_STREAM_WHOLE_SAMPLES | MFT_INPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER
            | MFT_INPUT_STREAM_FIXED_SAMPLE_SIZE | MFT_INPUT_STREAM_HOLDS_BUFFERS;

    return S_OK;
}

static HRESULT WINAPI transform_GetOutputStreamInfo(IMFTransform *iface, DWORD id, MFT_OUTPUT_STREAM_INFO *info)
{
    TRACE("iface %p, id %#lx, info %p.\n", iface, id, info);

    if (id)
        return MF_E_INVALIDSTREAMNUMBER;

    memset(info, 0, sizeof(*info));
    info->dwFlags = MFT_INPUT_STREAM_WHOLE_SAMPLES;
    info->cbSize = 0xc000;

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
    struct aac_decoder *decoder = impl_from_IMFTransform(iface);
    const WAVEFORMATEXTENSIBLE *format = decoder->input_types;
    UINT count = decoder->input_type_count;

    TRACE("iface %p, id %#lx, index %#lx, type %p.\n", iface, id, index, type);

    *type = NULL;
    if (id)
        return MF_E_INVALIDSTREAMNUMBER;
    for (format = decoder->input_types; index > 0 && count > 0; index--, count--)
        format = NEXT_WAVEFORMATEXTENSIBLE(format);
    return count ? MFCreateAudioMediaType(&format->Format, (IMFAudioMediaType **)type) : MF_E_NO_MORE_TYPES;
}

static HRESULT WINAPI transform_GetOutputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    struct aac_decoder *decoder = impl_from_IMFTransform(iface);
    UINT32 channel_count, sample_rate;
    WAVEFORMATEXTENSIBLE wfx = {{0}};
    IMFMediaType *media_type;
    HRESULT hr;

    TRACE("iface %p, id %#lx, index %#lx, type %p.\n", iface, id, index, type);

    *type = NULL;

    if (id)
        return MF_E_INVALIDSTREAMNUMBER;
    if (!decoder->input_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    wfx = aac_decoder_output_types[index % ARRAY_SIZE(aac_decoder_output_types)];

    if (FAILED(hr = IMFMediaType_GetUINT32(decoder->input_type, &MF_MT_AUDIO_NUM_CHANNELS, &channel_count))
            || !channel_count)
        channel_count = wfx.Format.nChannels;
    if (FAILED(hr = IMFMediaType_GetUINT32(decoder->input_type, &MF_MT_AUDIO_SAMPLES_PER_SECOND, &sample_rate)))
        sample_rate = wfx.Format.nSamplesPerSec;

    if (channel_count >= ARRAY_SIZE(default_channel_mask))
        return MF_E_INVALIDMEDIATYPE;

    if (channel_count > 2 && index >= ARRAY_SIZE(aac_decoder_output_types))
    {
        /* If there are more than two channels in the input type GetOutputAvailableType additionally lists
         * types with 2 channels. */
        index -= ARRAY_SIZE(aac_decoder_output_types);
        channel_count = 2;
    }

    if (index >= ARRAY_SIZE(aac_decoder_output_types))
        return MF_E_NO_MORE_TYPES;

    wfx.Format.nChannels = channel_count;
    wfx.Format.nSamplesPerSec = sample_rate;
    wfx.Format.nBlockAlign = wfx.Format.wBitsPerSample * wfx.Format.nChannels / 8;
    wfx.Format.nAvgBytesPerSec = wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign;

    if (wfx.Format.nChannels >= 3)
    {
        wfx.SubFormat = MFAudioFormat_Base;
        wfx.SubFormat.Data1 = wfx.Format.wFormatTag;
        wfx.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        wfx.dwChannelMask = default_channel_mask[wfx.Format.nChannels];
    }

    if (FAILED(hr = MFCreateAudioMediaType(&wfx.Format, (IMFAudioMediaType **)&media_type)))
        return hr;
    if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_FIXED_SIZE_SAMPLES, 1)))
        goto done;

done:
    if (SUCCEEDED(hr))
        IMFMediaType_AddRef((*type = media_type));

    IMFMediaType_Release(media_type);
    return hr;
}

static BOOL matches_format(const WAVEFORMATEXTENSIBLE *a, const WAVEFORMATEXTENSIBLE *b)
{
    if (a->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE && b->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        return IsEqualGUID(&a->SubFormat, &b->SubFormat);
    if (a->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        return a->SubFormat.Data1 == b->Format.wFormatTag;
    if (b->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        return b->SubFormat.Data1 == a->Format.wFormatTag;
    return a->Format.wFormatTag == b->Format.wFormatTag;
}

static HRESULT WINAPI transform_SetInputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct aac_decoder *decoder = impl_from_IMFTransform(iface);
    UINT32 size, count = decoder->input_type_count;
    WAVEFORMATEXTENSIBLE *format, wfx;
    HRESULT hr;

    TRACE("iface %p, id %#lx, type %p, flags %#lx.\n", iface, id, type, flags);

    if (id)
        return MF_E_INVALIDSTREAMNUMBER;

    if (FAILED(hr = MFCreateWaveFormatExFromMFMediaType(type, (WAVEFORMATEX **)&format, &size,
            MFWaveFormatExConvertFlag_ForceExtensible)))
        return hr;
    wfx = *format;
    CoTaskMemFree(format);

    for (format = decoder->input_types; count > 0 && !matches_format(format, &wfx); count--)
        format = NEXT_WAVEFORMATEXTENSIBLE(format);
    if (!count)
        return MF_E_INVALIDMEDIATYPE;

    if (wfx.Format.nChannels >= ARRAY_SIZE(default_channel_mask) || !wfx.Format.nSamplesPerSec || !wfx.Format.cbSize)
        return MF_E_INVALIDMEDIATYPE;
    if (flags & MFT_SET_TYPE_TEST_ONLY)
        return S_OK;

    if (!decoder->input_type && FAILED(hr = MFCreateMediaType(&decoder->input_type)))
        return hr;

    if (decoder->output_type)
    {
        IMFMediaType_Release(decoder->output_type);
        decoder->output_type = NULL;
    }

    return IMFMediaType_CopyAllItems(type, (IMFAttributes *)decoder->input_type);
}

static HRESULT WINAPI transform_SetOutputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct aac_decoder *decoder = impl_from_IMFTransform(iface);
    WAVEFORMATEXTENSIBLE *format, wfx;
    UINT32 size;
    HRESULT hr;
    ULONG i;

    TRACE("iface %p, id %#lx, type %p, flags %#lx.\n", iface, id, type, flags);

    if (id)
        return MF_E_INVALIDSTREAMNUMBER;
    if (!decoder->input_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    if (FAILED(hr = MFCreateWaveFormatExFromMFMediaType(type, (WAVEFORMATEX **)&format, &size,
            MFWaveFormatExConvertFlag_ForceExtensible)))
        return hr;
    wfx = *format;
    CoTaskMemFree(format);

    for (i = 0; i < ARRAY_SIZE(aac_decoder_output_types); ++i)
        if (matches_format(&aac_decoder_output_types[i], &wfx))
            break;
    if (i == ARRAY_SIZE(aac_decoder_output_types))
        return MF_E_INVALIDMEDIATYPE;

    if (!wfx.Format.wBitsPerSample || !wfx.Format.nChannels || !wfx.Format.nSamplesPerSec)
        return MF_E_INVALIDMEDIATYPE;
    if (flags & MFT_SET_TYPE_TEST_ONLY)
        return S_OK;

    if (!decoder->output_type && FAILED(hr = MFCreateMediaType(&decoder->output_type)))
        return hr;

    if (FAILED(hr = IMFMediaType_CopyAllItems(type, (IMFAttributes *)decoder->output_type)))
        return hr;

    if (FAILED(hr = try_create_wg_transform(decoder)))
        goto failed;

    return S_OK;

failed:
    IMFMediaType_Release(decoder->output_type);
    decoder->output_type = NULL;
    return hr;
}

static HRESULT WINAPI transform_GetInputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **out)
{
    struct aac_decoder *decoder = impl_from_IMFTransform(iface);
    IMFMediaType *type;
    HRESULT hr;

    TRACE("iface %p, id %#lx, out %p.\n", iface, id, out);

    if (id)
        return MF_E_INVALIDSTREAMNUMBER;
    if (!decoder->input_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    if (FAILED(hr = MFCreateMediaType(&type)))
        return hr;
    if (SUCCEEDED(hr = IMFMediaType_CopyAllItems(decoder->input_type, (IMFAttributes *)type)))
        IMFMediaType_AddRef(*out = type);
    IMFMediaType_Release(type);

    return hr;
}

static HRESULT WINAPI transform_GetOutputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **out)
{
    struct aac_decoder *decoder = impl_from_IMFTransform(iface);
    IMFMediaType *type;
    HRESULT hr;

    TRACE("iface %p, id %#lx, out %p.\n", iface, id, out);

    if (id)
        return MF_E_INVALIDSTREAMNUMBER;
    if (!decoder->output_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    if (FAILED(hr = MFCreateMediaType(&type)))
        return hr;
    if (SUCCEEDED(hr = IMFMediaType_CopyAllItems(decoder->output_type, (IMFAttributes *)type)))
        IMFMediaType_AddRef(*out = type);
    IMFMediaType_Release(type);

    return hr;
}

static HRESULT WINAPI transform_GetInputStatus(IMFTransform *iface, DWORD id, DWORD *flags)
{
    struct aac_decoder *decoder = impl_from_IMFTransform(iface);
    bool accepts_input;

    TRACE("iface %p, id %#lx, flags %p.\n", iface, id, flags);

    if (!decoder->wg_transform)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    if (!wg_transform_get_status(decoder->wg_transform, &accepts_input))
        return E_FAIL;

    *flags = accepts_input ? MFT_INPUT_STATUS_ACCEPT_DATA : 0;
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
    FIXME("iface %p, message %#x, param %p stub!\n", iface, message, (void *)param);
    return S_OK;
}

static HRESULT WINAPI transform_ProcessInput(IMFTransform *iface, DWORD id, IMFSample *sample, DWORD flags)
{
    struct aac_decoder *decoder = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, sample %p, flags %#lx.\n", iface, id, sample, flags);

    if (!decoder->wg_transform)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    return wg_transform_push_mf(decoder->wg_transform, sample, decoder->wg_sample_queue);
}

static HRESULT WINAPI transform_ProcessOutput(IMFTransform *iface, DWORD flags, DWORD count,
        MFT_OUTPUT_DATA_BUFFER *samples, DWORD *status)
{
    struct aac_decoder *decoder = impl_from_IMFTransform(iface);
    MFT_OUTPUT_STREAM_INFO info;
    HRESULT hr;

    TRACE("iface %p, flags %#lx, count %lu, samples %p, status %p.\n", iface, flags, count, samples, status);

    if (count != 1)
        return E_INVALIDARG;

    if (!decoder->wg_transform)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    *status = samples->dwStatus = 0;
    if (!samples->pSample)
        return E_INVALIDARG;

    if (FAILED(hr = IMFTransform_GetOutputStreamInfo(iface, 0, &info)))
        return hr;

    if (SUCCEEDED(hr = wg_transform_read_mf(decoder->wg_transform, samples->pSample,
            info.cbSize, NULL, &samples->dwStatus)))
        wg_sample_queue_flush(decoder->wg_sample_queue, false);
    else
        samples->dwStatus = MFT_OUTPUT_DATA_BUFFER_NO_SAMPLE;

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

static HEAACWAVEINFO aac_decoder_input_types[] =
{
#define MAKE_HEAACWAVEINFO(format, payload) \
    {.wfx = {.wFormatTag = format, .nChannels = 6, .nSamplesPerSec = 48000, .nAvgBytesPerSec = 1152000, \
             .nBlockAlign = 24, .wBitsPerSample = 32, .cbSize = sizeof(HEAACWAVEINFO) - sizeof(WAVEFORMATEX)}, \
     .wPayloadType = payload}

    MAKE_HEAACWAVEINFO(WAVE_FORMAT_MPEG_HEAAC, 0),
    MAKE_HEAACWAVEINFO(WAVE_FORMAT_RAW_AAC1, 0),
    MAKE_HEAACWAVEINFO(WAVE_FORMAT_MPEG_HEAAC, 1),
    MAKE_HEAACWAVEINFO(WAVE_FORMAT_MPEG_HEAAC, 3),
    MAKE_HEAACWAVEINFO(WAVE_FORMAT_MPEG_ADTS_AAC, 0),

#undef MAKE_HEAACWAVEINFO
};

HRESULT aac_decoder_create(REFIID riid, void **ret)
{
    static const struct wg_format output_format =
    {
        .major_type = WG_MAJOR_TYPE_AUDIO,
        .u.audio =
        {
            .format = WG_AUDIO_FORMAT_F32LE,
            .channel_mask = 1,
            .channels = 1,
            .rate = 44100,
        },
    };
    static const struct wg_format input_format = {.major_type = WG_MAJOR_TYPE_AUDIO_MPEG4};
    struct wg_transform_attrs attrs = {0};
    wg_transform_t transform;
    struct aac_decoder *decoder;
    HRESULT hr;

    TRACE("riid %s, ret %p.\n", debugstr_guid(riid), ret);

    if (!(transform = wg_transform_create(&input_format, &output_format, &attrs)))
    {
        ERR_(winediag)("GStreamer doesn't support WMA decoding, please install appropriate plugins\n");
        return E_FAIL;
    }
    wg_transform_destroy(transform);

    if (!(decoder = calloc(1, sizeof(*decoder))))
        return E_OUTOFMEMORY;
    decoder->input_types = (WAVEFORMATEXTENSIBLE *)aac_decoder_input_types;
    decoder->input_type_count = ARRAY_SIZE(aac_decoder_input_types);

    if (FAILED(hr = wg_sample_queue_create(&decoder->wg_sample_queue)))
    {
        free(decoder);
        return hr;
    }

    decoder->IMFTransform_iface.lpVtbl = &transform_vtbl;
    decoder->refcount = 1;

    *ret = &decoder->IMFTransform_iface;
    TRACE("Created decoder %p\n", *ret);
    return S_OK;
}
