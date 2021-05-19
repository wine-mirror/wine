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

struct pulse_stream
{
    EDataFlow dataflow;

    pa_stream *stream;
    pa_sample_spec ss;
    pa_channel_map map;
    pa_buffer_attr attr;

    DWORD flags;
    AUDCLNT_SHAREMODE share;
    HANDLE event;
    float vol[PA_CHANNELS_MAX];

    INT32 locked;
    UINT32 bufsize_frames, real_bufsize_bytes, period_bytes;
    UINT32 started, peek_ofs, read_offs_bytes, lcl_offs_bytes, pa_offs_bytes;
    UINT32 tmp_buffer_bytes, held_bytes, peek_len, peek_buffer_len, pa_held_bytes;
    BYTE *local_buffer, *tmp_buffer, *peek_buffer;
    void *locked_ptr;
    BOOL please_quit, just_started, just_underran;
    pa_usec_t last_time, mmdev_period_usec;

    INT64 clock_lastpos, clock_written;

    struct list packet_free_head;
    struct list packet_filled_head;
};

struct unix_funcs
{
    void (WINAPI *lock)(void);
    void (WINAPI *unlock)(void);
    int (WINAPI *cond_wait)(void);
    void (WINAPI *main_loop)(void);
    HRESULT (WINAPI *create_stream)(const char *name, EDataFlow dataflow, AUDCLNT_SHAREMODE mode,
                                    DWORD flags, REFERENCE_TIME duration, REFERENCE_TIME period,
                                    const WAVEFORMATEX *fmt, UINT32 *channel_count,
                                    struct pulse_stream **ret);
    void (WINAPI *release_stream)(struct pulse_stream *stream, HANDLE timer);
    HRESULT (WINAPI *start)(struct pulse_stream *stream);
    HRESULT (WINAPI *stop)(struct pulse_stream *stream);
    HRESULT (WINAPI *reset)(struct pulse_stream *stream);
    void (WINAPI *timer_loop)(struct pulse_stream *stream);
    HRESULT (WINAPI *get_render_buffer)(struct pulse_stream *stream, UINT32 frames, BYTE **data);
    HRESULT (WINAPI *release_render_buffer)(struct pulse_stream *stream, UINT32 written_frames,
                                            DWORD flags);
    HRESULT (WINAPI *get_capture_buffer)(struct pulse_stream *stream, BYTE **data, UINT32 *frames,
                                         DWORD *flags, UINT64 *devpos, UINT64 *qpcpos);
    HRESULT (WINAPI *release_capture_buffer)(struct pulse_stream *stream, BOOL done);
    HRESULT (WINAPI *get_buffer_size)(struct pulse_stream *stream, UINT32 *out);
    HRESULT (WINAPI *get_latency)(struct pulse_stream *stream, REFERENCE_TIME *latency);
    HRESULT (WINAPI *get_current_padding)(struct pulse_stream *stream, UINT32 *out);
    HRESULT (WINAPI *get_next_packet_size)(struct pulse_stream *stream, UINT32 *frames);
    HRESULT (WINAPI *get_frequency)(struct pulse_stream *stream, UINT64 *freq);
    void (WINAPI *set_volumes)(struct pulse_stream *stream, float master_volume,
                               const float *volumes, const float *session_volumes);
    HRESULT (WINAPI *set_event_handle)(struct pulse_stream *stream, HANDLE event);
    HRESULT (WINAPI *test_connect)(const char *name, struct pulse_config *config);
};
