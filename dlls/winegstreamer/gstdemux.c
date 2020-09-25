/*
 * GStreamer splitter + decoder, adapted from parser.c
 *
 * Copyright 2010 Maarten Lankhorst for CodeWeavers
 * Copyright 2010 Aric Stewart for CodeWeavers
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

#include "config.h"
#include "gst_private.h"
#include "gst_guids.h"
#include "gst_cbs.h"

#include "vfwmsgs.h"
#include "amvideo.h"

#include "wine/unicode.h"
#include "wine/debug.h"

#include <assert.h>

#include "dvdmedia.h"
#include "mmreg.h"
#include "ks.h"
#include "initguid.h"
#include "wmcodecdsp.h"
#include "ksmedia.h"

WINE_DEFAULT_DEBUG_CHANNEL(gstreamer);

static const GUID MEDIASUBTYPE_CVID = {mmioFOURCC('c','v','i','d'), 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};

struct gstdemux
{
    struct strmbase_filter filter;
    IAMStreamSelect IAMStreamSelect_iface;

    struct strmbase_sink sink;
    IAsyncReader *reader;

    struct gstdemux_source **sources;
    unsigned int source_count;
    BOOL enum_sink_first;

    LONGLONG filesize;

    BOOL initial, ignore_flush;
    GstElement *container;
    GstPad *my_src, *their_sink;
    GstBus *bus;
    guint64 start, nextofs, nextpullofs, stop;
    HANDLE no_more_pads_event, duration_event, error_event;

    HANDLE push_thread;

    BOOL (*init_gst)(struct gstdemux *filter);
    HRESULT (*source_query_accept)(struct gstdemux_source *pin, const AM_MEDIA_TYPE *mt);
    HRESULT (*source_get_media_type)(struct gstdemux_source *pin, unsigned int index, AM_MEDIA_TYPE *mt);
};

struct gstdemux_source
{
    struct strmbase_source pin;
    IQualityControl IQualityControl_iface;

    GstPad *their_src, *post_sink, *post_src, *my_sink;
    GstElement *flip;
    HANDLE caps_event, eos_event;
    GstSegment *segment;
    SourceSeeking seek;
};

static inline struct gstdemux *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct gstdemux, filter);
}

static const WCHAR wcsInputPinName[] = {'i','n','p','u','t',' ','p','i','n',0};
static const IMediaSeekingVtbl GST_Seeking_Vtbl;
static const IQualityControlVtbl GSTOutPin_QualityControl_Vtbl;

static struct gstdemux_source *create_pin(struct gstdemux *filter, const WCHAR *name);
static HRESULT GST_RemoveOutputPins(struct gstdemux *This);
static HRESULT WINAPI GST_ChangeCurrent(IMediaSeeking *iface);
static HRESULT WINAPI GST_ChangeStop(IMediaSeeking *iface);
static HRESULT WINAPI GST_ChangeRate(IMediaSeeking *iface);

static gboolean amt_from_gst_audio_info(const GstAudioInfo *info, AM_MEDIA_TYPE *amt)
{
    WAVEFORMATEXTENSIBLE *wfe;
    WAVEFORMATEX *wfx;
    gint32 depth, bpp;

    wfe = CoTaskMemAlloc(sizeof(*wfe));
    wfx = (WAVEFORMATEX*)wfe;
    amt->majortype = MEDIATYPE_Audio;
    amt->subtype = MEDIASUBTYPE_PCM;
    amt->formattype = FORMAT_WaveFormatEx;
    amt->pbFormat = (BYTE*)wfe;
    amt->cbFormat = sizeof(*wfe);
    amt->bFixedSizeSamples = TRUE;
    amt->bTemporalCompression = FALSE;
    amt->pUnk = NULL;

    wfx->wFormatTag = WAVE_FORMAT_EXTENSIBLE;

    wfx->nChannels = info->channels;
    wfx->nSamplesPerSec = info->rate;
    depth = GST_AUDIO_INFO_WIDTH(info);
    bpp = GST_AUDIO_INFO_DEPTH(info);

    if (!depth || depth > 32 || depth % 8)
        depth = bpp;
    else if (!bpp)
        bpp = depth;
    wfe->Samples.wValidBitsPerSample = depth;
    wfx->wBitsPerSample = bpp;
    wfx->cbSize = sizeof(*wfe)-sizeof(*wfx);
    switch (wfx->nChannels) {
        case 1: wfe->dwChannelMask = KSAUDIO_SPEAKER_MONO; break;
        case 2: wfe->dwChannelMask = KSAUDIO_SPEAKER_STEREO; break;
        case 4: wfe->dwChannelMask = KSAUDIO_SPEAKER_SURROUND; break;
        case 5: wfe->dwChannelMask = (KSAUDIO_SPEAKER_5POINT1 & ~SPEAKER_LOW_FREQUENCY); break;
        case 6: wfe->dwChannelMask = KSAUDIO_SPEAKER_5POINT1; break;
        case 8: wfe->dwChannelMask = KSAUDIO_SPEAKER_7POINT1; break;
        default:
        wfe->dwChannelMask = 0;
    }
    if (GST_AUDIO_INFO_IS_FLOAT(info))
    {
        amt->subtype = MEDIASUBTYPE_IEEE_FLOAT;
        wfe->SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    } else {
        wfe->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
        if (wfx->nChannels <= 2 && bpp <= 16 && depth == bpp)  {
            wfx->wFormatTag = WAVE_FORMAT_PCM;
            wfx->cbSize = 0;
            amt->cbFormat = sizeof(WAVEFORMATEX);
        }
    }
    amt->lSampleSize = wfx->nBlockAlign = wfx->nChannels * wfx->wBitsPerSample/8;
    wfx->nAvgBytesPerSec = wfx->nSamplesPerSec * wfx->nBlockAlign;
    return TRUE;
}

static gboolean amt_from_gst_video_info(const GstVideoInfo *info, AM_MEDIA_TYPE *amt)
{
    VIDEOINFOHEADER *vih;
    BITMAPINFOHEADER *bih;
    gint32 width, height;

    width = GST_VIDEO_INFO_WIDTH(info);
    height = GST_VIDEO_INFO_HEIGHT(info);

    vih = CoTaskMemAlloc(sizeof(*vih));
    bih = &vih->bmiHeader;

    amt->formattype = FORMAT_VideoInfo;
    amt->pbFormat = (BYTE*)vih;
    amt->cbFormat = sizeof(*vih);
    amt->bFixedSizeSamples = FALSE;
    amt->bTemporalCompression = TRUE;
    amt->lSampleSize = 1;
    amt->pUnk = NULL;
    ZeroMemory(vih, sizeof(*vih));
    amt->majortype = MEDIATYPE_Video;

    if (GST_VIDEO_INFO_IS_RGB(info))
    {
        switch (GST_VIDEO_INFO_FORMAT(info))
        {
        case GST_VIDEO_FORMAT_BGRA:
            amt->subtype = MEDIASUBTYPE_ARGB32;
            bih->biBitCount = 32;
            break;
        case GST_VIDEO_FORMAT_BGRx:
            amt->subtype = MEDIASUBTYPE_RGB32;
            bih->biBitCount = 32;
            break;
        case GST_VIDEO_FORMAT_BGR:
            amt->subtype = MEDIASUBTYPE_RGB24;
            bih->biBitCount = 24;
            break;
        case GST_VIDEO_FORMAT_BGR16:
            amt->subtype = MEDIASUBTYPE_RGB565;
            bih->biBitCount = 16;
            break;
        case GST_VIDEO_FORMAT_BGR15:
            amt->subtype = MEDIASUBTYPE_RGB555;
            bih->biBitCount = 16;
            break;
        default:
            WARN("Cannot convert %s to a DirectShow type.\n", GST_VIDEO_INFO_NAME(info));
            CoTaskMemFree(vih);
            return FALSE;
        }
        bih->biCompression = BI_RGB;
    } else {
        amt->subtype = MEDIATYPE_Video;
        if (!(amt->subtype.Data1 = gst_video_format_to_fourcc(GST_VIDEO_INFO_FORMAT(info))))
        {
            CoTaskMemFree(vih);
            return FALSE;
        }
        switch (amt->subtype.Data1) {
            case mmioFOURCC('I','4','2','0'):
            case mmioFOURCC('Y','V','1','2'):
            case mmioFOURCC('N','V','1','2'):
            case mmioFOURCC('N','V','2','1'):
                bih->biBitCount = 12; break;
            case mmioFOURCC('Y','U','Y','2'):
            case mmioFOURCC('Y','V','Y','U'):
            case mmioFOURCC('U','Y','V','Y'):
                bih->biBitCount = 16; break;
        }
        bih->biCompression = amt->subtype.Data1;
    }
    bih->biSizeImage = GST_VIDEO_INFO_SIZE(info);
    if ((vih->AvgTimePerFrame = (REFERENCE_TIME)MulDiv(10000000,
            GST_VIDEO_INFO_FPS_D(info), GST_VIDEO_INFO_FPS_N(info))) == -1)
        vih->AvgTimePerFrame = 0; /* zero division or integer overflow */
    bih->biSize = sizeof(*bih);
    bih->biWidth = width;
    bih->biHeight = height;
    bih->biPlanes = 1;
    return TRUE;
}

static gboolean amt_from_gst_caps_audio_mpeg(const GstCaps *caps, AM_MEDIA_TYPE *mt)
{
    GstStructure *structure = gst_caps_get_structure(caps, 0);
    gint layer, channels, rate;

    mt->majortype = MEDIATYPE_Audio;
    mt->subtype = MEDIASUBTYPE_MPEG1AudioPayload;
    mt->bFixedSizeSamples = FALSE;
    mt->bTemporalCompression = FALSE;
    mt->lSampleSize = 0;
    mt->formattype = FORMAT_WaveFormatEx;
    mt->pUnk = NULL;

    if (!gst_structure_get_int(structure, "layer", &layer))
    {
        WARN("Missing 'layer' value.\n");
        return FALSE;
    }
    if (!gst_structure_get_int(structure, "channels", &channels))
    {
        WARN("Missing 'channels' value.\n");
        return FALSE;
    }
    if (!gst_structure_get_int(structure, "rate", &rate))
    {
        WARN("Missing 'rate' value.\n");
        return FALSE;
    }

    if (layer == 3)
    {
        MPEGLAYER3WAVEFORMAT *wfx = CoTaskMemAlloc(sizeof(*wfx));
        memset(wfx, 0, sizeof(*wfx));

        mt->subtype.Data1 = WAVE_FORMAT_MPEGLAYER3;
        mt->cbFormat = sizeof(*wfx);
        mt->pbFormat = (BYTE *)wfx;
        wfx->wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3;
        wfx->wfx.nChannels = channels;
        wfx->wfx.nSamplesPerSec = rate;
        /* FIXME: We can't get most of the MPEG data from the caps. We may have
         * to manually parse the header. */
        wfx->wfx.cbSize = sizeof(*wfx) - sizeof(WAVEFORMATEX);
        wfx->wID = MPEGLAYER3_ID_MPEG;
        wfx->fdwFlags = MPEGLAYER3_FLAG_PADDING_ON;
        wfx->nFramesPerBlock = 1;
        wfx->nCodecDelay = 1393;
    }
    else
    {
        MPEG1WAVEFORMAT *wfx = CoTaskMemAlloc(sizeof(*wfx));
        memset(wfx, 0, sizeof(*wfx));

        mt->subtype.Data1 = WAVE_FORMAT_MPEG;
        mt->cbFormat = sizeof(*wfx);
        mt->pbFormat = (BYTE *)wfx;
        wfx->wfx.wFormatTag = WAVE_FORMAT_MPEG;
        wfx->wfx.nChannels = channels;
        wfx->wfx.nSamplesPerSec = rate;
        wfx->wfx.cbSize = sizeof(*wfx) - sizeof(WAVEFORMATEX);
        wfx->fwHeadLayer = layer;
    }

    return TRUE;
}

