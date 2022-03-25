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

#include "audioclient.h"

struct alsa_stream;

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
    const char *alsa_name;
    EDataFlow flow;
    AUDCLNT_SHAREMODE share;
    DWORD flags;
    REFERENCE_TIME duration;
    REFERENCE_TIME period;
    const WAVEFORMATEX *fmt;
    HRESULT result;
    struct alsa_stream **stream;
};

struct release_stream_params
{
    struct alsa_stream *stream;
    HANDLE timer_thread;
    HRESULT result;
};

struct start_params
{
    struct alsa_stream *stream;
    HRESULT result;
};

struct stop_params
{
    struct alsa_stream *stream;
    HRESULT result;
};

struct reset_params
{
    struct alsa_stream *stream;
    HRESULT result;
};

struct timer_loop_params
{
    struct alsa_stream *stream;
};

struct get_render_buffer_params
{
    struct alsa_stream *stream;
    UINT32 frames;
    HRESULT result;
    BYTE **data;
};

struct release_render_buffer_params
{
    struct alsa_stream *stream;
    UINT32 written_frames;
    UINT flags;
    HRESULT result;
};

struct get_capture_buffer_params
{
    struct alsa_stream *stream;
    HRESULT result;
    BYTE **data;
    UINT32 *frames;
    UINT *flags;
    UINT64 *devpos;
    UINT64 *qpcpos;
};

struct release_capture_buffer_params
{
    struct alsa_stream *stream;
    UINT32 done;
    HRESULT result;
};

struct is_format_supported_params
{
    const char *alsa_name;
    EDataFlow flow;
    AUDCLNT_SHAREMODE share;
    const WAVEFORMATEX *fmt_in;
    WAVEFORMATEXTENSIBLE *fmt_out;
    HRESULT result;
};

struct get_mix_format_params
{
    const char *alsa_name;
    EDataFlow flow;
    WAVEFORMATEXTENSIBLE *fmt;
    HRESULT result;
};

struct get_buffer_size_params
{
    struct alsa_stream *stream;
    HRESULT result;
    UINT32 *size;
};

struct get_latency_params
{
    struct alsa_stream *stream;
    HRESULT result;
    REFERENCE_TIME *latency;
};

struct get_current_padding_params
{
    struct alsa_stream *stream;
    HRESULT result;
    UINT32 *padding;
};

struct get_next_packet_size_params
{
    struct alsa_stream *stream;
    HRESULT result;
    UINT32 *frames;
};

struct get_frequency_params
{
    struct alsa_stream *stream;
    HRESULT result;
    UINT64 *freq;
};

struct get_position_params
{
    struct alsa_stream *stream;
    HRESULT result;
    UINT64 *pos;
    UINT64 *qpctime;
};

struct set_volumes_params
{
    struct alsa_stream *stream;
    float master_volume;
    const float *volumes;
    const float *session_volumes;
};

struct set_event_handle_params
{
    struct alsa_stream *stream;
    HANDLE event;
    HRESULT result;
};

struct is_started_params
{
    struct alsa_stream *stream;
    HRESULT result;
};

struct get_prop_value_params
{
    const char *alsa_name;
    EDataFlow flow;
    const GUID *guid;
    const PROPERTYKEY *prop;
    HRESULT result;
    PROPVARIANT *value;
    void *buffer; /* caller allocated buffer to hold value's strings */
    unsigned int *buffer_size;
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
    BOOL *quit;
    struct notify_context *notify;
};

enum alsa_funcs
{
    alsa_get_endpoint_ids,
    alsa_create_stream,
    alsa_release_stream,
    alsa_start,
    alsa_stop,
    alsa_reset,
    alsa_timer_loop,
    alsa_get_render_buffer,
    alsa_release_render_buffer,
    alsa_get_capture_buffer,
    alsa_release_capture_buffer,
    alsa_is_format_supported,
    alsa_get_mix_format,
    alsa_get_buffer_size,
    alsa_get_latency,
    alsa_get_current_padding,
    alsa_get_next_packet_size,
    alsa_get_frequency,
    alsa_get_position,
    alsa_set_volumes,
    alsa_set_event_handle,
    alsa_is_started,
    alsa_get_prop_value,
    alsa_midi_release,
    alsa_midi_out_message,
    alsa_midi_in_message,
    alsa_midi_notify_wait,
};

NTSTATUS midi_release(void *args) DECLSPEC_HIDDEN;
NTSTATUS midi_out_message(void *args) DECLSPEC_HIDDEN;
NTSTATUS midi_in_message(void *args) DECLSPEC_HIDDEN;
NTSTATUS midi_notify_wait(void *args) DECLSPEC_HIDDEN;

extern unixlib_handle_t alsa_handle;

#define ALSA_CALL(func, params) __wine_unix_call(alsa_handle, alsa_ ## func, params)
