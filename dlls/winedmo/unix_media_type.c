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

#include "initguid.h"
#include "unix_private.h"

#include "d3d9.h"
#include "mfapi.h"
#include "uuids.h"

#include "wine/debug.h"

#ifdef HAVE_FFMPEG

WINE_DEFAULT_DEBUG_CHANNEL(dmo);

DEFINE_MEDIATYPE_GUID( MFVideoFormat_ABGR32, D3DFMT_A8B8G8R8 );

static inline const char *debugstr_ratio( const MFRatio *ratio )
{
    return wine_dbg_sprintf( "%d:%d", (int)ratio->Numerator, (int)ratio->Denominator );
}

static inline const char *debugstr_area( const MFVideoArea *area )
{
    return wine_dbg_sprintf( "(%d,%d)-(%d,%d)", area->OffsetX.value, area->OffsetY.value,
                             (int)area->Area.cx, (int)area->Area.cy );
}

#define TRACE_HEXDUMP( data, size )                                                               \
    if (__WINE_IS_DEBUG_ON(_TRACE, __wine_dbch___default))                                        \
    do {                                                                                          \
        const unsigned char *__ptr, *__end, *__tmp;                                               \
        for (__ptr = (void *)(data), __end = __ptr + (size); __ptr < __end; __ptr += 16)          \
        {                                                                                         \
            char __buf[256], *__lim = __buf + sizeof(__buf), *__out = __buf;                      \
            __out += snprintf( __out, __lim - __out, "%08zx ", (char *)__ptr - (char *)data );    \
            for (__tmp = __ptr; __tmp < __end && __tmp < __ptr + 16; ++__tmp)                     \
                __out += snprintf( __out, __lim - __out, " %02x", *__tmp );                       \
            memset( __out, ' ', (__ptr + 16 - __tmp) * 3 + 2 );                                   \
            __out += (__ptr + 16 - __tmp) * 3 + 2;                                                \
            for (__tmp = __ptr; __tmp < __end && __tmp < __ptr + 16; ++__tmp)                     \
                *__out++ = *__tmp >= ' ' && *__tmp <= '~' ? *__tmp : '.';                         \
            *__out++ = 0;                                                                         \
            TRACE( "%s\n", __buf );                                                               \
        }                                                                                         \
    } while (0)

static GUID subtype_from_pixel_format( enum AVPixelFormat fmt )
{
    switch (fmt)
    {
    case AV_PIX_FMT_NONE: return MFVideoFormat_Base;
    case AV_PIX_FMT_YUV420P: return MFVideoFormat_I420;
    case AV_PIX_FMT_YUVJ420P: return MFVideoFormat_I420;
    case AV_PIX_FMT_YUYV422: return MFVideoFormat_YUY2;
    case AV_PIX_FMT_UYVY422: return MFVideoFormat_UYVY;
    case AV_PIX_FMT_BGR0: return MFVideoFormat_RGB32;
    case AV_PIX_FMT_RGBA: return MFVideoFormat_ABGR32;
    case AV_PIX_FMT_BGRA: return MFVideoFormat_ARGB32;
    case AV_PIX_FMT_BGR24: return MFVideoFormat_RGB24;
    case AV_PIX_FMT_NV12: return MFVideoFormat_NV12;
    case AV_PIX_FMT_NV21: return MFVideoFormat_NV21;
    case AV_PIX_FMT_RGB565: return MFVideoFormat_RGB565;
    case AV_PIX_FMT_RGB555: return MFVideoFormat_RGB555;
    case AV_PIX_FMT_RGB8: return MFVideoFormat_RGB8;
    default:
        FIXME( "Unsupported format %#x (%s)\n", fmt, av_get_pix_fmt_name( fmt ) );
        return MFVideoFormat_Base;
    }
}

static UINT video_format_tag_from_codec_id( enum AVCodecID id )
{
    const struct AVCodecTag *table[] = {avformat_get_riff_video_tags(), avformat_get_mov_video_tags(), 0};
    return av_codec_get_tag( table, id );
}

