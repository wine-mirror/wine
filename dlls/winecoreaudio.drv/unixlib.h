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

struct coreaudio_stream /* To be made private */
{
    OSSpinLock lock;
    AudioComponentInstance unit;
    AudioConverterRef converter;
    AudioStreamBasicDescription dev_desc; /* audio unit format, not necessarily the same as fmt */
    AudioDeviceID dev_id;
    EDataFlow flow;
    AUDCLNT_SHAREMODE share;

    BOOL playing;
    SIZE_T local_buffer_size, tmp_buffer_size;
    UINT32 period_ms, period_frames;
    UINT32 bufsize_frames, resamp_bufsize_frames;
    UINT32 lcl_offs_frames, held_frames, wri_offs_frames, tmp_buffer_frames;
    UINT32 cap_bufsize_frames, cap_offs_frames, cap_held_frames;
    UINT32 wrap_bufsize_frames;
    UINT64 written_frames;
    INT32 getbuf_last;
    WAVEFORMATEX *fmt;
    BYTE *local_buffer, *cap_buffer, *wrap_buffer, *resamp_buffer, *tmp_buffer;
};

struct endpoint
{
    WCHAR *name;
    DWORD id;
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

enum unix_funcs
{
    unix_get_endpoint_ids,
};

extern unixlib_handle_t coreaudio_handle;

#define UNIX_CALL( func, params ) __wine_unix_call( coreaudio_handle, unix_ ## func, params )