static gboolean amt_from_gst_caps(const GstCaps *caps, AM_MEDIA_TYPE *mt)
{
    const char *type = gst_structure_get_name(gst_caps_get_structure(caps, 0));
    GstStructure *structure = gst_caps_get_structure(caps, 0);

    memset(mt, 0, sizeof(AM_MEDIA_TYPE));

    if (!strcmp(type, "audio/x-raw"))
    {
        GstAudioInfo info;

        if (!(gst_audio_info_from_caps(&info, caps)))
            return FALSE;
        return amt_from_gst_audio_info(&info, mt);
    }
    else if (!strcmp(type, "video/x-raw"))
    {
        GstVideoInfo info;

        if (!gst_video_info_from_caps(&info, caps))
            return FALSE;
        return amt_from_gst_video_info(&info, mt);
    }
    else if (!strcmp(type, "audio/mpeg"))
        return amt_from_gst_caps_audio_mpeg(caps, mt);
    else if (!strcmp(type, "video/x-cinepak"))
    {
        VIDEOINFOHEADER *vih;
        gint i;

        mt->majortype = MEDIATYPE_Video;
        mt->subtype = MEDIASUBTYPE_CVID;
        mt->bTemporalCompression = TRUE;
        mt->lSampleSize = 1;
        mt->formattype = FORMAT_VideoInfo;
        if (!(vih = CoTaskMemAlloc(sizeof(VIDEOINFOHEADER))))
            return FALSE;
        mt->cbFormat = sizeof(VIDEOINFOHEADER);
        mt->pbFormat = (BYTE *)vih;

        memset(vih, 0, sizeof(VIDEOINFOHEADER));
        vih->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        if (gst_structure_get_int(structure, "width", &i))
            vih->bmiHeader.biWidth = i;
        if (gst_structure_get_int(structure, "height", &i))
            vih->bmiHeader.biHeight = i;
        vih->bmiHeader.biPlanes = 1;
        /* Both ffmpeg's encoder and a Cinepak file seen in the wild report
         * 24 bpp. ffmpeg sets biSizeImage as below; others may be smaller, but
         * as long as every sample fits into our allocator, we're fine. */
        vih->bmiHeader.biBitCount = 24;
        vih->bmiHeader.biCompression = mmioFOURCC('c','v','i','d');
        vih->bmiHeader.biSizeImage = vih->bmiHeader.biWidth
                * vih->bmiHeader.biHeight * vih->bmiHeader.biBitCount / 8;
        return TRUE;
    }
    else
    {
        FIXME("Unhandled type %s.\n", debugstr_a(type));
        return FALSE;
    }
}

static GstCaps *amt_to_gst_caps_video(const AM_MEDIA_TYPE *mt)
{
    static const struct
    {
        const GUID *subtype;
        GstVideoFormat format;
    }
    format_map[] =
    {
        {&MEDIASUBTYPE_ARGB32,  GST_VIDEO_FORMAT_BGRA},
        {&MEDIASUBTYPE_RGB32,   GST_VIDEO_FORMAT_BGRx},
        {&MEDIASUBTYPE_RGB24,   GST_VIDEO_FORMAT_BGR},
        {&MEDIASUBTYPE_RGB565,  GST_VIDEO_FORMAT_BGR16},
        {&MEDIASUBTYPE_RGB555,  GST_VIDEO_FORMAT_BGR15},
    };

    const VIDEOINFOHEADER *vih = (VIDEOINFOHEADER *)mt->pbFormat;
    GstVideoFormat format = GST_VIDEO_FORMAT_UNKNOWN;
    GstVideoInfo info;
    unsigned int i;
    GstCaps *caps;

    if (!IsEqualGUID(&mt->formattype, &FORMAT_VideoInfo)
            || mt->cbFormat < sizeof(VIDEOINFOHEADER) || !mt->pbFormat)
        return NULL;

    for (i = 0; i < ARRAY_SIZE(format_map); ++i)
    {
        if (IsEqualGUID(&mt->subtype, format_map[i].subtype))
        {
            format = format_map[i].format;
            break;
        }
    }

    if (format == GST_VIDEO_FORMAT_UNKNOWN)
        format = gst_video_format_from_fourcc(vih->bmiHeader.biCompression);

    if (format == GST_VIDEO_FORMAT_UNKNOWN)
    {
        FIXME("Unknown video format (subtype %s, compression %#x).\n",
                debugstr_guid(&mt->subtype), vih->bmiHeader.biCompression);
        return NULL;
    }

    gst_video_info_set_format(&info, format, vih->bmiHeader.biWidth, vih->bmiHeader.biHeight);
    if ((caps = gst_video_info_to_caps(&info)))
    {
        /* Clear some fields that shouldn't prevent us from connecting. */
        for (i = 0; i < gst_caps_get_size(caps); ++i)
        {
            gst_structure_remove_fields(gst_caps_get_structure(caps, i),
                    "framerate", "pixel-aspect-ratio", "colorimetry", "chroma-site", NULL);
        }
    }
    return caps;
}

static GstCaps *amt_to_gst_caps_audio(const AM_MEDIA_TYPE *mt)
{
    const WAVEFORMATEX *wfx = (WAVEFORMATEX *)mt->pbFormat;
    GstAudioFormat format = GST_AUDIO_FORMAT_UNKNOWN;
    GstAudioInfo info;

    if (!IsEqualGUID(&mt->formattype, &FORMAT_WaveFormatEx)
            || mt->cbFormat < sizeof(WAVEFORMATEX) || !mt->pbFormat)
        return NULL;

    if (IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_PCM))
        format = gst_audio_format_build_integer(wfx->wBitsPerSample != 8,
                G_LITTLE_ENDIAN, wfx->wBitsPerSample, wfx->wBitsPerSample);
    else if (IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_IEEE_FLOAT))
    {
        if (wfx->wBitsPerSample == 32)
            format = GST_AUDIO_FORMAT_F32LE;
        else if (wfx->wBitsPerSample == 64)
            format = GST_AUDIO_FORMAT_F64LE;
    }

    if (format == GST_AUDIO_FORMAT_UNKNOWN)
    {
        FIXME("Unknown audio format (subtype %s, depth %u).\n",
                debugstr_guid(&mt->subtype), wfx->wBitsPerSample);
        return NULL;
    }

    gst_audio_info_set_format(&info, format, wfx->nSamplesPerSec, wfx->nChannels, NULL);
    return gst_audio_info_to_caps(&info);
}

static GstCaps *amt_to_gst_caps(const AM_MEDIA_TYPE *mt)
{
    if (IsEqualGUID(&mt->majortype, &MEDIATYPE_Video))
        return amt_to_gst_caps_video(mt);
    else if (IsEqualGUID(&mt->majortype, &MEDIATYPE_Audio))
        return amt_to_gst_caps_audio(mt);

    FIXME("Unknown major type %s.\n", debugstr_guid(&mt->majortype));
    return NULL;
}

static gboolean query_sink(GstPad *pad, GstObject *parent, GstQuery *query)
{
    struct gstdemux_source *pin = gst_pad_get_element_private(pad);

    TRACE("pin %p, type \"%s\".\n", pin, gst_query_type_get_name(query->type));

    switch (query->type)
    {
        case GST_QUERY_CAPS:
        {
            GstCaps *caps, *filter, *temp;

            gst_query_parse_caps(query, &filter);

            if (pin->pin.pin.peer)
                caps = amt_to_gst_caps(&pin->pin.pin.mt);
            else
                caps = gst_caps_new_any();
            if (!caps)
                return FALSE;

            if (filter)
            {
                temp = gst_caps_intersect(caps, filter);
                gst_caps_unref(caps);
                caps = temp;
            }

            gst_query_set_caps_result(query, caps);
            gst_caps_unref(caps);
            return TRUE;
        }
        case GST_QUERY_ACCEPT_CAPS:
        {
            gboolean ret = TRUE;
            AM_MEDIA_TYPE mt;
            GstCaps *caps;

            if (!pin->pin.pin.peer)
            {
                gst_query_set_accept_caps_result(query, TRUE);
                return TRUE;
            }

            gst_query_parse_accept_caps(query, &caps);
            if (!amt_from_gst_caps(caps, &mt))
                return FALSE;

            if (!IsEqualGUID(&mt.majortype, &pin->pin.pin.mt.majortype)
                    || !IsEqualGUID(&mt.subtype, &pin->pin.pin.mt.subtype)
                    || !IsEqualGUID(&mt.formattype, &pin->pin.pin.mt.formattype))
                ret = FALSE;

            if (IsEqualGUID(&mt.majortype, &MEDIATYPE_Video))
            {
                const VIDEOINFOHEADER *req_vih = (VIDEOINFOHEADER *)mt.pbFormat;
                const VIDEOINFOHEADER *our_vih = (VIDEOINFOHEADER *)pin->pin.pin.mt.pbFormat;

                if (req_vih->bmiHeader.biWidth != our_vih->bmiHeader.biWidth
                        || req_vih->bmiHeader.biHeight != our_vih->bmiHeader.biHeight
                        || req_vih->bmiHeader.biBitCount != our_vih->bmiHeader.biBitCount
                        || req_vih->bmiHeader.biCompression != our_vih->bmiHeader.biCompression)
                    ret = FALSE;
            }
            else if (IsEqualGUID(&mt.majortype, &MEDIATYPE_Audio))
            {
                const WAVEFORMATEX *req_wfx = (WAVEFORMATEX *)mt.pbFormat;
                const WAVEFORMATEX *our_wfx = (WAVEFORMATEX *)pin->pin.pin.mt.pbFormat;

                if (req_wfx->nChannels != our_wfx->nChannels
                        || req_wfx->nSamplesPerSec != our_wfx->nSamplesPerSec
                        || req_wfx->wBitsPerSample != our_wfx->wBitsPerSample)
                    ret = FALSE;
            }

            FreeMediaType(&mt);

            if (!ret && WARN_ON(gstreamer))
            {
                gchar *str = gst_caps_to_string(caps);
                WARN("Rejecting caps \"%s\".\n", debugstr_a(str));
                g_free(str);
            }

            gst_query_set_accept_caps_result(query, ret);
            return TRUE;
        }
        default:
            return gst_pad_query_default (pad, parent, query);
    }
}

static gboolean gst_base_src_perform_seek(struct gstdemux *This, GstEvent *event)
{
    gboolean res = TRUE;
    gdouble rate;
    GstFormat seek_format;
    GstSeekFlags flags;
    GstSeekType cur_type, stop_type;
    gint64 cur, stop;
    gboolean flush;
    guint32 seqnum;
    GstEvent *tevent;
    BOOL thread = !!This->push_thread;

    gst_event_parse_seek(event, &rate, &seek_format, &flags,
                         &cur_type, &cur, &stop_type, &stop);

    if (seek_format != GST_FORMAT_BYTES)
    {
        FIXME("Unhandled format \"%s\".\n", gst_format_get_name(seek_format));
        return FALSE;
    }

    flush = flags & GST_SEEK_FLAG_FLUSH;
    seqnum = gst_event_get_seqnum(event);

    /* send flush start */
    if (flush) {
        tevent = gst_event_new_flush_start();
        gst_event_set_seqnum(tevent, seqnum);
        gst_pad_push_event(This->my_src, tevent);
        if (This->reader)
            IAsyncReader_BeginFlush(This->reader);
        if (thread)
            gst_pad_set_active(This->my_src, 1);
    }

    This->nextofs = This->start = cur;

    /* and prepare to continue streaming */
    if (flush) {
        tevent = gst_event_new_flush_stop(TRUE);
        gst_event_set_seqnum(tevent, seqnum);
        gst_pad_push_event(This->my_src, tevent);
        if (This->reader)
            IAsyncReader_EndFlush(This->reader);
        if (thread)
            gst_pad_set_active(This->my_src, 1);
    }

    return res;
}

