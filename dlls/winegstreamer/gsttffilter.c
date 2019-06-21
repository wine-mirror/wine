/*
 * GStreamer wrapper filter
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
 *
 */

#include "config.h"

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/audio/audio.h>

#include "gst_private.h"
#include "gst_guids.h"
#include "gst_cbs.h"

#include "uuids.h"
#include "mmreg.h"
#include "windef.h"
#include "winbase.h"
#include "dshow.h"
#include "strmif.h"
#include "vfwmsgs.h"
#include "dvdmedia.h"
#include "ks.h"
#include "ksmedia.h"
#include "msacm.h"

#include <assert.h>

#include "wine/unicode.h"
#include "wine/debug.h"

#include "initguid.h"
DEFINE_GUID(WMMEDIASUBTYPE_MP3, 0x00000055, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

WINE_DEFAULT_DEBUG_CHANNEL(gstreamer);

struct typeinfo {
    GstCaps *caps;
    const char *type;
};

static gboolean match_element(GstPluginFeature *feature, gpointer gdata)
{
    struct typeinfo *data = (struct typeinfo*)gdata;
    GstElementFactory *factory;
    const GList *list;

    if (!GST_IS_ELEMENT_FACTORY(feature))
        return FALSE;
    factory = GST_ELEMENT_FACTORY(feature);
    if (!strstr(gst_element_factory_get_klass(factory), data->type))
        return FALSE;
    for (list = gst_element_factory_get_static_pad_templates(factory); list; list = list->next) {
        GstStaticPadTemplate *pad = (GstStaticPadTemplate*)list->data;
        GstCaps *caps;
        gboolean ret;
        if (pad->direction != GST_PAD_SINK)
            continue;
        caps = gst_static_caps_get(&pad->static_caps);
        ret = gst_caps_is_always_compatible(caps, data->caps);
        gst_caps_unref(caps);
        if (ret)
            return TRUE;
    }
    return FALSE;
}

static const char *Gstreamer_FindMatch(const char *strcaps)
{
    struct typeinfo data;
    GList *list, *copy;
    guint bestrank = 0;
    GstElementFactory *bestfactory = NULL;
    GstCaps *caps = gst_caps_from_string(strcaps);

    TRACE("%s\n", strcaps);

    data.caps = caps;
    data.type = "Decoder";
    copy = gst_registry_feature_filter(gst_registry_get(), match_element, 0, &data);
    for (list = copy; list; list = list->next) {
        GstElementFactory *factory = (GstElementFactory*)list->data;
        guint rank;
        rank = gst_plugin_feature_get_rank(GST_PLUGIN_FEATURE(factory));
        if (rank > bestrank || !bestrank) {
            bestrank = rank;
            bestfactory = factory;
        }
    }
    gst_caps_unref(caps);
    g_list_free(copy);

    if (!bestfactory) {
        ERR("Could not find plugin for %s\n", strcaps);
        return NULL;
    }
    return gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(bestfactory));
}

typedef struct GstTfImpl {
    TransformFilter tf;
    const char *gstreamer_name;
    GstElement *filter;
    GstPad *my_src, *my_sink, *their_src, *their_sink;
    LONG cbBuffer;
} GstTfImpl;

static HRESULT WINAPI Gstreamer_transform_ProcessBegin(TransformFilter *iface)
{
    GstTfImpl *This = (GstTfImpl*)iface;
    int ret;

    mark_wine_thread();

    ret = gst_element_set_state(This->filter, GST_STATE_PLAYING);
    TRACE("Returned: %i\n", ret);
    return S_OK;
}

static HRESULT WINAPI Gstreamer_transform_DecideBufferSize(TransformFilter *tf, IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest)
{
    GstTfImpl *This = (GstTfImpl*)tf;
    ALLOCATOR_PROPERTIES actual;

    TRACE("%p, %p, %p\n", This, pAlloc, ppropInputRequest);

    if (!ppropInputRequest->cbAlign)
        ppropInputRequest->cbAlign = 1;

    ppropInputRequest->cbBuffer = This->cbBuffer;

    if (ppropInputRequest->cBuffers < 2)
        ppropInputRequest->cBuffers = 2;

    return IMemAllocator_SetProperties(pAlloc, ppropInputRequest, &actual);
}

