/*
 * AVI Decompressor (VFW decompressors wrapper)
 *
 * Copyright 2004-2005 Christian Costa
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

#include "quartz_private.h"

#include "uuids.h"
#include "amvideo.h"
#include "windef.h"
#include "winbase.h"
#include "dshow.h"
#include "strmif.h"
#include "vfwmsgs.h"
#include "vfw.h"
#include "dvdmedia.h"

#include <assert.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

typedef struct AVIDecImpl
{
    struct strmbase_filter filter;
    CRITICAL_SECTION stream_cs;

    struct strmbase_source source;
    IQualityControl source_IQualityControl_iface;
    IUnknown *seeking;

    struct strmbase_sink sink;

    AM_MEDIA_TYPE mt;
    HIC hvid;
    BITMAPINFOHEADER* pBihIn;
    BITMAPINFOHEADER* pBihOut;
    REFERENCE_TIME late;
} AVIDecImpl;

static AVIDecImpl *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, AVIDecImpl, filter);
}

static HRESULT avi_decompressor_sink_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    AVIDecImpl *filter = impl_from_strmbase_filter(iface->filter);

    if (IsEqualGUID(iid, &IID_IMemInputPin))
        *out = &filter->sink.IMemInputPin_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT avi_decompressor_sink_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    return S_OK;
}

static HRESULT avi_decompressor_sink_end_flush(struct strmbase_sink *iface)
{
    AVIDecImpl *filter = impl_from_strmbase_filter(iface->pin.filter);
    filter->late = -1;
    if (filter->source.pin.peer)
        return IPin_EndFlush(filter->source.pin.peer);
    return S_OK;
}

static int AVIDec_DropSample(AVIDecImpl *This, REFERENCE_TIME tStart) {
    if (This->late < 0)
        return 0;

    if (tStart < This->late) {
        TRACE("Dropping sample\n");
        return 1;
    }
    This->late = -1;
    return 0;
}

static HRESULT WINAPI avi_decompressor_sink_Receive(struct strmbase_sink *iface, IMediaSample *pSample)
{
    AVIDecImpl *This = impl_from_strmbase_filter(iface->pin.filter);
    HRESULT hr;
    DWORD res;
    IMediaSample* pOutSample = NULL;
    DWORD cbDstStream;
    LPBYTE pbDstStream;
    DWORD cbSrcStream;
    LPBYTE pbSrcStream;
    LONGLONG tStart, tStop;
    DWORD flags = 0;

    /* We do not expect pin connection state to change while the filter is
     * running. This guarantee is necessary, since otherwise we would have to
     * take the filter lock, and we can't take the filter lock from a streaming
     * thread. */
    if (!This->source.pMemInputPin)
    {
        WARN("Source is not connected, returning VFW_E_NOT_CONNECTED.\n");
        return VFW_E_NOT_CONNECTED;
    }

    if (This->filter.state == State_Stopped)
        return VFW_E_WRONG_STATE;

    if (This->sink.flushing)
        return S_FALSE;

    EnterCriticalSection(&This->stream_cs);

    hr = IMediaSample_GetPointer(pSample, &pbSrcStream);
    if (FAILED(hr))
    {
        ERR("Cannot get pointer to sample data (%x)\n", hr);
        LeaveCriticalSection(&This->stream_cs);
        return hr;
    }

    cbSrcStream = IMediaSample_GetActualDataLength(pSample);

    TRACE("Sample data ptr = %p, size = %d\n", pbSrcStream, cbSrcStream);

    /* Update input size to match sample size */
    This->pBihIn->biSizeImage = cbSrcStream;

    hr = BaseOutputPinImpl_GetDeliveryBuffer(&This->source, &pOutSample, NULL, NULL, 0);
    if (FAILED(hr)) {
        ERR("Unable to get delivery buffer (%x)\n", hr);
        LeaveCriticalSection(&This->stream_cs);
        return hr;
    }

    hr = IMediaSample_SetActualDataLength(pOutSample, 0);
    assert(hr == S_OK);

    hr = IMediaSample_GetPointer(pOutSample, &pbDstStream);
    if (FAILED(hr)) {
	ERR("Unable to get pointer to buffer (%x)\n", hr);
        IMediaSample_Release(pOutSample);
        LeaveCriticalSection(&This->stream_cs);
        return hr;
    }
    cbDstStream = IMediaSample_GetSize(pOutSample);
    if (cbDstStream < This->pBihOut->biSizeImage) {
        ERR("Sample size is too small %d < %d\n", cbDstStream, This->pBihOut->biSizeImage);
        IMediaSample_Release(pOutSample);
        LeaveCriticalSection(&This->stream_cs);
        return E_FAIL;
    }

    if (IMediaSample_IsPreroll(pSample) == S_OK)
        flags |= ICDECOMPRESS_PREROLL;
    if (IMediaSample_IsSyncPoint(pSample) != S_OK)
        flags |= ICDECOMPRESS_NOTKEYFRAME;
    hr = IMediaSample_GetTime(pSample, &tStart, &tStop);
    if (hr == S_OK && AVIDec_DropSample(This, tStart))
        flags |= ICDECOMPRESS_HURRYUP;

    res = ICDecompress(This->hvid, flags, This->pBihIn, pbSrcStream, This->pBihOut, pbDstStream);
    if (res != ICERR_OK)
        ERR("Error occurred during the decompression (%x)\n", res);

    /* Drop sample if it's intended to be dropped */
    if (flags & ICDECOMPRESS_HURRYUP) {
        IMediaSample_Release(pOutSample);
        LeaveCriticalSection(&This->stream_cs);
        return S_OK;
    }

    IMediaSample_SetActualDataLength(pOutSample, This->pBihOut->biSizeImage);

    IMediaSample_SetPreroll(pOutSample, (IMediaSample_IsPreroll(pSample) == S_OK));
    IMediaSample_SetDiscontinuity(pOutSample, (IMediaSample_IsDiscontinuity(pSample) == S_OK));
    IMediaSample_SetSyncPoint(pOutSample, (IMediaSample_IsSyncPoint(pSample) == S_OK));

    if (hr == S_OK)
        IMediaSample_SetTime(pOutSample, &tStart, &tStop);
    else if (hr == VFW_S_NO_STOP_TIME)
        IMediaSample_SetTime(pOutSample, &tStart, NULL);
    else
        IMediaSample_SetTime(pOutSample, NULL, NULL);

    if (IMediaSample_GetMediaTime(pSample, &tStart, &tStop) == S_OK)
        IMediaSample_SetMediaTime(pOutSample, &tStart, &tStop);
    else
        IMediaSample_SetMediaTime(pOutSample, NULL, NULL);

    hr = IMemInputPin_Receive(This->source.pMemInputPin, pOutSample);
    if (hr != S_OK && hr != VFW_E_NOT_CONNECTED)
        ERR("Error sending sample (%x)\n", hr);

    IMediaSample_Release(pOutSample);
    LeaveCriticalSection(&This->stream_cs);
    return hr;
}

