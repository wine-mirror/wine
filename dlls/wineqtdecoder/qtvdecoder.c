/*
 * QuickTime Toolkit decoder filter for video
 *
 * Copyright 2010 Aric Stewart, CodeWeavers
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

#define ULONG CoreFoundation_ULONG
#define HRESULT CoreFoundation_HRESULT

#define LoadResource __carbon_LoadResource
#define CompareString __carbon_CompareString
#define GetCurrentThread __carbon_GetCurrentThread
#define GetCurrentProcess __carbon_GetCurrentProcess
#define AnimatePalette __carbon_AnimatePalette
#define EqualRgn __carbon_EqualRgn
#define FillRgn __carbon_FillRgn
#define FrameRgn __carbon_FrameRgn
#define GetPixel __carbon_GetPixel
#define InvertRgn __carbon_InvertRgn
#define LineTo __carbon_LineTo
#define OffsetRgn __carbon_OffsetRgn
#define PaintRgn __carbon_PaintRgn
#define Polygon __carbon_Polygon
#define ResizePalette __carbon_ResizePalette
#define SetRectRgn __carbon_SetRectRgn

#define CheckMenuItem __carbon_CheckMenuItem
#define DeleteMenu __carbon_DeleteMenu
#define DrawMenuBar __carbon_DrawMenuBar
#define EnableMenuItem __carbon_EnableMenuItem
#define EqualRect __carbon_EqualRect
#define FillRect __carbon_FillRect
#define FrameRect __carbon_FrameRect
#define GetCursor __carbon_GetCursor
#define GetMenu __carbon_GetMenu
#define InvertRect __carbon_InvertRect
#define IsWindowVisible __carbon_IsWindowVisible
#define MoveWindow __carbon_MoveWindow
#define OffsetRect __carbon_OffsetRect
#define PtInRect __carbon_PtInRect
#define SetCursor __carbon_SetCursor
#define SetRect __carbon_SetRect
#define ShowCursor __carbon_ShowCursor
#define ShowWindow __carbon_ShowWindow
#define UnionRect __carbon_UnionRect

#include <QuickTime/ImageCompression.h>
#include <CoreVideo/CVPixelBuffer.h>

#undef LoadResource
#undef CompareString
#undef GetCurrentThread
#undef _CDECL
#undef GetCurrentProcess
#undef AnimatePalette
#undef EqualRgn
#undef FillRgn
#undef FrameRgn
#undef GetPixel
#undef InvertRgn
#undef LineTo
#undef OffsetRgn
#undef PaintRgn
#undef Polygon
#undef ResizePalette
#undef SetRectRgn
#undef CheckMenuItem
#undef DeleteMenu
#undef DrawMenuBar
#undef EnableMenuItem
#undef EqualRect
#undef FillRect
#undef FrameRect
#undef GetCursor
#undef GetMenu
#undef InvertRect
#undef IsWindowVisible
#undef MoveWindow
#undef OffsetRect
#undef PtInRect
#undef SetCursor
#undef SetRect
#undef ShowCursor
#undef ShowWindow
#undef UnionRect

#undef ULONG
#undef HRESULT
#undef STDMETHODCALLTYPE

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "winuser.h"
#include "dshow.h"

#include "uuids.h"
#include "amvideo.h"
#include "strmif.h"
#include "vfwmsgs.h"
#include "vfw.h"
#include "dvdmedia.h"

#include <assert.h>

#include "wine/unicode.h"
#include "wine/debug.h"
#include "wine/strmbase.h"

#include "qtprivate.h"
#include "wineqtdecoder_classes.h"

WINE_DEFAULT_DEBUG_CHANNEL(qtdecoder);

typedef struct QTVDecoderImpl
{
    struct strmbase_filter filter;
    CRITICAL_SECTION stream_cs;

    AM_MEDIA_TYPE mt;

    struct strmbase_source source;
    IUnknown *seeking;

    struct strmbase_sink sink;

    ImageDescriptionHandle hImageDescription;
    CFMutableDictionaryRef outputBufferAttributes;
    ICMDecompressionSessionRef decompressionSession;
    HRESULT decodeHR;

    DWORD outputSize;
} QTVDecoderImpl;

static inline QTVDecoderImpl *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, QTVDecoderImpl, filter);
}

static HRESULT video_decoder_sink_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    QTVDecoderImpl *filter = impl_from_strmbase_filter(iface->filter);

    if (IsEqualGUID(iid, &IID_IMemInputPin))
        *out = &filter->sink.IMemInputPin_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static void trackingCallback(
                    void *decompressionTrackingRefCon,
                    OSStatus result,
                    ICMDecompressionTrackingFlags decompressionTrackingFlags,
                    CVPixelBufferRef pixelBuffer,
                    TimeValue64 displayTime,
                    TimeValue64 displayDuration,
                    ICMValidTimeFlags validTimeFlags,
                    void *reserved,
                    void *sourceFrameRefCon )
{
    QTVDecoderImpl *This = (QTVDecoderImpl*)decompressionTrackingRefCon;
    IMediaSample *pSample = (IMediaSample*)sourceFrameRefCon;
    HRESULT hr  = S_OK;
    IMediaSample* pOutSample = NULL;
    LPBYTE pbDstStream;
    DWORD cbDstStream;

    if (result != noErr)
    {
        ERR("Error from Codec, no frame decompressed\n");
        return;
    }

    if (!pixelBuffer)
    {
        ERR("No pixel buffer, no frame decompressed\n");
        return;
    }

    EnterCriticalSection(&This->stream_cs);
    hr = BaseOutputPinImpl_GetDeliveryBuffer(&This->source, &pOutSample, NULL, NULL, 0);
    if (FAILED(hr)) {
        ERR("Unable to get delivery buffer (%x)\n", hr);
        goto error;
    }

    hr = IMediaSample_SetActualDataLength(pOutSample, 0);
    assert(hr == S_OK);

    hr = IMediaSample_GetPointer(pOutSample, &pbDstStream);
    if (FAILED(hr)) {
        ERR("Unable to get pointer to buffer (%x)\n", hr);
        goto error;
    }

    cbDstStream = IMediaSample_GetSize(pOutSample);
    if (cbDstStream < This->outputSize) {
        ERR("Sample size is too small %d < %d\n", cbDstStream, This->outputSize);
        hr = E_FAIL;
        goto error;
    }

    hr = AccessPixelBufferPixels(pixelBuffer, pbDstStream);
    if (FAILED(hr))
        goto error;

    IMediaSample_SetActualDataLength(pOutSample, This->outputSize);

    IMediaSample_SetPreroll(pOutSample, (IMediaSample_IsPreroll(pSample) == S_OK));
    IMediaSample_SetDiscontinuity(pOutSample, (IMediaSample_IsDiscontinuity(pSample) == S_OK));
    IMediaSample_SetSyncPoint(pOutSample, (IMediaSample_IsSyncPoint(pSample) == S_OK));

    if (!validTimeFlags)
        IMediaSample_SetTime(pOutSample, NULL, NULL);
    else
    {
        LONGLONG tStart, tStop;

        if (validTimeFlags & kICMValidTime_DisplayTimeStampIsValid)
            tStart = displayTime;
         else
            tStart = 0;
        if (validTimeFlags & kICMValidTime_DisplayDurationIsValid)
            tStop = tStart + displayDuration;
        else
            tStop = tStart;

        IMediaSample_SetTime(pOutSample, &tStart, &tStop);
    }

    hr = IMemInputPin_Receive(This->source.pMemInputPin, pOutSample);
    if (hr != S_OK && hr != VFW_E_NOT_CONNECTED)
        ERR("Error sending sample (%x)\n", hr);

error:
    LeaveCriticalSection(&This->stream_cs);
    if (pOutSample)
        IMediaSample_Release(pOutSample);

    This->decodeHR = hr;
}

static HRESULT WINAPI video_decoder_sink_Receive(struct strmbase_sink *iface, IMediaSample *pSample)
{
    QTVDecoderImpl *This = impl_from_strmbase_filter(iface->pin.filter);
    HRESULT hr;
    DWORD cbSrcStream;
    LPBYTE pbSrcStream;

    ICMFrameTimeRecord frameTime = {{0}};
    TimeValue time = 1;
    TimeScale timeScale = 1;
    OSStatus err = noErr;
    LONGLONG tStart, tStop;

    EnterCriticalSection(&This->stream_cs);

    hr = IMediaSample_GetPointer(pSample, &pbSrcStream);
    if (FAILED(hr))
    {
        ERR("Cannot get pointer to sample data (%x)\n", hr);
        goto error;
    }

    cbSrcStream = IMediaSample_GetActualDataLength(pSample);

    if (IMediaSample_GetTime(pSample, &tStart, &tStop) != S_OK)
        tStart = tStop = 0;

    time = tStart;

    frameTime.recordSize = sizeof(ICMFrameTimeRecord);
    *(TimeValue64 *)&frameTime.value = tStart;
    frameTime.scale = 1;
    frameTime.rate = fixed1;
    frameTime.duration = tStop - tStart;
    frameTime.frameNumber = 0;
    frameTime.flags = icmFrameTimeIsNonScheduledDisplayTime;

    err = ICMDecompressionSessionDecodeFrame(This->decompressionSession,
            (UInt8 *)pbSrcStream, cbSrcStream, NULL, &frameTime, pSample);

    if (err != noErr)
    {
        ERR("Error with ICMDecompressionSessionDecodeFrame\n");
        hr = E_FAIL;
        goto error;
    }

    ICMDecompressionSessionSetNonScheduledDisplayTime(This->decompressionSession, time, timeScale, 0);
    ICMDecompressionSessionFlush(This->decompressionSession);
    hr = This->decodeHR;

error:
    LeaveCriticalSection(&This->stream_cs);
    return hr;
}

static HRESULT video_decoder_sink_connect(struct strmbase_sink *iface, IPin *peer, const AM_MEDIA_TYPE *pmt)
{
    QTVDecoderImpl *This = impl_from_strmbase_filter(iface->pin.filter);
    HRESULT hr = VFW_E_TYPE_NOT_ACCEPTED;
    OSErr err = noErr;
    AM_MEDIA_TYPE *outpmt = &This->mt;
    CFNumberRef n = NULL;

    FreeMediaType(outpmt);
    CopyMediaType(outpmt, pmt);

    if (This->hImageDescription)
        DisposeHandle((Handle)This->hImageDescription);

    This->hImageDescription = (ImageDescriptionHandle)
        NewHandleClear(sizeof(ImageDescription));

    if (This->hImageDescription != NULL)
    {
        (**This->hImageDescription).idSize = sizeof(ImageDescription);
        (**This->hImageDescription).spatialQuality = codecNormalQuality;
        (**This->hImageDescription).frameCount = 1;
        (**This->hImageDescription).clutID = -1;
    }
    else
    {
        ERR("Failed to create ImageDescription\n");
        goto failed;
    }

    /* Check root (GUID w/o FOURCC) */
    if ((IsEqualIID(&pmt->majortype, &MEDIATYPE_Video)) &&
        (!memcmp(((const char *)&pmt->subtype)+4, ((const char *)&MEDIATYPE_Video)+4, sizeof(GUID)-4)))
    {
        VIDEOINFOHEADER *format1 = (VIDEOINFOHEADER *)outpmt->pbFormat;
        VIDEOINFOHEADER2 *format2 = (VIDEOINFOHEADER2 *)outpmt->pbFormat;
        BITMAPINFOHEADER *bmi;
        OSType fourCC;
        DecompressorComponent dc;
        OSType format;
        DWORD outputWidth, outputHeight, outputDepth;

        if (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo))
            bmi = &format1->bmiHeader;
        else if (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo2))
            bmi = &format2->bmiHeader;
        else
            goto failed;

        TRACE("Fourcc: %s\n", debugstr_an((const char *)&pmt->subtype.Data1, 4));
        fourCC = ((const char *)&pmt->subtype.Data1)[3] |
                 (((const char *)&pmt->subtype.Data1)[2]<<8) |
                 (((const char *)&pmt->subtype.Data1)[1]<<16) |
                 (((const char *)&pmt->subtype.Data1)[0]<<24);

        err = FindCodec(fourCC,NULL,NULL,&dc);
        if (err != noErr || dc == 0x0)
        {
            TRACE("Codec not found\n");
            goto failed;
        }

        outputWidth = bmi->biWidth;
        outputHeight = bmi->biHeight;

        (**This->hImageDescription).cType = fourCC;
        (**This->hImageDescription).width = outputWidth;
        (**This->hImageDescription).height = outputHeight;
        (**This->hImageDescription).depth = bmi->biBitCount;
        (**This->hImageDescription).hRes = 72<<16;
        (**This->hImageDescription).vRes = 72<<16;

        if (This->outputBufferAttributes)
            CFRelease(This->outputBufferAttributes);

        This->outputBufferAttributes = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        if (!This->outputBufferAttributes)
        {
            ERR("Failed to create outputBufferAttributes\n");
            goto failed;
        }

        n = CFNumberCreate(NULL, kCFNumberIntType, &outputWidth);
        CFDictionaryAddValue(This->outputBufferAttributes, kCVPixelBufferWidthKey, n);
        CFRelease(n);

        n = CFNumberCreate(NULL, kCFNumberIntType, &outputHeight);
        CFDictionaryAddValue(This->outputBufferAttributes, kCVPixelBufferHeightKey, n);
        CFRelease(n);

        /* yes this looks wrong.  but 32ARGB is 24 RGB with an alpha channel */
        format = k32ARGBPixelFormat;
        n = CFNumberCreate(NULL, kCFNumberIntType, &format);
        CFDictionaryAddValue(This->outputBufferAttributes, kCVPixelBufferPixelFormatTypeKey, n);
        CFRelease(n);

        CFDictionaryAddValue(This->outputBufferAttributes, kCVPixelBufferCGBitmapContextCompatibilityKey, kCFBooleanTrue);
        CFDictionaryAddValue(This->outputBufferAttributes, kCVPixelBufferCGImageCompatibilityKey, kCFBooleanTrue);

        outputDepth = 3;
        This->outputSize = outputWidth * outputHeight * outputDepth;
        bmi->biCompression =  BI_RGB;
        bmi->biBitCount =  24;
        outpmt->subtype = MEDIASUBTYPE_RGB24;

        return S_OK;
    }

