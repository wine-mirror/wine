/*
 * Copyright 2022 Huw Davies
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

#include "mmdeviceapi.h"

struct oss_stream
{
    WAVEFORMATEX *fmt;
    EDataFlow flow;
    UINT flags;
    AUDCLNT_SHAREMODE share;
    HANDLE event;

    int fd;

    BOOL playing, mute, please_quit;
    UINT64 written_frames, last_pos_frames;
    UINT32 period_frames, bufsize_frames, held_frames, tmp_buffer_frames, in_oss_frames;
    UINT32 oss_bufsize_bytes, lcl_offs_frames; /* offs into local_buffer where valid data starts */
    REFERENCE_TIME period;

    BYTE *local_buffer, *tmp_buffer;
    INT32 getbuf_last; /* <0 when using tmp_buffer */

    pthread_mutex_t lock;
};

/* From <dlls/mmdevapi/mmdevapi.h> */
enum DriverPriority
{
    Priority_Unavailable = 0,
    Priority_Low,
    Priority_Neutral,
    Priority_Preferred
};

struct test_connect_params
{
    enum DriverPriority priority;
};

struct endpoint
{
    WCHAR *name;
    char *device;
};

struct get_endpoint_ids_params
{
    EDataFlow flow;
    struct endpoint *endpoints;
    unsigned int size;
    HRESULT result;
    unsigned int num;
    unsigned int default_idx;
};

struct create_stream_params
{
    const char *device;
    EDataFlow flow;
    AUDCLNT_SHAREMODE share;
    UINT flags;
    REFERENCE_TIME duration;
    REFERENCE_TIME period;
    const WAVEFORMATEX *fmt;
    HRESULT result;
    struct oss_stream **stream;
};

struct release_stream_params
{
    struct oss_stream *stream;
    HANDLE timer_thread;
    HRESULT result;
};

struct start_params
{
    struct oss_stream *stream;
    HRESULT result;
};

struct stop_params
{
    struct oss_stream *stream;
    HRESULT result;
};

struct reset_params
{
    struct oss_stream *stream;
    HRESULT result;
};

struct timer_loop_params
{
    struct oss_stream *stream;
};

struct get_render_buffer_params
{
    struct oss_stream *stream;
    UINT32 frames;
    HRESULT result;
    BYTE **data;
};

struct release_render_buffer_params
{
    struct oss_stream *stream;
    UINT32 written_frames;
    UINT flags;
    HRESULT result;
};

struct get_capture_buffer_params
{
    struct oss_stream *stream;
    HRESULT result;
    BYTE **data;
    UINT32 *frames;
    UINT *flags;
    UINT64 *devpos;
    UINT64 *qpcpos;
};

struct release_capture_buffer_params
{
    struct oss_stream *stream;
    UINT32 done;
    HRESULT result;
};

struct is_format_supported_params
{
    const char *device;
    EDataFlow flow;
    AUDCLNT_SHAREMODE share;
    const WAVEFORMATEX *fmt_in;
    WAVEFORMATEXTENSIBLE *fmt_out;
    HRESULT result;
};

struct get_mix_format_params
{
    const char *device;
    EDataFlow flow;
    WAVEFORMATEXTENSIBLE *fmt;
    HRESULT result;
};

struct get_buffer_size_params
{
    struct oss_stream *stream;
    HRESULT result;
    UINT32 *size;
};

struct get_latency_params
{
    struct oss_stream *stream;
    HRESULT result;
    REFERENCE_TIME *latency;
};

struct get_current_padding_params
{
    struct oss_stream *stream;
    HRESULT result;
    UINT32 *padding;
};

struct set_event_handle_params
{
    struct oss_stream *stream;
    HANDLE event;
    HRESULT result;
};

enum oss_funcs
{
    oss_test_connect,
    oss_get_endpoint_ids,
    oss_create_stream,
    oss_release_stream,
    oss_start,
    oss_stop,
    oss_reset,
    oss_timer_loop,
    oss_get_render_buffer,
    oss_release_render_buffer,
    oss_get_capture_buffer,
    oss_release_capture_buffer,
    oss_is_format_supported,
    oss_get_mix_format,
    oss_get_buffer_size,
    oss_get_latency,
    oss_get_current_padding,
    oss_set_event_handle,
};

extern unixlib_handle_t oss_handle;

#define OSS_CALL(func, params) __wine_unix_call(oss_handle, oss_ ## func, params)
