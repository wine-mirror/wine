/* GStreamer Audio Converter
 *
 * Copyright 2020 Derek Lesho
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
#include "ks.h"
#include "ksmedia.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

struct audio_converter
{
    IMFTransform IMFTransform_iface;
    LONG refcount;
    IMFMediaType *input_type;
    IMFMediaType *output_type;
    CRITICAL_SECTION cs;
    BOOL buffer_inflight;
    LONGLONG buffer_pts, buffer_dur;
    struct wg_parser *parser;
    struct wg_parser_stream *stream;
    IMFAttributes *attributes, *output_attributes;
};

static struct audio_converter *impl_audio_converter_from_IMFTransform(IMFTransform *iface)
{
    return CONTAINING_RECORD(iface, struct audio_converter, IMFTransform_iface);
}

static HRESULT WINAPI audio_converter_QueryInterface(IMFTransform *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFTransform) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFTransform_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI audio_converter_AddRef(IMFTransform *iface)
{
    struct audio_converter *transform = impl_audio_converter_from_IMFTransform(iface);
    ULONG refcount = InterlockedIncrement(&transform->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI audio_converter_Release(IMFTransform *iface)
{
    struct audio_converter *transform = impl_audio_converter_from_IMFTransform(iface);
    ULONG refcount = InterlockedDecrement(&transform->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        transform->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&transform->cs);
        if (transform->attributes)
            IMFAttributes_Release(transform->attributes);
        if (transform->output_attributes)
            IMFAttributes_Release(transform->output_attributes);
        if (transform->stream)
            wg_parser_disconnect(transform->parser);
        if (transform->parser)
            wg_parser_destroy(transform->parser);
        free(transform);
    }

    return refcount;
}

static HRESULT WINAPI audio_converter_GetStreamLimits(IMFTransform *iface, DWORD *input_minimum, DWORD *input_maximum,
        DWORD *output_minimum, DWORD *output_maximum)
{
    TRACE("%p, %p, %p, %p, %p.\n", iface, input_minimum, input_maximum, output_minimum, output_maximum);

    *input_minimum = *input_maximum = *output_minimum = *output_maximum = 1;

    return S_OK;
}

static HRESULT WINAPI audio_converter_GetStreamCount(IMFTransform *iface, DWORD *inputs, DWORD *outputs)
{
    TRACE("%p, %p, %p.\n", iface, inputs, outputs);

    *inputs = *outputs = 1;

    return S_OK;
}

static HRESULT WINAPI audio_converter_GetStreamIDs(IMFTransform *iface, DWORD input_size, DWORD *inputs,
        DWORD output_size, DWORD *outputs)
{
    TRACE("%p %u %p %u %p.\n", iface, input_size, inputs, output_size, outputs);

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_converter_GetInputStreamInfo(IMFTransform *iface, DWORD id, MFT_INPUT_STREAM_INFO *info)
{
    struct audio_converter *converter = impl_audio_converter_from_IMFTransform(iface);

    TRACE("%p %u %p.\n", iface, id, info);

    if (id != 0)
        return MF_E_INVALIDSTREAMNUMBER;

    info->dwFlags = MFT_INPUT_STREAM_WHOLE_SAMPLES | MFT_INPUT_STREAM_DOES_NOT_ADDREF;
    info->cbMaxLookahead = 0;
    info->cbAlignment = 0;
    info->hnsMaxLatency = 0;
    info->cbSize = 0;

    EnterCriticalSection(&converter->cs);

    if (converter->input_type)
        IMFMediaType_GetUINT32(converter->input_type, &MF_MT_AUDIO_BLOCK_ALIGNMENT, &info->cbSize);

    LeaveCriticalSection(&converter->cs);

    return S_OK;
}

static HRESULT WINAPI audio_converter_GetOutputStreamInfo(IMFTransform *iface, DWORD id, MFT_OUTPUT_STREAM_INFO *info)
{
    struct audio_converter *converter = impl_audio_converter_from_IMFTransform(iface);

    TRACE("%p %u %p.\n", iface, id, info);

    if (id != 0)
        return MF_E_INVALIDSTREAMNUMBER;

    info->dwFlags = MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES | MFT_OUTPUT_STREAM_WHOLE_SAMPLES;
    info->cbAlignment = 0;
    info->cbSize = 0;

    EnterCriticalSection(&converter->cs);

    if (converter->output_type)
        IMFMediaType_GetUINT32(converter->output_type, &MF_MT_AUDIO_BLOCK_ALIGNMENT, &info->cbSize);

    LeaveCriticalSection(&converter->cs);

    return S_OK;
}

static HRESULT WINAPI audio_converter_GetAttributes(IMFTransform *iface, IMFAttributes **attributes)
{
    struct audio_converter *converter = impl_audio_converter_from_IMFTransform(iface);

    TRACE("%p, %p.\n", iface, attributes);

    *attributes = converter->attributes;
    IMFAttributes_AddRef(*attributes);

    return S_OK;
}

static HRESULT WINAPI audio_converter_GetInputStreamAttributes(IMFTransform *iface, DWORD id,
        IMFAttributes **attributes)
{
    FIXME("%p, %u, %p.\n", iface, id, attributes);

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_converter_GetOutputStreamAttributes(IMFTransform *iface, DWORD id,
        IMFAttributes **attributes)
{
    struct audio_converter *converter = impl_audio_converter_from_IMFTransform(iface);

    TRACE("%p, %u, %p.\n", iface, id, attributes);

    if (id != 0)
        return MF_E_INVALIDSTREAMNUMBER;

    *attributes = converter->output_attributes;
    IMFAttributes_AddRef(*attributes);

    return S_OK;
}

static HRESULT WINAPI audio_converter_DeleteInputStream(IMFTransform *iface, DWORD id)
{
    TRACE("%p, %u.\n", iface, id);

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_converter_AddInputStreams(IMFTransform *iface, DWORD streams, DWORD *ids)
{
    TRACE("%p, %u, %p.\n", iface, streams, ids);

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_converter_GetInputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    IMFMediaType *ret;
    HRESULT hr;

    TRACE("%p, %u, %u, %p.\n", iface, id, index, type);

    if (id != 0)
        return MF_E_INVALIDSTREAMNUMBER;

    if (index >= 2)
        return MF_E_NO_MORE_TYPES;

    if (FAILED(hr = MFCreateMediaType(&ret)))
        return hr;

    if (FAILED(hr = IMFMediaType_SetGUID(ret, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio)))
    {
        IMFMediaType_Release(ret);
        return hr;
    }

    if (FAILED(hr = IMFMediaType_SetGUID(ret, &MF_MT_SUBTYPE, index ? &MFAudioFormat_Float : &MFAudioFormat_PCM)))
    {
        IMFMediaType_Release(ret);
        return hr;
    }

    *type = ret;

    return S_OK;
}

static HRESULT WINAPI audio_converter_GetOutputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    IMFMediaType *output_type;
    HRESULT hr;

    static const struct
    {
        const GUID *subtype;
        DWORD depth;
    }
    formats[] =
    {
        {&MFAudioFormat_PCM, 16},
        {&MFAudioFormat_PCM, 24},
        {&MFAudioFormat_PCM, 32},
        {&MFAudioFormat_Float, 32},
    };

    static const DWORD rates[] = {44100, 48000};
    static const DWORD channel_cnts[] = {1, 2, 6};
    const GUID *subtype;
    DWORD rate, channels, bps;

    TRACE("%p, %u, %u, %p.\n", iface, id, index, type);

    if (id != 0)
        return MF_E_INVALIDSTREAMNUMBER;

    if (index >= ARRAY_SIZE(formats) * 2/*rates*/ * 3/*layouts*/)
        return MF_E_NO_MORE_TYPES;

    if (FAILED(hr = MFCreateMediaType(&output_type)))
        return hr;

    subtype = formats[index / 6].subtype;
    bps = formats[index / 6].depth;
    rate = rates[index % 2];
    channels = channel_cnts[(index / 2) % 3];

    if (FAILED(hr = IMFMediaType_SetGUID(output_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio)))
        goto fail;
    if (FAILED(hr = IMFMediaType_SetGUID(output_type, &MF_MT_SUBTYPE, subtype)))
        goto fail;
    if (FAILED(hr = IMFMediaType_SetUINT32(output_type, &MF_MT_AUDIO_SAMPLES_PER_SECOND, rate)))
        goto fail;
    if (FAILED(hr = IMFMediaType_SetUINT32(output_type, &MF_MT_AUDIO_NUM_CHANNELS, channels)))
        goto fail;
    if (FAILED(hr = IMFMediaType_SetUINT32(output_type, &MF_MT_AUDIO_BITS_PER_SAMPLE, bps)))
        goto fail;

    if (FAILED(hr = IMFMediaType_SetUINT32(output_type, &MF_MT_AUDIO_BLOCK_ALIGNMENT, channels * bps / 8)))
        goto fail;
    if (FAILED(hr = IMFMediaType_SetUINT32(output_type, &MF_MT_AUDIO_AVG_BYTES_PER_SECOND, rate * channels * bps / 8)))
        goto fail;
    if (FAILED(hr = IMFMediaType_SetUINT32(output_type, &MF_MT_AUDIO_CHANNEL_MASK,
            channels == 1 ? KSAUDIO_SPEAKER_MONO :
            channels == 2 ? KSAUDIO_SPEAKER_STEREO :
          /*channels == 6*/ KSAUDIO_SPEAKER_5POINT1)))
        goto fail;
    if (FAILED(hr = IMFMediaType_SetUINT32(output_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE)))
        goto fail;

    *type = output_type;

    return S_OK;
