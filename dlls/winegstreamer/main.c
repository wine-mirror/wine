/* GStreamer Base Functions
 *
 * Copyright 2002 Lionel Ulmer
 * Copyright 2010 Aric Stewart, CodeWeavers
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

#define EXTERN_GUID DEFINE_GUID

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "initguid.h"
#include "gst_private.h"
#include "winternl.h"
#include "rpcproxy.h"
#include "dmoreg.h"
#include "gst_guids.h"
#include "wmcodecdsp.h"
#include "mferror.h"
#include "mfapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);
WINE_DECLARE_DEBUG_CHANNEL(mfplat);
WINE_DECLARE_DEBUG_CHANNEL(wmvcore);

DEFINE_GUID(GUID_NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
DEFINE_GUID(MEDIASUBTYPE_VC1S,MAKEFOURCC('V','C','1','S'),0x0000,0x0010,0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71);

bool array_reserve(void **elements, size_t *capacity, size_t count, size_t size)
{
    unsigned int new_capacity, max_capacity;
    void *new_elements;

    if (count <= *capacity)
        return TRUE;

    max_capacity = ~(SIZE_T)0 / size;
    if (count > max_capacity)
        return FALSE;

    new_capacity = max(4, *capacity);
    while (new_capacity < count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;
    if (new_capacity < count)
        new_capacity = max_capacity;

    if (!(new_elements = realloc(*elements, new_capacity * size)))
        return FALSE;

    *elements = new_elements;
    *capacity = new_capacity;

    return TRUE;
}

static HRESULT video_format_from_media_type(IMFMediaType *media_type, MFVIDEOFORMAT **format, UINT32 *format_size)
{
    GUID subtype;
    HRESULT hr;

    if (FAILED(hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &subtype)))
        return hr;
    if (FAILED(hr = MFCreateMFVideoFormatFromMFMediaType(media_type, format, format_size)))
        return hr;

    /* fixup MPEG video formats here, so we can consistently use MFVIDEOFORMAT internally */
    if (IsEqualGUID(&subtype, &MEDIASUBTYPE_MPEG1Payload)
            || IsEqualGUID(&subtype, &MEDIASUBTYPE_MPEG1Packet)
            || IsEqualGUID(&subtype, &MEDIASUBTYPE_MPEG2_VIDEO))
    {
        struct mpeg_video_format *mpeg;
        UINT32 mpeg_size, len;

        if (FAILED(IMFMediaType_GetBlobSize(media_type, &MF_MT_MPEG_SEQUENCE_HEADER, &len)))
            len = 0;
        mpeg_size = offsetof(struct mpeg_video_format, sequence_header[len]);

        if ((mpeg = CoTaskMemAlloc(mpeg_size)))
        {
            memset(mpeg, 0, mpeg_size);
            mpeg->hdr = **format;

            IMFMediaType_GetBlob(media_type, &MF_MT_MPEG_SEQUENCE_HEADER, mpeg->sequence_header, len, NULL);
            IMFMediaType_GetUINT32(media_type, &MF_MT_MPEG_START_TIME_CODE, (UINT32 *)&mpeg->start_time_code);
            IMFMediaType_GetUINT32(media_type, &MF_MT_MPEG2_PROFILE, &mpeg->profile);
            IMFMediaType_GetUINT32(media_type, &MF_MT_MPEG2_LEVEL, &mpeg->level);
            IMFMediaType_GetUINT32(media_type, &MF_MT_MPEG2_FLAGS, &mpeg->flags);

            CoTaskMemFree(*format);
            *format = &mpeg->hdr;
            *format_size = mpeg_size;
        }
    }

    return hr;
}

static HRESULT wg_media_type_from_mf(IMFMediaType *media_type, struct wg_media_type *wg_media_type)
{
    HRESULT hr;

    if (FAILED(hr = IMFMediaType_GetMajorType(media_type, &wg_media_type->major)))
        return hr;

    if (IsEqualGUID(&wg_media_type->major, &MFMediaType_Video))
        return video_format_from_media_type(media_type, &wg_media_type->u.video,
                &wg_media_type->format_size);
    if (IsEqualGUID(&wg_media_type->major, &MFMediaType_Audio))
        return MFCreateWaveFormatExFromMFMediaType(media_type, &wg_media_type->u.audio,
                &wg_media_type->format_size, 0);

    FIXME("Unsupported major type %s\n", debugstr_guid(&wg_media_type->major));
    return E_NOTIMPL;
}

