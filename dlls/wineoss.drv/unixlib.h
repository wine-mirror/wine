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

struct stream_oss;

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

struct get_next_packet_size_params
{
    struct oss_stream *stream;
    HRESULT result;
    UINT32 *frames;
};

struct get_frequency_params
{
    struct oss_stream *stream;
    HRESULT result;
    UINT64 *frequency;
};

struct get_position_params
{
    struct oss_stream *stream;
    HRESULT result;
    UINT64 *position;
    UINT64 *qpctime;
};

struct set_volumes_params
{
    struct oss_stream *stream;
    float master_volume;
    const float *volumes;
    const float *session_volumes;
};

struct set_event_handle_params
{
    struct oss_stream *stream;
    HANDLE event;
    HRESULT result;
};

struct is_started_params
{
    struct oss_stream *stream;
    HRESULT result;
};

#include <mmddk.h> /* temporary */

typedef struct midi_src
{
    int                 state; /* -1 disabled, 0 is no recording started, 1 in recording, bit 2 set if in sys exclusive recording */
    MIDIOPENDESC        midiDesc;
    WORD                wFlags;
    MIDIHDR            *lpQueueHdr;
    unsigned char       incoming[3];
    unsigned char       incPrev;
    char                incLen;
    UINT                startTime;
    MIDIINCAPSW         caps;
    int                 fd;
} WINE_MIDIIN;

typedef struct midi_dest
{
    BOOL                bEnabled;
    MIDIOPENDESC        midiDesc;
    WORD                wFlags;
    MIDIHDR            *lpQueueHdr;
    void               *lpExtra; /* according to port type (MIDI, FM...), extra data when needed */
    MIDIOUTCAPSW        caps;
    int                 fd;
} WINE_MIDIOUT;

struct midi_init_params
{
    UINT *err;
    unsigned int num_dests;
    unsigned int num_srcs;
    unsigned int num_synths;
    struct midi_dest *dests;
    struct midi_src *srcs;
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

struct midi_seq_open_params
{
    int close;
    int fd;
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
    oss_get_next_packet_size,
    oss_get_frequency,
    oss_get_position,
    oss_set_volumes,
    oss_set_event_handle,
    oss_is_started,
    oss_midi_init,
    oss_midi_out_message,

    oss_midi_seq_open, /* temporary */
};

NTSTATUS midi_init(void *args) DECLSPEC_HIDDEN;
NTSTATUS midi_out_message(void *args) DECLSPEC_HIDDEN;
NTSTATUS midi_seq_open(void *args) DECLSPEC_HIDDEN;

extern unixlib_handle_t oss_handle;

#define OSS_CALL(func, params) __wine_unix_call(oss_handle, oss_ ## func, params)
