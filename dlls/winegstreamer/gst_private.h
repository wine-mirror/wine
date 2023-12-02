/*
 * GStreamer splitter + decoder, adapted from parser.c
 *
 * Copyright 2010 Maarten Lankhorst for CodeWeavers
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

#ifndef __GST_PRIVATE_INCLUDED__
#define __GST_PRIVATE_INCLUDED__

#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define COBJMACROS
#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include "dshow.h"
#include "mfidl.h"
#include "wine/debug.h"
#include "wine/strmbase.h"
#include "wine/mfinternal.h"

#include "unixlib.h"

bool array_reserve(void **elements, size_t *capacity, size_t count, size_t size);

static inline const char *debugstr_time(REFERENCE_TIME time)
{
    ULONGLONG abstime = time >= 0 ? time : -time;
    unsigned int i = 0, j = 0;
    char buffer[23], rev[23];

    while (abstime || i <= 8)
    {
        buffer[i++] = '0' + (abstime % 10);
        abstime /= 10;
        if (i == 7) buffer[i++] = '.';
    }
    if (time < 0) buffer[i++] = '-';

    while (i--) rev[j++] = buffer[i];
    while (rev[j-1] == '0' && rev[j-2] != '.') --j;
    rev[j] = 0;

    return wine_dbg_sprintf("%s", rev);
}

#define MEDIATIME_FROM_BYTES(x) ((LONGLONG)(x) * 10000000)

struct wg_sample_queue;

HRESULT wg_sample_queue_create(struct wg_sample_queue **out);
void wg_sample_queue_destroy(struct wg_sample_queue *queue);
void wg_sample_queue_flush(struct wg_sample_queue *queue, bool all);

wg_parser_t wg_parser_create(enum wg_parser_type type, bool output_compressed);
void wg_parser_destroy(wg_parser_t parser);

HRESULT wg_parser_connect(wg_parser_t parser, uint64_t file_size);
void wg_parser_disconnect(wg_parser_t parser);

bool wg_parser_get_next_read_offset(wg_parser_t parser, uint64_t *offset, uint32_t *size);
void wg_parser_push_data(wg_parser_t parser, const void *data, uint32_t size);

uint32_t wg_parser_get_stream_count(wg_parser_t parser);
wg_parser_stream_t wg_parser_get_stream(wg_parser_t parser, uint32_t index);

void wg_parser_stream_get_preferred_format(wg_parser_stream_t stream, struct wg_format *format);
void wg_parser_stream_get_codec_format(wg_parser_stream_t stream, struct wg_format *format);
void wg_parser_stream_enable(wg_parser_stream_t stream, const struct wg_format *format);
void wg_parser_stream_disable(wg_parser_stream_t stream);

bool wg_parser_stream_get_buffer(wg_parser_t parser, wg_parser_stream_t stream,
        struct wg_parser_buffer *buffer);
bool wg_parser_stream_copy_buffer(wg_parser_stream_t stream,
        void *data, uint32_t offset, uint32_t size);
void wg_parser_stream_release_buffer(wg_parser_stream_t stream);
void wg_parser_stream_notify_qos(wg_parser_stream_t stream,
        bool underflow, double proportion, int64_t diff, uint64_t timestamp);

/* Returns the duration in 100-nanosecond units. */
uint64_t wg_parser_stream_get_duration(wg_parser_stream_t stream);
char *wg_parser_stream_get_tag(wg_parser_stream_t stream, enum wg_parser_tag tag);
/* start_pos and stop_pos are in 100-nanosecond units. */
void wg_parser_stream_seek(wg_parser_stream_t stream, double rate,
        uint64_t start_pos, uint64_t stop_pos, DWORD start_flags, DWORD stop_flags);

wg_transform_t wg_transform_create(const struct wg_format *input_format,
        const struct wg_format *output_format, const struct wg_transform_attrs *attrs);
void wg_transform_destroy(wg_transform_t transform);
bool wg_transform_set_output_format(wg_transform_t transform, struct wg_format *format);
bool wg_transform_get_status(wg_transform_t transform, bool *accepts_input);
HRESULT wg_transform_drain(wg_transform_t transform);
HRESULT wg_transform_flush(wg_transform_t transform);
void wg_transform_notify_qos(wg_transform_t transform,
        bool underflow, double proportion, int64_t diff, uint64_t timestamp);