static HRESULT media_type_from_video_format(const MFVIDEOFORMAT *format, IMFMediaType **media_type)
{
    HRESULT hr;

    if (FAILED(hr = MFCreateVideoMediaType(format, (IMFVideoMediaType **)media_type)) || format->dwSize <= sizeof(*format))
        return hr;

    /* fixup MPEG video formats here, so we can consistently use MFVIDEOFORMAT internally */
    if (IsEqualGUID(&format->guidFormat, &MEDIASUBTYPE_MPEG1Payload)
            || IsEqualGUID(&format->guidFormat, &MEDIASUBTYPE_MPEG1Packet)
            || IsEqualGUID(&format->guidFormat, &MEDIASUBTYPE_MPEG2_VIDEO))
    {
        struct mpeg_video_format *mpeg = (struct mpeg_video_format *)format;
        IMFMediaType_SetBlob(*media_type, &MF_MT_MPEG_SEQUENCE_HEADER, mpeg->sequence_header, mpeg->sequence_header_count);
        IMFMediaType_SetUINT32(*media_type, &MF_MT_MPEG_START_TIME_CODE, mpeg->start_time_code);
        IMFMediaType_SetUINT32(*media_type, &MF_MT_MPEG2_PROFILE, mpeg->profile);
        IMFMediaType_SetUINT32(*media_type, &MF_MT_MPEG2_LEVEL, mpeg->level);
        IMFMediaType_SetUINT32(*media_type, &MF_MT_MPEG2_FLAGS, mpeg->flags);
        IMFMediaType_DeleteItem(*media_type, &MF_MT_USER_DATA);
    }

    return hr;
}

static HRESULT wg_media_type_to_mf(const struct wg_media_type *wg_media_type, IMFMediaType **media_type)
{
    if (IsEqualGUID(&wg_media_type->major, &MFMediaType_Video))
        return media_type_from_video_format(wg_media_type->u.video, media_type);
    if (IsEqualGUID(&wg_media_type->major, &MFMediaType_Audio))
        return MFCreateAudioMediaType(wg_media_type->u.audio, (IMFAudioMediaType **)media_type);

    FIXME("Unsupported major type %s\n", debugstr_guid(&wg_media_type->major));
    return E_NOTIMPL;
}

wg_parser_t wg_parser_create(bool output_compressed)
{
    struct wg_parser_create_params params =
    {
        .output_compressed = output_compressed,
        .err_on = ERR_ON(quartz),
        .warn_on = WARN_ON(quartz),
    };

    TRACE("output_compressed %d.\n", output_compressed);

    if (WINE_UNIX_CALL(unix_wg_parser_create, &params))
        return 0;

    TRACE("Returning parser %#I64x.\n", params.parser);

    return params.parser;
}

void wg_parser_destroy(wg_parser_t parser)
{
    TRACE("parser %#I64x.\n", parser);

    WINE_UNIX_CALL(unix_wg_parser_destroy, &parser);
}

HRESULT wg_parser_connect(wg_parser_t parser, uint64_t file_size, const WCHAR *uri)
{
    struct wg_parser_connect_params params =
    {
        .parser = parser,
        .file_size = file_size,
        .uri = uri,
    };

    TRACE("parser %#I64x, file_size %I64u.\n", parser, file_size);

    return WINE_UNIX_CALL(unix_wg_parser_connect, &params);
}

void wg_parser_disconnect(wg_parser_t parser)
{
    TRACE("parser %#I64x.\n", parser);

    WINE_UNIX_CALL(unix_wg_parser_disconnect, &parser);
}

bool wg_parser_get_next_read_offset(wg_parser_t parser, uint64_t *offset, uint32_t *size)
{
    struct wg_parser_get_next_read_offset_params params =
    {
        .parser = parser,
    };

    TRACE("parser %#I64x, offset %p, size %p.\n", parser, offset, size);

    if (WINE_UNIX_CALL(unix_wg_parser_get_next_read_offset, &params))
        return false;
    *offset = params.offset;
    *size = params.size;
    return true;
}

void wg_parser_push_data(wg_parser_t parser, const void *data, uint32_t size)
{
    struct wg_parser_push_data_params params =
    {
        .parser = parser,
        .data = data,
        .size = size,
    };

    TRACE("parser %#I64x, data %p, size %u.\n", parser, data, size);

    WINE_UNIX_CALL(unix_wg_parser_push_data, &params);
}

uint32_t wg_parser_get_stream_count(wg_parser_t parser)
{
    struct wg_parser_get_stream_count_params params =
    {
        .parser = parser,
    };

    TRACE("parser %#I64x.\n", parser);

    WINE_UNIX_CALL(unix_wg_parser_get_stream_count, &params);
    return params.count;
}

wg_parser_stream_t wg_parser_get_stream(wg_parser_t parser, uint32_t index)
{
    struct wg_parser_get_stream_params params =
    {
        .parser = parser,
        .index = index,
    };

    TRACE("parser %#I64x, index %u.\n", parser, index);

    WINE_UNIX_CALL(unix_wg_parser_get_stream, &params);

    TRACE("Returning stream %#I64x.\n", params.stream);
    return params.stream;
}

void wg_parser_stream_get_current_format(wg_parser_stream_t stream, struct wg_format *format)
{
    struct wg_parser_stream_get_current_format_params params =
    {
        .stream = stream,
        .format = format,
    };

    TRACE("stream %#I64x, format %p.\n", stream, format);

    WINE_UNIX_CALL(unix_wg_parser_stream_get_current_format, &params);
}