static HRESULT avi_decompressor_sink_connect(struct strmbase_sink *iface, IPin *peer, const AM_MEDIA_TYPE *pmt)
{
    AVIDecImpl *This = impl_from_strmbase_filter(iface->pin.filter);
    HRESULT hr = VFW_E_TYPE_NOT_ACCEPTED;

    /* Check root (GUID w/o FOURCC) */
    if ((IsEqualIID(&pmt->majortype, &MEDIATYPE_Video)) &&
        (!memcmp(((const char *)&pmt->subtype)+4, ((const char *)&MEDIATYPE_Video)+4, sizeof(GUID)-4)))
    {
        VIDEOINFOHEADER *format1 = (VIDEOINFOHEADER *)pmt->pbFormat;
        VIDEOINFOHEADER2 *format2 = (VIDEOINFOHEADER2 *)pmt->pbFormat;
        BITMAPINFOHEADER *bmi;

        if (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo))
            bmi = &format1->bmiHeader;
        else if (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo2))
            bmi = &format2->bmiHeader;
        else
            goto failed;

        This->hvid = ICLocate(pmt->majortype.Data1, pmt->subtype.Data1, bmi, NULL, ICMODE_DECOMPRESS);
        if (This->hvid)
        {
            const CLSID* outsubtype;
            DWORD bih_size;
            DWORD output_depth = bmi->biBitCount;
            DWORD result;
            FreeMediaType(&This->mt);

            switch(bmi->biBitCount)
            {
                case 32: outsubtype = &MEDIASUBTYPE_RGB32; break;
                case 24: outsubtype = &MEDIASUBTYPE_RGB24; break;
                case 16: outsubtype = &MEDIASUBTYPE_RGB565; break;
                case 8:  outsubtype = &MEDIASUBTYPE_RGB8; break;
                default:
                    WARN("Non standard input depth %d, forced output depth to 32\n", bmi->biBitCount);
                    outsubtype = &MEDIASUBTYPE_RGB32;
                    output_depth = 32;
                    break;
            }

            /* Copy bitmap header from media type to 1 for input and 1 for output */
            bih_size = bmi->biSize + bmi->biClrUsed * 4;
            This->pBihIn = CoTaskMemAlloc(bih_size);
            if (!This->pBihIn)
            {
                hr = E_OUTOFMEMORY;
                goto failed;
            }
            This->pBihOut = CoTaskMemAlloc(bih_size);
            if (!This->pBihOut)
            {
                hr = E_OUTOFMEMORY;
                goto failed;
            }
            memcpy(This->pBihIn, bmi, bih_size);
            memcpy(This->pBihOut, bmi, bih_size);

            /* Update output format as non compressed bitmap */
            This->pBihOut->biCompression = 0;
            This->pBihOut->biBitCount = output_depth;
            This->pBihOut->biSizeImage = This->pBihOut->biWidth * This->pBihOut->biHeight * This->pBihOut->biBitCount / 8;
            TRACE("Size: %u\n", This->pBihIn->biSize);
            result = ICDecompressQuery(This->hvid, This->pBihIn, This->pBihOut);
            if (result != ICERR_OK)
            {
                ERR("Unable to found a suitable output format (%d)\n", result);
                goto failed;
            }

            /* Update output media type */
            CopyMediaType(&This->mt, pmt);
            This->mt.subtype = *outsubtype;

            if (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo))
                memcpy(&(((VIDEOINFOHEADER *)This->mt.pbFormat)->bmiHeader), This->pBihOut, This->pBihOut->biSize);
            else if (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo2))
                memcpy(&(((VIDEOINFOHEADER2 *)This->mt.pbFormat)->bmiHeader), This->pBihOut, This->pBihOut->biSize);
            else
                assert(0);

            TRACE("Connection accepted\n");
            return S_OK;
        }
        TRACE("Unable to find a suitable VFW decompressor\n");
    }

