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

#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappbuffer.h>

#include "gst_private.h"
#include "gst_guids.h"

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

WINE_DEFAULT_DEBUG_CHANNEL(gstreamer);

struct typeinfo {
    GstCaps *caps;
    const char *type;
};

static const IBaseFilterVtbl GSTTf_Vtbl;

static gboolean match_element(GstPluginFeature *feature, gpointer gdata) {
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

    data.caps = caps;
    data.type = "Decoder";
    copy = gst_default_registry_feature_filter(match_element, 0, &data);
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
        FIXME("Could not find plugin for %s\n", strcaps);
        return NULL;
    }
    return gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(bestfactory));
}

typedef struct GstTfImpl {
    TransformFilter tf;
    IUnknown *seekthru_unk;
    const char *gstreamer_name;
    GstElement *filter;
    GstPad *my_src, *my_sink, *their_src, *their_sink;
    LONG cbBuffer;
} GstTfImpl;

static HRESULT WINAPI Gstreamer_transform_ProcessBegin(TransformFilter *iface) {
    GstTfImpl *This = (GstTfImpl*)iface;
    int ret;

    ret = gst_element_set_state(This->filter, GST_STATE_PLAYING);
    TRACE("Returned: %i\n", ret);
    return S_OK;
}

static HRESULT WINAPI Gstreamer_transform_DecideBufferSize(TransformFilter *tf, IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest)
{
    GstTfImpl *This = (GstTfImpl*)tf;
    ALLOCATOR_PROPERTIES actual;

    if (!ppropInputRequest->cbAlign)
        ppropInputRequest->cbAlign = 1;

    ppropInputRequest->cbBuffer = This->cbBuffer;

    if (!ppropInputRequest->cBuffers)
        ppropInputRequest->cBuffers = 1;

    return IMemAllocator_SetProperties(pAlloc, ppropInputRequest, &actual);
}

static void release_sample(void *data) {
    TRACE("Releasing %p\n", data);
    IMediaSample_Release((IMediaSample *)data);
}

static GstFlowReturn got_data(GstPad *pad, GstBuffer *buf) {
    GstTfImpl *This = gst_pad_get_element_private(pad);
    IMediaSample *sample = GST_APP_BUFFER(buf)->priv;
    REFERENCE_TIME tStart, tStop;
    HRESULT hr;

    if (GST_BUFFER_TIMESTAMP_IS_VALID(buf) &&
        GST_BUFFER_DURATION_IS_VALID(buf)) {
        tStart = buf->timestamp / 100;
        tStop = tStart + buf->duration / 100;
        IMediaSample_SetTime(sample, &tStart, &tStop);
    }
    else
        IMediaSample_SetTime(sample, NULL, NULL);

    IMediaSample_SetDiscontinuity(sample, GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_DISCONT));
    IMediaSample_SetPreroll(sample, GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_PREROLL));
    IMediaSample_SetSyncPoint(sample, !GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_DELTA_UNIT));

    hr = BaseOutputPinImpl_Deliver((BaseOutputPin*)This->tf.ppPins[1], sample);
    gst_buffer_unref(buf);
    if (FAILED(hr))
        return GST_FLOW_WRONG_STATE;
    if (hr != S_OK)
        return GST_FLOW_RESEND;
    return GST_FLOW_OK;
}

static GstFlowReturn request_buffer(GstPad *pad, guint64 ofs, guint size, GstCaps *caps, GstBuffer **buf) {
    GstTfImpl *This = gst_pad_get_element_private(pad);
    IMediaSample *sample;
    BYTE *ptr;
    HRESULT hr;
    TRACE("Requesting buffer\n");

    hr = BaseOutputPinImpl_GetDeliveryBuffer((BaseOutputPin*)This->tf.ppPins[1], &sample, NULL, NULL, 0);
    if (FAILED(hr)) {
        ERR("Could not get output buffer: %08x\n", hr);
        return GST_FLOW_WRONG_STATE;
    }
    IMediaSample_SetActualDataLength(sample, size);
    IMediaSample_GetPointer(sample, &ptr);
    *buf = gst_app_buffer_new(ptr, size, release_sample, sample);

    if (!*buf) {
        IMediaSample_Release(sample);
        ERR("Out of memory\n");
        return GST_FLOW_ERROR;
    }
    if (!caps)
        caps = gst_pad_get_caps_reffed(This->my_sink);
    gst_buffer_set_caps(*buf, caps);
    return GST_FLOW_OK;
}