GstFlowReturn got_data(GstPad *pad, GstObject *parent, GstBuffer *buf)
{
    GstTfImpl *This = gst_pad_get_element_private(pad);
    IMediaSample *sample = (IMediaSample *) gst_mini_object_get_qdata(GST_MINI_OBJECT(buf), g_quark_from_static_string(media_quark_string));
    REFERENCE_TIME tStart, tStop;
    HRESULT hr;

    TRACE("%p, %p\n", pad, buf);

    if(!sample){
        GstMapInfo info;
        BYTE *ptr;

        gst_buffer_map(buf, &info, GST_MAP_READ);

        hr = BaseOutputPinImpl_GetDeliveryBuffer(&This->tf.source, &sample, NULL, NULL, 0);
        if (FAILED(hr)) {
            ERR("Could not get output buffer: %08x\n", hr);
            return GST_FLOW_FLUSHING;
        }

        IMediaSample_SetActualDataLength(sample, info.size);

        IMediaSample_GetPointer(sample, &ptr);

        memcpy(ptr, info.data, info.size);

        gst_buffer_unmap(buf, &info);
    }

    if (GST_BUFFER_PTS_IS_VALID(buf) &&
        GST_BUFFER_DURATION_IS_VALID(buf)) {
        tStart = buf->pts / 100;
        tStop = tStart + buf->duration / 100;
        IMediaSample_SetTime(sample, &tStart, &tStop);
    }
    else
        IMediaSample_SetTime(sample, NULL, NULL);
    if (GST_BUFFER_OFFSET_IS_VALID(buf) &&
        GST_BUFFER_OFFSET_END_IS_VALID(buf)) {
        tStart = buf->offset / 100;
        tStop = buf->offset_end / 100;
        IMediaSample_SetMediaTime(sample, &tStart, &tStop);
    }
    else
        IMediaSample_SetMediaTime(sample, NULL, NULL);

    IMediaSample_SetDiscontinuity(sample, GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_DISCONT));
    IMediaSample_SetPreroll(sample, GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_LIVE));
    IMediaSample_SetSyncPoint(sample, !GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_DELTA_UNIT));
    IMediaSample_SetActualDataLength(sample, gst_buffer_get_size(buf));

    hr = BaseOutputPinImpl_Deliver(&This->tf.source, sample);
    IMediaSample_Release(sample);
    gst_buffer_unref(buf);
    if (FAILED(hr))
        return GST_FLOW_FLUSHING;
    return GST_FLOW_OK;
}

static HRESULT WINAPI Gstreamer_transform_ProcessData(TransformFilter *iface, IMediaSample *sample)
{
    GstTfImpl *This = (GstTfImpl*)iface;
    REFERENCE_TIME tStart, tStop;
    BYTE *data;
    GstBuffer *buf;
    HRESULT hr;
    DWORD bufsize;
    int ret;

    TRACE("%p, %p\n", This, sample);

    mark_wine_thread();

    EnterCriticalSection(&This->tf.csReceive);
    IMediaSample_GetPointer(sample, &data);

    IMediaSample_AddRef(sample);
    bufsize = IMediaSample_GetActualDataLength(sample);
    buf = gst_buffer_new_wrapped_full(0, data, bufsize, 0, bufsize, sample, release_sample_wrapper);
    if (!buf) {
        IMediaSample_Release(sample);
        LeaveCriticalSection(&This->tf.csReceive);
        return S_OK;
    }

    IMediaSample_AddRef(sample);
    gst_mini_object_set_qdata(GST_MINI_OBJECT(buf), g_quark_from_static_string(media_quark_string), sample, release_sample_wrapper);

    buf->duration = buf->pts = -1;
    hr = IMediaSample_GetTime(sample, &tStart, &tStop);
    if (SUCCEEDED(hr)) {
        buf->pts = tStart * 100;
        if (hr == S_OK)
            buf->duration = (tStop - tStart)*100;
    }
    if (IMediaSample_GetMediaTime(sample, &tStart, &tStop) == S_OK) {
        buf->offset = tStart * 100;
        buf->offset_end = tStop * 100;
    }
    if (IMediaSample_IsDiscontinuity(sample) == S_OK)
        GST_BUFFER_FLAG_SET(buf, GST_BUFFER_FLAG_DISCONT);
    if (IMediaSample_IsPreroll(sample) == S_OK)
        GST_BUFFER_FLAG_SET(buf, GST_BUFFER_FLAG_LIVE);
    if (IMediaSample_IsSyncPoint(sample) != S_OK)
        GST_BUFFER_FLAG_SET(buf, GST_BUFFER_FLAG_DELTA_UNIT);
    LeaveCriticalSection(&This->tf.csReceive);
    ret = gst_pad_push(This->my_src, buf);
    if (ret)
        WARN("Sending returned: %i\n", ret);
    if (ret == GST_FLOW_FLUSHING)
        return VFW_E_WRONG_STATE;
    return S_OK;
}

static HRESULT WINAPI Gstreamer_transform_ProcessEnd(TransformFilter *iface)
{
    GstTfImpl *This = (GstTfImpl*)iface;
    int ret;

    mark_wine_thread();

    LeaveCriticalSection(&This->tf.csReceive);
    ret = gst_element_set_state(This->filter, GST_STATE_READY);
    EnterCriticalSection(&This->tf.csReceive);
    TRACE("Returned: %i\n", ret);
    return S_OK;
}

void Gstreamer_transform_pad_added(GstElement *filter, GstPad *pad, gpointer user)
{
    GstTfImpl *This = (GstTfImpl*)user;
    int ret;

    TRACE("%p %p %p\n", This, filter, pad);

    if (!GST_PAD_IS_SRC(pad))
        return;

    ret = gst_pad_link(pad, This->my_sink);
    if (ret < 0)
        WARN("Failed to link with %i\n", ret);
    This->their_src = pad;
}