void wg_parser_stream_get_codec_format(wg_parser_stream_t stream, struct wg_format *format)
{
    struct wg_parser_stream_get_codec_format_params params =
    {
        .stream = stream,
        .format = format,
    };

    TRACE("stream %#I64x, format %p.\n", stream, format);

    WINE_UNIX_CALL(unix_wg_parser_stream_get_codec_format, &params);
}

void wg_parser_stream_enable(wg_parser_stream_t stream, const struct wg_format *format)
{
    struct wg_parser_stream_enable_params params =
    {
        .stream = stream,
        .format = format,
    };

    TRACE("stream %#I64x, format %p.\n", stream, format);

    WINE_UNIX_CALL(unix_wg_parser_stream_enable, &params);
}

void wg_parser_stream_disable(wg_parser_stream_t stream)
{
    TRACE("stream %#I64x.\n", stream);

    WINE_UNIX_CALL(unix_wg_parser_stream_disable, &stream);
}

bool wg_parser_stream_get_buffer(wg_parser_t parser, wg_parser_stream_t stream,
        struct wg_parser_buffer *buffer)
{
    struct wg_parser_stream_get_buffer_params params =
    {
        .parser = parser,
        .stream = stream,
        .buffer = buffer,
    };

    TRACE("parser %#I64x, stream %#I64x, buffer %p.\n", parser, stream, buffer);

    return !WINE_UNIX_CALL(unix_wg_parser_stream_get_buffer, &params);
}

bool wg_parser_stream_copy_buffer(wg_parser_stream_t stream,
        void *data, uint32_t offset, uint32_t size)
{
    struct wg_parser_stream_copy_buffer_params params =
    {
        .stream = stream,
        .data = data,
        .offset = offset,
        .size = size,
    };

    TRACE("stream %#I64x, data %p, offset %u, size %u.\n", stream, data, offset, size);

    return !WINE_UNIX_CALL(unix_wg_parser_stream_copy_buffer, &params);
}

void wg_parser_stream_release_buffer(wg_parser_stream_t stream)
{
    TRACE("stream %#I64x.\n", stream);

    WINE_UNIX_CALL(unix_wg_parser_stream_release_buffer, &stream);
}

void wg_parser_stream_notify_qos(wg_parser_stream_t stream,
        bool underflow, double proportion, int64_t diff, uint64_t timestamp)
{
    struct wg_parser_stream_notify_qos_params params =
    {
        .stream = stream,
        .underflow = underflow,
        .proportion = proportion,
        .diff = diff,
        .timestamp = timestamp,
    };

    TRACE("stream %#I64x, underflow %d, proportion %.16e, diff %I64d, timestamp %I64u.\n",
            stream, underflow, proportion, diff, timestamp);

    WINE_UNIX_CALL(unix_wg_parser_stream_notify_qos, &params);
}

uint64_t wg_parser_stream_get_duration(wg_parser_stream_t stream)
{
    struct wg_parser_stream_get_duration_params params =
    {
        .stream = stream,
    };

    TRACE("stream %#I64x.\n", stream);

    WINE_UNIX_CALL(unix_wg_parser_stream_get_duration, &params);

    TRACE("Returning duration %I64u.\n", params.duration);
    return params.duration;
}

char *wg_parser_stream_get_tag(wg_parser_stream_t stream, enum wg_parser_tag tag)
{
    uint32_t size = 0;
    struct wg_parser_stream_get_tag_params params =
    {
        .stream = stream,
        .tag = tag,
        .size = &size,
    };
    char *buffer;

    if (WINE_UNIX_CALL(unix_wg_parser_stream_get_tag, &params) != STATUS_BUFFER_TOO_SMALL)
        return NULL;
    if (!(buffer = malloc(size)))
    {
        ERR("No memory.\n");
        return NULL;
    }
    params.buffer = buffer;
    if (WINE_UNIX_CALL(unix_wg_parser_stream_get_tag, &params))
    {
        ERR("wg_parser_stream_get_tag failed unexpectedly.\n");
        free(buffer);
        return NULL;
    }
    return buffer;
}

void wg_parser_stream_seek(wg_parser_stream_t stream, double rate,
        uint64_t start_pos, uint64_t stop_pos, DWORD start_flags, DWORD stop_flags)
{
    struct wg_parser_stream_seek_params params =
    {
        .stream = stream,
        .rate = rate,
        .start_pos = start_pos,
        .stop_pos = stop_pos,
        .start_flags = start_flags,
        .stop_flags = stop_flags,
    };

    TRACE("stream %#I64x, rate %.16e, start_pos %I64u, stop_pos %I64u, start_flags %#lx, stop_flags %#lx.\n",
            stream, rate, start_pos, stop_pos, start_flags, stop_flags);

    WINE_UNIX_CALL(unix_wg_parser_stream_seek, &params);
}

