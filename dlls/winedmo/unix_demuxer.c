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
#include "unix_private.h"

#include "wine/debug.h"

#ifdef HAVE_FFMPEG

WINE_DEFAULT_DEBUG_CHANNEL(dmo);

static inline const char *debugstr_averr( int err )
{
    return wine_dbg_sprintf( "%d (%s)", err, av_err2str(err) );
}

struct stream
{
    AVBSFContext *filter;
    BOOL eos;
};

struct demuxer
{
    AVFormatContext *ctx;
    struct stream *streams;

    AVPacket *last_packet; /* last read packet */
    struct stream *last_stream; /* last read packet stream */
};

static struct demuxer *get_demuxer( struct winedmo_demuxer demuxer )
{
    return (struct demuxer *)(UINT_PTR)demuxer.handle;
}

static INT64 get_user_time( INT64 time, AVRational time_base )
{
    static const AVRational USER_TIME_BASE_Q = {1, 10000000};
    return av_rescale_q_rnd( time, time_base, USER_TIME_BASE_Q, AV_ROUND_PASS_MINMAX );
}

static INT64 get_stream_time( const AVStream *stream, INT64 time )
{
    if (stream->time_base.num && stream->time_base.den) return get_user_time( time, stream->time_base );
    return get_user_time( time, AV_TIME_BASE_Q );
}

static INT64 get_context_duration( const AVFormatContext *ctx )
{
    INT64 i, max_duration = AV_NOPTS_VALUE;

    for (i = 0; i < ctx->nb_streams; i++)
    {
        const AVStream *stream = ctx->streams[i];
        INT64 duration = get_stream_time( stream, stream->duration );
        if (duration == AV_NOPTS_VALUE) continue;
        if (duration >= max_duration) max_duration = duration;
        if (max_duration == AV_NOPTS_VALUE) max_duration = duration;
    }

    return max_duration;
}

NTSTATUS demuxer_check( void *arg )
{
    struct demuxer_check_params *params = arg;
    const AVInputFormat *format = NULL;

    if (!strcmp( params->mime_type, "video/mp4" )) format = av_find_input_format( "mp4" );
    else if (!strcmp( params->mime_type, "video/avi" )) format = av_find_input_format( "avi" );
    else if (!strcmp( params->mime_type, "audio/wav" )) format = av_find_input_format( "wav" );
    else if (!strcmp( params->mime_type, "audio/x-ms-wma" )) format = av_find_input_format( "asf" );
    else if (!strcmp( params->mime_type, "video/x-ms-wmv" )) format = av_find_input_format( "asf" );
    else if (!strcmp( params->mime_type, "video/x-ms-asf" )) format = av_find_input_format( "asf" );
    else if (!strcmp( params->mime_type, "video/mpeg" )) format = av_find_input_format( "mpeg" );
    else if (!strcmp( params->mime_type, "audio/mp3" )) format = av_find_input_format( "mp3" );

    if (format) TRACE( "Found format %s (%s)\n", format->name, format->long_name );
    else FIXME( "Unsupported MIME type %s\n", debugstr_a(params->mime_type) );

    return format ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED;
}

static NTSTATUS demuxer_create_streams( struct demuxer *demuxer )
{
    UINT i;

    for (i = 0; i < demuxer->ctx->nb_streams; i++)
    {
        AVCodecParameters *par = demuxer->ctx->streams[i]->codecpar;
        struct stream *stream = demuxer->streams + i;
        const AVBitStreamFilter *filter;

        if (par->codec_id == AV_CODEC_ID_H264)
        {
            if (!(filter = av_bsf_get_by_name( "h264_mp4toannexb" )))
                ERR( "Failed to find H264 bitstream filter\n" );
            else
            {
                if (av_bsf_alloc( filter, &stream->filter ) < 0) return STATUS_UNSUCCESSFUL;
                avcodec_parameters_copy( stream->filter->par_in, par );
                av_bsf_init( stream->filter );
                continue;
            }
        }

        av_bsf_get_null_filter( &stream->filter );
        avcodec_parameters_copy( stream->filter->par_in, demuxer->ctx->streams[i]->codecpar );
        avcodec_parameters_copy( stream->filter->par_out, demuxer->ctx->streams[i]->codecpar );
    }

    return STATUS_SUCCESS;
}