failed:
    if (This->hImageDescription)
    {
        DisposeHandle((Handle)This->hImageDescription);
        This->hImageDescription =  NULL;
    }
    if (This->outputBufferAttributes)
    {
        CFRelease(This->outputBufferAttributes);
        This->outputBufferAttributes = NULL;
    }

    TRACE("Connection refused\n");
    return hr;
}

static void video_decoder_sink_disconnect(struct strmbase_sink *iface)
{
    QTVDecoderImpl *This = impl_from_strmbase_filter(iface->pin.filter);

    if (This->hImageDescription)
        DisposeHandle((Handle)This->hImageDescription);
    if (This->outputBufferAttributes)
        CFRelease(This->outputBufferAttributes);

    This->hImageDescription = NULL;
    This->outputBufferAttributes = NULL;
}

static const struct strmbase_sink_ops sink_ops =
{
    .base.pin_query_interface = video_decoder_sink_query_interface,
    .pfnReceive = video_decoder_sink_Receive,
    .sink_connect = video_decoder_sink_connect,
    .sink_disconnect = video_decoder_sink_disconnect,
};

static HRESULT video_decoder_source_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    QTVDecoderImpl *filter = impl_from_strmbase_filter(iface->filter);

    if (IsEqualGUID(iid, &IID_IMediaSeeking))
        return IUnknown_QueryInterface(filter->seeking, iid, out);
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT video_decoder_source_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    QTVDecoderImpl *filter = impl_from_strmbase_filter(iface->filter);

    if (IsEqualGUID(&mt->majortype, &filter->mt.majortype)
            && (IsEqualGUID(&mt->subtype, &filter->mt.subtype)
            || IsEqualGUID(&filter->mt.subtype, &GUID_NULL)))
        return S_OK;
    return S_FALSE;
}