HRESULT wg_transform_create_mf(IMFMediaType *input_type, IMFMediaType *output_type,
        const struct wg_transform_attrs *attrs, wg_transform_t *transform)
{
    struct wg_transform_create_params params =
    {
        .attrs = *attrs,
    };
    NTSTATUS status;
    HRESULT hr;

    TRACE("input_type %p, output_type %p.\n", input_type, output_type);

    if (FAILED(hr = wg_media_type_from_mf(input_type, &params.input_type)))
        return hr;
    if (FAILED(hr = wg_media_type_from_mf(output_type, &params.output_type)))
    {
        CoTaskMemFree(params.input_type.u.format);
        return hr;
    }

    if ((status = WINE_UNIX_CALL(unix_wg_transform_create, &params)))
    {
        WARN("Failed to create transform, status %#lx\n", status);
        hr = HRESULT_FROM_NT(status);
    }

    CoTaskMemFree(params.output_type.u.format);
    CoTaskMemFree(params.input_type.u.format);
    *transform = params.transform;
    return hr;
}

HRESULT wg_transform_create_quartz(const AM_MEDIA_TYPE *input_format, const AM_MEDIA_TYPE *output_format,
        const struct wg_transform_attrs *attrs, wg_transform_t *transform)
{
    IMFMediaType *input_type, *output_type;
    HRESULT hr;

    TRACE("input_format %p, output_format %p.\n", input_format, output_format);

    /* through IMFMediaType to normalize representation to MFVIDEOFORMAT / WAVEFORMATEX */
    if (FAILED(hr = MFCreateMediaTypeFromRepresentation(AM_MEDIA_TYPE_REPRESENTATION, (void *)input_format, &input_type)))
        return 0;
    if (FAILED(hr = MFCreateMediaTypeFromRepresentation(AM_MEDIA_TYPE_REPRESENTATION, (void *)output_format, &output_type)))
    {
        IMFMediaType_Release(input_type);
        return 0;
    }

    hr = wg_transform_create_mf(input_type, output_type, attrs, transform);
    IMFMediaType_Release(output_type);
    IMFMediaType_Release(input_type);
    return hr;
}

void wg_transform_destroy(wg_transform_t transform)
{
    TRACE("transform %#I64x.\n", transform);

    WINE_UNIX_CALL(unix_wg_transform_destroy, &transform);
}

HRESULT wg_transform_push_data(wg_transform_t transform, struct wg_sample *sample)
{
    struct wg_transform_push_data_params params =
    {
        .transform = transform,
        .sample = sample,
    };
    NTSTATUS status;

    TRACE("transform %#I64x, sample %p.\n", transform, sample);

    if ((status = WINE_UNIX_CALL(unix_wg_transform_push_data, &params)))
        return HRESULT_FROM_NT(status);

    return params.result;
}

HRESULT wg_transform_read_data(wg_transform_t transform, struct wg_sample *sample)
{
    struct wg_transform_read_data_params params =
    {
        .transform = transform,
        .sample = sample,
    };
    NTSTATUS status;

    TRACE("transform %#I64x, sample %p.\n", transform, sample);

    if ((status = WINE_UNIX_CALL(unix_wg_transform_read_data, &params)))
        return HRESULT_FROM_NT(status);

    return params.result;
}

bool wg_transform_get_status(wg_transform_t transform, bool *accepts_input)
{
    struct wg_transform_get_status_params params =
    {
        .transform = transform,
    };

    TRACE("transform %#I64x, accepts_input %p.\n", transform, accepts_input);

    if (WINE_UNIX_CALL(unix_wg_transform_get_status, &params))
        return false;

    *accepts_input = params.accepts_input;
    return true;
}

HRESULT wg_transform_get_output_type(wg_transform_t transform, IMFMediaType **media_type)
{
    struct wg_transform_get_output_type_params params =
    {
        .transform = transform,
    };
    NTSTATUS status;
    HRESULT hr;

    TRACE("transform %#I64x, media_type %p.\n", transform, media_type);

    if ((status = WINE_UNIX_CALL(unix_wg_transform_get_output_type, &params))
            && status == STATUS_BUFFER_TOO_SMALL)
    {
        if (!(params.media_type.u.format = CoTaskMemAlloc(params.media_type.format_size)))
            return ERROR_OUTOFMEMORY;
        status = WINE_UNIX_CALL(unix_wg_transform_get_output_type, &params);
    }

    if (status)
    {
        CoTaskMemFree(params.media_type.u.format);
        WARN("Failed to get output media type, status %#lx\n", status);
        return HRESULT_FROM_NT(status);
    }

    hr = wg_media_type_to_mf(&params.media_type, media_type);
    CoTaskMemFree(params.media_type.u.format);
    return hr;
}

HRESULT wg_transform_set_output_type(wg_transform_t transform, IMFMediaType *media_type)
{
    struct wg_transform_set_output_type_params params =
    {
        .transform = transform,
    };
    NTSTATUS status;
    HRESULT hr;

    TRACE("transform %#I64x, media_type %p.\n", transform, media_type);

    if (FAILED(hr = wg_media_type_from_mf(media_type, &params.media_type)))
    {
        WARN("Failed to initialize media type, hr %#lx\n", hr);
        return hr;
    }
    if ((status = WINE_UNIX_CALL(unix_wg_transform_set_output_type, &params)))
    {
       WARN("Failed to set transform output type, status %#lx\n", status);
       hr = HRESULT_FROM_NT(status);
    }

    CoTaskMemFree(params.media_type.u.format);
    return hr;
}