static HRESULT Gstreamer_transform_ConnectInput(GstTfImpl *This, const AM_MEDIA_TYPE *amt, GstCaps *capsin, GstCaps *capsout)
{
    GstIterator *it;
    BOOL done = FALSE, found = FALSE;
    int ret;

    TRACE("%p %p %p %p\n", This, amt, capsin, capsout);

    mark_wine_thread();

    This->filter = gst_element_factory_make(This->gstreamer_name, NULL);
    if (!This->filter) {
        ERR("Could not make %s filter\n", This->gstreamer_name);
        return E_FAIL;
    }
    This->my_src = gst_pad_new("yuvsrc", GST_PAD_SRC);
    gst_pad_set_element_private (This->my_src, This);
    gst_pad_set_active(This->my_src, 1);

    This->my_sink = gst_pad_new("yuvsink", GST_PAD_SINK);
    gst_pad_set_chain_function(This->my_sink, got_data_wrapper);
    gst_pad_set_element_private (This->my_sink, This);
    gst_pad_set_active(This->my_sink, 1);

    it = gst_element_iterate_sink_pads(This->filter);
    while (!done) {
        GValue item = {0};

        switch (gst_iterator_next(it, &item)) {
        case GST_ITERATOR_RESYNC:
            gst_iterator_resync (it);
            break;
        case GST_ITERATOR_OK:
            This->their_sink = g_value_get_object(&item);
            gst_object_ref(This->their_sink);
            g_value_reset(&item);
        case GST_ITERATOR_ERROR:
        case GST_ITERATOR_DONE:
            done = TRUE;
            break;
        }
    }
    gst_iterator_free(it);
    if (!This->their_sink) {
        ERR("Could not find sink on filter %s\n", This->gstreamer_name);
        return E_FAIL;
    }

    it = gst_element_iterate_src_pads(This->filter);
    gst_iterator_resync(it);
    done = FALSE;
    while (!done) {
        GValue item = {0};

        switch (gst_iterator_next(it, &item)) {
        case GST_ITERATOR_RESYNC:
            gst_iterator_resync (it);
            break;
        case GST_ITERATOR_OK:
            This->their_src = g_value_get_object(&item);
            gst_object_ref(This->their_src);
            g_value_reset(&item);
        case GST_ITERATOR_ERROR:
        case GST_ITERATOR_DONE:
            done = TRUE;
            break;
        }
    }
    gst_iterator_free(it);
    found = !!This->their_src;
    if (!found)
        g_signal_connect(This->filter, "pad-added", G_CALLBACK(Gstreamer_transform_pad_added_wrapper), This);
    ret = gst_pad_link(This->my_src, This->their_sink);
    if (ret < 0) {
        WARN("Failed to link with %i\n", ret);
        return E_FAIL;
    }

    ret = gst_pad_set_caps(This->my_src, capsin);
    if (ret < 0) {
        WARN("Failed to set caps on own source with %i\n", ret);
        return E_FAIL;
    }

    if (found)
        Gstreamer_transform_pad_added(This->filter, This->their_src, This);

    if (!gst_pad_is_linked(This->my_sink))
        return E_FAIL;

    ret = gst_pad_set_caps(This->my_sink, capsout);
    if (ret < 0) {
        WARN("Failed to set caps on own sink with %i\n", ret);
        return E_FAIL;
    }

    TRACE("Connected\n");
    return S_OK;
}

static HRESULT WINAPI Gstreamer_transform_Cleanup(TransformFilter *tf, PIN_DIRECTION dir)
{
    GstTfImpl *This = (GstTfImpl*)tf;

    TRACE("%p 0x%x\n", This, dir);

    mark_wine_thread();

    if (dir == PINDIR_INPUT)
    {
        if (This->filter) {
            gst_element_set_state(This->filter, GST_STATE_NULL);
            gst_object_unref(This->filter);
        }
        This->filter = NULL;
        if (This->my_src) {
            gst_pad_unlink(This->my_src, This->their_sink);
            gst_object_unref(This->my_src);
            gst_object_unref(This->their_sink);
        }
        if (This->my_sink) {
            gst_pad_unlink(This->their_src, This->my_sink);
            gst_object_unref(This->my_sink);
            gst_object_unref(This->their_src);
        }
        This->my_sink = This->my_src = This->their_sink = This->their_src = NULL;
    }
    return S_OK;
}

static HRESULT WINAPI Gstreamer_transform_EndOfStream(TransformFilter *iface)
{
    GstTfImpl *This = (GstTfImpl*)iface;
    TRACE("%p\n", This);
    mark_wine_thread();

    gst_pad_push_event(This->my_src, gst_event_new_eos());
    return S_OK;
}

