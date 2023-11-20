/*
 * DirectShow parser filters
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

#include "gst_private.h"
#include "gst_guids.h"

#include "amvideo.h"

#include <limits.h>
#include "dvdmedia.h"
#include "mmreg.h"
#include "ks.h"
#include "wmcodecdsp.h"
#include "initguid.h"
#include "ksmedia.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static const GUID MEDIASUBTYPE_CVID = {mmioFOURCC('c','v','i','d'), 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
static const GUID MEDIASUBTYPE_VC1S = {mmioFOURCC('V','C','1','S'), 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
static const GUID MEDIASUBTYPE_MP3  = {WAVE_FORMAT_MPEGLAYER3, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
static const GUID MEDIASUBTYPE_WMV_Unknown = {0x7ce12ca9, 0xbfbf, 0x43d9, {0x9d, 0x00, 0x82, 0xb8, 0xed, 0x54, 0x31, 0x6b}};

struct parser
{
    struct strmbase_filter filter;
    IAMStreamSelect IAMStreamSelect_iface;

    struct strmbase_sink sink;
    IAsyncReader *reader;

    struct parser_source **sources;
    unsigned int source_count;
    BOOL enum_sink_first;

    wg_parser_t wg_parser;

    /* This protects the "streaming" and "flushing" fields, accessed by both
     * the application and streaming threads.
     * We cannot use the filter lock for this, since that is held while waiting
     * for the streaming thread, and hence the streaming thread cannot take the
     * filter lock.
     * This lock must not be acquired before acquiring the filter lock or
     * flushing_cs. */
    CRITICAL_SECTION streaming_cs;
    CONDITION_VARIABLE flushing_cv;

    /* FIXME: It would be nice to avoid duplicating these with strmbase.
     * However, synchronization is tricky; we need access to be protected by a
     * separate lock. */
    bool streaming, sink_connected;

    bool flushing;

    HANDLE read_thread;

    BOOL (*init_gst)(struct parser *filter);
    HRESULT (*source_query_accept)(struct parser_source *pin, const AM_MEDIA_TYPE *mt);
    HRESULT (*source_get_media_type)(struct parser_source *pin, unsigned int index, AM_MEDIA_TYPE *mt);
};

struct parser_source
{
    struct strmbase_source pin;
    IQualityControl IQualityControl_iface;

    wg_parser_stream_t wg_stream;

    SourceSeeking seek;

    CRITICAL_SECTION flushing_cs;
    CONDITION_VARIABLE eos_cv;
    HANDLE thread;

    /* This variable is read and written by both the streaming thread and
     * application threads. However, it is only written by the application
     * thread when the streaming thread is not running, or when it is blocked
     * by flushing_cs. */
    bool need_segment;

    bool eos;
};

static inline struct parser *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct parser, filter);
}

static const IMediaSeekingVtbl GST_Seeking_Vtbl;
static const IQualityControlVtbl GSTOutPin_QualityControl_Vtbl;

static struct parser_source *create_pin(struct parser *filter,
        wg_parser_stream_t stream, const WCHAR *name);
static HRESULT GST_RemoveOutputPins(struct parser *This);
static HRESULT WINAPI GST_ChangeCurrent(IMediaSeeking *iface);
static HRESULT WINAPI GST_ChangeStop(IMediaSeeking *iface);
static HRESULT WINAPI GST_ChangeRate(IMediaSeeking *iface);

static bool wg_audio_format_is_float(enum wg_audio_format format)
{
    switch (format)
    {
        case WG_AUDIO_FORMAT_UNKNOWN: return false;
        case WG_AUDIO_FORMAT_U8: return false;
        case WG_AUDIO_FORMAT_S16LE: return false;
        case WG_AUDIO_FORMAT_S24LE: return false;
        case WG_AUDIO_FORMAT_S32LE: return false;
        case WG_AUDIO_FORMAT_F32LE: return true;
        case WG_AUDIO_FORMAT_F64LE: return true;
    }

    assert(0);
    return false;
}

static WORD wg_audio_format_get_depth(enum wg_audio_format format)
{
    switch (format)
    {
        case WG_AUDIO_FORMAT_UNKNOWN: return 0;
        case WG_AUDIO_FORMAT_U8: return 8;
        case WG_AUDIO_FORMAT_S16LE: return 16;
        case WG_AUDIO_FORMAT_S24LE: return 24;
        case WG_AUDIO_FORMAT_S32LE: return 32;
        case WG_AUDIO_FORMAT_F32LE: return 32;
        case WG_AUDIO_FORMAT_F64LE: return 64;
    }

    assert(0);
    return 0;
}

static bool amt_from_wg_format_audio(AM_MEDIA_TYPE *mt, const struct wg_format *format)
{
    mt->majortype = MEDIATYPE_Audio;
    mt->formattype = FORMAT_WaveFormatEx;

    switch (format->u.audio.format)
    {
    case WG_AUDIO_FORMAT_UNKNOWN:
        return false;

    case WG_AUDIO_FORMAT_U8:
    case WG_AUDIO_FORMAT_S16LE:
    case WG_AUDIO_FORMAT_S24LE:
    case WG_AUDIO_FORMAT_S32LE:
    case WG_AUDIO_FORMAT_F32LE:
    case WG_AUDIO_FORMAT_F64LE:
    {
        bool is_float = wg_audio_format_is_float(format->u.audio.format);
        WORD depth = wg_audio_format_get_depth(format->u.audio.format);

        if (is_float || format->u.audio.channels > 2)
        {
            WAVEFORMATEXTENSIBLE *wave_format;

            if (!(wave_format = CoTaskMemAlloc(sizeof(*wave_format))))
                return false;
            memset(wave_format, 0, sizeof(*wave_format));

            mt->subtype = is_float ? MEDIASUBTYPE_IEEE_FLOAT : MEDIASUBTYPE_PCM;
            mt->bFixedSizeSamples = TRUE;
            mt->pbFormat = (BYTE *)wave_format;
            mt->cbFormat = sizeof(*wave_format);
            wave_format->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
            wave_format->Format.nChannels = format->u.audio.channels;
            wave_format->Format.nSamplesPerSec = format->u.audio.rate;
            wave_format->Format.nAvgBytesPerSec = format->u.audio.rate * format->u.audio.channels * depth / 8;
            wave_format->Format.nBlockAlign = format->u.audio.channels * depth / 8;
            wave_format->Format.wBitsPerSample = depth;
            wave_format->Format.cbSize = sizeof(*wave_format) - sizeof(WAVEFORMATEX);
            wave_format->Samples.wValidBitsPerSample = depth;
            wave_format->dwChannelMask = format->u.audio.channel_mask;
            wave_format->SubFormat = is_float ? KSDATAFORMAT_SUBTYPE_IEEE_FLOAT : KSDATAFORMAT_SUBTYPE_PCM;
            mt->lSampleSize = wave_format->Format.nBlockAlign;
        }
        else
        {
            WAVEFORMATEX *wave_format;

            if (!(wave_format = CoTaskMemAlloc(sizeof(*wave_format))))
                return false;
            memset(wave_format, 0, sizeof(*wave_format));

            mt->subtype = MEDIASUBTYPE_PCM;
            mt->bFixedSizeSamples = TRUE;
            mt->pbFormat = (BYTE *)wave_format;
            mt->cbFormat = sizeof(*wave_format);
            wave_format->wFormatTag = WAVE_FORMAT_PCM;
            wave_format->nChannels = format->u.audio.channels;
            wave_format->nSamplesPerSec = format->u.audio.rate;
            wave_format->nAvgBytesPerSec = format->u.audio.rate * format->u.audio.channels * depth / 8;
            wave_format->nBlockAlign = format->u.audio.channels * depth / 8;
            wave_format->wBitsPerSample = depth;
            wave_format->cbSize = 0;
            mt->lSampleSize = wave_format->nBlockAlign;
        }
        return true;
    }
    }

    assert(0);
    return false;
}

static bool amt_from_wg_format_audio_mpeg1(AM_MEDIA_TYPE *mt, const struct wg_format *format)
{
    mt->majortype = MEDIATYPE_Audio;
    mt->formattype = FORMAT_WaveFormatEx;

    switch (format->u.audio_mpeg1.layer)
    {
        case 1:
        case 2:
        {
            MPEG1WAVEFORMAT *wave_format;

            if (!(wave_format = CoTaskMemAlloc(sizeof(*wave_format))))
                return false;
            memset(wave_format, 0, sizeof(*wave_format));

            mt->subtype = MEDIASUBTYPE_MPEG1AudioPayload;
            mt->cbFormat = sizeof(*wave_format);
            mt->pbFormat = (BYTE *)wave_format;
            wave_format->wfx.wFormatTag = WAVE_FORMAT_MPEG;
            wave_format->wfx.nChannels = format->u.audio_mpeg1.channels;
            wave_format->wfx.nSamplesPerSec = format->u.audio_mpeg1.rate;
            wave_format->wfx.cbSize = sizeof(*wave_format) - sizeof(WAVEFORMATEX);
            wave_format->fwHeadLayer = format->u.audio_mpeg1.layer;
            return true;
        }

        case 3:
        {
            MPEGLAYER3WAVEFORMAT *wave_format;

            if (!(wave_format = CoTaskMemAlloc(sizeof(*wave_format))))
                return false;
            memset(wave_format, 0, sizeof(*wave_format));

            mt->subtype = MEDIASUBTYPE_MP3;
            mt->cbFormat = sizeof(*wave_format);
            mt->pbFormat = (BYTE *)wave_format;
            wave_format->wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3;
            wave_format->wfx.nChannels = format->u.audio_mpeg1.channels;
            wave_format->wfx.nSamplesPerSec = format->u.audio_mpeg1.rate;
            wave_format->wfx.cbSize = sizeof(*wave_format) - sizeof(WAVEFORMATEX);
            /* FIXME: We can't get most of the MPEG data from the caps. We may have
             * to manually parse the header. */
            wave_format->wID = MPEGLAYER3_ID_MPEG;
            wave_format->fdwFlags = MPEGLAYER3_FLAG_PADDING_ON;
            wave_format->nFramesPerBlock = 1;
            wave_format->nCodecDelay = 1393;
            return true;
        }
    }

    assert(0);
    return false;
}

static bool amt_from_wg_format_audio_wma(AM_MEDIA_TYPE *mt, const struct wg_format *format)
{
    DWORD codec_data_len, size;
    WAVEFORMATEX *wave_format;
    const GUID *subtype;
    WORD fmt_tag;

    mt->majortype = MEDIATYPE_Audio;
    mt->formattype = FORMAT_WaveFormatEx;

    switch (format->u.audio_wma.version)
    {
        case 1:
            subtype = &MEDIASUBTYPE_MSAUDIO1;
            codec_data_len = MSAUDIO1_WFX_EXTRA_BYTES;
            fmt_tag = WAVE_FORMAT_MSAUDIO1;
            break;
        case 2:
            subtype = &MEDIASUBTYPE_WMAUDIO2;
            codec_data_len = WMAUDIO2_WFX_EXTRA_BYTES;
            fmt_tag = WAVE_FORMAT_WMAUDIO2;
            break;
        case 3:
            subtype = &MEDIASUBTYPE_WMAUDIO3;
            codec_data_len = WMAUDIO3_WFX_EXTRA_BYTES;
            fmt_tag = WAVE_FORMAT_WMAUDIO3;
            break;
        case 4:
            subtype = &MEDIASUBTYPE_WMAUDIO_LOSSLESS;
            codec_data_len = 18;
            fmt_tag = WAVE_FORMAT_WMAUDIO_LOSSLESS;
            break;
        default:
            assert(false);
            return false;
    }

    size = sizeof(WAVEFORMATEX) + codec_data_len;
    if (!(wave_format = CoTaskMemAlloc(size)))
        return false;
    memset(wave_format, 0, size);

    mt->subtype = *subtype;
    mt->bFixedSizeSamples = TRUE;
    mt->lSampleSize = format->u.audio_wma.block_align;
    mt->cbFormat = size;
    mt->pbFormat = (BYTE *)wave_format;
    wave_format->wFormatTag = fmt_tag;
    wave_format->nChannels = format->u.audio_wma.channels;
    wave_format->nSamplesPerSec = format->u.audio_wma.rate;
    wave_format->nAvgBytesPerSec = format->u.audio_wma.bitrate / 8;
    wave_format->nBlockAlign = format->u.audio_wma.block_align;
    wave_format->wBitsPerSample = format->u.audio_wma.depth;
    wave_format->cbSize = codec_data_len;

    if (format->u.audio_wma.codec_data_len == codec_data_len)
        memcpy(wave_format+1, format->u.audio_wma.codec_data, format->u.audio_wma.codec_data_len);
    else
        FIXME("Unexpected codec_data length; got %u, expected %lu\n", format->u.audio_wma.codec_data_len, codec_data_len);
    return true;
}

