/*
 * Copyright 2021 Jacek Caban for CodeWeavers
 * Copyright 2021-2022 Huw Davies
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
#include "mmdeviceapi.h"

typedef UINT64 stream_handle;

enum driver_priority
{
    Priority_Unavailable = 0, /* driver won't work */
    Priority_Low, /* driver may work, but unlikely */
    Priority_Neutral, /* driver makes no judgment */
    Priority_Preferred /* driver thinks it's correct */
};

struct endpoint
{
    unsigned int name;
    unsigned int device;
};

struct main_loop_params
{
    HANDLE event;
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
    const WCHAR *name;
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

struct timer_loop_params
{
    stream_handle stream;
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
    UINT *flags;
    UINT64 *devpos;
    UINT64 *qpcpos;
};

struct release_capture_buffer_params
{
    stream_handle stream;
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

struct get_loopback_capture_device_params
{
    const WCHAR *name;
    const char *device;
    char *ret_device;
    UINT32 ret_device_len;
    HRESULT result;
};

struct get_mix_format_params
{
    const char *device;
    EDataFlow flow;
    WAVEFORMATEXTENSIBLE *fmt;
    HRESULT result;
};

struct get_device_period_params
{
    const char *device;
    EDataFlow flow;
    HRESULT result;
    REFERENCE_TIME *def_period;
    REFERENCE_TIME *min_period;
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

struct get_frequency_params
{
    stream_handle stream;
    HRESULT result;
    UINT64 *freq;
};

struct get_position_params
{
    stream_handle stream;
    BOOL device;
    HRESULT result;
    UINT64 *pos;
    UINT64 *qpctime;
};

struct set_volumes_params
{
    stream_handle stream;
    float master_volume;
    const float *volumes;
    const float *session_volumes;
};

struct set_event_handle_params
{
    stream_handle stream;
    HANDLE event;
    HRESULT result;
};

struct set_sample_rate_params
{
    stream_handle stream;
    float rate;
    HRESULT result;
};

struct test_connect_params
{
    const WCHAR *name;
    enum driver_priority priority;
};

struct is_started_params
{
    stream_handle stream;
    HRESULT result;
};

struct get_prop_value_params
{
    const char *device;
    EDataFlow flow;
    const GUID *guid;
    const PROPERTYKEY *prop;
    HRESULT result;
    PROPVARIANT *value;
    void *buffer; /* caller allocated buffer to hold value's strings */
    unsigned int *buffer_size;
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
    BOOL *quit;
    struct notify_context *notify;
};

struct aux_message_params
{
    UINT dev_id;
    UINT msg;
    UINT_PTR user;
    UINT_PTR param_1;
    UINT_PTR param_2;
    UINT *err;
};

enum unix_funcs
{
    process_attach,
    process_detach,
    main_loop,
    get_endpoint_ids,
    create_stream,
    release_stream,
    start,
    stop,
    reset,
    timer_loop,
    get_render_buffer,
    release_render_buffer,
    get_capture_buffer,
    release_capture_buffer,
    is_format_supported,
    get_loopback_capture_device,
    get_mix_format,
    get_device_period,
    get_buffer_size,
    get_latency,
    get_current_padding,
    get_next_packet_size,
    get_frequency,
    get_position,
    set_volumes,
    set_event_handle,
    set_sample_rate,
    test_connect,
    is_started,
    get_prop_value,
    midi_init,
    midi_release,
    midi_out_message,
    midi_in_message,
    midi_notify_wait,
    aux_message,
    funcs_count
};