static gboolean event_src(GstPad *pad, GstObject *parent, GstEvent *event)
{
    struct gstdemux *This = gst_pad_get_element_private(pad);

    TRACE("filter %p, type \"%s\".\n", This, GST_EVENT_TYPE_NAME(event));

    switch (event->type) {
        case GST_EVENT_SEEK:
            return gst_base_src_perform_seek(This, event);
        case GST_EVENT_FLUSH_START:
            EnterCriticalSection(&This->filter.csFilter);
            if (This->reader)
                IAsyncReader_BeginFlush(This->reader);
            LeaveCriticalSection(&This->filter.csFilter);
            break;
        case GST_EVENT_FLUSH_STOP:
            EnterCriticalSection(&This->filter.csFilter);
            if (This->reader)
                IAsyncReader_EndFlush(This->reader);
            LeaveCriticalSection(&This->filter.csFilter);
            break;
        default:
            WARN("Ignoring \"%s\" event.\n", GST_EVENT_TYPE_NAME(event));
        case GST_EVENT_TAG:
        case GST_EVENT_QOS:
        case GST_EVENT_RECONFIGURE:
            return gst_pad_event_default(pad, parent, event);
    }
    return TRUE;
}

static gboolean event_sink(GstPad *pad, GstObject *parent, GstEvent *event)
{
    struct gstdemux_source *pin = gst_pad_get_element_private(pad);
    gboolean ret;

    TRACE("pin %p, type \"%s\".\n", pin, GST_EVENT_TYPE_NAME(event));

    switch (event->type) {
        case GST_EVENT_SEGMENT: {
            gdouble rate, applied_rate;
            gint64 stop, pos;
            const GstSegment *segment;

            gst_event_parse_segment(event, &segment);

            pos = segment->position;
            stop = segment->stop;
            rate = segment->rate;
            applied_rate = segment->applied_rate;

            if (segment->format != GST_FORMAT_TIME)
            {
                FIXME("Unhandled format \"%s\".\n", gst_format_get_name(segment->format));
                return TRUE;
            }

            gst_segment_copy_into(segment, pin->segment);

            pos /= 100;

            if (stop > 0)
                stop /= 100;

            if (pin->pin.pin.peer)
                IPin_NewSegment(pin->pin.pin.peer, pos, stop, rate*applied_rate);

            return TRUE;
        }
        case GST_EVENT_EOS:
            if (pin->pin.pin.peer)
                IPin_EndOfStream(pin->pin.pin.peer);
            else
                SetEvent(pin->eos_event);
            return TRUE;
        case GST_EVENT_FLUSH_START:
            if (impl_from_strmbase_filter(pin->pin.pin.filter)->ignore_flush) {
                /* gst-plugins-base prior to 1.7 contains a bug which causes
                 * our sink pins to receive a flush-start event when the
                 * decodebin changes from PAUSED to READY (including
                 * PLAYING->PAUSED->READY), but no matching flush-stop event is
                 * sent. See <gst-plugins-base.git:60bad4815db966a8e4). Here we
                 * unset the flushing flag to avoid the problem. */
                TRACE("Working around gst <1.7 bug, ignoring FLUSH_START\n");
                GST_PAD_UNSET_FLUSHING (pad);
                return TRUE;
            }
            if (pin->pin.pin.peer)
                IPin_BeginFlush(pin->pin.pin.peer);
            return TRUE;
        case GST_EVENT_FLUSH_STOP:
            gst_segment_init(pin->segment, GST_FORMAT_TIME);
            if (pin->pin.pin.peer)
                IPin_EndFlush(pin->pin.pin.peer);
            return TRUE;
        case GST_EVENT_CAPS:
            ret = gst_pad_event_default(pad, parent, event);
            SetEvent(pin->caps_event);
            return ret;
        default:
            WARN("Ignoring \"%s\" event.\n", GST_EVENT_TYPE_NAME(event));
            return gst_pad_event_default(pad, parent, event);
    }
}

static DWORD CALLBACK push_data(LPVOID iface)
{
    LONGLONG maxlen, curlen;
    struct gstdemux *This = iface;
    GstMapInfo mapping;
    GstBuffer *buffer;
    HRESULT hr;

    if (!(buffer = gst_buffer_new_allocate(NULL, 16384, NULL)))
    {
        ERR("Failed to allocate memory.\n");
        return 0;
    }

    IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);

    if (!This->stop)
        IAsyncReader_Length(This->reader, &maxlen, &curlen);
    else
        maxlen = This->stop;

    TRACE("Starting..\n");
    for (;;) {
        ULONG len;
        int ret;

        if (This->nextofs >= maxlen)
            break;
        len = min(16384, maxlen - This->nextofs);

        if (!gst_buffer_map_range(buffer, -1, len, &mapping, GST_MAP_WRITE))
        {
            ERR("Failed to map buffer.\n");
            break;
        }
        hr = IAsyncReader_SyncRead(This->reader, This->nextofs, len, mapping.data);
        gst_buffer_unmap(buffer, &mapping);
        if (hr != S_OK)
        {
            ERR("Failed to read data, hr %#x.\n", hr);
            break;
        }

        This->nextofs += len;

        buffer->duration = buffer->pts = -1;
        ret = gst_pad_push(This->my_src, buffer);
        if (ret >= 0)
            hr = S_OK;
        else
            ERR("Sending returned: %i\n", ret);
        if (ret == GST_FLOW_ERROR)
            hr = E_FAIL;
        else if (ret == GST_FLOW_FLUSHING)
            hr = VFW_E_WRONG_STATE;
        if (hr != S_OK)
            break;
    }

    gst_buffer_unref(buffer);

    gst_pad_push_event(This->my_src, gst_event_new_eos());

    TRACE("Stopping.. %08x\n", hr);

    IBaseFilter_Release(&This->filter.IBaseFilter_iface);

    return 0;
}

static GstFlowReturn got_data_sink(GstPad *pad, GstObject *parent, GstBuffer *buf)
{
    struct gstdemux_source *pin = gst_pad_get_element_private(pad);
    struct gstdemux *This = impl_from_strmbase_filter(pin->pin.pin.filter);
    HRESULT hr;
    BYTE *ptr = NULL;
    IMediaSample *sample;
    GstMapInfo info;

    TRACE("%p %p\n", pad, buf);

    if (This->initial) {
        gst_buffer_unref(buf);
        return GST_FLOW_OK;
    }

    hr = BaseOutputPinImpl_GetDeliveryBuffer(&pin->pin, &sample, NULL, NULL, 0);

    if (hr == VFW_E_NOT_CONNECTED) {
        gst_buffer_unref(buf);
        return GST_FLOW_NOT_LINKED;
    }

    if (FAILED(hr)) {
        gst_buffer_unref(buf);
        ERR("Could not get a delivery buffer (%x), returning GST_FLOW_FLUSHING\n", hr);
        return GST_FLOW_FLUSHING;
    }

    gst_buffer_map(buf, &info, GST_MAP_READ);

    hr = IMediaSample_SetActualDataLength(sample, info.size);
    if(FAILED(hr)){
        WARN("SetActualDataLength failed: %08x\n", hr);
        return GST_FLOW_FLUSHING;
    }

    IMediaSample_GetPointer(sample, &ptr);

    memcpy(ptr, info.data, info.size);

    gst_buffer_unmap(buf, &info);

    if (GST_BUFFER_PTS_IS_VALID(buf)) {
        REFERENCE_TIME rtStart = gst_segment_to_running_time(pin->segment, GST_FORMAT_TIME, buf->pts);
        if (rtStart >= 0)
            rtStart /= 100;

        if (GST_BUFFER_DURATION_IS_VALID(buf)) {
            REFERENCE_TIME tStart = buf->pts / 100;
            REFERENCE_TIME tStop = (buf->pts + buf->duration) / 100;
            REFERENCE_TIME rtStop;
            rtStop = gst_segment_to_running_time(pin->segment, GST_FORMAT_TIME, buf->pts + buf->duration);
            if (rtStop >= 0)
                rtStop /= 100;
            TRACE("Current time on %p: %i to %i ms\n", pin, (int)(rtStart / 10000), (int)(rtStop / 10000));
            IMediaSample_SetTime(sample, &rtStart, rtStop >= 0 ? &rtStop : NULL);
            IMediaSample_SetMediaTime(sample, &tStart, &tStop);
        } else {
            IMediaSample_SetTime(sample, rtStart >= 0 ? &rtStart : NULL, NULL);
            IMediaSample_SetMediaTime(sample, NULL, NULL);
        }
    } else {
        IMediaSample_SetTime(sample, NULL, NULL);
        IMediaSample_SetMediaTime(sample, NULL, NULL);
    }

    IMediaSample_SetDiscontinuity(sample, GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_DISCONT));
    IMediaSample_SetPreroll(sample, GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_LIVE));
    IMediaSample_SetSyncPoint(sample, !GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_DELTA_UNIT));

    if (!pin->pin.pin.peer)
        hr = VFW_E_NOT_CONNECTED;
    else
        hr = IMemInputPin_Receive(pin->pin.pMemInputPin, sample);

    TRACE("sending sample returned: %08x\n", hr);

    gst_buffer_unref(buf);
    IMediaSample_Release(sample);

    if (hr == VFW_E_NOT_CONNECTED)
        return GST_FLOW_NOT_LINKED;

    if (FAILED(hr))
        return GST_FLOW_FLUSHING;

    return GST_FLOW_OK;
}

static GstFlowReturn request_buffer_src(GstPad *pad, GstObject *parent, guint64 ofs, guint len, GstBuffer **buffer)
{
    struct gstdemux *This = gst_pad_get_element_private(pad);
    GstBuffer *new_buffer = NULL;
    HRESULT hr;
    GstMapInfo info;

    TRACE("pad %p, offset %s, length %u, buffer %p.\n", pad, wine_dbgstr_longlong(ofs), len, *buffer);

    if (ofs == GST_BUFFER_OFFSET_NONE)
        ofs = This->nextpullofs;
    if (ofs >= This->filesize) {
        WARN("Reading past eof: %s, %u\n", wine_dbgstr_longlong(ofs), len);
        return GST_FLOW_EOS;
    }
    if (len + ofs > This->filesize)
        len = This->filesize - ofs;
    This->nextpullofs = ofs + len;

    if (!*buffer)
        *buffer = new_buffer = gst_buffer_new_and_alloc(len);
    gst_buffer_map(*buffer, &info, GST_MAP_WRITE);
    hr = IAsyncReader_SyncRead(This->reader, ofs, len, info.data);
    gst_buffer_unmap(*buffer, &info);
    if (FAILED(hr))
    {
        ERR("Failed to read data, hr %#x.\n", hr);
        if (new_buffer)
            gst_buffer_unref(new_buffer);
        return GST_FLOW_ERROR;
    }

    return GST_FLOW_OK;
}

static DWORD CALLBACK push_data_init(LPVOID iface)
{
    struct gstdemux *This = iface;
    DWORD64 ofs = 0;

    TRACE("Starting..\n");
    for (;;) {
        GstBuffer *buf;
        GstFlowReturn ret = request_buffer_src(This->my_src, NULL, ofs, 4096, &buf);
        if (ret < 0) {
            ERR("Obtaining buffer returned: %i\n", ret);
            break;
        }
        ret = gst_pad_push(This->my_src, buf);
        ofs += 4096;
        if (ret)
            TRACE("Sending returned: %i\n", ret);
        if (ret < 0)
            break;
    }
    TRACE("Stopping..\n");
    return 0;
}