#define ALIGN(n, alignment) (((n) + (alignment) - 1) & ~((alignment) - 1))

static unsigned int wg_format_get_max_size_video_raw(enum wg_video_format format, unsigned int width, unsigned int height)
{
    switch (format)
    {
        case WG_VIDEO_FORMAT_BGRA:
        case WG_VIDEO_FORMAT_BGRx:
        case WG_VIDEO_FORMAT_AYUV:
            return width * height * 4;

        case WG_VIDEO_FORMAT_BGR:
            return ALIGN(width * 3, 4) * height;

        case WG_VIDEO_FORMAT_RGB15:
        case WG_VIDEO_FORMAT_RGB16:
        case WG_VIDEO_FORMAT_UYVY:
        case WG_VIDEO_FORMAT_YUY2:
        case WG_VIDEO_FORMAT_YVYU:
            return ALIGN(width * 2, 4) * height;

        case WG_VIDEO_FORMAT_I420:
        case WG_VIDEO_FORMAT_YV12:
            return ALIGN(width, 4) * ALIGN(height, 2) /* Y plane */
                    + 2 * ALIGN((width + 1) / 2, 4) * ((height + 1) / 2); /* U and V planes */

        case WG_VIDEO_FORMAT_NV12:
            return ALIGN(width, 4) * ALIGN(height, 2) /* Y plane */
                    + ALIGN(width, 4) * ((height + 1) / 2); /* U/V plane */

        case WG_VIDEO_FORMAT_UNKNOWN:
            FIXME("Cannot guess maximum sample size for unknown video format.\n");
            return 0;
    }

    assert(0);
    return 0;
}

unsigned int wg_format_get_max_size(const struct wg_format *format)
{
    switch (format->major_type)
    {
        case WG_MAJOR_TYPE_VIDEO:
            return wg_format_get_max_size_video_raw(format->u.video.format,
                    format->u.video.width, abs(format->u.video.height));

        case WG_MAJOR_TYPE_VIDEO_CINEPAK:
            /* Both ffmpeg's encoder and a Cinepak file seen in the wild report
             * 24 bpp. ffmpeg sets biSizeImage as below; others may be smaller,
             * but as long as every sample fits into our allocator, we're fine. */
            return format->u.video_cinepak.width * format->u.video_cinepak.height * 3;

        case WG_MAJOR_TYPE_VIDEO_MPEG1:
            /* Estimated max size of a compressed video frame.
             * There's no way to no way to know the real upper bound,
             * so let's just use the decompressed size and hope it works. */
            return wg_format_get_max_size_video_raw(WG_VIDEO_FORMAT_YV12,
                    format->u.video_mpeg1.width, format->u.video_mpeg1.height);

        case WG_MAJOR_TYPE_VIDEO_WMV:
            return wg_format_get_max_size_video_raw(WG_VIDEO_FORMAT_YV12,
                    format->u.video_wmv.width, format->u.video_wmv.height);

        case WG_MAJOR_TYPE_AUDIO:
        {
            unsigned int rate = format->u.audio.rate, channels = format->u.audio.channels;

            /* Actually we don't know how large of a sample GStreamer will give
             * us. Hopefully 1 second is enough... */

            switch (format->u.audio.format)
            {
                case WG_AUDIO_FORMAT_U8:
                    return rate * channels;

                case WG_AUDIO_FORMAT_S16LE:
                    return rate * channels * 2;

                case WG_AUDIO_FORMAT_S24LE:
                    return rate * channels * 3;

                case WG_AUDIO_FORMAT_S32LE:
                case WG_AUDIO_FORMAT_F32LE:
                    return rate * channels * 4;

                case WG_AUDIO_FORMAT_F64LE:
                    return rate * channels * 8;

                case WG_AUDIO_FORMAT_UNKNOWN:
                    FIXME("Cannot guess maximum sample size for unknown audio format.\n");
                    return 0;
            }
            break;
        }

        case WG_MAJOR_TYPE_AUDIO_MPEG1:
            switch (format->u.audio_mpeg1.layer)
            {
            case 1:
                return 56000;

            case 2:
                return 48000;

            case 3:
                return 40000;
            }
            break;

        case WG_MAJOR_TYPE_AUDIO_WMA:
            /* Estimated max size of a compressed audio frame.
             * There's no way to no way to know the real upper bound,
             * so let's just use one second of decompressed size and hope it works. */
            return format->u.audio_wma.rate * format->u.audio_wma.channels * format->u.audio_wma.depth / 8;

        case WG_MAJOR_TYPE_AUDIO_MPEG4:
        case WG_MAJOR_TYPE_VIDEO_H264:
        case WG_MAJOR_TYPE_VIDEO_INDEO:
            FIXME("Format %u not implemented!\n", format->major_type);
            return 0;

        case WG_MAJOR_TYPE_UNKNOWN:
            FIXME("Cannot guess maximum sample size for unknown format.\n");
            return 0;
    }

    assert(0);
    return 0;
}

static const GUID *wg_video_format_get_mediasubtype(enum wg_video_format format)
{
    switch (format)
    {
        case WG_VIDEO_FORMAT_UNKNOWN: return &GUID_NULL;
        case WG_VIDEO_FORMAT_BGRA: return &MEDIASUBTYPE_ARGB32;
        case WG_VIDEO_FORMAT_BGRx: return &MEDIASUBTYPE_RGB32;
        case WG_VIDEO_FORMAT_BGR: return &MEDIASUBTYPE_RGB24;
        case WG_VIDEO_FORMAT_RGB15: return &MEDIASUBTYPE_RGB555;
        case WG_VIDEO_FORMAT_RGB16: return &MEDIASUBTYPE_RGB565;
        case WG_VIDEO_FORMAT_AYUV: return &MEDIASUBTYPE_AYUV;
        case WG_VIDEO_FORMAT_I420: return &MEDIASUBTYPE_I420;
        case WG_VIDEO_FORMAT_NV12: return &MEDIASUBTYPE_NV12;
        case WG_VIDEO_FORMAT_UYVY: return &MEDIASUBTYPE_UYVY;
        case WG_VIDEO_FORMAT_YUY2: return &MEDIASUBTYPE_YUY2;
        case WG_VIDEO_FORMAT_YV12: return &MEDIASUBTYPE_YV12;
        case WG_VIDEO_FORMAT_YVYU: return &MEDIASUBTYPE_YVYU;
    }

    assert(0);
    return NULL;
}

static DWORD wg_video_format_get_compression(enum wg_video_format format)
{
    switch (format)
    {
        case WG_VIDEO_FORMAT_UNKNOWN: return 0;
        case WG_VIDEO_FORMAT_BGRA: return BI_RGB;
        case WG_VIDEO_FORMAT_BGRx: return BI_RGB;
        case WG_VIDEO_FORMAT_BGR: return BI_RGB;
        case WG_VIDEO_FORMAT_RGB15: return BI_RGB;
        case WG_VIDEO_FORMAT_RGB16: return BI_BITFIELDS;
        case WG_VIDEO_FORMAT_AYUV: return mmioFOURCC('A','Y','U','V');
        case WG_VIDEO_FORMAT_I420: return mmioFOURCC('I','4','2','0');
        case WG_VIDEO_FORMAT_NV12: return mmioFOURCC('N','V','1','2');
        case WG_VIDEO_FORMAT_UYVY: return mmioFOURCC('U','Y','V','Y');
        case WG_VIDEO_FORMAT_YUY2: return mmioFOURCC('Y','U','Y','2');
        case WG_VIDEO_FORMAT_YV12: return mmioFOURCC('Y','V','1','2');
        case WG_VIDEO_FORMAT_YVYU: return mmioFOURCC('Y','V','Y','U');
    }

    assert(0);
    return 0;
}

static WORD wg_video_format_get_depth(enum wg_video_format format)
{
    switch (format)
    {
        case WG_VIDEO_FORMAT_UNKNOWN: return 0;
        case WG_VIDEO_FORMAT_BGRA: return 32;
        case WG_VIDEO_FORMAT_BGRx: return 32;
        case WG_VIDEO_FORMAT_BGR: return 24;
        case WG_VIDEO_FORMAT_RGB15: return 16;
        case WG_VIDEO_FORMAT_RGB16: return 16;
        case WG_VIDEO_FORMAT_AYUV: return 32;
        case WG_VIDEO_FORMAT_I420: return 12;
        case WG_VIDEO_FORMAT_NV12: return 12;
        case WG_VIDEO_FORMAT_UYVY: return 16;
        case WG_VIDEO_FORMAT_YUY2: return 16;
        case WG_VIDEO_FORMAT_YV12: return 12;
        case WG_VIDEO_FORMAT_YVYU: return 16;
    }

    assert(0);
    return 0;
}

static bool amt_from_wg_format_video(AM_MEDIA_TYPE *mt, const struct wg_format *format, bool wm)
{
    VIDEOINFO *video_format;
    uint32_t frame_time;

    if (format->u.video.format == WG_VIDEO_FORMAT_UNKNOWN)
        return false;

    if (!(video_format = CoTaskMemAlloc(sizeof(*video_format))))
        return false;

    mt->majortype = MEDIATYPE_Video;
    mt->subtype = *wg_video_format_get_mediasubtype(format->u.video.format);
    if (wm)
        mt->bFixedSizeSamples = TRUE;
    else
        mt->bTemporalCompression = TRUE;
    mt->lSampleSize = 1;
    mt->formattype = FORMAT_VideoInfo;
    mt->cbFormat = sizeof(VIDEOINFOHEADER);
    mt->pbFormat = (BYTE *)video_format;

    memset(video_format, 0, sizeof(*video_format));

    if (wm)
    {
        SetRect(&video_format->rcSource, 0, 0, format->u.video.width, abs(format->u.video.height));
        video_format->rcTarget = video_format->rcSource;
    }
    if ((frame_time = MulDiv(10000000, format->u.video.fps_d, format->u.video.fps_n)) != -1)
        video_format->AvgTimePerFrame = frame_time;
    video_format->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    video_format->bmiHeader.biWidth = format->u.video.width;
    video_format->bmiHeader.biHeight = format->u.video.height;
    if (wg_video_format_is_rgb(format->u.video.format))
        video_format->bmiHeader.biHeight = -format->u.video.height;
    video_format->bmiHeader.biPlanes = 1;
    video_format->bmiHeader.biBitCount = wg_video_format_get_depth(format->u.video.format);
    video_format->bmiHeader.biCompression = wg_video_format_get_compression(format->u.video.format);
    video_format->bmiHeader.biSizeImage = wg_format_get_max_size(format);

    if (format->u.video.format == WG_VIDEO_FORMAT_RGB16)
    {
        mt->cbFormat = offsetof(VIDEOINFO, dwBitMasks[3]);
        video_format->dwBitMasks[iRED]   = 0xf800;
        video_format->dwBitMasks[iGREEN] = 0x07e0;
        video_format->dwBitMasks[iBLUE]  = 0x001f;
    }

    return true;
}