failed:

    TRACE("Connection refused\n");
    return hr;
}

static void avi_decompressor_sink_disconnect(struct strmbase_sink *iface)
{
    AVIDecImpl *filter = impl_from_strmbase_filter(iface->pin.filter);

    if (filter->hvid)
        ICClose(filter->hvid);
    CoTaskMemFree(filter->pBihIn);
    CoTaskMemFree(filter->pBihOut);
    filter->hvid = NULL;
    filter->pBihIn = NULL;
    filter->pBihOut = NULL;
}

static const struct strmbase_sink_ops sink_ops =
{
    .base.pin_query_interface = avi_decompressor_sink_query_interface,
    .base.pin_query_accept = avi_decompressor_sink_query_accept,
    .base.pin_get_media_type = strmbase_pin_get_media_type,
    .pfnReceive = avi_decompressor_sink_Receive,
    .sink_connect = avi_decompressor_sink_connect,
    .sink_disconnect = avi_decompressor_sink_disconnect,
    .sink_end_flush = avi_decompressor_sink_end_flush,
};

static HRESULT avi_decompressor_source_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    AVIDecImpl *filter = impl_from_strmbase_filter(iface->filter);

    if (IsEqualGUID(iid, &IID_IQualityControl))
        *out = &filter->source_IQualityControl_iface;
    else if (IsEqualGUID(iid, &IID_IMediaSeeking))
        return IUnknown_QueryInterface(filter->seeking, iid, out);
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT avi_decompressor_source_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    AVIDecImpl *filter = impl_from_strmbase_filter(iface->filter);

    if (IsEqualGUID(&mt->majortype, &filter->mt.majortype)
            && (IsEqualGUID(&mt->subtype, &filter->mt.subtype)
            || IsEqualGUID(&filter->mt.subtype, &GUID_NULL)))
        return S_OK;
    return S_FALSE;
}