HRESULT wg_transform_drain(wg_transform_t transform)
{
    NTSTATUS status;

    TRACE("transform %#I64x.\n", transform);

    if ((status = WINE_UNIX_CALL(unix_wg_transform_drain, &transform)))
    {
        WARN("wg_transform_drain returned status %#lx\n", status);
        return HRESULT_FROM_NT(status);
    }

    return S_OK;
}

HRESULT wg_transform_flush(wg_transform_t transform)
{
    NTSTATUS status;

    TRACE("transform %#I64x.\n", transform);

    if ((status = WINE_UNIX_CALL(unix_wg_transform_flush, &transform)))
    {
        WARN("wg_transform_flush returned status %#lx\n", status);
        return HRESULT_FROM_NT(status);
    }

    return S_OK;
}

void wg_transform_notify_qos(wg_transform_t transform,
        bool underflow, double proportion, int64_t diff, uint64_t timestamp)
{
    struct wg_transform_notify_qos_params params =
    {
        .transform = transform,
        .underflow = underflow,
        .proportion = proportion,
        .diff = diff,
        .timestamp = timestamp,
    };

    TRACE("transform %#I64x, underflow %d, proportion %.16e, diff %I64d, timestamp %I64u.\n",
            transform, underflow, proportion, diff, timestamp);

    WINE_UNIX_CALL(unix_wg_transform_notify_qos, &params);
}

HRESULT wg_muxer_create(const char *format, wg_muxer_t *muxer)
{
    struct wg_muxer_create_params params =
    {
        .format = format,
    };
    NTSTATUS status;

    TRACE("format %p, muxer %p.\n", format, muxer);

    if (!(status = WINE_UNIX_CALL(unix_wg_muxer_create, &params)))
    {
        *muxer = params.muxer;
        TRACE("Created wg_muxer %#I64x.\n", params.muxer);
    }
    else
    {
        WARN("Failed to create muxer, status %#lx.\n", status);
    }

    return HRESULT_FROM_NT(status);
}

void wg_muxer_destroy(wg_muxer_t muxer)
{
    TRACE("muxer %#I64x.\n", muxer);

    WINE_UNIX_CALL(unix_wg_muxer_destroy, &muxer);
}

HRESULT wg_muxer_add_stream(wg_muxer_t muxer, UINT32 stream_id, const struct wg_format *format)
{
    struct wg_muxer_add_stream_params params =
    {
        .muxer = muxer,
        .stream_id = stream_id,
        .format = format,
    };
    NTSTATUS status;

    TRACE("muxer %#I64x, stream_id %u, format %p.\n", muxer, stream_id, format);

    if ((status = WINE_UNIX_CALL(unix_wg_muxer_add_stream, &params)))
    {
        WARN("Failed to add stream, status %#lx.\n", status);
        return HRESULT_FROM_NT(status);
    }

    return S_OK;
}

HRESULT wg_muxer_start(wg_muxer_t muxer)
{
    NTSTATUS status;

    TRACE("muxer %#I64x.\n",  muxer);

    if ((status = WINE_UNIX_CALL(unix_wg_muxer_start, &muxer)))
    {
        WARN("Failed to start muxer, status %#lx.\n", status);
        return HRESULT_FROM_NT(status);
    }

    return S_OK;
}

HRESULT wg_muxer_push_sample(wg_muxer_t muxer, struct wg_sample *sample, UINT32 steam_id)
{
    struct wg_muxer_push_sample_params params =
    {
        .muxer = muxer,
        .sample = sample,
        .stream_id = steam_id,
    };
    NTSTATUS status;

    TRACE("muxer %#I64x, sample %p.\n", muxer, sample);

    if ((status = WINE_UNIX_CALL(unix_wg_muxer_push_sample, &params)))
    {
        WARN("Failed to push sample, status %#lx.\n", status);
        return HRESULT_FROM_NT(status);
    }

    return S_OK;
}

HRESULT wg_muxer_read_data(wg_muxer_t muxer, void *buffer, UINT32 *size, UINT64 *offset)
{
    struct wg_muxer_read_data_params params =
    {
        .muxer = muxer,
        .buffer = buffer,
        .size = *size,
        .offset = UINT64_MAX,
    };
    NTSTATUS status;

    TRACE("muxer %#I64x, buffer %p, size %u.\n", muxer, buffer, *size);

    if (SUCCEEDED(status = WINE_UNIX_CALL(unix_wg_muxer_read_data, &params)))
    {
        *size = params.size;
        *offset = params.offset;
        TRACE("Read %u bytes, offset %#I64x.\n", *size, *offset);
    }

    return HRESULT_FROM_NT(status);
}