static HRESULT video_decoder_source_get_media_type(struct strmbase_pin *iface,
        unsigned int index, AM_MEDIA_TYPE *mt)
{
    QTVDecoderImpl *filter = impl_from_strmbase_filter(iface->filter);

    if (index)
        return VFW_S_NO_MORE_ITEMS;
    CopyMediaType(mt, &filter->mt);
    return S_OK;
}

static HRESULT WINAPI video_decoder_source_DecideBufferSize(struct strmbase_source *iface,
        IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest)
{
    QTVDecoderImpl *This = impl_from_strmbase_filter(iface->pin.filter);
    ALLOCATOR_PROPERTIES actual;

    if (!ppropInputRequest->cbAlign)
        ppropInputRequest->cbAlign = 1;

    if (ppropInputRequest->cbBuffer < This->outputSize)
            ppropInputRequest->cbBuffer = This->outputSize + ppropInputRequest->cbAlign;

    if (!ppropInputRequest->cBuffers)
        ppropInputRequest->cBuffers = 1;

    return IMemAllocator_SetProperties(pAlloc, ppropInputRequest, &actual);
}

static const struct strmbase_source_ops source_ops =
{
    .base.pin_query_interface = video_decoder_source_query_interface,
    .base.pin_query_accept = video_decoder_source_query_accept,
    .base.pin_get_media_type = video_decoder_source_get_media_type,
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideAllocator = BaseOutputPinImpl_DecideAllocator,
    .pfnDecideBufferSize = video_decoder_source_DecideBufferSize,
};