static HRESULT WINAPI Gstreamer_transform_BeginFlush(TransformFilter *iface)
{
    GstTfImpl *This = (GstTfImpl*)iface;
    TRACE("%p\n", This);
    mark_wine_thread();

    gst_pad_push_event(This->my_src, gst_event_new_flush_start());
    return S_OK;
}

static HRESULT WINAPI Gstreamer_transform_EndFlush(TransformFilter *iface)
{
    GstTfImpl *This = (GstTfImpl*)iface;
    TRACE("%p\n", This);
    mark_wine_thread();

    gst_pad_push_event(This->my_src, gst_event_new_flush_stop(TRUE));
    return S_OK;
}

static HRESULT WINAPI Gstreamer_transform_NewSegment(TransformFilter *iface, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    GstTfImpl *This = (GstTfImpl*)iface;
    const GstSegment segment = { GST_SEGMENT_FLAG_NONE, 1.0, dRate, GST_FORMAT_TIME, 0, 0, 0, tStop <= tStart ? -1 : tStop * 100, 0, tStart*100, -1 };

    TRACE("%p\n", This);
    mark_wine_thread();

    gst_pad_push_event(This->my_src, gst_event_new_segment(&segment));

    return S_OK;
}

static HRESULT WINAPI Gstreamer_transform_QOS(TransformFilter *iface, IBaseFilter *sender, Quality qm)
{
    GstTfImpl *This = (GstTfImpl*)iface;
    REFERENCE_TIME late = qm.Late;

    TRACE("%p %p { 0x%x %u %s %s }\n", This, sender,
            qm.Type, qm.Proportion,
            wine_dbgstr_longlong(qm.Late),
            wine_dbgstr_longlong(qm.TimeStamp));

    mark_wine_thread();

    if (qm.Late < 0 && -qm.Late > qm.TimeStamp)
        late = -qm.TimeStamp;
    gst_pad_push_event(This->my_sink, gst_event_new_qos(late <= 0 ? GST_QOS_TYPE_OVERFLOW : GST_QOS_TYPE_UNDERFLOW, 1000. / qm.Proportion, late * 100, qm.TimeStamp * 100));
    return TransformFilterImpl_Notify(iface, sender, qm);
}

static HRESULT Gstreamer_transform_create(IUnknown *outer, const CLSID *clsid,
        const char *name, const TransformFilterFuncTable *vtbl, void **obj)
{
    GstTfImpl *This;

    if (FAILED(strmbase_transform_create(sizeof(GstTfImpl), outer, clsid, vtbl, (IBaseFilter **)&This)))
        return E_OUTOFMEMORY;

    This->gstreamer_name = name;
    *obj = &This->tf.filter.IUnknown_inner;

    TRACE("returning %p\n", This);

    return S_OK;
}

static HRESULT WINAPI Gstreamer_Mp3_QueryConnect(TransformFilter *iface, const AM_MEDIA_TYPE *amt)
{
    GstTfImpl *This = (GstTfImpl*)iface;
    TRACE("%p %p\n", This, amt);
    dump_AM_MEDIA_TYPE(amt);

    if ( (!IsEqualGUID(&amt->majortype, &MEDIATYPE_Audio) &&
          !IsEqualGUID(&amt->majortype, &MEDIATYPE_Stream)) ||
         (!IsEqualGUID(&amt->subtype, &MEDIASUBTYPE_MPEG1AudioPayload) &&
          !IsEqualGUID(&amt->subtype, &WMMEDIASUBTYPE_MP3))
        || !IsEqualGUID(&amt->formattype, &FORMAT_WaveFormatEx)){
        return S_FALSE;
    }

    return S_OK;
}