NTSTATUS demuxer_create( void *arg )
{
    struct demuxer_create_params *params = arg;
    const char *ext = params->url ? strrchr( params->url, '.' ) : "";
    const AVInputFormat *format;
    struct demuxer *demuxer;
    int i, ret;

    TRACE( "context %p, url %s, mime %s\n", params->context, debugstr_a(params->url), debugstr_a(params->mime_type) );

    if (!(demuxer = calloc( 1, sizeof(*demuxer) ))) return STATUS_NO_MEMORY;
    if (!(demuxer->ctx = avformat_alloc_context())) goto failed;
    if (!(demuxer->ctx->pb = avio_alloc_context( NULL, 0, 0, params->context, unix_read_callback, NULL, unix_seek_callback ))) goto failed;

    if ((ret = avformat_open_input( &demuxer->ctx, NULL, NULL, NULL )) < 0)
    {
        ERR( "Failed to open input, error %s.\n", debugstr_averr(ret) );
        goto failed;
    }
    format = demuxer->ctx->iformat;

    if ((params->duration = get_context_duration( demuxer->ctx )) == AV_NOPTS_VALUE ||
        strstr( format->name, "mp3" ))
    {
        if ((ret = avformat_find_stream_info( demuxer->ctx, NULL )) < 0)
        {
            ERR( "Failed to find stream info, error %s.\n", debugstr_averr(ret) );
            goto failed;
        }
        params->duration = get_context_duration( demuxer->ctx );
    }
    if (!(demuxer->streams = calloc( demuxer->ctx->nb_streams, sizeof(*demuxer->streams) ))) goto failed;
    if (demuxer_create_streams( demuxer )) goto failed;

    params->demuxer.handle = (UINT_PTR)demuxer;
    params->stream_count = demuxer->ctx->nb_streams;
    if (strstr( format->name, "mp4" )) strcpy( params->mime_type, "video/mp4" );
    else if (strstr( format->name, "avi" )) strcpy( params->mime_type, "video/avi" );
    else if (strstr( format->name, "mpeg" )) strcpy( params->mime_type, "video/mpeg" );
    else if (strstr( format->name, "mp3" )) strcpy( params->mime_type, "audio/mp3" );
    else if (strstr( format->name, "wav" )) strcpy( params->mime_type, "audio/wav" );
    else if (strstr( format->name, "asf" ))
    {
        if (!strcmp( ext, ".wma" )) strcpy( params->mime_type, "audio/x-ms-wma" );
        else if (!strcmp( ext, ".wmv" )) strcpy( params->mime_type, "video/x-ms-wmv" );
        else strcpy( params->mime_type, "video/x-ms-asf" );
    }
    else
    {
        FIXME( "Unknown MIME type for format %s, url %s\n", debugstr_a(format->name), debugstr_a(params->url) );
        strcpy( params->mime_type, "video/x-application" );
    }

    return STATUS_SUCCESS;

failed:
    if (demuxer->ctx)
    {
        avio_context_free( &demuxer->ctx->pb );
        avformat_free_context( demuxer->ctx );
    }
    for (i = 0; demuxer->streams && i < demuxer->ctx->nb_streams; i++)
        av_bsf_free( &demuxer->streams[i].filter );
    free( demuxer->streams );
    free( demuxer );
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS demuxer_destroy( void *arg )
{
    struct demuxer_destroy_params *params = arg;
    struct demuxer *demuxer = get_demuxer( params->demuxer );
    int i;

    TRACE( "demuxer %p\n", demuxer );

    params->context = demuxer->ctx->pb->opaque;
    avio_context_free( &demuxer->ctx->pb );
    avformat_free_context( demuxer->ctx );
    for (i = 0; i < demuxer->ctx->nb_streams; i++)
        av_bsf_free( &demuxer->streams[i].filter );
    free( demuxer->streams );
    free( demuxer );

    return STATUS_SUCCESS;
}

static NTSTATUS demuxer_filter_packet( struct demuxer *demuxer, AVPacket **packet )
{
    struct stream *stream;
    int i, ret;

    do
    {
        if ((*packet = demuxer->last_packet)) return STATUS_SUCCESS;
        if (!(*packet = av_packet_alloc())) return STATUS_NO_MEMORY;

        if (!(stream = demuxer->last_stream)) ret = 0;
        else
        {
            if (!(ret = av_bsf_receive_packet( stream->filter, *packet ))) return STATUS_SUCCESS;
            if (ret == AVERROR_EOF) stream->eos = TRUE;
            if (!ret || ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) ret = 0;
            else WARN( "Failed to read packet from filter, error %s.\n", debugstr_averr( ret ) );
            stream = demuxer->last_stream = NULL;
        }

        if (!ret && !(ret = av_read_frame( demuxer->ctx, *packet )))
        {
            stream = demuxer->streams + (*packet)->stream_index;
            ret = av_bsf_send_packet( stream->filter, (*packet) );
            if (ret < 0) WARN( "Failed to send packet to filter, error %s.\n", debugstr_averr( ret ) );
            else demuxer->last_stream = stream;
        }
        av_packet_free( packet );

        if (ret == AVERROR_EOF)
        {
            for (i = 0; ret == AVERROR_EOF && i < demuxer->ctx->nb_streams; i++)
            {
                if (demuxer->streams[i].eos) continue;
                stream = demuxer->streams + i;
                ret = av_bsf_send_packet( stream->filter, NULL );
                if (ret < 0) WARN( "Failed to send packet to filter, error %s.\n", debugstr_averr( ret ) );
                else demuxer->last_stream = stream;
            }

            if (ret == AVERROR_EOF) return STATUS_END_OF_FILE;
        }
    } while (!ret || ret == AVERROR(EAGAIN));

    ERR( "Failed to read packet from demuxer %p, error %s.\n", demuxer, debugstr_averr( ret ) );
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS demuxer_read( void *arg )
{
    struct demuxer_read_params *params = arg;
    struct demuxer *demuxer = get_demuxer( params->demuxer );
    struct sample *sample = &params->sample;
    UINT capacity = params->sample.size;
    AVStream *stream;
    AVPacket *packet;
    NTSTATUS status;

    TRACE( "demuxer %p, capacity %#x\n", demuxer, capacity );

    if ((status = demuxer_filter_packet( demuxer, &packet ))) return status;

    params->sample.size = packet->size;
    if ((capacity < packet->size))
    {
        demuxer->last_packet = packet;
        return STATUS_BUFFER_TOO_SMALL;
    }

    stream = demuxer->ctx->streams[packet->stream_index];
    sample->pts = get_stream_time( stream, packet->pts );
    sample->dts = get_stream_time( stream, packet->dts );
    sample->duration = get_stream_time( stream, packet->duration );
    if (packet->flags & AV_PKT_FLAG_KEY) sample->flags |= SAMPLE_FLAG_SYNC_POINT;
    memcpy( (void *)(UINT_PTR)sample->data, packet->data, packet->size );
    params->stream = packet->stream_index;
    av_packet_free( &packet );
    demuxer->last_packet = NULL;

    return STATUS_SUCCESS;
}

NTSTATUS demuxer_seek( void *arg )
{
    struct demuxer_seek_params *params = arg;
    struct demuxer *demuxer = get_demuxer( params->demuxer );
    int64_t timestamp = params->timestamp * AV_TIME_BASE / 10000000;
    int i, ret;

    TRACE( "demuxer %p, timestamp 0x%s\n", demuxer, wine_dbgstr_longlong( params->timestamp ) );

    if ((ret = av_seek_frame( demuxer->ctx, -1, timestamp, AVSEEK_FLAG_ANY )) < 0)
    {
        ERR( "Failed to seek demuxer %p, error %s.\n", demuxer, debugstr_averr(ret) );
        return STATUS_UNSUCCESSFUL;
    }

    for (i = 0; i < demuxer->ctx->nb_streams; i++)
    {
        av_bsf_flush( demuxer->streams[i].filter );
        demuxer->streams[i].eos = FALSE;
    }
    av_packet_free( &demuxer->last_packet );
    demuxer->last_stream = NULL;

    return STATUS_SUCCESS;
}

NTSTATUS demuxer_stream_lang( void *arg )
{
    struct demuxer_stream_lang_params *params = arg;
    struct demuxer *demuxer = get_demuxer( params->demuxer );
    AVStream *stream = demuxer->ctx->streams[params->stream];
    AVDictionaryEntry *tag;

    TRACE( "demuxer %p, stream %u\n", demuxer, params->stream );

    if (!(tag = av_dict_get( stream->metadata, "language", NULL, AV_DICT_IGNORE_SUFFIX )))
        return STATUS_NOT_FOUND;

    lstrcpynA( params->buffer, tag->value, ARRAY_SIZE( params->buffer ) );
    return STATUS_SUCCESS;
}

NTSTATUS demuxer_stream_name( void *arg )
{
    struct demuxer_stream_name_params *params = arg;
    struct demuxer *demuxer = get_demuxer( params->demuxer );
    AVStream *stream = demuxer->ctx->streams[params->stream];
    AVDictionaryEntry *tag;

    TRACE( "demuxer %p, stream %u\n", demuxer, params->stream );

    if (!(tag = av_dict_get( stream->metadata, "title", NULL, AV_DICT_IGNORE_SUFFIX )))
        return STATUS_NOT_FOUND;

    lstrcpynA( params->buffer, tag->value, ARRAY_SIZE( params->buffer ) );
    return STATUS_SUCCESS;
}

NTSTATUS demuxer_stream_type( void *arg )
{
    struct demuxer_stream_type_params *params = arg;
    struct demuxer *demuxer = get_demuxer( params->demuxer );
    AVStream *stream = demuxer->ctx->streams[params->stream];
    AVCodecParameters *par = demuxer->streams[params->stream].filter->par_out;

    TRACE( "demuxer %p, stream %u, stream %p, index %u\n", demuxer, params->stream, stream, stream->index );

    return media_type_from_codec_params( par, &stream->sample_aspect_ratio,
                                         &stream->avg_frame_rate, 0, &params->media_type );
}

#endif /* HAVE_FFMPEG */