static struct strmbase_pin *video_decoder_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    QTVDecoderImpl *filter = impl_from_strmbase_filter(iface);

    if (index == 0)
        return &filter->sink.pin;
    else if (index == 1)
        return &filter->source.pin;
    return NULL;
}

static void video_decoder_destroy(struct strmbase_filter *iface)
{
    QTVDecoderImpl *filter = impl_from_strmbase_filter(iface);

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
}

static HRESULT video_decoder_init_stream(struct strmbase_filter *iface)
{
    QTVDecoderImpl *filter = impl_from_strmbase_filter(iface);
    HRESULT hr;

    OSErr err = noErr;
    ICMDecompressionSessionOptionsRef sessionOptions = NULL;
    ICMDecompressionTrackingCallbackRecord trackingCallbackRecord;

    trackingCallbackRecord.decompressionTrackingCallback = trackingCallback;
    trackingCallbackRecord.decompressionTrackingRefCon = filter;

    err = ICMDecompressionSessionCreate(NULL, filter->hImageDescription, sessionOptions,
            filter->outputBufferAttributes, &trackingCallbackRecord, &filter->decompressionSession);

    if (err != noErr)
    {
        ERR("Error with ICMDecompressionSessionCreate %i\n",err);
        return E_FAIL;
    }

    if (filter->source.pin.peer && FAILED(hr = IMemAllocator_Commit(filter->source.pAllocator)))
        ERR("Failed to commit allocator, hr %#x.\n", hr);

    return S_OK;
}