static HRESULT WINAPI Gstreamer_Mp3_SetMediaType(TransformFilter *tf, PIN_DIRECTION dir, const AM_MEDIA_TYPE *amt)
{
    GstTfImpl *This = (GstTfImpl*)tf;
    GstCaps *capsin, *capsout;
    AM_MEDIA_TYPE *outpmt = &This->tf.pmt;
    WAVEFORMATEX *wfx, *wfxin;
    HRESULT hr;
    int layer;

    TRACE("%p 0x%x %p\n", This, dir, amt);

    mark_wine_thread();

    if (dir != PINDIR_INPUT)
        return S_OK;

    if (Gstreamer_Mp3_QueryConnect(&This->tf, amt) == S_FALSE || !amt->pbFormat)
        return VFW_E_TYPE_NOT_ACCEPTED;

    wfxin = (WAVEFORMATEX*)amt->pbFormat;
    switch (wfxin->wFormatTag) {
        case WAVE_FORMAT_MPEGLAYER3:
            layer = 3;
            break;
        case WAVE_FORMAT_MPEG: {
            MPEG1WAVEFORMAT *mpgformat = (MPEG1WAVEFORMAT*)wfxin;
            layer = mpgformat->fwHeadLayer;
            break;
        }
        default:
            FIXME("Unhandled tag %x\n", wfxin->wFormatTag);
            return E_FAIL;
    }

    FreeMediaType(outpmt);
    CopyMediaType(outpmt, amt);

    outpmt->subtype = MEDIASUBTYPE_PCM;
    outpmt->formattype = FORMAT_WaveFormatEx;
    outpmt->cbFormat = sizeof(*wfx);
    CoTaskMemFree(outpmt->pbFormat);
    wfx = CoTaskMemAlloc(outpmt->cbFormat);
    outpmt->pbFormat = (BYTE*)wfx;
    wfx->wFormatTag = WAVE_FORMAT_PCM;
    wfx->wBitsPerSample = 16;
    wfx->nSamplesPerSec = wfxin->nSamplesPerSec;
    wfx->nChannels = wfxin->nChannels;
    wfx->nBlockAlign = wfx->wBitsPerSample * wfx->nChannels / 8;
    wfx->cbSize = 0;
    wfx->nAvgBytesPerSec = wfx->nSamplesPerSec * wfx->nBlockAlign;

    capsin = gst_caps_new_simple("audio/mpeg",
                                 "mpegversion", G_TYPE_INT, 1,
                                 "layer", G_TYPE_INT, layer,
                                 "rate", G_TYPE_INT, wfx->nSamplesPerSec,
                                 "channels", G_TYPE_INT, wfx->nChannels,
                                 NULL);
    capsout = gst_caps_new_simple("audio/x-raw",
                                  "format", G_TYPE_STRING, "S16LE",
                                  "rate", G_TYPE_INT, wfx->nSamplesPerSec,
                                  "channels", G_TYPE_INT, wfx->nChannels,
                                   NULL);

    hr = Gstreamer_transform_ConnectInput(This, amt, capsin, capsout);
    gst_caps_unref(capsin);
    gst_caps_unref(capsout);

    This->cbBuffer = wfx->nAvgBytesPerSec / 4;

    return hr;
}

static HRESULT WINAPI Gstreamer_Mp3_ConnectInput(TransformFilter *tf, PIN_DIRECTION dir, IPin *pin)
{
    TRACE("%p 0x%x %p\n", tf, dir, pin);
    return S_OK;
}

static const TransformFilterFuncTable Gstreamer_Mp3_vtbl = {
    Gstreamer_transform_DecideBufferSize,
    Gstreamer_transform_ProcessBegin,
    Gstreamer_transform_ProcessData,
    Gstreamer_transform_ProcessEnd,
    Gstreamer_Mp3_QueryConnect,
    Gstreamer_Mp3_SetMediaType,
    Gstreamer_Mp3_ConnectInput,
    Gstreamer_transform_Cleanup,
    Gstreamer_transform_EndOfStream,
    Gstreamer_transform_BeginFlush,
    Gstreamer_transform_EndFlush,
    Gstreamer_transform_NewSegment,
    Gstreamer_transform_QOS
};

IUnknown * CALLBACK Gstreamer_Mp3_create(IUnknown *punkouter, HRESULT *phr)
{
    const char *plugin;
    IUnknown *obj = NULL;

    TRACE("%p %p\n", punkouter, phr);

    if (!init_gstreamer())
    {
        *phr = E_FAIL;
        return NULL;
    }

    mark_wine_thread();

    plugin = Gstreamer_FindMatch("audio/mpeg, mpegversion=(int) 1");
    if (!plugin)
    {
        *phr = E_FAIL;
        return NULL;
    }

    *phr = Gstreamer_transform_create(punkouter, &CLSID_Gstreamer_Mp3, plugin, &Gstreamer_Mp3_vtbl, (LPVOID*)&obj);

    TRACE("returning %p\n", obj);

    return obj;
}

static HRESULT WINAPI Gstreamer_YUV_QueryConnect(TransformFilter *iface, const AM_MEDIA_TYPE *amt)
{
    GstTfImpl *This = (GstTfImpl*)iface;
    TRACE("%p %p\n", This, amt);
    dump_AM_MEDIA_TYPE(amt);

    if (!IsEqualGUID(&amt->majortype, &MEDIATYPE_Video) ||
        (!IsEqualGUID(&amt->formattype, &FORMAT_VideoInfo) &&
         !IsEqualGUID(&amt->formattype, &FORMAT_VideoInfo2)))
        return S_FALSE;
    if (memcmp(&amt->subtype.Data2, &MEDIATYPE_Video.Data2, sizeof(GUID) - sizeof(amt->subtype.Data1)))
        return S_FALSE;
    switch (amt->subtype.Data1) {
        case mmioFOURCC('I','4','2','0'):
        case mmioFOURCC('Y','V','1','2'):
        case mmioFOURCC('N','V','1','2'):
        case mmioFOURCC('N','V','2','1'):
        case mmioFOURCC('Y','U','Y','2'):
        case mmioFOURCC('Y','V','Y','U'):
            return S_OK;
        default:
            WARN("Unhandled fourcc %s\n", debugstr_an((char*)&amt->subtype.Data1, 4));
            return S_FALSE;
    }
}

