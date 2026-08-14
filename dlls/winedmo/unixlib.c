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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#ifdef HAVE_FFMPEG

#include "unix_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmo);

static UINT64 seek_callback;
static UINT64 read_callback;

int64_t unix_seek_callback( void *opaque, int64_t offset, int whence )
{
    struct seek_callback_params params = {.dispatch = {.callback = seek_callback}, .context = (UINT_PTR)opaque};
    struct stream_context *context = opaque;
    void *ret_ptr;
    ULONG ret_len;
    int status;

    TRACE( "opaque %p, offset %#"PRIx64", whence %#x\n", opaque, offset, whence );

    if (whence == AVSEEK_SIZE) return context->length;
    if (whence == SEEK_END) offset += context->length;
    if (whence == SEEK_CUR) offset += context->position;
    if ((UINT64)offset > context->length) offset = context->length;

    if (offset / context->capacity != context->position / context->capacity)
    {
        for (;;)
        {
            /* seek stream to multiples of buffer capacity */
            params.offset = (offset / context->capacity) * context->capacity;
            status = KeUserDispatchCallback( &params.dispatch, sizeof(params), &ret_ptr, &ret_len );
            if (status || ret_len != sizeof(UINT64)) return AVERROR( EINVAL );
            if (*(UINT64 *)ret_ptr == params.offset) break;
            offset = *(UINT64 *)ret_ptr;
        }
        context->size = 0;
    }

    context->position = offset;
    return offset;
}

static int stream_context_read( struct stream_context *context )
{
    struct read_callback_params params = {.dispatch = {.callback = read_callback}, .context = (UINT_PTR)context};
    void *ret_ptr;
    ULONG ret_len;
    int status;

    params.size = context->capacity;
    status = KeUserDispatchCallback( &params.dispatch, sizeof(params), &ret_ptr, &ret_len );
    if (status || ret_len != sizeof(ULONG)) return AVERROR( EINVAL );
    context->size = *(ULONG *)ret_ptr;
    return 0;
}

int unix_read_callback( void *opaque, uint8_t *buffer, int size )
{
    struct stream_context *context = opaque;
    int ret, total = 0;

    if (!(size = min( (UINT64)size, context->length - context->position ))) return AVERROR_EOF;

    while (size)
    {
        int step, buffer_offset = context->position % context->capacity;

        if (!context->size && (ret = stream_context_read( context )) < 0) return ret;
        if (!(step = min( size, context->size - buffer_offset ))) break;
        memcpy( buffer, context->buffer + buffer_offset, step );
        buffer += step;
        total += step;
        size -= step;

        context->position += step;
        if (!(context->position % context->capacity)) context->size = 0;
    }

    if (!total) return AVERROR_EOF;
    return total;
}

static void vlog( void *ctx, int level, const char *fmt, va_list va_args )
{
    enum __wine_debug_class dbcl = __WINE_DBCL_TRACE;
    if (level <= AV_LOG_ERROR) dbcl = __WINE_DBCL_ERR;
    if (level <= AV_LOG_WARNING) dbcl = __WINE_DBCL_WARN;
    wine_dbg_vlog( dbcl, __wine_dbch___default, __func__, fmt, va_args );
}

static const char *debugstr_version( UINT version )
{
    return wine_dbg_sprintf("%u.%u.%u", AV_VERSION_MAJOR(version), AV_VERSION_MINOR(version), AV_VERSION_MICRO(version));
}

static NTSTATUS process_attach( void *arg )
{
    struct process_attach_params *params = arg;
    const AVInputFormat *demuxer;
    void *opaque;

    TRACE( "FFmpeg support:\n" );
    TRACE( "  avutil version %s\n", debugstr_version( avutil_version() ) );
    TRACE( "  avformat version %s\n", debugstr_version( avformat_version() ) );

    TRACE( "available demuxers:\n" );
    for (opaque = NULL; (demuxer = av_demuxer_iterate( &opaque ));)
    {
        TRACE( "  %s (%s)\n", demuxer->name, demuxer->long_name );
        if (demuxer->extensions) TRACE( "    extensions: %s\n", demuxer->extensions );
        if (demuxer->mime_type) TRACE( "    mime_types: %s\n", demuxer->mime_type );
    }

    av_log_set_callback( vlog );

    seek_callback = params->seek_callback;
    read_callback = params->read_callback;

    return STATUS_SUCCESS;
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
#define X( name ) [unix_##name] = name
    X( process_attach ),

    X( demuxer_check ),
    X( demuxer_create ),
    X( demuxer_destroy ),
    X( demuxer_read ),
    X( demuxer_seek ),
    X( demuxer_stream_lang ),
    X( demuxer_stream_name ),
    X( demuxer_stream_type ),
};

C_ASSERT(ARRAY_SIZE(__wine_unix_call_funcs) == unix_funcs_count);

#ifdef _WIN64

typedef ULONG PTR32;

static NTSTATUS wow64_demuxer_create( void *arg )
{
    struct
    {
        PTR32 url;
        PTR32 context;
        struct winedmo_demuxer demuxer;
        char mime_type[256];
        UINT32 stream_count;
        INT64 duration;
    } *params32 = arg;
    struct demuxer_create_params params;
    NTSTATUS status;

    params.url = UintToPtr( params32->url );
    params.context = UintToPtr( params32->context );
    if ((status = demuxer_create( &params ))) return status;
    params32->demuxer = params.demuxer;
    memcpy( params32->mime_type, params.mime_type, 256 );
    params32->stream_count = params.stream_count;
    params32->duration = params.duration;

    return status;
}

static NTSTATUS wow64_demuxer_destroy( void *arg )
{
    struct
    {
        struct winedmo_demuxer demuxer;
        PTR32 context;
    } *params32 = arg;
    struct demuxer_create_params params;
    NTSTATUS status;

    params.demuxer = params32->demuxer;
    if ((status = demuxer_destroy( &params ))) return status;
    params32->context = PtrToUint( params.context );

    return status;
}

const unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
#define X64( name ) [unix_##name] = wow64_##name
    X( process_attach ),

    X( demuxer_check ),
    X64( demuxer_create ),
    X64( demuxer_destroy ),
    X( demuxer_read ),
    X( demuxer_seek ),
    X( demuxer_stream_lang ),
    X( demuxer_stream_name ),
    X( demuxer_stream_type ),
};

C_ASSERT(ARRAY_SIZE(__wine_unix_call_wow64_funcs) == unix_funcs_count);

#endif /* _WIN64 */

#endif /* HAVE_FFMPEG */