static HRESULT WINAPI Gstreamer_transform_ProcessData(TransformFilter *iface, IMediaSample *sample) {
    GstTfImpl *This = (GstTfImpl*)iface;
    REFERENCE_TIME tStart, tStop;
    BYTE *data;
    GstBuffer *buf;
    HRESULT hr;
    int ret;
    TRACE("Reading %p\n", sample);

    EnterCriticalSection(&This->tf.filter.csFilter);
    IMediaSample_GetPointer(sample, &data);
    buf = gst_app_buffer_new(data, IMediaSample_GetActualDataLength(sample), release_sample, sample);
    if (!buf) {
        LeaveCriticalSection(&This->tf.filter.csFilter);
        return S_OK;
    }
    gst_buffer_set_caps(buf, gst_pad_get_caps_reffed(This->my_src));
    IMediaSample_AddRef(sample);
    buf->duration = buf->timestamp = -1;
    hr = IMediaSample_GetTime(sample, &tStart, &tStop);
    if (SUCCEEDED(hr)) {
        buf->timestamp = tStart * 100;
        if (hr == S_OK)
            buf->duration = (tStop - tStart)*100;
    }
    if (IMediaSample_IsDiscontinuity(sample) == S_OK)
        GST_BUFFER_FLAG_SET(buf, GST_BUFFER_FLAG_DISCONT);
    if (IMediaSample_IsPreroll(sample) == S_OK)
        GST_BUFFER_FLAG_SET(buf, GST_BUFFER_FLAG_PREROLL);
    if (IMediaSample_IsSyncPoint(sample) != S_OK)
        GST_BUFFER_FLAG_SET(buf, GST_BUFFER_FLAG_DELTA_UNIT);
    LeaveCriticalSection(&This->tf.filter.csFilter);
    ret = gst_pad_push(This->my_src, buf);
    if (ret)
        WARN("Sending returned: %i\n", ret);
    if (ret == GST_FLOW_ERROR)
        return E_FAIL;
    if (ret == GST_FLOW_WRONG_STATE)
        return VFW_E_WRONG_STATE;
    if (ret == GST_FLOW_RESEND)
        return S_FALSE;
    return S_OK;
}

static HRESULT WINAPI Gstreamer_transform_ProcessEnd(TransformFilter *iface) {
    GstTfImpl *This = (GstTfImpl*)iface;
    int ret;

    ret = gst_element_set_state(This->filter, GST_STATE_PAUSED);
    TRACE("Returned: %i\n", ret);
    return S_OK;
}

static void Gstreamer_transform_pad_added(GstElement *filter, GstPad *pad, GstTfImpl *This)
{
    int ret;
    if (!GST_PAD_IS_SRC(pad))
        return;

    ret = gst_pad_link(pad, This->my_sink);
    if (ret < 0)
        WARN("Failed to link with %i\n", ret);
    This->their_src = pad;

    gst_pad_set_active(pad, TRUE);
    gst_pad_set_active(This->my_sink, TRUE);
}

static HRESULT Gstreamer_transform_ConnectInput(GstTfImpl *This, const AM_MEDIA_TYPE *amt, GstCaps *capsin, GstCaps *capsout) {
    GstIterator *it;
    int done = 0, found = 0, ret;

    This->filter = gst_element_factory_make(This->gstreamer_name, NULL);
    if (!This->filter) {
        FIXME("Could not make %s filter\n", This->gstreamer_name);
        return E_FAIL;
    }
    This->my_src = gst_pad_new(NULL, GST_PAD_SRC);
    gst_pad_set_element_private (This->my_src, This);

    This->my_sink = gst_pad_new(NULL, GST_PAD_SINK);
    gst_pad_set_chain_function(This->my_sink, got_data);
    gst_pad_set_bufferalloc_function(This->my_sink, request_buffer);
    gst_pad_set_element_private (This->my_sink, This);

    ret = gst_pad_set_caps(This->my_src, capsin);
    if (ret < 0) {
        WARN("Failed to set caps on own source with %i\n", ret);
        return E_FAIL;
    }

    ret = gst_pad_set_caps(This->my_sink, capsout);
    if (ret < 0) {
        WARN("Failed to set caps on own sink with %i\n", ret);
        return E_FAIL;
    }

    it = gst_element_iterate_sink_pads(This->filter);
    while (!done) {
        gpointer item;

        switch (gst_iterator_next(it, &item)) {
        case GST_ITERATOR_RESYNC:
            gst_iterator_resync (it);
            break;
        case GST_ITERATOR_OK:
            This->their_sink = item;
        case GST_ITERATOR_ERROR:
        case GST_ITERATOR_DONE:
            done = 1;
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
    done = 0;
    while (!done) {
        gpointer item;

        switch (gst_iterator_next(it, &item)) {
        case GST_ITERATOR_RESYNC:
            gst_iterator_resync (it);
            break;
        case GST_ITERATOR_OK:
            This->their_src = item;
        case GST_ITERATOR_ERROR:
        case GST_ITERATOR_DONE:
            done = 1;
            break;
        }
    }
    gst_iterator_free(it);
    found = !!This->their_src;
    if (!found)
        g_signal_connect(This->filter, "pad-added", G_CALLBACK(Gstreamer_transform_pad_added), This);
    ret = gst_pad_link(This->my_src, This->their_sink);
    if (ret < 0) {
        WARN("Failed to link with %i\n", ret);
        return E_FAIL;
    }

    if (found)
        Gstreamer_transform_pad_added(This->filter, This->their_src, This);

    if (!gst_pad_is_linked(This->my_sink))
        return E_FAIL;

    TRACE("Connected\n");
    return S_OK;
}

static HRESULT WINAPI Gstreamer_transform_Cleanup(TransformFilter *tf, PIN_DIRECTION dir) {
    GstTfImpl *This = (GstTfImpl*)tf;

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
        }
        if (This->my_sink) {
            gst_pad_unlink(This->their_src, This->my_sink);
            gst_object_unref(This->my_sink);
        }
        This->my_sink = This->my_src = This->their_sink = This->their_src = NULL;
        FIXME("%p stub\n", This);
    }
    return S_OK;
}

