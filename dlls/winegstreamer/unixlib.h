/*
 * winegstreamer Unix library interface
 *
 * Copyright 2020-2021 Zebediah Figura for CodeWeavers
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

#ifndef __WINE_WINEGSTREAMER_UNIXLIB_H
#define __WINE_WINEGSTREAMER_UNIXLIB_H

#include <stdbool.h>
#include <stdint.h>
#include "windef.h"
#include "wtypes.h"
#include "mmreg.h"

struct wg_format
{
    enum wg_major_type
    {
        WG_MAJOR_TYPE_UNKNOWN,
        WG_MAJOR_TYPE_VIDEO,
        WG_MAJOR_TYPE_AUDIO,
    } major_type;

    union
    {
        struct
        {
            enum wg_video_format
            {
                WG_VIDEO_FORMAT_UNKNOWN,

                WG_VIDEO_FORMAT_BGRA,
                WG_VIDEO_FORMAT_BGRx,
                WG_VIDEO_FORMAT_BGR,
                WG_VIDEO_FORMAT_RGB15,
                WG_VIDEO_FORMAT_RGB16,

                WG_VIDEO_FORMAT_AYUV,
                WG_VIDEO_FORMAT_I420,
                WG_VIDEO_FORMAT_NV12,
                WG_VIDEO_FORMAT_UYVY,
                WG_VIDEO_FORMAT_YUY2,
                WG_VIDEO_FORMAT_YV12,
                WG_VIDEO_FORMAT_YVYU,

                WG_VIDEO_FORMAT_CINEPAK,
            } format;
            uint32_t width, height;
            uint32_t fps_n, fps_d;
        } video;
        struct
        {
            enum wg_audio_format
            {
                WG_AUDIO_FORMAT_UNKNOWN,

                WG_AUDIO_FORMAT_U8,
                WG_AUDIO_FORMAT_S16LE,
                WG_AUDIO_FORMAT_S24LE,
                WG_AUDIO_FORMAT_S32LE,
                WG_AUDIO_FORMAT_F32LE,
                WG_AUDIO_FORMAT_F64LE,

                WG_AUDIO_FORMAT_MPEG1_LAYER1,
                WG_AUDIO_FORMAT_MPEG1_LAYER2,
                WG_AUDIO_FORMAT_MPEG1_LAYER3,
            } format;

            uint32_t channels;
            uint32_t channel_mask; /* In WinMM format. */
            uint32_t rate;
        } audio;
    } u;
};

enum wg_parser_event_type
{
    WG_PARSER_EVENT_NONE = 0,
    WG_PARSER_EVENT_BUFFER,
    WG_PARSER_EVENT_EOS,
    WG_PARSER_EVENT_SEGMENT,
};

struct wg_parser_event
{
    enum wg_parser_event_type type;
    union
    {
        struct
        {
            /* pts and duration are in 100-nanosecond units. */
            ULONGLONG pts, duration;
            uint32_t size;
            bool discontinuity, preroll, delta, has_pts, has_duration;
        } buffer;
        struct
        {
            ULONGLONG position, stop;
            DOUBLE rate;
        } segment;
    } u;
};
C_ASSERT(sizeof(struct wg_parser_event) == 40);

enum wg_parser_type
{
    WG_PARSER_DECODEBIN,
    WG_PARSER_AVIDEMUX,
    WG_PARSER_MPEGAUDIOPARSE,
    WG_PARSER_WAVPARSE,
};

struct unix_funcs
{
    struct wg_parser *(CDECL *wg_parser_create)(enum wg_parser_type type, bool unlimited_buffering);
    void (CDECL *wg_parser_destroy)(struct wg_parser *parser);

    HRESULT (CDECL *wg_parser_connect)(struct wg_parser *parser, uint64_t file_size);
    void (CDECL *wg_parser_disconnect)(struct wg_parser *parser);

    void (CDECL *wg_parser_begin_flush)(struct wg_parser *parser);
    void (CDECL *wg_parser_end_flush)(struct wg_parser *parser);

    bool (CDECL *wg_parser_get_next_read_offset)(struct wg_parser *parser,
            uint64_t *offset, uint32_t *size);
    void (CDECL *wg_parser_push_data)(struct wg_parser *parser,
            const void *data, uint32_t size);

    uint32_t (CDECL *wg_parser_get_stream_count)(struct wg_parser *parser);
    struct wg_parser_stream *(CDECL *wg_parser_get_stream)(struct wg_parser *parser, uint32_t index);

    void (CDECL *wg_parser_stream_get_preferred_format)(struct wg_parser_stream *stream, struct wg_format *format);
    void (CDECL *wg_parser_stream_enable)(struct wg_parser_stream *stream, const struct wg_format *format);
    void (CDECL *wg_parser_stream_disable)(struct wg_parser_stream *stream);

    bool (CDECL *wg_parser_stream_get_event)(struct wg_parser_stream *stream, struct wg_parser_event *event);
    bool (CDECL *wg_parser_stream_copy_buffer)(struct wg_parser_stream *stream,
            void *data, uint32_t offset, uint32_t size);
    void (CDECL *wg_parser_stream_release_buffer)(struct wg_parser_stream *stream);
    void (CDECL *wg_parser_stream_notify_qos)(struct wg_parser_stream *stream,
            bool underflow, double proportion, int64_t diff, uint64_t timestamp);

    /* Returns the duration in 100-nanosecond units. */
    uint64_t (CDECL *wg_parser_stream_get_duration)(struct wg_parser_stream *stream);
    /* start_pos and stop_pos are in 100-nanosecond units. */
    bool (CDECL *wg_parser_stream_seek)(struct wg_parser_stream *stream, double rate,
            uint64_t start_pos, uint64_t stop_pos, DWORD start_flags, DWORD stop_flags);
};

#endif /* __WINE_WINEGSTREAMER_UNIXLIB_H */
