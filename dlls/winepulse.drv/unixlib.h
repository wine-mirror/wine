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
#include "wine/unixlib.h"

#define MAX_PULSE_NAME_LEN 256

typedef UINT64 stream_handle;

enum phys_device_bus_type {
    phys_device_bus_invalid = -1,
    phys_device_bus_pci,
    phys_device_bus_usb
};

struct endpoint
{
    unsigned int name;
    unsigned int pulse_name;
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
    const char *name;
    const char *pulse_name;
    EDataFlow dataflow;
    AUDCLNT_SHAREMODE mode;
    DWORD flags;
    REFERENCE_TIME duration;
    const WAVEFORMATEX *fmt;
    HRESULT result;
    UINT32 *channel_count;
    stream_handle *stream;
};

struct release_stream_params
{
    stream_handle stream;
    HANDLE timer;
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
    DWORD flags;
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
    BOOL done;
    HRESULT result;
};

struct get_mix_format_params
{
    const char *pulse_name;
    EDataFlow flow;
    WAVEFORMATEXTENSIBLE *fmt;
    HRESULT result;
};

struct get_device_period_params
{
    const char *pulse_name;
    EDataFlow flow;
    HRESULT result;
    REFERENCE_TIME *def_period;
    REFERENCE_TIME *min_period;
};

struct get_buffer_size_params
{
    stream_handle stream;
    HRESULT result;
    UINT32 *size;
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

struct test_connect_params
{
    const char *name;
    HRESULT result;
};

struct is_started_params
{
    stream_handle stream;
    BOOL started;
};

struct get_prop_value_params
{
    const char *pulse_name;
    const GUID *guid;
    const PROPERTYKEY *prop;
    EDataFlow flow;
    HRESULT result;
    VARTYPE vt;
    union
    {
        WCHAR wstr[128];
        ULONG ulVal;
    };
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
    test_connect,
    is_started,
    get_prop_value,
};
