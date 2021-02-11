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

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/audio/audio.h>

#define COBJMACROS
#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "winuser.h"
#include "dshow.h"
#include "strmif.h"
#include "mfobjects.h"
#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/strmbase.h"

typedef enum
{
    GST_AUTOPLUG_SELECT_TRY,
    GST_AUTOPLUG_SELECT_EXPOSE,
    GST_AUTOPLUG_SELECT_SKIP,
} GstAutoplugSelectResult;

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
            uint32_t rate;
        } audio;
    } u;
};

struct wg_parser
{
    BOOL (*init_gst)(struct wg_parser *parser);

    struct wg_parser_stream **streams;
    unsigned int stream_count;

    GstElement *container;
    GstBus *bus;
    GstPad *my_src, *their_sink;

    guint64 file_size, start_offset, next_offset, stop_offset;

    pthread_t push_thread;

    pthread_mutex_t mutex;

    pthread_cond_t init_cond;
    bool no_more_pads, has_duration, error;

    pthread_cond_t read_cond, read_done_cond;
    struct
    {
        GstBuffer *buffer;
        uint64_t offset;
        uint32_t size;
        bool done;
        GstFlowReturn ret;
    } read_request;

    bool flushing, sink_connected;
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
        GstBuffer *buffer;
        struct
        {
            uint64_t position, stop;
            double rate;
        } segment;
    } u;
};

struct wg_parser_stream
{
    struct wg_parser *parser;

    GstPad *their_src, *post_sink, *post_src, *my_sink;
    GstElement *flip;
    struct wg_format preferred_format, current_format;

    pthread_cond_t event_cond, event_empty_cond;
    struct wg_parser_event event;

    bool flushing, eos, enabled, has_caps;

    uint64_t duration;
};

struct unix_funcs
{
    struct wg_parser *(CDECL *wg_decodebin_parser_create)(void);
    struct wg_parser *(CDECL *wg_avi_parser_create)(void);
    struct wg_parser *(CDECL *wg_mpeg_audio_parser_create)(void);
    struct wg_parser *(CDECL *wg_wave_parser_create)(void);
    void (CDECL *wg_parser_destroy)(struct wg_parser *parser);
};

extern const struct unix_funcs *unix_funcs;

extern LONG object_locks;

HRESULT avi_splitter_create(IUnknown *outer, IUnknown **out) DECLSPEC_HIDDEN;
HRESULT decodebin_parser_create(IUnknown *outer, IUnknown **out) DECLSPEC_HIDDEN;
HRESULT mpeg_splitter_create(IUnknown *outer, IUnknown **out) DECLSPEC_HIDDEN;
HRESULT wave_parser_create(IUnknown *outer, IUnknown **out) DECLSPEC_HIDDEN;

BOOL init_gstreamer(void) DECLSPEC_HIDDEN;

void start_dispatch_thread(void) DECLSPEC_HIDDEN;

extern HRESULT mfplat_get_class_object(REFCLSID rclsid, REFIID riid, void **obj) DECLSPEC_HIDDEN;
extern HRESULT mfplat_DllRegisterServer(void) DECLSPEC_HIDDEN;

HRESULT winegstreamer_stream_handler_create(REFIID riid, void **obj) DECLSPEC_HIDDEN;
IMFMediaType *mf_media_type_from_caps(const GstCaps *caps) DECLSPEC_HIDDEN;
GstCaps *caps_from_mf_media_type(IMFMediaType *type) DECLSPEC_HIDDEN;
IMFSample *mf_sample_from_gst_buffer(GstBuffer *in) DECLSPEC_HIDDEN;

HRESULT winegstreamer_stream_handler_create(REFIID riid, void **obj) DECLSPEC_HIDDEN;

HRESULT audio_converter_create(REFIID riid, void **ret) DECLSPEC_HIDDEN;

#endif /* __GST_PRIVATE_INCLUDED__ */
