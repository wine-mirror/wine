/*
 * GStreamer format helpers
 *
 * Copyright 2010 Maarten Lankhorst for CodeWeavers
 * Copyright 2010 Aric Stewart for CodeWeavers
 * Copyright 2019-2020 Zebediah Figura
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

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/audio/audio.h>

#include "winternl.h"
#include "codecapi.h"
#include "dshow.h"

#include "unix_private.h"

GST_DEBUG_CATEGORY_EXTERN(wine);
#define GST_CAT_DEFAULT wine

static enum wg_audio_format wg_audio_format_from_gst(GstAudioFormat format)
{
    switch (format)
    {
        case GST_AUDIO_FORMAT_U8:
            return WG_AUDIO_FORMAT_U8;
        case GST_AUDIO_FORMAT_S16LE:
            return WG_AUDIO_FORMAT_S16LE;
        case GST_AUDIO_FORMAT_S24LE:
            return WG_AUDIO_FORMAT_S24LE;
        case GST_AUDIO_FORMAT_S32LE:
            return WG_AUDIO_FORMAT_S32LE;
        case GST_AUDIO_FORMAT_F32LE:
            return WG_AUDIO_FORMAT_F32LE;
        case GST_AUDIO_FORMAT_F64LE:
            return WG_AUDIO_FORMAT_F64LE;
        default:
            return WG_AUDIO_FORMAT_UNKNOWN;
    }
}

static uint32_t wg_channel_position_from_gst(GstAudioChannelPosition position)
{
    static const uint32_t position_map[] =
    {
        SPEAKER_FRONT_LEFT,
        SPEAKER_FRONT_RIGHT,
        SPEAKER_FRONT_CENTER,
        SPEAKER_LOW_FREQUENCY,
        SPEAKER_BACK_LEFT,
        SPEAKER_BACK_RIGHT,
        SPEAKER_FRONT_LEFT_OF_CENTER,
        SPEAKER_FRONT_RIGHT_OF_CENTER,
        SPEAKER_BACK_CENTER,
        0,
        SPEAKER_SIDE_LEFT,
        SPEAKER_SIDE_RIGHT,
        SPEAKER_TOP_FRONT_LEFT,
        SPEAKER_TOP_FRONT_RIGHT,
        SPEAKER_TOP_FRONT_CENTER,
        SPEAKER_TOP_CENTER,
        SPEAKER_TOP_BACK_LEFT,
        SPEAKER_TOP_BACK_RIGHT,
        0,
        0,
        SPEAKER_TOP_BACK_CENTER,
    };

    if (position == GST_AUDIO_CHANNEL_POSITION_MONO)
        return SPEAKER_FRONT_CENTER;

    if (position >= 0 && position < ARRAY_SIZE(position_map))
        return position_map[position];
    return 0;
}

static uint32_t wg_channel_mask_from_gst(const GstAudioInfo *info)
{
    uint32_t mask = 0, position;
    unsigned int i;

    for (i = 0; i < GST_AUDIO_INFO_CHANNELS(info); ++i)
    {
        if (!(position = wg_channel_position_from_gst(GST_AUDIO_INFO_POSITION(info, i))))
        {
            GST_WARNING("Unsupported channel %#x.", GST_AUDIO_INFO_POSITION(info, i));
            return 0;
        }
        /* Make sure it's also in WinMM order. WinMM mandates that channels be
         * ordered, as it were, from least to most significant SPEAKER_* bit.
         * Hence we fail if the current channel was already specified, or if any
         * higher bit was already specified. */
        if (mask & ~(position - 1))
        {
            GST_WARNING("Unsupported channel order.");
            return 0;
        }
        mask |= position;
    }
    return mask;
}

static void wg_format_from_audio_info(struct wg_format *format, const GstAudioInfo *info)
{
    format->major_type = WG_MAJOR_TYPE_AUDIO;
    format->u.audio.format = wg_audio_format_from_gst(GST_AUDIO_INFO_FORMAT(info));
    format->u.audio.channels = GST_AUDIO_INFO_CHANNELS(info);
    format->u.audio.channel_mask = wg_channel_mask_from_gst(info);
    format->u.audio.rate = GST_AUDIO_INFO_RATE(info);
}