static bool amt_from_wg_format_video_cinepak(AM_MEDIA_TYPE *mt, const struct wg_format *format)
{
    VIDEOINFOHEADER *video_format;
    uint32_t frame_time;

    if (!(video_format = CoTaskMemAlloc(sizeof(*video_format))))
        return false;

    mt->majortype = MEDIATYPE_Video;
    mt->subtype = MEDIASUBTYPE_CVID;
    mt->bTemporalCompression = TRUE;
    mt->lSampleSize = 1;
    mt->formattype = FORMAT_VideoInfo;
    mt->cbFormat = sizeof(*video_format);
    mt->pbFormat = (BYTE *)video_format;

    memset(video_format, 0, sizeof(*video_format));
    if ((frame_time = MulDiv(10000000, format->u.video_cinepak.fps_d, format->u.video_cinepak.fps_n)) != -1)
        video_format->AvgTimePerFrame = frame_time;
    video_format->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    video_format->bmiHeader.biWidth = format->u.video_cinepak.width;
    video_format->bmiHeader.biHeight = format->u.video_cinepak.height;
    video_format->bmiHeader.biPlanes = 1;
    video_format->bmiHeader.biBitCount = 24;
    video_format->bmiHeader.biCompression = mt->subtype.Data1;
    video_format->bmiHeader.biSizeImage = wg_format_get_max_size(format);

    return true;
}

static bool amt_from_wg_format_video_wmv(AM_MEDIA_TYPE *mt, const struct wg_format *format)
{
    VIDEOINFOHEADER *video_format;
    uint32_t frame_time;
    const GUID *subtype;

    switch (format->u.video_wmv.format)
    {
        case WG_WMV_VIDEO_FORMAT_WMV1:
            subtype = &MEDIASUBTYPE_WMV1;
            break;
        case WG_WMV_VIDEO_FORMAT_WMV2:
            subtype = &MEDIASUBTYPE_WMV2;
            break;
        case WG_WMV_VIDEO_FORMAT_WMV3:
            subtype = &MEDIASUBTYPE_WMV3;
            break;
        case WG_WMV_VIDEO_FORMAT_WMVA:
            subtype = &MEDIASUBTYPE_WMVA;
            break;
        case WG_WMV_VIDEO_FORMAT_WVC1:
            subtype = &MEDIASUBTYPE_WVC1;
            break;
        default:
            WARN("Invalid WMV format %u.\n", format->u.video_wmv.format);
            return false;
    }

    if (!(video_format = CoTaskMemAlloc(sizeof(*video_format) + format->u.video_wmv.codec_data_len)))
        return false;

    mt->majortype = MEDIATYPE_Video;
    mt->subtype = *subtype;
    mt->bFixedSizeSamples = FALSE;
    mt->bTemporalCompression = TRUE;
    mt->lSampleSize = 0;
    mt->formattype = FORMAT_VideoInfo;
    mt->cbFormat = sizeof(*video_format) + format->u.video_wmv.codec_data_len;
    mt->pbFormat = (BYTE *)video_format;

    memset(video_format, 0, sizeof(*video_format));
    SetRect(&video_format->rcSource, 0, 0, format->u.video_wmv.width, format->u.video_wmv.height);
    video_format->rcTarget = video_format->rcSource;
    if ((frame_time = MulDiv(10000000, format->u.video_wmv.fps_d, format->u.video_wmv.fps_n)) != -1)
        video_format->AvgTimePerFrame = frame_time;
    video_format->bmiHeader.biSize = sizeof(BITMAPINFOHEADER) + format->u.video_wmv.codec_data_len;
    video_format->bmiHeader.biWidth = format->u.video_wmv.width;
    video_format->bmiHeader.biHeight = format->u.video_wmv.height;
    video_format->bmiHeader.biPlanes = 1;
    video_format->bmiHeader.biCompression = mt->subtype.Data1;
    video_format->bmiHeader.biBitCount = 24;
    video_format->dwBitRate = 0;
    memcpy(video_format+1, format->u.video_wmv.codec_data, format->u.video_wmv.codec_data_len);

    return true;
}

static bool amt_from_wg_format_video_mpeg1(AM_MEDIA_TYPE *mt, const struct wg_format *format)
{
    MPEG1VIDEOINFO *video_format;
    uint32_t frame_time;

    if (!(video_format = CoTaskMemAlloc(sizeof(*video_format))))
        return false;

    mt->majortype = MEDIATYPE_Video;
    mt->subtype = MEDIASUBTYPE_MPEG1Payload;
    mt->bTemporalCompression = TRUE;
    mt->lSampleSize = 1;
    mt->formattype = FORMAT_MPEGVideo;
    mt->cbFormat = sizeof(MPEG1VIDEOINFO);
    mt->pbFormat = (BYTE *)video_format;

    memset(video_format, 0, sizeof(*video_format));
    if ((frame_time = MulDiv(10000000, format->u.video_mpeg1.fps_d, format->u.video_mpeg1.fps_n)) != -1)
        video_format->hdr.AvgTimePerFrame = frame_time;
    video_format->hdr.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    video_format->hdr.bmiHeader.biWidth = format->u.video_mpeg1.width;
    video_format->hdr.bmiHeader.biHeight = format->u.video_mpeg1.height;
    video_format->hdr.bmiHeader.biPlanes = 1;
    video_format->hdr.bmiHeader.biBitCount = 12;
    video_format->hdr.bmiHeader.biCompression = mt->subtype.Data1;
    video_format->hdr.bmiHeader.biSizeImage = wg_format_get_max_size(format);

    return true;
}

bool amt_from_wg_format(AM_MEDIA_TYPE *mt, const struct wg_format *format, bool wm)
{
    memset(mt, 0, sizeof(*mt));

    switch (format->major_type)
    {
    case WG_MAJOR_TYPE_AUDIO_MPEG4:
    case WG_MAJOR_TYPE_VIDEO_H264:
    case WG_MAJOR_TYPE_VIDEO_INDEO:
        FIXME("Format %u not implemented!\n", format->major_type);
        /* fallthrough */
    case WG_MAJOR_TYPE_UNKNOWN:
        return false;

    case WG_MAJOR_TYPE_AUDIO:
        return amt_from_wg_format_audio(mt, format);

    case WG_MAJOR_TYPE_AUDIO_MPEG1:
        return amt_from_wg_format_audio_mpeg1(mt, format);

    case WG_MAJOR_TYPE_AUDIO_WMA:
        return amt_from_wg_format_audio_wma(mt, format);

    case WG_MAJOR_TYPE_VIDEO:
        return amt_from_wg_format_video(mt, format, wm);

    case WG_MAJOR_TYPE_VIDEO_CINEPAK:
        return amt_from_wg_format_video_cinepak(mt, format);

    case WG_MAJOR_TYPE_VIDEO_WMV:
        return amt_from_wg_format_video_wmv(mt, format);

    case WG_MAJOR_TYPE_VIDEO_MPEG1:
        return amt_from_wg_format_video_mpeg1(mt, format);
    }

    assert(0);
    return false;
}

static bool amt_to_wg_format_audio(const AM_MEDIA_TYPE *mt, struct wg_format *format)
{
    static const struct
    {
        const GUID *subtype;
        WORD depth;
        enum wg_audio_format format;
    }
    format_map[] =
    {
        {&MEDIASUBTYPE_PCM,          8, WG_AUDIO_FORMAT_U8},
        {&MEDIASUBTYPE_PCM,         16, WG_AUDIO_FORMAT_S16LE},
        {&MEDIASUBTYPE_PCM,         24, WG_AUDIO_FORMAT_S24LE},
        {&MEDIASUBTYPE_PCM,         32, WG_AUDIO_FORMAT_S32LE},
        {&MEDIASUBTYPE_IEEE_FLOAT,  32, WG_AUDIO_FORMAT_F32LE},
        {&MEDIASUBTYPE_IEEE_FLOAT,  64, WG_AUDIO_FORMAT_F64LE},
    };

    const WAVEFORMATEX *audio_format = (const WAVEFORMATEX *)mt->pbFormat;
    unsigned int i;

    if (!IsEqualGUID(&mt->formattype, &FORMAT_WaveFormatEx))
    {
        FIXME("Unknown format type %s.\n", debugstr_guid(&mt->formattype));
        return false;
    }
    if (mt->cbFormat < sizeof(WAVEFORMATEX) || !mt->pbFormat)
    {
        ERR("Unexpected format size %lu.\n", mt->cbFormat);
        return false;
    }

    format->major_type = WG_MAJOR_TYPE_AUDIO;
    format->u.audio.channels = audio_format->nChannels;
    format->u.audio.rate = audio_format->nSamplesPerSec;

    if (audio_format->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        const WAVEFORMATEXTENSIBLE *ext_format = (const WAVEFORMATEXTENSIBLE *)mt->pbFormat;

        format->u.audio.channel_mask = ext_format->dwChannelMask;
    }
    else
    {
        if (audio_format->nChannels == 1)
            format->u.audio.channel_mask = KSAUDIO_SPEAKER_MONO;
        else if (audio_format->nChannels == 2)
            format->u.audio.channel_mask = KSAUDIO_SPEAKER_STEREO;
        else
        {
            ERR("Unexpected channel count %u.\n", audio_format->nChannels);
            return false;
        }
    }

    for (i = 0; i < ARRAY_SIZE(format_map); ++i)
    {
        if (IsEqualGUID(&mt->subtype, format_map[i].subtype)
                && audio_format->wBitsPerSample == format_map[i].depth)
        {
            format->u.audio.format = format_map[i].format;
            return true;
        }
    }

    FIXME("Unknown subtype %s, depth %u.\n", debugstr_guid(&mt->subtype), audio_format->wBitsPerSample);
    return false;
}

static bool amt_to_wg_format_audio_mpeg1(const AM_MEDIA_TYPE *mt, struct wg_format *format)
{
    const MPEG1WAVEFORMAT *audio_format = (const MPEG1WAVEFORMAT *)mt->pbFormat;

    if (!IsEqualGUID(&mt->formattype, &FORMAT_WaveFormatEx))
    {
        FIXME("Unknown format type %s.\n", debugstr_guid(&mt->formattype));
        return false;
    }
    if (mt->cbFormat < sizeof(*audio_format) || !mt->pbFormat)
    {
        ERR("Unexpected format size %lu.\n", mt->cbFormat);
        return false;
    }

    format->major_type = WG_MAJOR_TYPE_AUDIO_MPEG1;
    format->u.audio_mpeg1.channels = audio_format->wfx.nChannels;
    format->u.audio_mpeg1.rate = audio_format->wfx.nSamplesPerSec;
    format->u.audio_mpeg1.layer = audio_format->fwHeadLayer;
    return true;
}

