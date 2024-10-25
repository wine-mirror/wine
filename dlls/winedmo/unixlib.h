/*
 * Copyright 2024 Rémi Bernon for CodeWeavers
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

#ifndef __WINE_WINEDMO_UNIXLIB_H
#define __WINE_WINEDMO_UNIXLIB_H

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "ntuser.h"

#include "wine/unixlib.h"
#include "wine/winedmo.h"

struct process_attach_params
{
    UINT64 seek_callback;
    UINT64 read_callback;
};

struct stream_context
{
    UINT64 stream; /* client-side stream handle */
    UINT64 length; /* total length of the stream */
    UINT64 position; /* current position in the stream */

    UINT32 capacity; /* total allocated capacity for the buffer */
    UINT32 size; /* current data size in the buffer */
    BYTE buffer[];
};

C_ASSERT( sizeof(struct stream_context) == offsetof( struct stream_context, buffer[0] ) );

struct seek_callback_params
{
    struct dispatch_callback_params dispatch;
    UINT64 context;
    INT64 offset;
};

struct read_callback_params
{
    struct dispatch_callback_params dispatch;
    UINT64 context;
    INT32 size;
};


enum sample_flag
{
    SAMPLE_FLAG_SYNC_POINT = 1,
    SAMPLE_FLAG_INCOMPLETE = 2,
};

struct sample
{
    UINT32 flags;
    INT64 dts;
    INT64 pts;
    UINT64 duration;
    UINT64 size;
    UINT64 data; /* pointer to user memory */
};


struct media_type
{
    GUID major;
    UINT32 format_size;
    union
    {
        void *format;
        WAVEFORMATEX *audio;
        MFVIDEOFORMAT *video;
        UINT64 __pad;
    };
};


struct demuxer_check_params
{
    char mime_type[256];
};

struct demuxer_create_params
{
    const char *url;
    struct stream_context *context;
    struct winedmo_demuxer demuxer;
    char mime_type[256];
    UINT32 stream_count;
    INT64 duration;
};

struct demuxer_destroy_params
{
    struct winedmo_demuxer demuxer;
    struct stream_context *context;
};

struct demuxer_read_params
{
    struct winedmo_demuxer demuxer;
    UINT32 stream;
    struct sample sample;
};

struct demuxer_seek_params
{
    struct winedmo_demuxer demuxer;
    INT64 timestamp;
};

struct demuxer_stream_lang_params
{
    struct winedmo_demuxer demuxer;
    UINT32 stream;
    char buffer[32];
};

struct demuxer_stream_name_params
{
    struct winedmo_demuxer demuxer;
    UINT32 stream;
    char buffer[256];
};

struct demuxer_stream_type_params
{
    struct winedmo_demuxer demuxer;
    UINT32 stream;
    struct media_type media_type;
};


enum unix_funcs
{
    unix_process_attach,

    unix_demuxer_check,
    unix_demuxer_create,
    unix_demuxer_destroy,
    unix_demuxer_read,
    unix_demuxer_seek,
    unix_demuxer_stream_lang,
    unix_demuxer_stream_name,
    unix_demuxer_stream_type,

    unix_funcs_count,
};

#define UNIX_CALL( func, params ) (__wine_unixlib_handle ? WINE_UNIX_CALL( unix_##func, params ) : STATUS_PROCEDURE_NOT_FOUND)

#endif /* __WINE_WINEDMO_UNIXLIB_H */
