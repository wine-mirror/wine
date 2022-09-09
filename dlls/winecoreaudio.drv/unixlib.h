/*
 * Unixlib header file for winecoreaudio driver.
 *
 * Copyright 2021 Huw Davies
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
#include "mmddk.h"

typedef UINT64 stream_handle;

struct endpoint
{
    unsigned int name;
    unsigned int device;
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
    const char *name;
    const char *device;
    EDataFlow flow;
    AUDCLNT_SHAREMODE share;
    DWORD flags;
    REFERENCE_TIME duration;
    REFERENCE_TIME period;
    const WAVEFORMATEX *fmt;
    HRESULT result;
    UINT32 *channel_count;
    stream_handle *stream;
};

struct release_stream_params
{
    stream_handle stream;
    HANDLE timer_thread;
    HRESULT result;
};

struct start_params
{
    stream_handle stream;
    HRESULT result;
};

struct stop_params
{
    stream_handle stream;
    HRESULT result;
};

struct reset_params
{
    stream_handle stream;
    HRESULT result;
};

struct get_render_buffer_params
{
    stream_handle stream;
    UINT32 frames;
    HRESULT result;
    BYTE **data;
};

struct release_render_buffer_params
{
    stream_handle stream;
    UINT32 written_frames;
    UINT flags;
    HRESULT result;
};

struct get_capture_buffer_params
{
    stream_handle stream;
    HRESULT result;
    BYTE **data;
    UINT32 *frames;
    DWORD *flags;
    UINT64 *devpos;
    UINT64 *qpcpos;
};

struct release_capture_buffer_params
{
    stream_handle stream;
    UINT32 done;
    HRESULT result;
};

struct get_mix_format_params
{
    const char *device;
    EDataFlow flow;
    WAVEFORMATEXTENSIBLE *fmt;
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

struct get_buffer_size_params
{
    stream_handle stream;
    HRESULT result;
    UINT32 *frames;
};

struct get_latency_params
{
    stream_handle stream;
    HRESULT result;
    REFERENCE_TIME *latency;
};

struct get_current_padding_params
{
    stream_handle stream;
    HRESULT result;
    UINT32 *padding;
};

struct get_next_packet_size_params
{
    stream_handle stream;
    HRESULT result;
    UINT32 *frames;
};

struct get_position_params
{
    stream_handle stream;
    HRESULT result;
    UINT64 *pos;
    UINT64 *qpctime;
};

struct get_frequency_params
{
    stream_handle stream;
    HRESULT result;
    UINT64 *freq;
};

struct is_started_params
{
    stream_handle stream;
    HRESULT result;
};

struct set_volumes_params
{
    stream_handle stream;
    float master_volume;
    const float *volumes;
    const float *session_volumes;
    int channel;
};

struct midi_init_params
{
    UINT *err;
};

struct notify_context
{
    BOOL send_notify;
    WORD dev_id;
    WORD msg;
    UINT_PTR param_1;
    UINT_PTR param_2;
    UINT_PTR callback;
    UINT flags;
    HANDLE device;
    UINT_PTR instance;
};

struct midi_out_message_params
{
    UINT dev_id;
    UINT msg;
    UINT_PTR user;
    UINT_PTR param_1;
    UINT_PTR param_2;
    UINT *err;
    struct notify_context *notify;
};

struct midi_in_message_params
{
    UINT dev_id;
    UINT msg;
    UINT_PTR user;
    UINT_PTR param_1;
    UINT_PTR param_2;
    UINT *err;
    struct notify_context *notify;
};

struct midi_notify_wait_params
{
    struct notify_context *notify;
    BOOL *quit;
};

enum unix_funcs
{
    get_endpoint_ids,
    create_stream,
    release_stream,
    start,
    stop,
    reset,
    get_render_buffer,
    release_render_buffer,
    get_capture_buffer,
    release_capture_buffer,
    get_mix_format,
    is_format_supported,
    get_buffer_size,
    get_latency,
    get_current_padding,
    get_next_packet_size,
    get_position,
    get_frequency,
    is_started,
    set_volumes,
    midi_init,
    midi_release,
    midi_out_message,
    midi_in_message,
    midi_notify_wait,
};

NTSTATUS unix_midi_init( void * ) DECLSPEC_HIDDEN;
NTSTATUS unix_midi_release( void * ) DECLSPEC_HIDDEN;
NTSTATUS unix_midi_out_message( void * ) DECLSPEC_HIDDEN;
NTSTATUS unix_midi_in_message( void * ) DECLSPEC_HIDDEN;
NTSTATUS unix_midi_notify_wait( void * ) DECLSPEC_HIDDEN;

#ifdef _WIN64
NTSTATUS unix_wow64_midi_init(void *args) DECLSPEC_HIDDEN;
NTSTATUS unix_wow64_midi_out_message(void *args) DECLSPEC_HIDDEN;
NTSTATUS unix_wow64_midi_in_message(void *args) DECLSPEC_HIDDEN;
NTSTATUS unix_wow64_midi_notify_wait(void *args) DECLSPEC_HIDDEN;
#endif

extern unixlib_handle_t coreaudio_handle;

#define UNIX_CALL( func, params ) __wine_unix_call( coreaudio_handle, func, params )