static void removed_decoded_pad(GstElement *bin, GstPad *pad, gpointer user)
{
    struct gstdemux *filter = user;
    unsigned int i;
    char *name;

    TRACE("filter %p, bin %p, pad %p.\n", filter, bin, pad);

    for (i = 0; i < filter->source_count; ++i)
    {
        struct gstdemux_source *pin = filter->sources[i];

        if (pin->their_src == pad)
        {
            if (pin->post_sink)
                gst_pad_unlink(pin->their_src, pin->post_sink);
            else
                gst_pad_unlink(pin->their_src, pin->my_sink);
            gst_object_unref(pin->their_src);
            pin->their_src = NULL;
            return;
        }
    }

    name = gst_pad_get_name(pad);
    WARN("No pin matching pad %s found.\n", debugstr_a(name));
    g_free(name);
}

static void init_new_decoded_pad(GstElement *bin, GstPad *pad, struct gstdemux *This)
{
    static const WCHAR formatW[] = {'S','t','r','e','a','m',' ','%','0','2','u',0};
    const char *typename;
    char *name;
    GstCaps *caps;
    GstStructure *arg;
    struct gstdemux_source *pin;
    int ret;
    WCHAR nameW[128];

    TRACE("%p %p %p\n", This, bin, pad);

    sprintfW(nameW, formatW, This->source_count);

    name = gst_pad_get_name(pad);
    TRACE("Name: %s\n", name);
    g_free(name);

    caps = gst_pad_query_caps(pad, NULL);
    caps = gst_caps_make_writable(caps);
    arg = gst_caps_get_structure(caps, 0);
    typename = gst_structure_get_name(arg);

    if (!(pin = create_pin(This, nameW)))
    {
        ERR("Failed to allocate memory.\n");
        goto out;
    }

    if (!strcmp(typename, "video/x-raw"))
    {
        GstElement *vconv, *flip, *deinterlace;

        /* DirectShow can express interlaced video, but downstream filters can't
         * necessarily consume it. In particular, the video renderer can't. */
        if (!(deinterlace = gst_element_factory_make("deinterlace", NULL)))
        {
            ERR("Failed to create deinterlace, are %u-bit GStreamer \"good\" plugins installed?\n",
                    8 * (int)sizeof(void *));
            goto out;
        }

        /* decodebin considers many YUV formats to be "raw", but some quartz
         * filters can't handle those. Also, videoflip can't handle all "raw"
         * formats either. Add a videoconvert to swap color spaces. */
        if (!(vconv = gst_element_factory_make("videoconvert", NULL)))
        {
            ERR("Failed to create videoconvert, are %u-bit GStreamer \"base\" plugins installed?\n",
                    8 * (int)sizeof(void *));
            goto out;
        }

        /* Avoid expensive color matrix conversions. */
        gst_util_set_object_arg(G_OBJECT(vconv), "matrix-mode", "none");

        /* GStreamer outputs RGB video top-down, but DirectShow expects bottom-up. */
        if (!(flip = gst_element_factory_make("videoflip", NULL)))
        {
            ERR("Failed to create videoflip, are %u-bit GStreamer \"good\" plugins installed?\n",
                    8 * (int)sizeof(void *));
            goto out;
        }

        /* The bin takes ownership of these elements. */
        gst_bin_add(GST_BIN(This->container), deinterlace);
        gst_element_sync_state_with_parent(deinterlace);
        gst_bin_add(GST_BIN(This->container), vconv);
        gst_element_sync_state_with_parent(vconv);
        gst_bin_add(GST_BIN(This->container), flip);
        gst_element_sync_state_with_parent(flip);

        gst_element_link(deinterlace, vconv);
        gst_element_link(vconv, flip);

        pin->post_sink = gst_element_get_static_pad(deinterlace, "sink");
        pin->post_src = gst_element_get_static_pad(flip, "src");
        pin->flip = flip;
    }
    else if (!strcmp(typename, "audio/x-raw"))
    {
        GstElement *convert;

        /* Currently our dsound can't handle 64-bit formats or all
         * surround-sound configurations. Native dsound can't always handle
         * 64-bit formats either. Add an audioconvert to allow changing bit
         * depth and channel count. */
        if (!(convert = gst_element_factory_make("audioconvert", NULL)))
        {
            ERR("Failed to create audioconvert, are %u-bit GStreamer \"base\" plugins installed?\n",
                    8 * (int)sizeof(void *));
            goto out;
        }

        gst_bin_add(GST_BIN(This->container), convert);
        gst_element_sync_state_with_parent(convert);

        pin->post_sink = gst_element_get_static_pad(convert, "sink");
        pin->post_src = gst_element_get_static_pad(convert, "src");
    }

    if (pin->post_sink)
    {
        if ((ret = gst_pad_link(pad, pin->post_sink)) < 0)
        {
            ERR("Failed to link decodebin source pad to post-processing elements, error %s.\n",
                    gst_pad_link_get_name(ret));
            gst_object_unref(pin->post_sink);
            pin->post_sink = NULL;
            goto out;
        }

        if ((ret = gst_pad_link(pin->post_src, pin->my_sink)) < 0)
        {
            ERR("Failed to link post-processing elements to our sink pad, error %s.\n",
                    gst_pad_link_get_name(ret));
            gst_object_unref(pin->post_src);
            pin->post_src = NULL;
            gst_object_unref(pin->post_sink);
            pin->post_sink = NULL;
            goto out;
        }
    }
    else if ((ret = gst_pad_link(pad, pin->my_sink)) < 0)
    {
        ERR("Failed to link decodebin source pad to our sink pad, error %s.\n",
                gst_pad_link_get_name(ret));
        goto out;
    }

    gst_pad_set_active(pin->my_sink, 1);
    gst_object_ref(pin->their_src = pad);
out:
    gst_caps_unref(caps);
}

static void existing_new_pad(GstElement *bin, GstPad *pad, gpointer user)
{
    struct gstdemux *This = user;
    unsigned int i;
    int ret;

    TRACE("%p %p %p\n", This, bin, pad);

    if (gst_pad_is_linked(pad))
        return;

    /* Still holding our own lock */
    if (This->initial) {
        init_new_decoded_pad(bin, pad, This);
        return;
    }

    for (i = 0; i < This->source_count; ++i)
    {
        struct gstdemux_source *pin = This->sources[i];
        if (!pin->their_src) {
            gst_segment_init(pin->segment, GST_FORMAT_TIME);

            if (pin->post_sink)
                ret = gst_pad_link(pad, pin->post_sink);
            else
                ret = gst_pad_link(pad, pin->my_sink);

            if (ret >= 0) {
                pin->their_src = pad;
                gst_object_ref(pin->their_src);
                TRACE("Relinked\n");
                return;
            }
        }
    }
    init_new_decoded_pad(bin, pad, This);
}

static gboolean query_function(GstPad *pad, GstObject *parent, GstQuery *query)
{
    struct gstdemux *This = gst_pad_get_element_private(pad);
    GstFormat format;

    TRACE("filter %p, type %s.\n", This, GST_QUERY_TYPE_NAME(query));

    switch (GST_QUERY_TYPE(query)) {
        case GST_QUERY_DURATION:
            gst_query_parse_duration(query, &format, NULL);
            if (format == GST_FORMAT_PERCENT)
            {
                gst_query_set_duration(query, GST_FORMAT_PERCENT, GST_FORMAT_PERCENT_MAX);
                return TRUE;
            }
            else if (format == GST_FORMAT_BYTES)
            {
                gst_query_set_duration(query, GST_FORMAT_BYTES, This->filesize);
                return TRUE;
            }
            return FALSE;
        case GST_QUERY_SEEKING:
            gst_query_parse_seeking (query, &format, NULL, NULL, NULL);
            if (format != GST_FORMAT_BYTES)
            {
                WARN("Cannot seek using format \"%s\".\n", gst_format_get_name(format));
                return FALSE;
            }
            gst_query_set_seeking(query, GST_FORMAT_BYTES, 1, 0, This->filesize);
            return TRUE;
        case GST_QUERY_SCHEDULING:
            gst_query_set_scheduling(query, GST_SCHEDULING_FLAG_SEEKABLE, 1, -1, 0);
            gst_query_add_scheduling_mode(query, GST_PAD_MODE_PUSH);
            gst_query_add_scheduling_mode(query, GST_PAD_MODE_PULL);
            return TRUE;
        default:
            WARN("Unhandled query type %s.\n", GST_QUERY_TYPE_NAME(query));
            return FALSE;
    }
}

static gboolean activate_push(GstPad *pad, gboolean activate)
{
    struct gstdemux *This = gst_pad_get_element_private(pad);

    EnterCriticalSection(&This->filter.csFilter);
    if (!activate) {
        TRACE("Deactivating\n");
        if (!This->initial)
            IAsyncReader_BeginFlush(This->reader);
        if (This->push_thread) {
            WaitForSingleObject(This->push_thread, -1);
            CloseHandle(This->push_thread);
            This->push_thread = NULL;
        }
        if (!This->initial)
            IAsyncReader_EndFlush(This->reader);
        if (This->filter.state == State_Stopped)
            This->nextofs = This->start;
    } else if (!This->push_thread) {
        TRACE("Activating\n");
        if (This->initial)
            This->push_thread = CreateThread(NULL, 0, push_data_init, This, 0, NULL);
        else
            This->push_thread = CreateThread(NULL, 0, push_data, This, 0, NULL);
    }
    LeaveCriticalSection(&This->filter.csFilter);
    return TRUE;
}

static gboolean activate_mode(GstPad *pad, GstObject *parent, GstPadMode mode, gboolean activate)
{
    struct gstdemux *filter = gst_pad_get_element_private(pad);

    TRACE("%s source pad for filter %p in %s mode.\n",
            activate ? "Activating" : "Deactivating", filter, gst_pad_mode_get_name(mode));

    switch (mode) {
      case GST_PAD_MODE_PULL:
        return TRUE;
      case GST_PAD_MODE_PUSH:
        return activate_push(pad, activate);
      default:
        return FALSE;
    }
    return FALSE;
}

static void no_more_pads(GstElement *decodebin, gpointer user)
{
    struct gstdemux *filter = user;
    TRACE("filter %p.\n", filter);
    SetEvent(filter->no_more_pads_event);
}

static GstAutoplugSelectResult autoplug_blacklist(GstElement *bin, GstPad *pad, GstCaps *caps, GstElementFactory *fact, gpointer user)
{
    const char *name = gst_element_factory_get_longname(fact);

    if (strstr(name, "Player protection")) {
        WARN("Blacklisted a/52 decoder because it only works in Totem\n");
        return GST_AUTOPLUG_SELECT_SKIP;
    }
    if (!strcmp(name, "Fluendo Hardware Accelerated Video Decoder")) {
        WARN("Disabled video acceleration since it breaks in wine\n");
        return GST_AUTOPLUG_SELECT_SKIP;
    }
    TRACE("using \"%s\"\n", name);
    return GST_AUTOPLUG_SELECT_TRY;
}

