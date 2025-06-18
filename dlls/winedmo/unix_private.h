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

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#include <stdint.h>

#ifdef HAVE_FFMPEG
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#ifdef HAVE_LIBAVCODEC_BSF_H
# include <libavcodec/bsf.h>
#endif
#else
typedef struct AVCodecParameters AVCodecParameters;
typedef struct AVRational AVRational;
#endif /* HAVE_FFMPEG */

#include "unixlib.h"
#include "wine/debug.h"

/* unixlib.c */
extern int64_t unix_seek_callback( void *opaque, int64_t offset, int whence );
extern int unix_read_callback( void *opaque, uint8_t *buffer, int size );

/* unix_demuxer.c */
extern NTSTATUS demuxer_check( void * );
extern NTSTATUS demuxer_create( void * );
extern NTSTATUS demuxer_destroy( void * );
extern NTSTATUS demuxer_read( void * );
extern NTSTATUS demuxer_seek( void * );
extern NTSTATUS demuxer_stream_lang( void * );
extern NTSTATUS demuxer_stream_name( void * );
extern NTSTATUS demuxer_stream_type( void * );

/* unix_media_type.c */
extern NTSTATUS media_type_from_codec_params( const AVCodecParameters *params, const AVRational *sar, const AVRational *fps,
                                              UINT32 align, struct media_type *media_type );