static HRESULT WINAPI Gstreamer_YUV_ConnectInput(TransformFilter *tf, PIN_DIRECTION dir, IPin *pin)
{
    TRACE("%p 0x%x %p\n", tf, dir, pin);
    return S_OK;
}

static HRESULT WINAPI Gstreamer_YUV2RGB_SetMediaType(TransformFilter *tf, PIN_DIRECTION dir,  const AM_MEDIA_TYPE *amt)
{
    GstTfImpl *This = (GstTfImpl*)tf;
    GstCaps *capsin, *capsout;
    AM_MEDIA_TYPE *outpmt = &This->tf.pmt;
    HRESULT hr;
    int avgtime;
    LONG width, height;

    TRACE("%p 0x%x %p\n", This, dir, amt);

    mark_wine_thread();

    if (dir != PINDIR_INPUT)
        return S_OK;

    if (Gstreamer_YUV_QueryConnect(&This->tf, amt) == S_FALSE || !amt->pbFormat)
        return E_FAIL;

    FreeMediaType(outpmt);
    CopyMediaType(outpmt, amt);

    if (IsEqualGUID(&amt->formattype, &FORMAT_VideoInfo)) {
        VIDEOINFOHEADER *vih = (VIDEOINFOHEADER*)outpmt->pbFormat;
        avgtime = vih->AvgTimePerFrame;
        width = vih->bmiHeader.biWidth;
        height = vih->bmiHeader.biHeight;
        vih->bmiHeader.biBitCount = 24;
        vih->bmiHeader.biCompression = BI_RGB;
        vih->bmiHeader.biSizeImage = width * abs(height) * 3;
    } else {
        VIDEOINFOHEADER2 *vih = (VIDEOINFOHEADER2*)outpmt->pbFormat;
        avgtime = vih->AvgTimePerFrame;
        width = vih->bmiHeader.biWidth;
        height = vih->bmiHeader.biHeight;
        vih->bmiHeader.biBitCount = 24;
        vih->bmiHeader.biCompression = BI_RGB;
        vih->bmiHeader.biSizeImage = width * abs(height) * 3;
    }
    if (!avgtime)
        avgtime = 10000000 / 30;

    outpmt->subtype = MEDIASUBTYPE_RGB24;

    capsin = gst_caps_new_simple("video/x-raw",
                                 "format", G_TYPE_STRING,
                                   gst_video_format_to_string(
                                     gst_video_format_from_fourcc(amt->subtype.Data1)),
                                 "width", G_TYPE_INT, width,
                                 "height", G_TYPE_INT, height,
                                 "framerate", GST_TYPE_FRACTION, 10000000, avgtime,
                                 NULL);
    capsout = gst_caps_new_simple("video/x-raw",
                                  "format", G_TYPE_STRING, "BGR",
                                  "width", G_TYPE_INT, width,
                                  "height", G_TYPE_INT, height,
                                  "framerate", GST_TYPE_FRACTION, 10000000, avgtime,
                                   NULL);

    hr = Gstreamer_transform_ConnectInput(This, amt, capsin, capsout);
    gst_caps_unref(capsin);
    gst_caps_unref(capsout);

    This->cbBuffer = width * height * 4;
    return hr;
}

static const TransformFilterFuncTable Gstreamer_YUV2RGB_vtbl = {
    Gstreamer_transform_DecideBufferSize,
    Gstreamer_transform_ProcessBegin,
    Gstreamer_transform_ProcessData,
    Gstreamer_transform_ProcessEnd,
    Gstreamer_YUV_QueryConnect,
    Gstreamer_YUV2RGB_SetMediaType,
    Gstreamer_YUV_ConnectInput,
    Gstreamer_transform_Cleanup,
    Gstreamer_transform_EndOfStream,
    Gstreamer_transform_BeginFlush,
    Gstreamer_transform_EndFlush,
    Gstreamer_transform_NewSegment,
    Gstreamer_transform_QOS
};

IUnknown * CALLBACK Gstreamer_YUV2RGB_create(IUnknown *punkouter, HRESULT *phr)
{
    IUnknown *obj = NULL;

    TRACE("%p %p\n", punkouter, phr);

    if (!init_gstreamer())
    {
        *phr = E_FAIL;
        return NULL;
    }

    *phr = Gstreamer_transform_create(punkouter, &CLSID_Gstreamer_YUV2RGB, "videoconvert", &Gstreamer_YUV2RGB_vtbl, (LPVOID*)&obj);

    TRACE("returning %p\n", obj);

    return obj;
}

