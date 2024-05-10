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
#define WG_GUID_ARGS(guid) (int)(guid).Data1, (guid).Data2, (guid).Data3, (guid).Data4[0], \
            (guid).Data4[1], (guid).Data4[2], (guid).Data4[3], (guid).Data4[4], \
            (guid).Data4[5], (guid).Data4[6], (guid).Data4[7]

#define WG_RATIO_FORMAT "%d:%d"
#define WG_RATIO_ARGS(ratio) (int)(ratio).Numerator, (int)(ratio).Denominator

#define WG_APERTURE_FORMAT "(%d,%d)-(%d,%d)"
#define WG_APERTURE_ARGS(aperture) (int)(aperture).OffsetX.value, (int)(aperture).OffsetY.value, \
            (int)(aperture).Area.cx, (int)(aperture).Area.cy

static const GUID GUID_NULL;

DEFINE_MEDIATYPE_GUID(MFAudioFormat_RAW_AAC,WAVE_FORMAT_RAW_AAC1);
DEFINE_MEDIATYPE_GUID(MFAudioFormat_MSAudio1,WAVE_FORMAT_MSAUDIO1);

DEFINE_MEDIATYPE_GUID(MFVideoFormat_CVID,MAKEFOURCC('c','v','i','d'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_IV50,MAKEFOURCC('I','V','5','0'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_VC1S,MAKEFOURCC('V','C','1','S'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_ABGR32,D3DFMT_A8B8G8R8);

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
            format->Format.wFormatTag, format->Format.nChannels, (int)format->Format.nSamplesPerSec,
            (int)format->Format.nAvgBytesPerSec, format->Format.nBlockAlign, format->Format.wBitsPerSample,
            format->Samples.wValidBitsPerSample, (int)format->dwChannelMask, WG_GUID_ARGS(format->SubFormat));
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
            format->wFormatTag, format->nChannels, (int)format->nSamplesPerSec,
            (int)format->nAvgBytesPerSec, format->nBlockAlign, format->wBitsPerSample);
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
            WG_GUID_ARGS(format->guidFormat), (int)format->videoInfo.dwWidth, (int)format->videoInfo.dwHeight,
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
            WG_GUID_ARGS(format->hdr.guidFormat), (int)format->hdr.videoInfo.dwWidth, (int)format->hdr.videoInfo.dwHeight,
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