HRESULT wg_muxer_finalize(wg_muxer_t muxer)
{
    NTSTATUS status;

    TRACE("muxer %#I64x.\n", muxer);

    if ((status = WINE_UNIX_CALL(unix_wg_muxer_finalize, &muxer)))
    {
        WARN("Failed to finalize, status %#lx.\n", status);
        return HRESULT_FROM_NT(status);
    }

    return S_OK;
}

static HRESULT check_transform_support(const struct wg_media_type *input, const struct wg_media_type *output)
{
    IMFMediaType *input_type, *output_type;
    struct wg_transform_attrs attrs = {0};
    wg_transform_t transform;
    HRESULT hr;

    if (FAILED(hr = wg_media_type_to_mf(input, &input_type)))
        return hr;
    if (FAILED(hr = wg_media_type_to_mf(output, &output_type)))
    {
        IMFMediaType_Release(input_type);
        return hr;
    }

    if (SUCCEEDED(hr = wg_transform_create_mf(input_type, output_type, &attrs, &transform)))
        wg_transform_destroy(transform);

    IMFMediaType_Release(output_type);
    IMFMediaType_Release(input_type);
    return hr;
}

HRESULT check_audio_transform_support(const WAVEFORMATEX *input, const WAVEFORMATEX *output)
{
    const struct wg_media_type input_type =
    {
        .major = MFMediaType_Audio,
        .format_size = sizeof(*input) + input->cbSize,
        .u.audio = (WAVEFORMATEX *)input,
    };
    const struct wg_media_type output_type =
    {
        .major = MFMediaType_Audio,
        .format_size = sizeof(*output) + output->cbSize,
        .u.audio = (WAVEFORMATEX *)output,
    };
    return check_transform_support(&input_type, &output_type);
}

HRESULT check_video_transform_support(const MFVIDEOFORMAT *input, const MFVIDEOFORMAT *output)
{
    const struct wg_media_type input_type =
    {
        .major = MFMediaType_Video,
        .format_size = input->dwSize,
        .u.video = (MFVIDEOFORMAT *)input,
    };
    const struct wg_media_type output_type =
    {
        .major = MFMediaType_Video,
        .format_size = output->dwSize,
        .u.video = (MFVIDEOFORMAT *)output,
    };
    return check_transform_support(&input_type, &output_type);
}

#define ALIGN(n, alignment) (((n) + (alignment) - 1) & ~((alignment) - 1))

unsigned int wg_format_get_stride(const struct wg_format *format)
{
    const unsigned int width = format->u.video.width;

    switch (format->u.video.format)
    {
        case WG_VIDEO_FORMAT_AYUV:
            return width * 4;

        case WG_VIDEO_FORMAT_BGRA:
        case WG_VIDEO_FORMAT_BGRx:
        case WG_VIDEO_FORMAT_RGBA:
            return width * 4;

        case WG_VIDEO_FORMAT_BGR:
            return ALIGN(width * 3, 4);

        case WG_VIDEO_FORMAT_UYVY:
        case WG_VIDEO_FORMAT_YUY2:
        case WG_VIDEO_FORMAT_YVYU:
            return ALIGN(width * 2, 4);

        case WG_VIDEO_FORMAT_RGB15:
        case WG_VIDEO_FORMAT_RGB16:
            return ALIGN(width * 2, 4);

        case WG_VIDEO_FORMAT_I420:
        case WG_VIDEO_FORMAT_NV12:
        case WG_VIDEO_FORMAT_YV12:
            return ALIGN(width, 4); /* Y plane */

        case WG_VIDEO_FORMAT_UNKNOWN:
            FIXME("Cannot calculate stride for unknown video format.\n");
    }

    return 0;
}

bool wg_video_format_is_rgb(enum wg_video_format format)
{
    switch (format)
    {
        case WG_VIDEO_FORMAT_BGRA:
        case WG_VIDEO_FORMAT_BGRx:
        case WG_VIDEO_FORMAT_BGR:
        case WG_VIDEO_FORMAT_RGB15:
        case WG_VIDEO_FORMAT_RGB16:
        case WG_VIDEO_FORMAT_RGBA:
            return true;

        default:
            break;
    }

    return false;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(instance);
        __wine_init_unix_call();
    }
    return TRUE;
}

struct class_factory
{
    IClassFactory IClassFactory_iface;
    HRESULT (*create_instance)(IUnknown *outer, IUnknown **out);
};

static inline struct class_factory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, struct class_factory, IClassFactory_iface);
}

static HRESULT WINAPI class_factory_QueryInterface(IClassFactory *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IClassFactory))
    {
        *out = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    *out = NULL;
    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI class_factory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI class_factory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI class_factory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID iid, void **out)
{
    struct class_factory *factory = impl_from_IClassFactory(iface);
    IUnknown *unk;
    HRESULT hr;

    TRACE("iface %p, outer %p, iid %s, out %p.\n", iface, outer, debugstr_guid(iid), out);

    if (outer && !IsEqualGUID(iid, &IID_IUnknown))
        return E_NOINTERFACE;

    *out = NULL;
    if (SUCCEEDED(hr = factory->create_instance(outer, &unk)))
    {
        hr = IUnknown_QueryInterface(unk, iid, out);
        IUnknown_Release(unk);
    }
    return hr;
}