static bool amt_to_wg_format_audio_mpeg1_layer3(const AM_MEDIA_TYPE *mt, struct wg_format *format)
{
    const MPEGLAYER3WAVEFORMAT *audio_format = (const MPEGLAYER3WAVEFORMAT *)mt->pbFormat;

    if (!IsEqualGUID(&mt->formattype, &FORMAT_WaveFormatEx))
    {
        FIXME("Unknown format type %s.\n", debugstr_guid(&mt->formattype));
        return false;
    }
    if (mt->cbFormat < sizeof(*audio_format) || !mt->pbFormat)
    {
        ERR("Unexpected format size %lu.\n", mt->cbFormat);
        return false;
    }

    format->major_type = WG_MAJOR_TYPE_AUDIO_MPEG1;
    format->u.audio_mpeg1.channels = audio_format->wfx.nChannels;
    format->u.audio_mpeg1.rate = audio_format->wfx.nSamplesPerSec;
    format->u.audio_mpeg1.layer = 3;
    return true;
}

static bool amt_to_wg_format_audio_wma(const AM_MEDIA_TYPE *mt, struct wg_format *format)
{
    const WAVEFORMATEX *audio_format = (const WAVEFORMATEX *)mt->pbFormat;

    if (!IsEqualGUID(&mt->formattype, &FORMAT_WaveFormatEx))
    {
        FIXME("Unknown format type %s.\n", debugstr_guid(&mt->formattype));
        return false;
    }
    if (mt->cbFormat < sizeof(*audio_format) || !mt->pbFormat)
    {
        ERR("Unexpected format size %lu.\n", mt->cbFormat);
        return false;
    }

    if (IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_MSAUDIO1))
        format->u.audio_wma.version = 1;
    else if (IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_WMAUDIO2))
        format->u.audio_wma.version = 2;
    else if (IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_WMAUDIO3))
        format->u.audio_wma.version = 3;
    else if (IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_WMAUDIO_LOSSLESS))
        format->u.audio_wma.version = 4;
    else
        assert(false);
    format->major_type = WG_MAJOR_TYPE_AUDIO_WMA;
    format->u.audio_wma.bitrate = audio_format->nAvgBytesPerSec * 8;
    format->u.audio_wma.rate = audio_format->nSamplesPerSec;
    format->u.audio_wma.depth = audio_format->wBitsPerSample;
    format->u.audio_wma.channels = audio_format->nChannels;
    format->u.audio_wma.block_align = audio_format->nBlockAlign;

    format->u.audio_wma.codec_data_len = 0;
    if (format->u.audio_wma.version == 1)
        format->u.audio_wma.codec_data_len = 4;
    if (format->u.audio_wma.version == 2)
        format->u.audio_wma.codec_data_len = 10;
    if (format->u.audio_wma.version == 3)
        format->u.audio_wma.codec_data_len = 18;
    if (format->u.audio_wma.version == 4)
        format->u.audio_wma.codec_data_len = 18;
    if (mt->cbFormat >= sizeof(WAVEFORMATEX) + format->u.audio_wma.codec_data_len)
        memcpy(format->u.audio_wma.codec_data, audio_format+1, format->u.audio_wma.codec_data_len);
    else
        FIXME("Too small format block, can't copy codec data\n");

    return true;
}

static bool amt_to_wg_format_video(const AM_MEDIA_TYPE *mt, struct wg_format *format)
{
    static const struct
    {
        const GUID *subtype;
        enum wg_video_format format;
    }
    format_map[] =
    {
        {&MEDIASUBTYPE_ARGB32,  WG_VIDEO_FORMAT_BGRA},
        {&MEDIASUBTYPE_RGB32,   WG_VIDEO_FORMAT_BGRx},
        {&MEDIASUBTYPE_RGB24,   WG_VIDEO_FORMAT_BGR},
        {&MEDIASUBTYPE_RGB555,  WG_VIDEO_FORMAT_RGB15},
        {&MEDIASUBTYPE_RGB565,  WG_VIDEO_FORMAT_RGB16},
        {&MEDIASUBTYPE_AYUV,    WG_VIDEO_FORMAT_AYUV},
        {&MEDIASUBTYPE_I420,    WG_VIDEO_FORMAT_I420},
        {&MEDIASUBTYPE_NV12,    WG_VIDEO_FORMAT_NV12},
        {&MEDIASUBTYPE_UYVY,    WG_VIDEO_FORMAT_UYVY},
        {&MEDIASUBTYPE_YUY2,    WG_VIDEO_FORMAT_YUY2},
        {&MEDIASUBTYPE_YV12,    WG_VIDEO_FORMAT_YV12},
        {&MEDIASUBTYPE_YVYU,    WG_VIDEO_FORMAT_YVYU},
    };

    const VIDEOINFOHEADER *video_format = (const VIDEOINFOHEADER *)mt->pbFormat;
    unsigned int i;

    if (!IsEqualGUID(&mt->formattype, &FORMAT_VideoInfo))
    {
        FIXME("Unknown format type %s.\n", debugstr_guid(&mt->formattype));
        return false;
    }
    if (mt->cbFormat < sizeof(VIDEOINFOHEADER) || !mt->pbFormat)
    {
        ERR("Unexpected format size %lu.\n", mt->cbFormat);
        return false;
    }

    format->major_type = WG_MAJOR_TYPE_VIDEO;
    format->u.video.width = video_format->bmiHeader.biWidth;
    format->u.video.height = video_format->bmiHeader.biHeight;
    format->u.video.fps_n = 10000000;
    format->u.video.fps_d = video_format->AvgTimePerFrame;

    for (i = 0; i < ARRAY_SIZE(format_map); ++i)
    {
        if (IsEqualGUID(&mt->subtype, format_map[i].subtype))
        {
            format->u.video.format = format_map[i].format;
            if (wg_video_format_is_rgb(format->u.video.format))
                format->u.video.height = -format->u.video.height;
            return true;
        }
    }

    FIXME("Unknown subtype %s.\n", debugstr_guid(&mt->subtype));
    return false;
}

static bool amt_to_wg_format_video_wmv(const AM_MEDIA_TYPE *mt, struct wg_format *format)
{
    const VIDEOINFOHEADER *video_format = (const VIDEOINFOHEADER *)mt->pbFormat;

    if (!IsEqualGUID(&mt->formattype, &FORMAT_VideoInfo))
    {
        FIXME("Unknown format type %s.\n", debugstr_guid(&mt->formattype));
        return false;
    }
    if (mt->cbFormat < sizeof(VIDEOINFOHEADER) || !mt->pbFormat)
    {
        ERR("Unexpected format size %lu.\n", mt->cbFormat);
        return false;
    }

    format->major_type = WG_MAJOR_TYPE_VIDEO_WMV;
    format->u.video_wmv.width = video_format->bmiHeader.biWidth;
    format->u.video_wmv.height = video_format->bmiHeader.biHeight;
    format->u.video_wmv.fps_n = 10000000;
    format->u.video_wmv.fps_d = video_format->AvgTimePerFrame;

    if (IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_WMV1))
        format->u.video_wmv.format = WG_WMV_VIDEO_FORMAT_WMV1;
    else if (IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_WMV2))
        format->u.video_wmv.format = WG_WMV_VIDEO_FORMAT_WMV2;
    else if (IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_WMV3))
        format->u.video_wmv.format = WG_WMV_VIDEO_FORMAT_WMV3;
    else if (IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_WMVA))
        format->u.video_wmv.format = WG_WMV_VIDEO_FORMAT_WMVA;
    else if (IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_WVC1))
        format->u.video_wmv.format = WG_WMV_VIDEO_FORMAT_WVC1;
    else
        format->u.video_wmv.format = WG_WMV_VIDEO_FORMAT_UNKNOWN;

    format->u.video_wmv.codec_data_len = mt->cbFormat - sizeof(VIDEOINFOHEADER);
    if (format->u.video_wmv.codec_data_len > sizeof(format->u.video_wmv.codec_data))
    {
        ERR("Too big codec_data value (%u).\n", format->u.video_wmv.codec_data_len);
        format->u.video_wmv.codec_data_len = 0;
    }
    memcpy(format->u.video_wmv.codec_data, video_format+1, format->u.video_wmv.codec_data_len);
    return true;
}

static bool amt_to_wg_format_video_mpeg1(const AM_MEDIA_TYPE *mt, struct wg_format *format)
{
    const MPEG1VIDEOINFO *video_format = (const MPEG1VIDEOINFO *)mt->pbFormat;

    if (!IsEqualGUID(&mt->formattype, &FORMAT_MPEGVideo))
    {
        FIXME("Unknown format type %s.\n", debugstr_guid(&mt->formattype));
        return false;
    }
    if (mt->cbFormat < sizeof(VIDEOINFOHEADER) || !mt->pbFormat)
    {
        ERR("Unexpected format size %lu.\n", mt->cbFormat);
        return false;
    }

    format->major_type = WG_MAJOR_TYPE_VIDEO_MPEG1;
    format->u.video_mpeg1.width = video_format->hdr.bmiHeader.biWidth;
    format->u.video_mpeg1.height = video_format->hdr.bmiHeader.biHeight;
    format->u.video_mpeg1.fps_n = 10000000;
    format->u.video_mpeg1.fps_d = video_format->hdr.AvgTimePerFrame;

    return true;
}

bool amt_to_wg_format(const AM_MEDIA_TYPE *mt, struct wg_format *format)
{
    memset(format, 0, sizeof(*format));

    if (IsEqualGUID(&mt->majortype, &MEDIATYPE_Video))
    {
        if (IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_WMV1)
                || IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_WMV2)
                || IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_WMVA)
                || IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_WMVP)
                || IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_WVP2)
                || IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_WMV_Unknown)
                || IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_WVC1)
                || IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_WMV3)
                || IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_VC1S))
            return amt_to_wg_format_video_wmv(mt, format);
        if (IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_MPEG1Payload))
            return amt_to_wg_format_video_mpeg1(mt, format);
        return amt_to_wg_format_video(mt, format);
    }
    if (IsEqualGUID(&mt->majortype, &MEDIATYPE_Audio))
    {
        if (IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_MPEG1AudioPayload))
            return amt_to_wg_format_audio_mpeg1(mt, format);
        if (IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_MP3))
            return amt_to_wg_format_audio_mpeg1_layer3(mt, format);
        if (IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_MSAUDIO1)
                || IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_WMAUDIO2)
                || IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_WMAUDIO3)
                || IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_WMAUDIO_LOSSLESS))
            return amt_to_wg_format_audio_wma(mt, format);
        return amt_to_wg_format_audio(mt, format);
    }

    FIXME("Unknown major type %s.\n", debugstr_guid(&mt->majortype));
    return false;
}