static GstBusSyncReply watch_bus(GstBus *bus, GstMessage *msg, gpointer data)
{
    struct gstdemux *filter = data;
    GError *err = NULL;
    gchar *dbg_info = NULL;

    TRACE("filter %p, message type %s.\n", filter, GST_MESSAGE_TYPE_NAME(msg));

    switch (msg->type)
    {
    case GST_MESSAGE_ERROR:
        gst_message_parse_error(msg, &err, &dbg_info);
        ERR("%s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
        ERR("%s\n", dbg_info);
        g_error_free(err);
        g_free(dbg_info);
        SetEvent(filter->error_event);
        break;
    case GST_MESSAGE_WARNING:
        gst_message_parse_warning(msg, &err, &dbg_info);
        WARN("%s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
        WARN("%s\n", dbg_info);
        g_error_free(err);
        g_free(dbg_info);
        break;
    case GST_MESSAGE_DURATION_CHANGED:
        SetEvent(filter->duration_event);
        break;
    default:
        break;
    }
    gst_message_unref(msg);
    return GST_BUS_DROP;
}

static void unknown_type(GstElement *bin, GstPad *pad, GstCaps *caps, gpointer user)
{
    gchar *strcaps = gst_caps_to_string(caps);
    ERR("Could not find a filter for caps: %s\n", debugstr_a(strcaps));
    g_free(strcaps);
}

static HRESULT GST_Connect(struct gstdemux *This, IPin *pConnectPin)
{
    LONGLONG avail;
    GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE(
        "quartz_src",
        GST_PAD_SRC,
        GST_PAD_ALWAYS,
        GST_STATIC_CAPS_ANY);

    IAsyncReader_Length(This->reader, &This->filesize, &avail);

    if (!This->bus) {
        This->bus = gst_bus_new();
        gst_bus_set_sync_handler(This->bus, watch_bus_wrapper, This, NULL);
    }

    This->container = gst_bin_new(NULL);
    gst_element_set_bus(This->container, This->bus);

    This->my_src = gst_pad_new_from_static_template(&src_template, "quartz-src");
    gst_pad_set_getrange_function(This->my_src, request_buffer_src_wrapper);
    gst_pad_set_query_function(This->my_src, query_function_wrapper);
    gst_pad_set_activatemode_function(This->my_src, activate_mode_wrapper);
    gst_pad_set_event_function(This->my_src, event_src_wrapper);
    gst_pad_set_element_private (This->my_src, This);

    This->start = This->nextofs = This->nextpullofs = This->stop = 0;

    This->initial = TRUE;
    if (!This->init_gst(This))
        return E_FAIL;
    This->initial = FALSE;

    This->nextofs = This->nextpullofs = 0;
    return S_OK;
}

static LONGLONG query_duration(GstPad *pad)
{
    gint64 duration, byte_length;

    if (gst_pad_query_duration(pad, GST_FORMAT_TIME, &duration))
        return duration / 100;

    WARN("Failed to query time duration; trying to convert from byte length.\n");

    /* To accurately get a duration for the stream, we want to only consider the
     * length of that stream. Hence, query for the pad duration, instead of
     * using the file duration. */
    if (gst_pad_query_duration(pad, GST_FORMAT_BYTES, &byte_length)
            && gst_pad_query_convert(pad, GST_FORMAT_BYTES, byte_length, GST_FORMAT_TIME, &duration))
        return duration / 100;

    ERR("Failed to query duration.\n");
    return 0;
}

static inline struct gstdemux_source *impl_from_IMediaSeeking(IMediaSeeking *iface)
{
    return CONTAINING_RECORD(iface, struct gstdemux_source, seek.IMediaSeeking_iface);
}

static struct strmbase_pin *gstdemux_get_pin(struct strmbase_filter *base, unsigned int index)
{
    struct gstdemux *filter = impl_from_strmbase_filter(base);

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

static void gstdemux_destroy(struct strmbase_filter *iface)
{
    struct gstdemux *filter = impl_from_strmbase_filter(iface);
    HRESULT hr;

    CloseHandle(filter->no_more_pads_event);
    CloseHandle(filter->duration_event);
    CloseHandle(filter->error_event);

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

    if (filter->bus)
    {
        gst_bus_set_sync_handler(filter->bus, NULL, NULL, NULL);
        gst_object_unref(filter->bus);
    }
    strmbase_sink_cleanup(&filter->sink);
    strmbase_filter_cleanup(&filter->filter);
    heap_free(filter);
}

static HRESULT gstdemux_init_stream(struct strmbase_filter *iface)
{
    struct gstdemux *filter = impl_from_strmbase_filter(iface);
    HRESULT hr = VFW_E_NOT_CONNECTED, pin_hr;
    const SourceSeeking *seeking;
    GstStateChangeReturn ret;
    unsigned int i;

    if (!filter->container)
        return VFW_E_NOT_CONNECTED;

    for (i = 0; i < filter->source_count; ++i)
    {
        if (SUCCEEDED(pin_hr = BaseOutputPinImpl_Active(&filter->sources[i]->pin)))
            hr = pin_hr;
    }

    if (FAILED(hr))
        return hr;

    if (filter->no_more_pads_event)
        ResetEvent(filter->no_more_pads_event);

    if ((ret = gst_element_set_state(filter->container, GST_STATE_PAUSED)) == GST_STATE_CHANGE_FAILURE)
    {
        ERR("Failed to pause stream.\n");
        return E_FAIL;
    }

    /* Make sure that all of our pads are connected before returning, lest we
     * e.g. try to seek and fail. */
    if (filter->no_more_pads_event)
        WaitForSingleObject(filter->no_more_pads_event, INFINITE);

    seeking = &filter->sources[0]->seek;

    /* GStreamer can't seek while stopped, and it resets position to the
     * beginning of the stream every time it is stopped. */
    if (seeking->llCurrent)
    {
        GstSeekType stop_type = GST_SEEK_TYPE_NONE;

        if (seeking->llStop && seeking->llStop != seeking->llDuration)
            stop_type = GST_SEEK_TYPE_SET;

        gst_pad_push_event(filter->sources[0]->my_sink, gst_event_new_seek(
                seeking->dRate, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                GST_SEEK_TYPE_SET, seeking->llCurrent * 100,
                stop_type, seeking->llStop * 100));
    }

    return hr;
}

static HRESULT gstdemux_start_stream(struct strmbase_filter *iface, REFERENCE_TIME time)
{
    struct gstdemux *filter = impl_from_strmbase_filter(iface);
    GstStateChangeReturn ret;

    if (!filter->container)
        return VFW_E_NOT_CONNECTED;

    if ((ret = gst_element_set_state(filter->container, GST_STATE_PLAYING)) == GST_STATE_CHANGE_FAILURE)
    {
        ERR("Failed to play stream.\n");
        return E_FAIL;
    }
    else if (ret == GST_STATE_CHANGE_ASYNC)
        return S_FALSE;
    return S_OK;
}

static HRESULT gstdemux_stop_stream(struct strmbase_filter *iface)
{
    struct gstdemux *filter = impl_from_strmbase_filter(iface);
    GstStateChangeReturn ret;

    if (!filter->container)
        return VFW_E_NOT_CONNECTED;

    if ((ret = gst_element_set_state(filter->container, GST_STATE_PAUSED)) == GST_STATE_CHANGE_FAILURE)
    {
        ERR("Failed to pause stream.\n");
        return E_FAIL;
    }
    else if (ret == GST_STATE_CHANGE_ASYNC)
        return S_FALSE;
    return S_OK;
}

static HRESULT gstdemux_cleanup_stream(struct strmbase_filter *iface)
{
    struct gstdemux *filter = impl_from_strmbase_filter(iface);
    GstStateChangeReturn ret;
    unsigned int i;

    if (!filter->container)
        return S_OK;

    filter->ignore_flush = TRUE;
    if ((ret = gst_element_set_state(filter->container, GST_STATE_READY)) == GST_STATE_CHANGE_FAILURE)
    {
        ERR("Failed to pause stream.\n");
        return E_FAIL;
    }
    gst_element_get_state(filter->container, NULL, NULL, GST_CLOCK_TIME_NONE);
    filter->ignore_flush = FALSE;

    for (i = 0; i < filter->source_count; ++i)
    {
        if (filter->sources[i]->pin.pin.peer)
            IMemAllocator_Decommit(filter->sources[i]->pin.pAllocator);
    }

    return S_OK;
}

static HRESULT gstdemux_wait_state(struct strmbase_filter *iface, DWORD timeout)
{
    struct gstdemux *filter = impl_from_strmbase_filter(iface);
    GstStateChangeReturn ret;

    if (!filter->container)
        return S_OK;

    ret = gst_element_get_state(filter->container, NULL, NULL,
            timeout == INFINITE ? GST_CLOCK_TIME_NONE : timeout * 1000000);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        ERR("Failed to get state.\n");
        return E_FAIL;
    }
    else if (ret == GST_STATE_CHANGE_ASYNC)
        return VFW_S_STATE_INTERMEDIATE;
    return S_OK;
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = gstdemux_get_pin,
    .filter_destroy = gstdemux_destroy,
    .filter_init_stream = gstdemux_init_stream,
    .filter_start_stream = gstdemux_start_stream,
    .filter_stop_stream = gstdemux_stop_stream,
    .filter_cleanup_stream = gstdemux_cleanup_stream,
    .filter_wait_state = gstdemux_wait_state,
};

static inline struct gstdemux *impl_from_strmbase_sink(struct strmbase_sink *iface)
{
    return CONTAINING_RECORD(iface, struct gstdemux, sink);
}

static HRESULT sink_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    if (IsEqualGUID(&mt->majortype, &MEDIATYPE_Stream))
        return S_OK;
    return S_FALSE;
}

static HRESULT gstdemux_sink_connect(struct strmbase_sink *iface, IPin *peer, const AM_MEDIA_TYPE *pmt)
{
    struct gstdemux *filter = impl_from_strmbase_sink(iface);
    HRESULT hr = S_OK;

    mark_wine_thread();

    filter->reader = NULL;
    if (FAILED(hr = IPin_QueryInterface(peer, &IID_IAsyncReader, (void **)&filter->reader)))
        return hr;

    if (FAILED(hr = GST_Connect(filter, peer)))
        goto err;

    return S_OK;
err:
    GST_RemoveOutputPins(filter);
    IAsyncReader_Release(filter->reader);
    filter->reader = NULL;
    return hr;
}

static void gstdemux_sink_disconnect(struct strmbase_sink *iface)
{
    struct gstdemux *filter = impl_from_strmbase_sink(iface);

    mark_wine_thread();

    GST_RemoveOutputPins(filter);

    IAsyncReader_Release(filter->reader);
    filter->reader = NULL;
}

static const struct strmbase_sink_ops sink_ops =
{
    .base.pin_query_accept = sink_query_accept,
    .sink_connect = gstdemux_sink_connect,
    .sink_disconnect = gstdemux_sink_disconnect,
};

static BOOL gstdecoder_init_gst(struct gstdemux *filter)
{
    GstElement *element = gst_element_factory_make("decodebin", NULL);
    unsigned int i;
    int ret;

    if (!element)
    {
        ERR("Failed to create decodebin; are %u-bit GStreamer \"base\" plugins installed?\n",
                8 * (int)sizeof(void*));
        return FALSE;
    }

    gst_bin_add(GST_BIN(filter->container), element);

    g_signal_connect(element, "pad-added", G_CALLBACK(existing_new_pad_wrapper), filter);
    g_signal_connect(element, "pad-removed", G_CALLBACK(removed_decoded_pad_wrapper), filter);
    g_signal_connect(element, "autoplug-select", G_CALLBACK(autoplug_blacklist_wrapper), filter);
    g_signal_connect(element, "unknown-type", G_CALLBACK(unknown_type_wrapper), filter);
    g_signal_connect(element, "no-more-pads", G_CALLBACK(no_more_pads_wrapper), filter);

    filter->their_sink = gst_element_get_static_pad(element, "sink");
    ResetEvent(filter->no_more_pads_event);

    if ((ret = gst_pad_link(filter->my_src, filter->their_sink)) < 0)
    {
        ERR("Failed to link pads, error %d.\n", ret);
        return FALSE;
    }

    gst_element_set_state(filter->container, GST_STATE_PLAYING);
    ret = gst_element_get_state(filter->container, NULL, NULL, -1);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        ERR("Failed to play stream.\n");
        return FALSE;
    }

    WaitForSingleObject(filter->no_more_pads_event, INFINITE);

    for (i = 0; i < filter->source_count; ++i)
    {
        struct gstdemux_source *pin = filter->sources[i];
        const HANDLE events[2] = {pin->caps_event, filter->error_event};

        pin->seek.llDuration = pin->seek.llStop = query_duration(pin->their_src);
        pin->seek.llCurrent = 0;
        if (WaitForMultipleObjects(2, events, FALSE, INFINITE))
            return FALSE;
    }

    filter->ignore_flush = TRUE;
    gst_element_set_state(filter->container, GST_STATE_READY);
    gst_element_get_state(filter->container, NULL, NULL, -1);
    filter->ignore_flush = FALSE;

    return TRUE;
}