fail:
    IMFMediaType_Release(output_type);
    return hr;
}

static HRESULT WINAPI audio_converter_SetInputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    GUID major_type, subtype;
    struct wg_format format;
    DWORD unused;
    HRESULT hr;

    struct audio_converter *converter = impl_audio_converter_from_IMFTransform(iface);

    TRACE("%p, %u, %p, %#x.\n", iface, id, type, flags);

    if (id != 0)
        return MF_E_INVALIDSTREAMNUMBER;

    if (!type)
    {
        if (flags & MFT_SET_TYPE_TEST_ONLY)
            return S_OK;

        EnterCriticalSection(&converter->cs);

        if (converter->input_type)
        {
            if (converter->stream)
            {
                wg_parser_disconnect(converter->parser);
                converter->stream = NULL;
            }
            IMFMediaType_Release(converter->input_type);
            converter->input_type = NULL;
        }

        LeaveCriticalSection(&converter->cs);

        return S_OK;
    }

    if (FAILED(IMFMediaType_GetGUID(type, &MF_MT_MAJOR_TYPE, &major_type)))
        return MF_E_INVALIDTYPE;
    if (FAILED(IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &subtype)))
        return MF_E_INVALIDTYPE;
    if (FAILED(IMFMediaType_GetUINT32(type, &MF_MT_AUDIO_SAMPLES_PER_SECOND, &unused)))
        return MF_E_INVALIDTYPE;
    if (FAILED(IMFMediaType_GetUINT32(type, &MF_MT_AUDIO_NUM_CHANNELS, &unused)))
        return MF_E_INVALIDTYPE;
    if (IsEqualGUID(&subtype, &MFAudioFormat_PCM) && FAILED(IMFMediaType_GetUINT32(type, &MF_MT_AUDIO_BITS_PER_SAMPLE, &unused)))
        return MF_E_INVALIDTYPE;

    if (!(IsEqualGUID(&major_type, &MFMediaType_Audio)))
        return MF_E_INVALIDTYPE;

    if (!IsEqualGUID(&subtype, &MFAudioFormat_PCM) && !IsEqualGUID(&subtype, &MFAudioFormat_Float))
        return MF_E_INVALIDTYPE;

    mf_media_type_to_wg_format(type, &format);
    if (!format.major_type)
        return MF_E_INVALIDTYPE;

    if (flags & MFT_SET_TYPE_TEST_ONLY)
        return S_OK;

    EnterCriticalSection(&converter->cs);

    hr = S_OK;

    if (!converter->input_type)
        hr = MFCreateMediaType(&converter->input_type);

    if (SUCCEEDED(hr))
        hr = IMFMediaType_CopyAllItems(type, (IMFAttributes *) converter->input_type);

    if (FAILED(hr))
    {
        IMFMediaType_Release(converter->input_type);
        converter->input_type = NULL;
    }

    if (converter->stream)
    {
        wg_parser_disconnect(converter->parser);
        converter->stream = NULL;
    }

    if (converter->input_type && converter->output_type)
    {
        struct wg_format output_format;
        mf_media_type_to_wg_format(converter->output_type, &output_format);

        if (SUCCEEDED(hr = wg_parser_connect_unseekable(converter->parser, &format, 1, &output_format, NULL)))
            converter->stream = wg_parser_get_stream(converter->parser, 0);
    }

    LeaveCriticalSection(&converter->cs);

    return hr;
}