/*
 * scale_uint64() is based on gst_util_scale_int() from GStreamer, which is
 * covered by the following license:
 *
 * GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *                    2000 Wim Taymans <wtay@chello.be>
 *                    2002 Thomas Vander Stichele <thomas@apestaart.org>
 *                    2004 Wim Taymans <wim@fluendo.com>
 *                    2015 Jan Schmidt <jan@centricular.com>
 *
 * gstutils.c: Utility functions
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
static uint64_t scale_uint64(uint64_t value, uint32_t numerator, uint32_t denominator)
{
    ULARGE_INTEGER i, high, low;

    if (!value)
        return 0;

    i.QuadPart = value;
    low.QuadPart = (ULONGLONG)i.u.LowPart * numerator;
    high.QuadPart = (ULONGLONG)i.u.HighPart * numerator + low.u.HighPart;
    low.u.HighPart = 0;

    if (high.u.HighPart >= denominator)
        return ULLONG_MAX;

    low.QuadPart += (high.QuadPart % denominator) << 32;
    return ((high.QuadPart / denominator) << 32) + (low.QuadPart / denominator);
}

/* Fill and send a single IMediaSample. */
static HRESULT send_sample(struct parser_source *pin, IMediaSample *sample,
        const struct wg_parser_buffer *buffer, uint32_t offset, uint32_t size, DWORD bytes_per_second)
{
    HRESULT hr;
    BYTE *ptr = NULL;

    TRACE("offset %u, size %u, sample size %lu.\n", offset, size, IMediaSample_GetSize(sample));

    hr = IMediaSample_SetActualDataLength(sample, size);
    if(FAILED(hr)){
        ERR("Failed to set sample size, hr %#lx.\n", hr);
        return hr;
    }

    IMediaSample_GetPointer(sample, &ptr);

    if (!wg_parser_stream_copy_buffer(pin->wg_stream, ptr, offset, size))
    {
        /* The GStreamer pin has been flushed. */
        return S_OK;
    }

    if (buffer->has_pts)
    {
        REFERENCE_TIME start_pts = buffer->pts;

        if (offset)
            start_pts += scale_uint64(offset, 10000000, bytes_per_second);
        start_pts -= pin->seek.llCurrent;
        start_pts *= pin->seek.dRate;

        if (buffer->has_duration)
        {
            REFERENCE_TIME end_pts = buffer->pts + buffer->duration;

            if (offset + size < buffer->size)
                end_pts = buffer->pts + scale_uint64(offset + size, 10000000, bytes_per_second);
            end_pts -= pin->seek.llCurrent;
            end_pts *= pin->seek.dRate;

            IMediaSample_SetTime(sample, &start_pts, &end_pts);
            IMediaSample_SetMediaTime(sample, &start_pts, &end_pts);
        }
        else
        {
            IMediaSample_SetTime(sample, &start_pts, NULL);
            IMediaSample_SetMediaTime(sample, NULL, NULL);
        }
    }
    else
    {
        IMediaSample_SetTime(sample, NULL, NULL);
        IMediaSample_SetMediaTime(sample, NULL, NULL);
    }

    IMediaSample_SetDiscontinuity(sample, !offset && buffer->discontinuity);
    IMediaSample_SetPreroll(sample, buffer->preroll);
    IMediaSample_SetSyncPoint(sample, !buffer->delta);

    if (!pin->pin.pin.peer)
        return VFW_E_NOT_CONNECTED;

    hr = IMemInputPin_Receive(pin->pin.pMemInputPin, sample);
    TRACE("Receive() returned hr %#lx.\n", hr);
    return hr;
}

/* Send a single GStreamer buffer (splitting it into multiple IMediaSamples if
 * necessary). */
static void send_buffer(struct parser_source *pin, const struct wg_parser_buffer *buffer)
{
    HRESULT hr;
    IMediaSample *sample;

    if (pin->need_segment)
    {
        if (FAILED(hr = IPin_NewSegment(pin->pin.pin.peer,
                pin->seek.llCurrent, pin->seek.llStop, pin->seek.dRate)))
            WARN("Failed to deliver new segment, hr %#lx.\n", hr);
        pin->need_segment = false;
    }

    if (IsEqualGUID(&pin->pin.pin.mt.formattype, &FORMAT_WaveFormatEx)
            && (IsEqualGUID(&pin->pin.pin.mt.subtype, &MEDIASUBTYPE_PCM)
            || IsEqualGUID(&pin->pin.pin.mt.subtype, &MEDIASUBTYPE_IEEE_FLOAT)))
    {
        WAVEFORMATEX *format = (WAVEFORMATEX *)pin->pin.pin.mt.pbFormat;
        uint32_t offset = 0;

        while (offset < buffer->size)
        {
            uint32_t advance;

            if (FAILED(hr = IMemAllocator_GetBuffer(pin->pin.pAllocator, &sample, NULL, NULL, 0)))
            {
                ERR("Failed to get a sample, hr %#lx.\n", hr);
                break;
            }

            advance = min(IMediaSample_GetSize(sample), buffer->size - offset);

            hr = send_sample(pin, sample, buffer, offset, advance, format->nAvgBytesPerSec);

            IMediaSample_Release(sample);

            if (FAILED(hr))
                break;

            offset += advance;
        }
    }
    else
    {
        if (FAILED(hr = IMemAllocator_GetBuffer(pin->pin.pAllocator, &sample, NULL, NULL, 0)))
        {
            ERR("Failed to get a sample, hr %#lx.\n", hr);
        }
        else
        {
            hr = send_sample(pin, sample, buffer, 0, buffer->size, 0);

            IMediaSample_Release(sample);
        }
    }

    wg_parser_stream_release_buffer(pin->wg_stream);
}

static DWORD CALLBACK stream_thread(void *arg)
{
    struct parser_source *pin = arg;
    struct parser *filter = impl_from_strmbase_filter(pin->pin.pin.filter);

    TRACE("Starting streaming thread for pin %p.\n", pin);

    for (;;)
    {
        struct wg_parser_buffer buffer;

        EnterCriticalSection(&filter->streaming_cs);

        while (filter->flushing)
            SleepConditionVariableCS(&filter->flushing_cv, &filter->streaming_cs, INFINITE);

        if (!filter->streaming)
        {
            LeaveCriticalSection(&filter->streaming_cs);
            break;
        }

        LeaveCriticalSection(&filter->streaming_cs);

        EnterCriticalSection(&pin->flushing_cs);

        if (pin->eos)
        {
            SleepConditionVariableCS(&pin->eos_cv, &pin->flushing_cs, INFINITE);
            LeaveCriticalSection(&pin->flushing_cs);
            continue;
        }

        if (wg_parser_stream_get_buffer(filter->wg_parser, pin->wg_stream, &buffer))
        {
            send_buffer(pin, &buffer);
        }
        else
        {
            TRACE("Got EOS.\n");
            IPin_EndOfStream(pin->pin.pin.peer);
            pin->eos = true;
        }

        LeaveCriticalSection(&pin->flushing_cs);
    }

    TRACE("Streaming stopped; exiting.\n");
    return 0;
}

static DWORD CALLBACK read_thread(void *arg)
{
    struct parser *filter = arg;
    LONGLONG file_size, unused;
    size_t buffer_size = 4096;
    void *data = NULL;

    if (!(data = malloc(buffer_size)))
        return 0;

    IAsyncReader_Length(filter->reader, &file_size, &unused);

    TRACE("Starting read thread for filter %p.\n", filter);

    while (filter->sink_connected)
    {
        uint64_t offset;
        uint32_t size;
        HRESULT hr;

        if (!wg_parser_get_next_read_offset(filter->wg_parser, &offset, &size))
            continue;

        if (offset >= file_size)
            size = 0;
        else if (offset + size >= file_size)
            size = file_size - offset;

        if (!array_reserve(&data, &buffer_size, size, 1))
        {
            free(data);
            return 0;
        }

        hr = IAsyncReader_SyncRead(filter->reader, offset, size, data);
        if (FAILED(hr))
            ERR("Failed to read %u bytes at offset %I64u, hr %#lx.\n", size, offset, hr);

        wg_parser_push_data(filter->wg_parser, SUCCEEDED(hr) ? data : NULL, size);
    }

    free(data);
    TRACE("Streaming stopped; exiting.\n");
    return 0;
}

static inline struct parser_source *impl_from_IMediaSeeking(IMediaSeeking *iface)
{
    return CONTAINING_RECORD(iface, struct parser_source, seek.IMediaSeeking_iface);
}

static struct strmbase_pin *parser_get_pin(struct strmbase_filter *base, unsigned int index)
{
    struct parser *filter = impl_from_strmbase_filter(base);

    if (filter->enum_sink_first)
    {
        if (!index)
            return &filter->sink.pin;
        else if (index <= filter->source_count)
            return &filter->sources[index - 1]->pin.pin;
    }
    else
    {
        if (index < filter->source_count)
            return &filter->sources[index]->pin.pin;
        else if (index == filter->source_count)
            return &filter->sink.pin;
    }
    return NULL;
}

static void parser_destroy(struct strmbase_filter *iface)
{
    struct parser *filter = impl_from_strmbase_filter(iface);
    HRESULT hr;

    /* Don't need to clean up output pins, disconnecting input pin will do that */
    if (filter->sink.pin.peer)
    {
        hr = IPin_Disconnect(filter->sink.pin.peer);
        assert(hr == S_OK);
        hr = IPin_Disconnect(&filter->sink.pin.IPin_iface);
        assert(hr == S_OK);
    }

    if (filter->reader)
        IAsyncReader_Release(filter->reader);
    filter->reader = NULL;

    wg_parser_destroy(filter->wg_parser);

    filter->streaming_cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&filter->streaming_cs);

    strmbase_sink_cleanup(&filter->sink);
    strmbase_filter_cleanup(&filter->filter);
    free(filter);
}

static HRESULT parser_init_stream(struct strmbase_filter *iface)
{
    struct parser *filter = impl_from_strmbase_filter(iface);
    DWORD stop_flags = AM_SEEKING_NoPositioning;
    const SourceSeeking *seeking;
    unsigned int i;

    if (!filter->sink_connected)
        return S_OK;

    filter->streaming = true;

    for (i = 0; i < filter->source_count; ++i)
    {
        struct parser_source *source = filter->sources[i];
        struct wg_format format;
        bool ret;

        if (source->pin.pin.peer)
        {
            ret = amt_to_wg_format(&source->pin.pin.mt, &format);
            assert(ret);
            wg_parser_stream_enable(source->wg_stream, &format);
        }
        else
        {
            wg_parser_stream_disable(source->wg_stream);
        }
    }

    /* DirectShow retains the old seek positions, but resets to them every time
     * it transitions from stopped -> paused. */

    seeking = &filter->sources[0]->seek;
    if (seeking->llStop)
        stop_flags = AM_SEEKING_AbsolutePositioning;
    wg_parser_stream_seek(filter->sources[0]->wg_stream, seeking->dRate,
            seeking->llCurrent, seeking->llStop, AM_SEEKING_AbsolutePositioning, stop_flags);

    for (i = 0; i < filter->source_count; ++i)
    {
        struct parser_source *pin = filter->sources[i];
        HRESULT hr;

        if (!pin->pin.pin.peer)
            continue;

        if (FAILED(hr = IMemAllocator_Commit(pin->pin.pAllocator)))
            ERR("Failed to commit allocator, hr %#lx.\n", hr);

        pin->need_segment = true;
        pin->eos = false;

        pin->thread = CreateThread(NULL, 0, stream_thread, pin, 0, NULL);
    }

    return S_OK;
}

static HRESULT parser_cleanup_stream(struct strmbase_filter *iface)
{
    struct parser *filter = impl_from_strmbase_filter(iface);
    unsigned int i;

    if (!filter->sink_connected)
        return S_OK;

    EnterCriticalSection(&filter->streaming_cs);
    filter->streaming = false;
    LeaveCriticalSection(&filter->streaming_cs);

    for (i = 0; i < filter->source_count; ++i)
    {
        struct parser_source *pin = filter->sources[i];

        if (!pin->pin.pin.peer)
            continue;

        IMemAllocator_Decommit(pin->pin.pAllocator);

        WakeConditionVariable(&pin->eos_cv);
        WaitForSingleObject(pin->thread, INFINITE);
        CloseHandle(pin->thread);
        pin->thread = NULL;
    }

    return S_OK;
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = parser_get_pin,
    .filter_destroy = parser_destroy,
    .filter_init_stream = parser_init_stream,
    .filter_cleanup_stream = parser_cleanup_stream,
};

static inline struct parser *impl_from_strmbase_sink(struct strmbase_sink *iface)
{
    return CONTAINING_RECORD(iface, struct parser, sink);
}

static HRESULT sink_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    if (IsEqualGUID(&mt->majortype, &MEDIATYPE_Stream))
        return S_OK;
    return S_FALSE;
}