static HRESULT avi_decompressor_source_get_media_type(struct strmbase_pin *iface,
        unsigned int index, AM_MEDIA_TYPE *mt)
{
    AVIDecImpl *filter = impl_from_strmbase_filter(iface->filter);

    if (index)
        return VFW_S_NO_MORE_ITEMS;
    CopyMediaType(mt, &filter->mt);
    return S_OK;
}

static HRESULT WINAPI avi_decompressor_source_DecideBufferSize(struct strmbase_source *iface,
        IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest)
{
    AVIDecImpl *pAVI = impl_from_strmbase_filter(iface->pin.filter);
    ALLOCATOR_PROPERTIES actual;

    if (!ppropInputRequest->cbAlign)
        ppropInputRequest->cbAlign = 1;

    if (ppropInputRequest->cbBuffer < pAVI->pBihOut->biSizeImage)
            ppropInputRequest->cbBuffer = pAVI->pBihOut->biSizeImage;

    if (!ppropInputRequest->cBuffers)
        ppropInputRequest->cBuffers = 1;

    return IMemAllocator_SetProperties(pAlloc, ppropInputRequest, &actual);
}

static const struct strmbase_source_ops source_ops =
{
    .base.pin_query_interface = avi_decompressor_source_query_interface,
    .base.pin_query_accept = avi_decompressor_source_query_accept,
    .base.pin_get_media_type = avi_decompressor_source_get_media_type,
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideAllocator = BaseOutputPinImpl_DecideAllocator,
    .pfnDecideBufferSize = avi_decompressor_source_DecideBufferSize,
};

static AVIDecImpl *impl_from_source_IQualityControl(IQualityControl *iface)
{
    return CONTAINING_RECORD(iface, AVIDecImpl, source_IQualityControl_iface);
}

static HRESULT WINAPI acm_wrapper_source_qc_QueryInterface(IQualityControl *iface,
        REFIID iid, void **out)
{
    AVIDecImpl *filter = impl_from_source_IQualityControl(iface);
    return IPin_QueryInterface(&filter->source.pin.IPin_iface, iid, out);
}

static ULONG WINAPI acm_wrapper_source_qc_AddRef(IQualityControl *iface)
{
    AVIDecImpl *filter = impl_from_source_IQualityControl(iface);
    return IPin_AddRef(&filter->source.pin.IPin_iface);
}

static ULONG WINAPI acm_wrapper_source_qc_Release(IQualityControl *iface)
{
    AVIDecImpl *filter = impl_from_source_IQualityControl(iface);
    return IPin_Release(&filter->source.pin.IPin_iface);
}

static HRESULT WINAPI acm_wrapper_source_qc_Notify(IQualityControl *iface,
        IBaseFilter *sender, Quality q)
{
    AVIDecImpl *filter = impl_from_source_IQualityControl(iface);

    TRACE("filter %p, sender %p, type %#x, proportion %u, late %s, timestamp %s.\n",
            filter, sender, q.Type, q.Proportion, debugstr_time(q.Late), debugstr_time(q.TimeStamp));

    EnterCriticalSection(&filter->stream_cs);
    if (q.Late > 0)
        filter->late = q.Late + q.TimeStamp;
    else
        filter->late = -1;
    LeaveCriticalSection(&filter->stream_cs);
    return S_OK;
}

static HRESULT WINAPI acm_wrapper_source_qc_SetSink(IQualityControl *iface, IQualityControl *sink)
{
    AVIDecImpl *filter = impl_from_source_IQualityControl(iface);

    TRACE("filter %p, sink %p.\n", filter, sink);

    return S_OK;
}