static HRESULT video_decoder_cleanup_stream(struct strmbase_filter *iface)
{
    QTVDecoderImpl *filter = impl_from_strmbase_filter(iface);

    if (filter->decompressionSession)
        ICMDecompressionSessionRelease(filter->decompressionSession);
    filter->decompressionSession = NULL;

    if (filter->source.pin.peer)
        IMemAllocator_Decommit(filter->source.pAllocator);

    return S_OK;
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = video_decoder_get_pin,
    .filter_destroy = video_decoder_destroy,
    .filter_init_stream = video_decoder_init_stream,
    .filter_cleanup_stream = video_decoder_cleanup_stream,
};

HRESULT video_decoder_create(IUnknown *outer, IUnknown **out)
{
    static const WCHAR inW[] = { 'I','n',0 };
    static const WCHAR outW[] = { 'O','u','t',0 };
    QTVDecoderImpl *object;
    HRESULT hr;
    ISeekingPassThru *passthrough;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_filter_init(&object->filter, outer, &CLSID_QTVDecoder, &filter_ops);

    InitializeCriticalSection(&object->stream_cs);
    object->stream_cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__": QTVDecoderImpl.stream_cs");

    strmbase_sink_init(&object->sink, &object->filter, inW, &sink_ops, NULL);

    strmbase_source_init(&object->source, &object->filter, outW, &source_ops);

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

    TRACE("Created video decoder %p.\n", object);
    *out = &object->filter.IUnknown_inner;
    return S_OK;
}