static HRESULT WINAPI audio_converter_SetOutputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct audio_converter *converter = impl_audio_converter_from_IMFTransform(iface);
    GUID major_type, subtype;
    struct wg_format format;
    DWORD unused;
    HRESULT hr;

    TRACE("%p, %u, %p, %#x.\n", iface, id, type, flags);

    if (id != 0)
        return MF_E_INVALIDSTREAMNUMBER;

    if (!type)
    {
        if (flags & MFT_SET_TYPE_TEST_ONLY)
            return S_OK;

        EnterCriticalSection(&converter->cs);

        if (converter->output_type)
        {
            if (converter->stream)
            {
                wg_parser_disconnect(converter->parser);
                converter->stream = NULL;
            }
            IMFMediaType_Release(converter->output_type);
            converter->output_type = NULL;
        }

        LeaveCriticalSection(&converter->cs);

        return S_OK;
    }

    if (FAILED(IMFMediaType_GetGUID(type, &MF_MT_MAJOR_TYPE, &major_type)))
        return MF_E_INVALIDTYPE;
    if (FAILED(IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &subtype)))
        return MF_E_INVALIDTYPE;
    if (FAILED(IMFMediaType_GetUINT32(type, &MF_MT_AUDIO_NUM_CHANNELS, &unused)))
        return MF_E_INVALIDTYPE;
    if (IsEqualGUID(&subtype, &MFAudioFormat_PCM) && FAILED(IMFMediaType_GetUINT32(type, &MF_MT_AUDIO_BITS_PER_SAMPLE, &unused)))
        return MF_E_INVALIDTYPE;
    if (FAILED(IMFMediaType_GetUINT32(type, &MF_MT_AUDIO_SAMPLES_PER_SECOND, &unused)))
        return MF_E_INVALIDTYPE;

    if (!(IsEqualGUID(&major_type, &MFMediaType_Audio)))
        return MF_E_INVALIDTYPE;

    if (!IsEqualGUID(&subtype, &MFAudioFormat_PCM) && !IsEqualGUID(&subtype, &MFAudioFormat_Float))
        return MF_E_INVALIDTYPE;

    mf_media_type_to_wg_format(type, &format);
    if (!format.major_type)
        return MF_E_INVALIDTYPE;

    if (flags & MFT_SET_TYPE_TEST_ONLY)
        return S_OK;

    EnterCriticalSection(&converter->cs);

    hr = S_OK;

    if (!converter->output_type)
        hr = MFCreateMediaType(&converter->output_type);

    if (SUCCEEDED(hr))
        hr = IMFMediaType_CopyAllItems(type, (IMFAttributes *) converter->output_type);

    if (FAILED(hr))
    {
        IMFMediaType_Release(converter->output_type);
        converter->output_type = NULL;
    }

    if (converter->stream)
    {
        wg_parser_disconnect(converter->parser);
        converter->stream = NULL;
    }

    if (converter->input_type && converter->output_type)
    {
        struct wg_format input_format;
        mf_media_type_to_wg_format(converter->input_type, &input_format);

        if (SUCCEEDED(hr = wg_parser_connect_unseekable(converter->parser, &input_format, 1, &format, NULL)))
            converter->stream = wg_parser_get_stream(converter->parser, 0);
    }

    LeaveCriticalSection(&converter->cs);

    return hr;
}