static enum wg_video_format wg_video_format_from_gst(GstVideoFormat format)
{
    switch (format)
    {
        case GST_VIDEO_FORMAT_BGRA:
            return WG_VIDEO_FORMAT_BGRA;
        case GST_VIDEO_FORMAT_BGRx:
            return WG_VIDEO_FORMAT_BGRx;
        case GST_VIDEO_FORMAT_BGR:
            return WG_VIDEO_FORMAT_BGR;
        case GST_VIDEO_FORMAT_RGB15:
            return WG_VIDEO_FORMAT_RGB15;
        case GST_VIDEO_FORMAT_RGB16:
            return WG_VIDEO_FORMAT_RGB16;
        case GST_VIDEO_FORMAT_AYUV:
            return WG_VIDEO_FORMAT_AYUV;
        case GST_VIDEO_FORMAT_I420:
            return WG_VIDEO_FORMAT_I420;
        case GST_VIDEO_FORMAT_NV12:
            return WG_VIDEO_FORMAT_NV12;
        case GST_VIDEO_FORMAT_UYVY:
            return WG_VIDEO_FORMAT_UYVY;
        case GST_VIDEO_FORMAT_YUY2:
            return WG_VIDEO_FORMAT_YUY2;
        case GST_VIDEO_FORMAT_YV12:
            return WG_VIDEO_FORMAT_YV12;
        case GST_VIDEO_FORMAT_YVYU:
            return WG_VIDEO_FORMAT_YVYU;
        default:
            return WG_VIDEO_FORMAT_UNKNOWN;
    }
}

static void wg_format_from_video_info(struct wg_format *format, const GstVideoInfo *info)
{
    format->major_type = WG_MAJOR_TYPE_VIDEO;
    format->u.video.format = wg_video_format_from_gst(GST_VIDEO_INFO_FORMAT(info));
    format->u.video.width = GST_VIDEO_INFO_WIDTH(info);
    format->u.video.height = GST_VIDEO_INFO_HEIGHT(info);
    format->u.video.fps_n = GST_VIDEO_INFO_FPS_N(info);
    format->u.video.fps_d = GST_VIDEO_INFO_FPS_D(info);
}

static void wg_format_from_caps_audio_mpeg1(struct wg_format *format, const GstCaps *caps)
{
    const GstStructure *structure = gst_caps_get_structure(caps, 0);
    gint layer, channels, rate;

    if (!gst_structure_get_int(structure, "layer", &layer))
    {
        GST_WARNING("Missing \"layer\" value.");
        return;
    }
    if (!gst_structure_get_int(structure, "channels", &channels))
    {
        GST_WARNING("Missing \"channels\" value.");
        return;
    }
    if (!gst_structure_get_int(structure, "rate", &rate))
    {
        GST_WARNING("Missing \"rate\" value.");
        return;
    }

    format->major_type = WG_MAJOR_TYPE_AUDIO_MPEG1;
    format->u.audio_mpeg1.layer = layer;
    format->u.audio_mpeg1.channels = channels;
    format->u.audio_mpeg1.rate = rate;
}

static void wg_format_from_caps_video_cinepak(struct wg_format *format, const GstCaps *caps)
{
    const GstStructure *structure = gst_caps_get_structure(caps, 0);
    gint width, height, fps_n, fps_d;

    if (!gst_structure_get_int(structure, "width", &width))
    {
        GST_WARNING("Missing \"width\" value.");
        return;
    }
    if (!gst_structure_get_int(structure, "height", &height))
    {
        GST_WARNING("Missing \"height\" value.");
        return;
    }
    if (!gst_structure_get_fraction(structure, "framerate", &fps_n, &fps_d))
    {
        fps_n = 0;
        fps_d = 1;
    }

    format->major_type = WG_MAJOR_TYPE_VIDEO_CINEPAK;
    format->u.video_cinepak.width = width;
    format->u.video_cinepak.height = height;
    format->u.video_cinepak.fps_n = fps_n;
    format->u.video_cinepak.fps_d = fps_d;
}

