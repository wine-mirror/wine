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

#include <stdarg.h>
#include <stddef.h>

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/audio/audio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "windef.h"
#include "winbase.h"

#include "initguid.h"
#include "d3d9types.h"
#include "mfapi.h"
#include "wmcodecdsp.h"

#include "unix_private.h"

#define WG_GUID_FORMAT "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"
#define WG_GUID_ARGS(guid) (guid).Data1, (guid).Data2, (guid).Data3, (guid).Data4[0], \
            (guid).Data4[1], (guid).Data4[2], (guid).Data4[3], (guid).Data4[4], \
            (guid).Data4[5], (guid).Data4[6], (guid).Data4[7]

#define WG_RATIO_FORMAT "%d:%d"
#define WG_RATIO_ARGS(ratio) (ratio).Numerator, (ratio).Denominator

#define WG_APERTURE_FORMAT "(%d,%d)-(%d,%d)"
#define WG_APERTURE_ARGS(aperture) (aperture).OffsetX.value, (aperture).OffsetY.value, \
            (aperture).Area.cx, (aperture).Area.cy

static const GUID GUID_NULL;

DEFINE_MEDIATYPE_GUID(MFAudioFormat_RAW_AAC,WAVE_FORMAT_RAW_AAC1);
DEFINE_MEDIATYPE_GUID(MFAudioFormat_MSAudio1,WAVE_FORMAT_MSAUDIO1);