static void mf_video_format_init( const AVCodecParameters *params, MFVIDEOFORMAT *format, UINT32 format_size,
                                  const AVRational *sar, const AVRational *fps, UINT32 align )
{
    memset( format, 0, format_size );
    format->dwSize = format_size;

    if (params->codec_id == AV_CODEC_ID_RAWVIDEO && params->format != AV_PIX_FMT_NONE)
        format->guidFormat = subtype_from_pixel_format( params->format );
    else
    {
        format->guidFormat = MFVideoFormat_Base;
        if (params->codec_tag) format->guidFormat.Data1 = params->codec_tag;
        else format->guidFormat.Data1 = video_format_tag_from_codec_id( params->codec_id );
    }

    format->videoInfo.dwWidth = (params->width + align) & ~align;
    format->videoInfo.dwHeight = (params->height + align) & ~align;
    if (format->videoInfo.dwWidth != params->width || format->videoInfo.dwHeight != params->height)
    {
        format->videoInfo.MinimumDisplayAperture.Area.cx = params->width;
        format->videoInfo.MinimumDisplayAperture.Area.cy = params->height;
    }
    format->videoInfo.GeometricAperture = format->videoInfo.MinimumDisplayAperture;
    format->videoInfo.PanScanAperture = format->videoInfo.MinimumDisplayAperture;

    if (sar->num && sar->den)
    {
        format->videoInfo.PixelAspectRatio.Numerator = sar->num;
        format->videoInfo.PixelAspectRatio.Denominator = sar->den;
    }
    else
    {
        format->videoInfo.PixelAspectRatio.Numerator = 1;
        format->videoInfo.PixelAspectRatio.Denominator = 1;
    }

    if (fps->num && fps->den)
    {
        format->videoInfo.FramesPerSecond.Numerator = fps->num;
        format->videoInfo.FramesPerSecond.Denominator = fps->den;
    }
}

static NTSTATUS video_format_from_codec_params( const AVCodecParameters *params, MFVIDEOFORMAT *format, UINT32 *format_size,
                                                const AVRational *sar, const AVRational *fps, UINT32 align )
{
    UINT32 capacity = *format_size;

    *format_size = sizeof(*format) + params->extradata_size;
    if (*format_size > capacity) return STATUS_BUFFER_TOO_SMALL;

    mf_video_format_init( params, format, *format_size, sar, fps, align );
    if (params->extradata_size && params->extradata) memcpy( format + 1, params->extradata, params->extradata_size );

    TRACE( "subtype %s (%s) %ux%u, FPS %s, aperture %s, PAR %s, videoFlags %#x.\n", debugstr_guid(&format->guidFormat),
           debugstr_fourcc(format->guidFormat.Data1), (int)format->videoInfo.dwWidth, (int)format->videoInfo.dwHeight,
           debugstr_ratio(&format->videoInfo.FramesPerSecond), debugstr_area(&format->videoInfo.MinimumDisplayAperture),
           debugstr_ratio(&format->videoInfo.PixelAspectRatio), (int)format->videoInfo.VideoFlags );
    if (format->dwSize > sizeof(*format)) TRACE_HEXDUMP( format + 1, format->dwSize - sizeof(*format) );

    return STATUS_SUCCESS;
}

NTSTATUS media_type_from_codec_params( const AVCodecParameters *params, const AVRational *sar, const AVRational *fps,
                                       UINT32 align, struct media_type *media_type )
{
    TRACE( "codec type %#x, id %#x (%s), tag %#x (%s)\n", params->codec_type, params->codec_id, avcodec_get_name(params->codec_id),
           params->codec_tag, debugstr_fourcc(params->codec_tag) );
    if (params->extradata_size) TRACE_HEXDUMP( params->extradata, params->extradata_size );

    if (params->codec_type == AVMEDIA_TYPE_AUDIO)
    {
        media_type->major = MFMediaType_Audio;
        return STATUS_SUCCESS;
    }

    if (params->codec_type == AVMEDIA_TYPE_VIDEO)
    {
        media_type->major = MFMediaType_Video;
        return video_format_from_codec_params( params, media_type->video, &media_type->format_size, sar, fps, align );
    }

    FIXME( "Unknown type %#x\n", params->codec_type );
    return STATUS_NOT_IMPLEMENTED;
}

#endif /* HAVE_FFMPEG */
