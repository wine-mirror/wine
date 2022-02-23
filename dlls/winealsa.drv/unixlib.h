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

struct alsa_stream
{
    snd_pcm_t *pcm_handle;
    snd_pcm_uframes_t alsa_bufsize_frames, alsa_period_frames, safe_rewind_frames;
    snd_pcm_hw_params_t *hw_params; /* does not hold state between calls */
    snd_pcm_format_t alsa_format;

    LARGE_INTEGER last_period_time;

    WAVEFORMATEX *fmt;
    DWORD flags;
    AUDCLNT_SHAREMODE share;
    EDataFlow flow;
    HANDLE event;

    BOOL need_remapping;
    int alsa_channels;
    int alsa_channel_map[32];

    BOOL started;
    REFERENCE_TIME mmdev_period_rt;
    UINT64 written_frames, last_pos_frames;
    UINT32 bufsize_frames, held_frames, tmp_buffer_frames, mmdev_period_frames;
    snd_pcm_uframes_t remapping_buf_frames;
    UINT32 lcl_offs_frames; /* offs into local_buffer where valid data starts */
    UINT32 wri_offs_frames; /* where to write fresh data in local_buffer */
    UINT32 hidden_frames;   /* ALSA reserve to ensure continuous rendering */
    UINT32 vol_adjusted_frames; /* Frames we've already adjusted the volume of but didn't write yet */
    UINT32 data_in_alsa_frames;

    BYTE *local_buffer, *tmp_buffer, *remapping_buf, *silence_buf;
    LONG32 getbuf_last; /* <0 when using tmp_buffer */
    float *vols;

    pthread_mutex_t lock;
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

enum alsa_funcs
{
    alsa_get_endpoint_ids,
    alsa_create_stream,
    alsa_release_stream,
    alsa_is_format_supported,
    alsa_get_mix_format,
    alsa_get_buffer_size,
    alsa_get_latency,
    alsa_get_current_padding,
};

extern unixlib_handle_t alsa_handle;

#define ALSA_CALL(func, params) __wine_unix_call(alsa_handle, alsa_ ## func, params)