DEFINE_MEDIATYPE_GUID(MFVideoFormat_CVID,MAKEFOURCC('c','v','i','d'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_IV50,MAKEFOURCC('I','V','5','0'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_VC1S,MAKEFOURCC('V','C','1','S'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_ABGR32,D3DFMT_A8B8G8R8);

static void init_caps_codec_data(GstCaps *caps, const void *codec_data, int codec_data_size)
{
    GstBuffer *buffer;

    if (codec_data_size > 0 && (buffer = gst_buffer_new_and_alloc(codec_data_size)))
    {
        gst_buffer_fill(buffer, 0, codec_data, codec_data_size);
        gst_caps_set_simple(caps, "codec_data", GST_TYPE_BUFFER, buffer, NULL);
        gst_buffer_unref(buffer);
    }
}

static void init_caps_from_wave_format_mpeg1(GstCaps *caps, const MPEG1WAVEFORMAT *format, UINT32 format_size)
{
    init_caps_codec_data(caps, &format->wfx + 1, format->wfx.cbSize);

    gst_structure_remove_field(gst_caps_get_structure(caps, 0), "format");
    gst_structure_set_name(gst_caps_get_structure(caps, 0), "audio/mpeg");
    gst_caps_set_simple(caps, "mpegversion", G_TYPE_INT, 1, NULL);
    gst_caps_set_simple(caps, "layer", G_TYPE_INT, format->fwHeadLayer, NULL);
    gst_caps_set_simple(caps, "parsed", G_TYPE_BOOLEAN, TRUE, NULL);
}

static void init_caps_from_wave_format_mp3(GstCaps *caps, const MPEGLAYER3WAVEFORMAT *format, UINT32 format_size)
{
    init_caps_codec_data(caps, &format->wfx + 1, format->wfx.cbSize);

    gst_structure_remove_field(gst_caps_get_structure(caps, 0), "format");
    gst_structure_set_name(gst_caps_get_structure(caps, 0), "audio/mpeg");
    gst_caps_set_simple(caps, "mpegversion", G_TYPE_INT, 1, NULL);
    gst_caps_set_simple(caps, "layer", G_TYPE_INT, 3, NULL);
    gst_caps_set_simple(caps, "parsed", G_TYPE_BOOLEAN, TRUE, NULL);
}

static void init_caps_from_wave_format_aac(GstCaps *caps, const HEAACWAVEFORMAT *format, UINT32 format_size)
{
    init_caps_codec_data(caps, format->pbAudioSpecificConfig, format_size - sizeof(format->wfInfo));

    gst_structure_remove_field(gst_caps_get_structure(caps, 0), "format");
    gst_structure_set_name(gst_caps_get_structure(caps, 0), "audio/mpeg");
    gst_caps_set_simple(caps, "mpegversion", G_TYPE_INT, 4, NULL);

    switch (format->wfInfo.wPayloadType)
    {
    case 0: gst_caps_set_simple(caps, "stream-format", G_TYPE_STRING, "raw", NULL); break;
    case 1: gst_caps_set_simple(caps, "stream-format", G_TYPE_STRING, "adts", NULL); break;
    case 2: gst_caps_set_simple(caps, "stream-format", G_TYPE_STRING, "adif", NULL); break;
    case 3: gst_caps_set_simple(caps, "stream-format", G_TYPE_STRING, "loas", NULL); break;
    }

    /* FIXME: Use gst_codec_utils_aac_caps_set_level_and_profile from GStreamer pbutils library */
}

static void init_caps_from_wave_format_aac_raw(GstCaps *caps, const WAVEFORMATEX *format, UINT32 format_size)
{
    init_caps_codec_data(caps, format + 1, format->cbSize);

    gst_structure_remove_field(gst_caps_get_structure(caps, 0), "format");
    gst_structure_set_name(gst_caps_get_structure(caps, 0), "audio/mpeg");
    gst_caps_set_simple(caps, "stream-format", G_TYPE_STRING, "raw", NULL);
    gst_caps_set_simple(caps, "mpegversion", G_TYPE_INT, 4, NULL);
}

static void init_caps_from_wave_format_wma1(GstCaps *caps, const MSAUDIO1WAVEFORMAT *format, UINT32 format_size)
{
    init_caps_codec_data(caps, &format->wfx + 1, format->wfx.cbSize);

    gst_structure_remove_field(gst_caps_get_structure(caps, 0), "format");
    gst_structure_set_name(gst_caps_get_structure(caps, 0), "audio/x-wma");
    gst_caps_set_simple(caps, "wmaversion", G_TYPE_INT, 1, NULL);
    gst_caps_set_simple(caps, "block_align", G_TYPE_INT, format->wfx.nBlockAlign, NULL);
    gst_caps_set_simple(caps, "depth", G_TYPE_INT, format->wfx.wBitsPerSample, NULL);
    gst_caps_set_simple(caps, "bitrate", G_TYPE_INT, format->wfx.nAvgBytesPerSec * 8, NULL);
}

static void init_caps_from_wave_format_wma2(GstCaps *caps, const WMAUDIO2WAVEFORMAT *format, UINT32 format_size)
{
    init_caps_codec_data(caps, &format->wfx + 1, format->wfx.cbSize);

    gst_structure_remove_field(gst_caps_get_structure(caps, 0), "format");
    gst_structure_set_name(gst_caps_get_structure(caps, 0), "audio/x-wma");
    gst_caps_set_simple(caps, "wmaversion", G_TYPE_INT, 2, NULL);
    gst_caps_set_simple(caps, "block_align", G_TYPE_INT, format->wfx.nBlockAlign, NULL);
    gst_caps_set_simple(caps, "depth", G_TYPE_INT, format->wfx.wBitsPerSample, NULL);
    gst_caps_set_simple(caps, "bitrate", G_TYPE_INT, format->wfx.nAvgBytesPerSec * 8, NULL);
}

static void init_caps_from_wave_format_wma3(GstCaps *caps, const WMAUDIO3WAVEFORMAT *format, UINT32 format_size, UINT version)
{
    init_caps_codec_data(caps, &format->wfx + 1, format->wfx.cbSize);

    gst_structure_remove_field(gst_caps_get_structure(caps, 0), "format");
    gst_structure_set_name(gst_caps_get_structure(caps, 0), "audio/x-wma");
    gst_caps_set_simple(caps, "wmaversion", G_TYPE_INT, version, NULL);
    gst_caps_set_simple(caps, "block_align", G_TYPE_INT, format->wfx.nBlockAlign, NULL);
    gst_caps_set_simple(caps, "depth", G_TYPE_INT, format->wfx.wBitsPerSample, NULL);
    gst_caps_set_simple(caps, "bitrate", G_TYPE_INT, format->wfx.nAvgBytesPerSec * 8, NULL);
}

static void init_caps_from_wave_format(GstCaps *caps, const GUID *subtype,
        const void *format, UINT32 format_size)
{
    if (IsEqualGUID(subtype, &MFAudioFormat_MPEG))
        return init_caps_from_wave_format_mpeg1(caps, format, format_size);
    if (IsEqualGUID(subtype, &MFAudioFormat_MP3))
        return init_caps_from_wave_format_mp3(caps, format, format_size);
    if (IsEqualGUID(subtype, &MFAudioFormat_AAC))
        return init_caps_from_wave_format_aac(caps, format, format_size);
    if (IsEqualGUID(subtype, &MFAudioFormat_RAW_AAC))
        return init_caps_from_wave_format_aac_raw(caps, format, format_size);
    if (IsEqualGUID(subtype, &MFAudioFormat_MSAudio1))
        return init_caps_from_wave_format_wma1(caps, format, format_size);
    if (IsEqualGUID(subtype, &MFAudioFormat_WMAudioV8))
        return init_caps_from_wave_format_wma2(caps, format, format_size);
    if (IsEqualGUID(subtype, &MFAudioFormat_WMAudioV9))
        return init_caps_from_wave_format_wma3(caps, format, format_size, 3);
    if (IsEqualGUID(subtype, &MFAudioFormat_WMAudio_Lossless))
        return init_caps_from_wave_format_wma3(caps, format, format_size, 4);

    GST_FIXME("Unsupported subtype " WG_GUID_FORMAT, WG_GUID_ARGS(*subtype));
}

static GstAudioFormat wave_format_tag_to_gst_audio_format(UINT tag, UINT depth)
{
    switch (tag)
    {
    case WAVE_FORMAT_PCM:
        if (depth == 32) return GST_AUDIO_FORMAT_S32LE;
        if (depth == 24) return GST_AUDIO_FORMAT_S24LE;
        if (depth == 16) return GST_AUDIO_FORMAT_S16LE;
        if (depth == 8) return GST_AUDIO_FORMAT_U8;
        break;

    case WAVE_FORMAT_IEEE_FLOAT:
        if (depth == 64) return GST_AUDIO_FORMAT_F64LE;
        if (depth == 32) return GST_AUDIO_FORMAT_F32LE;
        break;
    }

    return GST_AUDIO_FORMAT_ENCODED;
}

static GstCaps *caps_from_wave_format_ex(const WAVEFORMATEX *format, UINT32 format_size, const GUID *subtype, UINT64 channel_mask)
{
    GstAudioFormat audio_format = wave_format_tag_to_gst_audio_format(subtype->Data1, format->wBitsPerSample);
    GstCaps *caps;

    if (!(caps = gst_caps_new_simple("audio/x-raw", "format", G_TYPE_STRING, gst_audio_format_to_string(audio_format),
            "layout", G_TYPE_STRING, "interleaved", "rate", G_TYPE_INT, format->nSamplesPerSec,
            "channels", G_TYPE_INT, format->nChannels, "channel-mask", GST_TYPE_BITMASK, channel_mask, NULL)))
        return NULL;

    if (audio_format == GST_AUDIO_FORMAT_ENCODED)
        init_caps_from_wave_format(caps, subtype, format, format_size);

    return caps;
}

static WAVEFORMATEX *strip_wave_format_extensible(const WAVEFORMATEXTENSIBLE *format_ext)
{
    UINT32 extra_size = format_ext->Format.cbSize + sizeof(WAVEFORMATEX) - sizeof(WAVEFORMATEXTENSIBLE);
    WAVEFORMATEX *format;

    if (!(format = malloc(sizeof(*format) + extra_size)))
        return NULL;

    *format = format_ext->Format;
    format->cbSize = extra_size;
    format->wFormatTag = format_ext->SubFormat.Data1;
    memcpy(format + 1, format_ext + 1, extra_size);
    return format;
}

static GstCaps *caps_from_wave_format_extensible(const WAVEFORMATEXTENSIBLE *format, UINT32 format_size)
{
    WAVEFORMATEX *wfx;
    GstCaps *caps;

    GST_TRACE("tag %#x, %u channels, sample rate %u, %u bytes/sec, alignment %u, %u bits/sample, "
            "%u valid bps, channel mask %#x, subtype " WG_GUID_FORMAT ".",
            format->Format.wFormatTag, format->Format.nChannels, format->Format.nSamplesPerSec,
            format->Format.nAvgBytesPerSec, format->Format.nBlockAlign, format->Format.wBitsPerSample,
            format->Samples.wValidBitsPerSample, format->dwChannelMask, WG_GUID_ARGS(format->SubFormat));
    if (format->Format.cbSize)
    {
        guint extra_size = sizeof(WAVEFORMATEX) + format->Format.cbSize - sizeof(WAVEFORMATEXTENSIBLE);
        GST_MEMDUMP("extra bytes:", (guint8 *)(format + 1), extra_size);
    }

    if (!(wfx = strip_wave_format_extensible(format)))
        return NULL;

    caps = caps_from_wave_format_ex(wfx, format_size + sizeof(*wfx) - sizeof(*format),
            &format->SubFormat, format->dwChannelMask);
    free(wfx);
    return caps;
}

static GstCaps *caps_from_wave_format(const void *format, UINT32 format_size)
{
    const WAVEFORMATEX *wfx = format;
    GUID subtype = MFAudioFormat_Base;
    UINT channel_mask;

    if (wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        return caps_from_wave_format_extensible(format, format_size);

    GST_TRACE("tag %#x, %u channels, sample rate %u, %u bytes/sec, alignment %u, %u bits/sample.",
            wfx->wFormatTag, wfx->nChannels, wfx->nSamplesPerSec,
            wfx->nAvgBytesPerSec, wfx->nBlockAlign, wfx->wBitsPerSample);
    if (wfx->cbSize) GST_MEMDUMP("extra bytes:", (guint8 *)(wfx + 1), wfx->cbSize);

    subtype.Data1 = wfx->wFormatTag;
    channel_mask = gst_audio_channel_get_fallback_mask(wfx->nChannels);
    return caps_from_wave_format_ex(format, format_size, &subtype, channel_mask);
}

static void init_caps_from_video_cinepak(GstCaps *caps, const MFVIDEOFORMAT *format, UINT format_size)
{
    init_caps_codec_data(caps, format + 1, format_size - sizeof(*format));

    gst_structure_remove_field(gst_caps_get_structure(caps, 0), "format");
    gst_structure_set_name(gst_caps_get_structure(caps, 0), "video/x-cinepak");
}

static void init_caps_from_video_h264(GstCaps *caps, const MFVIDEOFORMAT *format, UINT format_size)
{
    GstBuffer *buffer;

    gst_structure_remove_field(gst_caps_get_structure(caps, 0), "format");
    gst_structure_set_name(gst_caps_get_structure(caps, 0), "video/x-h264");
    gst_caps_set_simple(caps, "stream-format", G_TYPE_STRING, "byte-stream", NULL);
    gst_caps_set_simple(caps, "alignment", G_TYPE_STRING, "au", NULL);

    if (format_size > sizeof(*format) && (buffer = gst_buffer_new_and_alloc(format_size - sizeof(*format))))
    {
        gst_buffer_fill(buffer, 0, format + 1, format_size - sizeof(*format));
        gst_caps_set_simple(caps, "streamheader", GST_TYPE_BUFFER, buffer, NULL);
        gst_buffer_unref(buffer);
    }
}

static void init_caps_from_video_wmv(GstCaps *caps, const MFVIDEOFORMAT *format, UINT format_size,
        int wmv_version, const char *wmv_format)
{
    init_caps_codec_data(caps, format + 1, format_size - sizeof(*format));

    gst_structure_remove_field(gst_caps_get_structure(caps, 0), "format");
    gst_structure_set_name(gst_caps_get_structure(caps, 0), "video/x-wmv");
    gst_caps_set_simple(caps, "wmvversion", G_TYPE_INT, wmv_version, NULL);
    gst_caps_set_simple(caps, "format", G_TYPE_STRING, wmv_format, NULL);
}

static void init_caps_from_video_indeo(GstCaps *caps, const MFVIDEOFORMAT *format, UINT format_size)
{
    init_caps_codec_data(caps, format + 1, format_size - sizeof(*format));

    gst_structure_remove_field(gst_caps_get_structure(caps, 0), "format");
    gst_structure_set_name(gst_caps_get_structure(caps, 0), "video/x-indeo");
    gst_caps_set_simple(caps, "indeoversion", G_TYPE_INT, 5, NULL);
}

static void init_caps_from_video_mpeg(GstCaps *caps, const struct mpeg_video_format *format, UINT format_size)
{
    init_caps_codec_data(caps, format->sequence_header, format->sequence_header_count);

    gst_structure_remove_field(gst_caps_get_structure(caps, 0), "format");
    gst_structure_set_name(gst_caps_get_structure(caps, 0), "video/mpeg");
    gst_caps_set_simple(caps, "mpegversion", G_TYPE_INT, 1, NULL);
    gst_caps_set_simple(caps, "systemstream", G_TYPE_BOOLEAN, FALSE, NULL);
    gst_caps_set_simple(caps, "parsed", G_TYPE_BOOLEAN, TRUE, NULL);
}

static void init_caps_from_video_subtype(GstCaps *caps, const GUID *subtype, const void *format, UINT format_size)
{
    if (IsEqualGUID(subtype, &MFVideoFormat_CVID))
        return init_caps_from_video_cinepak(caps, format, format_size);
    if (IsEqualGUID(subtype, &MFVideoFormat_H264))
        return init_caps_from_video_h264(caps, format, format_size);
    if (IsEqualGUID(subtype, &MFVideoFormat_WMV1))
        return init_caps_from_video_wmv(caps, format, format_size, 1, "WMV1");
    if (IsEqualGUID(subtype, &MFVideoFormat_WMV2))
        return init_caps_from_video_wmv(caps, format, format_size, 2, "WMV2");
    if (IsEqualGUID(subtype, &MFVideoFormat_WMV3))
        return init_caps_from_video_wmv(caps, format, format_size, 3, "WMV3");
    if (IsEqualGUID(subtype, &MEDIASUBTYPE_WMVA))
        return init_caps_from_video_wmv(caps, format, format_size, 3, "WMVA");
    if (IsEqualGUID(subtype, &MFVideoFormat_WVC1))
        return init_caps_from_video_wmv(caps, format, format_size, 3, "WVC1");
    if (IsEqualGUID(subtype, &MFVideoFormat_IV50))
        return init_caps_from_video_indeo(caps, format, format_size);
    if (IsEqualGUID(subtype, &MEDIASUBTYPE_MPEG1Payload))
        return init_caps_from_video_mpeg(caps, format, format_size);

    GST_FIXME("Unsupported subtype " WG_GUID_FORMAT, WG_GUID_ARGS(*subtype));
}

static GstVideoFormat subtype_to_gst_video_format(const GUID *subtype)
{
    GUID base = *subtype;
    base.Data1 = 0;

    if (IsEqualGUID(&base, &MFVideoFormat_Base))
    {
        switch (subtype->Data1)
        {
        case D3DFMT_A8R8G8B8: return GST_VIDEO_FORMAT_BGRA;
        case D3DFMT_X8R8G8B8: return GST_VIDEO_FORMAT_BGRx;
        case D3DFMT_R8G8B8:   return GST_VIDEO_FORMAT_BGR;
        case D3DFMT_A8B8G8R8: return GST_VIDEO_FORMAT_RGBA;
        case D3DFMT_X1R5G5B5: return GST_VIDEO_FORMAT_RGB15;
        case D3DFMT_R5G6B5:   return GST_VIDEO_FORMAT_RGB16;
        case MAKEFOURCC('A','Y','U','V'): return GST_VIDEO_FORMAT_AYUV;
        case MAKEFOURCC('I','Y','U','V'):
        case MAKEFOURCC('I','4','2','0'): return GST_VIDEO_FORMAT_I420;
        case MAKEFOURCC('N','V','1','2'): return GST_VIDEO_FORMAT_NV12;
        case MAKEFOURCC('U','Y','V','Y'): return GST_VIDEO_FORMAT_UYVY;
        case MAKEFOURCC('Y','U','Y','2'): return GST_VIDEO_FORMAT_YUY2;
        case MAKEFOURCC('Y','V','1','2'): return GST_VIDEO_FORMAT_YV12;
        case MAKEFOURCC('Y','V','Y','U'): return GST_VIDEO_FORMAT_YVYU;
        }
    }

    return GST_VIDEO_FORMAT_ENCODED;
}

static GstCaps *caps_from_video_format(const MFVIDEOFORMAT *format, UINT32 format_size)
{
    GstVideoFormat video_format = subtype_to_gst_video_format(&format->guidFormat);
    GstCaps *caps;

    GST_TRACE("subtype " WG_GUID_FORMAT " %ux%u, FPS " WG_RATIO_FORMAT ", aperture " WG_APERTURE_FORMAT ", "
            "PAR " WG_RATIO_FORMAT ", videoFlags %#x.",
            WG_GUID_ARGS(format->guidFormat), format->videoInfo.dwWidth, format->videoInfo.dwHeight,
            WG_RATIO_ARGS(format->videoInfo.FramesPerSecond), WG_APERTURE_ARGS(format->videoInfo.MinimumDisplayAperture),
            WG_RATIO_ARGS(format->videoInfo.PixelAspectRatio), (int)format->videoInfo.VideoFlags );
    if (format->dwSize > sizeof(*format)) GST_MEMDUMP("extra bytes:", (guint8 *)(format + 1), format->dwSize - sizeof(*format));

    if (!(caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, gst_video_format_to_string(video_format), NULL)))
        return NULL;

    if (format->videoInfo.dwWidth)
        gst_caps_set_simple(caps, "width", G_TYPE_INT, format->videoInfo.dwWidth, NULL);
    if (format->videoInfo.dwHeight)
        gst_caps_set_simple(caps, "height", G_TYPE_INT, format->videoInfo.dwHeight, NULL);

    if (format->videoInfo.PixelAspectRatio.Denominator)
        gst_caps_set_simple(caps, "pixel-aspect-ratio", GST_TYPE_FRACTION,
                format->videoInfo.PixelAspectRatio.Numerator,
                format->videoInfo.PixelAspectRatio.Denominator, NULL);
    if (format->videoInfo.FramesPerSecond.Denominator)
        gst_caps_set_simple(caps, "framerate", GST_TYPE_FRACTION,
                format->videoInfo.FramesPerSecond.Numerator,
                format->videoInfo.FramesPerSecond.Denominator, NULL);

    if (!is_mf_video_area_empty(&format->videoInfo.MinimumDisplayAperture))
    {
        gst_caps_set_simple(caps, "width", G_TYPE_INT, format->videoInfo.MinimumDisplayAperture.Area.cx, NULL);
        gst_caps_set_simple(caps, "height", G_TYPE_INT, format->videoInfo.MinimumDisplayAperture.Area.cy, NULL);
    }

    if (video_format == GST_VIDEO_FORMAT_ENCODED)
        init_caps_from_video_subtype(caps, &format->guidFormat, format, format_size);

    return caps;
}

GstCaps *caps_from_media_type(const struct wg_media_type *media_type)
{
    GstCaps *caps = NULL;

    if (IsEqualGUID(&media_type->major, &MFMediaType_Video))
        caps = caps_from_video_format(media_type->u.video, media_type->format_size);
    else if (IsEqualGUID(&media_type->major, &MFMediaType_Audio))
        caps = caps_from_wave_format(media_type->u.audio, media_type->format_size);

    GST_TRACE("caps %"GST_PTR_FORMAT, caps);
    return caps;
}

static WORD wave_format_tag_from_gst_audio_format(GstAudioFormat audio_format)
{
    switch (audio_format)
    {
    case GST_AUDIO_FORMAT_U8:    return WAVE_FORMAT_PCM;
    case GST_AUDIO_FORMAT_S16LE: return WAVE_FORMAT_PCM;
    case GST_AUDIO_FORMAT_S24LE: return WAVE_FORMAT_PCM;
    case GST_AUDIO_FORMAT_S32LE: return WAVE_FORMAT_PCM;
    case GST_AUDIO_FORMAT_F32LE: return WAVE_FORMAT_IEEE_FLOAT;
    case GST_AUDIO_FORMAT_F64LE: return WAVE_FORMAT_IEEE_FLOAT;
    default:                     return WAVE_FORMAT_EXTENSIBLE;
    }
}

static GUID subtype_from_gst_audio_format(GstAudioFormat audio_format)
{
    switch (audio_format)
    {
    case GST_AUDIO_FORMAT_U8:    return MFAudioFormat_PCM;
    case GST_AUDIO_FORMAT_S16LE: return MFAudioFormat_PCM;
    case GST_AUDIO_FORMAT_S24LE: return MFAudioFormat_PCM;
    case GST_AUDIO_FORMAT_S32LE: return MFAudioFormat_PCM;
    case GST_AUDIO_FORMAT_F32LE: return MFAudioFormat_Float;
    case GST_AUDIO_FORMAT_F64LE: return MFAudioFormat_Float;
    default:                     return GUID_NULL;
    }
}

static void init_wave_format_ex_from_gst_caps(const GstCaps *caps, WORD format_tag, gint depth,
        WAVEFORMATEX *format, UINT32 format_size)
{
    const GstStructure *structure = gst_caps_get_structure(caps, 0);
    gint bitrate, channels, rate, block_align;

    memset(format, 0, format_size);
    format->cbSize = format_size - sizeof(*format);
    format->wFormatTag = format_tag;
    format->wBitsPerSample = depth;

    if (gst_structure_get_int(structure, "channels", &channels))
        format->nChannels = channels;
    if (gst_structure_get_int(structure, "rate", &rate))
        format->nSamplesPerSec = rate;
    if (gst_structure_get_int(structure, "depth", &depth))
        format->wBitsPerSample = depth;

    format->nBlockAlign = format->wBitsPerSample * format->nChannels / 8;
    format->nAvgBytesPerSec = format->nSamplesPerSec * format->nBlockAlign;

    if (gst_structure_get_int(structure, "block_align", &block_align))
        format->nBlockAlign = block_align;
    if (gst_structure_get_int(structure, "bitrate", &bitrate))
        format->nAvgBytesPerSec = bitrate / 8;
}

static GstBuffer *caps_get_buffer(const GstCaps *caps, const char *name, UINT32 *buffer_size)
{
    const GstStructure *structure = gst_caps_get_structure(caps, 0);
    const GValue *buffer_value;

    if ((buffer_value = gst_structure_get_value(structure, name)))
    {
        GstBuffer *buffer = gst_value_get_buffer(buffer_value);
        *buffer_size = gst_buffer_get_size(buffer);
        return buffer;
    }

    *buffer_size = 0;
    return NULL;
}

static NTSTATUS wave_format_extensible_from_gst_caps(const GstCaps *caps, const GUID *subtype, UINT32 depth,
        UINT64 channel_mask, WAVEFORMATEXTENSIBLE *format, UINT32 *format_size)
{
    UINT32 capacity = *format_size, codec_data_size;
    GstBuffer *codec_data = caps_get_buffer(caps, "codec_data", &codec_data_size);

    *format_size = sizeof(*format) + codec_data_size;
    if (*format_size > capacity)
        return STATUS_BUFFER_TOO_SMALL;

    init_wave_format_ex_from_gst_caps(caps, WAVE_FORMAT_EXTENSIBLE, depth, &format->Format, *format_size);
    format->Samples.wValidBitsPerSample = 0;
    format->dwChannelMask = channel_mask;
    format->SubFormat = *subtype;

    if (codec_data)
        gst_buffer_extract(codec_data, 0, format + 1, codec_data_size);

    GST_TRACE("tag %#x, %u channels, sample rate %u, %u bytes/sec, alignment %u, %u bits/sample, "
            "%u valid bps, channel mask %#x, subtype " WG_GUID_FORMAT ".",
            format->Format.wFormatTag, format->Format.nChannels, format->Format.nSamplesPerSec,
            format->Format.nAvgBytesPerSec, format->Format.nBlockAlign, format->Format.wBitsPerSample,
            format->Samples.wValidBitsPerSample, format->dwChannelMask, WG_GUID_ARGS(format->SubFormat));
    if (format->Format.cbSize)
    {
        guint extra_size = sizeof(WAVEFORMATEX) + format->Format.cbSize - sizeof(WAVEFORMATEXTENSIBLE);
        GST_MEMDUMP("extra bytes:", (guint8 *)(format + 1), extra_size);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS wave_format_ex_from_gst_caps(const GstCaps *caps, WORD format_tag, UINT32 depth,
        UINT32 wave_format_size, WAVEFORMATEX *format, UINT32 *format_size)
{
    UINT32 capacity = *format_size, codec_data_size;
    GstBuffer *codec_data = caps_get_buffer(caps, "codec_data", &codec_data_size);

    *format_size = max(wave_format_size, sizeof(*format) + codec_data_size);
    if (*format_size > capacity)
        return STATUS_BUFFER_TOO_SMALL;

    init_wave_format_ex_from_gst_caps(caps, format_tag, depth, format, *format_size);

    if (codec_data)
        gst_buffer_extract(codec_data, 0, format + 1, codec_data_size);

    GST_TRACE("tag %#x, %u channels, sample rate %u, %u bytes/sec, alignment %u, %u bits/sample.",
            format->wFormatTag, format->nChannels, format->nSamplesPerSec,
            format->nAvgBytesPerSec, format->nBlockAlign, format->wBitsPerSample);
    if (format->cbSize) GST_MEMDUMP("extra bytes:", (guint8 *)(format + 1), format->cbSize);

    return STATUS_SUCCESS;
}

static NTSTATUS wave_format_from_gst_caps(const GstCaps *caps, void *format, UINT32 *format_size)
{
    const GstStructure *structure = gst_caps_get_structure(caps, 0);
    GstAudioFormat audio_format = GST_AUDIO_FORMAT_ENCODED;
    WORD format_tag = WAVE_FORMAT_EXTENSIBLE;
    gint channels, depth = 0;
    const gchar *str_value;
    guint64 channel_mask;

    if ((str_value = gst_structure_get_string(structure, "format")))
    {
        audio_format = gst_audio_format_from_string(str_value);
        format_tag = wave_format_tag_from_gst_audio_format(audio_format);
        depth = GST_AUDIO_FORMAT_INFO_DEPTH(gst_audio_format_get_info(audio_format));
    }

    if (!gst_structure_get_int(structure, "channels", &channels))
        channels = 1;
    if (!gst_structure_get(structure, "channel-mask", GST_TYPE_BITMASK, &channel_mask, NULL))
        channel_mask = 0;

    if (format_tag == WAVE_FORMAT_EXTENSIBLE || channel_mask != 0)
    {
        GUID subtype = subtype_from_gst_audio_format(audio_format);
        return wave_format_extensible_from_gst_caps(caps, &subtype, depth, channel_mask, format, format_size);
    }

    return wave_format_ex_from_gst_caps(caps, format_tag, depth, sizeof(WAVEFORMATEX), format, format_size);
}

static NTSTATUS mpeg_wave_format_from_gst_caps(const GstCaps *caps, WAVEFORMATEX *format, UINT32 *format_size)
{
    const GstStructure *structure = gst_caps_get_structure(caps, 0);
    NTSTATUS status;
    gint layer = 0;

    if (gst_structure_get_int(structure, "layer", &layer) && layer == 3)
        return wave_format_ex_from_gst_caps(caps, WAVE_FORMAT_MPEGLAYER3, 0, sizeof(MPEGLAYER3WAVEFORMAT), format, format_size);

    if (!(status = wave_format_ex_from_gst_caps(caps, WAVE_FORMAT_MPEG, 0, sizeof(MPEG1WAVEFORMAT), format, format_size)))
    {
        MPEG1WAVEFORMAT *mpeg = CONTAINING_RECORD(format, MPEG1WAVEFORMAT, wfx);
        mpeg->fwHeadLayer = layer;
    }

    return status;
}

static NTSTATUS wma_wave_format_from_gst_caps(const GstCaps *caps, WAVEFORMATEX *format, UINT32 *format_size)
{
    const GstStructure *structure = gst_caps_get_structure(caps, 0);
    gint wmaversion;

    if (!gst_structure_get_int(structure, "wmaversion", &wmaversion))
    {
        GST_WARNING("Missing \"wmaversion\" value in %" GST_PTR_FORMAT ".", caps);
        return STATUS_INVALID_PARAMETER;
    }

    if (wmaversion == 1)
        return wave_format_ex_from_gst_caps(caps, WAVE_FORMAT_MSAUDIO1, 0, sizeof(MSAUDIO1WAVEFORMAT), format, format_size);
    if (wmaversion == 2)
        return wave_format_ex_from_gst_caps(caps, WAVE_FORMAT_WMAUDIO2, 0, sizeof(WMAUDIO2WAVEFORMAT), format, format_size);
    if (wmaversion == 3)
        return wave_format_ex_from_gst_caps(caps, WAVE_FORMAT_WMAUDIO3, 0, sizeof(WMAUDIO3WAVEFORMAT), format, format_size);

    GST_FIXME("Unsupported wmaversion %u", wmaversion);
    return STATUS_NOT_IMPLEMENTED;
}

static GUID subtype_from_gst_video_format(GstVideoFormat video_format)
{
    switch (video_format)
    {
    case GST_VIDEO_FORMAT_BGRA:    return MFVideoFormat_ARGB32;
    case GST_VIDEO_FORMAT_BGRx:    return MFVideoFormat_RGB32;
    case GST_VIDEO_FORMAT_BGR:     return MFVideoFormat_RGB24;
    case GST_VIDEO_FORMAT_RGBA:    return MFVideoFormat_ABGR32;
    case GST_VIDEO_FORMAT_RGB15:   return MFVideoFormat_RGB555;
    case GST_VIDEO_FORMAT_RGB16:   return MFVideoFormat_RGB565;
    case GST_VIDEO_FORMAT_AYUV:    return MFVideoFormat_AYUV;
    case GST_VIDEO_FORMAT_I420:    return MFVideoFormat_I420;
    case GST_VIDEO_FORMAT_NV12:    return MFVideoFormat_NV12;
    case GST_VIDEO_FORMAT_P010_10LE: return MFVideoFormat_P010;
    case GST_VIDEO_FORMAT_UYVY:    return MFVideoFormat_UYVY;
    case GST_VIDEO_FORMAT_YUY2:    return MFVideoFormat_YUY2;
    case GST_VIDEO_FORMAT_YV12:    return MFVideoFormat_YV12;
    case GST_VIDEO_FORMAT_YVYU:    return MFVideoFormat_YVYU;
    default:                       return GUID_NULL;
    }
}

static void init_mf_video_format_from_gst_caps(const GstCaps *caps, const GUID *subtype, MFVIDEOFORMAT *format,
        UINT32 format_size, UINT32 video_plane_align)
{
    const GstStructure *structure = gst_caps_get_structure(caps, 0);
    gint width = 0, height = 0, num_value, den_value;
    const gchar *str_value;

    memset(format, 0, format_size);
    format->dwSize = format_size;

    if (subtype)
        format->guidFormat = *subtype;
    else if ((str_value = gst_structure_get_string(structure, "format")))
    {
        GstVideoFormat video_format = gst_video_format_from_string(str_value);
        format->guidFormat = subtype_from_gst_video_format(video_format);
    }

    if (gst_structure_get_int(structure, "width", &width))
        format->videoInfo.dwWidth = (width + video_plane_align) & ~video_plane_align;
    if (gst_structure_get_int(structure, "height", &height))
        format->videoInfo.dwHeight = (height + video_plane_align) & ~video_plane_align;
    if (format->videoInfo.dwWidth != width || format->videoInfo.dwHeight != height)
    {
        format->videoInfo.MinimumDisplayAperture.Area.cx = width;
        format->videoInfo.MinimumDisplayAperture.Area.cy = height;
    }
    format->videoInfo.GeometricAperture = format->videoInfo.MinimumDisplayAperture;
    format->videoInfo.PanScanAperture = format->videoInfo.MinimumDisplayAperture;

    if (gst_structure_get_fraction(structure, "pixel-aspect-ratio", &num_value, &den_value))
    {
        format->videoInfo.PixelAspectRatio.Numerator = num_value;
        format->videoInfo.PixelAspectRatio.Denominator = den_value;
    }
    if (gst_structure_get_fraction(structure, "framerate", &num_value, &den_value))
    {
        format->videoInfo.FramesPerSecond.Numerator = num_value;
        format->videoInfo.FramesPerSecond.Denominator = den_value;
    }
}

static NTSTATUS video_format_from_gst_caps(const GstCaps *caps, const GUID *subtype, MFVIDEOFORMAT *format,
        UINT32 *format_size, UINT32 video_plane_align)
{
    UINT32 capacity = *format_size, codec_data_size;
    GstBuffer *codec_data = caps_get_buffer(caps, "codec_data", &codec_data_size);

    *format_size = sizeof(*format) + codec_data_size;
    if (*format_size > capacity)
        return STATUS_BUFFER_TOO_SMALL;

    init_mf_video_format_from_gst_caps(caps, subtype, format, *format_size, video_plane_align);

    if (codec_data)
        gst_buffer_extract(codec_data, 0, format + 1, codec_data_size);

    GST_TRACE("subtype " WG_GUID_FORMAT " %ux%u, FPS " WG_RATIO_FORMAT ", aperture " WG_APERTURE_FORMAT ", "
            "PAR " WG_RATIO_FORMAT ", videoFlags %#x.",
            WG_GUID_ARGS(format->guidFormat), format->videoInfo.dwWidth, format->videoInfo.dwHeight,
            WG_RATIO_ARGS(format->videoInfo.FramesPerSecond), WG_APERTURE_ARGS(format->videoInfo.MinimumDisplayAperture),
            WG_RATIO_ARGS(format->videoInfo.PixelAspectRatio), (int)format->videoInfo.VideoFlags );
    if (format->dwSize > sizeof(*format)) GST_MEMDUMP("extra bytes:", (guint8 *)(format + 1), format->dwSize - sizeof(*format));

    return STATUS_SUCCESS;
}

static NTSTATUS wmv_video_format_from_gst_caps(const GstCaps *caps, MFVIDEOFORMAT *format,
        UINT32 *format_size, UINT32 video_plane_align)
{
    const GstStructure *structure = gst_caps_get_structure(caps, 0);
    gchar format_buffer[5] = {'W','M','V','0',0};
    const gchar *wmv_format_str;
    gint wmv_version = 0;
    const GUID *subtype;

    if (!(wmv_format_str = gst_structure_get_string(structure, "format")))
    {
        if (!gst_structure_get_int(structure, "wmvversion", &wmv_version))
            GST_WARNING("Unable to get WMV format.");
        format_buffer[3] += wmv_version;
        wmv_format_str = format_buffer;
    }

    if (!strcmp(wmv_format_str, "WMV1"))
        subtype = &MFVideoFormat_WMV1;
    else if (!strcmp(wmv_format_str, "WMV2"))
        subtype = &MFVideoFormat_WMV2;
    else if (!strcmp(wmv_format_str, "WMV3"))
        subtype = &MFVideoFormat_WMV3;
    else if (!strcmp(wmv_format_str, "WMVA"))
        subtype = &MEDIASUBTYPE_WMVA;
    else if (!strcmp(wmv_format_str, "WVC1"))
        subtype = &MFVideoFormat_WVC1;
    else
    {
        GST_WARNING("Unknown \"wmvversion\" value.");
        return STATUS_INVALID_PARAMETER;
    }

    return video_format_from_gst_caps(caps, subtype, format, format_size, video_plane_align);
}

static NTSTATUS mpeg_video_format_from_gst_caps(const GstCaps *caps, struct mpeg_video_format *format,
        UINT32 *format_size, UINT32 video_plane_align)
{
    UINT32 capacity = *format_size, codec_data_size;
    GstBuffer *codec_data = caps_get_buffer(caps, "codec_data", &codec_data_size);

    *format_size = sizeof(*format) + codec_data_size;
    if (*format_size > capacity)
        return STATUS_BUFFER_TOO_SMALL;

    init_mf_video_format_from_gst_caps(caps, &MEDIASUBTYPE_MPEG1Payload, &format->hdr, *format_size, video_plane_align);

    if (codec_data)
    {
        gst_buffer_extract(codec_data, 0, format->sequence_header, codec_data_size);
        format->sequence_header_count = codec_data_size;
    }

    GST_TRACE("subtype " WG_GUID_FORMAT " %ux%u, FPS " WG_RATIO_FORMAT ", aperture " WG_APERTURE_FORMAT ", "
            "PAR " WG_RATIO_FORMAT ", videoFlags %#x, start_time_code %u, profile %u, level %u, flags %#x.",
            WG_GUID_ARGS(format->hdr.guidFormat), format->hdr.videoInfo.dwWidth, format->hdr.videoInfo.dwHeight,
            WG_RATIO_ARGS(format->hdr.videoInfo.FramesPerSecond), WG_APERTURE_ARGS(format->hdr.videoInfo.MinimumDisplayAperture),
            WG_RATIO_ARGS(format->hdr.videoInfo.PixelAspectRatio), (int)format->hdr.videoInfo.VideoFlags, format->start_time_code,
            format->profile, format->level, format->flags );
    if (format->sequence_header_count) GST_MEMDUMP("extra bytes:", format->sequence_header, format->sequence_header_count);

    return STATUS_SUCCESS;
}

NTSTATUS caps_to_media_type(GstCaps *caps, struct wg_media_type *media_type, UINT32 video_plane_align)
{
    const GstStructure *structure = gst_caps_get_structure(caps, 0);
    const char *name = gst_structure_get_name(structure);
    gboolean parsed;

    GST_TRACE("caps %"GST_PTR_FORMAT, caps);

    if (g_str_has_prefix(name, "audio/"))
    {
        media_type->major = MFMediaType_Audio;

        if (!strcmp(name, "audio/x-raw"))
            return wave_format_from_gst_caps(caps, media_type->u.audio, &media_type->format_size);
        if (!strcmp(name, "audio/mpeg") && gst_structure_get_boolean(structure, "parsed", &parsed) && parsed)
            return mpeg_wave_format_from_gst_caps(caps, media_type->u.audio, &media_type->format_size);
        if (!strcmp(name, "audio/x-wma"))
            return wma_wave_format_from_gst_caps(caps, media_type->u.audio, &media_type->format_size);

        GST_FIXME("Unhandled caps %" GST_PTR_FORMAT ".", caps);
        return STATUS_UNSUCCESSFUL;
    }
    else if (g_str_has_prefix(name, "video/"))
    {
        media_type->major = MFMediaType_Video;

        if (!strcmp(name, "video/x-raw"))
            return video_format_from_gst_caps(caps, NULL, media_type->u.video, &media_type->format_size, video_plane_align);
        if (!strcmp(name, "video/x-cinepak"))
            return video_format_from_gst_caps(caps, &MFVideoFormat_CVID, media_type->u.video, &media_type->format_size, video_plane_align);
        if (!strcmp(name, "video/x-wmv"))
            return wmv_video_format_from_gst_caps(caps, media_type->u.video, &media_type->format_size, video_plane_align);
        if (!strcmp(name, "video/mpeg") && gst_structure_get_boolean(structure, "parsed", &parsed) && parsed)
            return mpeg_video_format_from_gst_caps(caps, media_type->u.format, &media_type->format_size, video_plane_align);

        GST_FIXME("Unhandled caps %" GST_PTR_FORMAT ".", caps);
        return STATUS_UNSUCCESSFUL;
    }
    else
    {
        GST_FIXME("Unhandled caps %" GST_PTR_FORMAT ".", caps);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}
