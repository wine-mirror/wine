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
    return wine_dbg_sprintf( "%d:%d", ratio->Numerator, ratio->Denominator );
}

static inline const char *debugstr_area( const MFVideoArea *area )
{
    return wine_dbg_sprintf( "(%d,%d)-(%d,%d)", area->OffsetX.value, area->OffsetY.value,
                             area->Area.cx, area->Area.cy );
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

static UINT wave_format_tag_from_codec_id( enum AVCodecID id )
{
    const struct AVCodecTag *table[] = {avformat_get_riff_audio_tags(), avformat_get_mov_audio_tags(), 0};
    return av_codec_get_tag( table, id );
}

static void wave_format_ex_init( const AVCodecParameters *params, WAVEFORMATEX *format, UINT32 format_size, WORD format_tag )
{
    memset( format, 0, format_size );
    format->cbSize = format_size - sizeof(*format);
    format->wFormatTag = format_tag;
#if LIBAVUTIL_VERSION_MAJOR >= 58
    format->nChannels = params->ch_layout.nb_channels;
#else
    format->nChannels = params->channels;
#endif
    format->nSamplesPerSec = params->sample_rate;
    format->wBitsPerSample = av_get_bits_per_sample( params->codec_id );
    if (!format->wBitsPerSample) format->wBitsPerSample = params->bits_per_coded_sample;
    if (!(format->nBlockAlign = params->block_align)) format->nBlockAlign = format->wBitsPerSample * format->nChannels / 8;
    if (!(format->nAvgBytesPerSec = params->bit_rate / 8)) format->nAvgBytesPerSec = format->nSamplesPerSec * format->nBlockAlign;
}

static NTSTATUS wave_format_extensible_from_codec_params( const AVCodecParameters *params, WAVEFORMATEXTENSIBLE *format, UINT32 *format_size,
                                                          UINT wave_format_size, const GUID *subtype, UINT64 channel_mask )
{
    UINT32 capacity = *format_size;

    *format_size = max( wave_format_size, sizeof(*format) + params->extradata_size );
    if (*format_size > capacity) return STATUS_BUFFER_TOO_SMALL;

    wave_format_ex_init( params, &format->Format, *format_size, WAVE_FORMAT_EXTENSIBLE );
    if (params->extradata_size && params->extradata) memcpy( format + 1, params->extradata, params->extradata_size );
    format->Samples.wValidBitsPerSample = 0;
    format->dwChannelMask = channel_mask;
    format->SubFormat = *subtype;

    TRACE( "tag %#x, %u channels, sample rate %u, %u bytes/sec, alignment %u, %u bits/sample, %u valid bps,"
           " channel mask %#x, subtype %s (%s).\n", format->Format.wFormatTag, format->Format.nChannels,
           format->Format.nSamplesPerSec, format->Format.nAvgBytesPerSec, format->Format.nBlockAlign,
           format->Format.wBitsPerSample, format->Samples.wValidBitsPerSample, format->dwChannelMask,
           debugstr_guid(&format->SubFormat), debugstr_fourcc(format->SubFormat.Data1) );
    if (format->Format.cbSize)
    {
        UINT extra_size = sizeof(WAVEFORMATEX) + format->Format.cbSize - sizeof(WAVEFORMATEXTENSIBLE);
        TRACE_HEXDUMP( format + 1, extra_size );
    }

    return STATUS_SUCCESS;
}

static NTSTATUS wave_format_ex_from_codec_params( const AVCodecParameters *params, WAVEFORMATEX *format, UINT32 *format_size,
                                                  UINT32 wave_format_size, WORD format_tag )
{
    UINT32 capacity = *format_size;

    *format_size = max( wave_format_size, sizeof(*format) + params->extradata_size );
    if (*format_size > capacity) return STATUS_BUFFER_TOO_SMALL;

    wave_format_ex_init( params, format, *format_size, format_tag );
    if (params->extradata_size && params->extradata) memcpy( format + 1, params->extradata, params->extradata_size );

    TRACE( "tag %#x, %u channels, sample rate %u, %u bytes/sec, alignment %u, %u bits/sample.\n",
           format->wFormatTag, format->nChannels, format->nSamplesPerSec, format->nAvgBytesPerSec,
           format->nBlockAlign, format->wBitsPerSample );
    if (format->cbSize) TRACE_HEXDUMP( format + 1, format->cbSize );

    return STATUS_SUCCESS;
}

static NTSTATUS heaac_wave_format_from_codec_params( const AVCodecParameters *params, HEAACWAVEINFO *format, UINT32 *format_size )
{
    UINT32 capacity = *format_size;

    *format_size = sizeof(*format) + params->extradata_size;
    if (*format_size > capacity) return STATUS_BUFFER_TOO_SMALL;

    wave_format_ex_init( params, &format->wfx, *format_size, WAVE_FORMAT_MPEG_HEAAC );
    if (params->extradata_size && params->extradata) memcpy( format + 1, params->extradata, params->extradata_size );
    format->wPayloadType = 0;
    format->wAudioProfileLevelIndication = 0;
    format->wStructType = 0;

    TRACE( "tag %#x, %u channels, sample rate %u, %u bytes/sec, alignment %u, %u bits/sample, payload %#x, "
           "level %#x, struct %#x.\n", format->wfx.wFormatTag, format->wfx.nChannels, format->wfx.nSamplesPerSec,
           format->wfx.nAvgBytesPerSec, format->wfx.nBlockAlign, format->wfx.wBitsPerSample, format->wPayloadType,
           format->wAudioProfileLevelIndication, format->wStructType );
    if (format->wfx.cbSize) TRACE_HEXDUMP( format + 1, sizeof(WAVEFORMATEX) + format->wfx.cbSize - sizeof(*format) );

    return STATUS_SUCCESS;
}

static NTSTATUS audio_format_from_codec_params( const AVCodecParameters *params, void *format, UINT32 *format_size )
{
    UINT format_tag, wave_format_size = sizeof(WAVEFORMATEX);
    UINT64 channel_mask;
    int channels;

    if (params->codec_id == AV_CODEC_ID_AAC) return heaac_wave_format_from_codec_params( params, format, format_size );
    if (params->codec_id == AV_CODEC_ID_MP1) wave_format_size = sizeof(MPEG1WAVEFORMAT);
    if (params->codec_id == AV_CODEC_ID_MP3) wave_format_size = sizeof(MPEGLAYER3WAVEFORMAT);
    if (params->codec_id == AV_CODEC_ID_WMAV1) wave_format_size = sizeof(MSAUDIO1WAVEFORMAT);
    if (params->codec_id == AV_CODEC_ID_WMAV2) wave_format_size = sizeof(WMAUDIO2WAVEFORMAT);
    if (params->codec_id == AV_CODEC_ID_WMAPRO) wave_format_size = sizeof(WMAUDIO3WAVEFORMAT);
    if (params->codec_id == AV_CODEC_ID_WMAVOICE) wave_format_size = sizeof(WMAUDIO3WAVEFORMAT);
    if (params->codec_id == AV_CODEC_ID_WMALOSSLESS) wave_format_size = sizeof(WMAUDIO3WAVEFORMAT);

#if LIBAVUTIL_VERSION_MAJOR >= 58
    if (!(channels = params->ch_layout.nb_channels)) channels = 1;
    if (params->ch_layout.order != AV_CHANNEL_ORDER_NATIVE) channel_mask = 0;
    else channel_mask = params->ch_layout.u.mask;
#else
    if (!(channels = params->channels)) channels = 1;
    channel_mask = params->channel_layout;
#endif

    format_tag = wave_format_tag_from_codec_id( params->codec_id );
    if (format_tag == WAVE_FORMAT_EXTENSIBLE || format_tag >> 16 || (channels > 2 && channel_mask != 0))
    {
        GUID subtype = MFAudioFormat_Base;
        subtype.Data1 = format_tag;
        wave_format_size += sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
        return wave_format_extensible_from_codec_params( params, format, format_size, wave_format_size,
                                                         &subtype, channel_mask );
    }

    return wave_format_ex_from_codec_params( params, format, format_size, wave_format_size, format_tag );
}

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
    case AV_PIX_FMT_P010: return MFVideoFormat_P010;
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
        if (params->codec_id == AV_CODEC_ID_MPEG1VIDEO) format->guidFormat = MEDIASUBTYPE_MPEG1Payload;
        else if (params->codec_id == AV_CODEC_ID_H264) format->guidFormat.Data1 = MFVideoFormat_H264.Data1;
        else if (params->codec_id == AV_CODEC_ID_VP9) format->guidFormat.Data1 = MFVideoFormat_VP90.Data1;
        else if (params->codec_tag) format->guidFormat.Data1 = params->codec_tag;
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
           debugstr_fourcc(format->guidFormat.Data1), format->videoInfo.dwWidth, format->videoInfo.dwHeight,
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
        return audio_format_from_codec_params( params, media_type->audio, &media_type->format_size );
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