static HRESULT WINAPI Gstreamer_YUV2ARGB_SetMediaType(TransformFilter *tf, PIN_DIRECTION dir,  const AM_MEDIA_TYPE *amt)
{
    GstTfImpl *This = (GstTfImpl*)tf;
    GstCaps *capsin, *capsout;
    AM_MEDIA_TYPE *outpmt = &This->tf.pmt;
    HRESULT hr;
    int avgtime;
    LONG width, height;

    TRACE("%p 0x%x %p\n", This, dir, amt);

    mark_wine_thread();

    if (dir != PINDIR_INPUT)
        return S_OK;

    if (Gstreamer_YUV_QueryConnect(&This->tf, amt) == S_FALSE || !amt->pbFormat)
        return E_FAIL;

    FreeMediaType(outpmt);
    CopyMediaType(outpmt, amt);

    if (IsEqualGUID(&amt->formattype, &FORMAT_VideoInfo)) {
        VIDEOINFOHEADER *vih = (VIDEOINFOHEADER*)outpmt->pbFormat;
        avgtime = vih->AvgTimePerFrame;
        width = vih->bmiHeader.biWidth;
        height = vih->bmiHeader.biHeight;
        vih->bmiHeader.biBitCount = 32;
        vih->bmiHeader.biCompression = BI_RGB;
        vih->bmiHeader.biSizeImage = width * abs(height) * 3;
    } else {
        VIDEOINFOHEADER2 *vih = (VIDEOINFOHEADER2*)outpmt->pbFormat;
        avgtime = vih->AvgTimePerFrame;
        width = vih->bmiHeader.biWidth;
        height = vih->bmiHeader.biHeight;
        vih->bmiHeader.biBitCount = 32;
        vih->bmiHeader.biCompression = BI_RGB;
        vih->bmiHeader.biSizeImage = width * abs(height) * 3;
    }
    if (!avgtime)
        avgtime = 10000000 / 30;

    outpmt->subtype = MEDIASUBTYPE_ARGB32;

    capsin = gst_caps_new_simple("video/x-raw",
                                 "format", G_TYPE_STRING,
                                   gst_video_format_to_string(
                                     gst_video_format_from_fourcc(amt->subtype.Data1)),
                                 "width", G_TYPE_INT, width,
                                 "height", G_TYPE_INT, height,
                                 "framerate", GST_TYPE_FRACTION, 10000000, avgtime,
                                 NULL);
    capsout = gst_caps_new_simple("video/x-raw",
                                  "format", G_TYPE_STRING, "BGRA",
                                  "width", G_TYPE_INT, width,
                                  "height", G_TYPE_INT, height,
                                  "framerate", GST_TYPE_FRACTION, 10000000, avgtime,
                                   NULL);

    hr = Gstreamer_transform_ConnectInput(This, amt, capsin, capsout);
    gst_caps_unref(capsin);
    gst_caps_unref(capsout);

    This->cbBuffer = width * height * 4;
    return hr;
}

static const TransformFilterFuncTable Gstreamer_YUV2ARGB_vtbl = {
    Gstreamer_transform_DecideBufferSize,
    Gstreamer_transform_ProcessBegin,
    Gstreamer_transform_ProcessData,
    Gstreamer_transform_ProcessEnd,
    Gstreamer_YUV_QueryConnect,
    Gstreamer_YUV2ARGB_SetMediaType,
    Gstreamer_YUV_ConnectInput,
    Gstreamer_transform_Cleanup,
    Gstreamer_transform_EndOfStream,
    Gstreamer_transform_BeginFlush,
    Gstreamer_transform_EndFlush,
    Gstreamer_transform_NewSegment,
    Gstreamer_transform_QOS
};

IUnknown * CALLBACK Gstreamer_YUV2ARGB_create(IUnknown *punkouter, HRESULT *phr)
{
    IUnknown *obj = NULL;

    TRACE("%p %p\n", punkouter, phr);

    if (!init_gstreamer())
    {
        *phr = E_FAIL;
        return NULL;
    }

    *phr = Gstreamer_transform_create(punkouter, &CLSID_Gstreamer_YUV2ARGB, "videoconvert", &Gstreamer_YUV2ARGB_vtbl, (LPVOID*)&obj);

    TRACE("returning %p\n", obj);

    return obj;
}

static HRESULT WINAPI Gstreamer_AudioConvert_QueryConnect(TransformFilter *iface, const AM_MEDIA_TYPE *amt)
{
    GstTfImpl *This = (GstTfImpl*)iface;
    TRACE("%p %p\n", This, amt);
    dump_AM_MEDIA_TYPE(amt);

    if (!IsEqualGUID(&amt->majortype, &MEDIATYPE_Audio) ||
        !IsEqualGUID(&amt->subtype, &MEDIASUBTYPE_PCM) ||
        !IsEqualGUID(&amt->formattype, &FORMAT_WaveFormatEx))
        return S_FALSE;
    return S_OK;
}

static HRESULT WINAPI Gstreamer_AudioConvert_ConnectInput(TransformFilter *tf, PIN_DIRECTION dir, IPin *pin)
{
    TRACE("%p 0x%x %p\n", tf, dir, pin);
    return S_OK;
}