static HRESULT WINAPI class_factory_LockServer(IClassFactory *iface, BOOL lock)
{
    TRACE("iface %p, lock %d.\n", iface, lock);
    return S_OK;
}

static const IClassFactoryVtbl class_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    class_factory_CreateInstance,
    class_factory_LockServer,
};

static struct class_factory avi_splitter_cf = {{&class_factory_vtbl}, avi_splitter_create};
static struct class_factory decodebin_parser_cf = {{&class_factory_vtbl}, decodebin_parser_create};
static struct class_factory mpeg_audio_codec_cf = {{&class_factory_vtbl}, mpeg_audio_codec_create};
static struct class_factory mpeg_video_codec_cf = {{&class_factory_vtbl}, mpeg_video_codec_create};
static struct class_factory mpeg_layer3_decoder_cf = {{&class_factory_vtbl}, mpeg_layer3_decoder_create};
static struct class_factory mpeg_splitter_cf = {{&class_factory_vtbl}, mpeg_splitter_create};
static struct class_factory wave_parser_cf = {{&class_factory_vtbl}, wave_parser_create};
static struct class_factory wma_decoder_cf = {{&class_factory_vtbl}, wma_decoder_create};
static struct class_factory wmv_decoder_cf = {{&class_factory_vtbl}, wmv_decoder_create};
static struct class_factory resampler_cf = {{&class_factory_vtbl}, resampler_create};
static struct class_factory color_convert_cf = {{&class_factory_vtbl}, color_convert_create};
static struct class_factory mp3_sink_class_factory_cf = {{&class_factory_vtbl}, mp3_sink_class_factory_create};
static struct class_factory mpeg4_sink_class_factory_cf = {{&class_factory_vtbl}, mpeg4_sink_class_factory_create};

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void **out)
{
    static const GUID CLSID_wg_avi_splitter = {0x272bfbfb,0x50d0,0x4078,{0xb6,0x00,0x1e,0x95,0x9c,0x30,0x13,0x37}};
    static const GUID CLSID_wg_color_converter = {0xf47e2da5,0xe370,0x47b7,{0x90,0x3a,0x07,0x8d,0xdd,0x45,0xa5,0xcc}};
    static const GUID CLSID_wg_mp3_sink_factory = {0x1f302877,0xaaab,0x40a3,{0xb9,0xe0,0x9f,0x48,0xda,0xf3,0x5b,0xc8}};
    static const GUID CLSID_wg_mpeg4_sink_factory = {0x5d5407d9,0xc6ca,0x4770,{0xa7,0xcc,0x27,0xc0,0xcb,0x8a,0x76,0x27}};
    static const GUID CLSID_wg_mpeg_audio_decoder = {0xc9f285f8,0x4380,0x4121,{0x97,0x1f,0x49,0xa9,0x53,0x16,0xc2,0x7b}};
    static const GUID CLSID_wg_mpeg_video_decoder = {0x5ed2e5f6,0xbf3e,0x4180,{0x83,0xa4,0x48,0x47,0xcc,0x5b,0x4e,0xa3}};
    static const GUID CLSID_wg_resampler = {0x92f35e78,0x15a5,0x486b,{0x88,0x8e,0x57,0x5f,0x99,0x65,0x1c,0xe2}};
    static const GUID CLSID_wg_wma_decoder = {0x5b4d4e54,0x0620,0x4cf9,{0x94,0xae,0x78,0x23,0x96,0x5c,0x28,0xb6}};
    static const GUID CLSID_wg_wmv_decoder = {0x62ee5ddb,0x4f52,0x48e2,{0x89,0x28,0x78,0x7b,0x02,0x53,0xa0,0xbc}};
    static const GUID CLSID_wg_mp3_decoder = {0x84cd8e3e,0xb221,0x434a,{0x88,0x82,0x9d,0x6c,0x8d,0xf4,0x90,0xe1}};
    static const GUID CLSID_wg_mpeg1_splitter = {0xa8edbf98,0x2442,0x42c5,{0x85,0xa1,0xab,0x05,0xa5,0x80,0xdf,0x53}};
    static const GUID CLSID_wg_wave_parser = {0x3f839ec7,0x5ea6,0x49e1,{0x80,0xc2,0x1e,0xa3,0x00,0xf8,0xb0,0xe0}};
    struct class_factory *factory;
    HRESULT hr;

    TRACE("clsid %s, iid %s, out %p.\n", debugstr_guid(clsid), debugstr_guid(iid), out);

    if (!init_gstreamer())
        return CLASS_E_CLASSNOTAVAILABLE;

    if (SUCCEEDED(hr = mfplat_get_class_object(clsid, iid, out)))
        return hr;

    if (IsEqualGUID(clsid, &CLSID_wg_avi_splitter))
        factory = &avi_splitter_cf;
    else if (IsEqualGUID(clsid, &CLSID_decodebin_parser))
        factory = &decodebin_parser_cf;
    else if (IsEqualGUID(clsid, &CLSID_wg_mpeg_audio_decoder))
        factory = &mpeg_audio_codec_cf;
    else if (IsEqualGUID(clsid, &CLSID_wg_mpeg_video_decoder))
        factory = &mpeg_video_codec_cf;
    else if (IsEqualGUID(clsid, &CLSID_wg_mp3_decoder))
        factory = &mpeg_layer3_decoder_cf;
    else if (IsEqualGUID(clsid, &CLSID_wg_mpeg1_splitter))
        factory = &mpeg_splitter_cf;
    else if (IsEqualGUID(clsid, &CLSID_wg_wave_parser))
        factory = &wave_parser_cf;
    else if (IsEqualGUID(clsid, &CLSID_wg_wma_decoder))
        factory = &wma_decoder_cf;
    else if (IsEqualGUID(clsid, &CLSID_wg_wmv_decoder))
        factory = &wmv_decoder_cf;
    else if (IsEqualGUID(clsid, &CLSID_wg_resampler))
        factory = &resampler_cf;
    else if (IsEqualGUID(clsid, &CLSID_wg_color_converter))
        factory = &color_convert_cf;
    else if (IsEqualGUID(clsid, &CLSID_wg_mp3_sink_factory))
        factory = &mp3_sink_class_factory_cf;
    else if (IsEqualGUID(clsid, &CLSID_wg_mpeg4_sink_factory))
        factory = &mpeg4_sink_class_factory_cf;
    else
    {
        FIXME("%s not implemented, returning CLASS_E_CLASSNOTAVAILABLE.\n", debugstr_guid(clsid));
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    return IClassFactory_QueryInterface(&factory->IClassFactory_iface, iid, out);
}

static BOOL CALLBACK init_gstreamer_proc(INIT_ONCE *once, void *param, void **ctx)
{
    struct wg_init_gstreamer_params params =
    {
        .trace_on = TRACE_ON(mfplat) || TRACE_ON(quartz) || TRACE_ON(wmvcore),
        .warn_on = WARN_ON(mfplat) || WARN_ON(quartz) || WARN_ON(wmvcore),
        .err_on = ERR_ON(mfplat) || ERR_ON(quartz) || ERR_ON(wmvcore),
    };
    HINSTANCE handle;

    if (WINE_UNIX_CALL(unix_wg_init_gstreamer, &params))
        return FALSE;

    /* Unloading glib is a bad idea.. it installs atexit handlers,
     * so never unload the dll after loading */
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            (LPCWSTR)init_gstreamer_proc, &handle);
    if (!handle)
        ERR("Failed to pin module.\n");

    return TRUE;
}

