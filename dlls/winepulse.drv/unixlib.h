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

typedef struct _ACPacket
{
    struct list entry;
    UINT64 qpcpos;
    BYTE *data;
    UINT32 discont;
} ACPacket;

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
    void (WINAPI *broadcast)(void);
    void (WINAPI *main_loop)(void);
    HRESULT (WINAPI *create_stream)(const char *name, EDataFlow dataflow, AUDCLNT_SHAREMODE mode,
                                    DWORD flags, REFERENCE_TIME duration, REFERENCE_TIME period,
                                    const WAVEFORMATEX *fmt, UINT32 *channel_count,
                                    struct pulse_stream **ret);
    void (WINAPI *release_stream)(struct pulse_stream *stream, HANDLE timer);
    HRESULT (WINAPI *test_connect)(const char *name, struct pulse_config *config);
};