static HRESULT WINAPI audio_converter_GetInputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct audio_converter *converter = impl_audio_converter_from_IMFTransform(iface);
    IMFMediaType *ret;
    HRESULT hr;

    TRACE("%p, %u, %p.\n", converter, id, type);

    if (id != 0)
        return MF_E_INVALIDSTREAMNUMBER;

    if (FAILED(hr = MFCreateMediaType(&ret)))
        return hr;

    EnterCriticalSection(&converter->cs);

    if (converter->input_type)
        hr = IMFMediaType_CopyAllItems(converter->input_type, (IMFAttributes *)ret);
    else
        hr = MF_E_TRANSFORM_TYPE_NOT_SET;

    LeaveCriticalSection(&converter->cs);

    if (SUCCEEDED(hr))
        *type = ret;
    else
        IMFMediaType_Release(ret);

    return hr;
}

static HRESULT WINAPI audio_converter_GetOutputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct audio_converter *converter = impl_audio_converter_from_IMFTransform(iface);
    IMFMediaType *ret;
    HRESULT hr;

    TRACE("%p, %u, %p.\n", converter, id, type);

    if (id != 0)
        return MF_E_INVALIDSTREAMNUMBER;

    if (FAILED(hr = MFCreateMediaType(&ret)))
        return hr;

    EnterCriticalSection(&converter->cs);

    if (converter->output_type)
        hr = IMFMediaType_CopyAllItems(converter->output_type, (IMFAttributes *)ret);
    else
        hr = MF_E_TRANSFORM_TYPE_NOT_SET;

    LeaveCriticalSection(&converter->cs);

    if (SUCCEEDED(hr))
        *type = ret;
    else
        IMFMediaType_Release(ret);

    return hr;
}