static HRESULT WINAPI Gstreamer_transform_EndOfStream(TransformFilter *iface) {
    GstTfImpl *This = (GstTfImpl*)iface;
    TRACE("%p\n", This);

    gst_pad_push_event(This->my_src, gst_event_new_eos());
    return S_OK;
}

static HRESULT WINAPI Gstreamer_transform_BeginFlush(TransformFilter *iface) {
    GstTfImpl *This = (GstTfImpl*)iface;
    TRACE("%p\n", This);

    gst_pad_push_event(This->my_src, gst_event_new_flush_start());
    return S_OK;
}

static HRESULT WINAPI Gstreamer_transform_EndFlush(TransformFilter *iface) {
    GstTfImpl *This = (GstTfImpl*)iface;
    TRACE("%p\n", This);

    gst_pad_push_event(This->my_src, gst_event_new_flush_stop());
    return S_OK;
}

static HRESULT WINAPI Gstreamer_transform_NewSegment(TransformFilter *iface, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate) {
    GstTfImpl *This = (GstTfImpl*)iface;
    TRACE("%p\n", This);

    gst_pad_push_event(This->my_src, gst_event_new_new_segment_full(1,
                       1.0, dRate, GST_FORMAT_TIME, tStart*100, tStop <= tStart ? -1 : tStop * 100, tStart*100));
    return S_OK;
}

static HRESULT Gstreamer_transform_create(IUnknown *punkout, const CLSID *clsid, const char *name, const TransformFilterFuncTable *vtbl, void **obj)
{
    GstTfImpl *This;

    if (FAILED(TransformFilter_Construct(&GSTTf_Vtbl, sizeof(GstTfImpl), clsid, vtbl, (IBaseFilter**)&This)))
        return E_OUTOFMEMORY;
    else
    {
        ISeekingPassThru *passthru;
        CoCreateInstance(&CLSID_SeekingPassThru, (IUnknown*)This, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void**)&This->seekthru_unk);
        IUnknown_QueryInterface(This->seekthru_unk, &IID_ISeekingPassThru, (void**)&passthru);
        ISeekingPassThru_Init(passthru, FALSE, (IPin*)This->tf.ppPins[0]);
        ISeekingPassThru_Release(passthru);
    }

    This->gstreamer_name = name;
    *obj = This;

    return S_OK;
}

static HRESULT WINAPI Gstreamer_YUV_QueryConnect(TransformFilter *iface, const AM_MEDIA_TYPE *amt) {
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
    return S_OK;
}