static HRESULT parser_sink_connect(struct strmbase_sink *iface, IPin *peer, const AM_MEDIA_TYPE *pmt)
{
    struct parser *filter = impl_from_strmbase_sink(iface);
    LONGLONG file_size, unused;
    HRESULT hr = S_OK;
    unsigned int i;

    filter->reader = NULL;
    if (FAILED(hr = IPin_QueryInterface(peer, &IID_IAsyncReader, (void **)&filter->reader)))
        return hr;

    IAsyncReader_Length(filter->reader, &file_size, &unused);

    filter->sink_connected = true;
    filter->read_thread = CreateThread(NULL, 0, read_thread, filter, 0, NULL);

    if (FAILED(hr = wg_parser_connect(filter->wg_parser, file_size)))
        goto err;

    if (!filter->init_gst(filter))
    {
        hr = E_FAIL;
        goto err;
    }

    for (i = 0; i < filter->source_count; ++i)
    {
        struct parser_source *pin = filter->sources[i];

        pin->seek.llDuration = pin->seek.llStop = wg_parser_stream_get_duration(pin->wg_stream);
        pin->seek.llCurrent = 0;
    }

    return S_OK;
err:
    GST_RemoveOutputPins(filter);
    IAsyncReader_Release(filter->reader);
    filter->reader = NULL;
    return hr;
}

static void parser_sink_disconnect(struct strmbase_sink *iface)
{
    struct parser *filter = impl_from_strmbase_sink(iface);

    GST_RemoveOutputPins(filter);

    IAsyncReader_Release(filter->reader);
    filter->reader = NULL;
}

static const struct strmbase_sink_ops sink_ops =
{
    .base.pin_query_accept = sink_query_accept,
    .sink_connect = parser_sink_connect,
    .sink_disconnect = parser_sink_disconnect,
};

static BOOL decodebin_parser_filter_init_gst(struct parser *filter)
{
    wg_parser_t parser = filter->wg_parser;
    unsigned int i, stream_count;
    WCHAR source_name[20];

    stream_count = wg_parser_get_stream_count(parser);
    for (i = 0; i < stream_count; ++i)
    {
        swprintf(source_name, ARRAY_SIZE(source_name), L"Stream %02u", i);
        if (!create_pin(filter, wg_parser_get_stream(parser, i), source_name))
            return FALSE;
    }

    return TRUE;
}

static HRESULT decodebin_parser_source_query_accept(struct parser_source *pin, const AM_MEDIA_TYPE *mt)
{
    struct wg_format format;

    /* At least make sure we can convert it to wg_format. */
    return amt_to_wg_format(mt, &format) ? S_OK : S_FALSE;
}

static HRESULT decodebin_parser_source_get_media_type(struct parser_source *pin,
        unsigned int index, AM_MEDIA_TYPE *mt)
{
    struct wg_format format;

    static const enum wg_video_format video_formats[] =
    {
        /* Try to prefer YUV formats over RGB ones. Most decoders output in the
         * YUV color space, and it's generally much less expensive for
         * videoconvert to do YUV -> YUV transformations. */
        WG_VIDEO_FORMAT_AYUV,
        WG_VIDEO_FORMAT_I420,
        WG_VIDEO_FORMAT_YV12,
        WG_VIDEO_FORMAT_YUY2,
        WG_VIDEO_FORMAT_UYVY,
        WG_VIDEO_FORMAT_YVYU,
        WG_VIDEO_FORMAT_NV12,
        WG_VIDEO_FORMAT_BGRA,
        WG_VIDEO_FORMAT_BGRx,
        WG_VIDEO_FORMAT_BGR,
        WG_VIDEO_FORMAT_RGB16,
        WG_VIDEO_FORMAT_RGB15,
    };

    wg_parser_stream_get_preferred_format(pin->wg_stream, &format);

    memset(mt, 0, sizeof(AM_MEDIA_TYPE));

    if (amt_from_wg_format(mt, &format, false))
    {
        if (!index--)
            return S_OK;
        FreeMediaType(mt);
    }

    if (format.major_type == WG_MAJOR_TYPE_VIDEO && index < ARRAY_SIZE(video_formats))
    {
        format.u.video.format = video_formats[index];
        /* Downstream filters probably expect RGB video to be bottom-up. */
        if (format.u.video.height > 0 && wg_video_format_is_rgb(video_formats[index]))
            format.u.video.height = -format.u.video.height;
        if (!amt_from_wg_format(mt, &format, false))
            return E_OUTOFMEMORY;
        return S_OK;
    }
    else if (format.major_type == WG_MAJOR_TYPE_AUDIO && !index)
    {
        format.u.audio.format = WG_AUDIO_FORMAT_S16LE;
        if (!amt_from_wg_format(mt, &format, false))
            return E_OUTOFMEMORY;
        return S_OK;
    }

    return VFW_S_NO_MORE_ITEMS;
}

static HRESULT parser_create(enum wg_parser_type type, BOOL output_compressed, struct parser **parser)
{
    struct parser *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (!(object->wg_parser = wg_parser_create(type, output_compressed)))
    {
        free(object);
        return E_OUTOFMEMORY;
    }

    InitializeCriticalSection(&object->streaming_cs);
    object->streaming_cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": parser.streaming_cs");

    InitializeConditionVariable(&object->flushing_cv);

    *parser = object;
    return S_OK;
}

HRESULT decodebin_parser_create(IUnknown *outer, IUnknown **out)
{
    struct parser *object;
    HRESULT hr;

    if (FAILED(hr = parser_create(WG_PARSER_DECODEBIN, FALSE, &object)))
        return hr;

    strmbase_filter_init(&object->filter, outer, &CLSID_decodebin_parser, &filter_ops);
    strmbase_sink_init(&object->sink, &object->filter, L"input pin", &sink_ops, NULL);

    object->init_gst = decodebin_parser_filter_init_gst;
    object->source_query_accept = decodebin_parser_source_query_accept;
    object->source_get_media_type = decodebin_parser_source_get_media_type;

    TRACE("Created GStreamer demuxer %p.\n", object);
    *out = &object->filter.IUnknown_inner;
    return S_OK;
}

static struct parser *impl_from_IAMStreamSelect(IAMStreamSelect *iface)
{
    return CONTAINING_RECORD(iface, struct parser, IAMStreamSelect_iface);
}

static HRESULT WINAPI stream_select_QueryInterface(IAMStreamSelect *iface, REFIID iid, void **out)
{
    struct parser *filter = impl_from_IAMStreamSelect(iface);
    return IUnknown_QueryInterface(filter->filter.outer_unk, iid, out);
}

static ULONG WINAPI stream_select_AddRef(IAMStreamSelect *iface)
{
    struct parser *filter = impl_from_IAMStreamSelect(iface);
    return IUnknown_AddRef(filter->filter.outer_unk);
}

static ULONG WINAPI stream_select_Release(IAMStreamSelect *iface)
{
    struct parser *filter = impl_from_IAMStreamSelect(iface);
    return IUnknown_Release(filter->filter.outer_unk);
}

static HRESULT WINAPI stream_select_Count(IAMStreamSelect *iface, DWORD *count)
{
    struct parser *filter = impl_from_IAMStreamSelect(iface);
    TRACE("filter %p, count %p\n", filter, count);
    EnterCriticalSection(&filter->filter.filter_cs);
    if (filter->sink.pin.peer)
        *count = wg_parser_get_stream_count(filter->wg_parser);
    else
        *count = 0;
    LeaveCriticalSection(&filter->filter.filter_cs);
    return S_OK;
}

static HRESULT WINAPI stream_select_Info(IAMStreamSelect *iface, LONG index,
        AM_MEDIA_TYPE **mt, DWORD *flags, LCID *lcid, DWORD *group, WCHAR **name,
        IUnknown **object, IUnknown **unknown)
{
    struct parser *filter = impl_from_IAMStreamSelect(iface);
    HRESULT hr = S_OK;

    FIXME("filter %p, index %ld, mt %p, flags %p, lcid %p, group %p, name %p, object %p, unknown %p, semi-stub!\n",
            filter, index, mt, flags, lcid, group, name, object, unknown);
    EnterCriticalSection(&filter->filter.filter_cs);

    if (!filter->sink.pin.peer)
    {
        LeaveCriticalSection(&filter->filter.filter_cs);
        return VFW_E_NOT_CONNECTED;
    }
    if (index < 0 || index >= filter->source_count)
    {
        LeaveCriticalSection(&filter->filter.filter_cs);
        return S_FALSE;
    }

    if (mt)
        *mt = CreateMediaType(&filter->sources[index]->pin.pin.mt);
    if (flags) /* todo */
        *flags = 0;
    if (lcid) /* todo */
        *lcid = 0;
    if (group) /* todo */
        *group = 0;
    if (name) /* todo */
        *name = NULL;
    if (object) /* todo */
        *object = NULL;
    if (unknown)
        *unknown = NULL;

    LeaveCriticalSection(&filter->filter.filter_cs);
    return hr;
}

static HRESULT WINAPI stream_select_Enable(IAMStreamSelect *iface, LONG index, DWORD flags)
{
    FIXME("iface %p, index %ld, flags %#lx, stub!\n", iface, index, flags);
    return E_NOTIMPL;
}

static const IAMStreamSelectVtbl stream_select_vtbl =
{
    stream_select_QueryInterface,
    stream_select_AddRef,
    stream_select_Release,
    stream_select_Count,
    stream_select_Info,
    stream_select_Enable,
};

static HRESULT WINAPI GST_ChangeCurrent(IMediaSeeking *iface)
{
    struct parser_source *This = impl_from_IMediaSeeking(iface);
    TRACE("(%p)\n", This);
    return S_OK;
}

static HRESULT WINAPI GST_ChangeStop(IMediaSeeking *iface)
{
    struct parser_source *This = impl_from_IMediaSeeking(iface);
    TRACE("(%p)\n", This);
    return S_OK;
}

static HRESULT WINAPI GST_ChangeRate(IMediaSeeking *iface)
{
    struct parser_source *pin = impl_from_IMediaSeeking(iface);

    wg_parser_stream_seek(pin->wg_stream, pin->seek.dRate, 0, 0,
            AM_SEEKING_NoPositioning, AM_SEEKING_NoPositioning);
    return S_OK;
}

static HRESULT WINAPI GST_Seeking_QueryInterface(IMediaSeeking *iface, REFIID riid, void **ppv)
{
    struct parser_source *This = impl_from_IMediaSeeking(iface);
    return IPin_QueryInterface(&This->pin.pin.IPin_iface, riid, ppv);
}

static ULONG WINAPI GST_Seeking_AddRef(IMediaSeeking *iface)
{
    struct parser_source *This = impl_from_IMediaSeeking(iface);
    return IPin_AddRef(&This->pin.pin.IPin_iface);
}

static ULONG WINAPI GST_Seeking_Release(IMediaSeeking *iface)
{
    struct parser_source *This = impl_from_IMediaSeeking(iface);
    return IPin_Release(&This->pin.pin.IPin_iface);
}