static HRESULT gstdecoder_source_query_accept(struct gstdemux_source *pin, const AM_MEDIA_TYPE *mt)
{
    /* At least make sure we can convert it to GstCaps. */
    GstCaps *caps = amt_to_gst_caps(mt);

    if (!caps)
        return S_FALSE;
    gst_caps_unref(caps);
    return S_OK;
}

static HRESULT gstdecoder_source_get_media_type(struct gstdemux_source *pin,
        unsigned int index, AM_MEDIA_TYPE *mt)
{
    GstCaps *caps = gst_pad_get_current_caps(pin->my_sink);
    const GstStructure *structure;
    const char *type;

    static const GstVideoFormat video_formats[] =
    {
        /* Try to prefer YUV formats over RGB ones. Most decoders output in the
         * YUV color space, and it's generally much less expensive for
         * videoconvert to do YUV -> YUV transformations. */
        GST_VIDEO_FORMAT_AYUV,
        GST_VIDEO_FORMAT_I420,
        GST_VIDEO_FORMAT_YV12,
        GST_VIDEO_FORMAT_YUY2,
        GST_VIDEO_FORMAT_UYVY,
        GST_VIDEO_FORMAT_YVYU,
        GST_VIDEO_FORMAT_NV12,
        GST_VIDEO_FORMAT_BGRA,
        GST_VIDEO_FORMAT_BGRx,
        GST_VIDEO_FORMAT_BGR,
    };

    assert(caps); /* We shouldn't be able to get here if caps haven't been set. */
    structure = gst_caps_get_structure(caps, 0);
    type = gst_structure_get_name(structure);

    memset(mt, 0, sizeof(AM_MEDIA_TYPE));

    if (amt_from_gst_caps(caps, mt))
    {
        if (!index--)
        {
            gst_caps_unref(caps);
            return S_OK;
        }
        FreeMediaType(mt);
    }

    if (!strcmp(type, "video/x-raw") && index < ARRAY_SIZE(video_formats))
    {
        gint width, height, fps_n, fps_d;
        GstVideoInfo info;

        gst_caps_unref(caps);
        gst_structure_get_int(structure, "width", &width);
        gst_structure_get_int(structure, "height", &height);
        gst_video_info_set_format(&info, video_formats[index], width, height);
        if (gst_structure_get_fraction(structure, "framerate", &fps_n, &fps_d) && fps_n)
        {
            info.fps_n = fps_n;
            info.fps_d = fps_d;
        }
        if (!amt_from_gst_video_info(&info, mt))
            return E_OUTOFMEMORY;
        return S_OK;
    }
    else if (!strcmp(type, "audio/x-raw") && !index)
    {
        GstAudioInfo info;
        gint rate;

        gst_caps_unref(caps);
        gst_structure_get_int(structure, "rate", &rate);
        gst_audio_info_set_format(&info, GST_AUDIO_FORMAT_S16LE, rate, 2, NULL);
        if (!amt_from_gst_audio_info(&info, mt))
            return E_OUTOFMEMORY;
        return S_OK;
    }

    gst_caps_unref(caps);
    return VFW_S_NO_MORE_ITEMS;
}

HRESULT gstdemux_create(IUnknown *outer, IUnknown **out)
{
    struct gstdemux *object;

    if (!init_gstreamer())
        return E_FAIL;

    mark_wine_thread();

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_filter_init(&object->filter, outer, &CLSID_Gstreamer_Splitter, &filter_ops);
    strmbase_sink_init(&object->sink, &object->filter, wcsInputPinName, &sink_ops, NULL);

    object->no_more_pads_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    object->error_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    object->init_gst = gstdecoder_init_gst;
    object->source_query_accept = gstdecoder_source_query_accept;
    object->source_get_media_type = gstdecoder_source_get_media_type;

    TRACE("Created GStreamer demuxer %p.\n", object);
    *out = &object->filter.IUnknown_inner;
    return S_OK;
}

static struct gstdemux *impl_from_IAMStreamSelect(IAMStreamSelect *iface)
{
    return CONTAINING_RECORD(iface, struct gstdemux, IAMStreamSelect_iface);
}

static HRESULT WINAPI stream_select_QueryInterface(IAMStreamSelect *iface, REFIID iid, void **out)
{
    struct gstdemux *filter = impl_from_IAMStreamSelect(iface);
    return IUnknown_QueryInterface(filter->filter.outer_unk, iid, out);
}

static ULONG WINAPI stream_select_AddRef(IAMStreamSelect *iface)
{
    struct gstdemux *filter = impl_from_IAMStreamSelect(iface);
    return IUnknown_AddRef(filter->filter.outer_unk);
}

static ULONG WINAPI stream_select_Release(IAMStreamSelect *iface)
{
    struct gstdemux *filter = impl_from_IAMStreamSelect(iface);
    return IUnknown_Release(filter->filter.outer_unk);
}

static HRESULT WINAPI stream_select_Count(IAMStreamSelect *iface, DWORD *count)
{
    FIXME("iface %p, count %p, stub!\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_select_Info(IAMStreamSelect *iface, LONG index,
        AM_MEDIA_TYPE **mt, DWORD *flags, LCID *lcid, DWORD *group, WCHAR **name,
        IUnknown **object, IUnknown **unknown)
{
    FIXME("iface %p, index %d, mt %p, flags %p, lcid %p, group %p, name %p, object %p, unknown %p, stub!\n",
            iface, index, mt, flags, lcid, group, name, object, unknown);
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_select_Enable(IAMStreamSelect *iface, LONG index, DWORD flags)
{
    FIXME("iface %p, index %d, flags %#x, stub!\n", iface, index, flags);
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
    struct gstdemux_source *This = impl_from_IMediaSeeking(iface);
    TRACE("(%p)\n", This);
    return S_OK;
}

static HRESULT WINAPI GST_ChangeStop(IMediaSeeking *iface)
{
    struct gstdemux_source *This = impl_from_IMediaSeeking(iface);
    TRACE("(%p)\n", This);
    return S_OK;
}

static HRESULT WINAPI GST_ChangeRate(IMediaSeeking *iface)
{
    struct gstdemux_source *This = impl_from_IMediaSeeking(iface);
    GstEvent *ev = gst_event_new_seek(This->seek.dRate, GST_FORMAT_TIME, 0, GST_SEEK_TYPE_NONE, -1, GST_SEEK_TYPE_NONE, -1);
    TRACE("(%p) New rate %g\n", This, This->seek.dRate);
    mark_wine_thread();
    gst_pad_push_event(This->my_sink, ev);
    return S_OK;
}

static HRESULT WINAPI GST_Seeking_QueryInterface(IMediaSeeking *iface, REFIID riid, void **ppv)
{
    struct gstdemux_source *This = impl_from_IMediaSeeking(iface);
    return IPin_QueryInterface(&This->pin.pin.IPin_iface, riid, ppv);
}

static ULONG WINAPI GST_Seeking_AddRef(IMediaSeeking *iface)
{
    struct gstdemux_source *This = impl_from_IMediaSeeking(iface);
    return IPin_AddRef(&This->pin.pin.IPin_iface);
}

static ULONG WINAPI GST_Seeking_Release(IMediaSeeking *iface)
{
    struct gstdemux_source *This = impl_from_IMediaSeeking(iface);
    return IPin_Release(&This->pin.pin.IPin_iface);
}

static HRESULT WINAPI GST_Seeking_GetCurrentPosition(IMediaSeeking *iface, REFERENCE_TIME *pos)
{
    struct gstdemux_source *This = impl_from_IMediaSeeking(iface);

    TRACE("(%p)->(%p)\n", This, pos);

    if (!pos)
        return E_POINTER;

    mark_wine_thread();

    if (This->pin.pin.filter->state == State_Stopped)
    {
        *pos = This->seek.llCurrent;
        TRACE("Cached value\n");
        return S_OK;
    }

    if (!gst_pad_query_position(This->their_src, GST_FORMAT_TIME, pos)) {
        WARN("Could not query position\n");
        return E_NOTIMPL;
    }
    *pos /= 100;
    This->seek.llCurrent = *pos;
    return S_OK;
}

static GstSeekType type_from_flags(DWORD flags)
{
    switch (flags & AM_SEEKING_PositioningBitsMask) {
    case AM_SEEKING_NoPositioning:
        return GST_SEEK_TYPE_NONE;
    case AM_SEEKING_AbsolutePositioning:
    case AM_SEEKING_RelativePositioning:
        return GST_SEEK_TYPE_SET;
    case AM_SEEKING_IncrementalPositioning:
        return GST_SEEK_TYPE_END;
    }
    return GST_SEEK_TYPE_NONE;
}

static HRESULT WINAPI GST_Seeking_SetPositions(IMediaSeeking *iface,
        REFERENCE_TIME *pCur, DWORD curflags, REFERENCE_TIME *pStop,
        DWORD stopflags)
{
    HRESULT hr;
    struct gstdemux_source *This = impl_from_IMediaSeeking(iface);
    GstSeekFlags f = 0;
    GstSeekType curtype, stoptype;
    GstEvent *e;
    gint64 stop_pos = 0, curr_pos = 0;

    TRACE("(%p)->(%p, 0x%x, %p, 0x%x)\n", This, pCur, curflags, pStop, stopflags);

    mark_wine_thread();

    hr = SourceSeekingImpl_SetPositions(iface, pCur, curflags, pStop, stopflags);
    if (This->pin.pin.filter->state == State_Stopped)
        return hr;

    curtype = type_from_flags(curflags);
    stoptype = type_from_flags(stopflags);
    if (curflags & AM_SEEKING_SeekToKeyFrame)
        f |= GST_SEEK_FLAG_KEY_UNIT;
    if (curflags & AM_SEEKING_Segment)
        f |= GST_SEEK_FLAG_SEGMENT;
    if (!(curflags & AM_SEEKING_NoFlush))
        f |= GST_SEEK_FLAG_FLUSH;

    if (((curflags & AM_SEEKING_PositioningBitsMask) == AM_SEEKING_RelativePositioning) ||
        ((stopflags & AM_SEEKING_PositioningBitsMask) == AM_SEEKING_RelativePositioning)) {
        gint64 tmp_pos;
        gst_pad_query_position (This->my_sink, GST_FORMAT_TIME, &tmp_pos);
        if ((curflags & AM_SEEKING_PositioningBitsMask) == AM_SEEKING_RelativePositioning)
            curr_pos = tmp_pos;
        if ((stopflags & AM_SEEKING_PositioningBitsMask) == AM_SEEKING_RelativePositioning)
            stop_pos = tmp_pos;
    }

    e = gst_event_new_seek(This->seek.dRate, GST_FORMAT_TIME, f, curtype, pCur ? curr_pos + *pCur * 100 : -1, stoptype, pStop ? stop_pos + *pStop * 100 : -1);
    if (gst_pad_push_event(This->my_sink, e))
        return S_OK;
    else
        return E_NOTIMPL;
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
    GST_Seeking_GetCurrentPosition,
    SourceSeekingImpl_ConvertTimeFormat,
    GST_Seeking_SetPositions,
    SourceSeekingImpl_GetPositions,
    SourceSeekingImpl_GetAvailable,
    SourceSeekingImpl_SetRate,
    SourceSeekingImpl_GetRate,
    SourceSeekingImpl_GetPreroll
};

static inline struct gstdemux_source *impl_from_IQualityControl( IQualityControl *iface )
{
    return CONTAINING_RECORD(iface, struct gstdemux_source, IQualityControl_iface);
}

static HRESULT WINAPI GST_QualityControl_QueryInterface(IQualityControl *iface, REFIID riid, void **ppv)
{
    struct gstdemux_source *pin = impl_from_IQualityControl(iface);
    return IPin_QueryInterface(&pin->pin.pin.IPin_iface, riid, ppv);
}

static ULONG WINAPI GST_QualityControl_AddRef(IQualityControl *iface)
{
    struct gstdemux_source *pin = impl_from_IQualityControl(iface);
    return IPin_AddRef(&pin->pin.pin.IPin_iface);
}

static ULONG WINAPI GST_QualityControl_Release(IQualityControl *iface)
{
    struct gstdemux_source *pin = impl_from_IQualityControl(iface);
    return IPin_Release(&pin->pin.pin.IPin_iface);
}

static HRESULT WINAPI GST_QualityControl_Notify(IQualityControl *iface, IBaseFilter *sender, Quality qm)
{
    struct gstdemux_source *pin = impl_from_IQualityControl(iface);
    GstQOSType type = GST_QOS_TYPE_OVERFLOW;
    GstClockTime timestamp;
    GstClockTimeDiff diff;
    GstEvent *evt;

    TRACE("(%p)->(%p, { 0x%x %u %s %s })\n", pin, sender,
            qm.Type, qm.Proportion,
            wine_dbgstr_longlong(qm.Late),
            wine_dbgstr_longlong(qm.TimeStamp));

    mark_wine_thread();

    /* GSTQOS_TYPE_OVERFLOW is also used for buffers that arrive on time, but
     * DirectShow filters might use Famine, so check that there actually is an
     * underrun. */
    if (qm.Type == Famine && qm.Proportion > 1000)
        type = GST_QOS_TYPE_UNDERFLOW;

    /* DirectShow filters sometimes pass negative timestamps (Audiosurf uses the
     * current time instead of the time of the last buffer). GstClockTime is
     * unsigned, so clamp it to 0. */
    timestamp = max(qm.TimeStamp * 100, 0);

    /* The documentation specifies that timestamp + diff must be nonnegative. */
    diff = qm.Late * 100;
    if (timestamp < -diff)
        diff = -timestamp;

    evt = gst_event_new_qos(type, qm.Proportion / 1000.0, diff, timestamp);

    if (!evt) {
        WARN("Failed to create QOS event\n");
        return E_INVALIDARG;
    }

    gst_pad_push_event(pin->my_sink, evt);

    return S_OK;
}

static HRESULT WINAPI GST_QualityControl_SetSink(IQualityControl *iface, IQualityControl *tonotify)
{
    struct gstdemux_source *pin = impl_from_IQualityControl(iface);
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

static inline struct gstdemux_source *impl_source_from_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, struct gstdemux_source, pin.pin.IPin_iface);
}

static HRESULT source_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct gstdemux_source *pin = impl_source_from_IPin(&iface->IPin_iface);

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
    struct gstdemux_source *pin = impl_source_from_IPin(&iface->IPin_iface);
    struct gstdemux *filter = impl_from_strmbase_filter(iface->filter);
    return filter->source_query_accept(pin, mt);
}