static HRESULT WINAPI Gstreamer_AudioConvert_SetMediaType(TransformFilter *tf, PIN_DIRECTION dir, const AM_MEDIA_TYPE *amt)
{
    GstTfImpl *This = (GstTfImpl*)tf;
    GstCaps *capsin, *capsout;
    AM_MEDIA_TYPE *outpmt = &This->tf.pmt;
    WAVEFORMATEX *inwfe;
    WAVEFORMATEX *outwfe;
    WAVEFORMATEXTENSIBLE *outwfx;
    GstAudioFormat format;
    HRESULT hr;
    BOOL inisfloat = FALSE;
    int indepth;

    TRACE("%p 0x%x %p\n", This, dir, amt);

    mark_wine_thread();

    if (dir != PINDIR_INPUT)
        return S_OK;

    if (Gstreamer_AudioConvert_QueryConnect(&This->tf, amt) == S_FALSE || !amt->pbFormat)
        return E_FAIL;

    FreeMediaType(outpmt);
    *outpmt = *amt;
    outpmt->pUnk = NULL;
    outpmt->cbFormat = sizeof(WAVEFORMATEXTENSIBLE);
    outpmt->pbFormat = CoTaskMemAlloc(outpmt->cbFormat);

    inwfe = (WAVEFORMATEX*)amt->pbFormat;
    indepth = inwfe->wBitsPerSample;
    if (inwfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        WAVEFORMATEXTENSIBLE *inwfx = (WAVEFORMATEXTENSIBLE*)inwfe;
        inisfloat = IsEqualGUID(&inwfx->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);
        if (inwfx->Samples.wValidBitsPerSample)
            indepth = inwfx->Samples.wValidBitsPerSample;
    } else if (inwfe->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
        inisfloat = TRUE;

    if (inisfloat)
        format = inwfe->wBitsPerSample == 64 ? GST_AUDIO_FORMAT_F64LE : GST_AUDIO_FORMAT_F32LE;
    else
        format = gst_audio_format_build_integer(inwfe->wBitsPerSample != 8, G_LITTLE_ENDIAN,
                inwfe->wBitsPerSample, indepth);

    capsin = gst_caps_new_simple("audio/x-raw",
                                 "format", G_TYPE_STRING, gst_audio_format_to_string(format),
                                 "channels", G_TYPE_INT, inwfe->nChannels,
                                 "rate", G_TYPE_INT, inwfe->nSamplesPerSec,
                                  NULL);

    outwfe = (WAVEFORMATEX*)outpmt->pbFormat;
    outwfx = (WAVEFORMATEXTENSIBLE*)outwfe;
    outwfe->wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    outwfe->nChannels = 2;
    outwfe->nSamplesPerSec = inwfe->nSamplesPerSec;
    outwfe->wBitsPerSample = 16;
    outwfe->nBlockAlign = outwfe->nChannels * outwfe->wBitsPerSample / 8;
    outwfe->nAvgBytesPerSec = outwfe->nBlockAlign * outwfe->nSamplesPerSec;
    outwfe->cbSize = sizeof(*outwfx) - sizeof(*outwfe);
    outwfx->Samples.wValidBitsPerSample = outwfe->wBitsPerSample;
    outwfx->dwChannelMask = SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT;
    outwfx->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

    capsout = gst_caps_new_simple("audio/x-raw",
                                  "format", G_TYPE_STRING, "S16LE",
                                  "channels", G_TYPE_INT, outwfe->nChannels,
                                  "rate", G_TYPE_INT, outwfe->nSamplesPerSec,
                                   NULL);


    hr = Gstreamer_transform_ConnectInput(This, amt, capsin, capsout);
    gst_caps_unref(capsin);
    gst_caps_unref(capsout);

    This->cbBuffer = inwfe->nAvgBytesPerSec;
    return hr;
}

static const TransformFilterFuncTable Gstreamer_AudioConvert_vtbl = {
    Gstreamer_transform_DecideBufferSize,
    Gstreamer_transform_ProcessBegin,
    Gstreamer_transform_ProcessData,
    Gstreamer_transform_ProcessEnd,
    Gstreamer_AudioConvert_QueryConnect,
    Gstreamer_AudioConvert_SetMediaType,
    Gstreamer_AudioConvert_ConnectInput,
    Gstreamer_transform_Cleanup,
    Gstreamer_transform_EndOfStream,
    Gstreamer_transform_BeginFlush,
    Gstreamer_transform_EndFlush,
    Gstreamer_transform_NewSegment,
    Gstreamer_transform_QOS
};

IUnknown * CALLBACK Gstreamer_AudioConvert_create(IUnknown *punkouter, HRESULT *phr)
{
    IUnknown *obj = NULL;

    TRACE("%p %p\n", punkouter, phr);

    if (!init_gstreamer())
    {
        *phr = E_FAIL;
        return NULL;
    }

    *phr = Gstreamer_transform_create(punkouter, &CLSID_Gstreamer_AudioConvert, "audioconvert", &Gstreamer_AudioConvert_vtbl, (LPVOID*)&obj);

    TRACE("returning %p\n", obj);

    return obj;
}