void wg_format_from_caps(struct wg_format *format, const GstCaps *caps)
{
    const GstStructure *structure = gst_caps_get_structure(caps, 0);
    const char *name = gst_structure_get_name(structure);

    memset(format, 0, sizeof(*format));

    if (!strcmp(name, "audio/x-raw"))
    {
        GstAudioInfo info;

        if (gst_audio_info_from_caps(&info, caps))
            wg_format_from_audio_info(format, &info);
    }
    else if (!strcmp(name, "video/x-raw"))
    {
        GstVideoInfo info;

        if (gst_video_info_from_caps(&info, caps))
            wg_format_from_video_info(format, &info);
    }
    else if (!strcmp(name, "audio/mpeg"))
    {
        wg_format_from_caps_audio_mpeg1(format, caps);
    }
    else if (!strcmp(name, "video/x-cinepak"))
    {
        wg_format_from_caps_video_cinepak(format, caps);
    }
    else
    {
        gchar *str = gst_caps_to_string(caps);

        GST_FIXME("Unhandled caps %s.", str);
        g_free(str);
    }
}

static GstAudioFormat wg_audio_format_to_gst(enum wg_audio_format format)
{
    switch (format)
    {
        case WG_AUDIO_FORMAT_U8:    return GST_AUDIO_FORMAT_U8;
        case WG_AUDIO_FORMAT_S16LE: return GST_AUDIO_FORMAT_S16LE;
        case WG_AUDIO_FORMAT_S24LE: return GST_AUDIO_FORMAT_S24LE;
        case WG_AUDIO_FORMAT_S32LE: return GST_AUDIO_FORMAT_S32LE;
        case WG_AUDIO_FORMAT_F32LE: return GST_AUDIO_FORMAT_F32LE;
        case WG_AUDIO_FORMAT_F64LE: return GST_AUDIO_FORMAT_F64LE;
        default: return GST_AUDIO_FORMAT_UNKNOWN;
    }
}

static void wg_channel_mask_to_gst(GstAudioChannelPosition *positions, uint32_t mask, uint32_t channel_count)
{
    const uint32_t orig_mask = mask;
    unsigned int i;
    DWORD bit;

    static const GstAudioChannelPosition position_map[] =
    {
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
        GST_AUDIO_CHANNEL_POSITION_LFE1,
        GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
        GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT_OF_CENTER,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER,
        GST_AUDIO_CHANNEL_POSITION_REAR_CENTER,
        GST_AUDIO_CHANNEL_POSITION_SIDE_LEFT,
        GST_AUDIO_CHANNEL_POSITION_SIDE_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_TOP_CENTER,
        GST_AUDIO_CHANNEL_POSITION_TOP_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_TOP_FRONT_CENTER,
        GST_AUDIO_CHANNEL_POSITION_TOP_FRONT_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_TOP_REAR_LEFT,
        GST_AUDIO_CHANNEL_POSITION_TOP_REAR_CENTER,
        GST_AUDIO_CHANNEL_POSITION_TOP_REAR_RIGHT,
    };

    for (i = 0; i < channel_count; ++i)
    {
        positions[i] = GST_AUDIO_CHANNEL_POSITION_NONE;
        if (BitScanForward(&bit, mask))
        {
            if (bit < ARRAY_SIZE(position_map))
                positions[i] = position_map[bit];
            else
                GST_WARNING("Invalid channel mask %#x.", orig_mask);
            mask &= ~(1 << bit);
        }
        else
        {
            GST_WARNING("Incomplete channel mask %#x.", orig_mask);
        }
    }
}