static HRESULT source_get_media_type(struct strmbase_pin *iface, unsigned int index, AM_MEDIA_TYPE *mt)
{
    struct gstdemux_source *pin = impl_source_from_IPin(&iface->IPin_iface);
    struct gstdemux *filter = impl_from_strmbase_filter(iface->filter);
    return filter->source_get_media_type(pin, index, mt);
}

static HRESULT WINAPI GSTOutPin_DecideBufferSize(struct strmbase_source *iface,
        IMemAllocator *allocator, ALLOCATOR_PROPERTIES *props)
{
    struct gstdemux_source *pin = impl_source_from_IPin(&iface->pin.IPin_iface);
    unsigned int buffer_size = 16384;
    ALLOCATOR_PROPERTIES ret_props;

    if (IsEqualGUID(&pin->pin.pin.mt.formattype, &FORMAT_VideoInfo))
    {
        VIDEOINFOHEADER *format = (VIDEOINFOHEADER *)pin->pin.pin.mt.pbFormat;
        buffer_size = format->bmiHeader.biSizeImage;

        gst_util_set_object_arg(G_OBJECT(pin->flip), "method",
                format->bmiHeader.biCompression == BI_RGB ? "vertical-flip" : "none");
    }
    else if (IsEqualGUID(&pin->pin.pin.mt.formattype, &FORMAT_WaveFormatEx)
            && (IsEqualGUID(&pin->pin.pin.mt.subtype, &MEDIASUBTYPE_PCM)
            || IsEqualGUID(&pin->pin.pin.mt.subtype, &MEDIASUBTYPE_IEEE_FLOAT)))
    {
        WAVEFORMATEX *format = (WAVEFORMATEX *)pin->pin.pin.mt.pbFormat;
        buffer_size = format->nAvgBytesPerSec;
    }

    props->cBuffers = max(props->cBuffers, 1);
    props->cbBuffer = max(props->cbBuffer, buffer_size);
    props->cbAlign = max(props->cbAlign, 1);
    return IMemAllocator_SetProperties(allocator, props, &ret_props);
}

static void free_source_pin(struct gstdemux_source *pin)
{
    if (pin->pin.pin.peer)
    {
        if (SUCCEEDED(IMemAllocator_Decommit(pin->pin.pAllocator)))
            IPin_Disconnect(pin->pin.pin.peer);
        IPin_Disconnect(&pin->pin.pin.IPin_iface);
    }

    if (pin->their_src)
    {
        if (pin->post_sink)
        {
            gst_pad_unlink(pin->their_src, pin->post_sink);
            gst_pad_unlink(pin->post_src, pin->my_sink);
            gst_object_unref(pin->post_src);
            gst_object_unref(pin->post_sink);
            pin->post_src = pin->post_sink = NULL;
        }
        else
            gst_pad_unlink(pin->their_src, pin->my_sink);
        gst_object_unref(pin->their_src);
    }
    gst_object_unref(pin->my_sink);
    CloseHandle(pin->caps_event);
    CloseHandle(pin->eos_event);
    gst_segment_free(pin->segment);

    strmbase_seeking_cleanup(&pin->seek);
    strmbase_source_cleanup(&pin->pin);
    heap_free(pin);
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

static struct gstdemux_source *create_pin(struct gstdemux *filter, const WCHAR *name)
{
    struct gstdemux_source *pin, **new_array;
    char pad_name[19];

    if (!(new_array = heap_realloc(filter->sources, (filter->source_count + 1) * sizeof(*new_array))))
        return NULL;
    filter->sources = new_array;

    if (!(pin = heap_alloc_zero(sizeof(*pin))))
        return NULL;

    strmbase_source_init(&pin->pin, &filter->filter, name, &source_ops);
    pin->caps_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    pin->eos_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    pin->segment = gst_segment_new();
    gst_segment_init(pin->segment, GST_FORMAT_TIME);
    pin->IQualityControl_iface.lpVtbl = &GSTOutPin_QualityControl_Vtbl;
    strmbase_seeking_init(&pin->seek, &GST_Seeking_Vtbl, GST_ChangeStop,
            GST_ChangeCurrent, GST_ChangeRate);
    BaseFilterImpl_IncrementPinVersion(&filter->filter);

    sprintf(pad_name, "qz_sink_%u", filter->source_count);
    pin->my_sink = gst_pad_new(pad_name, GST_PAD_SINK);
    gst_pad_set_element_private(pin->my_sink, pin);
    gst_pad_set_chain_function(pin->my_sink, got_data_sink_wrapper);
    gst_pad_set_event_function(pin->my_sink, event_sink_wrapper);
    gst_pad_set_query_function(pin->my_sink, query_sink_wrapper);

    filter->sources[filter->source_count++] = pin;
    return pin;
}

static HRESULT GST_RemoveOutputPins(struct gstdemux *This)
{
    unsigned int i;

    TRACE("(%p)\n", This);
    mark_wine_thread();

    if (!This->container)
        return S_OK;
    gst_element_set_state(This->container, GST_STATE_NULL);
    gst_pad_unlink(This->my_src, This->their_sink);
    gst_object_unref(This->my_src);
    gst_object_unref(This->their_sink);
    This->my_src = This->their_sink = NULL;

    for (i = 0; i < This->source_count; ++i)
        free_source_pin(This->sources[i]);

    This->source_count = 0;
    heap_free(This->sources);
    This->sources = NULL;
    gst_element_set_bus(This->container, NULL);
    gst_object_unref(This->container);
    This->container = NULL;
    BaseFilterImpl_IncrementPinVersion(&This->filter);
    return S_OK;
}

void perform_cb_gstdemux(struct cb_data *cbdata)
{
    switch(cbdata->type)
    {
    case WATCH_BUS:
        {
            struct watch_bus_data *data = &cbdata->u.watch_bus_data;
            cbdata->u.watch_bus_data.ret = watch_bus(data->bus, data->msg, data->user);
            break;
        }
    case EXISTING_NEW_PAD:
        {
            struct pad_added_data *data = &cbdata->u.pad_added_data;
            existing_new_pad(data->element, data->pad, data->user);
            break;
        }
    case QUERY_FUNCTION:
        {
            struct query_function_data *data = &cbdata->u.query_function_data;
            cbdata->u.query_function_data.ret = query_function(data->pad, data->parent, data->query);
            break;
        }
    case ACTIVATE_MODE:
        {
            struct activate_mode_data *data = &cbdata->u.activate_mode_data;
            cbdata->u.activate_mode_data.ret = activate_mode(data->pad, data->parent, data->mode, data->activate);
            break;
        }
    case NO_MORE_PADS:
        {
            struct no_more_pads_data *data = &cbdata->u.no_more_pads_data;
            no_more_pads(data->element, data->user);
            break;
        }
    case REQUEST_BUFFER_SRC:
        {
            struct getrange_data *data = &cbdata->u.getrange_data;
            cbdata->u.getrange_data.ret = request_buffer_src(data->pad, data->parent,
                    data->ofs, data->len, data->buf);
            break;
        }
    case EVENT_SRC:
        {
            struct event_src_data *data = &cbdata->u.event_src_data;
            cbdata->u.event_src_data.ret = event_src(data->pad, data->parent, data->event);
            break;
        }
    case EVENT_SINK:
        {
            struct event_sink_data *data = &cbdata->u.event_sink_data;
            cbdata->u.event_sink_data.ret = event_sink(data->pad, data->parent, data->event);
            break;
        }
    case GOT_DATA_SINK:
        {
            struct got_data_sink_data *data = &cbdata->u.got_data_sink_data;
            cbdata->u.got_data_sink_data.ret = got_data_sink(data->pad, data->parent, data->buf);
            break;
        }
    case REMOVED_DECODED_PAD:
        {
            struct pad_removed_data *data = &cbdata->u.pad_removed_data;
            removed_decoded_pad(data->element, data->pad, data->user);
            break;
        }
    case AUTOPLUG_BLACKLIST:
        {
            struct autoplug_blacklist_data *data = &cbdata->u.autoplug_blacklist_data;
            cbdata->u.autoplug_blacklist_data.ret = autoplug_blacklist(data->bin,
                    data->pad, data->caps, data->fact, data->user);
            break;
        }
    case UNKNOWN_TYPE:
        {
            struct unknown_type_data *data = &cbdata->u.unknown_type_data;
            unknown_type(data->bin, data->pad, data->caps, data->user);
            break;
        }
    case QUERY_SINK:
        {
            struct query_sink_data *data = &cbdata->u.query_sink_data;
            cbdata->u.query_sink_data.ret = query_sink(data->pad, data->parent,
                    data->query);
            break;
        }
    default:
        {
            assert(0);
        }
    }
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
    .sink_connect = gstdemux_sink_connect,
    .sink_disconnect = gstdemux_sink_disconnect,
};

static BOOL wave_parser_init_gst(struct gstdemux *filter)
{
    static const WCHAR source_name[] = {'o','u','t','p','u','t',0};
    struct gstdemux_source *pin;
    GstElement *element;
    HANDLE events[2];
    int ret;

    if (!(element = gst_element_factory_make("wavparse", NULL)))
    {
        ERR("Failed to create wavparse; are %u-bit GStreamer \"good\" plugins installed?\n",
                8 * (int)sizeof(void*));
        return FALSE;
    }

    gst_bin_add(GST_BIN(filter->container), element);

    filter->their_sink = gst_element_get_static_pad(element, "sink");
    if ((ret = gst_pad_link(filter->my_src, filter->their_sink)) < 0)
    {
        ERR("Failed to link sink pads, error %d.\n", ret);
        return FALSE;
    }

    if (!(pin = create_pin(filter, source_name)))
        return FALSE;
    pin->their_src = gst_element_get_static_pad(element, "src");
    gst_object_ref(pin->their_src);
    if ((ret = gst_pad_link(pin->their_src, pin->my_sink)) < 0)
    {
        ERR("Failed to link source pads, error %d.\n", ret);
        return FALSE;
    }

    gst_pad_set_active(pin->my_sink, 1);
    gst_element_set_state(filter->container, GST_STATE_PAUSED);
    ret = gst_element_get_state(filter->container, NULL, NULL, -1);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        ERR("Failed to play stream.\n");
        return FALSE;
    }

    pin->seek.llDuration = pin->seek.llStop = query_duration(pin->their_src);
    pin->seek.llCurrent = 0;

    events[0] = pin->caps_event;
    events[1] = filter->error_event;
    if (WaitForMultipleObjects(2, events, FALSE, INFINITE))
        return FALSE;

    filter->ignore_flush = TRUE;
    gst_element_set_state(filter->container, GST_STATE_READY);
    gst_element_get_state(filter->container, NULL, NULL, -1);
    filter->ignore_flush = FALSE;

    return TRUE;
}