static HRESULT WINAPI GST_Seeking_SetPositions(IMediaSeeking *iface,
        LONGLONG *current, DWORD current_flags, LONGLONG *stop, DWORD stop_flags)
{
    struct parser_source *pin = impl_from_IMediaSeeking(iface);
    struct parser *filter = impl_from_strmbase_filter(pin->pin.pin.filter);
    int i;

    TRACE("pin %p, current %s, current_flags %#lx, stop %s, stop_flags %#lx.\n",
            pin, current ? debugstr_time(*current) : "<null>", current_flags,
            stop ? debugstr_time(*stop) : "<null>", stop_flags);

    if (pin->pin.pin.filter->state == State_Stopped)
    {
        SourceSeekingImpl_SetPositions(iface, current, current_flags, stop, stop_flags);
        return S_OK;
    }

    if (!(current_flags & AM_SEEKING_NoFlush))
    {
        for (i = 0; i < filter->source_count; ++i)
        {
            if (filter->sources[i]->pin.pin.peer)
                IPin_BeginFlush(filter->sources[i]->pin.pin.peer);
        }

        if (filter->reader)
            IAsyncReader_BeginFlush(filter->reader);
    }

    /* Signal the streaming threads to "pause". */
    EnterCriticalSection(&filter->streaming_cs);
    filter->flushing = true;
    LeaveCriticalSection(&filter->streaming_cs);

    /* Acquire the flushing locks, to make sure the streaming threads really
     * are paused. This ensures the seek is serialized between flushes. */
    for (i = 0; i < filter->source_count; ++i)
    {
        struct parser_source *flush_pin = filter->sources[i];

        if (flush_pin->pin.pin.peer)
            EnterCriticalSection(&flush_pin->flushing_cs);
    }

    SourceSeekingImpl_SetPositions(iface, current, current_flags, stop, stop_flags);

    wg_parser_stream_seek(pin->wg_stream, pin->seek.dRate,
            pin->seek.llCurrent, pin->seek.llStop, current_flags, stop_flags);

    if (!(current_flags & AM_SEEKING_NoFlush))
    {
        for (i = 0; i < filter->source_count; ++i)
        {
            struct parser_source *flush_pin = filter->sources[i];

            if (flush_pin->pin.pin.peer)
                IPin_EndFlush(flush_pin->pin.pin.peer);
        }

        if (filter->reader)
            IAsyncReader_EndFlush(filter->reader);
    }

    /* Release the flushing locks. */
    for (i = filter->source_count - 1; i >= 0; --i)
    {
        struct parser_source *flush_pin = filter->sources[i];

        flush_pin->need_segment = true;
        flush_pin->eos = false;

        if (flush_pin->pin.pin.peer)
        {
            LeaveCriticalSection(&flush_pin->flushing_cs);
            WakeConditionVariable(&flush_pin->eos_cv);
        }
    }

    /* Signal the streaming threads to resume. */
    EnterCriticalSection(&filter->streaming_cs);
    filter->flushing = false;
    LeaveCriticalSection(&filter->streaming_cs);
    WakeAllConditionVariable(&filter->flushing_cv);

    return S_OK;
}

static const IMediaSeekingVtbl GST_Seeking_Vtbl =
{
    GST_Seeking_QueryInterface,
    GST_Seeking_AddRef,
    GST_Seeking_Release,
    SourceSeekingImpl_GetCapabilities,
    SourceSeekingImpl_CheckCapabilities,
    SourceSeekingImpl_IsFormatSupported,
    SourceSeekingImpl_QueryPreferredFormat,
    SourceSeekingImpl_GetTimeFormat,
    SourceSeekingImpl_IsUsingTimeFormat,
    SourceSeekingImpl_SetTimeFormat,
    SourceSeekingImpl_GetDuration,
    SourceSeekingImpl_GetStopPosition,
    SourceSeekingImpl_GetCurrentPosition,
    SourceSeekingImpl_ConvertTimeFormat,
    GST_Seeking_SetPositions,
    SourceSeekingImpl_GetPositions,
    SourceSeekingImpl_GetAvailable,
    SourceSeekingImpl_SetRate,
    SourceSeekingImpl_GetRate,
    SourceSeekingImpl_GetPreroll
};

static inline struct parser_source *impl_from_IQualityControl( IQualityControl *iface )
{
    return CONTAINING_RECORD(iface, struct parser_source, IQualityControl_iface);
}

static HRESULT WINAPI GST_QualityControl_QueryInterface(IQualityControl *iface, REFIID riid, void **ppv)
{
    struct parser_source *pin = impl_from_IQualityControl(iface);
    return IPin_QueryInterface(&pin->pin.pin.IPin_iface, riid, ppv);
}

static ULONG WINAPI GST_QualityControl_AddRef(IQualityControl *iface)
{
    struct parser_source *pin = impl_from_IQualityControl(iface);
    return IPin_AddRef(&pin->pin.pin.IPin_iface);
}

static ULONG WINAPI GST_QualityControl_Release(IQualityControl *iface)
{
    struct parser_source *pin = impl_from_IQualityControl(iface);
    return IPin_Release(&pin->pin.pin.IPin_iface);
}

static HRESULT WINAPI GST_QualityControl_Notify(IQualityControl *iface, IBaseFilter *sender, Quality q)
{
    struct parser_source *pin = impl_from_IQualityControl(iface);
    uint64_t timestamp;
    int64_t diff;

    TRACE("pin %p, sender %p, type %s, proportion %ld, late %s, timestamp %s.\n",
            pin, sender, q.Type == Famine ? "Famine" : "Flood", q.Proportion,
            debugstr_time(q.Late), debugstr_time(q.TimeStamp));

    /* DirectShow filters sometimes pass negative timestamps (Audiosurf uses the
     * current time instead of the time of the last buffer). GstClockTime is
     * unsigned, so clamp it to 0. */
    timestamp = max(q.TimeStamp, 0);

    /* The documentation specifies that timestamp + diff must be nonnegative. */
    diff = q.Late;
    if (diff < 0 && timestamp < (uint64_t)-diff)
        diff = -timestamp;

    /* DirectShow "Proportion" describes what percentage of buffers the upstream
     * filter should keep (i.e. dropping the rest). If frames are late, the
     * proportion will be less than 1. For example, a proportion of 500 means
     * that the element should drop half of its frames, essentially because
     * frames are taking twice as long as they should to arrive.
     *
     * GStreamer "proportion" is the inverse of this; it describes how much
     * faster the upstream element should produce frames. I.e. if frames are
     * taking twice as long as they should to arrive, we want the frames to be
     * decoded twice as fast, and so we pass 2.0 to GStreamer. */

    if (!q.Proportion)
    {
        WARN("Ignoring quality message with zero proportion.\n");
        return S_OK;
    }

    /* GST_QOS_TYPE_OVERFLOW is also used for buffers that arrive on time, but
     * DirectShow filters might use Famine, so check that there actually is an
     * underrun. */
    wg_parser_stream_notify_qos(pin->wg_stream, q.Type == Famine && q.Proportion < 1000,
            1000.0 / q.Proportion, diff, timestamp);

    return S_OK;
}

static HRESULT WINAPI GST_QualityControl_SetSink(IQualityControl *iface, IQualityControl *tonotify)
{
    struct parser_source *pin = impl_from_IQualityControl(iface);
    TRACE("(%p)->(%p)\n", pin, pin);
    /* Do nothing */
    return S_OK;
}

static const IQualityControlVtbl GSTOutPin_QualityControl_Vtbl = {
    GST_QualityControl_QueryInterface,
    GST_QualityControl_AddRef,
    GST_QualityControl_Release,
    GST_QualityControl_Notify,
    GST_QualityControl_SetSink
};

static inline struct parser_source *impl_source_from_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, struct parser_source, pin.pin.IPin_iface);
}

static HRESULT source_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct parser_source *pin = impl_source_from_IPin(&iface->IPin_iface);

    if (IsEqualGUID(iid, &IID_IMediaSeeking))
        *out = &pin->seek.IMediaSeeking_iface;
    else if (IsEqualGUID(iid, &IID_IQualityControl))
        *out = &pin->IQualityControl_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT source_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    struct parser_source *pin = impl_source_from_IPin(&iface->IPin_iface);
    struct parser *filter = impl_from_strmbase_filter(iface->filter);
    return filter->source_query_accept(pin, mt);
}

static HRESULT source_get_media_type(struct strmbase_pin *iface, unsigned int index, AM_MEDIA_TYPE *mt)
{
    struct parser_source *pin = impl_source_from_IPin(&iface->IPin_iface);
    struct parser *filter = impl_from_strmbase_filter(iface->filter);
    return filter->source_get_media_type(pin, index, mt);
}

static HRESULT WINAPI GSTOutPin_DecideBufferSize(struct strmbase_source *iface,
        IMemAllocator *allocator, ALLOCATOR_PROPERTIES *props)
{
    struct parser_source *pin = impl_source_from_IPin(&iface->pin.IPin_iface);
    unsigned int buffer_count = 1;
    unsigned int buffer_size = 16384;
    ALLOCATOR_PROPERTIES ret_props;

    if (IsEqualGUID(&pin->pin.pin.mt.formattype, &FORMAT_VideoInfo))
    {
        VIDEOINFOHEADER *format = (VIDEOINFOHEADER *)pin->pin.pin.mt.pbFormat;
        buffer_size = format->bmiHeader.biSizeImage;
    }
    else if (IsEqualGUID(&pin->pin.pin.mt.formattype, &FORMAT_MPEGVideo))
    {
        MPEG1VIDEOINFO *format = (MPEG1VIDEOINFO *)pin->pin.pin.mt.pbFormat;
        buffer_size = format->hdr.bmiHeader.biSizeImage;
        buffer_count = 8;
    }
    else if (IsEqualGUID(&pin->pin.pin.mt.formattype, &FORMAT_WaveFormatEx)
            && (IsEqualGUID(&pin->pin.pin.mt.subtype, &MEDIASUBTYPE_PCM)
            || IsEqualGUID(&pin->pin.pin.mt.subtype, &MEDIASUBTYPE_IEEE_FLOAT)))
    {
        WAVEFORMATEX *format = (WAVEFORMATEX *)pin->pin.pin.mt.pbFormat;
        buffer_size = format->nAvgBytesPerSec;
    }
    else if (IsEqualGUID(&pin->pin.pin.mt.subtype, &MEDIASUBTYPE_MPEG1AudioPayload)
            || IsEqualGUID(&pin->pin.pin.mt.subtype, &MEDIASUBTYPE_MP3))
    {
        /* mpg123audiodec requires at least 3 buffers as it will keep
         * references to the last 2 samples. */
        buffer_count = 3;
    }

    /* We do need to drop any buffers that might have been sent with the old
     * caps, but this will be handled in parser_init_stream(). */

    props->cBuffers = max(props->cBuffers, buffer_count);
    props->cbBuffer = max(props->cbBuffer, buffer_size);
    props->cbAlign = max(props->cbAlign, 1);
    return IMemAllocator_SetProperties(allocator, props, &ret_props);
}

static void free_source_pin(struct parser_source *pin)
{
    if (pin->pin.pin.peer)
    {
        if (SUCCEEDED(IMemAllocator_Decommit(pin->pin.pAllocator)))
            IPin_Disconnect(pin->pin.pin.peer);
        IPin_Disconnect(&pin->pin.pin.IPin_iface);
    }

    pin->flushing_cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&pin->flushing_cs);

    strmbase_seeking_cleanup(&pin->seek);
    strmbase_source_cleanup(&pin->pin);
    free(pin);
}

static const struct strmbase_source_ops source_ops =
{
    .base.pin_query_interface = source_query_interface,
    .base.pin_query_accept = source_query_accept,
    .base.pin_get_media_type = source_get_media_type,
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideAllocator = BaseOutputPinImpl_DecideAllocator,
    .pfnDecideBufferSize = GSTOutPin_DecideBufferSize,
};