static GstCaps *wg_format_to_caps_audio_mpeg1(const struct wg_format *format)
{
    GstCaps *caps;

    if (!(caps = gst_caps_new_empty_simple("audio/mpeg")))
        return NULL;

    gst_caps_set_simple(caps, "mpegversion", G_TYPE_INT, 1, NULL);
    gst_caps_set_simple(caps, "layer", G_TYPE_INT, format->u.audio_mpeg1.layer, NULL);
    gst_caps_set_simple(caps, "rate", G_TYPE_INT, format->u.audio_mpeg1.rate, NULL);
    gst_caps_set_simple(caps, "channels", G_TYPE_INT, format->u.audio_mpeg1.channels, NULL);
    gst_caps_set_simple(caps, "parsed", G_TYPE_BOOLEAN, TRUE, NULL);

    return caps;
}

static GstCaps *wg_format_to_caps_audio(const struct wg_format *format)
{
    GstAudioChannelPosition positions[32];
    GstAudioFormat audio_format;
    GstAudioInfo info;

    if ((audio_format = wg_audio_format_to_gst(format->u.audio.format)) == GST_AUDIO_FORMAT_UNKNOWN)
        return NULL;

    wg_channel_mask_to_gst(positions, format->u.audio.channel_mask, format->u.audio.channels);
    gst_audio_info_set_format(&info, audio_format, format->u.audio.rate, format->u.audio.channels, positions);
    return gst_audio_info_to_caps(&info);
}

static GstVideoFormat wg_video_format_to_gst(enum wg_video_format format)
{
    switch (format)
    {
        case WG_VIDEO_FORMAT_BGRA:  return GST_VIDEO_FORMAT_BGRA;
        case WG_VIDEO_FORMAT_BGRx:  return GST_VIDEO_FORMAT_BGRx;
        case WG_VIDEO_FORMAT_BGR:   return GST_VIDEO_FORMAT_BGR;
        case WG_VIDEO_FORMAT_RGB15: return GST_VIDEO_FORMAT_RGB15;
        case WG_VIDEO_FORMAT_RGB16: return GST_VIDEO_FORMAT_RGB16;
        case WG_VIDEO_FORMAT_AYUV:  return GST_VIDEO_FORMAT_AYUV;
        case WG_VIDEO_FORMAT_I420:  return GST_VIDEO_FORMAT_I420;
        case WG_VIDEO_FORMAT_NV12:  return GST_VIDEO_FORMAT_NV12;
        case WG_VIDEO_FORMAT_UYVY:  return GST_VIDEO_FORMAT_UYVY;
        case WG_VIDEO_FORMAT_YUY2:  return GST_VIDEO_FORMAT_YUY2;
        case WG_VIDEO_FORMAT_YV12:  return GST_VIDEO_FORMAT_YV12;
        case WG_VIDEO_FORMAT_YVYU:  return GST_VIDEO_FORMAT_YVYU;
        default: return GST_VIDEO_FORMAT_UNKNOWN;
    }
}

static GstCaps *wg_format_to_caps_video(const struct wg_format *format)
{
    GstVideoFormat video_format;
    GstVideoInfo info;
    unsigned int i;
    GstCaps *caps;

    if ((video_format = wg_video_format_to_gst(format->u.video.format)) == GST_VIDEO_FORMAT_UNKNOWN)
        return NULL;

    gst_video_info_set_format(&info, video_format, format->u.video.width, abs(format->u.video.height));
    if ((caps = gst_video_info_to_caps(&info)))
    {
        for (i = 0; i < gst_caps_get_size(caps); ++i)
        {
            if (!format->u.video.width)
                gst_structure_remove_fields(gst_caps_get_structure(caps, i), "width", NULL);
            if (!format->u.video.height)
                gst_structure_remove_fields(gst_caps_get_structure(caps, i), "height", NULL);
            if (!format->u.video.fps_d && !format->u.video.fps_n)
                gst_structure_remove_fields(gst_caps_get_structure(caps, i), "framerate", NULL);
        }
    }
    return caps;
}