static HRESULT WINAPI audio_converter_GetInputStatus(IMFTransform *iface, DWORD id, DWORD *flags)
{
    FIXME("%p, %u, %p.\n", iface, id, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_converter_GetOutputStatus(IMFTransform *iface, DWORD *flags)
{
    FIXME("%p, %p.\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_converter_SetOutputBounds(IMFTransform *iface, LONGLONG lower, LONGLONG upper)
{
    FIXME("%p, %s, %s.\n", iface, wine_dbgstr_longlong(lower), wine_dbgstr_longlong(upper));

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_converter_ProcessEvent(IMFTransform *iface, DWORD id, IMFMediaEvent *event)
{
    TRACE("%p, %u, %p.\n", iface, id, event);

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_converter_ProcessMessage(IMFTransform *iface, MFT_MESSAGE_TYPE message, ULONG_PTR param)
{
    struct audio_converter *converter = impl_audio_converter_from_IMFTransform(iface);
    struct wg_parser_buffer wg_buffer;

    TRACE("%p, %u %lu.\n", iface, message, param);

    switch(message)
    {
        case MFT_MESSAGE_COMMAND_FLUSH:
        {
            EnterCriticalSection(&converter->cs);
            if (!converter->buffer_inflight)
            {
                LeaveCriticalSection(&converter->cs);
                return S_OK;
            }

            wg_parser_stream_get_buffer(converter->stream, &wg_buffer);
            wg_parser_stream_release_buffer(converter->stream);
            converter->buffer_inflight = FALSE;

            LeaveCriticalSection(&converter->cs);
            return S_OK;
        }
        case MFT_MESSAGE_NOTIFY_BEGIN_STREAMING:
            return S_OK;
        default:
            FIXME("Unhandled message type %x.\n", message);
            return E_NOTIMPL;
    }
}

static HRESULT WINAPI audio_converter_ProcessInput(IMFTransform *iface, DWORD id, IMFSample *sample, DWORD flags)
{
    struct audio_converter *converter = impl_audio_converter_from_IMFTransform(iface);
    IMFMediaBuffer *buffer = NULL;
    unsigned char *buffer_data;
    DWORD buffer_size;
    uint64_t offset;
    uint32_t size;
    HRESULT hr;

    TRACE("%p, %u, %p, %#x.\n", iface, id, sample, flags);

    if (flags)
        WARN("Unsupported flags %#x.\n", flags);

    if (id != 0)
        return MF_E_INVALIDSTREAMNUMBER;

    EnterCriticalSection(&converter->cs);

    if (!converter->stream)
    {
        hr = MF_E_TRANSFORM_TYPE_NOT_SET;
        goto done;
    }

    if (converter->buffer_inflight)
    {
        hr = MF_E_NOTACCEPTING;
        goto done;
    }

    if (FAILED(hr = IMFSample_ConvertToContiguousBuffer(sample, &buffer)))
        goto done;

    if (FAILED(hr = IMFMediaBuffer_Lock(buffer, &buffer_data, NULL, &buffer_size)))
        goto done;

    if (!wg_parser_get_next_read_offset(converter->parser, &offset, &size))
    {
        hr = MF_E_UNEXPECTED;
        IMFMediaBuffer_Unlock(buffer);
        goto done;
    }

    wg_parser_push_data(converter->parser, WG_READ_SUCCESS, buffer_data, buffer_size);

    IMFMediaBuffer_Unlock(buffer);
    converter->buffer_inflight = TRUE;
    if (FAILED(IMFSample_GetSampleTime(sample, &converter->buffer_pts)))
        converter->buffer_pts = -1;
    if (FAILED(IMFSample_GetSampleDuration(sample, &converter->buffer_dur)))
        converter->buffer_dur = -1;

done:
    if (buffer)
        IMFMediaBuffer_Release(buffer);
    LeaveCriticalSection(&converter->cs);
    return hr;
}

static HRESULT WINAPI audio_converter_ProcessOutput(IMFTransform *iface, DWORD flags, DWORD count,
        MFT_OUTPUT_DATA_BUFFER *samples, DWORD *status)
{
    struct audio_converter *converter = impl_audio_converter_from_IMFTransform(iface);
    IMFSample *allocated_sample = NULL;
    struct wg_parser_buffer wg_buffer;
    IMFMediaBuffer *buffer = NULL;
    unsigned char *buffer_data;
    DWORD buffer_len;
    HRESULT hr = S_OK;

    TRACE("%p, %#x, %u, %p, %p.\n", iface, flags, count, samples, status);

    if (flags)
        WARN("Unsupported flags %#x.\n", flags);

    if (!count)
        return S_OK;

    if (count != 1)
        return MF_E_INVALIDSTREAMNUMBER;

    if (samples[0].dwStreamID != 0)
        return MF_E_INVALIDSTREAMNUMBER;

    EnterCriticalSection(&converter->cs);

    if (!converter->stream)
    {
        hr = MF_E_TRANSFORM_TYPE_NOT_SET;
        goto done;
    }

    if (!converter->buffer_inflight)
    {
        hr = MF_E_TRANSFORM_NEED_MORE_INPUT;
        goto done;
    }

    if (!wg_parser_stream_get_buffer(converter->stream, &wg_buffer))
        assert(0);

    if (!samples[0].pSample)
    {
        if (FAILED(hr = MFCreateMemoryBuffer(wg_buffer.size, &buffer)))
        {
            ERR("Failed to create buffer, hr %#x.\n", hr);
            goto done;
        }

        if (FAILED(hr = MFCreateSample(&allocated_sample)))
        {
            ERR("Failed to create sample, hr %#x.\n", hr);
            goto done;
        }

        samples[0].pSample = allocated_sample;

        if (FAILED(hr = IMFSample_AddBuffer(samples[0].pSample, buffer)))
        {
            ERR("Failed to add buffer, hr %#x.\n", hr);
            goto done;
        }

        IMFMediaBuffer_Release(buffer);
        buffer = NULL;
    }

    if (FAILED(hr = IMFSample_ConvertToContiguousBuffer(samples[0].pSample, &buffer)))
    {
        ERR("Failed to get buffer from sample, hr %#x.\n", hr);
        goto done;
    }

    if (FAILED(hr = IMFMediaBuffer_GetMaxLength(buffer, &buffer_len)))
    {
        ERR("Failed to get buffer size, hr %#x.\n", hr);
        goto done;
    }

    if (buffer_len < wg_buffer.size)
    {
        WARN("Client's buffer is smaller (%u bytes) than the output sample (%u bytes)\n",
            buffer_len, wg_buffer.size);

        hr = MF_E_BUFFERTOOSMALL;
        goto done;
    }

    if (FAILED(hr = IMFMediaBuffer_SetCurrentLength(buffer, wg_buffer.size)))
    {
        ERR("Failed to set size, hr %#x.\n", hr);
        goto done;
    }

    if (FAILED(hr = IMFMediaBuffer_Lock(buffer, &buffer_data, NULL, NULL)))
    {
        ERR("Failed to lock buffer hr %#x.\n", hr);
        goto done;
    }

    if (!wg_parser_stream_copy_buffer(converter->stream, buffer_data, 0, wg_buffer.size))
    {
        ERR("Failed to copy buffer.\n");
        IMFMediaBuffer_Unlock(buffer);
        hr = E_FAIL;
        goto done;
    }

    IMFMediaBuffer_Unlock(buffer);

    wg_parser_stream_release_buffer(converter->stream);
    converter->buffer_inflight = FALSE;

    if (converter->buffer_pts != -1)
        IMFSample_SetSampleTime(samples[0].pSample, converter->buffer_pts);
    if (converter->buffer_dur != -1)
        IMFSample_SetSampleDuration(samples[0].pSample, converter->buffer_dur);

    samples[0].dwStatus = 0;
    samples[0].pEvents = NULL;

    done:
    if (buffer)
        IMFMediaBuffer_Release(buffer);
    if (allocated_sample && FAILED(hr))
    {
        IMFSample_Release(allocated_sample);
        samples[0].pSample = NULL;
    }
    LeaveCriticalSection(&converter->cs);
    return hr;
}

static const IMFTransformVtbl audio_converter_vtbl =
{
    audio_converter_QueryInterface,
    audio_converter_AddRef,
    audio_converter_Release,
    audio_converter_GetStreamLimits,
    audio_converter_GetStreamCount,
    audio_converter_GetStreamIDs,
    audio_converter_GetInputStreamInfo,
    audio_converter_GetOutputStreamInfo,
    audio_converter_GetAttributes,
    audio_converter_GetInputStreamAttributes,
    audio_converter_GetOutputStreamAttributes,
    audio_converter_DeleteInputStream,
    audio_converter_AddInputStreams,
    audio_converter_GetInputAvailableType,
    audio_converter_GetOutputAvailableType,
    audio_converter_SetInputType,
    audio_converter_SetOutputType,
    audio_converter_GetInputCurrentType,
    audio_converter_GetOutputCurrentType,
    audio_converter_GetInputStatus,
    audio_converter_GetOutputStatus,
    audio_converter_SetOutputBounds,
    audio_converter_ProcessEvent,
    audio_converter_ProcessMessage,
    audio_converter_ProcessInput,
    audio_converter_ProcessOutput,
};

HRESULT audio_converter_create(REFIID riid, void **ret)
{
    struct audio_converter *object;
    HRESULT hr;

    TRACE("%s %p\n", debugstr_guid(riid), ret);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFTransform_iface.lpVtbl = &audio_converter_vtbl;
    object->refcount = 1;

    InitializeCriticalSection(&object->cs);
    object->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": audio_converter_lock");

    if (FAILED(hr = MFCreateAttributes(&object->attributes, 0)))
    {
        IMFTransform_Release(&object->IMFTransform_iface);
        return hr;
    }

    if (FAILED(hr = MFCreateAttributes(&object->output_attributes, 0)))
    {
        IMFTransform_Release(&object->IMFTransform_iface);
        return hr;
    }

    if (!(object->parser = wg_parser_create(WG_PARSER_AUDIOCONV, true)))
    {
        ERR("Failed to create audio converter due to GStreamer error.\n");
        IMFTransform_Release(&object->IMFTransform_iface);
        return E_OUTOFMEMORY;
    }

    *ret = &object->IMFTransform_iface;
    return S_OK;
}
