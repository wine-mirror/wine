/*
 * Copyright 2021 Jacek Caban for CodeWeavers
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

#include "wine/list.h"

struct pulse_stream;

struct pulse_config
{
    struct
    {
        WAVEFORMATEXTENSIBLE format;
        REFERENCE_TIME def_period;
        REFERENCE_TIME min_period;
    } modes[2];
    unsigned int speakers_mask;
};

struct main_loop_params
{
    HANDLE event;
};

struct create_stream_params
{
    const char *name;
    EDataFlow dataflow;
    AUDCLNT_SHAREMODE mode;
    DWORD flags;
    REFERENCE_TIME duration;
    const WAVEFORMATEX *fmt;
    HRESULT result;
    UINT32 *channel_count;
    struct pulse_stream **stream;
};

struct release_stream_params
{
    struct pulse_stream *stream;
    HANDLE timer;
    HRESULT result;
};

struct start_params
{
    struct pulse_stream *stream;
    HRESULT result;
};

struct stop_params
{
    struct pulse_stream *stream;
    HRESULT result;
};

struct reset_params
{
    struct pulse_stream *stream;
    HRESULT result;
};

struct timer_loop_params
{
    struct pulse_stream *stream;
};

struct get_render_buffer_params
{
    struct pulse_stream *stream;
    UINT32 frames;
    HRESULT result;
    BYTE **data;
};

struct release_render_buffer_params
{
    struct pulse_stream *stream;
    UINT32 written_frames;
    DWORD flags;
    HRESULT result;
};

struct get_capture_buffer_params
{
    struct pulse_stream *stream;
    HRESULT result;
    BYTE **data;
    UINT32 *frames;
    DWORD *flags;
    UINT64 *devpos;
    UINT64 *qpcpos;
};

struct release_capture_buffer_params
{
    struct pulse_stream *stream;
    BOOL done;
    HRESULT result;
};

struct get_buffer_size_params
{
    struct pulse_stream *stream;
    HRESULT result;
    UINT32 *size;
};

struct get_latency_params
{
    struct pulse_stream *stream;
    HRESULT result;
    REFERENCE_TIME *latency;
};

struct get_current_padding_params
{
    struct pulse_stream *stream;
    HRESULT result;
    UINT32 *padding;
};

struct get_next_packet_size_params
{
    struct pulse_stream *stream;
    HRESULT result;
    UINT32 *frames;
};

struct get_frequency_params
{
    struct pulse_stream *stream;
    HRESULT result;
    UINT64 *freq;
};

struct get_position_params
{
    struct pulse_stream *stream;
    BOOL device;
    HRESULT result;
    UINT64 *pos;
    UINT64 *qpctime;
};

struct set_volumes_params
{
    struct pulse_stream *stream;
    float master_volume;
    const float *volumes;
    const float *session_volumes;
};

struct set_event_handle_params
{
    struct pulse_stream *stream;
    HANDLE event;
    HRESULT result;
};

struct test_connect_params
{
    const char *name;
    HRESULT result;
    struct pulse_config *config;
};

struct is_started_params
{
    struct pulse_stream *stream;
    BOOL started;
};

struct unix_funcs
{
    void (WINAPI *main_loop)(struct main_loop_params *params);
    void (WINAPI *create_stream)(struct create_stream_params *params);
    void (WINAPI *release_stream)(struct release_stream_params *params);
    void (WINAPI *start)(struct start_params *params);
    void (WINAPI *stop)(struct stop_params *params);
    void (WINAPI *reset)(struct reset_params *params);
    void (WINAPI *timer_loop)(struct timer_loop_params *params);
    void (WINAPI *get_render_buffer)(struct get_render_buffer_params *params);
    void (WINAPI *release_render_buffer)(struct release_render_buffer_params *params);
    void (WINAPI *get_capture_buffer)(struct get_capture_buffer_params *params);
    void (WINAPI *release_capture_buffer)(struct release_capture_buffer_params *params);
    void (WINAPI *get_buffer_size)(struct get_buffer_size_params *params);
    void (WINAPI *get_latency)(struct get_latency_params *params);
    void (WINAPI *get_current_padding)(struct get_current_padding_params *params);
    void (WINAPI *get_next_packet_size)(struct get_next_packet_size_params *params);
    void (WINAPI *get_frequency)(struct get_frequency_params *params);
    void (WINAPI *get_position)(struct get_position_params *params);
    void (WINAPI *set_volumes)(struct set_volumes_params *params);
    void (WINAPI *set_event_handle)(struct set_event_handle_params *params);
    void (WINAPI *test_connect)(struct test_connect_params *params);
    void (WINAPI *is_started)(struct is_started_params *params);
};