static struct parser_source *create_pin(struct parser *filter,
        wg_parser_stream_t stream, const WCHAR *name)
{
    struct parser_source *pin, **new_array;

    if (!(new_array = realloc(filter->sources, (filter->source_count + 1) * sizeof(*filter->sources))))
        return NULL;
    filter->sources = new_array;

    if (!(pin = calloc(1, sizeof(*pin))))
        return NULL;

    pin->wg_stream = stream;
    strmbase_source_init(&pin->pin, &filter->filter, name, &source_ops);
    pin->IQualityControl_iface.lpVtbl = &GSTOutPin_QualityControl_Vtbl;
    strmbase_seeking_init(&pin->seek, &GST_Seeking_Vtbl, GST_ChangeStop,
            GST_ChangeCurrent, GST_ChangeRate);
    BaseFilterImpl_IncrementPinVersion(&filter->filter);

    InitializeCriticalSection(&pin->flushing_cs);
    pin->flushing_cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": pin.flushing_cs");
    InitializeConditionVariable(&pin->eos_cv);

    filter->sources[filter->source_count++] = pin;
    return pin;
}

static HRESULT GST_RemoveOutputPins(struct parser *This)
{
    unsigned int i;

    TRACE("(%p)\n", This);

    if (!This->sink_connected)
        return S_OK;

    for (i = 0; i < This->source_count; ++i)
    {
        if (This->sources[i])
            free_source_pin(This->sources[i]);
    }

    wg_parser_disconnect(This->wg_parser);

    /* read_thread() needs to stay alive to service any read requests GStreamer
     * sends, so we can only shut it down after GStreamer stops. */
    This->sink_connected = false;
    WaitForSingleObject(This->read_thread, INFINITE);
    CloseHandle(This->read_thread);

    This->source_count = 0;
    free(This->sources);
    This->sources = NULL;

    BaseFilterImpl_IncrementPinVersion(&This->filter);
    return S_OK;
}

static BOOL compare_media_types(const AM_MEDIA_TYPE *a, const AM_MEDIA_TYPE *b)
{
    return IsEqualGUID(&a->majortype, &b->majortype)
            && IsEqualGUID(&a->subtype, &b->subtype)
            && IsEqualGUID(&a->formattype, &b->formattype)
            && a->cbFormat == b->cbFormat
            && !memcmp(a->pbFormat, b->pbFormat, a->cbFormat);
}

static HRESULT wave_parser_sink_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    if (!IsEqualGUID(&mt->majortype, &MEDIATYPE_Stream))
        return S_FALSE;
    if (IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_WAVE))
        return S_OK;
    if (IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_AU) || IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_AIFF))
        FIXME("AU and AIFF files are not yet supported.\n");
    return S_FALSE;
}

static const struct strmbase_sink_ops wave_parser_sink_ops =
{
    .base.pin_query_accept = wave_parser_sink_query_accept,
    .sink_connect = parser_sink_connect,
    .sink_disconnect = parser_sink_disconnect,
};

static BOOL wave_parser_filter_init_gst(struct parser *filter)
{
    if (!create_pin(filter, wg_parser_get_stream(filter->wg_parser, 0), L"output"))
        return FALSE;

    return TRUE;
}

static HRESULT wave_parser_source_query_accept(struct parser_source *pin, const AM_MEDIA_TYPE *mt)
{
    struct wg_format format;
    AM_MEDIA_TYPE pad_mt;
    HRESULT hr;

    wg_parser_stream_get_preferred_format(pin->wg_stream, &format);
    if (!amt_from_wg_format(&pad_mt, &format, false))
        return E_OUTOFMEMORY;
    hr = compare_media_types(mt, &pad_mt) ? S_OK : S_FALSE;
    FreeMediaType(&pad_mt);
    return hr;
}

static HRESULT wave_parser_source_get_media_type(struct parser_source *pin,
        unsigned int index, AM_MEDIA_TYPE *mt)
{
    struct wg_format format;

    if (index > 0)
        return VFW_S_NO_MORE_ITEMS;
    wg_parser_stream_get_preferred_format(pin->wg_stream, &format);
    if (!amt_from_wg_format(mt, &format, false))
        return E_OUTOFMEMORY;
    return S_OK;
}

HRESULT wave_parser_create(IUnknown *outer, IUnknown **out)
{
    struct parser *object;
    HRESULT hr;

    if (FAILED(hr = parser_create(WG_PARSER_WAVPARSE, FALSE, &object)))
        return hr;

    strmbase_filter_init(&object->filter, outer, &CLSID_WAVEParser, &filter_ops);
    strmbase_sink_init(&object->sink, &object->filter, L"input pin", &wave_parser_sink_ops, NULL);
    object->init_gst = wave_parser_filter_init_gst;
    object->source_query_accept = wave_parser_source_query_accept;
    object->source_get_media_type = wave_parser_source_get_media_type;

    TRACE("Created WAVE parser %p.\n", object);
    *out = &object->filter.IUnknown_inner;
    return S_OK;
}

static HRESULT avi_splitter_sink_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    if (IsEqualGUID(&mt->majortype, &MEDIATYPE_Stream)
            && IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_Avi))
        return S_OK;
    return S_FALSE;
}

static const struct strmbase_sink_ops avi_splitter_sink_ops =
{
    .base.pin_query_accept = avi_splitter_sink_query_accept,
    .sink_connect = parser_sink_connect,
    .sink_disconnect = parser_sink_disconnect,
};

static BOOL avi_splitter_filter_init_gst(struct parser *filter)
{
    wg_parser_t parser = filter->wg_parser;
    uint32_t i, stream_count;
    WCHAR source_name[20];

    stream_count = wg_parser_get_stream_count(parser);
    for (i = 0; i < stream_count; ++i)
    {
        swprintf(source_name, ARRAY_SIZE(source_name), L"Stream %02u", i);
        if (!create_pin(filter, wg_parser_get_stream(parser, i), source_name))
            return FALSE;
    }

    return TRUE;
}

static HRESULT avi_splitter_source_query_accept(struct parser_source *pin, const AM_MEDIA_TYPE *mt)
{
    struct wg_format format;
    AM_MEDIA_TYPE pad_mt;
    HRESULT hr;

    wg_parser_stream_get_preferred_format(pin->wg_stream, &format);
    if (!amt_from_wg_format(&pad_mt, &format, false))
        return E_OUTOFMEMORY;
    hr = compare_media_types(mt, &pad_mt) ? S_OK : S_FALSE;
    FreeMediaType(&pad_mt);
    return hr;
}

static HRESULT avi_splitter_source_get_media_type(struct parser_source *pin,
        unsigned int index, AM_MEDIA_TYPE *mt)
{
    struct wg_format format;

    if (index > 0)
        return VFW_S_NO_MORE_ITEMS;
    wg_parser_stream_get_preferred_format(pin->wg_stream, &format);
    if (!amt_from_wg_format(mt, &format, false))
        return E_OUTOFMEMORY;
    return S_OK;
}

HRESULT avi_splitter_create(IUnknown *outer, IUnknown **out)
{
    struct parser *object;
    HRESULT hr;

    if (FAILED(hr = parser_create(WG_PARSER_AVIDEMUX, FALSE, &object)))
        return hr;

    strmbase_filter_init(&object->filter, outer, &CLSID_AviSplitter, &filter_ops);
    strmbase_sink_init(&object->sink, &object->filter, L"input pin", &avi_splitter_sink_ops, NULL);
    object->init_gst = avi_splitter_filter_init_gst;
    object->source_query_accept = avi_splitter_source_query_accept;
    object->source_get_media_type = avi_splitter_source_get_media_type;

    TRACE("Created AVI splitter %p.\n", object);
    *out = &object->filter.IUnknown_inner;
    return S_OK;
}

static HRESULT mpeg_splitter_sink_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    if (!IsEqualGUID(&mt->majortype, &MEDIATYPE_Stream))
        return S_FALSE;
    if (IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_MPEG1Audio)
            || IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_MPEG1System))
        return S_OK;
    if (IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_MPEG1Video)
            || IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_MPEG1VideoCD))
        FIXME("Unsupported subtype %s.\n", wine_dbgstr_guid(&mt->subtype));
    return S_FALSE;
}

static const struct strmbase_sink_ops mpeg_splitter_sink_ops =
{
    .base.pin_query_accept = mpeg_splitter_sink_query_accept,
    .sink_connect = parser_sink_connect,
    .sink_disconnect = parser_sink_disconnect,
};

static BOOL mpeg_splitter_filter_init_gst(struct parser *filter)
{
    wg_parser_t parser = filter->wg_parser;
    unsigned int i, stream_count;
    wg_parser_stream_t stream;
    struct wg_format fmt;

    stream_count = wg_parser_get_stream_count(parser);
    for (i = 0; i < stream_count; ++i)
    {
        stream = wg_parser_get_stream(parser, i);
        wg_parser_stream_get_preferred_format(stream, &fmt);
        if (fmt.major_type == WG_MAJOR_TYPE_VIDEO_MPEG1)
        {
            if (!create_pin(filter, wg_parser_get_stream(parser, i), L"Video"))
                return FALSE;
        }
        else if (fmt.major_type == WG_MAJOR_TYPE_AUDIO_MPEG1)
        {
            if (!create_pin(filter, wg_parser_get_stream(parser, i), L"Audio"))
                return FALSE;
        }
        else FIXME("unexpected format %u\n", fmt.major_type);
    }

    return TRUE;
}

static HRESULT mpeg_splitter_source_query_accept(struct parser_source *pin, const AM_MEDIA_TYPE *mt)
{
    struct wg_format format;
    AM_MEDIA_TYPE pad_mt;
    HRESULT hr;

    wg_parser_stream_get_preferred_format(pin->wg_stream, &format);
    if (!amt_from_wg_format(&pad_mt, &format, false))
        return E_OUTOFMEMORY;
    hr = compare_media_types(mt, &pad_mt) ? S_OK : S_FALSE;
    FreeMediaType(&pad_mt);
    return hr;
}

static HRESULT mpeg_splitter_source_get_media_type(struct parser_source *pin,
        unsigned int index, AM_MEDIA_TYPE *mt)
{
    struct wg_format format;

    if (index > 0)
        return VFW_S_NO_MORE_ITEMS;
    wg_parser_stream_get_preferred_format(pin->wg_stream, &format);
    if (!amt_from_wg_format(mt, &format, false))
        return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT mpeg_splitter_query_interface(struct strmbase_filter *iface, REFIID iid, void **out)
{
    struct parser *filter = impl_from_strmbase_filter(iface);

    if (IsEqualGUID(iid, &IID_IAMStreamSelect))
    {
        *out = &filter->IAMStreamSelect_iface;
        IUnknown_AddRef((IUnknown *)*out);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static const struct strmbase_filter_ops mpeg_splitter_ops =
{
    .filter_query_interface = mpeg_splitter_query_interface,
    .filter_get_pin = parser_get_pin,
    .filter_destroy = parser_destroy,
    .filter_init_stream = parser_init_stream,
    .filter_cleanup_stream = parser_cleanup_stream,
};

HRESULT mpeg_splitter_create(IUnknown *outer, IUnknown **out)
{
    struct parser *object;
    HRESULT hr;

    if (FAILED(hr = parser_create(WG_PARSER_DECODEBIN, TRUE, &object)))
        return hr;

    strmbase_filter_init(&object->filter, outer, &CLSID_MPEG1Splitter, &mpeg_splitter_ops);
    strmbase_sink_init(&object->sink, &object->filter, L"Input", &mpeg_splitter_sink_ops, NULL);
    object->IAMStreamSelect_iface.lpVtbl = &stream_select_vtbl;

    object->init_gst = mpeg_splitter_filter_init_gst;
    object->source_query_accept = mpeg_splitter_source_query_accept;
    object->source_get_media_type = mpeg_splitter_source_get_media_type;
    object->enum_sink_first = TRUE;

    TRACE("Created MPEG-1 splitter %p.\n", object);
    *out = &object->filter.IUnknown_inner;
    return S_OK;
}