static gboolean get_source_amt(const struct gstdemux_source *pin, AM_MEDIA_TYPE *mt)
{
    GstCaps *caps = gst_pad_get_current_caps(pin->my_sink);
    gboolean ret = amt_from_gst_caps(caps, mt);
    gst_caps_unref(caps);
    return ret;
}

static HRESULT wave_parser_source_query_accept(struct gstdemux_source *pin, const AM_MEDIA_TYPE *mt)
{
    AM_MEDIA_TYPE pad_mt;
    HRESULT hr;

    if (!get_source_amt(pin, &pad_mt))
        return E_OUTOFMEMORY;
    hr = compare_media_types(mt, &pad_mt) ? S_OK : S_FALSE;
    FreeMediaType(&pad_mt);
    return hr;
}

static HRESULT wave_parser_source_get_media_type(struct gstdemux_source *pin,
        unsigned int index, AM_MEDIA_TYPE *mt)
{
    if (index > 0)
        return VFW_S_NO_MORE_ITEMS;
    if (!get_source_amt(pin, mt))
        return E_OUTOFMEMORY;
    return S_OK;
}

HRESULT wave_parser_create(IUnknown *outer, IUnknown **out)
{
    static const WCHAR sink_name[] = {'i','n','p','u','t',' ','p','i','n',0};
    struct gstdemux *object;

    if (!init_gstreamer())
        return E_FAIL;

    mark_wine_thread();

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_filter_init(&object->filter, outer, &CLSID_WAVEParser, &filter_ops);
    strmbase_sink_init(&object->sink, &object->filter, sink_name, &wave_parser_sink_ops, NULL);
    object->init_gst = wave_parser_init_gst;
    object->error_event = CreateEventW(NULL, TRUE, FALSE, NULL);
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
    .sink_connect = gstdemux_sink_connect,
    .sink_disconnect = gstdemux_sink_disconnect,
};

static BOOL avi_splitter_init_gst(struct gstdemux *filter)
{
    GstElement *element = gst_element_factory_make("avidemux", NULL);
    unsigned int i;
    int ret;

    if (!element)
    {
        ERR("Failed to create avidemux; are %u-bit GStreamer \"good\" plugins installed?\n",
                8 * (int)sizeof(void*));
        return FALSE;
    }

    gst_bin_add(GST_BIN(filter->container), element);

    g_signal_connect(element, "pad-added", G_CALLBACK(existing_new_pad_wrapper), filter);
    g_signal_connect(element, "pad-removed", G_CALLBACK(removed_decoded_pad_wrapper), filter);
    g_signal_connect(element, "no-more-pads", G_CALLBACK(no_more_pads_wrapper), filter);

    filter->their_sink = gst_element_get_static_pad(element, "sink");
    ResetEvent(filter->no_more_pads_event);

    if ((ret = gst_pad_link(filter->my_src, filter->their_sink)) < 0)
    {
        ERR("Failed to link pads, error %d.\n", ret);
        return FALSE;
    }

    gst_element_set_state(filter->container, GST_STATE_PLAYING);
    ret = gst_element_get_state(filter->container, NULL, NULL, -1);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        ERR("Failed to play stream.\n");
        return FALSE;
    }

    WaitForSingleObject(filter->no_more_pads_event, INFINITE);

    for (i = 0; i < filter->source_count; ++i)
    {
        struct gstdemux_source *pin = filter->sources[i];
        const HANDLE events[2] = {pin->caps_event, filter->error_event};

        pin->seek.llDuration = pin->seek.llStop = query_duration(pin->their_src);
        pin->seek.llCurrent = 0;
        if (WaitForMultipleObjects(2, events, FALSE, INFINITE))
            return FALSE;
    }

    filter->ignore_flush = TRUE;
    gst_element_set_state(filter->container, GST_STATE_READY);
    gst_element_get_state(filter->container, NULL, NULL, -1);
    filter->ignore_flush = FALSE;

    return TRUE;
}

static HRESULT avi_splitter_source_query_accept(struct gstdemux_source *pin, const AM_MEDIA_TYPE *mt)
{
    AM_MEDIA_TYPE pad_mt;
    HRESULT hr;

    if (!get_source_amt(pin, &pad_mt))
        return E_OUTOFMEMORY;
    hr = compare_media_types(mt, &pad_mt) ? S_OK : S_FALSE;
    FreeMediaType(&pad_mt);
    return hr;
}

static HRESULT avi_splitter_source_get_media_type(struct gstdemux_source *pin,
        unsigned int index, AM_MEDIA_TYPE *mt)
{
    if (index > 0)
        return VFW_S_NO_MORE_ITEMS;
    if (!get_source_amt(pin, mt))
        return E_OUTOFMEMORY;
    return S_OK;
}

HRESULT avi_splitter_create(IUnknown *outer, IUnknown **out)
{
    static const WCHAR sink_name[] = {'i','n','p','u','t',' ','p','i','n',0};
    struct gstdemux *object;

    if (!init_gstreamer())
        return E_FAIL;

    mark_wine_thread();

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_filter_init(&object->filter, outer, &CLSID_AviSplitter, &filter_ops);
    strmbase_sink_init(&object->sink, &object->filter, sink_name, &avi_splitter_sink_ops, NULL);
    object->no_more_pads_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    object->error_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    object->init_gst = avi_splitter_init_gst;
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
    if (IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_MPEG1Audio))
        return S_OK;
    if (IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_MPEG1Video)
            || IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_MPEG1System)
            || IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_MPEG1VideoCD))
        FIXME("Unsupported subtype %s.\n", wine_dbgstr_guid(&mt->subtype));
    return S_FALSE;
}

static const struct strmbase_sink_ops mpeg_splitter_sink_ops =
{
    .base.pin_query_accept = mpeg_splitter_sink_query_accept,
    .sink_connect = gstdemux_sink_connect,
    .sink_disconnect = gstdemux_sink_disconnect,
};

static BOOL mpeg_splitter_init_gst(struct gstdemux *filter)
{
    static const WCHAR source_name[] = {'A','u','d','i','o',0};
    struct gstdemux_source *pin;
    GstElement *element;
    HANDLE events[3];
    DWORD res;
    int ret;

    if (!(element = gst_element_factory_make("mpegaudioparse", NULL)))
    {
        ERR("Failed to create mpegaudioparse; are %u-bit GStreamer \"good\" plugins installed?\n",
                8 * (int)sizeof(void*));
        return FALSE;
    }

    gst_bin_add(GST_BIN(filter->container), element);

    filter->their_sink = gst_element_get_static_pad(element, "sink");
    if ((ret = gst_pad_link(filter->my_src, filter->their_sink)) < 0)
    {
        ERR("Failed to link sink pads, error %d.\n", ret);
        return FALSE;
    }

    if (!(pin = create_pin(filter, source_name)))
        return FALSE;
    gst_object_ref(pin->their_src = gst_element_get_static_pad(element, "src"));
    if ((ret = gst_pad_link(pin->their_src, pin->my_sink)) < 0)
    {
        ERR("Failed to link source pads, error %d.\n", ret);
        return FALSE;
    }

    gst_pad_set_active(pin->my_sink, 1);
    gst_element_set_state(filter->container, GST_STATE_PAUSED);
    ret = gst_element_get_state(filter->container, NULL, NULL, -1);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        ERR("Failed to play stream.\n");
        return FALSE;
    }

    events[0] = filter->duration_event;
    events[1] = filter->error_event;
    events[2] = pin->eos_event;
    res = WaitForMultipleObjects(3, events, FALSE, INFINITE);
    if (res == 1)
        return FALSE;

    pin->seek.llDuration = pin->seek.llStop = query_duration(pin->their_src);
    pin->seek.llCurrent = 0;

    events[0] = pin->caps_event;
    if (WaitForMultipleObjects(2, events, FALSE, INFINITE))
        return FALSE;

    filter->ignore_flush = TRUE;
    gst_element_set_state(filter->container, GST_STATE_READY);
    gst_element_get_state(filter->container, NULL, NULL, -1);
    filter->ignore_flush = FALSE;

    return TRUE;
}

static HRESULT mpeg_splitter_source_query_accept(struct gstdemux_source *pin, const AM_MEDIA_TYPE *mt)
{
    AM_MEDIA_TYPE pad_mt;
    HRESULT hr;

    if (!get_source_amt(pin, &pad_mt))
        return E_OUTOFMEMORY;
    hr = compare_media_types(mt, &pad_mt) ? S_OK : S_FALSE;
    FreeMediaType(&pad_mt);
    return hr;
}

static HRESULT mpeg_splitter_source_get_media_type(struct gstdemux_source *pin,
        unsigned int index, AM_MEDIA_TYPE *mt)
{
    if (index > 0)
        return VFW_S_NO_MORE_ITEMS;
    if (!get_source_amt(pin, mt))
        return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT mpeg_splitter_query_interface(struct strmbase_filter *iface, REFIID iid, void **out)
{
    struct gstdemux *filter = impl_from_strmbase_filter(iface);

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
    .filter_get_pin = gstdemux_get_pin,
    .filter_destroy = gstdemux_destroy,
    .filter_init_stream = gstdemux_init_stream,
    .filter_start_stream = gstdemux_start_stream,
    .filter_stop_stream = gstdemux_stop_stream,
    .filter_cleanup_stream = gstdemux_cleanup_stream,
    .filter_wait_state = gstdemux_wait_state,
};

HRESULT mpeg_splitter_create(IUnknown *outer, IUnknown **out)
{
    static const WCHAR sink_name[] = {'I','n','p','u','t',0};
    struct gstdemux *object;

    if (!init_gstreamer())
        return E_FAIL;

    mark_wine_thread();

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_filter_init(&object->filter, outer, &CLSID_MPEG1Splitter, &mpeg_splitter_ops);
    strmbase_sink_init(&object->sink, &object->filter, sink_name, &mpeg_splitter_sink_ops, NULL);
    object->IAMStreamSelect_iface.lpVtbl = &stream_select_vtbl;

    object->duration_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    object->error_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    object->init_gst = mpeg_splitter_init_gst;
    object->source_query_accept = mpeg_splitter_source_query_accept;
    object->source_get_media_type = mpeg_splitter_source_get_media_type;
    object->enum_sink_first = TRUE;

    TRACE("Created MPEG-1 splitter %p.\n", object);
    *out = &object->filter.IUnknown_inner;
    return S_OK;
}