static HRESULT WINAPI Gstreamer_YUV_SetMediaType(TransformFilter *tf, PIN_DIRECTION dir,  const AM_MEDIA_TYPE *amt) {
    GstTfImpl *This = (GstTfImpl*)tf;
    GstCaps *capsin, *capsout;
    AM_MEDIA_TYPE *outpmt = &This->tf.pmt;
    HRESULT hr;
    int avgtime;
    DWORD width, height;

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
        if ((LONG)vih->bmiHeader.biHeight > 0)
            vih->bmiHeader.biHeight = -vih->bmiHeader.biHeight;
        vih->bmiHeader.biBitCount = 24;
        vih->bmiHeader.biCompression = BI_RGB;
    } else {
        VIDEOINFOHEADER2 *vih = (VIDEOINFOHEADER2*)outpmt->pbFormat;
        avgtime = vih->AvgTimePerFrame;
        width = vih->bmiHeader.biWidth;
        height = vih->bmiHeader.biHeight;
        if ((LONG)vih->bmiHeader.biHeight > 0)
            vih->bmiHeader.biHeight = -vih->bmiHeader.biHeight;
        vih->bmiHeader.biBitCount = 24;
        vih->bmiHeader.biCompression = BI_RGB;
    }
    if (!avgtime)
        avgtime = 10000000 / 30;

    outpmt->subtype = MEDIASUBTYPE_RGB24;

    capsin = gst_caps_new_simple("video/x-raw-yuv",
                                 "format", GST_TYPE_FOURCC, amt->subtype.Data1,
                                 "width", G_TYPE_INT, width,
                                 "height", G_TYPE_INT, height,
                                 "framerate", GST_TYPE_FRACTION, 10000000, avgtime,
                                 NULL);
    capsout = gst_caps_new_simple("video/x-raw-rgb",
                                  "endianness", G_TYPE_INT, 4321,
                                  "width", G_TYPE_INT, width,
                                  "height", G_TYPE_INT, height,
                                  "framerate", GST_TYPE_FRACTION, 10000000, avgtime,
                                  "bpp", G_TYPE_INT, 24,
                                  "depth", G_TYPE_INT, 24,
                                  "red_mask", G_TYPE_INT, 0xff,
                                  "green_mask", G_TYPE_INT, 0xff00,
                                  "blue_mask", G_TYPE_INT, 0xff0000,
                                   NULL);

    hr = Gstreamer_transform_ConnectInput(This, amt, capsin, capsout);
    gst_caps_unref(capsin);
    gst_caps_unref(capsout);

    This->cbBuffer = width * height * 4;
    return hr;
}

static const TransformFilterFuncTable Gstreamer_YUV_vtbl = {
    Gstreamer_transform_DecideBufferSize,
    Gstreamer_transform_ProcessBegin,
    Gstreamer_transform_ProcessData,
    Gstreamer_transform_ProcessEnd,
    Gstreamer_YUV_QueryConnect,
    Gstreamer_YUV_SetMediaType,
    Gstreamer_YUV_ConnectInput,
    Gstreamer_transform_Cleanup,
    Gstreamer_transform_EndOfStream,
    Gstreamer_transform_BeginFlush,
    Gstreamer_transform_EndFlush,
    Gstreamer_transform_NewSegment
};

IUnknown * CALLBACK Gstreamer_YUV_create(IUnknown *punkout, HRESULT *phr)
{
    IUnknown *obj = NULL;
    if (!Gstreamer_init())
    {
        *phr = E_FAIL;
        return NULL;
    }
    *phr = Gstreamer_transform_create(punkout, &CLSID_Gstreamer_YUV, "ffmpegcolorspace", &Gstreamer_YUV_vtbl, (LPVOID*)&obj);
    return obj;
}

HRESULT WINAPI GSTTf_QueryInterface(IBaseFilter * iface, REFIID riid, LPVOID * ppv)
{
    HRESULT hr;
    GstTfImpl *This = (GstTfImpl*)iface;
    TRACE("(%p/%p)->(%s, %p)\n", This, iface, debugstr_guid(riid), ppv);

    if (IsEqualIID(riid, &IID_IMediaSeeking))
        return IUnknown_QueryInterface(This->seekthru_unk, riid, ppv);

    hr = TransformFilterImpl_QueryInterface(iface, riid, ppv);

    return hr;
}

static const IBaseFilterVtbl GSTTf_Vtbl =
{
    GSTTf_QueryInterface,
    BaseFilterImpl_AddRef,
    TransformFilterImpl_Release,
    BaseFilterImpl_GetClassID,
    TransformFilterImpl_Stop,
    TransformFilterImpl_Pause,
    TransformFilterImpl_Run,
    BaseFilterImpl_GetState,
    BaseFilterImpl_SetSyncSource,
    BaseFilterImpl_GetSyncSource,
    BaseFilterImpl_EnumPins,
    TransformFilterImpl_FindPin,
    BaseFilterImpl_QueryFilterInfo,
    BaseFilterImpl_JoinFilterGraph,
    BaseFilterImpl_QueryVendorInfo
};
