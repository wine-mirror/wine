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
#include "wmsdk.h"
#include "wine/debug.h"
#include "wine/strmbase.h"

#include "unixlib.h"

bool array_reserve(void **elements, size_t *capacity, size_t count, size_t size) DECLSPEC_HIDDEN;

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

struct wg_parser *wg_parser_create(enum wg_parser_type type, bool unlimited_buffering) DECLSPEC_HIDDEN;
void wg_parser_destroy(struct wg_parser *parser) DECLSPEC_HIDDEN;

HRESULT wg_parser_connect(struct wg_parser *parser, uint64_t file_size) DECLSPEC_HIDDEN;
void wg_parser_disconnect(struct wg_parser *parser) DECLSPEC_HIDDEN;

void wg_parser_begin_flush(struct wg_parser *parser) DECLSPEC_HIDDEN;
void wg_parser_end_flush(struct wg_parser *parser) DECLSPEC_HIDDEN;

bool wg_parser_get_next_read_offset(struct wg_parser *parser, uint64_t *offset, uint32_t *size) DECLSPEC_HIDDEN;
void wg_parser_push_data(struct wg_parser *parser, const void *data, uint32_t size) DECLSPEC_HIDDEN;

uint32_t wg_parser_get_stream_count(struct wg_parser *parser) DECLSPEC_HIDDEN;
struct wg_parser_stream *wg_parser_get_stream(struct wg_parser *parser, uint32_t index) DECLSPEC_HIDDEN;

void wg_parser_stream_get_preferred_format(struct wg_parser_stream *stream, struct wg_format *format) DECLSPEC_HIDDEN;
void wg_parser_stream_enable(struct wg_parser_stream *stream, const struct wg_format *format) DECLSPEC_HIDDEN;
void wg_parser_stream_disable(struct wg_parser_stream *stream) DECLSPEC_HIDDEN;

bool wg_parser_stream_get_event(struct wg_parser_stream *stream, struct wg_parser_event *event) DECLSPEC_HIDDEN;
bool wg_parser_stream_copy_buffer(struct wg_parser_stream *stream,
        void *data, uint32_t offset, uint32_t size) DECLSPEC_HIDDEN;
void wg_parser_stream_release_buffer(struct wg_parser_stream *stream) DECLSPEC_HIDDEN;
void wg_parser_stream_notify_qos(struct wg_parser_stream *stream,
        bool underflow, double proportion, int64_t diff, uint64_t timestamp) DECLSPEC_HIDDEN;

/* Returns the duration in 100-nanosecond units. */
uint64_t wg_parser_stream_get_duration(struct wg_parser_stream *stream) DECLSPEC_HIDDEN;
/* start_pos and stop_pos are in 100-nanosecond units. */
void wg_parser_stream_seek(struct wg_parser_stream *stream, double rate,
        uint64_t start_pos, uint64_t stop_pos, DWORD start_flags, DWORD stop_flags) DECLSPEC_HIDDEN;

HRESULT avi_splitter_create(IUnknown *outer, IUnknown **out) DECLSPEC_HIDDEN;
HRESULT decodebin_parser_create(IUnknown *outer, IUnknown **out) DECLSPEC_HIDDEN;
HRESULT mpeg_splitter_create(IUnknown *outer, IUnknown **out) DECLSPEC_HIDDEN;
HRESULT wave_parser_create(IUnknown *outer, IUnknown **out) DECLSPEC_HIDDEN;

BOOL init_gstreamer(void) DECLSPEC_HIDDEN;

extern HRESULT mfplat_get_class_object(REFCLSID rclsid, REFIID riid, void **obj) DECLSPEC_HIDDEN;
extern HRESULT mfplat_DllRegisterServer(void) DECLSPEC_HIDDEN;

IMFMediaType *mf_media_type_from_wg_format(const struct wg_format *format) DECLSPEC_HIDDEN;
void mf_media_type_to_wg_format(IMFMediaType *type, struct wg_format *format) DECLSPEC_HIDDEN;

HRESULT winegstreamer_stream_handler_create(REFIID riid, void **obj) DECLSPEC_HIDDEN;

HRESULT audio_converter_create(REFIID riid, void **ret) DECLSPEC_HIDDEN;

struct wm_reader
{
    IWMHeaderInfo3 IWMHeaderInfo3_iface;
    IWMLanguageList IWMLanguageList_iface;
    IWMPacketSize2 IWMPacketSize2_iface;
    IWMProfile3 IWMProfile3_iface;
    IWMReaderPlaylistBurn IWMReaderPlaylistBurn_iface;
    IWMReaderTimecode IWMReaderTimecode_iface;
    LONG refcount;

    const struct wm_reader_ops *ops;
};

struct wm_reader_ops
{
    void *(*query_interface)(struct wm_reader *reader, REFIID iid);
    void (*destroy)(struct wm_reader *reader);
};

void wm_reader_init(struct wm_reader *reader, const struct wm_reader_ops *ops);

#endif /* __GST_PRIVATE_INCLUDED__ */