static const IQualityControlVtbl source_qc_vtbl =
{
    acm_wrapper_source_qc_QueryInterface,
    acm_wrapper_source_qc_AddRef,
    acm_wrapper_source_qc_Release,
    acm_wrapper_source_qc_Notify,
    acm_wrapper_source_qc_SetSink,
};

static struct strmbase_pin *avi_decompressor_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    AVIDecImpl *filter = impl_from_strmbase_filter(iface);

    if (index == 0)
        return &filter->sink.pin;
    else if (index == 1)
        return &filter->source.pin;
    return NULL;
}

static void avi_decompressor_destroy(struct strmbase_filter *iface)
{
    AVIDecImpl *filter = impl_from_strmbase_filter(iface);

    if (filter->sink.pin.peer)
        IPin_Disconnect(filter->sink.pin.peer);
    IPin_Disconnect(&filter->sink.pin.IPin_iface);

    if (filter->source.pin.peer)
        IPin_Disconnect(filter->source.pin.peer);
    IPin_Disconnect(&filter->source.pin.IPin_iface);

    strmbase_sink_cleanup(&filter->sink);
    strmbase_source_cleanup(&filter->source);

    filter->stream_cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&filter->stream_cs);
    FreeMediaType(&filter->mt);
    IUnknown_Release(filter->seeking);
    strmbase_filter_cleanup(&filter->filter);
    free(filter);

    InterlockedDecrement(&object_locks);
}

static HRESULT avi_decompressor_init_stream(struct strmbase_filter *iface)
{
    AVIDecImpl *filter = impl_from_strmbase_filter(iface);
    LRESULT res;

    filter->late = -1;

    if ((res = ICDecompressBegin(filter->hvid, filter->pBihIn, filter->pBihOut)))
    {
        ERR("ICDecompressBegin() failed, error %ld.\n", res);
        return E_FAIL;
    }

    BaseOutputPinImpl_Active(&filter->source);
    return S_OK;
}

static HRESULT avi_decompressor_cleanup_stream(struct strmbase_filter *iface)
{
    AVIDecImpl *filter = impl_from_strmbase_filter(iface);
    LRESULT res;

    if (filter->hvid && (res = ICDecompressEnd(filter->hvid)))
    {
        ERR("ICDecompressEnd() failed, error %ld.\n", res);
        return E_FAIL;
    }

    BaseOutputPinImpl_Inactive(&filter->source);
    return S_OK;
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = avi_decompressor_get_pin,
    .filter_destroy = avi_decompressor_destroy,
    .filter_init_stream = avi_decompressor_init_stream,
    .filter_cleanup_stream = avi_decompressor_cleanup_stream,
};

HRESULT avi_dec_create(IUnknown *outer, IUnknown **out)
{
    AVIDecImpl *object;
    ISeekingPassThru *passthrough;
    HRESULT hr;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_filter_init(&object->filter, outer, &CLSID_AVIDec, &filter_ops);

    InitializeCriticalSection(&object->stream_cs);
    object->stream_cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__": avi_decompressor.stream_cs");

    strmbase_sink_init(&object->sink, &object->filter, L"In", &sink_ops, NULL);

    strmbase_source_init(&object->source, &object->filter, L"Out", &source_ops);
    object->source_IQualityControl_iface.lpVtbl = &source_qc_vtbl;

    if (FAILED(hr = CoCreateInstance(&CLSID_SeekingPassThru,
            (IUnknown *)&object->source.pin.IPin_iface, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void **)&object->seeking)))
    {
        strmbase_sink_cleanup(&object->sink);
        strmbase_source_cleanup(&object->source);
        strmbase_filter_cleanup(&object->filter);
        free(object);
        return hr;
    }

    IUnknown_QueryInterface(object->seeking, &IID_ISeekingPassThru, (void **)&passthrough);
    ISeekingPassThru_Init(passthrough, FALSE, &object->sink.pin.IPin_iface);
    ISeekingPassThru_Release(passthrough);

    TRACE("Created AVI decompressor %p.\n", object);
    *out = &object->filter.IUnknown_inner;

    return S_OK;
}