static GstCaps *wg_format_to_caps_video_cinepak(const struct wg_format *format)
{
    GstCaps *caps;

    if (!(caps = gst_caps_new_empty_simple("video/x-cinepak")))
        return NULL;

    if (format->u.video_cinepak.width)
        gst_caps_set_simple(caps, "width", G_TYPE_INT, format->u.video_cinepak.width, NULL);
    if (format->u.video_cinepak.height)
        gst_caps_set_simple(caps, "height", G_TYPE_INT, format->u.video_cinepak.height, NULL);
    if (format->u.video_cinepak.fps_d || format->u.video_cinepak.fps_n)
        gst_caps_set_simple(caps, "framerate", GST_TYPE_FRACTION, format->u.video_cinepak.fps_n, format->u.video_cinepak.fps_d, NULL);

    return caps;
}

static GstCaps *wg_format_to_caps_audio_wma(const struct wg_format *format)
{
    GstBuffer *buffer;
    GstCaps *caps;

    if (!(caps = gst_caps_new_empty_simple("audio/x-wma")))
        return NULL;
    if (format->u.audio_wma.version)
        gst_caps_set_simple(caps, "wmaversion", G_TYPE_INT, format->u.audio_wma.version, NULL);

    if (format->u.audio_wma.bitrate)
        gst_caps_set_simple(caps, "bitrate", G_TYPE_INT, format->u.audio_wma.bitrate, NULL);
    if (format->u.audio_wma.rate)
        gst_caps_set_simple(caps, "rate", G_TYPE_INT, format->u.audio_wma.rate, NULL);
    if (format->u.audio_wma.depth)
        gst_caps_set_simple(caps, "depth", G_TYPE_INT, format->u.audio_wma.depth, NULL);
    if (format->u.audio_wma.channels)
        gst_caps_set_simple(caps, "channels", G_TYPE_INT, format->u.audio_wma.channels, NULL);
    if (format->u.audio_wma.block_align)
        gst_caps_set_simple(caps, "block_align", G_TYPE_INT, format->u.audio_wma.block_align, NULL);

    if (format->u.audio_wma.codec_data_len)
    {
        if (!(buffer = gst_buffer_new_and_alloc(format->u.audio_wma.codec_data_len)))
        {
            gst_caps_unref(caps);
            return NULL;
        }

        gst_buffer_fill(buffer, 0, format->u.audio_wma.codec_data, format->u.audio_wma.codec_data_len);
        gst_caps_set_simple(caps, "codec_data", GST_TYPE_BUFFER, buffer, NULL);
        gst_buffer_unref(buffer);
    }

    return caps;
}