BOOL init_gstreamer(void)
{
    static INIT_ONCE once = INIT_ONCE_STATIC_INIT;

    InitOnceExecuteOnce(&once, init_gstreamer_proc, NULL, NULL);

    return TRUE;
}

static const REGPINTYPES reg_audio_mt = {&MEDIATYPE_Audio, &GUID_NULL};
static const REGPINTYPES reg_stream_mt = {&MEDIATYPE_Stream, &GUID_NULL};
static const REGPINTYPES reg_video_mt = {&MEDIATYPE_Video, &GUID_NULL};

static const REGFILTERPINS2 reg_decodebin_parser_pins[3] =
{
    {
        .nMediaTypes = 1,
        .lpMediaType = &reg_stream_mt,
    },
    {
        .dwFlags = REG_PINFLAG_B_OUTPUT,
        .nMediaTypes = 1,
        .lpMediaType = &reg_audio_mt,
    },
    {
        .dwFlags = REG_PINFLAG_B_OUTPUT,
        .nMediaTypes = 1,
        .lpMediaType = &reg_video_mt,
    },
};

static const REGFILTER2 reg_decodebin_parser =
{
    .dwVersion = 2,
    .dwMerit = MERIT_NORMAL - 1,
    .cPins2 = 3,
    .rgPins2 = reg_decodebin_parser_pins,
};

HRESULT WINAPI DllRegisterServer(void)
{
    IFilterMapper2 *mapper;
    HRESULT hr;

    TRACE(".\n");

    if (FAILED(hr = __wine_register_resources()))
        return hr;

    if (FAILED(hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterMapper2, (void **)&mapper)))
        return hr;

    IFilterMapper2_RegisterFilter(mapper, &CLSID_decodebin_parser,
            L"GStreamer splitter filter", NULL, NULL, NULL, &reg_decodebin_parser);

    IFilterMapper2_Release(mapper);

    return S_OK;
}

HRESULT WINAPI DllUnregisterServer(void)
{
    IFilterMapper2 *mapper;
    HRESULT hr;

    TRACE(".\n");

    if (FAILED(hr = __wine_unregister_resources()))
        return hr;

    if (FAILED(hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterMapper2, (void **)&mapper)))
        return hr;

    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &CLSID_decodebin_parser);

    IFilterMapper2_Release(mapper);

    return S_OK;
}