HRESULT wg_muxer_create(const char *format, wg_muxer_t *muxer);
void wg_muxer_destroy(wg_muxer_t muxer);
HRESULT wg_muxer_add_stream(wg_muxer_t muxer, UINT32 stream_id, const struct wg_format *format);
HRESULT wg_muxer_start(wg_muxer_t muxer);
HRESULT wg_muxer_push_sample(wg_muxer_t muxer, struct wg_sample *sample, UINT32 stream_id);
HRESULT wg_muxer_read_data(wg_muxer_t muxer, void *buffer, UINT32 *size, UINT64 *offset);
HRESULT wg_muxer_finalize(wg_muxer_t muxer);

unsigned int wg_format_get_bytes_for_uncompressed(wg_video_format format, unsigned int width, unsigned int height);
unsigned int wg_format_get_max_size(const struct wg_format *format);

HRESULT avi_splitter_create(IUnknown *outer, IUnknown **out);
HRESULT decodebin_parser_create(IUnknown *outer, IUnknown **out);
HRESULT mpeg_audio_codec_create(IUnknown *outer, IUnknown **out);
HRESULT mpeg_video_codec_create(IUnknown *outer, IUnknown **out);
HRESULT mpeg_layer3_decoder_create(IUnknown *outer, IUnknown **out);
HRESULT mpeg_splitter_create(IUnknown *outer, IUnknown **out);
HRESULT wave_parser_create(IUnknown *outer, IUnknown **out);
HRESULT wma_decoder_create(IUnknown *outer, IUnknown **out);
HRESULT wmv_decoder_create(IUnknown *outer, IUnknown **out);
HRESULT resampler_create(IUnknown *outer, IUnknown **out);
HRESULT color_convert_create(IUnknown *outer, IUnknown **out);
HRESULT mp3_sink_class_factory_create(IUnknown *outer, IUnknown **out);
HRESULT mpeg4_sink_class_factory_create(IUnknown *outer, IUnknown **out);

bool amt_from_wg_format(AM_MEDIA_TYPE *mt, const struct wg_format *format, bool wm);
bool amt_to_wg_format(const AM_MEDIA_TYPE *mt, struct wg_format *format);

BOOL init_gstreamer(void);

extern HRESULT mfplat_get_class_object(REFCLSID rclsid, REFIID riid, void **obj);
extern HRESULT mfplat_DllRegisterServer(void);

IMFMediaType *mf_media_type_from_wg_format(const struct wg_format *format);
void mf_media_type_to_wg_format(IMFMediaType *type, struct wg_format *format);

HRESULT wg_sample_create_mf(IMFSample *sample, struct wg_sample **out);
HRESULT wg_sample_create_quartz(IMediaSample *sample, struct wg_sample **out);
HRESULT wg_sample_create_dmo(IMediaBuffer *media_buffer, struct wg_sample **out);
void wg_sample_release(struct wg_sample *wg_sample);

HRESULT wg_transform_push_mf(wg_transform_t transform, IMFSample *sample,
        struct wg_sample_queue *queue);
HRESULT wg_transform_push_quartz(wg_transform_t transform, struct wg_sample *sample,
        struct wg_sample_queue *queue);
HRESULT wg_transform_push_dmo(wg_transform_t transform, IMediaBuffer *media_buffer,
        DWORD flags, REFERENCE_TIME time_stamp, REFERENCE_TIME time_length, struct wg_sample_queue *queue);
HRESULT wg_transform_read_mf(wg_transform_t transform, IMFSample *sample,
        DWORD sample_size, struct wg_format *format, DWORD *flags);
HRESULT wg_transform_read_quartz(wg_transform_t transform, struct wg_sample *sample);
HRESULT wg_transform_read_dmo(wg_transform_t transform, DMO_OUTPUT_DATA_BUFFER *buffer);

HRESULT gstreamer_byte_stream_handler_create(REFIID riid, void **obj);

unsigned int wg_format_get_stride(const struct wg_format *format);

bool wg_video_format_is_rgb(enum wg_video_format format);

HRESULT aac_decoder_create(REFIID riid, void **ret);
HRESULT h264_decoder_create(REFIID riid, void **ret);
HRESULT video_processor_create(REFIID riid, void **ret);

extern const GUID MFAudioFormat_RAW_AAC;

#endif /* __GST_PRIVATE_INCLUDED__ */