static GstCaps *wg_format_to_caps_video_h264(const struct wg_format *format)
{
    const char *profile, *level;
    GstCaps *caps;

    if (!(caps = gst_caps_new_empty_simple("video/x-h264")))
        return NULL;
    gst_caps_set_simple(caps, "stream-format", G_TYPE_STRING, "byte-stream", NULL);
    gst_caps_set_simple(caps, "alignment", G_TYPE_STRING, "au", NULL);

    if (format->u.video_h264.width)
        gst_caps_set_simple(caps, "width", G_TYPE_INT, format->u.video_h264.width, NULL);
    if (format->u.video_h264.height)
        gst_caps_set_simple(caps, "height", G_TYPE_INT, format->u.video_h264.height, NULL);
    if (format->u.video_h264.fps_n || format->u.video_h264.fps_d)
        gst_caps_set_simple(caps, "framerate", GST_TYPE_FRACTION, format->u.video_h264.fps_n, format->u.video_h264.fps_d, NULL);

    switch (format->u.video_h264.profile)
    {
        case eAVEncH264VProfile_Main: profile = "main"; break;
        case eAVEncH264VProfile_High: profile = "high"; break;
        case eAVEncH264VProfile_444:  profile = "high-4:4:4"; break;
        default:
            GST_FIXME("H264 profile attribute %u not implemented.", format->u.video_h264.profile);
            /* fallthrough */
        case eAVEncH264VProfile_unknown:
            profile = NULL;
            break;
    }
    if (profile)
        gst_caps_set_simple(caps, "profile", G_TYPE_STRING, profile, NULL);

    switch (format->u.video_h264.level)
    {
        case eAVEncH264VLevel1:   level = "1";   break;
        case eAVEncH264VLevel1_1: level = "1.1"; break;
        case eAVEncH264VLevel1_2: level = "1.2"; break;
        case eAVEncH264VLevel1_3: level = "1.3"; break;
        case eAVEncH264VLevel2:   level = "2";   break;
        case eAVEncH264VLevel2_1: level = "2.1"; break;
        case eAVEncH264VLevel2_2: level = "2.2"; break;
        case eAVEncH264VLevel3:   level = "3";   break;
        case eAVEncH264VLevel3_1: level = "3.1"; break;
        case eAVEncH264VLevel3_2: level = "3.2"; break;
        case eAVEncH264VLevel4:   level = "4";   break;
        case eAVEncH264VLevel4_1: level = "4.1"; break;
        case eAVEncH264VLevel4_2: level = "4.2"; break;
        case eAVEncH264VLevel5:   level = "5";   break;
        case eAVEncH264VLevel5_1: level = "5.1"; break;
        case eAVEncH264VLevel5_2: level = "5.2"; break;
        default:
            GST_FIXME("H264 level attribute %u not implemented.", format->u.video_h264.level);
            /* fallthrough */
        case 0:
            level = NULL;
            break;
    }
    if (level)
        gst_caps_set_simple(caps, "level", G_TYPE_STRING, level, NULL);

    return caps;
}

GstCaps *wg_format_to_caps(const struct wg_format *format)
{
    switch (format->major_type)
    {
        case WG_MAJOR_TYPE_UNKNOWN:
            return gst_caps_new_any();
        case WG_MAJOR_TYPE_AUDIO:
            return wg_format_to_caps_audio(format);
        case WG_MAJOR_TYPE_AUDIO_MPEG1:
            return wg_format_to_caps_audio_mpeg1(format);
        case WG_MAJOR_TYPE_AUDIO_WMA:
            return wg_format_to_caps_audio_wma(format);
        case WG_MAJOR_TYPE_VIDEO:
            return wg_format_to_caps_video(format);
        case WG_MAJOR_TYPE_VIDEO_CINEPAK:
            return wg_format_to_caps_video_cinepak(format);
        case WG_MAJOR_TYPE_VIDEO_H264:
            return wg_format_to_caps_video_h264(format);
    }
    assert(0);
    return NULL;
}

bool wg_format_compare(const struct wg_format *a, const struct wg_format *b)
{
    if (a->major_type != b->major_type)
        return false;

    switch (a->major_type)
    {
        case WG_MAJOR_TYPE_AUDIO_MPEG1:
        case WG_MAJOR_TYPE_AUDIO_WMA:
        case WG_MAJOR_TYPE_VIDEO_H264:
            GST_FIXME("Format %u not implemented!", a->major_type);
            /* fallthrough */
        case WG_MAJOR_TYPE_UNKNOWN:
            return false;

        case WG_MAJOR_TYPE_AUDIO:
            return a->u.audio.format == b->u.audio.format
                    && a->u.audio.channels == b->u.audio.channels
                    && a->u.audio.rate == b->u.audio.rate;

        case WG_MAJOR_TYPE_VIDEO:
            /* Do not compare FPS. */
            return a->u.video.format == b->u.video.format
                    && a->u.video.width == b->u.video.width
                    && abs(a->u.video.height) == abs(b->u.video.height)
                    && EqualRect( &a->u.video.padding, &b->u.video.padding );

        case WG_MAJOR_TYPE_VIDEO_CINEPAK:
            /* Do not compare FPS. */
            return a->u.video_cinepak.width == b->u.video_cinepak.width
                    && a->u.video_cinepak.height == b->u.video_cinepak.height;
    }

    assert(0);
    return false;
}
