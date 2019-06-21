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

extern CLSID CLSID_QTVDecoder;

WINE_DEFAULT_DEBUG_CHANNEL(qtdecoder);

typedef struct QTVDecoderImpl
{
    TransformFilter tf;

    ImageDescriptionHandle hImageDescription;
    CFMutableDictionaryRef outputBufferAttributes;
    ICMDecompressionSessionRef decompressionSession;
    HRESULT decodeHR;

    DWORD outputSize;

} QTVDecoderImpl;

static inline QTVDecoderImpl *impl_from_IBaseFilter( IBaseFilter *iface )
{
    return CONTAINING_RECORD(iface, QTVDecoderImpl, tf.filter.IBaseFilter_iface);
}

static inline QTVDecoderImpl *impl_from_TransformFilter( TransformFilter *iface )
{
    return CONTAINING_RECORD(iface, QTVDecoderImpl, tf.filter);
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

    EnterCriticalSection(&This->tf.csReceive);
    hr = BaseOutputPinImpl_GetDeliveryBuffer(&This->tf.source, &pOutSample, NULL, NULL, 0);
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

    LeaveCriticalSection(&This->tf.csReceive);
    hr = BaseOutputPinImpl_Deliver(&This->tf.source, pOutSample);
    EnterCriticalSection(&This->tf.csReceive);
    if (hr != S_OK && hr != VFW_E_NOT_CONNECTED)
        ERR("Error sending sample (%x)\n", hr);

error:
    LeaveCriticalSection(&This->tf.csReceive);
    if (pOutSample)
        IMediaSample_Release(pOutSample);

    This->decodeHR = hr;
}

static HRESULT WINAPI QTVDecoder_StartStreaming(TransformFilter* pTransformFilter)
{
    QTVDecoderImpl* This = impl_from_TransformFilter(pTransformFilter);
    OSErr err = noErr;
    ICMDecompressionSessionOptionsRef sessionOptions = NULL;
    ICMDecompressionTrackingCallbackRecord trackingCallbackRecord;

    TRACE("(%p)->()\n", This);

    trackingCallbackRecord.decompressionTrackingCallback = trackingCallback;
    trackingCallbackRecord.decompressionTrackingRefCon = (void*)This;

    err = ICMDecompressionSessionCreate(NULL, This->hImageDescription, sessionOptions, This->outputBufferAttributes, &trackingCallbackRecord, &This->decompressionSession);

    if (err != noErr)
    {
        ERR("Error with ICMDecompressionSessionCreate %i\n",err);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI QTVDecoder_Receive(TransformFilter *tf, IMediaSample *pSample)
{
    QTVDecoderImpl* This = impl_from_TransformFilter(tf);
    HRESULT hr;
    DWORD cbSrcStream;
    LPBYTE pbSrcStream;

    ICMFrameTimeRecord frameTime = {{0}};
    TimeValue time = 1;
    TimeScale timeScale = 1;
    OSStatus err = noErr;
    LONGLONG tStart, tStop;

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
    return hr;
}

static HRESULT WINAPI QTVDecoder_StopStreaming(TransformFilter* pTransformFilter)
{
    QTVDecoderImpl* This = impl_from_TransformFilter(pTransformFilter);

    TRACE("(%p)->()\n", This);

    if (This->decompressionSession)
        ICMDecompressionSessionRelease(This->decompressionSession);
    This->decompressionSession = NULL;

    return S_OK;
}

static HRESULT WINAPI QTVDecoder_SetMediaType(TransformFilter *tf, PIN_DIRECTION dir, const AM_MEDIA_TYPE * pmt)
{
    QTVDecoderImpl* This = impl_from_TransformFilter(tf);
    HRESULT hr = VFW_E_TYPE_NOT_ACCEPTED;
    OSErr err = noErr;
    AM_MEDIA_TYPE *outpmt = &This->tf.pmt;
    CFNumberRef n = NULL;

    TRACE("(%p)->(%p)\n", This, pmt);

    if (dir != PINDIR_INPUT)
        return S_OK;

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

static HRESULT WINAPI QTVDecoder_BreakConnect(TransformFilter *tf, PIN_DIRECTION dir)
{
    QTVDecoderImpl *This = impl_from_TransformFilter(tf);

    TRACE("(%p)->()\n", This);

    if (This->hImageDescription)
        DisposeHandle((Handle)This->hImageDescription);
    if (This->outputBufferAttributes)
        CFRelease(This->outputBufferAttributes);

    This->hImageDescription = NULL;
    This->outputBufferAttributes = NULL;

    return S_OK;
}

static HRESULT WINAPI QTVDecoder_DecideBufferSize(TransformFilter *tf, IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest)
{
    QTVDecoderImpl *This = impl_from_TransformFilter(tf);
    ALLOCATOR_PROPERTIES actual;

    TRACE("()\n");

    if (!ppropInputRequest->cbAlign)
        ppropInputRequest->cbAlign = 1;

    if (ppropInputRequest->cbBuffer < This->outputSize)
            ppropInputRequest->cbBuffer = This->outputSize + ppropInputRequest->cbAlign;

    if (!ppropInputRequest->cBuffers)
        ppropInputRequest->cBuffers = 1;

    return IMemAllocator_SetProperties(pAlloc, ppropInputRequest, &actual);
}

static const TransformFilterFuncTable QTVDecoder_FuncsTable = {
    QTVDecoder_DecideBufferSize,
    QTVDecoder_StartStreaming,
    QTVDecoder_Receive,
    QTVDecoder_StopStreaming,
    NULL,
    QTVDecoder_SetMediaType,
    NULL,
    QTVDecoder_BreakConnect,
    NULL,
    NULL,
    NULL,
    NULL
};

IUnknown * CALLBACK QTVDecoder_create(IUnknown *outer, HRESULT* phr)
{
    HRESULT hr;
    QTVDecoderImpl * This;

    *phr = S_OK;

    hr = strmbase_transform_create(sizeof(QTVDecoderImpl), outer, &CLSID_QTVDecoder,
            &QTVDecoder_FuncsTable, (IBaseFilter **)&This);

    if (FAILED(hr))
    {
        *phr = hr;
        return NULL;
    }

    *phr = hr;
    return &This->tf.filter.IUnknown_inner;
}
